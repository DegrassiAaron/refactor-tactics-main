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
	/** Il punto mondo di un estremo di segmento, usato dai due test di contenimento di questo file. */
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
 * 🔴 **Il test che DIMENSIONA le cifre.** Nessun segmento esce dall'esagono. `-10,-10,1` ha nove
 * caratteri (due segni meno) — lungo, ma non il caso peggiore in assoluto: quello e'
 * `NothingLeavesTheHexagonAtTheTrueWorstCase` qui sotto, dove il layer stesso e' negativo a due cifre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLabelStaysInsideTest,
	"RefactorTactics.HexLabel.NothingLeavesTheHexagon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLabelStaysInsideTest::RunTest(const FString&)
{
	constexpr float HexSize = 150.f;    // il default di `URTHexMapAsset`
	constexpr float LayerHeight = 250.f;
	const FVector MapOrigin = FVector::ZeroVector;

	// Nove caratteri, due segni meno: una terna lunga ma non la piu' lunga possibile (vedi il test
	// gemello sotto per quella).
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
 * Il VERO caso peggiore, non quello che sembra tale: `FRTCellId::Layer` e' un `int32` senza limiti
 * dichiarati (`RTCellId.h`), quindi `-10,-10,-1` — dieci caratteri, tre segni meno, il layer negativo a
 * due cifre a meta' scala — e' una cella legittima. `BuildCellLabel` dichiara `WorstCaseChars = 10`:
 * questo test e' cio' che verifica che la dichiarazione sia vera, non solo plausibile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLabelStaysInsideAtTrueWorstCaseTest,
	"RefactorTactics.HexLabel.NothingLeavesTheHexagonAtTheTrueWorstCase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLabelStaysInsideAtTrueWorstCaseTest::RunTest(const FString&)
{
	constexpr float HexSize = 150.f;    // il default di `URTHexMapAsset`
	constexpr float LayerHeight = 250.f;
	const FVector MapOrigin = FVector::ZeroVector;

	// Dieci caratteri: il layer negativo aggiunge un terzo segno meno e una seconda cifra.
	const FRTCellId Worst(-10, -10, -1);
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
 * 🔴 **UNA sola run, allineata alla vista dall'alto.** Erano tre a `120` gradi, e questo test le contava.
 *
 * ⚠️ La sostituzione non e' una semplificazione: e' un difetto chiuso. Tre run a `120` gradi significano
 * che, da una camera FERMA, **due su tre sono ruotate** e si leggono al rovescio — due terzi
 * dell'inchiostro che somiglia a una coordinata sbagliata. Il vecchio test misurava fedelmente proprio
 * quella proprieta', e non poteva accorgersi che fosse il problema.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLabelThreeDirectionsTest,
	"RefactorTactics.HexLabel.OneRunAlignedWithTheTopDownCamera",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLabelThreeDirectionsTest::RunTest(const FString&)
{
	const FRTCellLabel Label = URTHexLabelLibrary::BuildCellLabel(
		FRTCellId(0, 0, 0), FVector::ZeroVector, 150.f, 250.f);
	TestTrue(TEXT("l'etichetta non e' vuota"), Label.Glyphs.Num() > 0);

	// UNA direzione, non tre: gli angoli distinti dell'asse `Right`, arrotondati al grado.
	//
	// ⚠️ Si normalizza DOPO l'arrotondamento, non prima: `-0.0000001` arrotondato prima della
	// normalizzazione diventerebbe `360`, un grado che il test non cerca mai.
	TSet<int32> Angles;
	for (const FRTLabelGlyph& G : Label.Glyphs)
	{
		const int32 Round = FMath::RoundToInt(FMath::RadiansToDegrees(FMath::Atan2(G.Right.Y, G.Right.X)));
		Angles.Add(((Round % 360) + 360) % 360);
	}
	TestEqual(TEXT("una direzione sola, non tre"), Angles.Num(), 1);

	// E la direzione e' quella della CAMERA, ricavata dai due assi dello schermo invece che incisa:
	// nella vista dall'alto di Unreal `Y` va a destra e `X` va in su.
	for (const FRTLabelGlyph& G : Label.Glyphs)
	{
		TestTrue(TEXT("l'asse di lettura e' la destra dello schermo"),
			FVector::DotProduct(G.Right.GetSafeNormal(), FVector::YAxisVector) > 0.999);
		TestTrue(TEXT("l'alto dei caratteri e' l'alto dello schermo"),
			FVector::DotProduct(G.Up.GetSafeNormal(), FVector::XAxisVector) > 0.999);
	}
	return true;
}

/**
 * 🔴 **Si legge in avanti, non riflessa, e il layer e' a META' scala.**
 *
 * Sostituisce `RunsAlongTheSideAndReadsForward`, e prima ancora `RunsInwardAndLayerIsHalfSize`. Le tre
 * stesure raccontano tre difetti diversi, e vale la pena tenerne memoria perche' **ogni volta la suite
 * era verde**:
 *
 *  1. la riga correva in direzione radiale con i caratteri posati **all'indietro** — `3,-2,0` si leggeva
 *     `0,2-,3`. Le asserzioni di allora (distanza dal bordo, altezza del layer) non guardavano
 *     l'orientamento dei glifi;
 *  2. i glifi erano **ribaltati sull'orizzontale**: `2` disegnato come `5` — a sette segmenti sono l'una
 *     il ribaltamento verticale dell'altra — e la virgola in alto. L'asserzione di allora incideva
 *     `(Right x Up).Z > 0`, che e' la condizione giusta in una base **destrorsa** con `X` a est: Unreal
 *     e' mancino, e il test confermava la mia convenzione invece della leggibilita';
 *  3. due run su tre erano ruotate di `120` gradi da una camera ferma.
 *
 * ⚠️ **Nessuno dei tre e' stato trovato da un test.** Tutti e tre da un occhio sul viewport. Cio' che i
 * test possono fare e' impedire che tornino, ed e' per questo che il segno atteso adesso si RICAVA dai
 * due assi dello schermo invece di essere inciso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLabelOrderAndScaleTest,
	"RefactorTactics.HexLabel.ReadsForwardAndLayerIsHalfSize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLabelOrderAndScaleTest::RunTest(const FString&)
{
	const FRTCellId Cell(2, -3, 0);
	const FVector MapOrigin = FVector::ZeroVector;
	const FRTCellLabel Label = URTHexLabelLibrary::BuildCellLabel(Cell, MapOrigin, 150.f, 250.f);

	// Ritorno anticipato: senza almeno tre caratteri, `Glyphs[0]`/`Last()` sarebbero fuori limite.
	if (!TestTrue(TEXT("la terna ha almeno tre caratteri"), Label.Glyphs.Num() >= 3))
	{
		return false;
	}
	const FRTLabelGlyph& First = Label.Glyphs[0];
	const FRTLabelGlyph& Last  = Label.Glyphs.Last();

	// 1. Piatta sul pavimento: nessun asse esce dal piano della cella.
	for (const FRTLabelGlyph& G : Label.Glyphs)
	{
		TestTrue(TEXT("nessun asse del glifo esce dal piano della cella"),
			FMath::Abs(G.Right.Z) + FMath::Abs(G.Up.Z) < 0.01);
	}

	// 2. 🔑 NON SPECCHIATA: la riga avanza NEL VERSO in cui i caratteri si leggono.
	const FVector Advance = (Last.Origin - First.Origin).GetSafeNormal();
	TestTrue(TEXT("l'avanzamento concorda con l'asse di lettura dei caratteri"),
		FVector::DotProduct(Advance, First.Right.GetSafeNormal()) > 0.99);

	// 3. 🔴 NON RIFLESSA, e il segno atteso si RICAVA dalla convenzione invece di essere inciso.
	//    Inciso l'avevo gia' inciso storto: `(Right x Up).Z > 0` e' la condizione di leggibilita' in una
	//    base DESTRORSA con `X` a est e `Y` a nord — la convenzione della matematica. Unreal e' MANCINO e
	//    la sua vista dall'alto mette `X` in su e `Y` a destra: li' la condizione si rovescia. Costruendo
	//    il riferimento dai due assi dello schermo, il segno non e' piu' una mia opinione.
	const double Reference = FVector::CrossProduct(FVector::YAxisVector, FVector::XAxisVector).Z;
	const double Actual    = FVector::CrossProduct(First.Right, First.Up).Z;
	TestTrue(TEXT("i glifi hanno lo stesso verso degli assi dello schermo: non riflessi"),
		Actual * Reference > 0.0);

	// 4. Centrata sulla cella: il centro della riga cade sul centro della cella. Senza, una riga che
	//    cominciasse dal centro invece di esservi centrata passerebbe tutte le altre asserzioni.
	const FVector CellCentre = URTHexLibrary::AxialToWorld(Cell, MapOrigin, 150.f, 250.f);
	const FVector RowMid = (First.Origin + Last.Origin + Last.Right + Last.Up) * 0.5;
	TestTrue(TEXT("la riga e' centrata sulla cella"), FVector::Dist2D(RowMid, CellCentre) < 12.0);

	// L'ultimo carattere della terna e' il layer: alto meta' del primo.
	TestTrue(TEXT("il layer e' alto meta' delle altre componenti"),
		FMath::IsNearlyEqual(Last.Up.Size(), First.Up.Size() * 0.5, 0.5));
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

	// ⌫ Attendeva **3** — uno per ciascuna delle tre run — e il numero era inciso. Da quando la run e'
	// una sola quel `3` sarebbe da riscrivere a ogni cambio di layout, quindi adesso l'atteso si DERIVA
	// dalla stessa stringa che la funzione compone: se un giorno le run tornassero a essere piu' d'una,
	// questo test lo direbbe invece di limitarsi a fallire su una costante vecchia.
	FString Expected = FString::Printf(TEXT("%d,%d,%d"), -2, 1, 0);
	int32 ExpectedMinus = 0;
	for (const TCHAR C : Expected) { if (C == TEXT('-')) { ++ExpectedMinus; } }
	TestEqual(TEXT("il segno meno c'e', e ce n'e' uno per ogni componente negativa"), Minus, ExpectedMinus);

	// ⛔ E non compare dove non serve: una terna tutta positiva non ha segni.
	const FRTCellLabel Positive = URTHexLabelLibrary::BuildCellLabel(
		FRTCellId(2, 1, 0), FVector::ZeroVector, 150.f, 250.f);
	int32 SpuriousMinus = 0;
	for (const FRTLabelGlyph& G : Positive.Glyphs) { if (G.Character == TEXT('-')) { ++SpuriousMinus; } }
	TestEqual(TEXT("nessun segno meno su una terna positiva"), SpuriousMinus, 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
