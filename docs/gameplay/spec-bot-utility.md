# Spec — Bot: utility scoring multi-fattore

> ⚠️ **Superato dal pivot esagonale** ([ADR-0002](../decisions/adr-0002-griglia-esagonale.md)) — **riferimento storico, non normativo.**
> Descrive il substrato **quadrato**, rimosso dal codice al **CP 7.2** (`Grid/`, `URTGridLibrary`, `FRTGridCoord`, resolver e bot quadrati). Il bot autorevole è oggi `URTHexBotLibrary`: i cinque test d'integrazione sono stati **portati** su hex al CP 6.6, non cancellati.
> Conservato per provenienza e come comportamento di riferimento della parità hex (M6). Punto di ritorno: tag `pre-hex-only`.

> `/sc:spec-panel` del **2026-08-04**. Panel: **Fowler** (architettura), **Wiegers** (requisiti), **Adzic**
> (esempi), **Crispin** (determinismo/test), **Nygard** (robustezza). Documentale.
> Base: **`feat/bot-utility` da `feat/skeletal-units`** (bot aggiornato + **griglia quadrata stabile**), *non*
> `feat/hex-grid` (griglia in rework) né `main` (~800 righe indietro). Lavoro in **worktree isolato** (editor
> occupato dall'hex). Ancorata a `RTBotLibrary` e `ARTTurnManager::PlanBots`.

## 1. Obiettivo
Sostituire la **pipeline a priorità fissa** di `PlanBots` con un **utility scoring**: il bot genera le mosse
candidate e sceglie quella a **punteggio massimo**, pesando danno/kill, minaccia subita, kiting, copertura,
focus-fire, posizionamento. Comportamento **emergente e bilanciabile**, non più soglie hardcoded a cascata.

## 2. Stato verificato (dal codice)
- `PlanBots` (`RTTurnManager.cpp:49-~240`): catena `if/else if` con soglie fisse (`bPanic = dist ≤ KiteStandoff/2`,
  scatto difensivo, firing→approach→kite…). Articolata ma rigida, difficile da bilanciare.
- Euristiche **pure** già pronte in `RTBotLibrary`: `AttackScore`, `BestApproachCell`, `BestKiteCell`,
  `BestFiringCell` → riusabili come **generatori di candidate** e come fattori.
- Test bot esistenti: `RTBotLibraryTests.cpp` (da mantenere verdi).

## 3. Il panel
- **Fowler**: `URTBotLibrary::ScorePlan(Plan, Context) → int32` (pura). `PlanBots` diventa *"genera candidate →
  scegli max"*; le euristiche restano come generatori. Separa decisione da esecuzione.
- **Wiegers**: fattori **misurabili** con **pesi espliciti** (interi), ognuno con criterio.
- **Adzic** (esempi): kill disponibile → l'azione-kill vince; Ranger ferito con nemico entro standoff → vince la
  cella di kiting; copertura a tiro → vince il posizionamento.
- **Crispin**: determinismo (stessi input → stessa scelta); un test per fattore; **permutare le candidate non
  cambia l'esito**.
- **Nygard**: punteggi **interi** (invariante #4, niente float/hash), **tie-break assoluto** (id/coord), fallback
  = fermo se nessuna candidata.

## 4. Requisiti
- **FR-BOT-U1** — `ScorePlan` pura, punteggio **intero** multi-fattore, deterministica.
- **FR-BOT-U2** — `PlanBots` enumera candidate (attacco/move/dash/support) e sceglie il max con tie-break assoluto.
- **FR-BOT-U3** — pesi come **parametri interi** (bilanciabili senza toccare la logica).
- **FR-BOT-U4** — **nessuna regressione** dei test bot esistenti (o aggiornati con motivazione).

## 5. Modello & fattori (BU.1)
```cpp
FRTBotPlan   { DestCell; bHasAttack; AttackDamage; TargetHealth; }
FRTBotContext{ Enemies[]; EnemyRanges[]; KiteStandoff; WKill/WDamage/WThreat/WKiteViolation/WApproach; }
int32 ScorePlan(Plan, Context)
```
- **Focus-fire**: `+WDamage·danno`; se `danno ≥ TargetHealth` → `+WKill` (bonus kill).
- **Minaccia**: `−WThreat` per ogni nemico che può colpire `DestCell` (dist ≤ gittata nemico).
- **Posizionamento**: kiter (`KiteStandoff>0`) → `−WKiteViolation·(standoff−dist)` se sotto standoff; mischia
  (`0`) → `−WApproach·dist` (chiudere è meglio).
- Default pesi: `WKill=10000` (domina), `WThreat=100`, `WKiteViolation=50`, `WDamage=10`, `WApproach=10`.

## 6. Determinismo (invariante #4)
Interi ovunque; nessun RNG; enumerazione candidate in ordine stabile; tie-break su coord/id → permutare l'input
non cambia la scelta.

## 7. Slicing (TDD)
- **BU.1** ✅ — `FRTBotPlan`/`FRTBotContext` + `ScorePlan` pura + **5 test** (kill, danno, minaccia, kiting,
  avvicinamento), RED→GREEN.
- **BU.2** ✅ *(C++; comportamento da confermare in PIE)* — `PlanBots` sceglie la **cella di posizionamento** via
  `ScorePlan` fra {restare, cella di tiro, cella d'avvicinamento}, pesando minaccia/kiting (può preferire di
  restare se muoversi espone). Suite **77/77** verde. Il resto della cascata (attacco/support/dash/panic) è
  invariato. Context ordine-invariante sui nemici → deterministico.
- **BU.3** ✅ *(C++; comportamento da confermare in PIE-BU3)* — `ChooseBestPlan` puro con **tie-break assoluto**
  (permutazione-invariante; **4 test**, suite **81/81**); `PlanBots` unifica {resta+attacca} e {posizionati} in
  un'unica utility deterministica; pesi esposti come `UPROPERTY` sul TurnManager (tuning in editor).
  **Vincolo di fase** (emerso in impl.): il **Blast precede il Move** ⇒ l'attacco vale solo dalla cella attuale
  (o post-dash), **non** da celle raggiunte col movimento normale. "Attacco dalla cella di destinazione via
  move-normale" è quindi impossibile per design.
- **BU.3c** ✅ *(C++; comportamento da confermare in PIE-BU3c)* — **dash+attacco**: il bot valuta anche candidate
  `{scatto → cella di tiro, attacco da lì}` (il Dash precede il Blast ⇒ colpisce dalla cella post-scatto); se
  vince l'utility, pianifica scatto+attacco. Flag puro `bViaDash` sul piano; `ChooseBestPlan` invariato.
  *Limite*: col movimento simultaneo lo scatto può essere deviato → attacco a vuoto (gestito: `NoLineOfSight`).
- **BU.3d** ✅ — **determinismo della scelta del bersaglio**: `AttackIsBetter` (tie-break assoluto sulle coord del
  bersaglio a parità di `AttackScore`; **2 test**, suite **83/83**) usato in `BestAttackFrom` → la selezione non
  dipende più dall'ordine di `GetAllActorsOfClass` (invariante #4).
- **BU.3e** ✅ — fattore **quota** in `ScorePlan`: `+ WElevation · DestCell.Layer` (premia l'alta quota; **1 test**,
  suite **84/84**), esposto come `UPROPERTY` sul TurnManager (tuning). Bilanciato da `WThreat` (una cella alta ma
  esposta resta penalizzata). **Copertura/LOS** come fattori restano aperti (§8): richiedono dati per-cella
  **impuri** (LOS bloccata per nemico) → più invasivi, rimandati.

## 8. Decisioni aperte
- Set/pesi finali dei fattori (bilanciamento in PIE). · Enumerare tutte le celle raggiungibili (costoso) **vs**
  solo le candidate delle euristiche (consigliato). · Copertura/LOS/quota come fattori aggiuntivi in BU.2.

## 9. Riferimenti
- Canone: [`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) §5 (invarianti #4 determinismo, #7 funzioni pure),
  §5.1 (tie-break assoluto). Codice: `RTBotLibrary`, `RTTurnManager::PlanBots`.
