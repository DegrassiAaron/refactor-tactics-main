// CHI APRE IL TURNO 1, E QUANDO — `#2102`, [D-314].
//
// Misurato in due sessioni PIE indipendenti: `Turno 1 - pianificazione` precedeva `Board 2v2 esagonale
// avviata` di 348 ms. Il turno si apriva su una board non ancora allestita, il cronometro della
// pianificazione partiva prima che ci fosse qualcosa da guardare, e `PlanBots()` decideva su uno stato che
// si stava componendo.
//
// 🔑 **La decisione non e' «attendere sempre», ed e' il punto di questi test.** `BeginPlay` gira anche per
// i test headless e per lo `ScenarioHarness`, che un bootstrapper non ce l'hanno: un'attesa incondizionata
// li lascerebbe senza turno per sempre. Apre l'allestimento **se ha rivendicato**; chi non rivendica apre
// come prima. Ogni test qui sotto misura una delle due meta'.

#include "Misc/AutomationTest.h"

#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTCellId.h"
#include "Turn/RTPacing.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UWorld* MakeFirstTurnWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyFirstTurnWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Il TurnManager come lo spawna un test headless: nessuna rivendicazione, `BeginPlay` apre. */
	ARTTurnManager* SpawnUnclaimedTurnManager(UWorld* World)
	{
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (TM) { TM->DispatchBeginPlay(); }
		return TM;
	}

	/**
	 * Il TurnManager come lo spawna l'allestimento: rivendicato PRIMA di `BeginPlay`.
	 *
	 * ⚠️ `SpawnActorDeferred` e non `SpawnActor`, ed e' la stessa ragione per cui `ARTGameMode` lo fa:
	 * `SpawnActor` esegue `BeginPlay` prima di restituire, quindi la rivendicazione arriverebbe a turno
	 * gia' aperto — cioe' il test misurerebbe il caso che non vuole misurare.
	 */
	ARTTurnManager* SpawnClaimedTurnManager(UWorld* World)
	{
		ARTTurnManager* TM = World->SpawnActorDeferred<ARTTurnManager>(
			ARTTurnManager::StaticClass(), FTransform::Identity);
		if (TM)
		{
			TM->ClaimFirstTurnForMatchSetup();
			UGameplayStatics::FinishSpawningActor(TM, FTransform::Identity);
			TM->DispatchBeginPlay();
		}
		return TM;
	}

	/**
	 * Quante volte il turno 1 e' stato APERTO, contato sulle voci che l'apertura lascia.
	 *
	 * 🔴 **Non si contano i `PacingSamples`, e la prima stesura di questi test lo faceva sbagliando**:
	 * quell'array si popola quando un campione si **chiude** (`LockInAndResolve`), non quando si apre, ed
	 * era `0` sia con l'apertura sia senza. Un'asserzione «zero campioni» sul differimento sarebbe stata
	 * **vacua** — vera per la ragione sbagliata — cioe' proprio il difetto che questi test devono escludere.
	 * La voce di log invece la scrive `StartPlanningTimer`, e c'e' se e solo se il turno e' stato aperto.
	 */
	int32 CountFirstTurnOpenings(const ARTTurnManager* TM)
	{
		int32 Count = 0;
		for (const FString& Line : TM->GetRecentEvents())
		{
			if (Line.Contains(TEXT("Turno 1 - pianificazione")))
			{
				++Count;
			}
		}
		return Count;
	}

	ARTUnit* SpawnFirstTurnUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->DispatchBeginPlay();
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}
}

// ---------------------------------------------------------------------------------------------------------
// LA META' CHE PROTEGGE I TEST HEADLESS E L'HARNESS
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFirstTurnBeginPlayOpensTest,
	"RefactorTactics.FirstTurn.BeginPlayOpensWhenNobodyClaims",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFirstTurnBeginPlayOpensTest::RunTest(const FString&)
{
	// 🔑 **E' la meta' che #2102 rischiava di rompere**, e per questo viene prima: i 27 file di test e lo
	// `ScenarioHarness` spawnano un TurnManager a mano e non hanno un allestimento che apra per loro.
	// Un'attesa incondizionata li avrebbe fermati tutti.
	UWorld* World = MakeFirstTurnWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTTurnManager* TM = SpawnUnclaimedTurnManager(World);
	if (!TM) { DestroyFirstTurnWorld(World); return false; }

	TestFalse(TEXT("nessuno ha rivendicato l'apertura"), TM->IsFirstTurnClaimedBySetup());
	TestTrue(TEXT("BeginPlay ha aperto il turno 1, come prima di #2102"), TM->HasOpenedFirstTurn());
	TestEqual(TEXT("ed e' il turno 1"), TM->GetTurnNumber(), 1);
	TestTrue(TEXT("il timer di pianificazione e' armato"), TM->GetPlanningTimeRemaining() > 0.f);
	TestEqual(TEXT("e la voce di apertura c'e', una sola volta"), CountFirstTurnOpenings(TM), 1);

	DestroyFirstTurnWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// LA META' NUOVA: rivendicato, `BeginPlay` non apre
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFirstTurnClaimDefersTest,
	"RefactorTactics.FirstTurn.ClaimDefersOpeningPastBeginPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFirstTurnClaimDefersTest::RunTest(const FString&)
{
	UWorld* World = MakeFirstTurnWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTTurnManager* TM = SpawnClaimedTurnManager(World);
	if (!TM) { DestroyFirstTurnWorld(World); return false; }

	TestTrue(TEXT("la rivendicazione e' registrata"), TM->IsFirstTurnClaimedBySetup());
	TestFalse(TEXT("BeginPlay NON ha aperto il turno"), TM->HasOpenedFirstTurn());

	// Il timer non e' armato: e' l'esito osservabile del differimento, e non solo un flag che si guarda
	// allo specchio. Senza questa riga il test passerebbe con una guardia che mette il flag e apre lo stesso.
	TestEqual(TEXT("e il timer di pianificazione non e' armato"), TM->GetPlanningTimeRemaining(), 0.f);

	// E la voce che l'apertura lascia non c'e': e' la stessa riga che in PIE precedeva `Board 2v2
	// esagonale avviata` di 348 ms.
	TestEqual(TEXT("nessuna apertura del turno 1"), CountFirstTurnOpenings(TM), 0);

	DestroyFirstTurnWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// L'ORDINE — il test che sa fallire su di esso
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFirstTurnOrderTest,
	"RefactorTactics.FirstTurn.OrderIsBoardThenPlanning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFirstTurnOrderTest::RunTest(const FString&)
{
	// 🔑 **Il soggetto e' l'ORDINE di due eventi, mai una durata.** I 348 ms della misura originale
	// dipendono da quanto costa l'allestimento su quella macchina, e nessuno li pinna: un test che li
	// asserisse misurerebbe l'hardware. Qui si misura che l'apertura NON preceda la board.
	UWorld* World = MakeFirstTurnWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	// (1) L'allestimento rivendica e il TurnManager entra in scena. La board non c'e' ancora.
	ARTTurnManager* TM = SpawnClaimedTurnManager(World);
	if (!TM) { DestroyFirstTurnWorld(World); return false; }

	TArray<AActor*> UnitsBefore;
	UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), UnitsBefore);
	TestEqual(TEXT("premessa: a BeginPlay la board e' vuota"), UnitsBefore.Num(), 0);
	TestFalse(TEXT("e il turno NON e' aperto su una board vuota"), TM->HasOpenedFirstTurn());

	// (2) L'allestimento mette in campo le unita' — e' il `Board 2v2 esagonale avviata` della misura.
	SpawnFirstTurnUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(-2, 0));
	SpawnFirstTurnUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(2, 0));

	TArray<AActor*> UnitsAfter;
	UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), UnitsAfter);
	TestEqual(TEXT("la board e' allestita"), UnitsAfter.Num(), 2);
	TestFalse(TEXT("e il turno e' ANCORA chiuso: l'apertura non anticipa l'allestimento"),
		TM->HasOpenedFirstTurn());

	// (3) `ERTLoadPhase::Ready`: adesso, e non prima.
	TM->OpenFirstTurnAfterSetup();

	TestTrue(TEXT("il turno 1 si apre DOPO la board"), TM->HasOpenedFirstTurn());
	TestEqual(TEXT("ed e' il turno 1"), TM->GetTurnNumber(), 1);
	TestTrue(TEXT("con il timer armato"), TM->GetPlanningTimeRemaining() > 0.f);
	TestEqual(TEXT("e la voce di apertura arriva adesso, non 348 ms fa"), CountFirstTurnOpenings(TM), 1);

	DestroyFirstTurnWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// IDEMPOTENZA — il GameMode apre da piu' cammini
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFirstTurnIdempotentTest,
	"RefactorTactics.FirstTurn.OpeningIsIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFirstTurnIdempotentTest::RunTest(const FString&)
{
	// 🔑 **Non e' cortesia**: `ARTGameMode::BeginPlay` chiama l'apertura da tre cammini d'uscita — scenario
	// non caricabile, scenario avviato, allestimento normale — perche' un turno mai aperto e' peggio del
	// difetto d'ordine che #2102 chiude. Due cammini che si sovrapponessero aprirebbero due campioni di
	// pacing sullo stesso turno, e i campioni sono un dato su cui si asserisce.
	UWorld* World = MakeFirstTurnWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTTurnManager* TM = SpawnClaimedTurnManager(World);
	if (!TM) { DestroyFirstTurnWorld(World); return false; }

	TM->OpenFirstTurnAfterSetup();
	TM->OpenFirstTurnAfterSetup();
	TM->OpenFirstTurnAfterSetup();

	TestTrue(TEXT("il turno e' aperto"), TM->HasOpenedFirstTurn());
	TestEqual(TEXT("tre aperture producono UNA voce di apertura"), CountFirstTurnOpenings(TM), 1);
	TestEqual(TEXT("e il numero di turno non e' avanzato"), TM->GetTurnNumber(), 1);

	// E su un TurnManager NON rivendicato la funzione non fa nulla: il turno l'ha gia' aperto `BeginPlay`,
	// e riaprirlo qui riaprirebbe il cronometro di una pianificazione gia' in corso.
	ARTTurnManager* Unclaimed = SpawnUnclaimedTurnManager(World);
	if (Unclaimed)
	{
		const int32 Before = CountFirstTurnOpenings(Unclaimed);
		TestEqual(TEXT("premessa: BeginPlay l'aveva gia' aperto una volta"), Before, 1);
		Unclaimed->OpenFirstTurnAfterSetup();
		TestEqual(TEXT("su un TurnManager non rivendicato l'apertura non fa nulla"),
			CountFirstTurnOpenings(Unclaimed), Before);
	}

	DestroyFirstTurnWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// LA RIVENDICAZIONE TARDIVA — dichiarata, non silenziosa
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFirstTurnLateClaimTest,
	"RefactorTactics.FirstTurn.LateClaimLeavesTurnOpenAndSaysSo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFirstTurnLateClaimTest::RunTest(const FString&)
{
	// ⚠️ **Il limite dichiarato di #2102.** Se il TurnManager e' piazzato nel livello e il suo `BeginPlay`
	// precede quello del GameMode, la rivendicazione arriva a turno gia' aperto. Chiuderlo e riaprirlo
	// significherebbe richiudere il campione di pacing — cioe' toccare `MsToLockIn`, che il DoD chiede di
	// non muovere senza misura. Il comportamento resta quello di prima; questo test pinna che **resti
	// quello**, invece di diventare un turno chiuso per sempre.
	UWorld* World = MakeFirstTurnWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTTurnManager* TM = SpawnUnclaimedTurnManager(World);
	if (!TM) { DestroyFirstTurnWorld(World); return false; }
	TestTrue(TEXT("premessa: il turno e' gia' aperto"), TM->HasOpenedFirstTurn());

	// ⚠️ Nessun `AddExpectedError`: la dichiarazione e' un `Warning`, non un `Error`, e attenderlo qui
	// legherebbe il test alla configurazione con cui l'automation tratta i warning. Cio' che questo test
	// deve provare e' il **comportamento** — il turno resta aperto e la rivendicazione non viene registrata
	// — non che una riga di log esista con quel testo.
	TM->ClaimFirstTurnForMatchSetup();

	TestFalse(TEXT("la rivendicazione tardiva NON viene registrata"), TM->IsFirstTurnClaimedBySetup());
	TestTrue(TEXT("e il turno resta aperto: nessuna sessione morta"), TM->HasOpenedFirstTurn());
	TestEqual(TEXT("con la sua unica apertura"), CountFirstTurnOpenings(TM), 1);

	DestroyFirstTurnWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
