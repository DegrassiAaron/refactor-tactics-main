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
#include "TimerManager.h"
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
	 *
	 * ⚠️ **Il default NON e' l'origine, ed e' deliberato.** Una mappa centrata in `(0,0)` rende degeneri i
	 * confronti di posizione: «dove `RecenterView` mette la camera» e «l'origine del mondo» diventano lo
	 * stesso punto, e un'assertion che li distingue passa per coincidenza. La prima stesura lasciava
	 * `(0,0,0)` come default e affidava a questo commento il compito di avvisare — cioe' teneva la trappola
	 * armata per ogni chiamante futuro che il commento non lo avesse letto. Ora il caso pericoloso si
	 * ottiene solo chiedendolo.
	 */
	ARTHexMapActor* SpawnCameraTestMap(UWorld* World, const FRTCellId& Center = FRTCellId(4, 1, 0))
	{
		if (!World) { return nullptr; }

		URTHexMapAsset* Asset = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(Center, /*Radius=*/ 2))
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
 * La rotazione CONTINUA si ferma dove il giocatore la lascia, e convive con lo scatto.
 *
 * Prima di `#863` la vista aveva otto orientamenti: guardare *fra* due file di celle — il caso d'uso per
 * cui `D-142` tiene lo step a 45° e non a un divisore di 60 — era impossibile proprio con lo strumento
 * che quella decisione motiva.
 *
 * ⚠️ I due gesti scrivono lo **stesso** stato: e' cio' che rende `Q` dopo un trascinamento un passo
 * avanti, e non un salto verso una griglia di angoli parallela.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraContinuousYawTest,
	"RefactorTactics.Camera.ContinuousYawStopsWhereItIsLeftAndKeepsTheSnap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraContinuousYawTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { DestroyCameraWorld(World); return false; }

	// ⚠️ Sensibilita' FISSATA dal test: senza, `AddYawContinuous(20)` pinnerebbe implicitamente il default
	// `0.5`, che questa issue dichiara taratura aperta da playtest. Il test cadrebbe per una decisione di
	// tuning invece che per un difetto.
	Cam->SetSensitivitiesForTest(/*Yaw=*/ 1.f, /*Pitch=*/ 1.f);

	// Un trascinamento che NON e' un multiplo dello step: e' esattamente cio' che prima non si poteva fare.
	Cam->AddYawContinuous(20.f);
	const float Free = Cam->GetCameraYaw();
	TestTrue(TEXT("la vista si e' mossa"), Free > 0.f);
	TestFalse(TEXT("e si e' fermata FRA due passi canonici, non su uno di essi"),
		FMath::IsNearlyZero(FMath::Fmod(Free, 45.f), 0.01f));
	TestEqual(TEXT("il braccio segue il campo"),
		static_cast<float>(Arm->GetRelativeRotation().Yaw), Free);

	// Lo scatto riparte da li': stesso stato, non due sistemi che si contendono la vista.
	// `ClampAxis` restituisce `double` (LWC), e `GetCameraYaw` un `float`: il cast rende esplicito quale
	// overload di `TestEqual` si sta chiedendo, invece di lasciarlo ambiguo (`error C2666`).
	Cam->AddYaw(+1.f);
	TestEqual(TEXT("Q/E fa un passo DA dove il trascinamento ha lasciato"),
		Cam->GetCameraYaw(), static_cast<float>(FRotator::ClampAxis(Free + 45.f)));

	// Input nullo: nessuna rotazione, come per lo scatto.
	const float Before = Cam->GetCameraYaw();
	Cam->AddYawContinuous(0.f);
	TestEqual(TEXT("input a zero non ruota"), Cam->GetCameraYaw(), Before);

	// E resta normalizzato: e' la ragione per cui lo scatto usa `ClampAxis`, e vale identica qui.
	for (int32 I = 0; I < 40; ++I) { Cam->AddYawContinuous(30.f); }
	TestTrue(TEXT("dopo molti giri il valore resta leggibile"),
		Cam->GetCameraYaw() >= 0.f && Cam->GetCameraYaw() < 360.f);

	DestroyCameraWorld(World);
	return true;
}

/**
 * L'orbita mappa X sullo yaw e Y sul pitch, e il gesto va ARMATO.
 *
 * ⚠️ Senza questo test, scambiare `Delta.X` con `Delta.Y` nel controller non farebbe cadere niente: i
 * test sul pawn verificano `AddYawContinuous` e `AddPitch`, non **chi li chiama con cosa**. E' lo stesso
 * buco che la review di `#887` ha trovato sul percorso del tasto `F`.
 *
 * ➕ Il **verso** verticale non e' pinnato qui di proposito: e' `bInvertOrbitPitch`, una preferenza il cui
 * default non e' stato verificato con le mani. Il test verifica che la Y muova il pitch, non in che senso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraOrbitAxesTest,
	"RefactorTactics.Camera.OrbitMapsXToYawAndYToPitchWhenArmed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraOrbitAxesTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	Cam->SetSensitivitiesForTest(/*Yaw=*/ 1.f, /*Pitch=*/ 1.f);

	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("controller"), PC)) { DestroyCameraWorld(World); return false; }
	PC->Possess(Cam);

	const float Yaw0 = Cam->GetCameraYaw();
	const float Pitch0 = Cam->GetCameraPitch();

	// DISARMATO: il mouse si muove ma nessuno sta orbitando. Senza questa guardia la vista ruoterebbe
	// di continuo, e nessun tasto lo avrebbe chiesto.
	PC->SetOrbitingForTest(false);
	PC->OrbitCameraForTest(FVector2D(30.f, 20.f));
	TestEqual(TEXT("disarmato: lo yaw non si muove"), Cam->GetCameraYaw(), Yaw0);
	TestEqual(TEXT("disarmato: il pitch non si muove"), Cam->GetCameraPitch(), Pitch0);

	// ARMATO, solo X: muove lo yaw e **non** il pitch.
	PC->SetOrbitingForTest(true);
	PC->OrbitCameraForTest(FVector2D(30.f, 0.f));
	TestNotEqual(TEXT("X muove lo yaw"), Cam->GetCameraYaw(), Yaw0);
	TestEqual(TEXT("e X non tocca il pitch"), Cam->GetCameraPitch(), Pitch0);

	// Solo Y: muove il pitch e **non** lo yaw. E' la coppia che rende lo scambio degli assi rilevabile.
	const float YawAfterX = Cam->GetCameraYaw();
	PC->OrbitCameraForTest(FVector2D(0.f, 20.f));
	TestNotEqual(TEXT("Y muove il pitch"), Cam->GetCameraPitch(), Pitch0);
	TestEqual(TEXT("e Y non tocca lo yaw"), Cam->GetCameraYaw(), YawAfterX);

	DestroyCameraWorld(World);
	return true;
}

/**
 * L'inclinazione si regola in partita, e non sfonda gli estremi.
 *
 * Il clamp e' in `AddPitch`, **non** nel `meta` del campo: `ClampMin`/`ClampMax` vincolano il widget del
 * Details e non le assegnazioni da codice — `#863` lo dava per un vincolo a runtime, e non lo e'. Gli
 * estremi non sono una preferenza: a `0` il piano di gioco sparisce in una linea, a `-90` `FRotator`
 * perde un grado di liberta'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraPitchTest,
	"RefactorTactics.Camera.PitchIsAdjustableAndClampedInCode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraPitchTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { DestroyCameraWorld(World); return false; }

	Cam->SetSensitivitiesForTest(/*Yaw=*/ 1.f, /*Pitch=*/ 1.f);

	const float Start = Cam->GetCameraPitch();
	Cam->AddPitch(+10.f);
	const float Raised = Cam->GetCameraPitch();
	if (!TestNotEqual(TEXT("l'inclinazione e' cambiata"), Raised, Start))
	{
		DestroyCameraWorld(World);
		return false;
	}
	TestEqual(TEXT("e il braccio la segue"),
		static_cast<float>(Arm->GetRelativeRotation().Pitch), Raised);

	// Verso l'alto: si ferma a 0, dove la camera guarda l'orizzonte.
	for (int32 I = 0; I < 100; ++I) { Cam->AddPitch(+10.f); }
	TestEqual(TEXT("non supera lo zero"), Cam->GetCameraPitch(), 0.f);

	// Verso il basso: si ferma un grado prima dello zenit.
	for (int32 I = 0; I < 100; ++I) { Cam->AddPitch(-10.f); }
	TestEqual(TEXT("e non scende sotto -89"), Cam->GetCameraPitch(), -89.f);

	// Input nullo non inclina.
	const float Before = Cam->GetCameraPitch();
	Cam->AddPitch(0.f);
	TestEqual(TEXT("input a zero non inclina"), Cam->GetCameraPitch(), Before);

	DestroyCameraWorld(World);
	return true;
}

/**
 * `Home` riporta anche l'INCLINAZIONE, non solo posizione e rotazione.
 *
 * Da `#863` il pitch e' regolabile, quindi «riportami a un'inquadratura che conosco» deve includerlo: e'
 * la ragione per cui `DefaultPitch` esiste come campo separato da `CameraPitch`. Prima bastava non
 * toccarlo — un campo solo faceva da default *e* da stato, e nessun input lo muoveva.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraRecenterRestoresPitchTest,
	"RefactorTactics.Camera.RecenterRestoresTheDefaultPitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraRecenterRestoresPitchTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { DestroyCameraWorld(World); return false; }

	// Default e stato corrente **distinti**: se fossero lo stesso campo — com'era prima di #863 — questo
	// test non potrebbe nemmeno essere scritto.
	Cam->SetPitchForTest(/*Default=*/ -40.f, /*Current=*/ -70.f);
	if (!TestEqual(TEXT("si parte dall'inclinazione regolata"), Cam->GetCameraPitch(), -70.f))
	{
		DestroyCameraWorld(World);
		return false;
	}

	Cam->RecenterView();
	TestEqual(TEXT("Home riporta alla taratura"), Cam->GetCameraPitch(), -40.f);
	TestEqual(TEXT("e il braccio la applica"),
		static_cast<float>(Arm->GetRelativeRotation().Pitch), -40.f);

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
 * `FocusOn` porta il pivot **sul bersaglio, quota compresa** — e lascia stare zoom e orientamento.
 *
 * 🔴 **Questo test verificava il contrario, e difendeva un difetto** (`#887`). Diceva «la quota resta la
 * sua, non quella del bersaglio» e piantava un `TestEqual` sulla `Z` di partenza: misurava la
 * *conservazione*, non la *correttezza*. La domanda giusta non e' «la quota resta invariata?» ma
 * «invariata **rispetto a cosa**?» — e la risposta era: rispetto al PlayerStart del livello, con cui il
 * pawn nasce e che non ha nessuna relazione con il piano della mappa. In partita, premere `F` mostrava il
 * vuoto.
 *
 * Le due meta' del contratto vanno tenute separate, ed e' il motivo per cui il test ora le nomina
 * entrambe: la **posizione** segue il bersaglio, lo **zoom** resta del giocatore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraFocusKeepsFramingTest,
	"RefactorTactics.Camera.FocusMovesToTargetAndKeepsZoom",
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

	// La quota del bersaglio e' **lontana** da quella del pawn, ed e' il punto del test: se `FocusOn` la
	// scartasse — com'era prima di #887 — la camera resterebbe a `1234` e orbiterebbe sopra il vuoto.
	Cam->FocusOn(FVector(500.f, -300.f, 250.f));

	// Letterali `double` senza suffisso: da LWC le componenti di `FVector` sono `FVector::FReal` (double),
	// e un `500.f` rende `TestEqual` ambiguo fra l'overload float e quello double (`error C2666`).
	const FVector After = Cam->GetActorLocation();
	TestEqual(TEXT("centra sulla X del bersaglio"), After.X, 500.0);
	TestEqual(TEXT("centra sulla Y del bersaglio"), After.Y, -300.0);
	TestEqual(TEXT("e prende anche la QUOTA del bersaglio, non quella con cui il pawn e' nato"),
		After.Z, 250.0);

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
 * L'inquadratura d'avvio non eredita la quota con cui il pawn e' nato.
 *
 * E' il caso reale di `#887`: `ARTGameMode` spawna la camera al **PlayerStart** del livello, e `BeginPlay`
 * chiama `FrameOwnTeam` — che calcola il centroide della squadra con la sua quota giusta. Finche' `FocusOn`
 * scartava quella `Z`, la partita si apriva con il pivot sospeso alla quota del PlayerStart, e nessuna
 * delle funzioni di input la correggeva: `AddPlanarMovement` ha `Z = 0` per scelta, `AddZoom` tocca il
 * braccio, `AddYaw` la rotazione. Solo `Home` rimetteva le cose a posto — ed e' il motivo per cui il
 * difetto si vedeva su `F` e non su `Home`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraFrameTeamIgnoresSpawnHeightTest,
	"RefactorTactics.Camera.FrameOwnTeamDoesNotInheritSpawnHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraFrameTeamIgnoresSpawnHeightTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	ARTHexMapActor* Map = SpawnCameraTestMap(World);
	if (!TestNotNull(TEXT("mappa"), Map)) { DestroyCameraWorld(World); return false; }

	// ⚠️ Il piano sta a una quota **non nulla**, ed e' cio' che rende il test discriminante: `Origin` di
	// `GetHexContext` e' la posizione dell'actor mappa. Con il piano a zero, «la quota giusta», «zero» e
	// «l'origine del mondo» sarebbero lo stesso numero, e un regresso a `Z = 0.f` passerebbe.
	const double PlaneZ = 300.0;
	Map->SetActorLocation(FVector(0.f, 0.f, PlaneZ));

	// ⚠️ E l'unita' NON sta sul centro mappa: `RecenterView` va su `GetCenterCell()`, quindi con l'unita'
	// li' sopra le due inquadrature coinciderebbero e il confronto non proverebbe niente.
	if (!TestNotNull(TEXT("unita'"), SpawnCameraTestUnit(World, /*TeamId=*/ 0, FRTCellId(6, 1))))
	{
		DestroyCameraWorld(World);
		return false;
	}

	// Il PlayerStart, simulato: una quota che non ha niente a che vedere con il piano della mappa.
	const double SpawnHeight = 5555.0;
	Cam->SetActorLocation(FVector(0.f, 0.f, SpawnHeight));

	if (!TestTrue(TEXT("inquadra la squadra"), Cam->FrameOwnTeam()))
	{
		DestroyCameraWorld(World);
		return false;
	}

	const FVector Framed = Cam->GetActorLocation();
	TestNotEqual(TEXT("la quota NON e' quella con cui il pawn e' nato"), Framed.Z, SpawnHeight);
	TestEqual(TEXT("ed e' quella del PIANO, che qui non e' zero"), Framed.Z, PlaneZ);

	// La prova che non ha semplicemente ricentrato: `Home` porta altrove, perche' l'unita' non sta sul
	// centro mappa.
	Cam->RecenterView();
	TestNotEqual(TEXT("e non e' il ricentramento: la squadra non sta sul centro mappa"),
		Cam->GetActorLocation(), Framed);

	DestroyCameraWorld(World);
	return true;
}

/**
 * Il tasto `F` inquadra il PIANO sotto l'unita', non il punto in cui il modello sta.
 *
 * E' il sintomo esatto riportato in seduta (`#887`): premendo `F` si vedeva il vuoto. Il percorso e'
 * `OnFocusSelected` → `FocusCameraOnUnit` → `FocusOn`, e la scelta che conta sta nel mezzo: `ARTUnit` si
 * posiziona con `VisualZOffset` (mezzo corpo sopra la cella), quindi passare `GetActorLocation()` porta il
 * pivot a una quota che nessun'altra inquadratura usa.
 *
 * ⚠️ Senza questo test il fix di `#887` resterebbe scoperto proprio sul suo caso d'uso: la copertura di
 * `FocusOn` verifica la funzione, non **cosa le viene passato**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraFocusUnitUsesCellPlaneTest,
	"RefactorTactics.Camera.FocusOnUnitFramesTheCellPlaneNotTheModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraFocusUnitUsesCellPlaneTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTHexMapActor* Map = SpawnCameraTestMap(World);
	if (!TestNotNull(TEXT("mappa"), Map)) { DestroyCameraWorld(World); return false; }

	// Piano a quota non nulla: senza, «la quota del piano» e «zero» coincidono e il test non discrimina.
	const double PlaneZ = 300.0;
	Map->SetActorLocation(FVector(0.f, 0.f, PlaneZ));

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("controller"), PC)) { DestroyCameraWorld(World); return false; }
	PC->Possess(Cam);

	ARTUnit* Unit = SpawnCameraTestUnit(World, /*TeamId=*/ 0, FRTCellId(6, 1));
	if (!TestNotNull(TEXT("unita'"), Unit)) { DestroyCameraWorld(World); return false; }

	// L'unita' sta **sopra** il proprio piano: e' la differenza che il test esiste per cogliere.
	Unit->SetActorLocation(FVector(1000.f, 2000.f, PlaneZ + 90.f));

	Cam->SetActorLocation(FVector(0.f, 0.f, 5555.f)); // quota di nascita, come il PlayerStart
	PC->FocusCameraOnUnit(Unit);

	const FVector Framed = Cam->GetActorLocation();
	TestEqual(TEXT("il pivot sta sul PIANO della cella"), Framed.Z, PlaneZ);
	TestNotEqual(TEXT("e non sul modello, che sta mezzo corpo sopra"), Framed.Z, PlaneZ + 90.0);
	TestNotEqual(TEXT("ne' alla quota di nascita del pawn"), Framed.Z, 5555.0);

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
 * La PRIMA inquadratura della partita: la camera aspetta le unita', e se non arrivano ripiega.
 *
 * `BeginPlay` prova `FrameOwnTeam`, e se fallisce riprova **al tick successivo** — perche' l'ordine di
 * `BeginPlay` fra actor non e' garantito e la camera puo' svegliarsi prima delle unita'. Solo se anche il
 * secondo tentativo fallisce ripiega su `RecenterView`.
 *
 * ⚠️ La mappa e' deliberatamente **non centrata sull'origine**: `RecenterView` porta la camera su
 * `GetCenterCell()`, e con una mappa centrata in `(0,0)` quel punto coinciderebbe con la posizione di
 * partenza del pawn — il test passerebbe qualunque cosa faccia il ripiego. E' lo stesso difetto che la
 * code review di #872 ha trovato nelle unita' simmetriche.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraBeginPlayRetriesTest,
	"RefactorTactics.Camera.BeginPlayWaitsOneTickForLateUnits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraBeginPlayRetriesTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	// Mappa lontana dall'origine: e' il riferimento che rende distinguibili le tre posizioni in gioco
	// (partenza del pawn, centro mappa, squadra).
	if (!TestNotNull(TEXT("mappa"), SpawnCameraTestMap(World, FRTCellId(6, 2))))
	{
		DestroyCameraWorld(World);
		return false;
	}

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	Cam->SetActorLocation(FVector::ZeroVector);

	// `BeginPlay` con il mondo ancora vuoto: `FrameOwnTeam` fallisce e registra il ritentativo.
	Cam->DispatchBeginPlay();
	const FVector AfterBeginPlay = Cam->GetActorLocation();

	// Le unita' arrivano DOPO, che e' l'intero caso d'uso. Verificate: se lo spawn fallisse, il
	// ritentativo non troverebbe nessuno, ripiegherebbe, e il test proverebbe l'altro ramo credendo di
	// provare questo.
	if (!TestNotNull(TEXT("prima unita' in ritardo"), SpawnCameraTestUnit(World, /*TeamId=*/ 0, FRTCellId(7, 2)))
		|| !TestNotNull(TEXT("seconda unita' in ritardo"), SpawnCameraTestUnit(World, /*TeamId=*/ 0, FRTCellId(8, 2))))
	{
		DestroyCameraWorld(World);
		return false;
	}

	// Un tick di timer: e' qui che il ritentativo deve scattare.
	World->GetTimerManager().Tick(0.05f);

	const FVector AfterRetry = Cam->GetActorLocation();
	TestNotEqual(TEXT("al tick successivo la camera si e' mossa"), AfterRetry, AfterBeginPlay);

	// ⚠️ «Si e' mossa» NON basta, ed e' il difetto che questa riga chiude: anche il **ripiego** su
	// `RecenterView` sposta la camera, sul centro mappa. Le due destinazioni vanno distinte, altrimenti
	// il test resta verde anche se il secondo `FrameOwnTeam` non trova nessuno.
	// `RecenterView` ci porta dove sarebbe finita ripiegando: se ci fosse gia', non si muoverebbe.
	Cam->RecenterView();
	TestNotEqual(TEXT("e non era il ripiego: la squadra non sta sul centro mappa"),
		Cam->GetActorLocation(), AfterRetry);

	DestroyCameraWorld(World);
	return true;
}

/**
 * Se le unita' non arrivano mai, la camera finisce sul centro mappa — non dove si trovava.
 *
 * E' il ramo che in partita non si vede: due `FrameOwnTeam` falliti di fila e il ripiego su
 * `RecenterView`. Senza test, togliere quella chiamata aprirebbe la partita su una vista non
 * inizializzata, e nulla lo segnalerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraBeginPlayFallsBackTest,
	"RefactorTactics.Camera.BeginPlayFallsBackToMapCentreWithoutUnits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraBeginPlayFallsBackTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	if (!TestNotNull(TEXT("mappa"), SpawnCameraTestMap(World, FRTCellId(6, 2))))
	{
		DestroyCameraWorld(World);
		return false;
	}

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	Cam->SetActorLocation(FVector::ZeroVector);

	Cam->DispatchBeginPlay();
	World->GetTimerManager().Tick(0.05f);

	// Nessuna unita' e' mai arrivata: la camera deve essere sul centro mappa, che con questa mappa NON
	// e' l'origine — cioe' non dove il pawn e' stato messo.
	const FVector Fallback = Cam->GetActorLocation();
	TestNotEqual(TEXT("ha ripiegato, non e' rimasta dov'era"), Fallback, FVector::ZeroVector);

	// E il punto e' proprio quello di `Home`: se il ripiego chiamasse altro, le due posizioni divergono.
	// ⚠️ La Z di partenza e' **diversa** da quella del ripiego, di proposito: riusare `Fallback.Z`
	// renderebbe il confronto cieco sull'asse verticale, cioe' su un terzo delle componenti che deve
	// verificare. (La prima stesura motivava la stessa riga con «`FocusOn` conserva la quota corrente»:
	// dopo `#887` non e' piu' vero — entrambe scrivono il vettore intero — ma la precauzione resta buona.)
	Cam->SetActorLocation(FVector(-9999.f, -9999.f, 4321.f));
	Cam->RecenterView();
	TestEqual(TEXT("ed e' esattamente dove porta Home"), Cam->GetActorLocation(), Fallback);

	DestroyCameraWorld(World);
	return true;
}

/**
 * `Home` non porta il braccio dove lo zoom non lo lascerebbe mai stare.
 *
 * `RecenterView` era l'unico dei quattro scrittori di `TargetArmLength` a non clampare. Il difetto **non
 * e' riproducibile con i valori di default** — `DefaultArmLength = 800` sta gia' dentro `[100, 4000]`, e
 * il clamp non cambierebbe niente — quindi il test deve costruire la combinazione che lo espone: un
 * default **oltre** il massimo, che dall'editor si ottiene con due campi e nessuna riga di codice.
 *
 * La conseguenza in partita non e' un numero fuori posto: e' che la prima tacca di rotellina dopo `Home`
 * riporta dentro di scatto, spostando l'inquadratura di colpo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraRecenterClampsArmTest,
	"RefactorTactics.Camera.RecenterKeepsArmWithinZoomLimits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraRecenterClampsArmTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { DestroyCameraWorld(World); return false; }

	// Il default sfora il massimo: e' la sola configurazione in cui il clamp mancante si vede.
	Cam->SetArmLengthRangeForTest(/*Default=*/ 5000.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);

	Cam->RecenterView();
	TestEqual(TEXT("Home si ferma al massimo, non al default fuori scala"), Arm->TargetArmLength, 4000.f);

	// La prova che conta per il giocatore: la prima tacca di zoom **dopo** `Home` deve muovere di un passo
	// e basta. Si zooma **avvicinando** (`-1`), perche' e' la sola direzione in cui il risultato e' un
	// valore esatto e non un altro clamp: 4000 - `ZoomStep`.
	//
	// 🔴 La prima stesura zoomava allontanando (`+1`) e verificava `Abs(nuovo - vecchio) <= 150`. Era una
	// **tautologia**: dopo il clamp il braccio sta gia' al massimo, quindi `Clamp(4150, 100, 4000)` non
	// puo' che restituire 4000 e la differenza e' sempre zero. Passava con qualunque implementazione, e
	// il commento la chiamava «la prova che conta». Trovata in code review.
	const float AfterHome = Arm->TargetArmLength;
	Cam->AddZoom(-1.f);
	const float AfterOneStep = Arm->TargetArmLength;
	TestTrue(TEXT("la prima rotellina muove di un passo, non di un salto"),
		AfterOneStep < AfterHome && AfterOneStep > AfterHome - 1000.f);
	// E il passo e' quello dello zoom, non un valore qualsiasi: lo si ricava dal comportamento —
	// due tacche coprono il doppio di una — invece di riscrivere la costante `ZoomStep`, che e'
	// `protected` e che i test di questo file per convenzione non duplicano.
	const float Step = AfterHome - AfterOneStep;
	Cam->AddZoom(-1.f);
	TestEqual(TEXT("il secondo passo e' uguale al primo"), AfterOneStep - Arm->TargetArmLength, Step);

	// E il limite inferiore vale allo stesso modo, per non lasciare mezza guardia.
	Cam->SetArmLengthRangeForTest(/*Default=*/ 10.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);
	Cam->RecenterView();
	TestEqual(TEXT("e non scende sotto il minimo"), Arm->TargetArmLength, 100.f);

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
