#include "Misc/AutomationTest.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Perception/RTKnowledgeView.h"
#include "Perception/RTTeamKnowledge.h"
#include "Replay/RTReplayAuditLibrary.h"
#include "Turn/RTTurnLog.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * L'artefatto d'audit di [D-313]: la conoscenza su cui si e' deciso, accanto alla traccia e non dentro.
 *
 * ⚠️ **Due dei test qui non guardano il formato, guardano un RICALCOLO.** E' la differenza fra un archivio
 * che *contiene* un dato e uno che *dimostra* qualcosa: il primo si verifica con un round-trip, il secondo
 * rifacendo il conto e confrontandolo con cio' che il gioco aveva scritto.
 */
namespace
{
	FString AuditRoot(const TCHAR* Name)
	{
		return FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ReplayAudit"), Name);
	}

	void Clean(const FString& Root)
	{
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }
	}

	/** Una conoscenza che vede una cella e ricorda un contatto: due stati diversi, come in partita. */
	FRTTeamKnowledge Knowledge(int32 TeamId, int32 TurnNumber)
	{
		FRTTeamKnowledge K;
		K.TeamId = TeamId;
		K.TurnNumber = TurnNumber;
		K.VisibleCells.Add(FRTCellId(1, 0, 0));
		K.VisibleCells.Add(FRTCellId(2, 0, 0));
		K.ExploredCells.Add(FRTCellId(0, 0, 0));

		FRTLastKnownContact Contact;
		Contact.StableUnitId = 7;
		Contact.Cell = FRTCellId(3, 0, 0);
		Contact.TurnNumber = TurnNumber;
		K.Contacts.Add(Contact);
		return K;
	}

	FRTTurnAudit SaturatedAudit(const FGuid& MatchId, int32 TurnNumber, int64 OrderedHash)
	{
		FRTTurnAudit A;
		A.MatchId = MatchId;
		A.TurnNumber = TurnNumber;
		A.OrderedHash = OrderedHash;
		A.PlanningKnowledge.Add(Knowledge(0, TurnNumber));
		A.PlanningKnowledge.Add(Knowledge(1, TurnNumber));
		A.BlastKnowledge.Add(Knowledge(0, TurnNumber));
		A.BlastKnowledge.Add(Knowledge(1, TurnNumber));

		FRTAuditVerdictRecord R0;
		R0.SubjectUnitId = 7;
		R0.SubjectTeamId = 1;
		R0.SubjectCell = FRTCellId(1, 0, 0);
		R0.Verdict = FRTKnowledgeVerdict::NoOne();
		R0.Verdict.AllowTeam(0);
		A.Verdicts.Add(R0);

		FRTAuditVerdictRecord R1;
		R1.SubjectUnitId = 9;
		R1.SubjectTeamId = 0;
		R1.SubjectCell = FRTCellId(2, 0, 0);
		R1.Verdict = FRTKnowledgeVerdict::Everyone();
		A.Verdicts.Add(R1);

		FRTAuditBotDecision D;
		D.UnitId = 3;
		D.TeamId = 0;
		D.TargetUnitId = 7;
		D.TargetTeamId = 1;
		D.TargetCell = FRTCellId(1, 0, 0);
		A.BotDecisions.Add(D);
		return A;
	}

}

/** Il formato conserva i tre record di [D-313]: due conoscenze per squadra piu' i verdetti. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayAuditRoundTripTest,
	"RefactorTactics.Replay.Audit.RoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayAuditRoundTripTest::RunTest(const FString&)
{
	const FGuid Id = FGuid(1, 2, 3, 4);
	const FRTTurnAudit Written = SaturatedAudit(Id, 3, 0x0BADC0DE);

	FRTTurnAudit Read;
	if (!TestTrue(TEXT("il JSON si rilegge"),
		URTReplayAuditLibrary::AuditFromJson(URTReplayAuditLibrary::AuditToJson(Written), Read)))
	{
		return false;
	}

	TestTrue(TEXT("MatchId conservato"), Read.MatchId == Written.MatchId);
	TestEqual(TEXT("TurnNumber conservato"), Read.TurnNumber, Written.TurnNumber);
	TestEqual(TEXT("l'ancora e' conservata"), Read.OrderedHash, Written.OrderedHash);

	if (TestEqual(TEXT("due conoscenze di Planning"), Read.PlanningKnowledge.Num(), 2))
	{
		TestEqual(TEXT("la squadra"), Read.PlanningKnowledge[1].TeamId, 1);
		TestEqual(TEXT("le celle viste"), Read.PlanningKnowledge[0].VisibleCells.Num(), 2);
		// ⛔ `ExploredCells` NON entra nell'artefatto, ed e' una scelta: nessun controllo lo legge —
		// `ClassifyTarget` guarda `VisibleCells` e `Contacts` — e non scade mai, quindi crescerebbe di turno
		// in turno dentro ogni file per un dato che nessuno consuma.
		TestEqual(TEXT("le celle esplorate restano fuori dall'artefatto"),
			Read.PlanningKnowledge[0].ExploredCells.Num(), 0);
		if (TestEqual(TEXT("il contatto"), Read.PlanningKnowledge[0].Contacts.Num(), 1))
		{
			TestEqual(TEXT("l'unita' del contatto"), Read.PlanningKnowledge[0].Contacts[0].StableUnitId, 7);
			TestTrue(TEXT("la cella del contatto"),
				Read.PlanningKnowledge[0].Contacts[0].Cell == FRTCellId(3, 0, 0));
		}
	}
	TestEqual(TEXT("due conoscenze di Blast"), Read.BlastKnowledge.Num(), 2);

	if (TestEqual(TEXT("una scelta di bot"), Read.BotDecisions.Num(), 1))
	{
		TestEqual(TEXT("chi ha scelto"), Read.BotDecisions[0].UnitId, 3);
		TestEqual(TEXT("il bersaglio scelto"), Read.BotDecisions[0].TargetUnitId, 7);
		TestEqual(TEXT("la squadra del bersaglio"), Read.BotDecisions[0].TargetTeamId, 1);
		TestTrue(TEXT("la cella del bersaglio"), Read.BotDecisions[0].TargetCell == FRTCellId(1, 0, 0));
	}

	// ⚠️ I verdetti sono l'unico record che non ha una struttura ricca: se il round-trip li perdesse, il
	// controllo di coerenza di [D-223] resterebbe verde su un array vuoto.
	if (TestEqual(TEXT("due verdetti"), Read.Verdicts.Num(), 2))
	{
		TestTrue(TEXT("il primo autorizza la squadra 0 e non la 1"),
			Read.Verdicts[0].Verdict.AllowsTeam(0) && !Read.Verdicts[0].Verdict.AllowsTeam(1));
		TestTrue(TEXT("il secondo autorizza tutti"), Read.Verdicts[1].Verdict.AllowsTeam(1));

		// 🔴 Il SOGGETTO viaggia col verdetto, e senza di lui il verdetto non e' verificabile: la cella
		// contro cui e' stato congelato non si ricava dalla voce archiviata.
		TestEqual(TEXT("l'unita' del soggetto"), Read.Verdicts[0].SubjectUnitId, 7);
		TestEqual(TEXT("la squadra del soggetto"), Read.Verdicts[0].SubjectTeamId, 1);
		TestTrue(TEXT("la cella del soggetto"), Read.Verdicts[0].SubjectCell == FRTCellId(1, 0, 0));
	}

	return true;
}

/**
 * Fail-closed sulle versioni sconosciute, come `DeserializeTurnLog` e `ManifestFromJson`: rifiutare invece
 * di interpretare campi arbitrari.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayAuditUnknownVersionTest,
	"RefactorTactics.Replay.Audit.UnknownVersionIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayAuditUnknownVersionTest::RunTest(const FString&)
{
	FRTTurnAudit Read;
	TestFalse(TEXT("una versione futura si rifiuta"),
		URTReplayAuditLibrary::AuditFromJson(
			TEXT("{\"Version\":9999,\"MatchId\":\"00000000000000000000000000000001\",\"TurnNumber\":1}"), Read));
	TestFalse(TEXT("un JSON senza versione si rifiuta"),
		URTReplayAuditLibrary::AuditFromJson(TEXT("{\"TurnNumber\":1}"), Read));
	TestFalse(TEXT("cio' che non e' JSON si rifiuta"), URTReplayAuditLibrary::AuditFromJson(TEXT("nope"), Read));

	return true;
}

/**
 * 🔴 L'aggancio: l'audit di **un'altra partita** o di **un altro turno** non si carica.
 *
 * Senza, due archivi in due cartelle vicine si scambierebbero le prove, e un'evidenza attribuita alla
 * partita sbagliata e' peggio di un'evidenza mancante — sembra una risposta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayAuditForeignArtifactTest,
	"RefactorTactics.Replay.Audit.ForeignArtifactIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayAuditForeignArtifactTest::RunTest(const FString&)
{
	const FString Root = AuditRoot(TEXT("Estraneo"));
	Clean(Root);

	const FGuid Mine = FGuid(1, 1, 1, 1);
	const FGuid Other = FGuid(2, 2, 2, 2);

	TestTrue(TEXT("si scrive"),
		URTReplayAuditLibrary::RecordTurnAudit(Root, SaturatedAudit(Mine, 2, 42)));

	FRTTurnAudit Read;
	TestTrue(TEXT("si rilegge con la chiave giusta"),
		URTReplayAuditLibrary::LoadTurnAudit(Root, Mine, 2, Read));
	TestFalse(TEXT("un'altra partita non si carica"),
		URTReplayAuditLibrary::LoadTurnAudit(Root, Other, 2, Read));
	TestFalse(TEXT("un altro turno non si carica"),
		URTReplayAuditLibrary::LoadTurnAudit(Root, Mine, 3, Read));

	// L'ANCORA: con un hash atteso diverso, l'artefatto non e' la coppia di quella traccia. Senza questo
	// confronto il campo che la struct chiama «l'aggancio vero» sarebbe decorativo.
	TestTrue(TEXT("con l'ancora giusta si carica"),
		URTReplayAuditLibrary::LoadTurnAudit(Root, Mine, 2, Read, /*ExpectedOrderedHash*/ 42));
	TestFalse(TEXT("con l'ancora di un'altra traccia non si carica"),
		URTReplayAuditLibrary::LoadTurnAudit(Root, Mine, 2, Read, /*ExpectedOrderedHash*/ 43));

	// Il caso che l'aggancio esiste per prendere: il file del turno 2 copiato sopra quello del turno 3.
	// Il nome del file direbbe «turno 3», il contenuto dice «turno 2», e vince il contenuto.
	const FString Src = FPaths::Combine(Root, Mine.ToString(EGuidFormats::Digits),
		URTReplayAuditLibrary::TurnAuditFileName(2));
	const FString Dst = FPaths::Combine(Root, Mine.ToString(EGuidFormats::Digits),
		URTReplayAuditLibrary::TurnAuditFileName(3));
	FString Payload;
	if (TestTrue(TEXT("il file scritto si legge"), FFileHelper::LoadFileToString(Payload, *Src)))
	{
		FFileHelper::SaveStringToFile(Payload, *Dst);
		TestFalse(TEXT("un file rinominato non inganna: vince il contenuto"),
			URTReplayAuditLibrary::LoadTurnAudit(Root, Mine, 3, Read));
	}

	Clean(Root);
	return true;
}

/**
 * 🔑 **Il controllo che nessuno faceva: l'anti-vacuita' di [D-223].**
 *
 * `D-223` impone che il verdetto sia calcolato quando il fatto e' accaduto e che i canali CHIAMINO il
 * predicato invece di riderivarlo — ma nessuno verifica che un verdetto congelato **corrisponda** alla
 * conoscenza da cui dichiara di derivare. Con i due record accanto, si rifa' il conto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayAuditVerdictCoherenceTest,
	"RefactorTactics.Replay.Audit.FrozenVerdictsMatchTheRecordedKnowledge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayAuditVerdictCoherenceTest::RunTest(const FString&)
{
	// La squadra 0 vede (1,0): un soggetto li' e' `Allowed` per lei e ignoto alla squadra 1.
	FRTTurnAudit A;
	A.MatchId = FGuid(3, 3, 3, 3);
	A.TurnNumber = 1;
	A.BlastKnowledge.Add(Knowledge(0, 1));

	FRTTeamKnowledge Cieca;
	Cieca.TeamId = 1;
	Cieca.TurnNumber = 1;
	A.BlastKnowledge.Add(Cieca);

	FRTAuditVerdictRecord R;
	// La fase decide contro QUALE istantanea si ricalcola: `Blast` sceglie `BlastKnowledge`.
	R.Phase = ERTMatchPhase::Blast;
	R.SubjectUnitId = 7;
	R.SubjectTeamId = 1;
	R.SubjectCell = FRTCellId(1, 0, 0);
	R.Verdict = URTReplayAuditLibrary::RecomputeVerdict(A, R);
	A.Verdicts.Add(R);

	// 🔑 Il controllo e' AUTOSUFFICIENTE: gli basta l'artefatto, e non le voci ne' una tabella passata da
	// fuori. E' cio' che rende l'evidenza leggibile da chi apre un archivio e non ha altro.
	TestEqual(TEXT("il verdetto ricalcolato coincide con quello registrato"),
		URTReplayAuditLibrary::FindVerdictMismatches(A).Num(), 0);

	// ⚠️ ANTI-VACUITA': si altera il FILE, non il mondo. Alterare il mondo cambierebbe la partita;
	// alterare la prova deve far diventare rosso il controllo, ed e' l'unica cosa che dimostra che il
	// controllo guarda davvero il dato registrato.
	FRTTurnAudit Manomesso = A;
	Manomesso.BlastKnowledge[0].VisibleCells.Reset(); // la squadra 0 «non vedeva piu'» quella cella
	TestTrue(TEXT("alterare la conoscenza registrata fa comparire una divergenza"),
		URTReplayAuditLibrary::FindVerdictMismatches(Manomesso).Num() > 0);

	// E il verso opposto: alterare il VERDETTO registrato, lasciando intatta la conoscenza.
	FRTTurnAudit VerdettoFalso = A;
	VerdettoFalso.Verdicts[0].Verdict = FRTKnowledgeVerdict::Everyone();
	TestTrue(TEXT("alterare il verdetto registrato fa comparire una divergenza"),
		URTReplayAuditLibrary::FindVerdictMismatches(VerdettoFalso).Num() > 0);

	// 🔴 **Un fatto di MONDO non ha un verdetto da ricalcolare: ne ha uno per REGOLA.** Senza questo ramo il
	// controllo accuserebbe il gioco a ogni turno di ogni partita su qualunque mappa con un obiettivo —
	// `AppendLogEntry(Objective, nullptr)` scrive `Everyone()` senza consultare nessuna conoscenza, e
	// ricalcolarlo darebbe al piu' la maschera delle squadre registrate. Trovato in code review.
	FRTTurnAudit ConMondo = A;
	FRTAuditVerdictRecord Mondo;
	Mondo.Phase = ERTMatchPhase::Cleanup;
	Mondo.SubjectUnitId = INDEX_NONE;   // nessun soggetto: e' un fatto del mondo
	Mondo.Verdict = FRTKnowledgeVerdict::Everyone();
	ConMondo.Verdicts.Add(Mondo);
	TestEqual(TEXT("un fatto di mondo con Everyone non e' una divergenza"),
		URTReplayAuditLibrary::FindVerdictMismatches(ConMondo).Num(), 0);

	// ✅ E la regola si VERIFICA invece di essere saltata: un fatto di mondo che non fosse leggibile da tutti
	// sarebbe un difetto, e questo lo direbbe.
	FRTTurnAudit MondoStorto = ConMondo;
	MondoStorto.Verdicts.Last().Verdict = FRTKnowledgeVerdict::NoOne();
	TestTrue(TEXT("un fatto di mondo che NON e' Everyone e' una divergenza"),
		URTReplayAuditLibrary::FindVerdictMismatches(MondoStorto).Num() > 0);

	// 🔴 **La FASE decide contro quale istantanea si ricalcola, e non e' un campo decorativo.** Lo dimostra
	// una conoscenza di Planning DIVERSA da quella di Blast: la stessa voce, letta come nata nel Dash, deve
	// ricalcolarsi contro l'altra istantanea e divergere. E' la regola che un rosso su una partita vera ha
	// insegnato — e senza questa riga tornerebbe a perdersi al primo refactor.
	FRTTurnAudit ConPlanning = A;
	FRTTeamKnowledge PlanningCieca;
	PlanningCieca.TeamId = 0;
	PlanningCieca.TurnNumber = 1;
	ConPlanning.PlanningKnowledge.Add(PlanningCieca);       // la squadra 0 al Planning non vedeva niente
	ConPlanning.PlanningKnowledge.Add(Cieca);
	ConPlanning.Verdicts[0].Phase = ERTMatchPhase::Dash;    // nata PRIMA del Blast
	TestTrue(TEXT("una voce nata prima del Blast si ricalcola sull'istantanea di Planning"),
		URTReplayAuditLibrary::FindVerdictMismatches(ConPlanning).Num() > 0);

	return true;
}

/**
 * 🔴 **L'equita' del bot, e stavolta e' verificabile.**
 *
 * La domanda che un archivio deve reggere e' *«il bot ha visto piu' di me?»*, e la si pone al `ClassifyTarget`
 * di produzione sulla conoscenza di **Planning** registrata: la STESSA domanda che il cancello ha posto in
 * partita, non una simile.
 *
 * ⚠️ **Si giudica la SCELTA, non l'effetto.** Una prima stesura guardava la cella colpita, ed era
 * insostenibile: `TgtCell` su una voce di combattimento e' la cella della VITTIMA, e un'area verso una cella
 * nota che investe un nemico ignoto accanto e' legittima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayAuditFairnessTest,
	"RefactorTactics.Replay.Audit.BotNeverTargetedWhatItDidNotKnow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayAuditFairnessTest::RunTest(const FString&)
{
	// La squadra 0 vede (1,0) e (2,0), e ricorda un contatto in (3,0).
	FRTTurnAudit A;
	A.MatchId = FGuid(5, 5, 5, 5);
	A.TurnNumber = 1;
	A.PlanningKnowledge.Add(Knowledge(0, 1));

	auto Scelta = [](int32 TargetUnitId, const FRTCellId& Cell)
	{
		FRTAuditBotDecision D;
		D.UnitId = 3;
		D.TeamId = 0;
		D.TargetUnitId = TargetUnitId;
		D.TargetTeamId = 1;
		D.TargetCell = Cell;
		return D;
	};

	// Un bersaglio in una cella VISTA: lecito.
	FRTTurnAudit Visto = A;
	Visto.BotDecisions.Add(Scelta(/*Target*/ 9, FRTCellId(2, 0, 0)));
	TestEqual(TEXT("scegliere un bersaglio in una cella vista non e' una violazione"),
		URTReplayAuditLibrary::FindUnauthorizedTargets(Visto).Num(), 0);

	// Il contatto RICORDATO: `CellOnly` e' conoscenza, non onniscienza.
	FRTTurnAudit Ricordato = A;
	Ricordato.BotDecisions.Add(Scelta(/*Target*/ 7, FRTCellId(3, 0, 0)));
	TestEqual(TEXT("scegliere un contatto ricordato non e' una violazione"),
		URTReplayAuditLibrary::FindUnauthorizedTargets(Ricordato).Num(), 0);

	// 🔑 ANTI-VACUITA': senza questo caso il controllo potrebbe non guardare niente e restare verde.
	FRTTurnAudit Onnisciente = A;
	Onnisciente.BotDecisions.Add(Scelta(/*Target*/ 11, FRTCellId(9, 9, 0)));
	TestEqual(TEXT("scegliere un bersaglio che la squadra non conosceva e' una violazione"),
		URTReplayAuditLibrary::FindUnauthorizedTargets(Onnisciente).Num(), 1);

	// ⚠️ E la mutazione va fatta sul FILE: togliere la cella dalla conoscenza REGISTRATA deve far diventare
	// rossa una scelta che era lecita. E' cio' che dimostra che il controllo legge il dato registrato.
	FRTTurnAudit Manomesso = Visto;
	Manomesso.PlanningKnowledge[0].VisibleCells.Reset();
	Manomesso.PlanningKnowledge[0].Contacts.Reset();
	TestTrue(TEXT("alterare la conoscenza registrata rende illecita una scelta che era lecita"),
		URTReplayAuditLibrary::FindUnauthorizedTargets(Manomesso).Num() > 0);

	// Nessun attacco pianificato: non c'e' una scelta da giudicare, e assolvere in silenzio sarebbe diverso
	// dal non avere niente da dire.
	FRTTurnAudit SenzaBersaglio = A;
	SenzaBersaglio.BotDecisions.Add(Scelta(INDEX_NONE, FRTCellId()));
	TestEqual(TEXT("una scelta senza bersaglio non si giudica"),
		URTReplayAuditLibrary::FindUnauthorizedTargets(SenzaBersaglio).Num(), 0);

	// ⛔ Una scelta di una squadra senza conoscenza registrata non e' «lecita»: e' NON VERIFICABILE, ed e' un
	// difetto dell'archivio che va detto. Un controllo che salta cio' che non sa leggere assolve.
	FRTTurnAudit SenzaConoscenza;
	SenzaConoscenza.MatchId = A.MatchId;
	SenzaConoscenza.TurnNumber = 1;
	SenzaConoscenza.BotDecisions.Add(Scelta(9, FRTCellId(2, 0, 0)));
	TestEqual(TEXT("una scelta senza conoscenza registrata si dichiara, non si assolve"),
		URTReplayAuditLibrary::FindUnauthorizedTargets(SenzaConoscenza).Num(), 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
