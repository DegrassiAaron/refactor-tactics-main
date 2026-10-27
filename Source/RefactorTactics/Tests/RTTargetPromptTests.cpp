// #2826 scope 5 — il prompt contestuale di targeting, e le tre cose che `None` significa.
//
// 🔑 **Il difetto che questo file presidia e' un collasso di significati, non una funzione mancante.**
// `ERTPointerTargetKind::None` e' la risposta di `GetPointerTargetKind()` in tre situazioni che il
// giocatore vive come opposte: non ho selezionato nessuno, non ho armato niente, ho armato un'azione che
// non chiede bersagli. Un prompt scritto come funzione del solo `Kind` — che e' cio' che lo scope della
// issue suggeriva — direbbe la stessa frase in tutti e tre i casi.
//
// ⚠️ **Nessun mondo, nessun Actor**: `BuildTargetPrompt` e' pura, e questi test lo sono di conseguenza.
// L'anti-vacuita' non viene da una fixture ma da `TargetKindForAction`, interrogata qui sotto per provare
// che i valori classificati sono davvero quelli che il core produce.

#include "Misc/AutomationTest.h"
#include "UI/RTHudViewModel.h"
#include "Player/RTPointerInteraction.h"
#include "Ability/RTActionDef.h"
#include "Tests/RTTargetPromptTestHelpers.h" // #3869: tutti i testi, enumerati dagli UENUM

#if WITH_DEV_AUTOMATION_TESTS

/**
 * 🔴 **I TRE SIGNIFICATI DI `None` RESTANO TRE.**
 *
 * E' l'oracolo centrale di `#2826` scope 5: le tre coppie hanno lo **stesso** `ERTPointerTargetKind` e
 * devono produrre tre `ERTTargetPromptKind` **diversi**. Se qualcuno riscrivesse `BuildTargetPrompt` come
 * funzione del solo `Kind`, questo test diventerebbe rosso e nessun altro se ne accorgerebbe.
 *
 * ⛔ **Il confronto e' fra loro, non con tre costanti attese.** Asserire `A == NoSelection` tre volte
 * passerebbe anche se due rami restituissero per sbaglio lo stesso valore in futuro: cio' che va pinnato e'
 * la DISTINZIONE.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTargetPromptSeparatesNoneTest,
	"RefactorTactics.HudViewModel.TargetPromptSeparatesTheThreeMeaningsOfNone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTTargetPromptSeparatesNoneTest::RunTest(const FString&)
{
	const ERTPointerTargetKind Nessuno = ERTPointerTargetKind::None;

	const FRTTargetPromptView SenzaSelezione =
		URTHudViewModel::BuildTargetPrompt(ERTPointerContext::IdleSelection, Nessuno);
	const FRTTargetPromptView Neutro =
		URTHudViewModel::BuildTargetPrompt(ERTPointerContext::Planning, Nessuno);
	const FRTTargetPromptView SuDiSe =
		URTHudViewModel::BuildTargetPrompt(ERTPointerContext::Targeting, Nessuno);

	// A. i tre Kind sono diversi a due a due — la proprieta' che il collasso violerebbe.
	TestNotEqual(TEXT("A1: «nessuna selezione» non e' «nessuna azione armata»"),
		SenzaSelezione.Kind, Neutro.Kind);
	TestNotEqual(TEXT("A2: «nessuna azione armata» non e' «azione su di se'»"),
		Neutro.Kind, SuDiSe.Kind);
	TestNotEqual(TEXT("A3: «nessuna selezione» non e' «azione su di se'»"),
		SenzaSelezione.Kind, SuDiSe.Kind);

	// B. e sono i tre valori dichiarati, cosi' il test dice anche QUALI sono e non solo che differiscono.
	TestEqual(TEXT("B1: senza selezione la domanda non ha soggetto"),
		SenzaSelezione.Kind, ERTTargetPromptKind::NoSelection);
	TestEqual(TEXT("B2: unita' selezionata e niente armato e' il neutro di D-128"),
		Neutro.Kind, ERTTargetPromptKind::Neutral);
	TestEqual(TEXT("B3: un'azione armata che non punta nulla e' confermata, non assente"),
		SuDiSe.Kind, ERTTargetPromptKind::SelfTargetConfirmed);

	// C. tre frasi diverse: un `Kind` distinto con lo stesso testo lascerebbe il giocatore dov'era.
	TestNotEqual(TEXT("C1: le frasi di «nessuna selezione» e «neutro» differiscono"),
		SenzaSelezione.Text.ToString(), Neutro.Text.ToString());
	TestNotEqual(TEXT("C2: le frasi di «neutro» e «azione su di se'» differiscono"),
		Neutro.Text.ToString(), SuDiSe.Text.ToString());

	// D. nessuno dei tre e' muto: qui il prompt e' applicabile, quindi il testo esiste.
	TestFalse(TEXT("D1: «nessuna selezione» ha una frase"), SenzaSelezione.Text.IsEmpty());
	TestFalse(TEXT("D2: il neutro ha una frase"), Neutro.Text.IsEmpty());
	TestFalse(TEXT("D3: «azione su di se'» ha una frase"), SuDiSe.Text.IsEmpty());

	return true;
}

/**
 * 🔴 **OGNI FORMA DI BERSAGLIO CHE IL CORE PRODUCE HA UN PROMPT — e l'elenco non e' scritto a mano.**
 *
 * 🔑 **L'anti-vacuita' e' la prima meta' del test.** Asserire che `Cell` produca «scegli una cella» non
 * dimostra niente se nessuna azione produce piu' `Cell`: il test resterebbe verde su un prompt morto. Qui
 * si interroga prima `URTPointerLibrary::TargetKindForAction` — l'autorita' di [D-128] — per stabilire
 * quali `Kind` esistano davvero, e solo dopo si chiede il prompt.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTargetPromptCoversProducibleKindsTest,
	"RefactorTactics.HudViewModel.TargetPromptNamesEveryProducibleTargetKind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTTargetPromptCoversProducibleKindsTest::RunTest(const FString&)
{
	// --- A. che cosa il core produce davvero ---------------------------------------------------------
	FRTActionDef Base;
	Base.StructureOp = ERTStructureOp::None;

	FRTActionDef Struttura;
	Struttura.StructureOp = ERTStructureOp::CreateCover;

	const ERTPointerTargetKind DaSelf =
		URTPointerLibrary::TargetKindForAction(Base, /*bSelfTarget=*/ true, ERTAbilityShape::Single);
	const ERTPointerTargetKind DaBordo =
		URTPointerLibrary::TargetKindForAction(Struttura, /*bSelfTarget=*/ false, ERTAbilityShape::Single);
	const ERTPointerTargetKind DaArea =
		URTPointerLibrary::TargetKindForAction(Base, /*bSelfTarget=*/ false, ERTAbilityShape::Area);
	const ERTPointerTargetKind DaSingolo =
		URTPointerLibrary::TargetKindForAction(Base, /*bSelfTarget=*/ false, ERTAbilityShape::Single);

	TestEqual(TEXT("A1: un'azione su di se' non chiede bersagli"), DaSelf, ERTPointerTargetKind::None);
	TestEqual(TEXT("A2: un'operazione di struttura chiede un BORDO"), DaBordo, ERTPointerTargetKind::Edge);
	TestEqual(TEXT("A3: un'area si centra su una CELLA"), DaArea, ERTPointerTargetKind::Cell);
	TestEqual(TEXT("A4: il resto chiede un'UNITA'"), DaSingolo, ERTPointerTargetKind::Unit);

	// --- B. ognuno di essi, armato, ha un prompt proprio e non muto -----------------------------------
	struct FAtteso
	{
		ERTPointerTargetKind Kind;
		ERTTargetPromptKind Prompt;
		const TCHAR* Nome;
	};

	const FAtteso Attesi[] = {
		{ DaBordo,   ERTTargetPromptKind::ChooseEdge, TEXT("bordo") },
		{ DaArea,    ERTTargetPromptKind::ChooseCell, TEXT("cella") },
		{ DaSingolo, ERTTargetPromptKind::ChooseUnit, TEXT("unita'") },
	};

	for (const FAtteso& Caso : Attesi)
	{
		const FRTTargetPromptView Prompt =
			URTHudViewModel::BuildTargetPrompt(ERTPointerContext::Targeting, Caso.Kind);

		TestEqual(*FString::Printf(TEXT("B: %s -> il prompt dichiarato"), Caso.Nome),
			Prompt.Kind, Caso.Prompt);
		TestFalse(*FString::Printf(TEXT("B: %s -> la frase non e' vuota"), Caso.Nome),
			Prompt.Text.IsEmpty());
		TestTrue(*FString::Printf(TEXT("B: %s -> il puntatore ASPETTA un bersaglio"), Caso.Nome),
			Prompt.bIsAwaitingTarget);
	}

	// --- C. le tre frasi sono distinte fra loro ------------------------------------------------------
	// Tre `Kind` diversi con la stessa frase non direbbero al giocatore che cosa cliccare.
	const FString Bordo = URTHudViewModel::BuildTargetPrompt(ERTPointerContext::Targeting, DaBordo).Text.ToString();
	const FString Cella = URTHudViewModel::BuildTargetPrompt(ERTPointerContext::Targeting, DaArea).Text.ToString();
	const FString Unita = URTHudViewModel::BuildTargetPrompt(ERTPointerContext::Targeting, DaSingolo).Text.ToString();

	TestNotEqual(TEXT("C1: bordo e cella si dicono diversamente"), Bordo, Cella);
	TestNotEqual(TEXT("C2: cella e unita' si dicono diversamente"), Cella, Unita);
	TestNotEqual(TEXT("C3: bordo e unita' si dicono diversamente"), Bordo, Unita);

	// --- D. l'orientamento e' il quinto prompt, e NON passa da un `TargetKind` ------------------------
	// ⚠️ Il `Kind` passato e' `None` di proposito: in `Facing` non c'e' nessuna azione che dichiari una
	// forma di bersaglio, e il prompt deve arrivarci comunque.
	const FRTTargetPromptView Orientamento =
		URTHudViewModel::BuildTargetPrompt(ERTPointerContext::Facing, ERTPointerTargetKind::None);
	TestEqual(TEXT("D1: il contesto Facing chiede l'orientamento"),
		Orientamento.Kind, ERTTargetPromptKind::ChooseFacing);
	TestTrue(TEXT("D2: e aspetta una risposta dal giocatore"), Orientamento.bIsAwaitingTarget);
	TestFalse(TEXT("D3: con una frase propria"), Orientamento.Text.IsEmpty());

	return true;
}

/**
 * 🔴 **A MONDO IN SOLA LETTURA IL PROMPT TACE — anche con un'azione ancora armata.**
 *
 * 🔑 **Il secondo argomento e' la trappola, ed e' per questo che il test lo passa acceso.** Uscendo dal
 * Planning l'abilita' armata non viene azzerata: se `BuildTargetPrompt` guardasse il `Kind` prima del
 * contesto, a partita in pausa comparirebbe «scegli un'unita'» sopra il menu. La precedenza
 * `Modal/Reaction > tutto` e' gia' quella di `GetPointerContext()`, e qui va rispettata invece che
 * reinventata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTargetPromptIsSilentWhenReadOnlyTest,
	"RefactorTactics.HudViewModel.TargetPromptIsNotApplicableWhileTheWorldIsReadOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTTargetPromptIsSilentWhenReadOnlyTest::RunTest(const FString&)
{
	const ERTPointerContext SolaLettura[] = {
		ERTPointerContext::Modal,
		ERTPointerContext::ResolutionPlayback,
		ERTPointerContext::ReactionWindow,
	};

	for (const ERTPointerContext Contesto : SolaLettura)
	{
		// ⚠️ `Unit` e non `None`: e' lo stato che il difetto produrrebbe.
		const FRTTargetPromptView Prompt =
			URTHudViewModel::BuildTargetPrompt(Contesto, ERTPointerTargetKind::Unit);

		TestEqual(TEXT("A: il prompt e' fuori fase, non vuoto per caso"),
			Prompt.Kind, ERTTargetPromptKind::NotApplicable);
		TestTrue(TEXT("B: e non porta nessuna frase da mostrare"), Prompt.Text.IsEmpty());
		TestFalse(TEXT("C: e non aspetta niente dal giocatore"), Prompt.bIsAwaitingTarget);
	}

	// D. e «fuori fase» non e' «nessuna selezione»: sono due assenze diverse, e il widget le disegna
	//    diversamente — una nasconde il prompt, l'altra invita a selezionare.
	const FRTTargetPromptView FuoriFase =
		URTHudViewModel::BuildTargetPrompt(ERTPointerContext::Modal, ERTPointerTargetKind::None);
	const FRTTargetPromptView SenzaSelezione =
		URTHudViewModel::BuildTargetPrompt(ERTPointerContext::IdleSelection, ERTPointerTargetKind::None);
	TestNotEqual(TEXT("D: «fuori fase» e «nessuna selezione» restano distinti"),
		FuoriFase.Kind, SenzaSelezione.Kind);

	return true;
}

/**
 * 🔴 **VISIBILE NON E' IN ATTESA**, ed e' la distinzione che `#2757` chiede di non comprimere.
 *
 * Due prompt si mostrano e non aspettano nessun click: il neutro di [D-128] e l'azione su di se'. Scrivere
 * `bIsAwaitingTarget = (Kind != NotApplicable)` supererebbe ogni altro test di questo file.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTargetPromptVisibleIsNotAwaitingTest,
	"RefactorTactics.HudViewModel.TargetPromptDistinguishesVisibleFromAwaiting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTTargetPromptVisibleIsNotAwaitingTest::RunTest(const FString&)
{
	const FRTTargetPromptView Neutro =
		URTHudViewModel::BuildTargetPrompt(ERTPointerContext::Planning, ERTPointerTargetKind::None);
	const FRTTargetPromptView SuDiSe =
		URTHudViewModel::BuildTargetPrompt(ERTPointerContext::Targeting, ERTPointerTargetKind::None);
	const FRTTargetPromptView SenzaSelezione =
		URTHudViewModel::BuildTargetPrompt(ERTPointerContext::IdleSelection, ERTPointerTargetKind::None);

	// A. si mostrano — hanno una frase — e non aspettano niente.
	TestFalse(TEXT("A1: il neutro si mostra"), Neutro.Text.IsEmpty());
	TestFalse(TEXT("A2: il neutro NON aspetta un bersaglio"), Neutro.bIsAwaitingTarget);

	TestFalse(TEXT("A3: «azione su di se'» si mostra"), SuDiSe.Text.IsEmpty());
	TestFalse(TEXT("A4: «azione su di se'» NON aspetta un bersaglio: e' gia' confermata"),
		SuDiSe.bIsAwaitingTarget);

	TestFalse(TEXT("A5: «nessuna selezione» si mostra"), SenzaSelezione.Text.IsEmpty());
	TestFalse(TEXT("A6: «nessuna selezione» NON aspetta un bersaglio"), SenzaSelezione.bIsAwaitingTarget);

	// B. e almeno uno invece aspetta: senza questo, un `bIsAwaitingTarget` sempre falso passerebbe.
	const FRTTargetPromptView Puntamento =
		URTHudViewModel::BuildTargetPrompt(ERTPointerContext::Targeting, ERTPointerTargetKind::Unit);
	TestTrue(TEXT("B: un targeting vero aspetta un bersaglio"), Puntamento.bIsAwaitingTarget);

	return true;
}

/**
 * 🔴 **LA FORMA CHE NESSUNO PRODUCE HA UN NOME, e non una frase inventata ne' il silenzio.**
 *
 * `ERTPointerTargetKind::Object` sta nell'enum del puntatore e `TargetKindForAction` non lo restituisce
 * mai. Le due uscite sbagliate sono simmetriche: promettere «scegli un oggetto» impegna un percorso che
 * non esiste, restituire testo vuoto lo fa sparire come `NotApplicable`. Il valore `Unsupported` e' il
 * segnaposto dichiarato, e questo test e' cio' che lo rende un gap tracciato invece che una svista.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTargetPromptDeclaresUnproducibleKindTest,
	"RefactorTactics.HudViewModel.TargetPromptDeclaresTheUnproducibleTargetKind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTTargetPromptDeclaresUnproducibleKindTest::RunTest(const FString&)
{
	// A. la premessa: nessuna delle quattro combinazioni del catalogo produce `Object`. Se un giorno una lo
	//    producesse, questo blocco diventa rosso ed e' il promemoria di scrivere il prompt vero.
	FRTActionDef Base;
	FRTActionDef Struttura;
	Struttura.StructureOp = ERTStructureOp::CreateCover;

	const ERTPointerTargetKind Prodotti[] = {
		URTPointerLibrary::TargetKindForAction(Base, true,  ERTAbilityShape::Single),
		URTPointerLibrary::TargetKindForAction(Struttura, false, ERTAbilityShape::Single),
		URTPointerLibrary::TargetKindForAction(Base, false, ERTAbilityShape::Area),
		URTPointerLibrary::TargetKindForAction(Base, false, ERTAbilityShape::Single),
	};

	for (const ERTPointerTargetKind Kind : Prodotti)
	{
		TestNotEqual(TEXT("A: il catalogo non produce ancora `Object`"), Kind, ERTPointerTargetKind::Object);
	}

	// B. e il prompt lo dichiara invece di indovinarlo o di tacere.
	const FRTTargetPromptView Oggetto =
		URTHudViewModel::BuildTargetPrompt(ERTPointerContext::Targeting, ERTPointerTargetKind::Object);

	TestEqual(TEXT("B1: la forma senza produttore ha un nome proprio"),
		Oggetto.Kind, ERTTargetPromptKind::Unsupported);
	TestFalse(TEXT("B2: e non sparisce in un testo vuoto"), Oggetto.Text.IsEmpty());
	TestNotEqual(TEXT("B3: e non si confonde con «fuori fase»"),
		Oggetto.Kind, ERTTargetPromptKind::NotApplicable);

	return true;
}

/**
 * 🔑 **I PROMPT CHE NOMINANO UN'UNITA' LA SCRIVONO CON L'ACCENTO** (#3869).
 *
 * L'apostrofo ASCII al posto dell'accento (`unita'`) e' un'abitudine di questo repository, e in altre stringhe visibili arriva
 * anche al giocatore (l'elenco e' nella PR di #3869): questo controllo copre il prompt di targeting. I prompt che nominano
 * un'unita' (nessuna selezione, e la scelta di un'unita' con un'azione armata) la stampavano così. Il controllo positivo e'
 * che i testi esistano e siano diversi fra loro: un test che cercasse l'assenza di `unita'` passerebbe anche su testi vuoti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTargetPromptWritesUnitWithItsAccentTest,
	"RefactorTactics.HudViewModel.TargetPromptWritesUnitWithItsAccent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTTargetPromptWritesUnitWithItsAccentTest::RunTest(const FString&)
{
	const FString SenzaSelezione =
		URTHudViewModel::BuildTargetPrompt(ERTPointerContext::IdleSelection, ERTPointerTargetKind::None).Text.ToString();
	const FString ScegliUnita =
		URTHudViewModel::BuildTargetPrompt(ERTPointerContext::Targeting, ERTPointerTargetKind::Unit).Text.ToString();

	TestFalse(TEXT("premessa: il prompt senza selezione esiste"), SenzaSelezione.IsEmpty());
	TestFalse(TEXT("premessa: il prompt che chiede un'unita' esiste"), ScegliUnita.IsEmpty());
	TestNotEqual(TEXT("premessa: i due prompt sono diversi fra loro"), SenzaSelezione, ScegliUnita);

	for (const FString* Testo : { &SenzaSelezione, &ScegliUnita })
	{
		TestTrue(*FString::Printf(TEXT("'%s' scrive unit\u00E0 con l'accento"), **Testo),
			Testo->Contains(TEXT("unit\u00E0"), ESearchCase::CaseSensitive));
		TestFalse(*FString::Printf(TEXT("'%s' non usa l'apostrofo al posto dell'accento"), **Testo),
			Testo->Contains(TEXT("unita'")));
	}
	return true;
}

/**
 * 🔑 **NESSUN TESTO DEL PROMPT USA UN APOSTROFO AL POSTO DI UN ACCENTO** (#3869, rilievo della review).
 *
 * Il test sull'accento di «unità» protegge due stringhe, e lo stesso difetto stava in una terza, nella stessa funzione e nello stesso
 * widget («sé»). Qui il controllo e' sulla CLASSE: si leggono tutti i testi che `BuildTargetPrompt` puo' produrre (enumerati dagli `UENUM`,
 * vedi `RTTargetPromptTestHelpers.h`) e si cerca un apostrofo che non sia un'elisione. Un'elisione (`un'azione`, `l'orientamento`) ha una lettera
 * subito dopo l'apostrofo; un accento scritto come apostrofo (`unita'`, `se':`) no.
 *
 * ⚠️ **Che cosa il predicato vede, e che cosa no.** Vede l'apostrofo ASCII (`'`) e quello tipografico (U+2019) non seguiti da una lettera. Lascia
 * passare il troncamento `po'` (`un po' di`), che e' italiano corretto. Non vede un accento scritto in un altro modo (un accento grave isolato, per esempio):
 * e' un controllo sul difetto noto, non un correttore ortografico.
 *
 * Il controllo positivo e' triplo: il predicato riconosce i testi sbagliati scritti a mano (in entrambe le forme), lascia passare un'elisione e un troncamento
 * veri, e fra i testi che il prompt produce c'e' un'elisione vera. Senza i primi sarebbe vero per assenza, senza gli altri rifiuterebbe tutto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTargetPromptNeverUsesAnApostropheForAnAccentTest,
	"RefactorTactics.HudViewModel.TargetPromptNeverUsesAnApostropheForAnAccent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTTargetPromptNeverUsesAnApostropheForAnAccentTest::RunTest(const FString&)
{
	// Una lettera: ASCII, o fuori dall'ASCII (`FChar::IsAlpha` e' `iswalpha`, che per le lettere accentate puo' rispondere no).
	const auto Lettera = [](const TCHAR C) { return C > 127 || FChar::IsAlpha(C); };
	const auto Apostrofo = [](const TCHAR C) { return C == TEXT('\'') || C == TEXT('\u2019'); };
	const auto ApostrofoComeAccento = [&](const FString& Testo)
	{
		for (int32 i = 0; i < Testo.Len(); ++i)
		{
			if (!Apostrofo(Testo[i]) || (i + 1 < Testo.Len() && Lettera(Testo[i + 1])))
			{
				continue; // non e' un apostrofo, oppure e' un'elisione
			}
			const bool bTroncamento = i >= 2 && FChar::ToLower(Testo[i - 1]) == TEXT('o') && FChar::ToLower(Testo[i - 2]) == TEXT('p')
				&& (i < 3 || !Lettera(Testo[i - 3]));
			if (!bTroncamento)
			{
				return true;
			}
		}
		return false;
	};

	// Il predicato riconosce il difetto in entrambe le forme, e non rifiuta l'italiano corretto.
	TestTrue(TEXT("premessa: riconosce un accento scritto come apostrofo ASCII"), ApostrofoComeAccento(TEXT("Seleziona un'unita'")));
	TestTrue(TEXT("premessa: ... e come apostrofo tipografico"), ApostrofoComeAccento(TEXT("Seleziona un'unita\u2019")));
	TestTrue(TEXT("premessa: ... anche prima di un segno"), ApostrofoComeAccento(TEXT("Azione su di se': confermata")));
	TestFalse(TEXT("premessa: lascia passare un'elisione"), ApostrofoComeAccento(TEXT("Scegli un'azione")));
	TestFalse(TEXT("premessa: lascia passare un troncamento"), ApostrofoComeAccento(TEXT("Aspetta un po' di tempo")));
	TestFalse(TEXT("premessa: ... anche in maiuscola a inizio frase"), ApostrofoComeAccento(TEXT("Po' di tempo")));
	TestFalse(TEXT("premessa: e un'elisione prima di una lettera accentata"), ApostrofoComeAccento(TEXT("Scegli un'\u00e0ncora")));
	TestTrue(TEXT("premessa: ma non una parola che finisce in `o'` senza essere `po'`"), ApostrofoComeAccento(TEXT("Perche' no")) && ApostrofoComeAccento(TEXT("Scelto'")));

	const TArray<FText> Testi = RTTargetPromptTest::AllTexts();
	TestTrue(TEXT("premessa: il prompt ha dei testi da leggere"), Testi.Num() > 0);
	bool bVistaUnElisione = false;
	for (const FText& Testo : Testi)
	{
		const FString Lettura = Testo.ToString();
		bool bConApostrofo = false;
		for (const TCHAR C : Lettura)
		{
			bConApostrofo |= Apostrofo(C);
		}
		bVistaUnElisione |= bConApostrofo && !ApostrofoComeAccento(Lettura);
		TestFalse(*FString::Printf(TEXT("'%s' non usa un apostrofo al posto di un accento"), *Lettura), ApostrofoComeAccento(Lettura));
	}
	TestTrue(TEXT("premessa: fra i testi letti c'e' un'elisione vera, e il predicato l'ha lasciata passare"), bVistaUnElisione);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
