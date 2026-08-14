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
#include "Ability/RTCatalogLibrary.h"
#include "Turn/RTMovementActionLibrary.h"
#include "Turn/RTFacingLibrary.h" // CP 11.8: la legalita' della rotazione si CHIEDE, non si riscrive qui
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
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
		FRTHexSnapshot& OutSnapshot, int32& OutUnitId, TArray<ARTUnit*>* OutUnits = nullptr)
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
		if (OutUnits)
		{
			// Gli stessi indici dello snapshot: servono a distinguere i NEMICI (che una carica colpisce) dagli
			// ostacoli (che la fermano). `FRTHexSimUnit` non porta la squadra, quindi la si legge dagli Actor.
			*OutUnits = MoveTemp(Units);
		}
		return OutUnitId != INDEX_NONE;
	}

	/**
	 * Aggiorna l'anteprima di pianificazione (SOLA PRESENTAZIONE) dallo stato dell'unita' selezionata:
	 * dove puo' arrivare e, se ha un attacco pianificato, quali celle colpirebbe — segnalando gli ALLEATI
	 * che finirebbero nell'area.
	 *
	 * Entrambi gli insiemi vengono dalle stesse funzioni che decidono l'esito (`ReachableCells`,
	 * `HexHitCells`): nessun calcolo parallelo, altrimenti il giocatore vedrebbe una zona e ne subirebbe
	 * un'altra. `Unit == nullptr` (deselezione, fine pianificazione) spegne l'anteprima.
	 */
	void RefreshPlanningPreview(const UWorld* World, const ARTUnit* Unit)
	{
		FVector Origin; float HexSize; float LayerH; const URTHexMapAsset* Map = nullptr;
		ARTHexMapActor* HexMap = HexMapWithContext(World, Origin, HexSize, LayerH, Map);
		if (!HexMap)
		{
			return;
		}
		if (!Unit)
		{
			HexMap->SetPreviewReachableCells(TArray<FRTCellId>());
			HexMap->SetPreviewHitCells(TArray<FRTCellId>(), TArray<FRTCellId>());
			return;
		}

		// Dove puo' arrivare: budget, blocchi, occupanti e archi sono gia' applicati da ReachableCells.
		FRTHexSnapshot Snapshot;
		int32 UnitId = INDEX_NONE;
		TArray<ARTUnit*> Units;
		TArray<FRTCellId> Reachable;
		if (PlanningSnapshotFor(World, Unit, Snapshot, UnitId, &Units))
		{
			for (const FRTHexReachableCell& R : URTHexSimLibrary::ReachableCells(Snapshot, UnitId))
			{
				Reachable.Add(R.Cell);
			}
		}
		HexMap->SetPreviewReachableCells(Reachable);

		// Chi colpirebbe: solo se c'e' davvero un attacco pianificato su un bersaglio vivo.
		TArray<FRTCellId> Hit;
		TArray<FRTCellId> Allies;
		const URTActionData* Ability = Unit->GetAbility(Unit->PlannedAbilityIndex);
		const ARTUnit* Target = Unit->PlannedAttackTarget;
		if (Ability && Target && Target->IsAlive())
		{
			Hit = URTHexCombatLibrary::HexHitCells(Ability->Shape, Unit->Cell, Target->Cell,
				Ability->RangeCells, Ability->AreaRadius);

			// Fuoco amico: un alleato dentro l'area va visto PRIMA del lock-in, non dedotto dai danni dopo.
			//
			// Ma solo se l'azione puo' DAVVERO colpirlo. L'avviso nasceva dalla sola geometria, e quindi
			// compariva anche per abilita' con `bFriendlyFire` a false, dove l'alleato non subisce nulla: un
			// allarme su un evento impossibile insegna a ignorare gli allarmi. Oggi riguarda `CircularTide`,
			// che per limite dichiarato non tocca i propri (curerebbe con l'effetto sbagliato).
			if (Ability->Def.bFriendlyFire)
			{
				for (const ARTUnit* Other : Units)
				{
					if (Other && Other != Unit && Other->IsAlive() && Other->TeamId == Unit->TeamId
						&& Hit.Contains(Other->Cell))
					{
						Allies.AddUnique(Other->Cell);
					}
				}
			}
		}
		HexMap->SetPreviewHitCells(Hit, Allies);
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

	RotateAction = NewObject<UInputAction>(this, TEXT("IA_Rotate"));
	RotateAction->ValueType = EInputActionValueType::Axis1D;

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

	// Rotazione della vista: E = orario, Q = antiorario. Sono i tasti che ogni gioco tattico con camera
	// libera usa per questo, e stanno accanto a WASD: chi guida non deve spostare la mano.
	MappingContext->MapKey(RotateAction, EKeys::E);
	{
		FEnhancedActionKeyMapping& M = MappingContext->MapKey(RotateAction, EKeys::Q);
		M.Modifiers.Add(NewObject<UInputModifierNegate>(this));
	}

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
		// `Started` e non `Triggered`: la rotazione e' a SCATTI di `YawStep`, quindi deve avvenire una volta
		// per pressione. Con `Triggered` un tasto tenuto giu' avrebbe girato la vista di 45 gradi per frame.
		EIC->BindAction(RotateAction, ETriggerEvent::Started, this, &ARTPlayerController::OnRotate);
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
		// La CELLA, non l'attore: `ARTUnit` si posiziona con `VisualZOffset` (= `UnitHalfHeight`), quindi
		// `GetActorLocation()` sta mezzo corpo **sopra** il piano. Inquadrare li' porterebbe il pivot di
		// `F` a una quota che nessun'altra inquadratura usa — `Home` e l'avvio partita stanno sul piano —
		// e le due divergerebbero di una costante.
		// ⚠️ La prima stesura di #887 passava `GetActorLocation()` e giustificava il fix dicendo che il
		// controller «passa l'unita' sul terreno»: falso, e trovato in code review.
		if (const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(GetWorld()))
		{
			FVector Origin; float HexSize; float LayerHeight;
			HexMap->GetHexContext(Origin, HexSize, LayerHeight);
			Cam->FocusOn(URTHexLibrary::AxialToWorld(Unit->Cell, Origin, HexSize, LayerHeight));
		}
		else
		{
			// Senza mappa non c'e' un piano a cui ancorarsi: si ripiega sull'attore, che e' comunque
			// meglio di non inquadrare.
			Cam->FocusOn(Unit->GetActorLocation());
		}
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

void ARTPlayerController::OnRotate(const FInputActionValue& Value)
{
	if (ARTCameraPawn* Cam = Cast<ARTCameraPawn>(GetPawn()))
	{
		Cam->AddYaw(Value.Get<float>());
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
		HandleClickOnUnit(ClickedUnit);
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
		SelectUnit(HitActor);
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

void ARTPlayerController::SelectUnit(AActor* Actor, bool bRecordAsPlayerInput)
{
	// Estratta da `OnSelect` — non duplicata: la selezione ha effetti collaterali (evidenziazione, anteprima,
	// telemetria) e averne due copie significa che una smettera' di essere aggiornata.
	//
	// Serve anche fuori dal clic: uno scenario che vuole mostrare l'anteprima deve selezionare come selezione
	// il giocatore, non impostando `SelectedActor` di nascosto. Il primo tentativo passava da
	// `HandleClickOnUnit`, che NON seleziona — presuppone una selezione e tratta l'argomento come BERSAGLIO —
	// quindi usciva subito senza fare nulla, e a schermo non compariva niente.
	if (Actor == SelectedActor)
	{
		return;
	}
	IRTSelectable* Selectable = Cast<IRTSelectable>(Actor);
	if (!Selectable)
	{
		return;
	}

	if (IRTSelectable* Previous = Cast<IRTSelectable>(SelectedActor))
	{
		Previous->OnDeselected();
	}
	Selectable->OnSelected();

	// La telemetria di ritmo misura quanto impiega un GIOCATORE a decidere: una selezione fatta da uno
	// scenario non e' una decisione, e contarla falserebbe i numeri di `PIE-V01-MATCHLEN`.
	if (bRecordAsPlayerInput)
	{
		if (ARTTurnManager* TM = PacingTurnManager(this))
		{
			TM->RecordPlanningInput(ERTPlanningInput::Selection);
		}
	}
	SelectedActor = Actor;
	UE_LOG(LogRT, Log, TEXT("[RT] Selezionata: %s"), *Actor->GetName());

	// L'anteprima segue la selezione: mostra il piano dell'unita' scelta (vuoto se non ne ha).
	FVector SOrigin; float SHexSize; float SLayerH; const URTHexMapAsset* SMap = nullptr;
	if (ARTHexMapActor* SHexMap = HexMapWithContext(GetWorld(), SOrigin, SHexSize, SLayerH, SMap))
	{
		const ARTUnit* NewUnit = Cast<ARTUnit>(Actor);
		SHexMap->SetPreviewPath(NewUnit ? NewUnit->PlannedPath : TArray<FRTCellId>());
		// Con il piano arrivano anche le zone: dove puo' arrivare e, se ha gia' un bersaglio, chi colpisce.
		RefreshPlanningPreview(GetWorld(), NewUnit);
	}
}

void ARTPlayerController::HandleClickOnUnit(ARTUnit* ClickedUnit)
{
	ARTUnit* SelectedUnit = Cast<ARTUnit>(SelectedActor);
	if (!ClickedUnit || !SelectedUnit) { return; }

	{
		const int32 AbilityIndex = SelectedUnit->SelectedAbilityIndex;
		const URTActionData* Ability = SelectedUnit->GetAbility(AbilityIndex);

		// Una CARICA si pianifica proprio cliccando il nemico: e' l'azione che punta LUI e si ferma addosso a
		// lui. Prima di #145 l'unico modo era cliccare una cella oltre il bersaglio e sperare che la
		// traiettoria lo incontrasse — un'affordance che nessuno indovina.
		if (Ability && Ability->Def.MovementStyle == ERTMovementStyle::LinearCharge)
		{
			HandleClickOnCell(ClickedUnit->Cell);
			return;
		}

		// Ogni altra mobilita' rapida si ferma DAVANTI alle unita': puntarne una non ha senso, e il rifiuto
		// dice perche' invece di non fare niente.
		if (Ability && URTCatalogLibrary::IsFastMovement(Ability->Def))
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
			// La zona colpita compare SUBITO, col fuoco amico gia' segnalato: e' il momento in cui il giocatore
			// puo' ancora cambiare idea. Dopo il lock-in l'informazione non serve piu' a niente.
			RefreshPlanningPreview(GetWorld(), SelectedUnit);
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
	TArray<ARTUnit*> SnapshotUnits;
	if (!PlanningSnapshotFor(this, SelectedUnit, Snapshot, UnitId, &SnapshotUnits))
	{
		UE_LOG(LogRT, Warning, TEXT("[RT] %s non e' nello snapshot: pianificazione rifiutata"), *SelectedUnit->GetName());
		return;
	}

	// Se l'abilita' selezionata e' uno SCATTO, si pianifica un DASH verso questa cella (fase Dash),
	// invece di un waypoint di movimento normale.
	const int32 SelIdx = SelectedUnit->SelectedAbilityIndex;
	const URTActionData* SelAb = SelectedUnit->GetAbility(SelIdx);
	if (SelAb && URTCatalogLibrary::IsFastMovement(SelAb->Def))
	{
		if (!SelectedUnit->CanUseAbility(SelIdx))
		{
			UE_LOG(LogRT, Log, TEXT("[RT] Scatto non pronto (ricarica) per %s"), *SelectedUnit->GetName());
			return;
		}
		// Portata letta come la legge ResolveDash: dal CATALOGO se l'azione ne fa parte, altrimenti dal campo
		// legacy dell'asset. Leggere un numero diverso da quello del resolver significa accettare piani che
		// poi non si eseguono (o negarne di buoni).
		const int32 DeclaredRange = SelAb->Def.ActionId.IsNone() ? SelAb->RangeCells : SelAb->Def.RangeCells;
		const int32 DashRange = SelectedUnit->GetEffectiveDashRange(DeclaredRange);
		if (DashRange <= 0)
		{
			// MaxCost == 0 significa "illimitato" per l'A*: il budget nullo va intercettato prima.
			UE_LOG(LogRT, Log, TEXT("[RT] Scatto senza portata utile per %s"), *SelectedUnit->GetName());
			return;
		}

		if (URTMovementActionLibrary::IsLinear(SelAb->Def.MovementStyle))
		{
			// Mobilita' LINEARE: la si valida con lo STESSO codice che la eseguira'. Con l'A* il giocatore
			// potrebbe cliccare una cella raggiungibile solo aggirando un ostacolo: il piano verrebbe
			// accettato, la fase Dash non muoverebbe nulla e il turno si perderebbe in silenzio.
			TSet<int32> Hostiles;
			for (int32 i = 0; i < SnapshotUnits.Num(); ++i)
			{
				const ARTUnit* Other = SnapshotUnits[i];
				if (Other && Other->IsAlive() && Other->TeamId != SelectedUnit->TeamId) { Hostiles.Add(i); }
			}

			const FRTLinearMoveResult Linear = URTMovementActionLibrary::ResolveLinearMove(
				Map, SelectedUnit->Cell, Cell, DashRange, SelAb->Def.MovementStyle, Snapshot.Occupancy, Hostiles);

			// Regola di CP 4.5, gia' fissata da `HexSim.DashIsLinear`: o si arriva sulla cella RICHIESTA, o lo
			// scatto non si pianifica. Niente scatto a meta' verso una cella che il giocatore non ha scelto —
			// stessa disciplina dei waypoint compositi. (Il `Fallback.Stop` del catalogo resta: vale in
			// RISOLUZIONE, quando il movimento simultaneo altrui chiude una traiettoria che era libera qui.)
			//
			// L'unica eccezione e' la CARICA: fermarsi addosso al nemico E' il suo modo di arrivare, e senza
			// questa riga un bersaglio adiacente renderebbe la carica impianificabile.
			const bool bCharges = (Linear.Stop == ERTLinearStop::Impact);
			if (Linear.Final != Cell && !bCharges)
			{
				UE_LOG(LogRT, Log, TEXT("[RT] Cella (%d,%d,L%d) non e' raggiungibile in LINEA (%s, max %d) per %s"),
					Cell.X, Cell.Y, Cell.Layer,
					Linear.Stop == ERTLinearStop::NotAligned ? TEXT("non allineata o fuori portata") : TEXT("traiettoria bloccata"),
					DashRange, *SelectedUnit->GetName());
				return;
			}

			SelectedUnit->PlannedDashAbility = SelIdx;
			SelectedUnit->PlannedDashCell = Cell;
			UE_LOG(LogRT, Log, TEXT("[RT] Piano: %s SCATTO -> (%d,%d,L%d)%s"),
				*SelectedUnit->GetName(), Cell.X, Cell.Y, Cell.Layer,
				bCharges ? TEXT(" con impatto") : TEXT(""));
			return;
		}

		// Mobilita' a BUDGET (`Action.Sprint`): risolve col pathfinding, quindi si valida col pathfinding.
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
			// L'anteprima muore col lock-in: da qui in poi mostrerebbe una minaccia gia' risolta, e la traccia
			// del percorso la sostituisce `LastMoveRoutes` (cio' che e' DAVVERO successo, non cio' che si voleva).
			RefreshPlanningPreview(GetWorld(), nullptr);
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

	// REAZIONE (`#601`): slot proprio, e va dichiarata prima degli altri due rami. Senza questo ramo una
	// reazione selezionata finiva nello slot PRINCIPALE — dove il pass delle reazioni non la guarda mai — o
	// apriva un targeting per un bersaglio che una reazione non ha: chi subira' la reazione lo decide il
	// trigger durante la risoluzione, non il giocatore in pianificazione.
	//
	// E' l'anello che mancava alla catena di E5: il campo, le regole, i cinque punti di valutazione e i sette
	// moduli esistevano, ma `PlannedReactionAbility` lo scrivevano **solo i test**.
	if (Ability->Def.Slot == ERTActionSlot::Reaction)
	{
		if (!Unit->CanUseAbility(Index))
		{
			// Il rifiuto e' DETTO: una reazione in ricarica che sparisse in silenzio lascerebbe il giocatore
			// convinto di averla armata, e la scoperta arriverebbe a turno risolto.
			UE_LOG(LogRT, Log, TEXT("[RT] %s non pronta: reazione non armata"), *Ability->DisplayName.ToString());
			return;
		}
		Unit->PlannedReactionAbility = Index;
		UE_LOG(LogRT, Log, TEXT("[RT] %s arma %s (reazione)"), *Unit->GetName(), *Ability->DisplayName.ToString());
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

// ======================================================================================================
// Contratto del puntatore (CP 11.8) — owner: docs/technical/spec-pointer-interaction.md
// ======================================================================================================

ERTPointerContext ARTPlayerController::GetPointerContext() const
{
	// ⚠️ `ReactionWindow` e `Modal` non compaiono, e non e' una dimenticanza: nessuno li produce ancora.
	// La finestra di reazione e' E14, il modale e' lo Screen HUD (#613). Mettere qui un flag che nessuno
	// scrive avrebbe creato un campo senza produttore — il difetto che questo stesso checkpoint ha appena
	// finito di documentare in §2.1 dell'owner. Quando quegli owner arrivano, aggiungono il proprio ramo.

	// Il playback sovrascrive tutto: dal primo segmento risolto al Cleanup nessun input cambia il piano.
	//
	// ⚠️ `MatchEnded` e' escluso di proposito e NON diventa un contesto suo: il contratto ne dichiara otto e
	// non ne prevede un nono. A partita finita il puntatore resta in `IdleSelection`/`Planning`, e cio' che
	// impedisce di pianificare e' lo snapshot del `TurnManager`, che rifiuta a valle — la stessa autorita'
	// di §3. Aggiungere qui un ramo significherebbe far decidere al puntatore quando il gioco e' finito.
	if (const ARTTurnManager* TM = PacingTurnManager(this))
	{
		const ERTMatchPhase Phase = TM->GetPhase();
		if (Phase != ERTMatchPhase::Planning && Phase != ERTMatchPhase::MatchEnded)
		{
			return ERTPointerContext::ResolutionPlayback;
		}
	}

	const ARTUnit* Unit = GetSelectedUnit();
	if (!Unit)
	{
		return ERTPointerContext::IdleSelection;
	}

	if (bDeclaringFacing)
	{
		return ERTPointerContext::Facing;
	}

	// Un'abilita' armata porta in `Targeting` — SALVO la mobilita' rapida, che non chiede un bersaglio ma
	// una destinazione e passa da `HandleClickOnCell` come qualunque waypoint.
	const int32 Armed = Unit->SelectedAbilityIndex;
	if (Armed != INDEX_NONE)
	{
		const URTActionData* Ability = Unit->GetAbility(Armed);
		if (Ability && !URTCatalogLibrary::IsFastMovement(Ability->Def))
		{
			return ERTPointerContext::Targeting;
		}
	}

	if (Unit->PlannedWaypoints.Num() > 0)
	{
		return ERTPointerContext::Pathing;
	}

	// Lo stato NEUTRO di D-128: unita' selezionata, niente armato, nessun waypoint. Qui un click su un
	// nemico ISPEZIONA e non pianifica.
	return ERTPointerContext::Planning;
}

ERTPointerTargetKind ARTPlayerController::GetPointerTargetKind() const
{
	const ARTUnit* Unit = GetSelectedUnit();
	if (!Unit) { return ERTPointerTargetKind::None; }

	const int32 Armed = Unit->SelectedAbilityIndex;
	if (Armed == INDEX_NONE) { return ERTPointerTargetKind::None; }

	const URTActionData* Ability = Unit->GetAbility(Armed);
	if (!Ability) { return ERTPointerTargetKind::None; }

	return URTPointerLibrary::TargetKindForAction(Ability->Def, Ability->bSelfTarget, Ability->Shape);
}

ERTPointerBackStep ARTPlayerController::ApplyBack()
{
	ARTUnit* Unit = GetSelectedUnit();
	const int32 Waypoints = Unit ? Unit->PlannedWaypoints.Num() : 0;

	const ERTPointerBackStep Step = URTPointerLibrary::ResolveBack(
		GetPointerContext(), bInspectorPinned, Waypoints, bPhaseFocusPinned);

	switch (Step)
	{
	case ERTPointerBackStep::Inspector:
		bInspectorPinned = false;
		break;

	case ERTPointerBackStep::Declaration:
		// Esce da `Targeting` o da `Facing` e torna al neutro. **Non deseleziona**: uscire da un targeting
		// non deve costare la selezione, che e' l'errore che costringe a ricliccare la propria unita' dopo
		// ogni ripensamento.
		bDeclaringFacing = false;
		if (Unit)
		{
			Unit->SelectAbility(INDEX_NONE);
		}
		break;

	case ERTPointerBackStep::Waypoint:
		if (Unit && Unit->PlannedWaypoints.Num() > 0)
		{
			Unit->PlannedWaypoints.Pop();
			RebuildPlannedPath();
		}
		break;

	case ERTPointerBackStep::Pathing:
		// Nessun waypoint da togliere: si esce dal contesto e basta. Non c'e' stato da azzerare — `Pathing`
		// e' derivato dal numero di waypoint, e a zero e' gia' `Planning`.
		break;

	case ERTPointerBackStep::PhaseFocus:
		bPhaseFocusPinned = false;
		break;

	case ERTPointerBackStep::ReactionFallback:
	case ERTPointerBackStep::Modal:
		// Nessun produttore: vedi la nota in `GetPointerContext`. L'ordine e' dichiarato e testato nella
		// libreria pura; l'effetto arrivera' col proprio owner.
		break;

	default:
		break;
	}

	// ⚠️ **Solo il waypoint conta come `Undo`, e la distinzione non e' pedanteria.** `ERTPlanningInput::Undo`
	// e' documentato come «waypoint annullato» (`RTPacing.h:21`) ed e' un segnale di INDECISIONE nelle
	// metriche di ritmo. Chiudere un inspector, uscire da un targeting o togliere il pin a una fase sono
	// attivita' del giocatore, non ripensamenti su un piano: contarli come `Undo` gonfierebbe una metrica di
	// `PIE-V01-MATCHLEN` con eventi che non significano quel che il numero dice. `Click` aggiorna i tempi
	// senza incrementare nulla, che e' esattamente cio' che serve.
	if (Step != ERTPointerBackStep::None)
	{
		if (ARTTurnManager* TM = PacingTurnManager(this))
		{
			TM->RecordPlanningInput(Step == ERTPointerBackStep::Waypoint
				? ERTPlanningInput::Undo
				: ERTPlanningInput::Click);
		}
	}

	return Step;
}

bool ARTPlayerController::HandleTargetCell(const FRTCellId& Cell)
{
	ARTUnit* Unit = GetSelectedUnit();
	if (!Unit)
	{
		return false;
	}
	if (GetPointerContext() != ERTPointerContext::Targeting
		|| GetPointerTargetKind() != ERTPointerTargetKind::Cell)
	{
		// Il rifiuto e' DETTO: `Blocked` senza motivo e' un difetto, non un esito.
		UE_LOG(LogRT, Log, TEXT("[RT] Bersaglio a cella: nessuna azione ad area armata"));
		return false;
	}

	const int32 Armed = Unit->SelectedAbilityIndex;
	const URTActionData* Ability = Unit->GetAbility(Armed);
	if (!Ability) { return false; }

	if (!Unit->CanUseAbility(Armed))
	{
		UE_LOG(LogRT, Log, TEXT("[RT] %s non pronta (ricarica/energia)"), *Ability->DisplayName.ToString());
		return false;
	}

	// La legalita' la CHIEDE al servizio autorevole, non la calcola. Nota: si valida la CELLA, non l'unita'
	// che ci sta sopra — un'area si centra dove si vuole, anche su un varco vuoto.
	FVector Origin; float HexSize; float LayerH; const URTHexMapAsset* Map = nullptr;
	HexMapWithContext(GetWorld(), Origin, HexSize, LayerH, Map);
	if (!Map || !Map->ContainsCell(Cell))
	{
		UE_LOG(LogRT, Log, TEXT("[RT] Bersaglio a cella fuori mappa"));
		return false;
	}

	const ERTHexTargetReason Reason = URTCombatLibrary::ClassifyHexTargeting(
		Map, Unit->Cell, Cell, Ability->RangeCells);
	if (Reason != ERTHexTargetReason::Ok)
	{
		UE_LOG(LogRT, Log, TEXT("[RT] Cella non bersagliabile (%s, portata %d)"),
			Reason == ERTHexTargetReason::OutOfRange ? TEXT("fuori portata") : TEXT("bloccata"),
			Ability->RangeCells);
		return false;
	}

	Unit->PlannedAbilityIndex = Armed;
	Unit->PlannedAttackTarget = nullptr; // il bersaglio e' la CELLA: un target-unita' residuo la sovrascriverebbe
	Unit->PlannedAttackCell = Cell;
	Unit->bAttackTargetsCell = true;

	RefreshPlanningPreview(GetWorld(), Unit);
	if (ARTTurnManager* TM = PacingTurnManager(this))
	{
		TM->RecordPlanningInput(ERTPlanningInput::Order);
	}
	UE_LOG(LogRT, Log, TEXT("[RT] Piano: %s usa %s sulla cella (%d,%d,%d)"), *Unit->GetName(),
		*Ability->DisplayName.ToString(), Cell.X, Cell.Y, Cell.Layer);
	return true;
}

bool ARTPlayerController::HandleTargetEdge(const FRTCellId& Cell, ERTHexDirection Edge)
{
	ARTUnit* Unit = GetSelectedUnit();
	if (!Unit)
	{
		return false;
	}
	if (GetPointerContext() != ERTPointerContext::Targeting
		|| GetPointerTargetKind() != ERTPointerTargetKind::Edge)
	{
		UE_LOG(LogRT, Log, TEXT("[RT] Bersaglio a bordo: nessuna azione su struttura armata"));
		return false;
	}

	const int32 Armed = Unit->SelectedAbilityIndex;
	const URTActionData* Ability = Unit->GetAbility(Armed);
	if (!Ability) { return false; }

	if (!Unit->CanUseAbility(Armed))
	{
		UE_LOG(LogRT, Log, TEXT("[RT] %s non pronta (ricarica/energia)"), *Ability->DisplayName.ToString());
		return false;
	}

	FVector Origin; float HexSize; float LayerH; const URTHexMapAsset* Map = nullptr;
	HexMapWithContext(GetWorld(), Origin, HexSize, LayerH, Map);
	if (!Map || !Map->ContainsCell(Cell))
	{
		UE_LOG(LogRT, Log, TEXT("[RT] Bordo fuori mappa"));
		return false;
	}

	const ERTHexTargetReason Reason = URTCombatLibrary::ClassifyHexTargeting(
		Map, Unit->Cell, Cell, Ability->RangeCells);
	if (Reason != ERTHexTargetReason::Ok)
	{
		UE_LOG(LogRT, Log, TEXT("[RT] Bordo non raggiungibile (portata %d)"), Ability->RangeCells);
		return false;
	}

	// Cella E direzione: il resolver di CP 9.5 rifiuta con `CoverRejected` se il piano non dichiara il lato,
	// e a portata 3 il bordo non si deduce piu' dalla coppia di celle.
	Unit->PlannedAbilityIndex = Armed;
	Unit->PlannedAttackTarget = nullptr;
	Unit->PlannedAttackCell = Cell;
	Unit->bAttackTargetsCell = true;
	Unit->PlannedCoverEdge = Edge;
	Unit->bHasPlannedCoverEdge = true;

	// ⚠️ **Niente `RefreshPlanningPreview` qui, a differenza di `HandleTargetCell`, e non e' una svista.**
	// Quella anteprima disegna l'AREA COLPITA dell'azione: per un'azione su struttura di bordo l'esito non e'
	// un'area ma un LATO, e mostrare un ventaglio di celle direbbe al giocatore una cosa che non succedera'.
	// L'anteprima del bordo e' lavoro di presentazione (#613), e finche' non esiste e' meglio non disegnare
	// nulla che disegnare la forma sbagliata.
	if (ARTTurnManager* TM = PacingTurnManager(this))
	{
		TM->RecordPlanningInput(ERTPlanningInput::Order);
	}
	UE_LOG(LogRT, Log, TEXT("[RT] Piano: %s agisce sul bordo %d della cella (%d,%d,%d)"), *Unit->GetName(),
		(int32)Edge, Cell.X, Cell.Y, Cell.Layer);
	return true;
}

void ARTPlayerController::BeginFacingDeclaration()
{
	if (!GetSelectedUnit())
	{
		UE_LOG(LogRT, Log, TEXT("[RT] Rotazione: nessuna unita' selezionata"));
		return;
	}
	bDeclaringFacing = true;
}

void ARTPlayerController::EndFacingDeclaration()
{
	bDeclaringFacing = false;
}

bool ARTPlayerController::HandleFacingSector(ERTHexDirection Sector)
{
	ARTUnit* Unit = GetSelectedUnit();
	if (!Unit)
	{
		return false;
	}
	if (GetPointerContext() != ERTPointerContext::Facing)
	{
		UE_LOG(LogRT, Log, TEXT("[RT] Rotazione: non si sta dichiarando un facing"));
		return false;
	}

	// Lo stile su cui si misura la legalita' e' quello del movimento PIANIFICATO: chi non si e' mosso ruota
	// libero (`None`, sei direzioni), chi ha un percorso a budget ha le tre dell'ultimo passo.
	//
	// ⚠️ Questa e' una PREVISIONE, non il verdetto. Il resolver rivalida a fine Move su `MovementStyleThisTurn`
	// e `WalkedThisTurn`, cioe' su quel che e' successo davvero: un percorso puo' essere interrotto, e la
	// dichiarazione allora cade con `DeclarationRejected`. La UI propone, il servizio decide — §3 dell'owner.
	const bool bHasPlannedMove = Unit->PlannedPath.Num() > 1;
	const ERTMovementStyle Style = bHasPlannedMove ? ERTMovementStyle::Budget : ERTMovementStyle::None;

	ERTHexDirection Applied = Unit->Facing;
	const bool bLegal = URTFacingLibrary::TryApplyDeclaredFacing(
		Style, Unit->PlannedPath, Unit->Facing, Sector, Applied);

	if (!bLegal)
	{
		// Rifiutata, MAI corretta in silenzio verso la legale piu' vicina: e' la regola di `URTFacingLibrary`,
		// e qui la si rispetta invece di riscriverla.
		UE_LOG(LogRT, Log, TEXT("[RT] Rotazione %d illegale per lo stile di movimento pianificato"),
			(int32)Sector);
		return false;
	}

	Unit->PlannedFacing = Sector;
	Unit->bDeclaresPlannedFacing = true;
	bDeclaringFacing = false;

	if (ARTTurnManager* TM = PacingTurnManager(this))
	{
		TM->RecordPlanningInput(ERTPlanningInput::Order);
	}
	UE_LOG(LogRT, Log, TEXT("[RT] Piano: %s dichiara rotazione %d"), *Unit->GetName(), (int32)Sector);
	return true;
}
