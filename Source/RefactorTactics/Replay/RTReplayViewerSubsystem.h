#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Replay/RTReplayViewModel.h"
#include "Replay/RTMatchHistoryLibrary.h"
#include "Replay/RTReplayManifest.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnRules.h"
#include "RTReplayViewerSubsystem.generated.h"

/**
 * Il **ponte Blueprint** del replay: l'altra meta' del pattern che `FRTReplayViewModel` da solo non ha
 * (`#999`).
 *
 * ## Perche' esiste
 *
 * Il view model e' `USTRUCT()` e non `BlueprintType`, quindi un widget UMG **non puo' raggiungerlo**: non
 * puo' dichiararne una variabile ne' chiamarne i metodi, che sono C++ puro. Le tre librerie del replay
 * hanno lo stesso stato — `grep -cE "^\s*UFUNCTION\(" ` su `RTMatchHistoryLibrary.h`,
 * `RTReplayPlayerLibrary.h` e `RTReplaySeekLibrary.h` da' **zero** su tutte e tre. Senza questo tipo,
 * `WBP_RT_MatchHistory` e `WBP_RT_ReplayViewer` non hanno niente da chiamare, e il lavoro da editor
 * produce layout scollegati.
 *
 * E' lo stesso ruolo che `URTFrontendNavigator` ha per `FRTScreenStack` e `URTHudViewModel` per
 * `FRTMatchHeaderView`: la logica sta in uno `USTRUCT` che si prova senza mondo, l'accesso passa da un
 * `UObject`.
 *
 * ## Perche' un `UGameInstanceSubsystem`
 *
 * Perche' la lista delle partite **non appartiene a una mappa** e deve sopravvivere al cambio di livello:
 * si apre un replay dal menu, prima che esista un mondo di gioco. E' la stessa ragione per cui
 * `URTFrontendNavigator` e' un subsystem e non un componente.
 *
 * ## ⚠️ Cosa questo tipo NON fa, ed e' un criterio della issue
 *
 * **Non decide dove si e'.** Ogni funzione qui e' un inoltro al view model: posizione, bordi, ritmo e
 * contenuto restano di la', dove sono provati senza mondo e senza widget. Un ponte che cominciasse a
 * calcolare avrebbe due sorgenti di verita' sulla stessa domanda, e sarebbe la seconda a decidere cosa il
 * giocatore vede — che e' esattamente cio' che `#472` vieta per il posizionamento.
 *
 * Fanno eccezione **due sole** cose, ed entrambe sono presentazione o configurazione, non posizione:
 * la radice degli archivi (`SetReplaysRoot`) e l'etichetta del turno (`GetTurnLabel`).
 *
 * **Non avvia il frontend.** `InitializeFrontend`/`RegisterScreen` sono di E46 (`#938`): questo tipo
 * espone, non avvia.
 *
 * **Non chiama il resolver.** Guarda gli `#include`: il view model, l'indice, il manifest, il TurnLog e
 * le regole di turno. Nessuna catena raggiunge `RTCombatResolver`, `RTHexSimLibrary` o `ARTTurnManager`,
 * quindi il confine di [ADR-0009](../../../docs/decisions/adr-0009-replay-logico-canonico.md) §3 vale
 * anche qui **per costruzione** — ed e' il punto piu' delicato, perche' questo e' il primo tipo del
 * replay che Blueprint puo' toccare.
 */
UCLASS()
class REFACTORTACTICS_API URTReplayViewerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// --- La lista -----------------------------------------------------------------------------------

	/**
	 * Le partite registrate, dall'indice. `false` = nessun indice leggibile, e `OutMatches` resta com'e'.
	 *
	 * ⚠️ **Non apre nessun archivio**, ed e' una proprieta' dell'indice (`#416`) che questo ponte non deve
	 * rompere: una riga puo' quindi puntare a una cartella che non c'e' piu'. E' cio' che rende la lista
	 * istantanea, ed e' cio' che la schermata deve saper dire.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	bool LoadMatchList(TArray<FRTMatchHistoryEntry>& OutMatches) const;

	// --- Apertura -----------------------------------------------------------------------------------

	/** Apre una partita. I **quattro** esiti restano distinti: e' un criterio di `#472`. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	ERTReplayOpenResult OpenMatch(const FGuid& MatchId);

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	bool IsOpen() const { return ViewModel.IsOpen(); }

	/** L'esito dell'ultima apertura. Da leggere con `HasAttemptedOpen()`, che dice se la domanda si applica. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	ERTReplayOpenResult GetLastOpenResult() const { return ViewModel.LastOpenResult(); }

	/** `false` = nessuno ha mai provato ad aprire: distingue «non ho chiesto» da «il manifest e' illeggibile». */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	bool HasAttemptedOpen() const { return ViewModel.HasAttemptedOpen(); }

	/** `true` = aperto **e** completo. `false` copre «parziale» e «nessun archivio»: distinguili con `IsOpen()`. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	bool IsArchiveComplete() const { return ViewModel.IsComplete(); }

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	FRTReplayManifest GetManifest() const { return ViewModel.GetManifest(); }

	// --- Dove siamo ---------------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	FRTReplayPosition GetPosition() const { return ViewModel.Position(); }

	/**
	 * I due guard di `FRTReplayPosition`, esposti.
	 *
	 * ⚠️ **Servono perche' i metodi di uno `USTRUCT` non sono chiamabili da Blueprint**, nemmeno quando la
	 * struttura e' `BlueprintType`. Senza, ogni binding ri-deriverebbe `State == AtPhase || State ==
	 * Unaddressable` in graph nodes — la duplicazione che quei guard esistono per evitare, spostata dove
	 * nessun test la vede.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	bool PositionHasPhase() const { return ViewModel.Position().HasPhase(); }

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	bool PositionHasTurn() const { return ViewModel.Position().HasTurn(); }

	/**
	 * Il turno come si mostra: il numero, oppure **`—`** quando non e' dichiarabile.
	 *
	 * ⚠️ E' l'unica formattazione che questo tipo fa, ed e' deliberato: la decisione «una traccia non
	 * indirizzabile non si stampa `Turno 0`» e' una **regola**, non uno stile, e lasciarla al widget
	 * significherebbe riscriverla in ogni schermata che mostra una posizione. Il resto della composizione
	 * — il testo intorno, il layout, la lingua — resta di chi disegna.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	FText GetTurnLabel() const;

	// --- Comandi ------------------------------------------------------------------------------------
	//
	// I quattro `Can*` pilotano `IsEnabled` e rispondono alla stessa domanda dei quattro `Step*`: ai bordi
	// il comando e' **disabilitato, non silenzioso**.

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	bool CanStepPhaseForward() const { return ViewModel.CanStepPhaseForward(); }

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	bool CanStepPhaseBackward() const { return ViewModel.CanStepPhaseBackward(); }

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	bool CanStepTurnForward() const { return ViewModel.CanStepTurnForward(); }

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	bool CanStepTurnBackward() const { return ViewModel.CanStepTurnBackward(); }

	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	bool StepPhaseForward() { return ViewModel.StepPhaseForward(); }

	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	bool StepPhaseBackward() { return ViewModel.StepPhaseBackward(); }

	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	bool StepTurnForward() { return ViewModel.StepTurnForward(); }

	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	bool StepTurnBackward() { return ViewModel.StepTurnBackward(); }

	/** Salto diretto a un turno **dichiarato**. Fail-closed: un turno che non c'e' non muove la posizione. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	ERTReplaySeekResult SeekToTurn(int32 TurnNumber) { return ViewModel.SeekToTurn(TurnNumber); }

	/** Salto a una fase del turno corrente. Le fasi non osservabili sono **rifiutate**, non seguite. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	ERTReplaySeekResult SeekToPhaseInCurrentTurn(ERTMatchPhase Phase)
	{
		return ViewModel.SeekToPhaseInCurrentTurn(Phase);
	}

	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	void Rewind() { ViewModel.Rewind(); }

	// --- Cosa c'e' da disegnare ---------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	TArray<FRTTurnLogEntry> GetCurrentPhaseEntries() const { return ViewModel.CurrentPhaseEntries(); }

	/** Le fasi che il turno corrente contiene **davvero**: e' la barra dei salti, e non si costruisce dall'enum. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	TArray<ERTMatchPhase> GetPhasesInCurrentTurn() const { return ViewModel.PhasesInCurrentTurn(); }

	/**
	 * Le **cinque** fasi che un replay puo' mostrare. `ERTMatchPhase` ne dichiara sette: `Planning` e
	 * `MatchEnded` non sono mai emesse, e una UI che enumerasse l'enum disegnerebbe due comandi morti.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	static TArray<ERTMatchPhase> GetObservablePhases() { return FRTReplayViewModel::ObservablePhases(); }

	// --- Riproduzione -------------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	void Play() { ViewModel.Play(); }

	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	void Pause() { ViewModel.Pause(); }

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	bool IsPlaying() const { return ViewModel.IsPlaying(); }

	/**
	 * Fa scorrere il tempo. `true` se la posizione e' cambiata.
	 *
	 * ⚠️ Il subsystem **non ha un tick proprio**, ed e' voluto: chiamarlo e' del widget, che sa quando e'
	 * a schermo. Un tick automatico farebbe avanzare un replay che nessuno sta guardando.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	bool Tick(float DeltaSeconds) { return ViewModel.Tick(DeltaSeconds); }

	/** Secondi che una fase resta a schermo in riproduzione automatica. Presentazione, non regola. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	void SetSecondsPerPhase(float Seconds) { ViewModel.SecondsPerPhase = Seconds; }

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	float GetSecondsPerPhase() const { return ViewModel.SecondsPerPhase; }

	// --- Dove stanno gli archivi --------------------------------------------------------------------

	/**
	 * La radice degli archivi. Vuota = il default, `Saved/Replays`, che e' dove il recorder scrive.
	 *
	 * Esiste per i test e per una eventuale cartella d'importazione; **non e' posizione**, e' dove si
	 * cerca.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	void SetReplaysRoot(const FString& Root) { ReplaysRootOverride = Root; }

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	FString GetReplaysRoot() const;

private:
	/** Tutta la logica sta qui, e questo tipo non ne aggiunge: e' il criterio della issue. */
	FRTReplayViewModel ViewModel;

	/** Vuoto = `Saved/Replays`. Vedi `GetReplaysRoot`. */
	FString ReplaysRootOverride;
};
