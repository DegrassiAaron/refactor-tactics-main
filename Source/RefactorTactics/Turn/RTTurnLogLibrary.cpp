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
