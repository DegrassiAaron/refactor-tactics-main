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
descrive come se il repository fosse vuoto: **quattro delle sue sette release intermedie sono già in
`main`**, il bot che propone come «MVP da costruire dopo» è `RELEASE_READY` da otto giorni, e la partita
2v2 bot-contro-bot che vuole vedere **gira già headless e si decide al turno 10**
(`RefactorTactics.HexMatch.PlaysToCompletion`).

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

`ERTHexSurface` ha **nove** valori, non cinque, e la copertura è **direzionale per bordo** dal CP 9.1
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

## 8. Mappatura sui tre processi di §18 → track esistenti

Nessuna track nuova. La colonna di destra è il **write-set che la track già dichiara** in
`parallel-batch.yaml`, non una riassegnazione.

| Processo §18 | Track esistente | Perimetro per la mini-roadmap |
|---|---|---|
| **A — Spatial / Board** | `spatial` | `Source/RefactorTactics/ScenarioHarness/` — free-run e configurazione dello scenario |
| **B — Simulation / Bot** | `simulation` | `Source/RefactorTactics/Turn/`, `Bot/` — modalità non presidiata, `PlanningSeconds`, seed |
| **C — Client / Tooling** | `client_tools` | `Source/RefactorTactics/UI/`, `Camera/` — HUD watchable, velocità di playback, schermata vincitore |
| *(quarto, umano)* | `playtest` | `docs/technical/test-manuali-pie.md` — registrazione della partita |

⚠️ **`client_tools` è `ACTIVE` su `#78` in questo momento** e ha `Source/RefactorTactics/UI/` nel proprio
`writable`: il lavoro di presentazione della mini-roadmap **non può aprirsi** finché quella track non
rilascia, o va negoziato nel prossimo batch. È dichiarato qui perché sia un vincolo e non una sorpresa.

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

La sorgente lo dice bene in §6 — *«NON usare casualità globale incontrollata»* — ma la conseguenza pratica
è che il seed è **una feature a sé**, non un dettaglio di `a3`. Con quattro eroi deterministici la varietà
fra partite viene già dalla disposizione iniziale e dal layout, che è ciò che §11 chiede di dimostrare.

**Decisione**: il seed **non entra** nella mini-roadmap. Resta registrato come domanda aperta, perché la
scelta «varietà pseudo-casuale sì/no» è di design e non di implementazione, e va presa da chi possiede il
gioco. `DifferentSeedVariation` (§15) è l'unico dei dieci test di rc1 che **oggi non può passare**, e
fallirebbe per assenza di premessa, non per un difetto.

---

## 10. Rischi, dal più alto

| # | Rischio | Perché | Mitigazione |
|---:|---|---|---|
| 1 | Il consolidamento riapre lo scope invece di ridurlo | Sette «release» che assomigliano a una seconda roadmap | Una sola epic, checkpoint dentro le milestone esistenti; nessuna milestone GitHub nuova (§4.3) |
| 2 | Collisione su `integration_only` | `docs/menu-frontend-consolidamento` tocca **10 degli stessi file**, misurato con `git diff --name-only origin/main...` | Merge tempestivo + rigenerazione dei derivati **dopo** l'unione, mai incremento a mano |
| 3 | `a3` scivola nel lavoro di presentazione | «Watchable» attira HUD, VFX, animazioni | Il gate di `a3` è **il vincitore compare senza input**, non «è bello da vedere» |
| 4 | Il seed entra di straforo | §6 lo elenca fra i requisiti base | §9: differito, con la ragione scritta |
| 5 | Secondo modello di copertura | §4/§11 la trattano come categoria di cella | §4.1: tassonomia solo di presentazione, zero `enum` nuovi |
| 6 | `client_tools` è ACTIVE sul write-set di presentazione | Misurato, non previsto | §8: dichiarato come blocco, non scoperto al merge |
| 7 | La modalità non presidiata diventa una seconda pipeline | Tentazione di un «bot runner» dedicato | Vincolo di §5 della sorgente = invariante #10: stesse primitive, nessuna pipeline parallela |
| 8 | Il default di spawn cambia e rompe la partita normale | `RTHeroSpawnTests` lo pinna | Configurazione additiva; i due test restano verdi |
| 9 | Scenari `AutoBattle.*` scritti prima della capability | Il free-run non esiste ancora | Dichiarati `planned:` nel registry, non versionati come JSON che uscirebbe `ERROR` |
| 10 | Il referto invecchia dentro la sessione | Già successo quattro volte in questa cartella | Ogni numero qui porta il comando che lo rimisura |

---

## 11. Il primo merge raccomandato (§36-K)

**Questo, e solo questo**: il consolidamento del tracking. Nessuna riga di C++.

La ragione è misurata al §8: due delle sei track sono `ACTIVE` e una delle due possiede il write-set della
presentazione. Aprire codice adesso significa contendere un file che un'altra sessione sta scrivendo — che
è il difetto che `parallel-batch.yaml` ha registrato cinque volte in due giorni.

Il secondo merge, quando `client_tools` rilascia, è **`E47.1`**: la configurazione «entrambe le squadre al
bot» più `PlanningSeconds` da scenario. È l'incremento più piccolo che produce una partita osservabile.

---

## 12. Cosa cambia questa PR

| Vista | Modifica |
|---|---|
| Questo referto | nuovo |
| [`../roadmap-v0.1.md`](../roadmap-v0.1.md) | epic **E47** nell'elenco e nella §2.1 |
| [`../roadmap-checkpoint.md`](../roadmap-checkpoint.md) | riga sull'effetto di E47 su **M6** (il playtest diventa osservazione) |
| [`../feature-registry.yaml`](../feature-registry.yaml) | **due** feature nuove: `RT-FEAT-MATCH-AUTOBATTLE`, `RT-FEAT-UI-BOARD-GRAMMAR`; `RT-FEAT-TEST-SCENARIO-HARNESS` e `RT-FEAT-CORE-PLAYBACK` guadagnano `completed_by`/note |
| [`../../technical/scenario-map.md`](../../technical/scenario-map.md) | i quattro `AutoBattle.*` come `planned` |
| [`../editor-sessions.yaml`](../editor-sessions.yaml) | una seduta per la registrazione della partita non presidiata |
| [`../../decisions/RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md) | due `D-nnn`: la tassonomia solo di presentazione; il seed differito |
| [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) | la domanda aperta sulla varietà pseudo-casuale |
| GitHub | epic **E47** + checkpoint, collegati alle milestone esistenti |
| [`../../archive/src/`](../../archive/src/README.md) | il sorgente archiviato, con riga d'indice |
| Wiki | nessuna pagina nuova: la mini-roadmap è **esecuzione**, non una meccanica che un giocatore legge |

⚠️ **I derivati** (`feature-registry.json`, `project-graph.json`, le cinque shortlist, `roadmap-map.svg`) si
rigenerano con `python scripts/feature_registry.py generate && … shortlist`, **dopo** l'unione con
`origin/main`, mai prima: è il rimedio che questa cartella ha imparato nove volte.
