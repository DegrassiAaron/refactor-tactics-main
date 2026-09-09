#include "Misc/AutomationTest.h"

#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexDoorLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTEnemyTacticalQuery.h"
#include "Perception/RTKnowledgeView.h"
#include "Perception/RTTeamKnowledge.h"
#include "Turn/RTMatchSetupLibrary.h"
// La classificazione dei campi si legge dalla REFLECTION, non da un elenco scritto a mano (#2331).
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Arena esagonale piena di raggio N sul layer 0, tutte le celle a MoveCost 1. */
	URTHexMapAsset* MakeQueryArena(int32 Radius)
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
	}

	/**
	 * Azione di catalogo REALE, con la forma e la portata d'attacco dichiarate dal chiamante.
	 *
	 * ⚠️ `Def` viene da `FindCoreAction`, non scritto a mano: e' cio' che rende i test 4 e 5 misure sul
	 * catalogo spedito invece che su una copia locale che potrebbe divergere da esso in silenzio.
	 */
	URTActionData* MakeQueryAction(const FName& CoreActionId, ERTAbilityShape Shape = ERTAbilityShape::Single,
		int32 AttackRange = 0, int32 AreaRadius = 0)
	{
		URTActionData* A = NewObject<URTActionData>(GetTransientPackage());
		A->Def = URTCatalogLibrary::FindCoreAction(CoreActionId);
		A->RangeCells = AttackRange > 0 ? AttackRange : A->Def.RangeCells;
		A->Shape = Shape;
		A->AreaRadius = AreaRadius;
		return A;
	}

	URTHeroData* MakeQueryHero(const FName& HeroId, int32 MovePoints, const TArray<URTActionData*>& Actions)
	{
		URTHeroData* H = NewObject<URTHeroData>(GetTransientPackage());
		H->HeroId = HeroId;
		H->MovePoints = MovePoints;
		for (URTActionData* A : Actions)
		{
			H->Actions.Add(A);
		}
		return H;
	}

	FRTKnowledgeEntry LiveEntry(int32 StableUnitId, const FRTCellId& Cell, const FName& HeroId)
	{
		FRTKnowledgeEntry E;
		E.StableUnitId = StableUnitId;
		E.Visibility = ERTKnowledgeVisibility::Live;
		E.Cell = Cell;
		E.HeroId = HeroId;
		return E;
	}

	FRTKnowledgeSubject Subject(int32 StableUnitId, int32 TeamId, const FRTCellId& Cell, const FName& HeroId)
	{
		FRTKnowledgeSubject S;
		S.StableUnitId = StableUnitId;
		S.TeamId = TeamId;
		S.Cell = Cell;
		S.HeroId = HeroId;
		S.bAlive = true;
		return S;
	}

	bool Has(const TArray<FRTCellId>& Cells, const FRTCellId& Id) { return Cells.Contains(Id); }

	/** Uguaglianza elemento per elemento. `TestEqual` non ha un'overload per `TArray<FRTCellId>`, e
	 *  confrontare i soli conteggi lascerebbe passare due insiemi diversi della stessa cardinalita'. */
	bool SameCells(const TArray<FRTCellId>& A, const TArray<FRTCellId>& B)
	{
		if (A.Num() != B.Num()) { return false; }
		for (int32 i = 0; i < A.Num(); ++i)
		{
			if (!(A[i] == B[i])) { return false; }
		}
		return true;
	}

	bool IsStablySorted(const TArray<FRTCellId>& Cells)
	{
		for (int32 i = 1; i < Cells.Num(); ++i)
		{
			// Strettamente crescente: cattura sia il disordine sia i duplicati, che per un insieme di celle
			// sono lo stesso difetto (una regione non e' un multiset).
			if (!URTHexLibrary::StableLess(Cells[i - 1], Cells[i])) { return false; }
		}
		return true;
	}

	/** Cella che blocca il movimento (muro pieno). */
	void PutBlocker(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.bBlocksMovement = true;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Cella che blocca la linea di tiro senza bloccare il passo: separa i due predicati. */
	void PutSightBlocker(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.bBlocksLineOfSight = true;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	void PutDoor(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge, ERTHexDoorState State)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Doors.Add(FRTHexDoor(Edge, State, /*DoorId*/ INDEX_NONE));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Il soggetto tipo dei test: si muove di 5, colpisce a 1 cella, forma singola. */
	URTHeroData* MeleeHero(int32 MovePoints = 5)
	{
		return MakeQueryHero(TEXT("Hero.QueryProbe"), MovePoints,
			{ MakeQueryAction(TEXT("Action.BasicAttack"), ERTAbilityShape::Single, /*AttackRange*/ 1) });
	}
}

// ---------------------------------------------------------------------------------------------------------
// 1 — Il canary. Due stati nascosti diversi, stessa conoscenza autorizzata, stesso output.
// ---------------------------------------------------------------------------------------------------------

/**
 * 🔴 **Il gate che decide questa feature.** Ha quattro parti, e la prima esiste perche' senza di essa
 * sarebbe VACUO: una query che non restituisse mai niente supererebbe tutte le altre tre.
 *
 * La differenza fra le due scene NON e' scritta a mano nella vista: passa da
 * `URTKnowledgeViewLibrary::ViewForTeam` con due elenchi di soggetti AUTOREVOLI che differiscono per
 * un'unita' nascosta. E' cio' che rende il test una misura della porta e non una tautologia sulla firma —
 * se un giorno `RegionsFor` prendesse lo snapshot, o se la porta perdesse, questa parte diventa rossa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyQueryHidesUnobservedStateTest,
	"RefactorTactics.Perception.EnemyQueryHidesUnobservedState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyQueryHidesUnobservedStateTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeQueryArena(6);
	URTHeroData* Hero = MeleeHero();

	// L'osservatore e' la squadra 0. Il soggetto interrogato e' l'unita' 7 della squadra 1, che vede.
	const FRTCellId SubjectCell(0, 0);
	const FRTCellId HiddenCell(2, 0); // dentro il raggio di movimento del soggetto: bloccherebbe, se si sapesse

	FRTTeamKnowledge Knowledge;
	Knowledge.TeamId = 0;
	Knowledge.TurnNumber = 1;
	Knowledge.VisibleCells = { SubjectCell }; // vede il soggetto, NON la cella nascosta

	TArray<FRTKnowledgeSubject> WithoutHidden;
	WithoutHidden.Add(Subject(7, /*Team*/ 1, SubjectCell, Hero->HeroId));

	TArray<FRTKnowledgeSubject> WithHidden = WithoutHidden;
	WithHidden.Add(Subject(9, /*Team*/ 1, HiddenCell, TEXT("Hero.Other")));

	const FRTKnowledgeView ViewA = URTKnowledgeViewLibrary::ViewForTeam(Knowledge, WithoutHidden, /*Observer*/ 0);
	const FRTKnowledgeView ViewB = URTKnowledgeViewLibrary::ViewForTeam(Knowledge, WithHidden, /*Observer*/ 0);

	FRTEnemyTacticalRegions FromA;
	FRTEnemyTacticalRegions FromB;

	// 1 — CONTROPROVA DI NON VACUITA'. Senza questa, le tre parti sotto passerebbero anche con una funzione
	// che non risponde mai.
	if (!TestTrue(TEXT("il soggetto osservato produce una risposta"),
		URTEnemyTacticalQueryLibrary::RegionsFor(Map, ViewA, /*Subject*/ 7, Hero, FromA))) { return false; }
	if (!TestTrue(TEXT("e le tre regioni non sono vuote"),
		FromA.ReachableCells.Num() > 0 && FromA.ImmediateThreat.Num() > 0)) { return false; }

	// 2 — LA CELLA NASCOSTA E' RAGGIUNGIBILE. E' la controprova che rende mordente la parte 3: se l'unita'
	// nascosta trapelasse come ostacolo, questa cella sparirebbe, e la differenza sarebbe osservabile.
	if (!TestTrue(TEXT("la cella dell'unita' nascosta risulta raggiungibile"),
		Has(FromA.ReachableCells, HiddenCell))) { return false; }

	// 3 — IL CANALE LATERALE. Due stati autorevoli diversi, stessa conoscenza autorizzata: output identico.
	TestTrue(TEXT("la seconda scena risponde"),
		URTEnemyTacticalQueryLibrary::RegionsFor(Map, ViewB, /*Subject*/ 7, Hero, FromB));

	TestEqual(TEXT("un'unita' nascosta in piu' non cambia il raggiungibile"),
		FromB.ReachableCells.Num(), FromA.ReachableCells.Num());
	TestEqual(TEXT("ne' la minaccia immediata"), FromB.ImmediateThreat.Num(), FromA.ImmediateThreat.Num());
	TestEqual(TEXT("ne' la minaccia post-scatto"), FromB.PostDashThreat.Num(), FromA.PostDashThreat.Num());

	// I conteggi da soli lascerebbero passare due insiemi diversi della stessa cardinalita': il confronto
	// che conta e' cella per cella, su tutte e tre le regioni.
	TestTrue(TEXT("il raggiungibile e' identico cella per cella"),
		SameCells(FromB.ReachableCells, FromA.ReachableCells));
	TestTrue(TEXT("la minaccia immediata e' identica cella per cella"),
		SameCells(FromB.ImmediateThreat, FromA.ImmediateThreat));
	TestTrue(TEXT("la minaccia post-scatto e' identica cella per cella"),
		SameCells(FromB.PostDashThreat, FromA.PostDashThreat));

	// 4 — L'UNITA' NASCOSTA NON E' INTERROGABILE. Non ha voce nella vista, quindi non ha regioni.
	FRTEnemyTacticalRegions Hidden;
	TestFalse(TEXT("l'unita' nascosta non produce regioni"),
		URTEnemyTacticalQueryLibrary::RegionsFor(Map, ViewB, /*Subject*/ 9, Hero, Hidden));

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// 2 — Nessuna regione, non una regione vuota. E il non-esito e' indistinguibile.
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyQueryYieldsNoRegionsForUnobservedTest,
	"RefactorTactics.Perception.EnemyQueryYieldsNoRegionsForUnobserved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyQueryYieldsNoRegionsForUnobservedTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeQueryArena(4);
	URTHeroData* Hero = MeleeHero();

	FRTKnowledgeView View;
	View.ObserverTeamId = 0;
	View.Entries.Add(LiveEntry(7, FRTCellId(0, 0), Hero->HeroId));

	// Controprova di non vacuita': su un soggetto noto la funzione risponde davvero.
	FRTEnemyTacticalRegions Known;
	if (!TestTrue(TEXT("il soggetto noto produce regioni"),
		URTEnemyTacticalQueryLibrary::RegionsFor(Map, View, 7, Hero, Known))) { return false; }

	// Non osservato: `false`, e le regioni restano al DEFAULT — non vuote per svuotamento, assenti.
	FRTEnemyTacticalRegions Unobserved;
	Unobserved.ReachableCells.Add(FRTCellId(9, 9)); // sporco deliberato: la funzione deve azzerarlo
	TestFalse(TEXT("un soggetto non osservato non produce regioni"),
		URTEnemyTacticalQueryLibrary::RegionsFor(Map, View, /*Subject*/ 42, Hero, Unobserved));
	TestEqual(TEXT("e l'uscita non conserva nulla di precedente"), Unobserved.ReachableCells.Num(), 0);
	TestEqual(TEXT("nessuna minaccia immediata"), Unobserved.ImmediateThreat.Num(), 0);
	TestEqual(TEXT("nessuna minaccia post-scatto"), Unobserved.PostDashThreat.Num(), 0);
	TestEqual(TEXT("e nessuna identita' dichiarata"), Unobserved.StableUnitId, (int32)INDEX_NONE);

	// 🔴 Indistinguibilita': «non esiste» e «non lo vedo» danno lo STESSO esito. Un reason code che li
	// separasse direbbe all'osservatore che quell'unita' esiste.
	FRTEnemyTacticalRegions Nonexistent;
	const bool bUnobserved = URTEnemyTacticalQueryLibrary::RegionsFor(Map, View, 42, Hero, Unobserved);
	const bool bNonexistent = URTEnemyTacticalQueryLibrary::RegionsFor(Map, View, -1, Hero, Nonexistent);
	TestEqual(TEXT("unita' inesistente e unita' non vista sono indistinguibili"), bNonexistent, bUnobserved);

	// Il profilo deve corrispondere alla voce autorizzata: non si interroga un eroe con la scheda di un altro.
	URTHeroData* Impostor = MakeQueryHero(TEXT("Hero.SomeoneElse"), 12,
		{ MakeQueryAction(TEXT("Action.BasicAttack"), ERTAbilityShape::Single, 4) });
	FRTEnemyTacticalRegions Swapped;
	TestFalse(TEXT("un profilo di un altro eroe e' rifiutato"),
		URTEnemyTacticalQueryLibrary::RegionsFor(Map, View, 7, Impostor, Swapped));

	// Fail-closed sugli ingressi mancanti: senza mappa non si sa cosa ci sia davanti.
	FRTEnemyTacticalRegions NoMap;
	TestFalse(TEXT("senza mappa non si risponde"),
		URTEnemyTacticalQueryLibrary::RegionsFor(nullptr, View, 7, Hero, NoMap));
	FRTEnemyTacticalRegions NoHero;
	TestFalse(TEXT("senza profilo non si risponde"),
		URTEnemyTacticalQueryLibrary::RegionsFor(Map, View, 7, nullptr, NoHero));

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// 3 — Ordine StableLess, indipendente dall'ordine dell'ingresso.
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyQueryRegionsAreStablyOrderedTest,
	"RefactorTactics.Perception.EnemyQueryRegionsAreStablyOrdered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyQueryRegionsAreStablyOrderedTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeQueryArena(5);
	URTHeroData* Hero = MakeQueryHero(TEXT("Hero.QueryProbe"), 3,
		{
			MakeQueryAction(TEXT("Action.BasicAttack"), ERTAbilityShape::Single, /*AttackRange*/ 2),
			MakeQueryAction(TEXT("Action.Reposition"))
		});

	FRTKnowledgeView Forward;
	Forward.ObserverTeamId = 0;
	Forward.Entries.Add(LiveEntry(3, FRTCellId(1, 1), TEXT("Hero.Ally")));
	Forward.Entries.Add(LiveEntry(7, FRTCellId(0, 0), Hero->HeroId));
	Forward.Entries.Add(LiveEntry(9, FRTCellId(-1, 2), TEXT("Hero.Ally")));

	// La stessa conoscenza con le voci in ordine inverso: la vista NON e' ordinata per contratto qui, ed e'
	// il caso da difendere — chi la compone potrebbe non passare da `ViewForTeam`.
	FRTKnowledgeView Reversed;
	Reversed.ObserverTeamId = 0;
	for (int32 i = Forward.Entries.Num() - 1; i >= 0; --i) { Reversed.Entries.Add(Forward.Entries[i]); }

	FRTEnemyTacticalRegions A;
	FRTEnemyTacticalRegions B;
	if (!TestTrue(TEXT("prima permutazione"),
		URTEnemyTacticalQueryLibrary::RegionsFor(Map, Forward, 7, Hero, A))) { return false; }
	if (!TestTrue(TEXT("seconda permutazione"),
		URTEnemyTacticalQueryLibrary::RegionsFor(Map, Reversed, 7, Hero, B))) { return false; }

	// Non vacuita': tre regioni popolate, altrimenti «ordinate» e «identiche» sono vere per assenza.
	if (!TestTrue(TEXT("le tre regioni sono popolate"),
		A.ReachableCells.Num() > 1 && A.ImmediateThreat.Num() > 1 && A.PostDashThreat.Num() > 1))
	{
		return false;
	}

	TestTrue(TEXT("raggiungibile ordinato StableLess e senza duplicati"), IsStablySorted(A.ReachableCells));
	TestTrue(TEXT("minaccia immediata ordinata StableLess e senza duplicati"), IsStablySorted(A.ImmediateThreat));
	TestTrue(TEXT("minaccia post-scatto ordinata StableLess e senza duplicati"), IsStablySorted(A.PostDashThreat));

	TestTrue(TEXT("permutare l'ingresso non cambia il raggiungibile"),
		SameCells(B.ReachableCells, A.ReachableCells));
	TestTrue(TEXT("ne' la minaccia immediata"), SameCells(B.ImmediateThreat, A.ImmediateThreat));
	TestTrue(TEXT("ne' la minaccia post-scatto"), SameCells(B.PostDashThreat, A.PostDashThreat));

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// 4 — `PostDashThreat` ammette SOLO origini di mobilita' rapida. Anti-vacuita' per mutazione.
// ---------------------------------------------------------------------------------------------------------

/**
 * 🔴 **Il test e' costruito perche' una mutazione lo faccia cadere.** Il raggiungibile del Move normale
 * (5 MP) e' STRETTAMENTE piu' ampio del riposizionamento (2 celle in linea): se qualcuno rendesse
 * `Action.Move` un'origine ammessa — togliendo il filtro `IsFastMovement` — la cella a distanza 5
 * entrerebbe in `PostDashThreat` e questo test diventerebbe rosso.
 *
 * ⚠️ Senza quel divario il test passerebbe anche col filtro rimosso, ed e' il modo in cui un criterio
 * anti-vacuita' si soddisfa sulla carta. Il divario e' il test.
 *
 * Il ruleset che lo motiva: `URTCatalogLibrary::MapResolutionPhase` manda `NormalMovement` in
 * `ERTMatchPhase::Move`, che risolve DOPO il Blast. Arrivare li' non abilita nessun attacco nel turno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyQueryPostDashUsesOnlyFastMovementTest,
	"RefactorTactics.Perception.EnemyQueryPostDashUsesOnlyFastMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyQueryPostDashUsesOnlyFastMovementTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeQueryArena(8);

	// Il catalogo spedito, non una copia: Move e' NormalMovement, Reposition e' FastMovement lineare a 2.
	const FRTActionDef MoveDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Move"));
	const FRTActionDef DashDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Reposition"));
	if (!TestFalse(TEXT("Action.Move NON e' mobilita' rapida"), URTCatalogLibrary::IsFastMovement(MoveDef))) { return false; }
	if (!TestTrue(TEXT("Action.Reposition E' mobilita' rapida"), URTCatalogLibrary::IsFastMovement(DashDef))) { return false; }

	URTHeroData* Hero = MakeQueryHero(TEXT("Hero.QueryProbe"), /*MovePoints*/ 5,
		{
			MakeQueryAction(TEXT("Action.BasicAttack"), ERTAbilityShape::Single, /*AttackRange*/ 1),
			MakeQueryAction(TEXT("Action.Move")),
			MakeQueryAction(TEXT("Action.Reposition"))
		});

	FRTKnowledgeView View;
	View.ObserverTeamId = 0;
	View.Entries.Add(LiveEntry(7, FRTCellId(0, 0), Hero->HeroId));

	FRTEnemyTacticalRegions R;
	if (!TestTrue(TEXT("il soggetto risponde"),
		URTEnemyTacticalQueryLibrary::RegionsFor(Map, View, 7, Hero, R))) { return false; }

	// Il divario che rende il test mordente: 5 col Move, 2 col riposizionamento.
	if (!TestTrue(TEXT("il Move normale arriva a distanza 5"), Has(R.ReachableCells, FRTCellId(5, 0)))) { return false; }

	// Non vacuita' del ramo rapido: da un'origine a distanza 2 l'attacco a portata 1 raggiunge la 3.
	if (!TestTrue(TEXT("il riposizionamento minaccia fino a distanza 3"),
		Has(R.PostDashThreat, FRTCellId(3, 0)))) { return false; }

	// 🔴 LA MUTAZIONE: con `Action.Move` ammesso, la 5 sarebbe un'origine e finirebbe qui dentro.
	TestFalse(TEXT("la cella raggiungibile solo col Move normale NON e' minaccia post-scatto"),
		Has(R.PostDashThreat, FRTCellId(5, 0)));
	TestFalse(TEXT("ne' lo e' la 6, che solo il Move + attacco raggiungerebbe"),
		Has(R.PostDashThreat, FRTCellId(6, 0)));

	// E il Move non contamina nemmeno la minaccia immediata: da fermo l'attacco arriva a 1.
	TestTrue(TEXT("minaccia immediata a distanza 1"), Has(R.ImmediateThreat, FRTCellId(1, 0)));
	TestFalse(TEXT("nessuna minaccia immediata a distanza 2"), Has(R.ImmediateThreat, FRTCellId(2, 0)));

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// 5 — `Action.Charge`: classificazione doppia e dichiarata.
// ---------------------------------------------------------------------------------------------------------

/**
 * La carica e' `FastMovement` **e** `bCountsAsAttack`, e occupa il solo slot Movimento. Non essendo
 * un'azione di slot principale cadrebbe fuori da entrambe le unioni: sparirebbe dall'anteprima in
 * silenzio, ed e' il difetto che questo test esiste per impedire.
 *
 * ⚠️ Pinna ENTRAMBI i lati. Verificare solo l'impronta d'impatto lascerebbe scoperta la meta' che si
 * perde per prima — che dopo una carica lo slot principale e' ancora libero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyQueryClassifiesChargeTest,
	"RefactorTactics.Perception.EnemyQueryClassifiesCharge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyQueryClassifiesChargeTest::RunTest(const FString&)
{
	// Le tre proprieta' del catalogo spedito su cui poggia la classificazione. Se una cambia, il test dice
	// QUALE — e la classificazione va ridecisa, non aggiustata.
	const FRTActionDef Charge = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge"));
	if (!TestTrue(TEXT("Action.Charge e' mobilita' rapida"), URTCatalogLibrary::IsFastMovement(Charge))) { return false; }
	if (!TestTrue(TEXT("Action.Charge conta come attacco"), Charge.bCountsAsAttack)) { return false; }
	if (!TestFalse(TEXT("Action.Charge NON occupa lo slot principale"),
		URTCatalogLibrary::TakesMainSlot(Charge))) { return false; }
	if (!TestTrue(TEXT("Action.Charge occupa lo slot movimento"),
		URTCatalogLibrary::TakesMovementSlot(Charge))) { return false; }

	URTHexMapAsset* Map = MakeQueryArena(8);
	URTHeroData* Hero = MakeQueryHero(TEXT("Hero.QueryProbe"), /*MovePoints*/ 1,
		{
			MakeQueryAction(TEXT("Action.BasicAttack"), ERTAbilityShape::Single, /*AttackRange*/ 1),
			MakeQueryAction(TEXT("Action.Charge"))
		});

	FRTKnowledgeView View;
	View.ObserverTeamId = 0;
	View.Entries.Add(LiveEntry(7, FRTCellId(0, 0), Hero->HeroId));

	FRTEnemyTacticalRegions R;
	if (!TestTrue(TEXT("il soggetto risponde"),
		URTEnemyTacticalQueryLibrary::RegionsFor(Map, View, 7, Hero, R))) { return false; }

	// LATO A — l'impronta della carica e' minaccia IMMEDIATA: parte dalla cella corrente e risolve in Dash,
	// prima del Blast. La 2 e la 3 sono fuori dalla portata 1 dell'attacco base: solo la carica le spiega.
	TestTrue(TEXT("la carica minaccia la cella a distanza 2 senza riposizionarsi"),
		Has(R.ImmediateThreat, FRTCellId(2, 0)));
	TestTrue(TEXT("e quella a distanza 3, fine corsa della carica"),
		Has(R.ImmediateThreat, FRTCellId(3, 0)));

	// LATO B — le celle d'ARRIVO della carica sono origini post-scatto: lo slot principale resta libero,
	// quindi da distanza 3 l'attacco a portata 1 raggiunge la 4.
	TestTrue(TEXT("dopo la carica lo slot principale e' ancora libero: la 4 e' minacciata"),
		Has(R.PostDashThreat, FRTCellId(4, 0)));
	TestFalse(TEXT("ma la 4 non e' minaccia immediata: richiede la carica prima"),
		Has(R.ImmediateThreat, FRTCellId(4, 0)));

	// Il movimento normale resta 1: la carica non lo estende, e le due regioni non si confondono.
	TestFalse(TEXT("la carica non allarga il raggiungibile del movimento normale"),
		Has(R.ReachableCells, FRTCellId(3, 0)));

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// 6 — Muri, porte, layer.
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyQueryRespectsWallsDoorsAndLayersTest,
	"RefactorTactics.Perception.EnemyQueryRespectsWallsDoorsAndLayers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyQueryRespectsWallsDoorsAndLayersTest::RunTest(const FString&)
{
	URTHeroData* Hero = MeleeHero(/*MovePoints*/ 3);

	FRTKnowledgeView View;
	View.ObserverTeamId = 0;
	View.Entries.Add(LiveEntry(7, FRTCellId(0, 0), Hero->HeroId));

	// --- Muro -------------------------------------------------------------------------------------------
	{
		URTHexMapAsset* Map = MakeQueryArena(4);
		FRTEnemyTacticalRegions Before;
		if (!TestTrue(TEXT("baseline"),
			URTEnemyTacticalQueryLibrary::RegionsFor(Map, View, 7, Hero, Before))) { return false; }
		if (!TestTrue(TEXT("senza muro la cella e' raggiungibile"),
			Has(Before.ReachableCells, FRTCellId(1, 0)))) { return false; }

		PutBlocker(Map, FRTCellId(1, 0));
		FRTEnemyTacticalRegions After;
		TestTrue(TEXT("con muro risponde comunque"),
			URTEnemyTacticalQueryLibrary::RegionsFor(Map, View, 7, Hero, After));
		TestFalse(TEXT("un muro toglie la cella dal raggiungibile"), Has(After.ReachableCells, FRTCellId(1, 0)));
		TestTrue(TEXT("e il raggiungibile si restringe"), After.ReachableCells.Num() < Before.ReachableCells.Num());
	}

	// --- Porta ------------------------------------------------------------------------------------------
	{
		URTHexMapAsset* Map = MakeQueryArena(4);
		// Corridoio: tutto murato tranne il varco (1,0) -> (2,0), su cui sta la porta.
		PutBlocker(Map, FRTCellId(1, -1));
		PutBlocker(Map, FRTCellId(2, -1));
		PutBlocker(Map, FRTCellId(0, 1));
		PutBlocker(Map, FRTCellId(1, 1));

		PutDoor(Map, FRTCellId(1, 0), ERTHexDirection::E, ERTHexDoorState::Open);
		FRTEnemyTacticalRegions Open;
		if (!TestTrue(TEXT("porta aperta: risponde"),
			URTEnemyTacticalQueryLibrary::RegionsFor(Map, View, 7, Hero, Open))) { return false; }
		if (!TestTrue(TEXT("porta aperta: si passa"), Has(Open.ReachableCells, FRTCellId(2, 0)))) { return false; }

		URTHexDoorLibrary::SetDoorState(Map, FRTCellId(1, 0), FRTCellId(2, 0), ERTHexDoorState::Closed);
		FRTEnemyTacticalRegions Shut;
		TestTrue(TEXT("porta chiusa: risponde"),
			URTEnemyTacticalQueryLibrary::RegionsFor(Map, View, 7, Hero, Shut));
		TestFalse(TEXT("porta chiusa: non si passa"), Has(Shut.ReachableCells, FRTCellId(2, 0)));
		TestTrue(TEXT("lo stato della porta cambia il raggiungibile"),
			Shut.ReachableCells.Num() < Open.ReachableCells.Num());
	}

	// --- Layer ------------------------------------------------------------------------------------------
	{
		URTHexMapAsset* Map = MakeQueryArena(4);
		FRTEnemyTacticalRegions R;
		TestTrue(TEXT("layer: risponde"), URTEnemyTacticalQueryLibrary::RegionsFor(Map, View, 7, Hero, R));
		TestTrue(TEXT("la cella sul layer 0 c'e'"), Has(R.ReachableCells, FRTCellId(1, 0, 0)));
		// Stesso X/Y, layer diverso: e' un'ALTRA cella, e senza arco esplicito non e' adiacente a nulla.
		TestFalse(TEXT("stesso X/Y su un layer diverso resta una cella distinta"),
			Has(R.ReachableCells, FRTCellId(1, 0, 1)));
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// 7 — Linea di tiro, e per le aree le celle INVESTITE.
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyQueryThreatRequiresLineOfSightTest,
	"RefactorTactics.Perception.EnemyQueryThreatRequiresLineOfSight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyQueryThreatRequiresLineOfSightTest::RunTest(const FString&)
{
	FRTKnowledgeView View;
	View.ObserverTeamId = 0;

	// --- LOS --------------------------------------------------------------------------------------------
	{
		URTHexMapAsset* Map = MakeQueryArena(5);
		URTHeroData* Hero = MakeQueryHero(TEXT("Hero.QueryProbe"), /*MovePoints*/ 0,
			{ MakeQueryAction(TEXT("Action.BasicAttack"), ERTAbilityShape::Single, /*AttackRange*/ 3) });

		FRTKnowledgeView Local = View;
		Local.Entries.Add(LiveEntry(7, FRTCellId(0, 0), Hero->HeroId));

		FRTEnemyTacticalRegions Clear;
		if (!TestTrue(TEXT("baseline"),
			URTEnemyTacticalQueryLibrary::RegionsFor(Map, Local, 7, Hero, Clear))) { return false; }
		if (!TestTrue(TEXT("in campo libero la 3 e' minacciata"),
			Has(Clear.ImmediateThreat, FRTCellId(3, 0)))) { return false; }

		// Ostacolo alla VISTA, non al passo: separa i due predicati. La 3 non e' piu' bersagliabile.
		PutSightBlocker(Map, FRTCellId(2, 0));
		FRTEnemyTacticalRegions Blocked;
		TestTrue(TEXT("con ostacolo risponde"),
			URTEnemyTacticalQueryLibrary::RegionsFor(Map, Local, 7, Hero, Blocked));
		TestFalse(TEXT("la LOS bloccata toglie la cella dalla minaccia"),
			Has(Blocked.ImmediateThreat, FRTCellId(3, 0)));
		TestTrue(TEXT("mentre la cella davanti all'ostacolo resta minacciata"),
			Has(Blocked.ImmediateThreat, FRTCellId(1, 0)));
	}

	// --- Area: celle INVESTITE, non centri bersagliabili -------------------------------------------------
	{
		URTHexMapAsset* Map = MakeQueryArena(6);
		URTHeroData* Hero = MakeQueryHero(TEXT("Hero.QueryProbe"), /*MovePoints*/ 0,
			{ MakeQueryAction(TEXT("Action.CircularAoE"), ERTAbilityShape::Area, /*AttackRange*/ 3, /*AreaRadius*/ 1) });

		FRTKnowledgeView Local = View;
		Local.Entries.Add(LiveEntry(7, FRTCellId(0, 0), Hero->HeroId));

		FRTEnemyTacticalRegions R;
		if (!TestTrue(TEXT("l'area risponde"),
			URTEnemyTacticalQueryLibrary::RegionsFor(Map, Local, 7, Hero, R))) { return false; }

		TestTrue(TEXT("il centro a portata e' investito"), Has(R.ImmediateThreat, FRTCellId(3, 0)));
		// 🔴 La cella oltre la portata di MIRA, investita dal raggio dell'esplosione: e' la differenza fra
		// mostrare i centri e mostrare cio' che si subisce.
		TestTrue(TEXT("la cella a distanza 4 e' investita, pur non essendo bersagliabile"),
			Has(R.ImmediateThreat, FRTCellId(4, 0)));
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// 8 — Classificazione dei campi del DTO (DoD G8, disciplina di #2331).
// ---------------------------------------------------------------------------------------------------------

namespace
{
	/** Classe di privacy di un campo di `FRTEnemyTacticalRegions`. Oggi una sola: la struttura E' la risposta. */
	enum class ERTEnemyRegionFieldClass : uint8
	{
		/** Autorizzato per costruzione: deriva solo da cio' che l'osservatore ha diritto di sapere. */
		Public
	};

	struct FRTEnemyRegionFieldRow
	{
		FName Name;
		ERTEnemyRegionFieldClass Class;
	};

	/**
	 * ⚠️ In un array e non direttamente in una `TMap`, per la ragione che `RTIntentPrivacyTests.cpp` ha gia'
	 * pagato: una `TMap` da initializer list INGOIA una chiave duplicata, e l'ultima riga vince in silenzio.
	 *
	 * `GET_MEMBER_NAME_CHECKED` e non un `FName` letterale: un campo RINOMINATO deve rompere la
	 * COMPILAZIONE, non lasciare qui un nome che non esiste piu'.
	 */
	const TArray<FRTEnemyRegionFieldRow>& EnemyRegionFieldRows()
	{
		static const TArray<FRTEnemyRegionFieldRow> Rows = {
			{ GET_MEMBER_NAME_CHECKED(FRTEnemyTacticalRegions, StableUnitId),    ERTEnemyRegionFieldClass::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTEnemyTacticalRegions, ReachableCells),  ERTEnemyRegionFieldClass::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTEnemyTacticalRegions, ImmediateThreat), ERTEnemyRegionFieldClass::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTEnemyTacticalRegions, PostDashThreat),  ERTEnemyRegionFieldClass::Public }
		};
		return Rows;
	}
}

/**
 * Ogni `UPROPERTY` del DTO deve avere una classe di privacy DICHIARATA, e la reflection non deve contenere
 * campi che la tabella non nomina.
 *
 * 🔑 **Misura i campi, non i valori.** I sette test sopra pinnano il comportamento dei quattro campi di
 * oggi; nessuno di essi e' il meccanismo che accorgerebbe di un quinto campo aggiunto domani. E' la stessa
 * lacuna che `#2331` ha chiuso su `FRTIntentView`, dove quattro guardie erano corrette *per fortuna, non
 * per costruzione*.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyTacticalRegionsFieldsAreClassifiedTest,
	"RefactorTactics.Perception.EnemyTacticalRegionsFieldsAreClassified",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyTacticalRegionsFieldsAreClassifiedTest::RunTest(const FString&)
{
	const TArray<FRTEnemyRegionFieldRow>& Rows = EnemyRegionFieldRows();

	TMap<FName, ERTEnemyRegionFieldClass> Table;
	for (const FRTEnemyRegionFieldRow& Row : Rows)
	{
		Table.Add(Row.Name, Row.Class);
	}
	// Un duplicato si perde nella TMap: si conta, non si spera.
	TestEqual(TEXT("nessun campo classificato due volte"), Table.Num(), Rows.Num());

	TSet<FName> Reflected;
	for (TFieldIterator<FProperty> It(FRTEnemyTacticalRegions::StaticStruct()); It; ++It)
	{
		Reflected.Add(It->GetFName());
	}

	// ANTI-VACUITA': una tabella vuota, o una reflection che non vede niente, renderebbe verdi per assenza
	// di soggetto tutti i confronti qui sotto.
	if (!TestTrue(TEXT("la tabella classifica almeno un campo"), Table.Num() > 0)) { return false; }
	if (!TestTrue(TEXT("la reflection vede almeno un campo"), Reflected.Num() > 0)) { return false; }

	for (const FName& Field : Reflected)
	{
		TestTrue(FString::Printf(TEXT("il campo %s ha una classe dichiarata"), *Field.ToString()),
			Table.Contains(Field));
	}
	for (const FRTEnemyRegionFieldRow& Row : Rows)
	{
		TestTrue(FString::Printf(TEXT("il campo classificato %s esiste ancora"), *Row.Name.ToString()),
			Reflected.Contains(Row.Name));
	}
	TestEqual(TEXT("tabella e reflection coprono lo stesso insieme"), Table.Num(), Reflected.Num());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
