#include "Content/RTBuildGrayboxMeshesCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSourceData.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "StaticMeshAttributes.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTGrayboxMeshes, Log, All);

namespace
{
	/** Lo stesso valore che `RTHexLibrary.cpp` dichiara: la geometria non tollera un secondo arrotondamento. */
	constexpr double RT_SQRT3 = 1.7320508075688772;

	/**
	 * I due denominatori del contratto (§6), letti dal CDO invece che ricopiati.
	 *
	 * 🔴 Ricopiare `150` e `250` qui sarebbe la terza copia dello stesso numero, e il repository ha gia'
	 * pagato quel difetto: `RTMapVisuals.h` esiste perche' le quote del disco vivevano in un namespace
	 * anonimo e chi doveva posarci sopra qualcosa le riscriveva a mano.
	 */
	struct FRTGrayboxBudget
	{
		/** Lato dell'esagono = circumraggio. E' il denominatore degli elementi di BORDO. */
		float Side = 0.f;

		/** Altezza del volume-cella. E' il denominatore delle ALTEZZE (`D-168`). */
		float H = 0.f;

		/** Passo centro-centro: `C = sqrt(3) * lato`. Denominatore di ingombri e inset. */
		float C() const { return static_cast<float>(RT_SQRT3) * Side; }

		static FRTGrayboxBudget Canonical()
		{
			const URTHexMapAsset* Defaults = GetDefault<URTHexMapAsset>();
			FRTGrayboxBudget Budget;
			Budget.Side = Defaults->HexSize;
			Budget.H = Defaults->LayerHeight;
			return Budget;
		}
	};

	/**
	 * Aggiunge un parallelepipedo con NORMALI PER FACCIA e UV planari.
	 *
	 * 🔴 Le normali si scrivono, non si sperano: `GetCellPrismMesh` tocca i soli `GetVertexPositions()`
	 * e vive di rendita perche' la board e' giudicata per colore, non per ombreggiatura. Queste mesh si
	 * guardano ILLUMINATE e in scala di grigi (`U25` e' `unblocked_by: [U21]`), e una faccia senza
	 * normale non fa ombra — il canale su cui si legge la silhouette sparirebbe prima di essere giudicato.
	 *
	 * Convenzione degli assi, ereditata da `EdgeRotation` e dai pannelli di `#712`:
	 * X = SPESSORE (punta al vicino) · Y = lunghezza lungo il bordo · Z = altezza dalla base.
	 */
	void AppendBox(
		FMeshDescription& Description,
		FStaticMeshAttributes& Attributes,
		const FPolygonGroupID Group,
		const FVector3f& Min,
		const FVector3f& Max)
	{
		TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();

		// Gli otto angoli, indicizzati per bit: X basso/alto, Y basso/alto, Z basso/alto.
		FVertexID Corners[8];
		for (int32 Index = 0; Index < 8; ++Index)
		{
			const FVector3f Position(
				(Index & 1) ? Max.X : Min.X,
				(Index & 2) ? Max.Y : Min.Y,
				(Index & 4) ? Max.Z : Min.Z);

			Corners[Index] = Description.CreateVertex();
			Positions[Corners[Index]] = Position;
		}

		// Sei facce in ordine antiorario vista da fuori, con la normale che ne discende.
		struct FFace
		{
			int32 A, B, C, D;
			FVector3f Normal;
		};
		const FFace Faces[6] = {
			{ 1, 3, 7, 5, FVector3f( 1.f,  0.f,  0.f) },  // +X
			{ 2, 0, 4, 6, FVector3f(-1.f,  0.f,  0.f) },  // -X
			{ 3, 2, 6, 7, FVector3f( 0.f,  1.f,  0.f) },  // +Y
			{ 0, 1, 5, 4, FVector3f( 0.f, -1.f,  0.f) },  // -Y
			{ 4, 5, 7, 6, FVector3f( 0.f,  0.f,  1.f) },  // +Z
			{ 2, 3, 1, 0, FVector3f( 0.f,  0.f, -1.f) },  // -Z
		};

		// Un texel ogni 100 uu: a graybox l'UV serve a non lasciare il materiale senza coordinate, non a
		// reggere una texture autorata.
		constexpr float UvScale = 0.01f;

		for (const FFace& Face : Faces)
		{
			const int32 Indices[4] = { Face.A, Face.B, Face.C, Face.D };
			TArray<FVertexInstanceID> Instances;
			Instances.Reserve(4);

			for (const int32 Index : Indices)
			{
				const FVertexInstanceID Instance = Description.CreateVertexInstance(Corners[Index]);
				Normals[Instance] = Face.Normal;

				// Proiezione planare sull'asse meno significativo della faccia.
				const FVector3f& P = Positions[Corners[Index]];
				const FVector2f Uv = FMath::Abs(Face.Normal.X) > 0.5f ? FVector2f(P.Y, P.Z)
					: FMath::Abs(Face.Normal.Y) > 0.5f ? FVector2f(P.X, P.Z)
					: FVector2f(P.X, P.Y);
				UVs[Instance] = Uv * UvScale;

				Instances.Add(Instance);
			}

			Description.CreatePolygon(Group, Instances);
		}
	}

	/** Il gruppo unico di poligoni: una mesh graybox ha uno slot materiale e nessuna necessita' di piu'. */
	FPolygonGroupID BeginDescription(FMeshDescription& Description, FStaticMeshAttributes& Attributes)
	{
		Attributes.Register();
		const FPolygonGroupID Group = Description.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("Default");
		return Group;
	}

	/**
	 * Copertura bassa: pannello di bordo, `0.28 H` di altezza e `0.10` del lato di spessore (§6.3).
	 *
	 * ⚠️ Lo SPESSORE e' meta' di quello della copertura alta, e non e' rifinitura: due prismi con lo
	 * stesso spessore e due altezze diverse ripeterebbero `PIE-HEX-VIZ-BLOCCHI` (#1246), dove la
	 * differenza stava in un asse che la vista a picco proietta a zero.
	 */
	FMeshDescription BuildCoverLow(const FRTGrayboxBudget& Budget)
	{
		FMeshDescription Description;
		FStaticMeshAttributes Attributes(Description);
		const FPolygonGroupID Group = BeginDescription(Description, Attributes);

		const float HalfThickness = 0.10f * Budget.Side * 0.5f;
		const float HalfLength    = 0.92f * Budget.Side * 0.5f;
		const float Height        = 0.28f * Budget.H;

		// Pivot: centro del segmento, ALLA BASE — il pivot contract di §4 per un `EdgeBound`.
		AppendBox(Description, Attributes, Group,
			FVector3f(-HalfThickness, -HalfLength, 0.f),
			FVector3f( HalfThickness,  HalfLength, Height));

		return Description;
	}


	/**
	 * Aggiunge un prisma retto da un poligono in pianta, con normali per faccia.
	 *
	 * Il poligono si passa ANTIORARIO: e' l'ordine di `URTHexLibrary::HexCorners`, ed e' cio' che rende
	 * uscenti le normali dei fianchi senza doverle correggere caso per caso.
	 */
	void AppendPrism(
		FMeshDescription& Description,
		FStaticMeshAttributes& Attributes,
		const FPolygonGroupID Group,
		const TArray<FVector2f>& Plan,
		const float ZMin,
		const float ZMax)
	{
		if (Plan.Num() < 3)
		{
			return;
		}

		TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();

		constexpr float UvScale = 0.01f;

		TArray<FVertexID> Top;
		TArray<FVertexID> Bottom;
		Top.Reserve(Plan.Num());
		Bottom.Reserve(Plan.Num());
		for (const FVector2f& Point : Plan)
		{
			const FVertexID TopId = Description.CreateVertex();
			Positions[TopId] = FVector3f(Point.X, Point.Y, ZMax);
			Top.Add(TopId);

			const FVertexID BottomId = Description.CreateVertex();
			Positions[BottomId] = FVector3f(Point.X, Point.Y, ZMin);
			Bottom.Add(BottomId);
		}

		auto AddPolygon = [&](const TArray<FVertexID>& Ring, const FVector3f& Normal)
		{
			TArray<FVertexInstanceID> Instances;
			Instances.Reserve(Ring.Num());
			for (const FVertexID Vertex : Ring)
			{
				const FVertexInstanceID Instance = Description.CreateVertexInstance(Vertex);
				Normals[Instance] = Normal;

				const FVector3f& P = Positions[Vertex];
				const FVector2f Uv = FMath::Abs(Normal.Z) > 0.5f ? FVector2f(P.X, P.Y) : FVector2f(P.X + P.Y, P.Z);
				UVs[Instance] = Uv * UvScale;

				Instances.Add(Instance);
			}
			Description.CreatePolygon(Group, Instances);
		};

		AddPolygon(Top, FVector3f(0.f, 0.f, 1.f));

		TArray<FVertexID> BottomReversed = Bottom;
		Algo::Reverse(BottomReversed);
		AddPolygon(BottomReversed, FVector3f(0.f, 0.f, -1.f));

		for (int32 Index = 0; Index < Plan.Num(); ++Index)
		{
			const int32 Next = (Index + 1) % Plan.Num();
			const FVector2f Edge = Plan[Next] - Plan[Index];
			const FVector3f Outward = FVector3f(Edge.Y, -Edge.X, 0.f).GetSafeNormal();
			AddPolygon(TArray<FVertexID>{ Bottom[Index], Bottom[Next], Top[Next], Top[Index] }, Outward);
		}
	}

	/** Il footprint esterno della cella, in pianta e centrato sull'origine: i vertici li da' la libreria. */
	TArray<FVector2f> CellPlan(const FRTGrayboxBudget& Budget)
	{
		TArray<FVector2f> Plan;
		for (const FVector& Corner : URTHexLibrary::HexCorners(FVector::ZeroVector, Budget.Side))
		{
			Plan.Add(FVector2f(static_cast<float>(Corner.X), static_cast<float>(Corner.Y)));
		}
		return Plan;
	}

	/** Copertura alta: `0.85 H`, e spessore `0.20` del lato — il DOPPIO della bassa (§6.3). */
	FMeshDescription BuildCoverHigh(const FRTGrayboxBudget& Budget)
	{
		FMeshDescription Description;
		FStaticMeshAttributes Attributes(Description);
		const FPolygonGroupID Group = BeginDescription(Description, Attributes);

		const float HalfThickness = 0.20f * Budget.Side * 0.5f;
		const float HalfLength    = 0.92f * Budget.Side * 0.5f;
		const float Height        = 0.85f * Budget.H;

		AppendBox(Description, Attributes, Group,
			FVector3f(-HalfThickness, -HalfLength, 0.f),
			FVector3f( HalfThickness,  HalfLength, Height));

		return Description;
	}

	/** Il pannello nudo della porta: largo `0.92` del lato come ogni pannello di bordo, alto `0.85 H`. */
	FMeshDescription BuildDoorPanel(const FRTGrayboxBudget& Budget)
	{
		FMeshDescription Description;
		FStaticMeshAttributes Attributes(Description);
		const FPolygonGroupID Group = BeginDescription(Description, Attributes);

		const float HalfThickness = 0.10f * Budget.Side * 0.5f;
		const float HalfLength    = 0.92f * Budget.Side * 0.5f;
		const float Height        = 0.85f * Budget.H;

		AppendBox(Description, Attributes, Group,
			FVector3f(-HalfThickness, -HalfLength, 0.f),
			FVector3f( HalfThickness,  HalfLength, Height));

		return Description;
	}

	/**
	 * `Locked`: lo stesso pannello PIU' la traversa in rilievo di `D-171`.
	 *
	 * ⚠️ Sporge su ENTRAMBE le facce, e discende da §3: un `EdgeBound` non appartiene a nessuna delle due
	 * celle che condividono il bordo, quindi si guarda da entrambi i lati. Un marcatore su una faccia sola
	 * sarebbe leggibile dalla meta' delle posizioni di camera, e `D-171` ha scelto la geometria proprio per
	 * non dipendere dal punto di vista.
	 */
	FMeshDescription BuildDoorLocked(const FRTGrayboxBudget& Budget)
	{
		FMeshDescription Description = BuildDoorPanel(Budget);
		FStaticMeshAttributes Attributes(Description);
		const FPolygonGroupID Group = Description.PolygonGroups().GetFirstValidID();

		const float HalfThickness = 0.10f * Budget.Side * 0.5f;
		const float Relief        = 0.06f * Budget.Side;
		const float HalfLength    = 0.92f * Budget.Side * 0.5f;
		const float Height        = 0.85f * Budget.H;
		const float HalfBand      = 0.12f * Budget.H * 0.5f;

		AppendBox(Description, Attributes, Group,
			FVector3f(-HalfThickness - Relief, -HalfLength, Height * 0.5f - HalfBand),
			FVector3f( HalfThickness + Relief,  HalfLength, Height * 0.5f + HalfBand));

		return Description;
	}

	/** Acqua: una lastra PIATTA e CONTINUA sul footprint esterno. E' il termine di paragone del ghiaccio. */
	FMeshDescription BuildSurfaceWater(const FRTGrayboxBudget& Budget)
	{
		FMeshDescription Description;
		FStaticMeshAttributes Attributes(Description);
		const FPolygonGroupID Group = BeginDescription(Description, Attributes);

		AppendPrism(Description, Attributes, Group, CellPlan(Budget), 0.f, 0.02f * Budget.H);
		return Description;
	}

	/**
	 * Ghiaccio: sei lastre triangolari a quote diverse dentro lo stesso `0.02 H`.
	 *
	 * Il canale e' la FRATTURA, non la tinta: in pianta si vedono le sei linee, e i dislivelli fra settori
	 * adiacenti sono pareti verticali che sotto la stessa luce hanno una luminanza diversa dalle facce
	 * orizzontali. 🔴 E' il budget che dipende dalle normali — se arrivassero degeneri, il ghiaccio sarebbe
	 * un'acqua con qualche linea, ed e' il motivo per cui `MeshesHaveFaceNormals` esiste.
	 *
	 * Le quote sono FISSE e non casuali: un generatore che sorteggiasse darebbe un asset diverso a ogni
	 * esecuzione, e due `.uasset` che si diffano sono meta' della ragione per cui `D-229` genera invece di
	 * far modellare.
	 */
	FMeshDescription BuildSurfaceIce(const FRTGrayboxBudget& Budget)
	{
		FMeshDescription Description;
		FStaticMeshAttributes Attributes(Description);
		const FPolygonGroupID Group = BeginDescription(Description, Attributes);

		const TArray<FVector2f> Plan = CellPlan(Budget);
		const float FullHeight = 0.02f * Budget.H;
		static const float SectorHeights[6] = { 1.00f, 0.55f, 0.85f, 0.40f, 0.70f, 0.25f };

		for (int32 Sector = 0; Sector < Plan.Num(); ++Sector)
		{
			const int32 Next = (Sector + 1) % Plan.Num();
			const TArray<FVector2f> Wedge = { FVector2f::ZeroVector, Plan[Sector], Plan[Next] };
			AppendPrism(Description, Attributes, Group, Wedge, 0.f, FullHeight * SectorHeights[Sector]);
		}

		return Description;
	}

	struct FRTGrayboxAsset
	{
		/** Nome dell'asset, senza prefisso di percorso. */
		const TCHAR* Name;

		/** Sottocartella di `D-173`: `Cover` · `Doors` · `Surfaces`. */
		const TCHAR* Folder;

		FMeshDescription (*Build)(const FRTGrayboxBudget&);
	};

	/**
	 * Le sei mesh di §8.1, e sono tutte.
	 *
	 * ⚠️ Per un tempo questa tabella ne ha avuta UNA, e non era incompletezza: `D-229` prescrive di
	 * generarne una sola, verificarne le normali e poi continuare — la verifica ha un esito
	 * (`RefactorTactics.Graybox.MeshesHaveFaceNormals`), quindi le altre cinque sono entrate.
	 */
	const FRTGrayboxAsset GGrayboxAssets[] = {
		{ TEXT("SM_Graybox_Cover_Low"),     TEXT("Cover"),    &BuildCoverLow     },
		{ TEXT("SM_Graybox_Cover_High"),    TEXT("Cover"),    &BuildCoverHigh    },
		{ TEXT("SM_Graybox_Door_Panel"),    TEXT("Doors"),    &BuildDoorPanel    },
		{ TEXT("SM_Graybox_Door_Locked"),   TEXT("Doors"),    &BuildDoorLocked   },
		{ TEXT("SM_Graybox_Surface_Water"), TEXT("Surfaces"), &BuildSurfaceWater },
		{ TEXT("SM_Graybox_Surface_Ice"),   TEXT("Surfaces"), &BuildSurfaceIce   },
	};

	bool SaveAssetPackage(UObject* Asset)
	{
		UPackage* Package = Asset ? Asset->GetOutermost() : nullptr;
		if (!Package)
		{
			return false;
		}

		const FString FileName = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());

		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Asset, *FileName, Args);
	}

	/** Crea l'asset con la sua MeshDescription SORGENTE, cosi' resta ri-generabile e ri-apribile. */
	UStaticMesh* CreateMeshAsset(const FString& PackageName, const FString& AssetName, FMeshDescription& Source)
	{
		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			return nullptr;
		}
		Package->FullyLoad();

		UStaticMesh* Mesh = NewObject<UStaticMesh>(Package, FName(*AssetName), RF_Public | RF_Standalone);
		Mesh->InitResources();
		Mesh->SetLightingGuid();

		FStaticMeshSourceModel& SourceModel = Mesh->AddSourceModel();
		// Le normali arrivano da `AppendBox` e non vanno ricalcolate: ricalcolarle fonderebbe gli spigoli
		// vivi di un box in una superficie morbida, cioe' cancellerebbe la silhouette che si va a guardare.
		SourceModel.BuildSettings.bRecomputeNormals = false;
		SourceModel.BuildSettings.bRecomputeTangents = true;
		SourceModel.BuildSettings.bGenerateLightmapUVs = true;
		SourceModel.BuildSettings.bUseHighPrecisionTangentBasis = false;

		if (FMeshDescription* Destination = Mesh->CreateMeshDescription(0))
		{
			*Destination = MoveTemp(Source);
			Mesh->CommitMeshDescription(0);
		}

		Mesh->GetStaticMaterials().Add(FStaticMaterial());
		Mesh->Build(false);
		Mesh->PostEditChange();
		Mesh->MarkPackageDirty();

		FAssetRegistryModule::AssetCreated(Mesh);
		return Mesh;
	}
}

URTBuildGrayboxMeshesCommandlet::URTBuildGrayboxMeshesCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URTBuildGrayboxMeshesCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> Options;
	ParseCommandLine(*Params, Tokens, Switches, Options);

	const bool bDryRun = Switches.Contains(TEXT("DryRun"));
	const FString Root = Options.Contains(TEXT("Package"))
		? Options[TEXT("Package")]
		: TEXT("/Game/RT/World/Graybox");
	const FString Only = Options.Contains(TEXT("Only")) ? Options[TEXT("Only")] : FString();

	const FRTGrayboxBudget Budget = FRTGrayboxBudget::Canonical();
	UE_LOG(LogRTGrayboxMeshes, Display,
		TEXT("Scala canonica: lato = %.1f uu, C = %.1f uu, H = %.1f uu%s"),
		Budget.Side, Budget.C(), Budget.H, bDryRun ? TEXT("  [DryRun]") : TEXT(""));

	if (Budget.Side <= 0.f || Budget.H <= 0.f)
	{
		UE_LOG(LogRTGrayboxMeshes, Error,
			TEXT("Il CDO di URTHexMapAsset da' una scala non utilizzabile (lato = %.1f, H = %.1f)."),
			Budget.Side, Budget.H);
		return 1;
	}

	int32 Written = 0;
	int32 Failed = 0;

	for (const FRTGrayboxAsset& Asset : GGrayboxAssets)
	{
		const FString Name(Asset.Name);
		if (!Only.IsEmpty() && !Name.Contains(Only))
		{
			continue;
		}

		const FString PackageName = FString::Printf(TEXT("%s/%s/%s"), *Root, Asset.Folder, *Name);

		FMeshDescription Description = Asset.Build(Budget);
		const int32 Triangles = Description.Triangles().Num();

		FBox3f Bounds(ForceInit);
		TVertexAttributesConstRef<FVector3f> Positions =
			FStaticMeshConstAttributes(Description).GetVertexPositions();
		for (const FVertexID Vertex : Description.Vertices().GetElementIDs())
		{
			Bounds += Positions[Vertex];
		}

		UE_LOG(LogRTGrayboxMeshes, Display,
			TEXT("%s -> %s | %d triangoli | ingombro %.1f x %.1f x %.1f uu"),
			*Name, *PackageName, Triangles,
			Bounds.GetSize().X, Bounds.GetSize().Y, Bounds.GetSize().Z);

		if (bDryRun)
		{
			continue;
		}

		UStaticMesh* Mesh = CreateMeshAsset(PackageName, Name, Description);
		if (Mesh && SaveAssetPackage(Mesh))
		{
			++Written;
		}
		else
		{
			++Failed;
			UE_LOG(LogRTGrayboxMeshes, Error, TEXT("%s non e' stato salvato."), *Name);
		}
	}

	UE_LOG(LogRTGrayboxMeshes, Display, TEXT("Scritti %d, falliti %d."), Written, Failed);
	return Failed > 0 ? 1 : 0;
}
