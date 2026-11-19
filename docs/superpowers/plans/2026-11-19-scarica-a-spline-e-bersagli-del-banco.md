# Scarica a spline e bersagli del banco — piano d'implementazione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** ogni salto della scarica di `Hero.Gadget.LinearDischarge` diventa un attore con spline, e nel PIE del banco il tasto `U` porta i bersagli da 1 a 2 a 3.

**Architecture:** la riga di `DeclaredChainFxRows` nomina una classe d'attore al posto di un sistema Niagara. `SpawnChainArc` crea l'attore e gli scrive tre punti di spline; il Niagara del Blueprint la percorre da sé. Il banco riceve il numero di bersagli voluto in `FRTAbilityLabFixtureSpec`. `BuildFixture` posa i bersagli in più nelle celle di `URTHexCombatLibrary::HexHitCells` e scrive il numero voluto in un tag della scena. Il GameMode lo rilegge per `U`, `,` e `.`.

**Tech Stack:** C++ Unreal Engine 5.8, `USplineComponent` (modulo `Engine`), Automation Test di Unreal, gate node e python di `tools/`.

**Spec:** [`docs/superpowers/specs/2026-11-16-scarica-a-spline-e-bersagli-del-banco-design.md`](../specs/2026-11-16-scarica-a-spline-e-bersagli-del-banco-design.md), approvato il 2026-11-19.

**Ramo:** `feat/4136-scarica-a-catena`, worktree `D:\Repositories\refactor-tactics-main\.claude\worktrees\4136-scarica`, punta `bf6e5b24f`. Tutti i comandi partono da lì, con percorsi assoluti.

## Global Constraints

- È presentazione: niente TurnLog, niente hash, niente stato canonico. Il resolver non cambia.
- `URTPlaybackLibrary::ChainHopFor` non cambia: la privacy fail-closed resta la sua.
- `SpawnChainArc` resta chiamata solo dai due punti di oggi in `AdvanceBlastSequence`, cioè dentro il prefisso congelato di `D-355`.
- La classe dell'arco è `/Game/RT/Characters/Gadget/FX/BP_Gadget_LinearDischarge_Arc.BP_Gadget_LinearDischarge_Arc_C`, il socket è `canister_main`.
- Gli `.uasset` di `Content/` sono ignorati da git (`.gitignore:48`). Se la classe non c'è, non nasce niente e il log lo scrive solo a `Verbose`.
- Il tasto del banco è `U`, i bersagli vanno da 1 a 3, il default è 1. Il tag della scena è `lab-targets-N`.
- La banda del banco aggiunge `«U» bersagli N`, più ` (qui ne stanno K)` quando K < N.
- Le celle dei bersagli le calcola `URTHexCombatLibrary::HexHitCells`. Il banco non scrive una seconda geometria del colpo.
- Lo stile è quello dei file toccati: commenti in italiano, marcatori `🔑 ⚠️ ⛔ 🔴`, riferimenti `[D-621]` e `#4136`.
- I test nuovi stanno in namespace con nomi distinti, perché la unity build condivide la translation unit. Nessun nome di test è prefisso di un altro.
- Unreal è uno per macchina. Prima di ogni build o suite leggi la `CommandLine`: `Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, CommandLine`. Un Editor sul **proprio** worktree blocca; uno nel clone principale no.
- Build (PowerShell): `& "D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development -Project="D:\Repositories\refactor-tactics-main\.claude\worktrees\4136-scarica\RefactorTactics.uproject" -WaitMutex -NoHotReloadFromIDE`. Leggi `$LASTEXITCODE`, mai il testo filtrato da una pipe.
- Suite: `python tools/suite/esegui.py <filtro> --log <FILE nello scratchpad>`. I filtri si combinano con `+`.
- `git add` con percorsi espliciti. Mai `git stash`. Commit `<type>(4136): …` con `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Nei documenti, niente totali che cambiano da soli.

## Review Focus

1. **Una scena del banco scritta prima di questo lavoro**, senza il tag `lab-targets-N`. Si legge come 1, e `U` la porta a 2. Il test sta nel Task 3.
2. **Un tag illeggibile o fuori scala**, come `lab-targets-7`, `lab-targets-0` o `lab-targets-x`. Si legge come il valore portato fra 1 e 3, o come 1 se non è un numero. Il test sta nel Task 2.
3. **Una linea che non può allungarsi.** Con `MapRadius = 1`, il `TARGET` in (1,0) e 3 bersagli voluti, il `TARGET` resta dov'è, se ne posa uno solo, e la banda dice `(qui ne stanno 1)`. Il test sta nel Task 2.
4. **Una scena con l'alleato**, `Hero.Muiren.CircularTide` a 3 bersagli. Gli altri due vanno nella squadra del `TARGET`, nessuno sulla cella dell'`OPPONENT`, e l'`OPPONENT` resta nell'altra squadra. Il test sta nel Task 2.
5. **`U` fuori dal banco** è inerte: niente ricarica e niente riga d'errore. Il test sta nel Task 3.

---

### Task 1: la scarica su spline

**Files:**
- Modify: `Source/RefactorTactics/Turn/Playback/RTPresentationBinding.h:142-155` (`FRTChainFxBinding`), `RTPresentationBinding.cpp:569-585` (`DeclaredChainFxRows`)
- Modify: `Source/RefactorTactics/Turn/Playback/RTPlaybackLibrary.h/.cpp` (due funzioni nuove)
- Modify: `Source/RefactorTactics/Turn/Playback/RTPlaybackDirector.cpp:16-18` (include), `:914-1002` (`SpawnChainArc`), `RTPlaybackDirector.h:160-168` (commento)
- Modify: `Source/RefactorTactics/RefactorTactics.Build.cs:45-47`, `RefactorTactics.uproject:23-26` (le righe Niagara di questo ramo si tolgono)
- Test: `Source/RefactorTactics/Tests/Turn/Playback/Reveal/RTChainArcTests.cpp`

**Interfaces:**
- Produces:
  - `struct FRTChainFxBinding { FSoftClassPath ArcClass; FName SourceSocket; float LifeSeconds = 1.5f; };` (`System` sparisce)
  - `static FVector URTPlaybackLibrary::ChainArcMidpoint(const FVector& Start, const FVector& End);` alza il centro del tratto di `ChainArcLiftFraction` × lunghezza, con `static constexpr float ChainArcLiftFraction = 0.2f;`
  - `static void URTPlaybackLibrary::SetChainArcPoints(USplineComponent& Spline, const FVector& Start, const FVector& End);` scrive tre punti in spazio mondo: `Start`, `ChainArcMidpoint`, `End`
  - `static AActor* URTPlaybackLibrary::SpawnChainArcActor(UWorld& World, UClass& ArcClass, const FVector& Start, const FVector& End, float LifeSeconds);` crea l'attore in `Start` (`AlwaysSpawn`, collisione spenta), trova la `USplineComponent` con `FindComponentByClass`, scrive i punti e dà la vita. Senza spline distrugge l'attore, scrive a `Verbose` e restituisce `nullptr`.

- [ ] **Step 1: Scrivi i test che falliscono** in `RTChainArcTests.cpp`:
  - In `RefactorTactics.Presentation.ChainFxIsDeclaredOnlyForLinearDischarge`, al posto dell'asserto su `System`:
    ```cpp
    TestEqual(TEXT("la classe e' la copia del Blueprint del pack"), Righe[0].Value.ArcClass.ToString(),
        FString(TEXT("/Game/RT/Characters/Gadget/FX/BP_Gadget_LinearDischarge_Arc.BP_Gadget_LinearDischarge_Arc_C")));
    TestEqual(TEXT("il primo salto parte dalla sfera"), Righe[0].Value.SourceSocket, FName(TEXT("canister_main")));
    TestTrue(TEXT("l'attore ha una vita"), Righe[0].Value.LifeSeconds > 0.f);
    ```
  - `RefactorTactics.Playback.ChainArcMidpointLiftsTheMiddle`:
    - `ChainArcMidpoint((0,0,0), (1000,0,0))` vale `(500,0,200)`;
    - `ChainArcMidpoint((0,0,100), (0,600,100))` vale `(0,300,220)`;
    - con `Start == End` vale `Start`, senza NaN.
  - `RefactorTactics.Playback.ChainArcPointsGoOnTheSpline`: crea `NewObject<USplineComponent>()` e chiama `SetChainArcPoints` con `(0,0,0)` e `(1000,0,0)`. Poi verifica che `GetNumberOfSplinePoints() == 3`, e che `GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World)` valga partenza, punto a metà e arrivo, con tolleranza `KINDA_SMALL_NUMBER`. Chiamata una seconda volta con estremi diversi, i punti restano tre: li sostituisce, non li accoda.
  - `RefactorTactics.Playback.ChainArcActorNeedsASpline`: in `RTWorldFixtures::MakeWorld()`, chiama `SpawnChainArcActor(*World, *AActor::StaticClass(), …)`. Deve restituire `nullptr`, e l'attore creato non deve restare vivo: conta gli `AActor` validi e non in distruzione prima e dopo la chiamata.

- [ ] **Step 2: Verifica che falliscano.** Lancia la build. Deve fallire alla compilazione, perché `ArcClass`, `ChainArcMidpoint`, `SetChainArcPoints` e `SpawnChainArcActor` non esistono.

- [ ] **Step 3: Implementa.**
  - `FRTChainFxBinding` come in *Produces*. Il commento dice il contratto nuovo: l'asset è un attore con una `USplineComponent`, che il suo Niagara percorre; il playback scrive i punti dopo la nascita, e il data interface `Spline` li rilegge al cambio di versione (`PerInstanceTick`, Engine 5.8).
  - La riga di `DeclaredChainFxRows` porta la classe, `canister_main` e `LifeSeconds` di default. Il commento cita la scelta dell'autore («una spline per salto e scegli tu il socket»), e dice perché il socket: lo anima `LMB_Fire_B_Slow_V1`.
  - Le tre funzioni in `RTPlaybackLibrary.cpp`, con `#include "Components/SplineComponent.h"`.
  - In `SpawnChainArc` cambia solo la coda dopo il calcolo di `Inizio` e `Fine`. Carica con `LoadClass<AActor>(nullptr, *Fx->ArcClass.ToString(), nullptr, LOAD_NoWarn | LOAD_Quiet)`. Se non c'è, il log `Verbose` di oggi; altrimenti `SpawnChainArcActor(*World, *Classe, Inizio, Fine, Fx->LifeSeconds)`.
  - Togli i tre `#include "Niagara…"`, `PrivateDependencyModuleNames.Add("Niagara")` col suo commento, e il blocco `Niagara` del `.uproject`. Dopo, `git diff origin/main -- RefactorTactics.uproject` dev'essere vuoto, e `Build.cs` non deve nominare Niagara.

- [ ] **Step 4: Verifica che passino.** Build con `$LASTEXITCODE` 0 e senza warning nuovi. Poi `python tools/suite/esegui.py RefactorTactics.Playback+RefactorTactics.Presentation --log <scratchpad>\t1.log`. Tutti verdi, compreso `RefactorTactics.Playback.ChainArcHopsFollowTheLine`, che misura i salti prima del caricamento della classe.

- [ ] **Step 5: Commit** dei file elencati sopra: `feat(4136): un attore con spline per salto della scarica, al posto del raggio Niagara`.

### Task 2: il banco posa 1, 2 o 3 bersagli

**Files:**
- Modify: `Source/RefactorTactics/Ability/Lab/RTAbilityLab.h:127-167` (spec), dichiarazioni nella libreria
- Modify: `Source/RefactorTactics/Ability/Lab/RTAbilityLab.cpp` (`RTAbilityLabInternal`, `BuildFixture:649-725`)
- Test: `Source/RefactorTactics/Tests/Ability/Lab/RTAbilityLabTests.cpp`

**Interfaces:**
- Produces:
  - `FRTAbilityLabFixtureSpec::TargetCount` (`UPROPERTY(BlueprintReadWrite)`, `int32`, default `1`)
  - `static int32 URTAbilityLabLibrary::MaxLabTargets() { return 3; }`
  - `static int32 URTAbilityLabLibrary::RequestedTargetsOf(const FRTTestScenario& Scene);` legge `lab-targets-N`: senza tag 1, valore portato fra 1 e `MaxLabTargets()`, non numerico 1
  - `static int32 URTAbilityLabLibrary::PlacedTargetsOf(const FRTTestScenario& Scene);` conta le unità `TARGET`, `TARGET2` e `TARGET3`
  - in `BuildFixture`: il tag `lab-targets-N` scritto sempre, con N portato fra 1 e 3; le unità in più si chiamano `TARGET2` e `TARGET3`

- [ ] **Step 1: Scrivi i test che falliscono.** Usano gli id veri, come `EachTargetKindRunsAndActs`, e gli aiuti `RunFixture` e `FieldBeforeAfter` del file.
  - `RefactorTactics.AbilityLab.LineTargetsGrowAlongTheLine`, su `Hero.Gadget.LinearDischarge` con la spec di default e `TargetCount` 1, 2 e 3:
    - N=1: unità `CASTER` e `TARGET`, `TARGET` in (2,0), tag `lab-targets-1`;
    - N=2: `TARGET` in (2,0) e `TARGET2` in (1,0);
    - N=3: `TARGET` in (3,0), `TARGET2` in (1,0) e `TARGET3` in (2,0), tag `lab-targets-3`;
    - per N=3: tutti nella squadra del `TARGET`, `HeroId` diversi fra loro e da quello del `CASTER`, `RequestedTargetsOf == 3` e `PlacedTargetsOf == 3`;
    - la run di N=3 non dà ERROR, gioca un turno, e la vita di `TARGET`, `TARGET2` e `TARGET3` scende (`FieldBeforeAfter`).
  - `RefactorTactics.AbilityLab.AreaTargetsStayInsideTheArea`:
    - `Hero.Gadget.Overload` a 3 bersagli: le celle dei bersagli in più stanno in `HexHitCells(Area, CASTER, cella mirata dall'intento, Def.RangeCells, Def.AreaRadius)`, nella mappa, diverse fra loro e da quella di chi lancia. La run dà meno vita a ognuno dei tre.
    - `Hero.Muiren.CircularTide` a 3 bersagli (Review Focus 4): i bersagli in più sono nella squadra del `CASTER`, nessuno sta sulla cella dell'`OPPONENT`, e l'`OPPONENT` resta in squadra 1.
  - `RefactorTactics.AbilityLab.SingleTargetActionsStayAtOne`:
    - a 3 bersagli, la prima abilità d'eroe di forma `Single` che mira un'unità (presa dal catalogo, come fa `FindHeroAbilityWithShape`) non ha `TARGET2`, e `PlacedTargetsOf` vale 1;
    - `Hero.Ivrin.Deflection` (reazione) e `Hero.Ivrin.PassingBlade` (scatto) a 3 bersagli non hanno `TARGET2`;
    - Review Focus 3: `Hero.Gadget.LinearDischarge` con `MapRadius = 1`, `TargetCell = (1,0)` e `TargetCount = 3` ha il `TARGET` in (1,0) e `PlacedTargetsOf == 1`.
  - `RefactorTactics.AbilityLab.TargetCountTravelsInTheSceneTag`:
    - due `BuildFixture` con la stessa spec (`TargetCount = 3`) danno lo stesso testo da `URTScenarioLoader::SaveToString`;
    - `SaveToString` seguito da `LoadFromString` conserva `RequestedTargetsOf`;
    - Review Focus 2: con i tag `lab-targets-7`, `lab-targets-0`, `lab-targets-x` e nessun tag, `RequestedTargetsOf` vale 3, 1, 1 e 1;
    - `TargetCount = 9` nella spec si scrive come `lab-targets-3`.

- [ ] **Step 2: Verifica che falliscano.** La build fallisce perché `TargetCount`, `RequestedTargetsOf` e `PlacedTargetsOf` non esistono.

- [ ] **Step 3: Implementa** in `RTAbilityLab.cpp`.
  - Le celle in più, in `RTAbilityLabInternal`: `static TArray<FRTCellId> ExtraTargetCells(const FRTActionDef& Def, ERTAbilityShape Shape, const FRTAbilityLabFixtureSpec& Spec, const FRTAbilityLabTargetChoice& Choice, int32 Wanted, const TArray<FRTCellId>& Occupied, FRTCellId& InOutTargetCell);`. Il corpo:
    ```text
    se Wanted <= 1 → nessuna
    mira = InOutTargetCell per Enemy/Ally, Choice.Cell per Cell; altri tipi → nessuna
    se Shape == Line e il tipo è Enemy/Ally:
        finché HexHitCells(Line, Caster, InOutTargetCell, …) ha meno di Wanted celle:
            prossima = Neighbor(InOutTargetCell, DirectionTowards(Caster, TargetCell))
            se prossima è fuori mappa (InLabMap) o fuori portata (WithinRange) → fermati
            InOutTargetCell = prossima; mira = prossima
    celle = HexHitCells(Shape, Caster, mira, Def.RangeCells, Def.AreaRadius)
            meno Occupied, meno InOutTargetCell, meno le celle fuori mappa
    ordina per HexDistance(Caster, c), a pari distanza per URTHexLibrary::StableLess
    tieni le prime Wanted-1
    ```
  - In `BuildFixture`:
    - porta `Wanted` fra 1 e `MaxLabTargets()`;
    - chiama `ExtraTargetCells` solo se la scena non è uno scatto, non è una reazione o una provocazione (`Provocazione == None`) e l'azione non è su se stessi;
    - lo fa prima di posare `Target.Cell`, che prende la cella spostata;
    - `Occupied` sono la cella di chi lancia e, nella scena con l'alleato, `OpponentCellFor(Spec)`, dove l'`OPPONENT` si posa comunque;
    - le unità in più entrano in `Scenario.Units` dopo l'`OPPONENT`, così con un solo bersaglio l'ordine delle unità resta quello di oggi (`EachTargetKindRunsAndActs` legge `Units[1]` e `Units[2]`).
  - Gli eroi dei bersagli in più vengono dal roster (`GetHeroIds`), in ordine, saltando quelli già in scena. Squadra e vita sono quelle del `TARGET`, cioè metà vita se l'azione cura. Se gli eroi o le celle finiscono, ne posi quanti ce ne stanno.
  - Aggiungi `Scenario.Tags.Add(FString::Printf(TEXT("lab-targets-%d"), Wanted))` accanto a `ability-lab`.
  - Scrivi `RequestedTargetsOf` e `PlacedTargetsOf`. Il prefisso del tag e gli id dei bersagli stanno in una sola costante ciascuno, in `RTAbilityLabInternal`.
  - Il commento della spec dice che il numero è una posa dello strumento, non una regola di gioco, e che chi colpisce lo decide comunque il resolver.

- [ ] **Step 4: Verifica che passino.** Build, poi `python tools/suite/esegui.py RefactorTactics.AbilityLab+RefactorTactics.HeroLab+RefactorTactics.Scenario --log <scratchpad>\t2.log`. Tutti verdi, compresi i test di oggi: `EveryKitAbilityRunsWithoutError` e `EachTargetKindRunsAndActs` restano a un bersaglio di default.

- [ ] **Step 5: Commit** `feat(4136): il banco posa fino a tre bersagli nelle celle che l'abilità colpisce`.

### Task 3: il tasto `U` e la riga della banda

**Files:**
- Modify: `Source/RefactorTactics/Ability/Lab/RTAbilityLab.h/.cpp` (`DescribeLabStep:964-979`, `WriteLabScenario:988-1031`)
- Modify: `Source/RefactorTactics/RTGameMode.h:420-440`, `RTGameMode.cpp:1859-1901` (`StepLabScenario`)
- Modify: `Source/RefactorTactics/Player/RTPlayerController.h:392-402, 829-833`, `RTPlayerController.cpp:206-211, 356-361, 536-537, 3807-3836`
- Test: `Tests/Ability/Lab/RTAbilityLabTests.cpp`, `Tests/Player/Input/RTInputBindingTests.cpp:73`

**Interfaces:**
- Consumes: `RequestedTargetsOf`, `PlacedTargetsOf`, `MaxLabTargets`, `FRTAbilityLabFixtureSpec::TargetCount` (Task 2)
- Produces:
  - `static int32 URTAbilityLabLibrary::NextLabTargetCount(int32 Current);` va 1→2→3→1; un valore fuori scala va a 1
  - `static FString URTAbilityLabLibrary::DescribeLabTargets(int32 Requested, int32 Placed);` dà `«U» bersagli N`, più ` (qui ne stanno K)` se `Placed < Requested`
  - `static FString URTAbilityLabLibrary::LabScenarioPath(const FString& ScenarioId);` è `Combine(LabScenariosRoot(), Id + ".json")` reso assoluto, oggi dentro `WriteLabScenario`
  - `static bool URTAbilityLabLibrary::ReadLabScene(const FString& ScenarioId, FRTTestScenario& OutScene);` è `LoadFromFile(LabScenarioPath(Id))`; `false` fuori dal banco o senza file. Prima controlla `FPaths::FileExists`, così un file assente non passa dal loader e non scrive errori nel log di un test
  - `static bool URTAbilityLabLibrary::PlanLabTargets(const FString& CurrentScenarioId, FName& OutAbilityId, int32& OutNextCount);` è `false` fuori dal banco; senza file legge 1
  - `DescribeLabStep(Id)` aggiunge `  ·  ` + `DescribeLabTargets`. Legge la scena con `ReadLabScene`; senza file vale `1/1`
  - `bool ARTGameMode::CycleLabTargets(FString& OutError);` ha lo stesso contratto di `StepLabScenario`: `false` con errore vuoto fuori dal banco
  - `bool ARTGameMode::RelaunchLabScene(const FRTTestScenario& Scene, FString& OutError);` (privata) è la coda di `StepLabScenario`: `WriteLabScenario`, la CVar alla priorità corrente, la rilettura, il log e `OpenLevel`
  - `ARTPlayerController::LabTargetsAction` (`IA_LabTargets`), mappata su `EKeys::U` e legata a `OnLabTargets`

- [ ] **Step 1: Scrivi i test che falliscono.**
  - `RefactorTactics.AbilityLab.TargetKeyCyclesAndIsInertOutside`:
    - `NextLabTargetCount` dà 2, 3, 1 per 1, 2, 3, e 1 per 0 e per 7;
    - Review Focus 5: `PlanLabTargets("Movement.Basic", …)` e `PlanLabTargets("AbilityLab.", …)` sono `false` e non toccano le uscite;
    - con la radice del Lab su una cartella di prova (`SetLabScenariosRootOverrideForTest` e `LabRootDiProva`, come in `RTScenarioIndexTests.cpp:380-388`):
      - una scena di LinearDischarge scritta con `TargetCount = 3` dà `OutNextCount == 1` e `OutAbilityId == Hero.Gadget.LinearDischarge`;
      - Review Focus 1: la stessa scena salvata senza il tag `lab-targets-*` dà `OutNextCount == 2`.
  - `RefactorTactics.AbilityLab.StepHintNamesTheTargetCount`:
    - `DescribeLabTargets(2, 2) == "«U» bersagli 2"` e `DescribeLabTargets(3, 1) == "«U» bersagli 3 (qui ne stanno 1)"`;
    - nella cartella di prova, `DescribeLabStep` di una scena scritta a 3 bersagli contiene `«U» bersagli 3`, e quella di una scena con `MapRadius = 1` contiene `(qui ne stanno 1)`;
    - senza file contiene `«U» bersagli 1`.
  - In `RTInputBindingTests.cpp:73` aggiungi `TEXT("IA_LabTargets:Started")` subito dopo `IA_LabStepNext:Started`.

- [ ] **Step 2: Verifica che falliscano.** La build fallisce sui nomi nuovi.

- [ ] **Step 3: Implementa.**
  - Le funzioni della libreria come in *Produces*. `WriteLabScenario` usa `LabScenarioPath`.
  - `StepLabScenario` mette nella spec della scena successiva `TargetCount = RequestedTargetsOf` della scena corrente, letta con `ReadLabScene`; senza file vale 1.
  - `CycleLabTargets` fa `PlanLabTargets`, poi `BuildFixture` con `TargetCount = OutNextCount`, poi `RelaunchLabScene`.
  - Nel controller, `U` segue `,` e `.` riga per riga: `NewObject` accanto a `LabStepNextAction`, `MapKey` accanto a `Comma`/`Period`, `BindAction` subito dopo `LabStepNextAction`. `OnLabTargets` ha la stessa guardia di `StepLab` (`IsGameplayInputBlocked`) e lo stesso `Warning` quando c'è un errore.
  - Il commento del controller dice che `U` sta per «unità colpite», e che a tenerlo libero è `PlayerInput.HotkeysDoNotCollide`.

- [ ] **Step 4: Verifica che passino.** Build, poi `python tools/suite/esegui.py RefactorTactics.AbilityLab+RefactorTactics.PlayerInput+RefactorTactics.Scenario.AutoRunBannerShowsTheLabStep --log <scratchpad>\t3.log`.
  - Devono essere verdi `PlayerInput.HotkeysDoNotCollide` e il test della banda del GameMode. Quello confronta `GetLabStepText()` con `DescribeLabStep(LabId)` e conta gli oggetti alla seconda lettura.
  - Se il conto degli oggetti si muove, la causa è `ReadLabScene` nel percorso della seconda lettura: va letta solo quando la riga si ricompone.

- [ ] **Step 5: Commit** `feat(4136): il tasto U porta i bersagli del banco da 1 a 3, e la banda lo dice`.

### Task 4: documenti, registro e gate

**Files:**
- Modify: `docs/decisions/RT_PDR_00_Decision_Log.md` (riga `D-621`), `docs/superpowers/specs/2026-10-08-profilo-fx-per-abilita-design.md` §2.10, `docs/technical/architecture/capability-map.md` e `.yaml` (`RT-CAP-VFX`), `docs/technical/test-manuali-pie.md` (`PIE-FX-CHAIN`, voce nuova accanto a `PIE-LAB-PASSO`), `docs/technical/asset-licenze.md` (riga nuova nel registro)

- [ ] **Step 1: Aggiorna i documenti.**
  - **`D-621`.** Il titolo dice «un attore con spline per salto», non più «un sistema Niagara». Il contratto con l'asset diventa la classe con la `USplineComponent`, non più `BeamStart`/`BeamEnd`. Cita alla lettera i messaggi del §1 del disegno e linka il disegno. Aggiunge il banco a 1/2/3 bersagli col tasto `U`.
  - **§2.10 del profilo FX.** Stesso cambio di contratto, e il socket `canister_main` non è più provvisorio.
  - **`RT-CAP-VFX`.** Il nome non dice più «Niagara solo per la scarica»: dice che la scarica è un attore con spline. Correggi la riga `#4136` di `capability-map.yaml:1764` e la vista `.md`.
  - **`PIE-FX-CHAIN`.** Si guarda con 1, 2 e 3 bersagli, premendo `U` nella scena `AbilityLab.Hero.Gadget.LinearDischarge`. Il prerequisito è `BP_Gadget_LinearDischarge_Arc` nel clone principale, non più `NS_…_Arc`.
  - **`PIE-LAB-BERSAGLI`.** Voce nuova accanto a `PIE-LAB-PASSO`: `U` fa 1→2→3→1, la banda dice `«U» bersagli N`, il numero resta con `R` e passa con `,` e `.`. Segui la regola del file per il ricalcolo `doc-coherence` A1 che una voce nuova richiede.
  - **`asset-licenze.md`.** Una riga dentro i marcatori del registro, sul modello di quella di Free Spells:

    | Famiglia | Copre | Fonte | Licenza | Versione | Acquisito | UE | Attribuzione | Consumer |
    |---|---|---|---|---|---|---|---|---|
    | Free Spline VFX | `/Game/FabAsset/_SplineVFX/` | [Fab · Dr.Game](https://www.fab.com/listings/2b923e61-b02d-4cc9-bd0b-b067c9e6056e) | Fab Standard License, gratuito; la copia sul disco viene dalla libreria di un amico dell'autore, da aggiungere a quella dell'autore | listing `2b923e61` | 2026-11-13 | 5.8 | nessuna — ⛔ NoAI §5 | `BP_Gadget_LinearDischarge_Arc` |

    La data è il `mtime` della cartella, e la nota delle date al §2 del registro va estesa a questa riga.

- [ ] **Step 2: Esegui i gate.** Ogni comando deve uscire con 0:
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
  Se `generate.ts --check` è rosso, rigenera con `node tools/radar/generate.ts` e committa le viste insieme ai sorgenti.

- [ ] **Step 3: Commit** dei documenti: `docs(4136): D-621 passa alla spline, il banco a tre bersagli, e la licenza del pack`.

### Task 5: chiusura

- [ ] **Step 1: La issue del banco** (aperta: [#4164](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4164)). Aprila con `GH_TOKEN="$(gh auth token --user DegrassiAaron)" gh issue create`, sotto lo stesso Epic di #4136, col corpo scritto in un file nello scratchpad e passato con `--body-file`. Poi aggiorna titolo e corpo di #4136 da D-620 a D-621 e dal raggio alla spline, con `gh issue edit` e lo stesso `GH_TOKEN`.
- [ ] **Step 2: La guida.** Il passo d'Editor dell'autore va nella guida: duplicare `BP_SplineVFX_ElectricLightning` in `Content/RT/Characters/Gadget/FX/` come `BP_Gadget_LinearDischarge_Arc`, controllare che la timeline non resti in loop oltre la vita dell'attore, e aggiungere il pack alla propria libreria Fab.
  - La guida sta sul ramo `docs/guida-vfx-skill`, che oggi non è aperto in nessun checkout. Crea un worktree temporaneo con `git worktree add <percorso> docs/guida-vfx-skill`, committa e rimuovi il worktree.
  - Nel Registro, D-620 diventa D-621.
- [ ] **Step 3: La suite intera** sul commit finale: `python tools/suite/esegui.py RefactorTactics --timeout 15 --log <scratchpad>\suite.log`. Conta i `Result={}` dal log e controlla che trovati = verdi + rossi. Prima, `git fetch` e `git log HEAD..origin/main`: se `main` è andato avanti, rebase e rimisura.
- [ ] **Step 4: Rimisura `D-621` nei tre posti.**
  - Le righe in grassetto del registro su `origin/main`: `grep -aoE '^\| \*\*D-[0-9]+\*\*'`.
  - Ogni ramo remoto.
  - GitHub.
- [ ] **Step 5: Review avversaria** del ramo con un agente nuovo, a cui dai la PR e il diff, non le conclusioni.
- [ ] **Step 6: Pubblica e apri la PR verso `main`.**
  - Il corpo porta:
    - `Closes #4136` e `Closes` della issue del banco;
    - la tabella dei gate;
    - l'esito della review;
    - la riga `misure-strutturali` (+1 in `RTTurnManager.h`, `ChainHopTraceForTest`);
    - `Packaged NOT RUN`, e PIE ⏳ (`PIE-FX-CHAIN`, `PIE-LAB-BERSAGLI`);
    - la nota delle due rinumerazioni: D-619→D-620 per #4137, D-620→D-621 per #4139.
  - Il merge si fa solo su richiesta dell'autore.
