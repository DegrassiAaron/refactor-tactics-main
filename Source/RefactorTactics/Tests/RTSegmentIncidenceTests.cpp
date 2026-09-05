#include "Misc/AutomationTest.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTGeometryBake.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Prefisso `Incidence`: la unity build fonde i namespace anonimi, e un nome generico collide con
	// l'omonimo di un altro file di test (`#1530`).
	constexpr float IncidenceHexSize = 100.0f;

	/** Il diametro `E`–`W`: giacitura a 0 gradi, per il centro, da un punto medio all'altro. */
	FRTGeometrySegment IncidenceHorizontalDiameter()
	{
		FRTGeometrySegment S;
		S.Axis = ERTTacticalAxis::Deg0;
		S.Offset = 0;
		S.AlongStart = -RT_GeometryQuanta;
		S.AlongEnd = RT_GeometryQuanta;
		S.Layer = 0;
		return S;
	}

	/** Il diametro verticale: giacitura a 90 gradi, per il centro, da un vertice all'altro. */
	FRTGeometrySegment IncidenceVerticalDiameter()
	{
		FRTGeometrySegment S;
		S.Axis = ERTTacticalAxis::Deg90;
		S.Offset = 0;
		S.AlongStart = -RT_GeometryQuanta;
		S.AlongEnd = RT_GeometryQuanta;
		S.Layer = 0;
		return S;
	}

	/** Quante volte una violazione compare fra le issue di una collezione. */
	int32 IncidenceCount(const TArray<FRTGeometrySegment>& Segments, ERTGeometryViolation Wanted)
	{
		TArray<FRTGeometryIssue> Issues;
		URTGeometryGrammarLibrary::Validate(Segments, Issues);
		int32 N = 0;
		for (const FRTGeometryIssue& Issue : Issues)
		{
			if (Issue.Violation == Wanted)
			{
				++N;
			}
		}
		return N;
	}
}

/**
 * DUE SEGMENTI SI INCONTRANO SOLO SU UN ANCHOR — `#1894`, e il caso legale viene prima.
 *
 * ⚠️ **L'ordine dei due casi non e' estetico.** Un validator che segnalasse OGNI incrocio passerebbe il
 * caso invalido e romperebbe il caso normale: due muri che si incrociano al centro della cella sono la
 * configurazione piu' comune che esista, e il documento del 2026-08-31 la dichiara valida. Il test la mette
 * per prima perche' e' quella che una regola scritta male sacrifica.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIncidenceCrossingOnAnchorIsLegalTest,
	"RefactorTactics.Incidence.CrossingOnAnchorIsLegal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIncidenceCrossingOnAnchorIsLegalTest::RunTest(const FString&)
{
	// I due diametri si incrociano nel CENTRO, che e' il tredicesimo anchor.
	const TArray<FRTGeometrySegment> Crossing = { IncidenceHorizontalDiameter(), IncidenceVerticalDiameter() };

	TArray<FRTGeometryIssue> Issues;
	URTGeometryGrammarLibrary::Validate(Crossing, Issues);
	TestEqual(TEXT("due diametri che si incrociano al centro non sono un difetto"), Issues.Num(), 0);

	// E nemmeno due segmenti che si toccano su un vertice invece che al centro: meta' diametro verticale
	// verso l'alto, e meta' diametro orizzontale — si incontrano nel centro, che resta un anchor.
	FRTGeometrySegment HalfUp = IncidenceVerticalDiameter();
	HalfUp.AlongStart = 0;
	FRTGeometrySegment HalfRight = IncidenceHorizontalDiameter();
	HalfRight.AlongStart = 0;

	TArray<FRTGeometryIssue> Corner;
	URTGeometryGrammarLibrary::Validate({ HalfUp, HalfRight }, Corner);
	TestEqual(TEXT("due mezzi diametri che condividono il centro non sono un difetto"), Corner.Num(), 0);

	// Due segmenti che non si incontrano affatto: giaciture diverse, ma tratti disgiunti.
	FRTGeometrySegment FarRight = IncidenceHorizontalDiameter();
	FarRight.AlongStart = RT_GeometryQuanta / 2;
	FarRight.AlongEnd = RT_GeometryQuanta;
	FRTGeometrySegment FarUp = IncidenceVerticalDiameter();
	FarUp.AlongStart = RT_GeometryQuanta / 2;
	FarUp.AlongEnd = RT_GeometryQuanta;

	TArray<FRTGeometryIssue> Disjoint;
	URTGeometryGrammarLibrary::Validate({ FarRight, FarUp }, Disjoint);
	TestEqual(TEXT("due segmenti che non si incontrano non sono un difetto"), Disjoint.Num(), 0);

	return true;
}

/**
 * L'INCROCIO FUORI-ANCHOR E' INVALIDO — la regola vera e propria.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIncidenceCrossingOffAnchorIsReportedTest,
	"RefactorTactics.Incidence.CrossingOffAnchorIsReported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIncidenceCrossingOffAnchorIsReportedTest::RunTest(const FString&)
{
	// Il diametro orizzontale, e un verticale SPOSTATO di mezzo punto notevole: si incrociano a meta'
	// strada fra il centro e il punto medio del lato `E`, dove non c'e' nessun anchor.
	FRTGeometrySegment Shifted = IncidenceVerticalDiameter();
	Shifted.Offset = RT_GeometryQuanta / 2;

	const TArray<FRTGeometrySegment> Segments = { IncidenceHorizontalDiameter(), Shifted };

	TArray<FRTGeometryIssue> Issues;
	URTGeometryGrammarLibrary::Validate(Segments, Issues);

	TestEqual(TEXT("un incrocio fuori-anchor e' segnalato una volta"),
		IncidenceCount(Segments, ERTGeometryViolation::CrossingOffAnchor), 1);

	// La segnalazione dice ENTRAMBI i segmenti: senza, chi disegna sa che qualcosa si incrocia e non con che cosa.
	bool bNamesBoth = false;
	for (const FRTGeometryIssue& Issue : Issues)
	{
		if (Issue.Violation == ERTGeometryViolation::CrossingOffAnchor)
		{
			bNamesBoth = Issue.OtherIndex != INDEX_NONE && Issue.OtherIndex != Issue.SegmentIndex;
		}
	}
	TestTrue(TEXT("la segnalazione nomina l'altro segmento"), bNamesBoth);

	// ⚠️ E non e' un effetto dell'ordine: scambiati, l'esito e' lo stesso. Un validator che guardasse solo
	// «indietro» segnalerebbe comunque, ma uno che guarda un verso solo su una regola simmetrica e' il
	// difetto che si vede a mappa grande e non su due segmenti.
	const TArray<FRTGeometrySegment> Swapped = { Shifted, IncidenceHorizontalDiameter() };
	TestEqual(TEXT("scambiati, sempre una segnalazione"),
		IncidenceCount(Swapped, ERTGeometryViolation::CrossingOffAnchor), 1);

	return true;
}

/**
 * UN SEGMENTO LUNGO CHE ATTRAVERSA UN ANCHOR NON VA SPEZZATO — `GEO-6`.
 *
 * 🔑 E' l'invariante che questa issue deve **non** violare: il documento del 2026-08-31 chiedeva lo split
 * esplicito, `GEO-6` lo ha escluso da v0.1, e il modo in cui una regola d'incidenza scritta male lo
 * reintroduce dalla porta di servizio e' segnalare un segmento perche' *«passa per un anchor dove un altro
 * finisce»*. Qui si dichiara che non lo fa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIncidenceLongSegmentIsNotSplitTest,
	"RefactorTactics.Incidence.LongSegmentThroughAnchorIsNotSplit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIncidenceLongSegmentIsNotSplitTest::RunTest(const FString&)
{
	// Un diametro intero, e un secondo segmento che TERMINA sul centro: e' la T che tocca su un anchor.
	FRTGeometrySegment Stem = IncidenceVerticalDiameter();
	Stem.AlongStart = 0;
	Stem.AlongEnd = RT_GeometryQuanta;

	const TArray<FRTGeometrySegment> Tee = { IncidenceHorizontalDiameter(), Stem };

	TArray<FRTGeometryIssue> Issues;
	URTGeometryGrammarLibrary::Validate(Tee, Issues);
	TestEqual(TEXT("una T che tocca su un anchor e' legale, e il diametro non si spezza"), Issues.Num(), 0);

	// E il diametro da solo, che attraversa il centro senza che nulla vi si innesti, non e' un difetto.
	TArray<FRTGeometryIssue> Alone;
	URTGeometryGrammarLibrary::Validate({ IncidenceHorizontalDiameter() }, Alone);
	TestEqual(TEXT("un segmento che attraversa il centro da solo e' legale"), Alone.Num(), 0);

	return true;
}

/**
 * LA SOVRAPPOSIZIONE PARZIALE — e il caso legale che le somiglia di piu'.
 *
 * ⚠️ **Toccarsi in un estremo non e' sovrapporsi**: due muri consecutivi sulla stessa giacitura sono la
 * continuita' che questa grammatica esprime — un muro lungo disegnato in due gesti — e una regola che li
 * segnalasse renderebbe invalido il modo normale di disegnare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIncidenceOverlapIsReportedTest,
	"RefactorTactics.Incidence.OverlapIsReported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIncidenceOverlapIsReportedTest::RunTest(const FString&)
{
	auto Along = [](int32 Start, int32 End)
	{
		FRTGeometrySegment S = IncidenceHorizontalDiameter();
		S.AlongStart = Start;
		S.AlongEnd = End;
		return S;
	};

	// 1. Sovrapposti su un tratto: segnalato.
	TestEqual(TEXT("due collineari sovrapposti sono segnalati"),
		IncidenceCount({ Along(0, 12), Along(6, 18) }, ERTGeometryViolation::OverlappingSegments), 1);

	// 2. Consecutivi, che condividono UN estremo: legale, ed e' un muro lungo disegnato in due gesti.
	TestEqual(TEXT("due consecutivi che si toccano in un estremo sono legali"),
		IncidenceCount({ Along(0, 12), Along(12, 24) }, ERTGeometryViolation::OverlappingSegments), 0);

	// 3. Disgiunti sulla stessa retta: legale.
	TestEqual(TEXT("due disgiunti sulla stessa retta sono legali"),
		IncidenceCount({ Along(0, 6), Along(12, 18) }, ERTGeometryViolation::OverlappingSegments), 0);

	// 4. Uno CONTENUTO nell'altro: il caso che un confronto ingenuo di estremi manca.
	TestEqual(TEXT("un segmento contenuto in un altro e' sovrapposto"),
		IncidenceCount({ Along(0, 24), Along(6, 12) }, ERTGeometryViolation::OverlappingSegments), 1);

	// 5. Stessa retta ma percorsa AL CONTRARIO: gli estremi sono una coppia non ordinata, e la regola deve
	//    saperlo — e' la stessa ragione per cui `operator==` usa `Min`/`Max`.
	TestEqual(TEXT("il verso di percorrenza non nasconde una sovrapposizione"),
		IncidenceCount({ Along(0, 12), Along(18, 6) }, ERTGeometryViolation::OverlappingSegments), 1);

	// 6. ⚠️ LA CONTROPROVA: stessa giacitura ma OFFSET diverso sono due rette parallele distinte, che non
	//    si sovrappongono mai. Una regola che confrontasse solo l'asse le segnalerebbe.
	FRTGeometrySegment Parallel = Along(6, 18);
	Parallel.Offset = RT_GeometryQuanta / 2;
	TestEqual(TEXT("due paralleli distinti non si sovrappongono"),
		IncidenceCount({ Along(0, 12), Parallel }, ERTGeometryViolation::OverlappingSegments), 0);

	// 7. E su LAYER diversi non si incontrano affatto: la geometria di un piano non tocca l'altro.
	FRTGeometrySegment Upstairs = Along(6, 18);
	Upstairs.Layer = 1;
	TestEqual(TEXT("due segmenti su layer diversi non si sovrappongono"),
		IncidenceCount({ Along(0, 12), Upstairs }, ERTGeometryViolation::OverlappingSegments), 0);

	// 8. L'identico resta `DuplicateSegment` e non diventa una sovrapposizione: due reason code per due
	//    configurazioni, o la verifica di mutazione non sa quale regola ha allentato.
	TestEqual(TEXT("l'identico resta un duplicato"),
		IncidenceCount({ Along(0, 12), Along(0, 12) }, ERTGeometryViolation::DuplicateSegment), 1);
	TestEqual(TEXT("e non viene contato anche come sovrapposizione"),
		IncidenceCount({ Along(0, 12), Along(0, 12) }, ERTGeometryViolation::OverlappingSegments), 0);

	return true;
}

/**
 * LE REGOLE ARRIVANO DOVE QUALCUNO LE LEGGE — `ValidateMap`.
 *
 * 🔴 **Il rilievo che ha cambiato dove va il codice.** `URTGeometryGrammarLibrary::Validate` non ha un solo
 * chiamante di produzione: `git grep` lo trova tre volte, tutte e tre in un file di test. Aggiungere le
 * regole solo li' le farebbe nascere morte insieme allo strato che le ospita. Questo test e' cio' che
 * impedisce che accada.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIncidenceReachesValidateMapTest,
	"RefactorTactics.Incidence.ReachesValidateMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIncidenceReachesValidateMapTest::RunTest(const FString&)
{
	const FRTCellId Origin(0, 0, 0);

	auto MakeMap = [&Origin]()
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		FRTHexCellData Cell;
		Cell.Id = Origin;
		Map->AddOrUpdateCell(Cell);
		return Map;
	};

	auto CountMatching = [](URTHexMapAsset* Map, const TCHAR* Needle)
	{
		int32 N = 0;
		for (const FString& Line : Map->ValidateMap())
		{
			if (Line.Contains(Needle))
			{
				++N;
			}
		}
		return N;
	};

	// Due muri interni della STESSA cella che si incrociano fuori-anchor.
	{
		URTHexMapAsset* Map = MakeMap();
		FRTGeometrySegment Shifted = IncidenceVerticalDiameter();
		Shifted.Offset = RT_GeometryQuanta / 2;
		Map->InteriorWalls.Add(FRTHexInteriorWall(Origin, IncidenceHorizontalDiameter()));
		Map->InteriorWalls.Add(FRTHexInteriorWall(Origin, Shifted));

		TestEqual(TEXT("l'incrocio fuori-anchor arriva in ValidateMap"),
			CountMatching(Map, TEXT("incrocio")), 1);
	}

	// Due muri interni sovrapposti.
	{
		URTHexMapAsset* Map = MakeMap();
		FRTGeometrySegment A = IncidenceHorizontalDiameter();
		A.AlongStart = 0; A.AlongEnd = 12;
		FRTGeometrySegment B = IncidenceHorizontalDiameter();
		B.AlongStart = 6; B.AlongEnd = 18;
		Map->InteriorWalls.Add(FRTHexInteriorWall(Origin, A));
		Map->InteriorWalls.Add(FRTHexInteriorWall(Origin, B));

		TestEqual(TEXT("la sovrapposizione arriva in ValidateMap"),
			CountMatching(Map, TEXT("sovrappo")), 1);
	}

	// ⚠️ LA CONTROPROVA, e non e' formale: i segmenti sono in coordinate LOCALI di cella, quindi due muri
	// di celle DIVERSE non si incrociano — anche quando i loro numeri sono identici. Una regola che
	// confrontasse la collezione intera senza raggruppare per cella segnalerebbe questi due, e sarebbe il
	// difetto piu' facile da introdurre qui.
	{
		URTHexMapAsset* Map = MakeMap();
		const FRTCellId Far = URTHexLibrary::Neighbor(Origin, ERTHexDirection::E);
		FRTHexCellData Cell;
		Cell.Id = Far;
		Map->AddOrUpdateCell(Cell);

		FRTGeometrySegment Shifted = IncidenceVerticalDiameter();
		Shifted.Offset = RT_GeometryQuanta / 2;
		Map->InteriorWalls.Add(FRTHexInteriorWall(Origin, IncidenceHorizontalDiameter()));
		Map->InteriorWalls.Add(FRTHexInteriorWall(Far, Shifted));

		TestEqual(TEXT("due muri di celle diverse non si incrociano"),
			CountMatching(Map, TEXT("incrocio")), 0);
	}

	// E una mappa senza muri interni non guadagna segnalazioni.
	{
		URTHexMapAsset* Map = MakeMap();
		TestEqual(TEXT("una mappa pulita resta pulita"), Map->ValidateMap().Num(), 0);
	}

	return true;
}

/**
 * `GEO-9` — UN SEGMENTO PER GIACITURA E INTERVALLO, e il bake smette di dire il contrario.
 *
 * 🔴 **Questa non e' una regola nuova: e' una contraddizione risolta.** Il bake creava due muri quando il
 * `WallType` differiva — *«due muri sulla stessa giacitura ma di tipo diverso sono due muri»* — mentre
 * `ValidateMap` e `DuplicateSegment` li segnalavano entrambi, perche' `operator==` il tipo non lo include.
 * Il bake produceva un asset che i due validator dichiaravano invalido.
 *
 * L'uscita di `D-288`: vince `High` su `Low`, la stessa regola deterministica gia' applicata sui bordi
 * invece di «vince l'ultimo arrivato».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIncidenceBakeKeepsOneWallPerGeometryTest,
	"RefactorTactics.Incidence.BakeKeepsOneWallPerGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIncidenceBakeKeepsOneWallPerGeometryTest::RunTest(const FString&)
{
	const FRTCellId Origin(0, 0, 0);

	auto MakeMap = [&Origin]()
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		FRTHexCellData Cell;
		Cell.Id = Origin;
		Map->AddOrUpdateCell(Cell);
		return Map;
	};

	// Lo stesso muro interno, disegnato due volte con tipo diverso. Il diametro fra due VERTICI non giace su
	// nessun bordo, quindi e' un muro interno e non una copertura.
	FRTGeometrySegment Low = IncidenceVerticalDiameter();
	Low.WallType = ERTHexCoverType::Low;
	FRTGeometrySegment High = IncidenceVerticalDiameter();
	High.WallType = ERTHexCoverType::High;

	{
		URTHexMapAsset* Map = MakeMap();
		URTGeometryBakeLibrary::BakeCell(Map, Origin, { Low, High }, IncidenceHexSize);

		TestEqual(TEXT("la stessa giacitura produce UN muro, non due"), Map->InteriorWalls.Num(), 1);
		if (Map->InteriorWalls.Num() == 1)
		{
			TestTrue(TEXT("e vince High su Low, come gia' accade sui bordi"),
				Map->InteriorWalls[0].Segment.WallType == ERTHexCoverType::High);
		}
		TestEqual(TEXT("e l'asset cotto non si contraddice piu'"), Map->ValidateMap().Num(), 0);
	}

	// ⚠️ L'ordine non decide il tipo: e' la differenza fra una regola e «vince l'ultimo arrivato».
	{
		URTHexMapAsset* Map = MakeMap();
		URTGeometryBakeLibrary::BakeCell(Map, Origin, { High, Low }, IncidenceHexSize);
		TestEqual(TEXT("un muro anche invertendo l'ordine"), Map->InteriorWalls.Num(), 1);
		if (Map->InteriorWalls.Num() == 1)
		{
			TestTrue(TEXT("e vince High comunque"),
				Map->InteriorWalls[0].Segment.WallType == ERTHexCoverType::High);
		}
	}

	// ⚠️ LA CONTROPROVA: due muri sulla stessa giacitura ma su TRATTI diversi restano due muri. La regola e'
	// l'identita' geometrica, non la giacitura — ed e' gia' cio' che `operator==` significa.
	{
		URTHexMapAsset* Map = MakeMap();
		FRTGeometrySegment Lower = IncidenceVerticalDiameter();
		Lower.AlongStart = -RT_GeometryQuanta; Lower.AlongEnd = 0;
		FRTGeometrySegment Upper = IncidenceVerticalDiameter();
		Upper.AlongStart = 0; Upper.AlongEnd = RT_GeometryQuanta;
		Upper.WallType = ERTHexCoverType::Low;

		URTGeometryBakeLibrary::BakeCell(Map, Origin, { Lower, Upper }, IncidenceHexSize);
		TestEqual(TEXT("due tratti diversi restano due muri"), Map->InteriorWalls.Num(), 2);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
