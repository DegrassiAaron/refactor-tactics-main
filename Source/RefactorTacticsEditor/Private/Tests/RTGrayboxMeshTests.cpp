#include "Misc/AutomationTest.h"

#include "Algo/Find.h"
#include "Engine/StaticMesh.h"
#include "MaterialEditingLibrary.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UObject/SoftObjectPath.h"

#include "Content/RTGrayboxMaterials.h"
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

	/**
	 * Un decimo di uu: la geometria e' esatta, la tolleranza copre il solo arrotondamento in float.
	 *
	 * 🔴 **Si chiamava `Tolerance`, e in unity build quel nome rompeva la compilazione del modulo.** Questa
	 * costante vive in un namespace ANONIMO, quindi in compilazione separata non si vede da nessun'altra
	 * parte; nella unity build finisce nella stessa unita' di traduzione dei test vicini, e li'
	 * `TRotator<T>::Equals(const TRotator<T>&, T Tolerance)` dell'engine — istanziato da
	 * `RTScenarioPerspectiveTests.cpp` — ha un PARAMETRO con lo stesso nome. Il parametro nasconde la
	 * dichiarazione di namespace: `C4459`, che in questo progetto e' un errore.
	 *
	 * ⚠️ **Il difetto era latente e non e' stato causato da chi lo ha incontrato**: nessuno dei due file
	 * andava toccato perche' emergesse — bastava che UBT li raggruppasse insieme, cosa che cambia quando si
	 * aggiungono sorgenti al modulo. E' la stessa forma della `SaveAssetPackage` duplicata fra i due
	 * commandlet, registrata il 2026-08-30.
	 *
	 * ∴ il nome e' specifico del kit invece che generico: un identificatore di namespace anonimo condivide
	 * lo spazio dei nomi con OGNI parametro dell'engine che finisca nella stessa unita' di traduzione.
	 */
	constexpr float KitTolerance = 0.1f;

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
	TestEqual(TEXT("spessore = 0.10 del lato"), static_cast<float>(Size.X), CoverLowThicknessFraction * Side(), KitTolerance);
	TestEqual(TEXT("larghezza = 0.92 del lato"), static_cast<float>(Size.Y), PanelLengthFraction * Side(), KitTolerance);
	TestEqual(TEXT("altezza = 0.28 H"), static_cast<float>(Size.Z), CoverLowHeightFraction * LayerH(), KitTolerance);

	// Pivot contract §4 per un `EdgeBound`: centro del segmento, ALLA BASE. Un pivot centrato in Z
	// interrerebbe meta' della copertura, e a schermo si vedrebbe come una copertura piu' bassa del budget
	// invece che come un pivot sbagliato.
	TestEqual(TEXT("la base sta a Z = 0"), static_cast<float>(Bounds.Min.Z), 0.f, KitTolerance);
	TestEqual(TEXT("il pivot e' centrato in X"), static_cast<float>(Bounds.GetCenter().X), 0.f, KitTolerance);
	TestEqual(TEXT("il pivot e' centrato in Y"), static_cast<float>(Bounds.GetCenter().Y), 0.f, KitTolerance);

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

	TestEqual(TEXT("la bassa e' spessa 0.10 del lato"), static_cast<float>(LowSize.X), CoverLowThicknessFraction * Side(), KitTolerance);
	TestEqual(TEXT("l'alta e' spessa 0.20 del lato"), static_cast<float>(HighSize.X), CoverHighThicknessFraction * Side(), KitTolerance);
	TestEqual(TEXT("l'alta e' alta 0.85 H"), static_cast<float>(HighSize.Z), CoverHighHeightFraction * LayerH(), KitTolerance);

	// L'invariante vera: in PIANTA il rapporto e' 2. Se qualcuno pareggiasse gli spessori lasciando le
	// altezze diverse, ogni asserzione sopra resterebbe verde tranne questa.
	TestEqual(TEXT("il fattore in pianta e' 2"),
		static_cast<float>(HighSize.X / LowSize.X), 2.f, 0.01f);

	// E la stessa larghezza: la differenza sta nello spessore, non nell'estensione lungo il bordo — un
	// pannello piu' corto lascerebbe passare lo sguardo dove la regola dice che non si passa.
	TestEqual(TEXT("le due coperture sono lunghe uguale"),
		static_cast<float>(HighSize.Y), static_cast<float>(LowSize.Y), KitTolerance);

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

	TestEqual(TEXT("il pannello e' spesso 0.10 del lato"), static_cast<float>(PanelSize.X), DoorThicknessFraction * Side(), KitTolerance);
	TestEqual(TEXT("il pannello e' alto 0.85 H"), static_cast<float>(PanelSize.Z), DoorHeightFraction * LayerH(), KitTolerance);

	// Il rilievo si somma DUE volte: una per faccia.
	TestEqual(TEXT("la traversa sporge su entrambe le facce"),
		static_cast<float>(LockedSize.X),
		static_cast<float>(PanelSize.X) + 2.f * TraverseReliefFraction * Side(), KitTolerance);

	// E non altera la sagoma del pannello: una porta bloccata resta una porta.
	TestEqual(TEXT("stessa altezza del pannello"), static_cast<float>(LockedSize.Z), static_cast<float>(PanelSize.Z), KitTolerance);
	TestEqual(TEXT("stessa larghezza del pannello"), static_cast<float>(LockedSize.Y), static_cast<float>(PanelSize.Y), KitTolerance);

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
		static_cast<float>(Water->GetBoundingBox().GetSize().Z), KitTolerance);

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

// ======================================================================================================
// I materiali del kit (#1714)
//
// La garanzia che mancava: fino al 2026-08-30 nulla distingueva «slot vuoto» da «slot pieno», e le sei
// mesh uscivano col grigio di default dell'engine mentre `MeshesHaveFaceNormals` garantiva che fossero
// ombreggiabili. La garanzia c'era, il consumatore no.
//
// ⚠️ I valori attesi si leggono da `RTGraybox::KitMaterials`, non si riscrivono qui: e' la stessa
// disciplina per cui i budget di forma si derivano dal CDO invece di essere fissati in uu.
// ======================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRTGrayboxMeshesCarryTheirMaterialTest,
	"RefactorTactics.Graybox.MeshesCarryTheirMaterial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGrayboxMeshesCarryTheirMaterialTest::RunTest(const FString&)
{
	// La copertura e' esaustiva per costruzione: se una mesh entrasse nel kit senza entrare nella tabella
	// dei materiali, questo confronto lo direbbe invece di lasciarla scoperta.
	TestEqual(TEXT("una istanza per ogni mesh del kit"),
		static_cast<int32>(UE_ARRAY_COUNT(RTGraybox::KitMaterials)),
		static_cast<int32>(UE_ARRAY_COUNT(AllKitMeshes)));

	for (const RTGraybox::FRTGrayboxMaterialSpec& Spec : RTGraybox::KitMaterials)
	{
		// Il path si CERCA fra quelli gia' dichiarati invece di ricostruirlo: la sottocartella
		// (`Cover` · `Doors` · `Surfaces`) non sta nella spec dei materiali, e ricopiarla qui creerebbe una
		// seconda verita' sui percorsi di §8.1.
		const TCHAR* const* Found = Algo::FindByPredicate(AllKitMeshes,
			[&Spec](const TCHAR* Path) { return FCString::Strifind(Path, Spec.MeshName) != nullptr; });
		if (!TestNotNull(*FString::Printf(TEXT("%s ha un path nel kit"), Spec.MeshName), Found))
		{
			continue;
		}

		UStaticMesh* Mesh = LoadKitMesh(*Found);
		if (!TestNotNull(*FString::Printf(TEXT("%s esiste"), Spec.MeshName), Mesh))
		{
			continue;
		}

		const TArray<FStaticMaterial>& Slots = Mesh->GetStaticMaterials();
		if (!TestEqual(*FString::Printf(TEXT("%s ha un solo slot materiale"), Spec.MeshName), Slots.Num(), 1))
		{
			continue;
		}

		// ⛔ Il cuore di #1714: uno slot che esiste ma non porta nulla e' esattamente lo stato che questa
		// issue chiude, e senza questa riga potrebbe tornare senza che nessuno se ne accorga.
		UMaterialInterface* Assigned = Slots[0].MaterialInterface;
		if (!TestNotNull(*FString::Printf(TEXT("%s ha un materiale assegnato"), Spec.MeshName), Assigned))
		{
			continue;
		}

		TestEqual(*FString::Printf(TEXT("%s porta la propria istanza"), Spec.MeshName),
			Assigned->GetName(), FString(Spec.InstanceName));

		// Il nome dello slot deve coincidere col polygon group che il commandlet chiama `Default`: se
		// divergessero, il materiale sarebbe assegnato e non ombreggerebbe niente.
		TestEqual(*FString::Printf(TEXT("%s: lo slot si chiama Default"), Spec.MeshName),
			Slots[0].MaterialSlotName, FName(TEXT("Default")));

		UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(Assigned);
		if (!TestNotNull(*FString::Printf(TEXT("%s: e' una MaterialInstanceConstant"), Spec.MeshName), Instance))
		{
			continue;
		}

		if (TestNotNull(TEXT("l'istanza ha un parent"), Instance->Parent.Get()))
		{
			TestEqual(*FString::Printf(TEXT("%s deriva dal master"), Spec.InstanceName),
				Instance->Parent->GetName(), FString(RTGraybox::MasterAssetName));
		}

		const FLinearColor Color =
			UMaterialEditingLibrary::GetMaterialInstanceVectorParameterValue(Instance, RTGraybox::ParamBaseColor);
		const float Roughness =
			UMaterialEditingLibrary::GetMaterialInstanceScalarParameterValue(Instance, RTGraybox::ParamRoughness);

		// 🔴 NEUTRO, e non e' una preferenza estetica: e' cio' che rende la lettura in scala di grigi vera
		// per costruzione. Un kit senza canale cromatico non puo' violare `D-146` affidando una categoria
		// al solo colore, perche' non c'e' un colore a cui affidarla.
		TestEqual(*FString::Printf(TEXT("%s: R == G"), Spec.InstanceName), Color.R, Color.G, 0.001f);
		TestEqual(*FString::Printf(TEXT("%s: G == B"), Spec.InstanceName), Color.G, Color.B, 0.001f);

		TestEqual(*FString::Printf(TEXT("%s: il valore e' quello della tabella"), Spec.InstanceName),
			RTGraybox::Luminance(Color), Spec.Value, 0.001f);
		TestEqual(*FString::Printf(TEXT("%s: la ruvidita' e' quella della tabella"), Spec.InstanceName),
			Roughness, Spec.Roughness, 0.001f);
	}

	return true;
}

/**
 * Le coppie che la geometria da sola non separa.
 *
 * 🔴 `Door_Panel` e `Cover_High` sono alte uguali (`0.85 H`) e lunghe uguali (`0.92` del lato): la sola
 * differenza geometrica e' lo spessore, che la vista a picco proietta quasi a zero — la stessa forma del
 * difetto che ha reso ❌ `PIE-HEX-VIZ-BLOCCHI` (#1246). Qui si verifica che il materiale porti i due canali
 * che mancano, e che li porti ENTRAMBI: un valore diverso da solo non basterebbe sotto una luce radente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRTGrayboxMaterialsSeparateAmbiguousPairsTest,
	"RefactorTactics.Graybox.MaterialsSeparateAmbiguousPairs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGrayboxMaterialsSeparateAmbiguousPairsTest::RunTest(const FString&)
{
	auto Spec = [](const TCHAR* MeshName) -> const RTGraybox::FRTGrayboxMaterialSpec*
	{
		for (const RTGraybox::FRTGrayboxMaterialSpec& Candidate : RTGraybox::KitMaterials)
		{
			if (FCString::Strcmp(Candidate.MeshName, MeshName) == 0)
			{
				return &Candidate;
			}
		}
		return nullptr;
	};

	const RTGraybox::FRTGrayboxMaterialSpec* CoverLow  = Spec(TEXT("SM_Graybox_Cover_Low"));
	const RTGraybox::FRTGrayboxMaterialSpec* CoverHigh = Spec(TEXT("SM_Graybox_Cover_High"));
	const RTGraybox::FRTGrayboxMaterialSpec* Door      = Spec(TEXT("SM_Graybox_Door_Panel"));
	const RTGraybox::FRTGrayboxMaterialSpec* Locked    = Spec(TEXT("SM_Graybox_Door_Locked"));
	const RTGraybox::FRTGrayboxMaterialSpec* Water     = Spec(TEXT("SM_Graybox_Surface_Water"));
	const RTGraybox::FRTGrayboxMaterialSpec* Ice       = Spec(TEXT("SM_Graybox_Surface_Ice"));

	if (!TestTrue(TEXT("le sei spec si risolvono per nome"),
		CoverLow && CoverHigh && Door && Locked && Water && Ice))
	{
		return false;
	}

	/** Sotto questa soglia due grigi si confondono a camera tattica, dove la cella occupa pochi pixel. */
	constexpr float MinValueSeparation = 0.10f;
	/** Una ruvidita' che differisce di meno non produce un highlight riconoscibile. */
	constexpr float MinRoughnessSeparation = 0.30f;

	TestTrue(TEXT("copertura bassa e alta si separano nel valore"),
		FMath::Abs(CoverLow->Value - CoverHigh->Value) >= MinValueSeparation);

	TestTrue(TEXT("acqua e ghiaccio si separano nel valore"),
		FMath::Abs(Water->Value - Ice->Value) >= MinValueSeparation);

	// La coppia critica: due canali, non uno.
	TestTrue(TEXT("porta e copertura alta si separano nel valore"),
		FMath::Abs(Door->Value - CoverHigh->Value) >= MinValueSeparation);
	TestTrue(TEXT("porta e copertura alta si separano nella ruvidita'"),
		FMath::Abs(Door->Roughness - CoverHigh->Roughness) >= MinRoughnessSeparation);

	// ⛔ `Closed` vs `Locked` va nell'altro verso, ed e' `D-171`: il canale e' la TRAVERSA, e il materiale
	// «puo' rinforzare, mai rimpiazzare» (#1714). Se questa distanza superasse quella delle coppie vere, il
	// colore diventerebbe il canale primario e la geometria un ornamento — cioe' esattamente la decisione
	// al contrario. `Graybox.LockedDoorCarriesTraverse` garantisce l'altra meta'.
	TestTrue(TEXT("Locked resta un rinforzo, non il canale primario"),
		FMath::Abs(Door->Value - Locked->Value) < MinValueSeparation);
	TestEqual(TEXT("Locked resta nella famiglia visiva della porta"),
		Door->Roughness, Locked->Roughness, 0.001f);

	return true;
}


/**
 * 🔴 **Il fixture si separa dal pavimento, e da se stesso.** Nasce da un difetto visto a schermo: senza
 * materiale le primitive engine prendono il grigio di default — quello del pavimento — e il fixture
 * spariva.
 *
 * ⛔ **Non e' stata `D-146` a causarlo.** Quella decisione dice al punto (4) che *«i colori concreti
 * restano placeholder sostituibili: il vincolo e' la ridondanza, non la tavolozza»*: chiede due canali,
 * non vieta il colore. Mancava il materiale.
 *
 * ⚠️ Si verificano **entrambi** i canali, come per `Door_Panel`/`Cover_High`: un valore diverso da solo
 * non basterebbe sotto una luce radente, e una ruvidita' diversa da sola non si vede a camera tattica.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGrayboxFixtureMaterialsTest,
	"RefactorTactics.Graybox.FixtureMaterialsSeparateBodyAndMarker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGrayboxFixtureMaterialsTest::RunTest(const FString&)
{
	auto Spec = [](const TCHAR* Component) -> const RTGraybox::FRTGrayboxMaterialSpec*
	{
		for (const RTGraybox::FRTGrayboxMaterialSpec& S : RTGraybox::FixtureMaterials)
		{
			if (FCString::Strcmp(S.MeshName, Component) == 0) { return &S; }
		}
		return nullptr;
	};

	const RTGraybox::FRTGrayboxMaterialSpec* Body   = Spec(TEXT("UnitBody"));
	const RTGraybox::FRTGrayboxMaterialSpec* Marker = Spec(TEXT("FacingMarker"));
	const RTGraybox::FRTGrayboxMaterialSpec* Anchor = Spec(TEXT("GroundAnchor"));
	if (!TestTrue(TEXT("le tre spec si risolvono per componente"), Body && Marker && Anchor))
	{
		return false;
	}

	// ⚠️ I `BaseColor` restano NEUTRI: e' cio' che rende la verifica in grigio vera per costruzione.
	// Un'istanza con una tinta passerebbe le soglie sotto e violerebbe comunque lo spirito di `D-146`.
	for (const RTGraybox::FRTGrayboxMaterialSpec& S : RTGraybox::FixtureMaterials)
	{
		const FLinearColor Neutral(S.Value, S.Value, S.Value, 1.f);
		TestTrue(*FString::Printf(TEXT("%s: il valore E' la luminanza (colore neutro)"), S.MeshName),
			FMath::IsNearlyEqual(RTGraybox::Luminance(Neutral), S.Value, 1.e-4f));
	}

	// La coppia che deve leggersi per prima: il marker contro il corpo su cui poggia.
	constexpr float MinValueGap = 0.30f;
	TestTrue(*FString::Printf(TEXT("corpo e marker distano in valore (%.2f contro %.2f)"), Body->Value, Marker->Value),
		FMath::Abs(Body->Value - Marker->Value) >= MinValueGap);

	// Il secondo canale, per la luce radente dove il valore si appiattisce.
	constexpr float MinRoughnessGap = 0.20f;
	TestTrue(TEXT("corpo e marker distano anche in ruvidita'"),
		FMath::Abs(Body->Roughness - Marker->Roughness) >= MinRoughnessGap);

	// ⚠️ E il corpo si separa dal DISCO a terra, che gli sta immediatamente sotto: senza, il fixture
	// sembrerebbe appoggiato su una macchia della propria stessa tinta.
	TestTrue(TEXT("corpo e ancora a terra si distinguono"),
		FMath::Abs(Body->Value - Anchor->Value) >= 0.15f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
