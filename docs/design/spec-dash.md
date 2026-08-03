# Spec — Dash come fase attiva (riposizionamento rapido)

> `/sc:design` + implementazione del **2026-08-03**. Obiettivo: attivare la fase **Dash** (finora pass-through)
> con un riposizionamento rapido che si risolve **prima del Blast**. Documentale + implementato in TDD dove la
> logica è pura, wiring + PIE per Actor/UI. Ancorata al canone ([`piano-canonico-mvp.md`](piano-canonico-mvp.md)),
> al pathfinding a grafo ([`spec-pathfinding-pf3-pf4.md`](spec-pathfinding-pf3-pf4.md)) e all'animazione della
> risoluzione ([`spec-anima-risoluzione.md`](spec-anima-risoluzione.md)).
>
> **Decisioni prese**: movimento del dash = **pathfinding** (aggira ostacoli) · **dash + move** (movimento doppio
> consentito) · scope = **bot + player**.

## 1. Obiettivo e valore tattico

La fase **Dash** era nel ciclo (`Planning → Prep → Dash → Blast → Move → Cleanup`) ma **senza handler**
(pass-through). Ora un'abilità di **scatto** ci sposta **prima che gli attacchi (Blast) colpiscano**: si schiva
un tiro previsto, si chiude la distanza per il corpo a corpo, o si esce dalla copertura. È il cuore di Atlas
Reactor e dà senso alla fase + profondità alla pianificazione (il Blast usa le posizioni **post-scatto**).

## 2. Modello dati

- `URTAbilityData::bDash` — l'abilità è uno scatto: non attacca, sposta l'unità fino a `RangeCells` celle
  (budget di **costo**), gated da `CooldownTurns`. `Power` = 0.
- `ARTUnit::PlannedDashAbility` (`INDEX_NONE` = nessuno) + `ARTUnit::PlannedDashCell` — pianificazione dello
  scatto per il turno. `ARTUnit::FindDashAbilityIndex()` trova l'abilità di scatto dell'unità.
- Abilità di scatto di default: **Ranger → "Scatto"** (5 celle, ricarica 2); **Guardian → "Carica"** (4 celle,
  ricarica 3); fallback generico "Scatto" (`MoveRange+2`, ricarica 2). Sono la **4ª abilità** (indice 3).

## 3. Risoluzione (`ARTTurnManager::ResolveDash`, fase Dash)

1. Raccoglie le unità vive con uno scatto pianificato **valido**: abilità `bDash` utilizzabile e
   `PlannedDashCell` raggiungibile via `FindPathByGraph` entro il **costo** `RangeCells`. Lo scatto è
   **consumato per il turno** in ogni caso (valido o no).
2. Risolve gli scatti **simultanei, ordine-indipendenti** (stesso `URTMovementResolver::ResolvePaths` del
   movimento).
3. Applica le posizioni **senza cancellare il move normale** (dash + move): aggiorna `GridCell` + visuale;
   la path composita (che partiva dalla cella pre-scatto) è invalidata; se **non** c'era un move pianificato
   (`PlannedCell == pre-scatto`) imposta `PlannedCell = destinazione` (niente ritorno indietro), altrimenti il
   `PlannedCell` resta e il Move normale instrada dalla nuova posizione.
4. Consuma l'abilità (cooldown). Applica il **cross-damage** del terreno attraversato (ord.-indip.); una morte
   durante lo scatto genera un evento `Defeated` (fase Dash).
5. Emette eventi `Move` con **fase Dash** per il playback.

**Invarianti**: determinismo (resolver ord.-indip., budget interi); il Blast usa `GridCell` post-scatto; i morti
sono esclusi dal Move (come per il combattimento).

## 4. Bot (`PlanBots`)

- **Scatto offensivo** (nessun tiro dalla cella attuale, non in fuga): se lo scatto è **pronto**, il bot lo usa
  per riposizionarsi/avvicinarsi **in fretta** (più portata del movimento) — prima cerca una **posizione di
  tiro** raggiungibile con lo scatto (`BestFiringCell`), altrimenti chiude la distanza (`BestApproachCell`).
- **Scatto difensivo (schiva)**: quando è minacciato (panic o kiter con nemico entro lo standoff) e lo scatto è
  pronto, il bot **fugge con lo scatto** (`BestKiteCell` col range dello scatto). Poiché il dash si risolve
  **prima del Blast**, la posizione post-scatto **ri-valida gittata/LOS** dell'attaccante: uscendo dal tiro,
  l'attacco previsto **manca**. Fallback al kite normale (movimento) se lo scatto non è pronto.

Il bot è **dash-only** (non pianifica anche il move quando scatta). Le abilità `bDash` sono escluse dalla
selezione d'attacco (non sono attacchi). *(Verifica PIE: osservati sia `scatto -> …` offensivi sia
`scatto difensivo (schiva) -> …`.)*

## 5. Giocatore (`RTPlayerController`, HUD)

- **Tasto `4`** seleziona lo scatto (4ª abilità). Con lo scatto selezionato, il **click su una cella** pianifica
  il dash verso quella cella (validato entro la portata dello scatto; supporta il click→layer del ponte).
  Cliccare un nemico con lo scatto selezionato non fa nulla (lo scatto è su cella).
- **HUD**: la preview dello scatto è disegnata in **magenta** (percorso + destinazione), distinta dal movimento
  normale (ciano). Soggetta alla privacy dell'intento (invariante #6, come gli altri piani).

## 6. Playback (animazione)

La fase Dash è **animata prima del Blast** (riusa il sistema di [`spec-anima-risoluzione.md`](spec-anima-risoluzione.md)):
gli eventi di scatto sono `Move`-type con fase Dash; le animazioni sono taggate per fase; un'unità che fa
scatto **e** move ha due animazioni consecutive e continue (scatto → posizione intermedia → move). Ordine di
playback: `Prep → Dash → Blast → Move`.

## 7. Verifica

- **TDD (logica pura riusata)**: il dash riusa `FindPathByGraph`/`PathCost`/`ResolvePaths` (già testati,
  ordine-indipendenza). Nessuna nuova matematica: l'orchestrazione è verificata in PIE.
- **Regressione**: 62/62 automation test verdi (invariati) → il dash non altera gli esiti.
- **PIE (log)**: il bot pianifica ed esegue scatti (`scatto -> (x,y,L)` / `Scatto: … -> (x,y,L)`); la fase Dash
  è animata **prima** del Blast (`Fase Dash → Playback fase: Dash → Blast → Playback fase: Blast → Colpo`);
  nessun crash. Osservati scatti a `layer 0` (terra); lo scatto sul ponte (`layer 1`) emerge quando il bot è
  vicino a una rampa (situazionale).

## 8. Limiti aperti

- **Dash del giocatore**: cablato (tasto `4` + click + preview), ma la conferma **interattiva** richiede il
  mouse dell'utente (come il click→layer di [`spec-mappa-multilivello.md`](spec-mappa-multilivello.md)).
- **Dash "leggero"** (salto che scavalca le coperture in linea retta) **non** implementato: scelto il
  pathfinding (aggira gli ostacoli). Possibile enhancement futuro.
- Tuning dei valori (portata/ricarica degli scatti) da tarare in gioco.

## 9. Interazione con gli status (2026-08-03)

Lo scatto rispetta gli status come il movimento normale: **Root azzera** la portata dello scatto (unità
radicata → niente scatto), **Slow la dimezza**. Implementato con `ARTUnit::GetEffectiveDashRange(BaseRange)`
che riusa `URTCombatLibrary::EffectiveMoveRange(...)` (già testato: Root→0, Slow→metà), applicato in
`ResolveDash` (autoritativo) e nella pianificazione di bot e giocatore. Regressione 62/62 invariata.
