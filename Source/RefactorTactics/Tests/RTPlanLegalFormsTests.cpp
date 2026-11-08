// Le forme di turno legali OGGI restano legali quando il validatore cambia modello (D-584, CP 38.6 / #4012).
//
// Una «forma» e' il multinsieme delle classi di fase delle azioni dichiarate in un turno: P (principale nel
// Prep), D (mobilita' speciale nel Dash), B (principale nel Blast), M (camminare nel Move), R (reazione),
// C (principale ambientale che risolve nel Cleanup). Con gli slot di D-028 sono legali le 23 forme elencate
// qui sotto: al piu' una fra D e M, al piu' una fra P, B e C, al piu' una R. La lista e' l'uscita di
// `python tools/balance/ap_forms_sim.py` (proposta E38, `docs/roadmap/plans/e38-action-point-numeri-simulazione-2026-11-08.md` §1).
//
// 🔑 Questo e' il primo dei cinque asserti che D-584 (3) chiede PRIMA dei numeri, ed e' l'unico scrivibile verde
// oggi: fissa che il passaggio al pool di Action Point non toglie nulla di cio' che e' legale adesso. Gli altri
// (somma oltre il budget rifiutata, ordine deterministico dentro la fase e fra reazioni sullo stesso trigger,
// piano come lista, costo dichiarato per ogni azione) non hanno ancora un soggetto nel codice e sono descritti
// in #4012: scriverli ora li renderebbe rossi per costruzione o vuoti.
//
// ⚠️ Il controllo positivo in coda fissa che il banco MISURA: due forme illegali per struttura oggi (`BB`, `DM`)
// sono rifiutate con `SlotOccupied`. Sono esattamente due di quelle che D-584 (1) rendera' legali: quando
// il validatore cambia, questo test lo dice (il controllo diventa rosso e va riscritto, non spento).

#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTPlanValidationLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Unita' viva con un budget di movimento largo: la legalita' di forma non legge i Movement Point (D-190). */
	FRTHexSimUnit FormUnit()
	{
		return FRTHexSimUnit(0, FRTCellId(0, 0), /*MoveBudget*/ 8, /*bAlive*/ true);
	}

	FRTPlannedAction FormEntry(const TCHAR* ActionId, ERTActionSlot Slot)
	{
		FRTPlannedAction Entry;
		Entry.Def.ActionId = FName(ActionId);
		Entry.Def.Slot = Slot;
		Entry.CooldownRemaining = 0;
		return Entry;
	}

	/** Un'azione rappresentativa per classe, con lo slot che occupa OGGI (catalogo azioni §«Slot per turno»). */
	bool EntryForClass(TCHAR Class, FRTPlannedAction& Out)
	{
		switch (Class)
		{
		case TEXT('P'): Out = FormEntry(TEXT("Action.Guard"), ERTActionSlot::Main); return true;
		case TEXT('D'): Out = FormEntry(TEXT("Action.Dodge"), ERTActionSlot::Movement); return true;
		case TEXT('B'): Out = FormEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main); return true;
		case TEXT('M'): Out = FormEntry(TEXT("Action.Move"), ERTActionSlot::Movement); return true;
		case TEXT('R'): Out = FormEntry(TEXT("Action.Counter"), ERTActionSlot::Reaction); return true;
		case TEXT('C'): Out = FormEntry(TEXT("Action.CreateWater"), ERTActionSlot::Main); return true;
		default: return false;
		}
	}

	TArray<FRTPlannedAction> PlanForForm(const FString& Form)
	{
		TArray<FRTPlannedAction> Plan;
		for (const TCHAR C : Form)
		{
			FRTPlannedAction Entry;
			if (EntryForClass(C, Entry))
			{
				Plan.Add(Entry);
			}
		}
		return Plan;
	}

	/** Le 23 forme legali con gli slot di D-028: uscita di `tools/balance/ap_forms_sim.py`. */
	const TCHAR* const LegalFormsToday[] = {
		TEXT("B"), TEXT("C"), TEXT("D"), TEXT("M"), TEXT("P"), TEXT("R"),
		TEXT("BD"), TEXT("BM"), TEXT("BR"), TEXT("CD"), TEXT("CM"), TEXT("CR"),
		TEXT("DP"), TEXT("DR"), TEXT("MP"), TEXT("MR"), TEXT("PR"),
		TEXT("BDR"), TEXT("BMR"), TEXT("CDR"), TEXT("CMR"), TEXT("DPR"), TEXT("MPR"),
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanLegalTodayStaysLegal,
	"RefactorTactics.Plan.LegalTodayStaysLegal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanLegalTodayStaysLegal::RunTest(const FString&)
{
	// --- premessa: la lista e' quella del comando, 23 forme, nessuna ripetuta -----------------------------
	TSet<FString> Distinct;
	for (const TCHAR* Form : LegalFormsToday)
	{
		Distinct.Add(Form);
	}
	TestEqual(TEXT("premessa: le forme legali oggi sono 23 e distinte"), Distinct.Num(), 23);

	// --- 1. ogni forma legale oggi e' accettata dal validatore ---------------------------------------------
	for (const TCHAR* Form : LegalFormsToday)
	{
		const TArray<FRTPlannedAction> Plan = PlanForForm(Form);
		TestEqual(*FString::Printf(TEXT("forma %s: una voce per classe"), Form), Plan.Num(), FCString::Strlen(Form));
		const FRTPlanValidation Verdict = URTPlanValidationLibrary::ValidatePlan(FormUnit(), Plan);
		TestTrue(*FString::Printf(TEXT("forma %s: legale"), Form), Verdict.bLegal);
		TestEqual(*FString::Printf(TEXT("forma %s: nessun motivo di rifiuto"), Form), Verdict.Reason,
			ERTActionInvalidReason::None);
	}

	// --- 2. controllo positivo: due forme che OGGI sono illegali per struttura ---------------------------
	// `BB` (due principali) e `DM` (scatto e camminata nello stesso turno) sono rifiutate con `SlotOccupied`.
	// D-584 (1) le rendera' legali: quando accade, questo controllo lo dice e va riscritto, non spento.
	for (const TCHAR* Form : { TEXT("BB"), TEXT("DM") })
	{
		const FRTPlanValidation Verdict = URTPlanValidationLibrary::ValidatePlan(FormUnit(), PlanForForm(Form));
		TestFalse(*FString::Printf(TEXT("controllo %s: oggi illegale"), Form), Verdict.bLegal);
		TestEqual(*FString::Printf(TEXT("controllo %s: il motivo e' lo slot occupato"), Form), Verdict.Reason,
			ERTActionInvalidReason::SlotOccupied);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
