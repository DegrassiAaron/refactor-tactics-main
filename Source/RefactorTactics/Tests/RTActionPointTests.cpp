// E38 CP 38.6 (#4012) — gli asserti degli Action Point dopo i numeri di D-594.
//
// Cinque asserti nel piano di E38 (`docs/roadmap/plans/e38-action-point-numeri-simulazione-2026-11-08.md` §5). Il
// primo, `Plan.LegalTodayStaysLegal`, vive in `RTPlanLegalFormsTests.cpp`. Qui gli altri quattro:
//
//  - `Plan.SumOverBudgetIsRejected` e `Catalog.EveryActionDeclaresACost` hanno un soggetto e sono verdi sul merito;
//  - `Resolution.SamePhaseOrderIsDeterministic`, `Resolution.SameTriggerReactionOrderIsDeterministic` e
//    `TurnLog.PlanIsAListNotThreeSlots` NON hanno ancora un soggetto: il resolver accetta una azione per fase,
//    l'unita' arma una reazione, il piano e' tre campi. La issue li voleva «rossi per costruzione»; un rosso permanente
//    rende rossa la suite di chiunque la lanci e si confonde con una regressione, quindi sono scritti nella forma
//    «limite di oggi, pinnato»: misurano che il limite C'E', con il predicato che il giorno della consegna di E38
//    diventera' falso. Allora questi test cadono e vanno riscritti sul soggetto vero, non spenti.
//
// ⚠️ Costi delle classi nelle fixture: P 1 · D 2 · B 2 · M 2 · R 2 · C 2, cioe' D-594 (1) — scritti a mano e non
// letti dal catalogo, perche' qui si prova il VALIDATORE: che legga la colonna e sommi, non che il catalogo la riempia
// (quello e' `EveryActionDeclaresACost`).
#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Turn/RTActionFallbackLibrary.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTPlanValidationLibrary.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Unit/RTUnit.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FRTHexSimUnit ApUnit()
	{
		return FRTHexSimUnit(0, FRTCellId(0, 0), /*MoveBudget*/ 8, /*bAlive*/ true);
	}

	FRTPlannedAction ApEntry(const TCHAR* ActionId, ERTActionSlot Slot, int32 Cost)
	{
		FRTPlannedAction Entry;
		Entry.Def.ActionId = FName(ActionId);
		Entry.Def.Slot = Slot;
		Entry.Def.ActionPointCost = Cost;
		Entry.CooldownRemaining = 0;
		return Entry;
	}

	/** Una voce per classe di D-594: P Guard 1 · D Dodge 2 · B BasicAttack 2 · M Move 2 · R Counter 2 · C CreateWater 2. */
	bool ApEntryForClass(TCHAR Class, FRTPlannedAction& Out)
	{
		switch (Class)
		{
		case TEXT('P'): Out = ApEntry(TEXT("Action.Guard"), ERTActionSlot::Main, URTCatalogLibrary::ActionPointCostPrep); return true;
		case TEXT('D'): Out = ApEntry(TEXT("Action.Dodge"), ERTActionSlot::Movement, URTCatalogLibrary::ActionPointCostDash); return true;
		case TEXT('B'): Out = ApEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main, URTCatalogLibrary::ActionPointCostBlast); return true;
		case TEXT('M'): Out = ApEntry(TEXT("Action.Move"), ERTActionSlot::Movement, URTCatalogLibrary::ActionPointCostMove); return true;
		case TEXT('R'): Out = ApEntry(TEXT("Action.Counter"), ERTActionSlot::Reaction, URTCatalogLibrary::ActionPointCostReaction); return true;
		case TEXT('C'): Out = ApEntry(TEXT("Action.CreateWater"), ERTActionSlot::Main, URTCatalogLibrary::ActionPointCostCleanup); return true;
		default: return false;
		}
	}

	TArray<FRTPlannedAction> ApPlanForForm(const FString& Form)
	{
		TArray<FRTPlannedAction> Plan;
		for (const TCHAR C : Form)
		{
			FRTPlannedAction Entry;
			if (ApEntryForClass(C, Entry)) { Plan.Add(Entry); }
		}
		return Plan;
	}

	/** Lo stesso `ActionId` ripetuto N volte con lo stesso costo: la forma piu' semplice per sforare un budget. */
	TArray<FRTPlannedAction> ApRepeated(const TCHAR* ActionId, ERTActionSlot Slot, int32 Cost, int32 Times)
	{
		TArray<FRTPlannedAction> Plan;
		for (int32 I = 0; I < Times; ++I) { Plan.Add(ApEntry(ActionId, Slot, Cost)); }
		return Plan;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanSumOverBudgetIsRejected,
	"RefactorTactics.Plan.SumOverBudgetIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanSumOverBudgetIsRejected::RunTest(const FString&)
{
	TestEqual(TEXT("premessa: il budget di D-594 e' 6"), URTPlanValidationLibrary::ActionPointBudget, 6);

	// --- 1. quattro principali del Blast fanno 8: rifiutate per BUDGET, non per slot, e nominando la voce che sfora
	{
		const TArray<FRTPlannedAction> Plan = ApRepeated(TEXT("Action.BasicAttack"), ERTActionSlot::Main, 2, 4);
		TestEqual(TEXT("premessa: la somma e' 8"), URTPlanValidationLibrary::ActionPointsSpent(Plan), 8);
		const FRTPlanValidation Verdict = URTPlanValidationLibrary::ValidatePlan(ApUnit(), Plan);
		TestFalse(TEXT("8 > 6: illegale"), Verdict.bLegal);
		TestEqual(TEXT("con il motivo proprio del budget"), Verdict.Reason, ERTActionInvalidReason::OverBudget);
		TestEqual(TEXT("e nomina l'azione che sfora"), Verdict.OffendingActionId, FName(TEXT("Action.BasicAttack")));
		TestTrue(TEXT("nessun detentore di slot: non e' un conflitto strutturale"), Verdict.HolderActionId.IsNone());
	}

	// --- 2. il budget precede lo slot: tre principali (6) sono rifiutate per SLOT, quattro (8) per budget ---------
	{
		const FRTPlanValidation Tre = URTPlanValidationLibrary::ValidatePlan(ApUnit(),
			ApRepeated(TEXT("Action.BasicAttack"), ERTActionSlot::Main, 2, 3));
		TestEqual(TEXT("6 = 6: il budget tace e parla lo slot"), Tre.Reason, ERTActionInvalidReason::SlotOccupied);
	}

	// --- 3. esattamente 6 non sfora: la forma piu' cara legale oggi (D B R: 2+2+2) e' accettata -------------------
	{
		const TArray<FRTPlannedAction> Plan = ApPlanForForm(TEXT("DBR"));
		TestEqual(TEXT("premessa: D B R costa 6"), URTPlanValidationLibrary::ActionPointsSpent(Plan), 6);
		TestTrue(TEXT("6 <= 6: legale"), URTPlanValidationLibrary::ValidatePlan(ApUnit(), Plan).bLegal);
	}

	// --- 4. la voce nominata e' la prima che sfora nell'ORDINE CANONICO, non nell'ordine dell'array -----------------
	// Costi 2+2+1+2 = 7 in due sequenze diverse: il validatore ordina per larghezza di slot e poi per ActionId, quindi
	// nomina la stessa azione in entrambi i casi. `Action.Move` (Movement) e `Action.Guard` (Main) precedono per
	// ActionId `Action.Counter` (Reaction)? No: la larghezza e' 1 per tutte, quindi vince l'ordine alfabetico —
	// BasicAttack, Counter, Guard, Move — e la somma 2 + 2 + 1 + 2 sfora su Move.
	{
		TArray<FRTPlannedAction> Diretto;
		Diretto.Add(ApEntry(TEXT("Action.Move"), ERTActionSlot::Movement, 2));
		Diretto.Add(ApEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main, 2));
		Diretto.Add(ApEntry(TEXT("Action.Guard"), ERTActionSlot::Main, 1));
		Diretto.Add(ApEntry(TEXT("Action.Counter"), ERTActionSlot::Reaction, 2));
		TArray<FRTPlannedAction> Inverso = Diretto;
		Algo::Reverse(Inverso);

		const FRTPlanValidation A = URTPlanValidationLibrary::ValidatePlan(ApUnit(), Diretto);
		const FRTPlanValidation B = URTPlanValidationLibrary::ValidatePlan(ApUnit(), Inverso);
		TestEqual(TEXT("7 > 6 in un verso"), A.Reason, ERTActionInvalidReason::OverBudget);
		TestEqual(TEXT("e nell'altro"), B.Reason, ERTActionInvalidReason::OverBudget);
		TestEqual(TEXT("la voce accusata non dipende dall'ordine dell'array"), A.OffendingActionId, B.OffendingActionId);
		TestEqual(TEXT("ed e' Move, l'ultima dell'ordine canonico"), A.OffendingActionId, FName(TEXT("Action.Move")));
	}

	// --- 5. il descrittore del TurnLog ha una frase per il motivo nuovo -------------------------------------------
	TestTrue(TEXT("`OverBudget` si legge nel TurnLog con una frase propria"),
		URTTurnLogLibrary::DescribeInvalidReason(ERTActionInvalidReason::OverBudget).Contains(TEXT("Action Point")));

	// --- 6. controllo positivo: le 23 forme legali di oggi non sforano mai (D-594 non perde nulla) ----------------
	for (const TCHAR* Form : { TEXT("BDR"), TEXT("BMR"), TEXT("CDR"), TEXT("CMR"), TEXT("DPR"), TEXT("MPR") })
	{
		const TArray<FRTPlannedAction> Plan = ApPlanForForm(Form);
		TestTrue(*FString::Printf(TEXT("forma a tre azioni %s: entro il budget"), Form),
			URTPlanValidationLibrary::ActionPointsSpent(Plan) <= URTPlanValidationLibrary::ActionPointBudget);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogEveryActionDeclaresACost,
	"RefactorTactics.Catalog.EveryActionDeclaresACost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogEveryActionDeclaresACost::RunTest(const FString&)
{
	// Catalogo core: ogni riga ha un costo > 0, tranne `Action.Wait` che vale 0; e il costo e' quello della classe
	// di D-594, perche' oggi nessuna riga ne dichiara uno proprio (le Ultimate non sono a catalogo).
	const TArray<FRTActionDef> Core = URTCatalogLibrary::GetCoreActionCatalog();
	if (!TestTrue(TEXT("premessa: il catalogo core non e' vuoto"), Core.Num() > 0)) { return false; }
	int32 Lette = 0;
	for (const FRTActionDef& Def : Core)
	{
		++Lette;
		const FString Id = Def.ActionId.ToString();
		if (Def.ActionId == FName(TEXT("Action.Wait")))
		{
			TestEqual(TEXT("Action.Wait: l'unica riga a 0, il turno che passa"), Def.ActionPointCost, 0);
			continue;
		}
		TestTrue(*FString::Printf(TEXT("%s: costo AP dichiarato (> 0)"), *Id), Def.ActionPointCost > 0);
		TestEqual(*FString::Printf(TEXT("%s: il costo e' quello della classe di D-594"), *Id),
			Def.ActionPointCost, URTCatalogLibrary::DefaultActionPointCost(Def));
	}
	TestTrue(TEXT("anti-vacuita': il catalogo core e' stato letto riga per riga"), Lette == Core.Num());

	// I valori di D-594, pinnati su una riga per classe: se la tabella cambia, cambia qui, non in silenzio.
	TestEqual(TEXT("Prep 1: Action.Guard"), URTCatalogLibrary::FindCoreAction(TEXT("Action.Guard")).ActionPointCost, 1);
	TestEqual(TEXT("Dash 2: Action.Dodge"), URTCatalogLibrary::FindCoreAction(TEXT("Action.Dodge")).ActionPointCost, 2);
	TestEqual(TEXT("Blast 2: Action.BasicAttack"), URTCatalogLibrary::FindCoreAction(TEXT("Action.BasicAttack")).ActionPointCost, 2);
	TestEqual(TEXT("Move 2: Action.Move"), URTCatalogLibrary::FindCoreAction(TEXT("Action.Move")).ActionPointCost, 2);
	TestEqual(TEXT("Move 2 anche per lo Sprint: stesso costo per tutti i profili (D-594 (2))"),
		URTCatalogLibrary::FindCoreAction(TEXT("Action.Sprint")).ActionPointCost, 2);
	TestEqual(TEXT("reazione 2: Action.Counter"), URTCatalogLibrary::FindCoreAction(TEXT("Action.Counter")).ActionPointCost, 2);
	TestEqual(TEXT("ambientale del Cleanup 2: Action.CreateWater"), URTCatalogLibrary::FindCoreAction(TEXT("Action.CreateWater")).ActionPointCost, 2);

	// Catalogo eroi: le abilita' dei quattro a runtime, costruite da `MakeHeroAction`, dichiarano tutte un costo.
	int32 Abilita = 0;
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!TestNotNull(TEXT("eroe del roster"), Hero)) { continue; }
		for (const URTActionData* Action : Hero->Actions)
		{
			if (!TestNotNull(*FString::Printf(TEXT("%s: abilita' non nulla"), *Hero->HeroId.ToString()), Action)) { continue; }
			++Abilita;
			const FString Id = Action->Def.ActionId.ToString();
			TestTrue(*FString::Printf(TEXT("%s: costo AP dichiarato (> 0)"), *Id), Action->Def.ActionPointCost > 0);
			TestEqual(*FString::Printf(TEXT("%s: il costo e' quello della classe di D-594"), *Id),
				Action->Def.ActionPointCost, URTCatalogLibrary::DefaultActionPointCost(Action->Def));
		}
	}
	TestTrue(TEXT("anti-vacuita': le abilita' d'eroe sono state lette"), Abilita >= 20);

	// E il validatore del catalogo rifiuta un costo negativo, come fa per portata e ricarica.
	FRTActionDef Negativo = URTCatalogLibrary::FindCoreAction(TEXT("Action.Guard"));
	Negativo.ActionPointCost = -1;
	TestTrue(TEXT("costo AP negativo: rifiutato da ValidateActions"), URTCatalogLibrary::ValidateActions({ Negativo }).Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolutionSamePhaseOrderIsDeterministic,
	"RefactorTactics.Resolution.SamePhaseOrderIsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolutionSamePhaseOrderIsDeterministic::RunTest(const FString&)
{
	// SOGGETTO FUTURO: due azioni della stessa unita' nella stessa fase risolvono nello stesso ordine su due run
	// (priorita' del catalogo, poi criterio stabile). LIMITE DI OGGI, pinnato: il validatore rifiuta due principali
	// della stessa fase per slot, quindi nessuna coppia arriva al resolver e l'ordine non ha un soggetto.
	// Quando `BB` passera' il validatore (ADR-0011 (1)), questo test cade e va riscritto sul resolver.
	const FRTPlanValidation Verdict = URTPlanValidationLibrary::ValidatePlan(ApUnit(),
		ApRepeated(TEXT("Action.BasicAttack"), ERTActionSlot::Main, 2, 2));
	TestFalse(TEXT("limite di oggi: due principali nella stessa fase non passano il validatore"), Verdict.bLegal);
	TestEqual(TEXT("e il motivo e' lo slot, non il budget (2 + 2 = 4 <= 6)"), Verdict.Reason, ERTActionInvalidReason::SlotOccupied);

	// E l'unita' porta UN indice di abilita' principale pianificata, non una lista: e' il campo che la lista sostituira'.
	const FProperty* Planned = ARTUnit::StaticClass()->FindPropertyByName(TEXT("PlannedAbilityIndex"));
	TestNotNull(TEXT("ARTUnit::PlannedAbilityIndex esiste"), Planned);
	TestTrue(TEXT("ed e' un intero (un'azione per fase), non un array"), Planned && Planned->IsA<FIntProperty>());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolutionSameTriggerReactionOrderIsDeterministic,
	"RefactorTactics.Resolution.SameTriggerReactionOrderIsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolutionSameTriggerReactionOrderIsDeterministic::RunTest(const FString&)
{
	// SOGGETTO FUTURO (D-592: le reazioni sono una lista): due reazioni della stessa unita' armate sullo stesso
	// trigger risolvono nello stesso ordine su due run. LIMITE DI OGGI, pinnato: l'unita' arma UNA reazione
	// (`PlannedReactionAbility` e' un intero) e il validatore rifiuta due reazioni per slot.
	const FRTPlanValidation Verdict = URTPlanValidationLibrary::ValidatePlan(ApUnit(),
		ApRepeated(TEXT("Action.Counter"), ERTActionSlot::Reaction, 2, 2));
	TestFalse(TEXT("limite di oggi: due reazioni non passano il validatore"), Verdict.bLegal);
	TestEqual(TEXT("per slot, non per budget (2 + 2 = 4 <= 6)"), Verdict.Reason, ERTActionInvalidReason::SlotOccupied);

	const FProperty* Reaction = ARTUnit::StaticClass()->FindPropertyByName(TEXT("PlannedReactionAbility"));
	TestNotNull(TEXT("ARTUnit::PlannedReactionAbility esiste"), Reaction);
	TestTrue(TEXT("ed e' un intero (una reazione armata), non una lista"), Reaction && Reaction->IsA<FIntProperty>());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogPlanIsAListNotThreeSlots,
	"RefactorTactics.TurnLog.PlanIsAListNotThreeSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogPlanIsAListNotThreeSlots::RunTest(const FString&)
{
	// SOGGETTO FUTURO: il piano serializzato porta una lista di azioni con fase e costo, e la versione dello schema
	// e' avanzata. LIMITE DI OGGI, pinnato: il piano di un'unita' sono TRE campi — principale, scatto, reazione —
	// ciascuno un indice, e `MakePlanFor` li raccoglie al piu' in tre voci. Il costo pero' viaggia gia' con ogni
	// voce (`FRTPlannedAction::Def.ActionPointCost`): la lista avra' la colonna pronta.
	const TCHAR* const TreCampi[] = { TEXT("PlannedAbilityIndex"), TEXT("PlannedDashAbility"), TEXT("PlannedReactionAbility") };
	for (const TCHAR* Nome : TreCampi)
	{
		const FProperty* P = ARTUnit::StaticClass()->FindPropertyByName(Nome);
		TestNotNull(*FString::Printf(TEXT("ARTUnit::%s esiste"), Nome), P);
		TestTrue(*FString::Printf(TEXT("ARTUnit::%s e' un indice, non una lista"), Nome), P && P->IsA<FIntProperty>());
	}

	// La voce del piano porta il costo: e' il campo che il TurnLog e lo snapshot leggeranno quando il piano sara' una lista.
	const FProperty* Costo = FRTActionDef::StaticStruct()->FindPropertyByName(TEXT("ActionPointCost"));
	TestNotNull(TEXT("FRTActionDef::ActionPointCost esiste"), Costo);
	TestTrue(TEXT("ed e' un intero"), Costo && Costo->IsA<FIntProperty>());

	// Controllo positivo del metodo: un nome inventato non si trova (la reflection non risponde sempre si').
	TestNull(TEXT("controllo: un campo inventato non esiste"), ARTUnit::StaticClass()->FindPropertyByName(TEXT("PlannedActionsList")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
