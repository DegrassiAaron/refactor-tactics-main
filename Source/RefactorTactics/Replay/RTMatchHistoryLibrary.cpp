#include "Replay/RTMatchHistoryLibrary.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const TCHAR* RT_HISTORY_FILE = TEXT("history.rtindex");

	/**
	 * Versione del formato dell'indice, indipendente da quella del manifest.
	 *
	 * Sono due formati con due cicli di vita: l'indice puo' crescere di una colonna senza che l'archivio
	 * cambi di un byte, e un archivio puo' cambiare versione senza che la lista debba saperlo. Legarli
	 * significherebbe far dipendere la lista dal payload — cioe' il difetto che `#416` esiste per evitare.
	 */
	constexpr int32 RT_HISTORY_VERSION = 1;

	const TCHAR* K_VERSION   = TEXT("Version");
	const TCHAR* K_ENTRIES   = TEXT("Matches");
	const TCHAR* K_MATCH_ID  = TEXT("MatchId");
	const TCHAR* K_STARTED   = TEXT("StartedUtc");
	const TCHAR* K_FORMAT_ID = TEXT("FormatId");
	const TCHAR* K_HEX       = TEXT("HexTopology");
	const TCHAR* K_OUTCOME   = TEXT("Outcome");
	const TCHAR* K_TURNS     = TEXT("TurnCount");
	const TCHAR* K_WALLCLOCK = TEXT("WallClockSeconds");
	const TCHAR* K_COMPLETE  = TEXT("ReplayComplete");

	TSharedRef<FJsonObject> EntryToJson(const FRTMatchHistoryEntry& E)
	{
		const TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
		O->SetStringField(K_MATCH_ID, E.MatchId.ToString(EGuidFormats::Digits));
		// ISO-8601: si legge a occhio in un file di diagnosi, e non dipende dal locale di chi lo ha scritto.
		O->SetStringField(K_STARTED, E.StartedUtc.ToIso8601());
		O->SetStringField(K_FORMAT_ID, E.FormatId.ToString());
		O->SetBoolField(K_HEX, E.bHexTopology);
		O->SetNumberField(K_OUTCOME, static_cast<double>(static_cast<uint8>(E.Outcome)));
		O->SetNumberField(K_TURNS, E.TurnCount);
		O->SetNumberField(K_WALLCLOCK, E.WallClockSeconds);
		O->SetBoolField(K_COMPLETE, E.bReplayComplete);
		return O;
	}

	bool EntryFromJson(const TSharedPtr<FJsonObject>& O, FRTMatchHistoryEntry& Out)
	{
		if (!O.IsValid())
		{
			return false;
		}

		FString IdText;
		if (!O->TryGetStringField(K_MATCH_ID, IdText) || !FGuid::Parse(IdText, Out.MatchId))
		{
			// Una riga senza identita' non e' una riga incompleta: e' una riga che non indicizza niente.
			return false;
		}

		FString Started;
		if (O->TryGetStringField(K_STARTED, Started))
		{
			FDateTime::ParseIso8601(*Started, Out.StartedUtc);
		}

		FString FormatText;
		if (O->TryGetStringField(K_FORMAT_ID, FormatText)) { Out.FormatId = FName(*FormatText); }
		O->TryGetBoolField(K_HEX, Out.bHexTopology);

		double Outcome = 0.0;
		if (O->TryGetNumberField(K_OUTCOME, Outcome))
		{
			Out.Outcome = static_cast<ERTMatchOutcome>(static_cast<uint8>(Outcome));
		}

		double Turns = 0.0;
		if (O->TryGetNumberField(K_TURNS, Turns)) { Out.TurnCount = static_cast<int32>(Turns); }

		double Wall = 0.0;
		if (O->TryGetNumberField(K_WALLCLOCK, Wall)) { Out.WallClockSeconds = static_cast<float>(Wall); }

		O->TryGetBoolField(K_COMPLETE, Out.bReplayComplete);
		return true;
	}
}

FString URTMatchHistoryLibrary::IndexPath(const FString& ReplaysRoot)
{
	return FPaths::Combine(ReplaysRoot, RT_HISTORY_FILE);
}

FRTMatchHistoryEntry URTMatchHistoryLibrary::EntryFromManifest(const FRTReplayManifest& Manifest,
	const FDateTime& StartedUtc)
{
	FRTMatchHistoryEntry E;
	E.MatchId = Manifest.MatchId;
	E.StartedUtc = StartedUtc;
	E.FormatId = Manifest.FormatId;
	E.bHexTopology = Manifest.bHexTopology;
	E.Outcome = Manifest.Outcome;
	E.TurnCount = Manifest.TurnCount;
	E.WallClockSeconds = Manifest.WallClockSeconds;

	// La disponibilita' del replay E' la chiusura del manifest, e non un terzo stato da mantenere allineato:
	// un archivio chiuso si riapre per intero, uno non chiuso si riapre fino a dove arriva.
	E.bReplayComplete = Manifest.bClosed;
	return E;
}

bool URTMatchHistoryLibrary::AppendOrUpdate(const FString& ReplaysRoot, const FRTMatchHistoryEntry& Entry)
{
	if (!Entry.MatchId.IsValid())
	{
		return false;
	}

	// Si parte da cio' che c'e': l'indice e' cumulativo, e riscriverlo da zero perderebbe le partite di ieri.
	TArray<FRTMatchHistoryEntry> Entries;
	LoadIndex(ReplaysRoot, Entries);

	const int32 Existing = Entries.IndexOfByPredicate(
		[&Entry](const FRTMatchHistoryEntry& E) { return E.MatchId == Entry.MatchId; });
	if (Existing != INDEX_NONE)
	{
		Entries[Existing] = Entry;
	}
	else
	{
		Entries.Add(Entry);
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(K_VERSION, RT_HISTORY_VERSION);

	TArray<TSharedPtr<FJsonValue>> Values;
	Values.Reserve(Entries.Num());
	for (const FRTMatchHistoryEntry& E : Entries)
	{
		Values.Add(MakeShared<FJsonValueObject>(EntryToJson(E)));
	}
	Root->SetArrayField(K_ENTRIES, Values);

	FString Json;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
	FJsonSerializer::Serialize(Root, Writer);

	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	if (!PF.CreateDirectoryTree(*ReplaysRoot))
	{
		return false;
	}

	// Temporaneo + move, per la stessa ragione del manifest: l'indice si riscrive a ogni partita, e un crash a
	// meta' scrittura cancellerebbe la lista di TUTTE le partite precedenti — un danno molto piu' grande di
	// quello che si stava scrivendo.
	const FString Finale = IndexPath(ReplaysRoot);
	const FString Temporaneo = Finale + TEXT(".tmp");
	if (!FFileHelper::SaveStringToFile(Json, *Temporaneo))
	{
		return false;
	}
	if (!IFileManager::Get().Move(*Finale, *Temporaneo, /*bReplace*/ true))
	{
		IFileManager::Get().Delete(*Temporaneo, /*RequireExists*/ false);
		return false;
	}
	return true;
}

bool URTMatchHistoryLibrary::LoadIndex(const FString& ReplaysRoot, TArray<FRTMatchHistoryEntry>& OutEntries)
{
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *IndexPath(ReplaysRoot)))
	{
		return false;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	double Version = 0.0;
	if (!Root->TryGetNumberField(K_VERSION, Version)
		|| static_cast<int32>(Version) < 1
		|| static_cast<int32>(Version) > RT_HISTORY_VERSION)
	{
		// Fail-closed come ovunque nel replay: un indice che non si sa leggere si rifiuta, invece di mostrare
		// una lista parziale che sembra completa.
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
	if (!Root->TryGetArrayField(K_ENTRIES, Values) || Values == nullptr)
	{
		return false;
	}

	TArray<FRTMatchHistoryEntry> Letto;
	Letto.Reserve(Values->Num());
	for (const TSharedPtr<FJsonValue>& V : *Values)
	{
		const TSharedPtr<FJsonObject>* Obj = nullptr;
		FRTMatchHistoryEntry E;
		if (V.IsValid() && V->TryGetObject(Obj) && Obj && EntryFromJson(*Obj, E))
		{
			Letto.Add(E);
		}
	}

	OutEntries = Letto;
	return true;
}
