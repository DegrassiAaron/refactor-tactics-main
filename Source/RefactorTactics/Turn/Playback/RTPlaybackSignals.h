#pragma once

#include "CoreMinimal.h"
#include "Turn/RTTurnRules.h" // ERTMatchPhase: la fase che il playback annuncia
#include "RTPlaybackSignals.generated.h"

class ARTUnit;

/*
 * I delegate del playback di presentazione (#4057).
 *
 * 🔑 **I TIPI vivono qui, le ISTANZE restano su `ARTTurnManager`.** Sono `UPROPERTY(BlueprintAssignable)` e un
 * contratto documentato (`capability-map.md`, il seam VFX/SFX/camera): chi si lega continua a legarsi al TurnManager.
 * Il director li trasmette attraverso `FRTPlaybackSignals`, senza conoscere la classe che li possiede.
 *
 * ⚠️ I nomi dei tipi non sono cambiati con lo spostamento, ed e' cio' che rende lo spostamento innocuo: un Blueprint
 * risolve la firma di un delegate per nome nel package del modulo, non per header.
 */

// Delegate per la presentazione in Blueprint (camera/VFX/SFX): il playback riproduce eventi gia' risolti.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTPhasePlaybackSignature, ERTMatchPhase, Phase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTUnitPlaybackSignature, ARTUnit*, Unit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRTAttackPlaybackSignature, ARTUnit*, Source, ARTUnit*, Target, int32, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRTPlaybackFinishedSignature);

/**
 * Il playback ha attraversato un confine di micro-step (`#2876`): le pose ANIMATE sono cambiate.
 *
 * ⛔ **Non porta la conoscenza nel payload**, come `FRTTeamKnowledgeRefreshedSignature` e per la stessa
 * ragione registrata in [D-227]: sceglierebbe la squadra per conto di tutti i subscriber. Chi ascolta
 * chiede `PlaybackKnowledgeForTeam` per la squadra che gli compete.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRTPlaybackStepSignature);


/**
 * I delegate del TurnManager che il director trasmette (#4057).
 *
 * ⛔ **Puntatori, non copie.** Un delegate copiato trasmetterebbe a una lista di ascoltatori che nessuno ha
 * registrato: l'HUD, il velo e il PlayerController si legano alle istanze del TurnManager, ed e' su quelle che il
 * playback deve suonare.
 */
struct FRTPlaybackSignals
{
	FRTPhasePlaybackSignature* OnPhasePlaybackStarted = nullptr;
	FRTUnitPlaybackSignature* OnUnitMoveStarted = nullptr;
	FRTUnitPlaybackSignature* OnUnitDefeated = nullptr;
	FRTAttackPlaybackSignature* OnAttackResolved = nullptr;
	FRTPlaybackStepSignature* OnPlaybackStepAdvanced = nullptr;
	FRTPlaybackFinishedSignature* OnResolvePlaybackFinished = nullptr;
};
