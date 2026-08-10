#include "Turn/RTReactionOpportunityTypes.h"

#include "Map/RTHexVisionLibrary.h"

FString URTReactionOpportunityLibrary::DeriveOpportunityId(const FRTReactionOpportunityKey& Key)
{
	// Il separatore e' `|` e non il punto della notazione `Turn.MacroPhase.MicroStep.OwnerId.ReactionDefId.Seq`
	// del checkpoint: quella nomina i CAMPI, non il carattere. Il punto compare gia' dentro `ReactionDefId`
	// (`Action.Counter`), quindi userebbe lo stesso separatore dei campi e renderebbe possibile una coppia di
	// chiavi diverse con lo stesso id. Costa un carattere evitarlo.
	//
	// `MacroPhase` entra come intero. `ERTMatchPhase` e' l'enum delle sei macro-fasi del turno, che il
	// progetto non riordina — l'ordine E' la sequenza del turno, quindi cambiarlo sarebbe una modifica alle
	// regole prima che al formato. Un valore nuovo va comunque aggiunto in CODA: gli id gia' registrati non
	// devono cambiare significato.
	//
	// `ReactionDefId` viene normalizzato a minuscole. ⚠️ **In questa build e' un no-op, misurato**: una code
	// review aveva segnalato che `FName::ToString()` restituisce la casing dell'ISTANZA — quindi due punti del
	// codice che l'engine considera la stessa azione avrebbero prodotto due id diversi. Verificato: qui
	// `ToString()` restituisce gia' minuscolo, e un test costruito per dimostrare la divergenza non e' riuscito
	// nemmeno a costruirne la premessa (`ToString() != ToString().ToLower()` era falso). Il test e' stato
	// rimosso invece che tenuto verde: non poteva fallire.
	//
	// La chiamata resta perche' e' gratis e la premessa della review tornerebbe VERA con
	// `WITH_CASE_PRESERVING_NAME` attivo — e in quel caso l'id dipenderebbe dalla casing del primo chiamante,
	// cioe' dall'ordine di caricamento. Non e' pinnata da un test, e va detto: in questa configurazione la
	// premessa non e' costruibile.
	return FString::Printf(TEXT("T%d|P%d|M%d|U%d|%s|S%d"),
		Key.TurnNumber,
		static_cast<int32>(Key.MacroPhase),
		Key.MicroStepIndex,
		Key.OwnerId,
		*Key.ReactionDefId.ToString().ToLower(),
		Key.Seq);
}

bool URTReactionOpportunityLibrary::RequiresDecisionBoundary(const FRTReactionOpportunity& Opportunity)
{
	// ADR-0004 §2, e nient'altro. La soglia e' `>= 2` e non `> 0`: una sola risposta legale non e' una scelta,
	// quindi non c'e' niente da chiedere e il commit e' immediato — e' il caso degenere in cui vivono le
	// reazioni E5 (`Counter`, `Deflect`, `Shield`, `Cleanse`, e `Brace` col suo profilo base).
	return Opportunity.AllowedResponses.Num() >= 2;
}

FString URTReactionOpportunityLibrary::FireResponse(int32 TargetUnitId)
{
	// Il bersaglio sta DENTRO la risposta e non in un campo parallelo: `AllowedResponses` e' gia' l'elenco
	// delle scelte legali, e una seconda lista di bersagli allineata per indice sarebbe una struttura che si
	// puo' disallineare. Qui `FIRE:7` significa una cosa sola.
	return FString::Printf(TEXT("FIRE:%d"), TargetUnitId);
}

TArray<FRTOverwatchTrigger> URTReactionOpportunityLibrary::BuildOverwatchTriggers(const URTHexMapAsset* Map,
	int32 TurnNumber, const TArray<FRTOverwatchWatcher>& Watchers, const TArray<FRTSuppressionMover>& Movers)
{
	TArray<FRTOverwatchTrigger> Triggers;

	// Fail-closed: la LOS e' una delle quattro condizioni, e senza mappa autorevole non e' calcolabile. Un
	// `Map` nullo non deve produrre trigger «per difetto» — sarebbe un Overwatch che spara al buio.
	if (!Map)
	{
		return Triggers;
	}

	for (const FRTOverwatchWatcher& Watcher : Watchers)
	{
		if (!Watcher.bArmed || Watcher.Zone.Cells.Num() == 0)
		{
			continue; // `ReactionStillArmed` falso, o una zona mai preparata: niente da controllare
		}

		// `TSet` per l'APPARTENENZA, mai per l'ordine (invariante #3): l'ordine di scansione e' quello dei
		// micro-step, che i percorsi dichiarano.
		const TSet<FRTCellId> Controlled(Watcher.Zone.Cells);

		// Quanti micro-step ha il piu' lungo dei percorsi: il trigger si valuta a OGNI passo, e il ciclo
		// esterno e' il passo — non il mover. E' cio' che rende «piu' bersagli nello stesso micro-step» una
		// sola opportunity invece di due, senza un secondo raggruppamento a valle.
		int32 MaxSteps = 0;
		for (const FRTSuppressionMover& Mover : Movers)
		{
			MaxSteps = FMath::Max(MaxSteps, Mover.Path.Num());
		}

		for (int32 Step = 0; Step < MaxSteps; ++Step)
		{
			TArray<int32> TargetsThisStep;

			for (const FRTSuppressionMover& Mover : Movers)
			{
				if (Mover.TeamId == Watcher.Zone.OwnerTeamId || Mover.UnitId == Watcher.Zone.OwnerUnitId)
				{
					continue; // l'Overwatch non scatta sui propri, come la linea di soppressione
				}
				if (!Mover.Path.IsValidIndex(Step))
				{
					continue; // percorso gia' finito: questa unita' non e' in movimento a questo passo
				}

				const FRTCellId& Cell = Mover.Path[Step];

				// 1) `TargetInsideArea` — la cella APPENA RAGGIUNTA, cioe' dopo l'ingresso valido: stessa
				//    convenzione di `ResolveSuppression`, dove la vittima resta nella cella in cui e' entrata.
				if (!Controlled.Contains(Cell))
				{
					continue;
				}

				// 2) `TargetDetected` — livello `Rilevato` e non «visibile» (ADR-0004 §6). Un contatto
				//    `Uncertain` — fumo oltre 2 celle, o solo rumore — non arma il trigger: e' informazione,
				//    non un bersaglio. `FindRef` su chiave assente da' `Hidden`, che e' il default giusto.
				if (Watcher.TeamAwareness.FindRef(Mover.UnitId) != ERTAwareness::Detected)
				{
					continue;
				}

				// 3) `HasLineOfSight` — ricontrollata sulla cella raggiunta, non sulla cella di partenza del
				//    turno: un bersaglio che entra nella zona dietro una copertura alta non e' ingaggiabile.
				if (!URTHexVisionLibrary::HasLineOfSight(Map, Watcher.OwnerCell, Cell))
				{
					continue;
				}

				TargetsThisStep.Add(Mover.UnitId);
			}

			if (TargetsThisStep.Num() == 0)
			{
				continue;
			}

			// Ordine dei bersagli DENTRO l'opportunity: crescente per `UnitId`. Serve a rendere
			// `AllowedResponses` una funzione dello stato — permutare `Movers` non deve riordinare le
			// risposte, o due esecuzioni dello stesso scenario darebbero due DTO diversi.
			TargetsThisStep.Sort();

			FRTOverwatchTrigger Trigger;
			Trigger.TargetUnitIds = TargetsThisStep;

			Trigger.Opportunity.Key.TurnNumber = TurnNumber;
			Trigger.Opportunity.Key.MacroPhase = ERTMatchPhase::Move; // l'Overwatch scatta sui micro-step del Move
			Trigger.Opportunity.Key.MicroStepIndex = Step;
			Trigger.Opportunity.Key.OwnerId = Watcher.Zone.OwnerUnitId;
			Trigger.Opportunity.Key.ReactionDefId = Watcher.ReactionDefId;
			Trigger.Opportunity.Key.Seq = 0; // un watcher apre al massimo una opportunity per micro-step

			for (int32 TargetId : TargetsThisStep)
			{
				Trigger.Opportunity.AllowedResponses.Add(FireResponse(TargetId));
			}
			// `HOLD` in coda ed e' SEMPRE presente: senza di lei un bersaglio solo darebbe cardinalita' 1,
			// cioe' un commit automatico — l'Overwatch sparerebbe da solo. La scelta di non sparare e' una
			// risposta legale quanto le altre (ADR-0004 §3: `Timeout -> HOLD`).
			Trigger.Opportunity.AllowedResponses.Add(HoldResponse());

			Triggers.Add(MoveTemp(Trigger));
		}
	}

	// Ordine TOTALE fra reazioni diverse (ADR-0004 §4). Il micro-step viene per primo perche' e' il tempo
	// della risoluzione; i cinque criteri seguono nell'ordine dell'ADR, e l'ultimo — `ReactionInstanceId` —
	// e' quello che garantisce che due elementi non restino mai indistinguibili. Se restassero, a deciderli
	// sarebbe l'ordine di `Watchers`, cioe' il caso.
	//
	// La mappa `WatcherByOwner` serve solo a ritrovare i tie-break del watcher a partire dal trigger: e'
	// letta per chiave, mai iterata (invariante #3).
	TMap<int32, const FRTOverwatchWatcher*> WatcherByOwner;
	for (const FRTOverwatchWatcher& Watcher : Watchers)
	{
		WatcherByOwner.Add(Watcher.Zone.OwnerUnitId, &Watcher);
	}

	Triggers.Sort([&WatcherByOwner](const FRTOverwatchTrigger& A, const FRTOverwatchTrigger& B)
	{
		if (A.Opportunity.Key.MicroStepIndex != B.Opportunity.Key.MicroStepIndex)
		{
			return A.Opportunity.Key.MicroStepIndex < B.Opportunity.Key.MicroStepIndex;
		}

		const FRTOverwatchWatcher* const* WA = WatcherByOwner.Find(A.Opportunity.Key.OwnerId);
		const FRTOverwatchWatcher* const* WB = WatcherByOwner.Find(B.Opportunity.Key.OwnerId);
		if (!WA || !WB)
		{
			return A.Opportunity.Key.OwnerId < B.Opportunity.Key.OwnerId;
		}

		const FRTOverwatchWatcher& X = **WA;
		const FRTOverwatchWatcher& Y = **WB;
		if (X.ReactionPriority != Y.ReactionPriority)   { return X.ReactionPriority < Y.ReactionPriority; }
		if (X.AbilityPriority != Y.AbilityPriority)     { return X.AbilityPriority < Y.AbilityPriority; }
		if (X.UnitInitiative != Y.UnitInitiative)       { return X.UnitInitiative < Y.UnitInitiative; }
		if (X.StableUnitId != Y.StableUnitId)           { return X.StableUnitId < Y.StableUnitId; }
		return X.ReactionInstanceId < Y.ReactionInstanceId;
	});

	return Triggers;
}
