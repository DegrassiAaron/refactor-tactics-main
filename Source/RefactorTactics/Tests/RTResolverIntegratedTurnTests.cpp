// L'oracolo causale dello scenario integrato `Spec.Resolver.IntegratedTurn` (`#3031` CHECKPOINT G e H).
//
// 🔴 **Perche' un test C++ accanto allo scenario, e non solo le sue `expect`.** Le assertion dell'harness
// sono dieci tipi (`UnitAtCell`, `LogEventCount`, `LogEventAmount`, `LogEventOrder`, …) e **nessuno** legge
// il `MicroStepIndex` di una voce. Senza quel campo lo scenario e' ambiguo su cio' che prova: due decisioni
// sono compatibili sia con «una finestra al passo 0 e una al passo 1» sia con «una al passo 0, e una al
// passo 1 che conteneva DUE bersagli» — e nel secondo caso il terzo nemico passa indisturbato perche' ha
// condiviso la finestra del secondo, non perche' la carica fosse spesa. Sono due mondi diversi con le stesse
// nove `expect` verdi.
//
// ⚠️ **Misurato, e va detto perche' e' la ragione di questo file**: le voci `Move` escono con
// `MicroStepIndex = -1`. Non e' un difetto — `CurrentMicroStepIndex` vive solo dentro il ciclo dei
// micro-step (`RTTurnManager_Movement.cpp`) e le voci `Move` sono riassunti di TURNO, scritti dopo — ma la
// conseguenza e' che il TurnLog **non puo' dire a quale micro-step un'unita' sia entrata in una cella. ∴ il
// micro-step d'ingresso non e' asseribile direttamente, e va raggiunto per controfattuale.
//
// ⛔ **Il controfattuale non e' un extra: e' l'unica forma in cui la proprieta' e' misurabile qui.** E' anche
// cio' che sostituisce un test rimosso in code review su questa stessa PR: `ChargeIsPerWatcherNotPerTarget`
// scriveva a mano `FRTOverwatchWatcher::bArmed = false`, che non e' la carica. La carica e'
// `FRTArmedOverwatch::bCharged`, e vive solo in una partita vera — cioe' qui.

#include "Misc/AutomationTest.h"

#include "Map/RTCellId.h"
#include "RTWorldFixtures.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTTestResult.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "Turn/RTReactionOpportunityTypes.h" // FireResponseTarget: «e' un FIRE?» si chiede alla RISPOSTA
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"

namespace
{
	/** Nomi distinti per file: la unity build condivide la translation unit. */
	bool LoadIntegratedScenario(FAutomationTestBase& Test, FRTTestScenario& Out)
	{
		FString Error;
		const FString Path = URTScenarioIndex::ResolvePath(TEXT("Spec.Resolver.IntegratedTurn"), Error);
		if (Path.IsEmpty() || !URTScenarioLoader::LoadFromFile(Path, Out, Error))
		{
			Test.AddError(FString::Printf(TEXT("Spec.Resolver.IntegratedTurn non caricabile: %s"), *Error));
			return false;
		}
		return true;
	}

	/** Le voci di tutti i turni, rilette dalle tracce serializzate. */
	TArray<FRTTurnLogEntry> IntegratedEntries(const FRTTestResult& Result)
	{
		TArray<FRTTurnLogEntry> All;
		for (const FRTTurnTrace& Trace : Result.TurnTraces)
		{
			TArray<FRTTurnLogEntry> Entries;
			if (URTTurnLogLibrary::DeserializeTurnLog(Trace.Bytes, Entries)) { All.Append(Entries); }
		}
		return All;
	}

	/**
	 * L'id di SCENARIO di un'unita' — `M2`, `W1` — invece del suo `UnitId` numerico.
	 *
	 * ⛔ Gli id numerici NON seguono l'ordine di dichiarazione: misurato su questo scenario, `W1` dichiarato
	 * per primo esce `6`. Cablarli renderebbe il test fragile a un cambio d'assegnazione che non e' una
	 * regola di gioco.
	 */
	FString ScenarioIdOf(const FRTTestResult& Result, int32 UnitId)
	{
		const FString* Found = Result.ScenarioIdByUnitId.Find(UnitId);
		return Found ? *Found : FString::Printf(TEXT("<unit %d>"), UnitId);
	}

	bool IsFire(const FRTTurnLogEntry& E)
	{
		return URTReactionOpportunityLibrary::FireResponseTarget(E.ReactionResponse) != INDEX_NONE;
	}

	TArray<FRTTurnLogEntry> OfCategory(const TArray<FRTTurnLogEntry>& All, ERTLogCategory Cat)
	{
		TArray<FRTTurnLogEntry> Out;
		for (const FRTTurnLogEntry& E : All) { if (E.Category == Cat) { Out.Add(E); } }
		return Out;
	}

	int32 CountMoveOutcome(const TArray<FRTTurnLogEntry>& All, ERTMoveOutcome Outcome)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : All)
		{
			if (E.Category == ERTLogCategory::Move && E.Outcome == static_cast<uint8>(Outcome)) { ++N; }
		}
		return N;
	}
}

/**
 * Lo scenario integrato SPIEGA il proprio esito: ogni voce che conta porta i campi causali, e il terzo
 * nemico passa perche' la carica e' spesa — non perche' abbia condiviso la finestra del secondo.
 *
 * ## Le due meta'
 *
 * **(1) La struttura causale della corsa diretta.** Due finestre e non tre; la prima e' un `HOLD` al
 * micro-step 0, la seconda un `FIRE` al micro-step 1 con danno, sorgente, bersaglio, cella e
 * `OpportunityId`; il bersaglio del `FIRE` esce con `StoppedByOverwatch` e non con «cella occupata»; e la
 * coppia estranea esce con `BlockedContested` su **entrambe** le unita', che e' cio' che «nessun vantaggio
 * a team o ordine» significa in un reason code.
 *
 * **(2) Il CONTROFATTUALE, che e' dove la proprieta' si misura.** La stessa partita in cui il `FIRE`
 * diventa un `HOLD`: la carica non si spende, e allora il terzo nemico DEVE trovare la propria finestra. Se
 * la trova — e la trova al micro-step **2** — allora sono provate insieme due cose che la corsa diretta
 * lascia ambigue:
 *
 *   · le durate d'arco ([D-381], qui dal `moveCost` delle celle) scaglionano davvero i tre ingressi su tre
 *     micro-step DISTINTI, invece di farli entrare insieme in una finestra sola a piu' bersagli;
 *   · ∴ nella corsa diretta e' il `FIRE` — cioe' la carica spesa — a negare la finestra al terzo.
 *
 * 🔑 **E la carica in gioco e' quella VERA**: `FRTArmedOverwatch::bCharged`, consumata da
 * `ApplyReactionDecision`. Nessun flag scritto a mano da questo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolverIntegratedTurnIsCausallyExplainedTest,
	"RefactorTactics.Resolver.IntegratedTurnIsCausallyExplained",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolverIntegratedTurnIsCausallyExplainedTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadIntegratedScenario(*this, Scenario)) { return false; }

	const FRTTestResult Diretta = RTWorldFixtures::RunScenarioIsolated(Scenario);
	if (Diretta.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("esecuzione fallita: %s"), *Diretta.ErrorMessage));
		return false;
	}
	TestEqual(TEXT("lo scenario gira per un turno"), Diretta.TurnsPlayed, 1);

	const TArray<FRTTurnLogEntry> Voci = IntegratedEntries(Diretta);
	if (!TestTrue(TEXT("la traccia non e' vuota"), Voci.Num() > 0)) { return false; }

	// --- 1) Due finestre, e i loro micro-step ---------------------------------------------------------
	const TArray<FRTTurnLogEntry> Decisioni = OfCategory(Voci, ERTLogCategory::ReactionDecision);
	if (!TestEqual(TEXT("due finestre e non tre: sulla terza la carica e' spesa"), Decisioni.Num(), 2))
	{
		return false;
	}

	const FRTTurnLogEntry* Hold = nullptr;
	const FRTTurnLogEntry* Fire = nullptr;
	for (const FRTTurnLogEntry& E : Decisioni)
	{
		if (IsFire(E)) { Fire = &E; } else { Hold = &E; }
	}
	if (!TestNotNull(TEXT("una finestra e' stata lasciata cadere (HOLD)"), (void*)Hold)) { return false; }
	if (!TestNotNull(TEXT("e una ha sparato (FIRE)"), (void*)Fire)) { return false; }

	TestEqual(TEXT("l'HOLD cade al micro-step 0: e' il primo ingresso nella linea"),
		Hold->MicroStepIndex, 0);
	TestEqual(TEXT("il FIRE cade al micro-step 1: il secondo ingresso, con la carica ancora intatta"),
		Fire->MicroStepIndex, 1);

	// --- 2) I campi CAUSALI del colpo (CHECKPOINT G) ---------------------------------------------------
	TestEqual(TEXT("sorgente del colpo: il watcher"), ScenarioIdOf(Diretta, Fire->UnitId), FString(TEXT("W1")));
	TestEqual(TEXT("bersaglio del colpo: il secondo nemico"),
		ScenarioIdOf(Diretta, Fire->SelectedTargetUnitId), FString(TEXT("M2")));
	TestTrue(TEXT("la cella del colpo e' quella in cui il bersaglio e' entrato"),
		Fire->TgtCell == FRTCellId(2, 0, 0));
	TestTrue(TEXT("il colpo dichiara un danno, e non zero"), Fire->Amount > 0);
	TestFalse(TEXT("e nomina la finestra che lo ha causato"), Fire->OpportunityId.IsEmpty());
	TestEqual(TEXT("l'azione e' l'Overwatch"), Fire->ActionId, FName(TEXT("Action.Overwatch")));

	// --- 3) Il reason code del troncamento, e non «cella occupata» ------------------------------------
	TestEqual(TEXT("una sola unita' e' stata fermata da un overwatch"),
		CountMoveOutcome(Voci, ERTMoveOutcome::StoppedByOverwatch), 1);
	for (const FRTTurnLogEntry& E : Voci)
	{
		if (E.Category == ERTLogCategory::Move
			&& E.Outcome == static_cast<uint8>(ERTMoveOutcome::StoppedByOverwatch))
		{
			TestEqual(TEXT("ed e' il bersaglio del FIRE"), ScenarioIdOf(Diretta, E.UnitId), FString(TEXT("M2")));
			TestTrue(TEXT("fermata nella cella raggiunta, non in quella pianificata"),
				E.TgtCell == FRTCellId(2, 0, 0));
		}
	}

	// --- 4) La contesa: lo STESSO reason code su entrambe --------------------------------------------
	//
	// ⛔ Due unita' dello stesso conflitto con due reason code diversi direbbero che una ha perso contro
	// l'altra: e' la forma che [D-383] esclude, e qui la contesa e' a parita' di priorita'.
	TestEqual(TEXT("la cella contesa ferma DUE unita', con lo stesso reason code"),
		CountMoveOutcome(Voci, ERTMoveOutcome::BlockedContested), 2);
	TestEqual(TEXT("e nessuna ha perso per priorita': non c'e' un vincitore arbitrario"),
		CountMoveOutcome(Voci, ERTMoveOutcome::BlockedByPriority), 0);

	// --- 5) IL CONTROFATTUALE: la carica non si spende ------------------------------------------------
	FRTTestScenario SenzaFuoco = Scenario;
	int32 Convertite = 0;
	for (FRTScenarioTurn& Turn : SenzaFuoco.Turns)
	{
		for (FRTScenarioDecision& D : Turn.Decisions)
		{
			if (D.Respond.Equals(TEXT("FIRE"), ESearchCase::IgnoreCase))
			{
				D.Respond = URTReactionOpportunityLibrary::HoldResponse();
				D.Target.Empty();
				++Convertite;
			}
		}
	}
	if (!TestEqual(TEXT("il controfattuale ha una risposta da convertire"), Convertite, 1)) { return false; }

	// 🔑 **E serve una TERZA decisione, per il terzo nemico** — che e' gia' metà della prova. L'harness
	// rifiuta una finestra senza risposta (*«finestra aperta per 'W1' senza una decisione che la nomini»*),
	// quindi il solo fatto che il controfattuale ne pretenda una terza dice che la terza finestra si apre.
	// Qui la si dichiara per poterne LEGGERE il micro-step, invece di fermarsi a quell'errore.
	if (!TestTrue(TEXT("il controfattuale ha un turno da estendere"), SenzaFuoco.Turns.Num() > 0))
	{
		return false;
	}
	{
		FRTScenarioDecision PerIlTerzo;
		PerIlTerzo.On.Reactor = TEXT("W1");
		PerIlTerzo.On.Reaction = FName(TEXT("Action.Overwatch"));
		PerIlTerzo.On.TriggerUnit = TEXT("M3");
		PerIlTerzo.bHasSelector = true;
		// ⚠️ `Unit` va riempito ANCHE con la forma a selettore: il loader fa `Decision.Unit =
		// Decision.On.Reactor` perche' *«in memoria il reactor resta un campo solo»*, e il validatore guarda
		// quello. Misurato: senza questa riga lo scenario e' rifiutato con «unita' '' non schierata».
		PerIlTerzo.Unit = PerIlTerzo.On.Reactor;
		PerIlTerzo.Respond = URTReactionOpportunityLibrary::HoldResponse();
		SenzaFuoco.Turns[0].Decisions.Add(PerIlTerzo);
	}

	// Le `expect` dello scenario descrivono la corsa CON il fuoco: «due decisioni» e «M2 fermo in (2,0,0)»
	// cadono per costruzione quando il fuoco non c'e'. Di questa corsa si legge il solo TurnLog.
	//
	// ⛔ **Svuotarle NON e' un'opzione, e il tentativo e' stato misurato**: il loader rifiuta uno scenario
	// senza assertion — *«nessuna assertion dichiarata (campo expect): lo scenario passerebbe sempre»*. E' la
	// guardia giusta, quindi si TIENE l'unica aspettativa che resta vera senza il fuoco invece di aggirarla.
	SenzaFuoco.Expect.RemoveAll([](const FRTTestExpectation& X)
	{
		return X.Kind != ERTAssertionKind::TurnsCompleted;
	});
	if (!TestEqual(TEXT("al controfattuale resta l'aspettativa sul numero di turni"),
		SenzaFuoco.Expect.Num(), 1))
	{
		return false;
	}

	const FRTTestResult Controfattuale = RTWorldFixtures::RunScenarioIsolated(SenzaFuoco);
	if (Controfattuale.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("controfattuale fallito: %s"), *Controfattuale.ErrorMessage));
		return false;
	}

	const TArray<FRTTurnLogEntry> VociCf = IntegratedEntries(Controfattuale);
	const TArray<FRTTurnLogEntry> DecisioniCf = OfCategory(VociCf, ERTLogCategory::ReactionDecision);

	// 🔴 TRE finestre, non due: il terzo nemico trova la sua quando la carica e' intatta.
	if (TestEqual(TEXT("a carica intatta le finestre sono TRE: anche il terzo nemico ne trova una"),
		DecisioniCf.Num(), 3))
	{
		// E su tre micro-step DISTINTI — 0, 1, 2 — che e' la prova che le durate d'arco scaglionano gli
		// ingressi invece di farli cadere tutti nello stesso passo.
		TSet<int32> Passi;
		for (const FRTTurnLogEntry& E : DecisioniCf) { Passi.Add(E.MicroStepIndex); }
		TestEqual(TEXT("le tre finestre cadono su tre micro-step distinti"), Passi.Num(), 3);
		TestTrue(TEXT("e sono i micro-step 0, 1 e 2: le ETA scaglionano i tre ingressi"),
			Passi.Contains(0) && Passi.Contains(1) && Passi.Contains(2));
	}

	// E senza fuoco nessuno e' stato troncato: la corsa diretta deve il proprio `StoppedByOverwatch` al FIRE.
	TestEqual(TEXT("senza fuoco nessuna unita' e' fermata da un overwatch"),
		CountMoveOutcome(VociCf, ERTMoveOutcome::StoppedByOverwatch), 0);

	return true;
}
