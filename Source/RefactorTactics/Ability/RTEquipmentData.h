#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Turn/RTActionEvent.h" // FRTActionEffectSpec: una variante aggiunge effetti all'attacco base
#include "RTEquipmentData.generated.h"

/** Categoria di equipaggiamento: uno slot per categoria, per eroe. */
UENUM(BlueprintType)
enum class ERTEquipmentSlot : uint8
{
	WeaponVariant, // variante d'arma (modifica l'attacco base)
	Gadget,        // oggetto attivo con cooldown
	ReactionModule // reazione con trigger dichiarato
};

/**
 * Definizione data-driven di un equipaggiamento del catalogo v0.1.
 *
 * Regola di progetto, verificata dal validator: ogni equipaggiamento dichiara **almeno uno svantaggio**.
 * La scelta e' orizzontale — cambia COME si gioca, non QUANTO si e' forti — ed e' il pilastro «nessuna potenza
 * permanente pay-to-win» del canone. Il catalogo elenca «rendere un equipaggiamento migliore in ogni
 * parametro» fra gli errori da evitare.
 *
 * Riferimento: docs/balance/RT_EquipmentCatalog_v0.1.md
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTEquipmentData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** ID stabile (es. `Weapon.Precision`, `Gadget.Medkit`, `Reaction.CounterShot`). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FName EquipmentId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	ERTEquipmentSlot Slot = ERTEquipmentSlot::Gadget;

	/** Cosa migliora (testo leggibile: la regola numerica vive nell'azione che modifica). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FText Advantage;

	/** Cosa peggiora. **Obbligatorio**: senza, l'equipaggiamento e' una scelta verticale. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FText Drawback;

	/** Ricarica in turni completi (gadget del catalogo: 3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 CooldownTurns = 0;

	// --- Modificatori dell'attacco base (solo `WeaponVariant`) --------------------------------------------
	//
	// `Advantage` e `Drawback` sono `FText`: prosa per l'interfaccia, che nessuna regola puo' applicare. Una
	// variante che dichiarasse «-4 danni» solo li' sarebbe un equipaggiamento che non modifica niente — il
	// difetto ricorrente del dato senza consumatore. I tre delta qui sotto sono la stessa frase in forma di
	// numero, e `URTCatalogLibrary::ApplyWeaponVariant` e' l'unico posto che li legge.
	//
	// Sono DELTA e non valori assoluti perche' la variante non conosce l'arma che modifica: `Weapon.Precision`
	// vale «+1 su qualunque portata di partenza», e l'attacco base di ogni eroe ha la propria (catalogo eroi).

	/** Danno aggiunto all'attacco base; negativo per le varianti che comprano altro col danno. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 DamageDelta = 0;

	/** Celle di portata aggiunte all'attacco base; negativo per chi si avvicina in cambio d'altro. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 RangeDeltaCells = 0;

	/** Turni di ricarica aggiunti all'attacco base. Positivo **e'** uno svantaggio: si spara piu' di rado. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 CooldownDeltaTurns = 0;

	/**
	 * Effetti che la variante AGGIUNGE all'attacco base (`Push 1` dell'Impatto, `Slow` della Soppressione).
	 *
	 * Aggiunti in coda a quelli dell'azione, mai sostituiti: un attacco base che gia' bagna continua a bagnare.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	TArray<FRTActionEffectSpec> AddedEffects;

	/**
	 * ⚠️ **Dichiarato e NON consumato**, con la ragione scritta: `Weapon.Split` compra un bersaglio in piu' con
	 * −6 danni, ma il motore della v0.1 non ha alcun concetto di «numero di bersagli» — `FRTActionDef` descrive
	 * la forma (`ERTAbilityShape`) e il raggio, non una cardinalita'. Verificato: nessun `MaxTargets`,
	 * `NumTargets` o `TargetCount` esiste in `Source/`.
	 *
	 * Il campo sta qui invece che nella sola prosa perche' il giorno in cui la cardinalita' esistera' il dato
	 * sara' gia' al posto giusto; ma finche' nessuno lo legge, `Weapon.Split` in partita e' **solo** il suo
	 * svantaggio. `Equipment.SplitHasNoConsumerYet` lo pinna, cosi' il giorno in cui qualcuno lo cabla il test
	 * diventa rosso e chiede di essere riscritto, invece di lasciare la meta' buona della variante per strada.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 ExtraTargets = 0;

	/**
	 * Azione core che l'equipaggiamento CONCEDE a chi lo porta (`None` = non concede azioni, come una variante
	 * d'arma che modifica soltanto numeri).
	 *
	 * E' un id e non una copia della definizione: un gadget che duplicasse i numeri dell'azione sarebbe un
	 * secondo catalogo da tenere allineato, e il primo a cambiare romperebbe il secondo in silenzio — lo
	 * stesso motivo per cui `Bastion.Ram` legge `Action.Charge` invece di riscriverlo.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FName GrantedActionId;

	/**
	 * Gli effetti che l'equipaggiamento dichiara PROPRI, e che sostituiscono quelli dell'azione concessa
	 * (`GrantedActionId`). Vuoto = si tengono quelli del core.
	 *
	 * Serve ai moduli di reazione, che prendono dal core **fase, priorita' e trigger** — cio' che li rende
	 * visibili al pass delle reazioni — ma hanno numeri loro: `Reaction.CounterShot` infligge 14 dove
	 * `Action.Counter` ne infligge 16, e `Reaction.ReactiveShield` para invece di colpire pur scattando sullo
	 * stesso trigger. E' lo stesso meccanismo che `MakeHeroReactionFromCoreAction` da' agli eroi, e per la
	 * stessa ragione: senza, l'unico modo di avere numeri diversi sarebbe una seconda azione nel catalogo core,
	 * cioe' due cataloghi da tenere allineati.
	 *
	 * **Sostituiscono, non si sommano** — al contrario di `AddedEffects` delle varianti d'arma, che modificano
	 * un attacco esistente. Un modulo non modifica: e' la reazione.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	TArray<FRTActionEffectSpec> GrantedEffects;

	/** ID primario stabile: `RTEquipment:<EquipmentId>`; senza id ricade sul nome dell'asset. */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		static const FPrimaryAssetType Type(TEXT("RTEquipment"));
		return FPrimaryAssetId(Type, EquipmentId.IsNone() ? GetFName() : EquipmentId);
	}
};
