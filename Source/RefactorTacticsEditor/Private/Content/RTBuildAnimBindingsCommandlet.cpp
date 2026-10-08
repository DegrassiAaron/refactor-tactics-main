#include "Content/RTBuildAnimBindingsCommandlet.h"

#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#include "Unit/RTAnimCatalogLibrary.h"
#include "Unit/RTAnimCatalogTypes.h"
#include "Unit/RTUnitAnimInstance.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTAnimBindings, Log, All);

namespace
{
	const TCHAR* RTAuthoredPackage = TEXT("/Game/RT/Anim/ABP_RTUnitAuthored");
	const TCHAR* RTAuthoredAsset = TEXT("ABP_RTUnitAuthored");
}

/**
 * Traduce i binding del catalogo nella forma che il CDO consuma.
 *
 * ⚠️ **`VariantId` e' l'`AV_ID`, e non un contatore locale.** L'identita' della variante e' gia' stata
 * coniata una volta sola, da `AllocateIds`, con la garanzia che non torna mai su un'altra clip.
 * Inventarne una seconda qui creerebbe due spazi di id per la stessa cosa.
 */
TMap<FName, FRTHeroPresentationClips> URTBuildAnimBindingsCommandlet::BuildClipsPerHero(
	const FRTAnimCatalog& Catalog, int32& OutLegami)
{
	TMap<FName, FRTHeroPresentationClips> PerEroe;
	OutLegami = 0;

	for (const FRTAnimCatalogEntry& Entry : Catalog.Entries)
	{
		for (const FRTAnimBinding& Binding : Entry.Authored.Bindings)
		{
			if (Binding.HeroId.IsNone() || Entry.Derived.AssetPath.IsEmpty())
			{
				continue;
			}

			FRTHeroPresentationClips& Eroe = PerEroe.FindOrAdd(Binding.HeroId);
			// 🔑 #3563: un binding d'azione va nel pool dell'AZIONE, mai in quello di ruolo — due pool, una attiva per
			// pool. Un binding di ruolo resta dov'era: per lui questa funzione non cambia.
			FRTAnimRoleClips& Ruolo = Binding.ActionId.IsNone()
				? Eroe.PerRole.FindOrAdd(Binding.Role)
				: Eroe.PerAction.FindOrAdd(Binding.ActionId).PerRole.FindOrAdd(Binding.Role);

			// `AddVariant` mette la variante INATTIVA, sempre: e' la stessa regola che vale nel pannello
			// e nel catalogo, e qui la si eredita invece di riscriverla.
			Ruolo.AddVariant(
				Entry.Id,
				FName(*Entry.Authored.Label),
				TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Entry.Derived.AssetPath)));

			if (Binding.bActive)
			{
				// `ValidateCatalog` ha gia' rifiutato due attive nello stesso pool, quindi questa
				// chiamata non puo' sovrascriverne un'altra: la garanzia sta a monte, non qui.
				Ruolo.MakeActive(Entry.Id);
			}
			++OutLegami;
		}
	}
	return PerEroe;
}

/**
 * Il catalogo SOPRA il default, un pool alla volta (#3563, `Ruling` di §2.3).
 *
 * 🔴 **Prima di questa funzione il commandlet SOSTITUIVA l'intera mappa**: con la classe autorata cablata (#3562),
 * un eroe senza binding avrebbe perso ogni clip. Qui ogni `(eroe, ruolo)` e ogni `(eroe, azione, ruolo)` che il
 * catalogo nomina sostituisce il pool di `Base` per intero; eroi e pool che non nomina restano quelli di `Base`.
 * ⚠️ Pura e separata da `BuildClipsPerHero`, che resta la traduzione del solo catalogo (`Anim.Bindings.MapToCdo`).
 */
TMap<FName, FRTHeroPresentationClips> URTBuildAnimBindingsCommandlet::MergeClipsPerHero(
	const TMap<FName, FRTHeroPresentationClips>& Base, const TMap<FName, FRTHeroPresentationClips>& PerEroe)
{
	TMap<FName, FRTHeroPresentationClips> Fuso = Base;
	for (const TPair<FName, FRTHeroPresentationClips>& Eroe : PerEroe)
	{
		FRTHeroPresentationClips& Dest = Fuso.FindOrAdd(Eroe.Key);
		for (const TPair<ERTPresentationRole, FRTAnimRoleClips>& Pool : Eroe.Value.PerRole)
		{
			Dest.PerRole.Add(Pool.Key, Pool.Value);   // `Add` su chiave esistente sostituisce: il pool intero
		}
		for (const TPair<FName, FRTActionPresentationClips>& Azione : Eroe.Value.PerAction)
		{
			FRTActionPresentationClips& DestAzione = Dest.PerAction.FindOrAdd(Azione.Key);
			for (const TPair<ERTPresentationRole, FRTAnimRoleClips>& Pool : Azione.Value.PerRole)
			{
				DestAzione.PerRole.Add(Pool.Key, Pool.Value);
			}
		}
	}
	return Fuso;
}

namespace
{
	bool SalvaPackage(UPackage* Package, UObject* Asset)
	{
		const FString FileName = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Asset, *FileName, SaveArgs);
	}
}

int32 URTBuildAnimBindingsCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	ParseCommandLine(*Params, Tokens, Switches);
	const bool bDryRun = Switches.Contains(TEXT("DryRun"));

	// ── 1. Il catalogo ──────────────────────────────────────────────────────────────────────────────
	const FString CatalogPath = URTAnimCatalogLibrary::DefaultCatalogPath();
	FRTAnimCatalog Catalog;
	FString Errore;
	if (!URTAnimCatalogLibrary::LoadFromFile(CatalogPath, Catalog, Errore))
	{
		UE_LOG(LogRTAnimBindings, Error, TEXT("[AnimBindings] catalogo illeggibile (%s): %s"),
			*CatalogPath, *Errore);
		return 1;
	}

	// ⛔ **Si rifiuta di generare da un catalogo non valido.** Due attive nello stesso POOL — `(eroe, ruolo)` o,
	// da #3563, `(eroe, ruolo, azione)` — sono rappresentabili nel testo e non a runtime: generare comunque
	// significherebbe sceglierne una per posizione nell'array, cioe' far dipendere la clip che suona dall'ordine
	// delle righe di un file. Lo stesso vale per un `actionId` che non suonerebbe mai.
	const TArray<FString> Errori = URTAnimCatalogLibrary::ValidateCatalog(&Catalog);
	if (Errori.Num() > 0)
	{
		for (const FString& E : Errori)
		{
			UE_LOG(LogRTAnimBindings, Error, TEXT("[AnimBindings] catalogo non valido: %s"), *E);
		}
		return 1;
	}

	int32 Legami = 0;
	const TMap<FName, FRTHeroPresentationClips> PerEroe =
		URTBuildAnimBindingsCommandlet::BuildClipsPerHero(Catalog, Legami);

	UE_LOG(LogRTAnimBindings, Display,
		TEXT("[AnimBindings] %d voci a catalogo, %d legami, %d eroi coinvolti."),
		Catalog.Entries.Num(), Legami, PerEroe.Num());

	if (bDryRun)
	{
		UE_LOG(LogRTAnimBindings, Display, TEXT("[AnimBindings] -DryRun: nessun asset scritto."));
		return 0;
	}

	// ── 2. Il Blueprint ─────────────────────────────────────────────────────────────────────────────
	//
	// ⚠️ Rigenerare qui e' **non distruttivo per definizione**: questo asset non ha grafo né layout autorato, e
	// porta solo `ClipsPerHero` — che e' il default C++ con sopra i POOL che il catalogo nomina (#3563). Il
	// catalogo non possiede la mappa intera: possiede i pool che nomina. E' la differenza con
	// `RTBuildPlaygroundPanel`, che invece si rifiuta senza `-Force` perche' cancellerebbe un grafo.
	UBlueprint* Esistente = LoadObject<UBlueprint>(nullptr, RTAuthoredPackage);
	if (Esistente)
	{
		// ⚠️ `CreateBlueprint` ASSERISCE se un oggetto con quel nome vive gia' nel package — misurato su
		// #1992: non ritorna `nullptr`, fa crashare il commandlet.
		Esistente->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
	}

	UPackage* Package = CreatePackage(RTAuthoredPackage);
	if (Package == nullptr)
	{
		UE_LOG(LogRTAnimBindings, Error, TEXT("[AnimBindings] package non creabile: %s"), RTAuthoredPackage);
		return 1;
	}

	UBlueprint* Generato = FKismetEditorUtilities::CreateBlueprint(
		URTUnitAnimInstance::StaticClass(), Package, FName(RTAuthoredAsset), BPTYPE_Normal,
		UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());

	if (Generato == nullptr || Generato->GeneratedClass == nullptr)
	{
		UE_LOG(LogRTAnimBindings, Error, TEXT("[AnimBindings] creazione del Blueprint fallita."));
		return 1;
	}

	// ── 3. Il CDO ───────────────────────────────────────────────────────────────────────────────────
	//
	// 🔑 Si scrive sul CDO della classe generata, non sull'oggetto Blueprint: e' il default che ogni istanza
	// erediterà. Il valore e' il default C++ della CLASSE BASE con sopra i pool del catalogo (#3563).
	URTUnitAnimInstance* Cdo = Cast<URTUnitAnimInstance>(Generato->GeneratedClass->GetDefaultObject());
	if (Cdo == nullptr)
	{
		UE_LOG(LogRTAnimBindings, Error, TEXT("[AnimBindings] CDO della classe generata non trovato."));
		return 1;
	}
	// ⛔ La base e' il CDO di `URTUnitAnimInstance`, NON quello della classe generata: quello si porterebbe dietro
	// la mappa della run precedente, e un pool tolto dal catalogo resterebbe nell'asset.
	Cdo->ClipsPerHero = URTBuildAnimBindingsCommandlet::MergeClipsPerHero(
		URTUnitAnimInstance::StaticClass()->GetDefaultObject<URTUnitAnimInstance>()->ClipsPerHero, PerEroe);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Generato);
	FKismetEditorUtilities::CompileBlueprint(Generato);
	Generato->MarkPackageDirty();

	if (!SalvaPackage(Package, Generato))
	{
		UE_LOG(LogRTAnimBindings, Error, TEXT("[AnimBindings] salvataggio fallito: %s"), RTAuthoredPackage);
		return 1;
	}

	UE_LOG(LogRTAnimBindings, Display,
		TEXT("[AnimBindings] %s scritto: %d eroi, %d legami."), RTAuthoredAsset, PerEroe.Num(), Legami);
	return 0;
}
