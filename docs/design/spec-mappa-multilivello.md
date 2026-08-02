# Design — Mappa multilivello: ponte sopraelevato (vertical slice PF.4)

> `/sc:design` del **2026-08-02**. Decisioni prese: **layout = ponte sopraelevato · LOS = regole di
> elevazione · scope = wiring minimo prima**. Ancorato al motore a grafo **PF.4.2**
> ([`spec-pathfinding-pf3-pf4.md`](spec-pathfinding-pf3-pf4.md)), al terreno ([`spec-terreni.md`](spec-terreni.md)),
> al canone ([`piano-canonico-mvp.md`](piano-canonico-mvp.md)). Documentale: nessuna modifica al codice.

## 1. Obiettivo

Vertical slice a **2 layer** con un **ponte sopraelevato** che attraversa la mappa: verticalità tattica con
**vantaggio di elevazione** reale sulla linea di tiro, riusando il motore a grafo e i sistemi esistenti.

## 2. Concept — il ponte

- **Layer 0** (terra): la 10×10 attuale, incluse le 4 coperture centrali (restano ostacoli a terra).
- **Layer 1** (ponte): una **passerella larga 1 cella** lungo la riga `y=4`, dai bordi al centro, che passa
  **sopra** le coperture centrali. Celle-ponte: `(2,4,1) … (7,4,1)` (6 celle).
- **Rampe** (archi bidirezionali, costo 2) alle due estremità: `(1,4,0) ↔ (2,4,1)` e `(8,4,0) ↔ (7,4,1)`.
- Il ponte è **alta quota**: chi ci sta vede e spara **oltre** le coperture basse (elevazione, §5), ma è
  **esposto** su una passerella stretta. Rischio/ricompensa; contendibile da entrambe le squadre (2 rampe).
- Buff opzionale: le celle-ponte possono portare **Altura** (`OccupantDamageBonus`) per rimarcare l'alta quota.

## 3. Modello dati

Riusa i tipi PF.4 (`FRTGridCoord{X,Y,Layer}`, `FRTTraversalEdge`). L'`ARTGridActor` è la fonte del grafo:
- **Celle valide per layer**: layer 0 = intera 10×10; layer 1 = **insieme esplicito** delle celle-ponte. Le
  celle non presenti su un layer sono impassabili (`RT_BLOCKED_COST` nella cost map di quel layer) — un solo
  meccanismo, nessun concetto nuovo di "cella inesistente".
- **Archi**: `TArray<FRTTraversalEdge> Edges` sull'`ARTGridActor` (le rampe), creati a runtime in C++
  (come terreno/abilità, nessun `.uasset`). `GetEdges()` per i chiamanti.
- **`BuildCostMap`/`BuildBotCostMap`** estesi ai layer (costo terreno per cella valida; bloccate le non-valide).

## 4. Rendering

- **Due ISM**: `Cells0` (esistente, quota base) e `Cells1` (nuovo, celle-ponte a quota `Z + LayerHeight`,
  ≈ 250 cm). Le coperture centrali (cubi) restano a terra, il ponte passa sopra.
- **Rampe**: una mesh inclinata (o marker) tra le celle collegate dagli archi.
- **Camera**: la top-down attuale gestisce le altezze; nessun cambio nell'MVP (eventuale pitch da tarare).
- `CellToWorld` estesa con la quota del layer; `RefreshTerrainVisuals` per-layer.

## 5. LOS con regole di elevazione (DECISO)

**Regola**: lungo la linea 2D tiratore→bersaglio, una cella blocca la LOS **solo se il suo blocker sta a un
layer ≥ quello del tiratore**. Modello: ogni blocker occupa l'altezza `[layer, layer+1)`; l'occhio del tiratore
è al suo layer.
- **Tiratore a terra (layer 0)**: bloccato dalle coperture di layer 0 (come oggi); il **ponte (layer 1) è sopra
  l'occhio → non blocca** (si spara *sotto* il ponte). ✓
- **Tiratore sul ponte (layer 1)**: le coperture di layer 0 sono **sotto l'occhio → non bloccano** (si spara
  *oltre* le coperture basse = **vantaggio di elevazione**). ✓
- Deterministica, riusa il campionamento 2D di `HasLineOfSight`; generalizzazione: la funzione prende il
  **layer del tiratore** e considera solo i vision-blocker con `layer ≥ tiratore`. *TDD-abile.*

## 6. Regole di gioco (MVP)

- **Movimento cross-layer** via rampe (archi, costo 2); budget = budget di costo; `FindPathByGraph`.
- **Occupazione** 1 unità per cella *per layer* (`(x,y,0)` e `(x,y,1)` distinte).
- **Alta quota**: elevazione (§5) + eventuale buff Altura sulle celle-ponte.
- **Caduta/spinta**: nessun push nell'MVP → nessuna caduta. Rimandata.

## 7. Interazione (il pezzo nuovo più delicato)

Un click deve risolvere `(X, Y, Layer)`. **Soluzione**: ogni layer ha il suo ISM con collisione;
`GetHitResultUnderCursor` ritorna il **componente colpito** + indice-istanza → si deriva il Layer
(`Cells0`→0, `Cells1`→1) e la cella. Il ponte (più in alto) "copre" le celle sotto: cliccando la passerella
si mira al ponte, cliccando il pavimento si mira a terra. Deterministico. `PlaceOnCell` posiziona l'unità alla
quota del layer. **Rischio**: la priorità di collisione tra `Cells0`/`Cells1` va provata in PIE presto.

## 8. Determinismo & invarianti

- Pathfinding a grafo **deterministico** (tie-break `X,Y,Layer`); costo intero (R3) → hash stabile.
- Autorità server: reachability/path validati col grafo.
- **`GraphRevision`/`SchemaVersion`**: solo con archi **dinamici** (crolli/portali da skill) → north-star. Nel
  ponte l'MVP è **statico** nello snapshot → non serve.

## 9. Piano d'implementazione (stato 2026-08-02)

- **MP.1 — Ponte + movimento cross-layer** ✅ *(codice completo, 53 test; salita interattiva non ancora
  PIE-verificata dall'utente)*: `SpawnBridge` (celle layer 1 (3,4,1)..(7,4,1) + 2 rampe) · `BuildCostMap`
  multilivello (celle-ponte calpestabili, resto di layer 1 impassabile) · `BridgeCells` ISM a quota ·
  `LayerFromHitComponent` (click→layer) · pipeline movimento graph-aware (`FindPathByGraph`, `PathCost`/
  `BuildCompositePath` con archi) · `PlaceOnCell(..., LayerHeight)`. Fix: ponte non sopra lo spawn del Guardian.
- **MP.2 — LOS di elevazione** ✅ TDD: un blocker blocca solo se allo **stesso layer** del tiratore (dal ponte
  si spara sopra le coperture basse; a terra si spara sotto il ponte). Retro-compatibile col 2D.
- **MP.3 — HUD path elevato** ✅: linea, pallini, destinazione e traccia post-lock seguono la quota del layer.
- **MP.4 — verifica PIE end-to-end + tuning** ⏳; **bot-sul-ponte** ⏳ (il bot resta a terra: enhancement).

> **Nota di verifica**: la logica cross-layer è coperta da test (`FindPathByGraph`, `PathCost` via arco,
> LOS di elevazione). Il solo pezzo non verificabile senza gioco manuale è il **click→layer** a schermo.

## 10. Rischi

- **Click→layer** (ISM-hit priority): il più nuovo → PIE presto (MP.1).
- **Leggibilità** due layer in top-down → colori/quota da tarare.
- **Bot** su reachability a grafo: scoring da estendere (salire/evitare esposizione).
- **Epica**: affettata (MP.1 minimo prima).

## 11. Conflitti segnalati (regola CLAUDE.md)

- Il ponte **non** cambia la semantica delle coperture centrali (restano ostacoli a terra); aggiunge un layer
  sopra. Nessun conflitto col canone attuale, salvo aggiornare roadmap/mappa quando il ponte sarà a schermo.
- Coerente con R1/R2/R3 (§3.1 canone) e col motore PF.4 già consegnato.

## 12. Prossimo passo

`/sc:design` è documentale: qui ci si ferma. L'implementazione parte da **MP.1** con `/sc:implement` (o via
richiesta esplicita) — TDD dove la logica è pura (LOS di elevazione), wiring + PIE per rendering/interazione.
