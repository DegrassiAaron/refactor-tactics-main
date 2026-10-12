#include "Content/RTUnitOutlineMaterial.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Unit/RTUnitLayerOutline.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTUnitOutlineMaterial, Log, All);

const TCHAR* const RTUnitOutlineMaterial::PinScene = TEXT("Scene");
const TCHAR* const RTUnitOutlineMaterial::PinStencil = TEXT("StencilTex");
const TCHAR* const RTUnitOutlineMaterial::PinUnitDepth = TEXT("UnitDepthTex");
const TCHAR* const RTUnitOutlineMaterial::PinSceneDepth = TEXT("SceneDepthTex");
const TCHAR* const RTUnitOutlineMaterial::CustomDescription = TEXT("UnitLayerOutline");

FString RTUnitOutlineMaterial::CustomCode()
{
	// I NOSTRI stencil sono un intervallo: da `Active` a `Below`. Uno stencil fuori da qui e' di qualcun altro, e il
	// materiale non lo tocca. I tre `static_assert` tengono vera la premessa su cui il codice qui sotto si regge.
	static_assert(RTUnitLayerOutline::StencilActive < RTUnitLayerOutline::StencilAbove
		&& RTUnitLayerOutline::StencilAbove < RTUnitLayerOutline::StencilBelow,
		"gli stencil del contorno sono un intervallo ordinato: il materiale ne controlla il primo e l'ultimo");
	static_assert(RTUnitLayerOutline::StencilNone < RTUnitLayerOutline::StencilActive,
		"lo stencil di nessun contorno sta fuori dall'intervallo");

	const int32 Active = RTUnitLayerOutline::StencilActive;
	const int32 Above = RTUnitLayerOutline::StencilAbove;
	const int32 Below = RTUnitLayerOutline::StencilBelow;

	// Quale stencil passa attraverso la geometria lo dice il runtime. Oggi e' uno solo; se diventassero di piu', la
	// condizione qui sotto li mette in OR invece di lasciarne indietro uno.
	FString Attraverso;
	for (const int32 Stencil : { Active, Above, Below })
	{
		if (RTUnitLayerOutline::ShowsThroughGeometry(static_cast<uint8>(Stencil)))
		{
			Attraverso += FString::Printf(TEXT("%sabs(S - %d.0) < 0.5"), Attraverso.IsEmpty() ? TEXT("") : TEXT(" || "), Stencil);
		}
	}
	if (Attraverso.IsEmpty())
	{
		Attraverso = TEXT("false");
	}

	// ⚠️ Nessuna istruzione con derivate: lo stencil si legge con `Load`, le profondita' con `SampleLevel`. Per questo
	// i `SceneTextureFetch` possono stare dentro cicli e rami senza che il compilatore li rifiuti.
	FString Code;
	Code += TEXT("// Generato da RTUnitOutlineMaterial::CustomCode (#3705). Non modificare nell'asset: cambia la ricetta e rilancia il commandlet.\n");
	Code += TEXT("float3 Result = Scene.rgb;\n");
	Code += TEXT("float Center = SceneTextureFetch(PPI_CustomStencil, float2(0.0, 0.0)).r;\n");
	Code += FString::Printf(TEXT("bool bInside = Center > %d.5 && Center < %d.5;\n"), Active - 1, Below);
	Code += TEXT("if (!bInside && ThicknessPx > 0.01)\n");
	Code += TEXT("{\n");
	Code += TEXT("	// Lo spessore e' in pixel a 1080 righe: si scala con l'altezza del quadro, e non scende sotto un pixel.\n");
	Code += TEXT("	float Radius = max(1.0, ThicknessPx * max(1.0, ResolvedView.ViewSizeAndInvSize.y / 1080.0));\n");
	Code += TEXT("	const float2 Dirs[8] = { float2(1.0, 0.0), float2(-1.0, 0.0), float2(0.0, 1.0), float2(0.0, -1.0),\n");
	Code += TEXT("		float2(0.7071, 0.7071), float2(-0.7071, 0.7071), float2(0.7071, -0.7071), float2(-0.7071, -0.7071) };\n");
	Code += TEXT("	float Found = 0.0;\n");
	Code += TEXT("	for (int Ring = 1; Ring <= 2; ++Ring)\n");
	Code += TEXT("	{\n");
	Code += TEXT("		for (int i = 0; i < 8; ++i)\n");
	Code += TEXT("		{\n");
	Code += TEXT("			float2 Off = round(Dirs[i] * (Radius * 0.5 * Ring));\n");
	Code += TEXT("			float S = SceneTextureFetch(PPI_CustomStencil, Off).r;\n");
	Code += FString::Printf(TEXT("			if (S > %d.5 && S < %d.5)\n"), Active - 1, Below);
	Code += TEXT("			{\n");
	Code += TEXT("				// Chi sta sotto il piano attivo si disegna anche dove la geometria lo copre; gli altri no.\n");
	Code += FString::Printf(TEXT("				bool bShown = %s;\n"), *Attraverso);
	Code += TEXT("				if (!bShown)\n");
	Code += TEXT("				{\n");
	Code += TEXT("					float UnitZ = SceneTextureFetch(PPI_CustomDepth, Off).r;\n");
	Code += TEXT("					float SceneZ = SceneTextureFetch(PPI_SceneDepth, Off).r;\n");
	Code += TEXT("					bShown = UnitZ <= SceneZ + max(2.0, SceneZ * 0.002);\n");
	Code += TEXT("				}\n");
	Code += TEXT("				Found = bShown ? max(Found, S) : Found;\n");
	Code += TEXT("			}\n");
	Code += TEXT("		}\n");
	Code += TEXT("	}\n");
	Code += FString::Printf(TEXT("	if (abs(Found - %d.0) < 0.5) { Result = ColorActive.rgb; }\n"), Active);
	Code += FString::Printf(TEXT("	else if (abs(Found - %d.0) < 0.5) { Result = ColorAbove.rgb; }\n"), Above);
	Code += FString::Printf(TEXT("	else if (abs(Found - %d.0) < 0.5) { Result = ColorBelow.rgb; }\n"), Below);
	Code += TEXT("}\n");
	Code += TEXT("return Result;");
	return Code;
}

namespace
{
	/** Salva il package di un asset appena creato o modificato. */
	bool SaveOutlineMaterialPackage(UObject* Asset)
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

	UMaterialExpressionSceneTexture* AddSceneTexture(UMaterial* Material, ESceneTextureId Id, int32 Y)
	{
		UMaterialExpressionSceneTexture* Node = Cast<UMaterialExpressionSceneTexture>(
			UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionSceneTexture::StaticClass(), -900, Y));
		if (Node)
		{
			Node->SceneTextureId = Id;
		}
		return Node;
	}

	UMaterialExpressionVectorParameter* AddColor(UMaterial* Material, const FName Name, const uint8 Stencil, int32 Y)
	{
		UMaterialExpressionVectorParameter* Node = Cast<UMaterialExpressionVectorParameter>(
			UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionVectorParameter::StaticClass(), -900, Y));
		if (Node)
		{
			Node->ParameterName = Name;
			// Il default e' il colore della regola: un'istanza che non scrivesse niente resterebbe giusta. Chi lo scrive
			// davvero e' la camera, dalla stessa `BorderColorFor`.
			FColor Colore(255, 0, 255);
			RTUnitLayerOutline::ColorFor(Stencil, Colore);
			Node->DefaultValue = FLinearColor::FromSRGBColor(Colore);
		}
		return Node;
	}
}

bool RTUnitOutlineMaterial::BuildGraph(UMaterial* Material)
{
	if (!Material)
	{
		return false;
	}

	// Il grafo si RICOSTRUISCE a ogni esecuzione (`D-229`): un nodo autorato a mano qui andrebbe perso, ed e' voluto.
	UMaterialEditingLibrary::DeleteAllMaterialExpressions(Material);

	Material->MaterialDomain = MD_PostProcess;
	// Dopo il DOF e PRIMA dell'antialiasing temporale: il contorno nasce a spigolo vivo sul custom depth, che ha lo
	// stesso jitter della scena, e l'antialiasing lo ammorbidisce insieme al resto. Dopo il tonemapper tremerebbe.
	// E i colori restano nello spazio della scena: il bianco del contorno passa dallo stesso tonemapper del bianco
	// dei bordi delle celle, quindi si leggono uguali.
	Material->BlendableLocation = BL_SceneColorAfterDOF;
	Material->BlendablePriority = 0;
	Material->bIsBlendable = true;

	UMaterialExpressionSceneTexture* Scene = AddSceneTexture(Material, PPI_PostProcessInput0, -300);
	UMaterialExpressionSceneTexture* Stencil = AddSceneTexture(Material, PPI_CustomStencil, -100);
	UMaterialExpressionSceneTexture* UnitDepth = AddSceneTexture(Material, PPI_CustomDepth, 100);
	UMaterialExpressionSceneTexture* SceneDepth = AddSceneTexture(Material, PPI_SceneDepth, 300);

	UMaterialExpressionVectorParameter* ColorActive =
		AddColor(Material, RTUnitLayerOutline::ParamColorActive, RTUnitLayerOutline::StencilActive, 500);
	UMaterialExpressionVectorParameter* ColorAbove =
		AddColor(Material, RTUnitLayerOutline::ParamColorAbove, RTUnitLayerOutline::StencilAbove, 700);
	UMaterialExpressionVectorParameter* ColorBelow =
		AddColor(Material, RTUnitLayerOutline::ParamColorBelow, RTUnitLayerOutline::StencilBelow, 900);

	UMaterialExpressionScalarParameter* Thickness = Cast<UMaterialExpressionScalarParameter>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionScalarParameter::StaticClass(), -900, 1100));
	UMaterialExpressionCustom* Custom = Cast<UMaterialExpressionCustom>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionCustom::StaticClass(), -400, 300));

	if (!Scene || !Stencil || !UnitDepth || !SceneDepth || !ColorActive || !ColorAbove || !ColorBelow || !Thickness || !Custom)
	{
		UE_LOG(LogRTUnitOutlineMaterial, Error, TEXT("le espressioni del materiale non sono nate."));
		return false;
	}

	Thickness->ParameterName = RTUnitLayerOutline::ParamThickness;
	Thickness->DefaultValue = RTUnitLayerOutline::DefaultThicknessPx;

	Custom->Description = CustomDescription;
	Custom->OutputType = CMOT_Float3;
	Custom->Code = CustomCode();

	// ⚠️ Le tre scene texture dopo `Scene` NON sono lette dal codice come pin: servono perche' il materiale le
	// dichiari, altrimenti `SceneTextureFetch` leggerebbe una texture che nessuno ha legato. Il compilatore del nodo
	// Custom lo prevede (un pin scene texture non usato si compila senza il suo campionamento).
	struct FPin { const TCHAR* Name; UMaterialExpression* From; };
	const FPin Pins[] = {
		{ PinScene, Scene },
		{ PinStencil, Stencil },
		{ PinUnitDepth, UnitDepth },
		{ PinSceneDepth, SceneDepth },
		{ TEXT("ColorActive"), ColorActive },
		{ TEXT("ColorAbove"), ColorAbove },
		{ TEXT("ColorBelow"), ColorBelow },
		{ TEXT("ThicknessPx"), Thickness },
	};
	Custom->Inputs.Reset();
	for (const FPin& Pin : Pins)
	{
		FCustomInput Input;
		Input.InputName = FName(Pin.Name);
		Input.Input.Connect(0, Pin.From);
		Custom->Inputs.Add(Input);
	}

	UMaterialEditingLibrary::ConnectMaterialProperty(Custom, TEXT(""), MP_EmissiveColor);

	// 🔴 `ConnectMaterialProperty` collega e NON spegne la costante in linea del pin: con `UseConstant` acceso il
	// traduttore compila la costante e il nodo collegato non viene letto, senza errori. E' il difetto che ha tenuto
	// opachi i piani sopra da #3679 a #3704 (`M_HexCell`). Qui si spegne esplicitamente, e il test lo guarda.
	if (UMaterialEditorOnlyData* Dati = Material->GetEditorOnlyData())
	{
		Dati->EmissiveColor.UseConstant = false;
	}

	UMaterialEditingLibrary::LayoutMaterialExpressions(Material);
	// `PostEditChange` ricompila il materiale. ⛔ NON `UMaterialEditingLibrary::RecompileMaterial`: quella ricostruisce
	// anche i dati di streaming delle texture compilando gli shader di debug, e senza un renderer va in crash
	// (`BuildTextureStreamingData`, misurato sotto `-NullRHI` il 2026-10-12). Questo materiale non ha texture.
	Material->PostEditChange();
	Material->MarkPackageDirty();
	return true;
}

int32 RTUnitOutlineMaterial::Build(const bool bDryRun)
{
	const FString PackageName = RTUnitLayerOutline::MaterialPackage;
	const FString AssetName = RTUnitLayerOutline::MaterialName;

	UE_LOG(LogRTUnitOutlineMaterial, Display, TEXT("%s -> %s%s"), *AssetName, *PackageName, bDryRun ? TEXT("  [DryRun]") : TEXT(""));
	UE_LOG(LogRTUnitOutlineMaterial, Display, TEXT("stencil: attivo %d, sopra %d, sotto %d; attraverso la geometria: %s"),
		RTUnitLayerOutline::StencilActive, RTUnitLayerOutline::StencilAbove, RTUnitLayerOutline::StencilBelow,
		RTUnitLayerOutline::ShowsThroughGeometry(RTUnitLayerOutline::StencilBelow) ? TEXT("solo sotto") : TEXT("nessuno"));

	if (bDryRun)
	{
		UE_LOG(LogRTUnitOutlineMaterial, Display, TEXT("codice del nodo Custom:\n%s"), *CustomCode());
		return 0;
	}

	UMaterial* Material = LoadObject<UMaterial>(nullptr, *(PackageName + TEXT(".") + AssetName), nullptr, LOAD_NoWarn);
	bool bCreated = false;
	if (!Material)
	{
		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			return 1;
		}
		Package->FullyLoad();
		Material = NewObject<UMaterial>(Package, FName(*AssetName), RF_Public | RF_Standalone);
		bCreated = true;
	}

	if (!BuildGraph(Material))
	{
		UE_LOG(LogRTUnitOutlineMaterial, Error, TEXT("%s: il grafo non e' stato costruito."), *AssetName);
		return 1;
	}
	if (bCreated)
	{
		FAssetRegistryModule::AssetCreated(Material);
	}
	if (!SaveOutlineMaterialPackage(Material))
	{
		UE_LOG(LogRTUnitOutlineMaterial, Error, TEXT("%s non e' stato salvato."), *AssetName);
		return 1;
	}
	UE_LOG(LogRTUnitOutlineMaterial, Display, TEXT("%s salvato (%s)."), *AssetName, bCreated ? TEXT("nuovo") : TEXT("ricostruito"));
	return 0;
}
