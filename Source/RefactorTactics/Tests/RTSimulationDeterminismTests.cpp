// Gate di DETERMINISMO della simulazione (CP 12.1).
//
// Il catalogo di bilanciamento nomina questi test per nome e li rende vincolanti: sono gli ultimi due dei
// dieci richiesti. La proprieta' che verificano e' quella su cui poggia tutto il resto — se la stessa
// partita giocata due volte finisce diversamente, nessun altro test dice piu' niente, perche' il suo verde
// potrebbe essere un caso.
//
// ATTENZIONE AL PRIMO NOME, che diverge dal catalogo di proposito (D-103, `#538`).
// Il catalogo v0.1 (p.24 §15) lo chiama `RefactorTactics.Simulation.DeterministicReplay`, e nel suo
// contesto non sbagliava: descrive «stesso snapshot, stesso seed, 100 ripetizioni, checksum identico»,
// cioe' una RI-SIMULAZIONE, e usa «replay» nel senso comune di «rigioca». Ma ADR-0009 ha reso «replay»
// una parola riservata del progetto — riproduzione di una traccia SENZA ricalcolo — e con quella
// definizione il vecchio nome insegnava l'opposto di cio' che il file fa. Qui si ri-simula attraverso il
// resolver: e' il VERIFIER, non il Player. Da qui `Replay.Verifier.ResimulationIsDeterministic`, accanto a
// `Replay.Verifier.ReportsFirstDivergence` e in opposizione a `Replay.Player.RunsWithoutResolver`.
// Gli altri test del file restano `Simulation.*`: i loro nomi non mentono e il rename si ferma dove
// l'ADR lo chiedeva.
//
// Si appoggiano allo Scenario Harness invece di ricostruire una partita a mano: cosi' il determinismo e'
// verificato sul percorso di gioco REALE (piani -> snapshot -> resolver -> stato), non su una simulazione
// scritta apposta per passare.

#include "Misc/AutomationTest.h"
#include "Replay/RTBoundaryChecksum.h"
#include "RTWorldFixtures.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTReactionOpportunityTypes.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Algo/Reverse.h"
#include "Replay/RTReplayRecorderLibrary.h"   // `#2196`: l'archivio su disco, non la traccia in memoria
#include "Turn/RTTurnRules.h"                 // `FRTMatchRules`: senza formato la registrazione si rifiuta
#include "Turn/RTMatchFormatLibrary.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Il mondo di prova e l'esecuzione isolata vivono in `RTWorldFixtures.h`: erano dichiarati qui con un
	// nome tutto loro perche' la unity build fonde le translation unit e due namespace anonimi con la stessa
	// funzione collidono. Un namespace NOMINATO non ha quel problema, ed e' la stessa entita' per tutti.
	// Le chiamate restano QUALIFICATE e non passano da un `using`: in unity build questo namespace anonimo e'
	// lo stesso di ogni altro file di test, e un `using` qui sarebbe visibile a tutti loro.

	bool LoadDeterminismScenario(FAutomationTestBase& Test, const TCHAR* Id, FRTTestScenario& Out)
	{
		FString Error;
		const FString Path = URTScenarioIndex::ResolvePath(Id, Error);
		if (Path.IsEmpty() || !URTScenarioLoader::LoadFromFile(Path, Out, Error))
		{
			Test.AddError(FString::Printf(TEXT("scenario '%s' non caricabile: %s"), Id, *Error));
			return false;
		}
		return true;
	}

	/**
	 * Le voci di TUTTI i turni di un risultato, rilette dalla traccia serializzata (`#886`).
	 *
	 * Per turno e non in blocco perche' e' cosi' che l'harness le consegna, e concatenarle qui e' l'unica
	 * cosa che il Verifier deve fare per avere la partita intera: le chiavi portano gia' `TurnNumber`, quindi
	 * due turni non si confondono.
	 */
	TArray<FRTTurnLogEntry> AllEntries(const FRTTestResult& Result)
	{
		TArray<FRTTurnLogEntry> All;
		for (const FRTTurnTrace& Trace : Result.TurnTraces)
		{
			TArray<FRTTurnLogEntry> Entries;
			if (URTTurnLogLibrary::DeserializeTurnLog(Trace.Bytes, Entries))
			{
				All.Append(Entries);
			}
		}
		return All;
	}

	/** Quante voci di decisione la traccia porta, e quante di esse sono un `FIRE`. */
	void CountDecisionEntries(const TArray<FRTTurnLogEntry>& Entries, int32& OutDecisions, int32& OutFires)
	{
		OutDecisions = 0;
		OutFires = 0;
		for (const FRTTurnLogEntry& E : Entries)
		{
			if (E.Category != ERTLogCategory::ReactionDecision) { continue; }
			++OutDecisions;
			// ⚠️ **«E' un FIRE?» si chiede alla RISPOSTA** (`#1118`): `Chosen` dice che si e' deciso, non cosa.
			if (URTReactionOpportunityLibrary::FireResponseTarget(E.ReactionResponse) != INDEX_NONE)
			{
				++OutFires;
			}
		}
	}

	/** Quante voci di decisione portano questo esito. */
	/**
	 * Quante decisioni hanno risposto `FIRE`.
	 *
	 * ⚠️ **Non e' `CountOutcome(..., Chosen)`** (`#1118`): quella conta anche chi ha tenuto la cella e chi ha
	 * scartato, perche' la ragione e' la stessa per tutti e tre. La domanda «quante volte si e' sparato» si
	 * fa alla risposta, ed e' lo stesso predicato che il resolver usa per applicare il colpo.
	 */
	int32 CountFireResponses(const TArray<FRTTurnLogEntry>& Entries)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : Entries)
		{
			if (E.Category == ERTLogCategory::ReactionDecision
				&& URTReactionOpportunityLibrary::FireResponseTarget(E.ReactionResponse) != INDEX_NONE)
			{
				++N;
			}
		}
		return N;
	}

	int32 CountOutcome(const TArray<FRTTurnLogEntry>& Entries, ERTReactionDecisionOutcome Wanted)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : Entries)
		{
			if (E.Category == ERTLogCategory::ReactionDecision
				&& static_cast<ERTReactionDecisionOutcome>(E.Outcome) == Wanted)
			{
				++N;
			}
		}
		return N;
	}

	/**
	 * Esegue uno scenario come farebbe il VERIFIER: mondo nuovo, `TurnManager` armato con la traccia (vuota
	 * = ri-simulazione normale) e — se `Decider` c'e' — un decisore vivo che risponde comunque.
	 *
	 * Il manager si spawna PRIMA di `Run`: `FRTScenarioSession` riusa quello che trova nel mondo
	 * (`GetActorOfClass`) invece di crearne un secondo, ed e' l'unico punto in cui il test puo' configurarlo.
	 * Le divergenze si leggono prima di distruggere il mondo, per la stessa ragione.
	 */
	/**
	 * L'ARCHIVIO su disco di una corsa, letto turno per turno (`#2196`).
	 *
	 * ⚠️ **I byte, non le voci.** `GoldenEntriesMatch` confronta per HASH, e `UnitId`, `TurnNumber`,
	 * `Priority`, `ReactionInstanceId`, `OriginalTargetUnitId` nell'hash non entrano ([D-063]) ma
	 * `SerializeTurnLog` li scrive. Confrontare gli hash direbbe «identici» su archivi diversi su disco.
	 */
	struct FRTArchivioSuDisco
	{
		TArray<TArray<uint8>> Byte;
		TArray<TArray<FRTTurnLogEntry>> Voci;
		bool bCompleto = false;   // ogni turno atteso ha il suo file, e si e' letto
	};

	FString RadiceArchivio(const TCHAR* Nome)
	{
		return FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("VerifierArchivio"), Nome);
	}

	void PulisciRadice(const FString& Root)
	{
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }
	}

	/** Rilegge da disco i turni `1..TurniAttesi` di un archivio. Una lettura fallita si DICHIARA, non si salta. */
	FRTArchivioSuDisco LeggiArchivio(const FString& Root, const FGuid& MatchId, int32 TurniAttesi)
	{
		FRTArchivioSuDisco Out;
		Out.bCompleto = (MatchId.IsValid() && TurniAttesi > 0);

		for (int32 Turno = 1; Turno <= TurniAttesi && Out.bCompleto; ++Turno)
		{
			const FString Path = FPaths::Combine(
				URTReplayRecorderLibrary::MatchDirectory(Root, MatchId),
				URTReplayRecorderLibrary::TurnFileName(Turno));

			TArray<uint8> Byte;
			TArray<FRTTurnLogEntry> Voci;
			// ⚠️ Saltare una lettura fallita compatterebbe gli array, e da li' in poi `Voci[i]` non sarebbe
			// piu' il turno `i+1`: un archivio rotto diventerebbe indistinguibile da una divergenza vera.
			if (!FFileHelper::LoadFileToArray(Byte, *Path)
				|| !URTTurnLogLibrary::LoadTurnLogFromFile(Path, Voci))
			{
				Out.bCompleto = false;
				break;
			}
			Out.Byte.Add(MoveTemp(Byte));
			Out.Voci.Add(MoveTemp(Voci));
		}
		return Out;
	}

	/** Il primo turno in cui due archivi divergono sui byte, o `INDEX_NONE`. Indice a base 0. */
	int32 PrimoTurnoDiverso(const FRTArchivioSuDisco& A, const FRTArchivioSuDisco& B)
	{
		const int32 Comuni = FMath::Min(A.Byte.Num(), B.Byte.Num());
		for (int32 i = 0; i < Comuni; ++i)
		{
			if (A.Byte[i] != B.Byte[i]) { return i; }
		}
		return INDEX_NONE;
	}

	FRTTestResult RunAsVerifier(const FRTTestScenario& Scenario, const TArray<FRTTurnLogEntry>& Trace,
		TFunction<FString(const FRTReactionOpportunity&, int32)> Decider, TArray<FString>& OutDivergences,
		const FString* RadiceRegistrazione = nullptr, FRTArchivioSuDisco* OutArchivio = nullptr)
	{
		OutDivergences.Reset();

		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!World) { return FRTTestResult(); }

		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM)
		{
			RTWorldFixtures::DestroyWorld(World);
			return FRTTestResult();
		}

		if (Trace.Num() > 0)
		{
			TM->ArmRecordedReactionDecisions(Trace);
		}

		// 🔑 **La registrazione su DISCO** (`#2196`): senza, questo helper produce una traccia in memoria, e
		// «ri-simulare da un archivio» resterebbe indistinguibile da «ri-simulare da un array».
		FGuid MatchId;
		if (RadiceRegistrazione != nullptr)
		{
			// ⚠️ **Il formato va risolto PRIMA di registrare, o `BeginReplayRecording` esce in silenzio.**
			// Si rifiuta con `MatchRules.FormatId.IsNone()`, perche' un archivio con formato assente non e'
			// confrontabile con niente. Il percorso di PARTITA lo riceve dal GameMode (`ApplyMatchFormat`);
			// quello dello SCENARIO no, e senza questa riga l'archivio resta vuoto — *misurato: la prima
			// stesura falliva su «l'archivio di riferimento e' completo su disco»*.
			FRTMatchRules Rules = TM->GetMatchRules();
			if (Rules.FormatId.IsNone())
			{
				Rules.FormatId = URTMatchFormatLibrary::Skirmish2v2FormatId;
				TM->SetMatchRules(Rules);
			}

			TM->ReplaysRootOverride = *RadiceRegistrazione;
			TM->BeginReplayRecording();
			MatchId = TM->GetReplayMatchId();
		}
		if (Decider)
		{
			TM->ReactionDecider.BindLambda(Decider);
		}

		const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);

		// A corsa finita «non ancora consumata» e «mai consumata» smettono di coincidere: e' l'istante in cui
		// una chiave orfana diventa dichiarabile.
		TM->ReportOrphanRecordedDecisions();
		OutDivergences = TM->GetVerificationDivergences();

		// L'archivio si rilegge PRIMA di distruggere il mondo, come le divergenze e per la stessa ragione.
		if (OutArchivio != nullptr && RadiceRegistrazione != nullptr)
		{
			*OutArchivio = LeggiArchivio(*RadiceRegistrazione, MatchId, Result.TurnTraces.Num());
		}

		RTWorldFixtures::DestroyWorld(World);
		return Result;
	}

	/** Lo stesso scenario senza le proprie `decisions`: nessuno risponde, se non la traccia. */
	FRTTestScenario WithoutScriptedDecisions(const FRTTestScenario& Scenario)
	{
		FRTTestScenario Stripped = Scenario;
		for (FRTScenarioTurn& Turn : Stripped.Turns)
		{
			Turn.Decisions.Reset();
		}
		return Stripped;
	}
}

/**
 * Stessa scenario + stesse definizioni ⇒ stesso stato finale, su **100 ripetizioni**.
 *
 * Cento e non due perche' il non-determinismo che conta e' quello raro: un `TMap` iterato in ordine diverso,
 * un puntatore usato come chiave, un indice che dipende dall'ordine di spawn. Con due ripetizioni un difetto
 * del genere passa quasi sempre; con cento si vede.
 */
// ⚠️ `ClientContext` oltre a `EditorContext`, e non per simmetria con i vicini: e' uno dei **dieci test
// vincolanti** del catalogo (`roadmap-v0.1.md` §6), e CP 12.3 (`#83`) chiede che la suite giri anche in un
// pacchetto. Con il solo `EditorContext` il controller lo filtra fuori dal target `Game` — misurato:
// `Automation List` in un pacchetto Development registrava **zero** test su 875, tutti dichiarati solo Editor.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayVerifierResimulationTest,
	"RefactorTactics.Replay.Verifier.ResimulationIsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
// ⚠️ **Due scenari e non uno, dal 2026-08-18 (`#886`).** `Movement.Collision` copre le collisioni e non ha
// finestre di reazione: su di lui il determinismo di una partita CON decisioni non e' mai stato verificato.
// `Spec.Overwatch.HoldThenFire` ne ha due, e una spende la carica troncando un movimento — cioe' il caso in
// cui una decisione cambia lo stato successivo. Il primo NON viene sostituito: sono due proprieta' diverse, e
// scambiare la copertura per un rinnovo e' il modo in cui una suite dimagrisce senza che nessuno lo decida.
bool FRTReplayVerifierResimulationTest::RunTest(const FString&)
{
	// ➕ **Un terzo dal 2026-09-01 (`#1830`)**: `Combat.BlockedByInteriorWall` e' l'unico che esercita la
	// GEOMETRIA INTRA-CELLA, che da `D-269` decide chi vede chi e quindi entra nell'hash della mappa. Nessuno
	// dei due sopra ne ha, quindi su di loro il determinismo della nuova primitiva non e' mai verificato. Vale
	// il criterio gia' scritto per il secondo: si AGGIUNGE, non sostituisce — sono proprieta' diverse.
	const TCHAR* Ids[] = {
		TEXT("Movement.Collision"),
		TEXT("Spec.Overwatch.HoldThenFire"),
		TEXT("Combat.BlockedByInteriorWall")
	};

	for (const TCHAR* Id : Ids)
	{
		FRTTestScenario Scenario;
		if (!LoadDeterminismScenario(*this, Id, Scenario)) { return false; }

		const FRTTestResult First = RTWorldFixtures::RunScenarioIsolated(Scenario);
		if (First.Outcome == ERTTestOutcome::Error)
		{
			AddError(FString::Printf(TEXT("[%s] la prima esecuzione e' fallita: %s"), Id, *First.ErrorMessage));
			return false;
		}
		// Un hash a zero significherebbe «nessuno stato»: confrontare zeri fra loro non proverebbe nulla.
		TestNotEqual(FString::Printf(TEXT("[%s] lo stato finale produce un hash reale"), Id), First.StateHash, 0u);

		constexpr int32 Repetitions = 100;
		int32 Divergences = 0;
		for (int32 I = 1; I < Repetitions; ++I)
		{
			const FRTTestResult Again = RTWorldFixtures::RunScenarioIsolated(Scenario);
			if (Again.StateHash != First.StateHash || Again.OutcomeString() != First.OutcomeString())
			{
				++Divergences;
				if (Divergences <= 3) // le prime tre bastano a diagnosticare; oltre e' rumore
				{
					AddError(FString::Printf(
						TEXT("[%s] divergenza alla ripetizione %d: hash %08x invece di %08x, esito '%s' invece di '%s'"),
						Id, I, Again.StateHash, First.StateHash, *Again.OutcomeString(), *First.OutcomeString()));
				}
			}
		}

		TestEqual(FString::Printf(TEXT("[%s] nessuna divergenza su %d ripetizioni"), Id, Repetitions),
			Divergences, 0);
	}

	return true;
}

/**
 * Il Verifier ri-simula leggendo le decisioni DALLA TRACCIA, e non le chiede a nessuno (`#886`, voce 1).
 *
 * Le tre esecuzioni servono tutte e tre, e la terza e' quella che rende il test non-vacuo:
 *
 *     A  scenario CON `decisions`          -> la partita di riferimento, e la traccia che produce
 *     B  scenario SENZA, traccia armata    -> deve dare lo STESSO stato finale di A
 *     C  scenario SENZA, senza traccia     -> deve dare uno stato DIVERSO
 *
 * Senza la C, la B passerebbe anche con una feature che non fa niente: basterebbe che le decisioni non
 * contassero. La C dimostra che contano — e' l'unico `FIRE` dello scenario a troncare il movimento di Phase,
 * e senza decisore quel movimento arriva in fondo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayVerifierDecisionsFromTraceTest,
	"RefactorTactics.Replay.Verifier.ReactionDecisionsComeFromTheTrace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayVerifierDecisionsFromTraceTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadDeterminismScenario(*this, TEXT("Spec.Overwatch.HoldThenFire"), Scenario)) { return false; }

	TArray<FString> Divergenze;
	const FRTTestResult A = RunAsVerifier(Scenario, {}, nullptr, Divergenze);
	if (A.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("la partita di riferimento e' fallita: %s"), *A.ErrorMessage));
		return false;
	}
	TestNotEqual(TEXT("la partita di riferimento produce un hash reale"), A.StateHash, 0u);

	const TArray<FRTTurnLogEntry> Traccia = AllEntries(A);
	int32 Decisioni = 0, Fuochi = 0;
	CountDecisionEntries(Traccia, Decisioni, Fuochi);

	// La fixture DEVE avere due finestre, o la voce sull'identita' non e' falsificabile: con una sola,
	// indicizzare per chiave e indicizzare per ordine sono indistinguibili.
	if (!TestTrue(FString::Printf(TEXT("la traccia porta almeno due decisioni (ne ha %d)"), Decisioni),
		Decisioni >= 2))
	{
		return false;
	}
	TestEqual(TEXT("e una di esse e' il FIRE che tronca il movimento"), Fuochi, 1);

	// B: nessuno risponde in questa esecuzione, se non la traccia.
	const FRTTestScenario Muto = WithoutScriptedDecisions(Scenario);
	const FRTTestResult B = RunAsVerifier(Muto, Traccia, nullptr, Divergenze);
	TestEqual(TEXT("la ri-simulazione dalla traccia dichiara di non aver avuto una sorgente propria"),
		B.DecisionSource, FString(TEXT("none")));
	TestEqual(FString::Printf(TEXT("stesso stato finale della partita reale (%08x vs %08x)"),
		B.StateHash, A.StateHash), B.StateHash, A.StateHash);
	TestEqual(TEXT("e nessun disaccordo fra traccia e ri-simulazione"), Divergenze.Num(), 0);
	if (Divergenze.Num() > 0)
	{
		AddError(FString::Printf(TEXT("primo disaccordo: %s"), *Divergenze[0]));
	}

	// C: la controprova. Senza traccia e senza decisore le finestre restano senza risposta, il `FIRE` non
	// avviene e il bersaglio arriva dove la traccia dice che non e' arrivato.
	const FRTTestResult C = RunAsVerifier(Muto, {}, nullptr, Divergenze);
	TestNotEqual(TEXT("senza traccia lo stato finale e' DIVERSO (altrimenti le decisioni non contano)"),
		C.StateHash, A.StateHash);

	return true;
}


/**
 * Una risposta REGISTRATA batte il decisore corrente, che non viene interrogato affatto (`#886`, voce 3).
 *
 * E' la precedenza verificata in NEGATIVO: un decisore vivo, collegato, che risponderebbe `HOLD` a tutto —
 * cioe' l'opposto del `FIRE` che la traccia contiene.
 *
 * ⚠️ Le due asserzioni servono INSIEME. Il solo hash passerebbe anche se il decisore fosse interrogato e la
 * sua risposta poi scartata: sarebbe la cosa giusta fatta per caso, e il giorno in cui il decisore e' una UI
 * umana significherebbe aprire una finestra a un giocatore per poi ignorarla. Il solo contatore passerebbe
 * con una feature che non applica niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayVerifierRecordedBeatsLiveTest,
	"RefactorTactics.Replay.Verifier.RecordedResponseBeatsLiveDecider",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayVerifierRecordedBeatsLiveTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadDeterminismScenario(*this, TEXT("Spec.Overwatch.HoldThenFire"), Scenario)) { return false; }

	TArray<FString> Divergenze;
	const FRTTestResult A = RunAsVerifier(Scenario, {}, nullptr, Divergenze);
	if (A.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("la partita di riferimento e' fallita: %s"), *A.ErrorMessage));
		return false;
	}

	const TArray<FRTTurnLogEntry> Traccia = AllEntries(A);
	int32 Decisioni = 0, Fuochi = 0;
	CountDecisionEntries(Traccia, Decisioni, Fuochi);
	if (!TestTrue(TEXT("la traccia contiene un FIRE, che e' cio' che il decisore ostile negherebbe"), Fuochi > 0))
	{
		return false;
	}

	int32 Interrogazioni = 0;
	auto Ostile = [&Interrogazioni](const FRTReactionOpportunity&, int32) -> FString
	{
		++Interrogazioni;
		return URTReactionOpportunityLibrary::HoldResponse();
	};

	const FRTTestResult B = RunAsVerifier(WithoutScriptedDecisions(Scenario), Traccia, Ostile, Divergenze);

	TestEqual(TEXT("il decisore vivo non viene interrogato nemmeno una volta"), Interrogazioni, 0);
	TestEqual(FString::Printf(TEXT("e l'esito e' quello della traccia, non il suo (%08x vs %08x)"),
		B.StateHash, A.StateHash), B.StateHash, A.StateHash);
	TestEqual(TEXT("nessun disaccordo"), Divergenze.Num(), 0);

	return true;
}


/**
 * Una risposta registrata che nessuna finestra reclama NON finisce su un'altra, e il disaccordo si vede
 * (`#886`, voce 4). `OpportunityId` e' un'identita', non un ordine.
 *
 * La traccia si PERTURBA invece di costruire uno scenario apposta: si prende quella vera e si cambia la
 * chiave del `FIRE` in una che la ri-simulazione non produrra' mai. Cosi' la partita e' identica e l'unica
 * variabile e' l'identita' della risposta — che e' precisamente cio' che il test deve isolare.
 *
 * ⚠️ **E' il test che cade se si indicizza per ordine di comparsa.** Con un array, la seconda finestra
 * prenderebbe la seconda risposta senza guardarne la chiave: la partita tornerebbe identica all'originale,
 * zero divergenze, e la perturbazione sarebbe invisibile. Con una fixture a UNA sola finestra le due
 * implementazioni sarebbero indistinguibili — per questo lo scenario ne ha due.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayVerifierOrphanRecordedResponseTest,
	"RefactorTactics.Replay.Verifier.OrphanRecordedResponseIsReported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayVerifierOrphanRecordedResponseTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadDeterminismScenario(*this, TEXT("Spec.Overwatch.HoldThenFire"), Scenario)) { return false; }

	TArray<FString> Divergenze;
	const FRTTestResult A = RunAsVerifier(Scenario, {}, nullptr, Divergenze);
	if (A.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("la partita di riferimento e' fallita: %s"), *A.ErrorMessage));
		return false;
	}

	TArray<FRTTurnLogEntry> Perturbata = AllEntries(A);
	const FString ChiaveInventata = TEXT("99|3|7|5|action.overwatch|0");
	FString ChiaveOriginale;
	for (FRTTurnLogEntry& E : Perturbata)
	{
		if (E.Category == ERTLogCategory::ReactionDecision
			&& URTReactionOpportunityLibrary::FireResponseTarget(E.ReactionResponse) != INDEX_NONE)
		{
			ChiaveOriginale = E.OpportunityId;
			E.OpportunityId = ChiaveInventata;
			break;
		}
	}
	if (!TestFalse(TEXT("la traccia conteneva un FIRE da perturbare"), ChiaveOriginale.IsEmpty()))
	{
		return false;
	}
	TestNotEqual(TEXT("e la chiave inventata non e' quella vera"), ChiaveInventata, ChiaveOriginale);

	const FRTTestResult B = RunAsVerifier(WithoutScriptedDecisions(Scenario), Perturbata, nullptr, Divergenze);

	// (a) la risposta non e' stata applicata a NESSUNA finestra: nella traccia della ri-simulazione non c'e'
	// un solo `FIRE`. E' l'asserzione che cade con l'indicizzazione per ordine.
	const TArray<FRTTurnLogEntry> TracciaB = AllEntries(B);
	TestEqual(TEXT("nessun FIRE nella ri-simulazione: la risposta orfana non e' scivolata su un'altra finestra"),
		CountFireResponses(TracciaB), 0);
	TestNotEqual(TEXT("e lo stato finale differisce da quello reale"), B.StateHash, A.StateHash);

	// (b) il disaccordo e' SEGNALATO, e in entrambe le sue facce: la finestra vera senza risposta, e la
	// risposta inventata senza finestra. Una sola delle due lascerebbe l'altra meta' silenziosa.
	bool bOrfanaNominata = false;
	bool bFinestraScoperta = false;
	for (const FString& D : Divergenze)
	{
		if (D.Contains(TEXT("orfana")) && D.Contains(ChiaveInventata)) { bOrfanaNominata = true; }
		if (D.Contains(TEXT("non coperta")) && D.Contains(ChiaveOriginale)) { bFinestraScoperta = true; }
	}
	TestTrue(TEXT("la chiave orfana e' nominata nel verdetto"), bOrfanaNominata);
	TestTrue(TEXT("e la finestra rimasta scoperta anche"), bFinestraScoperta);
	if (!bOrfanaNominata || !bFinestraScoperta)
	{
		for (const FString& D : Divergenze)
		{
			AddError(FString::Printf(TEXT("divergenza riportata: %s"), *D));
		}
	}

	return true;
}


/**
 * Due risposte registrate con la STESSA chiave sono una traccia ambigua, e si dice invece di arrotondare.
 *
 * `TMap::Add` sovrascriverebbe, e una risposta sparirebbe senza che nessuno se ne accorga — la stessa
 * famiglia di difetti che `#886` esiste per chiudere, spostata di un anello: dentro la costruzione della
 * mappa invece che dentro la consultazione.
 *
 * ⚠️ **Il caso oggi non si produce in partita**, e il test lo fabbrica di proposito duplicando una voce:
 * `FRTReactionOpportunityKey` non porta l'istanza della reaction — i suoi sei campi sono turno, macro-fase,
 * micro-step, proprietario, reaction e `Seq` — quindi due Overwatch della **stessa** unita' nello stesso
 * micro-step ricadrebbero sulla stessa chiave. `ResolveReactionBoundary` lo dichiara gia' per il proprio
 * conto. Un difetto che il codice sa di poter avere merita un test prima del giorno in cui arriva.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayVerifierAmbiguousTraceTest,
	"RefactorTactics.Replay.Verifier.AmbiguousTraceIsReported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayVerifierAmbiguousTraceTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadDeterminismScenario(*this, TEXT("Spec.Overwatch.HoldThenFire"), Scenario)) { return false; }

	TArray<FString> Divergenze;
	const FRTTestResult A = RunAsVerifier(Scenario, {}, nullptr, Divergenze);
	if (A.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("la partita di riferimento e' fallita: %s"), *A.ErrorMessage));
		return false;
	}

	// La voce del `FIRE`, duplicata con l'esito ribaltato: se la seconda vincesse, il bersaglio non verrebbe
	// piu' troncato e lo stato finale cambierebbe. La prima deve vincere, e la seconda essere dichiarata.
	TArray<FRTTurnLogEntry> Ambigua = AllEntries(A);
	FString ChiaveDoppia;
	for (int32 I = 0; I < Ambigua.Num(); ++I)
	{
		const FRTTurnLogEntry& E = Ambigua[I];
		if (E.Category == ERTLogCategory::ReactionDecision
			&& URTReactionOpportunityLibrary::FireResponseTarget(E.ReactionResponse) != INDEX_NONE)
		{
			ChiaveDoppia = E.OpportunityId;
			FRTTurnLogEntry Gemella = E;
			// La gemella tiene la STESSA ragione e cambia la RISPOSTA: e' cio' che rende ambigua la chiave,
			// e con l'esito conflazionato non si poteva nemmeno esprimere.
			Gemella.ReactionResponse = URTReactionOpportunityLibrary::HoldResponse();
			Gemella.SelectedTargetUnitId = 0;
			Ambigua.Insert(Gemella, I + 1);
			break;
		}
	}
	if (!TestFalse(TEXT("c'era una voce da duplicare"), ChiaveDoppia.IsEmpty())) { return false; }

	const FRTTestResult B = RunAsVerifier(WithoutScriptedDecisions(Scenario), Ambigua, nullptr, Divergenze);

	TestEqual(FString::Printf(TEXT("la PRIMA vince: stesso stato della partita reale (%08x vs %08x)"),
		B.StateHash, A.StateHash), B.StateHash, A.StateHash);

	bool bAmbiguitaNominata = false;
	for (const FString& D : Divergenze)
	{
		if (D.Contains(TEXT("ambigua")) && D.Contains(ChiaveDoppia)) { bAmbiguitaNominata = true; }
	}
	TestTrue(TEXT("e l'ambiguita' e' dichiarata con la chiave che la porta"), bAmbiguitaNominata);
	if (!bAmbiguitaNominata)
	{
		for (const FString& D : Divergenze) { AddError(FString::Printf(TEXT("divergenza: %s"), *D)); }
	}

	return true;
}


/**
 * Una finestra COLLASSATA ricalcola il proprio esito e non consulta la traccia (`#886`, voce 2 — [D-109]).
 *
 * `Spec.Overwatch.ConditionCollapsesToHold` produce `HoldCollapsedByCondition` (era `HoldImmediate` fino
 * al 2026-08-23, #583): la condizione dichiarata filtra via
 * ogni bersaglio, resta il solo `HOLD`, e `RequiresDecisionBoundary` — che e' `AllowedResponses >= 2` — e'
 * falso. Quell'esito non e' la scelta di nessuno: e' la constatazione che non c'era scelta, e rileggerlo
 * dalla traccia farebbe dipendere una regola del gioco da un file.
 *
 * Il test ha DUE meta', e la seconda e' nata da una verifica di mutazione fallita: con la sola traccia
 * originale, spostare la consultazione PRIMA del gate non faceva cadere niente. La ragione era che la
 * traccia di questo scenario contiene **solo** esiti di collasso, che la costruzione scarta: la mappa restava
 * vuota, il ramo non veniva nemmeno raggiunto, e «non consulta la traccia» era vero per assenza di traccia.
 *
 *     (a) traccia ORIGINALE      -> verdetto VUOTO. Se le voci del collasso finissero nella mappa
 *                                   resterebbero non consumate — nessuna finestra le chiede — e sarebbero
 *                                   riportate come orfane su una ri-simulazione riuscita
 *     (b) traccia PERTURBATA     -> la voce del collasso diventa un `FIRE`. Se qualcuno la consultasse,
 *                                   l'esito smetterebbe di essere un collasso; e la chiave, che nessuna
 *                                   finestra reclama, dev'essere segnalata orfana
 *
 * ⚠️ Nella (b) il verdetto NON e' vuoto, ed e' corretto che non lo sia: una traccia che porta una risposta
 * per una finestra che non apre e' un disaccordo, e tacerlo sarebbe il difetto gemello.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayVerifierCollapsedWindowTest,
	"RefactorTactics.Replay.Verifier.CollapsedWindowIgnoresTheTrace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayVerifierCollapsedWindowTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadDeterminismScenario(*this, TEXT("Spec.Overwatch.ConditionCollapsesToHold"), Scenario))
	{
		return false;
	}

	TArray<FString> Divergenze;
	const FRTTestResult A = RunAsVerifier(Scenario, {}, nullptr, Divergenze);
	if (A.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("la partita di riferimento e' fallita: %s"), *A.ErrorMessage));
		return false;
	}

	const TArray<FRTTurnLogEntry> Traccia = AllEntries(A);
	// ⚠️ **`HoldCollapsedByCondition` e non piu' `HoldImmediate`** (2026-08-23, #583): i due erano lo stesso
	// valore, e sono stati separati perche' `HoldImmediate` copriva due meccanismi — il collasso da condizione
	// e la finestra che nasce con una risposta sola per costruzione. Questo test vuole il PRIMO: e' quello che
	// si ricalcola come funzione pura dello stato, e la ragione per cui la traccia non va riletta.
	const int32 Collassi = CountOutcome(Traccia, ERTReactionDecisionOutcome::CollapsedByCondition);
	if (!TestTrue(TEXT("la traccia porta almeno un collasso da condizione, o il test non verifica niente"), Collassi > 0))
	{
		return false;
	}

	// (a) traccia originale: il collasso si ricalcola, e le sue voci non lasciano residui.
	const FRTTestResult B = RunAsVerifier(Scenario, Traccia, nullptr, Divergenze);

	TestEqual(FString::Printf(TEXT("il collasso ricalcolato da' lo stesso stato (%08x vs %08x)"),
		B.StateHash, A.StateHash), B.StateHash, A.StateHash);
	TestEqual(TEXT("e sempre lo stesso numero di collassi"),
		CountOutcome(AllEntries(B), ERTReactionDecisionOutcome::CollapsedByCondition), Collassi);
	TestEqual(TEXT("nessuna divergenza: le voci del collasso non entrano nella mappa e non restano orfane"),
		Divergenze.Num(), 0);
	if (Divergenze.Num() > 0)
	{
		AddError(FString::Printf(TEXT("primo disaccordo: %s"), *Divergenze[0]));
	}

	// (b) traccia PERTURBATA: la voce del collasso diventa un `FIRE`. E' la meta' che discrimina, perche'
	// costringe la mappa a NON essere vuota — e una mappa vuota rende «non consulta la traccia» vero per
	// assenza di traccia, cioe' vacuo. Con la consultazione spostata prima del gate, questa finestra
	// smetterebbe di produrre un esito di collasso.
	TArray<FRTTurnLogEntry> Tentatrice = Traccia;
	FString ChiaveCollasso;
	for (FRTTurnLogEntry& E : Tentatrice)
	{
		if (E.Category == ERTLogCategory::ReactionDecision
			&& static_cast<ERTReactionDecisionOutcome>(E.Outcome) == ERTReactionDecisionOutcome::CollapsedByCondition)
		{
			ChiaveCollasso = E.OpportunityId;
			// ⚠️ Si falsificano ENTRAMBI gli assi (`#1118`): la ragione diventa «ha scelto» e la risposta
			// diventa un `FIRE`. Cambiare la sola ragione non basterebbe piu' a tentare il Verifier — e'
			// precisamente cio' che la separazione rende visibile.
			E.Outcome = static_cast<uint8>(ERTReactionDecisionOutcome::Chosen);
			E.ReactionResponse = URTReactionOpportunityLibrary::FireResponse(0);
			E.SelectedTargetUnitId = 0; // un bersaglio qualunque: il punto e' che la risposta NON venga letta
			break;
		}
	}
	if (!TestFalse(TEXT("c'era una voce di collasso da rendere tentatrice"), ChiaveCollasso.IsEmpty()))
	{
		return false;
	}

	const FRTTestResult C = RunAsVerifier(Scenario, Tentatrice, nullptr, Divergenze);

	TestEqual(FString::Printf(TEXT("il FIRE registrato NON viene applicato: stesso stato (%08x vs %08x)"),
		C.StateHash, A.StateHash), C.StateHash, A.StateHash);
	TestEqual(TEXT("e la finestra collassata produce ancora CollapsedByCondition"),
		CountOutcome(AllEntries(C), ERTReactionDecisionOutcome::CollapsedByCondition), Collassi);
	TestEqual(TEXT("nessun FIRE nella ri-simulazione"), CountFireResponses(AllEntries(C)), 0);

	bool bCollassoOrfano = false;
	for (const FString& D : Divergenze)
	{
		if (D.Contains(TEXT("orfana")) && D.Contains(ChiaveCollasso)) { bCollassoOrfano = true; }
	}
	TestTrue(TEXT("e la risposta che nessuna finestra ha reclamato e' segnalata"), bCollassoOrfano);

	return true;
}


/**
 * L'ORDINE dell'input non cambia l'esito.
 *
 * E' la stessa scenario con gli intent e le unita' elencati al contrario. L'invariante #3 dice che l'ordine
 * dell'array non deve cambiare il risultato: qui lo si verifica dall'esterno, sul percorso di gioco intero,
 * invece che sulla singola funzione pura. Se questo test diventasse rosso, il resolver starebbe usando
 * l'ordine di iterazione come regola di gioco.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSimulationChecksumPermutationTest,
	"RefactorTactics.Simulation.ChecksumStableAcrossPermutations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSimulationChecksumPermutationTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadDeterminismScenario(*this, TEXT("Movement.Collision"), Scenario)) { return false; }
	if (!TestTrue(TEXT("lo scenario ha almeno due unita' da permutare"), Scenario.Units.Num() >= 2))
	{
		return false;
	}

	const FRTTestResult Straight = RTWorldFixtures::RunScenarioIsolated(Scenario);
	if (Straight.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("esecuzione diretta fallita: %s"), *Straight.ErrorMessage));
		return false;
	}

	// Permutazione: unita' e intent al contrario. Nient'altro cambia — stessa mappa, stesse celle, stesse
	// aspettative — quindi ogni differenza nell'esito verrebbe dall'ordine, che e' cio' che si vuole escludere.
	FRTTestScenario Reversed = Scenario;
	Algo::Reverse(Reversed.Units);
	for (FRTScenarioTurn& Turn : Reversed.Turns)
	{
		Algo::Reverse(Turn.Intents);
	}

	const FRTTestResult Permuted = RTWorldFixtures::RunScenarioIsolated(Reversed);
	if (Permuted.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("esecuzione permutata fallita: %s"), *Permuted.ErrorMessage));
		return false;
	}

	TestEqual(TEXT("stesso esito"), Permuted.OutcomeString(), Straight.OutcomeString());
	TestEqual(FString::Printf(TEXT("stesso stato finale (%08x vs %08x)"), Permuted.StateHash, Straight.StateHash),
		Permuted.StateHash, Straight.StateHash);
	TestEqual(TEXT("stesso numero di turni"), Permuted.TurnsPlayed, Straight.TurnsPlayed);
	return true;
}

/**
 * L'hash NON e' una costante travestita da verifica.
 *
 * I due test sopra confrontano hash fra loro: passerebbero anche se `HashFinalState` restituisse sempre lo
 * stesso numero, e non se ne accorgerebbe nessuno. Qui si esegue una situazione **diversa** e si pretende un
 * hash diverso — la controprova che rende significativi gli altri due.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSimulationHashDistinguishesStatesTest,
	"RefactorTactics.Simulation.StateHashDistinguishesOutcomes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSimulationHashDistinguishesStatesTest::RunTest(const FString&)
{
	FRTTestScenario Moving;
	if (!LoadDeterminismScenario(*this, TEXT("Movement.Basic"), Moving)) { return false; }

	// Stessa scenario senza l'intento di movimento: l'unita' resta alla partenza. Stato finale diverso.
	FRTTestScenario Still = Moving;
	for (FRTScenarioTurn& Turn : Still.Turns)
	{
		Turn.Intents.Reset();
	}

	const FRTTestResult WithMove = RTWorldFixtures::RunScenarioIsolated(Moving);
	const FRTTestResult WithoutMove = RTWorldFixtures::RunScenarioIsolated(Still);

	TestNotEqual(TEXT("posizioni finali diverse -> hash diversi"), WithMove.StateHash, WithoutMove.StateHash);
	return true;
}

/**
 * I campi di contesto della v6 sono VALORIZZATI su una partita vera (#405).
 *
 * Il formato li porta dal merge di #396, ma per un po' nessuno li ha scritti: `ARTTurnManager` popolava solo
 * i sette campi preesistenti, e ogni traccia reale li portava a `0`. Un campo che nessuno produce e' peggio
 * di un campo assente — sembra una capacita' e non lo e'.
 *
 * Il test gira sul percorso di gioco REALE (Scenario Harness -> resolver -> TurnLog serializzato) e legge la
 * traccia del primo turno cosi' come finirebbe in un corpus golden. Non costruisce voci a mano: verificare i
 * campi su una voce fabbricata dal test direbbe solo che la struct ha dei membri.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogProducerStampsContextTest,
	"RefactorTactics.TurnLog.ProducerStampsContextOnRealMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogProducerStampsContextTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadDeterminismScenario(*this, TEXT("Movement.Basic"), Scenario)) { return false; }

	const FRTTestResult Result = RTWorldFixtures::RunScenarioIsolated(Scenario);
	if (Result.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("esecuzione fallita: %s"), *Result.ErrorMessage));
		return false;
	}
	if (!TestTrue(TEXT("lo scenario ha prodotto almeno una traccia"), Result.TurnTraces.Num() > 0))
	{
		return false;
	}

	TArray<FRTTurnLogEntry> Entries;
	if (!TestTrue(TEXT("la traccia del turno 1 si rilegge"),
		URTTurnLogLibrary::DeserializeTurnLog(Result.TurnTraces[0].Bytes, Entries)))
	{
		return false;
	}
	if (!TestTrue(TEXT("il turno 1 ha prodotto voci"), Entries.Num() > 0)) { return false; }

	// Il numero di turno e' un dato di CONTESTO: vale per ogni voce, comprese quelle ambientali.
	int32 WrongTurn = 0;
	for (const FRTTurnLogEntry& E : Entries)
	{
		if (E.TurnNumber != 1) { ++WrongTurn; }
	}
	TestEqual(TEXT("ogni voce del turno 1 dichiara il turno 1"), WrongTurn, 0);

	// `UnitId` su una traccia vera: chi ha AGITO, non chi era su quella cella ([D-063]). Le voci di movimento
	// e combattimento hanno sempre un attore — se dopo il wiring ne restasse una a `0`, sarebbe un sito di
	// emissione che ha ereditato lo zero invece di dichiarare la propria unita', ed e' esattamente il difetto
	// che il parametro obbligatorio di `AppendLogEntry` esiste per rendere impossibile.
	int32 ActorlessCombat = 0;
	int32 WithActor = 0;
	for (const FRTTurnLogEntry& E : Entries)
	{
		const bool bHasActor = (E.Category == ERTLogCategory::Move || E.Category == ERTLogCategory::Combat);
		if (bHasActor && E.UnitId <= 0) { ++ActorlessCombat; }
		if (E.UnitId > 0) { ++WithActor; }
	}
	TestEqual(TEXT("nessuna voce di movimento o combattimento resta senza attore"), ActorlessCombat, 0);
	TestTrue(TEXT("almeno una voce dichiara la propria unita' (altrimenti il test non prova niente)"),
		WithActor > 0);

	return true;
}

/**
 * Il SEED e' dichiarato e NON consumato — e questo test esiste per accorgersi del giorno in cui smettera'
 * di esserlo.
 *
 * PERCHE' NON E' «stesso seed -> stesso output» (#578, voce 4 di PDR-05 §10). Quella formulazione, su un
 * progetto senza RNG, confronta una funzione deterministica con se' stessa: passa sempre, anche a resolver
 * rotto. Sarebbe un test verde che non verifica niente — e un test simile non protegge, occupa solo una
 * riga della matrice.
 *
 * L'invariante che invece morde e' la NEGAZIONE: due seed DIVERSI devono dare lo stesso risultato, perche'
 * oggi nessuno legge quel campo. Misurato il 2026-08-15: zero occorrenze di `FMath::Rand`, `RandRange`,
 * `FRandomStream` e simili in tutto `Source/RefactorTactics/`, test compresi. Lo dichiarano anche
 * `RTTestScenario.h` («Seed dichiarato ma non consumato») e `RTTestResult.h`.
 *
 * COSA FARE QUANDO DIVENTA ROSSO. Non aggiustare il test: e' il segnale che un RNG e' entrato nella
 * simulazione. A quel punto il contratto di PDR-05 §5 diventa esigibile — ogni estrazione deriva il proprio
 * seed come `Hash(TurnSeed, ActionId, RollKind)`, cosi' che aggiungere un VFX casuale non sposti hit e crit
 * — e questo test va SOSTITUITO da due: «stesso seed -> stesso output» (che allora non sara' piu' vacuo) e
 * «seed diverso -> muove solo gli stream correlati». La sostituzione e' una decisione, e va scritta accanto
 * all'invariante #4 del piano canonico.
 *
 * Verifica anche che il seed sia RIPORTATO nel result: e' la premessa perche' il giorno in cui conta lo si
 * possa correlare a un esito. Un campo che non arriva al report non e' dichiarabile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSimulationSeedDeclaredUnconsumedTest,
	"RefactorTactics.Simulation.SeedIsDeclaredAndUnconsumed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSimulationSeedDeclaredUnconsumedTest::RunTest(const FString&)
{
	FRTTestScenario Base;
	if (!LoadDeterminismScenario(*this, TEXT("Movement.Collision"), Base)) { return false; }

	// Un primo, e non un multiplo: se un RNG usasse il seed come passo o come modulo, un valore «tondo»
	// potrebbe ricadere sullo stesso stream per caso e lasciare passare il difetto.
	FRTTestScenario Reseeded = Base;
	Reseeded.Seed = Base.Seed + 7919;

	const FRTTestResult A = RTWorldFixtures::RunScenarioIsolated(Base);
	const FRTTestResult B = RTWorldFixtures::RunScenarioIsolated(Reseeded);

	if (A.Outcome == ERTTestOutcome::Error || B.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("esecuzione fallita: '%s' / '%s'"), *A.ErrorMessage, *B.ErrorMessage));
		return false;
	}

	// Un hash a zero significherebbe «nessuno stato»: confrontare zeri fra loro non proverebbe nulla.
	TestNotEqual(TEXT("lo stato finale produce un hash reale"), A.StateHash, 0u);

	// Il seed arriva al report, altrimenti non e' correlabile a un esito il giorno in cui conta.
	TestEqual(TEXT("il seed dello scenario e' riportato nel result"), A.Seed, Base.Seed);
	TestEqual(TEXT("anche il seed diverso e' riportato nel result"), B.Seed, Reseeded.Seed);
	TestNotEqual(TEXT("i due scenari dichiarano davvero seed diversi"), B.Seed, A.Seed);

	if (A.StateHash != B.StateHash || A.OutcomeString() != B.OutcomeString() || A.TurnsPlayed != B.TurnsPlayed)
	{
		AddError(FString::Printf(
			TEXT("cambiare il seed (%d -> %d) ha cambiato il risultato: hash %08x/%08x, esito '%s'/'%s', turni %d/%d. ")
			TEXT("Un RNG e' entrato nella simulazione: NON aggiustare questo test — vedi il commento sopra, ")
			TEXT("serve il contratto Hash(TurnSeed, ActionId, RollKind) di PDR-05 §5 e due test al posto di questo."),
			A.Seed, B.Seed, A.StateHash, B.StateHash, *A.OutcomeString(), *B.OutcomeString(),
			A.TurnsPlayed, B.TurnsPlayed));
		return false;
	}

	return true;
}

/**
 * L'ANELLO CHE MANCAVA: si apre un ARCHIVIO SU DISCO, lo si rigioca col resolver, e si confronta — `#2196`.
 *
 * 🔴 **Il buco era dichiarato in `RTReplayProducerTests.cpp`**, nell'intestazione di
 * `TwoIdenticalMatchesLeaveIdenticalArchives`: *«`Replay.Verifier.ReportsFirstDivergence` esiste […] ma il
 * suo stesso commento dichiara il confine: "Chi produce la seconda ri-simulando e' il chiamante". **Nessuno
 * era quel chiamante.** E il corpus golden non copre questo: le sue referenze sono `.rttl` COMMITTATI, non
 * archivi PRODOTTI da una corsa»*. Da qui in poi qualcuno lo e'.
 *
 * 🔑 **Le altre due gambe restano, e questa non le sostituisce.**
 *   · `Replay.Verifier.ResimulationIsDeterministic` — scenario → resolver → confronto di hash, in memoria;
 *   · `Replay.Producer.TwoIdenticalMatchesLeaveIdenticalArchives` — due archivi **entrambi prodotti** nella
 *     stessa run.
 * Nessuna delle due riapre un artefatto **conservato** per chiedergli se le regole lo riproducono ancora.
 *
 * ⚠️ **La fixture e' `Spec.Overwatch.HoldThenFire`, e la scelta e' stata MISURATA, non preferita.** La prima
 * stesura di questo test prendeva l'archivio da una partita 2v2 bot-contro-bot: quell'allestimento non apre
 * **nessuna** finestra di reazione, quindi `ArmRecordedReactionDecisions` costruiva una mappa VUOTA,
 * `AskReactionDecision` tornava al decisore vivo, e la «ri-simulazione» era una terza partita decisa dal
 * vivo — byte identici perche' deterministica, test verde, anello mai percorso. Il difetto e' stato preso
 * dalla guardia `DecisioniArmate > 0` qui sotto, che l'ha reso rosso: `«e ne ha 0»`. *Trovato da una code
 * review, e misurato.*
 *
 * 🔑 **Percio' la guardia viene PRIMA del confronto**: un test che confronta byte senza aver verificato di
 * aver consumato la traccia misura il determinismo, non il replay — e il determinismo ha gia' due test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayArchiveResimulationTest,
	"RefactorTactics.Replay.Verifier.ArchiveReplaysThroughTheResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayArchiveResimulationTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadDeterminismScenario(*this, TEXT("Spec.Overwatch.HoldThenFire"), Scenario)) { return false; }

	const FString RadiceRif = RadiceArchivio(TEXT("Rif"));
	const FString RadiceRig = RadiceArchivio(TEXT("Rig"));
	PulisciRadice(RadiceRif);
	PulisciRadice(RadiceRig);

	// 1. La corsa di riferimento: decide dal vivo (lo scenario porta le proprie risposte) e si ARCHIVIA.
	TArray<FString> Divergenze;
	FRTArchivioSuDisco Archivio;
	const FRTTestResult Rif = RunAsVerifier(Scenario, {}, nullptr, Divergenze, &RadiceRif, &Archivio);
	if (Rif.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("la corsa di riferimento e' fallita: %s"), *Rif.ErrorMessage));
		PulisciRadice(RadiceRif); PulisciRadice(RadiceRig);
		return false;
	}
	if (!TestTrue(TEXT("l'archivio di riferimento e' completo su disco"), Archivio.bCompleto))
	{
		PulisciRadice(RadiceRif); PulisciRadice(RadiceRig);
		return false;
	}

	// 2. 🔴 LA GUARDIA: la traccia porta decisioni ARMABILI. Senza, tutto il resto misura un'altra cosa.
	const TArray<FRTTurnLogEntry> Traccia = AllEntries(Rif);
	int32 Decisioni = 0, Fuochi = 0;
	CountDecisionEntries(Traccia, Decisioni, Fuochi);
	if (!TestTrue(FString::Printf(TEXT("l'archivio porta decisioni da rigiocare, e ne ha %d"), Decisioni),
		Decisioni > 0))
	{
		PulisciRadice(RadiceRif); PulisciRadice(RadiceRig);
		return false;
	}
	// ⚠️ **Il numero si STAMPA anche quando passa.** `TestTrue` parla solo fallendo, e un verde muto su
	// questa riga e' precisamente cio' che ha lasciato passare la prima stesura: l'anello non percorso era
	// indistinguibile da quello percorso. Ora il referto porta la misura, non la sua assenza.
	AddInfo(FString::Printf(TEXT("l'archivio porta %d decisioni (%d FIRE) su %d turni: l'anello e' percorso"),
		Decisioni, Fuochi, Archivio.Byte.Num()));

	// 3. La stessa corsa, rigiocata dal resolver consumando le decisioni dell'ARCHIVIO — non dello scenario:
	//    `WithoutScriptedDecisions` toglie le risposte scritte, quindi se la traccia non fosse consumata non
	//    risponderebbe nessuno e la divergenza sarebbe immediata.
	const FRTTestScenario Muto = WithoutScriptedDecisions(Scenario);
	TArray<FString> DivergenzeRig;
	FRTArchivioSuDisco Rigiocata;
	const FRTTestResult Rig = RunAsVerifier(Muto, Traccia, nullptr, DivergenzeRig, &RadiceRig, &Rigiocata);
	if (Rig.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("la ri-simulazione e' fallita: %s"), *Rig.ErrorMessage));
		PulisciRadice(RadiceRif); PulisciRadice(RadiceRig);
		return false;
	}

	TestTrue(TEXT("anche l'archivio della ri-simulazione e' completo"), Rigiocata.bCompleto);

	// 4. Il Verifier non protesta: nessuna finestra scoperta dalla traccia, nessuna risposta diventata
	//    illegale. Un archivio che si rigioca «identico» mentre il Verifier ha da ridire e' identico per caso.
	TestEqual(FString::Printf(TEXT("nessun disaccordo del Verifier (%s)"),
		DivergenzeRig.Num() > 0 ? *DivergenzeRig[0] : TEXT("nessuno")), DivergenzeRig.Num(), 0);

	// 5. E i due archivi coincidono sui BYTE, turno per turno.
	const int32 TurnoDiverso = PrimoTurnoDiverso(Archivio, Rigiocata);
	if (TurnoDiverso != INDEX_NONE)
	{
		AddError(FString::Printf(TEXT("la ri-simulazione diverge dall'archivio al turno %d: %s"),
			TurnoDiverso + 1,
			*URTTurnLogLibrary::DescribeFirstDivergence(
				TurnoDiverso + 1, Archivio.Voci[TurnoDiverso], Rigiocata.Voci[TurnoDiverso])));
	}
	TestEqual(TEXT("e i due archivi hanno lo stesso numero di turni"),
		Rigiocata.Byte.Num(), Archivio.Byte.Num());

	PulisciRadice(RadiceRif);
	PulisciRadice(RadiceRig);
	return true;
}


/**
 * **Il checksum per boundary dice DOVE, e sa fallire a meta' corsa** (`#2189`).
 *
 * 🔴 **L'anti-vacuita' e' nello stesso test, ed e' il criterio della issue**: un gate che sa solo dire
 * «uguali» non misura niente. Qui la stessa traccia viene confrontata con una sua copia **alterata a un
 * boundary intermedio**, e il gate deve nominare quel boundary — non l'ultimo, non «la partita».
 *
 * ⚠️ **La divergenza si inietta in un turno di mezzo, non nell'ultimo.** E' la differenza fra questo gate
 * e `FinalStateHash`: quello vedrebbe comunque un finale diverso, e non saprebbe dire da dove. Iniettarla
 * all'ultimo turno renderebbe i due indistinguibili, cioe' misurerebbe una proprieta' che gia' esisteva.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBoundaryChecksumNamesTheDivergenceTest,
	"RefactorTactics.Replay.Verifier.BoundaryChecksumNamesWhereTheyDiverge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBoundaryChecksumNamesTheDivergenceTest::RunTest(const FString&)
{
	// --- una traccia con quattro boundary in tre turni ------------------------------------------------
	auto Voce = [](int32 Turno, ERTMatchPhase Fase, int32 UnitId, const FRTCellId& Da, const FRTCellId& A)
	{
		FRTTurnLogEntry E;
		E.TurnNumber = Turno;
		E.Phase = Fase;
		E.Category = ERTLogCategory::Move;
		E.UnitId = UnitId;
		E.SrcCell = Da;
		E.TgtCell = A;
		E.ActionId = FName(TEXT("Action.Move"));
		return E;
	};

	TArray<FRTTurnLogEntry> Traccia;
	Traccia.Add(Voce(1, ERTMatchPhase::Move, 1, FRTCellId(0, 0), FRTCellId(1, 0)));
	Traccia.Add(Voce(2, ERTMatchPhase::Move, 1, FRTCellId(1, 0), FRTCellId(2, 0)));
	Traccia.Add(Voce(3, ERTMatchPhase::Move, 1, FRTCellId(2, 0), FRTCellId(3, 0)));

	TArray<FRTTracedUnitState> Iniziale;
	{
		FRTTracedUnitState S;
		S.UnitId = 1;
		S.Cell = FRTCellId(0, 0);
		Iniziale.Add(S);
	}

	const TArray<FRTBoundaryChecksum> A =
		URTBoundaryChecksumLibrary::ChecksumsAlongTrace(nullptr, Traccia, Iniziale);

	// ⛔ ANTI-VACUITA' 1: senza piu' di un boundary il test non potrebbe distinguere «a meta'» da «alla
	// fine», che e' la proprieta' che esiste per misurare.
	if (!TestTrue(TEXT("la traccia attraversa piu' di un boundary"), A.Num() >= 3)) { return false; }

	// ⛔ ANTI-VACUITA' 2: i checksum non devono essere tutti uguali, o il confronto sarebbe cieco.
	TestNotEqual(TEXT("boundary diversi hanno checksum diversi"), A[0].Hash, A[1].Hash);

	// --- il verso VERDE: la stessa traccia con se' stessa ---------------------------------------------
	const TArray<FRTBoundaryChecksum> Uguale =
		URTBoundaryChecksumLibrary::ChecksumsAlongTrace(nullptr, Traccia, Iniziale);

	TestEqual(TEXT("due esecuzioni identiche non divergono"),
		URTBoundaryChecksumLibrary::FirstDivergence(A, Uguale), INDEX_NONE);
	TestEqual(TEXT("e il messaggio e' vuoto"),
		URTBoundaryChecksumLibrary::DescribeDivergence(A, Uguale), FString());

	// --- 🔴 il verso ROSSO: una divergenza al SECONDO boundary di tre -------------------------------
	TArray<FRTTurnLogEntry> Alterata = Traccia;
	Alterata[1].TgtCell = FRTCellId(9, 9); // il turno 2 finisce altrove

	const TArray<FRTBoundaryChecksum> B =
		URTBoundaryChecksumLibrary::ChecksumsAlongTrace(nullptr, Alterata, Iniziale);

	const int32 Dove = URTBoundaryChecksumLibrary::FirstDivergence(A, B);

	// ⚠️ **Il primo boundary resta uguale**: e' il fatto che rende utile questa misura. Un gate che
	// segnalasse la divergenza dall'inizio non aiuterebbe a cercare.
	TestEqual(TEXT("il primo boundary NON diverge"), A[0].Hash, B[0].Hash);
	TestEqual(TEXT("la divergenza e' al secondo boundary, non all'ultimo"), Dove, 1);

	const FString Messaggio = URTBoundaryChecksumLibrary::DescribeDivergence(A, B);
	TestTrue(TEXT("il messaggio nomina il boundary"), Messaggio.Contains(A[1].ToString()));
	TestTrue(TEXT("e non e' vuoto"), !Messaggio.IsEmpty());

	// --- lunghezze diverse: si dice, invece di confrontare fin dove entrambe arrivano -----------------
	TArray<FRTBoundaryChecksum> Corta = A;
	Corta.Pop();
	TestEqual(TEXT("una sequenza piu' corta diverge dove finisce"),
		URTBoundaryChecksumLibrary::FirstDivergence(A, Corta), Corta.Num());
	TestTrue(TEXT("e il messaggio lo dice"),
		URTBoundaryChecksumLibrary::DescribeDivergence(A, Corta).Contains(TEXT("numero diverso")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
