#include "UI/RTPlayerEventProjector.h"

#include "Turn/RTTurnLog.h"

namespace
{
	/**
	 * Cosa il giocatore ha visto succedere, letto da categoria ed esito.
	 *
	 * ⚠️ **Restituisce `false` per tutto cio' che non e' narrazione**, e la lista di ammessi e' corta apposta:
	 * `Facing`, `Fallback` e il bookkeeping delle reazioni sono diagnostica: dicono al replay *perche'* una
	 * cosa e' andata cosi', non raccontano al giocatore *che cosa* e' successo. Restano interi in `#79` e nel
	 * `TurnLog`, che questa funzione non tocca.
	 */
	bool ClassifyEntry(const FRTTurnLogEntry& Entry, ERTPlayerEventType& OutType,
		ERTPlayerEventImportance& OutImportance)
	{
		switch (Entry.Category)
		{
		case ERTLogCategory::Move:
		{
			const ERTMoveOutcome Outcome = static_cast<ERTMoveOutcome>(Entry.Outcome);
			switch (Outcome)
			{
			// 🔴 Fermata da qualcosa: e' una conseguenza tattica, e il giocatore deve sapere che il piano
			// non e' andato come voleva. `StoppedByOverwatch` compresa — l'evento della reazione la
			// racconta dal lato di chi ha sparato, questa dal lato di chi si e' fermato.
			case ERTMoveOutcome::BlockedContested:
			case ERTMoveOutcome::BlockedByUnit:
			case ERTMoveOutcome::BlockedByPriority:
			case ERTMoveOutcome::BlockedByImpact:
			case ERTMoveOutcome::BlockedByTopology:
			case ERTMoveOutcome::StoppedByPrediction:
			case ERTMoveOutcome::StoppedByOverwatch:
				OutType = ERTPlayerEventType::MoveBlocked;
				OutImportance = ERTPlayerEventImportance::Important;
				return true;

			// Spostata contro la propria volonta': non e' il movimento ordinario, e si vede poco.
			case ERTMoveOutcome::Displaced:
			case ERTMoveOutcome::DisplacementResisted:
				OutType = ERTPlayerEventType::Moved;
				OutImportance = ERTPlayerEventImportance::Important;
				return true;

			// ⚠️ Il movimento RIUSCITO e' `Minor`, ed e' la scelta di §D: il giocatore lo vede gia'
			// animato, e una riga per ogni spostamento seppellirebbe le tre che contano.
			case ERTMoveOutcome::Moved:
				OutType = ERTPlayerEventType::Moved;
				OutImportance = ERTPlayerEventImportance::Minor;
				return true;

			// Lo scivolamento e' `Important` e non `Minor` come `Moved` (#2253). L'argomento di §D — «il
			// giocatore lo vede gia' animato» — vale per un movimento CHIESTO: qui l'unita' e' finita dove il
			// giocatore non l'aveva mandata, ed e' esattamente il genere di cosa che la riga esiste per
			// raccontare. Sta con `Displaced` e `DisplacementResisted`, che sono l'altra faccia dello stesso
			// fatto: spostamenti SUBITI.
			//
			// 🔴 **Un esito nuovo va tradotto QUI oltre che nel rendering leggibile, e questo posto non
			// fallisce a compilazione**: lo `switch` finisce in `default: return false`, quindi un valore non
			// tradotto semplicemente non produce nulla.
			//
			// ⚠️ **Rettifica a `#2258`, che qui scriveva «senza questo ramo lo scivolamento SPARISCE dal
			// feed».** Era falso, ed e' stato misurato in `#2284`: `Moved` e' `Minor`, e i `Minor` vengono
			// tolti in blocco alla fine di `Project` (§D li vuole «normalmente silenziosi»). Lo scivolamento
			// non compariva **gia' prima**; questo ramo non ripara una regressione, lo rende visibile per la
			// prima volta.
			case ERTMoveOutcome::Slid:
				OutType = ERTPlayerEventType::Moved;
				OutImportance = ERTPlayerEventImportance::Important;
				return true;

			// `Important` come `Slid`, e non `Minor` come `Moved` (#2284). §D tiene silenzioso il movimento
			// riuscito perche' e' l'esito ATTESO — e i `Minor` vengono tolti in blocco in fondo a `Project`,
			// quindi «Minor» qui significherebbe «mai raccontato». Questo esito e' invece un'aspettativa
			// DISATTESA: il ghiaccio non ha fatto cio' che fa sempre.
			//
			// I due sono simmetrici e stanno insieme: `Slid` e' «e' successo qualcosa che non avevi chiesto»,
			// `SlideBlocked` e' «non e' successo qualcosa su cui contavi». Dopo [D-319] la differenza si vedra'
			// nello stato dell'unita': uno dei due lascera' `Status.Unbalanced`, l'altro no. ⚠️ Al futuro: quella
			// decisione e' accettata e non implementata, e `Status.Unbalanced` non esiste ancora in `Source/`.
			case ERTMoveOutcome::SlideBlocked:
				OutType = ERTPlayerEventType::Moved;
				OutImportance = ERTPlayerEventImportance::Important;
				return true;

			// `Stayed` e `SupersededByDash` non sono accaduti: non c'e' niente da raccontare.
			default:
				return false;
			}
		}

		case ERTLogCategory::Combat:
		{
			const ERTCombatOutcome Outcome = static_cast<ERTCombatOutcome>(Entry.Outcome);
			switch (Outcome)
			{
			case ERTCombatOutcome::Lethal:
				OutType = ERTPlayerEventType::Defeated;
				OutImportance = ERTPlayerEventImportance::Critical;
				return true;

			case ERTCombatOutcome::Hit:
			case ERTCombatOutcome::ShieldAbsorbed:
			case ERTCombatOutcome::TerrainBonus:
				OutType = ERTPlayerEventType::Attacked;
				OutImportance = ERTPlayerEventImportance::Important;
				return true;

			case ERTCombatOutcome::Healed:
				OutType = ERTPlayerEventType::Healed;
				OutImportance = ERTPlayerEventImportance::Important;
				return true;

			// `NoLineOfSight` e' il *perche'* di un colpo che non c'e' stato: diagnostica, non cronaca.
			default:
				return false;
			}
		}

		case ERTLogCategory::Reaction:
			OutType = ERTPlayerEventType::ReactionFired;
			OutImportance = ERTPlayerEventImportance::Important;
			return true;

		case ERTLogCategory::Status:
			OutType = ERTPlayerEventType::StatusChanged;
			OutImportance = ERTPlayerEventImportance::Important;
			return true;

		case ERTLogCategory::Environment:
			OutType = ERTPlayerEventType::Environment;
			OutImportance = ERTPlayerEventImportance::Important;
			return true;

		case ERTLogCategory::Objective:
			OutType = ERTPlayerEventType::ObjectiveChanged;
			OutImportance = ERTPlayerEventImportance::Critical;
			return true;

		// `Facing`, `Fallback`, `Predictive`, `ReactionDecision`, `ReactionClash`: diagnostica.
		default:
			return false;
		}
	}

	/** Chi domina chi, quando due eventi parlano della STESSA unita' — `#1936` §E. */
	int32 DominanceRank(ERTPlayerEventType Type)
	{
		switch (Type)
		{
		case ERTPlayerEventType::Defeated:      return 100; // KO > Danno > Colpo
		case ERTPlayerEventType::Attacked:      return 80;
		case ERTPlayerEventType::Healed:        return 80;
		case ERTPlayerEventType::ReactionFired: return 60;
		case ERTPlayerEventType::StatusChanged: return 50;
		case ERTPlayerEventType::MoveBlocked:   return 40;  // Bloccato > Movimento
		case ERTPlayerEventType::Moved:         return 20;
		default:                                return 10;
		}
	}
}

bool URTPlayerEventProjector::IsAuthorized(const FRTTurnLogEntry& Entry, int32 ObserverTeamId)
{
	// Il predicato ESISTENTE, non uno nuovo: e' lo stesso che `ComposeVisibleLogLines` applica al canale
	// testuale, ed e' cio' che rende i due insiemi confrontabili invece che semplicemente simili.
	return Entry.Verdict.AllowsTeam(ObserverTeamId);
}

TArray<FRTPlayerEvent> URTPlayerEventProjector::Project(const TArray<FRTTurnLogEntry>& Entries,
	int32 ObserverTeamId)
{
	TArray<FRTPlayerEvent> Out;

	// L'evento gia' emesso per ciascuna unita', per indice in `Out`: e' come la dominanza sostituisce invece
	// di accodare. `INDEX_NONE` = quell'unita' non ha ancora una riga.
	TMap<int32, int32> IndexByUnit;

	// 🔴 L'ambiente si raggruppa per TURNO, non per unita': la propagazione dell'acqua o del fuoco tocca
	// celle, non qualcuno, e §E vieta esplicitamente «cella d'acqua A / cella d'acqua B / cella
	// elettrificata C». Un solo evento, e il conteggio in `Amount`.
	int32 EnvironmentIndex = INDEX_NONE;

	for (const FRTTurnLogEntry& Entry : Entries)
	{
		// ── PRIMO passo, sempre. Un fatto non autorizzato non diventa un evento, e non lascia traccia
		// nemmeno come conteggio: e' la differenza fra filtrare prima e sanitizzare dopo.
		if (!IsAuthorized(Entry, ObserverTeamId))
		{
			continue;
		}

		ERTPlayerEventType Type = ERTPlayerEventType::Moved;
		ERTPlayerEventImportance Importance = ERTPlayerEventImportance::Minor;
		if (!ClassifyEntry(Entry, Type, Importance))
		{
			continue;
		}

		if (Type == ERTPlayerEventType::Environment)
		{
			if (EnvironmentIndex != INDEX_NONE)
			{
				++Out[EnvironmentIndex].Amount; // quante celle, non quali
				continue;
			}

			FRTPlayerEvent& Ev = Out.AddDefaulted_GetRef();
			Ev.Type = Type;
			Ev.Importance = Importance;
			Ev.PrimaryUnitId = INDEX_NONE; // l'ambiente non e' di nessuno
			Ev.ActionId = Entry.ActionId;
			Ev.Amount = 1;
			EnvironmentIndex = Out.Num() - 1;
			continue;
		}

		// ⚠️ `UnitId == 0` significa «nessuna unita' dichiarata» nel TurnLog ([D-063]), non l'unita' zero.
		// Senza questa traduzione tutte le voci di mondo si raggrupperebbero fra loro come se fossero della
		// stessa unita', e la dominanza ne terrebbe una sola.
		const int32 UnitId = Entry.UnitId == 0 ? INDEX_NONE : Entry.UnitId;

		FRTPlayerEvent Candidate;
		Candidate.Type = Type;
		Candidate.Importance = Importance;
		Candidate.PrimaryUnitId = UnitId;
		// ⛔ `SecondaryUnitId` resta `INDEX_NONE` per le voci di combattimento, e non e' una scorciatoia: il
		// TurnLog ha **un solo** `UnitId` per voce e lo assegna a chi SUBISCE (`#1150`, `#1430`), quindi
		// l'attaccante non e' recuperabile da qui. Inventarlo dalla `SrcCell` sarebbe dedurre un'unita' da
		// una cella, che [D-063] vieta.
		Candidate.ActionId = Entry.ActionId;
		Candidate.Amount = Entry.Amount;

		if (UnitId == INDEX_NONE)
		{
			Out.Add(MoveTemp(Candidate)); // voce di mondo: nessuna dominanza da applicare
			continue;
		}

		const int32* Existing = IndexByUnit.Find(UnitId);
		if (Existing == nullptr)
		{
			Out.Add(MoveTemp(Candidate));
			IndexByUnit.Add(UnitId, Out.Num() - 1);
			continue;
		}

		// ── DOMINANZA: la stessa unita' ha gia' una riga in questo turno.
		//
		// 🔑 **Sostituisce sul posto invece di accodare**, ed e' cio' che impedisce la narrazione tripla
		// «colpo + danno + KO» che §E vieta: il KO prende il posto del danno, il danno quello del colpo, e
		// la riga resta una. La posizione e' quella del PRIMO evento di quell'unita', cosi' la cronaca
		// conserva l'ordine in cui le cose sono cominciate.
		//
		// ⚠️ A parita' di rango vince chi e' arrivato prima: due colpi sulla stessa unita' restano una riga,
		// e il secondo non riscrive il primo. Sommare i danni sarebbe un'altra decisione — non presa qui,
		// perche' §E parla di dominanza e non di aggregazione.
		FRTPlayerEvent& Held = Out[*Existing];
		if (DominanceRank(Candidate.Type) > DominanceRank(Held.Type))
		{
			const int32 Slot = *Existing;
			Out[Slot] = MoveTemp(Candidate);
		}
	}

	// ── Il movimento riuscito e' `Minor`, e §D lo vuole «normalmente silenzioso». Si toglie ALLA FINE e non
	// durante: un `Moved` puo' essere il primo evento di un'unita' che poi viene colpita, e scartarlo subito
	// avrebbe perso lo slot in cui la dominanza scrive il colpo.
	Out.RemoveAll([](const FRTPlayerEvent& Ev)
	{
		return Ev.Importance == ERTPlayerEventImportance::Minor;
	});

	return Out;
}
