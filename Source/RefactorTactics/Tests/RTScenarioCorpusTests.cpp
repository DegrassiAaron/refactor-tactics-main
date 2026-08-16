// Il CORPUS come test: ogni scenario versionato viene eseguito, non solo caricato.
//
// Prima di questo file uno scenario diventava un test solo se qualcuno gli scriveva accanto la propria
// `IMPLEMENT_SIMPLE_AUTOMATION_TEST`. Chi ne aggiungeva uno senza ricordarsene otteneva un file che sembrava
// coperto — `ShippedScenariosAreValid` lo carica, quindi il verde c'e' — mentre nessuno lo eseguiva mai.
// E' successo: diciassette scenari sono stati committati cosi'.
//
// Qui il corpus si scopre da solo. Aggiungere un file JSON basta perche' venga eseguito, e questo test e'
// l'unico posto da cambiare quando cambia la regola su cosa e' un esito accettabile.

#include "Misc/AutomationTest.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTScenarioSession.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	UWorld* MakeCorpusWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyCorpusWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Carica ed esegue uno scenario per ID. Mondo creato e distrutto qui: uno scenario non ne eredita da un altro. */
	bool RunCorpusScenario(FAutomationTestBase& Test, const FString& ScenarioId, FRTTestResult& OutResult)
	{
		FString ResolveError;
		const FString Path = URTScenarioIndex::ResolvePath(ScenarioId, ResolveError);
		if (Path.IsEmpty())
		{
			Test.AddError(FString::Printf(TEXT("%s: l'indice non lo risolve — %s"), *ScenarioId, *ResolveError));
			return false;
		}

		FRTTestScenario Scenario;
		FString LoadError;
		if (!URTScenarioLoader::LoadFromFile(Path, Scenario, LoadError))
		{
			Test.AddError(FString::Printf(TEXT("%s: non si carica — %s"), *ScenarioId, *LoadError));
			return false;
		}

		UWorld* World = MakeCorpusWorld();
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: mondo"), *ScenarioId), World))
		{
			return false;
		}
		OutResult = URTScenarioRunner::Run(World, Scenario);
		DestroyCorpusWorld(World);
		return true;
	}
}

/**
 * OGNI scenario versionato deve **girare**, e finire in uno dei due esiti che significano «il gioco sta bene»:
 *
 * - `PASS`    — ha giocato tutti i turni e le assertion tengono;
 * - `BLOCKED` — si e' fermato su una capability che non esiste ancora, e l'ha nominata.
 *
 * `BLOCKED` e' accettabile per costruzione: e' il meccanismo che permette di versionare uno scenario **prima**
 * dei suoi sistemi. Se lo trattassimo come rosso, l'unica strategia razionale sarebbe non scrivere piu' scenari
 * in anticipo — e si perderebbe il solo modo che il progetto ha di dichiarare una feature futura in forma
 * eseguibile.
 *
 * `FAIL` e' un difetto del GIOCO. `ERROR` e' un difetto dello SCENARIO. La distinzione e' gia' nel tipo di
 * esito, e qui viene riportata nel messaggio invece di essere appiattita su «non passa».
 *
 * Gli scenari con tag `expected-fail` sono esclusi: il loro mestiere e' fallire, e li verifica il test qui sotto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioCorpusRunsTest,
	"RefactorTactics.Scenario.EveryShippedScenarioRuns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioCorpusRunsTest::RunTest(const FString&)
{
	const TArray<FString> AllIds = URTScenarioIndex::ListIds(FString(), FString());
	const TArray<FString> ExpectedToFail = URTScenarioIndex::ListIds(TEXT("expected-fail"), FString());

	// Un corpus vuoto farebbe passare questo test senza eseguire niente: e' il modo piu' silenzioso in cui una
	// rete di sicurezza puo' smettere di esistere. La soglia e' bassa apposta — dice «l'indice funziona», non
	// «il corpus e' abbastanza grande».
	if (!TestTrue(TEXT("l'indice trova gli scenari"), AllIds.Num() >= 10))
	{
		AddError(FString::Printf(TEXT("l'indice ha restituito %d scenari: o il corpus e' sparito, o ListIds non elenca piu'"), AllIds.Num()));
		return false;
	}

	int32 Passed = 0;
	int32 Blocked = 0;

	for (const FString& Id : AllIds)
	{
		if (ExpectedToFail.Contains(Id))
		{
			continue;
		}

		FRTTestResult Result;
		if (!RunCorpusScenario(*this, Id, Result))
		{
			continue; // errore gia' riportato con il motivo
		}

		switch (Result.Outcome)
		{
		case ERTTestOutcome::Pass:
			++Passed;
			break;

		case ERTTestOutcome::Blocked:
			// Non e' un fallimento, ma non deve nemmeno passare in silenzio: il motivo compare nel log, cosi'
			// chi implementa la capability sa gia' quali scenari si accendono quando atterra.
			++Blocked;
			AddInfo(FString::Printf(TEXT("%s: BLOCKED — %s"), *Id, *Result.BlockedReason));
			break;

		case ERTTestOutcome::Fail:
			{
				// Il primo assert caduto, col valore reale: senza, si sa che qualcosa e' rosso ma non cosa,
				// e si finisce a rieseguire lo scenario a mano per leggere un dato che il runner aveva gia'.
				FString First = TEXT("(nessuna assertion registrata)");
				for (const FRTAssertionResult& A : Result.Assertions)
				{
					if (!A.bPassed)
					{
						First = FString::Printf(TEXT("%s — atteso %s, ottenuto %s (turno %d)"),
							*A.Description, *A.Expected, *A.Actual, A.Turn);
						break;
					}
				}
				AddError(FString::Printf(TEXT("%s: FAIL (difetto del GIOCO) — %s"), *Id, *First));
			}
			break;

		case ERTTestOutcome::Error:
		default:
			AddError(FString::Printf(TEXT("%s: ERROR (difetto dello SCENARIO) — %s"), *Id, *Result.ErrorMessage));
			break;
		}
	}

	AddInfo(FString::Printf(TEXT("corpus eseguito: %d PASS, %d BLOCKED, %d dichiarati expected-fail"),
		Passed, Blocked, ExpectedToFail.Num()));
	return true;
}

/**
 * Uno scenario `expected-fail` deve **fallire davvero**.
 *
 * Il tag esiste per dimostrare che il report diagnostica un fallimento; se il gioco cambiasse in modo da farlo
 * passare, il tag diventerebbe una bugia e — peggio — l'unica prova che l'harness sa dire «rosso» smetterebbe
 * di provarlo, senza che nulla diventi rosso. E' il caso in cui il verde e' il difetto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioExpectedFailTest,
	"RefactorTactics.Scenario.ExpectedFailScenariosReallyFail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioExpectedFailTest::RunTest(const FString&)
{
	const TArray<FString> ExpectedToFail = URTScenarioIndex::ListIds(TEXT("expected-fail"), FString());
	if (!TestTrue(TEXT("esiste almeno uno scenario expected-fail"), ExpectedToFail.Num() > 0))
	{
		return false;
	}

	for (const FString& Id : ExpectedToFail)
	{
		FRTTestResult Result;
		if (!RunCorpusScenario(*this, Id, Result))
		{
			continue;
		}

		// FAIL, non ERROR: deve fallire perche' il GIOCO non fa quel che lo scenario si aspetta, non perche'
		// lo scenario sia rotto. Un `expected-fail` che va in ERROR ha smesso di dimostrare quel che doveva.
		TestEqual(FString::Printf(TEXT("%s: dichiarato expected-fail, quindi FAIL"), *Id),
			Result.OutcomeString(), FString(TEXT("FAIL")));
	}
	return true;
}

// =====================================================================================================
// `#601`/`#602` — lo scenario della reazione dichiarata deve PASSARE, non solo «non fallire».
//
// `EveryShippedScenarioRuns` accetta `BLOCKED` per costruzione, ed e' giusto: e' il meccanismo che permette
// di versionare uno scenario prima della sua capability. Ma per uno scenario che oggi **gira davvero**
// quell'accettazione e' troppo larga: se un domani `ReactionPlanning` tornasse indisponibile — o il
// produttore di `PlannedReactionAbility` sparisse — il file scivolerebbe in `BLOCKED` e la suite resterebbe
// verde, senza che nessuno sappia che il giocatore ha smesso di poter armare una reazione.
//
// Qui l'esito atteso e' pinnato: `Pass`. E' la stessa disciplina dei test che pinnano un limite, con il
// segno opposto — quelli diventano rossi quando il limite cade, questo quando una capacita' si perde.
// =====================================================================================================
namespace
{
	/**
	 * Uno scenario ANCORATO: deve passare, e in particolare **non** deve scivolare in `BLOCKED`.
	 *
	 * Estratto in helper il 2026-08-13, quando le ancore sono diventate tre: la forma era gia' stata scritta
	 * due volte identica, e la terza copia sarebbe stata quella che smette di essere aggiornata.
	 *
	 * @param WhatWouldBeLost cosa e' sparito, se lo scenario e' `BLOCKED`. Va nel messaggio d'errore perche'
	 *        «e' bloccato» non dice a chi legge dove guardare, e le due cause — produttore sparito o elenco
	 *        delle capability regredito — stanno in file diversi.
	 */
	bool AnchorScenarioMustPass(FAutomationTestBase& Test, const FString& Id, const TCHAR* WhatWouldBeLost)
	{
		FRTTestResult Result;
		if (!RunCorpusScenario(Test, Id, Result))
		{
			return false; // motivo gia' riportato
		}

		if (Result.Outcome == ERTTestOutcome::Blocked)
		{
			Test.AddError(FString::Printf(
				TEXT("%s e' BLOCKED (%s): la capability c'era quando lo scenario e' stato scritto. ")
				TEXT("O %s e' sparito, o l'elenco delle capability e' regredito."),
				*Id, *Result.BlockedReason, WhatWouldBeLost));
			return false;
		}

		if (Result.Outcome != ERTTestOutcome::Pass)
		{
			FString First = TEXT("(nessuna assertion registrata)");
			for (const FRTAssertionResult& A : Result.Assertions)
			{
				if (!A.bPassed)
				{
					First = FString::Printf(TEXT("%s: atteso %s, osservato %s"),
						*A.Description, *A.Expected, *A.Actual);
					break;
				}
			}
			Test.AddError(FString::Printf(TEXT("%s non passa: %s"), *Id, *First));
			return false;
		}

		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioAnchorRunsTest,
	"RefactorTactics.Scenario.DeclaredReactionScenarioPasses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioAnchorRunsTest::RunTest(const FString&)
{
	return AnchorScenarioMustPass(*this, TEXT("Spec.Reaction.AnchorCancelsPush"),
		TEXT("il produttore di `PlannedReactionAbility`"));
}

// =====================================================================================================
// `#291`/`#737` — la stessa ancora per la ROTAZIONE dichiarata.
//
// Aggiunta il 2026-08-13, e la ragione e' stata **misurata invece che supposta**: togliendo
// `DeclaredRotation` dall'elenco delle capability, i due scenari passano a `BLOCKED` e l'intera suite resta
// **verde**. `EveryShippedScenarioRuns` accetta `BLOCKED` per costruzione — giustamente, perche' e' il
// meccanismo che permette di versionare uno scenario prima della sua capability — ma per uno scenario che
// oggi gira davvero quell'accettazione lascia scoperta proprio la regressione che conta: il giocatore smette
// di poter dichiarare una rotazione e nessuno se ne accorge.
//
// E' lo stesso buco che `#601` aveva chiuso per la reazione. Qui si chiude per il facing, ed e' il momento
// giusto: la capability e' entrata **oggi**, quindi l'ancora nasce insieme a cio' che protegge.
// =====================================================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioRotationAnchorTest,
	"RefactorTactics.Scenario.DeclaredRotationScenariosPass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioRotationAnchorTest::RunTest(const FString&)
{
	// Tutti e due, e non uno solo: il rifiuto e la sua controprova si reggono a vicenda. Uno scenario che
	// dimostra un rifiuto passerebbe anche con un resolver che rifiuta tutto, e la meta' che lo esclude e'
	// proprio quella da fermo.
	const bool bRejected = AnchorScenarioMustPass(*this,
		TEXT("Spec.Facing.IllegalDeclaredRotationIsRejected"),
		TEXT("il produttore di `PlannedFacing` (`ARTPlayerController::HandleFacingSector`)"));
	const bool bApplied = AnchorScenarioMustPass(*this,
		TEXT("Spec.Facing.StationaryDeclaredRotationApplies"),
		TEXT("il produttore di `PlannedFacing` (`ARTPlayerController::HandleFacingSector`)"));

	// `&` e non `&&`: si vogliono entrambi gli esiti riportati, non il primo che cade.
	return bRejected & bApplied;
}

// =====================================================================================================
// `#582` — uno scenario che chiede cio' che non c'e' resta BLOCKED, anche se e' il PRIMO turno a mancare.
//
// Il meccanismo `BLOCKED` esiste per versionare uno scenario prima della sua capability. Aveva pero' un buco
// che lo annullava proprio nel caso piu' comune per uno scenario nuovo: se e' il **primo** turno a chiedere
// la capability, la partita completa zero turni, l'assertion `TurnsCompleted >= 1` cade, e la precedenza
// degli esiti (`FAIL > BLOCKED`) trasforma l'attesa in un «difetto del GIOCO».
//
// Gli scenari `BLOCKED` gia' in repo non lo mostravano: hanno tutti il `requires` sul secondo turno, quindi
// il primo gira e il conteggio e' soddisfatto. Una salvezza accidentale — e infatti il difetto e' emerso
// scrivendo il primo scenario a turno singolo.
//
// Il test costruisce lo scenario da stringa invece di versionarlo: un file che chiede una capability
// inventata resterebbe nel corpus per sempre come rumore, e `ShippedScenariosAreTagged` dovrebbe farci
// spazio. Qui il caso vive dentro il test che lo verifica.
// =====================================================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioUnknownCapabilityIsErrorTest,
	"RefactorTactics.Scenario.UnknownCapabilityIsErrorNotBlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioUnknownCapabilityIsErrorTest::RunTest(const FString&)
{
	// TRE casi, e servono tutti e tre. Con il solo primo un gate che rifiuta *qualunque* capability non
	// disponibile passerebbe: bloccherebbe anche i dodici scenari legittimamente in attesa, e il verde direbbe
	// il contrario di cio' che e' successo. Il terzo pinna l'ORDINE delle due passate, che e' la parte che il
	// repository ha gia' pagato una volta — vedi il caso `Facing` piu' sotto.
	auto Esegui = [this](const TArray<FString>& Capabilities) -> FRTTestResult
	{
		FString Elenco;
		for (const FString& C : Capabilities)
		{
			if (!Elenco.IsEmpty()) { Elenco += TEXT(", "); }
			Elenco += FString::Printf(TEXT("\"%s\""), *C);
		}

		const FString Json = FString::Printf(TEXT(R"JSON(
		{
		  "scenarioId": "Spec.Harness.CapabilityProbe",
		  "tags": ["spec", "harness"],
		  "version": 1, "seed": 0, "mapRadius": 4,
		  "units": [
		    { "id": "A1", "hero": "Hero.Vektor", "team": 0, "cell": [-1, 0, 0] },
		    { "id": "B1", "hero": "Hero.Riva",   "team": 1, "cell": [1, 0, 0] }
		  ],
		  "turns": [ { "requires": [%s], "intents": [] } ],
		  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
		}
		)JSON"), *Elenco);

		FRTTestScenario Scenario;
		FString LoadError;
		if (!URTScenarioLoader::LoadFromString(Json, Scenario, LoadError))
		{
			AddError(FString::Printf(TEXT("scenario di prova malformato: %s"), *LoadError));
			return FRTTestResult();
		}
		UWorld* World = MakeCorpusWorld();
		const FRTTestResult R = URTScenarioRunner::Run(World, Scenario);
		DestroyCorpusWorld(World);
		return R;
	};

	// (1) Un nome che nessuno ha mai dichiarato e' un REFUSO, ed e' un difetto del TEST: `Error`.
	// Prima di questo test produceva un `Blocked` identico a quello di un'attesa legittima, quindi uno
	// scenario con un refuso restava bloccato per sempre senza che nulla lo segnalasse.
	const FRTTestResult Refuso = Esegui({TEXT("DecisionBoundry")});
	TestTrue(FString::Printf(TEXT("capability sconosciuta => Error, non %s"), *Refuso.OutcomeString()),
		Refuso.Outcome == ERTTestOutcome::Error);
	TestTrue(TEXT("e il messaggio nomina il refuso, perche' chi legge deve poterlo correggere"),
		Refuso.ErrorMessage.Contains(TEXT("DecisionBoundry")));

	// (2) Un nome NOTO ma non ancora disponibile resta un'attesa legittima: `Blocked`. E' il regime degli
	// scenari in repo, e non deve cambiare.
	//
	// 🔴 **Il soggetto e' cambiato il 2026-08-16 (`#512` fase B), e senza sostituirlo questo caso sarebbe
	// diventato verde per il motivo sbagliato.** Fino a oggi era `DecisionBoundary`: scoprendola, il turno
	// gira, l'esito e' `Pass` e `TestTrue(... == Blocked)` cade. Non e' un aggiustamento — e' il difetto
	// strutturale di usare come **esempio** un nome che si spera diventi disponibile: l'esempio scade
	// insieme all'attesa che rappresenta.
	// 🔴 **E il primo sostituto era `ReactionProfile`, che riarmava la stessa scadenza.** Quel nome atterra
	// con E14.7 (`#314`), e da quel giorno questo caso tornerebbe `Pass` esattamente come oggi. La cura sta
	// scritta novanta righe piu' sotto, nel test gemello, e vale parola per parola anche qui: *«La cura NON
	// e' sostituirlo con una capability vera tipo `DecisionBoundary`: quella un giorno atterra, e da quel
	// giorno questo test non parlerebbe piu' del meccanismo»*.
	// ✅ Il sostituto e' quindi `NeverAvailable`: nome **riservato**, dichiarato fra i noti-non-disponibili
	// con il vincolo scritto che non diventera' mai disponibile. E' l'unico che conserva la proprieta' che
	// serve — noto E non disponibile, per sempre — invece di prenderla in prestito da una feature futura.
	const FRTTestResult Attesa = Esegui({TEXT("NeverAvailable")});
	TestTrue(FString::Printf(TEXT("capability nota non disponibile => Blocked, non %s"), *Attesa.OutcomeString()),
		Attesa.Outcome == ERTTestOutcome::Blocked);

	// (3) Un refuso ACCANTO a un'attesa legittima resta un `Error`, e l'ordine dei due nomi non lo salva.
	//
	// E' il caso che il repository ha gia' pagato: `RT_Showcase_Relay_v01` e `Spec.Overwatch.HoldThenFire`
	// chiedevano `["DecisionBoundary", "Facing"]`, e `Facing` aveva smesso di essere un nome di capability con
	// `#291`. Chiedendo prima la disponibilita', il refuso si nasconde dietro il `Blocked` del primo nome e
	// nessuno lo vede — che e' precisamente cio' che e' successo, per mesi, in due file.
	//
	// Senza questo caso, invertire le due passate in `BeginTurn()` non farebbe cadere NIENTE: i casi (1) e (2)
	// hanno un solo nome per turno e non distinguono l'ordine.
	// ⬅️ Anche qui il primo nome e' passato da `DecisionBoundary` a `NeverAvailable` con `#512` fase B, e per
	// la stessa ragione del caso (2): serve un'attesa VERA davanti al refuso, altrimenti il caso non pinna
	// piu' l'ordine delle due passate — con una capability disponibile davanti, il turno non si bloccherebbe
	// affatto e il refuso verrebbe visto comunque. Il nome riservato e' l'unico che non riarma la scadenza.
	const FRTTestResult Misto = Esegui({TEXT("NeverAvailable"), TEXT("DecisionBoundry")});
	TestTrue(FString::Printf(TEXT("refuso accanto a un'attesa => Error, non %s"), *Misto.OutcomeString()),
		Misto.Outcome == ERTTestOutcome::Error);
	TestTrue(TEXT("e il messaggio nomina il refuso, non l'attesa che gli stava davanti"),
		Misto.ErrorMessage.Contains(TEXT("DecisionBoundry")));
	return true;
}

/**
 * Ogni `requires` di ogni scenario versionato nomina una capability che ESISTE — disponibile o no.
 *
 * ⚠️ Non e' il doppione del test qui sopra, e la differenza e' il motivo per cui questo file ne ha due.
 * Quello prova che il RUNNER sa distinguere un refuso da un'attesa; questo guarda il CORPUS, e lo guarda
 * **senza eseguirlo**. La distinzione conta perche' l'esecuzione ha un punto cieco preciso: il runner si
 * ferma al primo turno bloccato, quindi un refuso in un turno successivo non viene mai raggiunto.
 *
 * Non e' un'ipotesi. `RT_Showcase_Relay_v01` chiedeva `Facing` al turno 4 — `["DecisionBoundary", "Facing"]` —
 * e `Facing` aveva smesso di essere un nome di capability con `#291`. Nessuno se n'e' accorto: il turno era
 * gia' `Blocked` per il primo dei due nomi, e `EveryShippedScenarioRuns` vedeva un `BLOCKED` regolare.
 *
 * Un nome nuovo in uno scenario diventa quindi rosso QUI, subito, invece di aspettare che il gioco arrivi a
 * quel turno — cosa che per un refuso non succede mai.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioKnownCapabilitiesTest,
	"RefactorTactics.Scenario.ShippedScenariosRequireKnownCapabilities",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioKnownCapabilitiesTest::RunTest(const FString&)
{
	const TArray<FString> AllIds = URTScenarioIndex::ListIds(FString(), FString());
	if (!TestTrue(TEXT("l'indice trova gli scenari"), AllIds.Num() >= 10))
	{
		AddError(FString::Printf(TEXT("l'indice ha restituito %d scenari: il controllo non guarderebbe nulla"), AllIds.Num()));
		return false;
	}

	// Gli `expected-fail` NON si escludono: il loro mestiere e' fallire sulle assertion, non essere scritti male.
	int32 Controllate = 0;
	for (const FString& Id : AllIds)
	{
		FString ResolveError;
		const FString Path = URTScenarioIndex::ResolvePath(Id, ResolveError);
		if (Path.IsEmpty())
		{
			AddError(FString::Printf(TEXT("%s: l'indice non lo risolve — %s"), *Id, *ResolveError));
			continue;
		}

		FRTTestScenario Scenario;
		FString LoadError;
		if (!URTScenarioLoader::LoadFromFile(Path, Scenario, LoadError))
		{
			AddError(FString::Printf(TEXT("%s: non si carica — %s"), *Id, *LoadError));
			continue;
		}

		for (int32 T = 0; T < Scenario.Turns.Num(); ++T)
		{
			for (const FString& Required : Scenario.Turns[T].Requires)
			{
				++Controllate;
				if (!FRTScenarioSession::IsKnownCapability(Required))
				{
					AddError(FString::Printf(
						TEXT("%s turno %d: la capability '%s' non esiste in nessuno dei due elenchi di ")
						TEXT("`RTScenarioSession.cpp`. O e' un refuso, o e' un nome nuovo da dichiarare la' ")
						TEXT("prima di usarlo qui."),
						*Id, T + 1, *Required));
				}
			}
		}
	}

	// Il conteggio e' l'oracolo: se un domani `Requires` smettesse di essere popolato dal loader, il ciclo
	// girerebbe a vuoto e questo test resterebbe verde senza aver guardato niente.
	AddInfo(FString::Printf(TEXT("capability controllate: %d su %d scenari"), Controllate, AllIds.Num()));
	TestTrue(TEXT("gli scenari in repo dichiarano almeno una capability: se e' zero, il loader non le carica piu'"),
		Controllate > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBlockedBeatsFinalAssertionsTest,
	"RefactorTactics.Scenario.BlockedFirstTurnStaysBlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBlockedBeatsFinalAssertionsTest::RunTest(const FString&)
{
	// ⚠️ `NeverAvailable`: nome **riservato**, dichiarato fra i noti-non-disponibili con il vincolo scritto che
	// non diventera' mai disponibile. Il test chiedeva `CapabilityCheNonEsistera Mai` — inventato di proposito,
	// perche' una capability VERA che un domani atterra farebbe misurare a questo test un'altra cosa. Da quando
	// un nome sconosciuto vale `Error` (`UnknownCapabilityIsErrorNotBlocked`) quel veicolo non prova piu' il
	// meccanismo `BLOCKED`: lo aggira, e il test cadeva sul proprio assert.
	//
	// La cura NON e' sostituirlo con una capability vera tipo `DecisionBoundary`: quella un giorno atterra, e
	// da quel giorno questo test non parlerebbe piu' del meccanismo. Il nome riservato conserva la proprieta'
	// che serve — non atterra mai — pagando l'unica cosa che il vecchio veicolo non poteva piu' dare: essere
	// riconosciuto come nome esistente.
	const FString Json = TEXT(R"JSON(
	{
	  "scenarioId": "Spec.Harness.BlockedProbe",
	  "tags": ["spec", "harness"],
	  "version": 1,
	  "seed": 0,
	  "mapRadius": 4,
	  "units": [
	    { "id": "A1", "hero": "Hero.Vektor", "team": 0, "cell": [-1, 0, 0] },
	    { "id": "B1", "hero": "Hero.Riva",   "team": 1, "cell": [1, 0, 0] }
	  ],
	  "turns": [
	    {
	      "requires": ["NeverAvailable"],
	      "intents": []
	    }
	  ],
	  "expect": [
	    { "type": "TurnsCompleted", "value": 1 },
	    { "type": "UnitAlive", "unit": "A1", "value": true }
	  ]
	}
	)JSON");

	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("lo scenario di prova e' ben formato"),
		URTScenarioLoader::LoadFromString(Json, Scenario, Error)))
	{
		AddError(FString::Printf(TEXT("motivo del rifiuto: %s"), *Error));
		return false;
	}

	UWorld* World = MakeCorpusWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyCorpusWorld(World);

	// Il cuore: l'esito e' BLOCKED, non FAIL. La differenza non e' cosmetica — un report che dice «difetto del
	// GIOCO» per uno scenario in attesa manda qualcuno a cercare un bug che non esiste.
	if (!TestTrue(FString::Printf(TEXT("esito BLOCKED e non %s"), *Result.OutcomeString()),
		Result.Outcome == ERTTestOutcome::Blocked))
	{
		return false;
	}

	TestTrue(TEXT("e il motivo nomina la capability mancante"),
		Result.BlockedReason.Contains(TEXT("NeverAvailable")));

	// E le assertion finali non sono state valutate: misurerebbero una partita che non e' stata giocata.
	TestEqual(TEXT("nessuna assertion finale valutata su un turno mai giocato"), Result.Assertions.Num(), 0);
	return true;
}

// =====================================================================================================
// `#512` fase B — `Spec.Overwatch.HoldThenFire` non e' piu' una specifica in attesa: si esegue.
//
// Perche' un test DEDICATO quando `EveryShippedScenarioRuns` gia' esegue tutto il corpus: quello dice
// «PASS/BLOCKED/FAIL» e, sul rosso, la PRIMA assertion caduta. Non dice quante finestre si sono aperte, ne'
// quante decisioni sono state applicate — e sono esattamente i due numeri che distinguono «lo scenario
// passa» da «lo scenario passa PER IL MOTIVO GIUSTO». Con due decisioni dichiarate, un turno che aprisse una
// sola finestra e ne consumasse una lascerebbe le assertion di cella e HP soddisfatte da un'altra strada.
//
// Usa `RunById` e non `Run`: e' il percorso che scrive `result.json`, cioe' cio' che si legge quando questo
// test diventa rosso senza che nessuno sappia perche'.
// =====================================================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioHoldThenFireTest,
	"RefactorTactics.Scenario.OverwatchHoldThenFireConsumesBothDecisions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioHoldThenFireTest::RunTest(const FString&)
{
	UWorld* World = MakeCorpusWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	FString ReportDir;
	const FRTTestResult Result = URTScenarioRunner::RunById(World, TEXT("Spec.Overwatch.HoldThenFire"), ReportDir);
	DestroyCorpusWorld(World);

	// Il primo assert caduto col valore reale, come fa il corpus: senza, un rosso qui manda a rieseguire a
	// mano uno scenario che il runner ha gia' eseguito.
	if (Result.Outcome != ERTTestOutcome::Pass)
	{
		for (const FRTAssertionResult& A : Result.Assertions)
		{
			if (!A.bPassed)
			{
				AddError(FString::Printf(TEXT("%s — atteso %s, ottenuto %s (turno %d)"),
					*A.Description, *A.Expected, *A.Actual, A.Turn));
			}
		}
		for (const FString& N : Result.Notes)
		{
			AddError(FString::Printf(TEXT("nota: %s"), *N));
		}
		if (!Result.ErrorMessage.IsEmpty())
		{
			AddError(FString::Printf(TEXT("errore: %s"), *Result.ErrorMessage));
		}
		// ⚠️ **`BlockedReason` e' il campo che spiega il fallimento PIU' probabile di questo test, e le tre
		// righe qui sopra non lo stampano.** Se qualcuno rimette una `requires` non disponibile sullo
		// scenario — o una capability torna fuori da `AvailableCapabilities()` — l'esito e' `Blocked` al
		// turno 1: `Assertions` e' popolato solo per i turni giocati, `Notes` ed `ErrorMessage` restano
		// vuoti, e il blocco sopra non emetterebbe **niente**. Resterebbe «esito PASS e non BLOCKED», che e'
		// precisamente il rimando a rieseguire a mano che questo test dichiara di voler evitare.
		if (!Result.BlockedReason.IsEmpty())
		{
			AddError(FString::Printf(TEXT("bloccato: %s"), *Result.BlockedReason));
		}
	}

	TestEqual(FString::Printf(TEXT("esito PASS e non %s"), *Result.OutcomeString()),
		static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass));
	TestEqual(TEXT("due turni giocati"), Result.TurnsPlayed, 2);

	// I due numeri che rendono il verde non ambiguo: ENTRAMBE le decisioni consumate, NESSUN residuo. Un
	// residuo > 0 significa una finestra scoperta, e senza questa riga passerebbe dentro un esito verde.
	TestEqual(TEXT("entrambe le decisioni applicate"), Result.ScriptedDecisionsApplied, 2);
	TestEqual(TEXT("nessuna decisione rimasta inutilizzata"), Result.ScriptedDecisionsUnused, 0);

	// ⚠️ **Il docblock dice «usa `RunById` perche' scrive `result.json`»: senza questa riga quella ragione
	// non e' verificata da niente.** Se `URTTestReportWriter::Write` fallisce — cartella non creabile, disco
	// pieno — `RunById` torna un `OutReportDirectory` vuoto e il test resta VERDE mentre l'artefatto
	// diagnostico su cui si appoggia non esiste. E' la stessa guardia che `ShowcaseRelayV01RunsTurnOne` ha
	// da sempre, e che qui mancava.
	TestFalse(TEXT("il report ha una cartella"), ReportDir.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
