#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/RTTypes.h"
#include "Turn/RTTurnRules.h"
#include "Turn/RTResolvedEvent.h"
#include "Turn/RTTurnLog.h"
#include "RTTurnManager.generated.h"

class ARTUnit;

// Delegate per la presentazione in Blueprint (camera/VFX/SFX): il playback riproduce eventi gia' risolti.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTPhasePlaybackSignature, ERTMatchPhase, Phase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTUnitPlaybackSignature, ARTUnit*, Unit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRTAttackPlaybackSignature, ARTUnit*, Source, ARTUnit*, Target, int32, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRTPlaybackFinishedSignature);

/**
 * Orchestratore del turno: tiene fase e numero di turno e, al lock-in, risolve il turno (logica sincrona,
 * autoritativa) e poi ne RIPRODUCE nel tempo la risoluzione (playback) per rendere il round osservabile.
 * L'animazione legge eventi gia' risolti: non decide nulla (invariante #1).
 */
UCLASS()
class REFACTORTACTICS_API ARTTurnManager : public AActor
{
	GENERATED_BODY()

public:
	ARTTurnManager();

	virtual void Tick(float DeltaSeconds) override;

	/** Chiude la pianificazione e risolve il turno; il movimento si applica nella fase Move. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Turn")
	void LockInAndResolve();

	/** Hook per i test d'integrazione headless: invoca la pianificazione dei bot senza timer/playback. */
	void PlanBotsForTest() { PlanBots(); }

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	ERTMatchPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	int32 GetTurnNumber() const { return TurnNumber; }

	/** Secondi rimanenti alla pianificazione (0 se scaduto/assente). Utile per una futura HUD. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	float GetPlanningTimeRemaining() const;

	/** Vero mentre e' in corso il playback della risoluzione (pianificazione bloccata). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	bool IsResolving() const { return bIsResolving; }

	/** Nome leggibile della fase in riproduzione (per la HUD). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	FString GetPlaybackPhaseName() const;

	/** Avanzamento del playback in [0,1] (0 se non in risoluzione). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	float GetPlaybackProgress01() const;

	/** Salta il resto del playback e conclude subito il turno (snap alle posizioni finali). */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Turn")
	void SkipPlayback();

	/** Ultimi eventi (combat log) per la HUD, dal piu' vecchio al piu' recente. */
	const TArray<FString>& GetRecentEvents() const { return RecentEvents; }

	/** Esiti autoritativi dell'ultimo turno risolto (Movimento + Combat), ordinati deterministicamente. */
	const TArray<FRTTurnLogEntry>& GetTurnLog() const { return TurnLog; }

	/** Rotte effettivamente percorse nell'ultima risoluzione (viz post-lock del percorso eseguito). */
	const TArray<TArray<FRTGridCoord>>& GetLastMoveRoutes() const { return LastMoveRoutes; }

	// --- Presentazione (Blueprint) -------------------------------------------------------------
	UPROPERTY(BlueprintAssignable, Category = "RefactorTactics|Playback")
	FRTPhasePlaybackSignature OnPhasePlaybackStarted;

	UPROPERTY(BlueprintAssignable, Category = "RefactorTactics|Playback")
	FRTUnitPlaybackSignature OnUnitMoveStarted;

	/** Un'unita' viene eliminata VISIVAMENTE nel playback (per VFX/SFX di morte in Blueprint). */
	UPROPERTY(BlueprintAssignable, Category = "RefactorTactics|Playback")
	FRTUnitPlaybackSignature OnUnitDefeated;

	UPROPERTY(BlueprintAssignable, Category = "RefactorTactics|Playback")
	FRTAttackPlaybackSignature OnAttackResolved;

	UPROPERTY(BlueprintAssignable, Category = "RefactorTactics|Playback")
	FRTPlaybackFinishedSignature OnResolvePlaybackFinished;

	// --- Tuning del pacing (editabile in editor, tuning live senza ricompilare) -----------------
	/** Se falso, la risoluzione resta istantanea (nessun playback). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	bool bEnablePlayback = true;

	/** Velocita' di scorrimento dei cilindri nel Move (celle al secondo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	float PlaybackCellsPerSecond = 6.5f;

	/** Pausa tra una fase e la successiva (secondi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	float PhaseBeatSeconds = 0.30f;

	/** Durata di visualizzazione di ogni colpo nel Blast (secondi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	float AttackShowSeconds = 0.50f;

	/** Tetto di durata del playback: oltre, si accelera automaticamente (0 = nessun tetto). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	float MaxPlaybackSeconds = 12.f;

	// --- Tuning del bot (utility scoring, editabile in editor senza ricompilare) -----------------
	// Pesi interi iniettati nel FRTBotContext di PlanBots (invariante #4: niente float). I default
	// coincidono con quelli della struct: a parita' di valori il comportamento e' invariato.
	/** Bonus se l'attacco pianificato UCCIDE il bersaglio: domina la scelta (focus-fire letale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WKill = 10000;

	/** Peso del danno inflitto dall'attacco (focus-fire: a parita' d'altro, piu' danno = meglio). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WDamage = 10;

	/** Penalita' per ogni nemico che puo' colpire la cella scelta (evita di esporsi al tiro). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WThreat = 100;

	/** Penalita' (kiter) proporzionale a quanto si sta SOTTO la distanza di sicurezza (standoff). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WKiteViolation = 50;

	/** Penalita' (mischia) proporzionale alla distanza dal nemico: chiudere la distanza e' meglio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WApproach = 10;

	/** Bonus per la quota (Layer) della cella: premia l'alta quota (tiro oltre coperture basse, +danno). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Bot")
	int32 WElevation = 20;

protected:
	virtual void BeginPlay() override;

	void PlanBots();
	void ResolvePrep();
	void ResolveDash();
	void ResolveCombat();
	void ResolveMovement();
	void StartPlanningTimer();
	void OnPlanningTimeout();

	/** Termina il turno: se la partita e' decisa la chiude, altrimenti riapre la pianificazione. */
	void ConcludeTurn();

	/** Distrugge definitivamente le unita' morte (morte visiva differita): chiamato a fine turno. */
	void DestroyDefeatedUnits();

	/** Avvia il playback della risoluzione (movimento in parallelo, fasi a beat). */
	void BeginPlayback();
	void EnterPlaybackPhase();
	void TickPlayback(float DeltaSeconds);
	void FinishPlayback();
	float DurationForPlaybackPhase(ERTMatchPhase InPhase) const;

	/** Registra un evento: lo scrive nel log LogRT e lo accoda al combat log della HUD. */
	void AddLogEvent(const FString& Message);

	UPROPERTY()
	TArray<FString> RecentEvents;

	/** TurnLog dell'ultimo turno risolto (osservabilita' autoritativa; ordinato in LockInAndResolve). */
	TArray<FRTTurnLogEntry> TurnLog;

	/** Rotte (celle) percorse da ogni unita' che si e' mossa nell'ultima risoluzione. */
	TArray<TArray<FRTGridCoord>> LastMoveRoutes;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Turn")
	int32 MaxLogLines = 6;

	/** Durata della fase di pianificazione; allo scadere scatta il lock-in automatico. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Turn")
	float PlanningSeconds = 30.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Turn")
	ERTMatchPhase Phase = ERTMatchPhase::Planning;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Turn")
	int32 TurnNumber = 1;

	FTimerHandle PlanningTimerHandle;

private:
	/** Animazione di movimento di una singola unita': waypoint gia' convertiti in mondo + fase (Dash/Move). */
	struct FRTMoveAnim
	{
		TWeakObjectPtr<ARTUnit> Unit;
		TArray<FVector> World; // start + celle attraversate, in coordinate mondo
		ERTMatchPhase Phase = ERTMatchPhase::Move; // fase in cui va riprodotta (Dash o Move)
	};

	/** Eventi risolti nel turno corrente (movimenti, attacchi) da riprodurre. */
	TArray<FRTResolvedEvent> ResolvedTimeline;

	// Stato runtime del playback.
	bool bIsResolving = false;
	bool bPrepActiveThisTurn = false;
	ERTMatchOutcome PendingOutcome = ERTMatchOutcome::InProgress;

	TArray<FRTMoveAnim> MoveAnims;          // derivati dagli eventi Move
	TArray<FRTResolvedEvent> PlaybackAttacks; // eventi Attack, mostrati in serie nel Blast
	TArray<FRTResolvedEvent> PlaybackDefeated; // eventi Defeated, mostrati a fine della loro fase
	TArray<ERTMatchPhase> PlaybackPhases;   // fasi attive, in ordine
	int32 PlaybackPhaseIdx = 0;
	float PlaybackPhaseElapsed = 0.f;
	float PlaybackSpeed = 1.f;              // fattore di accelerazione per rientrare nel tetto
	float PlaybackTotalSeconds = 0.f;       // durata stimata (per la progress bar)
	float PlaybackElapsedTotal = 0.f;
	int32 AttacksShown = 0;                 // colpi gia' rivelati nel Blast corrente

	// Trasformazione griglia in cache per convertire celle->mondo durante il playback.
	FVector PBOrigin = FVector::ZeroVector;
	float PBCellSize = 200.f;
	float PBLayerHeight = 0.f;
};
