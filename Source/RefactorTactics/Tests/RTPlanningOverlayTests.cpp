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
		// Il fuoco (#3750): tutti i campi a un valore NON di default. Nel contesto di base (`Targeting`) il fuoco non si
		// mostra, ma i campi contano nel confronto: lo dice la tabella qui sotto.
		Inputs.FocusSubject = PlanOverlayUnitB();
		Inputs.FocusMove.Cell = FRTCellId(3, 1, 0);
		Inputs.FocusMove.BaseMoveRange = 4;
		Inputs.FocusMove.MoveCostModifier = 1;
		Inputs.FocusMove.bRunDenied = true;
		Inputs.FocusKnowledgeStamp = 7;
		Inputs.bFocusOnMap = true;
		// L'hover su una skill (#3750, fetta D): valori NON di default ma INATTIVI — l'unita' dell'hover e' un'altra da `Unit`, e
		// la barra e' di chi si comanda — cosi' gli altri test della famiglia restano quelli di prima.
		Inputs.HoverSkillUnit = PlanOverlayUnitB();
		Inputs.SkillRange.Origin = FRTCellId(2, 1, 0);
		Inputs.SkillRange.RangeCells = 4;
		Inputs.SkillRange.MinRangeCells = 1;
		Inputs.SkillRange.Policy = ERTLineOfSightPolicy::NotRequired;
		Inputs.SkillKnowledgeStamp = 9;
		return Inputs;
	}

	/** Ingressi in cui il fuoco TIENE la mappa: contesto `Planning`, niente armato, soggetto osservato, area sulla mappa. */
	FRTPlanningOverlayInputs PlanOverlayFocused()
	{
		FRTPlanningOverlayInputs Inputs = PlanOverlayBase();
		Inputs.Context = ERTPointerContext::Planning;
		Inputs.ArmedAbilityIndex = INDEX_NONE;
		Inputs.ArmedTargetKind = ERTPointerTargetKind::None;
		return Inputs;
	}

	/** Ingressi in cui il cursore e' su una skill dell'unita' comandata (contesto `Planning`, il fuoco ancora sulla mappa). */
	FRTPlanningOverlayInputs PlanOverlayHovering()
	{
		FRTPlanningOverlayInputs Inputs = PlanOverlayFocused();
		Inputs.HoverSkillUnit = Inputs.Unit;
		Inputs.SkillRange.Origin = FRTCellId(1, 1, 0);
		Inputs.SkillRange.RangeCells = 4;
		Inputs.SkillRange.MinRangeCells = 0;
		Inputs.SkillRange.Policy = ERTLineOfSightPolicy::Required;
		Inputs.SkillKnowledgeStamp = 5;
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
			// #3750: il fuoco cambia COSA si calcola, non dove sta il cursore.
			{ TEXT("FocusSubject"), true, [](FRTPlanningOverlayInputs& I) { I.FocusSubject = nullptr; } },
			{ TEXT("FocusMove.Cell"), true, [](FRTPlanningOverlayInputs& I) { I.FocusMove.Cell = FRTCellId(0, 0, 0); } },
			{ TEXT("FocusMove.BaseMoveRange"), true, [](FRTPlanningOverlayInputs& I) { I.FocusMove.BaseMoveRange += 1; } },
			{ TEXT("FocusMove.MoveCostModifier"), true, [](FRTPlanningOverlayInputs& I) { I.FocusMove.MoveCostModifier = 0; } },
			{ TEXT("FocusMove.bRunDenied"), true, [](FRTPlanningOverlayInputs& I) { I.FocusMove.bRunDenied = false; } },
			{ TEXT("FocusKnowledgeStamp"), true, [](FRTPlanningOverlayInputs& I) { I.FocusKnowledgeStamp += 1; } },
			{ TEXT("bFocusOnMap"), true, [](FRTPlanningOverlayInputs& I) { I.bFocusOnMap = false; } },
			// #3750, fetta D: anche la portata di una skill cambia COSA si calcola.
			{ TEXT("HoverSkillUnit"), true, [](FRTPlanningOverlayInputs& I) { I.HoverSkillUnit = nullptr; } },
			{ TEXT("SkillRange.Origin"), true, [](FRTPlanningOverlayInputs& I) { I.SkillRange.Origin = FRTCellId(0, 0, 0); } },
			{ TEXT("SkillRange.RangeCells"), true, [](FRTPlanningOverlayInputs& I) { I.SkillRange.RangeCells += 1; } },
			{ TEXT("SkillRange.MinRangeCells"), true, [](FRTPlanningOverlayInputs& I) { I.SkillRange.MinRangeCells = 0; } },
			{ TEXT("SkillRange.Policy"), true, [](FRTPlanningOverlayInputs& I) { I.SkillRange.Policy = ERTLineOfSightPolicy::Required; } },
			{ TEXT("SkillKnowledgeStamp"), true, [](FRTPlanningOverlayInputs& I) { I.SkillKnowledgeStamp += 1; } },
			// #3748: la riga della scheda e' un secondo produttore della stessa sorgente, e cambia COSA si calcola.
			{ TEXT("bSkillHoverFromCard"), true, [](FRTPlanningOverlayInputs& I) { I.bSkillHoverFromCard = true; } },
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

// =====================================================================================================
// `#3750`, fetta C di F5 — l'arbitro dell'anteprima: chi scrive la mappa fra piano e fuoco.
// =====================================================================================================

namespace
{
	const TCHAR* PlanOverlaySourceName(ERTPreviewSource Source)
	{
		switch (Source)
		{
		case ERTPreviewSource::None:  return TEXT("None");
		case ERTPreviewSource::Plan:  return TEXT("Plan");
		case ERTPreviewSource::Focus: return TEXT("Focus");
		case ERTPreviewSource::SkillHover: return TEXT("SkillHover");
		}
		return TEXT("?");
	}
}

/**
 * **`#3750` — il fuoco prende la mappa solo dove l'ispettore e' vivo, e il piano ci torna da solo quando il fuoco finisce.**
 *
 * Gli otto contesti sono elencati TUTTI, con la sorgente attesa a fianco, con e senza l'area del fuoco sulla mappa:
 * `ContextAllowsFocus` ha uno `switch` senza `default`, e il conteggio contro l'enum pinna il nono contesto dimenticato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningOverlayActiveSourceTest,
	"RefactorTactics.PlanningOverlay.TheFocusTakesTheMapOnlyWhereTheInspectorIsLive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningOverlayActiveSourceTest::RunTest(const FString&)
{
	struct FCase { ERTPointerContext Context; ERTPreviewSource WithFocus; ERTPreviewSource WithoutFocus; };
	const FCase Cases[] = {
		{ ERTPointerContext::IdleSelection,      ERTPreviewSource::Focus, ERTPreviewSource::None },
		{ ERTPointerContext::Planning,           ERTPreviewSource::Focus, ERTPreviewSource::Plan },
		// Gia' dopo un gesto di pianificazione: l'area del fuoco e' uscita, il piano tiene la mappa.
		{ ERTPointerContext::Pathing,            ERTPreviewSource::Plan,  ERTPreviewSource::Plan },
		{ ERTPointerContext::Targeting,          ERTPreviewSource::Plan,  ERTPreviewSource::Plan },
		{ ERTPointerContext::Facing,             ERTPreviewSource::Plan,  ERTPreviewSource::Plan },
		// Mondo in sola lettura: niente aree, e il fuoco resta spento in playback per scelta della sessione.
		{ ERTPointerContext::ResolutionPlayback, ERTPreviewSource::None,  ERTPreviewSource::None },
		{ ERTPointerContext::ReactionWindow,     ERTPreviewSource::None,  ERTPreviewSource::None },
		{ ERTPointerContext::Modal,              ERTPreviewSource::None,  ERTPreviewSource::None },
	};

	const UEnum* ContextEnum = StaticEnum<ERTPointerContext>();
	if (!TestNotNull(TEXT("l'enum dei contesti e' riflesso"), ContextEnum)) { return false; }
	TestEqual(TEXT("la tabella elenca TUTTI i contesti dell'enum"),
		static_cast<int32>(UE_ARRAY_COUNT(Cases)), ContextEnum->NumEnums() - 1);

	for (const FCase& Case : Cases)
	{
		const FString Name = ContextEnum->GetNameStringByValue(static_cast<int64>(Case.Context));

		FRTPlanningOverlayInputs With = PlanOverlayFocused();
		With.Context = Case.Context;
		TestEqual(*FString::Printf(TEXT("%s: con l'area del fuoco sulla mappa"), *Name),
			FString(PlanOverlaySourceName(With.ActiveSource())), FString(PlanOverlaySourceName(Case.WithFocus)));

		FRTPlanningOverlayInputs Without = PlanOverlayFocused();
		Without.Context = Case.Context;
		Without.bFocusOnMap = false;
		TestEqual(*FString::Printf(TEXT("%s: senza l'area del fuoco sulla mappa"), *Name),
			FString(PlanOverlaySourceName(Without.ActiveSource())), FString(PlanOverlaySourceName(Case.WithoutFocus)));
	}

	// L'HOVER SU UNA SKILL (#3750, fetta D): sta sopra il fuoco e sopra il piano, ma solo dove la barra e' attiva — gli stessi
	// quattro contesti in cui le aree di pianificazione si mostrano. Dove il fuoco c'e' (IdleSelection) e la barra no, vince il fuoco.
	{
		struct FHoverCase { ERTPointerContext Context; ERTPreviewSource Expected; };
		const FHoverCase HoverCases[] = {
			{ ERTPointerContext::IdleSelection,      ERTPreviewSource::Focus },
			{ ERTPointerContext::Planning,           ERTPreviewSource::SkillHover },
			{ ERTPointerContext::Pathing,            ERTPreviewSource::SkillHover },
			{ ERTPointerContext::Targeting,          ERTPreviewSource::SkillHover },
			{ ERTPointerContext::Facing,             ERTPreviewSource::SkillHover },
			{ ERTPointerContext::ResolutionPlayback, ERTPreviewSource::None },
			{ ERTPointerContext::ReactionWindow,     ERTPreviewSource::None },
			{ ERTPointerContext::Modal,              ERTPreviewSource::None },
		};
		TestEqual(TEXT("la tabella dell'hover elenca TUTTI i contesti dell'enum"),
			static_cast<int32>(UE_ARRAY_COUNT(HoverCases)), ContextEnum->NumEnums() - 1);
		for (const FHoverCase& Case : HoverCases)
		{
			FRTPlanningOverlayInputs Hovering = PlanOverlayHovering();
			Hovering.Context = Case.Context;
			TestEqual(*FString::Printf(TEXT("%s: con una skill sotto il cursore"),
				*ContextEnum->GetNameStringByValue(static_cast<int64>(Case.Context))),
				FString(PlanOverlaySourceName(Hovering.ActiveSource())), FString(PlanOverlaySourceName(Case.Expected)));
		}

		// Le uscite: l'hover non e' un lasciapassare.
		FRTPlanningOverlayInputs OtherUnit = PlanOverlayHovering();
		OtherUnit.HoverSkillUnit = PlanOverlayUnitB();
		TestEqual(TEXT("una skill di un'altra unita' non tiene la mappa: resta il fuoco"),
			FString(PlanOverlaySourceName(OtherUnit.ActiveSource())), FString(TEXT("Focus")));

		FRTPlanningOverlayInputs NoRange = PlanOverlayHovering();
		NoRange.SkillRange.RangeCells = 0;
		TestEqual(TEXT("una skill senza portata non tiene la mappa"),
			FString(PlanOverlaySourceName(NoRange.ActiveSource())), FString(TEXT("Focus")));

		FRTPlanningOverlayInputs NoHover = PlanOverlayHovering();
		NoHover.HoverSkillUnit = nullptr;
		TestEqual(TEXT("senza hover torna il fuoco (la sorgente di prima)"),
			FString(PlanOverlaySourceName(NoHover.ActiveSource())), FString(TEXT("Focus")));
		NoHover.bFocusOnMap = false;
		TestEqual(TEXT("...e senza fuoco, il piano"), FString(PlanOverlaySourceName(NoHover.ActiveSource())), FString(TEXT("Plan")));

		FRTPlanningOverlayInputs Dead = PlanOverlayHovering();
		Dead.bUnitAlive = false;
		TestFalse(TEXT("un'unita' comandata caduta non ha una barra che mostri una portata"), Dead.ShowsSkillHover());

		FRTPlanningOverlayInputs Ended = PlanOverlayHovering();
		Ended.bMatchEnded = true;
		TestEqual(TEXT("a partita finita non si mostra niente, hover compreso"),
			FString(PlanOverlaySourceName(Ended.ActiveSource())), FString(TEXT("None")));
	}

	// Le uscite che il contesto non dichiara.
	FRTPlanningOverlayInputs NoSubject = PlanOverlayFocused();
	NoSubject.FocusSubject = nullptr;
	TestEqual(TEXT("un soggetto non osservato (nullo) non tiene la mappa: torna il piano"),
		FString(PlanOverlaySourceName(NoSubject.ActiveSource())), FString(TEXT("Plan")));

	FRTPlanningOverlayInputs Ended = PlanOverlayFocused();
	Ended.bMatchEnded = true;
	TestEqual(TEXT("a partita finita non si mostra niente, fuoco compreso"),
		FString(PlanOverlaySourceName(Ended.ActiveSource())), FString(TEXT("None")));

	// Il fuoco non chiede un'unita' comandata: si ispeziona anche senza selezione.
	FRTPlanningOverlayInputs NoUnit = PlanOverlayFocused();
	NoUnit.Context = ERTPointerContext::IdleSelection;
	NoUnit.Unit = nullptr;
	NoUnit.bUnitAlive = false;
	TestEqual(TEXT("senza unita' selezionata il fuoco tiene la mappa"),
		FString(PlanOverlaySourceName(NoUnit.ActiveSource())), FString(TEXT("Focus")));
	NoUnit.bFocusOnMap = false;
	TestEqual(TEXT("controllo positivo: senza fuoco e senza unita' non si mostra niente"),
		FString(PlanOverlaySourceName(NoUnit.ActiveSource())), FString(TEXT("None")));
	return true;
}

/**
 * **`#3748` — la riga di una skill nella SCHEDA tiene la mappa per l'unita' a fuoco, anche se non e' la selezione.**
 *
 * Il proprietario non e' `Unit` (o non c'e' nessuna selezione): a farlo passare e' `bSkillHoverFromCard`, che scrive il solo risolutore del
 * controller dopo i suoi cancelli. L'arbitro non si apre con un `HoverSkillUnit` qualunque, e non si apre in playback, in finestra di
 * reazione e in modale. Sta sopra il fuoco e sopra il piano, come l'hover della barra.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningOverlayCardHoverTest,
	"RefactorTactics.PlanningOverlay.ACardRowHoverTakesTheMapForTheInspectedUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningOverlayCardHoverTest::RunTest(const FString&)
{
	struct FCase { ERTPointerContext Context; ERTPreviewSource Expected; };
	const FCase Cases[] = {
		// Dove la scheda si vede e il mondo si guarda per pianificare: la riga sta sopra il fuoco (dove c'e') e sopra il piano.
		{ ERTPointerContext::IdleSelection,      ERTPreviewSource::SkillHover },
		{ ERTPointerContext::Planning,           ERTPreviewSource::SkillHover },
		{ ERTPointerContext::Pathing,            ERTPreviewSource::SkillHover },
		{ ERTPointerContext::Targeting,          ERTPreviewSource::SkillHover },
		{ ERTPointerContext::Facing,             ERTPreviewSource::SkillHover },
		// Aree di pianificazione spente in playback, scheda nascosta in finestra di reazione e in modale.
		{ ERTPointerContext::ResolutionPlayback, ERTPreviewSource::None },
		{ ERTPointerContext::ReactionWindow,     ERTPreviewSource::None },
		{ ERTPointerContext::Modal,              ERTPreviewSource::None },
	};

	const UEnum* ContextEnum = StaticEnum<ERTPointerContext>();
	if (!TestNotNull(TEXT("l'enum dei contesti e' riflesso"), ContextEnum)) { return false; }
	TestEqual(TEXT("la tabella elenca TUTTI i contesti dell'enum"),
		static_cast<int32>(UE_ARRAY_COUNT(Cases)), ContextEnum->NumEnums() - 1);

	int32 Hovering = 0;
	int32 Silent = 0;
	for (const FCase& Case : Cases)
	{
		const FString Name = ContextEnum->GetNameStringByValue(static_cast<int64>(Case.Context));

		// Un'unita' a fuoco che non e' la selezione (`HoverSkillUnit` e' un'altra da `Unit`), ammessa dal risolutore.
		FRTPlanningOverlayInputs FromCard = PlanOverlayFocused();
		FromCard.Context = Case.Context;
		FromCard.bSkillHoverFromCard = true;
		TestEqual(*FString::Printf(TEXT("%s: con una riga della scheda sotto il cursore"), *Name),
			FString(PlanOverlaySourceName(FromCard.ActiveSource())), FString(PlanOverlaySourceName(Case.Expected)));
		(Case.Expected == ERTPreviewSource::SkillHover ? Hovering : Silent) += 1;

		// Lo stesso hover senza l'ammissione del risolutore non apre niente: resta la sorgente di prima.
		FRTPlanningOverlayInputs NotAdmitted = FromCard;
		NotAdmitted.bSkillHoverFromCard = false;
		FRTPlanningOverlayInputs Before = PlanOverlayFocused();
		Before.Context = Case.Context;
		Before.HoverSkillUnit = nullptr;
		TestEqual(*FString::Printf(TEXT("%s: senza l'ammissione del risolutore torna la sorgente di prima"), *Name),
			FString(PlanOverlaySourceName(NotAdmitted.ActiveSource())), FString(PlanOverlaySourceName(Before.ActiveSource())));
	}
	TestTrue(TEXT("anti-vacuita': la tabella ha contesti che mostrano e contesti che tacciono"), Hovering > 0 && Silent > 0);

	// Le uscite: l'ammissione non e' un lasciapassare.
	FRTPlanningOverlayInputs NoRange = PlanOverlayFocused();
	NoRange.bSkillHoverFromCard = true;
	NoRange.SkillRange.RangeCells = 0;
	TestEqual(TEXT("una skill senza portata non tiene la mappa: resta il fuoco"),
		FString(PlanOverlaySourceName(NoRange.ActiveSource())), FString(TEXT("Focus")));

	FRTPlanningOverlayInputs NoOwner = PlanOverlayFocused();
	NoOwner.bSkillHoverFromCard = true;
	NoOwner.HoverSkillUnit = nullptr;
	TestEqual(TEXT("senza proprietario della skill non tiene la mappa: resta il fuoco"),
		FString(PlanOverlaySourceName(NoOwner.ActiveSource())), FString(TEXT("Focus")));

	FRTPlanningOverlayInputs Ended = PlanOverlayFocused();
	Ended.bSkillHoverFromCard = true;
	Ended.bMatchEnded = true;
	TestEqual(TEXT("a partita finita non si mostra niente, riga della scheda compresa"),
		FString(PlanOverlaySourceName(Ended.ActiveSource())), FString(TEXT("None")));

	// Non chiede un'unita' comandata: si ispeziona, e si guarda una skill, anche senza selezione.
	FRTPlanningOverlayInputs NoSelection = PlanOverlayFocused();
	NoSelection.Context = ERTPointerContext::IdleSelection;
	NoSelection.Unit = nullptr;
	NoSelection.bUnitAlive = false;
	NoSelection.bSkillHoverFromCard = true;
	TestEqual(TEXT("senza unita' selezionata la riga della scheda tiene la mappa"),
		FString(PlanOverlaySourceName(NoSelection.ActiveSource())), FString(TEXT("SkillHover")));
	NoSelection.bSkillHoverFromCard = false;
	TestEqual(TEXT("controllo positivo: senza l'ammissione, senza selezione, resta il fuoco"),
		FString(PlanOverlaySourceName(NoSelection.ActiveSource())), FString(TEXT("Focus")));
	return true;
}

/**
 * **`#3750` — l'area del fuoco lascia la mappa al primo gesto di pianificazione, e non torna da sola.**
 *
 * Ogni gesto della spec §3 regola 4 (armare, posare un waypoint, dichiarare un verso) e ogni altra uscita, ciascuno da solo:
 * dalla stessa base, il gesto spegne; lo stato che torna uguale NON riaccende; una nuova fissazione, si'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusLatchTest,
	"RefactorTactics.PlanningOverlay.TheFocusAreaLeavesAtTheFirstPlanningGestureAndDoesNotComeBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusLatchTest::RunTest(const FString&)
{
	const FRTPlanningOverlayInputs Rest = PlanOverlayFocused();

	// CONTROLLI POSITIVI: senza questi ogni «spenta» sotto sarebbe gratis.
	{
		FRTFocusLatch Latch;
		TestFalse(TEXT("nessuna fissazione: l'area non e' sulla mappa"), Latch.Observe(0, Rest));
		TestTrue(TEXT("una fissazione di un soggetto osservato la accende"), Latch.Observe(1, Rest));
		TestTrue(TEXT("a ingressi invariati resta accesa"), Latch.Observe(1, Rest));

		FRTPlanningOverlayInputs Hover = Rest;
		Hover.HoveredCell = FRTCellId(0, 0, 0);
		Hover.bHoveredValid = false;
		Hover.FacingSector = 2;
		Hover.ActiveLayer = 0;
		TestTrue(TEXT("muovere il cursore, il settore o il piano attivo non e' un gesto di pianificazione"), Latch.Observe(1, Hover));

		// Nemmeno portare il cursore su una skill (#3750, fetta D): l'hover e' temporaneo, e quando finisce torna il fuoco.
		FRTPlanningOverlayInputs OnASkill = PlanOverlayHovering();
		TestTrue(TEXT("il cursore su una skill dell'unita' comandata non spegne l'area del fuoco"), Latch.Observe(1, OnASkill));
		TestTrue(TEXT("...e quando ne esce l'area e' ancora li'"), Latch.Observe(1, Rest));
	}

	struct FGesture { const TCHAR* Name; TFunction<void(FRTPlanningOverlayInputs&)> Mutate; };
	const FGesture Gestures[] = {
		{ TEXT("arma un'azione"),                      [](FRTPlanningOverlayInputs& I) { I.ArmedAbilityIndex = 3; } },
		{ TEXT("posa un waypoint (la revisione sale)"), [](FRTPlanningOverlayInputs& I) { I.PlanRevision += 1; } },
		{ TEXT("passa a un'altra unita' comandata"),   [](FRTPlanningOverlayInputs& I) { I.Unit = PlanOverlayUnitB(); } },
		{ TEXT("perde la selezione"),                  [](FRTPlanningOverlayInputs& I) { I.Unit = nullptr; } },
		{ TEXT("inizia un turno nuovo"),               [](FRTPlanningOverlayInputs& I) { I.TurnIndex += 1; } },
		{ TEXT("entra in Pathing"),                    [](FRTPlanningOverlayInputs& I) { I.Context = ERTPointerContext::Pathing; } },
		{ TEXT("entra in Targeting"),                  [](FRTPlanningOverlayInputs& I) { I.Context = ERTPointerContext::Targeting; } },
		{ TEXT("dichiara un verso (Facing)"),          [](FRTPlanningOverlayInputs& I) { I.Context = ERTPointerContext::Facing; } },
		{ TEXT("parte il playback"),                   [](FRTPlanningOverlayInputs& I) { I.Context = ERTPointerContext::ResolutionPlayback; } },
		{ TEXT("il soggetto non e' piu' osservato"),   [](FRTPlanningOverlayInputs& I) { I.FocusSubject = nullptr; } },
		{ TEXT("la partita finisce"),                  [](FRTPlanningOverlayInputs& I) { I.bMatchEnded = true; } },
	};

	for (const FGesture& Gesture : Gestures)
	{
		FRTFocusLatch Latch;
		if (!TestTrue(*FString::Printf(TEXT("%s: premessa, l'area e' sulla mappa"), Gesture.Name), Latch.Observe(1, Rest)))
		{
			continue;
		}

		FRTPlanningOverlayInputs After = Rest;
		Gesture.Mutate(After);
		TestFalse(*FString::Printf(TEXT("%s: l'area lascia la mappa"), Gesture.Name), Latch.Observe(1, After));
		TestFalse(*FString::Printf(TEXT("%s: e non torna da sola quando lo stato torna com'era"), Gesture.Name), Latch.Observe(1, Rest));
		TestTrue(*FString::Printf(TEXT("%s: una nuova fissazione la riporta"), Gesture.Name), Latch.Observe(2, Rest));
	}

	// SOSPESA, NON SPENTA (spec §3 regole 5 e 11): una finestra di reazione e un modale nascondono l'ispettore senza chiuderlo, e
	// non sono un gesto di pianificazione. L'area non si vede finche' sono aperti e TORNA quando si chiudono, come la scheda.
	for (const ERTPointerContext Suspending : { ERTPointerContext::ReactionWindow, ERTPointerContext::Modal })
	{
		const FString Name = StaticEnum<ERTPointerContext>()->GetNameStringByValue(static_cast<int64>(Suspending));
		FRTFocusLatch Latch;
		TestTrue(*FString::Printf(TEXT("%s: premessa, l'area e' sulla mappa"), *Name), Latch.Observe(1, Rest));

		FRTPlanningOverlayInputs While = Rest;
		While.Context = Suspending;
		const bool bStillLatched = Latch.Observe(1, While);
		TestTrue(*FString::Printf(TEXT("%s: il latch resta acceso"), *Name), bStillLatched);
		While.bFocusOnMap = bStillLatched;
		TestEqual(*FString::Printf(TEXT("%s: ma l'area non si vede (nessuna sorgente scrive la mappa)"), *Name),
			FString(PlanOverlaySourceName(While.ActiveSource())), FString(TEXT("None")));

		FRTPlanningOverlayInputs After = Rest;
		After.bFocusOnMap = Latch.Observe(1, Rest);
		TestEqual(*FString::Printf(TEXT("%s: chiuso, l'area TORNA da sola (non era un gesto di pianificazione)"), *Name),
			FString(PlanOverlaySourceName(After.ActiveSource())), FString(TEXT("Focus")));

		// Controllo positivo: il sospendere non e' un lasciapassare. Un gesto vero DURANTE la sospensione spegne.
		FRTFocusLatch Gestured;
		Gestured.Observe(1, Rest);
		FRTPlanningOverlayInputs Mixed = Rest;
		Mixed.Context = Suspending;
		Mixed.PlanRevision += 1;
		TestFalse(*FString::Printf(TEXT("%s: un gesto durante la sospensione spegne l'area"), *Name), Gestured.Observe(1, Mixed));
		TestFalse(*FString::Printf(TEXT("%s: e non torna alla chiusura"), *Name), Gestured.Observe(1, Rest));
	}

	// Una fissazione su un soggetto non osservato non accende niente, e il soggetto che compare dopo non la accende da solo.
	{
		FRTPlanningOverlayInputs Unseen = Rest;
		Unseen.FocusSubject = nullptr;
		FRTFocusLatch Latch;
		TestFalse(TEXT("fissare un soggetto non osservato non accende l'area"), Latch.Observe(1, Unseen));
		TestFalse(TEXT("e nemmeno quando poi compare, senza una nuova fissazione"), Latch.Observe(1, Rest));
	}
	return true;
}

/**
 * **`#3750` — col fuoco in mano, l'hover non rifa' l'area e cio' da cui l'area dipende si'.**
 *
 * E' il criterio di `#711` esteso al fuoco: la cella sotto il cursore, il settore e il piano attivo non fanno ricalcolare;
 * il soggetto che si muove, prende `Slow` o perde la corsa, e la vista che cambia, si'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningOverlayFocusPlanLevelTest,
	"RefactorTactics.PlanningOverlay.TheFocusAreaIsRedoneOnlyForWhatItDependsOn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningOverlayFocusPlanLevelTest::RunTest(const FString&)
{
	const FRTPlanningOverlayInputs Base = PlanOverlayFocused();
	if (!TestEqual(TEXT("premessa: nello stato di base il fuoco tiene la mappa"),
		FString(PlanOverlaySourceName(Base.ActiveSource())), FString(TEXT("Focus")))) { return false; }

	struct FChange { const TCHAR* Name; bool bRedoesTheArea; TFunction<void(FRTPlanningOverlayInputs&)> Mutate; };
	const FChange Changes[] = {
		{ TEXT("HoveredCell"),                false, [](FRTPlanningOverlayInputs& I) { I.HoveredCell = FRTCellId(0, 0, 0); } },
		{ TEXT("bHoveredValid"),              false, [](FRTPlanningOverlayInputs& I) { I.bHoveredValid = false; } },
		{ TEXT("FacingSector"),               false, [](FRTPlanningOverlayInputs& I) { I.FacingSector = INDEX_NONE; } },
		{ TEXT("ActiveLayer"),                false, [](FRTPlanningOverlayInputs& I) { I.ActiveLayer = 0; } },
		{ TEXT("FocusMove.Cell (si muove)"),  true,  [](FRTPlanningOverlayInputs& I) { I.FocusMove.Cell = FRTCellId(0, 0, 0); } },
		{ TEXT("FocusMove.MoveCostModifier"), true,  [](FRTPlanningOverlayInputs& I) { I.FocusMove.MoveCostModifier = 0; } },
		{ TEXT("FocusMove.bRunDenied"),       true,  [](FRTPlanningOverlayInputs& I) { I.FocusMove.bRunDenied = false; } },
		{ TEXT("FocusMove.BaseMoveRange"),    true,  [](FRTPlanningOverlayInputs& I) { I.FocusMove.BaseMoveRange += 1; } },
		{ TEXT("FocusKnowledgeStamp (la vista cambia)"), true, [](FRTPlanningOverlayInputs& I) { I.FocusKnowledgeStamp += 1; } },
	};
	for (const FChange& Change : Changes)
	{
		FRTPlanningOverlayInputs Changed = Base;
		Change.Mutate(Changed);
		TestFalse(*FString::Printf(TEXT("%s: cambiato da solo rompe l'uguaglianza"), Change.Name), Base == Changed);
		TestEqual(*FString::Printf(TEXT("%s: rifa' l'area solo se e' cio' da cui l'area dipende"), Change.Name),
			!Base.SamePlanLevel(Changed), Change.bRedoesTheArea);
	}
	return true;
}

/**
 * **`#3750` — con una skill sotto il cursore, il cursore sulla MAPPA non rifa' la portata e cio' da cui la portata dipende si'.**
 *
 * Il criterio di `#711` esteso all'hover: la cella sotto il cursore, il settore e il piano attivo non fanno ricalcolare; l'origine,
 * la portata, la portata minima, la policy di linea e la vista, si'. E' anche il motivo per cui lo slot non e' un campo: due skill
 * con gli stessi valori sono la stessa portata per la mappa, e passare dall'una all'altra non ricalcola niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningOverlayHoverPlanLevelTest,
	"RefactorTactics.PlanningOverlay.TheSkillRangeIsRedoneOnlyForWhatItDependsOn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningOverlayHoverPlanLevelTest::RunTest(const FString&)
{
	const FRTPlanningOverlayInputs Base = PlanOverlayHovering();
	if (!TestEqual(TEXT("premessa: nello stato di base la skill tiene la mappa"),
		FString(PlanOverlaySourceName(Base.ActiveSource())), FString(TEXT("SkillHover")))) { return false; }

	struct FChange { const TCHAR* Name; bool bRedoesTheRange; TFunction<void(FRTPlanningOverlayInputs&)> Mutate; };
	const FChange Changes[] = {
		{ TEXT("HoveredCell"),               false, [](FRTPlanningOverlayInputs& I) { I.HoveredCell = FRTCellId(0, 0, 0); } },
		{ TEXT("bHoveredValid"),             false, [](FRTPlanningOverlayInputs& I) { I.bHoveredValid = false; } },
		{ TEXT("FacingSector"),              false, [](FRTPlanningOverlayInputs& I) { I.FacingSector = INDEX_NONE; } },
		{ TEXT("ActiveLayer"),               false, [](FRTPlanningOverlayInputs& I) { I.ActiveLayer = 0; } },
		{ TEXT("SkillRange.Origin"),         true,  [](FRTPlanningOverlayInputs& I) { I.SkillRange.Origin = FRTCellId(0, 0, 0); } },
		{ TEXT("SkillRange.RangeCells"),     true,  [](FRTPlanningOverlayInputs& I) { I.SkillRange.RangeCells += 1; } },
		{ TEXT("SkillRange.MinRangeCells"),  true,  [](FRTPlanningOverlayInputs& I) { I.SkillRange.MinRangeCells = 2; } },
		{ TEXT("SkillRange.Policy"),         true,  [](FRTPlanningOverlayInputs& I) { I.SkillRange.Policy = ERTLineOfSightPolicy::NotRequired; } },
		{ TEXT("SkillKnowledgeStamp (la vista cambia)"), true, [](FRTPlanningOverlayInputs& I) { I.SkillKnowledgeStamp += 1; } },
	};
	for (const FChange& Change : Changes)
	{
		FRTPlanningOverlayInputs Changed = Base;
		Change.Mutate(Changed);
		TestFalse(*FString::Printf(TEXT("%s: cambiato da solo rompe l'uguaglianza"), Change.Name), Base == Changed);
		TestEqual(*FString::Printf(TEXT("%s: rifa' la portata solo se e' cio' da cui la portata dipende"), Change.Name),
			!Base.SamePlanLevel(Changed), Change.bRedoesTheRange);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
