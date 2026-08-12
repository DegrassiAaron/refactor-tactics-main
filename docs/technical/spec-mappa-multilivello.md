# Spec — La mappa come grafo tattico esagonale multilivello

> `CURRENT` · **Stato**: as-built, allineata al codice il **2026-08-08** · **Owner**: questo file
> **Autorità**: subordinata a [`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) e a
> [ADR-0002](../decisions/adr-0002-griglia-esagonale.md).
>
> Si legge **senza conoscere la migrazione**. Il corpo originale — ponte sopraelevato su griglia quadrata
> 10×10, `FRTGridCoord`, due ISM del vecchio sistema — è conservato in
> [`spec-mappa-multilivello-quadrato.md`](../archive/technical/spec-mappa-multilivello-quadrato.md): resta la
> fonte del *perché* il multilivello ha questa forma, non del *come* è fatto oggi.

---

## 1. Il principio

La mappa **è un sistema di gioco**, non uno sfondo: decide dove si passa, cosa si vede e cosa si può
distruggere. Due conseguenze che vincolano tutto il resto:

1. **Nessun Actor per cella.** Le celle sono dati compatti in un `URTHexMapAsset`; il rendering è un solo
   `ARTHexMapActor` con ISM/HISM. Diecimila celle non sono diecimila oggetti da tickare.
2. **Il dato autorevole è separato dal rendering.** La posizione vera è `FRTCellId`; il `FVector` esiste solo
   per disegnare.

---

## 2. Coordinate

`FRTCellId{ X = q, Y = r, Layer }` — assiale, con cubica derivata (`z = −q − r`).

| Proprietà | Valore |
|---|---|
| Vicini sullo stesso layer | **6**, direzioni `E, NE, NW, W, SW, SE` |
| Distanza | cubica, intera |
| Layer | intero: piani sovrapposti dello stesso spazio |
| Conversione ↔ mondo | assiale ↔ world con arrotondamento cubico; la dimensione della cella viene **dall'asset**, non da una costante |

**Celle su layer diversi non sono adiacenti.** Mai, nemmeno se si trovano una sopra l'altra. Si collegano
**solo** con una transizione esplicita: è la regola che rende il multilivello un grafo invece di un solido.

---

## 3. Il dato di cella

`FRTHexCellData`:

| Campo | Tipo | Ruolo |
|---|---|---|
| `Height` | `int32` | quota; vale per **geometria** — LOS, occlusione, accessibilità. **Nessun bonus a danno o vista** ([D-018](../decisions/RT_PDR_00_Decision_Log.md) · [D-024](../decisions/RT_PDR_00_Decision_Log.md)) |
| `Surface` | `ERTHexSurface` | pavimento, acqua, fuoco, ghiaccio… (E8) |
| `MoveCost` | `int32` | costo **intero** per entrare nella cella |
| `bBlocksMovement` | `bool` | cella impenetrabile |
| `bBlocksLineOfSight` | `bool` | cella opaca |
| `Covers` | `TArray<FRTHexCover>` | coperture **sui bordi**, non sulla cella — vedi §4 |

## 4. Le coperture stanno sui bordi

`FRTHexCover{ Edge: ERTHexDirection, Type: ERTHexCoverType, Integrity: int32 }`, con `CoverOn(Edge)` per
interrogarle.

Una copertura appartiene a **uno dei sei lati**, non alla cella: un muretto ti protegge da nord e non da sud, e
modellarlo come proprietà della cella renderebbe la direzionalità impossibile da esprimere.

> ⚠️ **Questa riga parla del dato, non del mondo — e la distinzione va tenuta.** Due triage indipendenti
> ([2026-08-09](../roadmap/plans/map-editor-brief-spec-panel-2026-08-09.md) §4,
> [2026-08-10](../roadmap/plans/handoff-geometry-reazioni-conflict-report-2026-08-10.md) §4) hanno letto
> «copertura sui bordi» come se dicesse *«i muri devono seguire i lati dell'esagono»*. **Non lo dice, e quella
> regola non esiste in nessun documento canonico.** Un muro può attraversare una cella a 90°, tagliarla a
> metà, non essere allineato a niente: la griglia si sovrappone alla geometria, non viceversa.
>
> Ciò che sta sul bordo è l'**effetto tattico cotto** — perché la direzionalità dev'essere esprimibile con
> interi, dentro un hash che tiene fermo il KPI *replay divergence = 0*. Il muro visivo è authoring; il bordo
> è il dato autorevole; la cottura sta in mezzo, ed è **E23**.
>
> Il caso che questo modello **non** copre ancora è il muro che blocca il passaggio *senza essere una
> copertura* — nessuna riduzione, nessuna integrità. Oggi si modella male, e la separazione fra *validità
> della cella* e *validità della transizione* che serve a coprirlo è in E23.

- **Bassa** — non chiude il bordo: riduce il danno di chi la usa (CP 9.1). La riduzione **decade** se il colpo
  arriva fuori dall'arco frontale ([ADR-0005](../decisions/adr-0005-orientamento.md) §4a).
- **Alta** — **chiude il bordo**: `URTHexCoverLibrary::BlocksTraversal` è consultato **sia** da
  `URTHexPathLibrary::GraphNeighbors` **sia** da `URTHexVisionLibrary`. Movimento e vista non possono
  divergere, perché leggono lo stesso predicato.
- `Integrity` (default **30**) rende la copertura **distruttibile**: il TurnLog registra `CoverDamaged` con
  l'integrità residua e `CoverDestroyed` quando il bordo si apre (CP 9.2).

---

## 5. Le transizioni fra layer

`FRTHexEdge{ From, To, Cost, Kind }`, gestite con `AddTransition(From, To, Cost, Kind, bBidirectional)` e
`RemoveTransition(From, To, bBothDirections)`.

Sono **direzionali**: il bidirezionale è due archi. Una scala percorribile in un solo senso non è un caso
speciale da programmare, è semplicemente un arco solo.

`ERTHexTransitionKind` = `Stair · Ramp · Bridge · Tunnel · Elevator · Jump`.
`Jump` è predisposizione; il teletrasporto resta fuori.

> ⚠️ **Un `Portal` sarebbe un valore di questo enum, non un trasferimento personale**
> ([D-118](../decisions/RT_PDR_00_Decision_Log.md)). Il confine è netto e vale la pena scriverlo qui: un
> `Transfer` sposta **un'unità** e non tocca la mappa; un portale cambia la **raggiungibilità** e quindi
> appartiene al grafo — `Transitions`, `Revision`, invalidazione della cache di path. Chi lo implementasse
> come un'abilità di teletrasporto costruirebbe una scorciatoia che il pathfinding non vede. È **CP 39.10**
> di [E39](../roadmap/roadmap-post-v0.1.md#e39--spatial-transfer--teleport-blink-e-movimento-istantaneo--p3),
> e la sua prima riga è un audit della serializzazione di questo enum.

> **Gli archi non portano trigger.** Una trap o un tripwire possiede la propria coppia `(From → To)` e la
> confronta col micro-step del movimento; `FRTHexEdge` resta riservato ai soli salti di layer
> ([D-013](../decisions/RT_PDR_00_Decision_Log.md)). Il motivo è nella §2 di
> [`spec-pathfinding-pf3-pf4.md`](spec-pathfinding-pf3-pf4.md): gli adiacenti orizzontali sono **calcolati**,
> quindi non esiste un arco su cui appendere il trigger.

---

## 6. Asset, formato e identità

| Elemento | Valore |
|---|---|
| Tipo | `URTHexMapAsset : UPrimaryDataAsset` |
| Versione di formato | `CurrentFormatVersion = 3`, con `MigrateToCurrentFormat()` in `PostLoad()` |
| Ordine | `SortCells()` — ordine stabile, indipendente dall'inserimento |
| Identità | `ComputeHash()` — hash deterministico del contenuto |
| Revisione | `Revision`, incrementata a ogni modifica strutturale |

`Revision` e `ComputeHash()` servono insieme allo **snapshot** di simulazione: `FRTHexSimSnapshot` cattura
entrambi e si dichiara obsoleto se uno dei due cambia. Sono il meccanismo che impedisce a un turno di essere
risolto contro una mappa diversa da quella su cui era stato pianificato.

L'editing (`BeginStroke` / `PaintCellInStroke` / `EraseCellInStroke` / `EndStroke`, `ApplyBrush`) è **solo
editor**: il modulo runtime non dipende da UnrealEd.

---

## 7. Che cosa questo documento **non** possiede

| Tema | Owner |
|---|---|
| A\*, archi percorribili, costi del cammino | [`spec-pathfinding-pf3-pf4.md`](spec-pathfinding-pf3-pf4.md) |
| Regole di copertura e distruzione | [`../gameplay/spec-copertura-cp91.md`](../gameplay/spec-copertura-cp91.md) · [`../gameplay/spec-copertura-alta-cp92.md`](../gameplay/spec-copertura-alta-cp92.md) |
| Superfici, stati, propagazione | [`../gameplay/spec-terreni-e8.md`](../gameplay/spec-terreni-e8.md) |
| Chi può interagire con che cosa, e perché no | [`../gameplay/spec-interazioni-mappa-cp101.md`](../gameplay/spec-interazioni-mappa-cp101.md) |
| LOS e forme di targeting | [`h6-4-hex-vision-spec.md`](h6-4-hex-vision-spec.md) (`AS-BUILT`, emendata da E9/E13) |
| Percorsi e naming degli asset in `Content/` | [`convenzioni-contenuti-ue.md`](convenzioni-contenuti-ue.md) |
| Stato di avanzamento | [`../roadmap/roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md) |
