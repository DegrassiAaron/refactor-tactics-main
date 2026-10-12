#include "Content/RTHexVolumeMaterial.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "MaterialEditingLibrary.h"
#include "Map/RTLayerPresentation.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionPerInstanceCustomData.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTHexVolumeMaterial, Log, All);

const TCHAR* const RTHexVolumeMaterial::PinOwn = TEXT("Own");
const TCHAR* const RTHexVolumeMaterial::PinTint = TEXT("Tint");
const TCHAR* const RTHexVolumeMaterial::PinDesaturation = TEXT("Desaturation");
const TCHAR* const RTHexVolumeMaterial::PinExposureStops = TEXT("ExposureStops");
const TCHAR* const RTHexVolumeMaterial::PinTintStrength = TEXT("TintStrength");
const TCHAR* const RTHexVolumeMaterial::CustomDescription = TEXT("HexVolumeColor");
const TCHAR* const RTHexVolumeMaterial::DitherFunctionPath =
	TEXT("/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA.DitherTemporalAA");
const TCHAR* const RTHexVolumeMaterial::DitherInputName = TEXT("Alpha Threshold");

FString RTHexVolumeMaterial::CustomCode()
{
	FString Code;
	Code += TEXT("// Generato da RTHexVolumeMaterial::CustomCode (#3706). Non modificare nell'asset: cambia la ricetta e rilancia il commandlet.\n");
	Code += TEXT("// 1. Il trattamento di D-501 sul colore proprio: desaturazione, poi luce in stop.\n");
	Code += TEXT("float Luma = dot(Own.rgb, float3(0.3, 0.59, 0.11));\n");
	Code += TEXT("float3 Treated = lerp(Own.rgb, float3(Luma, Luma, Luma), saturate(Desaturation));\n");
	Code += TEXT("Treated *= exp2(ExposureStops);\n");
	Code += TEXT("// 2. La tinta del piano, DOPO: e' il colore che dice la relazione, e la desaturazione lo cancellerebbe.\n");
	Code += TEXT("return saturate(lerp(Treated, Tint.rgb, saturate(TintStrength)));");
	return Code;
}

namespace
{
	bool SaveVolumeMaterialPackage(UObject* Asset)
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

	/** Un dato per istanza scalare, neutro a zero: un'istanza appena nata non ha ne' tinta ne' trattamento. */
	UMaterialExpressionPerInstanceCustomData* AddChannel(UMaterial* Material, int32 Index, int32 Y)
	{
		UMaterialExpressionPerInstanceCustomData* Node = Cast<UMaterialExpressionPerInstanceCustomData>(
			UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionPerInstanceCustomData::StaticClass(), -900, Y));
		if (Node)
		{
			Node->DataIndex = static_cast<uint32>(Index);
			Node->ConstDefaultValue = 0.f;
		}
		return Node;
	}
}

bool RTHexVolumeMaterial::BuildGraph(UMaterial* Material)
{
	if (!Material)
	{
		return false;
	}

	// Il grafo si RICOSTRUISCE a ogni esecuzione (`D-229`): un nodo autorato a mano qui andrebbe perso, ed e' voluto.
	UMaterialEditingLibrary::DeleteAllMaterialExpressions(Material);

	Material->MaterialDomain = MD_Surface;
	// `Masked` e non traslucido, per la ragione scritta per `M_HexCell` in D-501: il retino non ordina la traslucenza e
	// lascia profondita' e ombre come sono.
	Material->BlendMode = BLEND_Masked;
	// ⛔ Senza questo flag, su una mesh istanziata il pacchetto disegnerebbe il materiale di default: in Editor lo
	// accenderebbe il motore da solo al primo uso, e il difetto si vedrebbe soltanto nel pacchetto.
	bool bNeedsRecompile = false;
	UMaterialEditingLibrary::SetMaterialUsage(Material, MATUSAGE_InstancedStaticMeshes, bNeedsRecompile);

	const int32 Primo = RTLayerPresentation::FirstChannel;

	UMaterialExpressionVectorParameter* Own = Cast<UMaterialExpressionVectorParameter>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionVectorParameter::StaticClass(), -900, -400));
	UMaterialExpressionPerInstanceCustomData3Vector* Tint = Cast<UMaterialExpressionPerInstanceCustomData3Vector>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionPerInstanceCustomData3Vector::StaticClass(), -900, -200));
	UMaterialExpressionPerInstanceCustomData* Transparency = AddChannel(Material, Primo + 0, 0);
	UMaterialExpressionPerInstanceCustomData* Desaturation = AddChannel(Material, Primo + 1, 200);
	UMaterialExpressionPerInstanceCustomData* Exposure = AddChannel(Material, Primo + 2, 400);
	UMaterialExpressionPerInstanceCustomData* Strength = AddChannel(Material, RTLayerPresentation::VolumeTintStrengthChannel, 600);
	UMaterialExpressionScalarParameter* Roughness = Cast<UMaterialExpressionScalarParameter>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionScalarParameter::StaticClass(), -900, 800));
	UMaterialExpressionCustom* Custom = Cast<UMaterialExpressionCustom>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionCustom::StaticClass(), -400, 0));
	UMaterialExpressionOneMinus* Opacity = Cast<UMaterialExpressionOneMinus>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionOneMinus::StaticClass(), -600, 1000));
	UMaterialExpressionMaterialFunctionCall* Dither = Cast<UMaterialExpressionMaterialFunctionCall>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionMaterialFunctionCall::StaticClass(), -400, 1000));

	if (!Own || !Tint || !Transparency || !Desaturation || !Exposure || !Strength || !Roughness || !Custom || !Opacity || !Dither)
	{
		UE_LOG(LogRTHexVolumeMaterial, Error, TEXT("le espressioni del materiale non sono nate."));
		return false;
	}

	Own->ParameterName = RTLayerPresentation::VolumeParamOwnColor;
	// Il default e' il colore proprio dei PRISMI; i pannelli di bordo lo sovrascrivono con una istanza dinamica.
	Own->DefaultValue = RTLayerPresentation::VolumeOwnColor(/*bEdgePanel=*/ false);

	Tint->DataIndex = 0;
	Tint->ConstDefaultValue = FLinearColor::Black;

	Roughness->ParameterName = RTLayerPresentation::VolumeParamRoughness;
	Roughness->DefaultValue = RTLayerPresentation::VolumeRoughness;

	Custom->Description = CustomDescription;
	Custom->OutputType = CMOT_Float3;
	Custom->Code = CustomCode();
	struct FPin { const TCHAR* Name; UMaterialExpression* From; };
	const FPin Pins[] = {
		{ PinOwn, Own },
		{ PinTint, Tint },
		{ PinDesaturation, Desaturation },
		{ PinExposureStops, Exposure },
		{ PinTintStrength, Strength },
	};
	Custom->Inputs.Reset();
	for (const FPin& Pin : Pins)
	{
		FCustomInput Input;
		Input.InputName = FName(Pin.Name);
		Input.Input.Connect(0, Pin.From);
		Custom->Inputs.Add(Input);
	}

	// La maschera: `DitherTemporalAA(1 - trasparenza)`, come `M_HexCell`. A trasparenza zero la maschera vale 1 e il
	// volume e' pieno; a 0,65 ne restano il 35% dei pixel, e l'antialiasing temporale li fonde.
	Opacity->Input.Connect(0, Transparency);
	UMaterialFunctionInterface* DitherFunction = LoadObject<UMaterialFunctionInterface>(nullptr, DitherFunctionPath);
	if (!DitherFunction)
	{
		UE_LOG(LogRTHexVolumeMaterial, Error, TEXT("la funzione %s non si carica."), DitherFunctionPath);
		return false;
	}
	Dither->SetMaterialFunction(DitherFunction);
	FFunctionExpressionInput* Soglia = Dither->FunctionInputs.FindByPredicate(
		[](const FFunctionExpressionInput& In) { return In.Input.InputName == FName(DitherInputName); });
	if (!Soglia)
	{
		// Si nominano gli ingressi che ci sono: chi legge il log capisce subito se il motore ha rinominato il pin.
		FString Trovati;
		for (const FFunctionExpressionInput& In : Dither->FunctionInputs)
		{
			Trovati += FString::Printf(TEXT("%s'%s'"), Trovati.IsEmpty() ? TEXT("") : TEXT(", "), *In.Input.InputName.ToString());
		}
		UE_LOG(LogRTHexVolumeMaterial, Error, TEXT("la funzione del retino non ha l'ingresso '%s'. Ingressi trovati: %s"),
			DitherInputName, *Trovati);
		return false;
	}
	Soglia->Input.Connect(0, Opacity);

	UMaterialEditingLibrary::ConnectMaterialProperty(Custom, TEXT(""), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Roughness, TEXT(""), MP_Roughness);
	UMaterialEditingLibrary::ConnectMaterialProperty(Dither, TEXT(""), MP_OpacityMask);

	// 🔴 `ConnectMaterialProperty` collega e NON spegne la costante in linea del pin: con `UseConstant` acceso il
	// traduttore compila la costante e il nodo collegato non viene letto, senza errori. E' il difetto che ha tenuto
	// opachi i piani sopra da #3679 a #3704 (`M_HexCell`), ed era proprio sulla maschera. Qui si spegne su tutti e tre.
	if (UMaterialEditorOnlyData* Dati = Material->GetEditorOnlyData())
	{
		Dati->BaseColor.UseConstant = false;
		Dati->Roughness.UseConstant = false;
		Dati->OpacityMask.UseConstant = false;
	}

	UMaterialEditingLibrary::LayoutMaterialExpressions(Material);
	// `PostEditChange` ricompila. ⛔ NON `RecompileMaterial`: senza un renderer va in crash (vedi `RTUnitOutlineMaterial`).
	Material->PostEditChange();
	Material->MarkPackageDirty();
	return true;
}

int32 RTHexVolumeMaterial::Build(const bool bDryRun)
{
	const FString PackageName = RTLayerPresentation::VolumeMaterialPackage;
	const FString AssetName = RTLayerPresentation::VolumeMaterialName;
	const FLinearColor Prismi = RTLayerPresentation::VolumeOwnColor(false);
	const FLinearColor Pannelli = RTLayerPresentation::VolumeOwnColor(true);

	UE_LOG(LogRTHexVolumeMaterial, Display, TEXT("%s -> %s%s"), *AssetName, *PackageName, bDryRun ? TEXT("  [DryRun]") : TEXT(""));
	UE_LOG(LogRTHexVolumeMaterial, Display,
		TEXT("colore proprio: prismi %.3f, pannelli %.3f (istanza dinamica); ruvidita' %.2f; dati per istanza: %d"),
		Prismi.R, Pannelli.R, RTLayerPresentation::VolumeRoughness, RTLayerPresentation::VolumeNumCustomDataFloats);

	if (bDryRun)
	{
		UE_LOG(LogRTHexVolumeMaterial, Display, TEXT("codice del nodo Custom:\n%s"), *CustomCode());
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
		UE_LOG(LogRTHexVolumeMaterial, Error, TEXT("%s: il grafo non e' stato costruito."), *AssetName);
		return 1;
	}
	if (bCreated)
	{
		FAssetRegistryModule::AssetCreated(Material);
	}
	if (!SaveVolumeMaterialPackage(Material))
	{
		UE_LOG(LogRTHexVolumeMaterial, Error, TEXT("%s non e' stato salvato."), *AssetName);
		return 1;
	}
	UE_LOG(LogRTHexVolumeMaterial, Display, TEXT("%s salvato (%s)."), *AssetName, bCreated ? TEXT("nuovo") : TEXT("ricostruito"));
	return 0;
}
