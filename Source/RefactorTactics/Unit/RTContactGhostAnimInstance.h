#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/PoseSnapshot.h"
#include "AnimNodes/AnimNode_PoseSnapshot.h"
#include "RTContactGhostAnimInstance.generated.h"

/**
 * 🔴 **Il grafo della SAGOMA dell'ultimo contatto: un solo nodo, e nessun `.uasset`** (#1750, secondo
 * criterio).
 *
 * Applica alla sagoma la posa che l'unita' aveva **quando la squadra l'ha vista l'ultima volta**, catturata
 * one-shot da `ARTUnit::RefreshComponentVisibility` prima di nascondere la skeletal viva.
 *
 * ```text
 *     FPoseSnapshot ── FAnimNode_PoseSnapshot ── Output
 *      (dall'attore)     (Mode = SnapshotPin)
 * ```
 *
 * ⛔ **Non puo' animarsi, per COSTRUZIONE.** Nel grafo non esiste nessun sequence player e nessuna sorgente
 * di tempo: qualunque cosa faccia il motore, questo grafo valuta sempre la stessa posa. E' una garanzia
 * piu' forte di quella del ripiego, che poggia su uno `Stop()` che qualcuno potrebbe togliere.
 * Una sagoma che si animasse mentre l'unita' e' nascosta non sarebbe un ricordo: sarebbe una telecamera
 * sul nemico — il **terzo leak** che #1750 conta due volte nel titolo.
 *
 * 🔑 **C++ puro, sul modello di `URTUnitAnimInstance`.** La lettura ovvia di *«applicare un `FPoseSnapshot`
 * richiede un `AnimInstance` che lo consumi»* — scritta in `UpdateContactGhost` col primo criterio — e'
 * *«serve un AnimBlueprint»*, ed e' falsa qui: `URTUnitAnimInstance` monta gia' quattro nodi a mano con un
 * `FAnimInstanceProxy`, e il repository ha scelto quella strada su un numero (**~2,8 MB** di AnimBP
 * duplicati contro gli **0,7 MB** che pesa tutto `Content/` versionato). Questo grafo ne monta uno solo.
 *
 * ⚠️ **Solo presentazione** (invariante #1): niente qui decide un esito, e niente entra nello snapshot di
 * simulazione. La posa **non** e' in `FRTLastKnownContact` — vedi §2 del referto del 2026-08-30.
 */
UCLASS()
class REFACTORTACTICS_API URTContactGhostAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/**
	 * La posa da mostrare. La scrive `ARTUnit::UpdateContactGhost` copiandola dal proprio ricordo; il proxy
	 * la porta nel nodo in `PreUpdate`, cioe' sul game thread.
	 *
	 * ⚠️ `Transient`: e' un ricordo di presentazione, non uno stato da serializzare. Se un giorno finisse
	 * in un salvataggio sarebbe il primo passo verso il posto sbagliato.
	 */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "RefactorTactics|Anim")
	FPoseSnapshot Snapshot;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
};

/**
 * Il proxy: monta il solo `FAnimNode_PoseSnapshot` e ne fa la radice.
 *
 * 🔴 **`GetCustomRootNode` e `GetCustomNodes` sono i due agganci con cui un grafo montato a mano si
 * dichiara al motore**, e servono per due ragioni distinte:
 *
 *  1. `InitializeRootNode` prende la radice **solo** da `GetCustomRootNode`. Senza, `RootNode` resta
 *     `nullptr` e i nodi non ricevono mai `Initialize_AnyThread` ne' `CacheBones` — e' il difetto di
 *     `#1763`, che su Riktor si vedeva come catene distese **dopo un cambio di LOD**;
 *  2. `GetCustomNodes` e' cio' che porta questo nodo in `GameThreadPreUpdateNodes`
 *     (`FAnimInstanceProxy::InitializeNode`, che ce lo aggiunge quando `HasPreUpdate()` e' vero).
 *     ⚠️ **`FAnimNode_PoseSnapshot::PreUpdate` e' dove il nodo cachea i nomi delle ossa del target**:
 *     senza quella chiamata `TargetBoneNames` resta vuoto, `ApplyPose` non mappa nulla e la sagoma torna
 *     in **posa di riferimento** — cioe' il difetto di partenza, riprodotto dentro la sua correzione, e
 *     senza un errore in log.
 */
USTRUCT()
struct FRTContactGhostAnimProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FRTContactGhostAnimProxy() = default;
	explicit FRTContactGhostAnimProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance) {}

	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual void UpdateAnimationNode(const FAnimationUpdateContext& InContext) override;
	virtual bool Evaluate(FPoseContext& Output) override;

	virtual FAnimNode_Base* GetCustomRootNode() override { return &PoseNode; }

	virtual void GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes) override
	{
		OutNodes.Add(&PoseNode);
	}

	/** Il nome della mesh dello snapshot attualmente montato: e' cio' che un test puo' leggere. */
	FName GetSnapshotMeshName() const { return PoseNode.Snapshot.SkeletalMeshName; }

private:
	/**
	 * ⚠️ **`SnapshotPin` e non `NamedSnapshot`.** La cache interna dell'AnimInstance
	 * (`SavePoseSnapshot`/`GetPoseSnapshot`) vuole che sia l'AnimInstance stessa a catturare la posa —
	 * e la sagoma non ha nessuna posa da catturare: la sua arriva dalla skeletal **viva**, che e' un altro
	 * componente. Con `SnapshotPin` la posa e' un dato che si passa da fuori, ed e' esattamente il caso.
	 */
	FAnimNode_PoseSnapshot PoseNode;
};
