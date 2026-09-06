// Ability Lab 0.1 (#2599) — i sei test che la issue dichiara.
//
// ## Perche' nessun `AbilityId` e' cablato
//
// Le identita' di questo progetto si rinominano: `D-130` ha spostato `Flux` -> `Gadget` e `Riva` -> `Phase`,
// `D-334` ha spostato `Hero.Riktor` -> `Hero.Branth`. Un test che scrivesse `Hero.Gadget.LinearDischarge`
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

#endif // WITH_DEV_AUTOMATION_TESTS
