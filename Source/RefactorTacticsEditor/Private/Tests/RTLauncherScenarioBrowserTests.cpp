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
 * L'invariante su cui l'asse di #1681 sta in piedi o cade: due tag si INTERSECANO.
 *
 * ⚠️ **L'oracolo e' l'uguaglianza con l'intersezione calcolata qui, non una disuguaglianza.** Asserire
 * solo che il risultato non superi nessuno dei due lati e' un limite SUPERIORE, e lo rispettano anche due
 * implementazioni rotte: una che restituisce sempre l'insieme vuoto, e una che perde delle corrispondenze
 * vere. Confrontare con `OnlyA ∩ OnlyB` prende entrambe, ed e' anche l'unico modo di escludere l'unione
 * quando i due insiemi non sono disgiunti.
 *
 * Misurato sul corpus vero e non su un indice inventato: su dati finti sceglierei io le cardinalita', e
 * il test passerebbe anche con l'operatore sbagliato purche' gli insiemi fossero disgiunti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherTagFiltersIntersectTest,
	"RefactorTactics.DevSandboxLauncher.TagFiltersIntersect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherTagFiltersIntersectTest::RunTest(const FString&)
{
	// ⚠️ **Una sola scansione del corpus**, e le attese si costruiscono da qui in memoria. `ListIds` passa
	// da `Scan` a ogni chiamata — novanta letture e novanta parse — quindi un test che la interrogasse per
	// ogni tag pagherebbe il corpus decine di volte per rispondere a domande che una scansione sola copre.
	TArray<FString> Problems;
	const TArray<FRTScenarioEntry> Entries = URTScenarioIndex::Scan(Problems);

	if (!TestTrue(TEXT("il corpus si legge: senza, questo test non misura niente"), Entries.Num() > 0))
	{
		return false;
	}

	TMap<FString, TArray<FString>> IdsByTag;
	for (const FRTScenarioEntry& Entry : Entries)
	{
		for (const FString& EntryTag : Entry.Tags)
		{
			IdsByTag.FindOrAdd(EntryTag).AddUnique(Entry.ScenarioId);
		}
	}

	TArray<FString> Tags;
	IdsByTag.GetKeys(Tags);
	Tags.Sort();

	if (!TestTrue(TEXT("il corpus espone almeno due tag"), Tags.Num() >= 2))
	{
		return false;
	}

	/**
	 * ⚠️ **Le coppie si CERCANO, non si prendono le prime.** Misurato: fra le prime otto coppie in ordine
	 * alfabetico nessuna condivide uno scenario, e su coppie disgiunte intersezione e «restituisci sempre
	 * vuoto» danno lo stesso risultato — il test sarebbe verde senza distinguere niente. Servono entrambe
	 * le famiglie: quelle che si sovrappongono per prendere un'implementazione che perde corrispondenze, e
	 * almeno una disgiunta per prendere l'unione.
	 */
	TArray<TPair<FString, FString>> Overlapping;
	TPair<FString, FString> Disjoint;
	bool bFoundDisjoint = false;

	for (int32 i = 0; i < Tags.Num() && Overlapping.Num() < 3; ++i)
	{
		for (int32 j = i + 1; j < Tags.Num() && Overlapping.Num() < 3; ++j)
		{
			const TArray<FString>& A = IdsByTag[Tags[i]];
			const TArray<FString>& B = IdsByTag[Tags[j]];

			bool bShares = false;
			for (const FString& Id : A)
			{
				if (B.Contains(Id)) { bShares = true; break; }
			}

			if (bShares)
			{
				Overlapping.Add({ Tags[i], Tags[j] });
			}
			else if (!bFoundDisjoint)
			{
				Disjoint = { Tags[i], Tags[j] };
				bFoundDisjoint = true;
			}
		}
	}

	// Se questo fallisce non e' rotto `ListIds`: e' il corpus a non avere due tag sullo stesso scenario, e
	// senza quello l'asse a due filtri di #1681 non ha nulla da esprimere.
	if (!TestTrue(TEXT("il corpus ha almeno una coppia di tag che condivide degli scenari"), Overlapping.Num() > 0))
	{
		return false;
	}

	for (const TPair<FString, FString>& Pair : Overlapping)
	{
		// L'oracolo: l'intersezione calcolata qui dalle entry, non un limite superiore.
		TArray<FString> Expected;
		for (const FString& Id : IdsByTag[Pair.Key])
		{
			if (IdsByTag[Pair.Value].Contains(Id))
			{
				Expected.Add(Id);
			}
		}
		Expected.Sort();

		const TArray<FString> Both = URTScenarioIndex::ListIds(Pair.Key, Pair.Value);

		TestEqual(FString::Printf(TEXT("'%s' E '%s': %d id contro i %d attesi"),
			*Pair.Key, *Pair.Value, Both.Num(), Expected.Num()), Both, Expected);

		// Non vuota per costruzione: e' cio' che distingue l'intersezione da un `return {}` che passerebbe
		// ogni disuguaglianza.
		TestTrue(FString::Printf(TEXT("'%s' E '%s': l'intersezione non e' vuota"), *Pair.Key, *Pair.Value),
			Both.Num() > 0);

		// E resta un sottoinsieme di ciascun lato: e' la meta' che esclude l'unione.
		TestTrue(FString::Printf(TEXT("'%s' E '%s': %d non supera i %d del solo primo"),
			*Pair.Key, *Pair.Value, Both.Num(), IdsByTag[Pair.Key].Num()),
			Both.Num() <= IdsByTag[Pair.Key].Num());
		TestTrue(FString::Printf(TEXT("'%s' E '%s': %d non supera i %d del solo secondo"),
			*Pair.Key, *Pair.Value, Both.Num(), IdsByTag[Pair.Value].Num()),
			Both.Num() <= IdsByTag[Pair.Value].Num());
	}

	if (bFoundDisjoint)
	{
		// Due tag che non stanno mai insieme: l'intersezione e' vuota, l'unione avrebbe entrambi i lati.
		const TArray<FString> Both = URTScenarioIndex::ListIds(Disjoint.Key, Disjoint.Value);
		TestEqual(FString::Printf(TEXT("'%s' E '%s' non stanno su nessuno scenario: l'elenco e' vuoto"),
			*Disjoint.Key, *Disjoint.Value), Both.Num(), 0);
	}

	// Filtri vuoti = nessuna restrizione: il contratto di `ListIds`, e la ragione per cui «cercare a filtri
	// vuoti cerca su tutti» non e' un caso speciale nel pannello.
	const TArray<FString> All = URTScenarioIndex::ListIds(FString(), FString());
	TestEqual(TEXT("due filtri vuoti danno tutti gli scenari indicizzati"), All.Num(), Entries.Num());

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
 * L'elenco vuoto dice QUALE causa lo ha svuotato — e le cause sono tre, non due.
 *
 * ⚠️ I due casi che contano sono quelli in cui il conteggio non basta a distinguere:
 * - filtri che non lasciano passare niente **mentre c'e' del testo nella casella**: attribuirlo alla
 *   ricerca (l'errore naturale, perche' la casella e' piena) manda a cancellare la parola sbagliata;
 * - indice vuoto **a filtri aperti**: dire «allarga i filtri» manda a cercare una via d'uscita che non
 *   esiste, perche' non c'e' niente da allargare e la causa e' fuori dal pannello.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherEmptyStateNamesItsCauseTest,
	"RefactorTactics.DevSandboxLauncher.EmptyListNamesItsCause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherEmptyStateNamesItsCauseTest::RunTest(const FString&)
{
	// `TestTrue` con un confronto esplicito e non `TestEqual`: l'enum non ha una conversione a stringa, e
	// il messaggio di fallimento di `TestEqual` la vorrebbe.
	TestTrue(TEXT("con delle voci visibili non c'e' stato vuoto"),
		FRTLauncherScenarioBrowser::Classify(12, 3, true) == ERTLauncherListState::Populated);

	TestTrue(TEXT("i tag scelti non lasciano passare niente"),
		FRTLauncherScenarioBrowser::Classify(0, 0, true) == ERTLauncherListState::NoTagMatches);

	TestTrue(TEXT("i tag lasciavano passare, la ricerca ha azzerato"),
		FRTLauncherScenarioBrowser::Classify(12, 0, true) == ERTLauncherListState::NoSearchMatches);

	// Lo stesso conteggio del caso qui sopra, con l'unica differenza che nessun filtro sta restringendo:
	// senza il terzo dato le due situazioni sarebbero indistinguibili, e il pannello darebbe la colpa a
	// dei filtri che non ci sono.
	TestTrue(TEXT("indice vuoto a filtri aperti non e' colpa dei filtri"),
		FRTLauncherScenarioBrowser::Classify(0, 0, false) == ERTLauncherListState::EmptyCorpus);

	// I messaggi non possono essere lo stesso testo: se lo fossero, la distinzione esisterebbe nell'enum e
	// non sullo schermo, che e' l'unico posto dove serve.
	const FText NoTags = FRTLauncherScenarioBrowser::DescribeEmptyState(ERTLauncherListState::NoTagMatches);
	const FText NoSearch = FRTLauncherScenarioBrowser::DescribeEmptyState(ERTLauncherListState::NoSearchMatches);
	const FText NoCorpus = FRTLauncherScenarioBrowser::DescribeEmptyState(ERTLauncherListState::EmptyCorpus);

	TestFalse(TEXT("il messaggio dei tag non e' vuoto"), NoTags.IsEmpty());
	TestFalse(TEXT("il messaggio della ricerca non e' vuoto"), NoSearch.IsEmpty());
	TestFalse(TEXT("il messaggio del corpus vuoto non e' vuoto"), NoCorpus.IsEmpty());

	TestFalse(TEXT("tag e ricerca sono distinguibili"), NoTags.EqualTo(NoSearch));
	TestFalse(TEXT("tag e corpus vuoto sono distinguibili"), NoTags.EqualTo(NoCorpus));
	TestFalse(TEXT("ricerca e corpus vuoto sono distinguibili"), NoSearch.EqualTo(NoCorpus));

	TestTrue(TEXT("un elenco pieno non ha messaggio da mostrare"),
		FRTLauncherScenarioBrowser::DescribeEmptyState(ERTLauncherListState::Populated).IsEmpty());

	return true;
}

/**
 * Il terreno come lo scenario lo dichiara, nei due modi in cui il corpus lo dichiara davvero.
 *
 * Misurato il 2026-08-30 su questo branch: **90** scenari, **21** con una fixture, **69** con un raggio,
 * nessuno con entrambi e nessuno con nessuno dei due. I numeri sono contati qui e non ripresi dal corpo
 * della issue, dove erano fermi a 88.
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

	// `varianti` non compare quando sono zero: una riga costante su quasi tutti gli scenari allontana
	// dall'occhio le righe che cambiano.
	for (const FString& Line : Lines)
	{
		TestFalse(TEXT("nessuna riga 'varianti' quando non ce ne sono"), Line.Contains(TEXT("varianti")));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
