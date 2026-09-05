#include "Content/RTRemoveEnergyBarCommandlet.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTEnergyBar, Log, All);

namespace
{
	const TCHAR* OverlayPath = TEXT("/Game/RT/UI/Match/WBP_RT_UnitOverlay.WBP_RT_UnitOverlay");
	const TCHAR* CardPath    = TEXT("/Game/RT/UI/Match/WBP_RT_UnitCard.WBP_RT_UnitCard");
	const TCHAR* BarName     = TEXT("EnergyBar");

	/** Il salvataggio, nella stessa forma di `RTBuildPlaygroundPanel`. */
	bool SaveWidgetPackage(UPackage* Package, UBlueprint* Blueprint)
	{
		const FString FileName = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Blueprint, *FileName, SaveArgs);
	}

	/**
	 * Ricompila e risalva un `WBP` senza toccarne l'albero.
	 *
	 * Serve a `WBP_RT_UnitCard`, che non ha un widget da togliere ma **pin orfani**: il suo grafo leggeva
	 * `Card_Energy`/`Card_MaxEnergy` da `FRTUnitCardView`, e i membri non esistono piu'. La ricompilazione
	 * li fa cadere; senza il salvataggio resterebbero scritti nel `.uasset`.
	 */
	bool RecompileAndSave(UWidgetBlueprint* Blueprint, const TCHAR* Etichetta, bool bDryRun)
	{
		if (bDryRun)
		{
			UE_LOG(LogRTEnergyBar, Display, TEXT("[DryRun] %s: ricompilerei e risalverei"), Etichetta);
			return true;
		}
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		Blueprint->MarkPackageDirty();
		const bool bSaved = SaveWidgetPackage(Blueprint->GetOutermost(), Blueprint);
		UE_LOG(LogRTEnergyBar, Display, TEXT("%s: ricompilato e %s"),
			Etichetta, bSaved ? TEXT("salvato") : TEXT("NON salvato"));
		return bSaved;
	}
}

URTRemoveEnergyBarCommandlet::URTRemoveEnergyBarCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URTRemoveEnergyBarCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	ParseCommandLine(*Params, Tokens, Switches);
	const bool bDryRun = Switches.Contains(TEXT("DryRun"));

	UE_LOG(LogRTEnergyBar, Display, TEXT("=== RTRemoveEnergyBar%s ==="),
		bDryRun ? TEXT(" [DryRun]") : TEXT(""));

	// ---------------------------------------------------------------- 1. l'overlay: c'e' un widget da togliere
	UWidgetBlueprint* Overlay = LoadObject<UWidgetBlueprint>(nullptr, OverlayPath);
	if (!Overlay || !Overlay->WidgetTree)
	{
		UE_LOG(LogRTEnergyBar, Error, TEXT("overlay non caricato: %s"), OverlayPath);
		return 1;
	}

	UWidget* Bar = Overlay->WidgetTree->FindWidget(FName(BarName));
	if (!Bar)
	{
		// Non e' un errore: il commandlet e' ripetibile, e la seconda volta non trova piu' niente.
		UE_LOG(LogRTEnergyBar, Display,
			TEXT("overlay: nessun widget '%s' — gia' rimosso, niente da fare"), BarName);
	}
	else
	{
		UE_LOG(LogRTEnergyBar, Display, TEXT("overlay: trovato '%s' (%s)"),
			BarName, *Bar->GetClass()->GetName());

		if (!Cast<UProgressBar>(Bar))
		{
			// Se un giorno quel nome designasse un altro widget, toglierlo alla cieca cancellerebbe una cosa
			// diversa da quella che questa issue ha misurato.
			UE_LOG(LogRTEnergyBar, Error,
				TEXT("overlay: '%s' non e' una UProgressBar ma una %s — non lo tocco"),
				BarName, *Bar->GetClass()->GetName());
			return 1;
		}

		if (bDryRun)
		{
			UE_LOG(LogRTEnergyBar, Display, TEXT("[DryRun] overlay: rimuoverei '%s' e il suo GUID"), BarName);
		}
		else
		{
			// 🔴 **Togliere il widget non basta**, ed e' la stessa trappola che `RTBuildPlaygroundPanel`
			// documenta: ogni widget-variabile ha un GUID in `WidgetVariableNameToGuidMap`, e lasciandolo
			// `CompileBlueprint` ASSERISCE — *«Variable was deleted but still has a GUID referenced by
			// WidgetBlueprint»*.
			const FName DeletedName = Bar->GetFName();
			Overlay->WidgetTree->RemoveWidget(Bar);
			Overlay->WidgetVariableNameToGuidMap.Remove(DeletedName);
			UE_LOG(LogRTEnergyBar, Display, TEXT("overlay: '%s' rimosso dall'albero e dalla mappa dei GUID"),
				BarName);
		}

		if (!RecompileAndSave(Overlay, TEXT("overlay"), bDryRun))
		{
			return 1;
		}
	}

	// ---------------------------------------------------------------- 2. la card: pin orfani, non widget
	UWidgetBlueprint* Card = LoadObject<UWidgetBlueprint>(nullptr, CardPath);
	if (!Card)
	{
		UE_LOG(LogRTEnergyBar, Error, TEXT("card non caricata: %s"), CardPath);
		return 1;
	}
	UE_LOG(LogRTEnergyBar, Display,
		TEXT("card: nessun widget da togliere — si ricompila per far cadere i pin orfani "
		     "(`Card_Energy`, `Card_MaxEnergy`), che senza risalvataggio resterebbero nel .uasset"));
	if (!RecompileAndSave(Card, TEXT("card"), bDryRun))
	{
		return 1;
	}

	UE_LOG(LogRTEnergyBar, Display, TEXT("=== fatto%s ==="), bDryRun ? TEXT(" [DryRun: nulla e' stato scritto]") : TEXT(""));
	return 0;
}
