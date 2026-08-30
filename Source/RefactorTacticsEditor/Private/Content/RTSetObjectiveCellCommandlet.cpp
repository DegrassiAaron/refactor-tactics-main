#include "Content/RTSetObjectiveCellCommandlet.h"

#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTObjectiveCell, Log, All);

namespace
{
	/** `q,r,layer` -> `FRTCellId`. Il layer e' opzionale e vale 0, che e' il piano su cui sta tutto in v0.1. */
	bool ParseCell(const FString& Text, FRTCellId& Out)
	{
		TArray<FString> Parts;
		Text.ParseIntoArray(Parts, TEXT(","), /*CullEmpty=*/ true);
		if (Parts.Num() < 2 || Parts.Num() > 3)
		{
			return false;
		}
		for (FString& P : Parts)
		{
			P.TrimStartAndEndInline();
			// `IsNumeric` accetta il segno, e serve: le coordinate assiali sono negative meta' delle volte.
			if (P.IsEmpty() || !P.IsNumeric())
			{
				return false;
			}
		}
		Out = FRTCellId(FCString::Atoi(*Parts[0]), FCString::Atoi(*Parts[1]),
			Parts.Num() == 3 ? FCString::Atoi(*Parts[2]) : 0);
		return true;
	}

	FString Describe(const FRTCellId& Id)
	{
		return FString::Printf(TEXT("(q=%d,r=%d,L=%d)"), Id.X, Id.Y, Id.Layer);
	}

	bool SaveMapPackage(UObject* Asset)
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

URTSetObjectiveCellCommandlet::URTSetObjectiveCellCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URTSetObjectiveCellCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamsMap;
	ParseCommandLine(*Params, Tokens, Switches, ParamsMap);

	const bool bDryRun = Switches.Contains(TEXT("DryRun"));
	const bool bClear = Switches.Contains(TEXT("Clear"));
	const bool bForce = Switches.Contains(TEXT("Force"));

	const FString MapPath = ParamsMap.Contains(TEXT("Map"))
		? ParamsMap[TEXT("Map")]
		: TEXT("/Game/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena");

	FRTCellId Target;
	const FString CellText = ParamsMap.FindRef(TEXT("Cell"));
	if (!bClear && !ParseCell(CellText, Target))
	{
		UE_LOG(LogRTObjectiveCell, Error,
			TEXT("-Cell=q,r[,layer] mancante o illeggibile (ricevuto '%s'). Con -Clear non serve."), *CellText);
		return 1;
	}

	URTHexMapAsset* Map = LoadObject<URTHexMapAsset>(nullptr, *MapPath);
	if (!Map)
	{
		UE_LOG(LogRTObjectiveCell, Error, TEXT("mappa non caricata: %s"), *MapPath);
		return 1;
	}

	const uint32 HashPrima = Map->ComputeHash();
	const bool bAvevaObiettivo = Map->HasObjectiveCell();
	UE_LOG(LogRTObjectiveCell, Display, TEXT("mappa   %s — %d celle, formato v%d, hash %u"),
		*MapPath, Map->NumCells(), Map->FormatVersion, HashPrima);
	UE_LOG(LogRTObjectiveCell, Display, TEXT("prima   obiettivo: %s"),
		bAvevaObiettivo ? *Describe(Map->FirstObjectiveCell()) : TEXT("nessuno"));

	// --- ritiro ---------------------------------------------------------------------------------------
	if (bClear)
	{
		if (!bAvevaObiettivo)
		{
			UE_LOG(LogRTObjectiveCell, Display, TEXT("niente da ritirare: la mappa non dichiara obiettivi."));
			return 0;
		}
		int32 Ritirate = 0;
		for (const FRTHexCellData& C : Map->Cells)
		{
			if (C.bIsObjective)
			{
				FRTHexCellData Copia = C;
				Copia.bIsObjective = false;
				Map->AddOrUpdateCell(Copia);
				++Ritirate;
			}
		}
		UE_LOG(LogRTObjectiveCell, Display, TEXT("ritirate %d celle obiettivo."), Ritirate);
	}
	else
	{
		// --- verifiche, prima di toccare qualunque cosa -----------------------------------------------
		const FRTHexCellData* Cell = Map->FindCell(Target);
		if (!Cell)
		{
			UE_LOG(LogRTObjectiveCell, Error, TEXT("la cella %s non esiste in questa mappa (%d celle)."),
				*Describe(Target), Map->NumCells());
			return 1;
		}
		if (Cell->bBlocksMovement)
		{
			// Un obiettivo su cui nessuno puo' salire resterebbe `Unclaimed` per sempre, e nessun test
			// suonerebbe: la regola funzionerebbe perfettamente su un contenuto che non si puo' giocare.
			UE_LOG(LogRTObjectiveCell, Error,
				TEXT("la cella %s BLOCCA IL MOVIMENTO: un obiettivo li' non e' contendibile."), *Describe(Target));
			return 1;
		}
		if (bAvevaObiettivo && !(Map->FirstObjectiveCell() == Target) && !bForce)
		{
			UE_LOG(LogRTObjectiveCell, Error,
				TEXT("la mappa dichiara gia' l'obiettivo %s. Piu' obiettivi simultanei sono CP 31.1 (post-v0.1) ")
				TEXT("e il TurnLog ne nomina uno solo: usa -Clear, oppure -Force se sai cosa stai facendo."),
				*Describe(Map->FirstObjectiveCell()));
			return 1;
		}
		if (Cell->bIsObjective)
		{
			UE_LOG(LogRTObjectiveCell, Display, TEXT("%s e' GIA' l'obiettivo: niente da fare."), *Describe(Target));
			return 0;
		}

		FRTHexCellData Copia = *Cell;
		Copia.bIsObjective = true;
		Map->AddOrUpdateCell(Copia);
		UE_LOG(LogRTObjectiveCell, Display, TEXT("dichiarata obiettivo la cella %s (superficie %d, costo %d)."),
			*Describe(Target), static_cast<int32>(Copia.Surface), Copia.TotalMoveCost());
	}

	const uint32 HashDopo = Map->ComputeHash();
	UE_LOG(LogRTObjectiveCell, Display, TEXT("dopo    obiettivo: %s — hash %u -> %u"),
		Map->HasObjectiveCell() ? *Describe(Map->FirstObjectiveCell()) : TEXT("nessuno"), HashPrima, HashDopo);

	if (bDryRun)
	{
		UE_LOG(LogRTObjectiveCell, Display, TEXT("-DryRun: nessuna scrittura."));
		return 0;
	}

	Map->MarkPackageDirty();
	if (!SaveMapPackage(Map))
	{
		UE_LOG(LogRTObjectiveCell, Error, TEXT("salvataggio FALLITO: %s"), *MapPath);
		return 1;
	}
	UE_LOG(LogRTObjectiveCell, Display, TEXT("salvata: %s"), *MapPath);
	return 0;
}
