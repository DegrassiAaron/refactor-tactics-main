# Spec — Conoscenza parziale visibile (presentazione + emissione acustica)

> **Stato**: spec di design, **non** consuntivo · **Data**: 2026-08-26 · **Origine**: `/sc:brainstorm`
> **Cosa è**: come la conoscenza parziale, che come **regola** esiste ed è matura, diventa qualcosa che il
> giocatore **vede**; e come il rumore, che come **propagazione** esiste, comincia a essere **prodotto** in
> partita.
> **Cosa non è**: fog of war sul terreno. Quella è la fetta successiva (§10), e in questa spec il terreno
> resta noto.
> **Autorità**: subordinata a [`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md), al
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md) e a
> [`brief-conoscenza-parziale.md`](../../gameplay/brief-conoscenza-parziale.md), di cui è l'**esecuzione**:
> il brief dice *cosa* deve valere, questa spec dice *come* si costruisce.
> Estende [`h6-4-hex-vision-spec.md`](h6-4-hex-vision-spec.md) e consuma
> [`progettazione-hud.md`](progettazione-hud.md) §9, §25 e §26.

> 📍 **Ogni numero e ogni assenza in questo documento è stato misurato il 2026-08-26 su `main`**, e il
> comando è scritto accanto all'affermazione. Le misure invecchiano: prima di usarne una come premessa,
> rieseguila.

---

## 1. Stato misurato

### 1.1 La regola esiste, ed è matura

| Pezzo | Owner | Stato |
|---|---|---|
| Linea di vista geometrica | `URTHexVisionLibrary::HasLineOfSight` | ✅ con regola d'elevazione e copertura alta sul bordo attraversato |
| Vista a cono + consapevolezza a 360° entro 2 celle | `URTPerceptionLibrary::VisibleCells` | ✅ ADR-0005; il cap del fumo riusa `URTTerrainLibrary::EffectiveTargetingRange` |
| Conoscenza di squadra e memoria del contatto | `FRTTeamKnowledge`, `FRTLastKnownContact` | ✅ versionata, viaggia nello snapshot |
| Il targeting **decide** | `ARTTurnManager` → `URTTeamKnowledgeLibrary::ClassifyTarget` | ✅ e vale **anche per il giocatore umano**, non solo per il bot |
| Il bot gioca alla pari | `PlanBots` costruisce `Ctx.Enemies` da `FRTTeamKnowledge` | ✅ canary `HexBotPlay.HiddenEnemyFairness` |
| Propagazione del rumore | `URTAcousticPropagationLibrary` | 🟡 propaga e attenua, ma **nessun sistema di gioco la chiama** |

### 1.2 La presentazione non la usa — e non è che non possa

`git grep -c -E "Awareness|TeamKnowledge|VisibleCells|HasLineOfSight"` sui path presentazionali
(`UI/`, `Player/`, `Unit/`, `Camera/`, `Selection/`, `Replay/`, `Map/RTHexMapActor.cpp`) → **zero ovunque**.

🔴 **Ma la conclusione «la presentazione non ha il dato» è falsa.** `ARTTurnManager::MakeCurrentSnapshot` è
dichiarata a `RTTurnManager.h:495`, sotto il `public:` aperto a riga 177, e restituisce `FRTHexSnapshot`, che
porta `TeamKnowledge` di **entrambe** le squadre. `RTPlayerController.cpp:66` la chiama già, a ogni refresh
dell'anteprima di pianificazione.

**Il client ha già tutto in mano.** Non manca il dato: manca la **porta filtrata**. È la differenza fra
«non lo so» e «lo so e faccio finta di no», e la seconda è ciò che
[`progettazione-hud.md`](progettazione-hud.md) §17 vieta e che l'invariante #6 esclude per costruzione.

### 1.3 Quali canali possono stampare a schermo un fatto che la squadra non conosce

*(Riscritta il 2026-08-28, dopo `#1497`. La stesura precedente si intitolava «Due canali», e la cifra era il
primo dei suoi errori: contava i canali **noti allora**, non quelli esistenti. Le sue due celle «Misura»
citavano inoltre codice che nel frattempo è stato sostituito — l'unico filtro `IsAlive()` di `DrawHUD` e
`GetRecentEvents()` disegnata senza gate — e un «terzo dettaglio» sullo zero cablato che non c'è più.)*

🔴 **Un conteggio di canali non significa nulla senza la domanda a cui risponde**, e due domande vicine
danno numeri distanti un ordine di grandezza:

| Domanda | Che cosa conta | Ordine |
|---|---|---|
| *«quanti elementi disegna `DrawHUD` da dati di unità?»* | nome, barra HP, scudo, anelli, status, waypoint, anteprime, marker di fuoco amico, traccia, sagoma… | **decine**, e cresce a ogni feature di presentazione |
| *«quanti canali possono mostrare un fatto che l'osservatore non conosce?»* | quelli sotto, ciascuno con un owner | **l'elenco che segue**, non una cifra |

Il secondo è quello che questa specifica governa; il primo non è mai stato la domanda. Per questo qui c'è
una **tabella** e non un numero: una cifra in prosa invecchia in silenzio, un elenco no — chi aggiunge un
canale deve aggiungere una riga, e chi ne chiude uno deve cambiarne lo stato.

| Canale | Cosa rivelerebbe | Stato | Misura |
|---|---|---|---|
| `ARTHUD::DrawHUD` — overlay e modello | nome eroe, barra HP e scudo di ogni unità viva | ✅ **chiuso** | `ShouldDrawUnitOverlay` decide, e `if (!bIsKnownToObserver) { continue; }` salta l'unità |
| Combat log | la cella esatta di ogni movimento, e i punteggi di utility del bot con cella e bersaglio | ✅ **chiuso** ([D-223]) | l'HUD chiama `GetRecentEventsForTeam(PlayerTeamId)`; `GetRecentEvents()` non ha **più alcun chiamante fuori dai test** |
| Traccia post-lock | il percorso realmente eseguito da ogni unità, entrambe le squadre | ✅ **chiuso** (`#1497`) | `FRTMoveRoute::CellVerdicts` porta un verdetto **per cella**; `ARTTurnManager::VisibleTrailFor` tronca, e `DrawHUD` disegna solo il tratto che rende |
| Sagoma dell'ultimo contatto | dove un nemico era l'ultima volta | ✅ **per costruzione** | `ContactGhostTargetForUnit` è complementare a `ShouldDrawUnitOverlay`: o l'uno o l'altra |
| `rt.Debug.DrawPaths` | le rotte di entrambe le squadre, cella per cella, **in console** | ⚠️ **aperto e DICHIARATO** | **zero** `#if` nel file (`Debug/RTDebugConsole.cpp`), quindi non è un attrezzo da editor. Resta non filtrato per scelta: chi apre la console possiede già lo stato del client, e il comando stampa accanto il tratto che `VisibleTrailFor` concede — è così che il filtro si verifica |
| **Playback** (`TickPlayback`) | il modello di ogni unità che cammina lungo il percorso eseguito | ✅ **chiuso** (`#1525`) | `BeginPlayback` tronca la rotta al tratto osservato con `URTTeamKnowledgeLibrary::ObservedPrefixLength` — **la stessa funzione** che `VisibleTrailFor` usa per la traccia, non una seconda copia della regola. 🔴 **E con meno di due celle osservate non posiziona nemmeno il modello sulla partenza**: quel `SetVisualLocation` era esso stesso un canale, e rivelava da dove il nemico era uscito proprio quando la destinazione era visibile. ⚠️ **Il campione è quello di [D-223]** — verdetto congelato alla raccolta, osservatori alle celle di inizio fase: risponde bene quando a nascondere è il movimento del NEMICO, sbaglia quando è quello dell'OSSERVATORE |
| **Camera** — pan e focus automatici (`CAM-12`, [#1781](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1781)) | **dove** è successo qualcosa che l'osservatore non conosce: a rivelarlo non è un pixel, è il **movimento** dell'inquadratura | ⏳ **inerte oggi, normato da ora** ([D-287](../../decisions/RT_PDR_00_Decision_Log.md)) | il Director **non esiste** — `ARTCameraPawn::FocusOn` ha oggi consumatori solo manuali, quindi nessun pan automatico può ancora tradire nulla. La regola è scritta **prima** del canale, ed è deliberato: un director costruito sopra `ResolvedTimeline` non filtrata erediterebbe **#1525** invece di evitarlo. ⚠️ **Filtrare in camera NON è il confine**: l'evento non autorizzato non deve arrivare al client, non essere ignorato dopo. ℹ️ Un secondo debito già dichiarato sulla stessa superficie: `ARTCameraPawn::FrameOwnTeam` itera **tutte** le unità e legge `TeamId`/`IsAlive()` anche degli avversari per scartarli ([D-143](../../decisions/RT_PDR_00_Decision_Log.md), owner [`spec-tactical-camera.md`](spec-tactical-camera.md) §1) |

⚠️ **E le due colonne cambiano fra Development e Shipping**, che è la seconda ragione per cui una cifra in
prosa non regge: la riga `rt.Debug.DrawPaths` dipende da come il target tratta i comandi console, non da
questo documento — chi ha bisogno del numero esatto lo misura sul target che gli interessa, invece di
leggerlo qui.

### 1.4 Il substrato non esiste — tre buchi distinti

> ⚠️ Misurati contro un **velo**. Con la fog decisa da
> [D-225](../../decisions/RT_PDR_00_Decision_Log.md) **i primi due cadono**: vedi §5.3.

1. **Il canale**: `Cells->NumCustomDataFloats = 3`, e tutti e tre portano l'RGB della superficie che
   `RebuildInstances` scrive. `M_HexCell` è **Unlit + Opaque**, ha **un solo** nodo
   `MaterialExpressionPerInstanceCustomData3Vector` e scrive sull'**Emissive**. Nessuno dei quattro
   materiali versionati (`M_HexCell`, `M_TeamRing`, `M_SelectionRing`, `M_Global_Tint`) ha un
   `MaterialExpressionScalarParameter`.
2. **Il meccanismo**: `RebuildInstances` è l'**unico** costruttore, ricostruisce tutto da zero, e a runtime
   gira **solo all'allestimento** (`OnConstruction`, `ARTGameMode::ApplyMapSource`, harness scenari).
   `git grep -n RebuildInstances -- Source/RefactorTactics/Turn Source/RefactorTactics/Player` → nessun
   match. Non esiste mappa inversa cella→istanza: `InstanceCells` va da indice a cella.
3. **Il trigger**: `ARTTurnManager` ha **sei** delegate, tutti di playback o fine partita, e **nessuno ha
   subscriber**. Nessuno segnala l'inizio della pianificazione.

Per contro, il **dato** c'è ed è fresco: `RefreshTeamKnowledgeForPlanning` rinfresca **tutte** le squadre e
vive dentro `PlanBots()`, che `StartPlanningTimer` chiama a ogni turno.

### 1.5 Il rumore non ha né produttore né numeri

- `FRTNoiseEvent::Intensity` non è assegnata da nessuna parte fuori dai test.
- `NoiseIntensity` → **zero occorrenze** in `Source/` e in `docs/balance/`. Il campo non esiste su
  `URTActionData`.
- Il catalogo azioni porta la colonna `Rumore` con dei `—`, e scrive che *«un `—` non è "silenzioso": è
  "non ancora deciso"»*. La domanda ha un ID: **`AE-8`**.
- `FRTAcousticContact` → zero occorrenze.
- Il filtro acustico d'uscita non esiste: `RTIntentPrivacyLibrary` espone il solo
  `FilterForTeam(ObserverTeamId, Intents)`.

---

## 2. Le sei decisioni prese in sessione

| # | Decisione | Conseguenza |
|---|---|---|
| **S1** | **Due fette: prima le unità, poi il terreno** | Questa spec è la prima. La fog sul terreno è §10 |
| **S2** | **Vista e rumore nella stessa fetta** | Si tocca il TurnLog: versione di formato **11** e golden da rigenerare |
| **S3** | **`AE-8` si chiude con una regola derivata**, non con sei numeri scelti a tavolino | §6.1. Un solo numero nuovo in tutto |
| **S4** | **Il ricordo è una sagoma semitrasparente dell'eroe** nella cella dell'ultimo contatto | Legge tutti e tre i campi di `FRTLastKnownContact`: chi, dove, quando scade |
| **S5** | **Velo permanente** sulle celle fuori da `VisibleCells`, ottenuto **moltiplicando l'RGB in scrittura** | Nessun `.uasset` da toccare per il velo; il gate dei colori va esteso (§8) |
| **S6** | **Sagoma volumetrica sia per il ricordo sia per l'intento**, separate su **due** canali | Emenda `progettazione-hud.md` §9 e §25 |

---

## 3. La porta filtrata

### 3.1 Forma

Copiata alla lettera da un pattern già vivo e verde nel progetto:

```text
FRTPlannedIntent  →  URTIntentPrivacyLibrary::FilterForTeam(ObserverTeamId)  →  FRTIntentView      ← esiste
FRTKnowledgeSubject[] → URTKnowledgeViewLibrary::ViewForTeam(Knowledge, …)   →  FRTKnowledgeView   ← nuovo
```

**Due livelli, e la ragione è misurata.** Una prima stesura di questa spec faceva prendere alla porta il
`FRTHexSnapshot`. È sbagliato per due motivi indipendenti:

- `MakeCurrentSnapshot` **è costosa e non cacheata** — `GetAllActorsOfClass` su tutto il livello, allocazione,
  copia e **due** `Sort`. `ARTHUD::DrawHUD` gira a ogni frame: quella firma avrebbe messo uno snapshot
  completo dentro il ciclo di disegno.
- `FRTHexSimUnit` **non porta `TeamId`**. Lo snapshot da solo non distingue alleati da nemici, quindi
  servirebbe comunque l'array parallelo di `ARTUnit*` — cioè la firma non era nemmeno sufficiente.

Quindi:

1. **Nucleo puro e headless** — `ViewForTeam(const FRTTeamKnowledge&, const TArray<FRTKnowledgeSubject>&,
   int32 ObserverTeamId)`. `FRTKnowledgeSubject` è un soggetto ridotto a ciò che serve per **decidere se si
   sa**: `StableUnitId`, `TeamId`, `Cell`, chiave visiva. Non è un'unità — prendere `ARTUnit` legherebbe una
   regola pura al mondo di gioco, esattamente come `FRTPerceiver` evita di prendere `ARTUnit`.
2. **Adattatore sottile** — costruisce i soggetti dall'array di `ARTUnit*` che il chiamante **ha già**.

Il nucleo è ciò che il test di D-143 interroga: senza `UWorld`, senza Actor, senza montare una partita.

`ARTHUD.cpp` porta già il commento che descrive la disciplina: *«2. FILTRA per l'osservatore. Da qui in giù
lo stato completo non si tocca più»*. La porta nuova non inventa un principio: ne applica uno che il
progetto ha già scritto e verificato.

### 3.2 Contenuto

Per ogni unità avversaria, **uno di tre casi** — e ciò che distingue i casi è quale campo *non esiste* nel
DTO, mai un flag:

| Caso | Contiene | **Non** contiene |
|---|---|---|
| `Live` | cella attuale, identità | la **condizione** (HP, scudo) |
| `Remembered` | identità, **cella del ricordo** | la cella attuale, la condizione |
| *(ignoto)* | **nessuna voce** | tutto |

> ⚠️ **Corretta il 2026-08-27: questa tabella descriveva un DTO diverso da quello costruito.** Diceva
> `Detected · CellOnly · Hidden` — che sono i nomi di `ERTAwareness` e `ERTTargetKnowledge`, non della vista —
> e attribuiva a `Detected` la **condizione** e a `CellOnly` il **turno di scadenza**. I nomi veri sono
> `ERTKnowledgeVisibility::{Live, Remembered}`, e il terzo caso **non ha un nome** perché non ha una voce.
>
> 🔴 **La correzione stessa è stata corretta il 2026-08-27 (giro di fix), perché una sua metà era falsa.**
> Affermava che `FRTKnowledgeEntry` «non ha né l'una né l'altro»: vero per la **condizione**, che resta
> vietata dal piano («*un campo qui costringerebbe a inventarne il valore*»); **falso per il turno**, che
> `d8fdaed4` ha aggiunto come `ContactTurn` — significativo solo per un `Remembered`, ed è il campo che
> permette alla sagoma dell'ultimo contatto di sapere quando il ricordo scade senza reinterrogare
> `FRTTeamKnowledge`. La tabella qui sopra continua a non elencarlo perché elenca ciò che **identifica** un
> caso; `ContactTurn` non distingue i casi, li accompagna.

🔴 **`Hidden` è l'assenza della voce, non una voce con un flag.** È la stessa disciplina che il DoD di
[#159](https://github.com/DegrassiAaron/refactor-tactics-main/issues/159) impone al filtro acustico
(*«nessuna voce, non una voce vuota»*), e la ragione è che un flag si può leggere per sbaglio, un campo
che non c'è no.

⚠️ **`URTTeamKnowledgeLibrary::AwarenessOfUnit(Knowledge, StableUnitId, CurrentCell)` richiede la cella
ATTUALE del bersaglio.** La firma dice dove passa il confine: chi la chiama tiene in mano la posizione
vera, e quella non deve attraversare la porta. La conversione avviene **dentro** `ViewForTeam`, mai a valle.

### 3.3 Collocazione

`Perception/RTKnowledgeView.{h,cpp}`, accanto a `RTTeamKnowledge`.

- **Mai** dentro `UI/`: la conoscenza è regola, non presentazione.
- **Mai** dentro `RTTurnLog*`: il log è uno solo, è la sorgente di `HashTurnLog`, e non deve conoscere
  osservatori (correzione già registrata due volte, da
  [#295](https://github.com/DegrassiAaron/refactor-tactics-main/issues/295)).

### 3.4 L'osservatore

`ViewForTeam` prende `ObserverTeamId` come parametro.

> ⚠️ **Corretta il 2026-08-27.** Questa riga diceva che il fornitore è il `PlayerController` e che il letterale
> *«sparisce»*. Nessuna delle due regge: il Task 3 ha **consolidato** i letterali in una sola costante locale
> `PlayerTeamId` dentro `ARTHUD` — meglio del previsto, perché ne ha eliminato anche un secondo che la spec non
> nominava — ma il letterale **esiste ancora**, e nessun `PlayerController` è coinvolto. Spostarlo là è lavoro
> proprio, non un effetto collaterale di questa fase.

### 3.5 Quando si calcola il verdetto: alla scrittura, non alla lettura

**[D-223]** (2026-08-27) chiude la domanda che questa sezione lasciava aperta.

> **Un canale che racconta il PASSATO porta il verdetto di conoscenza calcolato quando il fatto è accaduto;
> solo la sagoma dell'ultimo contatto risponde al presente, ed è per costruzione.**

Il filtro chiedeva *«lo conosco adesso?»* costruendo la vista **al momento della lettura**, mentre parte dei
canali racconta ciò che è **già successo** — quali, lo dice la colonna «Quando si decide» qui sotto, e non
una cifra in questa frase: la stesura precedente diceva *«tre canali su cinque»*, ed è invecchiata il giorno
in cui il **playback** è entrato nella tabella come sesto. Un nemico che era `Live` mentre si muoveva e ora
è `Remembered` avrebbe — filtrando la traccia come si filtra il log — **la traccia nascosta e la sagoma
mostrata**: due canali, lo stesso soggetto, due regole opposte.

| Canale | Quando si decide | Perché |
|---|---|---|
| Combat log | **alla scrittura** | racconta il turno risolto; il soggetto esiste ancora quando la riga esce |
| Traccia post-lock | **alla scrittura, per cella** | una rotta non è un fatto puntuale: porta il tratto **osservato** e si tronca dove l'osservatore ha perso il soggetto (**#1497**) |
| Sagoma dell'ultimo contatto | **alla lettura** | risponde a *«adesso non lo conosco»*: è il canale del ricordo, e in lettura è dove deve stare |
| Overlay e modello | **alla lettura** | descrivono il presente |
| Fog of war ([D-225]) | **alla lettura** | è visibilità di **celle**, non di soggetti |
| **Playback** (`TickPlayback`) | **alla scrittura, per cella** | come la traccia, e per la stessa ragione: un movimento non è un fatto puntuale. Il modello percorre il tratto **osservato** e si ferma dove l'osservatore ha perso il soggetto (`#1525`). 🔴 **Il verdetto è lo STESSO oggetto della traccia**, copiato in `FRTResolvedEvent::CellVerdicts` dal punto in cui `FreezeRouteVerdicts` lo congela — se i due divergessero, traccia e modello tornerebbero a raccontare frasi diverse sullo stesso movimento, che è la **contraddizione** che [D-223] nomina |
| **Replay archiviato** (`GetCurrentPhaseEntries`) | **alla scrittura, alla registrazione** | è il combat log della prima riga, ma **differito**: chi lo guarda apre un file, e il verdetto in quel momento non esiste più — `FRTTurnLogEntry::Verdict` è `Transient`. Le voci si filtrano quindi mentre la partita gira, e l'archivio porta una traccia **per squadra** accanto alla canonica ([D-316], `#2098`) |

#### 🔴 Il replay archiviato è il canale che ha dimostrato perché questa colonna esiste

*(Aggiunto il 2026-09-03, [D-316].)*

Le altre cinque righe «alla scrittura» hanno il verdetto a portata di mano nel momento in cui decidono. Il
replay no: fra la scrittura e la lettura c'è un **file**, e il verdetto non lo attraversa.

Per un anno questo ha prodotto una lettura sbagliata della tabella — *«filtrare il replay richiede il
verdetto nella traccia»* — che è vera solo se si decide **alla lettura**. Decidendo alla scrittura il
problema non si pone: il prodotto pubblico nasce già filtrato, e la traccia non ha mai bisogno di portare
la maschera. ⚠️ Ed è meglio così anche se il costo fosse stato pari, perché una traccia che porta il
verdetto **spedisce allo spettatore la maschera di chi poteva vedere cosa** — cioè fa esattamente ciò che
§1.3 dichiara non essere un confine.

⛔ **Il confine chiuso è quello della superficie pubblica, non del filesystem.** La traccia canonica resta
sul disco accanto alle filtrate: chi ha accesso alla cartella ha accesso a tutto. Dove vivano gli archivi e
chi possa aprirli è [D-276] §3, ancora aperta.

**Lo spettatore neutrale vede tutte le voci**, ed è dichiarato ([D-316] punto 5): un replay pubblicato è già
una rinuncia alla privacy competitiva, e i **campi** di audit restano tolti anche a lui.

#### 🔴 Per la traccia «quando il fatto è accaduto» non è definito, e la rotta si tronca

*(Aggiunto il 2026-08-28. La riga della tabella diceva soltanto «alla scrittura», e non bastava.)*

Una riga di log è un fatto **puntuale**: c'è un istante in cui è accaduta, e il verdetto di quell'istante la
descrive. Una rotta no — è una **traiettoria**, e attraversa due istanti: il primo vertice è la cella di
partenza (`RTTurnManager.cpp:5377`, letta prima di `PlaceOnCell` a `:5399`), l'ultimo è la cella d'arrivo.

Un verdetto congelato sulla partenza autorizzerebbe a disegnare l'arrivo, e riprodurrebbe **gli stessi due
errori speculari** che §3.5 descrive per il canale derivato:

| | |
|---|---|
| **leak** | osservato alla partenza, arrivo in cella non osservata → la polilinea **entra nella nebbia** e rivela dove il nemico si è nascosto. `PIE-KNOW4` lo ha già osservato dal vivo: *«arriva dove il nemico sta davvero»* |
| **contraddizione** | non osservato alla partenza, arrivo in cella osservata → traccia nascosta mentre il **modello** è disegnato. È la frase con cui [D-223] apre: *«due canali, lo stesso soggetto, due regole opposte»* |

**La regola è quindi il tratto osservato**: la traccia porta le celle che l'osservatore vedeva, e si
interrompe dove ha perso il soggetto. Delle tre forme possibili è l'unica che sia una frase vera in ogni
caso — *«ho visto questa parte del suo movimento»* — e l'unica che **chiude** `PIE-KNOW4` invece di
dichiararlo limite noto.

➕ **Non serve macchinario nuovo**: `URTPerceptionLibrary::TeamAwarenessOfCell(Map, Observers, Cell)` esiste
e la produzione la usa già dentro il ciclo a micro-step del Move (`RTTurnManager.cpp:5115`).

⚠️ **Limite dichiarato**: il troncamento usa la visibilità **pre-Move**, l'unico campione disponibile —
`TeamKnowledgeState` ha **due** sole assegnazioni per turno, entrambe per fase. Risponde bene quando a
nascondere è il movimento del **nemico**; sbaglia quando è quello dell'**osservatore**. Chiude i due casi
che contano, non tutti.

🔴 **La regola atterra come UN predicato, e i canali lo chiamano.** Se ogni consumatore la ridériva dalla
prosa, le riletture divergono e si riforma la terza via che la decisione vieta — filtrare *alcuni* canali
col presente e altri col passato, che è esattamente lo stato da cui si è partiti.

**Come si calcola, misurato.** `URTTeamKnowledgeLibrary::ClassifyTarget` è puro e chiede
`(Knowledge, SubjectId, SubjectTeamId, SubjectCurrentCell)`. In `ConcludeTurn` le righe escono a
`RTTurnManager.cpp:2383-2386` e `DestroyDefeatedUnits` gira a `:2405`: alla scrittura il soggetto **esiste
ancora**. I **79** siti sparsi hanno l'unità in mano e costano una riga ciascuno.

#### 🔴 Il canale derivato è il caso difficile, e «alla scrittura» lì non basta

*(Corretto il 2026-08-28. Questo paragrafo diceva: «il canale primario è un ciclo solo, quindi basta **una**
mappa `id → cella` per turno». Era sbagliato due volte, e la seconda metà è il difetto vero.)*

L'unico sito che emette dal TurnLog (`RTTurnManager.cpp:2383-2386`) scrive **in un colpo solo le voci di
tutte e cinque le fasi**, e in quell'istante i due ingressi di `ClassifyTarget` vengono da due momenti
diversi:

| Ingresso | Da quando | Misura |
|---|---|---|
| `Knowledge` | ultimo refresh, che è quello del **Blast** — **pre-Move** | `RefreshTeamKnowledgeForBlast` chiamata a `RTTurnManager.cpp:3785` |
| `SubjectCurrentCell` | **post-Move** | `PlaceOnCell` a `RTTurnManager.cpp:5338` |

`AwarenessOfUnit` decide con `Knowledge.VisibleCells.Contains(CurrentCell)`, quindi mescolarli produce **due
errori speculari, entrambi reali**: un **leak** — un soggetto che nel Move entra in una cella visibile
pre-Move e che nessuno osserva più — e una **perdita** — un soggetto visibile mentre agiva, che nel Move esce
dal set stantio e si porta via l'intera narrazione del proprio turno.

⚠️ **Il refresh successivo esiste ma arriva dopo**: `StartPlanningTimer` (`:2446`) → `PlanBots` (`:1222`) →
`RefreshTeamKnowledgeForPlanning` (`:505`) osserva le celle post-Move, ma gira **sessanta righe dopo**
l'emissione. Non è disponibile a chi scrive.

⚠️ **E `ClassifyTarget` vuole un terzo ingresso che il TurnLog non ha**: `TargetTeamId`. Misurato: `TeamId`
compare **0** volte in `Turn/RTTurnLog.h`, con controprova a **29** su `UnitId` nello stesso file.

🔴 **Chi implementa deve dichiarare, per questo canale, quale conoscenza e quale cella entrano nel calcolo.**
È la sola parte della decisione che il codice non risolve da solo, e le vie che lascia aperte — accettare per
iscritto il verdetto «conoscenza pre-Move + cella del fatto», anticipare il congelamento al sito che produce
la voce, o aggiungere un terzo campione — hanno costi diversi e vanno confrontate, non scelte per inerzia.

### 3.6 ✅ Il limite del morto è chiuso da [D-223], e non serve toccare `ViewForTeam`

*(Questa sezione era il «limite dichiarato» §3.5. La regola che descriveva è ancora nel codice; ciò che è
cambiato è che non governa più il combat log.)*

`ViewForTeam` **salta i soggetti non vivi** — `if (!S.bAlive) continue;` (`Perception/RTKnowledgeView.cpp`),
*«un morto non è un soggetto di conoscenza: lo tratta la presentazione della sconfitta»*. Finché il filtro
era applicato **in lettura**, la conseguenza era che l'intera narrazione del turno di chi cade spariva —
retroattivamente su tutto il buffer di `MaxLogLines`, non solo sul turno della morte.

🔴 **Con [D-223] quella guardia non entra più in gioco per il combat log**, e la ragione è misurata:
`ClassifyTarget` **non guarda lo stato vitale** — `Perception/RTTeamKnowledge.cpp` ha **zero** occorrenze di
`bAlive`/`IsAlive`, con controprova a **1** in `RTKnowledgeView.cpp`, dove la guardia vive. Un verdetto
congelato alla scrittura non la incontra mai.

⚠️ **Due cose restano vere e vanno sapute.** La guardia continua a governare i canali che si calcolano **in
lettura** — overlay, modello, sagoma — ed è lì corretta: chiedere «conosco quel morto?» non è la domanda
giusta quando la morte è pubblica. E il caso da verificare invece che presupporre resta: si può uccidere
qualcosa che non si è **mai** visto (AoE, danno ambientale), e quella riga deve restare nascosta — con
[D-223] lo resta perché `ClassifyTarget` risponde `Rejected`, non perché il soggetto è sparito. La
distinzione conta: le due cose smettono di coincidere appena si tocca una delle due cause. Owner della
verifica: **#1498**.

**Cos'era il difetto, e perché la tabella qui sotto va riletta due volte.** Il filtro era applicato **in
lettura**: `ARTTurnManager::GetRecentEventsForTeam` costruiva la vista *ora* e la passava a
`ComposeVisibleLogLines` su **tutto** `RecentEvents`, un anello di `MaxLogLines` righe. La morte era quindi
**retroattiva sull'intero buffer** — nell'istante in cui un'unità cadeva, ogni riga già scritta il cui
soggetto era lei perdeva la propria voce nella vista.

Misurato sui siti che scrivono la morte:

| # | Riga | Sito | Soggetto | Prima di [D-223] | Con [D-223] |
|---|---|---|---|---|---|
| 1 | `<nome> eliminato dalle fiamme` (danno da `Status.Burning`) | `RTTurnManager.cpp:1459` | **la vittima** | ❌ spariva, insieme al resto del suo turno | ✅ resta per chi vedeva la vittima quando è caduta |
| 2 | `<nome> eliminato dalla scarica` | `RTTurnManager.cpp:2257` | nessuno | ✅ resta, **per omissione** | va deciso: nomina un'unità |
| 3 | `Eliminata: <nome> (team N)` (ramo `NewlyDefeated`, Blast) | `RTTurnManager.cpp:4320` | nessuno | ✅ resta, **per omissione** | va deciso: nomina un'unità **e la sua squadra** |
| 4 | `Morte mostrata: <nome>` (playback, fase corrente) | `RTTurnManager.cpp:5603` | nessuno | ✅ resta, **per omissione** | va deciso: nomina un'unità |
| 5 | `Morte mostrata: <nome>` (playback, catch-all finale) | `RTTurnManager.cpp:5659` | nessuno | ✅ resta, **per omissione** | va deciso: nomina un'unità |

⚠️ **Questa tabella ne elencava TRE, e ne classificava male una.** *(Corretta il 2026-08-28.)* Le due righe
`"Morte mostrata: %s"` del playback non erano mai state censite. E la riga 3 era descritta come *«una riga di
mondo»*: **non lo è** — stampa il nome di un'unità **e** il suo `TeamId`. Resta visibile solo perché il
default di `AddLogEvent` è fail-open, cioè per il difetto stesso che **#1499** chiude.

🔴 **Quattro righe di morte su cinque passano per omissione, e sono quindi in scope per #1499.** Nessuna di
esse è «restata» per una decisione: chiudere il default senza deciderle una per una le farebbe sparire tutte
e quattro insieme — ed è precisamente perché #1499 e [D-223] vanno fatte nella stessa passata.

### ✅ Deciso: la morte è pubblica

**Decisione d'autore del 2026-08-28**, dentro [D-223]. Tutte e cinque le righe di eliminazione portano
`FRTLogSubject::World()`: un'unità che cade la leggono **tutte le squadre**, anche chi non la vedeva.

🔴 **Non è il vecchio default fail-open che ricompare, e la differenza è tutta nel codice.** Prima quelle
righe passavano perché nessuno aveva dichiarato un soggetto; adesso passano perché qualcuno ha deciso che
devono. L'output a schermo è lo stesso — la ragione no, ed è la ragione che il prossimo autore può cambiare
sapendo cosa sta facendo.

⚠️ **Cosa si rivela, e cosa no.** Le cinque righe portano nome e squadra, **mai una cella**: chi uccide con
un'AoE un nemico mai visto scopre che esisteva e che è caduto, non dove fosse. E dopo la morte non c'è più
una posizione da proteggere. È il caso che questa sezione elencava come «non banale», e la decisione lo
copre esplicitamente invece di lasciarlo scoprire.

Il test che la protegge è `RefactorTactics.UI.DeathIsPublicEvenToWhoNeverSawIt`, e verifica
l'**asimmetria**: l'annuncio della morte arriva a chi non vedeva la vittima, e una riga ordinaria sulla
stessa unità **no**. Senza la seconda metà passerebbe anche a filtro spento.

#### ⚠️ «Pubblica» vale per l'ANNUNCIO, non per il racconto del colpo

C'è una sesta riga che parla di morte e che **resta filtrata**, e la distinzione non è un'incoerenza da
sanare: è il criterio.

| Riga | Cosa stampa | Esito |
|---|---|---|
| `Eliminata: <nome> (team N)` e le altre quattro | nome, squadra — **nessuna cella** | **pubblica** |
| `(q,r,L) -> (q,r,L): N danni, eliminata` (`DescribeEntry`, esito `Lethal`, canale derivato) | **due celle**, il danno e chi ha colpito | **filtrata** |

Il criterio è quindi *«che cosa la riga rivela»*, non *«di che cosa parla»*: è pubblico **che** un'unità sia
caduta, non **come** né **da dove**. La seconda riga porta la cella dell'attaccante e quella del bersaglio —
esattamente ciò che la conoscenza parziale esiste per proteggere — e passa dal verdetto congelato della
propria voce, quindi la legge solo chi vedeva il soggetto quando ha colpito.

#### ➕ Condizione di riapertura: la morte pubblica enumera il roster

Una conseguenza che la decisione accetta e che va scritta perché non si scopra dopo: ripetuta lungo la
partita, la morte pubblica rivela a chi non ha mai visto l'avversario **quante unità aveva e quali eroi
erano** — informazione di *composizione*, non di posizione.

In v0.1 è inerte: il formato è 2v2 con roster fisso e noto in partenza (**D-120**), quindi non si scopre
nulla che non fosse già sul tavolo. **La condizione di riapertura è il draft**: il giorno in cui la
composizione avversaria smette di essere nota a priori, questa decisione va rivalutata — non perché diventi
sbagliata, ma perché il costo che oggi è zero smetterebbe di esserlo.

---

## 4. Fase A — Le unità

Quattro consumatori della stessa porta, in ordine di costo crescente.

### A1 — `TargetUnknown` diventa leggibile

`URTTurnLogLibrary::DescribeInvalidReason` ha otto `case` e un `default: "non eseguibile"`;
`ERTActionInvalidReason::TargetUnknown` **non compare** e cade nel default.

Oggi la regola più significativa della conoscenza parziale — *«per la tua squadra quel bersaglio non c'è»* —
è **illeggibile in partita**. Un `case` mancante è tutta la distanza fra una regola che punisce e una che
insegna, ed è la voce più economica dell'intera spec.

### A2 — I due leak si chiudono

`ARTHUD::DrawHUD` continua a iterare gli attori, ma **salta** quelli che la vista non conosce
(`ShouldDrawUnitOverlay`). *(Questa riga diceva «smette di iterare `GetAllActorsOfClass` e itera
`FRTKnowledgeView`»: non è ciò che il Task 3 ha costruito, ed è una differenza che conta — il ciclo resta sugli
attori, quindi ogni cosa che il ciclo legge dall'attore va filtrata a sua volta.)* Il combat log
passa dallo stesso filtro prima di essere disegnato.

Senza questa voce tutte le altre sono cosmesi: si può nascondere il modello, ma il nome e la barra HP
dell'unità nascosta resterebbero stampati a schermo, e il log ne stamperebbe la cella esatta.

> 🔴 **Il leak si chiude per METÀ con A2, e va detto perché il commit da solo lascia intendere di più.**
> `ShouldDrawUnitOverlay` risponde `true` anche per una voce `Remembered` — la voce **esiste** — e da lì in giù
> il ciclo di `DrawHUD` legge dall'**attore**: `GetActorLocation()`, `Health`, `Shield`, `Energy`, gli stati.
> Quindi per un nemico di cui la squadra ha solo il **ricordo**, l'HUD disegna ancora **posizione e condizione
> VERE** — esattamente i due campi che `FRTKnowledgeEntry` rifiuta di portare, e per i quali `E.Cell` e
> `E.Visibility` oggi **non hanno consumatori**.
>
> Non è una regressione — alla base si disegnava tutto per tutti — e **A3 e A4 sono la cura**: A3 nasconde
> l'unità, A4 disegna il ricordo dove il ricordo dice. Ma finché non atterrano, «il leak è chiuso» è vero solo
> per `Rejected`. ⚠️ `Knowledge.HudDrawsOnlyKnownUnits` **non esercita** il caso `Remembered`: è la copertura
> che manca per accorgersene.

### A3 — L'unità ignota sparisce

L'attore **resta spawnato**: `SpawnActor<ARTUnit>` esiste **solo in `Tests/`**, quindi non c'è un percorso
di spawn runtime da inventare, e distruggere/ricreare attori a ogni cambio di conoscenza sarebbe una
macchina nuova per un problema che non c'è.

`ARTUnit::HideForDefeat` **non è riusabile**: è a senso unico, serve la morte, e disabilita la collisione
senza un percorso di ritorno. Serve un `SetKnownToObserver(bool)` reversibile, che governi visibilità **e**
proxy di click.

### A4 — La sagoma del ricordo

Un componente proprio sull'`ARTUnit`, alla `Cell` del contatto, che sfuma alla scadenza
(`URTTeamKnowledgeLibrary::ContactLifetimeTurns = 1` — **nessun numero nuovo**).

**Due canali di distinzione dall'Action Ghost** (§7, decisione nuova):

| | Ricordo (*Last Contact*) | Intento (*Action Ghost*, [#249](https://github.com/DegrassiAaron/refactor-tactics-main/issues/249)) |
|---|---|---|
| Colore | **monocromo, desaturato** | colori di squadra |
| Forma | **senza freccia di facing**, marker a terra proprio | facing, orientamento arma, origine attacco |

**Porta il nome, non la barra HP.** La squadra conosce l'identità ma non la condizione: è lo stesso confine
che [#160](https://github.com/DegrassiAaron/refactor-tactics-main/issues/160) ha già fissato per il bot
(*«`CellOnly` → HP massimi»*), e una barra costringerebbe a mostrarne una falsa.

⚠️ **La sagoma vale solo per il contatto VISIVO perduto.** Un contatto acustico non dice **chi**
([D-113](../../decisions/RT_PDR_00_Decision_Log.md)): dargli la silhouette di un eroe sarebbe inventare
un'identità che nessuno possiede. Il rumore ha la sua grammatica, ed è un'area (§6.4).

**Quota**: ogni decoro a terra deve stare sopra `RTCellTopZ` (2,5 uu) — chi disegna sotto quella soglia
disegna dentro un cilindro opaco. Le quote già assegnate sono 0,3 / 0,5 / 1,5 / 2,5; il marker del ricordo
ne prende una nuova, **dichiarata in `Map/RTMapVisuals.h`** come le altre, non ricopiata.

### A5 — Editor

Un materiale nuovo, e uno soltanto: nessuno dei quattro materiali versionati è traslucido né ha un
parametro scalare, quindi non c'è nulla da riusare.

⚠️ La mesh degli eroi vive in **`Content/FabAsset`, che non è versionata**. La sagoma la deriva **a runtime**
dalla stessa mesh dell'unità, non da un asset nuovo: un asset nuovo sarebbe una copia di un binario che il
repository non possiede.

---

## 5. Fase B — La fog of war

> ⛔ **Fino al 2026-08-27 questa sezione descriveva un *velo*: il colore di superficie scalato.**
> [D-225](../../decisions/RT_PDR_00_Decision_Log.md) ha deciso che la fog of war **entra nella v0.1 e
> nasconde**. Non è una differenza di intensità — un velo lascia leggere il contenuto della cella più
> debolmente, la fog non lo mostra affatto. Ciò che il velo aveva imparato e che sopravvive è marcato ➕;
> ciò che cade perché nascondere è più semplice che velare è marcato ➖.

### 5.1 Regola

Una cella che **nessuno** della squadra osserva non mostra il proprio contenuto: un **prisma esagonale
opaco** ne riempie il volume. Restano visibili la tassellatura e la distanza — *dove* sono le celle e che
forma ha la griglia — sparisce *cosa* c'è sopra: colore di superficie, corona incisa dei `SurfaceGlyphs`,
rilievo del costo, volumi del blocco, pannelli di bordo.

> ⚠️ **`FRTKnowledgeView` NON ha un campo `VisibleCells`**, e la prima stesura di questa riga lo leggeva. La
> struct costruita in Fase A ha `ObserverTeamId` ed `Entries`, nient'altro. Chi implementa la Fase B deve
> **aggiungerlo** — o passare `FRTTeamKnowledge::VisibleCells` accanto alla vista — e sceglierlo è parte di
> quel checkpoint, non un dettaglio: un campo in più sulla vista è un campo in più che attraversa la porta.

➕ La fog è **binaria**, come lo era il velo: `ERTAwareness` ha tre livelli **sulle unità**, e una terza
categoria per le *celle* non esiste in nessuna decisione. Inventarla qui creerebbe un vocabolario senza owner.

### 5.2 Perché è ammesso dal canone

[`progettazione-hud.md`](progettazione-hud.md) §25 *Partial Knowledge* dice alla lettera:

> Non usare una classica mappa nera.
>
> La mappa statica resta leggibile.

Il divieto resta, e riguarda il **vuoto piatto**: una mappa nera cancella la lettura *spaziale* — dove sono
le celle, quanto dista un punto, che forma ha il terreno. Un prisma che **occupa** la cella non la cancella:
la griglia continua a tassellare, la distanza si continua a contare, il terreno perde il proprio *tipo* e non
la propria *posizione*. È la distinzione che [D-225](../../decisions/RT_PDR_00_Decision_Log.md) scrive, e
senza la quale la fog sembrerebbe violare §25 mentre lo rispetta.

Il rapporto con [D-146](../../decisions/RT_PDR_00_Decision_Log.md) è chiarito da
[D-228](../../decisions/RT_PDR_00_Decision_Log.md) (2026-08-27), e **non c'è conflitto**: D-146 governa come
una cella **mostrata** comunica la propria superficie — è una regola di *encoding*, non di *visibilità*. Non
dice **se** una cella vada mostrata. Una cella che nessuno osserva, e che quindi non si legge, non è un
fallimento di leggibilità: è lo scopo.

D-228 dichiara anche il perimetro che entrambe presupponevano: **la forma è graybox e cadrà** con i
materiali per superficie di M8/M9 — dischi, corona incisa, `Relief`, `Blockers`, `EdgeFeatures` sono un
ponteggio — mentre **la regola dei due canali resta e vincolerà anche l'arte**, perché è ciò che regge
l'accessibilità (daltonismo, scala di grigi, video ricompresso) e i gate `G10`/`G13`. L'effetto fumo/nebbia
è un art pass sulla stessa regola, non un cambio di questa fase.

➖ **Cade il vincolo più insidioso del velo, e vale la pena dire perché.** La stesura a velo doveva attenuare
**entrambi** i canali insieme — disco *e* corona — o una cella **non osservata** avrebbe mostrato un anello
brillante su un disco spento, risultando **più** appariscente di una osservata. Era il difetto peggiore della
prima stesura di questa sezione, che diceva l'opposto. Una cella nascosta non ha canali da bilanciare: il
problema non si presenta.

### 5.3 Meccanismo

**Il gesto esiste già.** `ARTHexMapActor` istanzia prismi esagonali sopra la faccia del disco per i due
volumi delle regole — `AddVolume(PlanarFraction, VolumeHeight)` in `RTHexMapActor.cpp`, sulla stessa mesh
generata da `GetCellPrismMesh()`. Una cella di nebbia è la stessa chiamata con `PlanarFraction = 1.0` e
un'altezza che copra un'unità: la colonna blocca-movimento è alta **55 uu**, il cilindro segnaposto ne è alto
~180 (`VisualZOffset = 90`), quindi l'ordine di grandezza è **~200**. La quota di appoggio è `RTCellTopZ`,
come per gli altri volumi.

`ApplyFogOfWar(const FRTKnowledgeView&)` vive accanto a `RebuildInstances`, che resta l'unico **costruttore**
della mappa:

1. costruisce un `TSet<FRTCellId>` da `VisibleCells`;
2. `Fog->ClearInstances()`;
3. per ogni cella **non** osservata, una `AddInstance` col transform del prisma.

Dei **tre buchi** misurati in §1.4 ne restano **uno e mezzo**:

- ➖ **il canale cade del tutto.** Il velo aveva bisogno di un quarto `CustomDataFloat` o di un
  `ScalarParameter` che nessuno dei quattro materiali versionati possiede. La fog non tocca i dati per
  istanza di `Cells`: è un componente **separato**, con un materiale proprio.
- ➖ **la mappa inversa cella→istanza non serve.** Il velo doveva riscrivere il colore dell'istanza *giusta*;
  la fog ricostruisce l'intero componente a ogni refresh. Su un'arena da **61 celle** (`DemoArenaRadius = 4`)
  è `O(celle)` due volte per turno, deterministico, senza rebuild della mappa.
- 🔴 **il trigger resta**, ed è §5.4.
- 🔴 **il materiale resta**: `M_HexCell` è Unlit + Opaque, ma è il materiale delle *celle*. Alla nebbia ne
  serve uno proprio; per il graybox **l'opaco va bene**, ed è più onesto di un traslucido — un prisma
  attraverso cui si intravede il contenuto è un velo travestito.

➕ **Nessuna seconda verità sul colore di superficie.** Era il vincolo centrale del velo — conservare il
colore originale accanto a quello velato avrebbe creato una seconda sorgente, divergente alla prima modifica
della tavolozza. La fog non scrive colori: `URTHexLibrary::SurfaceColor` resta l'unico owner, intatto.

### 5.4 Trigger

Nasce un delegate — `OnTeamKnowledgeRefreshed` — emesso dai **due** punti che già rinfrescano la conoscenza:
`RefreshTeamKnowledgeForPlanning` e `RefreshTeamKnowledgeForBlast`. Non si inventano momenti nuovi.

**Durante il playback la fog segue quei due punti**, e quindi *salta* due volte per turno invece di scorrere.
Non è un difetto da nascondere: è la granularità che il resolver ha, ed è la stessa che D4 del brief fissa
per la visibilità (*«si ricalcola ai confini di fase, non a ogni micro-step»*). L'alternativa — spegnere la
fog durante il playback — sarebbe una finestra di onniscienza, e *«il replay del giocatore va filtrato
durante il match»* è uno dei due punti che il brief §9 dichiara **obbligatori**.

### 5.5 ➖ Il gate sulla leggibilità non va più esteso

`RefactorTactics.Hex.SurfaceColorsAreDistinguishable` (`Tests/RTHexTests.cpp:646`) confronta
`URTHexLibrary::SurfaceColor(All[I])` contro `SurfaceColor(All[J])`: misura il colore **non velato**.

Contro un **velo** era un gate cieco — avrebbe continuato a dire verde mentre la mappa perdeva leggibilità,
e la fase avrebbe dovuto portarsi dietro l'estensione alle coppie velate, perché D-146 registra che alcune si
distinguono per **luminanza**, cioè proprio per il canale che il velo tocca.

Contro la **fog** il gate resta valido com'è: continua a misurare le celle **mostrate**, che sono le uniche
che comunicano una superficie. Una cella nascosta non ha una lettura da preservare. **L'estensione non
serve**, e non perché la si rinvii: perché la classe di difetto non esiste più.

### 5.6 🔴 Il rischio che nessun test misura

Un prisma opaco alto ~200 uu può **occludere le unità proprie** quando la camera guarda attraverso una fascia
di nebbia. È l'unica cosa di questa fase che nessun test automatico misura, e va in `test-manuali-pie.md`
prima del merge, non dopo.

⚠️ **Se si verifica, la risposta non è abbassare il prisma** — rimetterebbe in vista esattamente ciò che deve
nascondere. È decidere se la nebbia si renda in modo diverso vista dall'alto, ed è una decisione, non una
taratura.

---

## 6. Fase C — Il rumore

### 6.1 La regola derivata (`AE-8`)

**Tre classi, e un solo numero nuovo in tutto.** Il criterio si legge da campi che `FRTActionDef` già
possiede (`MovementStyle`, `RangeCells`, `StructureOp`, `Effects`), mai da un elenco per azione.

| Classe | Criterio | Rumore |
|---|---|---|
| **Impatto** | fa danno, **oppure** ha uno `StructureOp`, **oppure** ha un `MovementStyle` non-`Budget` | `6` |
| **Passo** | ha `MovementStyle::Budget` | `max(0, RangeCells − 3)` |
| **Silenziosa** | nessuna delle due sopra | `0` |

La storia della classe *Passo*: **le prime tre celle sono silenziose**, ogni cella oltre vale `1`. Il **`3`**
è l'unico numero che questa regola inventa.

> 🔴 **Corretta il 2026-08-27, dopo che un panel l'ha falsificata. La prima stesura leggeva `CostMP`, e
> `CostMP` non è il budget di movimento.** La docstring di `FRTActionDef::CostMP` lo dichiara: è il
> *«contributo dell'azione al MODIFICATORE del costo per cella (D-117), non un costo da spendere»*, e
> aggiunge che *«i budget del movimento non stanno qui: vivono in `RangeCells` con
> `ERTMovementStyle::Budget`»*. Misurato: nel catalogo `CostMP` compare **2** volte, entrambe dentro il
> validator, contro **21** di `RangeCells`; e le due righe spedite scrivono il commento a chiare lettere —
> `Action.Sprint … /*Range (MP)*/ 8 … ERTMovementStyle::Budget` e `Action.Move … /*Range (MP)*/ 5`.
> Con il campo sbagliato la regola avrebbe prodotto **`0` per ogni azione del gioco**, falsificando la
> tabella qui sotto e l'ancora `Sprint 5` di D-041. **I numeri erano giusti, il campo era sbagliato.**
>
> 🔴 **E le classi non erano esaustive**, il che era il difetto più pericoloso perché silenzioso.
> `Action.Dash` non cadeva in nessuna: muove (quindi non *Silenziosa*), ha `MovementStyle::LinearDash`
> (quindi non *Passo*), e porta `Effects {}` (quindi non *Impatto*). Peggio: `Action.BasicAttack` nel
> catalogo **core** ha anch'esso `Effects {}` e sarebbe finito in *Silenziosa* — **uno sparo muto**.
> Da qui le due correzioni: *Impatto* include il movimento lineare, e l'ordine di valutazione è
> **Impatto → Passo → Silenziosa**, con `Silenziosa` come residuo invece che come criterio proprio.

🔴 **La regola si applica alla definizione RISOLTA PER EROE, non alla voce del catalogo core**, ed è la
distinzione che rendeva muto l'attacco base. Il catalogo core dichiara *«identità, fase, priorità e
fallback stanno qui; DANNO e PORTATA no»* — il danno lo mette `URTCatalogLibrary::MakeBasicAttack`, che è
anche il produttore di `Hero.Gadget.ArcPulse`. Interrogare la voce core significa interrogare un guscio.

**Il DoD porta un test di esaustività**: ogni voce restituita da `GetCoreActionCatalog()`, risolta per
eroe, cade in **esattamente una** classe — un ramo di default che non scatta mai, come il gate del
catalogo già fa altrove. Senza, una famiglia di azioni che non cade in nessuna classe resta silenziosa e
nessuno se ne accorge.

**Cosa ne cade fuori:**

| Azione | `RangeCells` · `MovementStyle` | Classe | Rumore | Provenienza |
|---|---|---|---:|---|
| `Action.Wait` · `Action.Guard` · `Action.Brace` · `Action.Overwatch` | — | Silenziosa | **0** | ✅ ancora di [D-041](../../decisions/RT_PDR_00_Decision_Log.md) (`Wait 0`) |
| `Action.Move` | `5` · `Budget` | Passo | **2** | derivato |
| `Action.Sprint` | `8` · `Budget` | Passo | **5** | ✅ ancora di D-041 (`Sprint 5`) |
| `Action.Dash` | `3` · `LinearDash` | Impatto | **6** | ✅ ancora di D-041 (`Dash 6`) |
| `Action.Charge` · `Action.Leap` · `Action.Reposition` | `3·3·2` · lineari | Impatto | **6** | derivato |
| `Action.BasicAttack` *(risolta)* · `Action.Interact` su struttura | — | Impatto | **6** | derivato |
| esplosione | — | evento ambientale, non un'azione | **10** | ✅ ancora di D-041 |

⚠️ **`Withdraw` non è nel catalogo** e la prima stesura di questa tabella lo elencava come se lo fosse: è
un **profilo** del movimento normale ([D-070](../../decisions/RT_PDR_00_Decision_Log.md)), non
un'`ActionId`. Se il profilo porta un budget proprio di `2`, la regola gli dà `0` — ma va verificato dove
quel budget vive prima di scriverlo come fatto.

⚠️ **`Reposition` (2 celle) suona come un `Dash` (3 celle)**, ed è una conseguenza da dichiarare invece
che da scoprire: la classe *Impatto* misura la **bruschezza**, non la distanza. Se un riposizionamento
breve dovesse suonare meno, serve un quarto criterio — cioè una decisione nuova, non un aggiustamento.

**Le tre ancore di D-041 non sono state rinegoziate: sono uscite dalla regola.**

Che `BasicAttack` e `Dash` suonino uguale **non è una svista**: è precisamente ciò che
[D-123](../../decisions/RT_PDR_00_Decision_Log.md) chiede, *«impedire che il volume diventi un
identificatore implicito della sorgente»*. Una regola iniettiva — un numero diverso per ogni azione —
violerebbe D-123 alla lettera.

**E risolve una contraddizione documentale aperta.** D-123 dice che il campo è obbligatorio per ogni
producer *«incluse le signature»*; il catalogo azioni dichiara che le abilità firma **non** ricevono
un'intensità, *«ed è una scelta»*, perché sarebbero **24 numeri nuovi**
([#690](https://github.com/DegrassiAaron/refactor-tactics-main/issues/690)). La regola le riconcilia: una
signature che fa danno cade in *Impatto* e vale `6` senza che nessuno scelga niente. D-123 è soddisfatta, e
i 24 numeri non esistono mai.

### 6.2 Verifica di scala

Soglie d'udito, misurate su **due fonti indipendenti** (`RT_HeroCatalog_v0.1.md` §5.1 e
`RTHeroCatalogLibrary.cpp`): **Gadget 5 · Phase 3 · Riktor 3 · Wraith 5**.

`Received` alla sorgente; area **stretta** con margine ≥ `TightBandMargin` (3), **larga** con margine ≥ 0,
nessun contatto sotto:

| Evento | Rumore | Ricevuto | Gadget / Wraith (5) | Phase / Riktor (3) |
|---|---:|---:|---|---|
| `Move` su terreno neutro | 2 | 2 | — | — |
| `Move` su **acqua bassa** (+2) | 2 | 4 | — | **larga** |
| `Sprint` neutro | 5 | 5 | larga | larga |
| `Sprint` su acqua bassa (+2) | 5 | **7** | larga | **stretta** |
| Attacco / `Dash` / `Interact` su struttura | 6 | 6 | larga | **stretta** |
| esplosione | 10 | 10 | stretta | stretta |

**Due controprove che la regola non è stata scelta per far tornare i conti**, perché entrambe erano scritte
prima e da altri:

- **D-042** scrive *«sprintarci fa **7 su 10**»*. La regola dà `5 + 2 = 7`.
- **D-041** scrive *«uno Sprint (5) attenuato di 2 arriva a 3 — lo sentono Riva e Bastion, non Flux e
  Vektor»*. Con la regola: `5 − 2 = 3`; margine `0` per le soglie a 3 (sentono) e `−2` per quelle a 5 (no).

Il risultato di gioco si dice in una frase: **camminare non si sente, correre sì, l'acqua tradisce chi
cammina.**

### 6.3 🔴 `0` deve significare muto davvero

`URTAcousticPropagationLibrary::Propagate` calcola `Budget = Intensity + SurfaceNoiseDelta(OriginCell)` e si
ferma solo a `Budget <= 0`. Un `Wait` (intensità 0) su terreno `Fire` (`NoiseDelta +4`) produrrebbe quindi un
evento a **4**, udibile dalle soglie a 3.

**Un'azione di classe *Silenziosa* non produce l'evento**: il ramo sta **all'emissione**, non nella
propagazione. Il delta di superficie **amplifica** un rumore che esiste; non lo crea. E il commento nel
codice che già afferma questo — *«`Action.Wait` vale 0: il silenzio non produce un evento udibile da
nessuno»* — va reso vero invece di essere lasciato falso.

### 6.4 Chi è l'ascoltatore: **il margine maggiore**

[D-043](../../decisions/RT_PDR_00_Decision_Log.md) dice che la conoscenza è di **squadra**; D-113 dice che
l'area d'incertezza è centrata sull'**ascoltatore** e larga secondo il **suo** margine. In un 2v2 con Gadget
(soglia 5) e Phase (soglia 3), lo **stesso** rumore produce **due aree diverse**. Il DoD di #159 dice *«la
squadra che ode ottiene **un** contatto»*, ma il test che descrive ha un ascoltatore solo: la domanda non era
mai stata affrontata.

**La squadra prende l'area dell'ascoltatore col MARGINE MAGGIORE** — chi ha sentito più forte, quindi chi
è verosimilmente più vicino. Un'area sola, come il DoD chiede, e la meno larga fra quelle disponibili.

> 🔴 **Ritirata il 2026-08-27: la prima stesura diceva «l'intersezione», e la sua giustificazione era
> falsa.** Diceva: *«la sorgente sta dentro l'area di ciascun ascoltatore, quindi intersecarle è
> logicamente corretto»*. **Non lo è.** Il raggio di `PlausibleOriginCells` deriva dal **margine sopra
> soglia** (`ReceivedNoise − HearingThreshold ≥ TightBandMargin ? 2 : 4`), mentre la distanza realmente
> percorsa dal suono è `Budget − ReceivedNoise`. Sono due grandezze **scorrelate**, e l'area è quindi una
> **euristica di plausibilità, non una garanzia di contenimento**.
>
> Controesempio coi valori versionati: un'esplosione (`Intensity 10`) su `Fire` (`NoiseDelta +4`) dà
> `Budget 14`; un ascoltatore con soglia `3` a costo acustico `10` riceve `4` → margine `1` → area
> **larga, raggio 4**. La sorgente è a **10**. Fuori.
>
> ⚠️ **E il caso limite che avevo dichiarato era quello sbagliato.** Non è l'intersezione *vuota*: è
> l'intersezione **non vuota che esclude la sorgente**. Intersecare due stime imprecise produce un'area
> più piccola, più sicura di sé, e più sbagliata — il difetto peggiore dei due, perché non si annuncia.

⚠️ **Questa è una scelta di design, non solo una correzione tecnica, e va confermata dall'autore.** La
triangolazione — due eroi distanziati che localizzano meglio — era una tattica emergente gradevole, e la
si perde. Rifondarla richiederebbe che l'area fosse costruita sulla **distanza acustica** invece che sul
margine, cioè una modifica a `PlausibleOriginCells` e un emendamento a
[D-113](../../decisions/RT_PDR_00_Decision_Log.md): è lavoro proprio, con la sua issue.

**Il test cambia di conseguenza.** `Noise.IntersectionNarrowsUncertainty` esce. Entrano:

- `Noise.TeamTakesBestInformedArea` — con due ascoltatori a margini diversi, la squadra riceve **una**
  voce, ed è quella del margine maggiore;
- `Noise.AreaIsHeuristicNotGuarantee` — costruisce il controesempio qui sopra e **asserisce che la
  sorgente cade fuori**. Un test che *documenta un limite* invece di fingere che non ci sia: se un giorno
  l'area diventasse una garanzia, questo test diventerebbe rosso e costringerebbe a riaprire la decisione
  invece di lasciarla scadere in silenzio.

### 6.5 Impianto

| Questione | Decisione | Perché |
|---|---|---|
| **Dove vive il contatto acustico** | Dentro `FRTTeamKnowledge`; `CurrentVersion` **1 → 2** | Un solo contenitore, un solo posto che fa scadere le cose. La struct non entra in `RTMatchStateHash` né in `Replay/`: l'unico effetto è scartare una memoria in corso |
| **Durata** | **1 turno**, come la vista | `ContactLifetimeTurns` esiste già. Il DoD dichiara che un `N ≠ 1` sarebbe **un numero di bilanciamento nuovo**: non se ne apre uno senza motivo |
| **Categoria di log** | Valore nuovo `Acoustic`, applicato in **tre** siti | Riusare `Environment` renderebbe un rumore indistinguibile da un evento di superficie negli scenari. ⚠️ `ReactionClash` è stata aggiunta all'enum e **dimenticata** in `OutcomeEnumForCategory` e `DescribeEntry`: costa tre siti, non uno |
| **`FRTNoiseEvent` e la fase** | Settimo campo, `Phase` | `MicroStepIndex` esiste solo nel Move: senza la fase, due eventi di fasi diverse collidono su `(TurnIndex, 0)` e l'ordine riproducibile non lo è. La docstring *«sei campi e non uno di più»* va **aggiornata**, non aggirata |
| **Quando la conoscenza cambia** | Evento emesso **al micro-step**, conoscenza applicata **al confine di fase** | Tiene l'invariante #3 (*raccogli poi applica*), coincide con D4 del brief e coi due refresh che già esistono |
| **Il tipo dell'evento acustico nel DTO** | **Non c'è** | Riconoscere il tipo è il Livello 5 (*Identificazione*), fuori dalla v0.1 per D-113 |

**Cosa il DTO acustico non può contenere**, e perché — la lista è il gemello di `FRTIntentView`:

| Campo di `FRTNoiseEvent` | Perché resta fuori |
|---|---|
| `OriginCell` | server-side per docstring: è la posizione esatta della sorgente |
| `SourceUnitId` | un rumore non dice **chi** (D-113) |
| `Intensity` | il volume identificherebbe la sorgente (D-123) |
| `NoiseType` | riconoscere il tipo è Livello 5, fuori v0.1 (D-113) |

Resta: **insieme di celle · turno · `Uncertain`**. Il DTO è per costruzione **diverso** da `FRTNoiseEvent`,
esattamente come `FRTIntentView` lo è da `FRTPlannedIntent`.

⚠️ **L'osservatore reale in v0.1 è il bot.** Se il filtro acustico non è quello che il bot attraversa, si
reintroduce l'onniscienza che `HexBotPlay.HiddenEnemyFairness` ha appena chiuso.

⚠️ Il filtro vive **accanto** a `RTIntentPrivacyLibrary`, mai dentro `RTTurnLog*`.

### 6.6 Il segnale acustico — la metà che mancava

> 🔴 **Aggiunta il 2026-08-27. Un panel ha notato che questo documento si intitola «conoscenza parziale
> VISIBILE» e che nel §6 la parola HUD non compariva mai**: tutti i test della Fase C erano headless
> (`Noise.*`), e l'unico consumatore dichiarato era il bot. Un giocatore che sentiva un rumore, a fine
> Fase C, **non vedeva nulla**. Il titolo prometteva ciò che i test non misuravano.

**Quando la squadra ode qualcosa, appare un segnale** (decisione **N2** dell'autore, 2026-08-27).

Il segnale è l'unico consumatore *umano* del canale acustico, e ha una grammatica propria perché il dato
che lo alimenta è diverso da tutti gli altri: **un rumore non dice chi**. Quindi non è una sagoma, non
porta un nome, non porta una barra.

| | Contatto **visivo** perso | Contatto **acustico** |
|---|---|---|
| Cosa si sa | chi, dove era, quando scade | **che** qualcosa è successo, **dove circa** |
| Forma | sagoma volumetrica monocroma (§4 A4) | **area** + un segnale, mai una figura |
| Identità | il nome dell'eroe | **nessuna** |

Il documento HUD ha già la voce: §25 assegna a *Uncertain Contact* «`?` + area approssimata».

⚠️ **`progettazione-hud.md` §26 è più permissivo delle decisioni, e va corretto insieme a questa fase.**
Elenca fra ciò che la modalità Sound «può mostrare» anche **categoria** e **intensità**: entrambe vietate —
riconoscere il *tipo* di un rumore è il Livello 5 della scala e resta fuori dalla v0.1
([D-113](../../decisions/RT_PDR_00_Decision_Log.md)), e mostrare il *volume* renderebbe il rumore un
identificatore della sorgente ([D-123](../../decisions/RT_PDR_00_Decision_Log.md)). Il segnale mostra
**dove circa**, e nient'altro.

**Test**: `Knowledge.AcousticSignalCarriesNoIdentity` — il DTO del segnale non contiene `SourceUnitId`,
`NoiseType` né `Intensity`, e la verifica è sull'**assenza dei campi**, non sul loro valore. Più la voce
PIE `PIE-V01-NOISE`, che è l'unica cosa qui che nessun test automatico può misurare.

---

## 7. Decisioni nuove da registrare

Quattro voci per il [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md).

🔴 **Questo branch ha dovuto rinumerare DUE volte in un giorno, ed è la misura di quanto `D-nnn` sia una
risorsa contesa.**

1. **Mattina**: la voce sul perimetro di D-146/D-183 era nata `D-196`, che su `origin/main` era già preso
   — la decisione sul corpus golden. Misurato allora: `origin/main` a **`D-212`**, la PR aperta **#1481**
   su **`D-213`**. Rinumerate a `D-214` (perimetro) e `D-215` (fog of war in v0.1).
2. **Sera**, al momento del merge: `origin/main` era passata a **`D-220`**, e il branch aperto
   `claude/spec-panel-map-scenario-menu-*` rivendicava **`D-214` … `D-217`** con tesi tutt'altre
   (affordance di sviluppo, menu degli scenari). Seconda rinumerazione: `D-221` il perimetro,
   `D-222` la fog of war. **`D-223`** è la regola sul momento del verdetto (§3.5, chiude `#1496`),
   registrata il 2026-08-27.
3. **Il giorno dopo, al merge vero**: entrambe erano state prese da `origin/main`. Terza rinumerazione, e
   quella che vale: **`D-228`** il perimetro, **`D-225`** la fog of war, `D-223` invariato. Le tesi ancora
   da registrare partono quindi da **`D-230`** — numero che si riverifica di nuovo prima del proprio merge,
   perché è esattamente ciò che questo elenco dimostra tre volte.

🔴 **Terza collisione, trovata il 2026-08-27 — ✅ riparata il 2026-08-28, al merge.** `origin/main` ha
assorbito un **`D-221`** proprio — *«un colpo è un concetto solo, `bCountsAsAttack`»*, che chiude `INT-8` e
`#1491` — mentre questo branch usava lo stesso numero per il perimetro di D-146/D-183. Sono **due tesi
diverse con lo stesso ID**, e per la regola di [`AGENTS.md`](../../../AGENTS.md) la seconda è quella non
ancora mergiata: è stata rinumerata **questa**, non quella. Il perimetro è ora **`D-228`**.

🔴 **E la stessa riga dichiarava `D-222` libero: era vero quando è stata scritta, ed è durato un giorno.**
Diceva *«`D-222` (fog of war) invece non collide: su `origin/main` è libero»*. Rimisurato il 2026-08-28
immediatamente prima del merge, `origin/main` porta un **`D-222`** proprio — *«lo sviluppo parallelo si
accetta come regime, e ciò che si protegge è la MISURA»* — che nel frattempo è finito in `CLAUDE.md`,
`AGENTS.md` e `CONTEXT_INDEX.md`. **Quarta collisione.**

✅ **Ma la fog of war non è stata rinumerata: è stata ADOTTATA, ed è un esito migliore.** Rinumerandola si
sarebbe scritta una voce nuova — e `origin/main` porta già **`D-225`**, *«la fog of war della v0.1 NASCONDE
la geometria che nessuno della squadra osserva»*: **la stessa tesi**, registrata da un'altra sessione
mentre questo branch la teneva ferma sotto un numero proprio. Una rinumerazione meccanica avrebbe messo nel
documento canonico **due voci per una decisione sola** — difetto peggiore della collisione, perché con ID
diversi nessun controllo di unicità lo vede. Tutti i riferimenti di questa spec puntano ora a `D-225`.

∴ **prima di rinumerare si confronta la TESI, non solo l'ID.** Due numeri diversi sulla stessa decisione
non collidono per costruzione, e proprio per questo passano.

∴ **la lezione non è che la misura fosse sbagliata: è che una misura di disponibilità vale finché non la
si usa.** Questo documento conteneva già la regola giusta — *«`git fetch --prune origin` + `git branch -r`
immediatamente prima del merge»* — e la collisione è arrivata lo stesso, perché fra lo scrivere il numero e
il mergiarlo è passato un giorno. Un ID scritto in un branch non pushato **non è prenotato**: è
un'intenzione, e la prenotazione avviene solo al merge.

⚠️ **`D-223` invece NON è stato toccato, e la ragione è misurata**: l'unico ref remoto che lo rivendica è
`origin/fix/1497-lastmoveroutes-porta-identita`, che porta **la stessa identica tesi** — è questo lavoro,
pushato sotto un altro nome, non un secondo autore. Verificato con `git rev-list`: **0** commit presenti là
e non qui. Rinumerarlo avrebbe creato una divergenza dove non c'era un conflitto.

⚠️ **La prima misura non era sbagliata: era scaduta.** Fra le due sono passate poche ore, e in mezzo
`origin/main` ha assorbito otto decisioni. Per questo la regola non è «misura una volta e scrivi»: è
`git fetch --prune origin` + `git branch -r` **immediatamente prima del merge**, e su **tutti** i
riferimenti remoti, non solo su `main` — la collisione della sera stava in un branch mai mergiato.

⚠️ **Il Decision Log locale ha un BUCO, non una coda corta.** Rimisurato il 2026-08-27: il log di questa
working directory arriva a **`D-222`** ma non contiene **nessuna** delle venticinque voci `D-196`…`D-220`,
che esistono su `origin/main`. Il branch è **164 commit indietro e 44 avanti** — le due linee divergono nei
due sensi. Conseguenza pratica: *«qual è il primo numero libero»* **non** si legge dal massimo locale, e
nemmeno dal massimo remoto da solo. Si guardano entrambi, più i branch remoti vivi.
*(Questa riga diceva «si ferma a `D-196`» e «116 commit indietro»: entrambi i numeri erano scaduti, ed è
esattamente il difetto che la riga sopra descrive.)*

🔴 **`D-nnn` è una risorsa contesa**: prima del merge, `git fetch --prune origin` e `gh pr list --state open`
per gli ID in volo. Questa misura invecchia, e una collisione di contatore è già successa tredici volte.

| Tesi | Cosa emenda |
|---|---|
| **La grammatica visiva della conoscenza**: sagoma volumetrica semitrasparente sia per *Last Contact* sia per *Action Ghost*, distinte su **due** canali — monocromo e senza facing il ricordo, colori di squadra e facing l'intento | `progettazione-hud.md` §9 e §25 |
| **La regola derivata per l'intensità di rumore**: tre classi valutate in ordine — *Impatto* `6` (danno, `StructureOp`, o `MovementStyle` non-`Budget`), *Passo* `max(0, RangeCells − 3)`, *Silenziosa* `0` come residuo | Chiude `AE-8`; popola la colonna `Rumore` del catalogo azioni; risolve la contraddizione con D-123 sulle signature |
| **L'area acustica di squadra è quella dell'ascoltatore col MARGINE MAGGIORE**, ed è dichiarata **euristica**: `PlausibleOriginCells` deriva il raggio dal margine, non dalla distanza percorsa, quindi non garantisce di contenere la sorgente | Colma un buco che nessun documento aveva posto. ⚠️ **Non è l'intersezione**: quella tesi è stata scritta e ritirata lo stesso giorno (§6.4), perché intersecare due stime imprecise dà un'area più piccola, più sicura di sé e più sbagliata |
| **Correzione alla prosa di D-113**: margine `0` **è** contatto | `IsAudible` è `ReceivedNoise > 0 && ReceivedNoise >= HearingThreshold`, e la verifica di scala di D-041 concorda col codice. Due fonti contro una: la prosa di D-113 è l'anomalia |

---

## 8. Test e gate

| Fase | Test |
|---|---|
| **A** | `Knowledge.ViewOmitsHidden` — il DTO non contiene la cella attuale di un ignoto<br>`Knowledge.ViewIsIndependentOfHiddenState` — due stati autoritativi diversi (nemico in A oppure in B) producono lo **stesso** `FRTKnowledgeView`<br>`Knowledge.LastContactCarriesIdentityNotCondition`<br>`Knowledge.HudDrawsOnlyKnownUnits`<br>`Knowledge.CombatLogOmitsUnknown`<br>`Knowledge.UnitRenderingCombinesAliveAndKnown`<br>`Knowledge.GhostFadesWithContactAge`<br>`TurnLog.TargetUnknownIsDescribed` |
| **B** | `Veil.CoversExactlyUnobservedCells`<br>`Veil.FollowsRefreshPoints`<br>**estensione di `Hex.SurfaceColorsAreDistinguishable` ai colori velati** |
| **C** | I **sei** del DoD di #159: `Noise.ProducesUncertainContact` · `Noise.AttackRevealsDirection` · `Noise.ObserverViewOmitsUnheard` · `Noise.HashIsIndependentOfObserver` · `Noise.MemoryDoesNotTrackUnseenSource` · `Noise.NoHiddenIntentLeak`<br>più `Noise.SilentActionEmitsNothing` (§6.3) · `Noise.IntensityFollowsDerivedRule` (§6.1) · `Noise.TeamTakesBestInformedArea` e `Noise.AreaIsHeuristicNotGuarantee` (§6.4) |

🔴 **`Knowledge.ViewIsIndependentOfHiddenState` non è un test in più: è un debito già iscritto.**
[D-143](../../decisions/RT_PDR_00_Decision_Log.md) dichiara che *«il primo consumatore che introduca overlay
di conoscenza dovrà portarsi il test dietro»*. Questa spec è quel consumatore. È il gemello umano di
`HexBotPlay.HiddenEnemyFairness`, che oggi tiene onesto **solo** il bot.

**Verifiche PIE**: `PIE-V01-VISION` e `PIE-V01-NOISE` (già pianificate in #160), più una voce nuova per la
leggibilità del velo — che è l'unica cosa di questa spec che nessun test automatico può misurare.

**Golden**: entrambi i `.rttl` di `Tests/Golden/` sono scenari di **movimento**, e l'emissione per passo li
fa divergere. Vanno rigenerati **nello stesso commit** della Fase C, e la procedura
(`rt.Test.RegenerateGolden 1`) entra in `docs/`: oggi è documentata **solo nel sorgente**, e un corpus
rigenerabile senza procedura scritta è un corpus che verrà rigenerato per far passare un test.

---

## 9. Precondizioni operative

Tutte e tre bloccanti.

1. 🔴 **L'albero di lavoro va liberato.** `Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset` è modificato e
   `WBP_RT_TurnHeader.uasset` è untracked, sopra CP 11.7
   ([#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613), aperta). La Fase A crea un
   materiale nuovo: due `.uasset` non si fondono, e
   [D-178](../../decisions/RT_PDR_00_Decision_Log.md) dice una sessione, una working directory, un branch.
2. 🔴 **La PR [#1428](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1428) tocca
   `Turn/RTTurnManager.cpp` e `.h`** — esattamente i file dell'emissione. La Fase C non parte prima che sia
   mergiata, oppure il conflitto si accetta consapevolmente.
3. **La numerazione `D-nnn` va riverificata** prima del merge (§7): `D-221`/`D-222` erano liberi il 2026-08-27.

### Dove atterra il lavoro

| Fase | Issue |
|---|---|
| **A** — porta, leak, unità nascoste, sagoma, `TargetUnknown` | **da aprire**: è metà porta e metà *difetto* — i due leak di §1.3 non sono nel DoD di nessuno |
| **B** — velo | **da aprire**: è un checkpoint mancante |
| **C** — rumore | [#159](https://github.com/DegrassiAaron/refactor-tactics-main/issues/159) (CP 13.4) |
| chiusura | la casella HUD di [#160](https://github.com/DegrassiAaron/refactor-tactics-main/issues/160) si spunta quando A, B e C sono tutte a terra |

Le Fasi A e B **non si allargano dentro #160 in silenzio**: servono due issue proprie.

---

## 10. Fuori perimetro, dichiarato

- **La fetta 2 — fog of war sul terreno.** Il meccanismo di §5.3 è il suo substrato: quella fetta cambia il
  **trattamento** delle celle non osservate, non costruisce una seconda macchina.

  > 🔴 **`N1` (2026-08-27): l'autore chiede che la fog of war NASCONDA la parte di mappa che nessuno della
  > squadra osserva** — graybox come resa provvisoria, effetto fumo/nebbia in seguito. *(«Nebbia» è qui la
  > **meccanica** in senso figurato, da non confondere con `Terrain.Smoke`, che è una superficie di gioco
  > con regole proprie.)*
  >
  > **Non è un emendamento a `progettazione-hud.md` §25: è un cambio di perimetro della release.**
  > `piano-canonico-mvp.md` — che questo documento dichiara sovraordinato — classifica la fog of war come
  > **P1**, cioè fuori dalla v0.1, e il brief scrive *«lo slice non è fog of war: la mappa statica resta
  > nota»*. Serve una **`D-nnn` di scope**, non una nota a piè di pagina.
  >
  > ⚠️ **La richiesta è fondata, ma su una premessa DICHIARATA, non misurata — e la prima stesura di questo
  > riquadro non lo diceva.** L'arena di riferimento è un esagono di **lato 50 → 7 351 celle** (`3R² + 3R + 1`
  > con `R = 49`, la convenzione di `URTHexLibrary::HexArea`): l'aritmetica regge, ma quell'arena è un
  > **intento dell'autore**, non un oggetto del repository. Misurato il 2026-08-27: `git grep "7351"` la trova
  > **solo in questo documento**, e le arene versionate stanno a `DemoArenaRadius = 4` — **61 celle** — con
  > `HexArea` a raggio 3, 4 e 5.
  >
  > 🔴 **La differenza ribalta la conclusione, e va detta.** Su 7 351 celle il cono di Gadget (vista 7,
  > `R² + 2R` ≈ 74 celle) copre l'**1 %**, e una squadra di due sta intorno al **2 %**: disegnare il 98 %
  > spento è rumore visivo, e nascondere è la scelta giusta. Su **61** celle lo stesso cono copre **l'arena
  > intera**, non c'è niente da nascondere, e il velo basta e avanza.
  >
  > ∴ **la decisione di scope non si può prendere finché non è chiaro su quale arena si gioca.** Questo
  > documento aveva accusato la §5.1 di aver scelto il velo su *«una premessa mai misurata»*, e le aveva
  > sostituito una premessa non misurata a sua volta. Il difetto è lo stesso, ed è per questo che la riga resta.

  > ⏳ **Resta una misura da prendere prima di stimare il costo**: se la geometria visibile di un livello
  > venga dagli `InstancedStaticMeshComponent` che `ARTHexMapActor` governa — nel qual caso nascondere è lo
  > stesso meccanismo di §5.3 — oppure da attori posati nel `.umap`, nel qual caso è un sistema nuovo.
- **L'Action Ghost** (#249): questa spec ne fissa la grammatica, non lo implementa.
- **Il profilo `Sneak`**: la regola derivata ha bisogno di un budget in `RangeCells` con `MovementStyle::Budget`, e `Sneak` non ne ha uno — `AE-5` resta aperta.
  **`AE-5` resta aperta**, e §6.1 **non** la chiude. Dirlo qui è l'unico modo perché nessuno creda il
  contrario leggendo che `AE-8` è chiusa.
- **L'identificazione del tipo di rumore** (Livello 5) e i cinque livelli del §13 sorgente.
- **Rete e privacy** (M10): questa spec le prepara per costruzione, essendo offline e senza replica.
- **`ARTCameraPawn::FrameOwnTeam`**, ed è fuori per una ragione precisa che va scritta perché è il tipo di
  cosa che qualcuno chiuderà per sbaglio credendola inclusa. D-143 lo nomina come debito noto: itera
  `TActorIterator<ARTUnit>` su **tutto** il mondo, quindi *«non interroga lo stato autorevole» è falso
  oggi*. **Ma non è un terzo leak**: valuta `It->TeamId == TeamId && It->IsAlive()` e calcola il centroide
  della **propria** squadra, quindi nessuna informazione avversaria esce dalla camera. È un debito di
  **disciplina**, non di informazione, e confonderlo con i canali elencati in §1.3 gonfierebbe la Fase A
  con lavoro che non serve a nascondere niente. *(Diceva «i due canali di §1.3» finché quella sezione ne
  contava due: la cifra è caduta con `#1497`, il rimando resta.)* La porta di §3 lo rende *riparabile*; ripararlo è un'altra
  issue.

---

## 11. Rischi

| Rischio | P/I | Mitigazione |
|---|---|---|
| Il velo compra un **falso verde**: il gate misura il colore non velato | **H/H** | §5.5 — l'estensione del gate è parte della Fase B, non un seguito |
| Si chiude la Fase A senza i due leak, e la fog resta cosmesi | M/**H** | §1.3 e A2: i leak sono **dentro** la Fase A, non una issue a valle |
| L'emissione hardcoda le intensità perché `AE-8` non è chiusa in tempo | M/**H** | §6.1 è il **gate d'ingresso** della Fase C: la regola e la colonna vengono prima dell'emissione |
| I golden vengono rigenerati per far passare un test | M/**H** | La procedura entra in `docs/` nello stesso commit (§8) |
| Aggiungere `Acoustic` a `ERTLogCategory` in un sito solo | **H**/M | Precedente misurato: `ReactionClash` è stata dimenticata in due dei tre siti |
| La sagoma viene calcolata risolvendo `StableUnitId → ARTUnit*` per prendere la mesh | M/**H** | Sarebbe il *«ricevi e nascondi»* vietato: il DTO porta la **chiave visiva**, mai il puntatore all'unità |
| L'area acustica **non contiene** la sorgente: il raggio deriva dal margine, la distanza percorsa è `Budget − Received`, e sono scorrelate | M/**H** | §6.4 — l'area è dichiarata **euristica**, e `Noise.AreaIsHeuristicNotGuarantee` costruisce il controesempio invece di fingere che non esista |

---

## 12. Divergenze documentali trovate durante la stesura

Registrate qui perché **non sono lavoro di questa spec**, e chi le incontra altrove deve sapere che sono
note.

| Divergenza | Misura |
|---|---|
| Il DoD di #159 dice *«`ERTLogCategory` ha oggi **sette** valori»* | Ne ha **dieci**: `Move · Combat · Fallback · Reaction · Environment · Facing · Predictive · ReactionDecision · ReactionClash · Status` |
| Il DoD di #159 dice che la versione di formato *«sale da `WithPriority = 7` a **8**»* | L'ultima è `WithReactionResponse = 10`: la prossima libera è **11** |
| La prosa di D-113 dice *«margine ≤ 0 → nessun contatto»* | `IsAudible` è `ReceivedNoise > 0 && ReceivedNoise >= HearingThreshold`, e D-041 concorda col codice (§7) |
| `docs/roadmap/v0.1-issue-plan.md` §#159 elenca **6** caselle | La issue su GitHub ne ha **9**: il file locale è stantio di tre. Autorità = GitHub |
| Il commento su `Action.Wait` dice che il silenzio non produce un evento udibile | Falso su superficie a delta positivo (§6.3) |
| `progettazione-hud.md` §26 ammette che la modalità Sound mostri **categoria** e **intensità** | Entrambe vietate da D-113 e D-123. Il documento HUD è più permissivo delle decisioni |
