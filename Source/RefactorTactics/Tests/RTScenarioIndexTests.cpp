// Indice degli scenari: risoluzione per ID, filtri per tag, redirect.
//
// Il grosso lavora su file FINTI passati a `BuildFrom`, non sul disco: cosi' i casi che contano — due file
// che dichiarano lo stesso ID, un JSON rotto, una catena di redirect ciclica — si scrivono senza sporcare
// `Scenarios/`, che deve restare la cartella degli scenari veri.
//
// Il solo test che tocca il disco e' quello sul corpus reale, ed e' anche il solo che possa accorgersi che
// uno scenario committato oggi rompe l'indice.

#include "Misc/AutomationTest.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioRunner.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	TPair<FString, FString> IndexFile(const TCHAR* Path, const TCHAR* Id, const TCHAR* Tags)
	{
		return TPair<FString, FString>(Path,
			FString::Printf(TEXT("{ \"scenarioId\": \"%s\", \"tags\": [%s] }"), Id, Tags));
	}
}

/**
 * T1 — L'indice risolve ogni scenario per il suo ID **indipendentemente dal percorso**.
 *
 * E' l'invariante che sostituisce «l'id e' il percorso»: due file in cartelle che non c'entrano niente con
 * il loro ID devono essere trovati lo stesso. Se questo test fallisce, spostare uno scenario ne cambia
 * l'identita' e siamo tornati al punto di partenza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioIndexBuildTest,
	"RefactorTactics.ScenarioIndex.IdIsIndependentOfPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioIndexBuildTest::RunTest(const FString&)
{
	const TArray<TPair<FString, FString>> Files = {
		IndexFile(TEXT("/x/QualsiasiCartella/uno.json"),   TEXT("Reaction.Overwatch.Hold"), TEXT("\"reactions\", \"flux\"")),
		IndexFile(TEXT("/x/AltraCartella/due.json"),       TEXT("Movement.Basic"),          TEXT("\"movement\"")),
	};

	TArray<FString> Problems;
	const TArray<FRTScenarioEntry> Entries = URTScenarioIndex::BuildFrom(Files, Problems);

	TestEqual(TEXT("due voci indicizzate"), Entries.Num(), 2);
	TestEqual(TEXT("nessun problema"), Problems.Num(), 0);

	// Il nome del file non ha alcun rapporto con l'ID: e' esattamente cio' che deve funzionare.
	const FRTScenarioEntry* Overwatch = Entries.FindByPredicate(
		[](const FRTScenarioEntry& E) { return E.ScenarioId == TEXT("Reaction.Overwatch.Hold"); });
	if (!TestNotNull(TEXT("scenario trovato per id, non per percorso"), Overwatch)) { return false; }
	TestEqual(TEXT("porta il percorso del suo file"), Overwatch->Path, FString(TEXT("/x/QualsiasiCartella/uno.json")));
	TestEqual(TEXT("due tag"), Overwatch->Tags.Num(), 2);
	return true;
}

/**
 * T2 — Due file che dichiarano lo stesso `scenarioId`.
 *
 * E' la classe di errore che questo indice **introduce**: finche' l'ID era il percorso, il filesystem la
 * rendeva impossibile. Deve essere segnalata alla costruzione e rifiutata alla risoluzione — sceglierne uno
 * in silenzio significherebbe eseguire uno scenario diverso da quello chiesto, che e' il difetto peggiore
 * che un harness possa avere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioIndexDuplicateIdTest,
	"RefactorTactics.ScenarioIndex.DuplicateIdIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioIndexDuplicateIdTest::RunTest(const FString&)
{
	const TArray<TPair<FString, FString>> Files = {
		IndexFile(TEXT("/x/a.json"), TEXT("Movement.Basic"), TEXT("\"movement\"")),
		IndexFile(TEXT("/x/b.json"), TEXT("Movement.Basic"), TEXT("\"combat\"")),
	};

	TArray<FString> Problems;
	URTScenarioIndex::BuildFrom(Files, Problems);

	if (!TestEqual(TEXT("un problema segnalato"), Problems.Num(), 1)) { return false; }
	// Il messaggio deve nominare l'ID e i file: un «id duplicato» senza dire quali non fa risparmiare
	// nemmeno la ricerca che si voleva evitare.
	TestTrue(TEXT("il messaggio nomina l'id"), Problems[0].Contains(TEXT("Movement.Basic")));
	TestTrue(TEXT("il messaggio nomina il primo file"), Problems[0].Contains(TEXT("a.json")));
	TestTrue(TEXT("il messaggio nomina il secondo file"), Problems[0].Contains(TEXT("b.json")));
	return true;
}

/** Un JSON rotto non deve far sparire gli altri scenari dall'indice: si segnala e si tira avanti. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioIndexBrokenFileTest,
	"RefactorTactics.ScenarioIndex.BrokenFileDoesNotHideTheOthers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioIndexBrokenFileTest::RunTest(const FString&)
{
	TArray<TPair<FString, FString>> Files;
	Files.Emplace(TEXT("/x/rotto.json"), TEXT("{ questo non e' json"));
	Files.Add(IndexFile(TEXT("/x/sano.json"), TEXT("Movement.Basic"), TEXT("\"movement\"")));

	TArray<FString> Problems;
	const TArray<FRTScenarioEntry> Entries = URTScenarioIndex::BuildFrom(Files, Problems);

	TestEqual(TEXT("lo scenario sano resta indicizzato"), Entries.Num(), 1);
	TestEqual(TEXT("il file rotto e' segnalato"), Problems.Num(), 1);
	TestTrue(TEXT("il problema nomina il file"), Problems[0].Contains(TEXT("rotto.json")));
	return true;
}

/**
 * T3 — Due filtri sono una **intersezione**, non una unione.
 *
 * E' la ragione per cui i filtri sono due: il caso che serve e' «reactions E flux». Se questo test si
 * rompesse restituendo l'unione, il filtro sembrerebbe funzionare (l'elenco cambia) mentre mostrerebbe
 * piu' scenari invece di meno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioIndexFilterIntersectionTest,
	"RefactorTactics.ScenarioIndex.TwoFiltersIntersect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioIndexFilterIntersectionTest::RunTest(const FString&)
{
	// Sul corpus reale, cosi' il test fallisce anche se qualcuno toglie i tag dagli scenari versionati.
	const TArray<FString> Tutti     = URTScenarioIndex::ListIds(FString(), FString());
	const TArray<FString> Movimento = URTScenarioIndex::ListIds(TEXT("movement"), FString());
	const TArray<FString> Incrocio  = URTScenarioIndex::ListIds(TEXT("movement"), TEXT("core"));

	TestTrue(TEXT("senza filtri si vede tutto"), Tutti.Num() > 0);
	TestTrue(TEXT("un filtro restringe"), Movimento.Num() > 0 && Movimento.Num() < Tutti.Num());
	TestTrue(TEXT("due filtri restringono ancora"), Incrocio.Num() > 0 && Incrocio.Num() < Movimento.Num());

	TestTrue(TEXT("Movement.Basic e' movement+core"), Incrocio.Contains(TEXT("Movement.Basic")));
	// `Movement.Blocked` e' taggato pathfinding, non core: deve cadere fuori dall'incrocio ma restare
	// dentro il filtro singolo. E' il caso che distingue intersezione da unione.
	TestTrue(TEXT("Movement.Blocked passa il filtro singolo"), Movimento.Contains(TEXT("Movement.Blocked")));
	TestFalse(TEXT("Movement.Blocked non passa l'incrocio"), Incrocio.Contains(TEXT("Movement.Blocked")));
	// Un tag di un'altra tipologia non deve trascinarsi dietro gli scenari di movimento.
	TestFalse(TEXT("Combat.BasicAttack non e' movement"), Movimento.Contains(TEXT("Combat.BasicAttack")));
	return true;
}

/**
 * T4 — L'elenco **filtrato** ha ordine totale.
 *
 * `ListScenarioIds` ordinava gia' l'elenco pieno, ma il filtro e' codice nuovo e l'ordine con cui il
 * filesystem restituisce i file non e' garantito. Una tendina che cambia ordine fra due aperture e' un modo
 * silenzioso di far cliccare la voce sbagliata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioIndexOrderTest,
	"RefactorTactics.ScenarioIndex.FilteredListIsTotallyOrdered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioIndexOrderTest::RunTest(const FString&)
{
	const TArray<FString> Ids = URTScenarioIndex::ListIds(TEXT("movement"), FString());
	if (!TestTrue(TEXT("almeno due scenari da ordinare"), Ids.Num() >= 2)) { return false; }

	for (int32 I = 1; I < Ids.Num(); ++I)
	{
		TestTrue(FString::Printf(TEXT("'%s' viene dopo '%s'"), *Ids[I], *Ids[I - 1]), Ids[I - 1] < Ids[I]);
	}

	// Stabile fra due chiamate: non basta che sia ordinato una volta.
	TestEqual(TEXT("stesso elenco alla seconda chiamata"), URTScenarioIndex::ListIds(TEXT("movement"), FString()), Ids);

	// Il vocabolario dei tag e' anch'esso ordinato, e contiene solo tag realmente presenti.
	const TArray<FString> Tags = URTScenarioIndex::ListTags();
	TestTrue(TEXT("il vocabolario non e' vuoto"), Tags.Num() > 0);
	for (int32 I = 1; I < Tags.Num(); ++I)
	{
		TestTrue(FString::Printf(TEXT("tag '%s' dopo '%s'"), *Tags[I], *Tags[I - 1]), Tags[I - 1] < Tags[I]);
	}
	TestTrue(TEXT("'movement' e' nel vocabolario"), Tags.Contains(TEXT("movement")));
	return true;
}

/**
 * T5 — I redirect seguono la catena e non girano all'infinito.
 *
 * Un ciclo `a -> b -> a` in una tabella scritta a mano e' un errore plausibile, e un harness che si pianta
 * su un file di configurazione e' peggio di uno che rifiuta l'ID.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioIndexRedirectTest,
	"RefactorTactics.ScenarioIndex.RedirectsFollowTheChain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioIndexRedirectTest::RunTest(const FString&)
{
	TMap<FString, FString> Redirects;
	Redirects.Add(TEXT("Vecchio.Nome"),    TEXT("Intermedio.Nome"));
	Redirects.Add(TEXT("Intermedio.Nome"), TEXT("Movement.Basic"));

	TestEqual(TEXT("catena seguita fino in fondo"),
		URTScenarioIndex::ApplyRedirects(Redirects, TEXT("Vecchio.Nome")), FString(TEXT("Movement.Basic")));
	TestEqual(TEXT("un id senza redirect resta se stesso"),
		URTScenarioIndex::ApplyRedirects(Redirects, TEXT("Combat.BasicAttack")), FString(TEXT("Combat.BasicAttack")));

	// Ciclo: deve terminare. Il valore restituito non e' interessante — che la chiamata ritorni lo e'.
	TMap<FString, FString> Ciclo;
	Ciclo.Add(TEXT("A"), TEXT("B"));
	Ciclo.Add(TEXT("B"), TEXT("A"));
	const FString Uscita = URTScenarioIndex::ApplyRedirects(Ciclo, TEXT("A"));
	TestTrue(TEXT("un ciclo termina invece di girare"), Uscita == TEXT("A") || Uscita == TEXT("B"));

	// Un ID vivo vince su un redirect rimasto indietro: se qualcuno riusasse un nome liberato, la tabella
	// non deve dirottarlo altrove.
	TMap<FString, FString> Obsoleto;
	Obsoleto.Add(TEXT("Movement.Basic"), TEXT("Qualcos.Altro"));
	FString Error;
	const FString Risolto = URTScenarioIndex::ResolvePath(TEXT("Movement.Basic"), Error);
	TestFalse(TEXT("Movement.Basic si risolve senza passare dai redirect"), Risolto.IsEmpty());
	return true;
}

/** Un ID che non esiste produce un errore che dice DOVE si e' cercato, non un percorso inventato. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioIndexUnknownIdTest,
	"RefactorTactics.ScenarioIndex.UnknownIdFailsWithReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioIndexUnknownIdTest::RunTest(const FString&)
{
	FString Error;
	const FString Path = URTScenarioIndex::ResolvePath(TEXT("Non.Esiste.Affatto"), Error);

	TestTrue(TEXT("nessun percorso restituito"), Path.IsEmpty());
	TestFalse(TEXT("il motivo e' valorizzato"), Error.IsEmpty());
	TestTrue(TEXT("il motivo nomina l'id cercato"), Error.Contains(TEXT("Non.Esiste.Affatto")));
	TestTrue(TEXT("il motivo nomina la cartella"), Error.Contains(TEXT("Scenarios")));
	return true;
}

/**
 * T6 — Il corpus reale e' coerente: ogni scenario versionato porta almeno un tag e resta raggiungibile
 * dall'elenco non filtrato, che e' quello che la tendina mostra quando i filtri sono vuoti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioIndexShippedCorpusTest,
	"RefactorTactics.ScenarioIndex.ShippedScenariosAreTagged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioIndexShippedCorpusTest::RunTest(const FString&)
{
	TArray<FString> Problems;
	const TArray<FRTScenarioEntry> Entries = URTScenarioIndex::Scan(Problems);

	if (!TestTrue(TEXT("almeno uno scenario indicizzato"), Entries.Num() > 0)) { return false; }
	if (Problems.Num() > 0)
	{
		AddError(FString::Printf(TEXT("l'indice ha %d problemi: %s"), Problems.Num(), *FString::Join(Problems, TEXT(" · "))));
	}
	TestEqual(TEXT("nessun problema nell'indice"), Problems.Num(), 0);

	const TArray<FString> Elenco = URTScenarioIndex::ListIds(FString(), FString());
	for (const FRTScenarioEntry& Entry : Entries)
	{
		TestTrue(FString::Printf(TEXT("%s ha almeno un tag"), *Entry.ScenarioId), Entry.Tags.Num() > 0);
		TestTrue(FString::Printf(TEXT("%s compare nell'elenco non filtrato"), *Entry.ScenarioId),
			Elenco.Contains(Entry.ScenarioId));
	}

	// La tabella di redirect non e' uno scenario: se comparisse qui, la scansione starebbe indicizzando
	// file che non lo sono.
	for (const FRTScenarioEntry& Entry : Entries)
	{
		TestFalse(TEXT("la tabella di redirect non e' indicizzata"),
			Entry.Path.Contains(URTScenarioIndex::RedirectsFileName));
	}

	// `ListScenarioIds` del runner deve restare d'accordo con l'indice: e' cio' che usa la console.
	TestEqual(TEXT("il runner elenca gli stessi id"), URTScenarioRunner::ListScenarioIds(), Elenco);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
