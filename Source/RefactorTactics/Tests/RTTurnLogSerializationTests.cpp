#include "Misc/AutomationTest.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Core/RTTypes.h"
#include "Algo/Reverse.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FRTTurnLogEntry MakeEntry(ERTMatchPhase Phase, ERTLogCategory Cat, uint8 Outcome,
		const FRTGridCoord& Src, const FRTGridCoord& Tgt, int32 Amount)
	{
		FRTTurnLogEntry E;
		E.Phase = Phase;
		E.Category = Cat;
		E.Outcome = Outcome;
		E.SrcCell = Src;
		E.TgtCell = Tgt;
		E.Amount = Amount;
		return E;
	}

	// Log di esempio: movimento riuscito, movimento bloccato (non-evento), colpo letale su un layer diverso.
	// Copre i tre campi interi che contano per hash e round-trip (Phase/Category/Outcome, Src/Tgt XYZ, Amount).
	TArray<FRTTurnLogEntry> SampleLog()
	{
		TArray<FRTTurnLogEntry> L;
		L.Add(MakeEntry(ERTMatchPhase::Move,  ERTLogCategory::Move,   static_cast<uint8>(ERTMoveOutcome::Moved),
			FRTGridCoord(0, 0), FRTGridCoord(1, 0), 1));
		L.Add(MakeEntry(ERTMatchPhase::Move,  ERTLogCategory::Move,   static_cast<uint8>(ERTMoveOutcome::BlockedContested),
			FRTGridCoord(4, 2), FRTGridCoord(4, 2), 0));
		L.Add(MakeEntry(ERTMatchPhase::Blast, ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Lethal),
			FRTGridCoord(2, 2, 1), FRTGridCoord(3, 3), 45));
		return L;
	}
}

// Cuore dello slice: serializzare e poi deserializzare deve preservare il TurnLog a livello di hash.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogSerializeRoundTripTest,
	"RefactorTactics.TurnLog.SerializeRoundTripPreservesHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogSerializeRoundTripTest::RunTest(const FString&)
{
	const TArray<FRTTurnLogEntry> Log = SampleLog();
	const uint32 ExpectedHash = URTTurnLogLibrary::HashTurnLog(Log);

	const TArray<uint8> Bytes = URTTurnLogLibrary::SerializeTurnLog(Log);
	TestTrue(TEXT("il buffer serializzato non e' vuoto"), Bytes.Num() > 0);

	TArray<FRTTurnLogEntry> Restored;
	const bool bOk = URTTurnLogLibrary::DeserializeTurnLog(Bytes, Restored);
	TestTrue(TEXT("deserializzazione riuscita"), bOk);
	TestEqual(TEXT("stesso numero di voci dopo il round-trip"), Restored.Num(), Log.Num());
	TestEqual(TEXT("hash preservato dal round-trip"), URTTurnLogLibrary::HashTurnLog(Restored), ExpectedHash);
	return true;
}

// La serializzazione e' CANONICA: stesse voci in ordine diverso -> byte identici (come l'hash,
// permutazione-invariante). Rende i replay confrontabili/deduplicabili byte-per-byte.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogSerializeCanonicalTest,
	"RefactorTactics.TurnLog.SerializeCanonicalPermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogSerializeCanonicalTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Original = SampleLog();
	TArray<FRTTurnLogEntry> Shuffled = Original;
	Algo::Reverse(Shuffled); // stesse voci, ordine d'inserimento invertito

	const TArray<uint8> BytesA = URTTurnLogLibrary::SerializeTurnLog(Original);
	const TArray<uint8> BytesB = URTTurnLogLibrary::SerializeTurnLog(Shuffled);

	TestTrue(TEXT("stessi byte a prescindere dall'ordine d'inserimento"), BytesA == BytesB);
	return true;
}

// Robustezza (fail-closed): un buffer con magic errato e' rifiutato senza crash, OutEntries svuotato.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogRejectsBadMagicTest,
	"RefactorTactics.TurnLog.DeserializeRejectsBadMagic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogRejectsBadMagicTest::RunTest(const FString&)
{
	TArray<uint8> Bytes = URTTurnLogLibrary::SerializeTurnLog(SampleLog());
	Bytes[0] ^= 0xFF; // corrompe il primo byte del magic

	TArray<FRTTurnLogEntry> Out;
	const bool bOk = URTTurnLogLibrary::DeserializeTurnLog(Bytes, Out);
	TestFalse(TEXT("magic errato -> rifiutato"), bOk);
	TestEqual(TEXT("nessuna voce restituita"), Out.Num(), 0);
	return true;
}

// Robustezza (fail-closed): una versione sconosciuta e' rifiutata invece di interpretare byte arbitrari.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogRejectsUnknownVersionTest,
	"RefactorTactics.TurnLog.DeserializeRejectsUnknownVersion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogRejectsUnknownVersionTest::RunTest(const FString&)
{
	TArray<uint8> Bytes = URTTurnLogLibrary::SerializeTurnLog(SampleLog());
	Bytes[4] = 0xEE; // versione = uint16 LE all'offset 4
	Bytes[5] = 0xEE; // -> valore sconosciuto

	TArray<FRTTurnLogEntry> Out;
	const bool bOk = URTTurnLogLibrary::DeserializeTurnLog(Bytes, Out);
	TestFalse(TEXT("versione sconosciuta -> rifiutata"), bOk);
	TestEqual(TEXT("nessuna voce restituita"), Out.Num(), 0);
	return true;
}

// Edge (caratterizzazione): un log vuoto fa round-trip a un log vuoto (solo header, zero voci).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogSerializeEmptyTest,
	"RefactorTactics.TurnLog.SerializeEmptyRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogSerializeEmptyTest::RunTest(const FString&)
{
	const TArray<FRTTurnLogEntry> Empty;
	const TArray<uint8> Bytes = URTTurnLogLibrary::SerializeTurnLog(Empty);
	TestTrue(TEXT("header presente anche per log vuoto"), Bytes.Num() >= 12);

	TArray<FRTTurnLogEntry> Out;
	Out.Add(FRTTurnLogEntry()); // sporca l'output per verificare che venga svuotato
	const bool bOk = URTTurnLogLibrary::DeserializeTurnLog(Bytes, Out);
	TestTrue(TEXT("deserializzazione riuscita"), bOk);
	TestEqual(TEXT("zero voci"), Out.Num(), 0);
	return true;
}

// Robustezza (caratterizzazione del bounds-check): un buffer troncato e' rifiutato senza crash.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogRejectsTruncatedTest,
	"RefactorTactics.TurnLog.DeserializeRejectsTruncated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogRejectsTruncatedTest::RunTest(const FString&)
{
	TArray<uint8> Bytes = URTTurnLogLibrary::SerializeTurnLog(SampleLog());
	Bytes.SetNum(Bytes.Num() - 5); // taglia gli ultimi byte dell'ultima voce

	TArray<FRTTurnLogEntry> Out;
	const bool bOk = URTTurnLogLibrary::DeserializeTurnLog(Bytes, Out);
	TestFalse(TEXT("buffer troncato -> rifiutato"), bOk);
	TestEqual(TEXT("nessuna voce restituita"), Out.Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
