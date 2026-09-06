#include "Unit/RTContactGhostAnimInstance.h"

FAnimInstanceProxy* URTContactGhostAnimInstance::CreateAnimInstanceProxy()
{
	return new FRTContactGhostAnimProxy(this);
}

void FRTContactGhostAnimProxy::Initialize(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::Initialize(InAnimInstance);

	// La sorgente e' la variabile dell'istanza, non la cache interna: vedi il commento su `PoseNode`.
	PoseNode.Mode = ESnapshotSourceMode::SnapshotPin;
}

void FRTContactGhostAnimProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);

	// 🔴 **Qui, e non in `Update`.** `PreUpdate` gira sul GAME THREAD, dove leggere lo stato di un UObject
	// e' lecito; `Update_AnyThread` puo' girare su un worker, e copiare centinaia di transform da li'
	// sarebbe una lettura in corsa. Stessa scelta di `FRTUnitAnimProxy`.
	//
	// ⚠️ La copia e' per VALORE ed e' voluta: il grafo lavora sulla propria istantanea, e chi scrive
	// `Snapshot` sull'AnimInstance non puo' cambiare la posa che il motore sta valutando a meta' strada.
	if (const URTContactGhostAnimInstance* Owner = Cast<URTContactGhostAnimInstance>(InAnimInstance))
	{
		if (PoseNode.Snapshot.SnapshotName != Owner->Snapshot.SnapshotName
			|| PoseNode.Snapshot.LocalTransforms.Num() != Owner->Snapshot.LocalTransforms.Num()
			|| PoseNode.Snapshot.SkeletalMeshName != Owner->Snapshot.SkeletalMeshName)
		{
			PoseNode.Snapshot = Owner->Snapshot;
		}
	}
}

void FRTContactGhostAnimProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext)
{
	PoseNode.Update_AnyThread(InContext);
}

bool FRTContactGhostAnimProxy::Evaluate(FPoseContext& Output)
{
	// ⛔ Nessun tempo, nessuna sequenza, nessun blend: la stessa posa a ogni valutazione. E' la ragione per
	// cui questo grafo esiste separato da `FRTUnitAnimProxy` invece di riusarlo con l'alpha a zero — quello
	// **potrebbe** animarsi, questo no.
	PoseNode.Evaluate_AnyThread(Output);
	return true;
}
