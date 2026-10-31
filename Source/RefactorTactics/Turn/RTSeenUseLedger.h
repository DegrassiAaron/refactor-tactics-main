#pragma once

#include "CoreMinimal.h"
#include "Turn/RTTurnLog.h"

/**
 * LE AZIONI CHE UNA SQUADRA HA VISTO USARE (`#3751`, fetta F6 della spec
 * `docs/superpowers/specs/2026-10-13-ispettore-del-mondo-design.md`, regola P4).
 *
 * Alimenta le **ricariche dedotte** della scheda di un nemico: l'ultimo turno in cui la squadra ha visto un'unita'
 * usare un'azione. Le ricariche VERE dell'avversario restano private (`ARTUnit::AbilityCooldowns`): rivelerebbero
 * anche gli usi avvenuti fuori vista. Qui entra soltanto cio' che la squadra e' autorizzata a leggere.
 *
 * 🔑 **E' funzione di due cose sole** (P4): il catalogo, che sta fuori da questo registro, e le voci del TurnLog il
 * cui verdetto autorizza la squadra, sull'INTERA partita (il `TurnLog` vive per turno, questo no). Non guarda
 * Actor, piani, slot ne' stato di partita: una voce che non porta il soggetto, l'azione e il turno non conta.
 *
 * ⛔ **Presentazione, non stato canonico.** Non entra in hash, TurnLog, snapshot ne' replay; si ricostruisce dal
 * TurnLog autorizzato. Vive accanto a `ARTTurnManager::TeamKnowledgeState`, che ha la stessa vita (una partita) e
 * lo stesso confine di privacy, e lo alimenta `AppendLogEntry` — l'unico punto che scrive il registro.
 */
struct REFACTORTACTICS_API FRTSeenUseLedger
{
	/**
	 * La voce dice che **il suo soggetto** (`UnitId`) ha USATO l'azione che porta (`ActionId`)?
	 *
	 * 🔴 **Non basta `UnitId` + `ActionId`**, ed e' il punto di P4: ci sono voci che nominano un'unita' come ALTRO
	 * capo e portano l'azione di qualcun altro. `Move/Displaced` ha `UnitId` = chi viene spostato e `ActionId` =
	 * l'azione di chi spinge (`#307`); il danno ambientale e l'azione cancellata da un `Interrupt` hanno `UnitId` =
	 * chi subisce (`IsSubjectTheSufferer`). Attribuire a quell'unita' l'azione dell'altro sarebbe un numero
	 * plausibile e sbagliato, e nessun errore lo segnalerebbe.
	 *
	 * Per questo la risposta e' una **lista di casi in cui il registro SA** chi ha agito, e non l'esclusione di
	 * quelli noti: una categoria che nessuno ha esaminato tace. Contano:
	 *   · il danno inflitto da chi agisce (`IsDamageInflictedByActor`: colpi, Overwatch che spara, previsione
	 *     azzeccata) — la tassonomia che il TurnLog gia' porta, non una seconda;
	 *   · la cura (`Combat/Healed`), che ha un agente vero.
	 *
	 * Non contano, e il limite e' dichiarato: movimento e Dash, azioni ambientali, stati, fallback, un attacco
	 * fermato dalla copertura (`NoLineOfSight`: la ricarica non e' detto che sia stata pagata), una cura su se
	 * stessi (`IsSubjectTheSufferer` la legge come ambientale). Per quelle azioni la scheda non ha riga: la
	 * deduzione tace, non indovina.
	 *
	 * ⌫ *Ritrattato il 2026-10-31 (`#3134`, [D-414]): «la ricarica non e' detto che sia stata pagata» non e' piu' vero per
	 * l'attacco fermato dalla copertura, che da `R4` PAGA anche senza colpo. La deduzione tace per un'altra ragione: il
	 * registro conta solo danno e cura e non distingue l'USO, quindi un attacco bloccato non lascia nessuna voce da cui
	 * dedurre chi ha agito. Il silenzio non dice piu' niente sulla ricarica.*
	 */
	static bool IsUseByItsSubject(const FRTTurnLogEntry& Entry);

	/**
	 * Registra la voce per ogni squadra fra `ObserverTeamIds` che il suo verdetto autorizza.
	 *
	 * Il predicato di autorizzazione e' `FRTKnowledgeVerdict::AllowsTeam`, lo stesso che sta sotto
	 * `URTPlayerEventProjector::IsAuthorized` (il filtro del registro a schermo): un test li tiene allineati,
	 * perche' `Turn/` non puo' includere `UI/`.
	 */
	void Observe(const FRTTurnLogEntry& Entry, TConstArrayView<int32> ObserverTeamIds);

	/** L'ultimo turno in cui `ObserverTeamId` ha visto `UnitId` usare `ActionId`. `0` = mai visto. */
	int32 LastSeenTurn(int32 ObserverTeamId, int32 UnitId, FName ActionId) const;

	/** Quante coppie (squadra, unita', azione) conosce. */
	int32 Num() const { return Rows.Num(); }

private:
	/** Un array e non una `TMap`: l'ordine di una mappa non deve decidere niente, e le righe sono poche. */
	struct FRow
	{
		int32 TeamId = INDEX_NONE;
		int32 UnitId = 0;
		FName ActionId;
		int32 Turn = 0;
	};

	TArray<FRow> Rows;
};
