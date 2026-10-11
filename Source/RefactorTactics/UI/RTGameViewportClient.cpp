#include "UI/RTGameViewportClient.h"

#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "UI/RTHudLayout.h"
#include "UnrealClient.h"

void URTGameViewportClient::LayoutPlayers()
{
	Super::LayoutPlayers();

	if (!IsSceneInsetActive() || Viewport == nullptr)
	{
		return;
	}

	const TArray<ULocalPlayer*>& Giocatori = GetOuterUEngine()->GetGamePlayers(this);
	if (Giocatori.Num() != 1 || Giocatori[0] == nullptr)
	{
		return; // lo split-screen tiene la tabella di UE: e' fuori da #3661
	}

	const FIntPoint Misura = Viewport->GetSizeXY();
	FVector2D Origine;
	FVector2D Dimensione;
	RTHudLayout::SceneOriginSize(Misura, RTHudLayout::UiScaleFor(Misura), RTHudLayout::Bands(), Origine, Dimensione);
	Giocatori[0]->Origin = Origine;
	Giocatori[0]->Size = Dimensione;
}

void URTGameViewportClient::SetSceneInsetRequested(bool bRequested)
{
	SceneInsetRequests = FMath::Max(0, SceneInsetRequests + (bRequested ? 1 : -1));
}

URTGameViewportClient* URTGameViewportClient::Get(const UObject* WorldContext)
{
	const UWorld* Mondo = WorldContext ? WorldContext->GetWorld() : nullptr;
	return Mondo ? Cast<URTGameViewportClient>(Mondo->GetGameViewport()) : nullptr;
}
