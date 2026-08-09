#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTMatchStateHash.h"

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
		U.Id = TEXT("hero.a");
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

#endif // WITH_DEV_AUTOMATION_TESTS
