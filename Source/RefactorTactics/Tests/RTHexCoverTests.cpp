#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h" // MakeFlatArena: un solo builder di arena piatta
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "Pathfinding/RTHexPathLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Esagono pieno di raggio N sul layer 0, tutto visibile. Nome distinto per file (unity build). */
	URTHexMapAsset* MakeCoverMap(int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		return M;
	}

	/** Copertura del tipo indicato sul bordo di una cella (la cella deve esistere). */
	void SetCoverEdge(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge, ERTHexCoverType Type)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Covers.Add(FRTHexCover(Edge, Type, FRTHexCover::DefaultIntegrity(Type)));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Copertura bassa sul bordo indicato della cella (la cella deve esistere). */
	void SetLowCoverEdge(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge)
	{
		SetCoverEdge(Map, Id, Edge, ERTHexCoverType::Low);
	}

	/** Vero se il grafo di traversata offre `To` fra i vicini di `From`. */
	bool CoverGraphHasStep(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To)
	{
		for (const TPair<FRTCellId, int32>& Step : URTHexPathLibrary::GraphNeighbors(Map, From))
		{
			if (Step.Key == To) { return true; }
		}
		return false;
	}

	/**
	 * Unita' del Blast. Da CP 16.2 il FACING conta: la copertura protegge solo dai colpi che arrivano
	 * nell'arco frontale, quindi un difensore va orientato verso chi lo attacca perche' il riparo valga.
	 * Prima di quella regola il campo non esisteva e questi test misuravano la copertura con l'attaccante
	 * implicitamente alle spalle — funzionava, ma non era cio' che dicevano di verificare.
	 */
	FRTHexCombatUnit CoverUnit(int32 UnitId, int32 TeamId, const FRTCellId& Cell,
		ERTHexDirection Facing = ERTHexDirection::E)
	{
		FRTHexCombatUnit U;
		U.UnitId = UnitId;
		U.TeamId = TeamId;
		U.Cell = Cell;
		U.bAlive = true;
		U.Facing = Facing;
		return U;
	}

	FRTHexAttackIntent CoverIntent(int32 AttackerId, int32 TargetId, ERTAbilityShape Shape,
		int32 RangeCells, int32 Power, int32 AreaRadius = 0)
	{
		FRTHexAttackIntent I;
		I.AttackerId = AttackerId;
		I.TargetId = TargetId;
		I.Shape = Shape;
		I.RangeCells = RangeCells;
		I.AreaRadius = AreaRadius;
		I.Power = Power;
		return I;
	}

	/** Danno arrivato al bersaglio indicato (-1 se non e' stato colpito). Il colpo a Power 0 resta un colpo. */
	int32 CoverPowerOn(const FRTHexBlastPlan& Plan, int32 TargetId)
	{
		for (const FRTHexAttackHit& Hit : Plan.Hits)
		{
			if (Hit.TargetId == TargetId) { return Hit.Power; }
		}
		return -1;
	}
}

/**
 * La copertura bassa sul bordo ATTRAVERSATO dal colpo riduce di 10 il danno diretto.
 *
 * Geometria: attaccante in (0,0), bersaglio in (2,0). La linea passa per (1,0), quindi il colpo entra nella
 * cella del bersaglio dal bordo W. Una copertura su quel bordo e' interposta; la stessa scena senza copertura
 * e' il controllo che il -10 venga dalla copertura e non da altro.
 */
// ⚠️ `ClientContext` oltre a `EditorContext`, e non per simmetria con i vicini: e' uno dei **dieci test
// vincolanti** del catalogo (`roadmap-v0.1.md` §6), e CP 12.3 (`#83`) chiede che la suite giri anche in un
// pacchetto. Con il solo `EditorContext` il controller lo filtra fuori dal target `Game` — misurato:
// `Automation List` in un pacchetto Development registrava **zero** test su 875, tutti dichiarati solo Editor.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverDirectionalDamageReductionTest,
	"RefactorTactics.Cover.DirectionalDamageReduction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverDirectionalDamageReductionTest::RunTest(const FString&)
{
	TArray<FRTHexCombatUnit> Units;
	Units.Add(CoverUnit(0, 0, FRTCellId(0, 0)));
	// Il difensore GUARDA l'attaccante (a ovest): la copertura protegge solo l'arco frontale (CP 16.2), e
	// senza dichiararlo questo test misurerebbe un colpo alle spalle invece della riduzione da riparo.
	Units.Add(CoverUnit(1, 1, FRTCellId(2, 0), ERTHexDirection::W));

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CoverIntent(0, 1, ERTAbilityShape::Single, 5, 30));

	URTHexMapAsset* Bare = MakeCoverMap(3);
	const FRTHexBlastPlan Uncovered = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Bare);
	TestEqual(TEXT("senza copertura: danno pieno"), CoverPowerOn(Uncovered, 1), 30);

	URTHexMapAsset* Covered = MakeCoverMap(3);
	SetLowCoverEdge(Covered, FRTCellId(2, 0), ERTHexDirection::W); // bordo verso l'attaccante
	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Covered);

	TestEqual(TEXT("il colpo avviene comunque"), Plan.Hits.Num(), 1);
	TestEqual(TEXT("copertura bassa dal lato protetto: -10"),
		CoverPowerOn(Plan, 1), 30 - URTCombatLibrary::LowCoverDamageReduction);
	TestEqual(TEXT("la riduzione e' quella di catalogo"), URTCombatLibrary::LowCoverDamageReduction, 10);
	return true;
}

/**
 * Da un'altra direzione la stessa copertura e' inefficace: e' un riparo su UN bordo, non un bonus dell'unita'.
 * Verificato sia sul bordo opposto sia sui due bordi adiacenti a quello attraversato, dove l'errore piu'
 * probabile (confondere "adiacente" con "attraversato") si vedrebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverLowCoverWrongSideTest,
	"RefactorTactics.Cover.LowCover.WrongSideNoReduction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverLowCoverWrongSideTest::RunTest(const FString&)
{
	TArray<FRTHexCombatUnit> Units;
	Units.Add(CoverUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CoverUnit(1, 1, FRTCellId(2, 0)));

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CoverIntent(0, 1, ERTAbilityShape::Single, 5, 30));

	// Il colpo entra da W: ogni altro bordo non e' sulla traiettoria.
	const ERTHexDirection WrongSides[] = { ERTHexDirection::E, ERTHexDirection::NW, ERTHexDirection::SW };
	for (const ERTHexDirection Edge : WrongSides)
	{
		URTHexMapAsset* Map = MakeCoverMap(3);
		SetLowCoverEdge(Map, FRTCellId(2, 0), Edge);
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
		TestEqual(FString::Printf(TEXT("bordo %d non attraversato: danno pieno"), static_cast<int32>(Edge)),
			CoverPowerOn(Plan, 1), 30);
	}

	// Copertura sulla cella SBAGLIATA (quella dell'attaccante, sul bordo verso il bersaglio): non protegge
	// chi la subisce. La copertura appartiene alla cella che ripara, non alla traiettoria.
	URTHexMapAsset* Attackers = MakeCoverMap(3);
	SetLowCoverEdge(Attackers, FRTCellId(0, 0), ERTHexDirection::E);
	TestEqual(TEXT("copertura sulla cella dell'attaccante: danno pieno"),
		CoverPowerOn(URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Attackers), 1), 30);
	return true;
}

/**
 * L'area non e' un proiettile: la copertura bassa non ne ripara, nemmeno quando il centro sta dal lato
 * protetto — il caso in cui la protezione sembrerebbe dovuta. E' il confine dichiarato della regola: se il
 * centro fosse dall'altro lato la copertura non sarebbe comunque interposta, quindi "AoE mai ridotto".
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverLowCoverAoESameSideTest,
	"RefactorTactics.Cover.LowCover.AoESameSide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverLowCoverAoESameSideTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCoverMap(3);
	SetLowCoverEdge(Map, FRTCellId(2, 0), ERTHexDirection::W); // bordo verso il centro dell'area

	TArray<FRTHexCombatUnit> Units;
	Units.Add(CoverUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CoverUnit(1, 1, FRTCellId(1, 0)));  // centro dell'area, dal lato protetto
	Units.Add(CoverUnit(2, 1, FRTCellId(2, 0)));  // riparato sul bordo W, preso dal raggio

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CoverIntent(0, 1, ERTAbilityShape::Area, 5, 30, /*AreaRadius*/ 1));

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);

	TestEqual(TEXT("l'area colpisce entrambi"), Plan.Hits.Num(), 2);
	TestEqual(TEXT("centro dell'area: danno pieno"), CoverPowerOn(Plan, 1), 30);
	TestEqual(TEXT("copertura dal lato del centro: nessuna riduzione"), CoverPowerOn(Plan, 2), 30);
	return true;
}

/**
 * Un colpo piu' debole della copertura non cura: il danno si ferma a 0 e il colpo resta AVVENUTO (stessa
 * disciplina di `Deflect`, dove il clamp e' sul valore e non sulla voce: trigger e marchi contano lo stesso).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverLowCoverClampTest,
	"RefactorTactics.Cover.LowCover.NeverHealsTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverLowCoverClampTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCoverMap(3);
	SetLowCoverEdge(Map, FRTCellId(2, 0), ERTHexDirection::W);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(CoverUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CoverUnit(1, 1, FRTCellId(2, 0), ERTHexDirection::W)); // guarda l'attaccante: il riparo vale (CP 16.2)

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CoverIntent(0, 1, ERTAbilityShape::Single, 5, 4)); // 4 - 10 sarebbe negativo

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
	TestEqual(TEXT("il colpo resta nel piano"), Plan.Hits.Num(), 1);
	TestEqual(TEXT("danno azzerato, mai negativo"), CoverPowerOn(Plan, 1), 0);
	return true;
}

/**
 * Nome vincolante della DoD (CP 9.2). La copertura ALTA nega l'attraversamento: vista, passo e proiettili,
 * e lo fa NEI DUE VERSI — il dato sta su un bordo di una cella sola, ma un muro e' un muro da qualunque lato
 * lo si guardi (decisione 2026-08-07 sulla issue #70).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverHighCoverBlocksAllTest,
	"RefactorTactics.Cover.HighCover.BlocksAll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverHighCoverBlocksAllTest::RunTest(const FString&)
{
	// Muro sul bordo W di (1,0): separa (0,0) da (1,0), che sono adiacenti.
	const FRTCellId Behind(0, 0);
	const FRTCellId Walled(1, 0);
	URTHexMapAsset* Map = MakeCoverMap(3);
	SetCoverEdge(Map, Walled, ERTHexDirection::W, ERTHexCoverType::High);

	// Vista: bloccata nei due sensi. E' un ATTRAVERSAMENTO, non una cella che si oscura da sola.
	TestFalse(TEXT("LOS bloccata verso il muro"), URTHexVisionLibrary::HasLineOfSight(Map, Behind, Walled));
	TestFalse(TEXT("LOS bloccata nel verso opposto"), URTHexVisionLibrary::HasLineOfSight(Map, Walled, Behind));
	TestFalse(TEXT("LOS bloccata anche piu' in la' sulla stessa linea"),
		URTHexVisionLibrary::HasLineOfSight(Map, Behind, FRTCellId(3, 0)));

	// Passo: negato nei due sensi, ma le due celle continuano a ESISTERE (non sono muri-cella).
	TestFalse(TEXT("il grafo non offre il passo verso il muro"), CoverGraphHasStep(Map, Behind, Walled));
	TestFalse(TEXT("ne' quello di ritorno"), CoverGraphHasStep(Map, Walled, Behind));
	TestTrue(TEXT("le due celle restano nella mappa"),
		Map->ContainsCell(Behind) && Map->ContainsCell(Walled));
	TestTrue(TEXT("gli altri bordi restano percorribili"), CoverGraphHasStep(Map, Behind, FRTCellId(0, 1)));

	// Proiettili: l'intento non produce colpi e finisce fra quelli bloccati dalla linea di tiro.
	TArray<FRTHexCombatUnit> Units;
	Units.Add(CoverUnit(0, 0, Behind));
	Units.Add(CoverUnit(1, 1, Walled));
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CoverIntent(0, 1, ERTAbilityShape::Single, 5, 30));
	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
	TestEqual(TEXT("nessun colpo attraversa il muro"), Plan.Hits.Num(), 0);
	TestEqual(TEXT("registrato come linea di tiro bloccata"), Plan.BlockedIntents.Num(), 1);
	return true;
}

/**
 * La copertura BASSA non e' un muro: continua a lasciar passare vista e passo, e resta quella di CP 9.1 (-10
 * al danno). Senza questo test, "alta blocca tutto" potrebbe essere implementata su TUTTE le coperture e la
 * suite non se ne accorgerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverLowCoverStillPassableTest,
	"RefactorTactics.Cover.HighCover.LowCoverStaysPassable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverLowCoverStillPassableTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCoverMap(3);
	SetCoverEdge(Map, FRTCellId(1, 0), ERTHexDirection::W, ERTHexCoverType::Low);

	TestTrue(TEXT("la copertura bassa non blocca la vista"),
		URTHexVisionLibrary::HasLineOfSight(Map, FRTCellId(0, 0), FRTCellId(1, 0)));
	TestTrue(TEXT("ne' il passo"), CoverGraphHasStep(Map, FRTCellId(0, 0), FRTCellId(1, 0)));
	TestTrue(TEXT("ne' il passo di ritorno"), CoverGraphHasStep(Map, FRTCellId(1, 0), FRTCellId(0, 0)));

	TestTrue(TEXT("il bordo e' riconosciuto come copertura bassa"),
		URTHexCoverLibrary::CoverBetween(Map, FRTCellId(0, 0), FRTCellId(1, 0)) == ERTHexCoverType::Low);
	TestFalse(TEXT("e non nega l'attraversamento"),
		URTHexCoverLibrary::BlocksTraversal(Map, FRTCellId(0, 0), FRTCellId(1, 0)));

	// Integrita' di catalogo per tipo: 30 la bassa, 50 l'alta.
	TestEqual(TEXT("integrita' della copertura bassa"),
		FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low), 30);
	TestEqual(TEXT("integrita' della copertura alta"),
		FRTHexCover::DefaultIntegrity(ERTHexCoverType::High), 50);
	return true;
}

namespace
{
	/** Intento che porta anche danno a STRUTTURA (l'azione dichiara `DamageStructure`). */
	FRTHexAttackIntent CoverBreachIntent(int32 AttackerId, int32 TargetId, int32 Power, int32 StructurePower)
	{
		FRTHexAttackIntent I = CoverIntent(AttackerId, TargetId, ERTAbilityShape::Single, 5, Power);
		I.StructurePower = StructurePower;
		return I;
	}

	/** Come sopra, ma mirando una CELLA (`Fallback.AttackCell`): e' cosi' che si punta una struttura. */
	FRTHexAttackIntent CoverBreachCellIntent(int32 AttackerId, const FRTCellId& Cell, int32 Power,
		int32 StructurePower)
	{
		FRTHexAttackIntent I = CoverIntent(AttackerId, INDEX_NONE, ERTAbilityShape::Single, 5, Power);
		I.TargetCell = Cell;
		I.StructurePower = StructurePower;
		return I;
	}

	/** Scena standard: muro alto sul bordo W di (1,0), attaccante in (0,0), bersaglio dietro in (2,0). */
	URTHexMapAsset* MakeWalledMap()
	{
		URTHexMapAsset* Map = MakeCoverMap(3);
		SetCoverEdge(Map, FRTCellId(1, 0), ERTHexDirection::W, ERTHexCoverType::High);
		return Map;
	}

	TArray<FRTHexCombatUnit> WalledUnits()
	{
		TArray<FRTHexCombatUnit> Units;
		Units.Add(CoverUnit(0, 0, FRTCellId(0, 0)));
		Units.Add(CoverUnit(1, 1, FRTCellId(2, 0)));
		return Units;
	}
}

/**
 * Nome vincolante della DoD. Abbattuto il muro la vista si riapre — e si riapre perche' la copertura non c'e'
 * PIU', non perche' qualcuno abbia spento un flag: la prova e' che prima dell'ultimo colpo e' ancora chiusa.
 *
 * Il colpo che sbatte contro il muro e' anche quello che lo danneggia: l'intento e' respinto dalla linea di
 * tiro (l'unita' dietro non viene colpita), ma i suoi `DamageStructure` arrivano alla barriera che l'ha
 * fermato. Senza questa regola una copertura alta sarebbe indistruttibile — dietro non c'e' nessun bersaglio
 * nominabile, perche' il muro stesso impedisce di vederlo (decisione 4 sulla issue #70).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverDestructionReopensLOSTest,
	"RefactorTactics.Cover.Destruction.ReopensLOS",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverDestructionReopensLOSTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeWalledMap();
	const TArray<FRTHexCombatUnit> Units = WalledUnits();
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CoverBreachIntent(0, 1, /*Power*/ 35, /*StructurePower*/ 20));

	// Due colpi da 20 su integrita' 50: il muro regge.
	for (int32 Shot = 0; Shot < 2; ++Shot)
	{
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
		TestEqual(FString::Printf(TEXT("colpo %d: nessuno passa"), Shot + 1), Plan.Hits.Num(), 0);
		URTHexCoverLibrary::ApplyStructureDamage(Map, Plan.StructureHits);
	}
	TestFalse(TEXT("dopo due colpi la vista e' ancora chiusa"),
		URTHexVisionLibrary::HasLineOfSight(Map, FRTCellId(0, 0), FRTCellId(2, 0)));

	// Terzo colpo: 60 danni cumulati su 50, il muro cade.
	const FRTHexBlastPlan Last = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
	const TArray<FRTCoverDamageResult> Changes = URTHexCoverLibrary::ApplyStructureDamage(Map, Last.StructureHits);

	TestEqual(TEXT("una struttura cambiata"), Changes.Num(), 1);
	TestTrue(TEXT("ed e' stata distrutta"), Changes.Num() == 1 && Changes[0].bDestroyed);
	TestTrue(TEXT("la vista si riapre"),
		URTHexVisionLibrary::HasLineOfSight(Map, FRTCellId(0, 0), FRTCellId(2, 0)));
	TestTrue(TEXT("e ora i colpi arrivano"),
		URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map).Hits.Num() == 1);
	return true;
}

/**
 * Nome vincolante della DoD. Il grafo di traversata deve aggiornarsi con la vista: se la barriera cade e il
 * passo resta negato, il pathfinding continua a evitare un varco che esiste. E la REVISIONE deve salire —
 * senza, una cache di percorso sopravvive a un grafo cambiato: e' il path fantasma che l'epic E9 esiste per
 * impedire (`URTHexSimLibrary::IsSnapshotStale` confronta hash **e** revisione).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverDestructionUpdatesGraphTest,
	"RefactorTactics.Cover.Destruction.UpdatesGraph",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverDestructionUpdatesGraphTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeWalledMap();
	const FRTCellId Near(0, 0), Far(1, 0);

	TestFalse(TEXT("prima: il passo e' negato"), CoverGraphHasStep(Map, Near, Far));
	const int32 RevisionBefore = Map->Revision;

	TArray<FRTStructureHit> Hits;
	Hits.Add(FRTStructureHit(Near, Far, /*Amount*/ 50, /*AttackerId*/ 0));
	const TArray<FRTCoverDamageResult> Changes = URTHexCoverLibrary::ApplyStructureDamage(Map, Hits);

	TestTrue(TEXT("distrutta"), Changes.Num() == 1 && Changes[0].bDestroyed);
	TestTrue(TEXT("il passo si riapre"), CoverGraphHasStep(Map, Near, Far));
	TestTrue(TEXT("e anche quello di ritorno"), CoverGraphHasStep(Map, Far, Near));
	TestTrue(TEXT("la revisione e' salita"), Map->Revision > RevisionBefore);
	return true;
}

/**
 * Il caso NORMALE, che i tre test della DoD non coprono: un colpo che non basta. La struttura resta in piedi
 * con l'integrita' scalata e continua a bloccare — se cadesse al primo colpo, "integrita' 50" sarebbe un
 * numero senza consumatore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverDestructionPartialTest,
	"RefactorTactics.Cover.Destruction.PartialDamageLeavesItStanding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverDestructionPartialTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeWalledMap();
	const FRTCellId Near(0, 0), Far(1, 0);

	TArray<FRTStructureHit> Hits;
	Hits.Add(FRTStructureHit(Near, Far, /*Amount*/ 20, /*AttackerId*/ 0));
	const TArray<FRTCoverDamageResult> Changes = URTHexCoverLibrary::ApplyStructureDamage(Map, Hits);

	TestEqual(TEXT("una struttura danneggiata"), Changes.Num(), 1);
	TestFalse(TEXT("non distrutta"), Changes.Num() == 1 && Changes[0].bDestroyed);
	TestEqual(TEXT("integrita' residua 30"), Changes.Num() == 1 ? Changes[0].RemainingIntegrity : -1, 30);

	TestTrue(TEXT("blocca ancora la vista"), URTHexCoverLibrary::BlocksTraversal(Map, Near, Far));
	TestFalse(TEXT("e ancora il passo"), CoverGraphHasStep(Map, Near, Far));

	// Il dato scalato e' nella mappa, non solo nel risultato: e' quello che il turno dopo legge.
	const FRTHexCellData* Data = Map->FindCell(Far);
	TestTrue(TEXT("la copertura e' ancora nel dato con 30 punti"),
		Data && Data->Covers.Num() == 1 && Data->Covers[0].Integrity == 30);
	return true;
}

/**
 * Invariante #3: due attaccanti sullo stesso muro nello stesso turno danno lo stesso esito in qualunque
 * ordine. E' la ragione per cui il danno alle strutture si RACCOGLIE (sommato per bordo) e si applica a fase
 * conclusa, invece di essere applicato colpo per colpo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverDestructionOrderIndependentTest,
	"RefactorTactics.Cover.Destruction.OrderIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverDestructionOrderIndependentTest::RunTest(const FString&)
{
	// Due attaccanti ai lati OPPOSTI dello stesso muro, ognuno mirando la cella oltre: 30 + 25 = 55 su 50.
	// Le due coppie di celle arrivano invertite — (0,0)->(1,0) e (1,0)->(0,0) — quindi il test verifica anche
	// che il bordo sia riconosciuto come lo STESSO da entrambi i lati, non come due strutture distinte.
	TArray<FRTHexCombatUnit> Units;
	Units.Add(CoverUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CoverUnit(1, 1, FRTCellId(2, 0)));

	TArray<FRTHexAttackIntent> Forward;
	Forward.Add(CoverBreachCellIntent(0, FRTCellId(2, 0), 10, 30));
	Forward.Add(CoverBreachCellIntent(1, FRTCellId(0, 0), 10, 25));
	TArray<FRTHexAttackIntent> Reversed;
	Reversed.Add(Forward[1]);
	Reversed.Add(Forward[0]);

	URTHexMapAsset* MapA = MakeWalledMap();
	URTHexMapAsset* MapB = MakeWalledMap();
	const FRTHexBlastPlan PlanA = URTHexCombatLibrary::CollectHexAttacks(Units, Forward, MapA);
	const FRTHexBlastPlan PlanB = URTHexCombatLibrary::CollectHexAttacks(Units, Reversed, MapB);

	TestEqual(TEXT("stesso numero di strutture colpite"), PlanA.StructureHits.Num(), PlanB.StructureHits.Num());
	TestEqual(TEXT("un solo bordo, i due colpi sommati"), PlanA.StructureHits.Num(), 1);
	TestEqual(TEXT("danno sommato 30+25"),
		PlanA.StructureHits.Num() == 1 ? PlanA.StructureHits[0].Amount : -1, 55);

	URTHexCoverLibrary::ApplyStructureDamage(MapA, PlanA.StructureHits);
	URTHexCoverLibrary::ApplyStructureDamage(MapB, PlanB.StructureHits);
	TestEqual(TEXT("le due mappe finiscono identiche"), MapA->ComputeHash(), MapB->ComputeHash());
	return true;
}

/**
 * La distruzione deve cambiare l'HASH della mappa (le coperture vi sono entrate con CP 9.1) e incrementare la
 * REVISIONE: sono i due segnali con cui `IsSnapshotStale` avverte chi tiene una cache. Un colpo che non trova
 * nulla non deve muovere ne' l'uno ne' l'altra.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverDestructionRevisionTest,
	"RefactorTactics.Cover.Destruction.BumpsRevisionAndHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverDestructionRevisionTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeWalledMap();
	const uint32 HashBefore = Map->ComputeHash();
	const int32 RevisionBefore = Map->Revision;

	// Colpo a vuoto: bordo senza copertura.
	TArray<FRTStructureHit> Nothing;
	Nothing.Add(FRTStructureHit(FRTCellId(0, 0), FRTCellId(0, 1), 30, 0));
	TestEqual(TEXT("nessuna struttura su quel bordo"),
		URTHexCoverLibrary::ApplyStructureDamage(Map, Nothing).Num(), 0);
	TestEqual(TEXT("hash invariato"), Map->ComputeHash(), HashBefore);
	TestEqual(TEXT("revisione invariata"), Map->Revision, RevisionBefore);

	// Danno parziale: cambia il dato, quindi cambiano hash e revisione.
	TArray<FRTStructureHit> Partial;
	Partial.Add(FRTStructureHit(FRTCellId(0, 0), FRTCellId(1, 0), 20, 0));
	URTHexCoverLibrary::ApplyStructureDamage(Map, Partial);
	const uint32 HashDamaged = Map->ComputeHash();
	TestTrue(TEXT("il danno parziale cambia l'hash"), HashDamaged != HashBefore);
	TestTrue(TEXT("e la revisione"), Map->Revision > RevisionBefore);

	// Distruzione: cambia ancora.
	TArray<FRTStructureHit> Finish;
	Finish.Add(FRTStructureHit(FRTCellId(0, 0), FRTCellId(1, 0), 30, 0));
	URTHexCoverLibrary::ApplyStructureDamage(Map, Finish);
	TestTrue(TEXT("la distruzione cambia l'hash"), Map->ComputeHash() != HashDamaged);
	TestTrue(TEXT("la copertura non c'e' piu'"),
		URTHexCoverLibrary::CoverBetween(Map, FRTCellId(0, 0), FRTCellId(1, 0)) == ERTHexCoverType::None);
	return true;
}

/**
 * Il catalogo DICHIARA chi puo' sfondare, e con quale scala. `HeavyAttack` vale 35 contro un'unita' e 20
 * contro una struttura: due effetti distinti, non un numero riusato. Senza questo test la costante potrebbe
 * derivare dal valore della DoD senza che niente se ne accorga — e la copertura alta sarebbe indistruttibile
 * o cadrebbe in due colpi invece di tre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverHeavyAttackBreachesTest,
	"RefactorTactics.Cover.Destruction.HeavyAttackDeclaresStructureDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverHeavyAttackBreachesTest::RunTest(const FString&)
{
	const FRTActionDef Heavy = URTCatalogLibrary::FindCoreAction(TEXT("Action.HeavyAttack"));
	TestFalse(TEXT("l'azione esiste"), Heavy.ActionId.IsNone());

	int32 UnitDamage = 0;
	int32 StructureDamage = 0;
	for (const FRTActionEffectSpec& Spec : Heavy.Effects)
	{
		if (Spec.Effect == ERTActionEffect::Damage) { UnitDamage = Spec.Amount; }
		if (Spec.Effect == ERTActionEffect::DamageStructure) { StructureDamage = Spec.Amount; }
	}
	TestEqual(TEXT("35 contro un'unita'"), UnitDamage, 35);
	TestEqual(TEXT("20 contro una struttura"), StructureDamage, 20);

	// Tre colpi per abbattere una copertura alta: e' il numero che i test di distruzione danno per buono.
	TestTrue(TEXT("integrita' 50: due colpi non bastano, tre si'"),
		2 * StructureDamage < FRTHexCover::DefaultIntegrity(ERTHexCoverType::High)
		&& 3 * StructureDamage >= FRTHexCover::DefaultIntegrity(ERTHexCoverType::High));

	// Il resto del catalogo non sfonda: sfondare e' una capacita' concessa, non un effetto dell'essere forti.
	int32 Breachers = 0;
	for (const FRTActionDef& Def : URTCatalogLibrary::GetCoreActionCatalog())
	{
		for (const FRTActionEffectSpec& Spec : Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::DamageStructure) { ++Breachers; break; }
		}
	}
	TestEqual(TEXT("una sola azione core sfonda, in v0.1"), Breachers, 1);
	return true;
}

/**
 * CP 9.5 — una copertura si puo' ERIGERE in partita, e il bordo ne regge UNA sola.
 *
 * «Non sovrapponibile» e' del catalogo (`Action.CreateCover`) e finora valeva solo per il dato disegnato a
 * mano (`ValidateMap`). Da qui si scrive sulla mappa durante il turno, quindi la stessa invariante deve reggere
 * a runtime — e reggere sulle DUE facce del bordo: una copertura dichiarata dal vicino e' la stessa barriera,
 * e ammetterne una seconda darebbe un muro che regge il doppio, con la meta' del danno che sparisce senza che
 * niente lo spieghi.
 *
 * Il rifiuto deve anche essere GRATUITO: se toccasse la revisione, ogni tentativo fallito invaliderebbe le
 * cache di percorso di tutti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverAddRejectsOccupiedEdgeTest,
	"RefactorTactics.Cover.AddCover.RejectsOccupiedEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverAddRejectsOccupiedEdgeTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCoverMap(2);
	const FRTCellId Cell(0, 0);
	const FRTCellId East = URTHexLibrary::Neighbors(Cell)[static_cast<int32>(ERTHexDirection::E)];

	TestTrue(TEXT("la prima copertura nasce"),
		URTHexCoverLibrary::AddCover(Map, Cell, ERTHexDirection::E, ERTHexCoverType::Low, 30));
	TestEqual(TEXT("ed e' quella del catalogo: bassa, integrita' 30"),
		URTHexCoverLibrary::CoverBetween(Map, Cell, East), ERTHexCoverType::Low);

	const int32 AfterCreate = Map->Revision;

	TestFalse(TEXT("lo stesso bordo, dalla stessa faccia: rifiutata"),
		URTHexCoverLibrary::AddCover(Map, Cell, ERTHexDirection::E, ERTHexCoverType::Low, 30));
	TestFalse(TEXT("lo stesso bordo, dalla faccia del VICINO: rifiutata"),
		URTHexCoverLibrary::AddCover(Map, East, ERTHexDirection::W, ERTHexCoverType::Low, 30));
	TestFalse(TEXT("fuori mappa: rifiutata"),
		URTHexCoverLibrary::AddCover(Map, FRTCellId(99, 99), ERTHexDirection::E, ERTHexCoverType::Low, 30));
	TestFalse(TEXT("integrita' zero: non e' una copertura"),
		URTHexCoverLibrary::AddCover(Map, FRTCellId(0, 1), ERTHexDirection::E, ERTHexCoverType::Low, 0));
	TestFalse(TEXT("tipo None: non e' una copertura"),
		URTHexCoverLibrary::AddCover(Map, FRTCellId(0, 1), ERTHexDirection::E, ERTHexCoverType::None, 30));

	TestEqual(TEXT("nessun rifiuto ha toccato la revisione"), Map->Revision, AfterCreate);

	// Un bordo LIBERO della stessa cella resta disponibile: il rifiuto e' del bordo, non della cella.
	TestTrue(TEXT("un altro bordo della stessa cella: ammesso"),
		URTHexCoverLibrary::AddCover(Map, Cell, ERTHexDirection::W, ERTHexCoverType::Low, 30));
	return true;
}

/**
 * CP 9.5 — la copertura eretta in partita e' una copertura VERA, e toglierla la fa smettere di proteggere.
 *
 * Il difetto ricorrente di questo repository e' il dato che nessuno legge: qui si verifica che a chiedere la
 * riduzione sia il **consumatore reale** (`CollectHexAttacks`, lo stesso che serve le coperture disegnate a
 * mano) e non un secondo percorso scritto per l'occasione. Poi si toglie e il danno torna pieno: e' la prova
 * che la scadenza di un pannello temporaneo avra' un effetto osservabile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverAddThenRemoveTest,
	"RefactorTactics.Cover.AddCover.RemovedStopsProtecting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverAddThenRemoveTest::RunTest(const FString&)
{
	TArray<FRTHexCombatUnit> Units;
	Units.Add(CoverUnit(0, 0, FRTCellId(0, 0)));
	// Il difensore GUARDA l'attaccante (a ovest): la copertura ripara solo l'arco frontale (CP 16.2), e senza
	// dichiararlo questo test misurerebbe un colpo alle spalle invece della riduzione da riparo — che e'
	// esattamente come si e' rotto quando E9.5 ed E16, sviluppate in parallelo, si sono incontrate su `main`.
	Units.Add(CoverUnit(1, 1, FRTCellId(2, 0), ERTHexDirection::W));

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CoverIntent(0, 1, ERTAbilityShape::Single, 5, 30));

	URTHexMapAsset* Map = MakeCoverMap(3);
	TestEqual(TEXT("prima: danno pieno"),
		CoverPowerOn(URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map), 1), 30);

	const int32 Before = Map->Revision;
	TestTrue(TEXT("il pannello si erige sul bordo verso l'attaccante"),
		URTHexCoverLibrary::AddCover(Map, FRTCellId(2, 0), ERTHexDirection::W, ERTHexCoverType::Low, 30));
	TestTrue(TEXT("erigerla incrementa la revisione"), Map->Revision > Before);

	TestEqual(TEXT("il consumatore reale la legge: -10"),
		CoverPowerOn(URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map), 1),
		30 - URTCombatLibrary::LowCoverDamageReduction);

	const int32 AfterCreate = Map->Revision;
	TestTrue(TEXT("si toglie"),
		URTHexCoverLibrary::RemoveCover(Map, FRTCellId(2, 0), ERTHexDirection::W));
	TestTrue(TEXT("toglierla incrementa la revisione"), Map->Revision > AfterCreate);
	TestEqual(TEXT("tolta: danno di nuovo pieno"),
		CoverPowerOn(URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map), 1), 30);

	const int32 AfterRemove = Map->Revision;
	TestFalse(TEXT("togliere un bordo scoperto non e' un evento"),
		URTHexCoverLibrary::RemoveCover(Map, FRTCellId(2, 0), ERTHexDirection::W));
	TestEqual(TEXT("e non tocca la revisione"), Map->Revision, AfterRemove);
	return true;
}

/**
 * Una copertura eretta IN PARTITA non e' piu' forte di una disegnata: anche lei si aggira alle spalle.
 *
 * Questo test non c'era, e la sua assenza ha avuto un costo. E9.5 (coperture temporanee) ed E16 (facing) sono
 * state sviluppate in parallelo e mergiate separatamente: entrambe verdi sulla propria base, il merge testuale
 * pulito, e sull'unione due test rossi — perche' nessun test copriva il punto in cui le due regole si toccano.
 * Il difetto e' stato scoperto ricompilando `main`, non da un test.
 *
 * Quel che si fissa qui e' che le due direzionalita' restano **ortogonali** (CP 9.1): la copertura e' del
 * BORDO, l'arco frontale e' dell'UNITA', e la seconda annulla la prima senza che l'origine della copertura —
 * dato di mappa o `Action.CreateCover` — cambi nulla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverRuntimeCoverRearHitTest,
	"RefactorTactics.Cover.AddCover.RearHitBypassesRuntimeCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverRuntimeCoverRearHitTest::RunTest(const FString&)
{
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CoverIntent(0, 1, ERTAbilityShape::Single, 5, 30));

	// Stessa copertura, eretta a runtime sul bordo attraversato dal colpo: cambia SOLO dove guarda il difensore.
	auto DamageWithFacing = [&Intents](ERTHexDirection Facing)
	{
		TArray<FRTHexCombatUnit> Units;
		Units.Add(CoverUnit(0, 0, FRTCellId(0, 0)));
		Units.Add(CoverUnit(1, 1, FRTCellId(2, 0), Facing));

		URTHexMapAsset* Map = MakeCoverMap(3);
		URTHexCoverLibrary::AddCover(Map, FRTCellId(2, 0), ERTHexDirection::W, ERTHexCoverType::Low, 30);
		return CoverPowerOn(URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map), 1);
	};

	// L'attaccante e' a ovest: guardandolo, il riparo vale.
	TestEqual(TEXT("difensore rivolto all'attaccante: la copertura vale"),
		DamageWithFacing(ERTHexDirection::W), 30 - URTCombatLibrary::LowCoverDamageReduction);

	// Di spalle, la stessa copertura non ripara — come una qualunque disegnata sulla mappa (CP 16.2).
	TestEqual(TEXT("colpo alle spalle: la copertura eretta in partita non protegge"),
		DamageWithFacing(ERTHexDirection::E), 30);
	return true;
}


/**
 * Il risultato del danno a una struttura dice CHI ha colpito (#405).
 *
 * L'informazione esisteva gia' in ingresso — `FRTStructureHit::AttackerId`, con il commento «serve al TurnLog,
 * non al calcolo» — e si perdeva in uscita: il chiamante iterava i risultati e non aveva piu' modo di sapere
 * chi avesse prodotto quale. Cosi' la voce di copertura danneggiata era l'unica del combattimento a non poter
 * nominare il proprio attore, e finiva nel TurnLog con `UnitId = 0`, cioe' dichiarando «nessuno».
 *
 * Resta un INDICE e non un Actor: lo strato non smette di essere puro per questo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverDamageCarriesAttackerTest,
	"RefactorTactics.Cover.Destruction.ResultCarriesAttacker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverDamageCarriesAttackerTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeWalledMap();
	const TArray<FRTHexCombatUnit> Units = WalledUnits();
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CoverBreachIntent(/*AttackerId*/ 0, /*TargetId*/ 1, /*Power*/ 35, /*StructurePower*/ 20));

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
	if (!TestEqual(TEXT("un bordo colpito"), Plan.StructureHits.Num(), 1)) { return false; }
	// Precondizione, non ridondanza: se l'hit non portasse l'attaccante, il test sotto verificherebbe la
	// propagazione di un dato che non c'e' e passerebbe per il motivo sbagliato.
	TestEqual(TEXT("l'hit dichiara l'attaccante"), Plan.StructureHits[0].AttackerId, 0);

	const TArray<FRTCoverDamageResult> Changes = URTHexCoverLibrary::ApplyStructureDamage(Map, Plan.StructureHits);
	if (!TestEqual(TEXT("una struttura cambiata"), Changes.Num(), 1)) { return false; }
	TestEqual(TEXT("e il risultato porta lo stesso attaccante"), Changes[0].AttackerId, 0);
	return true;
}

/**
 * **Una copertura costruita col default nasce al proprio valore di CATALOGO, qualunque sia il tipo**
 * (#1194, `D-186`).
 *
 * 🔴 **Nessun test lo confrontava, ed e' la ragione per cui l'incoerenza e' sopravvissuta.** Il costruttore
 * dichiarava `InIntegrity = 30` fisso e indipendente dal `Type`: `FRTHexCover(Edge, High)` nasceva a **30**,
 * il 60% del suo catalogo, senza che nulla l'avesse colpita. Accettava il tipo e ignorava la funzione che sa
 * cosa quel tipo vale.
 *
 * ⚠️ **Questo test copre il COSTRUTTORE, non l'autoraggio dall'editor**: chi aggiunge una entry `Covers`
 * nel dettaglio di un `URTHexMapAsset` costruisce con `FRTHexCover()` — `Low`/30 — e cambiare `Type` in
 * `High` non ricalcola niente. Quel percorso e' aperto, ed e' #1317.
 *
 * ⚠️ **Il confronto e' col catalogo, non coi letterali `30` e `50`**: scriverli qui creerebbe la terza copia
 * degli stessi numeri, che e' la specie di difetto che questa issue chiude.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverDefaultMatchesCatalogTest,
	"RefactorTactics.Cover.DefaultIntegrityMatchesCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverDefaultMatchesCatalogTest::RunTest(const FString&)
{
	for (const ERTHexCoverType Tipo : { ERTHexCoverType::Low, ERTHexCoverType::High })
	{
		const FRTHexCover Costruita(ERTHexDirection::E, Tipo);
		TestEqual(*FString::Printf(TEXT("il default del tipo %d e' quello di catalogo"),
			static_cast<int32>(Tipo)), Costruita.Integrity, FRTHexCover::DefaultIntegrity(Tipo));
	}

	// E i due tipi non valgono lo stesso: senza questa riga il test passerebbe anche su un catalogo piatto,
	// cioe' proprio col difetto che chiude.
	TestTrue(TEXT("la copertura alta vale piu' della bassa"),
		FRTHexCover::DefaultIntegrity(ERTHexCoverType::High)
			> FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low));

	// Un valore ESPLICITO resta quello chiesto, e `0` non e' la sentinella: una copertura a zero e' il caso
	// che `ValidateMap` deve rifiutare, e confonderlo con «prendi il catalogo» lo renderebbe inscrivibile.
	TestEqual(TEXT("un valore esplicito non viene sostituito"),
		FRTHexCover(ERTHexDirection::E, ERTHexCoverType::High, 12).Integrity, 12);
	TestEqual(TEXT("e zero resta zero"),
		FRTHexCover(ERTHexDirection::E, ERTHexCoverType::Low, 0).Integrity, 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
