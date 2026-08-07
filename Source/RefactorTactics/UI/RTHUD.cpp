#include "UI/RTHUD.h"
#include "Unit/RTUnit.h"
#include "Turn/RTIntentPrivacyLibrary.h"
#include "Ability/RTActionData.h"
#include "Player/RTPlayerController.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnRules.h"
#include "Combat/RTCombatLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Pathfinding/RTHexPathLibrary.h"
#include "Turn/RTMovementActionLibrary.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// Centro-mondo di una cella esagonale, con la quota del suo layer (per disegnare i path sul ponte).
	// Stessa conversione usata da risoluzione e playback: l'anteprima non puo' divergere dal percorso reale.
	FVector HexCellWorld(const FRTCellId& Cell, const FVector& Origin, float HexSize, float LayerHeight)
	{
		return URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerHeight);
	}

	/** Cella in coordinate assiali, come compaiono nel TurnLog. */
	FString HexCellText(const FRTCellId& Cell)
	{
		return FString::Printf(TEXT("(q=%d,r=%d,L=%d)"), Cell.X, Cell.Y, Cell.Layer);
	}
}

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

	// Geometria della mappa ESAGONALE: unica fonte di scala per ogni conversione cella -> schermo di questa
	// HUD (traccia, anteprime, waypoint). La stessa che usano risoluzione e playback (ARTHexMapActor).
	FVector Origin = FVector::ZeroVector;
	float HexSize = 100.f;
	float LayerH = 250.f;
	const URTHexMapAsset* Map = nullptr;
	if (const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(GetWorld()))
	{
		Map = HexMap->GetHexContext(Origin, HexSize, LayerH);
	}

	// Traccia post-lock: il percorso realmente eseguito nell'ultima risoluzione (grigio, sotto le preview).
	if (TurnManager && TurnManager->GetPhase() == ERTMatchPhase::Planning)
	{
		const FLinearColor TrailColor(0.6f, 0.6f, 0.6f, 0.5f);
		for (const TArray<FRTCellId>& Route : TurnManager->GetLastMoveRoutes())
		{
			for (int32 i = 1; i < Route.Num(); ++i)
			{
				const FVector A = Project(HexCellWorld(Route[i - 1], Origin, HexSize, LayerH));
				const FVector B = Project(HexCellWorld(Route[i], Origin, HexSize, LayerH));
				if (A.Z > 0.f && B.Z > 0.f)
				{
					DrawLine(A.X, A.Y, B.X, B.Y, TrailColor, 1.5f);
				}
			}
		}
	}

	// Visualizzazione degli INTENTI di pianificazione (fase Planning, non durante il playback).
	//
	// Invariante #6 (privacy dell'intento), esteso alle reazioni con CP 5.4. La UI NON legge piu' lo stato di
	// pianificazione delle unita': costruisce i piani autorevoli, li fa filtrare per squadra da
	// `URTIntentPrivacyLibrary::FilterForTeam` e disegna SOLO le viste che tornano indietro.
	//
	// La differenza non e' stilistica. Prima il ciclo scorreva tutte le unita', leggeva il piano completo anche
	// dei nemici e decideva di non disegnarlo: un occultamento GRAFICO, cioe' un dato presente sul client e
	// nascosto a schermo — leggibile con qualunque strumento, e insostenibile quando arrivera' la rete (M10).
	// Ora un piano avversario non rivelato non compare proprio fra le viste, e la reazione di un alleato non
	// viene mai copiata in una vista avversaria.
	if (TurnManager && TurnManager->GetPhase() == ERTMatchPhase::Planning && !TurnManager->IsResolving())
	{
		const int32 PlayerTeam = 0; // il giocatore controlla il team 0 (blu)

		// 1. RACCOGLI i piani autorevoli (in rete: lato server, mai spediti cosi' come sono).
		TArray<FRTPlannedIntent> Authoritative;
		Authoritative.Reserve(Actors.Num());
		for (AActor* Actor : Actors)
		{
			const ARTUnit* Unit = Cast<ARTUnit>(Actor);
			if (!Unit || !Unit->IsAlive()) { continue; }

			const URTActionData* Planned = Unit->GetAbility(Unit->PlannedAbilityIndex);
			const URTActionData* Reaction = Unit->GetAbility(Unit->PlannedReactionAbility);

			FRTPlannedIntent Intent;
			Intent.OwnerCell = Unit->Cell;
			Intent.TeamId = Unit->TeamId;
			Intent.bAlive = true;
			Intent.bRevealed = Unit->HasStatus(TAG_Status_Reveal);
			Intent.bMoving = (Unit->PlannedCell != Unit->Cell);
			Intent.PlannedCell = Unit->PlannedCell;
			Intent.ActionName = Planned ? Planned->DisplayName : FText::GetEmpty();
			Intent.bHasTarget = (Unit->PlannedAttackTarget != nullptr && Unit->PlannedAttackTarget->IsAlive());
			Intent.TargetCell = Intent.bHasTarget ? Unit->PlannedAttackTarget->Cell : Unit->Cell;
			Intent.ReactionName = Reaction ? Reaction->DisplayName : FText::GetEmpty();
			Intent.PlannedPath = Unit->PlannedPath;
			Intent.PlannedWaypoints = Unit->PlannedWaypoints;
			Intent.bDashing = (Unit->PlannedDashAbility != INDEX_NONE);
			Intent.DashCell = Unit->PlannedDashCell;
			if (const URTActionData* DashAb = Unit->GetAbility(Unit->PlannedDashAbility))
			{
				Intent.DashStyle = DashAb->Def.MovementStyle;
			}
			Authoritative.Add(Intent);
		}

		// 2. FILTRA per l'osservatore. Da qui in giu' lo stato completo non si tocca piu'.
		const TArray<FRTIntentView> Views = URTIntentPrivacyLibrary::FilterForTeam(PlayerTeam, Authoritative);

		// 3. DISEGNA le sole viste ricevute.
		for (const FRTIntentView& View : Views)
		{
			const bool bOwn = View.bIsAlly;
			const bool bHasPlan = View.bMoving || View.bHasTarget || !View.ActionName.IsEmpty()
				|| View.bDashing || !View.ReactionName.IsEmpty();
			if (bOwn && !bHasPlan)
			{
				continue; // unita' propria senza ordine: niente da mostrare
			}

			const FLinearColor Color = bOwn
				? FLinearColor(0.2f, 0.9f, 1.f, 1.f)   // ciano: le tue unita'
				: FLinearColor(1.f, 0.9f, 0.2f, 1.f);  // giallo: nemico rivelato

			// Descrizione testuale dell'intento, dalla sola vista.
			FString Intent;
			if (!View.ActionName.IsEmpty() && View.bHasTarget)
			{
				Intent = FString::Printf(TEXT("%s -> %s"), *View.ActionName.ToString(), *HexCellText(View.TargetCell));
			}
			else if (!View.ActionName.IsEmpty())
			{
				Intent = View.ActionName.ToString();
			}
			else if (View.bMoving)
			{
				Intent = FString::Printf(TEXT("-> %s"), *HexCellText(View.PlannedCell));
			}
			else
			{
				Intent = TEXT("fermo");
			}
			// La reazione compare solo se la vista ce l'ha: per un avversario e' vuota per costruzione.
			if (!View.ReactionName.IsEmpty())
			{
				Intent += FString::Printf(TEXT("  (reazione: %s)"), *View.ReactionName.ToString());
			}

			// Etichetta sopra la testa, posizionata dalla CELLA (identita' stabile), non da un pointer all'Actor.
			const FVector Head = HexCellWorld(View.OwnerCell, Origin, HexSize, LayerH) + FVector(0.f, 0.f, WorldHeadOffset);
			const FVector HeadScreen = Project(Head);
			if (HeadScreen.Z > 0.f)
			{
				const TCHAR* Prefix = bOwn ? TEXT("[PIANO] ") : TEXT("[REVEAL] ");
				DrawText(FString(Prefix) + Intent, Color, HeadScreen.X - BarWidth * 0.5f, HeadScreen.Y - 36.f, nullptr, 0.85f);
			}

			// Percorso pianificato: la rotta composita se la vista la porta, altrimenti lo stesso A* dell'autorita'.
			if (View.bMoving)
			{
				const TArray<FRTCellId> PathCells = (View.PlannedPath.Num() >= 2)
					? View.PlannedPath
					: URTHexPathLibrary::FindPath(Map, View.OwnerCell, View.PlannedCell).Path;

				for (int32 i = 1; i < PathCells.Num(); ++i)
				{
					const FVector A = Project(HexCellWorld(PathCells[i - 1], Origin, HexSize, LayerH));
					const FVector B = Project(HexCellWorld(PathCells[i], Origin, HexSize, LayerH));
					if (A.Z > 0.f && B.Z > 0.f)
					{
						DrawLine(A.X, A.Y, B.X, B.Y, Color, 2.f);
					}
				}

				const FVector DestScreen = Project(HexCellWorld(View.PlannedCell, Origin, HexSize, LayerH));
				if (DestScreen.Z > 0.f)
				{
					DrawRect(FLinearColor(Color.R, Color.G, Color.B, 0.35f), DestScreen.X - 12.f, DestScreen.Y - 12.f, 24.f, 24.f);
				}
			}

			// Marker sui waypoint cliccati: la vista li porta solo per le unita' proprie.
			for (const FRTCellId& WP : View.PlannedWaypoints)
			{
				const FVector WPScreen = Project(HexCellWorld(WP, Origin, HexSize, LayerH));
				if (WPScreen.Z > 0.f)
				{
					DrawRect(Color, WPScreen.X - 5.f, WPScreen.Y - 5.f, 10.f, 10.f);
				}
			}

			// Preview dello SCATTO pianificato (fase Dash): percorso e destinazione in MAGENTA.
			//
			// La traiettoria si disegna come la fase Dash la ESEGUIRA' (#142): una mobilita' lineare va dritta
			// e non gira gli angoli, una a budget (`Action.Sprint`) segue il grafo. Disegnare l'A* per uno
			// scatto lineare mostrerebbe un percorso curvo attorno a un ostacolo che in realta' lo ferma — e
			// la leggibilita' tattica e' un pilastro, non un dettaglio estetico.
			if (View.bDashing && Map)
			{
				const TArray<FRTCellId> DPath = URTMovementActionLibrary::IsLinear(View.DashStyle)
					? URTHexLibrary::HexLine(View.OwnerCell, View.DashCell)
					: URTHexPathLibrary::FindPath(Map, View.OwnerCell, View.DashCell).Path;
				const FLinearColor DashColor(1.f, 0.2f, 0.9f, 1.f);
				for (int32 i = 1; i < DPath.Num(); ++i)
				{
					const FVector DA = Project(HexCellWorld(DPath[i - 1], Origin, HexSize, LayerH));
					const FVector DB = Project(HexCellWorld(DPath[i], Origin, HexSize, LayerH));
					if (DA.Z > 0.f && DB.Z > 0.f) { DrawLine(DA.X, DA.Y, DB.X, DB.Y, DashColor, 2.5f); }
				}
				const FVector DDest = Project(HexCellWorld(View.DashCell, Origin, HexSize, LayerH));
				if (DDest.Z > 0.f) { DrawRect(FLinearColor(DashColor.R, DashColor.G, DashColor.B, 0.4f), DDest.X - 10.f, DDest.Y - 10.f, 20.f, 20.f); }
			}

			// Linea verso il bersaglio d'attacco pianificato (dalla CELLA del bersaglio, non dal suo Actor).
			if (View.bHasTarget && HeadScreen.Z > 0.f)
			{
				const FVector TgtScreen = Project(HexCellWorld(View.TargetCell, Origin, HexSize, LayerH) + FVector(0.f, 0.f, WorldHeadOffset));
				if (TgtScreen.Z > 0.f)
				{
					DrawLine(HeadScreen.X, HeadScreen.Y, TgtScreen.X, TgtScreen.Y, Color, 2.f);
				}
			}
		}
	}

	// Barra di stato in alto: turno, fase e timer/avanzamento.
	if (TurnManager)
	{
		// Contatore del turno: con un formato in vigore mostra anche il limite, altrimenti resta il solo
		// numero — un "su 0" direbbe che la partita e' gia' scaduta, che non e' cio' che accade.
		const int32 RoundLimit = TurnManager->GetMatchRules().RoundLimit;
		const FString TurnCounter = RoundLimit > 0
			? FString::Printf(TEXT("Turno %d/%d"), TurnManager->GetTurnNumber(), RoundLimit)
			: FString::Printf(TEXT("Turno %d"), TurnManager->GetTurnNumber());

		FString Status;
		if (TurnManager->IsResolving())
		{
			// Durante il playback: fase in riproduzione + avanzamento + come saltare.
			const int32 Pct = FMath::RoundToInt(TurnManager->GetPlaybackProgress01() * 100.f);
			Status = FString::Printf(TEXT("%s  -  Risoluzione: %s  [%d%%]  (Spazio: salta)"),
				*TurnCounter, *TurnManager->GetPlaybackPhaseName(), Pct);
		}
		else
		{
			const TCHAR* PhaseName = TEXT("");
			switch (TurnManager->GetPhase())
			{
			case ERTMatchPhase::Planning:   PhaseName = TEXT("Pianificazione"); break;
			case ERTMatchPhase::MatchEnded: PhaseName = TEXT("Fine"); break;
			default:                        PhaseName = TEXT("Risoluzione"); break;
			}
			Status = FString::Printf(TEXT("%s  -  %s"), *TurnCounter, PhaseName);
			const float Remaining = TurnManager->GetPlanningTimeRemaining();
			if (TurnManager->GetPhase() == ERTMatchPhase::Planning && Remaining > 0.f)
			{
				Status += FString::Printf(TEXT("  -  %.0fs"), FMath::CeilToFloat(Remaining));
			}
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
				const URTActionData* Ability = Sel->GetAbility(A);
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

	// Esito, VIA che l'ha determinato e istruzione di riavvio a partita conclusa (CP 10.3). "Vince il team 0"
	// da solo non distingue un'eliminazione da un punto di vantaggio allo scadere dei round.
	if (TurnManager && TurnManager->GetPhase() == ERTMatchPhase::MatchEnded)
	{
		const FRTMatchResult Result = TurnManager->GetMatchResult();
		const FString Headline = FString::Printf(TEXT("%s - %s"),
			*URTTurnRules::DescribeOutcome(Result.Outcome),
			*URTTurnRules::DescribeEndReason(Result.Reason));

		float TW = 0.f, TH = 0.f;
		GetTextSize(Headline, TW, TH, nullptr, 2.f);
		DrawText(Headline, FLinearColor::White, (Canvas->SizeX - TW) * 0.5f, Canvas->SizeY * 0.4f, nullptr, 2.f);

		const FString Restart = TEXT("premi R per rigiocare");
		float RW = 0.f, RH = 0.f;
		GetTextSize(Restart, RW, RH, nullptr, 1.2f);
		DrawText(Restart, FLinearColor(0.85f, 0.85f, 0.85f, 1.f),
			(Canvas->SizeX - RW) * 0.5f, Canvas->SizeY * 0.4f + TH + 8.f, nullptr, 1.2f);
	}
}
