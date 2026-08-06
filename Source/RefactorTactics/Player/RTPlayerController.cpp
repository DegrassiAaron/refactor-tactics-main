#include "Player/RTPlayerController.h"
#include "Camera/RTCameraPawn.h"
#include "Selection/RTSelectable.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "Pathfinding/RTHexPathLibrary.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Unit/RTUnit.h"
#include "Ability/RTActionData.h"
#include "Combat/RTCombatLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Core/RTTypes.h"
#include "RefactorTactics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** Mappa esagonale del livello + contesto geometrico (unica fonte di scala). nullptr = livello senza mappa. */
	ARTHexMapActor* HexMapWithContext(const UWorld* World, FVector& OutOrigin, float& OutHexSize,
		float& OutLayerHeight, const URTHexMapAsset*& OutAsset)
	{
		OutAsset = nullptr;
		ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(World);
		if (HexMap)
		{
			OutAsset = HexMap->GetHexContext(OutOrigin, OutHexSize, OutLayerHeight);
		}
		return HexMap;
	}

	/**
	 * Snapshot dello stato corrente chiesto all'AUTORITA' (il TurnManager) e indice dell'unita' al suo interno.
	 * Il client non si costruisce uno stato parallelo: calcola solo la preview su quello autorevole (invariante #5).
	 * False se manca il turn manager o l'unita' non e' nello snapshot (es. non viva).
	 */
	bool PlanningSnapshotFor(const UObject* WorldContext, const ARTUnit* Unit,
		FRTHexSnapshot& OutSnapshot, int32& OutUnitId)
	{
		OutUnitId = INDEX_NONE;
		ARTTurnManager* TurnManager = Cast<ARTTurnManager>(
			UGameplayStatics::GetActorOfClass(const_cast<UObject*>(WorldContext), ARTTurnManager::StaticClass()));
		if (!TurnManager || !Unit)
		{
			return false;
		}
		TArray<ARTUnit*> Units;
		OutSnapshot = TurnManager->MakeCurrentSnapshot(Units);
		// L'UnitId e' l'INDICE nell'array delle unita' vive: va ricalcolato a ogni interazione, non memorizzato.
		OutUnitId = Units.IndexOfByKey(const_cast<ARTUnit*>(Unit));
		return OutUnitId != INDEX_NONE;
	}

	/** Testo del motivo di rifiuto di un waypoint, dallo stato del pathfinding (per il log). */
	const TCHAR* RejectReason(ERTHexPathStatus Status)
	{
		switch (Status)
		{
		case ERTHexPathStatus::GoalInvalid:  return TEXT("cella fuori dalla mappa");
		case ERTHexPathStatus::StartInvalid: return TEXT("unita' non presente nello snapshot");
		case ERTHexPathStatus::NodeLimit:    return TEXT("ricerca interrotta (limite nodi)");
		case ERTHexPathStatus::NoPath:
		default:                             return TEXT("oltre il budget, bloccata o occupata");
		}
	}
}

void ARTPlayerController::BuildInputMappings()
{
	if (MappingContext)
	{
		return; // gia' costruito
	}

	PanAction = NewObject<UInputAction>(this, TEXT("IA_Pan"));
	PanAction->ValueType = EInputActionValueType::Axis2D;

	ZoomAction = NewObject<UInputAction>(this, TEXT("IA_Zoom"));
	ZoomAction->ValueType = EInputActionValueType::Axis1D;

	SelectAction = NewObject<UInputAction>(this, TEXT("IA_Select"));
	SelectAction->ValueType = EInputActionValueType::Boolean;

	LockInAction = NewObject<UInputAction>(this, TEXT("IA_LockIn"));
	LockInAction->ValueType = EInputActionValueType::Boolean;

	RestartAction = NewObject<UInputAction>(this, TEXT("IA_Restart"));
	RestartAction->ValueType = EInputActionValueType::Boolean;

	Ability1Action = NewObject<UInputAction>(this, TEXT("IA_Ability1"));
	Ability1Action->ValueType = EInputActionValueType::Boolean;
	Ability2Action = NewObject<UInputAction>(this, TEXT("IA_Ability2"));
	Ability2Action->ValueType = EInputActionValueType::Boolean;
	Ability3Action = NewObject<UInputAction>(this, TEXT("IA_Ability3"));
	Ability3Action->ValueType = EInputActionValueType::Boolean;
	Ability4Action = NewObject<UInputAction>(this, TEXT("IA_Ability4"));
	Ability4Action->ValueType = EInputActionValueType::Boolean;

	UndoAction = NewObject<UInputAction>(this, TEXT("IA_UndoWaypoint"));
	UndoAction->ValueType = EInputActionValueType::Boolean;

	RecenterAction = NewObject<UInputAction>(this, TEXT("IA_Recenter"));
	RecenterAction->ValueType = EInputActionValueType::Boolean;

	FocusAction = NewObject<UInputAction>(this, TEXT("IA_FocusSelected"));
	FocusAction->ValueType = EInputActionValueType::Boolean;

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Tactical"));

	// Pan (Axis2D): D=+X, A=-X, W=+Y (Swizzle YXZ), S=-Y (Swizzle YXZ + Negate).
	MappingContext->MapKey(PanAction, EKeys::D);
	{
		FEnhancedActionKeyMapping& M = MappingContext->MapKey(PanAction, EKeys::A);
		M.Modifiers.Add(NewObject<UInputModifierNegate>(this));
	}
	{
		FEnhancedActionKeyMapping& M = MappingContext->MapKey(PanAction, EKeys::W);
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(this);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		M.Modifiers.Add(Swizzle);
	}
	{
		FEnhancedActionKeyMapping& M = MappingContext->MapKey(PanAction, EKeys::S);
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(this);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		M.Modifiers.Add(Swizzle);
		M.Modifiers.Add(NewObject<UInputModifierNegate>(this));
	}

	// Zoom (Axis1D): rotellina del mouse.
	MappingContext->MapKey(ZoomAction, EKeys::MouseWheelAxis);

	// Select (Boolean): tasto sinistro del mouse.
	MappingContext->MapKey(SelectAction, EKeys::LeftMouseButton);

	// Lock-in (Boolean): barra spaziatrice.
	MappingContext->MapKey(LockInAction, EKeys::SpaceBar);

	// Riavvio partita (Boolean): tasto R (attivo solo a match concluso).
	MappingContext->MapKey(RestartAction, EKeys::R);

	// Selezione abilita' (Boolean): tasti 1/2/3.
	MappingContext->MapKey(Ability1Action, EKeys::One);
	MappingContext->MapKey(Ability2Action, EKeys::Two);
	MappingContext->MapKey(Ability3Action, EKeys::Three);
	MappingContext->MapKey(Ability4Action, EKeys::Four); // scatto

	// Annulla l'ultimo waypoint della path composita (tasto destro del mouse o Backspace).
	MappingContext->MapKey(UndoAction, EKeys::RightMouseButton);
	MappingContext->MapKey(UndoAction, EKeys::BackSpace);

	// Ricentra la camera sul centro griglia + reset zoom (tasto Home).
	MappingContext->MapKey(RecenterAction, EKeys::Home);
	MappingContext->MapKey(FocusAction, EKeys::F);
}

void ARTPlayerController::BeginPlay()
{
	Super::BeginPlay();
	bShowMouseCursor = true;

	BuildInputMappings();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(MappingContext, 0);
	}
}

void ARTPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	BuildInputMappings();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EIC->BindAction(PanAction, ETriggerEvent::Triggered, this, &ARTPlayerController::OnPan);
		EIC->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &ARTPlayerController::OnZoom);
		EIC->BindAction(SelectAction, ETriggerEvent::Started, this, &ARTPlayerController::OnSelect);
		EIC->BindAction(LockInAction, ETriggerEvent::Started, this, &ARTPlayerController::OnLockIn);
		EIC->BindAction(RestartAction, ETriggerEvent::Started, this, &ARTPlayerController::OnRestart);
		EIC->BindAction(Ability1Action, ETriggerEvent::Started, this, &ARTPlayerController::OnAbility1);
		EIC->BindAction(Ability2Action, ETriggerEvent::Started, this, &ARTPlayerController::OnAbility2);
		EIC->BindAction(Ability3Action, ETriggerEvent::Started, this, &ARTPlayerController::OnAbility3);
		EIC->BindAction(Ability4Action, ETriggerEvent::Started, this, &ARTPlayerController::OnAbility4);
		EIC->BindAction(UndoAction, ETriggerEvent::Started, this, &ARTPlayerController::OnUndoWaypoint);
		EIC->BindAction(RecenterAction, ETriggerEvent::Started, this, &ARTPlayerController::OnRecenter);
		EIC->BindAction(FocusAction, ETriggerEvent::Started, this, &ARTPlayerController::OnFocusSelected);
	}
	else
	{
		UE_LOG(LogRT, Error, TEXT("[RT] InputComponent non e' un UEnhancedInputComponent"));
	}
}

void ARTPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Evidenzia la cella sotto il cursore (solo presentazione: non tocca la logica).
	FVector Origin; float HexSize; float LayerH; const URTHexMapAsset* Map = nullptr;
	ARTHexMapActor* HexMap = HexMapWithContext(GetWorld(), Origin, HexSize, LayerH, Map);
	if (!HexMap)
	{
		return;
	}
	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, /*bTraceComplex=*/ false, Hit) && Hit.GetActor())
	{
		// Il layer viene dalla QUOTA del punto colpito: cliccando il ponte si evidenzia la cella del ponte
		// (in editor lo decide invece ActiveLayer, perche' li' si dipinge su un piano scelto).
		const FRTCellId Cell = URTHexLibrary::WorldToCellId(Hit.Location, Origin, HexSize, LayerH);
		HexMap->SetHoveredCell(Cell, Map && Map->ContainsCell(Cell));
	}
	else
	{
		HexMap->SetHoveredCell(FRTCellId(), false);
	}
}

void ARTPlayerController::OnRecenter(const FInputActionValue& Value)
{
	if (ARTCameraPawn* Cam = Cast<ARTCameraPawn>(GetPawn()))
	{
		Cam->RecenterView();
	}
}

void ARTPlayerController::OnFocusSelected(const FInputActionValue& Value)
{
	ARTUnit* Unit = GetSelectedUnit();
	if (!Unit)
	{
		UE_LOG(LogRT, Log, TEXT("[RT] Focus: nessuna unita' selezionata (Home ricentra sulla griglia)."));
		return;
	}
	if (ARTCameraPawn* Cam = Cast<ARTCameraPawn>(GetPawn()))
	{
		Cam->FocusOn(Unit->GetActorLocation());
		UE_LOG(LogRT, Log, TEXT("[RT] Focus su %s"), *Unit->GetName());
	}
}

void ARTPlayerController::OnPan(const FInputActionValue& Value)
{
	if (ARTCameraPawn* Cam = Cast<ARTCameraPawn>(GetPawn()))
	{
		Cam->AddPlanarMovement(Value.Get<FVector2D>());
	}
}

void ARTPlayerController::OnZoom(const FInputActionValue& Value)
{
	if (ARTCameraPawn* Cam = Cast<ARTCameraPawn>(GetPawn()))
	{
		Cam->AddZoom(Value.Get<float>());
	}
}

namespace
{
	/** TurnManager del livello, o nullptr. La telemetria non deve mai far crashare l'input. */
	ARTTurnManager* PacingTurnManager(const UObject* WorldContext)
	{
		return Cast<ARTTurnManager>(
			UGameplayStatics::GetActorOfClass(WorldContext, ARTTurnManager::StaticClass()));
	}
}

void ARTPlayerController::OnSelect(const FInputActionValue& Value)
{
	// Attivita' generica: aggiorna i tempi anche quando il click non produce nulla. Un click a vuoto
	// e' comunque il giocatore che sta lavorando, e serve a non scambiarlo per un giocatore assente.
	if (ARTTurnManager* TM = PacingTurnManager(this))
	{
		TM->RecordPlanningInput(ERTPlanningInput::Click);
	}

	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECC_Visibility, /*bTraceComplex=*/ false, Hit) || !Hit.GetActor())
	{
		return;
	}

	AActor* HitActor = Hit.GetActor();
	ARTUnit* ClickedUnit = Cast<ARTUnit>(HitActor);
	ARTUnit* SelectedUnit = Cast<ARTUnit>(SelectedActor);

	// Guardia di autorita': si pianifica solo per le proprie unita'. Se per qualche via SelectedActor fosse
	// un'unita' avversaria, la deselezioniamo invece di prenderne il comando.
	if (SelectedUnit && !URTCombatLibrary::CanPlayerControlUnit(SelectedUnit->TeamId, PlayerTeamId))
	{
		if (IRTSelectable* PreviousSel = Cast<IRTSelectable>(SelectedActor))
		{
			PreviousSel->OnDeselected();
		}
		SelectedActor = nullptr;
		SelectedUnit = nullptr;
	}

	// Click su un'unita' nemica, con una nostra unita' selezionata -> pianifica l'abilita' attiva.
	if (ClickedUnit && SelectedUnit && ClickedUnit != SelectedUnit && ClickedUnit->TeamId != SelectedUnit->TeamId)
	{
		const int32 AbilityIndex = SelectedUnit->SelectedAbilityIndex;
		const URTActionData* Ability = SelectedUnit->GetAbility(AbilityIndex);
		if (Ability && Ability->bDash)
		{
			UE_LOG(LogRT, Log, TEXT("[RT] Lo scatto si pianifica su una CELLA, non su un nemico"));
			return;
		}
		if (!Ability)
		{
			return;
		}
		FVector TOrigin; float THexSize; float TLayerH; const URTHexMapAsset* TMap = nullptr;
		HexMapWithContext(GetWorld(), TOrigin, THexSize, TLayerH, TMap);
		// Un solo gate (FAIL-CLOSED: senza mappa autorevole non si ingaggia — test
		// Combat.HexTargetingIsFailClosed) che dichiara anche il MOTIVO, cosi' il log non attribuisce alla
		// copertura un bersaglio che era solo troppo lontano (test
		// Combat.HexTargetingReasonDistinguishesRangeFromCover).
		const bool bReady = SelectedUnit->CanUseAbility(AbilityIndex);
		const ERTHexTargetReason Reason = URTCombatLibrary::ClassifyHexTargeting(
			TMap, SelectedUnit->Cell, ClickedUnit->Cell, Ability->RangeCells);

		if (bReady && Reason == ERTHexTargetReason::Ok)
		{
			SelectedUnit->PlannedAbilityIndex = AbilityIndex;
			SelectedUnit->PlannedAttackTarget = ClickedUnit;
			if (ARTTurnManager* TM = PacingTurnManager(this))
			{
				TM->RecordPlanningInput(ERTPlanningInput::Order);
			}
			UE_LOG(LogRT, Log, TEXT("[RT] Piano: %s usa %s su %s"), *SelectedUnit->GetName(), *Ability->DisplayName.ToString(), *ClickedUnit->GetName());
		}
		else if (!bReady)
		{
			UE_LOG(LogRT, Log, TEXT("[RT] %s non pronta (ricarica/energia)"), *Ability->DisplayName.ToString());
		}
		else
		{
			switch (Reason)
			{
			case ERTHexTargetReason::OutOfRange:
				UE_LOG(LogRT, Log, TEXT("[RT] %s fuori portata (max %d)"), *ClickedUnit->GetName(), Ability->RangeCells);
				break;
			case ERTHexTargetReason::NoLineOfSight:
				UE_LOG(LogRT, Log, TEXT("[RT] %s coperto (nessuna linea di tiro)"), *ClickedUnit->GetName());
				break;
			case ERTHexTargetReason::NoMap:
			default:
				UE_LOG(LogRT, Warning,
					TEXT("[RT] Nessuna mappa esagonale: bersagliamento non validabile, piano rifiutato"));
				break;
			}
		}
		return;
	}

	// Click su un'unita' AVVERSARIA senza nulla di selezionato: non e' nostra, non la si comanda. Senza questa
	// guardia resterebbe "selezionata" e ogni click successivo su una nostra unita' finirebbe nel ramo di
	// pianificazione dell'attacco qui sopra, rendendo le proprie unita' inselezionabili.
	if (ClickedUnit && !URTCombatLibrary::CanPlayerControlUnit(ClickedUnit->TeamId, PlayerTeamId))
	{
		UE_LOG(LogRT, Log, TEXT("[RT] %s e' avversaria: seleziona prima una tua unita' per bersagliarla"),
			*ClickedUnit->GetName());
		return;
	}

	// Click su un'unita' (o altro selezionabile) -> selezione.
	if (IRTSelectable* Selectable = Cast<IRTSelectable>(HitActor))
	{
		if (HitActor != SelectedActor)
		{
			if (IRTSelectable* Previous = Cast<IRTSelectable>(SelectedActor))
			{
				Previous->OnDeselected();
			}
			Selectable->OnSelected();
			if (ARTTurnManager* TM = PacingTurnManager(this))
			{
				TM->RecordPlanningInput(ERTPlanningInput::Selection);
			}
			SelectedActor = HitActor;
			UE_LOG(LogRT, Log, TEXT("[RT] Selezionata: %s"), *HitActor->GetName());

			// L'anteprima segue la selezione: mostra il piano dell'unita' scelta (vuoto se non ne ha).
			FVector SOrigin; float SHexSize; float SLayerH; const URTHexMapAsset* SMap = nullptr;
			if (ARTHexMapActor* SHexMap = HexMapWithContext(GetWorld(), SOrigin, SHexSize, SLayerH, SMap))
			{
				const ARTUnit* NewUnit = Cast<ARTUnit>(HitActor);
				SHexMap->SetPreviewPath(NewUnit ? NewUnit->PlannedPath : TArray<FRTCellId>());
			}
		}
		return;
	}

	// Click sulla mappa: la CELLA si ricava dalla quota del punto colpito (ponte vs terra), poi decide
	// HandleClickOnCell — che e' guidabile dai test senza viewport.
	{
		FVector HitOrigin; float HitHexSize; float HitLayerH; const URTHexMapAsset* HitMap = nullptr;
		if (HexMapWithContext(GetWorld(), HitOrigin, HitHexSize, HitLayerH, HitMap))
		{
			HandleClickOnCell(URTHexLibrary::WorldToCellId(Hit.Location, HitOrigin, HitHexSize, HitLayerH));
		}
		else
		{
			UE_LOG(LogRT, Warning, TEXT("[RT] Nessuna mappa esagonale nel livello: pianificazione non disponibile"));
		}
	}
}

void ARTPlayerController::HandleClickOnCell(const FRTCellId& Cell)
{
	ARTUnit* SelectedUnit = GetSelectedUnit();
	if (!SelectedUnit)
	{
		return;
	}

	FVector Origin; float HexSize; float LayerH; const URTHexMapAsset* Map = nullptr;
	ARTHexMapActor* HexMap = HexMapWithContext(GetWorld(), Origin, HexSize, LayerH, Map);
	if (!HexMap || !Map)
	{
		UE_LOG(LogRT, Warning, TEXT("[RT] Nessuna mappa esagonale nel livello: pianificazione non disponibile"));
		return;
	}
	if (!Map->ContainsCell(Cell))
	{
		return; // click fuori dalla mappa: nessun piano, nessun rumore nel log
	}

	// Stato autorevole per la validazione: lo fornisce il TurnManager, il client non se lo ricostruisce.
	FRTHexSnapshot Snapshot;
	int32 UnitId = INDEX_NONE;
	if (!PlanningSnapshotFor(this, SelectedUnit, Snapshot, UnitId))
	{
		UE_LOG(LogRT, Warning, TEXT("[RT] %s non e' nello snapshot: pianificazione rifiutata"), *SelectedUnit->GetName());
		return;
	}

	// Se l'abilita' selezionata e' uno SCATTO, si pianifica un DASH verso questa cella (fase Dash),
	// invece di un waypoint di movimento normale.
	const int32 SelIdx = SelectedUnit->SelectedAbilityIndex;
	const URTActionData* SelAb = SelectedUnit->GetAbility(SelIdx);
	if (SelAb && SelAb->bDash)
	{
		if (!SelectedUnit->CanUseAbility(SelIdx))
		{
			UE_LOG(LogRT, Log, TEXT("[RT] Scatto non pronto (ricarica) per %s"), *SelectedUnit->GetName());
			return;
		}
		const int32 DashRange = SelectedUnit->GetEffectiveDashRange(SelAb->RangeCells);
		if (DashRange <= 0)
		{
			// MaxCost == 0 significa "illimitato" per l'A*: il budget nullo va intercettato prima.
			UE_LOG(LogRT, Log, TEXT("[RT] Scatto senza portata utile per %s"), *SelectedUnit->GetName());
			return;
		}
		const TSet<FRTCellId> Occupied = [&Snapshot, UnitId]()
		{
			TSet<FRTCellId> Set;
			for (const TPair<FRTCellId, int32>& Pair : Snapshot.Occupancy)
			{
				if (Pair.Value != UnitId) { Set.Add(Pair.Key); }
			}
			return Set;
		}();
		const FRTHexPathResult DashPath =
			URTHexPathLibrary::FindPathAvoiding(Map, SelectedUnit->Cell, Cell, &Occupied, DashRange);
		if (DashPath.Status != ERTHexPathStatus::Success)
		{
			UE_LOG(LogRT, Log, TEXT("[RT] Cella (%d,%d,L%d) fuori dallo scatto (%s, max %d) per %s"),
				Cell.X, Cell.Y, Cell.Layer, RejectReason(DashPath.Status), DashRange, *SelectedUnit->GetName());
			return;
		}
		SelectedUnit->PlannedDashAbility = SelIdx;
		SelectedUnit->PlannedDashCell = Cell;
		UE_LOG(LogRT, Log, TEXT("[RT] Piano: %s SCATTO -> (%d,%d,L%d) costo %d"),
			*SelectedUnit->GetName(), Cell.X, Cell.Y, Cell.Layer, DashPath.TotalCost);
		return;
	}

	// Movimento normale: il waypoint si aggiunge IN PROVA e si tiene solo se l'intero percorso resta valido.
	SelectedUnit->PlannedWaypoints.Add(Cell);
	const FRTHexPathResult Composite =
		URTHexSimLibrary::BuildCompositeHexPath(Snapshot, UnitId, SelectedUnit->PlannedWaypoints);
	if (Composite.Status != ERTHexPathStatus::Success || Composite.Path.Num() < 2)
	{
		SelectedUnit->PlannedWaypoints.Pop(); // rifiutato: si torna al piano precedente, non a uno a meta'

		// Il motivo GIUSTO, non un elenco di tre: se la cella in se' va bene, il rifiuto e' questione di budget,
		// e allora si dice quanto era gia' speso. Test: HexSim.WaypointRejectionSaysWhich.
		const ERTHexWaypointReason CellReason =
			URTHexSimLibrary::ClassifyWaypointCell(Snapshot, UnitId, Cell);
		const int32 Budget = SelectedUnit->GetEffectiveMoveRange();
		switch (CellReason)
		{
		case ERTHexWaypointReason::NotOnMap:
			UE_LOG(LogRT, Log, TEXT("[RT] Waypoint (%d,%d,L%d) rifiutato: cella fuori dalla mappa (%s)"),
				Cell.X, Cell.Y, Cell.Layer, *SelectedUnit->GetName());
			break;
		case ERTHexWaypointReason::BlocksMovement:
			UE_LOG(LogRT, Log, TEXT("[RT] Waypoint (%d,%d,L%d) rifiutato: cella bloccata (%s)"),
				Cell.X, Cell.Y, Cell.Layer, *SelectedUnit->GetName());
			break;
		case ERTHexWaypointReason::Occupied:
			UE_LOG(LogRT, Log, TEXT("[RT] Waypoint (%d,%d,L%d) rifiutato: cella occupata da un'altra unita' (%s)"),
				Cell.X, Cell.Y, Cell.Layer, *SelectedUnit->GetName());
			break;
		case ERTHexWaypointReason::Ok:
		default:
			// La cella e' percorribile e libera: quel che manca sono punti movimento. Il percorso precedente
			// (quello ancora valido) dice quanto e' gia' impegnato.
			{
				const FRTHexPathResult Kept =
					URTHexSimLibrary::BuildCompositeHexPath(Snapshot, UnitId, SelectedUnit->PlannedWaypoints);
				UE_LOG(LogRT, Log,
					TEXT("[RT] Waypoint (%d,%d,L%d) rifiutato: oltre il budget (gia' spesi %d di %d) per %s"),
					Cell.X, Cell.Y, Cell.Layer, Kept.TotalCost, Budget, *SelectedUnit->GetName());
			}
			break;
		}
		return;
	}

	SelectedUnit->PlannedPath = Composite.Path;
	SelectedUnit->PlannedCell = Composite.Path.Last();
	HexMap->SetPreviewPath(Composite.Path);
	UE_LOG(LogRT, Log, TEXT("[RT] Piano: %s -> %d waypoint (costo %d/%d)"),
		*SelectedUnit->GetName(), SelectedUnit->PlannedWaypoints.Num(), Composite.TotalCost,
		SelectedUnit->GetEffectiveMoveRange());
}

void ARTPlayerController::OnLockIn(const FInputActionValue& Value)
{
	if (ARTTurnManager* TurnManager = Cast<ARTTurnManager>(UGameplayStatics::GetActorOfClass(this, ARTTurnManager::StaticClass())))
	{
		// Durante il playback lo stesso tasto (Spazio) salta la risoluzione; altrimenti chiude la pianificazione.
		if (TurnManager->IsResolving())
		{
			TurnManager->SkipPlayback();
		}
		else
		{
			TurnManager->LockInAndResolve();
		}
	}
}

void ARTPlayerController::OnRestart(const FInputActionValue& Value)
{
	// Riavvia la partita solo quando è conclusa: ricarica il livello corrente.
	const ARTTurnManager* TurnManager =
		Cast<ARTTurnManager>(UGameplayStatics::GetActorOfClass(this, ARTTurnManager::StaticClass()));
	if (TurnManager && TurnManager->GetPhase() == ERTMatchPhase::MatchEnded)
	{
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
	}
}

ARTUnit* ARTPlayerController::GetSelectedUnit() const
{
	return Cast<ARTUnit>(SelectedActor);
}

void ARTPlayerController::SelectAbilityForCurrent(int32 Index)
{
	ARTUnit* Unit = GetSelectedUnit();
	if (!Unit)
	{
		return;
	}
	Unit->SelectAbility(Index);
	// Qui e non "in fondo alla funzione": sotto ci sono due return anticipati e il ramo bSelfTarget,
	// quindi questo e' l'unico punto attraversato da ogni pressione di tasto che produca un effetto.
	if (ARTTurnManager* TM = PacingTurnManager(this))
	{
		TM->RecordPlanningInput(ERTPlanningInput::Order);
	}
	const URTActionData* Ability = Unit->GetAbility(Index);
	if (!Ability)
	{
		return;
	}

	if (Ability->bSelfTarget)
	{
		// Supporto: si pianifica immediatamente su se stessi (nessun bersaglio da cliccare).
		if (Unit->CanUseAbility(Index))
		{
			Unit->PlannedAbilityIndex = Index;
			Unit->PlannedAttackTarget = nullptr;
			UE_LOG(LogRT, Log, TEXT("[RT] %s pianifica %s (supporto)"), *Unit->GetName(), *Ability->DisplayName.ToString());
		}
		else
		{
			UE_LOG(LogRT, Log, TEXT("[RT] %s non pronta"), *Ability->DisplayName.ToString());
		}
	}
	else
	{
		UE_LOG(LogRT, Log, TEXT("[RT] %s: abilita' attiva -> %s"), *Unit->GetName(), *Ability->DisplayName.ToString());
	}
}

void ARTPlayerController::OnAbility1(const FInputActionValue& Value) { SelectAbilityForCurrent(0); }
void ARTPlayerController::OnAbility2(const FInputActionValue& Value) { SelectAbilityForCurrent(1); }
void ARTPlayerController::OnAbility3(const FInputActionValue& Value) { SelectAbilityForCurrent(2); }
void ARTPlayerController::OnAbility4(const FInputActionValue& Value) { SelectAbilityForCurrent(3); } // scatto

void ARTPlayerController::OnUndoWaypoint(const FInputActionValue& Value)
{
	ARTUnit* Unit = GetSelectedUnit();
	if (!Unit || Unit->PlannedWaypoints.Num() == 0)
	{
		return;
	}
	Unit->PlannedWaypoints.Pop(); // rimuove l'ultimo waypoint
	RebuildPlannedPath();
	if (ARTTurnManager* TM = PacingTurnManager(this))
	{
		TM->RecordPlanningInput(ERTPlanningInput::Undo);
	}
	UE_LOG(LogRT, Log, TEXT("[RT] Annullato waypoint: %s -> %d waypoint"), *Unit->GetName(), Unit->PlannedWaypoints.Num());
}

void ARTPlayerController::RebuildPlannedPath()
{
	ARTUnit* Unit = GetSelectedUnit();
	if (!Unit)
	{
		return;
	}
	FVector Origin; float HexSize; float LayerH; const URTHexMapAsset* Map = nullptr;
	ARTHexMapActor* HexMap = HexMapWithContext(GetWorld(), Origin, HexSize, LayerH, Map);

	FRTHexSnapshot Snapshot;
	int32 UnitId = INDEX_NONE;
	FRTHexPathResult Composite;
	if (Unit->PlannedWaypoints.Num() > 0 && PlanningSnapshotFor(this, Unit, Snapshot, UnitId))
	{
		Composite = URTHexSimLibrary::BuildCompositeHexPath(Snapshot, UnitId, Unit->PlannedWaypoints);
	}

	if (Composite.Status == ERTHexPathStatus::Success && Composite.Path.Num() >= 2)
	{
		Unit->PlannedPath = Composite.Path;
		Unit->PlannedCell = Composite.Path.Last();
	}
	else
	{
		// Nessun waypoint (o piano non piu' valido): si torna a "resto fermo".
		Unit->PlannedPath.Reset();
		Unit->PlannedCell = Unit->Cell;
	}

	if (HexMap)
	{
		HexMap->SetPreviewPath(Unit->PlannedPath);
	}
}
