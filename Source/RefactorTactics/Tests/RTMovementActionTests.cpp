#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTMovementActionLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Esagono pieno di raggio N, tutte le celle percorribili. Nome distinto per file (unity build). */
	URTHexMapAsset* MakeMoveActionMap(int32 Radius = 6)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}

	/** Trasforma una cella in muro: blocca il passo e la vista. */
	void MakeWall(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		FRTHexCellData Data = Map->FindCell(Id) ? *Map->FindCell(Id) : FRTHexCellData(Id);
		Data.bBlocksMovement = true;
		Data.bBlocksLineOfSight = true;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDashBlockedArcTest,
	"RefactorTactics.Actions.Dash.BlockedArc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDashBlockedArcTest::RunTest(const FString&)
{
	// Nome vincolante del catalogo v0.1 §15: lo scatto e' interrotto da un muro sulla traiettoria. E' la
	// differenza fra una mobilita' LINEARE e il movimento a budget: con il pathfinding il muro si aggirerebbe,
	// e non ci sarebbe alcun arco bloccato da osservare.
	URTHexMapAsset* Map = MakeMoveActionMap();
	MakeWall(Map, FRTCellId(2, 0));

	const TMap<FRTCellId, int32> Empty;
	const TSet<int32> NoHostiles;

	const FRTLinearMoveResult Blocked = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(3, 0), /*MaxCells*/ 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);

	TestTrue(TEXT("si ferma PRIMA del muro"), Blocked.Final == FRTCellId(1, 0));
	TestTrue(TEXT("e il motivo e' il terreno, non un'unita'"), Blocked.Stop == ERTLinearStop::BlockedByTerrain);
	TestEqual(TEXT("ha attraversato una sola cella"), Blocked.Entered.Num(), 1);

	// Senza muro, lo stesso scatto arriva a destinazione.
	URTHexMapAsset* Clear = MakeMoveActionMap();
	const FRTLinearMoveResult Free = URTMovementActionLibrary::ResolveLinearMove(
		Clear, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);
	TestTrue(TEXT("via libera: arriva"), Free.Final == FRTCellId(3, 0) && Free.Stop == ERTLinearStop::Completed);

	// Un'unita' sulla traiettoria ferma lo scatto tanto quanto un muro, ma con un motivo diverso.
	TMap<FRTCellId, int32> Occupied;
	Occupied.Add(FRTCellId(2, 0), 7);
	const FRTLinearMoveResult Crowded = URTMovementActionLibrary::ResolveLinearMove(
		Clear, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearDash, Occupied, NoHostiles);
	TestTrue(TEXT("si ferma davanti all'unita'"), Crowded.Final == FRTCellId(1, 0));
	TestTrue(TEXT("col motivo giusto"), Crowded.Stop == ERTLinearStop::BlockedByUnit);

	// Lo scatto NON e' un percorso: una destinazione obliqua non e' su una delle sei direzioni e l'azione non
	// parte, invece di cercarsi una strada.
	const FRTLinearMoveResult Oblique = URTMovementActionLibrary::ResolveLinearMove(
		Clear, FRTCellId(0, 0), FRTCellId(2, 1), 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);
	TestTrue(TEXT("destinazione non allineata: non si muove"), Oblique.Final == FRTCellId(0, 0));
	TestTrue(TEXT("e lo dichiara"), Oblique.Stop == ERTLinearStop::NotAligned);

	// Oltre la portata dichiarata non si va, nemmeno in linea retta.
	const FRTLinearMoveResult TooFar = URTMovementActionLibrary::ResolveLinearMove(
		Clear, FRTCellId(0, 0), FRTCellId(5, 0), 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);
	TestTrue(TEXT("fuori portata: non parte"), TooFar.Final == FRTCellId(0, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTChargeStopsOnImpactTest,
	"RefactorTactics.Actions.Charge.StopsOnImpact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTChargeStopsOnImpactTest::RunTest(const FString&)
{
	// La carica si arresta ADDOSSO al primo nemico incontrato e lo segnala: il colpo (20 danni + spinta 1) lo
	// applica poi il Blast, perche' l'impatto e' controllo — codice 20/30 del catalogo.
	URTHexMapAsset* Map = MakeMoveActionMap();

	TMap<FRTCellId, int32> Occupancy;
	Occupancy.Add(FRTCellId(2, 0), /*UnitId*/ 4);
	TSet<int32> Hostiles;
	Hostiles.Add(4);

	const FRTLinearMoveResult Impact = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(3, 0), /*MaxCells*/ 3, ERTMovementStyle::LinearCharge, Occupancy, Hostiles);

	TestTrue(TEXT("si ferma davanti al nemico, non lo attraversa"), Impact.Final == FRTCellId(1, 0));
	TestTrue(TEXT("l'esito e' l'impatto, non un blocco qualsiasi"), Impact.Stop == ERTLinearStop::Impact);
	TestEqual(TEXT("e dice CHI ha urtato"), Impact.ImpactUnitId, 4);

	// Un ALLEATO sulla traiettoria non e' un bersaglio: la carica si ferma, ma non colpisce nessuno.
	TSet<int32> NoHostiles;
	const FRTLinearMoveResult Friend = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearCharge, Occupancy, NoHostiles);
	TestTrue(TEXT("davanti a un alleato ci si ferma senza impatto"), Friend.Stop == ERTLinearStop::BlockedByUnit);
	TestEqual(TEXT("nessuno da colpire"), Friend.ImpactUnitId, static_cast<int32>(INDEX_NONE));

	// Senza nessuno sulla strada, la carica e' uno scatto come gli altri.
	const TMap<FRTCellId, int32> Empty;
	const FRTLinearMoveResult Missed = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearCharge, Empty, Hostiles);
	TestTrue(TEXT("a vuoto: arriva e non colpisce"),
		Missed.Final == FRTCellId(3, 0) && Missed.ImpactUnitId == INDEX_NONE);

	// Un muro interrompe la carica prima del bersaglio: niente impatto attraverso la parete.
	URTHexMapAsset* Walled = MakeMoveActionMap();
	MakeWall(Walled, FRTCellId(1, 0));
	const FRTLinearMoveResult Wall = URTMovementActionLibrary::ResolveLinearMove(
		Walled, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearCharge, Occupancy, Hostiles);
	TestTrue(TEXT("il muro ferma la carica"), Wall.Stop == ERTLinearStop::BlockedByTerrain);
	TestEqual(TEXT("e non c'e' impatto"), Wall.ImpactUnitId, static_cast<int32>(INDEX_NONE));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLeapIgnoresIntermediateTest,
	"RefactorTactics.Actions.Leap.IgnoresIntermediateCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLeapIgnoresIntermediateTest::RunTest(const FString&)
{
	// Il salto scavalca cio' che sta in mezzo — unita' e coperture basse — ma la cella d'atterraggio la
	// SUBISCE: dev'essere percorribile e libera.
	URTHexMapAsset* Map = MakeMoveActionMap();

	TMap<FRTCellId, int32> Occupancy;
	Occupancy.Add(FRTCellId(1, 0), 2);
	Occupancy.Add(FRTCellId(2, 0), 3);
	const TSet<int32> NoHostiles;

	const FRTLinearMoveResult Over = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(3, 0), /*MaxCells*/ 3, ERTMovementStyle::LinearLeap, Occupancy, NoHostiles);

	TestTrue(TEXT("atterra oltre le due unita'"), Over.Final == FRTCellId(3, 0));
	TestTrue(TEXT("l'esito e' completato"), Over.Stop == ERTLinearStop::Completed);
	TestEqual(TEXT("nessuna cella intermedia attraversata: solo l'atterraggio"), Over.Entered.Num(), 1);

	// Confronto che rende esplicita la differenza: uno scatto normale, sulla stessa mappa, non passa.
	const FRTLinearMoveResult Dash = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearDash, Occupancy, NoHostiles);
	TestTrue(TEXT("lo scatto invece si ferma subito"), Dash.Final == FRTCellId(0, 0));

	// Atterraggio occupato: il salto non parte (non si atterra addosso a qualcuno).
	TMap<FRTCellId, int32> Landing = Occupancy;
	Landing.Add(FRTCellId(3, 0), 5);
	const FRTLinearMoveResult Busy = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearLeap, Landing, NoHostiles);
	TestTrue(TEXT("atterraggio occupato: si resta"), Busy.Final == FRTCellId(0, 0));
	TestTrue(TEXT("col motivo giusto"), Busy.Stop == ERTLinearStop::BlockedByUnit);

	// Atterraggio dentro un muro: idem, e il motivo e' il terreno.
	URTHexMapAsset* Walled = MakeMoveActionMap();
	MakeWall(Walled, FRTCellId(3, 0));
	const FRTLinearMoveResult IntoWall = URTMovementActionLibrary::ResolveLinearMove(
		Walled, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearLeap, Occupancy, NoHostiles);
	TestTrue(TEXT("non si atterra dentro un muro"), IntoWall.Stop == ERTLinearStop::BlockedByTerrain);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSprintNoReactionTest,
	"RefactorTactics.Actions.Sprint.NoReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSprintNoReactionTest::RunTest(const FString&)
{
	// Chi corre a perdifiato non para: `Sprint` e' l'unica mobilita' della v0.1 che nega la reazione, e lo
	// dichiara nei DATI. A farla valere sara' E5, che introduce lo slot reazione; qui si fissa la regola.
	const FRTActionDef Sprint = URTCatalogLibrary::FindCoreAction(TEXT("Action.Sprint"));
	if (!TestTrue(TEXT("Action.Sprint e' nel catalogo"), Sprint.ActionId == FName(TEXT("Action.Sprint"))))
	{
		return false;
	}
	TestFalse(TEXT("lo Sprint non consente reazioni"), Sprint.bAllowsReaction);

	// Le altre mobilita' invece la consentono: la differenza e' il punto: se tutte la negassero, il dato non
	// distinguerebbe nulla.
	const TCHAR* Others[] = { TEXT("Action.Dash"), TEXT("Action.Charge"), TEXT("Action.Leap"),
		TEXT("Action.Reposition"), TEXT("Action.Move"), TEXT("Action.Wait") };
	for (const TCHAR* Id : Others)
	{
		const FRTActionDef Def = URTCatalogLibrary::FindCoreAction(FName(Id));
		TestTrue(FString::Printf(TEXT("%s consente la reazione"), Id), Def.bAllowsReaction);
	}

	// E dopo D-028 negare la reazione e' rimasto il suo prezzo PRINCIPALE, non uno in piu': lo Sprint occupa
	// il solo slot movimento come `Move`, quindi cio' che lo distingue da 3 MP in piu' e' esattamente questa
	// riga piu' `Status.Exposed`. Se al playtest non basta, e' un `Move` migliore — l'upgrade puro che D-015
	// vieta (voce `BAL-1` del backlog).
	TestTrue(TEXT("Sprint spende il solo movimento"), Sprint.Slot == ERTActionSlot::Movement);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementCatalogTest,
	"RefactorTactics.Actions.MovementActionsDeclareStyleAndPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementCatalogTest::RunTest(const FString&)
{
	// Ogni mobilita' dichiara IN DATI la macro-fase (ADR-0003 §3) e il modo in cui si sposta. Le cinque
	// mobilita' speciali risolvono nel Dash; solo `Action.Move` risolve nel Move, dopo il Blast.
	struct FExpected { const TCHAR* Id; ERTMatchPhase Macro; ERTMovementStyle Style; int32 Range; int32 Cooldown; };
	const FExpected Expected[] = {
		{ TEXT("Action.Sprint"),     ERTMatchPhase::Dash, ERTMovementStyle::Budget,       8, 0 },
		{ TEXT("Action.Dash"),       ERTMatchPhase::Dash, ERTMovementStyle::LinearDash,   3, 1 },
		{ TEXT("Action.Charge"),     ERTMatchPhase::Dash, ERTMovementStyle::LinearCharge, 3, 2 },
		{ TEXT("Action.Leap"),       ERTMatchPhase::Dash, ERTMovementStyle::LinearLeap,   3, 2 },
		{ TEXT("Action.Reposition"), ERTMatchPhase::Dash, ERTMovementStyle::LinearDash,   2, 1 },
		{ TEXT("Action.Move"),       ERTMatchPhase::Move, ERTMovementStyle::Budget,       5, 0 },
	};

	for (const FExpected& E : Expected)
	{
		const FRTActionDef Def = URTCatalogLibrary::FindCoreAction(FName(E.Id));
		if (!TestTrue(FString::Printf(TEXT("%s e' nel catalogo"), E.Id), Def.ActionId == FName(E.Id))) { continue; }

		TestTrue(FString::Printf(TEXT("%s: macro-fase dichiarata"), E.Id),
			URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase) == E.Macro);
		TestTrue(FString::Printf(TEXT("%s: stile di movimento"), E.Id), Def.MovementStyle == E.Style);
		TestEqual(FString::Printf(TEXT("%s: portata"), E.Id), Def.RangeCells, E.Range);
		TestEqual(FString::Printf(TEXT("%s: cooldown"), E.Id), Def.CooldownTurns, E.Cooldown);
		TestTrue(FString::Printf(TEXT("%s: fallback Stop (ci si ferma, non si annulla)"), E.Id),
			Def.Fallback == ERTActionFallback::Stop);
	}

	// La carica porta con se' i suoi effetti: 20 danni e una spinta di 1, dichiarati come dati.
	const FRTActionDef Charge = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge"));
	TestEqual(TEXT("la carica dichiara due effetti"), Charge.Effects.Num(), 2);
	if (Charge.Effects.Num() == 2)
	{
		TestTrue(TEXT("20 danni"), Charge.Effects[0].Effect == ERTActionEffect::Damage && Charge.Effects[0].Amount == 20);
		TestTrue(TEXT("e una spinta di 1"), Charge.Effects[1].Effect == ERTActionEffect::Push && Charge.Effects[1].Amount == 1);
	}

	// `Reposition` non applica `Exposed`: e' la differenza con lo Sprint, oltre alla distanza.
	TestEqual(TEXT("Reposition non applica stati"),
		URTCatalogLibrary::FindCoreAction(TEXT("Action.Reposition")).Effects.Num(), 0);

	// Solo le mobilita' lineari passano da ResolveLinearMove.
	TestTrue(TEXT("il budget non e' lineare"), !URTMovementActionLibrary::IsLinear(ERTMovementStyle::Budget));
	TestTrue(TEXT("lo scatto lo e'"), URTMovementActionLibrary::IsLinear(ERTMovementStyle::LinearDash));
	return true;
}

/**
 * Guardia contro il buco di #142: `ERTMovementStyle` ha `None` come default, quindi un'azione di mobilita'
 * che si dimentica di dichiararlo NON e' un errore di compilazione — e' uno scatto che, in silenzio, torna a
 * usare il pathfinding del movimento normale (aggira gli ostacoli, attraversa `Rough`, sale di layer).
 *
 * La regola vale per ENTRAMBI i cataloghi rimasti — quello generico (`Action.*`) e quello degli EROI. Dal
 * 2026-08-10 non ce n'e' un terzo: il catalogo dei due archetipi legacy e' stato rimosso, e con esso l'unico
 * percorso di configurazione che nessuna partita esercitava. `URTHeroData` e' cio' che il GameMode schiera
 * davvero — se la guardia guardasse altrove sarebbe verde mentre il buco e' aperto proprio dove il gioco passa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFastMovementDeclaresStyleTest,
	"RefactorTactics.Actions.EveryFastMovementDeclaresStyle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFastMovementDeclaresStyleTest::RunTest(const FString&)
{
	TArray<FRTActionDef> All = URTCatalogLibrary::GetCoreActionCatalog();
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Hero) { continue; }
		for (const URTActionData* Action : Hero->Actions)
		{
			if (Action) { All.Add(Action->Def); }
		}
	}

	int32 FastMovers = 0;
	for (const FRTActionDef& Def : All)
	{
		if (URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase) != ERTMatchPhase::Dash)
		{
			continue;
		}
		++FastMovers;
		TestTrue(FString::Printf(TEXT("%s risolve nel Dash: deve dichiarare COME si sposta"), *Def.ActionId.ToString()),
			Def.MovementStyle != ERTMovementStyle::None);
	}

	// Se il filtro non trovasse nulla, il test passerebbe senza provare niente.
	TestTrue(TEXT("il catalogo contiene mobilita' rapide da verificare"), FastMovers > 0);

	// Le mobilita' rapide sono LINEARI: e' il comportamento che CP 4.5 ha introdotto e che il default
	// `None` stava annullando. Le due asserzioni stavano su `Ranger.Dash` e `Guardian.Charge`, cioe' sul
	// catalogo dei due archetipi legacy, rimosso il 2026-08-10: verificavano lo stile su azioni che nessuna
	// partita spediva. Ora stanno sul catalogo generico e sul roster, che sono cio' che il gioco usa.
	const FRTActionDef CoreDash = URTCatalogLibrary::FindCoreAction(TEXT("Action.Dash"));
	TestTrue(TEXT("Action.Dash e' uno scatto lineare"), CoreDash.MovementStyle == ERTMovementStyle::LinearDash);

	const FRTActionDef CoreCharge = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge"));
	TestTrue(TEXT("Action.Charge e' una carica"), CoreCharge.MovementStyle == ERTMovementStyle::LinearCharge);

	// Una carica si ferma ADDOSSO al nemico: senza effetti dichiarati l'impatto sarebbe un contatto da zero
	// danni, cioe' una carica che non carica.
	TestEqual(TEXT("Action.Charge dichiara due effetti"), CoreCharge.Effects.Num(), 2);
	if (CoreCharge.Effects.Num() == 2)
	{
		TestTrue(TEXT("20 danni d'impatto"),
			CoreCharge.Effects[0].Effect == ERTActionEffect::Damage && CoreCharge.Effects[0].Amount == 20);
		TestTrue(TEXT("e una spinta di 1"),
			CoreCharge.Effects[1].Effect == ERTActionEffect::Push && CoreCharge.Effects[1].Amount == 1);
	}

	// L'eroe che quella carica la porta in partita: `Bastion.Ram` riusa `Action.Charge` per intero, ed e'
	// il motivo per cui lo stile e' un dato e non un `if` sull'ActionId.
	const URTHeroData* Bastion = nullptr;
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (Hero && Hero->HeroId == TEXT("Hero.Bastion")) { Bastion = Hero; break; }
	}
	if (TestNotNull(TEXT("Bastion e' nel roster"), Bastion))
	{
		const URTActionData* Ram = nullptr;
		for (const URTActionData* A : Bastion->Actions)
		{
			if (A && A->Def.ActionId == TEXT("Bastion.Ram")) { Ram = A; break; }
		}
		if (TestNotNull(TEXT("Bastion.Ram e' nel kit"), Ram))
		{
			TestTrue(TEXT("Bastion.Ram e' una carica"), Ram->Def.MovementStyle == ERTMovementStyle::LinearCharge);
		}
	}
	return true;
}

// =====================================================================================================
// MOB-1 — la lama che PASSA ATTRAVERSO.
//
// `LinearDash` si ferma su cio' che incontra, `LinearCharge` si ferma addosso, `LinearLeap` scavalca senza
// toccare. Manca la quarta: attraversare colpendo. `Vektor.PassingBlade` la descriveva gia' («colpisce per
// 20 le unita' ATTRAVERSATE») ma usava `LinearDash`, quindi non attraversava nessuno e quei 20 danni non
// avevano un momento in cui applicarsi.
//
// Stile NUOVO invece di cambiare `LinearDash`: quello e' condiviso con `Action.Dash` e `Action.Reposition`,
// dove fermarsi davanti a un nemico e' il comportamento giusto. Cambiarlo li' avrebbe fatto passare ogni
// scatto attraverso le linee avversarie.
// =====================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLinearPassTest,
	"RefactorTactics.Movement.PassGoesThroughUnitsAndRecordsThem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLinearPassTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeMoveActionMap();

	// Due unita' sulla traiettoria fra (0,0) e (4,0): 7 e 9, in celle diverse.
	TMap<FRTCellId, int32> Occupancy;
	Occupancy.Add(FRTCellId(1, 0, 0), 7);
	Occupancy.Add(FRTCellId(3, 0, 0), 9);
	const TSet<int32> Hostiles = { 7, 9 };

	const FRTLinearMoveResult Pass = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0, 0), FRTCellId(4, 0, 0), /*MaxCells*/ 4,
		ERTMovementStyle::LinearPass, Occupancy, Hostiles);

	TestTrue(TEXT("la lama arriva a destinazione"), Pass.Final == FRTCellId(4, 0, 0));
	TestEqual(TEXT("e lo dichiara come arrivo, non come blocco"),
		static_cast<int32>(Pass.Stop), static_cast<int32>(ERTLinearStop::Completed));
	TestEqual(TEXT("ha attraversato ENTRAMBE le unita'"), Pass.PassedThroughUnitIds.Num(), 2);
	TestTrue(TEXT("e sa quali"),
		Pass.PassedThroughUnitIds.Contains(7) && Pass.PassedThroughUnitIds.Contains(9));

	// Il confronto che rende il test discriminante: lo STESSO percorso con lo stile vecchio si ferma subito.
	const FRTLinearMoveResult Dash = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0, 0), FRTCellId(4, 0, 0), /*MaxCells*/ 4,
		ERTMovementStyle::LinearDash, Occupancy, Hostiles);
	TestTrue(TEXT("lo scatto normale invece si ferma davanti alla prima"), Dash.Final == FRTCellId(0, 0, 0));
	TestEqual(TEXT("e non attraversa nessuno"), Dash.PassedThroughUnitIds.Num(), 0);

	// La cella d'ARRIVO resta un vincolo: si passa ATTRAVERSO, non ci si ferma dentro qualcuno.
	TMap<FRTCellId, int32> Occupied;
	Occupied.Add(FRTCellId(4, 0, 0), 5);
	const FRTLinearMoveResult OntoUnit = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0, 0), FRTCellId(4, 0, 0), /*MaxCells*/ 4,
		ERTMovementStyle::LinearPass, Occupied, TSet<int32>{ 5 });
	TestFalse(TEXT("non si atterra sopra un'unita'"), OntoUnit.Final == FRTCellId(4, 0, 0));

	// Gli stessi identici parametri della partita (Probe.PassingBlade): se questo passa e lo scenario no, il
	// difetto non e' qui.
	TMap<FRTCellId, int32> Mid;
	Mid.Add(FRTCellId(0, 0, 0), 1);
	const FRTLinearMoveResult AsInMatch = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(-2, 0, 0), FRTCellId(1, 0, 0), /*MaxCells*/ 3,
		ERTMovementStyle::LinearPass, Mid, TSet<int32>{ 1 });
	TestTrue(TEXT("coi parametri della partita la lama arriva"), AsInMatch.Final == FRTCellId(1, 0, 0));
	TestEqual(TEXT("e registra chi ha attraversato"), AsInMatch.PassedThroughUnitIds.Num(), 1);

	// E un muro ferma anche la lama: attraversare le UNITA' non vuol dire attraversare la roccia.
	MakeWall(Map, FRTCellId(2, 0, 0));
	const FRTLinearMoveResult Walled = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0, 0), FRTCellId(4, 0, 0), /*MaxCells*/ 4,
		ERTMovementStyle::LinearPass, Occupancy, Hostiles);
	TestEqual(TEXT("il muro ferma la lama"),
		static_cast<int32>(Walled.Stop), static_cast<int32>(ERTLinearStop::BlockedByTerrain));
	TestEqual(TEXT("e conta solo chi ha attraversato PRIMA del muro"), Walled.PassedThroughUnitIds.Num(), 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
