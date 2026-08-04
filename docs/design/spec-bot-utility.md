# Spec — Bot: utility scoring multi-fattore

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
- **BU.1** ✅ *(questo)* — `FRTBotPlan`/`FRTBotContext` + `ScorePlan` pura + **5 test** (kill, danno, minaccia,
  kiting, avvicinamento), RED→GREEN. Nessun cambio a `PlanBots`.
- **BU.2** — `PlanBots` genera candidate e usa `ScorePlan` (sostituisce la cascata). Test bot verdi/aggiornati + PIE.
- **BU.3** — tuning pesi + eventuale riga di debug nel combat log.

## 8. Decisioni aperte
- Set/pesi finali dei fattori (bilanciamento in PIE). · Enumerare tutte le celle raggiungibili (costoso) **vs**
  solo le candidate delle euristiche (consigliato). · Copertura/LOS/quota come fattori aggiuntivi in BU.2.

## 9. Riferimenti
- Canone: [`piano-canonico-mvp.md`](piano-canonico-mvp.md) §5 (invarianti #4 determinismo, #7 funzioni pure),
  §5.1 (tie-break assoluto). Codice: `RTBotLibrary`, `RTTurnManager::PlanBots`.
