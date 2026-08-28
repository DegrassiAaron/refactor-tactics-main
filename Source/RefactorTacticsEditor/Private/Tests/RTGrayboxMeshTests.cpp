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
	const TCHAR* CoverLowPath =
		TEXT("/Game/RT/World/Graybox/Cover/SM_Graybox_Cover_Low.SM_Graybox_Cover_Low");

	/** Le frazioni di §6.3 per la copertura bassa: spessore e larghezza sul LATO, altezza su `H`. */
	constexpr float CoverLowThicknessFraction = 0.10f;
	constexpr float CoverLowLengthFraction    = 0.92f;
	constexpr float CoverLowHeightFraction    = 0.28f;

	/** Un decimo di uu: la geometria e' esatta, la tolleranza copre il solo arrotondamento in float. */
	constexpr float Tolerance = 0.1f;

	UStaticMesh* LoadKitMesh(const TCHAR* Path)
	{
		return Cast<UStaticMesh>(FSoftObjectPath(Path).TryLoad());
	}
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

	const URTHexMapAsset* Defaults = GetDefault<URTHexMapAsset>();
	const float Side = Defaults->HexSize;
	const float H = Defaults->LayerHeight;

	const FBox Bounds = Mesh->GetBoundingBox();
	const FVector Size = Bounds.GetSize();

	// X = spessore verso il vicino, Y = lunghezza lungo il bordo, Z = altezza: la convenzione di
	// `EdgeRotation`, la stessa dei pannelli di #712.
	TestEqual(TEXT("spessore = 0.10 del lato"), static_cast<float>(Size.X), CoverLowThicknessFraction * Side, Tolerance);
	TestEqual(TEXT("larghezza = 0.92 del lato"), static_cast<float>(Size.Y), CoverLowLengthFraction * Side, Tolerance);
	TestEqual(TEXT("altezza = 0.28 H"), static_cast<float>(Size.Z), CoverLowHeightFraction * H, Tolerance);

	// Pivot contract §4 per un `EdgeBound`: centro del segmento, ALLA BASE. Un pivot centrato in Z
	// interrerebbe meta' della copertura, e a schermo si vedrebbe come una copertura piu' bassa del budget
	// invece che come un pivot sbagliato.
	TestEqual(TEXT("la base sta a Z = 0"), static_cast<float>(Bounds.Min.Z), 0.f, Tolerance);
	TestEqual(TEXT("il pivot e' centrato in X"), static_cast<float>(Bounds.GetCenter().X), 0.f, Tolerance);
	TestEqual(TEXT("il pivot e' centrato in Y"), static_cast<float>(Bounds.GetCenter().Y), 0.f, Tolerance);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRTGrayboxMeshesHaveFaceNormalsTest,
	"RefactorTactics.Graybox.MeshesHaveFaceNormals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGrayboxMeshesHaveFaceNormalsTest::RunTest(const FString&)
{
	UStaticMesh* Mesh = LoadKitMesh(CoverLowPath);
	if (!TestNotNull(TEXT("SM_Graybox_Cover_Low esiste"), Mesh))
	{
		return false;
	}

	const FMeshDescription* Description = Mesh->GetMeshDescription(0);
	if (!TestNotNull(TEXT("l'asset conserva la sua MeshDescription sorgente"), Description))
	{
		return false;
	}

	FStaticMeshConstAttributes Attributes(*Description);
	TVertexInstanceAttributesConstRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();

	// 🔴 Senza questa riga il test sarebbe CIECO: i due contatori sotto restano a zero anche su una
	// description vuota, e «nessuna normale degenere» si leggerebbe come un esito mentre e' un'assenza di
	// dati. Sei facce per quattro angoli, e gli angoli NON si condividono fra facce — e' cio' che tiene gli
	// spigoli vivi.
	TestEqual(TEXT("un box ha 24 vertex instance"), Description->VertexInstances().Num(), 24);

	int32 Degenerate = 0;
	int32 NonAxial = 0;
	for (const FVertexInstanceID Instance : Description->VertexInstances().GetElementIDs())
	{
		const FVector3f Normal = Normals[Instance];
		if (!Normal.IsNormalized())
		{
			++Degenerate;
			continue;
		}

		// Un box ha sei facce assiali: una normale che non lo e' significa che qualcuno le ha ricalcolate
		// e fuse, cioe' che gli spigoli vivi — il canale su cui si legge la silhouette — sono spariti.
		const float MaxComponent = Normal.GetAbs().GetMax();
		if (!FMath::IsNearlyEqual(MaxComponent, 1.f, 0.01f))
		{
			++NonAxial;
		}
	}

	// 🔴 E' il difetto che `D-229` mette al primo posto: `GetCellPrismMesh` scrive i soli
	// `GetVertexPositions()` e se la cava perche' la board non e' giudicata per ombreggiatura. Queste mesh
	// si guardano ILLUMINATE, e una faccia senza normale non fa ombra.
	TestEqual(TEXT("nessuna normale degenere"), Degenerate, 0);
	TestEqual(TEXT("le normali di un box sono assiali"), NonAxial, 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
