// Loader degli scenari di test: interpretazione e VALIDAZIONE.
//
// La validazione e' la meta' che conta. Uno scenario che nomina un eroe inesistente deve produrre un ERROR
// leggibile, non un FAIL di gioco: confondere «il test e' scritto male» con «il gioco e' rotto» fa perdere
// piu' tempo di quanto il test ne faccia risparmiare.

#include "Misc/AutomationTest.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioSession.h" // le due domande del vocabolario: noto e disponibile
#include "Turn/RTTurnRules.h" // #1515: il vincolo sulle squadre si CHIEDE a chi decide il vincitore
#include "Perception/RTTeamKnowledge.h" // la bitmask di percezione: il candidato SBAGLIATO, e il test lo dice
#include "Turn/RTTurnLog.h" // gli esiti che le assertion sul log nominano per nome
#include "Turn/RTReactionOpportunityTypes.h" // TargetHealthAtOrBelowPercent: l'id della condizione sta nel gioco
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	const TCHAR* ScenarioLoaderValidJson = TEXT(R"JSON(
	{
	  "scenarioId": "Movement.Probe",
	  "version": 1,
	  "mapRadius": 3,
	  "units": [
	    { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-2, 0, 0] },
	    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [2, 0] }
	  ],
	  "turns": [ { "intents": [ { "unit": "A1", "move": [[-1, 0, 0]] } ] } ],
	  "expect": [ { "type": "UnitAtCell", "unit": "A1", "cell": [-1, 0, 0] } ]
	}
	)JSON");

	// Le decisioni di finestra come DATO (CP 15.3 meta' B, #512). Nome distinto, come sopra.
	//
	// ⚠️ Gli id eroe sono i LEGACY, e non e' una svista: `RTHeroCatalogLibrary.cpp` dichiara oggi solo
	// `Hero.Gadget`, `Hero.Phase`, `Hero.Riktor`, `Hero.Wraith`, e i nomi di [D-130] — Gadget, Phase, Riktor,
	// Wraith — hanno ZERO occorrenze in tutto `Source/`, perche' la fetta 3 (`#753`) non e' stata eseguita.
	// Un `Hero.Riktor` qui non risolverebbe. Si rinominano insieme al catalogo, non prima.
	const TCHAR* ScenarioLoaderDecisionsJson = TEXT(R"JSON(
	{
	  "scenarioId": "Spec.Decisions.Parse",
	  "version": 2,
	  "mapRadius": 3,
	  "units": [
	    { "id": "A1", "hero": "Hero.Riktor", "team": 0, "cell": [-2, 0, 0] },
	    { "id": "B1", "hero": "Hero.Wraith",  "team": 1, "cell": [ 2, 0, 0] }
	  ],
	  "turns": [ {
	    "intents": [],
	    "decisions": [
	      { "unit": "A1", "respond": "FIRE", "target": "B1" },
	      { "unit": "A1", "respond": "HOLD" }
	    ]
	  } ],
	  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
	}
	)JSON");
}

/** Il caso felice: uno scenario ben formato produce i dati attesi, layer opzionale incluso. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderValidTest,
	"RefactorTactics.Scenario.LoaderAcceptsValidScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderValidTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("scenario valido accettato"), URTScenarioLoader::LoadFromString(ScenarioLoaderValidJson, Scenario, Error)))
	{
		AddError(FString::Printf(TEXT("motivo del rifiuto: %s"), *Error));
		return false;
	}

	TestEqual(TEXT("scenarioId"), Scenario.ScenarioId, TEXT("Movement.Probe"));
	TestEqual(TEXT("due unita' schierate"), Scenario.Units.Num(), 2);
	TestEqual(TEXT("un turno"), Scenario.Turns.Num(), 1);
	TestEqual(TEXT("una assertion"), Scenario.Expect.Num(), 1);

	const FRTScenarioUnit* A1 = Scenario.FindUnit(TEXT("A1"));
	if (!TestNotNull(TEXT("A1 trovata per id"), A1)) { return false; }
	TestEqual(TEXT("A1 e' Gadget"), A1->HeroId, FName(TEXT("Hero.Gadget")));
	TestEqual(TEXT("A1 parte da (-2,0,0)"), A1->Cell, FRTCellId(-2, 0, 0));

	// Il layer e' opzionale: `[2, 0]` deve valere `[2, 0, 0]`, non essere rifiutato.
	const FRTScenarioUnit* B1 = Scenario.FindUnit(TEXT("B1"));
	if (!TestNotNull(TEXT("B1 trovata"), B1)) { return false; }
	TestEqual(TEXT("cella a due componenti -> layer 0"), B1->Cell, FRTCellId(2, 0, 0));

	// Il seed non dichiarato resta 0: il campo esiste ma oggi NON viene consumato (nessun RNG nel progetto).
	TestEqual(TEXT("seed assente -> 0"), Scenario.Seed, 0);
	return true;
}

/** Ogni rifiuto deve dire COSA non va: un errore muto costringe a indovinare. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderRejectsTest,
	"RefactorTactics.Scenario.LoaderRejectsWithClearReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderRejectsTest::RunTest(const FString&)
{
	auto Rejects = [this](const TCHAR* Json, const TCHAR* MustMention, const TCHAR* What)
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = URTScenarioLoader::LoadFromString(Json, Scenario, Error);
		TestFalse(FString::Printf(TEXT("%s: rifiutato"), What), bOk);
		TestTrue(FString::Printf(TEXT("%s: il motivo cita '%s' (era: '%s')"), What, MustMention, *Error),
			Error.Contains(MustMention));
	};

	Rejects(TEXT("{ non e' json }"), TEXT("JSON"), TEXT("testo non interpretabile"));

	// Il caso che il documento di specifica sbagliava per primo: eroi inventati (Drift, Vex, Nyx).
	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Drift","team":0,"cell":[0,0,0]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("eroe sconosciuto"), TEXT("eroe inesistente"));

	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":2,"units":[{"id":"A","hero":"Hero.Gadget","team":0,"cell":[9,0,0]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("fuori dall'arena"), TEXT("cella fuori mappa"));

	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Gadget","team":0,"cell":[0,0,0]},{"id":"A","hero":"Hero.Phase","team":1,"cell":[1,0,0]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("duplicato"), TEXT("id unita' duplicato"));

	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Gadget","team":0,"cell":[0,0,0]},{"id":"B","hero":"Hero.Phase","team":1,"cell":[0,0,0]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("stessa cella"), TEXT("due unita' sovrapposte alla partenza"));

	// #1515 — una squadra che la partita non conosce. Non e' pedanteria di formato: `GetTeamScore` risponde
	// `0` a ogni indice diverso da 0 e 1, e l'hash di stato si costruisce da quei due soltanto, quindi uno
	// scenario con `"team": 7` girerebbe producendo un esito che nessuno puo' riprodurre.
	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Gadget","team":7,"cell":[0,0,0]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("squadra 7"), TEXT("squadra fuori dall'intervallo ammesso"));

	// Anche il verso negativo: `TeamId` e' un `int32` libero, e `-1` non e' meno rotto di `7`.
	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Gadget","team":-1,"cell":[0,0,0]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("squadra -1"), TEXT("squadra negativa"));

	// Uno scenario senza assertion passerebbe sempre: e' un test che non testa.
	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Gadget","team":0,"cell":[0,0,0]}]})"),
		TEXT("nessuna assertion"), TEXT("scenario senza expect"));

	// Un'assertion scritta male non deve essere IGNORATA: il test sembrerebbe passare.
	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Gadget","team":0,"cell":[0,0,0]}],"expect":[{"type":"UnitHasSuperpowers"}]})"),
		TEXT("assertion sconosciuta"), TEXT("assertion non riconosciuta"));

	// Intent su un'unita' mai schierata: errore di scrittura tipico, va colto subito.
	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Gadget","team":0,"cell":[0,0,0]}],"turns":[{"intents":[{"unit":"Z","move":[[1,0,0]]}]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("non schierata"), TEXT("intent su unita' inesistente"));

	// Un formato piu' nuovo di quanto il loader sappia leggere non va interpretato a caso.
	Rejects(TEXT(R"({"scenarioId":"X","version":99,"mapRadius":3,"units":[{"id":"A","hero":"Hero.Gadget","team":0,"cell":[0,0,0]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("non supportato"), TEXT("versione di formato futura"));

	// Chiave di INTENT sconosciuta (CP 16.1). Prima veniva ignorata in silenzio, e uno scenario che chiedeva
	// qualcosa che l'harness non sa fare girava verde verificando tutto tranne quello. Vale anche per un refuso
	// su una chiave vera: `dashCell` al posto di `dashTo` e' l'errore che questo controllo coglie per primo.
	//
	// 🔴 **La fixture era `"facing":"NE"`, ed e' stata cambiata il 2026-08-13 perche' ha smesso di essere
	// inventata.** Era stata scelta proprio perche' `facing` era l'esempio canonico di «cosa che l'harness non
	// sa fare» — il commento del loader lo dice ancora: *«uno scenario che chiedeva un orientamento
	// dichiarato, per dire»*. Con #291/#737 la chiave esiste, quindi quel JSON ora si CARICA e il test
	// falliva chiedendo un rifiuto che non doveva piu' arrivare.
	//
	// Il test non aveva torto: la sua fixture dipendeva dall'ASSENZA di una feature, e nessun gate lo
	// segnala. Sostituita con `dashCell`, il refuso che il commento qui sopra nomina da sempre — che ha il
	// pregio di essere un errore realistico invece di una chiave impossibile, e nessuno ha in programma di
	// implementarlo.
	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Gadget","team":0,"cell":[0,0,0]}],"turns":[{"intents":[{"unit":"A","dashCell":[1,0,0]}]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("chiave sconosciuta"), TEXT("chiave di intent inventata"));

	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Riktor","team":0,"cell":[0,0,0]}],"turns":[{"intents":[{"unit":"A","dash":"Riktor.Ram","dashCell":[1,0,0]}]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("chiave sconosciuta"), TEXT("refuso su una chiave vera"));

	// Una direzione inventata in UnitFacing non deve diventare «guarda a est» per arrotondamento.
	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Gadget","team":0,"cell":[0,0,0]}],"expect":[{"type":"UnitFacing","unit":"A","value":"NNE"}]})"),
		TEXT("direzione 'NNE' sconosciuta"), TEXT("direzione di facing inventata"));

	return true;
}

/**
 * OGNI scenario versionato in `Scenarios/` deve essere valido. E' la rete che impedisce di committare
 * uno scenario rotto: senza, il difetto si scopre solo quando qualcuno prova a eseguirlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderShippedScenariosTest,
	"RefactorTactics.Scenario.ShippedScenariosAreValid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderShippedScenariosTest::RunTest(const FString&)
{
	const FString Root = URTScenarioLoader::ScenariosRoot();
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *Root, TEXT("*.json"), /*Files=*/ true, /*Directories=*/ false);

	if (!TestTrue(FString::Printf(TEXT("esiste almeno uno scenario in %s"), *Root), Files.Num() > 0))
	{
		return false;
	}

	for (const FString& File : Files)
	{
		// I file `_*.json` non sono scenari (la tabella di redirect vive li' accanto): stessa convenzione
		// che usa la scansione dell'indice, e va ripetuta qui o questo test fallirebbe sulla tabella.
		if (FPaths::GetCleanFilename(File).StartsWith(TEXT("_")))
		{
			continue;
		}

		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = URTScenarioLoader::LoadFromFile(File, Scenario, Error);
		TestTrue(FString::Printf(TEXT("scenario valido: %s"), *FPaths::GetCleanFilename(File)), bOk);
		if (!bOk)
		{
			AddError(FString::Printf(TEXT("%s -> %s"), *FPaths::GetCleanFilename(File), *Error));
			continue;
		}

		// T1 — L'ID deve riportare a QUESTO file passando per l'indice: altrimenti `rt.Test.Run <id>` non
		// troverebbe il file. Da quando le cartelle sono libere l'invariante non e' piu' «l'id e' il
		// percorso» ma «l'id risolve al percorso», e questa e' piu' forte: fallisce anche quando due file
		// dichiarano lo stesso ID, caso che il confronto con il percorso non poteva vedere.
		FString ResolveError;
		const FString Resolved = URTScenarioIndex::ResolvePath(Scenario.ScenarioId, ResolveError);
		if (!TestFalse(FString::Printf(TEXT("%s: l'indice lo risolve"), *Scenario.ScenarioId), Resolved.IsEmpty()))
		{
			AddError(FString::Printf(TEXT("%s -> %s"), *Scenario.ScenarioId, *ResolveError));
			continue;
		}
		TestEqual(FString::Printf(TEXT("%s: l'id risolve a questo file"), *Scenario.ScenarioId),
			Resolved, FPaths::ConvertRelativePathToFull(File));
	}
	return true;
}


/**
 * Le assertion sul TurnLog (#318): l'evento si nomina per NOME, e un nome sbagliato e' un ERRORE di scenario.
 *
 * Cio' che questo test sorveglia non e' il parsing in se' — sono venti righe — ma che i nomi si risolvano per
 * RIFLESSIONE sull'enum invece che da una tabella scritta a mano. Con la tabella, aggiungere un esito al
 * TurnLog lo renderebbe inscrivibile negli scenari finche' qualcuno non si ricorda di aggiornarla anche qui,
 * e quel qualcuno non se ne ricorda: e' gia' successo in questo loader con la chiave `edge`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderLogAssertionsTest,
	"RefactorTactics.Scenario.LoaderReadsLogAssertionsByName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderLogAssertionsTest::RunTest(const FString&)
{
	auto Load = [](const TCHAR* ExpectJson, FRTTestScenario& Out, FString& Error)
	{
		const FString Json = FString::Printf(TEXT(R"JSON(
		{
		  "scenarioId": "Test.LogAssertions",
		  "mapRadius": 3,
		  "units": [
		    { "id": "A1", "hero": "Hero.Gadget",    "team": 0, "cell": [-1, 0, 0] },
		    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [1, 0, 0] }
		  ],
		  "turns": [ { "intents": [] } ],
		  "expect": [ %s ]
		})JSON"), ExpectJson);
		return URTScenarioLoader::LoadFromString(Json, Out, Error);
	};

	// 1. Conteggio: nomi validi -> categoria ed esito risolti nei valori dell'enum.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(TEXT(R"({ "type": "LogEventCount", "category": "Environment", "outcome": "BridgeRemoved", "value": 0 })"),
			Scenario, Error);
		if (TestTrue(FString::Printf(TEXT("scenario valido (%s)"), *Error), bOk)
			&& TestEqual(TEXT("una assertion"), Scenario.Expect.Num(), 1))
		{
			TestTrue(TEXT("kind LogEventCount"), Scenario.Expect[0].Kind == ERTAssertionKind::LogEventCount);
			TestTrue(TEXT("categoria Environment"), Scenario.Expect[0].LogCategory == ERTLogCategory::Environment);
			TestEqual(TEXT("esito BridgeRemoved"),
				static_cast<int32>(Scenario.Expect[0].LogOutcome),
				static_cast<int32>(ERTEnvironmentOutcome::BridgeRemoved));
			TestEqual(TEXT("value 0 e' un conteggio legittimo: asserisce l'ASSENZA"), Scenario.Expect[0].Value, 0);
		}
	}

	// 2. `value` assente = 1, cioe' «l'evento c'e'». E' il caso piu' comune e scriverlo ogni volta sarebbe rumore.
	{
		FRTTestScenario Scenario;
		FString Error;
		if (Load(TEXT(R"({ "type": "LogEventCount", "category": "Facing", "outcome": "DerivedFromMove" })"), Scenario, Error)
			&& TestEqual(TEXT("una assertion"), Scenario.Expect.Num(), 1))
		{
			TestEqual(TEXT("conteggio atteso implicito"), Scenario.Expect[0].Value, 1);
		}
	}

	// 3. Ordine: due eventi, entrambi risolti.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(TEXT(R"({ "type": "LogEventOrder", "category": "Facing", "outcome": "UsedByBlast", "thenCategory": "Facing", "thenOutcome": "DerivedFromMove" })"),
			Scenario, Error);
		if (TestTrue(FString::Printf(TEXT("scenario valido (%s)"), *Error), bOk)
			&& TestEqual(TEXT("una assertion"), Scenario.Expect.Num(), 1))
		{
			TestTrue(TEXT("kind LogEventOrder"), Scenario.Expect[0].Kind == ERTAssertionKind::LogEventOrder);
			TestEqual(TEXT("primo evento"), static_cast<int32>(Scenario.Expect[0].LogOutcome),
				static_cast<int32>(ERTFacingOutcome::UsedByBlast));
			TestEqual(TEXT("secondo evento"), static_cast<int32>(Scenario.Expect[0].ThenOutcome),
				static_cast<int32>(ERTFacingOutcome::DerivedFromMove));
		}
	}

	// 4. Un esito che appartiene a UN'ALTRA categoria e' rifiutato. E' il caso che una tabella scritta a mano
	//    sbaglierebbe piu' facilmente: `BridgeRemoved` esiste, ma non fra gli esiti di `Facing`.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(TEXT(R"({ "type": "LogEventCount", "category": "Facing", "outcome": "BridgeRemoved" })"),
			Scenario, Error);
		TestFalse(TEXT("esito di un'altra categoria: rifiutato"), bOk);
		TestTrue(TEXT("e l'errore nomina la categoria, o si cerca il difetto nel posto sbagliato"),
			Error.Contains(TEXT("Facing")));
	}

	// 5. Categoria inventata: rifiutata, con l'elenco di quelle previste.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(TEXT(R"({ "type": "LogEventCount", "category": "Meteo", "outcome": "Pioggia" })"),
			Scenario, Error);
		TestFalse(TEXT("categoria inesistente: rifiutata"), bOk);
		TestTrue(TEXT("l'errore elenca le categorie previste"), Error.Contains(TEXT("Environment")));
	}

	// 6. VALORE (#361): la terza primitiva di #318, sbloccata dalla decisione di formato di `spec-turnlog.md`
	//    §4.2. `Amount` e' il payload numerico di ogni categoria, quindi l'assertion non e' del solo bank.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(TEXT(R"({ "type": "LogEventAmount", "category": "Combat", "outcome": "Hit", "value": 24 })"),
			Scenario, Error);
		if (TestTrue(FString::Printf(TEXT("scenario valido (%s)"), *Error), bOk)
			&& TestEqual(TEXT("una assertion"), Scenario.Expect.Num(), 1))
		{
			TestTrue(TEXT("kind LogEventAmount"), Scenario.Expect[0].Kind == ERTAssertionKind::LogEventAmount);
			TestTrue(TEXT("categoria Combat"), Scenario.Expect[0].LogCategory == ERTLogCategory::Combat);
			TestEqual(TEXT("il valore atteso arriva dal JSON"), Scenario.Expect[0].Value, 24);
		}
	}

	// 7. `value` OBBLIGATORIO, al contrario di `LogEventCount`: li' l'omissione ha un default sensato
	//    («l'evento c'e'»), qui sarebbe un numero inventato dal parser, e uno scenario passerebbe senza dire
	//    cosa si aspettava.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(TEXT(R"({ "type": "LogEventAmount", "category": "Combat", "outcome": "Hit" })"),
			Scenario, Error);
		TestFalse(TEXT("senza 'value' l'assertion e' rifiutata"), bOk);
		TestTrue(TEXT("e l'errore nomina il campo mancante"), Error.Contains(TEXT("value")));
	}

	// 8. Un valore NEGATIVO e' legittimo qui, mentre `LogEventCount` lo rifiuta: un conteggio non puo' essere
	//    negativo, un `Amount` si' — una differenza di punteggio o un delta lo sono.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(TEXT(R"({ "type": "LogEventAmount", "category": "Combat", "outcome": "Hit", "value": -3 })"),
			Scenario, Error);
		if (TestTrue(FString::Printf(TEXT("un Amount negativo si scrive (%s)"), *Error), bOk)
			&& TestEqual(TEXT("una assertion"), Scenario.Expect.Num(), 1))
		{
			TestEqual(TEXT("e arriva intatto"), Scenario.Expect[0].Value, -3);
		}
	}

	return true;
}

/**
 * CP 13.5 (#160): un'unita' puo' essere dichiarata guidata dal BOT, e la sua condizione iniziale scritta.
 *
 * Le due meta' del test sono asimmetriche e devono restarlo. Il PARSING e' banale — tre chiavi — mentre cio'
 * che vale sono i due RIFIUTI: un intent scritto su un'unita' bot e un sentinella che si confonde con un
 * valore chiesto davvero. Senza il primo, uno scenario dichiara una mossa e ne gioca un'altra restando verde
 * quando le due combaciano per caso; senza il secondo, `"shield": 0` sarebbe indistinguibile da «shield non
 * dichiarato» e un'esca con lo scudo del roster passerebbe per un'esca senza scudo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderBotUnitTest,
	"RefactorTactics.Scenario.LoaderReadsBotUnitsAndCondition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderBotUnitTest::RunTest(const FString&)
{
	auto Load = [](const TCHAR* UnitsJson, const TCHAR* TurnsJson, FRTTestScenario& Out, FString& Error)
	{
		const FString Json = FString::Printf(TEXT(R"JSON(
		{
		  "scenarioId": "Test.BotUnits",
		  "mapRadius": 3,
		  "units": [ %s ],
		  "turns": [ %s ],
		  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
		})JSON"), UnitsJson, TurnsJson);
		return URTScenarioLoader::LoadFromString(Json, Out, Error);
	};

	// 1. Le tre chiavi arrivano, e i default non cambiano per chi non le scrive.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(
			TEXT(R"({ "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-1, 0, 0] },
			         { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [1, 0, 0], "bot": true, "health": 7, "shield": 0, "visionRange": 1 })"),
			TEXT(R"({ "intents": [] })"), Scenario, Error);
		if (TestTrue(FString::Printf(TEXT("scenario valido (%s)"), *Error), bOk))
		{
			const FRTScenarioUnit* A1 = Scenario.FindUnit(TEXT("A1"));
			const FRTScenarioUnit* B1 = Scenario.FindUnit(TEXT("B1"));
			if (TestNotNull(TEXT("A1"), A1) && TestNotNull(TEXT("B1"), B1))
			{
				TestFalse(TEXT("chi non dichiara `bot` non lo diventa"), A1->bBotControlled);
				TestTrue(TEXT("B1 e' guidata dal bot"), B1->bBotControlled);
				TestEqual(TEXT("health dichiarata"), B1->Health, 7);
				// La riga che il sentinella `0` renderebbe impossibile: `shield: 0` e' una RICHIESTA.
				TestEqual(TEXT("shield 0 e' un valore chiesto, non un campo assente"), B1->Shield, 0);
				TestEqual(TEXT("vista dichiarata"), B1->VisionRange, 1);
				TestEqual(TEXT("A1 non dichiara condizione: resta quella del roster"), A1->Health, -1);
				TestEqual(TEXT("idem lo scudo"), A1->Shield, -1);
				TestEqual(TEXT("idem la vista"), A1->VisionRange, -1);
			}
		}
	}

	// 2. Un intent su un'unita' bot e' un errore di SCRITTURA, non un piano.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(
			TEXT(R"({ "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-1, 0, 0], "bot": true },
			         { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [1, 0, 0] })"),
			TEXT(R"({ "intents": [ { "unit": "A1", "move": [[0, 0, 0]] } ] })"), Scenario, Error);
		TestFalse(TEXT("un intent su un'unita' bot e' rifiutato"), bOk);
		TestTrue(FString::Printf(TEXT("e il motivo nomina l'unita' (%s)"), *Error), Error.Contains(TEXT("A1")));
	}

	// 3. Una salute a zero schiererebbe un cadavere.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(
			TEXT(R"({ "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-1, 0, 0], "health": 0 })"),
			TEXT(R"({ "intents": [] })"), Scenario, Error);
		TestFalse(TEXT("health 0 e' rifiutata"), bOk);
	}

	return true;
}

/**
 * CP 13.5 (#160): le VARIANTI, e i quattro modi in cui un canary differenziale puo' nascere gia' vuoto.
 *
 * Ognuno dei rifiuti qui sotto corrisponde a uno scenario che sarebbe stato **verde per sempre** senza mettere
 * alla prova niente: un confronto fra una cosa sola, una variante che non cambia nulla, due varianti con lo
 * stesso nome (il report non saprebbe dire quale e' rossa), e un'unita' spostata dove ce n'e' gia' un'altra.
 * Sono la ragione per cui la validazione delle varianti sta nel loader e non nella buona volonta' di chi
 * scrive lo scenario.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderVariantsTest,
	"RefactorTactics.Scenario.LoaderRejectsEmptyVariantComparisons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderVariantsTest::RunTest(const FString&)
{
	auto Load = [](const TCHAR* VariantsJson, const TCHAR* SameJson, FRTTestScenario& Out, FString& Error)
	{
		const FString Json = FString::Printf(TEXT(R"JSON(
		{
		  "scenarioId": "Test.Variants",
		  "mapRadius": 4,
		  "units": [
		    { "id": "A1", "hero": "Hero.Gadget",    "team": 0, "cell": [-1, 0, 0] },
		    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [1, 0, 0] }
		  ],
		  %s
		  "variants": [ %s ],
		  "turns": [ { "intents": [] } ],
		  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
		})JSON"), SameJson, VariantsJson);
		return URTScenarioLoader::LoadFromString(Json, Out, Error);
	};

	// 1. Il caso valido, che da' senso ai rifiuti sotto.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(
			TEXT(R"({ "name": "a", "units": [ { "id": "A1", "cell": [-2, 0, 0] } ] },
			         { "name": "b", "units": [ { "id": "A1", "cell": [-3, 0, 0] } ] })"),
			TEXT(R"("expectSameAcrossVariants": true,)"), Scenario, Error);
		if (TestTrue(FString::Printf(TEXT("due varianti valide (%s)"), *Error), bOk))
		{
			TestEqual(TEXT("due varianti lette"), Scenario.Variants.Num(), 2);
			TestTrue(TEXT("il confronto e' richiesto"), Scenario.bExpectSameAcrossVariants);
			if (Scenario.Variants.Num() == 2)
			{
				TestEqual(TEXT("la seconda sposta A1"), Scenario.Variants[1].Units[0].Cell, FRTCellId(-3, 0, 0));
			}
		}
	}

	// 2. Un confronto fra UNA cosa: verde per costruzione.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(TEXT(R"({ "name": "sola", "units": [ { "id": "A1", "cell": [-2, 0, 0] } ] })"),
			TEXT(R"("expectSameAcrossVariants": true,)"), Scenario, Error);
		TestFalse(TEXT("una sola variante e' rifiutata"), bOk);
	}

	// 3. Una variante che non sposta niente: il suo TurnLog coincide senza aver provato nessun ingresso.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(
			TEXT(R"({ "name": "a", "units": [ { "id": "A1", "cell": [-2, 0, 0] } ] }, { "name": "vuota", "units": [] })"),
			TEXT(R"("expectSameAcrossVariants": true,)"), Scenario, Error);
		TestFalse(TEXT("una variante che non cambia nulla e' rifiutata"), bOk);
	}

	// 4. Due varianti con lo stesso nome: il report non saprebbe dire quale e' rossa.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(
			TEXT(R"({ "name": "a", "units": [ { "id": "A1", "cell": [-2, 0, 0] } ] },
			         { "name": "a", "units": [ { "id": "A1", "cell": [-3, 0, 0] } ] })"),
			TEXT(R"("expectSameAcrossVariants": true,)"), Scenario, Error);
		TestFalse(TEXT("due varianti omonime sono rifiutate"), bOk);
	}

	// 5. Una variante che mette due unita' sulla stessa cella: e' invalida SOLO nella variante, cioe' nel
	//    posto in cui la validazione dell'allestimento normale non guarda.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(
			TEXT(R"({ "name": "a", "units": [ { "id": "A1", "cell": [1, 0, 0] } ] },
			         { "name": "b", "units": [ { "id": "A1", "cell": [-3, 0, 0] } ] })"),
			TEXT(R"("expectSameAcrossVariants": true,)"), Scenario, Error);
		TestFalse(TEXT("A1 spostata sopra B1 e' rifiutata"), bOk);
		TestTrue(FString::Printf(TEXT("e il motivo nomina la variante (%s)"), *Error), Error.Contains(TEXT("'a'")));
	}

	// 6. Una variante che sposta un'unita' inesistente.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(
			TEXT(R"({ "name": "a", "units": [ { "id": "FANTASMA", "cell": [-2, 0, 0] } ] },
			         { "name": "b", "units": [ { "id": "A1", "cell": [-3, 0, 0] } ] })"),
			TEXT(R"("expectSameAcrossVariants": true,)"), Scenario, Error);
		TestFalse(TEXT("spostare un'unita' non schierata e' rifiutato"), bOk);
	}

	return true;
}

/**
 * La rotazione DICHIARATA nell'intent (D-020, #291): la chiave si legge, il flag distingue «E» dichiarato da
 * «campo assente», e una direzione inventata viene rifiutata con un motivo.
 *
 * ⚠️ **Il flag e' la meta' che conta.** `ERTHexDirection::E` e' il valore di default del campo *e* una
 * direzione legittima: senza `bDeclaresFacing` un intent che non nomina `facing` sarebbe indistinguibile da
 * uno che chiede di girarsi a est, e ogni unita' di ogni scenario dichiarerebbe una rotazione senza saperlo.
 * E' la stessa ragione per cui `TargetCell` ha `bTargetsCell` e `CoverEdge` ha `bHasCoverEdge`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderDeclaredFacingTest,
	"RefactorTactics.Scenario.LoaderReadsDeclaredFacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderDeclaredFacingTest::RunTest(const FString&)
{
	auto Load = [](const TCHAR* IntentJson, FRTTestScenario& Out, FString& Error)
	{
		const FString Json = FString::Printf(TEXT(R"JSON(
		{
		  "scenarioId": "Test.DeclaredFacing",
		  "mapRadius": 3,
		  "units": [ { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [0, 0, 0] },
		             { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [2, 0, 0] } ],
		  "turns": [ { "requires": ["DeclaredRotation"], "intents": [ %s ] } ],
		  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
		})JSON"), IntentJson);
		return URTScenarioLoader::LoadFromString(Json, Out, Error);
	};

	// 1. Dichiarata: arriva la direzione E si alza il flag.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(TEXT(R"({ "unit": "A1", "facing": "NW" })"), Scenario, Error);
		if (TestTrue(FString::Printf(TEXT("scenario valido (%s)"), *Error), bOk)
			&& TestTrue(TEXT("un turno con un intent"),
				Scenario.Turns.Num() == 1 && Scenario.Turns[0].Intents.Num() == 1))
		{
			const FRTScenarioIntent& Intent = Scenario.Turns[0].Intents[0];
			TestTrue(TEXT("il flag dichiara che la rotazione c'e'"), Intent.bDeclaresFacing);
			TestEqual(TEXT("ed e' quella scritta nel file"), Intent.Facing, ERTHexDirection::NW);
		}
	}

	// 2. NON dichiarata: il flag resta falso. Senza questa meta' il caso 1 passerebbe anche con un flag
	//    sempre vero, e ogni intent del repository si porterebbe dietro una rotazione mai chiesta.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(TEXT(R"({ "unit": "A1", "move": [[1, 0, 0]] })"), Scenario, Error);
		if (TestTrue(FString::Printf(TEXT("scenario valido (%s)"), *Error), bOk)
			&& Scenario.Turns.Num() == 1 && Scenario.Turns[0].Intents.Num() == 1)
		{
			TestFalse(TEXT("senza la chiave, nessuna rotazione dichiarata"),
				Scenario.Turns[0].Intents[0].bDeclaresFacing);
		}
	}

	// 3. Direzione inventata: rifiutata con un motivo, non ricondotta silenziosamente a `E`.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(TEXT(R"({ "unit": "A1", "facing": "NNE" })"), Scenario, Error);
		TestFalse(TEXT("una direzione inesistente e' rifiutata"), bOk);
		TestTrue(FString::Printf(TEXT("e il motivo la nomina (era: '%s')"), *Error),
			Error.Contains(TEXT("NNE")));
	}

	return true;
}

/**
 * Le decisioni di finestra si caricano come DATO, in ordine e col vocabolario dello scenario (CP 15.3
 * meta' B, `#512`).
 *
 * L'ORDINE e' significativo: la coda si consuma in ordine di dichiarazione, e un abbinamento posizionale
 * cambierebbe bersaglio quando cambia il movimento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderDecisionsTest,
	"RefactorTactics.Scenario.LoaderAcceptsDecisions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderDecisionsTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("scenario con decisions accettato"),
		URTScenarioLoader::LoadFromString(ScenarioLoaderDecisionsJson, Scenario, Error)))
	{
		AddError(FString::Printf(TEXT("motivo del rifiuto: %s"), *Error));
		return false;
	}

	if (!TestEqual(TEXT("un turno"), Scenario.Turns.Num(), 1)) { return false; }
	const FRTScenarioTurn& T = Scenario.Turns[0];
	if (!TestEqual(TEXT("due decisioni"), T.Decisions.Num(), 2)) { return false; }

	TestEqual(TEXT("prima: unita'"),     T.Decisions[0].Unit,    FString(TEXT("A1")));
	TestEqual(TEXT("prima: risposta"),   T.Decisions[0].Respond, FString(TEXT("FIRE")));
	TestEqual(TEXT("prima: bersaglio"),  T.Decisions[0].Target,  FString(TEXT("B1")));
	TestEqual(TEXT("seconda: risposta"), T.Decisions[1].Respond, FString(TEXT("HOLD")));
	TestTrue(TEXT("HOLD non porta bersaglio"), T.Decisions[1].Target.IsEmpty());
	return true;
}

/**
 * La validazione e' la meta' che conta. Uno scenario scritto male deve produrre un ERROR leggibile, non un
 * `HOLD` silenzioso: `HoldNoDecider` e' indistinguibile da «nessuno ha risposto», ed e' il modo in cui un
 * refuso resta verde per sempre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderDecisionsRejectTest,
	"RefactorTactics.Scenario.LoaderRejectsMalformedDecisions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderDecisionsRejectTest::RunTest(const FString&)
{
	auto Rifiuta = [this](const TCHAR* Cosa, const TCHAR* Decision, const TCHAR* Atteso)
	{
		const FString Json = FString::Printf(TEXT(R"JSON(
		{
		  "scenarioId": "Spec.Decisions.Reject", "version": 2, "mapRadius": 3,
		  "units": [
		    { "id": "A1", "hero": "Hero.Riktor", "team": 0, "cell": [-2, 0, 0] },
		    { "id": "B1", "hero": "Hero.Wraith",  "team": 1, "cell": [ 2, 0, 0] }
		  ],
		  "turns": [ { "intents": [], "decisions": [ %s ] } ],
		  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
		}
		)JSON"), Decision);

		FRTTestScenario Scenario;
		FString Error;
		const bool bLoaded = URTScenarioLoader::LoadFromString(*Json, Scenario, Error);
		TestFalse(FString::Printf(TEXT("%s: rifiutato"), Cosa), bLoaded);
		// Il messaggio deve NOMINARE il difetto: un rifiuto muto manda a cercare nel posto sbagliato.
		TestTrue(FString::Printf(TEXT("%s: il motivo nomina '%s' (era: '%s')"), Cosa, Atteso, *Error),
			Error.Contains(Atteso));
	};

	Rifiuta(TEXT("chiave sconosciuta"),
		TEXT(R"({ "unit": "A1", "respond": "HOLD", "reason": "perche' si" })"), TEXT("reason"));
	Rifiuta(TEXT("respond sconosciuto"),
		TEXT(R"({ "unit": "A1", "respond": "MAYBE" })"), TEXT("MAYBE"));
	Rifiuta(TEXT("FIRE senza bersaglio"),
		TEXT(R"({ "unit": "A1", "respond": "FIRE" })"), TEXT("target"));
	Rifiuta(TEXT("HOLD con bersaglio"),
		TEXT(R"({ "unit": "A1", "respond": "HOLD", "target": "B1" })"), TEXT("B1"));
	Rifiuta(TEXT("unita' non schierata"),
		TEXT(R"({ "unit": "Z9", "respond": "HOLD" })"), TEXT("Z9"));
	Rifiuta(TEXT("bersaglio non schierato"),
		TEXT(R"({ "unit": "A1", "respond": "FIRE", "target": "Z9" })"), TEXT("Z9"));
	return true;
}

/** Un refuso a livello di turno non deve essere ignorato: `desicions` non e' un commento. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderTurnKeysTest,
	"RefactorTactics.Scenario.LoaderRejectsUnknownTurnKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderTurnKeysTest::RunTest(const FString&)
{
	// ⚠️ `expect` NON puo' essere vuoto: il loader lo rifiuta a monte («nessuna assertion dichiarata»), e
	// un JSON di prova senza assertion farebbe passare il primo `TestFalse` per la ragione sbagliata e
	// cadere il secondo caso per sempre. Il piano lo dichiara nei vincoli globali e lo violava qui.
	const TCHAR* Json = TEXT(R"JSON(
	{
	  "scenarioId": "Spec.Decisions.TurnKey", "version": 1, "mapRadius": 3,
	  "units": [ { "id": "A1", "hero": "Hero.Riktor", "team": 0, "cell": [-2, 0, 0] } ],
	  "turns": [ { "intents": [], "desicions": [] } ],
	  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
	}
	)JSON");

	FRTTestScenario Scenario;
	FString Error;
	TestFalse(TEXT("refuso di turno rifiutato"), URTScenarioLoader::LoadFromString(Json, Scenario, Error));
	// Il messaggio deve nominare LA CHIAVE: un rifiuto generico manda a cercare nel posto sbagliato, ed e'
	// anche cio' che distingue questo controllo da quello — gia' esistente — sul contenuto di `intents`.
	TestTrue(FString::Printf(TEXT("il messaggio nomina 'desicions' (era: '%s')"), *Error),
		Error.Contains(TEXT("desicions")));

	// E il commento resta un commento: `_turno` e `_nota` non devono cadere insieme al refuso.
	const TCHAR* ConCommento = TEXT(R"JSON(
	{
	  "scenarioId": "Spec.Decisions.TurnComment", "version": 1, "mapRadius": 3,
	  "units": [ { "id": "A1", "hero": "Hero.Riktor", "team": 0, "cell": [-2, 0, 0] } ],
	  "turns": [ { "_turno": "commento", "_nota": "altro", "intents": [] } ],
	  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
	}
	)JSON");
	FRTTestScenario ConCommentoScenario;
	FString CommentoError;
	if (!TestTrue(TEXT("i commenti di turno restano ammessi"),
		URTScenarioLoader::LoadFromString(ConCommento, ConCommentoScenario, CommentoError)))
	{
		AddError(FString::Printf(TEXT("motivo del rifiuto: %s"), *CommentoError));
	}
	return true;
}

/**
 * Il formato cresce di una versione, e il gate serve nel verso che conta: una build VECCHIA deve
 * **rifiutare** uno scenario che non sa leggere, non ignorarne i campi. Da `#926` gli scenari viaggiano
 * dentro il pacchetto, quindi la coppia build/dato puo' disallinearsi davvero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderVersionTwoTest,
	"RefactorTactics.Scenario.LoaderSupportsVersionTwo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderVersionTwoTest::RunTest(const FString&)
{
	auto Prova = [this](int32 Versione, bool bAtteso)
	{
		const FString Json = FString::Printf(TEXT(R"JSON(
		{
		  "scenarioId": "Spec.Decisions.Version", "version": %d, "mapRadius": 3,
		  "units": [ { "id": "A1", "hero": "Hero.Riktor", "team": 0, "cell": [-2, 0, 0] } ],
		  "turns": [ { "intents": [] } ],
		  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
		}
		)JSON"), Versione);

		FRTTestScenario Scenario;
		FString Error;
		const bool bLoaded = URTScenarioLoader::LoadFromString(*Json, Scenario, Error);
		TestEqual(FString::Printf(TEXT("versione %d"), Versione), bLoaded, bAtteso);
	};

	Prova(1, true);   // i 76 scenari esistenti restano a 1 e non si toccano
	Prova(2, true);   // la versione che dichiara `decisions`
	// 🔁 **`3` e' passata da rifiutata ad accettata il 2026-08-19** (E14.7 fetta 4): e' la versione che
	// ammette in `decisions.respond` le risposte di un Reaction Profile. Il numero si e' mosso di uno e il
	// test **non** ha perso il proprio mestiere: il caso «il gate resta un gate» e' ora la riga sotto.
	Prova(3, true);
	// ⚠️ **La riga che conta e' questa, e va SEMPRE tenuta a `SupportedVersion + 1`**: senza, il test
	// dimostrerebbe solo che le versioni note si caricano — cioe' niente sul gate. Il verso protetto e'
	// quello dichiarato in `RTScenarioLoader.h`: una build vecchia deve RIFIUTARE uno scenario che non sa
	// leggere invece di ignorarne i campi, e da `#926` gli scenari viaggiano dentro il pacchetto.
	Prova(URTScenarioLoader::SupportedVersion + 1, false);
	return true;
}

/**
 * `loadout` ha **tre** forme, e la differenza fra due di esse è invisibile a un conteggio (#1054).
 *
 * - **assente** → l'unità riceve il default del suo eroe, cioè ciò che la partita monta;
 * - **`[]`** → l'unità entra **spoglia**, e lo scenario lo sta dichiarando;
 * - **lista** → quei pezzi, e vincono sul default.
 *
 * ⚠️ **Perché serve la seconda.** Quando `#1054` ha acceso i default anche nell'harness, cinque scenari
 * hanno cambiato risultato — e quattro non avevano numeri sbagliati: avevano perso la **variabile che
 * tenevano ferma**. `Spec.Brace.GuardAndBraceOnMixedHit` misura il confine Guard/Brace *con spinta 1*, e
 * il default di Phase porta la spinta a 2: senza un modo di dire «questa unità entra spoglia», quello
 * scenario non può più esistere — diventa il gemello di `PushBeyondGuardThreshold`.
 *
 * ⚠️ E `[]` **non** si distingue da assente contando gli elementi: entrambe danno `Loadout.Num() == 0`.
 * È per questo che esiste `bLoadoutDeclared` invece di una condizione sul conteggio, ed è il difetto che
 * questo test impedisce di reintrodurre — sarebbe silenzioso, perché il corpus resterebbe verde mentre
 * quattro scenari misurano di nuovo la cosa sbagliata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoadoutThreeFormsTest,
	"RefactorTactics.Scenario.LoadoutHasThreeForms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoadoutThreeFormsTest::RunTest(const FString&)
{
	auto Carica = [this](const TCHAR* CampoLoadout, const TCHAR* Etichetta) -> FRTScenarioUnit
	{
		const FString Json = FString::Printf(TEXT(R"JSON(
		{
		  "scenarioId": "Spec.Loadout.Forme", "version": 1, "mapRadius": 3,
		  "units": [ { "id": "A1", "hero": "Hero.Riktor", "team": 0, "cell": [-2, 0, 0]%s } ],
		  "turns": [ { "intents": [] } ],
		  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
		}
		)JSON"), CampoLoadout);

		FRTTestScenario Scenario;
		FString Error;
		if (!URTScenarioLoader::LoadFromString(*Json, Scenario, Error))
		{
			AddError(FString::Printf(TEXT("%s: caricamento fallito — %s"), Etichetta, *Error));
			return FRTScenarioUnit();
		}
		if (Scenario.Units.Num() != 1)
		{
			AddError(FString::Printf(TEXT("%s: attesa una unita'"), Etichetta));
			return FRTScenarioUnit();
		}
		return Scenario.Units[0];
	};

	// 1. Assente: nessuna dichiarazione, quindi la sessione userà il default dell'eroe.
	const FRTScenarioUnit Assente = Carica(TEXT(""), TEXT("assente"));
	TestFalse(TEXT("assente: non dichiarato"), Assente.bLoadoutDeclared);
	TestEqual(TEXT("assente: lista vuota"), Assente.Loadout.Num(), 0);

	// 2. Vuoto: DICHIARATO, e dichiarato vuoto. È la forma che il conteggio non distingue dalla prima.
	const FRTScenarioUnit Vuoto = Carica(TEXT(", \"loadout\": []"), TEXT("vuoto"));
	TestTrue(TEXT("vuoto: DICHIARATO"), Vuoto.bLoadoutDeclared);
	TestEqual(TEXT("vuoto: lista vuota"), Vuoto.Loadout.Num(), 0);

	// 3. Pieno: dichiarato con i pezzi. Un loadout legale, o `Validate` lo rifiuterebbe prima.
	const FRTScenarioUnit Pieno = Carica(
		TEXT(", \"loadout\": [\"Weapon.Impact\", \"Gadget.Medkit\", \"Reaction.Anchor\"]"), TEXT("pieno"));
	TestTrue(TEXT("pieno: dichiarato"), Pieno.bLoadoutDeclared);
	TestEqual(TEXT("pieno: tre pezzi"), Pieno.Loadout.Num(), 3);

	// L'asserzione che rende il test non vacuo: le prime due sono indistinguibili SUL CONTEGGIO e distinte
	// sul flag. Senza questa riga il test passerebbe anche con `bLoadoutDeclared` sempre falso.
	TestNotEqual(TEXT("assente e vuoto si distinguono, e non per il numero di pezzi"),
		Assente.bLoadoutDeclared, Vuoto.bLoadoutDeclared);
	TestEqual(TEXT("...mentre il conteggio le direbbe uguali"), Assente.Loadout.Num(), Vuoto.Loadout.Num());
	return true;
}

/**
 * Il bump a 2 deve GATARE la feature per cui e' stato fatto, o non gatea niente.
 *
 * Trovato in code review: il solo controllo era `Version > SupportedVersion`, quindi un file `version: 1`
 * con `decisions` veniva accettato. Spedito cosi', una build vecchia — che non conosce ne' la chiave ne' la
 * versione — lo caricherebbe ignorando le decisioni e giocherebbe il turno non scriptato: il silenzio che
 * il bump esiste per impedire.
 *
 * E una chiave `decisions` che non e' un array non deve sparire: supera il controllo sulle chiavi di turno,
 * perche' la chiave e' nota, e senza questo verrebbe saltata da `TryGetArrayField` senza un errore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderDecisionsNeedVersionTwoTest,
	"RefactorTactics.Scenario.LoaderRejectsDecisionsBelowVersionTwo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderDecisionsNeedVersionTwoTest::RunTest(const FString&)
{
	auto Carica = [](int32 Versione, const TCHAR* DecisionsField, FString& OutError)
	{
		const FString Json = FString::Printf(TEXT(R"JSON(
		{
		  "scenarioId": "Spec.Decisions.VersionGate", "version": %d, "mapRadius": 3,
		  "units": [ { "id": "A1", "hero": "Hero.Riktor", "team": 0, "cell": [-2, 0, 0] } ],
		  "turns": [ { "intents": [], %s } ],
		  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
		}
		)JSON"), Versione, DecisionsField);
		FRTTestScenario Scenario;
		return URTScenarioLoader::LoadFromString(*Json, Scenario, OutError);
	};

	const TCHAR* UnaDecisione = TEXT(R"("decisions": [ { "unit": "A1", "respond": "HOLD" } ])");

	FString Error;
	TestFalse(TEXT("version 1 con decisions e' rifiutato"), Carica(1, UnaDecisione, Error));
	TestTrue(FString::Printf(TEXT("e il motivo nomina la versione (era: '%s')"), *Error),
		Error.Contains(TEXT("version")));

	Error.Reset();
	TestTrue(FString::Printf(TEXT("version 2 con le stesse decisions e' accettato (errore: '%s')"), *Error),
		Carica(2, UnaDecisione, Error));

	// Una lista VUOTA non chiede nulla di nuovo: non deve costringere ad alzare la versione.
	Error.Reset();
	TestTrue(TEXT("version 1 con decisions vuote resta valida"),
		Carica(1, TEXT(R"("decisions": [])"), Error));

	Error.Reset();
	TestFalse(TEXT("decisions non-array e' rifiutato"), Carica(2, TEXT(R"("decisions": {})"), Error));
	TestTrue(FString::Printf(TEXT("e il motivo dice che serve un array (era: '%s')"), *Error),
		Error.Contains(TEXT("array")));
	return true;
}

/**
 * `Reaction` e `DecisionBoundary` sono due nomi con due regimi, e il confine e' la CARDINALITA' — non due
 * nomi per la stessa cosa. Il test esiste perche' la divisione, senza, vive solo in un commento: e un
 * commento che dichiara la propria scadenza l'ha gia' mancata una volta.
 *
 * ⚠️ Servono ENTRAMBE le domande, e il piano ne faceva una sola. `IsKnownCapability` e' vera anche per le
 * indisponibili — e' il suo scopo — quindi chiederla per `Reaction` e per `DecisionBoundary` avrebbe
 * pinnato soltanto che i due nomi esistono, non che stanno da parti opposte. La divisione E' la coppia
 * `noto` × `disponibile`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioReactionSplitTest,
	"RefactorTactics.Scenario.ReactionAndDecisionBoundaryAreDistinct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioReactionSplitTest::RunTest(const FString&)
{
	// `Reaction` copre il regime E5 `AllowedResponses <= 1` ed e' DISPONIBILE: i tre scenari che la chiedono
	// girano oggi.
	TestTrue(TEXT("Reaction e' nota"), FRTScenarioSession::IsKnownCapability(TEXT("Reaction")));
	TestTrue(TEXT("e disponibile"), FRTScenarioSession::IsAvailableCapability(TEXT("Reaction")));

	// `DecisionBoundary` copre `>= 2`: nome NOTO — quindi un turno che lo chiede vale `Blocked`, non
	// `Error`.
	//
	// 🔴 **La seconda asserzione diceva `TestFalse(... "ma NON e' disponibile in fase A")` ed e' caduta il
	// 2026-08-16 con la fase B, che l'ha resa disponibile.** Il test dichiarava la propria scadenza nella
	// stringa e l'ha rispettata. ⚠️ Ma il difetto non e' il valore, e' la DISCRIMINANTE: la divisione fra
	// `Reaction` e `DecisionBoundary` non e' mai stata «una c'e' e l'altra no» — quello era un accidente
	// della fase A. E' che sono due NOMI distinti per due regimi distinti (`AllowedResponses <= 1` contro
	// `>= 2`), e resta vera ora che sono entrambe disponibili. Ripararla mettendo `TestTrue` avrebbe tenuto
	// una discriminante che scade di nuovo alla prossima capability.
	TestTrue(TEXT("DecisionBoundary e' un nome noto"),
		FRTScenarioSession::IsKnownCapability(TEXT("DecisionBoundary")));
	TestTrue(TEXT("ed e' disponibile dalla fase B di #512"),
		FRTScenarioSession::IsAvailableCapability(TEXT("DecisionBoundary")));

	// ⚠️ **La divisione e' che i due nomi sono DIVERSI, e questa riga e' cio' che lo rende falsificabile.**
	// Con entrambe disponibili, le quattro asserzioni qui sopra passerebbero anche se `IsKnownCapability` /
	// `IsAvailableCapability` ignorassero l'argomento e rispondessero sempre `true`: e' il test vacuo che
	// questo repository chiama «un test che smette di verificare senza dirlo». Serve un nome NOTO e NON
	// disponibile, e dev'essere `NeverAvailable` — il nome **riservato**, che per costruzione non atterra
	// mai. 🔴 La prima stesura usava `ReactionProfile`: noto e non disponibile oggi, ma atterra con E14.7
	// (`#314`), e da quel giorno questa coppia sarebbe tornata vacua senza che nulla diventasse rosso.
	TestTrue(TEXT("NeverAvailable e' un nome noto"),
		FRTScenarioSession::IsKnownCapability(TEXT("NeverAvailable")));
	TestFalse(TEXT("e NON e' disponibile: i due insiemi restano distinti"),
		FRTScenarioSession::IsAvailableCapability(TEXT("NeverAvailable")));

	// ⛔ **Limite DICHIARATO, e vale la pena scriverlo invece di simularne la copertura.** Cio' che questo
	// test verifica e' il VOCABOLARIO — quali nomi esistono e quali sono disponibili — non la cardinalita'
	// che li distingue: `Reaction` copre `AllowedResponses <= 1` e `DecisionBoundary` copre `>= 2`, e quella
	// non e' un campo interrogabile da qui. Chi collassasse i due nomi ai call site lascerebbe questo test
	// verde. Il regime lo verificano gli SCENARI — `Spec.Overwatch.HoldThenFire` apre due finestre a una
	// risposta ciascuna — e aggiungere qui un `TestNotEqual` fra due stringhe letterali darebbe l'apparenza
	// della copertura senza poter fallire mai, che e' il difetto che questo file combatte da tre commit.

	// E un nome inventato resta un errore di scrittura, non un'attesa: e' l'altra meta' del vocabolario.
	TestFalse(TEXT("un nome inventato non e' noto"),
		FRTScenarioSession::IsKnownCapability(TEXT("ReactionDecision")));
	return true;
}

/**
 * La CONDIZIONE dichiarata arriva dal JSON al piano ([D-109], #583).
 *
 * Legge i due campi che la compongono — quale condizione e con quale soglia — perche' sono cose diverse: un
 * `id` giusto con un `param` perso in strada produrrebbe «spara sotto lo 0%», cioe' una condizione mai vera,
 * e lo scenario direbbe di aver ristretto il fuoco mentre lo ha spento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderDeclaredConditionTest,
	"RefactorTactics.Scenario.LoaderReadsDeclaredCondition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderDeclaredConditionTest::RunTest(const FString&)
{
	const FString Json = TEXT(R"JSON(
	{
	  "scenarioId": "Spec.Condition.Probe", "version": 2, "mapRadius": 3,
	  "units": [
	    { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-2, 0, 0] },
	    { "id": "V1", "hero": "Hero.Wraith", "team": 1, "cell": [ 2, 0, 0] }
	  ],
	  "turns": [ { "intents": [
	    { "unit": "V1", "ability": "Action.Overwatch", "reaction": "Hero.Wraith.Deflection",
	      "condition": { "id": "TargetHealthAtOrBelowPercent", "param": 10 } },
	    { "unit": "A1", "move": [[-1, 0, 0]] }
	  ] } ],
	  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
	}
	)JSON");

	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("scenario con condition accettato"),
		URTScenarioLoader::LoadFromString(*Json, Scenario, Error)))
	{
		AddError(FString::Printf(TEXT("motivo del rifiuto: %s"), *Error));
		return false;
	}

	if (!TestEqual(TEXT("un turno"), Scenario.Turns.Num(), 1)) { return false; }
	if (!TestEqual(TEXT("due intent"), Scenario.Turns[0].Intents.Num(), 2)) { return false; }

	const FRTScenarioIntent& Watcher = Scenario.Turns[0].Intents[0];
	TestTrue(TEXT("la condizione e' dichiarata"), Watcher.Condition.IsDeclared());
	TestEqual(TEXT("con il proprio id"), Watcher.Condition.Id,
		URTReactionOpportunityLibrary::TargetHealthAtOrBelowPercent());
	// Il valore CONCRETO, non «diverso da zero»: e' la soglia, e un test che accettasse qualunque numero
	// lascerebbe passare un parsing che legge il campo sbagliato.
	TestEqual(TEXT("e la propria soglia"), Watcher.Condition.Param, 10);

	// Chi NON la dichiara resta senza, e non eredita quella del vicino: `FRTScenarioIntent` e' per intent, ma
	// il default va verificato invece che assunto — e' la meta' che un parsing sbagliato romperebbe in
	// silenzio, dando a un'unita' una condizione che non ha chiesto.
	TestFalse(TEXT("l'altro intent non ha condizione"), Scenario.Turns[0].Intents[1].Condition.IsDeclared());
	return true;
}

/**
 * Il rifiuto MOTIVATO di una condizione mal posta.
 *
 * Vale piu' del test qui sopra, e la ragione e' misurata: `ARTUnit::SetPlannedReactionCondition` restituisce
 * `false` senza scrivere niente — nessun log, nessun errore. Una condizione rifiutata a valle produrrebbe uno
 * scenario che gira SENZA condizione: l'opportunity non collassa, la finestra si apre, e chi legge vede solo
 * un'assertion sugli HP che non torna. Il difetto arriva a valle travestito da regressione di gioco.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderConditionRejectTest,
	"RefactorTactics.Scenario.LoaderRejectsMalformedCondition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderConditionRejectTest::RunTest(const FString&)
{
	auto Rifiuta = [this](const TCHAR* Cosa, const TCHAR* Intent, const TCHAR* Atteso)
	{
		const FString Json = FString::Printf(TEXT(R"JSON(
		{
		  "scenarioId": "Spec.Condition.Reject", "version": 2, "mapRadius": 3,
		  "units": [
		    { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-2, 0, 0] },
		    { "id": "V1", "hero": "Hero.Wraith", "team": 1, "cell": [ 2, 0, 0] }
		  ],
		  "turns": [ { "intents": [ %s ] } ],
		  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
		}
		)JSON"), Intent);

		FRTTestScenario Scenario;
		FString Error;
		const bool bLoaded = URTScenarioLoader::LoadFromString(*Json, Scenario, Error);
		TestFalse(FString::Printf(TEXT("%s: rifiutato"), Cosa), bLoaded);
		TestTrue(FString::Printf(TEXT("%s: il motivo nomina '%s' (era: '%s')"), Cosa, Atteso, *Error),
			Error.Contains(Atteso));
	};

	// 🔴 **Il caso che falliva DAVVERO in silenzio, e che questo test non copriva.** Gli altri sei
	// esercitano strade che producono gia' un `OutError`; questa no. `TryGetObjectField` restituisce `false`
	// per qualunque valore non-oggetto, `condition` e' una chiave NOTA quindi il gate delle chiavi
	// sconosciute tace, e lo scenario girava SENZA condizione con l'autore che leggeva solo un'assertion del
	// TurnLog caduta. La forma con la stringa e' anche la prima che verrebbe in mente: ogni altro campo
	// dell'intent e' una stringa. Trovato dalla code review, non dal test che diceva di coprire il silenzio.
	Rifiuta(TEXT("condition come stringa invece che oggetto"),
		TEXT(R"({ "unit": "V1", "reaction": "Hero.Wraith.Deflection",
		          "condition": "TargetHealthAtOrBelowPercent" })"),
		TEXT("oggetto"));
	Rifiuta(TEXT("condition come numero"),
		TEXT(R"({ "unit": "V1", "reaction": "Hero.Wraith.Deflection", "condition": 10 })"),
		TEXT("oggetto"));

	// Il gemello del `101` gia' coperto: `IsDeclaredConditionAllowed` chiede `Param >= 0` e la coppia
	// verifica **entrambi** i lati del dominio, non solo quello a cui si pensa per primo.
	Rifiuta(TEXT("soglia negativa"),
		TEXT(R"({ "unit": "V1", "reaction": "Hero.Wraith.Deflection",
		          "condition": { "id": "TargetHealthAtOrBelowPercent", "param": -1 } })"),
		TEXT("-1"));

	// ⚠️ **Il caso che morde davvero**, e non e' un caso di scuola: l'Overwatch costa l'azione PRINCIPALE,
	// quindi un intent di solo-Overwatch con una condizione e' precisamente cio' che qualcuno scrivera' per
	// primo leggendo [D-109]. `SetPlannedReactionCondition` lo rifiuterebbe in silenzio.
	Rifiuta(TEXT("condizione senza reazione armata"),
		TEXT(R"({ "unit": "V1", "ability": "Action.Overwatch",
		          "condition": { "id": "TargetHealthAtOrBelowPercent", "param": 10 } })"),
		TEXT("reaction"));

	// L'elenco delle condizioni ammesse e' CHIUSO e vive nel gioco: l'harness non puo' conoscerne una che il
	// resolver non sa valutare, o produrrebbe un piano che il gioco scarta.
	Rifiuta(TEXT("condizione inesistente"),
		TEXT(R"({ "unit": "V1", "reaction": "Hero.Wraith.Deflection",
		          "condition": { "id": "TargetIsFlanked", "param": 1 } })"),
		TEXT("TargetIsFlanked"));

	// Oltre il 100% sarebbe sempre vera — una condizione che non condiziona.
	Rifiuta(TEXT("soglia oltre il 100"),
		TEXT(R"({ "unit": "V1", "reaction": "Hero.Wraith.Deflection",
		          "condition": { "id": "TargetHealthAtOrBelowPercent", "param": 101 } })"),
		TEXT("101"));

	// Gate `G7`: il confronto del gioco e' in aritmetica intera, quindi un `50.5` verrebbe troncato in
	// silenzio e lo scenario descriverebbe una soglia che non e' la sua.
	Rifiuta(TEXT("soglia in virgola mobile"),
		TEXT(R"({ "unit": "V1", "reaction": "Hero.Wraith.Deflection",
		          "condition": { "id": "TargetHealthAtOrBelowPercent", "param": 50.5 } })"),
		TEXT("INTERO"));

	// 🔴 **Il caso che il controllo di interezza NON intercetta**, e per cui il range va verificato prima del
	// cast: `1e20` E' un intero, quindi passa di li' senza un graffio, ma non entra in un `int32` — e
	// `static_cast` di un double fuori scala e' undefined behavior. Il valore indefinito che ne esce
	// arriverebbe a `IsDeclaredConditionAllowed`, che controlla `0..100`: se ci cadesse dentro per caso, lo
	// scenario sarebbe ACCETTATO con una soglia diversa da quella scritta nel file. Silenziosamente, che e'
	// il modo di fallire contro cui questo intero file esiste.
	Rifiuta(TEXT("soglia fuori dalla scala di int32"),
		TEXT(R"({ "unit": "V1", "reaction": "Hero.Wraith.Deflection",
		          "condition": { "id": "TargetHealthAtOrBelowPercent", "param": 1e20 } })"),
		TEXT("fuori scala"));

	// I due campi si chiedono ENTRAMBI: un default silenzioso su `param` significherebbe soglia 0, cioe' una
	// condizione mai vera — il fuoco spento invece che ristretto.
	Rifiuta(TEXT("condizione senza param"),
		TEXT(R"({ "unit": "V1", "reaction": "Hero.Wraith.Deflection",
		          "condition": { "id": "TargetHealthAtOrBelowPercent" } })"),
		TEXT("param"));
	Rifiuta(TEXT("condizione senza id"),
		TEXT(R"({ "unit": "V1", "reaction": "Hero.Wraith.Deflection", "condition": { "param": 10 } })"),
		TEXT("id"));

	// 🔴 **Il difetto che questo test ha SCOPERTO, e che non riguarda solo la condizione.** Il blocco che
	// valida l'abilita' fa `continue` per le azioni di Prep che risolvono su chi le usa — `Action.Overwatch`
	// e' una di quelle — e un `continue` salta il RESTO del corpo del ciclo. Finche' la validazione della
	// `reaction` e' stata piu' in basso, un intent che armava l'Overwatch poteva dichiarare una reazione
	// INESISTENTE e passare in silenzio: esattamente il modo di fallire che il commento accanto a quella
	// validazione dichiara di voler impedire. Questa riga lo pinna, e cade se qualcuno riordina i blocchi.
	Rifiuta(TEXT("reazione inesistente insieme a un'azione di Prep"),
		TEXT(R"({ "unit": "V1", "ability": "Action.Overwatch", "reaction": "Hero.Wraith.NonEsiste" })"),
		TEXT("NonEsiste"));
	return true;
}

/**
 * Le chiavi che cominciano per `_` sono COMMENTI anche dentro `expect`.
 *
 * ⚠️ **Oggi questo e' vero per ASSENZA di controllo, non per una regola**, ed e' la ragione per cui il test
 * esiste: turni, decisioni e intent hanno ciascuno un elenco chiuso di chiavi note che ammette il prefisso
 * `_` esplicitamente; `expect` no. `Spec.Overwatch.ConditionCollapsesToHold` mette la spiegazione di ogni
 * assertion accanto all'assertion — dove serve a chi la modifichera' — e senza questa riga il giorno in cui
 * qualcuno aggiungesse il quarto elenco quel file si romperebbe con «chiave sconosciuta: _assertion»,
 * lontano da dove ha inserito la regola. Il test trasforma una convenzione accidentale in una garantita.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderExpectCommentKeysTest,
	"RefactorTactics.Scenario.LoaderTreatsUnderscoreKeysAsCommentsInExpect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderExpectCommentKeysTest::RunTest(const FString&)
{
	const FString Json = TEXT(R"JSON(
	{
	  "scenarioId": "Spec.Expect.Comments", "version": 2, "mapRadius": 3,
	  "units": [ { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-2, 0, 0] } ],
	  "turns": [ { "intents": [ { "unit": "A1", "move": [[-1, 0, 0]] } ] } ],
	  "expect": [
	    { "_assertion": "perche' questa assertion esiste", "type": "UnitAtCell", "unit": "A1", "cell": [-1, 0, 0] },
	    { "type": "TurnsCompleted", "value": 1 }
	  ]
	}
	)JSON");

	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("un commento dentro un'assertion non la invalida"),
		URTScenarioLoader::LoadFromString(*Json, Scenario, Error)))
	{
		AddError(FString::Printf(TEXT("motivo del rifiuto: %s"), *Error));
		return false;
	}
	// E il commento non diventa un'assertion in piu': due voci scritte, due lette.
	TestEqual(TEXT("le assertion restano due"), Scenario.Expect.Num(), 2);
	return true;
}

/**
 * Il parsing dei due capi di un redirect, e il suo rifiuto (#1060).
 *
 * ⚠️ La meta' che conta e' la **seconda**: `unit` mancante deve essere un errore, non un id vuoto accettato
 * in silenzio. Un'assertion che confronta una stringa vuota fallisce (o peggio passa) per un motivo che non
 * e' quello scritto nello scenario — ed e' lo stesso argomento con cui `UnitHpEquals` rifiuta un `value`
 * mancante invece di indovinare `0`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderRedirectExpectTest,
	"RefactorTactics.Scenario.LoaderParsesRedirectExpectations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderRedirectExpectTest::RunTest(const FString&)
{
	const FString Ok = TEXT(R"JSON(
	{
	  "scenarioId": "Spec.Expect.Redirect", "version": 2, "mapRadius": 3,
	  "units": [ { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-2, 0, 0] } ],
	  "turns": [ { "intents": [ { "unit": "A1", "move": [[-1, 0, 0]] } ] } ],
	  "expect": [
	    { "type": "OriginalTargetEquals", "unit": "V1" },
	    { "type": "EffectiveTargetEquals", "unit": "B1" }
	  ]
	}
	)JSON");

	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("i due capi del redirect si leggono"),
		URTScenarioLoader::LoadFromString(*Ok, Scenario, Error)))
	{
		AddError(FString::Printf(TEXT("motivo del rifiuto: %s"), *Error));
		return false;
	}
	if (TestEqual(TEXT("due assertion lette"), Scenario.Expect.Num(), 2))
	{
		TestTrue(TEXT("la prima e' OriginalTargetEquals su V1"),
			Scenario.Expect[0].Kind == ERTAssertionKind::OriginalTargetEquals
			&& Scenario.Expect[0].UnitId == TEXT("V1"));
		TestTrue(TEXT("la seconda e' EffectiveTargetEquals su B1"),
			Scenario.Expect[1].Kind == ERTAssertionKind::EffectiveTargetEquals
			&& Scenario.Expect[1].UnitId == TEXT("B1"));
	}

	// Senza `unit` non c'e' niente da confrontare: si rifiuta invece di accettare un id vuoto.
	const FString NoUnit = TEXT(R"JSON(
	{
	  "scenarioId": "Spec.Expect.RedirectNoUnit", "version": 2, "mapRadius": 3,
	  "units": [ { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-2, 0, 0] } ],
	  "turns": [ { "intents": [ { "unit": "A1", "move": [[-1, 0, 0]] } ] } ],
	  "expect": [ { "type": "EffectiveTargetEquals" } ]
	}
	)JSON");

	FRTTestScenario Rejected;
	FString RejectError;
	TestFalse(TEXT("senza `unit` l'assertion non passa la lettura"),
		URTScenarioLoader::LoadFromString(*NoUnit, Rejected, RejectError));
	TestTrue(TEXT("e il messaggio nomina il campo mancante"),
		RejectError.Contains(TEXT("unit")));
	return true;
}

/**
 * **IL VINCOLO SULLE SQUADRE E' DERIVATO, NON RISCRITTO** (#1515).
 *
 * L'AC chiedeva che l'intervallo venisse da *dove le squadre sono definite nel runtime*, e non da una
 * costante `0..1` scritta nel loader. Questo test lo verifica **dal lato che conta**: non che il numero sia
 * 2, ma che sia LO STESSO che decide chi vince.
 *
 * ⛔ **Perche' non `FRTKnowledgeVerdict::MaxTeamId`**, il candidato ovvio: vale **31** ed e' la capacita' di una
 * bitmask di percezione — quanti indici quella maschera sa rappresentare, non quante squadre giocano.
 * Derivare il vincolo da li' ammetterebbe `"team": 7`, cioe' esattamente il caso rotto che questa issue
 * descrive. La riga qui sotto lo rende falsificabile invece di lasciarlo in un commento.
 *
 * ⚠️ **E non e' un vincolo di FORMATO**: 3v3 e 4v4 muovono le unita' per squadra (`UnitsPerTeam`), non il
 * numero di squadre. Il formato competitivo resta aperto in `OPEN_DECISIONS` e questa riga non lo tocca.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioTeamConstraintIsDerivedTest,
	"RefactorTactics.Scenario.TeamConstraintComesFromTheRuntime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioTeamConstraintIsDerivedTest::RunTest(const FString&)
{
	// L'autorita': `URTTurnRules` e' la libreria che decide l'esito, e le sue firme conoscono due squadre.
	TestEqual(TEXT("il runtime dichiara due squadre"), URTTurnRules::NumTeams, 2);
	TestTrue(TEXT("la squadra 0 e' valida"), URTTurnRules::IsValidTeamId(0));
	TestTrue(TEXT("la squadra 1 e' valida"), URTTurnRules::IsValidTeamId(1));
	TestFalse(TEXT("la squadra 2 non lo e'"), URTTurnRules::IsValidTeamId(2));
	TestFalse(TEXT("ne' la -1"), URTTurnRules::IsValidTeamId(-1));

	// 🔴 **Il legame che rende la derivazione una derivazione**: la prima squadra che il vincolo rifiuta e'
	// anche la prima per cui `EvaluateOutcome` non ha un caso. Se qualcuno introducesse una terza squadra
	// nel runtime senza toccare questa costante, il loader continuerebbe a rifiutarla e questa riga direbbe
	// dove guardare.
	TestFalse(FString::Printf(
		TEXT("la prima squadra fuori intervallo (%d) e' fuori anche per chi decide il vincitore"),
		URTTurnRules::NumTeams), URTTurnRules::IsValidTeamId(URTTurnRules::NumTeams));

	// ⛔ La bitmask di percezione dichiara un massimo MOLTO piu' largo: e' la prova che i due numeri
	// rispondono a domande diverse, e che sceglierli l'uno per l'altro non e' un dettaglio.
	TestTrue(FString::Printf(
		TEXT("la bitmask di percezione ammette molto piu' delle squadre giocanti (%d contro %d)"),
		FRTKnowledgeVerdict::MaxTeamId, URTTurnRules::NumTeams - 1),
		FRTKnowledgeVerdict::MaxTeamId > URTTurnRules::NumTeams - 1);

	// E il messaggio d'errore nomina l'unita' E il valore, come chiede il DoD.
	FRTTestScenario Scenario;
	FString Error;
	const bool bOk = URTScenarioLoader::LoadFromString(
		TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"Ribelle","hero":"Hero.Gadget","team":7,"cell":[0,0,0]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		Scenario, Error);
	TestFalse(TEXT("lo scenario con la squadra 7 e' rifiutato"), bOk);
	TestTrue(FString::Printf(TEXT("l'errore nomina l'unita' (era: '%s')"), *Error),
		Error.Contains(TEXT("Ribelle")));
	TestTrue(FString::Printf(TEXT("e nomina il valore (era: '%s')"), *Error),
		Error.Contains(TEXT("7")));

	return true;
}

/**
 * #1796 — una cella su un layer che l'arena NON ha viene rifiutata, e l'errore dice quale layer.
 *
 * 🔴 **Il difetto che chiude, e come e' stato trovato.** Il controllo di appartenenza era
 * `HexDistance(Cella, FRTCellId(0, 0, Cella.Layer)) > MapRadius`: l'origine costruita **sul layer della
 * cella stessa**, quindi il raggio misurato dentro quel piano e la domanda «quel piano esiste?» mai posta.
 * Su un'arena piatta — che `MakeFlatArena` costruisce sul solo layer 0 — ogni layer diverso passava.
 *
 * Non e' teorico: `Spec.Hud.MatchWithTwoAllies` piazzava un'unita' su `L=-1` e girava, e i tre test HUD che
 * lo consumano guardano il roster, non le posizioni. L'ha trovato un test sul corpus.
 *
 * ⛔ **La correzione OVVIA e sbagliata sarebbe «rifiuta i layer negativi»**, perche' il caso reale portava
 * `-1`. Il penultimo blocco qui sotto la boccia: un `L=+7` sulla stessa arena piatta e' fuori mappa
 * esattamente quanto un `-1`, e una regola sul segno lo lascerebbe entrare.
 *
 * ⛔ **La seconda correzione ovvia e sbagliata sarebbe «rifiuta ogni layer != 0»**, e l'ultimo blocco la
 * boccia: con una FIXTURE l'arena puo' essere multilivello, ed e' la ragione per cui `FRTCellId` porta un
 * layer. La regola distingue «layer che l'arena non ha» da «layer diverso da zero».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderRejectsLayerOutsideFlatArenaTest,
	"RefactorTactics.Scenario.LoaderRejectsLayerOutsideFlatArena",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderRejectsLayerOutsideFlatArenaTest::RunTest(const FString&)
{
	auto Rejects = [this](const TCHAR* Json, const TCHAR* MustMention, const TCHAR* What)
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = URTScenarioLoader::LoadFromString(Json, Scenario, Error);
		TestFalse(FString::Printf(TEXT("%s: rifiutato"), What), bOk);
		TestTrue(FString::Printf(TEXT("%s: il motivo cita '%s' (era: '%s')"), What, MustMention, *Error),
			Error.Contains(MustMention));
	};

	auto Accepts = [this](const TCHAR* Json, const TCHAR* What)
	{
		FRTTestScenario Scenario;
		FString Error;
		TestTrue(FString::Printf(TEXT("%s: accettato (errore: '%s')"), What, *Error),
			URTScenarioLoader::LoadFromString(Json, Scenario, Error));
	};

	// --- 1. UNITA' su un layer che l'arena piatta non ha -------------------------------------------
	// E' il caso reale, con lo stesso `L=-1` e la stessa coordinata planare VALIDA: `(-2,1)` sta dentro il
	// raggio 3. Prima passava, ed e' il motivo per cui il messaggio deve nominare il layer invece di dire
	// solo «fuori dall'arena» — su una planare giusta, quel messaggio manda a cercare il raggio.
	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Gadget","team":0,"cell":[-2,1,-1]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("layer -1"), TEXT("unita' su un layer inesistente"));

	// --- 2. CELLA MODIFICATA sullo stesso difetto --------------------------------------------------
	// Un ostacolo posato fuori mappa non blocca niente, e lo scenario verificherebbe una condizione che non
	// ha mai creato.
	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Gadget","team":0,"cell":[0,0,0]}],"cells":[{"cell":[1,0,2],"blocksLineOfSight":true}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("layer 2"), TEXT("cella modificata su un layer inesistente"));

	// --- 3. Il segno NON e' la regola --------------------------------------------------------------
	// 🔴 Il blocco che boccia «rifiuta i layer negativi»: `+7` e' fuori mappa quanto `-1`.
	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Gadget","team":0,"cell":[0,0,7]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("layer 7"), TEXT("unita' su un layer POSITIVO inesistente"));

	// --- 4. Il layer 0 continua a passare, e il raggio resta la sua regola --------------------------
	Accepts(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Gadget","team":0,"cell":[-2,1,0]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("unita' sul layer 0, dentro il raggio"));

	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":2,"units":[{"id":"A","hero":"Hero.Gadget","team":0,"cell":[9,0,0]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("distanza"), TEXT("layer giusto ma fuori raggio: il motivo cambia"));

	// --- 5. ⛔ Con una FIXTURE il multilivello resta legale -----------------------------------------
	// 🔴 Il blocco che boccia «rifiuta ogni layer != 0». Con una fixture la forma non e' un raggio, e
	// l'arena PUO' avere piu' piani: e' la ragione per cui `FRTCellId` porta un layer. Qui il loader non
	// decide — la cella la verifica `FRTScenarioSession` sulla mappa vera.
	Accepts(TEXT(R"({"scenarioId":"X","fixture":"TestArena","units":[{"id":"A","hero":"Hero.Gadget","team":0,"cell":[0,0,1]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("fixture multilivello: il layer 1 non e' del loader"));

	return true;
}

/**
 * UN TAG SCONOSCIUTO RIFIUTA LO SCENARIO, E IL MESSAGGIO LO NOMINA — `#1629`.
 *
 * 🔴 `FGameplayTag::RequestGameplayTag` con `ErrorIfNotFound=false` torna un tag **vuoto** senza
 * lamentarsi, e un tag vuoto applicato non fa niente: senza questo rifiuto uno scenario con
 * `"Status.Guarded1"` girerebbe **verde**, verificando l'assenza di un effetto che nessuno ha chiesto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioUnknownStatusIsRefusedTest,
	"RefactorTactics.Scenario.UnknownStatusTagIsRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioUnknownStatusIsRefusedTest::RunTest(const FString&)
{
	auto Load = [](const TCHAR* Json, FRTTestScenario& Out, FString& Err)
	{
		return URTScenarioLoader::LoadFromString(Json, Out, Err);
	};

	FRTTestScenario Scenario;
	FString Error;

	const bool bBad = Load(TEXT(R"JSON({
	  "scenarioId": "T", "version": 1, "mapRadius": 2,
	  "units": [
	    { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-1,0,0],
	      "statuses": [ { "tag": "Status.Guarded1", "turns": 2 } ] },
	    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [1,0,0] }
	  ],
	  "turns": [ { "intents": [] } ],
	  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
	})JSON"), Scenario, Error);

	TestFalse(TEXT("un tag sconosciuto rifiuta"), bBad);
	TestTrue(FString::Printf(TEXT("e il messaggio lo nomina (era: '%s')"), *Error),
		Error.Contains(TEXT("Status.Guarded1")));

	// Uno status che dura zero turni non e' uno status.
	FRTTestScenario Zero;
	FString ZeroError;
	const bool bZero = Load(TEXT(R"JSON({
	  "scenarioId": "T", "version": 1, "mapRadius": 2,
	  "units": [
	    { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-1,0,0],
	      "statuses": [ { "tag": "Status.Wet", "turns": 0 } ] },
	    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [1,0,0] }
	  ],
	  "turns": [ { "intents": [] } ],
	  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
	})JSON"), Zero, ZeroError);
	TestFalse(TEXT("turns a zero rifiuta"), bZero);

	// ✅ LA CONTROPROVA: un tag VALIDO si accetta. Senza, i due rifiuti sopra sarebbero compatibili con un
	// loader che rifiuta sempre.
	FRTTestScenario Good;
	FString GoodError;
	const bool bGood = Load(TEXT(R"JSON({
	  "scenarioId": "T", "version": 1, "mapRadius": 2,
	  "units": [
	    { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-1,0,0],
	      "statuses": [ { "tag": "Status.Wet", "turns": 2 } ] },
	    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [1,0,0] }
	  ],
	  "turns": [ { "intents": [] } ],
	  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
	})JSON"), Good, GoodError);
	TestTrue(FString::Printf(TEXT("un tag valido si accetta (errore: '%s')"), *GoodError), bGood);

	return true;
}

/**
 * IL VOCABOLARIO DEL JSON E' QUELLO DEL RUNTIME, E NON CE N'E' UN SECONDO — `#1629`.
 *
 * 🔑 È il quarto criterio d'accettazione scritto come test invece che come `git grep`: ogni tag che il
 * loader accetta **deve** essere uno di quelli che `Core/RTGameplayTags.cpp` dichiara. Se qualcuno
 * aggiungesse una tabella di nomi per il formato scenario, questo test la troverebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioStatusVocabularyIsTheRuntimeOneTest,
	"RefactorTactics.Scenario.StatusVocabularyHasNoSecondTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioStatusVocabularyIsTheRuntimeOneTest::RunTest(const FString&)
{
	// Gli undici status che il runtime dichiara. Se il loader ne accettasse uno che non è qui, o ne
	// rifiutasse uno che c'è, i due vocabolari sarebbero due.
	const TCHAR* Known[] = {
		TEXT("Status.Braced"), TEXT("Status.Burning"), TEXT("Status.Electrified"),
		TEXT("Status.Exposed"), TEXT("Status.Guarded"), TEXT("Status.Marked"),
		TEXT("Status.Obscured"), TEXT("Status.Reveal"), TEXT("Status.Root"),
		TEXT("Status.Slow"), TEXT("Status.Wet")
	};

	for (const TCHAR* Name : Known)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(Name), /*ErrorIfNotFound=*/ false);
		TestTrue(FString::Printf(TEXT("'%s' e' un tag del runtime"), Name), Tag.IsValid());

		const FString Json = FString::Printf(TEXT(R"JSON({
		  "scenarioId": "T", "version": 1, "mapRadius": 2,
		  "units": [
		    { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-1,0,0],
		      "statuses": [ { "tag": "%s", "turns": 1 } ] },
		    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [1,0,0] }
		  ],
		  "turns": [ { "intents": [] } ],
		  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
		})JSON"), Name);

		FRTTestScenario Scenario;
		FString Error;
		if (!TestTrue(FString::Printf(TEXT("e il loader lo accetta: %s (%s)"), Name, *Error),
			URTScenarioLoader::LoadFromString(*Json, Scenario, Error)))
		{
			continue;
		}

		// ⛔ **E lo CARICA davvero.** Senza questa riga il test sarebbe vacuo: passerebbe anche se il
		// loader ignorasse il campo `statuses`, perche' un file che si accetta e un campo che arriva sono
		// due cose diverse. Misurato: e' esattamente cosi' che questo test e' passato al primo giro,
		// mentre gli altri due cadevano perche' il blocco del loader era finito dentro un ramo opzionale.
		if (TestEqual(FString::Printf(TEXT("%s: uno status caricato"), Name),
			Scenario.Units[0].Statuses.Num(), 1))
		{
			TestEqual(FString::Printf(TEXT("%s: col nome giusto"), Name),
				Scenario.Units[0].Statuses[0].Tag, FName(Name));
		}
	}

	return true;
}

/**
 * UN'ABILITA' D'EROE CHE RISOLVE SU CHI LA USA E' ESPRIMIBILE — `#2283`.
 *
 * 🔑 **E' la meta' mancante del criterio di `EveryD025ActionIsExpressible` qui sotto**: quello copre le
 * sette azioni GENERICHE, questo le azioni d'EROE che risolvono in Preparation su chi le arma. Fino al
 * 2026-09-04 quella meta' non era esprimibile affatto, e il buco non aveva sintomi — si vedeva solo
 * provando a scrivere lo scenario.
 *
 * ⛔ **Le due porte erano entrambe chiuse.** Senza `target`: *«l'abilita' non dichiara un bersaglio»*,
 * perche' `bResolvesOnSelf` interrogava il solo catalogo CORE e un'azione d'eroe non c'e'. Con
 * `"target"` su se stessa: *«l'unita' bersaglia se stessa»*, dalla guardia che il suo stesso commento
 * diceva di rilassare *«di PROPOSITO»* il giorno in cui un'abilita' del genere fosse esistita. Quel
 * giorno e' il 2026-08-28 (`D-226`, `Action.Shield` prende due portatori) e nessuno l'ha preso.
 *
 * ⚠️ **La correzione non rilassa la guardia sull'auto-bersaglio**, che resta giusta: un'azione self
 * semplicemente non dichiara bersaglio, quindi non ci ricade. Il test lo pinna sotto, al punto 3.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioHeroSelfActionTest,
	"RefactorTactics.Scenario.HeroActionOnSelfIsExpressible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioHeroSelfActionTest::RunTest(const FString&)
{
	auto Load = [](const FString& Intents, FRTTestScenario& OutScenario, FString& OutError) -> bool
	{
		const FString Json = FString::Printf(TEXT(R"JSON({
		  "scenarioId": "T", "version": 1, "mapRadius": 3,
		  "units": [
		    { "id": "P1", "hero": "Hero.Phase", "team": 0, "cell": [-1,0,0] },
		    { "id": "R1", "hero": "Hero.Riktor", "team": 1, "cell": [1,0,0] }
		  ],
		  "turns": [ { "intents": [ %s ] } ],
		  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
		})JSON"), *Intents);
		return URTScenarioLoader::LoadFromString(*Json, OutScenario, OutError);
	};

	// 1. Il caso: `Hero.Phase.TideGuard` deriva da `Action.Shield`, che nel catalogo core e' `Preparation`.
	//    Risolve su chi la arma, quindi non ha un bersaglio da dichiarare.
	{
		FRTTestScenario S;
		FString Error;
		const bool bLoaded = Load(TEXT("{ \"unit\": \"P1\", \"ability\": \"Hero.Phase.TideGuard\" }"), S, Error);
		if (!TestTrue(FString::Printf(TEXT("TideGuard senza bersaglio si esprime (errore: '%s')"), *Error), bLoaded))
		{
			return false;
		}
		if (TestTrue(TEXT("il turno porta l'intento"), S.Turns.Num() > 0 && S.Turns[0].Intents.Num() > 0))
		{
			TestEqual(TEXT("con l'abilita' giusta"), S.Turns[0].Intents[0].Ability, FName(TEXT("Hero.Phase.TideGuard")));
			TestTrue(TEXT("e nessun bersaglio dichiarato"), S.Turns[0].Intents[0].Target.IsEmpty());
		}
	}

	// 2. ANTI-VACUITA': un'azione d'eroe che NON risolve su se stessa deve restare rifiutata senza bersaglio.
	//    Senza questo, il punto 1 passerebbe anche se la correzione avesse semplicemente spento il controllo.
	{
		FRTTestScenario S;
		FString Error;
		const bool bLoaded = Load(TEXT("{ \"unit\": \"P1\", \"ability\": \"Hero.Phase.PressureJet\" }"), S, Error);
		TestFalse(TEXT("un attacco d'eroe senza bersaglio resta rifiutato"), bLoaded);
		TestTrue(TEXT("e il motivo lo nomina"), Error.Contains(TEXT("bersaglio")));
	}

	// 3. La guardia sull'auto-bersaglio NON e' stata rilassata: dichiararlo esplicitamente resta un errore.
	{
		FRTTestScenario S;
		FString Error;
		const bool bLoaded = Load(TEXT("{ \"unit\": \"P1\", \"ability\": \"Hero.Phase.TideGuard\", \"target\": \"P1\" }"), S, Error);
		TestFalse(TEXT("bersagliare se stessi resta un errore"), bLoaded);
		TestTrue(TEXT("e il motivo lo dice"), Error.Contains(TEXT("se stessa")));
	}

	return true;
}

/**
 * LE SETTE AZIONI DI `D-025` SONO ESPRIMIBILI, E IL TEST DICE COME — `#1626`.
 *
 * 🔑 **È il criterio di copertura scritto come misura invece che come dichiarazione.** La issue chiede che
 * per ciascuna si dica *«se è esprimibile e come»*: qui ciascuna è espressa davvero, e il file prodotto
 * deve rileggersi identico. Una risposta scritta in prosa invecchia; questa cade.
 *
 * ⚠️ **`Wait` non ha un campo, e non è una lacuna**: si esprime OMETTENDO l'unità dagli intent del turno.
 * `ApplyScenarioIntents` cicla sugli intent dichiarati, quindi un'unità che non compare non riceve ordini
 * — che è precisamente l'attesa. Misurato: zero scenari usano `"Action.Wait"`, e zero intent portano il
 * solo campo `unit`.
 *
 * ⛔ La forma di `Move` è il campo `move[]`, **non** `ability: "Action.Move"`: 48 scenari la usano così, e
 * nessuno nell'altro modo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioD025CoverageTest,
	"RefactorTactics.Scenario.EveryD025ActionIsExpressible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioD025CoverageTest::RunTest(const FString&)
{
	auto Loads = [this](const FString& Intents, const TCHAR* What) -> FRTTestScenario
	{
		const FString Json = FString::Printf(TEXT(R"JSON({
		  "scenarioId": "T", "version": 1, "mapRadius": 3,
		  "units": [
		    { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-1,0,0] },
		    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [1,0,0] }
		  ],
		  "turns": [ { "intents": [ %s ] } ],
		  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
		})JSON"), *Intents);

		FRTTestScenario Scenario;
		FString Error;
		TestTrue(FString::Printf(TEXT("%s si esprime (errore: '%s')"), What, *Error),
			URTScenarioLoader::LoadFromString(*Json, Scenario, Error));
		return Scenario;
	};

	// 1. WAIT — l'unita' non compare fra gli intent. Nessun campo, e non e' un buco del formato.
	{
		const FRTTestScenario S = Loads(TEXT("{ \"unit\": \"B1\", \"ability\": \"Action.Guard\" }"), TEXT("Wait (A1 omessa)"));
		bool bA1Present = false;
		if (S.Turns.Num() > 0)
		{
			for (const FRTScenarioIntent& I : S.Turns[0].Intents)
			{
				if (I.UnitId == TEXT("A1")) { bA1Present = true; }
			}
		}
		TestFalse(TEXT("A1 non ha ordini: e' l'attesa"), bA1Present);
	}

	// 2. MOVE — il campo `move[]`, non un'abilita'.
	{
		const FRTTestScenario S = Loads(TEXT("{ \"unit\": \"A1\", \"move\": [[0,0,0]] }"), TEXT("Move"));
		if (S.Turns.Num() > 0 && S.Turns[0].Intents.Num() > 0)
		{
			TestEqual(TEXT("Move usa il percorso"), S.Turns[0].Intents[0].Move.Num(), 1);
			TestTrue(TEXT("e nessuna abilita'"), S.Turns[0].Intents[0].Ability.IsNone());
		}
	}

	// 3-4. GUARD e BRACE: `ability` e basta. Non hanno bersaglio, ed e' cio' che le distingue.
	const TCHAR* Senza[] = { TEXT("Action.Guard"), TEXT("Action.Brace") };
	for (const TCHAR* Action : Senza)
	{
		const FString Intent = FString::Printf(TEXT("{ \"unit\": \"A1\", \"ability\": \"%s\" }"), Action);
		const FRTTestScenario S = Loads(Intent, Action);
		if (S.Turns.Num() > 0 && S.Turns[0].Intents.Num() > 0)
		{
			TestEqual(FString::Printf(TEXT("%s arriva nel campo ability"), Action),
				S.Turns[0].Intents[0].Ability, FName(Action));
			TestTrue(FString::Printf(TEXT("%s non ha bersaglio"), Action),
				S.Turns[0].Intents[0].Target.IsEmpty());
		}
	}

	// 5-6. BASICATTACK e INTERACT: `ability` **con** un bersaglio, e il loader lo ESIGE.
	//
	// 🔑 Misurato, non assunto: senza `target` il caricamento fallisce con *«l'abilita' 'X' non dichiara un
	// bersaglio (campo target)»*. La copertura di un'azione non e' «il campo esiste», e' «l'azione si
	// esprime con i suoi vincoli» — e questi vincoli il criterio 5 li deve riportare.
	const TCHAR* ConBersaglio[] = { TEXT("Action.BasicAttack"), TEXT("Action.Interact") };
	for (const TCHAR* Action : ConBersaglio)
	{
		const FString Intent = FString::Printf(
			TEXT("{ \"unit\": \"A1\", \"ability\": \"%s\", \"target\": \"B1\" }"), Action);
		const FRTTestScenario S = Loads(Intent, Action);
		if (S.Turns.Num() > 0 && S.Turns[0].Intents.Num() > 0)
		{
			TestEqual(FString::Printf(TEXT("%s arriva nel campo ability"), Action),
				S.Turns[0].Intents[0].Ability, FName(Action));
			TestEqual(FString::Printf(TEXT("%s porta il proprio bersaglio"), Action),
				S.Turns[0].Intents[0].Target, FString(TEXT("B1")));
		}

		// ⛔ E senza bersaglio non si esprime: il rifiuto e' parte della copertura.
		const FString Nudo = FString::Printf(TEXT("{ \"unit\": \"A1\", \"ability\": \"%s\" }"), Action);
		FRTTestScenario Rifiutato;
		FString Errore;
		const FString Json = FString::Printf(TEXT(R"JSON({
		  "scenarioId": "T", "version": 1, "mapRadius": 3,
		  "units": [
		    { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-1,0,0] },
		    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [1,0,0] }
		  ],
		  "turns": [ { "intents": [ %s ] } ],
		  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
		})JSON"), *Nudo);
		TestFalse(FString::Printf(TEXT("%s senza bersaglio e' rifiutata"), Action),
			URTScenarioLoader::LoadFromString(*Json, Rifiutato, Errore));
	}

	// 7. OVERWATCH — si arma con `ability`, e **non** con `reaction`.
	//
	// 🔴 L'assunzione naturale e' sbagliata, ed e' misurata: `Scenarios/Spec/Overwatch/` lo arma con
	// `ability`, e il campo `reaction` porta un'abilita' reattiva DIVERSA (p.es. `Hero.Wraith.Deflection`).
	// La ragione sta scritta in quello scenario: l'Overwatch costa l'azione **principale**, e
	// `SetPlannedReactionCondition` rifiuta se `PlannedReactionAbility` e' `INDEX_NONE` — quindi un piano
	// di solo-Overwatch non riesce a dichiarare una condizione, e il campo resterebbe vuoto senza dirlo.
	// E' un limite noto (`RTTurnManager.cpp`, ramo `Action.Overwatch`), non un difetto di questa slice.
	{
		const FRTTestScenario S = Loads(TEXT("{ \"unit\": \"A1\", \"ability\": \"Action.Overwatch\" }"),
			TEXT("Overwatch"));
		if (S.Turns.Num() > 0 && S.Turns[0].Intents.Num() > 0)
		{
			TestEqual(TEXT("Overwatch occupa lo slot PRINCIPALE"),
				S.Turns[0].Intents[0].Ability, FName(TEXT("Action.Overwatch")));
			TestTrue(TEXT("e non quello reattivo"), S.Turns[0].Intents[0].Reaction.IsNone());
		}
	}

	// ⛔ E IL CASO CHE LA UI DEVE POTER ESPRIMERE: movimento e slot principale INSIEME, che il doc header
	// della struct dichiara essere «la norma, non un caso limite».
	{
		const FRTTestScenario S = Loads(
			TEXT("{ \"unit\": \"A1\", \"move\": [[0,0,0]], \"ability\": \"Action.BasicAttack\", \"target\": \"B1\" }"),
			TEXT("movimento e azione insieme"));
		if (S.Turns.Num() > 0 && S.Turns[0].Intents.Num() > 0)
		{
			const FRTScenarioIntent& I = S.Turns[0].Intents[0];
			TestTrue(TEXT("lo slot movimento e' pieno"), I.Move.Num() > 0);
			TestFalse(TEXT("e anche quello principale"), I.Ability.IsNone());
		}
	}

	return true;
}

/**
 * UN BERSAGLIO SI DICE IN DUE MODI, E DICHIARARLI ENTRAMBI E' UN ERRORE — `#1626`.
 *
 * 🔑 «Le due forme non si contraddicono nel file prodotto» ha bisogno di una definizione di
 * contraddizione, e questa è: `bTargetsCell` vero **con** un `Target` non vuoto — due bersagli dichiarati,
 * e il runtime ne sceglierebbe uno senza che il file dica quale.
 *
 * ✅ Il loader lo rifiuta già; questo test lo rende un gate invece di una constatazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioTargetFormsTest,
	"RefactorTactics.Scenario.TargetIsUnitOrCellButNeverBoth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioTargetFormsTest::RunTest(const FString&)
{
	auto Try = [](const TCHAR* Intent, FRTTestScenario& Out, FString& Err)
	{
		const FString Json = FString::Printf(TEXT(R"JSON({
		  "scenarioId": "T", "version": 1, "mapRadius": 3,
		  "units": [
		    { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-1,0,0] },
		    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [1,0,0] }
		  ],
		  "turns": [ { "intents": [ %s ] } ],
		  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
		})JSON"), Intent);
		return URTScenarioLoader::LoadFromString(*Json, Out, Err);
	};

	// Per UNITA'.
	{
		FRTTestScenario S; FString E;
		if (TestTrue(FString::Printf(TEXT("bersaglio per unita' (%s)"), *E),
			Try(TEXT("{ \"unit\": \"A1\", \"ability\": \"Action.BasicAttack\", \"target\": \"B1\" }"), S, E)))
		{
			const FRTScenarioIntent& I = S.Turns[0].Intents[0];
			TestEqual(TEXT("il bersaglio e' l'unita'"), I.Target, FString(TEXT("B1")));
			TestFalse(TEXT("e non e' una cella"), I.bTargetsCell);
		}
	}

	// Per CELLA.
	{
		FRTTestScenario S; FString E;
		if (TestTrue(FString::Printf(TEXT("bersaglio per cella (%s)"), *E),
			Try(TEXT("{ \"unit\": \"A1\", \"ability\": \"Action.BasicAttack\", \"targetCell\": [1,0,0] }"), S, E)))
		{
			const FRTScenarioIntent& I = S.Turns[0].Intents[0];
			TestTrue(TEXT("il flag della cella e' acceso"), I.bTargetsCell);
			TestTrue(TEXT("e nessuna unita' e' nominata"), I.Target.IsEmpty());
		}
	}

	// ⛔ ENTRAMBI: rifiutato, e il messaggio nomina il bersaglio in conflitto.
	{
		FRTTestScenario S; FString E;
		const bool bOk = Try(
			TEXT("{ \"unit\": \"A1\", \"ability\": \"Action.BasicAttack\", \"target\": \"B1\", \"targetCell\": [1,0,0] }"),
			S, E);
		TestFalse(TEXT("due bersagli insieme sono rifiutati"), bOk);
		TestTrue(FString::Printf(TEXT("e il messaggio nomina il conflitto (era: '%s')"), *E),
			E.Contains(TEXT("targetCell")) || E.Contains(TEXT("B1")));
	}

	return true;
}

/**
 * NESSUN ENUM D'AZIONE IN `ScenarioHarness/` — `#1626`.
 *
 * 🔑 Il vocabolario delle azioni è `Action.*` nei gameplay tag, e un `enum` parallelo nel formato scenario
 * sarebbe la seconda tabella da tenere allineata alla prima. ⚠️ Il criterio è **già soddisfatto oggi**:
 * questo test lo rende un gate invece di una constatazione, perché un criterio non protetto si perde al
 * primo che arriva.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioNoActionEnumTest,
	"RefactorTactics.Scenario.HarnessHasNoParallelActionEnum",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioNoActionEnumTest::RunTest(const FString&)
{
	const FString Dir = FPaths::ProjectDir() / TEXT("Source/RefactorTactics/ScenarioHarness/");

	TArray<FString> Headers;
	IFileManager::Get().FindFiles(Headers, *(Dir + TEXT("*.h")), /*Files=*/ true, /*Directories=*/ false);
	if (!TestTrue(FString::Printf(TEXT("gli header di ScenarioHarness si leggono (%d)"), Headers.Num()),
		Headers.Num() > 0))
	{
		return false;
	}

	// I nomi che un enum d'azione avrebbe. Non si cerca «enum» in generale: `ScenarioHarness` ne ha di
	// legittimi — l'esito di un test, lo stato di una sessione — e vietarli tutti sarebbe un criterio che
	// boccia il codice sano.
	const TCHAR* Suspicious[] = { TEXT("enum class ERTScenarioAction"), TEXT("enum class ERTAction"),
		TEXT("enum class ERTIntentAction"), TEXT("enum class ERTActionKind") };

	for (const FString& File : Headers)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *(Dir + File)))
		{
			AddError(FString::Printf(TEXT("non ho potuto leggere %s: la misura non vale"), *File));
			continue;
		}
		for (const TCHAR* Name : Suspicious)
		{
			TestFalse(FString::Printf(TEXT("%s non dichiara %s"), *File, Name), Text.Contains(Name));
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
