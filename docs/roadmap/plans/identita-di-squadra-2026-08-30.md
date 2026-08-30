# Identità di squadra sul PlayerState — piano di implementazione

> **Per chi esegue:** usa `superpowers:subagent-driven-development` (consigliata) o
> `superpowers:executing-plans` per eseguire task per task. I passi usano `- [ ]` per il tracciamento.

**Obiettivo:** spostare l'identità di squadra del giocatore da un campo editabile del controller a un
`ARTPlayerState` derivato dal formato, lasciando il gioco offline **identico**.

**Architettura:** `ARTPlayerState` porta il `TeamId`; `ARTPlayerState::TeamIdOf(const APlayerController*)`
è l'**unico** lettore e assorbe `ARTHUD::ViewerTeamIdOf`; `ARTGameMode::AssignSeats()` deriva i posti da
`UnitsPerTeam / UnitsPerPlayer` ed è idempotente perché l'ordine fra `OnPostLogin` e `BeginPlay` non è
garantito.

**Tech stack:** Unreal Engine **5.8.1**, C++ puro, Automation Framework. Nessuna dipendenza nuova.

**Spec:** [`docs/technical/systems/identita-di-squadra-spec.md`](../../technical/systems/identita-di-squadra-spec.md)

## Vincoli globali

- **Nessuna replicazione, nessuna RPC, nessun secondo client.** Questa fetta è preparazione; il debito di
  rete del presenter resta **aperto** (spec §3).
- **Il gioco offline deve restare identico** a ogni task, non solo alla fine.
- Il ripiego è **`0`**, mai `INDEX_NONE` (spec §4.2).
- Prefisso dei test nuovi: **`RefactorTactics.Player.`** — verificato libero il 2026-08-30.
- Nessun `.uasset` va toccato. Nessun asset cita `PlayerTeamId` (verificato su tutto `Content/`).
- **Non inventare API Unreal.** Le firme usate qui sono verificate su UE 5.8:
  `AGameModeBase::PlayerStateClass` (`GameModeBase.h:100`), `OnPostLogin(AController*)` (`:334`),
  `AController::SetPlayerState` (`Controller.h:51`), `GetPlayerState<T>()` (`:192`).
- Build: l'Editor dev'essere **chiuso**. Con un editor aperto su un **altro** checkout serve
  `-NoHotReloadFromIDE`.
- Il referto della suite va scritto **fuori** dall'albero: il digest di `rt-suite` copre anche gli
  untracked, e scriverlo dentro rende la misura NON REGISTRABILE.

```powershell
# Build
& "D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development `
    -Project="<repo>/RefactorTactics.uproject" -WaitMutex -NoHotReloadFromIDE
# Suite — exit: 0 verde · 1 test falliti · 2 non avviata · 3 NON VALIDA
./scripts/rt-suite.ps1 -Filter RefactorTactics.Player
```

---

## Struttura dei file

| File | Responsabilità |
|---|---|
| `Source/RefactorTactics/Player/RTPlayerState.h/.cpp` | il `TeamId` e il suo **unico** lettore |
| `Source/RefactorTactics/Tests/RTPlayerIdentityTests.cpp` | la terna che discrimina + il presidio di reflection |
| `Source/RefactorTactics/Tests/RTWorldFixtures.h` | `MakePlayerOnTeam`, una sola sede |
| `RTGameMode.h/.cpp` | `PlayerStateClass` e `AssignSeats` — composition root |
| `Match/RTMatchBootstrapper.h/.cpp` | le `FRTMatchRules` risolte escono nell'esito |

---

### Task 1: `ARTPlayerState` e il lettore unico

**Files:**
- Create: `Source/RefactorTactics/Player/RTPlayerState.h`, `Source/RefactorTactics/Player/RTPlayerState.cpp`
- Test: `Source/RefactorTactics/Tests/RTPlayerIdentityTests.cpp`

**Interfaces:**
- Consumes: niente.
- Produces: `ARTPlayerState` con `int32 GetTeamId() const`, `void AssignTeam(int32)`, e la statica
  `static int32 ARTPlayerState::TeamIdOf(const APlayerController* Controller)`.

- [ ] **Passo 1: scrivi i tre test che falliscono**

In `Source/RefactorTactics/Tests/RTPlayerIdentityTests.cpp`:

```cpp
// L'identita' di squadra e' del PlayerState, e si legge da UNA porta sola.
//
// 🔴 **La terna che discrimina.** Il ripiego a `0` ha TRE cause: nessun PlayerState, PlayerState della
// CLASSE SBAGLIATA, e squadra realmente `0`. Un presidio che ne copra due lascia scoperta la piu' comune —
// misurato il 2026-08-30: dopo `InitializeActorsForPlay` il controller ha un `APlayerState` nudo, non un
// `ARTPlayerState`, e quattro file di test passano di li'.

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Player/RTPlayerController.h"
#include "Player/RTPlayerState.h"
#include "Tests/RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

/** Con un `ARTPlayerState` assegnato, la vista e' quella. Il valore di prova e' `1` e non `0`: con `0` il
 *  test resterebbe verde anche se `TeamIdOf` rispondesse sempre il ripiego. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerTeamComesFromPlayerStateTest,
    "RefactorTactics.Player.TeamComesFromThePlayerState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerTeamComesFromPlayerStateTest::RunTest(const FString&)
{
    UWorld* World = RTWorldFixtures::MakeWorld();
    if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

    ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
    ARTPlayerState* PS = World->SpawnActor<ARTPlayerState>();
    if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("player state"), PS))
    {
        RTWorldFixtures::DestroyWorld(World);
        return false;
    }
    PC->SetPlayerState(PS);
    PS->AssignTeam(1);

    TestEqual(TEXT("la vista e' quella del PlayerState"), ARTPlayerState::TeamIdOf(PC), 1);

    // E SEGUE lo stato, non lo copia: un lettore che memorizzasse alla prima chiamata passerebbe sopra
    // e fallirebbe qui.
    PS->AssignTeam(0);
    TestEqual(TEXT("segue lo stato, non una copia"), ARTPlayerState::TeamIdOf(PC), 0);

    RTWorldFixtures::DestroyWorld(World);
    return true;
}

/** Senza PlayerState si ripiega su `0`, DICHIARATAMENTE. E' il caso dei mondi nudi. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerTeamFallsBackWithoutPlayerStateTest,
    "RefactorTactics.Player.TeamFallsBackWithoutPlayerState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerTeamFallsBackWithoutPlayerStateTest::RunTest(const FString&)
{
    UWorld* World = RTWorldFixtures::MakeWorld();
    if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

    ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
    if (!TestNotNull(TEXT("controller"), PC)) { RTWorldFixtures::DestroyWorld(World); return false; }

    TestNull(TEXT("il mondo nudo non crea un PlayerState"), PC->PlayerState.Get());
    TestEqual(TEXT("senza PlayerState: squadra 0"), ARTPlayerState::TeamIdOf(PC), 0);

    // Puro, senza mondo: la funzione e' statica apposta.
    TestEqual(TEXT("senza controller: squadra 0"), ARTPlayerState::TeamIdOf(nullptr), 0);

    RTWorldFixtures::DestroyWorld(World);
    return true;
}

/**
 * 🔴 **Il terzo caso, ed e' il piu' comune nella suite reale.** Un PlayerState c'e' ma NON e' un
 * `ARTPlayerState`: e' cio' che `InitializeActorsForPlay` produce, perche' il ripiego di
 * `InitPlayerState` pesca il game mode di DEFAULT. A colpo d'occhio il sistema sembra sano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerTeamFallsBackOnWrongPlayerStateClassTest,
    "RefactorTactics.Player.TeamFallsBackOnWrongPlayerStateClass",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerTeamFallsBackOnWrongPlayerStateClassTest::RunTest(const FString&)
{
    UWorld* World = RTWorldFixtures::MakeWorld();
    if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

    ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
    APlayerState* Nudo = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("player state nudo"), Nudo))
    {
        RTWorldFixtures::DestroyWorld(World);
        return false;
    }
    PC->SetPlayerState(Nudo);

    TestNotNull(TEXT("un PlayerState c'e'"), PC->PlayerState.Get());
    TestEqual(TEXT("ma della classe sbagliata: si ripiega su 0"), ARTPlayerState::TeamIdOf(PC), 0);

    RTWorldFixtures::DestroyWorld(World);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Passo 2: compila e verifica che NON compili**

Attesa: errore su `Player/RTPlayerState.h` inesistente. È il rosso giusto: la classe non c'è ancora.

- [ ] **Passo 3: scrivi la classe**

`Source/RefactorTactics/Player/RTPlayerState.h`:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "RTPlayerState.generated.h"

class APlayerController;

/**
 * L'IDENTITA' DI SQUADRA DEL GIOCATORE, e la sua unica porta di lettura.
 *
 * 🔑 **Il viewer e' del giocatore, non della partita.** Fino a questa fetta viveva su
 * `ARTPlayerController::PlayerTeamId`, un `EditDefaultsOnly` che valeva `0` e che nessuno assegnava a
 * runtime: con due client varrebbe `0` per entrambi, e si romperebbe **in silenzio** perche' `0` e' una
 * risposta plausibile.
 *
 * ⛔ **`TeamId` non e' editabile**: e' stato di runtime, scritto da `AssignTeam`. Quando la replicazione
 * arrivera' il campo diventa `Replicated` con condizione owner-only e il seam e' gia' al posto giusto.
 */
UCLASS()
class REFACTORTACTICS_API ARTPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    /** La squadra assegnata. Ripiego `0` finche' nessuno ha assegnato: vedi `TeamIdOf`. */
    int32 GetTeamId() const { return TeamId; }

    /**
     * Assegna la squadra. **Il nome dichiara l'autorita'**: oggi chiama solo `ARTGameMode::AssignSeats`,
     * e domani sara' il lato server a farlo.
     */
    void AssignTeam(int32 InTeamId) { TeamId = InTeamId; }

    /**
     * 🔑 **L'UNICA porta.** Risale dal controller al proprio `ARTPlayerState` e ripiega su `0`.
     *
     * ⚠️ **Il ripiego ha TRE cause e una sola risposta**: controller nullo, nessun PlayerState, PlayerState
     * della classe sbagliata — quest'ultimo e' cio' che `InitializeActorsForPlay` produce nei mondi di
     * prova, misurato il 2026-08-30. Tutte e tre valgono `0`.
     *
     * ⛔ **`0` e non `INDEX_NONE`**, e la ragione non e' pigrizia: `URTIntentPrivacyLibrary::FilterForTeam`
     * decide con `Intent.TeamId == ObserverTeamId`, quindi un osservatore invalido non nasconde **di
     * meno** — rovescia la simmetria, e gli intenti non rivelati dell'avversario diventano «alleati».
     * Nessuno dei consumatori ha una risposta per «nessuna squadra».
     */
    static int32 TeamIdOf(const APlayerController* Controller);

private:
    int32 TeamId = 0;
};
```

`Source/RefactorTactics/Player/RTPlayerState.cpp`:

```cpp
#include "Player/RTPlayerState.h"

#include "GameFramework/PlayerController.h"

int32 ARTPlayerState::TeamIdOf(const APlayerController* Controller)
{
    if (const ARTPlayerState* PS = Controller ? Cast<ARTPlayerState>(Controller->PlayerState) : nullptr)
    {
        return PS->GetTeamId();
    }
    return 0;
}
```

- [ ] **Passo 4: compila e lancia i tre test**

```powershell
./scripts/rt-suite.ps1 -Filter RefactorTactics.Player
```
Attesa: `VALIDA`, 3/3, 0 fallimenti. ⚠️ Se `TeamFallsBackWithoutPlayerState` fallisce sul `TestNull`, il
mondo di prova sta creando un PlayerState: **fermati e rileggi §5 dello spec**, la premessa è cambiata.

- [ ] **Passo 5: commit**

```bash
git add Source/RefactorTactics/Player/RTPlayerState.h Source/RefactorTactics/Player/RTPlayerState.cpp Source/RefactorTactics/Tests/RTPlayerIdentityTests.cpp
git commit -m "feat(player): l'identita' di squadra e' del PlayerState, con una porta sola"
```

---

### Task 2: la fixture che garantisce la classe

**Files:**
- Modify: `Source/RefactorTactics/Tests/RTWorldFixtures.h`
- Test: `Source/RefactorTactics/Tests/RTPlayerIdentityTests.cpp`

**Interfaces:**
- Consumes: `ARTPlayerState::AssignTeam`, `ARTPlayerState::TeamIdOf` (Task 1).
- Produces: `RTWorldFixtures::MakePlayerOnTeam(UWorld* World, int32 TeamId) -> ARTPlayerController*`.

- [ ] **Passo 1: scrivi il test che falliscono**

Aggiungi in `RTPlayerIdentityTests.cpp` (prima di `#endif`):

```cpp
/**
 * La fixture non CREA soltanto un PlayerState: ne garantisce la CLASSE, sostituendo quello di default che
 * `InitializeActorsForPlay` ha gia' messo li'. Senza questa sostituzione il test riceverebbe un
 * `APlayerState` nudo e leggerebbe il ripiego credendo di leggere la squadra.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerFixtureReplacesDefaultPlayerStateTest,
    "RefactorTactics.Player.FixtureReplacesTheDefaultPlayerState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerFixtureReplacesDefaultPlayerStateTest::RunTest(const FString&)
{
    UWorld* World = RTWorldFixtures::MakeWorld();
    if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

    // È il mondo che produce il PlayerState di DEFAULT: il caso misurato in §5 dello spec.
    World->InitializeActorsForPlay(FURL());

    ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, 1);
    if (!TestNotNull(TEXT("la fixture produce un controller"), PC))
    {
        RTWorldFixtures::DestroyWorld(World);
        return false;
    }

    TestNotNull(TEXT("ed e' un ARTPlayerState, non quello di default"),
        Cast<ARTPlayerState>(PC->PlayerState));
    TestEqual(TEXT("con la squadra chiesta"), ARTPlayerState::TeamIdOf(PC), 1);

    RTWorldFixtures::DestroyWorld(World);
    return true;
}
```

- [ ] **Passo 2: compila e verifica che NON compili**

Attesa: `MakePlayerOnTeam` non è un membro di `RTWorldFixtures`.

- [ ] **Passo 3: aggiungi la fixture**

In `Source/RefactorTactics/Tests/RTWorldFixtures.h`, dentro `namespace RTWorldFixtures`, dopo
`DestroyWorld`. Aggiungi in testa al file `#include "Player/RTPlayerController.h"` e
`#include "Player/RTPlayerState.h"`.

```cpp
	/**
	 * Un giocatore su una squadra dichiarata: controller + `ARTPlayerState`, legati.
	 *
	 * ⚠️ **Sostituisce il PlayerState esistente invece di crearne uno solo se manca.** Misurato il
	 * 2026-08-30: dopo `InitializeActorsForPlay` il controller ha gia' un `APlayerState` **nudo**, perche'
	 * il ripiego di `InitPlayerState` pesca il game mode di DEFAULT quando `GetAuthGameMode()` e' nullo.
	 * Una fixture che si limitasse a riempire il vuoto lascerebbe la classe sbagliata, e il test leggerebbe
	 * il ripiego credendo di leggere la squadra — verde, e muto.
	 *
	 * ⛔ Non registra `PlayerStateClass` sul GameMode: in questi mondi il nostro GameMode non e' l'autorita'
	 * che decide la classe, quindi non servirebbe a niente.
	 */
	inline ARTPlayerController* MakePlayerOnTeam(UWorld* World, int32 TeamId)
	{
		if (!World)
		{
			return nullptr;
		}
		ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
		ARTPlayerState* PS = World->SpawnActor<ARTPlayerState>();
		if (!PC || !PS)
		{
			return nullptr;
		}
		PC->SetPlayerState(PS);
		PS->AssignTeam(TeamId);
		return PC;
	}
```

- [ ] **Passo 4: compila e lancia**

```powershell
./scripts/rt-suite.ps1 -Filter RefactorTactics.Player
```
Attesa: `VALIDA`, 4/4, 0 fallimenti.

- [ ] **Passo 5: commit**

```bash
git add Source/RefactorTactics/Tests/RTWorldFixtures.h Source/RefactorTactics/Tests/RTPlayerIdentityTests.cpp
git commit -m "test(player): una fixture che garantisce la CLASSE del PlayerState, non solo la sua presenza"
```

---

### Task 3: migrare i cinque lettori e assorbire `ViewerTeamIdOf`

**Files:**
- Modify: `Source/RefactorTactics/Camera/RTCameraPawn.cpp` (`FrameOwnTeam`)
- Modify: `Source/RefactorTactics/Player/RTPlayerController.cpp` (righe ~696 e ~716)
- Modify: `Source/RefactorTactics/Perception/RTKnowledgeVeilPresenter.cpp` (`ViewerTeamId`)
- Modify: `Source/RefactorTactics/UI/RTHUD.h`, `Source/RefactorTactics/UI/RTHUD.cpp`
- Modify: `Source/RefactorTactics/Tests/RTHudViewerTeamTests.cpp`, `Source/RefactorTactics/Tests/RTCameraPawnTests.cpp`

**Interfaces:**
- Consumes: `ARTPlayerState::TeamIdOf` (Task 1), `RTWorldFixtures::MakePlayerOnTeam` (Task 2).
- Produces: nessuna API nuova. `ARTHUD::ViewerTeamIdOf` **cessa di esistere**.

- [ ] **Passo 1: sposta i due test dell'HUD sulla nuova porta**

In `RTHudViewerTeamTests.cpp`: sostituisci `#include "UI/RTHUD.h"` con
`#include "Player/RTPlayerState.h"` e `#include "Tests/RTWorldFixtures.h"`, e riscrivi i corpi:

```cpp
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, 1);
	if (!TestNotNull(TEXT("controller"), PC)) { HvtDestroyWorld(World); return false; }

	TestEqual(TEXT("la squadra e' quella del PlayerState"), ARTPlayerState::TeamIdOf(PC), 1);

	Cast<ARTPlayerState>(PC->PlayerState)->AssignTeam(0);
	TestEqual(TEXT("segue lo stato, non una copia"), ARTPlayerState::TeamIdOf(PC), 0);

	APlayerController* Plain = World->SpawnActor<APlayerController>();
	if (TestNotNull(TEXT("controller generico"), Plain))
	{
		TestEqual(TEXT("controller senza PlayerState RT: ripiega su 0"), ARTPlayerState::TeamIdOf(Plain), 0);
	}
```

e nel secondo test `ARTHUD::ViewerTeamIdOf(nullptr)` → `ARTPlayerState::TeamIdOf(nullptr)`.

⚠️ **Le attese non cambiano**: `1`, `0`, `0`, `0`. Cambia l'indirizzo, non il contratto.

- [ ] **Passo 2: sposta il test della camera sulla fixture**

In `RTCameraPawnTests.cpp`, sostituisci le tre righe che spawnano il controller e scrivono il campo:

```cpp
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, 1);
	if (!TestNotNull(TEXT("controller"), PC)) { DestroyCameraWorld(World); return false; }
	PC->Possess(Cam);
```

Aggiungi `#include "Tests/RTWorldFixtures.h"` se assente. Aggiorna il commento che nomina
`PlayerTeamId` in `ARTPlayerState::TeamIdOf`.

- [ ] **Passo 3: compila e verifica che NON compili**

Attesa: `PlayerTeamId` non è più usato dai test, ma i **cinque lettori di produzione** lo usano ancora e
compilano. I test falliranno a runtime, non in compilazione: è il rosso atteso.

- [ ] **Passo 4: migra i cinque lettori**

`RTCameraPawn.cpp` — sostituisci il blocco `int32 TeamId = 0; if (const ARTPlayerController* PC = ...)`:

```cpp
	// La squadra da inquadrare e' quella del giocatore; senza PlayerState si assume la 0 (demo).
	const int32 TeamId = ARTPlayerState::TeamIdOf(Cast<APlayerController>(GetController()));
```

`RTPlayerController.cpp` — nei due siti, `PlayerTeamId` → `ARTPlayerState::TeamIdOf(this)`.

`RTKnowledgeVeilPresenter.cpp` — il corpo di `ViewerTeamId()`:

```cpp
	// L'`Outer` E' il viewer: il presenter appartiene al client che guarda. La squadra la risponde il
	// PlayerState, con il ripiego a `0` che vale per tutti e tre i modi di non averla.
	return ARTPlayerState::TeamIdOf(Cast<APlayerController>(GetOuter()));
```

`RTHUD.cpp` — **cancella** `ARTHUD::ViewerTeamIdOf` e sostituisci la riga 455:

```cpp
	const int32 PlayerTeamId = ARTPlayerState::TeamIdOf(GetOwningPlayerController());
```

`RTHUD.h` — **cancella** la dichiarazione di `ViewerTeamIdOf`, e **trasferisci** la sua prosa (la ragione
di `D-242` punto 5, sul letterale che non ha un modo di fallire chiuso) nel docstring di
`ARTPlayerState::TeamIdOf`, se non già presente.

Aggiungi `#include "Player/RTPlayerState.h"` nei quattro `.cpp` toccati.

- [ ] **Passo 5: compila e lancia le aree toccate**

```powershell
./scripts/rt-suite.ps1 -Filter RefactorTactics.Player
./scripts/rt-suite.ps1 -Filter RefactorTactics.Hud
./scripts/rt-suite.ps1 -Filter RefactorTactics.Camera
./scripts/rt-suite.ps1 -Filter RefactorTactics.Veil
```
Attesa: `VALIDA`, 0 fallimenti in tutte e quattro.

- [ ] **Passo 6: commit**

```bash
git add Source/RefactorTactics
git commit -m "refactor(player): i cinque lettori passano da una porta sola, e ViewerTeamIdOf viene assorbita"
```

---

### Task 4: togliere il campo, e presidiarne il ritorno

**Files:**
- Modify: `Source/RefactorTactics/Player/RTPlayerController.h` (rimozione di `PlayerTeamId`)
- Test: `Source/RefactorTactics/Tests/RTPlayerIdentityTests.cpp`

**Interfaces:**
- Consumes: tutto Task 3 (nessun lettore residuo).
- Produces: nessuna.

- [ ] **Passo 1: scrivi il presidio che fallisce**

```cpp
/**
 * 🔴 **Il campo editabile non deve tornare.** E' cosi' che il letterale e' tornato la prima volta.
 *
 * Interroga la classe REALE con la reflection, non un elenco scritto qui: chi riaprisse la proprieta'
 * trova rosso senza che nessuno aggiorni il test. E' lo stesso modello di
 * `Heroes.AbilityIdsAreNamespacedUnderTheirHero` e `Unit.CanonicalHeroIdHasNoLegacyName`.
 *
 * ⚠️ **Limite dichiarato**: coglie il campo riaperto, NON un lettore nuovo che inlinei
 * `Cast<ARTPlayerState>(PC->PlayerState)->GetTeamId()` duplicando il ripiego. Contro quello la difesa e' la
 * prosa su `TeamIdOf`, ed e' una difesa parziale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerControllerHasNoTeamFieldTest,
    "RefactorTactics.Player.ControllerCarriesNoTeamField",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerControllerHasNoTeamFieldTest::RunTest(const FString&)
{
    TArray<FString> Colpevoli;
    for (TFieldIterator<FProperty> It(ARTPlayerController::StaticClass(),
             EFieldIteratorFlags::ExcludeSuper); It; ++It)
    {
        const FString Nome = It->GetName();
        if (Nome.Contains(TEXT("TeamId")))
        {
            Colpevoli.Add(Nome);
        }
    }

    TestEqual(*FString::Printf(TEXT("nessuna UPROPERTY di squadra sul controller (trovate: %s)"),
            Colpevoli.Num() > 0 ? *FString::Join(Colpevoli, TEXT(", ")) : TEXT("nessuna")),
        Colpevoli.Num(), 0);
    return true;
}
```

Aggiungi `#include "UObject/UnrealType.h"` in testa al file.

- [ ] **Passo 2: lancia e verifica che FALLISCA**

```powershell
./scripts/rt-suite.ps1 -Filter RefactorTactics.Player.ControllerCarriesNoTeamField
```
Attesa: **FAIL**, con `PlayerTeamId` fra i colpevoli. È la prova che il test discrimina.

- [ ] **Passo 3: togli il campo**

In `RTPlayerController.h`, cancella la `UPROPERTY` `PlayerTeamId` e il suo docstring. Aggiorna il
docstring di `GetKnowledgeVeilPresenter()` che la nomina, e il commento su `IsPlanningInputInert()` che
cita `CanPlayerControlUnit` decidere su `UnitTeamId == PlayerTeamId`, rimandando a
`ARTPlayerState::TeamIdOf`.

- [ ] **Passo 4: compila e lancia la suite INTERA**

```powershell
./scripts/rt-suite.ps1
```
Attesa: `VALIDA`, 0 fallimenti. È il primo punto in cui il campo non esiste più: qualunque lettore
dimenticato non compilerebbe.

- [ ] **Passo 5: commit**

```bash
git add Source/RefactorTactics
git commit -m "refactor(player): via il campo editabile, e un presidio di reflection perche' non torni"
```

---

### Task 5: assegnare i posti dal formato

**Files:**
- Modify: `Source/RefactorTactics/Match/RTMatchBootstrapper.h`, `Source/RefactorTactics/Match/RTMatchBootstrapper.cpp`
- Modify: `Source/RefactorTactics/RTGameMode.h`, `Source/RefactorTactics/RTGameMode.cpp`
- Test: `Source/RefactorTactics/Tests/RTPlayerIdentityTests.cpp`

**Interfaces:**
- Consumes: `ARTPlayerState::AssignTeam` (Task 1).
- Produces: `FRTMatchBootstrapOutcome::Rules` (`FRTMatchRules`), `ARTGameMode::AssignSeats()`.

- [ ] **Passo 1: scrivi i test che falliscono**

```cpp
/**
 * I posti si derivano dal FORMATO, e l'assegnazione e' idempotente e indipendente dall'ordine.
 *
 * 🔴 `OnPostLogin` e `BeginPlay` non hanno un ordine garantito, e il formato e' risolto solo
 * nell'allestimento: `AssignSeats` chiamata prima delle regole non deve fare nulla NE' sporcare niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSeatsComeFromTheFormatTest,
    "RefactorTactics.Player.SeatsComeFromTheFormat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSeatsComeFromTheFormatTest::RunTest(const FString&)
{
    UWorld* World = RTWorldFixtures::MakeWorld();
    if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

    // ⚠️ La mappa si costruisce con `MakeFlatArena` e si assegna a mano, come fa
    // `RTMatchFormatWorldTests`: cosi' l'allestimento non emette l'avviso dell'arena generata e il test non
    // deve dichiarare un `AddExpectedError` il cui conteggio andrebbe misurato a parte.
    ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
    if (HexMap)
    {
        HexMap->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 4);
    }
    World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
    ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
    ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, 1);
    if (!TestNotNull(TEXT("mappa"), HexMap) || !TestNotNull(TEXT("game mode"), GameMode)
        || !TestNotNull(TEXT("controller"), PC))
    {
        RTWorldFixtures::DestroyWorld(World);
        return false;
    }

    // PRIMA delle regole: non fa nulla e non sporca. La squadra resta quella scritta dalla fixture.
    GameMode->AssignSeats();
    TestEqual(TEXT("senza regole non assegna"), ARTPlayerState::TeamIdOf(PC), 1);

    // Allestire risolve il formato e assegna: `Format.Skirmish2v2` fa 2/2 = 1 posto per squadra, quindi
    // l'unico giocatore prende la squadra 0.
    GameMode->SetupHexMatch(HexMap);
    TestEqual(TEXT("un posto per squadra: il primo arrivato e' la squadra 0"),
        ARTPlayerState::TeamIdOf(PC), 0);

    // Idempotente: richiamarla non sposta nessuno.
    GameMode->AssignSeats();
    TestEqual(TEXT("idempotente"), ARTPlayerState::TeamIdOf(PC), 0);

    RTWorldFixtures::DestroyWorld(World);
    return true;
}

/**
 * 🔑 **Il conteggio dei posti CAMBIA col formato, ed e' l'unico test che lo dimostra.**
 *
 * ⚠️ Con due soli giocatori non si distingue: l'alternanza da' `0` e `1` sia con un posto per squadra sia
 * con due. Cio' che discrimina e' il TERZO arrivo — rifiutato quando i posti sono due, accolto sulla
 * squadra `0` quando sono quattro. Senza questa asimmetria il test passerebbe anche con un numero di posti
 * scritto in una costante.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSeatCountFollowsUnitsPerPlayerTest,
    "RefactorTactics.Player.SeatCountFollowsUnitsPerPlayer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSeatCountFollowsUnitsPerPlayerTest::RunTest(const FString&)
{
    // `UnitsPerTeam = 2`, `UnitsPerPlayer = 1` -> due posti per squadra, quattro in tutto: il terzo
    // giocatore ENTRA, sulla squadra 0. Con `Format.Skirmish2v2` (2/2 = un posto per squadra) sarebbe
    // rimasto fuori.
    UWorld* World = RTWorldFixtures::MakeWorld();
    if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

    ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
    if (HexMap)
    {
        HexMap->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 4);
    }
    World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
    ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
    if (!TestNotNull(TEXT("mappa"), HexMap) || !TestNotNull(TEXT("game mode"), GameMode))
    {
        RTWorldFixtures::DestroyWorld(World);
        return false;
    }

    URTMatchFormatData* Format = NewObject<URTMatchFormatData>();
    Format->FormatId = FName(TEXT("Format.SeatTest2v2"));
    Format->RoundLimit = 12;
    Format->ExpectedRounds = 12;
    Format->ScoreToWin = 0;
    Format->UnitsPerTeam = 2;
    Format->UnitsPerPlayer = 1;   // <- due posti per squadra, non uno
    GameMode->MatchFormat = Format;

    // Il valore sentinella `9` NON e' decorativo: se `AssignSeats` non assegnasse nessuno, le squadre
    // resterebbero `{9, 9, 9}` e il test lo direbbe. Partendo da `0` un `AssignSeats` inerte passerebbe
    // per due terzi.
    ARTPlayerController* P0 = RTWorldFixtures::MakePlayerOnTeam(World, 9);
    ARTPlayerController* P1 = RTWorldFixtures::MakePlayerOnTeam(World, 9);
    ARTPlayerController* P2 = RTWorldFixtures::MakePlayerOnTeam(World, 9);
    if (!TestNotNull(TEXT("tre controller"), P2))
    {
        RTWorldFixtures::DestroyWorld(World);
        return false;
    }

    GameMode->SetupHexMatch(HexMap);

    // ⚠️ L'ordine dell'iteratore dei controller non e' quello di spawn per contratto: si asserisce
    // sull'INSIEME delle squadre assegnate, non su quale controller ha preso quale.
    TArray<int32> Squadre = { ARTPlayerState::TeamIdOf(P0), ARTPlayerState::TeamIdOf(P1),
                              ARTPlayerState::TeamIdOf(P2) };
    Squadre.Sort();

    TestEqual(TEXT("tre giocatori entrano tutti: due posti per squadra, nessuno resta al sentinella"),
        Squadre, TArray<int32>({ 0, 0, 1 }));

    RTWorldFixtures::DestroyWorld(World);
    return true;
}
```

⚠️ Include necessari per questi due test, in testa a `RTPlayerIdentityTests.cpp`: `Map/RTHexMapActor.h`,
`Map/RTHexMapAsset.h`, `Turn/RTTurnManager.h`, `Turn/RTMatchSetupLibrary.h`, `Turn/RTMatchFormatData.h`,
`RTGameMode.h`.

- [ ] **Passo 2: le regole escono nell'esito del bootstrapper**

In `RTMatchBootstrapper.h`, aggiungi a `FRTMatchBootstrapOutcome` (e `#include "Turn/RTMatchFormatData.h"`):

```cpp
	/**
	 * Le regole in vigore, valide **solo** se `bModeLatched`. Escono di qui perche' il composition root ne
	 * ha bisogno per derivare i posti dei giocatori: e' un OUTPUT dell'allestimento, non una seconda
	 * risoluzione del formato — quella resta una sola.
	 */
	FRTMatchRules Rules;
```

In `RTMatchBootstrapper.cpp`, subito dopo `Outcome.bAutobattleInEffect = Config.bAutobattle;`:

```cpp
	Outcome.Rules = Rules;
```

- [ ] **Passo 3: `PlayerStateClass` e `AssignSeats`**

In `RTGameMode.cpp`, nel costruttore dopo `HUDClass = ARTHUD::StaticClass();`:

```cpp
	// L'identita' di squadra e' del client, e vive sul PlayerState. E' un framework default come i tre
	// qui sopra, quindi sta nello stesso posto.
	PlayerStateClass = ARTPlayerState::StaticClass();
```

In `RTGameMode.h`, sezione pubblica:

```cpp
	/**
	 * Assegna la squadra ai giocatori presenti, derivando i posti dal formato.
	 *
	 * ⚠️ **Idempotente e chiamata da DUE lati** — `OnPostLogin` e `SetupHexMatch` — perche' il motore non
	 * garantisce il loro ordine e le regole esistono solo dopo l'allestimento. Senza regole non fa nulla.
	 */
	void AssignSeats();

protected:
	virtual void OnPostLogin(AController* NewPlayer) override;

private:
	/** Le regole dell'ultimo allestimento riuscito. Vuote finche' non c'e' stato. */
	FRTMatchRules AssignedRules;
	bool bHasRules = false;
```

In `RTGameMode.cpp`:

```cpp
void ARTGameMode::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);
	AssignSeats();
}

void ARTGameMode::AssignSeats()
{
	if (!bHasRules)
	{
		// Le regole non ci sono ancora: chi ci passa adesso verra' assegnato da `SetupHexMatch`.
		return;
	}

	// ⛔ Fail-closed sulla divisione: il default di `UnitsPerPlayer` e' `0`, e dividere per zero sarebbe
	// un crash, non un ripiego.
	if (AssignedRules.UnitsPerPlayer <= 0 || AssignedRules.UnitsPerTeam <= 0)
	{
		UE_LOG(LogRT, Warning,
			TEXT("[RT] Il formato '%s' non dichiara quante unita' comanda una persona "
				 "(UnitsPerTeam=%d, UnitsPerPlayer=%d): nessun posto assegnato."),
			*AssignedRules.FormatId.ToString(), AssignedRules.UnitsPerTeam, AssignedRules.UnitsPerPlayer);
		return;
	}

	const int32 SeatsPerTeam = AssignedRules.UnitsPerTeam / AssignedRules.UnitsPerPlayer;
	const int32 TotalSeats = SeatsPerTeam * 2;

	int32 Arrival = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ARTPlayerState* PS = It->IsValid() ? (*It)->GetPlayerState<ARTPlayerState>() : nullptr;
		if (!PS)
		{
			continue;
		}

		if (Arrival >= TotalSeats)
		{
			UE_LOG(LogRT, Warning,
				TEXT("[RT] Piu' giocatori (%d) che posti (%d) nel formato '%s': l'eccedenza resta senza "
					 "squadra assegnata."),
				Arrival + 1, TotalSeats, *AssignedRules.FormatId.ToString());
			break;
		}

		// ALTERNATO e non «prima una squadra e poi l'altra»: con un posto per squadra il secondo arrivato
		// prende la `1`, che e' il caso utile. Riempire prima la `0` metterebbe due persone dalla stessa
		// parte lasciando l'altra vuota.
		PS->AssignTeam(Arrival % 2);
		++Arrival;
	}
}
```

E in `SetupHexMatch`, dentro `if (Outcome.bModeLatched)`, dopo le due righe esistenti:

```cpp
		AssignedRules = Outcome.Rules;
		bHasRules = true;
		AssignSeats();
```

Aggiungi `#include "Player/RTPlayerState.h"` e `#include "Turn/RTMatchFormatData.h"` a `RTGameMode.cpp`,
e `#include "Turn/RTMatchFormatData.h"` a `RTGameMode.h` (serve per il membro `FRTMatchRules`).

- [ ] **Passo 4: compila e lancia la suite INTERA**

```powershell
./scripts/rt-suite.ps1
```
Attesa: `VALIDA`, 0 fallimenti. ⚠️ Se `RTHeroSpawnTests` o `RTMatchAutobattleTests` diventano rossi con
avvisi «SCAVALCA» in più o in meno, hai cambiato **quante volte** un resolver viene chiamato: rileggi il
commit `0c9fed4a` prima di toccare le attese.

- [ ] **Passo 5: commit**

```bash
git add Source/RefactorTactics
git commit -m "feat(match): i posti dei giocatori si derivano dal formato, e UnitsPerPlayer trova un consumatore"
```

---

### Task 6: mettere a verbale

**Files:**
- Modify: `docs/decisions/RT_PDR_00_Decision_Log.md`
- Modify: `docs/technical/architecture/architettura-codice.md`

- [ ] **Passo 1: prendi un `D-nnn` libero**

⛔ **Non copiare l'ultimo ID da un documento**: scade in ore. Leggilo dai **ref remoti**, perché fra il
commit che prende un ID e l'apertura della sua PR c'è una finestra in cui l'ID è preso e invisibile, e
`gh pr list` non la mostra.

```bash
git fetch --prune origin
for b in $(git branch -r --format='%(refname:short)' | grep -v HEAD); do
  id=$(git show "$b:docs/decisions/RT_PDR_00_Decision_Log.md" 2>/dev/null \
       | grep -oE '^\| \*\*D-[0-9]{3}\*\*' | grep -oE 'D-[0-9]{3}' | sort -u | tail -1)
  [ -n "$id" ] && echo "$b -> $id"
done
```

- [ ] **Passo 2: scrivi la voce**

Contenuto: l'identità di squadra vive sul `PlayerState`, si deriva dal formato, ha **un lettore solo**, il
ripiego a `0` ha **tre cause** ed è testato per tutte e tre; `UnitsPerPlayer` riceve il suo primo
consumatore vivo; il presidio di reflection e il suo **limite dichiarato**; e — esplicitamente — che questa
fetta **non** paga il debito di rete del presenter.

- [ ] **Passo 3: aggiungi le righe alla mappa del codice**

In `architettura-codice.md`, una riga per `Player/RTPlayerState`, e aggiorna quella di `RTGameMode`
aggiungendo l'assegnazione dei posti fra le sue responsabilità di composition root.

- [ ] **Passo 4: gate documentali**

```bash
node tools/radar/doc-links.ts --check
node tools/radar/doc-tables.ts --check
```
⚠️ `doc-links` è **già rosso su `main`** per 8 link in 6 documenti verso 3 file mancanti, nessuno di questa
fetta. Confronta l'elenco prima e dopo: deve essere **identico**.

- [ ] **Passo 5: riverifica il `D-nnn` e commit**

Rilancia il comando del Passo 1: un branch che nel frattempo avesse rivendicato lo stesso ID è una
collisione, e si rinumera **la seconda**.

```bash
git add docs
git commit -m "docs(D-nnn): l'identita' di squadra e' del PlayerState, derivata dal formato"
```

---

## Chiusura

- [ ] `./scripts/rt-suite.ps1` — **VALIDA**, 0 fallimenti, sul commit di testa
- [ ] `git diff --check`
- [ ] PR verso `main` (mai un branch diverso senza averlo verificato)
- [ ] ⏳ **PIE**: `PIE-HEX-VIZ-VELO` resta l'unica voce eseguibile e resta **dovuta** — il velo con un
      viewer che ora arriva dal PlayerState non è mai stato guardato a schermo.
