// Loader degli scenari di test: interpretazione e VALIDAZIONE.
//
// La validazione e' la meta' che conta. Uno scenario che nomina un eroe inesistente deve produrre un ERROR
// leggibile, non un FAIL di gioco: confondere «il test e' scritto male» con «il gioco e' rotto» fa perdere
// piu' tempo di quanto il test ne faccia risparmiare.

#include "Misc/AutomationTest.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "Turn/RTTurnLog.h" // gli esiti che le assertion sul log nominano per nome
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
	    { "id": "A1", "hero": "Hero.Flux", "team": 0, "cell": [-2, 0, 0] },
	    { "id": "B1", "hero": "Hero.Bastion", "team": 1, "cell": [2, 0] }
	  ],
	  "turns": [ { "intents": [ { "unit": "A1", "move": [[-1, 0, 0]] } ] } ],
	  "expect": [ { "type": "UnitAtCell", "unit": "A1", "cell": [-1, 0, 0] } ]
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
	TestEqual(TEXT("A1 e' Flux"), A1->HeroId, FName(TEXT("Hero.Flux")));
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

	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":2,"units":[{"id":"A","hero":"Hero.Flux","team":0,"cell":[9,0,0]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("fuori dall'arena"), TEXT("cella fuori mappa"));

	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Flux","team":0,"cell":[0,0,0]},{"id":"A","hero":"Hero.Riva","team":1,"cell":[1,0,0]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("duplicato"), TEXT("id unita' duplicato"));

	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Flux","team":0,"cell":[0,0,0]},{"id":"B","hero":"Hero.Riva","team":1,"cell":[0,0,0]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("stessa cella"), TEXT("due unita' sovrapposte alla partenza"));

	// Uno scenario senza assertion passerebbe sempre: e' un test che non testa.
	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Flux","team":0,"cell":[0,0,0]}]})"),
		TEXT("nessuna assertion"), TEXT("scenario senza expect"));

	// Un'assertion scritta male non deve essere IGNORATA: il test sembrerebbe passare.
	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Flux","team":0,"cell":[0,0,0]}],"expect":[{"type":"UnitHasSuperpowers"}]})"),
		TEXT("assertion sconosciuta"), TEXT("assertion non riconosciuta"));

	// Intent su un'unita' mai schierata: errore di scrittura tipico, va colto subito.
	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Flux","team":0,"cell":[0,0,0]}],"turns":[{"intents":[{"unit":"Z","move":[[1,0,0]]}]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("non schierata"), TEXT("intent su unita' inesistente"));

	// Un formato piu' nuovo di quanto il loader sappia leggere non va interpretato a caso.
	Rejects(TEXT(R"({"scenarioId":"X","version":99,"mapRadius":3,"units":[{"id":"A","hero":"Hero.Flux","team":0,"cell":[0,0,0]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("non supportato"), TEXT("versione di formato futura"));

	// Chiave di INTENT sconosciuta (CP 16.1). Prima veniva ignorata in silenzio, e uno scenario che chiedeva
	// qualcosa che l'harness non sa fare girava verde verificando tutto tranne quello. Vale anche per un refuso
	// su una chiave vera: `dashCell` al posto di `dashTo` e' l'errore che questo controllo coglie per primo.
	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Flux","team":0,"cell":[0,0,0]}],"turns":[{"intents":[{"unit":"A","facing":"NE"}]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("chiave sconosciuta"), TEXT("chiave di intent inventata"));

	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Bastion","team":0,"cell":[0,0,0]}],"turns":[{"intents":[{"unit":"A","dash":"Bastion.Ram","dashCell":[1,0,0]}]}],"expect":[{"type":"TurnsCompleted","value":1}]})"),
		TEXT("chiave sconosciuta"), TEXT("refuso su una chiave vera"));

	// Una direzione inventata in UnitFacing non deve diventare «guarda a est» per arrotondamento.
	Rejects(TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[{"id":"A","hero":"Hero.Flux","team":0,"cell":[0,0,0]}],"expect":[{"type":"UnitFacing","unit":"A","value":"NNE"}]})"),
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
		    { "id": "A1", "hero": "Hero.Flux",    "team": 0, "cell": [-1, 0, 0] },
		    { "id": "B1", "hero": "Hero.Bastion", "team": 1, "cell": [1, 0, 0] }
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
			TEXT(R"({ "id": "A1", "hero": "Hero.Flux", "team": 0, "cell": [-1, 0, 0] },
			         { "id": "B1", "hero": "Hero.Bastion", "team": 1, "cell": [1, 0, 0], "bot": true, "health": 7, "shield": 0, "visionRange": 1 })"),
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
			TEXT(R"({ "id": "A1", "hero": "Hero.Flux", "team": 0, "cell": [-1, 0, 0], "bot": true },
			         { "id": "B1", "hero": "Hero.Bastion", "team": 1, "cell": [1, 0, 0] })"),
			TEXT(R"({ "intents": [ { "unit": "A1", "move": [[0, 0, 0]] } ] })"), Scenario, Error);
		TestFalse(TEXT("un intent su un'unita' bot e' rifiutato"), bOk);
		TestTrue(FString::Printf(TEXT("e il motivo nomina l'unita' (%s)"), *Error), Error.Contains(TEXT("A1")));
	}

	// 3. Una salute a zero schiererebbe un cadavere.
	{
		FRTTestScenario Scenario;
		FString Error;
		const bool bOk = Load(
			TEXT(R"({ "id": "A1", "hero": "Hero.Flux", "team": 0, "cell": [-1, 0, 0], "health": 0 })"),
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
		    { "id": "A1", "hero": "Hero.Flux",    "team": 0, "cell": [-1, 0, 0] },
		    { "id": "B1", "hero": "Hero.Bastion", "team": 1, "cell": [1, 0, 0] }
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

#endif // WITH_DEV_AUTOMATION_TESTS
