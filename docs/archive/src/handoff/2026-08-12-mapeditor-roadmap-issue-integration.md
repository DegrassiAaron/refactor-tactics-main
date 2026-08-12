# RefactorTactics — Roadmap Map Editor / Geometry / Diagnostics
## Audit issue + piano di integrazione — 2026-08-12 20:10 CEST

> Scope: Map Editor v0.1, visualizzazione tattica, multilayer/reachability, workspace, geometry authoring,
> bake, Movement Probe, scenari/PIE e consolidamento delle mappe di progetto.
>
> Principio: il runtime legge solo dati canonici. L'Editor visualizza, authora, valida e cuoce.
> Nessuna mesh/geometry di authoring diventa una seconda authority del gameplay.

## 1. Stato delle issue

| Issue | Feature | Stato | Azione |
|---|---|---|---|
| #567 | Focus multilayer + click layer attivo | DONE | non riaprire |
| #551 | Surface + MoveCost relief | DONE | non riaprire |
| #588 | component-safe picking | DONE | correggere riferimenti stale |
| #552 | movement block vs LOS block | DONE | resta PIE visuale |
| #553 | cover/door edge viz + edge helpers | DONE | resta #695 + PIE |
| #554 | transitions + map reachability | OPEN | recuperare PR #694, chiusa unmerged |
| #619 | occupancy 12 settori + Constrained | DONE | base geometry series |
| #620 | quantized grammar + validator | OPEN | aggiornare body stale |
| #621 | geometry bake → canonical map data | OPEN | aggiungere rischio #687 |
| #622 | ghost workspace grid | OPEN | aggiornare body stale |
| #623 | lighting + Frame Whole Map | OPEN | chiarire seduta + small code |
| #687 | asset FormatVersion non serializzato | OPEN P1 | prima delle migrazioni trasformative |
| #695 | DoorState readability | OPEN | tenere, P2, follow-up #553 |

## 2. Roadmap

### R0 — Fondazioni e visual language — DONE

`#567 → #551 → #588 → #552 → #553`

Gate: layer leggibili, superficie/costo, picking component-safe, blocker distinti, cover e porte sul bordo.

### R1 — Safety del dato — #687 P1

`FormatVersion` non è persistito nei byte dell'asset. Prima di introdurre uno schema che richieda una
migrazione trasformativa, rendere reale il versioning e verificarlo con un test old-writer/new-reader.

### R2 — Chiudere il quarto layer visuale — #554

#554 resta OPEN. La PR #694 contiene transizioni sempre visibili, `FindUnreachableCells` condivisa con
il criterio numerico e celle isolate evidenziate, ma la PR è stata chiusa senza merge dopo lo stacking
su #688.

Azioni:
1. portare il diff proprio di #694 su `main`;
2. nuova PR oppure reopen/retarget se possibile;
3. suite + Editor build sulla base corrente;
4. PIE per transizioni/zone isolate;
5. chiudere #554 soltanto dopo merge/verifica.

### R3 — Workspace — #622 + #623

#622: ghost workspace grid visibile anche dove non esistono celle; transient, non collidibile,
distinta dalla mappa reale.

#623: due sottolavori espliciti:
- A: seduta Content Browser per l'illuminazione di `L_DevSandbox`;
- B: piccolo comando editor `Home → Frame Whole Map`;
- C: PIE + `editor-sessions.yaml`.

### R4 — Geometry model — #619 DONE → #620 → #621

`#619 DONE → #620 grammar/validator → #621 deterministic bake`

#620 deve riusare i helper già arrivati con #553: `EdgeMidpointWorld`, `EdgeRotation`,
`OppositeDirection`. Non va creato un secondo calcolo dei bordi.

#621:
- `LOW WALL → FRTHexCover{Low}`;
- `WALL → FRTHexCover{High}`;
- solid footprint → `bBlocksMovement`;
- void/cliff → `Surface::Void`;
- bake puro runtime;
- runtime non legge geometry di authoring.

Qualunque schema persistito che richieda una vera trasformazione è protetto da #687.

### R5 — NUOVA ISSUE: Geometry Authoring Tool

Titolo:
`Map Geometry Tool: disegno quantizzato, ghost/snap, Undo/Redo e Bake`

Perché manca: #620 mette ghost/snap fuori scope, #621 possiede il bake, #622 la workspace.
Nessuna issue possiede il gesto dell'autore.

Dipendenze hard: #620 + #621. Dipendenza UX consigliata: #622.

Scope:
- tool Geometry/Sketch nell'Hex Map Editor;
- segmenti legali secondo #620;
- ghost valid/invalid;
- snap a direzioni/junction/layer;
- thin slice Wall / LowWall / VoidFootprint;
- una gesture = una transaction Undo;
- usa validator #620 e bake #621;
- nessuna regola duplicata nell'Editor module.

Decisione da chiudere nella issue: persistenza editor-only della geometria modificabile senza creare
una seconda authority runtime né salvarla come mesh `.umap`.

### R6 — NUOVA ISSUE: Movement Probe

Titolo:
`Map Editor: Movement Probe — start, profilo e budget sopra ReachableCells autorevole`

#554 risponde «questa zona è strutturalmente raggiungibile dagli spawn?».
Il Movement Probe risponde «dove arriva questa unità/profilo con questo budget?».

Scope:
- start cell;
- profilo/unità e budget da dati reali;
- reachable set dal servizio autorevole;
- hover → best path + costo;
- cella esclusa → reason;
- refresh dopo surface/blocker/transition edit.

Divieto: niente Dijkstra/A* parallelo nell'Editor.

PIE: `PIE-HEX-MOVEMENT-PROBE`.

### R7 — Door State readability — #695

Non riaprire #553. #695 è un residuo reale:
- Closed vs Locked deve essere leggibile;
- Open vs Destroyed va distinta senza indebolire il canale principale passabile/non-passabile.

Azione: tenere #695, aggiungere P2 e dichiararla follow-up di #553.

### R8 — NUOVA ISSUE: M9 Integration Gate

Titolo:
`Map Editor M9: integration gate — Registry, EditorMap, ScenarioMap, MilestoneMap, Wiki e PIE`

Non è una nuova fonte di stato: chiude solo il wiring.

Dipendenze:
#554, #620, #621, #622, #623, #695, Geometry Authoring Tool, Movement Probe.

DoD:
- Feature Registry aggiornato per `RT-FEAT-TOOL-MAP-EDITOR` e `RT-FEAT-TOOL-MAP-GEOMETRY`;
- test/scenario/PIE refs reali;
- `editor-sessions.yaml` senza PIE orfane;
- `scenario-map.md` distingue runtime scenario da editor-only PIE;
- brief/editor docs aggiornati;
- Wiki aggiornata se pertinente;
- `feature_registry.py validate`;
- `generate --check`;
- `shortlist --check`;
- `wiki --check` se applicabile;
- `check-docs-links.py`;
- FeatureMap/MilestoneMap/ScenarioMap/EditorMap rigenerate, mai editate a mano.

## 3. Issue da aggiornare

### #620
Correggere:
- #588 è CLOSED/merged;
- edge helper è già stato consegnato da #553.

Aggiungere:
`Prerequisiti già soddisfatti: #619 e #588 chiuse; edge helper consegnato da #553. Questa issue li consuma e non li reimplementa.`

### #622
Correggere la riga che dice #588/PR #598 non mergiate.
Aggiornare Out of scope:
- #552 DONE;
- #553 DONE;
- #554 ancora OPEN.

### #554
Aggiungere:
`PR #694 contiene il delta visuale ma è stata chiusa unmerged; non è evidenza di codice in main.`

### #621
Aggiornare dipendenze:
- #619 DONE;
- hard: #620;
- #687 prima di qualunque schema persistito che richieda migrazione trasformativa.

### #623
Esplicitare A=seduta, B=small code, C=PIE; non riscrivere i controlli viewport nativi.

### #695
Aggiungere `P2` e `follow-up #553`.

## 4. Issue da NON creare / non riaprire

- niente nuova `Reachability Core`: #554 e i servizi esistenti la possiedono;
- niente secondo edge helper;
- niente riapertura #551/#552/#553;
- niente apertura anticipata dell'epic E23;
- niente scenario JSON per segmento/angolo/footprint: sono fixture runtime, non partite;
- niente `Walls[]` autorevole runtime o geometry salvata nel `.umap`.

## 5. Issue da chiudere/eliminare

Audit corrente: **nessuna issue OPEN è un duplicato sicuro da chiudere come `not planned`**.

Quindi:
- #554 resta OPEN perché #694 non è mergiata;
- #695 resta OPEN perché è un residuo reale;
- #620/#621/#622/#623 hanno scope distinti;
- #687 resta P1;
- #324/E23 resta post-v0.1.

L'eliminazione utile è evitare duplicati: nessun secondo pathfinder, reachability system, edge helper
o authority geometry.

## 6. Percorso critico

```text
#687 P1
   │
   └──────────── protects serialized schemas

#554 recover PR694 ──► #622 ──► Movement Probe
        │
        └────────────► #695

#619 DONE
   ↓
#620
   ↓
#621
   ↓
Geometry Authoring Tool
   └── uses #622

#623
   ↓
M9 Integration Gate
   ├── Feature Registry
   ├── EditorMap
   ├── ScenarioMap
   ├── FeatureMap
   ├── MilestoneMap
   ├── PIE
   └── Wiki/docs
```

Ordine pratico:
1. #687
2. #554
3. #620
4. #621
5. #622
6. Geometry Authoring Tool
7. Movement Probe
8. #695
9. #623
10. M9 Integration Gate

#554 e #620 possono procedere in parallelo dopo il triage.

## 7. DoD Editor v0.1

L'Editor è chiuso quando l'autore può:
1. vedere il workspace prima di creare celle;
2. creare/dipingere celle e superfici;
3. leggere costo, movement block e LOS block senza Details;
4. lavorare in multilayer con Focus;
5. vedere transizioni e isole irraggiungibili;
6. vedere cover e DoorState sul bordo corretto;
7. disegnare geometry nella grammatica quantizzata;
8. usare ghost/snap e Undo/Redo;
9. cuocere geometry in dati canonici;
10. usare Movement Probe start/profilo/budget;
11. validare mappe e reason;
12. usare scenari runtime come anchor e PIE per editor-only visuals;
13. salvare/riaprire senza drift o seconda authority;
14. rigenerare Registry/FeatureMap/ScenarioMap/EditorMap/MilestoneMap dai loro owner.
