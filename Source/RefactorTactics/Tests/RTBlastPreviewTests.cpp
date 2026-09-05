// L'anteprima del Blast: da DOVE parte l'azione e SU COSA cade, prima del lock-in.
//
// Questi test coprono il PRODUTTORE, non il wiring. La distinzione conta: `RTPlanningPreviewTests.cpp`
// verifica che `ARTHexMapActor` conservi le celle che gli si passano — un setter — e quelle celle gliele
// calcola il test stesso. Nessuno di quei verdi dice che il gioco le calcola giuste, perche' il produttore
// reale (`RefreshPlanningPreview`) viveva in un namespace anonimo dentro `RTPlayerController.cpp` e non era
// chiamabile (limite gia' dichiarato in `RTHexPerfTests.cpp:202`).
//
// Qui si verifica la derivazione: piano autorevole -> origine -> forma canonica -> fuoco amico.

#include "Misc/AutomationTest.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Ability/RTActionData.h"
#include "Map/RTCellId.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: nella unity build condividono la translation unit.
	FRTHexCombatUnit MakeBlastPreviewUnit(int32 UnitId, int32 TeamId, const FRTCellId& Cell, bool bAlive = true)
	{
		FRTHexCombatUnit U;
		U.UnitId = UnitId;
		U.TeamId = TeamId;
		U.Cell = Cell;
		U.bAlive = bAlive;
		return U;
	}

	bool BlastPreviewSameCells(const TArray<FRTCellId>& A, const TArray<FRTCellId>& B)
	{
		if (A.Num() != B.Num()) { return false; }
		for (const FRTCellId& C : A)
		{
			if (!B.Contains(C)) { return false; }
		}
		return true;
	}
}

/**
 * Con uno scatto pianificato, l'anteprima parte dalla cella dello SCATTO.
 *
 * E' la parita' con il resolver, non una preferenza estetica: `ResolveDash` gira PRIMA del Blast
 * (`RTTurnManager.cpp:1578` contro `:1582`) e il Blast legge `Unit->Cell` a scatto gia' applicato
 * (`RTTurnManager_Blast.cpp:163`, con la nota a `:165` — «la posizione autorevole per il Blast e' quella
 * post-Dash»). Un'anteprima che parte dalla cella corrente mostra una geometria che il turno non usera'.
 *
 * L'oracolo e' `Line`, la sola forma del catalogo le cui celle DIPENDONO dall'origine: con `Single` o `Area`
 * il test sarebbe verde anche ignorando lo scatto, cioe' non proverebbe niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlastPreviewOriginIsPlannedDashCellTest,
	"RefactorTactics.Preview.OriginIsPlannedDashCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlastPreviewOriginIsPlannedDashCellTest::RunTest(const FString&)
{
	const FRTCellId Current(0, 0, 0);
	const FRTCellId DashCell(0, -2, 0);
	const FRTCellId TargetCell(3, 0, 0);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(MakeBlastPreviewUnit(0, /*TeamId=*/ 0, Current));

	FRTBlastPreviewPlan Plan;
	Plan.AttackerId = 0;
	Plan.bDashResolves = true;
	Plan.PlannedDashCell = DashCell;
	Plan.bHasAction = true;
	Plan.Shape = ERTAbilityShape::Line;
	Plan.RangeCells = 6;
	Plan.bTargetsCell = true;
	Plan.TargetCell = TargetCell;

	const FRTBlastPreview Preview = URTHexCombatLibrary::MakeBlastPreview(Plan, Units);

	TestTrue(TEXT("l'origine e' la cella dello scatto"), Preview.Origin == DashCell);
	TestTrue(TEXT("l'anteprima dichiara che l'origine viene dallo scatto"), Preview.bOriginFromPlannedDash);

	const TArray<FRTCellId> FromDash =
		URTHexCombatLibrary::HexHitCells(ERTAbilityShape::Line, DashCell, TargetCell, 6, 0);
	const TArray<FRTCellId> FromCurrent =
		URTHexCombatLibrary::HexHitCells(ERTAbilityShape::Line, Current, TargetCell, 6, 0);

	// Precondizione dell'oracolo: se le due linee coincidessero il test non distinguerebbe le due origini.
	TestFalse(TEXT("precondizione: le due origini danno linee DIVERSE"),
		BlastPreviewSameCells(FromDash, FromCurrent));

	TestTrue(TEXT("la traiettoria e' quella dalla cella dello scatto"),
		BlastPreviewSameCells(Preview.HitCells, FromDash));
	TestFalse(TEXT("la traiettoria NON e' quella dalla cella corrente"),
		BlastPreviewSameCells(Preview.HitCells, FromCurrent));

	return true;
}

/**
 * Senza scatto risolto l'origine resta la cella corrente. E' l'altra meta' del test sopra: senza,
 * `bDashResolves` potrebbe essere ignorato in entrambi i versi e uno dei due verdi lo coprirebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlastPreviewOriginIsCurrentCellWithoutDashTest,
	"RefactorTactics.Preview.OriginIsCurrentCellWithoutDash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlastPreviewOriginIsCurrentCellWithoutDashTest::RunTest(const FString&)
{
	const FRTCellId Current(0, 0, 0);
	const FRTCellId TargetCell(3, 0, 0);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(MakeBlastPreviewUnit(0, 0, Current));

	FRTBlastPreviewPlan Plan;
	Plan.AttackerId = 0;
	Plan.bHasAction = true;
	Plan.Shape = ERTAbilityShape::Line;
	Plan.RangeCells = 6;
	Plan.bTargetsCell = true;
	Plan.TargetCell = TargetCell;

	// Una cella di scatto valorizzata ma NON risolta non deve spostare l'origine: e' il caso di uno scatto
	// dichiarato e poi scartato dal catalogo o dalla ricarica, che `ResolveDash` non applica.
	Plan.bDashResolves = false;
	Plan.PlannedDashCell = FRTCellId(0, -2, 0);

	const FRTBlastPreview Preview = URTHexCombatLibrary::MakeBlastPreview(Plan, Units);

	TestTrue(TEXT("l'origine e' la cella corrente"), Preview.Origin == Current);
	TestFalse(TEXT("l'anteprima non dichiara un'origine da scatto"), Preview.bOriginFromPlannedDash);
	TestTrue(TEXT("la traiettoria parte dalla cella corrente"), BlastPreviewSameCells(Preview.HitCells,
		URTHexCombatLibrary::HexHitCells(ERTAbilityShape::Line, Current, TargetCell, 6, 0)));

	return true;
}

/**
 * Un bersaglio a CELLA produce un'area, non il vuoto.
 *
 * E' il caso che `#737` ha dichiarato chiuso — «l'area colpita in preview prima del click» — e che il
 * produttore precedente spegneva: `RefreshPlanningPreview` leggeva il solo `PlannedAttackTarget`, che
 * `HandleTargetCell` azzera per costruzione (il bersaglio e' la cella). Dichiarare un'area su una cella
 * vuota cancellava l'anteprima invece di mostrarla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlastPreviewCellTargetProducesFootprintTest,
	"RefactorTactics.Preview.CellTargetProducesFootprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlastPreviewCellTargetProducesFootprintTest::RunTest(const FString&)
{
	const FRTCellId Current(0, 0, 0);
	const FRTCellId TargetCell(2, 0, 0); // cella VUOTA: nessuna unita' sopra, ed e' il caso che rende l'AoE usabile

	TArray<FRTHexCombatUnit> Units;
	Units.Add(MakeBlastPreviewUnit(0, 0, Current));

	FRTBlastPreviewPlan Plan;
	Plan.AttackerId = 0;
	Plan.bHasAction = true;
	Plan.Shape = ERTAbilityShape::Area;
	Plan.RangeCells = 4;
	Plan.AreaRadius = 1;
	Plan.bTargetsCell = true;
	Plan.TargetCell = TargetCell;
	Plan.TargetId = INDEX_NONE; // il bersaglio e' la cella: nessuna unita' da cui dedurre l'area

	const FRTBlastPreview Preview = URTHexCombatLibrary::MakeBlastPreview(Plan, Units);

	const TArray<FRTCellId> Expected =
		URTHexCombatLibrary::HexHitCells(ERTAbilityShape::Area, Current, TargetCell, 4, 1);
	TestTrue(TEXT("precondizione: la forma di riferimento non e' vuota"), Expected.Num() > 0);

	TestEqual(TEXT("l'anteprima ha le celle dell'area, non zero"), Preview.HitCells.Num(), Expected.Num());
	TestTrue(TEXT("le celle sono quelle della forma canonica"),
		BlastPreviewSameCells(Preview.HitCells, Expected));
	TestTrue(TEXT("la cella mirata e' nell'area"), Preview.HitCells.Contains(TargetCell));

	return true;
}

/**
 * Il fuoco amico si vede anche quando il bersaglio e' una CELLA. La regressione di sopra lo portava via
 * insieme all'area: un alleato dentro un'area centrata su un varco non era segnalato da nessuno.
 *
 * E resta subordinato a `bFriendlyFire`, come nel produttore precedente: un avviso su un evento impossibile
 * insegna a ignorare gli avvisi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlastPreviewAllyInCellTargetAreaIsFlaggedTest,
	"RefactorTactics.Preview.AllyInCellTargetAreaIsFlagged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlastPreviewAllyInCellTargetAreaIsFlaggedTest::RunTest(const FString&)
{
	const FRTCellId Current(0, 0, 0);
	const FRTCellId TargetCell(2, 0, 0);
	const FRTCellId AllyCell(2, -1, 0);   // adiacente al centro: dentro l'area di raggio 1
	const FRTCellId EnemyCell(3, 0, 0);   // pure dentro l'area, ma non e' fuoco amico
	const FRTCellId FallenCell(2, 1, 0);  // alleato caduto, dentro l'area: non si segnala

	TArray<FRTHexCombatUnit> Units;
	Units.Add(MakeBlastPreviewUnit(0, /*TeamId=*/ 0, Current));
	Units.Add(MakeBlastPreviewUnit(1, /*TeamId=*/ 0, AllyCell));
	Units.Add(MakeBlastPreviewUnit(2, /*TeamId=*/ 1, EnemyCell));
	Units.Add(MakeBlastPreviewUnit(3, /*TeamId=*/ 0, FallenCell, /*bAlive=*/ false));

	FRTBlastPreviewPlan Plan;
	Plan.AttackerId = 0;
	Plan.bHasAction = true;
	Plan.Shape = ERTAbilityShape::Area;
	Plan.RangeCells = 4;
	Plan.AreaRadius = 1;
	Plan.bTargetsCell = true;
	Plan.TargetCell = TargetCell;
	Plan.bFriendlyFire = true;

	const FRTBlastPreview Preview = URTHexCombatLibrary::MakeBlastPreview(Plan, Units);

	// Precondizioni dell'oracolo: le tre celle stanno davvero nell'area, altrimenti i tre asserti sotto
	// sarebbero verdi per assenza.
	TestTrue(TEXT("precondizione: l'alleato e' nell'area"), Preview.HitCells.Contains(AllyCell));
	TestTrue(TEXT("precondizione: il nemico e' nell'area"), Preview.HitCells.Contains(EnemyCell));
	TestTrue(TEXT("precondizione: la cella del caduto e' nell'area"), Preview.HitCells.Contains(FallenCell));

	TestEqual(TEXT("un solo alleato segnalato"), Preview.AllyCells.Num(), 1);
	TestTrue(TEXT("l'alleato vivo e' segnalato"), Preview.AllyCells.Contains(AllyCell));
	TestFalse(TEXT("il nemico NON e' fuoco amico"), Preview.AllyCells.Contains(EnemyCell));
	TestFalse(TEXT("un alleato caduto NON e' fuoco amico"), Preview.AllyCells.Contains(FallenCell));

	// Senza `bFriendlyFire` l'area resta identica e l'avviso sparisce: e' un avviso, non una forma.
	Plan.bFriendlyFire = false;
	const FRTBlastPreview NoFF = URTHexCombatLibrary::MakeBlastPreview(Plan, Units);
	TestTrue(TEXT("l'area non cambia senza fuoco amico"), BlastPreviewSameCells(NoFF.HitCells, Preview.HitCells));
	TestEqual(TEXT("nessun avviso quando l'azione non colpisce i propri"), NoFF.AllyCells.Num(), 0);

	return true;
}

/**
 * Un bersaglio-unita' MORTO non produce anteprima, ma non porta via l'origine: quella e' un fatto del piano,
 * non del bersaglio. Senza la distinzione un bersaglio caduto spegnerebbe anche il ghost dell'unita'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlastPreviewDeadTargetHasNoFootprintTest,
	"RefactorTactics.Preview.DeadTargetHasNoFootprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlastPreviewDeadTargetHasNoFootprintTest::RunTest(const FString&)
{
	const FRTCellId Current(0, 0, 0);
	const FRTCellId DashCell(1, 0, 0);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(MakeBlastPreviewUnit(0, 0, Current));
	Units.Add(MakeBlastPreviewUnit(1, 1, FRTCellId(2, 0, 0), /*bAlive=*/ false));

	FRTBlastPreviewPlan Plan;
	Plan.AttackerId = 0;
	Plan.bDashResolves = true;
	Plan.PlannedDashCell = DashCell;
	Plan.bHasAction = true;
	Plan.Shape = ERTAbilityShape::Area;
	Plan.RangeCells = 4;
	Plan.AreaRadius = 1;
	Plan.TargetId = 1;

	const FRTBlastPreview Preview = URTHexCombatLibrary::MakeBlastPreview(Plan, Units);

	TestEqual(TEXT("nessuna cella colpita su un bersaglio caduto"), Preview.HitCells.Num(), 0);
	TestEqual(TEXT("nessun fuoco amico"), Preview.AllyCells.Num(), 0);
	TestTrue(TEXT("l'origine resta quella dello scatto"), Preview.Origin == DashCell);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
