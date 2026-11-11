# RefactorTactics — Level Designer / Map Editor
## Handoff di contesto per Claude

**Data di consolidamento:** 2026-08-12  
**Repository verificato:** `DegrassiAaron/refactor-tactics-main`  
**Branch di riferimento:** `main`  
**HEAD verificato durante l’audit:** `52c082860548adf59151dd529e05af7d0406c916`  
**Baseline Unreal corrente da verificare comunque nel repository prima di modificare codice:** UE 5.8.x, con riferimenti recenti a UE 5.8.1.

**Owner del documento:** l’autore del focus Level Designer.  
**Condizione di scadenza:** questo handoff è obsoleto quando **#620, #621, #622, #623 e #554** sono tutte chiuse.
A quel punto va archiviato o rigenerato, non aggiornato riga per riga.

> **Revisione spec panel del 2026-08-12** (Wiegers · Adzic · Cockburn · Fowler · Nygard · Newman · Crispin).
> Le correzioni del panel sono marcate `🔎 PANEL` nel corpo. Verifica rifatta contro HEAD `402c154b`:
> le issue #554/#620/#621/#622/#623 risultano ancora **OPEN** e #588/#619 **CLOSED**, quindi la catena
> di §6 regge. È salito solo l’HEAD — lo sha qui sopra è lo snapshot dell’audit, non lo stato corrente.
> ⚠️ Esiste un worktree attivo su `feat/554-transizioni-visibili`: **prima di aprire #554, controlla quel branch.**

> Questo file è un handoff di **contesto e decisioni**.  
> Non va trattato come una nuova roadmap autonoma e non deve creare un secondo sistema parallelo.
> Prima di applicare qualsiasi punto, Claude deve controllare HEAD, issue, roadmap, Feature Registry,
> editor sessions, scenari e codice effettivo.

---

# 1. Perché esiste questo handoff

È stato fatto un focus sul **Level Designer di RefactorTactics**.

La prima ipotesi era progettare un vero “RT Map Designer” con:

- authoring delle celle hex;
- superfici;
- quota/layer;
- cover e archi;
- porte, ponti, tunnel, ascensori;
- spawn e objective;
- overlay di movement, LOS, cover, facing;
- analisi Overwatch;
- propagazione del rumore;
- interazioni ambientali;
- validazione;
- scenario runner;
- heatmap;
- analisi del bilanciamento spaziale;
- supporto futuro alla generazione procedurale.

Dopo questa proposta è stato fatto un **audit reale del repository**.

Risultato importante:

> Una quantità significativa di questa infrastruttura ESISTE GIÀ.

Quindi il lavoro corretto non è:

> “costruire un Level Designer da zero”

ma:

> **trasformare gli strumenti spaziali già presenti in una workstation completa, leggibile e produttiva per il Level Designer.**

Questo file conserva entrambe le cose:

1. cosa è già implementato;
2. quale visione completa del Level Designer vogliamo raggiungere senza duplicare sistemi.

---

# 2. Principio fondamentale: Logical Map ≠ Visual Map

Decisione concettuale da mantenere.

```text
                     MAP AUTHORING
                          │
                ┌─────────┴─────────┐
                ▼                   ▼
           LOGICAL MAP          VISUAL MAP
                │                   │
           FRTCellId              Mesh
           Graph edges            Props
           Layers                 Decals
           Surface                Materials
           Cover                  Lighting
           Hazards                VFX
           Interactions           Audio
           Occupancy
                │                   │
                └─────────┬─────────┘
                          ▼
                   PLAYABLE LEVEL
```

## Regola

La **Logical Map** decide il gameplay.

La **Visual Map** spiega e rappresenta il gameplay.

La geometria visiva non deve diventare una seconda fonte di verità runtime.

Il progetto ha già adottato un modello coerente:

- celle compatte;
- grafo hex multilivello;
- archi come dati;
- cover e porte sui bordi;
- pathfinding separato dalla visualizzazione;
- LOS separata dal pathfinding;
- runtime autorevole indipendente da mesh e animazioni;
- editor che modifica dati e genera visualizzazione transient.

Questa separazione non va indebolita per rendere più comodo l’authoring.

---

# 3. Che cos’è il Level Designer in RefactorTactics

Il Level Designer non è semplicemente chi “piazza muri e props”.

La mappa è uno dei sistemi strategici principali del gioco.

Ownership ideale:

```text
LEVEL DESIGNER
│
├─ Layout tattico
│  ├─ celle hex
│  ├─ layer / quota
│  ├─ percorsi
│  ├─ choke
│  ├─ loop routes
│  ├─ spazi aperti
│  └─ rotte alternative
│
├─ Architettura
│  ├─ muri
│  ├─ low/high cover
│  ├─ porte
│  ├─ ponti
│  ├─ tunnel
│  ├─ rampe / scale
│  └─ ascensori
│
├─ Gameplay Environment
│  ├─ acqua
│  ├─ ghiaccio
│  ├─ fuoco
│  ├─ elettricità
│  ├─ vapore
│  ├─ rumore
│  └─ elementi interattivi/distruttibili
│
├─ Combat Space
│  ├─ LOS
│  ├─ facing
│  ├─ firing lanes
│  ├─ Overwatch lanes
│  ├─ flank routes
│  ├─ displacement opportunities
│  └─ linee di pressione sull’obiettivo
│
├─ Match Flow
│  ├─ spawn
│  ├─ objective
│  ├─ tempo al primo contatto
│  ├─ rotazioni
│  ├─ zone di controllo
│  └─ comeback routes
│
└─ Validation
   ├─ connectivity
   ├─ path
   ├─ LOS
   ├─ cover
   ├─ fairness
   ├─ scenari
   ├─ leggibilità
   └─ performance
```

Il designer deve poter rispondere a domande come:

> “Da Spawn A al Relay esistono quattro classi di rotta: due esposte, una protetta ma lenta, una che attraversa acqua e quindi crea rischio elettrico.”

Non basta:

> “Questo corridoio sembra bello.”

---

# 4. Stato reale dell’Hex Map Editor

Audit sul codice reale.

Esiste un Editor Mode:

`Source/RefactorTacticsEditor/Private/RTHexEditorMode.cpp`

Registra quattro tool:

```cpp
RegisterTool(... "RTHexSelectTool" ...)
RegisterTool(... "RTHexPaintTool" ...)
RegisterTool(... "RTHexArchTool" ...)
RegisterTool(... "RTHexFillTool" ...)
```

Esistono quindi realmente:

- Select;
- Paint;
- Arch;
- Fill.

I comandi sono in:

`Source/RefactorTacticsEditor/Private/RTHexEditorModeCommands.cpp`

Semantica corrente:

- **Select**: seleziona una cella nel viewport sul layer attivo;
- **Paint**: dipinge o cancella una cella;
- **Arch**: crea transizioni tra celle usando il layer attivo;
- **Fill**: flood fill della regione contigua della stessa superficie.

Il toolkit usa il nome:

**Hex Map**

Quindi NON creare un nuovo editor separato se il problema può essere risolto estendendo questo mode.

---

# 5. Matrice IMPLEMENTED / PARTIAL / MISSING

Questa matrice sostituisce l’ipotesi precedente “tutto da fare”.

> 🔎 **PANEL — questa matrice NON è la fonte di verità dello stato.**
>
> Lo stato per gate vive in `docs/roadmap/featuremap.shortlist.md`, che è **generata** da
> `docs/roadmap/feature-registry.yaml` e protetta da `python scripts/feature_registry.py validate`.
> Alla data dell’audit diceva:
>
> - `RT-FEAT-TOOL-MAP-EDITOR` → **TESTABLE**, 4/6
> - `RT-FEAT-TOOL-MAP-GEOMETRY` → **IMPLEMENTING**, 3/7 (`data: done`, `automation: done`,
>   `scenario: done`, `runtime: partial`, `spec: partial`, `log_debug: todo`, `ui_wiki: todo`)
>
> Cioè: #620/#621 **non sono un blocco vuoto**, come invece lascia intendere la §5.3 qui sotto.
> Dove i due divergono, **vince la shortlist generata**, non questa tabella.
>
> **Vocabolario** — in questa sezione `IMPLEMENTED` significa *il codice esiste ed è raggiungibile
> da HEAD*. NON significa «testato», NON significa «usabile in produzione»: è per questo che §5.2
> può dire `Production usability: non ancora completa` senza contraddire le trenta righe di §5.1.
>
> Le tabelle restano come **referto dell’audit** — cosa il panel ha trovato e perché ha concluso
> «non costruire da zero» — non come cruscotto da consultare. Nessuna riga porta un’evidenza
> verificabile: per sapere se una riga è ancora vera oggi servono la shortlist o il simbolo.

## 5.1 Implementato / già disponibile

| Capability | Stato |
|---|---|
| `FRTCellId` con X/Y assiali + Layer | IMPLEMENTED |
| Hex-only gameplay substrate | IMPLEMENTED |
| Grafo tattico multilivello | IMPLEMENTED |
| A* autorevole | IMPLEMENTED |
| Camera tattica/runtime | IMPLEMENTED |
| Hover / selezione celle | IMPLEMENTED |
| Hex Editor Mode | IMPLEMENTED |
| Select tool | IMPLEMENTED |
| Paint / erase | IMPLEMENTED |
| Flood Fill | IMPLEMENTED |
| Arch / transition authoring | IMPLEMENTED |
| Layer attivo | IMPLEMENTED |
| `ERTLayerViewMode` | IMPLEMENTED |
| `Focus` + `GhostLayerRange` | IMPLEMENTED |
| Click corretto rispetto al layer attivo | IMPLEMENTED |
| Raycast irrobustito component-safe | IMPLEMENTED |
| Surface visualization | IMPLEMENTED |
| Move-cost relief | IMPLEMENTED |
| Blocker visualization | IMPLEMENTED |
| LOS-blocker visualization | IMPLEMENTED |
| Low/High Cover runtime | IMPLEMENTED |
| Low/High Cover edge visualization | IMPLEMENTED |
| Porte runtime | IMPLEMENTED |
| Door state visualization | IMPLEMENTED |
| Ponti / `ModifyArc` | IMPLEMENTED |
| Transition visualization nel tool Arch | IMPLEMENTED |
| Facing logico | IMPLEMENTED |
| LOS core | IMPLEMENTED |
| Scenario Harness | IMPLEMENTED |
| TurnLog | IMPLEMENTED |
| Map validation foundation | IMPLEMENTED |
| Occupancy a 12 settori | IMPLEMENTED |
| `CoreBlocked` | IMPLEMENTED |
| `Free / Constrained / Blocked` | IMPLEMENTED |
| `Constrained` con consumer reale sul costo | IMPLEMENTED |

---

## 5.2 Parziale

| Capability | Stato / residuo |
|---|---|
| Multilayer authoring UX | Base presente; migliorabile |
| Transizioni leggibili in ogni tool | Oggi soprattutto nel tool Arch |
| Reachability diagnostic | Da chiudere con #554 |
| Workspace prima di creare celle | Mancano celle ghost fuori dalla mappa esistente |
| Wall / LowWall authoring | Fondazioni esistono, grammatica/bake ancora da chiudere |
| Solid/void footprint authoring | Fondazioni occupancy presenti; workflow non completo |
| Level Designer workflow end-to-end | Parziale |
| Verifica visuale di alcune feature editor | Richiede sedute PIE/editor reali |
| Production usability | Non ancora completa |

---

## 5.3 Mancante / prossimo lavoro reale

| Capability | Owner corrente |
|---|---|
| Quantized geometry grammar | #620 |
| Junction grammar | #620 |
| Geometry validator | #620 |
| Geometry → `FRTHexCover` bake | #621 |
| Geometry → `bBlocksMovement` bake | #621 |
| Void/cliff → superficie canonica | #621 |
| Affected-region bake | #621 |
| Workspace ghost grid | #622 |
| Frame whole map / Home | #623 |
| Lighting del DevSandbox | #623, seduta Unreal |
| Reachability count | #554 |
| Unreachable cell visualization | #554 |
| Transition visualization fuori Arch | #554 |

> 🔎 **PANEL — la colonna “Owner corrente” porta numeri di issue, che questo stesso documento
> dichiara inaffidabili** (§0 del file 02: «non fidarti dello stato descritto»). L’owner stabile è
> il **feature ID** — `RT-FEAT-TOOL-MAP-GEOMETRY` per #620/#621, `RT-FEAT-TOOL-MAP-EDITOR` per
> #622/#623/#554 — perché sopravvive alla chiusura della issue. Usa il numero per *trovare* il lavoro,
> il feature ID per *attribuirlo*.
>
> ⚠️ **`RT-FEAT-TOOL-MAP-GEOMETRY` ha `spec: partial` per una ragione precisa**, non per incompletezza
> generica: la decisione aperta **`MSE-1`** (vedi §11 del file 02). Non si chiude quel gate senza chiuderla.

---

# 6. Issue corrente: catena reale

Lo stato verificato il 2026-08-12 è:

- **#588 CLOSED**
- **#619 CLOSED**
- **#620 OPEN**
- **#621 OPEN**
- **#622 OPEN**
- **#623 OPEN**
- **#554 OPEN**

La dipendenza più importante è:

```text
DONE #619
Occupancy 12 sectors
      │
      ▼
NEXT #620
Quantized Geometry Grammar + Validator
      │
      ▼
#621
Geometry → Canonical Map Bake
      │
      ├──────────────┐
      ▼              ▼
#622              #623
Workspace Grid     Lighting / Frame Map
      │              │
      └──────┬───────┘
             ▼
      MAP SKETCH USABLE
```

In parallelo:

```text
#554
Reachability + transition diagnostics
```

## Nota di drift documentale

Il corpo di alcune issue create prima della merge di #588 contiene ancora testo del tipo:

> “#588 è aperta / PR #598 non mergiata”

Questa informazione è ora stale.

#588 risulta chiusa e il repository usa il discriminante per componente.

Quando Claude tocca #620/#622 deve correggere questa deriva documentale dove necessario.

---

# 7. #619 — cosa NON va rifatto

#619 è stata chiusa.

Ha introdotto/chiuso il problema di occupancy geometrica:

- 12-bit sector mask;
- `CoreBlocked`;
- classificazione:
  - `Free`
  - `Constrained`
  - `Blocked`;
- soglie runtime verificabili;
- `Constrained` con effetto osservabile;
- sovrapprezzo di movimento separato da `MoveCost`;
- consumer del sovrapprezzo nei lettori del costo;
- implicazioni su formato/hash/scenario.

Decisione importante:

> Il sovrapprezzo di occupancy NON deve essere scritto in `MoveCost`, perché le superfici dinamiche possono ricalcolare `MoveCost`.

Quindi se un nuovo codice di geometry authoring tenta di “risolvere” il costo scrivendo direttamente `MoveCost`, sta duplicando/rompendo #619.

---

# 8. #620 — prossimo passo tecnico

Titolo:

**Geometria quantizzata: la grammatica delle direttrici, e il validator che la rende una regola**

Scopo:

La geometria architettonica non deve portare coordinate arbitrarie float dentro i dati competitivi/hashabili.

La grammatica deve essere discreta e deterministica.

Sono ammesse:

1. direttrici principali derivate dall’esagono;
2. ortogonali a tali direttrici;
3. segmenti sui lati/perimetro;
4. junction compatibili.

Principio:

> La direzione si CHIEDE alla libreria. Non si incidono angoli a mano.

Il validator è l’invariante, non un optional.

Deve rifiutare almeno:

- off-axis tactical geometry;
- ~~junction invalida~~ → **rimossa, vedi sotto**;
- segmento di lunghezza zero;
- segmento duplicato;
- layer invalido;
- geometria fuori dai bordi editabili.

> 🔎 **PANEL — DECISA il 2026-08-12: «junction» non è un concetto di #620.**
>
> La regola è stata tolta dalla lista, e i due test che la nominavano —
> `GeometryGrammar.ValidJunction` e `GeometryGrammar.RejectsInvalidJunction` — **vanno cancellati dalla
> suite di §8 del file 02, non implementati.**
>
> **Perché.** Allo strato che consuma la geometria, la junction non esiste già oggi. `ComputeMask`
> itera i lati e accende i settori con un **OR** (`RTHexOccupancyLibrary.cpp:143-152`): l’OR è
> idempotente e commutativo — è precisamente il meccanismo che rende la maschera indipendente
> dall’ordine, pinnato da `MaskIsIndependentOfInputOrder`. Due segmenti che si incontrano in un punto
> producono la stessa maschera che produrrebbero separati.
>
> La dimostrazione è già in suite e passa: la **Fixture 2** si chiama `Corner()` ed *è* una junction —
> tre punti, due segmenti che condividono `PointAt(10.0, 0.5)` — coperta da
> `CornerSpansEverySectorItCrosses`. Una junction è testata da prima che qualcuno la chiamasse così.
>
> **La rappresentazione che chiude la questione: polilinea.**
>
> | Rappresentazione | Continuità | Test necessario |
> |---|---|---|
> | **Polilinea** — lista ordinata di punti, come `FRTOccupancyPolyline` | **strutturale**: due segmenti consecutivi condividono l’estremo per costruzione | nessuno |
> | Insieme di segmenti indipendenti | da validare: servirebbe una regola sui gap | sì |
>
> Scegliendo la polilinea, la continuità diventa invariante **di tipo** invece che di validator, e
> combacia con la struttura che `ComputeMask` già consuma. Un concetto in meno e un test in meno.
>
> **Dove la junction torna a esistere: #621, non #620.** Quando due muri si incontrano in un angolo,
> quel punto produce cover su un bordo hex, su due o su nessuno. È una domanda della **cottura** —
> «come un angolo si proietta sui bordi canonici» — e va posta lì.
>
> Le fixture stanno in `Source/RefactorTactics/Tests/RTOccupancyFixtures.h`, che il Feature Registry
> destina esplicitamente anche a #620 e #621. Non creare un secondo set.
>
> ⚠️ Prima di scrivere la grammatica, leggi il blocco **D0-bis** in §16: gli assi che questa sezione
> impone collidono con i confini dei dodici settori di #619, e la collisione ha un costo misurabile.

La grammatica deve essere espressa in:

- interi;
- enum;
- ID discreti;

non in estremi float serializzati.

Serve una singola funzione canonica per centro/orientamento di bordo.

NON creare due implementazioni tra #553 e #620.

> 🔎 **PANEL — quella funzione esiste già. Non cercarla: riusala.**
>
> Verificato su HEAD `402c154b` con `git grep`:
>
> | Cosa | Dove |
> |---|---|
> | Centro del bordo | `URTHexLibrary::EdgeMidpointWorld` — `RTHexLibrary.h:90`, `.cpp:182` |
> | Orientamento del bordo | `URTHexLibrary::EdgeRotation` — `RTHexLibrary.h:100`, `.cpp:191` |
> | Consumer già vivo | `RTHexMapActor.cpp:558, 569` |
> | Invariante pinnata da test | `RefactorTactics.Hex.EdgeMidpointIsSharedByBothCells` — `RTHexTests.cpp:384` |
>
> L’invariante che quel test protegge è esattamente quella che serve a #620: *lo stesso bordo fisico ha
> lo stesso centro visto dalle DUE celle che lo condividono*. È ciò che impedisce a una copertura di
> esistere «due volte con due centri». Costruisci la grammatica **sopra** queste due, non accanto.

---

# 9. #621 — bake della geometria nei dati canonici

Titolo:

**Cottura: la geometria disegnata diventa FRTHexCover e bBlocksMovement, poi è arte**

Principio fondamentale:

```text
Authoring Geometry
       │
       ▼
deterministic bake
       │
       ▼
Canonical Runtime Spatial Data
       │
       ▼
runtime gameplay
```

Dopo il bake:

> la geometria di authoring è arte, non autorità runtime.

Destinazioni canoniche:

- `LOW WALL` → `FRTHexCover{Low}`
- `WALL` → `FRTHexCover{High}`
- footprint solido → `bBlocksMovement` secondo classificazione
- footprint void/cliff → `bBlocksMovement` **senza** `bBlocksLineOfSight`

> 🔎 **PANEL — DECISA il 2026-08-12: il bake NON scrive `Surface`.**
>
> La riga qui sopra diceva `footprint void/cliff → ERTHexSurface::Void`. **È stata corretta**, perché
> avrebbe creato un terzo campo a produttore condiviso che nessuno aveva contato.
>
> | Campo | Produttori dopo #621 | In `MSE-1`? |
> |---|---|---|
> | `FRTHexCover` | bake + authoring bordi | ✅ sì |
> | `bBlocksMovement` | bake + Paint | ✅ sì |
> | `ERTHexSurface` | ~~bake~~ + Paint/Fill soltanto | — **non più conteso** |
>
> Tre ragioni, in ordine di forza:
>
> 1. **`Void` è una superficie dipinta**, non un prodotto geometrico: sta in `ERTHexSurface` accanto a
>    `Floor`, `ShallowWater`, `Rough`, `Fire`, `Conductive`, `Ice`, `Smoke`, `HighGround`. Sono tutte
>    cose che il pennello sceglie, e nessuna regola geometrica sa scegliere fra nove.
> 2. **`Fill` propaga sulla contiguità di superficie.** Una `Surface` cotta non cambierebbe una cella:
>    cambierebbe il confine di **ogni futuro flood fill** che la attraversa. È un effetto sullo
>    *strumento*, non sul dato — categoria peggiore di `MSE-1`, non uguale.
> 3. **Il precipizio è già esprimibile senza toccare `Surface`.** La coppia
>    `bBlocksMovement = true` + `bBlocksLineOfSight = false` dice «non ci si sta sopra, ma ci si vede
>    attraverso»: è esattamente un baratro, e lo distingue da un muro. Entrambi i campi sono già di
>    #621 per `D1`.
>
> Applicata la forma di `D2` nella sua versione più economica: non separare i produttori — **non creare
> il secondo produttore.** `MSE-1` resta correttamente a due campi.
>
> ⚠️ **Collisione di terminologia da non subire.** `VoidFootprint()` in `RTOccupancyFixtures.h:92`
> significa «contorno chiuso che **non contiene il centro**, quindi lascia `CoreBlocked` falso» — è il
> gemello di controllo del solido. **Non** significa `ERTHexSurface::Void`. Due sensi della stessa
> parola in due file che verranno letti insieme.

Non creare:

- `Walls[]` runtime parallelo;
- un secondo modello cover;
- geometria autorevole nel `.umap`.

Il bake deve essere funzione pura nel modulo runtime e richiamata dall’editor.

---

# 10. #622 — workspace grid

Problema reale osservato:

Il designer vede bene le celle esistenti, ma non vede dove potrebbe disegnare una cella nuova.

Quindi sul bordo della mappa “disegna al buio”.

Serve:

```text
Workspace grid   → thin / ghost
Real cell        → normal rendering
Hovered cell     → highlighted
```

Vincoli:

- transient;
- data-derived;
- non salvata nel `.umap`;
- non un Actor per esagono;
- non usare `DemoRadius` come falsa sorgente di celle;
- deve essere VISIVAMENTE diversa da una cella reale.

Una vista che mente è peggio di una vista mancante.

> 🔎 **PANEL — «visivamente diversa» non è un criterio: è un aggettivo.**
>
> Quattro dei sei vincoli qui sopra sono verificabili da codice (transient, data-derived, non nel `.umap`,
> nessun Actor-per-esagono). Gli ultimi due — *deve essere VISIVAMENTE diversa*, *chiaramente ghost* nel
> file 02 §15 — non sono falsificabili, e una seduta senza condizione di pass produce un ✅ che significa
> «l’ho guardato».
>
> **Condizione di pass proposta**, da registrare in `editor-sessions.yaml` come oracolo della seduta:
>
> > Un tester **che non ha scritto il codice**, davanti al viewport e senza selezionare nulla, classifica
> > correttamente ghost/reale su **5 posizioni campione** — di cui almeno 2 sul bordo della mappa e 1
> > adiacente a una cella reale — in meno di 2 secondi ciascuna. Un solo errore = seduta rossa.
>
> Il campione va scelto **prima** di guardare, non dopo. E va scritto qual era, altrimenti la seduta
> successiva non è confrontabile con questa.

---

# 11. #623 — usability della seduta

Due problemi osservati:

1. `L_DevSandbox` troppo scuro per valutare la geometria;
2. non c’è un comando comodo per inquadrare l’intera mappa.

Non riscrivere il viewport Unreal.

Unreal fornisce già:

- pan;
- orbit;
- wheel zoom;
- WASD;
- focus;
- speed modifiers.

Residuo reale:

- `Home` / comando equivalente → frame whole editable map;
- valutare snap rotazione 30° solo se serve;
- sistemare lighting in editor:
  - Directional Light
  - Sky Light
  - Sky Atmosphere
  - exposure prevedibile.

> 🔎 **PANEL — «no zone quasi nere» e «exposure prevedibile» vanno resi osservabili.**
>
> Il lighting è l’unica di queste cinque issue il cui esito non può essere verificato da un test, quindi
> è l’unica dove la condizione di pass va scritta **prima** della seduta. Proposta:
>
> | Criterio | Oracolo |
> |---|---|
> | Nessuna zona illeggibile | Su una mappa multilayer di prova, **nessuna cella** in cui superficie e presenza di cover siano indistinguibili a occhio dalla camera di default |
> | Exposure prevedibile | Ruotando la camera di 360° la scena **non cambia esposizione a scatti**: nessun auto-exposure visibile |
> | Grid leggibile | I bordi cella si distinguono anche sulla superficie più scura del catalogo |
> | Non-regressione | **Due screenshot di riferimento versionati** (prima/dopo), citati nella seduta |
>
> Gli screenshot sono la parte che conta: senza un riferimento in-repo, la seduta successiva non ha modo
> di sapere se il lighting è peggiorato. Verificare l’allowlist prima di committarli (§28 del file 02).

`RTCameraPawn` è camera di gioco e NON va usata per risolvere la camera di authoring.

La seduta deve aggiornare:

- `test-manuali-pie.md`;
- `editor-sessions.yaml`;

e rigenerare:

- `editormap.shortlist.md`

tramite lo script previsto.

NON modificare la shortlist generata a mano.

---

# 12. #554 — reachability e transizioni

La vista `Focus` multilayer è già fatta.

Quello che manca è capire a colpo d’occhio se una zona è collegata.

Le transizioni sono l’unico modo in cui layer distinti si collegano.

Due celle sovrapposte NON sono adiacenti automaticamente.

#554 richiede due cose:

## 12.1 Misura headless

Contare le celle irraggiungibili dagli spawn.

Esempi:

- piattaforma con transizione → 0 unreachable;
- rimuovere unica transizione → tutte le celle piattaforma unreachable;
- layer senza entrate → unreachable;
- risultato indipendente dall’ordine TMap/TSet.

## 12.2 Vista

Fuori dal tool Arch deve essere possibile:

- vedere le transizioni;
- capire il verso;
- distinguere zone raggiungibili da isole scollegate.

Il test dice **quante**.

L’editor dice **quali**.

Servono entrambi.

---

# 13. Visione futura: RT Map Designer come workstation, non secondo editor

Il nome “RT Map Designer” può essere usato come **concetto UX**, non necessariamente come nuovo plugin/mode.

La soluzione preferita è evolvere l’Hex Map Mode esistente.

UI concettuale:

```text
┌──────────────────────────────────────────────────────────────┐
│ RT MAP DESIGNER                    Map: Industrial_Relay_01  │
├───────────────┬──────────────────────────────────────────────┤
│ TOOLS         │                                              │
│ Select        │                                              │
│ Paint Cells   │                 LEVEL VIEW                   │
│ Surface       │                                              │
│ Elevation     │                                              │
│ Cover         │                                              │
│ Walls         │                                              │
│ Transition    │                                              │
│ Hazards       │                                              │
│ Interactions  │                                              │
│ Spawn         │                                              │
│ Objective     │                                              │
├───────────────┼──────────────────────────────────────────────┤
│ OVERLAYS      │ Selected: FRTCellId{X,Y,Layer}              │
│ Movement      │ Surface                                     │
│ LOS           │ Elevation                                   │
│ Cover         │ Cover                                       │
│ Noise         │ Cost                                        │
│ Threat        │ Tags                                        │
│ Height        │                              [VALIDATE MAP]  │
└───────────────┴──────────────────────────────────────────────┘
```

Questa è una visione graduale.

NON va implementata tutta dentro #620.

---

# 14. Brush system desiderato

Da considerare come UX futura sopra il sistema esistente.

## Cell Brush

Possibili superfici:

- Ground/Floor
- Water
- Ice
- Metal
- Mud/Rough
- Void
- altre superfici canoniche già presenti nel catalogo

Con:

- radius;
- layer;
- paint/erase;
- fill;
- eventualmente selection set.

## Height / Layer Brush

Attenzione a distinguere:

- `Layer` = identità logica della posizione sovrapposta;
- elevation/height = proprietà geometrica/tattica.

Un ponte può avere:

```text
FRTCellId{10, 7, 1}
```

sopra:

```text
FRTCellId{10, 7, 0}
```

senza diventare la stessa cella.

Non confondere layer ed elevation.

---

# 15. Edge editor

In RefactorTactics gli edge sono dati di prima classe.

Concettualmente il designer deve poter lavorare sui sei bordi hex:

```text
       edge
     _______
   /         \
  /           \
 |     HEX     |
  \           /
   \_________/
```

Tipi/semantiche possibili, sempre mappati sui tipi canonici reali:

- Open
- Low Cover
- High Cover
- Door
- Transition
- Bridge
- special transition

Non introdurre enum nuovi se il runtime possiede già un tipo canonico equivalente.

Proprietà logiche da visualizzare quando esistono:

- movement allowed/blocked;
- LOS;
- projectile blocking;
- cover type;
- integrity;
- open/closed;
- enabled/disabled;
- transition type;
- direction.

---

# 16. Orientamenti architettonici

Decisione recente:

L’architettura NON è limitata ai soli sei lati degli hex.

Le direttrici tattiche ammesse derivano:

- dagli assi principali dell’esagono;
- dalle rispettive ortogonali.

Per una griglia regolare si ottengono sei assi non orientati distanziati di 30°.

Idea concettuale:

```text
0°
30°
60°
90°
120°
150°
```

Ma NON incidere questi valori arbitrariamente nella logica.

La grammatica concreta deve essere quella discreta/canonica implementata da #620.

> 🔎 **PANEL — dichiarare l’orientamento, o quei gradi non vogliono dire niente.**
>
> La griglia è **pointy-top**. Non è un dettaglio di rendering: è il datum a cui «0°» si riferisce, e
> senza di esso l’elenco qui sopra è ambiguo di 30°. Il repository lo pinna in due punti —
> `RefactorTactics.Hex.HexCornersPointyTop` (`RTHexTests.cpp:238`) e il commento di `HexCorners`
> («pointy-top, primo vertice a -30 gradi») — ma **nessuno dei due handoff lo diceva.**
>
> Ne segue una raccomandazione sui nomi. Il file 02 §6 propone `ERTTacticalAxis{Axis0…Axis150}`:
> nomi che incidono gradi sono ancorati a una convenzione di orientamento, e se la convenzione viene
> letta male **i nomi mentono senza che un test se ne accorga** — perché i nomi non sono testabili.
>
> Due uscite, entrambe accettabili:
>
> 1. nominare gli assi per **ruolo** (`MainA/MainB/MainC` + `OrthoA/OrthoB/OrthoC`), derivandoli dalla
>    libreria come §8 già impone;
> 2. tenere i gradi, ma scrivere nel commento del tipo che il datum è pointy-top con primo vertice a −30°,
>    e coprire la corrispondenza con un test.
>
> Le sei **direzioni di movimento** restano sei e sono cosa diversa dai dodici settori di occupancy di
> #619: un settore ogni 30°, che non sono direzioni. Non confondere i due conteggi.

> 🔎 **PANEL — D0-bis: gli assi di questa sezione COINCIDONO con i confini dei dodici settori,
> e le soglie di #619 non sono tarate per questo.**
>
> **Il fatto geometrico.** I confini radiali dei dodici settori stanno a `-30 + 30k` gradi
> (`RTHexOccupancyLibrary.cpp:99`): tutti i multipli di 30°. Gli assi tattici che questa sezione impone
> sono 0°, 30°, 60°, 90°, 120°, 150°. **Sono lo stesso insieme di angoli.**
>
> **La regola che si attiva.** Il contatto su un confine conta come occupazione di *entrambi* i settori
> adiacenti. Non è un caso limite dimenticato: è deliberato e commentato — `RTHexOccupancyLibrary.cpp:38`,
> *«un muro appoggiato esattamente al confine fra due settori li invade entrambi»*. Presa da sola è la
> scelta giusta.
>
> **La conseguenza, con le soglie reali** (`ConstrainedFrom = 4`, `BlockedFrom = 6`). Il meccanismo è
> certo; il conteggio esatto **no**, e va misurato — vedi l’avvertenza in fondo. Due effetti si sommano:
>
> 1. un muro su un lato hex è **collineare** ai lati esterni dei due triangoli adiacenti → li accende
>    entrambi per la regola conservativa;
> 2. i suoi **estremi cadono su vertici di confine**, e `PointInTriangle` conta il bordo
>    (`RTHexOccupancyLibrary.cpp:54`) → si accendono anche i triangoli che condividono quel vertice.
>
> Il secondo effetto è quello che sfugge leggendo in fretta, ed è additivo: un muro perimetrale occupa
> **più** dei due settori che il designer si aspetta guardando la cella.
>
> Con `BlockedFrom = 6` su dodici settori, il margine è stretto: se pochi muri perimetrali bastano a
> superare la soglia, una cella con la maggioranza dei lati ancora aperti risulta `Blocked` — e **#621
> cuocerebbe quel `Blocked` in `bBlocksMovement`**, rendendo impassabile una cella attraversabile.
> L’angolo di una stanza è precisamente la geometria per cui #620 e #621 esistono.
>
> **Perché nessuno se n’è accorto.** Le quattro fixture usano `-20`, `-10`, `10`, `40`, `-15` gradi:
> **evitano tutte i multipli di 30°**, e il commento della Fixture 1 lo dichiara —
> *«non tocca i confini, quindi l’esito non dipende da come si trattano i casi collineari»*. Le soglie
> sono state calibrate contro geometria fuori asse e stanno per ricevere solo geometria in asse.
> Nessuno dei diciassette test `HexOccupancy.*` copre il caso.
>
> **Uscite possibili, da decidere prima di #620 — nessuna è ovvia:**
>
> 1. **Ritarare** `ConstrainedFrom`/`BlockedFrom` per la geometria in asse (non tocca codice chiuso, solo
>    valori di default);
> 2. **De-duplicare in `ComputeMask`**: un segmento esattamente collineare a un confine accende un solo
>    settore — ma cambia una regola chiusa e va motivata;
> 3. **Contare i lati murati invece dei settori** quando la geometria è perimetrale: è il conteggio che il
>    designer ha in testa;
> 4. **Accettare** e dichiararlo semantica voluta.
>
> ⚠️ **Questo blocco è derivato leggendo il codice, non eseguendolo — e la distinzione qui pesa.**
> Rifacendo i conti a mano tre volte ho ottenuto tre conteggi diversi: la geometria dei dodici triangoli
> con la regola «il bordo conta» è più insidiosa di quanto sembri, e nessuna quantità di lettura
> sostituisce una misura.
>
> Per questo la revisione ha scritto due test invece di un numero:
>
> | Test | Cosa fa |
> |---|---|
> | `HexOccupancy.SegmentOnSectorBoundaryOccupiesBothAdjacentSectors` | **Asserisce** — il caso radiale è verificabile con certezza: 2 settori sul confine contro 1 fuori. Pinna la regola conservativa, che finora nessun test proteggeva |
> | `HexOccupancy.PerimeterWallsOccupancyIsRecorded` | **Misura e registra** — asserisce solo la monotonia (l’OR non spegne bit, vera per costruzione) e stampa in `AddInfo` settori e classificazione per 1, 2 e 3 muri |
>
> **Il secondo va letto da un umano la prima volta che gira.** Se un muro solo, o due, bastano a portare
> la cella a `Blocked` mentre restano quattro o cinque lati aperti, le soglie vanno ritarate **prima**
> che #621 esista. Se invece il margine regge, D0-bis si chiude senza toccare niente e i due test
> restano come protezione contro la regressione.

---

# 17. Overlay diagnostici desiderati

Questi sono il vero valore del Level Designer tooling.

## 17.1 Movement

Dato un’origine/profilo unità/budget:

- reachable;
- expensive;
- blocked;
- path cost;
- block reason;
- special transitions.

## 17.2 Connectivity

Mostrare:

- connected regions;
- unreachable islands;
- broken transitions;
- unidirectional mistakes;
- layer senza ingressi.

Questa parte è già direttamente collegata a #554.

## 17.3 LOS

Selezione source → mostrare:

- visible;
- occluded;
- blocked;
- opacity/attenuation quando prevista;
- blocker.

Non duplicare il servizio LOS.

## 17.4 Cover

Mostrare il bordo da cui la cover protegge.

La cover è direzionale.

Non basta:

`HasCover = true`

Serve:

`quale Edge protegge e di che tipo`.

La visualizzazione base è già stata implementata; futuri probe possono renderla interrogabile.

## 17.5 Facing

Dato:

- cella;
- facing;

mostrare:

- front;
- flank;
- rear;
- settori di risposta;
- interazione con cover/Overwatch quando le relative regole sono disponibili.

---

# 18. Overwatch overlay — futuro

Dopo che la feature Overwatch runtime è stabile, il Level Designer dovrebbe poter fare un probe:

```text
Source Cell
Facing
Overwatch profile
```

Output:

- celle/archi coperti;
- LOS;
- entry lanes;
- choke controllati;
- route che entrano nella zona;
- eventuale percentuale/numero di tactical routes attraversate.

Scopo:

evitare mappe dove un singolo punto produce Overwatch dominante senza counter-route.

NON implementare un “nuovo Overwatch simulator” nell’editor.

Riutilizzare le regole runtime.

---

# 19. Noise overlay — futuro

Il rumore è un sistema tattico, non solo audio.

Il Level Designer dovrebbe poter generare una sonda:

```text
Noise source
Intensity
Noise type / profile
```

e vedere propagazione/attenuazione considerando, quando implementati:

- archi;
- muri;
- porte;
- tunnel;
- superfici;
- rumore ambientale;
- layer.

Output:

- intensità per cella;
- zone percepibili;
- attenuatori;
- possibili reveal/awareness zones.

La propagazione deve usare il grafo/sistema rumore canonico, non SphereOverlap inventato nell’editor.

---

# 20. Environment interaction preview — futuro

Il designer dovrebbe poter verificare catene ambientali:

```text
Water
   +
Electric
   ↓
Conductive propagation
```

oppure:

```text
Ice
  +
Fire
  ↓
Water
  +
Heat
  ↓
Steam
```

Probe utili:

- affected cells;
- propagation boundaries;
- layer involved;
- blockers;
- door/edge effect;
- unit-independent logical result.

Non ricreare l’ambiente nel tool.

Usare le funzioni pure/runtime quando disponibili.

---

# 21. Spawn / objective analyzer — futuro

Per ogni team/spawn/objective:

Misurare almeno:

- min path cost;
- N-best route classes;
- costo medio;
- first LOS;
- first possible contact;
- high-ground access;
- hard/low cover access;
- environmental access;
- special transition dependency.

Esempio:

```text
Relay Alpha

             TEAM A     TEAM B
Min cost       600        600
Routes           4          4
Hard cover       2          2
Water access     3          4
High ground      5          5
First LOS        4          4
```

Il tool NON deve dichiarare automaticamente:

`MAP BALANCED = TRUE`

Deve fornire dati al designer.

---

# 22. Tactical heatmaps — futuro

Il TurnLog autorevole permette analisi robuste.

Pipeline:

```text
AUTHORING
   ↓
PLAYTEST
   ↓
TurnLog
   ↓
Telemetry / parser
   ↓
Heatmaps
   ↓
LEVEL DESIGN ITERATION
```

Heatmap desiderate:

- movement;
- combat;
- KO/death;
- exposure;
- cover usage;
- objective pressure;
- noise;
- displacement;
- reactions;
- Overwatch triggers;
- environmental interactions.

Usare eventi canonici, non l’animazione.

---

# 23. Scenario Designer / validation scenarios

Il progetto possiede già Scenario Harness e Scenario Map.

Il Level Designer workflow dovrebbe collegarsi ad essi.

Esempio concettuale:

```text
Scenario:
Spec.Map.WaterElectricLane

Teams:
2v2

Required map features:
Water
Metal
Cover
Alternative route

Expected property:
Water-electric interaction is reachable
without forcing a single mandatory route
```

Principio importante:

NON chiamare “scenario” una semplice fixture geometrica.

Il repository distingue:

- fixture/unit test geometrico;
- scenario di partita;
- verifica editor/PIE manuale.

Esempio:

- “segmento ad angolo produce questa maschera” → fixture runtime/test;
- “mappa con cover cotta permette questo esito in partita” → scenario;
- “il ghost wall si legge bene nel viewport” → verifica PIE/manuale.

---

# 24. Map Validation

Il Level Designer dovrebbe avere un comando/flow `VALIDATE MAP`.

Output ideale:

```text
[PASS] Stable IDs
[PASS] Cells valid
[PASS] Edges point to real cells
[PASS] Spawn A connected
[PASS] Spawn B connected
[PASS] Objective reachable

[WARN] Cell ...
       cover lacks expected visual affordance

[ERROR] Door ...
        inconsistent binding

[ERROR] Layer ...
        unreachable region
```

Riutilizzare sempre validator esistenti.

Non creare un validator parallelo del tool.

#620 aggiunge validator della grammatica geometrica.

#554 aggiunge reachability.

Questi devono confluire nell’esperienza del designer.

---

# 25. Map Complexity Budget — proposta futura

Pannello diagnostico, non gate automatico.

Esempio:

```text
MAP COMPLEXITY

Cells                 428
Edges                2338
Special transitions     19
Doors                    7
Dynamic covers           8
Water cells             24
Hazard sources           6
Interactive props       14
Layers                    3
```

Possibili indicatori:

- path complexity;
- LOS complexity;
- environmental complexity;
- graph branching;
- number of dynamic topology elements.

Scopo:

impedire mappe ingestibili senza trasformare la metrica in una legge arbitraria.

---

# 26. Procedural generation e Level Designer

Il procedural non sostituisce il Level Designer.

Il designer dovrebbe authorare:

- archetipi;
- vincoli;
- range;
- metriche;
- required features;
- diversity constraints.

Esempio concettuale:

```text
Map Archetype: Industrial Small

Cell target: 220–280
Required:
- 2 spawns
- 1 primary objective
- >= 3 route classes
- >= 2 loop routes

High ground: 10–20%
Hard cover: 8–15%
Water: 0–12%
Critical choke: max 2
```

Pipeline futura:

```text
GENERATE N MAPS
      ↓
VALIDATE
      ↓
SCORE
      ↓
SHORTLIST
      ↓
LEVEL DESIGNER REVIEW
```

IMPORTANTE:

Riutilizzare:

- map graph;
- pathfinding;
- LOS;
- spawn system;
- validator;
- Scenario Harness;

non crearne una copia “procedural”.

---

# 27. Workflow ideale del Level Designer

Visione target:

```text
Create Map
   ↓
Choose archetype / blank
   ↓
Paint hex layout
   ↓
Define layers
   ↓
Paint surfaces
   ↓
Author architectural geometry
   ↓
Bake to canonical spatial data
   ↓
Place / bind transitions
   ↓
Place spawn / objective
   ↓
VALIDATE
   ↓
Movement / connectivity probes
   ↓
LOS / cover / facing probes
   ↓
Environment / Overwatch / noise probes
   ↓
Run scripted scenarios
   ↓
Playtest
   ↓
Analyze TurnLog / heatmaps
   ↓
Iterate
   ↓
Validate
   ↓
Package
```

> 🔎 **PANEL — questo è un diagramma di successo, non un workflow.**
>
> Sedici passi in fila e **nessun ramo di fallimento**. La domanda che il documento non risponde:
> cosa fa il designer quando `VALIDATE` fallisce al passo dodici? Torna al bake? Al paint? Perde il lavoro
> fatto dopo? È lì che uno strumento si rivela usabile o no, ed è l’unico punto del workflow che nessuna
> delle cinque issue copre.
>
> Servono **due o tre casi d’uso a livello *user goal***, ciascuno con le sue estensioni. Minimo:
>
> | Caso d’uso | Attore | Estensione che manca oggi |
> |---|---|---|
> | Autorare una regione nuova sul bordo | Level Designer | La grammatica rifiuta il segmento → cosa vede? l’edit è respinto o accettato-e-segnalato? |
> | Diagnosticare un’area isolata | Level Designer | `VALIDATE` segnala unreachable → come si risale **alla transizione mancante**? |
> | Verificare la fairness prima del playtest | Level Designer | I numeri non tornano → quale passo si rifà, senza rifare tutto? |
>
> Nota per #620 in particolare: **«l’edit è respinto o accettato-e-segnalato?»** non è una questione di UX.
> Se è accettato, esiste uno stato invalido persistibile, e il validator deve poter girare su una mappa
> salvata. Se è respinto, non esiste. Le due scelte producono due validator diversi — va decisa prima.

---

# 28. Roadmap concettuale aggiornata

NON trattare questa tabella come nuova autorità. Va consolidata nei documenti owner.

## Fase A — già largamente eseguita

- Hex Editor Mode
- Select
- Paint
- Fill
- Arch
- Surface
- layer
- Focus
- cost visualization
- blockers
- cover/door visual
- click robusto
- occupancy

## Fase B — adesso

1. #620 Geometry Grammar + Validator
2. #621 Geometry Bake
3. #554 Reachability / transition visibility
4. #622 Workspace Grid
5. #623 Lighting + Frame Map

## Fase C — navigation/spatial probes

- reachable area;
- path inspector;
- reason codes;
- connectivity regions;
- selected-unit movement profile;
- path cost overlays.

## Fase D — tactical probes

- LOS;
- cover;
- facing;
- ability geometry;
- displacement;
- Overwatch geometry.

## Fase E — environment sandbox

- water;
- fire;
- electric;
- smoke/steam;
- ice;
- noise.

## Fase F — production analytics

- spawn/objective analysis;
- heatmaps;
- scenario runner UX;
- complexity metrics;
- batch procedural evaluation;
- prefab/multi-select/mirror solo se misurati utili.

---

# 29. Cose esplicitamente da NON fare

1. Non creare un secondo map graph.
2. Non creare un secondo A*.
3. Non creare un secondo LOS service.
4. Non creare un secondo Scenario Harness.
5. Non creare un secondo validator globale.
6. Non creare Actor per ogni cella.
7. Non mettere planning o simulazione dentro l’editor.
8. Non rendere una mesh runtime-authoritative.
9. Non creare `Walls[]` runtime parallelo a `FRTHexCover`.
10. Non creare `LowWall` come nuovo tipo se corrisponde a `ERTHexCoverType::Low`.
11. Non scrivere il surcharge di occupancy dentro `MoveCost`.
12. Non usare float arbitrari serializzati per geometry grammar.
13. Non incidere direzioni/angoli a mano.
14. Non aggiornare shortlist generate manualmente.
15. Non trasformare fixture geometriche in Scenarios solo per “avere uno scenario”.
16. Non riscrivere la navigazione del viewport Unreal.
17. Non toccare `RTCameraPawn` per problemi di editor viewport.
18. Non assumere lo stato di issue/PR da questo file: verificarlo sempre.
19. Non aprire issue duplicate senza cercare aperte e chiuse.
20. Non confondere una idea futura con scope v0.1.

> 🔎 **PANEL — un divieto senza metodo di rilevazione è un auspicio.**
>
> Venti prescrizioni negative, quasi nessuna con un modo di accorgersi che è stata violata. Il repository
> possiede già la forma giusta — il file 02 §16/§28 usa `git ls-files` come **oracolo** invece di «il file
> esiste sul disco». Applicare la stessa disciplina ai divieti che contano di più:
>
> | Divieto | Come ci si accorge della violazione |
> |---|---|
> | #1/#2/#3 — secondo graph / A* / LOS | `git grep -n "class.*\(Graph\|Pathfind\|LineOfSight\|LOS\)" Source/RefactorTacticsEditor/` → deve restare **vuoto** |
> | #6 — Actor per cella | `git grep -n "SpawnActor" Source/RefactorTacticsEditor/` → nessuno nel path di disegno griglia |
> | #9 — `Walls[]` parallelo | `git grep -n "Walls\b" Source/RefactorTactics/Map/` → nessun array nuovo accanto a `FRTHexCover` |
> | #11 — surcharge dentro `MoveCost` | `git grep -n "MoveCost\s*[+*]=" Source/` → nessuna scrittura fuori dal produttore dichiarato |
> | #14 — shortlist editate a mano | `git diff --stat -- docs/roadmap/*.shortlist.md` in una PR che non ha rilanciato lo script |
>
> Cinque righe in checklist di PR valgono più di venti in prosa: sono le uniche che qualcuno può
> **eseguire** durante una review invece di ricordarsele.

---

# 30. Outcome desiderato

Il Level Designer deve arrivare a poter:

> creare una mappa tatticamente valida, capire come il simulatore la interpreta, verificare connectivity/LOS/cover/costi, eseguire scenari, individuare problemi e iterare senza aprire Visual Studio.

C++ definisce:

- invarianti;
- possibilità;
- tipi;
- validator;
- pathfinding;
- LOS;
- targeting;
- resolver;
- bake deterministico.

Editor/Data/Blueprint definiscono:

- layout;
- varianti;
- authoring;
- visualizzazione;
- presentazione;
- contenuto.

---

# 31. Fonti repository da leggere prima di implementare

Minimo:

- `CLAUDE.md`
- `AGENTS.md`
- `CONTEXT_INDEX.md`
- `README.md`
- **`docs/OPEN_DECISIONS.md`** — 🔎 PANEL: **è dove vive `MSE-1`**, la decisione che blocca il gate `spec`
  di `RT-FEAT-TOOL-MAP-GEOMETRY`. Mancava da questo elenco, ed è la fonte che più di ogni altra evita
  di riaprire una domanda già registrata.
- **`docs/roadmap/featuremap.shortlist.md`** — 🔎 PANEL: GENERATA. È la fonte di verità dello stato per
  gate, e prevale sulla matrice §5 di questo file.
- `docs/roadmap/feature-registry.yaml`
- `docs/roadmap/editor-sessions.yaml`
- `docs/roadmap/editormap.shortlist.md` (GENERATA, non editare a mano)
- `docs/technical/test-manuali-pie.md`
- `docs/technical/scenario-map.md`
- `docs/technical/brief-editor-map-viz.md`
- `docs/roadmap/plans/map-sketch-editor-spec-panel-2026-08-12.md`
- `docs/roadmap/plans/map-editor-brief-spec-panel-2026-08-09.md`
- roadmap v0.1 / post-v0.1 pertinenti
- issue #554, #588, #619, #620, #621, #622, #623
- epic #324 / E23 per il confine authoring vs runtime

Codice minimo:

- `Source/RefactorTacticsEditor/`
- `Source/RefactorTactics/Map/`
- `Source/RefactorTactics/Pathfinding/`
- `Source/RefactorTactics/Turn/`
- `Source/RefactorTactics/ScenarioHarness/`
- `Source/RefactorTactics/Tests/`

---

# 32. Sintesi da ricordare

La decisione più importante di questo focus è:

> **Non stiamo progettando un nuovo editor. Stiamo completando e consolidando il Level Designer workflow sopra l’Hex Map Editor già esistente.**

La seconda:

> **La geometria di authoring può essere ricca, ma il gameplay deve continuare a leggere dati canonici discreti e deterministici.**

La terza:

> **Il prossimo step reale è #620, non una nuova epic generica “Level Designer”.**

La quarta:

> **Le idee avanzate — Overwatch/Noise/Heatmaps/Spawn Analysis — vanno registrate e pianificate dopo avere chiuso il workflow base di architectural authoring e spatial validation.**
