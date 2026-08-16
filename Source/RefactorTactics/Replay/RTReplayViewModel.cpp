#include "Replay/RTReplayViewModel.h"

const TArray<ERTMatchPhase>& FRTReplayViewModel::ObservablePhases()
{
	// Nell'ordine cronologico di `ERTMatchPhase`, che e' anche quello di `EntryLess`: dentro una traccia
	// canonica le fasi si susseguono cosi'. `Planning` e `MatchEnded` non ci sono perche' nessun punto del
	// resolver le emette — non e' una scelta di questo file, e' un fatto misurato che `RTReplaySeekLibrary`
	// documenta e che qui si smette di dover ricordare.
	static const TArray<ERTMatchPhase> Fasi = {
		ERTMatchPhase::Prep,
		ERTMatchPhase::Dash,
		ERTMatchPhase::Blast,
		ERTMatchPhase::Move,
		ERTMatchPhase::Cleanup
	};
	return Fasi;
}

ERTReplayOpenResult FRTReplayViewModel::Open(const FString& ReplaysRoot, const FGuid& MatchId)
{
	// Reset esplicito invece di `*this = FRTReplayViewModel()`: `SecondsPerPhase` e' configurazione di chi
	// ci guarda dentro, non stato della partita aperta, e riaprire un archivio non deve resettargliela.
	Session = FRTReplaySession();
	Current = FRTReplayPosition();
	CurrentTraceIndex = INDEX_NONE;
	bPlaying = false;
	PhaseElapsed = 0.f;
	bOpen = false;

	OpenResult = URTReplayPlayerLibrary::OpenArchive(ReplaysRoot, MatchId, Session);
	if (OpenResult != ERTReplayOpenResult::Opened)
	{
		// Fail-closed come il Player: su un rifiuto non resta niente di mezzo. Una sessione parzialmente
		// popolata sarebbe la cosa peggiore da consegnare a una UI, che la troverebbe navigabile.
		Session = FRTReplaySession();
		return OpenResult;
	}

	bOpen = true;
	return OpenResult;
}

void FRTReplayViewModel::Rewind()
{
	URTReplayPlayerLibrary::Rewind(Session);
	Current = FRTReplayPosition();
	CurrentTraceIndex = INDEX_NONE;
	bPlaying = false;
	PhaseElapsed = 0.f;
}

TArray<ERTMatchPhase> FRTReplayViewModel::PhasesInTrace(int32 TraceIndex) const
{
	TArray<ERTMatchPhase> Presenti;
	if (!Session.Traces.IsValidIndex(TraceIndex))
	{
		return Presenti;
	}

	const TArray<FRTTurnLogEntry>& Traccia = Session.Traces[TraceIndex];
	for (const ERTMatchPhase Fase : ObservablePhases())
	{
		int32 Indice = 0;
		if (URTReplaySeekLibrary::SeekToPhase(Traccia, Fase, Indice) == ERTReplaySeekResult::Found)
		{
			Presenti.Add(Fase);
		}
	}
	return Presenti;
}

int32 FRTReplayViewModel::AdjacentNonEmptyTrace(int32 From, int32 Direction) const
{
	for (int32 i = From + Direction; Session.Traces.IsValidIndex(i); i += Direction)
	{
		if (PhasesInTrace(i).Num() > 0)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

FRTReplayPosition FRTReplayViewModel::MakePosition(int32 TraceIndex, ERTMatchPhase Phase) const
{
	FRTReplayPosition P;
	P.Phase = Phase;

	if (!Session.Traces.IsValidIndex(TraceIndex) || Session.Traces[TraceIndex].Num() == 0)
	{
		// Non raggiungibile passando dai due `Next*Position`, che scartano le tracce senza fasi. Resta
		// perche' la funzione e' totale: una posizione a cui non corrisponde una traccia non e' `AtPhase`.
		P.State = ERTReplayPositionState::Ended;
		return P;
	}

	// Il turno di una traccia e' quello che le sue voci DICHIARANO — la stessa regola di `SeekToTurn`, e
	// per la stessa ragione: `0` e' il sentinella delle tracce pre-v6, non il turno zero. Dedurlo
	// dall'indice della traccia sarebbe trasformare l'assenza del dato in un numero plausibile.
	const int32 Turno = Session.Traces[TraceIndex][0].TurnNumber;
	if (Turno <= 0)
	{
		P.State = ERTReplayPositionState::Unaddressable;
		P.TurnNumber = 0;
	}
	else
	{
		P.State = ERTReplayPositionState::AtPhase;
		P.TurnNumber = Turno;
	}
	return P;
}

void FRTReplayViewModel::ApplyPosition(const FRTReplayPosition& NewPos, int32 NewTraceIndex)
{
	Current = NewPos;
	CurrentTraceIndex = NewTraceIndex;
	PhaseElapsed = 0.f;
}

bool FRTReplayViewModel::NextPhasePosition(int32 Direction, FRTReplayPosition& OutPos,
	int32& OutTraceIndex) const
{
	if (!bOpen || Direction == 0)
	{
		return false;
	}

	if (Current.State == ERTReplayPositionState::BeforeStart)
	{
		if (Direction < 0)
		{
			return false; // bordo chiuso: prima dell'inizio non c'e' niente
		}
		const int32 Prima = AdjacentNonEmptyTrace(INDEX_NONE, +1);
		if (Prima == INDEX_NONE)
		{
			return false; // nessuna traccia con fasi: non c'e' niente da guardare
		}
		OutTraceIndex = Prima;
		OutPos = MakePosition(Prima, PhasesInTrace(Prima)[0]);
		return true;
	}

	if (Current.State == ERTReplayPositionState::Ended)
	{
		if (Direction > 0)
		{
			return false; // bordo chiuso: dopo la fine non c'e' niente
		}
		const int32 Ultima = AdjacentNonEmptyTrace(Session.Traces.Num(), -1);
		if (Ultima == INDEX_NONE)
		{
			return false;
		}
		OutTraceIndex = Ultima;
		OutPos = MakePosition(Ultima, PhasesInTrace(Ultima).Last());
		return true;
	}

	// `AtPhase` o `Unaddressable`: si e' dentro una traccia, e la fase corrente e' una delle sue.
	const TArray<ERTMatchPhase> Fasi = PhasesInTrace(CurrentTraceIndex);
	const int32 Pos = Fasi.IndexOfByKey(Current.Phase);
	if (Pos == INDEX_NONE)
	{
		return false;
	}

	const int32 Prossima = Pos + Direction;
	if (Fasi.IsValidIndex(Prossima))
	{
		OutTraceIndex = CurrentTraceIndex;
		OutPos = MakePosition(CurrentTraceIndex, Fasi[Prossima]);
		return true;
	}

	// Confine di turno. Si salta alle tracce **senza fasi** invece di fermarcisi: un turno in cui non e'
	// successo niente non e' una posizione, e mostrarlo darebbe una schermata vuota da cui il giocatore
	// non capirebbe se e' un turno tranquillo o un difetto.
	const int32 Adiacente = AdjacentNonEmptyTrace(CurrentTraceIndex, Direction);
	if (Adiacente != INDEX_NONE)
	{
		const TArray<ERTMatchPhase> FasiAdiacenti = PhasesInTrace(Adiacente);
		OutTraceIndex = Adiacente;
		OutPos = MakePosition(Adiacente, Direction > 0 ? FasiAdiacenti[0] : FasiAdiacenti.Last());
		return true;
	}

	// Non c'e' un'altra traccia: si esce dalla sequenza dal lato in cui si stava andando. Sono i due
	// estremi, e sono posizioni — e' oltre di essi che il comando si disabilita.
	OutTraceIndex = INDEX_NONE;
	OutPos = FRTReplayPosition();
	OutPos.State = Direction > 0 ? ERTReplayPositionState::Ended : ERTReplayPositionState::BeforeStart;
	return true;
}

bool FRTReplayViewModel::NextTurnPosition(int32 Direction, FRTReplayPosition& OutPos,
	int32& OutTraceIndex) const
{
	if (!bOpen || Direction == 0)
	{
		return false;
	}

	// Il punto di partenza della ricerca dipende da dove si e': fuori dalla sequenza si parte dall'estremo
	// corrispondente, dentro dalla traccia corrente.
	int32 Da = CurrentTraceIndex;
	if (Current.State == ERTReplayPositionState::BeforeStart)
	{
		if (Direction < 0) { return false; }
		Da = INDEX_NONE;
	}
	else if (Current.State == ERTReplayPositionState::Ended)
	{
		if (Direction > 0) { return false; }
		Da = Session.Traces.Num();
	}

	const int32 Adiacente = AdjacentNonEmptyTrace(Da, Direction);
	if (Adiacente == INDEX_NONE)
	{
		return false;
	}

	// Sempre la PRIMA fase del turno di destinazione, in entrambe le direzioni: «turno precedente» porta
	// all'inizio di quel turno, non alla sua fine. E' la stessa semantica di `SeekToTurn`, che posiziona
	// `EntryIndex = 0` — un secondo comportamento qui sarebbe il secondo meccanismo che #472 vieta.
	OutTraceIndex = Adiacente;
	OutPos = MakePosition(Adiacente, PhasesInTrace(Adiacente)[0]);
	return true;
}

bool FRTReplayViewModel::CanStepPhaseForward() const
{
	FRTReplayPosition Ignorata;
	int32 IgnoratoIndice = INDEX_NONE;
	return NextPhasePosition(+1, Ignorata, IgnoratoIndice);
}

bool FRTReplayViewModel::CanStepPhaseBackward() const
{
	FRTReplayPosition Ignorata;
	int32 IgnoratoIndice = INDEX_NONE;
	return NextPhasePosition(-1, Ignorata, IgnoratoIndice);
}

bool FRTReplayViewModel::CanStepTurnForward() const
{
	FRTReplayPosition Ignorata;
	int32 IgnoratoIndice = INDEX_NONE;
	return NextTurnPosition(+1, Ignorata, IgnoratoIndice);
}

bool FRTReplayViewModel::CanStepTurnBackward() const
{
	FRTReplayPosition Ignorata;
	int32 IgnoratoIndice = INDEX_NONE;
	return NextTurnPosition(-1, Ignorata, IgnoratoIndice);
}

bool FRTReplayViewModel::StepPhaseForward()
{
	FRTReplayPosition Nuova;
	int32 NuovoIndice = INDEX_NONE;
	if (!NextPhasePosition(+1, Nuova, NuovoIndice))
	{
		return false;
	}
	ApplyPosition(Nuova, NuovoIndice);
	return true;
}

bool FRTReplayViewModel::StepPhaseBackward()
{
	FRTReplayPosition Nuova;
	int32 NuovoIndice = INDEX_NONE;
	if (!NextPhasePosition(-1, Nuova, NuovoIndice))
	{
		return false;
	}
	ApplyPosition(Nuova, NuovoIndice);
	return true;
}

bool FRTReplayViewModel::StepTurnForward()
{
	FRTReplayPosition Nuova;
	int32 NuovoIndice = INDEX_NONE;
	if (!NextTurnPosition(+1, Nuova, NuovoIndice))
	{
		return false;
	}
	ApplyPosition(Nuova, NuovoIndice);
	return true;
}

bool FRTReplayViewModel::StepTurnBackward()
{
	FRTReplayPosition Nuova;
	int32 NuovoIndice = INDEX_NONE;
	if (!NextTurnPosition(-1, Nuova, NuovoIndice))
	{
		return false;
	}
	ApplyPosition(Nuova, NuovoIndice);
	return true;
}

ERTReplaySeekResult FRTReplayViewModel::SeekToTurn(int32 TurnNumber)
{
	if (!bOpen)
	{
		return ERTReplaySeekResult::TurnNotFound;
	}

	// Il seek di `#415`, non una ricerca scritta qui: e' il criterio «nessun secondo meccanismo di
	// posizionamento». Il cursore che ne esce serve solo a sapere QUALE traccia — la posizione da mostrare
	// la costruisce `MakePosition`, perche' il cursore non porta ne' il turno corrente ne' la fase.
	FRTReplayCursor Cursore;
	const ERTReplaySeekResult Esito = URTReplaySeekLibrary::SeekToTurn(Session.Traces, TurnNumber, Cursore);
	if (Esito != ERTReplaySeekResult::Found)
	{
		return Esito; // fail-closed: la posizione corrente non si muove
	}

	const TArray<ERTMatchPhase> Fasi = PhasesInTrace(Cursore.TraceIndex);
	if (Fasi.Num() == 0)
	{
		// La traccia esiste e dichiara il turno, ma non ha nessuna fase osservabile. Non e' `TurnNotFound`:
		// il turno c'e'. Non e' nemmeno una posizione, e dirlo con l'esito che la fase non c'e' e' l'unica
		// risposta che non mente.
		return ERTReplaySeekResult::PhaseNotFound;
	}

	ApplyPosition(MakePosition(Cursore.TraceIndex, Fasi[0]), Cursore.TraceIndex);
	return ERTReplaySeekResult::Found;
}

ERTReplaySeekResult FRTReplayViewModel::SeekToPhaseInCurrentTurn(ERTMatchPhase Phase)
{
	if (!bOpen || !Session.Traces.IsValidIndex(CurrentTraceIndex))
	{
		// Fuori dalla sequenza non esiste un «turno corrente» dentro cui cercare: `BeforeStart` e `Ended`
		// finiscono qui, ed e' corretto — la domanda non si applica.
		return ERTReplaySeekResult::TurnNotFound;
	}

	int32 Indice = 0;
	const ERTReplaySeekResult Esito =
		URTReplaySeekLibrary::SeekToPhase(Session.Traces[CurrentTraceIndex], Phase, Indice);
	if (Esito != ERTReplaySeekResult::Found)
	{
		return Esito;
	}

	ApplyPosition(MakePosition(CurrentTraceIndex, Phase), CurrentTraceIndex);
	return ERTReplaySeekResult::Found;
}

void FRTReplayViewModel::Play()
{
	// Su una sequenza finita `Play` non parte: un pulsante che si accende senza che nulla si muova e' il
	// difetto che il criterio dei bordi vieta, e vale anche per il play.
	if (!bOpen || !CanStepPhaseForward())
	{
		return;
	}
	bPlaying = true;
}

void FRTReplayViewModel::Pause()
{
	bPlaying = false;
}

bool FRTReplayViewModel::Tick(float DeltaSeconds)
{
	if (!bOpen || !bPlaying || DeltaSeconds <= 0.f)
	{
		return false;
	}

	PhaseElapsed += DeltaSeconds;
	if (SecondsPerPhase > 0.f && PhaseElapsed < SecondsPerPhase)
	{
		return false;
	}

	// ⚠️ **Una** fase per chiamata, anche se il delta ne conterrebbe tre. Consumare l'arretrato farebbe
	// dipendere dal frame rate quante fasi il giocatore vede passare — cioe' esattamente il tipo di
	// dipendenza dal tempo reale che il progetto tiene fuori dalle decisioni.
	const bool bMosso = StepPhaseForward(); // azzera `PhaseElapsed` tramite `ApplyPosition`
	if (!bMosso || Current.State == ERTReplayPositionState::Ended)
	{
		bPlaying = false;
	}
	return bMosso;
}
