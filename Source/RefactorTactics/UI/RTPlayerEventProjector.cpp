#include "UI/RTPlayerEventProjector.h"

#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h" // IsSubjectTheSufferer, IsStatusBirth: chi ha dato, chi ha subito (#3734)
#include "Turn/RTReactionLibrary.h" // ERTReactionOutcome: solo una reazione scattata e' «Reazione scattata»

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
			// `BlockedByCycle` e' di questa famiglia, e ne era rimasto fuori solo perche' e' nato dopo (`#3081`).
			// ⚠️ **Oggi nessuno lo emette** ([D-453]; il produttore l'ha spento [D-445]), quindi questo ramo non
			// cambia nessuna partita: c'e' perche' riaccendere quel valore non renda muto il feed.
			case ERTMoveOutcome::BlockedByCycle:
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
			// 🔴 **Senza questo ramo lo scivolamento sparisce dal feed**: prima di `Slid` la voce portava
			// `Moved` e produceva un evento; con un valore nuovo non tradotto qui cadrebbe nel `default` e
			// l'unita' si sposterebbe senza che nulla lo dica. Un esito nuovo va aggiunto in DUE posti — il
			// rendering leggibile e questa proiezione — e il secondo non fallisce a compilazione.
			case ERTMoveOutcome::Slid:
				OutType = ERTPlayerEventType::Moved;
				OutImportance = ERTPlayerEventImportance::Important;
				return true;

			// Lo scivolamento IMPEDITO ha un tipo proprio (#2314), e non e' un lusso: con `Moved` sarebbe
			// indistinguibile da `Slid` — stesso tipo, stessa importanza, stesso `ActionId` — e la
			// distinzione che l'esito esiste per registrare non arriverebbe al canale piu' visibile. Con
			// `MoveBlocked` direbbe che il piano del giocatore e' fallito, che e' falso: il Move chiesto e'
			// riuscito, e solo lo spostamento AMBIENTALE successivo non e' avvenuto.
			//
			// `Important` come `Slid`: il giocatore non lo vede animato — non succede niente — ed e' proprio
			// il non-fatto che deve leggere, perche' cambia cosa aspettarsi nel turno dopo (`D-319`).
			case ERTMoveOutcome::SlideBlocked:
				OutType = ERTPlayerEventType::SlideBlocked;
				OutImportance = ERTPlayerEventImportance::Important;
				return true;

			// IL PARAPETTO E LE QUATTRO CADUTE (`#3081`): spostamenti SUBITI, come `Displaced` e `Slid`.
			//
			// 🔴 **Erano valori nati dopo questo `switch` e mai tradotti qui**: cadevano nel `default`, e
			// l'unita' cambiava cella — spesso piano — senza che il feed lo dicesse. E' la sparizione che il
			// commento di `Slid` qui sopra aveva previsto.
			//
			// `StoppedByEdgeGuard` sta con `Displaced` e NON con i `Blocked*`: lo spostamento forzato ha
			// percorso celle, e il parapetto l'ha accorciato. `MoveBlocked` direbbe che il piano del giocatore
			// e' fallito, che e' falso — l'obiezione di `#2314` per `SlideBlocked`. Prima che il valore
			// esistesse lo stesso fatto era `Displaced` ([D-354]) e produceva gia' questa riga.
			//
			// ⚠️ **Per chi cade, questa riga di solito NON e' quella che resta.** La caduta scrive per la stessa
			// unita' anche una voce di danno (`URTTurnLogLibrary::FallCauseOfFaller`, rango 80), e la dominanza
			// tiene una riga per unita': vince il danno. Che il giocatore legga «Caduta» lo decide il TESTO di
			// quella riga (`URTHudViewModel::ComposePlayerEventText`), non questo ramo. Questo serve quando
			// l'osservatore e' autorizzato alla voce di movimento e non a quella di danno.
			//
			// I quattro esiti di atterraggio hanno qui lo stesso tipo, e nel feed non si distinguono: dove
			// l'unita' e' finita lo mostra la board, e i quattro testi restano nel log diagnostico
			// (`URTTurnLogLibrary::DescribeEntry`, [D-352]).
			case ERTMoveOutcome::StoppedByEdgeGuard:
			case ERTMoveOutcome::Fell:
			case ERTMoveOutcome::FellToAlternative:
			case ERTMoveOutcome::FellToLastStable:
			case ERTMoveOutcome::FellWithoutLanding:
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

			// 🔴 **`NoLineOfSight` E' cronaca, e fino a `#2697` questo ramo diceva il contrario.** La riga
			// che stava qui — *«e' il perche' di un colpo che non c'e' stato: diagnostica, non cronaca»* —
			// era vera finche' il giocatore aveva un altro modo di capire: guardare. Non ce l'ha, ed e' il
			// caso particolare che `D-340` ha misurato — e' il muro stesso a velare il bersaglio, quindi
			// non si vede ne' l'ostacolo ne' il nemico. Senza questo ramo l'azione dichiarata sparisce
			// senza spiegazione, e `PIE-HEXPLAY-6` resta rossa.
			//
			// ⚠️ **Vale per QUESTO esito e non per la famiglia**: gli altri `Combat` non risolti restano
			// diagnostica, perche' ciascuno lascia qualcosa da guardare.
			case ERTCombatOutcome::NoLineOfSight:
				OutType = ERTPlayerEventType::AttackBlocked;
				OutImportance = ERTPlayerEventImportance::Important;
				return true;

			default:
				return false;
			}
		}

		case ERTLogCategory::Reaction:
			// 🔴 **Solo una reazione SCATTATA** (`#3734`, trovato in code review). Il passaggio delle reazioni scrive
			// una voce per ogni reazione armata in ogni Blast, anche quando non scatta (`NotTriggered`) o non puo'
			// (`Unavailable`): il registro deve dirlo, il feed no. Prima ogni voce diventava «Reazione scattata», e la
			// riga falsa compariva quando l'unita' non aveva niente di piu' alto; con la regola di `#3734` avrebbe
			// coperto anche il colpo che l'unita' aveva dato.
			if (static_cast<ERTReactionOutcome>(Entry.Outcome) != ERTReactionOutcome::Activated)
			{
				return false;
			}
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

	/**
	 * Le tre specie di riga della regola di `#3734`, per le unita' di chi guarda.
	 *
	 * - `OwnSuccess`: un'azione SCELTA e andata a segno — il movimento arrivato, il colpo dato, la cura data.
	 * - `News`: cio' che l'unita' non ha deciso — uno spostamento subito, un piano fallito, uno stato che le arriva
	 *   addosso, una reazione che scatta, il KO.
	 * - `Neutral`: la FINE di uno stato. Non e' successo niente all'unita' in questo turno: scade, si spegne, si
	 *   consuma. Senza questa specie uno stato che scade nel Cleanup copriva il colpo dato (trovato in code review).
	 *
	 * Una riuscita propria cede a una notizia; il resto lo decide il rango.
	 */
	enum class ERTFeedRowSpecies : uint8 { OwnSuccess, News, Neutral };

	/**
	 * 🔑 **Chi ha dato e chi ha subito lo dice il registro**: `IsSubjectTheSufferer` per i colpi — un `Hit` d'arma e'
	 * di chi colpisce, il danno ambientale e quello di una caduta di chi li subisce — e `IsStatusBirth` per gli
	 * stati. Non se ne scrive una seconda tassonomia qui.
	 *
	 * ⚠️ **Uno stato che l'unita' si da' da sola e' una notizia**: la voce di nascita non dice chi l'ha applicato, e
	 * distinguerlo vorrebbe dire cambiare il formato del registro. Limite accettato con la decisione d'autore.
	 */
	ERTFeedRowSpecies FeedRowSpeciesOf(const FRTTurnLogEntry& Entry, ERTPlayerEventType Type,
		ERTPlayerEventImportance Importance)
	{
		switch (Type)
		{
		case ERTPlayerEventType::Moved:
			// chiesto e arrivato e' `Minor`; il resto di `Moved` — spinta, scivolata, caduta, parapetto — e' subito
			return Importance == ERTPlayerEventImportance::Minor ? ERTFeedRowSpecies::OwnSuccess : ERTFeedRowSpecies::News;
		case ERTPlayerEventType::Attacked:
			return URTTurnLogLibrary::IsSubjectTheSufferer(Entry) ? ERTFeedRowSpecies::News : ERTFeedRowSpecies::OwnSuccess;
		case ERTPlayerEventType::Healed:
			// ⚠️ **Sempre data, e non per la tassonomia dei danni.** L'unico produttore e' `ApplyPlannedHeals`, che la
			// scrive a nome di chi cura. `IsSubjectTheSufferer` qui sbaglierebbe la cura su se stessi: celle di
			// partenza e d'arrivo coincidono, e la sua rete la legge come un danno ambientale (code review).
			return ERTFeedRowSpecies::OwnSuccess;
		case ERTPlayerEventType::Defeated:
			return ERTFeedRowSpecies::News; // il KO non cede mai
		case ERTPlayerEventType::StatusChanged:
			return URTTurnLogLibrary::IsStatusBirth(static_cast<ERTStatusOutcome>(Entry.Outcome))
				? ERTFeedRowSpecies::News : ERTFeedRowSpecies::Neutral;
		default:
			return ERTFeedRowSpecies::News; // piani falliti, reazioni scattate
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
		// Fra i due, e non per caso (#2314): sopra `Moved` perche' un movimento riuscito non deve poter
		// coprire il fatto che il terreno abbia provato a spostare l'unita'; sotto `MoveBlocked` perche' un
		// piano fallito e' una notizia piu' grande di uno spostamento ambientale mancato. Senza una riga
		// propria cadrebbe nel `default` a `10` e verrebbe sostituito da qualunque altro evento della stessa
		// unita' — compreso un `Moved` `Minor`, che poi il filtro finale scarta: l'evento sparirebbe.
		//
		// ⚠️ **Ne esce un'asimmetria con `Slid`, ed e' consapevole.** Lo scivolamento AVVENUTO non ha un
		// tipo proprio — resta `Moved`, rango 20 — quindi un `Displaced` dello stesso turno, arrivato prima
		// e con lo stesso rango, lo tiene fuori dal feed; questo esito, a 30, entrerebbe. Sembra rovesciato,
		// e ha una ragione: `Slid` ha una SECONDA voce che lo racconta — `Status.Unbalanced` ([D-319]),
		// proiettata come `StatusChanged` a rango 50, che vince su tutto quanto sopra — mentre `SlideBlocked`
		// non ne ha nessuna. Abbassarlo a 20 per simmetria lo renderebbe l'unico dei due davvero muto.
		// Che `Slid` meriti un tipo proprio resta aperto, ed e' fuori dal perimetro di `#2314`.
		case ERTPlayerEventType::SlideBlocked:  return 30;
		// Fra `MoveBlocked` e `SlideBlocked`, e i due confini hanno ragioni diverse (#2697).
		//
		// 🔴 **Sopra `Moved` perche' senza sarebbe MUTO**: un'unita' che si sposta e poi non riesce a
		// sparare e' il turno ordinario di questo gioco, e col rango di `default` (`10`) la riga del
		// movimento — `Minor`, quindi scartata alla fine — teneva lo slot e l'attacco bloccato spariva.
		// Zero eventi, misurato da `AttackBlockedSurvivesAMoveInTheSameTurn`. E' lo stesso difetto che il
		// commento di `SlideBlocked` qui sopra descrive, ripresentatosi identico per il tipo successivo.
		//
		// ⚠️ **Sotto `MoveBlocked` perche' spesso ne e' la CONSEGUENZA**: un'unita' fermata prima di
		// arrivare in posizione non ha linea di tiro *per quello*, e §E vieta la narrazione doppia. La
		// causa a monte e' la riga piu' utile delle due.
		case ERTPlayerEventType::AttackBlocked: return 35;
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

bool URTPlayerEventProjector::IsAuthorized(const FRTTurnLogEntry& Entry, const TArray<int32>& ObserverTeamIds)
{
	// ⚠️ `ContainsByPredicate` su un insieme vuoto risponde `false`, ed e' il comportamento voluto: un
	// insieme vuoto e' «nessuno guarda», non «guardano tutti». Scriverlo con un ciclo esplicito e un
	// `return true` renderebbe la stessa cosa piu' facile da invertire per sbaglio.
	return ObserverTeamIds.ContainsByPredicate([&Entry](int32 TeamId)
	{
		return IsAuthorized(Entry, TeamId);
	});
}

TArray<FRTPlayerEvent> URTPlayerEventProjector::Project(const TArray<FRTTurnLogEntry>& Entries,
	int32 ObserverTeamId)
{
	// 🔑 **Delega, e non e' una comodita': e' cio' che tiene UNA sola implementazione della dominanza.**
	// Duplicare il corpo qui significherebbe che il raggruppamento per unita' e per turno esiste in due
	// copie, e la seconda si scoprirebbe divergente solo quando un esito nuovo viene tradotto in una sola
	// delle due.
	return Project(Entries, TArray<int32>{ ObserverTeamId });
}

TArray<FRTPlayerEvent> URTPlayerEventProjector::Project(const TArray<FRTTurnLogEntry>& Entries,
	const TArray<int32>& ObserverTeamIds, int32 OwnTeamId)
{
	TArray<FRTPlayerEvent> Out;

	// L'evento gia' emesso per ciascuna unita', per indice in `Out`: e' come la dominanza sostituisce invece
	// di accodare. `INDEX_NONE` = quell'unita' non ha ancora una riga.
	TMap<int32, int32> IndexByUnit;

	// La specie della riga tenuta per ciascuna unita' (`#3734`): accanto alla chiave, non in un array parallelo a
	// `Out`, che andrebbe tenuto allineato a ogni `Add` — anche a quelli delle righe di mondo, che non lo leggono.
	TMap<int32, ERTFeedRowSpecies> SpeciesByUnit;

	// 🔴 L'ambiente si raggruppa per TURNO, non per unita': la propagazione dell'acqua o del fuoco tocca
	// celle, non qualcuno, e §E vieta esplicitamente «cella d'acqua A / cella d'acqua B / cella
	// elettrificata C». Un solo evento, e il conteggio in `Amount`.
	int32 EnvironmentIndex = INDEX_NONE;

	for (const FRTTurnLogEntry& Entry : Entries)
	{
		// ── PRIMO passo, sempre. Un fatto non autorizzato non diventa un evento, e non lascia traccia
		// nemmeno come conteggio: e' la differenza fra filtrare prima e sanitizzare dopo.
		//
		// ⚠️ **Una domanda per VOCE, non una proiezione per osservatore.** E' il punto dell'overload: la
		// dominanza qui sotto tiene una riga per unita' e una per turno per l'ambiente, e quello stato
		// vive in QUESTA passata. Proiettare due volte e concatenare produrrebbe doppioni e due
		// raggruppamenti scollegati.
		if (!IsAuthorized(Entry, ObserverTeamIds))
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
			Ev.Phase = Entry.Phase; // la prima cella dell'ambiente: le successive si contano, non si fasano (#3637)
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
		// TurnLog ha **un solo** `UnitId` per voce, quindi la seconda unita' non e' recuperabile da qui.
		// Inventarla dalla `SrcCell` sarebbe dedurre un'unita' da una cella, che [D-063] vieta.
		//
		// ⏱️ **Questa riga diceva che l'unico `UnitId` e' «chi SUBISCE» (`#1150`, `#1430`). E' falso per le
		// voci di combattimento, ed e' stato misurato** (`#3271`): il produttore scrive
		// `AttackActors.Add(Attacker)` (`RTTurnManager.cpp`), e la tassonomia che dovrebbe codificarlo —
		// `URTTurnLogLibrary::IsSubjectTheSufferer` — per un `Combat`/`Hit` semplice risponde **`false`**.
		// ∴ su queste voci il soggetto e' chi **colpisce**.
		//
		// 🔑 **La conseguenza che il difetto rende visibile**: la riga del feed e' agganciata a chi ha
		// colpito, mentre il testo composto (*«Colpita — …»*) e' al femminile di chi subisce. Due
		// attaccanti diversi danno due chiavi diverse, quindi la dominanza non fonde le righe e ne
		// sopravvivono due. La lettura si chiude con la decisione che `#3271` chiede; ⛔ cio' che non deve
		// restare e' un commento che afferma il contrario del codice accanto.
		Candidate.ActionId = Entry.ActionId;
		Candidate.Amount = Entry.Amount;
		Candidate.Phase = Entry.Phase; // la striscia del registro (#3637)

		// ⛔ **La cella si copia SOLO per `AttackBlocked`, e la guardia e' strutturale** (`#2697`). Copiarla
		// per ogni voce sembrerebbe innocuo — oggi `SightBlockerCell` e' scritta in un sito solo, per questo
		// esito — ma renderebbe il guardrail di `SlideBlocked` vero per **coincidenza** invece che per
		// costruzione: basterebbe un secondo produttore che riempie il campo per un esito il cui blocker e'
		// un'unita', e il feed nominerebbe una cella che nessun filtro di conoscenza ha esaminato.
		//
		// ⚠️ La cella non viene ne' ricalcolata ne' rifiltrata: `SightBlockerForLog` ha gia' deciso alla
		// scrittura, e una seconda decisione qui sarebbe il secondo contratto di conoscenza che `#1936`
		// vieta.
		if (Type == ERTPlayerEventType::AttackBlocked)
		{
			Candidate.BlockerCell = Entry.SightBlockerCell;
		}

		const ERTFeedRowSpecies CandidateSpecies = FeedRowSpeciesOf(Entry, Type, Importance);

		if (UnitId == INDEX_NONE)
		{
			Out.Add(MoveTemp(Candidate)); // voce di mondo: nessuna dominanza da applicare
			continue;
		}

		const int32* Existing = IndexByUnit.Find(UnitId);
		if (Existing == nullptr)
		{
			SpeciesByUnit.Add(UnitId, CandidateSpecies);
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
		//
		// ➕ **Per un'unita' di chi guarda, cio' che ha SUBITO vince su cio' che ha FATTO** (`#3734`, decisione
		// d'autore A1 del 2026-10-13). Una riuscita propria cede a una notizia della stessa unita', e una notizia non
		// cede a una riuscita propria arrivata dopo; tutto il resto lo decide il rango, qui sopra. Il caso che l'ha
		// chiesta: chi scatta nel Dash e viene spinto nel Blast aveva per prima la riga silenziosa dello scatto, che
		// teneva il posto allo stesso rango e poi spariva; chi colpiva e poi cadeva leggeva il colpo.
		//
		// 🔴 **Di chi e' l'unita' lo dice la voce** (`VerdictSubject.TeamId`), e per le altre squadre la scala di
		// sempre: la riga di un colpo d'arma e' agganciata a chi COLPISCE, e quando un nemico colpisce una tua unita'
		// quella riga e' la notizia per te. Farla cedere a cio' che succede al nemico la cancellerebbe.
		const ERTFeedRowSpecies HeldSpecies = SpeciesByUnit.FindRef(UnitId);
		const bool bOwnUnit = OwnTeamId != INDEX_NONE && Entry.VerdictSubject.StableUnitId != INDEX_NONE
			&& Entry.VerdictSubject.TeamId == OwnTeamId;
		bool bReplace = DominanceRank(Candidate.Type) > DominanceRank(Held.Type);
		if (bOwnUnit && HeldSpecies == ERTFeedRowSpecies::OwnSuccess && CandidateSpecies == ERTFeedRowSpecies::News)
		{
			bReplace = true; // la riuscita propria cede
		}
		else if (bOwnUnit && HeldSpecies == ERTFeedRowSpecies::News && CandidateSpecies == ERTFeedRowSpecies::OwnSuccess)
		{
			bReplace = false; // la notizia non cede a una riuscita propria arrivata dopo
		}
		if (bReplace)
		{
			const int32 Slot = *Existing;
			Out[Slot] = MoveTemp(Candidate);
			SpeciesByUnit.Add(UnitId, CandidateSpecies);
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
