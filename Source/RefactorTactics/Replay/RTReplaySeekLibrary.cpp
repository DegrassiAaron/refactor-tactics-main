#include "Replay/RTReplaySeekLibrary.h"

ERTReplaySeekResult URTReplaySeekLibrary::SeekToPhase(const TArray<FRTTurnLogEntry>& Trace, ERTMatchPhase Phase,
	int32& OutEntryIndex)
{
	// Scansione lineare, e non una ricerca binaria che la forma canonica pure consentirebbe: il valore del
	// seek e' non RIPRODURRE i turni precedenti, non il costo di trovare l'indice. Una traccia e' un turno,
	// cioe' decine di voci — la binaria qui comprerebbe complessita' al posto di tempo.
	for (int32 Index = 0; Index < Trace.Num(); ++Index)
	{
		if (Trace[Index].Phase == Phase)
		{
			OutEntryIndex = Index;
			return ERTReplaySeekResult::Found;
		}
	}
	return ERTReplaySeekResult::PhaseNotFound;
}

ERTReplaySeekResult URTReplaySeekLibrary::SeekToBoundary(const TArray<FRTTurnLogEntry>& Trace,
	ERTMatchPhase Phase, int32 MicroStepIndex, int32& OutEntryIndex)
{
	// Stessa scansione lineare di `SeekToPhase`, e per la stessa ragione: il valore del seek e' non
	// riprodurre cio' che precede, non il costo di trovare l'indice.
	//
	// ⚠️ Si distinguono DUE assenze. Se la fase non compare affatto, l'esito e' `PhaseNotFound` — il turno
	// non l'ha attraversata. Se la fase c'e' ma non a quel micro-step, e' `BoundaryNotFound`: la barriera
	// chiesta non esiste in un turno che pure ha avuto quella fase. Dire la prima al posto della seconda
	// manderebbe a cercare il turno sbagliato.
	bool bPhaseSeen = false;
	for (int32 Index = 0; Index < Trace.Num(); ++Index)
	{
		if (Trace[Index].Phase != Phase)
		{
			continue;
		}
		bPhaseSeen = true;

		if (Trace[Index].MicroStepIndex == MicroStepIndex)
		{
			// La PRIMA in ordine canonico, che e' dove il gruppo comincia. Non la prima accaduta: le voci di
			// un boundary sono simultanee, e la traccia non porta — deliberatamente — un ordine fra loro.
			OutEntryIndex = Index;
			return ERTReplaySeekResult::Found;
		}
	}

	return bPhaseSeen ? ERTReplaySeekResult::BoundaryNotFound : ERTReplaySeekResult::PhaseNotFound;
}

ERTReplaySeekResult URTReplaySeekLibrary::SeekToTurn(const TArray<TArray<FRTTurnLogEntry>>& Sequence,
	int32 TurnNumber, FRTReplayCursor& OutCursor)
{
	// `0` non e' un turno: e' il sentinella «non dichiarato» delle tracce pre-v6 (i turni veri partono da 1,
	// `ARTTurnManager::TurnNumber`). Senza questo rifiuto la richiesta combacerebbe con il buco e il seek
	// risponderebbe «trovato», trasformando l'assenza del dato in una posizione — l'opposto del fail-closed,
	// e la smentita di cio' che l'header di questa funzione promette.
	if (TurnNumber <= 0)
	{
		return ERTReplaySeekResult::TurnNotFound;
	}

	for (int32 TraceIndex = 0; TraceIndex < Sequence.Num(); ++TraceIndex)
	{
		const TArray<FRTTurnLogEntry>& Trace = Sequence[TraceIndex];
		// Il turno di una traccia e' quello che le sue voci DICHIARANO. Una traccia vuota non ne dichiara
		// nessuno, e una v5 dichiara 0: in entrambi i casi non e' indirizzabile, che e' l'esito giusto.
		if (Trace.Num() == 0 || Trace[0].TurnNumber != TurnNumber)
		{
			continue;
		}
		OutCursor.TraceIndex = TraceIndex;
		OutCursor.EntryIndex = 0;
		return ERTReplaySeekResult::Found;
	}
	// Fail-closed: `OutCursor` resta come l'ha lasciato il chiamante. Azzerarlo darebbe una posizione
	// valida — l'inizio della sequenza — a chi ha chiesto un turno che non esiste.
	return ERTReplaySeekResult::TurnNotFound;
}

ERTReplaySeekResult URTReplaySeekLibrary::SeekToTurnPhase(const TArray<TArray<FRTTurnLogEntry>>& Sequence,
	int32 TurnNumber, ERTMatchPhase Phase, FRTReplayCursor& OutCursor)
{
	FRTReplayCursor Candidate;
	const ERTReplaySeekResult TurnResult = SeekToTurn(Sequence, TurnNumber, Candidate);
	if (TurnResult != ERTReplaySeekResult::Found)
	{
		return TurnResult;
	}

	int32 EntryIndex = 0;
	const ERTReplaySeekResult PhaseResult = SeekToPhase(Sequence[Candidate.TraceIndex], Phase, EntryIndex);
	if (PhaseResult != ERTReplaySeekResult::Found)
	{
		return PhaseResult;
	}

	Candidate.EntryIndex = EntryIndex;
	OutCursor = Candidate;
	return ERTReplaySeekResult::Found;
}

bool URTReplaySeekLibrary::AdvanceOneEntry(const TArray<TArray<FRTTurnLogEntry>>& Sequence, FRTReplayCursor& Cursor)
{
	if (!Sequence.IsValidIndex(Cursor.TraceIndex))
	{
		return false;
	}

	if (Cursor.EntryIndex + 1 < Sequence[Cursor.TraceIndex].Num())
	{
		++Cursor.EntryIndex;
		return true;
	}

	// Traccia esaurita: si passa alla prima traccia successiva che abbia voci. Saltare quelle vuote qui
	// invece di fermarsi su di esse tiene il cursore sempre su una posizione leggibile, che e' la
	// precondizione con cui il chiamante lo confronta con un seek.
	for (int32 Next = Cursor.TraceIndex + 1; Next < Sequence.Num(); ++Next)
	{
		if (Sequence[Next].Num() > 0)
		{
			Cursor.TraceIndex = Next;
			Cursor.EntryIndex = 0;
			return true;
		}
	}
	return false;
}
