#include "Misc/AutomationTest.h"
#include "Algo/Reverse.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Turn/RTActionQueue.h"
#include "Turn/RTActionQueueLibrary.h"
#include "Turn/RTTurnRules.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Istanza minima: chi la esegue, con quale definizione. Nome distinto per file (unity build). */
	FRTActionInstance QueuedAction(const FName& Id, ERTResolutionPhase Phase, int32 Priority,
		int32 SourceUnitId, int32 EventSequence = 0)
	{
		FRTActionInstance Instance;
		Instance.Def.ActionId = Id;
		Instance.Def.ResolutionPhase = Phase;
		Instance.Def.Priority = Priority;
		Instance.Def.Fallback = (Phase == ERTResolutionPhase::NormalMovement || Phase == ERTResolutionPhase::FastMovement)
			? ERTActionFallback::Stop : ERTActionFallback::Cancel;
		Instance.SourceUnitId = SourceUnitId;
		Instance.EventSequence = EventSequence;
		return Instance;
	}

	/** Gli ActionId nell'ordine in cui la coda li risolve. */
	TArray<FName> ResolvedOrder(const TArray<FRTActionInstance>& Queue)
	{
		TArray<FName> Out;
		for (const FRTActionInstance& Instance : Queue) { Out.Add(Instance.Def.ActionId); }
		return Out;
	}
}

// ---------------------------------------------------------------------------------------------------------
// Ordinamento: macro-fase -> priorita' -> ActionId -> SourceUnitId -> EventSequence
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionOrderByPriorityTest,
	"RefactorTactics.Actions.OrderByPriority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionOrderByPriorityTest::RunTest(const FString&)
{
	// Dentro la stessa macro-fase, priorita' MINORE risolve prima (regola del catalogo v0.1).
	TArray<FRTActionInstance> Queue;
	Queue.Add(QueuedAction(TEXT("Action.HeavyAttack"),     ERTResolutionPhase::Attack, 80, /*Unit*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.PrecisionAttack"), ERTResolutionPhase::Attack, 60, /*Unit*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.Push"),            ERTResolutionPhase::Control, 40, /*Unit*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.Interrupt"),       ERTResolutionPhase::Control, 20, /*Unit*/ 0));

	URTActionQueueLibrary::SortActionInstances(Queue);
	const TArray<FName> Order = ResolvedOrder(Queue);

	// Controllo e attacco stanno entrambi nel Blast: e' la PRIORITA' a metterli in fila, non una fase in piu'.
	TestEqual(TEXT("interrupt (20) per primo"), Order[0], FName(TEXT("Action.Interrupt")));
	TestEqual(TEXT("push (40) secondo"),        Order[1], FName(TEXT("Action.Push")));
	TestEqual(TEXT("precisione (60) terzo"),    Order[2], FName(TEXT("Action.PrecisionAttack")));
	TestEqual(TEXT("pesante (80) per ultimo"),  Order[3], FName(TEXT("Action.HeavyAttack")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionPhaseRespectsAtlasTest,
	"RefactorTactics.Actions.PhaseMappingRespectsAtlas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionPhaseRespectsAtlasTest::RunTest(const FString&)
{
	// La macro-fase viene PRIMA della priorita': un Move a priorita' 50 risolve dopo un attacco a priorita' 80,
	// perche' il Move e' una macro-fase successiva. E' l'identita' tattica di Atlas (ADR-0003 §1): l'attacco
	// vale da fermo, muoversi e' un impegno che si paga. Il catalogo v0.1 metteva il movimento PRIMA.
	TArray<FRTActionInstance> Queue;
	Queue.Add(QueuedAction(TEXT("Action.Move"),        ERTResolutionPhase::NormalMovement, 50, /*Unit*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.HeavyAttack"), ERTResolutionPhase::Attack,         80, /*Unit*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.Dodge"),        ERTResolutionPhase::FastMovement,   30, /*Unit*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.Guard"),       ERTResolutionPhase::Preparation,    40, /*Unit*/ 0));

	URTActionQueueLibrary::SortActionInstances(Queue);
	const TArray<FName> Order = ResolvedOrder(Queue);

	TestEqual(TEXT("Prep per primo"),   Order[0], FName(TEXT("Action.Guard")));
	TestEqual(TEXT("poi il Dash"),      Order[1], FName(TEXT("Action.Dodge")));
	TestEqual(TEXT("poi il Blast"),     Order[2], FName(TEXT("Action.HeavyAttack")));
	TestEqual(TEXT("il Move per ULTIMO, dopo l'attacco"), Order[3], FName(TEXT("Action.Move")));

	// La stessa cosa detta come invariante, non come sequenza: qualunque coda contenga entrambe, il Move
	// non puo' mai precedere un attacco.
	const int32 MoveIdx = Order.IndexOfByKey(FName(TEXT("Action.Move")));
	const int32 AttackIdx = Order.IndexOfByKey(FName(TEXT("Action.HeavyAttack")));
	TestTrue(TEXT("Action.Move risolve DOPO Action.HeavyAttack"), MoveIdx > AttackIdx);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionPermutationInvariantTest,
	"RefactorTactics.Actions.PermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionPermutationInvariantTest::RunTest(const FString&)
{
	// Coda con collisioni su OGNI chiave, cosi' il tie-break viene esercitato fino in fondo: stessa fase,
	// stessa priorita', stesso ActionId, e infine stesso SourceUnitId (a distinguere resta EventSequence).
	TArray<FRTActionInstance> Queue;
	Queue.Add(QueuedAction(TEXT("Action.BasicAttack"), ERTResolutionPhase::Attack, 50, /*Unit*/ 2, /*Seq*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.BasicAttack"), ERTResolutionPhase::Attack, 50, /*Unit*/ 1, /*Seq*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.Heal"),        ERTResolutionPhase::Attack, 50, /*Unit*/ 1, /*Seq*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.BasicAttack"), ERTResolutionPhase::Attack, 50, /*Unit*/ 1, /*Seq*/ 1));
	Queue.Add(QueuedAction(TEXT("Action.Move"),        ERTResolutionPhase::NormalMovement, 50, /*Unit*/ 0));

	TArray<FRTActionInstance> Sorted = Queue;
	URTActionQueueLibrary::SortActionInstances(Sorted);

	// Ordine invertito in ingresso -> stessa sequenza in uscita.
	TArray<FRTActionInstance> Reversed = Queue;
	Algo::Reverse(Reversed);
	URTActionQueueLibrary::SortActionInstances(Reversed);

	// E una permutazione qualsiasi, per non verificare solo il caso simmetrico.
	TArray<FRTActionInstance> Shuffled;
	Shuffled.Add(Queue[3]); Shuffled.Add(Queue[0]); Shuffled.Add(Queue[4]);
	Shuffled.Add(Queue[2]); Shuffled.Add(Queue[1]);
	URTActionQueueLibrary::SortActionInstances(Shuffled);

	bool bSame = Sorted.Num() == Reversed.Num() && Sorted.Num() == Shuffled.Num();
	for (int32 i = 0; bSame && i < Sorted.Num(); ++i)
	{
		bSame = Sorted[i].Def.ActionId == Reversed[i].Def.ActionId
			&& Sorted[i].SourceUnitId == Reversed[i].SourceUnitId
			&& Sorted[i].EventSequence == Reversed[i].EventSequence
			&& Sorted[i].Def.ActionId == Shuffled[i].Def.ActionId
			&& Sorted[i].SourceUnitId == Shuffled[i].SourceUnitId
			&& Sorted[i].EventSequence == Shuffled[i].EventSequence;
	}
	TestTrue(TEXT("permutare l'ingresso non cambia l'ordine risolto"), bSame);

	// L'ordine e' TOTALE: due istanze adiacenti non possono mai essere "equivalenti", altrimenti a deciderle
	// resterebbe l'ordine di arrivo — cioe' il caso.
	bool bTotal = true;
	for (int32 i = 1; i < Sorted.Num(); ++i)
	{
		bTotal &= URTActionQueueLibrary::InstanceLess(Sorted[i - 1], Sorted[i]);
	}
	TestTrue(TEXT("ordine totale: nessuna coppia indistinguibile"), bTotal);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Ordine delle UNITA' (#2922): cella -> StableUnitId -> nome dell'Actor
//
// Si prova sulla CHIAVE e non sull'Actor, ed e' deliberato: `ARTUnit` e' un Actor e vorrebbe un mondo,
// mentre la regola d'ordine non ne ha bisogno per essere vera. Il percorso completo che passa dagli Actor —
// `CollectLivingUnits` -> `MakeCurrentSnapshot` -> risoluzione — ha gia' il suo test, che spawna un mondo:
// `RefactorTactics.Match.Autobattle.DeterminismSurvivesUnitPermutation`.
// ---------------------------------------------------------------------------------------------------------

namespace
{
	/** Chiave d'ordine minima. Nome distinto per file (unity build). */
	FRTUnitOrderKey UnitKey(int32 X, int32 Y, int32 Layer, int32 StableUnitId, const TCHAR* ActorName)
	{
		return FRTUnitOrderKey(FRTCellId(X, Y, Layer), StableUnitId, FName(ActorName));
	}

	void SortUnitKeys(TArray<FRTUnitOrderKey>& Keys)
	{
		Keys.Sort([](const FRTUnitOrderKey& A, const FRTUnitOrderKey& B)
		{
			return URTActionQueueLibrary::UnitOrderLess(A, B);
		});
	}

	/** Le chiavi identificate dal nome, nell'ordine risolto. */
	TArray<FString> UnitOrderNames(const TArray<FRTUnitOrderKey>& Keys)
	{
		TArray<FString> Out;
		for (const FRTUnitOrderKey& Key : Keys) { Out.Add(Key.ActorName.ToString()); }
		return Out;
	}

	int32 UnitOrderFactorial(int32 N)
	{
		int32 Out = 1;
		for (int32 i = 2; i <= N; ++i) { Out *= i; }
		return Out;
	}

	/**
	 * La `Index`-esima permutazione di `[0..N)` in ordine lessicografico, per enumerarle TUTTE.
	 *
	 * ⚠️ Tutte e non tre scelte a mano: una permutazione campionata prova che quel caso funziona, non che la
	 * proprieta' vale. Con `N == 5` sono 120 riordini, che costano niente.
	 */
	TArray<int32> UnitOrderNthPermutation(int32 N, int32 Index)
	{
		TArray<int32> Pool;
		for (int32 i = 0; i < N; ++i) { Pool.Add(i); }

		TArray<int32> Out;
		for (int32 i = N; i >= 1; --i)
		{
			const int32 BlockSize = UnitOrderFactorial(i - 1);
			const int32 Pick = Index / BlockSize;
			Index %= BlockSize;
			Out.Add(Pool[Pick]);
			Pool.RemoveAt(Pick);
		}
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitOrderIsTotalOnSharedCellTest,
	"RefactorTactics.Actions.UnitOrderIsTotalOnSharedCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitOrderIsTotalOnSharedCellTest::RunTest(const FString&)
{
	// Due unita' VIVE sulla stessa cella: non e' un caso teorico. `URTHexSimLibrary::MakeSnapshot` la
	// registra in `FRTHexSnapshot::Overlaps` e `ARTTurnManager::ReportSnapshotOverlaps` la segnala a runtime
	// (#1733, #1970) — quindi il comparatore la incontra, e sulla sola cella pareggerebbe.
	//
	// 🔑 **Id e nome DISCORDANO, ed e' deliberato**: `A` ha l'id maggiore e il nome lessicalmente minore.
	// Con le fixture concordanti della prima stesura, togliere la chiave `StableUnitId` lasciava questo test
	// verde — il nome decideva nello stesso verso. Una fixture che non distingue le due chiavi non prova
	// quale delle due sta lavorando. Trovato in code review.
	const FRTUnitOrderKey A = UnitKey(2, 3, 0, /*StableUnitId*/ 4, TEXT("BP_Unit_Aaa_0"));
	const FRTUnitOrderKey B = UnitKey(2, 3, 0, /*StableUnitId*/ 1, TEXT("BP_Unit_Zzz_7"));

	// 🔴 L'assertion che nessuna mutazione del sort puo' salvare: l'ordine e' TOTALE, cioe' per ogni coppia
	// distinta esattamente uno dei due versi e' vero. Con il comparatore sulla sola cella non lo sarebbe
	// **nessuno** dei due, e a decidere resterebbe l'ordine di arrivo nel container.
	const bool bAB = URTActionQueueLibrary::UnitOrderLess(A, B);
	const bool bBA = URTActionQueueLibrary::UnitOrderLess(B, A);
	TestTrue(TEXT("stessa cella: esattamente un verso e' vero"), bAB != bBA);
	TestTrue(TEXT("l'identita' stabile minore entra prima, MALGRADO il nome dica il contrario"), bBA);

	// Irriflessivita': una chiave non precede se stessa, altrimenti il sort non ha un punto fisso.
	TestFalse(TEXT("nessuna chiave precede se stessa"), URTActionQueueLibrary::UnitOrderLess(A, A));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitOrderStableIdBeatsActorNameTest,
	"RefactorTactics.Actions.UnitOrderStableIdBeatsActorName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitOrderStableIdBeatsActorNameTest::RunTest(const FString&)
{
	// 🔴 **Il test che difende la chiave per cui #2922 esiste, e che i primi quattro non difendevano.**
	// L'anti-vacuita' dichiarata nella issue diceva: tolta `StableUnitId`, un test deve diventare rosso.
	// Non era vero: in ogni fixture l'ordine dei nomi concordava con quello degli id, quindi il nome
	// rispondeva al posto suo e tutto restava verde. Qui i due criteri dicono cose OPPOSTE, e vince l'id.
	//
	// ⚠️ Se questa riga cade, non e' un dettaglio di ordinamento: e' che la partita torna a spareggiare
	// sulla `MakeUniqueObjectName`, cioe' sul contatore di spawn — l'input che [#990] aveva tolto.
	TArray<FRTUnitOrderKey> Keys;
	Keys.Add(UnitKey(4, 4, 0, /*Stable*/ 7, TEXT("AAA_nome_minore_id_maggiore")));
	Keys.Add(UnitKey(4, 4, 0, /*Stable*/ 2, TEXT("ZZZ_nome_maggiore_id_minore")));
	SortUnitKeys(Keys);

	const TArray<FString> Order = UnitOrderNames(Keys);
	TestEqual(TEXT("con la stessa cella decide l'id, non il nome"),
		Order[0], FString(TEXT("ZZZ_nome_maggiore_id_minore")));

	// E il verso opposto: a parita' di id, allora si', decide il nome. Le due meta' insieme dicono che le
	// chiavi sono DUE e in quest'ordine — una sola assertion non distinguerebbe «id vince» da «id esiste».
	TArray<FRTUnitOrderKey> PariId;
	PariId.Add(UnitKey(4, 4, 0, /*Stable*/ 3, TEXT("ZZZ")));
	PariId.Add(UnitKey(4, 4, 0, /*Stable*/ 3, TEXT("AAA")));
	SortUnitKeys(PariId);
	TestEqual(TEXT("a parita' di id decide il nome"), UnitOrderNames(PariId)[0], FString(TEXT("AAA")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitOrderKeepsCellAsPrimaryKeyTest,
	"RefactorTactics.Actions.UnitOrderKeepsCellAsPrimaryKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitOrderKeepsCellAsPrimaryKeyTest::RunTest(const FString&)
{
	// Anti-vacuita' dell'altro verso: le due chiavi nuove NON devono poter riordinare unita' su celle
	// diverse. Se lo facessero, l'ordine di risoluzione cambierebbe dove oggi non pareggia — cioe' ovunque —
	// e con esso il corpus golden. La cella governa da sola, come prima di #2922.
	//
	// `StableUnitId` e nome dicono entrambi il contrario della cella, apposta: e devono perdere.
	TArray<FRTUnitOrderKey> Keys;
	Keys.Add(UnitKey(5, 0, 0, /*Stable*/ 1, TEXT("AAA")));  // cella lontana, id minore, nome primo
	Keys.Add(UnitKey(0, 0, 0, /*Stable*/ 9, TEXT("ZZZ")));  // cella vicina, id maggiore, nome ultimo
	SortUnitKeys(Keys);

	const TArray<FString> Order = UnitOrderNames(Keys);
	TestEqual(TEXT("la cella minore entra prima, malgrado id e nome"), Order[0], FString(TEXT("ZZZ")));
	TestEqual(TEXT("la cella maggiore entra dopo"),                    Order[1], FString(TEXT("AAA")));

	// E il Layer viene prima di X e Y, che e' `URTHexLibrary::StableLess` e non una regola nuova.
	TArray<FRTUnitOrderKey> Layered;
	Layered.Add(UnitKey(0, 0, 1, /*Stable*/ 1, TEXT("SOPRA")));
	Layered.Add(UnitKey(9, 9, 0, /*Stable*/ 2, TEXT("SOTTO")));
	SortUnitKeys(Layered);
	TestEqual(TEXT("Layer inferiore per primo"), UnitOrderNames(Layered)[0], FString(TEXT("SOTTO")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitOrderFallsBackToActorNameTest,
	"RefactorTactics.Actions.UnitOrderFallsBackToActorName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitOrderFallsBackToActorNameTest::RunTest(const FString&)
{
	// `StableUnitId` vale `0` finche' `ARTTurnManager::EnsureMatchRoster()` non e' passato, e c'e' almeno un
	// chiamante che ordina PRIMA: `ARTGameMode::AssignUnitControlGroups`, a inizio partita. Li' le prime due
	// chiavi pareggiano entrambe, e senza la terza il pareggio tornerebbe a `GetAllActorsOfClass`.
	const FRTUnitOrderKey A = UnitKey(1, 1, 0, /*Stable*/ 0, TEXT("BP_Unit_Riktor_2"));
	const FRTUnitOrderKey B = UnitKey(1, 1, 0, /*Stable*/ 0, TEXT("BP_Unit_Gadget_1"));

	TestTrue(TEXT("senza identita' stabile decide il nome, e decide"),
		URTActionQueueLibrary::UnitOrderLess(A, B) != URTActionQueueLibrary::UnitOrderLess(B, A));
	TestTrue(TEXT("nome lessicalmente minore per primo"), URTActionQueueLibrary::UnitOrderLess(B, A));

	// 🔴 **Il caso che con una `FString` sarebbe stato un buco, e con un `FName` non e' rappresentabile.**
	// `FName::LexicalLess` e' case-INSENSITIVE, quindi due nomi che differiscono solo per il caso
	// pareggerebbero in entrambi i versi — l'ordine non sarebbe totale. Ma quella coppia non puo' esistere
	// fra due Actor: i nomi degli `UObject` sono unici in modo case-insensitive dentro lo stesso Outer,
	// quindi `FName("UNIT")` e `FName("unit")` sono **lo stesso nome**, cioe' la stessa unita'.
	//
	// ⚠️ Questa assertion nasce ROSSA e corretta: la stesura precedente teneva una `FString` confrontata
	// case-sensitive e affermava che i due restassero distinguibili. Passando a `FName` — per non allocare
	// a ogni confronto — quell'affermazione e' diventata falsa, e il test l'ha presa. Trovato dalla suite,
	// non dalla rilettura.
	const FRTUnitOrderKey Upper = UnitKey(1, 1, 0, /*Stable*/ 0, TEXT("UNIT"));
	const FRTUnitOrderKey Lower = UnitKey(1, 1, 0, /*Stable*/ 0, TEXT("unit"));
	TestTrue(TEXT("due nomi che differiscono solo per il caso sono lo STESSO FName, non due unita'"),
		Upper.ActorName == Lower.ActorName);
	TestFalse(TEXT("e infatti nessuno dei due precede l'altro: non c'e' una coppia da spareggiare"),
		URTActionQueueLibrary::UnitOrderLess(Upper, Lower) || URTActionQueueLibrary::UnitOrderLess(Lower, Upper));

	// Due nomi VERAMENTE diversi, invece, si ordinano: e' la proprieta' che la terza chiave deve avere.
	const FRTUnitOrderKey Primo  = UnitKey(1, 1, 0, /*Stable*/ 0, TEXT("BP_Unit_Aaa"));
	const FRTUnitOrderKey Ultimo = UnitKey(1, 1, 0, /*Stable*/ 0, TEXT("BP_Unit_Zzz"));
	TestTrue(TEXT("nomi distinti restano distinguibili, in un verso solo"),
		URTActionQueueLibrary::UnitOrderLess(Primo, Ultimo) != URTActionQueueLibrary::UnitOrderLess(Ultimo, Primo));

	// Un `StableUnitId` assegnato batte comunque uno non assegnato, deterministicamente: `0` non e' l'unita'
	// zero, e non deve comportarsi come un valore mancante che scivola a caso.
	const FRTUnitOrderKey Senza = UnitKey(1, 1, 0, /*Stable*/ 0, TEXT("AAA"));
	const FRTUnitOrderKey Con   = UnitKey(1, 1, 0, /*Stable*/ 3, TEXT("ZZZ"));
	TestTrue(TEXT("id non assegnato prima di uno assegnato, e non a caso"),
		URTActionQueueLibrary::UnitOrderLess(Senza, Con));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitOrderPermutationInvariantTest,
	"RefactorTactics.Actions.UnitOrderPermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitOrderPermutationInvariantTest::RunTest(const FString&)
{
	// L'insieme collide su OGNI chiave, cosi' il tie-break viene esercitato fino in fondo:
	//  - `SU_A` e `SU_B` condividono la cella e si distinguono per `StableUnitId`;
	//  - `NOME_A` e `NOME_B` condividono cella E `StableUnitId == 0`, e si distinguono solo per il nome;
	//  - `SOLA` sta altrove e tiene onesto il confronto sulla prima chiave.
	TArray<FRTUnitOrderKey> Base;
	Base.Add(UnitKey(2, 3, 0, /*Stable*/ 4, TEXT("SU_AAA")));
	Base.Add(UnitKey(2, 3, 0, /*Stable*/ 1, TEXT("SU_ZZZ")));
	Base.Add(UnitKey(7, 1, 0, /*Stable*/ 0, TEXT("NOME_A")));
	Base.Add(UnitKey(7, 1, 0, /*Stable*/ 0, TEXT("NOME_B")));
	Base.Add(UnitKey(0, 0, 1, /*Stable*/ 2, TEXT("SOLA")));

	TArray<FRTUnitOrderKey> Riferimento = Base;
	SortUnitKeys(Riferimento);
	const TArray<FString> Atteso = UnitOrderNames(Riferimento);

	// TUTTE le permutazioni dell'ingresso, non tre scelte a mano.
	const int32 N = Base.Num();
	const int32 Permutazioni = UnitOrderFactorial(N);
	int32 Divergenti = 0;
	FString PrimaDivergenza;

	for (int32 P = 0; P < Permutazioni; ++P)
	{
		const TArray<int32> Indici = UnitOrderNthPermutation(N, P);
		TArray<FRTUnitOrderKey> Permutato;
		Permutato.Reserve(N);
		for (int32 i = 0; i < N; ++i) { Permutato.Add(Base[Indici[i]]); }

		SortUnitKeys(Permutato);
		const TArray<FString> Ottenuto = UnitOrderNames(Permutato);

		if (Ottenuto != Atteso)
		{
			++Divergenti;
			if (PrimaDivergenza.IsEmpty())
			{
				PrimaDivergenza = FString::Printf(TEXT("permutazione %d: [%s] invece di [%s]"),
					P, *FString::Join(Ottenuto, TEXT(",")), *FString::Join(Atteso, TEXT(",")));
			}
		}
	}

	const FString Esito = FString::Printf(TEXT("permutare l'ingresso non cambia l'ordine risolto (%s)"),
		PrimaDivergenza.IsEmpty() ? TEXT("nessuna divergenza") : *PrimaDivergenza);
	TestEqual(*Esito, Divergenti, 0);

	// 🔴 L'assertion che nessuna implementazione di `Sort` puo' rendere vacua. `Algo::Sort` non e' stabile,
	// ma su array piccoli ripiega su un insertion sort che di fatto lo e': un comparatore parziale potrebbe
	// quindi passare il ciclo qui sopra per un accidente dell'engine, non perche' l'ordine sia totale. Questa
	// riga chiede la proprieta' direttamente — ogni coppia adiacente e' STRETTAMENTE ordinata — e cade se la
	// chiave `StableUnitId` o quella del nome vengono tolte dal comparatore.
	bool bTotale = true;
	for (int32 i = 1; i < Riferimento.Num(); ++i)
	{
		bTotale &= URTActionQueueLibrary::UnitOrderLess(Riferimento[i - 1], Riferimento[i]);
	}
	TestTrue(TEXT("ordine totale: nessuna coppia indistinguibile, nemmeno sulla cella condivisa"), bTotale);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
