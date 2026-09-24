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
	// L'UNICO che e' un click: i due estremi sono lo stesso anchor, quindi non c'e' lunghezza.
	TestTrue(TEXT("due estremi sullo stesso anchor sono un CLICK, e un click seleziona"),
		RTHexEditor::GestureIsASelection(ERTAnchorPairRefusal::SameAnchor));

	// ⛔ CONTROPROVA: ogni altro esito NON deve selezionare. `None` significa che il muro si fa — li' il
	// gesto non arriva nemmeno a questa domanda — e gli altri dicono che il disegno e' fallito.
	TestFalse(TEXT("un gesto che PRODUCE un segmento non e' una selezione"),
		RTHexEditor::GestureIsASelection(ERTAnchorPairRefusal::None));
	TestFalse(TEXT("un trascinamento fra due celle stava disegnando"),
		RTHexEditor::GestureIsASelection(ERTAnchorPairRefusal::DifferentCell));
	TestFalse(TEXT("e fra due layer anche"),
		RTHexEditor::GestureIsASelection(ERTAnchorPairRefusal::DifferentLayer));

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
		if (RTHexEditor::GestureIsASelection(R))
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

#endif // WITH_DEV_AUTOMATION_TESTS
