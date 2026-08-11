#include "Misc/AutomationTest.h"
#include "Replay/RTReplayRecorderLibrary.h"
#include "Replay/RTReplayManifest.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Core/RTTypes.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Radice temporanea, una per test: due test che condividono una cartella si mascherano i difetti. */
	FString TempRoot(const TCHAR* Nome)
	{
		return FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ReplayRec"), Nome);
	}

	void PuliscI(const FString& Root)
	{
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }
	}

	FRTTurnLogEntry Voce(int32 Turno, ERTMatchPhase Fase, int32 Amount)
	{
		FRTTurnLogEntry E;
		E.TurnNumber = Turno;
		E.Phase = Fase;
		E.Category = ERTLogCategory::Move;
		E.Amount = Amount;
		E.SrcCell = FRTCellId(Amount, Turno);
		E.TgtCell = FRTCellId(Amount + 1, Turno);
		return E;
	}

	FRTReplayManifest ManifestDiProva()
	{
		FRTReplayManifest M;
		M.MatchId = FGuid(0x11111111, 0x22222222, 0x33333333, 0x44444444);
		M.FormatId = FName(TEXT("Format.Skirmish2v2"));
		M.bHexTopology = true;
		return M;
	}
}

// Round-trip del manifest: cio' che si scrive si rilegge uguale. E' il minimo che un formato deve dare.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayManifestRoundTripTest,
	"RefactorTactics.Replay.Manifest.RoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayManifestRoundTripTest::RunTest(const FString&)
{
	FRTReplayManifest M = ManifestDiProva();
	// Il terzo e' `0xFFFFFFFF`, e non e' pedanteria: `HashTurnLogOrdered` e' FNV a 32 bit, quindi in
	// partita **meta'** degli hash ha il bit 31 acceso. I valori piccoli non attraversano il confine che
	// conta — il passaggio per il TESTO JSON, che e' una precisione diversa da quella del double.
	M.OrderedHashPerTurn = { 111, 222, 0xFFFFFFFF };
	M.TurnCount = 3;
	M.FinalStateHash = 4242;
	M.Outcome = ERTMatchOutcome::Team0Wins;
	M.WallClockSeconds = 91.5f;
	M.bClosed = true;

	FRTReplayManifest Riletto;
	TestTrue(TEXT("il manifest si rilegge"),
		URTReplayRecorderLibrary::ManifestFromJson(URTReplayRecorderLibrary::ManifestToJson(M), Riletto));

	TestEqual(TEXT("MatchId"), Riletto.MatchId, M.MatchId);
	TestEqual(TEXT("FormatId"), Riletto.FormatId, M.FormatId);
	// `TestEqual` NON interrompe: senza questo `return` un array corto farebbe crashare la run intera
	// all'indicizzazione qui sotto, e un test che crasha non dice cos'e' andato storto — porta giu' tutti.
	if (!TestEqual(TEXT("numero di hash per turno"), Riletto.OrderedHashPerTurn.Num(), 3)) { return false; }
	TestEqual(TEXT("il secondo hash"), Riletto.OrderedHashPerTurn[1], (int64)222);
	TestEqual(TEXT("e un hash col bit 31 acceso sopravvive al testo JSON"),
		Riletto.OrderedHashPerTurn[2], (int64)0xFFFFFFFF);
	TestEqual(TEXT("checksum finale"), Riletto.FinalStateHash, (int64)4242);
	TestEqual(TEXT("esito"), Riletto.Outcome, ERTMatchOutcome::Team0Wins);
	TestTrue(TEXT("chiuso"), Riletto.bClosed);
	return true;
}

// Fail-closed: una versione che non conosciamo si rifiuta, non si interpreta. E' la convenzione che
// `DeserializeTurnLog` applica gia' al formato binario, e che ADR-0009 §4 chiede al Player in apertura.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayManifestUnknownVersionTest,
	"RefactorTactics.Replay.Manifest.UnknownVersionIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayManifestUnknownVersionTest::RunTest(const FString&)
{
	const FString Futuro = TEXT("{\"Version\":9999,\"MatchId\":\"00000000000000000000000000000001\"}");

	FRTReplayManifest Out;
	Out.TurnCount = 7; // sentinella: se il loader tocca l'uscita su un rifiuto, si vede

	TestFalse(TEXT("una versione sconosciuta non si legge"),
		URTReplayRecorderLibrary::ManifestFromJson(Futuro, Out));
	TestEqual(TEXT("e l'uscita non e' stata toccata"), Out.TurnCount, 7);

	TestFalse(TEXT("nemmeno JSON malformato"),
		URTReplayRecorderLibrary::ManifestFromJson(TEXT("non sono json"), Out));

	// Sotto `Initial` non c'e' un formato piu' vecchio da interpretare: c'e' un file che non e' un manifest.
	const FString Zero = TEXT("{\"Version\":0,\"MatchId\":\"00000000000000000000000000000001\"}");
	TestFalse(TEXT("la versione 0 non si legge"), URTReplayRecorderLibrary::ManifestFromJson(Zero, Out));
	TestEqual(TEXT("e nemmeno lei tocca l'uscita"), Out.TurnCount, 7);
	return true;
}

/**
 * Un manifest scritto da un binario PRECEDENTE resta leggibile — la verifica a due binari, resa possibile
 * su un formato testuale congelandone un payload (`#471`).
 *
 * Questo JSON e' cio' che il binario del 2026-08-10 scrive: e' il «binario vecchio» della verifica, e vive
 * qui invece che su disco perche' un golden in un test si legge in una code review. Il giorno in cui il
 * manifest crescera' di un campo, questo test dira' se le versioni precedenti sono ancora leggibili o se
 * l'estensione ha inserito qualcosa **in mezzo** invece che in coda.
 *
 * ⚠️ Non e' un test di round-trip: quello prova che scrittore e lettore si capiscano fra loro, e resterebbe
 * verde anche se **entrambi** cambiassero insieme rendendo illeggibile tutto cio' che e' gia' su disco.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayManifestGoldenV1Test,
	"RefactorTactics.Replay.Manifest.GoldenV1StaysReadable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayManifestGoldenV1Test::RunTest(const FString&)
{
	const FString GoldenV1 =
		TEXT("{\"Version\":1,\"MatchId\":\"11111111222222223333333344444444\",")
		TEXT("\"FormatId\":\"Format.Skirmish2v2\",\"HexTopology\":true,")
		TEXT("\"OrderedHashPerTurn\":[4294967295,7,42],")
		TEXT("\"FinalStateHash\":123456789,\"Outcome\":1,\"WallClockSeconds\":91.5,")
		TEXT("\"Closed\":true,\"TurnCount\":3}");

	FRTReplayManifest M;
	if (!TestTrue(TEXT("il manifest v1 si rilegge"), URTReplayRecorderLibrary::ManifestFromJson(GoldenV1, M)))
	{
		return false;
	}

	TestEqual(TEXT("MatchId"), M.MatchId, FGuid(0x11111111, 0x22222222, 0x33333333, 0x44444444));
	TestEqual(TEXT("FormatId"), M.FormatId, FName(TEXT("Format.Skirmish2v2")));
	TestTrue(TEXT("topologia hex"), M.bHexTopology);
	TestEqual(TEXT("tre hash ordinati"), M.OrderedHashPerTurn.Num(), 3);

	// `0xFFFFFFFF` e' il caso che una svista di tipo romperebbe per primo: un `uint32` allargato a `int64` e'
	// una zero-extension, e un `int32` per errore lo leggerebbe come `-1`.
	TestEqual(TEXT("l'hash a 32 bit pieni non diventa negativo"),
		M.OrderedHashPerTurn[0], static_cast<int64>(4294967295));
	TestEqual(TEXT("FinalStateHash"), M.FinalStateHash, static_cast<int64>(123456789));
	TestTrue(TEXT("Outcome"), M.Outcome == static_cast<ERTMatchOutcome>(1));
	TestEqual(TEXT("WallClockSeconds"), M.WallClockSeconds, 91.5f);
	TestTrue(TEXT("chiuso"), M.bClosed);
	TestEqual(TEXT("TurnCount"), M.TurnCount, 3);

	return true;
}

/**
 * I campi che un binario precedente NON scriveva si leggono a valore neutro, senza rifiutare il manifest.
 *
 * E' l'altra meta' della retrocompatibilita': accettare la versione non basta se poi manca un campo e la
 * lettura fallisce. Il minimo indispensabile e' versione + identita' — tutto il resto ha un default che
 * significa qualcosa: `Closed = false` dice «archivio parziale», `FinalStateHash = 0` dice «non calcolato».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayManifestNeutralFieldsTest,
	"RefactorTactics.Replay.Manifest.MissingFieldsStayNeutral",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayManifestNeutralFieldsTest::RunTest(const FString&)
{
	const FString Minimo = TEXT("{\"Version\":1,\"MatchId\":\"11111111222222223333333344444444\"}");

	FRTReplayManifest M;
	M.TurnCount = 99; // sporca l'uscita: i default devono venire dal formato, non da cio' che c'era prima
	M.bClosed = true;

	if (!TestTrue(TEXT("un manifest senza campi opzionali si rilegge"),
			URTReplayRecorderLibrary::ManifestFromJson(Minimo, M)))
	{
		return false;
	}

	TestEqual(TEXT("MatchId letto"), M.MatchId, FGuid(0x11111111, 0x22222222, 0x33333333, 0x44444444));
	TestEqual(TEXT("nessun turno dichiarato"), M.TurnCount, 0);
	TestEqual(TEXT("nessun hash per turno"), M.OrderedHashPerTurn.Num(), 0);
	TestEqual(TEXT("checksum non calcolato"), M.FinalStateHash, static_cast<int64>(0));
	TestFalse(TEXT("non chiuso: un archivio senza chiusura e' parziale"), M.bClosed);
	TestTrue(TEXT("esito in corso"), M.Outcome == ERTMatchOutcome::InProgress);

	return true;
}

// La traccia scritta dal recorder e' BYTE-IDENTICA a quella di `SerializeTurnLog`: il recorder non e' un
// secondo serializzatore. E' il criterio che, se non verificato cosi', si soddisfa a parole.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayRecorderBytesTest,
	"RefactorTactics.Replay.Recorder.TurnBytesMatchSerializeTurnLog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayRecorderBytesTest::RunTest(const FString&)
{
	const FString Root = TempRoot(TEXT("Bytes"));
	PuliscI(Root);

	TArray<FRTTurnLogEntry> Voci;
	Voci.Add(Voce(1, ERTMatchPhase::Blast, 20));
	Voci.Add(Voce(1, ERTMatchPhase::Move, 30));
	URTTurnLogLibrary::SortTurnLog(Voci);

	FRTReplayManifest M = ManifestDiProva();
	TestTrue(TEXT("il turno si registra"), URTReplayRecorderLibrary::RecordTurn(Root, M, 1, Voci));

	const FString Percorso = FPaths::Combine(
		URTReplayRecorderLibrary::MatchDirectory(Root, M.MatchId),
		URTReplayRecorderLibrary::TurnFileName(1));

	TArray<uint8> DalDisco;
	TestTrue(TEXT("il file della traccia esiste"), FFileHelper::LoadFileToArray(DalDisco, *Percorso));

	const TArray<uint8> Atteso = URTTurnLogLibrary::SerializeTurnLog(Voci, ERTLogTopology::Hex,
		FName(TEXT("Format.Skirmish2v2")));
	TestEqual(TEXT("stessa lunghezza di SerializeTurnLog"), DalDisco.Num(), Atteso.Num());
	TestTrue(TEXT("stessi byte"), DalDisco == Atteso);

	PuliscI(Root);
	return true;
}

// Un archivio parziale si riconosce: il manifest non e' chiuso. Nessun secondo meccanismo, nessun campo
// «bParziale» da ricordarsi di scrivere — la parzialita' e' l'ASSENZA della chiusura.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayRecorderPartialTest,
	"RefactorTactics.Replay.Recorder.UnclosedArchiveIsRecognisable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayRecorderPartialTest::RunTest(const FString&)
{
	const FString Root = TempRoot(TEXT("Partial"));
	PuliscI(Root);

	TArray<FRTTurnLogEntry> Voci;
	Voci.Add(Voce(1, ERTMatchPhase::Move, 10));

	FRTReplayManifest M = ManifestDiProva();
	URTReplayRecorderLibrary::RecordTurn(Root, M, 1, Voci);
	URTReplayRecorderLibrary::RecordTurn(Root, M, 2, Voci);

	// La partita muore qui: nessuna CloseMatch.
	FRTReplayManifest Riletto;
	TestTrue(TEXT("il manifest parziale si rilegge lo stesso"),
		URTReplayRecorderLibrary::LoadManifest(Root, M.MatchId, Riletto));
	TestFalse(TEXT("e dichiara di NON essere chiuso"), Riletto.bClosed);
	TestEqual(TEXT("i due turni registrati ci sono"), Riletto.TurnCount, 2);
	TestEqual(TEXT("l'esito non e' stato inventato"), Riletto.Outcome, ERTMatchOutcome::InProgress);

	// E dopo la chiusura, lo stesso archivio si dichiara completo.
	TestTrue(TEXT("la partita si chiude"), URTReplayRecorderLibrary::CloseMatch(
		Root, M, ERTMatchOutcome::Team1Wins, 777, 12.5f));

	FRTReplayManifest Chiuso;
	URTReplayRecorderLibrary::LoadManifest(Root, M.MatchId, Chiuso);
	TestTrue(TEXT("ora e' chiuso"), Chiuso.bClosed);
	TestEqual(TEXT("con l'esito vero"), Chiuso.Outcome, ERTMatchOutcome::Team1Wins);
	TestEqual(TEXT("e il checksum finale"), Chiuso.FinalStateHash, (int64)777);

	PuliscI(Root);
	return true;
}

// `MatchId` sta FUORI da ogni hash (D-077): identifica la registrazione, non il contenuto. Due partite con
// id diverso e stessi eventi hanno gli stessi hash di traccia — se cosi' non fosse, l'id sarebbe entrato
// nel determinismo dalla porta di servizio.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayRecorderMatchIdOutOfHashTest,
	"RefactorTactics.Replay.Recorder.MatchIdStaysOutOfHashes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayRecorderMatchIdOutOfHashTest::RunTest(const FString&)
{
	const FString Root = TempRoot(TEXT("IdHash"));
	PuliscI(Root);

	TArray<FRTTurnLogEntry> Voci;
	Voci.Add(Voce(1, ERTMatchPhase::Move, 10));
	URTTurnLogLibrary::SortTurnLog(Voci);

	FRTReplayManifest A = ManifestDiProva();
	FRTReplayManifest B = ManifestDiProva();
	B.MatchId = FGuid(0x99999999, 0x88888888, 0x77777777, 0x66666666); // sola differenza

	URTReplayRecorderLibrary::RecordTurn(Root, A, 1, Voci);
	URTReplayRecorderLibrary::RecordTurn(Root, B, 1, Voci);

	if (!TestEqual(TEXT("un hash per turno in A"), A.OrderedHashPerTurn.Num(), 1)) { PuliscI(Root); return false; }
	if (!TestEqual(TEXT("un hash per turno in B"), B.OrderedHashPerTurn.Num(), 1)) { PuliscI(Root); return false; }
	TestEqual(TEXT("stesso hash ordinato: l'id non ci entra"),
		A.OrderedHashPerTurn[0], B.OrderedHashPerTurn[0]);

	PuliscI(Root);
	return true;
}

// Una scrittura fallita NON deve lasciare il manifest in memoria piu' avanti del disco. E' l'invariante
// su cui poggia tutto il resto — «un manifest non chiuso e' la dichiarazione di parzialita'» — e senza
// questo test la si puo' rompere proprio nel caso che il recorder esiste per sopravvivere.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayRecorderFailedWriteTest,
	"RefactorTactics.Replay.Recorder.FailedWriteLeavesManifestUntouched",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayRecorderFailedWriteTest::RunTest(const FString&)
{
	const FString Root = TempRoot(TEXT("FailWrite"));
	PuliscI(Root);

	FRTReplayManifest M = ManifestDiProva();

	// Un FILE dove dovrebbe nascere la CARTELLA della partita: la creazione dell'albero fallira'.
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Root);
	const FString Ostacolo = URTReplayRecorderLibrary::MatchDirectory(Root, M.MatchId);
	FFileHelper::SaveStringToFile(TEXT("non sono una cartella"), *Ostacolo);

	TArray<FRTTurnLogEntry> Voci;
	Voci.Add(Voce(1, ERTMatchPhase::Move, 10));

	TestFalse(TEXT("registrare un turno fallisce"), URTReplayRecorderLibrary::RecordTurn(Root, M, 1, Voci));
	TestEqual(TEXT("e il manifest NON ha contato il turno"), M.OrderedHashPerTurn.Num(), 0);
	TestEqual(TEXT("ne' il conteggio"), M.TurnCount, 0);

	TestFalse(TEXT("chiudere fallisce"),
		URTReplayRecorderLibrary::CloseMatch(Root, M, ERTMatchOutcome::Team0Wins, 999, 5.f));
	TestFalse(TEXT("e il manifest NON si dichiara chiuso"), M.bClosed);
	TestEqual(TEXT("ne' porta l'esito che non e' stato scritto"), M.Outcome, ERTMatchOutcome::InProgress);

	PuliscI(Root);
	return true;
}

// Un turno fuori sequenza si rifiuta: se il conteggio degli hash e quello dei file divergono, il manifest
// smette di descrivere l'archivio e nessuno se ne accorge. Fail-closed anche qui.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayRecorderSequenceTest,
	"RefactorTactics.Replay.Recorder.OutOfSequenceTurnIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayRecorderSequenceTest::RunTest(const FString&)
{
	const FString Root = TempRoot(TEXT("Sequence"));
	PuliscI(Root);

	TArray<FRTTurnLogEntry> Voci;
	Voci.Add(Voce(1, ERTMatchPhase::Move, 10));

	FRTReplayManifest M = ManifestDiProva();
	TestTrue(TEXT("il primo turno e' l'1"), URTReplayRecorderLibrary::RecordTurn(Root, M, 1, Voci));

	TestFalse(TEXT("registrare di nuovo l'1 si rifiuta"),
		URTReplayRecorderLibrary::RecordTurn(Root, M, 1, Voci));
	TestFalse(TEXT("e saltare al 5 pure"), URTReplayRecorderLibrary::RecordTurn(Root, M, 5, Voci));
	TestEqual(TEXT("il manifest ha ancora un solo turno"), M.OrderedHashPerTurn.Num(), 1);

	TestTrue(TEXT("il 2 invece si registra"), URTReplayRecorderLibrary::RecordTurn(Root, M, 2, Voci));

	PuliscI(Root);
	return true;
}

// Il wall-clock vive SOLO nel manifest: due partite identiche durate diversamente hanno gli stessi hash.
// E' l'AC 4 di #469, che senza un test resterebbe vero per costruzione e falso al primo refactor.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayRecorderWallClockTest,
	"RefactorTactics.Replay.Recorder.WallClockStaysOutOfHashes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayRecorderWallClockTest::RunTest(const FString&)
{
	const FString Root = TempRoot(TEXT("WallClock"));
	PuliscI(Root);

	TArray<FRTTurnLogEntry> Voci;
	Voci.Add(Voce(1, ERTMatchPhase::Move, 10));
	URTTurnLogLibrary::SortTurnLog(Voci);

	FRTReplayManifest Veloce = ManifestDiProva();
	FRTReplayManifest Lenta = ManifestDiProva();
	Lenta.MatchId = FGuid(1, 2, 3, 4);

	URTReplayRecorderLibrary::RecordTurn(Root, Veloce, 1, Voci);
	URTReplayRecorderLibrary::RecordTurn(Root, Lenta, 1, Voci);
	URTReplayRecorderLibrary::CloseMatch(Root, Veloce, ERTMatchOutcome::Team0Wins, 42, 3.f);
	URTReplayRecorderLibrary::CloseMatch(Root, Lenta, ERTMatchOutcome::Team0Wins, 42, 3000.f);

	if (!TestEqual(TEXT("un hash per parte"), Veloce.OrderedHashPerTurn.Num(), 1)) { PuliscI(Root); return false; }
	if (!TestEqual(TEXT("idem"), Lenta.OrderedHashPerTurn.Num(), 1)) { PuliscI(Root); return false; }
	TestEqual(TEXT("durata diversa, stesso hash di traccia"),
		Veloce.OrderedHashPerTurn[0], Lenta.OrderedHashPerTurn[0]);

	PuliscI(Root);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
