#include "Misc/AutomationTest.h"
#include "Map/RTHexLabel.h"
#include "Map/RTHexLabelLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il font a segmenti delle coordinate (#1920).
 *
 * ⛔ Non e' un font: e' il set CHIUSO di dodici caratteri che una terna di coordinate puo' contenere. Un
 * font vero e' l'approccio C della spec, e questa architettura lo lascia possibile senza anticiparlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLabelGlyphSetTest,
	"RefactorTactics.HexLabel.GlyphSetIsClosedAndNormalised",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLabelGlyphSetTest::RunTest(const FString&)
{
	const FString Alphabet = TEXT("0123456789,-");

	for (const TCHAR Ch : Alphabet)
	{
		const TArray<FRTLabelStroke> Strokes = URTHexLabelLibrary::GlyphStrokes(Ch);
		TestTrue(FString::Printf(TEXT("'%c' ha almeno un segmento"), Ch), Strokes.Num() > 0);

		for (const FRTLabelStroke& S : Strokes)
		{
			const bool bInside =
				S.From.X >= 0.f && S.From.X <= 1.f && S.From.Y >= 0.f && S.From.Y <= 1.f &&
				S.To.X   >= 0.f && S.To.X   <= 1.f && S.To.Y   >= 0.f && S.To.Y   <= 1.f;
			TestTrue(FString::Printf(TEXT("'%c' resta nel quadrato unitario"), Ch), bInside);
		}
	}

	// 🔴 Un carattere fuori set NON produce segmenti inventati: meglio niente che un glifo che nessuno
	// ha disegnato, e che a schermo sembrerebbe una cifra sbagliata invece che un dato mancante.
	TestEqual(TEXT("un carattere fuori set non disegna nulla"),
		URTHexLabelLibrary::GlyphStrokes(TEXT('Z')).Num(), 0);
	return true;
}

/**
 * Le dieci cifre sono DISTINTE fra loro. Questo test guarda solo l'INSIEME delle dieci forme, non quale
 * forma sta su quale cifra: da solo non basta a garantire la tabella giusta (vedi
 * 'DigitShapesMatchRecognizableStructure' qui sotto), ma senza di lui un font che assegna a due cifre
 * lo stesso identico set di segmenti — non vuoto, dentro il quadrato — supererebbe comunque
 * 'GlyphSetIsClosedAndNormalised', perche' quel test valida apertura e confini per ogni cifra presa da
 * sola, non l'unicita' fra cifre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLabelDigitsDifferTest,
	"RefactorTactics.HexLabel.EveryDigitLooksDifferent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLabelDigitsDifferTest::RunTest(const FString&)
{
	TSet<FString> Shapes;
	for (int32 D = 0; D <= 9; ++D)
	{
		FString Shape;
		for (const FRTLabelStroke& S : URTHexLabelLibrary::GlyphStrokes(TEXT('0') + D))
		{
			Shape += FString::Printf(TEXT("%.2f,%.2f-%.2f,%.2f;"), S.From.X, S.From.Y, S.To.X, S.To.Y);
		}
		Shapes.Add(Shape);
	}
	TestEqual(TEXT("dieci cifre, dieci forme diverse"), Shapes.Num(), 10);
	return true;
}

/**
 * Le forme non solo esistono, sono normalizzate e distinte fra loro: hanno anche la struttura GIUSTA
 * per la cifra che rappresentano. Senza questo test, scambiare per errore i corpi di due `case` dello
 * switch (es. '2' con '5') lascerebbe la suite verde: lo scambio produce comunque dieci forme non
 * vuote, dentro il quadrato e reciprocamente distinte — i due test sopra non guardano QUALE forma sta
 * su QUALE cifra, solo che le forme esistano e differiscano fra loro.
 *
 * Ogni asserzione verifica una proprieta' geometrica riconoscibile a occhio sulla cifra risultante, MAI
 * la tabella dei segmenti: non confronta contro `RTHexLabelSegX`, solo contro coordinate e conteggi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLabelStructureTest,
	"RefactorTactics.HexLabel.DigitShapesMatchRecognizableStructure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLabelStructureTest::RunTest(const FString&)
{
	// '1': due soli segmenti, entrambi sul lato destro del quadrato.
	{
		const TArray<FRTLabelStroke> Strokes = URTHexLabelLibrary::GlyphStrokes(TEXT('1'));
		TestEqual(TEXT("'1' ha due segmenti"), Strokes.Num(), 2);
		for (const FRTLabelStroke& S : Strokes)
		{
			TestTrue(TEXT("'1' resta sul lato destro (X == 1)"), S.From.X == 1.f && S.To.X == 1.f);
		}
	}

	// '7': tre segmenti.
	TestEqual(TEXT("'7' ha tre segmenti"), URTHexLabelLibrary::GlyphStrokes(TEXT('7')).Num(), 3);

	// '8': tutti e sette i segmenti del display.
	TestEqual(TEXT("'8' ha tutti e sette i segmenti"), URTHexLabelLibrary::GlyphStrokes(TEXT('8')).Num(), 7);

	// '0': sei segmenti, e nessuno di questi attraversa il quadrato a meta' altezza: '0' non ha la
	// sbarra di mezzo che invece hanno '2', '3', '4', '5', '6', '8' e '9'.
	{
		const TArray<FRTLabelStroke> Strokes = URTHexLabelLibrary::GlyphStrokes(TEXT('0'));
		TestEqual(TEXT("'0' ha sei segmenti"), Strokes.Num(), 6);

		bool bHasMiddleBar = false;
		for (const FRTLabelStroke& S : Strokes)
		{
			if (S.From.Y == 0.5f && S.To.Y == 0.5f)
			{
				bHasMiddleBar = true;
			}
		}
		TestFalse(TEXT("'0' non ha il segmento centrale"), bHasMiddleBar);
	}

	// '2' e '5' hanno lo stesso numero di segmenti e condividono la sbarra centrale: cio' che li
	// distingue e' su quale lato sta il segmento verticale della meta' superiore del quadrato. '2' si
	// apre a destra in alto, '5' si apre a sinistra in alto.
	auto FindUpperHalfVerticalSide = [](TCHAR Digit, bool& bOutFound, bool& bOutOnRight)
	{
		bOutFound = false;
		bOutOnRight = false;
		for (const FRTLabelStroke& S : URTHexLabelLibrary::GlyphStrokes(Digit))
		{
			const bool bVertical = S.From.X == S.To.X;
			const float MinY = FMath::Min(S.From.Y, S.To.Y);
			const float MaxY = FMath::Max(S.From.Y, S.To.Y);
			if (bVertical && MinY == 0.5f && MaxY == 1.f)
			{
				bOutFound = true;
				bOutOnRight = (S.From.X == 1.f);
				return;
			}
		}
	};

	bool bTwoFound = false, bTwoOnRight = false;
	FindUpperHalfVerticalSide(TEXT('2'), bTwoFound, bTwoOnRight);
	TestTrue(TEXT("'2' ha un segmento verticale nella meta' superiore"), bTwoFound);
	TestTrue(TEXT("'2' si apre a destra in alto"), bTwoOnRight);

	bool bFiveFound = false, bFiveOnRight = false;
	FindUpperHalfVerticalSide(TEXT('5'), bFiveFound, bFiveOnRight);
	TestTrue(TEXT("'5' ha un segmento verticale nella meta' superiore"), bFiveFound);
	TestFalse(TEXT("'5' si apre a sinistra in alto"), bFiveOnRight);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
