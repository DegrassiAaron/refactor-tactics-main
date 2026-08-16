# Mini Roadmap v0.1 Autobattle — spec panel e triage del consolidamento

> `CURRENT` · **Stato**: revisione chiusa, residuo applicato in questa PR · **Data**: 2026-08-16
> **HEAD della revisione**: `80d967ee` (`origin/main`, «Merge pull request #947»), worktree `D:/rt-issue`,
> branch `docs/mini01-consolidamento-autobattle`
> **Sorgente revisionato**: `RefactorTactics — Prompt consolidato Mini Roadmap v0.1 Autobattle.md`
> (1664 righe, **38 sezioni numerate**, untracked nella radice del checkout principale), archiviato a fine
> sessione in
> [`../../archive/src/RefactorTactics_Mini_Roadmap_v01_Autobattle_Claude_Consolidation_2026-08-16.md`](../../archive/src/RefactorTactics_Mini_Roadmap_v01_Autobattle_Claude_Consolidation_2026-08-16.md)
> **Panel**: Wiegers (lead) · Cockburn · Adzic · Fowler · Nygard · Crispin
> **Modo**: debate · **Focus**: requirements + architecture
> **Regola applicata**: un prompt di consolidamento è l'**ultima** fonte della gerarchia
> ([`../../CONTEXT_INDEX.md`](../../CONTEXT_INDEX.md) §2). Dove contraddice una `D-nnn`, un gate definito o
> un fatto misurabile sul branch, prevale il repository e la proposta si **registra**.

---

## 1. Il verdetto in una riga

Il documento chiede di costruire una corsia accelerata verso una demo automatica e osservabile, e la
descrive come se il repository fosse vuoto: il bot che propone come «MVP da costruire dopo» è
`RELEASE_READY` da otto giorni, e la partita 2v2 bot-contro-bot che vuole vedere **gira già headless**
(`RefactorTactics.HexMatch.PlaysToCompletion`, che il proprio commento misura al **turno 10**).

Le sue sette release intermedie, voce per voce contro il repository — perché un «quasi tutte» non si
verifica:

| Release | Stato misurato |
|---|---|
| **a1** Hex Board | ✅ **in `main`**: `FRTCellId`, coordinate, vicini, Cell ↔ World, renderer ISM, camera, quattro unità, debug celle |
| **a2** Auto Move | ✅ **in `main`**: grafo hex, A\*, costi, Move Intent, snapshot immutabile, resolver, micro-step, occupancy, TurnLog, playback |
| **a3** Auto Skirmish | 🟡 **completa nella simulazione, assente in partita**: enumerazione legale, attacco base, HP/KO, `Wait`, policy del bot, fine partita e `RoundLimit` esistono e sono testati; ciò che manca è **osservarla** senza una mano umana |
| **a4** Tactical Board | 🟡 terreni ed effetti chiusi con **E8**, coperture con **E9**; gli **obiettivi** sono `RT-FEAT-OBJECTIVE-SYSTEM` `IMPLEMENTING` — la partita finisce per obiettivo, ma non c'è nulla da attivare in mappa |
| **b1** Scenario Runner | 🟡 l'harness esiste e gira dal percorso reale, ma **enumera i turni**: nessun free-run |
| **b2** Watchable Build | 🟡 HUD e combat log ci sono; **velocità di playback, schermata vincitore e restart** no (E11, E21, E46) |
| **rc1** Determinismo | 🟡 `ReplayDivergenceZero` e l'invarianza per permutazione ci sono; il **corpus dei casi limite** dell'autobattle no |

**Due complete, cinque parziali** — e la parziale che conta è `a3`, che il documento stesso dichiara a
priorità assoluta.

Ciò che resta, dopo il triage, è **piccolo, vero e prezioso**: nessuno può *guardare* quella partita.
`ARTGameMode::SpawnHero` assegna il bot alla sola squadra 1 (`RTGameMode.cpp:547`), quindi in PIE serve
sempre una mano umana per la squadra 0 — e le nove verifiche `PIE-HEXPLAY` che tengono aperti **M6**, l'epic
**E2** e i gate **G10**/**G13** chiedono proprio a una persona di giocare una partita intera per poterla
osservare. La mini-roadmap, ridotta a ciò che il repository non ha, è **un interruttore e un ritmo**: la
squadra 0 guidata dal bot e un `PlanningSeconds` corto. Da lì la partita si guarda invece di giocarla, e
quattro gate di release smettono di dipendere dalla pazienza di chi tiene il mouse.

Il documento contiene il proprio antidoto, e va citato perché è la lettura più giusta che se ne possa dare:
§1 prescrive `REUSE / UPDATE / NEW / SUPERSEDED` prima di toccare qualsiasi cosa, §13 dice *«creare o
**estendere** l'infrastruttura scenario esistente»*, §26 *«non duplicare feature già presenti»*, §30 *«non
creare una seconda dashboard»*. Questo referto è l'esecuzione di quei quattro punti.

---

## 2. Il conto

| Classe | Sezioni | Significato |
|---|---:|---|
| `CURRENT` | **13** | descrive come da fare una pratica che il repository ha già |
| `DUPLICATE` | **7** | riscrive in un secondo posto contenuto che esiste |
| `CONFLICT` | **4** | creerebbe un secondo owner di un modello già deciso |
| `SUPERSEDED` | **3** | il repository è **oltre** la proposta: applicarla sarebbe una regressione |
| `PROPOSED` → accettato | **9** | idea nuova, nessun conflitto: entra nel tracking con questa PR |
| meta / procedura | **2** | il formato del referto: eseguito, non recepito |

**38 sezioni.** Si rimisura con `grep -c '^# [0-9]' <sorgente>`. La somma delle classi — `13+7+4+3+9+2` —
è il controllo che il totale da solo non offre.

Il rapporto ha la forma che i triage precedenti di questa cartella hanno già misurato — il repository era
avanti alla fonte — con **una differenza che vale la pena isolare**: qui le tre `SUPERSEDED` non sono
materiale invecchiato, sono **istruzioni a ridurre lo scope al di sotto di ciò che è già consegnato**.
Un consolidamento eseguito alla lettera avrebbe tolto azioni dal catalogo e sostituito un bot a utility
scoring con una scelta casuale fra mosse legali.

---

## 3. `SUPERSEDED` — le tre istruzioni che sarebbero una regressione

> **WIEGERS**: «Un requisito che chiede *meno* di ciò che è consegnato non è un requisito di scope
> ridotto: è un requisito di **rimozione**, e va dichiarato come tale prima di essere eseguito.»

### 3.1 §5 — «implementare inizialmente soltanto Move, Basic Attack, Wait»

Motivazione della sorgente: *«non aspettare il kit completo dei personaggi»*.

**Il kit non si aspetta: è chiuso.** L'epic **E6** (roster 4 eroi) è `CLOSED` su GitHub (`#20`), il registry
dichiara `RT-FEAT-CHAR-V01-ROSTER` `INTEGRATED`, e `RT_ActionCatalog_v0.1.md` elenca il set base
`Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch` più le abilità d'eroe. `Action.Wait`
**esiste già a catalogo** (riga 97 del catalogo), quindi anche la terza primitiva richiesta è REUSE.

Verdetto: **REUSE integrale**. La mini-roadmap non tocca il set di azioni.

### 3.2 §7 e §12 — «prima un bot che sceglie a caso fra le mosse legali, poi utility scoring»

La sorgente propone una progressione `random legal choice → Enumerate → Evaluate → Score → Weighted
deterministic choice`, e la colloca **dopo** `a3`.

`RT-FEAT-BOT-BASE` è `RELEASE_READY`, epic E2 / CP 2.6: `URTHexBotLibrary` espone già
`BuildCandidates → ScorePlan → ChooseBestPlan`, con quattro famiglie di candidate (riposizionamento,
attacco da fermo, scatto+attacco, scatto di riposizionamento) e le candidate che **nascono da
`ReachableCells`**, cioè nessuna mossa illegale è proponibile. È esattamente il punto d'arrivo di §12,
raggiunto il 2026-08-06.

> **FOWLER**: «La progressione proposta non è sbagliata — è già stata percorsa. Il difetto è che il
> documento la presenta come lavoro futuro invece che come vincolo di compatibilità: qualunque cosa la
> mini-roadmap aggiunga deve passare da `ChooseBestPlan`, non affiancarlo.»

Verdetto: **REUSE**. Resta valido, e va scritto, il vincolo di §5 che il documento formula meglio di
chiunque: *«i bot devono utilizzare le stesse primitive/intenti che in futuro useranno giocatori e altre AI.
Nessuna pipeline speciale parallela dedicata ai bot»* — che è l'invariante #10 del `CONTEXT_INDEX`.

---

## 4. `CONFLICT` — quattro modelli che il repository ha già deciso diversamente

### 4.1 §4 e §11 — `Normal · Rough · Cover · Hazard · Objective` come tassonomia **di cella**

La sorgente chiede una prima tassonomia MVP di cinque categorie di cella, ciascuna con colore **e**
pattern/glyph. Misurata contro il modello dati reale:

| Categoria della sorgente | Dove vive davvero | Nota |
|---|---|---|
| `Normal` | `ERTHexSurface::Floor` | ✅ corrisponde |
| `Rough` | `ERTHexSurface::Rough` | ✅ corrisponde |
| `Cover` | **`FRTHexCover` su un BORDO**, non sulla cella | ⚠️ modello diverso |
| `Hazard` | `Fire` · `Ice` · `Conductive`+`ShallowWater` + stati temporanei | ⚠️ molti-a-uno |
| `Objective` | entità di mappa (epic **E10**), non una superficie | ⚠️ modello diverso |

`ERTHexSurface` ha **nove** valori, non cinque, e la copertura è **direzionale per bordo** da **E9.1**
(`docs/gameplay/spec-copertura-cp91.md`, formato mappa v3): *«la direzionalità è del BORDO, non
dell'unità: girarsi non sposta un muretto»*. Introdurre `Cover` come categoria di cella creerebbe il
secondo modello di copertura, che è precisamente ciò che §11 della sorgente vieta — *«riutilizzare
eventuali primitive di cover già implementate. Non creare un secondo sistema»*.

**Risoluzione**: la tassonomia a cinque voci è accettata **come vocabolario di presentazione**, mai come
dato. È una *legenda* che il renderer deriva dallo stato esistente (superficie della cella + coperture dei
bordi + entità obiettivo), e non aggiunge nessun `enum` al modello. Il vincolo che ne resta — e che è la
parte migliore di §4 — è **ridondanza dell'encoding**: mai solo il colore.

> **NYGARD**: «La ridondanza colore+forma non è un vezzo estetico. È la stessa classe di requisito di un
> log che nomina la causa: se l'unico canale fallisce — daltonismo, screenshot in scala di grigi, video
> ricompresso — la lettura non degrada, sparisce.»

Misurato: il colore per superficie **esiste già in partita**. `ARTHexMapActor::CellMaterial` legge i tre
`PerInstanceCustomData` che `RebuildInstances` scrive e li usa come colore, *«così ogni cella si legge per
superficie»* (`RTHexMapActor.h:59-69`). Il secondo canale — pattern/glyph — **non esiste**, in nessuna
forma, né in partita né nell'overlay dell'editor.

### 4.2 §18 — «utilizza massimo TRE processi: Spatial · Simulation · Client»

Il repository ha `docs/roadmap/parallel-batch.yaml` (`schema_version: 3`, decisione **D-139**) con **sei**
track dichiarate — `spatial`, `simulation`, `client_tools`, `content_editor`, `playtest`, `verification` —
e il modello a *«quattro processi, tre agenti e un umano davanti all'Editor»* è già stato triagiato il
2026-08-14 ([`quattro-processi-paralleli-triage-2026-08-14.md`](quattro-processi-paralleli-triage-2026-08-14.md)).

Le tre responsabilità della sorgente **coincidono** con `spatial`, `simulation` e `client_tools`. Il
conflitto non è nel contenuto, è nel **numero fisso**: «massimo tre» eliminerebbe la track umana
(`playtest`, mandato `qa-prompt-terminal-d-verifiche-pie.md`), che è proprio quella che la mini-roadmap
esiste per sbloccare.

**Risoluzione**: la ripartizione di §18 è recepita come **mappatura sulle track esistenti**, dichiarata
al §8 di questo referto. Il `parallel-batch.yaml` **non viene toccato da questa PR** — la sua intestazione
dichiara il lotto `ACTIVE` con due track vive e una domanda aperta sulla chiusura, e un settimo write-set
scritto da una sessione documentale è esattamente il difetto che quel file registra cinque volte.

### 4.3 §24 — «associare le issue a `0.1-a1 … 0.1-rc1`» come Release field

Su GitHub il lavoro è raggruppato **per fetta di release**, non per milestone `M<n>` né per release
intermedia, dal 2026-08-09
([`organizzazione-milestone-github-2026-08-09.md`](organizzazione-milestone-github-2026-08-09.md)): sedici
milestone, da `v0.1 · Mondo giocabile` a `v1.0 · Launch`. La regola scritta è che *«i nomi non usano mai
`M<n>`: i due spazi di numerazione collidono e GitHub non ha modo di disambiguare»*.

Otto milestone nuove `0.1-a1 … 0.1` creerebbero un **terzo** spazio di numerazione sugli stessi oggetti.

**Risoluzione**: le sette release intermedie diventano **checkpoint di una sola epic** (`E47`, §7), che è
la forma che il repository usa già per esprimere una progressione ordinata dentro una milestone. Le issue
restano nelle milestone esistenti.

---

## 5. `DUPLICATE` — sette sezioni che riscrivono owner esistenti

| § | La sorgente chiede | L'owner che esiste già |
|---|---|---|
| §12 | Bot Tactical MVP con scoring | `RT-FEAT-BOT-BASE` `RELEASE_READY` + `RT-FEAT-BOT-TACTICAL` (v0.2, E26) |
| §14 | HUD watchable, log compatto, restart | `RT-FEAT-UI-SCREEN-HUD` · `RT-FEAT-UI-COMBAT-LOG` · epic **E11**, **E21**, **E46** (`#934`, CP 46.4/46.5/46.6) |
| §15 | corpus di determinismo | gate **G4** + `docs/technical/qa-prompt-terminal-a-determinismo.md` + `RefactorTactics.Replay.Verifier.ResimulationIsDeterministic` |
| §16 | vertical slice v0.1 | epic **`#14`** *«Vertical slice 2v2 su hex — release v0.1»* |
| §19 | manifest di ownership per batch | `docs/roadmap/parallel-batch.yaml` (**D-139**), con Binary Asset Lease sui `.uasset` |
| §22 | formato «EDITOR CHECKPOINT #N» | `docs/roadmap/editor-sessions.yaml` → [`editormap.shortlist.md`](../editormap.shortlist.md) + `docs/technical/test-manuali-pie.md` |
| §26 | Feature Map con 16 voci | **15 delle 16 esistono** nel registry (§6.1) |
| §32 | Definition of Done in 10 punti | DoD trasversale + gate `G1`–`G15` di [`../v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) |

§32 merita una nota, perché è l'unica sezione che si autolimita correttamente: *«non modificare la
Definition of Done generale del progetto per questo motivo»*. Recepita alla lettera — la DoD non cambia.

---

## 6. `Existing vs Required` — la tabella che §36-B chiede

Misurata su `80d967ee`. **875 test dichiarati in 107 file**
(`grep -rhoE '"RefactorTactics\.[A-Za-z0-9_.]+"' Source/RefactorTactics/Tests/*.cpp | tr -d '"' | sort -u | wc -l`)
· **76 scenari** versionati in `Scenarios/` (77 file JSON, uno è `_redirects.json`) · **UE 5.8**
(`EngineAssociation` in `RefactorTactics.uproject`).

| Sistema richiesto | Stato misurato | Azione |
|---|---|---|
| `FRTCellId`, coordinate, vicini, Cell ↔ World | `RT-FEAT-MAP-HEXGRAPH` `RELEASE_READY` | **REUSE** |
| Hex graph, A\*, traversabilità, costi | `RT-FEAT-MAP-PATHFINDING` `RELEASE_READY` · path mediana **0,025 ms** | **REUSE** |
| Snapshot immutabile + resolver | `RT-FEAT-CORE-DETERMINISM` `INTEGRATED` · resolver **0,41 ms/turno** | **REUSE** |
| Movement resolution, occupancy, collisioni | `RT-FEAT-CORE-TURN` `RELEASE_READY` | **REUSE** |
| TurnLog, reason code, hash | `RT-FEAT-CORE-TURNLOG` `RELEASE_READY` · hash permutazione-invariante | **REUSE** |
| Basic Attack, HP, danno, KO | `RT-FEAT-ACTION-BASIC-ATTACK-PROFILES` `RELEASE_READY` | **REUSE** |
| `Wait` | `Action.Wait` a catalogo | **REUSE** |
| Legal Action Enumerator + decisione bot | `RT-FEAT-BOT-BASE` `RELEASE_READY` | **REUSE** |
| Fine partita: eliminazione, obiettivo, turn limit | `RT-FEAT-MATCH-END-CONDITIONS` `RELEASE_READY` (`ERTMatchEndReason`, `FRTMatchResult`) | **REUSE** |
| Terreni con costo e hazard | `RT-FEAT-ENV-TERRAIN` `INTEGRATED` (8 terreni) + E8 chiusa | **REUSE** |
| Copertura | `RT-FEAT-MAP-COVER` `INTEGRATED` — **per bordo**, non per cella | **REUSE** |
| Obiettivi | `RT-FEAT-OBJECTIVE-SYSTEM` `IMPLEMENTING`, epic **E10** aperta | **UPDATE** |
| Playback della risoluzione | `RT-FEAT-CORE-PLAYBACK` `INTEGRATED` — **nessun controllo di velocità** | **UPDATE** |
| Combat log | `RT-FEAT-UI-COMBAT-LOG` `RELEASE_READY` | **REUSE** |
| Camera tattica | `RT-FEAT-UI-TACTICAL-CAMERA` `IMPLEMENTING` | **REUSE** |
| Scenario Runner configurabile | `RT-FEAT-TEST-SCENARIO-HARNESS` `INTEGRATED` — turni **scriptati**, nessun free-run | **UPDATE** |
| `StateHash` / `LogHash` | `URTMatchStateHashLibrary::HashMatchState` · `HashTurnLog` | **REUSE** |
| Colore per superficie in partita | `CellMaterial` + `PerInstanceCustomData` | **REUSE** |
| **Pattern / glyph di cella** | **assente** in partita e in editor | **NEW** |
| **Match non presidiato da PLAY** | **assente**: `SpawnHero` fissa `bIsBotControlled = (TeamId == 1)` | **NEW** |
| **`MatchSeed` e stream RNG derivati** | `Seed` è nello scenario ma **dichiarato non consumato**; zero `FRandomStream` nel runtime | **NEW** *(differito, §9)* |
| **`RulesVersion` / `BotPolicyVersion`** | assenti come campi; `RT-FEAT-DATA-HASH` copre l'hash di regole e contenuti | **UPDATE** |
| Verifica su build packaged | `RT-FEAT-PROD-PACKAGED` `IMPLEMENTING` · **G12** ✅, **G13** 🟡 | **UPDATE** |

### 6.1 La Feature Map di §26, voce per voce

Quindici voci su sedici hanno già un `feature_id`. L'unica scoperta è **Auto Match**.

| §26 | `feature_id` | Stato |
|---|---|---|
| Hex Board | `RT-FEAT-MAP-HEXGRAPH` | `RELEASE_READY` |
| Pathfinding | `RT-FEAT-MAP-PATHFINDING` | `RELEASE_READY` |
| Turn Snapshot | `RT-FEAT-CORE-DETERMINISM` | `INTEGRATED` |
| Movement Resolution | `RT-FEAT-CORE-TURN` · `RT-FEAT-ACTION-MOVE-PROFILES` | `RELEASE_READY` |
| TurnLog | `RT-FEAT-CORE-TURNLOG` | `RELEASE_READY` |
| Basic Combat | `RT-FEAT-ACTION-BASIC-ATTACK-PROFILES` | `RELEASE_READY` |
| HP / KO | `RT-FEAT-MATCH-END-CONDITIONS` | `RELEASE_READY` |
| Bot Legal Actions | `RT-FEAT-BOT-BASE` | `RELEASE_READY` |
| Bot Decision | `RT-FEAT-BOT-BASE` | `RELEASE_READY` |
| **Auto Match** | — | ⚠️ **nessuna feature la rivendica** |
| Terrain Types | `RT-FEAT-ENV-TERRAIN` | `INTEGRATED` |
| Objective | `RT-FEAT-OBJECTIVE-SYSTEM` | `IMPLEMENTING` |
| Scenario Runner | `RT-FEAT-TEST-SCENARIO-HARNESS` | `INTEGRATED` |
| Playback | `RT-FEAT-CORE-PLAYBACK` | `INTEGRATED` |
| Combat Log | `RT-FEAT-UI-COMBAT-LOG` | `RELEASE_READY` |
| Determinism Verification | `RT-FEAT-CORE-DETERMINISM` · `RT-FEAT-TEST-GOLDEN` | `INTEGRATED` / `IMPLEMENTING` |

---

## 7. Il delta vero: `a3` è un interruttore, non una release

> **COCKBURN**: «Chi è l'attore primario di questa mini-roadmap? Il documento dice *il giocatore, che deve
> poter semplicemente guardare la battaglia*. Ma l'attore che oggi è bloccato è un altro: **chi deve
> eseguire le nove verifiche `PIE-HEXPLAY`** e non può, perché ognuna richiede di giocare una partita
> intera a mano.»

Questa è la lettura che cambia il valore della proposta, e regge alla misura.

**La catena si chiude già.** `ARTTurnManager::StartPlanningTimer()` avvia un timer di `PlanningSeconds`
(default **30 s**, `RTTurnManager.h:885`); alla scadenza `OnPlanningTimeout()` chiama `LockInAndResolve()`
(righe 783-787), e a fine risoluzione `StartPlanningTimer()` riparte (riga 1787). `StartPlanningTimer`
chiama `PlanBots()`. Quindi **il turno avanza già da solo**: non manca un motore di AutoReady/AutoCommit,
manca soltanto che *entrambe* le squadre siano guidate dal bot.

**L'unico punto che lo impedisce è una riga:**

```cpp
// Source/RefactorTactics/RTGameMode.cpp:547
Unit->bIsBotControlled = (TeamId == 1); // team 1 giocato dal bot
```

ed è **pinnata da un test** — `RTHeroSpawnTests.cpp:151-152`, *«il giocatore comanda i suoi»* / *«il bot
comanda i propri»* — quindi non è un default da cambiare: è un contratto da **estendere con una
configurazione**, lasciando il default dov'è.

**La prova che il resto regge** è già in suite: `RefactorTactics.HexMatch.PlaysToCompletion` costruisce un
2v2 con tutte e quattro le unità `bIsBotControlled = true` — *«nessuna mano umana, la partita si gioca da
sola»* — e verifica, turno per turno, che nessuna unità finisca fuori mappa o sovrapposta, che la partita
raggiunga `MatchEnded`, che serva più di un turno e che una squadra sia eliminata. Il commento del test
registra la misura: **la partita si decide al turno 10**, dentro il limite di 12 del catalogo v0.1.

> **ADZIC**: «Allora l'accettazione di `a3` si scrive in una riga, ed è falsificabile: *dato lo scenario
> autobattle e un seme di configurazione, premendo Play e non toccando più nulla, entro N minuti compare un
> vincitore e il TurnLog ha almeno una voce `Combat` e una `Move` per round giocato.* Se serve una
> pressione di tasto, `a3` non è passata.»

### 7.1 Perché vale più di una demo

Le nove voci `PIE-HEXPLAY-1..9` sono ⏳ e tengono aperti, misurato:

- **M6** «Parità hex» (`roadmap-checkpoint.md`: *«Ciò che la tiene aperta non è un'epic ma il playtest M6.8»*);
- l'epic **E2** (`#16`, `OPEN`), il cui gate di chiusura è *«`PIE-HEXPLAY-1..9` tutte ✅ e una partita 2v2
  completa fino alla vittoria»*;
- il gate **G10** *«Partita completa 2v2 su mappa multilivello, dall'avvio alla vittoria — playtest
  registrato (log o video)»*, ⏳;
- il gate **G13** *«Partita giocabile senza editor dalla build packaged»*, 🟡 con riserva dichiarata
  («sull'arena di test»).

Una modalità non presidiata non chiude quei gate da sola — restano verifiche umane — ma **cambia il costo
di ciascuna da «gioca una partita» a «guarda una partita»**, e rende registrabile in video ciò che oggi si
può solo raccontare. È il motivo per cui `a3` merita un'epic invece di una issue.

---

## 8. Ownership dei sei checkpoint, misurata

> 🔴 **La prima stesura di questa sezione mappava i tre processi di §18 sulle track per *affinità di nome*,
> e la code review l'ha falsificata su tre righe su quattro.** Diceva che la colonna di destra era *«il
> write-set che la track già dichiara in `parallel-batch.yaml`»*: non lo era. `Source/RefactorTactics/Camera/`
> **non compare nel `writable` di nessuna track**; la track `simulation` ha il write-set **vuoto** (`Turn/` e
> `Bot/` le sono stati esplicitamente rilasciati); e `Map/RTHexMapActor.{h,cpp}` — i file di `E47.3` —
> appartengono a `content_editor`, non a `client_tools`. Attribuire un file alla track sbagliata è peggio di
> non attribuirlo: fa credere che il blocco sia altrove, e `D-139` dice che **un file non assegnato è uno
> STOP**, non un via libera.
>
> La tabella qui sotto è ricostruita **leggendo `parallel-batch.yaml`**, non i nomi delle track.

| CP | File che tocca | Chi li possiede oggi | Conseguenza |
|---|---|---|---|
| **E47.1** | `RTGameMode.{h,cpp}` · `Turn/RTTurnManager.{h,cpp}` | ⚠️ **nessuna track** | **STOP**: serve una richiesta di allocazione prima di aprire |
| **E47.2** | `Turn/RTPlaybackLibrary.*` · `UI/` (se tocca l'HUD) | `Turn/…` **nessuna** · `UI/` = `client_tools` **ACTIVE** (#78) | doppio blocco: allocazione **e** attesa |
| **E47.3** | `Map/RTHexMapActor.{h,cpp}` | `content_editor` **ACTIVE** (#451) | attende il rilascio di quella track |
| **E47.4** | `Source/RefactorTactics/ScenarioHarness/` | `spatial` — **IDLE** | l'unico con un owner dichiarato e fermo; resta il blocco su [#542](https://github.com/DegrassiAaron/refactor-tactics-main/issues/542) |
| **E47.5** | `Source/RefactorTactics/Tests/` | `verification` — **IDLE, write-set vuoto** | non assegnato: **STOP** |
| **E47.6** | `docs/technical/test-manuali-pie.md` | `playtest` — **IDLE** | owner dichiarato e fermo: serve una riallocazione, non un'attesa |

⚠️ **Cinque checkpoint su sei toccano file che nessuna track ACTIVE può scrivere, e tre non hanno owner
affatto.** Non è un difetto di questa epic: è la fotografia del batch al 2026-08-16, e la conseguenza
pratica è che **il prossimo batch va aperto prima del codice**, non dopo. Il write-set dell'autobattle non
è stato scritto in `parallel-batch.yaml` da questa PR di proposito — il file dichiara il lotto `ACTIVE`
con due track vive e una domanda aperta sulla chiusura, e un settimo write-set aggiunto da una sessione
documentale è esattamente il difetto che quel file registra cinque volte.

**La corrispondenza con i tre processi di §18 resta valida come *ripartizione del lavoro***, ed è l'unica
cosa che quella sezione chiedeva davvero: `spatial` per la board e l'harness, `simulation` per il turno e
il bot, `client_tools` per presentazione e HUD, più la quarta track umana `playtest` che §18 non prevede e
che è proprio quella che la mini-roadmap esiste per sbloccare.

---

## 9. `PROPOSED` differito — il seed, e perché non entra adesso

§6 chiede `MatchSeed`, stream RNG derivati da identificatori stabili, `RulesVersion` e `BotPolicyVersion`.

Misurato: il runtime **non ha alcun RNG**. `FRTTestScenario::Seed` esiste ed è esplicitamente documentato
come *«seed dichiarato ma **non consumato**: oggi il progetto non ha alcun RNG e il determinismo viene
da…»* (`RTTestScenario.h:509-515`), e `FRTTestResult::Seed` lo registra nel report *«anche se oggi nessun
RNG lo consuma»*.

> **CRISPIN**: «Attenzione a cosa si sta comprando. Il determinismo oggi è una **proprietà strutturale**:
> non c'è casualità da controllare. Introdurre un seed per avere varietà significa sostituire una garanzia
> gratuita con una garanzia da mantenere — e il primo test che serve non è `SameSeedSameResult`, è
> *nessun percorso di gioco consuma un RNG non seminato*.»

🔴 **Quel test esiste già, e il panel se n'è accorto dopo aver scritto la riga qui sopra.** La prima
stesura di questa sezione concludeva che `DifferentSeedVariation` *«fallirebbe per assenza di premessa»* —
plausibile, e sbagliato. Verificando l'assenza di RNG con `grep FRandomStream Source/`, le uniche due
occorrenze del repository sono **nel commento di un test dedicato**.

`RefactorTactics.Simulation.SeedIsDeclaredAndUnconsumed` (2026-08-15) verifica l'invariante nell'unico
verso che morde — *due seed **diversi** danno lo stesso risultato* — e argomenta perché la formulazione
ovvia sarebbe vacua: *«su un progetto senza RNG, "stesso seed → stesso output" confronta una funzione
deterministica con sé stessa: passa sempre, anche a resolver rotto»*. Dichiara anche cosa fare quando
diventa rosso: **non aggiustarlo** — è il segnale che un RNG è entrato — ma sostituirlo con due test nuovi,
e *«la sostituzione è una decisione»*.

La conclusione non cade, si **rafforza**: `DifferentSeedVariation` non fallirebbe per assenza di premessa,
**contraddirebbe un test verde**. Introdurre il seed non aggiunge un test — ne **rimuove uno** e ne apre
due. E il *come* è già deciso: PDR-05 §5, `Hash(TurnSeed, ActionId, RollKind)`, *«così che aggiungere un
VFX casuale non sposti hit e crit»*.

> È la lezione che questo referto rimprovera alla sorgente, applicata a sé stesso: **si cerca chi produce
> la proprietà, non chi la dichiara.** Il commento del test era l'unico posto dove quella misura viveva, e
> l'ho trovato cercando il meccanismo che credevo assente.

La sorgente lo dice bene in §6 — *«NON usare casualità globale incontrollata»* — ma la conseguenza pratica
è che il seed è **una feature a sé**, non un dettaglio di `a3`. Con quattro eroi deterministici la varietà
fra partite viene già dalla disposizione iniziale e dal layout, che è ciò che §11 chiede di dimostrare.

**Decisione**: il seed **non entra** nella mini-roadmap. Resta registrato come domanda aperta
(`RNG-1`/`RNG-2`, issue [#960](https://github.com/DegrassiAaron/refactor-tactics-main/issues/960)), perché
ciò che è aperto è il **se**, non il come. `DifferentSeedVariation` (§15) è quindi l'unico dei dieci test
di `rc1` **escluso** dal corpus di `E47.5`, dichiarato invece che scritto.

---

## 10. Rischi, dal più alto

| # | Rischio | Perché | Mitigazione |
|---:|---|---|---|
| 1 | Il consolidamento riapre lo scope invece di ridurlo | Sette «release» che assomigliano a una seconda roadmap | Una sola epic, checkpoint dentro le milestone esistenti; nessuna milestone GitHub nuova (§4.3) |
| 2 | Collisione su `integration_only` | `docs/menu-frontend-consolidamento` tocca **10 degli stessi file**, misurato con `git diff --name-only origin/main...` | Merge tempestivo + rigenerazione dei derivati **dopo** l'unione, mai incremento a mano |
| 3 | `a3` scivola nel lavoro di presentazione | «Watchable» attira HUD, VFX, animazioni | Il gate di `a3` è **il vincitore compare senza input**, non «è bello da vedere» |
| 4 | Il seed entra di straforo | §6 lo elenca fra i requisiti base | §9: differito, con la ragione scritta |
| 5 | Secondo modello di copertura | §4/§11 la trattano come categoria di cella | §4.1: tassonomia solo di presentazione, zero `enum` nuovi |
| 6 | **Cinque checkpoint su sei toccano file che nessuna track ACTIVE può scrivere, e tre non hanno owner** | Misurato su `parallel-batch.yaml`, non dedotto dai nomi delle track — la prima stesura ne attribuiva due a `client_tools` ed era falsa | §8: la tabella per checkpoint. Il prossimo batch va aperto **prima** del codice |
| 7 | La modalità non presidiata diventa una seconda pipeline | Tentazione di un «bot runner» dedicato | Vincolo di §5 della sorgente = invariante #10: stesse primitive, nessuna pipeline parallela |
| 8 | Il default di spawn cambia e rompe la partita normale | **Un solo test lo pinna** — `Heroes.SpawnFromData`, righe 151-152; il secondo test del file non tocca `bIsBotControlled` | Configurazione **additiva**, e il test resta verde. ⚠️ Chi estende `SpawnHero` non ha una seconda rete |
| 9 | Scenari `AutoBattle.*` scritti prima della capability | Il free-run non esiste ancora | Dichiarati `planned:` nel registry, non versionati come JSON che uscirebbe `ERROR` |
| 10 | Il referto invecchia dentro la sessione | Già successo quattro volte in questa cartella | Ogni numero qui porta il comando che lo rimisura |

---

## 11. Il primo merge raccomandato (§36-K)

**Questo, e solo questo**: il consolidamento del tracking. Nessuna riga di C++.

La ragione è misurata al §8, ed è più larga di «una track è occupata»: **cinque checkpoint su sei toccano
file che nessuna track `ACTIVE` può scrivere, e tre non hanno owner affatto** — `E47.1` incluso. Per
`D-139` un file non assegnato è uno **STOP**, quindi il prossimo passo non è aspettare che qualcuno
rilasci: è **aprire il prossimo batch** con il write-set dell'autobattle dichiarato.

Il secondo merge è **`E47.1`** — la configurazione «entrambe le squadre al bot» più `PlanningSeconds` da
scenario, su `RTGameMode.{h,cpp}` e `Turn/RTTurnManager.{h,cpp}`. È l'incremento più piccolo che produce
una partita osservabile, e **richiede un'allocazione prima di cominciare**, non un'attesa.

---

## 12. Cosa cambia questa PR

> 🔴 **Questa tabella è stata riscritta dopo la code review, che l'ha trovata falsa in tre punti**:
> diceva che `RT-FEAT-TEST-SCENARIO-HARNESS` e `RT-FEAT-CORE-PLAYBACK` guadagnavano `completed_by`/note —
> guadagnano **solo** una voce `issues:` — e ometteva due feature toccate e un intero file sorgente
> (`execution-graph.yaml`, che riceve sei nodi e sette archi). Un manifesto che non descrive il diff
> certifica una PR che non è quella che si sta mergiando, ed è **la peggiore riga di tutto il referto**
> perché è quella su cui si fa affidamento per non rileggere il diff.

**Sorgenti** — si editano a mano:

| File | Modifica |
|---|---|
| `plans/mini-roadmap-autobattle-spec-panel-2026-08-16.md` | questo referto, nuovo |
| [`../roadmap-v0.1.md`](../roadmap-v0.1.md) | riga **E47** in §3 · sezione **E47** in §5 con sei checkpoint · quattro righe nuove in §2 (i buchi misurati) · totale rimisurato · riga «Tracciata su GitHub» aggiunta anche a **E46**, che ne era priva e faceva fallire un gate |
| [`../roadmap-checkpoint.md`](../roadmap-checkpoint.md) | riga **E47** nella tabella epic→milestone · effetto su **M6** · tre copie del totale epic/CP riallineate |
| [`../feature-registry.yaml`](../feature-registry.yaml) | **due feature nuove** (`RT-FEAT-MATCH-AUTOBATTLE`, `RT-FEAT-UI-BOARD-GRAMMAR`) · **quattro** feature esistenti guadagnano una `issues:` — `CORE-PLAYBACK` (955), `TEST-SCENARIO-HARNESS` (957), `CORE-DETERMINISM` (958), `PROD-PACKAGED` (959) |
| [`../execution-graph.yaml`](../execution-graph.yaml) | **catena D**: sei nodi (`issue:954`–`issue:959`), cinque `requires`, due `follows` · `meta.note` corretta da «tre catene» a quattro |
| [`../editor-sessions.yaml`](../editor-sessions.yaml) | seduta **U23**, la partita registrata |
| [`../../technical/scenario-map.md`](../../technical/scenario-map.md) | i quattro `AutoBattle.*` `planned` · conteggio dei `planned` rimisurato e **dotato di un comando**, che non aveva |
| [`../../decisions/RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md) | **D-145** (execution slice) · **D-146** (grammatica visiva derivata) |
| [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) | `RNG-1`/`RNG-2`, la varietà pseudo-casuale |
| [`../../archive/src/README.md`](../../archive/src/README.md) + il sorgente | archiviazione con banner `HISTORICAL` e riga d'indice; totale rimisurato |
| [`README.md`](README.md) | conteggi della cartella rimisurati, e il comando per la **ripartizione**, che non esisteva |

**Generati** — `python scripts/feature_registry.py generate && … shortlist && … suite`, eseguito
**sull'albero unito** e mai prima: `feature-registry.json`, `project-graph.json`, `roadmap-map.svg`, le
cinque `*.shortlist.md`, il blocco `RT_SUITE_COUNT`.

**GitHub**: epic [#952](https://github.com/DegrassiAaron/refactor-tactics-main/issues/952) con sei
sub-issue [#954](https://github.com/DegrassiAaron/refactor-tactics-main/issues/954)–[#959](https://github.com/DegrassiAaron/refactor-tactics-main/issues/959) ·
domanda [#960](https://github.com/DegrassiAaron/refactor-tactics-main/issues/960) ·
difetto strutturale [#962](https://github.com/DegrassiAaron/refactor-tactics-main/issues/962) ·
commenti su [#16](https://github.com/DegrassiAaron/refactor-tactics-main/issues/16),
[#38](https://github.com/DegrassiAaron/refactor-tactics-main/issues/38) e
[#542](https://github.com/DegrassiAaron/refactor-tactics-main/issues/542).

**Wiki**: nessuna pagina nuova — la mini-roadmap è **esecuzione**, non una meccanica che un giocatore
legge. `RT-FEAT-UI-BOARD-GRAMMAR` referenzia `wiki:mappa-terreni-e-ambiente`, che riceve il blocco
`RT_FEATURE_STATUS` col deploy.

**Non toccato, e dichiarato**: [`../parallel-batch.yaml`](../parallel-batch.yaml) — il lotto è `ACTIVE` e
un settimo write-set scritto da una sessione documentale è il difetto che quel file registra cinque volte ·
`docs/README.md`, il cui totale epic/CP resta indietro perché non è nel `writable` di nessuna track
(**#962**).
