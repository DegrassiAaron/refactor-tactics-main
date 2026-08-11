#include "Replay/RTReplayRecorderLibrary.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const TCHAR* RT_MANIFEST_FILE = TEXT("match.rtmanifest");

	// Chiavi del formato. Costanti e non letterali sparsi: una chiave scritta a mano in due punti e' un
	// campo che si perde alla prima svista di battitura, e il round-trip verde non se ne accorgerebbe
	// perche' scrittura e lettura sbaglierebbero insieme.
	const TCHAR* K_VERSION   = TEXT("Version");
	const TCHAR* K_MATCH_ID  = TEXT("MatchId");
	const TCHAR* K_FORMAT_ID = TEXT("FormatId");
	const TCHAR* K_HEX       = TEXT("HexTopology");
	const TCHAR* K_HASHES    = TEXT("OrderedHashPerTurn");
	const TCHAR* K_FINAL     = TEXT("FinalStateHash");
	const TCHAR* K_OUTCOME   = TEXT("Outcome");
	const TCHAR* K_WALLCLOCK = TEXT("WallClockSeconds");
	const TCHAR* K_CLOSED    = TEXT("Closed");
	const TCHAR* K_TURNS     = TEXT("TurnCount");

	/**
	 * Scrive il manifest su un temporaneo e poi lo sposta sopra il definitivo.
	 *
	 * Il manifest viene riscritto a OGNI turno, e senza questo passaggio un crash a meta' scrittura
	 * corromperebbe un file che dopo il turno precedente era valido — mentre tutte le tracce su disco
	 * restano intatte. Sarebbe il fallimento peggiore possibile per un componente che esiste proprio per
	 * sopravvivere ai crash: l'archivio diventerebbe illeggibile nel momento in cui serve.
	 */
	bool ScriviManifestAtomico(const FString& Dir, const FString& Json)
	{
		const FString Finale = FPaths::Combine(Dir, RT_MANIFEST_FILE);
		const FString Temporaneo = Finale + TEXT(".tmp");

		if (!FFileHelper::SaveStringToFile(Json, *Temporaneo))
		{
			return false;
		}
		// `Move` con sovrascrittura: il file definitivo passa dal contenuto vecchio a quello nuovo senza
		// stati intermedi visibili a chi legge.
		if (!IFileManager::Get().Move(*Finale, *Temporaneo, /*bReplace*/ true))
		{
			IFileManager::Get().Delete(*Temporaneo, /*RequireExists*/ false);
			return false;
		}
		return true;
	}
}

FString URTReplayRecorderLibrary::ManifestToJson(const FRTReplayManifest& Manifest)
{
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();

	// La versione per PRIMA, ed e' l'unico campo che un lettore deve saper trovare sempre: e' cio' che
	// gli permette di rifiutare il resto senza interpretarlo.
	Root->SetNumberField(K_VERSION, static_cast<double>(ERTReplayManifestVersion::Initial));
	Root->SetStringField(K_MATCH_ID, Manifest.MatchId.ToString(EGuidFormats::Digits));
	Root->SetStringField(K_FORMAT_ID, Manifest.FormatId.ToString());
	Root->SetBoolField(K_HEX, Manifest.bHexTopology);

	// Gli hash sono `uint32` e viaggiano come numeri JSON, cioe' come double: fino a 2^53 la conversione e'
	// ESATTA, quindi nessuna perdita. Vale la pena saperlo se un giorno un hash diventasse a 64 bit —
	// allora andrebbero scritti come stringhe, o il round-trip comincerebbe a mentire sugli ultimi bit.
	TArray<TSharedPtr<FJsonValue>> Hashes;
	Hashes.Reserve(Manifest.OrderedHashPerTurn.Num());
	for (const int64 H : Manifest.OrderedHashPerTurn)
	{
		Hashes.Add(MakeShared<FJsonValueNumber>(static_cast<double>(H)));
	}
	Root->SetArrayField(K_HASHES, Hashes);

	Root->SetNumberField(K_FINAL, static_cast<double>(Manifest.FinalStateHash));
	// L'esito come INTERO e non come nome: un enum serializzato per nome si rompe alla prima rinomina, e
	// i valori di `ERTMatchOutcome` sono accodati per convenzione, quindi il numero e' stabile.
	Root->SetNumberField(K_OUTCOME, static_cast<double>(static_cast<uint8>(Manifest.Outcome)));
	Root->SetNumberField(K_WALLCLOCK, Manifest.WallClockSeconds);
	Root->SetBoolField(K_CLOSED, Manifest.bClosed);
	Root->SetNumberField(K_TURNS, Manifest.TurnCount);

	FString Out;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	return Out;
}

bool URTReplayRecorderLibrary::ManifestFromJson(const FString& Json, FRTReplayManifest& OutManifest)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	double Version = 0.0;
	if (!Root->TryGetNumberField(K_VERSION, Version))
	{
		return false;
	}

	// ⚠️ **Un INTERVALLO, non un'uguaglianza** (`#471`). L'uguaglianza sembrava fail-closed ed era anche
	// altro: il giorno in cui `Current` fosse salito a `2`, ogni archivio gia' scritto sarebbe diventato
	// illeggibile — cioe' il formato avrebbe rotto la retrocompatibilita' al primo campo aggiunto, che e'
	// esattamente cio' che il TurnLog ha evitato per cinque versioni di fila accodando i campi in coda.
	//
	// Sotto `Initial` non c'e' niente da leggere, sopra `Current` c'e' un formato che questo binario non
	// conosce: entrambi si rifiutano invece di interpretare campi arbitrari (ADR-0009 §4).
	const uint16 Letta = static_cast<uint16>(Version);
	if (Letta < static_cast<uint16>(ERTReplayManifestVersion::Initial)
		|| Letta > static_cast<uint16>(ERTReplayManifestVersion::Current))
	{
		return false;
	}

	// Si popola un TEMPORANEO e si assegna solo alla fine: su un rifiuto a meta' lettura il chiamante
	// deve ritrovare il proprio manifest com'era, non mezzo sovrascritto.
	FRTReplayManifest Letto;

	FString IdText;
	if (!Root->TryGetStringField(K_MATCH_ID, IdText) || !FGuid::Parse(IdText, Letto.MatchId))
	{
		return false;
	}

	FString FormatText;
	if (Root->TryGetStringField(K_FORMAT_ID, FormatText)) { Letto.FormatId = FName(*FormatText); }
	Root->TryGetBoolField(K_HEX, Letto.bHexTopology);

	const TArray<TSharedPtr<FJsonValue>>* Hashes = nullptr;
	if (Root->TryGetArrayField(K_HASHES, Hashes) && Hashes)
	{
		Letto.OrderedHashPerTurn.Reserve(Hashes->Num());
		for (const TSharedPtr<FJsonValue>& V : *Hashes)
		{
			if (V.IsValid()) { Letto.OrderedHashPerTurn.Add(static_cast<int64>(V->AsNumber())); }
		}
	}

	double Final = 0.0;
	if (Root->TryGetNumberField(K_FINAL, Final)) { Letto.FinalStateHash = static_cast<int64>(Final); }

	double Outcome = 0.0;
	if (Root->TryGetNumberField(K_OUTCOME, Outcome))
	{
		Letto.Outcome = static_cast<ERTMatchOutcome>(static_cast<uint8>(Outcome));
	}

	double Wall = 0.0;
	if (Root->TryGetNumberField(K_WALLCLOCK, Wall)) { Letto.WallClockSeconds = static_cast<float>(Wall); }

	Root->TryGetBoolField(K_CLOSED, Letto.bClosed);

	double Turns = 0.0;
	if (Root->TryGetNumberField(K_TURNS, Turns)) { Letto.TurnCount = static_cast<int32>(Turns); }

	OutManifest = Letto;
	return true;
}

FString URTReplayRecorderLibrary::TurnFileName(int32 TurnNumber)
{
	// Zero-padded a tre cifre: cosi' l'ordine alfabetico di un elenco di cartella coincide con quello dei
	// turni. Con 6-12 turni per partita tre cifre bastano, e se un giorno non bastassero l'ordinamento
	// degraderebbe — non la lettura.
	return FString::Printf(TEXT("turn-%03d.rtlog"), TurnNumber);
}

FString URTReplayRecorderLibrary::MatchDirectory(const FString& ReplaysRoot, const FGuid& MatchId)
{
	return FPaths::Combine(ReplaysRoot, MatchId.ToString(EGuidFormats::Digits));
}

bool URTReplayRecorderLibrary::RecordTurn(const FString& ReplaysRoot, FRTReplayManifest& Manifest,
	int32 TurnNumber, const TArray<FRTTurnLogEntry>& Entries)
{
	// I turni si registrano in sequenza, dall'1. Un turno ripetuto o saltato farebbe divergere il numero di
	// hash nel manifest da quello dei file su disco, e da quel momento il manifest descriverebbe un archivio
	// che non esiste — senza che nulla se ne accorga. Rifiutare qui costa una riga; accorgersene dopo, no.
	if (TurnNumber != Manifest.OrderedHashPerTurn.Num() + 1)
	{
		return false;
	}

	const FString Dir = MatchDirectory(ReplaysRoot, Manifest.MatchId);
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	if (!PF.CreateDirectoryTree(*Dir))
	{
		return false;
	}

	// La traccia la serializza `SerializeTurnLog`, non un secondo serializzatore scritto qui: e' l'unico
	// modo di rendere VERO il criterio «byte-identiche», invece di ripromettersi di tenerli allineati.
	const TArray<uint8> Bytes = URTTurnLogLibrary::SerializeTurnLog(
		Entries,
		Manifest.bHexTopology ? ERTLogTopology::Hex : ERTLogTopology::Square,
		Manifest.FormatId);

	if (!FFileHelper::SaveArrayToFile(Bytes, *FPaths::Combine(Dir, TurnFileName(TurnNumber))))
	{
		return false;
	}

	// ⚠️ Si lavora su una COPIA e si assegna solo a scrittura riuscita. Mutare prima renderebbe il manifest
	// in memoria piu' avanti del disco su un fallimento — e chi ritenta si ritroverebbe un hash in piu' per
	// lo stesso turno. La regola vale doppio qui, perche' l'invariante di questa classe e' proprio che lo
	// stato del manifest dica la verita' su cosa e' stato scritto.
	FRTReplayManifest Aggiornato = Manifest;

	// L'hash ORDINATO, che e' il motivo per cui il manifest esiste (`D-062` gli assegna «l'header del Replay
	// Archive»; la forma per-turno e' di `D-077`): non e' ricalcolabile dai byte appena scritti, perche'
	// quelli sono in forma canonica e hanno perso l'ordine di append.
	Aggiornato.OrderedHashPerTurn.Add(static_cast<int64>(URTTurnLogLibrary::HashTurnLogOrdered(Entries)));
	Aggiornato.TurnCount = Aggiornato.OrderedHashPerTurn.Num();

	// Il manifest si riscrive a ogni turno, ancora NON chiuso: e' cosi' che una partita interrotta lascia
	// un archivio parziale e leggibile invece di niente.
	if (!ScriviManifestAtomico(Dir, ManifestToJson(Aggiornato)))
	{
		return false;
	}

	Manifest = Aggiornato;
	return true;
}

bool URTReplayRecorderLibrary::CloseMatch(const FString& ReplaysRoot, FRTReplayManifest& Manifest,
	ERTMatchOutcome Outcome, int64 FinalStateHash, float WallClockSeconds)
{
	// Come in `RecordTurn`: si prepara una copia e la si adotta solo a scrittura riuscita. Qui il rischio e'
	// il piu' grave di tutti — un `bClosed = true` in memoria dopo una chiusura FALLITA direbbe «partita
	// completa» mentre il disco dice il contrario, cioe' romperebbe l'invariante che regge l'intero design.
	FRTReplayManifest Chiuso = Manifest;
	Chiuso.Outcome = Outcome;
	Chiuso.FinalStateHash = FinalStateHash;
	Chiuso.WallClockSeconds = WallClockSeconds;
	Chiuso.bClosed = true;

	const FString Dir = MatchDirectory(ReplaysRoot, Chiuso.MatchId);
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	if (!PF.CreateDirectoryTree(*Dir) || !ScriviManifestAtomico(Dir, ManifestToJson(Chiuso)))
	{
		return false;
	}

	Manifest = Chiuso;
	return true;
}

bool URTReplayRecorderLibrary::LoadManifest(const FString& ReplaysRoot, const FGuid& MatchId,
	FRTReplayManifest& OutManifest)
{
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *FPaths::Combine(MatchDirectory(ReplaysRoot, MatchId), RT_MANIFEST_FILE)))
	{
		return false;
	}
	return ManifestFromJson(Json, OutManifest);
}
