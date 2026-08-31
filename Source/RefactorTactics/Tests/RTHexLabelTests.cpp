#include "Misc/AutomationTest.h"
#include "Map/RTHexLabel.h"
#include "Map/RTHexLabelLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"

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

namespace
{
	/** Il punto mondo di un estremo di segmento. Un'unica formula, usata da ogni test di questo file. */
	FVector HexLabelStrokePoint(const FRTLabelGlyph& G, const FVector2D& Local)
	{
		return G.Origin + G.Right * Local.X + G.Up * Local.Y;
	}

	/** Dentro il poligono convesso, sul piano XY. I sei vertici arrivano in ordine da `CellCorners`. */
	bool HexLabelInsideHex(const TArray<FVector>& Corners, const FVector& P)
	{
		for (int32 I = 0; I < Corners.Num(); ++I)
		{
			const FVector& A = Corners[I];
			const FVector& B = Corners[(I + 1) % Corners.Num()];
			const double Cross = (B.X - A.X) * (P.Y - A.Y) - (B.Y - A.Y) * (P.X - A.X);
			if (Cross < -0.01) // tolleranza: il confronto e' su centimetri, non su bit
			{
				return false;
			}
		}
		return true;
	}
}

/**
 * 🔴 **Il test che DIMENSIONA le cifre.** Nessun segmento esce dall'esagono — e il caso e' il peggiore
 * possibile, non `(0,0,0)`: `-10,-10,1` sono dieci caratteri, e una taratura fatta sulla terna corta
 * sborda alla prima mappa grande senza che nessuno se ne accorga prima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLabelStaysInsideTest,
	"RefactorTactics.HexLabel.NothingLeavesTheHexagon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLabelStaysInsideTest::RunTest(const FString&)
{
	constexpr float HexSize = 150.f;    // il default di `URTHexMapAsset`
	constexpr float LayerHeight = 250.f;
	const FVector MapOrigin = FVector::ZeroVector;

	// La terna piu' lunga che una mappa possa produrre: dieci caratteri, due segni meno.
	const FRTCellId Worst(-10, -10, 1);
	const FRTCellLabel Label = URTHexLabelLibrary::BuildCellLabel(Worst, MapOrigin, HexSize, LayerHeight);
	const TArray<FVector> Corners = URTHexLibrary::CellCorners(Worst, MapOrigin, HexSize, LayerHeight);

	TestTrue(TEXT("l'etichetta non e' vuota"), Label.Glyphs.Num() > 0);

	int32 Checked = 0;
	for (const FRTLabelGlyph& G : Label.Glyphs)
	{
		for (const FRTLabelStroke& S : URTHexLabelLibrary::GlyphStrokes(G.Character))
		{
			TestTrue(TEXT("l'inizio del segmento e' dentro l'esagono"), HexLabelInsideHex(Corners, HexLabelStrokePoint(G, S.From)));
			TestTrue(TEXT("la fine del segmento e' dentro l'esagono"),  HexLabelInsideHex(Corners, HexLabelStrokePoint(G, S.To)));
			++Checked;
		}
	}
	TestTrue(TEXT("qualche segmento e' stato davvero controllato"), Checked > 0);
	return true;
}

/**
 * Tre run a 120 gradi esatti, verificato sulle POSE e non a occhio: e' la differenza fra «sembrano
 * ruotate» e «sono ruotate».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLabelThreeDirectionsTest,
	"RefactorTactics.HexLabel.ThreeRunsAtOneHundredTwentyDegrees",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLabelThreeDirectionsTest::RunTest(const FString&)
{
	const FRTCellId Cell(0, 0, 0);
	const FRTCellLabel Label = URTHexLabelLibrary::BuildCellLabel(Cell, FVector::ZeroVector, 150.f, 250.f);

	// Gli angoli distinti dell'asse `Right`, arrotondati al grado: devono essere tre.
	//
	// ⚠️ Si normalizza DOPO l'arrotondamento, non prima: un angolo come -0.0000001 arrotondato PRIMA
	// della normalizzazione (Deg + 360.0) diventerebbe 359.9999999, poi 360 per arrotondamento — un
	// grado che il test non cerca mai, perche' non e' ne' 0 ne' un multiplo di 120. Arrotondando prima
	// e normalizzando dopo, -0.0000001 arrotonda a 0 e il modulo lo lascia 0.
	TSet<int32> Angles;
	for (const FRTLabelGlyph& G : Label.Glyphs)
	{
		const double Deg = FMath::RadiansToDegrees(FMath::Atan2(G.Right.Y, G.Right.X));
		const int32 Round = FMath::RoundToInt(Deg);
		Angles.Add(((Round % 360) + 360) % 360);
	}

	TestEqual(TEXT("tre direzioni, non una ne' sei"), Angles.Num(), 3);

	TArray<int32> Sorted = Angles.Array();
	Sorted.Sort();
	TestEqual(TEXT("la prima e' a 0 gradi: il punto medio del primo lato"), Sorted[0], 0);
	TestEqual(TEXT("la seconda a 120"), Sorted[1], 120);
	TestEqual(TEXT("la terza a 240"),   Sorted[2], 240);
	return true;
}

/**
 * La terna corre DAL BORDO AL CENTRO, e il layer e' a META' scala. Due criteri della spec che, sbagliati,
 * danno un'etichetta plausibile: le cifre ci sono, stanno dentro, e dicono la cosa nell'ordine sbagliato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLabelOrderAndScaleTest,
	"RefactorTactics.HexLabel.RunsInwardAndLayerIsHalfSize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLabelOrderAndScaleTest::RunTest(const FString&)
{
	const FRTCellId Cell(2, -3, 0);
	const FVector MapOrigin = FVector::ZeroVector;
	const FRTCellLabel Label = URTHexLabelLibrary::BuildCellLabel(Cell, MapOrigin, 150.f, 250.f);
	const FVector Centre = URTHexLibrary::AxialToWorld(Cell, MapOrigin, 150.f, 250.f);

	// La run a 0 gradi, nell'ordine in cui e' stata costruita.
	TArray<FRTLabelGlyph> Run;
	for (const FRTLabelGlyph& G : Label.Glyphs)
	{
		if (FMath::IsNearlyZero(G.Right.Y, 0.01) && G.Right.X > 0.0)
		{
			Run.Add(G);
		}
	}
	TestTrue(TEXT("la run a 0 gradi ha almeno tre caratteri"), Run.Num() >= 3);

	const double FirstDist = FVector::Dist2D(Run[0].Origin, Centre);
	const double LastDist  = FVector::Dist2D(Run.Last().Origin, Centre);
	TestTrue(TEXT("il primo carattere e' piu' vicino al BORDO dell'ultimo"), FirstDist > LastDist);

	// L'ultimo carattere della terna e' il layer: alto meta' del primo.
	const double FirstHeight = Run[0].Up.Size();
	const double LastHeight  = Run.Last().Up.Size();
	TestTrue(TEXT("il layer e' alto meta' delle altre componenti"),
		FMath::IsNearlyEqual(LastHeight, FirstHeight * 0.5, 0.5));
	return true;
}

/**
 * Le coordinate negative si leggono: il segno meno e' nella terna, non sottinteso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLabelNegativeTest,
	"RefactorTactics.HexLabel.NegativeCoordinatesShowTheirSign",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLabelNegativeTest::RunTest(const FString&)
{
	const FRTCellLabel Negative = URTHexLabelLibrary::BuildCellLabel(
		FRTCellId(-2, 1, 0), FVector::ZeroVector, 150.f, 250.f);

	int32 Minus = 0;
	for (const FRTLabelGlyph& G : Negative.Glyphs) { if (G.Character == TEXT('-')) { ++Minus; } }

	// Una `x` negativa, tre direzioni: tre segni meno, uno per run.
	TestEqual(TEXT("un segno meno per ciascuna delle tre run"), Minus, 3);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
