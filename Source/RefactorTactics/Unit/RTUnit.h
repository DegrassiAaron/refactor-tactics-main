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
	 * Cella bersagliata dall'azione principale, in alternativa a `PlannedAttackTarget`: le aree si centrano
	 * su una CELLA, che puo' essere vuota (`Flux.Overload` su un varco, una cella conduttiva senza nessuno
	 * sopra). Valida solo con `bAttackTargetsCell`.
	 *
	 * Chiude a meta' il limite dichiarato in `RTTurnManager` (CP 8.3): la pianificazione non aveva un
	 * bersaglio-cella. Resta scoperto il lato HUD — puntare una cella col mouse e' E11.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Plan")
	FRTCellId PlannedAttackCell;

	/**
	 * Bordo bersagliato dalle azioni che agiscono su una STRUTTURA di bordo (CP 9.5: `Action.CreateCover`,
	 * `Bastion.Reconfigure`). Valido solo con `bHasPlannedCoverEdge`.
	 *
	 * Serve un dato in piu' perche' una copertura sta su un BORDO, e una cella ne ha sei: con portata 3 la
	 * coppia (chi la erige, cella bersaglio) non basta a determinarlo — a portata 1 sarebbe bastata, ma il
	 * catalogo azioni dichiara 3. E' l'equivalente per i bordi di `PlannedAttackCell`, e come quello resta
	 * scoperto sul lato HUD: puntare un bordo col mouse e' E11.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Plan")
	ERTHexDirection PlannedCoverEdge = ERTHexDirection::E;

	/**
	 * Vero se il piano dichiara `PlannedCoverEdge`. Serve un flag e non basta un valore di riposo: le sei
	 * direzioni sono tutte legittime, quindi nessuna puo' fare da «nessuna» senza diventare un caso speciale
	 * che il giocatore non puo' dedurre.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Plan")
	bool bHasPlannedCoverEdge = false;

	/**
	 * Vero se l'azione principale mira a `PlannedAttackCell` invece che a un'unita'.
	 *
	 * Serve un flag e non basta «target nullo»: nel resolver `TargetUnitId == INDEX_NONE` significa gia'
	 * **bersaglio perso** (eliminato o mai valido), che degrada al fallback. Senza distinguerle, mirare a una
	 * cella verrebbe letto come un errore di pianificazione.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Plan")
	bool bAttackTargetsCell = false;

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

	/**
	 * Reazione pianificata per il turno (INDEX_NONE = nessuna reazione pronta), slot indipendente da
	 * Movimento e Principale (CP 5.1, epic E5). Non ha una cella o un bersaglio: il trigger dichiarato
	 * dall'abilita' (`FRTActionDef::ReactionTrigger`) e' valutato sullo snapshot del Blast, non scelto qui.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Turn")
	int32 PlannedReactionAbility = INDEX_NONE;

	/**
	 * Ordine di rimozione dichiarato per `Action.Cleanse` (CP 5.2): si purifica il PRIMO stato di questa lista
	 * che l'unita' possiede davvero, e uno solo.
	 *
	 * La scelta e' del giocatore ed e' fatta in PIANIFICAZIONE, mai a runtime: il catalogo lo pretende
	 * esplicitamente ("nessuna scelta implicita"), perche' un'euristica automatica ("togli il piu' dannoso")
	 * renderebbe l'esito impredicibile — e la predizione e' un pilastro di prodotto. Lista vuota = nessuna
	 * scelta dichiarata = nessuna rimozione (fail-closed): il Cleanse risolve senza effetto e lo registra,
	 * invece di indovinare per conto del giocatore.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Turn")
	TArray<FGameplayTag> PlannedCleansePriority;

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

	/**
	 * Indice della prima azione di mobilita' RAPIDA (fase Dash del catalogo), o INDEX_NONE se l'unita' non ne
	 * ha. Il gate e' lo stesso di `ARTTurnManager::ResolveDash`: se qui e li' rispondessero in modo diverso, il
	 * bot pianificherebbe scatti che il resolver rifiuta — o non ne pianificherebbe affatto (#142).
	 */
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

	/**
	 * Durata sentinella: lo stato NON scade da solo, vale finche' la cella sotto i piedi dell'unita' lo
	 * sostiene (CP 8.2, `spec-stati-temporanei-cp82.md` §3 D1). E' la forma di `Wet` sull'acqua bassa e di
	 * `Obscured` nel fumo: un contatore li farebbe scadere mentre l'unita' e' ancora nell'acqua.
	 * La revoca avviene nel Cleanup via RevokeCellBoundStatusesNotIn.
	 */
	static constexpr int32 PersistentWhileOnCell = -1;

	/**
	 * Applica uno status per Turns turni (non accorcia una durata gia' piu' lunga), oppure legato alla cella
	 * se Turns == PersistentWhileOnCell.
	 *
	 * Le due nature convivono senza sovrascriversi: chi e' bagnato dall'acqua E dall'abilita' di Riva resta
	 * bagnato per la durata di Riva anche dopo essere uscito dall'acqua.
	 */
	void ApplyStatus(FGameplayTag Tag, int32 Turns);

	/** Vero se lo status e' attivo (durata residua > 0). */
	bool HasStatus(FGameplayTag Tag) const;

	/**
	 * Applica `Status.Marked` registrando la SQUADRA del marcatore (CP 8.2).
	 *
	 * Il catalogo promette "+6 al prossimo attacco **alleato**": senza sapere chi ha marcato, il marchio
	 * sarebbe una vulnerabilita' generica di cui approfitterebbe anche la squadra del bersaglio.
	 */
	void ApplyMarkedBy(int32 MarkerTeamId, int32 Turns);

	/** Squadra che ha piazzato il marchio, `INDEX_NONE` se l'unita' non e' marcata. */
	int32 GetMarkedByTeam() const { return MarkedByTeam; }

	/**
	 * Rimuove uno status PRIMA della sua scadenza naturale (`Action.Cleanse`, CP 5.2). Ritorna vero se lo
	 * status c'era davvero: il chiamante distingue "purificato" da "non c'era nulla da togliere" senza
	 * doverlo chiedere prima con `HasStatus`.
	 */
	bool RemoveStatus(FGameplayTag Tag);

	/** Decrementa la durata di tutti gli status A TERMINE; rimuove quelli scaduti. Non tocca i persistenti. */
	void TickStatuses();

	/**
	 * Revoca gli stati legati alla cella che la cella corrente non sostiene piu' (Cleanup, CP 8.2).
	 * `Sustained` sono i tag dichiarati dal terreno su cui l'unita' ha TERMINATO il turno: chi e' uscito
	 * dall'acqua smette di essere bagnato subito, senza aspettare il turno successivo.
	 *
	 * Una durata esplicita in corso sullo stesso tag sopravvive: la revoca toglie solo la parte "finche'
	 * sulla cella".
	 */
	void RevokeCellBoundStatusesNotIn(const TSet<FGameplayTag>& Sustained);

	/** Range di movimento tenendo conto degli status (Root/Slow). */
	int32 GetEffectiveMoveRange() const;

	/** Portata effettiva dello SCATTO con gli status (Root azzera -> niente scatto, Slow dimezza). */
	int32 GetEffectiveDashRange(int32 BaseRange) const;

private:
	/** Quota dello `Shield` corrente che scade nel Cleanup: il resto e' scudo base dell'unita'. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat", meta = (AllowPrivateAccess = "true"))
	int32 TemporaryShield = 0;

	/** Status a termine: tag -> turni residui (sempre > 0; gli scaduti vengono rimossi). */
	UPROPERTY()
	TMap<FGameplayTag, int32> StatusTurns;

	/** Squadra che ha applicato `Status.Marked` (INDEX_NONE = nessun marchio attivo). */
	UPROPERTY()
	int32 MarkedByTeam = INDEX_NONE;

	/**
	 * Status legati alla cella (durata PersistentWhileOnCell). Contenitore separato e non una sentinella
	 * dentro StatusTurns: le due nature devono poter coesistere sullo STESSO tag senza che una cancelli
	 * l'altra (acqua bassa + `Riva.PressureJet` sono entrambe sorgenti di `Wet`).
	 */
	UPROPERTY()
	TSet<FGameplayTag> CellBoundStatuses;

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
	 * Nome breve da mostrare sopra l'unita': `Hero.Flux` -> `Flux`. Serve al playtest, dove quattro cilindri
	 * identici rendono impossibile dire chi sta facendo cosa — e senza quello un giudizio sul comportamento
	 * del bot o sul ritmo della partita non vale nulla.
	 *
	 * `NAME_None` (unita' legacy da archetipo) -> `Fallback`, cosi' l'etichetta non sparisce mai. Pura e
	 * testabile: la presentazione non deve inventarsi il nome, lo deriva dall'ID stabile.
	 */
	static FString ShortHeroName(FName InHeroId, const FString& Fallback);

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
