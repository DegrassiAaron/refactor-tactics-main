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
#include "Ability/Catalog/RTCatalogLibrary.h"
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Turn/Actions/RTActionFallbackLibrary.h"
#include "Turn/Resolution/RTHexSim.h"
#include "Turn/Resolution/RTHexSimLibrary.h" // E38 CP 38.11: lo snapshot d'osservatore e i budget altrui
#include "Turn/Actions/RTPlanValidationLibrary.h"
#include "Turn/Actions/RTActionQueueLibrary.h" // E38 CP 38.14: la numerazione per contenuto, nel test dell'ordine
#include "Turn/Log/RTTurnLogLibrary.h"
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
	TestEqual(TEXT("premessa: il budget di partenza di D-594 e' 6"), URTPlanValidationLibrary::DefaultActionPointBudget, 6);
	TestEqual(TEXT("premessa: un FRTHexSimUnit costruito a mano porta il default"), FRTHexSimUnit().ActionPointBudget, 6);

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

	// --- 2. tre principali del Blast (6) sono legali, quattro (8) no: decide solo il budget -------------------------
	// E38 CP 38.14: il Blast e' aperto, e lo slot non le separa piu'. Fino a CP 38.14 tre principali uscivano SlotOccupied.
	{
		const FRTPlanValidation Tre = URTPlanValidationLibrary::ValidatePlan(ApUnit(),
			ApRepeated(TEXT("Action.BasicAttack"), ERTActionSlot::Main, 2, 3));
		TestTrue(TEXT("6 = 6: legale"), Tre.bLegal);
	}

	// --- 3. esattamente 6 non sfora: la forma piu' cara legale oggi (D B R: 2+2+2) e' accettata -------------------
	{
		const TArray<FRTPlannedAction> Plan = ApPlanForForm(TEXT("DBR"));
		TestEqual(TEXT("premessa: D B R costa 6"), URTPlanValidationLibrary::ActionPointsSpent(Plan), 6);
		TestTrue(TEXT("6 <= 6: legale"), URTPlanValidationLibrary::ValidatePlan(ApUnit(), Plan).bLegal);
	}

	// --- 4. la voce nominata e' la prima che sfora nell'ORDINE DI DICHIARAZIONE, non nell'ordine dell'array ----------
	// E38 CP 38.12: le stesse voci con gli stessi indici, array rovesciato. Costi 2 + 2 + 1 + 2 = 7, dichiarati Move,
	// BasicAttack, Guard, Counter (somme 2, 4, 5, 7): sfora Counter, e lo accusano entrambi gli array. Fino a CP 38.12
	// si accusava la prima che sforava nell'ordine canonico, alfabetico a parita' di slot: Move.
	{
		TArray<FRTPlannedAction> Diretto;
		Diretto.Add(ApEntry(TEXT("Action.Move"), ERTActionSlot::Movement, 2));
		Diretto.Add(ApEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main, 2));
		Diretto.Add(ApEntry(TEXT("Action.Guard"), ERTActionSlot::Main, 1));
		Diretto.Add(ApEntry(TEXT("Action.Counter"), ERTActionSlot::Reaction, 2));
		for (int32 i = 0; i < Diretto.Num(); ++i) { Diretto[i].DeclarationIndex = i; }
		TArray<FRTPlannedAction> Inverso = Diretto;
		Algo::Reverse(Inverso);

		const FRTPlanValidation A = URTPlanValidationLibrary::ValidatePlan(ApUnit(), Diretto);
		const FRTPlanValidation B = URTPlanValidationLibrary::ValidatePlan(ApUnit(), Inverso);
		TestEqual(TEXT("7 > 6 in un verso"), A.Reason, ERTActionInvalidReason::OverBudget);
		TestEqual(TEXT("e nell'altro"), B.Reason, ERTActionInvalidReason::OverBudget);
		TestEqual(TEXT("la voce accusata non dipende dall'ordine dell'array"), A.OffendingActionId, B.OffendingActionId);
		TestEqual(TEXT("ed e' Counter, la prima che sfora nell'ordine di dichiarazione"), A.OffendingActionId, FName(TEXT("Action.Counter")));
		TestEqual(TEXT("con la sua occorrenza"), A.OffendingDeclarationIndex, 3);
	}

	// --- 5. il descrittore del TurnLog ha una frase per il motivo nuovo -------------------------------------------
	TestTrue(TEXT("`OverBudget` si legge nel TurnLog con una frase propria"),
		URTTurnLogLibrary::DescribeInvalidReason(ERTActionInvalidReason::OverBudget).Contains(TEXT("Action Point")));

	// --- 6. controllo positivo: le 23 forme legali di oggi non sforano mai (D-594 non perde nulla) ----------------
	for (const TCHAR* Form : { TEXT("BDR"), TEXT("BMR"), TEXT("CDR"), TEXT("CMR"), TEXT("DPR"), TEXT("MPR") })
	{
		const TArray<FRTPlannedAction> Plan = ApPlanForForm(Form);
		TestTrue(*FString::Printf(TEXT("forma a tre azioni %s: entro il budget"), Form),
			URTPlanValidationLibrary::ActionPointsSpent(Plan) <= URTPlanValidationLibrary::DefaultActionPointBudget);
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
	// IL SOGGETTO VERO, da E38 CP 38.14 (fino a li' questo test pinnava il limite: due principali non passavano il
	// validatore). Due azioni della stessa unita' nella stessa fase sono legali, il piano le porta in due voci con due
	// identita', e l'ordine in cui il resolver le visita e' lo stesso su due esecuzioni e nei due ordini di dichiarazione
	// ([D-615] (5)). Nel mondo lo misurano `Resolution.BlastOrderIgnoresDeclaration` e `Resolution.PrepIgnoresTheDeclaredSequence`.
	const FRTPlanValidation Verdict = URTPlanValidationLibrary::ValidatePlan(ApUnit(),
		ApRepeated(TEXT("Action.BasicAttack"), ERTActionSlot::Main, 2, 2));
	TestTrue(TEXT("due principali del Blast passano il validatore (2 + 2 = 4 <= 6)"), Verdict.bLegal);

	FRTUnitPlan Piano;
	FRTPlanEntry Prima;
	Prima.Class = ERTPlanEntryClass::Main;
	Prima.AbilityIndex = 0;
	FRTPlanEntry Seconda = Prima;
	Seconda.AbilityIndex = 1;
	TestTrue(TEXT("la prima principale entra"), Piano.AddEntry(Prima));
	TestTrue(TEXT("e la seconda pure"), Piano.AddEntry(Seconda));
	if (TestEqual(TEXT("il piano porta due voci"), Piano.GetEntries().Num(), 2))
	{
		TestNotEqual(TEXT("con due identita'"), Piano.GetEntries()[0].Id, Piano.GetEntries()[1].Id);
	}

	auto Ordine = [](bool bRovescio)
	{
		FRTActionInstance A;
		A.Def.ActionId = TEXT("Test.A");
		A.Def.ResolutionPhase = ERTResolutionPhase::Attack;
		A.Def.Priority = 50;
		A.SourceUnitId = 0;
		A.DeclarationIndex = bRovescio ? 1 : 0;
		FRTActionInstance B = A;
		B.Def.ActionId = TEXT("Test.B");
		B.DeclarationIndex = bRovescio ? 0 : 1;
		TArray<FRTActionInstance> Voci = bRovescio ? TArray<FRTActionInstance>{ B, A } : TArray<FRTActionInstance>{ A, B };
		int32 Contatore = 0;
		URTActionQueueLibrary::NumberInstancesByContent(Voci, Contatore);
		URTActionQueueLibrary::SortActionInstances(Voci);
		return Voci[0].Def.ActionId;
	};
	TestEqual(TEXT("stesso ordine su due esecuzioni"), Ordine(false), Ordine(false));
	TestEqual(TEXT("e nei due ordini di dichiarazione"), Ordine(false), Ordine(true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolutionSameTriggerReactionOrderIsDeterministic,
	"RefactorTactics.Resolution.SameTriggerReactionOrderIsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolutionSameTriggerReactionOrderIsDeterministic::RunTest(const FString&)
{
	// E38 CP 38.15 ([D-592], [D-615] (5), [D-619]): due reazioni DISTINTE della stessa unita' sono legali, e il piano le
	// porta tutte e due. L'ordine in cui scattano, priorita' poi `ActionId` e mai la dichiarazione, lo provano nel mondo
	// `Reactions.TwoReactionsOfOneUnitFireInEngineOrder` e `Reactions.CounterBlocksASecondActivation`.
	// ⌫ Fino a CP 38.15 questo test pinnava il limite: due reazioni `SlotOccupied`, e una sola voce nel piano.
	const FRTPlanValidation Verdict = URTPlanValidationLibrary::ValidatePlan(ApUnit(), {
		ApEntry(TEXT("Action.Counter"), ERTActionSlot::Reaction, 2), ApEntry(TEXT("Action.Deflect"), ERTActionSlot::Reaction, 2) });
	TestTrue(TEXT("due reazioni distinte passano il validatore (2 + 2 = 4 <= 6)"), Verdict.bLegal);

	FRTUnitPlan Piano;
	FRTPlanEntry Prima;
	Prima.Class = ERTPlanEntryClass::Reaction;
	Prima.AbilityIndex = 0;
	FRTPlanEntry Seconda = Prima;
	Seconda.AbilityIndex = 1;
	TestTrue(TEXT("la prima reazione entra"), Piano.AddEntry(Prima));
	TestTrue(TEXT("e la seconda pure, in coda"), Piano.AddEntry(Seconda));
	TestEqual(TEXT("il piano porta due voci reazione"), Piano.GetEntries().Num(), 2);
	Piano.RemoveReactionEntries();
	TestEqual(TEXT("e toglierle le toglie tutte e due"), Piano.GetEntries().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogPlanIsAListNotThreeSlots,
	"RefactorTactics.TurnLog.PlanIsAListNotThreeSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogPlanIsAListNotThreeSlots::RunTest(const FString&)
{
	// IL SOGGETTO E' APPARSO (E38 CP 38.12, #4113): il piano di un'unita' e' una LISTA di voci, `FRTUnitPlan::Entries`,
	// e ogni voce porta l'indice dell'abilita' nel kit. I tre campi a casella unica di prima — principale, scatto,
	// reazione — non sono piu' proprieta' del piano: ne restano gli accessori, che leggono la voce della loro classe.
	// SOGGETTO ANCORA FUTURO: il piano serializzato, e la versione dello schema del TurnLog, arrivano con CP 38.13.
	const FArrayProperty* Lista = CastField<FArrayProperty>(FRTUnitPlan::StaticStruct()->FindPropertyByName(TEXT("Entries")));
	const FStructProperty* Voce = Lista ? CastField<FStructProperty>(Lista->Inner) : nullptr;
	TestTrue(TEXT("FRTUnitPlan::Entries e' una lista di FRTPlanEntry"), Voce && Voce->Struct == FRTPlanEntry::StaticStruct());
	const FProperty* Indice = FRTPlanEntry::StaticStruct()->FindPropertyByName(TEXT("AbilityIndex"));
	TestTrue(TEXT("e la voce porta l'indice dell'abilita'"), Indice && Indice->IsA<FIntProperty>());
	for (const TCHAR* Nome : { TEXT("PlannedAbilityIndex"), TEXT("PlannedDashAbility"), TEXT("PlannedReactionAbility") })
	{
		TestNull(*FString::Printf(TEXT("FRTUnitPlan::%s non e' piu' una proprieta': e' un accessore della lista"), Nome),
			FRTUnitPlan::StaticStruct()->FindPropertyByName(Nome));
	}

	// La voce del piano porta il costo: e' il campo che il TurnLog e lo snapshot leggeranno quando il piano sara' una lista.
	const FProperty* Costo = FRTActionDef::StaticStruct()->FindPropertyByName(TEXT("ActionPointCost"));
	TestNotNull(TEXT("FRTActionDef::ActionPointCost esiste"), Costo);
	TestTrue(TEXT("ed e' un intero"), Costo && Costo->IsA<FIntProperty>());

	// Controllo positivo del metodo: un nome inventato non si trova (la reflection non risponde sempre si').
	TestNull(TEXT("controllo: un campo inventato non esiste"), FRTUnitPlan::StaticStruct()->FindPropertyByName(TEXT("PlannedActionsList")));
	TestNotNull(TEXT("controllo: e la ricerca trova un campo che c'e'"), FRTUnitPlan::StaticStruct()->FindPropertyByName(TEXT("PlannedCell")));
	return true;
}

// E38 CP 38.9 (#4110) — l'Ultimate costa 3 in qualunque fase ([D-613] (4)). Righe sintetiche: nessuna riga del catalogo
// di oggi e' un'Ultimate, e le prime arrivano col kit di Riktor ([D-612] (1)).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogUltimateCostsThreeInEveryPhase,
	"RefactorTactics.Catalog.UltimateCostsThreeInEveryPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogUltimateCostsThreeInEveryPhase::RunTest(const FString&)
{
	struct FCaso { ERTResolutionPhase Fase; const TCHAR* Nome; int32 CostoDiClasse; };
	const FCaso Casi[] = {
		{ ERTResolutionPhase::Preparation,    TEXT("Preparation"),    1 },
		{ ERTResolutionPhase::FastMovement,   TEXT("FastMovement"),   2 },
		{ ERTResolutionPhase::Control,        TEXT("Control"),        2 },
		{ ERTResolutionPhase::Attack,         TEXT("Attack"),         2 },
		{ ERTResolutionPhase::NormalMovement, TEXT("NormalMovement"), 2 },
		{ ERTResolutionPhase::Environment,    TEXT("Environment"),    2 },
	};
	for (const FCaso& Caso : Casi)
	{
		FRTActionDef Riga;
		Riga.ActionId = FName(TEXT("Test.UltimateSintetica"));
		Riga.ResolutionPhase = Caso.Fase;
		Riga.bIsUltimate = true;
		TestEqual(*FString::Printf(TEXT("%s: un'Ultimate costa 3"), Caso.Nome),
			URTCatalogLibrary::DefaultActionPointCost(Riga), 3);
		// Controllo positivo: la stessa riga non marcata costa quanto la sua classe di D-594 (1). E' il marcatore a
		// cambiare il numero, non la fase.
		Riga.bIsUltimate = false;
		TestEqual(*FString::Printf(TEXT("%s: la stessa riga non marcata costa %d"), Caso.Nome, Caso.CostoDiClasse),
			URTCatalogLibrary::DefaultActionPointCost(Riga), Caso.CostoDiClasse);
	}

	// Il ramo dell'Ultimate viene prima di quello della reazione: una reazione marcata costa 3, non 2 (l'ordine dei
	// rami e' una proposta di CP 38.9; D-613 (4) dice solo «qualunque sia la sua fase»).
	FRTActionDef Reazione;
	Reazione.ActionId = FName(TEXT("Test.ReazioneUltimate"));
	Reazione.Slot = ERTActionSlot::Reaction;
	Reazione.bIsUltimate = true;
	TestEqual(TEXT("una reazione marcata Ultimate costa 3"), URTCatalogLibrary::DefaultActionPointCost(Reazione), 3);
	TestEqual(TEXT("la costante e' quella di D-613 (4)"), URTCatalogLibrary::ActionPointCostUltimate, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogUltimateWithAnotherCostIsRejected,
	"RefactorTactics.Catalog.UltimateWithAnotherCostIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogUltimateWithAnotherCostIsRejected::RunTest(const FString&)
{
	// Il caso che il controllo vuole prendere: una riga marcata Ultimate dopo la costruzione, che tiene il costo di
	// classe. Per una riga di Prep e' 1; qui 2, il «+1» di D-594 (3) che D-613 (4) ha emendato.
	FRTActionDef Riga;
	Riga.ActionId = FName(TEXT("Test.UltimateSintetica"));
	Riga.ResolutionPhase = ERTResolutionPhase::Preparation;
	Riga.bIsUltimate = true;
	Riga.ActionPointCost = 2;
	const TArray<FString> ConDue = URTCatalogLibrary::ValidateActions({ Riga });
	TestEqual(TEXT("un'Ultimate che costa 2 da' un errore"), ConDue.Num(), 1);
	TestTrue(TEXT("e l'errore nomina D-613 (4)"), ConDue.Num() == 1 && ConDue[0].Contains(TEXT("D-613 (4)")));

	Riga.ActionPointCost = URTCatalogLibrary::DefaultActionPointCost(Riga);
	TestEqual(TEXT("riscritta con DefaultActionPointCost costa 3"), Riga.ActionPointCost, 3);
	TestEqual(TEXT("e la stessa riga a 3 e' valida"), URTCatalogLibrary::ValidateActions({ Riga }).Num(), 0);

	// Controllo positivo: il controllo guarda il marcatore, non il numero. Una riga non marcata che dichiara 3 resta
	// valida, perche' una riga puo' dichiarare un costo proprio.
	Riga.bIsUltimate = false;
	TestEqual(TEXT("una riga non marcata a 3 e' valida"), URTCatalogLibrary::ValidateActions({ Riga }).Num(), 0);
	return true;
}

// E38 CP 38.11 (#4112) — il budget del turno e' un numero dell'unita' ([D-613] (1)), e `ValidatePlan` legge quello.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanBudgetIsReadFromTheUnit,
	"RefactorTactics.Plan.BudgetIsReadFromTheUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanBudgetIsReadFromTheUnit::RunTest(const FString&)
{
	// Criterio 1: principale + reazione + Move, 2 + 2 + 2 = 6.
	TArray<FRTPlannedAction> Sei;
	for (const TCHAR Classe : { TEXT('B'), TEXT('R'), TEXT('M') })
	{
		FRTPlannedAction Voce;
		if (!TestTrue(TEXT("premessa: la classe esiste"), ApEntryForClass(Classe, Voce))) { return false; }
		Sei.Add(Voce);
	}
	FRTHexSimUnit Unita = ApUnit();
	Unita.ActionPointBudget = 5;
	const FRTPlanValidation ConCinque = URTPlanValidationLibrary::ValidatePlan(Unita, Sei);
	TestTrue(TEXT("con budget 5 il piano da 6 e' OverBudget"),
		!ConCinque.bLegal && ConCinque.Reason == ERTActionInvalidReason::OverBudget);
	Unita.ActionPointBudget = 6;
	TestTrue(TEXT("con budget 6 lo stesso piano e' legale"), URTPlanValidationLibrary::ValidatePlan(Unita, Sei).bLegal);

	// Su voci sintetiche, il verso opposto: un piano da 7 (movimento 2 + principale 3 + reazione 2).
	TArray<FRTPlannedAction> Sette;
	FRTPlannedAction Movimento;
	FRTPlannedAction Reazione;
	ApEntryForClass(TEXT('M'), Movimento);
	ApEntryForClass(TEXT('R'), Reazione);
	Sette.Add(Movimento);
	Sette.Add(ApEntry(TEXT("Test.PrincipaleDaTre"), ERTActionSlot::Main, 3));
	Sette.Add(Reazione);
	Unita.ActionPointBudget = 7;
	TestTrue(TEXT("con budget 7 il piano da 7 e' legale"), URTPlanValidationLibrary::ValidatePlan(Unita, Sette).bLegal);
	Unita.ActionPointBudget = 6;
	const FRTPlanValidation ConSei = URTPlanValidationLibrary::ValidatePlan(Unita, Sette);
	TestTrue(TEXT("con budget 6 lo stesso piano e' OverBudget"),
		!ConSei.bLegal && ConSei.Reason == ERTActionInvalidReason::OverBudget);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTObserverSnapshotHidesForeignBudgets,
	"RefactorTactics.Privacy.ObserverSnapshotHidesForeignBudgets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTObserverSnapshotHidesForeignBudgets::RunTest(const FString&)
{
	// Criterio 5: due mondi che differiscono SOLO per il budget di un'unita' della squadra 1 danno alla squadra 0 lo
	// stesso snapshot ([D-371]).
	auto Mondo = [](int32 BudgetAvversario)
	{
		FRTHexSimUnit Mia(0, FRTCellId(0, 0), 4, true);
		Mia.TeamId = 0;
		Mia.ActionPointBudget = 6;
		FRTHexSimUnit Sua(1, FRTCellId(3, 0), 4, true);
		Sua.TeamId = 1;
		Sua.ActionPointBudget = BudgetAvversario;
		return TArray<FRTHexSimUnit>{ Mia, Sua };
	};
	const FRTHexSnapshot A = URTHexSimLibrary::MakeSnapshot(nullptr, Mondo(5), {}, 0);
	const FRTHexSnapshot B = URTHexSimLibrary::MakeSnapshot(nullptr, Mondo(4), {}, 0);
	if (!TestEqual(TEXT("premessa: due unita' per snapshot"), A.Units.Num(), 2)) { return false; }
	for (int32 i = 0; i < A.Units.Num(); ++i)
	{
		TestEqual(*FString::Printf(TEXT("unita' %d: stesso budget nei due mondi, per la squadra 0"), i),
			A.Units[i].ActionPointBudget, B.Units[i].ActionPointBudget);
	}
	TestEqual(TEXT("il budget dell'avversario e' nascosto (INDEX_NONE)"), A.Units[1].ActionPointBudget, (int32)INDEX_NONE);
	TestEqual(TEXT("il budget della propria unita' resta"), A.Units[0].ActionPointBudget, 6);

	// Controllo positivo: lo snapshot onnisciente li distingue, quindi l'uguaglianza qui sopra e' il filtro.
	const FRTHexSnapshot OA = URTHexSimLibrary::MakeSnapshot(nullptr, Mondo(5), {}, RTObserver::Omniscient);
	const FRTHexSnapshot OB = URTHexSimLibrary::MakeSnapshot(nullptr, Mondo(4), {}, RTObserver::Omniscient);
	TestNotEqual(TEXT("controllo: l'onnisciente vede i due budget diversi"), OA.Units[1].ActionPointBudget, OB.Units[1].ActionPointBudget);
	return true;
}

// ── E38 CP 38.12 (#4113): il piano e' una lista, l'ordine di dichiarazione si mostra e non decide ─────────────────────
namespace
{
	/** Le voci con il loro ordine di dichiarazione: la posizione nell'array, prima di qualunque permutazione. */
	TArray<FRTPlannedAction> Dichiarate(std::initializer_list<FRTPlannedAction> Voci)
	{
		TArray<FRTPlannedAction> Out(Voci);
		for (int32 i = 0; i < Out.Num(); ++i) { Out[i].DeclarationIndex = i; }
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanVerdictNamesTheOccurrence,
	"RefactorTactics.Plan.VerdictNamesTheOccurrence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanVerdictNamesTheOccurrence::RunTest(const FString&)
{
	// Criterio 4: con due voci dello stesso `ActionId` il verdetto nomina l'indice dell'occorrenza. Voci sintetiche:
	// in questo CP i mutatori non scrivono due voci della stessa classe.
	{
		// Slot: due movimenti, la classe che resta chiusa fino a CP 38.16 (la principale e' aperta da CP 38.14, la reazione da
		// CP 38.15). Tiene lo slot la prima dichiarata, la seconda e' la colpevole.
		const TArray<FRTPlannedAction> Plan = Dichiarate({
			ApEntry(TEXT("Action.Move"), ERTActionSlot::Movement, 1),
			ApEntry(TEXT("Action.Counter"), ERTActionSlot::Reaction, 1),
			ApEntry(TEXT("Action.Move"), ERTActionSlot::Movement, 1) });
		TArray<FRTPlannedAction> Rovesciato = Plan;
		Algo::Reverse(Rovesciato);
		const TArray<FRTPlannedAction>* const Due[] = { &Plan, &Rovesciato };
		for (const TArray<FRTPlannedAction>* P : Due)
		{
			const FRTPlanValidation V = URTPlanValidationLibrary::ValidatePlan(ApUnit(), *P);
			TestEqual(TEXT("due movimenti: SlotOccupied"), V.Reason, ERTActionInvalidReason::SlotOccupied);
			TestEqual(TEXT("la colpevole e' la seconda occorrenza"), V.OffendingDeclarationIndex, 2);
			TestEqual(TEXT("e la detentrice la prima"), V.HolderDeclarationIndex, 0);
			TestEqual(TEXT("tutte e due Move"), V.HolderActionId, V.OffendingActionId);
		}
	}
	{
		// Ricarica: delle due copie, e' in ricarica solo quella dichiarata per seconda.
		TArray<FRTPlannedAction> Plan = Dichiarate({
			ApEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main, 1),
			ApEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main, 1) });
		Plan[1].CooldownRemaining = 2;
		const FRTPlanValidation V = URTPlanValidationLibrary::ValidatePlan(ApUnit(), Plan);
		TestEqual(TEXT("una copia in ricarica: OnCooldown"), V.Reason, ERTActionInvalidReason::OnCooldown);
		TestEqual(TEXT("ed e' nominata la copia in ricarica"), V.OffendingDeclarationIndex, 1);
		TestEqual(TEXT("senza detentore"), V.HolderDeclarationIndex, (int32)INDEX_NONE);
	}
	{
		// Controllo: un piano legale non nomina nessuna occorrenza.
		const FRTPlanValidation V = URTPlanValidationLibrary::ValidatePlan(ApUnit(),
			Dichiarate({ ApEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main, 2) }));
		TestTrue(TEXT("controllo: legale"), V.bLegal);
		TestEqual(TEXT("e nessuna occorrenza nominata"), V.OffendingDeclarationIndex, (int32)INDEX_NONE);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanOverBudgetBlamesTheEntryThatCrossesTheBudget,
	"RefactorTactics.Plan.OverBudgetBlamesTheEntryThatCrossesTheBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanOverBudgetBlamesTheEntryThatCrossesTheBudget::RunTest(const FString&)
{
	// Criterio 5: si accusa la prima voce, nell'ordine di dichiarazione, la cui somma corrente supera il budget.
	const FRTPlannedAction Move = ApEntry(TEXT("Action.Move"), ERTActionSlot::Movement, 2);
	const FRTPlannedAction Attack = ApEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main, 2);
	const FRTPlannedAction Guard = ApEntry(TEXT("Action.Guard"), ERTActionSlot::Main, 1);
	const FRTPlannedAction Counter = ApEntry(TEXT("Action.Counter"), ERTActionSlot::Reaction, 2);
	const FRTPlannedAction Brace = ApEntry(TEXT("Action.Brace"), ERTActionSlot::Main, 1);

	struct FCaso { const TCHAR* Nome; TArray<FRTPlannedAction> Piano; FName Colpevole; int32 Indice; };
	const FCaso Casi[] = {
		// 2 + 2 + 1 + 2: somme 2, 4, 5, 7.
		{ TEXT("Move, BasicAttack, Guard, Counter"), Dichiarate({ Move, Attack, Guard, Counter }), TEXT("Action.Counter"), 3 },
		// L'ordine opposto: 2 + 1 + 2 + 2, somme 2, 3, 5, 7.
		{ TEXT("Counter, Guard, BasicAttack, Move"), Dichiarate({ Counter, Guard, Attack, Move }), TEXT("Action.Move"), 3 },
		// Una voce dichiarata dopo lo sforamento: 2 + 2 + 2 + 1 + 1, somme 2, 4, 6, 7, 8. Sfora Guard, non Brace.
		{ TEXT("Move, BasicAttack, Counter, Guard, Brace"), Dichiarate({ Move, Attack, Counter, Guard, Brace }), TEXT("Action.Guard"), 3 },
	};
	for (const FCaso& C : Casi)
	{
		const FRTPlanValidation V = URTPlanValidationLibrary::ValidatePlan(ApUnit(), C.Piano);
		TestEqual(*FString::Printf(TEXT("%s: OverBudget"), C.Nome), V.Reason, ERTActionInvalidReason::OverBudget);
		TestEqual(*FString::Printf(TEXT("%s: colpevole"), C.Nome), V.OffendingActionId, C.Colpevole);
		TestEqual(*FString::Printf(TEXT("%s: occorrenza"), C.Nome), V.OffendingDeclarationIndex, C.Indice);
		// Le stesse voci con gli stessi indici, array rovesciato: stessa accusa (criterio 3).
		TArray<FRTPlannedAction> Rovesciato = C.Piano;
		Algo::Reverse(Rovesciato);
		const FRTPlanValidation R = URTPlanValidationLibrary::ValidatePlan(ApUnit(), Rovesciato);
		TestEqual(*FString::Printf(TEXT("%s, array rovesciato: stesso colpevole"), C.Nome), R.OffendingActionId, C.Colpevole);
		TestEqual(*FString::Printf(TEXT("%s, array rovesciato: stessa occorrenza"), C.Nome), R.OffendingDeclarationIndex, C.Indice);
	}
	// Criterio 8, su voci sintetiche: un piano illegale dichiarato in due ordini ha lo stesso verdetto e lo stesso motivo.
	const FRTPlanValidation A = URTPlanValidationLibrary::ValidatePlan(ApUnit(), Casi[0].Piano);
	const FRTPlanValidation B = URTPlanValidationLibrary::ValidatePlan(ApUnit(), Casi[1].Piano);
	TestEqual(TEXT("due ordini di dichiarazione: stesso verdetto"), A.bLegal, B.bLegal);
	TestEqual(TEXT("e stesso motivo"), A.Reason, B.Reason);
	return true;
}

// ── E38 CP 38.13 (#4114): la regola della copia ([D-614]) e il Prep aperto ────────────────────────────────────────────
namespace
{
	/** La riga di catalogo come voce dichiarata: il `Def` vero (fase, slot, costo, cooldown), come lo compone `MakePlanFor`. */
	FRTPlannedAction CatalogRowEntry(const TCHAR* ActionId, int32 DeclarationIndex)
	{
		FRTPlannedAction Entry;
		Entry.Def = URTCatalogLibrary::FindCoreAction(FName(ActionId));
		if (Entry.Def.ActionId.IsNone())
		{
			for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
			{
				for (const URTActionData* Action : Hero->Actions)
				{
					if (Action && Action->Def.ActionId == FName(ActionId)) { Entry.Def = Action->Def; }
				}
			}
		}
		Entry.CatalogCooldownTurns = Entry.Def.CooldownTurns; // nessuna variante: la riga e' il `Def`
		Entry.DeclarationIndex = DeclarationIndex;
		return Entry;
	}

	TArray<FRTPlannedAction> CatalogRows(std::initializer_list<const TCHAR*> Ids)
	{
		TArray<FRTPlannedAction> Plan;
		for (const TCHAR* Id : Ids) { Plan.Add(CatalogRowEntry(Id, Plan.Num())); }
		return Plan;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanCopiesWithoutCooldownAreLegalTest,
	"RefactorTactics.Plan.CopiesWithoutCooldownAreLegal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanCopiesWithoutCooldownAreLegalTest::RunTest(const FString&)
{
	// Criteri 1 e 2: il Prep e' aperto ([D-584] (1)), e una copia senza ricarica e' ammessa ([D-614]).
	const TArray<FRTPlannedAction> DueGuardie = CatalogRows({ TEXT("Action.Guard"), TEXT("Action.Guard"), TEXT("Hero.Riktor.ImpactShot") });
	TestEqual(TEXT("premessa: la Guard non ha ricarica"), DueGuardie[0].CatalogCooldownTurns, 0);
	TestEqual(TEXT("premessa: la Guard e' di Prep"), URTCatalogLibrary::MapResolutionPhase(DueGuardie[0].Def.ResolutionPhase), ERTMatchPhase::Prep);
	TestEqual(TEXT("1 + 1 + 2 = 4"), URTPlanValidationLibrary::ActionPointsSpent(DueGuardie), 4);
	const FRTPlanValidation Due = URTPlanValidationLibrary::ValidatePlan(ApUnit(), DueGuardie);
	TestTrue(TEXT("Guard + Guard + ImpactShot e' legale"), Due.bLegal);

	// Una voce di Prep accanto alla principale del Blast non e' piu' `SlotOccupied`.
	TestTrue(TEXT("Guard + ImpactShot e' legale"), URTPlanValidationLibrary::ValidatePlan(ApUnit(),
		CatalogRows({ TEXT("Action.Guard"), TEXT("Hero.Riktor.ImpactShot") })).bLegal);

	// E38 CP 38.14: anche il Blast e' aperto. Due voci diverse del Blast, e due copie senza ricarica, sono legali.
	TestTrue(TEXT("due voci diverse del Blast sono legali"), URTPlanValidationLibrary::ValidatePlan(ApUnit(),
		CatalogRows({ TEXT("Hero.Riktor.ImpactShot"), TEXT("Action.Interact") })).bLegal);
	TestTrue(TEXT("e due attacchi base, senza ricarica (criterio 1 di CP 38.14)"), URTPlanValidationLibrary::ValidatePlan(ApUnit(),
		CatalogRows({ TEXT("Action.BasicAttack"), TEXT("Action.BasicAttack") })).bLegal);
	// E38 CP 38.15: anche la reazione e' aperta. Due copie senza ricarica sono legali ([D-614]); quante scattano lo dice
	// [D-619], una volta per reazione. Voci sintetiche: la riga di catalogo di una reazione ha una ricarica.
	TestTrue(TEXT("due reazioni senza ricarica sono legali"), URTPlanValidationLibrary::ValidatePlan(ApUnit(),
		ApRepeated(TEXT("Test.Reazione"), ERTActionSlot::Reaction, 2, 2)).bLegal);
	// Controllo: la classe ancora chiusa, il movimento, resta sotto il vincolo di slot fino a CP 38.16.
	const FRTPlanValidation DueMovimenti = URTPlanValidationLibrary::ValidatePlan(ApUnit(),
		ApRepeated(TEXT("Test.Movimento"), ERTActionSlot::Movement, 1, 2));
	TestEqual(TEXT("controllo: due movimenti senza ricarica restano SlotOccupied"), DueMovimenti.Reason, ERTActionInvalidReason::SlotOccupied);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanSecondCopyWithCooldownIsRejectedTest,
	"RefactorTactics.Plan.SecondCopyWithCooldownIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanSecondCopyWithCooldownIsRejectedTest::RunTest(const FString&)
{
	// Criteri 1 e 17: una seconda copia di un'azione con ricarica e' `RepeatedWithCooldown`, in qualunque fase.
	const FRTPlanValidation Brace = URTPlanValidationLibrary::ValidatePlan(ApUnit(),
		CatalogRows({ TEXT("Action.Brace"), TEXT("Action.Brace") }));
	TestEqual(TEXT("premessa: il Brace ha ricarica 1"), CatalogRowEntry(TEXT("Action.Brace"), 0).CatalogCooldownTurns, 1);
	TestEqual(TEXT("Brace + Brace: RepeatedWithCooldown"), Brace.Reason, ERTActionInvalidReason::RepeatedWithCooldown);
	TestEqual(TEXT("accusata la seconda copia, la prima che ripete l'azione"), Brace.OffendingDeclarationIndex, 1);
	TestTrue(TEXT("il motivo non ha un detentore"), Brace.HolderActionId.IsNone());

	// La copia vietata non pesa nel budget: con un solo AP il motivo resta la copia, non la somma.
	FRTHexSimUnit UnSoloPunto = ApUnit();
	UnSoloPunto.ActionPointBudget = 1;
	TestEqual(TEXT("la copia precede il budget"), URTPlanValidationLibrary::ValidatePlan(UnSoloPunto,
		CatalogRows({ TEXT("Action.Brace"), TEXT("Action.Brace") })).Reason, ERTActionInvalidReason::RepeatedWithCooldown);

	// [D-615] (4): l'Overwatch ha ricarica 1, quindi la sua seconda copia e' vietata.
	TestEqual(TEXT("Overwatch + Overwatch: RepeatedWithCooldown"), URTPlanValidationLibrary::ValidatePlan(ApUnit(),
		CatalogRows({ TEXT("Action.Overwatch"), TEXT("Action.Overwatch") })).Reason, ERTActionInvalidReason::RepeatedWithCooldown);

	// [D-615] (1): la regola vale per ogni voce, anche nelle fasi ancora chiuse, e precede lo slot.
	for (const TCHAR* Id : { TEXT("Hero.Riktor.MortarShot"), TEXT("Hero.Riktor.Interposition"), TEXT("Hero.Riktor.Ram") })
	{
		TestTrue(*FString::Printf(TEXT("premessa: %s ha ricarica"), Id), CatalogRowEntry(Id, 0).CatalogCooldownTurns > 0);
		TestEqual(*FString::Printf(TEXT("%s due volte: RepeatedWithCooldown, non SlotOccupied"), Id),
			URTPlanValidationLibrary::ValidatePlan(ApUnit(), CatalogRows({ Id, Id })).Reason,
			ERTActionInvalidReason::RepeatedWithCooldown);
	}

	// L'accusa e' resa: la stessa coppia dichiarata a rovescio accusa sempre la seconda dichiarata.
	TArray<FRTPlannedAction> Rovescio = CatalogRows({ TEXT("Action.Brace"), TEXT("Action.Brace") });
	Swap(Rovescio[0], Rovescio[1]);
	TestEqual(TEXT("l'ordine dell'array non sposta l'accusa"),
		URTPlanValidationLibrary::ValidatePlan(ApUnit(), Rovescio).OffendingDeclarationIndex, 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
