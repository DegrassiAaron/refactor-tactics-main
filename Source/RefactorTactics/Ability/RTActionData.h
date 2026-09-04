#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Ability/RTActionDef.h"
#include "RTActionData.generated.h"

/**
 * Una CONFIGURAZIONE alternativa di un'abilita' d'eroe: stesso ActionId, effetti diversi, scelta nel loadout
 * pre-partita (catalogo eroi v0.1 §6, "Variante di <Abilita>"). E' un compromesso ORIZZONTALE — cambia COME
 * si usa l'abilita', non QUANTO e' forte — lo stesso principio degli equipaggiamenti
 * (`URTEquipmentData::Drawback`), applicato qui a UNA abilita' fondamentale invece che al loadout intero.
 *
 * Il catalogo v0.1 vincola: **una sola** abilita' fondamentale per eroe puo' dichiarare varianti
 * (`URTHeroCatalogLibrary::ValidateHeroes` lo fa valere).
 */
USTRUCT(BlueprintType)
struct FRTAbilityVariant
{
	GENERATED_BODY()

	/** ID stabile della variante (es. `Gadget.LinearDischarge.Branched`). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FName VariantId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FText DisplayName;

	/** Il compromesso in una riga leggibile (UI/log): il numero vive in `Effects`, qui sta il PERCHE'. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FText Tradeoff;

	/**
	 * Effetti di QUESTA variante, che sostituiscono per intero `URTActionData::Def.Effects` quando la
	 * variante e' selezionata nel loadout. Stesso schema di `FRTActionDef::Effects`: un cambio di bersagli
	 * aggiuntivi o di tipo di effetto (non solo di quantita') resta rappresentabile senza un secondo modello.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	TArray<FRTActionEffectSpec> Effects;

	/**
	 * Numeri di catalogo della variante che **non sono effetti** e che nessun sistema consuma ancora:
	 * l'integrita' di una struttura (`Riktor.KineticPanel`: 45 rinforzato / 25 adattivo), la durata di una
	 * marcatura, il numero di celle controllate.
	 *
	 * Esistono perche' il catalogo li DICHIARA e il compromesso fra due varianti va reso verificabile — non
	 * perche' qualcosa li legga: le strutture sono E9, le reazioni E5. Sono una dichiarazione, non un
	 * meccanismo, e la differenza sta tutta qui: il giorno in cui E9 costruira' le strutture, questi numeri
	 * saranno gia' scritti nel posto giusto invece di essere stati inventati allora.
	 *
	 * NON e' il posto dove mettere un effetto rappresentabile: quello va in `Effects`, che il registry sa
	 * gia' tradurre in eventi. Una `TMap` qui e' l'ultima risorsa per cio' che il modello non copre ancora.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	TMap<FName, int32> Parameters;

	FRTAbilityVariant() = default;
};

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
 * catalogo (docs/balance/RT_ActionCatalog_v0.1.md).
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

	/**
	 * Abilita' di supporto su se stessi (fase Prep): non fa danno, aggiunge scudo pari a Power.
	 * Se false, e' un attacco su un nemico (fase Blast).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	bool bSelfTarget = false;

	/** Turni di ricarica dopo l'uso (0 = nessuna ricarica). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	int32 CooldownTurns = 0;

	/** Se vero, incendia le celle infiammabili nell'area colpita (terreno dinamico). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	bool bIgnites = false;

	/**
	 * Configurazioni alternative di QUESTA abilita' (0, 1 o 2 secondo il catalogo eroi v0.1). Vuoto = nessuna
	 * variante: l'abilita' esiste solo nella sua forma base. Non e' un menu di potenziamenti: due varianti si
	 * ESCLUDONO a vicenda nel loadout, e ognuna dichiara un compromesso, mai un puro vantaggio.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	TArray<FRTAbilityVariant> Variants;

	/**
	 * La variante con quell'id, o nullptr — anche per un id nullo, che significa «nessuna variante scelta»:
	 * l'abilita' vale allora coi numeri del catalogo base.
	 *
	 * E' il primo consumatore runtime di `Variants` (CP 9.5). Fino a qui i `Parameters` erano una
	 * dichiarazione che nessuno leggeva: il commento del catalogo lo diceva apertamente — «il giorno in cui E9
	 * costruira' le strutture, questi numeri saranno gia' scritti nel posto giusto invece di essere stati
	 * inventati allora».
	 */
	const FRTAbilityVariant* FindVariant(const FName& VariantId) const
	{
		if (VariantId.IsNone()) { return nullptr; }
		for (const FRTAbilityVariant& Variant : Variants)
		{
			if (Variant.VariantId == VariantId) { return &Variant; }
		}
		return nullptr;
	}
};
