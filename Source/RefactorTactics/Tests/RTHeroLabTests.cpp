// Hero Lab 0.1 (#2600).
//
// Il test che conta davvero e' `ReusesAbilityLabRunner`: e' l'unico che prova il non-goal
// *«nessun secondo Ability Runner»*. Lo fa per CONFRONTO — la fixture che Hero Lab produce deve essere
// identica, campo per campo, a quella che #2599 produce per la stessa ability. Se Hero Lab si costruisse
// anche un solo campo per conto proprio, quel confronto cadrebbe.
//
// Un test che si limitasse a verificare «Hero Lab esegue e produce un TurnLog» sarebbe verde anche sopra un
// secondo runner scritto in casa, cioe' verde esattamente nel caso che la issue vieta.

#include "Misc/AutomationTest.h"
#include "Ability/RTHeroLab.h"
#include "Ability/RTAbilityLab.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestResult.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
namespace RTHeroLabTestsInternal
{
	UWorld* MakeHeroLabWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyHeroLabWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Il primo eroe che dichiara almeno un'ability indirizzabile. */
	bool FirstHeroWithKit(FRTHeroLabEntry& OutHero, FRTAbilityLabEntry& OutAbility)
	{
		for (const FRTHeroLabEntry& Hero : URTHeroLabLibrary::ListCanonicalHeroes())
		{
			const TArray<FRTAbilityLabEntry> Kit = URTHeroLabLibrary::ListHeroKit(Hero.HeroId);
			if (Kit.Num() > 0)
			{
				OutHero = Hero;
				OutAbility = Kit[0];
				return true;
			}
		}
		return false;
	}

	/** Confronto campo per campo di due scenari: e' la guardia sul riuso del runner. */
	bool ScenariosMatch(FAutomationTestBase& Test, const FRTTestScenario& A, const FRTTestScenario& B)
	{
		bool bOk = true;
		bOk &= Test.TestEqual(TEXT("stesso ScenarioId"), A.ScenarioId, B.ScenarioId);
		bOk &= Test.TestEqual(TEXT("stesso Seed"), A.Seed, B.Seed);
		bOk &= Test.TestEqual(TEXT("stesso MapRadius"), A.MapRadius, B.MapRadius);

		if (!Test.TestEqual(TEXT("stesso numero di unita'"), A.Units.Num(), B.Units.Num())) { return false; }
		for (int32 i = 0; i < A.Units.Num(); ++i)
		{
			bOk &= Test.TestEqual(TEXT("stesso Id di unita'"), A.Units[i].Id, B.Units[i].Id);
			bOk &= Test.TestEqual(TEXT("stesso HeroId"), A.Units[i].HeroId, B.Units[i].HeroId);
			bOk &= Test.TestEqual(TEXT("stessa squadra"), A.Units[i].TeamId, B.Units[i].TeamId);
			bOk &= Test.TestTrue(TEXT("stessa cella"), A.Units[i].Cell == B.Units[i].Cell);
		}

		if (!Test.TestEqual(TEXT("stesso numero di turni"), A.Turns.Num(), B.Turns.Num())) { return false; }
		for (int32 t = 0; t < A.Turns.Num(); ++t)
		{
			if (!Test.TestEqual(TEXT("stesso numero di intent"),
				A.Turns[t].Intents.Num(), B.Turns[t].Intents.Num()))
			{
				return false;
			}
			for (int32 i = 0; i < A.Turns[t].Intents.Num(); ++i)
			{
				bOk &= Test.TestEqual(TEXT("stesso soggetto"),
					A.Turns[t].Intents[i].UnitId, B.Turns[t].Intents[i].UnitId);
				bOk &= Test.TestEqual(TEXT("stessa ability"),
					A.Turns[t].Intents[i].Ability, B.Turns[t].Intents[i].Ability);
				bOk &= Test.TestEqual(TEXT("stesso bersaglio"),
					A.Turns[t].Intents[i].Target, B.Turns[t].Intents[i].Target);
			}
		}

		return bOk;
	}
}

/** I quattro `HeroId` canonici si risolvono, e sono quelli che il catalogo dichiara. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroLabAllFourHeroIdsResolveTest,
	"RefactorTactics.HeroLab.AllFourHeroIdsResolve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroLabAllFourHeroIdsResolveTest::RunTest(const FString&)
{
	const TArray<FRTHeroLabEntry> Heroes = URTHeroLabLibrary::ListCanonicalHeroes();
	TestEqual(TEXT("il roster della v0.1 ha quattro eroi"), Heroes.Num(), 4);

	// Non si cablano i nomi: si confronta con la seconda lista che il catalogo mantiene, che e' gia'
	// guardata da `Heroes.HeroIdsMatchRoster`. Cablarli qui creerebbe una terza copia dell'elenco.
	const TArray<FName> CatalogIds = URTHeroCatalogLibrary::GetHeroIds();
	TestEqual(TEXT("tanti quanti il catalogo"), Heroes.Num(), CatalogIds.Num());

	for (const FName& HeroId : CatalogIds)
	{
		FRTHeroLabEntry Entry;
		TestTrue(FString::Printf(TEXT("'%s' si risolve"), *HeroId.ToString()),
			URTHeroLabLibrary::FindHero(HeroId, Entry));
		TestEqual(TEXT("e' lo stesso eroe"), Entry.HeroId, HeroId);
		TestTrue(TEXT("dichiara punti vita"), Entry.MaxHealth > 0);
	}

	// `Hero.Riktor` e' l'identita' che `D-334` ha rinominato: non deve piu' risolversi.
	FRTHeroLabEntry Stale;
	TestFalse(TEXT("Hero.Riktor non esiste piu' come identita'"),
		URTHeroLabLibrary::FindHero(FName(TEXT("Hero.Riktor")), Stale));

	return true;
}

/** Nessun `HeroId` compare due volte. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroLabNoDuplicateHeroIdTest,
	"RefactorTactics.HeroLab.NoDuplicateHeroId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroLabNoDuplicateHeroIdTest::RunTest(const FString&)
{
	TSet<FName> Seen;
	for (const FRTHeroLabEntry& Hero : URTHeroLabLibrary::ListCanonicalHeroes())
	{
		TestFalse(FString::Printf(TEXT("'%s' non e' duplicato"), *Hero.HeroId.ToString()),
			Seen.Contains(Hero.HeroId));
		Seen.Add(Hero.HeroId);
	}
	TestEqual(TEXT("quattro identita' distinte"), Seen.Num(), 4);
	return true;
}

/**
 * Ogni ability dichiarata dal kit si risolve — e **nessuna viene saltata**.
 *
 * La seconda meta' e' il criterio vero: `ListCanonicalAbilities` scarta le azioni prive di `Def.ActionId`
 * perche' non sono indirizzabili per nome. Se un eroe ne avesse, il suo kit apparirebbe piu' corto di quanto
 * il catalogo dichiara, e il pannello mostrerebbe un roster incompleto **senza dirlo**. Questo test rende
 * visibile quello scarto invece di lasciarlo silenzioso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroLabDeclaredAbilitiesResolveTest,
	"RefactorTactics.HeroLab.DeclaredAbilitiesResolve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroLabDeclaredAbilitiesResolveTest::RunTest(const FString&)
{
	for (const FRTHeroLabEntry& Hero : URTHeroLabLibrary::ListCanonicalHeroes())
	{
		const TArray<FRTAbilityLabEntry> Kit = URTHeroLabLibrary::ListHeroKit(Hero.HeroId);

		TestTrue(FString::Printf(TEXT("'%s' dichiara un kit"), *Hero.HeroId.ToString()), Kit.Num() > 0);
		TestEqual(FString::Printf(TEXT("nessuna voce di '%s' e' stata saltata"), *Hero.HeroId.ToString()),
			Kit.Num(), Hero.DeclaredAbilityCount);

		for (const FRTAbilityLabEntry& Ability : Kit)
		{
			FRTAbilityLabEntry Resolved;
			TestTrue(FString::Printf(TEXT("'%s' si risolve nel catalogo"), *Ability.AbilityId.ToString()),
				URTAbilityLabLibrary::FindAbility(Ability.AbilityId, Resolved));
			TestEqual(TEXT("e' attribuita al suo eroe"), Resolved.OwnerHeroId, Hero.HeroId);
		}
	}
	return true;
}

/** Un'ability di un ALTRO eroe viene rifiutata: e' la domanda che l'Ability Lab non puo' porre. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroLabRejectsAbilityFromAnotherHeroTest,
	"RefactorTactics.HeroLab.RejectsAbilityFromAnotherHero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroLabRejectsAbilityFromAnotherHeroTest::RunTest(const FString&)
{
	const TArray<FRTHeroLabEntry> Heroes = URTHeroLabLibrary::ListCanonicalHeroes();
	if (!TestTrue(TEXT("servono almeno due eroi"), Heroes.Num() >= 2)) { return false; }

	const TArray<FRTAbilityLabEntry> OtherKit = URTHeroLabLibrary::ListHeroKit(Heroes[1].HeroId);
	if (!TestTrue(TEXT("il secondo eroe ha un kit"), OtherKit.Num() > 0)) { return false; }

	const FName Borrowed = OtherKit[0].AbilityId;

	// L'ability e' canonica — l'Ability Lab la eseguirebbe volentieri. Non e' pero' di QUESTO eroe.
	FRTAbilityLabEntry Canonical;
	TestTrue(TEXT("l'ability e' canonica"), URTAbilityLabLibrary::FindAbility(Borrowed, Canonical));

	FRTTestScenario Scenario;
	FString Error;
	TestFalse(TEXT("Hero Lab la rifiuta al primo eroe"),
		URTHeroLabLibrary::BuildHeroFixture(Heroes[0].HeroId, Borrowed,
			FRTAbilityLabFixtureSpec(), Scenario, Error));
	TestTrue(TEXT("l'errore nomina entrambi"),
		Error.Contains(Borrowed.ToString()) && Error.Contains(Heroes[0].HeroId.ToString()));
	TestEqual(TEXT("fail closed: nessuna unita' posata"), Scenario.Units.Num(), 0);

	// E un HeroId inesistente cade prima ancora.
	FString HeroError;
	TestFalse(TEXT("HeroId inesistente rifiutato"),
		URTHeroLabLibrary::BuildHeroFixture(FName(TEXT("Hero.Riktor")), Borrowed,
			FRTAbilityLabFixtureSpec(), Scenario, HeroError));
	TestTrue(TEXT("l'errore nomina l'eroe"), HeroError.Contains(TEXT("Hero.Riktor")));

	return true;
}

/**
 * **La guardia strutturale sul non-goal.** La fixture di Hero Lab e' identica a quella di #2599 per la
 * stessa ability: se Hero Lab avesse un runner proprio, i due scenari divergerebbero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroLabReusesAbilityLabRunnerTest,
	"RefactorTactics.HeroLab.ReusesAbilityLabRunner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroLabReusesAbilityLabRunnerTest::RunTest(const FString&)
{
	FRTHeroLabEntry Hero;
	FRTAbilityLabEntry Ability;
	if (!TestTrue(TEXT("un eroe con kit esiste"),
		RTHeroLabTestsInternal::FirstHeroWithKit(Hero, Ability)))
	{
		return false;
	}

	FRTAbilityLabFixtureSpec Spec;
	Spec.Seed = 7;

	FRTTestScenario ViaHeroLab;
	FString HeroError;
	if (!TestTrue(TEXT("Hero Lab costruisce"),
		URTHeroLabLibrary::BuildHeroFixture(Hero.HeroId, Ability.AbilityId, Spec, ViaHeroLab, HeroError)))
	{
		return false;
	}

	FRTTestScenario ViaAbilityLab;
	FString AbilityError;
	if (!TestTrue(TEXT("Ability Lab costruisce"),
		URTAbilityLabLibrary::BuildFixture(Ability.AbilityId, Spec, ViaAbilityLab, AbilityError)))
	{
		return false;
	}

	TestTrue(TEXT("le due fixture coincidono campo per campo"),
		RTHeroLabTestsInternal::ScenariosMatch(*this, ViaHeroLab, ViaAbilityLab));

	return true;
}

/** Stesso eroe, stessa ability, stesso seed: stesso TurnLog byte per byte. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroLabDeterministicRepeatTest,
	"RefactorTactics.HeroLab.DeterministicRepeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroLabDeterministicRepeatTest::RunTest(const FString&)
{
	FRTHeroLabEntry Hero;
	FRTAbilityLabEntry Ability;
	if (!TestTrue(TEXT("un eroe con kit esiste"),
		RTHeroLabTestsInternal::FirstHeroWithKit(Hero, Ability)))
	{
		return false;
	}

	FRTAbilityLabFixtureSpec Spec;
	Spec.Seed = 11;

	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("BuildHeroFixture riesce"),
		URTHeroLabLibrary::BuildHeroFixture(Hero.HeroId, Ability.AbilityId, Spec, Scenario, Error)))
	{
		return false;
	}

	UWorld* FirstWorld = RTHeroLabTestsInternal::MakeHeroLabWorld();
	if (!TestNotNull(TEXT("primo world"), FirstWorld)) { return false; }
	const FRTTestResult First = URTScenarioRunner::Run(FirstWorld, Scenario);
	RTHeroLabTestsInternal::DestroyHeroLabWorld(FirstWorld);

	UWorld* SecondWorld = RTHeroLabTestsInternal::MakeHeroLabWorld();
	if (!TestNotNull(TEXT("secondo world"), SecondWorld)) { return false; }
	const FRTTestResult Second = URTScenarioRunner::Run(SecondWorld, Scenario);
	RTHeroLabTestsInternal::DestroyHeroLabWorld(SecondWorld);

	if (First.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("ERROR su %s / %s: %s"),
			*Hero.HeroId.ToString(), *Ability.AbilityId.ToString(), *First.ErrorMessage));
		return false;
	}

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

#endif // WITH_DEV_AUTOMATION_TESTS
