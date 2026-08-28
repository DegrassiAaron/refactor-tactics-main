#include "Content/RTBuildIconCatalogCommandlet.h"

#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "IAssetTools.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#include "UI/RTIconCatalogData.h"
#include "UI/RTIconLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTIconCatalog, Log, All);

namespace
{
	/** `UI.Icon.Action.Move` -> `RT_UI_Icon_Action_Move`.
	 *
	 *  La derivazione e' l'unica ragione per cui questo commandlet non ha bisogno di una tabella di
	 *  corrispondenza: il generatore di `tools/hud-assets/` scrive i PNG con lo stesso nome, quindi la
	 *  chiave trova la sua texture da sola. Se un giorno le due regole divergono, non si rompe in
	 *  silenzio — la texture non si trova e il commandlet lo stampa.
	 */
	FString AssetNameForIcon(const FName& IconId)
	{
		return FString::Printf(TEXT("RT_%s"), *IconId.ToString().Replace(TEXT("."), TEXT("_")));
	}

	/** La categoria e' il segmento dentro l'ID, e DEVE essere un valore di `ERTIconCategory`: e' quello
	 *  che `ValidateIconCatalog` confronta. Dedurla qui invece di dichiararla evita l'unico errore che
	 *  un catalogo scritto a mano fa davvero: chiave giusta, categoria sbagliata. */
	bool CategoryForIcon(const FName& IconId, ERTIconCategory& OutCategory)
	{
		const FString Id = IconId.ToString();
		const UEnum* Enum = StaticEnum<ERTIconCategory>();
		for (int32 i = 0; Enum && i < Enum->NumEnums() - 1; ++i)
		{
			const FString Name = Enum->GetNameStringByIndex(i);
			if (Id.StartsWith(FString::Printf(TEXT("UI.Icon.%s."), *Name)))
			{
				OutCategory = static_cast<ERTIconCategory>(Enum->GetValueByIndex(i));
				return true;
			}
		}
		return false;
	}

	FString ObjectPathFor(const FString& PackageRoot, const FString& AssetName)
	{
		return FString::Printf(TEXT("%s/%s.%s"), *PackageRoot, *AssetName, *AssetName);
	}

	/** Impostazioni texture per un'icona di HUD. Non sono estetica: una icona con le mipmap e la
	 *  compressione di default arriva a schermo sfocata e con gli artefatti sui bordi netti, ed e' il
	 *  motivo per cui un import "a occhio" sembra sempre peggiore del PNG di partenza. */
	void ApplyUiTextureSettings(UTexture2D* Texture)
	{
		if (!Texture)
		{
			return;
		}
		Texture->LODGroup = TEXTUREGROUP_UI;
		Texture->CompressionSettings = TC_EditorIcon;   // UserInterface2D
		Texture->MipGenSettings = TMGS_NoMipmaps;
		Texture->SRGB = true;
		Texture->NeverStream = true;
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
	}

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
}

URTBuildIconCatalogCommandlet::URTBuildIconCatalogCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URTBuildIconCatalogCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamsMap;
	ParseCommandLine(*Params, Tokens, Switches, ParamsMap);

	const bool bDryRun = Switches.Contains(TEXT("DryRun"));

	FString SourceDir = ParamsMap.FindRef(TEXT("SourceDir"));
	if (SourceDir.IsEmpty())
	{
		SourceDir = FPaths::ProjectContentDir() / TEXT("RT/UI/_Generated/Icons");
	}
	const FString PackageRoot = ParamsMap.Contains(TEXT("Package"))
		? ParamsMap[TEXT("Package")] : TEXT("/Game/RT/UI/Icons");
	const FString CatalogPath = ParamsMap.Contains(TEXT("Catalog"))
		? ParamsMap[TEXT("Catalog")] : TEXT("/Game/RT/UI/DA_IconCatalog");
	const int32 Size = ParamsMap.Contains(TEXT("Size"))
		? FCString::Atoi(*ParamsMap[TEXT("Size")]) : 48;

	const TArray<FName> Required = URTIconLibrary::RequiredIconIds();
	UE_LOG(LogRTIconCatalog, Display, TEXT("Chiavi richieste: %d"), Required.Num());
	UE_LOG(LogRTIconCatalog, Display, TEXT("Sorgente PNG: %s (taglia %d)"), *SourceDir, Size);

	// ---------------------------------------------------------------------------------------------
	// 1. Il PNG di ogni chiave deve esistere PRIMA di toccare qualsiasi asset.
	//
	// Importare a meta' lascerebbe un catalogo parziale che sembra progredito: un catalogo con 40
	// chiavi su 61 non e' «quasi pronto», e' rotto in un modo che la validazione non distingue da
	// «qualcuno ha cancellato venti icone». Meglio non cominciare.
	// ---------------------------------------------------------------------------------------------
	TMap<FName, FString> SourceFiles;
	TArray<FName> WithoutSource;
	for (const FName& IconId : Required)
	{
		const FString File = SourceDir / FString::Printf(
			TEXT("%s_%d.png"), *AssetNameForIcon(IconId), Size);
		if (FPaths::FileExists(File))
		{
			SourceFiles.Add(IconId, File);
		}
		else
		{
			WithoutSource.Add(IconId);
		}
	}

	const FString MissingIconAsset = TEXT("RT_UI_Icon_MissingIcon");
	const FString MissingIconFile = SourceDir / FString::Printf(TEXT("%s_%d.png"), *MissingIconAsset, Size);
	const bool bHasMissingIconSource = FPaths::FileExists(MissingIconFile);

	if (WithoutSource.Num() > 0 || !bHasMissingIconSource)
	{
		UE_LOG(LogRTIconCatalog, Error,
			TEXT("%d chiavi senza PNG%s. Rigenera con: python3 tools/hud-assets/generate_hud_assets.py"),
			WithoutSource.Num(), bHasMissingIconSource ? TEXT("") : TEXT(" (manca anche MissingIcon)"));
		for (const FName& IconId : WithoutSource)
		{
			UE_LOG(LogRTIconCatalog, Error, TEXT("   %s"), *IconId.ToString());
		}
		return 1;
	}

	if (bDryRun)
	{
		UE_LOG(LogRTIconCatalog, Display,
			TEXT("DryRun: %d PNG presenti, nessun asset scritto. Rilancia senza -DryRun."),
			SourceFiles.Num() + 1);
		return 0;
	}

	// ---------------------------------------------------------------------------------------------
	// 2. Import.
	//
	// `ImportAssetTasks` invece di `ImportAssets` perche' solo il primo lascia scegliere il NOME di
	// destinazione: il secondo lo prende dal file, e i file portano il suffisso della taglia
	// (`..._48.png`). Un asset chiamato `RT_UI_Icon_Action_Move_48` romperebbe la derivazione, che e'
	// l'unica cosa che tiene insieme chiave e texture.
	// ---------------------------------------------------------------------------------------------
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(
		TEXT("AssetTools")).Get();

	TArray<UAssetImportTask*> Tasks;
	TMap<FName, UAssetImportTask*> TaskByIcon;
	auto MakeTask = [&](const FString& File, const FString& AssetName) -> UAssetImportTask*
	{
		UAssetImportTask* Task = NewObject<UAssetImportTask>();
		Task->Filename = File;
		Task->DestinationPath = PackageRoot;
		Task->DestinationName = AssetName;
		Task->bAutomated = true;          // niente dialoghi: deve girare headless
		Task->bReplaceExisting = true;    // rilanciarlo aggiorna, non duplica
		Task->bSave = false;              // si salva dopo, insieme al catalogo
		Task->Factory = NewObject<UTextureFactory>();
		Tasks.Add(Task);
		return Task;
	};

	for (const TPair<FName, FString>& Pair : SourceFiles)
	{
		TaskByIcon.Add(Pair.Key, MakeTask(Pair.Value, AssetNameForIcon(Pair.Key)));
	}
	UAssetImportTask* MissingTask = MakeTask(MissingIconFile, MissingIconAsset);

	AssetTools.ImportAssetTasks(Tasks);

	int32 Imported = 0;
	for (UAssetImportTask* Task : Tasks)
	{
		for (UObject* Object : Task->GetObjects())
		{
			if (UTexture2D* Texture = Cast<UTexture2D>(Object))
			{
				ApplyUiTextureSettings(Texture);
				SaveAssetPackage(Texture);
				++Imported;
			}
		}
	}
	UE_LOG(LogRTIconCatalog, Display, TEXT("Texture importate e salvate: %d"), Imported);

	// ---------------------------------------------------------------------------------------------
	// 3. Il catalogo.
	//
	// Le voci si RICOSTRUISCONO da zero a ogni esecuzione. Non e' pigrizia: una chiave rimasta dopo
	// che il gioco l'ha tolta e' un duplicato in attesa — e `ValidateIconCatalog` tratta una chiave
	// duplicata come errore, non come sovrascrittura silenziosa.
	// ---------------------------------------------------------------------------------------------
	URTIconCatalogData* Catalog = LoadObject<URTIconCatalogData>(nullptr, *CatalogPath);
	if (!Catalog)
	{
		const FString CatalogName = FPackageName::GetShortName(CatalogPath);
		UPackage* Package = CreatePackage(*CatalogPath);
		Catalog = NewObject<URTIconCatalogData>(Package, URTIconCatalogData::StaticClass(),
			*CatalogName, RF_Public | RF_Standalone);
		UE_LOG(LogRTIconCatalog, Display, TEXT("Creato %s"), *CatalogPath);
	}

	Catalog->Icons.Reset();
	TArray<FName> WithoutTexture;
	for (const FName& IconId : Required)
	{
		ERTIconCategory Category = ERTIconCategory::Identity;
		if (!CategoryForIcon(IconId, Category))
		{
			UE_LOG(LogRTIconCatalog, Error,
				TEXT("'%s' non comincia con una categoria di ERTIconCategory: e' un errore della "
					 "chiave, non dell'import"), *IconId.ToString());
			return 1;
		}

		const FSoftObjectPath Path(ObjectPathFor(PackageRoot, AssetNameForIcon(IconId)));
		if (!Path.ResolveObject() && !Path.TryLoad())
		{
			WithoutTexture.Add(IconId);
			continue;
		}
		Catalog->Icons.Add(FRTIconDef(IconId, Category, TSoftObjectPtr<UTexture2D>(Path)));
	}
	Catalog->MissingIcon = TSoftObjectPtr<UTexture2D>(
		FSoftObjectPath(ObjectPathFor(PackageRoot, MissingIconAsset)));

	if (WithoutTexture.Num() > 0)
	{
		UE_LOG(LogRTIconCatalog, Error, TEXT("%d chiavi senza texture importata:"), WithoutTexture.Num());
		for (const FName& IconId : WithoutTexture)
		{
			UE_LOG(LogRTIconCatalog, Error, TEXT("   %s"), *IconId.ToString());
		}
	}

	Catalog->MarkPackageDirty();
	if (!SaveAssetPackage(Catalog))
	{
		UE_LOG(LogRTIconCatalog, Error, TEXT("Salvataggio di %s fallito"), *CatalogPath);
		return 1;
	}

	// ---------------------------------------------------------------------------------------------
	// 4. Il verdetto lo danno le funzioni del gioco, non questo commandlet.
	// ---------------------------------------------------------------------------------------------
	const TArray<FName> StillMissing = URTIconLibrary::FindMissingRequiredIcons(Catalog);
	const TArray<FString> Errors = URTIconLibrary::ValidateIconCatalog(Catalog);

	UE_LOG(LogRTIconCatalog, Display, TEXT("--- verdetto ---"));
	UE_LOG(LogRTIconCatalog, Display, TEXT("FindMissingRequiredIcons: %d"), StillMissing.Num());
	for (const FName& IconId : StillMissing)
	{
		UE_LOG(LogRTIconCatalog, Warning, TEXT("   manca %s"), *IconId.ToString());
	}
	UE_LOG(LogRTIconCatalog, Display, TEXT("ValidateIconCatalog: %d errori"), Errors.Num());
	for (const FString& Error : Errors)
	{
		UE_LOG(LogRTIconCatalog, Warning, TEXT("   %s"), *Error);
	}

	const bool bClean = StillMissing.Num() == 0 && Errors.Num() == 0;
	UE_LOG(LogRTIconCatalog, Display, TEXT("%s"),
		bClean ? TEXT("Catalogo completo e valido.") : TEXT("Catalogo NON pronto."));
	return bClean ? 0 : 1;
}
