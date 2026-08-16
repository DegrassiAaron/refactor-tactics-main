#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTActionFallbackLibrary.h" // ERTActionInvalidReason: il motivo del fallback, leggibile nel log
#include "Turn/RTReactionLibrary.h" // ERTReactionOutcome: l'esito di una reazione, leggibile nel log
#include "Misc/FileHelper.h"

bool URTTurnLogLibrary::EntryLess(const FRTTurnLogEntry& A, const FRTTurnLogEntry& B)
{
	// Ordine totale: confronta ogni campo in sequenza, cosi' due voci diverse hanno sempre un ordine definito
	// (permutare l'input e riordinare -> stessa sequenza). Enum confrontati per valore intero (invariante #4).
	if (A.Phase != B.Phase)                 { return static_cast<uint8>(A.Phase) < static_cast<uint8>(B.Phase); }
	if (A.Category != B.Category)           { return static_cast<uint8>(A.Category) < static_cast<uint8>(B.Category); }
	if (A.SrcCell.X != B.SrcCell.X)         { return A.SrcCell.X < B.SrcCell.X; }
	if (A.SrcCell.Y != B.SrcCell.Y)         { return A.SrcCell.Y < B.SrcCell.Y; }
	if (A.SrcCell.Layer != B.SrcCell.Layer) { return A.SrcCell.Layer < B.SrcCell.Layer; }
	if (A.TgtCell.X != B.TgtCell.X)         { return A.TgtCell.X < B.TgtCell.X; }
	if (A.TgtCell.Y != B.TgtCell.Y)         { return A.TgtCell.Y < B.TgtCell.Y; }
	if (A.TgtCell.Layer != B.TgtCell.Layer) { return A.TgtCell.Layer < B.TgtCell.Layer; }
	if (A.Outcome != B.Outcome)             { return A.Outcome < B.Outcome; }
	if (A.Amount != B.Amount)               { return A.Amount < B.Amount; }
	// Confronto LESSICOGRAFICO (`FName::Compare`), mai `FastLess`: quello ordina per indice nella name table,
	// che dipende dall'ordine in cui i nomi sono stati creati nel processo — due esecuzioni della stessa
	// partita darebbero due ordini diversi, cioe' due hash diversi (#4).
	if (A.ActionId != B.ActionId) { return A.ActionId.Compare(B.ActionId) < 0; }

	// I campi della v6 chiudono l'ordine. NON e' un dettaglio estetico: `SerializeTurnLog` li SCRIVE, e un
	// campo scritto che il confronto non guarda lascia due voci a pari merito — dove a decidere l'ordine
	// resta `TArray::Sort`, che non e' stabile. Due inserimenti diversi produrrebbero due file diversi con
	// lo stesso contenuto, cioe' esattamente cio' che `D-SR-1` promette non accada.
	//
	// `BaseActionId` resta fuori e non e' un'omissione: e' una FUNZIONE di `ActionId`, quindi due voci che
	// pareggiano su `ActionId` pareggiano anche su di lui e non c'e' niente da spareggiare.
	if (A.TurnNumber != B.TurnNumber)     { return A.TurnNumber < B.TurnNumber; }
	if (A.GraphRevision != B.GraphRevision) { return A.GraphRevision < B.GraphRevision; }
	if (A.UnitId != B.UnitId)             { return A.UnitId < B.UnitId; }
	// `Priority` (v7) chiude l'ordine per la stessa ragione dei tre campi qui sopra: e' SCRITTO da
	// `SerializeTurnLog`, e un campo scritto che il confronto non guarda lascia due voci a pari merito.
	if (A.Priority != B.Priority) { return A.Priority < B.Priority; }

	// I tre campi della v8, per la stessa ragione ancora: sono scritti, quindi devono spareggiare. Due
	// decisioni della **stessa** unita' nello stesso micro-step — due Overwatch armate — pareggiano su tutto
	// il resto e si distinguono solo qui.
	if (A.OpportunityId != B.OpportunityId) { return A.OpportunityId < B.OpportunityId; }
	if (A.ReactionInstanceId != B.ReactionInstanceId) { return A.ReactionInstanceId < B.ReactionInstanceId; }
	return A.SelectedTargetUnitId < B.SelectedTargetUnitId;
}

void URTTurnLogLibrary::SortTurnLog(TArray<FRTTurnLogEntry>& Entries)
{
	Entries.Sort([](const FRTTurnLogEntry& A, const FRTTurnLogEntry& B) { return EntryLess(A, B); });
}

FString URTTurnLogLibrary::DescribeActionIdentity(const FRTTurnLogEntry& Entry)
{
	// «azione base + profilo» quando la voce sa dirlo (D-033), altrimenti il solo ActionId. La forma con la
	// barretta si legge in un colpo — `Action.BasicAttack · Riktor.ImpactShot` — e non richiede di sapere a
	// memoria che ImpactShot e' un attacco base.
	//
	// Il caso `BaseActionId == ActionId` non produce «X · X»: un'azione generica usata direttamente e' il
	// profilo di se stessa, e ripeterla due volte sarebbe rumore.
	if (Entry.BaseActionId.IsNone() || Entry.BaseActionId == Entry.ActionId)
	{
		return Entry.ActionId.ToString();
	}
	return FString::Printf(TEXT("%s · %s"), *Entry.BaseActionId.ToString(), *Entry.ActionId.ToString());
}

FString URTTurnLogLibrary::DescribeEntry(const FRTTurnLogEntry& Entry)
{
	auto CellText = [](const FRTCellId& Cell)
	{
		return FString::Printf(TEXT("(q=%d,r=%d,L=%d)"), Cell.X, Cell.Y, Cell.Layer);
	};

	if (Entry.Category == ERTLogCategory::Move)
	{
		const TCHAR* Reason = TEXT("");
		switch (static_cast<ERTMoveOutcome>(Entry.Outcome))
		{
		case ERTMoveOutcome::Moved:             Reason = TEXT("si muove"); break;
		case ERTMoveOutcome::BlockedContested:  Reason = TEXT("fermo: cella contesa"); break;
		case ERTMoveOutcome::BlockedByUnit:     Reason = TEXT("fermo: cella occupata"); break;
		case ERTMoveOutcome::BlockedByPriority: Reason = TEXT("fermo: precedenza avversa"); break;
		case ERTMoveOutcome::BlockedByImpact:   Reason = TEXT("fermo: scontro frontale"); break;
		// La cella e' LIBERA: a fermarla e' stato un colpo deciso un turno prima (E18). Dirlo «occupata»
		// manderebbe il giocatore a cercare un'unita' che non c'e'.
		case ERTMoveOutcome::StoppedByPrediction: Reason = TEXT("fermo: colto da una previsione"); break;
		// Come la previsione, la cella e' libera — ma il giocatore deve poter distinguere le due cose, perche'
		// insegnano lezioni diverse: la previsione era una scommessa fatta al buio un turno prima, l'Overwatch
		// e' qualcuno che ti ha visto arrivare e ha scelto TE. «Colto da una previsione» qui manderebbe a
		// cercare un errore di lettura dove c'e' stata una lettura riuscita (CP 14.5).
		case ERTMoveOutcome::StoppedByOverwatch: Reason = TEXT("fermo: colpito in overwatch"); break;
		// Subito, non scelto (#307): «si muove» direbbe una cosa falsa di un'unita' che e' stata spinta.
		case ERTMoveOutcome::Displaced:         Reason = TEXT("spostata"); break;
		// Spinta annullata (#420). Il testo NON e' costante: il valore di questa voce sta tutto nel dire
		// QUALE dei sei modi di non muoversi si e' verificato, e «fermo: spinta annullata» ne direbbe zero.
		case ERTMoveOutcome::DisplacementResisted:
			switch (static_cast<ERTDisplacementBlockReason>(Entry.Amount))
			{
			case ERTDisplacementBlockReason::Guarded:        Reason = TEXT("spinta retta: in guardia"); break;
			case ERTDisplacementBlockReason::Braced:         Reason = TEXT("spinta retta: irrigidito"); break;
			case ERTDisplacementBlockReason::OpposingForces: Reason = TEXT("spinta annullata: forze opposte"); break;
			case ERTDisplacementBlockReason::NoDestination:  Reason = TEXT("spinta annullata: nessuna uscita"); break;
			case ERTDisplacementBlockReason::ContestedDestination:
				Reason = TEXT("spinta annullata: destinazione contesa"); break;
			// «retta» come `Guarded`/`Braced` e non «annullata»: qualcuno ha fatto qualcosa. La differenza con
			// quelle due — che l'ancora e' una REAZIONE, e si consuma — la dice gia' la voce di categoria
			// `Reaction` che il pass scrive nello stesso turno.
			case ERTDisplacementBlockReason::Anchored:
				Reason = TEXT("spinta retta: ancorato"); break;
			// Un valore aggiunto in coda e non ancora tradotto qui: si legge lo stesso, senza mentire su quale
			// sia. Il `default` di sopra direbbe «resta», che e' la parola dell'esito sbagliato.
			default:                                         Reason = TEXT("spinta annullata"); break;
			}
			break;
		default:                                Reason = TEXT("resta"); break;
		}

		// PERCHE' si e' mossa (#307): l'azione che ha causato lo spostamento, quando la voce la dichiara.
		// `Action.Move` compreso — un movimento volontario e uno scatto vanno distinti, e sono la stessa
		// categoria di voce con `ActionId` diverso.
		const FString Cause = Entry.ActionId.IsNone()
			? FString()
			: (Entry.Priority != 0
				? FString::Printf(TEXT(" (%s, p%d)"), *DescribeActionIdentity(Entry), Entry.Priority)
				: FString::Printf(TEXT(" (%s)"), *DescribeActionIdentity(Entry)));

		if (static_cast<ERTMoveOutcome>(Entry.Outcome) == ERTMoveOutcome::Moved
			|| static_cast<ERTMoveOutcome>(Entry.Outcome) == ERTMoveOutcome::Displaced)
		{
			return FString::Printf(TEXT("%s %s -> %s (%d celle)%s"),
				Reason, *CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount, *Cause);
		}
		return FString::Printf(TEXT("%s %s%s"), Reason, *CellText(Entry.SrcCell), *Cause);
	}

	// Fallback: cosa e' successo all'azione che non era piu' eseguibile. Il motivo per cui non lo era viaggia
	// in `Amount` (ERTActionInvalidReason): una riga che dice «annullata» senza dire da cosa non insegna nulla.
	if (Entry.Category == ERTLogCategory::Fallback)
	{
		const TCHAR* What = TEXT("");
		switch (static_cast<ERTFallbackOutcome>(Entry.Outcome))
		{
		case ERTFallbackOutcome::Stopped:      What = TEXT("fermata"); break;
		case ERTFallbackOutcome::Waited:       What = TEXT("sostituita con l'attesa"); break;
		case ERTFallbackOutcome::AttackedCell: What = TEXT("colpisce la cella pianificata"); break;
		default:                               What = TEXT("annullata"); break;
		}

		const TCHAR* Why = TEXT("");
		switch (static_cast<ERTActionInvalidReason>(Entry.Amount))
		{
		case ERTActionInvalidReason::TargetGone:     Why = TEXT("bersaglio assente"); break;
		case ERTActionInvalidReason::TargetDead:     Why = TEXT("bersaglio eliminato"); break;
		case ERTActionInvalidReason::TargetFriendly: Why = TEXT("bersaglio alleato"); break;
		case ERTActionInvalidReason::OutOfRange:     Why = TEXT("fuori portata"); break;
		case ERTActionInvalidReason::NoLineOfSight:  Why = TEXT("nessuna linea di tiro"); break;
		case ERTActionInvalidReason::NoMap:          Why = TEXT("nessuna mappa autorevole"); break;
		default:                                     Why = TEXT("non eseguibile"); break;
		}

		return FString::Printf(TEXT("%s -> %s: azione %s (%s)"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), What, Why);
	}

	// Reazione: attivata o no, e perche' — mai in silenzio (CP 5.1).
	if (Entry.Category == ERTLogCategory::Reaction)
	{
		const TCHAR* What = TEXT("");
		switch (static_cast<ERTReactionOutcome>(Entry.Outcome))
		{
		case ERTReactionOutcome::Activated:    What = TEXT("reazione attivata"); break;
		case ERTReactionOutcome::NotTriggered: What = TEXT("reazione pronta, nessun trigger"); break;
		default:                               What = TEXT("reazione non disponibile"); break;
		}
		// QUALE reazione, quando l'identita' c'e': fra `Riktor.Interposition` e `Action.Intercept` cambia
		// l'abilita' spesa e il cooldown, non solo l'esito (CP 5.5).
		if (!Entry.ActionId.IsNone())
		{
			return FString::Printf(TEXT("%s: %s (%s)"),
				*CellText(Entry.SrcCell), What, *DescribeActionIdentity(Entry));
		}
		return FString::Printf(TEXT("%s: %s"), *CellText(Entry.SrcCell), What);
	}

	// Previsione: dove si e' scommesso, e se la scommessa ha pagato (E18 CP 18.1). Il whiff ha una riga
	// propria perche' e' il caso da leggere: senza, un turno in cui non succede niente sarebbe indistinguibile
	// da un turno in cui l'azione non e' mai stata dichiarata.
	if (Entry.Category == ERTLogCategory::Predictive)
	{
		const FString Who = Entry.ActionId.IsNone()
			? FString(TEXT("previsione")) : DescribeActionIdentity(Entry);

		if (static_cast<ERTPredictiveOutcome>(Entry.Outcome) == ERTPredictiveOutcome::TriggerMatched)
		{
			return FString::Printf(TEXT("%s -> %s: previsione azzeccata, %d danni e movimento troncato (%s)"),
				*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount, *Who);
		}
		return FString::Printf(TEXT("%s -> %s: previsione a vuoto, nessuno e' entrato (%s)"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), *Who);
	}

	// La DECISIONE di una finestra di reazione (CP 14.5). Il testo dice sempre e per prima cosa il MOTIVO,
	// perche' e' li' che sta l'informazione: `HOLD` scelto e `HOLD` per scadenza hanno lo stesso effetto e
	// raccontano due turni diversi, e un combat log che scrivesse «tiene il colpo» per entrambi cancellerebbe
	// proprio la distinzione per cui il motivo esiste.
	if (Entry.Category == ERTLogCategory::ReactionDecision)
	{
		const FString Who = Entry.ActionId.IsNone()
			? FString(TEXT("overwatch")) : DescribeActionIdentity(Entry);

		switch (static_cast<ERTReactionDecisionOutcome>(Entry.Outcome))
		{
		case ERTReactionDecisionOutcome::FireChosen:
			return FString::Printf(TEXT("%s -> %s: overwatch spara sull'unita' %d, %d danni (%s)"),
				*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.SelectedTargetUnitId,
				Entry.Amount, *Who);
		// I cinque `Hold` NON condividono un testo, ed e' il punto della voce: applicano tutti lo stesso
		// effetto — nessuno — e rispondono in modo diverso all'unica domanda che il giocatore pone davvero,
		// *perche' non ha sparato?*. Un «tiene il colpo» per tutti cancellerebbe proprio quella differenza.
		case ERTReactionDecisionOutcome::HoldChosen:
			return FString::Printf(TEXT("%s: overwatch tiene il colpo, resta armato (%s)"),
				*CellText(Entry.SrcCell), *Who);
		case ERTReactionDecisionOutcome::HoldTimeout:
			return FString::Printf(TEXT("%s: overwatch non risponde in tempo, resta armato (%s)"),
				*CellText(Entry.SrcCell), *Who);
		case ERTReactionDecisionOutcome::HoldNoDecider:
			return FString::Printf(TEXT("%s: overwatch senza decisore, resta armato (%s)"),
				*CellText(Entry.SrcCell), *Who);
		case ERTReactionDecisionOutcome::HoldRejected:
			return FString::Printf(TEXT("%s: risposta non ammessa, overwatch resta armato (%s)"),
				*CellText(Entry.SrcCell), *Who);
		case ERTReactionDecisionOutcome::HoldImmediate:
			return FString::Printf(TEXT("%s: nessun bersaglio ammesso, overwatch resta armato (%s)"),
				*CellText(Entry.SrcCell), *Who);
		}
		// Un valore aggiunto in coda e non ancora tradotto: si dice cosi', invece di mentire su quale sia.
		// Stessa disciplina di `ERTDisplacementBlockReason` piu' sopra.
		return FString::Printf(TEXT("%s: overwatch, esito non tradotto (%s)"), *CellText(Entry.SrcCell), *Who);
	}

	// Orientamento: quando cambia, e chi lo ha letto (CP 16.1). Senza queste righe il combat log direbbe che
	// un colpo e' arrivato alle spalle senza mai dire quando l'unita' si e' girata.
	if (Entry.Category == ERTLogCategory::Facing)
	{
		static const TCHAR* DirectionNames[6] = { TEXT("E"), TEXT("NE"), TEXT("NW"), TEXT("W"), TEXT("SW"), TEXT("SE") };
		const int32 DirIndex = FMath::Clamp(Entry.Amount, 0, 5);
		const TCHAR* Dir = DirectionNames[DirIndex];

		switch (static_cast<ERTFacingOutcome>(Entry.Outcome))
		{
		case ERTFacingOutcome::DerivedFromMove:
			return FString::Printf(TEXT("%s: si orienta a %s (movimento)"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::DerivedFromDash:
			return FString::Printf(TEXT("%s: si orienta a %s (scatto)"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::DeclaredInPlanning:
			return FString::Printf(TEXT("%s: si gira a %s (dichiarata)"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::DeclarationRejected:
			return FString::Printf(TEXT("%s: rotazione illegale rifiutata, resta a %s"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::TargetingReoriented:
			return FString::Printf(TEXT("%s: si orienta a %s verso il bersaglio"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::TurnedToDisplacementSource:
			return FString::Printf(TEXT("%s: spinta, si gira a %s verso la sorgente"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::KeptOnEnvironmentalDisplacement:
			return FString::Printf(TEXT("%s: trascinata, resta a %s"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::UsedByBlast:
			return FString::Printf(TEXT("%s: il colpo usa l'orientamento %s"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::UsedByOverwatch:
			return FString::Printf(TEXT("%s: l'overwatch usa l'orientamento %s"), *CellText(Entry.SrcCell), Dir);
		case ERTFacingOutcome::RearHitBypassedCover:
			return FString::Printf(TEXT("%s -> %s: colpo fuori dall'arco frontale (guardava %s), la protezione non vale"),
				*CellText(Entry.TgtCell), *CellText(Entry.SrcCell), Dir);
		default:
			return FString::Printf(TEXT("%s: orientamento %s"), *CellText(Entry.SrcCell), Dir);
		}
	}

	// Combat: chi colpisce chi, con quale esito e quanto danno.
	//
	// CON CHE COSA (CP 11.3, #79): il nome dell'azione si accoda fra parentesi quando la voce lo porta, con la
	// stessa forma «base · profilo» delle reazioni. Va in coda e non in testa perche' cio' che il giocatore
	// cerca per primo e' l'esito — «22 danni, eliminata» — e il nome e' la risposta alla domanda successiva.
	// Le voci senza identita' restano ESATTAMENTE la stringa di prima: le righe gia' verificate non cambiano.
	// Con che cosa, e **con quale precedenza** (CP 11.3, formato v7). La priorita' compare solo quando la
	// voce la dichiara: `p0` su ogni riga sarebbe rumore su tracce scritte prima della v7, dove lo zero
	// significa «non dichiarata» e non «priorita' zero».
	const FString Tail = Entry.ActionId.IsNone()
		? FString()
		: (Entry.Priority != 0
			? FString::Printf(TEXT(" (%s, p%d)"), *DescribeActionIdentity(Entry), Entry.Priority)
			: FString::Printf(TEXT(" (%s)"), *DescribeActionIdentity(Entry)));

	switch (static_cast<ERTCombatOutcome>(Entry.Outcome))
	{
	case ERTCombatOutcome::NoLineOfSight:
		return FString::Printf(TEXT("%s -> %s: nessuna linea di tiro%s"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), *Tail);

	case ERTCombatOutcome::ShieldAbsorbed:
		return FString::Printf(TEXT("%s -> %s: %d assorbiti dallo scudo%s"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount, *Tail);

	case ERTCombatOutcome::Lethal:
		return FString::Printf(TEXT("%s -> %s: %d danni, eliminata%s"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount, *Tail);

	case ERTCombatOutcome::TerrainBonus:
		return FString::Printf(TEXT("%s -> %s: %d danni (bonus posizione)%s"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount, *Tail);

	default:
		return FString::Printf(TEXT("%s -> %s: %d danni%s"),
			*CellText(Entry.SrcCell), *CellText(Entry.TgtCell), Entry.Amount, *Tail);
	}
}

namespace
{
	constexpr uint32 RT_FNV_OFFSET_BASIS = 2166136261u;
	constexpr uint32 RT_FNV_PRIME        = 16777619u;

	/**
	 * Mescola i CAMPI di una voce in un FNV-1a a 32 bit.
	 *
	 * Estratto perche' i due hash del TurnLog — `HashTurnLog` (canonico) e `HashTurnLogOrdered` — devono
	 * mescolare **esattamente gli stessi campi**: l'unica differenza fra loro e' il sort davanti. Se i due
	 * elenchi di campi divergessero, i due hash risponderebbero a domande diverse da quelle documentate e
	 * nessun test se ne accorgerebbe.
	 */
	void MixEntryFields(uint32& Hash, const FRTTurnLogEntry& E)
	{
		auto Mix = [&Hash](uint32 V)
		{
			Hash ^= V;
			Hash *= RT_FNV_PRIME;
		};
		Mix(static_cast<uint32>(E.Phase));
		Mix(static_cast<uint32>(E.Category));
		Mix(static_cast<uint32>(E.Outcome));
		Mix(static_cast<uint32>(E.SrcCell.X));
		Mix(static_cast<uint32>(E.SrcCell.Y));
		Mix(static_cast<uint32>(E.SrcCell.Layer));
		Mix(static_cast<uint32>(E.TgtCell.X));
		Mix(static_cast<uint32>(E.TgtCell.Y));
		Mix(static_cast<uint32>(E.TgtCell.Layer));
		Mix(static_cast<uint32>(E.Amount));
		// L'identita' dell'azione entra nell'hash byte per byte: due reazioni con la stessa geometria e lo
		// stesso esito, ma abilita' diverse, devono produrre hash diversi — altrimenti il replay di CP 12.6
		// non distinguerebbe `Riktor.Interposition` da `Action.Intercept`. Un nome vuoto non mescola nulla,
		// quindi le tracce senza ActionId hanno lo stesso hash di prima di CP 5.5.
		for (const TCHAR Ch : E.ActionId.ToString())
		{
			Mix(static_cast<uint32>(Ch));
		}
		// `BaseActionId` NON entra, ed e' deliberato: e' una FUNZIONE di `ActionId`, che qui c'e' gia'.
		// Due tracce non possono differire solo per quel campo, quindi mescolarlo aggiungerebbe zero potere
		// discriminante — e invaliderebbe in blocco ogni hash golden. Stesso ragionamento di `FormatId`
		// (CP 10.3). Se un giorno `BaseActionId` smettesse di essere derivabile da `ActionId`, questa riga
		// di commento diventa falsa e il campo deve entrare: e' la condizione da ricontrollare, non una
		// proprieta' per sempre.
		//
		// `GraphRevision` ENTRA: due tracce possono differire SOLO per lei — stessi eventi, ma grafo modificato
		// in un turno precedente — e sono due partite diverse. Un movimento validato su un grafo e uno
		// validato su un altro non sono lo stesso evento, anche quando le celle coincidono.
		Mix(static_cast<uint32>(E.GraphRevision));
		// `UnitId` e `TurnNumber` NON entrano, per lo stesso criterio (D-063): servono a rendere la traccia
		// spiegabile — chi ha agito, in quale turno — non a discriminarla. Includerli invaliderebbe in blocco
		// ogni hash golden senza aggiungere potere discriminante.
		//
		// La DECISIONE di una finestra ENTRA (v8, CP 14.5), e mescolarla e' il punto: due partite con gli
		// stessi movimenti in cui un giocatore ha sparato e l'altro ha tenuto sono due partite diverse, ed e'
		// esattamente cio' che E14 aggiunge al gioco. `Outcome` e `Amount` — la risposta e il suo motivo —
		// sono gia' mescolati sopra insieme a tutti gli altri esiti; qui restano i due che li qualificano.
		//
		// ⚠️ Un id VUOTO non mescola nulla, ed e' questo che tiene fermi gli hash golden delle tracce senza
		// decisioni: il ciclo non gira, e `SelectedTargetUnitId` e' mescolato solo dentro il ramo. Se lo si
		// mescolasse incondizionatamente, `INDEX_NONE` cambierebbe l'hash di **ogni** voce del progetto.
		for (const TCHAR Ch : E.OpportunityId)
		{
			Mix(static_cast<uint32>(Ch));
		}
		if (!E.OpportunityId.IsEmpty())
		{
			Mix(static_cast<uint32>(E.SelectedTargetUnitId));
		}
		// `ReactionInstanceId` NON entra: e' un numero d'ordine dell'armamento, quindi spiega e non discrimina.
		// Due tracce che differissero solo per lui differirebbero gia' per l'`OpportunityId`, che l'istanza la
		// porta dentro attraverso `Seq`.
	}
}

uint32 URTTurnLogLibrary::HashTurnLog(const TArray<FRTTurnLogEntry>& Entries)
{
	// Ordina prima di mescolare: stesso insieme di voci -> stessa sequenza -> stesso hash (permutazione-invariante).
	TArray<FRTTurnLogEntry> Sorted = Entries;
	SortTurnLog(Sorted);

	uint32 Hash = RT_FNV_OFFSET_BASIS;
	for (const FRTTurnLogEntry& E : Sorted)
	{
		MixEntryFields(Hash, E);
	}
	return Hash;
}

uint32 URTTurnLogLibrary::HashTurnLogOrdered(const TArray<FRTTurnLogEntry>& Entries)
{
	// NESSUN sort: le voci si mescolano nell'ordine in cui il resolver le ha emesse. E' l'UNICA differenza
	// con `HashTurnLog` — stessi campi, stesso FNV — ed e' cio' che rende visibile un riordino delle
	// emissioni, che all'hash canonico e' invisibile per costruzione.
	uint32 Hash = RT_FNV_OFFSET_BASIS;
	for (const FRTTurnLogEntry& E : Entries)
	{
		MixEntryFields(Hash, E);
	}
	return Hash;
}

namespace
{
	// Magic 'RTTL' e helper little-endian espliciti: il formato non dipende dall'endianness della
	// piattaforma (determinismo/portabilita', invariante #4). Solo interi.
	constexpr uint32 RT_TURNLOG_MAGIC = 0x4C545452u; // byte su disco: 'R','T','T','L'

	void AppendU8(TArray<uint8>& B, uint8 V) { B.Add(V); }

	void AppendU16LE(TArray<uint8>& B, uint16 V)
	{
		B.Add(static_cast<uint8>(V & 0xFF));
		B.Add(static_cast<uint8>((V >> 8) & 0xFF));
	}

	void AppendU32LE(TArray<uint8>& B, uint32 V)
	{
		B.Add(static_cast<uint8>(V & 0xFF));
		B.Add(static_cast<uint8>((V >> 8) & 0xFF));
		B.Add(static_cast<uint8>((V >> 16) & 0xFF));
		B.Add(static_cast<uint8>((V >> 24) & 0xFF));
	}

	void AppendI32LE(TArray<uint8>& B, int32 V) { AppendU32LE(B, static_cast<uint32>(V)); }

	/**
	 * Stringa a lunghezza variabile: uint16 di lunghezza in byte + payload UTF-8. Primo campo non a
	 * dimensione fissa del formato — l'ActionId e' un nome, e troncarlo a lunghezza fissa renderebbe due
	 * azioni dal prefisso comune indistinguibili.
	 *
	 * Oltre 65535 byte la stringa viene troncata: e' il limite del campo di lunghezza. Nessun ActionId del
	 * catalogo si avvicina a quella soglia (sono nomi come `Riktor.Interposition`), quindi il caso non e'
	 * raggiungibile da dati validi — se lo diventasse, il posto dove rifiutarlo e' il validator del catalogo,
	 * non il serializzatore.
	 */
	void AppendStringUtf8(TArray<uint8>& B, const FString& S)
	{
		const FTCHARToUTF8 Utf8(*S);
		const int32 Len = FMath::Min(Utf8.Length(), static_cast<int32>(MAX_uint16));
		AppendU16LE(B, static_cast<uint16>(Len));
		B.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Len);
	}

	// Letture con bounds-check: ritornano false invece di leggere fuori dal buffer (parser sicuro).
	bool ReadU8(const TArray<uint8>& B, int32& Pos, uint8& Out)
	{
		if (Pos + 1 > B.Num()) { return false; }
		Out = B[Pos];
		Pos += 1;
		return true;
	}

	bool ReadU16LE(const TArray<uint8>& B, int32& Pos, uint16& Out)
	{
		if (Pos + 2 > B.Num()) { return false; }
		Out = static_cast<uint16>(static_cast<uint16>(B[Pos]) | (static_cast<uint16>(B[Pos + 1]) << 8));
		Pos += 2;
		return true;
	}

	bool ReadU32LE(const TArray<uint8>& B, int32& Pos, uint32& Out)
	{
		if (Pos + 4 > B.Num()) { return false; }
		Out = static_cast<uint32>(B[Pos])
			| (static_cast<uint32>(B[Pos + 1]) << 8)
			| (static_cast<uint32>(B[Pos + 2]) << 16)
			| (static_cast<uint32>(B[Pos + 3]) << 24);
		Pos += 4;
		return true;
	}

	bool ReadI32LE(const TArray<uint8>& B, int32& Pos, int32& Out)
	{
		uint32 U = 0;
		if (!ReadU32LE(B, Pos, U)) { return false; }
		Out = static_cast<int32>(U);
		return true;
	}

	bool ReadStringUtf8(const TArray<uint8>& B, int32& Pos, FString& Out)
	{
		uint16 Len = 0;
		if (!ReadU16LE(B, Pos, Len)) { return false; }
		if (Pos + Len > B.Num()) { return false; } // bounds-check come per gli interi: nessuna lettura fuori
		Out = FString(FUTF8ToTCHAR(reinterpret_cast<const ANSICHAR*>(B.GetData() + Pos), Len));
		Pos += Len;
		return true;
	}

	// Checksum FNV-1a 32-bit sui byte grezzi (stesso mescolamento di HashTurnLog, ma sul buffer):
	// rileva la corruzione del contenuto che magic/versione da soli non catturano.
	uint32 FnvBytes(const uint8* Data, int32 Len)
	{
		uint32 H = 2166136261u; // offset basis
		for (int32 i = 0; i < Len; ++i)
		{
			H ^= Data[i];
			H *= 16777619u; // prime
		}
		return H;
	}
}

TArray<uint8> URTTurnLogLibrary::SerializeTurnLog(const TArray<FRTTurnLogEntry>& Entries, ERTLogTopology Topology,
	FName FormatId)
{
	// Forma CANONICA: ordina con EntryLess prima di scrivere -> byte permutazione-invarianti (come l'hash).
	TArray<FRTTurnLogEntry> Canonical = Entries;
	SortTurnLog(Canonical);

	TArray<uint8> Out;
	// 31 byte fissi + 2+2 di lunghezza per ActionId e BaseActionId + 12 per i tre interi della v6.
	Out.Reserve(14 + Canonical.Num() * 47);

	// Header: magic + versione + flags(topologia) + identita' del formato + conteggio (little-endian).
	// Il FormatId sta DOPO i flags e prima del conteggio: le posizioni dei campi precedenti non si spostano,
	// cosi' un lettore che ispeziona magic/versione/flags continua a trovarli dove sono sempre stati.
	AppendU32LE(Out, RT_TURNLOG_MAGIC);
	AppendU16LE(Out, static_cast<uint16>(ERTTurnLogFormatVersion::WithReactionDecision));
	AppendU16LE(Out, static_cast<uint16>(Topology));
	AppendStringUtf8(Out, FormatId.IsNone() ? FString() : FormatId.ToString());
	AppendU32LE(Out, static_cast<uint32>(Canonical.Num()));

	for (const FRTTurnLogEntry& E : Canonical)
	{
		AppendU8(Out, static_cast<uint8>(E.Phase));
		AppendU8(Out, static_cast<uint8>(E.Category));
		AppendU8(Out, E.Outcome);
		AppendI32LE(Out, E.SrcCell.X);
		AppendI32LE(Out, E.SrcCell.Y);
		AppendI32LE(Out, E.SrcCell.Layer);
		AppendI32LE(Out, E.TgtCell.X);
		AppendI32LE(Out, E.TgtCell.Y);
		AppendI32LE(Out, E.TgtCell.Layer);
		AppendI32LE(Out, E.Amount);
		AppendStringUtf8(Out, E.ActionId.IsNone() ? FString() : E.ActionId.ToString());
		// Subito dopo l'ActionId, con lo stesso schema: e' il suo complemento, e tenerli adiacenti
		// significa che un lettore che sa saltare uno sa saltare anche l'altro.
		AppendStringUtf8(Out, E.BaseActionId.IsNone() ? FString() : E.BaseActionId.ToString());
		// In coda alla voce (v6): i campi precedenti non si spostano. Interi, non stringhe — l'identita' di
		// un'unita' e' un numero, e passare da `FName` costerebbe una tabella dei nomi in un formato che
		// esiste per essere confrontabile byte-per-byte.
		AppendI32LE(Out, E.UnitId);
		AppendI32LE(Out, E.TurnNumber);
		AppendI32LE(Out, E.GraphRevision);
		AppendI32LE(Out, E.Priority); // v7: in CODA, i campi precedenti non si spostano
		// v8: la decisione di una finestra di reazione. La stringa per prima, con lo schema di `ActionId`,
		// poi i due interi — stesso ordine in cui il lettore li ripesca.
		AppendStringUtf8(Out, E.OpportunityId);
		AppendI32LE(Out, E.ReactionInstanceId);
		AppendI32LE(Out, E.SelectedTargetUnitId);
	}

	// Checksum FNV di tutto cio' che precede (header + voci), in coda: rileva la corruzione del contenuto.
	const uint32 Checksum = FnvBytes(Out.GetData(), Out.Num());
	AppendU32LE(Out, Checksum);
	return Out;
}

bool URTTurnLogLibrary::DeserializeTurnLog(const TArray<uint8>& Bytes, TArray<FRTTurnLogEntry>& OutEntries,
	ERTLogTopology* OutTopology, FName* OutFormatId)
{
	OutEntries.Reset();
	if (OutFormatId)
	{
		*OutFormatId = NAME_None;
	}

	int32 Pos = 0;
	uint32 Magic = 0;
	if (!ReadU32LE(Bytes, Pos, Magic) || Magic != RT_TURNLOG_MAGIC) { return false; }

	// Versioni LEGGIBILI: la corrente e le quattro precedenti. La 2 non porta l'ActionId, la 3 non porta il
	// FormatId, la 4 non porta il BaseActionId, la 5 non porta UnitId/TurnNumber, e in ogni caso il posto
	// resta vuoto — leggerle e' onesto (quei byte non contenevano quell'informazione), inventarla no. Ogni
	// altro valore e' rifiutato: interpretare byte di un formato ignoto produce un replay sbagliato in silenzio.
	uint16 Version = 0;
	if (!ReadU16LE(Bytes, Pos, Version)) { return false; }
	const bool bHasReactionDecision =
		(Version == static_cast<uint16>(ERTTurnLogFormatVersion::WithReactionDecision));
	const bool bHasPriority = bHasReactionDecision
		|| (Version == static_cast<uint16>(ERTTurnLogFormatVersion::WithPriority));
	const bool bHasUnitId = bHasPriority
		|| (Version == static_cast<uint16>(ERTTurnLogFormatVersion::WithUnitId));
	const bool bHasBaseActionId = bHasUnitId
		|| (Version == static_cast<uint16>(ERTTurnLogFormatVersion::WithBaseActionId));
	const bool bHasFormatId = bHasBaseActionId
		|| (Version == static_cast<uint16>(ERTTurnLogFormatVersion::WithFormatId));
	const bool bHasActionId = bHasFormatId
		|| (Version == static_cast<uint16>(ERTTurnLogFormatVersion::WithActionId));
	if (!bHasActionId && Version != static_cast<uint16>(ERTTurnLogFormatVersion::WithChecksum)) { return false; }

	// Flags = topologia delle celle. Fail-closed sui valori sconosciuti (come per la versione): interpretare
	// coordinate di una topologia ignota produrrebbe un replay sbagliato in silenzio.
	uint16 Flags = 0;
	if (!ReadU16LE(Bytes, Pos, Flags)) { return false; }
	if (Flags != static_cast<uint16>(ERTLogTopology::Square) && Flags != static_cast<uint16>(ERTLogTopology::Hex))
	{
		return false;
	}
	if (OutTopology)
	{
		*OutTopology = static_cast<ERTLogTopology>(Flags);
	}

	if (bHasFormatId)
	{
		FString FormatId;
		if (!ReadStringUtf8(Bytes, Pos, FormatId)) { return false; }
		if (OutFormatId)
		{
			*OutFormatId = FormatId.IsEmpty() ? NAME_None : FName(*FormatId);
		}
	}

	uint32 Count = 0;
	if (!ReadU32LE(Bytes, Pos, Count)) { return false; }

	// Fail-closed sul CONTEGGIO, prima di riservare memoria. Un file corrotto puo' dichiarare miliardi di
	// voci, e `Reserve` su quel numero termina il processo (`OnInvalidArrayNum`) prima ancora che il checksum
	// in coda possa smentirlo: il parser deve rifiutare, non morire. Il limite superiore vero e' il buffer
	// che resta — ogni voce occupa almeno i suoi campi a dimensione fissa.
	constexpr int32 FixedEntryBytes = 31;         // 3 uint8 + 7 int32
	// + 2 byte di lunghezza per ogni stringa presente nel formato: ActionId da v3, BaseActionId da v5.
	// + 12 byte fissi per UnitId, TurnNumber e GraphRevision da v6, + 4 per Priority da v7.
	// + 2 di lunghezza per `OpportunityId` e 8 per i due interi della decisione, da v8.
	const int32 MinEntryBytes = FixedEntryBytes + (bHasActionId ? 2 : 0) + (bHasBaseActionId ? 2 : 0)
		+ (bHasUnitId ? 12 : 0) + (bHasPriority ? 4 : 0) + (bHasReactionDecision ? 10 : 0);
	const int32 Remaining = Bytes.Num() - Pos;
	if (Remaining < 0 || Count > static_cast<uint32>(Remaining / MinEntryBytes))
	{
		return false;
	}

	OutEntries.Reserve(static_cast<int32>(Count));
	for (uint32 i = 0; i < Count; ++i)
	{
		FRTTurnLogEntry E;
		uint8 Phase = 0;
		uint8 Category = 0;
		uint8 Outcome = 0;
		if (!ReadU8(Bytes, Pos, Phase) || !ReadU8(Bytes, Pos, Category) || !ReadU8(Bytes, Pos, Outcome))
		{
			OutEntries.Reset();
			return false;
		}
		E.Phase = static_cast<ERTMatchPhase>(Phase);
		E.Category = static_cast<ERTLogCategory>(Category);
		E.Outcome = Outcome;

		if (!ReadI32LE(Bytes, Pos, E.SrcCell.X) || !ReadI32LE(Bytes, Pos, E.SrcCell.Y) || !ReadI32LE(Bytes, Pos, E.SrcCell.Layer)
			|| !ReadI32LE(Bytes, Pos, E.TgtCell.X) || !ReadI32LE(Bytes, Pos, E.TgtCell.Y) || !ReadI32LE(Bytes, Pos, E.TgtCell.Layer)
			|| !ReadI32LE(Bytes, Pos, E.Amount))
		{
			OutEntries.Reset();
			return false;
		}

		if (bHasActionId)
		{
			FString ActionId;
			if (!ReadStringUtf8(Bytes, Pos, ActionId))
			{
				OutEntries.Reset();
				return false;
			}
			E.ActionId = ActionId.IsEmpty() ? NAME_None : FName(*ActionId);
		}
		if (bHasBaseActionId)
		{
			FString BaseActionId;
			if (!ReadStringUtf8(Bytes, Pos, BaseActionId))
			{
				OutEntries.Reset();
				return false;
			}
			E.BaseActionId = BaseActionId.IsEmpty() ? NAME_None : FName(*BaseActionId);
		}
		if (bHasUnitId)
		{
			if (!ReadI32LE(Bytes, Pos, E.UnitId) || !ReadI32LE(Bytes, Pos, E.TurnNumber)
				|| !ReadI32LE(Bytes, Pos, E.GraphRevision))
			{
				OutEntries.Reset();
				return false;
			}
		}
		// Sotto la v6 i tre campi restano a 0: nessuna unita' dedotta dalla cella, nessun turno inventato,
		// nessuna revisione di grafo attribuita a una traccia che non la dichiarava.
		if (bHasPriority)
		{
			if (!ReadI32LE(Bytes, Pos, E.Priority))
			{
				OutEntries.Reset();
				return false;
			}
		}
		// Sotto la v7 `Priority` resta 0, e NON si deduce dall'`ActionId` consultando il catalogo — che pure
		// sarebbe possibile. Sarebbe la stessa inferenza che D-063 ha dichiarato non valida per l'unita': il
		// catalogo di oggi puo' non essere quello con cui la traccia fu scritta, e una priorita' inventata
		// racconterebbe un ordine di risoluzione che quel turno non ha avuto.
		if (bHasReactionDecision)
		{
			if (!ReadStringUtf8(Bytes, Pos, E.OpportunityId)
				|| !ReadI32LE(Bytes, Pos, E.ReactionInstanceId)
				|| !ReadI32LE(Bytes, Pos, E.SelectedTargetUnitId))
			{
				OutEntries.Reset();
				return false;
			}
		}
		// Sotto la v8 i tre campi restano ai loro default — id vuoto, `INDEX_NONE` sui due interi — e qui non
		// c'e' nemmeno la tentazione di dedurli: in quelle versioni nessuna finestra si apriva in partita, e
		// una traccia senza decisioni e' completa cosi' com'e', non monca.
		OutEntries.Add(E);
	}

	// Verifica il checksum in coda: ricalcola FNV su header+voci e confronta (rileva corruzione del contenuto).
	const int32 PayloadEnd = Pos;
	uint32 StoredChecksum = 0;
	if (!ReadU32LE(Bytes, Pos, StoredChecksum))
	{
		OutEntries.Reset();
		return false;
	}
	if (FnvBytes(Bytes.GetData(), PayloadEnd) != StoredChecksum)
	{
		OutEntries.Reset();
		return false;
	}
	return true;
}

bool URTTurnLogLibrary::SaveTurnLogToFile(const FString& Path, const TArray<FRTTurnLogEntry>& Entries,
	ERTLogTopology Topology, FName FormatId)
{
	const TArray<uint8> Bytes = SerializeTurnLog(Entries, Topology, FormatId);
	return FFileHelper::SaveArrayToFile(Bytes, *Path);
}

bool URTTurnLogLibrary::LoadTurnLogFromFile(const FString& Path, TArray<FRTTurnLogEntry>& OutEntries,
	ERTLogTopology* OutTopology, FName* OutFormatId)
{
	OutEntries.Reset();
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *Path))
	{
		return false; // file mancante o illeggibile
	}
	return DeserializeTurnLog(Bytes, OutEntries, OutTopology, OutFormatId);
}

ERTTraceComparison URTTurnLogLibrary::CompareSerializedTraces(const TArray<uint8>& A, const TArray<uint8>& B)
{
	TArray<FRTTurnLogEntry> EntriesA;
	TArray<FRTTurnLogEntry> EntriesB;
	ERTLogTopology TopologyA = ERTLogTopology::Square;
	ERTLogTopology TopologyB = ERTLogTopology::Square;
	FName FormatA = NAME_None;
	FName FormatB = NAME_None;

	if (!DeserializeTurnLog(A, EntriesA, &TopologyA, &FormatA)
		|| !DeserializeTurnLog(B, EntriesB, &TopologyB, &FormatB))
	{
		return ERTTraceComparison::Unreadable;
	}

	// Il CONTESTO prima del contenuto: due tracce prodotte con formati (o topologie) diversi non sono
	// confrontabili, e dire "divergenza" manderebbe a cercare un difetto nel codice dove c'e' una
	// configurazione diversa. E' la stessa ragione per cui l'header porta questi due campi.
	if (FormatA != FormatB)
	{
		return ERTTraceComparison::FormatMismatch;
	}
	if (TopologyA != TopologyB)
	{
		return ERTTraceComparison::TopologyMismatch;
	}

	return HashTurnLog(EntriesA) == HashTurnLog(EntriesB)
		? ERTTraceComparison::Identical
		: ERTTraceComparison::Divergence;
}

namespace
{
	/** Nome della fase per la diagnosi. Switch esplicito, come gli altri di questo file. */
	const TCHAR* GoldenPhaseName(ERTMatchPhase Phase)
	{
		switch (Phase)
		{
		case ERTMatchPhase::Planning:   return TEXT("Planning");
		case ERTMatchPhase::Prep:       return TEXT("Prep");
		case ERTMatchPhase::Dash:       return TEXT("Dash");
		case ERTMatchPhase::Blast:      return TEXT("Blast");
		case ERTMatchPhase::Move:       return TEXT("Move");
		case ERTMatchPhase::Cleanup:    return TEXT("Cleanup");
		case ERTMatchPhase::MatchEnded: return TEXT("MatchEnded");
		default:                        return TEXT("?");
		}
	}

	/**
	 * Uguaglianza secondo i campi che entrano nell'HASH, non campo per campo a mano.
	 *
	 * Cosi' la diagnosi considera divergenza esattamente cio' che `HashTurnLog` considera, che e' l'unica
	 * regola con cui ha senso confrontarsi: `DescribeFirstDivergence` viene chiamata *dopo* che l'hash ha
	 * dichiarato una divergenza, e deve indicare **dove** quell'hash e' cambiato.
	 *
	 * ⚠️ Confrontava con `EntryLess`, ed era corretto finche' i campi dell'ordinamento coincidevano con
	 * quelli dell'hash. Non e' piu' vero: `UnitId` e `TurnNumber` sono entrati in `EntryLess` per chiudere
	 * la forma canonica della serializzazione (D-067) e restano fuori dall'hash (D-063). Con `EntryLess`
	 * questa funzione discriminerebbe **piu'** dell'hash e si fermerebbe su una voce identica per l'hash —
	 * per giunta mostrando due descrizioni uguali, perche' quei campi nessuno li stampa.
	 *
	 * Passa dalla `HashTurnLogOrdered` di una voce sola invece di elencare i campi a mano: cosi' l'elenco
	 * resta uno solo (`MixEntryFields`) e non puo' divergere in silenzio da quello vero.
	 */
	bool GoldenEntriesMatch(const FRTTurnLogEntry& A, const FRTTurnLogEntry& B)
	{
		return URTTurnLogLibrary::HashTurnLogOrdered({ A }) == URTTurnLogLibrary::HashTurnLogOrdered({ B });
	}
}

FString URTTurnLogLibrary::DescribeFirstDivergence(int32 TurnNumber, const TArray<FRTTurnLogEntry>& Golden,
	const TArray<FRTTurnLogEntry>& Actual)
{
	const int32 Common = FMath::Min(Golden.Num(), Actual.Num());
	for (int32 i = 0; i < Common; ++i)
	{
		if (GoldenEntriesMatch(Golden[i], Actual[i]))
		{
			continue;
		}

		// Turno, fase e ActionId sono cio' che il DoD di CP 12.6 chiede per nome; la descrizione delle due
		// voci evita il viaggio di ritorno al codice per capire cosa sia cambiato.
		//
		// L'ActionId si nomina DA ENTRAMBE le parti quando differisce, e non e' un dettaglio estetico:
		// `DescribeEntry` non lo stampa per le voci `Move`, quindi una regressione che cambia SOLO l'azione
		// produceva «atteso [X], trovato [X]» — due stringhe identiche accanto alla parola «diverge». Trovato
		// con la verifica di mutazione, che e' esattamente il caso per cui serve.
		const bool bSameAction = Golden[i].ActionId == Actual[i].ActionId;
		const FString ActionText = bSameAction
			? FString::Printf(TEXT("azione '%s'"), *Golden[i].ActionId.ToString())
			: FString::Printf(TEXT("azione attesa '%s', trovata '%s'"),
				*Golden[i].ActionId.ToString(), *Actual[i].ActionId.ToString());

		const FString GoldenText = DescribeEntry(Golden[i]);
		const FString ActualText = DescribeEntry(Actual[i]);

		// Se le due descrizioni COINCIDONO, il campo che diverge e' uno che `DescribeEntry` non stampa per
		// quella categoria — `TgtCell` fuori da `Moved`, per dirne uno. Mostrare «atteso [X], trovato [X]»
		// farebbe concludere che il confronto e' rotto: e' successo con l'ActionId, trovato in mutazione, e
		// qui si chiude la CLASSE invece del singolo caso. I campi grezzi non sono belli da leggere, ma
		// rispondono alla sola domanda che conta quando la prosa non basta.
		FString RawDetail;
		if (GoldenText.Equals(ActualText))
		{
			auto RawOf = [](const FRTTurnLogEntry& E)
			{
				return FString::Printf(TEXT("outcome=%u amount=%d src=(%d,%d,%d) tgt=(%d,%d,%d)"),
					E.Outcome, E.Amount,
					E.SrcCell.X, E.SrcCell.Y, E.SrcCell.Layer,
					E.TgtCell.X, E.TgtCell.Y, E.TgtCell.Layer);
			};
			RawDetail = FString::Printf(TEXT(" — campi: atteso {%s}, trovato {%s}"),
				*RawOf(Golden[i]), *RawOf(Actual[i]));
		}

		return FString::Printf(
			TEXT("turno %d, voce %d: fase %s, %s — atteso [%s], trovato [%s]%s"),
			TurnNumber, i, GoldenPhaseName(Golden[i].Phase), *ActionText,
			*GoldenText, *ActualText, *RawDetail);
	}

	// Stesse voci fin dove entrambe arrivano, ma una delle due finisce prima: e' una divergenza, e va detta
	// invece di leggere fuori dall'array. La prima voce in piu' (o in meno) e' la piu' informativa.
	if (Golden.Num() != Actual.Num())
	{
		const bool bMissing = Actual.Num() < Golden.Num();
		const FRTTurnLogEntry& Odd = bMissing ? Golden[Common] : Actual[Common];
		return FString::Printf(
			TEXT("turno %d: %d voci attese, %d trovate — la prima %s e' in fase %s, azione '%s' [%s]"),
			TurnNumber, Golden.Num(), Actual.Num(),
			bMissing ? TEXT("MANCANTE") : TEXT("IN PIU'"),
			GoldenPhaseName(Odd.Phase), *Odd.ActionId.ToString(), *DescribeEntry(Odd));
	}

	return FString();
}
