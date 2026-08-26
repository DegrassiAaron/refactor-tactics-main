#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "RTGameMode.h"
#include "Core/RTTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTMatchFormatData.h"
#include "Turn/RTMatchFormatLibrary.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"

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
		URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = Map;
		return Actor;
	}

	URTMatchFormatData* MakeFormatAsset(int32 RoundLimit, const TCHAR* Id = TEXT("Format.WorldTest"))
	{
		URTMatchFormatData* Format = NewObject<URTMatchFormatData>();
		Format->FormatId = FName(Id);
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
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeRiktor());
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
	// ...e ANCHE nessun formato spedito: dal 2026-08-10 (#375) l'assenza dell'asset non basta piu' a far
	// ripiegare, perche' il GameMode risolve `Format.Skirmish2v2` dal catalogo C++. Il ripiego copre ora
	// l'id SCONOSCIUTO — che e' il caso che resta davvero scoperto, ed e' quello che questo test misura.
	GameMode->ShippedFormatId = FName(TEXT("Format.NonEsiste"));
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
 * Senza asset assegnato si gioca il formato SPEDITO, non il ripiego (#375). E' la meta' che mancava: CP 12.5
 * ha misurato una build pacchettizzata che girava su `Format.Fallback` proprio perche' nessuno aveva creato
 * un `.uasset` in editor, e il formato canonico della v0.1 non puo' dipendere da quel passo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchFormatShippedInMatchTest,
	"RefactorTactics.MatchFormat.ShippedFormatPlaysWithoutAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchFormatShippedInMatchTest::RunTest(const FString&)
{
	UWorld* World = MakeFormatWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTHexMapActor* MapActor = SpawnFormatMap(World);
	ARTTurnManager* TurnManager = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	GameMode->MatchFormat = nullptr; // nessun asset: e' il caso normale del repository, che non ne contiene
	GameMode->SetupHexMatch(MapActor);

	TestTrue(TEXT("la partita si allestisce"), CountUnits(World) > 0);

	const FRTMatchRules& Rules = TurnManager->GetMatchRules();
	TestEqual(TEXT("gioca il formato SPEDITO, non il ripiego"),
		Rules.FormatId, URTMatchFormatLibrary::Skirmish2v2FormatId);
	TestNotEqual(TEXT("e l'identita' NON e' quella riservata al ripiego"),
		Rules.FormatId, URTMatchFormatLibrary::FallbackFormatId);
	// I numeri del catalogo: se qualcuno li cambia, questo test lo dice. Ha gia' fatto il suo mestiere una
	// volta — era `RoundLimit 5` fino al 2026-08-10, ed e' caduto quando il valore e' salito a 12 per
	// allinearsi a D-010 (10-14 in 2v2).
	// ⚠️ Da qui in poi il RoundLimit **non distingue piu'** questo formato dal ripiego, che vale 12 anche lui:
	// a separarli resta la sola identita', verificata dalle due asserzioni qui sopra. Non e' una perdita di
	// copertura — il numero non era un discriminante affidabile — ma chi legge deve saperlo.
	TestEqual(TEXT("RoundLimit 12, allineato a D-010"), Rules.RoundLimit, 12);
	TestEqual(TEXT("due unita' per squadra"), Rules.UnitsPerTeam, 2);

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

/**
 * CP 19.2 — la composizione arriva dal FORMATO, e il `GameMode` la onora invece di dichiararla per conto
 * proprio. Finche' il `2` viveva in `SetupHexMatch`, «2v2» era una proprieta' del codice di allestimento: lo
 * stress 4v4 di E17 doveva essere un ramo del `GameMode` invece di un formato che dichiara 4.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchFormatGameModeHonoursCompositionTest,
	"RefactorTactics.MatchFormat.GameModeHonoursComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchFormatGameModeHonoursCompositionTest::RunTest(const FString&)
{
	UWorld* World = MakeFormatWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTHexMapActor* MapActor = SpawnFormatMap(World, /*Radius=*/ 4);
	ARTTurnManager* TurnManager = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();

	// 1. Il formato del vertical slice: 2 per squadra, formazioni da due eroi. Si allestisce.
	URTMatchFormatData* Skirmish = MakeFormatAsset(/*RoundLimit=*/ 12, TEXT("Format.Skirmish2v2"));
	Skirmish->UnitsPerTeam = 2;
	GameMode->MatchFormat = Skirmish;
	GameMode->SetupHexMatch(MapActor);

	TestEqual(TEXT("la composizione dichiarata raggiunge le regole in vigore"),
		TurnManager->GetMatchRules().UnitsPerTeam, 2);
	TestEqual(TEXT("e in campo ci sono due squadre da due"), CountUnits(World), 4);

	DestroyFormatWorld(World);

	// 2. Un formato che ne schiera TRE con formazioni da due: il `GameMode` rifiuta invece di allestire una
	//    partita che contraddice il proprio formato. E' la stessa disciplina del formato invalido.
	UWorld* World2 = MakeFormatWorld();
	if (!TestNotNull(TEXT("secondo world"), World2)) { return false; }

	ARTHexMapActor* MapActor2 = SpawnFormatMap(World2, /*Radius=*/ 4);
	World2->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode2 = World2->SpawnActor<ARTGameMode>();

	URTMatchFormatData* Three = MakeFormatAsset(/*RoundLimit=*/ 12, TEXT("Format.Test3v3"));
	Three->UnitsPerTeam = 3;
	GameMode2->MatchFormat = Three;

	AddExpectedError(TEXT("schiera 3 unita' per squadra"), EAutomationExpectedErrorFlags::Contains, 1);
	GameMode2->SetupHexMatch(MapActor2);

	TestEqual(TEXT("formazione e formato in disaccordo: nessuna unita' in campo"), CountUnits(World2), 0);

	DestroyFormatWorld(World2);
	return true;
}

/**
 * CP 19.1 — l'accoppiata formato/mappa si verifica all'allestimento. Un formato disegnato per una classe di
 * mappa diversa non e' una partita piu' stretta: e' una partita sbagliata, e va rifiutata prima, non scoperta
 * al terzo turno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapClassFormatAndMapAgreeTest,
	"RefactorTactics.MapClass.FormatAndMapAgree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapClassFormatAndMapAgreeTest::RunTest(const FString&)
{
	UWorld* World = MakeFormatWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTHexMapActor* MapActor = SpawnFormatMap(World, /*Radius=*/ 4);
	ARTTurnManager* TurnManager = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();

	// La mappa e' Skirmish (default, ed e' cio' che una mappa del vertical slice e').
	TestTrue(TEXT("la mappa di prova e' Skirmish"), MapActor->MapAsset->MapClass == ERTMapClass::Skirmish);

	// Un formato Standard sopra: la validazione pura lo rifiuta...
	FRTMatchRules StandardRules;
	StandardRules.FormatId = FName(TEXT("Format.Standard3v3"));
	StandardRules.RoundLimit = 14;
	StandardRules.UnitsPerTeam = 3;
	StandardRules.MapClass = ERTMapClass::Standard;

	const TArray<FString> Errors = URTMatchFormatLibrary::ValidateAgainstMap(StandardRules, MapActor->MapAsset);
	TestEqual(TEXT("l'accoppiata sbagliata e' un errore, non un avvio silenzioso"), Errors.Num(), 1);
	if (Errors.Num() == 1)
	{
		// Il messaggio deve nominare ENTRAMBE le classi: chi legge deve sapere cosa cambiare.
		TestTrue(TEXT("e l'errore nomina la classe richiesta"), Errors[0].Contains(TEXT("Standard")));
		TestTrue(TEXT("e quella trovata"), Errors[0].Contains(TEXT("Skirmish")));
	}

	// ...e l'allestimento non parte.
	URTMatchFormatData* Standard = MakeFormatAsset(/*RoundLimit=*/ 14, TEXT("Format.Standard3v3"));
	Standard->UnitsPerTeam = 3;
	Standard->MapClass = ERTMapClass::Standard;
	GameMode->MatchFormat = Standard;

	AddExpectedError(TEXT("Formato e mappa non combaciano"), EAutomationExpectedErrorFlags::Contains, 1);
	GameMode->SetupHexMatch(MapActor);

	TestEqual(TEXT("nessuna unita' allestita"), CountUnits(World), 0);
	TestEqual(TEXT("e nessuna regola applicata"), TurnManager->GetMatchRules().RoundLimit, 0);

	// La stessa mappa con il formato della sua classe si allestisce: il rifiuto e' dell'accoppiata, non della
	// mappa. Senza questa meta' il test sarebbe verde anche se il validator rifiutasse tutto.
	URTMatchFormatData* Skirmish = MakeFormatAsset(/*RoundLimit=*/ 12, TEXT("Format.Skirmish2v2"));
	Skirmish->UnitsPerTeam = 2;
	Skirmish->MapClass = ERTMapClass::Skirmish;
	GameMode->MatchFormat = Skirmish;
	GameMode->SetupHexMatch(MapActor);

	TestTrue(TEXT("la stessa mappa col formato giusto si allestisce"), CountUnits(World) > 0);

	DestroyFormatWorld(World);
	return true;
}

/**
 * CP 19.1 — **la simulazione non ramifica sulla classe**. Non e' una proprieta' che si legge dal codice: si
 * misura eseguendo due partite identiche che differiscono per la sola classe, e confrontando la traccia.
 *
 * Se un giorno qualcuno scrivesse `if (MapClass == Operations)` nel resolver, questo test cadrebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapClassNotBranchedInSimulationTest,
	"RefactorTactics.MapClass.NotBranchedInSimulation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapClassNotBranchedInSimulationTest::RunTest(const FString&)
{
	auto PlayAndHash = [](ERTMapClass Class, uint32& OutLogHash, int32& OutRounds) -> UWorld*
	{
		UWorld* W = MakeFormatWorld();
		if (!W) { return nullptr; }

		ARTHexMapActor* MapActor = SpawnFormatMap(W, /*Radius=*/ 5);
		MapActor->MapAsset->MapClass = Class;

		SpawnFormatUnit(W, 0, FRTCellId(-4, 2));
		SpawnFormatUnit(W, 1, FRTCellId(4, -2));

		ARTTurnManager* TM = W->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		FRTMatchRules Rules;
		Rules.FormatId = FName(TEXT("Format.BranchTest"));
		Rules.RoundLimit = 3;
		Rules.UnitsPerTeam = 2;
		Rules.MapClass = Class;
		TM->SetMatchRules(Rules);

		OutRounds = 0;
		while (TM->GetPhase() != ERTMatchPhase::MatchEnded && OutRounds < 10)
		{
			PlayEmptyRound(TM);
			++OutRounds;
		}

		OutLogHash = URTTurnLogLibrary::HashTurnLog(TM->GetTurnLog());
		return W;
	};

	uint32 HashSkirmish = 0, HashOperations = 0;
	int32 RoundsSkirmish = 0, RoundsOperations = 0;

	UWorld* A = PlayAndHash(ERTMapClass::Skirmish, HashSkirmish, RoundsSkirmish);
	if (!TestNotNull(TEXT("prima partita"), A)) { return false; }
	DestroyFormatWorld(A);

	UWorld* B = PlayAndHash(ERTMapClass::Operations, HashOperations, RoundsOperations);
	if (!TestNotNull(TEXT("seconda partita"), B)) { return false; }
	DestroyFormatWorld(B);

	TestEqual(TEXT("stessa durata"), RoundsOperations, RoundsSkirmish);
	TestEqual(TEXT("e la stessa traccia: la classe non entra in una sola decisione del turno"),
		HashOperations, HashSkirmish);

	return true;
}

/**
 * CP 19.3 (**D-155**) — **il resolver e' invariante al conteggio di controllo**, e non e' una proprieta' che si
 * legge dal codice: si misura giocando due partite identiche che differiscono per il solo `UnitsPerPlayer` e
 * confrontando la traccia.
 *
 * E' la stessa forma di `MapClass.NotBranchedInSimulation`, e per la stessa ragione. Quel campo dice *dove* si
 * gioca, questo dice *chi comanda cosa*: nessuno dei due dice *cosa succede*, e il giorno in cui qualcuno
 * scrivesse `if (Rules.UnitsPerPlayer == 1)` dentro la risoluzione, questo test cadrebbe.
 *
 * ⚠️ **Il test e' un TRIPWIRE, non un controllo vivo, e va detto perche' il verde non lo dice.** Nessun
 * percorso di risoluzione legge `UnitsPerPlayer` — verificato: la verifica di mutazione che ha tolto la
 * propagazione da `ResolveRules` ha lasciato questo test VERDE. Asserisce quindi «X non cambia Y» per una X
 * che oggi non fa niente. Il suo valore e' futuro: cade il giorno in cui qualcuno scrivesse
 * `if (Rules.UnitsPerPlayer == 1)` dentro la risoluzione.
 *
 * ⚠️ `UnitsPerTeam` resta **2** in entrambe le partite per simmetria con la forma di `MapClass`, non perche'
 * cambiarlo sposterebbe qualcosa: questo test **non passa dallo spawner**, mette in campo una unita' per
 * squadra con `SpawnFormatUnit`, e `Rules.UnitsPerTeam` lo consuma solo `ARTGameMode`, che qui non
 * interviene. 🔴 Il commento precedente diceva che cambiarlo avrebbe messo «due unita' in campo contro
 * quattro»: falso, e trovato in code review — un lettore ci avrebbe creduto che la struct delle regole
 * governa la composizione anche nei test di mondo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchFormatResolverInvariantToControlCountTest,
	"RefactorTactics.MatchFormat.ResolverIsInvariantToControlCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchFormatResolverInvariantToControlCountTest::RunTest(const FString&)
{
	auto PlayAndHash = [](int32 UnitsPerPlayer, uint32& OutLogHash, int32& OutRounds) -> UWorld*
	{
		UWorld* W = MakeFormatWorld();
		if (!W) { return nullptr; }

		SpawnFormatMap(W, /*Radius=*/ 5);

		SpawnFormatUnit(W, 0, FRTCellId(-4, 2));
		SpawnFormatUnit(W, 1, FRTCellId(4, -2));

		ARTTurnManager* TM = W->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		FRTMatchRules Rules;
		Rules.FormatId = FName(TEXT("Format.ControlCountTest"));
		Rules.RoundLimit = 3;
		Rules.UnitsPerTeam = 2;
		Rules.UnitsPerPlayer = UnitsPerPlayer;
		TM->SetMatchRules(Rules);

		OutRounds = 0;
		while (TM->GetPhase() != ERTMatchPhase::MatchEnded && OutRounds < 10)
		{
			PlayEmptyRound(TM);
			++OutRounds;
		}

		OutLogHash = URTTurnLogLibrary::HashTurnLog(TM->GetTurnLog());
		return W;
	};

	uint32 HashWholeTeam = 0, HashOnePerPerson = 0;
	int32 RoundsWholeTeam = 0, RoundsOnePerPerson = 0;

	// Una persona comanda entrambe le unita': e' la v0.1.
	UWorld* A = PlayAndHash(/*UnitsPerPlayer=*/ 2, HashWholeTeam, RoundsWholeTeam);
	if (!TestNotNull(TEXT("prima partita"), A)) { return false; }
	DestroyFormatWorld(A);

	// Due persone, una unita' a testa: il formato competitivo ipotizzato.
	UWorld* B = PlayAndHash(/*UnitsPerPlayer=*/ 1, HashOnePerPerson, RoundsOnePerPerson);
	if (!TestNotNull(TEXT("seconda partita"), B)) { return false; }
	DestroyFormatWorld(B);

	TestEqual(TEXT("stessa durata"), RoundsOnePerPerson, RoundsWholeTeam);
	TestEqual(TEXT("e la stessa traccia: chi comanda non entra in una sola decisione del turno"),
		HashOnePerPerson, HashWholeTeam);

	return true;
}

/**
 * CP 19.1 — la classe del vertical slice e' `Skirmish`, e lo e' **per default**: una mappa scritta prima che
 * il campo esistesse (`FormatVersion` 5) non deve cambiare significato solo per essere stata ricaricata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapClassSliceIsSkirmishTest,
	"RefactorTactics.MapClass.SliceIsSkirmish",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapClassSliceIsSkirmishTest::RunTest(const FString&)
{
	UWorld* World = MakeFormatWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	// Una mappa appena creata: il default e' la classe del vertical slice.
	ARTHexMapActor* MapActor = SpawnFormatMap(World, /*Radius=*/ 2);
	TestTrue(TEXT("una mappa nuova nasce Skirmish"), MapActor->MapAsset->MapClass == ERTMapClass::Skirmish);

	// Verifica a due binari, la meta' che un test in memoria puo' davvero fare: una mappa scritta con il
	// formato PRECEDENTE (v5, senza il campo) migra a v6 senza che la classe cambi da sotto — il default e'
	// cio' che quelle mappe gia' erano, non un valore nuovo che si porta dietro un significato nuovo.
	URTHexMapAsset* Legacy = NewObject<URTHexMapAsset>();
	Legacy->FormatVersion = 5;
	Legacy->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, 0)));
	Legacy->SortCells();
	const int32 CellsBefore = Legacy->NumCells();

	Legacy->MigrateToCurrentFormat();

	TestEqual(TEXT("la migrazione porta la versione avanti"),
		Legacy->FormatVersion, URTHexMapAsset::CurrentFormatVersion);
	TestTrue(TEXT("e la classe di una mappa vecchia resta Skirmish"), Legacy->MapClass == ERTMapClass::Skirmish);
	TestEqual(TEXT("senza toccare i campi che c'erano gia'"), Legacy->NumCells(), CellsBefore);

	// Il formato di RIPIEGO dichiara la stessa classe: la partita che si avvia senza formato e' quella del
	// vertical slice, non una partita di classe indefinita.
	const FRTMatchRules Fallback = URTMatchFormatLibrary::MakeFallbackRules();
	TestTrue(TEXT("il ripiego e' Skirmish"), Fallback.MapClass == ERTMapClass::Skirmish);
	TestEqual(TEXT("e schiera due unita' per squadra"), Fallback.UnitsPerTeam, 2);

	DestroyFormatWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
