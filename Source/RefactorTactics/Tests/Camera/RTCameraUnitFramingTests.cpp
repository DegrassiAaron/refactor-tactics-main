// `#4121`, D-618 — l'inquadratura d'unita': il tasto `T` e l'apertura delle scene del banco.
//
// La camera va DIETRO l'unita', girata dove guarda lei, col campo davanti: l'unita' sta nella parte bassa del quadro.
// I test leggono lo STATO del pawn (pivot, yaw, pitch, braccio) e ne ricavano dove cade un punto nel quadro col
// modello di `MaintainYFOV` della spec della camera (§2): nessun render, quindi girano headless.

#include "Misc/AutomationTest.h"
#include "Ability/Lab/RTAbilityLab.h"
#include "Camera/RTCameraPawn.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputMappingContext.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Player/Input/RTInputKeys.h"
#include "Player/RTPlayerController.h"
#include "RTGameMode.h"
#include "ScenarioHarness/Authoring/RTTestScenario.h"
#include "ScenarioHarness/RTScenarioSession.h"
#include "Tests/Shared/RTInputBindingFixtures.h"
#include "Tests/Shared/RTWorldFixtures.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	UWorld* MakeUnitFramingWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyUnitFramingWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Il centro della mappa NON e' l'origine, per la ragione di `SpawnCameraTestMap`: i confronti non degenerano. */
	const FRTCellId UnitFramingMapCentre(4, 1);

	ARTHexMapActor* SpawnUnitFramingMap(UWorld* World, int32 Radius = 2)
	{
		if (!World) { return nullptr; }
		URTHexMapAsset* Asset = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(UnitFramingMapCentre, Radius))
		{
			Asset->AddOrUpdateCell(FRTHexCellData(Id));
		}
		Asset->SortCells();
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor) { MapActor->MapAsset = Asset; }
		return MapActor;
	}

	ARTUnit* SpawnUnitFramingUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell, ERTHexDirection Facing)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->Cell = Cell;
		U->Facing = Facing;
		U->FinishSpawning(FTransform::Identity);
		return U;
	}

	/** Il punto del PIANO di una cella, come lo calcola chi inquadra (in tattica la board e' quella d'authoring). */
	FVector UnitFramingCellWorld(const ARTHexMapActor* Map, const FRTCellId& Cell)
	{
		FVector Origin; float HexSize; float LayerHeight;
		Map->GetHexContext(Origin, HexSize, LayerHeight);
		return URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerHeight);
	}

	/** Lo spostamento di UNA cella nel verso, sul piano: la misura con cui si esprime `UnitFramingLeadCells`. */
	FVector UnitFramingOneCellAhead(const ARTHexMapActor* Map, const FRTCellId& Cell, ERTHexDirection Facing)
	{
		FVector Ahead = UnitFramingCellWorld(Map, URTHexLibrary::Neighbor(Cell, Facing)) - UnitFramingCellWorld(Map, Cell);
		Ahead.Z = 0.0;
		return Ahead;
	}

	const float UnitFramingAspect = 16.f / 9.f;
	const float UnitFramingFov = 90.f;

	/**
	 * Dove cade `WorldPoint` nel quadro, in coordinate normalizzate: X verso destra, Y verso l'ALTO, entrambe in
	 * `[-1, 1]` dentro il quadro. Ricostruita dallo stato del pawn: l'occhio sta a un braccio dal pivot, contro lo
	 * sguardo; il semiangolo verticale e' quello di `MaintainYFOV`, `atan(tan(FOV/2) / aspetto)`.
	 * `OutDepth` e' la profondita' lungo lo sguardo: non positiva vuol dire alle spalle della camera.
	 */
	FVector2D UnitFramingNdcOf(const ARTCameraPawn& Cam, const FVector& WorldPoint, double& OutDepth)
	{
		const USpringArmComponent* Arm = Cam.FindComponentByClass<USpringArmComponent>();
		const FRotator Vista(Cam.GetCameraPitch(), Cam.GetCameraYaw(), 0.f);
		const FRotationMatrix Assi(Vista);
		const FVector Avanti = Assi.GetUnitAxis(EAxis::X);
		const FVector Destra = Assi.GetUnitAxis(EAxis::Y);
		const FVector Su = Assi.GetUnitAxis(EAxis::Z);
		const FVector Occhio = Cam.GetCameraPivot() - Avanti * (Arm ? Arm->TargetArmLength : 0.f);
		const FVector D = WorldPoint - Occhio;
		OutDepth = FVector::DotProduct(D, Avanti);
		const double TanH = FMath::Tan(FMath::DegreesToRadians(UnitFramingFov * 0.5));
		const double TanV = TanH / UnitFramingAspect;
		const double Prof = FMath::Max(OutDepth, 1e-3);
		return FVector2D(FVector::DotProduct(D, Destra) / (Prof * TanH), FVector::DotProduct(D, Su) / (Prof * TanV));
	}

	const ERTHexDirection UnitFramingAllFacings[] = {
		ERTHexDirection::E, ERTHexDirection::NE, ERTHexDirection::NW,
		ERTHexDirection::W, ERTHexDirection::SW, ERTHexDirection::SE };
}

/**
 * Dopo l'inquadratura lo yaw e' quello del verso, e il pivot sta DAVANTI all'unita' lungo il verso, per tutti e sei i versi.
 *
 * ⚠️ L'atteso viene da `FacingRotation`, cioe' dalla stessa funzione che la camera chiama: da solo sarebbe una
 * tautologia. Il controllo scritto a mano sono E e W, a 0 e a 180 gradi, e i sei yaw devono essere sei valori diversi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraUnitFramingFacingTest,
	"RefactorTactics.Camera.UnitFramingLooksAlongTheFacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraUnitFramingFacingTest::RunTest(const FString&)
{
	UWorld* World = MakeUnitFramingWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnUnitFramingMap(World, /*Radius=*/ 3);
	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam)) { DestroyUnitFramingWorld(World); return false; }
	Cam->SetViewportMetricsForTest(UnitFramingAspect, UnitFramingFov);
	Cam->SetUnitFramingForTest(/*Arm=*/ 600.f, /*LeadCells=*/ 0.75f);

	TSet<int32> YawVisti;
	for (const ERTHexDirection Verso : UnitFramingAllFacings)
	{
		const FString Nome = UEnum::GetValueAsString(Verso);
		ARTUnit* Unit = SpawnUnitFramingUnit(World, 0, UnitFramingMapCentre, Verso);
		if (!TestNotNull(*FString::Printf(TEXT("%s: unita'"), *Nome), Unit)) { DestroyUnitFramingWorld(World); return false; }

		TestTrue(*FString::Printf(TEXT("%s: inquadra"), *Nome), Cam->FrameUnit(Unit));
		const float Atteso = FRotator::ClampAxis(URTHexLibrary::FacingRotation(Verso).Yaw);
		TestTrue(*FString::Printf(TEXT("%s: lo yaw e' quello del verso (%.2f contro %.2f)"), *Nome, Cam->GetCameraYaw(), Atteso),
			FMath::IsNearlyEqual(Cam->GetCameraYaw(), Atteso, 0.01f));
		YawVisti.Add(FMath::RoundToInt(Cam->GetCameraYaw()));

		const FVector Piano = UnitFramingCellWorld(Map, UnitFramingMapCentre);
		const FVector Davanti = Piano + UnitFramingOneCellAhead(Map, UnitFramingMapCentre, Verso) * 0.75f;
		TestTrue(*FString::Printf(TEXT("%s: il pivot sta tre quarti di cella davanti all'unita'"), *Nome),
			FVector::Dist2D(Cam->GetCameraPivot(), Davanti) < 1.0);
		TestEqual(*FString::Printf(TEXT("%s: alla quota del piano"), *Nome), Cam->GetCameraPivot().Z, Piano.Z);
		Unit->Destroy();
	}
	TestEqual(TEXT("sei versi, sei yaw diversi"), YawVisti.Num(), 6);

	// Il controllo scritto a mano: E guarda lungo +X, W all'opposto.
	ARTUnit* Est = SpawnUnitFramingUnit(World, 0, UnitFramingMapCentre, ERTHexDirection::E);
	if (TestNotNull(TEXT("unita' verso E"), Est) && TestTrue(TEXT("E: inquadra"), Cam->FrameUnit(Est)))
	{
		TestTrue(TEXT("E vale 0 gradi"), FMath::IsNearlyEqual(Cam->GetCameraYaw(), 0.f, 0.01f));
		Est->Facing = ERTHexDirection::W;
		Cam->FrameUnit(Est);
		TestTrue(TEXT("W vale 180 gradi"), FMath::IsNearlyEqual(Cam->GetCameraYaw(), 180.f, 0.01f));
	}

	DestroyUnitFramingWorld(World);
	return true;
}

/**
 * L'unita' cade nella META' BASSA del quadro, centrata in orizzontale: e' cio' che D-618 chiede («in basso, campo davanti»).
 *
 * Il discriminante e' lo spostamento in avanti: con `UnitFramingLeadCells = 0` l'unita' sta al centro del quadro, ed e'
 * il controllo positivo che a portarla giu' sia lo spostamento e non l'inclinazione da sola. Poi la taratura di default
 * deve rispettare lo stesso contratto: e' `PROPOSED FOR PLAYTEST`, ma una taratura che mettesse l'unita' al centro o
 * fuori dal quadro non sarebbe piu' D-618.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraUnitFramingLowerHalfTest,
	"RefactorTactics.Camera.UnitFramingPutsTheUnitInTheLowerHalfOfTheFrame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraUnitFramingLowerHalfTest::RunTest(const FString&)
{
	UWorld* World = MakeUnitFramingWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnUnitFramingMap(World, /*Radius=*/ 3);
	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	ARTUnit* Unit = SpawnUnitFramingUnit(World, 0, UnitFramingMapCentre, ERTHexDirection::NE);
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam) || !TestNotNull(TEXT("unita'"), Unit))
	{
		DestroyUnitFramingWorld(World);
		return false;
	}
	Cam->SetViewportMetricsForTest(UnitFramingAspect, UnitFramingFov);
	const float ArmDiDefault = Cam->GetUnitFramingArmLength();
	const float AvantiDiDefault = Cam->GetUnitFramingLeadCells();
	const FVector Piedi = UnitFramingCellWorld(Map, UnitFramingMapCentre);

	// Il controllo positivo: senza spostamento l'unita' sta al centro del quadro.
	Cam->SetUnitFramingForTest(ArmDiDefault, /*LeadCells=*/ 0.f);
	Cam->FrameUnit(Unit);
	double Prof = 0.0;
	const FVector2D AlCentro = UnitFramingNdcOf(*Cam, Piedi, Prof);
	TestTrue(FString::Printf(TEXT("controllo: senza spostamento i piedi sono al centro (%.3f, %.3f)"), AlCentro.X, AlCentro.Y),
		AlCentro.GetAbsMax() < 0.01);

	// La taratura di default.
	Cam->SetUnitFramingForTest(ArmDiDefault, AvantiDiDefault);
	Cam->FrameUnit(Unit);
	const FVector2D InBasso = UnitFramingNdcOf(*Cam, Piedi, Prof);
	AddInfo(FString::Printf(TEXT("taratura di default (arm=%.0f, avanti=%.2f celle): piedi a NDC (%.3f, %.3f)"),
		ArmDiDefault, AvantiDiDefault, InBasso.X, InBasso.Y));
	TestTrue(TEXT("l'unita' sta davanti alla camera"), Prof > 0.0);
	TestTrue(TEXT("centrata in orizzontale"), FMath::Abs(InBasso.X) < 0.01);
	TestTrue(TEXT("nella meta' bassa del quadro"), InBasso.Y < 0.0);
	TestTrue(TEXT("e dentro il quadro"), InBasso.Y > -1.0);

	DestroyUnitFramingWorld(World);
	return true;
}

/**
 * Da qualunque vista di partenza, strategica compresa: distanza `UnitFramingArmLength`, pitch tarato, peek azzerato,
 * yaw del verso.
 *
 * 🔴 Il caso che discrimina e' l'uscita dalla strategica: D-499 riprende all'uscita l'imbardata tattica salvata
 * all'ingresso (qui 45), e senza la riga che la riscrive l'inquadratura tornerebbe girata come prima di `Tab`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraUnitFramingResetsViewTest,
	"RefactorTactics.Camera.UnitFramingResetsTheViewToItsOwnDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraUnitFramingResetsViewTest::RunTest(const FString&)
{
	UWorld* World = MakeUnitFramingWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnUnitFramingMap(World, /*Radius=*/ 3);
	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	ARTUnit* Unit = SpawnUnitFramingUnit(World, 0, UnitFramingMapCentre, ERTHexDirection::SW);
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam) || !TestNotNull(TEXT("unita'"), Unit))
	{
		DestroyUnitFramingWorld(World);
		return false;
	}
	Cam->SetArmLengthRangeForTest(/*Default=*/ 800.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);
	Cam->SetStrategicThresholdsForTest(/*Enter=*/ 2400.f, /*Exit=*/ 1900.f);
	Cam->SetUnitFramingForTest(/*Arm=*/ 600.f, /*LeadCells=*/ 0.75f);

	// Uno stato lontano da quello atteso: inclinata a mano, girata di 45, in strategica, con un peek.
	Cam->SetPitchForTest(/*Default=*/ -40.f, /*Current=*/ -70.f);
	Cam->AddYaw(+1.f);
	Cam->ToggleStrategicView();
	Cam->SetPeekOffset(FVector(60.f, 0.f, 0.f));
	if (!TestTrue(TEXT("precondizione: in strategica"), Cam->IsStrategicView())
		|| !TestFalse(TEXT("precondizione: c'e' un peek"), Cam->GetPeekOffset().IsNearlyZero()))
	{
		DestroyUnitFramingWorld(World);
		return false;
	}

	TestTrue(TEXT("inquadra"), Cam->FrameUnit(Unit));
	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { DestroyUnitFramingWorld(World); return false; }
	TestFalse(TEXT("la distanza fissa riporta in tattica"), Cam->IsStrategicView());
	TestEqual(TEXT("la distanza e' quella dell'inquadratura d'unita'"), Arm->TargetArmLength, 600.f);
	TestEqual(TEXT("l'inclinazione torna quella tarata"), Cam->GetCameraPitch(), -40.f);
	TestEqual(TEXT("senza offset manuale"), Cam->GetManualPitchOffset(), 0.f);
	TestTrue(TEXT("il peek e' azzerato"), Cam->GetPeekOffset().IsNearlyZero());
	const float Atteso = FRotator::ClampAxis(URTHexLibrary::FacingRotation(ERTHexDirection::SW).Yaw);
	TestTrue(FString::Printf(TEXT("lo yaw e' quello del verso, non il 45 salvato all'ingresso (%.2f contro %.2f)"),
		Cam->GetCameraYaw(), Atteso), FMath::IsNearlyEqual(Cam->GetCameraYaw(), Atteso, 0.01f));

	DestroyUnitFramingWorld(World);
	return true;
}

/**
 * Sul bordo il pivot si ferma al limite (D-525, D-526), e l'unita' resta nel quadro.
 *
 * L'unita' sta su una cella del bordo e guarda FUORI dalla mappa: il pivot voluto cade oltre le celle. Il margine e'
 * a zero perche' il limite morda gia' con lo spostamento di default (precondizione). Dentro il limite vuol dire che la
 * porta unica non lo sposta piu'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraUnitFramingEdgeTest,
	"RefactorTactics.Camera.UnitFramingAtTheEdgeKeepsThePivotInsideTheBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraUnitFramingEdgeTest::RunTest(const FString&)
{
	UWorld* World = MakeUnitFramingWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnUnitFramingMap(World, /*Radius=*/ 2);
	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	// (6,1) e' sul bordo della mappa di raggio 2 attorno a (4,1), e il suo vicino a E, (7,1), non c'e'.
	const FRTCellId Bordo(6, 1);
	ARTUnit* Unit = SpawnUnitFramingUnit(World, 0, Bordo, ERTHexDirection::E);
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam) || !TestNotNull(TEXT("unita'"), Unit))
	{
		DestroyUnitFramingWorld(World);
		return false;
	}
	if (!TestNotNull(TEXT("precondizione: la cella del bordo c'e'"), Map->MapAsset->FindCell(Bordo))
		|| !TestNull(TEXT("precondizione: oltre il bordo non c'e' niente"),
			Map->MapAsset->FindCell(URTHexLibrary::Neighbor(Bordo, ERTHexDirection::E))))
	{
		DestroyUnitFramingWorld(World);
		return false;
	}
	Cam->SetViewportMetricsForTest(UnitFramingAspect, UnitFramingFov);
	Cam->SetBoundsMarginForTest(0.f);

	TestTrue(TEXT("inquadra"), Cam->FrameUnit(Unit));
	const FVector Pivot = Cam->GetCameraPivot();
	const FVector Voluto = UnitFramingCellWorld(Map, Bordo)
		+ UnitFramingOneCellAhead(Map, Bordo, ERTHexDirection::E) * Cam->GetUnitFramingLeadCells();
	if (!TestTrue(TEXT("precondizione: il limite morde, il pivot non e' quello voluto"), FVector::Dist2D(Pivot, Voluto) > 1.0))
	{
		DestroyUnitFramingWorld(World);
		return false;
	}

	Cam->SetCameraPivot(Pivot);
	TestTrue(TEXT("il pivot sta dentro il limite: la porta unica non lo sposta piu'"),
		FVector::Dist2D(Cam->GetCameraPivot(), Pivot) < 0.5);
	double Prof = 0.0;
	const FVector2D Ndc = UnitFramingNdcOf(*Cam, UnitFramingCellWorld(Map, Bordo), Prof);
	AddInfo(FString::Printf(TEXT("sul bordo: piedi a NDC (%.3f, %.3f)"), Ndc.X, Ndc.Y));
	TestTrue(TEXT("l'unita' sta davanti alla camera"), Prof > 0.0);
	TestTrue(TEXT("e resta nel quadro"), Ndc.GetAbsMax() <= 1.0);

	DestroyUnitFramingWorld(World);
	return true;
}

/**
 * 🔴 In partita il tasto non inquadra mai un'unita' d'altri, ne' una caduta, ne' niente senza selezione, e senza
 * `ARTPlayerState` non indovina la squadra. In ognuno di quei casi la camera non si muove.
 *
 * Il controllo positivo e' l'ultimo caso: la stessa scena con un'unita' propria e viva, che si inquadra.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraUnitFramingGuardTest,
	"RefactorTactics.Camera.UnitFramingNeverFramesAnotherTeamsUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraUnitFramingGuardTest::RunTest(const FString&)
{
	const FRTCellId Lontano(2, 1);
	const FRTCellId Inizio(6, 0);

	auto Scena = [](int32 PlayerTeam, bool bConStato, UWorld*& OutWorld, ARTHexMapActor*& OutMap) -> ARTCameraPawn*
	{
		OutWorld = MakeUnitFramingWorld();
		OutMap = SpawnUnitFramingMap(OutWorld);
		ARTCameraPawn* Cam = OutWorld ? OutWorld->SpawnActor<ARTCameraPawn>() : nullptr;
		ARTPlayerController* PC = !OutWorld ? nullptr
			: bConStato ? RTWorldFixtures::MakePlayerOnTeam(OutWorld, PlayerTeam)
			: OutWorld->SpawnActor<ARTPlayerController>();
		if (!OutMap || !Cam || !PC) { return nullptr; }
		PC->Possess(Cam);
		Cam->SetViewportMetricsForTest(UnitFramingAspect, UnitFramingFov);
		return Cam;
	};

	struct FCaso { const TCHAR* Nome; int32 PlayerTeam; bool bConStato; int32 UnitTeam; bool bViva; bool bPassata; bool bAtteso; };
	const FCaso Casi[] = {
		{ TEXT("un nemico"),               0, true,  1, true,  true,  false },
		{ TEXT("un'unita' propria caduta"), 0, true,  0, false, true,  false },
		{ TEXT("nessuna selezione"),       0, true,  0, true,  false, false },
		{ TEXT("la squadra 0 per la 1"),   1, true,  0, true,  true,  false },
		{ TEXT("senza ARTPlayerState"),    0, false, 0, true,  true,  false },
		{ TEXT("controllo: propria e viva"), 0, true, 0, true, true,  true  },
	};
	for (const FCaso& C : Casi)
	{
		UWorld* World = nullptr;
		ARTHexMapActor* Map = nullptr;
		ARTCameraPawn* Cam = Scena(C.PlayerTeam, C.bConStato, World, Map);
		ARTUnit* Unit = Cam ? SpawnUnitFramingUnit(World, C.UnitTeam, Lontano, ERTHexDirection::NE) : nullptr;
		if (!TestNotNull(*FString::Printf(TEXT("%s: scena"), C.Nome), Unit)) { DestroyUnitFramingWorld(World); return false; }
		if (!C.bViva) { Unit->ApplyCombatState(/*NewHealth=*/ 0, /*NewShield=*/ 0); }

		Cam->SetCameraPivotForTest(UnitFramingCellWorld(Map, Inizio));
		const FVector Prima = Cam->GetCameraPivot();
		const float YawPrima = Cam->GetCameraYaw();

		const bool bInquadra = Cam->FrameOwnUnit(C.bPassata ? Unit : nullptr);
		TestTrue(*FString::Printf(TEXT("%s: l'esito e' %s"), C.Nome, C.bAtteso ? TEXT("vero") : TEXT("falso")), bInquadra == C.bAtteso);
		if (C.bAtteso)
		{
			TestTrue(*FString::Printf(TEXT("%s: la camera si e' mossa"), C.Nome), FVector::Dist2D(Cam->GetCameraPivot(), Prima) > 1.0);
		}
		else
		{
			TestTrue(*FString::Printf(TEXT("%s: il pivot non si muove"), C.Nome), FVector::Dist2D(Cam->GetCameraPivot(), Prima) < 0.5);
			TestEqual(*FString::Printf(TEXT("%s: e la vista non gira"), C.Nome), Cam->GetCameraYaw(), YawPrima);
		}
		DestroyUnitFramingWorld(World);
	}
	return true;
}

/**
 * Il tasto e' `T`, sul contesto di input reale, e passa dal gestore vero: la selezione propria si inquadra; in uno
 * scenario che non e' del banco il tasto non muove niente (il controllo e' la stessa scena senza scenario).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerInputUnitFramingKeyTest,
	"RefactorTactics.PlayerInput.UnitFramingIsOnT",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerInputUnitFramingKeyTest::RunTest(const FString&)
{
	UWorld* World = MakeUnitFramingWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnUnitFramingMap(World);
	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, 0);
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam) || !TestNotNull(TEXT("controller"), PC)
		|| !TestNotNull(TEXT("game mode"), GameMode))
	{
		DestroyUnitFramingWorld(World);
		return false;
	}
	PC->Possess(Cam);
	Cam->SetViewportMetricsForTest(UnitFramingAspect, UnitFramingFov);

	// Il tasto, sul contesto vero.
	const UInputMappingContext* Imc = PC->BuildAndGetMappingContextForTest();
	if (!TestNotNull(TEXT("mapping context"), Imc)) { DestroyUnitFramingWorld(World); return false; }
	TArray<FKey> Tasti;
	for (const FEnhancedActionKeyMapping& M : Imc->GetMappings())
	{
		if (M.Action && M.Action->GetName() == TEXT("IA_FrameUnit")) { Tasti.Add(M.Key); }
	}
	TestEqual(TEXT("IA_FrameUnit ha esattamente un tasto"), Tasti.Num(), 1);
	if (Tasti.Num() == 1)
	{
		TestEqual(TEXT("ed e' UnitFramingHotkey()"), Tasti[0], RTInputKeys::UnitFramingHotkey());
	}
	TestEqual(TEXT("che e' T"), RTInputKeys::UnitFramingHotkey(), EKeys::T);

	// Il gestore vero, con la selezione del controller.
	const FRTCellId Cella(2, 1);
	ARTUnit* Selected = SpawnUnitFramingUnit(World, 0, Cella, ERTHexDirection::NW);
	if (!TestNotNull(TEXT("unita'"), Selected)) { DestroyUnitFramingWorld(World); return false; }
	PC->SelectUnit(Selected, /*bRecordAsPlayerInput=*/ false);
	UEnhancedInputComponent* Input = RTInputBindingFixtures::BindInput(*PC);

	RTInputBindingFixtures::Fire(*Input, TEXT("IA_FrameUnit"), ETriggerEvent::Started);
	const float Atteso = FRotator::ClampAxis(URTHexLibrary::FacingRotation(ERTHexDirection::NW).Yaw);
	TestTrue(TEXT("T gira la vista sul verso della selezione"), FMath::IsNearlyEqual(Cam->GetCameraYaw(), Atteso, 0.01f));
	TestTrue(TEXT("e porta il pivot davanti a lei"), FVector::Dist2D(Cam->GetCameraPivot(),
		UnitFramingCellWorld(Map, Cella) + UnitFramingOneCellAhead(Map, Cella, ERTHexDirection::NW) * Cam->GetUnitFramingLeadCells()) < 1.0);

	// In uno scenario che non e' del banco: osservazione, e niente si muove.
	GameMode->ScenarioToRun = TEXT("Visual.Camera.HomeTest");
	if (!TestTrue(TEXT("precondizione: e' un'osservazione"), PC->IsObservationSession()))
	{
		DestroyUnitFramingWorld(World);
		return false;
	}
	Cam->RecenterView();
	const FVector Prima = Cam->GetCameraPivot();
	RTInputBindingFixtures::Fire(*Input, TEXT("IA_FrameUnit"), ETriggerEvent::Started);
	TestTrue(TEXT("in osservazione fuori dal banco T non muove la camera"), FVector::Dist2D(Cam->GetCameraPivot(), Prima) < 0.5);
	TestEqual(TEXT("ne' la gira"), Cam->GetCameraYaw(), 0.f);

	DestroyUnitFramingWorld(World);
	return true;
}

/**
 * L'apertura di uno scenario (D-618 (c)): una scena del banco si apre dietro chi lancia; ogni altro scenario, e il banco
 * senza chi lancia, sulla mappa intera.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraScenarioOpeningLabCasterTest,
	"RefactorTactics.Camera.ScenarioOpeningFramesTheLabCaster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraScenarioOpeningLabCasterTest::RunTest(const FString&)
{
	UWorld* World = MakeUnitFramingWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnUnitFramingMap(World);
	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	// Fuori dal centro della mappa, perche' «sulla mappa intera» e «su chi lancia» siano due punti diversi.
	const FRTCellId Cella(3, 2);
	ARTUnit* Caster = SpawnUnitFramingUnit(World, 0, Cella, ERTHexDirection::NE);
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam) || !TestNotNull(TEXT("chi lancia"), Caster))
	{
		DestroyUnitFramingWorld(World);
		return false;
	}
	Cam->SetViewportMetricsForTest(UnitFramingAspect, UnitFramingFov);
	const FString Banco = TEXT("AbilityLab.Hero.Gadget.LinearDischarge");
	if (!TestTrue(TEXT("precondizione: l'id e' una scena del banco"), URTAbilityLabLibrary::IsLabScenarioId(Banco)))
	{
		DestroyUnitFramingWorld(World);
		return false;
	}
	const FVector CentroMappa = UnitFramingCellWorld(Map, UnitFramingMapCentre);
	const FVector DavantiAChiLancia = UnitFramingCellWorld(Map, Cella)
		+ UnitFramingOneCellAhead(Map, Cella, ERTHexDirection::NE) * Cam->GetUnitFramingLeadCells();

	TestTrue(TEXT("banco: dietro chi lancia"), ARTGameMode::FrameScenarioOpening(*Cam, Banco, Caster));
	TestTrue(TEXT("banco: il pivot sta davanti a chi lancia"), FVector::Dist2D(Cam->GetCameraPivot(), DavantiAChiLancia) < 1.0);
	TestTrue(TEXT("banco: girata sul suo verso"), FMath::IsNearlyEqual(Cam->GetCameraYaw(),
		FRotator::ClampAxis(URTHexLibrary::FacingRotation(ERTHexDirection::NE).Yaw), 0.01f));

	TestFalse(TEXT("altro scenario: la mappa intera"), ARTGameMode::FrameScenarioOpening(*Cam, TEXT("Visual.Camera.HomeTest"), Caster));
	TestTrue(TEXT("altro scenario: il pivot sta sul centro mappa"), FVector::Dist2D(Cam->GetCameraPivot(), CentroMappa) < 1.0);
	TestEqual(TEXT("altro scenario: orientamento di default"), Cam->GetCameraYaw(), 0.f);

	Cam->FrameUnit(Caster);
	TestFalse(TEXT("banco senza chi lancia: la mappa intera"), ARTGameMode::FrameScenarioOpening(*Cam, Banco, nullptr));
	TestTrue(TEXT("banco senza chi lancia: il pivot sta sul centro mappa"), FVector::Dist2D(Cam->GetCameraPivot(), CentroMappa) < 1.0);

	DestroyUnitFramingWorld(World);
	return true;
}

/**
 * La sessione ritrova per id le unita' che ha posato: e' la porta da cui la camera del banco prende chi lancia.
 *
 * La scena e' quella vera del banco (`BuildFixture`), cosi' l'id e' quello che `BuildFixture` scrive e non uno scritto qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioSessionFindsLabCasterTest,
	"RefactorTactics.Scenario.SessionFindsTheLabCasterById",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioSessionFindsLabCasterTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	FString Errore;
	if (!TestTrue(TEXT("la scena del banco si costruisce"), URTAbilityLabLibrary::BuildFixture(
		TEXT("Hero.Gadget.LinearDischarge"), FRTAbilityLabFixtureSpec(), Scenario, Errore)))
	{
		AddError(Errore);
		return false;
	}

	UWorld* World = MakeUnitFramingWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	FRTScenarioSession Session;
	TestNull(TEXT("prima della partenza non c'e' nessuno"), Session.FindUnit(URTAbilityLabLibrary::CasterUnitId()));
	if (!TestTrue(TEXT("la sessione parte"), Session.Start(World, Scenario)))
	{
		AddError(Session.GetResult().ErrorMessage);
		DestroyUnitFramingWorld(World);
		return false;
	}

	const ARTUnit* Caster = Session.FindUnit(URTAbilityLabLibrary::CasterUnitId());
	const ARTUnit* Target = Session.FindUnit(TEXT("TARGET"));
	if (TestNotNull(TEXT("chi lancia c'e'"), Caster))
	{
		TestTrue(TEXT("sulla cella della posa"), Caster->Cell == FRTAbilityLabFixtureSpec().CasterCell);
		TestEqual(TEXT("nella squadra 0"), Caster->TeamId, 0);
	}
	TestTrue(TEXT("il bersaglio e' un'altra unita'"), Target != nullptr && Target != Caster);
	TestNull(TEXT("un id che lo scenario non dichiara non trova niente"), Session.FindUnit(TEXT("NESSUNO")));

	DestroyUnitFramingWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
