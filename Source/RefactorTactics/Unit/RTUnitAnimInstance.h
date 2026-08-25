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
