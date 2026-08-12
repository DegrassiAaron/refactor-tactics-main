# Griglia esagonale ed editor mappa — il prompt che ha aperto il pivot

> **Non è fonte normativa**, ed è il sorgente **più consumato** dell'intera cartella: da qui nascono
> [ADR-0002](../../decisions/adr-0002-griglia-esagonale.md) e le milestone **H0–H9** di
> [`hex-map-roadmap.md`](../../roadmap/hex-map-roadmap.md). Quello che segue è la *richiesta*; ciò che è stato
> effettivamente costruito sta nel canone e nel registro di esecuzione, e in caso di conflitto vincono loro.
>
> **Testo estratto dal `.docx` originario, non riscritto.**

## Da dove viene

`editor-griglia-esagonale-e-mappa.docx` (23 KB), rimosso il **2026-08-12** — ultimo binario di prosa in
`docs/`, convertito per la stessa regola dei ventitré PDF: PDR-00 §6 #5 /
[D-009](../../decisions/RT_PDR_00_Decision_Log.md).

Non è un PRD e non è una specifica: è il **prompt operativo** con cui è stato commissionato il pivot
esagonale — ruolo, regole di ingaggio, assunzioni, architettura richiesta, ventitré sezioni fino al
«principio finale». Il progetto lo ha eseguito quasi alla lettera.

## Cosa resta vero, cosa no

**Recepito, e verificabile nel codice.** Le assunzioni §2 sono diventate
[ADR-0002](../../decisions/adr-0002-griglia-esagonale.md) e poi tipi reali:

| Il prompt chiedeva | Oggi esiste |
|---|---|
| Esagoni **pointy-top**, coordinate assiali `X≡Q`, `Y≡R`, cubica derivata `Z = -X - Y`, `Layer` per i piani sovrapposti | `FRTCellId{X=q, Y=r, Layer}` — il substrato **unico**, il quadrato rimosso al CP 7.2 |
| Celle in dati compatti, **niente Actor per cella**, rendering via ISM/HISM | `URTHexMapAsset` (storage + hash stabile) · `ARTHexMapActor` (ISM) |
| Costi di movimento **interi**, A\* sul grafo tattico e **non** NavMesh | `URTHexPathLibrary` · invariante #4 del canone |
| Due moduli, runtime **indipendente** dall'Editor | `Source/RefactorTactics/` + `Source/RefactorTacticsEditor/` |
| Editor mappa con selezione, painting, Undo/Redo, validator | `URTHexEditorMode` con Select · Paint · Arch · Fill tool |

Anche le **regole operative** §1 e il **principio finale** §23 sono sopravvissuti al loro documento: *«non
inventare API Unreal»*, *«niente Blueprint per logica autorevole, coordinate, serializzazione, validazione o
pathfinding»*, *«milestone piccole e compilabili»* sono oggi guardrail permanenti in
[`CLAUDE.md`](../../../CLAUDE.md) e [`AGENTS.md`](../../../AGENTS.md).

**Superato.** La dimensione esagono *«default 100 cm»* — oggi la cella prende la sua dimensione **dall'asset
mappa** (canone §6). *«Unreal Engine 5 stabile configurata nel progetto»* — la patch è bloccata a **5.8.1**
([D-022](../../decisions/RT_PDR_00_Decision_Log.md)).

**⚠️ La roadmap §18 non è un piano, è un registro chiuso.** H0–H6.5 sono **consegnate**; H7 (networking),
H8 (ambienti), H9 (production) sono state **rimappate** su M10 · M9 · M11 di
[`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md), perché due tracker dello stesso stato ne
producono uno sbagliato. Lo stato per milestone sta in
[`hex-map-roadmap.md`](../../roadmap/hex-map-roadmap.md), che è `DELIVERED` e non si aggiorna. **Non aprire
issue da §18.**

**Recuperabile.** §21 descrive un formato di lavoro in cinque tempi — *audit iniziale → piano →
implementazione → verifica → report finale* — che nessun documento corrente possiede, e che è
riconoscibilmente l'antenato del preflight di `CLAUDE.md` §3. §13 (validazione) e §15 (Automation Test)
elencano controlli sulla mappa più fini di quelli implementati.

---

## PROMPT — IMPLEMENTAZIONE GRIGLIA ESAGONALE ED EDITOR MAPPA PER REFACTORTACTICS

### Ruolo

Agisci come Senior Unreal Engine Developer, Gameplay Systems Engineer ed Editor Tools Developer.

Stai lavorando su **RefactorTactics**, tattico competitivo PC-first in Unreal Engine 5 con turni simultanei, simulazione deterministica e mappe multilivello.

Devi implementare:

1. griglia tattica esagonale;
1. rappresentazione dati della mappa;
1. generazione graybox;
1. selezione e modifica delle celle;
1. primo editor di mappa;
1. serializzazione in asset;
1. validazione;
1. pathfinding A* locale;
1. strumenti di debug;
1. roadmap verso un editor completo production-ready.

Non limitarti a scrivere una proposta teorica. Devi analizzare il repository e produrre codice C++ compilabile, configurazione Editor, test e documentazione operativa.

## 1. Regole operative

Prima di modificare il progetto:

1. individua la versione Unreal Engine configurata;
1. analizza Source/, Content/, file .uproject, moduli e plugin;
1. identifica classi esistenti relative a griglia, mappa, pathfinding, unità e selezione;
1. non duplicare sistemi già presenti;
1. presenta una breve lista delle modifiche previste;
1. implementa il lavoro in milestone piccole e compilabili;
1. compila dopo ogni milestone significativa;
1. correggi tutti gli errori introdotti prima di procedere.

Non inventare API Unreal Engine.

Quando una API cambia tra versioni UE5, verifica la firma disponibile nella versione del progetto e segnala la differenza.

Non usare Blueprint per logica autorevole, coordinate, serializzazione, validazione o pathfinding.

Usa Blueprint solo per:

- materiali;
- mesh;
- configurazione visiva;
- widget;
- VFX;
- prototipazione della presentazione.

La soluzione deve funzionare anche in build packaged. Il modulo Editor non deve essere richiesto a runtime.

## 2. Assunzioni iniziali

Salvo incompatibilità rilevate nel repository, usa questi default:

- Unreal Engine 5 stabile configurata nel progetto;
- progetto C++;
- griglia esagonale **pointy-top**;
- coordinate assiali:
  - X equivalente a Q;
  - Y equivalente a R;
  - coordinata cubica derivata Z = -X - Y;
- Layer separa piani, tetti, ponti, tunnel e livelli sovrapposti;
- dimensione iniziale esagono configurabile, default 100 cm;
- una cella rappresenta una posizione tattica valida;
- celle logiche conservate in dati compatti;
- niente Actor per ogni cella;
- rendering iniziale tramite Instanced Static Mesh o Hierarchical Instanced Static Mesh;
- costi di movimento interi;
- identificatori e ordinamenti deterministici;
- A* sul grafo tattico, non NavMesh;
- NavMesh eventualmente usata solo come supporto visivo o per personaggi non tattici.

Non trasformare migliaia di celle in Actor o Component separati.

## 3. Architettura richiesta

Se il repository non contiene già una struttura equivalente, crea o adatta due moduli:

```text
Source/
├── RefactorTactics/
│   ├── RefactorTactics.Build.cs
│   ├── Public/
│   │   ├── Map/
│   │   ├── Pathfinding/
│   │   └── Debug/
│   └── Private/
│       ├── Map/
│       ├── Pathfinding/
│       └── Debug/
│
└── RefactorTacticsEditor/
    ├── RefactorTacticsEditor.Build.cs
    ├── Public/
    │   └── HexMapEditor/
    └── Private/
        └── HexMapEditor/
```

Il modulo runtime deve contenere:

- coordinate;
- dati delle celle;
- asset della mappa;
- lookup;
- conversioni coordinate/mondo;
- vicini;
- distanze;
- grafo;
- query;
- pathfinding;
- validazione runtime;
- debug runtime opzionale.

Il modulo Editor deve contenere:

- strumenti di creazione;
- selezione;
- painting;
- modifica proprietà;
- salvataggio asset;
- visualizzazione;
- validator Editor;
- eventuale modalità Editor personalizzata.

Nel .uproject, il modulo Editor deve essere di tipo Editor.

## 4. Strutture dati fondamentali

Implementa FRTCellId come USTRUCT(BlueprintType).

Campi minimi:

```text
int32 X;
int32 Y;
int32 Layer;
```

Funzioni richieste:

- costruttore di default;
- costruttore con coordinate;
- IsValid;
- operatori == e !=;
- ordinamento stabile;
- GetTypeHash;
- conversione in stringa;
- coordinata cubica derivata;
- distanza esagonale;
- accesso ai sei vicini;
- conversione axial → world;
- conversione world → axial con arrotondamento cubico corretto.

La distanza tra celle dello stesso layer deve essere basata sulle coordinate cubiche.

Le celle su layer diversi non devono essere considerate automaticamente adiacenti. Le transizioni verticali devono essere archi espliciti.

Crea un enum per le sei direzioni esagonali, con ordine stabile:

```text
0..5
```

L’ordine non deve dipendere da TMap, indirizzi di memoria o ordine casuale di iterazione.

## 5. Dati della cella

Crea una struttura compatta, ad esempio FRTHexCellData, con almeno:

- FRTCellId Id;
- quota o offset verticale;
- tipo superficie;
- costo movimento base intero;
- blocco movimento;
- blocco linea di vista;
- opacità;
- cover direzionale su sei lati;
- hazard;
- occupazione logica;
- Gameplay Tags;
- revisione;
- riferimento opzionale a interazione;
- flag Editor;
- lista di transizioni speciali.

Prevedi superfici iniziali:

- Normal;
- Water;
- Mud;
- Fire;
- Electrified;
- Ice;
- Void.

Non implementare ancora tutta la simulazione ambientale. Prepara i dati senza sovraestendere lo scope.

Per cover direzionale usa sei valori compatti, uno per lato.

La cover deve appartenere al bordo della cella o essere interpretata in modo esplicito e documentato. Evita ambiguità tra cover entrante e uscente.

## 6. Asset della mappa

Crea un UPrimaryDataAsset, ad esempio:

```text
URTHexMapAsset
```

Deve contenere almeno:

- ID stabile della mappa;
- versione formato;
- dimensione esagono;
- orientamento;
- lista o storage compatto delle celle;
- lista archi/transizioni esplicite;
- bounds logici;
- revision;
- metadata;
- hash o dati necessari per calcolare un hash deterministico;
- riferimenti soft agli asset visuali;
- eventuali chunk.

Non conservare come formato autorevole solo una TMap.

L’asset serializzato deve usare un ordine stabile delle celle, ordinato almeno per:

1. Layer;
1. X;

È possibile costruire lookup temporanei runtime per velocizzare l’accesso.

Implementa:

- BuildLookup;
- FindCell;
- ContainsCell;
- GetSortedCells;
- ValidateMap;
- invalidazione e ricostruzione cache;
- calcolo revisione/hash deterministico.

## 7. Rappresentazione nel livello

Crea un Actor visualizzatore, ad esempio:

```text
ARTHexMapActor
```

Responsabilità:

- riferimento a URTHexMapAsset;
- generazione delle istanze visive;
- trasformazione cella → world;
- evidenziazione selezione;
- debug dei layer;
- debug coordinate;
- refresh manuale;
- refresh Editor;
- nessuna autorità sui dati di simulazione.

Usa instancing.

Non creare un Actor per ogni esagono.

Prevedi almeno questi gruppi visuali:

- celle normali;
- celle bloccate;
- acqua;
- hazard;
- selezione;
- path debug;
- celle invalide.

Non hardcodare materiali obbligatori. Esponi riferimenti configurabili e usa fallback sicuri.

Gestisci correttamente:

- mesh esagonale con pivot centrale;
- scala dalla dimensione cella;
- offset di layer;
- aggiornamento selettivo o rebuild completo;
- cleanup delle istanze;
- mapping tra instance index e FRTCellId.

## 8. Generatore graybox

Implementa un primo generatore Editor capace di creare:

- rettangolo assiale;
- esagono di raggio N;
- anello;
- linea;
- area piena;
- cancellazione area;
- duplicazione su altro layer.

Parametri minimi:

- larghezza;
- altezza;
- raggio;
- layer;
- quota;
- superficie;
- costo;
- cella bloccata o valida.

Il generatore deve scrivere nel URTHexMapAsset, non solo creare geometria nel livello.

Ogni operazione deve:

1. modificare l’asset;
1. marcare il package dirty;
1. supportare Undo/Redo tramite transaction Editor;
1. rigenerare la visualizzazione;
1. mantenere ordinamento stabile;
1. eseguire validazione.

## 9. Editor mappa MVP

Implementa il percorso più semplice ma scalabile.

### Fase iniziale obbligatoria

Creare un editor funzionante direttamente nel Level Editor usando:

- ARTHexMapActor;
- pannello Details;
- comandi Editor;
- selezione tramite raycast;
- strumenti paint;
- Undo/Redo;
- salvataggio su URTHexMapAsset.

Funzioni MVP:

- Create Map Asset;
- Assign Map Asset;
- Generate Hex Area;
- Add Cell;
- Remove Cell;
- Select Cell;
- Select Multiple Cells;
- Paint Surface;
- Paint Movement Cost;
- Toggle Movement Block;
- Toggle LOS Block;
- Paint Height;
- Paint Layer;
- Paint Directional Cover;
- Add Vertical Transition;
- Remove Vertical Transition;
- Validate;
- Save;
- Rebuild Preview.

Input suggeriti:

- click: selezione;
- Shift + click: selezione multipla;
- Ctrl + click: toggle selezione;
- drag: paint;
- rotella o comando dedicato: brush radius;
- tasti numerici o UI: layer visibile;
- Escape: annulla strumento.

Non sovrascrivere shortcut standard dell’Editor senza controllo.

### Evoluzione successiva

Dopo l’MVP, predisponi una roadmap per una modalità Editor dedicata con:

- toolbar;
- palette;
- viewport overlay;
- brush;
- gizmo;
- filtri layer;
- pannello proprietà;
- pannello validazione;
- minimappa;
- strumenti di connessione.

Non iniziare da un editor standalone complesso se il repository non ha ancora una base funzionante.

## 10. Selezione della cella

Implementa la selezione senza dipendere da collisioni individuali per ogni cella.

Strategia preferita:

1. raycast su piano o mesh di rappresentazione;
1. conversione posizione world → axial;
1. arrotondamento cubico;
1. applicazione del layer attivo;
1. lookup nell’asset;
1. aggiornamento selezione.

Per mappe multilivello, prepara la selezione a distinguere:

- layer attivo;
- layer visibili;
- cella più vicina all’impatto;
- quota;
- eventuale geometria sovrapposta.

Per il primo MVP è accettabile selezionare esplicitamente un layer attivo.

## 11. Grafo tattico

Crea un servizio o struttura di grafo che esponga:

- vicini orizzontali;
- transizioni verticali;
- archi speciali;
- validità della transizione;
- costo fisico;
- costo superficie;
- costo verticale;
- blocchi;
- profilo unità.

Ogni cella valida può avere fino a sei vicini orizzontali, ma un arco esiste solo se:

- la cella destinazione esiste;
- il movimento non è bloccato;
- la differenza di quota è accettabile;
- il profilo unità può attraversare la superficie;
- eventuali porte o interazioni consentono il passaggio.

Non usare distanza world come unica regola di adiacenza.

Prevedi transizioni speciali:

- scale;
- rampe;
- ascensori;
- tunnel;
- ponti;
- salto;
- teletrasporto futuro.

## 12. Primo A*

Implementa un primo pathfinding locale autorevole.

Preferenza:

- FGraphAStar con adapter e query filter custom, se compatibile con la versione UE;
- altrimenti implementazione A* minimale e deterministica, ben isolata e sostituibile.

Requisiti:

- costi interi;
- euristica esagonale;
- budget movimento;
- celle bloccate;
- profilo unità;
- supporto layer;
- archi speciali;
- risultato stabile;
- limite massimo nodi;
- diagnostica;
- nessuna dipendenza dal frame rate.

Il risultato deve contenere:

- stato query;
- lista ordinata di FRTCellId;
- costo totale;
- numero nodi visitati;
- motivo del fallimento.

In caso di parità usa un tie-break deterministico basato sull’ID della cella.

Non affidarti all’ordine di iterazione di TSet o TMap.

## 13. Validazione

Implementa un validator richiamabile da codice e dall’Editor.

Errori minimi:

- ID duplicati;
- coordinate invalide;
- celle fuori bounds;
- costi negativi;
- superficie sconosciuta;
- Gameplay Tag non valido;
- transizione verso cella inesistente;
- transizione duplicata;
- transizione unidirezionale non dichiarata;
- cover fuori range;
- layer incoerente;
- quota non valida;
- cella isolata;
- asset visuale mancante;
- hash/revisione incoerenti.

Distingui:

- Error;
- Warning;
- Info.

Il validator non deve correggere automaticamente dati distruttivi senza comando esplicito.

## 14. Debug

Aggiungi strumenti debug attivabili:

- coordinate cella;
- ID;
- layer;
- quota;
- costo;
- superficie;
- cover;
- archi;
- vicini;
- path;
- celle visitate da A*;
- transizioni verticali;
- errori validator.

Usa dove opportuno:

- console variable;
- console command;
- debug draw;
- log category dedicata.

Crea una categoria di log, ad esempio:

```text
LogRTHexMap
```

Non stampare log per ogni cella a ogni frame.

## 15. Automation Test

Crea Automation Test C++ per almeno:

1. uguaglianza e hash di FRTCellId;
1. conversione axial → world → axial;
1. sei vicini univoci;
1. distanza esagonale;
1. ordinamento stabile;
1. generazione esagono di raggio N;
1. lookup;
1. rilevamento duplicati;
1. validazione transizione invalida;
1. path semplice;
1. path con ostacolo;
1. path tra layer con transizione;
1. fallimento senza transizione;
1. tie-break deterministico;
1. serializzazione e reload dell’asset, se praticabile nel test environment.

Aggiungi anche un test visibile nella mappa L_DevSandbox:

- griglia hex graybox;
- almeno due layer;
- ostacolo;
- acqua;
- cover;
- ponte o scala;
- percorso visualizzato tra due celle.

## 16. Setup Editor

Documenta esattamente:

1. come creare un URTHexMapAsset;
1. come inserire ARTHexMapActor;
1. come assegnare l’asset;
1. come impostare mesh e materiali;
1. come generare la prima area;
1. come scegliere il layer;
1. come selezionare celle;
1. come dipingere superficie e costo;
1. come aggiungere cover;
1. come creare una transizione;
1. come lanciare il validator;
1. come visualizzare un path;
1. come salvare;
1. come lanciare gli Automation Test.

## 17. Build.cs e dipendenze

Mantieni minime le dipendenze.

Runtime, solo se necessarie:

```text
Core
CoreUObject
Engine
GameplayTags
DeveloperSettings
AIModule
```

Non aggiungere AIModule se il pathfinding implementato non lo richiede.

Modulo Editor, solo se necessarie e disponibili:

```text
UnrealEd
LevelEditor
Slate
SlateCore
EditorSubsystem
PropertyEditor
ToolMenus
InputCore
Projects
AssetTools
AssetRegistry
EditorFramework
InteractiveToolsFramework
```

Verifica i nomi effettivi dei moduli nella versione UE del progetto.

Non spostare dipendenze Editor nel modulo runtime.

## 18. Roadmap richiesta

Produci e mantieni una roadmap in un file Markdown, ad esempio:

```text
Docs/HexMapEditorRoadmap.md
```

### Milestone H0 — Analisi e fondazioni

- verifica versione UE;
- audit repository;
- moduli runtime/editor;
- FRTCellId;
- conversioni;
- test coordinate.

#### Done quando

- progetto compila;
- test coordinate passano;
- nessuna dipendenza Editor nel runtime.

### Milestone H1 — Asset e rendering graybox

- FRTHexCellData;
- URTHexMapAsset;
- lookup;
- ordinamento;
- ARTHexMapActor;
- instanced rendering;
- generatore base.

#### Done quando

- è possibile creare e salvare una griglia;
- reload dell’Editor mantiene i dati;
- nessun Actor per cella.

### Milestone H2 — Editor MVP

- selezione;
- multi-selezione;
- paint superficie;
- paint costo;
- blocchi;
- altezza;
- layer;
- Undo/Redo;
- validazione.

#### Done quando

- una mappa graybox può essere creata senza modificare codice;
- tutte le operazioni sono annullabili;
- l’asset viene marcato dirty correttamente.

### Milestone H3 — Grafo e A*

- vicini;
- archi;
- profilo unità;
- A*;
- debug path;
- Automation Test.

#### Done quando

- path deterministico;
- ostacoli e costi funzionano;
- layer diversi richiedono transizione esplicita.

### Milestone H4 — Mappa multilivello

- filtri layer;
- bridge;
- tunnel;
- scale;
- ascensori;
- celle sovrapposte;
- selezione multilivello.

#### Done quando

- due celle con stessi X/Y e Layer diversi sono gestite correttamente;
- visualizzazione e pathfinding non confondono i livelli.

### Milestone H5 — Editor Mode dedicato

- toolbar;
- brush avanzati;
- palette;
- overlay;
- pannello validator;
- strumenti archi;
- copia/incolla;
- fill;
- shape tools.

#### Done quando

- il workflow non dipende più principalmente dal pannello Details;
- designer può creare una mappa completa in modo rapido.

### Milestone H6 — Integrazione simulatore

- snapshot mappa;
- occupazione;
- planning;
- movement budget;
- collisioni simultanee;
- TurnLog;
- replay.

#### Done quando

- stesso asset, snapshot, seed e intenti producono lo stesso risultato;
- animazioni non influenzano il movimento.

### Milestone H7 — Networking e privacy

- asset hash;
- validazione server;
- path proposto dal client;
- path ricalcolato o validato dal server;
- planning team-only;
- zero intenti inviati agli avversari.

#### Done quando

- dedicated server valida ogni percorso;
- client avversario non riceve celle/path pianificati dal team nemico.

### Milestone H8 — Ambienti tattici

- acqua;
- fuoco;
- elettricità;
- cover dinamica;
- porte;
- ponti;
- hazard;
- revisioni chunk.

#### Done quando

- modifiche ambientali invalidano correttamente cache e path;
- TurnLog registra ogni modifica.

### Milestone H9 — Production readiness

- chunk;
- performance;
- mappe grandi;
- commandlet validator;
- cook;
- packaged build;
- test replay;
- profiling.

#### Done quando

- path query mediana sotto 2 ms sul target MVP;
- preview completa sotto 50 ms;
- zero divergenze replay;
- validator eseguibile in CI;
- packaged build verificata.

## 19. Limiti di scope iniziali

Non implementare nella prima iterazione:

- modding pubblico;
- procedural generation complessa;
- multiplayer completo;
- GAS;
- simulazione ambientale completa;
- editor standalone;
- spline decorative;
- landscape deformation;
- runtime mesh complessa;
- salvataggi utente;
- matchmaking;
- progressione.

Prepara punti di estensione, ma completa prima H0, H1 e una parte funzionale di H2.

## 20. Criteri di accettazione immediati

La prima consegna è accettata solo se:

- il progetto compila;
- è presente FRTCellId;
- la griglia usa coordinate assiali;
- gli esagoni hanno sei vicini;
- esiste conversione world/axial corretta;
- esiste un asset mappa persistente;
- esiste un Actor visualizzatore instanced;
- è possibile generare una griglia graybox;
- è possibile selezionare una cella;
- è possibile aggiungere e rimuovere celle;
- è possibile modificare almeno superficie, costo e blocco;
- Undo/Redo funziona;
- l’asset resta valido dopo riapertura Editor;
- il validator trova errori reali;
- esiste almeno un Automation Test;
- L_DevSandbox mostra il risultato;
- il modulo runtime non dipende da moduli Editor.

## 21. Formato della risposta e del lavoro

Procedi in questo ordine.

### A. Audit iniziale

Mostra:

- versione UE;
- struttura repository;
- sistemi esistenti rilevanti;
- rischi;
- assunzioni;
- file da creare o modificare.

### B. Piano di implementazione

Dividi il lavoro in commit piccoli.

Per ogni commit indica:

- scopo;
- file;
- dipendenze;
- test;
- criterio Done.

### C. Implementazione

Scrivi codice completo, non pseudocodice.

Per ogni classe indica:

- header;
- source;
- responsabilità;
- dipendenze;
- setup Editor.

### D. Verifica

Esegui:

- compilazione;
- test;
- verifica log;
- verifica reload asset;
- verifica Undo/Redo;
- verifica mappa L_DevSandbox.

### E. Report finale

Riporta:

- cosa è stato implementato;
- cosa non è stato implementato;
- file modificati;
- test eseguiti;
- errori rimasti;
- performance osservate;
- roadmap aggiornata;
- prossimo passo consigliato.

## 22. Convenzioni Git

Proponi commit separati simili a:

```text
feat(hex-grid): add axial cell coordinates and conversion tests
feat(hex-map): add persistent map asset and deterministic lookup
feat(hex-render): add instanced graybox map actor
feat(hex-editor): add cell selection and painting tools
feat(hex-path): add deterministic local A-star
test(hex-map): add validation and pathfinding automation tests
docs(hex-map): add editor setup and implementation roadmap
```

Non includere file generati, Binaries, Intermediate, Saved o cache IDE.

## 23. Principio finale

La mappa deve essere trattata come un grafo tattico esagonale multilivello, non come una decorazione visiva.

I dati autorevoli devono restare indipendenti da:

- Actor;
- mesh;
- animazioni;
- frame rate;
- ordine casuale delle collection;
- timing dell’Editor;
- NavMesh.

La prima implementazione deve essere semplice, verificabile e realmente utilizzabile, ma non deve creare debito tecnico che impedisca:

- multiplayer autorevole;
- replay deterministico;
- mappe multilivello;
- editor avanzato;
- validazione server;
- contenuti data-driven.
