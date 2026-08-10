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
