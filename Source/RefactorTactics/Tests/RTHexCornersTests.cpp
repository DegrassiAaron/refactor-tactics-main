#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

// Il Cell Placement Volume (#1095, contratto graybox §5) e' un prisma esagonale "derivato dalla cella
// logica". Derivato significa che i vertici escono dalla stessa matematica che AxialToWorld usa per i
// centri: se il volume li ricalcolasse per conto suo, la guida potrebbe mentire senza che nulla fallisca.
// Questi test fissano proprio quella parentela.

// I nomi portano il prefisso del file di proposito: con la unity build di UE piu' .cpp finiscono in
// una sola unita' di compilazione, quindi due namespace anonimi si vedono a vicenda. Un `CornersHexSize`
// generico collide con quello di RTHexOccupancyTests.cpp, e l'errore arriva dal file dell'altro.
namespace
{
	constexpr float CornersHexSize = 150.f;   // il canone: URTHexMapAsset::HexSize
	constexpr float CornersLayerH  = 250.f;   // LayerHeight
	const FVector   CornersOrigin  = FVector::ZeroVector;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCornersShapeTest,
	"RefactorTactics.Hex.CellCornersFormPointyTopHexagon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCornersShapeTest::RunTest(const FString&)
{
	const FRTCellId Cell(0, 0, 0);
	const TArray<FVector> Corners = URTHexLibrary::CellCorners(Cell, CornersOrigin, CornersHexSize, CornersLayerH);

	TestEqual(TEXT("sei vertici"), Corners.Num(), 6);

	const FVector Center = URTHexLibrary::AxialToWorld(Cell, CornersOrigin, CornersHexSize, CornersLayerH);

	// Circumraggio: la "dimensione" e' il lato, e RTHexLibrary.cpp lo dichiara — un pointy-top di
	// circumraggio HexSize. Ogni vertice dista quindi esattamente HexSize dal centro, nel piano.
	for (int32 i = 0; i < Corners.Num(); ++i)
	{
		const double R = FVector2D(Corners[i].X - Center.X, Corners[i].Y - Center.Y).Size();
		TestTrue(FString::Printf(TEXT("vertice %d a distanza HexSize (%.3f)"), i, R),
			FMath::IsNearlyEqual(R, static_cast<double>(CornersHexSize), 0.01));
		TestTrue(FString::Printf(TEXT("vertice %d complanare al centro"), i),
			FMath::IsNearlyEqual(Corners[i].Z, Center.Z, 0.01));
	}

	// Lato-a-lato: C = sqrt(3) * HexSize = 259,8 uu col canone. E' il denominatore che il contratto §6
	// usa per inset e ingombri, quindi sbagliarlo di 1,73x e' il difetto che quella sezione teme.
	double MinX = TNumericLimits<double>::Max(), MaxX = -TNumericLimits<double>::Max();
	double MinY = TNumericLimits<double>::Max(), MaxY = -TNumericLimits<double>::Max();
	for (const FVector& V : Corners)
	{
		MinX = FMath::Min(MinX, V.X); MaxX = FMath::Max(MaxX, V.X);
		MinY = FMath::Min(MinY, V.Y); MaxY = FMath::Max(MaxY, V.Y);
	}
	TestTrue(TEXT("larghezza = sqrt(3)*HexSize (lato-a-lato)"),
		FMath::IsNearlyEqual(MaxX - MinX, 1.7320508075688772 * static_cast<double>(CornersHexSize), 0.01));
	// Pointy-top: le punte stanno sull'asse Y, quindi l'estensione verticale e' 2*HexSize.
	TestTrue(TEXT("altezza in pianta = 2*HexSize (le punte)"),
		FMath::IsNearlyEqual(MaxY - MinY, 2.0 * static_cast<double>(CornersHexSize), 0.01));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCornersTessellateTest,
	"RefactorTactics.Hex.CellCornersAreSharedByNeighbours",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCornersTessellateTest::RunTest(const FString&)
{
	// La prova che i vertici sono DERIVATI e non ricalcolati: due celle adiacenti devono condividere
	// esattamente due vertici. Se la formula divergesse anche di poco dalla griglia, i prismi si
	// sovrapporrebbero o lascerebbero una fessura, e una guida d'authoring che non tassella e' peggio
	// che nessuna guida.
	const FRTCellId Center(0, 0, 0);
	const TArray<FVector> A = URTHexLibrary::CellCorners(Center, CornersOrigin, CornersHexSize, CornersLayerH);

	for (const FRTCellId& N : URTHexLibrary::Neighbors(Center))
	{
		const TArray<FVector> B = URTHexLibrary::CellCorners(N, CornersOrigin, CornersHexSize, CornersLayerH);
		int32 Shared = 0;
		for (const FVector& Va : A)
		{
			for (const FVector& Vb : B)
			{
				if (FVector::DistSquared(Va, Vb) < 0.01) { ++Shared; break; }
			}
		}
		TestEqual(FString::Printf(TEXT("vicino (%d,%d): due vertici condivisi"), N.X, N.Y), Shared, 2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCornersFollowLayerTest,
	"RefactorTactics.Hex.CellCornersFollowLayerHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCornersFollowLayerTest::RunTest(const FString&)
{
	// §5: i prismi tassellano anche in verticale — sopra il soffitto di una cella c'e' il pavimento
	// della successiva, senza intercapedine. I vertici seguono quindi Layer*LayerHeight come il centro.
	const TArray<FVector> L0 = URTHexLibrary::CellCorners(FRTCellId(1, -2, 0), CornersOrigin, CornersHexSize, CornersLayerH);
	const TArray<FVector> L1 = URTHexLibrary::CellCorners(FRTCellId(1, -2, 1), CornersOrigin, CornersHexSize, CornersLayerH);

	TestEqual(TEXT("stesso numero di vertici"), L0.Num(), L1.Num());
	for (int32 i = 0; i < L0.Num(); ++i)
	{
		TestTrue(FString::Printf(TEXT("vertice %d: stessa pianta"), i),
			FMath::IsNearlyEqual(L0[i].X, L1[i].X, 0.01) && FMath::IsNearlyEqual(L0[i].Y, L1[i].Y, 0.01));
		TestTrue(FString::Printf(TEXT("vertice %d: quota +LayerHeight"), i),
			FMath::IsNearlyEqual(L1[i].Z - L0[i].Z, static_cast<double>(CornersLayerH), 0.01));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
