# Spec H6.5 — Bot su griglia esagonale (utility scoring)

> ## 🧱 `AS-BUILT 2026-08-05` — spec di ciò che fu consegnato a H6.5
>
> **Non è il documento in cui si cambia il bot oggi.** Descrive l'utility scoring com'era al checkpoint H6.5.
>
> **Requisiti sopravvenuti**, che questa spec non poteva conoscere e che vincolano ogni lavoro futuro sul bot:
>
> - il bot **non può leggere stato nemico nascosto**: deve decidere sulla **Team Knowledge** della propria
>   squadra (E13), altrimenti bara e il playtest misura la cosa sbagliata;
> - deve tenere conto del **facing** e dell'arco frontale (E16, [ADR-0005](../../decisions/adr-0005-orientamento.md));
> - deve avere una **politica di reazione** esplicita per Overwatch e finestre (E14);
> - va sottoposto alla **validazione di stress** a scala maggiore (E17).
>
> Owner corrente: [`architettura-codice.md`](../architecture/architettura-codice.md) · roadmap E13/E14/E16/E17.

> **Stato**: **Implementata** · **Data**: 2026-08-05 · **Branch**: `feat/hex-bot`
> *(la suite «166/166» è il numero di allora; oggi si misura col comando in [`../../README.md`](../../README.md))*
> **Fonte**: milestone **H6** di [`hex-map-roadmap.md`](../../roadmap/hex-map-roadmap.md). Ultimo prerequisito puro prima
> dello switch di `ARTTurnManager`. Non tocca il bot quadrato ([`spec-bot-utility.md`](../../archive/gameplay/spec-bot-utility.md)).

## 1. Contesto

Con H6.1–H6.4 la simulazione esagonale ha movimento (budget, collisioni), tracciamento (TurnLog/replay) e
targeting (LOS, linea, cono). Manca chi decide: `URTBotLibrary` ragiona in distanze di Manhattan, griglia
`Width × Height` e liste di `VisionBlockers` — tutte assunzioni della topologia quadrata.

**Obiettivo**: portare su hex la stessa politica del bot (BU.1–BU.3), **senza cambiarne il comportamento
concettuale**: focus-fire con bonus letale, minaccia mitigata dalla copertura, kiting o avvicinamento secondo
l'archetipo, bonus di elevazione, scelta deterministica.

## 2. Decisioni

| # | Decisione | Motivo |
|---|-----------|--------|
| D1 | Politica **identica** al quadrato (stessi pesi, stesse formule), cambiano solo distanza e LOS | Il pivot hex non è l'occasione per ribilanciare il bot: se il comportamento cambiasse insieme alla topologia non si saprebbe a cosa attribuire le differenze. I pesi restano quelli di `FRTBotContext`. |
| D2 | Distanza = `HexDistance`, LOS = `URTHexVisionLibrary::HasLineOfSight` **sull'asset** | Sull'esagono la distanza di Manhattan non ha senso; la copertura ora è un dato della mappa (`bBlocksLineOfSight`), non una lista passata dal chiamante — meno duplicazione di stato e coerenza con il resto dello strato hex. |
| D3 | Le candidate nascono da `URTHexSimLibrary::ReachableCells` | Budget di movimento, celle bloccate, unità occupanti e archi verticali sono **già** risolti lì: il bot non rifà pathfinding e non può proporre mosse illegali. Il quadrato invece ha quattro ricerche dedicate (`StepToward`, `BestApproachCell`, `BestKiteCell`, `BestFiringCell`) perché quel supporto non esisteva. |
| D4 | Una candidata per (cella raggiungibile × bersaglio colpibile) + una senza attacco per cella | Il pattern "genera candidate, valuta con utility" è quello di BU.3; con `ReachableCells` diventa esaustivo invece che euristico. A 2v2 con budget piccoli il numero di candidate resta nell'ordine delle decine. |
| D5 | Tie-break con `StableLess` (Layer, X, Y) | È l'ordinamento canonico dello strato hex. Il quadrato usa (X, Y, Layer): stessa proprietà (ordine totale → esito indipendente dall'ordine di enumerazione), convenzione locale diversa. |

## 3. API — `Bot/RTHexBotLibrary.{h,cpp}`

- `FRTHexBotPlan` — `DestCell`, `bHasAttack`, `TargetIndex` (indice in `Enemies`), `AttackDamage`, `TargetHealth`.
- `FRTHexBotContext` — `Origin`, `Enemies` + `EnemyRanges` + `EnemyHealth` (paralleli), `AttackRange`,
  `AttackDamage`, `KiteStandoff`, pesi `WKill/WDamage/WThreat/WKiteViolation/WApproach/WElevation`.
- `ScorePlan(Map, Plan, Context)` → intero. Nell'ordine: `WDamage × danno` (+`WKill` se il colpo uccide);
  −`WThreat` per **ogni nemico** che raggiunge la cella con la propria gittata **e** ha LOS su di essa;
  poi, sulla distanza dal nemico più vicino, −`WKiteViolation × (standoff − dist)` se kiter sotto la soglia
  oppure −`WApproach × (dist − portata)` se kiter oltre la propria portata (dentro la banda utile è
  indifferente), oppure −`WApproach × dist` se mischia; infine +`WElevation × Layer`, vincolato da
  `WElevation × MaxLayer < WApproach` — sopra quella soglia il bot si parcheggia in quota (#1088).
- `ChooseBestPlan(Map, Candidates, Context)` — punteggio massimo; a parità **mossa minima** da `Origin`, poi
  `StableLess` sulla cella. Nessuna candidata → resta a `Origin`.
- `BuildCandidates(Snapshot, UnitId, Context)` — per ogni cella raggiungibile: una candidata senza attacco e una
  per ciascun nemico entro `AttackRange` e con LOS da quella cella.
- `PlanUnit(Snapshot, UnitId, Context)` — `ChooseBestPlan(BuildCandidates(...))`.

## 4. Test — `Tests/RTHexBotTests.cpp`, prefisso `RefactorTactics.HexBot.*`

| Test | Comportamento |
|---|---|
| `ScoreFocusFire` | un colpo letale batte un colpo che non uccide; più danno batte meno danno |
| `ScoreThreatRespectsCover` | un nemico in gittata **con** LOS abbassa il punteggio; con un muro in mezzo no |
| `ScoreKiterVsMelee` | kiter penalizzato sotto lo standoff; mischia penalizzata dalla distanza |
| `ScoreElevationBonus` | a parità di tutto, la cella su layer più alto vince |
| `ChooseBestPlanOrderIndependent` | permutare le candidate non cambia l'esito |
| `ChooseBestPlanTieBreak` | a parità di punteggio vince la mossa minima da `Origin` (restare), poi `StableLess` |
| `ChooseBestPlanEmptyStays` | nessuna candidata → resta a `Origin` |
| `PlanUnitTakesKillingShot` | scenario completo: si sposta nella cella da cui uccide |
| `PlanUnitRespectsBudgetAndOccupancy` | non propone celle fuori budget né occupate da altre unità |
| `PlanUnitSeeksCover` | fra due celle equivalenti sceglie quella non esposta al tiro nemico |

## 5. Definition of Done (raggiunta)

☑ TDD RED→GREEN misurato (RED: 8 test falliti con gli stub; `PlanUnitRespectsBudgetAndOccupancy` e
`PlanUnitSeeksCover` passavano a vuoto — con le candidate vuote i loro cicli non asserivano nulla — e sono stati
**rinforzati prima** dell'implementazione con asserzioni sul numero di candidate e sull'esposizione dell'origine) ·
☑ build Editor `Succeeded` · ☑ suite Automation **166/166** misurata (156 + 10) · ☑ nessuna modifica a
`URTBotLibrary`, `Grid/`, `ARTTurnManager` · ☑ solo interi (invariante #4) · ☑ nessuna verifica PIE necessaria ·
☑ spec, roadmap H6 e architettura aggiornate.

## 6. Fuori scope (YAGNI)

Scelta dell'**abilità** fra più disponibili (il quadrato lo fa in `ARTTurnManager::PlanBots`, non nella libreria) ·
**dash** come mossa candidata · supporto/panic (guardie del quadrato legate agli Actor) · coordinamento di squadra ·
previsione dei piani avversari.

## 7. Cosa resta dopo

Solo il **wiring**: `ARTTurnManager`, `ARTUnit`, controller e HUD su celle esagonali. Sarà il primo slice non
headless dello switch (verifica in PIE).
