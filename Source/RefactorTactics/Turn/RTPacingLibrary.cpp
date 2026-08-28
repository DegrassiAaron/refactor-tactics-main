#include "Turn/RTPacingLibrary.h"

int32 URTPacingLibrary::PercentileNearestRank(const TArray<int32>& SortedValues, int32 Percentile)
{
	if (SortedValues.Num() == 0)
	{
		return 0;
	}
	const int32 P = FMath::Clamp(Percentile, 1, 100);
	// Rango 1-based = ceil(P/100 * N), su interi per non passare mai da un float.
	const int32 Rank = FMath::DivideAndRoundUp(P * SortedValues.Num(), 100);
	return SortedValues[FMath::Clamp(Rank - 1, 0, SortedValues.Num() - 1)];
}

FRTPacingSummary URTPacingLibrary::SummarizeSamples(const TArray<FRTPacingSample>& Samples, int32 CutoffWindowMs)
{
	FRTPacingSummary Out;
	Out.SampleCount = Samples.Num();
	if (Samples.Num() == 0)
	{
		return Out; // fail-closed: nessun campione -> tutto zero
	}

	TArray<int32> LockIn;
	TArray<int32> Playback;
	LockIn.Reserve(Samples.Num());
	Playback.Reserve(Samples.Num());

	for (const FRTPacingSample& S : Samples)
	{
		// Il playback ha un cronometro suo (`PlaybackElapsedTotal`) e non dipende dall'apertura del
		// campione: vale anche quando i tempi di PIANIFICAZIONE non sono stati misurati. Per lo stesso
		// motivo `bPlaybackSkipped` si conta qui, prima di ogni esclusione: un playback saltato e' un fatto
		// osservato, e contarlo solo per i campioni cronometrati direbbe «saltati 0» su una sessione che ne
		// aveva.
		Playback.Add(S.MsPlayback);
		if (S.bPlaybackSkipped)
		{
			++Out.SkippedPlaybacks;
		}

		// Le finestre si sommano PRIMA di ogni esclusione, per la stessa ragione del playback: sono un fatto
		// osservato del turno e non dipendono dal cronometro della pianificazione. Un turno senza tempi
		// misurati — ogni run headless — ha comunque aperto le finestre che ha aperto.
		Out.TotalReactionWindows += S.ReactionWindowsOpened;

		// ⚠️ Un campione NON MISURATO non e' un lock-in rapido: e' l'assenza di una misura, e va tolto da
		// ogni statistica che risponde «quanto tempo». La sentinella e' negativa apposta, ma escluderla non
		// e' automatico — `Unmeasured < CutoffWindowMs` e' sempre vero, quindi senza questa guardia OGNI
		// timeout non misurato si classificherebbe come taglio del timer, che e' il segnale piu' forte che
		// questo sommario produce (`#1421`).
		if (S.MsToLockIn == FRTPacingSample::Unmeasured)
		{
			++Out.UnmeasuredSamples;
			continue;
		}
		LockIn.Add(S.MsToLockIn);

		if (S.LockInSource == ERTLockInSource::Timeout)
		{
			// ⚠️ La guardia va rifatta su QUESTO campo, non basta quella su `MsToLockIn`: e' `MsSinceLastInput`
			// a decidere la classificazione, e i due possono divergere — un campione ricaricato da CSV, o un
			// percorso futuro che cronometra il lock-in e perde l'origine dell'ultimo input. Non classificabile
			// non e' «taglio»: e' non classificabile, e resta fuori da entrambi i conteggi.
			if (S.MsSinceLastInput != FRTPacingSample::Unmeasured)
			{
				// Sotto soglia = il giocatore stava ancora agendo -> taglio. Dalla soglia in su -> attesa a vuoto.
				(S.MsSinceLastInput < CutoffWindowMs ? Out.TrueCutoffs : Out.IdleTimeouts)++;
			}
		}
	}

	LockIn.Sort();
	Playback.Sort();
	// ⚠️ Nessun campione cronometrato non fa «mediana zero»: zero e' un lock-in istantaneo, cioe' il valore
	// legittimo da cui `Unmeasured` esiste per distinguersi. Farlo tornare qui rimetterebbe al livello del
	// sommario lo stesso dato plausibile e falso che la sentinella toglie dal campione.
	Out.MedianMsToLockIn = LockIn.Num() > 0
		? PercentileNearestRank(LockIn, 50) : FRTPacingSample::Unmeasured;
	Out.P90MsToLockIn = LockIn.Num() > 0
		? PercentileNearestRank(LockIn, 90) : FRTPacingSample::Unmeasured;
	Out.MedianMsPlayback = PercentileNearestRank(Playback, 50);
	return Out;
}

int32 URTPacingLibrary::CountOpenedReactionWindows(const TArray<FRTTurnLogEntry>& Entries,
	const TSet<int32>& ResponderUnitIds)
{
	int32 Opened = 0;
	for (const FRTTurnLogEntry& Entry : Entries)
	{
		if (Entry.Category != ERTLogCategory::ReactionDecision)
		{
			continue;
		}
		if (!ResponderUnitIds.Contains(Entry.UnitId))
		{
			continue;
		}

		// 🔴 **ELENCO POSITIVO, non «tutto tranne».** Un `default` che conta gonfierebbe la baseline al primo
		// esito aggiunto all'enum — e `ERTReactionDecisionOutcome` **prescrive** che i valori nuovi si
		// aggiungano in coda, quindi non e' un'ipotesi: `ResponseChosen` e `HoldCollapsedByCondition` sono
		// arrivati esattamente cosi'. Un secondo motivo di collasso, o una opportunity annullata, cadrebbero
		// nel ramo che conta senza che nessun test diventi rosso. Qui il caso nuovo non conta **finche'
		// qualcuno non lo dichiara**, e lo `switch` senza `default` lo fa dire al compilatore.
		//
		// `Outcome` e' un `uint8` condiviso da tutte le categorie: il cast e' legittimo SOLO dopo il filtro
		// sulla categoria qui sopra, che e' la ragione per cui i due controlli sono in quest'ordine.
		const ERTReactionDecisionOutcome Outcome = static_cast<ERTReactionDecisionOutcome>(Entry.Outcome);
		bool bOccupiedSomeone = false;
		switch (Outcome)
		{
		case ERTReactionDecisionOutcome::FireChosen:
		case ERTReactionDecisionOutcome::HoldChosen:
		case ERTReactionDecisionOutcome::HoldTimeout:
		case ERTReactionDecisionOutcome::HoldRejected:
		case ERTReactionDecisionOutcome::ResponseChosen:
			// Qualcuno ha ricevuto la domanda e ha risposto, o ha lasciato scadere: in tutti e cinque i casi
			// la finestra ha occupato il suo tempo. `HoldRejected` incluso — la risposta e' arrivata, e il
			// fatto che fosse illegale non restituisce i secondi spesi a deciderla.
			bOccupiedSomeone = true;
			break;

		case ERTReactionDecisionOutcome::HoldNoDecider:
			// 🔴 **Esiste la finestra, non l'attesa.** `AskReactionDecision` restituisce questo esito per
			// *«un'unita' umana senza UI: la finestra esiste e nessuno puo' rispondere»* — e la UI **e'**
			// CP 14.6, cioe' oggi non c'e'. Contarlo scriverebbe `3,0 s` di attesa per ogni finestra di una
			// persona che non e' mai stata interpellata: la stessa inflazione per cui `HoldImmediate` e'
			// escluso, applicata all'esito che la rende sistematica invece che occasionale.
			//
			// ➕ **Rientrera' da solo** quando la UI di DIR-A atterrera': quelle finestre diventeranno
			// `HoldChosen` o `HoldTimeout`, che stanno nel ramo sopra. Nessuno dovra' ricordarsi di
			// aggiornare questa funzione.
			break;

		case ERTReactionDecisionOutcome::HoldImmediate:
		case ERTReactionDecisionOutcome::HoldCollapsedByCondition:
			// Commit immediati: nessuna finestra si e' aperta, nessun secondo speso. I due esiti sono
			// separati apposta — «non c'era scelta» e «la condizione dichiarata l'ha tolta» sono meccanismi
			// diversi — e qui contano allo stesso modo: zero.
			break;
		}

		if (bOccupiedSomeone)
		{
			++Opened;
		}
	}

	return Opened;
}

float URTPacingLibrary::ReactionDecisionSecondsUpperBound(int32 OpenedWindows, float WindowSeconds)
{
	if (OpenedWindows <= 0 || WindowSeconds <= 0.f)
	{
		return 0.f;
	}
	return static_cast<float>(OpenedWindows) * WindowSeconds;
}

FString URTPacingLibrary::CsvHeader()
{
	// ⚠️ `ReactionWindows` in CODA e non in mezzo: le colonne si leggono per posizione da fogli e script gia'
	// scritti, e inserirla prima sposterebbe ogni colonna a valle senza che nessun errore lo dica.
	return TEXT("Turn,AliveT0,AliveT1,ActionsAvailable,MsToFirstInput,SelectionCount,OrderCount,")
		   TEXT("UndoCount,MsToLockIn,MsSinceLastInput,LockInSource,MsPlayback,PlaybackSkipped,")
		   TEXT("ReactionWindows");
}

FString URTPacingLibrary::CsvRow(const FRTPacingSample& Sample)
{
	// Tutti %d: nessun float, quindi nessuna virgola decimale da locale che spezzi le colonne.
	return FString::Printf(TEXT("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d"),
		Sample.TurnNumber,
		Sample.UnitsAliveTeam0,
		Sample.UnitsAliveTeam1,
		Sample.ActionsAvailable,
		Sample.MsToFirstInput,
		Sample.SelectionCount,
		Sample.OrderCount,
		Sample.UndoCount,
		Sample.MsToLockIn,
		Sample.MsSinceLastInput,
		static_cast<int32>(Sample.LockInSource),
		Sample.MsPlayback,
		Sample.bPlaybackSkipped ? 1 : 0,
		Sample.ReactionWindowsOpened);
}
