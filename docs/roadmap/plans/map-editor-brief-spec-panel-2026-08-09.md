# Map Editor Roadmap Brief — spec panel

> `CURRENT` · **Stato**: revisione chiusa, **nessuna modifica applicata** al canone · **Data**: 2026-08-09
> **HEAD della revisione**: `748b4b0`
> **Sorgente revisionata**: `todo/RefactorTactics_MapEditor_Roadmap_Consolidation_Claude.md` (1919 righe,
> untracked), archiviato a fine sessione in
> [`../../archive/src/handoff/2026-08-09-map-editor-roadmap.md`](../../archive/src/handoff/2026-08-09-map-editor-roadmap.md)
> **Scopo**: classificare ogni affermazione del brief contro il repository **prima** che qualcuno la applichi a
> Feature Registry, roadmap, epic, scenari o codice.
> **Regola applicata**: un handoff AI è l'ultima fonte della gerarchia. Dove contraddice un ADR, una `D-0xx` o
> un'epic già assegnata, prevale il canone e la proposta si **registra**, non si applica.
> **Cosa non è**: un piano di implementazione. Le epic e i checkpoint qui non esistono; §7 dice quale sarebbe
> la prima fetta reale, e si apre solo se l'autore la vuole.

---

## 1. Il verdetto in una riga

Il brief è un **buon memo di design** e una **cattiva istruzione per il repository**: descrive con precisione
un modello di editor-come-client-della-simulazione che il progetto condivide già, e poi chiede di costruirlo
in namespace paralleli (5 roadmap di versione, 5 epic, 61 issue candidate, 29 feature, 15 ScenarioId) che
sono esattamente il registry parallelo vietato dalla sua stessa §Purpose.

Sotto la superficie il conto è questo:

| | Voci | Significato |
|---|---:|---|
| `CURRENT` | **14** | il brief riporta correttamente il canone (spesso senza sapere che è canone) |
| `DUPLICATE` | **9** | ridefinisce qualcosa che ha già un owner e un formato serializzato |
| `CONFLICT` | **5** | contraddice una decisione accettata o un'epic già assegnata |
| `PROPOSED` | **6** | idea nuova, nessun conflitto: si registra o si costruisce |
| `STALE` | **2** | usa una formulazione superata da lavoro più recente |

**Una sola cosa del brief non esiste da nessuna parte ed è costruibile domani**: la sonda di movimento
nell'editor (§9–§13). Vale un checkpoint, non cinque release.

---

## 2. Il panel

Sei revisori, un focus ciascuno. Le citazioni sono ricostruzioni della metodologia, non attribuzioni reali.

### 📋 WIEGERS — qualità dei requisiti

> «§33 elenca dodici voci di Definition of Done e **nessuna** è falsificabile. "Uses canonical runtime/core
> map services" — con quale osservazione dimostro che è falso? "Has debug/explainability output" — quanto
> output? Questo progetto ha già risolto il problema in modo migliore di quanto il brief proponga:
> `feature-registry.yaml` ha nove gate con valori `done|partial|todo|na`, e
> `scripts/feature_registry.py validate` **fallisce** se lo stato dichiarato non regge i gate. Una DoD in prosa
> accanto a un gate verificato da una macchina non aggiunge rigore: aggiunge una seconda verità, e sarà quella
> in prosa a essere citata quando farà comodo.»

> «Seconda osservazione, più seria. §5 fissa gli stati della porta a `Open` / `Closed` e scrive "No
> percentage-based logical opening". La regola contro le percentuali è giusta ed è già rispettata. Ma il
> repository non ha due stati: ne ha **quattro**, e i due che il brief cancellerebbe portano ciascuno una
> regola scritta — `Locked` distingue *chi* può riaprire (serve all'apertura autorizzata di CP 10.1),
> `Destroyed` è **terminale** (una porta sfondata non si richiude). Semplificare un enum già in produzione non
> è ridurre lo scope: è perdere due requisiti, e per giunta in silenzio.»

### 🎯 COCKBURN — attore primario e obiettivo

> «Chiedo la domanda di sempre: *chi* è l'attore, e *quale* obiettivo raggiunge? Il brief risponde bene una
> volta — §9, il level designer che vuole sapere se la mappa è attraversabile come intende — e da lì in poi
> l'attore sparisce. §24 dichiara quindici scenari senza dire chi li esegue; §26 elenca sessantuno issue senza
> un solo goal di stakeholder.»

> «Questo repository quella domanda l'ha già formalizzata, e da un mese: `scenario-map.md` classifica ogni
> verifica in **A** (la macchina esegue e giudica), **B** (la macchina esegue, l'umano giudica), **C** (solo
> umano), **D** (dichiarato ma bloccato). Uno scenario che richiede di aprire l'editor, cliccare e guardare un
> overlay è **classe C**, e la classe C è l'unica che costi tempo all'autore: oggi sono 95 voci su 116. I
> quindici `MAP-ED-*` del brief nascono quasi tutti in C. Dichiararli senza dirlo significa promettere
> copertura e consegnare arretrato manuale.»

### 🏗️ FOWLER — confini e responsabilità

> «§1.3 è la tesi centrale del documento: *"Walls are not constrained to the edges of the hex grid"*, e da lì
> derivano tre effetti logici — occupazione, transizione, LOS — da una geometria world-space. È un modello
> coerente. È anche il modello che questo progetto ha **già valutato e collocato altrove**: E23.1, in v0.2, si
> chiama *Separazione geometria/logica* e ha come DoD misurabile "la logica di transizione non legge la mesh:
> legge archi e stati". Il brief non propone un'alternativa a E23: propone la sua negazione, senza sapere che
> esiste.»

> «C'è una lettura che salva tutto, e vale la pena scriverla perché è la sintesi utile: un muro disegnato in
> world-space è legittimo **come gesto di authoring** se il suo effetto viene *cotto* nei dati che già
> esistono — `bBlocksMovement` sulla cella, `FRTHexCover` o `FRTHexDoor` sul bordo — e se dopo la cottura la
> geometria è arte. Autore disegna una linea, l'editor produce dati canonici, il runtime non sa che esistesse
> una linea. Questo è compatibile con E23.1. Quello che non è compatibile è un `Walls[]` autorevole dentro la
> mappa, interrogato a runtime per decidere una transizione.»

### 🛡️ NYGARD — modi di guasto e invarianti

> «Guardo dove il sistema si rompe. `hex-map-roadmap.md` §Invarianti dichiara *"no float in coord/hash"* e
> *"dati autorevoli indipendenti da Actor/mesh"*; `URTHexMapAsset` è a `FormatVersion=4` e il suo hash entra
> nella verifica di replay (`ReplayDivergenceZero`, KPI dichiarato). §16 del brief propone un
> `RTMapDefinition` con dentro `Walls[]`. Un segmento di muro ha estremi in virgola mobile. Se entra nella
> mappa autorevole, entra nell'hash; se entra nell'hash, due macchine che arrotondano diversamente producono
> due replay diversi, e il KPI che oggi è verde diventa un guasto intermittente che nessuno saprà attribuire.»

> «La versione difendibile è la conversione: gli estremi float restano nel dato di **authoring**, ciò che
> entra nell'asset è il risultato intero della cottura. E la conversione va testata come una migrazione, non
> come una funzione — scrivi con il binario vecchio, rileggi con il nuovo, confronta un digest dei soli campi
> vecchi.»

> «Un secondo punto, meno vistoso. §16 dice *"The `.umap` must not be the only source of competitive truth"*.
> Non lo è, e non lo è mai stato: `URTHexMapAsset` esiste, ha storage stabile, ordinamento, hash, validator e
> quattro versioni di formato alle spalle. La frase giusta è "estendere l'asset a v5", non "convergere su una
> definizione di mappa": la seconda formulazione autorizza chi la legge a scrivere un formato nuovo.»

### 🔗 HOHPE — flusso, revisioni, invalidazione

> «§8 disegna la catena giusta — cambio di stato → revisione → invalidazione cache → ricalcolo — e §13 la usa
> come percorso d'accettazione dell'editor. Questa catena **esiste**: `FRTHexSnapshot` porta hash e revisione
> della mappa, `IsSnapshotStale` e `ValidateSnapshot` sono i punti di controllo, e il resolver si rifiuta di
> girare su uno snapshot vecchio. Quello che manca non è la catena: è un **consumatore d'editor** che la
> attraversi e la renda visibile. È una differenza importante, perché decide se il lavoro è "costruire un
> sistema di invalidazione" (settimane) o "far vedere una revisione che già cambia" (ore).»

### 🧪 CRISPIN — strategia di test

> «§25 chiede tredici test di core: `CellId equality/hash`, `world ↔ grid mapping`, `graph neighbor validity`,
> `movement cost lookup`, `bounded reachable-area query`, `A* point-to-point`, `stable tie-break`,
> `GraphRevision invalidation`, `door state transition`, `map validation`. **Undici su tredici esistono già**,
> e non a campione: `RefactorTactics.HexMap.*`, `HexSim.*`, `Hex.WorldToLayer`, `HexMap.Validate*`,
> `RTHexDoorTests`. Chiedere di crearli produrrà o duplicati, o una sessione persa a scoprire che c'erano.»

> «I due che mancano nell'elenco non sono lacune di copertura: `wall-derived transition blocking` non esiste
> perché **i muri non esistono** (vedi C2), e `movement profile restrictions` è la collisione di nome di C4 —
> il vincolo per profilo d'azione c'è (`Terrain.Rough.BlocksDash`), quello per archetipo d'unità è una feature
> che nessuno ha deciso di volere. Nessuno dei due si risolve scrivendo un test.»

> «Il buco vero il brief non lo elenca affatto, ed è a portata di mano: una **golden fixture di area
> raggiungibile** — mappa fissa, budget, insieme atteso di celle con il loro costo. Oggi il determinismo
> dell'area raggiungibile è garantito dalla *struttura* del codice — costi interi, tie-break assoluto — ma non
> c'è un file che dica *quale* sia la risposta giusta. Con quello, la sonda di §9 nasce già verificabile invece
> che verificata a occhio.»

> «Nota di metodo su §25: *"Do not make screenshot comparison the primary correctness mechanism"* è corretto e
> il progetto già lo applica — la classe B di `scenario-map.md` esiste proprio per dire che l'assertion resta
> anche quando l'oracolo è un occhio.»

### 📐 ADZIC — esempi eseguibili

> «Gli esempi del brief sono la sua parte migliore. §12 mostra una cella con `Total Cost: 4` e la
> scomposizione `Concrete +1 / Water +2 / Concrete +1`; §12 mostra un blocco con `Edge: (4,-2,0) -> (5,-2,0)`
> e `Source: Wall_013`. Questo è esattamente il livello di concretezza che rende una specifica verificabile.»

> «E proprio per questo si vede cosa manca: **nessun esempio ha una mappa**. "Concrete +1, Water +2" non è
> eseguibile finché non esiste il file che contiene quelle celle. Il progetto ha il posto dove metterlo —
> `Scenarios/Spec/Map/` — e una convenzione di identità che il brief non usa: gli ScenarioId sono puntati
> (`Spec.Cover.TemporaryCoverExpires`, `Visual.Core.PhaseOrder`), non `MAP-ED-001`. Cinque convenzioni di ID
> diverse in un pacchetto sono già costate un audit a questo repository il 2026-08-09; questa sarebbe la
> sesta.»

> «Ultimo: `Concrete` e `Water` non sono superfici di questo gioco. Le otto sono `Floor`, `Rough`,
> `ShallowWater`, `Fire`, `Conductive`, `Smoke`, `Ice`, `HighGround`, fissate in CP 8.1 con i costi e le
> regole. Un esempio scritto in un vocabolario che non esiste non si può eseguire nemmeno a mano.»

---

## 3. `CONFLICT` — le cinque voci che contraddicono il canone

| # | §  | Il brief dice | Il canone dice | Esito |
|---|---|---|---|---|
| **C1** | §17, §23, §26, §28 | Cinque release di Map Editor (v0.1–v0.5), 5 epic, 61 issue, 29 feature entry | La numerazione delle epic è **unica e condivisa** (E1–E35, assegnata **al merge** — [D-039](../../decisions/RT_PDR_00_Decision_Log.md)); lo stato di una feature vive **solo** in `feature-registry.yaml`, verificato da `scripts/feature_registry.py validate` | **Non applicabile come scritto.** Un "Map Editor v0.1" accanto alla release v0.1 è ambiguo al primo riferimento incrociato |
| **C2** | §1.3, §4 | I muri sono geometria world-space e **derivano** occupazione, transizione e LOS | **E23.1** (v0.2): «la logica di transizione **non legge la mesh**: legge archi e stati». [`hex-map-roadmap.md`](../hex-map-roadmap.md): *no float in coord/hash*, *dati indipendenti da Actor/mesh* | **Ammissibile solo come authoring che cuoce dati.** Vedi §4 |
| **C3** | §5 | Porte `Open` / `Closed` | `ERTHexDoorState{Open, Closed, Locked, Destroyed}` + `DoorId` per il portone atomico ([`spec-porte-cp93.md`](../../gameplay/spec-porte-cp93.md), E23.2) | **Regressione.** Perde l'apertura autorizzata (CP 10.1) e la terminalità |
| **C4** | §11 | `MovementProfileId ∈ {Standard, Heavy, Agile}` | `RT-FEAT-ACTION-MOVE-PROFILES` = **Move, Sprint, Charge** — profilo dell'**azione**, con [D-015](../../decisions/RT_PDR_00_Decision_Log.md) «Sprint non è Dash» | **Collisione di nome su assi diversi.** L'asse archetipo dell'unità è legittimo, il nome no |
| **C5** | §3 | Roster terreni `Concrete / Metal / Dirt / Grass / Water / Ice / Rubble / Default` | Le otto superfici di **CP 8.1** ([`spec-terreni-e8.md`](../../gameplay/spec-terreni-e8.md)) con costi, regole e test | **Vocabolario inesistente.** §3 dice «should match the current project vocabulary» e poi non lo fa |

---

## 4. La domanda dei muri, discussa

Merita più di una riga di tabella, perché è l'unico punto in cui il brief propone qualcosa che il canone non
ha e la proposta **non è sbagliata** — è collocata male.

**Quello che il brief coglie**: un muro architettonico reale non rispetta i sei lati di un esagono. Un angolo
a 90° attraversa le celle come gli pare, e obbligare il level designer a pensare per bordi produce mappe che
sembrano fatte di alveari. È un'osservazione di produzione, e viene da chi disegnerebbe le mappe.

**Quello che il brief rompe**: se il muro resta autorevole, ogni query di transizione deve intersecare
geometria. Con estremi float. Dentro un hash che oggi tiene fermo il KPI *replay divergence = 0*.

**La sintesi che regge entrambe le cose** — ed è la raccomandazione del panel:

```
authoring          cottura                     dato autorevole
─────────          ───────                     ───────────────
segmento           il tool decide una volta:   FRTHexCellData.bBlocksMovement
world-space   ──►  quale cella, quale bordo,  ──►  FRTHexCover{Edge, High}
(float)            quale effetto                   FRTHexDoor{Edge, State}
                                                   (interi, hashabili)
```

Il muro diventa un **sorgente di authoring versionato accanto alla mappa**, non un campo dentro di essa. Il
runtime continua a non sapere che esista. E la cottura si testa come una migrazione di formato: si scrive col
binario vecchio, si rilegge col nuovo, si confronta un digest dei soli campi vecchi.

Resta un costo che va detto invece che nascosto: **la cottura non è invertibile senza conservare il sorgente**.
Se qualcuno modifica a mano `bBlocksMovement` su una cella cotta, il muro e la mappa divergono e il prossimo
ricalcolo cancella la modifica. È la stessa classe di problema dei prefab, e va deciso — non scoperto.

> **Collocazione.** Questo lavoro *è* **E23**, in **v0.2**, e nasce da un sorgente già consumato:
> [`2026-08-08-muri-porte-e-interazioni.md`](../../archive/src/design/2026-08-08-muri-porte-e-interazioni.md).
> Il brief lo chiama "Map Editor v0.1". Anticiparlo significherebbe costruire l'authoring dei muri prima che
> E9 (coperture e strutture, oggi **P2 e aperta**) abbia verificato i bordi su cui i muri dovrebbero cuocere.

---

## 5. `DUPLICATE` — nove cose che hanno già un owner

| § | Il brief chiede di creare | Esiste già come |
|---|---|---|
| §2, §14 | Shell dell'editor, toolbar, overlay di debug | `URTHexEditorMode` + tool Select/Paint/Fill/Arch, `BrushRadius`, `DrawSurfaceOverlay` — `RT-FEAT-TOOL-MAP-EDITOR`, **TESTABLE**, M9 CP 9.1 |
| §10 | Query di area raggiungibile a costo limitato | `URTHexSimLibrary::ReachableCells` — Dijkstra a costi interi, archi e occupanti inclusi (H6.1) |
| §10 | Path punto-a-punto autorevole | `URTHexPathLibrary::FindPathAvoiding` / `FindPathForUnit` (H3, H6.2) |
| §6 | Copertura bassa/alta, direzionale, con metadati di distruttibilità | `FRTHexCover{Edge, Type, Integrity}` — sparso, per bordo, integrità 30/50 (CP 9.1, 9.2, 9.5) |
| §5, §8 | Porte come interattivi che modificano il grafo | `FRTHexDoor` sui bordi + `SetDoorState` (CP 9.3) |
| §7, §8 | Palette di interattivi, valvola/generatore/serbatoio/relè | [`spec-interazioni-mappa-cp101.md`](../../gameplay/spec-interazioni-mappa-cp101.md) — owner della grammatica (CP 10.1) |
| §15 | Validator di mappa | `URTHexMapAsset::ValidateMap` + `RefactorTactics.HexMap.Validate*` — `RT-FEAT-TOOL-VALIDATION`, **DONE** |
| §16 | Definizione di mappa versionata con hash e ID stabili | `URTHexMapAsset`, `FormatVersion=4`, hash nel replay |
| §18 | LOS, facing a sei direzioni, geometria di Overwatch | `URTHexVisionLibrary::HasLineOfSight`, `HexLine`, `HexCone` (H6.4); facing = [ADR-0005](../../decisions/adr-0005-orientamento.md) |

Undici dei tredici test chiesti da §25 esercitano codice di questa tabella, o le fondazioni H0 su cui poggia.

---

## 6. `PROPOSED` — le sei cose davvero nuove

Nessuna contraddice il canone. Non sono ordinate per valore.

| # | Cosa | Perché regge |
|---|---|---|
| **P1** | **Sonda di movimento nell'editor** (§9, §12, §13) — piazza un'unità, vedi area, costi, percorso, motivo del blocco, e vedi cambiare quando dipingi | È l'unica cosa del brief che non esiste in nessuna forma, e poggia interamente su servizi già verdi |
| **P2** | **`Parent` in `FRTHexReachableCell`** (§10) | Oggi la struct è `{Cell, Cost}`: l'area si può disegnare ma il percorso verso una cella hovered va ricalcolato con A\*. Il campo genitore è ~10 righe di Dijkstra e rende la ricostruzione O(lunghezza) |
| **P3** | **Reason code diagnostici della query** (§12) | *Perché* una cella non è raggiungibile è una domanda che oggi nessuna API risponde. ⚠️ **Non** vanno confusi con `ERTMoveOutcome`, che è **serializzato nei replay**: vedi §7 |
| **P4** | **Golden fixture di area raggiungibile** (§25) | Mappa fissa + budget + insieme atteso con costi. Il determinismo oggi è strutturale, non attestato da un file |
| **P5** | **Statistiche e analisi dei choke point** (§21, §22) | Diagnostica di design, nessuna regola di gioco. Il brief lo etichetta correttamente come «designer assistance, not authoritative balance proof» |
| **P6** | **Sonda di rumore sul grafo** (§20) | `RT-FEAT-PERCEPTION-NOISE` esiste, un modo di **guardarlo** no. La propagazione sul grafo invece che a sfera è già la regola |

---

## 7. Due trappole già viste, che il brief ripete

Non sono errori del brief: sono errori che questo repository ha già pagato e catalogato, e che il brief
ricrea perché non poteva saperlo.

**7.1 — Il secondo vocabolario di reason code.** §12 propone sette codici: `MovementBudgetExceeded`,
`CellBlocked`, `TransitionBlocked`, `DoorClosed`, `ProfileRestriction`, `InvalidCell`, `NoPath`. Il progetto
ne ha già sette in `ERTMoveOutcome` — `Stayed`, `Moved`, `BlockedContested`, `BlockedByUnit`,
`BlockedByPriority`, `BlockedByImpact`, `BlockedByTopology`.

Le due liste **rispondono a domande diverse**, e questa è la parte che va scritta prima che qualcuno le
unisca: `ERTMoveOutcome` dice *com'è andata la risoluzione di un movimento pianificato* ed è **serializzato
nei replay** (aggiungere un valore in coda è una migrazione, riordinarli è una rottura). I codici di §12
dicono *perché una cella non è raggiungibile in una query di pianificazione*, e non devono finire nel TurnLog.

La stessa trappola è già registrata in [`spec-tassonomia-movimento.md`](../../gameplay/spec-tassonomia-movimento.md)
§36: «dieci reason code nuovi → **duplicati**: sette esistono con altri nomi, in un enum serializzato nei
replay». È la seconda volta.

**7.2 — Il terreno che contiene lo stato transitorio.** §3 del brief è netto: *base surface* e *runtime state*
non si combinano, altrimenti nasce `MetalWetElectrified`. La regola è giusta, e questo referto — nella sua
prima stesura — l'aveva usata per accusare il repository di violarla, perché `ERTHexSurface` contiene `Fire` e
`Smoke` mentre CP 8.2 ha `Burning` e `Obscured`.

> ⚠️ **Corretto il 2026-08-09, prima che qualcuno ci lavorasse sopra.** L'accusa era **falsa**, e lo si è
> scoperto guardando dove ciascun valore è memorizzato invece di confrontare due elenchi. Vedi
> [`D-059`](../../decisions/RT_PDR_00_Decision_Log.md).

Il modello ha **tre** strati, non due:

| Strato | Dove vive | Contiene |
|---|---|---|
| Superficie **corrente** della cella | `FRTHexCellData.Surface` (asset mappa) | `Fire`, `Smoke`, `ShallowWater`… — ciò che *tutti* leggono già: costi, Dash, ghiaccio, targeting, on-enter, conduttività |
| Superficie **originale** + scadenza | `ARTTurnManager::DynamicSurfaces{Original, TurnsRemaining}` | stato **di partita**, non dato di mappa |
| Stato dell'**unità** | `ARTUnit::StatusTurns` | `Status.Burning`, `Status.Obscured`, `Status.Wet` |

`Fire` non *è* `Burning`: è la superficie che lo **produce** su chi entra
(`MakeTerrain(Fire, …, {Damage 10, Status Burning 2})`). Cella e unità sono due oggetti, non due nomi per la
stessa cosa — e la separazione base/transitorio che il brief chiedeva di introdurre **esiste già**, sul terzo
strato: `Action.Ignite` mette `Fire` per due turni ricordando `Original`, `Action.CreateWater` lo spegne.
`MetalWetElectrified` non può nascere perché la composizione avviene **fra strati**, mai dentro un enum.

`RTHexCellData.h` e `RTTurnManager.h:246` argomentano per giunta *contro* la correzione che l'accusa
implicava: un campo `BaseSurface` nella cella sarebbe «un secondo posto da consultare, cioè un secondo modello
di verità», e la scadenza è stato di partita — «due partite sulla stessa arena non devono ereditarsi il
fuoco».

**La lezione è per chi scrive referti, non per il brief.** Il difetto di §3 e quello di §12 sono lo stesso:
confrontare due *documenti* invece di guardare il codice. Il brief lo fa perché non ha il repository; io l'ho
fatto avendocelo.

---

## 8. Punteggi

Misurati sul brief **come istruzione per questo repository**, non come memo di design.

| Dimensione | Voto | Evidenza |
|---|---:|---|
| **Chiarezza** | 8/10 | Prosa non ambigua, diagrammi leggibili, distinzioni nette (§10 area vs percorso è ineccepibile) |
| **Completezza** | 6/10 | Nessuna mappatura verso owner esistenti; nessuna soglia numerica; §24 non dice chi esegue |
| **Testabilità** | 4/10 | §33 non ha un criterio falsificabile; §24 non ha fixture; gli esempi di §12 non hanno una mappa |
| **Coerenza** | 4/10 | La §Purpose vieta i registry paralleli, §26/§28 li costruiscono; §5 semplifica un enum in produzione |
| **Allineamento al canone** | 3/10 | 9 duplicati e 5 conflitti sulle 36 voci classificate; il vocabolario dei terreni non esiste |

Come **memo di design** il voto sarebbe altro — la tesi «l'editor è un client dei servizi autorevoli, mai una
seconda implementazione» è quella giusta, ed è la ragione per cui vale la pena archiviarlo invece di
scartarlo.

---

## 9. Cosa fare — e cosa non fare

**Non fare**, in nessun ordine: creare epic `Map Editor v0.1…v0.5`; aprire 29 voci nel Feature Registry;
scrivere ScenarioId `MAP-ED-*`; introdurre `RTMapDefinition`; semplificare `ERTHexDoorState`; ribattezzare le
superfici.

**La fetta reale**, se e quando l'autore la vuole, è una sola e sta dentro un checkpoint:

> **Sonda di movimento nell'editor** — un tool nell'Editor Mode esistente che piazza un'origine, chiama
> `ReachableCells` con un budget, disegna area e costi sull'overlay che già esiste, ricostruisce il percorso
> verso la cella sotto il cursore, e mostra la revisione della mappa cambiare quando si dipinge o si commuta
> una porta.

Dipendenze: **nessuna nuova**. Tocca: `FRTHexReachableCell` (+`Parent`), un `URTHexProbeTool`, un reason code
di query non serializzato. Collocazione naturale: **M9 CP 9.1**, dove il residuo dell'editor è già tracciato.

Le altre cinque `PROPOSED` non hanno fretta e due (P5, P6) non hanno nemmeno un consumatore: registrarle come
idee è più onesto che pianificarle.

**Decisioni che aspettano l'autore: nessuna.** Questa sezione ne dichiarava una — `MED-1`, «`Fire` e `Smoke`
sono superfici o stati?» — ed è stata **chiusa lo stesso giorno come mal posta**: vedi §7.2 e
[`D-059`](../../decisions/RT_PDR_00_Decision_Log.md). Il brief non lascia dietro di sé alcuna scelta di
modello che il repository non abbia già fatto.

Resta invece un **buco di implementazione**, che non è una decisione: `Hero.Phase.MistVeil` dichiara «crea fumo
raggio 1» e non lo fa — `Smoke` è l'unica superficie del catalogo che nessuna azione sa creare
(`bCreatesSurface` assente, `Range 0` dichiarato segnaposto). Il meccanismo esiste già ed è usato da `Ignite`
e `CreateWater`: manca il collegamento. Tracciato nella issue `#353`, non qui.

---

## 10. Rapporto con gli altri documenti

| Documento | Ruolo rispetto a questa revisione |
|---|---|
| [`../../archive/src/handoff/2026-08-09-map-editor-roadmap.md`](../../archive/src/handoff/2026-08-09-map-editor-roadmap.md) | Il sorgente revisionato — **provenienza, non regola** |
| `../feature-registry.yaml` | Owner dello stato di ogni feature citata qui |
| [`../roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) | Owner di **E23** — muri, porte e interaction graph (v0.2) |
| [`../roadmap-checkpoint.md`](../roadmap-checkpoint.md) | Owner di **M9**, dove vive il residuo dell'editor |
| [`../../gameplay/spec-porte-cp93.md`](../../gameplay/spec-porte-cp93.md) · [`spec-terreni-e8.md`](../../gameplay/spec-terreni-e8.md) · [`spec-interazioni-mappa-cp101.md`](../../gameplay/spec-interazioni-mappa-cp101.md) | Owner delle tre aree che il brief ridefinisce |
| [`../../technical/tooling/scenario-map.md`](../../technical/tooling/scenario-map.md) | Owner della ripartizione automatico/umano che §24 ignora |
| [`consolidamento-chat-openai-triage-2026-08-09.md`](consolidamento-chat-openai-triage-2026-08-09.md) | Triage gemello, stesso metodo, sorgente diverso |
