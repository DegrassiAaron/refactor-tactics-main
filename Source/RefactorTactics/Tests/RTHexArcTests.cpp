#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexArcLibrary.h"
#include "Map/RTHexCellData.h"
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
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 2);
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
	// v12 (#1864) da' un nome stabile al muro interno, perche' il muro SI SPOSTA e il move cambia la sua
	// chiave naturale `(Cell, Segment)`; nessun dato precedente cambia significato, e il default
	// `NAME_None` e' cio' che ogni muro scritto prima gia' era.
	TestEqual(TEXT("la versione corrente e' la 16"), URTHexMapAsset::CurrentFormatVersion, 16);
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

/**
 * UNA SCALA COLLEGA SOLO LAYER ADIACENTI: il predicato, che e' il PRIMO dei due strati di #1869.
 *
 * 🔑 **La regola e' `v0.1` e lo dice**: l'innesco per rivederla — una rampa o un ascensore che vogliono
 * saltare un piano — sta accanto alla funzione, non nella memoria di chi l'ha scritta.
 *
 * ⚠️ **La meta' che discrimina e' quella che AMMETTE.** Un test che asserisse solo *«il salto e' rifiutato»*
 * passerebbe con una funzione che risponde sempre `false`, cioe' con una regola che vieta ogni transizione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStairLayerAdjacencyTest,
	"RefactorTactics.HexMap.StairLayerAdjacencyRule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStairLayerAdjacencyTest::RunTest(const FString&)
{
	const FRTCellId L0(0, 0, 0);
	const FRTCellId L1(0, 0, 1);
	const FRTCellId L2(0, 0, 2);
	const FRTCellId L0Altrove(1, 0, 0);

	// Lo span e' un valore assoluto: non dipende da quale estremo si nomina per primo.
	TestEqual(TEXT("stesso piano: span zero"), URTHexArcLibrary::TransitionLayerSpan(L0, L0Altrove), 0);
	TestEqual(TEXT("piani adiacenti: span uno"), URTHexArcLibrary::TransitionLayerSpan(L0, L1), 1);
	TestEqual(TEXT("e in discesa lo span non cambia"), URTHexArcLibrary::TransitionLayerSpan(L2, L0), 2);

	// ── Cio' che la regola AMMETTE ────────────────────────────────────────────────────────────────────
	TestTrue(TEXT("una scala fra piani adiacenti e' legale"),
		URTHexArcLibrary::IsTransitionLayerSpanLegal(L0, L1, ERTHexTransitionKind::Stair));
	TestTrue(TEXT("e in discesa anche"),
		URTHexArcLibrary::IsTransitionLayerSpanLegal(L1, L0, ERTHexTransitionKind::Stair));

	// 🔴 **Lo span ZERO e' legale, e non e' una dimenticanza.** La issue scrive la regola `== 1`, che
	// vieterebbe anche questo caso: ma `FRTHexEdge::Kind` vale `Stair` per DEFAULT, quindi ogni transizione
	// scritta senza scegliere un tipo e' una scala — comprese quelle sullo stesso piano. Con `== 1` il test
	// `RefactorTactics.Map.Dependency.CellTakesTransitionsCitingIt` diventerebbe rosso: tre transizioni
	// tutte a layer 0, e l'asserzione che l'allestimento non produca errori. Il difetto e' il SALTO.
	TestTrue(TEXT("una transizione sullo stesso piano non e' un salto"),
		URTHexArcLibrary::IsTransitionLayerSpanLegal(L0, L0Altrove, ERTHexTransitionKind::Stair));

	// ── Cio' che la regola RIFIUTA ────────────────────────────────────────────────────────────────────
	TestFalse(TEXT("una scala che salta un piano NON e' legale"),
		URTHexArcLibrary::IsTransitionLayerSpanLegal(L0, L2, ERTHexTransitionKind::Stair));
	TestFalse(TEXT("e in discesa nemmeno"),
		URTHexArcLibrary::IsTransitionLayerSpanLegal(L2, L0, ERTHexTransitionKind::Stair));
	TestFalse(TEXT("ne' un salto piu' lungo"),
		URTHexArcLibrary::IsTransitionLayerSpanLegal(L0, FRTCellId(0, 0, 5), ERTHexTransitionKind::Stair));

	// 🔑 **LA GUARDIA, e non e' fra due letterali**: si enumera `ERTHexTransitionKind` per RIFLESSIONE e si
	// asserisce che i tipi vincolati dalla v0.1 siano **esattamente uno**. Coglie i due modi opposti di
	// sbagliare — nessuno vincolato, cioe' la regola non morde; tutti vincolati, cioe' morde cinque tipi
	// che nessuna issue ha nominato — e un `Kind` nuovo aggiunto alla grammatica entra qui da solo.
	const UEnum* Enum = StaticEnum<ERTHexTransitionKind>();
	if (!TestNotNull(TEXT("l'enum dei tipi di transizione ha la riflessione"), Enum))
	{
		return false;
	}
	// `NumEnums() - 1`: l'ultimo e' il `_MAX` sintetico che UHT aggiunge.
	const int32 Valori = Enum->NumEnums() - 1;
	if (!TestTrue(TEXT("l'enum dichiara piu' di un tipo"), Valori > 1))
	{
		return false;
	}

	int32 Vincolati = 0;
	FString Quali;
	for (int32 Index = 0; Index < Valori; ++Index)
	{
		const ERTHexTransitionKind K = static_cast<ERTHexTransitionKind>(Enum->GetValueByIndex(Index));
		if (!URTHexArcLibrary::IsTransitionLayerSpanLegal(L0, L2, K))
		{
			++Vincolati;
			Quali += (Quali.IsEmpty() ? TEXT("") : TEXT(", "));
			Quali += Enum->GetNameStringByIndex(Index);
		}
	}
	TestEqual(FString::Printf(
		TEXT("in v0.1 UN tipo solo vieta il salto di un piano (sono: %s)"), *Quali), Vincolati, 1);
	TestEqual(TEXT("ed e' Stair"), Quali, FString(TEXT("Stair")));

	// 🔴 **L'INNESCO MECCANICO, ed e' il pezzo che un commento non puo' portare.** L'uscita anticipata
	// `Kind != Stair -> true` ASSORBE IN SILENZIO ogni tipo futuro: un `Escalator` aggiunto domani
	// passerebbe a qualunque span senza una riga di codice ne' una segnalazione — e la guardia qui sopra,
	// che conta i tipi VINCOLATI, resterebbe verde, perche' un tipo in piu' NON vincolato non la muove.
	//
	// ⚠️ Questa asserzione pinna la grammatica **per nome**: il giorno in cui compare un settimo valore
	// cade, e chiede di decidere `MAP-5` — *«una rampa o un ascensore possono saltare un piano, e la scala
	// no?»* — invece di estendere per abitudine. E' scritta per diventare rossa, come
	// `Equipment.SplitHasNoConsumerYet`: una scadenza che nessun gate rilegge non e' una scadenza.
	FString Grammatica;
	for (int32 Index = 0; Index < Valori; ++Index)
	{
		Grammatica += (Grammatica.IsEmpty() ? TEXT("") : TEXT(","));
		Grammatica += Enum->GetNameStringByIndex(Index);
	}
	TestEqual(TEXT("la grammatica governata da MAP-5: un tipo nuovo cade qui e chiede una decisione"),
		Grammatica, FString(TEXT("Stair,Ramp,Bridge,Tunnel,Elevator,Jump")));
	return true;
}

/**
 * IL SECONDO STRATO: una `L0 <-> L2` che sta GIA' nell'asset e' SEGNALATA da `ValidateMap` (#1869).
 *
 * 🔑 **Sono due asserzioni distinte perche' sono due strati, e uno non copre l'altro**: il rifiuto al gesto
 * impedisce di scriverla d'ora in poi, questo dichiara quella che c'e' — in un asset di versione precedente
 * o in un dato ricostruito, che si carica comunque.
 *
 * ⚠️ **L'asserzione e' sul CONTENUTO della voce, non sul conteggio.** Un `Num() > 0` passerebbe per
 * qualunque altra regola che scattasse sull'allestimento, e misurerebbe un errore diverso da quello in
 * esame; e la controprova — la scala legale, e il salto di un `Kind` che la v0.1 non vincola — e' cio' che
 * distingue questa regola da una che segnala tutto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStairLayerSkipValidationTest,
	"RefactorTactics.HexMap.StairLayerSkipIsSignalled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStairLayerSkipValidationTest::RunTest(const FString&)
{
	// Tre piani ESISTENTI: senza la cella di mezzo scatterebbe «transizione verso cella inesistente», e
	// l'asserzione misurerebbe quell'errore invece del salto.
	auto ColonnaATrePiani = []() -> URTHexMapAsset*
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		M->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, 0)));
		M->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, 1)));
		M->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, 2)));
		M->SortCells();
		return M;
	};

	// 🔑 **Si filtra per REASON CODE, non per il testo del messaggio.** E' la disciplina dichiarata
	// sull'enum: *«un test che riconosce una regola dalla sua stringa si rompe alla prima riformulazione, e
	// insegna a non toccare i messaggi»*. Questo test sopravvive a una riscrittura del messaggio.
	auto SegnalazioniDelSalto = [](const URTHexMapAsset* M) -> TArray<FRTMapValidationIssue>
	{
		TArray<FRTMapValidationIssue> Tutte;
		M->ValidateMapDetailed(Tutte);
		return Tutte.FilterByPredicate([](const FRTMapValidationIssue& I)
		{
			return I.Reason == ERTMapValidationReason::StairSkipsLayer;
		});
	};

	// Quante righe `Error:` produce `ValidateMap` in tutto: serve per il DELTA, piu' sotto.
	auto RigheDiErrore = [](const URTHexMapAsset* M) -> int32
	{
		int32 N = 0;
		for (const FString& Riga : M->ValidateMap())
		{
			if (Riga.StartsWith(TEXT("Error:"))) { ++N; }
		}
		return N;
	};

	// ── CONTROPROVA 1: la scala legale non fa rumore ──────────────────────────────────────────────────
	// ⚠️ E' la meta' che discrimina: senza, una regola che segnalasse OGNI transizione passerebbe.
	URTHexMapAsset* Legale = ColonnaATrePiani();
	Legale->Transitions.Add(FRTHexEdge(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), /*Cost*/ 2,
		ERTHexTransitionKind::Stair));
	TestEqual(TEXT("una scala fra piani adiacenti non e' segnalata"), SegnalazioniDelSalto(Legale).Num(), 0);

	// ── CONTROPROVA 2: il salto di un tipo che la v0.1 NON vincola non e' segnalato ───────────────────
	// Senza questa, una regola che ignorasse il `Kind` passerebbe il test.
	URTHexMapAsset* Ponte = ColonnaATrePiani();
	Ponte->Transitions.Add(FRTHexEdge(FRTCellId(0, 0, 0), FRTCellId(0, 0, 2), /*Cost*/ 2,
		ERTHexTransitionKind::Bridge));
	TestEqual(TEXT("un PONTE fra piani non adiacenti non e' segnalato in v0.1"),
		SegnalazioniDelSalto(Ponte).Num(), 0);

	// ── IL DIFETTO: la scala che salta un piano ───────────────────────────────────────────────────────
	URTHexMapAsset* Salto = ColonnaATrePiani();
	Salto->Transitions.Add(FRTHexEdge(FRTCellId(0, 0, 0), FRTCellId(0, 0, 2), /*Cost*/ 2,
		ERTHexTransitionKind::Stair));
	const TArray<FRTMapValidationIssue> Segnalate = SegnalazioniDelSalto(Salto);
	if (!TestEqual(TEXT("una scala L0 -> L2 e' segnalata, una volta sola"), Segnalate.Num(), 1))
	{
		return false;
	}

	// `Error` e non `Warning`: le due sole Warning di questo validator dicono «inerte, non cambia nessun
	// esito», e una scala percorsa non e' inerte — il grafo la offre.
	TestTrue(TEXT("e' un Error, non un Warning"), Segnalate[0].bIsError);

	// La cella colpevole e' l'estremo BASSO: chi interroga per cella trova la scala sul suo piede.
	TestTrue(TEXT("la segnalazione e' ancorata all'estremo basso"),
		Segnalate[0].Cell == FRTCellId(0, 0, 0));

	// La diagnosi nomina DI QUANTO salta: «non valido» non basta a correggerlo (#1869, Debug/Logging).
	TestTrue(FString::Printf(TEXT("il messaggio dice di quanti layer salta: %s"), *Segnalate[0].Message),
		Segnalate[0].Message.Contains(TEXT("2 layer")));

	// ── E `ValidateMap` la porta in superficie, misurato come DELTA ───────────────────────────────────
	// 🔑 Il delta invece del valore assoluto: le due mappe differiscono per il solo estremo alto della
	// transizione, quindi qualunque altra segnalazione dell'allestimento si cancella fra i due termini —
	// e l'asserzione non dipende dalla formulazione del mio messaggio.
	TestEqual(TEXT("ValidateMap guadagna UNA riga Error: rispetto alla stessa mappa con la scala legale"),
		RigheDiErrore(Salto) - RigheDiErrore(Legale), 1);

	// ⚠️ E il caricamento NON e' bloccato: `ValidateMap` e' un referto, e i suoi due consumatori non-test
	// (`ARTHexMapActor::ValidateAsset` e `URTHexMapSummaryLibrary::DescriviValidazione`) lo stampano e lo
	// contano. La mappa resta leggibile — la cella su L2 c'e' ancora, e la transizione pure.
	TestNotNull(TEXT("la mappa segnalata si legge comunque"), Salto->FindCell(FRTCellId(0, 0, 2)));
	TestEqual(TEXT("e la transizione difettosa non e' stata rimossa dalla validazione"),
		Salto->Transitions.Num(), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
