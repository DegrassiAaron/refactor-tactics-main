#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Core/RTTypes.h"
#include "Selection/RTSelectable.h"
#include "RTUnit.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class URTActionData;
class URTHeroData;

/** Archetipi dell'unita' con statistiche e abilita' distinte. */
UENUM(BlueprintType)
enum class ERTArchetype : uint8
{
	Ranger,   // fragile, lunga gittata, mobile
	Guardian  // resistente, corta gittata, lento
};

/**
 * Unita' segnaposto per il demo: una mesh su una cella, colorata per team e selezionabile.
 * E' un marker minimale (niente statistiche/abilita' qui): quelle arrivano in M2/M3.
 */
UCLASS()
class REFACTORTACTICS_API ARTUnit : public AActor, public IRTSelectable
{
	GENERATED_BODY()

public:
	ARTUnit();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	int32 TeamId = 0;

	/** Numero massimo di celle percorribili in un turno (distanza di Manhattan). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	int32 MoveRange = 4;

	/** Se vero, l'unita' e' pianificata automaticamente dal bot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Unit")
	bool bIsBotControlled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	FRTCellId Cell;

	/** Cella di destinazione pianificata per il turno corrente (default = cella attuale). */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Unit")
	FRTCellId PlannedCell;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 MaxHealth = 100;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Health = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Shield = 0;

	/** Portata dell'attacco base, in celle (distanza di Manhattan). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 AttackRange = 5;

	/** Danno dell'attacco base. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 AttackPower = 30;

	/**
	 * Statistiche del catalogo eroi v0.1 senza ancora un consumatore in partita: la vista è LOS/FoW (non
	 * costruito), la resistenza push base è distinta da quella temporanea di `Status.Guarded`
	 * (`URTCombatLibrary::GuardResistedPushDistance`, l'unica oggi applicata). Esistono qui perché il DoD di
	 * CP 6.1 le vuole DATI sull'unità, non perché qualcosa le legga già.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	int32 VisionRange = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	int32 PushResistance = 0;

	/** Affinità e debolezza ambientale dell'eroe (identità per le combo fra eroi, es. Flux su bersaglio Wet). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	FName Affinity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	FName Weakness;

	/** Eroe configurato via `ConfigureFromHeroData` (`NAME_None` = unità legacy configurata via archetipo). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	FName HeroId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 MaxEnergy = 100;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Energy = 0;

	/** Energia guadagnata a ogni turno. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Combat")
	int32 EnergyPerTurn = 25;

	/** Energia guadagnata quando si porta a segno un attacco (non ultimate). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Combat")
	int32 EnergyOnHit = 15;

	/** Moltiplicatore di danno dell'ultimate (attacco a energia piena). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Combat")
	int32 UltimateMultiplier = 2;

	/** Raggio dell'area colpita dall'ultimate attorno al bersaglio (0 = singolo bersaglio). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Combat")
	int32 UltimateRadius = 1;

	/** Abilita' data-driven dell'unita' (se vuota, popolata con default in codice all'avvio). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	TArray<TObjectPtr<URTActionData>> Abilities;

	/** Abilita' selezionata dal giocatore per la pianificazione. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Ability")
	int32 SelectedAbilityIndex = 0;

	/** Abilita' pianificata per il turno (INDEX_NONE = nessun attacco). */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Ability")
	int32 PlannedAbilityIndex = INDEX_NONE;

	/** Bersaglio dell'attacco pianificato per il turno (nullo = nessun attacco). */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Combat")
	TObjectPtr<ARTUnit> PlannedAttackTarget = nullptr;

	/**
	 * Percorso composito pianificato (waypoint risolti in celle, From = Cell incluso).
	 * Vuoto o < 2 celle = nessun movimento composito (si usa PlannedCell come destinazione singola).
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Turn")
	TArray<FRTCellId> PlannedPath;

	/**
	 * Waypoint cliccati dal giocatore (esclusa la cella di partenza), da cui deriva PlannedPath.
	 * Vive sull'unita' cosi' la riselezione ne preserva l'editing (aggiungi/annulla step).
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Turn")
	TArray<FRTCellId> PlannedWaypoints;

	/** Abilita' di scatto pianificata per il turno (INDEX_NONE = nessuno scatto). Si risolve in fase Dash. */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Turn")
	int32 PlannedDashAbility = INDEX_NONE;

	/** Cella di destinazione dello scatto pianificato (valida solo se PlannedDashAbility e' impostata). */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Turn")
	FRTCellId PlannedDashCell;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	ERTArchetype Archetype = ERTArchetype::Ranger;

	/**
	 * Distanza di sicurezza per il kiting del bot: se un nemico si avvicina sotto questa soglia
	 * e non c'e' un attacco disponibile, il bot arretra. 0 = non fa kiting (mischia).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	int32 KiteStandoff = 0;

	/** Imposta statistiche e abilita' in base all'archetipo (chiamare prima di FinishSpawning). */
	void ConfigureAsArchetype(ERTArchetype InArchetype);

	/**
	 * Imposta statistiche e azioni interamente da `URTHeroData` (CP 6.1, epic E6): nessun numero scritto qui,
	 * a differenza di `ConfigureAsArchetype` che resta il percorso dei due archetipi legacy finche' CP 6.6 non
	 * sposta lo spawn sui quattro eroi. `Hero == nullptr` non configura nulla (fail-closed, come il resto del
	 * motore azioni: un'unita' senza dati non e' un'unita' con numeri a caso).
	 */
	void ConfigureFromHeroData(const URTHeroData* Hero);

	int32 NumAbilities() const { return Abilities.Num(); }
	URTActionData* GetAbility(int32 Index) const;

	/** Indice della prima abilita' di scatto (bDash), o INDEX_NONE se l'unita' non ne ha. */
	int32 FindDashAbilityIndex() const;

	/** Vero se l'abilita' e' pronta (non in ricarica) e c'e' energia sufficiente. */
	bool CanUseAbility(int32 Index) const;

	/** Cooldown residuo (turni) di un'abilita'. */
	int32 GetAbilityCooldown(int32 Index) const;

	/** Seleziona l'abilita' attiva del giocatore (se l'indice e' valido). */
	void SelectAbility(int32 Index);

	/** Avvia la ricarica dell'abilita' e ne consuma l'energia. */
	void ConsumeAbility(int32 Index);

	/** Decrementa i cooldown di tutte le abilita'. */
	void TickCooldowns();

	/**
	 * Applica lo stato di combattimento risolto (solo logico). Se HP<=0 l'unita' e' morta, ma NON viene
	 * distrutta subito: la rimozione visiva/distruzione e' differita (morte visiva differita, vedi TurnManager).
	 *
	 * Il danno assorbito dallo scudo erode PRIMA la parte temporanea (vedi AddTemporaryShield).
	 */
	void ApplyCombatState(int32 NewHealth, int32 NewShield);

	/**
	 * Aggiunge scudo TEMPORANEO: protegge per il turno corrente e scade nel Cleanup (ExpireTemporaryShield).
	 * E' la forma di protezione delle abilita' di supporto — il catalogo v0.1 dice «lo Scudo scade durante il
	 * Cleanup del turno», e senza scadenza si accumulava turno dopo turno rendendo i duelli interminabili
	 * (issue #96: una partita 2v2 durava 25 turni contro i 12 previsti).
	 */
	void AddTemporaryShield(int32 Amount);

	/** Rimuove la parte temporanea dello scudo (fine turno). Lo scudo BASE dell'unita' resta. */
	void ExpireTemporaryShield();

	/** Quota dello scudo corrente che scadra' a fine turno (diagnostica/HUD). */
	int32 GetTemporaryShield() const { return TemporaryShield; }

	/** Nasconde la mesh e disabilita la collisione (morte visiva; la distruzione avviene a fine turno). */
	void HideForDefeat();

	bool IsAlive() const { return Health > 0; }

	/** Applica uno status per Turns turni (non accorcia una durata gia' piu' lunga). */
	void ApplyStatus(FGameplayTag Tag, int32 Turns);

	/** Vero se lo status e' attivo (durata residua > 0). */
	bool HasStatus(FGameplayTag Tag) const;

	/** Decrementa la durata di tutti gli status; rimuove quelli scaduti. */
	void TickStatuses();

	/** Range di movimento tenendo conto degli status (Root/Slow). */
	int32 GetEffectiveMoveRange() const;

	/** Portata effettiva dello SCATTO con gli status (Root azzera -> niente scatto, Slow dimezza). */
	int32 GetEffectiveDashRange(int32 BaseRange) const;

private:
	/** Quota dello `Shield` corrente che scade nel Cleanup: il resto e' scudo base dell'unita'. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat", meta = (AllowPrivateAccess = "true"))
	int32 TemporaryShield = 0;

	/** Status attivi: tag -> turni residui. */
	UPROPERTY()
	TMap<FGameplayTag, int32> StatusTurns;

	/** Cooldown residuo per abilita' (parallelo a Abilities). */
	UPROPERTY()
	TArray<int32> AbilityCooldowns;

	/** Popola Abilities con un set di default (attacco, colpo pesante, ultimate) se vuota. */
	void EnsureDefaultAbilities();

	/** Crea un'abilita' data-driven in codice. */
	URTActionData* MakeAbility(const FString& Name, int32 Range, int32 Power, int32 Area,
		int32 Cooldown, int32 EnergyCost, FGameplayTag Status, int32 StatusDur);

public:

	/** Semi-altezza della mesh (cilindro base ~100uu con scala Z 1.8 -> ~180, meta' = 90). */
	static constexpr float UnitHalfHeight = 90.f;

	/**
	 * Offset verticale del pivot rispetto al piano della cella (SOLO presentazione).
	 * Default = UnitHalfHeight (90) per il cilindro segnaposto col pivot al centro; impostare a 0 per
	 * personaggi skeletali col pivot ai piedi (via BP_Unit). Non tocca la logica: la griglia resta autoritativa.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	float VisualZOffset = UnitHalfHeight;

	/**
	 * Se vero, durante il movimento visivo l'unita' si orienta verso la direzione di spostamento (solo yaw).
	 * Default false = comportamento invariato (il cilindro non ruota). I BP_Unit dei personaggi lo attivano
	 * cosi' la corsa (es. Jog_Fwd) punta dove vanno. Solo presentazione: non tocca la logica.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	bool bFaceMovementDirection = false;

	/**
	 * Vero mentre l'unita' e' in movimento nel playback del turno. Lo imposta il TurnManager; l'AnimBP lo legge
	 * (un solo Cast to RTUnit, uguale per ogni personaggio) per passare da idle a corsa, senza wiring per-unita'.
	 * Solo presentazione: non tocca la logica.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Unit")
	bool bIsMovingVisually = false;

	/** Posiziona l'unita' al centro-mondo della cella esagonale, con la base appoggiata al piano. */
	void PlaceOnCell(const FRTCellId& InCell, const FVector& Origin, float HexSize, float LayerHeight);

	/**
	 * Posizione-mondo che PlaceOnCell userebbe per InCell, senza modificare lo stato logico (per il playback).
	 * LayerHeight non ha default: su mappa multilivello e' un dato reale della mappa, non un extra opzionale.
	 */
	FVector WorldForCell(const FRTCellId& InCell, const FVector& Origin, float HexSize, float LayerHeight) const;

	/** Sposta solo la mesh (presentazione): NON cambia Cell ne' il piano. Usato dall'animazione del turno. */
	void SetVisualLocation(const FVector& World);

	// --- Eventi di presentazione del combattimento (montaggi): implementati nei BP_Unit, chiamati dal TurnManager.
	//     Se un BP non li implementa non succede nulla (nessun crash): la logica resta invariata (invariante #1).
	/** L'attaccante esegue il montaggio d'attacco (fase Blast). */
	UFUNCTION(BlueprintImplementableEvent, Category = "RefactorTactics|Anim")
	void PlayAttackMontage();

	/** Il bersaglio reagisce al colpo subito. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RefactorTactics|Anim")
	void PlayHitMontage();

	/** L'unita' eliminata esegue il montaggio di morte, prima della rimozione visiva. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RefactorTactics|Anim")
	void PlayDefeatMontage();

	// IRTSelectable
	virtual void OnSelected() override;
	virtual void OnDeselected() override;

	/** Colore del team (TeamId 0 -> Team0, altrimenti Team1). Pura: usata da ApplyTeamColor e testabile. */
	static FLinearColor TeamColorFor(int32 InTeamId, const FLinearColor& Team0, const FLinearColor& Team1);

	/**
	 * Offset Z LOCALE per portare un anello a terra (TeamRing/SelectionRing, figlio della mesh) al piano della
	 * cella: compensa VisualZOffset e la scala Z del genitore. Guardia: ParentScaleZ 0 -> 1 (niente divisione). Pura.
	 */
	static float RingLocalZ(float VisualZOffset, float ParentScaleZ);

protected:
	virtual void BeginPlay() override;

	void ApplyTeamColor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynMaterial;

	/** Anello di team a terra: identita' di squadra visibile anche con personaggi skeletal. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	TObjectPtr<UStaticMeshComponent> TeamRing;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RingDynMaterial;

	/** Materiale dell'anello (M_TeamRing, con parametro "Color"). Assente -> anello nascosto (fallback). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	TSoftObjectPtr<UMaterialInterface> TeamRingMaterial;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	FLinearColor Team0Color = FLinearColor(0.10f, 0.40f, 1.00f);

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	FLinearColor Team1Color = FLinearColor(1.00f, 0.20f, 0.15f);

	/** Anello di SELEZIONE a terra: riscontro visivo della selezione, visibile anche sui personaggi skeletal. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	TObjectPtr<UStaticMeshComponent> SelectionRing;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SelectionRingDynMaterial;

	/** Materiale dell'anello di selezione (parametro "Color"). Assente -> nessun anello di selezione (fallback). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	TSoftObjectPtr<UMaterialInterface> SelectionRingMaterial;

	/** Colore dell'anello di selezione (default giallo). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	FLinearColor SelectionColor = FLinearColor(1.00f, 0.85f, 0.10f);

	/** Scala base della mesh; l'evidenziazione di selezione la moltiplica, non la sostituisce. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	FVector BaseMeshScale = FVector(1.2f, 1.2f, 1.8f);

	/**
	 * Materiale con un parametro vettoriale "Color" usato per il colore-team.
	 * Default: /Game/RT/Art/GlobalMaterials/M_Global_Tint (tint parametrico trasversale, usato anche
	 * dalla griglia). Se assente, l'unita' resta grigia.
	 */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	TSoftObjectPtr<UMaterialInterface> UnitMaterial =
		TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/RT/Art/GlobalMaterials/M_Global_Tint.M_Global_Tint")));
};
