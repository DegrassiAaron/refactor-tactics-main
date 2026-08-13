> 🔎 **ESITO DELLA REVISIONE — 2026-08-13.** Sorgente **recepito in parte**. Referto:
> [`hexgeometry-editor-spec-panel-2026-08-13.md`](../../../roadmap/plans/hexgeometry-editor-spec-panel-2026-08-13.md).
>
> ✅ **È il documento più accurato della serie**: 29 verifiche di stato su 29 corrette — dieci issue, sette
> simboli di codice, otto feature ID, quattro decisioni. Nessuna premessa falsa, primo caso in sei handoff.
> Il suo §27 aveva ragione: l'owner tecnico mancava davvero, ed è stato creato —
> [`spec-hex-geometry-authoring.md`](../../../technical/spec-hex-geometry-authoring.md). Il §45 e il §26 sono
> entrati quasi intatti in quell'owner.
>
> 🔴 **Una prescrizione è superata, ed è nel percorso critico**: il **§12** dice
> `Void / cliff footprint → ERTHexSurface::Void`. Quella proposta è stata valutata e **respinta il
> 2026-08-12** — *«il bake non scrive `Surface`»*, vedi [`OPEN_DECISIONS.md`](../../../OPEN_DECISIONS.md):
> `Fill` propaga sulla contiguità di superficie, quindi una `Surface` cotta cambierebbe il confine di ogni
> futuro flood fill, cioè lo **strumento** e non il dato. Il precipizio si esprime con
> `bBlocksMovement = true` + `bBlocksLineOfSight = false`. Il §32 Step B cita il §12 come parte della DoD di
> `#621`: **chi apre quella issue con questo documento in mano implementerebbe la cosa respinta.**
>
> ⚠️ Il **§7** dà le soglie `0–3 / 4–5 / 6+` come baseline stabile, senza sapere che `MSE-2` le mette in
> discussione con numeri misurati. E la revisione ha trovato, confrontandolo col bundle `grid/`, che il
> repository ha **due modelli di calpestabilità** mai messi a confronto: `MSE-3`.

# RefactorTactics — Claude Implementation & Consolidation Brief
## Hex Grid, Geometry Authoring, Bake e Map Editor
### Snapshot di partenza: 2026-08-13 — DA RIVERIFICARE PRIMA DI MODIFICARE IL REPOSITORY

> Questo file è un **handoff operativo per Claude Code**.
>
> Obiettivo: implementare e consolidare il modello **esagoni ↔ geometria architettonica ↔ dati tattici ↔ Map Editor**,
> aggiornando codice, issue, roadmap, Feature Registry, documentazione tecnica e Wiki senza creare una seconda
> authority o duplicare sistemi già presenti.
>
> Regola principale: **non fidarti dello snapshot di questo file come stato corrente**.
> Prima di toccare codice o documenti, misura `origin/main`, le issue e le PR correnti. In questo progetto
> un handoff può diventare stale in minuti.

---

# 0. Missione

Portare il repository a una spiegazione unica e implementabile di questi concetti:

1. cosa significa una **cella esagonale** in RefactorTactics;
2. come funzionano coordinate, direzioni, bordi e layer;
3. perché la **geometria architettonica non coincide necessariamente con i lati degli hex**;
4. come una geometria world/editor-space viene **quantizzata**, validata e trasformata in dati tattici;
5. come funziona l'**occupancy a 12 settori** senza introdurre 12 direzioni di movimento;
6. come muri, muretti, void, cover, porte e transizioni diventano dati runtime;
7. quale parte appartiene al runtime e quale all'Editor;
8. come il Map Editor deve visualizzare, disegnare, validare e fare Undo/Redo;
9. come il Movement Probe deve usare la simulazione reale, non un pathfinder editor parallelo;
10. quali documenti sono canonici, quali storici e quali generati.

Il risultato non deve essere soltanto codice che funziona. Deve essere **comprensibile, documentato,
testabile e tracciato**.

---

# 1. Prima di tutto: audit obbligatorio del repository

Prima di implementare qualsiasi cosa:

```bash
git fetch --all --prune
git status
git log -20 --oneline --decorate
gh issue view 620
gh issue view 621
gh issue view 622
gh issue view 623
gh issue view 687
gh issue view 695
gh issue view 711
gh issue view 712
gh issue view 324
gh issue list --state open --limit 200
gh pr list --state open --limit 100
```

Poi cerca nel codice e nei documenti:

```bash
git grep -n "FRTCellId"
git grep -n "ERTHexDirection"
git grep -n "EdgeMidpointWorld"
git grep -n "EdgeRotation"
git grep -n "OppositeDirection"
git grep -n "FRTHexCover"
git grep -n "FRTHexDoor"
git grep -n "OccupancySurcharge"
git grep -n "CoreBlocked"
git grep -n "ReachableCells"
git grep -n "FromCell"
git grep -n "RebuildInstances"
git grep -n "RT-FEAT-TOOL-MAP-EDITOR"
git grep -n "RT-FEAT-TOOL-MAP-GEOMETRY"
git grep -n "MSE-1"
```

Leggi almeno:

```text
AGENTS.md
CLAUDE.md
docs/decisions/adr-0002-griglia-esagonale.md
docs/roadmap/hex-map-roadmap.md
docs/roadmap/roadmap-checkpoint.md
docs/roadmap/roadmap_lane_4.md            (se ancora corrente)
docs/technical/brief-editor-map-viz.md
docs/roadmap/plans/map-sketch-editor-spec-panel-2026-08-12.md
docs/roadmap/plans/mapeditor-integration-spec-panel-2026-08-12.md
docs/OPEN_DECISIONS.md
docs/roadmap/feature-registry.yaml
docs/technical/test-manuali-pie.md
docs/roadmap/editor-sessions.yaml
```

## Regola di freschezza

Non dedurre lo stato del lavoro dallo stato di **una sola PR**.

Caso già successo:
- una PR stacked può essere chiusa senza merge;
- lo stesso branch/delta può essere ricollocato e mergiato con un'altra PR.

Quindi verifica:
- issue;
- branch;
- PR col branch come `head`;
- commit presente o meno in `origin/main`;
- documenti derivati.

---

# 2. Gerarchia delle fonti

Usa questa precedenza:

```text
decisione esplicita dell'autore
    >
ADR / Decision Log CANONICAL
    >
specifica tecnica CURRENT / AS-BUILT
    >
codice + test su origin/main
    >
Feature Registry
    >
roadmap corrente
    >
issue GitHub
    >
referti/spec-panel
    >
handoff AI
```

Un referto o questo file **non possono sovrascrivere** un ADR o una decisione consolidata.

Se trovi una contraddizione:
1. non scegliere in silenzio;
2. misura;
3. documenta;
4. usa `OPEN_DECISIONS.md` se serve davvero una scelta dell'autore.

---

# 3. Modello canonico della griglia esagonale

## 3.1 L'hex è l'unico substrato di gioco

Il quadrato è storico. Non introdurre compatibilità o astrazioni per una griglia quadrata rimossa.

La cella canonica è:

```cpp
FRTCellId
{
    X,      // coordinata assiale Q
    Y,      // coordinata assiale R
    Layer
}
```

Coordinate cubiche derivate:

```text
Q = X
R = Y
S = -Q - R
```

`Layer` distingue piani sovrapposti:

```text
ground
bridge
roof
tunnel
upper floor
ecc.
```

Due celle con stesso `(X,Y)` e `Layer` diverso sono **due celle distinte**.

Non esiste adiacenza verticale implicita.

Per passare fra layer serve una **transizione esplicita nel grafo**.

---

# 4. Orientamento pointy-top e sei direzioni

La griglia è **pointy-top**.

Le direzioni tattiche sono esattamente sei:

```text
E   (+1,  0)
NE  (+1, -1)
NW  ( 0, -1)
W   (-1,  0)
SW  (-1, +1)
SE  ( 0, +1)
```

Non introdurre:

```text
North
South
N
S
```

come direzioni hex.

In un pointy-top esistono vertici in alto e in basso, ma **non lati puramente Nord/Sud**.

Questa distinzione deve comparire chiaramente nella documentazione, perché ha già generato errori
nell'authoring e nella visualizzazione.

---

# 5. Hex direction ≠ geometry direction

Questa è una delle informazioni più importanti da consolidare.

## 5.1 Le sei direzioni sono del grafo tattico

Servono per:

- vicini;
- facing;
- edge;
- cover direzionale;
- porte su bordo;
- traversal;
- LOS/attacchi quando la regola usa un lato.

Sono **sei** e restano sei.

## 5.2 La geometria architettonica NON è obbligata a seguire i sei lati

Una parete architettonica può:

- attraversare una cella;
- passare vicino al centro;
- invadere parzialmente più celle;
- formare junction;
- formare angoli a 90°;
- seguire una direttrice che non coincide con il perimetro di un singolo esagono.

Quindi:

```text
HEX = discretizzazione tattica
GEOMETRIA = gesto di authoring / forma architettonica
```

Non sono la stessa cosa.

## 5.3 Ma la geometria non è float arbitrario

Per mantenere il determinismo, la geometria tatticamente significativa usa una **grammatica quantizzata**.

La baseline corrente di #620 ammette:

1. direttrici principali derivate dall'esagono;
2. ortogonali a tali direttrici;
3. segmenti sul lato/perimetro dell'esagono;
4. junction compatibili con la grammatica.

Queste famiglie producono una griglia di orientamenti quantizzati a incrementi coerenti con l'esagono.

L'effetto pratico è che sono possibili anche configurazioni architettoniche a **90°**, senza trasformare
la mappa in un sistema di sei soli muri possibili.

### Regola

```text
la geometria può NON stare sul bordo dell'hex
ma la geometria tattica NON può avere endpoint/angoli float arbitrari nell'authority serializzata
```

---

# 6. Le 12 occupancy sectors NON sono 12 direzioni

L'occupancy a 12 settori divide la cella in **dodici settori angolari da 30°**, più il core.

Serve a descrivere **quanto e dove una geometria invade una cella**.

Non serve a definire:
- movimento;
- facing;
- edge direction;
- cover direction;
- numero di vicini.

Quindi:

```text
movement/facing/edge directions = 6
occupancy measurement sectors  = 12
```

Non confondere mai i due vocabolari.

La rappresentazione deve essere compatta e deterministica:

```text
SectorMask    // 12 bit
CoreBlocked   // centro occupato
Classification
```

Esempio concettuale già usato nel progetto:

```text
Cell: FRTCellId{X,Y,Layer}
Occupied Sectors: 5 / 12
Mask: 001111000100
Core: Free
Classification: Constrained
```

Il settore `0` deve avere un riferimento pointy-top dichiarato e stabile.

---

# 7. Classificazione occupancy

Baseline corrente:

```text
0–3 settori  → Free
4–5          → Constrained
6+           → Blocked
CoreBlocked  → Blocked indipendentemente dal conteggio
```

Le soglie sono **dati/regole runtime verificabili**, non preferenze private del tool.

`Constrained` deve avere un consumatore reale.

Il progetto ha già rifiutato il pattern:

```text
campo scritto
ma nessun sistema lo legge
```

---

# 8. Constrained e costo di movimento

`Constrained` non deve essere solo un'etichetta.

Il consumatore minimo già introdotto è un sovrapprezzo di movimento.

## Importante

Il sovrapprezzo **NON deve vivere dentro `MoveCost`**.

`MoveCost` viene ricalcolato dalla superficie durante i cambi dinamici.

Se il surcharge fosse fuso in `MoveCost`:

```text
corridoio stretto
→ superficie cambia in acqua
→ Cleanup ripristina Floor.MoveCost
→ il costo geometrico sparisce
```

Quindi il modello corretto mantiene un campo separato, già introdotto nel progetto:

```cpp
OccupancySurcharge
```

e i consumatori usano il costo totale canonico, non una somma duplicata in cinque funzioni diverse.

Non reimplementare la somma in nuovi consumer.

---

# 9. Bordo dell'hex: una sola primitive geometrica

Cover e porte sono proprietà di **bordo**.

Il progetto ha già introdotto helper condivisi.

Non ricrearli.

Usa:

```cpp
URTHexLibrary::EdgeMidpointWorld(...)
URTHexLibrary::EdgeRotation(...)
URTHexLibrary::OppositeDirection(...)
```

Invariante:

```text
il bordo E di A e il bordo W del vicino
sono lo STESSO bordo fisico
```

Il midpoint deve coincidere visto da entrambe le celle.

Non derivare il bordo scegliendo a mano indici di `HexCorners`.

Non incidere angoli in codice.

---

# 10. Separare tre concetti che sembrano "muro"

Nel modello tattico vanno distinti:

## 10.1 Occupancy di cella

Risponde:

> quanto la geometria invade lo spazio in cui un'unità potrebbe stare?

Può produrre:

```text
Free
Constrained
Blocked
```

## 10.2 Edge traversal / cover

Risponde:

> cosa succede attraversando o sparando attraverso QUESTO bordo?

È una proprietà topologica/direzionale.

## 10.3 Geometry authoring

È ciò che il level designer disegna.

Non è l'authority runtime.

Una geometria può attraversare celle e poi **cuocere** in:
- occupancy;
- `bBlocksMovement`;
- cover su edge;
- Void;
- eventuali altri dati canonici.

---

# 11. Bake: Authoring Geometry → Runtime Spatial Data

Flusso canonico:

```text
Authoring Geometry
      ↓
quantization + validation
      ↓
affected cells / touched edges
      ↓
occupancy
      ↓
bake
      ↓
canonical runtime map data
      ↓
pathfinding / LOS / combat / simulation
```

Dopo il bake:

```text
la geometria è arte
```

Il runtime NON deve interrogare una mesh per sapere:
- se può passare;
- se vede;
- se ha cover;
- se una cella è standable.

Il runtime legge **dati tattici canonici**.

---

# 12. Mapping minimo della cottura

Baseline della issue #621:

```text
LOW WALL
    → FRTHexCover{Low}

WALL
    → FRTHexCover{High}

Solid footprint
    → bBlocksMovement
      secondo classificazione/occupancy

Void / cliff footprint
    → ERTHexSurface::Void
```

Non creare un secondo `Walls[]` autorevole interrogato dal gameplay.

Se serve conservare la geometry di authoring per modificarla in seguito, quella è **source data editor-only**,
non il dato letto dal resolver.

---

# 13. Muri bassi e alti: non duplicare il concetto di cover

Il progetto possiede già il vocabolario:

```text
ERTHexCoverType::Low
ERTHexCoverType::High
```

Semantica corrente:

### Low
- protegge dai colpi diretti che attraversano quel bordo;
- non blocca movimento;
- non blocca LOS.

### High
- nega traversal attraverso il bordo;
- nega LOS attraverso il bordo;
- nega projectile/direct line attraverso il bordo.

Un `LOW WALL` visuale che non cuoce in `FRTHexCover{Low}` creerebbe **due rappresentazioni dello stesso oggetto**.

Non farlo.

---

# 14. Porte

Una porta è un dato logico legato al bordo/transizione.

La mesh non decide se è aperta.

Gli stati già esistenti includono:

```text
Open
Closed
Locked
Destroyed
```

La visualizzazione dell'editor deve distinguere il dato importante.

Almeno:

```text
passabile / non passabile
```

deve essere il canale dominante.

Il follow-up #695 esiste perché quattro stati erano collassati in due forme.

Prima di lavorarci, riverifica lo stato dell'issue.

---

# 15. Layer e transizioni

Due celle sovrapposte non sono collegate perché si trovano una sopra l'altra.

Serve un arco/transizione.

Questo vale per:

```text
bridge
stairs
ramp
tunnel
elevator
special transition
```

L'Editor deve far vedere:
- layer attivo;
- contesto dei layer vicini;
- transizioni;
- verso della transizione quando rilevante;
- piattaforme irraggiungibili.

La vista `Focus` esiste già: non ricrearla.

La serie editor-viz #551–#554 è già stata consolidata come implementata.
Non riaprirla salvo regressione reale.

---

# 16. Runtime vs Editor: confine obbligatorio

## Runtime possiede

- coordinate;
- geometry grammar;
- validation;
- occupancy;
- classification;
- bake;
- pathfinding;
- reachability;
- LOS;
- targeting;
- costi;
- reason code;
- hash/revision;
- dati serializzati;
- invarianti deterministici.

## Editor possiede

- input mouse;
- tool state;
- ghost;
- snap;
- highlight;
- overlays;
- widget/pannelli;
- Undo/Redo transaction;
- visualizzazione;
- chiamate ai servizi runtime.

### Regola

```text
Editor chiama Runtime
Runtime non dipende da Editor
```

Non mettere una regola competitiva in `Source/RefactorTacticsEditor/`.

---

# 17. Rendering editor: la vista non è il dato

`ARTHexMapActor` usa geometria instanced/transient per rappresentare il dato.

Il principio già consolidato è:

```text
data → RebuildInstances → visualization
```

Non:

```text
visual mesh → gameplay
```

La geometria di debug/lettura:

- è transient;
- viene rigenerata dal dato;
- non si salva come seconda verità nel `.umap`;
- non deve rubare i click.

Solo il componente delle celle destinato al picking deve avere la collisione prevista.

Ogni nuovo componente visuale deve rispettare il test/invariante equivalente a:

```text
OnlyTheCellsComponentIsClickable
```

---

# 18. Vocabolario visuale dell'Editor

Decisione già emersa e valida:

```text
FORMA  = cosa fa
COLORE = che superficie è
```

Esempi:

```text
wall/column         → non si passa
low slab            → blocco LOS ma traversabile
edge panel          → cover/door
relief              → costo
surface color       → terrain
arrow               → transition
warning marker      → unreachable
```

Non codificare tutto con colori diversi.

L'Editor può usare colori aggressivi: è uno strumento per il designer, non E21/UI finale.

---

# 19. Workspace Grid — #622

La griglia di lavoro mostra **dove potresti creare celle**, non dice che quelle celle esistono.

Deve essere distinguibile da:

```text
existing map cell
```

Requisiti:

- ghost;
- transient;
- estensione controllabile;
- nessun Actor per cella;
- nessun `DemoRadius` usato come falsa sorgente dati;
- non deve creare click ambigui;
- niente salvataggio nel `.umap`.

Prima di implementare: riverifica #622.

---

# 20. Geometry Authoring Tool — #712

Snapshot conosciuto: esiste una issue dedicata al gesto dell'autore.

Titolo noto:

```text
#712 — Il gesto dell'autore: disegnare geometria quantizzata con ghost, snap e un solo Ctrl+Z
```

Dipende da:
- #620 grammar/validator;
- #621 bake;
- usa bene #622 come workspace UX.

Thin slice:

```text
Wall
LowWall
VoidFootprint
```

Richieste:

- ghost valid/invalid prima del commit;
- snap alla grammatica;
- una gesture = una Unreal transaction;
- un solo Ctrl+Z annulla l'intera gesture;
- validator chiamato dal runtime;
- bake chiamato dal runtime;
- nessuna seconda implementazione nell'Editor.

---

# 21. MSE-1: non decidere di nascosto

Esiste una decisione aperta sulla persistenza della geometry di authoring.

Domanda:

```text
se il designer modifica manualmente un dato già cotto,
e poi rifà il bake, quale sorgente vince?
```

E più in generale:

```text
dove vive il source editabile della geometria?
```

Non risolvere questa domanda dentro un commit di #712.

Prima:
1. trova `MSE-1` in `OPEN_DECISIONS.md`;
2. verifica se è ancora aperta;
3. se aperta, non introdurre una seconda authority.

Vincolo già valido:

```text
NON salvare la geometria tattica come mesh autorevole nel .umap
```

---

# 22. Movement Probe — #711

Esiste una issue dedicata.

Domanda:

> dove arriva questa unità con questo profilo e questo budget, e perché quella cella no?

Non confonderla con la raggiungibilità strutturale della mappa.

## Deve usare

```cpp
URTHexSimLibrary::ReachableCells(...)
```

e il predecessore già disponibile:

```cpp
FRTHexReachableCell::FromCell
```

Il path su hover si ricostruisce risalendo `FromCell`.

Non lanciare un A* per ogni cella.

Non scrivere un secondo Dijkstra.

## Il probe deve mostrare

- start;
- unit/profile;
- budget reale;
- reachable set;
- costo cumulato;
- path in hover;
- reason per cella esclusa;
- refresh su revisione dati.

Il `reason` deve usare il vocabolario runtime esistente.

---

# 23. Determinismo

Tutto ciò che influenza il gameplay deve essere:

- integer/enum/stable ID;
- indipendente dall'ordine delle collection;
- hashabile;
- riproducibile;
- stabile fra macchina e macchina.

Evita endpoint float arbitrari come dato competitivo.

Il mondo Unreal può usare `FVector` per disegnare.

Ma la **decisione tattica serializzata** non deve dipendere da rounding casuale di coordinate float.

---

# 24. Pathfinding e geometry

La geometria non modifica il pathfinding direttamente.

Fa:

```text
geometry
  → bake
    → cells/edges/cost/block data
      → graph/pathfinding
```

Il pathfinding continua a leggere il grafo tattico.

A* resta il point-to-point autorevole.

Reachable area resta bounded-cost traversal/Dijkstra tramite il servizio runtime esistente.

NavMesh/Recast non è l'authority del movimento competitivo.

---

# 25. LOS e movement non sono la stessa regola

Non usare un solo `Blocked` generico per tutte le domande.

Il progetto distingue almeno:

```text
movement blocking
LOS blocking
cover
edge traversal
occupancy
```

Una cella/feature può essere:

```text
traversabile ma opaca
```

oppure:

```text
non traversabile ma con una diversa semantica di vista
```

Non collassare queste proprietà per semplificare il tool.

L'Editor deve renderle leggibili separatamente.

---

# 26. Geometry ≠ cover ≠ obstacle

Consolidare questa tabella nei documenti tecnici:

| Concetto | Che domanda risponde | Authority |
|---|---|---|
| Geometry authoring | cosa ha disegnato il designer? | editor/source authoring |
| Occupancy | quanto una cella è invasa? | runtime data |
| Movement block | posso stare/entrare/passare? | runtime data |
| LOS block | la vista attraversa? | runtime data |
| Cover | ricevo protezione attraversando questo edge? | runtime edge data |
| Door | questo edge è transitabile nello stato corrente? | runtime edge/object state |
| Transition | quali celle/layer sono connessi? | runtime graph |
| Mesh | cosa vede l'umano? | presentation only |

---

# 27. Documentazione: creare UN owner tecnico, non una quinta roadmap

Obiettivo di consolidamento:

deve esistere **un solo documento tecnico corrente** che spieghi:

```text
hex model
directions
geometry grammar
12-sector occupancy
bake
runtime/editor split
edge semantics
authoring tool
movement probe
```

## Prima cerca un owner già adatto

```bash
git grep -n "Geometry Authoring"
git grep -n "occupancy a 12"
git grep -n "geometria architettonica"
git grep -n "Authoring Geometry"
git grep -n "Geometry.*Bake"
```

Se esiste già una specifica `CURRENT`/`CANONICAL` che ha questo ruolo:
- aggiorna quella;
- non crearne un'altra.

Se non esiste un owner unico, crea:

```text
docs/technical/spec-hex-geometry-authoring.md
```

con stato:

```text
CURRENT
```

Non trasformarlo in tracker.

Lo stato di implementazione continua a vivere in:
- Feature Registry;
- issue;
- roadmap corrente;
- editor sessions / PIE.

---

# 28. Struttura consigliata del documento owner

Se devi creare/aggiornare l'owner, usa questa struttura:

```text
1. Scopo
2. Hex coordinate e Layer
3. Pointy-top e 6 direzioni
4. Bordo condiviso
5. Geometry non vincolata ai lati
6. Grammar quantizzata
7. Occupancy 12-sector
8. Free / Constrained / Blocked
9. OccupancySurcharge
10. Bake authoring → runtime
11. Wall / LowWall / Void
12. Cover / Door / Transition
13. Runtime vs Editor
14. Visualization invariants
15. Workspace Grid
16. Geometry Tool
17. Movement Probe
18. Determinismo / hash / revision
19. Validation
20. Testing
21. Decisioni aperte
22. Link alle issue
```

---

# 29. Documenti da consolidare, non duplicare

Controlla e aggiorna i riferimenti se necessario:

```text
docs/decisions/adr-0002-griglia-esagonale.md
docs/roadmap/hex-map-roadmap.md
docs/technical/brief-editor-map-viz.md
docs/roadmap/plans/map-sketch-editor-spec-panel-2026-08-12.md
docs/roadmap/plans/mapeditor-integration-spec-panel-2026-08-12.md
docs/roadmap/roadmap-checkpoint.md
docs/roadmap/feature-registry.yaml
docs/OPEN_DECISIONS.md
docs/technical/test-manuali-pie.md
docs/roadmap/editor-sessions.yaml
```

### Importante

`hex-map-roadmap.md` è **storico DELIVERED**.

Non usarlo come tracker corrente e non riaprirlo.

Puoi:
- correggere link evidentemente rotti se la policy lo consente;
- aggiungere un puntatore all'owner corrente se serve.

Non riscrivere la storia come se fosse ancora in esecuzione.

---

# 30. Wiki

Dopo aver consolidato la specifica tecnica, controlla se la Wiki ha già pagine per:

```text
Griglia esagonale
Mappe
Level Editor
Coperture
Muri e porte
Movimento
```

Non creare pagine duplicate.

La Wiki deve spiegare il concetto a un lettore di prodotto/design:

- 6 direzioni del personaggio;
- geometry che può tagliare gli esagoni;
- 12 settori come misura di occupancy, non movimento;
- bake verso dati tattici;
- editor come strumento, non authority.

La Wiki non deve diventare una copia delle issue.

---

# 31. Issue: comportamento richiesto a Claude

Per ogni gap:

1. cerca prima issue open e closed;
2. cerca PR recenti;
3. verifica se il codice è già in `main`;
4. aggiorna una issue esistente se possiede già il delta;
5. crea una issue nuova solo se nessuna esistente ha ownership.

## Snapshot noto da riverificare

Alla stesura di questo handoff risultavano:

```text
#554 DONE
#619 DONE
#620 OPEN  — grammar + validator
#621 OPEN  — bake
#622 OPEN  — workspace
#687 OPEN  — serialized versioning bug, P1
#695 OPEN  — door-state visual distinction
#711 OPEN  — Movement Probe
#712 OPEN  — Geometry Authoring Tool
#324 OPEN/post-v0.1 — E23, non aprire anticipatamente come epic
```

NON assumere che siano ancora così.

---

# 32. Implementazione consigliata

## Step A — #620

Implementa/chiudi grammar + validator.

Deve usare gli edge helper esistenti.

Test minimi:

```text
legal principal direction
legal orthogonal direction
legal perimeter direction
invalid off-axis
invalid junction
zero-length
duplicate
invalid layer
outside editable bounds
permutation invariance
```

Verifica di mutazione richiesta.

---

## Step B — #621

Implementa/chiudi il bake puro runtime.

Test:

```text
same geometry → same affected cells
order-independent
LowWall → correct Low cover edge
Wall → correct High cover edge
solid footprint → movement block
void footprint → Void
rebake region bounded
ValidateMap remains green
```

Non fare una migrazione trasformativa se #687 è ancora aperta.

---

## Step C — #622

Workspace grid.

PIE obbligatoria perché la differenza "ghost vs real" è visuale.

---

## Step D — #712

Geometry Tool.

Usa:
- #620 per validazione;
- #621 per bake;
- #622 per workspace.

Non mettere regole nel modulo Editor.

---

## Step E — #711

Movement Probe.

Riusa:
- `ReachableCells`;
- `FromCell`;
- reason code runtime;
- graph revision.

Non costruire un nuovo pathfinder.

---

## Step F — visual/editor residue

Rimisura:

```text
#623
#695
PIE-HEX-VIZ-*
PIE-HEX-MOVEMENT-PROBE
PIE del Geometry Tool
```

Chiudi soltanto dopo verifica reale nell'Editor.

---

# 33. Test automatici vs PIE

## Automation / headless

Usa per:

- grammar;
- quantization;
- sector mask;
- classification;
- bake;
- edge mapping;
- costs;
- path/reachability;
- deterministic ordering;
- hash/revision;
- serialization/migration.

## PIE / editor session

Usa per:

- ghost leggibile;
- snap percepibile;
- workspace ≠ real cell;
- Undo/Redo visuale;
- door states leggibili;
- transition visibility;
- overlay composition;
- mouse selection;
- layer focus;
- frame whole map;
- lighting.

Non chiamare uno scenario JSON ciò che è una verifica interattiva di authoring.

---

# 34. Scenari

Le fixture geometriche non sono partite.

Quindi:

```text
segmento
angolo
footprint solido
footprint void
```

vanno nei test runtime/fixture data.

Uno scenario JSON esiste solo quando vuoi dimostrare un comportamento di partita, ad esempio:

```text
una unità non attraversa il muro cotto
una cover Low riduce correttamente un colpo sul bordo
un Void impedisce una posizione
```

Non estendere lo schema dello Scenario Harness per poterci infilare input di editor.

---

# 35. Registry e viste generate

Quando una issue tocca una feature, aggiornare il suo owner nel Feature Registry nello stesso lavoro.

Feature rilevanti:

```text
RT-FEAT-TOOL-MAP-EDITOR
RT-FEAT-TOOL-MAP-GEOMETRY
```

Poi:

```bash
python scripts/feature_registry.py validate
python scripts/feature_registry.py generate
python scripts/feature_registry.py shortlist
python scripts/feature_registry.py wiki --check
python scripts/check-docs-links.py
```

Dopo la generazione:

```bash
python scripts/feature_registry.py generate --check
python scripts/feature_registry.py shortlist --check
```

Non editare manualmente le shortlist generate.

---

# 36. EditorMap e PIE

Ogni nuova verifica PIE dell'Editor deve stare in:

```text
docs/technical/test-manuali-pie.md
```

e anche in una seduta di:

```text
docs/roadmap/editor-sessions.yaml
```

Una voce PIE senza seduta rischia di non essere mai eseguita.

Non cercare però di "eliminare tutte le PIE orfane del repository" dentro un lavoro Map Editor:
molte appartengono a combat/status/altre feature.

Lavora sul perimetro dell'issue.

---

# 37. Versioning — #687

Prima di qualunque migrazione reale verifica la situazione.

Problema noto:

```text
FormatVersion uguale al default del CDO
→ delta serialization può non scriverlo
→ asset vecchio caricato col codice nuovo prende il default nuovo
→ MigrateToCurrentFormat crede che l'asset sia già aggiornato
```

Se #687 è ancora aperta:

- non promettere una migrazione trasformativa;
- non chiudere un DoD che dipende dal fatto che quella migrazione parta.

Se la risolvi:
- test old-binary writer / new-binary reader;
- non soltanto `NewObject + FormatVersion = old`.

---

# 38. Performance

Non ottimizzare `RebuildInstances` o il bake "per principio".

Prima misura.

Il rebuild completo ha una proprietà importante:

```text
la vista torna sempre a derivare dal dato
```

Un incremental update introduce possibilità di:
- residui;
- geometry orfana;
- vista divergente.

Passa a incremental soltanto se:
1. esiste un problema misurato;
2. c'è un test che confronta incremental vs full rebuild;
3. il risultato resta deterministico.

---

# 39. Debug

Aggiungi debug utile, non spam.

Il designer deve poter capire:

```text
CellId
Layer
Surface
MoveCost
OccupancySurcharge
Occupancy Mask
CoreBlocked
Classification
Edges
Cover
Door
Transition
Reachable Cost
Blocked Reason
Revision
```

Non serve mostrare tutto contemporaneamente.

Usa toggle/focus.

---

# 40. Errori da evitare

❌ 12 occupancy sector interpretati come 12 facing direction  
❌ North/South aggiunti a `ERTHexDirection`  
❌ muro obbligato a stare sul lato di una cella  
❌ endpoint float arbitrario nella struttura autoritativa  
❌ `LOW WALL` separato da `FRTHexCover{Low}`  
❌ mesh usata dal runtime per decidere passaggio/LOS  
❌ geometry salvata nel `.umap` come seconda authority  
❌ secondo A* o Dijkstra nel modulo Editor  
❌ reason code Editor-only  
❌ secondo calcolo del midpoint del bordo  
❌ `MoveCost` sovrascritto col surcharge geometry  
❌ regole competitive in `Source/RefactorTacticsEditor/`  
❌ Actor-per-cell  
❌ NavMesh come authority  
❌ nuova roadmap manuale che duplica Feature Registry  
❌ scenario JSON usato per una verifica visuale dell'editor  
❌ dichiarare "done" da una PR senza verificare `origin/main`

---

# 41. Output documentale richiesto

Alla fine del lavoro deve essere facile rispondere a queste domande leggendo il repository:

### Hex
- Quali sono le coordinate?
- Perché ci sono 6 direzioni?
- Che cosa significa Layer?
- Come si passa fra layer?

### Geometry
- Un muro deve stare sul bordo dell'hex?
- Quali orientamenti sono legali?
- Perché esistono 12 occupancy sectors?
- Che differenza c'è fra `Constrained` e `Blocked`?
- Dove vive il surcharge?
- Come viene fatto il bake?
- Perché runtime non legge le mesh?

### Editor
- Come si disegna?
- Dove avviene lo snap?
- Chi valida?
- Chi cuoce?
- Come funziona Undo?
- Come vedo il workspace?
- Come verifico la mobilità?
- Quale codice è runtime e quale editor?

Se una di queste risposte richiede leggere cinque issue storiche, il consolidamento non è finito.

---

# 42. Definition of Done del lavoro Claude

Il lavoro è completato solo quando:

- [ ] stato repo/issue/PR rimisurato;
- [ ] nessun sistema duplicato;
- [ ] owner tecnico hex-geometry identificato o creato;
- [ ] 6 directions vs 12 sectors spiegato senza ambiguità;
- [ ] geometry-not-bound-to-hex-edge spiegata;
- [ ] grammar quantizzata spiegata;
- [ ] bake verso runtime canonical data spiegato;
- [ ] cover/wall/door/transition separati correttamente;
- [ ] runtime/editor split documentato;
- [ ] #620/#621/#622/#711/#712 allineate allo stato reale;
- [ ] `MSE-1` non decisa di nascosto;
- [ ] test runtime aggiunti/aggiornati dove serve;
- [ ] PIE aggiunte e collocate in editor session dove serve;
- [ ] Feature Registry aggiornato;
- [ ] viste generate rigenerate;
- [ ] Wiki consolidata senza duplicati;
- [ ] Game target compila se toccato;
- [ ] Editor target compila;
- [ ] Automation eseguita, con **eseguiti vs dichiarati**;
- [ ] `feature_registry.py validate` verde;
- [ ] `generate --check` verde;
- [ ] `shortlist --check` verde;
- [ ] `check-docs-links.py` verde;
- [ ] ogni issue chiusa ha evidenza del proprio DoD;
- [ ] report finale distingue: implementato / documentato / ancora aperto / decisione richiesta.

---

# 43. Strategia Git consigliata

Non fare una mega-PR.

Sequenza preferita:

```text
docs/hex-geometry-owner
feat/620-geometry-grammar
feat/621-geometry-bake
feat/622-workspace-grid
feat/712-geometry-tool
feat/711-movement-probe
docs/wiki-registry-consolidation
```

Adatta i branch alle issue realmente aperte.

Ogni PR:
- scope stretto;
- test;
- documenti owner;
- Registry;
- PIE se pertinente;
- link issue;
- niente generated stale.

---

# 44. Report finale che voglio da Claude

Restituisci:

```text
1. Stato iniziale misurato
2. Decisioni canoniche confermate
3. Contraddizioni/stale trovati
4. File modificati
5. Codice implementato
6. Issue create/aggiornate/chiuse
7. Documenti consolidati
8. Wiki aggiornata
9. Test automatici: dichiarati / eseguiti / falliti
10. PIE aggiunte/eseguite
11. Registry/maps rigenerate
12. Decisioni ancora aperte
13. Rischi residui
14. Commit/PR prodotti
15. Prossima issue raccomandata
```

Non scrivere "tutto allineato" senza mostrare i comandi che lo dimostrano.

---

# 45. Sintesi non negoziabile

Se devi ricordare soltanto dieci righe, sono queste:

```text
1. RefactorTactics usa SOLO hex pointy-top.
2. FRTCellId = X(Q), Y(R), Layer; il verticale passa da transizioni esplicite.
3. Movimento/facing/edge hanno 6 direzioni.
4. I 12 settori sono OCCUPANCY, non 12 direzioni.
5. La geometria architettonica può tagliare gli hex e fare 90°.
6. La geometria tattica è però quantizzata: niente endpoint float arbitrari nell'authority.
7. Geometry authoring → validate → bake → canonical runtime data.
8. Runtime non interroga mesh; Editor non possiede regole competitive.
9. Wall/LowWall cuociono nei dati edge/cover già esistenti, non in un secondo sistema.
10. Movement Probe usa ReachableCells/FromCell reali: nessun secondo pathfinder.
```

Queste regole devono risultare coerenti in codice, documenti, issue, Wiki e test.
