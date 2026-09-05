#pragma once

#include "CoreMinimal.h"
#include "Replay/RTReplayPlayerLibrary.h"
#include "Replay/RTReplaySeekLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnRules.h"
#include "RTReplayViewModel.generated.h"

/**
 * In che stato si trova la riproduzione, dal punto di vista di chi la guarda (`#472`).
 *
 * ⚠️ **L'elenco e' chiuso e copre i quattro stati che il criterio di #472 nomina.** Non e' una comodita':
 * il criterio dice *«dichiara SEMPRE turno e fase correnti»*, e «sempre» e' verificabile solo se gli stati
 * in cui deve valere sono enumerati. Un quinto stato che comparisse senza entrare qui renderebbe il
 * criterio di nuovo indimostrabile.
 */
UENUM(BlueprintType)
enum class ERTReplayPositionState : uint8
{
	/** Archivio aperto, nessuna fase ancora emessa. Non e' «turno 1»: e' *prima* del turno 1. */
	BeforeStart,
	/** Posizione piena: `TurnNumber` e `Phase` valgono entrambi. */
	AtPhase,
	/** La sequenza e' finita. `Phase` e `TurnNumber` NON valgono — l'ultima fase e' gia' stata mostrata. */
	Ended,
	/**
	 * Si e' a una fase, ma il turno non e' dichiarabile: `Phase` vale, `TurnNumber` **no**.
	 *
	 * Non e' un errore ne' un caso teorico. Le tracce scritte prima del formato v6 dichiarano `0`, che e'
	 * il sentinella «non dichiarato» e non un turno: `URTReplaySeekLibrary::SeekToTurn` lo rifiuta per
	 * fail-closed. Un archivio simile si apre e si guarda — semplicemente non e' indirizzabile per turno,
	 * e l'interfaccia deve dirlo invece di stampare «Turno 0».
	 */
	Unaddressable
};

/**
 * Dove si trova la riproduzione. E' la risposta alla domanda «dove sono», che meta' di `#472` esiste per
 * dare.
 *
 * 🔴 **Non si costruisce dal cursore, ed e' il difetto che questa struttura esiste per rendere
 * impossibile.** `URTReplayPlayerLibrary::AdvancePhase` emette le voci di una fase e lascia il cursore
 * **oltre** di esse; il suo `while` di testa puo' aver gia' cambiato traccia. Quindi
 * `Traces[Cursor.TraceIndex][0].TurnNumber` NON e' il turno di cio' che si sta guardando — e' il
 * prossimo. Una UI che leggesse la posizione dal cursore mostrerebbe il turno sbagliato a ogni fine
 * turno, una volta per turno, sempre.
 */
USTRUCT(BlueprintType)
struct FRTReplayPosition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	ERTReplayPositionState State = ERTReplayPositionState::BeforeStart;

	/** Valido **solo** con `State == AtPhase`. Negli altri tre stati e' `0` e non va mostrato come turno. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 TurnNumber = 0;

	/**
	 * Valida con `AtPhase` e con `Unaddressable`. Con `BeforeStart` e `Ended` non c'e' fase corrente.
	 *
	 * 🔴 **Il default e' `Planning` perche' e' l'unico valore che NON puo' essere una fase mostrata**, ed
	 * e' la stessa scelta che `TurnNumber = 0` fa per il turno. La prima stesura usava `Prep`, che e' una
	 * fase osservabile vera: negli stati senza fase il campo portava un valore plausibile, e un binding
	 * che lo leggesse senza guardare `State` avrebbe stampato «Prep» prima dell'inizio e di nuovo dopo la
	 * fine. Trovato in code review — ed e' lo stesso default di `FRTMatchHeaderView::Phase`.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	ERTMatchPhase Phase = ERTMatchPhase::Planning;

	/** `true` quando `Phase` porta un valore leggibile: `AtPhase` o `Unaddressable`. */
	bool HasPhase() const
	{
		return State == ERTReplayPositionState::AtPhase || State == ERTReplayPositionState::Unaddressable;
	}

	/** `true` solo quando anche il turno e' dichiarabile. */
	bool HasTurn() const { return State == ERTReplayPositionState::AtPhase; }
};

/**
 * Il view model del replay: posizione dichiarata, stato di riproduzione, comandi abilitati (`#472`).
 *
 * ## Perche' esiste, invece di far chiamare le librerie ai widget
 *
 * Perche' `#472` dichiarava *«cablaggio e leggibilita', non logica nuova»* e la revisione del 2026-08-16
 * l'ha falsificato in due punti misurati
 * ([`plans/replay-r6-spec-panel-2026-08-16.md`](../../../docs/roadmap/plans/replay-r6-spec-panel-2026-08-16.md) §4.3):
 * la posizione **non** si legge dal cursore, e play/pausa e' stato che nessuna libreria possiede —
 * `URTPlaybackLibrary` e' matematica pura senza clock. Quella logica esiste comunque: la scelta e' se
 * viva in un tipo che si prova, o dentro un `.uasset` che si ispeziona.
 *
 * E' lo stesso pattern che il repository ha gia' scelto due volte, per la stessa ragione:
 * `FRTMatchHeaderView` (l'HUD non ricalcola) e `FRTScreenStack` (*«la navigazione non e' UI: e' una
 * macchina a stati che governa la UI»*).
 *
 * ⚠️ **Il parallelo con quei due e' pero' incompleto oggi, e va detto invece di lasciarlo intendere.**
 * Entrambi hanno una superficie Blueprint — `FRTMatchHeaderView` passa dalle `UFUNCTION` di
 * `URTHudViewModel`, `FRTScreenStack` da quelle di `URTFrontendNavigator` — mentre questo tipo non ne ha
 * ancora nessuna: e' `USTRUCT()` e non `BlueprintType`, quindi **un widget UMG non puo' raggiungerlo**.
 * `FRTReplayPosition` e `ERTReplayPositionState` *sono* `BlueprintType`, quindi il dato da mostrare e'
 * gia' visibile a Blueprint mentre nulla, in Blueprint, puo' produrlo. Trovato in code review.
 *
 * Non e' una svista di questo file: e' il pezzo successivo di `#472` — un `UGameInstanceSubsystem` che
 * possieda il view model e lo esponga, come `URTFrontendNavigator` fa per lo stack. Finche' non esiste,
 * il lavoro da editor puo' produrre **layout**, non un viewer collegato. Le librerie replay hanno lo
 * stesso stato: `grep -c UFUNCTION` su `RTMatchHistoryLibrary.h`, `RTReplayPlayerLibrary.h` e
 * `RTReplaySeekLibrary.h` da' **zero** su tutte e tre.
 *
 * ## Cosa NON fa
 *
 * - **Non ricalcola niente.** Vale qui l'invariante del Player: autorevole e' la traccia
 *   ([ADR-0009](../../../docs/decisions/adr-0009-replay-logico-canonico.md) §2). Guarda gli `#include`:
 *   il Player, il seek, il TurnLog, le regole di turno. Nessuna catena raggiunge `RTCombatResolver`,
 *   `RTHexSimLibrary` o `ARTTurnManager`, quindi «la UI non chiama il resolver» non e' una promessa da
 *   mantenere ma **l'assenza della possibilita' di violarla** — la stessa forma che l'ADR §3 ha preso da
 *   `URTReplaySeekLibrary` come precedente.
 * - **Non apre un secondo meccanismo di posizionamento.** Ogni movimento passa da `URTReplaySeekLibrary`
 *   (`#415`), ed e' un criterio di accettazione, non uno stile: due meccanismi divergerebbero al primo
 *   caso limite, e sarebbe quello a decidere cosa il giocatore vede.
 * - **Non conosce i widget.** Nessun `UUserWidget`, nessun `UWorld`: si prova headless.
 * - **Non interpola e non misura durate**: quello e' `URTPlaybackLibrary`, ed e' presentazione.
 *   Qui il tempo entra da un solo punto — `Tick` — e serve unicamente a decidere *quando* avanzare.
 */
USTRUCT()
struct REFACTORTACTICS_API FRTReplayViewModel
{
	GENERATED_BODY()

	/**
	 * Le fasi che un replay puo' davvero mostrare: **cinque**, non sette.
	 *
	 * ⚠️ `ERTMatchPhase` ne dichiara sette, ma nessun punto del resolver emette voci con `Planning` o
	 * `MatchEnded`, quindi un seek a quelle due risponde **sempre** `PhaseNotFound`
	 * (`RTReplaySeekLibrary.h`). Una UI che enumerasse l'enum disegnerebbe due comandi morti — ed e' il
	 * motivo per cui questa lista sta qui e non nel widget: enumerare l'enum e' l'errore *naturale*, e va
	 * reso non necessario invece che vietato in prosa.
	 */
	static const TArray<ERTMatchPhase>& ObservablePhases();

	/**
	 * Apre un archivio e si posiziona **prima** dell'inizio.
	 *
	 * Delega interamente a `URTReplayPlayerLibrary::OpenArchive`: i quattro esiti sono i suoi, e questa
	 * funzione non ne aggiunge ne' ne nasconde. Su un esito diverso da `Opened` il view model resta
	 * **inutilizzabile e lo dichiara** (`IsOpen() == false`), invece di restare a meta'.
	 *
	 * `ObserverTeamId` sceglie il prodotto da aprire ([D-316], `#2098`): una squadra dichiarata
	 * nell'archivio apre la traccia **filtrata alla registrazione**; `INDEX_NONE` — il default — apre la
	 * canonica come spettatore neutrale. Il contratto completo sta su `OpenArchive`, e non si duplica qui.
	 */
	ERTReplayOpenResult Open(const FString& ReplaysRoot, const FGuid& MatchId,
		int32 ObserverTeamId = INDEX_NONE);

	/**
	 * Apre su tracce **gia' in memoria**, senza archivio da leggere.
	 *
	 * 🔑 E' il secondo ingresso previsto da `#1625`: lo Scenario Playback riproduce l'ultima esecuzione di
	 * uno scenario, che vive nel draft d'authoring e **non e' mai stata scritta su disco**. Senza questa porta
	 * l'unico modo di riusare la navigazione sarebbe riscriverla nell'editor — cioe' avere due
	 * implementazioni di *«fase successiva»* che possono divergere, che e' precisamente cio' che il secondo
	 * consumatore doveva evitare.
	 *
	 * ⚠️ **Nessun manifest, e non e' una dimenticanza.** Un'esecuzione di scenario non ha `MatchId`, ne'
	 * topologia dichiarata, ne' osservatore: il manifest resta al suo default e chi apre di qui non deve
	 * leggerlo. Cio' che la navigazione usa davvero — `Traces` e la cache delle fasi — e' tutto qui.
	 *
	 * ⛔ **Tracce vuote sono un RIFIUTO**, non un'apertura su una partita in cui non e' successo niente. A
	 * schermo le due si vedono uguali — un campo fermo — e sono affermazioni diverse.
	 *
	 * `bComplete` resta `true`: chi passa tracce in memoria ha per costruzione l'esecuzione intera, e
	 * l'incompletezza che `Open` sa riconoscere e' una proprieta' dell'**archivio**, non di questa via.
	 */
	bool OpenFromTraces(TArray<TArray<FRTTurnLogEntry>> Traces);

	/** `true` solo dopo un `Open` riuscito. */
	bool IsOpen() const { return bOpen; }

	/**
	 * `true` dopo che un `Open` e' stato **tentato**, comunque sia andato.
	 *
	 * 🔴 Esiste perche' `LastOpenResult()` da solo mente su un view model appena costruito: il suo valore
	 * iniziale e' `ManifestUnreadable`, quindi «non ho mai provato ad aprire niente» e «il manifest e'
	 * illeggibile» erano indistinguibili. Trovato in code review. L'enum non si estende — e' di
	 * `URTReplayPlayerLibrary` e i suoi quattro esiti descrivono **aperture**, non l'assenza di una.
	 */
	bool HasAttemptedOpen() const { return bOpenAttempted; }

	/** L'esito dell'ultimo `Open`. La UI ne distingue quattro: e' un criterio di `#472`. Da leggere con
	 *  `HasAttemptedOpen()`, che dice se la domanda si applica. */
	ERTReplayOpenResult LastOpenResult() const { return OpenResult; }

	/**
	 * `true` = archivio aperto **e** completo. `false` copre due casi che la UI deve distinguere con
	 * `IsOpen()`: archivio **parziale**, oppure nessun archivio.
	 *
	 * 🔴 La prima stesura leggeva `Session.bComplete` senza guardare `bOpen`, quindi un view model mai
	 * aperto — o il cui `Open` era fallito — rispondeva `false`, che una schermata avrebbe rese come
	 * «archivio parziale»: una partita guardabile ma troncata al posto di un manifest mancante. Trovato
	 * in code review, ed e' lo stesso difetto del `0.f` che direbbe «scaduto adesso».
	 *
	 * Un archivio parziale **non e' un archivio rotto**: e' la distinzione che la lista di `#416` gia' fa
	 * — un manifest non chiuso e' una partita che il gioco non ha visto finire.
	 */
	bool IsComplete() const { return bOpen && Session.bComplete; }

	/** L'header della partita aperta: esito, durata, conteggio turni. Vuoto se non c'e' un archivio. */
	const FRTReplayManifest& GetManifest() const { return Session.Manifest; }

	/** Dove siamo. Vale in tutti e quattro gli stati — e' il punto della struttura. */
	FRTReplayPosition Position() const { return Current; }

	/**
	 * Le voci della fase che si sta guardando: **cio' che il viewer disegna**.
	 *
	 * 🔴 Mancava, e senza di essa il view model diceva *dove* si e' senza dare niente da mostrare —
	 * `#472` chiede di guardare una partita, non solo di sapere a che punto e'. Trovato in code review.
	 *
	 * Vuoto negli stati senza fase (`BeforeStart`, `Ended`), che e' la risposta giusta: non c'e' una fase
	 * corrente di cui elencare le voci.
	 */
	TArray<FRTTurnLogEntry> CurrentPhaseEntries() const;

	/**
	 * Le fasi che il turno corrente contiene davvero, in ordine cronologico. Vuoto fuori dalla sequenza.
	 *
	 * Serve alla barra dei salti di fase, e la sua assenza costringeva il widget a enumerare
	 * `ERTMatchPhase` sondando `SeekToPhaseInCurrentTurn` — cioe' proprio l'errore che
	 * `ObservablePhases()` esiste per rendere non necessario.
	 */
	TArray<ERTMatchPhase> PhasesInCurrentTurn() const;

	/** Torna prima dell'inizio senza rileggere il disco. Ferma anche la riproduzione. */
	void Rewind();

	// --- Movimento -------------------------------------------------------------------------------
	//
	// I quattro `Can*` non mutano nulla e rispondono alla domanda che disegna il pulsante: e' il
	// criterio dei bordi di `#472` — «disabilitato, non silenzioso». Un comando che non fa nulla senza
	// dirlo e' indistinguibile da uno rotto.

	bool CanStepPhaseForward() const;
	bool CanStepPhaseBackward() const;
	bool CanStepTurnForward() const;
	bool CanStepTurnBackward() const;

	/** Avanza di una fase, attraversando il confine di turno. `false` = non c'era dove andare. */
	bool StepPhaseForward();

	/**
	 * Indietro di una fase, attraversando il confine di turno **all'indietro**.
	 *
	 * Dalla prima fase del turno N si torna all'**ultima fase emessa** del turno N-1, non alla sua
	 * `Cleanup`: una fase senza voci non e' una posizione (`SeekToPhase` risponde `PhaseNotFound`), e
	 * fermarcisi mostrerebbe una schermata vuota per un turno in cui non e' successo niente.
	 */
	bool StepPhaseBackward();

	/**
	 * Al primo turno **successivo**, prima fase.
	 *
	 * ⚠️ **Non e' `SeekToTurn(N+1)`**, e la differenza non e' teorica: le tracce dichiarano il proprio
	 * `TurnNumber` e una sequenza puo' iniziare da un turno qualsiasi (`RTReplaySeekLibrary.h`). Il turno
	 * successivo e' quello della **traccia adiacente**, che va letto. Con `+1` un archivio con un buco di
	 * numerazione bloccherebbe il pulsante su una partita perfettamente riproducibile.
	 */
	bool StepTurnForward();

	/** Al primo turno **precedente**, prima fase. Stessa ragione: e' la traccia adiacente, non `N-1`. */
	bool StepTurnBackward();

	/**
	 * Salta al turno DICHIARATO, prima fase, riusando `URTReplaySeekLibrary::SeekToTurn`.
	 *
	 * `TurnNotFound` lascia la posizione **invariata**: e' la convenzione fail-closed del replay, e qui
	 * conta due volte — un salto fallito che spostasse comunque il cursore darebbe alla UI una posizione
	 * che nessuno ha chiesto.
	 */
	ERTReplaySeekResult SeekToTurn(int32 TurnNumber);

	/** Salta a una fase dentro il turno corrente. `PhaseNotFound` se quella fase non ha prodotto voci. */
	ERTReplaySeekResult SeekToPhaseInCurrentTurn(ERTMatchPhase Phase);

	// --- Riproduzione ----------------------------------------------------------------------------
	//
	// ⚠️ Questo e' lo stato che `#472` dichiarava di non dover creare. `URTPlaybackLibrary` non lo porta:
	// e' matematica pura — interpolazione e stima di durate — senza clock e senza `bPlaying`. Il tempo
	// entra qui da un punto solo, e decide *quando* avanzare, mai *cosa* mostrare.

	void Play();
	void Pause();
	bool IsPlaying() const { return bPlaying; }

	/** Secondi che una fase resta a schermo in riproduzione automatica. Presentazione, non regola. */
	float SecondsPerPhase = 1.5f;

	/**
	 * Fa scorrere il tempo. Ritorna `true` se la posizione e' cambiata.
	 *
	 * A fine sequenza **si mette in pausa da solo**: restare «in play» su una sequenza finita darebbe un
	 * pulsante di pausa che non pausa niente.
	 *
	 * ⚠️ Un `DeltaSeconds` grande avanza di **una** fase per chiamata e non di quante ne entrerebbero:
	 * saltare fasi perche' il frame e' stato lungo significherebbe che cosa si vede dipende dal frame
	 * rate, ed e' l'opposto del motivo per cui questo tipo esiste.
	 */
	bool Tick(float DeltaSeconds);

private:
	/** Le tracce e il manifest. Il **cursore** che porta con se' non e' la posizione: vedi `FRTReplayPosition`. */
	FRTReplaySession Session;

	FRTReplayPosition Current;

	/** Traccia della posizione corrente. `INDEX_NONE` con `BeforeStart` e con `Ended`. */
	int32 CurrentTraceIndex = INDEX_NONE;

	bool bOpen = false;
	bool bOpenAttempted = false;
	bool bPlaying = false;
	float PhaseElapsed = 0.f;
	ERTReplayOpenResult OpenResult = ERTReplayOpenResult::ManifestUnreadable;

	/**
	 * Le fasi presenti in ciascuna traccia, **calcolate una volta sola** in `Open`.
	 *
	 * Le tracce sono immutabili dopo l'apertura, quindi ricalcolarle a ogni domanda era lavoro buttato: la
	 * prima stesura lo faceva a ogni `Can*`/`Step*` — cinque scansioni della traccia e un'allocazione per
	 * chiamata — e una UI che polla i quattro predicati per abilitare i pulsanti lo avrebbe pagato **per
	 * frame**. Trovato in code review.
	 */
	TArray<TArray<ERTMatchPhase>> PhasesPerTrace;

	/**
	 * Le fasi che la traccia contiene DAVVERO, in ordine cronologico. Legge la cache.
	 *
	 * La cache e' costruita in `Open` interrogando `URTReplaySeekLibrary::SeekToPhase` sulle cinque
	 * osservabili: e' il riuso del seek che `#472` richiede, non un secondo meccanismo che «guarda le
	 * voci». Una traccia che non ne contiene nessuna — un turno senza voci — da' un array vuoto, e chi
	 * naviga la salta.
	 */
	const TArray<ERTMatchPhase>& PhasesInTrace(int32 TraceIndex) const;

	/** Popola `PhasesPerTrace`. Chiamata solo da `Open`, quando le tracce smettono di cambiare. */
	void BuildPhaseCache();

	/** Indice della prossima traccia NON vuota in direzione `Direction` (+1 / -1), o `INDEX_NONE`. */
	int32 AdjacentNonEmptyTrace(int32 From, int32 Direction) const;

	/** Costruisce la posizione da (traccia, fase), decidendo fra `AtPhase` e `Unaddressable`. */
	FRTReplayPosition MakePosition(int32 TraceIndex, ERTMatchPhase Phase) const;

	/**
	 * La posizione a una fase di distanza, senza applicarla. `false` = non c'era dove andare.
	 *
	 * Esiste perche' `CanStepPhase*` e `StepPhase*` rispondano alla **stessa** domanda: un `Can` che
	 * ragionasse per conto suo potrebbe abilitare un pulsante che poi non fa nulla, ed e' esattamente il
	 * difetto che il criterio dei bordi vieta. I due estremi sono posizioni valide — avanti dall'ultima
	 * fase si arriva a `Ended`, indietro dalla prima a `BeforeStart` — e i bordi **chiusi** sono quelli
	 * oltre di essi.
	 */
	bool NextPhasePosition(int32 Direction, FRTReplayPosition& OutPos, int32& OutTraceIndex) const;

	/**
	 * Come sopra, per il salto di turno. ⚠️ **Non produce mai `Ended` ne' `BeforeStart`**: «turno
	 * successivo» dall'ultimo turno non e' «la fine», e' una richiesta senza risposta — il pulsante si
	 * disabilita invece di consumare il comando per portare altrove.
	 */
	bool NextTurnPosition(int32 Direction, FRTReplayPosition& OutPos, int32& OutTraceIndex) const;

	/** Applica una posizione calcolata e azzera il conteggio della riproduzione. */
	void ApplyPosition(const FRTReplayPosition& NewPos, int32 NewTraceIndex);
};
