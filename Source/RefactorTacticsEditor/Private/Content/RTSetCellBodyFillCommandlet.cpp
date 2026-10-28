#include "Content/RTSetCellBodyFillCommandlet.h"

#include "Core/RTEnumName.h"
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
		// Le parti vuote NON si scartano: `3,,1` letto come `(3,1,0)` scriverebbe su una cella che nessuno ha chiesto.
		Text.ParseIntoArray(Parts, TEXT(","), /*CullEmpty=*/ false);
		if (Parts.Num() < 2 || Parts.Num() > 3)
		{
			return false;
		}
		for (FString& P : Parts)
		{
			P.TrimStartAndEndInline();
			// Un intero con segno, e nient'altro: le coordinate assiali sono negative meta' delle volte, ma
			// `IsNumeric` accetta anche `1.9` e `1e3`, che `Atoi` troncherebbe in silenzio.
			bool bInteger = !P.IsEmpty();
			for (int32 I = 0; bInteger && I < P.Len(); ++I)
			{
				const bool bSign = (I == 0) && (P.Len() > 1) && (P[I] == TEXT('-') || P[I] == TEXT('+'));
				bInteger = bSign || FChar::IsDigit(P[I]);
			}
			if (!bInteger)
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
	 * tipo, e una seconda lista nel commandlet divergerebbe il giorno in cui un valore nuovo entrasse nell'enum.
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

	// Nessun default per la mappa: una dimenticanza riscriverebbe un asset tracciato che nessuno ha scelto.
	const FString MapPath = ParamsMap.FindRef(TEXT("Map"));
	if (MapPath.IsEmpty())
	{
		UE_LOG(LogRTCellBodyFill, Error, TEXT("-Map=/Game/... mancante: nessun default, e' una scelta di chi lancia."));
		return 1;
	}

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

	const uint32 HashBefore = Map->ComputeHash();
	const int32 RevisionBefore = Map->Revision;
	UE_LOG(LogRTCellBodyFill, Display, TEXT("mappa   %s — %d celle, formato v%d, hash %u, revisione %d"),
		*MapPath, Map->NumCells(), Map->FormatVersion, HashBefore, RevisionBefore);

	// Il salvataggio riscrive l'asset al formato corrente. L'hash non ne risente (si calcola dopo la migrazione),
	// ma il diff binario si': meglio vederlo prima di salvare che scoprirlo in una PR.
	if (Map->LoadedFormatVersion != INDEX_NONE && Map->LoadedFormatVersion != Map->FormatVersion)
	{
		UE_LOG(LogRTCellBodyFill, Warning, TEXT("l'asset e' su disco a formato v%d e verra' salvato a v%d."),
			Map->LoadedFormatVersion, Map->FormatVersion);
	}

	// La cella puo' non esserci: in quel caso e' la libreria a rifiutare, qui si legge solo per il «prima».
	const FRTHexCellData* Existing = Map->FindCell(Target);
	const FString FillBefore = Existing ? RTReflection::EnumName(Existing->BodyFill) : FString(TEXT("(cella assente)"));
	// Letto PRIMA della scrittura: `Existing` punta dentro l'array, e dopo vedrebbe gia' il valore nuovo.
	const bool bAlreadySet = (Existing != nullptr) && (Existing->BodyFill == Fill);
	UE_LOG(LogRTCellBodyFill, Display, TEXT("prima   corpo di %s: %s"), *DescribeBodyFillCell(Target), *FillBefore);

	// ⛔ **La regola non e' qui.** `SetCellBodyFill` valida e scrive; questo commandlet nomina l'esito e salva.
	const ERTMapEditOutcome Outcome = URTMapEditLibrary::SetCellBodyFill(Map, Target, Fill);
	if (Outcome != ERTMapEditOutcome::Applied)
	{
		UE_LOG(LogRTCellBodyFill, Error, TEXT("scrittura RIFIUTATA su %s: %s"),
			*DescribeBodyFillCell(Target), *URTMapEditLibrary::DescribeOutcome(Outcome));
		return 1;
	}

	const uint32 HashAfter = Map->ComputeHash();
	const bool bIdentityUnchanged = (HashBefore == HashAfter) && (RevisionBefore == Map->Revision);
	UE_LOG(LogRTCellBodyFill, Display,
		TEXT("%s corpo di %s: %s -> %s — hash %u -> %u, revisione %d -> %d (%s)"),
		bAlreadySet ? (bDryRun ? TEXT("confermerebbe") : TEXT("confermo"))
			: (bDryRun ? TEXT("scriverebbe") : TEXT("scritto")),
		*DescribeBodyFillCell(Target), *FillBefore, *RTReflection::EnumName(Fill),
		HashBefore, HashAfter, RevisionBefore, Map->Revision,
		bIdentityUnchanged ? TEXT("invariati") : TEXT("MOSSI"));

	// 🔴 **Hash e revisione sono invarianti per costruzione**: `BodyFill` e' presentazione (primo non-goal di
	// `#1865`), e `Revision` entra in `HashMatchState` e nel `GraphRevision` del TurnLog. Se uno dei due si e'
	// mosso, il comando non ha sbagliato: ha trovato che qualcosa ha cominciato a leggere il campo. Salvare
	// comunque scriverebbe nell'asset il difetto, quindi si rifiuta — anche in `-DryRun`, che deve dire la verita'.
	if (!bIdentityUnchanged)
	{
		UE_LOG(LogRTCellBodyFill, Error,
			TEXT("hash o revisione mossi da un gesto di sola presentazione: NON salvo. Controlla `SetCellBodyFill`."));
		return 1;
	}

	if (bDryRun)
	{
		UE_LOG(LogRTCellBodyFill, Display, TEXT("-DryRun: nessuna scrittura."));
		return 0;
	}

	// Un no-op non riscrive il binario: il corpo vale gia' quello chiesto.
	if (bAlreadySet)
	{
		UE_LOG(LogRTCellBodyFill, Display, TEXT("il corpo vale gia' %s: nessuna modifica, asset non salvato."),
			*RTReflection::EnumName(Fill));
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
