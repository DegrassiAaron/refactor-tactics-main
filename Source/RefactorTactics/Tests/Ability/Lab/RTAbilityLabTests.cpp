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
			URTAbilityLabLibrary::BuildFixture(FName(TEXT("Hero.Branth.ImpactShot")), FRTAbilityLabFixtureSpec(), Scena, Errore)))
	{
		return false;
	}
	TestTrue(TEXT("🔴 l'Id che BuildFixture scrive e' una scena del banco"), URTAbilityLabLibrary::IsLabScenarioId(Scena.ScenarioId));
	TestFalse(TEXT("un prefisso senza azione non lo e'"), URTAbilityLabLibrary::IsLabScenarioId(TEXT("AbilityLab.")));
	TestFalse(TEXT("ne' con le maiuscole sbagliate"), URTAbilityLabLibrary::IsLabScenarioId(TEXT("abilitylab.Hero.Branth.ImpactShot")));
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

	if (TestTrue(TEXT("premessa: LinearDischarge e' nel kit"), FindKitAction(TEXT("Hero.Aevik.LinearDischarge"), Def, Shape)))
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
	if (TestTrue(TEXT("premessa: Overload e' nel kit"), FindKitAction(TEXT("Hero.Aevik.Overload"), Def, Shape))
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
	if (FindKitAction(TEXT("Hero.Aevik.Overload"), Def, Shape))
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
	const FName Attacco(TEXT("Hero.Aevik.LinearDischarge"));
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
 * su se stessi). Il bordo lo prende `Hero.Branth.KineticPanel`, che dichiara la stessa `StructureOp` del gadget.
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
	if (Esegui(TEXT("Hero.Aevik.LinearDischarge"), Scenario, Result))
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
	if (Esegui(TEXT("Hero.Aevik.Overload"), Scenario, Result))
	{
		TestTrue(TEXT("Overload: mira una cella"), Scenario.Turns[0].Intents[0].bTargetsCell);
		TestTrue(TEXT("Overload: l'area prende il nemico"),
			FieldBeforeAfter(Result, TEXT("TARGET"), TEXT("Health"), Prima, Dopo) && Dopo < Prima);
	}

	// Bordo: la copertura nasce.
	if (Esegui(TEXT("Hero.Branth.KineticPanel"), Scenario, Result))
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
	if (TestTrue(TEXT("premessa: LinearDischarge e' nel kit"), FindKitAction(TEXT("Hero.Aevik.LinearDischarge"), Def, Shape)))
	{
		TestEqual(TEXT("un attacco non e' una reazione"), ProvocationName(URTAbilityLabLibrary::ProvocationFor(Def)),
			ProvocationName(ERTAbilityLabProvocation::None));
	}
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
	for (const TCHAR* Id : { TEXT("Hero.Aevik.ReactiveCapacitor"), TEXT("Hero.Ivrin.Deflection") })
	{
		if (!Esegui(Id, Scenario, Voci)) { continue; }
		const FRTScenarioIntent* Arma = IntentOf(Scenario, TEXT("CASTER"));
		const FRTScenarioIntent* Provoca = IntentOf(Scenario, TEXT("TARGET"));
		TestTrue(FString::Printf(TEXT("%s: chi reagisce la arma, e non la usa"), Id),
			Arma && Arma->Reaction == FName(Id) && Arma->Ability.IsNone());
		TestTrue(FString::Printf(TEXT("%s: il TARGET colpisce chi reagisce"), Id),
			Provoca && !Provoca->Ability.IsNone() && Provoca->Target == TEXT("CASTER"));
		TestTrue(FString::Printf(TEXT("%s: scatta (Reaction/Activated)"), Id), HasEntry(Voci, ERTLogCategory::Reaction, FName(Id), Attivata));
	}

	// L'intercettazione: un alleato accanto a chi reagisce, e il `TARGET` colpisce lui.
	if (Esegui(TEXT("Hero.Branth.Interposition"), Scenario, Voci))
	{
		const FRTScenarioUnit* Alleato = UnitOf(Scenario, TEXT("ALLY"));
		const FRTScenarioIntent* Provoca = IntentOf(Scenario, TEXT("TARGET"));
		TestTrue(TEXT("Interposition: c'e' un alleato da proteggere"), Alleato && Alleato->TeamId == Scenario.Units[0].TeamId);
		TestTrue(TEXT("Interposition: il TARGET colpisce l'alleato"), Provoca && Provoca->Target == TEXT("ALLY"));
		TestTrue(TEXT("Interposition: scatta (Reaction/Activated)"),
			HasEntry(Voci, ERTLogCategory::Reaction, TEXT("Hero.Branth.Interposition"), Attivata));
	}

	// L'Overwatch: chi guarda verso il bersaglio, il bersaglio entra nella linea, la finestra si apre.
	if (Esegui(TEXT("Action.Overwatch"), Scenario, Voci))
	{
		const FRTScenarioIntent* Provoca = IntentOf(Scenario, TEXT("TARGET"));
		TestTrue(TEXT("Overwatch: le finestre del giocatore sono aperte"), Scenario.bPlayerReactionWindows);
		TestTrue(TEXT("Overwatch: il TARGET fa un passo"), Provoca && Provoca->Move.Num() == 1);
		TestTrue(TEXT("Overwatch: la finestra si apre (ReactionDecision)"),
			HasEntry(Voci, ERTLogCategory::ReactionDecision, TEXT("Action.Overwatch")));
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
		TestTrue(TEXT("Brace: la finestra si apre (ReactionDecision)"),
			HasEntry(Voci, ERTLogCategory::ReactionDecision, TEXT("Action.Brace")));
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

#endif // WITH_DEV_AUTOMATION_TESTS
