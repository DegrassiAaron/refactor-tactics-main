#include "Unit/RTUnitAnimInstance.h"

#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Unit/RTUnit.h"

namespace
{
	/**
	 * Il default del roster, dai pack Paragon.
	 *
	 * ⚠️ **I nomi sono MISURATI sul disco**, non dedotti: §AS.3b della guida animazioni li ha contati, e
	 * **sei caselle su venti** divergono da quelle di Gideon. Le tre che si vedono qui sono la corsa di
	 * Gadget (`Run_Fwd`, non `Jog_Fwd`) e l'idle di Wraith (`Idle_NonCombat`).
	 */
	FString ClipPath(const TCHAR* Pack, const TCHAR* Clip)
	{
		return FString::Printf(
			TEXT("/Game/FabAsset/Paragon/Paragon%s/Characters/Heroes/%s/Animations/%s.%s"),
			Pack, Pack, Clip, Clip);
	}

	FRTLocomotionClips MakeClips(const TCHAR* Pack, const TCHAR* Idle, const TCHAR* Run)
	{
		FRTLocomotionClips Clips;
		Clips.Idle = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(ClipPath(Pack, Idle)));
		Clips.Run = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(ClipPath(Pack, Run)));
		return Clips;
	}
}

URTUnitAnimInstance::URTUnitAnimInstance()
{
	ClipsPerHero.Add(FName(TEXT("Hero.Gadget")), MakeClips(TEXT("Gadget"), TEXT("Idle"), TEXT("Run_Fwd")));
	ClipsPerHero.Add(FName(TEXT("Hero.Phase")), MakeClips(TEXT("Phase"), TEXT("Idle"), TEXT("Jog_Fwd")));
	ClipsPerHero.Add(FName(TEXT("Hero.Riktor")), MakeClips(TEXT("Riktor"), TEXT("Idle"), TEXT("Jog_Fwd")));
	ClipsPerHero.Add(FName(TEXT("Hero.Wraith")), MakeClips(TEXT("Wraith"), TEXT("Idle_NonCombat"), TEXT("Jog_Fwd")));
}

FAnimInstanceProxy* URTUnitAnimInstance::CreateAnimInstanceProxy()
{
	return new FRTUnitAnimProxy(this);
}

void FRTUnitAnimProxy::Initialize(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::Initialize(InAnimInstance);

	// Il grafo, montato a mano: Idle e Run entrano nel blend, il blend entra nello slot, lo slot e' la
	// radice. Senza questi `SetLinkNode` i `FPoseLink` restano scollegati — nell'AnimBlueprint li
	// risolve il compilatore, qui non c'e' nessun compilatore.
	Blend.A.SetLinkNode(&IdleNode);
	Blend.B.SetLinkNode(&RunNode);
	Slot.Source.SetLinkNode(&Blend);

	// Lo slot di default: e' quello che `PlayAnimMontage` usa quando non gliene si passa un altro, ed e'
	// il punto in cui i montaggi `Cast`/`Hit`/`Death` entreranno **in override** su idle e corsa.
	Slot.SlotName = FAnimSlotGroup::DefaultSlotName;

	// `bLoopAnimation` e' protected sulla `_Standalone`: si passa dal setter, che e' il modo previsto.
	IdleNode.SetLoopAnimation(true);
	RunNode.SetLoopAnimation(true);
	Blend.Alpha = 0.f; // si parte fermi

	// Le clip dell'eroe di QUESTA unita'. `HeroId` sta sull'attore, non sull'AnimInstance: e' il dato
	// che il GameMode ha gia' scritto quando ha allestito la partita.
	const URTUnitAnimInstance* Owner = Cast<URTUnitAnimInstance>(InAnimInstance);
	if (Owner == nullptr)
	{
		return;
	}

	// ⚠️ `GetOwningActor`, non `TryGetPawnOwner`: `ARTUnit` deriva da `AActor` e **non** da `APawn`.
	// `TryGetPawnOwner` e' il nodo che ogni tutorial usa, qui restituisce null, e la macchina resterebbe
	// ferma senza un errore, senza un warning e senza un log. E' lo stesso scoglio che la guida
	// animazioni scrive in chiaro per chi lavora in Blueprint.
	const ARTUnit* Unit = Cast<ARTUnit>(InAnimInstance->GetOwningActor());
	if (Unit == nullptr)
	{
		return;
	}

	const FRTLocomotionClips* Clips = Owner->FindClipsFor(Unit->HeroId);
	if (Clips == nullptr)
	{
		return; // eroe senza voce: resta in posa di riferimento, e la partita si gioca uguale
	}

	// `LoadSynchronous` su un soft pointer che non risolve restituisce `nullptr` senza rumore: e' il
	// caso di chi ha clonato il repository senza i pack Paragon, che sono gitignorati.
	IdleNode.SetSequence(Clips->Idle.LoadSynchronous());
	RunNode.SetSequence(Clips->Run.LoadSynchronous());
}

void FRTUnitAnimProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);

	// 🔴 **Qui, e non in `Update`.** `PreUpdate` gira sul GAME THREAD, dove leggere lo stato di un attore
	// e' lecito; `Update_AnyThread` puo' girare su un worker, e toccare l'unita' da li' sarebbe una
	// lettura in corsa. Si copia il valore adesso e il grafo lavora sulla copia.
	const ARTUnit* Unit = InAnimInstance ? Cast<ARTUnit>(InAnimInstance->GetOwningActor()) : nullptr;
	Blend.Alpha = (Unit != nullptr && Unit->bIsMovingVisually) ? 1.f : 0.f;
}

void FRTUnitAnimProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext)
{
	Slot.Update_AnyThread(InContext);
}

bool FRTUnitAnimProxy::Evaluate(FPoseContext& Output)
{
	Slot.Evaluate_AnyThread(Output);
	return true;
}
