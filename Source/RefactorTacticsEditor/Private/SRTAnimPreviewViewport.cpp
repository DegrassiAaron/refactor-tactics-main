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
			SetRealtime(true);
			bDisableInput = false;
		}

		/**
		 * 🔴 **Senza questo override l'animazione non parte, e `Play` sembra un pulsante rotto.**
		 *
		 * `FEditorViewportClient::Tick` **non fa avanzare il mondo della preview scene** — verificato nel
		 * sorgente del motore (`EditorViewportClient.cpp`): calcola i bounds e ticka i mode tools, e basta.
		 * Il mondo di una `FPreviewScene` lo ticka chi la possiede, ed e' l'idioma che usano tutti i
		 * viewport d'anteprima dell'engine (`SMaterialEditorViewport`, `StaticMeshEditorViewportClient`,
		 * `SCSEditorViewportClient`).
		 *
		 * ⚠️ `SetRealtime(true)` da solo NON basta: dice che il viewport ridisegna ogni frame, non che il
		 * mondo avanzi. Con quello e senza questo si vede la prima posa della clip, ferma per sempre — che
		 * e' esattamente il sintomo riportato.
		 */
		virtual void Tick(float DeltaSeconds) override
		{
			FEditorViewportClient::Tick(DeltaSeconds);
			if (PreviewScene != nullptr)
			{
				if (UWorld* World = PreviewScene->GetWorld())
				{
					World->Tick(IsRealtime() ? LEVELTICK_All : LEVELTICK_TimeOnly, DeltaSeconds);
				}
			}
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

	// 🔴 **L'inquadratura si CALCOLA dai bounds, e non si indovina.** La stesura precedente metteva la
	// camera a `(0, -260, 110)` guardando `(0, 0, 90)`: tre numeri scelti a mano, giusti per nessuno.
	// I quattro eroi hanno taglie diverse — Riktor e' molto piu' alto di Gadget — quindi un personaggio
	// nasceva fuori centro e toccava all'autore trascinare la camera prima di poter giudicare.
	//
	// ⚠️ Si usano i bounds della MESH e non della clip: la clip puo' spostare l'attore (root motion), ma
	// cio' che si deve inquadrare e' il corpo.
	if (Client.IsValid())
	{
		Mesh->UpdateBounds();
		const FBoxSphereBounds B = Mesh->Bounds;
		// `ExpandBy` lascia un margine perche' la silhouette non tocchi i bordi: si giudica anche il
		// profilo, e un personaggio incollato al bordo non lo si legge.
		Client->FocusViewportOnBox(B.GetBox().ExpandBy(B.SphereRadius * 0.25f), /*bInstant*/ true);
	}
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
