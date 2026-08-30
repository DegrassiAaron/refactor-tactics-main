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
