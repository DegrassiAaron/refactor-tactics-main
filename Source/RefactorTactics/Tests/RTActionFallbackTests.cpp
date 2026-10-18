#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTCombatLibrary.h" // ERTHexTargetReason: i verdetti che la risoluzione deve coprire ([D-509])
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
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

	// [D-415]: la portata si misura sulla MIRA congelata (`Instance.TargetCell`), quindi la cella fuori portata
	// va dichiarata anche li' — una fixture con la mira a (2,0) e l'unita' a (5,0) descriverebbe un bersaglio
	// scattato via DOPO il lock-in, che [D-415] non annulla. `Fallback.ValidationJudgesTheFrozenAim` pinna la
	// differenza fra le due celle.
	TestTrue(TEXT("bersaglio fuori portata"),
		URTActionFallbackLibrary::ValidateInstance(FallbackAction(ERTActionFallback::Cancel, /*Range*/ 2, FRTCellId(5, 0)),
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

// ---------------------------------------------------------------------------------------------------------
// [D-415]/[D-419] — la validazione giudica la cella CONGELATA, non quella viva del bersaglio
// ---------------------------------------------------------------------------------------------------------

/**
 * 🔴 **Trovato dalla code review di #3230**: `ValidateInstance` misurava portata e linea di tiro su
 * `Target.Cell` — la cella viva — mentre il Blast sparava su `Instance.TargetCell`, la mira congelata. Due
 * celle decidevano due meta' dello stesso colpo, e il golden `FallbackTargetMoved` aveva archiviato
 * `OutOfRange` su una cella in portata.
 *
 * ⛔ Le due scene sono SPECULARI, ed e' cio' che rende il test non vacuo: nella prima la cella viva e' fuori
 * portata e la mira dentro (deve passare); nella seconda la cella viva e' dentro e la mira fuori (deve
 * rifiutare). Un'implementazione che leggesse la cella viva fallirebbe entrambe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallbackValidationJudgesTheFrozenAimTest,
	"RefactorTactics.Fallback.ValidationJudgesTheFrozenAim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallbackValidationJudgesTheFrozenAimTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeFallbackMap(/*Radius*/ 8);
	const int32 Range = 4;
	const FRTCellId Dentro(2, 0);
	const FRTCellId Fuori(7, 0);

	// Controllo positivo: la portata morde davvero su questa mappa, con questa distanza.
	TestEqual(TEXT("controllo positivo: viva e mira entrambe fuori portata -> OutOfRange"),
		URTActionFallbackLibrary::ValidateInstance(FallbackAction(ERTActionFallback::Cancel, Range, Fuori),
			MakeFallbackUnits(true, 1, Fuori), Map),
		ERTActionInvalidReason::OutOfRange);

	// Scena 1: il bersaglio e' SCATTATO fuori portata, ma la mira congelata e' dentro: il colpo parte.
	TestEqual(TEXT("[D-415] la cella viva fuori portata non annulla una mira congelata in portata"),
		URTActionFallbackLibrary::ValidateInstance(FallbackAction(ERTActionFallback::Cancel, Range, Dentro),
			MakeFallbackUnits(true, 1, Fuori), Map),
		ERTActionInvalidReason::None);

	// Scena 2: il bersaglio e' vicino, ma la mira congelata e' fuori portata: si rifiuta la MIRA.
	TestEqual(TEXT("[D-415] la cella viva in portata non salva una mira congelata fuori portata"),
		URTActionFallbackLibrary::ValidateInstance(FallbackAction(ERTActionFallback::Cancel, Range, Fuori),
			MakeFallbackUnits(true, 1, Dentro), Map),
		ERTActionInvalidReason::OutOfRange);

	// E il bersaglio PERSO resta perso: la mira congelata non resuscita un'unita' morta.
	TestEqual(TEXT("un bersaglio morto e' TargetDead anche con la mira in portata"),
		URTActionFallbackLibrary::ValidateInstance(FallbackAction(ERTActionFallback::Cancel, Range, Dentro),
			MakeFallbackUnits(/*bTargetAlive*/ false, 1, Dentro), Map),
		ERTActionInvalidReason::TargetDead);
	return true;
}

/**
 * **I valori di `ERTActionInvalidReason` sono APPEND-ONLY, e questo test lo rende vero** ([D-509]).
 *
 * 🔴 **Il motivo viaggia come intero grezzo in `FRTTurnLogEntry::Amount`.** Togliere una voce, spostarla o
 * inserirne una in mezzo rinumera le successive e cambia il SIGNIFICATO di ogni traccia gia' scritta che ne
 * porti una — senza cambiare la versione del formato, perche' i byte restano validi. L'header lo ripete a
 * ogni enumeratore aggiunto in coda, e racconta che e' gia' successo: `Interrupted` fu inserito a meta', e
 * lo trovo' una code review — «ogni test usa i nomi simbolici e nessun golden porta quelle voci».
 *
 * 🔑 **Si fissano NOME e NUMERO di ogni voce, in ordine.** Un test coi soli nomi simbolici resta verde dopo
 * una rinumerazione: e' esattamente la cecita' che l'header descrive. Aggiungere un motivo vuol dire
 * aggiungere una riga IN CODA a questa lista; qualunque altra modifica fa cadere il test, ed e' voluto.
 *
 * ⚠️ Se un giorno una rinumerazione fosse DECISA, questo test si aggiorna nella stessa PR che alza
 * `ERTTurnLogFormatVersion` e migra le tracce: e' la sola forma in cui quel cambio e' legittimo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInvalidReasonValuesAreAppendOnlyTest,
	"RefactorTactics.Actions.Fallback.InvalidReasonValuesAreAppendOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInvalidReasonValuesAreAppendOnlyTest::RunTest(const FString&)
{
	const TCHAR* Attesi[] = {
		TEXT("None"), TEXT("TargetGone"), TEXT("TargetDead"), TEXT("TargetFriendly"), TEXT("OutOfRange"),
		TEXT("NoLineOfSight"), TEXT("NoMap"), TEXT("TargetUnknown"), TEXT("SlotOccupied"),
		TEXT("InsufficientMovementPoints"), TEXT("OnCooldown"), TEXT("Interrupted"), TEXT("NoEffect"),
		TEXT("Neutralised"), TEXT("Unbalanced"), TEXT("DoorLocked"), TEXT("DoorDestroyed"),
		TEXT("TerrainDeniesDash"), TEXT("DashNotAligned"), TEXT("DashPathBlocked"), TEXT("Stunned"),
		TEXT("OtherLayer"), TEXT("DashEdgeBlocked") };

	const UEnum* Motivi = StaticEnum<ERTActionInvalidReason>();
	if (!TestNotNull(TEXT("l'enum dei motivi e' riflesso"), Motivi)) { return false; }

	// `NumEnums()` conta anche il `_MAX` sintetico che UHT aggiunge in coda.
	const int32 Dichiarati = Motivi->NumEnums() - 1;
	TestEqual(TEXT("il numero dei motivi e' quello della lista: uno in piu' va aggiunto IN CODA qui"),
		Dichiarati, static_cast<int32>(UE_ARRAY_COUNT(Attesi)));

	for (int32 I = 0; I < FMath::Min(Dichiarati, static_cast<int32>(UE_ARRAY_COUNT(Attesi))); ++I)
	{
		const FString Nome = Motivi->GetNameStringByIndex(I);
		const int64 Valore = Motivi->GetValueByIndex(I);
		// `Equals` con le maiuscole: `TestEqual` su due `FString` le ignora.
		TestTrue(FString::Printf(TEXT("posizione %d: atteso «%s», trovato «%s»"), I, Attesi[I], *Nome),
			Nome.Equals(Attesi[I], ESearchCase::CaseSensitive));
		// Il NUMERO, non solo l'ordine di dichiarazione: un `= 5` scritto su una voce lascerebbe l'ordine
		// intatto e cambierebbe il valore che finisce nel log.
		TestEqual(FString::Printf(TEXT("posizione %d (%s): il valore serializzato"), I, *Nome),
			Valore, static_cast<int64>(I));
	}
	return true;
}

/**
 * **Ogni verdetto del classificatore ha un esito DICHIARATO in risoluzione** ([D-509], `#2951`).
 *
 * 🔴 **Nasce da un difetto e da una promessa falsa.** Il difetto: `ValidateInstance` confrontava il verdetto
 * con UN valore, e quando `#2951` ha aggiunto `OtherLayer` a `ERTHexTargetReason` quel valore e' caduto nel
 * ramo «valido» — in risoluzione la regola del piano non esisteva. La promessa: lo `switch` che l'ha
 * sostituito non ha `default`, e la prima stesura del suo commento diceva che un verdetto nuovo non avrebbe
 * compilato. Su questo stesso enum il repository l'ha gia' misurato falso (`#3080`): MSVC emette un warning,
 * e la build non lo alza. E' la terza volta che questo enum insegna la stessa cosa.
 *
 * 🔑 **Cio' che regge e' questo banco**, sul modello di `Combat.RefusalCoversEveryReason`: i verdetti si
 * leggono dalla REFLECTION e si confrontano con una tabella dichiarata qui. Un verdetto aggiunto in coda
 * senza un esito rende il test rosso nominandolo, prima che `checkNoEntry()` uccida il runner.
 *
 * ⛔ **La tabella e' dichiarata, non derivata**: una tabella ricalcolata dal codice sotto prova sarebbe
 * verde per definizione. E per ogni verdetto che la geometria sa produrre si CHIAMA `ValidateInstance` con
 * una scena che lo produce — la tabella sola proverebbe che qualcuno l'ha scritta, non che il codice la segue.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTValidationCoversEveryTargetingVerdictTest,
	"RefactorTactics.Actions.Fallback.ValidationCoversEveryTargetingVerdict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTValidationCoversEveryTargetingVerdictTest::RunTest(const FString&)
{
	// `bProducibile`: `ValidateInstance` puo' ricevere questo verdetto dal classificatore con una scena vera.
	// `TooClose` no — la distanza minima in risoluzione non si passa — e il suo esito resta dichiarato lo
	// stesso: il giorno in cui si passera', questa riga e' la prima che qualcuno dovra' guardare.
	struct FAtteso { ERTHexTargetReason Verdetto; ERTActionInvalidReason Esito; bool bProducibile; };
	const TArray<FAtteso> Attesi = {
		{ ERTHexTargetReason::Ok,            ERTActionInvalidReason::None,          true  },
		{ ERTHexTargetReason::NoMap,         ERTActionInvalidReason::NoMap,         true  },
		{ ERTHexTargetReason::OutOfRange,    ERTActionInvalidReason::OutOfRange,    true  },
		{ ERTHexTargetReason::NoLineOfSight, ERTActionInvalidReason::NoLineOfSight, true  },
		{ ERTHexTargetReason::TooClose,      ERTActionInvalidReason::None,          false },
		{ ERTHexTargetReason::OtherLayer,    ERTActionInvalidReason::OtherLayer,    true  },
	};

	const UEnum* Enum = StaticEnum<ERTHexTargetReason>();
	if (!TestNotNull(TEXT("la reflection conosce ERTHexTargetReason"), Enum)) { return false; }
	const int32 Valori = Enum->NumEnums() - 1; // l'ultima voce e' il `_MAX` sintetico di UHT
	if (!TestTrue(TEXT("anti-vacuita': la reflection vede almeno un valore"), Valori > 0)) { return false; }

	TArray<FString> NonElencati;
	for (int32 i = 0; i < Valori; ++i)
	{
		const ERTHexTargetReason R = static_cast<ERTHexTargetReason>(Enum->GetValueByIndex(i));
		if (!Attesi.ContainsByPredicate([R](const FAtteso& A) { return A.Verdetto == R; }))
		{
			NonElencati.Add(Enum->GetNameStringByIndex(i));
		}
	}
	if (!TestTrue(*FString::Printf(TEXT("ogni verdetto ha un esito dichiarato in risoluzione; scoperti: [%s]"),
		*FString::Join(NonElencati, TEXT(", "))), NonElencati.Num() == 0))
	{
		return false;
	}
	TestEqual(TEXT("la tabella non porta verdetti fantasma"), Attesi.Num(), Valori);

	// --- E ora si chiama: una scena per ogni verdetto che la geometria sa produrre -------------------------
	URTHexMapAsset* Piana = MakeFallbackMap();
	URTHexMapAsset* ConMuro = MakeFallbackMap();
	{
		FRTHexCellData Wall(FRTCellId(1, 0));
		Wall.bBlocksLineOfSight = true;
		ConMuro->AddOrUpdateCell(Wall);
		ConMuro->SortCells();
	}
	URTHexMapAsset* ADuePiani = MakeFallbackMap();
	ADuePiani->AddOrUpdateCell(FRTHexCellData(FRTCellId(2, 0, 1)));
	ADuePiani->SortCells();

	struct FScena { ERTHexTargetReason Verdetto; const URTHexMapAsset* Mappa; FRTCellId Bersaglio; int32 Portata; };
	const FScena Scene[] = {
		{ ERTHexTargetReason::Ok,            Piana,     FRTCellId(2, 0, 0), 4 },
		{ ERTHexTargetReason::NoMap,         nullptr,   FRTCellId(2, 0, 0), 4 },
		{ ERTHexTargetReason::OutOfRange,    Piana,     FRTCellId(5, 0, 0), 2 },
		{ ERTHexTargetReason::NoLineOfSight, ConMuro,   FRTCellId(2, 0, 0), 4 },
		{ ERTHexTargetReason::OtherLayer,    ADuePiani, FRTCellId(2, 0, 1), 4 },
	};

	int32 Chiamati = 0;
	for (const FAtteso& Atteso : Attesi)
	{
		const FScena* Scena = nullptr;
		for (const FScena& S : Scene) { if (S.Verdetto == Atteso.Verdetto) { Scena = &S; } }
		const FString Nome = Enum->GetNameStringByValue(static_cast<int64>(Atteso.Verdetto));

		if (!Atteso.bProducibile)
		{
			TestNull(*FString::Printf(TEXT("%s e' dichiarato non producibile, e non ha una scena"), *Nome), Scena);
			continue;
		}
		if (!TestNotNull(*FString::Printf(TEXT("%s e' producibile: deve avere una scena"), *Nome), Scena)) { continue; }

		// La premessa, per i verdetti che il classificatore da' con una mappa: la scena produce DAVVERO
		// quel verdetto. Senza, la riga proverebbe solo che `ValidateInstance` risponde qualcosa.
		if (Scena->Mappa != nullptr)
		{
			TestTrue(*FString::Printf(TEXT("premessa: la scena di %s produce quel verdetto"), *Nome),
				URTCombatLibrary::ClassifyHexTargeting(Scena->Mappa, FRTCellId(0, 0), Scena->Bersaglio,
					Scena->Portata, ERTLineOfSightPolicy::Required) == Atteso.Verdetto);
		}

		const ERTActionInvalidReason Trovato = URTActionFallbackLibrary::ValidateInstance(
			FallbackAction(ERTActionFallback::Cancel, Scena->Portata, Scena->Bersaglio),
			MakeFallbackUnits(true, 1, Scena->Bersaglio), Scena->Mappa);
		TestTrue(*FString::Printf(TEXT("%s: ValidateInstance risponde l'esito dichiarato (trovato %s)"), *Nome,
				*StaticEnum<ERTActionInvalidReason>()->GetNameStringByValue(static_cast<int64>(Trovato))),
			Trovato == Atteso.Esito);
		++Chiamati;
	}
	TestTrue(TEXT("anti-vacuita': almeno un verdetto e' stato davvero chiamato"), Chiamati > 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
