#include "Misc/AutomationTest.h"
#include "Turn/RTPresentationBinding.h"
#include "Turn/RTResolvedEvent.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/**
	 * Una voce con cue, per costruire tabelle di prova **a mano**.
	 *
	 * 🔴 **A mano, e non derivandola da `DeclaredBindings()`.** E' la lezione che il gate delle icone ha
	 * pagato con una code review: la' un'assertion diceva «un catalogo che copre l'insieme richiesto non ha
	 * mancanze», ma il catalogo era costruito da `RequiredIconIds()` e confrontato con un gate che itera
	 * `RequiredIconIds()` — vera per costruzione, qualunque cosa facesse la derivazione. Qui la simmetria
	 * sarebbe ancora piu' naturale (tabella derivata dall'enum, gate che itera l'enum), quindi le tabelle di
	 * prova di questo file non chiamano mai la tabella vera. Nome distinto per file (unity build).
	 */
	FRTPresentationBinding PresBindingWithCue(ERTResolvedEventType Type)
	{
		return FRTPresentationBinding(Type, { FName(TEXT("CueDiProva")) });
	}

	/** Tutti i tipi dell'enum, letti dalla reflection: serve a costruire tabelle complete o bucate. */
	TArray<ERTResolvedEventType> PresAllEventTypes()
	{
		TArray<ERTResolvedEventType> Out;
		if (const UEnum* Enum = StaticEnum<ERTResolvedEventType>())
		{
			for (int32 I = 0; I < Enum->NumEnums() - 1; ++I)
			{
				Out.Add(static_cast<ERTResolvedEventType>(Enum->GetValueByIndex(I)));
			}
		}
		return Out;
	}
}

// ---------------------------------------------------------------------------------------------------------
// Il conteggio dei tipi e' PINNATO: aggiungerne uno rompe questa riga, non un ciclo
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPresentationEnumSizeIsPinnedTest,
	"RefactorTactics.Presentation.EnumSizeIsPinned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPresentationEnumSizeIsPinnedTest::RunTest(const FString&)
{
	// 🔑 Questo test non protegge un numero: protegge un MOMENTO. Chi aggiunge un valore a
	// `ERTResolvedEventType` vede fallire una riga che lo nomina, invece di scoprire mesi dopo che l'evento
	// non si vedeva. Il gate vero copre il valore nuovo per costruzione; questa riga fa in modo che
	// qualcuno se ne ACCORGA e vada a dichiararne la presentazione.
	// ➕ **Era 4, ed e' diventato 5 il 2026-08-31 con `AttackFootprint` ([D-301], #1945).** Il numero non e'
	// ➕ **Ed e' diventato 6 il 2026-09-04 con `ReactionResolved` (#2191)**: stessa storia, e la riga ha
	// funzionato di nuovo — e' fallita, e ha mandato a dichiarare la presentazione del valore nuovo.
	// il punto: il punto e' che quella issue ha visto fallire QUESTA riga, ed e' andata a dichiarare la
	// presentazione del valore nuovo invece di scoprire fra sei mesi che l'evento non si vedeva.
	TestEqual(TEXT("ERTResolvedEventType dichiara cinque valori (Move, Attack, HazardDamage, Defeated, AttackFootprint)"),
		URTPresentationBindingLibrary::DeclaredEventTypeCount(), 6);

	// La reflection c'e' davvero: senza, `DeclaredEventTypeCount()` restituirebbe 0 e l'assertion sopra
	// fallirebbe per il motivo sbagliato.
	TestNotNull(TEXT("StaticEnum<ERTResolvedEventType>() risolve"), StaticEnum<ERTResolvedEventType>());

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// La tabella vera copre ogni tipo
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPresentationEveryEventTypeIsCoveredTest,
	"RefactorTactics.Presentation.EveryEventTypeIsCovered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPresentationEveryEventTypeIsCoveredTest::RunTest(const FString&)
{
	const TArray<FString> Missing =
		URTPresentationBindingLibrary::FindMissingBindings(URTPresentationBindingLibrary::DeclaredBindings());

	// Il messaggio porta le mancanze: un gate che dice solo «fallito» costringe a rileggere la tabella.
	TestEqual(*FString::Printf(TEXT("nessun tipo scoperto (mancanti: %s)"),
		Missing.Num() > 0 ? *FString::Join(Missing, TEXT(" | ")) : TEXT("-")), Missing.Num(), 0);

	// La tabella dichiara esattamente un tipo per voce: se qualcuno ne aggiungesse una senza togliere la
	// vecchia, `FindMissingBindings` lo direbbe come ambiguita', ma questa riga lo dice prima e meglio.
	TestEqual(TEXT("una voce per tipo"),
		URTPresentationBindingLibrary::DeclaredBindings().Num(),
		URTPresentationBindingLibrary::DeclaredEventTypeCount());

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// 🔴 Il test che il gate NON puo' passare per omissione: una voce tolta produce ESATTAMENTE quella mancanza
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPresentationMissingEntryIsReportedTest,
	"RefactorTactics.Presentation.MissingEntryIsReported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPresentationMissingEntryIsReportedTest::RunTest(const FString&)
{
	// Tabella costruita a mano, completa TRANNE `Attack`. Non deriva dalla tabella vera: se derivasse, il
	// test resterebbe verde anche con un gate che non guarda niente.
	TArray<FRTPresentationBinding> Bucata;
	for (const ERTResolvedEventType Type : PresAllEventTypes())
	{
		if (Type != ERTResolvedEventType::Attack)
		{
			Bucata.Add(PresBindingWithCue(Type));
		}
	}

	const TArray<FString> Missing = URTPresentationBindingLibrary::FindMissingBindings(Bucata);

	// «Esattamente una», non «almeno una»: cade in due modi opposti — se il gate smette di vedere la
	// mancanza diventa 0, se comincia a vederne dove non ci sono diventa piu' di 1.
	TestEqual(TEXT("togliere Attack produce esattamente una mancanza"), Missing.Num(), 1);
	if (Missing.Num() == 1)
	{
		TestTrue(*FString::Printf(TEXT("la mancanza nomina Attack (e' invece: %s)"), *Missing[0]),
			Missing[0].StartsWith(TEXT("Attack")));
	}
	else
	{
		// ⚠️ Senza questo ramo il test uscirebbe MUTO quando il conteggio e' diverso da 1, e un'uscita
		// anticipata senza assertion viene riportata come Success.
		AddError(FString::Printf(TEXT("mancanze inattese: %s"), *FString::Join(Missing, TEXT(" | "))));
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Un binding ASSENTE non e' «zero mancanze»: e' la mancanza totale
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPresentationEmptyBindingReportsEveryTypeTest,
	"RefactorTactics.Presentation.EmptyBindingReportsEveryType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPresentationEmptyBindingReportsEveryTypeTest::RunTest(const FString&)
{
	// 🔴 E' il caso peggiore, ed e' quello in cui un gate scritto con leggerezza tace: nessuna voce da
	// controllare, nessun errore da riportare. Qui la risposta e' l'opposto.
	const TArray<FString> Missing =
		URTPresentationBindingLibrary::FindMissingBindings(TArray<FRTPresentationBinding>());

	TestEqual(TEXT("una tabella vuota lascia scoperti TUTTI i tipi"),
		Missing.Num(), URTPresentationBindingLibrary::DeclaredEventTypeCount());

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Le tre forme di dichiarazione che NON coprono
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPresentationHollowDeclarationsDoNotCoverTest,
	"RefactorTactics.Presentation.HollowDeclarationsDoNotCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPresentationHollowDeclarationsDoNotCoverTest::RunTest(const FString&)
{
	const TArray<ERTResolvedEventType> AllTypes = PresAllEventTypes();

	// Costruisce una tabella completa in cui SOLO `Move` e' dichiarato nel modo sbagliato, cosi' l'unica
	// mancanza attesa e' quella: se ne comparissero altre, il difetto sarebbe nel gate e non nel caso.
	auto TabellaCon = [&AllTypes](const FRTPresentationBinding& Storta)
	{
		TArray<FRTPresentationBinding> T;
		for (const ERTResolvedEventType Type : AllTypes)
		{
			T.Add(Type == ERTResolvedEventType::Move ? Storta : PresBindingWithCue(Type));
		}
		return T;
	};

	// (a) Voce `Cues` senza alcuna cue: dichiarata, e non copre nulla.
	{
		FRTPresentationBinding Vuota;
		Vuota.Type = ERTResolvedEventType::Move;
		Vuota.Kind = ERTPresentationKind::Cues;
		const TArray<FString> Missing = URTPresentationBindingLibrary::FindMissingBindings(TabellaCon(Vuota));
		TestEqual(TEXT("(a) una voce con cue vuote non copre"), Missing.Num(), 1);
	}

	// (b) Cue dichiarate ma tutte `NAME_None`: e' il widget dichiarato e mai disegnato.
	{
		FRTPresentationBinding SoloNone(ERTResolvedEventType::Move, { NAME_None, NAME_None });
		const TArray<FString> Missing = URTPresentationBindingLibrary::FindMissingBindings(TabellaCon(SoloNone));
		TestEqual(TEXT("(b) cue tutte NAME_None non coprono"), Missing.Num(), 1);
	}

	// (c) `NoPresentation` senza motivo: e' il default implicito che `D-278` vieta, scritto a mano.
	{
		const FRTPresentationBinding Muta =
			FRTPresentationBinding::MakeNoPresentation(ERTResolvedEventType::Move, TEXT("   "));
		const TArray<FString> Missing = URTPresentationBindingLibrary::FindMissingBindings(TabellaCon(Muta));
		TestEqual(TEXT("(c) NoPresentation senza motivo non copre"), Missing.Num(), 1);
	}

	// (d) Controprova: `NoPresentation` CON motivo copre. Senza questa riga le tre sopra proverebbero solo
	//     che il gate si lamenta, non che sa distinguere.
	{
		const FRTPresentationBinding Dichiarata = FRTPresentationBinding::MakeNoPresentation(
			ERTResolvedEventType::Move, TEXT("motivo scritto"));
		const TArray<FString> Missing =
			URTPresentationBindingLibrary::FindMissingBindings(TabellaCon(Dichiarata));
		TestEqual(TEXT("(d) NoPresentation con motivo copre"), Missing.Num(), 0);
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Due voci per lo stesso tipo: un'ambiguita' non e' una dichiarazione
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPresentationDuplicateDeclarationIsAmbiguousTest,
	"RefactorTactics.Presentation.DuplicateDeclarationIsAmbiguous",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPresentationDuplicateDeclarationIsAmbiguousTest::RunTest(const FString&)
{
	// Tabella completa PIU' un doppione di `Defeated`. Entrambe le voci sono valide prese da sole: e'
	// proprio il caso in cui un gate che si ferma alla prima corrispondenza direbbe «coperto».
	TArray<FRTPresentationBinding> ConDoppione;
	for (const ERTResolvedEventType Type : PresAllEventTypes())
	{
		ConDoppione.Add(PresBindingWithCue(Type));
	}
	ConDoppione.Add(FRTPresentationBinding(ERTResolvedEventType::Defeated,
		{ FName(TEXT("UnAltraCue")) }));

	const TArray<FString> Missing = URTPresentationBindingLibrary::FindMissingBindings(ConDoppione);

	TestEqual(TEXT("un tipo dichiarato due volte e' segnalato"), Missing.Num(), 1);
	if (Missing.Num() == 1)
	{
		TestTrue(*FString::Printf(TEXT("la segnalazione nomina Defeated (e' invece: %s)"), *Missing[0]),
			Missing[0].StartsWith(TEXT("Defeated")));
	}
	else
	{
		AddError(FString::Printf(TEXT("mancanze inattese: %s"), *FString::Join(Missing, TEXT(" | "))));
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Determinismo: stesso ingresso, stessa uscita nello stesso ordine
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPresentationResultIsDeterministicTest,
	"RefactorTactics.Presentation.ResultIsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPresentationResultIsDeterministicTest::RunTest(const FString&)
{
	// Una tabella vuota lascia scoperti tutti i tipi: e' l'ingresso che produce l'uscita piu' lunga, quindi
	// quello su cui un ordine instabile si vedrebbe.
	const TArray<FString> Primo =
		URTPresentationBindingLibrary::FindMissingBindings(TArray<FRTPresentationBinding>());
	const TArray<FString> Secondo =
		URTPresentationBindingLibrary::FindMissingBindings(TArray<FRTPresentationBinding>());

	TestEqual(TEXT("stesso numero di mancanze"), Secondo.Num(), Primo.Num());
	TestEqual(TEXT("stesso ordine, riga per riga"),
		FString::Join(Secondo, TEXT("|")), FString::Join(Primo, TEXT("|")));

	// 🔑 E l'ordine e' quello dell'ENUM, non quello di arrivo: l'uscita non dipende da come e' scritta la
	// tabella in ingresso. Si verifica dando le voci in ordine inverso e osservando che le mancanze
	// prodotte dai tipi restanti conservino l'ordine dei valori.
	TArray<FRTPresentationBinding> SoloUltimo;
	SoloUltimo.Add(PresBindingWithCue(ERTResolvedEventType::Defeated));
	const TArray<FString> Mancanti = URTPresentationBindingLibrary::FindMissingBindings(SoloUltimo);
	TestEqual(TEXT("gli altri tipi restano scoperti"),
		Mancanti.Num(), URTPresentationBindingLibrary::DeclaredEventTypeCount() - 1);
	if (Mancanti.Num() >= 3)
	{
		// L'ordine e' quello dei VALORI, e `Defeated` - l'unico dichiarato - non compare: le tre mancanze
		// che lo precedono restano nella loro sequenza anche se l'enum cresce in coda.
		TestTrue(TEXT("l'ordine segue l'enum: Move, Attack, HazardDamage"),
			Mancanti[0].StartsWith(TEXT("Move"))
			&& Mancanti[1].StartsWith(TEXT("Attack"))
			&& Mancanti[2].StartsWith(TEXT("HazardDamage")));
		TestFalse(TEXT("il tipo dichiarato non e' fra le mancanze"),
			Mancanti.ContainsByPredicate([](const FString& M) { return M.StartsWith(TEXT("Defeated")); }));
	}
	else
	{
		AddError(FString::Printf(TEXT("mancanze inattese: %s"), *FString::Join(Mancanti, TEXT(" | "))));
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// La clausola di `HazardDamage` e' parte della dichiarazione, non un commento
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPresentationHazardDamageIsDeclaredSilentTest,
	"RefactorTactics.Presentation.HazardDamageIsDeclaredSilent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPresentationHazardDamageIsDeclaredSilentTest::RunTest(const FString&)
{
	const TArray<FRTPresentationBinding> Reale = URTPresentationBindingLibrary::DeclaredBindings();

	const FRTPresentationBinding* Hazard = Reale.FindByPredicate(
		[](const FRTPresentationBinding& B) { return B.Type == ERTResolvedEventType::HazardDamage; });

	if (!Hazard)
	{
		// ⚠️ `AddError` e non un `return` muto: uscire senza asserire verrebbe riportato come Success.
		AddError(TEXT("HazardDamage non e' dichiarato nella tabella"));
		return true;
	}

	TestTrue(TEXT("HazardDamage e' NoPresentation, non una voce dimenticata"),
		Hazard->Kind == ERTPresentationKind::NoPresentation);

	// 🔴 La clausola: oggi il valore non ha un produttore, e la voce deve DIRLO. Senza questa riga la
	// dichiarazione varrebbe «perche' non accade» invece che «quando accadra'», e resterebbe verde il
	// giorno in cui qualcuno emette l'evento — cioe' il difetto che questa tabella esiste per impedire.
	TestTrue(TEXT("il motivo avverte che oggi il valore non ha un produttore"),
		Hazard->Rationale.Contains(TEXT("produttore")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
