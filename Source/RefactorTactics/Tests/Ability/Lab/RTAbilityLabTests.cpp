// Ability Lab (#2599) — i sei test che la issue dichiara.
//
// ## Perche' nessun `AbilityId` e' cablato
//
// Le identita' di questo progetto si rinominano: `D-130` ha spostato `Flux` -> `Gadget` e `Riva` -> `Phase`,
// `D-334` ha spostato `Hero.Riktor` -> `Hero.Branth`, e `D-585` (#4011) l'ha riportato indietro. Un test che
// scrivesse `Hero.Gadget.LinearDischarge` diventerebbe rosso al prossimo rename **senza che nulla si sia rotto**,
// e il costo di quel falso rosso lo paga chi rinomina.
//
// Questi test chiedono invece al catalogo *«dammi un'ability di forma `Line`»* — che e' la proprieta' che
// stanno verificando davvero. Se un giorno il roster non ne avesse piu' nessuna, il test lo dice: e' un
// fatto sul catalogo, non un refuso di manutenzione.

#include "Misc/AutomationTest.h"
#include "Ability/Lab/RTAbilityLab.h"
#include "Ability/RTActionData.h"
#include "Ability/Catalog/RTActionReadout.h"
#include "Ability/Catalog/RTCatalogLibrary.h"
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/Lab/RTHeroLab.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/Report/RTTestResult.h"
#include "ScenarioHarness/Authoring/RTTestScenario.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Ability/RTEquipmentData.h"   // la copertura portatile, per il bersaglio sul bordo (#4072)
#include "Turn/Log/RTTurnLog.h"
#include "Turn/Log/RTTurnLogLibrary.h"
#include "Turn/Reaction/RTReactionLibrary.h" // l'esito `Activated` di una reazione (#4073)
#include "Unit/RTUnit.h"                      // tipo completo: l'header del controller assegna `TWeakObjectPtr<ARTUnit>`
#include "Player/RTPlayerController.h"        // la finestra di reazione si apre al controller (#4073)
#include "Map/RTHexLibrary.h"                  // la distanza della cella bloccata da una predittiva (#4088)
#include "Combat/RTHexCombatLibrary.h"         // le celle che l'azione colpisce: dove il banco posa i bersagli in piu' (#4136)
#include "ScenarioHarness/Authoring/RTScenarioLoader.h" // il tag dei bersagli attraversa il file della scena (#4136)
#include "HAL/FileManager.h"                   // il file della scena si toglie, per provare la banda senza file (#4136)
#include "Misc/ScopeExit.h"                    // la radice di prova del Lab si azzera sempre (#4136)
#include "Tests/Shared/RTScenarioTestSupport.h" // la radice di prova del Lab (#4136)

#if WITH_DEV_AUTOMATION_TESTS

// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
namespace RTAbilityLabTestsInternal
{
	UWorld* MakeAbilityLabWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyAbilityLabWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** La prima ability d'eroe con quella forma, se il roster ne offre una. */
	bool FindHeroAbilityWithShape(ERTAbilityShape Shape, FRTAbilityLabEntry& OutEntry)
	{
		for (const FRTAbilityLabEntry& Entry : URTAbilityLabLibrary::ListCanonicalAbilities())
		{
			if (!Entry.bIsCoreAction && Entry.Shape == Shape)
			{
				OutEntry = Entry;
				return true;
			}
		}
		return false;
	}

	/**
	 * Una spec che posa il bersaglio dentro la portata dichiarata dall'ability.
	 *
	 * E' collocazione di fixture, non aritmetica di gameplay: chi decide se il colpo arriva resta il
	 * resolver. Serve solo a non scrivere un test che misura «fuori portata» credendo di misurare la forma.
	 */
	FRTAbilityLabFixtureSpec SpecWithinRange(const FRTAbilityLabEntry& Entry)
	{
		FRTAbilityLabFixtureSpec Spec;
		Spec.MapRadius = 3;
		Spec.CasterCell = FRTCellId(0, 0);

		const int32 Reach = FMath::Clamp(Entry.RangeCells, 1, Spec.MapRadius);
		Spec.TargetCell = FRTCellId(Reach, 0);
		return Spec;
	}

	/** Esegue una fixture gia' costruita e restituisce il risultato del runner reale. */
	FRTTestResult RunFixture(FAutomationTestBase& Test, const FRTTestScenario& Scenario)
	{
		UWorld* World = MakeAbilityLabWorld();
		if (!World)
		{
			Test.AddError(TEXT("world non creato"));
			return FRTTestResult();
		}
		const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
		DestroyAbilityLabWorld(World);
		return Result;
	}
}

/**
 * Un `AbilityId` canonico si risolve, e produce una fixture eseguibile.
 *
 * Prende la prima ability d'eroe che il catalogo offre: se il roster e' vuoto o nessun eroe dichiara
 * un'azione indirizzabile per nome, e' quello il difetto da vedere, non un id sbagliato in questo file.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabValidAbilityIdResolvesTest,
	"RefactorTactics.AbilityLab.ValidAbilityIdResolves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabValidAbilityIdResolvesTest::RunTest(const FString&)
{
	const TArray<FRTAbilityLabEntry> Catalog = URTAbilityLabLibrary::ListCanonicalAbilities();
	if (!TestTrue(TEXT("il catalogo canonico non e' vuoto"), Catalog.Num() > 0)) { return false; }

	const FRTAbilityLabEntry* FirstHeroAbility = Catalog.FindByPredicate(
		[](const FRTAbilityLabEntry& E) { return !E.bIsCoreAction; });
	if (!TestNotNull(TEXT("il roster dichiara almeno un'ability d'eroe"), FirstHeroAbility)) { return false; }

	FRTAbilityLabEntry Found;
	TestTrue(TEXT("FindAbility trova l'ability appena elencata"),
		URTAbilityLabLibrary::FindAbility(FirstHeroAbility->AbilityId, Found));
	TestEqual(TEXT("e' la stessa ability"), Found.AbilityId, FirstHeroAbility->AbilityId);
	TestTrue(TEXT("l'ability appartiene a un eroe"), !Found.OwnerHeroId.IsNone());

	FRTTestScenario Scenario;
	FString Error;
	TestTrue(TEXT("BuildFixture riesce"),
		URTAbilityLabLibrary::BuildFixture(Found.AbilityId,
			RTAbilityLabTestsInternal::SpecWithinRange(Found), Scenario, Error));
	TestEqual(TEXT("nessun errore"), Error, FString());
	TestEqual(TEXT("due unita' nella posa"), Scenario.Units.Num(), 2);
	TestEqual(TEXT("un turno"), Scenario.Turns.Num(), 1);
	TestTrue(TEXT("l'harness ha un'assertion su cui cadere"), Scenario.Expect.Num() > 0);

	return true;
}

/**
 * Un `AbilityId` inesistente **rifiuta**, e non lascia dietro una fixture a meta'.
 *
 * La seconda meta' del criterio e' quella che conta: un `BuildFixture` che ritornasse `false` **dopo** aver
 * scritto in `OutScenario` farebbe eseguire al chiamante distratto una posa senza intent, il cui esito
 * sarebbe indistinguibile da quello di un'ability che non fa nulla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabInvalidAbilityIdFailsClosedTest,
	"RefactorTactics.AbilityLab.InvalidAbilityIdFailsClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabInvalidAbilityIdFailsClosedTest::RunTest(const FString&)
{
	const FName Bogus(TEXT("Hero.Nessuno.AbilitaCheNonEsiste"));

	FRTAbilityLabEntry Entry;
	TestFalse(TEXT("FindAbility non la trova"), URTAbilityLabLibrary::FindAbility(Bogus, Entry));

	FRTTestScenario Scenario;
	FString Error;
	TestFalse(TEXT("BuildFixture rifiuta"),
		URTAbilityLabLibrary::BuildFixture(Bogus, FRTAbilityLabFixtureSpec(), Scenario, Error));
	TestTrue(TEXT("l'errore nomina il motivo"), !Error.IsEmpty());
	TestTrue(TEXT("l'errore cita l'id rifiutato"), Error.Contains(Bogus.ToString()));

	// Fail closed: lo scenario non e' stato toccato.
	TestEqual(TEXT("nessuna unita' posata"), Scenario.Units.Num(), 0);
	TestEqual(TEXT("nessun turno dichiarato"), Scenario.Turns.Num(), 0);
	TestEqual(TEXT("nessun ScenarioId scritto"), Scenario.ScenarioId, FString());

	TArray<FRTActionParameterView> Parameters;
	TestEqual(TEXT("il readout dichiara UnknownAction"),
		URTAbilityLabLibrary::DescribeAbility(Bogus, Parameters), ERTActionReadoutResult::UnknownAction);

	return true;
}

/** Stesso seed, stessa fixture, stesso TurnLog: byte per byte. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabDeterministicRepeatTest,
	"RefactorTactics.AbilityLab.DeterministicRepeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabDeterministicRepeatTest::RunTest(const FString&)
{
	FRTAbilityLabEntry Entry;
	if (!TestTrue(TEXT("il roster offre un'ability lineare"),
		RTAbilityLabTestsInternal::FindHeroAbilityWithShape(ERTAbilityShape::Line, Entry)))
	{
		return false;
	}

	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("BuildFixture riesce"),
		URTAbilityLabLibrary::BuildFixture(Entry.AbilityId,
			RTAbilityLabTestsInternal::SpecWithinRange(Entry), Scenario, Error)))
	{
		return false;
	}

	const FRTTestResult First = RTAbilityLabTestsInternal::RunFixture(*this, Scenario);
	const FRTTestResult Second = RTAbilityLabTestsInternal::RunFixture(*this, Scenario);

	TestEqual(TEXT("stesso esito"), First.OutcomeString(), Second.OutcomeString());
	TestEqual(TEXT("stessi turni giocati"), First.TurnsPlayed, Second.TurnsPlayed);

	if (!TestEqual(TEXT("stesso numero di tracce"), First.TurnTraces.Num(), Second.TurnTraces.Num()))
	{
		return false;
	}
	for (int32 i = 0; i < First.TurnTraces.Num(); ++i)
	{
		TestTrue(FString::Printf(TEXT("traccia %d identica byte per byte"), i),
			First.TurnTraces[i].Bytes == Second.TurnTraces[i].Bytes);
	}

	return true;
}

/** La run scrive nel TurnLog canonico, e quelle voci si rileggono. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabTurnLogIsProducedTest,
	"RefactorTactics.AbilityLab.TurnLogIsProduced",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabTurnLogIsProducedTest::RunTest(const FString&)
{
	FRTAbilityLabEntry Entry;
	if (!TestTrue(TEXT("il roster offre un'ability lineare"),
		RTAbilityLabTestsInternal::FindHeroAbilityWithShape(ERTAbilityShape::Line, Entry)))
	{
		return false;
	}

	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("BuildFixture riesce"),
		URTAbilityLabLibrary::BuildFixture(Entry.AbilityId,
			RTAbilityLabTestsInternal::SpecWithinRange(Entry), Scenario, Error)))
	{
		return false;
	}

	const FRTTestResult Result = RTAbilityLabTestsInternal::RunFixture(*this, Scenario);
	if (Result.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("la run e' andata in ERROR: %s"), *Result.ErrorMessage));
		return false;
	}

	if (!TestTrue(TEXT("la run ha prodotto almeno una traccia"), Result.TurnTraces.Num() > 0))
	{
		return false;
	}

	const TArray<FString> Lines = URTAbilityLabLibrary::DescribeRunTurnLog(Result);
	TestTrue(TEXT("il TurnLog si rilegge in righe"), Lines.Num() > 0);
	for (const FString& Line : Lines)
	{
		TestFalse(TEXT("nessuna traccia illeggibile"), Line.Contains(TEXT("non deserializzabile")));
	}

	return true;
}

/** Una forma `Line` si esegue nel runtime reale. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabLinearAttackTest,
	"RefactorTactics.AbilityLab.LinearAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabLinearAttackTest::RunTest(const FString&)
{
	FRTAbilityLabEntry Entry;
	if (!TestTrue(TEXT("il roster offre un'ability di forma Line"),
		RTAbilityLabTestsInternal::FindHeroAbilityWithShape(ERTAbilityShape::Line, Entry)))
	{
		return false;
	}

	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("BuildFixture riesce"),
		URTAbilityLabLibrary::BuildFixture(Entry.AbilityId,
			RTAbilityLabTestsInternal::SpecWithinRange(Entry), Scenario, Error)))
	{
		return false;
	}

	const FRTTestResult Result = RTAbilityLabTestsInternal::RunFixture(*this, Scenario);
	if (Result.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("ERROR su %s: %s"),
			*Entry.AbilityId.ToString(), *Result.ErrorMessage));
		return false;
	}
	TestEqual(TEXT("un turno giocato"), Result.TurnsPlayed, 1);

	return true;
}

/** Una forma `Area` si esegue nel runtime reale. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabAreaOfEffectTest,
	"RefactorTactics.AbilityLab.AreaOfEffect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabAreaOfEffectTest::RunTest(const FString&)
{
	FRTAbilityLabEntry Entry;
	if (!TestTrue(TEXT("il roster offre un'ability di forma Area"),
		RTAbilityLabTestsInternal::FindHeroAbilityWithShape(ERTAbilityShape::Area, Entry)))
	{
		return false;
	}
	TestTrue(TEXT("un'AoE dichiara un raggio"), Entry.AreaRadius >= 0);

	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("BuildFixture riesce"),
		URTAbilityLabLibrary::BuildFixture(Entry.AbilityId,
			RTAbilityLabTestsInternal::SpecWithinRange(Entry), Scenario, Error)))
	{
		return false;
	}

	const FRTTestResult Result = RTAbilityLabTestsInternal::RunFixture(*this, Scenario);
	if (Result.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("ERROR su %s: %s"),
			*Entry.AbilityId.ToString(), *Result.ErrorMessage));
		return false;
	}
	TestEqual(TEXT("un turno giocato"), Result.TurnsPlayed, 1);

	return true;
}

// --- I tre difetti trovati in seduta il 2026-10-04 (`#3472`, `#3473`, `#3474`) -----------------------------
//
// Tutti e tre erano verdi qui sopra per la stessa ragione: i test del Lab scelgono la loro ability fra le voci
// di KIT (`!bIsCoreAction`) e verificano che il turno si giochi, non che cosa produca. In Editor la lista
// offre anche le azioni core, e il pannello mostra il diff di stato: le due cose che nessun test guardava.

/**
 * Un'azione core si ESEGUE solo se un'unita' la impugna davvero — `#3472`.
 *
 * Il caster di una core e' il primo eroe del roster, e un'unita' possiede il proprio kit piu' le generiche che
 * `MakeGenericActions` le accoda. Una core fuori da quell'insieme, costruita lo stesso, arriva all'harness che
 * la rifiuta in ERROR con *«'CASTER' non possiede l'abilita'»* — ed e' quello che la seduta ha visto su
 * `Action.Withdraw`. Il contratto qui: ogni core o si esegue senza ERROR, o e' rifiutata da `BuildFixture`
 * con un errore che la nomina e senza lasciare una fixture a meta'.
 *
 * ⚠️ **I due controlli positivi non sono decorazione.** Senza «almeno un rifiuto» il test sarebbe verde su un
 * Lab che le costruisce tutte e su un catalogo dove ogni core e' generica; senza «almeno una run» sarebbe
 * verde su un Lab che le rifiuta tutte, cioe' su un Lab che ha smesso di eseguire le generiche.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabCoreActionRunsOnlyIfAUnitWieldsItTest,
	"RefactorTactics.AbilityLab.CoreActionRunsOnlyIfAUnitWieldsIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabCoreActionRunsOnlyIfAUnitWieldsItTest::RunTest(const FString&)
{
	int32 Rifiutate = 0;
	int32 Eseguite = 0;

	for (const FRTAbilityLabEntry& Entry : URTAbilityLabLibrary::ListCanonicalAbilities())
	{
		if (!Entry.bIsCoreAction) { continue; }
		const FString Id = Entry.AbilityId.ToString();

		FRTTestScenario Scenario;
		FString Error;
		if (!URTAbilityLabLibrary::BuildFixture(Entry.AbilityId,
			RTAbilityLabTestsInternal::SpecWithinRange(Entry), Scenario, Error))
		{
			++Rifiutate;
			TestTrue(FString::Printf(TEXT("%s: il rifiuto la nomina"), *Id), Error.Contains(Id));
			TestEqual(FString::Printf(TEXT("%s: fail closed, nessuna unita' posata"), *Id), Scenario.Units.Num(), 0);
			continue;
		}

		++Eseguite;
		const FRTTestResult Result = RTAbilityLabTestsInternal::RunFixture(*this, Scenario);
		if (Result.Outcome == ERTTestOutcome::Error)
		{
			AddError(FString::Printf(TEXT("%s: BuildFixture l'ha costruita, ma la run va in ERROR: %s"),
				*Id, *Result.ErrorMessage));
		}
	}

	TestTrue(TEXT("almeno una core e' rifiutata"), Rifiutate > 0);
	TestTrue(TEXT("almeno una core si esegue"), Eseguite > 0);
	return true;
}

/**
 * Ogni voce di KIT che il Lab offre si esegue senza ERROR — `#3472`, la stessa classe di difetto dal lato kit.
 *
 * Una voce di kit la impugna sempre il suo eroe, quindi qui non c'e' rifiuto legittimo: se la run va in ERROR
 * e' la fixture a essere sbagliata. Il caso che l'ha imposto: un'azione che si applica a chi la usa
 * (`bSelfTarget`) veniva scritta con il CASTER come bersaglio, e il formato lo rifiuta — *«l'unita' bersaglia
 * se stessa»* — perche' un'abilita' su di se' si dichiara SENZA bersaglio (`AbilityResolvesOnSelf`, `#2283`).
 * I test sopra non lo vedevano: scelgono la prima voce di una forma, mai una che si applica a se'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabEveryKitAbilityRunsWithoutErrorTest,
	"RefactorTactics.AbilityLab.EveryKitAbilityRunsWithoutError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabEveryKitAbilityRunsWithoutErrorTest::RunTest(const FString&)
{
	int32 Eseguite = 0;

	for (const FRTAbilityLabEntry& Entry : URTAbilityLabLibrary::ListCanonicalAbilities())
	{
		if (Entry.bIsCoreAction) { continue; }
		const FString Id = Entry.AbilityId.ToString();

		FRTTestScenario Scenario;
		FString Error;
		if (!TestTrue(FString::Printf(TEXT("%s: BuildFixture la costruisce"), *Id),
			URTAbilityLabLibrary::BuildFixture(Entry.AbilityId,
				RTAbilityLabTestsInternal::SpecWithinRange(Entry), Scenario, Error)))
		{
			AddInfo(FString::Printf(TEXT("%s: %s"), *Id, *Error));
			continue;
		}

		++Eseguite;
		const FRTTestResult Result = RTAbilityLabTestsInternal::RunFixture(*this, Scenario);
		if (Result.Outcome == ERTTestOutcome::Error)
		{
			AddError(FString::Printf(TEXT("%s: la run va in ERROR: %s"), *Id, *Result.ErrorMessage));
		}
	}

	// Senza questo controllo il test sarebbe verde su un roster senza kit.
	TestTrue(TEXT("il roster offre voci di kit da eseguire"), Eseguite > 0);
	return true;
}

/**
 * Ogni SCATTO di kit che il Lab offre muove chi lo usa — `#3773`.
 *
 * Il Lab scriveva ogni azione come abilita' principale, e una mobilita' di fase Dash in quel campo il turno la
 * ignorava: la seduta del banco di #3562 ha visto Ivrin restare fermo su `PassingBlade`, con la scena superata.
 * I test sopra non lo vedevano per la ragione scritta in testa a questa sezione: verificano che il turno si
 * giochi, non che cosa produca.
 *
 * 🔑 **Gli scatti si cercano per FASE**, non per nome (vedi la testa del file): un'azione la cui fase mappa su
 * `ERTMatchPhase::Dash`. La posa e' quella di default del Lab — la stessa del pannello — perche' e' quella che
 * l'autore vede: un test con una posa scelta apposta direbbe poco del banco vero.
 *
 * ⚠️ **«Si e' mosso», non «e' arrivato li'».** Dove si ferma lo decide il resolver — chi attraversa e chi si
 * ferma sul primo nemico — e il Lab non ha il compito di saperlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabEveryKitDashMovesTheCasterTest,
	"RefactorTactics.AbilityLab.EveryKitDashMovesTheCaster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabEveryKitDashMovesTheCasterTest::RunTest(const FString&)
{
	TArray<FName> Scatti;
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Hero) { continue; }
		for (const TObjectPtr<URTActionData>& Action : Hero->Actions)
		{
			if (Action && !Action->Def.ActionId.IsNone()
				&& URTCatalogLibrary::MapResolutionPhase(Action->Def.ResolutionPhase) == ERTMatchPhase::Dash)
			{
				Scatti.Add(Action->Def.ActionId);
			}
		}
	}
	// Senza questo controllo il test sarebbe verde su un roster senza scatti.
	if (!TestTrue(TEXT("il roster offre scatti di kit"), Scatti.Num() > 0)) { return false; }

	for (const FName& AbilityId : Scatti)
	{
		const FString Id = AbilityId.ToString();

		FRTTestScenario Scenario;
		FString Error;
		if (!TestTrue(FString::Printf(TEXT("%s: BuildFixture la costruisce"), *Id),
			URTAbilityLabLibrary::BuildFixture(AbilityId, FRTAbilityLabFixtureSpec(), Scenario, Error)))
		{
			AddInfo(FString::Printf(TEXT("%s: %s"), *Id, *Error));
			continue;
		}

		// La forma dell'intent: lo slot movimento, non la principale.
		const FRTScenarioIntent& Intent = Scenario.Turns[0].Intents[0];
		TestEqual(FString::Printf(TEXT("%s: dichiarata come scatto"), *Id), Intent.Dash, AbilityId);
		TestTrue(FString::Printf(TEXT("%s: e non come abilita'"), *Id), Intent.Ability.IsNone());

		const FRTTestResult Result = RTAbilityLabTestsInternal::RunFixture(*this, Scenario);
		if (Result.Outcome == ERTTestOutcome::Error)
		{
			AddError(FString::Printf(TEXT("%s: la run va in ERROR: %s"), *Id, *Result.ErrorMessage));
			continue;
		}

		bool bCasterSiMuove = false;
		for (const FRTUnitStateDiff& Diff : Result.StateDiff)
		{
			const FString* Authoring = Result.ScenarioIdByUnitId.Find(Diff.UnitId);
			if (!Authoring || *Authoring != TEXT("CASTER")) { continue; }
			for (const FRTUnitFieldChange& Cambio : Diff.Changes)
			{
				bCasterSiMuove |= Cambio.Field == FName(TEXT("Cell"));
			}
		}
		TestTrue(FString::Printf(TEXT("%s: il caster cambia cella"), *Id), bCasterSiMuove);
	}
	return true;
}

/**
 * Il readout di un'azione core descrive l'istanza che l'unita' RICEVE, o dichiara che non ce n'e' una — `#3473`.
 *
 * Il valore «letto» e' per contratto cio' che il consumatore reale legge. Per una generica il consumatore e'
 * l'istanza di `MakeGenericActions`, che propaga dal `Def` portata, ricarica e potenza; il Lab ne costruiva
 * un'altra copiando il solo `Def`, e mostrava i default legacy (portata 5) come se fossero letti. Per una core
 * che nessuna unita' porta, un valore letto non esiste affatto: va detto, non inventato.
 *
 * 🔑 **Il confronto e' contro l'istanza, non contro il catalogo.** «Letto == catalogo» sarebbe verde anche su
 * un Lab che riallineasse da se' i campi specchio, cioe' su una terza copia che nessuna unita' usa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabCoreReadoutMatchesTheUnitsInstanceTest,
	"RefactorTactics.AbilityLab.CoreReadoutMatchesTheUnitsInstance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabCoreReadoutMatchesTheUnitsInstanceTest::RunTest(const FString&)
{
	const TArray<URTActionData*> Generiche = URTCatalogLibrary::MakeGenericActions(GetTransientPackage());
	if (!TestTrue(TEXT("il catalogo dichiara delle azioni generiche"), Generiche.Num() > 0)) { return false; }

	for (const URTActionData* Istanza : Generiche)
	{
		const FString Id = Istanza->Def.ActionId.ToString();

		TArray<FRTActionParameterView> DalLab;
		TArray<FRTActionParameterView> DallIstanza;
		const ERTActionReadoutResult Esito = URTAbilityLabLibrary::DescribeAbility(Istanza->Def.ActionId, DalLab);
		URTActionReadoutLibrary::DescribeActionParameters(Istanza, DallIstanza);

		TestTrue(FString::Printf(TEXT("%s: il Lab la legge"), *Id), Esito == ERTActionReadoutResult::Ok);
		if (!TestEqual(FString::Printf(TEXT("%s: stesse voci"), *Id), DalLab.Num(), DallIstanza.Num()))
		{
			continue;
		}
		for (int32 i = 0; i < DalLab.Num(); ++i)
		{
			const FString Voce = FString::Printf(TEXT("%s · %s"), *Id, *DallIstanza[i].ParameterKey.ToString());
			TestEqual(Voce + TEXT(": stessa chiave"), DalLab[i].ParameterKey, DallIstanza[i].ParameterKey);
			TestEqual(Voce + TEXT(": stesso valore di catalogo"),
				DalLab[i].DeclaredValue, DallIstanza[i].DeclaredValue);
			TestEqual(Voce + TEXT(": stesso valore letto"), DalLab[i].ConsumedValue, DallIstanza[i].ConsumedValue);
		}
	}

	// La meta' che nessuna unita' porta. Si cerca, non si nomina: vedi la testa di questo file.
	const TArray<FName> IdGeneriche = URTCatalogLibrary::GetGenericActionIds();
	const TArray<FRTAbilityLabEntry> Catalogo = URTAbilityLabLibrary::ListCanonicalAbilities();
	const FRTAbilityLabEntry* NonImpugnata = Catalogo.FindByPredicate([&IdGeneriche](const FRTAbilityLabEntry& E)
	{
		return E.bIsCoreAction && !IdGeneriche.Contains(E.AbilityId);
	});
	if (!TestNotNull(TEXT("il catalogo ha una core che nessuna unita' impugna"), NonImpugnata)) { return false; }

	TArray<FRTActionParameterView> SoloCatalogo;
	TestTrue(FString::Printf(TEXT("%s: il readout dichiara che esiste solo la casa del catalogo"),
			*NonImpugnata->AbilityId.ToString()),
		URTAbilityLabLibrary::DescribeAbility(NonImpugnata->AbilityId, SoloCatalogo)
			== ERTActionReadoutResult::CatalogOnly);
	TestTrue(TEXT("e il catalogo resta leggibile"), SoloCatalogo.Num() > 0);

	return true;
}

/**
 * Lo `StateDiff` della run accoppia le unita' di prima con quelle di dopo — `#3474`.
 *
 * Il diff si costruisce per `StableUnitId`, e lo stato «prima» si fotografava in `Start()`, quando l'harness
 * non ha ancora assegnato le identita' (lo fa al lock-in). Prima `{0, 0}`, dopo due id veri: quattro voci,
 * due «sparite» e due «comparse», nessuna con un campo cambiato — e il pannello, che stampa i campi cambiati,
 * taceva su un colpo che il TurnLog dichiarava.
 *
 * ⚠️ Il controllo sulla `Health` e' il verso che conta: un diff con due voci presenti e nessun campo sarebbe
 * il sintomo di una cattura spostata DOPO il turno, e questo test deve prendere anche quella.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabStateDiffPairsTheUnitsOfTheRunTest,
	"RefactorTactics.AbilityLab.StateDiffPairsTheUnitsOfTheRun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabStateDiffPairsTheUnitsOfTheRunTest::RunTest(const FString&)
{
	FRTAbilityLabEntry Entry;
	if (!TestTrue(TEXT("il roster offre un'ability lineare"),
		RTAbilityLabTestsInternal::FindHeroAbilityWithShape(ERTAbilityShape::Line, Entry)))
	{
		return false;
	}

	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("BuildFixture riesce"),
		URTAbilityLabLibrary::BuildFixture(Entry.AbilityId,
			RTAbilityLabTestsInternal::SpecWithinRange(Entry), Scenario, Error)))
	{
		return false;
	}

	const FRTTestResult Result = RTAbilityLabTestsInternal::RunFixture(*this, Scenario);
	if (Result.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("ERROR su %s: %s"), *Entry.AbilityId.ToString(), *Result.ErrorMessage));
		return false;
	}

	if (!TestEqual(TEXT("una voce di diff per ogni unita' della posa"), Result.StateDiff.Num(), Scenario.Units.Num()))
	{
		for (const FRTUnitStateDiff& Diff : Result.StateDiff)
		{
			AddInfo(FString::Printf(TEXT("voce: unita' %d, presenza %d, campi cambiati %d"),
				Diff.UnitId, static_cast<int32>(Diff.Presence), Diff.Changes.Num()));
		}
		return false;
	}

	bool bBersaglioColpito = false;
	for (const FRTUnitStateDiff& Diff : Result.StateDiff)
	{
		TestTrue(FString::Printf(TEXT("unita' %d presente prima e dopo"), Diff.UnitId),
			Diff.Presence == ERTUnitDiffPresence::Present);

		const FString* Authoring = Result.ScenarioIdByUnitId.Find(Diff.UnitId);
		if (!TestNotNull(FString::Printf(TEXT("unita' %d ha un'identita' d'authoring"), Diff.UnitId), Authoring))
		{
			continue;
		}
		if (*Authoring != TEXT("TARGET")) { continue; }

		for (const FRTUnitFieldChange& Cambio : Diff.Changes)
		{
			if (Cambio.Field == FName(TEXT("Health")) && FCString::Atoi(*Cambio.After) < FCString::Atoi(*Cambio.Before))
			{
				bBersaglioColpito = true;
			}
		}
	}
	TestTrue(TEXT("la Health del bersaglio scende nel diff, come il TurnLog dichiara"), bBersaglioColpito);

	return true;
}

/**
 * Lo `StateDiff` copre l'INTERA run, non l'ultimo turno — `#3474`, dalla revisione di #3476.
 *
 * Il «prima» si cattura al primo `PlanningLocked`, e UNA volta sola. Senza quella guardia ogni lock-in lo
 * ricatturerebbe, e uno scenario a piu' turni misurerebbe solo l'ultimo: il test sopra, a un turno solo, non
 * se ne accorgerebbe. Qui il colpo arriva al turno 1 e il turno 2 e' vuoto — ogni unita' ferma, come il
 * corpus gia' scrive (`"intents": []`) — quindi un diff senza la `Health` del bersaglio ha dimenticato il turno 1.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabStateDiffSpansTheWholeRunTest,
	"RefactorTactics.AbilityLab.StateDiffSpansTheWholeRun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabStateDiffSpansTheWholeRunTest::RunTest(const FString&)
{
	FRTAbilityLabEntry Entry;
	if (!TestTrue(TEXT("il roster offre un'ability lineare"),
		RTAbilityLabTestsInternal::FindHeroAbilityWithShape(ERTAbilityShape::Line, Entry)))
	{
		return false;
	}

	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("BuildFixture riesce"),
		URTAbilityLabLibrary::BuildFixture(Entry.AbilityId,
			RTAbilityLabTestsInternal::SpecWithinRange(Entry), Scenario, Error)))
	{
		return false;
	}

	// Il turno 2, vuoto. L'assertion della fixture conta i turni, quindi segue.
	Scenario.Turns.Add(FRTScenarioTurn());
	for (FRTTestExpectation& Attesa : Scenario.Expect)
	{
		if (Attesa.Kind == ERTAssertionKind::TurnsCompleted) { Attesa.Value = Scenario.Turns.Num(); }
	}

	const FRTTestResult Result = RTAbilityLabTestsInternal::RunFixture(*this, Scenario);
	if (Result.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("ERROR su %s: %s"), *Entry.AbilityId.ToString(), *Result.ErrorMessage));
		return false;
	}
	if (!TestEqual(TEXT("due turni giocati"), Result.TurnsPlayed, 2)) { return false; }

	bool bBersaglioColpito = false;
	for (const FRTUnitStateDiff& Diff : Result.StateDiff)
	{
		const FString* Authoring = Result.ScenarioIdByUnitId.Find(Diff.UnitId);
		if (!Authoring || *Authoring != TEXT("TARGET")) { continue; }

		for (const FRTUnitFieldChange& Cambio : Diff.Changes)
		{
			if (Cambio.Field == FName(TEXT("Health")) && FCString::Atoi(*Cambio.After) < FCString::Atoi(*Cambio.Before))
			{
				bBersaglioColpito = true;
			}
		}
	}
	TestTrue(TEXT("il diff porta il colpo del turno 1, non solo il turno 2"), bBersaglioColpito);

	return true;
}

// --- #3774 · cambiare azione dentro il PIE ---------------------------------------------------------------
//
// I due tasti del banco passano all'azione precedente o successiva DELLO STESSO EROE e ripartono. La parte che
// tocca il mondo (scrivere la scena, la console variable, ricaricare il livello) si giudica in PIE; qui sta la
// parte che decide: quale azione viene dopo, e cosa dice la banda.

namespace RTAbilityLabTestsInternal
{
	/**
	 * Il ciclo ATTESO dell'eroe della prima voce di kit del catalogo: le voci di `ListHeroKit` che `BuildFixture`
	 * costruisce, nell'ordine del kit. Lo calcola il test con le funzioni pubbliche, non chiedendolo alla
	 * funzione che verifica.
	 */
	TArray<FName> ExpectedHeroCycle(FName& OutHeroId)
	{
		OutHeroId = NAME_None;
		for (const FRTAbilityLabEntry& Entry : URTAbilityLabLibrary::ListCanonicalAbilities())
		{
			if (!Entry.bIsCoreAction) { OutHeroId = Entry.OwnerHeroId; break; }
		}
		TArray<FName> Out;
		for (const FRTAbilityLabEntry& Entry : URTHeroLabLibrary::ListHeroKit(OutHeroId))
		{
			FRTTestScenario Scratch;
			FString Error;
			if (URTAbilityLabLibrary::BuildFixture(Entry.AbilityId, FRTAbilityLabFixtureSpec(), Scratch, Error))
			{
				Out.Add(Entry.AbilityId);
			}
		}
		return Out;
	}

	FString LabId(const FName& AbilityId)
	{
		return FString::Printf(TEXT("AbilityLab.%s"), *AbilityId.ToString());
	}
}

/**
 * Il passo del banco gira dentro il kit dell'eroe, nei due versi, e fa il giro agli estremi — `#3774`.
 *
 * ⚠️ **Ogni passo, non uno solo**: un passo verificato una volta sarebbe verde anche su un ciclo che salta una voce
 * o che si ferma all'ultima invece di tornare alla prima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabStepCyclesWithinTheHeroTest,
	"RefactorTactics.AbilityLab.StepCyclesWithinTheHero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabStepCyclesWithinTheHeroTest::RunTest(const FString&)
{
	FName Eroe;
	const TArray<FName> Ciclo = RTAbilityLabTestsInternal::ExpectedHeroCycle(Eroe);
	// Con meno di due voci non c'e' un passo da verificare, e il test lo dice invece di passare.
	if (!TestTrue(TEXT("l'eroe ha almeno due azioni che il Lab costruisce"), Ciclo.Num() >= 2)) { return false; }

	const int32 N = Ciclo.Num();
	for (int32 i = 0; i < N; ++i)
	{
		const FString Da = RTAbilityLabTestsInternal::LabId(Ciclo[i]);
		for (const int32 Passo : { +1, -1 })
		{
			const int32 j = (i + Passo + N) % N;
			FName Prossima;
			int32 Posizione = 0;
			int32 Totale = 0;
			if (!TestTrue(FString::Printf(TEXT("%s, passo %+d: c'e' un'azione"), *Da, Passo),
				URTAbilityLabLibrary::PlanLabStep(Da, Passo, Prossima, Posizione, Totale)))
			{
				continue;
			}
			TestEqual(FString::Printf(TEXT("%s, passo %+d: l'azione"), *Da, Passo), Prossima, Ciclo[j]);
			TestEqual(FString::Printf(TEXT("%s, passo %+d: la posizione, da 1"), *Da, Passo), Posizione, j + 1);
			TestEqual(FString::Printf(TEXT("%s, passo %+d: quante sono"), *Da, Passo), Totale, N);
		}
	}
	AddInfo(FString::Printf(TEXT("eroe %s, %d azioni nel ciclo"), *Eroe.ToString(), N));
	return true;
}

/** Fuori da una scena del banco il passo non trova niente: i due tasti restano inerti — `#3774`. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabStepIsInertOutsideTheLabTest,
	"RefactorTactics.AbilityLab.StepIsInertOutsideTheLab",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabStepIsInertOutsideTheLabTest::RunTest(const FString&)
{
	// Una partita normale, uno scenario versionato che usa un'abilita' d'eroe, e un Id del Lab che non nomina
	// nessuna azione canonica: tutti e tre fuori dal banco.
	for (const TCHAR* Id : { TEXT(""), TEXT("Visual.Ability.ClipIvrin"), TEXT("AbilityLab.Azione.Inesistente") })
	{
		FName Prossima;
		int32 Posizione = -7;
		int32 Totale = -7;
		TestFalse(FString::Printf(TEXT("'%s': nessun passo"), Id),
			URTAbilityLabLibrary::PlanLabStep(Id, +1, Prossima, Posizione, Totale));
		TestTrue(FString::Printf(TEXT("'%s': nessuna azione proposta"), Id), Prossima.IsNone());
		TestTrue(FString::Printf(TEXT("'%s': la banda non aggiunge niente"), Id),
			URTAbilityLabLibrary::DescribeLabStep(Id).IsEmpty());
	}

	// Controllo positivo: una scena del banco vera il passo lo trova. Senza, il test sarebbe verde su un passo
	// che non funziona mai.
	FName Eroe;
	const TArray<FName> Ciclo = RTAbilityLabTestsInternal::ExpectedHeroCycle(Eroe);
	if (!TestTrue(TEXT("premessa: l'eroe ha delle azioni"), Ciclo.Num() > 0)) { return false; }
	FName Prossima;
	int32 Posizione = 0;
	int32 Totale = 0;
	TestTrue(TEXT("in una scena del banco il passo c'e'"),
		URTAbilityLabLibrary::PlanLabStep(RTAbilityLabTestsInternal::LabId(Ciclo[0]), +1, Prossima, Posizione, Totale));
	return true;
}

/** La banda di una scena del banco dice dove si e' nel kit e quali tasti premere — `#3774`. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabStepHintNamesPositionAndKeysTest,
	"RefactorTactics.AbilityLab.StepHintNamesPositionAndKeys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabStepHintNamesPositionAndKeysTest::RunTest(const FString&)
{
	FName Eroe;
	const TArray<FName> Ciclo = RTAbilityLabTestsInternal::ExpectedHeroCycle(Eroe);
	if (!TestTrue(TEXT("premessa: l'eroe ha almeno due azioni"), Ciclo.Num() >= 2)) { return false; }

	const FString Seconda = URTAbilityLabLibrary::DescribeLabStep(RTAbilityLabTestsInternal::LabId(Ciclo[1]));
	TestTrue(TEXT("dice la posizione della scena corrente"), Seconda.Contains(FString::Printf(TEXT("2/%d"), Ciclo.Num())));
	TestTrue(TEXT("nomina il tasto per la precedente"), Seconda.Contains(TEXT("«,»")));
	TestTrue(TEXT("nomina il tasto per la successiva"), Seconda.Contains(TEXT("«.»")));
	return true;
}

/**
 * Una scena del banco si riconosce dal suo Id, e solo dalla forma che `BuildFixture` scrive — `#4071`, [D-607] (1).
 *
 * 🔑 E' la domanda da cui dipende la velocita' del banco: un Id riconosciuto per sbaglio darebbe x0,5 a uno scenario che
 * non e' del banco. ⛔ Il prefisso e' sensibile alle maiuscole, come in `PlanLabStep`, e senza un'azione dopo non basta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabRecognisesItsScenesTest,
	"RefactorTactics.AbilityLab.RecognisesOnlyItsOwnScenes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabRecognisesItsScenesTest::RunTest(const FString&)
{
	FRTTestScenario Scena;
	FString Errore;
	if (!TestTrue(TEXT("⛔ premessa: il banco costruisce la scena di ImpactShot"),
			URTAbilityLabLibrary::BuildFixture(FName(TEXT("Hero.Riktor.ImpactShot")), FRTAbilityLabFixtureSpec(), Scena, Errore)))
	{
		return false;
	}
	TestTrue(TEXT("🔴 l'Id che BuildFixture scrive e' una scena del banco"), URTAbilityLabLibrary::IsLabScenarioId(Scena.ScenarioId));
	TestFalse(TEXT("un prefisso senza azione non lo e'"), URTAbilityLabLibrary::IsLabScenarioId(TEXT("AbilityLab.")));
	TestFalse(TEXT("ne' con le maiuscole sbagliate"), URTAbilityLabLibrary::IsLabScenarioId(TEXT("abilitylab.Hero.Riktor.ImpactShot")));
	TestFalse(TEXT("ne' uno scenario del corpus"), URTAbilityLabLibrary::IsLabScenarioId(TEXT("Visual.Reaction.Deflection")));
	TestFalse(TEXT("ne' l'Id vuoto"), URTAbilityLabLibrary::IsLabScenarioId(FString()));
	TestEqual(TEXT("e la velocita' del banco e' x0,5"), URTAbilityLabLibrary::BenchPlaybackSpeed(), 0.5f);
	return true;
}

// =====================================================================================================
// `#4072` — il bersaglio adatto ([D-607] (2)). La FORMA la decide `URTPointerLibrary::TargetKindForAction`, il LATO di
// un'unita' gli effetti dell'azione (`BenefitsItsRecipient`); la spec del banco puo' forzarli.
//
// 🔑 **La posa e' quella di default del banco**, la stessa del pannello: chi lancia in (0,0), il secondo eroe in (2,0),
// mappa di raggio 3. Un test con una posa scelta apposta direbbe poco della scena che l'autore guarda.
// =====================================================================================================

namespace RTAbilityLabTestsInternal
{
	/** Il `Def` e la forma di un'azione di kit, come li impugna l'eroe. `false` se il roster non la porta. */
	bool FindKitAction(const FName& AbilityId, FRTActionDef& OutDef, ERTAbilityShape& OutShape)
	{
		for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (!Hero) { continue; }
			for (const TObjectPtr<URTActionData>& Ptr : Hero->Actions)
			{
				if (Ptr && Ptr->Def.ActionId == AbilityId)
				{
					OutDef = Ptr->Def;
					OutShape = Ptr->Shape;
					return true;
				}
			}
		}
		return false;
	}

	/** Il `Def` e la forma di una generica, dall'istanza che ogni unita' riceve (`MakeGenericActions`). */
	bool FindGenericAction(const FName& AbilityId, FRTActionDef& OutDef, ERTAbilityShape& OutShape)
	{
		for (const URTActionData* A : URTCatalogLibrary::MakeGenericActions(GetTransientPackage()))
		{
			if (A && A->Def.ActionId == AbilityId)
			{
				OutDef = A->Def;
				OutShape = A->Shape;
				return true;
			}
		}
		return false;
	}

	FString TargetName(ERTAbilityLabTarget Kind)
	{
		return StaticEnum<ERTAbilityLabTarget>()->GetNameStringByValue(static_cast<int64>(Kind));
	}

	void TestTargetKind(FAutomationTestBase& Test, const FString& What, ERTAbilityLabTarget Actual, ERTAbilityLabTarget Expected)
	{
		Test.TestEqual(What, TargetName(Actual), TargetName(Expected));
	}

	/** Tutte le voci del TurnLog della run. */
	TArray<FRTTurnLogEntry> RunEntries(const FRTTestResult& Result)
	{
		TArray<FRTTurnLogEntry> Out;
		for (const FRTTurnTrace& Trace : Result.TurnTraces)
		{
			TArray<FRTTurnLogEntry> Entries;
			if (URTTurnLogLibrary::DeserializeTurnLog(Trace.Bytes, Entries)) { Out.Append(Entries); }
		}
		return Out;
	}

	/** Un campo intero di un'unita' prima e dopo la run, dal diff di stato. `false` se il diff non lo porta. */
	bool FieldBeforeAfter(const FRTTestResult& Result, const FString& ScenarioUnitId, const FName& Field,
		int32& OutBefore, int32& OutAfter)
	{
		for (const FRTUnitStateDiff& Diff : Result.StateDiff)
		{
			const FString* Authoring = Result.ScenarioIdByUnitId.Find(Diff.UnitId);
			if (!Authoring || *Authoring != ScenarioUnitId) { continue; }
			for (const FRTUnitFieldChange& C : Diff.Changes)
			{
				if (C.Field == Field)
				{
					OutBefore = FCString::Atoi(*C.Before);
					OutAfter = FCString::Atoi(*C.After);
					return true;
				}
			}
		}
		return false;
	}

	bool HasEnvironmentEvent(const TArray<FRTTurnLogEntry>& Entries, ERTEnvironmentOutcome Outcome)
	{
		for (const FRTTurnLogEntry& E : Entries)
		{
			if (E.Category == ERTLogCategory::Environment && E.Outcome == static_cast<uint8>(Outcome)) { return true; }
		}
		return false;
	}
}

/**
 * **La scelta del bersaglio, un caso per tipo** (`#4072`): un attacco mira un nemico, una cura un alleato, un'area una
 * cella vuota, la copertura portatile il bordo verso chi lancia, `Action.Interact` il bordo con la porta, una generica
 * su se stessi nessuno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabChoosesTheTargetFromTheActionTest,
	"RefactorTactics.AbilityLab.ChoosesTheTargetFromTheAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabChoosesTheTargetFromTheActionTest::RunTest(const FString&)
{
	using namespace RTAbilityLabTestsInternal;
	const FRTAbilityLabFixtureSpec Spec;
	FRTActionDef Def;
	ERTAbilityShape Shape = ERTAbilityShape::Single;

	if (TestTrue(TEXT("premessa: LinearDischarge e' nel kit"), FindKitAction(TEXT("Hero.Gadget.LinearDischarge"), Def, Shape)))
	{
		TestTargetKind(*this, TEXT("LinearDischarge: un nemico"), URTAbilityLabLibrary::ChooseTarget(Def, Shape, Spec).Kind,
			ERTAbilityLabTarget::Enemy);
	}

	// La cura di catalogo: nessun eroe la impugna col suo id, ma la regola si legge dal suo `Def`.
	const FRTActionDef Heal = URTCatalogLibrary::FindCoreAction(TEXT("Action.Heal"));
	if (TestEqual(TEXT("premessa: Action.Heal e' nel catalogo"), Heal.ActionId, FName(TEXT("Action.Heal"))))
	{
		const FRTAbilityLabTargetChoice C = URTAbilityLabLibrary::ChooseTarget(Heal, ERTAbilityShape::Single, Spec);
		TestTargetKind(*this, TEXT("Action.Heal: un alleato"), C.Kind, ERTAbilityLabTarget::Ally);
		TestTrue(TEXT("Action.Heal: nella squadra di chi lancia"), C.bSecondHeroIsAlly);
	}

	// Un'area: una cella VUOTA a portata, oltre il bersaglio sulla linea da chi lancia.
	if (TestTrue(TEXT("premessa: Overload e' nel kit"), FindKitAction(TEXT("Hero.Gadget.Overload"), Def, Shape))
		&& TestTrue(TEXT("premessa: Overload e' un'area"), Shape == ERTAbilityShape::Area))
	{
		const FRTAbilityLabTargetChoice C = URTAbilityLabLibrary::ChooseTarget(Def, Shape, Spec);
		TestTargetKind(*this, TEXT("Overload: una cella"), C.Kind, ERTAbilityLabTarget::Cell);
		TestTrue(FString::Printf(TEXT("Overload: la cella oltre il bersaglio, (3,0) (e' %s)"), *C.Cell.ToString()),
			C.Cell == FRTCellId(3, 0));
		TestFalse(TEXT("Overload: un'area che fa danno prende un nemico"), C.bSecondHeroIsAlly);
	}

	// Una cura ad area: una cella, e il secondo eroe e' un alleato.
	if (TestTrue(TEXT("premessa: CircularTide e' nel kit"), FindKitAction(TEXT("Hero.Muiren.CircularTide"), Def, Shape)))
	{
		const FRTAbilityLabTargetChoice C = URTAbilityLabLibrary::ChooseTarget(Def, Shape, Spec);
		TestTargetKind(*this, TEXT("CircularTide: una cella"), C.Kind, ERTAbilityLabTarget::Cell);
		TestTrue(TEXT("CircularTide: la cura ad area prende un alleato"), C.bSecondHeroIsAlly);
	}

	// La copertura portatile: l'azione che il gadget concede, sul bordo della cella del bersaglio rivolto a chi lancia.
	const URTEquipmentData* Gadget = URTCatalogLibrary::MakePortableCoverGadget();
	const FRTActionDef Cover = Gadget ? URTCatalogLibrary::FindCoreAction(Gadget->GrantedActionId) : FRTActionDef();
	if (TestTrue(TEXT("premessa: il gadget concede un'azione di catalogo"), !Cover.ActionId.IsNone()))
	{
		const FRTAbilityLabTargetChoice C = URTAbilityLabLibrary::ChooseTarget(Cover, ERTAbilityShape::Single, Spec);
		TestTargetKind(*this, TEXT("Gadget.PortableCover: un bordo"), C.Kind, ERTAbilityLabTarget::Edge);
		TestTrue(FString::Printf(TEXT("Gadget.PortableCover: sulla cella del bersaglio (e' %s)"), *C.Cell.ToString()),
			C.Cell == Spec.TargetCell);
		TestTrue(TEXT("Gadget.PortableCover: il lato rivolto a chi lancia, W"), C.Edge == ERTHexDirection::W);
		TestFalse(TEXT("Gadget.PortableCover: nessuna porta"), C.bPlacesDoor);
	}

	// `Action.Interact` ha portata 1: il bordo e' sulla cella accanto a chi lancia, e la scena posa la porta.
	if (TestTrue(TEXT("premessa: Interact e' una generica"), FindGenericAction(TEXT("Action.Interact"), Def, Shape)))
	{
		const FRTAbilityLabTargetChoice C = URTAbilityLabLibrary::ChooseTarget(Def, Shape, Spec);
		TestTargetKind(*this, TEXT("Action.Interact: un bordo"), C.Kind, ERTAbilityLabTarget::Edge);
		TestTrue(FString::Printf(TEXT("Action.Interact: la cella accanto a chi lancia, (1,0) (e' %s)"), *C.Cell.ToString()),
			C.Cell == FRTCellId(1, 0));
		TestTrue(TEXT("Action.Interact: il lato rivolto a chi lancia, W"), C.Edge == ERTHexDirection::W);
		TestTrue(TEXT("Action.Interact: la scena posa la porta"), C.bPlacesDoor);
		TestTrue(TEXT("Action.Interact: la porta dalla parte di chi lancia, (0,0) E"),
			C.DoorCell == Spec.CasterCell && C.DoorEdge == ERTHexDirection::E);
	}

	if (TestTrue(TEXT("premessa: Guard e' una generica"), FindGenericAction(TEXT("Action.Guard"), Def, Shape)))
	{
		TestTargetKind(*this, TEXT("Action.Guard: nessun bersaglio"), URTAbilityLabLibrary::ChooseTarget(Def, Shape, Spec).Kind,
			ERTAbilityLabTarget::None);
	}

	// Una REAZIONE che riduce il danno la provoca un nemico: la riduzione la riceve chi reagisce, non il bersaglio.
	if (TestTrue(TEXT("premessa: Deflection e' nel kit"), FindKitAction(TEXT("Hero.Ivrin.Deflection"), Def, Shape))
		&& TestTrue(TEXT("premessa: Deflection e' una reazione"), Def.Slot == ERTActionSlot::Reaction)
		&& TestTrue(TEXT("premessa: e i suoi effetti, da soli, farebbero bene"), URTAbilityLabLibrary::BenefitsItsRecipient(Def)))
	{
		const FRTAbilityLabTargetChoice C = URTAbilityLabLibrary::ChooseTarget(Def, Shape, Spec);
		TestTargetKind(*this, TEXT("Deflection: un nemico"), C.Kind, ERTAbilityLabTarget::Enemy);
		TestFalse(TEXT("Deflection: il secondo eroe non e' un alleato"), C.bSecondHeroIsAlly);
	}

	// Il bersaglio sul bordo della mappa: la cella oltre esce, e si ripiega su quella PRIMA del bersaglio, non su quella
	// accanto a chi lancia (che metterebbe chi lancia nell'area e il bersaglio fuori).
	if (FindKitAction(TEXT("Hero.Gadget.Overload"), Def, Shape))
	{
		FRTAbilityLabFixtureSpec AlBordo;
		AlBordo.TargetCell = FRTCellId(3, 0);
		const FRTAbilityLabTargetChoice C = URTAbilityLabLibrary::ChooseTarget(Def, Shape, AlBordo);
		TestTrue(FString::Printf(TEXT("Overload col bersaglio in (3,0): la cella prima, (2,0) (e' %s)"), *C.Cell.ToString()),
			C.Cell == FRTCellId(2, 0));
	}

	// La portata minima conta come la conta il resolver: nessuna delle due celle accanto al bersaglio e' abbastanza
	// lontana, e si resta sulla cella del bersaglio.
	FRTActionDef Mortaio;
	Mortaio.RangeCells = 5;
	Mortaio.MinRangeCells = 4;
	const FRTAbilityLabTargetChoice Lontano = URTAbilityLabLibrary::ChooseTarget(Mortaio, ERTAbilityShape::Area, Spec);
	TestTrue(FString::Printf(TEXT("portata minima 4: nessuna cella accanto al bersaglio va bene, resta (2,0) (e' %s)"),
		*Lontano.Cell.ToString()), Lontano.Cell == Spec.TargetCell);

	// La regola dell'alleato, sui casi di confine: tutti gli effetti devono fare bene, e ce ne deve essere almeno uno.
	FRTActionDef Misto;
	Misto.Effects = { FRTActionEffectSpec(ERTActionEffect::Heal, 10), FRTActionEffectSpec(ERTActionEffect::Damage, 10) };
	FRTActionDef Vuoto;
	FRTActionDef Scudo;
	Scudo.Effects = { FRTActionEffectSpec(ERTActionEffect::Shield, 10), FRTActionEffectSpec(ERTActionEffect::CancelStatus, 0) };
	TestTrue(TEXT("scudo e purificazione fanno bene"), URTAbilityLabLibrary::BenefitsItsRecipient(Scudo));
	TestFalse(TEXT("cura e danno insieme non sono un aiuto"), URTAbilityLabLibrary::BenefitsItsRecipient(Misto));
	TestFalse(TEXT("un'azione senza effetti non e' un aiuto"), URTAbilityLabLibrary::BenefitsItsRecipient(Vuoto));
	return true;
}

/**
 * **La forzatura** (`#4072`, [D-607] (2)): la spec sceglie il bersaglio a mano, e la scena lo segue — la squadra del
 * secondo eroe e l'intento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabTheSpecForcesTheTargetTest,
	"RefactorTactics.AbilityLab.TheSpecForcesTheTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabTheSpecForcesTheTargetTest::RunTest(const FString&)
{
	using namespace RTAbilityLabTestsInternal;
	const FName Attacco(TEXT("Hero.Gadget.LinearDischarge"));
	FRTActionDef Def;
	ERTAbilityShape Shape = ERTAbilityShape::Single;
	if (!TestTrue(TEXT("premessa: LinearDischarge e' nel kit"), FindKitAction(Attacco, Def, Shape))) { return false; }

	for (const ERTAbilityLabTarget Forzato : { ERTAbilityLabTarget::Enemy, ERTAbilityLabTarget::Ally, ERTAbilityLabTarget::Cell,
		ERTAbilityLabTarget::Edge, ERTAbilityLabTarget::None })
	{
		FRTAbilityLabFixtureSpec Spec;
		Spec.Target = Forzato;
		TestTargetKind(*this, FString::Printf(TEXT("forzato %s"), *TargetName(Forzato)),
			URTAbilityLabLibrary::ChooseTarget(Def, Shape, Spec).Kind, Forzato);
	}

	// La scena segue la forzatura: un attacco forzato su un alleato posa il secondo eroe nella squadra di chi lancia.
	FRTAbilityLabFixtureSpec SuAlleato;
	SuAlleato.Target = ERTAbilityLabTarget::Ally;
	FRTTestScenario Scenario;
	FString Errore;
	if (TestTrue(TEXT("la scena forzata si costruisce"), URTAbilityLabLibrary::BuildFixture(Attacco, SuAlleato, Scenario, Errore))
		&& TestEqual(TEXT("tre unita': chi lancia, l'alleato e un terzo eroe contro"), Scenario.Units.Num(), 3))
	{
		TestEqual(TEXT("il secondo eroe e' nella squadra di chi lancia"), Scenario.Units[1].TeamId, Scenario.Units[0].TeamId);
		TestEqual(TEXT("e l'intento lo mira"), Scenario.Turns[0].Intents[0].Target, FString(TEXT("TARGET")));
		// Senza una seconda squadra la partita finirebbe al primo Cleanup, e in PIE si aprirebbe il risultato.
		TestNotEqual(TEXT("il terzo eroe e' dell'altra squadra"), Scenario.Units[2].TeamId, Scenario.Units[0].TeamId);
		TestTrue(TEXT("e non sta su nessuna delle due celle della scena"),
			Scenario.Units[2].Cell != Scenario.Units[0].Cell && Scenario.Units[2].Cell != Scenario.Units[1].Cell);
	}

	// «Nessuno» forzato su un'azione che chiede un bersaglio: fail closed, come un id sconosciuto. L'harness la
	// rifiuterebbe alla run, dopo aver posato una scena che non poteva girare.
	FRTAbilityLabFixtureSpec Senza;
	Senza.Target = ERTAbilityLabTarget::None;
	FRTTestScenario Nessuno;
	Errore.Reset();
	TestFalse(TEXT("«nessuno» su un attacco non si costruisce"), URTAbilityLabLibrary::BuildFixture(Attacco, Senza, Nessuno, Errore));
	TestTrue(FString::Printf(TEXT("e il rifiuto nomina l'azione (%s)"), *Errore), Errore.Contains(Attacco.ToString()));
	TestEqual(TEXT("fail closed: nessuna unita' posata"), Nessuno.Units.Num(), 0);

	// Su un'azione su se stessi «nessuno» e' la sua forma, e la scena si costruisce.
	FRTTestScenario Guardia;
	TestTrue(TEXT("«nessuno» su Guard si costruisce"),
		URTAbilityLabLibrary::BuildFixture(TEXT("Action.Guard"), Senza, Guardia, Errore));
	return true;
}

/**
 * **Ogni tipo di bersaglio gira nell'harness, e la skill agisce** (`#4072`): nessun ERROR, un turno giocato, e l'effetto
 * dove deve stare — la vita del nemico scende, lo scudo dell'alleato sale, l'area prende il bersaglio, la copertura nasce,
 * la porta si apre, la generica lascia la sua voce.
 *
 * ⚠️ **Le azioni sono di kit o generiche**, perche' il banco esegue solo cio' che un'unita' impugna: `Action.Heal` e il
 * gadget li prova il test sopra sulla scelta. Qui l'alleato lo prende `Hero.Muiren.CircularTide`, una cura ad area:
 * nei kit non c'e' un aiuto su un'altra unita' singola (`TideGuard` e `PhaseGuard` derivano da `Action.Shield`, che e'
 * su se stessi). Il bordo lo prende `Hero.Riktor.KineticPanel`, che dichiara la stessa `StructureOp` del gadget.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabEachTargetKindRunsAndActsTest,
	"RefactorTactics.AbilityLab.EachTargetKindRunsAndActs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabEachTargetKindRunsAndActsTest::RunTest(const FString&)
{
	using namespace RTAbilityLabTestsInternal;

	auto Esegui = [this](const TCHAR* AbilityId, FRTTestScenario& OutScenario, FRTTestResult& OutResult) -> bool
	{
		FString Errore;
		const bool bCostruita = URTAbilityLabLibrary::BuildFixture(FName(AbilityId), FRTAbilityLabFixtureSpec(), OutScenario, Errore);
		if (!TestTrue(FString::Printf(TEXT("%s: la scena si costruisce (%s)"), AbilityId, *Errore), bCostruita))
		{
			return false;
		}
		OutResult = RunFixture(*this, OutScenario);
		const bool bOk = TestFalse(FString::Printf(TEXT("%s: nessun ERROR (%s)"), AbilityId, *OutResult.ErrorMessage),
			OutResult.Outcome == ERTTestOutcome::Error);
		TestEqual(FString::Printf(TEXT("%s: un turno giocato"), AbilityId), OutResult.TurnsPlayed, 1);
		return bOk;
	};

	FRTTestScenario Scenario;
	FRTTestResult Result;
	int32 Prima = 0;
	int32 Dopo = 0;

	// Nemico: la vita del bersaglio scende.
	if (Esegui(TEXT("Hero.Gadget.LinearDischarge"), Scenario, Result))
	{
		TestTrue(TEXT("LinearDischarge: la vita del nemico scende"),
			FieldBeforeAfter(Result, TEXT("TARGET"), TEXT("Health"), Prima, Dopo) && Dopo < Prima);
	}

	// Alleato: una cura ad area, che e' l'unico aiuto su un'altra unita' che un eroe impugna. L'alleato e' ferito, e la
	// sua vita sale.
	if (Esegui(TEXT("Hero.Muiren.CircularTide"), Scenario, Result))
	{
		TestEqual(TEXT("CircularTide: il secondo eroe e' un alleato"), Scenario.Units[1].TeamId, Scenario.Units[0].TeamId);
		TestTrue(TEXT("CircularTide: e' ferito, o la cura non si vedrebbe"), Scenario.Units[1].Health > 0);
		TestTrue(TEXT("CircularTide: c'e' una seconda squadra"),
			Scenario.Units.Num() == 3 && Scenario.Units[2].TeamId != Scenario.Units[0].TeamId);
		TestTrue(TEXT("CircularTide: la vita dell'alleato sale"),
			FieldBeforeAfter(Result, TEXT("TARGET"), TEXT("Health"), Prima, Dopo) && Dopo > Prima);
	}

	// Cella: l'area centrata oltre il bersaglio lo prende.
	if (Esegui(TEXT("Hero.Gadget.Overload"), Scenario, Result))
	{
		TestTrue(TEXT("Overload: mira una cella"), Scenario.Turns[0].Intents[0].bTargetsCell);
		TestTrue(TEXT("Overload: l'area prende il nemico"),
			FieldBeforeAfter(Result, TEXT("TARGET"), TEXT("Health"), Prima, Dopo) && Dopo < Prima);
	}

	// Bordo: la copertura nasce.
	if (Esegui(TEXT("Hero.Riktor.KineticPanel"), Scenario, Result))
	{
		TestTrue(TEXT("KineticPanel: dichiara il bordo"), Scenario.Turns[0].Intents[0].bHasCoverEdge);
		TestTrue(TEXT("KineticPanel: la copertura nasce"), HasEnvironmentEvent(RunEntries(Result), ERTEnvironmentOutcome::CoverCreated));
	}

	// Porta: la scena la posa chiusa, e l'interazione la apre.
	if (Esegui(TEXT("Action.Interact"), Scenario, Result))
	{
		TestEqual(TEXT("Interact: la scena posa una porta"), Scenario.Doors.Num(), 1);
		TestTrue(TEXT("Interact: la porta si apre"), HasEnvironmentEvent(RunEntries(Result), ERTEnvironmentOutcome::DoorOpened));
	}

	// Nessun bersaglio: chi lancia si mette in guardia. La voce di stato porta il TAG dello stato, non l'id dell'azione.
	if (Esegui(TEXT("Action.Guard"), Scenario, Result))
	{
		const FRTScenarioIntent& I = Scenario.Turns[0].Intents[0];
		TestTrue(TEXT("Guard: l'intento non mira niente"), I.Target.IsEmpty() && !I.bTargetsCell);
		bool bInGuardia = false;
		for (const FRTTurnLogEntry& E : RunEntries(Result))
		{
			bInGuardia |= E.Category == ERTLogCategory::Status
				&& E.Outcome == static_cast<uint8>(ERTStatusOutcome::AppliedByAction)
				&& E.ActionId == FName(TEXT("Status.Guarded")) && E.SrcCell == Scenario.Units[0].Cell;
		}
		TestTrue(TEXT("Guard: chi lancia prende Status.Guarded da un'azione"), bInGuardia);
	}
	return true;
}

// =====================================================================================================
// `#4073` — le scene delle reazioni. Chi reagisce ARMA la reazione; il `TARGET` la provoca con un intento suo, scelto
// dal trigger del catalogo (`ProvocationFor`); il resolver fa il resto. La posa e' quella di default del banco.
// =====================================================================================================

namespace RTAbilityLabTestsInternal
{
	FString ProvocationName(ERTAbilityLabProvocation P)
	{
		return StaticEnum<ERTAbilityLabProvocation>()->GetNameStringByValue(static_cast<int64>(P));
	}

	/** La provocazione che un trigger chiede, scritta QUI e non letta da `ProvocationFor`: e' l'oracolo della tabella. */
	ERTAbilityLabProvocation ExpectedProvocationFor(ERTReactionTrigger Trigger)
	{
		switch (Trigger)
		{
		case ERTReactionTrigger::HitByDirectAttack:     return ERTAbilityLabProvocation::DirectHit;
		case ERTReactionTrigger::AllyHitByDirectAttack: return ERTAbilityLabProvocation::AllyHit;
		case ERTReactionTrigger::AboutToBeDisplaced:    return ERTAbilityLabProvocation::Push;
		case ERTReactionTrigger::AboutToReceiveControl: return ERTAbilityLabProvocation::Control;
		case ERTReactionTrigger::CellBecameHazardous:   return ERTAbilityLabProvocation::Hazard;
		default:                                        return ERTAbilityLabProvocation::None;
		}
	}

	/** L'intento di un'unita' della scena, o `nullptr`. */
	const FRTScenarioIntent* IntentOf(const FRTTestScenario& Scenario, const FString& UnitId)
	{
		for (const FRTScenarioTurn& Turn : Scenario.Turns)
		{
			for (const FRTScenarioIntent& Intent : Turn.Intents)
			{
				if (Intent.UnitId == UnitId) { return &Intent; }
			}
		}
		return nullptr;
	}

	const FRTScenarioUnit* UnitOf(const FRTTestScenario& Scenario, const FString& UnitId)
	{
		for (const FRTScenarioUnit& U : Scenario.Units)
		{
			if (U.Id == UnitId) { return &U; }
		}
		return nullptr;
	}

	/** Vero se il TurnLog ha una voce di `Category` per `ActionId`, con `Outcome` se dato (altrimenti qualunque). */
	bool HasEntry(const TArray<FRTTurnLogEntry>& Entries, ERTLogCategory Category, const FName& ActionId, int32 Outcome = INDEX_NONE)
	{
		for (const FRTTurnLogEntry& E : Entries)
		{
			if (E.Category == Category && E.ActionId == ActionId && (Outcome == INDEX_NONE || E.Outcome == Outcome)) { return true; }
		}
		return false;
	}
}

/**
 * **La provocazione si legge dal catalogo** (`#4073`): per ogni reazione dei kit, quella del suo trigger; l'Overwatch
 * entra nella linea, il Brace e' una spinta; le generiche che non reagiscono non hanno provocazione. Anche i trigger che
 * oggi ascoltano solo moduli d'equipaggiamento hanno la loro riga.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabReadsTheProvocationFromTheCatalogTest,
	"RefactorTactics.AbilityLab.ReadsTheProvocationFromTheCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabReadsTheProvocationFromTheCatalogTest::RunTest(const FString&)
{
	using namespace RTAbilityLabTestsInternal;

	// Ogni reazione dei kit: il catalogo, non un elenco di id.
	int32 Reazioni = 0;
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Hero) { continue; }
		for (const TObjectPtr<URTActionData>& Ptr : Hero->Actions)
		{
			if (!Ptr || Ptr->Def.Slot != ERTActionSlot::Reaction) { continue; }
			++Reazioni;
			TestEqual(FString::Printf(TEXT("%s: la provocazione del suo trigger"), *Ptr->Def.ActionId.ToString()),
				ProvocationName(URTAbilityLabLibrary::ProvocationFor(Ptr->Def)),
				ProvocationName(ExpectedProvocationFor(Ptr->Def.ReactionTrigger)));
		}
	}
	TestTrue(FString::Printf(TEXT("i kit hanno reazioni (%d)"), Reazioni), Reazioni > 0);

	// Ogni trigger, anche quelli dei soli moduli.
	for (const ERTReactionTrigger Trigger : { ERTReactionTrigger::HitByDirectAttack, ERTReactionTrigger::AllyHitByDirectAttack,
		ERTReactionTrigger::AboutToBeDisplaced, ERTReactionTrigger::AboutToReceiveControl, ERTReactionTrigger::CellBecameHazardous })
	{
		FRTActionDef Reazione;
		Reazione.Slot = ERTActionSlot::Reaction;
		Reazione.ReactionTrigger = Trigger;
		TestEqual(FString::Printf(TEXT("trigger %d"), static_cast<int32>(Trigger)),
			ProvocationName(URTAbilityLabLibrary::ProvocationFor(Reazione)), ProvocationName(ExpectedProvocationFor(Trigger)));
	}

	// Le principali che reagiscono, e quelle che no.
	FRTActionDef Def;
	ERTAbilityShape Shape = ERTAbilityShape::Single;
	struct FAttesa { const TCHAR* Id; ERTAbilityLabProvocation Provocazione; };
	for (const FAttesa& A : { FAttesa{ TEXT("Action.Overwatch"), ERTAbilityLabProvocation::EnterLine },
		FAttesa{ TEXT("Action.Brace"), ERTAbilityLabProvocation::Push }, FAttesa{ TEXT("Action.Guard"), ERTAbilityLabProvocation::None },
		FAttesa{ TEXT("Action.Interact"), ERTAbilityLabProvocation::None } })
	{
		if (TestTrue(FString::Printf(TEXT("premessa: %s e' una generica"), A.Id), FindGenericAction(FName(A.Id), Def, Shape)))
		{
			TestEqual(FString::Printf(TEXT("%s"), A.Id), ProvocationName(URTAbilityLabLibrary::ProvocationFor(Def)),
				ProvocationName(A.Provocazione));
		}
	}
	if (TestTrue(TEXT("premessa: LinearDischarge e' nel kit"), FindKitAction(TEXT("Hero.Gadget.LinearDischarge"), Def, Shape)))
	{
		TestEqual(TEXT("un attacco non e' una reazione"), ProvocationName(URTAbilityLabLibrary::ProvocationFor(Def)),
			ProvocationName(ERTAbilityLabProvocation::None));
	}

	// Ogni predittiva dei kit (`#4088`): il `TARGET` entra nella cella bloccata. Il catalogo, non un elenco di id.
	int32 Predittive = 0;
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Hero) { continue; }
		for (const TObjectPtr<URTActionData>& Ptr : Hero->Actions)
		{
			if (!Ptr || Ptr->Def.PredictiveTargeting == ERTPredictiveTargeting::None) { continue; }
			++Predittive;
			TestEqual(FString::Printf(TEXT("%s: chi provoca entra nella cella"), *Ptr->Def.ActionId.ToString()),
				ProvocationName(URTAbilityLabLibrary::ProvocationFor(Ptr->Def)), ProvocationName(ERTAbilityLabProvocation::EnterCell));
		}
	}
	TestTrue(FString::Printf(TEXT("i kit hanno predittive (%d)"), Predittive), Predittive > 0);
	return true;
}

/**
 * **Ogni reazione che il banco esegue viene provocata, e scatta** (`#4073`): `Counter` e `Deflect` colpiti, `Intercept`
 * con un alleato colpito, l'Overwatch con un nemico che entra nella linea, il Brace spinto con la finestra.
 *
 * Il segno che scatta e' nel TurnLog: `Reaction/Activated` per una reazione dello slot; `ReactionDecision` per
 * l'Overwatch e il Brace, che rispondono in una finestra e non scrivono `Activated`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabEachReactionIsProvokedAndFiresTest,
	"RefactorTactics.AbilityLab.EachReactionIsProvokedAndFires",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabEachReactionIsProvokedAndFiresTest::RunTest(const FString&)
{
	using namespace RTAbilityLabTestsInternal;

	auto Esegui = [this](const TCHAR* AbilityId, FRTTestScenario& OutScenario, TArray<FRTTurnLogEntry>& OutEntries) -> bool
	{
		FString Errore;
		const bool bCostruita = URTAbilityLabLibrary::BuildFixture(FName(AbilityId), FRTAbilityLabFixtureSpec(), OutScenario, Errore);
		if (!TestTrue(FString::Printf(TEXT("%s: la scena si costruisce (%s)"), AbilityId, *Errore), bCostruita)) { return false; }
		const FRTTestResult Result = RunFixture(*this, OutScenario);
		const bool bOk = TestFalse(FString::Printf(TEXT("%s: nessun ERROR (%s)"), AbilityId, *Result.ErrorMessage),
			Result.Outcome == ERTTestOutcome::Error);
		OutEntries = RunEntries(Result);
		return bOk;
	};

	const int32 Attivata = static_cast<int32>(ERTReactionOutcome::Activated);
	FRTTestScenario Scenario;
	TArray<FRTTurnLogEntry> Voci;

	// Le reazioni dello slot: chi reagisce la ARMA, e il `TARGET` lo colpisce col suo attacco base.
	for (const TCHAR* Id : { TEXT("Hero.Gadget.ReactiveCapacitor"), TEXT("Hero.Ivrin.Deflection") })
	{
		if (!Esegui(Id, Scenario, Voci)) { continue; }
		const FRTScenarioIntent* Arma = IntentOf(Scenario, TEXT("CASTER"));
		const FRTScenarioIntent* Provoca = IntentOf(Scenario, TEXT("TARGET"));
		TestTrue(FString::Printf(TEXT("%s: chi reagisce la arma, e non la usa"), Id),
			Arma && Arma->Reaction == FName(Id) && Arma->Ability.IsNone());
		TestTrue(FString::Printf(TEXT("%s: il TARGET colpisce chi reagisce"), Id),
			Provoca && !Provoca->Ability.IsNone() && Provoca->Target == TEXT("CASTER"));
		TestTrue(FString::Printf(TEXT("%s: scatta (Reaction/Activated)"), Id), HasEntry(Voci, ERTLogCategory::Reaction, FName(Id), Attivata));
		TestFalse(FString::Printf(TEXT("%s: una reazione dello slot non apre finestre del giocatore"), Id), Scenario.bPlayerReactionWindows);
	}

	// Il bersaglio di una reazione non si forza: un `TARGET` alleato provocherebbe la propria squadra.
	{
		FRTAbilityLabFixtureSpec Forzata;
		Forzata.Target = ERTAbilityLabTarget::Ally;
		FRTTestScenario Rifiutata;
		FString Errore;
		TestFalse(TEXT("una reazione col bersaglio forzato non si costruisce"),
			URTAbilityLabLibrary::BuildFixture(TEXT("Hero.Gadget.ReactiveCapacitor"), Forzata, Rifiutata, Errore));
		TestTrue(FString::Printf(TEXT("e il rifiuto la nomina (%s)"), *Errore), Errore.Contains(TEXT("Hero.Gadget.ReactiveCapacitor")));
	}

	// Chi provoca deve arrivare da dove sta: col bersaglio in (3,0) nessun attacco base che spinge ci arriva (quello di
	// Riktor, con `Weapon.Impact`, ha portata 2), e la scena del Brace rifiuta invece di posare una spinta che non arriva.
	{
		FRTAbilityLabFixtureSpec Lontano;
		Lontano.TargetCell = FRTCellId(3, 0);
		FRTTestScenario Rifiutata;
		FString Errore;
		TestFalse(TEXT("Brace col bersaglio fuori dalla portata di chi spinge: non si costruisce"),
			URTAbilityLabLibrary::BuildFixture(TEXT("Action.Brace"), Lontano, Rifiutata, Errore));
	}

	// L'intercettazione: un alleato accanto a chi reagisce, e il `TARGET` colpisce lui.
	if (Esegui(TEXT("Hero.Riktor.Interposition"), Scenario, Voci))
	{
		const FRTScenarioUnit* Alleato = UnitOf(Scenario, TEXT("ALLY"));
		const FRTScenarioIntent* Provoca = IntentOf(Scenario, TEXT("TARGET"));
		TestTrue(TEXT("Interposition: c'e' un alleato da proteggere"), Alleato && Alleato->TeamId == Scenario.Units[0].TeamId);
		TestTrue(TEXT("Interposition: il TARGET colpisce l'alleato"), Provoca && Provoca->Target == TEXT("ALLY"));
		TestTrue(TEXT("Interposition: scatta (Reaction/Activated)"),
			HasEntry(Voci, ERTLogCategory::Reaction, TEXT("Hero.Riktor.Interposition"), Attivata));
	}

	// L'Overwatch: chi guarda verso il bersaglio, il bersaglio entra nella linea, la finestra si apre.
	if (Esegui(TEXT("Action.Overwatch"), Scenario, Voci))
	{
		const FRTScenarioIntent* Provoca = IntentOf(Scenario, TEXT("TARGET"));
		TestTrue(TEXT("Overwatch: le finestre del giocatore sono aperte"), Scenario.bPlayerReactionWindows);
		TestTrue(TEXT("Overwatch: il TARGET fa un passo"), Provoca && Provoca->Move.Num() == 1);
		// Headless la finestra non ha chi la apra: `NoDecider` e' il segno che ne andava aperta una. Un `Immediate` o un
		// `CollapsedByCondition` scriverebbero la stessa categoria senza finestra. Che si apra davvero lo prova
		// `AbilityLab.ReactionWindowsOpenToTheController`.
		TestTrue(TEXT("Overwatch: ne va aperta una finestra (ReactionDecision/NoDecider)"),
			HasEntry(Voci, ERTLogCategory::ReactionDecision, TEXT("Action.Overwatch"),
				static_cast<int32>(ERTReactionDecisionOutcome::NoDecider)));
	}

	// Chi guarda guarda verso il bersaglio anche quando non e' a est, il verso di partenza di ogni unita'.
	{
		FRTAbilityLabFixtureSpec AOvest;
		AOvest.TargetCell = FRTCellId(-2, 0);
		FRTTestScenario Ovest;
		FString Errore;
		if (TestTrue(TEXT("Overwatch col bersaglio a ovest: la scena si costruisce"),
			URTAbilityLabLibrary::BuildFixture(TEXT("Action.Overwatch"), AOvest, Ovest, Errore)))
		{
			const FRTScenarioUnit* ChiGuarda = UnitOf(Ovest, TEXT("CASTER"));
			TestTrue(TEXT("Overwatch col bersaglio a ovest: chi guarda guarda a ovest"), ChiGuarda && ChiGuarda->Facing == ERTHexDirection::W);
		}
	}

	// Il Brace: lo impugna chi ne apre la finestra, e il `TARGET` lo spinge.
	if (Esegui(TEXT("Action.Brace"), Scenario, Voci))
	{
		const FRTScenarioUnit* ChiReagisce = UnitOf(Scenario, TEXT("CASTER"));
		const URTHeroData* Eroe = nullptr;
		for (const URTHeroData* H : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (H && ChiReagisce && H->HeroId == ChiReagisce->HeroId) { Eroe = H; }
		}
		TestTrue(TEXT("Brace: chi lo impugna ne apre la finestra (almeno due risposte)"),
			Eroe && URTCatalogLibrary::BraceExecutableResponses(Eroe->ReactionProfileId).Num() >= 2);
		TestTrue(TEXT("Brace: le finestre del giocatore sono aperte"), Scenario.bPlayerReactionWindows);
		const FRTScenarioIntent* Provoca = IntentOf(Scenario, TEXT("TARGET"));
		TestTrue(TEXT("Brace: il TARGET colpisce chi lo usa"), Provoca && Provoca->Target == TEXT("CASTER"));
		TestTrue(TEXT("Brace: ne va aperta una finestra (ReactionDecision/NoDecider)"),
			HasEntry(Voci, ERTLogCategory::ReactionDecision, TEXT("Action.Brace"),
				static_cast<int32>(ERTReactionDecisionOutcome::NoDecider)));
	}
	return true;
}

/**
 * **La scena di una predittiva blocca una cella, e chi ci entra viene colpito** (`#4088`). Il banco mira una cella perche'
 * il puntatore la chiede (`TargetKindForAction`), e fa entrare il `TARGET` (`ProvocationFor`): il TurnLog porta la
 * previsione azzeccata, e la vita del `TARGET` scende.
 *
 * 🔴 Prima di `#4088` la scena di `InterceptShot` mirava un'unita': il turno non armava niente, e il banco restava verde
 * perche' `EveryKitAbilityRunsWithoutError` guarda solo che la run non dia ERROR.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabThePredictiveCatchesWhoEntersTest,
	"RefactorTactics.AbilityLab.ThePredictiveCatchesWhoEntersItsCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabThePredictiveCatchesWhoEntersTest::RunTest(const FString&)
{
	using namespace RTAbilityLabTestsInternal;
	const FName Intercetto(TEXT("Hero.Ivrin.InterceptShot"));
	const FRTAbilityLabFixtureSpec Posa;

	FRTTestScenario Scenario;
	FString Errore;
	const bool bCostruita = URTAbilityLabLibrary::BuildFixture(Intercetto, Posa, Scenario, Errore);
	if (!TestTrue(FString::Printf(TEXT("la scena si costruisce (%s)"), *Errore), bCostruita))
	{
		return false;
	}

	const FRTScenarioIntent* Blocca = IntentOf(Scenario, TEXT("CASTER"));
	const FRTScenarioIntent* Entra = IntentOf(Scenario, TEXT("TARGET"));
	if (!TestNotNull(TEXT("chi lancia ha un intento"), Blocca) || !TestNotNull(TEXT("il TARGET ha un intento"), Entra))
	{
		return false;
	}
	const FRTCellId Bloccata = Blocca->TargetCell;
	TestTrue(TEXT("chi lancia usa InterceptShot su una CELLA"), Blocca->Ability == Intercetto && Blocca->bTargetsCell);
	TestTrue(TEXT("che non e' quella di chi lancia ne' quella del TARGET"),
		Bloccata != Posa.CasterCell && Bloccata != Posa.TargetCell);
	TestTrue(TEXT("il TARGET ci entra, con un passo"), Entra->Move.Num() == 1 && Entra->Move[0] == Bloccata);
	TestFalse(TEXT("una predittiva non apre finestre del giocatore"), Scenario.bPlayerReactionWindows);

	const FRTTestResult Result = RunFixture(*this, Scenario);
	TestFalse(FString::Printf(TEXT("nessun ERROR (%s)"), *Result.ErrorMessage), Result.Outcome == ERTTestOutcome::Error);
	bool bAzzeccata = false;
	for (const FRTTurnLogEntry& E : RunEntries(Result))
	{
		bAzzeccata |= E.Category == ERTLogCategory::Predictive && E.ActionId == Intercetto
			&& E.Outcome == static_cast<uint8>(ERTPredictiveOutcome::TriggerMatched) && E.TgtCell == Bloccata;
	}
	TestTrue(TEXT("la previsione si arma e colpisce chi entra (Predictive/TriggerMatched sulla cella)"), bAzzeccata);
	int32 Prima = 0;
	int32 Dopo = 0;
	TestTrue(TEXT("la vita del TARGET scende"), FieldBeforeAfter(Result, TEXT("TARGET"), TEXT("Health"), Prima, Dopo) && Dopo < Prima);

	// Il bersaglio si forza solo a «cella», la sua forma: un'unita' forzata pianificherebbe un colpo che il turno non arma.
	{
		FRTAbilityLabFixtureSpec Forzata;
		Forzata.Target = ERTAbilityLabTarget::Enemy;
		FRTTestScenario Rifiutata;
		Errore.Reset();
		TestFalse(TEXT("forzata su un nemico: non si costruisce"), URTAbilityLabLibrary::BuildFixture(Intercetto, Forzata, Rifiutata, Errore));
		TestTrue(FString::Printf(TEXT("e il rifiuto la nomina (%s)"), *Errore), Errore.Contains(Intercetto.ToString()));

		FRTAbilityLabFixtureSpec ACella;
		ACella.Target = ERTAbilityLabTarget::Cell;
		FRTTestScenario Forzata2;
		Errore.Reset();
		const bool bACella = URTAbilityLabLibrary::BuildFixture(Intercetto, ACella, Forzata2, Errore);
		const FRTScenarioIntent* BloccaForzata = bACella ? IntentOf(Forzata2, TEXT("CASTER")) : nullptr;
		TestTrue(FString::Printf(TEXT("forzata a «cella»: si costruisce (%s)"), *Errore), bACella);
		TestTrue(TEXT("forzata a «cella»: la stessa cella di Auto"), BloccaForzata && BloccaForzata->TargetCell == Bloccata);
	}

	// Col TARGET accanto a chi lancia la linea offre solo la cella oltre, fuori portata 1, e quella di chi lancia: la scena
	// blocca un'altra cella accanto al TARGET, a portata. E' la posa di `EveryKitAbilityRunsWithoutError`.
	{
		FRTAbilityLabFixtureSpec Accanto;
		Accanto.TargetCell = FRTCellId(1, 0);
		FRTTestScenario Vicina;
		Errore.Reset();
		const bool bVicina = URTAbilityLabLibrary::BuildFixture(Intercetto, Accanto, Vicina, Errore);
		const FRTScenarioIntent* BloccaVicina = bVicina ? IntentOf(Vicina, TEXT("CASTER")) : nullptr;
		if (TestTrue(FString::Printf(TEXT("TARGET accanto a chi lancia: la scena si costruisce (%s)"), *Errore), bVicina)
			&& TestNotNull(TEXT("TARGET accanto a chi lancia: chi lancia ha un intento"), BloccaVicina))
		{
			const FRTCellId C = BloccaVicina->TargetCell;
			TestTrue(TEXT("TARGET accanto a chi lancia: la cella bloccata non e' la sua, ne' quella di chi lancia"),
				C != Accanto.TargetCell && C != Accanto.CasterCell);
			TestEqual(TEXT("TARGET accanto a chi lancia: e' a portata"), URTHexLibrary::HexDistance(Accanto.CasterCell, C), 1);
			TestEqual(TEXT("TARGET accanto a chi lancia: e accanto al TARGET"), URTHexLibrary::HexDistance(Accanto.TargetCell, C), 1);
		}
	}

	// Col TARGET a ovest il primo vicino nell'ordine delle direzioni e' la cella di chi lancia: il ripiego la salta.
	{
		FRTAbilityLabFixtureSpec AOvest;
		AOvest.TargetCell = FRTCellId(-1, 0);
		FRTTestScenario Ovest;
		Errore.Reset();
		const bool bOvest = URTAbilityLabLibrary::BuildFixture(Intercetto, AOvest, Ovest, Errore);
		const FRTScenarioIntent* BloccaOvest = bOvest ? IntentOf(Ovest, TEXT("CASTER")) : nullptr;
		if (TestTrue(FString::Printf(TEXT("TARGET a ovest: la scena si costruisce (%s)"), *Errore), bOvest)
			&& TestNotNull(TEXT("TARGET a ovest: chi lancia ha un intento"), BloccaOvest))
		{
			const FRTCellId C = BloccaOvest->TargetCell;
			TestTrue(TEXT("TARGET a ovest: la cella bloccata non e' quella di chi lancia, ne' la sua"),
				C != AOvest.CasterCell && C != AOvest.TargetCell);
			TestEqual(TEXT("TARGET a ovest: e' accanto a chi lancia"), URTHexLibrary::HexDistance(AOvest.CasterCell, C), 1);
			TestEqual(TEXT("TARGET a ovest: e accanto al TARGET"), URTHexLibrary::HexDistance(AOvest.TargetCell, C), 1);
		}
	}

	// Col TARGET dove nessuna cella accanto a lui e' a portata, la scena rifiuta invece di bloccare la cella su cui il
	// TARGET sta gia', che mancherebbe sempre.
	{
		FRTAbilityLabFixtureSpec Lontano;
		Lontano.TargetCell = FRTCellId(3, 0);
		FRTTestScenario Rifiutata;
		Errore.Reset();
		TestFalse(TEXT("TARGET lontano: non si costruisce"), URTAbilityLabLibrary::BuildFixture(Intercetto, Lontano, Rifiutata, Errore));
		TestTrue(FString::Printf(TEXT("e il rifiuto la nomina (%s)"), *Errore), Errore.Contains(Intercetto.ToString()));
		TestEqual(TEXT("fail closed: nessuna unita' posata"), Rifiutata.Units.Num(), 0);
	}
	return true;
}

/**
 * **Le finestre dell'Overwatch e del Brace si aprono al CONTROLLER** (`#4073`, review): la stessa scena del banco, eseguita
 * nei tre modi di `Scenario.PlayerReactionWindowsOpenToTheController`. Headless la decisione esce `NoDecider`; con un
 * `ARTPlayerController` la finestra si apre e, senza risposte, scade (`Timeout`); col controller ma senza la chiave
 * `playerReactionWindows` torna `NoDecider`.
 *
 * ⚠️ Per il Brace e' la prima volta che la sua finestra, nel Blast, si apre attraverso la sessione dello scenario con un
 * controller: fino a qui la provavano solo i test del turn manager (`Reactions.Brace.WindowSuspendsBlast`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabReactionWindowsOpenToTheControllerTest,
	"RefactorTactics.AbilityLab.ReactionWindowsOpenToTheController",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabReactionWindowsOpenToTheControllerTest::RunTest(const FString&)
{
	using namespace RTAbilityLabTestsInternal;

	auto Conta = [](const TArray<FRTTurnLogEntry>& Voci, const FName& ActionId, ERTReactionDecisionOutcome Esito)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : Voci)
		{
			N += (E.Category == ERTLogCategory::ReactionDecision && E.ActionId == ActionId && E.Outcome == static_cast<uint8>(Esito)) ? 1 : 0;
		}
		return N;
	};
	auto Esegui = [this](const FRTTestScenario& S, bool bConController) -> TArray<FRTTurnLogEntry>
	{
		UWorld* World = MakeAbilityLabWorld();
		if (!World) { AddError(TEXT("world non creato")); return {}; }
		if (bConController) { TestNotNull(TEXT("player controller"), World->SpawnActor<ARTPlayerController>()); }
		const FRTTestResult R = URTScenarioRunner::Run(World, S);
		DestroyAbilityLabWorld(World);
		TestFalse(FString::Printf(TEXT("%s: nessun ERROR (%s)"), *S.ScenarioId, *R.ErrorMessage), R.Outcome == ERTTestOutcome::Error);
		return RunEntries(R);
	};

	for (const TCHAR* Id : { TEXT("Action.Overwatch"), TEXT("Action.Brace") })
	{
		FRTTestScenario Scena;
		FString Errore;
		const bool bCostruita = URTAbilityLabLibrary::BuildFixture(FName(Id), FRTAbilityLabFixtureSpec(), Scena, Errore);
		if (!TestTrue(FString::Printf(TEXT("%s: la scena si costruisce (%s)"), Id, *Errore), bCostruita)
			|| !TestTrue(FString::Printf(TEXT("premessa: %s apre le finestre del giocatore"), Id), Scena.bPlayerReactionWindows))
		{
			continue;
		}
		const FName Azione(Id);

		const TArray<FRTTurnLogEntry> Headless = Esegui(Scena, false);
		TestEqual(FString::Printf(TEXT("%s headless: NoDecider"), Id), Conta(Headless, Azione, ERTReactionDecisionOutcome::NoDecider), 1);
		TestEqual(FString::Printf(TEXT("%s headless: nessun Timeout"), Id), Conta(Headless, Azione, ERTReactionDecisionOutcome::Timeout), 0);

		const TArray<FRTTurnLogEntry> ConController = Esegui(Scena, true);
		TestEqual(FString::Printf(TEXT("%s col controller: la finestra si apre e scade (Timeout)"), Id),
			Conta(ConController, Azione, ERTReactionDecisionOutcome::Timeout), 1);
		TestEqual(FString::Printf(TEXT("%s col controller: nessun NoDecider"), Id),
			Conta(ConController, Azione, ERTReactionDecisionOutcome::NoDecider), 0);

		FRTTestScenario Senza = Scena;
		Senza.bPlayerReactionWindows = false;
		const TArray<FRTTurnLogEntry> SenzaChiave = Esegui(Senza, true);
		TestEqual(FString::Printf(TEXT("%s col controller ma senza la chiave: NoDecider"), Id),
			Conta(SenzaChiave, Azione, ERTReactionDecisionOutcome::NoDecider), 1);
	}
	return true;
}

/** **Il passo `,` / `.` attraversa anche le reazioni del kit** (`#4073`): da una voce del kit si arriva a ognuna. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabStepReachesTheKitReactionsTest,
	"RefactorTactics.AbilityLab.StepReachesTheKitReactions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabStepReachesTheKitReactionsTest::RunTest(const FString&)
{
	int32 Controllate = 0;
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Hero || Hero->Actions.Num() == 0 || !Hero->Actions[0]) { continue; }
		TArray<FName> Reazioni;
		for (const TObjectPtr<URTActionData>& Ptr : Hero->Actions)
		{
			if (Ptr && Ptr->Def.Slot == ERTActionSlot::Reaction) { Reazioni.Add(Ptr->Def.ActionId); }
		}
		if (Reazioni.Num() == 0) { continue; }

		const FString Partenza = FString(TEXT("AbilityLab.")) + Hero->Actions[0]->Def.ActionId.ToString();
		TSet<FName> Raggiunte;
		FName Id;
		int32 Posizione = 0;
		int32 Quante = 0;
		if (!TestTrue(FString::Printf(TEXT("%s: il passo parte"), *Partenza),
			URTAbilityLabLibrary::PlanLabStep(Partenza, 1, Id, Posizione, Quante)))
		{
			continue;
		}
		for (int32 Passo = 1; Passo <= Quante; ++Passo)
		{
			if (URTAbilityLabLibrary::PlanLabStep(Partenza, Passo, Id, Posizione, Quante)) { Raggiunte.Add(Id); }
		}
		for (const FName& Reazione : Reazioni)
		{
			++Controllate;
			TestTrue(FString::Printf(TEXT("%s: il passo arriva a %s"), *Hero->HeroId.ToString(), *Reazione.ToString()),
				Raggiunte.Contains(Reazione));
		}
	}
	TestTrue(FString::Printf(TEXT("i kit hanno reazioni da raggiungere (%d)"), Controllate), Controllate > 0);
	return true;
}

// =====================================================================================================
// `#4136` — il banco posa 1, 2 o 3 bersagli. Il numero e' una POSA dello strumento: dove stanno i bersagli in piu' lo dice
// `URTHexCombatLibrary::HexHitCells`, la funzione che il gioco usa per le celle colpite, e chi viene colpito davvero lo
// decide il resolver. La posa e' quella di default del banco: chi lancia in (0,0), il bersaglio in (2,0), mappa di raggio 3.
// =====================================================================================================

namespace RTAbilityLabTargetsTestsInternal
{
	/** Gli Id dei bersagli, in ordine. Scritti qui e non letti dal banco: sono l'oracolo di cio' che il banco scrive. */
	const TCHAR* const TargetIds[] = { TEXT("TARGET"), TEXT("TARGET2"), TEXT("TARGET3") };

	/** L'unita' di scena con quell'Id, o `nullptr`. */
	const FRTScenarioUnit* UnitById(const FRTTestScenario& Scenario, const FString& UnitId)
	{
		return Scenario.Units.FindByPredicate([&UnitId](const FRTScenarioUnit& U) { return U.Id == UnitId; });
	}

	/** Costruisce la scena della spec; se non si costruisce lo dice come test, con il motivo del rifiuto. */
	bool BuildSceneWith(FAutomationTestBase& Test, const TCHAR* AbilityId, const FRTAbilityLabFixtureSpec& Spec,
		FRTTestScenario& OutScenario)
	{
		FString Errore;
		const bool bCostruita = URTAbilityLabLibrary::BuildFixture(FName(AbilityId), Spec, OutScenario, Errore);
		return Test.TestTrue(FString::Printf(TEXT("%s: la scena si costruisce (%s)"), AbilityId, *Errore), bCostruita);
	}

	/** La scena di default del banco con `TargetCount` bersagli. */
	bool BuildScene(FAutomationTestBase& Test, const TCHAR* AbilityId, int32 TargetCount, FRTTestScenario& OutScenario)
	{
		FRTAbilityLabFixtureSpec Spec;
		Spec.TargetCount = TargetCount;
		return BuildSceneWith(Test, AbilityId, Spec, OutScenario);
	}

	/** Nessuna coppia di unita' sulla stessa cella: la posa non ne impila due. */
	bool AllUnitsOnDistinctCells(const FRTTestScenario& Scenario)
	{
		for (int32 A = 0; A < Scenario.Units.Num(); ++A)
		{
			for (int32 B = A + 1; B < Scenario.Units.Num(); ++B)
			{
				if (Scenario.Units[A].Cell == Scenario.Units[B].Cell) { return false; }
			}
		}
		return true;
	}

	/** Quanti bersagli chiede una scena che porta solo questi tag. */
	int32 RequestedByTags(const TArray<FString>& Tags)
	{
		FRTTestScenario Scena;
		Scena.Tags = Tags;
		return URTAbilityLabLibrary::RequestedTargetsOf(Scena);
	}
}

/**
 * **Una linea si allunga** (`#4136`): `Hero.Gadget.LinearDischarge` mira un'unita' lungo una linea, e a 2 o 3 bersagli la linea
 * deve contenerne 2 o 3. Il `TARGET` si sposta in fuori sulla stessa direzione finche' la linea ha le celle che servono, e
 * gli altri stanno nelle celle che la linea attraversa, dal piu' vicino a chi lancia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabLineTargetsGrowAlongTheLineTest,
	"RefactorTactics.AbilityLab.LineTargetsGrowAlongTheLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabLineTargetsGrowAlongTheLineTest::RunTest(const FString&)
{
	using namespace RTAbilityLabTestsInternal;
	using namespace RTAbilityLabTargetsTestsInternal;

	const TCHAR* const Scarica = TEXT("Hero.Gadget.LinearDischarge");
	FRTActionDef Def;
	ERTAbilityShape Shape = ERTAbilityShape::Single;
	if (!TestTrue(TEXT("premessa: LinearDischarge e' una linea del kit"),
		FindKitAction(Scarica, Def, Shape) && Shape == ERTAbilityShape::Line))
	{
		return false;
	}

	// Le celle di ogni numero di bersagli, per Id: dal `TARGET` in poi. A 3 il `TARGET` e' in (3,0), e gli altri due nelle
	// celle di prima, dal piu' vicino a chi lancia.
	const TArray<FRTCellId> Attese[3] = {
		{ FRTCellId(2, 0) },
		{ FRTCellId(2, 0), FRTCellId(1, 0) },
		{ FRTCellId(3, 0), FRTCellId(1, 0), FRTCellId(2, 0) },
	};

	for (int32 N = 1; N <= 3; ++N)
	{
		FRTTestScenario Scenario;
		if (!BuildScene(*this, Scarica, N, Scenario)) { continue; }

		TestTrue(FString::Printf(TEXT("%d bersagli: il tag lab-targets-%d"), N, N),
			Scenario.Tags.Contains(FString::Printf(TEXT("lab-targets-%d"), N)));
		TestEqual(FString::Printf(TEXT("%d bersagli: chi lancia e i bersagli, nessun altro"), N), Scenario.Units.Num(), 1 + N);
		for (int32 I = 0; I < 3; ++I)
		{
			const FRTScenarioUnit* Unit = UnitById(Scenario, TargetIds[I]);
			if (I >= N)
			{
				TestNull(FString::Printf(TEXT("%d bersagli: nessun %s"), N, TargetIds[I]), Unit);
			}
			else if (TestNotNull(FString::Printf(TEXT("%d bersagli: c'e' %s"), N, TargetIds[I]), Unit))
			{
				TestTrue(FString::Printf(TEXT("%d bersagli: %s in %s (e' %s)"), N, TargetIds[I], *Attese[N - 1][I].ToString(),
					*Unit->Cell.ToString()), Unit->Cell == Attese[N - 1][I]);
			}
		}
		TestTrue(FString::Printf(TEXT("%d bersagli: nessuna coppia di unita' sulla stessa cella"), N), AllUnitsOnDistinctCells(Scenario));
		TestEqual(FString::Printf(TEXT("%d bersagli: se ne chiedono %d"), N, N), URTAbilityLabLibrary::RequestedTargetsOf(Scenario), N);
		TestEqual(FString::Printf(TEXT("%d bersagli: se ne posano %d"), N, N), URTAbilityLabLibrary::PlacedTargetsOf(Scenario), N);
	}

	// Verso Ovest (-X): l'ordine canonico di `HexHitCells` e' per X crescente, cioe' dalla cella piu' LONTANA da chi lancia, e i
	// bersagli in piu' si posano dalla piu' VICINA. Verso Est le due cose coincidono, e un ordinamento tolto non si vedrebbe: qui
	// senza l'ordinamento per distanza `TARGET2` finirebbe in (-2,0) invece che in (-1,0).
	{
		FRTAbilityLabFixtureSpec VersoOvest;
		VersoOvest.TargetCell = FRTCellId(-2, 0);
		VersoOvest.TargetCount = 3;
		const TArray<FRTCellId> Canoniche = URTHexCombatLibrary::HexHitCells(ERTAbilityShape::Line, VersoOvest.CasterCell,
			FRTCellId(-3, 0), Def.RangeCells, 0);
		TestTrue(TEXT("premessa: verso Ovest l'ordine canonico delle celle non e' dalla piu' vicina a chi lancia"),
			Canoniche.Num() == 3
			&& URTHexLibrary::HexDistance(VersoOvest.CasterCell, Canoniche[0]) > URTHexLibrary::HexDistance(VersoOvest.CasterCell, Canoniche[2]));

		const TArray<FRTCellId> AtteseOvest = { FRTCellId(-3, 0), FRTCellId(-1, 0), FRTCellId(-2, 0) };
		FRTTestScenario Ovest;
		if (BuildSceneWith(*this, Scarica, VersoOvest, Ovest))
		{
			TestEqual(TEXT("verso Ovest, 3 bersagli: chi lancia e i bersagli, nessun altro"), Ovest.Units.Num(), 1 + AtteseOvest.Num());
			for (int32 I = 0; I < AtteseOvest.Num(); ++I)
			{
				const FRTScenarioUnit* Unit = UnitById(Ovest, TargetIds[I]);
				if (TestNotNull(FString::Printf(TEXT("verso Ovest, 3 bersagli: c'e' %s"), TargetIds[I]), Unit))
				{
					TestTrue(FString::Printf(TEXT("verso Ovest, 3 bersagli: %s in %s (e' %s)"), TargetIds[I], *AtteseOvest[I].ToString(),
						*Unit->Cell.ToString()), Unit->Cell == AtteseOvest[I]);
				}
			}
			TestTrue(TEXT("verso Ovest, 3 bersagli: nessuna coppia di unita' sulla stessa cella"), AllUnitsOnDistinctCells(Ovest));
			TestEqual(TEXT("verso Ovest, 3 bersagli: se ne posano 3"), URTAbilityLabLibrary::PlacedTargetsOf(Ovest), 3);
		}
	}

	// A 3: tutti nella squadra del `TARGET`, eroi diversi fra loro e da chi lancia, e la run li colpisce tutti e tre.
	FRTTestScenario Tre;
	if (BuildScene(*this, Scarica, 3, Tre))
	{
		const FRTScenarioUnit* Caster = UnitById(Tre, URTAbilityLabLibrary::CasterUnitId());
		const FRTScenarioUnit* Bersaglio = UnitById(Tre, TargetIds[0]);
		if (TestNotNull(TEXT("3 bersagli: c'e' chi lancia"), Caster) && TestNotNull(TEXT("3 bersagli: c'e' il TARGET"), Bersaglio))
		{
			TestNotEqual(TEXT("3 bersagli: il TARGET e' nell'altra squadra"), Bersaglio->TeamId, Caster->TeamId);
			TSet<FName> Eroi;
			Eroi.Add(Caster->HeroId);
			for (const TCHAR* Id : TargetIds)
			{
				const FRTScenarioUnit* Unit = UnitById(Tre, Id);
				if (!TestNotNull(FString::Printf(TEXT("3 bersagli: c'e' %s"), Id), Unit)) { continue; }
				TestEqual(FString::Printf(TEXT("3 bersagli: %s e' nella squadra del TARGET"), Id), Unit->TeamId, Bersaglio->TeamId);
				TestFalse(FString::Printf(TEXT("3 bersagli: %s ha un eroe diverso dagli altri e da chi lancia (%s)"), Id,
					*Unit->HeroId.ToString()), Eroi.Contains(Unit->HeroId));
				Eroi.Add(Unit->HeroId);
			}
		}

		const FRTTestResult Result = RunFixture(*this, Tre);
		TestFalse(FString::Printf(TEXT("3 bersagli: nessun ERROR (%s)"), *Result.ErrorMessage), Result.Outcome == ERTTestOutcome::Error);
		TestEqual(TEXT("3 bersagli: un turno giocato"), Result.TurnsPlayed, 1);
		for (const TCHAR* Id : TargetIds)
		{
			int32 Prima = 0;
			int32 Dopo = 0;
			TestTrue(FString::Printf(TEXT("3 bersagli: la vita di %s scende"), Id),
				FieldBeforeAfter(Result, Id, TEXT("Health"), Prima, Dopo) && Dopo < Prima);
		}
	}
	return true;
}

/**
 * **Un'area li tiene dentro** (`#4136`): i bersagli in piu' di `Hero.Gadget.Overload` stanno nelle celle che l'area colpisce,
 * per la cella che l'intento mira, e la run li colpisce tutti. Con l'alleato (`Hero.Muiren.CircularTide`, che cura) gli altri
 * vanno nella squadra di chi lancia, nessuno sulla cella dell'`OPPONENT`, e l'`OPPONENT` resta nell'altra squadra.
 *
 * ⚠️ **Le celle colpite si chiedono a `HexHitCells`, qui come nel banco**: due elenchi diversi sarebbero il banco che
 * ricalcola la geometria del colpo, e un test che li confrontasse fra loro non direbbe quale dei due sbaglia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabAreaTargetsStayInsideTheAreaTest,
	"RefactorTactics.AbilityLab.AreaTargetsStayInsideTheArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabAreaTargetsStayInsideTheAreaTest::RunTest(const FString&)
{
	using namespace RTAbilityLabTestsInternal;
	using namespace RTAbilityLabTargetsTestsInternal;

	// Un'area che fa danno: i tre bersagli stanno nell'area, e la run li colpisce.
	{
		const TCHAR* const Sovraccarico = TEXT("Hero.Gadget.Overload");
		// Portata e raggio come li legge il resolver: dall'azione che l'eroe impugna, non da `FRTActionDef` che il raggio non lo porta.
		FRTAbilityLabEntry Voce;
		FRTTestScenario Scenario;
		if (TestTrue(TEXT("premessa: Overload e' un'area del kit"),
				URTAbilityLabLibrary::FindAbility(Sovraccarico, Voce) && Voce.Shape == ERTAbilityShape::Area)
			&& BuildScene(*this, Sovraccarico, 3, Scenario))
		{
			const FRTScenarioUnit* Caster = UnitById(Scenario, URTAbilityLabLibrary::CasterUnitId());
			const FRTScenarioIntent& Intent = Scenario.Turns[0].Intents[0];
			TestTrue(TEXT("Overload a 3 bersagli: l'intento mira una cella"), Intent.bTargetsCell);
			if (TestNotNull(TEXT("Overload a 3 bersagli: c'e' chi lancia"), Caster))
			{
				const TArray<FRTCellId> Colpite = URTHexCombatLibrary::HexHitCells(ERTAbilityShape::Area, Caster->Cell,
					Intent.TargetCell, Voce.RangeCells, Voce.AreaRadius);
				TestEqual(TEXT("Overload a 3 bersagli: se ne posano 3"), URTAbilityLabLibrary::PlacedTargetsOf(Scenario), 3);
				for (const TCHAR* Id : TargetIds)
				{
					const FRTScenarioUnit* Unit = UnitById(Scenario, Id);
					if (!TestNotNull(FString::Printf(TEXT("Overload a 3 bersagli: c'e' %s"), Id), Unit)) { continue; }
					TestTrue(FString::Printf(TEXT("Overload a 3 bersagli: %s e' nell'area, centrata in %s (e' in %s)"), Id,
						*Intent.TargetCell.ToString(), *Unit->Cell.ToString()), Colpite.Contains(Unit->Cell));
					TestTrue(FString::Printf(TEXT("Overload a 3 bersagli: %s e' nella mappa"), Id),
						URTHexLibrary::HexDistance(FRTCellId(0, 0), Unit->Cell) <= Scenario.MapRadius);
					TestTrue(FString::Printf(TEXT("Overload a 3 bersagli: %s non e' su chi lancia"), Id), Unit->Cell != Caster->Cell);
				}
				TestTrue(TEXT("Overload a 3 bersagli: nessuna coppia di unita' sulla stessa cella"), AllUnitsOnDistinctCells(Scenario));
			}

			const FRTTestResult Result = RunFixture(*this, Scenario);
			TestFalse(FString::Printf(TEXT("Overload a 3 bersagli: nessun ERROR (%s)"), *Result.ErrorMessage),
				Result.Outcome == ERTTestOutcome::Error);
			TestEqual(TEXT("Overload a 3 bersagli: un turno giocato"), Result.TurnsPlayed, 1);
			for (const TCHAR* Id : TargetIds)
			{
				int32 Prima = 0;
				int32 Dopo = 0;
				TestTrue(FString::Printf(TEXT("Overload a 3 bersagli: la vita di %s scende"), Id),
					FieldBeforeAfter(Result, Id, TEXT("Health"), Prima, Dopo) && Dopo < Prima);
			}
		}
	}

	// Una cura ad area, con l'alleato (Review Focus 4): i bersagli in piu' sono alleati di chi lancia, e l'`OPPONENT` --
	// la seconda squadra della scena, che ne impedisce la fine al primo Cleanup -- non perde ne' la squadra ne' la cella.
	{
		const TCHAR* const Marea = TEXT("Hero.Muiren.CircularTide");
		FRTAbilityLabEntry Voce;
		FRTTestScenario Scenario;
		if (TestTrue(TEXT("premessa: CircularTide e' un'area del kit"),
				URTAbilityLabLibrary::FindAbility(Marea, Voce) && Voce.Shape == ERTAbilityShape::Area)
			&& BuildScene(*this, Marea, 3, Scenario))
		{
			const FRTScenarioUnit* Caster = UnitById(Scenario, URTAbilityLabLibrary::CasterUnitId());
			const FRTScenarioUnit* Bersaglio = UnitById(Scenario, TargetIds[0]);
			const FRTScenarioUnit* Avversario = UnitById(Scenario, TEXT("OPPONENT"));
			const FRTScenarioIntent& Intent = Scenario.Turns[0].Intents[0];
			if (TestNotNull(TEXT("CircularTide a 3 bersagli: c'e' chi lancia"), Caster)
				&& TestNotNull(TEXT("CircularTide a 3 bersagli: c'e' il TARGET"), Bersaglio)
				&& TestNotNull(TEXT("CircularTide a 3 bersagli: c'e' l'OPPONENT"), Avversario))
			{
				TestEqual(TEXT("CircularTide a 3 bersagli: il TARGET e' nella squadra di chi lancia"), Bersaglio->TeamId, Caster->TeamId);
				TestNotEqual(TEXT("CircularTide a 3 bersagli: l'OPPONENT resta nell'altra squadra"), Avversario->TeamId, Caster->TeamId);
				TestEqual(TEXT("CircularTide a 3 bersagli: l'OPPONENT e' in squadra 1"), Avversario->TeamId, 1);

				const TArray<FRTCellId> Colpite = URTHexCombatLibrary::HexHitCells(ERTAbilityShape::Area, Caster->Cell,
					Intent.TargetCell, Voce.RangeCells, Voce.AreaRadius);
				int32 InPiu = 0;
				for (const TCHAR* Id : { TargetIds[1], TargetIds[2] })
				{
					const FRTScenarioUnit* Unit = UnitById(Scenario, Id);
					if (!Unit) { continue; }
					++InPiu;
					TestEqual(FString::Printf(TEXT("CircularTide a 3 bersagli: %s e' nella squadra di chi lancia"), Id), Unit->TeamId, Caster->TeamId);
					TestTrue(FString::Printf(TEXT("CircularTide a 3 bersagli: %s non e' sulla cella dell'OPPONENT"), Id), Unit->Cell != Avversario->Cell);
					TestTrue(FString::Printf(TEXT("CircularTide a 3 bersagli: %s e' nell'area (e' in %s)"), Id, *Unit->Cell.ToString()),
						Colpite.Contains(Unit->Cell));
					TestTrue(FString::Printf(TEXT("CircularTide a 3 bersagli: %s e' ferito, o la cura non si vedrebbe"), Id), Unit->Health > 0);
				}
				// Chi lancia, il `TARGET` e l'`OPPONENT` prendono ciascuno un eroe del roster, e i bersagli in piu' ne prendono altri: se gli
				// eroi finiscono, se ne posano quanti ce ne stanno. Che ce ne sia almeno uno libero e' la PREMESSA di questo caso, e si
				// asserisce invece di darla per buona: senza, `InPiu >= 1` direbbe un difetto del banco dove manca un eroe nel roster.
				int32 EroiLiberi = 0;
				for (const FName& HeroId : URTHeroCatalogLibrary::GetHeroIds())
				{
					EroiLiberi += (HeroId != Caster->HeroId && HeroId != Bersaglio->HeroId && HeroId != Avversario->HeroId) ? 1 : 0;
				}
				TestTrue(TEXT("premessa: il roster ha almeno un eroe oltre a chi lancia, al TARGET e all'OPPONENT"), EroiLiberi >= 1);
				TestTrue(FString::Printf(TEXT("CircularTide a 3 bersagli: almeno un bersaglio in piu' (%d)"), InPiu), InPiu >= 1);
				TestEqual(TEXT("CircularTide a 3 bersagli: i bersagli posati sono quelli contati"),
					URTAbilityLabLibrary::PlacedTargetsOf(Scenario), 1 + InPiu);
				TestTrue(TEXT("CircularTide a 3 bersagli: nessuna coppia di unita' sulla stessa cella"), AllUnitsOnDistinctCells(Scenario));
			}

			const FRTTestResult Result = RunFixture(*this, Scenario);
			TestFalse(FString::Printf(TEXT("CircularTide a 3 bersagli: nessun ERROR (%s)"), *Result.ErrorMessage),
				Result.Outcome == ERTTestOutcome::Error);
			TestEqual(TEXT("CircularTide a 3 bersagli: un turno giocato"), Result.TurnsPlayed, 1);
		}
	}
	return true;
}

/**
 * **Un'azione che colpisce una cella sola resta a un bersaglio** (`#4136`): un colpo `Single` non ha celle in piu' da
 * riempire, e uno scatto o una reazione non sono una scena a piu' bersagli. Se la linea non puo' allungarsi (la mappa finisce),
 * si posa quello che ci sta. In ogni caso il tag dice quanti se ne sono CHIESTI: i posati si contano dalle unita'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabSingleTargetActionsStayAtOneTest,
	"RefactorTactics.AbilityLab.SingleTargetActionsStayAtOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabSingleTargetActionsStayAtOneTest::RunTest(const FString&)
{
	using namespace RTAbilityLabTestsInternal;
	using namespace RTAbilityLabTargetsTestsInternal;

	auto StaAUno = [this](const TCHAR* AbilityId, const FRTAbilityLabFixtureSpec& Spec, const TCHAR* Come = TEXT(""))
	{
		FRTTestScenario Scenario;
		if (!BuildSceneWith(*this, AbilityId, Spec, Scenario)) { return; }
		TestEqual(FString::Printf(TEXT("%s a %d bersagli%s: se ne chiedono %d"), AbilityId, Spec.TargetCount, Come, FMath::Clamp(Spec.TargetCount, 1, 3)),
			URTAbilityLabLibrary::RequestedTargetsOf(Scenario), FMath::Clamp(Spec.TargetCount, 1, 3));
		TestEqual(FString::Printf(TEXT("%s a %d bersagli%s: se ne posa uno"), AbilityId, Spec.TargetCount, Come),
			URTAbilityLabLibrary::PlacedTargetsOf(Scenario), 1);
		TestNull(FString::Printf(TEXT("%s a %d bersagli%s: nessun TARGET2"), AbilityId, Spec.TargetCount, Come), UnitById(Scenario, TargetIds[1]));
		TestNull(FString::Printf(TEXT("%s a %d bersagli%s: nessun TARGET3"), AbilityId, Spec.TargetCount, Come), UnitById(Scenario, TargetIds[2]));
	};
	FRTAbilityLabFixtureSpec TreBersagli;
	TreBersagli.TargetCount = 3;
	// Un colpo `Single` forzato su una cella: la cella mirata e' vuota, e metterci un bersaglio toglierebbe al `TARGET` il posto
	// dell'unica cella colpita. Non se ne posa un secondo.
	FRTAbilityLabFixtureSpec AUnaCella = TreBersagli;
	AUnaCella.Target = ERTAbilityLabTarget::Cell;

	// La prima abilita' d'eroe `Single` che mira un'unita': non una reazione, non uno scatto, e senza provocazione.
	FName PrimaSingola = NAME_None;
	for (const FRTAbilityLabEntry& Entry : URTAbilityLabLibrary::ListCanonicalAbilities())
	{
		FRTActionDef Def;
		ERTAbilityShape Shape = ERTAbilityShape::Single;
		if (Entry.bIsCoreAction || Entry.Shape != ERTAbilityShape::Single || !FindKitAction(Entry.AbilityId, Def, Shape)) { continue; }
		if (Def.Slot == ERTActionSlot::Reaction || URTAbilityLabLibrary::ProvocationFor(Def) != ERTAbilityLabProvocation::None
			|| URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase) == ERTMatchPhase::Dash)
		{
			continue;
		}
		const ERTAbilityLabTarget Kind = URTAbilityLabLibrary::ChooseTarget(Def, Shape, FRTAbilityLabFixtureSpec()).Kind;
		if (Kind == ERTAbilityLabTarget::Enemy || Kind == ERTAbilityLabTarget::Ally)
		{
			PrimaSingola = Entry.AbilityId;
			break;
		}
	}
	if (TestFalse(TEXT("premessa: il roster ha un'abilita' `Single` che mira un'unita'"), PrimaSingola.IsNone()))
	{
		StaAUno(*PrimaSingola.ToString(), TreBersagli);
		StaAUno(*PrimaSingola.ToString(), AUnaCella, TEXT(" (forzato su una cella)"));
	}

	// Una reazione e uno scatto: non sono scene a piu' bersagli, anche se la loro forma ha piu' celle.
	StaAUno(TEXT("Hero.Ivrin.Deflection"), TreBersagli);
	StaAUno(TEXT("Hero.Ivrin.PassingBlade"), TreBersagli);

	// E tutte le altre dello stesso genere, dal catalogo e non per nome: ogni azione di forma `Single`, e ogni scatto, reazione,
	// azione con una provocazione o su chi la usa. Il banco non ha niente da dividere per loro, qualunque sia la forma.
	// 🔴 Un filtro che non ne riconoscesse nessuna assolverebbe il banco: `Controllate` lo dice.
	int32 Controllate = 0;
	for (const FRTAbilityLabEntry& Entry : URTAbilityLabLibrary::ListCanonicalAbilities())
	{
		FRTActionDef Def;
		ERTAbilityShape Shape = ERTAbilityShape::Single;
		if (!FindKitAction(Entry.AbilityId, Def, Shape) && !FindGenericAction(Entry.AbilityId, Def, Shape)) { continue; }
		const bool bUnSolo = Shape == ERTAbilityShape::Single || Def.Slot == ERTActionSlot::Reaction || Def.bSelfTarget
			|| URTAbilityLabLibrary::ProvocationFor(Def) != ERTAbilityLabProvocation::None
			|| URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase) == ERTMatchPhase::Dash;
		FRTTestScenario Scena;
		FString Errore;
		if (!bUnSolo || !URTAbilityLabLibrary::BuildFixture(Entry.AbilityId, TreBersagli, Scena, Errore)) { continue; }
		++Controllate;
		TestEqual(FString::Printf(TEXT("%s a 3 bersagli: se ne posa uno"), *Entry.AbilityId.ToString()),
			URTAbilityLabLibrary::PlacedTargetsOf(Scena), 1);
	}
	TestTrue(FString::Printf(TEXT("il banco riconosce azioni che restano a un bersaglio (%d)"), Controllate), Controllate > 0);

	// Review Focus 3, una linea che non puo' allungarsi: mappa di raggio 1, il `TARGET` in (1,0). La cella oltre e' fuori
	// dalla mappa, il `TARGET` resta dov'e', e se ne posa uno solo.
	FRTAbilityLabFixtureSpec Stretta;
	Stretta.MapRadius = 1;
	Stretta.TargetCell = FRTCellId(1, 0);
	Stretta.TargetCount = 3;
	FRTTestScenario Scenario;
	if (BuildSceneWith(*this, TEXT("Hero.Gadget.LinearDischarge"), Stretta, Scenario))
	{
		const FRTScenarioUnit* Bersaglio = UnitById(Scenario, TargetIds[0]);
		if (TestNotNull(TEXT("mappa di raggio 1: c'e' il TARGET"), Bersaglio))
		{
			TestTrue(FString::Printf(TEXT("mappa di raggio 1: il TARGET resta in (1,0) (e' in %s)"), *Bersaglio->Cell.ToString()),
				Bersaglio->Cell == FRTCellId(1, 0));
		}
		TestEqual(TEXT("mappa di raggio 1: se ne chiedono 3"), URTAbilityLabLibrary::RequestedTargetsOf(Scenario), 3);
		TestEqual(TEXT("mappa di raggio 1: se ne posa uno solo"), URTAbilityLabLibrary::PlacedTargetsOf(Scenario), 1);
		TestEqual(TEXT("mappa di raggio 1: chi lancia e il TARGET, nessun altro"), Scenario.Units.Num(), 2);
	}
	return true;
}

/**
 * **Il numero di bersagli viaggia nel tag della scena** (`#4136`): la scena e' un file, e chi la rilegge (il passo del banco, il
 * tasto che cambia il numero) non ha la spec che l'ha prodotta. Il tag e' scritto sempre, ed e' deterministico; un tag illeggibile
 * o fuori scala si legge come un numero valido (Review Focus 2).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabTargetCountTravelsInTheSceneTagTest,
	"RefactorTactics.AbilityLab.TargetCountTravelsInTheSceneTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabTargetCountTravelsInTheSceneTagTest::RunTest(const FString&)
{
	using namespace RTAbilityLabTargetsTestsInternal;

	const TCHAR* const Scarica = TEXT("Hero.Gadget.LinearDischarge");
	TestEqual(TEXT("il banco posa al piu' 3 bersagli"), URTAbilityLabLibrary::MaxLabTargets(), 3);
	TestEqual(TEXT("il default della spec e' un bersaglio"), FRTAbilityLabFixtureSpec().TargetCount, 1);

	// Due costruzioni della stessa spec danno lo stesso testo: nessun caso, nessun ordine di iterazione.
	FRTAbilityLabFixtureSpec Spec;
	Spec.TargetCount = 3;
	FRTTestScenario Prima;
	FRTTestScenario Seconda;
	if (BuildSceneWith(*this, Scarica, Spec, Prima) && BuildSceneWith(*this, Scarica, Spec, Seconda))
	{
		FString TestoPrima;
		FString TestoSeconda;
		FString Errore;
		const bool bPrima = URTScenarioLoader::SaveToString(Prima, TestoPrima, Errore);
		TestTrue(FString::Printf(TEXT("la scena a 3 bersagli si scrive (%s)"), *Errore), bPrima);
		Errore.Reset();
		const bool bSeconda = URTScenarioLoader::SaveToString(Seconda, TestoSeconda, Errore);
		TestTrue(FString::Printf(TEXT("e anche la seconda (%s)"), *Errore), bSeconda);
		TestEqual(TEXT("due costruzioni della stessa spec danno lo stesso testo"), TestoPrima, TestoSeconda);

		// E il numero sopravvive al giro: e' scritto nel file, non nella spec.
		FRTTestScenario Riletta;
		Errore.Reset();
		const bool bRiletta = bPrima && URTScenarioLoader::LoadFromString(TestoPrima, Riletta, Errore);
		TestTrue(FString::Printf(TEXT("e si rilegge (%s)"), *Errore), bRiletta);
		if (bRiletta)
		{
			TestEqual(TEXT("riletta: se ne chiedono ancora 3"), URTAbilityLabLibrary::RequestedTargetsOf(Riletta), 3);
			TestEqual(TEXT("riletta: se ne posano ancora 3"), URTAbilityLabLibrary::PlacedTargetsOf(Riletta), 3);
		}
	}

	// Un tag illeggibile o fuori scala (Review Focus 2): portato fra 1 e 3, o 1 se non e' un numero. Senza tag, 1: e' una
	// scena scritta prima di questo lavoro.
	TestEqual(TEXT("lab-targets-7 si legge 3"), RequestedByTags({ TEXT("lab-targets-7") }), 3);
	TestEqual(TEXT("lab-targets-0 si legge 1"), RequestedByTags({ TEXT("lab-targets-0") }), 1);
	TestEqual(TEXT("lab-targets-x si legge 1"), RequestedByTags({ TEXT("lab-targets-x") }), 1);
	TestEqual(TEXT("lab-targets- (senza numero) si legge 1"), RequestedByTags({ TEXT("lab-targets-") }), 1);
	TestEqual(TEXT("lab-targets--2 (negativo) si legge 1"), RequestedByTags({ TEXT("lab-targets--2") }), 1);
	TestEqual(TEXT("lab-targets-99999999999999 (enorme) si legge 3"), RequestedByTags({ TEXT("lab-targets-99999999999999") }), 3);
	TestEqual(TEXT("senza tag si legge 1"), RequestedByTags({}), 1);
	TestEqual(TEXT("solo ability-lab si legge 1"), RequestedByTags({ TEXT("ability-lab") }), 1);
	TestEqual(TEXT("lab-targets-2 fra gli altri tag si legge 2"), RequestedByTags({ TEXT("ability-lab"), TEXT("lab-targets-2") }), 2);

	// Un numero fuori scala nella SPEC si scrive portato fra 1 e 3.
	for (const TPair<int32, int32>& Coppia : { TPair<int32, int32>(9, 3), TPair<int32, int32>(0, 1), TPair<int32, int32>(-4, 1),
		TPair<int32, int32>(2, 2) })
	{
		FRTAbilityLabFixtureSpec Fuori;
		Fuori.TargetCount = Coppia.Key;
		FRTTestScenario Scena;
		if (BuildSceneWith(*this, Scarica, Fuori, Scena))
		{
			TestTrue(FString::Printf(TEXT("TargetCount %d si scrive lab-targets-%d"), Coppia.Key, Coppia.Value),
				Scena.Tags.Contains(FString::Printf(TEXT("lab-targets-%d"), Coppia.Value)));
			TestEqual(FString::Printf(TEXT("TargetCount %d: se ne chiedono %d"), Coppia.Key, Coppia.Value),
				URTAbilityLabLibrary::RequestedTargetsOf(Scena), Coppia.Value);
		}
	}
	return true;
}

// =====================================================================================================
// `#4136` — il tasto `U` e la riga della banda. Il tasto fa girare il numero di bersagli della scena in corso (1 → 2 → 3 → 1) e
// la scena riparte; la riga dice quanti se ne chiedono e, se la mappa non li regge, quanti ne stanno.
//
// 🔑 Le scene che questi test leggono stanno nella radice di PROVA del Lab: mai in `Saved/RTLab`, che e' dell'Editor.
// =====================================================================================================

namespace RTAbilityLabTargetKeyTestsInternal
{
	/** L'azione la cui linea si allunga: l'unica con cui un bersaglio in piu' ha un posto da cercare. */
	const TCHAR* const Scarica = TEXT("Hero.Gadget.LinearDischarge");

	FString SceneIdDi(const TCHAR* AbilityId)
	{
		return FString::Printf(TEXT("AbilityLab.%s"), AbilityId);
	}

	/** Costruisce la scena della spec e la scrive nella radice del Lab; se non si scrive lo dice come test, col motivo. */
	bool BuildAndWrite(FAutomationTestBase& Test, const FRTAbilityLabFixtureSpec& Spec, FRTTestScenario& OutScenario)
	{
		// Il booleano prima del messaggio: l'ordine di valutazione degli argomenti non e' definito, e `Errore` si legge dopo.
		FString Errore;
		const bool bCostruita = URTAbilityLabLibrary::BuildFixture(FName(Scarica), Spec, OutScenario, Errore);
		if (!Test.TestTrue(FString::Printf(TEXT("la scena si costruisce (%s)"), *Errore), bCostruita))
		{
			return false;
		}
		const bool bScritta = URTAbilityLabLibrary::WriteLabScenario(OutScenario, Errore);
		return Test.TestTrue(FString::Printf(TEXT("la scena si scrive (%s)"), *Errore), bScritta);
	}

	/** Toglie dalla scena i tag `lab-targets-N`: com'era una scena scritta prima di questo lavoro (Review Focus 1). */
	void StripTargetTags(FRTTestScenario& Scenario)
	{
		Scenario.Tags.RemoveAll([](const FString& Tag) { return Tag.StartsWith(TEXT("lab-targets-")); });
	}

	/**
	 * L'azione a `Step` posizioni da `AbilityId` nel suo kit, come la propone il passo del banco: le voci di `ListHeroKit` che
	 * `BuildFixture` costruisce, nell'ordine del kit. La calcola il test con il catalogo, non la chiede alla funzione che prova.
	 * `NAME_None` se l'azione non e' di un eroe o non e' nel ciclo.
	 */
	FName KitNeighbour(const FName& AbilityId, int32 Step, int32& OutPosition, int32& OutCount)
	{
		FRTAbilityLabEntry Voce;
		if (!URTAbilityLabLibrary::FindAbility(AbilityId, Voce) || Voce.OwnerHeroId.IsNone()) { return NAME_None; }

		TArray<FName> Ciclo;
		for (const FRTAbilityLabEntry& Entry : URTHeroLabLibrary::ListHeroKit(Voce.OwnerHeroId))
		{
			FRTTestScenario Scartata;
			FString Errore;
			if (URTAbilityLabLibrary::BuildFixture(Entry.AbilityId, FRTAbilityLabFixtureSpec(), Scartata, Errore))
			{
				Ciclo.Add(Entry.AbilityId);
			}
		}
		const int32 Qui = Ciclo.IndexOfByKey(AbilityId);
		if (Qui == INDEX_NONE) { return NAME_None; }

		const int32 N = Ciclo.Num();
		const int32 Li = ((Qui + Step) % N + N) % N;
		OutPosition = Li + 1;
		OutCount = N;
		return Ciclo[Li];
	}
}

/**
 * **`U` gira fra 1, 2 e 3, e fuori dal banco non fa niente** (`#4136`). La parte che decide vive nella libreria: quale numero viene
 * dopo, e per quale azione. Il GameMode scrive la scena e ricarica il livello, e questo si giudica in PIE.
 *
 * 🔴 Review Focus 1: una scena scritta prima di questo lavoro non porta il tag, si legge come 1, e `U` la porta a 2. Review Focus 5:
 * fuori dal banco `PlanLabTargets` risponde `false` senza toccare le uscite, e quello e' cio' che tiene inerte il tasto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabTargetKeyCyclesAndIsInertOutsideTest,
	"RefactorTactics.AbilityLab.TargetKeyCyclesAndIsInertOutside",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabTargetKeyCyclesAndIsInertOutsideTest::RunTest(const FString&)
{
	using namespace RTAbilityLabTargetKeyTestsInternal;

	// Il giro: 1 → 2 → 3 → 1, e un valore fuori scala riparte da 1.
	TestEqual(TEXT("da 1 si va a 2"), URTAbilityLabLibrary::NextLabTargetCount(1), 2);
	TestEqual(TEXT("da 2 si va a 3"), URTAbilityLabLibrary::NextLabTargetCount(2), 3);
	TestEqual(TEXT("da 3 si torna a 1"), URTAbilityLabLibrary::NextLabTargetCount(3), 1);
	TestEqual(TEXT("0 e' fuori scala: va a 1"), URTAbilityLabLibrary::NextLabTargetCount(0), 1);
	TestEqual(TEXT("7 e' fuori scala: va a 1"), URTAbilityLabLibrary::NextLabTargetCount(7), 1);
	TestEqual(TEXT("-2 e' fuori scala: va a 1"), URTAbilityLabLibrary::NextLabTargetCount(-2), 1);

	// Fuori dal banco (Review Focus 5): niente da pianificare, e le uscite restano com'erano. Il segnaposto e' un valore che la
	// funzione non scriverebbe mai per caso.
	for (const TCHAR* Id : { TEXT("Movement.Basic"), TEXT("AbilityLab."), TEXT(""), TEXT("abilitylab.Hero.Gadget.LinearDischarge"),
		TEXT("AbilityLab.Azione.Inesistente") })
	{
		FName Azione(TEXT("Segnaposto"));
		int32 Prossimo = -7;
		TestFalse(FString::Printf(TEXT("'%s': nessun cambio di bersagli"), Id),
			URTAbilityLabLibrary::PlanLabTargets(Id, Azione, Prossimo));
		TestEqual(FString::Printf(TEXT("'%s': l'azione non e' toccata"), Id), Azione, FName(TEXT("Segnaposto")));
		TestEqual(FString::Printf(TEXT("'%s': il numero non e' toccato"), Id), Prossimo, -7);
	}

	// Sotto automation senza override la radice del Lab e' vuota: il percorso e' vuoto e la scena non si legge, senza combinare una
	// radice vuota con un nome relativo.
	const FString IdScarica = SceneIdDi(Scarica);
	TestTrue(TEXT("senza radice del Lab il percorso della scena e' vuoto"),
		URTAbilityLabLibrary::LabScenarioPath(IdScarica).IsEmpty());
	FRTTestScenario Letta;
	TestFalse(TEXT("senza radice del Lab la scena non si legge"), URTAbilityLabLibrary::ReadLabScene(IdScarica, Letta));
	{
		FName Azione;
		int32 Prossimo = 0;
		TestTrue(TEXT("senza radice del Lab `U` pianifica comunque: la scena vale 1 bersaglio"),
			URTAbilityLabLibrary::PlanLabTargets(IdScarica, Azione, Prossimo));
		TestEqual(TEXT("senza radice del Lab da 1 si va a 2"), Prossimo, 2);
		TestEqual(TEXT("e l'azione e' quella della scena"), Azione, FName(Scarica));
	}

	// Con una radice di prova: la scena scritta a 3 bersagli porta `U` a 1, e senza il tag a 2.
	const FString Root = RTScenarioTestSupport::ApriRadiceDiProva(TEXT("RTLabTargets"), TEXT("Giro"));
	ON_SCOPE_EXIT{ RTScenarioTestSupport::ChiudiRadiceDiProva(Root); };

	TestFalse(TEXT("nella radice di prova, senza file, la scena non si legge"),
		URTAbilityLabLibrary::ReadLabScene(IdScarica, Letta));
	{
		FName Azione;
		int32 Prossimo = 0;
		TestTrue(TEXT("senza file `U` pianifica: la scena vale 1 bersaglio"),
			URTAbilityLabLibrary::PlanLabTargets(IdScarica, Azione, Prossimo));
		TestEqual(TEXT("senza file da 1 si va a 2"), Prossimo, 2);
	}

	// Per ogni numero chiesto, `U` va al successivo: il controllo positivo che il tag e' davvero letto dal file.
	for (const TPair<int32, int32>& Coppia : { TPair<int32, int32>(1, 2), TPair<int32, int32>(2, 3), TPair<int32, int32>(3, 1) })
	{
		FRTAbilityLabFixtureSpec Spec;
		Spec.TargetCount = Coppia.Key;
		FRTTestScenario Scena;
		if (!BuildAndWrite(*this, Spec, Scena)) { continue; }

		FName Azione;
		int32 Prossimo = 0;
		if (TestTrue(FString::Printf(TEXT("a %d bersagli `U` pianifica"), Coppia.Key),
			URTAbilityLabLibrary::PlanLabTargets(IdScarica, Azione, Prossimo)))
		{
			TestEqual(FString::Printf(TEXT("a %d bersagli `U` va a %d"), Coppia.Key, Coppia.Value), Prossimo, Coppia.Value);
			TestEqual(FString::Printf(TEXT("a %d bersagli l'azione e' LinearDischarge"), Coppia.Key), Azione, FName(Scarica));
		}
	}

	// Review Focus 1: la stessa scena a 3 bersagli, salvata senza il tag, si legge come 1 e `U` la porta a 2 (non a 1).
	FRTAbilityLabFixtureSpec TreBersagli;
	TreBersagli.TargetCount = 3;
	FRTTestScenario Vecchia;
	if (BuildAndWrite(*this, TreBersagli, Vecchia))
	{
		TestEqual(TEXT("premessa: la scena nuova chiede 3 bersagli"), URTAbilityLabLibrary::RequestedTargetsOf(Vecchia), 3);
		StripTargetTags(Vecchia);
		TestEqual(TEXT("senza tag la scena chiede 1 bersaglio"), URTAbilityLabLibrary::RequestedTargetsOf(Vecchia), 1);
		FString Errore;
		const bool bRiscritta = URTAbilityLabLibrary::WriteLabScenario(Vecchia, Errore);
		if (TestTrue(FString::Printf(TEXT("la scena senza tag si riscrive (%s)"), *Errore), bRiscritta))
		{
			FName Azione;
			int32 Prossimo = 0;
			TestTrue(TEXT("senza tag `U` pianifica"), URTAbilityLabLibrary::PlanLabTargets(IdScarica, Azione, Prossimo));
			TestEqual(TEXT("senza il tag `U` porta la scena a 2"), Prossimo, 2);
			TestEqual(TEXT("e l'azione e' LinearDischarge"), Azione, FName(Scarica));
		}
	}
	return true;
}

/**
 * **La banda dice quanti bersagli la scena chiede, e quanti ne stanno** (`#4136`). `DescribeLabTargets` e' la frase, e
 * `DescribeLabStep` la porta in fondo alla riga, leggendo la scena dal file; senza file la scena vale 1.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabStepHintNamesTheTargetCountTest,
	"RefactorTactics.AbilityLab.StepHintNamesTheTargetCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabStepHintNamesTheTargetCountTest::RunTest(const FString&)
{
	using namespace RTAbilityLabTargetKeyTestsInternal;

	TestEqual(TEXT("2 chiesti, 2 posati"), URTAbilityLabLibrary::DescribeLabTargets(2, 2), FString(TEXT("«U» bersagli 2")));
	TestEqual(TEXT("1 chiesto, 1 posato"), URTAbilityLabLibrary::DescribeLabTargets(1, 1), FString(TEXT("«U» bersagli 1")));
	TestEqual(TEXT("3 chiesti, 1 posato"), URTAbilityLabLibrary::DescribeLabTargets(3, 1),
		FString(TEXT("«U» bersagli 3 (qui ne stanno 1)")));
	TestEqual(TEXT("3 chiesti, 2 posati"), URTAbilityLabLibrary::DescribeLabTargets(3, 2),
		FString(TEXT("«U» bersagli 3 (qui ne stanno 2)")));

	const FString IdScarica = SceneIdDi(Scarica);
	const FString RootDiProva = RTScenarioTestSupport::ApriRadiceDiProva(TEXT("RTLabTargets"), TEXT("Banda"));
	ON_SCOPE_EXIT{ RTScenarioTestSupport::ChiudiRadiceDiProva(RootDiProva); };

	// I tasti di prima restano nella riga: la banda li dice ancora, e quella dei bersagli e' un pezzo in piu'.
	FRTAbilityLabFixtureSpec TreBersagli;
	TreBersagli.TargetCount = 3;
	FRTTestScenario Larga;
	if (BuildAndWrite(*this, TreBersagli, Larga))
	{
		TestEqual(TEXT("premessa: sulla mappa di default ne stanno 3"), URTAbilityLabLibrary::PlacedTargetsOf(Larga), 3);
		const FString Riga = URTAbilityLabLibrary::DescribeLabStep(IdScarica);
		TestTrue(FString::Printf(TEXT("una scena a 3 bersagli lo dice (%s)"), *Riga), Riga.Contains(TEXT("«U» bersagli 3")));
		TestFalse(TEXT("e se ne stanno 3, non dice altro"), Riga.Contains(TEXT("qui ne stanno")));
		TestTrue(TEXT("i due tasti di prima restano nella riga"), Riga.Contains(TEXT("«,»")) && Riga.Contains(TEXT("«.»")));
	}

	// Una mappa che non regge i bersagli voluti: la linea non puo' allungarsi, se ne posa uno solo e la riga lo dice.
	FRTAbilityLabFixtureSpec Stretta;
	Stretta.MapRadius = 1;
	Stretta.TargetCell = FRTCellId(1, 0);
	Stretta.TargetCount = 3;
	FRTTestScenario Corta;
	if (BuildAndWrite(*this, Stretta, Corta))
	{
		TestEqual(TEXT("premessa: sulla mappa di raggio 1 ne sta uno"), URTAbilityLabLibrary::PlacedTargetsOf(Corta), 1);
		const FString Riga = URTAbilityLabLibrary::DescribeLabStep(IdScarica);
		TestTrue(FString::Printf(TEXT("una mappa stretta dice quanti ne stanno (%s)"), *Riga),
			Riga.Contains(TEXT("(qui ne stanno 1)")));
		TestTrue(TEXT("e quanti se ne chiedono"), Riga.Contains(TEXT("«U» bersagli 3")));
	}

	// Senza file la scena vale 1, e la riga non e' vuota: e' ancora una scena del banco.
	IFileManager::Get().Delete(*URTAbilityLabLibrary::LabScenarioPath(IdScarica));
	const FString SenzaFile = URTAbilityLabLibrary::DescribeLabStep(IdScarica);
	TestTrue(FString::Printf(TEXT("senza file la riga dice 1 bersaglio (%s)"), *SenzaFile),
		SenzaFile.Contains(TEXT("«U» bersagli 1")));
	TestFalse(TEXT("e non dice quanti ne stanno"), SenzaFile.Contains(TEXT("qui ne stanno")));

	// Fuori dal banco la riga resta vuota: il pezzo dei bersagli non la rende non vuota.
	TestTrue(TEXT("uno scenario che non e' del banco non ha la riga"),
		URTAbilityLabLibrary::DescribeLabStep(TEXT("Movement.Basic")).IsEmpty());
	return true;
}

/**
 * **Il passo del banco porta con se' il numero di bersagli** (`#4136`): `,` e `.` passano alla scena successiva i bersagli della
 * scena in corso. La scena la costruisce `BuildLabStepScene`, senza ricaricare il livello, e quindi si prova qui: se il numero non
 * passasse, chi ha scelto 3 bersagli con `U` e cambia azione ne ritroverebbe 1.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabStepSceneCarriesTheTargetCountTest,
	"RefactorTactics.AbilityLab.StepSceneCarriesTheTargetCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabStepSceneCarriesTheTargetCountTest::RunTest(const FString&)
{
	using namespace RTAbilityLabTargetKeyTestsInternal;

	const FString IdScarica = SceneIdDi(Scarica);
	const FString Root = RTScenarioTestSupport::ApriRadiceDiProva(TEXT("RTLabTargets"), TEXT("PassoScena"));
	ON_SCOPE_EXIT{ RTScenarioTestSupport::ChiudiRadiceDiProva(Root); };

	// Fuori dal banco: `false`, errore VUOTO (la sentinella prova che e' la funzione a svuotarlo), uscite intatte.
	for (const TCHAR* Id : { TEXT("Movement.Basic"), TEXT("AbilityLab."), TEXT("") })
	{
		FRTTestScenario Scena;
		Scena.ScenarioId = TEXT("Segnaposto");
		int32 Posizione = -7;
		int32 Totale = -7;
		FString Errore = TEXT("sentinella");
		TestFalse(FString::Printf(TEXT("'%s': nessun passo"), Id),
			URTAbilityLabLibrary::BuildLabStepScene(Id, +1, Scena, Posizione, Totale, Errore));
		TestTrue(FString::Printf(TEXT("'%s': nessun errore (%s)"), Id, *Errore), Errore.IsEmpty());
		TestEqual(FString::Printf(TEXT("'%s': la scena non e' toccata"), Id), Scena.ScenarioId, FString(TEXT("Segnaposto")));
		TestEqual(FString::Printf(TEXT("'%s': la posizione non e' toccata"), Id), Posizione, -7);
		TestEqual(FString::Printf(TEXT("'%s': il totale non e' toccato"), Id), Totale, -7);
	}

	// L'azione dopo e prima, calcolate dal catalogo. Servono almeno due voci perche' il passo cambi azione.
	int32 PosAvanti = 0;
	int32 TotAvanti = 0;
	int32 PosIndietro = 0;
	int32 TotIndietro = 0;
	const FName Avanti = KitNeighbour(FName(Scarica), +1, PosAvanti, TotAvanti);
	const FName Indietro = KitNeighbour(FName(Scarica), -1, PosIndietro, TotIndietro);
	if (!TestTrue(TEXT("premessa: nel kit di LinearDischarge c'e' un'altra azione, dopo e prima"),
		!Avanti.IsNone() && !Indietro.IsNone() && Avanti != FName(Scarica) && Indietro != FName(Scarica)))
	{
		return false;
	}

	// La scena in corso chiede 3 bersagli: la successiva, avanti e indietro, ne chiede 3.
	FRTAbilityLabFixtureSpec TreBersagli;
	TreBersagli.TargetCount = 3;
	FRTTestScenario Corrente;
	if (BuildAndWrite(*this, TreBersagli, Corrente))
	{
		for (const TPair<int32, FName>& Passo : { TPair<int32, FName>(+1, Avanti), TPair<int32, FName>(-1, Indietro) })
		{
			FRTTestScenario Scena;
			int32 Posizione = 0;
			int32 Totale = 0;
			FString Errore;
			const bool bCostruita = URTAbilityLabLibrary::BuildLabStepScene(IdScarica, Passo.Key, Scena, Posizione, Totale, Errore);
			if (!TestTrue(FString::Printf(TEXT("passo %+d: la scena si costruisce (%s)"), Passo.Key, *Errore), bCostruita))
			{
				continue;
			}
			TestEqual(FString::Printf(TEXT("passo %+d: e' la scena dell'azione successiva"), Passo.Key),
				Scena.ScenarioId, SceneIdDi(*Passo.Value.ToString()));
			TestEqual(FString::Printf(TEXT("passo %+d: l'azione, letta dalla scena"), Passo.Key),
				URTAbilityLabLibrary::LabAbilityIdOf(Scena.ScenarioId), Passo.Value);
			TestEqual(FString::Printf(TEXT("🔴 passo %+d: i bersagli della scena in corso passano alla nuova"), Passo.Key),
				URTAbilityLabLibrary::RequestedTargetsOf(Scena), 3);
			TestEqual(FString::Printf(TEXT("passo %+d: la posizione, da 1"), Passo.Key),
				Posizione, Passo.Key > 0 ? PosAvanti : PosIndietro);
			TestEqual(FString::Printf(TEXT("passo %+d: quante sono"), Passo.Key),
				Totale, Passo.Key > 0 ? TotAvanti : TotIndietro);
		}
	}

	// Controllo positivo, con un altro numero: a 2 bersagli la nuova ne chiede 2. Un passo che li portasse sempre a 3 (o a 1)
	// sarebbe verde sul solo caso di sopra.
	FRTAbilityLabFixtureSpec DueBersagli;
	DueBersagli.TargetCount = 2;
	FRTTestScenario Due;
	if (BuildAndWrite(*this, DueBersagli, Due))
	{
		FRTTestScenario Scena;
		int32 Posizione = 0;
		int32 Totale = 0;
		FString Errore;
		const bool bCostruita = URTAbilityLabLibrary::BuildLabStepScene(IdScarica, +1, Scena, Posizione, Totale, Errore);
		if (TestTrue(FString::Printf(TEXT("da 2 bersagli la scena si costruisce (%s)"), *Errore), bCostruita))
		{
			TestEqual(TEXT("da 2 bersagli la nuova ne chiede 2"), URTAbilityLabLibrary::RequestedTargetsOf(Scena), 2);
		}
	}

	// Senza file la scena in corso vale 1, e la nuova ne chiede 1: e' il default, non un errore.
	IFileManager::Get().Delete(*URTAbilityLabLibrary::LabScenarioPath(IdScarica));
	{
		FRTTestScenario Scena;
		int32 Posizione = 0;
		int32 Totale = 0;
		FString Errore;
		const bool bCostruita = URTAbilityLabLibrary::BuildLabStepScene(IdScarica, +1, Scena, Posizione, Totale, Errore);
		if (TestTrue(FString::Printf(TEXT("senza file la scena si costruisce (%s)"), *Errore), bCostruita))
		{
			TestEqual(TEXT("senza file la nuova ne chiede 1"), URTAbilityLabLibrary::RequestedTargetsOf(Scena), 1);
		}
	}
	TestTrue(TEXT("fuori dal banco non c'e' un'azione da leggere"),
		URTAbilityLabLibrary::LabAbilityIdOf(TEXT("Movement.Basic")).IsNone());
	return true;
}

/**
 * **`U` costruisce la stessa azione col numero di bersagli successivo** (`#4136`). La scena la costruisce `BuildLabTargetsScene`,
 * senza ricaricare il livello, e quindi si prova qui: se il numero non venisse portato avanti, `U` ricostruirebbe sempre a 1.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityLabTargetsSceneCyclesTheTargetCountTest,
	"RefactorTactics.AbilityLab.TargetsSceneCyclesTheTargetCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityLabTargetsSceneCyclesTheTargetCountTest::RunTest(const FString&)
{
	using namespace RTAbilityLabTargetKeyTestsInternal;

	const FString IdScarica = SceneIdDi(Scarica);
	const FString Root = RTScenarioTestSupport::ApriRadiceDiProva(TEXT("RTLabTargets"), TEXT("TastoScena"));
	ON_SCOPE_EXIT{ RTScenarioTestSupport::ChiudiRadiceDiProva(Root); };

	// Fuori dal banco: `false`, errore VUOTO (la sentinella prova che e' la funzione a svuotarlo), scena intatta.
	for (const TCHAR* Id : { TEXT("Movement.Basic"), TEXT("AbilityLab."), TEXT(""), TEXT("AbilityLab.Azione.Inesistente") })
	{
		FRTTestScenario Scena;
		Scena.ScenarioId = TEXT("Segnaposto");
		FString Errore = TEXT("sentinella");
		TestFalse(FString::Printf(TEXT("'%s': nessun cambio di bersagli"), Id),
			URTAbilityLabLibrary::BuildLabTargetsScene(Id, Scena, Errore));
		TestTrue(FString::Printf(TEXT("'%s': nessun errore (%s)"), Id, *Errore), Errore.IsEmpty());
		TestEqual(FString::Printf(TEXT("'%s': la scena non e' toccata"), Id), Scena.ScenarioId, FString(TEXT("Segnaposto")));
	}

	// Da N bersagli a N+1, e da 3 a 1. La scena nuova e' la stessa azione, e i bersagli posati sono quelli che la mappa regge.
	for (const TPair<int32, int32>& Coppia : { TPair<int32, int32>(3, 1), TPair<int32, int32>(1, 2), TPair<int32, int32>(2, 3) })
	{
		FRTAbilityLabFixtureSpec Spec;
		Spec.TargetCount = Coppia.Key;
		FRTTestScenario Corrente;
		if (!BuildAndWrite(*this, Spec, Corrente)) { continue; }

		FRTTestScenario Scena;
		FString Errore;
		const bool bCostruita = URTAbilityLabLibrary::BuildLabTargetsScene(IdScarica, Scena, Errore);
		if (!TestTrue(FString::Printf(TEXT("da %d bersagli la scena si costruisce (%s)"), Coppia.Key, *Errore), bCostruita))
		{
			continue;
		}
		TestEqual(FString::Printf(TEXT("da %d bersagli: e' la stessa azione"), Coppia.Key), Scena.ScenarioId, IdScarica);
		TestEqual(FString::Printf(TEXT("🔴 da %d bersagli la nuova ne chiede %d"), Coppia.Key, Coppia.Value),
			URTAbilityLabLibrary::RequestedTargetsOf(Scena), Coppia.Value);
		// Sulla mappa di default la linea regge fino a 3 bersagli: quelli chiesti sono quelli posati.
		TestEqual(FString::Printf(TEXT("da %d bersagli la nuova ne posa %d"), Coppia.Key, Coppia.Value),
			URTAbilityLabLibrary::PlacedTargetsOf(Scena), Coppia.Value);
	}

	// Una scena scritta prima del tag chiede 1, e `U` la porta a 2 (Review Focus 1).
	FRTAbilityLabFixtureSpec TreBersagli;
	TreBersagli.TargetCount = 3;
	FRTTestScenario Vecchia;
	if (BuildAndWrite(*this, TreBersagli, Vecchia))
	{
		StripTargetTags(Vecchia);
		FString ErroreScrittura;
		const bool bRiscritta = URTAbilityLabLibrary::WriteLabScenario(Vecchia, ErroreScrittura);
		if (TestTrue(FString::Printf(TEXT("la scena senza tag si riscrive (%s)"), *ErroreScrittura), bRiscritta))
		{
			FRTTestScenario Scena;
			FString Errore;
			const bool bCostruita = URTAbilityLabLibrary::BuildLabTargetsScene(IdScarica, Scena, Errore);
			if (TestTrue(FString::Printf(TEXT("senza tag la scena si costruisce (%s)"), *Errore), bCostruita))
			{
				TestEqual(TEXT("senza il tag la nuova ne chiede 2"), URTAbilityLabLibrary::RequestedTargetsOf(Scena), 2);
			}
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
