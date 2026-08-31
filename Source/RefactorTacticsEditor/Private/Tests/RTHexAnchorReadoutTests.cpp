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

#endif // WITH_DEV_AUTOMATION_TESTS
