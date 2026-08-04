#include "Turn/RTTurnLogLibrary.h"

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
}

TArray<uint8> URTTurnLogLibrary::SerializeTurnLog(const TArray<FRTTurnLogEntry>& Entries)
{
	// Forma CANONICA: ordina con EntryLess prima di scrivere -> byte permutazione-invarianti (come l'hash).
	TArray<FRTTurnLogEntry> Canonical = Entries;
	SortTurnLog(Canonical);

	TArray<uint8> Out;
	Out.Reserve(12 + Canonical.Num() * 31);

	// Header: magic + versione + reserved/flags + conteggio (tutto little-endian).
	AppendU32LE(Out, RT_TURNLOG_MAGIC);
	AppendU16LE(Out, static_cast<uint16>(ERTTurnLogFormatVersion::Initial));
	AppendU16LE(Out, 0); // reserved/flags (spazio per estensioni future del formato)
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
	return Out;
}

bool URTTurnLogLibrary::DeserializeTurnLog(const TArray<uint8>& Bytes, TArray<FRTTurnLogEntry>& OutEntries)
{
	OutEntries.Reset();

	int32 Pos = 0;
	uint32 Magic = 0;
	if (!ReadU32LE(Bytes, Pos, Magic) || Magic != RT_TURNLOG_MAGIC) { return false; }

	uint16 Version = 0;
	if (!ReadU16LE(Bytes, Pos, Version) || Version != static_cast<uint16>(ERTTurnLogFormatVersion::Initial)) { return false; }

	uint16 Reserved = 0;
	if (!ReadU16LE(Bytes, Pos, Reserved)) { return false; }
	(void)Reserved;

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
	return true;
}
