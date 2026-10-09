#include "Misc/AutomationTest.h"

#include "RTHexEditorClick.h"
#include "Map/RTGeometryGrammar.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * GEOMETRY PARTECIPA ALLA SELEZIONE, e prende solo il gesto che gia' non produceva nulla (#1864, casella 2).
 *
 * 🔴 **Il difetto: il tool Geometry era un consumatore della selezione condivisa, non un partecipante.**
 * Misurato — i tre tool la DISEGNANO (`DrawSharedSelection` in Select, Geometry e Arch), ma solo Select e
 * Arch la SCRIVEVANO. Il criterio chiede che sia condivisa *fra* Select, Geometry e Arch.
 *
 * 🔑 **Il gesto libero c'era gia'**: `OnClickRelease` usciva senza toccare la mappa quando lo snap non
 * produceva un segmento. Ma fra i modi in cui puo' non produrlo, **uno solo** significa «non stavo
 * disegnando», e prenderli tutti sarebbe rubare il gesto a chi disegna.
 *
 * ⚠️ **La controprova e' meta' del test, ed e' la meta' che discrimina**: un test che asserisse solo
 * «`SameAnchor` seleziona» passerebbe anche con una funzione che risponde sempre `true`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexGestureSelectsOnlyOnTheEmptyOneTest,
	"RefactorTactics.Editor.Selection.GeometrySelectsOnlyWhereItWouldDrawNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexGestureSelectsOnlyOnTheEmptyOneTest::RunTest(const FString&)
{
	constexpr float Hex = 100.f;
	const FVector2D Fermo(10.f, 10.f); // premuto e rilasciato nello stesso punto

	// L'UNICO che e' un click: i due estremi sono lo stesso anchor, quindi non c'e' lunghezza.
	TestTrue(TEXT("due estremi sullo stesso anchor sono un CLICK, e un click seleziona"),
		RTHexEditor::GestureIsASelection(ERTAnchorPairRefusal::SameAnchor, Fermo, Fermo, Hex));

	// ⛔ CONTROPROVA: ogni altro esito NON deve selezionare. `None` significa che il muro si fa — li' il
	// gesto non arriva nemmeno a questa domanda — e gli altri dicono che il disegno e' fallito.
	//
	// ⚠️ `DifferentCell` e `DifferentLayer` NON sono producibili dal gesto di Geometry: `ExplainPair`
	// li da' confrontando la cella dei due anchor, e `SnapGestureToAnchors` passa `ActiveCell` a
	// entrambe le `NearestAnchor`. Si asseriscono lo stesso perche' la funzione e' PURA e non sa chi la
	// chiama — ma non vanno letti come «il caso e' gestito»: da li' quel gesto arriva travestito.
	TestFalse(TEXT("un gesto che PRODUCE un segmento non e' una selezione"),
		RTHexEditor::GestureIsASelection(ERTAnchorPairRefusal::None, Fermo, Fermo, Hex));
	TestFalse(TEXT("un trascinamento fra due celle stava disegnando"),
		RTHexEditor::GestureIsASelection(ERTAnchorPairRefusal::DifferentCell, Fermo, Fermo, Hex));
	TestFalse(TEXT("e fra due layer anche"),
		RTHexEditor::GestureIsASelection(ERTAnchorPairRefusal::DifferentLayer, Fermo, Fermo, Hex));

	// 🔑 LA GUARDIA CHE CONTA, e non e' fra due letterali: si enumera l'enum per RIFLESSIONE e si
	// asserisce che i valori che selezionano siano **esattamente uno**. Un valore nuovo aggiunto alla
	// grammatica — un rifiuto che oggi non esiste — entra qui da solo, e se qualcuno lo facesse
	// selezionare per sbaglio questo conteggio lo direbbe.
	const UEnum* Enum = StaticEnum<ERTAnchorPairRefusal>();
	if (!TestNotNull(TEXT("l'enum dei rifiuti ha la riflessione"), Enum))
	{
		return false;
	}

	// `NumEnums() - 1`: l'ultimo e' il `_MAX` sintetico che UHT aggiunge.
	const int32 Valori = Enum->NumEnums() - 1;
	if (!TestTrue(TEXT("l'enum dichiara dei valori"), Valori > 1))
	{
		return false;
	}

	int32 Selezionano = 0;
	FString Quali;
	for (int32 Index = 0; Index < Valori; ++Index)
	{
		const ERTAnchorPairRefusal R = static_cast<ERTAnchorPairRefusal>(Enum->GetValueByIndex(Index));
		if (RTHexEditor::GestureIsASelection(R, Fermo, Fermo, Hex))
		{
			++Selezionano;
			Quali += (Quali.IsEmpty() ? TEXT("") : TEXT(", "));
			Quali += Enum->GetNameStringByIndex(Index);
		}
	}

	TestEqual(FString::Printf(
		TEXT("esattamente UN esito della grammatica vale come selezione (sono: %s)"), *Quali),
		Selezionano, 1);
	TestEqual(TEXT("ed e' SameAnchor"), Quali, FString(TEXT("SameAnchor")));

	return true;
}


/**
 * UN GESTO LUNGO NON E' UN CLICK, nemmeno quando la grammatica dice `SameAnchor` (#1864, casella 2).
 *
 * 🔴 **Il difetto che la prima stesura aveva, trovato da una code review.** La regola era il solo
 * `Refusal == SameAnchor`, e sembrava esatta: «i due estremi sono lo stesso anchor, quindi non c'e'
 * lunghezza». Ma `URTGeometryGrammarLibrary::NearestAnchor` **non ha nessun limite di distanza** — prende
 * il piu' vicino fra i tredici anchor della cella, a qualunque distanza si trovi il punto.
 *
 * ∴ un trascinamento **lungo** che resta dalla parte dello stesso anchor aggancia entrambi gli estremi a
 * quello: premere verso il punto medio del lato `E` e tirare oltre il bordo, dentro la cella vicina, e'
 * un disegno che `ExplainPair` chiama `SameAnchor`. La prima stesura avrebbe **cancellato la selezione
 * multipla** di chi stava disegnando un muro.
 *
 * ⚠️ **La soglia e' in frazione di `HexSize`, e la distinzione dai pixel resta valida**: una frazione di
 * `HexSize` e' una misura del MONDO, invariante allo zoom, mentre il `ClickDistanceThreshold` dell'engine
 * e' in pixel di schermo e dipende dalla camera. E' la convenzione che questo modulo usa gia'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexGestureLongDragIsNotAClickTest,
	"RefactorTactics.Editor.Selection.ALongDragIsNotAClickEvenOnTheSameAnchor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexGestureLongDragIsNotAClickTest::RunTest(const FString&)
{
	constexpr float Hex = 100.f;

	// Il gesto che la review ha costruito: premuto verso il lato `E`, rilasciato OLTRE il bordo. Entrambi
	// gli estremi restano piu' vicini al punto medio di quel lato, quindi la grammatica dice `SameAnchor`.
	const FVector2D Premuto(0.80f * Hex, 0.f);
	const FVector2D Rilasciato(2.00f * Hex, 0.f);

	TestFalse(TEXT("un trascinamento lungo NON e' un click, benche' la grammatica dica SameAnchor"),
		RTHexEditor::GestureIsASelection(ERTAnchorPairRefusal::SameAnchor, Premuto, Rilasciato, Hex));

	// CONTROPROVA: lo stesso rifiuto, con gli estremi FERMI, e' un click. Senza questa, una regola che
	// rispondesse sempre `false` supererebbe l'asserzione qui sopra.
	TestTrue(TEXT("e fermo nello stesso punto lo e'"),
		RTHexEditor::GestureIsASelection(ERTAnchorPairRefusal::SameAnchor, Premuto, Premuto, Hex));

	// ⚠️ **La soglia scala con `HexSize`, e non e' un dettaglio**: la stessa distanza in unita' di mondo
	// e' un click su una mappa a esagoni grandi e un trascinamento su una a esagoni piccoli. Se fosse una
	// costante assoluta, questa coppia di asserzioni non potrebbe reggere entrambe.
	const FVector2D Poco(0.80f * Hex, 0.02f * Hex);
	TestTrue(TEXT("uno scarto ben dentro la soglia resta un click"),
		RTHexEditor::GestureIsASelection(ERTAnchorPairRefusal::SameAnchor, Premuto, Poco, Hex));
	TestFalse(TEXT("e lo stesso scarto su esagoni dieci volte piu' piccoli non lo e'"),
		RTHexEditor::GestureIsASelection(ERTAnchorPairRefusal::SameAnchor, Premuto, Poco, Hex * 0.1f));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
