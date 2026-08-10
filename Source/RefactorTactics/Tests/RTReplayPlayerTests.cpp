#include "Misc/AutomationTest.h"
#include "Replay/RTReplayPlayerLibrary.h"
#include "Replay/RTReplayRecorderLibrary.h"
#include "Replay/RTReplayManifest.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il Player (`#470`) e i tre test che [ADR-0009](../../../docs/decisions/adr-0009-replay-logico-canonico.md)
 * richiede e dichiarava inesistenti. Finche' mancavano, il rischio `REPLAY-04` restava aperto.
 *
 * ⚠️ **Guarda gli `#include` di questo file**: il Player, il recorder, il TurnLog. Nessun `ARTTurnManager`,
 * nessun `URTHexSimLibrary`, nessun resolver, nessun `UWorld`. Non e' economia di scrittura: e' la forma
 * NEGATIVA che l'ADR §3 chiede — se il Player avesse bisogno del resolver per riprodurre, questi test non
 * potrebbero esistere in questa forma.
 */
namespace
{
	FString PlayerRoot(const TCHAR* Nome)
	{
		return FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ReplayPlayer"), Nome);
	}

	void PuliscIPlayer(const FString& Root)
	{
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }
	}

	FRTTurnLogEntry VocePlayer(int32 Turno, ERTMatchPhase Fase, const TCHAR* Azione, int32 Amount)
	{
		FRTTurnLogEntry E;
		E.TurnNumber = Turno;
		E.Phase = Fase;
		E.Category = ERTLogCategory::Move;
		E.ActionId = FName(Azione);
		E.Amount = Amount;
		E.SrcCell = FRTCellId(Amount, Turno);
		E.TgtCell = FRTCellId(Amount + 1, Turno);
		return E;
	}

	/** Un turno con voci in DUE fasi: e' cio' che rende osservabile il raggruppamento per fase. */
	TArray<FRTTurnLogEntry> TracciaDiDueFasi(int32 Turno)
	{
		TArray<FRTTurnLogEntry> Voci;
		Voci.Add(VocePlayer(Turno, ERTMatchPhase::Move,  TEXT("Action.Move"),   Turno * 10));
		Voci.Add(VocePlayer(Turno, ERTMatchPhase::Blast, TEXT("Action.Attack"), Turno * 10 + 1));
		Voci.Add(VocePlayer(Turno, ERTMatchPhase::Blast, TEXT("Action.Attack"), Turno * 10 + 2));
		URTTurnLogLibrary::SortTurnLog(Voci); // forma canonica: e' la precondizione del Player
		return Voci;
	}

	/** Scrive un archivio di `Turni` turni. `bChiudi = false` lascia l'archivio parziale. */
	FGuid ScriviArchivioPlayer(const FString& Root, int32 Turni, bool bChiudi, const FGuid& Id)
	{
		FRTReplayManifest M;
		M.MatchId = Id;
		M.FormatId = FName(TEXT("Format.Skirmish2v2"));
		M.bHexTopology = true;

		for (int32 T = 1; T <= Turni; ++T)
		{
			URTReplayRecorderLibrary::RecordTurn(Root, M, T, TracciaDiDueFasi(T));
		}
		if (bChiudi)
		{
			URTReplayRecorderLibrary::CloseMatch(Root, M, ERTMatchOutcome::Team0Wins, 999, 12.5f);
		}
		return M.MatchId;
	}
}

/**
 * `Replay.Player.RunsWithoutResolver` — la forma NEGATIVA del confine.
 *
 * Riproduce una partita intera in un contesto dove il resolver non esiste: nessun mondo, nessuna unita',
 * nessun `TurnManager`, nessun catalogo. Se il Player calcolasse anche una sola cosa — una collisione, un
 * danno, la legalita' di un percorso — qui non avrebbe niente su cui calcolarla.
 *
 * Verifica anche cosa il Player puo' promettere: **lo stesso insieme di voci**, raggruppate per fase e in
 * ordine canonico dentro la fase, con le fasi nell'ordine in cui sono state giocate. Non l'ordine di
 * emissione, che `SortTurnLog` ha perso in memoria durante la risoluzione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayPlayerNoResolverTest,
	"RefactorTactics.Replay.Player.RunsWithoutResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayPlayerNoResolverTest::RunTest(const FString&)
{
	const FString Root = PlayerRoot(TEXT("NoResolver"));
	PuliscIPlayer(Root);

	const FGuid Id = ScriviArchivioPlayer(Root, /*Turni=*/ 3, /*bChiudi=*/ true, FGuid(1, 2, 3, 4));

	FRTReplaySession Session;
	if (!TestTrue(TEXT("l'archivio si apre"),
			URTReplayPlayerLibrary::OpenArchive(Root, Id, Session) == ERTReplayOpenResult::Opened))
	{
		PuliscIPlayer(Root);
		return false;
	}

	TestTrue(TEXT("l'archivio e' completo"), Session.bComplete);
	TestEqual(TEXT("tre tracce caricate"), Session.Traces.Num(), 3);

	// Riproduzione integrale, fase per fase.
	TArray<ERTMatchPhase> FasiViste;
	TArray<FRTTurnLogEntry> Riprodotte;
	ERTMatchPhase Fase = ERTMatchPhase::Planning;
	TArray<FRTTurnLogEntry> DellaFase;
	int32 Guardia = 0;
	while (URTReplayPlayerLibrary::AdvancePhase(Session, Fase, DellaFase) && Guardia++ < 100)
	{
		TestTrue(TEXT("una fase emessa non e' mai vuota"), DellaFase.Num() > 0);
		FasiViste.Add(Fase);
		Riprodotte.Append(DellaFase);
	}

	// 1. STESSO INSIEME di voci: 3 turni x 3 voci.
	TestEqual(TEXT("tutte le voci sono state emesse"), Riprodotte.Num(), 9);

	// 2. RAGGRUPPATE PER FASE: due fasi per turno, sei gruppi in tutto, e dentro un gruppo la fase e' una sola.
	TestEqual(TEXT("sei gruppi di fase"), FasiViste.Num(), 6);

	// 3. LE FASI SI SUSSEGUONO nell'ordine in cui sono state giocate: `Blast` precede `Move` dentro il turno
	//    (`ERTMatchPhase` e' cronologico e `Phase` e' la prima chiave di `EntryLess`).
	for (int32 T = 0; T < 3; ++T)
	{
		TestTrue(FString::Printf(TEXT("turno %d: prima il Blast"), T + 1),
			FasiViste[T * 2] == ERTMatchPhase::Blast);
		TestTrue(FString::Printf(TEXT("turno %d: poi il Move"), T + 1),
			FasiViste[T * 2 + 1] == ERTMatchPhase::Move);
	}

	// 4. ORDINE CANONICO dentro la fase: le voci riprodotte sono esattamente quelle della traccia, nello
	//    stesso ordine — il Player non riordina, e non e' un dettaglio: riordinare sarebbe una decisione.
	{
		TArray<FRTTurnLogEntry> Attese;
		for (int32 T = 1; T <= 3; ++T) { Attese.Append(TracciaDiDueFasi(T)); }
		TestEqual(TEXT("stesso numero di voci attese"), Riprodotte.Num(), Attese.Num());
		bool bIdentiche = Riprodotte.Num() == Attese.Num();
		for (int32 i = 0; bIdentiche && i < Attese.Num(); ++i)
		{
			bIdentiche = Riprodotte[i].Phase == Attese[i].Phase
				&& Riprodotte[i].ActionId == Attese[i].ActionId
				&& Riprodotte[i].Amount == Attese[i].Amount;
		}
		TestTrue(TEXT("le voci riprodotte sono quelle della traccia, in ordine canonico"), bIdentiche);
	}

	// 5. IL SEEK E' QUELLO DI #415: non se ne scrive un secondo. Dopo il salto, la prima fase emessa e' quella
	//    del turno richiesto.
	URTReplayPlayerLibrary::Rewind(Session);
	TestTrue(TEXT("il seek al turno 3 trova il turno"),
		URTReplayPlayerLibrary::SeekToTurn(Session, 3) == ERTReplaySeekResult::Found);
	if (TestTrue(TEXT("dopo il seek c'e' ancora qualcosa da riprodurre"),
			URTReplayPlayerLibrary::AdvancePhase(Session, Fase, DellaFase)))
	{
		TestEqual(TEXT("ed e' il turno 3"), DellaFase[0].TurnNumber, 3);
	}

	PuliscIPlayer(Root);
	return true;
}

/**
 * `Replay.Player.RejectsIncompatibleArchive` — fail-closed **in apertura**.
 *
 * Il Player rifiuta cio' che non sa leggere quando apre, e da li' in poi non controlla piu' niente
 * (ADR-0009 §4). Ogni esito ha il proprio motivo: chi diagnostica deve sapere se manca il file, se e' di
 * un'altra versione o se e' di un'altra topologia — sono tre problemi con tre rimedi diversi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayPlayerRejectsTest,
	"RefactorTactics.Replay.Player.RejectsIncompatibleArchive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayPlayerRejectsTest::RunTest(const FString&)
{
	const FString Root = PlayerRoot(TEXT("Rejects"));
	PuliscIPlayer(Root);

	// 1. ARCHIVIO ASSENTE: non c'e' nessun manifest sotto quel MatchId.
	{
		FRTReplaySession S;
		TestTrue(TEXT("un archivio inesistente non si apre"),
			URTReplayPlayerLibrary::OpenArchive(Root, FGuid(9, 9, 9, 9), S)
				== ERTReplayOpenResult::ManifestUnreadable);
	}

	// 2. VERSIONE SCONOSCIUTA: il manifest c'e' ma viene da un formato futuro.
	{
		const FGuid Id(0xFEEDFACE, 1, 1, 1);
		const FString Dir = URTReplayRecorderLibrary::MatchDirectory(Root, Id);
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		PF.CreateDirectoryTree(*Dir);
		FFileHelper::SaveStringToFile(
			FString::Printf(TEXT("{\"Version\":9999,\"MatchId\":\"%s\"}"), *Id.ToString(EGuidFormats::Digits)),
			*FPaths::Combine(Dir, TEXT("match.rtmanifest")));

		FRTReplaySession S;
		TestTrue(TEXT("una versione sconosciuta non si apre"),
			URTReplayPlayerLibrary::OpenArchive(Root, Id, S) == ERTReplayOpenResult::ManifestUnreadable);
	}

	// 3. TRACCIA CORROTTA: il manifest dichiara tre turni, ma uno dei file e' spazzatura. Il rifiuto arriva
	//    in APERTURA — non a meta' riproduzione, davanti a chi sta guardando.
	{
		const FGuid Id = ScriviArchivioPlayer(Root, /*Turni=*/ 3, /*bChiudi=*/ true, FGuid(2, 2, 2, 2));
		const FString Traccia = FPaths::Combine(
			URTReplayRecorderLibrary::MatchDirectory(Root, Id),
			URTReplayRecorderLibrary::TurnFileName(2));
		FFileHelper::SaveStringToFile(TEXT("questi non sono i byte che cerchi"), *Traccia);

		FRTReplaySession S;
		S.Traces.SetNum(7); // sentinella: su un rifiuto l'uscita non va toccata
		TestTrue(TEXT("una traccia corrotta non si apre"),
			URTReplayPlayerLibrary::OpenArchive(Root, Id, S) == ERTReplayOpenResult::TraceUnreadable);
		TestEqual(TEXT("e la sessione del chiamante e' intatta"), S.Traces.Num(), 7);
	}

	// 4. TRACCIA MANCANTE: il manifest ne dichiara tre e sul disco ce ne sono due. Un archivio che promette
	//    piu' di quello che ha non si apre a meta': lo dice.
	{
		const FGuid Id = ScriviArchivioPlayer(Root, /*Turni=*/ 3, /*bChiudi=*/ true, FGuid(3, 3, 3, 3));
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		PF.DeleteFile(*FPaths::Combine(
			URTReplayRecorderLibrary::MatchDirectory(Root, Id),
			URTReplayRecorderLibrary::TurnFileName(3)));

		FRTReplaySession S;
		TestTrue(TEXT("una traccia mancante non si apre"),
			URTReplayPlayerLibrary::OpenArchive(Root, Id, S) == ERTReplayOpenResult::TraceUnreadable);
	}

	PuliscIPlayer(Root);
	return true;
}

/**
 * Un archivio PARZIALE si riproduce fino a dove arriva, dichiarando che finisce li'.
 *
 * E' il caso per cui il recorder scrive durante la partita invece che alla fine: una partita che va in crash
 * deve lasciare qualcosa di apribile. Un archivio parziale non e' un archivio rotto, e i due esiti non vanno
 * confusi — `bComplete = false` lo si sa PRIMA di cominciare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayPlayerPartialTest,
	"RefactorTactics.Replay.Player.PartialArchivePlaysAsFarAsItGoes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayPlayerPartialTest::RunTest(const FString&)
{
	const FString Root = PlayerRoot(TEXT("Partial"));
	PuliscIPlayer(Root);

	const FGuid Id = ScriviArchivioPlayer(Root, /*Turni=*/ 2, /*bChiudi=*/ false, FGuid(4, 4, 4, 4));

	FRTReplaySession S;
	if (!TestTrue(TEXT("un archivio parziale si apre"),
			URTReplayPlayerLibrary::OpenArchive(Root, Id, S) == ERTReplayOpenResult::Opened))
	{
		PuliscIPlayer(Root);
		return false;
	}

	TestFalse(TEXT("e dichiara di essere parziale"), S.bComplete);
	TestEqual(TEXT("due turni riproducibili"), S.Traces.Num(), 2);

	int32 Voci = 0;
	ERTMatchPhase Fase = ERTMatchPhase::Planning;
	TArray<FRTTurnLogEntry> DellaFase;
	int32 Guardia = 0;
	while (URTReplayPlayerLibrary::AdvancePhase(S, Fase, DellaFase) && Guardia++ < 100)
	{
		Voci += DellaFase.Num();
	}
	TestEqual(TEXT("si riproduce fino a dove arriva"), Voci, 6);

	PuliscIPlayer(Root);
	return true;
}

/**
 * `Replay.Verifier.ReportsFirstDivergence` — il verdetto nomina turno, fase e `ActionId`.
 *
 * ⚠️ **E' un test del VERIFIER, non del Player**, ed e' qui perche' l'ADR li chiede insieme: il Verifier
 * confronta, il Player no. `DescribeFirstDivergence` esisteva gia' (CP 12.6) — questo test pinna che il suo
 * verdetto sia utilizzabile come verdetto di replay, cioe' che dica DOVE e non solo CHE.
 *
 * Il confronto e' fra due tracce. Chi produce la seconda ri-simulando e' il chiamante: e' li' che il resolver
 * entra, e non in questa funzione — che infatti resta pura.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayVerifierDivergenceTest,
	"RefactorTactics.Replay.Verifier.ReportsFirstDivergence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayVerifierDivergenceTest::RunTest(const FString&)
{
	const TArray<FRTTurnLogEntry> Golden = TracciaDiDueFasi(7);

	// Nessuna divergenza: due tracce identiche non producono un verdetto.
	TestEqual(TEXT("due tracce identiche non divergono"),
		URTTurnLogLibrary::DescribeFirstDivergence(7, Golden, Golden), FString());

	// Una sola voce cambia, e solo nell'azione: e' il caso che un confronto sciatto lascerebbe passare o
	// descriverebbe come «atteso [X], trovato [X]».
	TArray<FRTTurnLogEntry> Divergente = Golden;
	const int32 Ultima = Divergente.Num() - 1;
	Divergente[Ultima].ActionId = FName(TEXT("Action.SomethingElse"));

	const FString Verdetto = URTTurnLogLibrary::DescribeFirstDivergence(7, Golden, Divergente);
	if (!TestFalse(TEXT("una divergenza produce un verdetto"), Verdetto.IsEmpty()))
	{
		return false;
	}

	// Il verdetto NOMINA: turno, fase e azione. Sono i tre che il DoD chiede, e senza i quali chi legge deve
	// ricostruire da capo dove guardare.
	TestTrue(TEXT("il verdetto nomina il turno"), Verdetto.Contains(TEXT("7")));
	TestTrue(TEXT("il verdetto nomina la fase"), Verdetto.Contains(TEXT("fase")));
	TestTrue(TEXT("il verdetto nomina l'azione attesa"), Verdetto.Contains(TEXT("Action.Move"))
		|| Verdetto.Contains(TEXT("Action.Attack")));
	TestTrue(TEXT("il verdetto nomina l'azione trovata"), Verdetto.Contains(TEXT("Action.SomethingElse")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
