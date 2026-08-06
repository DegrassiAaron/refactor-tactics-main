#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/RTTypes.h"
#include "Turn/RTTurnLog.h"
#include "RTCombatLibrary.generated.h"

class URTHexMapAsset;

/** Esito dell'applicazione del danno: HP e scudo risultanti. */
USTRUCT(BlueprintType)
struct FRTDamageResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Health = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Shield = 0;

	FRTDamageResult() = default;
	FRTDamageResult(int32 InHealth, int32 InShield) : Health(InHealth), Shield(InShield) {}
};

/** Calcoli puri di combattimento (indipendenti dagli Actor, testabili). */
UCLASS()
class REFACTORTACTICS_API URTCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Applica il danno: lo scudo assorbe per primo, poi gli HP. Nessun valore scende sotto 0. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static FRTDamageResult ApplyDamage(int32 Damage, int32 Shield, int32 Health);

	/** Accumula energia con clamp in [0, Max]. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static int32 GainEnergy(int32 Current, int32 Gain, int32 Max);

	/** Vero se l'ultimate e' disponibile (energia al massimo). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static bool IsUltimateReady(int32 Energy, int32 Max);

	/** Range di movimento effettivo con gli status: Root azzera, Slow dimezza (arrotondato per difetto). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static int32 EffectiveMoveRange(int32 BaseRange, bool bRooted, bool bSlowed);

	/** Vero se un'abilita' e' utilizzabile: non in ricarica e con energia sufficiente. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static bool IsAbilityUsable(int32 CooldownRemaining, int32 Energy, int32 EnergyCost);

	/**
	 * Invariante #6 (privacy dell'intento): il piano di un'unita' e' visibile agli alleati
	 * (stessa squadra) sempre, agli avversari solo se l'unita' e' "rivelata" (status Reveal).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static bool IsIntentVisibleTo(int32 ObserverTeamId, int32 OwnerTeamId, bool bOwnerRevealed);

	/**
	 * AUTORITA' sull'unita': un giocatore comanda solo le unita' della PROPRIA squadra — quindi puo' selezionarle
	 * e pianificarne le azioni. Regola esplicita e testabile invece che implicita nel controller: senza, un click
	 * su un'unita' avversaria la rende "selezionata" e da li' si finisce a pianificare i turni del nemico
	 * (oltre a bloccare la selezione delle proprie, vedi OnSelect). Parente dell'invariante #6: quel che non e'
	 * tuo non lo vedi e non lo comandi.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static bool CanPlayerControlUnit(int32 UnitTeamId, int32 PlayerTeamId);

	/**
	 * Vero se il bersaglio e' ingaggiabile su griglia esagonale: entro `RangeCells` (distanza esagonale) e con
	 * linea di tiro libera sulla mappa.
	 *
	 * **FAIL-CLOSED**: `Map == nullptr` -> falso. Senza mappa autorevole non si valida la linea di tiro, quindi
	 * non si pianifica l'attacco. La versione precedente nel controller faceva l'opposto (`!Grid || HasLOS`) e
	 * lasciava passare ogni bersaglio quando la griglia non c'era.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static bool CanTargetHexCell(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To,
		int32 RangeCells);

	/**
	 * Danno effettivo di un attacco dato il bonus della cella occupata dall'attaccante
	 * (es. Altura +danno). Risultato con clamp >= 0.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static int32 EffectiveAttackPower(int32 BasePower, int32 OccupantDamageBonus);

	/**
	 * Indici delle unita' "appena eliminate": vive (HP > 0) nello stato PRIMA e morte (HP <= 0) nello
	 * stato DOPO. Serve al playback per sapere CHI muore ora (e generare l'evento Defeated) senza
	 * ri-notificare chi era gia' morto. Confronto per indice, fino al minimo delle due lunghezze.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static TArray<int32> NewlyDefeated(const TArray<int32>& HealthBefore, const TArray<int32>& HealthAfter);

	/**
	 * Classifica l'esito di un colpo a segno secondo la priorita' Lethal > ShieldAbsorbed > TerrainBonus > Hit.
	 * ShieldAbsorbed = HP invariati (assorbito dallo scudo). TerrainBonus = HP calati con bonus altura > 0.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static ERTCombatOutcome ClassifyCombatOutcome(int32 HealthBefore, int32 HealthAfter, int32 AttackerDmgBonus);
};
