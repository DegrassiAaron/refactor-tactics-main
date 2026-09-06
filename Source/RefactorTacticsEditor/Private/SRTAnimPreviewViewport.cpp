#include "SRTAnimPreviewViewport.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "EditorViewportClient.h"
#include "Engine/SkeletalMesh.h"

namespace
{
	/** Il client di viewport: una camera che guarda la mesh, e niente altro. */
	class FRTAnimPreviewClient : public FEditorViewportClient
	{
	public:
		FRTAnimPreviewClient(FPreviewScene* InScene, const TSharedRef<SEditorViewport>& InViewport)
			: FEditorViewportClient(nullptr, InScene, InViewport)
		{
			SetViewMode(VMI_Lit);
			SetRealtime(true);   // senza, l'animazione non avanza e la preview sembra rotta
			bDisableInput = false;
			SetViewLocation(FVector(0.f, -260.f, 110.f));
			SetLookAtLocation(FVector(0.f, 0.f, 90.f));
		}

		// ⛔ Nessun override che tocchi lo stato del gioco: questa e' una scena d'anteprima e non ha un
		// mondo di partita. E' la ragione per cui non deriva da nulla di `RefactorTactics`.
	};
}

void SRTAnimPreviewViewport::Construct(const FArguments&)
{
	PreviewScene = MakeShared<FPreviewScene>(FPreviewScene::ConstructionValues());
	SEditorViewport::Construct(SEditorViewport::FArguments());
}

SRTAnimPreviewViewport::~SRTAnimPreviewViewport() = default;

TSharedRef<FEditorViewportClient> SRTAnimPreviewViewport::MakeEditorViewportClient()
{
	return MakeShared<FRTAnimPreviewClient>(PreviewScene.Get(), SharedThis(this));
}

void SRTAnimPreviewViewport::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Mesh);
}

void SRTAnimPreviewViewport::SetClip(UAnimSequence* Clip)
{
	LastError.Reset();

	if (Mesh != nullptr)
	{
		Mesh->DestroyComponent();
		Mesh = nullptr;
	}
	if (Clip == nullptr || !PreviewScene.IsValid())
	{
		return;
	}

	USkeleton* Skeleton = Clip->GetSkeleton();
	if (Skeleton == nullptr)
	{
		LastError = TEXT("la clip non dichiara uno Skeleton");
		return;
	}

	// 🔑 **La mesh viene dallo Skeleton della clip, e non da un default.** Montare l'animazione di Gadget
	// su una mesh qualunque produce deformazioni che sembrano un difetto della clip — e questo pannello
	// esiste perche' una persona giudichi la clip, non l'accoppiamento sbagliato che gliel'ha mostrata.
	USkeletalMesh* PreviewMesh = Skeleton->GetPreviewMesh();
	if (PreviewMesh == nullptr)
	{
		// ⛔ Vuoto e dichiarato, invece di qualcosa di sbagliato. Un'anteprima che mostra la mesh
		// dell'eroe precedente e' peggio di un'anteprima assente: si giudica la clip su un corpo che non
		// e' il suo.
		LastError = FString::Printf(
			TEXT("lo Skeleton '%s' non ha una preview mesh: la clip non si puo' mostrare"),
			*Skeleton->GetName());
		return;
	}

	Mesh = NewObject<USkeletalMeshComponent>(GetTransientPackage());
	Mesh->SetSkeletalMesh(PreviewMesh);
	PreviewScene->AddComponent(Mesh, FTransform::Identity);

	// `SingleNodeInstance` e' il modo previsto per suonare UNA sequenza senza un AnimBlueprint: e' cio'
	// che usa l'editor di animazione dell'engine, e non richiede il grafo di `URTUnitAnimInstance`.
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Mesh->SetAnimation(Clip);
	Mesh->SetPlayRate(PlayRate);
	Mesh->OverrideAnimationData(Clip, bLooping, /*bIsPlaying*/ true, /*Position*/ 0.f, PlayRate);
	Mesh->Play(bLooping);
}

void SRTAnimPreviewViewport::Play()
{
	if (Mesh != nullptr)
	{
		Mesh->Play(bLooping);
	}
}

void SRTAnimPreviewViewport::Pause()
{
	if (Mesh != nullptr)
	{
		Mesh->Stop();   // `Stop` sul single node mette in pausa: la posizione resta dov'e'
	}
}

void SRTAnimPreviewViewport::Restart()
{
	if (Mesh != nullptr)
	{
		Mesh->SetPosition(0.f, /*bFireNotifies*/ false);
		Mesh->Play(bLooping);
	}
}

void SRTAnimPreviewViewport::SetLooping(bool bLoop)
{
	bLooping = bLoop;
	if (Mesh != nullptr)
	{
		if (UAnimSingleNodeInstance* Single = Mesh->GetSingleNodeInstance())
		{
			Single->SetLooping(bLooping);
		}
	}
}

void SRTAnimPreviewViewport::SetPlayRate(float Rate)
{
	// Si accetta anche molto lento: giudicare la partenza e il recovery di un attacco a 0,1x e' l'uso
	// per cui questo controllo esiste. Lo zero no — sarebbe una pausa mascherata da velocita'.
	PlayRate = FMath::Max(Rate, KINDA_SMALL_NUMBER);
	if (Mesh != nullptr)
	{
		Mesh->SetPlayRate(PlayRate);
	}
}

bool SRTAnimPreviewViewport::IsPlaying() const
{
	return Mesh != nullptr && Mesh->IsPlaying();
}
