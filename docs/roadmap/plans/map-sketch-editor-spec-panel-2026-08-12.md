# Map Sketch Editor v0.1 — spec panel

> `CURRENT` · **Stato**: revisione chiusa, **applicata in parte** · **Data**: 2026-08-12
> **HEAD della revisione**: `399041c7`
> **Sorgente revisionato**: `RefactorTactics_Map_Sketch_Editor_v0.1_Claude.md` (1166 righe, untracked),
> archiviato a fine sessione in
> [`../../archive/src/handoff/2026-08-12-map-sketch-editor.md`](../../archive/src/handoff/2026-08-12-map-sketch-editor.md)
> **Scopo**: classificare ogni istruzione del prompt contro il repository **prima** che qualcuno la applichi a
> Feature Registry, roadmap, epic, scenari o codice.
> **Regola applicata**: un handoff AI è l'ultima fonte della gerarchia. Dove contraddice un ADR, una `D-0xx` o
> un'epic già assegnata, prevale il canone e la proposta si **registra**, non si applica — salvo decisione
> esplicita dell'autore, che qui è stata presa **due volte** e si legge in §4.
> **Terzo della sua famiglia**: vedi §1.1.

---

## 1. Il verdetto in una riga

Il prompt è il **primo della serie map-editor che paga il proprio debito tecnico**: la sua §4 — l'esagono
diviso in dodici settori — è la risposta puntuale all'obiezione che aveva bloccato il predecessore, cioè che
un muro world-space porta estremi in virgola mobile dentro un hash che tiene fermo un KPI. Attorno a quella
idea, però, ricostruisce per la terza volta un editor che esiste, e ordina il lavoro su un asse di
numerazione che collide con i tre già in uso.

| | Voci | Significato |
|---|---:|---|
| `CURRENT` | **8** | il prompt riporta correttamente il canone, spesso senza sapere che è canone |
| `DUPLICATE` | **8** | chiede di costruire qualcosa che ha già un owner, un formato e dei test |
| `CONFLICT` | **6** | contraddice una decisione accettata, un'epic assegnata o una regola di processo |
| `PROPOSED` | **7** | idea nuova, nessun conflitto: si registra o si costruisce |
| `STALE` | **3** | si rivolge a un artefatto ritirato o generato |

**Su 32 voci classificate, 16 hanno già un padrone.** Ciò che resta vale cinque issue, non trentasette
checkpoint su quattro livelli di priorità.

### 1.1 Il terzo della famiglia, e cosa era già successo agli altri due

Questo non è il primo prompt di map editor arrivato a questo repository, ed è importante dirlo perché due
delle sue tesi centrali hanno **già** un verdetto:

| Sorgente | Referto | Esito |
|---|---|---|
| `2026-08-09-map-editor-roadmap.md` | [`map-editor-brief-spec-panel-2026-08-09.md`](map-editor-brief-spec-panel-2026-08-09.md) | ⛔ **Revisionato e non applicato** — 9 duplicati, 5 conflitti. Sopravviveva **una** proposta |
| `2026-08-10-full-grid-geometry-walls-water.md` | [`triage-grid-geometry-water-2026-08-10.md`](triage-grid-geometry-water-2026-08-10.md) | 3159 righe, 55+ sezioni `LOCKED`; tre feature `IDEA`, `GEO-1`…`GEO-3` |
| *questo* | *questo file* | **applicato in parte**, per decisione esplicita dell'autore (§4) |

⚠️ **La lezione più cara non sta in nessuno dei due referti: sta in ciò che non è successo dopo.** Il panel
del 2026-08-09 chiudeva indicando la sua «fetta reale» — la sonda di movimento nell'editor — e cinque altre
proposte. **Nessuna delle sei è mai diventata una issue.** Misurato oggi: `FRTHexReachableCell` è ancora
`{Cell, Cost}` (`RTHexSim.h:65`), il campo `Parent` di `P2` non c'è, e `gh issue list --search "sonda
movimento editor"` non trova nulla.

È il difetto che questo repository ha già catalogato — *una prescrizione scritta in una colonna e mai
aperta* — e la conseguenza è concreta: **tre giorni dopo è arrivato un prompt che riproponeva le stesse
cose**, perché niente di ciò che era stato deciso era visibile a chi l'ha scritto. Per questo il presente
referto apre le issue **nello stesso commit** in cui si scrive, e ne aggancia i numeri in due posti — qui e
nel campo `issues:` del registry.

---

## 2. Il panel

Sei revisori, un focus ciascuno. Le citazioni sono ricostruzioni della metodologia, non attribuzioni reali.

### 📋 WIEGERS — qualità dei requisiti

> «§32 elenca ventidue criteri di accettazione e li apre tutti con lo stesso verbo: *"Wall funziona"*,
> *"LowWall funziona"*, *"occupancy 12-sector funziona"*, *"Undo/Redo funziona"*. Con quale osservazione
> dimostro che *"Wall funziona"* è **falso**? Non esiste. È la stessa struttura che il brief precedente aveva
> in §33, e il rilievo è lo stesso: questo progetto ha già risolto il problema meglio di così —
> `feature-registry.yaml` porta nove gate con valori `done|partial|todo|na`, e
> `scripts/feature_registry.py validate` **fallisce** se lo stato dichiarato non regge i gate.»

> «C'è però una differenza rispetto al predecessore, e va riconosciuta: §14 e §15 sono **falsificabili**. Una
> cella con maschera `001111000100` e cinque settori occupati su dodici o è classificata `Constrained` o non
> lo è, e la soglia è un numero che posso cambiare per vedere l'overlay cambiare. Quello è un requisito.
> Il resto di §32 è una lista della spesa.»

> «Rilievo più serio, su §5. Il prompt introduce tre stati — `Free`, `Constrained`, `Blocked` — e poi scrive
> che *"per v0.1 `Constrained` può essere trattata come traversabile dal pathfinding se non esistono ancora
> regole specifiche"*. Tradotto: nella v0.1 `Constrained` e `Free` sono **indistinguibili per ogni
> consumatore**. Questo repository ha un nome per quella cosa, e l'ha pagata più volte: un campo che nessuno
> legge. Non è un difetto minore da sistemare dopo — è la ragione per cui §5 va costruita **con** il suo
> consumatore o non va costruita.»

### 🎯 COCKBURN — attore primario e obiettivo

> «L'attore qui è nominato e non sparisce mai, e questa è la cosa migliore del documento: il *level designer
> che deve vedere la griglia prima di disegnarci sopra*. §7 elenca tre problemi osservati — griglia invisibile,
> scena buia, camera scomoda — e sono osservazioni di produzione, non desiderata. Il predecessore perdeva
> l'attore dopo §9; questo no.»

> «Ma §21 chiama "Scenario" tre cose che in questo repository sono un'altra: `MapSketch_House`,
> `MapSketch_Quarry`, `MapSketch_CoverGeometry` chiedono di *aprire l'editor, disegnare, guardare un
> overlay*. `scenario-map.md` classifica le verifiche in **A** (la macchina esegue e giudica), **B** (la
> macchina esegue, l'umano giudica), **C** (solo umano). Tre verifiche di authoring interattivo sono
> **classe C**, e la classe C ha già un registro suo — `test-manuali-pie.md` con le voci `PIE-*`, e
> `editor-sessions.yaml` per le sedute. Metterle in `Scenarios/` prometterebbe copertura automatica e
> consegnerebbe arretrato manuale.»

### 🏗️ FOWLER — confini e responsabilità

> «§3 è la tesi del documento — *"La geometria architettonica NON coincide necessariamente con i lati degli
> hex"* — ed è **letteralmente la stessa tesi** del brief del 2026-08-09 §1.3. Quel panel non l'ha respinta:
> l'ha collocata, in **E23.1** (v0.2, *Separazione geometria/logica*), con la sintesi che la rende
> compatibile: un muro world-space è legittimo **come gesto di authoring** se il suo effetto viene *cotto*
> nei dati che già esistono, e se dopo la cottura la geometria è arte.»

> «Il prompt arriva alla stessa sintesi da solo — §20, *"Authoring Geometry → Bake → Runtime Spatial Data"*,
> e *"Runtime non deve dipendere da Editor-only UObject/classes"*. Non c'è disaccordo di modello. Il
> disaccordo è di **collocazione**, ed è una scelta dell'autore, non del panel: vedi §4.»

> «Un rilievo che nessuno dei due documenti fa, e che conta più della collocazione: `WALL` e `LOW WALL` di §12
> **hanno già un bersaglio canonico**, e non è una cella. `ERTHexCoverType` ha tre valori, e le loro
> definizioni sono scritte nel codice: `Low` *"ripara dai colpi diretti che attraversano il bordo, non blocca
> né vista né passo"*; `High` *"NEGA l'attraversamento del bordo a vista, passo e proiettili"*. Sono
> esattamente il muretto e il muro del prompt, **già sui bordi, già serializzati, già sparsi**. §12 dice che
> *"`LOW WALL` è un tipo geometrico distinto ma NON assegna automaticamente cover"*: quella riga, presa alla
> lettera, crea una **seconda rappresentazione** dello stesso oggetto visibile. Un muretto disegnato che non è
> `FRTHexCover{Low}` è un muretto di cui il gioco non sa niente.»

### 🛡️ NYGARD — modi di guasto e invarianti

> «Qui devo dare atto di un progresso reale. L'obiezione che avevo mosso al brief precedente era che gli
> estremi di un muro sono float, che i float entrano nell'hash di `URTHexMapAsset`, e che due macchine che
> arrotondano diversamente rompono il KPI *replay divergence = 0*. §3 e §11 di questo prompt rispondono
> **precisamente** a quella obiezione: la grammatica ammette solo direttrici derivate dall'esagono, le loro
> ortogonali, i lati del perimetro e le junction compatibili. Una geometria enumerabile non ha estremi
> arbitrari, e una maschera a dodici bit è un intero. **Questo è il contributo tecnico del documento**, ed è
> ciò che lo distingue dai due che l'hanno preceduto.»

> «Detto questo, la difesa va costruita, non dichiarata. §24 chiede un validator per *"off-axis tactical
> geometry"*: quel validator **è** l'invariante. Senza, la grammatica è una convenzione — e questo repository
> ha un caso **aperto in questo momento** che dice come va a finire: `#588` osserva che il discriminante del
> click guarda l'*actor* invece del *componente*, cioè che *"il vincolo che tiene in piedi lo strumento è una
> convenzione"*, e la PR `#598` che lo corregge **non è mergiata**. Sul ramo di questa revisione
> `RTHexEditorClick.cpp:84` porta ancora `Result.GetActor() == Actor`. Non è una lezione imparata: è una
> lezione in corso, ed è più utile così — la grammatica di §11 nasce oggi, e può nascere con la sua rete
> invece che acquisirla dopo.»

> ⚠️ **La lezione si è chiusa lo stesso giorno, ventotto minuti dopo.** La PR `#598` è **mergiata** alle
> `07:07:42Z` del 2026-08-12 — le issue di questa revisione erano state aperte alle `06:39:54Z` — e `#588` si è
> chiusa un secondo dopo. Su `main` `RTHexEditorClick.cpp` confronta ora `Result.GetComponent()`.
> Il rilievo di NYGARD sopra resta valido come **argomento** — un validator è ciò che trasforma una
> convenzione in una regola — ma il suo esempio non è più «in corso». Chi lo cita per sostenere che la
> grammatica di §11 nasca con la sua rete, citi `#588` **chiusa dalla sua correzione**, non lo stato della PR.

> «Ultimo, su §19. *"NON ricostruire tutta la mappa a ogni movimento del mouse"* è un requisito di
> performance senza una misura. Oggi `RebuildInstances` ricostruisce l'intero ISM a ogni cambiamento, ed è
> agganciato a `OnMapChanged` e `PostEditUndo` — ed è **esattamente quel comportamento** che finora ha
> impedito alle celle di divergere dall'asset. Sostituirlo con un rebake incrementale non è
> un'ottimizzazione: è togliere la proprietà che rende la vista non-bugiarda, e va fatto solo con un numero
> in mano che dica che serve.»

### 🔗 HOHPE — flusso, revisioni, invalidazione

> «§19 disegna la catena giusta — *commit → determine affected cells → rebake affected region → update
> overlay → update revision*. La revisione esiste già: `FRTHexSnapshot` porta hash e revisione, `IsSnapshotStale`
> e `ValidateSnapshot` sono i controlli, e il resolver si rifiuta di girare su uno snapshot vecchio.»

> «La domanda che il prompt non pone, e che decide se la cottura è sicura: **la cottura è invertibile?** §20
> separa *Authoring Geometry* da *Runtime Spatial Data*, ma non dice cosa succede se qualcuno modifica a mano
> `bBlocksMovement` su una cella cotta. Il panel precedente aveva registrato il costo — *"la cottura non è
> invertibile senza conservare il sorgente… è la stessa classe di problema dei prefab, e va deciso, non
> scoperto"* — e questo documento non lo decide. È l'unica domanda di modello che resta aperta dopo §4.»

### 🧪 CRISPIN — strategia di test

> «§22 chiede sei famiglie di test. Le misuro una per una contro `Source/`:»

| §22 chiede | Stato misurato |
|---|---|
| `SectorMask deterministic` | **non esiste** — è il cuore della proposta, ed è la parte più facilmente testabile headless |
| `CoreBlocked classification` | **non esiste** |
| `Threshold classification` | **non esiste** |
| `Geometry -> affected cells stable` | **non esiste** |
| `Save/reload` | esiste come famiglia: `RefactorTactics.HexMap.*`, più il gate di formato di `RT-FEAT-TOOL-VALIDATION` |
| `Undo/Redo where testable` | esiste in forma indiretta (`LookupInvalidatedAfterExternalEdit`, `LayerFilterOnConstruction`) |

> «Quattro su sei mancano, e sono **le quattro giuste** — cioè quelle che non duplicano niente. Notare
> l'inversione rispetto al brief precedente, dove undici test su tredici esistevano già: questo prompt chiede
> di testare una cosa che davvero non c'è.»

> «Un vincolo strutturale che nessuno dei due documenti nomina: **in `Source/RefactorTacticsEditor/` non
> esiste alcun test** — `find … -iname "*test*"` restituisce vuoto, ed è la ragione per cui la PR `#598`
> (**aperta**) sposta una funzione nel modulo runtime: là dov'era, la regola non sarebbe stata verificabile.
> Che quella PR non sia ancora mergiata non indebolisce l'argomento, lo rafforza — il vincolo è della
> struttura del progetto, non di quel lavoro. Ne segue una regola di progettazione, non un
> desiderio: *la maschera dei settori, la classificazione e la cottura vivono nel modulo runtime*; l'editor
> le **chiama**. Se nascono dentro l'editor, nascono non verificabili.»

> ⚠️ **`#598` è mergiata dal 2026-08-12T07:07:42Z**, quindi «(**aperta**)» sopra è scaduto. La conclusione
> però non dipendeva da quella PR, e **è stata rimisurata**: su `origin/main`, `Source/RefactorTacticsEditor/`
> porta 19 file e **zero** corrispondenze `*test*`. La regola di collocazione resta, con una prova nuova.

### 📐 ADZIC — esempi eseguibili

> «§14 è la parte migliore del documento, e vale la pena citarla intera perché è ciò che un requisito
> dovrebbe sembrare:»
>
> ```text
> Cell: (Q,R,L) · Occupied Sectors: 5 / 12 · Mask: 001111000100 · Core: Free · Classification: Constrained
> ```
>
> «Ha un'entità, uno stato, una rappresentazione binaria e una conclusione derivabile. Da questo si scrive un
> test senza chiedere niente a nessuno.»

> «E proprio per questo si vede cosa manca: **la baseline di §4 non ha una fonte**. `0-3 → Free`, `4-5 →
> Constrained`, `6+ → Blocked` è dichiarata *"da validare visivamente"*, il che è onesto, ma non dice **su
> quale mappa**. Il progetto ha il posto — `Scenarios/Spec/Map/` — e oggi contiene **un solo file**,
> `BridgeBreaksThePath.json`. Le quattro fixture che §22 elenca (segmento solido, angolo, footprint solido,
> footprint void) sono la cosa da scrivere per prima: senza, la soglia si tara a occhio e il numero resterà
> quello che era al primo tentativo.»

> «Nota di vocabolario, come per il predecessore: §14 scrive `(Q,R,L)`. Il repository scrive `FRTCellId{X, Y,
> Layer}` con X/Y assiali, e i log del bot mostrano `(q,r,L)` minuscolo. È una sciocchezza, ma è il tipo di
> sciocchezza che genera un secondo nome per la stessa cosa.»

---

## 3. `CONFLICT` — le sei voci che contraddicono il canone

| # | § | Il prompt dice | Il canone dice | Esito |
|---|---|---|---|---|
| **C1** | §3 | La geometria architettonica non coincide coi lati dell'hex, ed è la direzione della v0.1 | **E23.1** (v0.2, epic [`#324`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324)): *«la logica di transizione non legge la mesh: legge archi e stati»*. Il panel del 2026-08-09 aveva collocato lì la stessa tesi | **Risolto per decisione dell'autore** (§4.1): si anticipa in v0.1 **come strumento d'editor**, col rischio dichiarato |
| **C2** | §5 | Tre stati di cella, e in v0.1 `Constrained` è traversabile «se non esistono ancora regole specifiche» | Il repository ha già pagato quattro volte il **dato senza consumatore**; `bBlocksMovement` è il binario che tutti leggono | **Da correggere prima di costruire.** `Constrained` nasce con il suo consumatore o non nasce: vedi §6, `P2` |
| **C3** | §12 | `LOW WALL` è geometria che **non** assegna cover | `ERTHexCoverType{None, Low, High}`, per **bordo**, sparso, serializzato dal formato v3, con `Integrity` di catalogo | **Seconda rappresentazione.** Il muretto disegnato *deve* cuocere in `FRTHexCover{Low}`, o il gioco non lo vede |
| **C4** | §15 | Centralizzare le soglie in `Ruleset`, `DeveloperSettings` o `MapEditorSettings` | **Nessun `UDeveloperSettings` esiste in `Source/`** (misurato). Il pattern in uso è `UInteractiveToolPropertySet` — `BrushRadius`, `ActiveLayer`, `bShowOverlay` | **Pattern nuovo dove ne esiste uno.** Le soglie vanno dove stanno già le altre proprietà di tool |
| **C5** | §27, §28 | Priorità `P0`/`P1.1`…`P1.7`/`P2.1`…`P2.5`/`P3` | Il progetto ha **tre** assi già in uso — `M6`–`M11`, `E1`–`E38`, `CP x.y` — e le priorità sono `P0`–`P3` **nel registry**, non un albero di numerazione | **Quarto asse.** È lo stesso difetto per cui `F0`–`F6` delle cinque lane fu respinto: `P1.3` non è risolvibile senza sapere di quale documento |
| **C6** | §32 | Ventidue criteri di accettazione in forma «X funziona» | I nove gate del registry hanno valori verificati da `feature_registry.py validate` | **Non falsificabile.** I criteri utili sono quelli di §14/§15, che hanno un numero |

---

## 4. La geometria, e le due decisioni prese oggi

Questa sezione esiste perché il panel **non** ha l'ultima parola su due punti, e l'autore l'ha data.

### 4.1 Dove vive la geometria quantizzata — *anticipata in v0.1*

Il panel del 2026-08-09 raccomandava di **non** anticipare E23, con questa motivazione:

> «Anticiparlo significherebbe costruire l'authoring dei muri prima che E9 (coperture e strutture, oggi P2 e
> aperta) abbia verificato i bordi su cui i muri dovrebbero cuocere.»

**Decisione dell'autore, 2026-08-12: si anticipa in v0.1, come strumento d'editor.** La ragione che la
sostiene è quella di §Perimetro della lane 4 e del registry: `RT-FEAT-TOOL-MAP-EDITOR` è `packaged: na` e
*fuori dai gate di release* — non compete con la consegna della v0.1, perché non entra nella build di gioco.

⚠️ **Il rischio resta quello scritto sopra, e va tenuto in vista invece che dimenticato**: si costruisce la
cottura verso `FRTHexCover` e `FRTHexDoor` prima che E9 abbia verificato quei bordi in partita. Se E9
cambiasse la forma del bordo, la cottura andrebbe rifatta. La contromisura non è una promessa: è che la
cottura sia **una funzione pura testata**, non codice sparso nell'editor — ed è la ragione per cui le issue
di §9 mettono maschera, classificazione e cottura nel modulo **runtime**.

### 4.2 Layout generato **e** geometria disegnata — non si escludono

Nella stessa sessione era aperta una seconda questione, da una proposta di oggi: *le mappe si generano da
codice o si dipingono?* Argomento a favore della generazione: `Content/**` è gitignorato, un `.uasset` non
sopravvive a un clone, e durante la seduta U1 il lavoro dipinto **si è perso due volte**.

**Decisione dell'autore, 2026-08-12: convivono, perché hanno soggetti diversi.**

```text
layout tattico                        geometria architettonica
(celle, superfici, costi, spawn)      (muri, muretti, footprint, cave)
        │                                       │
   generato da codice                    disegnata nell'editor
        │                                       │
   verificato dall'hash                   cotta in dati canonici
        │                                       │
        └───────────────► URTHexMapAsset ◄──────┘
                          (interi, hashabile)
```

Il layout resta riproducibile e verificabile da un test sull'hash; il disegno serve a ciò che scrivere in
coordinate rende cieco — un perimetro di edificio, il bordo di una cava. Nessuna delle due è il workflow
«primario»: sono due input dello stesso artefatto.

> **Conseguenza che va detta**: se il layout è generato e la geometria è disegnata, l'asset ha **due
> produttori**, e un test che confronta l'hash con il solo generatore diventerebbe rosso al primo muro
> disegnato. Il test dell'hash, se verrà scritto, deve avere per soggetto la parte **generata**. Questa
> è la domanda che HOHPE lascia aperta in §2 — l'invertibilità della cottura — e **non è chiusa qui**:
> è registrata in [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) come `MSE-1`.

---

## 5. `DUPLICATE` — otto cose che hanno già un owner

| § | Il prompt chiede di creare | Esiste già come |
|---|---|---|
| §10 | Grid preview e distinzione workspace / celle esistenti / hover / selezione | `bShowOverlay` (colora per superficie, esagono rosso sulle bloccate) + `ERTLayerViewMode{AllLayers, ActiveOnly, Focus}` con `GhostLayerRange` — `#567`, mergiata `fe9883c`. Manca **solo** la griglia dove le celle non esistono: vedi `P4` |
| §12 | Tool `SELECT` · `HEX/FLOOR` · `ERASE` · `VOID/CLIFF` | `URTHexSelectTool`, `URTHexPaintTool` (`ERTHexPaintOp{Paint, Erase}`), `URTHexFillTool`, `URTHexArchTool`; `ERTHexSurface::Void` è **già** una superficie dipingibile |
| §12 | `WALL` / `LOW WALL` come tipi geometrici | `FRTHexCover{Edge, Type, Integrity}` con `ERTHexCoverType{None, Low, High}` — per bordo, sparso, formato v3 (vedi `C3`) |
| §17 | Camera: MMB pan, RMB orbit, wheel zoom, WASD, F focus, Shift veloce | **Il viewport di Unreal**, che le fornisce tutte. Un `UEdMode` non possiede la camera del viewport; `RTCameraPawn` è la camera **di gioco**, un oggetto diverso. Residuo reale: `Home` (inquadrare la mappa) e lo snap a 30° |
| §18 | Toolbar / HUD dell'editor | `FRTHexEditorModeToolkit` + `FRTHexEditorModeCommands`; i pannelli sono `UInteractiveToolPropertySet`. *«Non introdurre CommonUI»* è già rispettato |
| §21 | Tre «scenari» di authoring | **Classe C** di `scenario-map.md` → `test-manuali-pie.md` (voci `PIE-*`) e `editor-sessions.yaml` (sedute). Non `Scenarios/` |
| §23 | Integrazione Spatial Debug: griglia, ID, occupancy, classificazione, segmenti, junction | [`brief-editor-map-viz.md`](../../technical/tooling/brief-editor-map-viz.md) è l'owner del perimetro, e la serie è già aperta: `#552`, `#553`, `#554` |
| §24 | Validator di mappa | `URTHexMapAsset::ValidateMap` + `RefactorTactics.HexMap.Validate*` — `RT-FEAT-TOOL-VALIDATION`, **DONE**. I nuovi controlli si **aggiungono** lì |

---

## 6. `PROPOSED` — le sette cose davvero nuove

Nessuna contraddice il canone. Ordinate per quanto presto pagano.

| # | Cosa | Perché regge |
|---|---|---|
| **P1** | **Occupancy a 12 settori: maschera, `CoreBlocked`, classificazione** (§4, §14) | Il contributo tecnico del documento. Interamente **puro e headless**: una geometria in ingresso, dodici bit e un enum in uscita. Nessuna dipendenza dall'editor |
| **P2** | **`Constrained` con il suo consumatore** (§5) | Non è `P1`: è la condizione perché `P1` non produca un campo morto. Il consumatore minimo che il repository può già esprimere è il **costo** — una cella stretta costa di più — non un terzo booleano |
| **P3** | **Grammatica quantizzata dei segmenti e suo validator** (§3, §11, §13, §24) | Direttrici hex, ortogonali, ancore di perimetro, junction. Il validator *è* l'invariante: senza, la grammatica è una convenzione (vedi NYGARD) |
| **P4** | **Griglia di lavoro dove le celle non esistono** (§7A, §10) | L'unico pezzo di §10 che non esiste. È il difetto osservato: si disegna al buio sul bordo della mappa |
| **P5** | **Cottura geometria → dati canonici** (§14, §20) | `segmento → celle investite → maschera → classificazione → FRTHexCover / bBlocksMovement`. Funzione pura nel modulo runtime |
| **P6** | **Luci leggibili e inquadratura in `L_DevSandbox`** (§7B, §7C, §16, §17) | Nessuna issue del repository copre luci o camera d'editor (misurato: zero risultati). ⚠️ È una **seduta**, non codice: `L_DevSandbox.umap` è un `.umap`, e questo repository non modifica `.umap` da riga di comando |
| **P7** | **Sonda di movimento** — *ereditata dal panel 2026-08-09* | Non è di questo prompt, ma §23 la sfiora e il suo predecessore la indicava come «la fetta reale». Mai aperta. Vedi §1.1 |

---

## 7. Due trappole già viste, che il prompt ripete

**7.1 — Il quarto asse di numerazione.** §27 e §28 costruiscono `P0` → `P1.1`…`P1.7` → `P2.1`…`P2.5` → `P3`.
Il repository ha già respinto esattamente questa forma il 2026-08-11: le cinque lane proponevano `F0`–`F6` e
il verdetto fu che aprirebbero *«un quarto asse di numerazione accanto a `M6`–`M11`, `E1`–`E36` e `CP x.y`»*.
Peggiora qui, perché `P1`–`P3` **esistono già** nel registry come priorità piatte: `P1.3` e `priority: P1`
userebbero la stessa lettera per due cose diverse. La sequenza di §28 è comunque utile — è un **ordine**, non
una numerazione — e sopravvive come tale nell'ordine delle issue di §9.

**7.2 — Il documento roadmap unico da creare.** §30 chiede: *«Se non esiste una roadmap editor consolidata,
crearne UNA soltanto»*. Ne esisteva una, [`roadmap-editor.md`](../roadmap-editor.md), ed è `HISTORICAL` dal
2026-08-08 — ritirata perché era *«la terza vista di stato da tenere allineata a mano»*. È tornata il
2026-08-10 **generata**: le sedute vivono in `editor-sessions.yaml` e la vista è
`editormap.shortlist.md`, prodotta da `feature_registry.py shortlist`.

Creare oggi «una roadmap editor consolidata» scritta a mano significherebbe **rifare l'errore che quel
ritiro ha corretto**, tre giorni dopo che era stato corretto. Per questo qui non nasce nessun documento di
roadmap: nascono righe in `editor-sessions.yaml` e issue su GitHub, e la vista si rigenera.

---

## 8. Punteggi

Misurati sul prompt **come istruzione per questo repository**, non come memo di design.

| Dimensione | Voto | Evidenza |
|---|---:|---|
| **Chiarezza** | 9/10 | Il workflow di §8 e l'esempio di §14 sono i più concreti arrivati finora; l'attore non si perde mai |
| **Completezza** | 6/10 | Nessuna mappatura verso owner esistenti; la baseline di §4 non ha una mappa su cui essere validata |
| **Testabilità** | 6/10 | §14/§15 sono falsificabili e quattro test su sei mancano davvero; §32 non ha un criterio verificabile |
| **Coerenza** | 5/10 | §9 vieta di ricreare l'editor e §12/§17/§18 lo ricreano; §26 vieta i duplicati e §30 ne chiede uno |
| **Allineamento al canone** | 5/10 | 8 duplicati e 6 conflitti su 32 voci — ma la tesi centrale è **compatibile** e risolve l'obiezione che aveva fermato il predecessore |

Il voto di allineamento sale rispetto al 3/10 del brief precedente per una ragione sola e vale scriverla: la
quantizzazione di §3–§4 non è una variante estetica della proposta vecchia, è la **sua correzione**.

---

## 9. Cosa fare — e cosa non fare

**Non fare**, in nessun ordine: creare un documento di roadmap editor scritto a mano (§30); introdurre
priorità `P1.1`…`P2.5` (§27–§28); creare `UDeveloperSettings` per tre soglie (§15); scrivere scenari
`MapSketch_*` in `Scenarios/` (§21); costruire un `LOW WALL` che non cuoce in `FRTHexCover{Low}` (§12);
riscrivere camera, toolbar o tool che esistono (§12, §17, §18).

**Le issue aperte da questo referto** — cinque, nell'ordine in cui pagano. Tutte agganciate a
`RT-FEAT-TOOL-MAP-GEOMETRY` (nuova) o `RT-FEAT-TOOL-MAP-EDITOR`, e collegate all'epic
[`#324`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324) (E23) come **anticipazione
dichiarata**, non come sua sostituzione.

| # | Issue | Cosa | Gate d'uscita |
|---|---|---|---|
| 1 | [`#619`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/619) | `P1`+`P2` — maschera a 12 settori, `CoreBlocked`, classificazione, **e il consumatore di `Constrained`** | I quattro test di §22 che mancano sono verdi, e `Constrained` cambia un risultato osservabile |
| 2 | [`#620`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/620) | `P3` — grammatica quantizzata e validator delle direttrici | Un segmento fuori grammatica è **rifiutato da un test**, non da una convenzione |
| 3 | [`#621`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/621) | `P5` — cottura geometria → `FRTHexCover` / `bBlocksMovement` | La cottura è una funzione pura del modulo runtime, con fixture in `Scenarios/Spec/Map/` |
| 4 | [`#622`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/622) | `P4` — griglia di lavoro dove le celle non esistono | Si vede dove si sta per disegnare, fuori dal bordo della mappa |
| 5 | [`#623`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/623) | `P6` — seduta: luci e inquadratura in `L_DevSandbox` | Voci `PIE-*` con esito reale, seduta in `editor-sessions.yaml` |

**`P7` non è aperta da questo referto** ed è la sola eredità che resta scoperta: la sonda di movimento del
panel 2026-08-09. Non la apro qui perché non appartiene a questo prompt e aprirla di straforo ripeterebbe il
difetto al contrario — una issue senza il documento che la motiva. È registrata in §1.1 e nel referto che la
propose.

**La `NEXT ISSUE` che §29 chiede è una sola**: [`#619`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/619).
È l'unica interamente headless, non tocca l'editor, e ogni altra della lista la usa.

### 9.1 `#619` rivista lo stesso giorno, e due decisioni in più

`#619` è passata da un secondo spec panel il 2026-08-12, prima che qualcuno la implementasse. Otto rilievi;
due erano decisioni dell'autore e sono state prese, e cambiano il **modello**, non solo il testo:

- **`D1` — il confine `#619`/`#621` è per campo.** Il **costo** è di `#619`, i **bordi** (`FRTHexCover`,
  `bBlocksMovement`) di `#621`. Serviva perché la prima stesura metteva la cottura fuori scope e ne chiedeva
  l'effetto nella DoD: `ReachableCells` e `FindPath` leggono solo `FRTHexCellData`, quindi senza un campo
  scritto `Constrained` restava non osservabile — cioè il dato senza consumatore che l'issue voleva evitare.
- **`D2` — il sovrapprezzo di `Constrained` non vive in `MoveCost`.** Quel campo ha già un produttore che lo
  ricalcola dalla sola `Surface` ogni turno — `ARTTurnManager::ApplyDynamicSurface` quando la superficie
  cambia, `TickDynamicSurfaces` quando scade: una superficie dinamica che nasce e si ripristina su una cella
  stretta ne cancellerebbe il sovrapprezzo per il resto della partita.
  È un intero suo, sommato dai lettori del costo — quindi `FormatVersion` da 6 a 7 e un campo in più
  in `RTMatchStateHash`.

Un terzo rilievo cade su `#621` e la riga della tabella qui sopra lo porta ancora: **le fixture di geometria
non vanno in `Scenarios/`**. `FRTScenarioCell` porta `Cell`, `bBlocksMovement`, `bBlocksLineOfSight` e
`MoveCost`, e nessun campo per segmenti o footprint; uno scenario è una partita. In `Scenarios/Spec/Map/` va
**uno** scenario — `Spec.Map.ConstrainedCellCostsMore`, quello che mostra l'effetto osservabile, dichiarato
`planned` nel registry perché resti visibile come warning finché non è un file — e le fixture sono dati di
test del modulo runtime.

⚠️ Il gate di `#619` in tabella dice «i quattro test di §22 che mancano sono verdi». Resta vero come
intenzione, ma la DoD in vigore è quella **nel corpo della issue**, non questa riga: dopo la revisione ha
sedici voci, e tre di quelle nuove (`FormatVersion`, ciclo `SurfaceChanged → Cleanup`, soglia canonica nel
cuocere) non sono deducibili da qui.

**Decisioni che aspettano l'autore: una.** `MSE-1` — l'invertibilità della cottura (§4.2), registrata in
[`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).

---

## 10. Rapporto con gli altri documenti

| Documento | Ruolo rispetto a questa revisione |
|---|---|
| [`../../archive/src/handoff/2026-08-12-map-sketch-editor.md`](../../archive/src/handoff/2026-08-12-map-sketch-editor.md) | Il sorgente revisionato — **provenienza, non regola** |
| [`map-editor-brief-spec-panel-2026-08-09.md`](map-editor-brief-spec-panel-2026-08-09.md) | **Il predecessore**: stessa tesi, stesso metodo, verdetto ⛔. Le sue sei `PROPOSED` mai aperte sono §1.1 |
| [`triage-grid-geometry-water-2026-08-10.md`](triage-grid-geometry-water-2026-08-10.md) | Terzo sorgente sullo stesso perimetro; `GEO-1`…`GEO-3` |
| `../feature-registry.yaml` | Owner dello stato di ogni feature citata qui |
| [`../roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) | Owner di **E23**, che §4.1 anticipa in parte |
| [`../roadmap-checkpoint.md`](../roadmap-checkpoint.md) | Owner di **M9**, dove vive il residuo dell'editor |
| `../editormap.shortlist.md` · `editor-sessions.yaml` | La vista **generata** delle sedute — §7.2 |
| [`../../technical/tooling/brief-editor-map-viz.md`](../../technical/tooling/brief-editor-map-viz.md) | Owner della visualizzazione in editor — §23 del prompt è suo |
| [`../../technical/tooling/scenario-map.md`](../../technical/tooling/scenario-map.md) | Owner della ripartizione automatico/umano che §21 ignora |
| [`../../technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md) | Registro delle voci `PIE-*` — dove finiscono davvero i tre «scenari» di §21 |
