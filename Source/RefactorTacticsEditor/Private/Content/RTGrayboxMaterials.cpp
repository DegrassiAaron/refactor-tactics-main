#include "Content/RTGrayboxMaterials.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTGrayboxMaterials, Log, All);

namespace RTGraybox
{
	const FName ParamBaseColor(TEXT("BaseColor"));
	const FName ParamRoughness(TEXT("Roughness"));

	const TCHAR* MasterAssetName = TEXT("M_Graybox_Master");
	const TCHAR* MaterialsFolder = TEXT("Materials");

	/**
	 * La grammatica del kit, in due colonne.
	 *
	 * **Valore** separa cio' che si legge a colpo d'occhio; **ruvidita'** separa cio' che il valore non
	 * basta a separare, e sopravvive alla scala di grigi perche' un highlight speculare e' luminanza.
	 *
	 * 🔴 **Il caso che ha deciso questa tabella e' `Door_Panel` contro `Cover_High`**, ed e' misurato:
	 * `RTBuildGrayboxMeshesCommandlet` costruisce entrambe alte `0.85 H` e lunghe `0.92` del lato — la
	 * sola differenza geometrica e' lo SPESSORE (`0.10` contro `0.20`), che la vista a picco proietta
	 * quasi a zero. E' la stessa forma del difetto che ha reso ❌ `PIE-HEX-VIZ-BLOCCHI` (#1246). Qui le
	 * separano DUE canali indipendenti: il valore (`0.30` contro `0.14`) e la ruvidita' (`0.20` contro
	 * `0.90`) — una porta e' una superficie lavorata, una copertura e' un blocco grezzo.
	 *
	 * ⛔ **`Door_Locked` non e' separata dal materiale, e non e' una svista.** `D-171` ha deciso che il
	 * canale fra `Closed` e `Locked` e' la TRAVERSA IN RILIEVO, cioe' geometria; il materiale «puo'
	 * rinforzare, mai rimpiazzare» (`#1714`). Il valore piu' basso e' quel rinforzo, e resta abbastanza
	 * vicino da non poter fare da canale primario: chi togliesse la traversa non otterrebbe due porte
	 * distinguibili, otterrebbe due porte quasi uguali — e `Graybox.LockedDoorCarriesTraverse` fallirebbe
	 * comunque.
	 *
	 * 🔴 **`0.25` era `0.20` nella prima stesura, e il test lo ha respinto: `|0.30 - 0.20|` vale
	 * ESATTAMENTE la soglia di separazione, e un valore sul confine non e' un design — e' un'ambiguita'
	 * che il primo che rilegge la tabella scioglie a caso.** La correzione e' stata sul DATO e non
	 * sull'asserzione: allentare la soglia a `<=` avrebbe fatto passare il test lasciando `Locked` sul
	 * confine, cioe' avrebbe cancellato la misura invece del difetto. A `0.25` il rinforzo vale meta'
	 * soglia, e resta comunque distinguibile da `Cover_High` (`0.25` contro `0.14`).
	 */
	const FRTGrayboxMaterialSpec KitMaterials[6] = {
		//  mesh                          istanza                            valore  ruvidita'
		{ TEXT("SM_Graybox_Cover_Low"),     TEXT("MI_Graybox_Cover_Low"),     0.45f,  0.90f },
		{ TEXT("SM_Graybox_Cover_High"),    TEXT("MI_Graybox_Cover_High"),    0.14f,  0.90f },
		{ TEXT("SM_Graybox_Door_Panel"),    TEXT("MI_Graybox_Door_Panel"),    0.30f,  0.20f },
		{ TEXT("SM_Graybox_Door_Locked"),   TEXT("MI_Graybox_Door_Locked"),   0.25f,  0.20f },
		{ TEXT("SM_Graybox_Surface_Water"), TEXT("MI_Graybox_Surface_Water"), 0.05f,  0.02f },
		{ TEXT("SM_Graybox_Surface_Ice"),   TEXT("MI_Graybox_Surface_Ice"),   0.65f,  0.55f },
	};

	float Luminance(const FLinearColor& Color)
	{
		return 0.2126f * Color.R + 0.7152f * Color.G + 0.0722f * Color.B;
	}
}

namespace
{
	/** Salva il package di un asset appena creato o modificato. */
	bool SaveMaterialPackage(UObject* Asset)
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

	/**
	 * Il master, con i suoi DUE parametri collegati.
	 *
	 * Il graph si RICOSTRUISCE a ogni esecuzione — `DeleteAllMaterialExpressions` e poi da capo — perche'
	 * la sorgente e' questo codice e l'asset e' il suo output (`D-229`). Se un giorno il master avesse un
	 * nodo autorato a mano, lo perderebbe qui: e' la stessa proprieta' per cui le sei mesh si rigenerano
	 * invece di fondersi.
	 */
	UMaterial* EnsureMaster(const FString& PackageName, const FString& AssetName)
	{
		UMaterial* Master = LoadObject<UMaterial>(nullptr, *(PackageName + TEXT(".") + AssetName));
		bool bCreated = false;

		if (!Master)
		{
			UPackage* Package = CreatePackage(*PackageName);
			if (!Package)
			{
				return nullptr;
			}
			Package->FullyLoad();

			Master = NewObject<UMaterial>(Package, FName(*AssetName), RF_Public | RF_Standalone);
			bCreated = true;
		}

		if (!Master)
		{
			return nullptr;
		}

		UMaterialEditingLibrary::DeleteAllMaterialExpressions(Master);

		UMaterialExpressionVectorParameter* BaseColor = Cast<UMaterialExpressionVectorParameter>(
			UMaterialEditingLibrary::CreateMaterialExpression(
				Master, UMaterialExpressionVectorParameter::StaticClass(), -400, 0));
		UMaterialExpressionScalarParameter* Roughness = Cast<UMaterialExpressionScalarParameter>(
			UMaterialEditingLibrary::CreateMaterialExpression(
				Master, UMaterialExpressionScalarParameter::StaticClass(), -400, 200));

		if (!BaseColor || !Roughness)
		{
			UE_LOG(LogRTGrayboxMaterials, Error, TEXT("%s: le espressioni del master non sono nate."), *AssetName);
			return nullptr;
		}

		BaseColor->ParameterName = RTGraybox::ParamBaseColor;
		// Il default e' il grigio medio del kit: un'istanza che non sovrascrivesse nulla resterebbe
		// leggibile invece di diventare nera, ed e' cio' che rende innocuo dimenticare un override.
		BaseColor->DefaultValue = FLinearColor(0.30f, 0.30f, 0.30f, 1.f);

		Roughness->ParameterName = RTGraybox::ParamRoughness;
		Roughness->DefaultValue = 0.80f;

		UMaterialEditingLibrary::ConnectMaterialProperty(BaseColor, TEXT(""), MP_BaseColor);
		UMaterialEditingLibrary::ConnectMaterialProperty(Roughness, TEXT(""), MP_Roughness);

		UMaterialEditingLibrary::LayoutMaterialExpressions(Master);
		Master->PostEditChange();
		UMaterialEditingLibrary::RecompileMaterial(Master);
		Master->MarkPackageDirty();

		if (bCreated)
		{
			FAssetRegistryModule::AssetCreated(Master);
		}
		return Master;
	}

	/** Una istanza, col suo parent e i suoi due override. */
	UMaterialInstanceConstant* EnsureInstance(
		const FString& PackageName,
		const FString& AssetName,
		UMaterial* Master,
		const RTGraybox::FRTGrayboxMaterialSpec& Spec)
	{
		UMaterialInstanceConstant* Instance =
			LoadObject<UMaterialInstanceConstant>(nullptr, *(PackageName + TEXT(".") + AssetName));
		bool bCreated = false;

		if (!Instance)
		{
			UPackage* Package = CreatePackage(*PackageName);
			if (!Package)
			{
				return nullptr;
			}
			Package->FullyLoad();

			Instance = NewObject<UMaterialInstanceConstant>(Package, FName(*AssetName), RF_Public | RF_Standalone);
			bCreated = true;
		}

		if (!Instance)
		{
			return nullptr;
		}

		UMaterialEditingLibrary::SetMaterialInstanceParent(Instance, Master);

		// Il colore e' NEUTRO per costruzione: il valore va nei tre canali insieme, e non c'e' una tinta
		// da cui una categoria possa dipendere (`D-146`).
		const FLinearColor Neutral(Spec.Value, Spec.Value, Spec.Value, 1.f);
		UMaterialEditingLibrary::SetMaterialInstanceVectorParameterValue(Instance, RTGraybox::ParamBaseColor, Neutral);
		UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(Instance, RTGraybox::ParamRoughness, Spec.Roughness);

		Instance->PostEditChange();
		Instance->MarkPackageDirty();

		if (bCreated)
		{
			FAssetRegistryModule::AssetCreated(Instance);
		}
		return Instance;
	}
}

int32 RTGraybox::BuildKitMaterials(
	const FString& PackageRoot,
	const bool bDryRun,
	TMap<FString, UMaterialInterface*>& OutInstances)
{
	const FString MasterPackage = FString::Printf(TEXT("%s/%s/%s"), *PackageRoot, MaterialsFolder, MasterAssetName);

	UE_LOG(LogRTGrayboxMaterials, Display, TEXT("Master -> %s%s"),
		*MasterPackage, bDryRun ? TEXT("  [DryRun]") : TEXT(""));

	if (bDryRun)
	{
		for (const FRTGrayboxMaterialSpec& Spec : KitMaterials)
		{
			UE_LOG(LogRTGrayboxMaterials, Display,
				TEXT("  %s -> %s | valore %.2f | ruvidita' %.2f"),
				Spec.MeshName, Spec.InstanceName, Spec.Value, Spec.Roughness);
		}
		return 0;
	}

	UMaterial* Master = EnsureMaster(MasterPackage, MasterAssetName);
	if (!Master || !SaveMaterialPackage(Master))
	{
		UE_LOG(LogRTGrayboxMaterials, Error, TEXT("%s non e' stato salvato."), MasterAssetName);
		return 1;
	}

	int32 Failed = 0;
	for (const FRTGrayboxMaterialSpec& Spec : KitMaterials)
	{
		const FString InstancePackage =
			FString::Printf(TEXT("%s/%s/%s"), *PackageRoot, MaterialsFolder, Spec.InstanceName);

		UMaterialInstanceConstant* Instance = EnsureInstance(InstancePackage, Spec.InstanceName, Master, Spec);
		if (Instance && SaveMaterialPackage(Instance))
		{
			OutInstances.Add(FString(Spec.MeshName), Instance);
			UE_LOG(LogRTGrayboxMaterials, Display,
				TEXT("  %s -> %s | valore %.2f | ruvidita' %.2f"),
				Spec.MeshName, *InstancePackage, Spec.Value, Spec.Roughness);
		}
		else
		{
			++Failed;
			UE_LOG(LogRTGrayboxMaterials, Error, TEXT("%s non e' stata salvata."), Spec.InstanceName);
		}
	}

	return Failed;
}
