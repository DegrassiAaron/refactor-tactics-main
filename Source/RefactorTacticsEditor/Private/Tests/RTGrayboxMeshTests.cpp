#include "Misc/AutomationTest.h"

#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UObject/SoftObjectPath.h"

#include "Map/RTHexMapAsset.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * La rete che i budget di forma non avevano.
 *
 * §10 del contratto graybox dichiara che «nessuna parte di questo contratto e' oggi difesa da un gate
 * automatico», e la ragione che porta e' giusta ma parziale: l'oracolo di «e' leggibile» non esiste
 * nell'harness e non va simulato. Quello di «e' spesso `0.10` del lato» invece esiste, ed e' una
 * sottrazione.
 *
 * Cosa questi test proteggono, e da cosa: le mesh del kit sono GENERATE (`D-229`), quindi il rischio non
 * e' che qualcuno le modelli male — e' che i budget di §6.3 cambino nel contratto e gli `.uasset` restino
 * quelli di prima, oppure che la scala canonica si muova sotto e nessuno rigeneri. In entrambi i casi il
 * repository resterebbe verde mostrando una geometria che nessun documento descrive.
 *
 * ⚠️ **I valori attesi si DERIVANO dal CDO e dalle frazioni, non si scrivono in uu.** Un test che
 * confrontasse `15.0` con `15.0` passerebbe anche dopo un cambio di `HexSize`, cioe' esattamente quando
 * dovrebbe fallire.
 */
namespace
{
	const TCHAR* CoverLowPath  = TEXT("/Game/RT/World/Graybox/Cover/SM_Graybox_Cover_Low.SM_Graybox_Cover_Low");
	const TCHAR* CoverHighPath = TEXT("/Game/RT/World/Graybox/Cover/SM_Graybox_Cover_High.SM_Graybox_Cover_High");
	const TCHAR* DoorPanelPath = TEXT("/Game/RT/World/Graybox/Doors/SM_Graybox_Door_Panel.SM_Graybox_Door_Panel");
	const TCHAR* DoorLockedPath = TEXT("/Game/RT/World/Graybox/Doors/SM_Graybox_Door_Locked.SM_Graybox_Door_Locked");
	const TCHAR* WaterPath = TEXT("/Game/RT/World/Graybox/Surfaces/SM_Graybox_Surface_Water.SM_Graybox_Surface_Water");
	const TCHAR* IcePath   = TEXT("/Game/RT/World/Graybox/Surfaces/SM_Graybox_Surface_Ice.SM_Graybox_Surface_Ice");

	const TCHAR* AllKitMeshes[] = {
		CoverLowPath, CoverHighPath, DoorPanelPath, DoorLockedPath, WaterPath, IcePath,
	};

	/** Le frazioni di §6.3. Stanno qui per essere confrontate col contratto a occhio nudo. */
	constexpr float CoverLowThicknessFraction  = 0.10f;
	constexpr float CoverHighThicknessFraction = 0.20f;
	constexpr float PanelLengthFraction        = 0.92f;
	constexpr float CoverLowHeightFraction     = 0.28f;
	constexpr float CoverHighHeightFraction    = 0.85f;
	constexpr float DoorThicknessFraction      = 0.10f;
	constexpr float DoorHeightFraction         = 0.85f;
	constexpr float TraverseReliefFraction     = 0.06f;

	/** Un decimo di uu: la geometria e' esatta, la tolleranza copre il solo arrotondamento in float. */
	constexpr float Tolerance = 0.1f;

	UStaticMesh* LoadKitMesh(const TCHAR* Path)
	{
		return Cast<UStaticMesh>(FSoftObjectPath(Path).TryLoad());
	}

	float Side() { return GetDefault<URTHexMapAsset>()->HexSize; }
	float LayerH() { return GetDefault<URTHexMapAsset>()->LayerHeight; }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRTGrayboxCoverLowMatchesContractTest,
	"RefactorTactics.Graybox.CoverLowMatchesContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGrayboxCoverLowMatchesContractTest::RunTest(const FString&)
{
	UStaticMesh* Mesh = LoadKitMesh(CoverLowPath);
	if (!TestNotNull(TEXT("SM_Graybox_Cover_Low esiste"), Mesh))
	{
		return false;
	}

	const FBox Bounds = Mesh->GetBoundingBox();
	const FVector Size = Bounds.GetSize();

	// X = spessore verso il vicino, Y = lunghezza lungo il bordo, Z = altezza: la convenzione di
	// `EdgeRotation`, la stessa dei pannelli di #712.
	TestEqual(TEXT("spessore = 0.10 del lato"), static_cast<float>(Size.X), CoverLowThicknessFraction * Side(), Tolerance);
	TestEqual(TEXT("larghezza = 0.92 del lato"), static_cast<float>(Size.Y), PanelLengthFraction * Side(), Tolerance);
	TestEqual(TEXT("altezza = 0.28 H"), static_cast<float>(Size.Z), CoverLowHeightFraction * LayerH(), Tolerance);

	// Pivot contract §4 per un `EdgeBound`: centro del segmento, ALLA BASE. Un pivot centrato in Z
	// interrerebbe meta' della copertura, e a schermo si vedrebbe come una copertura piu' bassa del budget
	// invece che come un pivot sbagliato.
	TestEqual(TEXT("la base sta a Z = 0"), static_cast<float>(Bounds.Min.Z), 0.f, Tolerance);
	TestEqual(TEXT("il pivot e' centrato in X"), static_cast<float>(Bounds.GetCenter().X), 0.f, Tolerance);
	TestEqual(TEXT("il pivot e' centrato in Y"), static_cast<float>(Bounds.GetCenter().Y), 0.f, Tolerance);

	return true;
}

/**
 * Il test che impedisce di ripetere `#1246`.
 *
 * 🔴 `PIE-HEX-VIZ-BLOCCHI` e' ❌ dal 2026-08-20 perche' la differenza fra due volumi stava nell'ALTEZZA,
 * che la vista a picco proietta a zero. Le due coperture del kit rischiano lo stesso difetto per
 * costruzione, e qui si verifica che non lo abbiano: la differenza dev'essere anche IN PIANTA, e il
 * fattore e' `2` (§6.3).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRTGrayboxCoverPairSeparatesInPlanTest,
	"RefactorTactics.Graybox.CoverPairSeparatesInPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGrayboxCoverPairSeparatesInPlanTest::RunTest(const FString&)
{
	UStaticMesh* Low = LoadKitMesh(CoverLowPath);
	UStaticMesh* High = LoadKitMesh(CoverHighPath);
	if (!TestNotNull(TEXT("SM_Graybox_Cover_Low esiste"), Low) ||
		!TestNotNull(TEXT("SM_Graybox_Cover_High esiste"), High))
	{
		return false;
	}

	const FVector LowSize = Low->GetBoundingBox().GetSize();
	const FVector HighSize = High->GetBoundingBox().GetSize();

	TestEqual(TEXT("la bassa e' spessa 0.10 del lato"), static_cast<float>(LowSize.X), CoverLowThicknessFraction * Side(), Tolerance);
	TestEqual(TEXT("l'alta e' spessa 0.20 del lato"), static_cast<float>(HighSize.X), CoverHighThicknessFraction * Side(), Tolerance);
	TestEqual(TEXT("l'alta e' alta 0.85 H"), static_cast<float>(HighSize.Z), CoverHighHeightFraction * LayerH(), Tolerance);

	// L'invariante vera: in PIANTA il rapporto e' 2. Se qualcuno pareggiasse gli spessori lasciando le
	// altezze diverse, ogni asserzione sopra resterebbe verde tranne questa.
	TestEqual(TEXT("il fattore in pianta e' 2"),
		static_cast<float>(HighSize.X / LowSize.X), 2.f, 0.01f);

	// E la stessa larghezza: la differenza sta nello spessore, non nell'estensione lungo il bordo — un
	// pannello piu' corto lascerebbe passare lo sguardo dove la regola dice che non si passa.
	TestEqual(TEXT("le due coperture sono lunghe uguale"),
		static_cast<float>(HighSize.Y), static_cast<float>(LowSize.Y), Tolerance);

	return true;
}

/**
 * `D-171`: `Locked` non e' `Closed` ricolorata, ed e' una mesh diversa con una traversa in rilievo.
 *
 * ⚠️ Il rilievo sta su ENTRAMBE le facce (§3): un `EdgeBound` non appartiene a nessuna delle due celle,
 * quindi si guarda da entrambi i lati.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRTGrayboxLockedDoorCarriesTraverseTest,
	"RefactorTactics.Graybox.LockedDoorCarriesTraverse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGrayboxLockedDoorCarriesTraverseTest::RunTest(const FString&)
{
	UStaticMesh* Panel = LoadKitMesh(DoorPanelPath);
	UStaticMesh* Locked = LoadKitMesh(DoorLockedPath);
	if (!TestNotNull(TEXT("SM_Graybox_Door_Panel esiste"), Panel) ||
		!TestNotNull(TEXT("SM_Graybox_Door_Locked esiste"), Locked))
	{
		return false;
	}

	const FVector PanelSize = Panel->GetBoundingBox().GetSize();
	const FVector LockedSize = Locked->GetBoundingBox().GetSize();

	TestEqual(TEXT("il pannello e' spesso 0.10 del lato"), static_cast<float>(PanelSize.X), DoorThicknessFraction * Side(), Tolerance);
	TestEqual(TEXT("il pannello e' alto 0.85 H"), static_cast<float>(PanelSize.Z), DoorHeightFraction * LayerH(), Tolerance);

	// Il rilievo si somma DUE volte: una per faccia.
	TestEqual(TEXT("la traversa sporge su entrambe le facce"),
		static_cast<float>(LockedSize.X),
		static_cast<float>(PanelSize.X) + 2.f * TraverseReliefFraction * Side(), Tolerance);

	// E non altera la sagoma del pannello: una porta bloccata resta una porta.
	TestEqual(TEXT("stessa altezza del pannello"), static_cast<float>(LockedSize.Z), static_cast<float>(PanelSize.Z), Tolerance);
	TestEqual(TEXT("stessa larghezza del pannello"), static_cast<float>(LockedSize.Y), static_cast<float>(PanelSize.Y), Tolerance);

	return true;
}

/**
 * `PIE-GBX-SURFACE` chiede di distinguere acqua e ghiaccio a zoom tattico e in scala di grigi. Il canale
 * scelto (§6.3) e' la FRATTURA: l'acqua e' una lastra continua, il ghiaccio sono sei lastre a quote
 * diverse dentro lo stesso spessore.
 *
 * ⚠️ Questo test non dice che si LEGGONO — quello lo dice l'occhio in seduta. Dice che il canale esiste
 * nella geometria: se un giorno il ghiaccio tornasse piatto, la voce PIE fallirebbe davanti a una persona
 * invece che qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRTGrayboxSurfacesSeparateByFractureTest,
	"RefactorTactics.Graybox.SurfacesSeparateByFracture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGrayboxSurfacesSeparateByFractureTest::RunTest(const FString&)
{
	auto CountDistinctTopHeights = [](UStaticMesh* Mesh) -> int32
	{
		const FMeshDescription* Description = Mesh ? Mesh->GetMeshDescription(0) : nullptr;
		if (!Description)
		{
			return 0;
		}

		FStaticMeshConstAttributes Attributes(*Description);
		TVertexAttributesConstRef<FVector3f> Positions = Attributes.GetVertexPositions();

		TSet<int32> Heights;
		for (const FVertexID Vertex : Description->Vertices().GetElementIDs())
		{
			const float Z = Positions[Vertex].Z;
			if (Z > KINDA_SMALL_NUMBER)
			{
				Heights.Add(FMath::RoundToInt(Z * 100.f));
			}
		}
		return Heights.Num();
	};

	UStaticMesh* Water = LoadKitMesh(WaterPath);
	UStaticMesh* Ice = LoadKitMesh(IcePath);
	if (!TestNotNull(TEXT("SM_Graybox_Surface_Water esiste"), Water) ||
		!TestNotNull(TEXT("SM_Graybox_Surface_Ice esiste"), Ice))
	{
		return false;
	}

	TestEqual(TEXT("l'acqua e' una lastra sola"), CountDistinctTopHeights(Water), 1);
	TestEqual(TEXT("il ghiaccio ha sei quote"), CountDistinctTopHeights(Ice), 6);

	// Entrambe stanno dentro lo stesso spessore: la frattura e' un canale, non un ingombro diverso.
	TestEqual(TEXT("stesso spessore"),
		static_cast<float>(Ice->GetBoundingBox().GetSize().Z),
		static_cast<float>(Water->GetBoundingBox().GetSize().Z), Tolerance);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRTGrayboxMeshesHaveFaceNormalsTest,
	"RefactorTactics.Graybox.MeshesHaveFaceNormals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGrayboxMeshesHaveFaceNormalsTest::RunTest(const FString&)
{
	for (const TCHAR* Path : AllKitMeshes)
	{
		UStaticMesh* Mesh = LoadKitMesh(Path);
		if (!TestNotNull(*FString::Printf(TEXT("%s esiste"), Path), Mesh))
		{
			continue;
		}

		const FMeshDescription* Description = Mesh->GetMeshDescription(0);
		if (!TestNotNull(TEXT("l'asset conserva la sua MeshDescription sorgente"), Description))
		{
			continue;
		}

		FStaticMeshConstAttributes Attributes(*Description);
		TVertexInstanceAttributesConstRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();

		// 🔴 Senza queste due righe il test sarebbe CIECO: i contatori sotto restano a zero anche su una
		// description vuota, e «nessuna normale degenere» si leggerebbe come un esito mentre e' un'assenza
		// di dati.
		TestTrue(TEXT("ha poligoni"), Description->Polygons().Num() > 0);
		TestTrue(TEXT("ha vertex instance"), Description->VertexInstances().Num() > 0);

		int32 Degenerate = 0;
		for (const FVertexInstanceID Instance : Description->VertexInstances().GetElementIDs())
		{
			if (!Normals[Instance].IsNormalized())
			{
				++Degenerate;
			}
		}

		// E' il difetto che `D-229` mette al primo posto: `GetCellPrismMesh` scrive i soli
		// `GetVertexPositions()` e se la cava perche' la board non e' giudicata per ombreggiatura. Queste
		// si guardano ILLUMINATE, e una faccia senza normale non fa ombra.
		TestEqual(*FString::Printf(TEXT("%s: nessuna normale degenere"), Path), Degenerate, 0);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
