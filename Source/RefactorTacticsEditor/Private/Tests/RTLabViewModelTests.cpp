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
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "Tests/RTScenarioTestSupport.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
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

/**
 * Una lista vuota DICE PERCHE', e i due modi di essere vuota non si leggono uguali (`#3461`).
 *
 * 🔑 **Il caso osservato in seduta, pinnato**: scrivendo `Ivrin` invece di `Hero.Ivrin` la lista si
 * svuotava in silenzio, e la lettura naturale e' stata *«non le filtra»*. Non era il filtro: era il
 * formato, e il pannello rendeva quel caso con la frase riservata a «nessun filtro» -- cioe' dicendo
 * «catalogo canonico intero» proprio mentre mostrava zero voci.
 *
 * ⚠️ **L'id si DERIVA dal catalogo, e il nome nudo si ricava togliendo il prefisso.** Scrivere
 * `Hero.Ivrin` a mano legherebbe il test a un roster che puo' cambiare, e un roster rinominato lo
 * lascerebbe verde sulla domanda sbagliata.
 *
 * ⛔ **La mutazione che questo test uccide**: far rispondere `DescribeFilterState` con `NoFilter`
 * ogni volta che `FindHero` fallisce -- che e' esattamente il comportamento di prima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabEmptyListSaysWhyTest,
	"RefactorTactics.Lab.EmptyListSaysWhy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabEmptyListSaysWhyTest::RunTest(const FString&)
{
	FRTHeroLabEntry Atteso;
	FRTAbilityLabEntry Ability;
	if (!TestTrue(TEXT("premessa: il catalogo dichiara almeno un eroe con kit"),
			RTLabViewModelTestsInternal::PrimoEroeConKit(Atteso, Ability)))
	{
		return false;
	}

	FRTLabViewModel Modello;
	FRTHeroLabEntry Eroe;

	// --- stato 1: nessun filtro ---
	TestTrue(TEXT("senza filtro: NoFilter"),
		Modello.DescribeFilterState(Eroe) == FRTLabViewModel::EFilterState::NoFilter);

	// --- stato 4: l'id COMPLETO trova l'eroe e il suo kit ---
	Modello.SetHeroFilter(Atteso.HeroId);
	TestTrue(TEXT("con l'id completo: HeroWithKit"),
		Modello.DescribeFilterState(Eroe) == FRTLabViewModel::EFilterState::HeroWithKit);
	TestEqual(TEXT("e l'eroe valorizzato e' quello chiesto"), Eroe.HeroId, Atteso.HeroId);

	// --- stato 2: il NOME NUDO, cioe' il caso della seduta ---
	// Il prefisso si toglie invece di scriverlo: `Hero.Ivrin` -> `Ivrin`, qualunque sia il roster.
	FString Nudo = Atteso.HeroId.ToString();
	int32 Punto = INDEX_NONE;
	if (Nudo.FindLastChar(TEXT('.'), Punto) && Punto + 1 < Nudo.Len())
	{
		Nudo = Nudo.RightChop(Punto + 1);
	}
	if (!TestNotEqual(TEXT("premessa: il nome nudo e' DIVERSO dall'id -- senza prefisso non c'e' caso"),
			Nudo, Atteso.HeroId.ToString()))
	{
		return false;
	}

	Modello.SetHeroFilter(FName(*Nudo));
	TestTrue(TEXT("col nome nudo: UnknownHeroId, NON NoFilter"),
		Modello.DescribeFilterState(Eroe) == FRTLabViewModel::EFilterState::UnknownHeroId);

	// ⛔ E la premessa che rende il difetto quello che era: il filtro E' attivo, quindi l'elenco e' vuoto
	// per il ramo filtrato -- non perche' il modello sia tornato al catalogo intero.
	TestTrue(TEXT("il filtro e' attivo"), Modello.HasHeroFilter());
	TestEqual(TEXT("e l'elenco e' VUOTO"), Modello.VisibleAbilities().Num(), 0);

	// ⚠️ Controllo positivo sullo stesso oggetto: tornando all'id completo l'elenco si ripopola. Senza,
	// l'asserto sopra sarebbe verde anche su un modello che rende sempre un elenco vuoto.
	Modello.SetHeroFilter(Atteso.HeroId);
	TestTrue(TEXT("controllo positivo: con l'id completo l'elenco non e' vuoto"),
		Modello.VisibleAbilities().Num() > 0);

	return true;
}

/**
 * `PrepareForPie` scrive un file che l'indice risolve a QUEL percorso e che si rilegge uguale alla fixture
 * in memoria (spec §5.1). Il confronto campo per campo e' lo stesso di `RunWithoutHeroUsesAbilityLabFixture`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabPrepareForPieWritesAResolvableScenarioTest,
	"RefactorTactics.Lab.PrepareForPieWritesAResolvableScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabPrepareForPieWritesAResolvableScenarioTest::RunTest(const FString&)
{
	using namespace RTScenarioTestSupport;
	using namespace RTLabViewModelTestsInternal;
	const FString Root = ApriRadiceDiProva(TEXT("RTLabPrepare"), TEXT("Scrive"));
	ON_SCOPE_EXIT{ ChiudiRadiceDiProva(Root); };

	FRTHeroLabEntry Eroe;
	FRTAbilityLabEntry Ability;
	if (!TestTrue(TEXT("un eroe con kit esiste"), PrimoEroeConKit(Eroe, Ability))) { return false; }

	FRTLabViewModel Modello;
	Modello.MutableSpec().Seed = 21;
	TestTrue(TEXT("l'ability si seleziona"), Modello.SelectAbility(Ability.AbilityId));

	FString Id, Errore;
	if (!TestTrue(TEXT("PrepareForPie riesce"), Modello.PrepareForPie(Id, Errore)))
	{
		AddError(Errore);
		return false;
	}
	TestEqual(TEXT("l'Id e' quello della fixture"), Id, FString::Printf(TEXT("AbilityLab.%s"), *Ability.AbilityId.ToString()));

	const FString Atteso = FPaths::ConvertRelativePathToFull(FPaths::Combine(Root, Id + TEXT(".json")));
	TestTrue(TEXT("il file esiste nella radice del Lab"), IFileManager::Get().FileExists(*Atteso));

	FString ErroreIndice;
	const FString Risolto = URTScenarioIndex::ResolvePath(Id, ErroreIndice);
	TestTrue(TEXT("l'indice risolve l'Id a QUEL file"), FPaths::IsSamePath(Risolto, Atteso));

	FRTTestScenario InMemoria, DaDisco;
	FString E1, E2;
	if (!TestTrue(TEXT("la fixture in memoria si costruisce"), Modello.BuildScenario(InMemoria, E1))) { return false; }
	if (!TestTrue(TEXT("il file si rilegge"), URTScenarioLoader::LoadFromFile(Atteso, DaDisco, E2))) { AddError(E2); return false; }
	TestTrue(TEXT("il file rilegge la stessa fixture, campo per campo"), FixtureCoincidono(*this, InMemoria, DaDisco));
	// Asserto sul VALORE, non sul confronto fra i due array: tolto il tag dalla fixture, due array vuoti
	// sarebbero uguali e il vecchio `TestEqual` passerebbe.
	TestTrue(TEXT("e porta il tag del Lab"), DaDisco.Tags.Contains(TEXT("ability-lab")));
	return true;
}

/** Senza selezione: `false`, motivo scritto, NESSUN file. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabPrepareForPieRefusesAndWritesNothingTest,
	"RefactorTactics.Lab.PrepareForPieRefusesAndWritesNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabPrepareForPieRefusesAndWritesNothingTest::RunTest(const FString&)
{
	using namespace RTScenarioTestSupport;
	using namespace RTLabViewModelTestsInternal;
	const FString Root = ApriRadiceDiProva(TEXT("RTLabPrepare"), TEXT("Rifiuta"));
	ON_SCOPE_EXIT{ ChiudiRadiceDiProva(Root); };

	FRTLabViewModel Modello; // nessuna ability selezionata
	FString Id, Errore;
	TestFalse(TEXT("senza selezione PrepareForPie rifiuta"), Modello.PrepareForPie(Id, Errore));
	TestFalse(TEXT("e dice perche'"), Errore.IsEmpty());
	TestTrue(TEXT("e l'Id resta vuoto"), Id.IsEmpty());
	TestEqual(TEXT("e non scrive nessun file"), FileJsonIn(Root), 0);
	return true;
}

/** Due file con lo stesso Id nella radice del Lab: l'Id e' ambiguo e `PrepareForPie` lo dice, invece di lasciarlo al GameMode. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabPrepareForPieRefusesAnAmbiguousIdTest,
	"RefactorTactics.Lab.PrepareForPieRefusesAnAmbiguousId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabPrepareForPieRefusesAnAmbiguousIdTest::RunTest(const FString&)
{
	using namespace RTScenarioTestSupport;
	using namespace RTLabViewModelTestsInternal;
	const FString Root = ApriRadiceDiProva(TEXT("RTLabPrepare"), TEXT("Doppione"));
	ON_SCOPE_EXIT{ ChiudiRadiceDiProva(Root); };

	FRTHeroLabEntry Eroe;
	FRTAbilityLabEntry Ability;
	if (!TestTrue(TEXT("un eroe con kit esiste"), PrimoEroeConKit(Eroe, Ability))) { return false; }

	// Un secondo file, con nome diverso, che dichiara lo stesso Id della fixture.
	const FString IdFixture = FString::Printf(TEXT("AbilityLab.%s"), *Ability.AbilityId.ToString());
	ScriviHeaderScenario(Root, TEXT("Doppione.json"), *IdFixture, TEXT("\"ability-lab\""));

	FRTLabViewModel Modello;
	TestTrue(TEXT("l'ability si seleziona"), Modello.SelectAbility(Ability.AbilityId));

	FString Id, Errore;
	TestFalse(TEXT("con un doppione l'Id e' ambiguo e PrepareForPie rifiuta"), Modello.PrepareForPie(Id, Errore));
	// ⚠️ Il messaggio incorpora il percorso e `Contains` e' case-insensitive: la cartella di prova non deve contenere la parola cercata.
	TestTrue(TEXT("e il motivo dice che e' ambiguo"), Errore.Contains(TEXT("ambigu")));
	TestTrue(TEXT("e il motivo e' quello dell'indice"), Errore.Contains(TEXT("dichiarato da")));
	// Un file che rende ambiguo un Id avvelenerebbe console e GameMode a ogni clic: non deve restare.
	TestFalse(TEXT("il file del Lab non resta sul disco"),
		IFileManager::Get().FileExists(*FPaths::Combine(Root, IdFixture + TEXT(".json"))));
	TestTrue(TEXT("il doppione preesistente non viene toccato"),
		IFileManager::Get().FileExists(*FPaths::Combine(Root, TEXT("Doppione.json"))));
	return true;
}

/**
 * Sotto automation, senza override, la radice del Lab e' VUOTA: `PrepareForPie` rifiuta invece di creare e
 * scrivere in una cartella di cui nessuno ha deciso l'esistenza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabPrepareForPieRefusesWithoutARootTest,
	"RefactorTactics.Lab.PrepareForPieRefusesWithoutARootUnderAutomation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabPrepareForPieRefusesWithoutARootTest::RunTest(const FString&)
{
	using namespace RTLabViewModelTestsInternal;
	URTScenarioLoader::SetLabScenariosRootOverrideForTest(FString());
	ON_SCOPE_EXIT{ URTScenarioLoader::SetLabScenariosRootOverrideForTest(FString()); };

	// Controllo positivo della premessa: senza, il rifiuto potrebbe venire da altro.
	TestTrue(TEXT("premessa: la radice del Lab e' vuota"), URTScenarioLoader::LabScenariosRoot().IsEmpty());

	FRTHeroLabEntry Eroe;
	FRTAbilityLabEntry Ability;
	if (!TestTrue(TEXT("un eroe con kit esiste"), PrimoEroeConKit(Eroe, Ability))) { return false; }

	FRTLabViewModel Modello;
	TestTrue(TEXT("l'ability si seleziona"), Modello.SelectAbility(Ability.AbilityId));

	FString Id, Errore;
	TestFalse(TEXT("senza radice PrepareForPie rifiuta"), Modello.PrepareForPie(Id, Errore));
	TestFalse(TEXT("e dice perche'"), Errore.IsEmpty());
	TestTrue(TEXT("e l'Id resta vuoto"), Id.IsEmpty());
	return true;
}

/** Una fixture stantia con lo stesso Id viene sovrascritta: su disco c'e' l'ultimo clic. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabPrepareForPieOverwritesAStaleFixtureTest,
	"RefactorTactics.Lab.PrepareForPieOverwritesAStaleFixture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabPrepareForPieOverwritesAStaleFixtureTest::RunTest(const FString&)
{
	using namespace RTScenarioTestSupport;
	using namespace RTLabViewModelTestsInternal;
	const FString Root = ApriRadiceDiProva(TEXT("RTLabPrepare"), TEXT("Stantio"));
	ON_SCOPE_EXIT{ ChiudiRadiceDiProva(Root); };

	FRTHeroLabEntry Eroe;
	FRTAbilityLabEntry Ability;
	if (!TestTrue(TEXT("un eroe con kit esiste"), PrimoEroeConKit(Eroe, Ability))) { return false; }

	FRTLabViewModel Modello;
	TestTrue(TEXT("l'ability si seleziona"), Modello.SelectAbility(Ability.AbilityId));

	FString Id, Errore;
	Modello.MutableSpec().Seed = 7;
	if (!TestTrue(TEXT("prima corsa"), Modello.PrepareForPie(Id, Errore))) { AddError(Errore); return false; }
	Modello.MutableSpec().Seed = 11;
	if (!TestTrue(TEXT("seconda corsa, stesso Id"), Modello.PrepareForPie(Id, Errore))) { AddError(Errore); return false; }

	FRTTestScenario DaDisco;
	FString E;
	const FString Percorso = FPaths::Combine(Root, Id + TEXT(".json"));
	if (!TestTrue(TEXT("il file si rilegge"), URTScenarioLoader::LoadFromFile(Percorso, DaDisco, E))) { AddError(E); return false; }
	TestEqual(TEXT("su disco c'e' il seed dell'ultimo clic"), DaDisco.Seed, 11);
	TestEqual(TEXT("e c'e' un solo file"), FileJsonIn(Root), 1);
	return true;
}

/**
 * L'Id dell'ultimo lancio in PIE vive nel modello e lo azzera ogni gesto successivo (#3542): selezione,
 * filtro, run. La riga di stato mostra una cosa sola, l'ultimo gesto.
 *
 * ⚠️ **Controlli positivi sullo stesso oggetto**: un `SelectAbility` rifiutato e un `SetHeroFilter` che non
 * cambia il filtro NON sono gesti, e l'Id deve restare. Senza, l'asserto sarebbe verde anche su un modello
 * che azzera a ogni chiamata.
 *
 * Per `Run` basta il percorso senza mondo: `Run` azzera in testa, prima di qualunque rifiuto, quindi non
 * serve un `UWorld` transitorio per provare l'azzeramento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabLaunchedIdIsClearedBySelectionFilterAndRunTest,
	"RefactorTactics.Lab.LaunchedIdIsClearedBySelectionFilterAndRun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabLaunchedIdIsClearedBySelectionFilterAndRunTest::RunTest(const FString&)
{
	using namespace RTLabViewModelTestsInternal;
	const TArray<FRTAbilityLabEntry> Catalogo = URTAbilityLabLibrary::ListCanonicalAbilities();
	if (!TestTrue(TEXT("premessa: il catalogo ha almeno due ability"), Catalogo.Num() >= 2)) { return false; }

	FRTHeroLabEntry Eroe;
	FRTAbilityLabEntry AbilityEroe;
	if (!TestTrue(TEXT("premessa: un eroe con kit esiste"), PrimoEroeConKit(Eroe, AbilityEroe))) { return false; }

	const FString IdLanciato = TEXT("AbilityLab.X");
	FRTLabViewModel Modello;
	TestTrue(TEXT("all'apertura nessun lancio"), Modello.LaunchedScenarioId().IsEmpty());
	TestTrue(TEXT("la prima selezione e' accettata"), Modello.SelectAbility(Catalogo[0].AbilityId));

	// --- controllo positivo: i non-gesti non azzerano ---
	Modello.NoteLaunched(IdLanciato);
	TestEqual(TEXT("NoteLaunched imposta l'Id"), Modello.LaunchedScenarioId(), IdLanciato);
	TestFalse(TEXT("un'ability inesistente e' rifiutata"), Modello.SelectAbility(TEXT("Ability.NonEsiste")));
	TestEqual(TEXT("un SelectAbility rifiutato NON azzera l'Id"), Modello.LaunchedScenarioId(), IdLanciato);
	Modello.SetHeroFilter(NAME_None); // gia' senza filtro: non cambia nulla
	TestEqual(TEXT("un SetHeroFilter che non cambia il filtro NON azzera l'Id"), Modello.LaunchedScenarioId(), IdLanciato);

	// --- la selezione di un'altra ability ---
	TestTrue(TEXT("un'altra ability si seleziona"), Modello.SelectAbility(Catalogo[1].AbilityId));
	TestTrue(TEXT("SelectAbility accettato azzera l'Id"), Modello.LaunchedScenarioId().IsEmpty());

	// --- il filtro ---
	Modello.NoteLaunched(IdLanciato);
	Modello.SetHeroFilter(Eroe.HeroId);
	TestTrue(TEXT("SetHeroFilter che cambia azzera l'Id"), Modello.LaunchedScenarioId().IsEmpty());

	// --- la run (senza mondo: azzera in testa, poi rifiuta) ---
	Modello.NoteLaunched(IdLanciato);
	FString Errore;
	TestFalse(TEXT("senza mondo la run rifiuta"), Modello.Run(nullptr, Errore));
	TestTrue(TEXT("Run azzera l'Id anche quando rifiuta"), Modello.LaunchedScenarioId().IsEmpty());
	return true;
}

/**
 * A fine PIE il modello azzera l'Id e ricorda che il lancio e' finito: e' la riga «PIE terminato» del
 * pannello (#3542). Un gesto successivo la toglie, come toglie l'Id.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabLaunchFinishedClearsTheLaunchedIdTest,
	"RefactorTactics.Lab.LaunchFinishedClearsTheLaunchedId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabLaunchFinishedClearsTheLaunchedIdTest::RunTest(const FString&)
{
	const TArray<FRTAbilityLabEntry> Catalogo = URTAbilityLabLibrary::ListCanonicalAbilities();
	if (!TestTrue(TEXT("premessa: il catalogo ha almeno un'ability"), Catalogo.Num() >= 1)) { return false; }

	FRTLabViewModel Modello;
	TestFalse(TEXT("all'apertura nessun lancio e' finito"), Modello.WasLaunchFinished());

	Modello.NoteLaunched(TEXT("AbilityLab.X"));
	TestFalse(TEXT("controllo positivo: lanciato ma non finito"), Modello.WasLaunchFinished());

	// (a) fine PIE con ripristino riuscito
	Modello.NoteLaunchFinished(true);
	TestTrue(TEXT("a fine PIE l'Id e' vuoto"), Modello.LaunchedScenarioId().IsEmpty());
	TestTrue(TEXT("e il lancio risulta finito"), Modello.WasLaunchFinished());
	TestTrue(TEXT("e il ripristino risulta riuscito"), Modello.LastLaunchRestored());

	TestTrue(TEXT("un gesto successivo: la selezione e' accettata"), Modello.SelectAbility(Catalogo[0].AbilityId));
	TestFalse(TEXT("SelectAbility toglie la riga «PIE terminato»"), Modello.WasLaunchFinished());

	// (b) l'ultimo gesto vince: una selezione fra il lancio e la fine del PIE non viene scavalcata.
	// Controllo positivo: stesso modello, stesso ordine di chiamate di (a), salvo il gesto in mezzo.
	Modello.NoteLaunched(TEXT("AbilityLab.X"));
	TestTrue(TEXT("(b) la selezione e' accettata"), Modello.SelectAbility(Catalogo[0].AbilityId));
	Modello.NoteLaunchFinished(true);
	TestFalse(TEXT("(b) la fine del PIE NON scavalca il gesto successivo"), Modello.WasLaunchFinished());
	TestTrue(TEXT("(b) e l'Id resta vuoto"), Modello.LaunchedScenarioId().IsEmpty());

	// (c) ripristino fallito: la riga «terminato» c'e', ma dice che il ripristino non ha preso.
	Modello.NoteLaunched(TEXT("AbilityLab.X"));
	Modello.NoteLaunchFinished(false);
	TestTrue(TEXT("(c) il lancio risulta finito"), Modello.WasLaunchFinished());
	TestFalse(TEXT("(c) e il ripristino risulta NON riuscito"), Modello.LastLaunchRestored());

	// (d) ripristino fallito DOPO un gesto successivo: si registra comunque. Stessa sequenza di (b), salvo
	// l'esito: con `true` (b) non registra, con `false` si'.
	Modello.NoteLaunched(TEXT("AbilityLab.X"));
	TestTrue(TEXT("(d) la selezione e' accettata"), Modello.SelectAbility(Catalogo[0].AbilityId));
	Modello.NoteLaunchFinished(false);
	TestTrue(TEXT("(d) un ripristino fallito si registra anche dopo un gesto successivo"), Modello.WasLaunchFinished());
	TestFalse(TEXT("(d) e dice che il ripristino NON e' riuscito"), Modello.LastLaunchRestored());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
