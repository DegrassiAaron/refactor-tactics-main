// La tabella azione → evento → handler del controller (#4065, S0).
//
// I test `PlayerInput.*` che esistevano leggono il mapping context: tasto → azione. Nessuno leggeva l'altra meta',
// azione → handler, che e' scritta in C++ in `ARTPlayerController::BindInputActions`. E' la meta' che si rompe in
// silenzio quando un handler si sposta: il codice compila, e il tasto non fa niente. La vedrebbe solo chi gioca.
//
// Qui la si legge in due modi:
//   1. la TABELLA: le coppie (azione, evento) che il componente porta, contro quelle attese, in tutte e due le
//      direzioni; ogni coppia legata al controller; ogni azione mappata ha un binding e ogni binding ha un tasto;
//   2. l'EFFETTO: i binding della camera e del kit si eseguono attraverso il delegate vero, con un valore e un
//      evento costruiti dal test, e si guarda cosa cambia sulla camera o sull'unita'.
//
// ⚠️ **Cosa NON copre.** Il passo tasto → azione resta dei test che leggono il mapping context. Della rotella si prova
// solo il verso negativo: senza viewport `IsCursorInsideScene()` e' falso e lo zoom, per contratto, non parte. Il dolly
// (`Ctrl` + destro) vuole `IsInputKeyDown(RightMouseButton)`, che senza `PlayerInput` e' falso. Le loro coppie stanno
// comunque nella tabella.

#include "Misc/AutomationTest.h"
#include "Camera/RTCameraPawn.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Player/RTPlayerController.h"
#include "Player/Input/RTInputKeys.h"
#include "Tests/Shared/RTAbilityFixtures.h"
#include "Tests/Shared/RTInputBindingFixtures.h"
#include "Tests/Shared/RTWorldFixtures.h"
#include "Unit/RTUnit.h"
#include "Ability/RTActionData.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FString BindingEventName(ETriggerEvent Event)
	{
		return StaticEnum<ETriggerEvent>()->GetNameStringByValue(static_cast<int64>(Event));
	}

	FString BindingPair(const UInputAction* Action, ETriggerEvent Event)
	{
		return FString::Printf(TEXT("%s:%s"), Action ? *Action->GetName() : TEXT("<nessuna azione>"), *BindingEventName(Event));
	}

	/**
	 * Le coppie attese, scritte a mano: e' la tabella contro cui si misura `BindInputActions`, e non si ricava da lui.
	 *
	 * Le due famiglie (`IA_Ability*`, `IA_Generic*`) si contano sulle tabelle dei tasti: quanti tasti ci sono lo decide
	 * quella tabella, e lo difendono `PlayerInput.EveryKitEntryIsReachable` e i suoi gemelli. Qui si chiede che OGNI
	 * tasto di quelle tabelle abbia il suo binding sull'evento giusto.
	 */
	TArray<FString> BindingExpectedPairs()
	{
		TArray<FString> Attese = {
			TEXT("IA_Pan:Triggered"), TEXT("IA_Zoom:Triggered"), TEXT("IA_Rotate:Started"),
			TEXT("IA_OrbitModifier:Started"), TEXT("IA_OrbitModifier:Completed"), TEXT("IA_OrbitModifier:Canceled"),
			TEXT("IA_Orbit:Triggered"),
			TEXT("IA_Select:Started"), TEXT("IA_Select:Completed"), TEXT("IA_Select:Canceled"),
			TEXT("IA_LockIn:Started"), TEXT("IA_Restart:Started"),
			TEXT("IA_UndoWaypoint:Started"), TEXT("IA_UndoWaypoint:Completed"), TEXT("IA_UndoWaypoint:Canceled"),
			TEXT("IA_CycleSelection:Started"), TEXT("IA_ToggleStrategicView:Started"), TEXT("IA_DeclarePlan:Started"),
			TEXT("IA_Recenter:Started"), TEXT("IA_FocusSelected:Started"),
			TEXT("IA_CameraModifier:Started"), TEXT("IA_CameraModifier:Completed"), TEXT("IA_CameraModifier:Canceled"),
			TEXT("IA_LayerUp:Started"), TEXT("IA_LayerDown:Started"),
			TEXT("IA_CyclePlaybackSpeed:Started"), TEXT("IA_PausePrepWindow:Started"), TEXT("IA_DeclareSneak:Started"),
			TEXT("IA_TogglePlaybackPause:Started"), TEXT("IA_StepPlaybackMicroStep:Started"),
			TEXT("IA_LabStepPrevious:Started"), TEXT("IA_LabStepNext:Started"),
			TEXT("IA_Pause:Started"),
		};
		for (int32 I = 0; I < RTInputKeys::AbilityHotkeys().Num(); ++I)
		{
			Attese.Add(FString::Printf(TEXT("IA_Ability%d:Started"), I + 1));
		}
		for (int32 I = 0; I < RTInputKeys::GenericHotkeys().Num(); ++I)
		{
			Attese.Add(FString::Printf(TEXT("IA_Generic%d:Started"), I + 1));
		}
		return Attese;
	}

	/** Mappa piatta di raggio 2 attorno a una cella lontana dall'origine: `Home` ha un centro a cui tornare. */
	ARTHexMapActor* BindingSpawnMap(UWorld* World, const FRTCellId& Center)
	{
		URTHexMapAsset* Asset = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(Center, /*Radius=*/ 2))
		{
			Asset->AddOrUpdateCell(FRTHexCellData(Id));
		}
		Asset->SortCells();
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor)
		{
			MapActor->MapAsset = Asset;
		}
		return MapActor;
	}

	ARTUnit* BindingSpawnUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U)
		{
			return nullptr;
		}
		U->TeamId = TeamId;
		U->Cell = Cell;
		U->FinishSpawning(FTransform::Identity);
		return U;
	}

	/** Controller della squadra 0, con camera posseduta e un componente d'input legato. */
	struct FRTBindingBench
	{
		UWorld* World = nullptr;
		ARTPlayerController* PC = nullptr;
		ARTCameraPawn* Cam = nullptr;
		UEnhancedInputComponent* Input = nullptr;

		bool Build(FAutomationTestBase& Test)
		{
			World = RTWorldFixtures::MakeWorld();
			PC = RTWorldFixtures::MakePlayerOnTeam(World, 0);
			Cam = World ? World->SpawnActor<ARTCameraPawn>() : nullptr;
			if (!Test.TestNotNull(TEXT("premessa: il mondo"), World) || !Test.TestNotNull(TEXT("premessa: il controller"), PC)
				|| !Test.TestNotNull(TEXT("premessa: la camera"), Cam))
			{
				return false;
			}
			PC->Possess(Cam);
			Input = RTInputBindingFixtures::BindInput(*PC);
			return Test.TestTrue(TEXT("premessa: il controller possiede la camera"), PC->GetPawn() == Cam);
		}

		~FRTBindingBench() { RTWorldFixtures::DestroyWorld(World); }
	};
}

/**
 * **Ogni azione e' legata sui suoi eventi, e solo su quelli.**
 *
 * 🔴 Il difetto che misura: un `BindAction` che manca, o che cambia evento (`Started` al posto di `Completed`), non
 * fa fallire niente al momento della compilazione. Il primo caso spegne un tasto; il secondo lascia un gesto armato
 * per sempre (vedi i commenti su `Canceled` in `BindInputActions`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInputBindingTableTest,
	"RefactorTactics.PlayerInput.EveryActionIsBoundOnItsEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInputBindingTableTest::RunTest(const FString&)
{
	FRTBindingBench B;
	if (!B.Build(*this))
	{
		return false;
	}

	TArray<FString> Trovate;
	TSet<FString> AzioniLegate;
	for (const TUniquePtr<FEnhancedInputActionEventBinding>& Binding : B.Input->GetActionEventBindings())
	{
		if (!TestTrue(TEXT("ogni binding porta un'azione"), Binding.IsValid() && Binding->GetAction() != nullptr))
		{
			continue;
		}
		const FString Coppia = BindingPair(Binding->GetAction(), Binding->GetTriggerEvent());
		TestTrue(FString::Printf(TEXT("%s non compare due volte"), *Coppia), !Trovate.Contains(Coppia));
		TestTrue(FString::Printf(TEXT("%s e' legata al controller"), *Coppia), Binding->GetUObject() == B.PC);
		Trovate.Add(Coppia);
		AzioniLegate.Add(Binding->GetAction()->GetName());
	}

	const TArray<FString> Attese = BindingExpectedPairs();
	TArray<FString> Mancanti;
	for (const FString& Attesa : Attese)
	{
		if (!Trovate.Contains(Attesa)) { Mancanti.Add(Attesa); }
	}
	TArray<FString> Inattese;
	for (const FString& Trovata : Trovate)
	{
		if (!Attese.Contains(Trovata)) { Inattese.Add(Trovata); }
	}
	Mancanti.Sort();
	Inattese.Sort();
	TestEqual(FString::Printf(TEXT("nessuna coppia attesa manca (mancano: %s)"), *FString::Join(Mancanti, TEXT(", "))), Mancanti.Num(), 0);
	TestEqual(FString::Printf(TEXT("nessuna coppia inattesa (in piu': %s)"), *FString::Join(Inattese, TEXT(", "))), Inattese.Num(), 0);
	TestEqual(TEXT("le coppie sono quante la tabella attesa"), Trovate.Num(), Attese.Num());

	// Il ponte con l'altra meta': un tasto senza handler non fa niente, un handler senza tasto non si raggiunge.
	const UInputMappingContext* Contesto = B.PC->BuildAndGetMappingContextForTest();
	if (!TestNotNull(TEXT("il mapping context"), Contesto))
	{
		return false;
	}
	TSet<FString> AzioniMappate;
	for (const FEnhancedActionKeyMapping& Mapping : Contesto->GetMappings())
	{
		if (Mapping.Action)
		{
			AzioniMappate.Add(Mapping.Action->GetName());
		}
	}
	for (const FString& Mappata : AzioniMappate)
	{
		TestTrue(FString::Printf(TEXT("%s ha un tasto e ha anche un binding"), *Mappata), AzioniLegate.Contains(Mappata));
	}
	for (const FString& Legata : AzioniLegate)
	{
		TestTrue(FString::Printf(TEXT("%s ha un binding e ha anche un tasto"), *Legata), AzioniMappate.Contains(Legata));
	}
	return true;
}

/**
 * **I binding della camera muovono la camera.** Si eseguono attraverso il delegate vero, con il valore che il
 * tasto consegnerebbe, e si guarda la camera.
 *
 * E' la rete della fetta S3 di #4065, che sposta questi gesti fuori dal controller: se un handler smette di inoltrare,
 * la tabella resta giusta e questo test diventa rosso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInputBindingCameraTest,
	"RefactorTactics.PlayerInput.CameraBindingsReachTheCamera",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInputBindingCameraTest::RunTest(const FString&)
{
	FRTBindingBench B;
	if (!B.Build(*this))
	{
		return false;
	}
	B.Cam->SetSensitivitiesForTest(/*Yaw=*/ 1.f, /*Pitch=*/ 1.f);
	const auto Fire = [&](const TCHAR* Name, ETriggerEvent Event, const FInputActionValue& Value)
	{
		TestEqual(FString::Printf(TEXT("%s esegue un binding"), *BindingPair(RTInputBindingFixtures::ActionNamed(*B.Input, Name), Event)),
			RTInputBindingFixtures::Fire(*B.Input, Name, Event, Value), 1);
	};

	// Q/E: la rotazione a scatti.
	const float Yaw0 = B.Cam->GetCameraYaw();
	Fire(TEXT("IA_Rotate"), ETriggerEvent::Started, FInputActionValue(1.f));
	TestNotEqual(TEXT("E ruota la vista"), B.Cam->GetCameraYaw(), Yaw0);

	// WASD: il pan e' a velocita', quindi vuole il tempo del fotogramma, che in un mondo mai avanzato vale zero.
	B.World->DeltaTimeSeconds = 1.f / 60.f;
	B.Cam->SetCameraPivotForTest(FVector::ZeroVector);
	Fire(TEXT("IA_Pan"), ETriggerEvent::Triggered, FInputActionValue(FVector2D(1.f, 0.f)));
	TestTrue(TEXT("D sposta la vista"), FVector::Dist2D(B.Cam->GetCameraPivot(), FVector::ZeroVector) > 0.01);

	// Il tasto centrale arma l'orbita, il mouse la guida; rilascio e annullamento la disarmano.
	const FVector2D Trascinamento(30.f, 0.f);
	float Yaw = B.Cam->GetCameraYaw();
	Fire(TEXT("IA_Orbit"), ETriggerEvent::Triggered, FInputActionValue(Trascinamento));
	TestEqual(TEXT("senza il tasto centrale il mouse non orbita"), B.Cam->GetCameraYaw(), Yaw);
	Fire(TEXT("IA_OrbitModifier"), ETriggerEvent::Started, FInputActionValue(true));
	Fire(TEXT("IA_Orbit"), ETriggerEvent::Triggered, FInputActionValue(Trascinamento));
	TestNotEqual(TEXT("col tasto centrale il mouse orbita"), B.Cam->GetCameraYaw(), Yaw);
	Fire(TEXT("IA_OrbitModifier"), ETriggerEvent::Completed, FInputActionValue(false));
	Yaw = B.Cam->GetCameraYaw();
	Fire(TEXT("IA_Orbit"), ETriggerEvent::Triggered, FInputActionValue(Trascinamento));
	TestEqual(TEXT("rilasciato il tasto centrale, il mouse non orbita piu'"), B.Cam->GetCameraYaw(), Yaw);
	Fire(TEXT("IA_OrbitModifier"), ETriggerEvent::Started, FInputActionValue(true));
	Fire(TEXT("IA_OrbitModifier"), ETriggerEvent::Canceled, FInputActionValue(false));
	Fire(TEXT("IA_Orbit"), ETriggerEvent::Triggered, FInputActionValue(Trascinamento));
	TestEqual(TEXT("annullato il tasto centrale, il mouse non orbita"), B.Cam->GetCameraYaw(), Yaw);

	// `Ctrl` da solo: il mouse sposta il peek, e lo spostamento finisce col rilascio o con l'annullamento.
	B.Cam->SetPeekOffset(FVector::ZeroVector);
	Fire(TEXT("IA_CameraModifier"), ETriggerEvent::Started, FInputActionValue(true));
	Fire(TEXT("IA_Orbit"), ETriggerEvent::Triggered, FInputActionValue(FVector2D(10.f, 0.f)));
	const FVector Peek = B.Cam->GetPeekOffset();
	TestFalse(TEXT("Ctrl tenuto: il mouse sposta il peek"), Peek.IsNearlyZero());
	TestEqual(TEXT("Ctrl tenuto: il mouse non orbita"), B.Cam->GetCameraYaw(), Yaw);
	Fire(TEXT("IA_CameraModifier"), ETriggerEvent::Completed, FInputActionValue(false));
	Fire(TEXT("IA_Orbit"), ETriggerEvent::Triggered, FInputActionValue(FVector2D(10.f, 0.f)));
	TestTrue(TEXT("rilasciato Ctrl, il mouse non sposta piu' il peek"), B.Cam->GetPeekOffset().Equals(Peek));
	Fire(TEXT("IA_CameraModifier"), ETriggerEvent::Started, FInputActionValue(true));
	Fire(TEXT("IA_CameraModifier"), ETriggerEvent::Canceled, FInputActionValue(false));
	Fire(TEXT("IA_Orbit"), ETriggerEvent::Triggered, FInputActionValue(FVector2D(10.f, 0.f)));
	TestTrue(TEXT("annullato Ctrl, il mouse non sposta il peek"), B.Cam->GetPeekOffset().Equals(Peek));

	// `Ctrl` + sinistro trascinato oltre la soglia orbita; il rilascio (o l'annullamento) del sinistro chiude il gesto.
	// Il trascinamento e' lungo quanto basta a superare la soglia di default.
	const FVector2D Lungo(50.f, 0.f);
	TestTrue(TEXT("premessa: il trascinamento supera la soglia del click"), Lungo.Size() > B.PC->GetCameraGestureTuning().ClickDragThreshold);
	const auto CtrlLeftDrag = [&](ETriggerEvent Fine, const TCHAR* Nome)
	{
		Fire(TEXT("IA_CameraModifier"), ETriggerEvent::Started, FInputActionValue(true));
		Fire(TEXT("IA_Select"), ETriggerEvent::Started, FInputActionValue(true));
		const float Prima = B.Cam->GetCameraYaw();
		Fire(TEXT("IA_Orbit"), ETriggerEvent::Triggered, FInputActionValue(Lungo));
		TestNotEqual(FString::Printf(TEXT("Ctrl + sinistro trascinato orbita (%s)"), Nome), B.Cam->GetCameraYaw(), Prima);
		Fire(TEXT("IA_Select"), Fine, FInputActionValue(false));
		const float Dopo = B.Cam->GetCameraYaw();
		Fire(TEXT("IA_Orbit"), ETriggerEvent::Triggered, FInputActionValue(Lungo));
		TestEqual(FString::Printf(TEXT("chiuso il sinistro (%s), Ctrl da solo non orbita"), Nome), B.Cam->GetCameraYaw(), Dopo);
		Fire(TEXT("IA_CameraModifier"), ETriggerEvent::Completed, FInputActionValue(false));
	};
	CtrlLeftDrag(ETriggerEvent::Completed, TEXT("rilascio"));
	CtrlLeftDrag(ETriggerEvent::Canceled, TEXT("annullamento"));

	// La rotella: in headless non c'e' viewport, il cursore non e' nella scena, e lo zoom non parte. Che il ramo
	// «dentro la scena» zoomi lo prova `PlayerInput.WheelZoomsTheWorldOnlyInsideTheScene`, sui gesti.
	const float Braccio = B.Cam->GetZoomAlpha();
	Fire(TEXT("IA_Zoom"), ETriggerEvent::Triggered, FInputActionValue(1.f));
	TestEqual(TEXT("senza viewport la rotella non zooma"), B.Cam->GetZoomAlpha(), Braccio);

	// `Tab`: la vista strategica, avanti e indietro.
	const bool bStrategica = B.Cam->IsStrategicView();
	Fire(TEXT("IA_ToggleStrategicView"), ETriggerEvent::Started, FInputActionValue(true));
	TestNotEqual(TEXT("Tab cambia vista"), B.Cam->IsStrategicView(), bStrategica);
	Fire(TEXT("IA_ToggleStrategicView"), ETriggerEvent::Started, FInputActionValue(true));
	TestEqual(TEXT("un secondo Tab la riporta"), B.Cam->IsStrategicView(), bStrategica);

	// `Home` e `F`: servono una mappa e un'unita' propria.
	const FRTCellId Centro(4, 1, 0);
	ARTHexMapActor* Mappa = BindingSpawnMap(B.World, Centro);
	ARTUnit* Mia = BindingSpawnUnit(B.World, /*TeamId=*/ 0, FRTCellId(5, 1, 0));
	if (!TestNotNull(TEXT("premessa: la mappa"), Mappa) || !TestNotNull(TEXT("premessa: l'unita'"), Mia))
	{
		return false;
	}
	const FVector Lontano(50000.f, 50000.f, 0.f);
	B.Cam->SetCameraPivotForTest(Lontano);
	Fire(TEXT("IA_Recenter"), ETriggerEvent::Started, FInputActionValue(true));
	TestTrue(TEXT("Home riporta la vista"), FVector::Dist2D(B.Cam->GetCameraPivot(), Lontano) > 1.0);

	FVector Origine; float Lato; float Piano;
	Mappa->GetHexContext(Origine, Lato, Piano);
	B.PC->SelectActorForTest(Mia);
	B.Cam->SetCameraPivotForTest(Lontano);
	Fire(TEXT("IA_FocusSelected"), ETriggerEvent::Started, FInputActionValue(true));
	TestTrue(TEXT("F inquadra l'unita' selezionata"),
		FVector::Dist2D(B.Cam->GetCameraPivot(), URTHexLibrary::AxialToWorld(Mia->Cell, Origine, Lato, Piano)) < 1.0);
	return true;
}

/**
 * **Il tasto della posizione i arma la posizione i, e il tasto di una generica arma quella generica.**
 *
 * Due tabelle stanno dietro questi tasti: quella dei tasti (`AbilityHotkeys`, `GenericHotkeys`) e quella degli
 * handler (`OnAbility1`..`OnAbility10`, `OnGeneric1`..`OnGeneric5`). Uno scambio fra due righe della seconda compila,
 * e da' un tasto che arma la voce accanto. Qui il binding si esegue e si guarda quale voce del kit si e' mossa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInputBindingKitTest,
	"RefactorTactics.PlayerInput.KitBindingsArmTheirOwnSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInputBindingKitTest::RunTest(const FString&)
{
	FRTBindingBench B;
	if (!B.Build(*this))
	{
		return false;
	}
	ARTUnit* Mia = BindingSpawnUnit(B.World, /*TeamId=*/ 0, FRTCellId(0, 0, 0));
	if (!TestNotNull(TEXT("premessa: l'unita'"), Mia))
	{
		return false;
	}
	// Dieci posizioni dello stesso attacco a bersaglio: armarlo non pianifica niente da solo, e l'indice armato resta
	// quello della posizione. Poi le generiche, in coda come in partita.
	Mia->Abilities.Reset();
	for (int32 I = 0; I < RTInputKeys::AbilityHotkeys().Num(); ++I)
	{
		RTAbilityFixtures::AddCoreAbility(Mia, TEXT("Action.BasicAttack"));
	}
	for (const TPair<FName, FKey>& Generica : RTInputKeys::GenericHotkeys())
	{
		RTAbilityFixtures::AddCoreAbility(Mia, *Generica.Key.ToString());
	}
	B.PC->SelectActorForTest(Mia);

	const auto Reset = [&]()
	{
		Mia->SelectAbility(INDEX_NONE);
		Mia->ClearPlannedMainAction();
		Mia->SetPlannedReaction(INDEX_NONE);
	};
	const auto Touched = [&](int32 Index)
	{
		return Mia->SelectedAbilityIndex == Index || Mia->PlannedAbilityIndex == Index || Mia->PlannedReactionAbility == Index;
	};

	for (int32 I = 0; I < RTInputKeys::AbilityHotkeys().Num(); ++I)
	{
		Reset();
		const FString Nome = FString::Printf(TEXT("IA_Ability%d"), I + 1);
		TestEqual(FString::Printf(TEXT("%s esegue un binding"), *Nome),
			RTInputBindingFixtures::Fire(*B.Input, *Nome, ETriggerEvent::Started, FInputActionValue(true)), 1);
		TestEqual(FString::Printf(TEXT("%s arma la posizione %d"), *Nome, I), Mia->SelectedAbilityIndex, I);
	}

	for (int32 Riga = 0; Riga < RTInputKeys::GenericHotkeys().Num(); ++Riga)
	{
		const FName Id = RTInputKeys::GenericHotkeys()[Riga].Key;
		int32 Indice = INDEX_NONE;
		for (int32 I = 0; I < Mia->NumAbilities(); ++I)
		{
			if (Mia->GetAbility(I) && Mia->GetAbility(I)->Def.ActionId == Id) { Indice = I; }
		}
		if (!TestNotEqual(FString::Printf(TEXT("premessa: %s e' nel kit"), *Id.ToString()), Indice, (int32)INDEX_NONE))
		{
			continue;
		}
		Reset();
		const FString Nome = FString::Printf(TEXT("IA_Generic%d"), Riga + 1);
		TestEqual(FString::Printf(TEXT("%s esegue un binding"), *Nome),
			RTInputBindingFixtures::Fire(*B.Input, *Nome, ETriggerEvent::Started, FInputActionValue(true)), 1);
		TestTrue(FString::Printf(TEXT("%s arma %s (posizione %d)"), *Nome, *Id.ToString(), Indice), Touched(Indice));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
