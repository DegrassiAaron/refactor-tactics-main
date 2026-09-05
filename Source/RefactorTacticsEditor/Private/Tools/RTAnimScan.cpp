#include "Tools/RTAnimScan.h"

#include "Animation/AnimSequence.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"

FRTAnimScanResult RTScanAnimSequencesUnder(const FString& PackageFolder)
{
	FRTAnimScanResult Esito;

	if (PackageFolder.IsEmpty())
	{
		Esito.MotivoNotRun = TEXT("nessuna cartella passata");
		return Esito;
	}

	// 🔴 **Prima si guarda il DISCO, e non il registry.** Il registry di un pack mai montato risponde
	// «nessun asset» senza errore, e quella risposta e' indistinguibile da «la cartella e' vuota». La
	// differenza conta: la prima e' un checkout senza i pack Paragon (il caso di ogni clone appena
	// creato, perche' `Content/FabAsset/` e' gitignorato), la seconda sarebbe un difetto vero.
	FString CartellaSuDisco;
	if (!FPackageName::TryConvertLongPackageNameToFilename(PackageFolder / TEXT(""), CartellaSuDisco))
	{
		Esito.MotivoNotRun = FString::Printf(
			TEXT("'%s' non e' un package path risolvibile"), *PackageFolder);
		return Esito;
	}
	if (!IFileManager::Get().DirectoryExists(*CartellaSuDisco))
	{
		Esito.MotivoNotRun = FString::Printf(
			TEXT("la cartella non esiste sul disco (%s): pack non installato, non e' un difetto"),
			*CartellaSuDisco);
		return Esito;
	}

	IAssetRegistry& Registry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	// ⚠️ Il registry non conosce una cartella che non ha mai scandito: `Content/FabAsset/` non e' fra
	// quelle montate all'avvio in ogni configurazione. Senza questa riga il filtro sotto restituisce un
	// elenco vuoto **senza errore**, e sarebbe di nuovo uno zero che sembra una misura.
	Registry.ScanPathsSynchronous({ PackageFolder }, /*bForceRescan*/ false);

	FARFilter Filtro;
	Filtro.PackagePaths.Add(FName(*PackageFolder));
	Filtro.bRecursivePaths = true;

	TArray<FAssetData> Trovati;
	Registry.GetAssets(Filtro, Trovati);

	if (Trovati.Num() == 0)
	{
		// La cartella c'e' ma il registry non ne sa niente: e' un ambiente in cui la misura non si puo'
		// fare, non un pack senza animazioni.
		Esito.MotivoNotRun = FString::Printf(
			TEXT("il registry non riporta nessun asset sotto '%s'"), *PackageFolder);
		return Esito;
	}

	// 🔑 **Si confronta la CLASSE, non il nome.** `AM_` e `BS_` danno zero risultati sotto `Animations/`
	// di Gadget: i blend space e gli aim offset stanno in sottocartelle e portano nomi qualunque.
	const FTopLevelAssetPath ClasseSequenza = UAnimSequence::StaticClass()->GetClassPathName();

	for (const FAssetData& Asset : Trovati)
	{
		if (Asset.AssetClassPath == ClasseSequenza)
		{
			Esito.SequencePaths.Add(Asset.PackageName.ToString());
		}
		else
		{
			++Esito.ScartatiPerClasse;
		}
	}

	// Ordine stabile: due scansioni della stessa cartella devono dare la stessa sequenza, altrimenti
	// un `AV_ID` assegnato in ordine di enumerazione dipenderebbe dall'ordine del registry.
	Esito.SequencePaths.Sort();

	Esito.bRan = true;
	return Esito;
}
