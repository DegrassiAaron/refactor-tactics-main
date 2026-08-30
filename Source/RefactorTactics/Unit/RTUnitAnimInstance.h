#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "AnimNodes/AnimNode_Slot.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "RTUnitAnimInstance.generated.h"

class UAnimSequenceBase;

/**
 * Le due clip di locomozione di UN eroe.
 *
 * Sono `TSoftObjectPtr` e non riferimenti duri per una ragione misurata: i pack Paragon vivono in
 * `Content/FabAsset/`, che e' **gitignorato** (riga 97 del `.gitignore`). Un riferimento duro
 * impedirebbe di aprire il progetto a chi clona il repository senza i pack; un soft pointer che non
 * risolve lascia semplicemente il nodo senza sequenza, e l'unita' resta in posa di riferimento invece
 * di far fallire il caricamento.
 */
USTRUCT(BlueprintType)
struct FRTLocomotionClips
{
	GENERATED_BODY()

	/** Fermo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	TSoftObjectPtr<UAnimSequenceBase> Idle;

	/** In movimento nel playback del turno. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	TSoftObjectPtr<UAnimSequenceBase> Run;
};

/**
 * 🔴 **Il grafo di animazione dell'unita', in C++ e senza nessun `.uasset`.**
 *
 * La via ovvia sarebbe duplicare l'AnimBlueprint di ogni pack Paragon e ricablarne l'ingresso: quei
 * grafi esistono, sono completi e i loro skeleton combaciano con le mesh che i `BP_Unit_*` gia' usano.
 * E' stata scartata su un numero: **650–735 KB l'uno, ~2,8 MB per quattro**, contro gli **0,7 MB** che
 * pesa oggi tutto `Content/` versionato. Quadruplicare il contenuto binario del repository per una
 * macchina a due stati e' un prezzo che si paga a ogni salvataggio successivo, perche' i `.uasset` non
 * si comprimono per delta.
 *
 * Qui le clip sono **dati versionati e diffabili**, il grafo e' codice sotto test, e il repository non
 * cresce di un byte.
 *
 * ⚠️ **Solo presentazione** (invariante #1): niente qui decide un esito. Se una clip manca, l'unita'
 * resta in posa di riferimento e la partita si gioca uguale.
 */
UCLASS()
class REFACTORTACTICS_API URTUnitAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	URTUnitAnimInstance();

	/**
	 * Clip per `HeroId`. Il default C++ punta ai pack del roster; resta `EditDefaultsOnly`, quindi un
	 * Blueprint figlio puo' scavalcare una voce senza ricompilare.
	 *
	 * ⚠️ **I nomi delle clip NON si deducono**: `docs/technical/runbooks/guida-animazioni-paragon.md`
	 * §AS.3b li ha misurati sul disco, e **sei caselle su venti** non si chiamano come ci si aspetta —
	 * su Gadget la corsa e' `Run_Fwd` e non `Jog_Fwd`, su Wraith l'idle e' `Idle_NonCombat`.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	TMap<FName, FRTLocomotionClips> ClipsPerHero;

	/** Le clip dell'eroe passato, o `nullptr` se non ce ne sono: un eroe senza voce non e' un errore. */
	const FRTLocomotionClips* FindClipsFor(const FName& HeroId) const { return ClipsPerHero.Find(HeroId); }

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
};

/**
 * Il grafo vero e proprio: due sequence player, un blend fra loro, uno slot per i montaggi.
 *
 *     Idle  ─┐
 *            ├─ TwoWayBlend ── Slot('DefaultSlot') ── Output
 *     Run   ─┘      (alpha)         (Cast/Hit/Death)
 *
 * Il modello e' `FAnimSequencerInstanceProxy` dell'engine, che costruisce i propri nodi allo stesso
 * modo — `SetLinkNode` sui `FPoseLink`, `Update_AnyThread`/`Evaluate_AnyThread` sul nodo radice.
 */
USTRUCT()
struct FRTUnitAnimProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FRTUnitAnimProxy() = default;
	explicit FRTUnitAnimProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance) {}

	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual void UpdateAnimationNode(const FAnimationUpdateContext& InContext) override;
	virtual bool Evaluate(FPoseContext& Output) override;

	/**
	 * 🔴 **I due agganci con cui un grafo montato a mano si DICHIARA al motore.**
	 *
	 * `FAnimInstanceProxy::InitializeRootNode` assegna la radice da qui e da nessun altro posto —
	 * `RootNode = (FAnimNode_Base*) GetCustomRootNode();`. Senza questo override `RootNode` resta
	 * `nullptr`, e allora **due delle quattro traversate non arrivano mai ai nodi**:
	 *
	 *  1. `Initialize_AnyThread`, che i nodi non ricevono mai;
	 *  2. `CacheBones`, che e' il punto in cui i `FBoneReference` si riallineano all'array delle
	 *     *required bones* — e quell'array **cambia a ogni cambio di LOD**
	 *     (`FAnimInstanceProxy::OnPreUpdateLODChanged`).
	 *
	 * ⚠️ **Il difetto non si vede all'avvio**, ed e' la ragione per cui e' passato: le prime
	 * animazioni sono corrette, e la posa degrada solo dopo che il LOD e' cambiato. Su un umanoide una
	 * cache stantia si nota appena; su **Riktor**, che porta decine di ossa in fila
	 * (`arm_chain_long_r_01`, `_sub_01`, `_sub_02`, ...), le catene si stendono sullo schermo.
	 *
	 * ⛔ **Non e' un dettaglio di stile**: `FRTUnitAnimClipsTest` verifica che le clip dei quattro
	 * eroi siano nel default C++ e resta verde comunque, perche' un test sui DATI non vede un grafo
	 * che non viene inizializzato. Vedi `#1763`.
	 */
	virtual FAnimNode_Base* GetCustomRootNode() override { return &Slot; }

	/** I nodi del grafo, perche' il motore possa raggiungerli tutti e non solo la radice. */
	virtual void GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes) override
	{
		OutNodes.Add(&IdleNode);
		OutNodes.Add(&RunNode);
		OutNodes.Add(&Blend);
		OutNodes.Add(&Slot);
	}

	/** Quanto l'unita' sta correndo, `0` fermo e `1` in corsa. Lo copia `PreUpdate` dal game thread. */
	float GetRunAlpha() const { return Blend.Alpha; }

private:
	/**
	 * ⚠️ **`_Standalone` e non `FAnimNode_SequencePlayer`**: la variante normale prende la propria
	 * sequenza dalla proprieta' che il compilatore dell'AnimBlueprint le assegna, e fuori da un grafo
	 * compilato resta vuota. La `_Standalone` esiste per l'uso da codice, ed e' l'unica delle due che
	 * si puo' riempire a mano.
	 */
	FAnimNode_SequencePlayer_Standalone IdleNode;
	FAnimNode_SequencePlayer_Standalone RunNode;

	FAnimNode_TwoWayBlend Blend;
	FAnimNode_Slot Slot;
};
