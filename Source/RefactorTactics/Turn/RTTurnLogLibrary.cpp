#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTActionFallbackLibrary.h" // ERTActionInvalidReason: il motivo del fallback, leggibile nel log
#include "Misc/FileHelper.h"

bool URTTurnLogLibrary::EntryLess(const FRTTurnLogEntry& A, const FRTTurnLogEntry& B)
{
	// Ordine totale: confronta ogni campo in sequenza, cosi' due voci diverse hanno sempre un ordine definito
	// (permutare l'input e riordinare -> stessa sequenza). Enum confrontati per valore intero (invariante #4).
	if (A.Phase != B.Phase)                 { return static_cast<uint8>(A.Phase) < static_cast<uint8>(B.Phase); }
	if (A.Category != B.Category)           { return static_cast<uint8>(A.Category) < static_cast<uint8>(B.Category); }
	if (A.SrcCell.X != B.SrcCell.X)         { return A.SrcCell.X < B.SrcCell.X; }
	if (A.SrcCell.Y != B.SrcCell.Y)         { return A.SrcCell.Y < B.SrcCell.Y; }
	if (A.SrcCell.Layer != B.SrcCell.Layer) { return A.SrcCell.Layer < B.SrcCell.Layer; }
	if (A.TgtCell.X != B.TgtCell.X)         { return A.TgtCell.X < B.TgtCell.X; }
	if (A.TgtCell.Y != B.TgtCell.Y)         { return A.TgtCell.Y < B.TgtCell.Y; }
	if (A.TgtCell.Layer != B.TgtCell.Layer) { return A.TgtCell.Layer < B.TgtCell.Layer; }
	if (A.Outcome != B.Outcome)             { return A.Outcome < B.Outcome; }
	return A.Amount < B.Amount;
}

void URTTurnLogLibrary::SortTurnLog(TArray<FRTTurnLogEntry>& Entries)
{
	Entries.Sort([](const FRTTurnLogEntry& A, const FRTTurnLogEntry& B) { return EntryLess(A, B); });
}

FString URTTurnLogLibrary::DescribeEntry(const FRTTurnLogEntry& Entry)
{
	auto CellText = [](const FRTCellId& Cell)
	{
		return FString::Printf(TEXT("(q=%d,r=%d,L=%d)"), Cell.X, Cell.Y, Cell.Layer);
	};

	if (Entry.Category == ERTLogCategory::Move)
	{
		const TCHAR* Reason = TEXT("");
		switch (static_cast<ERTMoveOutcome>(Entry.Outcome))
		{
		case ERTMoveOutcome::Moved:            Reason = TEXT("si muove"); break;
		case ERTMoveOutcome::BlockedContested: Reason = TEXT("fermo: cella contesa"); break;
		case ERTMoveOutcome::BlockedByUnit:    Reason = TEXT("fermo: cella occupata"); break;
		default:                               Reason = TEXT("resta"); break;
		}

		if (static_cast<ERTMoveOutcome>(Entry.Outcome) == ERTMoveOutcome::Moved)
		{
			return FString::Printf(TEXT("%s %s -> %s (%d celle)"),
				Reason, *CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount);
		}
		return FString::Printf(TEXT("%s %s"), Reason, *CellText(Entry.SrcCell));
	}

	// Fallback: cosa e' successo all'azione che non era piu' eseguibile. Il motivo per cui non lo era viaggia
	// in `Amount` (ERTActionInvalidReason): una riga che dice «annullata» senza dire da cosa non insegna nulla.
	if (Entry.Category == ERTLogCategory::Fallback)
	{
		const TCHAR* What = TEXT("");
		switch (static_cast<ERTFallbackOutcome>(Entry.Outcome))
		{
		case ERTFallbackOutcome::Stopped:      What = TEXT("fermata"); break;
		case ERTFallbackOutcome::Waited:       What = TEXT("sostituita con l'attesa"); break;
		case ERTFallbackOutcome::AttackedCell: What = TEXT("colpisce la cella pianificata"); break;
		default:                               What = TEXT("annullata"); break;
		}

		const TCHAR* Why = TEXT("");
		switch (static_cast<ERTActionInvalidReason>(Entry.Amount))
		{
		case ERTActionInvalidReason::TargetGone:     Why = TEXT("bersaglio assente"); break;
		case ERTActionInvalidReason::TargetDead:     Why = TEXT("bersaglio eliminato"); break;
		case ERTActionInvalidReason::TargetFriendly: Why = TEXT("bersaglio alleato"); break;
		case ERTActionInvalidReason::OutOfRange:     Why = TEXT("fuori portata"); break;
		case ERTActionInvalidReason::NoLineOfSight:  Why = TEXT("nessuna linea di tiro"); break;
		case ERTActionInvalidReason::NoMap:          Why = TEXT("nessuna mappa autorevole"); break;
		default:                                     Why = TEXT("non eseguibile"); break;
		}

		return FString::Printf(TEXT("%s -> %s: azione %s (%s)"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), What, Why);
	}

	// Combat: chi colpisce chi, con quale esito e quanto danno.
	switch (static_cast<ERTCombatOutcome>(Entry.Outcome))
	{
	case ERTCombatOutcome::NoLineOfSight:
		return FString::Printf(TEXT("%s -> %s: nessuna linea di tiro"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell));

	case ERTCombatOutcome::ShieldAbsorbed:
		return FString::Printf(TEXT("%s -> %s: %d assorbiti dallo scudo"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount);

	case ERTCombatOutcome::Lethal:
		return FString::Printf(TEXT("%s -> %s: %d danni, eliminata"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount);

	case ERTCombatOutcome::TerrainBonus:
		return FString::Printf(TEXT("%s -> %s: %d danni (bonus posizione)"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount);

	default:
		return FString::Printf(TEXT("%s -> %s: %d danni"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount);
	}
}

uint32 URTTurnLogLibrary::HashTurnLog(const TArray<FRTTurnLogEntry>& Entries)
{
	// Ordina prima di mescolare: stesso insieme di voci -> stessa sequenza -> stesso hash (permutazione-invariante).
	TArray<FRTTurnLogEntry> Sorted = Entries;
	SortTurnLog(Sorted);

	uint32 Hash = 2166136261u; // FNV-1a offset basis (32 bit)
	auto Mix = [&Hash](uint32 V)
	{
		Hash ^= V;
		Hash *= 16777619u; // FNV-1a prime (32 bit)
	};
	for (const FRTTurnLogEntry& E : Sorted)
	{
		Mix(static_cast<uint32>(E.Phase));
		Mix(static_cast<uint32>(E.Category));
		Mix(static_cast<uint32>(E.Outcome));
		Mix(static_cast<uint32>(E.SrcCell.X));
		Mix(static_cast<uint32>(E.SrcCell.Y));
		Mix(static_cast<uint32>(E.SrcCell.Layer));
		Mix(static_cast<uint32>(E.TgtCell.X));
		Mix(static_cast<uint32>(E.TgtCell.Y));
		Mix(static_cast<uint32>(E.TgtCell.Layer));
		Mix(static_cast<uint32>(E.Amount));
	}
	return Hash;
}

namespace
{
	// Magic 'RTTL' e helper little-endian espliciti: il formato non dipende dall'endianness della
	// piattaforma (determinismo/portabilita', invariante #4). Solo interi.
	constexpr uint32 RT_TURNLOG_MAGIC = 0x4C545452u; // byte su disco: 'R','T','T','L'

	void AppendU8(TArray<uint8>& B, uint8 V) { B.Add(V); }

	void AppendU16LE(TArray<uint8>& B, uint16 V)
	{
		B.Add(static_cast<uint8>(V & 0xFF));
		B.Add(static_cast<uint8>((V >> 8) & 0xFF));
	}

	void AppendU32LE(TArray<uint8>& B, uint32 V)
	{
		B.Add(static_cast<uint8>(V & 0xFF));
		B.Add(static_cast<uint8>((V >> 8) & 0xFF));
		B.Add(static_cast<uint8>((V >> 16) & 0xFF));
		B.Add(static_cast<uint8>((V >> 24) & 0xFF));
	}

	void AppendI32LE(TArray<uint8>& B, int32 V) { AppendU32LE(B, static_cast<uint32>(V)); }

	// Letture con bounds-check: ritornano false invece di leggere fuori dal buffer (parser sicuro).
	bool ReadU8(const TArray<uint8>& B, int32& Pos, uint8& Out)
	{
		if (Pos + 1 > B.Num()) { return false; }
		Out = B[Pos];
		Pos += 1;
		return true;
	}

	bool ReadU16LE(const TArray<uint8>& B, int32& Pos, uint16& Out)
	{
		if (Pos + 2 > B.Num()) { return false; }
		Out = static_cast<uint16>(static_cast<uint16>(B[Pos]) | (static_cast<uint16>(B[Pos + 1]) << 8));
		Pos += 2;
		return true;
	}

	bool ReadU32LE(const TArray<uint8>& B, int32& Pos, uint32& Out)
	{
		if (Pos + 4 > B.Num()) { return false; }
		Out = static_cast<uint32>(B[Pos])
			| (static_cast<uint32>(B[Pos + 1]) << 8)
			| (static_cast<uint32>(B[Pos + 2]) << 16)
			| (static_cast<uint32>(B[Pos + 3]) << 24);
		Pos += 4;
		return true;
	}

	bool ReadI32LE(const TArray<uint8>& B, int32& Pos, int32& Out)
	{
		uint32 U = 0;
		if (!ReadU32LE(B, Pos, U)) { return false; }
		Out = static_cast<int32>(U);
		return true;
	}

	// Checksum FNV-1a 32-bit sui byte grezzi (stesso mescolamento di HashTurnLog, ma sul buffer):
	// rileva la corruzione del contenuto che magic/versione da soli non catturano.
	uint32 FnvBytes(const uint8* Data, int32 Len)
	{
		uint32 H = 2166136261u; // offset basis
		for (int32 i = 0; i < Len; ++i)
		{
			H ^= Data[i];
			H *= 16777619u; // prime
		}
		return H;
	}
}

TArray<uint8> URTTurnLogLibrary::SerializeTurnLog(const TArray<FRTTurnLogEntry>& Entries, ERTLogTopology Topology)
{
	// Forma CANONICA: ordina con EntryLess prima di scrivere -> byte permutazione-invarianti (come l'hash).
	TArray<FRTTurnLogEntry> Canonical = Entries;
	SortTurnLog(Canonical);

	TArray<uint8> Out;
	Out.Reserve(12 + Canonical.Num() * 31);

	// Header: magic + versione + flags(topologia) + conteggio (tutto little-endian). Square = 0 -> i byte
	// restano identici a quelli scritti prima che il campo flags fosse usato (retrocompatibilita').
	AppendU32LE(Out, RT_TURNLOG_MAGIC);
	AppendU16LE(Out, static_cast<uint16>(ERTTurnLogFormatVersion::WithChecksum));
	AppendU16LE(Out, static_cast<uint16>(Topology));
	AppendU32LE(Out, static_cast<uint32>(Canonical.Num()));

	for (const FRTTurnLogEntry& E : Canonical)
	{
		AppendU8(Out, static_cast<uint8>(E.Phase));
		AppendU8(Out, static_cast<uint8>(E.Category));
		AppendU8(Out, E.Outcome);
		AppendI32LE(Out, E.SrcCell.X);
		AppendI32LE(Out, E.SrcCell.Y);
		AppendI32LE(Out, E.SrcCell.Layer);
		AppendI32LE(Out, E.TgtCell.X);
		AppendI32LE(Out, E.TgtCell.Y);
		AppendI32LE(Out, E.TgtCell.Layer);
		AppendI32LE(Out, E.Amount);
	}

	// Checksum FNV di tutto cio' che precede (header + voci), in coda: rileva la corruzione del contenuto.
	const uint32 Checksum = FnvBytes(Out.GetData(), Out.Num());
	AppendU32LE(Out, Checksum);
	return Out;
}

bool URTTurnLogLibrary::DeserializeTurnLog(const TArray<uint8>& Bytes, TArray<FRTTurnLogEntry>& OutEntries,
	ERTLogTopology* OutTopology)
{
	OutEntries.Reset();

	int32 Pos = 0;
	uint32 Magic = 0;
	if (!ReadU32LE(Bytes, Pos, Magic) || Magic != RT_TURNLOG_MAGIC) { return false; }

	uint16 Version = 0;
	if (!ReadU16LE(Bytes, Pos, Version) || Version != static_cast<uint16>(ERTTurnLogFormatVersion::WithChecksum)) { return false; }

	// Flags = topologia delle celle. Fail-closed sui valori sconosciuti (come per la versione): interpretare
	// coordinate di una topologia ignota produrrebbe un replay sbagliato in silenzio.
	uint16 Flags = 0;
	if (!ReadU16LE(Bytes, Pos, Flags)) { return false; }
	if (Flags != static_cast<uint16>(ERTLogTopology::Square) && Flags != static_cast<uint16>(ERTLogTopology::Hex))
	{
		return false;
	}
	if (OutTopology)
	{
		*OutTopology = static_cast<ERTLogTopology>(Flags);
	}

	uint32 Count = 0;
	if (!ReadU32LE(Bytes, Pos, Count)) { return false; }

	OutEntries.Reserve(static_cast<int32>(Count));
	for (uint32 i = 0; i < Count; ++i)
	{
		FRTTurnLogEntry E;
		uint8 Phase = 0;
		uint8 Category = 0;
		uint8 Outcome = 0;
		if (!ReadU8(Bytes, Pos, Phase) || !ReadU8(Bytes, Pos, Category) || !ReadU8(Bytes, Pos, Outcome))
		{
			OutEntries.Reset();
			return false;
		}
		E.Phase = static_cast<ERTMatchPhase>(Phase);
		E.Category = static_cast<ERTLogCategory>(Category);
		E.Outcome = Outcome;

		if (!ReadI32LE(Bytes, Pos, E.SrcCell.X) || !ReadI32LE(Bytes, Pos, E.SrcCell.Y) || !ReadI32LE(Bytes, Pos, E.SrcCell.Layer)
			|| !ReadI32LE(Bytes, Pos, E.TgtCell.X) || !ReadI32LE(Bytes, Pos, E.TgtCell.Y) || !ReadI32LE(Bytes, Pos, E.TgtCell.Layer)
			|| !ReadI32LE(Bytes, Pos, E.Amount))
		{
			OutEntries.Reset();
			return false;
		}
		OutEntries.Add(E);
	}

	// Verifica il checksum in coda: ricalcola FNV su header+voci e confronta (rileva corruzione del contenuto).
	const int32 PayloadEnd = Pos;
	uint32 StoredChecksum = 0;
	if (!ReadU32LE(Bytes, Pos, StoredChecksum))
	{
		OutEntries.Reset();
		return false;
	}
	if (FnvBytes(Bytes.GetData(), PayloadEnd) != StoredChecksum)
	{
		OutEntries.Reset();
		return false;
	}
	return true;
}

bool URTTurnLogLibrary::SaveTurnLogToFile(const FString& Path, const TArray<FRTTurnLogEntry>& Entries,
	ERTLogTopology Topology)
{
	const TArray<uint8> Bytes = SerializeTurnLog(Entries, Topology);
	return FFileHelper::SaveArrayToFile(Bytes, *Path);
}

bool URTTurnLogLibrary::LoadTurnLogFromFile(const FString& Path, TArray<FRTTurnLogEntry>& OutEntries,
	ERTLogTopology* OutTopology)
{
	OutEntries.Reset();
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *Path))
	{
		return false; // file mancante o illeggibile
	}
	return DeserializeTurnLog(Bytes, OutEntries, OutTopology);
}
