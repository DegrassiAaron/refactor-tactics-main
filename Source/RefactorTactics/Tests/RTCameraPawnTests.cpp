// Camera tattica: rotazione della vista e scorrimento RELATIVO a dove si guarda.
//
// Il difetto che questi test chiudono non e' «manca la rotazione»: e' che aggiungerla, da sola, avrebbe
// rotto il pan. Con lo scorrimento ancorato agli assi del mondo — com'era — dopo aver girato la vista di 90
// gradi premere «avanti» avrebbe spostato l'inquadratura di lato. Le due cose sono una sola.

#include "Misc/AutomationTest.h"
#include "Camera/RTCameraPawn.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	UWorld* MakeCameraWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyCameraWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}
}

/**
 * Ruotare cambia la direzione di sguardo, a scatti, e il valore resta leggibile.
 *
 * La normalizzazione non e' cosmetica: senza, ruotando a lungo nella stessa direzione il campo cresce senza
 * fine e in editor si legge `1035` per una vista identica a `315`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraYawStepsTest,
	"RefactorTactics.Camera.YawTurnsInStepsAndStaysNormalized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraYawStepsTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	// `YawStep` resta protected: i test usano il DEFAULT del gioco (45) invece di forzarlo. Se qualcuno lo
	// cambiasse, e' giusto che questi test lo notino — sono anche la documentazione di quanto gira un tasto.
	TestEqual(TEXT("si parte da zero"), Cam->GetCameraYaw(), 0.f);

	Cam->AddYaw(+1.f);
	TestEqual(TEXT("un passo orario"), Cam->GetCameraYaw(), 45.f);

	Cam->AddYaw(-1.f);
	TestEqual(TEXT("indietro allo zero"), Cam->GetCameraYaw(), 0.f);

	// Sotto zero deve avvolgersi, non diventare negativo.
	Cam->AddYaw(-1.f);
	TestEqual(TEXT("sotto zero si avvolge a 315"), Cam->GetCameraYaw(), 315.f);

	// Giro completo: otto passi da 45 tornano al punto di partenza, non a 360.
	for (int32 I = 0; I < 8; ++I) { Cam->AddYaw(+1.f); }
	TestEqual(TEXT("un giro intero torna a 315, non a 675"), Cam->GetCameraYaw(), 315.f);

	// Un input nullo non e' una rotazione: senza questa guardia un asse a riposo girerebbe la vista.
	const float Before = Cam->GetCameraYaw();
	Cam->AddYaw(0.f);
	TestEqual(TEXT("input a zero non ruota"), Cam->GetCameraYaw(), Before);

	DestroyCameraWorld(World);
	return true;
}

/**
 * Lo ZOOM sopravvive alla rotazione.
 *
 * Difetto evitato per un soffio scrivendo il codice: `ApplyViewSettings()` riporta il braccio a
 * `DefaultArmLength`, quindi chiamarla per aggiornare la rotazione avrebbe annullato lo zoom a ogni
 * pressione di E. Sono due responsabilita' che vivevano nella stessa funzione perche' finora nessuno
 * cambiava l'inquadratura senza volerla anche ripristinare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraYawKeepsZoomTest,
	"RefactorTactics.Camera.RotatingDoesNotResetZoom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraYawKeepsZoomTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { DestroyCameraWorld(World); return false; }

	const float BeforeZoom = Arm->TargetArmLength;
	Cam->AddZoom(+1.f);
	const float Zoomed = Arm->TargetArmLength;
	if (!TestNotEqual(TEXT("lo zoom ha cambiato la distanza"), Zoomed, BeforeZoom))
	{
		DestroyCameraWorld(World);
		return false;
	}

	Cam->AddYaw(+1.f);
	TestEqual(TEXT("ruotare non tocca la distanza"), Arm->TargetArmLength, Zoomed);
	TestEqual(TEXT("ma la rotazione e' applicata al braccio"),
		static_cast<float>(Arm->GetRelativeRotation().Yaw), Cam->GetCameraYaw());

	DestroyCameraWorld(World);
	return true;
}

/**
 * Lo SCORRIMENTO segue la vista: «avanti» e' avanti sullo schermo, non sulla mappa.
 *
 * E' la meta' che rende utilizzabile la rotazione, e la ragione per cui non bastava aggiungere un tasto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraPanFollowsYawTest,
	"RefactorTactics.Camera.PanIsRelativeToTheView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraPanFollowsYawTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	// Yaw 0: il comportamento STORICO non cambia — «avanti» e' +X nel mondo. Se questa riga cadesse, la
	// modifica avrebbe cambiato la guida di ogni inquadratura esistente, non solo di quelle ruotate.
	Cam->SetActorLocation(FVector::ZeroVector);
	Cam->AddPlanarMovement(FVector2D(0.f, 1.f));
	const FVector Straight = Cam->GetActorLocation();
	TestTrue(TEXT("yaw 0: avanti e' +X"), Straight.X > 1.f && FMath::Abs(Straight.Y) < 0.01f);

	// Yaw 90: lo STESSO input deve andare da un'altra parte nel mondo — perche' sullo schermo va nello
	// stesso posto. E' esattamente cio' che il pan in assi mondo non faceva.
	// Due passi da 45 fanno 90: l'angolo comodo per il test si ottiene dai passi veri, non riconfigurandoli.
	Cam->AddYaw(+1.f);
	Cam->AddYaw(+1.f);
	TestEqual(TEXT("ruotata di 90"), Cam->GetCameraYaw(), 90.f);
	Cam->SetActorLocation(FVector::ZeroVector);
	Cam->AddPlanarMovement(FVector2D(0.f, 1.f));
	const FVector Turned = Cam->GetActorLocation();
	TestTrue(TEXT("yaw 90: lo stesso 'avanti' va su +Y"), Turned.Y > 1.f && FMath::Abs(Turned.X) < 0.01f);

	// La LUNGHEZZA dello spostamento non cambia con l'angolo: ruotare non deve far scorrere piu' veloce.
	TestTrue(TEXT("stessa distanza percorsa"), FMath::IsNearlyEqual(Straight.Size(), Turned.Size(), 0.01f));

	DestroyCameraWorld(World);
	return true;
}

/** `Home` riporta a un'inquadratura NOTA: posizione, zoom **e** rotazione. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraRecenterResetsYawTest,
	"RefactorTactics.Camera.RecenterAlsoResetsRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraRecenterResetsYawTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	Cam->AddYaw(+1.f);
	Cam->AddYaw(+1.f);
	if (!TestNotEqual(TEXT("la vista e' girata"), Cam->GetCameraYaw(), 0.f))
	{
		DestroyCameraWorld(World);
		return false;
	}

	Cam->RecenterView();
	TestEqual(TEXT("Home rimette la vista dritta"), Cam->GetCameraYaw(), 0.f);

	// Il pan torna a comportarsi come all'inizio: e' la prova che il reset e' arrivato fino alla guida, non
	// solo al numero.
	Cam->SetActorLocation(FVector::ZeroVector);
	Cam->AddPlanarMovement(FVector2D(0.f, 1.f));
	TestTrue(TEXT("dopo Home 'avanti' e' di nuovo +X"), Cam->GetActorLocation().X > 1.f);

	DestroyCameraWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
