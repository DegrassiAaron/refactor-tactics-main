#include "Misc/AutomationTest.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexOccupancyLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** `HexSize` di riferimento: lo stesso usato dalle fixture di occupancy, per confrontare i numeri. */
	constexpr float TestHexSize = 100.0f;

	/** Un segmento che appartiene alla grammatica: radiale sull'asse a 0 gradi, lungo mezzo raggio. */
	FRTGeometrySegment ValidSegment()
	{
		FRTGeometrySegment S;
		S.Axis = ERTTacticalAxis::Deg0;
		S.Offset = 0;
		S.AlongStart = 2;
		S.AlongEnd = 8;
		S.Layer = 0;
		return S;
	}

	/**
	 * Il muro sul LATO `E` dell'esagono — la terza famiglia della grammatica.
	 *
	 * Asse `Deg90` (la giacitura del lato), offset di un punto notevole intero lungo la perpendicolare
	 * (il punto medio del lato), estremi a mezzo quanto per parte (i due vertici).
	 */
	FRTGeometrySegment WallOnEastEdge()
	{
		FRTGeometrySegment S;
		S.Axis = ERTTacticalAxis::Deg90;
		S.Offset = RT_GeometryQuanta;
		S.AlongStart = -RT_GeometryQuanta / 2;
		S.AlongEnd = RT_GeometryQuanta / 2;
		S.Layer = 0;
		return S;
	}
}

/**
 * LA TERZA FAMIGLIA, che e' la ragione per cui il quanto non e' una frazione di `HexSize`.
 *
 * Un muro appoggiato al lato `E` deve ricostruire ESATTAMENTE i due vertici dell'esagono. Con un quanto
 * lineare uniforme non potrebbe: l'apotema vale `HexSize * sqrt(3)/2` — con `HexSize = 100` fa
 * `86.602540...` — e nessun numero di suddivisioni di `HexSize` la rende intera. Il quanto e' invece
 * relativo al punto notevole di ciascuna direzione, e i due estremi cadono sui vertici a meno
 * dell'epsilon di macchina.
 *
 * Se questo test cade, la grammatica ha perso la capacita' di esprimere un muro di stanza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGeometryPerimeterWallTest,
	"RefactorTactics.GeometryGrammar.PerimeterWallReconstructsHexVertices",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGeometryPerimeterWallTest::RunTest(const FString&)
{
	TArray<FVector2D> Boundary;
	URTHexOccupancyLibrary::SectorBoundaryPoints(TestHexSize, Boundary);

	// I due vertici che delimitano il lato `E`: i confini 0 (a -30 gradi) e 2 (a +30).
	const FVector2D ExpectedA = Boundary[0];
	const FVector2D ExpectedB = Boundary[2];

	const FRTOccupancyPolyline Line = URTGeometryGrammarLibrary::ToPolyline(WallOnEastEdge(), TestHexSize);

	TestEqual(TEXT("il muro perimetrale ha due estremi"), Line.Points.Num(), 2);
	if (Line.Points.Num() != 2)
	{
		return false;
	}

	const double ErrorA = FVector2D::Distance(Line.Points[0], ExpectedA);
	const double ErrorB = FVector2D::Distance(Line.Points[1], ExpectedB);

	// Tolleranza da epsilon di macchina, non da approssimazione: se la costruzione fosse sbagliata
	// l'errore sarebbe dell'ordine di `HexSize * (1 - sqrt(3)/2)`, cioe' circa 13 unita' su 100.
	TestTrue(FString::Printf(TEXT("estremo A cade sul vertice a -30 gradi (errore %g)"), ErrorA), ErrorA < 1e-3);
	TestTrue(FString::Printf(TEXT("estremo B cade sul vertice a +30 gradi (errore %g)"), ErrorB), ErrorB < 1e-3);

	return true;
}

/**
 * GLI ASSI NON SONO INCISI: ognuno e' il punto di confine `Axis + 1` di `SectorBoundaryPoints`.
 *
 * E' il DoD *«la direzione si CHIEDE alla libreria, non si scrive a mano»*. Il test confronta la direzione
 * di ogni asse con il punto di confine corrispondente: se qualcuno sostituisse la derivazione con angoli
 * scritti a mano e la convenzione dei sei lati cambiasse, questo test cadrebbe mentre il codice inciso
 * continuerebbe a rispondere numeri plausibili.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGeometryAxesComeFromLibraryTest,
	"RefactorTactics.GeometryGrammar.AxesAreDerivedFromSectorBoundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGeometryAxesComeFromLibraryTest::RunTest(const FString&)
{
	TArray<FVector2D> Boundary;
	URTHexOccupancyLibrary::SectorBoundaryPoints(TestHexSize, Boundary);

	for (int32 Index = 0; Index < RT_TacticalAxisCount; ++Index)
	{
		const ERTTacticalAxis Axis = static_cast<ERTTacticalAxis>(Index);

		const FVector2D FromLibrary = Boundary[Index + 1];
		const FVector2D FromGrammar = URTGeometryGrammarLibrary::AxisPoint(Axis, TestHexSize);
		TestTrue(FString::Printf(TEXT("asse %d deriva dal confine %d"), Index, Index + 1),
			FVector2D::Distance(FromLibrary, FromGrammar) < 1e-6);

		// La perpendicolare e' nove passi da 30 gradi piu' avanti, cioe' -90.
		const FVector2D ExpectedPerp = Boundary[(Index + 1 + 9) % RT_OccupancySectorCount];
		const FVector2D ActualPerp = URTGeometryGrammarLibrary::AxisPerpendicularPoint(Axis, TestHexSize);
		TestTrue(FString::Printf(TEXT("perpendicolare dell'asse %d"), Index),
			FVector2D::Distance(ExpectedPerp, ActualPerp) < 1e-6);

		// E la perpendicolare e' davvero ortogonale: il prodotto scalare delle due direzioni e' nullo.
		const double Dot = FVector2D::DotProduct(FromGrammar.GetSafeNormal(), ActualPerp.GetSafeNormal());
		TestTrue(FString::Printf(TEXT("asse %d e perpendicolare sono ortogonali (dot %g)"), Index, Dot),
			FMath::Abs(Dot) < 1e-6);
	}

	return true;
}

/**
 * LE CINQUE REGOLE, UN TEST CIASCUNA — e la separazione non e' cosmetica.
 *
 * Il DoD di `#620` chiede che, allentata una regola per volta, cada **esattamente** il test che la
 * protegge. Con le cinque regole dentro un test solo la mutazione fa cadere sempre lo stesso nome, e
 * «esattamente» non e' asseribile: e' lo stesso difetto per cui il validator restituisce un reason code
 * enumerato invece di una stringa.
 *
 * Ognuno ha il suo caso ROSSO e il suo gemello VERDE, perche' senza il verde «rifiuta» passerebbe anche
 * con un validator che rifiuta tutto — e il verde e' scelto sul CONFINE della regola, non lontano da esso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGeometryRejectsOffAxisTest,
	"RefactorTactics.GeometryGrammar.RejectsOffAxis",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGeometryRejectsOffAxisTest::RunTest(const FString&)
{
	FRTGeometrySegment S = ValidSegment();
	S.Axis = static_cast<ERTTacticalAxis>(RT_TacticalAxisCount); // il primo valore che non esiste
	TestTrue(TEXT("asse sconosciuto -> OffAxis"),
		URTGeometryGrammarLibrary::ValidateSegment(S) == ERTGeometryViolation::OffAxis);

	S.Axis = ERTTacticalAxis::Deg150; // l'ultimo valido: il confine della regola
	TestTrue(TEXT("l'ultimo asse valido e' accettato"),
		URTGeometryGrammarLibrary::ValidateSegment(S) == ERTGeometryViolation::None);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGeometryRejectsZeroLengthTest,
	"RefactorTactics.GeometryGrammar.RejectsZeroLength",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGeometryRejectsZeroLengthTest::RunTest(const FString&)
{
	FRTGeometrySegment S = ValidSegment();
	S.AlongStart = 5;
	S.AlongEnd = 5;
	TestTrue(TEXT("estremi coincidenti -> ZeroLength"),
		URTGeometryGrammarLibrary::ValidateSegment(S) == ERTGeometryViolation::ZeroLength);

	S.AlongEnd = 6; // un solo quanto di differenza: il confine
	TestTrue(TEXT("un quanto di lunghezza basta"),
		URTGeometryGrammarLibrary::ValidateSegment(S) == ERTGeometryViolation::None);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGeometryRejectsInvalidLayerTest,
	"RefactorTactics.GeometryGrammar.RejectsInvalidLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGeometryRejectsInvalidLayerTest::RunTest(const FString&)
{
	FRTGeometrySegment S = ValidSegment();
	S.Layer = -1;
	TestTrue(TEXT("layer negativo -> InvalidLayer"),
		URTGeometryGrammarLibrary::ValidateSegment(S) == ERTGeometryViolation::InvalidLayer);

	S.Layer = 0; // il confine: i piani partono da zero
	TestTrue(TEXT("layer zero e' valido"),
		URTGeometryGrammarLibrary::ValidateSegment(S) == ERTGeometryViolation::None);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGeometryRejectsOutsideBoundsTest,
	"RefactorTactics.GeometryGrammar.RejectsOutsideEditableBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGeometryRejectsOutsideBoundsTest::RunTest(const FString&)
{
	FRTGeometrySegment S = ValidSegment();
	S.Offset = RT_GeometryMaxQuanta + 1;
	TestTrue(TEXT("offset oltre il limite -> OutsideEditableBounds"),
		URTGeometryGrammarLibrary::ValidateSegment(S) == ERTGeometryViolation::OutsideEditableBounds);

	S.Offset = RT_GeometryMaxQuanta; // esattamente al limite: dentro
	TestTrue(TEXT("offset al limite e' valido"),
		URTGeometryGrammarLibrary::ValidateSegment(S) == ERTGeometryViolation::None);

	// Anche in negativo e su un altro campo: il controllo guarda i tre interi insieme, e provarne uno
	// solo lascerebbe passare un'implementazione che ne dimentica due.
	S = ValidSegment();
	S.AlongEnd = -(RT_GeometryMaxQuanta + 1);
	TestTrue(TEXT("estremo oltre il limite in negativo -> OutsideEditableBounds"),
		URTGeometryGrammarLibrary::ValidateSegment(S) == ERTGeometryViolation::OutsideEditableBounds);

	// ⚠️ IL VALORE PIU' ESTREMO, che e' anche quello che scivola via.
	// `FMath::Abs(MIN_int32)` non e' rappresentabile in `int32` e resta negativo: una validazione scritta
	// sul valore assoluto avrebbe accettato proprio il caso peggiore. E' l'unico intero che una
	// deserializzazione corrotta produce piu' facilmente di un valore a caso, perche' e' un bit pattern
	// intero (`0x80000000`).
	S = ValidSegment();
	S.Offset = MIN_int32;
	TestTrue(TEXT("MIN_int32 -> OutsideEditableBounds, non un overflow silenzioso"),
		URTGeometryGrammarLibrary::ValidateSegment(S) == ERTGeometryViolation::OutsideEditableBounds);

	S = ValidSegment();
	S.AlongStart = MAX_int32;
	TestTrue(TEXT("MAX_int32 -> OutsideEditableBounds"),
		URTGeometryGrammarLibrary::ValidateSegment(S) == ERTGeometryViolation::OutsideEditableBounds);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGeometryRejectsDuplicateTest,
	"RefactorTactics.GeometryGrammar.RejectsDuplicateSegment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGeometryRejectsDuplicateTest::RunTest(const FString&)
{
	using Lib = URTGeometryGrammarLibrary;
	TArray<FRTGeometryIssue> Issues;

	Lib::Validate({ ValidSegment(), ValidSegment() }, Issues);
	TestEqual(TEXT("due segmenti identici -> una violazione"), Issues.Num(), 1);
	if (Issues.Num() == 1)
	{
		TestEqual(TEXT("segnalata sulla SECONDA occorrenza"), Issues[0].SegmentIndex, 1);
		TestTrue(TEXT("ed e' DuplicateSegment"),
			Issues[0].Violation == ERTGeometryViolation::DuplicateSegment);
	}

	// Gemello verde: due segmenti diversi non sono duplicati.
	FRTGeometrySegment Other = ValidSegment();
	Other.Offset = 3;
	Lib::Validate({ ValidSegment(), Other }, Issues);
	TestEqual(TEXT("due segmenti diversi -> nessuna violazione"), Issues.Num(), 0);

	// Lo STESSO segmento percorso al contrario resta un duplicato: gli estremi sono una coppia non
	// ordinata, e senza questo la regola sarebbe aggirabile scambiandoli.
	FRTGeometrySegment Reversed = ValidSegment();
	Swap(Reversed.AlongStart, Reversed.AlongEnd);
	Lib::Validate({ ValidSegment(), Reversed }, Issues);
	TestEqual(TEXT("lo stesso segmento invertito e' un duplicato"), Issues.Num(), 1);

	return true;
}

/** Il VERDE di riferimento, separato: un segmento in grammatica non deve produrre alcuna violazione. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGeometryAcceptsValidSegmentTest,
	"RefactorTactics.GeometryGrammar.AcceptsSegmentInGrammar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGeometryAcceptsValidSegmentTest::RunTest(const FString&)
{
	TestTrue(TEXT("un segmento radiale in grammatica e' accettato"),
		URTGeometryGrammarLibrary::ValidateSegment(ValidSegment()) == ERTGeometryViolation::None);
	TestTrue(TEXT("e anche il muro perimetrale"),
		URTGeometryGrammarLibrary::ValidateSegment(WallOnEastEdge()) == ERTGeometryViolation::None);

	TArray<FRTGeometryIssue> Issues;
	URTGeometryGrammarLibrary::Validate({ ValidSegment(), WallOnEastEdge() }, Issues);
	TestEqual(TEXT("una collezione valida non segnala nulla"), Issues.Num(), 0);

	return true;
}

/**
 * ORACOLO DEL DETERMINISMO, meta' 1: il round-trip di serializzazione non perde nulla.
 *
 * ⚠️ **La fixture e' sintetica su OGNI campo, e non e' pignoleria**: `ExportText` confrontato con il
 * default del tipo omette i campi che gli sono uguali, quindi un segmento lasciato ai valori di default
 * verrebbe esportato quasi vuoto, reimportato identico, e il test passerebbe **senza aver trasportato
 * nulla**. Nessun confronto finale se ne accorgerebbe. Qui ogni campo differisce dal proprio default.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGeometryRoundTripTest,
	"RefactorTactics.GeometryGrammar.SerializationRoundTripPreservesEveryField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGeometryRoundTripTest::RunTest(const FString&)
{
	FRTGeometrySegment Original;
	Original.Axis = ERTTacticalAxis::Deg120; // != Deg0
	Original.Offset = 7;                     // != 0
	Original.AlongStart = -5;                // != 0
	Original.AlongEnd = 9;                   // != 0
	Original.Layer = 2;                      // != 0

	FString Text;
	FRTGeometrySegment::StaticStruct()->ExportText(Text, &Original, nullptr, nullptr, PPF_None, nullptr);

	// Se il testo esportato non nomina i campi, il round-trip che segue non prova nulla.
	TestTrue(TEXT("l'export nomina l'asse"), Text.Contains(TEXT("Axis")));
	TestTrue(TEXT("l'export nomina l'offset"), Text.Contains(TEXT("Offset")));
	TestTrue(TEXT("l'export nomina il layer"), Text.Contains(TEXT("Layer")));

	FRTGeometrySegment Reloaded;
	FRTGeometrySegment::StaticStruct()->ImportText(*Text, &Reloaded, nullptr, PPF_None, nullptr, TEXT("FRTGeometrySegment"));

	TestTrue(TEXT("l'asse sopravvive"), Reloaded.Axis == Original.Axis);
	TestEqual(TEXT("l'offset sopravvive"), Reloaded.Offset, Original.Offset);
	TestEqual(TEXT("il primo estremo sopravvive"), Reloaded.AlongStart, Original.AlongStart);
	TestEqual(TEXT("il secondo estremo sopravvive"), Reloaded.AlongEnd, Original.AlongEnd);
	TestEqual(TEXT("il layer sopravvive"), Reloaded.Layer, Original.Layer);

	// E la geometria derivata coincide: il trasporto ha preservato cio' che conta a valle.
	const FRTOccupancyPolyline FromOriginal = URTGeometryGrammarLibrary::ToPolyline(Original, TestHexSize);
	const FRTOccupancyPolyline FromReloaded = URTGeometryGrammarLibrary::ToPolyline(Reloaded, TestHexSize);
	TestEqual(TEXT("stessa polilinea derivata"), FromOriginal.Points.Num(), FromReloaded.Points.Num());
	for (int32 i = 0; i < FromOriginal.Points.Num() && i < FromReloaded.Points.Num(); ++i)
	{
		TestTrue(TEXT("stesso punto"), FVector2D::Distance(FromOriginal.Points[i], FromReloaded.Points[i]) < 1e-9);
	}

	return true;
}

/**
 * ORACOLO DEL DETERMINISMO, meta' 2: l'esito non dipende dall'ordine in cui i segmenti arrivano.
 *
 * E' il gemello di `HexOccupancy.MaskIsIndependentOfInputOrder` sul lato grammatica. La maschera che
 * `ComputeMask` produce dalla stessa collezione permutata deve essere identica, e le violazioni devono
 * essere le stesse — cambia solo l'indice a cui sono attaccate.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGeometryOrderIndependenceTest,
	"RefactorTactics.GeometryGrammar.ValidationAndMaskAreIndependentOfInputOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGeometryOrderIndependenceTest::RunTest(const FString&)
{
	FRTGeometrySegment A = ValidSegment();
	FRTGeometrySegment B = WallOnEastEdge();
	FRTGeometrySegment C = ValidSegment();
	C.Axis = ERTTacticalAxis::Deg60;
	C.Offset = -4;

	const TArray<FRTGeometrySegment> Forward{ A, B, C };
	const TArray<FRTGeometrySegment> Backward{ C, B, A };

	TArray<FRTGeometryIssue> IssuesForward, IssuesBackward;
	URTGeometryGrammarLibrary::Validate(Forward, IssuesForward);
	URTGeometryGrammarLibrary::Validate(Backward, IssuesBackward);
	TestEqual(TEXT("nessuna violazione in avanti"), IssuesForward.Num(), 0);
	TestEqual(TEXT("nessuna violazione all'indietro"), IssuesBackward.Num(), 0);

	TArray<FRTOccupancyPolyline> GeomForward, GeomBackward;
	for (const FRTGeometrySegment& S : Forward)
	{
		GeomForward.Add(URTGeometryGrammarLibrary::ToPolyline(S, TestHexSize));
	}
	for (const FRTGeometrySegment& S : Backward)
	{
		GeomBackward.Add(URTGeometryGrammarLibrary::ToPolyline(S, TestHexSize));
	}

	const FRTOccupancyMask MaskForward = URTHexOccupancyLibrary::ComputeMask(GeomForward, TestHexSize);
	const FRTOccupancyMask MaskBackward = URTHexOccupancyLibrary::ComputeMask(GeomBackward, TestHexSize);

	TestEqual(TEXT("stessa maschera comunque ordinati"), MaskForward.Sectors, MaskBackward.Sectors);
	TestTrue(TEXT("stesso CoreBlocked"), MaskForward.bCoreBlocked == MaskBackward.bCoreBlocked);

	// E la maschera non e' vuota: un test di invarianza su due zeri passerebbe con qualunque implementazione.
	TestTrue(TEXT("la geometria occupa davvero dei settori"),
		URTHexOccupancyLibrary::NumOccupiedSectors(MaskForward) > 0);

	return true;
}

/**
 * IL CONFINE FRA AUTHORITY E CALCOLO: un segmento non valido non produce coordinate.
 *
 * Senza questo, `ToPolyline` su un asse inesistente restituirebbe punti calcolati da una direzione nulla —
 * cioe' un segmento degenere nel centro della cella, che `ComputeMask` misurerebbe come geometria vera.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGeometryInvalidProducesNoGeometryTest,
	"RefactorTactics.GeometryGrammar.InvalidSegmentProducesNoGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGeometryInvalidProducesNoGeometryTest::RunTest(const FString&)
{
	FRTGeometrySegment Invalid = ValidSegment();
	Invalid.Axis = static_cast<ERTTacticalAxis>(RT_TacticalAxisCount);

	const FRTOccupancyPolyline Line = URTGeometryGrammarLibrary::ToPolyline(Invalid, TestHexSize);
	TestEqual(TEXT("un segmento fuori grammatica non produce punti"), Line.Points.Num(), 0);

	const FRTOccupancyMask Mask = URTHexOccupancyLibrary::ComputeMask({ Line }, TestHexSize);
	TestEqual(TEXT("e non occupa alcun settore"), URTHexOccupancyLibrary::NumOccupiedSectors(Mask), 0);
	TestTrue(TEXT("ne' il centro"), !Mask.bCoreBlocked);

	// Controprova: lo stesso segmento con un asse valido produce geometria. Senza, questo test passerebbe
	// anche con un `ToPolyline` che non restituisce mai nulla.
	const FRTOccupancyPolyline Valid = URTGeometryGrammarLibrary::ToPolyline(ValidSegment(), TestHexSize);
	TestEqual(TEXT("il gemello valido produce due punti"), Valid.Points.Num(), 2);

	return true;
}

/**
 * LO SNAP, ed è la parte del gesto d'authoring che si può verificare headless (`#712`).
 *
 * Un gesto è approssimativo per definizione: il mouse non cade sui quanti. Il test parte da un segmento
 * **legale**, lo sporca di un rumore più piccolo di mezzo quanto, e pretende che lo snap ritrovi
 * esattamente l'originale. È l'unica proprietà che conta: due gesti vicini devono produrre lo stesso dato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGeometrySnapRecoversTest,
	"RefactorTactics.GeometryGrammar.SnapRecoversTheIntendedSegment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGeometrySnapRecoversTest::RunTest(const FString&)
{
	const FRTGeometrySegment Intended = WallOnEastEdge();
	const FRTOccupancyPolyline Exact = URTGeometryGrammarLibrary::ToPolyline(Intended, TestHexSize);

	// Rumore deliberatamente asimmetrico e sotto il mezzo quanto: un gesto umano, non un valore esatto.
	const double Quantum = TestHexSize / RT_GeometryQuanta;
	const FVector2D NoisyA = Exact.Points[0] + FVector2D(Quantum * 0.30, -Quantum * 0.20);
	const FVector2D NoisyB = Exact.Points[1] + FVector2D(-Quantum * 0.15, Quantum * 0.35);

	FRTGeometrySegment Snapped;
	const bool bOk = URTGeometryGrammarLibrary::SnapToGrammar(NoisyA, NoisyB, TestHexSize, Snapped);

	TestTrue(TEXT("un gesto vicino a un segmento legale produce un segmento"), bOk);
	if (bOk)
	{
		TestTrue(TEXT("stesso asse"), Snapped.Axis == Intended.Axis);
		TestEqual(TEXT("stesso offset"), Snapped.Offset, Intended.Offset);
		TestEqual(TEXT("stesso primo estremo"), Snapped.AlongStart, Intended.AlongStart);
		TestEqual(TEXT("stesso secondo estremo"), Snapped.AlongEnd, Intended.AlongEnd);
	}

	// E ciò che esce è SEMPRE legale: è la garanzia che il tool non può committare un segmento fuori
	// grammatica, qualunque cosa faccia il mouse.
	TestTrue(TEXT("lo snap produce solo segmenti in grammatica"),
		URTGeometryGrammarLibrary::ValidateSegment(Snapped) == ERTGeometryViolation::None);

	return true;
}

/**
 * IL GHOST INVALIDO: un gesto che non può produrre un segmento legale deve **fallire**, non ripiegare su
 * qualcosa di plausibile.
 *
 * È la metà che rende onesto il ghost «invalido» del tool: se lo snap ripiegasse sempre su un segmento,
 * l'autore non vedrebbe mai un rifiuto e la grammatica diventerebbe una decorazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGeometrySnapRejectsTest,
	"RefactorTactics.GeometryGrammar.SnapRejectsWhatCannotBeLegal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGeometrySnapRejectsTest::RunTest(const FString&)
{
	FRTGeometrySegment Out;

	// Due punti coincidenti: qualunque asse produrrebbe lunghezza zero.
	const FVector2D P(12.0, 7.0);
	TestTrue(TEXT("un gesto senza lunghezza non produce un segmento"),
		!URTGeometryGrammarLibrary::SnapToGrammar(P, P, TestHexSize, Out));

	// Un gesto lontanissimo: oltre i bordi editabili su entrambi gli estremi.
	const double Far = TestHexSize * (RT_GeometryMaxQuanta / RT_GeometryQuanta) * 10.0;
	TestTrue(TEXT("un gesto fuori dai bordi editabili non produce un segmento"),
		!URTGeometryGrammarLibrary::SnapToGrammar(FVector2D(Far, Far), FVector2D(Far * 1.1, Far), TestHexSize, Out));

	// Controprova: un gesto normale invece funziona, altrimenti questo test passerebbe con uno snap
	// che non produce mai nulla.
	const FRTOccupancyPolyline Exact = URTGeometryGrammarLibrary::ToPolyline(ValidSegment(), TestHexSize);
	TestTrue(TEXT("ma un gesto normale sì"),
		URTGeometryGrammarLibrary::SnapToGrammar(Exact.Points[0], Exact.Points[1], TestHexSize, Out));

	return true;
}

/**
 * #712 / seduta `U22`: LO SNAP SI AGGANCIA AI PUNTI NOTEVOLI, e la mano ha spazio per sbagliare.
 *
 * 🔴 Prima la tolleranza era **mezzo quanto di `Offset`**: 3,61 uu su una cella da 100, cioe' il 3,6% del
 * raggio — pochi pixel in viewport. Misurato allora: al 4% di scarto comparivano i primi muri sul lato
 * sbagliato, all'8% un gesto su tre non produceva niente, al 15% ne falliva il 61%. E il ghost restava
 * VERDE, perche' il candidato era legale: era solo un altro.
 *
 * ⚠️ Il test misura il BACINO DI CATTURA, non una formula: perturba i due estremi in modo indipendente e
 * pretende che il segmento resti quello giusto. E' l'unico modo di asserire «la mano ha spazio» — un
 * `TestEqual` sull'implementazione dello snap direbbe solo che lo snap fa cio' che fa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSnapCatchesAWobblyHandTest,
	"RefactorTactics.Geometry.SnapCatchesAWobblyHand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSnapCatchesAWobblyHandTest::RunTest(const FString&)
{
	TArray<FVector2D> Boundary;
	URTHexOccupancyLibrary::SectorBoundaryPoints(TestHexSize, Boundary);
	if (!TestEqual(TEXT("dodici punti di confine"), Boundary.Num(), RT_OccupancySectorCount))
	{
		return false;
	}

	// Lo scarto della mano che il test pretende di reggere. 15% del raggio e' il valore su cui la
	// quantizzazione fine falliva il 61% dei gesti.
	const double Wobble = static_cast<double>(TestHexSize) * 0.15;
	const TArray<FVector2D> Nudges = {
		{ +1, 0 }, { -1, 0 }, { 0, +1 }, { 0, -1 }, { +1, +1 }, { -1, -1 }, { +1, -1 }, { -1, +1 }
	};

	int32 Checked = 0;
	for (int32 Edge = 0; Edge < 6; ++Edge)
	{
		// Il lato `Edge`, dai suoi due vertici: il gesto che l'autore fa per murare un lato.
		const FVector2D V0 = Boundary[(2 * Edge) % RT_OccupancySectorCount];
		const FVector2D V1 = Boundary[(2 * Edge + 2) % RT_OccupancySectorCount];

		FRTGeometrySegment Exact;
		if (!TestTrue(TEXT("il gesto preciso si aggancia"),
			URTGeometryGrammarLibrary::SnapToGrammar(V0, V1, TestHexSize, Exact)))
		{
			continue;
		}

		for (const FVector2D& NudgeA : Nudges)
		{
			for (const FVector2D& NudgeB : Nudges)
			{
				FRTGeometrySegment Wobbly;
				const bool bSnapped = URTGeometryGrammarLibrary::SnapToGrammar(
					V0 + NudgeA * Wobble, V1 + NudgeB * Wobble, TestHexSize, Wobbly);

				++Checked;
				TestTrue(FString::Printf(TEXT("lato %d: il gesto storto si aggancia comunque"), Edge), bSnapped);
				if (!bSnapped)
				{
					continue;
				}

				// La riga che il difetto faceva fallire: lo stesso muro, non un altro.
				const bool bSame = Wobbly.Axis == Exact.Axis
					&& Wobbly.Offset == Exact.Offset
					&& FMath::Min(Wobbly.AlongStart, Wobbly.AlongEnd) == FMath::Min(Exact.AlongStart, Exact.AlongEnd)
					&& FMath::Max(Wobbly.AlongStart, Wobbly.AlongEnd) == FMath::Max(Exact.AlongStart, Exact.AlongEnd);
				TestTrue(FString::Printf(
					TEXT("lato %d: uno scarto del 15%% del raggio da' ancora QUEL muro"), Edge), bSame);
			}
		}
	}

	TestEqual(TEXT("il test ha davvero provato 6 lati x 64 perturbazioni"), Checked, 6 * 64);
	return true;
}

/**
 * #712 / seduta `U22`: OGNI muro prodotto sta nell'alfabeto. Non «quasi ogni».
 *
 * 🔴 L'autore, dopo due correzioni: *«son capitati muri fuori dalla geometria consentita»*. Le stesure
 * precedenti arrotondavano le coordinate del gesto e speravano che cadessero bene: bastava un estremo
 * fuori dalla cella perche' l'altro finisse su un `Along` legale ma che non corrisponde a nessun punto
 * notevole — un muro a mezz'aria.
 *
 * La ricerca ora **enumera le coppie di punti notevoli**, quindi l'alfabeto e' una proprieta' della
 * costruzione. Questo test lo verifica dove conta: su gesti QUALSIASI, anche storti, corti, obliqui e
 * mezzi fuori dalla cella — e pretende che l'esito sia sempre due punti notevoli su un asse.
 *
 * ⚠️ Sostituisce un test che pretendeva il RIFIUTO delle 24 coppie fuori asse. Il rewrite ha cambiato
 * quella decisione: invece di rifiutare, si sceglie il muro legale piu' vicino. E' una scelta diversa e
 * va detta — il rifiuto resta per il gesto senza lunghezza e per quello che non tocca la cella
 * (`SnapRejectsWhatCannotBeLegal`), non per la giacitura.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSnapAlwaysProducesAWallFromTheAlphabetTest,
	"RefactorTactics.Geometry.SnapAlwaysProducesAWallFromTheAlphabet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSnapAlwaysProducesAWallFromTheAlphabetTest::RunTest(const FString&)
{
	TArray<FVector2D> Boundary;
	URTHexOccupancyLibrary::SectorBoundaryPoints(TestHexSize, Boundary);

	TArray<FVector2D> Notable;
	Notable.Add(FVector2D::ZeroVector); // il centro e' il tredicesimo
	Notable.Append(Boundary);

	auto IsNotable = [&Notable](const FVector2D& P)
	{
		for (const FVector2D& Point : Notable)
		{
			if (Point.Equals(P, 0.5f))
			{
				return true;
			}
		}
		return false;
	};

	// Una griglia deterministica di gesti: raggi da 0 a due volte la cella, angoli ogni 17 gradi (primo
	// con 360, quindi non ricade sulle direttrici). Non e' casuale — l'esito dev'essere riproducibile.
	int32 Produced = 0;
	int32 Refused = 0;
	for (int32 AngleA = 0; AngleA < 360; AngleA += 17)
	{
		for (int32 AngleB = 0; AngleB < 360; AngleB += 43)
		{
			for (const double RadiusA : { 0.15, 0.55, 0.95, 1.40 })
			{
				for (const double RadiusB : { 0.30, 0.80, 1.10, 2.00 })
				{
					const double Ra = FMath::DegreesToRadians(static_cast<double>(AngleA));
					const double Rb = FMath::DegreesToRadians(static_cast<double>(AngleB));
					const FVector2D A(
						RadiusA * TestHexSize * FMath::Cos(Ra), RadiusA * TestHexSize * FMath::Sin(Ra));
					const FVector2D B(
						RadiusB * TestHexSize * FMath::Cos(Rb), RadiusB * TestHexSize * FMath::Sin(Rb));

					FRTGeometrySegment Segment;
					if (!URTGeometryGrammarLibrary::SnapToGrammar(A, B, TestHexSize, Segment))
					{
						++Refused;
						continue;
					}
					++Produced;

					// 1. E' in grammatica. La rete di sicurezza che c'era gia'.
					if (!TestTrue(TEXT("il segmento prodotto e' in grammatica"),
						URTGeometryGrammarLibrary::ValidateSegment(Segment) == ERTGeometryViolation::None))
					{
						return false;
					}

					// 2. E i suoi DUE ESTREMI sono punti notevoli. E' la riga che il difetto faceva fallire,
					//    ed e' l'unica formulazione di «dentro la geometria consentita» che si possa misurare.
					const FRTOccupancyPolyline Line =
						URTGeometryGrammarLibrary::ToPolyline(Segment, TestHexSize);
					if (!TestEqual(TEXT("il segmento ha due estremi"), Line.Points.Num(), 2))
					{
						return false;
					}
					if (!TestTrue(FString::Printf(
							TEXT("estremi su punti notevoli: (%.1f,%.1f)-(%.1f,%.1f) da gesto (%.0f,%.0f)-(%.0f,%.0f)"),
							Line.Points[0].X, Line.Points[0].Y, Line.Points[1].X, Line.Points[1].Y,
							A.X, A.Y, B.X, B.Y),
						IsNotable(Line.Points[0]) && IsNotable(Line.Points[1])))
					{
						return false;
					}
				}
			}
		}
	}

	// Controprova: il test non passerebbe con uno snap che non produce mai niente.
	TestTrue(TEXT("la maggior parte dei gesti produce un muro"), Produced > Refused * 4);
	TestTrue(TEXT("e qualcuno viene rifiutato, altrimenti il rifiuto e' morto"), Refused > 0);
	return true;
}

/**
 * #712 / seduta `U22`: quante coppie di punti notevoli la GRAMMATICA sa esprimere.
 *
 * ⚠️ E' una proprieta' degli assi tattici, non dello snap, e per questo si misura senza passare da
 * `SnapToGrammar`: delle 78 coppie, quelle che stanno su un asse comune sono **54**, e le altre **24**
 * sono vertice ↔ punto medio non adiacente. I due numeri sono scritti perche' un cambio di grammatica li
 * faccia cadere qui, e non a schermo tre giorni dopo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNotablePairsOnTacticalAxesTest,
	"RefactorTactics.Geometry.NotablePairsOnTacticalAxes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNotablePairsOnTacticalAxesTest::RunTest(const FString&)
{
	TArray<FVector2D> Boundary;
	URTHexOccupancyLibrary::SectorBoundaryPoints(TestHexSize, Boundary);

	TArray<FVector2D> Notable;
	Notable.Add(FVector2D::ZeroVector);
	Notable.Append(Boundary);

	auto SharesAnAxis = [](const FVector2D& P, const FVector2D& Q)
	{
		for (int32 AxisIndex = 0; AxisIndex < RT_TacticalAxisCount; ++AxisIndex)
		{
			const ERTTacticalAxis Axis = static_cast<ERTTacticalAxis>(AxisIndex);
			const FVector2D Along = URTGeometryGrammarLibrary::AxisPoint(Axis, TestHexSize) / RT_GeometryQuanta;
			const FVector2D Perp =
				URTGeometryGrammarLibrary::AxisPerpendicularPoint(Axis, TestHexSize) / RT_GeometryQuanta;

			const double AlongP = FVector2D::DotProduct(P, Along) / Along.SizeSquared();
			const double AlongQ = FVector2D::DotProduct(Q, Along) / Along.SizeSquared();
			const double OffP = FVector2D::DotProduct(P, Perp) / Perp.SizeSquared();
			const double OffQ = FVector2D::DotProduct(Q, Perp) / Perp.SizeSquared();

			auto Whole = [](double V) { return FMath::Abs(V - FMath::RoundToDouble(V)) < 0.001; };
			if (Whole(AlongP) && Whole(AlongQ) && Whole(OffP) && Whole(OffQ)
				&& FMath::RoundToInt(OffP) == FMath::RoundToInt(OffQ)
				&& FMath::RoundToInt(AlongP) != FMath::RoundToInt(AlongQ))
			{
				return true;
			}
		}
		return false;
	};

	int32 OnAxis = 0;
	int32 OffAxis = 0;
	for (int32 I = 0; I < Notable.Num(); ++I)
	{
		for (int32 J = I + 1; J < Notable.Num(); ++J)
		{
			SharesAnAxis(Notable[I], Notable[J]) ? ++OnAxis : ++OffAxis;
		}
	}

	TestEqual(TEXT("coppie totali di punti notevoli"), OnAxis + OffAxis, 78);
	TestEqual(TEXT("coppie esprimibili su un asse tattico"), OnAxis, 54);
	TestEqual(TEXT("coppie che nessun asse porta"), OffAxis, 24);
	return true;
}

/**
 * #712 / seduta `U22`: TRASCINANDO LONTANO, IL MURO PARTE ANCORA DA DOVE HAI PREMUTO.
 *
 * 🔴 Non lo faceva. L'autore: *«continua a disegnare al centro degli esagoni e la linea non parte dal
 * vertice iniziale, quando lo sposto lontano»*. Due difetti nella stessa riga, entrambi miei:
 *
 * ```text
 * 1. l'aggancio dei due estremi era legato insieme  ->  se il secondo usciva dalla cella,
 *                                                       il PRIMO perdeva il suo punto notevole
 * 2. `Offset` era sempre la media dei due           ->  con un estremo lontano la media
 *                                                       collassava a zero, cioe' al CENTRO
 * ```
 *
 * Sono indipendenti, e per questo il test verifica **entrambe** le conseguenze: che il muro parta dal
 * punto premuto, e che non finisca in mezzo alla cella.
 *
 * ⚠️ Il gesto di prova esce DAVVERO dalla cella (tre raggi), perche' e' li' che l'aggancio si spegneva.
 * Un test con l'estremo appena fuori dal bordo non avrebbe visto niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSnapKeepsTheAnchorWhenDraggingFarTest,
	"RefactorTactics.Geometry.SnapKeepsTheAnchorWhenDraggingFar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSnapKeepsTheAnchorWhenDraggingFarTest::RunTest(const FString&)
{
	TArray<FVector2D> Boundary;
	URTHexOccupancyLibrary::SectorBoundaryPoints(TestHexSize, Boundary);
	if (!TestEqual(TEXT("dodici punti di confine"), Boundary.Num(), RT_OccupancySectorCount))
	{
		return false;
	}

	// ⚠️ Il trascinamento NON deve essere radiale, e la prima stesura di questo test lo era: partendo da un
	// punto e andando verso l'esterno lungo la sua stessa direzione si ottiene una retta che passa per il
	// CENTRO — ed e' geometricamente giusto, non il difetto. Un gesto simile passava anche col codice
	// rotto, perche' i due estremi hanno lo stesso offset e la media non sposta niente. Serve una direzione
	// obliqua, che e' poi cio' che fa una mano: si preme su un vertice e si trascina altrove.
	const TArray<double> ObliqueDegrees = { 20.0, -35.0, 70.0 };

	int32 Checked = 0;
	// ⚠️ E la pressione NON deve cadere esattamente sul punto notevole. La seconda stesura di questo test lo
	// faceva, e la verifica di mutazione l'ha smontata: legando di nuovo insieme l'aggancio dei due estremi
	// — il primo dei due difetti — **nessun test cadeva**, perche' su un'ancora esatta il punto agganciato
	// e quello grezzo coincidono. Un gesto vero cade *vicino* al vertice, mai sopra.
	const double AnchorWobble = static_cast<double>(TestHexSize) * 0.10;

	for (int32 Index = 0; Index < Boundary.Num(); ++Index)
	{
		// Si preme su un punto notevole — vertice o punto medio, entrambi valgono come ancora.
		const FVector2D Anchor = Boundary[Index];

		for (const double Oblique : ObliqueDegrees)
		{
			// La pressione: vicino all'ancora, non sopra. Direzione dello scarto legata all'indice, cosi'
			// l'esito resta deterministico e i dodici punti non ricevono tutti la stessa spinta.
			const double WobbleAngle = FMath::DegreesToRadians(37.0 * Index);
			const FVector2D Pressed(
				Anchor.X + AnchorWobble * FMath::Cos(WobbleAngle),
				Anchor.Y + AnchorWobble * FMath::Sin(WobbleAngle));

			// E si trascina LONTANO e di traverso: tre raggi, fuori dalla cella, dove l'aggancio del
			// secondo estremo si spegne per costruzione.
			const double Base = FMath::Atan2(Anchor.Y, Anchor.X) + FMath::DegreesToRadians(Oblique);
			const double Reach = static_cast<double>(TestHexSize) * 3.0;
			const FVector2D Far(Reach * FMath::Cos(Base), Reach * FMath::Sin(Base));

			FRTGeometrySegment Segment;
			if (!TestTrue(FString::Printf(TEXT("punto %d a %.0f gradi: il gesto lungo si aggancia"),
				Index, Oblique), URTGeometryGrammarLibrary::SnapToGrammar(Pressed, Far, TestHexSize, Segment)))
			{
				continue;
			}

			const FRTOccupancyPolyline Line = URTGeometryGrammarLibrary::ToPolyline(Segment, TestHexSize);
			if (!TestEqual(FString::Printf(TEXT("punto %d a %.0f gradi: due estremi"), Index, Oblique),
				Line.Points.Num(), 2))
			{
				continue;
			}

			++Checked;

			// La riga che il difetto faceva fallire: il muro parte DAL PUNTO PREMUTO, non da dove la media
			// fra i due offset lo faceva scivolare.
			TestTrue(FString::Printf(TEXT("punto %d a %.0f gradi: il muro parte dall'ancora (%.1f,%.1f invece di %.1f,%.1f)"),
				Index, Oblique, Line.Points[0].X, Line.Points[0].Y, Anchor.X, Anchor.Y),
				Line.Points[0].Equals(Anchor, 0.5f));
		}
	}

	TestEqual(TEXT("provati dodici punti per tre direzioni oblique"),
		Checked, RT_OccupancySectorCount * ObliqueDegrees.Num());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
