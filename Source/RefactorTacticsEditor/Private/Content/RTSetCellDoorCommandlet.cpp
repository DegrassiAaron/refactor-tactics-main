#include "Content/RTSetCellDoorCommandlet.h"

#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTMapEditLibrary.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTCellDoor, Log, All);

namespace
{
	/** `q,r,layer` -> `FRTCellId`. Stessa forma di `RTSetObjectiveCell`: il layer e' opzionale e vale 0. */
	bool ParseDoorCell(const FString& Text, FRTCellId& Out)
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
	 * `E|NE|NW|W|SW|SE` -> `ERTHexDirection`.
	 *
	 * ⚠️ **Le sei grafie sono le stesse dello Scenario Harness** (`RTScenarioLoader`, *«attese E, NE, NW, W,
	 * SW, SE»*): due vocabolari per la stessa cosa sono due modi di sbagliare, e chi scrive uno scenario e
	 * chi lancia questo comando sono la stessa persona.
	 */
	bool ParseDoorEdge(const FString& Text, ERTHexDirection& Out)
	{
		const FString Upper = Text.TrimStartAndEnd().ToUpper();
		static const TMap<FString, ERTHexDirection> Nomi =
		{
			{ TEXT("E"),  ERTHexDirection::E  },
			{ TEXT("NE"), ERTHexDirection::NE },
			{ TEXT("NW"), ERTHexDirection::NW },
			{ TEXT("W"),  ERTHexDirection::W  },
			{ TEXT("SW"), ERTHexDirection::SW },
			{ TEXT("SE"), ERTHexDirection::SE },
		};
		if (const ERTHexDirection* Trovata = Nomi.Find(Upper))
		{
			Out = *Trovata;
			return true;
		}
		return false;
	}

	bool ParseDoorState(const FString& Text, ERTHexDoorState& Out)
	{
		const FString Upper = Text.TrimStartAndEnd().ToUpper();
		static const TMap<FString, ERTHexDoorState> Nomi =
		{
			{ TEXT("OPEN"),      ERTHexDoorState::Open      },
			{ TEXT("CLOSED"),    ERTHexDoorState::Closed    },
			{ TEXT("LOCKED"),    ERTHexDoorState::Locked    },
			{ TEXT("DESTROYED"), ERTHexDoorState::Destroyed },
		};
		if (const ERTHexDoorState* Trovato = Nomi.Find(Upper))
		{
			Out = *Trovato;
			return true;
		}
		return false;
	}

	FString DescribeDoorCell(const FRTCellId& Id)
	{
		return FString::Printf(TEXT("(q=%d,r=%d,L=%d)"), Id.X, Id.Y, Id.Layer);
	}

	/** Il rifiuto si NOMINA, con la ragione: e' la disciplina di `ERTMapEditOutcome`. */
	FString DescribeOutcome(ERTMapEditOutcome Outcome)
	{
		switch (Outcome)
		{
		case ERTMapEditOutcome::Applied:              return TEXT("applicata");
		case ERTMapEditOutcome::RefusedNoSuchCell:    return TEXT("la cella non esiste in questa mappa");
		case ERTMapEditOutcome::RefusedNoNeighbour:   return TEXT("oltre quel bordo non c'e' nessuna cella: "
			"una porta e' SOTTRATTIVA, e li' non negherebbe nessuna adiacenza");
		case ERTMapEditOutcome::RefusedDuplicate:     return TEXT("quel bordo ha gia' una porta");
		default:                                      return TEXT("rifiutata");
		}
	}

	bool SaveDoorMapPackage(UObject* Asset)
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

URTSetCellDoorCommandlet::URTSetCellDoorCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URTSetCellDoorCommandlet::Main(const FString& Params)
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
	if (!ParseDoorCell(CellText, Target))
	{
		UE_LOG(LogRTCellDoor, Error,
			TEXT("-Cell=q,r[,layer] mancante o illeggibile (ricevuto '%s')."), *CellText);
		return 1;
	}

	ERTHexDirection Edge;
	const FString EdgeText = ParamsMap.FindRef(TEXT("Edge"));
	if (!ParseDoorEdge(EdgeText, Edge))
	{
		UE_LOG(LogRTCellDoor, Error,
			TEXT("-Edge mancante o sconosciuto (ricevuto '%s'; attese E, NE, NW, W, SW, SE)."), *EdgeText);
		return 1;
	}

	// `Closed` di default: e' l'unico stato in cui una porta e' un oggetto DA ATTIVARE, che e' la ragione
	// per cui si posa una porta.
	ERTHexDoorState State = ERTHexDoorState::Closed;
	if (ParamsMap.Contains(TEXT("State")) && !ParseDoorState(ParamsMap[TEXT("State")], State))
	{
		UE_LOG(LogRTCellDoor, Error,
			TEXT("-State sconosciuto (ricevuto '%s'; attesi Open, Closed, Locked, Destroyed)."),
			*ParamsMap[TEXT("State")]);
		return 1;
	}

	const int32 DoorId = ParamsMap.Contains(TEXT("DoorId"))
		? FCString::Atoi(*ParamsMap[TEXT("DoorId")])
		: INDEX_NONE;
	const FName StableId = ParamsMap.Contains(TEXT("StableId"))
		? FName(*ParamsMap[TEXT("StableId")])
		: NAME_None;

	URTHexMapAsset* Map = LoadObject<URTHexMapAsset>(nullptr, *MapPath);
	if (!Map)
	{
		UE_LOG(LogRTCellDoor, Error, TEXT("mappa non caricata: %s"), *MapPath);
		return 1;
	}

	const uint32 HashPrima = Map->ComputeHash();
	const int32 RevisionePrima = Map->Revision;
	UE_LOG(LogRTCellDoor, Display, TEXT("mappa   %s — %d celle, formato v%d, hash %u, revisione %d"),
		*MapPath, Map->NumCells(), Map->FormatVersion, HashPrima, RevisionePrima);

	int32 PorteDiPrima = 0;
	for (const FRTHexCellData& C : Map->Cells)
	{
		PorteDiPrima += C.Doors.Num();
	}
	UE_LOG(LogRTCellDoor, Display, TEXT("prima   porte in mappa: %d"), PorteDiPrima);

	// ⛔ **La regola non e' qui.** `AddDoor` valida e scrive; questo commandlet nomina l'esito e salva.
	const ERTMapEditOutcome Esito = URTMapEditLibrary::AddDoor(Map, Target, Edge, State, DoorId, StableId);
	if (Esito != ERTMapEditOutcome::Applied)
	{
		UE_LOG(LogRTCellDoor, Error, TEXT("posa RIFIUTATA su %s bordo %s: %s"),
			*DescribeDoorCell(Target), *EdgeText, *DescribeOutcome(Esito));
		return 1;
	}

	const uint32 HashDopo = Map->ComputeHash();
	int32 PorteDiDopo = 0;
	for (const FRTHexCellData& C : Map->Cells)
	{
		PorteDiDopo += C.Doors.Num();
	}
	UE_LOG(LogRTCellDoor, Display,
		TEXT("posata porta su %s bordo %s stato %s%s — porte %d -> %d, hash %u -> %u, revisione %d -> %d"),
		*DescribeDoorCell(Target), *EdgeText,
		ParamsMap.Contains(TEXT("State")) ? *ParamsMap[TEXT("State")] : TEXT("Closed"),
		StableId.IsNone() ? TEXT("") : *FString::Printf(TEXT(" id '%s'"), *StableId.ToString()),
		PorteDiPrima, PorteDiDopo, HashPrima, HashDopo, RevisionePrima, Map->Revision);

	if (bDryRun)
	{
		UE_LOG(LogRTCellDoor, Display, TEXT("-DryRun: nessuna scrittura."));
		return 0;
	}

	Map->MarkPackageDirty();
	if (!SaveDoorMapPackage(Map))
	{
		UE_LOG(LogRTCellDoor, Error, TEXT("salvataggio FALLITO: %s"), *MapPath);
		return 1;
	}
	UE_LOG(LogRTCellDoor, Display, TEXT("salvata: %s"), *MapPath);

	// ⚠️ **Il conteggio delle porte cambia, e un oracolo lo sa**: `HexMap.AuthoredArenaDoorCountMatchesTheRoadmap`
	// (`#2312`) pinna quel numero su `DA_HexMap_Arena` e diventera' rosso. E' il suo mestiere: il rosso si
	// chiude aggiornando il test **e** la riga di `roadmap-v0.1.md` §2, non togliendo l'asserzione.
	UE_LOG(LogRTCellDoor, Warning,
		TEXT("promemoria: se la mappa e' DA_HexMap_Arena, aggiorna HexMap.AuthoredArenaDoorCountMatchesTheRoadmap "
			"e roadmap-v0.1.md §2 — il conteggio delle porte e' pinnato."));
	return 0;
}
