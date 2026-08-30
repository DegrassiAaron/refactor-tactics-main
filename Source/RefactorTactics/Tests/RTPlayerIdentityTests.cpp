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
#include "UObject/UnrealType.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTMatchFormatData.h"
#include "RTGameMode.h"

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
 *
 * ✅ **Il canarino del presidio.** Zero colpevoli su un'iterazione VUOTA e' indistinguibile, nell'asserzione
 * sopra, da zero colpevoli su un'iterazione che ha davvero scandito la classe: se `ExcludeSuper` fosse
 * invertito, o la classe passata fosse sbagliata, l'iterazione sarebbe vuota e questo test passerebbe verde
 * senza presidiare niente. Per questo si conta anche QUANTE proprieta' sono state visitate e si asserisce
 * che siano piu' di zero — `ARTPlayerController` dichiara circa diciannove `UPROPERTY` proprie, quindi la
 * soglia e' ampiamente sicura e rende il presidio strutturalmente incapace di essere vuoto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerControllerHasNoTeamFieldTest,
	"RefactorTactics.Player.ControllerCarriesNoTeamField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerControllerHasNoTeamFieldTest::RunTest(const FString&)
{
	TArray<FString> Colpevoli;
	int32 ProprietaVisitate = 0;
	for (TFieldIterator<FProperty> It(ARTPlayerController::StaticClass(),
			 EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		++ProprietaVisitate;
		const FString Nome = It->GetName();
		if (Nome.Contains(TEXT("TeamId")))
		{
			Colpevoli.Add(Nome);
		}
	}

	// Il canarino: senza questa riga un `ExcludeSuper` invertito (o la classe sbagliata) darebbe
	// un'iterazione vuota, zero colpevoli, e il test sotto passerebbe verde senza aver guardato niente.
	TestTrue(*FString::Printf(
			TEXT("l'iterazione ha visitato proprieta' reali (%d): un'iterazione vuota non presidierebbe nulla"),
			ProprietaVisitate),
		ProprietaVisitate > 0);

	TestEqual(*FString::Printf(TEXT("nessuna UPROPERTY di squadra sul controller (trovate: %s)"),
			Colpevoli.Num() > 0 ? *FString::Join(Colpevoli, TEXT(", ")) : TEXT("nessuna")),
		Colpevoli.Num(), 0);
	return true;
}

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

	// 🔴 SENZA questa riga `GetWorld()->GetPlayerControllerIterator()` in `AssignSeats` e' VUOTO: senza
	// `InitializeActorsForPlay` il mondo non e' `AreActorsInitialized()`, `AActor::PostActorConstruction`
	// salta `PostInitializeComponents()` per ogni Actor spawnato dopo — e `AController::AddController`,
	// che popola `PlayerControllerList`, vive proprio li'. Stessa premessa gia' documentata per
	// `InitPlayerState` in `FRTPlayerFixtureReplacesDefaultPlayerStateTest` piu' sopra: e' la stessa
	// chiamata mancante, e qui morde su un lettore diverso (l'iteratore, non lo stato).
	World->InitializeActorsForPlay(FURL());

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

	// Vedi `FRTSeatsComeFromTheFormatTest` per il perche': senza questa riga il mondo non e'
	// `AreActorsInitialized()`, `AddController` non corre mai, e i tre controller sotto restano invisibili
	// a `GetPlayerControllerIterator()`.
	World->InitializeActorsForPlay(FURL());

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

	// ✅ L'ordine dell'iteratore E' l'ordine di arrivo, e non e' un'assunzione: `UWorld::AddController`
	// (Engine/Private/World.cpp) fa `PlayerControllerList.Add(PlayerController)` — un append, guardato da
	// `Contains` — e `GetPlayerControllerIterator()` e' un `TConstIterator` su quello stesso array.
	// Si asserisce quindi la SEQUENZA per arrivo, non l'insieme: e' l'unica asserzione che distingue
	// l'alternanza (`Arrival % 2`) dal riempimento a blocchi, che su tre arrivi produrrebbero insiemi
	// diversi ma stessi conteggi per squadra — ordinati sarebbero indistinguibili.
	TArray<int32> Squadre = { ARTPlayerState::TeamIdOf(P0), ARTPlayerState::TeamIdOf(P1),
							  ARTPlayerState::TeamIdOf(P2) };

	TestEqual(TEXT("alternanza per arrivo: squadra 0, poi 1, poi di nuovo 0 - non riempimento a blocchi"),
		Squadre, TArray<int32>({ 0, 1, 0 }));

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
