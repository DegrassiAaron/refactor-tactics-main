#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTMatchStateHash.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "ScenarioHarness/RTTestResult.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il checksum di fine partita copre l'AMBIENTE, non solo le unità (CP 12.1, issue #81).
 *
 * Il DoD chiedeva che coprisse «stati, terreni modificati, strutture e progresso degli obiettivi (non solo le
 * posizioni)». Non era così: `RTScenarioSession` mescolava, per ogni unità, cella / HP / scudo / energia /
 * vivo-morto e nient'altro. Due partite che finivano con le stesse unità nelle stesse condizioni davano lo
 * stesso hash **anche se una lasciava il campo in fiamme e l'altra no** — e un corpus golden costruito su quel
 * checksum (CP 12.6, #178) sarebbe nato con quel punto cieco dentro.
 *
 * L'ordine è la parte delicata: gli stati vivono in `TMap`/`TSet`, la cui iterazione non è deterministica
 * (invariante #4). Entrano ordinati, e l'ultimo test di questo file è quello che lo dimostra.
 */
namespace
{
	/** Mappa minima: esagono di raggio 1, tutte celle `Floor`. */
	URTHexMapAsset* MakeStateHashMap()
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), /*Radius*/ 1))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}

	/** Una sola unità viva, senza stati: la base da cui ogni caso si discosta di UNA cosa sola. */
	TArray<FRTUnitStateDigest> BaseUnits()
	{
		FRTUnitStateDigest U;
		U.StableUnitId = 1;
		U.Cell = FRTCellId(0, 0);
		U.Health = 100;
		U.Shield = 0;
		U.Energy = 25;
		U.bAlive = true;
		return { U };
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTChecksumCoversEnvironmentTest,
	"RefactorTactics.Simulation.ChecksumCoversEnvironment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTChecksumCoversEnvironmentTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeStateHashMap();
	const TArray<FRTUnitStateDigest> Units = BaseUnits();
	const TArray<int32> NoScore = { 0, 0 };

	const uint32 Baseline = URTMatchStateHashLibrary::HashMatchState(Map, Units, NoScore);

	// Riferimento: lo stesso stato dà lo stesso hash. Senza, tutto il resto del test non significherebbe nulla.
	TestEqual(TEXT("stesso stato -> stesso hash"),
		URTMatchStateHashLibrary::HashMatchState(Map, Units, NoScore), Baseline);

	// 1. TERRENO MODIFICATO: la stessa cella prende fuoco. Le unità non cambiano di una virgola.
	{
		URTHexMapAsset* Burning = MakeStateHashMap();
		FRTHexCellData Cell = *Burning->FindCell(FRTCellId(1, 0));
		Cell.Surface = ERTHexSurface::Fire;
		Burning->AddOrUpdateCell(Cell);
		Burning->SortCells();

		TestNotEqual(TEXT("una cella in fiamme cambia il checksum"),
			URTMatchStateHashLibrary::HashMatchState(Burning, Units, NoScore), Baseline);
	}

	// 2. STRUTTURE: una copertura eretta su un bordo. È il caso di CP 9.5 — le coperture si erigono e si
	//    spostano in partita, quindi due finali con e senza riparo non sono lo stesso finale.
	{
		URTHexMapAsset* Covered = MakeStateHashMap();
		URTHexCoverLibrary::AddCover(Covered, FRTCellId(0, 0), ERTHexDirection::E, ERTHexCoverType::Low, 30);

		TestNotEqual(TEXT("una copertura eretta cambia il checksum"),
			URTMatchStateHashLibrary::HashMatchState(Covered, Units, NoScore), Baseline);
	}

	// 3. STATI TEMPORANEI sull'unità: stessa cella, stessi HP, ma una è rallentata.
	{
		TArray<FRTUnitStateDigest> Slowed = Units;
		Slowed[0].Statuses.Add(FName(TEXT("Status.Slow")));

		TestNotEqual(TEXT("uno stato temporaneo cambia il checksum"),
			URTMatchStateHashLibrary::HashMatchState(Map, Slowed, NoScore), Baseline);
	}

	// 4. PROGRESSO OBIETTIVI: nessuno si è mosso, ma una squadra ha segnato.
	{
		const TArray<int32> Scored = { 1, 0 };
		TestNotEqual(TEXT("il progresso obiettivo cambia il checksum"),
			URTMatchStateHashLibrary::HashMatchState(Map, Units, Scored), Baseline);
	}

	// 5. INVARIANTE #4: gli stati arrivano da `TMap`/`TSet`, la cui iterazione non è deterministica. Due
	//    esecuzioni identiche non devono dare hash diversi solo perché i tag sono stati enumerati in un altro
	//    ordine — è esattamente il difetto che un checksum dovrebbe scoprire, non introdurre.
	{
		TArray<FRTUnitStateDigest> OneOrder = Units;
		OneOrder[0].Statuses = { FName(TEXT("Status.Slow")), FName(TEXT("Status.Burning")) };

		TArray<FRTUnitStateDigest> OtherOrder = Units;
		OtherOrder[0].Statuses = { FName(TEXT("Status.Burning")), FName(TEXT("Status.Slow")) };

		TestEqual(TEXT("permutare gli stati non cambia il checksum"),
			URTMatchStateHashLibrary::HashMatchState(Map, OneOrder, NoScore),
			URTMatchStateHashLibrary::HashMatchState(Map, OtherOrder, NoScore));

		// E due stati DIVERSI restano distinguibili: l'ordinamento non deve degenerare in «tutti uguali».
		TArray<FRTUnitStateDigest> Different = Units;
		Different[0].Statuses = { FName(TEXT("Status.Burning")), FName(TEXT("Status.Rooted")) };
		TestNotEqual(TEXT("stati diversi -> hash diversi"),
			URTMatchStateHashLibrary::HashMatchState(Map, OneOrder, NoScore),
			URTMatchStateHashLibrary::HashMatchState(Map, Different, NoScore));
	}

	return true;
}

/**
 * L'identità che entra nel checksum è `StableUnitId`, e discrimina (`#490`).
 *
 * Era la chiave del file di scenario, una `FString`: l'harness scriveva `"F1"`, una partita vera non ha un
 * file da cui prenderla, e lo stesso identico stato finale dava due hash diversi. Il campo è passato a
 * `int32` perché il tipo è il posto in cui la decisione si fa rispettare.
 *
 * ⚠️ Il caso che conta è il **secondo**: se l'identità uscisse dall'hash (opzione C di `#490`), due unità che
 * si scambiano cella e salute diventerebbero indistinguibili — e sono stati di gioco diversi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTChecksumIdentityTest,
	"RefactorTactics.Simulation.ChecksumIdentityIsStableUnitId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTChecksumIdentityTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeStateHashMap();
	const TArray<int32> NoScore = { 0, 0 };

	FRTUnitStateDigest A;
	A.StableUnitId = 1;
	A.Cell = FRTCellId(0, 0);
	A.Health = 100;

	FRTUnitStateDigest B;
	B.StableUnitId = 2;
	B.Cell = FRTCellId(1, 0);
	B.Health = 40;

	const uint32 Baseline = URTMatchStateHashLibrary::HashMatchState(Map, { A, B }, NoScore);

	// 1. L'ORDINE dell'array non conta: `GetAllActorsOfClass` non è ordinato, e due esecuzioni della stessa
	//    partita non devono dare hash diversi solo per l'ordine in cui il mondo ha restituito gli Actor.
	TestEqual(TEXT("permutare le unita' non cambia il checksum"),
		URTMatchStateHashLibrary::HashMatchState(Map, { B, A }, NoScore), Baseline);

	// 2. L'identità DISCRIMINA: A e B si scambiano cella e salute. Nessun altro campo cambia, la somma degli
	//    stati è identica, e i due finali devono restare distinguibili.
	FRTUnitStateDigest SwappedA = A;
	FRTUnitStateDigest SwappedB = B;
	SwappedA.Cell = B.Cell;
	SwappedA.Health = B.Health;
	SwappedB.Cell = A.Cell;
	SwappedB.Health = A.Health;

	TestNotEqual(TEXT("scambiare cella e salute fra due unita' cambia il checksum"),
		URTMatchStateHashLibrary::HashMatchState(Map, { SwappedA, SwappedB }, NoScore), Baseline);

	// 3. Due identità diverse non collidono: cambiare SOLO l'id cambia l'hash.
	FRTUnitStateDigest Renamed = B;
	Renamed.StableUnitId = 3;
	TestNotEqual(TEXT("un'identita' diversa cambia il checksum"),
		URTMatchStateHashLibrary::HashMatchState(Map, { A, Renamed }, NoScore), Baseline);

	return true;
}

/**
 * Il checksum della PARTITA GIOCATA vede la mappa (CP 12.1, issue #81) — copertura del CABLAGGIO.
 *
 * `ChecksumCoversEnvironment` prova la libreria; questo prova che la sessione gliela passi davvero. La
 * distinzione non è accademica: con il cablaggio mutato in `HashMatchState(nullptr, ...)` la suite restava
 * **interamente verde**. È il difetto ricorrente di questo repository — libreria coperta, cablaggio scoperto.
 *
 * `Action.CreateCover` è l'azione giusta per dimostrarlo perché cambia la MAPPA lasciando identiche le
 * UNITÀ: nessuno si muove, nessuno perde vita. Se l'hash ignorasse la mappa, i due finali sarebbero
 * indistinguibili — ed è esattamente ciò che succedeva.
 */
namespace
{
	UWorld* MakeWiringWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyWiringWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Un turno, due unità ferme. Con `bErectCover` Bastion erige un pannello sul proprio bordo E. */
	FRTTestScenario MakeCoverWiringScenario(bool bErectCover)
	{
		FRTTestScenario S;
		S.ScenarioId = TEXT("Internal.ChecksumMapWiring");
		S.MapRadius = 3;

		FRTScenarioUnit Bastion;
		Bastion.Id = TEXT("B1");
		Bastion.HeroId = FName(TEXT("Hero.Bastion"));
		Bastion.TeamId = 0;
		Bastion.Cell = FRTCellId(0, 0);
		S.Units.Add(Bastion);

		// Un avversario lontano e inerte: serve solo perché la partita non finisca per eliminazione prima di
		// arrivare al digest.
		FRTScenarioUnit Foe;
		Foe.Id = TEXT("V1");
		Foe.HeroId = FName(TEXT("Hero.Vektor"));
		Foe.TeamId = 1;
		Foe.Cell = FRTCellId(3, 0);
		S.Units.Add(Foe);

		FRTScenarioTurn Turn;
		if (bErectCover)
		{
			FRTScenarioIntent Erect;
			Erect.UnitId = TEXT("B1");
			Erect.Ability = FName(TEXT("Bastion.KineticPanel"));
			Erect.TargetCell = FRTCellId(0, 0);
			Erect.bTargetsCell = true;
			Erect.CoverEdge = ERTHexDirection::E;
			Erect.bHasCoverEdge = true;
			Turn.Intents.Add(Erect);
			Turn.Requires.Add(TEXT("CreateCover"));
		}
		S.Turns.Add(Turn);

		// L'harness RIFIUTA uno scenario senza assertion — «passerebbe sempre» — ed e' una guardia giusta.
		// Questa e' vera in entrambe le varianti: nessuno si muove, ed e' esattamente il punto del test. Se un
		// giorno smettesse di esserlo, i due hash differirebbero per le UNITA' e non per la mappa, e il test
		// direbbe di aver verificato una cosa che non ha verificato.
		FRTTestExpectation Still;
		Still.Kind = ERTAssertionKind::UnitAtCell;
		Still.UnitId = TEXT("B1");
		Still.Cell = FRTCellId(0, 0);
		S.Expect.Add(Still);

		FRTTestExpectation FoeStill;
		FoeStill.Kind = ERTAssertionKind::UnitAtCell;
		FoeStill.UnitId = TEXT("V1");
		FoeStill.Cell = FRTCellId(3, 0);
		S.Expect.Add(FoeStill);

		return S;
	}

	uint32 RunAndHash(FAutomationTestBase& Test, bool bErectCover, bool& bOutOk)
	{
		bOutOk = false;
		UWorld* World = MakeWiringWorld();
		if (!World)
		{
			return 0;
		}
		const FRTTestResult Result = URTScenarioRunner::Run(World, MakeCoverWiringScenario(bErectCover));
		DestroyWiringWorld(World);

		// Uno scenario BLOCKED o in errore darebbe due hash uguali per il motivo sbagliato, e il test
		// passerebbe da verde a verde senza aver verificato nulla.
		if (Result.Outcome == ERTTestOutcome::Error || Result.Outcome == ERTTestOutcome::Blocked)
		{
			Test.AddError(FString::Printf(TEXT("scenario non eseguibile (%s): %s%s"),
				*Result.OutcomeString(), *Result.ErrorMessage, *Result.BlockedReason));
			return 0;
		}
		bOutOk = true;
		return Result.StateHash;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTChecksumSeesMapWiringTest,
	"RefactorTactics.Simulation.ChecksumSeesMapInPlayedScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTChecksumSeesMapWiringTest::RunTest(const FString&)
{
	bool bPlainOk = false;
	bool bCoverOk = false;

	const uint32 Plain = RunAndHash(*this, /*bErectCover*/ false, bPlainOk);
	const uint32 WithCover = RunAndHash(*this, /*bErectCover*/ true, bCoverOk);

	if (!TestTrue(TEXT("entrambe le partite sono eseguibili"), bPlainOk && bCoverOk))
	{
		return false;
	}

	// Le unità finiscono identiche in entrambe: l'unica differenza è il pannello sulla mappa. Se il digest
	// non ricevesse la mappa, i due hash coinciderebbero.
	TestNotEqual(TEXT("una copertura eretta in partita cambia il checksum"), WithCover, Plain);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
