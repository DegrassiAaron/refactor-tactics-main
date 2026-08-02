#include "UI/RTHUD.h"
#include "Unit/RTUnit.h"
#include "Ability/RTAbilityData.h"
#include "Player/RTPlayerController.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnRules.h"
#include "Combat/RTCombatLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Grid/RTGridActor.h"
#include "Grid/RTGridLibrary.h"
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

	// Intento nemico RIVELATO (status Reveal): durante la pianificazione mostra il piano
	// dell'avversario. La visibilita' rispetta l'invariante #6 (privacy dell'intento).
	if (TurnManager && TurnManager->GetPhase() == ERTMatchPhase::Planning)
	{
		const int32 PlayerTeam = 0; // il giocatore controlla il team 0 (blu)
		const ARTGridActor* Grid = Cast<ARTGridActor>(UGameplayStatics::GetActorOfClass(this, ARTGridActor::StaticClass()));
		const FVector Origin = Grid ? Grid->GetActorLocation() : FVector::ZeroVector;
		const float CellSize = Grid ? Grid->CellSize : 200.f;

		for (AActor* Actor : Actors)
		{
			const ARTUnit* Unit = Cast<ARTUnit>(Actor);
			if (!Unit || !Unit->IsAlive() || Unit->TeamId == PlayerTeam)
			{
				continue; // il piano degli alleati non fa parte di questa vista
			}
			if (!URTCombatLibrary::IsIntentVisibleTo(PlayerTeam, Unit->TeamId, Unit->HasStatus(TAG_Status_Reveal)))
			{
				continue; // nemico non rivelato: intento privato
			}

			// Descrizione dell'intento pianificato.
			FString Intent;
			const URTAbilityData* Planned = Unit->GetAbility(Unit->PlannedAbilityIndex);
			if (Planned && Unit->PlannedAttackTarget)
			{
				Intent = FString::Printf(TEXT("%s -> %s"), *Planned->DisplayName.ToString(), *Unit->PlannedAttackTarget->GetName());
			}
			else if (Planned && Planned->bSelfTarget)
			{
				Intent = Planned->DisplayName.ToString();
			}
			else if (Unit->PlannedCell != Unit->GridCell)
			{
				Intent = TEXT("si muove");
			}
			else
			{
				Intent = TEXT("fermo");
			}

			// Etichetta sopra la testa del nemico rivelato.
			const FVector Head = Unit->GetActorLocation() + FVector(0.f, 0.f, WorldHeadOffset);
			const FVector Screen = Project(Head);
			if (Screen.Z > 0.f)
			{
				DrawText(FString::Printf(TEXT("[REVEAL] %s"), *Intent),
					FLinearColor(1.f, 0.9f, 0.2f, 1.f), Screen.X - BarWidth * 0.5f, Screen.Y - 36.f, nullptr, 0.85f);
			}

			// Se si muove, evidenzia la cella di destinazione pianificata.
			if (Unit->PlannedCell != Unit->GridCell)
			{
				const FVector Dest = URTGridLibrary::CellToWorld(Unit->PlannedCell, Origin, CellSize);
				const FVector DestScreen = Project(Dest);
				if (DestScreen.Z > 0.f)
				{
					DrawRect(FLinearColor(1.f, 0.9f, 0.2f, 0.35f), DestScreen.X - 12.f, DestScreen.Y - 12.f, 24.f, 24.f);
				}
			}
		}
	}

	// Barra di stato in alto: turno, fase e timer di pianificazione.
	if (TurnManager)
	{
		const TCHAR* PhaseName = TEXT("");
		switch (TurnManager->GetPhase())
		{
		case ERTMatchPhase::Planning:   PhaseName = TEXT("Pianificazione"); break;
		case ERTMatchPhase::MatchEnded: PhaseName = TEXT("Fine"); break;
		default:                        PhaseName = TEXT("Risoluzione"); break;
		}
		FString Status = FString::Printf(TEXT("Turno %d  -  %s"), TurnManager->GetTurnNumber(), PhaseName);
		const float Remaining = TurnManager->GetPlanningTimeRemaining();
		if (TurnManager->GetPhase() == ERTMatchPhase::Planning && Remaining > 0.f)
		{
			Status += FString::Printf(TEXT("  -  %.0fs"), FMath::CeilToFloat(Remaining));
		}
		float TW = 0.f, TH = 0.f;
		GetTextSize(Status, TW, TH, nullptr, 1.2f);
		DrawText(Status, FLinearColor::White, (Canvas->SizeX - TW) * 0.5f, 16.f, nullptr, 1.2f);
	}

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

	// Barra abilita' dell'unita' selezionata (in basso al centro).
	if (const ARTPlayerController* RTPC = Cast<ARTPlayerController>(GetOwningPlayerController()))
	{
		if (const ARTUnit* Sel = RTPC->GetSelectedUnit())
		{
			const float LineH = 18.f;
			float Y = Canvas->SizeY - 24.f - LineH * (Sel->NumAbilities() - 1);
			const float X = Canvas->SizeX * 0.45f;
			for (int32 A = 0; A < Sel->NumAbilities(); ++A)
			{
				const URTAbilityData* Ability = Sel->GetAbility(A);
				if (!Ability)
				{
					continue;
				}
				const bool bActive = (A == Sel->SelectedAbilityIndex);
				const bool bUsable = Sel->CanUseAbility(A);
				const int32 CD = Sel->GetAbilityCooldown(A);

				FString Line = FString::Printf(TEXT("%d. %s"), A + 1, *Ability->DisplayName.ToString());
				if (CD > 0) { Line += FString::Printf(TEXT("  (ricarica %d)"), CD); }
				else if (Ability->EnergyCost > 0 && Sel->Energy < Ability->EnergyCost) { Line += TEXT("  (energia)"); }
				if (bActive) { Line = TEXT("> ") + Line; }

				const FLinearColor Color = bActive ? FLinearColor::White
					: (bUsable ? FLinearColor(0.8f, 0.8f, 0.8f, 1.f) : FLinearColor(0.45f, 0.45f, 0.45f, 1.f));
				DrawText(Line, Color, X, Y, nullptr, 1.f);
				Y += LineH;
			}
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
