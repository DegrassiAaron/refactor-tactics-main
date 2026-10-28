#include "Misc/AutomationTest.h"

#include "RTHexEditorClick.h"
#include "Map/RTCellId.h"
#include "Map/RTGeometryBake.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr float GhostHexSize = 150.f;

	bool GhostWallBetween(ERTAnchorKind KindA, int32 IndexA, ERTAnchorKind KindB, int32 IndexB,
		FRTGeometrySegment& Out)
	{
		const FRTCellId Home(0, 0, 0);
		if (!URTGeometryGrammarLibrary::SegmentBetweenAnchors(
				FRTAnchorRef(Home, KindA, IndexA), FRTAnchorRef(Home, KindB, IndexB), GhostHexSize, Out))
		{
			return false;
		}
		Out.WallType = ERTHexCoverType::High;
		return true;
	}

	/** Cio' che la COTTURA scrive per quel segmento, su una cella sola: quante coperture e quanti muri interni. */
	void GhostBakeResult(const FRTGeometrySegment& Segment, int32& OutCovers, int32& OutInteriorWalls)
	{
		const FRTCellId Home(0, 0, 0);
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>(GetTransientPackage());
		Map->HexSize = GhostHexSize;
		Map->Cells.Add(FRTHexCellData(Home));
		OutCovers = URTGeometryBakeLibrary::AddSegmentsToCell(Map, Home, { Segment }, GhostHexSize);
		OutInteriorWalls = Map->InteriorWalls.Num();
	}
}

/**
 * **Il ghost mostra CIO' CHE LA COTTURA SCRIVERA', per ogni forma di segmento che passa per il centro o giace sul lato.**
 *
 * 🔴 **Il difetto di `#3326` era qui, e non era provabile:** su un diametro fra lati opposti il ghost era argento
 * (= copertura) mentre la cottura scriveva un muro interno. Il ghost disegna su un `FPrimitiveDrawInterface`, ma la
 * domanda che ne decide il colore passa da `RTHexEditor::GhostPieceIsInteriorWall`, che e' pura: qui la si confronta
 * con l'esito REALE della cottura, non con una costante. Se il ghost tornasse a chiedere a `EdgesTouchedBy`, o il
 * verdetto si invertisse, il primo confronto cadrebbe.
 *
 * Quattro famiglie, perche' sono quelle in cui le sedi potevano divergere o concordare: diametri fra lati (dove
 * divergevano), diametri fra vertici e raggi verso i vertici (dove concordavano), raggi verso il punto medio di un
 * lato (divergevano), muri sul lato (devono restare coperture — la controprova, senza la quale «sempre muro interno»
 * passerebbe).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexGhostMatchesBakeTest,
	"RefactorTactics.Editor.Geometry.GhostShowsWhatTheBakeWrites",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexGhostMatchesBakeTest::RunTest(const FString&)
{
	struct FCase
	{
		FString Name;
		FRTGeometrySegment Segment;
	};
	TArray<FCase> Cases;
	bool bEveryCaseBuilt = true;

	auto Add = [&](const FString& Name, ERTAnchorKind KindA, int32 IndexA, ERTAnchorKind KindB, int32 IndexB)
	{
		FCase Case;
		Case.Name = Name;
		bEveryCaseBuilt &= TestTrue(*(Name + TEXT(": si aggancia alla grammatica")),
			GhostWallBetween(KindA, IndexA, KindB, IndexB, Case.Segment));
		Cases.Add(Case);
	};

	for (int32 K = 0; K < 3; ++K)
	{
		Add(FString::Printf(TEXT("diametro fra lati %d->%d"), K, K + 3), ERTAnchorKind::EdgeMid, K, ERTAnchorKind::EdgeMid, K + 3);
		Add(FString::Printf(TEXT("diametro fra vertici %d->%d"), K, K + 3), ERTAnchorKind::Vertex, K, ERTAnchorKind::Vertex, K + 3);
	}
	for (int32 K = 0; K < 6; ++K)
	{
		Add(FString::Printf(TEXT("raggio al punto medio %d"), K), ERTAnchorKind::Center, 0, ERTAnchorKind::EdgeMid, K);
		Add(FString::Printf(TEXT("raggio al vertice %d"), K), ERTAnchorKind::Center, 0, ERTAnchorKind::Vertex, K);
		Add(FString::Printf(TEXT("muro sul lato %d"), K), ERTAnchorKind::Vertex, K, ERTAnchorKind::Vertex, (K + 1) % 6);
	}

	if (!TestTrue(TEXT("tutti i casi sono stati costruiti"), bEveryCaseBuilt))
	{
		return false;
	}

	int32 Interior = 0;
	int32 Covers = 0;
	for (const FCase& Case : Cases)
	{
		TArray<ERTHexDirection> CoverEdges = { ERTHexDirection::NE, ERTHexDirection::SW }; // spazzatura da riscrivere
		const bool bGhostSaysInterior = RTHexEditor::GhostPieceIsInteriorWall(Case.Segment, GhostHexSize, CoverEdges);

		int32 BakedCovers = 0;
		int32 BakedWalls = 0;
		GhostBakeResult(Case.Segment, BakedCovers, BakedWalls);
		const bool bBakeWritesInterior = (BakedWalls == 1 && BakedCovers == 0);
		const bool bBakeWritesCovers = (BakedCovers >= 1 && BakedWalls == 0);

		if (!TestTrue(*(Case.Name + TEXT(": la cottura scrive una cosa sola, muro interno o coperture")),
				bBakeWritesInterior != bBakeWritesCovers))
		{
			continue;
		}

		TestEqual(*(Case.Name + TEXT(": il ghost dice «muro interno» se e solo se la cottura lo scrive tale")),
			bGhostSaysInterior, bBakeWritesInterior);
		if (!bGhostSaysInterior)
		{
			TestEqual(*(Case.Name + TEXT(": e i bordi che il ghost evidenzia sono tanti quanti le coperture scritte")),
				CoverEdges.Num(), BakedCovers);
		}
		else
		{
			TestEqual(*(Case.Name + TEXT(": un muro interno non evidenzia bordi, nemmeno su un array pre-riempito")),
				CoverEdges.Num(), 0);
		}

		(bGhostSaysInterior ? Interior : Covers) += 1;
	}

	// Guardia anti-vacuita': il test ha visto ENTRAMBE le classi, altrimenti confrontare ghost e cottura su una
	// sola direbbe poco (una funzione che risponde sempre «interno» passerebbe se i casi fossero tutti interni).
	TestTrue(TEXT("il test ha esaminato muri interni"), Interior > 0);
	TestTrue(TEXT("e coperture di bordo"), Covers > 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
