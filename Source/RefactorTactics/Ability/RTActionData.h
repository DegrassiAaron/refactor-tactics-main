#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Ability/RTActionDef.h"
#include "RTActionData.generated.h"

/** Forma dell'area colpita da un'azione. */
UENUM(BlueprintType)
enum class ERTAbilityShape : uint8
{
	Single, // solo il bersaglio
	Area,   // esagono pieno di raggio AreaRadius attorno al bersaglio
	Line,   // celle sulla traiettoria dall'attaccante al bersaglio
	Cone    // ventaglio di 120 gradi dall'attaccante verso il bersaglio
};

/**
 * Definizione data-driven di un'AZIONE (Primary Data Asset): identita' e parametri del catalogo (`Def`) piu'
 * gli effetti concreti che il resolver applica.
 *
 * Era `URTAbilityData`: rinominata al CP 1.3 perche' il catalogo v0.1 parla di *azioni* con ID stabili
 * (`Action.Move`, `Action.HeavyAttack`), e due definizioni parallele — una per le "abilita" del codice e una
 * per le "azioni" del catalogo — avrebbero diviso in due la stessa cosa. Il rename e' stato possibile senza
 * redirector perche' nessun asset referenziava la classe (le abilita' sono create in codice).
 *
 * I campi di effetto qui sotto restano quelli dell'MVP; il motore azioni dell'epic E4 li rileggera' dal
 * catalogo (docs/design/balance/RT_ActionCatalog_v0.1.md).
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTActionData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Identita' e parametri di catalogo dell'azione (ID stabile, fase dichiarata, priorita', costo, fallback).
	 * Vuoto per le abilita' create in codice prima del motore azioni: `ActionId` assente = azione non ancora
	 * catalogata, non un errore.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FRTActionDef Def;

	/** ID primario stabile: `RTAction:<ActionId>`; senza ActionId ricade sul nome dell'asset. */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		static const FPrimaryAssetType Type(TEXT("RTAction"));
		return FPrimaryAssetId(Type, Def.ActionId.IsNone() ? GetFName() : Def.ActionId);
	}

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
