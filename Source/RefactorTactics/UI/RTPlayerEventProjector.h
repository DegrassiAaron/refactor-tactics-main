#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UI/RTPlayerEvent.h"
#include "RTPlayerEventProjector.generated.h"

struct FRTTurnLogEntry;

/**
 * Dal `TurnLog` canonico al feed di chi gioca — `#1936` §C, epic `#1937`.
 *
 * ```text
 * Resolver autorevole -> TurnLog canonico -> predicato di autorizzazione -> QUI -> FRTPlayerEvent[]
 * ```
 *
 * 🔴 **Pura e non autoritativa.** Legge il `TurnLog`, non lo tocca, e non entra nel suo hash: la
 * derivazione degli eventi giocatore e' a valle di tutto cio' che decide la partita. Se questa classe
 * sparisse, nessun esito cambierebbe.
 *
 * ⛔ **Tre divieti, e nascono dal difetto che `#295` ha gia' chiuso una volta:**
 *
 * · non si deriva un evento facendo il **parsing** di una stringa diagnostica — il testo e' una vista, non
 *   una fonte;
 * · non si **ricalcola** la conoscenza: si consuma il verdetto che ogni voce porta gia', congelato quando
 *   la voce e' nata ([D-223]);
 * · non si proietta prima e si filtra dopo. L'autorizzazione e' il **primo** passo, e un fatto non
 *   autorizzato non diventa mai un evento — nemmeno un evento vuoto, un conteggio o un tipo.
 *
 * 🔑 **Il predicato non e' nuovo, ed e' la misura che ha ridotto lo scope di `#1936`.** Il corpo della issue
 * chiedeva di estrarne uno da `ComposeVisibleLogLines`, *«dove filtro e composizione stanno nella stessa
 * funzione»*. Misurato il 2026-09-02: non e' cosi'. Quella funzione riceve righe **gia' composte** e applica
 * `FRTKnowledgeVerdict::AllowsTeam`, che vive in `Perception/RTTeamKnowledge.h` ed e' gia' isolato e
 * fail-closed. Qui si chiama **lo stesso**, sul `Verdict` che ogni `FRTTurnLogEntry` porta — nessuna
 * estrazione, nessun secondo contratto di conoscenza.
 */
UCLASS()
class REFACTORTACTICS_API URTPlayerEventProjector : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Proietta le voci autorizzate per `ObserverTeamId` in eventi leggibili, in ordine di produzione.
	 *
	 * L'ordine e' quello del `TurnLog` ricevuto e non viene riordinato: e' l'ordine di risoluzione, ed e'
	 * cio' che rende il feed una cronaca invece di un elenco.
	 *
	 * @param Entries          il `TurnLog` canonico, completo. Non viene modificato.
	 * @param ObserverTeamId   chi guarda. Fuori intervallo -> nessun evento (fail-closed di `AllowsTeam`).
	 */
	static TArray<FRTPlayerEvent> Project(const TArray<FRTTurnLogEntry>& Entries, int32 ObserverTeamId);

	/**
	 * Lo stesso, per un INSIEME di osservatori autorizzati — la sessione non presidiata (`#2744`).
	 *
	 * 🔴 **Esiste perche' chiamare l'overload singolo due volte e' SBAGLIATO, e in due modi.** Una voce
	 * autorizzata per entrambe le squadre — un KO pubblico — comparirebbe **due volte**; ed e' la stessa
	 * trappola che `ARTHUD::ComposeVisibleIntentViews` documenta gia' per gli intenti (*«due
	 * `FilterForTeam` sull'insieme intero produrrebbero DOPPIONI»*). ⌫ Quella frase stava in
	 * `ARTHUD::DrawHUD` fino al 2026-09-24, quando `#2184` ne ha estratto la decisione: il puntatore e'
	 * aggiornato, la trappola e' la stessa. In piu' `IndexByUnit` ed `EnvironmentIndex` sono stato **per
	 * proiezione**: due chiamate produrrebbero due raggruppamenti indipendenti, e la dominanza — la regola
	 * che tiene una riga sola per unita' — si applicherebbe due volte a meta' dei fatti.
	 *
	 * ∴ La decisione resta **una per voce**: si chiede all'insieme, non si somma il risultato di due
	 * domande.
	 *
	 * 🔑 **Il nome dell'insieme non e' nuovo**: `FRTReplayManifest::ObserverTeamIds` (`D-316`) chiama gia'
	 * cosi' *«le squadre per cui esiste una traccia pubblica filtrata per osservatore»*. Stesso concetto,
	 * stesso tipo, stesso nome.
	 *
	 * @param ObserverTeamIds  chi guarda. **Vuoto -> nessun evento**: il fail-closed di `AllowsTeam` non si
	 *                         allenta passando per un contenitore, e un insieme vuoto non significa «tutti».
	 */
	static TArray<FRTPlayerEvent> Project(const TArray<FRTTurnLogEntry>& Entries,
		const TArray<int32>& ObserverTeamIds);

	/**
	 * Lo stesso, sapendo QUALI unita' sono di chi guarda (`#3734`, decisione d'autore del 2026-10-13).
	 *
	 * 🔑 **Per un'unita' di chi guarda, cio' che ha SUBITO vince su cio' che ha FATTO.** Una riga e' una
	 * *riuscita propria* quando racconta un'azione scelta e andata a segno: il movimento arrivato, il colpo
	 * dato, la cura data. Per le unita' in `OwnUnitIds` una riuscita propria cede a qualunque altra riga della
	 * stessa unita' nel turno — spinta, caduta, stato, piano fallito, reazione, KO — e una di queste non cede a
	 * una riuscita propria arrivata dopo. Dentro ciascuna delle due specie decide la scala di sempre
	 * (`DominanceRank`), e a parita' resta la prima arrivata.
	 *
	 * ⚠️ **Per le altre unita' la scala di sempre, invariata, e non per prudenza.** La riga di un colpo d'arma e'
	 * agganciata a chi COLPISCE (`IsSubjectTheSufferer` e' falso per un `Hit`): quando un nemico colpisce una tua
	 * unita', quella riga e' la notizia che la tua unita' e' stata colpita, e non deve cedere a cio' che e'
	 * successo al nemico nello stesso turno.
	 *
	 * @param OwnUnitIds  `StableUnitId` delle unita' di chi guarda. **Vuoto -> la scala di sempre per tutte**:
	 *                    e' cio' che fanno i due overload senza questo parametro.
	 */
	static TArray<FRTPlayerEvent> Project(const TArray<FRTTurnLogEntry>& Entries,
		const TArray<int32>& ObserverTeamIds, const TSet<int32>& OwnUnitIds);

	/**
	 * Il solo passo di autorizzazione, esposto perche' e' **verificabile**.
	 *
	 * Esiste per `UI.PlayerEventLog.AuthorizationMatchesLogLines`: senza un punto nominabile, «il proiettore
	 * e il canale testuale ammettono lo stesso insieme di fatti» sarebbe un'affermazione invece che
	 * un'assertion. Chiama `AllowsTeam` e nient'altro.
	 */
	static bool IsAuthorized(const FRTTurnLogEntry& Entry, int32 ObserverTeamId);

	/**
	 * Autorizzata per **almeno uno** degli osservatori. Insieme vuoto -> `false`, sempre.
	 *
	 * ⛔ Non e' una guardia ammorbidita: `AllowsTeam` decide ancora da sola per ogni squadra, e questa
	 * funzione non fa altro che chiederglielo piu' volte. Una squadra che non e' nell'insieme non vede
	 * niente in piu' di prima.
	 */
	static bool IsAuthorized(const FRTTurnLogEntry& Entry, const TArray<int32>& ObserverTeamIds);
};
