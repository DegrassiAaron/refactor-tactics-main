#include "Content/RTSyncUnitAnimRefsCommandlet.h"

#include "Animation/AnimSequenceBase.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "RTGameMode.h"
#include "Unit/RTUnit.h"
#include "Unit/RTUnitAnimInstance.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTSyncUnitAnimRefs, Log, All);

namespace
{
	const TCHAR* RTAuthoredClassPath   = TEXT("/Game/RT/Anim/ABP_RTUnitAuthored.ABP_RTUnitAuthored_C");
	const TCHAR* RTAuthoredPackageName = TEXT("/Game/RT/Anim/ABP_RTUnitAuthored");
	const FName  RTCookDepsName(TEXT("CookDeps_Anim"));

	/** Cio' che la fase 1 ha validato per un blueprint, e che la fase 2 scrive. */
	struct FRTPianoBlueprint
	{
		FName HeroId;
		UBlueprint* Blueprint = nullptr;
		UClass* Classe = nullptr;
		FArrayProperty* CookDeps = nullptr;
		FObjectProperty* Elemento = nullptr;
		TArray<UAnimSequenceBase*> Clip;   // ordinate per path: l'ordine di `CollectActiveClips`
	};

	/** Le clip che `CookDeps_Anim` contiene oggi, come path, in ordine. */
	TArray<FString> RTClipAttuali(const FRTPianoBlueprint& P)
	{
		TArray<FString> Out;
		const UObject* Cdo = P.Classe->GetDefaultObject();
		FScriptArrayHelper Helper(P.CookDeps, P.CookDeps->ContainerPtrToValuePtr<void>(Cdo));
		for (int32 I = 0; I < Helper.Num(); ++I)
		{
			if (const UObject* O = P.Elemento->GetObjectPropertyValue(Helper.GetRawPtr(I)))
			{
				Out.Add(O->GetPathName());
			}
			else
			{
				Out.Add(TEXT("None"));
			}
		}
		Out.Sort();
		return Out;
	}

	bool RTSalva(UBlueprint* Blueprint)
	{
		UPackage* Package = Blueprint->GetOutermost();
		const FString FileName = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Blueprint, *FileName, SaveArgs);
	}

	/** Le dipendenze DURE di un package secondo l'Asset Registry, rilette dal file su disco. */
	TSet<FName> RTDipendenzeDure(IAssetRegistry& Registro, const FString& PackageName, bool bRileggi)
	{
		if (bRileggi)
		{
			const FString File = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
			Registro.ScanFilesSynchronous({ File }, /*bForceRescan=*/ true);
		}
		TArray<FName> Dipendenze;
		Registro.GetDependencies(FName(*PackageName), Dipendenze, UE::AssetRegistry::EDependencyCategory::Package,
			UE::AssetRegistry::FDependencyQuery(UE::AssetRegistry::EDependencyQuery::Hard));
		return TSet<FName>(Dipendenze);
	}
}

URTSyncUnitAnimRefsCommandlet::URTSyncUnitAnimRefsCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URTSyncUnitAnimRefsCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	ParseCommandLine(*Params, Tokens, Switches);
	// `-ExpectNoChanges` implica `-DryRun`: e' la prova di andata e ritorno dopo un salvataggio (rilievo `R3`), ed esce
	// con errore se qualcosa entrerebbe o uscirebbe ancora.
	const bool bExpectNoChanges = Switches.Contains(TEXT("ExpectNoChanges"));
	const bool bDryRun = bExpectNoChanges || Switches.Contains(TEXT("DryRun"));
	UE_LOG(LogRTSyncUnitAnimRefs, Display, TEXT("=== RTSyncUnitAnimRefs%s%s ==="), bDryRun ? TEXT(" [DryRun]") : TEXT(""),
		bExpectNoChanges ? TEXT(" [ExpectNoChanges]") : TEXT(""));

	// ⚠️ `CompileBlueprint` esegue una garbage collection dopo ogni blueprint: i puntatori nudi di `Piani` devono
	// sopravviverle. Gli asset caricati da un package sono di norma `RF_Standalone`, ma non lo si da' per scontato.
	TArray<TStrongObjectPtr<UObject>> Trattenuti;

	IAssetRegistry& Registro = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registro.SearchAllAssets(/*bSynchronousSearch=*/ true);

	// ── Fase 1: validare tutto, senza scrivere niente ───────────────────────────────────────────────────
	int32 Errori = 0;
	auto Errore = [&Errori](const FString& Messaggio)
	{
		++Errori;
		UE_LOG(LogRTSyncUnitAnimRefs, Error, TEXT("%s"), *Messaggio);
	};

	UClass* Autorata = LoadClass<URTUnitAnimInstance>(nullptr, RTAuthoredClassPath);
	if (Autorata == nullptr)
	{
		Errore(FString::Printf(TEXT("la classe autorata non si carica (%s): lancia prima -run=RTBuildAnimBindings"),
			RTAuthoredClassPath));
		return 1;
	}
	Trattenuti.Emplace(Autorata);
	const URTUnitAnimInstance* CdoAutorato = Autorata->GetDefaultObject<URTUnitAnimInstance>();
	int32 TotaleEntrano = 0;
	int32 TotaleEscono = 0;
	int32 ClassiDaAssegnare = 0;

	// L'elenco ATTESO degli eroi e' quello del default C++: `HeroUnitClasses` lascia ASSENTE, in silenzio, un eroe il cui
	// `FClassFinder` non risolve (`RTGameMode.cpp`), e un commandlet che iterasse solo quella mappa uscirebbe 0 avendo
	// sincronizzato meno eroi.
	TArray<FName> EroiAttesi;
	GetDefault<URTUnitAnimInstance>()->ClipsPerHero.GetKeys(EroiAttesi);
	EroiAttesi.Sort(FNameLexicalLess());
	const TMap<FName, TSubclassOf<ARTUnit>>& Classi = GetDefault<ARTGameMode>()->HeroUnitClasses;

	TArray<FRTPianoBlueprint> Piani;
	for (const FName& Eroe : EroiAttesi)
	{
		const TSubclassOf<ARTUnit>* Voce = Classi.Find(Eroe);
		if (Voce == nullptr || Voce->Get() == nullptr)
		{
			Errore(FString::Printf(TEXT("%s: nessun blueprint in ARTGameMode::HeroUnitClasses"), *Eroe.ToString()));
			continue;
		}
		FRTPianoBlueprint P;
		P.HeroId = Eroe;
		P.Classe = Voce->Get();
		P.Blueprint = UBlueprint::GetBlueprintFromClass(P.Classe);
		if (P.Blueprint == nullptr)
		{
			Errore(FString::Printf(TEXT("%s: %s non viene da un Blueprint"), *Eroe.ToString(), *P.Classe->GetName()));
			continue;
		}
		Trattenuti.Emplace(P.Blueprint);
		const FString Package = P.Blueprint->GetOutermost()->GetName();

		P.CookDeps = FindFProperty<FArrayProperty>(P.Classe, RTCookDepsName);
		P.Elemento = P.CookDeps ? CastField<FObjectProperty>(P.CookDeps->Inner) : nullptr;
		if (P.CookDeps == nullptr)
		{
			Errore(FString::Printf(TEXT("%s: la variabile %s non c'e'"), *Package, *RTCookDepsName.ToString()));
			continue;
		}
		// 🔴 Rilievo `R2`: un array di riferimenti MORBIDI scriverebbe i path senza portarli nel cook, e il gate a byte
		// lo darebbe per buono. `FSoftObjectProperty` non e' un `FObjectProperty`, quindi il cast qui sopra lo esclude.
		if (P.Elemento == nullptr)
		{
			Errore(FString::Printf(TEXT("%s: %s non e' un array di riferimenti duri (elemento %s)"), *Package,
				*RTCookDepsName.ToString(), *P.CookDeps->Inner->GetClass()->GetName()));
			continue;
		}

		// La trappola del worktree: ogni dipendenza dura del blueprint deve esistere, o il salvataggio la azzererebbe.
		for (const FName& Dipendenza : RTDipendenzeDure(Registro, Package, /*bRileggi=*/ false))
		{
			const FString Nome = Dipendenza.ToString();
			if (Nome.StartsWith(TEXT("/Script/")))
			{
				continue;
			}
			if (!FPackageName::DoesPackageExist(Nome))
			{
				Errore(FString::Printf(TEXT("%s: la dipendenza dura %s non c'e' su disco"), *Package, *Nome));
				continue;
			}
			// «C'e' su disco» non basta: un package che non si carica verrebbe salvato come `None` (rilievo `R4`).
			if (LoadPackage(nullptr, *Nome, LOAD_None) == nullptr)
			{
				Errore(FString::Printf(TEXT("%s: la dipendenza dura %s non si carica"), *Package, *Nome));
			}
		}

		TArray<FSoftObjectPath> Richieste;
		CdoAutorato->CollectActiveClips(Eroe, Richieste);
		for (const FSoftObjectPath& Path : Richieste)
		{
			UAnimSequenceBase* Clip = Cast<UAnimSequenceBase>(Path.TryLoad());
			if (Clip == nullptr)
			{
				Errore(FString::Printf(TEXT("%s: la clip %s non si carica"), *Package, *Path.ToString()));
				continue;
			}
			if (!Clip->IsA(P.Elemento->PropertyClass))
			{
				Errore(FString::Printf(TEXT("%s: la clip %s non e' un %s"), *Package, *Path.ToString(),
					*P.Elemento->PropertyClass->GetName()));
				continue;
			}
			P.Clip.Add(Clip);
			Trattenuti.Emplace(Clip);
		}

		// Il confronto, nel log: cio' che entra e cio' che esce.
		TArray<FString> Nuove;
		for (const UAnimSequenceBase* Clip : P.Clip) { Nuove.Add(Clip->GetPathName()); }
		Nuove.Sort();
		const TArray<FString> Attuali = RTClipAttuali(P);
		int32 Entrano = 0;
		int32 Escono = 0;
		for (const FString& S : Nuove)
		{
			if (!Attuali.Contains(S)) { ++Entrano; UE_LOG(LogRTSyncUnitAnimRefs, Display, TEXT("  %s + %s"), *Package, *S); }
		}
		for (const FString& S : Attuali)
		{
			if (!Nuove.Contains(S)) { ++Escono; UE_LOG(LogRTSyncUnitAnimRefs, Display, TEXT("  %s - %s"), *Package, *S); }
		}
		const ARTUnit* UnitCdo = Cast<ARTUnit>(P.Classe->GetDefaultObject());
		const bool bClasseGiusta = UnitCdo && UnitCdo->UnitAnimClass.Get() == Autorata;
		TotaleEntrano += Entrano;
		TotaleEscono += Escono;
		ClassiDaAssegnare += bClasseGiusta ? 0 : 1;
		UE_LOG(LogRTSyncUnitAnimRefs, Display, TEXT("%s (%s): %d clip, %d entrano, %d escono; UnitAnimClass %s"),
			*Package, *Eroe.ToString(), P.Clip.Num(), Entrano, Escono,
			bClasseGiusta ? TEXT("gia' autorata") : TEXT("da assegnare"));
		Piani.Add(MoveTemp(P));
	}

	// Una riga fissa, che si legge col `Select-String` e non con l'occhio (rilievo 7 della revisione del codice).
	UE_LOG(LogRTSyncUnitAnimRefs, Display, TEXT("TOTALE blueprint=%d entrano=%d escono=%d classi_da_assegnare=%d errori=%d"),
		Piani.Num(), TotaleEntrano, TotaleEscono, ClassiDaAssegnare, Errori);
	if (Errori > 0)
	{
		UE_LOG(LogRTSyncUnitAnimRefs, Error, TEXT("%d errori nella validazione: nessun blueprint scritto."), Errori);
		return 1;
	}
	if (bDryRun)
	{
		UE_LOG(LogRTSyncUnitAnimRefs, Display, TEXT("-DryRun: nessun blueprint scritto."));
		if (bExpectNoChanges && (TotaleEntrano + TotaleEscono + ClassiDaAssegnare) > 0)
		{
			UE_LOG(LogRTSyncUnitAnimRefs, Error, TEXT("-ExpectNoChanges: i blueprint non portano ancora cio' che dovrebbero."));
			return 1;
		}
		return 0;
	}

	// ── Fase 2: scrivere ───────────────────────────────────────────────────────────────────────────────────
	for (FRTPianoBlueprint& P : Piani)
	{
		ARTUnit* UnitCdo = Cast<ARTUnit>(P.Classe->GetDefaultObject());
		UnitCdo->UnitAnimClass = Autorata;

		FScriptArrayHelper Helper(P.CookDeps, P.CookDeps->ContainerPtrToValuePtr<void>(UnitCdo));
		Helper.EmptyValues();
		for (UAnimSequenceBase* Clip : P.Clip)
		{
			const int32 I = Helper.AddValue();
			P.Elemento->SetObjectPropertyValue(Helper.GetRawPtr(I), Clip);
		}

		// Rilievo `R3`: se la variabile porta un default proprio, la compilazione potrebbe rimetterlo sul CDO. Lo si
		// riscrive con lo stesso valore; il secondo `-DryRun` dopo il salvataggio prova che e' sopravvissuto.
		const int32 Indice = FBlueprintEditorUtils::FindNewVariableIndex(P.Blueprint, RTCookDepsName);
		if (Indice != INDEX_NONE && !P.Blueprint->NewVariables[Indice].DefaultValue.IsEmpty())
		{
			FString Valore;
			if (!FBlueprintEditorUtils::PropertyValueToString(P.CookDeps, reinterpret_cast<const uint8*>(UnitCdo), Valore))
			{
				// Senza la stringa nuova, la compilazione rimetterebbe il default vecchio sopra il CDO appena scritto.
				Errore(FString::Printf(TEXT("%s: il default di %s non si converte, non compilo"),
					*P.Blueprint->GetPathName(), *RTCookDepsName.ToString()));
			}
			else
			{
				P.Blueprint->NewVariables[Indice].DefaultValue = Valore;
			}
		}

		if (Errori == 0)
		{
			FKismetEditorUtilities::CompileBlueprint(P.Blueprint);
			if (P.Blueprint->Status == BS_Error)
			{
				Errore(FString::Printf(TEXT("%s: la compilazione e' fallita, non salvo"), *P.Blueprint->GetPathName()));
			}
		}
		if (Errori == 0)
		{
			P.Blueprint->MarkPackageDirty();
			if (!RTSalva(P.Blueprint))
			{
				Errore(FString::Printf(TEXT("%s: salvataggio fallito"), *P.Blueprint->GetPathName()));
			}
		}
		if (Errori > 0)
		{
			// Al primo errore ci si ferma, e si dice chi era gia' stato scritto: l'albero non deve restare a meta' in
			// silenzio (rilievo 5 della revisione del codice).
			TArray<FString> Scritti;
			for (const FRTPianoBlueprint& Gia : Piani)
			{
				if (&Gia == &P) { break; }
				Scritti.Add(Gia.Blueprint->GetOutermost()->GetName());
			}
			UE_LOG(LogRTSyncUnitAnimRefs, Error, TEXT("Scrittura interrotta. Gia' salvati: %s"),
				Scritti.Num() > 0 ? *FString::Join(Scritti, TEXT(", ")) : TEXT("nessuno"));
			return 1;
		}
		UE_LOG(LogRTSyncUnitAnimRefs, Display, TEXT("%s: scritto"), *P.Blueprint->GetOutermost()->GetName());
	}

	// ── Fase 3: rileggere ──────────────────────────────────────────────────────────────────────────────────
	for (const FRTPianoBlueprint& P : Piani)
	{
		const FString Package = P.Blueprint->GetOutermost()->GetName();
		const TSet<FName> Dure = RTDipendenzeDure(Registro, Package, /*bRileggi=*/ true);
		if (!Dure.Contains(FName(RTAuthoredPackageName)))
		{
			Errore(FString::Printf(TEXT("%s: dopo il salvataggio la classe autorata non e' una dipendenza dura"), *Package));
		}
		for (const UAnimSequenceBase* Clip : P.Clip)
		{
			const FName ClipPackage = Clip->GetOutermost()->GetFName();
			if (!Dure.Contains(ClipPackage))
			{
				Errore(FString::Printf(TEXT("%s: dopo il salvataggio %s non e' una dipendenza dura"), *Package,
					*ClipPackage.ToString()));
			}
		}
	}

	if (Errori > 0)
	{
		UE_LOG(LogRTSyncUnitAnimRefs, Error, TEXT("%d errori: vedi sopra."), Errori);
		return 1;
	}
	UE_LOG(LogRTSyncUnitAnimRefs, Display, TEXT("%d blueprint sincronizzati."), Piani.Num());
	return 0;
}
