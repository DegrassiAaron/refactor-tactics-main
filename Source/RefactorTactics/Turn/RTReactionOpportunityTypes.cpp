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

FName URTReactionOpportunityLibrary::TargetHealthAtOrBelowPercent()
{
	static const FName Id(TEXT("TargetHealthAtOrBelowPercent"));
	return Id;
}

bool URTReactionOpportunityLibrary::IsDeclaredConditionAllowed(const FRTDeclaredCondition& Condition)
{
	// Elenco CHIUSO e nel codice: la v0.1 ne ammette una sola ([D-109]). Un id assente — compreso `NAME_None`,
	// cioe' «nessuna condizione» — non e' una condizione valida: chi non dichiara non passa di qui.
	static const TSet<FName> Allowed = { TargetHealthAtOrBelowPercent() };
	if (!Allowed.Contains(Condition.Id))
	{
		return false;
	}

	// Percentuale INTERA. `100` resta ammessa: e' «spara comunque», cioe' l'assenza di restrizione detta come
	// condizione, e vietarla costringerebbe a cancellare la condizione per ottenere lo stesso effetto. Oltre il
	// 100 sarebbe sempre vera — una condizione che non condiziona, peggio di nessuna condizione.
	return Condition.Param >= 0 && Condition.Param <= 100;
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

int32 URTReactionOpportunityLibrary::FireResponseTarget(const FString& Response)
{
	// Il prefisso si ricava dalla funzione che lo SCRIVE, non da una seconda costante: `FireResponse(0)` da'
	// «FIRE:0», e togliere l'ultimo carattere lascia «FIRE:». Con una `TEXT("FIRE:")` scritta qui, cambiare il
	// formato in un posto solo lo spezzerebbe nell'altro senza che niente smetta di compilare.
	static const FString Prefix = FireResponse(0).LeftChop(1);

	if (!Response.StartsWith(Prefix, ESearchCase::CaseSensitive))
	{
		return INDEX_NONE; // `HOLD`, o qualunque altra cosa: non c'e' un bersaglio, e non e' un errore
	}

	const FString Digits = Response.RightChop(Prefix.Len());
	if (Digits.IsEmpty() || !Digits.IsNumeric())
	{
		// `FIRE:` senza numero, o con qualcosa che numero non e'. `Atoi` darebbe 0, che e' un UnitId valido:
		// sarebbe una risposta malformata che spara alla prima unita' invece di essere rifiutata.
		return INDEX_NONE;
	}

	return FCString::Atoi(*Digits);
}

bool URTReactionOpportunityLibrary::IsResponseAllowed(const FRTReactionOpportunity& Opportunity,
	const FString& Response)
{
	// Confronto ESATTO e case-sensitive: `AllowedResponses` e' un elenco chiuso costruito dal produttore, e
	// una tolleranza qui — trim, case-insensitive, prefisso — significherebbe accettare risposte che quel
	// produttore non ha mai offerto.
	return Opportunity.AllowedResponses.ContainsByPredicate([&Response](const FString& Allowed)
	{
		return Allowed.Equals(Response, ESearchCase::CaseSensitive);
	});
}

int32 URTReactionOpportunityLibrary::CountParticipantsWithChoice(const FRTContestedBoundary& Boundary)
{
	int32 WithChoice = 0;
	for (const FRTReactionParticipant& P : Boundary.Participants)
	{
		// La stessa domanda che si fa per il single-responder, e non una seconda regola: «questo partecipante
		// ha davvero una scelta?». Riusarla e' cio' che tiene UNA sola definizione di cardinalita' nel gioco.
		if (RequiresDecisionBoundary(P.Opportunity))
		{
			++WithChoice;
		}
	}
	return WithChoice;
}

bool URTReactionOpportunityLibrary::IsContested(const FRTContestedBoundary& Boundary)
{
	// DUE, non «almeno uno»: §3.2 e' esplicita in negativo — se una delle due risposte e' di fatto obbligata,
	// l'opportunity NON e' contested e si degrada a single-responder. Senza questo `>= 2` un attacco base
	// diventerebbe un Clash appena qualcuno arma un `Brace`, che e' il caso che la spec esclude per primo.
	return CountParticipantsWithChoice(Boundary) >= 2;
}

void URTReactionOpportunityLibrary::SortParticipantsCanonically(FRTContestedBoundary& Boundary)
{
	// `StableSort` e non `Sort`: a parita' di criterio l'ordine dev'essere quello d'ingresso e non uno
	// qualunque, o due esecuzioni identiche potrebbero registrare i due lock in ordine diverso — che e'
	// precisamente la dipendenza dal tempo reale che §7.3 esiste per escludere.
	Boundary.Participants.StableSort([](const FRTReactionParticipant& A, const FRTReactionParticipant& B)
	{
		return A.UnitId < B.UnitId;
	});
}

int32 URTReactionOpportunityLibrary::CompareGrammarIntents(ERTGrammarIntent A, ERTGrammarIntent B)
{
	if (A == B)
	{
		return 0; // pareggio: intenzioni uguali. §5 dice che il Tie non e' un fallimento, e si applica UNA volta
	}

	// Il ciclo, scritto per intero invece che con aritmetica modulare: `(A - B + 3) % 3` darebbe la stessa
	// risposta e non direbbe QUALE regola sta applicando. Tre righe leggibili valgono piu' di una furba, e il
	// giorno in cui il playtest cambia una relazione si cambia la riga che la porta.
	const bool bAWins =
		(A == ERTGrammarIntent::Read  && B == ERTGrammarIntent::Stand) ||
		(A == ERTGrammarIntent::Stand && B == ERTGrammarIntent::Shift) ||
		(A == ERTGrammarIntent::Shift && B == ERTGrammarIntent::Read);

	return bAWins ? 1 : -1;
}

FString URTReactionOpportunityLibrary::SafeResponse(const FRTReactionOpportunity& Opportunity)
{
	// `HOLD` PRIMA di tutto, e l'ordine dei due rami e' la regola: e' cio' che garantisce che una finestra
	// scaduta non spari mai. Cercarla invece di prendere la prima e' necessario perche' nell'Overwatch `HOLD`
	// e' in CODA, dopo i `FIRE:` — «la prima risposta» li' sarebbe uno sparo.
	if (IsResponseAllowed(Opportunity, HoldResponse()))
	{
		return HoldResponse();
	}

	// Nessun `HOLD` nel vocabolario: e' il `Brace`, che chiama la propria scelta sicura `Hold Ground` e la
	// tiene in testa proprio per questo ([D-047] §2.1, e il commento di `BraceAllowedResponses` lo dichiara).
	// ⚠️ La responsabilita' si sposta su chi COSTRUISCE le risposte: un produttore futuro che mettesse in
	// testa una risposta costosa la renderebbe l'esito di ogni scadenza. E' il vincolo che sostituisce la
	// vecchia costante, e va detto qui perche' e' l'unico posto da cui si vede.
	if (Opportunity.AllowedResponses.Num() > 0)
	{
		return Opportunity.AllowedResponses[0];
	}

	// Zero risposte: non c'e' una prima da prendere. Non e' un caso da gestire con un'invenzione — con zero
	// risposte `RequiresDecisionBoundary` e' falso e quella finestra non si e' mai aperta.
	return HoldResponse();
}

FRTReactionDecision URTReactionOpportunityLibrary::DecisionOnTimeout(const FRTReactionOpportunity& Opportunity)
{
	// PURA: lo scadere applica la scelta sicura, che per ogni finestra dell'Overwatch e' `HOLD` — mai `FIRE`,
	// perche' consuma una risorsa irreversibile e un input mancato non deve spenderla (ADR-0004 §3). La
	// garanzia sta in `SafeResponse`, non in una costante scritta qui.
	return FRTReactionDecision(SafeResponse(Opportunity), ERTReactionDecisionOutcome::HoldTimeout);
}

bool URTReactionOpportunityLibrary::IsConditionSatisfied(const FRTDeclaredCondition& Condition,
	const FRTTargetVitals* Vitals)
{
	if (!Condition.IsDeclared())
	{
		return true; // chi non pone condizioni non restringe nulla: e' il comportamento di sempre
	}

	// Una condizione che il gioco non conosce non e' valutabile, quindi non autorizza niente. Puo' arrivare
	// qui da un piano salvato prima che il catalogo cambiasse: trattarla come vera cambierebbe l'esito della
	// partita in silenzio, e in una direzione che il giocatore non ha chiesto.
	if (!IsDeclaredConditionAllowed(Condition))
	{
		return false;
	}

	// Dato mancante o incoerente: stessa scelta fail-closed di `TeamAwareness`. Offrire `FIRE` qui
	// significherebbe sparare su una condizione che nessuno ha verificato.
	if (!Vitals || Vitals->MaxHealth <= 0)
	{
		return false;
	}

	// `Health / MaxHealth <= Param / 100` senza divisioni: aritmetica INTERA, perche' `G7` vieta i float e
	// una soglia in virgola mobile li farebbe rientrare proprio nel punto che decide chi viene colpito.
	return Vitals->Health * 100 <= Vitals->MaxHealth * Condition.Param;
}

TArray<FRTOverwatchTrigger> URTReactionOpportunityLibrary::BuildOverwatchTriggers(const URTHexMapAsset* Map,
	int32 TurnNumber, const TArray<FRTOverwatchWatcher>& Watchers, const TArray<FRTSuppressionMover>& Movers,
	const TMap<int32, FRTTargetVitals>& TargetVitals, int32 FirstMicroStepIndex)
{
	TArray<FRTOverwatchTrigger> Triggers;

	// Fail-closed: la LOS e' una delle quattro condizioni, e senza mappa autorevole non e' calcolabile. Un
	// `Map` nullo non deve produrre trigger «per difetto» — sarebbe un Overwatch che spara al buio.
	if (!Map)
	{
		return Triggers;
	}

	/**
	 * I cinque tie-break di ADR-0004 §4 viaggiano ACCANTO al trigger mentre lo si ordina, e non dentro.
	 *
	 * ⚠️ La prima stesura li ritrovava con una `TMap<OwnerId, const FRTOverwatchWatcher*>`, ed era un difetto:
	 * un'unita' puo' avere **due reaction armate** — e' esattamente il caso che ADR-0004 §4 chiama «piu'
	 * reazioni distinte nello stesso micro-step». Con la mappa, la seconda `Add` sovrascriveva la prima, e
	 * ENTRAMBI i trigger si ordinavano coi tie-break dell'ULTIMO watcher inserito: cioe' in base all'ordine
	 * dell'array `Watchers`, che e' precisamente la dipendenza che questo ordinamento esiste per togliere.
	 */
	struct FPendingOverwatchTrigger
	{
		FRTOverwatchTrigger Trigger;
		int32 ReactionPriority = 0;
		int32 AbilityPriority = 0;
		int32 UnitInitiative = 0;
		int32 StableUnitId = INDEX_NONE;
		int32 ReactionInstanceId = INDEX_NONE;
	};
	TArray<FPendingOverwatchTrigger> Pending;

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
			// `Step` e' relativo ai `Movers` ricevuti; l'indice della CHIAVE e' quello del turno. Coincidono
			// solo quando il chiamante passa i percorsi interi — vedi `FirstMicroStepIndex` nell'header.
			Trigger.Opportunity.Key.MicroStepIndex = FirstMicroStepIndex + Step;
			Trigger.Opportunity.Key.OwnerId = Watcher.Zone.OwnerUnitId;
			Trigger.Opportunity.Key.ReactionDefId = Watcher.ReactionDefId;
			// `Seq` NON si assegna qui: dipenderebbe dall'ordine di `Watchers`. Si assegna dopo
			// l'ordinamento totale, scorrendo il risultato — vedi in fondo.

			// La condizione dichiarata in pianificazione RIDUCE le risposte legali ([D-012], [D-109]). Filtra
			// le risposte, non i bersagli: il trigger e' scattato davvero — quelle unita' sono entrate nella
			// zona e sono state rilevate — e `TargetUnitIds` continua a dire chi e' passato. Se la riduzione
			// lascia il solo `HOLD`, `RequiresDecisionBoundary` diventa falso e il commit e' immediato: e' cosi'
			// che il regime *Conditional* emerge dai dati, senza un enum di policy parallelo.
			for (int32 TargetId : TargetsThisStep)
			{
				if (!IsConditionSatisfied(Watcher.DeclaredCondition, TargetVitals.Find(TargetId)))
				{
					continue;
				}
				Trigger.Opportunity.AllowedResponses.Add(FireResponse(TargetId));
			}
			// `HOLD` in coda ed e' SEMPRE presente: senza di lei un bersaglio solo darebbe cardinalita' 1,
			// cioe' un commit automatico — l'Overwatch sparerebbe da solo. La scelta di non sparare e' una
			// risposta legale quanto le altre (ADR-0004 §3: `Timeout -> HOLD`).
			Trigger.Opportunity.AllowedResponses.Add(HoldResponse());

			FPendingOverwatchTrigger P;
			P.Trigger = MoveTemp(Trigger);
			P.ReactionPriority   = Watcher.ReactionPriority;
			P.AbilityPriority    = Watcher.AbilityPriority;
			P.UnitInitiative     = Watcher.UnitInitiative;
			P.StableUnitId       = Watcher.StableUnitId;
			P.ReactionInstanceId = Watcher.ReactionInstanceId;
			Pending.Add(MoveTemp(P));
		}
	}

	// Ordine TOTALE fra reazioni diverse (ADR-0004 §4). Il micro-step viene per primo perche' e' il tempo
	// della risoluzione; i cinque criteri seguono nell'ordine dell'ADR, e l'ultimo — `ReactionInstanceId` —
	// e' quello che garantisce che due elementi non restino mai indistinguibili. Se restassero, a deciderli
	// sarebbe l'ordine di `Watchers`, cioe' il caso.
	Pending.Sort([](const FPendingOverwatchTrigger& A, const FPendingOverwatchTrigger& B)
	{
		if (A.Trigger.Opportunity.Key.MicroStepIndex != B.Trigger.Opportunity.Key.MicroStepIndex)
		{
			return A.Trigger.Opportunity.Key.MicroStepIndex < B.Trigger.Opportunity.Key.MicroStepIndex;
		}
		if (A.ReactionPriority != B.ReactionPriority)     { return A.ReactionPriority < B.ReactionPriority; }
		if (A.AbilityPriority != B.AbilityPriority)       { return A.AbilityPriority < B.AbilityPriority; }
		if (A.UnitInitiative != B.UnitInitiative)         { return A.UnitInitiative < B.UnitInitiative; }
		if (A.StableUnitId != B.StableUnitId)             { return A.StableUnitId < B.StableUnitId; }
		return A.ReactionInstanceId < B.ReactionInstanceId;
	});

	/**
	 * `Seq` si assegna QUI, sul risultato gia' ordinato, e non durante la costruzione.
	 *
	 * ⚠️ La prima stesura lo lasciava a `0` con la motivazione «un watcher apre al massimo una opportunity per
	 * micro-step». Vera, e irrilevante: la chiave non identifica il WATCHER, identifica
	 * `(Turn, MacroPhase, MicroStep, OwnerId, ReactionDefId, Seq)`. Due reaction della **stessa unita'** con lo
	 * **stesso** `ReactionDefId` — due istanze, che e' il motivo per cui `ReactionInstanceId` esiste —
	 * producevano due chiavi IDENTICHE, quindi lo stesso `OpportunityId`: il replay avrebbe attribuito a una
	 * la decisione dell'altra. E' il difetto che CP 14.3 esiste per impedire, e la doc di `Seq` lo nomina
	 * parola per parola: «la stessa unita', la stessa reaction, lo stesso micro-step, due volte».
	 *
	 * Assegnarlo DOPO l'ordinamento e' cio' che lo rende una funzione dello stato: sull'array non ordinato
	 * dipenderebbe da come il chiamante ha costruito `Watchers`.
	 */
	Triggers.Reserve(Pending.Num());
	for (int32 I = 0; I < Pending.Num(); ++I)
	{
		const FRTReactionOpportunityKey& Cur = Pending[I].Trigger.Opportunity.Key;
		int32 Seq = 0;
		for (int32 J = 0; J < I; ++J)
		{
			const FRTReactionOpportunityKey& Prev = Pending[J].Trigger.Opportunity.Key;
			if (Prev.TurnNumber == Cur.TurnNumber
				&& Prev.MacroPhase == Cur.MacroPhase
				&& Prev.MicroStepIndex == Cur.MicroStepIndex
				&& Prev.OwnerId == Cur.OwnerId
				&& Prev.ReactionDefId == Cur.ReactionDefId)
			{
				++Seq; // `Seq` escluso dal confronto: e' il campo che si sta assegnando
			}
		}
		Pending[I].Trigger.Opportunity.Key.Seq = Seq;
		Triggers.Add(MoveTemp(Pending[I].Trigger));
	}

	return Triggers;
}
