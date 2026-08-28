#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTMapVisuals.h"   // RTCellPrismRadius: la convenzione della mesh che il volume scala

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCellVolumeTransformTest,
	"RefactorTactics.Hex.CellVolumeTransformScalesThePrismToTheCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCellVolumeTransformTest::RunTest(const FString&)
{
	// Il Cell Placement Volume posa la mesh di GetCellPrismMesh, che nasce con circumraggio
	// RTCellPrismRadius (50 uu) e mezza-altezza 50 uu, centrata. Questa funzione traduce le misure
	// della cella in una scala per quella mesh: e' il solo punto in cui la convenzione della mesh e
	// quella della griglia si incontrano, e per questo non vive nel Blueprint.
	const FRTCellId Cell(2, -1, 0);
	const FTransform T = URTHexLibrary::CellVolumeTransform(Cell, CornersOrigin, CornersHexSize, CornersLayerH, 1.0f);

	// Posizione: il centro della cella, non un punto ricalcolato.
	const FVector Center = URTHexLibrary::AxialToWorld(Cell, CornersOrigin, CornersHexSize, CornersLayerH);
	TestTrue(TEXT("posizionato sul centro della cella"), T.GetLocation().Equals(Center, 0.01));

	const FVector Scale = T.GetScale3D();
	TestTrue(TEXT("scala planare = HexSize/RTCellPrismRadius"),
		FMath::IsNearlyEqual(Scale.X, 3.0, 0.001) && FMath::IsNearlyEqual(Scale.Y, 3.0, 0.001));

	// La meta' che conta: l'estensione verticale del volume scalato deve valere ESATTAMENTE
	// LayerHeight. E' cio' che rende vera l'affermazione di §5 — i prismi tassellano anche in
	// verticale, senza intercapedine fra il soffitto di una cella e il pavimento della successiva.
	const double VerticalExtent = Scale.Z * 2.0 * static_cast<double>(RTCellPrismRadius);
	TestTrue(TEXT("estensione verticale = LayerHeight (tassella)"),
		FMath::IsNearlyEqual(VerticalExtent, static_cast<double>(CornersLayerH), 0.01));

	// E il raggio scalato deve valere HexSize, altrimenti il volume non e' la cella che dichiara.
	const double PlanarRadius = Scale.X * static_cast<double>(RTCellPrismRadius);
	TestTrue(TEXT("raggio scalato = HexSize"),
		FMath::IsNearlyEqual(PlanarRadius, static_cast<double>(CornersHexSize), 0.01));

	TestTrue(TEXT("nessuna rotazione: il volume e' allineato alla griglia"),
		T.GetRotation().Equals(FQuat::Identity, 0.0001));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCellVolumeInsetTest,
	"RefactorTactics.Hex.CellVolumeInsetShrinksOnlyThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCellVolumeInsetTest::RunTest(const FString&)
{
	// GBX-1 e' una frazione di C e misura quanto un asset si ritrae dai vicini: agisce sulla PIANTA.
	// L'altezza non c'entra — §6 dichiara due denominatori, uno per asse, e usarne uno solo e' il
	// difetto che D-168 ha corretto. Se l'inset toccasse anche Z, i volumi smetterebbero di tassellare
	// in verticale e la guida mentirebbe sull'unica cosa che nessuno va a ricontrollare.
	const FRTCellId Cell(0, 0, 0);
	const FTransform Outer = URTHexLibrary::CellVolumeTransform(Cell, CornersOrigin, CornersHexSize, CornersLayerH, 1.00f);
	const FTransform Safe  = URTHexLibrary::CellVolumeTransform(Cell, CornersOrigin, CornersHexSize, CornersLayerH, 0.90f);

	TestTrue(TEXT("la pianta si ritrae del 10%"),
		FMath::IsNearlyEqual(Safe.GetScale3D().X, Outer.GetScale3D().X * 0.90, 0.001));
	TestTrue(TEXT("l'altezza NON cambia"),
		FMath::IsNearlyEqual(Safe.GetScale3D().Z, Outer.GetScale3D().Z, 0.0001));
	TestTrue(TEXT("stesso centro"), Safe.GetLocation().Equals(Outer.GetLocation(), 0.01));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
