#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Replay/RTReplayViewModel.h"
#include "Replay/RTMatchHistoryLibrary.h"
#include "Replay/RTReplayManifest.h"
#include "Replay/RTReplayPrivacyLibrary.h" // FRTPublicReplayEntry: il confine di D-276 sulla superficie spettatore
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
	 * `bNewestFirst` ordina per `StartedUtc` decrescente — che e' quello che una schermata di cronologia
	 * vuole, e che **Blueprint non puo' fare da solo**: non esiste un nodo che ordini un array di struct
	 * per `FDateTime`, e l'indice e' permanentemente dal piu' vecchio perche' la riga si scrive quando la
	 * partita comincia.
	 *
	 * ⚠️ **Non apre nessun archivio**, ed e' una proprieta' dell'indice (`#416`) che questo ponte non deve
	 * rompere: una riga puo' quindi puntare a una cartella che non c'e' piu'. E' cio' che rende la lista
	 * istantanea, ed e' cio' che la schermata deve saper dire.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	bool LoadMatchList(bool bNewestFirst, TArray<FRTMatchHistoryEntry>& OutMatches) const;

	/**
	 * Chiude l'archivio aperto e **rilascia le tracce**.
	 *
	 * ⚠️ Non e' simmetria di cortesia: `FRTReplaySession::Traces` porta il TurnLog di ogni turno
	 * registrato, e un subsystem di GameInstance sopravvive a ogni caricamento di livello. Senza,
	 * un replay aperto una volta resta in memoria finche' il processo vive.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	void Close();

	// --- Apertura -----------------------------------------------------------------------------------

	/** Apre una partita come spettatore **neutrale**. I **quattro** esiti restano distinti: criterio di `#472`. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	ERTReplayOpenResult OpenMatch(const FGuid& MatchId);

	/**
	 * Apre una partita **con gli occhi di una squadra** ([D-316], `#2098`).
	 *
	 * Se l'archivio porta una traccia per `ObserverTeamId` — cioe' se il suo manifest lo elenca in
	 * `ObserverTeamIds` — da qui in poi `GetCurrentPhaseEntries` consegna solo i fatti che quella squadra
	 * era autorizzata a conoscere **quando sono accaduti**. Altrimenti apre la canonica, come `OpenMatch`.
	 *
	 * ⚠️ **Due porte e non un parametro con default**, ed e' deliberato: `UFUNCTION` non ammette
	 * l'overloading, e un solo nodo con `ObserverTeamId = -1` predefinito avrebbe reso lo **spettatore
	 * neutrale il comportamento che si ottiene dimenticandosi di scegliere**. Qui la scelta e' nel nome del
	 * nodo, e un widget che apra il replay del proprio giocatore non ci arriva per omissione.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	ERTReplayOpenResult OpenMatchAsTeam(const FGuid& MatchId, int32 ObserverTeamId);

	/**
	 * Apre una partita **con gli occhi di chi l'ha giocata**: l'osservatore lo dichiara l'archivio
	 * ([D-317], `#2156`).
	 *
	 * 🔑 **E' la porta che una schermata deve chiamare**, ed e' il motivo per cui esiste. `OpenMatchAsTeam`
	 * chiede un `TeamId` che il chiamante non ha modo di conoscere — nessuna UI sa di chi fosse una partita
	 * registrata la settimana scorsa — quindi in pratica si sarebbe finiti su `OpenMatch`, ottenendo la vista
	 * neutrale **per omissione** invece che per scelta. Qui la cosa giusta e' anche la piu' semplice.
	 *
	 * ⚠️ **Ricade sul neutrale quando l'archivio non lo dichiara**, e non e' un ripiego: un manifest `v1` o
	 * `v2`, o una partita registrata da un dedicated server, non hanno un osservatore locale — `INDEX_NONE`
	 * significa *«non c'era»*, e la vista completa e' la lettura corretta di quelle registrazioni.
	 *
	 * ⛔ **Non e' un confine di privacy e non va usata come tale.** Non impedisce di chiamare
	 * `OpenMatchAsTeam` con l'altra squadra: le tracce sono entrambe sul disco, e il confine che [D-316]
	 * chiude e' quello della superficie pubblica — chi ha accesso alla cartella ha accesso a tutto.
	 *
	 * Gli esiti sono i **quattro** di `OpenMatch`, più nessuno: un manifest illeggibile risponde
	 * `ManifestUnreadable` come sempre, invece di inventare un quinto stato per «non so chi guardava».
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	ERTReplayOpenResult OpenMatchAsRecordedObserver(const FGuid& MatchId);

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

	/**
	 * ⚠️ `BlueprintCallable` e **non** `BlueprintPure`, ed e' deliberato: un nodo pure viene rivalutato a
	 * ogni punto d'uso, e questo **copia** il manifest intero — `OrderedHashPerTurn` compreso, un `int64`
	 * per turno registrato. Legato a una property binding lo copierebbe per frame. Trovato in code review,
	 * ed e' la stessa lezione della cache di `PhasesPerTrace`, che si sarebbe persa al confine Blueprint.
	 * I widget lo prendono **una volta** all'apertura.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
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

	/**
	 * La fase come si mostra: il nome, oppure **`—`** quando non c'e' una fase corrente.
	 *
	 * ⚠️ Esiste per la stessa ragione di `GetTurnLabel`, e la sua assenza era il gemello scoperto del
	 * «Turno 0»: `Phase` vale `Planning` negli stati senza fase, e uno `Switch on ERTMatchPhase` in UMG
	 * imboccherebbe quel ramo disegnando una fase che il replay dichiara impossibile.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	FText GetPhaseLabel() const;

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

	/**
	 * Le voci della fase a schermo, nella forma **pubblica**.
	 *
	 * 🔴 **Il tipo di ritorno e' il confine di [D-276], e non una preferenza di stile.** Questa e' la sola
	 * superficie del replay che un widget puo' raggiungere — `BlueprintCallable` — ed e' cio' che quella
	 * decisione chiama *«la riproduzione spettatore»*, uno dei tre posti in cui i campi di audit non devono
	 * arrivare per errore. Finche' ha consegnato `FRTTurnLogEntry`, ogni widget legato a questo nodo
	 * leggeva `Verdict`, `OpportunityId`, `ReactionInstanceId` e `ReactionResponse` di **entrambe** le
	 * squadre. Il view model resta sulla voce completa perche' e' `USTRUCT()` e non `BlueprintType`: da li'
	 * un widget non passa, e chi fa audit ha bisogno della voce intera.
	 *
	 * ✅ **E il filtro sulle VOCI esiste dal 2026-09-03** — [D-316], `#2098` — ma **non e' qui**, ed e' la
	 * cosa da sapere prima di cercarlo in questa funzione. Le voci che questa consegna sono quelle della
	 * traccia che `OpenMatchAsTeam` ha aperto: **gia' filtrate alla registrazione**, perche' il verdetto
	 * congelato vive in memoria mentre la partita gira e non sopravvive alla serializzazione
	 * (`FRTTurnLogEntry::Verdict` e' `Transient`). Chi apre con `OpenMatch` e' uno spettatore **neutrale** e
	 * vede tutte le voci: [D-316] punto (5).
	 *
	 * 🔴 **Perche' non filtrare qui, che sembrava la sede ovvia.** A questo punto il verdetto e' azzerato
	 * dalla deserializzazione: un filtro in lettura non avrebbe nascosto **troppo poco**, avrebbe nascosto
	 * **tutto** — `AllowsTeam` e' fail-closed. Per farlo funzionare servirebbe il verdetto nel formato, che
	 * e' il conto — versione, `EntryLess`, `MixEntryFields`, 11 golden — che [D-313] ha rifiutato di pagare.
	 *
	 * ⚠️ **Fino al 2026-09-02 questa riga citava `#1525`, e la citazione era sbagliata**: quella issue
	 * riguarda il **playback della partita** — il modello che cammina nella nebbia — ed e' implementata.
	 * Il filtro delle voci del replay e' un canale diverso, ed e' rimasto senza owner finche' qualcuno non
	 * ha SEGUITO la citazione invece di crederle.
	 *
	 * ⚠️ `BlueprintCallable` per la stessa ragione di `GetManifest`: ogni chiamata rifa' un seek sulla
	 * traccia e **alloca** l'array delle voci. Come nodo pure una list view lo rieseguirebbe a ogni tick,
	 * per ogni punto d'uso. Si chiama **quando la posizione cambia**, non mentre si disegna.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	TArray<FRTPublicReplayEntry> GetCurrentPhaseEntries() const
	{
		return URTReplayPrivacyLibrary::ToPublicTrace(ViewModel.CurrentPhaseEntries());
	}

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

	/**
	 * Secondi che una fase resta a schermo in riproduzione automatica. Presentazione, non regola.
	 *
	 * ⚠️ **Il valore viene clampato**: `0`, un negativo o un `NaN` farebbero avanzare il replay di una fase
	 * per ogni `Tick`, perche' la guardia del view model e' `SecondsPerPhase > 0.f` — falsa anche per
	 * `NaN`. Uno slider di velocita' che tocchi lo zero non deve poter far scorrere la partita intera.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	void SetSecondsPerPhase(float Seconds);

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	float GetSecondsPerPhase() const { return ViewModel.SecondsPerPhase; }

	// --- Dove stanno gli archivi --------------------------------------------------------------------

	/**
	 * La radice degli archivi. Vuota = il default, `Saved/Replays`, che e' dove il recorder scrive.
	 *
	 * Esiste per i test e per una eventuale cartella d'importazione; **non e' posizione**, e' dove si
	 * cerca.
	 */
	/**
	 * ⚠️ **Cambiare la radice chiude l'archivio aperto.** Senza, il subsystem descriverebbe una partita di
	 * una cartella mentre ne elenca un'altra, e una schermata che mostra «in riproduzione» accanto alla
	 * lista direbbe due partite senza rapporto.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Replay")
	void SetReplaysRoot(const FString& Root);

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Replay")
	FString GetReplaysRoot() const;

	// ~UGameInstanceSubsystem — rilascia le tracce quando la GameInstance se ne va.
	virtual void Deinitialize() override;

private:
	/**
	 * Tutta la logica sta qui, e questo tipo non ne aggiunge: e' il criterio della issue.
	 *
	 * ⚠️ `UPROPERTY()` come `URTFrontendNavigator::Stack`, e non e' cerimonia: oggi la sessione e' dati
	 * puri, ma il giorno in cui qualcuno vi aggiungesse un `TObjectPtr<>` — un data asset dell'eroe per
	 * disegnare le voci, per dire — quel riferimento sarebbe invisibile al GC e l'oggetto verrebbe raccolto
	 * mentre il replay e' aperto. Annotarlo costa nulla e ripristina il pattern del repository.
	 */
	UPROPERTY()
	FRTReplayViewModel ViewModel;

	/** Vuoto = il default del recorder. Vedi `GetReplaysRoot`. */
	UPROPERTY()
	FString ReplaysRootOverride;
};
