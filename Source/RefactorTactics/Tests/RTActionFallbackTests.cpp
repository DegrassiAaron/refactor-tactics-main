#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h" // MakeFlatArena: un solo builder di arena piatta
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTActionFallbackLibrary.h"
#include "Turn/RTTurnLogLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Mappa piena senza ostacoli: la geometria non interferisce con i casi che riguardano il bersaglio. */
	URTHexMapAsset* MakeFallbackMap(int32 Radius = 6)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		return M;
	}

	/** Due squadre a distanza 2: attaccante (id 0, team 0) e bersaglio (id 1, team 1). */
	TArray<FRTHexCombatUnit> MakeFallbackUnits(bool bTargetAlive = true, int32 TargetTeam = 1,
		const FRTCellId& TargetCell = FRTCellId(2, 0))
	{
		TArray<FRTHexCombatUnit> Units;
		FRTHexCombatUnit A;
		A.UnitId = 0; A.TeamId = 0; A.Cell = FRTCellId(0, 0); A.bAlive = true;
		FRTHexCombatUnit B;
		B.UnitId = 1; B.TeamId = TargetTeam; B.Cell = TargetCell; B.bAlive = bTargetAlive;
		Units.Add(A);
		Units.Add(B);
		return Units;
	}

	/** Azione d'attacco pianificata da 0 su 1, col fallback dichiarato dal chiamante. Nome distinto per file. */
	FRTActionInstance FallbackAction(ERTActionFallback Fallback, int32 RangeCells = 4,
		const FRTCellId& TargetCell = FRTCellId(2, 0))
	{
		FRTActionInstance Instance;
		Instance.Def.ActionId = TEXT("Action.Test");
		Instance.Def.ResolutionPhase = ERTResolutionPhase::Attack;
		Instance.Def.RangeCells = RangeCells;
		Instance.Def.Fallback = Fallback;
		Instance.Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 25));
		Instance.SourceUnitId = 0;
		Instance.TargetUnitId = 1;
		Instance.TargetCell = TargetCell;
		return Instance;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallbackValidationTest,
	"RefactorTactics.Actions.Fallback.ValidationReasons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallbackValidationTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeFallbackMap();

	// Il caso buono: bersaglio vivo, nemico, in portata, in vista.
	TestTrue(TEXT("azione eseguibile: nessun motivo"),
		URTActionFallbackLibrary::ValidateInstance(FallbackAction(ERTActionFallback::Cancel),
			MakeFallbackUnits(), Map) == ERTActionInvalidReason::None);

	// Un motivo per ciascuna condizione, nell'ordine in cui si escludono a vicenda.
	TestTrue(TEXT("bersaglio eliminato"),
		URTActionFallbackLibrary::ValidateInstance(FallbackAction(ERTActionFallback::Cancel),
			MakeFallbackUnits(/*bTargetAlive*/ false), Map) == ERTActionInvalidReason::TargetDead);

	TestTrue(TEXT("bersaglio alleato"),
		URTActionFallbackLibrary::ValidateInstance(FallbackAction(ERTActionFallback::Cancel),
			MakeFallbackUnits(true, /*TargetTeam*/ 0), Map) == ERTActionInvalidReason::TargetFriendly);

	TestTrue(TEXT("bersaglio fuori portata"),
		URTActionFallbackLibrary::ValidateInstance(FallbackAction(ERTActionFallback::Cancel, /*Range*/ 2),
			MakeFallbackUnits(true, 1, FRTCellId(5, 0)), Map) == ERTActionInvalidReason::OutOfRange);

	// Indice inesistente: il bersaglio non c'e' piu' nel turno.
	FRTActionInstance Ghost = FallbackAction(ERTActionFallback::Cancel);
	Ghost.TargetUnitId = 7;
	TestTrue(TEXT("bersaglio assente dal turno"),
		URTActionFallbackLibrary::ValidateInstance(Ghost, MakeFallbackUnits(), Map)
		== ERTActionInvalidReason::TargetGone);

	// FAIL-CLOSED: senza mappa non si valida la traiettoria, e il motivo resta distinto da una copertura.
	TestTrue(TEXT("nessuna mappa autorevole"),
		URTActionFallbackLibrary::ValidateInstance(FallbackAction(ERTActionFallback::Cancel),
			MakeFallbackUnits(), nullptr) == ERTActionInvalidReason::NoMap);

	// Un muro fra i due: in portata, ma senza linea di tiro.
	URTHexMapAsset* Walled = MakeFallbackMap();
	FRTHexCellData Wall(FRTCellId(1, 0));
	Wall.bBlocksLineOfSight = true;
	Walled->AddOrUpdateCell(Wall);
	Walled->SortCells();
	TestTrue(TEXT("traiettoria bloccata"),
		URTActionFallbackLibrary::ValidateInstance(FallbackAction(ERTActionFallback::Cancel),
			MakeFallbackUnits(), Walled) == ERTActionInvalidReason::NoLineOfSight);

	// Le azioni senza bersaglio-unita' (movimento, supporto su se stessi) non falliscono qui.
	FRTActionInstance SelfCast = FallbackAction(ERTActionFallback::Cancel);
	SelfCast.TargetUnitId = SelfCast.SourceUnitId;
	TestTrue(TEXT("su se stessi si e' sempre a portata"),
		URTActionFallbackLibrary::ValidateInstance(SelfCast, MakeFallbackUnits(), Map)
		== ERTActionInvalidReason::None);

	FRTActionInstance NoTarget = FallbackAction(ERTActionFallback::Stop);
	NoTarget.TargetUnitId = INDEX_NONE;
	TestTrue(TEXT("azione senza bersaglio: niente da validare qui"),
		URTActionFallbackLibrary::ValidateInstance(NoTarget, MakeFallbackUnits(), Map)
		== ERTActionInvalidReason::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallbackCancelTest,
	"RefactorTactics.Actions.Fallback.Cancel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallbackCancelTest::RunTest(const FString&)
{
	// Attacchi diretti e cure: se il bersaglio non c'e' piu', non si esegue nulla.
	const FRTFallbackResult R = URTActionFallbackLibrary::ApplyFallback(
		FallbackAction(ERTActionFallback::Cancel), ERTActionInvalidReason::TargetDead);

	TestTrue(TEXT("il fallback applicato e' Cancel"), R.Applied == ERTActionFallback::Cancel);
	TestFalse(TEXT("nessun effetto prodotto"), R.bProducesEffects);
	TestEqual(TEXT("gli effetti sono stati rimossi, non solo ignorati"), R.Instance.Def.Effects.Num(), 0);
	TestEqual(TEXT("nessun bersaglio di ripiego"), R.Instance.TargetUnitId, static_cast<int32>(INDEX_NONE));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallbackStopTest,
	"RefactorTactics.Actions.Fallback.Stop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallbackStopTest::RunTest(const FString&)
{
	// Il movimento non si annulla: procede fino all'ultima cella valida. L'istanza resta intatta perche' a
	// fermarla e' il resolver dei percorsi, che avanza a micro-step.
	const FRTActionInstance Planned = FallbackAction(ERTActionFallback::Stop);
	const FRTFallbackResult R =
		URTActionFallbackLibrary::ApplyFallback(Planned, ERTActionInvalidReason::TargetDead);

	TestTrue(TEXT("il fallback applicato e' Stop"), R.Applied == ERTActionFallback::Stop);
	TestTrue(TEXT("l'azione procede"), R.bProducesEffects);
	TestTrue(TEXT("la destinazione pianificata non viene toccata"), R.Instance.TargetCell == Planned.TargetCell);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallbackAttackCellTest,
	"RefactorTactics.Actions.Fallback.AttackCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallbackAttackCellTest::RunTest(const FString&)
{
	// AoE: il bersaglio muore prima del Blast, l'area parte lo stesso dove era stata puntata.
	const FRTFallbackResult Dead = URTActionFallbackLibrary::ApplyFallback(
		FallbackAction(ERTActionFallback::AttackCell), ERTActionInvalidReason::TargetDead);

	TestTrue(TEXT("il fallback applicato e' AttackCell"), Dead.Applied == ERTActionFallback::AttackCell);
	TestTrue(TEXT("l'azione procede"), Dead.bProducesEffects);
	TestEqual(TEXT("l'unita' bersaglio sparisce"), Dead.Instance.TargetUnitId, static_cast<int32>(INDEX_NONE));
	TestTrue(TEXT("la cella pianificata resta il centro dell'area"),
		Dead.Instance.TargetCell == FRTCellId(2, 0));
	TestEqual(TEXT("gli effetti restano: l'area colpisce davvero"), Dead.Instance.Def.Effects.Num(), 1);

	// Se pero' il problema era la GEOMETRIA, la cella ha lo stesso difetto del bersaglio: si annulla invece di
	// sparare oltre la portata o attraverso un muro.
	const FRTFallbackResult Far = URTActionFallbackLibrary::ApplyFallback(
		FallbackAction(ERTActionFallback::AttackCell), ERTActionInvalidReason::OutOfRange);
	TestTrue(TEXT("fuori portata: degrada a Cancel"), Far.Applied == ERTActionFallback::Cancel);
	TestFalse(TEXT("e non produce effetti"), Far.bProducesEffects);

	const FRTFallbackResult Blind = URTActionFallbackLibrary::ApplyFallback(
		FallbackAction(ERTActionFallback::AttackCell), ERTActionInvalidReason::NoLineOfSight);
	TestTrue(TEXT("traiettoria bloccata: degrada a Cancel"), Blind.Applied == ERTActionFallback::Cancel);

	// Bersaglio sparito del tutto: non si sa nemmeno dove fosse, quindi non c'e' una cella su cui ripiegare.
	const FRTFallbackResult Gone = URTActionFallbackLibrary::ApplyFallback(
		FallbackAction(ERTActionFallback::AttackCell), ERTActionInvalidReason::TargetGone);
	TestTrue(TEXT("bersaglio assente: degrada a Cancel"), Gone.Applied == ERTActionFallback::Cancel);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallbackWaitTest,
	"RefactorTactics.Actions.Fallback.Wait",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallbackWaitTest::RunTest(const FString&)
{
	// L'azione viene sostituita con `Action.Wait`, presa dal catalogo: occupa lo slot, non fa nulla, e resta
	// osservabile nel TurnLog.
	const FRTFallbackResult R = URTActionFallbackLibrary::ApplyFallback(
		FallbackAction(ERTActionFallback::Wait), ERTActionInvalidReason::TargetDead);

	TestTrue(TEXT("il fallback applicato e' Wait"), R.Applied == ERTActionFallback::Wait);
	TestFalse(TEXT("l'attesa non produce effetti"), R.bProducesEffects);
	TestTrue(TEXT("l'azione e' diventata Action.Wait"), R.Instance.Def.ActionId == FName(TEXT("Action.Wait")));
	TestEqual(TEXT("e Wait non ha effetti"), R.Instance.Def.Effects.Num(), 0);
	TestEqual(TEXT("l'attesa risolve per ultima (priorita' 100 del catalogo)"), R.Instance.Def.Priority, 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallbackNoRandomTargetingTest,
	"RefactorTactics.Actions.Fallback.NoRandomTargeting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallbackNoRandomTargetingTest::RunTest(const FString&)
{
	// Regola di prodotto (catalogo §17): nessun fallback sceglie un bersaglio al posto di chi gioca. Nemmeno
	// `BasicAttack`, che nel catalogo direbbe «il bersaglio valido piu' vicino»: in v0.1 degrada a Cancel e lo
	// DICHIARA, invece di far sparire l'azione o di sparare a qualcun altro.
	const FRTFallbackResult Basic = URTActionFallbackLibrary::ApplyFallback(
		FallbackAction(ERTActionFallback::BasicAttack), ERTActionInvalidReason::TargetDead);
	TestTrue(TEXT("BasicAttack degrada a Cancel"), Basic.Applied == ERTActionFallback::Cancel);
	TestFalse(TEXT("e non colpisce nessuno"), Basic.bProducesEffects);
	TestEqual(TEXT("nessun bersaglio scelto d'ufficio"), Basic.Instance.TargetUnitId, static_cast<int32>(INDEX_NONE));

	// `AttackTarget` segue il bersaglio SE ancora valido: qui non lo e' (per questo siamo nel fallback), quindi
	// non c'e' nessuno da seguire — non «il prossimo».
	const FRTFallbackResult Follow = URTActionFallbackLibrary::ApplyFallback(
		FallbackAction(ERTActionFallback::AttackTarget), ERTActionInvalidReason::TargetDead);
	TestTrue(TEXT("AttackTarget senza bersaglio valido: Cancel"), Follow.Applied == ERTActionFallback::Cancel);
	TestEqual(TEXT("nessun ripiego su un'altra unita'"), Follow.Instance.TargetUnitId, static_cast<int32>(INDEX_NONE));

	// La garanzia strutturale: NESSUN fallback, per nessun motivo, punta a un'unita' diversa da quella scelta
	// in pianificazione. O tiene la sua, o non ne ha nessuna.
	const ERTActionFallback AllFallbacks[] = {
		ERTActionFallback::Stop, ERTActionFallback::Wait, ERTActionFallback::AttackCell,
		ERTActionFallback::AttackTarget, ERTActionFallback::BasicAttack, ERTActionFallback::Cancel };
	const ERTActionInvalidReason AllReasons[] = {
		ERTActionInvalidReason::TargetGone, ERTActionInvalidReason::TargetDead,
		ERTActionInvalidReason::TargetFriendly, ERTActionInvalidReason::OutOfRange,
		ERTActionInvalidReason::NoLineOfSight, ERTActionInvalidReason::NoMap };

	for (const ERTActionFallback Fallback : AllFallbacks)
	{
		for (const ERTActionInvalidReason Reason : AllReasons)
		{
			const FRTFallbackResult R = URTActionFallbackLibrary::ApplyFallback(FallbackAction(Fallback), Reason);
			const bool bKeepsOrDrops = (R.Instance.TargetUnitId == INDEX_NONE || R.Instance.TargetUnitId == 1);
			TestTrue(FString::Printf(TEXT("fallback %d, motivo %d: nessun bersaglio inventato"),
				static_cast<int32>(Fallback), static_cast<int32>(Reason)), bKeepsOrDrops);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallbackLogOutcomeTest,
	"RefactorTactics.Actions.Fallback.LoggedOutcome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallbackLogOutcomeTest::RunTest(const FString&)
{
	// Ogni fallback ha un esito registrabile: un'azione che fallisce non puo' sparire dal TurnLog.
	TestTrue(TEXT("Stop -> fermata"),
		URTActionFallbackLibrary::ToLogOutcome(ERTActionFallback::Stop) == ERTFallbackOutcome::Stopped);
	TestTrue(TEXT("Wait -> attesa"),
		URTActionFallbackLibrary::ToLogOutcome(ERTActionFallback::Wait) == ERTFallbackOutcome::Waited);
	TestTrue(TEXT("AttackCell -> colpisce la cella"),
		URTActionFallbackLibrary::ToLogOutcome(ERTActionFallback::AttackCell) == ERTFallbackOutcome::AttackedCell);
	TestTrue(TEXT("Cancel -> annullata"),
		URTActionFallbackLibrary::ToLogOutcome(ERTActionFallback::Cancel) == ERTFallbackOutcome::Cancelled);
	TestTrue(TEXT("AttackTarget senza bersaglio -> annullata"),
		URTActionFallbackLibrary::ToLogOutcome(ERTActionFallback::AttackTarget) == ERTFallbackOutcome::Cancelled);
	TestTrue(TEXT("BasicAttack (non praticabile in v0.1) -> annullata"),
		URTActionFallbackLibrary::ToLogOutcome(ERTActionFallback::BasicAttack) == ERTFallbackOutcome::Cancelled);

	// La riga di log dice cosa e' successo E perche': senza il motivo, «annullata» non insegna nulla.
	FRTTurnLogEntry Entry;
	Entry.Phase = ERTMatchPhase::Blast;
	Entry.Category = ERTLogCategory::Fallback;
	Entry.Outcome = static_cast<uint8>(ERTFallbackOutcome::Cancelled);
	Entry.SrcCell = FRTCellId(0, 0);
	Entry.TgtCell = FRTCellId(2, 0);
	Entry.Amount = static_cast<int32>(ERTActionInvalidReason::TargetDead);

	const FString Text = URTTurnLogLibrary::DescribeEntry(Entry);
	TestTrue(TEXT("la riga dice che l'azione e' annullata"), Text.Contains(TEXT("annullata")));
	TestTrue(TEXT("e dice perche'"), Text.Contains(TEXT("bersaglio eliminato")));
	TestTrue(TEXT("con le coordinate assiali di partenza"), Text.Contains(TEXT("q=0")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
