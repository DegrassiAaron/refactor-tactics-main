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
// #971: la sessione non presidiata e' un fatto del GameMode (`IsAutobattleInEffect()`), non dell'unita'.
// Vedi `IsPlanningInputInert()`.
#include "RTGameMode.h"
// CP 46.6 (#941): `ESC` apre il menu di pausa, e il contesto `Modal` del puntatore LEGGE lo stato del
// navigatore invece di tenerne una copia.
#include "Frontend/RTFrontendNavigator.h"
#include "Engine/GameInstance.h"
#include "UI/RTHUD.h" // CP 47.7: la scala x1/x2/x4 e' vocabolario di presentazione e vive nell'HUD
#include "Core/RTTypes.h"
#include "RefactorTactics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"
#include "Turn/RTPlaybackLibrary.h" // DirectionYaw: l'anteprima del facing usa la stessa geometria del playback

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

		// Dove puo' ANCORA arrivare: budget, blocchi, occupanti e archi sono gia' applicati, e i waypoint gia'
		// pianificati hanno gia' speso la loro parte (#877). Il ventaglio risponde a «quanto mi resta», non a
		// «quanto avevo»: senza i waypoint mostrerebbe un raggio che il piano in corso ha gia' consumato.
		FRTHexSnapshot Snapshot;
		int32 UnitId = INDEX_NONE;
		TArray<ARTUnit*> Units;
		TArray<FRTCellId> Reachable;
		if (PlanningSnapshotFor(World, Unit, Snapshot, UnitId, &Units))
		{
			for (const FRTHexReachableCell& R :
				URTHexSimLibrary::ReachableCellsAfterPlan(Snapshot, UnitId, Unit->PlannedWaypoints))
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

const TArray<FKey>& ARTPlayerController::AbilityHotkeys()
{
	// `1`..`9` piu' `0`: dieci posizioni, che e' quanto basta al kit di oggi — cinque voci d'eroe piu' le
	// cinque generiche di D-025. Non e' un numero scelto per stare largo: e' la dimensione misurata del kit,
	// e il test che la difende confronta le due cose invece di fidarsi di questo commento.
	//
	// ⚠️ Nessuno di questi tasti collide: mappati altrove sono A D E F Q R S T V W, Home, Escape, BackSpace,
	// Spazio e i pulsanti del mouse. Verificato sull'elenco completo dei `MapKey` di `BuildInputMappings`.
	static const TArray<FKey> Hotkeys = {
		EKeys::One,  EKeys::Two,   EKeys::Three, EKeys::Four, EKeys::Five,
		EKeys::Six,  EKeys::Seven, EKeys::Eight, EKeys::Nine, EKeys::Zero };
	return Hotkeys;
}

const TArray<TPair<FName, FKey>>& ARTPlayerController::GenericHotkeys()
{
	// Le cinque generiche di D-025 che entrano nel kit, ognuna col suo tasto STABILE: non cambiano da eroe
	// a eroe, quindi non hanno ragione di occupare una posizione della fila dei numeri.
	//
	// 🔴 **Questa tabella e' la ragione per cui un eroe puo' portare una sesta azione.** Prima le generiche
	// stavano nei numeri, il kit faceva undici voci contro dieci tasti, e la sesta abilita' finiva in una
	// posizione che il giocatore non poteva premere: raggiungibile per il gate del catalogo — che conta i
	// kit, non i tasti — e impremibile in partita. Un verde che mente.
	//
	// ⚠️ **I tasti sono scelti per la MANO, non per l'iniziale.** `O` sarebbe mnemonico per Overwatch e `I`
	// per Interact, ma stanno dall'altra parte della tastiera: queste si premono mentre la sinistra guida
	// la camera su `WASD`. `G` e `B` cadono bene e sono anche mnemonici; `C`, `X` e `Z` sono vicini e liberi.
	//
	// ⚠️ **Nessuno di questi collide**: mappati altrove sono `A D E F Q R S T V W`, `Home`, `Escape`,
	// `BackSpace`, `Spazio`, i pulsanti del mouse e le dieci cifre. Verificato sull'elenco completo dei
	// `MapKey` di `BuildInputMappings`, ed e' un controllo che `PlayerInput.HotkeysDoNotCollide` rifa'.
	static const TArray<TPair<FName, FKey>> Hotkeys = {
		{ TEXT("Action.Guard"),     EKeys::G },
		{ TEXT("Action.Brace"),     EKeys::B },
		{ TEXT("Action.Overwatch"), EKeys::C },
		{ TEXT("Action.Interact"),  EKeys::X },
		{ TEXT("Action.Wait"),      EKeys::Z } };
	return Hotkeys;
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

	// #863: orbita CONTINUA, distinta dallo scatto di Q/E. Due gesti sullo stesso stato — il tasto
	// riparte da dove il trascinamento ha lasciato.
	OrbitAction = NewObject<UInputAction>(this, TEXT("IA_Orbit"));
	OrbitAction->ValueType = EInputActionValueType::Axis2D;

	OrbitModifierAction = NewObject<UInputAction>(this, TEXT("IA_OrbitModifier"));
	OrbitModifierAction->ValueType = EInputActionValueType::Boolean;

	FacingAction = NewObject<UInputAction>(this, TEXT("IA_Facing"));
	FacingAction->ValueType = EInputActionValueType::Boolean;

	SelectAction = NewObject<UInputAction>(this, TEXT("IA_Select"));
	SelectAction->ValueType = EInputActionValueType::Boolean;

	LockInAction = NewObject<UInputAction>(this, TEXT("IA_LockIn"));
	LockInAction->ValueType = EInputActionValueType::Boolean;

	RestartAction = NewObject<UInputAction>(this, TEXT("IA_Restart"));
	RestartAction->ValueType = EInputActionValueType::Boolean;

	// Una `UInputAction` per posizione del kit: quante siano lo dice `AbilityHotkeys()`, che e' la stessa
	// lista che mappa i tasti e che il test interroga. Aggiungere un tasto e' una riga LI', non qui.
	AbilityActions.Reset();
	for (int32 i = 0; i < AbilityHotkeys().Num(); ++i)
	{
		UInputAction* Action = NewObject<UInputAction>(this, *FString::Printf(TEXT("IA_Ability%d"), i + 1));
		Action->ValueType = EInputActionValueType::Boolean;
		AbilityActions.Add(Action);
	}

	// Una `UInputAction` per azione generica. Stesso schema dei numeri, altro criterio: qui l'indice e' la
	// riga di `GenericHotkeys()`, non una posizione del kit.
	GenericActions.Reset();
	for (int32 i = 0; i < GenericHotkeys().Num(); ++i)
	{
		UInputAction* Action = NewObject<UInputAction>(this, *FString::Printf(TEXT("IA_Generic%d"), i + 1));
		Action->ValueType = EInputActionValueType::Boolean;
		GenericActions.Add(Action);
	}

	UndoAction = NewObject<UInputAction>(this, TEXT("IA_UndoWaypoint"));
	UndoAction->ValueType = EInputActionValueType::Boolean;

	RecenterAction = NewObject<UInputAction>(this, TEXT("IA_Recenter"));
	RecenterAction->ValueType = EInputActionValueType::Boolean;

	FocusAction = NewObject<UInputAction>(this, TEXT("IA_FocusSelected"));
	FocusAction->ValueType = EInputActionValueType::Boolean;

	PlaybackSpeedAction = NewObject<UInputAction>(this, TEXT("IA_CyclePlaybackSpeed"));
	PlaybackSpeedAction->ValueType = EInputActionValueType::Boolean;

	// CP 46.6 (#941): il menu di pausa.
	PauseAction = NewObject<UInputAction>(this, TEXT("IA_Pause"));
	PauseAction->ValueType = EInputActionValueType::Boolean;

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

	// Orbita continua (#863): il tasto centrale **tenuto** arma il gesto, il movimento del mouse lo guida.
	// X = yaw, Y = pitch, come dichiara la §3.2 dell'handoff camera. `RMB` resta gameplay contestuale e
	// non entra qui: e' un guardrail esplicito di quel documento.
	MappingContext->MapKey(OrbitModifierAction, EKeys::MiddleMouseButton);
	MappingContext->MapKey(OrbitAction, EKeys::Mouse2D);

	// Select (Boolean): tasto sinistro del mouse.
	MappingContext->MapKey(SelectAction, EKeys::LeftMouseButton);

	// Lock-in (Boolean): barra spaziatrice.
	MappingContext->MapKey(LockInAction, EKeys::SpaceBar);

	// Riavvio partita (Boolean): tasto R (attivo solo a match concluso).
	MappingContext->MapKey(RestartAction, EKeys::R);

	// Selezione abilita' (Boolean): `1`..`9` e `0` -> indici 0-9 dell'elenco dell'unita' selezionata.
	// Il tasto sceglie una POSIZIONE, non un'azione: quale abilita' occupi l'indice N dipende dall'eroe.
	//
	// 🔴 **Erano quattro, e quattro non bastavano**: il kit di ogni unita' e' cinque voci d'eroe piu' le
	// generiche accodate (D-025), quindi con `1`..`4` il giocatore raggiungeva la meta' scarsa di cio' che
	// possedeva — `Overwatch`, `Guard`, `Brace`, `Wait`, `Interact` e la reazione di tre eroi su quattro
	// restavano irraggiungibili mentre il bot le usava ([#1409], [#1034]).
	for (int32 i = 0; i < AbilityHotkeys().Num() && i < AbilityActions.Num(); ++i)
	{
		MappingContext->MapKey(AbilityActions[i], AbilityHotkeys()[i]);
	}

	// Selezione delle GENERICHE per nome: `G`uard, `B`race, Overwatch, Interact, Wait. Il tasto sceglie
	// un'AZIONE, non una posizione — vedi `GenericHotkeys()` per il perche' la differenza conti.
	for (int32 i = 0; i < GenericHotkeys().Num() && i < GenericActions.Num(); ++i)
	{
		MappingContext->MapKey(GenericActions[i], GenericHotkeys()[i].Value);
	}

	// Annulla l'ultimo waypoint della path composita (tasto destro del mouse o Backspace).
	MappingContext->MapKey(UndoAction, EKeys::RightMouseButton);
	MappingContext->MapKey(UndoAction, EKeys::BackSpace);

	// Ricentra la camera sul centro griglia + reset zoom (tasto Home).
	MappingContext->MapKey(RecenterAction, EKeys::Home);
	MappingContext->MapKey(FocusAction, EKeys::F);

	// Rotazione dichiarata: `T` come *turn*. Misurati i tasti gia' presi — A D E F Q R S V W, Home, Escape,
	// BackSpace, Spazio, 1-4 e i pulsanti del mouse — `T` e' libero e sta accanto a chi guida la camera.
	MappingContext->MapKey(FacingAction, EKeys::T);

	// Velocita' di riproduzione, un tasto che CICLA `x1 · x2 · x4` (CP 47.7, #1015).
	//
	// ⚠️ **Un tasto solo e non tre, e la ragione e' che tre non ci sono.** `1/2/4` sarebbero i tasti
	// ovvi per tre velocita' — e sono gia' posizioni del kit (`AbilityHotkeys()`, qui sopra), come ora lo
	// e' l'intera fila `1`..`9` piu' `0`. Il ciclo evita la collisione senza spostare hotkey che il
	// giocatore ha gia' imparato, e su una scala di tre valori costa al massimo due pressioni per arrivare
	// ovunque.
	// `V` e' libero: verificato sull'elenco completo dei `MapKey` di questa funzione.
	MappingContext->MapKey(PlaybackSpeedAction, EKeys::V);

	// `ESC`: la pausa (CP 46.6).
	//
	// ⚠️ **In PIE questo tasto e' anche lo STOP della sessione**, e la precedenza e' dell'editor: chi
	// verifica la pausa in `PIE-V01-FRONTEND-PAUSE` deve togliere la spunta a *Editor Preferences → Level
	// Editor → Play → "Escape" key stops PIE*, oppure guardare la pausa in una build packaged. Non e' un
	// difetto del binding, ed e' scritto qui perche' altrimenti si scopre davanti a una sessione che si
	// chiude e si conclude che il tasto non e' collegato.
	MappingContext->MapKey(PauseAction, EKeys::Escape);
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

		// #863 — l'orbita continua ha bisogno di `Triggered` (ogni frame in cui il mouse si muove), non di
		// `Started`: `Started` scatta una volta sola all'inizio del gesto, che e' giusto per un tasto e
		// sbagliato per un trascinamento.
		EIC->BindAction(OrbitModifierAction, ETriggerEvent::Started, this, &ARTPlayerController::OnOrbitPressed);
		EIC->BindAction(OrbitModifierAction, ETriggerEvent::Completed, this, &ARTPlayerController::OnOrbitReleased);
		// ⚠️ **Anche `Canceled`, e non e' pedanteria**: se il trigger viene annullato — rimozione del
		// mapping context, cambio pawn, un `FlushPressedKeys` parziale — `Completed` non arriva. Senza
		// questa riga `bOrbiting` resterebbe armato e da quel momento **ogni** movimento del mouse
		// ruoterebbe la camera senza che nessun tasto sia premuto. Trovato in code review.
		EIC->BindAction(OrbitModifierAction, ETriggerEvent::Canceled, this, &ARTPlayerController::OnOrbitReleased);
		EIC->BindAction(OrbitAction, ETriggerEvent::Triggered, this, &ARTPlayerController::OnOrbit);
		EIC->BindAction(SelectAction, ETriggerEvent::Started, this, &ARTPlayerController::OnSelect);
		EIC->BindAction(LockInAction, ETriggerEvent::Started, this, &ARTPlayerController::OnLockIn);
		EIC->BindAction(RestartAction, ETriggerEvent::Started, this, &ARTPlayerController::OnRestart);
		// La tabella che lega tasto e handler sta QUI e in nessun altro posto. Un handler per posizione
		// perche' `FInputActionValue` porta il valore e non l'azione che l'ha prodotto: senza la bindatura,
		// l'indice non arriverebbe da nessuna parte.
		typedef void (ARTPlayerController::*FAbilityHandler)(const FInputActionValue&);
		static const FAbilityHandler Handlers[] = {
			&ARTPlayerController::OnAbility1, &ARTPlayerController::OnAbility2,
			&ARTPlayerController::OnAbility3, &ARTPlayerController::OnAbility4,
			&ARTPlayerController::OnAbility5, &ARTPlayerController::OnAbility6,
			&ARTPlayerController::OnAbility7, &ARTPlayerController::OnAbility8,
			&ARTPlayerController::OnAbility9, &ARTPlayerController::OnAbility10 };
		const int32 NumHandlers = UE_ARRAY_COUNT(Handlers);
		for (int32 i = 0; i < AbilityActions.Num() && i < NumHandlers; ++i)
		{
			EIC->BindAction(AbilityActions[i], ETriggerEvent::Started, this, Handlers[i]);
		}
		// Un tasto senza handler non si bindera' in silenzio: il ciclo si ferma al piu' corto dei due, e
		// questa riga lo DICE invece di lasciarlo dedurre da un tasto che non fa niente.
		if (AbilityActions.Num() > NumHandlers)
		{
			UE_LOG(LogRT, Warning,
				TEXT("[RT] %d tasti abilita' mappati ma solo %d handler: le posizioni oltre la %d non rispondono"),
				AbilityActions.Num(), NumHandlers, NumHandlers);
		}
		// Le generiche, stessa forma e stessa guardia: la tabella sta qui, gli `ActionId` in `GenericHotkeys()`.
		static const FAbilityHandler GenericHandlers[] = {
			&ARTPlayerController::OnGeneric1, &ARTPlayerController::OnGeneric2,
			&ARTPlayerController::OnGeneric3, &ARTPlayerController::OnGeneric4,
			&ARTPlayerController::OnGeneric5 };
		const int32 NumGenericHandlers = UE_ARRAY_COUNT(GenericHandlers);
		for (int32 i = 0; i < GenericActions.Num() && i < NumGenericHandlers; ++i)
		{
			EIC->BindAction(GenericActions[i], ETriggerEvent::Started, this, GenericHandlers[i]);
		}
		// Stessa ragione di sopra, e qui morde prima: aggiungere una generica al catalogo (`D-025`) senza
		// aggiungere il suo handler la lascerebbe con un tasto che non fa niente, e il kit tornerebbe ad
		// avere una voce irraggiungibile — il difetto che questa tabella esiste per chiudere.
		if (GenericActions.Num() > NumGenericHandlers)
		{
			UE_LOG(LogRT, Warning,
				TEXT("[RT] %d tasti generici mappati ma solo %d handler: le generiche oltre la %d non rispondono"),
				GenericActions.Num(), NumGenericHandlers, NumGenericHandlers);
		}
		EIC->BindAction(UndoAction, ETriggerEvent::Started, this, &ARTPlayerController::OnUndoWaypoint);
		EIC->BindAction(RecenterAction, ETriggerEvent::Started, this, &ARTPlayerController::OnRecenter);
		EIC->BindAction(PlaybackSpeedAction, ETriggerEvent::Started, this, &ARTPlayerController::OnCyclePlaybackSpeed);
		EIC->BindAction(FocusAction, ETriggerEvent::Started, this, &ARTPlayerController::OnFocusSelected);
		EIC->BindAction(FacingAction, ETriggerEvent::Started, this, &ARTPlayerController::CycleDeclaredFacing);
		EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &ARTPlayerController::OnTogglePause);
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
	FocusCameraOnUnit(Unit);
	UE_LOG(LogRT, Log, TEXT("[RT] Focus su %s"), *Unit->GetName());
}

void ARTPlayerController::FocusCameraOnUnit(const ARTUnit* Unit)
{
	ARTCameraPawn* Cam = Cast<ARTCameraPawn>(GetPawn());
	if (!Cam || !Unit)
	{
		return;
	}

	// La CELLA, non l'attore: `ARTUnit` si posiziona con `VisualZOffset` (= `UnitHalfHeight`), quindi
	// `GetActorLocation()` sta mezzo corpo **sopra** il piano. Inquadrare li' porterebbe il pivot di `F`
	// a una quota che nessun'altra inquadratura usa — `Home` e l'avvio partita stanno sul piano — e le due
	// divergerebbero di una costante.
	// ⚠️ La prima stesura di #887 passava `GetActorLocation()` e giustificava il fix dicendo che il
	// controller «passa l'unita' sul terreno»: falso, e trovato in code review.
	if (const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(GetWorld()))
	{
		FVector Origin; float HexSize; float LayerHeight;
		HexMap->GetHexContext(Origin, HexSize, LayerHeight);
		Cam->FocusOn(URTHexLibrary::AxialToWorld(Unit->Cell, Origin, HexSize, LayerHeight));
		return;
	}

	// Senza mappa non c'e' un piano a cui ancorarsi: si ripiega sull'attore, che e' comunque meglio di
	// non inquadrare.
	Cam->FocusOn(Unit->GetActorLocation());
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
	ARTCameraPawn* Cam = Cast<ARTCameraPawn>(GetPawn());
	if (!Cam)
	{
		return;
	}

	// L'ancora e' la **CELLA** sotto il cursore, convertita col centro del piano — non il punto d'impatto
	// del raycast.
	//
	// 🔴 La prima stesura passava `Hit.ImpactPoint`, ed era lo stesso difetto di `#887` ripetuto in questo
	// file: se il cursore sta su un'unita', l'impatto e' sul cilindro, **180 unita' sopra il piano**
	// (`UnitHalfHeight` + `VisualZOffset`). A pitch -40° quella quota si traduce in ~215 unita' di scarto
	// orizzontale — **oltre una cella**, cioe' piu' del doppio della tolleranza di mezza cella che il DoD
	// e il nome del test dichiarano. Trovato in code review.
	//
	// ➕ E la cella e' gia' calcolata: `PlayerTick` traccia sotto il cursore a ogni frame e la memorizza
	// con `SetHoveredCell`. Rifare il raycast qui sarebbe lavoro doppio per un dato peggiore.
	if (const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(GetWorld()))
	{
		if (HexMap->IsHoveredCellValid())
		{
			FVector Origin; float HexSize; float LayerHeight;
			HexMap->GetHexContext(Origin, HexSize, LayerHeight);
			Cam->ZoomTowards(Value.Get<float>(),
				URTHexLibrary::AxialToWorld(HexMap->GetHoveredCell(), Origin, HexSize, LayerHeight));
			return;
		}
	}

	// Il cursore non e' su una cella valida (fuori mappa, o nessun viewport): si zooma sul centro, com'e'
	// sempre stato. Meglio di non zoomare.
	Cam->AddZoom(Value.Get<float>());
}

void ARTPlayerController::OnRotate(const FInputActionValue& Value)
{
	if (ARTCameraPawn* Cam = Cast<ARTCameraPawn>(GetPawn()))
	{
		Cam->AddYaw(Value.Get<float>());
	}
}

void ARTPlayerController::OnOrbitPressed(const FInputActionValue& Value)
{
	bOrbiting = true;
}

void ARTPlayerController::OnOrbitReleased(const FInputActionValue& Value)
{
	// ⚠️ Nessuno snap al rilascio: la spec lo vieta esplicitamente. Chi si e' fermato *fra* due file di
	// celle — il caso d'uso per cui `D-142` tiene lo step a 45° — vedrebbe la vista scattare via da sola.
	bOrbiting = false;
}

void ARTPlayerController::OnOrbit(const FInputActionValue& Value)
{
	OrbitCameraForTest(Value.Get<FVector2D>());
}

void ARTPlayerController::OrbitCameraForTest(const FVector2D& Delta)
{
	// 🔴 **La guardia sta QUI, non nel chiamante.** L'estrazione l'aveva lasciata in `OnOrbit`, quindi il
	// percorso testabile scavalcava proprio il gate che doveva verificare — e il test l'ha fatto cadere
	// alla prima esecuzione. Una decisione estratta per essere verificabile deve portarsi dietro **tutta**
	// la decisione, altrimenti il test misura una funzione che in partita non esiste.
	//
	// Il movimento del mouse arriva a ogni frame: senza questa riga la vista ruoterebbe di continuo, senza
	// che nessuno abbia chiesto di orbitare.
	if (!bOrbiting)
	{
		return;
	}
	if (ARTCameraPawn* Cam = Cast<ARTCameraPawn>(GetPawn()))
	{
		// X orizzontale → yaw, Y verticale → pitch. **Una sola** scrittura di trasformata: chiamare i due
		// `Add*` in fila aggiornerebbe il braccio due volte per frame di trascinamento.
		// Il verso verticale e' una preferenza del pawn (`bInvertOrbitPitch`), non una costante decisa qui:
		// vedi la nota su quel campo — il default non e' stato verificato con le mani.
		Cam->AddOrbit(Delta.X, Delta.Y);
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
	// Una schermata bloccante copre la partita: questo input non le arriva. Vedi `IsGameplayInputBlocked`.
	if (IsGameplayInputBlocked())
	{
		return;
	}

	// #971 — sessione non presidiata: il click non seleziona e non registra. La guardia sta PRIMA del
	// campione apposta: un click che non aggancia niente non e' «il giocatore che sta lavorando».
	if (IsPlanningInputInert())
	{
		return;
	}

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
	// #971 — primo dei cinque siti `Order`. La guardia sta QUI e non solo su `OnSelect`: questa funzione e'
	// pubblica (`HandleClickOnUnitForTest` la espone), quindi «non si arriva a selezionare» non e' una
	// prova che non si arriva a pianificare.
	if (IsPlanningInputInert())
	{
		return;
	}

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
	// Il percorso si e' allungato, quindi il ventaglio si accorcia: le due meta' della stessa anteprima si
	// aggiornano insieme (#877). Aggiornarne una sola lasciava il verde a promettere celle che il waypoint
	// successivo avrebbe rifiutato.
	RefreshPlanningPreview(GetWorld(), SelectedUnit);
	PreviewPlannedFacing(SelectedUnit); // la mesh segue il piano invece di restare girata come prima
	UE_LOG(LogRT, Log, TEXT("[RT] Piano: %s -> %d waypoint (costo %d/%d)"),
		*SelectedUnit->GetName(), SelectedUnit->PlannedWaypoints.Num(), Composite.TotalCost,
		SelectedUnit->GetEffectiveMoveRange());
}

void ARTPlayerController::OnLockIn(const FInputActionValue& Value)
{
	// Una schermata bloccante copre la partita: questo input non le arriva. Vedi `IsGameplayInputBlocked`.
	if (IsGameplayInputBlocked())
	{
		return;
	}

	// #971 — sessione non presidiata. Questo tasto non produce un piano, quindi nessun criterio scritto sui
	// cinque siti `Order` lo avrebbe raggiunto: salta il playback o chiude il turno in anticipo. In una
	// partita che esiste per essere registrata e' il difetto peggiore dei due, perche' taglia il filmato
	// invece di confonderlo.
	if (IsPlanningInputInert())
	{
		return;
	}

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

void ARTPlayerController::ApplyNextPlaybackSpeed(ARTTurnManager* TurnManager)
{
	if (!TurnManager)
	{
		return;
	}

	// L'unica scrittura della UI nel modello. La scala e' dell'HUD — e' vocabolario di presentazione,
	// non una regola — e qui si applica soltanto.
	TurnManager->ViewerPlaybackSpeed = ARTHUD::NextViewerPlaybackSpeed(TurnManager->ViewerPlaybackSpeed);
}

void ARTPlayerController::OnCyclePlaybackSpeed(const FInputActionValue& Value)
{
	// Nessun vincolo di fase: si cambia ritmo mentre la risoluzione scorre — e' il punto di CP 47.2, che
	// ha reso `TickPlayback` capace di rileggere la velocita' a ogni tick invece di congelarla — e anche
	// prima che parta, perche' chi guarda una partita non presidiata sceglie il ritmo in anticipo.
	ApplyNextPlaybackSpeed(
		Cast<ARTTurnManager>(UGameplayStatics::GetActorOfClass(this, ARTTurnManager::StaticClass())));
}

void ARTPlayerController::OnRestart(const FInputActionValue& Value)
{
	// Una schermata bloccante copre la partita: questo input non le arriva. Vedi `IsGameplayInputBlocked`.
	if (IsGameplayInputBlocked())
	{
		return;
	}

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
	// Le `OnAbility*` sono one-liner che passano tutte di qui: la guardia sta nel punto comune invece che
	// ripetuta dieci volte, cosi' un tasto abilita' in piu' la eredita per costruzione.
	if (IsGameplayInputBlocked())
	{
		return;
	}

	// #971 — secondo dei cinque siti `Order`, e vale per tutti e dieci i tasti abilita' per la stessa
	// ragione del commento qui sopra.
	if (IsPlanningInputInert())
	{
		return;
	}

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

// Selezionano per INDICE, non per azione. Uno scatto e' un'abilita' di fase `ERTResolutionPhase::Dash`
// (nel roster ce n'e' una, `Hero.Phase.FluidTrail`) e non ha un tasto dedicato: sta dove la mette il
// suo eroe. Un commento che promettesse un'azione a un tasto invecchierebbe al primo cambio di roster.
void ARTPlayerController::OnAbility1(const FInputActionValue& Value)  { SelectAbilityForCurrent(0); }
void ARTPlayerController::OnAbility2(const FInputActionValue& Value)  { SelectAbilityForCurrent(1); }
void ARTPlayerController::OnAbility3(const FInputActionValue& Value)  { SelectAbilityForCurrent(2); }
void ARTPlayerController::OnAbility4(const FInputActionValue& Value)  { SelectAbilityForCurrent(3); }
void ARTPlayerController::OnAbility5(const FInputActionValue& Value)  { SelectAbilityForCurrent(4); }
void ARTPlayerController::OnAbility6(const FInputActionValue& Value)  { SelectAbilityForCurrent(5); }
void ARTPlayerController::OnAbility7(const FInputActionValue& Value)  { SelectAbilityForCurrent(6); }
void ARTPlayerController::OnAbility8(const FInputActionValue& Value)  { SelectAbilityForCurrent(7); }
void ARTPlayerController::OnAbility9(const FInputActionValue& Value)  { SelectAbilityForCurrent(8); }
void ARTPlayerController::OnAbility10(const FInputActionValue& Value) { SelectAbilityForCurrent(9); }

// Le generiche: il numero qui e' la riga di `GenericHotkeys()`, non una posizione del kit.
void ARTPlayerController::OnGeneric1(const FInputActionValue& Value) { SelectGenericSlot(0); }
void ARTPlayerController::OnGeneric2(const FInputActionValue& Value) { SelectGenericSlot(1); }
void ARTPlayerController::OnGeneric3(const FInputActionValue& Value) { SelectGenericSlot(2); }
void ARTPlayerController::OnGeneric4(const FInputActionValue& Value) { SelectGenericSlot(3); }
void ARTPlayerController::OnGeneric5(const FInputActionValue& Value) { SelectGenericSlot(4); }

void ARTPlayerController::SelectGenericSlot(int32 Slot)
{
	if (!GenericHotkeys().IsValidIndex(Slot))
	{
		return;
	}
	SelectAbilityByIdForCurrent(GenericHotkeys()[Slot].Key);
}

void ARTPlayerController::SelectAbilityByIdForCurrent(const FName& ActionId)
{
	ARTUnit* Unit = GetSelectedUnit();
	if (!Unit)
	{
		return;
	}

	// Si cerca per `ActionId` e non per posizione, ed e' l'intero punto di questo canale: le generiche sono
	// accodate al kit, quindi il loro indice dipende da quante azioni porta l'eroe.
	for (int32 i = 0; i < Unit->NumAbilities(); ++i)
	{
		const URTActionData* Ability = Unit->GetAbility(i);
		if (Ability && Ability->Def.ActionId == ActionId)
		{
			SelectAbilityForCurrent(i);
			return;
		}
	}

	// Un'unita' senza quella generica nel kit e' un caso da DIRE: le generiche le riceve ogni unita'
	// (`EnsureDefaultAbilities`), quindi se una manca il kit e' stato composto male e il silenzio
	// lascerebbe credere a un tasto rotto.
	UE_LOG(LogRT, Warning, TEXT("[RT] %s non ha %s nel kit: tasto generico senza effetto"),
		*Unit->GetName(), *ActionId.ToString());
}

void ARTPlayerController::OnUndoWaypoint(const FInputActionValue& Value)
{
	// Una schermata bloccante copre la partita: questo input non le arriva. Vedi `IsGameplayInputBlocked`.
	if (IsGameplayInputBlocked())
	{
		return;
	}

	// #971 — sessione non presidiata: non c'e' un piano umano da disfare, e `UndoCount` non deve crescere.
	if (IsPlanningInputInert())
	{
		return;
	}

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
	// Simmetrico all'aggiunta: l'undo restituisce MP, quindi il ventaglio si riallarga. Vale anche per il ramo
	// «piano non piu' valido» qui sopra, dove `PlannedWaypoints` puo' essere ancora pieno ma il percorso e'
	// stato azzerato: `ReachableCellsAfterPlan` rifa' la domanda a `BuildCompositeHexPath` e ottiene la stessa
	// risposta, cioe' il raggio pieno. Le due strade non possono divergere perche' sono la stessa funzione.
	RefreshPlanningPreview(GetWorld(), Unit);
	PreviewPlannedFacing(Unit); // vale anche quando un waypoint viene tolto: il piano si accorcia, la mesh lo segue
}

// ======================================================================================================
// Contratto del puntatore (CP 11.8) — owner: docs/technical/systems/spec-pointer-interaction.md
// ======================================================================================================

void ARTPlayerController::OnTogglePause()
{
	UGameInstance* GameInstance = GetGameInstance();
	URTFrontendNavigator* Navigator = GameInstance ? GameInstance->GetSubsystem<URTFrontendNavigator>() : nullptr;
	if (!Navigator)
	{
		// Senza frontend non c'e' pausa da aprire: uno scenario headless o una mappa di prova girano cosi',
		// e `ESC` semplicemente non fa niente. `Verbose` e non `Warning`: e' il caso normale li'.
		UE_LOG(LogRT, Verbose, TEXT("[RT] ESC senza frontend: nessun menu di pausa da aprire."));
		return;
	}

	// ⚠️ **Il toggle guarda lo stato del navigatore, non un flag locale.** Un `bPaused` qui sarebbe una
	// seconda verita' che diverge dal primo `RETURN TO MAIN MENU` — che smonta la pausa senza passare da
	// questo controller, e anzi distrugge questo controller.
	const ERTNavResult Result = Navigator->IsPauseOpen() ? Navigator->ResumeMatch() : Navigator->ShowPause();
	if (Result != ERTNavResult::Ok)
	{
		UE_LOG(LogRT, Warning, TEXT("[RT] ESC: la pausa non ha cambiato stato (%s)."),
			*UEnum::GetValueAsString(Result));
	}
}

bool ARTPlayerController::IsGameplayInputBlocked() const
{
	// Si legge il contesto invece di ri-chiedere al navigatore: il contratto del puntatore e' l'autorita'
	// su *chi consuma un input*, e un secondo interrogante produrrebbe due risposte da tenere allineate.
	return GetPointerContext() == ERTPointerContext::Modal;
}

void ARTPlayerController::OnLockInForTest()
{
	OnLockIn(FInputActionValue());
}

bool ARTPlayerController::IsPlanningInputInert() const
{
	// `GetActorOfClass` e non `GetAuthGameMode`, per la stessa ragione di `PacingTurnManager` qui sopra: i
	// mondi di prova spawnano il GameMode come un attore qualunque, e `AuthorityGameMode` li' e' nullo. Un
	// predicato che rispondesse solo in PIE non sarebbe verificabile headless, e questo DEVE esserlo.
	const ARTGameMode* GameMode =
		Cast<ARTGameMode>(UGameplayStatics::GetActorOfClass(this, ARTGameMode::StaticClass()));
	return GameMode && GameMode->IsAutobattleInEffect();
}

ERTPointerContext ARTPlayerController::GetPointerContext() const
{
	// ⚠️ `ReactionWindow` non compare ancora: nessuno lo produce, ed e' E14. Mettere qui un flag che nessuno
	// scrive creerebbe un campo senza produttore — il difetto che questo stesso checkpoint ha finito di
	// documentare in §2.1 dell'owner.
	//
	// ✅ **`Modal` invece adesso ha il suo produttore, ed e' il primo ad arrivare come questo commento
	// prevedeva** (*«quando quegli owner arrivano, aggiungono il proprio ramo»*): il menu di pausa di
	// CP 46.6. Precede ogni altro ramo perche' la precedenza dichiarata dal contratto e'
	// `Modal/Reaction UI > HUD > world` — con la pausa a schermo nessun click deve raggiungere il mondo,
	// qualunque cosa sia selezionata.
	//
	// ⛔ **Si LEGGE uno stato, non se ne tiene una copia.** Il contesto e' derivato per scelta, e un
	// `bPauseOpen` qui sarebbe la seconda verita' che diverge — la stessa ragione per cui selezione,
	// waypoint e abilita' armata non sono duplicati. E' anche il motivo per cui `RESUME` non deve
	// ripristinare niente: appena la pausa esce dallo stack, questa funzione ricalcola com'era.
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		const URTFrontendNavigator* Navigator = GameInstance->GetSubsystem<URTFrontendNavigator>();
		if (Navigator && Navigator->IsPauseOpen())
		{
			return ERTPointerContext::Modal;
		}
	}

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
		// Nessun produttore: la finestra di reazione e' E14. L'ordine e' dichiarato e testato nella
		// libreria pura; l'effetto arrivera' col proprio owner.
		break;

	case ERTPointerBackStep::Modal:
		// 🔴 **Ha un produttore dal 2026-08-24 — la pausa di CP 46.6 — e finche' non l'ha avuto questo ramo
		// era innocuo. Adesso no**: cadeva insieme a `ReactionFallback` e usciva con `Step != None`, quindi
		// la coda di questa funzione scriveva `RecordPlanningInput(Click)` nel `TurnManager`. Un `BACK`
		// premuto **a partita in pausa** sporcava la telemetria di ritmo che `PIE-V01-MATCHLEN` legge, e
		// contraddiceva il criterio del checkpoint — *«la pausa non tocca la simulazione»* — in un punto che
		// nessuno guardava. Trovato in code review sulla PR #1304.
		//
		// ⛔ **E non chiude la pausa**: il `BACK` del contratto del puntatore smonta stati di *pianificazione*
		// (inspector, targeting, waypoint). La pausa e' del navigatore, e la chiude `ESC` o il pulsante
		// `RESUME` — dare a questo ramo un'uscita dal menu sarebbe la seconda autorita' sulla navigazione
		// che l'invariante 1 di CP 46.1 vieta.
		return ERTPointerBackStep::None;

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
	// #971 — terzo dei cinque siti `Order`.
	if (IsPlanningInputInert())
	{
		return false;
	}

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
	// #971 — quarto dei cinque siti `Order`.
	if (IsPlanningInputInert())
	{
		return false;
	}

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

void ARTPlayerController::PreviewPlannedFacing(ARTUnit* Unit) const
{
	if (Unit == nullptr)
	{
		return;
	}

	// Il facing che l'unita' AVRA': la dichiarazione vince sul derivato, esattamente come a fine Move nel
	// TurnManager. Se le due regole divergessero, l'anteprima mentirebbe.
	ERTHexDirection Previsto = Unit->Facing;
	if (Unit->bDeclaresPlannedFacing)
	{
		Previsto = Unit->PlannedFacing;
	}
	else if (Unit->PlannedPath.Num() > 1)
	{
		// 🔴 **Il PRIMO passo, non l'ultimo.** In pianificazione l'unita' e' ancora sulla cella di
		// partenza: orientarla secondo l'ultimo passo del percorso la fa guardare verso una direzione che
		// assumera' dall'altra parte della mappa, e che da qui puo' essere l'opposta di dove sta per
		// incamminarsi. Chi guarda vede una figura ferma rivolta dalla parte sbagliata.
		//
		// ⚠️ **Questa e' l'anteprima della PARTENZA e non del facing finale**, che resta `FacingFromPath` e
		// lo scrive il resolver a fine Move. Le due coincidono su un percorso dritto e divergono su uno che
		// curva: durante il playback la mesh ruota passo per passo e ci arriva, e a fine risoluzione il
		// TurnManager la riallinea al valore logico.
		ERTHexDirection Partenza = Unit->Facing;
		if (URTHexLibrary::DirectionBetween(Unit->PlannedPath[0], Unit->PlannedPath[1], Partenza))
		{
			Previsto = Partenza;
		}
	}

	FVector Origin; float HexSize; float LayerH; const URTHexMapAsset* Map = nullptr;
	if (HexMapWithContext(GetWorld(), Origin, HexSize, LayerH, Map) == nullptr)
	{
		return;
	}

	// Lo yaw si ricava dalla geometria come fa il TurnManager a fine playback: dal centro della cella al
	// centro del vicino nella direzione voluta. Ricavarlo dall'enum con una tabella sarebbe una seconda
	// verita' da tenere allineata alla prima.
	const FVector Here = Unit->WorldForCell(Unit->Cell, Origin, HexSize, LayerH);
	const FVector There = Unit->WorldForCell(URTHexLibrary::Neighbor(Unit->Cell, Previsto), Origin, HexSize, LayerH);
	Unit->SetActorRotation(FRotator(0.f, URTPlaybackLibrary::DirectionYaw(Here, There), 0.f));
}

void ARTPlayerController::CycleDeclaredFacing()
{
	if (IsGameplayInputBlocked())
	{
		return;
	}

	ARTUnit* Unit = GetSelectedUnit();
	if (!Unit)
	{
		UE_LOG(LogRT, Log, TEXT("[RT] Rotazione: nessuna unita' selezionata"));
		return;
	}

	// Lo stile e' quello del movimento PIANIFICATO, come in `HandleFacingSector`: chi non si e' mosso ruota
	// libero, chi ha un percorso a budget ha le tre dell'ultimo passo. E' una previsione, non il verdetto —
	// il resolver rivalida a fine Move su quel che e' successo davvero.
	const bool bHasPlannedMove = Unit->PlannedPath.Num() > 1;
	const ERTMovementStyle Style = bHasPlannedMove ? ERTMovementStyle::Budget : ERTMovementStyle::None;

	const TArray<ERTHexDirection> Legal = URTFacingLibrary::LegalFacings(Style, Unit->PlannedPath, Unit->Facing);
	if (Legal.Num() == 0)
	{
		return; // nessuna rotazione possibile: non c'e' niente da ciclare
	}

	// Il punto di partenza e' cio' che vale ORA: la dichiarazione di questo turno se c'e', altrimenti
	// l'orientamento attuale. Senza, premere il tasto due volte ripartirebbe sempre dalla stessa direzione.
	const ERTHexDirection Corrente = Unit->bDeclaresPlannedFacing ? Unit->PlannedFacing : Unit->Facing;

	// `LegalFacings` ha ordine STABILE (per valore dell'enum), quindi il ciclo e' ripetibile: la stessa
	// sequenza di pressioni da' la stessa sequenza di direzioni.
	const int32 Indice = Legal.IndexOfByKey(Corrente);
	const ERTHexDirection Prossima = Legal[(Indice == INDEX_NONE) ? 0 : (Indice + 1) % Legal.Num()];

	// Si passa dal comando esistente invece di scrivere `PlannedFacing` a mano: e' li' che vivono la
	// validazione, il rifiuto e la registrazione dell'input di planning.
	BeginFacingDeclaration();
	if (!HandleFacingSector(Prossima))
	{
		// Non dovrebbe accadere: `Prossima` viene da `LegalFacings`. Se accade, le due funzioni non
		// concordano sullo stile, ed e' un difetto da vedere subito invece che un tasto che non fa nulla.
		UE_LOG(LogRT, Warning,
			TEXT("[RT] Rotazione: %d era nell'insieme legale ma e' stata rifiutata"), (int32)Prossima);
		EndFacingDeclaration();
		return;
	}

	UE_LOG(LogRT, Log, TEXT("[RT] %s dichiara la rotazione a %d (%d legali)"),
		*Unit->GetName(), (int32)Prossima, Legal.Num());
}

bool ARTPlayerController::HandleFacingSector(ERTHexDirection Sector)
{
	// #971 — quinto dei cinque siti `Order`. `CycleDeclaredFacing` ci arriva da un tasto e sarebbe gia'
	// coperta a monte; questa funzione e' pubblica e raggiungibile da sola, quindi la guardia sta qui.
	if (IsPlanningInputInert())
	{
		return false;
	}

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

	// La dichiarazione si vede SUBITO: un tasto che non produce nessun riscontro a schermo e' un tasto
	// che il giocatore crede rotto.
	PreviewPlannedFacing(Unit);

	if (ARTTurnManager* TM = PacingTurnManager(this))
	{
		TM->RecordPlanningInput(ERTPlanningInput::Order);
	}
	UE_LOG(LogRT, Log, TEXT("[RT] Piano: %s dichiara rotazione %d"), *Unit->GetName(), (int32)Sector);
	return true;
}
