// Ability Lab (#2599) — i sei test che la issue dichiara.
//
// ## Perche' nessun `AbilityId` e' cablato
//
// Le identita' di questo progetto si rinominano: `D-130` ha spostato `Flux` -> `Aevik` e `Riva` -> `Phase`,
// `D-334` ha spostato `Hero.Riktor` -> `Hero.Branth`. Un test che scrivesse `Hero.Aevik.LinearDischarge`
// diventerebbe rosso al prossimo rename **senza che nulla si sia rotto**, e il costo di quel falso rosso lo
// paga chi rinomina.
//
// Questi test chiedono invece al catalogo *«dammi un'ability di forma `Line`»* — che e' la proprieta' che
// stanno verificando davvero. Se un giorno il roster non ne avesse piu' nessuna, il test lo dice: e' un
// fatto sul catalogo, non un refuso di manutenzione.

#include "Misc/AutomationTest.h"
#include "Ability/RTAbilityLab.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestResult.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

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

#endif // WITH_DEV_AUTOMATION_TESTS
