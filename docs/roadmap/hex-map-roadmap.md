# RefactorTactics — Griglia Esagonale ed Editor Mappa (H0–H6.5)

> Pivot deciso in [`adr-0002-griglia-esagonale.md`](adr-0002-griglia-esagonale.md), da
> `docs/guides/Implementazione Griglia Esagonale ed Editor Mappa.docx`. Milestone **piccole e compilabili**;
> si compila e si testa dopo ogni milestone. Runtime hex **indipendente** dai moduli Editor.
> Regola: una milestone è ✅ solo quando i suoi criteri **Done** sono verificati.
>
> ⚠️ **Ambito ridotto (2026-08-05)**: questo documento è il **registro tecnico delle fondamenta esagonali**
> (H0–H6.5, consegnate) e del residuo dell'editor mappa. Il **futuro** della linea hex — wiring della partita,
> rete, ambienti, production — è passato alla roadmap di prodotto
> ([`roadmap-checkpoint.md`](roadmap-checkpoint.md), milestone M6–M11) per evitare due tracker dello stesso
> stato. Le vecchie voci H7/H8/H9 sono rimappate in fondo.

## Invarianti (dal documento §23, coerenti col canone)
Dati autorevoli indipendenti da Actor/mesh/animazioni/frame rate/ordine collection/NavMesh · determinismo · costi
interi · no float in coord/hash · no Actor-per-cella (ISM/HISM) · A* sul grafo tattico (non NavMesh).

## Milestone

| ID | Contenuto | Done quando | Stato |
|----|-----------|-------------|-------|
| **H0** Fondazioni | Modulo/cartelle `Map/Pathfinding/Debug`; `FRTCellId` (assiale+cubica, ==/!=, hash, ordinamento, ToString, 6 vicini, distanza, axial↔world con rounding cubico); test coordinate | Compila; test coordinate verdi; runtime senza dipendenze Editor | ✅ (77/77 test, 4f822a9) |
| **H1** Asset + rendering graybox | `FRTHexCellData`; `URTHexMapAsset` (storage stabile, lookup, ordinamento, hash); `ARTHexMapActor` (ISM); generatore base | Si crea/salva una griglia; reload Editor mantiene i dati; nessun Actor per cella | 🟡 logica 81/81 (8ac987a); ARTHexMapActor graybox (f2ee6b0). **Ricostruzione della vista corretta**: `RebuildInstances` era chiamato solo da `BeginPlay` e dai pulsanti CallInEditor → riaprendo il livello la griglia non veniva ridisegnata e `InstanceCells` (mappa istanza→cella per il click) restava vuoto; ora ci pensano `OnConstruction` e `PostEditChangeProperty` (test `RefactorTactics.HexMapActor.*`). Resta la verifica PIE del save/reload |
| **H2** Editor MVP | Selezione (raycast→axial), multi-selezione, paint superficie/costo/blocco/altezza/layer, Undo/Redo, validazione | Mappa graybox creabile senza codice; operazioni annullabili; package dirty corretto | ✅ criteri Done: H2a generatore (af373b3) + H2b Undo/Redo+painting (507d289). Selezione raycast nel VIEWPORT (click) → H5 |
| **H3** Grafo + A\* | Vicini/archi/profilo unità; A* deterministico; debug path; Automation Test | Path deterministico; ostacoli/costi ok; layer diversi richiedono transizione esplicita | ✅ 86/86 (89747a9); A* deterministico. Profilo-unità/debug-draw = estensioni |
| **H4** Multilivello | Filtri layer; bridge/tunnel/scale/ascensori; celle sovrapposte; selezione multilivello | (X,Y) uguali con Layer diversi gestiti; viz/path non confondono i livelli | ✅ H4a logica (90/90 test): GetLayers/CellsInLayer, celle sovrapposte, ERTHexTransitionKind + Add/RemoveTransition, hash include Transitions (FormatVersion=2), validator esteso. H4b rendering/editor: ActiveLayer + ERTLayerViewMode(AllLayers/ActiveOnly), Generate/Paint su layer attivo, Add/RemoveVerticalTransition CallInEditor (Undo/Redo). Verifica PIE-HEX-LAYER/TRANS aperta (editor). Selezione raycast per-layer → H5 |
| **H5** Editor Mode dedicato | Toolbar, brush, palette, overlay, pannello validator, strumenti archi, copia/incolla, shape tools | Workflow non dipende più dal solo pannello Details | 🟡 Prima consegna fatta (spec+piano in `h5-editor-mode-{spec,plan}.md`): H5a modulo editor `RefactorTacticsEditor` + `URTHexEditorMode` (UEdMode, auto-reg via CDO); H5b `URTHexSelectTool` (USingleClickTool: click→raycast→WorldToAxial sul layer attivo, highlight, readout). Build Editor verde. Verifica editor PIE-HEX-MODE-A/B aperta. H5c prima fetta: URTHexPaintTool (paint+erase a click, marker verde/rosso) via helper condivisi RTHexEditorClick + ARTHexMapActor::PaintCellData/EraseCell + URTHexMapAsset::ApplyBrush (test RefactorTactics.HexMap.ApplyBrushMerge). Verifica editor PIE-HEX-MODE-C/D aperta. Restano H5c.2+ (drag-brush, palette Slate, gizmo archi, copia/incolla, overlay). H5c.2: URTHexArchTool (transizioni con gizmo) - click From + UCombinedTransformGizmo su To (snap via WorldToLayer+WorldToAxial) + Commit -> AddTransitionData; render archi nel tool. Test RefactorTactics.Hex.WorldToLayer. Verifica editor PIE-HEX-MODE-E/F aperta. H5c.3: drag-brush - URTHexPaintTool passa a UClickDragTool (pennellata press->release, dedup, una FScopedTransaction/Undo per pennellata); primitive di stroke su URTHexMapAsset (BeginStroke/PaintCellInStroke/EraseCellInStroke/EndStroke, test RefactorTactics.HexMap.StrokeEquivalence); PaintCellData/EraseCell ri-espressi. Verifica editor PIE-HEX-MODE-I/J aperta. H5c.4: pennello a raggio N - URTHexPaintToolProperties::BrushRadius (0=1 cella; N=HexArea); ApplyOne -> ApplyBrushAt (area, dedup per-cella, RebuildInstances se cambiato). Verifica editor PIE-HEX-MODE-K aperta. H5c.5: rimozione archi via tool - ERTHexArchOp{Add,Remove}; in Remove OnClicked fa hit-test URTHexLibrary::DistanceRayToSegment (test) su tutte le transizioni e rimuove la piu' vicina entro HexSize*0.6 via ARTHexMapActor::RemoveTransitionData. Verifica editor PIE-HEX-MODE-L aperta. H5c.6: overlay debug superfici - RTHexEditor::SurfaceColor + DrawSurfaceOverlay (esagoni PDI colorati per superficie + rosso su bBlocksMovement); toggle bShowOverlay in Select e Paint. Verifica editor PIE-HEX-MODE-M aperta. H5c.7 (flood-fill/secchiello): URTHexMapAsset::FloodRegion (flood-fill same-layer/same-surface, frontiera a stack, pura, test RefactorTactics.HexMap.FloodRegion) + URTHexFillTool (USingleClickTool: click su cella esistente -> riempie l'intera regione col pennello corrente via BeginStroke/PaintCellInStroke/EndStroke, un solo Undo; click su cella vuota = no-op). Verifica editor PIE-HEX-MODE-N aperta. |
| **H6** Integrazione simulatore | Snapshot mappa; occupazione; planning; movement budget; collisioni simultanee; TurnLog; replay | Stesso asset+snapshot+seed+intenti → stesso risultato; animazioni non influenzano il movimento | 🟡 H6.1–H6.2 (spec `h6-hex-sim-spec.md`): strato **parallelo** `Turn/RTHexSim{,Library}` su `FRTCellId` — `FRTHexSnapshot` (mappa referenziata con hash/revisione + occupazione copiata, `IsSnapshotStale`/`ValidateSnapshot`), `IsCellFree`, `ReachableCells` entro `MoveBudget` (Dijkstra su costi interi, archi inclusi), `FindPathForUnit` (A* che evita le unità, via il nuovo `URTHexPathLibrary::FindPathAvoiding`), `ResolveHexPaths` (microstep simultanei, ordine-indipendenti). 14 test `RefactorTactics.HexSim.*`. **H6.3**: `ToLogCoord`/`BuildMoveLog` (voci di TurnLog dagli esiti del movimento) + **topologia nei flags** del formato serializzato (`ERTLogTopology` Square/Hex, loader fail-closed, byte del quadrato invariati) → KPI «Replay divergence = 0» verificato anche su hex (`RefactorTactics.HexSim.ReplayDivergenceZero`). **H6.4** (spec `h6-4-hex-vision-spec.md`): geometria di targeting mancante per il combat — `URTHexLibrary::HexLine` (lerp in **aritmetica intera** + arrotondamento cubico sui resti: niente float nella logica, nessuna oscillazione sui confini) e `HexCone` (ventaglio 120° = unione di due settori a 60°); `URTHexVisionLibrary::HasLineOfSight` con la **stessa regola di elevazione del quadrato** (blocca solo al layer del tiratore), estremi mai bloccanti, celle assenti non bloccanti. **H6.5** (spec `h6-5-hex-bot-spec.md`): bot esagonale — `URTHexBotLibrary` con la **stessa politica** del quadrato (focus-fire con bonus letale, minaccia mitigata dalla copertura, kiting/avvicinamento, bonus di quota, tie-break assoluto) su distanza esagonale e LOS letta dall'asset; le candidate nascono da `ReachableCells`, quindi budget/blocchi/occupanti/archi sono già rispettati e il bot non può proporre mosse illegali. Suite **169/169**. Resta **solo il wiring**: `ARTTurnManager`, `ARTUnit`, controller e HUD su celle esagonali — primo slice non headless (verifica in PIE) → tracciato come **M6 Parità hex** in [`roadmap-checkpoint.md`](roadmap-checkpoint.md) |

## Rimappatura H7–H9 → roadmap di prodotto (2026-08-05)

Le milestone che restavano da fare non vivono più qui: sono diventate milestone di prodotto con DoD misurabile
in [`roadmap-checkpoint.md`](roadmap-checkpoint.md). Nessun contenuto perso, un solo tracker.

| Vecchia voce | Dove vive ora |
|---|---|
| **H6.6** wiring del turn loop (`ARTTurnManager`, `ARTUnit`, controller, HUD su celle esagonali) | **M6 — Parità hex** (CP 6.1–6.8), con le verifiche `PIE-HEXPLAY-1..9` |
| **H5** residuo editor mappa (verifiche PIE aperte, copia/incolla, palette Slate) | **M9 — Ambienti tattici e maturità dell'editor** (CP 9.1) |
| **H7** networking/privacy | **M10 — Rete e privacy** |
| **H8** ambienti tattici | **M9 — Ambienti tattici** (CP 9.2–9.3) |
| **H9** production readiness | **M11 — Production readiness** |

## Criteri di accettazione della prima consegna (documento §20)
Compila · `FRTCellId` assiale con 6 vicini · conversione world/axial corretta · asset mappa persistente ·
`ARTHexMapActor` instanced · generazione graybox · selezione/aggiunta/rimozione cella · paint superficie/costo/blocco ·
Undo/Redo · asset valido dopo reload · validator trova errori reali · ≥1 Automation Test · `L_DevSandbox` mostra il
risultato · runtime senza dipendenze Editor.

## Rapporto con l'MVP quadrato (aggiornato 2026-08-05)
Entrambi i substrati vivono ora su **`main`** (il pivot non è più su un branch separato). Il sistema quadrato
resta la **base di rollback** (ADR-0002 §Revisione) e il comportamento di riferimento per la traduzione su hex,
fino alla **parità funzionale** (M6). La dismissione non è più a tempo indeterminato: è la milestone **M7**, con
tag git di ritorno prima della rimozione.
