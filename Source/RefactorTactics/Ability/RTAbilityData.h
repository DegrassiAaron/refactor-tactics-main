#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RTAbilityData.generated.h"

/** Forma dell'area colpita da un'abilita'. */
UENUM(BlueprintType)
enum class ERTAbilityShape : uint8
{
	Single, // solo il bersaglio
	Area,   // diamante di raggio AreaRadius attorno al bersaglio
	Line,   // celle sulla traiettoria dall'attaccante al bersaglio
	Cone    // ventaglio dall'attaccante nella direzione del bersaglio
};

/**
 * Definizione data-driven di un'abilita' (Primary Data Asset).
 * Consente di creare varianti (eroi/attacchi diversi) come contenuto, senza toccare il C++.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Nome mostrato (UI/log). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	FText DisplayName;

	/** Portata in celle (distanza di Manhattan). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	int32 RangeCells = 5;

	/** Danno inflitto. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	int32 Power = 30;

	/** Forma dell'area colpita. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	ERTAbilityShape Shape = ERTAbilityShape::Single;

	/** Raggio dell'area colpita attorno al bersaglio (usato con Shape = Area). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	int32 AreaRadius = 0;

	/** Status inflitto ai bersagli (nessuno se il tag e' vuoto). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	FGameplayTag StatusToApply;

	/** Durata in turni dello status inflitto. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	int32 StatusDuration = 0;

	/**
	 * Abilita' di supporto su se stessi (fase Prep): non fa danno, aggiunge scudo pari a Power.
	 * Se false, e' un attacco su un nemico (fase Blast).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	bool bSelfTarget = false;

	/** Turni di ricarica dopo l'uso (0 = nessuna ricarica). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	int32 CooldownTurns = 0;

	/** Energia richiesta e consumata dall'uso (0 = nessun costo; >0 = abilita' "ultimate"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	int32 EnergyCost = 0;

	/** Se vero, incendia le celle infiammabili nell'area colpita (terreno dinamico). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	bool bIgnites = false;

	/**
	 * Abilita' di SCATTO (fase Dash): non attacca, sposta l'unita' fino a RangeCells celle (pathfinding),
	 * risolta PRIMA del Blast — ci si riposiziona prima che gli attacchi colpiscano. Gated da CooldownTurns.
	 * Compatibile col movimento normale del turno (scatto + move).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	bool bDash = false;

	/** Se vero, l'attacco RESPINGE (knockback) i bersagli colpiti di KnockbackDistance celle (fase Blast). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	bool bKnockback = false;

	/** Celle di respinta del knockback (usato con bKnockback). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	int32 KnockbackDistance = 0;
};
