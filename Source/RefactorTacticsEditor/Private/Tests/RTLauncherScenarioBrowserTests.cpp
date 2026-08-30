#include "Misc/AutomationTest.h"

#include "RTLauncherScenarioBrowser.h"
#include "ScenarioHarness/RTScenarioDraft.h"
#include "ScenarioHarness/RTScenarioIndex.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * La lista e il readout del launcher (#1705, sull'asse deciso da #1681).
 *
 * ⛔ **Cosa questi test NON coprono.** Che le due tendine si popolino, che la lista si ridisegni mentre si
 * digita e che il readout compaia accanto alla selezione sono Slate su un editor vivo: nessun automation
 * test li vede, e sono voce di seduta (`editor-sessions.yaml`, insieme a `U31`). Qui c'e' cio' che decide
 * *quali* id restano e *cosa* il readout dice — che e' dove la slice puo' sbagliare in silenzio.
 */

/**
 * L'invariante che l'asse di #1681 sta in piedi o cade: due tag si INTERSECANO.
 *
 * ⚠️ Misurato sul corpus vero e non su un indice inventato, perche' il difetto che prende e' proprio la
 * sostituzione dell'intersezione con l'unione: su dati finti sceglierei io le cardinalita' e il test
 * passerebbe anche con l'operatore sbagliato, purche' gli insiemi fossero disgiunti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherTagFiltersIntersectTest,
	"RefactorTactics.DevSandboxLauncher.TagFiltersIntersect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherTagFiltersIntersectTest::RunTest(const FString&)
{
	const TArray<FString> Tags = URTScenarioIndex::ListTags();

	// Senza corpus non c'e' misura, e un `return true` silenzioso sarebbe verde su nulla: e' esattamente
	// il caso che `rt-suite` esiste per non far registrare.
	if (!TestTrue(TEXT("il corpus espone dei tag: senza, questo test non misura niente"), Tags.Num() >= 2))
	{
		return false;
	}

	const TArray<FString> All = URTScenarioIndex::ListIds(FString(), FString());
	TestTrue(TEXT("due filtri vuoti danno l'elenco completo"), All.Num() > 0);

	int32 PairsChecked = 0;

	// Tutte le coppie sarebbero 53*52/2 letture dell'indice: si fermano alle prime che bastano a
	// distinguere intersezione da unione, cioe' quelle con entrambi i lati non vuoti.
	for (int32 i = 0; i < Tags.Num() && PairsChecked < 8; ++i)
	{
		for (int32 j = i + 1; j < Tags.Num() && PairsChecked < 8; ++j)
		{
			const TArray<FString> OnlyA = URTScenarioIndex::ListIds(Tags[i], FString());
			const TArray<FString> OnlyB = URTScenarioIndex::ListIds(Tags[j], FString());
			const TArray<FString> Both = URTScenarioIndex::ListIds(Tags[i], Tags[j]);

			if (OnlyA.Num() == 0 || OnlyB.Num() == 0)
			{
				continue;
			}

			++PairsChecked;

			TestTrue(FString::Printf(TEXT("'%s' E '%s': %d non supera i %d del solo primo"),
				*Tags[i], *Tags[j], Both.Num(), OnlyA.Num()), Both.Num() <= OnlyA.Num());

			TestTrue(FString::Printf(TEXT("'%s' E '%s': %d non supera i %d del solo secondo"),
				*Tags[i], *Tags[j], Both.Num(), OnlyB.Num()), Both.Num() <= OnlyB.Num());

			// L'unione sarebbe >= max(|A|,|B|): questa riga e' quella che la esclude quando i due
			// insiemi non sono disgiunti.
			TestTrue(FString::Printf(TEXT("'%s' E '%s': %d non supera l'elenco completo (%d)"),
				*Tags[i], *Tags[j], Both.Num(), All.Num()), Both.Num() <= All.Num());
		}
	}

	TestTrue(TEXT("almeno una coppia di tag con entrambi i lati non vuoti e' stata misurata"), PairsChecked > 0);

	return true;
}

/**
 * La ricerca restringe, e non puo' fare altro.
 *
 * Il caso che prende: una ricerca implementata ripartendo dall'indice invece che dall'elenco gia'
 * filtrato. Sembrerebbe funzionare in ogni prova a mano — finche' qualcuno cerca una parola che compare
 * anche in scenari che i tag avevano escluso, e se li vede ricomparire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherSearchIsSubsetTest,
	"RefactorTactics.DevSandboxLauncher.SearchNarrowsTheFilteredList",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherSearchIsSubsetTest::RunTest(const FString&)
{
	const TArray<FString> Filtered = {
		TEXT("Movement.Basic"),
		TEXT("Movement.Blocked"),
		TEXT("Reactions.Overwatch"),
		TEXT("Gadget.Mine"),
	};

	const TArray<FString> Hit = FRTLauncherScenarioBrowser::ApplySearch(Filtered, TEXT("movement"));
	TestEqual(TEXT("'movement' trova i due Movement"), Hit.Num(), 2);
	for (const FString& Id : Hit)
	{
		TestTrue(FString::Printf(TEXT("%s viene dall'elenco filtrato"), *Id), Filtered.Contains(Id));
	}

	// Maiuscole: l'id e' `Movement.Basic` e chi cerca digita minuscolo. Due esiti per la stessa parola
	// sarebbero un difetto che si nota solo per caso.
	TestEqual(TEXT("la ricerca ignora le maiuscole"),
		FRTLauncherScenarioBrowser::ApplySearch(Filtered, TEXT("MOVEMENT")).Num(), 2);

	// Sottostringa e non prefisso: chi cerca `overwatch` non sa sotto quale prefisso l'hanno messo.
	TestEqual(TEXT("la ricerca guarda dentro l'id, non solo l'inizio"),
		FRTLauncherScenarioBrowser::ApplySearch(Filtered, TEXT("overwatch")).Num(), 1);

	// Identita' a ricerca vuota: e' cio' che rende «cercare a filtri vuoti cerca su tutti» una
	// conseguenza invece di un ramo a parte.
	TestEqual(TEXT("una ricerca vuota non toglie niente"),
		FRTLauncherScenarioBrowser::ApplySearch(Filtered, FString()).Num(), Filtered.Num());
	TestEqual(TEXT("una ricerca di soli spazi non toglie niente"),
		FRTLauncherScenarioBrowser::ApplySearch(Filtered, TEXT("   ")).Num(), Filtered.Num());

	// Nessun id inventato: cercare qualcosa che non c'e' svuota, non pesca altrove.
	TestEqual(TEXT("una parola assente svuota l'elenco"),
		FRTLauncherScenarioBrowser::ApplySearch(Filtered, TEXT("nessunoscenariosichiamacosi")).Num(), 0);

	return true;
}

/**
 * L'elenco vuoto dice QUALE delle due cause lo ha svuotato.
 *
 * ⚠️ Il caso che conta e' il terzo: filtri che non lasciano passare niente **mentre c'e' del testo nella
 * casella**. Attribuirlo alla ricerca — che e' l'errore naturale, perche' la casella e' piena — manda a
 * cancellare la parola sbagliata, e l'elenco resta vuoto lo stesso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherEmptyStateNamesItsCauseTest,
	"RefactorTactics.DevSandboxLauncher.EmptyListNamesItsCause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherEmptyStateNamesItsCauseTest::RunTest(const FString&)
{
	// `TestTrue` con un confronto esplicito e non `TestEqual`: l'enum non ha una conversione a stringa, e
	// il messaggio di fallimento di `TestEqual` la vorrebbe.
	TestTrue(TEXT("con delle voci visibili non c'e' stato vuoto"),
		FRTLauncherScenarioBrowser::Classify(12, 3) == ERTLauncherListState::Populated);

	TestTrue(TEXT("i tag non lasciano passare niente"),
		FRTLauncherScenarioBrowser::Classify(0, 0) == ERTLauncherListState::NoTagMatches);

	TestTrue(TEXT("i tag lasciavano passare, la ricerca ha azzerato"),
		FRTLauncherScenarioBrowser::Classify(12, 0) == ERTLauncherListState::NoSearchMatches);

	// I due messaggi non possono essere lo stesso testo: se lo fossero, la distinzione esisterebbe
	// nell'enum e non sullo schermo, che e' l'unico posto dove serve.
	const FText NoTags = FRTLauncherScenarioBrowser::DescribeEmptyState(ERTLauncherListState::NoTagMatches);
	const FText NoSearch = FRTLauncherScenarioBrowser::DescribeEmptyState(ERTLauncherListState::NoSearchMatches);

	TestFalse(TEXT("il messaggio dei tag non e' vuoto"), NoTags.IsEmpty());
	TestFalse(TEXT("il messaggio della ricerca non e' vuoto"), NoSearch.IsEmpty());
	TestFalse(TEXT("i due messaggi sono distinguibili"), NoTags.EqualTo(NoSearch));

	TestTrue(TEXT("un elenco pieno non ha messaggio da mostrare"),
		FRTLauncherScenarioBrowser::DescribeEmptyState(ERTLauncherListState::Populated).IsEmpty());

	return true;
}

/**
 * Il terreno come lo scenario lo dichiara, nei due modi in cui il corpus lo dichiara davvero:
 * **21** scenari con una fixture, **67** con un raggio (#1705).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherTerrainReadoutTest,
	"RefactorTactics.DevSandboxLauncher.TerrainReadoutKeepsTheDeclaredForm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherTerrainReadoutTest::RunTest(const FString&)
{
	FRTScenarioSummary WithRadius;
	WithRadius.Fixture = FString();
	WithRadius.MapRadius = 4;
	TestEqual(TEXT("senza fixture si legge il raggio"),
		FRTLauncherScenarioBrowser::DescribeTerrain(WithRadius), FString(TEXT("radius 4")));

	FRTScenarioSummary WithFixture;
	WithFixture.Fixture = TEXT("Arena.V01");
	// ⚠️ `MapRadius` resta al suo default anche in uno scenario che parte da un allestimento: se la
	// funzione lo leggesse comunque, questo caso stamperebbe un raggio che nessuno ha scritto.
	WithFixture.MapRadius = 0;
	TestEqual(TEXT("con fixture si legge l'allestimento"),
		FRTLauncherScenarioBrowser::DescribeTerrain(WithFixture), FString(TEXT("fixture Arena.V01")));

	// La fixture vince anche quando il raggio e' valorizzato: e' cio' che lo scenario dichiara di essere.
	FRTScenarioSummary Both;
	Both.Fixture = TEXT("Arena.V01");
	Both.MapRadius = 9;
	TestEqual(TEXT("con entrambi vince cio' che e' dichiarato"),
		FRTLauncherScenarioBrowser::DescribeTerrain(Both), FString(TEXT("fixture Arena.V01")));

	// Nessuno dei due: il corpus oggi non ha questo caso, e la funzione non deve inventarne uno.
	FRTScenarioSummary Neither;
	TestEqual(TEXT("senza ne' l'uno ne' l'altro lo dice, invece di stampare radius 0"),
		FRTLauncherScenarioBrowser::DescribeTerrain(Neither), FString(TEXT("terreno non dichiarato")));

	return true;
}

/** La composizione e' un conteggio per squadra, ordinato, e non dipende dall'ordine del file. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherCompositionReadoutTest,
	"RefactorTactics.DevSandboxLauncher.CompositionCountsPerTeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherCompositionReadoutTest::RunTest(const FString&)
{
	auto MakeUnit = [](const TCHAR* Id, int32 TeamId)
	{
		FRTScenarioUnitView Unit;
		Unit.Id = Id;
		Unit.TeamId = TeamId;
		return Unit;
	};

	// Ordine di file volutamente mescolato: le squadre devono uscire per TeamId crescente comunque.
	const TArray<FRTScenarioUnitView> Units = {
		MakeUnit(TEXT("b1"), 1),
		MakeUnit(TEXT("a1"), 0),
		MakeUnit(TEXT("b2"), 1),
		MakeUnit(TEXT("a2"), 0),
	};

	TestEqual(TEXT("un 2v2 si legge come 2v2"),
		FRTLauncherScenarioBrowser::DescribeComposition(Units), FString(TEXT("team 0: 2 · team 1: 2")));

	TestEqual(TEXT("nessuna unita' lo dice"),
		FRTLauncherScenarioBrowser::DescribeComposition(TArray<FRTScenarioUnitView>()),
		FString(TEXT("nessuna unita' schierata")));

	// Il readout completo porta i conteggi del summary, non quelli ricontati sugli array: se i due
	// divergessero, la divergenza deve restare visibile.
	FRTScenarioSummary Summary;
	Summary.Fixture = TEXT("Arena.V01");
	Summary.UnitCount = 4;
	Summary.TurnCount = 22;
	Summary.ExpectationCount = 3;

	const TArray<FString> Lines = FRTLauncherScenarioBrowser::BuildReadout(Summary, Units);
	TestTrue(TEXT("il readout ha delle righe"), Lines.Num() >= 5);
	TestTrue(TEXT("la prima riga e' il terreno dichiarato"), Lines[0].Contains(TEXT("fixture Arena.V01")));
	TestTrue(TEXT("il readout porta la composizione"), Lines[1].Contains(TEXT("team 0: 2")));

	// `varianti` non compare quando sono zero: una riga costante su quasi tutti gli 88 scenari allontana
	// dall'occhio le righe che cambiano.
	for (const FString& Line : Lines)
	{
		TestFalse(TEXT("nessuna riga 'varianti' quando non ce ne sono"), Line.Contains(TEXT("varianti")));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
