// Il modello del Lab (#2599 Fetta B, #2600).
//
// I due test che contano davvero sono `RunWithoutHeroUsesAbilityLabFixture` e
// `RunWithHeroUsesHeroFixture`: provano **per confronto** che il modello non costruisce una terza fixture
// propria. La fixture che produce dev'essere identica, campo per campo, a quella che le due librerie
// canoniche producono per lo stesso ingresso.
//
// Un test che si limitasse a verificare «il Lab esegue e produce un TurnLog» sarebbe verde anche sopra un
// terzo percorso scritto in casa — cioe' verde esattamente nel caso che la Fetta B deve escludere.

#include "Misc/AutomationTest.h"

#include "RTLabViewModel.h"
#include "Ability/RTAbilityLab.h"
#include "Ability/RTHeroLab.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
namespace RTLabViewModelTestsInternal
{
	/** Il primo eroe che dichiara un kit non vuoto, con la sua prima ability. */
	bool PrimoEroeConKit(FRTHeroLabEntry& OutHero, FRTAbilityLabEntry& OutAbility)
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

	/** Un secondo eroe, diverso dal primo, con kit non vuoto. */
	bool SecondoEroeConKit(const FName& Escluso, FRTHeroLabEntry& OutHero, FRTAbilityLabEntry& OutAbility)
	{
		for (const FRTHeroLabEntry& Hero : URTHeroLabLibrary::ListCanonicalHeroes())
		{
			if (Hero.HeroId == Escluso) { continue; }
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

	/** Confronto campo per campo di due fixture: e' la guardia sul percorso unico. */
	bool FixtureCoincidono(FAutomationTestBase& Test, const FRTTestScenario& A, const FRTTestScenario& B)
	{
		bool bOk = true;
		bOk &= Test.TestEqual(TEXT("stesso ScenarioId"), A.ScenarioId, B.ScenarioId);
		bOk &= Test.TestEqual(TEXT("stesso Seed"), A.Seed, B.Seed);
		bOk &= Test.TestEqual(TEXT("stesso MapRadius"), A.MapRadius, B.MapRadius);

		if (!Test.TestEqual(TEXT("stesso numero di unita'"), A.Units.Num(), B.Units.Num())) { return false; }
		for (int32 i = 0; i < A.Units.Num(); ++i)
		{
			bOk &= Test.TestEqual(TEXT("stesso Id"), A.Units[i].Id, B.Units[i].Id);
			bOk &= Test.TestEqual(TEXT("stesso HeroId"), A.Units[i].HeroId, B.Units[i].HeroId);
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
				bOk &= Test.TestEqual(TEXT("stessa ability"),
					A.Turns[t].Intents[i].Ability, B.Turns[t].Intents[i].Ability);
				bOk &= Test.TestEqual(TEXT("stesso soggetto"),
					A.Turns[t].Intents[i].UnitId, B.Turns[t].Intents[i].UnitId);
			}
		}
		return bOk;
	}
}

/** Senza filtro l'elenco e' il catalogo canonico intero — le azioni core comprese. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabEmptyFilterShowsWholeCatalogTest,
	"RefactorTactics.Lab.EmptyFilterShowsWholeCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabEmptyFilterShowsWholeCatalogTest::RunTest(const FString&)
{
	FRTLabViewModel Modello;
	TestFalse(TEXT("nessun filtro all'apertura"), Modello.HasHeroFilter());

	const TArray<FRTAbilityLabEntry> Visibili = Modello.VisibleAbilities();
	const TArray<FRTAbilityLabEntry> Canoniche = URTAbilityLabLibrary::ListCanonicalAbilities();

	if (!TestEqual(TEXT("stesso numero di voci del catalogo"), Visibili.Num(), Canoniche.Num()))
	{
		return false;
	}
	for (int32 i = 0; i < Visibili.Num(); ++i)
	{
		TestEqual(TEXT("stesso ordine, stessa voce"), Visibili[i].AbilityId, Canoniche[i].AbilityId);
	}

	// Senza filtro non c'e' un eroe da descrivere: e' un'assenza dichiarata, non un readout vuoto.
	FRTHeroLabEntry Eroe;
	TestFalse(TEXT("nessun readout d'eroe senza filtro"), Modello.GetHeroReadout(Eroe));

	return true;
}

/** Con il filtro l'elenco e' esattamente il kit di quell'eroe. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabHeroFilterShowsOnlyThatKitTest,
	"RefactorTactics.Lab.HeroFilterShowsOnlyThatKit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabHeroFilterShowsOnlyThatKitTest::RunTest(const FString&)
{
	FRTHeroLabEntry Eroe;
	FRTAbilityLabEntry Ability;
	if (!TestTrue(TEXT("un eroe con kit esiste"),
		RTLabViewModelTestsInternal::PrimoEroeConKit(Eroe, Ability)))
	{
		return false;
	}

	FRTLabViewModel Modello;
	Modello.SetHeroFilter(Eroe.HeroId);
	TestTrue(TEXT("il filtro e' attivo"), Modello.HasHeroFilter());

	const TArray<FRTAbilityLabEntry> Visibili = Modello.VisibleAbilities();
	const TArray<FRTAbilityLabEntry> Kit = URTHeroLabLibrary::ListHeroKit(Eroe.HeroId);

	TestEqual(TEXT("l'elenco e' il kit"), Visibili.Num(), Kit.Num());
	TestTrue(TEXT("il kit non e' vuoto"), Visibili.Num() > 0);

	for (const FRTAbilityLabEntry& Voce : Visibili)
	{
		TestEqual(TEXT("ogni voce appartiene all'eroe filtrato"), Voce.OwnerHeroId, Eroe.HeroId);
		TestFalse(TEXT("nessuna azione core nel kit"), Voce.bIsCoreAction);
	}

	// Il filtro e' un restringimento reale, non un riordino: il catalogo intero ha piu' voci.
	TestTrue(TEXT("il catalogo intero e' piu' ampio del kit"),
		URTAbilityLabLibrary::ListCanonicalAbilities().Num() > Visibili.Num());

	FRTHeroLabEntry Letto;
	TestTrue(TEXT("il readout d'eroe c'e'"), Modello.GetHeroReadout(Letto));
	TestEqual(TEXT("ed e' quello filtrato"), Letto.HeroId, Eroe.HeroId);

	return true;
}

/**
 * Un'ability fuori dall'elenco visibile viene rifiutata, e la selezione precedente **resta**.
 *
 * E' la regola d'appartenenza di #2600 che affiora nel pannello: non una convenzione della UI, ma la
 * stessa domanda che `BuildHeroFixture` pone prima di delegare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabSelectionOutsideFilterIsRejectedTest,
	"RefactorTactics.Lab.SelectionOutsideFilterIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabSelectionOutsideFilterIsRejectedTest::RunTest(const FString&)
{
	FRTHeroLabEntry Primo, Secondo;
	FRTAbilityLabEntry AbilityPrimo, AbilitySecondo;
	if (!TestTrue(TEXT("primo eroe con kit"),
		RTLabViewModelTestsInternal::PrimoEroeConKit(Primo, AbilityPrimo))) { return false; }
	if (!TestTrue(TEXT("secondo eroe con kit"),
		RTLabViewModelTestsInternal::SecondoEroeConKit(Primo.HeroId, Secondo, AbilitySecondo))) { return false; }

	FRTLabViewModel Modello;
	Modello.SetHeroFilter(Primo.HeroId);

	TestTrue(TEXT("l'ability del proprio eroe si seleziona"), Modello.SelectAbility(AbilityPrimo.AbilityId));
	TestEqual(TEXT("ed e' selezionata"), Modello.GetSelectedAbility(), AbilityPrimo.AbilityId);

	// L'ability dell'altro eroe e' canonica — l'Ability Lab la eseguirebbe — ma non e' nell'elenco visibile.
	FRTAbilityLabEntry Canonica;
	TestTrue(TEXT("l'ability altrui e' canonica"),
		URTAbilityLabLibrary::FindAbility(AbilitySecondo.AbilityId, Canonica));

	TestFalse(TEXT("ma il modello la rifiuta"), Modello.SelectAbility(AbilitySecondo.AbilityId));
	TestEqual(TEXT("e la selezione precedente resta intatta"),
		Modello.GetSelectedAbility(), AbilityPrimo.AbilityId);

	return true;
}

/**
 * Cambiare eroe azzera una selezione che non appartiene al nuovo kit.
 *
 * ⚠️ E' il difetto che una verifica a mano non troverebbe: senza questa regola il pannello mostrerebbe il
 * readout dell'ability precedente sotto l'elenco di un altro eroe — numeri veri, che pero' non rispondono
 * a cio' che si sta guardando.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabChangingHeroClearsStaleSelectionTest,
	"RefactorTactics.Lab.ChangingHeroClearsStaleSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabChangingHeroClearsStaleSelectionTest::RunTest(const FString&)
{
	FRTHeroLabEntry Primo, Secondo;
	FRTAbilityLabEntry AbilityPrimo, AbilitySecondo;
	if (!TestTrue(TEXT("primo eroe con kit"),
		RTLabViewModelTestsInternal::PrimoEroeConKit(Primo, AbilityPrimo))) { return false; }
	if (!TestTrue(TEXT("secondo eroe con kit"),
		RTLabViewModelTestsInternal::SecondoEroeConKit(Primo.HeroId, Secondo, AbilitySecondo))) { return false; }

	FRTLabViewModel Modello;
	Modello.SetHeroFilter(Primo.HeroId);
	TestTrue(TEXT("selezione valida"), Modello.SelectAbility(AbilityPrimo.AbilityId));

	Modello.SetHeroFilter(Secondo.HeroId);
	TestTrue(TEXT("la selezione stantia e' sparita"), Modello.GetSelectedAbility().IsNone());

	// E cambiare filtro NON deve invece perdere una selezione che il nuovo elenco contiene ancora:
	// senza filtro il catalogo intero contiene tutto, quindi la selezione sopravvive.
	FRTLabViewModel Secondo2;
	Secondo2.SetHeroFilter(Primo.HeroId);
	TestTrue(TEXT("selezione valida"), Secondo2.SelectAbility(AbilityPrimo.AbilityId));
	Secondo2.SetHeroFilter(NAME_None);
	TestEqual(TEXT("togliere il filtro non perde una selezione ancora visibile"),
		Secondo2.GetSelectedAbility(), AbilityPrimo.AbilityId);

	return true;
}

/** Senza filtro la fixture e' **identica** a quella di `URTAbilityLabLibrary::BuildFixture`. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabRunWithoutHeroUsesAbilityLabFixtureTest,
	"RefactorTactics.Lab.RunWithoutHeroUsesAbilityLabFixture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabRunWithoutHeroUsesAbilityLabFixtureTest::RunTest(const FString&)
{
	FRTHeroLabEntry Eroe;
	FRTAbilityLabEntry Ability;
	if (!TestTrue(TEXT("un eroe con kit esiste"),
		RTLabViewModelTestsInternal::PrimoEroeConKit(Eroe, Ability))) { return false; }

	FRTLabViewModel Modello;
	Modello.MutableSpec().Seed = 21;
	TestTrue(TEXT("senza filtro l'ability e' visibile"), Modello.SelectAbility(Ability.AbilityId));

	FRTTestScenario DalModello;
	FString ErroreModello;
	if (!TestTrue(TEXT("il modello costruisce"), Modello.BuildScenario(DalModello, ErroreModello)))
	{
		return false;
	}

	FRTTestScenario DallaLibreria;
	FString ErroreLibreria;
	if (!TestTrue(TEXT("l'Ability Lab costruisce"),
		URTAbilityLabLibrary::BuildFixture(Ability.AbilityId, Modello.GetSpec(),
			DallaLibreria, ErroreLibreria)))
	{
		return false;
	}

	TestTrue(TEXT("le due fixture coincidono campo per campo"),
		RTLabViewModelTestsInternal::FixtureCoincidono(*this, DalModello, DallaLibreria));

	return true;
}

/** Con il filtro la fixture e' **identica** a quella di `URTHeroLabLibrary::BuildHeroFixture`. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabRunWithHeroUsesHeroFixtureTest,
	"RefactorTactics.Lab.RunWithHeroUsesHeroFixture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabRunWithHeroUsesHeroFixtureTest::RunTest(const FString&)
{
	FRTHeroLabEntry Eroe;
	FRTAbilityLabEntry Ability;
	if (!TestTrue(TEXT("un eroe con kit esiste"),
		RTLabViewModelTestsInternal::PrimoEroeConKit(Eroe, Ability))) { return false; }

	FRTLabViewModel Modello;
	Modello.MutableSpec().Seed = 21;
	Modello.SetHeroFilter(Eroe.HeroId);
	TestTrue(TEXT("l'ability del kit si seleziona"), Modello.SelectAbility(Ability.AbilityId));

	FRTTestScenario DalModello;
	FString ErroreModello;
	if (!TestTrue(TEXT("il modello costruisce"), Modello.BuildScenario(DalModello, ErroreModello)))
	{
		return false;
	}

	FRTTestScenario DallaLibreria;
	FString ErroreLibreria;
	if (!TestTrue(TEXT("l'Hero Lab costruisce"),
		URTHeroLabLibrary::BuildHeroFixture(Eroe.HeroId, Ability.AbilityId, Modello.GetSpec(),
			DallaLibreria, ErroreLibreria)))
	{
		return false;
	}

	TestTrue(TEXT("le due fixture coincidono campo per campo"),
		RTLabViewModelTestsInternal::FixtureCoincidono(*this, DalModello, DallaLibreria));

	// Senza ability selezionata non si costruisce nulla, e il motivo si legge.
	FRTLabViewModel Vuoto;
	FRTTestScenario Niente;
	FString Motivo;
	TestFalse(TEXT("senza selezione non si costruisce"), Vuoto.BuildScenario(Niente, Motivo));
	TestTrue(TEXT("il motivo e' scritto"), !Motivo.IsEmpty());
	TestEqual(TEXT("fail closed: nessuna unita' posata"), Niente.Units.Num(), 0);

	return true;
}

/**
 * Dopo una run il TurnLog e' leggibile, e l'esito distingue «non ho corso» da «ho corso».
 *
 * Il `UWorld` lo crea il test, come lo creera' il pannello: il modello non conosce `GEditor`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabLastRunExposesTurnLogLinesTest,
	"RefactorTactics.Lab.LastRunExposesTurnLogLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabLastRunExposesTurnLogLinesTest::RunTest(const FString&)
{
	FRTHeroLabEntry Eroe;
	FRTAbilityLabEntry Ability;
	if (!TestTrue(TEXT("un eroe con kit esiste"),
		RTLabViewModelTestsInternal::PrimoEroeConKit(Eroe, Ability))) { return false; }

	FRTLabViewModel Modello;
	Modello.SetHeroFilter(Eroe.HeroId);
	TestTrue(TEXT("selezione"), Modello.SelectAbility(Ability.AbilityId));

	TestFalse(TEXT("prima della run non c'e' esito"), Modello.LastRun().bHasRun);

	// Senza mondo si rifiuta, e il motivo finisce nell'esito invece che solo nel valore di ritorno.
	FString SenzaMondo;
	TestFalse(TEXT("senza mondo non si esegue"), Modello.Run(nullptr, SenzaMondo));
	TestTrue(TEXT("il motivo e' scritto"), !Modello.LastRun().Error.IsEmpty());
	TestFalse(TEXT("e non risulta eseguito"), Modello.LastRun().bHasRun);

	UWorld* Mondo = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!TestNotNull(TEXT("mondo creato"), Mondo)) { return false; }
	if (GEngine)
	{
		FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
		Ctx.SetCurrentWorld(Mondo);
	}

	FString Errore;
	const bool bEseguito = Modello.Run(Mondo, Errore);

	if (GEngine)
	{
		GEngine->DestroyWorldContext(Mondo);
	}
	Mondo->DestroyWorld(/*bInformEngineOfWorld=*/ false);

	if (!bEseguito)
	{
		AddError(FString::Printf(TEXT("la run non e' riuscita: %s"), *Errore));
		return false;
	}

	TestTrue(TEXT("risulta eseguito"), Modello.LastRun().bHasRun);
	TestTrue(TEXT("un turno giocato"), Modello.LastRun().TurnsPlayed >= 1);
	TestTrue(TEXT("il TurnLog e' leggibile"), Modello.LastRun().TurnLogLines.Num() > 0);
	for (const FString& Riga : Modello.LastRun().TurnLogLines)
	{
		TestFalse(TEXT("nessuna traccia illeggibile"), Riga.Contains(TEXT("non deserializzabile")));
	}

	// `ClearRun` azzera l'esito e **non** la selezione: non e' un reset del pannello.
	Modello.ClearRun();
	TestFalse(TEXT("l'esito e' azzerato"), Modello.LastRun().bHasRun);
	TestEqual(TEXT("la selezione resta"), Modello.GetSelectedAbility(), Ability.AbilityId);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
