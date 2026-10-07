# Vedere un'abilità in azione — il banco Ability Lab → PIE

> **Statuto**: design **accettato in sessione** il 2026-10-07 e **implementato lo stesso giorno** (vedi il
> blocco ✅ qui sotto). Issue [#3532](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3532).
> È il **primo di quattro sotto-progetti** di una stessa richiesta d'autore, elencati in §0; gli altri tre
> avranno ciascuno la propria spec quando arriverà il loro turno.
>
> **Revisione indipendente** del 2026-10-07 (agente revisore, letta su `3b50eaf7f`): dieci affermazioni
> verificate sul codice, dieci findings accolti. I cambiamenti che ne derivano sono marcati `➕ rev.` nel
> testo; la via `GlobalMapOverride` di §2 è emersa nello stesso giro leggendo gli header dell'Engine.
>
> ✅ **Implementato il 2026-10-07** sul branch `issue/3532-banco-ability-lab-pie`, PR
> [#3535](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3535). 🔴 **`IMPLEMENTATION DRIFT`
> accolto nella stessa PR, marcato `➕ impl.` nel testo**: la premessa di §2 passo 2 e §6 — *«i gate sul corpus
> passano da `Scan`»* — era vera a metà. Diversi test enumerano il corpus da `ListIds` e `ResolvePath`
> (`DevSandboxLauncher.TagFiltersIntersect`, `Scenario.EveryShippedScenarioRuns`, i selettori «primo scenario a
> due squadre»), quindi un file reale in `Saved/RTLab` li avrebbe resi rossi su una macchina sola. La review
> finale dell'intero branch l'ha trovato; la correzione è in §2 passo 2. Il ramo `CancelPIE` di §2 passo 4 e
> §4 citava Live Coding come causa, ed era falso: corretto. La voce PIE `PIE-LAB-PIE` e la seduta `U67` sono
> nel registro: ➕ fu. **seduta U67 eseguita il 2026-10-07** (criteri 0-3 misurati a schermo, (4) `NOT RUN`),
> PR [#3540](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3540), e **ratificata dall'autore**
> lo stesso giorno («ratifico»).
>
> ➕ fu. **Follow-up chiusi il 2026-10-07** nella PR che chiude #3541–#3544, marcati `➕ fu.` nel testo: [#3541](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3541)
> `Set` esplicito a `max(SetByPrima, Code)` al posto di `ECVF_SetByConsole` (§2 passi 3-4, §4); [#3542](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3542)
> lo stato dell'ultimo lancio vive nel modello e si azzera a fine PIE (§3, criterio **(5)** della voce PIE);
> [#3543](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3543) l'abbreviazione per segmenti
> confronta solo gli Id versionati (§6); [#3544](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3544)
> helper di test condivisi in `Tests/RTScenarioTestSupport.h` (§5.1). Il quinto candidato di #3532 —
> l'invariante dei gate nel codice invece che nei test — resta in §7.
>
> **Stato misurato**: 2026-10-07, `main` = `8286276e1`. Ogni riga `file:riga` qui sotto è stata letta su
> quel commit; chi la rilegge più tardi la **rimisura**. Nessun totale volatile in questo documento: dove
> serve una misura c'è il comando che la produce.

---

## 0. La richiesta, le decisioni prese e la decomposizione

### 0.1 La richiesta

> *«voglio fare/associare le animazioni e i fx alle skill dei personaggi, poterle vedere in azione»*

Due metà: un **legame** fra ogni abilità dei quattro eroi e la sua presentazione, e un **banco** su cui
provarle una per una senza giocare una partita intera.

### 0.2 Le quattro decisioni d'autore (2026-10-07)

| # | Domanda | Decisione | Perché |
|---|---|---|---|
| D1 | Perimetro FX | **Dentro la v0.1**: clip per abilità + *Basic Combat Cues* con primitive semplici. **Niente Niagara.** | [D-124](../../decisions/RT_PDR_00_Decision_Log.md) tiene *«Niagara dedicato a ogni abilità»* fuori dalla v0.1; il plugin non è nel `.uproject`; `Content/` non ha un asset Niagara. La grammatica dei cue è di [#2454](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2454). |
| D2 | Banco | **Ability Lab → PIE**: un pulsante che lancia la fixture dell'abilità nel playback vero. | L'Ability Lab (#2599) costruisce già la fixture per ogni abilità canonica ma la esegue solo in un mondo transitorio senza grafica (`SRTLabPanel.cpp:149-175`). Il percorso scenario → `rt.Test.Scenario` → TurnManager → playback esiste già. |
| D3 | Granularità | **Per abilità**: ogni `(eroe, ActionId)` può avere la propria clip attiva, con il ruolo come ripiego. | Oggi la clip si risolve per `(eroe, ruolo)` e l'`ActionId`, pur presente nell'evento, viene scartato al punto di chiamata (`Turn/RTTurnManager.cpp:8462`). |
| D4 | Abilità senza evento | **Nuovo evento `AbilityActivated`**, uno per intento, in coda all'enum. | Un'abilità che non colpisce, non applica stati e non muove nessuno oggi non produce **nessun** evento risolto, quindi non ha un istante in cui suonare. È la forma di #2505: il produttore c'è, manca il momento. |

### 0.3 La decomposizione, in ordine di consegna

Approccio scelto: **estendere l'asse ANIM CORE** (`Action → PresentationRole → variante`, #2445–#2449)
e tenere il profilo dei cue in una **tabella C++ dichiarativa** accanto a `URTPresentationBindingLibrary`.
È l'asse che [#2453](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2453) nomina in
chiaro: *«se durante l'implementazione il legame giusto passa per l'altro asse, si estende quello e non se
ne crea un terzo»*.

Scartati: un Data Asset di presentazione per abilità — D-278 ha scelto tabella C++ e non Data Asset per
la v0.1, e il panel delle animazioni
([`animazioni-paragon-issue-orchestrator-spec-panel-2026-08-30.md`](../../roadmap/plans/animazioni-paragon-issue-orchestrator-spec-panel-2026-08-30.md))
ha respinto i «profili di presentation» come asset; e tutto in tabella C++, percorsi delle clip inclusi —
ogni cambio di clip ricompilerebbe, cioè il costo che il commandlet `RTBuildAnimBindings` è nato per
togliere.

| Ordine | Sotto-progetto | Scope in una riga | Dipende da |
|---|---|---|---|
| **1** | **Il banco** (questa spec) | Ability Lab → PIE: fixture salvata in `Saved/`, seconda radice dell'indice, avvio PIE dal pannello, ripristino CVar. | nessuno |
| 2 | Il momento | `ERTResolvedEventType::AbilityActivated` emesso una volta per intento in Prep, Dash e Blast; voce D-278 con cue di attivazione; beat nel playback. TurnLog e `StateHash` invariati. | nessuno |
| 3 | La clip per abilità | Chiave `ActionId` opzionale in `FRTAnimBinding`, nel CDO (`PerAction` accanto a `PerRole`) e nel commandlet; `PlayPresentationRole` riceve l'`ActionId`; l'Anim Browser guadagna l'asse «Abilità». | 2 per le abilità senza colpo; #2554 per giudicare le clip |
| 4 | Il profilo FX per abilità | Tabella `ActionId → profilo di cue` con default derivati da `Shape` e override per abilità, consumata dalle Basic Combat Cues. | #2454, aperta e owner della grammatica |

Il banco viene **primo** perché non dipende da niente e rende visibile il *prima*: finché 2, 3 e 4 non
atterrano, un attacco suona la clip di ruolo e le abilità senza colpo non hanno beat. Il banco serve a
vedere entrambe le cose.

### 0.4 Owner e confini

| Owner | Relazione |
|---|---|
| E21 [#286](https://github.com/DegrassiAaron/refactor-tactics-main/issues/286) | epic della presentazione: le issue dei quattro sotto-progetti nascono qui |
| [#2453](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2453) · [#2454](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2454) | grammatica dei cue di combattimento: il sotto-progetto 4 la **consuma**, non la ridefinisce |
| [D-278](../../decisions/RT_PDR_00_Decision_Log.md) · #1801 | contratto evento → presentazione e gate di esaustività: il sotto-progetto 2 **estende** la tabella |
| ANIM CORE #2445–#2449 · [#2554](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2554) | catalogo, binding, browser: il sotto-progetto 3 estende l'asse; l'anteprima rotta è una dipendenza di quel lavoro, non di questo |
| [#1881](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881) | coordinata di playback: si legge, non si ricostruisce |
| [#2599](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2599) | Ability Lab: questa spec ne estende il pannello |

---

## 1. Il problema che questa spec risolve

Per guardare un'abilità oggi servono: aprire `L_DevSandbox`, scrivere a mano uno scenario JSON che la
dichiari, aprire la console, digitare `rt.Test.Scenario <Id>`, premere Play, e poi **ricordarsi di
azzerare la CVar**, che vive quanto il processo e dirotta in silenzio ogni Play successivo.

Tre pezzi di questo giro esistono già e non si parlano:

- `URTAbilityLabLibrary::BuildFixture` (`Ability/RTAbilityLab.h`) costruisce uno scenario valido per
  qualunque abilità canonica, con Id `AbilityLab.<AbilityId>` e tag `ability-lab`
  (`Ability/RTAbilityLab.cpp:251-254`). Fail-closed: un'abilità non canonica non produce una fixture a metà;
- `URTScenarioLoader::SaveToFile` (dichiarata in `ScenarioHarness/RTScenarioLoader.h:171`,
  **implementata in `RTScenarioWriter.cpp`**) valida e scrive uno scenario; `URTScenarioIndex::Scan`
  indicizza ricorsivamente **una sola radice**, `FPaths::ProjectDir()/Scenarios`
  (`RTScenarioIndex.cpp:124-129`, `RTScenarioLoader.cpp:709-712`). Un Id dichiarato da più file è
  segnalato da `BuildFrom` e reso **non lanciabile** da `ResolvePath`, che risponde «ambiguo»;
- `ARTGameMode::ResolveScenarioToRun` (`RTGameMode.cpp`) legge quattro sorgenti — seduta PIE, console,
  riga di comando, property — e lo Scenario Harness esegue lo scenario **nel TurnManager vero, con
  playback visivo** (`ScenarioHarness/RTScenarioSession.cpp`).

Il Lab, però, esegue la fixture su un `UWorld` transitorio e mostra parametri e TurnLog in testo
(`SRTLabPanel.cpp:149-175`): nessuna resa. E l'indice non vede `Saved/`, quindi una fixture salvata lì
non è lanciabile per Id.

---

## 2. Flusso

Nel pannello Ability Lab, accanto a «Esegui», compare **«Esegui in PIE»**. Al clic:

1. **Costruzione e salvataggio.** Il modello costruisce la fixture con la stessa `BuildFixture` di oggi e
   la salva in `Saved/RTLab/Scenarios/<ScenarioId>.json` tramite `URTScenarioLoader::SaveToFile`, che
   valida prima di toccare il disco. Una fixture invalida non produce un file. `Saved/` è in `.gitignore`
   (riga `Saved/`): niente entra in git.
   ➕ rev. **Poi verifica di essere lanciabile**: chiama `URTScenarioIndex::ResolvePath` sull'Id e
   pretende che il percorso risolto sia **quel** file. Se un `AbilityLab.<AbilityId>` esiste anche in
   `Scenarios/`, l'indice risponde «ambiguo» e `PrepareForPie` ritorna `false` con quel motivo, invece di
   lasciare che il GameMode lo scopra a schermo.
2. **Indicizzazione.** ➕ rev. L'indice impara una **seconda radice**, `Saved/RTLab/Scenarios/`, ma
   **non dentro `Scan`**. `Scan` resta a una radice, perché due gate sul corpus la usano come «tutto ciò
   che è versionato» — `ScenarioIndex.ShippedScenariosAreTagged` e `WriterRoundTripsShippedScenarios` — e
   un file stantio in `Saved/` di una macchina li farebbe rossi lì e verdi altrove. Si aggiunge
   `ScanAll`, che legge entrambe le radici, e la usano le **ricerche** — `ResolvePath`, `ListIds`,
   `ListTags` — cioè il GameMode, la console e il Launcher. `rt.Test.Scenario AbilityLab.Hero.Aevik.ArcPulse`
   risolve quindi come qualunque altro Id, e `rt.Test.List` lo elenca (il comando stampa i soli Id; il tag
   `ability-lab` serve ai filtri di `ListIds`).
   ➕ impl. **Ma `ScanAll` non basta a tenere i gate sul corpus fuori da `Saved/`.** Misurato in review
   finale: `DevSandboxLauncher.TagFiltersIntersect` confronta `Scan` con `ListIds`;
   `Scenario.EveryShippedScenarioRuns` e `ShippedScenariosRequireKnownCapabilities` eseguono ogni Id di
   `ListIds`; i test che scelgono «il primo scenario a due squadre» prenderebbero `AbilityLab.*`, che precede
   `AutoBattle.*`. ∴ `LabScenariosRoot()` ha **tre rami in quest'ordine**: l'override di test; sotto
   `GIsAutomationTesting` una radice **vuota** (`ScanRoots` la salta, `PrepareForPie` rifiuta: nessuno può
   crearla né scriverci — ⌫ *la prima stesura usava un percorso transiente «Assente», che la code review ha
   mostrato scrivibile*); altrimenti `Saved/RTLab/Scenarios`. Ogni automation test è cieco a `Saved/RTLab` per
   default; chi la vuole la imposta con l'override, che vince. ➕ impl. **La tendina `ScenarioToRun` di
   `BP_GameMode`** usa `ListVersionedIds`/`ListVersionedTags` (solo `Scan`): un Id del Lab salvato in un
   `.uasset` non risolverebbe su nessun'altra macchina. La console e l'auto-run continuano a vedere il Lab. `GIsAutomationTesting` è vero solo mentre un test esegue
   (`Runtime/Core/Private/Misc/AutomationTest.cpp`), mai in Editor interattivo o in PIE. Lo pinna
   `ScenarioIndex.LabRootIsHiddenFromAutomationByDefault`, validato per mutazione: tolto il ramo, cadono il test
   nuovo **e** i tre gate sopra — cioè il difetto riprodotto. La misura reale: con un file in
   `Saved/RTLab/Scenarios` del worktree, le famiglie `DevSandboxLauncher`, `ScenarioIndex`, `Scenario` e `Lab`
   restano verdi.
3. **Avvio.** ➕ rev. Il pannello, lato Editor, **non apre la mappa nell'Editor**: chiede PIE con
   `GEditor->RequestPlaySession` passando `FRequestPlaySessionParams::GlobalMapOverride =
   "/Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox"`. È il campo che l'Engine documenta come *«Override which
   map is loaded for the Play session»* e che `UGameInstance::InitializeForPlayInEditor` legge come
   `OverrideMapURL`. Il livello aperto nell'Editor resta com'è, sporco o pulito che sia. Prima della
   richiesta il lanciatore **cattura** i valori correnti di `rt.Test.Scenario` e
   `rt.Debug.PlaybackControls` e li imposta sull'Id e su `1`. ➕ fu. (#3541) Non più con
   `ECVF_SetByConsole` ma tramite `FRTLabCVarSnapshot` (`RTLabCVarSnapshot.{h,cpp}`), che fotografa valore
   e `SetBy` e scrive con un **`Set` esplicito alla priorità `P = max(SetByPrima, ECVF_SetByCode)`**, con
   rilettura dopo ogni scrittura: scavalca il valore che l'utente avesse già digitato in console (a
   priorità uguale la scrittura prende) **senza alzare la priorità** oltre quella che la variabile aveva.
   ⌫ *Fino alla code review della PR dei follow-up questa riga diceva `SetWithCurrentPriority`*: su una CVar
   **mai impostata** (storia = solo `Constructor`, lo stato normale in un Editor appena aperto) quella
   chiamata risolve a `SETBY_ERROR` e l'Engine stampa *«This is not allowed»*
   (`Runtime/Core/Private/HAL/ConsoleManager.cpp:783-810, 986-1025`). L'ordine dell'enum, letto in
   `IConsoleManager.h:155-187`: `Constructor` < … < `Commandline` < `Code` < `Temp` < `Console` — quindi
   `-dpcvars=` sta **sotto** `Code`, e nulla sta sopra `Console`. Un `Set` a priorità `SetByCode` fissa sarebbe ignorato con un solo warning
   se l'utente avesse digitato la variabile, e il banco giocherebbe lo scenario sbagliato credendo di aver
   scelto. Se una delle due CVar non si trova, o la scrittura non prende (`ReadOnly`, o una priorità
   superiore), il lanciatore si ferma **prima** di toccare l'altra, e rimette la prima se era già stata
   applicata. Da lì in avanti il
   percorso è quello esistente: il GameMode risolve lo scenario dalla console e lo Scenario Harness lo
   gioca nel TurnManager con il playback.
4. **Ripristino.** ➕ rev. Su `FEditorDelegates::EndPIE` **oppure** `FEditorDelegates::CancelPIE` il
   lanciatore riapplica i valori catturati al passo 3 — non un `""` cieco: ripristina ciò che c'era. ➕ fu.
   (#3541) Con `FRTLabCVarSnapshot::Restore`, che riscrive il valore catturato alla stessa priorità `P`:
   dopo il ripristino il `SetBy` è `P`, cioè quello di prima per `Code` e `Console`.
   **Limite dichiarato**: una CVar che stava **sotto** `Code` (`Constructor` se mai impostata, `Scalability`,
   un `.ini`, `Commandline` da `-dpcvars=`) torna a `Code`, non alla sua priorità — un `Set` a una priorità
   più bassa di quella corrente è rifiutato dall'Engine — e `Code` è il pavimento di qualunque `Set` da
   codice (`IConsoleVariable::Unset(Code)` toglierebbe la voce dalla storia e riporterebbe la priorità di
   prima: `FOLLOW-UP CANDIDATE`, §7); e se
   qualcuno alza la priorità **durante** il PIE (una riga digitata in console con `P = Code`), `Restore`
   non prende, ritorna `false` e il pannello lo dice. ➕ fu. (#3542) La
   callback di fine PIE porta l'esito del ripristino, e il pannello lo dice se una CVar non è tornata.
   `CancelPIE` copre il PIE che **non comincia**: `RequestPlaySession` è
   una richiesta differita e `EndPIE` da sola scatterebbe solo per una sessione partita. Al primo dei due
   che scatta, il lanciatore ripristina e si sgancia da entrambi. Senza questo passo il Play successivo
   rilancerebbe lo scenario del Lab in silenzio.
   ➕ impl. ⌫ *Questa riga diceva «errore di compilazione, Live Coding in corso», ed era falso*: letto in
   `Editor/UnrealEd/Private/PlayLevel.cpp`, una modifica C++ non compilata con Live Coding **non** blocca
   l'avvio del PIE, che parte coi binari vecchi. Il ramo reale di `CancelPIE` è «Blueprint con errori di
   compilazione → alla domanda *avviare comunque?* si risponde No», ramo che emette `EndPIE` e poi
   `CancelPIE`. ⌫ ➕ fu. *Questo paragrafo diceva che il ripristino con `ECVF_SetByConsole` lasciava le due
   CVar a priorità console per il resto del processo, e che un `Set(..., ECVF_SetByCode)` successivo veniva
   ignorato.* Era il follow-up #3541, chiuso: scrivendo a `P = max(SetByPrima, Code)` il pavimento non si
   alza oltre quello che c'era, e il test `Lab.CVarSnapshotRestoresValueAndPriority` asserisce, caso per
   caso (`Constructor`, `Code`, `Console`, `Commandline`, alzata durante), valore e `SetBy` dopo `Restore`,
   e che dopo `Restore` un `Set` a `SetByCode` **prende**. Limite dichiarato: se una `Apply` non prende, la
   CVar che ha fallito non viene ripristinata (resta di norma intatta: le cause sono `ReadOnly` o una
   priorità superiore a `P`).

Il pannello mostra l'Id lanciato e con quale riga di log confermare che è partito il banco giusto. ➕ rev.
La riga la scrive `FRTScenarioCoordinator` (`ScenarioHarness/RTScenarioCoordinator.cpp:43`) e **comincia**
così — segue il numero di turni e la pausa:

```
LogRT: Warning: [RT-Test] AUTO-RUN AbilityLab.<AbilityId> (da: console rt.Test.Scenario): …
```

Le API dell'Engine che i passi 3 e 4 usano esistono in UE 5.8.1, lette negli header il 2026-10-07:
`UEditorEngine::RequestPlaySession` (`Editor/UnrealEd/Classes/Editor/EditorEngine.h:1817`),
`FRequestPlaySessionParams::GlobalMapOverride` (`Editor/UnrealEd/Public/PlayInEditorDataTypes.h`, letto in
`Runtime/Engine/Private/GameInstance.cpp:324`), `FEditorDelegates::EndPIE` e `CancelPIE`
(`Editor/UnrealEd/Public/Editor.h:284,298`), `ECVF_SetByCode < ECVF_SetByConsole` (ordine dell'enum, ancora
valido: è il motivo per cui un `Set` a priorità `Console` scavalca il valore digitato, e uno a `Code` no)
(`Runtime/Core/Public/HAL/IConsoleManager.h:183,187`).

---

## 3. Componenti e file

| File | Cosa cambia |
|---|---|
| `Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.{h,cpp}` | `ScenariosRoot()` resta. Si aggiunge `LabScenariosRoot()` → `FPaths::ProjectSavedDir()/RTLab/Scenarios`. (`SaveToFile` non si tocca: è dichiarata qui e implementata in `RTScenarioWriter.cpp`.) |
| `Source/RefactorTactics/ScenarioHarness/RTScenarioIndex.{h,cpp}` | ➕ rev. `Scan` **invariata** (una radice). Nuova `ScanAll(OutProblems)` che legge entrambe; `ResolvePath`, `ListIds` e `ListTags` passano a `ScanAll`. Una radice assente non è un problema né una voce. Un Id presente in entrambe è un **duplicato**: `BuildFrom` lo segnala e `ResolvePath` lo rifiuta come «ambiguo», come già oggi dentro `Scenarios/`. Il messaggio «non trovato nell'indice (… sotto `<radice>`)» nomina entrambe le radici. |
| `Source/RefactorTactics/ScenarioHarness/RTTestConsole.cpp` | ➕ rev. Due testi d'aiuto che nominano la sola `Scenarios/`: l'help di `rt.Test.List` («versionati in Scenarios/») e il messaggio «nessuno scenario in `<radice>`». |
| `Source/RefactorTacticsEditor/Private/RTLabViewModel.{h,cpp}` | `bool PrepareForPie(FString& OutScenarioId, FString& OutError)`: costruisce, valida, salva, **e verifica che l'Id risolva a quel file**. **Pura e headless**: non sa nulla di PIE né di `GEditor`. |
| `Source/RefactorTacticsEditor/Private/RTLabPieLauncher.{h,cpp}` (nuovo) | La sola parte che tocca `GEditor`: cattura e imposta le CVar (➕ fu. #3541: con `FRTLabCVarSnapshot`, `Set` esplicito a `max(SetByPrima, Code)`), chiede PIE con `GlobalMapOverride`, si aggancia a `EndPIE` e `CancelPIE` per il ripristino, si sgancia da entrambi al primo scatto. ➕ fu. (#3542) `CanLaunch(OutError)` espone le guardie prima di scrivere, e `Launch` accetta un `OnFinished` chiamato dopo il ripristino. Isolata perché nessun automation test la vede. |
| `Source/RefactorTacticsEditor/Private/RTLabCVarSnapshot.{h,cpp}` (➕ fu. #3541) | Fotografia di una CVar: valore e `SetBy` alla cattura; `Apply`/`Restore` con un `Set` esplicito a `max(SetByPrima, Code)` e rilettura. Nessuna dipendenza da `GEditor`: verificabile headless. |
| `Source/RefactorTacticsEditor/Private/SRTLabPanel.{h,cpp}` | Il pulsante «Esegui in PIE» e la riga di stato (Id lanciato, riga di log da cercare, oppure l'errore). ➕ fu. (#3542) Lo stato dell'ultimo lancio vive nel modello (`LaunchedScenarioId`, `WasLaunchFinished`): il pannello chiede `CanLaunch` **prima** di `PrepareForPie`, così non scrive se non può lanciare, e a fine PIE mostra «PIE terminato: le CVar sono tornate com'erano» — oppure «PIE terminato: il ripristino di una CVar NON ha preso, vedi il log» se una `Restore` non ha preso. L'ultimo gesto vince: una Run o una selezione fatta durante il PIE non viene scavalcata dalla fine del PIE (e in quel caso un ripristino fallito resta solo nel log). |
| `Source/RefactorTactics/Tests/RTScenarioIndexTests.cpp` | I test della seconda radice (§5). |
| `Source/RefactorTacticsEditor/Private/Tests/RTLabViewModelTests.cpp` | I test di `PrepareForPie` (§5). |

⛔ **Nessun `.uasset`.** Nessuna modifica al TurnManager, al resolver, al formato scenario o al GameMode.
⛔ **`Scan` non cambia firma né semantica**: è la superficie che i gate sul corpus misurano.

La separazione modello / lanciatore ricalca quella che il pannello ha già: `FRTLabViewModel` è
verificabile headless e `SRTLabPanel` no, e il motivo è scritto in testa a
`RTDevSandboxLauncherTests.cpp` — Slate su un editor vivo non lo vede nessun automation test.

---

## 4. Errori e degrado

| Caso | Comportamento |
|---|---|
| Fixture invalida, abilità non canonica | `PrepareForPie` ritorna `false` con il motivo di `BuildFixture` o di `SaveToFile`; **nulla viene scritto**; il pannello mostra la frase. Stesso fail-closed del Lab di oggi. |
| Id ambiguo (lo stesso `AbilityLab.<AbilityId>` esiste anche in `Scenarios/`) | ➕ rev. `PrepareForPie` ritorna `false` con il motivo di `ResolvePath`. ➕ impl. Il file appena scritto viene **rimosso**: lasciarlo avvelenerebbe quell'Id per la console e il GameMode a ogni clic. Il pulsante non chiede PIE. |
| PIE in corso **o già richiesto** per il tick successivo | ➕ impl. `GEditor->IsPlaySessionInProgress()` copre entrambi: il pulsante rifiuta senza toccare niente. |
| PIE già in corso (`GEditor->PlayWorld != nullptr`) | Il pulsante rifiuta con «PIE in corso». Non chiede una seconda sessione. |
| CVar non trovata (`FindConsoleVariable` → `nullptr`) | ➕ rev. Il lanciatore si ferma **prima** di toccare l'altra. `rt.Debug.PlaybackControls` è compilata `!UE_BUILD_SHIPPING`, quindi in Editor c'è sempre; la guardia resta perché il costo è una riga e l'alternativa è un crash. |
| CVar già impostata a mano in console | ➕ rev. ➕ fu. (#3541) il `Set` a `P = max(SetByPrima, Code)` la scavalca a priorità uguale, senza alzarla; un `Set` a priorità inferiore sarebbe ignorato con un solo warning. Vale anche per il ripristino, che riporta valore **e** `SetBy` di prima. |
| CVar `ReadOnly` o valore che non prende | ➕ fu. (#3541) `Apply` rilegge e rifiuta con il motivo; la prima CVar già applicata viene rimessa; nessun PIE parte. |
| Scenario non risolto dal GameMode | Il log lo dice già con `[RT-Test]`. Il pannello non lo intercetta: il verdetto sul lancio resta di chi guarda, come per ogni scenario. |
| Motore occupato da un'altra sessione | Il pannello non può saperlo. Vale `CLAUDE.md` §10, a carico di chi preme il pulsante. |
| PIE non comincia (Blueprint con errori di compilazione, «No» al prompt) | ➕ rev. `CancelPIE` scatta: il ripristino avviene. ➕ impl. Live Coding **non** è una causa: il PIE parte coi binari vecchi. |
| PIE termina per errore | `EndPIE` scatta comunque: il ripristino avviene. |
| Pannello chiuso durante PIE | I delegate restano agganciati finché uno scatta, poi il lanciatore si sgancia da entrambi. Il ripristino non dipende dalla vita del widget. |
| Una seduta PIE «in corso» (`URTPieSessionSubsystem::IsConducting`) che vincerebbe sulla console | ➕ rev. **Non può accadere**: il subsystem è un `UGameInstanceSubsystem`, nasce e muore con la GameInstance del PIE, quindi all'avvio di un PIE nuovo non è mai «in corso». Nessuna guardia da aggiungere. |

---

## 5. Verifica

### 5.1 Automation, headless

| Test | Asserisce |
|---|---|
| `RefactorTactics.ScenarioIndex.ScanAllSeesTheLabRoot` | Un file scritto nella radice del Lab compare in `ScanAll` con Id, percorso e tag e **non** compare in `Scan`; `ResolvePath` lo trova; rimosso il file, l'Id non risolve più. |
| `RefactorTactics.ScenarioIndex.LabRootAbsentIsNotAnError` | Radice del Lab mancante: `ScanAll` non aggiunge problemi né voci, e le voci di `Scenarios/` sono le stesse di `Scan`, confrontate **per Id** e non contate. |
| `RefactorTactics.Lab.PrepareForPieWritesAResolvableScenario` | Per un'abilità canonica il file esiste nella radice del Lab, `OutScenarioId` è `AbilityLab.<AbilityId>`, `ResolvePath` restituisce **quel** percorso e `LoadFromFile` lo rilegge **uguale** alla fixture in memoria, campo per campo con lo stesso confronto di `RunWithoutHeroUsesAbilityLabFixture`. |
| `RefactorTactics.Lab.PrepareForPieRefusesAndWritesNothing` | Nessuna abilità selezionata → `false`, `OutError` non vuota, **nessun file** nella radice del Lab. |
| `RefactorTactics.ScenarioIndex.LabRootProblemsStayOutOfScan` | ➕ piano. Un file corrotto nella radice del Lab è un problema di `ScanAll` e non di `Scan`, e non nasconde gli altri file del Lab. |
| `RefactorTactics.Lab.PrepareForPieRefusesAnAmbiguousId` | ➕ piano. Un secondo file nella radice del Lab con lo stesso `scenarioId` rende l'Id ambiguo: `PrepareForPie` ritorna `false` e il motivo dice «ambiguo». |
| `RefactorTactics.Lab.PrepareForPieOverwritesAStaleFixture` | ➕ piano. Due chiamate con lo stesso Id e seed diversi lasciano **un** file, con il seed della seconda. |
| `RefactorTactics.ScenarioIndex.LabRootIsHiddenFromAutomationByDefault` | ➕ impl. Senza override e sotto `GIsAutomationTesting`, `LabScenariosRoot()` è **vuota**; `ScanAll` e `Scan` coincidono per Id. Mutazione: tolto il ramo, cade insieme ai gate sul corpus. |
| `RefactorTactics.Lab.PrepareForPieRefusesWithoutARootUnderAutomation` | ➕ impl. Senza override, sotto automation, `PrepareForPie` → `false`, nessun file. |
| `RefactorTactics.ScenarioIndex.AbbreviationIgnoresTheLabRoot` | ➕ fu. (#3543) Con `AbilityLab.Hero.Ivrin.Deflection` nella radice del Lab, l'Id esatto risolve al file del Lab e `Deflection` risolve all'unico versionato che termina con `.Deflection`. Mutazione: candidati di nuovo da `ScanAll` → cade su «ambigua». |
| `RefactorTactics.Lab.CVarSnapshotRestoresValueAndPriority` | ➕ fu. (#3541) Su una CVar di prova: `Apply` scavalca un valore a priorità code **e** uno a priorità console; `Restore` riporta valore e `SetBy`; dopo `Restore` un `Set` a `SetByCode` prende. Mutazione: `Set(..., ECVF_SetByConsole)` in `Apply` → cade «il pavimento non si alza». |
| `RefactorTactics.Lab.LaunchedIdIsClearedBySelectionFilterAndRun` · `LaunchFinishedClearsTheLaunchedId` | ➕ fu. (#3542) L'Id dell'ultimo lancio vive nel modello: lo azzerano selezione accettata, cambio di filtro, run e fine PIE; una selezione rifiutata **non** lo azzera; la fine del PIE **non** scavalca un gesto fatto durante il PIE (asserto (b), mutazione: senza la guardia cade); `LastLaunchRestored()` porta l'esito del ripristino (asserto (c)). |

➕ fu. (#3544) Gli helper di radice di prova, header di scenario e conteggi stanno in
`Source/RefactorTactics/Tests/RTScenarioTestSupport.h` (namespace `RTScenarioTestSupport`), condivisi dai
test dell'indice e del Lab: nessuna copia locale (`grep -n "FString LabRootDiProva" Source/**/*.cpp` → 0).

🔴 **Controllo di mutazione dichiarato**: togliere la radice del Lab da `ScanAll` deve far diventare
rosso `ScanAllSeesTheLabRoot`. Un test che resta verde con la mutazione non prova niente.

➕ rev. **I test non misurano `Saved/RTLab` vero**: la radice del Lab è una funzione sovrascrivibile per
il test (`URTScenarioLoader::LabScenariosRoot()` legge un override impostabile da test e lo azzera a fine
test con `ON_SCOPE_EXIT`), e i file di prova vivono sotto `FPaths::AutomationTransientDir()`, come già
`RTMatchHistoryTests.cpp`. `Saved/RTLab` è condiviso con l'Editor e non è un luogo di prova.
⛔ Nessun test asserisce `Entries.Num()` sul corpus: è un totale che cambia da solo.

### 5.2 Non misurabile headless, e dichiarato

Il pulsante, il caricamento della mappa, l'avvio PIE e il ripristino delle CVar: automation `N/A`.
Si verificano in una **seduta PIE** registrata nel registro del progetto
([`test-manuali-pie.md`](../../technical/test-manuali-pie.md) e
[`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml)), con criterio binario:

0. ➕ rev. **prima** del clic, in console: `rt.Test.Scenario Core.PhaseOrder` (o un altro Id del corpus),
   così la CVar porta già un valore a priorità console;
1. dopo il clic, il log porta `AUTO-RUN AbilityLab.<Id>` con `da: console rt.Test.Scenario` — e **non**
   `Core.PhaseOrder`: è la prova che il lanciatore ha scavalcato il valore digitato (➕ fu. da #3541 con
   `Set` esplicito a `max(SetByPrima, Code)`: i criteri **(1)** e **(3)** della voce `PIE-LAB-PIE` sono da rigiudicare su
   quel codice, insieme al **(5)** di #3542 — li porta
   [#3546](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3546));
2. il playback mostra l'abilità scelta sulle due unità della fixture, su `L_DevSandbox`, **qualunque**
   mappa fosse aperta nell'Editor;
3. terminato PIE, `rt.Test.Scenario` vale di nuovo `Core.PhaseOrder` (si legge con il solo nome in
   console) e un Play senza passare dal Lab avvia **quello**, non il banco.

Il terzo punto è il controllo del ripristino, e dice *«ripristina ciò che c'era»*, non *«azzera»*: senza
di esso il banco funzionerebbe e lascerebbe il processo dirottato.

---

## 6. Limiti dichiarati e non-goal

- La fixture resta quella del Lab: due unità, celle fisse, un turno. Non è uno scenario d'autore e non
  sostituisce i `Scenarios/Visual/`.
- Il banco mostra lo **stato attuale** della presentazione. Finché i sotto-progetti 2, 3 e 4 non
  atterrano, un attacco suona la clip di ruolo e le abilità senza colpo non hanno beat. È voluto.
- Un file in `Saved/RTLab/` sopravvive al processo. Il Lab lo sovrascrive al lancio successivo con lo
  stesso Id; non fa pulizia. ➕ rev. Un file stantio o corrotto lì **non** tocca i gate sul corpus, che
  passano da `Scan`; può solo comparire in `rt.Test.List` o produrre un problema in `ScanAll`.
- Il livello aperto nell'Editor non viene né cambiato né salvato: PIE carica `L_DevSandbox` per
  proprio conto.
- ⌫ ➕ fu. *Questa riga diceva che un file del Lab può rendere ambigua un'abbreviazione (`rt.Test.Scenario
  Deflection` con `AbilityLab.Hero.Ivrin.Deflection` sulla macchina) e che non si cambiava `ResolvePath`.*
  Era il follow-up #3543, chiuso: l'abbreviazione per segmenti confronta **solo gli Id versionati**
  (`Scan`), mentre l'Id esatto e i redirect vedono anche il Lab (`ScanAll`). Uno scenario del Lab si
  nomina quindi con il suo Id completo; il test `ScenarioIndex.AbbreviationIgnoresTheLabRoot` lo asserisce
  e cade se il ciclo dei candidati torna su `ScanAll`.
- ➕ impl. La tendina `ScenarioToRun` di `BP_GameMode` e il browser del Tactical Designer elencano anche gli
  Id `AbilityLab.*` della macchina: non salvare un asset che li citi.
- `rt.Debug.PlaybackStartPaused` **non** viene acceso dal banco. Chi vuole il playback fermo lo imposta
  dalla console come oggi (`FOLLOW-UP CANDIDATES`).
- L'Anim Browser (#2554) e il Gray Kit Playground (#1990) non sono toccati.
- Nessun dato di presentazione entra in `MapState`, snapshot, TurnLog o `StateHash`: questa spec non
  tocca nemmeno quel confine.

---

## 7. Follow-up candidates

- Un interruttore «parte fermo» nel pannello che accenda anche `rt.Debug.PlaybackStartPaused`.
- Pulizia di `Saved/RTLab/Scenarios/` da un pulsante o all'avvio dell'Editor.
- Il pannello che legge il `result.json` della run a fine PIE e lo mostra accanto al TurnLog testuale.
- Le spec dei sotto-progetti 2, 3 e 4, nell'ordine di §0.3.
- ➕ fu. L'invariante «i gate sul corpus vedono solo il versionato» nel codice invece che nei test
  (quinto candidato di #3532, non aperto).
- ➕ fu. (#3541) Un test sui rami di fallimento di `FRTLabCVarSnapshot` (`ReadOnly`) e sul rollback di `Launch`.
- ➕ fu. (#3541) `Restore` con `IConsoleVariable::Unset(Code)` quando la CVar stava sotto `Code`: toglierebbe la
  voce dalla storia e riporterebbe valore **e** priorità di prima, chiudendo il limite «torna a `Code`».
- ➕ fu. (#3541) Un helper condiviso per l'idioma «scrivi alla priorità giusta e rileggi», oggi in due copie
  (`RTConsoleVariableGuardForTest.h` con `SetWithCurrentPriority`, `RTLabCVarSnapshot` con `Set` esplicito).
- ➕ fu. (#3544) Sostituire i `MakeDirectory` a mano dei test dell'indice con `ApriRadiceDiProva`.
