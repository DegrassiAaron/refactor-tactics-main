#include "UI/RTHUD.h"
#include "Unit/RTUnit.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnRules.h"
#include "Core/RTGameplayTags.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"

void ARTHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	// Barre HP/scudo sopra ogni unita' viva.
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		const ARTUnit* Unit = Cast<ARTUnit>(Actor);
		if (!Unit || !Unit->IsAlive())
		{
			continue;
		}

		const FVector Head = Unit->GetActorLocation() + FVector(0.f, 0.f, WorldHeadOffset);
		const FVector Screen = Project(Head);
		if (Screen.Z <= 0.f)
		{
			continue; // dietro la camera
		}

		const float X = Screen.X - BarWidth * 0.5f;
		const float Y = Screen.Y - BarHeight;

		// Sfondo.
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X - 1.f, Y - 1.f, BarWidth + 2.f, BarHeight + 2.f);

		// HP: verde (pieno) -> rosso (vuoto).
		const float HpFrac = Unit->MaxHealth > 0 ? FMath::Clamp((float)Unit->Health / Unit->MaxHealth, 0.f, 1.f) : 0.f;
		DrawRect(FLinearColor(1.f - HpFrac, HpFrac, 0.15f, 1.f), X, Y, BarWidth * HpFrac, BarHeight);

		// Scudo: barretta ciano sopra la barra HP (proporzionale a MaxHealth).
		if (Unit->Shield > 0 && Unit->MaxHealth > 0)
		{
			const float ShieldFrac = FMath::Clamp((float)Unit->Shield / Unit->MaxHealth, 0.f, 1.f);
			DrawRect(FLinearColor(0.2f, 0.8f, 1.f, 1.f), X, Y - 4.f, BarWidth * ShieldFrac, 3.f);
		}

		// Energia: barretta sotto la barra HP (oro se ultimate pronta, giallo scuro se in carica).
		if (Unit->MaxEnergy > 0)
		{
			const float EnergyFrac = FMath::Clamp((float)Unit->Energy / Unit->MaxEnergy, 0.f, 1.f);
			const bool bReady = Unit->Energy >= Unit->MaxEnergy;
			const FLinearColor EColor = bReady ? FLinearColor(1.f, 0.85f, 0.1f, 1.f) : FLinearColor(0.5f, 0.45f, 0.1f, 1.f);
			DrawRect(EColor, X, Y + BarHeight + 1.f, BarWidth * EnergyFrac, 3.f);
		}

		// Marker di status sopra la barra HP.
		FString StatusStr;
		if (Unit->HasStatus(TAG_Status_Root)) { StatusStr = TEXT("ROOT"); }
		else if (Unit->HasStatus(TAG_Status_Slow)) { StatusStr = TEXT("SLOW"); }
		if (!StatusStr.IsEmpty())
		{
			DrawText(StatusStr, FLinearColor(1.f, 0.6f, 0.2f, 1.f), X, Y - 20.f, nullptr, 0.8f);
		}
	}

	const ARTTurnManager* TurnManager =
		Cast<ARTTurnManager>(UGameplayStatics::GetActorOfClass(this, ARTTurnManager::StaticClass()));

	// Combat log in basso a sinistra (dal piu' vecchio in alto al piu' recente in basso).
	if (TurnManager)
	{
		const TArray<FString>& Events = TurnManager->GetRecentEvents();
		const float LineH = 16.f;
		float Y = Canvas->SizeY - 24.f - LineH * (Events.Num() - 1);
		for (const FString& Line : Events)
		{
			DrawText(Line, FLinearColor(0.85f, 0.85f, 0.85f, 1.f), 16.f, Y, nullptr, 1.f);
			Y += LineH;
		}
	}

	// Esito + istruzione di riavvio a partita conclusa.
	if (TurnManager && TurnManager->GetPhase() == ERTMatchPhase::MatchEnded)
	{
		const FString Text = TEXT("PARTITA FINITA - premi R per rigiocare");
		float TW = 0.f, TH = 0.f;
		GetTextSize(Text, TW, TH, nullptr, 2.f);
		DrawText(Text, FLinearColor::White, (Canvas->SizeX - TW) * 0.5f, Canvas->SizeY * 0.4f, nullptr, 2.f);
	}
}
