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
#include "Ability/RTActionData.h"
#include "Ability/RTActionReadout.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTHeroLab.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
