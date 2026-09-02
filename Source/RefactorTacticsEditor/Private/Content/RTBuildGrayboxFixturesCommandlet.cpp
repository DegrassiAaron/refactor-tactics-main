#include "Content/RTBuildGrayboxFixturesCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#include "Content/RTGrayboxMaterials.h"
#include "RTPlaygroundLayout.h"
#include "World/RTGrayboxUnitFacingFixture.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTGrayboxFixtures, Log, All);

namespace
{
	const TCHAR* RTFixturePackage = TEXT("/Game/RT/World/Graybox/Fixtures/BP_Graybox_UnitFacingFixture");
	const TCHAR* RTFixtureAsset   = TEXT("BP_Graybox_UnitFacingFixture");
	const TCHAR* RTPlaygroundMap  = TEXT("/Game/RT/Maps/Dev/L_GrayKitPlayground/L_GrayKitPlayground");

	bool RTSavePackageToDisk(UPackage* Package, UObject* Asset)
	{
		const FString FileName = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());

		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Asset, *FileName, Args);
	}
}

int32 URTBuildGrayboxFixturesCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	ParseCommandLine(*Params, Tokens, Switches);
	const bool bForce = Switches.Contains(TEXT("Force"));
	const bool bPlace = Switches.Contains(TEXT("Place"));

	// ---------------------------------------------------------------- i materiali

	// ⚠️ **PRIMA del Blueprint**, e non e' un dettaglio d'ordine: il costruttore dell'attore risolve i tre
	// materiali per path, quindi devono esistere su disco prima che la classe venga istanziata. Senza, il
	// fixture nasce col grigio di default — cioe' quello del pavimento, e a schermo sparisce.
	TMap<FString, UMaterialInterface*> FixtureMats;
	const int32 MatFailed = RTGraybox::BuildFixtureMaterials(TEXT("/Game/RT/World/Graybox"), /*bDryRun=*/ false, FixtureMats);
	if (MatFailed > 0)
	{
		UE_LOG(LogRTGrayboxFixtures, Error, TEXT("[GrayboxFixtures] %d materiali non salvati."), MatFailed);
		return 1;
	}
	UE_LOG(LogRTGrayboxFixtures, Display, TEXT("[GrayboxFixtures] %d materiali del fixture pronti."), FixtureMats.Num());

	// ---------------------------------------------------------------- il Blueprint

	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, RTFixturePackage);

	if (Blueprint && !bForce)
	{
		// ⛔ **Si RIUSA, non si sovrascrive.** Un `.uasset` non si fonde, e se qualcuno ha tarato i cinque
		// parametri a mano rigenerarlo glieli cancella senza dirlo. `-Force` e' come si chiede davvero.
		UE_LOG(LogRTGrayboxFixtures, Display,
			TEXT("[GrayboxFixtures] %s esiste gia': lo riuso. -Force per rigenerarlo, sapendo che i parametri tarati a mano si perdono."),
			RTFixtureAsset);

		// Un Blueprint con quel nome ma un altro parent non e' questo fixture: meglio fermarsi che posare
		// un oggetto che somiglia a quello giusto.
		if (Blueprint->ParentClass != ARTGrayboxUnitFacingFixture::StaticClass())
		{
			UE_LOG(LogRTGrayboxFixtures, Error,
				TEXT("[GrayboxFixtures] %s esiste ma deriva da %s, non da RTGrayboxUnitFacingFixture."),
				RTFixtureAsset, Blueprint->ParentClass ? *Blueprint->ParentClass->GetName() : TEXT("<nessuno>"));
			return 1;
		}
	}
	else
	{
		if (Blueprint)
		{
			// 🔴 **`CreateBlueprint` ASSERISCE se un oggetto con quel nome vive gia' nel package** — non
			// ritorna `nullptr`, fa crashare il commandlet (`Kismet2.cpp:441`). Misurato: la seconda
			// esecuzione con `-Force` moriva li'. Il vecchio va spostato nel transient, non sovrascritto.
			Blueprint->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
			Blueprint = nullptr;
		}

		UPackage* Package = CreatePackage(RTFixturePackage);
		if (!Package)
		{
			UE_LOG(LogRTGrayboxFixtures, Error, TEXT("[GrayboxFixtures] package non creabile: %s"), RTFixturePackage);
			return 1;
		}

		Blueprint = FKismetEditorUtilities::CreateBlueprint(
			ARTGrayboxUnitFacingFixture::StaticClass(), Package, FName(RTFixtureAsset),
			BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());

		if (!Blueprint)
		{
			UE_LOG(LogRTGrayboxFixtures, Error, TEXT("[GrayboxFixtures] CreateBlueprint ha fallito."));
			return 1;
		}

		FAssetRegistryModule::AssetCreated(Blueprint);
		Blueprint->MarkPackageDirty();

		if (!RTSavePackageToDisk(Package, Blueprint))
		{
			UE_LOG(LogRTGrayboxFixtures, Error, TEXT("[GrayboxFixtures] salvataggio fallito: %s"), RTFixturePackage);
			return 1;
		}
		UE_LOG(LogRTGrayboxFixtures, Display,
			TEXT("[GrayboxFixtures] creato %s (parent: %s, nessun grafo: la geometria e' in C++)"),
			RTFixtureAsset, *ARTGrayboxUnitFacingFixture::StaticClass()->GetName());
	}

	if (!bPlace)
	{
		UE_LOG(LogRTGrayboxFixtures, Display, TEXT("[GrayboxFixtures] posa non richiesta (-Place per farla)."));
		return 0;
	}

	// ---------------------------------------------------------------- la posa

	// ⛔ La posizione NON e' incisa: viene dalla planimetria, che e' la stessa che il pannello (#1993) e i
	// test di `RTPlaygroundLayoutTests` consumano. Due copie dello stesso rettangolo divergono in silenzio.
	const RTPlayground::FStation* Station = RTPlayground::FindStation(1);
	if (!Station)
	{
		UE_LOG(LogRTGrayboxFixtures, Error, TEXT("[GrayboxFixtures] la Station 01 non esiste nella planimetria."));
		return 1;
	}
	const FVector2D CentreMetres = Station->Bounds.GetCenter();
	const FVector Location(RTPlayground::WorldFromMetres(CentreMetres.X),
	                       RTPlayground::WorldFromMetres(CentreMetres.Y),
	                       0.0);

	UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(RTPlaygroundMap);
	if (!World)
	{
		UE_LOG(LogRTGrayboxFixtures, Error, TEXT("[GrayboxFixtures] mappa non caricabile: %s"), RTPlaygroundMap);
		return 1;
	}

	// Un'istanza gia' posata: si conta, non si duplica. Rieseguire il commandlet due volte non deve
	// lasciare due fixture sovrapposti che a schermo sembrano uno solo.
	TArray<AActor*> Already;
	for (TActorIterator<ARTGrayboxUnitFacingFixture> It(World); It; ++It)
	{
		Already.Add(*It);
	}
	if (Already.Num() > 0 && !bForce)
	{
		UE_LOG(LogRTGrayboxFixtures, Error,
			TEXT("[GrayboxFixtures] la mappa contiene gia' %d fixture. Usa -Force per sostituirli."), Already.Num());
		return 1;
	}
	for (AActor* Old : Already)
	{
		World->DestroyActor(Old);
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// 🔴 **Nessun `SpawnParams.Name` fisso.** Con `-Force` l'istanza vecchia viene distrutta qui sopra ma
	// non e' ancora raccolta, quindi il nome resta occupato e `SpawnActor` **asserisce**
	// (`LevelActor.cpp:586`) invece di tornare `nullptr`. Misurato alla seconda esecuzione.
	// Il nome leggibile nell'Outliner lo da' `SetActorLabel`, che e' quello che si vede davvero.
	AActor* Placed = World->SpawnActor(Blueprint->GeneratedClass, &Location, &FRotator::ZeroRotator, SpawnParams);
	if (!Placed)
	{
		UE_LOG(LogRTGrayboxFixtures, Error, TEXT("[GrayboxFixtures] spawn fallito."));
		return 1;
	}
	Placed->SetActorLabel(TEXT("Station01_UnitFacing"));

	UPackage* MapPackage = World->GetOutermost();
	MapPackage->MarkPackageDirty();
	if (!UEditorLoadingAndSavingUtils::SavePackages({ MapPackage }, /*bOnlyDirty=*/ false))
	{
		UE_LOG(LogRTGrayboxFixtures, Error, TEXT("[GrayboxFixtures] salvataggio della mappa fallito."));
		return 1;
	}

	// ⚠️ Si stampa DOVE, in metri e in unita': e' l'unica cosa che il diff di un `.umap` non mostrera' mai,
	// e chi legge la PR deve poterla confrontare con la planimetria senza aprire l'Editor.
	UE_LOG(LogRTGrayboxFixtures, Display,
		TEXT("[GrayboxFixtures] posato in Station 01 «%s»: centro (%.1f, %.1f) m = (%.0f, %.0f) uu"),
		Station->Name, CentreMetres.X, CentreMetres.Y, Location.X, Location.Y);
	return 0;
}
