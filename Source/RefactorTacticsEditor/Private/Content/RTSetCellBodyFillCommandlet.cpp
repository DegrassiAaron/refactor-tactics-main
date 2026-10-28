#include "Content/RTSetCellBodyFillCommandlet.h"

#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTMapEditLibrary.h"
#include "Misc/PackageName.h"
#include "UObject/Class.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTCellBodyFill, Log, All);

namespace
{
	/** `q,r,layer` -> `FRTCellId`. Stessa forma di `RTSetCellDoor`: il layer e' opzionale e vale 0. */
	bool ParseBodyFillCell(const FString& Text, FRTCellId& Out)
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

	/**
	 * Il vocabolario dei valori e' quello dell'ENUM, letto per riflessione.
	 *
	 * ⚠️ Non una mappa scritta a mano come per `-State` di `RTSetCellDoor`: qui il vocabolario e' chiuso dal
	 * tipo (`None` · `Third` · `TwoThirds` · `Full`), e una seconda lista nel commandlet divergerebbe il giorno
	 * in cui un quinto valore entrasse nell'enum. Le grafie sono quelle dell'enum, senza maiuscole obbligate.
	 */
	bool ParseBodyFill(const FString& Text, ERTHexBodyFill& Out)
	{
		const UEnum* Enum = StaticEnum<ERTHexBodyFill>();
		if (Enum == nullptr)
		{
			return false;
		}
		// `EGetByNameFlags::None` e' gia' senza badare al caso (`CaseSensitive` e' l'opt-in), e accetta il nome
		// breve: `-Fill=full` e `-Fill=Full` sono la stessa cosa.
		const int32 Index = Enum->GetIndexByNameString(Text.TrimStartAndEnd());
		if (Index == INDEX_NONE || Index >= Enum->NumEnums() - 1) // l'ultimo e' `_MAX`, non un valore
		{
			return false;
		}
		Out = static_cast<ERTHexBodyFill>(Enum->GetValueByIndex(Index));
		return true;
	}

	FString DescribeBodyFill(ERTHexBodyFill Fill)
	{
		const UEnum* Enum = StaticEnum<ERTHexBodyFill>();
		return Enum ? Enum->GetNameStringByValue(static_cast<int64>(Fill)) : FString::FromInt(static_cast<int32>(Fill));
	}

	/** I valori ammessi, per il messaggio di errore: letti dall'enum, quindi mai in ritardo su di esso. */
	FString ListBodyFillValues()
	{
		const UEnum* Enum = StaticEnum<ERTHexBodyFill>();
		TArray<FString> Names;
		for (int32 I = 0; Enum && I < Enum->NumEnums() - 1; ++I)
		{
			Names.Add(Enum->GetNameStringByIndex(I));
		}
		return FString::Join(Names, TEXT(", "));
	}

	FString DescribeBodyFillCell(const FRTCellId& Id)
	{
		return FString::Printf(TEXT("(q=%d,r=%d,L=%d)"), Id.X, Id.Y, Id.Layer);
	}

	bool SaveBodyFillMapPackage(UObject* Asset)
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

URTSetCellBodyFillCommandlet::URTSetCellBodyFillCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URTSetCellBodyFillCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamsMap;
	ParseCommandLine(*Params, Tokens, Switches, ParamsMap);

	const bool bDryRun = Switches.Contains(TEXT("DryRun"));

	const FString MapPath = ParamsMap.Contains(TEXT("Map"))
		? ParamsMap[TEXT("Map")]
		: TEXT("/Game/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena");

	FRTCellId Target;
	const FString CellText = ParamsMap.FindRef(TEXT("Cell"));
	if (!ParseBodyFillCell(CellText, Target))
	{
		UE_LOG(LogRTCellBodyFill, Error,
			TEXT("-Cell=q,r[,layer] mancante o illeggibile (ricevuto '%s')."), *CellText);
		return 1;
	}

	// Nessun valore di default: il riempimento e' una scelta di contenuto, e un comando che ne indovinasse uno
	// scriverebbe sull'asset un'intenzione che l'autore non ha avuto.
	ERTHexBodyFill Fill = ERTHexBodyFill::None;
	const FString FillText = ParamsMap.FindRef(TEXT("Fill"));
	if (!ParseBodyFill(FillText, Fill))
	{
		UE_LOG(LogRTCellBodyFill, Error,
			TEXT("-Fill mancante o sconosciuto (ricevuto '%s'; attesi %s)."), *FillText, *ListBodyFillValues());
		return 1;
	}

	URTHexMapAsset* Map = LoadObject<URTHexMapAsset>(nullptr, *MapPath);
	if (!Map)
	{
		UE_LOG(LogRTCellBodyFill, Error, TEXT("mappa non caricata: %s"), *MapPath);
		return 1;
	}

	const uint32 HashPrima = Map->ComputeHash();
	const int32 RevisionePrima = Map->Revision;
	UE_LOG(LogRTCellBodyFill, Display, TEXT("mappa   %s — %d celle, formato v%d, hash %u, revisione %d"),
		*MapPath, Map->NumCells(), Map->FormatVersion, HashPrima, RevisionePrima);

	// La cella puo' non esserci: in quel caso e' la libreria a rifiutare, qui si legge solo per il «prima».
	const FRTHexCellData* Esistente = Map->FindCell(Target);
	const FString CorpoPrima = Esistente ? DescribeBodyFill(Esistente->BodyFill) : FString(TEXT("(cella assente)"));
	UE_LOG(LogRTCellBodyFill, Display, TEXT("prima   corpo di %s: %s"), *DescribeBodyFillCell(Target), *CorpoPrima);

	// ⛔ **La regola non e' qui.** `SetCellBodyFill` valida e scrive; questo commandlet nomina l'esito e salva.
	const ERTMapEditOutcome Esito = URTMapEditLibrary::SetCellBodyFill(Map, Target, Fill);
	if (Esito != ERTMapEditOutcome::Applied)
	{
		UE_LOG(LogRTCellBodyFill, Error, TEXT("scrittura RIFIUTATA su %s: %s"),
			*DescribeBodyFillCell(Target), *URTMapEditLibrary::DescribeOutcome(Esito));
		return 1;
	}

	const uint32 HashDopo = Map->ComputeHash();
	UE_LOG(LogRTCellBodyFill, Display,
		TEXT("scritto corpo di %s: %s -> %s — hash %u -> %u (%s), revisione %d -> %d"),
		*DescribeBodyFillCell(Target), *CorpoPrima, *DescribeBodyFill(Fill),
		HashPrima, HashDopo, HashPrima == HashDopo ? TEXT("invariato") : TEXT("MOSSO"),
		RevisionePrima, Map->Revision);

	// ⚠️ **L'hash e' invariato per costruzione** (`BodyFill` e' presentazione: primo non-goal di `#1865`,
	// pinnato da `StructuralBody.FillChangesNeitherCellsNorHash`). Se qui risulta mosso, il comando non ha
	// sbagliato: ha trovato che `ComputeHash` ha cominciato a leggere il campo, ed e' un difetto di un altro.
	if (HashPrima != HashDopo)
	{
		UE_LOG(LogRTCellBodyFill, Warning,
			TEXT("l'hash e' cambiato, e `BodyFill` non dovrebbe entrare in ComputeHash: controlla prima di salvare."));
	}

	if (bDryRun)
	{
		UE_LOG(LogRTCellBodyFill, Display, TEXT("-DryRun: nessuna scrittura."));
		return 0;
	}

	Map->MarkPackageDirty();
	if (!SaveBodyFillMapPackage(Map))
	{
		UE_LOG(LogRTCellBodyFill, Error, TEXT("salvataggio FALLITO: %s"), *MapPath);
		return 1;
	}
	UE_LOG(LogRTCellBodyFill, Display, TEXT("salvata: %s"), *MapPath);
	return 0;
}
