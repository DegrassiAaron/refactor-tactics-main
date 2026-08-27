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

### 1.3 Due canali stampano a schermo ciò che la squadra non sa

Non erano nel DoD di nessuno, e sono **portanti**: senza chiuderli, nascondere il modello è teatro.

| Canale | Cosa perde | Misura |
|---|---|---|
| `ARTHUD::DrawHUD` | **nome eroe, barra HP e scudo di ogni unità viva, nemici inclusi** | itera `GetAllActorsOfClass(ARTUnit)` e l'unico filtro è `if (!Unit \|\| !Unit->IsAlive()) continue;` |
| Combat log | **la cella esatta di ogni movimento** e i **punteggi di utility del bot con cella e bersaglio** | `AddLogEvent` li scrive; `ARTHUD` disegna `GetRecentEvents()` senza gate |

⚠️ Un terzo dettaglio: `ARTHUD::DrawHUD` chiama `ComputePlannedHitMarks(AllUnits, /*PlayerTeamId=*/ 0, …)`
con lo **zero scritto a mano**. L'osservatore oggi è cablato in un letterale.

### 1.4 Il substrato non esiste — tre buchi distinti

> ⚠️ Misurati contro un **velo**. Con la fog decisa da
> [D-215](../../decisions/RT_PDR_00_Decision_Log.md) **i primi due cadono**: vedi §5.3.

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
> e attribuiva a `Detected` la **condizione** e a `CellOnly` il **turno di scadenza**: `FRTKnowledgeEntry` non
> ha né l'una né l'altro, e il piano lo vieta esplicitamente («*un campo qui costringerebbe a inventarne il
> valore*»). La spec contraddiceva il piano dello stesso branch. I nomi veri sono
> `ERTKnowledgeVisibility::{Live, Remembered}`, e il terzo caso **non ha un nome** perché non ha una voce.

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
> [D-215](../../decisions/RT_PDR_00_Decision_Log.md) ha deciso che la fog of war **entra nella v0.1 e
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
la propria *posizione*. È la distinzione che [D-215](../../decisions/RT_PDR_00_Decision_Log.md) scrive, e
senza la quale la fog sembrerebbe violare §25 mentre lo rispetta.

Il rapporto con [D-146](../../decisions/RT_PDR_00_Decision_Log.md) è chiarito da
[D-214](../../decisions/RT_PDR_00_Decision_Log.md) (2026-08-27), e **non c'è conflitto**: D-146 governa come
una cella **mostrata** comunica la propria superficie — è una regola di *encoding*, non di *visibilità*. Non
dice **se** una cella vada mostrata. Una cella che nessuno osserva, e che quindi non si legge, non è un
fallimento di leggibilità: è lo scopo.

D-214 dichiara anche il perimetro che entrambe presupponevano: **la forma è graybox e cadrà** con i
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

⚠️ **La collisione su `D-196` è stata risolta il 2026-08-27 rinumerando a `D-214` la voce di questo branch.**
Su `origin/main` esisteva già un `D-196` diverso — quello sul corpus golden. Misurato allora: `origin/main`
era a **`D-212`** e la PR aperta **#1481** rivendicava **`D-213`**, quindi il perimetro di D-146/D-183 è
`D-214` e la fog of war in v0.1 è **`D-215`**. Le tesi qui sotto ancora da registrare partono da `D-216`.
⚠️ **Il Decision Log locale si ferma a `D-196`**: il `main` di questa working directory è **116 commit
indietro** rispetto a `origin/main`, per scelta dichiarata dall'autore. La numerazione qui è corretta
rispetto a `origin/main` — che è dove finisce — e lascia un buco visibile in locale.

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
3. **La numerazione `D-nnn` va riverificata** prima del merge (§7): `D-214`/`D-215` erano liberi il 2026-08-27.

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
  **disciplina**, non di informazione, e confonderlo con i due canali di §1.3 gonfierebbe la Fase A con
  lavoro che non serve a nascondere niente. La porta di §3 lo rende *riparabile*; ripararlo è un'altra
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
