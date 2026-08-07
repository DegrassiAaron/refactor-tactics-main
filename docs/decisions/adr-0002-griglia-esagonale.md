# ADR-0002 — Pivot da griglia quadrata a griglia esagonale (+ editor mappa)

> **Stato**: Accettato — in implementazione · **Data**: 2026-08-03 · **Decisore**: utente (dev singolo)
> **Contesto sorgente**: `/sc:spec-panel` su `docs/src/Editor - Implementazione Griglia Esagonale ed Editor Mappa.docx`

## Contesto

Il canone (`piano-canonico-mvp.md §3`) fissa una griglia **quadrata 10×10 @ 200 cm** (`FRTGridCoord{X,Y,Layer}`,
distanza **Manhattan**), su cui poggia l'MVP M0–M5 (quasi completo): pathfinding a grafo multilivello
(`ReachableCellsByGraph`/`FindPathByGraph` + `FRTTraversalEdge`), combat/LOS/forme, bot, terreno, knockback, TurnLog.

L'utente (2026-08-03) decide di adottare una griglia **esagonale** (assiale/cubica) + **editor mappa** data-driven,
da un documento di specifica dettagliato. Lo **spec-panel** ha evidenziato che è un **rifacimento** che invalida
M1–M5 (~71 test) e ha proposto tre vie (ADR+analisi · prototipo isolato · pivot completo); l'utente, **informato dei
costi**, ha scelto il **pivot completo**.

Vincoli mantenuti: invarianti #1 (regole in C++), #4 (determinismo, no float), no Actor-per-cella, dati autorevoli
separati da rendering/NavMesh; split C++/Blueprint.

## Decisione

Sostituire la griglia quadrata con una **esagonale**, **per milestone** (H0–H9, [`hex-map-roadmap.md`](../roadmap/hex-map-roadmap.md)),
piccole e **compilabili**:

- **Coordinate**: `FRTCellId` assiale (X=Q, Y=R) + cubica derivata (Z = −X − Y), `Layer` per i piani; 6 vicini;
  distanza cubica; conversioni axial↔world con arrotondamento cubico.
- **Dati cella**: `FRTHexCellData` (superficie, costo intero, blocco movimento/LOS, cover direzionale a 6 lati, hazard, tag, transizioni).
- **Asset**: `URTHexMapAsset` (`UPrimaryDataAsset` serializzato, ordine stabile, hash deterministico).
- **Rendering**: `ARTHexMapActor` (ISM/HISM, **no Actor per cella**).
- **Grafo tattico + A\*** esagonale deterministico (costi interi, tie-break su ID).
- **Modulo Editor** mappa (selezione/painting/undo/salvataggio), **non richiesto a runtime**.

Il canone §3 (griglia quadrata, `FRTGridCoord`) è **superato** da questa decisione.

## Alternative considerate

| Alternativa | Esito |
|---|---|
| Mantenere la griglia quadrata | Scartata (decisione utente di procedere) |
| Prototipo hex **isolato/parallelo** | Scartata (utente vuole pivot completo) |
| ADR + analisi poi decidere | Assorbito in questo ADR (decisione già presa) |
| **Pivot completo** ✅ | **Scelto** dall'utente, informato dei costi |

## Conseguenze

**Positive**: tattica esagonale (6 direzioni, distanze uniformi) più adatta a un tattico; mappa data-driven + editor
(contenuti senza codice); architettura pulita (grafo tattico autorevole).

**Negative / costi**: rifà **M1–M5** (griglia, movimento, combat/LOS, forme, bot, MP.1–MP.4, ~71 test); MVP quadrato
(quasi chiuso) destabilizzato durante il pivot; nuovo modulo Editor (UnrealEd/Slate) = superficie ampia; lavoro lungo.

**Mitigazione**: milestone piccole e compilabili; **H0 crea i tipi hex ISOLATI** (non rompe il quadrato) — la
sostituzione dei sistemi avviene da H1+. Il ramo **`feat/hex-grid`** isola il pivot; il quadrato resta su
`feat/skeletal-units`/`main` come base di rollback finché il pivot non è pronto.

**Invarianti**: #1 e #4 preservati.

## Verifica (per milestone)

Criteri Done H0–H9 in [`hex-map-roadmap.md`](../roadmap/hex-map-roadmap.md). **H0**: compila; test coordinate (axial↔world,
6 vicini, distanza cubica, ordinamento) verdi; il modulo runtime **non** dipende da moduli Editor.

## Revisione

Se il rifacimento risulta troppo costoso/destabilizzante: (a) tenere l'hex come sottosistema isolato e rimandare la
sostituzione; (b) rollback a quadrata (`feat/skeletal-units` resta la base). **Rivedere dopo H2** (editor MVP) con i
costi reali misurati.

### Aggiornamento 2026-08-04 — merge su `main`

L'utente (dev singolo, decisore) ha deciso di **mergiare `feat/hex-grid` in `main`** dopo la prima fetta di H5c
(tool paint-a-click), **prima** che l'hex sostituisca funzionalmente l'MVP quadrato (restano H5c.2+, H6–H9). Questo
**supera** la guida originaria di questo ADR («niente merge finché l'hex non sostituisce l'MVP»). Conseguenze
accettate: `main` contiene ora il pivot hex ancora in corso; la base di rollback resta **`feat/skeletal-units`** (non
toccata). Decisione presa consapevolmente su richiesta esplicita; lo sviluppo hex prosegue su `feat/hex-grid`.
