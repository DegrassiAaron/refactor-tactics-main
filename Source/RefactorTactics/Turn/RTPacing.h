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

	/** Millisecondi dall'inizio della pianificazione al primo input; = MsToLockIn se non c'e' stato input. */
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

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 MsToLockIn = 0;

	/**
	 * Millisecondi dall'ultimo input al lock-in; = MsToLockIn se non c'e' stato NESSUN input.
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
