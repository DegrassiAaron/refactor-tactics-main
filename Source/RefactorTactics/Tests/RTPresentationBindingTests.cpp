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
	//
	// La storia del numero, che e' il curriculum della riga:
	//   4 -> 5   2026-08-31   `AttackFootprint`   ([D-301], #1945)
	//   5 -> 6   2026-09-04   `ReactionResolved`  (#2191)
	//   6 -> 7   2026-09-04   `StatusChanged`     (#2245)
	// Tre volte su tre e' fallita per prima e ha mandato a dichiarare la presentazione del valore nuovo.
	//
	// ⚠️ **Il messaggio elencava i valori per nome e si e' scollato dal numero**: diceva *«dichiara cinque
	// valori (Move, Attack, HazardDamage, Defeated, AttackFootprint)»* mentre ne attendeva **6**, perche'
	// `#2191` aggiorno' la cifra e non la frase. Un messaggio d'errore che mente su cio' che misura manda a
	// cercare il difetto nel posto sbagliato — quindi l'elenco non si ripete piu' qui: lo porta l'enum.
	TestEqual(TEXT("ERTResolvedEventType dichiara sette valori: l'ultimo aggiunto ha una voce nella tabella?"),
		URTPresentationBindingLibrary::DeclaredEventTypeCount(), 7);

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

	// 🔁 **Dal 2026-09-05 e' `PendingPresentation`, per decisione d'autore.** `#2460` l'aveva lasciata
	// `NoPresentation` dichiarando la scelta deliberata, ma il suo stesso motivo diceva *«la cue e' lavoro
	// di #2455»*: sotto la tassonomia di `#2483` le due clausole si contraddicono, e la contraddizione e'
	// stata portata all'autore invece che risolta a intuito.
	//
	// ⛔ **Il nome del test resta vero**: `PendingPresentation` dichiara comunque che l'evento NON si
	// mostra. Cio' che aggiunge e' chi lo mostrera'.
	//
	// ⚠️ **Il test PRECEDENTE sarebbe rimasto verde sulla contraddizione**, perche' guardava
	// `Kind == NoPresentation` e due sottostringhe che la voce nuova conserva. E' lo stesso difetto che
	// questo file ha gia' documentato per il predicato `Rationale.Contains("produttore")`: un guardiano che
	// passa su una stringa che afferma altro non e' un guardiano.
	TestTrue(TEXT("HazardDamage non si mostra, ed e' un'attesa dichiarata"),
		Hazard->Kind == ERTPresentationKind::PendingPresentation);

	// 🔑 **L'owner e' `#2505` e non piu' `#2455`, e il cambio e' il meccanismo che funziona.** La decisione
	// d'autore aveva nominato `#2455` perche' era la issue che stava per disegnare il feedback di
	// combattimento; misurando, quella issue ha scoperto che questo evento **non ha un istante** in cui una
	// cue possa essere giocata, ha consegnato il token per `Attack` — che un beat ce l'ha — e ha aperto
	// `#2505` per costruire quello che manca. Il promemoria non e' vissuto in una frase: e' stato questo
	// censimento a convocare la revisione.
	TestEqual(TEXT("e la cue la deve #2505, non una issue che ha gia' chiuso"),
		Hazard->PendingOwner, FString(TEXT("#2505")));

	// Il motivo dichiara che il produttore ESISTE: il predicato di `#2460` resta valido e va conservato,
	// perche' e' cio' che impedisce di tornare a credere che l'evento sia muto.
	TestTrue(TEXT("il motivo dichiara che il produttore esiste, non che manca"),
		Hazard->Rationale.Contains(TEXT("produttore ESISTE")));

	// ⚠️ **Non si cerca l'owner nel MOTIVO**: e' un campo, e cercarlo in una stringa libera tornerebbe a
	// misurare la prosa — il difetto che questo file documenta due volte. L'assertion sull'owner sta sopra,
	// sul campo. Qui resta cio' che solo il motivo puo' dire: **perche'** oggi non si disegna.
	//
	// 🔴 **E cio' che manca non e' il dato ma l'ISTANTE**, che e' l'informazione nuova e la sola che spiega
	// perche' la cue non e' nata insieme a quella di `Attack`: `BeginPlayback` non consuma questo tipo, e
	// `PlaybackPhases` non contiene mai `Cleanup`.
	TestTrue(TEXT("il motivo dichiara che manca l'ISTANTE, non il dato"),
		Hazard->Rationale.Contains(TEXT("ISTANTE")));

	// ⚠️ **E che la morte da hazard non ha un beat proprio.** Misurato in `#2460`: `Defeated` lo emette solo
	// `ResolveCombatPasses`, sul Blast. La voce lo dichiara perche' chi costruira' la cue non dia per
	// scontato un momento di morte che non esiste — era precisamente l'errore della stesura precedente.
	TestTrue(TEXT("il motivo avverte che la morte da hazard non emette Defeated"),
		Hazard->Rationale.Contains(TEXT("non emette Defeated")));

	return true;
}

// ---------------------------------------------------------------------------------------------------------

/**
 * `PendingPresentation` ben formata COPRE; senza owner o senza motivo NON copre (`#2483`).
 *
 * 🔑 **La controprova non e' decorativa.** Senza il caso (c) i due sopra proverebbero solo che il gate si
 * lamenta, non che sa **distinguere** una voce in attesa legittima da una malformata — ed e' esattamente la
 * forma che il caso (d) di `HollowDeclarationsDoNotCover` usa gia' per `NoPresentation`.
 *
 * ⛔ Se una voce in attesa ben formata NON coprisse, la pressione sarebbe a cancellare la distinzione per
 * tornare verdi, e il gate tornerebbe a misurare niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPresentationPendingNeedsOwnerAndRationaleTest,
	"RefactorTactics.Presentation.PendingNeedsOwnerAndRationale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPresentationPendingNeedsOwnerAndRationaleTest::RunTest(const FString&)
{
	const TArray<ERTResolvedEventType> AllTypes = PresAllEventTypes();

	// Tabella completa in cui SOLO `Move` e' dichiarato nel modo in esame: l'unica mancanza attesa e' quella.
	auto TabellaCon = [&AllTypes](const FRTPresentationBinding& InEsame)
	{
		TArray<FRTPresentationBinding> T;
		for (const ERTResolvedEventType Type : AllTypes)
		{
			T.Add(Type == ERTResolvedEventType::Move ? InEsame : PresBindingWithCue(Type));
		}
		return T;
	};

	// (a) In attesa senza owner: la promessa che nessuno puo' riscuotere.
	{
		const FRTPresentationBinding SenzaOwner = FRTPresentationBinding::MakePendingPresentation(
			ERTResolvedEventType::Move, TEXT("motivo scritto"), TEXT("   "));
		const TArray<FString> Missing =
			URTPresentationBindingLibrary::FindMissingBindings(TabellaCon(SenzaOwner));
		TestEqual(TEXT("(a) PendingPresentation senza owner non copre"), Missing.Num(), 1);
	}

	// (b) In attesa senza motivo: vale la stessa regola di `NoPresentation`, e non e' assorbita da (a).
	{
		const FRTPresentationBinding SenzaMotivo = FRTPresentationBinding::MakePendingPresentation(
			ERTResolvedEventType::Move, TEXT("  "), TEXT("#2454"));
		const TArray<FString> Missing =
			URTPresentationBindingLibrary::FindMissingBindings(TabellaCon(SenzaMotivo));
		TestEqual(TEXT("(b) PendingPresentation senza motivo non copre"), Missing.Num(), 1);
	}

	// (c) Controprova: ben formata, COPRE.
	{
		const FRTPresentationBinding BenFormata = FRTPresentationBinding::MakePendingPresentation(
			ERTResolvedEventType::Move, TEXT("motivo scritto"), TEXT("#2454"));
		const TArray<FString> Missing =
			URTPresentationBindingLibrary::FindMissingBindings(TabellaCon(BenFormata));
		TestEqual(TEXT("(c) PendingPresentation con motivo e owner copre"), Missing.Num(), 0);
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------

/**
 * Il censimento delle assenze e' PINNATO: quante decise, quante in attesa, e ogni in-attesa nomina l'owner.
 *
 * 🔴 **E' il test che rende falsificabile la classificazione, e senza di esso `#2483` non consegna niente.**
 * I due test sopra provano che il *gate* sa distinguere; questo prova che le voci *reali* sono classificate.
 * Declassare `AttackFootprint` da `PendingPresentation` a `NoPresentation` lascerebbe il gate verde — la
 * voce resterebbe ben formata — e la distinzione morirebbe in silenzio. Qui diventa rossa.
 *
 * ⚠️ **I numeri sono un'osservazione, non una derivazione**, e vanno **rimisurati** se il censimento cambia
 * per una ragione legittima — non aggiornati d'ufficio al numero nuovo. Chi li cambia dichiari perche'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPresentationAbsenceCensusIsPinnedTest,
	"RefactorTactics.Presentation.AbsenceCensusIsPinned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPresentationAbsenceCensusIsPinnedTest::RunTest(const FString&)
{
	const TArray<FRTPresentationBinding> Reale = URTPresentationBindingLibrary::DeclaredBindings();

	int32 Decise = 0;
	int32 InAttesa = 0;
	int32 InAttesaSenzaOwner = 0;
	for (const FRTPresentationBinding& B : Reale)
	{
		if (B.Kind == ERTPresentationKind::NoPresentation) { ++Decise; }
		else if (B.Kind == ERTPresentationKind::PendingPresentation)
		{
			++InAttesa;
			if (B.PendingOwner.TrimStartAndEnd().IsEmpty()) { ++InAttesaSenzaOwner; }
		}
	}

	// 🔴 **Rimisurato il 2026-09-05: zero decise, quattro in attesa** — `HazardDamage` e' passata a
	// `PendingPresentation`, e con lei l'ultima assenza decisa e' sparita.
	//
	// 🔑 **Il numero dice qualcosa di vero sulla v0.1.** Dei sette tipi, tre hanno cue e **quattro sono in
	// attesa**: in questa release nulla e' invisibile *per scelta definitiva* — e' solo non ancora
	// disegnato. ⛔ Il valore `NoPresentation` resta nel contratto ed e' esercitato dai test di questo file:
	// serve al primo evento che si decidera' di non mostrare mai.
	//
	// ⚠️ **Se questa riga tornasse a 1, qualcuno avra' preso quella decisione: la si cerchi.** Non si
	// aggiorna il numero — si cerca chi l'ha deciso e perche'.
	TestEqual(TEXT("nessuna assenza e' DECISA in v0.1"), Decise, 0);
	TestEqual(TEXT("quattro assenze sono IN ATTESA"), InAttesa, 4);
	TestEqual(TEXT("nessuna voce in attesa e' senza owner"), InAttesaSenzaOwner, 0);

	// Gli owner per nome: senza questa riga il conteggio starebbe in piedi anche con owner scambiati fra
	// loro, e «chi chiude #2454 sa quale riga lo riguarda» tornerebbe a non essere vero.
	auto OwnerDi = [&Reale](ERTResolvedEventType Type) -> FString
	{
		const FRTPresentationBinding* B = Reale.FindByPredicate(
			[Type](const FRTPresentationBinding& X) { return X.Type == Type; });
		return B ? B->PendingOwner : FString();
	};
	TestEqual(TEXT("HazardDamage attende #2505"),
		OwnerDi(ERTResolvedEventType::HazardDamage), FString(TEXT("#2505")));
	TestEqual(TEXT("AttackFootprint attende E21"),
		OwnerDi(ERTResolvedEventType::AttackFootprint), FString(TEXT("E21")));
	TestEqual(TEXT("ReactionResolved attende #2454"),
		OwnerDi(ERTResolvedEventType::ReactionResolved), FString(TEXT("#2454")));
	TestEqual(TEXT("StatusChanged attende #2456"),
		OwnerDi(ERTResolvedEventType::StatusChanged), FString(TEXT("#2456")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
