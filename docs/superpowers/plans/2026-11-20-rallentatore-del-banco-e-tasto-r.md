# Rallentatore del banco e tasto R — piano d'implementazione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** nel banco Ability Lab tutto ciò che si vede va a x0,5 insieme, e `R` rifà la scena in qualsiasi momento.

**Architecture:** i battiti restano rallentati come oggi (`ViewerPlaybackSpeed`/`BenchPlaybackSpeed`, D-607 (1)). In più il playback dà alle unità, e all'attore dell'arco, una `CustomTimeDilation` pari alla velocità del banco. All'arco allunga anche la vita, perché il suo timer non segue la dilatazione. `R`, a partita in corso, chiede al GameMode di rifare la scena se è del banco; la decisione è una funzione pura del controller.

**Tech Stack:** C++ Unreal Engine 5.8 (`AActor::CustomTimeDilation`, `AActor::SetLifeSpan`), Automation Test di Unreal, gate di `tools/`.

**Spec:** [`docs/superpowers/specs/2026-11-20-rallentatore-del-banco-e-tasto-r-design.md`](../specs/2026-11-20-rallentatore-del-banco-e-tasto-r-design.md), approvato il 2026-11-20. Issue #4166, decisione D-632.

**Ramo:** `feat/4166-banco-rallentatore-e-r`, worktree `D:\Repositories\refactor-tactics-main\.claude\worktrees\banco-rallentatore`, punta `ddddf9b4b`. Comandi da lì, con percorsi assoluti.

## Global Constraints

- Nessuna chiamata a `SetGlobalTimeDilation(` o `SetPause(` in `Source/` (CP 46.6, #941). Il criterio, da `RTFrontendNavigator.h`: `git grep -n "SetPause(\|SetGlobalTimeDilation(" -- Source/ | grep -vE "^[^:]+:[0-9]+:[[:space:]]*(//|\*|/\*)"` → zero righe.
- È presentazione: niente TurnLog, niente hash, niente stato canonico; il resolver non cambia.
- Fuori dal banco (`BenchPlaybackSpeed` = 0) niente cambia: dilatazione 1, vita dell'arco `LifeSeconds`, `R` riavvia solo a `MatchEnded`.
- La velocità del banco resta `URTAbilityLabLibrary::BenchPlaybackSpeed()` = 0,5, scritta da `ARTGameMode::ApplyPlaybackControlCVars` in `ARTTurnManager::BenchPlaybackSpeed`.
- Riga di log di `R` nel banco: `[RT-Test] banco: la scena riparte`, a `Display`.
- Stile dei file toccati: commenti in italiano, marcatori `🔑 ⚠️ ⛔ 🔴`, riferimenti `[D-632]` e `#4166`. Nessun totale volatile in prosa.
- Test nuovi in namespace con nomi distinti (unity build), nessun nome di test prefisso di un altro.
- Unreal è uno per macchina. Prima di build e suite: `Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, CommandLine`. Un Editor su **questo** worktree blocca. Un Editor dell'autore nel clone principale non blocca la build. Una suite che riporta `completati 0` è NOT RUN e si rifà; prima di dichiararla appesa si legge la coda del log.
- Build (PowerShell, `$LASTEXITCODE`): `& "D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development -Project="D:\Repositories\refactor-tactics-main\.claude\worktrees\banco-rallentatore\RefactorTactics.uproject" -WaitMutex -NoHotReloadFromIDE`.
- Suite: `python tools/suite/esegui.py <filtro> --timeout 15 --log <file nello scratchpad>`; la prima nel worktree può avere un avvio a freddo lungo.
- `git add` con percorsi espliciti, mai `git stash`, CRLF conservato (Edit, non `sed -i`). Commit `<type>(4166): …` con `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.

## Review Focus

1. **Una partita normale.** Fuori dal banco ogni unità ha dilatazione 1, e un arco nato fuori dal banco ha vita `LifeSeconds`. Test nel Task 1.
2. **Un valore della velocità fuori scala** (0, negativo, maggiore di 1, NaN) non dà dilatazioni assurde: vale 1. Test nel Task 1.
3. **Il playback che riprende dopo una finestra di reazione** (`BeginPlayback(true)`) non deve azzerare la dilatazione impostata all'avvio: si imposta solo quando il playback ricomincia. Test nel Task 1.
4. **`R` a partita finita nel banco** ricarica come oggi, non due volte. Test nel Task 2.
5. **`R` in un'altra schermata** (input bloccato) resta inerte come oggi: la guardia `IsGameplayInputBlocked` viene prima della scelta. Lo si guarda nel codice in revisione; la scelta pura è nel Task 2.

---

### Task 1: unità e arco rallentano col banco

**Files:**
- Modify: `Source/RefactorTactics/Turn/Playback/RTPlaybackHost.h` (`FRTPlaybackTuning`), `Source/RefactorTactics/Turn/RTTurnManager.cpp:9184-9202` (`GetPlaybackTuning`)
- Modify: `Source/RefactorTactics/Turn/Playback/RTPlaybackLibrary.h/.cpp` (`PresentationTimeDilation`, `SpawnChainArcActor`)
- Modify: `Source/RefactorTactics/Turn/Playback/RTPlaybackDirector.cpp` (`BeginPlayback`, `SpawnChainArc`)
- Test: `Source/RefactorTactics/Tests/Turn/Playback/Reveal/RTChainArcTests.cpp` (accanto ai test dell'arco)

**Interfaces:**
- Produces:
  - `float FRTPlaybackTuning::BenchPlaybackSpeed = 0.f;` riempito da `GetPlaybackTuning` con `ARTTurnManager::BenchPlaybackSpeed`
  - `static float URTPlaybackLibrary::PresentationTimeDilation(float BenchPlaybackSpeed);` dà `BenchPlaybackSpeed` se è in (0, 1], altrimenti 1 (anche per NaN)
  - `static AActor* URTPlaybackLibrary::SpawnChainArcActor(UWorld& World, UClass& ArcClass, const FVector& Start, const FVector& End, float LifeSeconds, float TimeDilation = 1.f);` imposta `CustomTimeDilation = PresentationTimeDilation(TimeDilation)` e vita = (vita positiva di oggi) / quella dilatazione

- [ ] **Step 1: Scrivi i test che falliscono.**
  - `RefactorTactics.Playback.PresentationTimeDilationFollowsTheBench`: `PresentationTimeDilation` di 0,5 vale 0,5, di 1 vale 1, di 0 vale 1, di -1 vale 1, di 2 vale 1, di NaN vale 1.
  - `RefactorTactics.Playback.ChainArcSlowsWithTheBench`, con la sonda `ARTChainArcProbeForTest` in `RTWorldFixtures::MakeWorld()`:
    - `SpawnChainArcActor(…, 1.5f, 0.5f)`: `CustomTimeDilation` = 0,5 e `GetLifeSpan()` ≈ 3,0;
    - senza l'ultimo argomento: 1 e ≈ 1,5;
    - con `LifeSeconds` = 0 e dilatazione 0,5: vita ≈ `FRTChainFxBinding().LifeSeconds / 0.5`.
  - `RefactorTactics.Playback.BenchSlowsEveryUnitOfThePlayback`, nel playback vero sul modello di `ChainArcHopsFollowTheLine` (Gadget e tre Ivrin):
    - con `TM->BenchPlaybackSpeed = 0.5f`, dopo `LockInAndResolve()` e un `Tick`, ogni unità ha `CustomTimeDilation` = 0,5;
    - nello stesso test, in un secondo mondo, con `BenchPlaybackSpeed = 0.f`, ogni unità ha 1;
    - un controllo positivo prima del `LockInAndResolve`: le unità del primo mondo partono da 1.
  - Review Focus 3: nello stesso test, dopo l'avvio, chiama `BeginPlayback(true)` tramite l'API che i test di estensione già usano, se esiste. Verifica che la dilatazione resti 0,5. Se l'API di test non esiste, il caso si copre con una lettura del codice in revisione: dichiaralo nel report.

- [ ] **Step 2: Verifica che falliscano**: la build fallisce sui nomi nuovi.

- [ ] **Step 3: Implementa.**
  - Il campo nella taratura e in `GetPlaybackTuning`.
  - `PresentationTimeDilation` e il parametro nuovo di `SpawnChainArcActor`.
  - In `SpawnChainArc` passa `PresentationTimeDilation(Tuning().BenchPlaybackSpeed)`.
  - In `BeginPlayback`, solo nel ramo che RICOMINCIA (`!bPreserveClock`), dai a ogni `ARTUnit` del mondo del playback `CustomTimeDilation = PresentationTimeDilation(Tuning().BenchPlaybackSpeed)`. Usa `UGameplayStatics::GetAllActorsOfClass`, come il director già fa altrove, prima che si mostri qualunque battito.
  - Il commento dice perché non `SetGlobalTimeDilation` (#941) e perché la vita dell'arco si divide: il timer di `SetLifeSpan` non segue la dilatazione dell'attore.

- [ ] **Step 4: Verifica che passino.**
  - Build: exit 0, senza warning nuovi.
  - `python tools/suite/esegui.py RefactorTactics.Playback+RefactorTactics.Presentation --timeout 15 --log <scratchpad>\r1.log`: tutti verdi.
  - Mutazione: togli l'assegnazione in `BeginPlayback` e `BenchSlowsEveryUnitOfThePlayback` deve diventare rosso; poi ripristina.

- [ ] **Step 5: Commit** `feat(4166): nel banco unità e archi rallentano con i battiti`.

### Task 2: `R` rifà la scena del banco

**Files:**
- Modify: `Source/RefactorTactics/Player/RTPlayerController.h/.cpp` (`OnRestart`, la scelta pura)
- Modify: `Source/RefactorTactics/RTGameMode.h/.cpp` (accanto a `StepLabScenario`/`FindLabCaster`)
- Test: `Source/RefactorTactics/Tests/Player/RTPlayerInteractionTests.cpp` (scelta pura), `Source/RefactorTactics/Tests/ScenarioHarness/RTScenarioAutoRunTests.cpp` (GameMode, sul modello di `AutoRunBannerShowsTheLabStep` con `FScopedScenarioCVar`)

**Interfaces:**
- Produces:
  - `enum class ERTRestartChoice : uint8 { Nothing, ReloadMatch, RestartLabScene };`, dichiarato in `RTPlayerController.h`, non `UENUM`
  - `static ERTRestartChoice ARTPlayerController::RestartChoiceFor(bool bMatchEnded, bool bLabScene);` dà `ReloadMatch` se la partita è finita (anche nel banco), altrimenti `RestartLabScene` se la scena è del banco, altrimenti `Nothing`
  - `bool ARTGameMode::IsRunningLabScene() const;` vale `URTAbilityLabLibrary::IsLabScenarioId(ResolveScenarioToRun())`
  - `void ARTGameMode::RestartLabScene();` è inerte fuori dal banco; nel banco scrive la riga di log e ricarica il livello corrente come fa `StepLabScenario`, senza riscrivere né la scena né `rt.Test.Scenario`

- [ ] **Step 1: Scrivi i test che falliscono.**
  - `RefactorTactics.PlayerInput.RestartChoiceFollowsMatchAndBench`, con le quattro combinazioni:
    - (finita, banco) → `ReloadMatch` (Review Focus 4);
    - (finita, non banco) → `ReloadMatch`;
    - (in corso, banco) → `RestartLabScene`;
    - (in corso, non banco) → `Nothing`.
  - `RefactorTactics.Scenario.RestartKnowsTheLabScene`, sul modello di `AutoRunBannerShowsTheLabStep`:
    - con `rt.Test.Scenario` = `AbilityLab.<prima voce di kit>`, `IsRunningLabScene()` è vero;
    - con `Movement.Basic` è falso;
    - con valore vuoto è falso.

- [ ] **Step 2: Verifica che falliscano**: la build fallisce sui nomi nuovi.

- [ ] **Step 3: Implementa.**
  - `OnRestart`: la guardia `IsGameplayInputBlocked` resta in testa. Poi la scelta con `RestartChoiceFor`, dalla fase del TurnManager e da `IsRunningLabScene()` del GameMode, trovato con `GetActorOfClass` come in `StepLab`. Poi `ReloadMatch` → `OpenLevel` come oggi, `RestartLabScene` → `GameMode->RestartLabScene()`.
  - Il commento nel controller dice che la regola «solo a partita finita» vale fuori dal banco ([D-632]).

- [ ] **Step 4: Verifica che passino.**
  - Build: exit 0.
  - `python tools/suite/esegui.py RefactorTactics.PlayerInput+RefactorTactics.Scenario+RefactorTactics.AbilityLab --timeout 15 --log <scratchpad>\r2.log`: tutti verdi. `PlayerInput.HotkeysDoNotCollide` e la lista dei binding di `RTInputBindingTests.cpp` restano invariati, perché il tasto non cambia.
  - Mutazione: `RestartChoiceFor` che ignora il banco fa diventare rosso il test della scelta.

- [ ] **Step 5: Commit** `feat(4166): R rifà la scena del banco in qualsiasi momento`.

### Task 3: documenti e gate

**Files:**
- Modify: `docs/decisions/RT_PDR_00_Decision_Log.md`: riga nuova `D-632` fra `D-631` e la fine della tabella, la sua nota di rivendicazione, e una nota datata in `D-607` (1) e in `D-621`
- Modify: `docs/superpowers/specs/2026-10-08-profilo-fx-per-abilita-design.md` §2.10, punto 7 (`R`)
- Modify: `docs/superpowers/specs/2026-11-16-scarica-a-spline-e-bersagli-del-banco-design.md` §5.2 (`R`), con una nota datata e senza riscrivere il testo approvato
- Modify: `docs/technical/test-manuali-pie.md`, voce `PIE-LAB-BERSAGLI`

- [ ] **Step 1: Aggiorna i documenti.**
  - **`D-632`.**
    - Cita alla lettera le parole dell'autore dal §1 del disegno, e lo linka.
    - Decide: nel banco unità e archi hanno `CustomTimeDilation` = velocità del banco, e l'arco ha la vita divisa per la dilatazione; mai `SetGlobalTimeDilation` (#941).
    - Decide anche: `R` rifà la scena del banco in qualsiasi momento; fuori dal banco riavvia solo a partita finita.
    - Correggi `D-621`: «`R` ricarica lo stesso file» era vero solo a partita finita.
    - Prima di scrivere il numero, rimisuralo nei tre posti: righe in grassetto di `origin/main`, ogni ramo remoto, GitHub. Se `D-632` è preso, usa il primo libero.
  - **`D-607` (1) e `D-621`.** In fondo a ciascuna una nota datata (➕ 2026-11-20) che rimanda a `D-632`. Il testo che l'autore ha visto non si riscrive.
  - **§2.10 punto 7 e il §5.2 del disegno del 2026-11-16.** Dove dicono che `R` ricarica la scena, una nota datata: oggi vale grazie a `D-632`.
  - **`PIE-LAB-BERSAGLI`.** Aggiungi due osservazioni: a x0,5 nessuna pausa fra l'animazione del braccio e la scarica che a x1 non ci sia; `R` rifà la scena con lo stesso numero di bersagli e scrive `[RT-Test] banco: la scena riparte`. Lo stato resta ⏳. Se il file chiede il ricalcolo A1, segui la sua procedura.
- [ ] **Step 2: Gate.** Ogni comando esce con 0:
  ```text
  node tools/radar/generate.ts --check
  node tools/radar/doc-links.ts --check
  node tools/radar/catalog-code.ts
  node tools/radar/doc-tables.ts --check
  node tools/radar/doc-coherence.ts --check
  node tools/radar/issue-refs.ts --check
  node tools/radar/scenario-notes.ts --check
  node tools/radar/decision-ids.ts --check
  node tools/asset-refs/check.ts
  node tools/asset-provenance/check.ts
  python tools/architettura/misure-strutturali.py --check
  python tools/architettura/mappa-cartelle.py --verifica
  ```
  Aggiungi il criterio di #941 (vedi Global Constraints): zero righe. Se `misure-strutturali` segnala una crescita, riporta l'uscita esatta e la riga da dichiarare.
- [ ] **Step 3: Commit** `docs(4166): D-632, il rallentatore del banco vale per tutto e R rifà la scena`.

### Task 4: chiusura (il controller)

- [ ] **Step 1:** la suite intera sul commit finale (`python tools/suite/esegui.py RefactorTactics --timeout 30 --log <scratchpad>\suite.log`). Conta i `Result={}` e controlla che trovati = verdi + rossi. Prima, `git fetch` e `git log HEAD..origin/main`: se `main` si è mosso, uniscilo e rimisura.
- [ ] **Step 2:** la revisione dell'intero ramo con un agente nuovo, a cui dai il diff, non le conclusioni.
- [ ] **Step 3:** pubblica il ramo e apri la PR verso `main`. Il corpo porta:
  - `Closes #4166`;
  - la tabella delle verifiche;
  - PIE ⏳ (`PIE-LAB-BERSAGLI`);
  - Packaged NOT RUN.

  Correggi in #4164 la frase su `R` con `gh issue edit` e `GH_TOKEN`. Il merge si fa solo su richiesta dell'autore.
