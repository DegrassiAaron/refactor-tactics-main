#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

class UWorld;
class FRTScenarioSession;

/**
 * Cosa e' successo alla richiesta di eseguire uno scenario. **Tre esiti perche' i casi sono tre**, e il
 * terzo e' quello che si perde quando la firma e' un `bool`:
 *
 *  - nessuno ha chiesto niente        -> si allestisce la partita normale;
 *  - qualcuno ha chiesto uno scenario che non si carica -> **non** si allestisce niente (fail-closed);
 *  - lo scenario e' partito           -> la partita normale non viene allestita.
 *
 * ⚠️ Il secondo e il terzo hanno lo stesso effetto sull'allestimento e ragioni opposte: unificarli
 * renderebbe indistinguibile «ho chiesto uno scenario e sta girando» da «ho chiesto uno scenario e non
 * esiste», che e' esattamente la confusione che il fail-closed esiste per evitare.
 */
enum class ERTScenarioStart : uint8
{
	/** `ScenarioId` vuoto: nessuna richiesta. Il chiamante prosegue con la partita normale. */
	NotRequested,

	/**
	 * Richiesto ma non caricabile — ID sconosciuto, ambiguo, o file illeggibile. Il motivo e' gia' nel log.
	 *
	 * 🔴 **Il chiamante NON deve ripiegare sulla partita normale**: chi ha scritto un ID sbagliato si
	 * ritroverebbe a giocare una partita che non ha chiesto, e la sola traccia sarebbe una riga di log che
	 * non si ha motivo di aprire.
	 */
	NotLoadable,

	/**
	 * La sessione e' stata avviata. ⚠️ Vale **anche** se `FRTScenarioSession::Start` ha fallito: l'errore e'
	 * gia' nel risultato della sessione e la partita normale resta comunque non allestita — allestirla a
	 * quel punto sarebbe il ripiego silenzioso che la riga sopra vieta.
	 */
	Started
};

/**
 * IL CICLO DI VITA A RUNTIME DI UNO SCENARIO: caricamento, sessione, avanzamento, referto.
 *
 * ## Perche' esiste, e cosa NON decide
 *
 * `ARTGameMode` decide **se** questa sessione e' una run di scenario o una partita — e' la sua scelta di
 * sempre, e resta sua perche' e' la stessa domanda a cui rispondono `MapSource` e il formato. Cio' che non
 * gli appartiene e' il **come**: `URTScenarioIndex`, `URTScenarioLoader`, `FRTScenarioSession` e
 * `URTTestReportWriter` sono quattro collaboratori dell'harness, e un orchestratore che li nomina tutti e
 * quattro ha una ragione di cambiare per ognuno di loro.
 *
 * ⛔ **Non risolve nessuna precedenza.** L'ID e l'etichetta della fonte arrivano gia' decisi: la scala
 * `proprieta' < -RTScenario= < rt.Test.Scenario` vive nel GameMode, dove vivono anche le altre due
 * (`MapSource`, autobattle), e spostarla qui la separerebbe dalle sorelle senza guadagno.
 *
 * ⛔ **Non tocca la camera.** L'inquadratura dello scenario e' presentazione e ha bisogno del ciclo di vita
 * di un Actor (`SetTimerForNextTick` su un `WeakLambda`): resta di chi quell'Actor lo e'.
 *
 * Classe C++ pura e non `UObject`: nessun delegate dinamico da ricevere, nessuna reflection da esporre,
 * nessuna proprieta' da editare. La sessione che possiede e' gia' un `TSharedPtr` di un tipo non-UObject.
 */
class REFACTORTACTICS_API FRTScenarioCoordinator
{
public:
	/**
	 * Carica lo scenario e avvia la sessione.
	 *
	 * @param World          il mondo su cui allestire lo scenario.
	 * @param ScenarioId     l'ID gia' risolto. Vuoto -> `NotRequested`, e non viene toccato niente.
	 * @param SourceLabel    chi ha scelto questo scenario, per il log dell'AUTO-RUN. E' un'informazione che
	 *                       il coordinatore non puo' avere: la precedenza la risolve il chiamante.
	 * @param InTurnPauseSeconds pausa fra un turno e l'altro: e' cio' che rende lo scenario osservabile.
	 */
	ERTScenarioStart Start(UWorld* World, const FString& ScenarioId, const FString& SourceLabel,
		float InTurnPauseSeconds);

	/**
	 * Fa avanzare la sessione di un passo, e alla fine scrive il referto.
	 *
	 * ⚠️ **Un passo per frame, e il turn manager NON viene pompato**: in gioco lo ticca gia' il mondo, e
	 * pomparlo anche da qui farebbe correre il playback al doppio della velocita' — cioe' proprio cio' che
	 * si vuole guardare passerebbe in meta' del tempo.
	 *
	 * Senza sessione, o a sessione finita, non fa nulla: chiamarla a ogni tick e' sicuro.
	 */
	void Tick(float DeltaSeconds);

	/** Vero finche' la sessione sta girando. Falso anche quando non e' mai partita. */
	bool IsRunning() const;

	/**
	 * L'esito quando la sessione ha finito; **stringa vuota** se non e' partita o sta ancora girando.
	 *
	 * Il vuoto e' un terzo stato e non un esito mancante: chi disegna la banda lo traduce in «in corso»,
	 * che e' la sola cosa vera prima che ci sia un verdetto.
	 */
	FString OutcomeString() const;

private:
	/** La sessione in corso, o nulla. Avanza un passo per frame da `Tick`. */
	TSharedPtr<FRTScenarioSession> Session;
};
