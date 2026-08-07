#include "Misc/AutomationTest.h"
#include "RTGameMode.h"
#include "Core/RTTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTMatchFormatData.h"
#include "Turn/RTMatchFormatLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti dagli helper degli altri file di test: nella unity build i namespace anonimi si fondono.
	UWorld* MakeFormatWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyFormatWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTHexMapActor* SpawnFormatMap(UWorld* World, int32 Radius = 2)
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			Map->AddOrUpdateCell(FRTHexCellData(Id));
		}
		Map->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = Map;
		return Actor;
	}

	URTMatchFormatData* MakeFormatAsset(int32 RoundLimit, const TCHAR* Id = TEXT("Format.WorldTest"))
	{
		URTMatchFormatData* Format = NewObject<URTMatchFormatData>();
		Format->FormatId = FName(Id);
		Format->FormatVersion = 1;
		Format->RoundLimit = RoundLimit;
		Format->ExpectedRounds = RoundLimit;
		Format->ScoreToWin = 0;
		return Format;
	}

	int32 CountUnits(UWorld* World)
	{
		int32 N = 0;
		for (TActorIterator<ARTUnit> It(World); It; ++It) { ++N; }
		return N;
	}

	/**
	 * Un'unita' viva e FERMA: questi test verificano quando la partita finisce, non come si combatte, e due
	 * unita' che non si toccano sono l'unico modo per far arrivare una partita fino allo scadere dei round.
	 */
	ARTUnit* SpawnFormatUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureAsArchetype(ERTArchetype::Guardian);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->DispatchBeginPlay();
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Un round completo senza pianificazione: nessuno agisce, il turno passa. */
	void PlayEmptyRound(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}
}

/**
 * La via del `RoundLimit` in una partita VERA, non solo nella regola pura: senza questo test la funzione
 * potrebbe essere corretta e non venire chiamata da nessuno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchRoundLimitEndsPlayedMatchTest,
	"RefactorTactics.Match.TurnLimitEndsAPlayedMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchRoundLimitEndsPlayedMatchTest::RunTest(const FString&)
{
	UWorld* World = MakeFormatWorld();
	if (!TestNotNull(TEXT("world di prova"), World))
	{
		return false;
	}
	SpawnFormatMap(World, /*Radius=*/ 5);

	// Due unita' lontane e ferme: nessuna eliminazione, cosi' l'unica via che puo' chiudere e' la scadenza.
	SpawnFormatUnit(World, 0, FRTCellId(-4, 2));
	SpawnFormatUnit(World, 1, FRTCellId(4, -2));

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM)
	{
		DestroyFormatWorld(World);
		return false;
	}

	FRTMatchRules Rules;
	Rules.FormatId = FName(TEXT("Format.ShortTest"));
	Rules.RoundLimit = 3;
	TM->SetMatchRules(Rules);

	int32 RoundsPlayed = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && RoundsPlayed < 10)
	{
		PlayEmptyRound(TM);
		++RoundsPlayed;
	}

	TestTrue(TEXT("la partita e' finita"), TM->GetPhase() == ERTMatchPhase::MatchEnded);
	TestEqual(TEXT("al terzo round, non prima ne' dopo"), RoundsPlayed, 3);
	TestTrue(TEXT("per scadenza dei round"),
		TM->GetMatchResult().Reason == ERTMatchEndReason::RoundLimit);
	TestTrue(TEXT("e senza progresso e' un pareggio dichiarato"),
		TM->GetMatchResult().Outcome == ERTMatchOutcome::Draw);

	// La via che ha deciso la partita compare nel combat log, non solo nello stato interno.
	bool bLogged = false;
	for (const FString& Event : TM->GetRecentEvents())
	{
		bLogged = bLogged || (Event.Contains(TEXT("Partita finita")) && Event.Contains(TEXT("scadere dei round")));
	}
	TestTrue(TEXT("il log dice esito e via"), bLogged);

	DestroyFormatWorld(World);
	return true;
}

/**
 * La via dell'obiettivo in partita: `AddTeamScore` e' l'ingresso che CP 10.2 usera', e qui si verifica che
 * il punteggio arrivi davvero fino alla condizione di fine (altrimenti sarebbe l'ennesimo dato che nessuno
 * legge).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchObjectiveEndsPlayedMatchTest,
	"RefactorTactics.Match.ObjectiveEndsAPlayedMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchObjectiveEndsPlayedMatchTest::RunTest(const FString&)
{
	UWorld* World = MakeFormatWorld();
	if (!TestNotNull(TEXT("world di prova"), World))
	{
		return false;
	}
	SpawnFormatMap(World, /*Radius=*/ 5);
	SpawnFormatUnit(World, 0, FRTCellId(-4, 2));
	SpawnFormatUnit(World, 1, FRTCellId(4, -2));

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM)
	{
		DestroyFormatWorld(World);
		return false;
	}

	FRTMatchRules Rules;
	Rules.FormatId = FName(TEXT("Format.ObjectiveTest"));
	Rules.RoundLimit = 20; // lontano: deve chiudere l'obiettivo, non la scadenza
	Rules.ScoreToWin = 2;
	TM->SetMatchRules(Rules);

	PlayEmptyRound(TM);
	TestTrue(TEXT("sotto soglia la partita continua"), TM->GetPhase() != ERTMatchPhase::MatchEnded);

	TM->AddTeamScore(/*TeamId=*/ 0, /*Points=*/ 2);
	TestEqual(TEXT("il punteggio e' un intero leggibile"), TM->GetTeamScore(0), 2);

	PlayEmptyRound(TM);
	TestTrue(TEXT("raggiunta la soglia la partita finisce"), TM->GetPhase() == ERTMatchPhase::MatchEnded);
	TestTrue(TEXT("per obiettivo"), TM->GetMatchResult().Reason == ERTMatchEndReason::Objective);
	TestTrue(TEXT("e vince chi ha segnato"), TM->GetMatchResult().Outcome == ERTMatchOutcome::Team0Wins);

	DestroyFormatWorld(World);
	return true;
}

/**
 * Il `RoundLimit` arriva dal DATO e non da una costante: due formati diversi producono due limiti diversi
 * senza ricompilare nulla. E' il criterio che ha fatto nascere l'issue #185 — la costante `12` scritta in
 * `ARTTurnManager` e' esattamente cio' che la spec esclude.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchRoundLimitFromDataTest,
	"RefactorTactics.Match.RoundLimitComesFromData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchRoundLimitFromDataTest::RunTest(const FString&)
{
	UWorld* World = MakeFormatWorld();
	if (!TestNotNull(TEXT("world di prova"), World))
	{
		return false;
	}

	ARTHexMapActor* MapActor = SpawnFormatMap(World);
	ARTTurnManager* TurnManager = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	// Nessun formato applicato: nessun limite. Un TurnManager appena spawnato non inventa una durata.
	TestEqual(TEXT("senza formato non c'e' limite"), TurnManager->GetMatchRules().RoundLimit, 0);

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	GameMode->MatchFormat = MakeFormatAsset(/*RoundLimit=*/ 16, TEXT("Format.Standard3v3"));
	GameMode->SetupHexMatch(MapActor);

	TestEqual(TEXT("il limite arriva dall'asset"), TurnManager->GetMatchRules().RoundLimit, 16);
	TestEqual(TEXT("e con esso l'identita' del formato"),
		TurnManager->GetMatchRules().FormatId, FName(TEXT("Format.Standard3v3")));

	// Un secondo formato, un secondo limite: nessuna ricompilazione, solo un dato diverso.
	GameMode->MatchFormat = MakeFormatAsset(/*RoundLimit=*/ 10, TEXT("Format.Skirmish2v2"));
	GameMode->SetupHexMatch(MapActor);
	TestEqual(TEXT("altro formato, altro limite"), TurnManager->GetMatchRules().RoundLimit, 10);

	DestroyFormatWorld(World);
	return true;
}

/**
 * D1: senza formato la partita si avvia comunque — ma il ripiego deve essere **osservabile**, altrimenti i
 * numeri del playtest finiscono attribuiti a un formato fantasma. Qui il test *nota* il ripiego dal suo
 * effetto in partita, non da un log che nessuno legge.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchFormatFallbackObservableTest,
	"RefactorTactics.MatchFormat.FallbackIsObservable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchFormatFallbackObservableTest::RunTest(const FString&)
{
	UWorld* World = MakeFormatWorld();
	if (!TestNotNull(TEXT("world di prova"), World))
	{
		return false;
	}

	ARTHexMapActor* MapActor = SpawnFormatMap(World);
	ARTTurnManager* TurnManager = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	GameMode->MatchFormat = nullptr; // formato assente: e' il caso che D1 dichiara giocabile
	GameMode->SetupHexMatch(MapActor);

	// La partita c'e' (D1: non ci si rifiuta di partire)...
	TestTrue(TEXT("la partita si allestisce comunque"), CountUnits(World) > 0);

	// ...e dichiara di stare girando sul RIPIEGO, con un'identita' che nessun formato d'autore puo' usare.
	TestEqual(TEXT("il formato in vigore e' quello di ripiego"),
		TurnManager->GetMatchRules().FormatId, URTMatchFormatLibrary::FallbackFormatId);
	TestEqual(TEXT("con il RoundLimit del ripiego"), TurnManager->GetMatchRules().RoundLimit, 12);

	DestroyFormatWorld(World);
	return true;
}

/**
 * Il ripiego copre l'ASSENZA del formato, non un contenuto sbagliato: un formato presente e invalido non si
 * allestisce affatto. Sostituirlo in silenzio farebbe girare la partita con regole diverse da quelle scritte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchFormatInvalidBlocksSetupTest,
	"RefactorTactics.MatchFormat.InvalidFormatBlocksSetup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchFormatInvalidBlocksSetupTest::RunTest(const FString&)
{
	UWorld* World = MakeFormatWorld();
	if (!TestNotNull(TEXT("world di prova"), World))
	{
		return false;
	}

	ARTHexMapActor* MapActor = SpawnFormatMap(World);
	ARTTurnManager* TurnManager = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	GameMode->MatchFormat = MakeFormatAsset(/*RoundLimit=*/ 0); // limite non positivo: asset invalido

	// L'Error del GameMode e' atteso: senza dichiararlo il test fallirebbe per il log, non per la logica.
	AddExpectedError(TEXT("Formato di partita .* NON valido"), EAutomationExpectedErrorFlags::Contains, 1);
	GameMode->SetupHexMatch(MapActor);

	TestEqual(TEXT("nessuna unita' allestita"), CountUnits(World), 0);
	TestEqual(TEXT("e nessuna regola applicata"), TurnManager->GetMatchRules().RoundLimit, 0);

	DestroyFormatWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
