#pragma once

#include "CoreMinimal.h"
#include "Turn/RTCombatLog.h"     // FRTLogSubject: il soggetto di una riga del log, passato per valore
#include "Turn/RTResolvedEvent.h" // FRTResolvedEvent: la timeline che il playback riproduce

class ARTUnit;
class URTHexMapAsset;
class UWorld;

/**
 * Le manopole del playback, lette dall'orchestratore **a ogni uso** (#4057).
 *
 * 🔑 **Restano `UPROPERTY` su `ARTTurnManager`, e il director non ne tiene una copia.** Blueprint e due test le leggono
 * dal CDO e le scrivono a runtime; chiederle a ogni uso e' cio' che lascia vero, come prima dello spostamento, che un
 * valore scritto a playback in corso vale dal tick dopo (`ViewerPlaybackSpeed`). Una copia fatta in `BeginPlayback`
 * lo renderebbe falso senza che nessun test lo dica.
 *
 * Il significato di ciascun campo sta sulla sua `UPROPERTY` in `RTTurnManager.h`: qui non si ripete.
 */
struct FRTPlaybackTuning
{
	float PlaybackCellsPerSecond = 0.f;
	float StepTurnFraction = 0.f;
	float FinalPivotSeconds = 0.f;
	float PhaseBeatSeconds = 0.f;
	float AttackShowSeconds = 0.f;
	float TracerFlightSeconds = 0.f;
	float ActivationCueSeconds = 0.f;
	float ImpactCueSeconds = 0.f;
	float DefeatBeatSeconds = 0.f;
	float MaxPlaybackSeconds = 0.f;
	float ViewerPlaybackSpeed = 0.f;
	bool bRecordAttackBeatsForTest = false;
	bool bRevealActivationsOnlyAtPhaseEndForTest = false;
};

/**
 * Cio' che il playback chiede all'orchestratore, e nient'altro (#4057).
 *
 * 🔑 **Il director non conosce `ARTTurnManager`**: parla con questa interfaccia, che il TurnManager implementa. E' il
 * criterio del piano di struttura del 2026-11-05 per ogni carico che esce dalla classe: il file nuovo non nomina i
 * membri dell'orchestratore, quindi cio' di cui ha bisogno sta scritto qui, in un posto solo.
 *
 * ⛔ **Nessun metodo qui decide un esito.** Il playback LEGGE la timeline e lo stato finale; l'unico atto di flusso che
 * compie e' chiudere il turno, e lo chiede (`ConcludeTurn`) invece di farlo.
 *
 * ⚠️ **Prefisso `F` e non `I`**: e' una classe astratta C++, non una `UINTERFACE`, come `FExec` per `UGameInstance`.
 * Con `I`, UHT la cercherebbe fra le interfacce riflesse.
 */
class FRTPlaybackHost
{
public:
	virtual ~FRTPlaybackHost() = default;

	/** Il mondo del playback: le unita', l'attore della mappa, il controller di chi guarda. */
	virtual UWorld* GetPlaybackWorld() const = 0;

	/** Vedi `FRTPlaybackTuning`: si chiede a ogni uso. */
	virtual FRTPlaybackTuning GetPlaybackTuning() const = 0;

	/** La timeline del turno, in sola lettura: il playback la riproduce, non la scrive. */
	virtual const TArray<FRTResolvedEvent>& GetPlaybackTimeline() const = 0;

	virtual bool IsPrepActiveThisTurn() const = 0;

	/** Il turno delle righe del Registro: `ConcludeTurn` lo avanza DOPO il playback. */
	virtual int32 GetPlaybackTurnNumber() const = 0;

	virtual ARTUnit* UnitByStableId(int32 StableUnitId) const = 0;
	virtual const URTHexMapAsset* GetHexContext(FVector& OutOrigin, float& OutHexSize, float& OutLayerHeight) const = 0;
	virtual void AddLogEvent(const FString& Message, FRTLogSubject Subject) = 0;

	/**
	 * `IsResolving()` dell'orchestratore. Lo SCRIVE il playback — vero all'avvio, falso alla fine — ma lo leggono
	 * anche la pianificazione, l'HUD e la finestra di reazione: il significato resta dell'orchestratore.
	 */
	virtual bool IsPlaybackResolving() const = 0;
	virtual void SetPlaybackResolving(bool bResolving) = 0;

	/** Il tick dell'attore, che non serve solo al playback: lo usano anche la finestra di reazione e lo slow motion. */
	virtual void SetPlaybackTickEnabled(bool bEnabled) = 0;

	virtual bool IsPlaybackResolutionSuspended() const = 0;
	virtual void PresentPendingReactionWindow() = 0;

	/** Chiude il turno: e' l'unico atto di flusso del playback, e resta dell'orchestratore. */
	virtual void ConcludeTurn() = 0;

	/** La conoscenza ANIMATA: resta nell'orchestratore, perche' dipende da `MakeCurrentSnapshot`. */
	virtual bool AdvancePlaybackKnowledge() = 0;

	/** Telemetria: il campione di pacing del turno annota che il playback e' stato saltato. */
	virtual void MarkPlaybackSkipped() = 0;
};
