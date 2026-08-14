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
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Player/RTPlayerController.h"
#include "Unit/RTUnit.h"

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

	/**
	 * Mappa piatta minima. Il raggio 2 non e' decorativo: `RecenterView` centra su `GetCenterCell()`, quindi
	 * senza celle il confronto con l'inquadratura di squadra perderebbe il proprio riferimento.
	 */
	ARTHexMapActor* SpawnCameraTestMap(UWorld* World)
	{
		if (!World) { return nullptr; }

		URTHexMapAsset* Asset = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), /*Radius=*/ 2))
		{
			Asset->AddOrUpdateCell(FRTHexCellData(Id));
		}
		Asset->SortCells();

		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor) { MapActor->MapAsset = Asset; }
		return MapActor;
	}

	/**
	 * Unita' viva sulla cella data. `Health` parte a 100 dal default della classe, quindi `IsAlive()` e' gia'
	 * vero senza configurare un eroe: questi test guardano la CAMERA, e un roster completo introdurrebbe
	 * dipendenze che non c'entrano con l'inquadratura.
	 */
	ARTUnit* SpawnCameraTestUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->Cell = Cell;
		U->FinishSpawning(FTransform::Identity);
		return U;
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

/**
 * `FocusOn` CENTRA e basta: quota, zoom e orientamento restano quelli che il giocatore si era regolato.
 *
 * L'header lo promette — «mantenendo la quota e lo zoom correnti» — e finora nessun test lo difendeva.
 * Sono tre proprieta' indipendenti che vivono in una riga sola (`SetActorLocation` con la Z presa da se'):
 * la prima modifica che passasse la Z del bersaglio le romperebbe tutte e tre senza far cadere niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraFocusKeepsFramingTest,
	"RefactorTactics.Camera.FocusKeepsHeightZoomAndOrientation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraFocusKeepsFramingTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { DestroyCameraWorld(World); return false; }

	// Uno stato «regolato dal giocatore»: girato e zoomato, a una quota sua.
	const float ZoomAtRest = Arm->TargetArmLength;
	Cam->AddYaw(+1.f);
	Cam->AddZoom(+1.f);
	Cam->SetActorLocation(FVector(0.f, 0.f, 1234.f));
	const float ZoomBefore = Arm->TargetArmLength;
	const float YawBefore = Cam->GetCameraYaw();
	const float PitchBefore = static_cast<float>(Arm->GetRelativeRotation().Pitch);

	// La premessa va DIMOSTRATA, non assunta: se `AddZoom`/`AddYaw` regredissero a no-op, i valori
	// «prima» sarebbero i default e le tre verifiche «non si tocca» passerebbero senza provare niente.
	// E' la stessa guardia che `RotatingDoesNotResetZoom` mette prima di procedere.
	if (!TestNotEqual(TEXT("lo zoom e' stato davvero cambiato"), ZoomBefore, ZoomAtRest)
		|| !TestNotEqual(TEXT("la vista e' stata davvero girata"), YawBefore, 0.f))
	{
		DestroyCameraWorld(World);
		return false;
	}

	// La Z del bersaglio e' deliberatamente assurda: se `FocusOn` la usasse, la camera finirebbe li'.
	Cam->FocusOn(FVector(500.f, -300.f, 99999.f));

	// Letterali `double` senza suffisso: da LWC le componenti di `FVector` sono `FVector::FReal` (double),
	// e un `500.f` rende `TestEqual` ambiguo fra l'overload float e quello double (`error C2666`).
	const FVector After = Cam->GetActorLocation();
	TestEqual(TEXT("centra sulla X del bersaglio"), After.X, 500.0);
	TestEqual(TEXT("centra sulla Y del bersaglio"), After.Y, -300.0);
	TestEqual(TEXT("la quota resta la sua, non quella del bersaglio"), After.Z, 1234.0);

	TestEqual(TEXT("lo zoom non si tocca"), Arm->TargetArmLength, ZoomBefore);
	TestEqual(TEXT("la rotazione non si tocca"), Cam->GetCameraYaw(), YawBefore);
	TestEqual(TEXT("nemmeno l'inclinazione"),
		static_cast<float>(Arm->GetRelativeRotation().Pitch), PitchBefore);
	// ⚠️ Il campo del pawn e il BRACCIO sono due cose diverse, e guardare solo il primo lascia passare
	// un `FocusOn` che raddrizzi la vista: `CameraYaw` resterebbe 45 mentre lo schermo torna a 0, e il pan
	// — che legge il campo — si sfaserebbe da quello che il giocatore vede.
	TestEqual(TEXT("e il braccio guarda ancora dove dice il campo"),
		static_cast<float>(Arm->GetRelativeRotation().Yaw), YawBefore);

	DestroyCameraWorld(World);
	return true;
}

/**
 * `FrameOwnTeam` ha DUE rami che rispondono `false`, e in partita non se ne vede nessuno.
 *
 * Il contratto e' scritto nell'header — «Ritorna falso se non c'e' nulla da inquadrare … cosi' il chiamante
 * puo' riprovare o ripiegare su `RecenterView`» — ed e' l'unico modo che il chiamante ha di sapere che
 * l'inquadratura non e' avvenuta. Un ramo che nessuno esercita e' un ramo che nessuno sa se funziona.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraFrameTeamFalseBranchesTest,
	"RefactorTactics.Camera.FrameOwnTeamReportsWhenThereIsNothingToFrame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraFrameTeamFalseBranchesTest::RunTest(const FString&)
{
	// Ramo 1: nessuna mappa nel mondo.
	{
		UWorld* World = MakeCameraWorld();
		if (!TestNotNull(TEXT("world"), World)) { return false; }

		ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
		if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

		Cam->SetActorLocation(FVector(77.f, 88.f, 99.f));
		TestFalse(TEXT("senza mappa non c'e' niente da inquadrare"), Cam->FrameOwnTeam());
		// Un `false` che avesse gia' mosso la camera sarebbe peggio di un crash: il chiamante ripiega su
		// `RecenterView` credendo di partire da fermo.
		TestEqual(TEXT("e la camera non si e' mossa"), Cam->GetActorLocation(), FVector(77.f, 88.f, 99.f));

		DestroyCameraWorld(World);
	}

	// Ramo 2: mappa presente, ma nessuna unita' VIVA della propria squadra.
	{
		UWorld* World = MakeCameraWorld();
		if (!TestNotNull(TEXT("world (2)"), World)) { return false; }

		ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
		if (!TestNotNull(TEXT("camera (2)"), Cam)) { DestroyCameraWorld(World); return false; }
		if (!TestNotNull(TEXT("mappa"), SpawnCameraTestMap(World))) { DestroyCameraWorld(World); return false; }

		// Senza PlayerController la squadra assunta e' la 0: queste due unita' non la compongono.
		// Il nemico VIVO e' il discriminante del test — senza di lui questo sarebbe «un mondo con una
		// mappa e un cadavere» — quindi va verificato che esista, non solo spawnato.
		ARTUnit* Enemy = SpawnCameraTestUnit(World, /*TeamId=*/ 1, FRTCellId(1, 0));
		if (!TestNotNull(TEXT("nemico vivo"), Enemy)) { DestroyCameraWorld(World); return false; }
		ARTUnit* Fallen = SpawnCameraTestUnit(World, /*TeamId=*/ 0, FRTCellId(0, 1));
		if (!TestNotNull(TEXT("unita' della squadra 0"), Fallen)) { DestroyCameraWorld(World); return false; }
		// Uccisa con l'API di gioco, non azzerando il campo: e' `IsAlive()` che il filtro interroga.
		Fallen->ApplyCombatState(/*NewHealth=*/ 0, /*NewShield=*/ 0);

		Cam->SetActorLocation(FVector(11.f, 22.f, 33.f));
		TestFalse(TEXT("un nemico vivo e un compagno morto non fanno un'inquadratura"), Cam->FrameOwnTeam());
		TestEqual(TEXT("e la camera non si e' mossa"), Cam->GetActorLocation(), FVector(11.f, 22.f, 33.f));

		DestroyCameraWorld(World);
	}

	return true;
}

/**
 * L'inquadratura d'inizio partita e' PIU' VICINA di `Home`, ed e' una scelta, non un caso.
 *
 * `MatchStartArmLength` e `DefaultArmLength` sono due campi distinti perche' rispondono a due domande
 * diverse — «dove sono le mie unita'» e «dov'e' la mappa». Sono `protected`, quindi il test non li legge:
 * confronta i due comportamenti osservabili, che e' anche l'unico modo in cui la differenza conta davvero.
 * Se qualcuno unificasse i due campi, qui cadrebbe una riga sola e direbbe esattamente cosa e' successo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraFrameTeamZoomTest,
	"RefactorTactics.Camera.FrameOwnTeamZoomsCloserThanRecenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraFrameTeamZoomTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { DestroyCameraWorld(World); return false; }
	if (!TestNotNull(TEXT("mappa"), SpawnCameraTestMap(World))) { DestroyCameraWorld(World); return false; }

	// ⚠️ Celle ASIMMETRICHE rispetto al centro mappa. Con `(-1,0)` e `(1,0)` il centroide cadrebbe
	// sull'origine, cioe' esattamente dove `RecenterView` ha appena messo la camera: il confronto di
	// posizione sarebbe vero per coincidenza e non proverebbe nulla.
	SpawnCameraTestUnit(World, /*TeamId=*/ 0, FRTCellId(1, 0));
	SpawnCameraTestUnit(World, /*TeamId=*/ 0, FRTCellId(2, 0));

	Cam->RecenterView();
	const float HomeArm = Arm->TargetArmLength;
	const FVector HomeLocation = Cam->GetActorLocation();

	if (!TestTrue(TEXT("con due unita' vive l'inquadratura riesce"), Cam->FrameOwnTeam()))
	{
		DestroyCameraWorld(World);
		return false;
	}
	const float MatchArm = Arm->TargetArmLength;

	TestTrue(TEXT("inizio partita e' piu' vicino di Home"), MatchArm < HomeArm);

	// L'altra meta' del contratto — «centra sul punto medio delle sue unita'» — che senza questa riga
	// nessun test toccava: cancellando la chiamata a `CellsCentroidWorld` la suite restava verde e la
	// camera avrebbe smesso in silenzio di inquadrare la squadra all'avvio.
	TestNotEqual(TEXT("si e' spostata dal centro mappa verso le proprie unita'"),
		Cam->GetActorLocation(), HomeLocation);

	// E lo zoom non e' un effetto collaterale di `FocusOn`, che per contratto non lo tocca: e'
	// `FrameOwnTeam` a riscriverlo dopo. Le due responsabilita' restano distinte.
	Cam->FocusOn(FVector(999.f, 999.f, 0.f));
	TestEqual(TEXT("centrare altrove non cambia la distanza appena scelta"), Arm->TargetArmLength, MatchArm);

	DestroyCameraWorld(World);
	return true;
}

/**
 * «La propria squadra» e' quella del CONTROLLER, non la 0.
 *
 * `FrameOwnTeam` legge `PlayerTeamId` e ripiega sulla squadra 0 solo quando un controller non c'e' — una
 * comodita' da demo. Senza questo test la lettura del controller e' cancellabile senza far cadere niente, e
 * in una partita in cui il giocatore e' la squadra 1 la camera aprirebbe inquadrando **il nemico**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraFrameTeamFollowsControllerTest,
	"RefactorTactics.Camera.FrameOwnTeamFramesTheControllersTeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraFrameTeamFollowsControllerTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	if (!TestNotNull(TEXT("mappa"), SpawnCameraTestMap(World))) { DestroyCameraWorld(World); return false; }

	// Due squadre su lati opposti: quale delle due si inquadra e' l'intera domanda del test.
	SpawnCameraTestUnit(World, /*TeamId=*/ 0, FRTCellId(-2, 0));
	SpawnCameraTestUnit(World, /*TeamId=*/ 1, FRTCellId(2, 0));

	// Senza controller: ripiego sulla squadra 0.
	if (!TestTrue(TEXT("senza controller inquadra comunque"), Cam->FrameOwnTeam()))
	{
		DestroyCameraWorld(World);
		return false;
	}
	const FVector FramedTeamZero = Cam->GetActorLocation();

	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("controller"), PC)) { DestroyCameraWorld(World); return false; }
	PC->PlayerTeamId = 1;
	PC->Possess(Cam);

	if (!TestTrue(TEXT("con il controller inquadra"), Cam->FrameOwnTeam()))
	{
		DestroyCameraWorld(World);
		return false;
	}

	// Se la lettura di `PlayerTeamId` sparisse, questa resterebbe l'inquadratura della squadra 0 e le due
	// posizioni coinciderebbero.
	TestNotEqual(TEXT("la squadra 1 non si inquadra dove sta la squadra 0"),
		Cam->GetActorLocation(), FramedTeamZero);

	DestroyCameraWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
