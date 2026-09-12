#include "Misc/AutomationTest.h"

#include "RTLauncherScenarioBrowser.h"
#include "ScenarioHarness/RTScenarioDraft.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioAuthoring.h"
#include "Turn/RTTurnRules.h" // ERTMatchPhase
#include "UObject/Package.h"       // GetTransientPackage
#include "UObject/StrongObjectPtr.h"

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

/**
 * Il conio dell'`UnitId` (#2786).
 *
 * ⚠️ **Perche' e' una funzione e non due righe nel pannello.** `AddUnit` prende l'id in INGRESSO (#1115):
 * il conio e' del chiamante, e se sbaglia il difetto si manifesta come `Invalid` con «id gia' preso» —
 * un messaggio che accusa lo SCENARIO per un errore della SCHERMATA. Tenendolo qui, cio' che puo'
 * sbagliare ha un test che gira senza un editor vivo.
 *
 * ⛔ **Cosa questo test NON copre**: che la tendina si popoli, che il pulsante sia abilitato e che il
 * readout si aggiorni sono Slate su un editor vivo — voce `PIE-SCEN-COMPOSER`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherCoinUnitIdTest,
	"RefactorTactics.DevSandboxLauncher.CoinUnitIdAvoidsCollisions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherCoinUnitIdTest::RunTest(const FString&)
{
	auto Unita = [](const FString& Id)
	{
		FRTScenarioUnitView View;
		View.Id = Id;
		return View;
	};

	// Scenario vuoto: il primo id e' `U1`. Non `U0` — gli id sono nomi, non indici, e partire da zero
	// inviterebbe a leggerli come posizioni nell'array.
	TestEqual(TEXT("senza unita' schierate il conio da' U1"),
		FRTLauncherScenarioBrowser::CoinUnitId({}), FString(TEXT("U1")));

	// 🔑 **L'invariante che il pannello compra con questa funzione**: due gesti di fila non collidono. Il
	// secondo conio vede la lista DOPO il primo `AddUnit`, ed e' per questo che il pannello la rilegge
	// dalla facade gia' aperta invece di usarne una fotografia precedente.
	const TArray<FRTScenarioUnitView> Uno = { Unita(TEXT("U1")) };
	TestEqual(TEXT("con U1 schierata il conio da' U2"),
		FRTLauncherScenarioBrowser::CoinUnitId(Uno), FString(TEXT("U2")));

	// I buchi si riempiono, ed e' dichiarato: rimossa `U2`, la prossima torna `U2`. L'alternativa —
	// un contatore monotono — richiederebbe uno stato che sopravviva a salva/riapri, e senza di quello
	// ripartirebbe da capo producendo proprio la collisione che il conio evita.
	const TArray<FRTScenarioUnitView> ConBuco = { Unita(TEXT("U1")), Unita(TEXT("U3")) };
	TestEqual(TEXT("il conio riempie il buco lasciato da una rimozione"),
		FRTLauncherScenarioBrowser::CoinUnitId(ConBuco), FString(TEXT("U2")));

	// ⚠️ **Case-insensitive, e non e' pedanteria**: il formato non impone una capitalizzazione agli id, e
	// uno scenario scritto a mano puo' portare `u1`. Un confronto sensibile alle maiuscole lo dichiarerebbe
	// libero e produrrebbe la collisione — cioe' il caso peggiore, perche' passa il conio e fallisce la
	// facade.
	const TArray<FRTScenarioUnitView> Minuscolo = { Unita(TEXT("u1")) };
	TestEqual(TEXT("u1 minuscola occupa U1"),
		FRTLauncherScenarioBrowser::CoinUnitId(Minuscolo), FString(TEXT("U2")));

	// Id fuori convenzione: il conio non prova a indovinarne la forma, cerca il primo `U<n>` libero. Uno
	// scenario del corpus puo' chiamare le unita' `attaccante` o `hero_a`, e nessuna collide con `U1`.
	const TArray<FRTScenarioUnitView> FuoriConvenzione = { Unita(TEXT("attaccante")), Unita(TEXT("hero_a")) };
	TestEqual(TEXT("gli id fuori convenzione non spostano il conio"),
		FRTLauncherScenarioBrowser::CoinUnitId(FuoriConvenzione), FString(TEXT("U1")));

	// La piccionaia: con N id presi, fra `U1` e `U(N+1)` almeno uno e' libero. Qui tutti gli `U1..U3` sono
	// presi, quindi l'esito deve essere `U4` e non una stringa vuota.
	const TArray<FRTScenarioUnitView> Pieno = { Unita(TEXT("U1")), Unita(TEXT("U2")), Unita(TEXT("U3")) };
	TestEqual(TEXT("con U1..U3 prese il conio da' U4"),
		FRTLauncherScenarioBrowser::CoinUnitId(Pieno), FString(TEXT("U4")));

	// 🔴 **Il conio non restituisce mai un id gia' preso.** E' l'asserzione che vale piu' delle singole
	// uguaglianze: le altre fissano la forma, questa fissa la proprieta'.
	for (const TArray<FRTScenarioUnitView>& Caso : { Uno, ConBuco, Minuscolo, FuoriConvenzione, Pieno })
	{
		const FString Coniato = FRTLauncherScenarioBrowser::CoinUnitId(Caso);
		TestFalse(TEXT("l'id coniato non e' vuoto"), Coniato.IsEmpty());
		for (const FRTScenarioUnitView& Presente : Caso)
		{
			TestFalse(FString::Printf(TEXT("'%s' non collide con '%s'"), *Coniato, *Presente.Id),
				Coniato.Equals(Presente.Id, ESearchCase::IgnoreCase));
		}
	}

	return true;
}

/**
 * Le quattro posizioni del playback si leggono diverse — e `Ended` non si legge come l'inizio (#2788).
 *
 * 🔑 **Questo test non esisteva, e il commento nella funzione diceva perche'**: la frase viveva in un
 * anonimo dentro il pannello Slate, dove nessun automation test la raggiungeva. Il difetto che quel
 * commento registra — *«Posa iniziale» anche a partita finita* — lo trovo' una seduta in Editor il
 * 2026-09-04. Spostata la funzione, la stessa domanda costa una chiamata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherPlaybackPositionNamesItselfTest,
	"RefactorTactics.DevSandboxLauncher.PlaybackPositionNamesItself",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherPlaybackPositionNamesItselfTest::RunTest(const FString&)
{
	FRTReplayPosition Inizio;
	Inizio.State = ERTReplayPositionState::BeforeStart;

	FRTReplayPosition Fine;
	Fine.State = ERTReplayPositionState::Ended;

	FRTReplayPosition Dentro;
	Dentro.State = ERTReplayPositionState::AtPhase;
	Dentro.TurnNumber = 3;
	Dentro.Phase = ERTMatchPhase::Move;

	FRTReplayPosition Cieca;
	Cieca.State = ERTReplayPositionState::Unaddressable;
	Cieca.Phase = ERTMatchPhase::Move;

	const FText DaInizio = FRTLauncherScenarioBrowser::DescribePlaybackPosition(Inizio);
	const FText DaFine = FRTLauncherScenarioBrowser::DescribePlaybackPosition(Fine);
	const FText DaDentro = FRTLauncherScenarioBrowser::DescribePlaybackPosition(Dentro);
	const FText DaCieca = FRTLauncherScenarioBrowser::DescribePlaybackPosition(Cieca);

	// ⚠️ **`HasTurn()` e' falso sia prima dell'inizio sia alla fine**, ed e' esattamente l'errore che la
	// prima stesura fece: guardare solo quello e scrivere «Posa iniziale» a partita finita.
	TestFalse(TEXT("l'inizio e la fine non si leggono uguali"), DaInizio.EqualTo(DaFine));

	// `Unaddressable` porta una fase e nessun turno: leggerlo come l'inizio manderebbe a premere `>` da un
	// punto che non e' quello in cui si e'.
	TestFalse(TEXT("l'inizio e una posizione non indirizzabile non si leggono uguali"),
		DaInizio.EqualTo(DaCieca));
	TestFalse(TEXT("la fine e una posizione non indirizzabile non si leggono uguali"),
		DaFine.EqualTo(DaCieca));

	// Il turno compare per davvero: una frase che lo ignorasse passerebbe tutti i confronti qui sopra.
	TestTrue(TEXT("dentro un turno la riga porta il numero del turno"),
		DaDentro.ToString().Contains(TEXT("3")));

	// ⛔ E non lo porta dove non vale: `TurnNumber` e' `0` per default negli altri tre stati, e stamparlo
	// direbbe «turno 0» come se fosse un istante della partita.
	TestFalse(TEXT("prima dell'inizio non si annuncia nessun turno"),
		DaInizio.ToString().Contains(TEXT("Turno")));

	// 🔴 **Le disuguaglianze qui sopra NON bloccano il difetto storico** (#2836), ed e' la segnalazione che
	// #2832 lascia aperta. Una stesura che scrivesse *«Posa iniziale.»* alla fine e, poniamo, *«Inizio.»*
	// all'inizio passerebbe tutti i confronti: sono diversi. Il sintomo esatto era che la fine **si leggeva
	// come l'inizio**, e il solo modo di escluderlo e' un'asserzione POSITIVA — che il testo nomini la fine.
	TestTrue(FString::Printf(TEXT("la fine si NOMINA, non e' solo diversa dall'inizio: %s"), *DaFine.ToString()),
		DaFine.ToString().Contains(TEXT("Fine")));

	// E la posa iniziale nomina se stessa, per la stessa ragione e dall'altro capo.
	TestTrue(FString::Printf(TEXT("l'inizio si nomina: %s"), *DaInizio.ToString()),
		DaInizio.ToString().Contains(TEXT("iniziale")));

	return true;
}

/**
 * `Esegui` non e' piu' muto: la riga di trasporto dice **cosa** sta mostrando, non solo dove si e' (#2788).
 *
 * 🔴 **Il difetto misurato, e perche' nessun test poteva vederlo prima.** Seduta in Editor del
 * 2026-09-09, pilotata via MCP: su `AutoBattle.ArenaV01` il click su `Esegui` non cambiava una sola parola
 * sullo schermo, e il pulsante fu classificato non funzionante per due tentativi consecutivi. La riga
 * rispondeva alla domanda «dove sono nella traccia», che a playback appena aperto e' sempre la stessa
 * risposta; la domanda che il designer stava facendo era «e' successo qualcosa». Le tre condizioni che
 * collassavano sono i tre casi qui sotto.
 *
 * ⚠️ **Questo test copre la traduzione, non il gesto.** Che il click chiami la facade, che il playback si
 * apra e che la riga si ridisegni sono Slate su un editor vivo: restano voce di seduta
 * **`PIE-SCEN-PLAYBACK`** (seduta `U44`), e nessuna asserzione qui li tocca.
 *
 * ⛔ **Non `PIE-SCEN-COMPOSER`, che e' cio' che questa riga diceva** (#2836). Quella voce verifica il
 * piazzamento delle unita' — *«piazza almeno due unita', salva, riapri»* — non il trasporto. L'equivoco era
 * gia' corretto nella descrizione di #2832 e lasciato nel SORGENTE, che e' l'unico dei due che qualcuno
 * legge mentre esegue il gate: chi apriva la seduta da questo commento verificava il pannello sbagliato, e
 * il trasporto restava `NOT RUN` credendo di essere coperto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherTransportNamesTheRunTest,
	"RefactorTactics.DevSandboxLauncher.TransportNamesTheRun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherTransportNamesTheRunTest::RunTest(const FString&)
{
	FRTReplayPosition Inizio;
	Inizio.State = ERTReplayPositionState::BeforeStart;

	// 1. Mai eseguito: nessun playback aperto, e la riga invita a eseguire.
	//
	// ⚠️ `bScenarioSelected` esplicito in OGNI stato di questo test, e il default e' `false` (#2836): uno
	// stato che non dichiara la selezione descrive un pannello senza scenario a schermo, dove la riga non
	// deve parlare di corse. Qui lo scenario c'e' sempre — cio' che varia e' cosa `Esegui` ha prodotto.
	FRTLauncherTransportStatus MaiEseguito;
	MaiEseguito.bScenarioSelected = true;

	// 2. Eseguito senza turni: la corsa e' avvenuta e non ha aperto niente, perche' non c'era niente da
	//    aprire. E' l'esito legittimo che si leggeva come un pulsante rotto.
	FRTLauncherTransportStatus SenzaTurni;
	SenzaTurni.bScenarioSelected = true;
	SenzaTurni.Run = ERTLauncherRunState::Ran;
	SenzaTurni.TurnsPlayed = 0;

	// 3. Eseguito con turni: il playback e' aperto sul turno 0 della TRACCIA — la stessa posizione che,
	//    da sola, si legge «Posa iniziale».
	FRTLauncherTransportStatus ConTraccia;
	ConTraccia.bScenarioSelected = true;
	ConTraccia.bHasTrace = true;
	ConTraccia.Run = ERTLauncherRunState::Ran;
	ConTraccia.TurnsPlayed = 22;
	ConTraccia.bPlaybackOpen = true;
	ConTraccia.Position = Inizio;

	// 4. Corsa rifiutata dalla facade: nessun playback, e il readout porta il messaggio.
	FRTLauncherTransportStatus Fallita;
	Fallita.bScenarioSelected = true;
	Fallita.Run = ERTLauncherRunState::Failed;

	const FText Mai = FRTLauncherScenarioBrowser::DescribeTransport(MaiEseguito);
	const FText Vuota = FRTLauncherScenarioBrowser::DescribeTransport(SenzaTurni);
	const FText Traccia = FRTLauncherScenarioBrowser::DescribeTransport(ConTraccia);
	const FText Rifiutata = FRTLauncherScenarioBrowser::DescribeTransport(Fallita);

	// --- criterio 1: la corsa senza turni non si legge come «non hai ancora eseguito» ------------------
	TestFalse(TEXT("una corsa senza turni non si legge come un pulsante mai premuto"),
		Vuota.EqualTo(Mai));
	TestTrue(TEXT("e nomina l'assenza di turni invece di tacerla"),
		Vuota.ToString().Contains(TEXT("nessun turno")));

	// --- criterio 2: la traccia prodotta non si legge come la posa d'authoring ---------------------------
	//
	// 🎯 **I due confronti sono diversi e servono entrambi.** Il primo tiene la traccia distinta dallo stato
	// in cui il pannello si trova PRIMA del click; il secondo la tiene distinta dalla frase che la sola
	// posizione produce — cioe' da *«Posa iniziale.»*, che e' la parola per parola che ha ingannato il
	// lettore. Senza il secondo, una stesura che ignorasse la corsa e restituisse la sola posizione
	// passerebbe il primo.
	TestFalse(TEXT("la traccia aperta non si legge come lo stato precedente al click"),
		Traccia.EqualTo(Mai));
	TestFalse(TEXT("e non si legge come la sola posizione, che dice \"Posa iniziale\""),
		Traccia.EqualTo(FRTLauncherScenarioBrowser::DescribePlaybackPosition(Inizio)));

	// Il conteggio non e' decorativo: e' il dato che distingue una corsa lunga da una che non ha giocato
	// niente, e arriva dal referto del runner.
	TestTrue(TEXT("la riga porta i turni che la corsa ha giocato"),
		Traccia.ToString().Contains(TEXT("22")));

	// --- criterio 3: il rifiuto resta distinto da entrambi ----------------------------------------------
	TestFalse(TEXT("una corsa fallita non si legge come un pulsante mai premuto"),
		Rifiutata.EqualTo(Mai));
	TestFalse(TEXT("una corsa fallita non si legge come una corsa senza turni"),
		Rifiutata.EqualTo(Vuota));
	TestFalse(TEXT("una corsa fallita non si legge come una traccia aperta"),
		Rifiutata.EqualTo(Traccia));

	// --- il quinto caso: turni giocati, ma niente da riprodurre -----------------------------------------
	//
	// ⚠️ Non e' teorico: gli scenari a VARIANTI aggregano piu' corse e non assegnano ne' hash ne'
	// `TurnTraces` all'aggregato (`FRTScenarioRunReport::bHasTrace`). Fondere questo caso con «nessun
	// turno» direbbe che la partita non ha giocato niente, mentre ha giocato e non e' ispezionabile.
	FRTLauncherTransportStatus NonRiproducibile;
	NonRiproducibile.bScenarioSelected = true;
	NonRiproducibile.Run = ERTLauncherRunState::Ran;
	NonRiproducibile.TurnsPlayed = 7;

	// ⛔ **`bHasTrace` resta `false`, ed e' cio' che autorizza la frase** (#2836). Prima si deduceva da
	// `!bPlaybackOpen`, che e' vero anche per un'anteprima morta: il segnale autorevole e' il referto.

	const FText Muta = FRTLauncherScenarioBrowser::DescribeTransport(NonRiproducibile);
	TestFalse(TEXT("una traccia mancante non si legge come una corsa senza turni"), Muta.EqualTo(Vuota));
	TestTrue(TEXT("e dichiara i turni che sono stati giocati"), Muta.ToString().Contains(TEXT("7")));

	// --- il singolare, su `Movement.Basic`, che di turno ne ha esattamente uno --------------------------
	FRTLauncherTransportStatus UnTurno;
	UnTurno.bScenarioSelected = true;
	UnTurno.bHasTrace = true;
	UnTurno.Run = ERTLauncherRunState::Ran;
	UnTurno.TurnsPlayed = 1;
	UnTurno.bPlaybackOpen = true;
	UnTurno.Position = Inizio;

	const FString Singolare = FRTLauncherScenarioBrowser::DescribeTransport(UnTurno).ToString();
	TestTrue(TEXT("un turno solo si dice al singolare"), Singolare.Contains(TEXT("1 turno")));
	TestFalse(TEXT("e non al plurale"), Singolare.Contains(TEXT("1 turni")));

	// --- un playback aperto di cui il pannello non ha memoria -------------------------------------------
	//
	// ⛔ Nessun conteggio inventato: si dice dove si e', che e' l'unica cosa che si sa. Scrivere «corsa di 0
	// turni» su una traccia aperta — che quindi di turni ne ha — sarebbe un numero falso.
	FRTLauncherTransportStatus SenzaMemoria;
	SenzaMemoria.bScenarioSelected = true;
	SenzaMemoria.bPlaybackOpen = true;
	SenzaMemoria.Position = Inizio;

	TestTrue(TEXT("un playback senza corsa nota si descrive per posizione"),
		FRTLauncherScenarioBrowser::DescribeTransport(SenzaMemoria)
			.EqualTo(FRTLauncherScenarioBrowser::DescribePlaybackPosition(Inizio)));

	return true;
}

/**
 * Gli stati del trasporto sul CORPUS vero, e la distinzione che il criterio 4 di #2788 confonde.
 *
 * 🔴 **«turni» nel readout e «turni giocati» non sono lo stesso numero, ed e' la causa del difetto.**
 * La riga `turni` del readout viene da `FRTScenarioSummary::TurnCount`, cioe' dai turni **authorati** nel
 * file; quelli che la corsa gioca stanno in `FRTScenarioRunReport::TurnsPlayed`. Su `AutoBattle.ArenaV01` i
 * due divergono al massimo: il file ne dichiara **0** ed e' `freeRun`, quindi la partita ne gioca decine.
 * Chi ha misurato la seduta ha letto «turni 0» e si e' aspettato che non succedesse niente — e il
 * pulsante, che invece stava eseguendo, e' sembrato rotto.
 *
 * 🎯 **Percio' la coppia che verifica davvero il criterio non e' quella scritta nella issue.** Uno scenario
 * la cui CORSA non gioca turni e' uno con `turns` vuoto e **senza** `freeRun`; misurati sul corpus il
 * 2026-09-10 con un `python -c` che apre ricorsivamente i `.json` di `Scenarios/`, sono `Spec.Hud.DockArmsTheSelectedAction`,
 * `Spec.Hud.MatchWithTwoAllies`, `Visual.Input.PcGym`, `Visual.Map.BlockVolumes` e
 * `Visual.Map.TwoLayersSameColumn`. Qui si usa l'ultimo.
 *
 * ⚠️ **Cosa questo test NON dimostra.** Non apre nessun playback — `OpenPlayback` pretende un'anteprima
 * viva nel viewport, che headless non c'e' — e non preme nessun pulsante. Dimostra i FATTI da cui lo stato
 * del pannello dipende: quanti turni la corsa gioca e se lascia una traccia da riprodurre. Che il click li
 * legga resta la seduta **`PIE-SCEN-PLAYBACK`** (`U44`), non `PIE-SCEN-COMPOSER` (#2836).
 *
 * ⚠️ **Questo test ESEGUE due partite vere, in un file che prima non ne eseguiva nessuna**, e vincola il
 * contenuto di due scenari che questo dominio non possiede: `Movement.Basic` deve giocare **1** turno e
 * `Visual.Map.TwoLayersSameColumn` **0**. La segnalazione e' confermata, e il baratto e' accettato con gli
 * occhi aperti: sono i due fatti da cui la riga di trasporto dipende, e misurarli su dati inventati non
 * direbbe niente sul corpus reale. ⛔ **Cio' che NON e' accettabile e' un rosso mal attribuito**, quindi i
 * messaggi di queste asserzioni nominano il corpus: se un giorno diventano rossi per una modifica di
 * scenario, si legge che e' cambiato il corpus e non che e' rotto il trasporto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherTransportMatchesTheCorpusTest,
	"RefactorTactics.DevSandboxLauncher.TransportMatchesTheCorpus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherTransportMatchesTheCorpusTest::RunTest(const FString&)
{
	// Apre, esegue, richiude. La facade e' la stessa che il pannello usa, e come li' il draft non resta
	// aperto: il ciclo e' quello, non una scorciatoia del test.
	auto Corri = [this](const TCHAR* Id, int32& OutTurniGiocati, int32& OutTracce) -> bool
	{
		TStrongObjectPtr<URTScenarioAuthoring> Facade(
			URTScenarioAuthoring::CreateScenarioDraft(GetTransientPackage()));

		if (!TestTrue(TEXT("la facade d'authoring si costruisce"), Facade.IsValid()))
		{
			return false;
		}

		// ⚠️ L'apertura su una riga a se': dentro `TestTrue` il messaggio e la condizione sono due argomenti,
		// e l'ordine in cui si valutano non e' specificato — la diagnostica stamperebbe la stringa d'errore
		// come era PRIMA della chiamata, cioe' vuota, proprio nel caso in cui serve.
		FString ApriErrore;
		const bool bAperto = Facade->OpenById(Id, ApriErrore) == ERTScenarioAuthoringResult::Success;
		if (!TestTrue(FString::Printf(TEXT("%s si apre: %s"), Id, *ApriErrore), bAperto))
		{
			Facade->Close();
			return false;
		}

		FRTScenarioRunReport Referto;
		FString CorsaErrore;
		const bool bCorsa = Facade->Run(Referto, CorsaErrore) == ERTScenarioAuthoringResult::Success;

		OutTurniGiocati = Referto.TurnsPlayed;
		OutTracce = Facade->GetLastRunTraces().Num();
		Facade->Close();

		return TestTrue(FString::Printf(TEXT("%s esegue: %s"), Id, *CorsaErrore), bCorsa);
	};

	// --- uno scenario a turni authorati: la corsa produce una traccia -----------------------------------
	int32 TurniBasic = -1;
	int32 TracceBasic = -1;
	if (Corri(TEXT("Movement.Basic"), TurniBasic, TracceBasic))
	{
		TestEqual(TEXT("CORPUS: Movement.Basic gioca il turno che dichiara (rosso qui = scenario cambiato)"),
			TurniBasic, 1);
		TestTrue(TEXT("CORPUS: e Movement.Basic lascia una traccia da riprodurre"), TracceBasic > 0);

		// Lo stato che il pannello costruisce da questi due fatti, con il playback aperto sulla posa.
		FRTReplayPosition Inizio;
		Inizio.State = ERTReplayPositionState::BeforeStart;

		FRTLauncherTransportStatus Stato;
		Stato.bScenarioSelected = true;
		Stato.bHasTrace = true;
		Stato.Run = ERTLauncherRunState::Ran;
		Stato.TurnsPlayed = TurniBasic;
		Stato.bPlaybackOpen = true;
		Stato.Position = Inizio;

		const FString Riga = FRTLauncherScenarioBrowser::DescribeTransport(Stato).ToString();
		TestTrue(FString::Printf(TEXT("la riga nomina la corsa e il suo turno: %s"), *Riga),
			Riga.Contains(TEXT("1 turno")));

		// 🎯 Il criterio 2, sullo scenario che l'ha rivelato: non si legge come la sola posizione.
		TestFalse(TEXT("e non si legge come la posa d'authoring"),
			Riga == FRTLauncherScenarioBrowser::DescribePlaybackPosition(Inizio).ToString());
	}

	// --- uno scenario la cui corsa NON gioca turni ------------------------------------------------------
	int32 TurniVuoto = -1;
	int32 TracceVuoto = -1;
	if (Corri(TEXT("Visual.Map.TwoLayersSameColumn"), TurniVuoto, TracceVuoto))
	{
		TestEqual(TEXT("CORPUS: Visual.Map.TwoLayersSameColumn non gioca turni (rosso qui = scenario cambiato)"),
			TurniVuoto, 0);

		// 🔑 **E' questa la ragione per cui il playback non si apriva, e la riga taceva.**
		// `URTScenarioPreviewSubsystem::OpenPlayback` rifiuta quando `GetLastRunTraces()` e' vuota, e il
		// pannello restava sulla frase che invita a eseguire — detta a chi aveva appena eseguito.
		TestEqual(TEXT("CORPUS: e non lascia nessuna traccia da riprodurre"), TracceVuoto, 0);

		FRTLauncherTransportStatus Stato;
		Stato.bScenarioSelected = true;
		Stato.Run = ERTLauncherRunState::Ran;
		Stato.TurnsPlayed = TurniVuoto;
		Stato.bPlaybackOpen = false;

		const FText Riga = FRTLauncherScenarioBrowser::DescribeTransport(Stato);
		TestTrue(TEXT("la riga nomina l'assenza di turni invece di tacere"),
			Riga.ToString().Contains(TEXT("nessun turno")));

		FRTLauncherTransportStatus MaiEseguito;
		MaiEseguito.bScenarioSelected = true;
		TestFalse(TEXT("e non si legge come un pulsante mai premuto"),
			Riga.EqualTo(FRTLauncherScenarioBrowser::DescribeTransport(MaiEseguito)));
	}

	// --- il numero che il readout mostra per `AutoBattle.ArenaV01` --------------------------------------
	//
	// ⛔ **Non si esegue qui.** La corsa di questo scenario e' gia' misurata da
	// `Scenario.FreeRun.ArenaV01ReachesAWinner`, che asserisce `PASS` e `TurnsPlayed` sotto il tetto di 40:
	// rieseguirla pagherebbe decine di turni di bot per una conclusione che quel test gia' porta. Qui serve
	// solo il numero che il READOUT mostra, e quello si legge dall'header senza correre.
	{
		TStrongObjectPtr<URTScenarioAuthoring> Facade(
			URTScenarioAuthoring::CreateScenarioDraft(GetTransientPackage()));

		FString ApriErrore;
		const bool bAperto = Facade.IsValid()
			&& Facade->OpenById(TEXT("AutoBattle.ArenaV01"), ApriErrore) == ERTScenarioAuthoringResult::Success;

		if (TestTrue(FString::Printf(TEXT("AutoBattle.ArenaV01 si apre: %s"), *ApriErrore), bAperto))
		{
			// 🔴 Il readout dice «turni 0», e la partita ne gioca decine: e' la divergenza che ha fatto
			// leggere come rotto un pulsante che funzionava.
			TestEqual(TEXT("CORPUS: il readout di ArenaV01 dichiara zero turni authorati"),
				Facade->GetSummary().TurnCount, 0);
			Facade->Close();
		}
	}

	return true;
}


/**
 * Il pannello legge l'ESITO del referto, non solo il fatto che una corsa sia avvenuta (#2836).
 *
 * 🔴 **Il difetto, e perche' la suite non lo vedeva.** `URTScenarioAuthoring::Run` restituisce `Success`
 * quando *«l'esecuzione e' avvenuta»* — lo dichiara `RTScenarioAuthoring.h` — e il pannello ne scriveva
 * `Ran` senza guardare `FRTScenarioRunReport::Outcome`. Il solo campo che leggeva era `TurnsPlayed`, e
 * `RTScenarioSession` definisce `Blocked` ed `Error` **proprio** sul caso a zero turni:
 *
 *     const bool bBlocked      = !BlockedBy.IsEmpty() && Result.TurnsPlayed == 0;
 *     const bool bNonHaGiocato = !ErroredBy.IsEmpty() && Result.TurnsPlayed == 0;
 *
 * ∴ uno scenario bloccato su una capability mancante entrava nel ramo di successo e la riga dichiarava
 * *«Corsa eseguita: nessun turno da riprodurre.»*. ⚠️ **Nessun test poteva accorgersene**: `ClassifyRun`
 * non esisteva e la traduzione avveniva per omissione dentro un `FReply` di Slate.
 *
 * ⛔ **`Fail` resta `Ran`, ed e' asserito qui insieme al resto.** Un `FAIL` e' un difetto del GIOCO su una
 * partita giocata, e travestirlo da guasto dello strumento e' lo stesso errore girato dall'altra parte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherRunStateReadsTheOutcomeTest,
	"RefactorTactics.DevSandboxLauncher.RunStateReadsTheOutcome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherRunStateReadsTheOutcomeTest::RunTest(const FString&)
{
	auto Referto = [](ERTTestOutcome Esito, int32 Turni) -> FRTScenarioRunReport
	{
		FRTScenarioRunReport R;
		R.Outcome = Esito;
		R.TurnsPlayed = Turni;
		R.bHasRun = true;
		return R;
	};

	// ⚠️ **Il nome e non il numero.** `TestEqual` sugli `enum class` non compila — il suo `ReportError`
	// generico chiama `Actual.ToString()` — e un `static_cast<int32>` compilerebbe stampando *«3 != 2»*,
	// che in un rosso non dice quale esito sia arrivato. Il nome lo dice.
	auto Nome = [](ERTLauncherRunState Stato) -> FString
	{
		switch (Stato)
		{
		case ERTLauncherRunState::NotRun:  return TEXT("NotRun");
		case ERTLauncherRunState::Failed:  return TEXT("Failed");
		case ERTLauncherRunState::Ran:     return TEXT("Ran");
		case ERTLauncherRunState::Blocked: return TEXT("Blocked");
		case ERTLauncherRunState::Errored: return TEXT("Errored");
		}
		// Uno stato aggiunto e non nominato: qui il rosso e' il messaggio.
		return TEXT("<stato non nominato da questo test>");
	};

	auto Tradotto = [&Nome](const FRTScenarioRunReport& R) -> FString
	{
		return Nome(FRTLauncherScenarioBrowser::ClassifyRun(R));
	};

	// --- la traduzione: quattro esiti, tre stati, e nessuno che diventi un successo per omissione -------
	TestEqual(TEXT("Pass e' una corsa avvenuta"),
		Tradotto(Referto(ERTTestOutcome::Pass, 3)), Nome(ERTLauncherRunState::Ran));

	TestEqual(TEXT("e Fail pure: la partita si e' giocata, e l'esito lo dice il referto"),
		Tradotto(Referto(ERTTestOutcome::Fail, 3)), Nome(ERTLauncherRunState::Ran));

	TestEqual(TEXT("Blocked NON e' una corsa riuscita"),
		Tradotto(Referto(ERTTestOutcome::Blocked, 0)), Nome(ERTLauncherRunState::Blocked));

	TestEqual(TEXT("ed Error nemmeno"),
		Tradotto(Referto(ERTTestOutcome::Error, 0)), Nome(ERTLauncherRunState::Errored));

	// --- il motivo arriva a destinazione ---------------------------------------------------------------
	//
	// 🔑 Senza, la riga dice «Corsa BLOCCATA» e non dice QUALE capability manchi — che e' l'unica cosa da
	// sapere per andare avanti, ed e' gia' nel referto.
	FRTScenarioRunReport Bloccato = Referto(ERTTestOutcome::Blocked, 0);
	Bloccato.BlockedReason = TEXT("scenario: manca la capability 'Overwatch'");
	TestTrue(TEXT("il motivo del blocco raggiunge il readout"),
		FRTLauncherScenarioBrowser::DescribeRunDetail(Bloccato).Contains(TEXT("Overwatch")));

	FRTScenarioRunReport Rotto = Referto(ERTTestOutcome::Error, 0);
	Rotto.ErrorMessage = TEXT("'U1' non possiede l'abilita' 'Blink' (turno 2)");
	TestTrue(TEXT("il messaggio d'errore raggiunge il readout"),
		FRTLauncherScenarioBrowser::DescribeRunDetail(Rotto).Contains(TEXT("Blink")));

	// ⛔ E i due non si leggono uguali: «il test e' scritto male» e «il progetto non c'e' ancora» sono la
	// distinzione per cui `ERTTestOutcome` ha quattro valori invece di due.
	TestNotEqual(TEXT("un blocco e un errore non si leggono uguali"),
		FRTLauncherScenarioBrowser::DescribeRunDetail(Bloccato),
		FRTLauncherScenarioBrowser::DescribeRunDetail(Rotto));

	// Una corsa riuscita non porta nessun motivo: una riga «referto: » vuota sotto ogni corsa sarebbe rumore.
	TestTrue(TEXT("Pass non porta nessun motivo da riportare"),
		FRTLauncherScenarioBrowser::DescribeRunDetail(Referto(ERTTestOutcome::Pass, 3)).IsEmpty());

	// ⚠️ Un referto che dichiara il motivo e non lo porta lo DICE, invece di restituire una stringa vuota
	// che a schermo diventa una riga assente: un referto incompleto e' un difetto, e va visto.
	TestFalse(TEXT("un blocco senza motivo non tace"),
		FRTLauncherScenarioBrowser::DescribeRunDetail(Referto(ERTTestOutcome::Blocked, 0)).IsEmpty());

	// --- e la FRASE che ne esce: e' qui che il difetto si vedeva ----------------------------------------
	auto Riga = [](ERTLauncherRunState Stato, int32 Turni) -> FString
	{
		FRTLauncherTransportStatus S;
		S.bScenarioSelected = true;
		S.Run = Stato;
		S.TurnsPlayed = Turni;
		return FRTLauncherScenarioBrowser::DescribeTransport(S).ToString();
	};

	const FString Eseguita = Riga(ERTLauncherRunState::Ran, 0);
	const FString Bloccata = Riga(ERTLauncherRunState::Blocked, 0);
	const FString Errata = Riga(ERTLauncherRunState::Errored, 0);

	// 🎯 **L'asserzione che chiude la issue.** A zero turni i tre casi collassavano su questa frase, e due
	// dei tre la rendevano falsa.
	TestFalse(FString::Printf(TEXT("una corsa BLOCCATA non si legge come una eseguita: %s"), *Bloccata),
		Bloccata == Eseguita);
	TestFalse(FString::Printf(TEXT("una corsa in ERRORE non si legge come una eseguita: %s"), *Errata),
		Errata == Eseguita);
	TestFalse(TEXT("e il blocco non si legge come l'errore"), Bloccata == Errata);

	// ⛔ Nessuna delle due afferma un esito che il referto non conferma: la parola «eseguita» non compare.
	TestFalse(FString::Printf(TEXT("la riga di un blocco non dichiara la corsa eseguita: %s"), *Bloccata),
		Bloccata.Contains(TEXT("eseguita")));
	TestFalse(FString::Printf(TEXT("ne' quella di un errore: %s"), *Errata),
		Errata.Contains(TEXT("eseguita")));

	// E si nominano, invece di essere solo diverse — la stessa asticella di `PlaybackPositionNamesItself`.
	TestTrue(FString::Printf(TEXT("il blocco si nomina: %s"), *Bloccata), Bloccata.Contains(TEXT("BLOCCATA")));
	TestTrue(FString::Printf(TEXT("l'errore si nomina: %s"), *Errata), Errata.Contains(TEXT("ERRORE")));

	// --- gli stessi due esiti con dei turni giocati -----------------------------------------------------
	//
	// ⚠️ Non sono solo il caso a zero turni: `Finish()` assegna `Error` anche a meta' partita (`ErroredBy`
	// ha la precedenza su tutto). La riga deve nominarli anche li', e dire quanti turni sono passati.
	const FString ErrataInCorsa = Riga(ERTLauncherRunState::Errored, 5);
	TestTrue(FString::Printf(TEXT("un errore a meta' partita porta i turni giocati: %s"), *ErrataInCorsa),
		ErrataInCorsa.Contains(TEXT("5")));
	TestTrue(TEXT("e si nomina lo stesso"), ErrataInCorsa.Contains(TEXT("ERRORE")));

	return true;
}

/**
 * La memoria della corsa non sopravvive alla deselezione (#2836).
 *
 * 🔴 **Riproduzione misurata**: seleziona `Visual.Map.TwoLayersSameColumn`, premi `Esegui` — che produce
 * `Ran` con `TurnsPlayed = 0` — poi clicca nell'area vuota sotto la lista. `ClearSelection()` azzerava
 * `SelectedId`, `ReadoutLines`, `ReadoutError`, `PlacedUnitOptions` e `SelectedUnitId`, e **non** la corsa:
 * l'unica sede che azzerava `LastRunState` era `RefreshReadout()`, che da li' non viene chiamata. La riga
 * continuava a dire *«Corsa eseguita: nessun turno da riprodurre.»* sopra un readout vuoto e senza nessuno
 * scenario a schermo.
 *
 * ⚠️ **Perche' il test sta QUI e non sul pannello.** `ClearSelection()` e' un membro privato di uno Slate
 * widget: headless non lo raggiunge nessuno. La correzione e' quindi doppia, e non per ridondanza —
 * `ClearSelection()` dimentica la corsa (il DATO), e `bScenarioSelected` impedisce alla riga di usarla (la
 * FRASE). Questo test misura la seconda meta', che e' quella che lo schermo mostra.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherTransportForgetsTheRunOnDeselectTest,
	"RefactorTactics.DevSandboxLauncher.TransportForgetsTheRunOnDeselect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherTransportForgetsTheRunOnDeselectTest::RunTest(const FString&)
{
	// Lo stato subito dopo `Esegui` su uno scenario la cui corsa non gioca turni.
	FRTLauncherTransportStatus DopoEsegui;
	DopoEsegui.bScenarioSelected = true;
	DopoEsegui.Run = ERTLauncherRunState::Ran;
	DopoEsegui.TurnsPlayed = 0;

	const FString ConScenario = FRTLauncherScenarioBrowser::DescribeTransport(DopoEsegui).ToString();
	TestTrue(FString::Printf(TEXT("con lo scenario a schermo la corsa si annuncia: %s"), *ConScenario),
		ConScenario.Contains(TEXT("nessun turno")));

	// 🎯 **Il clic nel vuoto.** La memoria della corsa e' rimasta identica — e' esattamente cio' che il
	// difetto lasciava indietro — e cambia solo la selezione.
	FRTLauncherTransportStatus DopoClicNelVuoto = DopoEsegui;
	DopoClicNelVuoto.bScenarioSelected = false;

	const FString SenzaScenario = FRTLauncherScenarioBrowser::DescribeTransport(DopoClicNelVuoto).ToString();
	TestFalse(FString::Printf(TEXT("senza scenario la riga non descrive piu' la corsa: %s"), *SenzaScenario),
		SenzaScenario == ConScenario);
	TestFalse(TEXT("e non annuncia nessuna corsa"), SenzaScenario.Contains(TEXT("Corsa")));

	// ⛔ **E non si legge nemmeno come «esegui uno scenario»**, che invita a un gesto impossibile: senza
	// selezione il pulsante `Esegui` e' disabilitato, e mandare a premerlo manda contro un muro.
	FRTLauncherTransportStatus MaiEseguito;
	MaiEseguito.bScenarioSelected = true;
	TestFalse(TEXT("ne' come un pulsante mai premuto, che sarebbe un invito impossibile"),
		SenzaScenario == FRTLauncherScenarioBrowser::DescribeTransport(MaiEseguito).ToString());

	// La frase dice cosa fare per davvero: scegliere dalla lista.
	TestTrue(FString::Printf(TEXT("dice cosa fare: %s"), *SenzaScenario),
		SenzaScenario.Contains(TEXT("lista")));

	// ⚠️ Vale per OGNI stato di corsa, non solo per quello che ha prodotto la riproduzione: la selezione e'
	// la condizione perche' esista un soggetto di cui parlare, e non un caso particolare di `Ran`.
	for (const ERTLauncherRunState Stato : {
		ERTLauncherRunState::NotRun, ERTLauncherRunState::Failed, ERTLauncherRunState::Ran,
		ERTLauncherRunState::Blocked, ERTLauncherRunState::Errored })
	{
		FRTLauncherTransportStatus Orfano;
		Orfano.Run = Stato;
		Orfano.TurnsPlayed = 9;
		Orfano.bPlaybackOpen = true;

		TestTrue(TEXT("nessuno stato di corsa parla senza uno scenario a schermo"),
			FRTLauncherScenarioBrowser::DescribeTransport(Orfano).ToString() == SenzaScenario);
	}

	return true;
}

/**
 * «Traccia non riproducibile» si dice dal REFERTO, non dal playback chiuso (#2836).
 *
 * 🔴 **La deduzione era sbagliata, e accusava la cosa sbagliata.**
 * `URTScenarioPreviewSubsystem::OpenPlayback` rifiuta per cause distinte — nessuna anteprima viva
 * (`!IsShowing()`), nessuna unita' posata (`AllUnits.Num() == 0`), tracce vuote, una traccia che non si
 * deserializza, la navigazione che non si apre — e la riga le traduceva tutte in *«traccia non
 * riproducibile»*. Con un viewport che non ha posato lo scenario, la traccia c'e' eccome: la frase
 * incolpava il runner per un difetto della preview, e mandava a cercare nel posto sbagliato.
 *
 * 🔑 Il segnale autorevole e' `FRTScenarioRunReport::bHasTrace`, che il RUNNER scrive — ed e' `false`
 * proprio negli aggregati a varianti, dove ne' hash ne' TurnLog esistono.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherTransportBlamesTheReportNotThePlaybackTest,
	"RefactorTactics.DevSandboxLauncher.TransportBlamesTheReportNotThePlayback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherTransportBlamesTheReportNotThePlaybackTest::RunTest(const FString&)
{
	// Due stati identici in tutto, tranne il campo che il referto dichiara.
	FRTLauncherTransportStatus SenzaTraccia;
	SenzaTraccia.bScenarioSelected = true;
	SenzaTraccia.Run = ERTLauncherRunState::Ran;
	SenzaTraccia.TurnsPlayed = 7;
	SenzaTraccia.bHasTrace = false;
	SenzaTraccia.bPlaybackOpen = false;

	FRTLauncherTransportStatus ConTracciaChiusa = SenzaTraccia;
	ConTracciaChiusa.bHasTrace = true;

	const FString Muta = FRTLauncherScenarioBrowser::DescribeTransport(SenzaTraccia).ToString();
	const FString Chiusa = FRTLauncherScenarioBrowser::DescribeTransport(ConTracciaChiusa).ToString();

	TestFalse(FString::Printf(TEXT("una traccia che esiste non si legge come una che manca: %s"), *Chiusa),
		Chiusa == Muta);

	// L'aggregato a varianti: il referto dichiara che non c'e' niente da confrontare, e la riga lo ripete.
	TestTrue(FString::Printf(TEXT("senza traccia lo si dice: %s"), *Muta),
		Muta.Contains(TEXT("traccia non riproducibile")));

	// ⛔ **E con la traccia NON lo si dice**, che e' l'asserzione che blocca il difetto: si nomina il
	// playback, che e' cio' che non si e' aperto.
	TestFalse(FString::Printf(TEXT("con la traccia non si accusa la traccia: %s"), *Chiusa),
		Chiusa.Contains(TEXT("traccia non riproducibile")));
	TestTrue(TEXT("si nomina il playback, che e' cio' che non si e' aperto"),
		Chiusa.Contains(TEXT("playback")));

	// ⚠️ Entrambe portano i turni giocati: la partita c'e' stata comunque, ed e' l'altra meta' della frase.
	TestTrue(TEXT("senza traccia i turni si dichiarano lo stesso"), Muta.Contains(TEXT("7")));
	TestTrue(TEXT("e con la traccia pure"), Chiusa.Contains(TEXT("7")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
