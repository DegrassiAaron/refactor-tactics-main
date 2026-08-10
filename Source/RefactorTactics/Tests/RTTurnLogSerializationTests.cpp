#include "Misc/AutomationTest.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Core/RTTypes.h"
#include "Algo/Reverse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nome distinto da quello di RTTurnLogLibraryTests.cpp: nella unity build i due file finiscono nella STESSA
	// translation unit e i namespace anonimi si fondono -> due helper omonime sarebbero una ridefinizione.
	FRTTurnLogEntry MakeSerEntry(ERTMatchPhase Phase, ERTLogCategory Cat, uint8 Outcome,
		const FRTCellId& Src, const FRTCellId& Tgt, int32 Amount)
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
		L.Add(MakeSerEntry(ERTMatchPhase::Move,  ERTLogCategory::Move,   static_cast<uint8>(ERTMoveOutcome::Moved),
			FRTCellId(0, 0), FRTCellId(1, 0), 1));
		L.Add(MakeSerEntry(ERTMatchPhase::Move,  ERTLogCategory::Move,   static_cast<uint8>(ERTMoveOutcome::BlockedContested),
			FRTCellId(4, 2), FRTCellId(4, 2), 0));
		L.Add(MakeSerEntry(ERTMatchPhase::Blast, ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Lethal),
			FRTCellId(2, 2, 1), FRTCellId(3, 3), 45));
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

// SR.check: un bit-flip nel PAYLOAD (magic e versione restano validi) e' rilevato dal checksum del formato.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogChecksumTest,
	"RefactorTactics.TurnLog.DeserializeDetectsPayloadCorruption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogChecksumTest::RunTest(const FString&)
{
	TArray<uint8> Bytes = URTTurnLogLibrary::SerializeTurnLog(SampleLog());
	// offset 13 = dentro la prima voce (header = 12 byte); magic (0..3) e versione (4..5) restano validi.
	Bytes[13] ^= 0xFF;

	TArray<FRTTurnLogEntry> Out;
	const bool bOk = URTTurnLogLibrary::DeserializeTurnLog(Bytes, Out);
	TestFalse(TEXT("corruzione del contenuto rilevata dal checksum"), bOk);
	TestEqual(TEXT("nessuna voce restituita"), Out.Num(), 0);
	return true;
}

// SR.file: salvare e ricaricare da file preserva il TurnLog (a livello di hash).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogFileRoundTripTest,
	"RefactorTactics.TurnLog.FileRoundTripPreservesHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogFileRoundTripTest::RunTest(const FString&)
{
	const TArray<FRTTurnLogEntry> Log = SampleLog();
	const uint32 ExpectedHash = URTTurnLogLibrary::HashTurnLog(Log);
	const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("turnlog_roundtrip.rttl"));

	const bool bSaved = URTTurnLogLibrary::SaveTurnLogToFile(Path, Log);
	TestTrue(TEXT("salvataggio su file riuscito"), bSaved);

	TArray<FRTTurnLogEntry> Restored;
	const bool bLoaded = URTTurnLogLibrary::LoadTurnLogFromFile(Path, Restored);
	TestTrue(TEXT("caricamento da file riuscito"), bLoaded);
	TestEqual(TEXT("hash preservato via file"), URTTurnLogLibrary::HashTurnLog(Restored), ExpectedHash);

	IFileManager::Get().Delete(*Path);
	return true;
}

// Robustezza (caratterizzazione): caricare un file inesistente fallisce senza crash, output svuotato.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogLoadMissingTest,
	"RefactorTactics.TurnLog.LoadMissingFileFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogLoadMissingTest::RunTest(const FString&)
{
	const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("turnlog_inesistente.rttl"));
	IFileManager::Get().Delete(*Path); // assicura che non esista

	TArray<FRTTurnLogEntry> Out;
	Out.Add(FRTTurnLogEntry()); // sporca l'output per verificare lo svuotamento
	const bool bLoaded = URTTurnLogLibrary::LoadTurnLogFromFile(Path, Out);
	TestFalse(TEXT("file inesistente -> false"), bLoaded);
	TestEqual(TEXT("output svuotato"), Out.Num(), 0);
	return true;
}

// Robustezza (caratterizzazione): un file valido corrotto su disco e' rifiutato dal checksum al load.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogLoadCorruptedTest,
	"RefactorTactics.TurnLog.LoadCorruptedFileFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogLoadCorruptedTest::RunTest(const FString&)
{
	const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("turnlog_corrotto.rttl"));
	TArray<uint8> Bytes = URTTurnLogLibrary::SerializeTurnLog(SampleLog());
	Bytes[13] ^= 0xFF; // corrompe un byte del payload
	const bool bWritten = FFileHelper::SaveArrayToFile(Bytes, *Path);
	TestTrue(TEXT("file di prova scritto"), bWritten);

	TArray<FRTTurnLogEntry> Out;
	const bool bLoaded = URTTurnLogLibrary::LoadTurnLogFromFile(Path, Out);
	TestFalse(TEXT("file corrotto -> rifiutato dal checksum"), bLoaded);
	TestEqual(TEXT("output svuotato"), Out.Num(), 0);

	IFileManager::Get().Delete(*Path);
	return true;
}

// Topologia dichiarata nei flags dell'header: un log esagonale non e' confondibile con uno quadrato.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogTopologyRoundTripTest,
	"RefactorTactics.TurnLog.TopologyRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogTopologyRoundTripTest::RunTest(const FString&)
{
	const TArray<FRTTurnLogEntry> Log = SampleLog();

	const TArray<uint8> HexBytes = URTTurnLogLibrary::SerializeTurnLog(Log, ERTLogTopology::Hex);
	TArray<FRTTurnLogEntry> Out;
	ERTLogTopology Topology = ERTLogTopology::Square;
	TestTrue(TEXT("log esagonale letto"), URTTurnLogLibrary::DeserializeTurnLog(HexBytes, Out, &Topology));
	TestTrue(TEXT("topologia = Hex"), Topology == ERTLogTopology::Hex);
	TestEqual(TEXT("hash preservato"), URTTurnLogLibrary::HashTurnLog(Out), URTTurnLogLibrary::HashTurnLog(Log));

	const TArray<uint8> SquareBytes = URTTurnLogLibrary::SerializeTurnLog(Log, ERTLogTopology::Square);
	Topology = ERTLogTopology::Hex;
	TestTrue(TEXT("log quadrato letto"), URTTurnLogLibrary::DeserializeTurnLog(SquareBytes, Out, &Topology));
	TestTrue(TEXT("topologia = Square"), Topology == ERTLogTopology::Square);

	TestNotEqual(TEXT("le due tracce differiscono nei byte"), HexBytes, SquareBytes);
	return true;
}

// Retrocompatibilita': il default resta Square e i byte non cambiano rispetto ai file gia' scritti (flags = 0).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogSquareBytesUnchangedTest,
	"RefactorTactics.TurnLog.SquareBytesUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogSquareBytesUnchangedTest::RunTest(const FString&)
{
	const TArray<FRTTurnLogEntry> Log = SampleLog();
	const TArray<uint8> Implicit = URTTurnLogLibrary::SerializeTurnLog(Log);
	const TArray<uint8> Explicit = URTTurnLogLibrary::SerializeTurnLog(Log, ERTLogTopology::Square);

	TestEqual(TEXT("il default e' Square"), Implicit, Explicit);
	TestTrue(TEXT("flags a zero nell'header"), Implicit.Num() > 8 && Implicit[6] == 0 && Implicit[7] == 0);

	// Un buffer scritto prima di questa modifica (flags = 0) resta leggibile.
	TArray<FRTTurnLogEntry> Out;
	TestTrue(TEXT("file legacy leggibile"), URTTurnLogLibrary::DeserializeTurnLog(Implicit, Out));
	TestEqual(TEXT("hash preservato"), URTTurnLogLibrary::HashTurnLog(Out), URTTurnLogLibrary::HashTurnLog(Log));
	return true;
}

// CP 5.5: l'identita' dell'azione sopravvive al round-trip, entra nell'hash e non collide con il vuoto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogActionIdRoundTripTest,
	"RefactorTactics.TurnLog.ActionIdRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogActionIdRoundTripTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Log;
	FRTTurnLogEntry Hero = MakeSerEntry(ERTMatchPhase::Blast, ERTLogCategory::Reaction, /*Attivata*/ 0,
		FRTCellId(1, 1), FRTCellId(1, 1), 0);
	Hero.ActionId = TEXT("Bastion.Interposition");
	Log.Add(Hero);

	// Stessa voce in tutto, tranne l'identita': senza il campo sarebbero indistinguibili.
	FRTTurnLogEntry Core = Hero;
	Core.ActionId = TEXT("Action.Intercept");
	Log.Add(Core);

	TArray<FRTTurnLogEntry> Restored;
	TestTrue(TEXT("round-trip riuscito"),
		URTTurnLogLibrary::DeserializeTurnLog(URTTurnLogLibrary::SerializeTurnLog(Log), Restored));
	TestEqual(TEXT("due voci distinte"), Restored.Num(), 2);
	TestEqual(TEXT("hash preservato"), URTTurnLogLibrary::HashTurnLog(Restored), URTTurnLogLibrary::HashTurnLog(Log));

	// L'ordine canonico e' deterministico: `Action.Intercept` precede `Bastion.Interposition`.
	if (Restored.Num() == 2)
	{
		TestTrue(TEXT("identita' preservata e ordinata lessicograficamente"),
			Restored[0].ActionId == FName(TEXT("Action.Intercept"))
			&& Restored[1].ActionId == FName(TEXT("Bastion.Interposition")));
	}

	// Due tracce che differiscono SOLO per l'identita' dell'azione hanno hash diversi: altrimenti il replay
	// non potrebbe dire quale abilita' e' scattata.
	TArray<FRTTurnLogEntry> OnlyCore;
	OnlyCore.Add(Core);
	TArray<FRTTurnLogEntry> OnlyHero;
	OnlyHero.Add(Hero);
	TestNotEqual(TEXT("l'identita' conta nell'hash"),
		URTTurnLogLibrary::HashTurnLog(OnlyCore), URTTurnLogLibrary::HashTurnLog(OnlyHero));
	return true;
}

// Retrocompatibilita': una traccia scritta prima di CP 5.5 (versione 2, senza ActionId) resta leggibile, e
// l'identita' resta vuota — che e' esattamente cio' che quei byte dicevano.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogLegacyVersionReadableTest,
	"RefactorTactics.TurnLog.LegacyVersionWithoutActionIdIsReadable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogLegacyVersionReadableTest::RunTest(const FString&)
{
	// Traccia in formato 2, costruita a mano: header (magic, versione 2, flags Square, conteggio) + una voce
	// a campi fissi, senza il campo a lunghezza variabile, + checksum FNV in coda.
	TArray<uint8> Bytes;
	auto U16 = [&Bytes](uint16 V) { Bytes.Add(V & 0xFF); Bytes.Add((V >> 8) & 0xFF); };
	auto U32 = [&Bytes](uint32 V)
	{
		Bytes.Add(V & 0xFF); Bytes.Add((V >> 8) & 0xFF); Bytes.Add((V >> 16) & 0xFF); Bytes.Add((V >> 24) & 0xFF);
	};

	U32(0x4C545452u); // 'RTTL'
	U16(static_cast<uint16>(ERTTurnLogFormatVersion::WithChecksum));
	U16(0); // topologia Square
	U32(1); // una voce
	Bytes.Add(static_cast<uint8>(ERTMatchPhase::Blast));
	Bytes.Add(static_cast<uint8>(ERTLogCategory::Combat));
	Bytes.Add(static_cast<uint8>(ERTCombatOutcome::Hit));
	U32(0); U32(0); U32(0);  // SrcCell
	U32(1); U32(0); U32(0);  // TgtCell
	U32(25);                 // Amount

	uint32 H = 2166136261u;
	for (const uint8 B : Bytes) { H ^= B; H *= 16777619u; }
	U32(H);

	TArray<FRTTurnLogEntry> Out;
	TestTrue(TEXT("una traccia in versione 2 resta leggibile"), URTTurnLogLibrary::DeserializeTurnLog(Bytes, Out));
	TestEqual(TEXT("una voce letta"), Out.Num(), 1);
	if (Out.Num() == 1)
	{
		TestEqual(TEXT("danno preservato"), Out[0].Amount, 25);
		TestTrue(TEXT("nessuna identita' inventata"), Out[0].ActionId.IsNone());
	}
	return true;
}

// Fail-closed: una topologia sconosciuta e' rifiutata invece di essere interpretata a caso.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogUnknownTopologyTest,
	"RefactorTactics.TurnLog.RejectsUnknownTopology",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogUnknownTopologyTest::RunTest(const FString&)
{
	TArray<uint8> Bytes = URTTurnLogLibrary::SerializeTurnLog(SampleLog(), ERTLogTopology::Hex);
	Bytes[6] = 0x7F; // topologia inesistente

	// Il checksum in coda copre l'header: va ricalcolato, altrimenti si verificherebbe solo il checksum.
	uint32 H = 2166136261u;
	for (int32 i = 0; i < Bytes.Num() - 4; ++i)
	{
		H ^= Bytes[i];
		H *= 16777619u;
	}
	Bytes[Bytes.Num() - 4] = static_cast<uint8>(H & 0xFF);
	Bytes[Bytes.Num() - 3] = static_cast<uint8>((H >> 8) & 0xFF);
	Bytes[Bytes.Num() - 2] = static_cast<uint8>((H >> 16) & 0xFF);
	Bytes[Bytes.Num() - 1] = static_cast<uint8>((H >> 24) & 0xFF);

	TArray<FRTTurnLogEntry> Out;
	Out.Add(FRTTurnLogEntry()); // sporca l'output per verificare lo svuotamento
	TestFalse(TEXT("topologia sconosciuta -> rifiutata"), URTTurnLogLibrary::DeserializeTurnLog(Bytes, Out));
	TestEqual(TEXT("output svuotato"), Out.Num(), 0);
	return true;
}

/**
 * Un conteggio di voci IMPLAUSIBILE va rifiutato, non allocato. Trovato mentre l'header cresceva (CP 10.3):
 * il byte corrotto di `DeserializeDetectsPayloadCorruption` e' finito sul campo del conteggio e `Reserve` ha
 * terminato il processo — il checksum in coda non fa in tempo a smentire un numero che uccide il parser
 * prima. Il difetto era latente da sempre: dipendeva solo da DOVE cadeva la corruzione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogRejectsImplausibleCountTest,
	"RefactorTactics.TurnLog.RejectsImplausibleEntryCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogRejectsImplausibleCountTest::RunTest(const FString&)
{
	TArray<uint8> Bytes;
	auto U16 = [&Bytes](uint16 V) { Bytes.Add(V & 0xFF); Bytes.Add((V >> 8) & 0xFF); };
	auto U32 = [&Bytes](uint32 V)
	{
		Bytes.Add(V & 0xFF); Bytes.Add((V >> 8) & 0xFF); Bytes.Add((V >> 16) & 0xFF); Bytes.Add((V >> 24) & 0xFF);
	};

	U32(0x4C545452u); // 'RTTL'
	U16(static_cast<uint16>(ERTTurnLogFormatVersion::WithFormatId));
	U16(static_cast<uint16>(ERTLogTopology::Hex));
	U16(0);           // FormatId vuoto
	U32(0xFF000003u); // quattro miliardi di voci in un buffer di venti byte
	U32(0);           // checksum qualsiasi: non ci si deve nemmeno arrivare

	TArray<FRTTurnLogEntry> Out;
	Out.Add(FRTTurnLogEntry());
	TestFalse(TEXT("conteggio impossibile -> rifiutato, senza allocare"),
		URTTurnLogLibrary::DeserializeTurnLog(Bytes, Out));
	TestEqual(TEXT("output svuotato"), Out.Num(), 0);
	return true;
}

// CP 10.3: l'IDENTITA' del formato di partita viaggia nell'header e sopravvive al round-trip.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogFormatIdRoundTripTest,
	"RefactorTactics.TurnLog.FormatIdRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogFormatIdRoundTripTest::RunTest(const FString&)
{
	const TArray<FRTTurnLogEntry> Log = SampleLog();
	const FName FormatId(TEXT("Format.Skirmish2v2"));

	TArray<FRTTurnLogEntry> Restored;
	ERTLogTopology Topology = ERTLogTopology::Square;
	FName ReadFormatId;
	TestTrue(TEXT("round-trip riuscito"), URTTurnLogLibrary::DeserializeTurnLog(
		URTTurnLogLibrary::SerializeTurnLog(Log, ERTLogTopology::Hex, FormatId), Restored, &Topology, &ReadFormatId));
	TestEqual(TEXT("identita' del formato preservata"), ReadFormatId, FormatId);
	TestTrue(TEXT("topologia preservata"), Topology == ERTLogTopology::Hex);
	TestEqual(TEXT("hash preservato"),
		URTTurnLogLibrary::HashTurnLog(Restored), URTTurnLogLibrary::HashTurnLog(Log));

	// Senza formato dichiarato l'identita' resta vuota: non si inventa un formato che nessuno ha scritto.
	FName NoFormat(TEXT("segnaposto"));
	TArray<FRTTurnLogEntry> Plain;
	TestTrue(TEXT("round-trip senza formato"),
		URTTurnLogLibrary::DeserializeTurnLog(URTTurnLogLibrary::SerializeTurnLog(Log), Plain, nullptr, &NoFormat));
	TestTrue(TEXT("identita' assente, non inventata"), NoFormat.IsNone());

	// Il file su disco porta lo stesso campo: e' li' che serve, quando due tracce vengono confrontate mesi dopo.
	const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Test_TurnLog_FormatId.rtlog"));
	TestTrue(TEXT("salvataggio riuscito"),
		URTTurnLogLibrary::SaveTurnLogToFile(Path, Log, ERTLogTopology::Hex, FormatId));
	TArray<FRTTurnLogEntry> FromFile;
	FName FromFileFormatId;
	TestTrue(TEXT("caricamento riuscito"),
		URTTurnLogLibrary::LoadTurnLogFromFile(Path, FromFile, nullptr, &FromFileFormatId));
	TestEqual(TEXT("identita' preservata su file"), FromFileFormatId, FormatId);
	IFileManager::Get().Delete(*Path);

	return true;
}

// Il formato NON entra nell'hash: includerlo invaliderebbe in blocco ogni hash golden gia' registrato, che e'
// esattamente cio' che la regola di rigenerazione di CP 12.6 esiste per impedire (issue #185).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogHashIgnoresFormatIdTest,
	"RefactorTactics.TurnLog.HashIgnoresFormatId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogHashIgnoresFormatIdTest::RunTest(const FString&)
{
	const TArray<FRTTurnLogEntry> Log = SampleLog();

	TArray<FRTTurnLogEntry> FromSkirmish;
	TArray<FRTTurnLogEntry> FromStandard;
	URTTurnLogLibrary::DeserializeTurnLog(
		URTTurnLogLibrary::SerializeTurnLog(Log, ERTLogTopology::Hex, FName(TEXT("Format.Skirmish2v2"))), FromSkirmish);
	URTTurnLogLibrary::DeserializeTurnLog(
		URTTurnLogLibrary::SerializeTurnLog(Log, ERTLogTopology::Hex, FName(TEXT("Format.Standard3v3"))), FromStandard);

	TestEqual(TEXT("stesse voci, stesso hash, formato diverso"),
		URTTurnLogLibrary::HashTurnLog(FromSkirmish), URTTurnLogLibrary::HashTurnLog(FromStandard));
	TestEqual(TEXT("e l'hash e' quello di sempre"),
		URTTurnLogLibrary::HashTurnLog(FromSkirmish), URTTurnLogLibrary::HashTurnLog(Log));
	return true;
}

// Retrocompatibilita': una traccia in versione 3 (con ActionId, senza FormatId) resta leggibile e l'identita'
// del formato resta neutra — come Square = 0 per la topologia e l'ActionId vuoto della versione 2.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogLegacyWithoutFormatIdTest,
	"RefactorTactics.TurnLog.LegacyVersionWithoutFormatIdIsReadable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogLegacyWithoutFormatIdTest::RunTest(const FString&)
{
	TArray<uint8> Bytes;
	auto U16 = [&Bytes](uint16 V) { Bytes.Add(V & 0xFF); Bytes.Add((V >> 8) & 0xFF); };
	auto U32 = [&Bytes](uint32 V)
	{
		Bytes.Add(V & 0xFF); Bytes.Add((V >> 8) & 0xFF); Bytes.Add((V >> 16) & 0xFF); Bytes.Add((V >> 24) & 0xFF);
	};

	U32(0x4C545452u); // 'RTTL'
	U16(static_cast<uint16>(ERTTurnLogFormatVersion::WithActionId));
	U16(static_cast<uint16>(ERTLogTopology::Hex));
	U32(1); // una voce, e NESSUN campo di formato fra flags e conteggio
	Bytes.Add(static_cast<uint8>(ERTMatchPhase::Blast));
	Bytes.Add(static_cast<uint8>(ERTLogCategory::Combat));
	Bytes.Add(static_cast<uint8>(ERTCombatOutcome::Hit));
	U32(0); U32(0); U32(0);  // SrcCell
	U32(1); U32(0); U32(0);  // TgtCell
	U32(25);                 // Amount
	U16(0);                  // ActionId vuoto (versione 3)

	uint32 H = 2166136261u;
	for (const uint8 B : Bytes) { H ^= B; H *= 16777619u; }
	U32(H);

	TArray<FRTTurnLogEntry> Out;
	FName FormatId(TEXT("segnaposto"));
	TestTrue(TEXT("una traccia in versione 3 resta leggibile"),
		URTTurnLogLibrary::DeserializeTurnLog(Bytes, Out, nullptr, &FormatId));
	TestEqual(TEXT("una voce letta"), Out.Num(), 1);
	TestTrue(TEXT("nessun formato inventato"), FormatId.IsNone());
	if (Out.Num() == 1)
	{
		TestEqual(TEXT("danno preservato"), Out[0].Amount, 25);
	}
	return true;
}

/**
 * Retrocompatibilita' del formato 4 (issue #354): una traccia scritta PRIMA che esistesse `BaseActionId`
 * resta leggibile, e il campo resta vuoto — che e' esattamente cio' che quei byte dicevano. E' la meta'
 * «vecchio -> nuovo» della verifica a due binari: qui il binario nuovo legge byte del vecchio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogLegacyWithoutBaseActionIdTest,
	"RefactorTactics.TurnLog.LegacyVersionWithoutBaseActionIdIsReadable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogLegacyWithoutBaseActionIdTest::RunTest(const FString&)
{
	TArray<uint8> Bytes;
	auto U16 = [&Bytes](uint16 V) { Bytes.Add(V & 0xFF); Bytes.Add((V >> 8) & 0xFF); };
	auto U32 = [&Bytes](uint32 V)
	{
		Bytes.Add(V & 0xFF); Bytes.Add((V >> 8) & 0xFF); Bytes.Add((V >> 16) & 0xFF); Bytes.Add((V >> 24) & 0xFF);
	};
	auto Str = [&Bytes, &U16](const char* S)
	{
		const int32 Len = FCStringAnsi::Strlen(S);
		U16(static_cast<uint16>(Len));
		for (int32 i = 0; i < Len; ++i) { Bytes.Add(static_cast<uint8>(S[i])); }
	};

	U32(0x4C545452u); // 'RTTL'
	U16(static_cast<uint16>(ERTTurnLogFormatVersion::WithFormatId)); // versione 4: ActionId si', BaseActionId no
	U16(static_cast<uint16>(ERTLogTopology::Hex));
	Str("Format.Skirmish2v2"); // FormatId (presente dalla 4)
	U32(1);
	Bytes.Add(static_cast<uint8>(ERTMatchPhase::Blast));
	Bytes.Add(static_cast<uint8>(ERTLogCategory::Combat));
	Bytes.Add(static_cast<uint8>(ERTCombatOutcome::Hit));
	U32(0); U32(0); U32(0);
	U32(1); U32(0); U32(0);
	U32(8);
	Str("Bastion.ImpactShot"); // ActionId c'e'; BaseActionId NON e' scritto: il formato 4 non lo prevede

	uint32 H = 2166136261u;
	for (const uint8 B : Bytes) { H ^= B; H *= 16777619u; }
	U32(H);

	TArray<FRTTurnLogEntry> Out;
	TestTrue(TEXT("una traccia in versione 4 resta leggibile"),
		URTTurnLogLibrary::DeserializeTurnLog(Bytes, Out, nullptr, nullptr));
	if (!TestEqual(TEXT("una voce letta"), Out.Num(), 1)) { return false; }
	TestEqual(TEXT("l'ActionId e' preservato"), Out[0].ActionId, FName(TEXT("Bastion.ImpactShot")));
	// Il punto del test: NON inventare. Una traccia vecchia non sapeva che ImpactShot fosse un attacco base,
	// e il loader non deve dedurlo — dedurlo significherebbe scrivere nella traccia un'informazione che quei
	// byte non contenevano, cioe' esattamente il difetto che il versionamento esiste per evitare.
	TestTrue(TEXT("BaseActionId resta VUOTO: non si inventa cio' che i byte non dicevano"),
		Out[0].BaseActionId.IsNone());
	// E la descrizione degrada senza rompersi: dice l'ActionId da solo, non «None · Bastion.ImpactShot».
	TestEqual(TEXT("la descrizione ricade sul solo ActionId"),
		URTTurnLogLibrary::DescribeActionIdentity(Out[0]), FString(TEXT("Bastion.ImpactShot")));
	return true;
}

/**
 * Il round-trip del campo nuovo, e la proprieta' che rende sicura tutta l'operazione: **l'hash non cambia**.
 * `BaseActionId` e' una funzione di `ActionId`, quindi mescolarlo non distinguerebbe nulla di piu' — e
 * avrebbe invalidato in blocco ogni hash golden. Se un giorno qualcuno lo aggiunge all'hash, questo test
 * cade, ed e' il punto: la scelta e' deliberata e va ridiscussa, non cambiata di passaggio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogBaseActionIdRoundTripTest,
	"RefactorTactics.TurnLog.BaseActionIdSurvivesRoundTripAndStaysOutOfHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogBaseActionIdRoundTripTest::RunTest(const FString&)
{
	FRTTurnLogEntry E;
	E.Phase = ERTMatchPhase::Blast;
	E.Category = ERTLogCategory::Combat;
	E.Outcome = static_cast<uint8>(ERTCombatOutcome::Hit);
	E.Amount = 8;
	E.ActionId = TEXT("Bastion.ImpactShot");

	FRTTurnLogEntry WithBase = E;
	WithBase.BaseActionId = TEXT("Action.BasicAttack");

	// 1. Round-trip: il campo sopravvive alla serializzazione.
	const TArray<uint8> Bytes = URTTurnLogLibrary::SerializeTurnLog({ WithBase }, ERTLogTopology::Hex, NAME_None);
	TArray<FRTTurnLogEntry> Out;
	TestTrue(TEXT("la traccia si rilegge"), URTTurnLogLibrary::DeserializeTurnLog(Bytes, Out, nullptr, nullptr));
	if (!TestEqual(TEXT("una voce"), Out.Num(), 1)) { return false; }
	TestEqual(TEXT("BaseActionId sopravvive"), Out[0].BaseActionId, FName(TEXT("Action.BasicAttack")));

	// 2. L'hash NON cambia: e' cio' che tiene validi gli hash golden gia' registrati.
	TestEqual(TEXT("l'hash e' lo stesso con e senza BaseActionId"),
		URTTurnLogLibrary::HashTurnLog({ WithBase }), URTTurnLogLibrary::HashTurnLog({ E }));

	// 3. La descrizione mostra la coppia (D-033), e non ripete se i due coincidono.
	TestEqual(TEXT("azione base + profilo"),
		URTTurnLogLibrary::DescribeActionIdentity(Out[0]),
		FString(TEXT("Action.BasicAttack · Bastion.ImpactShot")));
	FRTTurnLogEntry Generic = E;
	Generic.ActionId = TEXT("Action.BasicAttack");
	Generic.BaseActionId = TEXT("Action.BasicAttack");
	TestEqual(TEXT("un'azione generica usata direttamente non si ripete due volte"),
		URTTurnLogLibrary::DescribeActionIdentity(Generic), FString(TEXT("Action.BasicAttack")));
	return true;
}

/**
 * Il confronto fra tracce verifica il FORMATO prima degli hash e fallisce dicendo *formato diverso*, non
 * *divergenza*: se l'hash non copre il formato, deve coprirlo la procedura di confronto, altrimenti il falso
 * "nessuna divergenza" (o il falso allarme) ricompare un piano piu' su — issue #185, spec §16.3.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGoldenCorpusFormatMismatchTest,
	"RefactorTactics.Simulation.GoldenCorpusRejectsFormatMismatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGoldenCorpusFormatMismatchTest::RunTest(const FString&)
{
	const TArray<FRTTurnLogEntry> Log = SampleLog();
	const FName Skirmish(TEXT("Format.Skirmish2v2"));
	const FName Standard(TEXT("Format.Standard3v3"));

	const TArray<uint8> Golden = URTTurnLogLibrary::SerializeTurnLog(Log, ERTLogTopology::Hex, Skirmish);

	// Stesse voci, stesso formato: identiche.
	TestTrue(TEXT("stessa traccia, stesso formato -> identiche"),
		URTTurnLogLibrary::CompareSerializedTraces(Golden,
			URTTurnLogLibrary::SerializeTurnLog(Log, ERTLogTopology::Hex, Skirmish))
		== ERTTraceComparison::Identical);

	// Stesse voci, formato diverso: il verdetto NON e' "divergenza". Le due tracce sono uguali voce per voce
	// proprio perche' il limite non ha ancora morso: attribuirlo al codice sarebbe l'errore da evitare.
	TestTrue(TEXT("formato diverso -> mismatch di formato, non divergenza"),
		URTTurnLogLibrary::CompareSerializedTraces(Golden,
			URTTurnLogLibrary::SerializeTurnLog(Log, ERTLogTopology::Hex, Standard))
		== ERTTraceComparison::FormatMismatch);

	// Una traccia senza formato dichiarato non e' confrontabile con una che lo dichiara.
	TestTrue(TEXT("formato assente da un lato -> mismatch di formato"),
		URTTurnLogLibrary::CompareSerializedTraces(Golden,
			URTTurnLogLibrary::SerializeTurnLog(Log, ERTLogTopology::Hex))
		== ERTTraceComparison::FormatMismatch);

	// Topologia diversa: stesso ruolo, stesso trattamento.
	TestTrue(TEXT("topologia diversa -> mismatch di topologia"),
		URTTurnLogLibrary::CompareSerializedTraces(Golden,
			URTTurnLogLibrary::SerializeTurnLog(Log, ERTLogTopology::Square, Skirmish))
		== ERTTraceComparison::TopologyMismatch);

	// Stesso formato, voci diverse: QUESTA e' una divergenza, ed e' l'unico caso in cui va detto.
	TArray<FRTTurnLogEntry> Diverged = Log;
	Diverged[0].Amount += 1;
	TestTrue(TEXT("stesso formato, esito diverso -> divergenza"),
		URTTurnLogLibrary::CompareSerializedTraces(Golden,
			URTTurnLogLibrary::SerializeTurnLog(Diverged, ERTLogTopology::Hex, Skirmish))
		== ERTTraceComparison::Divergence);

	// Un buffer illeggibile non e' ne' identico ne' divergente: e' illeggibile.
	TArray<uint8> Corrupted = Golden;
	Corrupted[0] ^= 0xFF; // magic rotto
	TestTrue(TEXT("traccia illeggibile -> dichiarata tale"),
		URTTurnLogLibrary::CompareSerializedTraces(Golden, Corrupted) == ERTTraceComparison::Unreadable);

	return true;
}

/**
 * D-062, il test che rende il secondo hash una verifica invece che un ornamento.
 *
 * `HashTurnLog` ordina prima di mescolare, quindi un riordino delle emissioni gli e' invisibile: oggi un ciclo
 * che iterasse una `TMap` cambierebbe la traccia lasciando ogni hash identico, e nessun golden se ne
 * accorgerebbe. `HashTurnLogOrdered` guarda l'ordine di append, che e' l'ordine in cui il resolver ha
 * davvero emesso.
 *
 * Le DUE asserzioni servono entrambe: la prima fissa il contratto vecchio (che resta), la seconda quello
 * nuovo. Senza la prima, un'implementazione che rompesse la permutazione-invarianza passerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogOrderedHashDetectsReorderingTest,
	"RefactorTactics.TurnLog.OrderedHashDetectsReordering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogOrderedHashDetectsReorderingTest::RunTest(const FString&)
{
	const TArray<FRTTurnLogEntry> Straight = SampleLog();
	TArray<FRTTurnLogEntry> Reordered = Straight;
	Algo::Reverse(Reordered); // stesse voci, ordine di emissione diverso

	TestEqual(TEXT("l'hash canonico resta cieco all'ordine (contratto invariato)"),
		URTTurnLogLibrary::HashTurnLog(Reordered), URTTurnLogLibrary::HashTurnLog(Straight));
	TestNotEqual(TEXT("l'hash ORDINATO vede il riordino"),
		URTTurnLogLibrary::HashTurnLogOrdered(Reordered), URTTurnLogLibrary::HashTurnLogOrdered(Straight));
	return true;
}

/**
 * Controprova: l'hash ordinato non e' una costante travestita da verifica, e non e' l'hash canonico con un
 * altro nome. Su una traccia gia' in forma canonica i due COINCIDONO — non e' un difetto, e' la definizione —
 * ma su una traccia emessa in altro ordine devono divergere. Senza questo test, `HashTurnLogOrdered` potrebbe
 * delegare a `HashTurnLog` e il test qui sopra sarebbe l'unico a saperlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogOrderedHashIsNotTheCanonicalOneTest,
	"RefactorTactics.TurnLog.OrderedHashIsNotTheCanonicalOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogOrderedHashIsNotTheCanonicalOneTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Canonical = SampleLog();
	URTTurnLogLibrary::SortTurnLog(Canonical);
	TestEqual(TEXT("su una traccia gia' canonica i due hash coincidono"),
		URTTurnLogLibrary::HashTurnLogOrdered(Canonical), URTTurnLogLibrary::HashTurnLog(Canonical));

	TArray<FRTTurnLogEntry> Emitted = Canonical;
	Algo::Reverse(Emitted);
	TestNotEqual(TEXT("su una traccia emessa in altro ordine, no"),
		URTTurnLogLibrary::HashTurnLogOrdered(Emitted), URTTurnLogLibrary::HashTurnLog(Emitted));
	return true;
}

/**
 * Il limite di D-062, fissato da un test perche' altrimenti qualcuno scrivera' un «ricalcola dal file e
 * confronta» che passa sempre.
 *
 * `D-SR-1` scrive i byte in forma CANONICA (ordinata): la serializzazione **perde** l'ordine di emissione.
 * Quindi l'hash ordinato non e' ricalcolabile da un file — va calcolato prima di serializzare e conservato
 * altrove (l'header del Replay Archive, quando esistera'). Qui si pretende esattamente questo: il round-trip
 * preserva l'hash canonico e NON quello ordinato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogOrderedHashIsLostBySerializationTest,
	"RefactorTactics.TurnLog.OrderedHashIsLostBySerialization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogOrderedHashIsLostBySerializationTest::RunTest(const FString&)
{
	// Un log deliberatamente NON in forma canonica: se lo fosse, il test passerebbe per il motivo sbagliato.
	TArray<FRTTurnLogEntry> Emitted = SampleLog();
	Algo::Reverse(Emitted);
	TArray<FRTTurnLogEntry> Canonical = Emitted;
	URTTurnLogLibrary::SortTurnLog(Canonical);
	if (!TestTrue(TEXT("il log di prova NON e' gia' in forma canonica"),
		URTTurnLogLibrary::HashTurnLogOrdered(Emitted) != URTTurnLogLibrary::HashTurnLogOrdered(Canonical)))
	{
		return false;
	}

	const uint32 OrderedBefore = URTTurnLogLibrary::HashTurnLogOrdered(Emitted);

	TArray<FRTTurnLogEntry> Restored;
	if (!TestTrue(TEXT("round-trip riuscito"), URTTurnLogLibrary::DeserializeTurnLog(
		URTTurnLogLibrary::SerializeTurnLog(Emitted), Restored)))
	{
		return false;
	}

	TestEqual(TEXT("il round-trip preserva l'hash canonico"),
		URTTurnLogLibrary::HashTurnLog(Restored), URTTurnLogLibrary::HashTurnLog(Emitted));
	TestNotEqual(TEXT("ma NON quello ordinato: i byte tornano in forma canonica"),
		URTTurnLogLibrary::HashTurnLogOrdered(Restored), OrderedBefore);
	return true;
}

/**
 * D-063: `UnitId` e `TurnNumber` sopravvivono al round-trip (formato v6).
 *
 * Servono perche' la cella NON identifica l'attore in tre casi reali: le voci ambientali non hanno un'unita',
 * l'interposizione scrive la cella del PROTETTO, e dopo un Dash la cella in fase Blast non e' piu' quella di
 * partenza. `UnitId = 0` significa «nessuna unita'», ed e' cio' che una voce ambientale dice davvero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogUnitIdAndTurnRoundTripTest,
	"RefactorTactics.TurnLog.UnitIdAndTurnRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogUnitIdAndTurnRoundTripTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Log;
	FRTTurnLogEntry Acted = MakeSerEntry(ERTMatchPhase::Blast, ERTLogCategory::Combat,
		static_cast<uint8>(ERTCombatOutcome::Hit), FRTCellId(0, 0), FRTCellId(1, 0), 8);
	Acted.UnitId = 7;
	Acted.TurnNumber = 9;
	Log.Add(Acted);

	// Voce ambientale: nessuna unita' l'ha prodotta, e il campo deve poterlo dire.
	FRTTurnLogEntry Ambient = MakeSerEntry(ERTMatchPhase::Cleanup, ERTLogCategory::Move,
		static_cast<uint8>(ERTMoveOutcome::Stayed), FRTCellId(3, 3), FRTCellId(3, 3), 0);
	Ambient.UnitId = 0;
	Ambient.TurnNumber = 9;
	Log.Add(Ambient);

	TArray<FRTTurnLogEntry> Restored;
	if (!TestTrue(TEXT("round-trip riuscito"), URTTurnLogLibrary::DeserializeTurnLog(
		URTTurnLogLibrary::SerializeTurnLog(Log), Restored)))
	{
		return false;
	}
	if (!TestEqual(TEXT("due voci lette"), Restored.Num(), 2)) { return false; }

	// Le voci tornano in forma canonica: si cerca per contenuto, non per indice.
	const FRTTurnLogEntry* Combat = Restored.FindByPredicate(
		[](const FRTTurnLogEntry& E) { return E.Category == ERTLogCategory::Combat; });
	const FRTTurnLogEntry* Env = Restored.FindByPredicate(
		[](const FRTTurnLogEntry& E) { return E.Category == ERTLogCategory::Move; });
	if (!TestTrue(TEXT("entrambe le voci ritrovate"), Combat != nullptr && Env != nullptr)) { return false; }

	TestEqual(TEXT("UnitId preservato"), Combat->UnitId, 7);
	TestEqual(TEXT("TurnNumber preservato"), Combat->TurnNumber, 9);
	TestEqual(TEXT("una voce ambientale resta senza unita'"), Env->UnitId, 0);
	return true;
}

/**
 * D-063: i due campi nuovi restano FUORI dall'hash, come `FormatId` (v4) e `BaseActionId` (v5).
 *
 * Il criterio e' sempre lo stesso: un campo entra nell'hash se e solo se due tracce possono differire SOLO
 * per quel campo. `UnitId` e `TurnNumber` servono a rendere la traccia spiegabile, non a discriminarla —
 * e includerli invaliderebbe in blocco ogni hash golden, che e' cio' che la regola esiste per impedire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogHashIgnoresUnitIdAndTurnTest,
	"RefactorTactics.TurnLog.HashIgnoresUnitIdAndTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogHashIgnoresUnitIdAndTurnTest::RunTest(const FString&)
{
	const TArray<FRTTurnLogEntry> Plain = SampleLog();

	TArray<FRTTurnLogEntry> Attributed = Plain;
	for (int32 i = 0; i < Attributed.Num(); ++i)
	{
		Attributed[i].UnitId = i + 1;
		Attributed[i].TurnNumber = 4;
	}

	TestEqual(TEXT("attribuire le voci non cambia l'hash canonico"),
		URTTurnLogLibrary::HashTurnLog(Attributed), URTTurnLogLibrary::HashTurnLog(Plain));
	TestEqual(TEXT("ne' quello ordinato"),
		URTTurnLogLibrary::HashTurnLogOrdered(Attributed), URTTurnLogLibrary::HashTurnLogOrdered(Plain));
	return true;
}

/**
 * Retrocompatibilita' del formato 5: una traccia scritta prima che esistessero `UnitId` e `TurnNumber` resta
 * leggibile, e i due campi restano neutri. Non si inventa cio' che quei byte non dicevano — in particolare
 * NON si deduce l'unita' dalla cella, che e' esattamente l'inferenza che D-063 ha dichiarato non valida.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogLegacyWithoutUnitIdTest,
	"RefactorTactics.TurnLog.LegacyVersionWithoutUnitIdIsReadable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogLegacyWithoutUnitIdTest::RunTest(const FString&)
{
	TArray<uint8> Bytes;
	auto U16 = [&Bytes](uint16 V) { Bytes.Add(V & 0xFF); Bytes.Add((V >> 8) & 0xFF); };
	auto U32 = [&Bytes](uint32 V)
	{
		Bytes.Add(V & 0xFF); Bytes.Add((V >> 8) & 0xFF); Bytes.Add((V >> 16) & 0xFF); Bytes.Add((V >> 24) & 0xFF);
	};
	auto Str = [&Bytes, &U16](const char* S)
	{
		const int32 Len = FCStringAnsi::Strlen(S);
		U16(static_cast<uint16>(Len));
		for (int32 i = 0; i < Len; ++i) { Bytes.Add(static_cast<uint8>(S[i])); }
	};

	U32(0x4C545452u); // 'RTTL'
	U16(static_cast<uint16>(ERTTurnLogFormatVersion::WithBaseActionId)); // v5: UnitId/TurnNumber non esistono
	U16(static_cast<uint16>(ERTLogTopology::Hex));
	Str("Format.Skirmish2v2");
	U32(1);
	Bytes.Add(static_cast<uint8>(ERTMatchPhase::Blast));
	Bytes.Add(static_cast<uint8>(ERTLogCategory::Combat));
	Bytes.Add(static_cast<uint8>(ERTCombatOutcome::Hit));
	U32(2); U32(2); U32(0);  // SrcCell: una cella c'e', ma non dice CHI
	U32(3); U32(3); U32(0);
	U32(12);
	Str("Bastion.ImpactShot");
	Str("Action.BasicAttack");

	uint32 H = 2166136261u;
	for (const uint8 B : Bytes) { H ^= B; H *= 16777619u; }
	U32(H);

	TArray<FRTTurnLogEntry> Out;
	TestTrue(TEXT("una traccia in versione 5 resta leggibile"),
		URTTurnLogLibrary::DeserializeTurnLog(Bytes, Out, nullptr, nullptr));
	if (!TestEqual(TEXT("una voce letta"), Out.Num(), 1)) { return false; }
	TestEqual(TEXT("l'ActionId e' preservato"), Out[0].ActionId, FName(TEXT("Bastion.ImpactShot")));
	TestEqual(TEXT("nessuna unita' dedotta dalla cella"), Out[0].UnitId, 0);
	TestEqual(TEXT("nessun turno inventato"), Out[0].TurnNumber, 0);
	return true;
}

/**
 * La forma canonica deve essere definita su OGNI campo che il serializzatore scrive.
 *
 * `D-SR-1` promette byte permutazione-invarianti, e la promessa regge solo se `EntryLess` e' un ordine
 * TOTALE sui campi serializzati. Se ne resta fuori uno, due voci che pareggiano su tutto il resto restano
 * a pari merito e il loro ordine lo decide `TArray::Sort`, che non e' stabile: due inserimenti diversi
 * producono due file diversi con lo stesso contenuto. E' il difetto che `UnitId` ha introdotto arrivando
 * nella v6 senza entrare nel confronto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogCanonicalOrderCoversSerializedFieldsTest,
	"RefactorTactics.TurnLog.CanonicalOrderCoversSerializedFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogCanonicalOrderCoversSerializedFieldsTest::RunTest(const FString&)
{
	// Due voci identiche in TUTTO tranne l'unita' che le ha prodotte: e' il pareggio che scopre il difetto.
	FRTTurnLogEntry First = MakeSerEntry(ERTMatchPhase::Blast, ERTLogCategory::Combat,
		static_cast<uint8>(ERTCombatOutcome::Hit), FRTCellId(1, 1), FRTCellId(2, 2), 10);
	First.UnitId = 1;
	FRTTurnLogEntry Second = First;
	Second.UnitId = 2;

	TArray<FRTTurnLogEntry> Ascending;
	Ascending.Add(First);
	Ascending.Add(Second);
	TArray<FRTTurnLogEntry> Descending;
	Descending.Add(Second);
	Descending.Add(First);

	TestTrue(TEXT("stessi byte a prescindere dall'ordine d'inserimento, anche a pari merito"),
		URTTurnLogLibrary::SerializeTurnLog(Ascending) == URTTurnLogLibrary::SerializeTurnLog(Descending));

	// E l'ordine deve essere ANTISIMMETRICO, non solo deterministico: due voci diverse non sono mai «uguali».
	TestTrue(TEXT("A < B sull'unita'"), URTTurnLogLibrary::EntryLess(First, Second));
	TestFalse(TEXT("e non B < A"), URTTurnLogLibrary::EntryLess(Second, First));
	return true;
}

/**
 * `GraphRevision` ENTRA nell'hash, al contrario di `UnitId` e `TurnNumber`.
 *
 * Il criterio e' quello di sempre: due tracce possono differire SOLO per questo campo — stessi eventi, ma
 * grafo modificato in un turno precedente — e sono due partite diverse. Un movimento validato su un grafo
 * e uno validato su un altro non sono lo stesso evento, anche quando le celle coincidono.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogGraphRevisionEntersHashTest,
	"RefactorTactics.TurnLog.GraphRevisionEntersTheHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogGraphRevisionEntersHashTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Before = SampleLog();
	TArray<FRTTurnLogEntry> After = Before;
	for (FRTTurnLogEntry& E : Before) { E.GraphRevision = 4; }
	for (FRTTurnLogEntry& E : After)  { E.GraphRevision = 5; }

	TestNotEqual(TEXT("grafo diverso -> hash diverso"),
		URTTurnLogLibrary::HashTurnLog(After), URTTurnLogLibrary::HashTurnLog(Before));

	// Controprova: a parita' di revisione l'hash non si muove, cioe' il campo non e' rumore.
	TArray<FRTTurnLogEntry> SameAsBefore = SampleLog();
	for (FRTTurnLogEntry& E : SameAsBefore) { E.GraphRevision = 4; }
	TestEqual(TEXT("stessa revisione -> stesso hash"),
		URTTurnLogLibrary::HashTurnLog(SameAsBefore), URTTurnLogLibrary::HashTurnLog(Before));
	return true;
}

/** `GraphRevision` sopravvive al round-trip, come gli altri campi della v6. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogGraphRevisionRoundTripTest,
	"RefactorTactics.TurnLog.GraphRevisionRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogGraphRevisionRoundTripTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Log;
	FRTTurnLogEntry Moved = MakeSerEntry(ERTMatchPhase::Move, ERTLogCategory::Move,
		static_cast<uint8>(ERTMoveOutcome::Moved), FRTCellId(0, 0), FRTCellId(1, 0), 1);
	Moved.UnitId = 3;
	Moved.TurnNumber = 9;
	Moved.GraphRevision = 12; // il ponte e' crollato due eventi fa: il movimento e' stato validato su QUESTO grafo
	Log.Add(Moved);

	TArray<FRTTurnLogEntry> Restored;
	if (!TestTrue(TEXT("round-trip riuscito"), URTTurnLogLibrary::DeserializeTurnLog(
		URTTurnLogLibrary::SerializeTurnLog(Log), Restored)))
	{
		return false;
	}
	if (!TestEqual(TEXT("una voce letta"), Restored.Num(), 1)) { return false; }
	TestEqual(TEXT("GraphRevision preservata"), Restored[0].GraphRevision, 12);
	TestEqual(TEXT("e gli altri campi della v6 con lei"), Restored[0].UnitId, 3);
	return true;
}

/**
 * La diagnosi della prima divergenza deve considerare divergenza **esattamente cio' che l'hash considera**.
 *
 * `GoldenEntriesMatch` confrontava le voci con `EntryLess`, e finche' i campi di `EntryLess` coincidevano con
 * quelli dell'hash la promessa reggeva. Da quando `UnitId` e `TurnNumber` sono entrati nell'ordinamento — e
 * NON nell'hash — `EntryLess` discrimina di piu': una voce che differisce solo per l'unita' verrebbe indicata
 * come «prima divergenza» pur essendo identica per l'hash, e siccome ne' `DescribeEntry` ne' il fallback dei
 * campi grezzi stampano `UnitId`, la diagnosi mostrerebbe due stringhe uguali — la stessa illusione di
 * «confronto rotto» gia' corretta una volta per l'`ActionId`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogDivergenceIgnoresNonHashedFieldsTest,
	"RefactorTactics.TurnLog.FirstDivergenceIgnoresFieldsOutsideTheHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogDivergenceIgnoresNonHashedFieldsTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Golden = SampleLog();
	TArray<FRTTurnLogEntry> Actual = Golden;
	// Stessa identica traccia per l'hash: cambia solo CHI ha agito e in quale turno, che l'hash non guarda.
	for (int32 i = 0; i < Actual.Num(); ++i)
	{
		Actual[i].UnitId = i + 1;
		Actual[i].TurnNumber = 7;
	}

	if (!TestEqual(TEXT("premessa: per l'hash le due tracce sono identiche"),
		URTTurnLogLibrary::HashTurnLog(Actual), URTTurnLogLibrary::HashTurnLog(Golden)))
	{
		return false;
	}

	const FString Diagnosis = URTTurnLogLibrary::DescribeFirstDivergence(1, Golden, Actual);
	TestEqual(TEXT("nessuna divergenza da riportare: l'hash non vede quei campi"), Diagnosis, FString());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
