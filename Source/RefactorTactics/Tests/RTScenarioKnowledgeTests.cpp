#include "Misc/AutomationTest.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTCellId.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTPerceptionLibrary.h"
#include "ScenarioHarness/RTScenarioKnowledge.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * La conoscenza di squadra dello STATO INIZIALE di uno scenario, senza mondo e senza `ARTTurnManager`
 * (#1754).
 *
 * 🔑 **Cio' che questi test devono davvero prendere** non e' che `Observe` funzioni — quello e' gia' coperto
 * da `RefactorTactics.Vision.*` — ma che il Tactical Designer ne costruisca gli **ingressi** senza
 * inventarli: il raggio visivo viene dall'eroe, le squadre dal dato, e l'onniscienza passa dallo stesso
 * percorso invece di saltarlo.
 */

namespace
{
	URTHexMapAsset* MakeScenarioKnowledgeMap(int32 Radius)
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
	}

	FRTScenarioUnitView MakeUnit(const FString& Id, FName HeroId, int32 TeamId, const FRTCellId& Cell,
		ERTHexDirection Facing = ERTHexDirection::E)
	{
		FRTScenarioUnitView V;
		V.Id = Id;
		V.HeroId = HeroId;
		V.TeamId = TeamId;
		V.Cell = Cell;
		V.Facing = Facing;
		return V;
	}

	/** Il primo eroe del roster, per non cablare un nome che il roster potrebbe rinominare. */
	FName AnyHeroId()
	{
		const TArray<FName> Ids = URTHeroCatalogLibrary::GetHeroIds();
		return Ids.Num() > 0 ? Ids[0] : NAME_None;
	}

	bool ContainsUnitId(const TArray<FRTScenarioUnitView>& Units, const FString& Id)
	{
		for (const FRTScenarioUnitView& U : Units)
		{
			if (U.Id == Id)
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioKnowledgeTeamsComeFromDataTest,
	"RefactorTactics.Scenario.KnowledgeTeamsComeFromData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioKnowledgeTeamsComeFromDataTest::RunTest(const FString&)
{
	const FName Hero = AnyHeroId();

	// Il 2v2 di v0.1: due squadre, e il selettore ne mostrera' tre posizioni con `Omniscient`.
	const TArray<FRTScenarioUnitView> TwoTeams = {
		MakeUnit(TEXT("a"), Hero, 0, FRTCellId(0, 0, 0)),
		MakeUnit(TEXT("b"), Hero, 1, FRTCellId(3, 0, 0)),
		MakeUnit(TEXT("c"), Hero, 0, FRTCellId(1, 0, 0))
	};
	const TArray<int32> Teams = RTScenarioKnowledge::TeamIds(TwoTeams);
	TestEqual(TEXT("due squadre, non tre unita'"), Teams.Num(), 2);
	TestEqual(TEXT("crescenti"), Teams[0], 0);
	TestEqual(TEXT("crescenti"), Teams[1], 1);

	// ⚠️ Il caso che un `{0, 1}` cablato non distinguerebbe: qui la seconda squadra e' la 3, non la 1.
	const TArray<FRTScenarioUnitView> Sparse = {
		MakeUnit(TEXT("a"), Hero, 3, FRTCellId(0, 0, 0)),
		MakeUnit(TEXT("b"), Hero, 0, FRTCellId(2, 0, 0))
	};
	const TArray<int32> SparseTeams = RTScenarioKnowledge::TeamIds(Sparse);
	TestEqual(TEXT("le squadre sono quelle del dato"), SparseTeams.Num(), 2);
	TestEqual(TEXT("e portano i loro id, non 0 e 1"), SparseTeams[1], 3);

	// Una squadra sola: due posizioni nel selettore, non tre. Non e' un caso limite da tappare.
	const TArray<FRTScenarioUnitView> Lonely = { MakeUnit(TEXT("a"), Hero, 0, FRTCellId(0, 0, 0)) };
	TestEqual(TEXT("uno scenario a squadra sola ne dichiara una"),
		RTScenarioKnowledge::TeamIds(Lonely).Num(), 1);

	TestEqual(TEXT("nessuna unita', nessuna squadra"), RTScenarioKnowledge::TeamIds({}).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioKnowledgeVisionComesFromHeroTest,
	"RefactorTactics.Scenario.KnowledgeVisionComesFromHero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioKnowledgeVisionComesFromHeroTest::RunTest(const FString&)
{
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	if (!TestTrue(TEXT("premessa: il roster non e' vuoto"), Roster.Num() > 0))
	{
		return false;
	}

	const URTHeroData* First = Roster[0];
	const TArray<FRTScenarioUnitView> Units = {
		MakeUnit(TEXT("a"), First->HeroId, 0, FRTCellId(0, 0, 0))
	};

	const TArray<FRTPerceiver> Observers = RTScenarioKnowledge::Observers(Units, 0, Roster);
	TestEqual(TEXT("un'unita', un osservatore"), Observers.Num(), 1);
	TestEqual(TEXT("il raggio e' quello che l'eroe dichiara"),
		Observers[0].VisionRange, First->VisionRange);
	TestEqual(TEXT("e l'orientamento quello dell'unita'"),
		static_cast<uint8>(Observers[0].Facing), static_cast<uint8>(ERTHexDirection::E));

	// ⚠️ Fail-closed: un eroe che il roster non conosce non produce un osservatore con il raggio di default
	// della struct. Vedrebbe cinque celle che il gioco non gli concede.
	const TArray<FRTScenarioUnitView> Ghost = {
		MakeUnit(TEXT("a"), TEXT("Hero.NonEsiste"), 0, FRTCellId(0, 0, 0))
	};
	TestEqual(TEXT("un eroe ignoto non vede"),
		RTScenarioKnowledge::Observers(Ghost, 0, Roster).Num(), 0);

	// La squadra chiesta e' quella che si ottiene: gli avversari non entrano fra gli osservatori.
	const TArray<FRTScenarioUnitView> Mixed = {
		MakeUnit(TEXT("a"), First->HeroId, 0, FRTCellId(0, 0, 0)),
		MakeUnit(TEXT("b"), First->HeroId, 1, FRTCellId(3, 0, 0))
	};
	TestEqual(TEXT("solo gli osservatori della squadra chiesta"),
		RTScenarioKnowledge::Observers(Mixed, 0, Roster).Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioKnowledgeIsTeamUnionTest,
	"RefactorTactics.Scenario.KnowledgeIsTeamUnion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioKnowledgeIsTeamUnionTest::RunTest(const FString&)
{
	// I tre casi che #1754 chiede per nome: overlap centrale, terza unita' che non aggiunge celle, e
	// invarianza per permutazione. La vista di squadra e' l'UNIONE, non la somma di cerchi per unita'.
	URTHexMapAsset* Map = MakeScenarioKnowledgeMap(8);
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	const FName Hero = AnyHeroId();

	// Due unita' che coprono regioni diverse con un overlap: l'unione e' piu' grande di ciascuna.
	const TArray<FRTScenarioUnitView> Pair = {
		MakeUnit(TEXT("est"), Hero, 0, FRTCellId(0, 0, 0), ERTHexDirection::E),
		MakeUnit(TEXT("ovest"), Hero, 0, FRTCellId(0, 3, 0), ERTHexDirection::W)
	};
	const FRTTeamKnowledge Both = RTScenarioKnowledge::ForTeam(Map, Pair, 0, Roster);

	const TArray<FRTScenarioUnitView> OnlyEast = { Pair[0] };
	const FRTTeamKnowledge East = RTScenarioKnowledge::ForTeam(Map, OnlyEast, 0, Roster);

	TestTrue(TEXT("l'unione supera il singolo contributo"),
		Both.VisibleCells.Num() > East.VisibleCells.Num());
	for (const FRTCellId& Cell : East.VisibleCells)
	{
		TestTrue(TEXT("e non perde nulla di cio' che il primo vedeva"), Both.VisibleCells.Contains(Cell));
	}

	// Una terza unita' NELLA stessa cella della prima: non aggiunge celle, e il risultato non cambia.
	TArray<FRTScenarioUnitView> WithRedundant = Pair;
	WithRedundant.Add(MakeUnit(TEXT("doppione"), Hero, 0, FRTCellId(0, 0, 0), ERTHexDirection::E));
	const FRTTeamKnowledge Redundant = RTScenarioKnowledge::ForTeam(Map, WithRedundant, 0, Roster);
	TestEqual(TEXT("un osservatore che non aggiunge celle non cambia l'unione"),
		Redundant.VisibleCells.Num(), Both.VisibleCells.Num());

	// Permutare gli osservatori non cambia l'esito: chi guarda per primo non decide cosa la squadra sa.
	TArray<FRTScenarioUnitView> Swapped = { Pair[1], Pair[0] };
	const FRTTeamKnowledge Reversed = RTScenarioKnowledge::ForTeam(Map, Swapped, 0, Roster);
	TestEqual(TEXT("permutare gli osservatori non cambia il conteggio"),
		Reversed.VisibleCells.Num(), Both.VisibleCells.Num());
	for (int32 i = 0; i < Both.VisibleCells.Num(); ++i)
	{
		TestTrue(TEXT("ne' l'ordine dell'esito"), Reversed.VisibleCells[i] == Both.VisibleCells[i]);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioKnowledgeOmniscientIsNamedPositionTest,
	"RefactorTactics.Scenario.KnowledgeOmniscientIsNamedPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioKnowledgeOmniscientIsNamedPositionTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeScenarioKnowledgeMap(4);
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	const FName Hero = AnyHeroId();

	const TArray<FRTScenarioUnitView> Units = {
		MakeUnit(TEXT("mio"), Hero, 0, FRTCellId(0, 0, 0), ERTHexDirection::E),
		// Alle SPALLE dell'osservatore e oltre il canale ravvicinato: in `Team 0` non si conosce.
		MakeUnit(TEXT("ignoto"), Hero, 1, FRTCellId(-4, 0, 0), ERTHexDirection::E)
	};

	const FRTTeamKnowledge All = RTScenarioKnowledge::ForTeam(Map, Units,
		RTScenarioKnowledge::OmniscientTeamId, Roster);

	TestEqual(TEXT("Omniscient vede ogni cella della mappa"), All.VisibleCells.Num(), Map->Cells.Num());
	TestEqual(TEXT("e nessuna squadra la possiede"), All.TeamId, RTScenarioKnowledge::OmniscientTeamId);
	TestEqual(TEXT("la struttura e' quella canonica, leggibile dal velo"),
		All.Version, FRTTeamKnowledge::CurrentVersion);

	// Il punto della slice: in `Omniscient` la prospettiva del designer non si perde.
	const TArray<FRTScenarioUnitView> SeenByAll = RTScenarioKnowledge::VisibleUnits(Units, All);
	TestEqual(TEXT("in Omniscient si disegnano tutte le unita'"), SeenByAll.Num(), Units.Num());

	// E in `Team 0` no: l'unita' avversaria mai vista non arriva a chi disegna.
	const FRTTeamKnowledge Team0 = RTScenarioKnowledge::ForTeam(Map, Units, 0, Roster);
	const TArray<FRTScenarioUnitView> SeenByZero = RTScenarioKnowledge::VisibleUnits(Units, Team0);
	TestTrue(TEXT("la propria unita' si disegna sempre"), ContainsUnitId(SeenByZero, TEXT("mio")));
	TestFalse(TEXT("il nemico mai visto non arriva a chi disegna"),
		ContainsUnitId(SeenByZero, TEXT("ignoto")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioKnowledgeLeavesScenarioUntouchedTest,
	"RefactorTactics.Scenario.KnowledgeLeavesScenarioUntouched",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioKnowledgeLeavesScenarioUntouchedTest::RunTest(const FString&)
{
	// ⚠️ **L'invariante e' sul DRAFT, non sullo stato partita.** `URTMatchStateHashLibrary::HashMatchState`
	// costruisce i digest da `TArray<ARTUnit*>`, e fuori da PIE non esiste nessun `ARTUnit`: qui l'oggetto
	// che non deve cambiare e' cio' che lo scenario dichiara — l'arena e le unita' schierate.
	URTHexMapAsset* Map = MakeScenarioKnowledgeMap(5);
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	const FName Hero = AnyHeroId();

	TArray<FRTScenarioUnitView> Units = {
		MakeUnit(TEXT("a"), Hero, 0, FRTCellId(0, 0, 0)),
		MakeUnit(TEXT("b"), Hero, 1, FRTCellId(4, 0, 0))
	};

	const uint32 MapBefore = Map->ComputeHash();
	const int32 CellsBefore = Map->Cells.Num();
	const int32 UnitsBefore = Units.Num();
	const FRTCellId FirstCellBefore = Units[0].Cell;
	const int32 FirstTeamBefore = Units[0].TeamId;

	// Ogni prospettiva, in sequenza: e' il giro che il designer fa davvero col selettore.
	for (const int32 TeamId : { RTScenarioKnowledge::OmniscientTeamId, 0, 1, 0 })
	{
		const FRTTeamKnowledge K = RTScenarioKnowledge::ForTeam(Map, Units, TeamId, Roster);
		RTScenarioKnowledge::VisibleUnits(Units, K);
	}

	TestEqual(TEXT("l'arena non cambia: stesso hash"), Map->ComputeHash(), MapBefore);
	TestEqual(TEXT("ne' il numero di celle"), Map->Cells.Num(), CellsBefore);
	TestEqual(TEXT("le unita' schierate restano quelle"), Units.Num(), UnitsBefore);
	TestTrue(TEXT("nessuna si e' mossa"), Units[0].Cell == FirstCellBefore);
	TestEqual(TEXT("e nessuna ha cambiato squadra"), Units[0].TeamId, FirstTeamBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioKnowledgeMatchesCanonicalPerceptionTest,
	"RefactorTactics.Scenario.KnowledgeMatchesCanonicalPerception",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioKnowledgeMatchesCanonicalPerceptionTest::RunTest(const FString&)
{
	// 🔑 **Il criterio che #1754 dichiarava «verificabile per assenza in code review», reso eseguibile.**
	// Se il Tactical Designer si costruisse una query propria, i due insiemi divergerebbero al primo caso
	// non banale. Qui si confronta con `URTPerceptionLibrary::TeamVisibleCells`, che e' l'owner.
	URTHexMapAsset* Map = MakeScenarioKnowledgeMap(8);
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	const FName Hero = AnyHeroId();

	const TArray<FRTScenarioUnitView> Units = {
		MakeUnit(TEXT("a"), Hero, 0, FRTCellId(0, 0, 0), ERTHexDirection::E),
		MakeUnit(TEXT("b"), Hero, 0, FRTCellId(-2, 4, 0), ERTHexDirection::NW),
		MakeUnit(TEXT("nemico"), Hero, 1, FRTCellId(5, 0, 0), ERTHexDirection::W)
	};

	const FRTTeamKnowledge K = RTScenarioKnowledge::ForTeam(Map, Units, 0, Roster);
	const TArray<FRTCellId> Canonical = URTPerceptionLibrary::TeamVisibleCells(
		Map, RTScenarioKnowledge::Observers(Units, 0, Roster));

	TestEqual(TEXT("stesso numero di celle viste"), K.VisibleCells.Num(), Canonical.Num());
	for (int32 i = 0; i < Canonical.Num(); ++i)
	{
		TestTrue(TEXT("cella per cella, e nello stesso ordine"), K.VisibleCells[i] == Canonical[i]);
	}

	// Premessa: la vista non e' banale. Senza, l'uguaglianza sopra reggerebbe anche fra due insiemi vuoti.
	TestTrue(TEXT("premessa: la squadra vede qualcosa, e non tutto"),
		Canonical.Num() > 1 && Canonical.Num() < Map->Cells.Num());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
