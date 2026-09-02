# Spec — Tactical Designer: un solo loop fra mappa, skill e scenario

> `CURRENT` · **Stato**: owner del **concetto** e del suo confine, allineato al codice il **2026-08-29**
> **Autorità**: subordinata a [`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) e al
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md).
> **Nato da**: [referto di consolidamento del 2026-08-17](../../roadmap/plans/tactical-designer-consolidamento-2026-08-17.md),
> che ha verificato l'assenza di un owner: *Tactical Designer*, *Skill Workbench* e *Scenario Composer* non
> avevano **nessuna** occorrenza fuori da `docs/archive/`.

Questo documento risponde a **una** domanda: *quali strumenti d'authoring esistono, che cosa hanno il
diritto di decidere, e che cosa devono invece chiedere al gioco?*

> ⚠️ **Non è un tracker.** Lo stato di implementazione vive nelle **issue** — l'epic
> [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105) e le sue sub-issue — e nei
> checkpoint di [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md) (**M9.4**). Le sedute vivono
> in [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml). Se una riga di questo file dichiara uno
> stato, è un difetto.
>
> 🔵 **Questa riga mandava al `feature-registry.yaml` fino al 2026-08-29, e quel file non esiste dal
> 2026-08-21** ([D-181](../../decisions/RT_PDR_00_Decision_Log.md)). Era una delle **cinque** occorrenze
> superstiti in questo documento — le altre quattro sono al §8, al §9 e nelle due righe del §10, corrette
> nella stessa passata. ⚠️ **Ciò che si è perso va detto invece di essere rimpiazzato**: D-181 dichiara che
> *«non esiste più un punto unico in cui lo stato di una feature si scrive»*. Le issue e i checkpoint sono
> **due** fonti, non una vista: rispondono a *«che lavoro è aperto»* e *«quale checkpoint manca»*, non a
> *«a che punto è la capability X»*.

---

## 1. Cosa questo documento non possiede

| Tema | Owner |
|---|---|
| Grammatica dei segmenti, occupancy a dodici settori, cottura verso i bordi | [`spec-hex-geometry-authoring.md`](../systems/spec-hex-geometry-authoring.md) |
| Coordinate, `FRTCellId`, transizioni fra layer, formato dell'asset mappa | [`spec-mappa-multilivello.md`](../architecture/spec-mappa-multilivello.md) |
| Come si scrive ed esegue uno scenario | [`test-e-diagnosi.md`](../runbooks/test-e-diagnosi.md) |
| Come si identifica e si trova uno scenario | [`scenario-index-e-tag.md`](scenario-index-e-tag.md) |
| Chi verifica cosa — macchina, occhio umano, nessuno | [`scenario-map.md`](scenario-map.md) |
| Il registro delle verifiche interattive | [`test-manuali-pie.md`](../test-manuali-pie.md) |
| Serializzazione del TurnLog e replay canonico | [`spec-turnlog-serialize.md`](../architecture/spec-turnlog-serialize.md) · [ADR-0009](../../decisions/adr-0009-replay-logico-canonico.md) |
| Priorità, milestone e checkpoint | [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md) |

---

## 2. Il nome, e che cosa non implica

**Tactical Designer** è il nome del *workflow*, non di un modulo. Non esiste — e non deve nascere — un
`URTTacticalDesignerSubsystem`. Le classi si chiamano come si chiamano: `URTHexEditorMode`,
`URTHexPaintTool`, `URTHexGeometryTool`, `FRTTestScenario`.

> ⚠️ **Nessun rename è stato fatto e nessuno è previsto.** Un nome di prodotto che diventa un mass rename di
> API stabili produce churn in file ad alto conflitto e non riduce nessuna ambiguità: `URTHexEditorMode`
> dice già che cos'è. Se un giorno una classe nuova avrà bisogno del prefisso, lo prenderà da sola.

Il workflow copre quattro superfici che oggi hanno owner diversi:

```text
Tactical Designer
├── Map / Level authoring      URTHexEditorMode + i cinque tool
├── Character setup            FRTScenarioUnit
├── Skill Workbench            —
└── Scenario Composer          FRTTestScenario + URTScenarioAuthoring
```

> 🔵 **Questo blocco aveva una quarta colonna di stato fino al 2026-08-29, ed è stata rimossa — non
> aggiornata.** Diceva `✅ esiste` · `✅ esiste (dati, non UI)` · `⬜ non esiste` · `🟡 dati sì, authoring no`,
> e l'ultima era **falsa**: l'authoring visuale del Composer è consegnato da
> [#1114](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1114)–[#1117](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1117),
> con la porta su Blueprint decisa da [ADR-0010](../../decisions/adr-0010-esposizione-blueprint-scenario-harness.md).
>
> **Aggiornarla sarebbe stato il difetto, non la correzione.** Il banner in testa a questo documento dice
> *«Se una riga di questo file dichiara uno stato, è un difetto»*: tutte e quattro le celle lo erano, non
> solo quella falsa — le altre tre erano vere e sarebbero marcite allo stesso modo, con la stessa
> impossibilità di accorgersene. Lo stato vive nell'epic
> [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105) e nelle sue sub-issue.
>
> ⚠️ **Resta ciò che è di questo documento**: *quali* superfici esistono e *chi possiede* ciascuna. Un `—`
> nella colonna owner non è uno stato di avanzamento — è la constatazione che una superficie non ha ancora
> un proprietario, che è esattamente la domanda a cui questo file risponde.

---

## 3. L'invariante che tiene in piedi tutto il resto

Uno strumento d'authoring **non è mai un'autorità di gioco**. La catena è unidirezionale:

```text
Dati canonici + regole del gioco
        │
        ├─── resolver                (l'unica autorità sull'esito)
        ├─── Scenario Harness        (esegue il percorso reale)
        ├─── TurnLog / replay        (spiega cosa è successo)
        │
        └─── pure query / DTO
                    │
                    ▼
            visualizzazione d'editor
```

Mai:

```text
l'editor inventa una regola parallela
        │
        ▼
   somiglia al runtime
```

**Se l'editor e il runtime possono divergere, lo strumento ha perso il suo valore** — non è più una lente
sul gioco, è un secondo gioco che nessuno testa.

### 3.1 Il playback è un consumer, non un percorso di simulazione

Vedere una risoluzione accadere è il modo più diretto che un designer ha di capirla, ed è anche il punto in
cui la catena qui sopra si romperebbe più facilmente: animare una partita *somiglia* a giocarla.

La riga è netta e discende da [ADR-0009](../../decisions/adr-0009-replay-logico-canonico.md):

> **Il playback legge una traccia già prodotta. Non la produce, non la ricalcola, non la corregge.**

Ciò che la visualizzazione consuma è il **TurnLog / replay logico canonico** — la stessa traccia che
l'headless produce. Da questo discendono tre conseguenze verificabili:

1. **La posizione di un'unità durante il playback viene dalla traccia**, non da un calcolo. Un
   `SetActorLocation` che *decide* dove qualcuno è arrivato è la forma più comune di secondo simulatore, e
   non smette di esserlo perché il risultato coincide.
2. **La velocità è presentazione.** `0.25x`, `4x` o `Instant` devono produrre lo stesso stato finale: se non
   lo producono, il tempo reale sta entrando in una decisione, che è ciò che il §8 del
   [piano canonico](../../product/piano-canonico-mvp.md) vieta.
3. **Un evento che il playback non sa rendere resta invisibile, non inventato.** Meglio una lacuna
   dichiarata di una ricostruzione plausibile: la seconda è indistinguibile da un dato, e nessuno la
   verifica.

Il trasporto per farlo esiste già e non va riscritto: `URTReplaySeekLibrary` (`SeekToPhase`, `SeekToTurn`,
`SeekToTurnPhase`), l'esito tipizzato `ERTReplaySeekResult`, e `FRTReplayViewModel` — la logica in una
`USTRUCT` non-`BlueprintType` con test propri, che è la stessa forma che il §5.2 descrive per l'authoring.

### 3.2 Due playback, due attori, e cosa si rompe se si fondono

Esistono **due** superfici che riproducono una risoluzione, e la differenza non è tecnica: è di **attore**.

| | Replay Viewer | Scenario Playback |
|---|---|---|
| Attore | il **giocatore** | il **technical designer** |
| Domanda | *«cosa è successo nella mia partita»* | *«perché lo scenario che ho scritto finisce così»* |
| Sorgente | una partita registrata | l'esecuzione dello scenario in authoring |
| Vede | **solo ciò che la sua squadra sa** | **tutto**, per costruzione |
| Presentazione | interpolazione e durate di gioco | graybox, con velocità fino a `Instant` |
| Lavoro | [#472](https://github.com/DegrassiAaron/refactor-tactics-main/issues/472) | [#1625](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1625) |

**Ciò che condividono è il core**, ed è giusto che lo condividano: `FRTReplayViewModel` e il seek. Il
secondo consumer non duplica il primo — è la stessa forma che [ADR-0010](../../decisions/adr-0010-esposizione-blueprint-scenario-harness.md)
dichiara *«la forma»* e che estende allo Scenario Harness.

🔴 **Ciò che non possono condividere è la politica di visibilità, e il costo di confonderle è misurato.**
[#1525](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1525) è un leak vivo dal lato del
giocatore: il playback di partita muove il modello di **ogni** unità lungo il percorso realmente eseguito,
senza filtro di conoscenza, e il giocatore guarda camminare un nemico che non vede. Fondere le due superfici
produce uno di due danni, mai zero:

- se prevale la politica del giocatore, il **designer perde la vista completa** — cioè l'unica ragione per
  cui il suo playback esiste;
- se prevale quella del designer, il **giocatore eredita il leak** che #1525 sta chiudendo.

⚠️ **Da qui la regola pratica**: una modifica utile a entrambi si fa **nel core**, con i test del core. Ciò
che descrive *chi può vedere cosa* non scende mai nel core condiviso — resta nel consumer, dove l'attore è
noto.

Il repository applica già questo vincolo in una forma più forte di una raccomandazione:

> **La logica pura vive nel modulo runtime, e l'editor la chiama.**

Non è teorico: lo snap del gesto d'autore vive in `Map/RTGeometryGrammar` e i suoi due test sono
`RefactorTactics.GeometryGrammar.Snap*`, benché il gesto sia interamente d'editor.

> 🔴 **La ragione che quasi tutti danno per questa regola è FALSA dal 2026-08-16, ed è già costata quattro
> volte.** La formulazione corrente in tre punti del repository è *«in `Source/RefactorTacticsEditor/` non
> esiste alcun test — `find … -iname "*test*"` è vuoto»*. **Misurato il 2026-08-17: restituisce due voci**,
> `Private/Tests/` e `Private/Tests/RTHexToolPropertiesTests.cpp`, con **due** test — arrivati con `#993`.
>
> Il file di test lo dice da sé, e vale la pena citarlo perché nomina il difetto per quello che è:
>
> > *«Tre issue di fila (#871, #921, #931) hanno dichiarato "RefactorTacticsEditor/ non ha test, quindi la
> > verifica è manuale" trattandolo come un dato di fatto. Non lo era: il `Build.cs` ha già `Core` — dove
> > vive `Misc/AutomationTest.h` — e il modulo runtime, e `WITH_DEV_AUTOMATION_TESTS` è definito sui target
> > Editor. I test non erano impossibili: non erano stati scritti.»*
>
> La prima stesura di **questo** documento ha ripetuto la stessa frase, ed è la quarta volta. Il correttivo
> non è ammorbidire la regola: **è cambiarne la giustificazione.** La logica non va nel runtime perché
> l'editor sia intestabile — non lo è più — ma perché *il modulo editor non è dove una regola di gioco
> appartiene*: l'editor **visualizza** una risposta che il gioco dà, e una regola che vive solo lì è una
> seconda risposta alla stessa domanda. È il §3 di questo documento, non un vincolo di tooling.
>
> Quello che i due test dell'editor coprono è **il proprio dominio, non il gioco**: che il pennello Fill
> derivi il costo dalla superficie, e che il readout non si aggiorni da solo. Sono esattamente i test che un
> modulo d'editor deve avere — e nessuno dei due decide un esito di partita.

---

## 4. Chi possiede cosa

| Dato | Owner del dato | L'editor può |
|---|---|---|
| Celle, superfici, costi, layer, transizioni | `URTHexMapAsset` | scrivere tramite `ARTHexMapActor` e le primitive di stroke |
| Geometria d'authoring (segmenti quantizzati) | `FRTGeometrySegment` | scrivere; è **arte** dopo la cottura |
| `FRTHexCover`, `bBlocksMovement` | la **cottura** (`RTGeometryBake`) e il pennello | ⚠️ due produttori — la provenienza li distingue |
| Occupancy, `ERTCellOccupancy` | `URTHexOccupancyLibrary` | **solo leggere** |
| Percorsi, raggiungibilità | `Pathfinding/` | **solo leggere** |
| LOS, copertura applicata | `Perception/`, resolver | **solo leggere** |
| Scenario | `FRTTestScenario` (JSON in `Scenarios/`) | scrivere il file **tramite `URTScenarioLoader::SaveToFile`**, mai interpretarlo per conto suo (§5.1) |
| Esito di un turno | resolver | **niente** |
| TurnLog | `ARTTurnManager` | **solo leggere** |
| Conoscenza di squadra | `URTTeamKnowledgeLibrary` | **solo leggere**, e sceglierne la prospettiva (§4.2) |

⚠️ **La riga con due produttori è l'unica delicata, ed è già risolta.**
[`D-131`](../../decisions/RT_PDR_00_Decision_Log.md) dà a `FRTHexCover` il campo `bGenerated`: il rebake rimuove
e riscrive **solo** le coperture generate e non tocca mai quelle dipinte a mano. Il campo **non entra
nell'hash di stato** — è metadato d'authoring, e due mappe che si giocano identiche non devono divergere.

### 4.1 L'ingresso al workflow: `L_DevSandbox`, e cosa il launcher ha il diritto di decidere

Le quattro superfici del §2 dicono cosa il designer può fare **una volta dentro**. Nessuna riga diceva **come
ci entra**, e l'ingresso è una superficie d'authoring come le altre: ha un owner, ha un confine, e ha una lista
di cose che non gli spettano.

**`L_DevSandbox` è il bootstrap environment del workflow.** Non è una scelta nuova, è una che il progetto ha
già preso e non aveva scritto: [`Config/DefaultEngine.ini`](../../../Config/DefaultEngine.ini) dichiara
`EditorStartupMap=/Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox` col commento *«aprire l'editor sul menu non
serve a nessuno»*. Il livello su cui l'editor si apre **è già** quello del lavoro tecnico.

> ⛔ **Aprire `L_DevSandbox` non avvia nessuno scenario.** L'ingresso presenta il lavoro possibile; non ne
> sceglie uno. Un livello che al caricamento esegue qualcosa non è un ingresso, è un `Play` con più passi — e
> toglie al designer la sola decisione che l'ingresso esiste per fargli prendere.

#### `Start Session` non è `Run`, e la differenza è quale domanda si sta facendo

| | Cosa fa | Cosa cambia | Chi risponde |
|---|---|---|---|
| **`Start Session`** | apre uno scenario nella facade — `OpenById`, oppure `NewScenario` per uno nuovo | *cosa hai aperto davanti* | `URTScenarioAuthoring` |
| **`Run`** | esegue lo scenario aperto attraverso lo Scenario Harness | *cosa succede se lo giochi* | il resolver, per la via reale |

Fra i due resta vero **lo scenario aperto**: `Run` non lo chiude e non lo modifica, e `Reset` lo riporta allo
stato salvato **dicendo** se ha scartato modifiche (`bOutDiscardedEdits`). Sono due domande diverse sullo stesso
oggetto, e confonderle è il modo in cui un ingresso diventa un secondo `Play`.

⚠️ **`Run` non è materia di questa sezione se non per il confine**: passa da `URTScenarioAuthoring::Run` e da
nessun'altra parte, ed è presidiato da `RefactorTactics.Scenario.RunFromTheEditorMatchesTheHeadlessRun`. Una
seconda via d'esecuzione con semantica propria è la definizione del secondo simulatore che il §3 vieta.

#### Qual è la Session — la risposta era già scritta

⚠️ Nel runtime «sessione» è un nome **già occupato due volte**: `FRTScenarioSession` è l'esecuzione di uno
scenario un passo per frame, e `URTScenarioAuthoring` è la facade che possiede il draft aperto. Un terzo
oggetto chiamato allo stesso modo non chiarirebbe niente.

**La Tactical Designer Session è `URTScenarioAuthoring`.** Ha già tutto ciò che una sessione d'authoring deve
avere — `OpenById` · `IsOpen` · `Close` · `Validate` · `Run` · `Reset` · `GetSummary` — e il launcher **non ne
crea una seconda**.

Ciò che il launcher aggiunge è un punto d'accesso, e la sua forma è decisa da
[ADR-0010 §4](../../decisions/adr-0010-esposizione-blueprint-scenario-harness.md), che l'aveva prevista senza
sapere quando servisse:

> *«Se un giorno servirà un punto d'accesso unico in Editor, sarà un `UEditorSubsystem` di poche righe **sopra
> questa stessa facade**, nel modulo Editor, dove un punto d'accesso d'editor appartiene.»*

Il launcher è quel giorno. ∴ **`URTDevSandboxLauncherSubsystem`**, un `UEditorSubsystem` nel modulo Editor, e
il nome dice il livello a cui è legato e il mestiere che fa — non il nome del workflow.

> ⛔ Resta vietato un `URTTacticalDesignerSubsystem` (§2). ⚠️ **E il divieto riguarda l'autorità, non il
> modulo**: la deduzione *«quindi niente nel modulo Editor»* è già stata commessa più volte in questo
> progetto, ed è falsa — il modulo Editor ha `URTHexEditorMode`, i cinque tool e i propri Automation Test.
> Ciò che non può vivere lì è ciò che i test headless devono poter chiamare, ed è precisamente perché la
> facade **non** è un subsystem (ADR-0010 §4).

Il subsystem possiede tre cose, e nessuna riguarda il gioco:

| Possiede | Perché non può stare nella facade |
|---|---|
| **quale** facade è la sessione corrente | la facade non è globale per costruzione — più draft possono essere aperti insieme, ed è una proprietà voluta |
| il ciclo di vita legato all'apertura della mappa | è un fatto d'editor: la facade non sa che esistono i livelli |
| lo stato per-utente dell'ultima selezione | è preferenza locale, non dato canonico — vive in `Saved/`, che [`.gitignore`](../../../.gitignore) ignora già, e non entra in source control per costruzione |

#### Chi apre l'editor per lavorare su altro

`EditorStartupMap` punta a `L_DevSandbox`: **il launcher si presenta all'avvio dell'editor**, anche a chi ha
aperto Unreal per il Frontend. «Non aprirsi sulle mappe di gameplay» non copre il caso, perché DevSandbox *è* la
mappa d'avvio.

⚠️ **Ma «a ogni avvio» sarebbe falso, e la differenza è una via d'uscita che non dobbiamo costruire.** Quale
mappa si apra all'avvio lo decide `LoadLevelAtStartup`, una preferenza **per-utente**
(*Editor Preferences → Loading & Saving → Startup*) che `FEditorFileUtils::LoadDefaultMapAtStartup` legge prima
di `EditorStartupMap`:

| Valore | Cosa carica | Il launcher |
|---|---|---|
| `None` | niente | **non compare**: nessuna mappa si apre |
| `ProjectDefault` — **il default** | `EditorStartupMap`, cioè `L_DevSandbox` | compare |
| `LastOpened` | l'ultimo livello aperto; `EditorStartupMap` solo se vuoto | compare **solo** se l'ultimo era DevSandbox |

∴ chi apre Unreal per il Frontend ha **già oggi** un modo nativo di non vedere l'ingresso, e non è il launcher
a doverglielo dare. Il contract lo registra come risposta esistente invece di scriverne una nuova.

⚠️ **E quella preferenza è solo uno di quattro cancelli.** Prima che la mappa d'avvio si carichi,
`UnrealEdMisc.cpp:417-429` ne attraversa quattro, e il launcher deve **conoscerli senza combatterli**:

| | Cancello | Quando l'ingresso non compare |
|---|---|---|
| ① | `bDoAutomatedMapBuild` | build automatizzata: nessuno guarda, ed è corretto |
| ② | `bMapLoaded` | una mappa passata da riga di comando ha già caricato, e la mappa di default non si carica |
| ③ | `FEditorDelegates::OnEditorLoadDefaultStartupMap` | esiste apposta per **annullare** il caricamento: chiunque può cancellarlo |
| ④ | `LoadLevelAtStartup` | vale `None` — la tabella qui sopra |

⛔ **Nessuno dei quattro va aggirato.** Un ingresso che si apre quando l'engine ha deciso di non caricare la
mappa è la seconda autorità in miniatura: piccola, e della stessa specie di quella che il §3 vieta. Vanno
conosciuti perché il sintomo di ③ — *«il launcher non compare e il gancio sembra rotto»* — punta al posto
sbagliato per giorni.

Il contract è che l'ingresso **si presenta e non pretende**: è un pannello dockabile la cui visibilità la ricorda
il layout dell'editor, non un modale; non ruba il focus; non esegue niente. Chi lo chiude non se lo ritrova
aperto, e il meccanismo è quello nativo di Unreal — nessuno nuovo.

Su una mappa che non è `L_DevSandbox` l'ingresso **non si presenta da sé**. Non è una restrizione di
capability — la facade resta raggiungibile da chi sa dove sta — è che presentarsi su `L_HexArena` o su una
mappa di frontend affermerebbe un legame che non esiste: il bootstrap è di *questo* livello, e un ingresso che
compare ovunque smette di dire dove si entra.

✅ **Misurato il 2026-08-29, e l'esito è quello favorevole**: `FEditorDelegates::OnMapOpened` **scatta** sulla
mappa d'avvio. Non esiste un percorso di startup separato — `LoadDefaultMapAtStartup` chiama la **stessa**
`FEditorFileUtils::LoadMap` di un'apertura manuale, che fa il broadcast come ultima istruzione. E su un avvio
reale l'engine risulta inizializzato **23 ms prima** che il caricamento cominci (`Engine is initialized` →
`pre map load` → `MAP LOAD`), quindi un `UEditorSubsystem` registrato in `Initialize()` **riceve** quel
broadcast: nessun secondo gancio. La misura sta in
[#1680](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1680).

✅ **E porta il meccanismo che a questa sezione mancava.** `FEditorFileUtils::IsLoadingStartupMap()` è pubblico,
e dentro l'handler distingue il caricamento della mappa d'avvio da un'apertura deliberata. La distinzione non
va inventata: la dà l'engine.

⚠️ **Ma il nome promette più di quanto mantenga, e la differenza si riproduce da riga di comando.**
`IsLoadingStartupMap()` non significa *«l'editor sta partendo»* — significa *«sto caricando la mappa di
**default**»*. Chi lancia l'editor con una mappa sull'argomento (`UnrealEditor.exe <progetto> <mappa>`) sta
avviando l'editor a tutti gli effetti, e l'accessor vale `false`: quel percorso carica la mappa **prima**, e
la mappa di default non si carica affatto. Usarlo per dire «siamo all'avvio» sbaglia in un caso reale.

⚠️ **Con un avvertimento da tenere**: quell'accessor **non ha consumatori nell'engine** — cinque occorrenze in
tutto `Engine/Source`, tutte nel file che lo definisce. Chi lo usa è il primo, e non eredita copertura da
nessuno. Ne segue dove può stare: il predicato che decide **se** il launcher si apre non lo tocca ed è
verificabile headless; l'accessor entra solo nel ramo che distingue avvio da apertura deliberata, cioè in
presentazione. Se un giorno sparisse, si perderebbe una sfumatura, non la funzione.

#### Cosa il launcher decide, e cosa deve chiedere

| Il launcher decide | Il launcher chiede, e a chi |
|---|---|
| cosa mostrare, cosa filtrare, quale pannello attivare | *quali scenari esistono* → `URTScenarioIndex` |
| quale sessione aprire | *questo scenario è valido* → `URTScenarioAuthoring::Validate` |
| se ricordare l'ultima selezione | *cosa contiene questo scenario* → `FRTScenarioSummary`, `FRTScenarioUnitView` |
| — | *cosa succede se lo giochi* → Scenario Harness, e nient'altro |

#### Gli assi della selezione, decisi

Mappa e formato **non sono campi di uno scenario** — `FRTTestScenario` porta `Fixture` e `MapRadius`, e
`Format.*` appartiene a `URTMatchFormatData`, che l'harness non nomina mai. Deciso il **2026-08-29** dal
technical designer, su #1681:

| Elemento | Asse o readout | Fonte canonica |
|---|---|---|
| **Tag** (due, in intersezione) | filtro della **lista** | `URTScenarioIndex::ListIds(FilterA, FilterB)` |
| **Ricerca testuale** | filtro della lista | nessuna: è la sola aggiunta, e vive nel pannello Slate di `L1` |
| Terreno (`Fixture` / `MapRadius`) | **readout** del selezionato | `FRTScenarioSummary` |
| Composizione (`2v2`, `1v2`) | **readout** del selezionato | `FRTScenarioUnitView::TeamId`, a scenario aperto |
| Formato `Format.*` | ⛔ **assente dalla UI del launcher** | resta di `URTMatchFormatData` |

⚠️ **Il terreno non può essere un filtro, e la ragione si vede a occhio.** I cinque scenari che portano i tag
`spec` e `map` usano `radius 4`, `TestArena`, `radius 3`, `RelayBasin`, `RelayBasin`: il terreno non raggruppa
niente. È un attributo di uno scenario, non un modo di trovarlo.

**Le due strade scartate, e perché** — scritte perché non vengano riproposte:

- ⛔ **Estendere `FRTScenarioEntry` / `ReadHeader`** con fixture e composizione. È l'unica strada che
  renderebbe *«mostrami i 2v2»* una lista invece di una scoperta a uno a uno, ed è rimasta sul tavolo per
  quello. Scartata **adesso** perché tocca un componente runtime canonico per risolvere un bisogno che
  nessuno ha ancora misurato, e perché `ReadHeader` esiste apposta per non caricare ogni scenario.
- ⛔ **Convenzione sui tag** (`map:arena`, `2v2`). Il vocabolario dei tag **non si dichiara**, per decisione
  (§3 di [`scenario-index-e-tag.md`](scenario-index-e-tag.md)): un namespace convenzionale lo dichiarerebbe
  dalla porta di servizio.

**Cosa il launcher fa che il Details Panel di `ARTGameMode` non fa** — perché un secondo browser non serve a
nessuno: i due filtri per tag e la selezione dello scenario **esistono già** lì, e il launcher non li
reinventa (`ListIds` è la stessa funzione). Aggiunge quattro cose che quel pannello non ha: una **ricerca
testuale**, un **readout** di ciò che lo scenario contiene, un `Start Session` distinto da `Run`, e il non
dover sapere **quale actor selezionare** per trovarlo.

#### Cosa costa ripensarci

Cambiare asse dopo costa **quanto il designer ha imparato**, non quanto codice si riscrive: gli assi sono la
lingua con cui cerca il proprio lavoro, e una lingua ritirata lascia indietro chi l'aveva imparata. Il codice
è poco — la lista è una vista sopra `ListIds`.

🔁 **L'osservazione che riaprirebbe la decisione**, dichiarata adesso perché dopo sarebbe un'opinione: un
designer che, per trovare un allestimento, apre scenari **uno a uno** per leggerne la composizione. È il
sintomo che l'asse mancante è la composizione, e allora la strada scartata qui sopra diventa la strada giusta
— come issue di [`scenario-index-e-tag.md`](scenario-index-e-tag.md), non come slice di launcher.

#### Gli stati di fallimento che l'ingresso deve saper dire

Mai `silent fallback`, `silent mutation`, `implicit conversion` — è la politica che il runtime applica già in
tre punti indipendenti (`MakeFixtureArena` rifiuta un nome sconosciuto invece di dare un'arena vuota,
`ResolvePath` rifiuta un id ambiguo invece di sceglierne uno, `ResolveRules` è fail-closed).

Lo stesso vale a monte dello scenario: un id che l'indice non risolve, un file che non si legge e uno scenario
che `Validate` respinge sono **tre** esiti, non un `errore`. La facade li distingue già con
`ERTScenarioAuthoringResult` — `NotFound`, `Invalid`, `NoScenarioOpen` — e ADR-0010 §3 dice perché quell'enum
non è un `bool`: *«un `bool` costringerebbe la UI a indovinare perché qualcosa non è andato»*. L'ingresso
riporta il codice che ha ricevuto; non ne inventa uno proprio e non ne fonde due.

⚠️ **`MapAsset assente` e `MapAsset presente ma vuoto` restano due condizioni distinte**, e la seconda è quella
che si verifica davvero: `DA_HexMap_Sandbox` — l'asset che la convenzione di cartella associa a `L_DevSandbox` —
**è vuoto**. Il runtime le distingue già, perché `DemoArenaRadius` è documentato come ripiego *«quando il livello
non porta una mappa esagonale con celle (asset assente **oppure presente ma vuoto**)»*. Fonderle in un solo
`No Map Asset` perderebbe l'unico caso reale.

⚠️ **Un livello che non si carica non raggiunge l'ingresso**, e va detto perché somiglia a un caso che invece
lo raggiunge: `LoadMap` ha sei uscite anticipate prima del broadcast, quindi `OnMapOpened` scatta **solo sul
percorso di successo**. «Il livello non si è aperto» non è uno stato che il launcher possa riportare — non
arriva mai — ed è diverso da «la mappa è aperta e vuota», che è il caso reale qui sotto.

⚠️ E l'asset a cui il livello punta è **volatile**: `GenerateFixtureIntoAsset` riscrive
`_Scratch/DA_HexMap_Scratch_Basin` a ogni rigenerazione. Una selezione ricordata può quindi riaprirsi su una
mappa cambiata sotto di lei — è il comportamento normale dello strumento, non un caso limite, e va rilevato e
detto invece che ripristinato in silenzio.

> 🔵 Lo **stato** di questo ingresso vive in [#1678](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1678)
> e nelle sue sub-issue, non qui. Questa sezione dichiara il confine, che è ciò che questo documento possiede.

---

### 4.2 La prospettiva tecnica: il designer sceglie da CHI guardare, non cosa è vero

Il Tactical Designer è omnisciente per costruzione, e resta il suo default. Ma metà delle domande su questo
gioco non sono *«cosa è successo»* — sono ***«cosa sapeva chi ha deciso»***: perché il bot non ha sparato a
un bersaglio in LOS, perché il velo copre quella zona, se lo scenario è leggibile o chiede di indovinare.

Un selettore nella sessione porta `Omniscient` più una posizione per ogni squadra che lo scenario schiera
(#1754). In `Team N` il viewport mostra ciò che quella squadra **conosce**.

🔑 **La conoscenza è quella canonica, e la via è dichiarata invece che dedotta.** In PIE si chiede
all'autorità — `ARTTurnManager::KnowledgeForTeamPublic`. **Fuori da PIE quel manager non esiste**, e la via è
`URTTeamKnowledgeLibrary::Observe`: pura, e *lo stesso produttore* che il TurnManager chiama. Non è una
seconda verità. Gli ingressi li costruisce `RTScenarioKnowledge`, che sta nel **runtime** — decidere
`VisionRange` e `Facing` significa decidere *chi vede cosa*, e quella non è una risposta dell'editor.

⛔ **Il modulo Editor decide quale squadra osservare e come disegnarla. Non decide chi vede cosa.** Nessuna
LOS, percezione o fog-of-war logic in `Source/RefactorTacticsEditor/`.

⚠️ **`Omniscient` è una posizione NOMINATA, non «il filtro spento».** Si esprime come un `FRTTeamKnowledge`
che vede tutto e passa dallo stesso `ApplyKnowledgeVeil` di `Team N`: un ramo che saltasse il velo sarebbe
una seconda strada che nessun test attraversa, e divergerebbe dalla prima al primo cambiamento.

🔴 **Le squadre vengono dal dato.** `1 + N` posizioni, con `N` dai `TeamId` che le unità dichiarano — mai
`{0, 1}` cablato, che è il difetto già registrato su `ARTHUD`. Uno scenario a squadra sola ne ha due, il 4v4
cinque: è *un cambio di dato*, non un caso limite.

⚠️ **Il velo copre la mappa, non i marcatori.** `ApplyKnowledgeVeil` tocca le cinque famiglie di istanze di
`ARTHexMapActor`; le unità stanno su un altro attore. Velare la board e lasciare i marcatori mostrerebbe ogni
nemico mai visto **rispettando alla lettera** il resto — ed è per questo che
`RTScenarioKnowledge::VisibleUnits` esiste, e che la regola che applica è `ClassifyTarget` e non una nuova.

**Cambiare prospettiva non altera nulla**: non il draft, non il simulator state, non lo snapshot, non il
replay, non `ARTPlayerController::PlayerTeamId`. È una lente, e le lenti non scrivono.

⏳ **Durante il playback (#1625) la stessa scelta si applica**, e la decisione di presentazione su *cosa fa
un modello a metà corsa* resta di #1525 — finché non è presa, la scelta conservativa è non animare ciò che
non si conosce.

## 5. Il formato scenario è già la lingua comune

Un authoring visuale degli scenari non ha bisogno di un formato proprio: `FRTTestScenario` esprime già
quasi tutto ciò che serve, e va **esteso**, mai affiancato.

Sono **diciassette** campi, misurati sulla struct e non elencati a memoria:

```text
FRTTestScenario
├── ScenarioId                  ID stabile e gerarchico
├── Version                     versione del FORMATO, non del contenuto
├── Tags[]                      parole per il filtro dell'indice, conservate GREZZE
├── Fixture                     l'allestimento da cui si parte
├── MapRadius                   dimensione dell'arena generata
├── Seed                        dichiarato e NON consumato (vedi sotto)
├── PreviewUnit                 presentazione: headless non fa niente
├── Cells[]                     FRTScenarioCell  — le celle che questo scenario cambia
├── Units[]                     FRTScenarioUnit  — eroe, squadra, cella, facing, HP, scudo, vista, loadout
├── Turns[]                     FRTScenarioTurn
│   ├── Intents[]               FRTScenarioIntent    — chi, dove si muove, quale abilità, su quale bersaglio
│   ├── Decisions[]             FRTScenarioDecision  — risposta scriptata a una finestra: FIRE | HOLD
│   └── Requires[]              capability necessarie → altrimenti ERTTestOutcome::Blocked
├── Expect[]                    FRTTestExpectation + ERTAssertionKind
├── Variants[]                  FRTScenarioVariant — stesso allestimento, celle diverse
├── bExpectSameAcrossVariants   il canary: tutte le varianti devono dare lo stesso esito
├── bFreeRun                    la partita decide quando finire, non il file
├── MaxTurns                    tetto di sicurezza del free-run
├── RepeatCount                 ripetizioni per misurare il determinismo
└── Requires[]                  capability di scenario (solo con free-run)
```

> ⚠️ **Questo elenco diceva «dodici» ed era misurato — nell'agosto 2026.** Quattro campi sono arrivati con
> `#957` (le chiavi del free-run, `version: 4`) e uno con [#1114](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1114)
> (`Tags`), e il conteggio non li aveva seguiti. Vale la pena dirlo invece di correggerlo in silenzio: un
> numero scritto in un documento è una misura **con una data**, e questa sezione ne dichiara una senza
> nominarla. Se conta di nuovo diverso, è la struct ad avere ragione.

Tre proprietà che rendono questo formato adatto a essere il bersaglio di un editor visuale:

1. **Gli ID sono di scenario, non di runtime.** `FRTScenarioDecision::Unit` è l'id di scenario, e la
   traduzione verso l'identità di runtime avviene dove esiste la mappa — non nel JSON. Un authoring layer
   non deve conoscere gli id interni.
2. **Un turno dichiara ciò che gli manca.** `Requires` più `Blocked` permettono di versionare uno scenario
   **prima** che i suoi sistemi esistano, senza una suite rossa cronica.
3. **Le varianti esistono, e il loro limite è scritto.** `FRTScenarioVariant` cambia **solo le celle**, e la
   struct dichiara il prezzo di allargarla: *«una variante che potesse cambiare eroi, squadre o condizione
   iniziale non sarebbe più lo stesso scenario con un ingresso diverso, e il confronto fra le sue tracce non
   direbbe più quale ingresso ha prodotto la differenza»*.

### 5.1 Il formato si legge e si scrive dallo stesso posto

`URTScenarioLoader` sapeva leggere uno scenario e non sapeva scriverlo. Finché è stato così, qualunque
authoring visuale avrebbe dovuto conoscere il JSON da sé — diventando una **seconda autorità sul formato**
accanto al loader, che è la forma che il §3 vieta applicata ai dati invece che alle regole.

`SaveToString` / `SaveToFile` chiudono il verso mancante, e stanno **nello stesso header** del loader
([#1114](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1114)): lettura e scrittura sono due
metà della stessa regola, e separarle in due classi renderebbe possibile aggiungere una chiave da una parte
sola. L'implementazione vive in `RTScenarioWriter.cpp` solo perché il `.cpp` del loader ha già 1769 righe.

Tre garanzie, tutte verificate da `RefactorTactics.Scenario.Writer*`:

1. **`Validate` prima di scrivere.** Uno scenario invalido non viene serializzato a metà, il file già presente
   non viene toccato, e l'errore **nomina il campo** che ha impedito la scrittura.
2. **Forma canonica.** I campi si scrivono in ordine esplicito e i default si omettono: due scritture dello
   stesso scenario producono lo stesso testo. Non si costruisce un `FJsonObject` per poi serializzarlo — le
   sue chiavi vivono in una `TMap`, il cui ordine di iterazione non è quello di inserimento, e un diff di PR
   diventerebbe rumore.
3. **Identità preservate.** `ScenarioId`, i tag dell'indice e gli Stable Unit ID sopravvivono al round-trip.
   L'identità è **dichiarata dal file**, non dedotta dalla cartella: salvare altrove non la cambia.

> 🔴 **`tags` era nel formato ma il loader non lo leggeva**, e per questo `FRTTestScenario` ha oggi un campo
> `Tags`. La chiave esisteva già — la legge `URTScenarioIndex::ReadHeader` per costruire i filtri — ma
> `LoadFromString` la ignorava: due letture dello stesso file che vedevano campi diversi. Finché nessuno
> scriveva scenari la differenza non si vedeva; il primo `load → save` avrebbe cancellato i tag da ogni file
> che li dichiara. **Non è un'estensione del formato**, è il modello che ha smesso di perdere per strada una
> chiave che il formato aveva già.
>
> I tag si conservano **grezzi**, non normalizzati. La forma canonica di un tag appartiene a
> `URTScenarioIndex::NormalizeTag`, e l'indice la applica per conto suo: se la applicasse anche il loader,
> salvare uno scenario riscriverebbe `"Gadget"` in `"gadget"` in tutti i file che lo dichiarano così — una
> modifica che nessuno ha chiesto, prodotta da uno strumento che doveva solo preservare.

### 5.2 Blueprint vede una porta, non il modello

Il formato si legge e si scrive dal C++. L'authoring visuale vive in Blueprint/UMG, e fra i due c'è **una sola
porta**: `URTScenarioAuthoring`, un `UObject` creato da factory che possiede un `FRTScenarioDraft` — il
ViewModel C++ puro dove sta la logica. Decisione registrata in
[ADR-0010](../../decisions/adr-0010-esposizione-blueprint-scenario-harness.md).

Le nove `USTRUCT` del formato **restano non-`BlueprintType`**: Blueprint non le vede, non le costruisce, non le
muta. Ciò che attraversa il confine sono DTO di sola lettura — `FRTScenarioSummary`, `FRTScenarioUnitView` —
che portano `FRTCellId` ed `ERTHexDirection`, cioè il vocabolario del gioco. Un DTO è una fotografia:
modificarlo non modifica niente, ed è la proprietà che rende impossibile all'actor visuale di diventare
authority **per costruzione**, non per disciplina di chi scrive il Blueprint.

> ⚠️ Il costo è che ogni operazione va esposta **una per una**, a ogni slice. È il prezzo che compra
> l'invariante del §3: chi lo trova troppo caro sta chiedendo di pagare l'altro — un editor che diverge dal
> gioco. Il guardiano è `RefactorTactics.Scenario.AuthoringContractIsReachableFromBlueprint`, che verifica
> per riflessione **entrambi i versi**: che il contratto sia raggiungibile da Blueprint, e che il modello non
> lo sia.

**Ciò che il formato non esprime**, e che va aggiunto solo quando ha un consumatore:

| Manca | Serve a | Innesco |
|---|---|---|
| ~~status/condizioni iniziali~~ | ~~fixture con mitigazione o controllo attivo~~ | ✅ **colmato il 2026-09-02** — [#1629](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1629). `FRTScenarioUnit::Statuses` porta la coppia `{ tag, turns }`, perche' `ApplyStatus` chiede entrambi e un formato che scrivesse il solo tag dovrebbe inventare una durata. Il vocabolario resta quello del runtime: un tag fuori da `Core/RTGameplayTags.cpp` **rifiuta** lo scenario col nome sbagliato nel messaggio, invece di applicare un tag vuoto in silenzio |
| stato d'ambiente (acqua, fuoco, ghiaccio) | fixture d'interazione ambientale | M9.2 |
| override di abilità in una variante | *baseline vs variante* | lo Skill Workbench |

> 🔴 **Il seed NON è in questa tabella, e la prima stesura ce l'aveva messo.** `FRTTestScenario::Seed`
> **esiste**, ed è documentato per esteso: *«Seed dichiarato ma non consumato: oggi il progetto non ha alcun
> RNG e il determinismo viene da coordinate intere e ordinamenti totali. Il campo esiste perché il giorno in
> cui un RNG entrerà nel resolver lo scenario debba già saperlo dichiarare — non perché faccia qualcosa
> adesso.»*
>
> ⚠️ E ha un **guardiano**: `RefactorTactics.Simulation.SeedIsDeclaredAndUnconsumed` verifica che due seed
> **diversi** diano lo stesso risultato — l'unico verso che morde su un progetto senza casualità. Chi
> introducesse un RNG non troverebbe un campo mancante: **contraddirebbe un test verde**. Il *se* è aperto
> (`RNG-1`/`RNG-2` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md)), il *come* no.

---

## 6. Scala di maturità del Tactical Designer

⚠️ **`TD 0.x` non è una release di gioco.** `TD 0.7` non ha **niente** a che vedere con
`RefactorTactics v0.7`: è il grado di maturità di uno *strumento*, che non entra nella build, non ha un gate
di release e non compete con la consegna. Le release di gioco stanno in
[`roadmap-post-v0.1.md`](../../roadmap/roadmap-post-v0.1.md) e non sono state toccate.

> 🔴 **Il precedente.** Un sorgente del 2026-08-13 proponeva una milestone *«Skill Balance Lab v0.3»*, e il
> consolidamento di quel giorno la dichiarò superata: `RT-FEAT-TOOL-BALANCE-GROUND` era **già v0.1
> `IMPLEMENTING`**. Una scala di maturità di uno strumento collocata nella roadmap di release si mette in
> concorrenza con il gioco, e perde.

| Stadio | Il designer può | Chi possiede la capability |
|---|---|---|
| **TD 0.1** | aprire una mappa canonica, disegnarla, caricare ed eseguire uno scenario esistente, vedere perché un ordine è invalido | `URTHexEditorMode` e i cinque tool · Scenario Harness · **M9.1** |
| **TD 0.2** | creare e modificare uno scenario **senza scrivere JSON**, e ottenere lo stesso TurnLog di una fixture scritta a mano | Scenario Composer: `FRTTestScenario` + `URTScenarioAuthoring` · **M9.4** |
| **TD 0.3** | configurare una skill *variante* senza toccare il dato di produzione, e provarla sulla mappa con le regole runtime | Skill Workbench — **nessun owner: non esiste** · **M9.4** |
| **TD 0.4** | legare la variante a più scenari e leggere il diff baseline↔variante | TD 0.2 + TD 0.3 |
| **TD 0.5** | spiegare con dati runtime perché un bersaglio è valido, un percorso passa, una copertura si applica | le sonde d'editor sopra `Pathfinding/` e `Perception/` |
| **TD 0.6** | trasformare una sessione registrata in scenario editabile e rieseguibile | l'archivio replay + una conversione che non esiste |
| **TD 0.7** | confrontare due varianti su una suite e ottenere metriche riconducibili a eventi del TurnLog | **E43** — non si duplica qui |
| **TD 0.8** | sapere quali scenari una modifica impatta, e classificarne l'esito | l'indice degli scenari + il corpus golden |
| **TD 0.9** | promuovere una variante a dato di produzione con un gate di validazione, e non per errore | dipende da TD 0.3 |
| **TD 1.0** | fare tutto il giro senza leggere il codice sorgente | — |

> 🔵 **La terza colonna si chiamava «Owner reale» e conteneva sette `RT-FEAT-*`, fino al 2026-08-29.** Sono
> Feature ID del registry uscito con [D-181](../../decisions/RT_PDR_00_Decision_Log.md) il 2026-08-21:
> **nessun file del repository li definisce più**, quindi la colonna che prometteva l'owner di ogni stadio
> non ne risolveva sette su otto.
>
> ⚠️ **Ed erano sopravvissuti a una passata che credeva di averli tolti tutti**: la correzione del
> 2026-08-29 cercava `feature-registry`, `feature_registry` e `Control Center`, e un `RT-FEAT-TOOL-MAP-EDITOR`
> non contiene nessuna delle tre. Un grep trova ciò che nomina; il resto lo trova solo chi legge. È lo stesso
> modo in cui erano sfuggite le due righe del §10 e la vista delle sedute al §9.
>
> Ora la colonna dice **chi possiede la capability** — un modulo, una classe, un'epic — che è una domanda a
> cui questo documento può rispondere senza dipendere da un registro. Il *lavoro* aperto resta dell'epic
> [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105), come dice il §10.

---

### 6.1 La TD Trial è un taglio trasversale, non uno stadio

La scala qui sopra misura **quanto è maturo** lo strumento. Non dice quando diventa **usabile**, e le due
cose non coincidono: uno strumento può avere metà degli stadi aperti ed essere già sufficiente a fare un
giro di lavoro completo, oppure averne otto chiusi e restare inservibile perché ne manca uno in mezzo.

La **TD Trial / Scenario Sandbox** è il nome di quel giro completo:

> Un designer crea o apre uno scenario, imposta lo stato iniziale, dichiara le azioni, **valida**, esegue con
> il runtime reale, **vede** cosa è successo, legge il perché, azzera, modifica e riesegue — senza scrivere
> JSON e senza toccare il C++.

Tre proprietà la qualificano, e sono le stesse tre che la distinguono da un elenco di funzionalità:

1. **È un ciclo, non una somma.** Il valore compare quando il giro si chiude: authoring senza esecuzione è
   un editor di testo, esecuzione senza lettura è un verdetto muto, lettura senza modifica è un rapporto.
2. **Attraversa la scala invece di seguirla.** Tocca `TD 0.1` (residui), `TD 0.2` (consegnato) e `TD 0.5`
   (le sonde), e **non** tocca `TD 0.3`: il Skill Workbench sta fuori, e la Trial resta possibile senza.
3. **Il suo criterio di riuscita è esterno allo strumento.** Non «quante funzioni ha», ma se lo stesso
   scenario, eseguito dalla UI e headless, dà **lo stesso risultato logico** — che è l'invariante del §3
   applicato a un giro intero invece che a una chiamata.

⚠️ **Quello che qui NON si scrive, e non è una dimenticanza**: quali slice la compongono, quali issue le
portano, quanto manca. Il banner in testa a questo documento dice che una riga che dichiara uno stato è un
difetto, e un piano è uno stato con più righe. La Trial come **piano** vive nell'epic
[#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105), sezione *TD Trial / Scenario
Sandbox*, con i suoi gate d'ingresso e d'uscita. Qui sta solo **cosa la rende quella cosa lì**.

### Il DoD della v1.0, ridotto a ciò che è verificabile

1. Nessun rules engine parallelo nell'editor — **misurabile**: nessuna regola di gioco definita in
   `Source/RefactorTacticsEditor/`.
2. Preview ed esecuzione usano gli stessi dati e le stesse regole — **misurabile**: una fixture che
   confronta l'esito della preview con quello del resolver.
3. Una variante non può sovrascrivere il dato di produzione senza un atto esplicito — **misurabile** con un
   test.
4. Scenari con **ID stabile** — **misurabile**: `ScenarioId`, tag e Stable Unit ID sopravvivono al
   round-trip, verificato da `RefactorTactics.Scenario.Writer*` ([#1114](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1114)).
   La **copertura di feature tracciabile** che questa riga chiedeva accanto **non ha più una misura**:
   la produceva `feature_registry.py validate`, uscito con [D-181](../../decisions/RT_PDR_00_Decision_Log.md)
   il 2026-08-21. 🔵 *Fino al 2026-08-29 questa riga prescriveva quel comando come metodo di verifica — non
   un link rotto ma un criterio che fallisce all'esecuzione, e chi lo eseguiva non poteva sapere se il
   difetto fosse suo. Il criterio resta legittimo e oggi si verifica **a mano**, oppure non si verifica: è
   un costo dichiarato di D-181, non un buco di questo documento.*
5. Regressioni distinguibili da cambi di bilanciamento intenzionali — **misurabile**: i tipi di assertion
   sono già distinti, la classificazione deve derivare da quelli e non da string matching.
6. La documentazione descrive il workflow reale — **verificabile solo da una persona**, e per questo è una
   voce di seduta, non un gate automatico.

---

## 7. Distinguere le aspettative, o un nerf sembra un bug

Uno scenario dichiara aspettative di **quattro** nature diverse, e confonderle è il modo in cui una modifica
di bilanciamento diventa un falso allarme strutturale — o, peggio, un difetto vero viene archiviato come
«cambio di balance».

| Natura | Deve sempre passare | Chi la cambia |
|---|---|---|
| **Invariante forte** | sì | nessuno: se cade, è un difetto |
| **Aspettativa di design** | sì, finché il design non cambia | chi cambia il design, nello stesso commit |
| **Soglia di bilanciamento** | può cambiare intenzionalmente | richiede review, e la review è il punto |
| **Osservazione di telemetria** | no — è informazione | nessuno: non è un gate |

⚠️ **Oggi questa distinzione non esiste nei dati**: `ERTAssertionKind` dice *che cosa* si verifica, non *di
che natura* è l'aspettativa. Finché non esiste, la classificazione automatica di TD 0.8 non è costruibile —
e costruirla su string matching del nome dello scenario sarebbe fragile esattamente dove serve robustezza.
È un DoD di TD 0.4, non un lavoro separato.

---

## 8. Guardrail

Il Tactical Designer **non** deve:

- creare un resolver, un targeting system o un calcolo di LOS d'editor;
- mantenere copie indipendenti dei dati eroe/skill;
- scrivere valori derivati che il runtime dovrebbe calcolare;
- registrare coordinate world-space o eventi di widget dove esistono `FRTCellId` e ID stabili;
- mostrare una preview che *sembra* una cella reale quando non lo è;
- introdurre metriche che richiedono un modello statistico che il runtime non alimenta.

E in particolare, sul bilanciamento:

> **Nessun punteggio opaco diventa un gate.** Un `Power = 83.7` senza scomposizione non è una misura: è
> un'opinione con i decimali. Un indicatore euristico può esistere se mostra **quali componenti** lo
> compongono, e non decide niente da solo.

Il vincolo che viene prima di tutti gli altri è
[`D-102`](../../decisions/RT_PDR_00_Decision_Log.md): *un risultato bot-vs-bot non è evidenza di bilanciamento
finché non sappiamo che il bot sa usare la capability misurata*. Un bot che non usa una reazione produce un
numero in cui quella reazione sembra debole — **il numero è vero e la conclusione è falsa**, e niente nel
numero lo segnala. Per questo TD 0.7 segue il competence gate, e non lo precede.

---

## 9. Strati di test

| Strato | Che cosa verifica | Dove vive |
|---|---|---|
| **Puro / dati** | serializzazione, round-trip, ID stabili, precedenza degli override, migrazione | modulo runtime, Automation |
| **Fixture runtime** | targeting, LOS, copertura, percorso, spostamento, reazioni | modulo runtime, Automation |
| **Scenario** | esecuzione deterministica, baseline↔variante, invarianti forti | `Scenarios/`, Scenario Harness |
| **Editor** | binding modello↔toolkit, save/load, selezione, Undo/Redo | dove sostenibile sotto il layer widget |
| **PIE / manuale** | leggibilità, overlay, ergonomia, percezione della latenza | [`test-manuali-pie.md`](../test-manuali-pie.md), dentro una seduta |

Due regole che il repository ha già pagato per imparare:

- **Una verifica PIE che non appartiene a una seduta tende a non essere mai eseguita.** Le sedute vivono in
  [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml), che oggi si **legge direttamente**: la vista
  generata `editormap.shortlist.md` è uscita con [D-181](../../decisions/RT_PDR_00_Decision_Log.md).
  🔵 *Questa riga la dava per esistente fino al 2026-08-29.* ⚠️ **Ed è la stessa riga a essere diventata più
  vera**: D-181 dichiara fra i propri costi che `editor-sessions.yaml` è ora *«dato senza consumatore e senza
  vista — chi aggiunge una seduta scrive in un file che nessuno rende»*. Una verifica PIE che non appartiene
  a una seduta non viene eseguita; una seduta in un file che nessuno rende ha lo stesso problema un livello
  più su.
- **Un test importante deve essere dimostrato capace di diventare rosso.** Si rompe *una* mutazione per
  volta e deve cadere esattamente il test che protegge quella regola — se ne cadono zero, il test non
  verificava; se ne cadono cinque, non si sa quale.

---

## 10. Dove leggere lo stato

| Domanda | Fonte |
|---|---|
| A che punto è una capability | ⚠️ **non c'è più una vista che risponda**: si legge dalle issue e da [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md), che però rispondono a due domande più strette (vedi sotto) |
| Quale seduta d'editor fare, e in che ordine | [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml), letto direttamente |
| Chi verifica cosa fra macchina e persona | [`scenario-map.md`](scenario-map.md) |
| Quali domande di modello sono aperte | [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) |
| Che lavoro è aperto adesso | l'epic [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105) e le sue sub-issue |
| Perché questo documento esiste | [referto 2026-08-17](../../roadmap/plans/tactical-designer-consolidamento-2026-08-17.md) · [D-154](../../decisions/RT_PDR_00_Decision_Log.md) |

> 🔵 **Le prime due righe di questa tabella mandavano a due file inesistenti fino al 2026-08-29** —
> `feature-registry.yaml` ed `editormap.shortlist.md`, usciti entrambi con
> [D-181](../../decisions/RT_PDR_00_Decision_Log.md) il 2026-08-21. Erano **code span, non link**: nessun
> gate poteva vederle, perché `doc-links.ts` cammina i collegamenti e `doc-tables.ts` la larghezza delle
> righe, non il significato delle celle. Trovate leggendo, in code review sulla PR
> [#1620](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1620).
>
> ⚠️ **La prima riga resta la domanda a cui questo repository non sa più rispondere in un posto solo.** Le
> issue dicono *«che lavoro è aperto»*, `roadmap-checkpoint.md` dice *«quale checkpoint manca»*: nessuna
> delle due dice *«a che punto è la capability X»*, che è ciò che il registry derivava. D-181 lo elenca fra
> i propri costi — *«le viste che rispondevano a "a che punto è la consegna" non hanno più dati»* — e questa
> riga lo dichiara invece di indicare un sostituto che non c'è. Un puntatore a un file inesistente si nota;
> un puntatore a una fonte che **non risponde a quella domanda** no.
