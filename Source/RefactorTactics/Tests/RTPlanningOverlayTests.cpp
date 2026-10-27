#include "Misc/AutomationTest.h"
#include "Player/RTPlanningOverlay.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS

// =====================================================================================================
// `#3511` — gli ingressi delle aree di pianificazione, in UNA struct con uguaglianza.
//
// La struct e' pura: nessun mondo, nessun attore. Qui si prova cio' che il controller poi si limita a usare — quali
// campi fanno ricalcolare il PIANO, quali solo l'hover, e quando le aree si mostrano.
//
// 🔑 **Un test per CAMPO, non uno per la struct.** «Due struct diverse sono diverse» passa anche se il confronto guarda
// la meta' dei campi; qui ogni campo viene cambiato da solo, e il test dice quale.
// =====================================================================================================

namespace
{
	/** Due oggetti vivi e distinti: l'unita' e' un puntatore debole, e uno nullo non fa da campione di niente. */
	const UObject* PlanOverlayUnitA() { return GetTransientPackage(); }
	const UObject* PlanOverlayUnitB() { return UObject::StaticClass()->GetDefaultObject(); }

	/** Ogni campo a un valore NON di default: cambiarne uno solo e' sempre una differenza, mai un no-op. */
	FRTPlanningOverlayInputs PlanOverlayBase()
	{
		FRTPlanningOverlayInputs Inputs;
		Inputs.Unit = PlanOverlayUnitA();
		Inputs.bUnitAlive = true;
		Inputs.Context = ERTPointerContext::Targeting;
		Inputs.ArmedAbilityIndex = 2;
		Inputs.ArmedTargetKind = ERTPointerTargetKind::Unit;
		Inputs.HoveredCell = FRTCellId(1, 2, 0);
		Inputs.bHoveredValid = true;
		Inputs.FacingSector = 3;
		Inputs.ActiveLayer = 1;
		Inputs.PlanRevision = 4;
		Inputs.TurnIndex = 5;
		Inputs.bMatchEnded = false;
		return Inputs;
	}

	struct FPlanOverlayFieldCase
	{
		const TCHAR* Name;
		/** Vero se cambiare il campo deve far ricalcolare il PIANO (ventaglio e portata); falso se e' solo l'hover. */
		bool bChangesPlan;
		TFunction<void(FRTPlanningOverlayInputs&)> Mutate;
	};

	TArray<FPlanOverlayFieldCase> PlanOverlayFieldCases()
	{
		return {
			{ TEXT("Unit"), true, [](FRTPlanningOverlayInputs& I) { I.Unit = PlanOverlayUnitB(); } },
			{ TEXT("bUnitAlive"), true, [](FRTPlanningOverlayInputs& I) { I.bUnitAlive = false; } },
			// Il contesto conta nell'uguaglianza ma al livello del piano conta solo se cambia fra mostrare e non mostrare le aree.
			{ TEXT("Context (fra contesti che mostrano le aree)"), false,
				[](FRTPlanningOverlayInputs& I) { I.Context = ERTPointerContext::Pathing; } },
			{ TEXT("Context (da mostrare a non mostrare)"), true,
				[](FRTPlanningOverlayInputs& I) { I.Context = ERTPointerContext::ResolutionPlayback; } },
			{ TEXT("ArmedAbilityIndex"), true, [](FRTPlanningOverlayInputs& I) { I.ArmedAbilityIndex = 3; } },
			{ TEXT("ArmedTargetKind"), true, [](FRTPlanningOverlayInputs& I) { I.ArmedTargetKind = ERTPointerTargetKind::Cell; } },
			{ TEXT("ActiveLayer"), false, [](FRTPlanningOverlayInputs& I) { I.ActiveLayer = 0; } },
			{ TEXT("PlanRevision"), true, [](FRTPlanningOverlayInputs& I) { I.PlanRevision += 1; } },
			{ TEXT("TurnIndex"), true, [](FRTPlanningOverlayInputs& I) { I.TurnIndex += 1; } },
			{ TEXT("bMatchEnded"), true, [](FRTPlanningOverlayInputs& I) { I.bMatchEnded = true; } },
			{ TEXT("HoveredCell"), false, [](FRTPlanningOverlayInputs& I) { I.HoveredCell = FRTCellId(0, 0, 0); } },
			{ TEXT("bHoveredValid"), false, [](FRTPlanningOverlayInputs& I) { I.bHoveredValid = false; } },
			{ TEXT("FacingSector"), false, [](FRTPlanningOverlayInputs& I) { I.FacingSector = INDEX_NONE; } },
		};
	}
}

/**
 * **`#3511` — ogni campo cambiato da solo rompe l'uguaglianza, e solo i campi del PIANO rompono `SamePlanLevel`.**
 *
 * `SamePlanLevel` e' cio' che tiene il criterio di `#711`: un cambio della sola cella sotto il cursore non rifa' il
 * ventaglio. Un campo nuovo della struct che non compare ne' nell'uno ne' nell'altro confronto e' un ingresso che non fa
 * mai ricalcolare niente — e questo test, che elenca i campi, e' dove se ne accorge chi lo aggiunge.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningOverlayInputsComparisonTest,
	"RefactorTactics.PlanningOverlay.EveryInputFieldBreaksEqualityAndOnlyPlanFieldsBreakThePlanLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningOverlayInputsComparisonTest::RunTest(const FString&)
{
	const FRTPlanningOverlayInputs Base = PlanOverlayBase();

	// CONTROLLO POSITIVO: una copia identica e' uguale, a entrambi i livelli. Senza, ogni «diverso» sotto sarebbe gratis.
	const FRTPlanningOverlayInputs Copy = PlanOverlayBase();
	TestTrue(TEXT("controllo: due struct costruite uguali sono uguali"), Base == Copy);
	TestFalse(TEXT("controllo: e non sono diverse"), Base != Copy);
	TestTrue(TEXT("controllo: e hanno lo stesso piano"), Base.SamePlanLevel(Copy));

	for (const FPlanOverlayFieldCase& Case : PlanOverlayFieldCases())
	{
		FRTPlanningOverlayInputs Changed = PlanOverlayBase();
		Case.Mutate(Changed);

		TestFalse(*FString::Printf(TEXT("%s: cambiato da solo rompe l'uguaglianza"), Case.Name), Base == Changed);
		TestTrue(*FString::Printf(TEXT("%s: e `!=` lo dice"), Case.Name), Base != Changed);
		TestEqual(*FString::Printf(TEXT("%s: rompe il livello del piano solo se e' un campo del piano"), Case.Name),
			!Base.SamePlanLevel(Changed), Case.bChangesPlan);
	}
	return true;
}

/**
 * **`#3511` — un'unita' distrutta non e' uguale a nessuna, e una nulla non e' un'unita'.**
 *
 * L'unita' e' un puntatore DEBOLE perche' una nuda potrebbe puntare a un'unita' distrutta e il cui indirizzo e' stato
 * riusato: il confronto la direbbe invariata. Qui la distinzione che conta e' fra «nessuna unita'» e «un'unita'».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningOverlayInputsNoUnitTest,
	"RefactorTactics.PlanningOverlay.NoUnitIsNotAUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningOverlayInputsNoUnitTest::RunTest(const FString&)
{
	FRTPlanningOverlayInputs None = PlanOverlayBase();
	None.Unit = nullptr;
	const FRTPlanningOverlayInputs WithUnit = PlanOverlayBase();

	TestFalse(TEXT("un'unita' e nessuna unita' sono ingressi diversi"), None == WithUnit);
	TestFalse(TEXT("e le aree non si mostrano senza un'unita', qualunque cosa dicano gli altri campi"), None.ShowsAreas());
	TestTrue(TEXT("controllo: con un'unita' viva, in targeting, si mostrano"), WithUnit.ShowsAreas());
	return true;
}

/**
 * **`#3511` — le aree si mostrano nei quattro contesti di pianificazione e in nessun altro.**
 *
 * Gli otto contesti sono elencati TUTTI, con la risposta attesa a fianco: `ShowsAreas` ha uno `switch` senza `default`,
 * quindi un contesto nuovo non compila finche' non dice se il mondo e' scrivibile, e questa tabella e' dove lo dice anche
 * al test. ⚠️ Il conteggio contro l'enum pinna il caso in cui qualcuno aggiunge un nono contesto e dimentica la tabella.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningOverlayShowsAreasByContextTest,
	"RefactorTactics.PlanningOverlay.ShowsAreasOnlyWhilePlanning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningOverlayShowsAreasByContextTest::RunTest(const FString&)
{
	struct FCase { ERTPointerContext Context; bool bShows; };
	const FCase Cases[] = {
		{ ERTPointerContext::IdleSelection, false },
		{ ERTPointerContext::Planning, true },
		{ ERTPointerContext::Pathing, true },
		{ ERTPointerContext::Targeting, true },
		{ ERTPointerContext::Facing, true },
		{ ERTPointerContext::ResolutionPlayback, false },
		{ ERTPointerContext::ReactionWindow, false },
		{ ERTPointerContext::Modal, false },
	};

	const UEnum* ContextEnum = StaticEnum<ERTPointerContext>();
	if (!TestNotNull(TEXT("l'enum dei contesti e' riflesso"), ContextEnum)) { return false; }
	TestEqual(TEXT("la tabella elenca TUTTI i contesti dell'enum"),
		static_cast<int32>(UE_ARRAY_COUNT(Cases)), ContextEnum->NumEnums() - 1);

	for (const FCase& Case : Cases)
	{
		FRTPlanningOverlayInputs Inputs = PlanOverlayBase();
		Inputs.Context = Case.Context;
		TestEqual(*FString::Printf(TEXT("%s: le aree si mostrano solo a mondo scrivibile"),
			*ContextEnum->GetNameStringByValue(static_cast<int64>(Case.Context))), Inputs.ShowsAreas(), Case.bShows);
	}
	return true;
}

/**
 * **`#3511` — KO dell'unita' e fine partita spengono le aree anche in un contesto di pianificazione.**
 *
 * Sono le due uscite che il puntatore NON dichiara: un'unita' caduta resta selezionata a fine playback, e a partita finita
 * `GetPointerContext` resta in `Planning` di proposito. Per questo sono ingressi a parte e non contesti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningOverlayShowsAreasExitsNotInContextTest,
	"RefactorTactics.PlanningOverlay.KoAndMatchEndSwitchTheAreasOffWhateverTheContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningOverlayShowsAreasExitsNotInContextTest::RunTest(const FString&)
{
	for (const ERTPointerContext Context : { ERTPointerContext::Planning, ERTPointerContext::Pathing,
		ERTPointerContext::Targeting, ERTPointerContext::Facing })
	{
		FRTPlanningOverlayInputs Ko = PlanOverlayBase();
		Ko.Context = Context;
		TestTrue(TEXT("controllo: viva e a partita in corso, si mostrano"), Ko.ShowsAreas());

		Ko.bUnitAlive = false;
		TestFalse(TEXT("l'unita' selezionata e' KO: non si mostrano"), Ko.ShowsAreas());

		FRTPlanningOverlayInputs Ended = PlanOverlayBase();
		Ended.Context = Context;
		Ended.bMatchEnded = true;
		TestFalse(TEXT("la partita e' finita: non si mostrano"), Ended.ShowsAreas());
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
