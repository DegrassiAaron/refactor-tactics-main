#pragma once

#include "CoreMinimal.h"
#include "RTPacing.generated.h"

/** Chi ha chiuso la pianificazione: il giocatore o il timer. */
UENUM(BlueprintType)
enum class ERTLockInSource : uint8
{
	Input,      // il giocatore ha premuto il lock-in
	Timeout     // l'ha chiusa lo scadere del timer
};

/** Tipo di input di pianificazione, per distinguere "sto pensando" da "sto litigando con l'interfaccia". */
UENUM(BlueprintType)
enum class ERTPlanningInput : uint8
{
	Click,      // attivita' generica: aggiorna solo i tempi, non incrementa nulla
	Selection,  // il giocatore ha selezionato un'unita'
	Order,      // il giocatore ha impartito un ordine (abilita' o destinazione)
	Undo        // waypoint annullato
};

/**
 * Un turno misurato. TELEMETRIA: non entra nel TurnLog, non entra nel suo hash, non influenza nessuna
 * decisione di gioco (docs/gameplay/spec-pacing-turno.md §4). E' l'unica ragione per cui puo' permettersi di
 * non essere deterministico.
 *
 * Tutti interi in millisecondi: un `float` in CSV con locale italiano stampa la virgola decimale, che
 * collide col separatore, e renderebbe i test a tolleranza invece che esatti.
 */
USTRUCT(BlueprintType)
struct FRTPacingSample
{
	GENERATED_BODY()

	/**
	 * Un tempo che NON e' stato misurato, e che quindi non e' un tempo.
	 *
	 * Serve perche' `0` e' un valore legittimo — un lock-in istantaneo — e usarlo per dire «non misurato»
	 * produce un dato **plausibile e falso**, che nessun errore segnala: la mediana scende, e un timeout con
	 * `MsSinceLastInput = 0` viene classificato come taglio del timer, cioe' come il segnale che questa
	 * metrica esiste per catturare. Un tempo negativo, invece, non si confonde con nessun tempo reale.
	 *
	 * ⚠️ Chi aggrega deve ESCLUDERLI: `URTPacingLibrary::SummarizeSamples` lo fa e li conta a parte in
	 * `FRTPacingSummary::UnmeasuredSamples`. Nel CSV la colonna porta `-1`.
	 *
	 * Il caso che lo rende necessario (`#1421`): un `LockInAndResolve()` raggiunto senza passare da
	 * `StartPlanningTimer()` — ogni test headless, lo Scenario Harness, e un `OnPlanningTimeout` armato da
	 * `SetPlanningSeconds()`. Li' il campione si apre comunque, perche' il CONTESTO (unita' vive, azioni,
	 * numero di turno) e' misurabile e va misurato: sono solo i TEMPI a non avere un'origine.
	 */
	static constexpr int32 Unmeasured = INDEX_NONE;

	/** Numero del turno misurato. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 TurnNumber = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 UnitsAliveTeam0 = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 UnitsAliveTeam1 = 0;

	/** Azioni utilizzabili dalle unita' vive della squadra misurata (cooldown/energia escludono). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 ActionsAvailable = 0;

	/**
	 * Millisecondi dall'inizio della pianificazione al primo input; = MsToLockIn se non c'e' stato input,
	 * `Unmeasured` se il campione non e' mai stato aperto.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 MsToFirstInput = 0;

	/** Quante volte il giocatore ha selezionato un'unita' in questo turno. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 SelectionCount = 0;

	/** Quanti ordini (abilita' o destinazione) ha impartito in questo turno. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 OrderCount = 0;

	/** Quanti waypoint ha annullato in questo turno. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 UndoCount = 0;

	/** Millisecondi dall'apertura della pianificazione al lock-in; `Unmeasured` se il campione non e' stato aperto. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 MsToLockIn = 0;

	/**
	 * Millisecondi dall'ultimo input al lock-in; = MsToLockIn se non c'e' stato NESSUN input, `Unmeasured`
	 * se il campione non e' mai stato aperto.
	 * E' il campo che distingue un timer che TAGLIA (valore basso) da un timer che scade A VUOTO (alto):
	 * due patologie con cure opposte, indistinguibili contando solo i timeout.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 MsSinceLastInput = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	ERTLockInSource LockInSource = ERTLockInSource::Input;

	/** Durata effettiva del playback della risoluzione (0 se non c'e' stato playback). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 MsPlayback = 0;

	/**
	 * Quante finestre di reazione si sono APERTE in questo turno alla squadra misurata (CP 14.6, `#166`).
	 *
	 * E' il moltiplicatore della componente di decisione, che ADR-0004 tratta come aritmetica:
	 * `finestre × FastReactionDuration` e' il tempo che un giocatore puo' vedersi occupare, e la sola meta'
	 * di quel prodotto che un test headless puo' misurare. L'altra — quanto ci mette **davvero** a
	 * rispondere — richiede decisori veri, e non si simula.
	 *
	 * 🔴 **Si conta per SQUADRA e non per partita**, ed e' la distinzione che [D-167] rende vincolante: due
	 * unita' armate su squadre **diverse** aprono due finestre che due persone aspettano **in parallelo**,
	 * due dello **stesso** giocatore gliene impilano due **in fila**. Sommarle darebbe la baseline di un
	 * gioco che non giochiamo.
	 *
	 * Derivato dal TurnLog e non contato a parte: una finestra che si apre e' gia' un fatto registrato
	 * (`ERTLogCategory::ReactionDecision`), e un secondo contatore nel resolver sarebbe una seconda verita'
	 * che diverge al primo esito nuovo.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 ReactionWindowsOpened = 0;

	/** Vero se il giocatore ha saltato il playback. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	bool bPlaybackSkipped = false;
};

/** Sommario di una sessione di campioni. Prodotto da URTPacingLibrary::SummarizeSamples. */
USTRUCT(BlueprintType)
struct FRTPacingSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 SampleCount = 0;

	/**
	 * Quanti dei `SampleCount` non portavano un tempo (`FRTPacingSample::Unmeasured`).
	 *
	 * Sta qui e non e' un dettaglio: mediana, p90 e la classificazione dei timeout sono calcolate sui soli
	 * campioni misurati, quindi senza questo numero una mediana su 3 campioni di 100 si legge come la
	 * mediana di 100.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 UnmeasuredSamples = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 MedianMsToLockIn = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 P90MsToLockIn = 0;

	/** Timeout con input recente: il timer ha tagliato una decisione in corso. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 TrueCutoffs = 0;

	/** Timeout senza input recente: il timer e' scaduto a vuoto. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 IdleTimeouts = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 SkippedPlaybacks = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 MedianMsPlayback = 0;
};
