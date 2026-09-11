#include "Misc/AutomationTest.h"
#include "Algo/Reverse.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Turn/RTActionQueue.h"
#include "Turn/RTActionQueueLibrary.h"
#include "Turn/RTTurnRules.h"
#include "UObject/UnrealType.h" // TFieldIterator: lo sweep dei campi di FRTActionInstance (#2970)

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
		return FRTUnitOrderKey(FRTCellId(X, Y, Layer), StableUnitId, ActorName);
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
		for (const FRTUnitOrderKey& Key : Keys) { Out.Add(Key.ActorName); }
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
	const FRTUnitOrderKey A = UnitKey(2, 3, 0, /*StableUnitId*/ 4, TEXT("Unita_Alfa_0"));
	const FRTUnitOrderKey B = UnitKey(2, 3, 0, /*StableUnitId*/ 1, TEXT("Unita_Zeta_7"));

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
	// 🔴 **Il test che difende la chiave per cui #2922 esiste.** Non lo facevano `UnitOrderIsTotalOnSharedCell`,
	// `UnitOrderKeepsCellAsPrimaryKey`, `UnitOrderFallsBackToActorName` ne' `UnitOrderPermutationInvariant`:
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
	// ⚠️ Nomi INVENTATI, e deliberatamente non quelli di un eroe. Qui conta solo l'ordine lessicale fra
	// due stringhe, e le prime stesure ci avevano messo due identita' del roster **ritirato** — i vecchi
	// nomi Paragon, col prefisso delle Blueprint d'unita' — che #2291 esiste per purgare.
	//
	// 🔴 **E i letterali NON si ripetono qui**, nemmeno per spiegare: e' il punto. Quella decorazione ha
	// fatto costruire a due sessioni una diagnosi sbagliata (#2938), perche' un `grep` sul prefisso non
	// distingue una stringa d'arredo da un riferimento vivo. Riscriverli in un commento che racconta il
	// difetto lo **riprodurrebbe**. Quanto ne resta si conta, invece di scriverlo qui dove invecchia:
	//     git grep -hoE "BP_Unit_[A-Za-z]+" -- 'Source/RefactorTactics/Tests/*.cpp' | sort -u
	// I riferimenti VIVI stanno in `RTHeroSpawnTests.cpp`; tutto il resto e' commento o decorazione. La
	// lezione sta nel fatto, non nei nomi.
	const FRTUnitOrderKey A = UnitKey(1, 1, 0, /*Stable*/ 0, TEXT("Unita_Beta"));
	const FRTUnitOrderKey B = UnitKey(1, 1, 0, /*Stable*/ 0, TEXT("Unita_Alfa"));

	TestTrue(TEXT("senza identita' stabile decide il nome, e decide"),
		URTActionQueueLibrary::UnitOrderLess(A, B) != URTActionQueueLibrary::UnitOrderLess(B, A));
	TestTrue(TEXT("nome lessicalmente minore per primo"), URTActionQueueLibrary::UnitOrderLess(B, A));

	// 🔴 **Due nomi che differiscono solo per il caso restano DISTINGUIBILI**, e questa assertion e' la
	// ragione per cui la terza chiave e' una `FString` confrontata case-sensitive.
	//
	// ⚠️ Una stesura intermedia di #2922 era passata a `FName`, dove il confronto e' case-insensitive, e
	// aveva **invertito questa riga** per farla passare: certificava che il comparatore non sa separare
	// quella coppia, invece di fallire. E' il modo in cui un test smette di difendere qualcosa. Il caso e'
	// rappresentabile — due Actor in sublevel diversi, perche' l'unicita' degli `UObject` vale dentro un solo
	// Outer — e negli Editor target il caso e' preservato (`WITH_CASE_PRESERVING_NAME`), quindi qui morde.
	const FRTUnitOrderKey Upper = UnitKey(1, 1, 0, /*Stable*/ 0, TEXT("UNIT"));
	const FRTUnitOrderKey Lower = UnitKey(1, 1, 0, /*Stable*/ 0, TEXT("unit"));
	TestTrue(TEXT("nomi che differiscono solo per il caso restano distinguibili, in un verso solo"),
		URTActionQueueLibrary::UnitOrderLess(Upper, Lower) != URTActionQueueLibrary::UnitOrderLess(Lower, Upper));

	// Un `StableUnitId` assegnato batte comunque uno non assegnato, deterministicamente: `0` non e' l'unita'
	// zero, e non deve comportarsi come un valore mancante che scivola a caso.
	//
	// 🔴 **I nomi sono invertiti apposta.** La prima stesura dava a `Senza` il nome minore e a `Con` il
	// maggiore: i due criteri CONCORDAVANO, quindi togliendo la chiave `StableUnitId` questa assertion
	// restava verde e non misurava niente sugli id — la stessa vacuita' che questo test file esiste per
	// togliere, rifatta due riquadri piu' in la'. Trovato in code review.
	const FRTUnitOrderKey Senza = UnitKey(1, 1, 0, /*Stable*/ 0, TEXT("ZZZ"));
	const FRTUnitOrderKey Con   = UnitKey(1, 1, 0, /*Stable*/ 3, TEXT("AAA"));
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
	//  - `SU_AAA` e `SU_ZZZ` condividono la cella e si distinguono per `StableUnitId` — con i nomi in ordine
	//    OPPOSTO agli id, cosi' che sia la seconda chiave a decidere e non la terza;
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

// ---------------------------------------------------------------------------------------------------------
// Ordine delle AZIONI a pari tick e pari priorita' (#2970): le tre chiavi TECNICHE che chiudono
// `InstanceLess`, dopo le cinque che c'erano gia'.
//
// 🔑 **Spareggi tecnici, non priorita' di gioco.** Fase e `Priority` restano in testa e decidono come
// prima: `GameplayKeysStillBeatTechnicalTieBreaks` e `KeyPrecedenceIsPinned` sono li' a dirlo. Cio' che
// queste chiavi stabiliscono e' soltanto che l'esito non dipenda dall'ordine d'inserimento nel container
// (`CLAUDE.md` §11) — la stessa distinzione che `UnitOrderLess` fa fra la cella e i due spareggi che la
// seguono.
// ---------------------------------------------------------------------------------------------------------

namespace
{
	/**
	 * Un'istanza che pareggia su tutte e cinque le chiavi STORICHE, e si distingue solo su quelle nuove.
	 *
	 * 🔑 I tre parametri in coda sono esattamente i campi che `InstanceLess` non guardava: e' la fixture che
	 * rende raggiungibile il pareggio, e senza di essa non c'e' niente da provare.
	 */
	FRTActionInstance SameTickAction(int32 TargetUnitId, const FRTCellId& TargetCell, bool bInterrupted)
	{
		FRTActionInstance Instance = QueuedAction(TEXT("Action.BasicAttack"), ERTResolutionPhase::Attack,
			/*Priority*/ 50, /*SourceUnitId*/ 3, /*EventSequence*/ 7);
		Instance.TargetUnitId = TargetUnitId;
		Instance.TargetCell = TargetCell;
		Instance.bInterrupted = bInterrupted;
		return Instance;
	}

	/**
	 * L'identita' di un'istanza come STRINGA, per asserire una sequenza attesa **fissata**.
	 *
	 * 🔴 **Non si confronta l'uscita con un'uscita ricalcolata dallo stesso comparatore**, ed e' la lezione
	 * che #2922 ha pagato: la sua DoD prometteva che togliendo una chiave sarebbe caduto il test di
	 * permutazione, e quel test restava verde perche' atteso e trovato si spostavano insieme. Qui l'atteso e'
	 * scritto a mano una volta, e una chiave tolta lo fa divergere.
	 */
	FString SameTickIdentity(const FRTActionInstance& Instance)
	{
		return FString::Printf(TEXT("T%d/C%d,%d,%d/I%d"), Instance.TargetUnitId,
			Instance.TargetCell.X, Instance.TargetCell.Y, Instance.TargetCell.Layer,
			Instance.bInterrupted ? 1 : 0);
	}

	TArray<FString> SameTickIdentities(const TArray<FRTActionInstance>& Queue)
	{
		TArray<FString> Out;
		for (const FRTActionInstance& Instance : Queue) { Out.Add(SameTickIdentity(Instance)); }
		return Out;
	}

	/**
	 * Una chiave di `InstanceLess`: il campo `UPROPERTY` che perturba, e come metterla al minimo / al massimo.
	 *
	 * 🔑 **Il nome del campo sta QUI, accanto alla perturbazione, e non in un elenco separato** — ed e' una
	 * correzione fatta in code review (#3004). Prima l'insieme dei campi «confrontati» era una seconda lista
	 * scritta a mano: un campo nuovo rendeva rosso lo sweep, e la via piu' economica per il verde era
	 * **aggiungere il suo nome a quella lista** invece di insegnarlo a `InstanceLess`. La suite tornava
	 * interamente verde con il pareggio non deterministico ancora li'. Il commento lo vietava a parole e
	 * nient'altro lo impediva; ora dichiarare un campo confrontato SIGNIFICA dargli una perturbazione, e la
	 * perturbazione deve ribaltare il verdetto perche' la parte 2 dello sweep la misura.
	 *
	 * ⚠️ Puntatore a funzione e non `TFunction`: le lambda non catturano niente, quindi si convertono da
	 * sole e non c'e' alcuna type-erasure da pagare — ne' un include in piu'.
	 */
	struct FInstanceKey
	{
		/** Il nome dell'`UPROPERTY` di `FRTActionInstance` che questa chiave distingue. */
		const TCHAR* CampoUProperty;
		void (*Perturba)(FRTActionInstance& I, bool bHigh);
	};

	/**
	 * Le chiavi di `InstanceLess` **nell'ordine in cui decidono**. E' la tabella su cui poggiano sia lo sweep
	 * dei campi sia il pinning della precedenza.
	 *
	 * ⚠️ **Tre chiavi dichiarano lo stesso campo `Def`**, e non e' una svista: `InstanceLess` di `Def`
	 * guarda tre sottocampi in tre posizioni distinte della precedenza. Il campo e' coperto se **almeno una**
	 * lo distingue; che ne servano tre e' quanto serve a `KeyPrecedenceIsPinned`, non allo sweep.
	 */
	const TArray<FInstanceKey>& InstanceKeysInOrder()
	{
		static const TArray<FInstanceKey> Chiavi =
		{
			{ TEXT("Def"), [](FRTActionInstance& I, bool bHigh) { I.Def.ResolutionPhase = bHigh ? ERTResolutionPhase::NormalMovement : ERTResolutionPhase::Preparation; } },
			{ TEXT("Def"), [](FRTActionInstance& I, bool bHigh) { I.Def.Priority = bHigh ? 90 : 10; } },
			{ TEXT("Def"), [](FRTActionInstance& I, bool bHigh) { I.Def.ActionId = FName(bHigh ? TEXT("Action.ZZZ") : TEXT("Action.AAA")); } },
			{ TEXT("SourceUnitId"), [](FRTActionInstance& I, bool bHigh) { I.SourceUnitId = bHigh ? 9 : 1; } },
			{ TEXT("EventSequence"), [](FRTActionInstance& I, bool bHigh) { I.EventSequence = bHigh ? 9 : 1; } },
			{ TEXT("TargetUnitId"), [](FRTActionInstance& I, bool bHigh) { I.TargetUnitId = bHigh ? 9 : 1; } },
			// ⚠️ `StableLess` confronta **Layer -> X -> Y**, non solo X: il lato alto muove tutti e tre, cosi'
			// una sostituzione con un `A.TargetCell.X < B.TargetCell.X` non resta verde. La prima stesura di
			// questi test toccava il solo X, e la delega a `StableLess` non era provata. Trovato in code review.
			{ TEXT("TargetCell"), [](FRTActionInstance& I, bool bHigh) { I.TargetCell = bHigh ? FRTCellId(4, 5, 6) : FRTCellId(0, 0, 0); } },
			{ TEXT("bInterrupted"), [](FRTActionInstance& I, bool bHigh) { I.bInterrupted = bHigh; } },
		};
		return Chiavi;
	}

	/**
	 * I campi che `InstanceLess` confronta, **derivati dalle chiavi** invece che elencati a parte.
	 *
	 * ⛔ **Non esiste un modo di dichiarare un campo confrontato senza dargli una perturbazione** — che e'
	 * precisamente il buco chiuso da #3004.
	 */
	TSet<FString> CampiConfrontati()
	{
		TSet<FString> Nomi;
		for (const FInstanceKey& Chiave : InstanceKeysInOrder()) { Nomi.Add(Chiave.CampoUProperty); }
		return Nomi;
	}
}

/**
 * La forma canonica dell'ordine deve essere definita su OGNI campo che DISTINGUE due istanze.
 *
 * Gemello, per le azioni, di `RefactorTactics.TurnLog.CanonicalOrderCoversSerializedFields`. La ragione e'
 * la stessa, ed e' scritta accanto a `EntryLess`: un campo che il confronto non guarda lascia due istanze a
 * pari merito, e li' a decidere resta `TArray::Sort` — che inoltra ad `Algo::Sort`, introsort e **non**
 * stabile.
 *
 * 🔴 **E' uno SWEEP per reflection, non un elenco scritto a mano**, ed e' una correzione fatta in code
 * review. La prima stesura era una tabella degli otto campi che il comparatore gia' guardava: non poteva
 * cadere sul campo numero nove, cioe' non era un gate ma un esempio — **esattamente la forma che il commento
 * del gemello dichiara insufficiente** (*«verificava il solo `UnitId`, quindi era un esempio e non uno
 * sweep — un campo serializzato aggiunto dopo poteva mancare dall'ordine canonico e restare verde»*). Qui
 * la reflection enumera i campi di `FRTActionInstance` e ognuno deve essere **classificato**: o e' coperto da
 * una perturbazione che ribalta il verdetto, o sta nell'elenco delle esenzioni dichiarate. Un campo nuovo
 * non e' ne' l'uno ne' l'altro, e il test diventa rosso finche' qualcuno non decide quale dei due sia.
 *
 * E' la stessa forma di `RefactorTactics.BlindFire.BlastPreviewFieldsStayClosed`, che questo repository ha
 * gia' scelto una volta per lo stesso problema.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionCanonicalOrderCoversInstanceFieldsTest,
	"RefactorTactics.Actions.CanonicalOrderCoversInstanceFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionCanonicalOrderCoversInstanceFieldsTest::RunTest(const FString&)
{
	// --- 1) Ogni campo di `FRTActionInstance` e' classificato -------------------------------------------
	UScriptStruct* Struct = FRTActionInstance::StaticStruct();
	if (!TestNotNull(TEXT("FRTActionInstance risolta dalla reflection"), Struct)) { return false; }

	// I campi che `InstanceLess` confronta — **derivati dalle chiavi**, non riscritti qui (#3004).
	const TSet<FString> Confrontati = CampiConfrontati();

	// ⛔ **Esenzioni dichiarate: nessuna, oggi.** La riga esiste perche' il giorno in cui ne servisse una la
	// si scriva QUI con la ragione, invece di risolvere il rosso allargando l'elenco dei confrontati —
	// che e' lo stesso gesto con conseguenze opposte.
	const TSet<FString> EsentiDichiarati;

	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		const FString Nome = It->GetName();
		if (!Confrontati.Contains(Nome) && !EsentiDichiarati.Contains(Nome))
		{
			AddError(FString::Printf(
				TEXT("FRTActionInstance espone il campo '%s', che non e' ne' confrontato da InstanceLess ne' ")
				TEXT("esente dichiarato: se distingue due istanze deve entrare nell'ordine canonico, altrimenti ")
				TEXT("due istanze restano a pari merito e a decidere torna TArray::Sort, che non e' stabile ")
				TEXT("(#2970). Se invece NON le distingue, aggiungilo a EsentiDichiarati con la ragione"), *Nome));
		}
	}

	// --- 2) Ogni chiave confrontata ribalta davvero il verdetto -----------------------------------------
	const TArray<FInstanceKey>& Chiavi = InstanceKeysInOrder();
	for (int32 k = 0; k < Chiavi.Num(); ++k)
	{
		FRTActionInstance Minore = SameTickAction(/*Target*/ 7, FRTCellId(0, 0, 0), /*Interrotta*/ false);
		FRTActionInstance Maggiore = Minore;
		Chiavi[k].Perturba(Minore, /*bHigh*/ false);
		Chiavi[k].Perturba(Maggiore, /*bHigh*/ true);

		// 🔴 **L'etichetta deve corrispondere a cio' che la perturbazione MUOVE**, e lo verifica la
		// reflection invece della buona fede. Trovato in code review: senza questo, il buco di #3004 era
		// chiuso solo a meta'. Restava questa scorciatoia — duplicare una riga e cambiarne il solo nome:
		//
		//     { TEXT("CampoNuovo"), [](FRTActionInstance& I, bool bHigh) { I.SourceUnitId = bHigh ? 9 : 1; } }
		//
		// La parte 2 sarebbe passata (il verdetto si ribalta davvero — su `SourceUnitId`), `CampiConfrontati()`
		// avrebbe contenuto `CampoNuovo`, la parte 1 sarebbe tornata verde, e il pareggio non deterministico
		// sul campo nuovo sarebbe rimasto esattamente dov'era.
		FProperty* CampoDichiarato = Struct->FindPropertyByName(FName(Chiavi[k].CampoUProperty));
		if (TestNotNull(*FString::Printf(TEXT("chiave %d: il campo dichiarato '%s' esiste su FRTActionInstance"),
			k, Chiavi[k].CampoUProperty), CampoDichiarato))
		{
			TestFalse(*FString::Printf(
				TEXT("chiave %d: la perturbazione muove davvero il campo '%s' che dichiara"),
				k, Chiavi[k].CampoUProperty),
				CampoDichiarato->Identical_InContainer(&Minore, &Maggiore));
		}

		// 🔴 **E deve muovere SOLO quello.** L'assertion qui sopra chiede che il campo dichiarato si muova;
		// da sola non esclude che la lambda ne muova un SECONDO — ed e' il secondo a poter ribaltare il
		// verdetto. Cosi' il buco di #3004 restava aperto da una porta piu' larga (#3031): la scorciatoia
		// che quel commento vieta a parole non e' solo «duplicare una riga e cambiarne il nome», e'
		// duplicarla cambiandone il nome **e muovendo anche il campo nuovo**:
		//
		//     { TEXT("CampoNuovo"), [](FRTActionInstance& I, bool bHigh) {
		//           I.CampoNuovo   = bHigh ? 1 : 0;   // dichiarato, e mosso -> l'assertion sopra passa
		//           I.SourceUnitId = bHigh ? 9 : 1;   // ed e' QUESTO a ribaltare il verdetto
		//       } },
		//
		// Con quella riga la parte 1 (`CampiConfrontati()` contiene `CampoNuovo`), la parte 2 e
		// `Actions.KeyPrecedenceIsPinned` restano TUTTE verdi, mentre `InstanceLess` non guarda mai
		// `CampoNuovo`: due istanze che differiscono solo per esso tornano a pari merito, e a decidere torna
		// `TArray::Sort` — che inoltra ad `Algo::Sort`, introsort e non stabile. Isolando la perturbazione il
		// verdetto e' attribuibile alla SOLA chiave, e l'unico modo di tornare verdi e' insegnare il campo a
		// `InstanceLess` — che e' cio' che il gate esiste per ottenere.
		//
		// ⚠️ **Le tre chiavi che dichiarano `Def` passano per costruzione**: muovono un SOTTOcampo, quindi
		// `Def` resta la sola proprieta' di primo livello che differisce. Il gate lavora sulle proprieta' di
		// `FRTActionInstance`, non sui sottocampi — che e' anche il limite pinnato dalla parte 3.
		for (TFieldIterator<FProperty> AltroIt(Struct); AltroIt; ++AltroIt)
		{
			if (AltroIt->GetName() == Chiavi[k].CampoUProperty) { continue; }
			TestTrue(*FString::Printf(
				TEXT("chiave %d: la perturbazione NON muove '%s', che non dichiara"),
				k, *AltroIt->GetName()),
				AltroIt->Identical_InContainer(&Minore, &Maggiore));
		}

		TestTrue(*FString::Printf(TEXT("chiave %d: il lato minore precede"), k),
			URTActionQueueLibrary::InstanceLess(Minore, Maggiore));

		// ANTISIMMETRIA, e non solo determinismo: due istanze diverse non sono mai «uguali». E' il verso che
		// cade per primo quando un campo resta fuori dal confronto — entrambe le direzioni diventano false.
		TestFalse(*FString::Printf(TEXT("chiave %d: e non il contrario"), k),
			URTActionQueueLibrary::InstanceLess(Maggiore, Minore));
	}

	// --- 3) Il LIMITE dichiarato, pinnato invece che raccontato -----------------------------------------
	//
	// 🔴 **`Def` e' confrontata su tre sottocampi soltanto** — `ResolutionPhase`, `Priority`, `ActionId` — e
	// ne porta molti altri. Due istanze che differiscono **solo** per `Def.Effects` o `Def.RangeCells`
	// restano quindi a pari merito, e questo test lo ASSERISCE invece di lasciarlo implicito: una stesura
	// precedente ci scriveva sopra un commento che dichiarava il confronto esaustivo, il che era falso.
	// Trovato in code review.
	//
	// ⚠️ **La premessa che rende il limite accettabile va detta, perche' e' l'unica cosa che regge**: le
	// istanze che entrano in uno STESSO sort vengono da un solo produttore, e li' `EventSequence` e' distinto
	// per costruzione (un contatore per sede). `URTActionQueueLibrary::SortActionInstances` ha due chiamanti
	// fuori dai test, e uno solo riceve istanze reali — `ARTTurnManager::ResolvePrep`. Verificabile:
	//
	//     git grep -n "SortActionInstances" -- Source/RefactorTactics ":(exclude)Source/RefactorTactics/Tests"
	//
	// ⚠️ **Quel comando risponde piu' righe che chiamate** — fra i match ci sono anche la dichiarazione, la
	// definizione e i commenti. Le chiamate sono DUE: `ARTTurnManager::ResolvePrep` (`RTTurnManager.cpp`) e
	// `URTActionQueueLibrary::InstancesForPhase` (`RTActionQueueLibrary.cpp`). Cercale per nome: contare i
	// match porta fuori strada, e una stesura precedente diceva «un solo chiamante» lasciando al lettore una
	// smentita senza spiegazione (#3004). ⛔ Il filtro esclude `Source/RefactorTactics/Tests`, quindi questo
	// commento NON e' fra i match — il lettore non deve cercarlo li'.
	//
	// ⚠️ **L'altro e' `URTActionQueueLibrary::InstancesForPhase`, e una stesura precedente lo ometteva**
	// dichiarando «un solo chiamante» (#3004). Non rompe la premessa per **un solo motivo: non ha chiamanti**.
	// ⛔ Non dedurlo dal fatto che non sia `UFUNCTION`: in `RTActionQueueLibrary.h` non lo e' nessuno, nemmeno
	// `SortActionInstances`, quindi la proprieta' e' vera anche della porta che le istanze reali le riceve e
	// non distingue niente. E' `static` pubblica: un qualunque commit C++ puo' darle il primo chiamante.
	//
	// ⛔ **Il giorno in cui due produttori confluissero nello stesso array — e' la direzione di #1818 — la
	// premessa cade, e non basta aggiungere `Def` al confronto**: `FRTActionDef` appartiene al catalogo, e
	// confrontarla a fondo qui ne creerebbe una seconda verita'. Servirebbe un'identita' d'istanza unica per
	// turno, che oggi non esiste e che #2970 dichiara di non introdurre.
	FRTActionInstance ConEffettoA = SameTickAction(/*Target*/ 7, FRTCellId(0, 0, 0), /*Interrotta*/ false);
	ConEffettoA.Def.Effects = { FRTActionEffectSpec(ERTActionEffect::Damage, 10) };
	FRTActionInstance ConEffettoB = ConEffettoA;
	ConEffettoB.Def.Effects = { FRTActionEffectSpec(ERTActionEffect::Heal, 10) };

	TestFalse(TEXT("limite dichiarato: due istanze che differiscono solo in Def.Effects non si ordinano (A<B)"),
		URTActionQueueLibrary::InstanceLess(ConEffettoA, ConEffettoB));
	TestFalse(TEXT("limite dichiarato: ne' nel verso opposto (B<A)"),
		URTActionQueueLibrary::InstanceLess(ConEffettoB, ConEffettoA));
	return true;
}

/**
 * L'ORDINE DELLE CHIAVI, non solo la loro presenza.
 *
 * 🔴 **Senza questo test l'intera suite resta verde se si sposta `TargetUnitId` davanti ad `ActionId`**, e
 * quello sarebbe un cambio di regola di gioco: l'ordine di risoluzione di due azioni della stessa unita'
 * dipenderebbe dal bersaglio invece che dalla dichiarazione. Lo sweep dei campi non lo vede — perturba un
 * campo per volta da una base comune, quindi ogni riga e' decisa dalla sua sola chiave, qualunque sia la
 * posizione. Trovato in code review.
 *
 * Il metodo: per ogni chiave `k`, due istanze che pareggiano su tutte le chiavi PRECEDENTI, si separano su
 * `k` — e sono messe **al contrario** su tutte le SUCCESSIVE. Se `k` fosse scavalcata da una che la segue,
 * il verdetto si ribalterebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionKeyPrecedenceIsPinnedTest,
	"RefactorTactics.Actions.KeyPrecedenceIsPinned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionKeyPrecedenceIsPinnedTest::RunTest(const FString&)
{
	const TArray<FInstanceKey>& Chiavi = InstanceKeysInOrder();
	for (int32 k = 0; k < Chiavi.Num(); ++k)
	{
		FRTActionInstance Vince;
		FRTActionInstance Perde;
		for (int32 j = 0; j < Chiavi.Num(); ++j)
		{
			if (j < k)
			{
				Chiavi[j].Perturba(Vince, /*bHigh*/ true);  // identiche: le precedenti non decidono
				Chiavi[j].Perturba(Perde, /*bHigh*/ true);
			}
			else if (j == k)
			{
				Chiavi[j].Perturba(Vince, /*bHigh*/ false); // `Vince` e' minore QUI, e solo qui
				Chiavi[j].Perturba(Perde, /*bHigh*/ true);
			}
			else
			{
				Chiavi[j].Perturba(Vince, /*bHigh*/ true);  // ...e maggiore su TUTTE le successive
				Chiavi[j].Perturba(Perde, /*bHigh*/ false);
			}
		}

		TestTrue(*FString::Printf(
			TEXT("la chiave %d decide, malgrado tutte le successive dicano il contrario"), k),
			URTActionQueueLibrary::InstanceLess(Vince, Perde));
		TestFalse(*FString::Printf(TEXT("chiave %d: e in un verso solo"), k),
			URTActionQueueLibrary::InstanceLess(Perde, Vince));
	}
	return true;
}

/**
 * Stesso tick, stessa priorita', stessa azione, stessa unita': a distinguere restano solo i tre campi nuovi.
 *
 * E' il caso che #2970 esiste per chiudere. Prima, qui, `InstanceLess` rispondeva `false` nei due versi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionOrderIsTotalOnSameTickAndPriorityTest,
	"RefactorTactics.Actions.OrderIsTotalOnSameTickAndPriority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionOrderIsTotalOnSameTickAndPriorityTest::RunTest(const FString&)
{
	// ⚠️ Le celle cambiano **Layer**, non solo X: `StableLess` confronta Layer -> X -> Y, e una fixture che
	// muovesse il solo X lascerebbe la delega non provata. Trovato in code review.
	TArray<FRTActionInstance> Coda;
	Coda.Add(SameTickAction(/*Target*/ 9, FRTCellId(0, 0, 0), /*Interrotta*/ false)); // D
	Coda.Add(SameTickAction(/*Target*/ 7, FRTCellId(5, 5, 1), /*Interrotta*/ false)); // C — Layer 1
	Coda.Add(SameTickAction(/*Target*/ 7, FRTCellId(0, 3, 0), /*Interrotta*/ true));  // B
	Coda.Add(SameTickAction(/*Target*/ 7, FRTCellId(0, 3, 0), /*Interrotta*/ false)); // A

	URTActionQueueLibrary::SortActionInstances(Coda);

	// La sequenza attesa e' SCRITTA, non ricalcolata: `TargetUnitId` -> `TargetCell` (`StableLess`:
	// Layer -> X -> Y, quindi il Layer 0 precede il Layer 1 malgrado X e Y maggiori) -> `bInterrupted`.
	const TArray<FString> Atteso = { TEXT("T7/C0,3,0/I0"), TEXT("T7/C0,3,0/I1"),
		TEXT("T7/C5,5,1/I0"), TEXT("T9/C0,0,0/I0") };
	const TArray<FString> Ottenuto = SameTickIdentities(Coda);

	TestEqual(*FString::Printf(TEXT("ordine a pari tick: [%s]"), *FString::Join(Ottenuto, TEXT(","))),
		FString::Join(Ottenuto, TEXT(",")), FString::Join(Atteso, TEXT(",")));

	// 🔴 L'assertion che nessuna implementazione di `Sort` puo' rendere vacua — la stessa di
	// `UnitOrderPermutationInvariant`, e per la stessa ragione: su array piccoli `Algo::Sort` ripiega su un
	// insertion sort che di fatto e' stabile, quindi un comparatore PARZIALE puo' produrre la sequenza giusta
	// per un accidente dell'engine. Qui la proprieta' si chiede direttamente.
	bool bTotale = true;
	for (int32 i = 1; i < Coda.Num(); ++i)
	{
		bTotale &= URTActionQueueLibrary::InstanceLess(Coda[i - 1], Coda[i]);
	}
	TestTrue(TEXT("ordine totale: nessuna coppia indistinguibile, nemmeno a pari tick e pari priorita'"), bTotale);
	return true;
}

/**
 * Gli stessi eventi inseriti in ordine diverso -> uscita identica. TUTTE le permutazioni, non tre a mano.
 *
 * ⚠️ **L'atteso e' una costante scritta**, e non il risultato di un secondo ordinamento: e' la correzione che
 * #2922 ha dovuto fare alla propria DoD dopo il merge. Un test di permutazione che ricalcola l'atteso con lo
 * stesso comparatore mutato resta verde qualunque chiave si tolga, e non prova niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionSameTickPermutationInvariantTest,
	"RefactorTactics.Actions.SameTickPermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionSameTickPermutationInvariantTest::RunTest(const FString&)
{
	TArray<FRTActionInstance> Base;
	Base.Add(SameTickAction(/*Target*/ 7, FRTCellId(0, 3, 0), /*Interrotta*/ false));
	Base.Add(SameTickAction(/*Target*/ 7, FRTCellId(0, 3, 0), /*Interrotta*/ true));
	Base.Add(SameTickAction(/*Target*/ 7, FRTCellId(5, 5, 1), /*Interrotta*/ false));
	Base.Add(SameTickAction(/*Target*/ 9, FRTCellId(0, 0, 0), /*Interrotta*/ false));

	const FString Atteso = TEXT("T7/C0,3,0/I0,T7/C0,3,0/I1,T7/C5,5,1/I0,T9/C0,0,0/I0");

	int32 Divergenti = 0;
	FString PrimaDivergenza;
	const int32 Permutazioni = UnitOrderFactorial(Base.Num());
	for (int32 P = 0; P < Permutazioni; ++P)
	{
		const TArray<int32> Ordine = UnitOrderNthPermutation(Base.Num(), P);

		TArray<FRTActionInstance> Coda;
		for (const int32 Idx : Ordine) { Coda.Add(Base[Idx]); }
		URTActionQueueLibrary::SortActionInstances(Coda);

		const FString Ottenuto = FString::Join(SameTickIdentities(Coda), TEXT(","));
		if (Ottenuto != Atteso)
		{
			++Divergenti;
			if (PrimaDivergenza.IsEmpty())
			{
				PrimaDivergenza = FString::Printf(TEXT("permutazione %d: [%s] invece di [%s]"),
					P, *Ottenuto, *Atteso);
			}
		}
	}

	const FString Esito = FString::Printf(TEXT("permutare l'ingresso non cambia l'ordine risolto (%s)"),
		PrimaDivergenza.IsEmpty() ? TEXT("nessuna divergenza") : *PrimaDivergenza);
	TestEqual(*Esito, Divergenti, 0);
	return true;
}

/**
 * Le chiavi di GIOCO restano in testa: le tre nuove non spostano nulla dove il comparatore gia' decideva.
 *
 * E' l'acceptance scenario 2 di #2970 detto come test — *«dove non c'e' pareggio, l'ordine e' identico a
 * quello di prima»* — ed e' la meta' che protegge il corpus golden. Senza, questa issue sarebbe
 * indistinguibile da un cambio di regola di gameplay.
 *
 * ⚠️ **Verde anche PRIMA della modifica, ed e' corretto che lo sia**: asserisce cio' che non e' cambiato.
 * La precedenza fra TUTTE le chiavi, invece, la pinna `KeyPrecedenceIsPinned`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionGameplayKeysBeatTechnicalTieBreaksTest,
	"RefactorTactics.Actions.GameplayKeysStillBeatTechnicalTieBreaks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionGameplayKeysBeatTechnicalTieBreaksTest::RunTest(const FString&)
{
	// Le nuove chiavi sono messe CONTRO le vecchie, ciascuna nel verso che le farebbe vincere se avesse la
	// precedenza: bersaglio piu' basso, cella minore e non interrotta stanno tutte dalla parte che
	// perderebbe. Se una di esse fosse salita davanti a fase o priorita', questi confronti cadrebbero.
	FRTActionInstance MoveMinore = QueuedAction(TEXT("Action.Move"), ERTResolutionPhase::NormalMovement,
		/*Priority*/ 10, /*SourceUnitId*/ 0, /*EventSequence*/ 0);
	MoveMinore.TargetUnitId = 1;
	MoveMinore.TargetCell = FRTCellId(0, 0, 0);

	FRTActionInstance AttaccoMaggiore = QueuedAction(TEXT("Action.HeavyAttack"), ERTResolutionPhase::Attack,
		/*Priority*/ 90, /*SourceUnitId*/ 9, /*EventSequence*/ 9);
	AttaccoMaggiore.TargetUnitId = 99;
	AttaccoMaggiore.TargetCell = FRTCellId(9, 9, 9);
	AttaccoMaggiore.bInterrupted = true;

	TestTrue(TEXT("la macro-fase decide prima di qualunque spareggio tecnico"),
		URTActionQueueLibrary::InstanceLess(AttaccoMaggiore, MoveMinore));
	TestFalse(TEXT("e il Move non risale davanti all'attacco per via del bersaglio"),
		URTActionQueueLibrary::InstanceLess(MoveMinore, AttaccoMaggiore));

	// Stessa prova dentro UNA fase: e' la priorita' a ordinare, non il bersaglio.
	FRTActionInstance PrioritaBassa = SameTickAction(/*Target*/ 99, FRTCellId(9, 9, 9), /*Interrotta*/ true);
	PrioritaBassa.Def.Priority = 20;
	FRTActionInstance PrioritaAlta = SameTickAction(/*Target*/ 1, FRTCellId(0, 0, 0), /*Interrotta*/ false);
	PrioritaAlta.Def.Priority = 80;

	TestTrue(TEXT("dentro la fase decide la priorita', non il bersaglio"),
		URTActionQueueLibrary::InstanceLess(PrioritaBassa, PrioritaAlta));
	TestFalse(TEXT("e in un verso solo"),
		URTActionQueueLibrary::InstanceLess(PrioritaAlta, PrioritaBassa));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
