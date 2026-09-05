#include "Misc/AutomationTest.h"

#include "RTHexAnchorReadout.h"

#include "Map/RTCellId.h"
#include "Map/RTGeometryGrammar.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Cosa il gesto di geometria SCRIVE dato un rifiuto gia' deciso dal runtime — `#1895`.
 *
 * 🔑 **Perche' questi test esistono qui.** Di un `UClickDragTool` un automation test non vede il ghost e non
 * trascina un mouse; vede pero' *cosa si scrive dato un verdetto*, ed e' la meta' che — sbagliata — produce
 * un'interfaccia plausibile e falsa. E' la stessa scelta di `RTHexProbeReadout` (`#711`) e `RTHexLos`
 * (`#1755`), e rende headless tre dei criteri di accettazione di questa issue.
 */

namespace
{
	/** Nomi distinti per file: namespace anonimo + unity build (vedi `IncidenceHexSize` in #1530). */
	const FRTCellId AnchorReadoutCell(0, 0, 0);

	FRTAnchorRef ReadoutCentre()
	{
		return FRTAnchorRef(AnchorReadoutCell, ERTAnchorKind::Center);
	}

	FRTAnchorRef ReadoutVertex(int32 Index)
	{
		return FRTAnchorRef(AnchorReadoutCell, ERTAnchorKind::Vertex, Index);
	}
}

/**
 * 🔴 **OGNI RIFIUTO HA LA SUA RIGA** — il test per cui questo readout esiste.
 *
 * Il difetto che previene ha un precedente scritto nel repository, e nasce da un caso reale (`#711`):
 * *«"oltre il budget, bloccata o occupata" mette tre difetti diversi nella stessa frase: chi gioca clicca
 * una cella libera, legge "bloccata" e crede a un difetto del gioco»*.
 *
 * Qui varrebbe uguale, e peggio: un autore che ha trascinato su un'altra cella e legge *«non sta su nessun
 * asse tattico»* va a cercare un difetto nella geometria invece che nel proprio gesto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexAnchorReadoutDistinctTest,
	"RefactorTactics.AnchorReadout.EveryRefusalGetsItsOwnLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexAnchorReadoutDistinctTest::RunTest(const FString&)
{
	const ERTAnchorPairRefusal Refusals[] = {
		ERTAnchorPairRefusal::DifferentCell,
		ERTAnchorPairRefusal::DifferentLayer,
		ERTAnchorPairRefusal::SameAnchor,
		ERTAnchorPairRefusal::NoTacticalAxis
	};

	TSet<FString> Seen;
	for (const ERTAnchorPairRefusal Refusal : Refusals)
	{
		const RTHexAnchor::FReadout R = RTHexAnchor::Describe(ReadoutCentre(), ReadoutVertex(0), Refusal);

		TestFalse(*FString::Printf(TEXT("il rifiuto %d non e' valido"), static_cast<int32>(Refusal)),
			R.bValid);
		TestFalse(*FString::Printf(TEXT("il rifiuto %d ha una ragione"), static_cast<int32>(Refusal)),
			R.Reason.IsEmpty());

		bool bAlready = false;
		Seen.Add(R.Reason, &bAlready);
		TestFalse(*FString::Printf(TEXT("la ragione del rifiuto %d non e' gia' di un altro"),
			static_cast<int32>(Refusal)), bAlready);
	}

	// `UE_ARRAY_COUNT` non e' un `int32`: senza il cast i due overload di `TestEqual` sono ambigui.
	TestEqual(TEXT("quattro rifiuti, quattro righe diverse"), Seen.Num(),
		static_cast<int32>(UE_ARRAY_COUNT(Refusals)));
	return true;
}

/**
 * IL GESTO RIUSCITO NON SPIEGA NIENTE, e nomina i due estremi.
 *
 * ⚠️ Una riga di successo su ogni gesto sarebbe rumore su quasi tutti: la ragione esiste per i casi in cui
 * qualcosa non si fa. Ma il NOME degli estremi si vede sempre — e' il criterio *«quale anchor e' agganciato
 * si vede, non si indovina dalla posizione del ghost»*.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexAnchorReadoutValidTest,
	"RefactorTactics.AnchorReadout.ValidGestureNamesBothEnds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexAnchorReadoutValidTest::RunTest(const FString&)
{
	const FRTAnchorRef From = ReadoutCentre();
	const FRTAnchorRef To = ReadoutVertex(3);

	const RTHexAnchor::FReadout R =
		RTHexAnchor::Describe(From, To, ERTAnchorPairRefusal::None);

	TestTrue(TEXT("il gesto e' valido"), R.bValid);
	TestTrue(TEXT("e non spiega niente"), R.Reason.IsEmpty());
	TestTrue(TEXT("il nome del primo estremo compare"), R.Anchor.Contains(From.ToString()));
	TestTrue(TEXT("e quello del secondo pure"), R.Anchor.Contains(To.ToString()));

	// ⚠️ I due estremi devono essere DISTINGUIBILI nella riga: `C -> V3`, non una coppia di nomi appaiati
	// che l'occhio non separa. Senza, «quale anchor e' agganciato» resterebbe da indovinare, che e'
	// esattamente cio' che questa issue toglie.
	TestNotEqual(TEXT("i due nomi non collassano in uno"), From.ToString(), To.ToString());
	TestTrue(TEXT("la riga li separa"), R.Anchor.Len() > From.ToString().Len() + To.ToString().Len());

	return true;
}

/**
 * IL GESTO APPENA INIZIATO NON E' UN RIFIUTO — e non deve somigliarci.
 *
 * 🔑 Premuto il primo estremo, il secondo anchor non esiste ancora: chiedere `ExplainPair(From, From)`
 * darebbe `SameAnchor`, cioe' un verdetto su una coppia che nessuno ha chiesto. E' lo stesso difetto che in
 * `#711` faceva dire «nessuna strada» a una sonda senza unita' selezionata: l'assenza della domanda ha una
 * frase sua.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexAnchorReadoutPendingTest,
	"RefactorTactics.AnchorReadout.PendingIsNotARefusal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexAnchorReadoutPendingTest::RunTest(const FString&)
{
	const FRTAnchorRef From = ReadoutVertex(1);
	const RTHexAnchor::FReadout Pending = RTHexAnchor::DescribePending(From);

	TestFalse(TEXT("non promette un muro"), Pending.bValid);
	TestFalse(TEXT("ma dice qualcosa"), Pending.Reason.IsEmpty());
	TestTrue(TEXT("e nomina l'estremo gia' premuto"), Pending.Anchor.Contains(From.ToString()));

	// LA DISTINZIONE CHE CONTA: la riga dell'attesa non e' quella di nessuno dei quattro rifiuti.
	const ERTAnchorPairRefusal Refusals[] = {
		ERTAnchorPairRefusal::DifferentCell,
		ERTAnchorPairRefusal::DifferentLayer,
		ERTAnchorPairRefusal::SameAnchor,
		ERTAnchorPairRefusal::NoTacticalAxis
	};
	for (const ERTAnchorPairRefusal Refusal : Refusals)
	{
		TestNotEqual(TEXT("l'attesa non si confonde con un rifiuto"),
			Pending.Reason, RTHexAnchor::Describe(From, ReadoutVertex(4), Refusal).Reason);
	}

	return true;
}

/**
 * IL READOUT NON DECIDE: TRADUCE — e questo test lo dimostra invece di dichiararlo.
 *
 * 🔴 Il primo Non-goal di `#1895` e' *«nessuna regola nuova in `Source/RefactorTacticsEditor/`»*. Un modo di
 * violarlo senza accorgersene sarebbe far guardare al readout gli anchor per capire da se' se la coppia si
 * esprime. Qui si passa una coppia **realmente esprimibile** dichiarandola rifiutata, e viceversa: se il
 * readout ricalcolasse, contraddirebbe l'argomento e uno dei due casi cadrebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexAnchorReadoutObeysTheVerdictTest,
	"RefactorTactics.AnchorReadout.ObeysTheVerdictItIsGiven",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexAnchorReadoutObeysTheVerdictTest::RunTest(const FString&)
{
	// Centro e vertice: una coppia che la grammatica porta davvero — lo dice il runtime, non questo file.
	const FRTAnchorRef From = ReadoutCentre();
	const FRTAnchorRef To = ReadoutVertex(0);
	FRTGeometrySegment Segment;
	const bool bReallyExpressible =
		URTGeometryGrammarLibrary::SegmentBetweenAnchors(From, To, 100.0f, Segment);
	TestTrue(TEXT("la coppia scelta e' davvero esprimibile"), bReallyExpressible);

	// ...eppure, dichiarata rifiutata, il readout rifiuta.
	const RTHexAnchor::FReadout Forced =
		RTHexAnchor::Describe(From, To, ERTAnchorPairRefusal::NoTacticalAxis);
	TestFalse(TEXT("obbedisce al verdetto, non alla geometria"), Forced.bValid);
	TestFalse(TEXT("e ne riporta la ragione"), Forced.Reason.IsEmpty());

	// E lo stesso anchor due volte — che il runtime rifiuterebbe — dichiarato valido, passa.
	const RTHexAnchor::FReadout Allowed =
		RTHexAnchor::Describe(From, From, ERTAnchorPairRefusal::None);
	TestTrue(TEXT("e non inventa un rifiuto che non gli e' stato dato"), Allowed.bValid);

	return true;
}

/**
 * 🔴 **OGNI INCIDENZA HA LA SUA RIGA, E NOMINA L'ALTRO MURO** — `#1895` parte 4.
 *
 * Il criterio dell'issue e' che l'incidenza *«si veda sui DUE segmenti coinvolti»*. Uno e' il ghost che
 * l'autore sta guardando; l'altro esiste solo se qualcosa lo nomina — ed e' la ragione per cui `#1894` ha
 * aggiunto `FRTGeometryIssue::OtherIndex` invece di lasciare *«il segmento 4 incrocia qualcosa»*.
 *
 * Le tre configurazioni hanno **rimedi diversi**, e per questo non possono condividere una frase: un
 * incrocio si sposta, una sovrapposizione si accorcia, un duplicato non si disegna affatto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexAnchorIncidenceDistinctTest,
	"RefactorTactics.AnchorReadout.EveryIncidenceNamesTheOtherWall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexAnchorIncidenceDistinctTest::RunTest(const FString&)
{
	const ERTGeometryViolation Incidences[] = {
		ERTGeometryViolation::CrossingOffAnchor,
		ERTGeometryViolation::OverlappingSegments,
		ERTGeometryViolation::DuplicateSegment
	};

	TSet<FString> Seen;
	for (const ERTGeometryViolation Violation : Incidences)
	{
		const FString Line = RTHexAnchor::DescribeIncidence(Violation, /*OtherWallIndex=*/ 7);

		TestFalse(*FString::Printf(TEXT("l'incidenza %d dice qualcosa"), static_cast<int32>(Violation)),
			Line.IsEmpty());

		// IL CRITERIO: l'altro muro e' NOMINATO, non sottinteso.
		TestTrue(*FString::Printf(TEXT("l'incidenza %d nomina l'altro muro"), static_cast<int32>(Violation)),
			Line.Contains(TEXT("7")));

		bool bAlready = false;
		Seen.Add(Line, &bAlready);
		TestFalse(*FString::Printf(TEXT("la riga dell'incidenza %d non e' gia' di un'altra"),
			static_cast<int32>(Violation)), bAlready);
	}

	TestEqual(TEXT("tre incidenze, tre righe diverse"), Seen.Num(),
		static_cast<int32>(UE_ARRAY_COUNT(Incidences)));

	// ⚠️ Senza un indice la riga esiste comunque: il gesto non deve restare muto perche' l'altro segmento
	// non ha un numero da mostrare. Dice «un altro muro», che e' meno utile ma non e' silenzio.
	const FString Anonymous =
		RTHexAnchor::DescribeIncidence(ERTGeometryViolation::CrossingOffAnchor, INDEX_NONE);
	TestFalse(TEXT("senza indice la riga non e' vuota"), Anonymous.IsEmpty());
	TestFalse(TEXT("e non stampa il sentinella"), Anonymous.Contains(TEXT("-1")));

	return true;
}

/**
 * CIO' CHE NON E' UN'INCIDENZA NON PRODUCE UNA RIGA — e il silenzio qui e' la cosa giusta.
 *
 * 🔑 Le violazioni del SINGOLO segmento hanno gia' i loro canali: lo snap le ferma prima che il gesto arrivi
 * qui, e `Refusal` le racconta. Una riga anche in questo campo sarebbe la **seconda voce sullo stesso
 * difetto**, e chi disegna leggerebbe due frasi per un problema solo — che e' il difetto di forma che
 * `#711` ha gia' pagato in senso opposto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexAnchorIncidenceSilentTest,
	"RefactorTactics.AnchorReadout.NonIncidencesStaySilent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexAnchorIncidenceSilentTest::RunTest(const FString&)
{
	const ERTGeometryViolation NotIncidences[] = {
		ERTGeometryViolation::None,
		ERTGeometryViolation::OffAxis,
		ERTGeometryViolation::ZeroLength,
		ERTGeometryViolation::InvalidLayer,
		ERTGeometryViolation::OutsideEditableBounds
	};

	for (const ERTGeometryViolation Violation : NotIncidences)
	{
		TestTrue(*FString::Printf(TEXT("la violazione %d non e' un'incidenza e tace"),
			static_cast<int32>(Violation)),
			RTHexAnchor::DescribeIncidence(Violation, 3).IsEmpty());
	}

	return true;
}

/**
 * IL TRADUTTORE OBBEDISCE AL VERDETTO ANCHE QUI — non guarda la geometria.
 *
 * 🔴 Il primo Non-goal di `#1895` vieta regole nuove nel modulo editor, e il modo di violarlo senza
 * accorgersene sarebbe far dedurre a questa funzione **quale** incidenza sia, dai segmenti. Qui si passa una
 * violazione che il runtime non produrrebbe mai per quella coppia: se la funzione ricalcolasse, il caso
 * cadrebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexAnchorIncidenceObeysTest,
	"RefactorTactics.AnchorReadout.IncidenceObeysTheVerdict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexAnchorIncidenceObeysTest::RunTest(const FString&)
{
	// Due segmenti IDENTICI: il runtime li chiamerebbe `DuplicateSegment`, mai `OverlappingSegments` —
	// `RefactorTactics.Incidence.OverlapIsReported` lo pinna. Dichiarata sovrapposizione, il readout dice
	// sovrapposizione.
	const FString Line =
		RTHexAnchor::DescribeIncidence(ERTGeometryViolation::OverlappingSegments, 2);
	TestFalse(TEXT("obbedisce al verdetto ricevuto"), Line.IsEmpty());
	TestNotEqual(TEXT("e non lo traduce come un duplicato"),
		Line, RTHexAnchor::DescribeIncidence(ERTGeometryViolation::DuplicateSegment, 2));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
