# RefactorTactics — Roadmap Griglia Esagonale ed Editor Mappa (H0–H9)

> Pivot deciso in [`adr-0002-griglia-esagonale.md`](adr-0002-griglia-esagonale.md), da
> `docs/guides/Implementazione Griglia Esagonale ed Editor Mappa.docx`. Milestone **piccole e compilabili**;
> si compila e si testa dopo ogni milestone. Runtime hex **indipendente** dai moduli Editor.
> Regola: una milestone è ✅ solo quando i suoi criteri **Done** sono verificati.

## Invarianti (dal documento §23, coerenti col canone)
Dati autorevoli indipendenti da Actor/mesh/animazioni/frame rate/ordine collection/NavMesh · determinismo · costi
interi · no float in coord/hash · no Actor-per-cella (ISM/HISM) · A* sul grafo tattico (non NavMesh).

## Milestone

| ID | Contenuto | Done quando | Stato |
|----|-----------|-------------|-------|
| **H0** Fondazioni | Modulo/cartelle `Map/Pathfinding/Debug`; `FRTCellId` (assiale+cubica, ==/!=, hash, ordinamento, ToString, 6 vicini, distanza, axial↔world con rounding cubico); test coordinate | Compila; test coordinate verdi; runtime senza dipendenze Editor | ✅ (77/77 test, 4f822a9) |
| **H1** Asset + rendering graybox | `FRTHexCellData`; `URTHexMapAsset` (storage stabile, lookup, ordinamento, hash); `ARTHexMapActor` (ISM); generatore base | Si crea/salva una griglia; reload Editor mantiene i dati; nessun Actor per cella | 🟡 logica 81/81 (8ac987a); ARTHexMapActor graybox (f2ee6b0) — PIE/save-reload da verificare |
| **H2** Editor MVP | Selezione (raycast→axial), multi-selezione, paint superficie/costo/blocco/altezza/layer, Undo/Redo, validazione | Mappa graybox creabile senza codice; operazioni annullabili; package dirty corretto | ⏳ |
| **H3** Grafo + A\* | Vicini/archi/profilo unità; A* deterministico; debug path; Automation Test | Path deterministico; ostacoli/costi ok; layer diversi richiedono transizione esplicita | ⏳ |
| **H4** Multilivello | Filtri layer; bridge/tunnel/scale/ascensori; celle sovrapposte; selezione multilivello | (X,Y) uguali con Layer diversi gestiti; viz/path non confondono i livelli | ⏳ |
| **H5** Editor Mode dedicato | Toolbar, brush, palette, overlay, pannello validator, strumenti archi, copia/incolla, shape tools | Workflow non dipende più dal solo pannello Details | ⏳ |
| **H6** Integrazione simulatore | Snapshot mappa; occupazione; planning; movement budget; collisioni simultanee; TurnLog; replay | Stesso asset+snapshot+seed+intenti → stesso risultato; animazioni non influenzano il movimento | ⏳ |
| **H7** Networking/privacy | Asset hash; validazione server; path client proposto / server validato; planning team-only | Server valida ogni percorso; avversario non riceve celle/path del team nemico | ⏳ |
| **H8** Ambienti tattici | Acqua/fuoco/elettricità; cover dinamica; porte/ponti; hazard; revisioni chunk | Modifiche ambientali invalidano cache/path; TurnLog registra ogni modifica | ⏳ |
| **H9** Production readiness | Chunk; performance; mappe grandi; commandlet validator; cook; packaged; replay; profiling | Path mediana < 2 ms; preview < 50 ms; zero divergenze replay; validator in CI; packaged ok | ⏳ |

## Criteri di accettazione della prima consegna (documento §20)
Compila · `FRTCellId` assiale con 6 vicini · conversione world/axial corretta · asset mappa persistente ·
`ARTHexMapActor` instanced · generazione graybox · selezione/aggiunta/rimozione cella · paint superficie/costo/blocco ·
Undo/Redo · asset valido dopo reload · validator trova errori reali · ≥1 Automation Test · `L_DevSandbox` mostra il
risultato · runtime senza dipendenze Editor.

## Rapporto con l'MVP quadrato
Il sistema quadrato (M1–M5) resta su `feat/skeletal-units`/`main` come **base di rollback** (ADR-0002 §Revisione)
finché l'hex non sostituisce funzionalmente l'MVP. Il pivot vive su **`feat/hex-grid`**.
