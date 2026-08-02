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

	// Visualizzazione degli INTENTI di pianificazione (fase Planning).
	// Invariante #6 (privacy dell'intento): il piano di un'unita' e' visibile agli alleati sempre,
	// ai nemici solo se rivelati (status Reveal). Unita' proprie in ciano, nemici rivelati in giallo.
	if (TurnManager && TurnManager->GetPhase() == ERTMatchPhase::Planning)
	{
		const int32 PlayerTeam = 0; // il giocatore controlla il team 0 (blu)
		const ARTGridActor* Grid = Cast<ARTGridActor>(UGameplayStatics::GetActorOfClass(this, ARTGridActor::StaticClass()));
		const FVector Origin = Grid ? Grid->GetActorLocation() : FVector::ZeroVector;
		const float CellSize = Grid ? Grid->CellSize : 200.f;

		for (AActor* Actor : Actors)
		{
			const ARTUnit* Unit = Cast<ARTUnit>(Actor);
			if (!Unit || !Unit->IsAlive())
			{
				continue;
			}
			if (!URTCombatLibrary::IsIntentVisibleTo(PlayerTeam, Unit->TeamId, Unit->HasStatus(TAG_Status_Reveal)))
			{
				continue; // nemico non rivelato: intento privato
			}

			const bool bOwn = (Unit->TeamId == PlayerTeam);
			const URTAbilityData* Planned = Unit->GetAbility(Unit->PlannedAbilityIndex);
			const bool bMoving = (Unit->PlannedCell != Unit->GridCell);
			const bool bHasPlan = bMoving || Unit->PlannedAttackTarget != nullptr || Planned != nullptr;
			if (bOwn && !bHasPlan)
			{
				continue; // unita' propria senza ordine: niente da mostrare
			}

			const FLinearColor Color = bOwn
				? FLinearColor(0.2f, 0.9f, 1.f, 1.f)   // ciano: le tue unita'
				: FLinearColor(1.f, 0.9f, 0.2f, 1.f);  // giallo: nemico rivelato

			// Descrizione testuale dell'intento.
			FString Intent;
			if (Planned && Unit->PlannedAttackTarget)
			{
				Intent = FString::Printf(TEXT("%s -> %s"), *Planned->DisplayName.ToString(), *Unit->PlannedAttackTarget->GetName());
			}
			else if (Planned && Planned->bSelfTarget)
			{
				Intent = Planned->DisplayName.ToString();
			}
			else if (bMoving)
			{
				Intent = FString::Printf(TEXT("-> (%d,%d)"), Unit->PlannedCell.X, Unit->PlannedCell.Y);
			}
			else
			{
				Intent = TEXT("fermo");
			}

			// Etichetta sopra la testa.
			const FVector Head = Unit->GetActorLocation() + FVector(0.f, 0.f, WorldHeadOffset);
			const FVector HeadScreen = Project(Head);
			if (HeadScreen.Z > 0.f)
			{
				const TCHAR* Prefix = bOwn ? TEXT("[PIANO] ") : TEXT("[REVEAL] ");
				DrawText(FString(Prefix) + Intent, Color, HeadScreen.X - BarWidth * 0.5f, HeadScreen.Y - 36.f, nullptr, 0.85f);
			}

			// Percorso pianificato (PF.2): traccia la rotta reale che aggira gli ostacoli
			// (FindPath), cella per cella, ed evidenzia la cella di destinazione.
			if (bMoving)
			{
				TMap<FRTGridCoord, int32> CostMap;
				if (Grid) { Grid->BuildCostMap(CostMap); }
				const int32 GW = Grid ? Grid->Width : 10;
				const int32 GH = Grid ? Grid->Height : 10;
				const TArray<FRTGridCoord> PathCells =
					URTGridLibrary::FindPathByCost(Unit->GridCell, Unit->PlannedCell, CostMap, GW, GH);

				// Polilinea lungo i centri-cella (a terra): mostra la deviazione attorno alle coperture.
				for (int32 i = 1; i < PathCells.Num(); ++i)
				{
					const FVector A = Project(URTGridLibrary::CellToWorld(PathCells[i - 1], Origin, CellSize));
					const FVector B = Project(URTGridLibrary::CellToWorld(PathCells[i], Origin, CellSize));
					if (A.Z > 0.f && B.Z > 0.f)
					{
						DrawLine(A.X, A.Y, B.X, B.Y, Color, 2.f);
					}
				}

				const FVector DestScreen = Project(URTGridLibrary::CellToWorld(Unit->PlannedCell, Origin, CellSize));
				if (DestScreen.Z > 0.f)
				{
					DrawRect(FLinearColor(Color.R, Color.G, Color.B, 0.35f), DestScreen.X - 12.f, DestScreen.Y - 12.f, 24.f, 24.f);
				}
			}

			// Linea verso il bersaglio d'attacco pianificato.
			if (Unit->PlannedAttackTarget && Unit->PlannedAttackTarget->IsAlive() && HeadScreen.Z > 0.f)
			{
				const FVector TgtScreen = Project(Unit->PlannedAttackTarget->GetActorLocation() + FVector(0.f, 0.f, WorldHeadOffset));
				if (TgtScreen.Z > 0.f)
				{
					DrawLine(HeadScreen.X, HeadScreen.Y, TgtScreen.X, TgtScreen.Y, Color, 2.f);
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
