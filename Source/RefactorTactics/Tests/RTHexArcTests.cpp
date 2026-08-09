#include "Misc/AutomationTest.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexArcLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Pathfinding/RTHexPath.h"
#include "Pathfinding/RTHexPathLibrary.h"
#include "Terrain/RTTerrainLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Ponti e archi come oggetti di gioco (CP 9.4).
 *
 * L'arco e' ADDITIVO — crea un collegamento dove non c'era — e questo cambia cosa significa romperlo: le due
 * celle tornano irraggiungibili e il percorso FALLISCE, mentre una porta chiusa (CP 9.3) si aggira. Non c'e'
 * un'adiacenza planare di riserva fra due layer.
 */
namespace
{
	/** Due piani collegati: esagono r=2 sul layer 0, la sola colonna centrale sul layer 1, un ponte fra i due. */
	URTHexMapAsset* MakeArcMap(bool bWithBridge = true, ERTHexArcState State = ERTHexArcState::Active)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 2))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, 1)));
		M->AddOrUpdateCell(FRTHexCellData(FRTCellId(1, 0, 1)));
		M->SortCells();

		if (bWithBridge)
		{
			M->AddTransition(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), /*Cost*/ 1,
				ERTHexTransitionKind::Bridge, /*bBidirectional*/ true);
			if (State != ERTHexArcState::Active)
			{
				URTHexArcLibrary::SetArcState(M, FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), State);
			}
		}
		return M;
	}

	/** Vero se il grafo di traversata offre `To` fra i vicini di `From`. */
	bool ArcGraphHasStep(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To)
	{
		for (const TPair<FRTCellId, int32>& Step : URTHexPathLibrary::GraphNeighbors(Map, From))
		{
			if (Step.Key == To) { return true; }
		}
		return false;
	}

	FRTHexCombatUnit ArcUnit(int32 UnitId, const FRTCellId& Cell)
	{
		FRTHexCombatUnit U;
		U.UnitId = UnitId;
		U.TeamId = 0;
		U.Cell = Cell;
		U.bAlive = true;
		return U;
	}
}

/**
 * La DoD in una riga: togliendo il ponte i due layer tornano irraggiungibili e il percorso **fallisce**.
 * Non e' come una porta chiusa, che si aggira: fra due layer non esiste una via alternativa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBridgeRemovalTest,
	"RefactorTactics.Structures.Bridge.RemovalBreaksPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBridgeRemovalTest::RunTest(const FString&)
{
	const FRTCellId Ground(0, 0, 0);
	const FRTCellId Upper(0, 0, 1);

	URTHexMapAsset* Map = MakeArcMap();
	TestTrue(TEXT("col ponte attivo il grafo offre il passo"), ArcGraphHasStep(Map, Ground, Upper));
	const FRTHexPathResult Across = URTHexPathLibrary::FindPath(Map, Ground, Upper, /*MaxCost*/ 0);
	TestTrue(TEXT("e il percorso arriva"), Across.Status == ERTHexPathStatus::Success);

	// Disattivato: il collegamento non si percorre piu'.
	URTHexArcLibrary::SetArcState(Map, Ground, Upper, ERTHexArcState::Inactive);
	TestFalse(TEXT("ponte spento: nessun passo nel grafo"), ArcGraphHasStep(Map, Ground, Upper));
	const FRTHexPathResult Broken = URTHexPathLibrary::FindPath(Map, Ground, Upper, /*MaxCost*/ 0);
	TestTrue(TEXT("il percorso FALLISCE"), Broken.Status == ERTHexPathStatus::NoPath);
	TestEqual(TEXT("e non produce un salto"), Broken.Path.Num(), 0);

	// La cella oltre esiste ancora: e' il COLLEGAMENTO a mancare, non la destinazione.
	TestTrue(TEXT("la cella sul piano di sopra esiste ancora"), Map->ContainsCell(Upper));

	// Riattivato: il collegamento torna. `Inactive` e' reversibile, ed e' cio' che lo distingue da `Destroyed`.
	URTHexArcLibrary::SetArcState(Map, Ground, Upper, ERTHexArcState::Active);
	TestTrue(TEXT("riacceso: si passa di nuovo"), ArcGraphHasStep(Map, Ground, Upper));

	// Rimosso del tutto (`RemoveTransition`): stesso esito del disattivato, per chi cammina.
	URTHexMapAsset* Removed = MakeArcMap();
	Removed->RemoveTransition(Ground, Upper, /*bBothDirections*/ true);
	TestTrue(TEXT("ponte rimosso: il percorso fallisce lo stesso"),
		URTHexPathLibrary::FindPath(Removed, Ground, Upper, /*MaxCost*/ 0).Status == ERTHexPathStatus::NoPath);
	return true;
}

/**
 * Il ponte bidirezionale sono DUE archi ma un evento solo: la revisione sale una volta. E' la stessa regola
 * del portone di CP 9.3, e per la stessa ragione — chi osserva la revisione non deve vedere due cambi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBridgeRevisionTest,
	"RefactorTactics.Structures.Bridge.StateChangeBumpsRevision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBridgeRevisionTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeArcMap();
	const FRTCellId Ground(0, 0, 0);
	const FRTCellId Upper(0, 0, 1);
	TestEqual(TEXT("il ponte bidirezionale sono due archi"), Map->Transitions.Num(), 2);

	const int32 Before = Map->Revision;
	const TArray<FRTArcChange> Off = URTHexArcLibrary::SetArcState(Map, Ground, Upper, ERTHexArcState::Inactive);
	TestEqual(TEXT("entrambi i versi cambiano"), Off.Num(), 2);
	TestEqual(TEXT("ma una sola revisione"), Map->Revision, Before + 1);

	// Riapplicare lo stesso stato non e' un cambio: la revisione non si muove e nessuna voce esce.
	const int32 After = Map->Revision;
	TestEqual(TEXT("nessun cambio riportato"),
		URTHexArcLibrary::SetArcState(Map, Ground, Upper, ERTHexArcState::Inactive).Num(), 0);
	TestEqual(TEXT("la revisione resta ferma"), Map->Revision, After);

	// Anche comandandolo dal verso opposto: il ponte e' uno, non due oggetti.
	URTHexMapAsset* Reverse = MakeArcMap();
	TestEqual(TEXT("comandato dall'altro verso cambia comunque entrambi"),
		URTHexArcLibrary::SetArcState(Reverse, Upper, Ground, ERTHexArcState::Inactive).Num(), 2);
	return true;
}

/** Un ponte abbattuto non si riaccende: `Destroyed` e' terminale, come per le porte. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBridgeTerminalTest,
	"RefactorTactics.Structures.Bridge.DestroyedIsTerminal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBridgeTerminalTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeArcMap(/*bWithBridge*/ true, ERTHexArcState::Destroyed);
	const FRTCellId Ground(0, 0, 0);
	const FRTCellId Upper(0, 0, 1);

	const int32 Before = Map->Revision;
	TestEqual(TEXT("un ponte abbattuto non si riattiva"),
		URTHexArcLibrary::SetArcState(Map, Ground, Upper, ERTHexArcState::Active).Num(), 0);
	TestEqual(TEXT("e la revisione non si muove"), Map->Revision, Before);
	TestFalse(TEXT("il collegamento resta rotto"), ArcGraphHasStep(Map, Ground, Upper));
	return true;
}

/**
 * L'integrita' 40 si scala fino al crollo, e l'arco e' identificato dalla COPPIA di celle — la stessa
 * convenzione con cui `Action.ModifyArc` lo indica dal CP 8.5, perche' la pianificazione non ha un
 * bersaglio-arco.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBridgeDamageTest,
	"RefactorTactics.Structures.Bridge.DamageBreaksAtZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBridgeDamageTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeArcMap();
	const FRTCellId Ground(0, 0, 0);
	const FRTCellId Upper(0, 0, 1);

	TestEqual(TEXT("integrita' di catalogo"), FRTHexEdge::DefaultIntegrity, 40);

	const TArray<FRTArcChange> Dented = URTHexArcLibrary::DamageArc(Map, Ground, Upper, 25);
	// L'indice si convalida prima di usarlo: un test che sbaglia deve FALLIRE, non portarsi via i test dopo.
	if (TestEqual(TEXT("entrambi i versi incassano"), Dented.Num(), 2))
	{
		TestEqual(TEXT("integrita' residua"), Dented[0].RemainingIntegrity, 15);
		TestFalse(TEXT("il ponte regge"), Dented[0].bBroken);
	}
	TestTrue(TEXT("e si percorre ancora"), ArcGraphHasStep(Map, Ground, Upper));

	const TArray<FRTArcChange> Broken = URTHexArcLibrary::DamageArc(Map, Ground, Upper, 25);
	if (TestEqual(TEXT("il colpo che lo abbatte"), Broken.Num(), 2))
	{
		TestTrue(TEXT("crollato"), Broken[0].bBroken);
		TestEqual(TEXT("integrita' a zero, mai negativa"), Broken[0].RemainingIntegrity, 0);
	}
	TestFalse(TEXT("non si passa piu'"), ArcGraphHasStep(Map, Ground, Upper));

	// E resta abbattuto: un arco `Destroyed` non incassa altro danno e non torna indietro.
	TestEqual(TEXT("un ponte gia' crollato non incassa"),
		URTHexArcLibrary::DamageArc(Map, Ground, Upper, 25).Num(), 0);

	// Danno non positivo o arco inesistente: nessuna voce, nessuna revisione.
	URTHexMapAsset* Intact = MakeArcMap();
	const int32 Before = Intact->Revision;
	TestEqual(TEXT("danno zero non tocca nulla"),
		URTHexArcLibrary::DamageArc(Intact, Ground, Upper, 0).Num(), 0);
	TestEqual(TEXT("nessun arco fra celle non collegate"),
		URTHexArcLibrary::DamageArc(Intact, FRTCellId(1, 0, 0), FRTCellId(1, 0, 1), 10).Num(), 0);
	TestEqual(TEXT("revisione ferma"), Intact->Revision, Before);
	return true;
}

/**
 * Un ponte conduttivo e' un RISCHIO oltre che una scorciatoia: la scarica risale il collegamento e colpisce
 * chi sta sul piano di sopra. Senza questo, la propagazione resta planare — il BFS cammina sui sei vicini e
 * non sale mai di layer.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBridgeConductsTest,
	"RefactorTactics.Structures.Bridge.ConductsElectricity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBridgeConductsTest::RunTest(const FString&)
{
	const FRTCellId Ground(0, 0, 0);
	const FRTCellId Upper(0, 0, 1);

	// Le due celle agli estremi conducono; l'unita' 1 sta sopra.
	auto MakeChargedMap = [&](bool bConductiveArc)
	{
		URTHexMapAsset* M = MakeArcMap();
		FRTHexCellData Low = *M->FindCell(Ground);
		Low.Surface = ERTHexSurface::Conductive;
		FRTHexCellData High = *M->FindCell(Upper);
		High.Surface = ERTHexSurface::Conductive;
		M->AddOrUpdateCell(Low);
		M->AddOrUpdateCell(High);
		if (bConductiveArc)
		{
			for (FRTHexEdge& E : M->Transitions) { E.bConductsElectricity = true; }
		}
		return M;
	};

	TArray<FRTHexCombatUnit> Units;
	Units.Add(ArcUnit(0, Ground));
	Units.Add(ArcUnit(1, Upper));

	// Arco NON conduttivo: la scarica resta al piano terra.
	const TArray<FRTPropagationHit> Planar = URTTerrainLibrary::CollectElectricPropagation(
		MakeChargedMap(/*bConductiveArc*/ false), Ground, /*MaxSteps*/ 3, /*Initial*/ 20, /*Propagated*/ 10, Units);
	bool bUpperHitPlanar = false;
	for (const FRTPropagationHit& Hit : Planar) { if (Hit.UnitId == 1) { bUpperHitPlanar = true; } }
	TestFalse(TEXT("arco non conduttivo: la scarica non sale"), bUpperHitPlanar);

	// Arco conduttivo: risale di un passo e colpisce chi sta sopra.
	const TArray<FRTPropagationHit> Climbing = URTTerrainLibrary::CollectElectricPropagation(
		MakeChargedMap(/*bConductiveArc*/ true), Ground, /*MaxSteps*/ 3, /*Initial*/ 20, /*Propagated*/ 10, Units);
	int32 UpperHits = 0;
	for (const FRTPropagationHit& Hit : Climbing)
	{
		if (Hit.UnitId == 1)
		{
			++UpperHits;
			TestEqual(TEXT("raggiunta in un passo"), Hit.Steps, 1);
			TestEqual(TEXT("col danno propagato"), Hit.Damage, 10);
		}
	}
	TestEqual(TEXT("colpita una volta sola"), UpperHits, 1);

	// Un ponte SPENTO non conduce, per quanto sia dichiarato conduttivo: non c'e' collegamento.
	URTHexMapAsset* Off = MakeChargedMap(/*bConductiveArc*/ true);
	URTHexArcLibrary::SetArcState(Off, Ground, Upper, ERTHexArcState::Inactive);
	bool bHitThroughOffBridge = false;
	for (const FRTPropagationHit& Hit : URTTerrainLibrary::CollectElectricPropagation(
		Off, Ground, /*MaxSteps*/ 3, /*Initial*/ 20, /*Propagated*/ 10, Units))
	{
		if (Hit.UnitId == 1) { bHitThroughOffBridge = true; }
	}
	TestFalse(TEXT("un ponte spento non conduce"), bHitThroughOffBridge);
	return true;
}

/** L'hash e' cio' su cui si basa l'invalidazione: stato, integrita' e conduttivita' sono dato autorevole. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArcHashTest,
	"RefactorTactics.HexMap.ArcHashDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArcHashTest::RunTest(const FString&)
{
	URTHexMapAsset* Active = MakeArcMap();
	URTHexMapAsset* Off = MakeArcMap(/*bWithBridge*/ true, ERTHexArcState::Inactive);
	TestTrue(TEXT("lo stato cambia l'hash"), Active->ComputeHash() != Off->ComputeHash());

	URTHexMapAsset* Dented = MakeArcMap();
	URTHexArcLibrary::DamageArc(Dented, FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), 10);
	TestTrue(TEXT("l'integrita' cambia l'hash"), Dented->ComputeHash() != Active->ComputeHash());

	URTHexMapAsset* Conductive = MakeArcMap();
	for (FRTHexEdge& E : Conductive->Transitions) { E.bConductsElectricity = true; }
	TestTrue(TEXT("la conduttivita' cambia l'hash"), Conductive->ComputeHash() != Active->ComputeHash());

	// Due mappe uguali costruite allo stesso modo hashano uguale: l'ordine di inserimento non conta.
	TestEqual(TEXT("stesso contenuto, stesso hash"), MakeArcMap()->ComputeHash(), Active->ComputeHash());
	return true;
}

/** Gli archi si scrivono a mano nell'editor: qui si intercettano gli stati che nessuna regola sa risolvere. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArcValidationTest,
	"RefactorTactics.HexMap.ArcValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArcValidationTest::RunTest(const FString&)
{
	URTHexMapAsset* Clean = MakeArcMap();
	TestEqual(TEXT("un ponte ben posato non genera errori"), Clean->ValidateMap().Num(), 0);

	// Integrita' non positiva su un arco ancora in piedi: e' un ponte gia' crollato che non lo dichiara.
	URTHexMapAsset* Ghost = MakeArcMap();
	for (FRTHexEdge& E : Ghost->Transitions) { E.Integrity = 0; }
	TestTrue(TEXT("un ponte a zero punti struttura ancora attivo e' incoerente"),
		Ghost->ValidateMap().Num() > 0);
	return true;
}

/**
 * `FRTHexEdge` guadagna tre campi: la versione sale a 5 e una mappa scritta con la v4 deve sopravvivere senza
 * perdere niente. I default devono essere quelli di un ponte SANO — un arco letto da un asset vecchio non
 * puo' risultare spento, o la mappa cambierebbe significato caricandola.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArcMigrationTest,
	"RefactorTactics.HexMap.ArcFormatMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArcMigrationTest::RunTest(const FString&)
{
	URTHexMapAsset* Legacy = NewObject<URTHexMapAsset>();
	Legacy->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, 0)));
	Legacy->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, 1)));
	FRTHexCellData Sheltered(FRTCellId(1, 0, 0));
	Sheltered.Covers.Add(FRTHexCover(ERTHexDirection::W, ERTHexCoverType::Low, 30));
	Sheltered.Doors.Add(FRTHexDoor(ERTHexDirection::E, ERTHexDoorState::Closed));
	Legacy->AddOrUpdateCell(Sheltered);
	Legacy->AddTransition(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), 2, ERTHexTransitionKind::Stair);
	Legacy->FormatVersion = 4;

	Legacy->MigrateToCurrentFormat();

	TestEqual(TEXT("versione portata alla corrente"), Legacy->FormatVersion,
		URTHexMapAsset::CurrentFormatVersion);
	// Il numero e' pinnato di proposito: un bump di formato deve far cadere un test, non passare inosservato.
	// v6 (CP 19.1) aggiunge la classe di mappa; nessun dato precedente cambia significato.
	TestEqual(TEXT("la versione corrente e' la 6"), URTHexMapAsset::CurrentFormatVersion, 6);
	TestEqual(TEXT("nessuna cella persa"), Legacy->NumCells(), 3);
	TestEqual(TEXT("nessuna transizione persa"), Legacy->Transitions.Num(), 2);

	// I campi vecchi non si toccano.
	const FRTHexCellData* Kept = Legacy->FindCell(FRTCellId(1, 0, 0));
	TestTrue(TEXT("copertura e porta preservate"),
		Kept && Kept->Covers.Num() == 1 && Kept->Doors.Num() == 1);

	// I campi nuovi nascono come un ponte SANO: attivo, integro, non conduttivo.
	for (const FRTHexEdge& E : Legacy->Transitions)
	{
		TestTrue(TEXT("arco attivo"), E.State == ERTHexArcState::Active);
		TestEqual(TEXT("integrita' di catalogo"), E.Integrity, 40);
		TestFalse(TEXT("non conduttivo per difetto"), E.bConductsElectricity);
	}
	TestEqual(TEXT("mappa migrata valida"), Legacy->ValidateMap().Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
