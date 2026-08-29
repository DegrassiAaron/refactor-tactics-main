// Editing dell'initial state: Add / Move / Remove / Facing (#1115).
//
// La domanda di questi test non e' «l'operazione riesce?» ma **«chi ha deciso che poteva riuscire?»**.
// L'invariante di `spec-tactical-designer.md` §3 dice che la validita' di una cella la stabilisce il runtime,
// e un editor che se la calcolasse da se' sarebbe un secondo gioco che nessuno testa. Per questo il file
// verifica due cose diverse:
//
// 1. che le operazioni facciano cio' che dicono, e che l'errore **nomini** il problema;
// 2. che le regole rifiutate dall'authoring siano **le stesse** che `Validate` rifiuta sullo scenario intero.
//    E' la meta' che protegge dall'errore silenzioso: due copie della stessa regola non divergono il giorno
//    in cui vengono scritte, divergono al primo campo aggiunto a una sola delle due.

#include "Misc/AutomationTest.h"
#include "ScenarioHarness/RTScenarioAuthoring.h"
#include "ScenarioHarness/RTScenarioDraft.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nome distinto da ogni altro file di test: la unity build condivide la translation unit.
	const TCHAR* EditingBaseJson = TEXT(R"JSON(
	{
	  "scenarioId": "Movement.EditingProbe",
	  "version": 1,
	  "mapRadius": 3,
	  "cells": [ { "cell": [0, 1, 0], "blocksMovement": true } ],
	  "units": [
	    { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-2, 0, 0] },
	    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [2, 0, 0] }
	  ],
	  "turns": [ { "intents": [ { "unit": "A1", "move": [[-1, 0, 0]] } ] } ],
	  "expect": [ { "type": "UnitAtCell", "unit": "A1", "cell": [-1, 0, 0] } ]
	}
	)JSON");

	/** Un draft aperto sullo scenario di prova. `false` se non si e' potuto caricare. */
	bool OpenEditingDraft(FRTScenarioDraft& OutDraft, FString& OutError)
	{
		FRTTestScenario Loaded;
		if (!URTScenarioLoader::LoadFromString(EditingBaseJson, Loaded, OutError))
		{
			return false;
		}
		OutDraft.NewScenario(TEXT("segnaposto"), 3);
		OutDraft.MutableScenario() = Loaded;
		return true;
	}

	FString EditingTempDir()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Tests"), TEXT("ScenarioEditing"));
	}
}

// --- le operazioni fanno cio' che dicono ----------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioEditingAddMoveRemoveTest,
	"RefactorTactics.Scenario.EditingAddsMovesAndRemovesUnits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioEditingAddMoveRemoveTest::RunTest(const FString&)
{
	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"), OpenEditingDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}

	// --- Add ---
	if (!TestEqual(TEXT("una unita' nuova si schiera"),
		Draft.AddUnit(TEXT("A2"), FName(TEXT("Hero.Phase")), 0, FRTCellId(-1, 1, 0), ERTHexDirection::NE, Error),
		ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	TestEqual(TEXT("ora sono tre"), Draft.GetSummary().UnitCount, 3);

	// ⚠️ Il criterio di #1115: **il dato canonico cambia**, non una vista. Si rilegge dal modello, non da
	// `ListUnits`, perche' una vista costruita male potrebbe mostrare cio' che l'operazione ha promesso invece
	// di cio' che il modello contiene.
	const int32 Index = Draft.IndexOfUnit(TEXT("A2"));
	if (TestTrue(TEXT("A2 e' nel modello canonico"), Index != INDEX_NONE))
	{
		const FRTScenarioUnit& Canonical = Draft.GetScenario().Units[Index];
		TestEqual(TEXT("eroe scritto nel dato"), Canonical.HeroId, FName(TEXT("Hero.Phase")));
		TestEqual(TEXT("squadra scritta nel dato"), Canonical.TeamId, 0);
		TestEqual(TEXT("cella scritta nel dato"), Canonical.Cell, FRTCellId(-1, 1, 0));
		TestEqual(TEXT("facing scritto nel dato"), Canonical.Facing, ERTHexDirection::NE);
	}

	// --- Move ---
	if (!TestEqual(TEXT("A2 si sposta"),
		Draft.MoveUnit(TEXT("A2"), FRTCellId(0, -1, 0), Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	TestEqual(TEXT("la nuova cella e' nel dato canonico"),
		Draft.GetScenario().Units[Draft.IndexOfUnit(TEXT("A2"))].Cell, FRTCellId(0, -1, 0));

	// Muovere sul posto deve restare lecito: l'unita' non collide con se stessa. Senza `IgnoreUnitIndex`
	// questo caso fallirebbe dicendo «due unita' partono dalla stessa cella» parlando di una sola.
	TestEqual(TEXT("una unita' puo' essere rimessa dov'e' gia'"),
		Draft.MoveUnit(TEXT("A2"), FRTCellId(0, -1, 0), Error), ERTScenarioAuthoringResult::Success);

	// --- Facing ---
	TestEqual(TEXT("A2 ruota"),
		Draft.SetUnitFacing(TEXT("A2"), ERTHexDirection::SW, Error), ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("la rotazione e' nel dato canonico"),
		Draft.GetScenario().Units[Draft.IndexOfUnit(TEXT("A2"))].Facing, ERTHexDirection::SW);

	// --- Remove ---
	TestEqual(TEXT("A2 si ritira"),
		Draft.RemoveUnit(TEXT("A2"), Error), ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("tornano due"), Draft.GetSummary().UnitCount, 2);
	TestEqual(TEXT("A2 non e' piu' nel modello"), Draft.IndexOfUnit(TEXT("A2")), INDEX_NONE);

	// Gli Stable Unit ID delle altre non si spostano quando una viene tolta di mezzo.
	TestTrue(TEXT("A1 c'e' ancora"), Draft.IndexOfUnit(TEXT("A1")) != INDEX_NONE);
	TestTrue(TEXT("B1 c'e' ancora"), Draft.IndexOfUnit(TEXT("B1")) != INDEX_NONE);

	// Lo scenario resta valido dopo un giro completo di modifiche che si annullano.
	TestEqual(TEXT("lo scenario e' ancora valido"), Draft.Validate(Error), ERTScenarioAuthoringResult::Success);

	return true;
}

// --- gli errori nominano il problema --------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioEditingNamesItsRefusalsTest,
	"RefactorTactics.Scenario.EditingNamesWhatItRefuses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioEditingNamesItsRefusalsTest::RunTest(const FString&)
{
	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"), OpenEditingDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}

	const int32 Before = Draft.GetScenario().Units.Num();

	// Ogni rifiuto: la condizione, un frammento che l'errore DEVE contenere, e cosa si sta provando a fare.
	// Il frammento non e' il messaggio intero — un test che asserisse la frase esatta cadrebbe a ogni
	// riformulazione senza che nulla si sia rotto — ma deve nominare **la cosa**, perche' e' cio' che #1115
	// chiede: un errore leggibile che dica qual e' il problema.

	// (1) Stable Unit ID gia' preso.
	TestEqual(TEXT("id duplicato rifiutato"),
		Draft.AddUnit(TEXT("A1"), FName(TEXT("Hero.Phase")), 0, FRTCellId(-1, 1, 0), ERTHexDirection::E, Error), ERTScenarioAuthoringResult::Invalid);
	TestTrue(*FString::Printf(TEXT("l'errore nomina l'id duplicato (era: %s)"), *Error),
		Error.Contains(TEXT("A1")) && Error.Contains(TEXT("duplicat")));

	// (2) Cella gia' occupata da un'altra unita'.
	TestEqual(TEXT("cella occupata rifiutata"),
		Draft.AddUnit(TEXT("C1"), FName(TEXT("Hero.Phase")), 0, FRTCellId(2, 0, 0), ERTHexDirection::E, Error), ERTScenarioAuthoringResult::Invalid);
	TestTrue(*FString::Printf(TEXT("l'errore nomina la cella (era: %s)"), *Error),
		Error.Contains(TEXT("cella")));

	// (3) Cella fuori dall'arena: raggio 3, questa e' a distanza 9.
	TestEqual(TEXT("cella fuori arena rifiutata"),
		Draft.AddUnit(TEXT("C2"), FName(TEXT("Hero.Phase")), 0, FRTCellId(9, 0, 0), ERTHexDirection::E, Error), ERTScenarioAuthoringResult::Invalid);
	TestTrue(*FString::Printf(TEXT("l'errore nomina l'arena (era: %s)"), *Error),
		Error.Contains(TEXT("arena")));

	// (4) Cella che blocca il movimento: lo scenario ne dichiara una a (0,1,0).
	TestEqual(TEXT("cella bloccante rifiutata"),
		Draft.AddUnit(TEXT("C3"), FName(TEXT("Hero.Phase")), 0, FRTCellId(0, 1, 0), ERTHexDirection::E, Error), ERTScenarioAuthoringResult::Invalid);
	TestTrue(*FString::Printf(TEXT("l'errore nomina il blocco (era: %s)"), *Error),
		Error.Contains(TEXT("blocca")));

	// (5) Eroe che non e' a catalogo.
	TestEqual(TEXT("eroe sconosciuto rifiutato"),
		Draft.AddUnit(TEXT("C4"), FName(TEXT("Hero.NonEsiste")), 0, FRTCellId(-1, 1, 0), ERTHexDirection::E, Error), ERTScenarioAuthoringResult::Invalid);
	TestTrue(*FString::Printf(TEXT("l'errore nomina l'eroe (era: %s)"), *Error),
		Error.Contains(TEXT("eroe")) || Error.Contains(TEXT("sconosciuto")));

	// (6) Id vuoto.
	TestEqual(TEXT("id vuoto rifiutato"),
		Draft.AddUnit(FString(), FName(TEXT("Hero.Phase")), 0, FRTCellId(-1, 1, 0), ERTHexDirection::E, Error), ERTScenarioAuthoringResult::Invalid);
	TestTrue(*FString::Printf(TEXT("l'errore nomina l'id (era: %s)"), *Error), Error.Contains(TEXT("id")));

	// ⚠️ Sei rifiuti, e **nessuno** deve aver lasciato traccia: un'operazione che fallisce a meta' lascia lo
	// scenario in uno stato che nessuno ha chiesto, ed e' peggio di una che non parte.
	TestEqual(TEXT("nessun rifiuto ha modificato lo scenario"), Draft.GetScenario().Units.Num(), Before);

	// --- Move e Remove su un id che non esiste: e' `NotFound`, non `Invalid`. La UI ci ramifica sopra.
	TestEqual(TEXT("muovere un'unita' inesistente -> NotFound"),
		Draft.MoveUnit(TEXT("Fantasma"), FRTCellId(0, 0, 0), Error), ERTScenarioAuthoringResult::NotFound);
	TestTrue(*FString::Printf(TEXT("e nomina l'unita' (era: %s)"), *Error), Error.Contains(TEXT("Fantasma")));

	TestEqual(TEXT("ritirare un'unita' inesistente -> NotFound"),
		Draft.RemoveUnit(TEXT("Fantasma"), Error), ERTScenarioAuthoringResult::NotFound);
	TestEqual(TEXT("ruotare un'unita' inesistente -> NotFound"),
		Draft.SetUnitFacing(TEXT("Fantasma"), ERTHexDirection::E, Error), ERTScenarioAuthoringResult::NotFound);

	// --- Move verso una cella illecita: `Invalid`, e l'unita' resta dov'era.
	const FRTCellId Was = Draft.GetScenario().Units[Draft.IndexOfUnit(TEXT("A1"))].Cell;
	TestEqual(TEXT("spostare su una cella occupata -> Invalid"),
		Draft.MoveUnit(TEXT("A1"), FRTCellId(2, 0, 0), Error), ERTScenarioAuthoringResult::Invalid);
	TestEqual(TEXT("e l'unita' non si e' mossa"),
		Draft.GetScenario().Units[Draft.IndexOfUnit(TEXT("A1"))].Cell, Was);

	TestEqual(TEXT("spostare fuori arena -> Invalid"),
		Draft.MoveUnit(TEXT("A1"), FRTCellId(9, 9, 0), Error), ERTScenarioAuthoringResult::Invalid);
	TestEqual(TEXT("e nemmeno adesso si e' mossa"),
		Draft.GetScenario().Units[Draft.IndexOfUnit(TEXT("A1"))].Cell, Was);

	// Un draft senza scenario aperto risponde `NoScenarioOpen` a tutte, invece di fingere.
	FRTScenarioDraft Empty;
	TestEqual(TEXT("AddUnit senza scenario aperto"),
		Empty.AddUnit(TEXT("X"), FName(TEXT("Hero.Gadget")), 0, FRTCellId(0, 0, 0), ERTHexDirection::E, Error),
		ERTScenarioAuthoringResult::NoScenarioOpen);
	TestEqual(TEXT("MoveUnit senza scenario aperto"),
		Empty.MoveUnit(TEXT("X"), FRTCellId(0, 0, 0), Error), ERTScenarioAuthoringResult::NoScenarioOpen);
	TestEqual(TEXT("RemoveUnit senza scenario aperto"),
		Empty.RemoveUnit(TEXT("X"), Error), ERTScenarioAuthoringResult::NoScenarioOpen);
	// La quarta operazione va coperta come le altre tre: senza, togliere la sua guardia lascerebbe la suite
	// verde e la UI direbbe «unita' non schierata» dove la verita' e' che non c'e' nessuno scenario aperto.
	TestEqual(TEXT("SetUnitFacing senza scenario aperto"),
		Empty.SetUnitFacing(TEXT("X"), ERTHexDirection::E, Error), ERTScenarioAuthoringResult::NoScenarioOpen);

	return true;
}

// --- una regola sola, non due copie ---------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioEditingSharesTheRuleWithValidateTest,
	"RefactorTactics.Scenario.EditingRefusesExactlyWhatValidateRefuses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioEditingSharesTheRuleWithValidateTest::RunTest(const FString&)
{
	// ⚠️ **Questo e' il test che protegge l'invariante, e vale la pena dire da cosa.**
	//
	// `ValidateUnitPlacement` e' stata ESTRATTA da `ValidateScenarioUnits` proprio perche' l'authoring non
	// dovesse scriversi i propri controlli. Ma un'estrazione si puo' disfare: basta che qualcuno aggiunga una
	// regola dentro `Validate` e non dentro la funzione estratta, e da quel momento l'editor accetta
	// piazzamenti che il salvataggio poi rifiuta — o peggio, che salva scenari che il runner non sa eseguire.
	//
	// Il test costruisce piazzamenti illeciti UNO PER UNO e verifica che le due strade concordino:
	// se l'authoring lo rifiuta, `Validate` sullo scenario che ne risulterebbe deve rifiutarlo pure.
	struct FCase
	{
		const TCHAR* What;
		FString Id;
		FName Hero;
		FRTCellId Cell;
	};

	const TArray<FCase> Cases = {
		{ TEXT("id duplicato"),      TEXT("A1"), FName(TEXT("Hero.Phase")),     FRTCellId(-1, 1, 0) },
		{ TEXT("cella occupata"),    TEXT("C1"), FName(TEXT("Hero.Phase")),     FRTCellId( 2, 0, 0) },
		{ TEXT("fuori arena"),       TEXT("C2"), FName(TEXT("Hero.Phase")),     FRTCellId( 9, 0, 0) },
		{ TEXT("cella bloccante"),   TEXT("C3"), FName(TEXT("Hero.Phase")),     FRTCellId( 0, 1, 0) },
		{ TEXT("eroe sconosciuto"),  TEXT("C4"), FName(TEXT("Hero.NonEsiste")), FRTCellId(-1, 1, 0) }
	};

	for (const FCase& Case : Cases)
	{
		// ⚠️ Un draft FRESCO per caso, non uno condiviso. Se un caso regredisse ad «accettato», l'unita'
		// aggiunta resterebbe nel draft e ogni caso successivo verrebbe validato contro una baseline sporca:
		// il test che esiste per nominare la divergenza riporterebbe una cascata di fallimenti fuorvianti.
		FRTScenarioDraft Draft;
		FString Setup;
		if (!TestTrue(*FString::Printf(TEXT("'%s': scenario di partenza caricato"), Case.What),
			OpenEditingDraft(Draft, Setup)))
		{
			AddError(Setup);
			return false;
		}

		FString AuthoringError;
		const bool bAuthoringAccepts =
			Draft.AddUnit(Case.Id, Case.Hero, 0, Case.Cell, ERTHexDirection::E, AuthoringError)
				== ERTScenarioAuthoringResult::Success;

		// La stessa unita', forzata dentro lo scenario scavalcando l'authoring: cosa ne dice `Validate`?
		FRTTestScenario Forced = Draft.GetScenario();
		FRTScenarioUnit Unit;
		Unit.Id = Case.Id;
		Unit.HeroId = Case.Hero;
		Unit.TeamId = 0;
		Unit.Cell = Case.Cell;
		Forced.Units.Add(Unit);

		FString ValidateError;
		const bool bValidateAccepts = URTScenarioLoader::Validate(Forced, ValidateError);

		TestEqual(
			*FString::Printf(TEXT("'%s': authoring e Validate danno la stessa risposta"), Case.What),
			bAuthoringAccepts, bValidateAccepts);

		if (!bAuthoringAccepts && !bValidateAccepts)
		{
			// E non basta che entrambe rifiutino: devono rifiutare per lo STESSO motivo, altrimenti sono due
			// regole diverse che per caso cadono insieme su questo esempio.
			TestEqual(*FString::Printf(TEXT("'%s': stesso motivo di rifiuto"), Case.What),
				AuthoringError, ValidateError);
		}
	}

	return true;
}

// --- il giro completo: modifica, salva, rileggi ---------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioEditingSurvivesSaveReloadTest,
	"RefactorTactics.Scenario.EditingSurvivesSaveAndReload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioEditingSurvivesSaveReloadTest::RunTest(const FString&)
{
	const FString Dir = EditingTempDir();
	const FString Path = FPaths::Combine(Dir, TEXT("Edited.json"));
	ON_SCOPE_EXIT{ IFileManager::Get().DeleteDirectory(*Dir, false, true); };

	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"), OpenEditingDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}

	// Un giro di modifiche come lo farebbe l'Editor: piazza, sposta, ruota, ritira.
	if (!TestEqual(TEXT("schierata A2"),
		Draft.AddUnit(TEXT("A2"), FName(TEXT("Hero.Wraith")), 0, FRTCellId(-1, 1, 0), ERTHexDirection::NW, Error),
		ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	// Gli esiti si asseriscono qui: buttarli farebbe emergere una regressione di `MoveUnit` come un mismatch
	// di cella DOPO il round-trip, puntando il lettore verso il writer o il loader invece che verso il difetto.
	TestEqual(TEXT("B1 spostata"),
		Draft.MoveUnit(TEXT("B1"), FRTCellId(1, 1, 0), Error), ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("B1 ruotata"),
		Draft.SetUnitFacing(TEXT("B1"), ERTHexDirection::W, Error), ERTScenarioAuthoringResult::Success);

	if (!TestEqual(TEXT("salvato"), Draft.SaveToFile(Path, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	// #1115: «Save → riapri conserva lo scenario semanticamente: unità, celle, facing, ID». Si rilegge da
	// disco, non dalla memoria: e' l'unico modo di provare che le modifiche sono passate per il writer.
	FRTScenarioDraft Reloaded;
	if (!TestEqual(TEXT("riaperto da disco"),
		Reloaded.OpenFromFile(Path, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	TestEqual(TEXT("tre unita' dopo il giro"), Reloaded.GetSummary().UnitCount, 3);

	const int32 A2 = Reloaded.IndexOfUnit(TEXT("A2"));
	if (TestTrue(TEXT("A2 sopravvive al salvataggio"), A2 != INDEX_NONE))
	{
		TestEqual(TEXT("A2: eroe"), Reloaded.GetScenario().Units[A2].HeroId, FName(TEXT("Hero.Wraith")));
		TestEqual(TEXT("A2: cella"), Reloaded.GetScenario().Units[A2].Cell, FRTCellId(-1, 1, 0));
		TestEqual(TEXT("A2: facing"), Reloaded.GetScenario().Units[A2].Facing, ERTHexDirection::NW);
	}

	const int32 B1 = Reloaded.IndexOfUnit(TEXT("B1"));
	if (TestTrue(TEXT("B1 sopravvive"), B1 != INDEX_NONE))
	{
		TestEqual(TEXT("B1: la cella nuova"), Reloaded.GetScenario().Units[B1].Cell, FRTCellId(1, 1, 0));
		TestEqual(TEXT("B1: la rotazione nuova"), Reloaded.GetScenario().Units[B1].Facing, ERTHexDirection::W);
	}

	// E lo scenario riletto e' eseguibile: valido secondo la stessa regola che lo ha lasciato salvare.
	TestEqual(TEXT("lo scenario riletto e' valido"),
		Reloaded.Validate(Error), ERTScenarioAuthoringResult::Success);

	return true;
}

// --- la facade espone l'editing --------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioEditingIsReachableFromBlueprintTest,
	"RefactorTactics.Scenario.EditingIsReachableFromBlueprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioEditingIsReachableFromBlueprintTest::RunTest(const FString&)
{
	// Come per il contratto di ADR-0010: che il codice funzioni non prova che l'Editor possa chiamarlo.
	UClass* Facade = URTScenarioAuthoring::StaticClass();
	if (!TestNotNull(TEXT("la facade esiste"), Facade)) { return false; }

	for (const FName& Name : { FName(TEXT("AddUnit")), FName(TEXT("MoveUnit")), FName(TEXT("RemoveUnit")),
		FName(TEXT("SetUnitFacing")), FName(TEXT("ListHeroIds")) })
	{
		const UFunction* Function = Facade->FindFunctionByName(Name);
		if (!TestNotNull(*FString::Printf(TEXT("'%s' e' una UFUNCTION"), *Name.ToString()), Function))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("'%s' e' chiamabile da Blueprint"), *Name.ToString()),
			Function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure));
	}

	// Il catalogo che popola la tendina non deve essere vuoto, altrimenti l'Editor non ha niente da offrire
	// e chi lo usa finisce a scrivere gli id a mano — cioe' esattamente cio' che esporlo doveva evitare.
	const TArray<FName> Heroes = URTScenarioAuthoring::ListHeroIds();
	// `return false` e non un semplice TestTrue: sotto si indicizza `Heroes[0]`, e su un array vuoto il range
	// check ABORTISCE il processo. La suite morirebbe a meta' invece di riportare un rosso — e una suite morta
	// a meta' non e' rossa, e' NON VALIDA (D-222).
	if (!TestTrue(TEXT("il catalogo eroi non e' vuoto"), Heroes.Num() > 0)) { return false; }
	TestTrue(TEXT("e contiene il roster della v0.1"),
		Heroes.Contains(FName(TEXT("Hero.Gadget"))) && Heroes.Contains(FName(TEXT("Hero.Wraith"))));

	// Un giro dell'editing attraverso la facade, non attraverso il draft: e' il percorso che fara' l'Editor.
	URTScenarioAuthoring* Authoring = URTScenarioAuthoring::CreateScenarioDraft(nullptr);
	if (!TestNotNull(TEXT("draft creato"), Authoring)) { return false; }

	FString Error;
	Authoring->NewScenario(TEXT("Nuovo.DallaFacade"), 3);
	TestEqual(TEXT("la facade schiera"),
		Authoring->AddUnit(TEXT("A1"), Heroes[0], 0, FRTCellId(-1, 0, 0), ERTHexDirection::E, Error),
		ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("la facade sposta"),
		Authoring->MoveUnit(TEXT("A1"), FRTCellId(1, 0, 0), Error), ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("la facade ruota"),
		Authoring->SetUnitFacing(TEXT("A1"), ERTHexDirection::SE, Error), ERTScenarioAuthoringResult::Success);

	const TArray<FRTScenarioUnitView> Units = Authoring->ListUnits();
	if (TestEqual(TEXT("una unita' schierata"), Units.Num(), 1))
	{
		TestEqual(TEXT("la vista riflette lo spostamento"), Units[0].Cell, FRTCellId(1, 0, 0));
		TestEqual(TEXT("e la rotazione"), Units[0].Facing, ERTHexDirection::SE);
	}

	TestEqual(TEXT("la facade ritira"),
		Authoring->RemoveUnit(TEXT("A1"), Error), ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("non resta nessuno"), Authoring->ListUnits().Num(), 0);

	return true;
}

// --- i due difetti che la review ha trovato --------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioVariantRespectsBlockingCellsTest,
	"RefactorTactics.Scenario.VariantCannotPlaceAUnitInsideAnObstacle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioVariantRespectsBlockingCellsTest::RunTest(const FString&)
{
	// 🔴 Regressione trovata dalla review di #1115, ed era **preesistente**: `ValidateScenarioVariants` aveva
	// una copia PARZIALE della regola di piazzamento — controllava arena e collisioni, mai `bBlocksMovement`.
	// Una variante poteva quindi far partire una unita' dentro un ostacolo, cioe' esattamente lo «scenario
	// impossibile» che il percorso `units` rifiuta da sempre a due funzioni di distanza.
	const TCHAR* VariantOnBlockedJson = TEXT(R"JSON(
	{
	  "scenarioId": "Movement.VariantOnBlocked",
	  "version": 1,
	  "mapRadius": 3,
	  "cells": [ { "cell": [0, 1, 0], "blocksMovement": true } ],
	  "units": [
	    { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-2, 0, 0] },
	    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [2, 0, 0] }
	  ],
	  "turns": [ { "intents": [] } ],
	  "expect": [ { "type": "TurnsCompleted", "value": 1 } ],
	  "variants": [
	    { "name": "SullOstacolo", "units": [ { "id": "A1", "cell": [0, 1, 0] } ] },
	    { "name": "Altrove",      "units": [ { "id": "A1", "cell": [1, 0, 0] } ] }
	  ]
	}
	)JSON");

	FRTTestScenario Scenario;
	FString Error;
	TestFalse(TEXT("una variante che piazza su cella bloccante viene rifiutata"),
		URTScenarioLoader::LoadFromString(VariantOnBlockedJson, Scenario, Error));
	TestTrue(*FString::Printf(TEXT("l'errore nomina la variante e il blocco (era: %s)"), *Error),
		Error.Contains(TEXT("SullOstacolo")) && Error.Contains(TEXT("blocca")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioRemoveUnitReportsOrphansTest,
	"RefactorTactics.Scenario.EditingReportsWhatARemovalLeavesBehind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioRemoveUnitReportsOrphansTest::RunTest(const FString&)
{
	// `RemoveUnit` non ripulisce turni e assertion — e' una scelta — ma tacerlo lascerebbe scoprire il
	// problema solo al salvataggio, e con l'authoring dei turni ancora da fare (#1116) sarebbe un vicolo
	// cieco senza uscita. Qui si verifica che la rimozione **riesca** e che **lo dica**.
	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"), OpenEditingDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}

	// Nello scenario di prova A1 e' nominata da un intent e da una assertion.
	TestEqual(TEXT("A1 si ritira comunque"),
		Draft.RemoveUnit(TEXT("A1"), Error), ERTScenarioAuthoringResult::Success);
	TestFalse(TEXT("e la rimozione non e' silenziosa"), Error.IsEmpty());
	TestTrue(*FString::Printf(TEXT("il messaggio nomina cio' che resta (era: %s)"), *Error),
		Error.Contains(TEXT("A1")) && Error.Contains(TEXT("intent")));

	// E la conseguenza dichiarata e' vera: lo scenario non si salva piu'.
	TestEqual(TEXT("lo scenario non e' piu' valido"), Draft.Validate(Error), ERTScenarioAuthoringResult::Invalid);

	// Togliere una unita' che nessuno nomina resta silenzioso: il messaggio compare solo quando serve.
	FRTScenarioDraft Clean;
	if (TestTrue(TEXT("secondo scenario caricato"), OpenEditingDraft(Clean, Error)))
	{
		TestEqual(TEXT("B1 non e' nominata da nessuno"),
			Clean.RemoveUnit(TEXT("B1"), Error), ERTScenarioAuthoringResult::Success);
		TestTrue(TEXT("e la sua rimozione non ha niente da segnalare"), Error.IsEmpty());
	}

	return true;
}

/**
 * **L'UNICITA' DEGLI ID E' RETTA DA DUE PORTE, E TUTTE E DUE SI CHIUDONO** (#1515).
 *
 * 🔴 **Il difetto che questo test previene non e' un rifiuto mancato: e' un editor che OBBEDISCE A META'.**
 * `FRTScenarioDraft::IndexOfUnit` restituisce il **primo** match. Con due unita' che portano lo stesso id
 * `MoveUnit` sposterebbe la prima e lascerebbe la seconda, `RemoveUnit` ne toglierebbe una sola, e
 * `AddUnit` rifiuterebbe ogni piazzamento per quell'id — senza che niente lo dica.
 *
 * Le due porte da cui uno scenario entra in un draft:
 *   · **l'apertura** — `LoadFromString` termina con `Validate`, e il duplicato viene rifiutato NOMINANDOLO;
 *   · **l'editing** — `AddUnit` chiama `ValidateUnitPlacement` sul candidato prima di inserirlo.
 *
 * ⚠️ **Un test per porta, e non e' ridondanza**: sono due percorsi di codice diversi verso la stessa
 * regola, ed e' il caso in cui due copie divergono al primo campo aggiunto a una sola delle due — la
 * ragione per cui questo file esiste (vedi l'intestazione).
 *
 * ⛔ **La terza strada resta aperta per costruzione**: `MutableScenario()` da' accesso diretto allo
 * scenario, quindi un id duplicato puo' ancora entrare da codice. Non e' un buco da tappare con un terzo
 * guardiano — sarebbe una difesa senza consumatore — ma non e' nemmeno silenzioso: `IndexOfUnit` porta un
 * `ensure` che lo dichiara in sviluppo. Qui si asserisce cio' che le due porte garantiscono, non cio' che
 * il tipo impedisce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioDuplicateIdBothDoorsTest,
	"RefactorTactics.Scenario.DuplicateUnitIdIsRejectedAtBothDoors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioDuplicateIdBothDoorsTest::RunTest(const FString&)
{
	// PORTA 1 — l'apertura. Il file porta due unita' con lo stesso id.
	{
		FRTTestScenario Loaded;
		FString Error;
		const bool bOk = URTScenarioLoader::LoadFromString(
			TEXT(R"({"scenarioId":"X","mapRadius":3,"units":[)")
			TEXT(R"({"id":"Gemella","hero":"Hero.Gadget","team":0,"cell":[0,0,0]},)")
			TEXT(R"({"id":"Gemella","hero":"Hero.Phase","team":1,"cell":[1,0,0]}],)")
			TEXT(R"("expect":[{"type":"TurnsCompleted","value":1}]})"),
			Loaded, Error);
		TestFalse(TEXT("porta 1: lo scenario con id duplicato non si apre"), bOk);
		TestTrue(FString::Printf(TEXT("porta 1: l'errore NOMINA l'id (era: '%s')"), *Error),
			Error.Contains(TEXT("Gemella")));
	}

	// PORTA 2 — l'editing. Lo scenario di partenza e' valido; il duplicato lo introdurrebbe `AddUnit`.
	{
		FRTScenarioDraft Draft;
		FString Error;
		if (!TestTrue(TEXT("porta 2: scenario di partenza caricato"), OpenEditingDraft(Draft, Error)))
		{
			return false;
		}

		const int32 PrimaDiTutto = Draft.GetSummary().UnitCount;
		const FString IdEsistente = Draft.GetScenario().Units[0].Id;

		// Una cella LIBERA: il rifiuto deve venire dall'id, non dalla sovrapposizione — altrimenti il test
		// passerebbe per il motivo sbagliato e resterebbe verde anche togliendo il controllo sui duplicati.
		const ERTScenarioAuthoringResult Esito = Draft.AddUnit(
			IdEsistente, FName(TEXT("Hero.Wraith")), 1, FRTCellId(2, -1, 0), ERTHexDirection::NE, Error);

		TestEqual(TEXT("porta 2: AddUnit rifiuta un id gia' schierato"),
			Esito, ERTScenarioAuthoringResult::Invalid);
		TestTrue(FString::Printf(TEXT("porta 2: l'errore NOMINA l'id (era: '%s')"), *Error),
			Error.Contains(IdEsistente));
		TestEqual(TEXT("porta 2: e nessuna unita' e' stata aggiunta"),
			Draft.GetSummary().UnitCount, PrimaDiTutto);

		// Controprova: con un id nuovo, sulla STESSA cella, l'inserimento riesce. Senza, «rifiutato» non
		// distinguerebbe la regola sull'id da una fixture che non sa aggiungere unita'.
		const ERTScenarioAuthoringResult Controprova = Draft.AddUnit(
			TEXT("IdNuovo"), FName(TEXT("Hero.Wraith")), 1, FRTCellId(2, -1, 0), ERTHexDirection::NE, Error);
		TestEqual(TEXT("controprova: con un id nuovo la stessa aggiunta riesce"),
			Controprova, ERTScenarioAuthoringResult::Success);
		TestEqual(TEXT("controprova: ora le unita' sono una in piu'"),
			Draft.GetSummary().UnitCount, PrimaDiTutto + 1);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
