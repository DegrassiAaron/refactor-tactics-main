#include "Misc/AutomationTest.h"
#include "Replay/RTReplaySeekLibrary.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Core/RTTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FRTTurnLogEntry Entry(int32 Turn, ERTMatchPhase Phase, ERTLogCategory Cat, int32 Amount)
	{
		FRTTurnLogEntry E;
		E.TurnNumber = Turn;
		E.Phase = Phase;
		E.Category = Cat;
		E.Amount = Amount;
		E.SrcCell = FRTCellId(Amount, Turn);
		E.TgtCell = FRTCellId(Amount + 1, Turn);
		return E;
	}

	/**
	 * Tre turni. La forma canonica e' la PRECONDIZIONE della libreria, quindi la fixture la produce con
	 * `SortTurnLog` invece di dichiararla a parole — cosi' il test verifica il seek, non la mia capacita' di
	 * scrivere le voci gia' in ordine.
	 *
	 * Il turno 2 non ha voci in `Prep`: e' il caso «la fase non ha prodotto niente», che non e' un errore.
	 */
	TArray<TArray<FRTTurnLogEntry>> MakeSequence()
	{
		TArray<FRTTurnLogEntry> T1;
		T1.Add(Entry(1, ERTMatchPhase::Move,  ERTLogCategory::Move,   30)); // inseriti fuori ordine di proposito
		T1.Add(Entry(1, ERTMatchPhase::Blast, ERTLogCategory::Combat, 21));
		T1.Add(Entry(1, ERTMatchPhase::Prep,  ERTLogCategory::Combat, 10));
		T1.Add(Entry(1, ERTMatchPhase::Blast, ERTLogCategory::Combat, 20));

		TArray<FRTTurnLogEntry> T2;
		T2.Add(Entry(2, ERTMatchPhase::Move,  ERTLogCategory::Move,   31));
		T2.Add(Entry(2, ERTMatchPhase::Blast, ERTLogCategory::Combat, 22));
		T2.Add(Entry(2, ERTMatchPhase::Move,  ERTLogCategory::Move,   32));

		TArray<FRTTurnLogEntry> T3;
		T3.Add(Entry(3, ERTMatchPhase::Move, ERTLogCategory::Move, 33));

		URTTurnLogLibrary::SortTurnLog(T1);
		URTTurnLogLibrary::SortTurnLog(T2);
		URTTurnLogLibrary::SortTurnLog(T3);

		TArray<TArray<FRTTurnLogEntry>> Seq;
		Seq.Add(T1);
		Seq.Add(T2);
		Seq.Add(T3);
		return Seq;
	}

	/** Scansione lineare: avanza una voce per volta finche' il cursore punta al bersaglio. E' l'oracolo. */
	bool AdvanceUntil(const TArray<TArray<FRTTurnLogEntry>>& Seq, FRTReplayCursor& Cursor,
		TFunctionRef<bool(const FRTTurnLogEntry&)> IsTarget)
	{
		int32 Budget = 0;
		for (const TArray<FRTTurnLogEntry>& Trace : Seq) { Budget += Trace.Num(); }

		for (int32 Step = 0; Step <= Budget; ++Step)
		{
			if (Seq.IsValidIndex(Cursor.TraceIndex) && Seq[Cursor.TraceIndex].IsValidIndex(Cursor.EntryIndex)
				&& IsTarget(Seq[Cursor.TraceIndex][Cursor.EntryIndex]))
			{
				return true;
			}
			if (!URTReplaySeekLibrary::AdvanceOneEntry(Seq, Cursor)) { return false; }
		}
		return false;
	}
}

// Il seek alla fase da' la PRIMA voce di quella fase. Regge perche' `Phase` e' la prima chiave di
// `EntryLess` (spec-turnlog.md §6) e l'enum e' in ordine cronologico: in una traccia canonica le voci di
// una fase sono contigue. Il turno 1 ha Prep(1) + Blast(2) + Move(1), quindi Blast comincia a 1 e Move a 3.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplaySeekPhaseTest,
	"RefactorTactics.Replay.Seek.PhaseStartsAtItsFirstEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplaySeekPhaseTest::RunTest(const FString&)
{
	const TArray<TArray<FRTTurnLogEntry>> Seq = MakeSequence();

	int32 Index = -1;
	TestEqual(TEXT("Blast e' una posizione del turno 1"),
		URTReplaySeekLibrary::SeekToPhase(Seq[0], ERTMatchPhase::Blast, Index), ERTReplaySeekResult::Found);
	TestEqual(TEXT("Blast comincia dopo l'unica voce di Prep"), Index, 1);

	TestEqual(TEXT("Move e' una posizione del turno 1"),
		URTReplaySeekLibrary::SeekToPhase(Seq[0], ERTMatchPhase::Move, Index), ERTReplaySeekResult::Found);
	TestEqual(TEXT("Move comincia dopo Prep e le due voci di Blast"), Index, 3);

	return true;
}

// Una fase senza voci non e' una posizione. Restituire l'indice della fase successiva risponderebbe a
// un'altra domanda, e chi legge non avrebbe modo di accorgersene.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplaySeekEmptyPhaseTest,
	"RefactorTactics.Replay.Seek.PhaseWithoutEntriesIsNotAPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplaySeekEmptyPhaseTest::RunTest(const FString&)
{
	const TArray<TArray<FRTTurnLogEntry>> Seq = MakeSequence();

	int32 Index = -1;
	TestEqual(TEXT("nel turno 2 nessuno ha agito in Prep"),
		URTReplaySeekLibrary::SeekToPhase(Seq[1], ERTMatchPhase::Prep, Index), ERTReplaySeekResult::PhaseNotFound);

	FRTReplayCursor Cursor;
	TestEqual(TEXT("e nemmeno passando dal turno"),
		URTReplaySeekLibrary::SeekToTurnPhase(Seq, 2, ERTMatchPhase::Prep, Cursor),
		ERTReplaySeekResult::PhaseNotFound);

	return true;
}

// L'ORACOLO di #415: saltare al turno N e scorrere fino al turno N devono dare la stessa posizione.
// Se le due divergono, il seek non e' una scorciatoia: e' un'altra cosa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplaySeekTurnEquivalenceTest,
	"RefactorTactics.Replay.Seek.TurnEqualsLinearScan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplaySeekTurnEquivalenceTest::RunTest(const FString&)
{
	const TArray<TArray<FRTTurnLogEntry>> Seq = MakeSequence();

	FRTReplayCursor Played;
	const bool bReached = AdvanceUntil(Seq, Played,
		[](const FRTTurnLogEntry& E) { return E.TurnNumber == 3; });
	TestTrue(TEXT("la scansione lineare arriva al turno 3"), bReached);

	FRTReplayCursor Sought;
	TestEqual(TEXT("il turno 3 esiste"),
		URTReplaySeekLibrary::SeekToTurn(Seq, 3, Sought), ERTReplaySeekResult::Found);

	TestTrue(TEXT("seek e scansione lasciano il cursore nello stesso punto"), Played == Sought);
	return true;
}

// Stessa equivalenza, un livello piu' fine: la fase dentro il turno.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplaySeekPhaseEquivalenceTest,
	"RefactorTactics.Replay.Seek.PhaseEqualsLinearScan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplaySeekPhaseEquivalenceTest::RunTest(const FString&)
{
	const TArray<TArray<FRTTurnLogEntry>> Seq = MakeSequence();

	FRTReplayCursor Played;
	const bool bReached = AdvanceUntil(Seq, Played, [](const FRTTurnLogEntry& E)
		{ return E.TurnNumber == 2 && E.Phase == ERTMatchPhase::Move; });
	TestTrue(TEXT("la scansione lineare arriva al Move del turno 2"), bReached);

	FRTReplayCursor Sought;
	TestEqual(TEXT("il Move del turno 2 esiste"),
		URTReplaySeekLibrary::SeekToTurnPhase(Seq, 2, ERTMatchPhase::Move, Sought), ERTReplaySeekResult::Found);

	TestTrue(TEXT("seek e scansione lasciano il cursore nello stesso punto"), Played == Sought);
	return true;
}

// Fail-closed: un turno che non c'e' lo dice, e non lascia in giro un cursore plausibile.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplaySeekMissingTurnTest,
	"RefactorTactics.Replay.Seek.MissingTurnFailsExplicitly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplaySeekMissingTurnTest::RunTest(const FString&)
{
	const TArray<TArray<FRTTurnLogEntry>> Seq = MakeSequence();

	FRTReplayCursor Cursor;
	Cursor.TraceIndex = 7;
	Cursor.EntryIndex = 7;

	TestEqual(TEXT("il turno 9 non esiste"),
		URTReplaySeekLibrary::SeekToTurn(Seq, 9, Cursor), ERTReplaySeekResult::TurnNotFound);
	TestEqual(TEXT("il cursore in uscita non e' stato toccato (traccia)"), Cursor.TraceIndex, 7);
	TestEqual(TEXT("il cursore in uscita non e' stato toccato (voce)"), Cursor.EntryIndex, 7);

	return true;
}

// Compatibilita': una traccia scritta prima del formato v6 dichiara `TurnNumber = 0`. Non e' indirizzabile
// per turno, e questo E' l'esito corretto — dedurre il turno dalla posizione nell'array sarebbe un'inferenza
// ANALOGA a quella che D-063 ha dichiarato non valida per l'identita' dell'unita' (li' era la cella al posto
// di `UnitId`; D-063 non parla di turni, quindi qui vale l'analogia e non la citazione).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplaySeekLegacyTraceTest,
	"RefactorTactics.Replay.Seek.TraceWithoutTurnNumberIsNotAddressable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplaySeekLegacyTraceTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Legacy;
	Legacy.Add(Entry(0, ERTMatchPhase::Blast, ERTLogCategory::Combat, 40)); // v5: nessun TurnNumber
	URTTurnLogLibrary::SortTurnLog(Legacy);

	TArray<TArray<FRTTurnLogEntry>> Seq;
	Seq.Add(Legacy);

	FRTReplayCursor Cursor;
	Cursor.TraceIndex = 5;
	Cursor.EntryIndex = 5;

	TestEqual(TEXT("una traccia v5 non risponde al turno 1"),
		URTReplaySeekLibrary::SeekToTurn(Seq, 1, Cursor), ERTReplaySeekResult::TurnNotFound);

	// Il caso che rende vera la frase «non indirizzabile per turno»: chiedere PROPRIO `0`. Senza questo
	// assert il sentinella «turno non dichiarato» combacerebbe con la richiesta e il seek risponderebbe
	// «trovato», trasformando l'assenza del dato in una posizione — cioe' l'opposto del fail-closed.
	// I turni veri partono da 1 (`ARTTurnManager::TurnNumber`), quindi `0` non e' un turno: e' un buco.
	TestEqual(TEXT("e nemmeno al turno 0, che non e' un turno ma il sentinella del campo assente"),
		URTReplaySeekLibrary::SeekToTurn(Seq, 0, Cursor), ERTReplaySeekResult::TurnNotFound);
	TestEqual(TEXT("il cursore non e' stato toccato nemmeno li'"), Cursor.TraceIndex, 5);

	int32 Index = -1;
	TestEqual(TEXT("ma la fase resta indirizzabile: non dipende dal turno dichiarato"),
		URTReplaySeekLibrary::SeekToPhase(Seq[0], ERTMatchPhase::Blast, Index), ERTReplaySeekResult::Found);
	TestEqual(TEXT("ed e' la prima voce"), Index, 0);

	return true;
}


/**
 * **`SeekToBoundary` indirizza la terza coordinata, e distingue le due assenze** (`#1880`).
 *
 * ⛔ **Il caso che decide e' `BoundaryNotFound` contro `PhaseNotFound`.** Sono due fatti diversi — «il turno
 * non ha attraversato quella fase» e «la fase c'e' ma non a quel micro-step» — e un seek che li confondesse
 * manderebbe a cercare nel turno sbagliato. Un'implementazione che restituisse sempre `PhaseNotFound`
 * passerebbe qualunque test che non li separi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplaySeekBoundaryTest,
	"RefactorTactics.Replay.Seek.BoundaryIsAddressable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplaySeekBoundaryTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Traccia;
	{
		FRTTurnLogEntry A = Entry(1, ERTMatchPhase::Move, ERTLogCategory::Move, 1);
		A.MicroStepIndex = 0;
		Traccia.Add(A);

		// Due voci allo STESSO boundary: sono un gruppo simultaneo.
		FRTTurnLogEntry B = Entry(1, ERTMatchPhase::Move, ERTLogCategory::Move, 2);
		B.MicroStepIndex = 3;
		Traccia.Add(B);
		FRTTurnLogEntry C = Entry(1, ERTMatchPhase::Move, ERTLogCategory::Move, 9);
		C.MicroStepIndex = 3;
		Traccia.Add(C);

		FRTTurnLogEntry D = Entry(1, ERTMatchPhase::Blast, ERTLogCategory::Combat, 4);
		D.MicroStepIndex = 0;
		Traccia.Add(D);
	}
	URTTurnLogLibrary::SortTurnLog(Traccia);

	int32 Indice = INDEX_NONE;

	// --- la terna si indirizza ------------------------------------------------------------------------
	TestEqual(TEXT("il boundary 3 di Move si trova"),
		URTReplaySeekLibrary::SeekToBoundary(Traccia, ERTMatchPhase::Move, 3, Indice),
		ERTReplaySeekResult::Found);
	if (Traccia.IsValidIndex(Indice))
	{
		TestEqual(TEXT("e l'indice punta a una voce di QUEL boundary"), Traccia[Indice].MicroStepIndex, 3);
		TestEqual(TEXT("e di quella fase"), Traccia[Indice].Phase, ERTMatchPhase::Move);
	}

	TestEqual(TEXT("e il boundary 0 e' un'altra posizione"),
		URTReplaySeekLibrary::SeekToBoundary(Traccia, ERTMatchPhase::Move, 0, Indice),
		ERTReplaySeekResult::Found);
	if (Traccia.IsValidIndex(Indice))
	{
		TestEqual(TEXT("col suo micro-step"), Traccia[Indice].MicroStepIndex, 0);
	}

	// --- 🔴 le due assenze, che non vanno confuse -------------------------------------------------------
	TestEqual(TEXT("la fase c'e' ma non a quel micro-step: BoundaryNotFound"),
		URTReplaySeekLibrary::SeekToBoundary(Traccia, ERTMatchPhase::Move, 99, Indice),
		ERTReplaySeekResult::BoundaryNotFound);

	TestEqual(TEXT("la fase non c'e' affatto: PhaseNotFound"),
		URTReplaySeekLibrary::SeekToBoundary(Traccia, ERTMatchPhase::Dash, 0, Indice),
		ERTReplaySeekResult::PhaseNotFound);

	// --- ⛔ il gruppo resta un gruppo -------------------------------------------------------------------
	// Le due voci del boundary 3 devono stare INSIEME dopo l'ordinamento canonico: se `EntryLess` le
	// separasse, il boundary smetterebbe di essere una posizione e diventerebbe un insieme sparso.
	int32 Primo = INDEX_NONE, Ultimo = INDEX_NONE, Quante = 0;
	for (int32 i = 0; i < Traccia.Num(); ++i)
	{
		if (Traccia[i].Phase == ERTMatchPhase::Move && Traccia[i].MicroStepIndex == 3)
		{
			if (Primo == INDEX_NONE) { Primo = i; }
			Ultimo = i;
			++Quante;
		}
	}
	TestEqual(TEXT("le due voci del boundary ci sono entrambe"), Quante, 2);
	TestEqual(TEXT("e sono adiacenti: il gruppo non si sparpaglia"), Ultimo - Primo, Quante - 1);

	// ⚠️ E `SeekToBoundary` punta all'INIZIO del gruppo, non a una voce qualsiasi.
	URTReplaySeekLibrary::SeekToBoundary(Traccia, ERTMatchPhase::Move, 3, Indice);
	TestEqual(TEXT("il seek punta alla prima voce del gruppo"), Indice, Primo);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
