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
#include "Camera/CameraComponent.h" // ProjectionMode, OrthoWidth e GetCameraView della vista strategica (#3662)
#include "Engine/LocalPlayer.h"     // il vincolo d'aspetto con cui UE costruisce la proiezione
#include "SceneView.h"              // FSceneViewProjectionData
#include "UI/RTHudLayout.h"         // #3661: il rettangolo della scena
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Player/RTPlayerController.h"
#include "RTGameMode.h"           // #1837: lo scenario che rende la sessione un'osservazione
#include "Tests/RTWorldFixtures.h"
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
 * Lo zoom verso il cursore tiene fermo il punto puntato — entro **mezza cella**.
 *
 * La soglia non e' arbitraria: e' il numero deciso da `#864` sulla scala reale del mondo. Una cella misura
 * `√3 × HexSize ≈ 173` unita' fra centri, quindi sotto mezza cella **il cursore non cambia cella
 * esagonale** — l'unica cosa che il giocatore percepisce su una griglia. Non promette che il punto resti
 * fermo al pixel, e il DoD va letto cosi'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraZoomAnchorTest,
	"RefactorTactics.Camera.ZoomTowardsKeepsTheAnchorWithinHalfACell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraZoomAnchorTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { DestroyCameraWorld(World); return false; }

	// Intervallo fissato dal test: senza, il calcolo qui sotto dipenderebbe da `DefaultArmLength`, che e'
	// una manopola di taratura — il test cadrebbe per una decisione di tuning invece che per un difetto.
	Cam->SetArmLengthRangeForTest(/*Default=*/ 800.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);
	Arm->TargetArmLength = 800.f;

	// Ancora **lontana dal pivot**: se fosse sotto la camera, tenerla ferma sarebbe vero per costruzione.
	const FVector Anchor(900.f, 400.f, 0.f);
	Cam->SetCameraPivotForTest(FVector::ZeroVector);

	// `√3 × 100` — una spaziatura di RIFERIMENTO per costruire una tolleranza, non la scala del mondo.
	// ⚠️ Diceva «con `HexSize` di default», e dal 2026-08-25 il default e' `150` (`#1155`): qui il numero
	// resta `100` di proposito, perche' questo test non confronta nulla che il gioco produca a scala reale
	// — misura che un'ancora lontana resti ferma, e la tolleranza puo' essere qualunque valore sensato.
	const double CellSpacing = UE_SQRT_3 * 100.0;
	const double Tolerance = 0.5 * CellSpacing;

	// 🔴 **La prima stesura verificava una quantita' INVARIANTE PER COSTRUZIONE.** Confrontava
	// `|Ancora - Pivot| / Braccio` prima e dopo — ma e' esattamente cio' che la formula preserva per
	// definizione, quindi l'errore misurato era ~0 qualunque cosa facesse l'implementazione, e la
	// tolleranza di mezza cella non veniva mai esercitata. Trovato in code review.
	//
	// La verifica giusta e' **geometrica**: dove finisce sullo schermo il punto ancorato. Per una camera a
	// inclinazione fissa, la posizione a schermo dipende dall'offset dal pivot **diviso** per la distanza;
	// se quel rapporto si conserva, l'ancora e' ferma. Lo si confronta col valore che avrebbe **senza**
	// ancoraggio, che e' il difetto che il test deve poter vedere.
	const double OffsetBefore = FVector2D(Anchor.X, Anchor.Y).Size();       // pivot a zero
	const double ScreenBefore = OffsetBefore / 800.0;

	for (int32 I = 0; I < 12; ++I) { Cam->ZoomTowards(-1.f, Anchor); }

	const FVector Pivot = Cam->GetActorLocation();
	const double ScreenAfter = FVector2D(Anchor.X - Pivot.X, Anchor.Y - Pivot.Y).Size()
		/ FMath::Max(Arm->TargetArmLength, KINDA_SMALL_NUMBER);

	// Lo scarto si riporta in unita' di mondo alla distanza corrente: e' la misura che la tolleranza in
	// celle vuole vincolare.
	const double DriftWorld = FMath::Abs(ScreenAfter - ScreenBefore) * Arm->TargetArmLength;
	TestTrue(TEXT("l'ancora resta entro mezza cella"), DriftWorld <= Tolerance);

	// ⚠️ **E il test deve poter FALLIRE**: senza ancoraggio, con lo stesso zoom, l'ancora scivolerebbe
	// molto oltre la tolleranza. Questa riga lo dimostra sui numeri, cosi' la soglia non e' decorativa.
	const double ScreenNoAnchor = OffsetBefore / FMath::Max(Arm->TargetArmLength, KINDA_SMALL_NUMBER);
	const double DriftNoAnchor = FMath::Abs(ScreenNoAnchor - ScreenBefore) * Arm->TargetArmLength;
	TestTrue(TEXT("senza ancoraggio lo scarto supererebbe la tolleranza (la soglia morde)"),
		DriftNoAnchor > Tolerance);

	// Guardia di fondo corsa: al limite lo zoom non trascina piu' la vista.
	for (int32 I = 0; I < 50; ++I) { Cam->ZoomTowards(-1.f, Anchor); }
	const FVector AtLimit = Cam->GetActorLocation();
	Cam->ZoomTowards(-1.f, Anchor);
	TestEqual(TEXT("a fondo corsa il pivot non si muove piu'"), Cam->GetActorLocation(), AtLimit);

	DestroyCameraWorld(World);
	return true;
}

/**
 * I soft bounds fermano il centro camera a **3 celle** oltre il bordo mappa, da ciascun lato.
 *
 * Il numero e' deciso da `#864`: su una mappa di raggio 4 il centro arriva a 7 celle dall'origine, quindi
 * il bordo si puo' portare al centro dello schermo senza perdere la mappa. E' misura **fissa in celle**,
 * non proporzionale al raggio — il margine serve al bordo, e il bordo e' locale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraSoftBoundsTest,
	"RefactorTactics.Camera.PanStopsThreeCellsPastTheMapEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraSoftBoundsTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	// Mappa centrata sull'origine: qui serve, perche' il test misura una **distanza dal bordo** e un
	// centro spostato la renderebbe solo piu' difficile da leggere senza aggiungere nulla.
	ARTHexMapActor* HexMap = SpawnCameraTestMap(World, FRTCellId(0, 0, 0));
	if (!TestNotNull(TEXT("mappa"), HexMap))
	{
		DestroyCameraWorld(World);
		return false;
	}

	// 🔴 **La scala si CHIEDE alla mappa, non si scrive.** Questa misura aveva `100.0` a mano e cadeva
	// quando `#1155` ha portato `HexSize` a `150`: il margine risultava `5.37` celle invece di `3`. Il
	// difetto non era nella camera — `ARTCameraPawn` legge la scala da `GetHexContext` — ma nell'attesa,
	// calcolata in un mondo che non esisteva piu'. Riscriverla come `150.0` avrebbe rimandato la stessa
	// caduta al cambio di scala successivo.
	FVector MapOrigin = FVector::ZeroVector;
	float MapHexSize = 0.f;
	float MapLayerHeight = 0.f;
	HexMap->GetHexContext(MapOrigin, MapHexSize, MapLayerHeight);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	Cam->SetCameraPivotForTest(FVector::ZeroVector);

	// 🔴 **La prima stesura non pinnava il margine, e passava anche con `BoundsMarginCells = 0`.**
	// Verificava solo un limite superiore lasco — «il centro resta entro 5 celle» — che il bordo mappa
	// da solo (2 celle) rispetta senza bisogno di nessun margine. Il numero **3 celle**, presentato come
	// la decisione centrale della issue, non era verificato da nessuna assertion. Trovato in code review.
	//
	// La verifica giusta misura **la distanza fra il limite raggiunto e il bordo mappa**, che e' la
	// definizione del margine, e la confronta col numero deciso.
	const double CellSpacing = UE_SQRT_3 * static_cast<double>(MapHexSize);

	// Bordo mappa in X per un'area di raggio 2: la cella piu' lontana e' a `2 * spacing`.
	const double EdgeX = 2.0 * CellSpacing;

	for (int32 Dir = 0; Dir < 4; ++Dir)
	{
		Cam->SetCameraPivotForTest(FVector::ZeroVector);
		const FVector2D Axis = (Dir == 0) ? FVector2D(1, 0) : (Dir == 1) ? FVector2D(-1, 0)
			: (Dir == 2) ? FVector2D(0, 1) : FVector2D(0, -1);
		for (int32 I = 0; I < 400; ++I) { Cam->AddPlanarMovement(Axis); }

		// ⚠️ **`Axis.X` e' «a destra sullo schermo», che con yaw 0 e' +Y nel MONDO**, e `Axis.Y` e'
		// «avanti», cioe' +X. La prima stesura di questo test misurava l'asse sbagliato — e il vecchio
		// limite lasco lo nascondeva, perche' zero rispetta qualunque massimo.
		const FVector P = Cam->GetActorLocation();
		const double Reached = (Dir < 2) ? FMath::Abs(P.Y) : FMath::Abs(P.X);
		// Bordo mappa: in Y le file distano `1.5 * HexSize` (due file dal centro), in X `√3 * HexSize`.
		const double Edge = (Dir < 2) ? (1.5 * static_cast<double>(MapHexSize) * 2.0) : EdgeX;

		// Il margine effettivo, in celle, misurato: e' il numero che la issue ha deciso.
		const double MarginCells = (Reached - Edge) / CellSpacing;
		TestTrue(*FString::Printf(TEXT("direzione %d: margine %.2f celle, atteso 3"), Dir, MarginCells),
			FMath::Abs(MarginCells - 3.0) < 0.05);
	}

	// ⚠️ E il margine **si muove col campo**: se fosse ignorato, questa riga non cambierebbe nulla. E' la
	// prova che il `3` viene dal parametro e non da un caso.
	Cam->SetCameraPivotForTest(FVector::ZeroVector);
	for (int32 I = 0; I < 400; ++I) { Cam->AddPlanarMovement(FVector2D(1.f, 0.f)); }
	const double ReachedAtThree = FMath::Abs(Cam->GetActorLocation().Y);

	Cam->SetBoundsMarginForTest(1.f);
	Cam->SetCameraPivotForTest(FVector::ZeroVector);
	for (int32 I = 0; I < 400; ++I) { Cam->AddPlanarMovement(FVector2D(1.f, 0.f)); }
	const double ReachedAtOne = FMath::Abs(Cam->GetActorLocation().Y);

	TestTrue(TEXT("con margine 1 il limite e' piu' vicino di due celle"),
		FMath::Abs((ReachedAtThree - ReachedAtOne) - 2.0 * CellSpacing) < 1.0);

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
	Cam->SetCameraPivotForTest(FVector::ZeroVector);
	Cam->AddPlanarMovement(FVector2D(0.f, 1.f));
	const FVector Straight = Cam->GetActorLocation();
	TestTrue(TEXT("yaw 0: avanti e' +X"), Straight.X > 1.f && FMath::Abs(Straight.Y) < 0.01f);

	// Yaw 90: lo STESSO input deve andare da un'altra parte nel mondo — perche' sullo schermo va nello
	// stesso posto. E' esattamente cio' che il pan in assi mondo non faceva.
	// Due passi da 45 fanno 90: l'angolo comodo per il test si ottiene dai passi veri, non riconfigurandoli.
	Cam->AddYaw(+1.f);
	Cam->AddYaw(+1.f);
	TestEqual(TEXT("ruotata di 90"), Cam->GetCameraYaw(), 90.f);
	Cam->SetCameraPivotForTest(FVector::ZeroVector);
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
	Cam->SetCameraPivotForTest(FVector::ZeroVector);
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
	Cam->SetCameraPivotForTest(FVector(0.f, 0.f, 1234.f));
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
	Cam->SetCameraPivotForTest(FVector(0.f, 0.f, SpawnHeight));

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

	Cam->SetCameraPivotForTest(FVector(0.f, 0.f, 5555.f)); // quota di nascita, come il PlayerStart
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

		Cam->SetCameraPivotForTest(FVector(77.f, 88.f, 99.f));
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

		Cam->SetCameraPivotForTest(FVector(11.f, 22.f, 33.f));
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
	Cam->SetCameraPivotForTest(FVector::ZeroVector);

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
	Cam->SetCameraPivotForTest(FVector::ZeroVector);

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
	Cam->SetCameraPivotForTest(FVector(-9999.f, -9999.f, 4321.f));
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
 * `FrameOwnTeam` legge `ARTPlayerState::TeamIdOf` e ripiega sulla squadra 0 solo quando un controller non
 * c'e' — una comodita' da demo. Senza questo test la lettura del controller e' cancellabile senza far
 * cadere niente, e in una partita in cui il giocatore e' la squadra 1 la camera aprirebbe inquadrando
 * **il nemico**.
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

	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, 1);
	if (!TestNotNull(TEXT("controller"), PC)) { DestroyCameraWorld(World); return false; }
	PC->Possess(Cam);

	if (!TestTrue(TEXT("con il controller inquadra"), Cam->FrameOwnTeam()))
	{
		DestroyCameraWorld(World);
		return false;
	}

	// Se la lettura di `ARTPlayerState::TeamIdOf` sparisse, questa resterebbe l'inquadratura della squadra 0
	// e le due posizioni coinciderebbero.
	TestNotEqual(TEXT("la squadra 1 non si inquadra dove sta la squadra 0"),
		Cam->GetActorLocation(), FramedTeamZero);

	DestroyCameraWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------
// #1837 - `Home` contestuale: selezione, poi squadra, poi centro mappa.
// ---------------------------------------------------------------------------------------------------

namespace
{
	/** Il punto del PIANO di una cella, come lo calcola chi inquadra. Nomi distinti: la unity build e' una. */
	FVector HomeTestCellWorld(const ARTHexMapActor* Map, const FRTCellId& Cell)
	{
		FVector Origin; float HexSize; float LayerHeight;
		Map->GetHexContext(Origin, HexSize, LayerHeight);
		return URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerHeight);
	}

	FVector HomeTestCellsCentroid(const ARTHexMapActor* Map, const TArray<FRTCellId>& Cells)
	{
		FVector Origin; float HexSize; float LayerHeight;
		Map->GetHexContext(Origin, HexSize, LayerHeight);
		return URTHexLibrary::CellsCentroidWorld(Cells, Origin, HexSize, LayerHeight);
	}

	FVector HomeTestMapCentre(const ARTHexMapActor* Map)
	{
		FVector Origin; float HexSize; float LayerHeight;
		const URTHexMapAsset* Asset = Map->GetHexContext(Origin, HexSize, LayerHeight);
		return URTHexLibrary::AxialToWorld(Asset->GetCenterCell(), Origin, HexSize, LayerHeight);
	}

	/**
	 * Le tre celle dei test di `Home`, tutte dentro la mappa di raggio 2 attorno a `(4,1)` e scelte perche' i
	 * tre bersagli — la selezione, il punto medio della squadra, il centro mappa — siano **tre punti
	 * diversi**. Con due di loro sovrapposti un ramo sbagliato passerebbe per coincidenza.
	 */
	const FRTCellId HomeSelectedCell(6, 0);
	const FRTCellId HomeMateCell(2, 1);
	const FRTCellId HomeEnemyCell(5, 2);

	/** Mondo, mappa, camera e un giocatore della squadra `PlayerTeam` che la possiede. */
	struct FHomeTestScene
	{
		UWorld* World = nullptr;
		ARTHexMapActor* Map = nullptr;
		ARTCameraPawn* Cam = nullptr;

		bool Build(FAutomationTestBase& Test, int32 PlayerTeam)
		{
			World = MakeCameraWorld();
			if (!Test.TestNotNull(TEXT("world"), World)) { return false; }
			Map = SpawnCameraTestMap(World);
			Cam = World->SpawnActor<ARTCameraPawn>();
			ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, PlayerTeam);
			if (!Test.TestNotNull(TEXT("mappa"), Map) || !Test.TestNotNull(TEXT("camera"), Cam)
				|| !Test.TestNotNull(TEXT("controller"), PC))
			{
				return false;
			}
			PC->Possess(Cam);
			return true;
		}

		~FHomeTestScene() { DestroyCameraWorld(World); }
	};
}

/**
 * Con un'unita' selezionata, `Home` inquadra QUELLA — e azzera orientamento e distanza come `RecenterView`.
 *
 * ⚠️ Il discriminante e' il compagno su un'altra cella: senza di lui la selezione e la squadra cadrebbero
 * sullo stesso punto, e un `Home` che ignora la selezione passerebbe. L'azzeramento e' la scelta
 * dell'autore del 2026-10-15: «Home resta il tasto di recupero».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraHomeFramesSelectionTest,
	"RefactorTactics.Camera.HomeFramesTheSelectedOwnUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraHomeFramesSelectionTest::RunTest(const FString&)
{
	FHomeTestScene S;
	if (!S.Build(*this, /*PlayerTeam=*/ 0)) { return false; }

	ARTUnit* Selected = SpawnCameraTestUnit(S.World, /*TeamId=*/ 0, HomeSelectedCell);
	if (!TestNotNull(TEXT("unita' selezionata"), Selected)
		|| !TestNotNull(TEXT("compagno"), SpawnCameraTestUnit(S.World, 0, HomeMateCell)))
	{
		return false;
	}

	// Uno stato «perso»: vista girata, zoomata lontano, inclinata.
	S.Cam->SetArmLengthRangeForTest(/*Default=*/ 800.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);
	S.Cam->SetPitchForTest(/*Default=*/ -40.f, /*Current=*/ -70.f);
	S.Cam->AddYaw(+1.f);
	for (int32 i = 0; i < 5; ++i) { S.Cam->AddZoom(+1.f); }
	S.Cam->SetPeekOffset(FVector(60.f, 0.f, 0.f));
	USpringArmComponent* Arm = S.Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { return false; }
	if (!TestNotEqual(TEXT("precondizione: la vista e' girata"), S.Cam->GetCameraYaw(), 0.f)
		|| !TestNotEqual(TEXT("precondizione: lo zoom non e' il default"), Arm->TargetArmLength, 800.f)
		|| !TestFalse(TEXT("precondizione: c'e' un peek"), S.Cam->GetPeekOffset().IsNearlyZero()))
	{
		return false;
	}

	TestTrue(TEXT("il ramo e' la selezione"), S.Cam->FrameHome(Selected) == ERTHomeFraming::Selection);
	const FVector Pivot = S.Cam->GetCameraPivot();
	const FVector Cell = HomeTestCellWorld(S.Map, HomeSelectedCell);
	TestTrue(TEXT("il pivot sta sulla cella selezionata"), FVector::Dist2D(Pivot, Cell) < 1.0);
	TestTrue(TEXT("e non sul punto medio della squadra"),
		FVector::Dist2D(Pivot, HomeTestCellsCentroid(S.Map, { HomeSelectedCell, HomeMateCell })) > 1.0);
	TestEqual(TEXT("sul PIANO della cella, non alla quota del modello"), Pivot.Z, Cell.Z);
	TestEqual(TEXT("l'orientamento torna dritto"), S.Cam->GetCameraYaw(), 0.f);
	TestEqual(TEXT("l'inclinazione torna al default"), S.Cam->GetCameraPitch(), -40.f);
	TestEqual(TEXT("e la distanza al default, come RecenterView"), Arm->TargetArmLength, 800.f);
	TestTrue(TEXT("e il peek e' azzerato"), S.Cam->GetPeekOffset().IsNearlyZero());
	return true;
}

/**
 * Senza selezione, `Home` inquadra il punto medio della propria squadra — alla distanza di `Home`, non a
 * quella d'inizio partita.
 *
 * ⚠️ La seconda meta' e' il discriminante di una scorciatoia ovvia: chiamare `FrameOwnTeam` porterebbe il
 * pivot nel posto giusto e il braccio a `MatchStartArmLength`, cioe' a un'inquadratura che non e' `Home`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraHomeFramesOwnTeamTest,
	"RefactorTactics.Camera.HomeWithoutSelectionFramesTheOwnTeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraHomeFramesOwnTeamTest::RunTest(const FString&)
{
	FHomeTestScene S;
	if (!S.Build(*this, /*PlayerTeam=*/ 0)) { return false; }
	if (!TestNotNull(TEXT("unita' 1"), SpawnCameraTestUnit(S.World, 0, HomeSelectedCell))
		|| !TestNotNull(TEXT("unita' 2"), SpawnCameraTestUnit(S.World, 0, HomeMateCell))
		|| !TestNotNull(TEXT("nemico"), SpawnCameraTestUnit(S.World, 1, HomeEnemyCell)))
	{
		return false;
	}
	S.Cam->SetArmLengthRangeForTest(/*Default=*/ 800.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);

	TestTrue(TEXT("il ramo e' la squadra"), S.Cam->FrameHome(nullptr) == ERTHomeFraming::OwnTeam);
	const FVector Pivot = S.Cam->GetCameraPivot();
	TestTrue(TEXT("il pivot sta sul punto medio delle proprie unita', il nemico escluso"),
		FVector::Dist2D(Pivot, HomeTestCellsCentroid(S.Map, { HomeSelectedCell, HomeMateCell })) < 1.0);
	TestTrue(TEXT("e non sul centro mappa"), FVector::Dist2D(Pivot, HomeTestMapCentre(S.Map)) > 1.0);

	USpringArmComponent* Arm = S.Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { return false; }
	TestEqual(TEXT("la distanza e' quella di Home"), Arm->TargetArmLength, 800.f);
	TestNotEqual(TEXT("non quella d'inizio partita di FrameOwnTeam"),
		Arm->TargetArmLength, S.Cam->GetMatchStartArmLength());
	return true;
}

/**
 * Senza selezione e senza unita' proprie vive, `Home` fa cio' che faceva prima di #1837: il centro mappa.
 *
 * L'ultimo anello non si toglie. Una `Home` che puo' non fare nulla e' peggio di una `Home` grossolana: la
 * camera deve muoversi, e il ramo deve dirlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraHomeFallsBackTest,
	"RefactorTactics.Camera.HomeWithoutOwnUnitsFallsBackToTheMapCentre",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraHomeFallsBackTest::RunTest(const FString&)
{
	FHomeTestScene S;
	if (!S.Build(*this, /*PlayerTeam=*/ 0)) { return false; }
	ARTUnit* Fallen = SpawnCameraTestUnit(S.World, 0, HomeMateCell);
	if (!TestNotNull(TEXT("compagno"), Fallen)
		|| !TestNotNull(TEXT("nemico vivo"), SpawnCameraTestUnit(S.World, 1, HomeEnemyCell)))
	{
		return false;
	}
	// Ucciso con l'API di gioco: e' `IsAlive()` che il filtro interroga.
	Fallen->ApplyCombatState(/*NewHealth=*/ 0, /*NewShield=*/ 0);

	const FVector Start = HomeTestCellWorld(S.Map, HomeSelectedCell);
	S.Cam->SetCameraPivotForTest(Start);

	TestTrue(TEXT("il ramo e' il centro mappa"), S.Cam->FrameHome(nullptr) == ERTHomeFraming::MapCenter);
	TestTrue(TEXT("il pivot sta sul centro mappa"),
		FVector::Dist2D(S.Cam->GetCameraPivot(), HomeTestMapCentre(S.Map)) < 1.0);
	TestTrue(TEXT("quindi la camera si e' mossa"), FVector::Dist2D(S.Cam->GetCameraPivot(), Start) > 1.0);
	return true;
}

/**
 * 🔴 `Home` non inquadra MAI un'unita' di un'altra squadra — e' il criterio discriminante di #1837.
 *
 * Un test che verifica solo «Home muove la camera» passa anche su un'implementazione che centra il nemico
 * piu' vicino. Qui il nemico viene **passato come selezione**, che e' l'unico modo in cui potrebbe
 * arrivarci: la guardia deve reggere anche contro un chiamante che sbaglia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraHomeNeverFramesEnemyTest,
	"RefactorTactics.Camera.HomeNeverFramesAnotherTeamsUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraHomeNeverFramesEnemyTest::RunTest(const FString&)
{
	// (a) Il nemico e' l'unica unita' viva del mondo, ed e' la selezione passata.
	{
		FHomeTestScene S;
		if (!S.Build(*this, /*PlayerTeam=*/ 0)) { return false; }
		ARTUnit* Enemy = SpawnCameraTestUnit(S.World, 1, HomeEnemyCell);
		if (!TestNotNull(TEXT("(a) nemico"), Enemy)) { return false; }

		TestTrue(TEXT("(a) il nemico non e' un ramo: si ripiega sul centro mappa"),
			S.Cam->FrameHome(Enemy) == ERTHomeFraming::MapCenter);
		TestTrue(TEXT("(a) il pivot non sta sul nemico"),
			FVector::Dist2D(S.Cam->GetCameraPivot(), HomeTestCellWorld(S.Map, HomeEnemyCell)) > 1.0);
	}

	// (b) Con un'unita' propria viva, il nemico passato come selezione cede alla squadra.
	{
		FHomeTestScene S;
		if (!S.Build(*this, /*PlayerTeam=*/ 0)) { return false; }
		ARTUnit* Enemy = SpawnCameraTestUnit(S.World, 1, HomeEnemyCell);
		if (!TestNotNull(TEXT("(b) nemico"), Enemy)
			|| !TestNotNull(TEXT("(b) propria"), SpawnCameraTestUnit(S.World, 0, HomeSelectedCell)))
		{
			return false;
		}

		TestTrue(TEXT("(b) si inquadra la squadra"), S.Cam->FrameHome(Enemy) == ERTHomeFraming::OwnTeam);
		TestTrue(TEXT("(b) cioe' l'unita' propria, non il nemico"),
			FVector::Dist2D(S.Cam->GetCameraPivot(), HomeTestCellWorld(S.Map, HomeSelectedCell)) < 1.0);
	}

	// (c) «La propria squadra» e' quella del CONTROLLER: per il giocatore della squadra 1, un'unita' della 0
	// e' un'unita' altrui. Senza questo caso la guardia potrebbe confrontare con la 0 scritta a mano.
	{
		FHomeTestScene S;
		if (!S.Build(*this, /*PlayerTeam=*/ 1)) { return false; }
		ARTUnit* TeamZero = SpawnCameraTestUnit(S.World, 0, HomeSelectedCell);
		if (!TestNotNull(TEXT("(c) unita' della 0"), TeamZero)
			|| !TestNotNull(TEXT("(c) unita' della 1"), SpawnCameraTestUnit(S.World, 1, HomeMateCell)))
		{
			return false;
		}

		TestTrue(TEXT("(c) per la squadra 1 la squadra 0 non e' una selezione"),
			S.Cam->FrameHome(TeamZero) == ERTHomeFraming::OwnTeam);
		TestTrue(TEXT("(c) si inquadra la squadra 1"),
			FVector::Dist2D(S.Cam->GetCameraPivot(), HomeTestCellWorld(S.Map, HomeMateCell)) < 1.0);
	}

	// (d) Una selezione propria ma caduta non e' un'inquadratura: si passa alla squadra.
	{
		FHomeTestScene S;
		if (!S.Build(*this, /*PlayerTeam=*/ 0)) { return false; }
		ARTUnit* Fallen = SpawnCameraTestUnit(S.World, 0, HomeSelectedCell);
		if (!TestNotNull(TEXT("(d) caduta"), Fallen)
			|| !TestNotNull(TEXT("(d) compagno"), SpawnCameraTestUnit(S.World, 0, HomeMateCell)))
		{
			return false;
		}
		Fallen->ApplyCombatState(/*NewHealth=*/ 0, /*NewShield=*/ 0);

		TestTrue(TEXT("(d) la selezione caduta cede alla squadra"),
			S.Cam->FrameHome(Fallen) == ERTHomeFraming::OwnTeam);
		TestTrue(TEXT("(d) cioe' il compagno vivo"),
			FVector::Dist2D(S.Cam->GetCameraPivot(), HomeTestCellWorld(S.Map, HomeMateCell)) < 1.0);
	}

	return true;
}

/**
 * Il pivot di `Home` passa dai limiti del viewport (`D-251`) con la vista DOPO il reset, in ogni ramo.
 *
 * Il caso e' quello che #873 aveva rotto: zoom al massimo su una mappa piccola, dove l'inquadratura e' piu'
 * grande dell'area e il limite inchioda il pivot. Il riferimento non e' un numero scritto a mano: e' cio'
 * che `SetCameraPivot` fa sullo stesso bersaglio con la vista gia' azzerata. Il ramo del centro mappa e'
 * `RecenterView`, e lo copre gia' `Camera.RecenterClampsWithTheViewItIsAboutToHaveNotTheOldOne`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraHomeStaysInBoundsTest,
	"RefactorTactics.Camera.HomeKeepsThePivotInsideTheViewportBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraHomeStaysInBoundsTest::RunTest(const FString&)
{
	for (const ERTHomeFraming Expected : { ERTHomeFraming::Selection, ERTHomeFraming::OwnTeam })
	{
		const TCHAR* Ramo = Expected == ERTHomeFraming::Selection ? TEXT("selezione") : TEXT("squadra");
		FHomeTestScene S;
		if (!S.Build(*this, /*PlayerTeam=*/ 0)) { return false; }
		ARTUnit* Unit = SpawnCameraTestUnit(S.World, 0, HomeSelectedCell);
		if (!TestNotNull(TEXT("unita'"), Unit)) { return false; }

		// Il default di `Home` e' il massimo: dopo il reset la vista e' la piu' grande che la camera abbia.
		// ⚠️ Le soglie strategiche vanno spostate oltre: a 4000 la camera passerebbe in strategica, e il caso
		// che conta — `Home` in tattica, prospettica — non verrebbe mai esercitato (rilievo di review).
		S.Cam->SetViewportMetricsForTest(16.f/9.f, 90.f);
		S.Cam->SetStrategicThresholdsForTest(/*Enter=*/ 100000.f, /*Exit=*/ 90000.f);
		S.Cam->SetArmLengthRangeForTest(/*Default=*/ 4000.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);
		// Si parte vicini e girati: il clamp con la vista VECCHIA lascerebbe il pivot dove la vista piccola
		// lo consente, cioe' sulla cella.
		for (int32 i = 0; i < 40; ++i) { S.Cam->AddZoom(-1.f); }
		S.Cam->AddYaw(+1.f);

		const ERTHomeFraming Got = S.Cam->FrameHome(Expected == ERTHomeFraming::Selection ? Unit : nullptr);
		TestTrue(FString::Printf(TEXT("%s: il ramo e' quello atteso"), Ramo), Got == Expected);
		if (!TestFalse(FString::Printf(TEXT("%s: precondizione, Home resta in tattica"), Ramo), S.Cam->IsStrategicView()))
		{
			return false;
		}
		const FVector Framed = S.Cam->GetCameraPivot();

		// Il riferimento: la stessa vista azzerata, lo stesso bersaglio, la porta unica.
		S.Cam->RecenterView();
		S.Cam->SetCameraPivot(HomeTestCellWorld(S.Map, HomeSelectedCell));
		const FVector Reference = S.Cam->GetCameraPivot();
		if (!TestTrue(FString::Printf(TEXT("%s: precondizione, il limite morde a zoom massimo"), Ramo),
			FVector::Dist2D(Reference, HomeTestCellWorld(S.Map, HomeSelectedCell)) > 1.0))
		{
			return false;
		}
		TestTrue(FString::Printf(TEXT("%s: Home clampa come la porta unica, con la vista dopo il reset"), Ramo),
			FVector::Dist2D(Framed, Reference) < 1.0);
	}
	return true;
}

/**
 * Il tasto `Home` passa da `FrameHome`, con la selezione del controller.
 *
 * 🔴 I test qui sopra chiamano `FrameHome` a mano, quindi provano la camera e non il cablaggio: un
 * `OnRecenter` rimasto su `RecenterView` li lascerebbe tutti verdi, e in partita `Home` continuerebbe a
 * centrare la mappa. Questo parte dal gestore vero del tasto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraHomeKeyUsesFrameHomeTest,
	"RefactorTactics.Camera.HomeKeyFramesTheControllersSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraHomeKeyUsesFrameHomeTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnCameraTestMap(World);
	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, 0);
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam) || !TestNotNull(TEXT("controller"), PC))
	{
		DestroyCameraWorld(World);
		return false;
	}
	PC->Possess(Cam);

	ARTUnit* Selected = SpawnCameraTestUnit(World, 0, HomeSelectedCell);
	if (!TestNotNull(TEXT("unita' selezionata"), Selected)
		|| !TestNotNull(TEXT("compagno"), SpawnCameraTestUnit(World, 0, HomeMateCell)))
	{
		DestroyCameraWorld(World);
		return false;
	}
	PC->SelectUnit(Selected, /*bRecordAsPlayerInput=*/ false);
	if (!TestTrue(TEXT("precondizione: il controller ha la selezione"), PC->GetSelectedUnit() == Selected))
	{
		DestroyCameraWorld(World);
		return false;
	}

	PC->RecenterForTest();
	TestTrue(TEXT("Home va sulla selezione del controller"),
		FVector::Dist2D(Cam->GetCameraPivot(), HomeTestCellWorld(Map, HomeSelectedCell)) < 1.0);
	TestTrue(TEXT("e non sul centro mappa, dove andava prima di #1837"),
		FVector::Dist2D(Cam->GetCameraPivot(), HomeTestMapCentre(Map)) > 1.0);

	DestroyCameraWorld(World);
	return true;
}

/**
 * Un nemico ISPEZIONATO non entra in `Home`, dal tasto vero.
 *
 * 🔴 `Camera.HomeNeverFramesAnotherTeamsUnit` prova la guardia della camera chiamando `FrameHome(Enemy)`: un
 * ingresso che il tasto non usa. `OnRecenter` passa `GetSelectedUnit()`, e l'ispezione scrive un campo a
 * parte (`InspectedUnit`) senza toccare la selezione. Questo parte dal gestore del tasto con un nemico
 * ispezionato, e prova che il pivot non gli si avvicina in nessuno dei due rami che restano: selezione propria
 * e squadra (#1837, `PIE-CAM-HOME-CONTEXT` caso 3).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraHomeKeyIgnoresInspectedEnemyTest,
	"RefactorTactics.Camera.HomeKeyIgnoresAnInspectedEnemy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraHomeKeyIgnoresInspectedEnemyTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnCameraTestMap(World);
	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, 0);
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam) || !TestNotNull(TEXT("controller"), PC))
	{
		DestroyCameraWorld(World);
		return false;
	}
	PC->Possess(Cam);

	ARTUnit* Mine = SpawnCameraTestUnit(World, 0, HomeSelectedCell);
	ARTUnit* Mate = SpawnCameraTestUnit(World, 0, HomeMateCell);
	ARTUnit* Enemy = SpawnCameraTestUnit(World, 1, HomeEnemyCell);
	if (!TestNotNull(TEXT("unita' propria"), Mine) || !TestNotNull(TEXT("compagno"), Mate)
		|| !TestNotNull(TEXT("nemico"), Enemy))
	{
		DestroyCameraWorld(World);
		return false;
	}
	PC->SetInspectedUnitForTest(Enemy);
	if (!TestTrue(TEXT("precondizione: il nemico e' ispezionato"), PC->GetInspectedUnit() == Enemy)
		|| !TestTrue(TEXT("precondizione: nessuna selezione"), PC->GetSelectedUnit() == nullptr))
	{
		DestroyCameraWorld(World);
		return false;
	}

	const FVector EnemyCell = HomeTestCellWorld(Map, HomeEnemyCell);

	// Senza selezione: la squadra. Il nemico ispezionato non sposta il punto medio.
	PC->RecenterForTest();
	TestTrue(TEXT("(a) senza selezione Home va sul punto medio della squadra"),
		FVector::Dist2D(Cam->GetCameraPivot(), HomeTestCellsCentroid(Map, { HomeSelectedCell, HomeMateCell })) < 1.0);
	TestTrue(TEXT("(a) e non sul nemico ispezionato"), FVector::Dist2D(Cam->GetCameraPivot(), EnemyCell) > 1.0);

	// Con una selezione propria: quella. L'ispezione non la sostituisce.
	PC->SelectUnit(Mine, /*bRecordAsPlayerInput=*/ false);
	PC->SetInspectedUnitForTest(Enemy);
	PC->RecenterForTest();
	TestTrue(TEXT("(b) con la selezione Home va sulla selezione, non sull'ispezionato"),
		FVector::Dist2D(Cam->GetCameraPivot(), HomeTestCellWorld(Map, HomeSelectedCell)) < 1.0);
	TestTrue(TEXT("(b) e non sul nemico ispezionato"), FVector::Dist2D(Cam->GetCameraPivot(), EnemyCell) > 1.0);
	TestTrue(TEXT("(b) l'ispezione non ha cambiato la selezione"), PC->GetSelectedUnit() == Mine);

	DestroyCameraWorld(World);
	return true;
}

/**
 * Senza `ARTPlayerState`, `Home` non indovina la squadra: va sul centro mappa.
 *
 * ⛔ `TeamIdOf` ripiega sulla 0 quando lo stato manca. Per `FrameOwnTeam` all'avvio e' una comodita' da demo;
 * per `Home` vorrebbe dire inquadrare la squadra 0 per un giocatore della 1 il cui stato non e' ancora
 * arrivato — un oracolo attivabile con un tasto (rilievo di review).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraHomeWithoutPlayerStateTest,
	"RefactorTactics.Camera.HomeWithoutPlayerStateDoesNotGuessTheTeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraHomeWithoutPlayerStateTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnCameraTestMap(World);
	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>(); // nessuno stato del giocatore
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam) || !TestNotNull(TEXT("controller"), PC))
	{
		DestroyCameraWorld(World);
		return false;
	}
	PC->Possess(Cam);
	ARTUnit* TeamZero = SpawnCameraTestUnit(World, 0, HomeSelectedCell);
	if (!TestNotNull(TEXT("unita' della squadra 0"), TeamZero)) { DestroyCameraWorld(World); return false; }

	TestTrue(TEXT("con la selezione: centro mappa"), Cam->FrameHome(TeamZero) == ERTHomeFraming::MapCenter);
	TestTrue(TEXT("senza selezione: centro mappa"), Cam->FrameHome(nullptr) == ERTHomeFraming::MapCenter);
	TestTrue(TEXT("e il pivot non sta sull'unita' della squadra 0"),
		FVector::Dist2D(Cam->GetCameraPivot(), HomeTestCellWorld(Map, HomeSelectedCell)) > 1.0);

	DestroyCameraWorld(World);
	return true;
}

/**
 * In osservazione `Home` va sul centro mappa, e la posizione delle unita' non cambia l'inquadratura.
 *
 * E' il criterio 4 della issue — il `Replay` — applicato alle osservazioni che esistono: autobattle e scenari,
 * dove il giocatore non comanda una squadra. Il centro deriva dalle celle, quindi spostare le unita' non lo
 * deve muovere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraHomeObservationTest,
	"RefactorTactics.Camera.HomeWhileObservingFramesTheMapNotTheUnits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraHomeObservationTest::RunTest(const FString&)
{
	FHomeTestScene S;
	if (!S.Build(*this, /*PlayerTeam=*/ 0)) { return false; }
	ARTUnit* Mine = SpawnCameraTestUnit(S.World, 0, HomeSelectedCell);
	ARTUnit* Mate = SpawnCameraTestUnit(S.World, 0, HomeMateCell);
	if (!TestNotNull(TEXT("unita' 1"), Mine) || !TestNotNull(TEXT("unita' 2"), Mate)) { return false; }

	TestTrue(TEXT("con la selezione: centro mappa"),
		S.Cam->FrameHome(Mine, /*bObservationOnly=*/ true) == ERTHomeFraming::MapCenter);
	const FVector First = S.Cam->GetCameraPivot();
	TestTrue(TEXT("il pivot sta sul centro mappa"), FVector::Dist2D(First, HomeTestMapCentre(S.Map)) < 1.0);

	// Le unita' si spostano: l'inquadratura d'osservazione non le segue.
	Mine->Cell = HomeEnemyCell;
	Mate->Cell = FRTCellId(4, 1);
	S.Cam->FrameHome(nullptr, /*bObservationOnly=*/ true);
	TestTrue(TEXT("mutate le unita', l'inquadratura non cambia"), FVector::Dist2D(S.Cam->GetCameraPivot(), First) < 1.0);
	return true;
}

/**
 * Premuto dalla vista strategica, `Home` mette il pivot sulla quota TATTICA del piano dell'unita'.
 *
 * 🔴 In strategica la board e' posata con i piani separati, e torna alla quota tattica solo al prossimo
 * `ARTHUD::UpdateObserverVeil`. Leggere la quota posata metterebbe il pivot un piano sopra l'unita' — il
 * difetto di #887 per `F` (rilievo di review). Il caso vuole un'unita' su un piano diverso dallo 0: sul piano 0
 * le due quote coincidono e il test non discriminerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraHomeFromStrategicUsesTacticalHeightTest,
	"RefactorTactics.Camera.HomeFromStrategicFramesTheTacticalFloorHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraHomeFromStrategicUsesTacticalHeightTest::RunTest(const FString&)
{
	FHomeTestScene S;
	if (!S.Build(*this, /*PlayerTeam=*/ 0)) { return false; }

	// Un secondo piano sopra la cella della selezione.
	const FRTCellId Upper(HomeSelectedCell.X, HomeSelectedCell.Y, 1);
	S.Map->MapAsset->AddOrUpdateCell(FRTHexCellData(Upper));
	S.Map->MapAsset->SortCells();
	ARTUnit* Selected = SpawnCameraTestUnit(S.World, 0, Upper);
	if (!TestNotNull(TEXT("unita' al piano 1"), Selected)) { return false; }

	FVector Origin; float HexSize; float TacticalLayer;
	S.Map->GetAuthoringHexContext(Origin, HexSize, TacticalLayer);
	const double TacticalZ = URTHexLibrary::AxialToWorld(Upper, Origin, HexSize, TacticalLayer).Z;

	// In strategica, con la board posata come la posa l'HUD.
	S.Map->SetStrategicLayerSeparationForTest(2.f);
	S.Cam->ToggleStrategicView();
	S.Map->SyncStrategicPresentation(/*bStrategic=*/ true);
	FVector PosedOrigin; float PosedHexSize; float PosedLayer;
	S.Map->GetHexContext(PosedOrigin, PosedHexSize, PosedLayer);
	if (!TestTrue(TEXT("precondizione: siamo in strategica"), S.Cam->IsStrategicView())
		|| !TestTrue(TEXT("precondizione: la board posata separa i piani"), PosedLayer > TacticalLayer + 1.f))
	{
		return false;
	}

	TestTrue(TEXT("il ramo e' la selezione"), S.Cam->FrameHome(Selected) == ERTHomeFraming::Selection);
	TestFalse(TEXT("Home e' uscito dalla strategica"), S.Cam->IsStrategicView());
	TestEqual(TEXT("il pivot sta alla quota TATTICA del piano 1"), S.Cam->GetCameraPivot().Z, TacticalZ);
	return true;
}

/**
 * In uno scenario il tasto `Home` riporta il campo intero, come l'inquadratura con cui lo scenario si apre.
 *
 * Il controllo e' `Camera.HomeKeyFramesTheControllersSelection`: stessa scena, senza scenario, e `Home` va sulla
 * selezione. Qui l'unica differenza e' `ScenarioToRun` (rilievo di review: prima di #1837 `Home` in uno scenario
 * mostrava la mappa intera, e non va perso).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraHomeKeyInScenarioTest,
	"RefactorTactics.Camera.HomeKeyInAScenarioFramesTheWholeMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraHomeKeyInScenarioTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnCameraTestMap(World);
	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, 0);
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam) || !TestNotNull(TEXT("controller"), PC)
		|| !TestNotNull(TEXT("game mode"), GameMode))
	{
		DestroyCameraWorld(World);
		return false;
	}
	PC->Possess(Cam);
	ARTUnit* Selected = SpawnCameraTestUnit(World, 0, HomeSelectedCell);
	if (!TestNotNull(TEXT("unita'"), Selected)) { DestroyCameraWorld(World); return false; }
	PC->SelectUnit(Selected, /*bRecordAsPlayerInput=*/ false);

	if (!TestFalse(TEXT("controllo: senza scenario la sessione non e' un'osservazione"), PC->IsObservationSession()))
	{
		DestroyCameraWorld(World);
		return false;
	}
	GameMode->ScenarioToRun = TEXT("Visual.Camera.HomeTest");
	if (!TestTrue(TEXT("con ScenarioToRun la sessione e' un'osservazione"), PC->IsObservationSession()))
	{
		DestroyCameraWorld(World);
		return false;
	}

	PC->RecenterForTest();
	TestTrue(TEXT("Home va sul centro mappa"), FVector::Dist2D(Cam->GetCameraPivot(), HomeTestMapCentre(Map)) < 1.0);
	TestTrue(TEXT("e non sulla selezione"),
		FVector::Dist2D(Cam->GetCameraPivot(), HomeTestCellWorld(Map, HomeSelectedCell)) > 1.0);

	DestroyCameraWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------
// #1838 - l'edge pan: il cursore al bordo del viewport entra nella pipeline di pan esistente.
// ---------------------------------------------------------------------------------------------------

namespace
{
	const FVector2D EdgePanViewport(1920.0, 1080.0);
	constexpr float EdgePanBand = 16.f;
	/** Un fotogramma a 60 fps: a questo tempo `AddPlanarVelocity` percorre cio' che `PanSpeed = 25` percorreva per fotogramma (#3792). */
	constexpr float EdgePanFrame60 = 1.f / 60.f;

	/**
	 * Mappa ampia (raggio 8): i test dell'edge pan misurano DIREZIONE e VELOCITA', e su una mappa piccola il
	 * limite del pivot morderebbe al primo fotogramma, confondendo un pan fermo con un pan clampato.
	 */
	ARTHexMapActor* SpawnEdgePanTestMap(UWorld* World)
	{
		URTHexMapAsset* Asset = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(4, 1, 0), /*Radius=*/ 8))
		{
			Asset->AddOrUpdateCell(FRTHexCellData(Id));
		}
		Asset->SortCells();
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor) { MapActor->MapAsset = Asset; }
		return MapActor;
	}

	/** Il cursore a `Distance` pixel dal bordo alto, a meta' larghezza. */
	FVector2D EdgePanTopCursor(double Distance) { return FVector2D(EdgePanViewport.X * 0.5, Distance); }

	/**
	 * L'ultimo pixel che un cursore vero raggiunge: `Size - 1`. Il motore tronca la posizione a intero, quindi
	 * `Size` non arriva mai — e i test che lo iniettavano nascondevano i bordi destro e basso piu' lenti.
	 */
	const double EdgePanLastX = EdgePanViewport.X - 1.0;
	const double EdgePanLastY = EdgePanViewport.Y - 1.0;

	/**
	 * Camera con l'edge pan ACCESO, scritto e non letto dal default: `bEdgePanEnabled` e' `Config`, e un
	 * `Game.ini` locale che lo spegne non deve far cadere i test (rilievo di review).
	 */
	ARTCameraPawn* SpawnEdgePanCamera(UWorld* World)
	{
		ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
		if (Cam) { Cam->SetEdgePanEnabled(true); }
		return Cam;
	}
}

/**
 * L'asse: zero fuori dalla banda, cresce con la penetrazione, e nessun pan con il cursore fuori dal viewport.
 *
 * Criteri 2 (nessun movimento fuori dalla banda) e 3 (velocita' monotona nella penetrazione) sulla funzione
 * pura; il «zero lavoro» lo misura `Camera.EdgePanDoesNoWorkAtRestAndStopsTheSameFrame`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraEdgePanAxisTest,
	"RefactorTactics.Camera.EdgePanAxisIsZeroOutsideTheBandAndGrowsWithPenetration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraEdgePanAxisTest::RunTest(const FString&)
{
	const auto Axis = [](const FVector2D& Cursor) { return ARTCameraPawn::ComputeEdgePanAxis(Cursor, EdgePanViewport, EdgePanBand); };

	TestTrue(TEXT("al centro: zero"), Axis(EdgePanViewport * 0.5).IsNearlyZero());
	TestTrue(TEXT("sul bordo interno della banda: zero"), Axis(EdgePanTopCursor(EdgePanBand)).IsNearlyZero());
	TestTrue(TEXT("appena dentro la banda: avanti"), Axis(EdgePanTopCursor(EdgePanBand - 1.0)).Y > 0.0);
	TestEqual(TEXT("sul bordo del viewport: penetrazione piena"), Axis(EdgePanTopCursor(0.0)).Y, 1.0);

	// Monotona: piu' vicino al bordo, piu' veloce. Un passo alla volta, dal bordo interno al bordo esterno.
	double Previous = 0.0;
	bool bMonotonic = true;
	for (double Distance = EdgePanBand; Distance >= 0.0; Distance -= 1.0)
	{
		const double Y = Axis(EdgePanTopCursor(Distance)).Y;
		bMonotonic &= Y >= Previous;
		Previous = Y;
	}
	TestTrue(TEXT("la penetrazione cresce verso il bordo"), bMonotonic);

	// I quattro bordi danno i quattro versi; negli angoli le due componenti si sommano.
	// 🔴 I quattro bordi arrivano TUTTI alla penetrazione piena, sull'ultimo pixel che il cursore raggiunge.
	TestEqual(TEXT("bordo basso, ultimo pixel: indietro, pieno"), Axis(FVector2D(960.0, EdgePanLastY)).Y, -1.0);
	TestEqual(TEXT("bordo sinistro: a sinistra, pieno"), Axis(FVector2D(0.0, 540.0)).X, -1.0);
	TestEqual(TEXT("bordo destro, ultimo pixel: a destra, pieno"), Axis(FVector2D(EdgePanLastX, 540.0)).X, 1.0);
	TestTrue(TEXT("`Size` non e' un pixel del viewport: zero"), Axis(FVector2D(EdgePanViewport.X, 540.0)).IsNearlyZero());
	const FVector2D Corner = Axis(FVector2D(EdgePanLastX, 0.0));
	TestTrue(TEXT("angolo alto a destra: avanti e a destra"), Corner.X > 0.0 && Corner.Y > 0.0);

	// Fuori dal viewport — un altro monitor — e senza viewport: nessun pan.
	TestTrue(TEXT("cursore fuori dal viewport: zero"), Axis(FVector2D(-5.0, 540.0)).IsNearlyZero());
	TestTrue(TEXT("viewport senza area: zero"),
		ARTCameraPawn::ComputeEdgePanAxis(FVector2D::ZeroVector, FVector2D::ZeroVector, EdgePanBand).IsNearlyZero());
	return true;
}

/**
 * Il pan va dove guarda la vista, in tutti e otto gli orientamenti (criterio 1).
 *
 * Il bordo alto e' «avanti sullo schermo»: con la vista girata di 90 gradi deve andare di lato nel mondo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraEdgePanFollowsYawTest,
	"RefactorTactics.Camera.EdgePanFollowsTheViewYawInAllEightOrientations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraEdgePanFollowsYawTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnEdgePanTestMap(World);
	ARTCameraPawn* Cam = SpawnEdgePanCamera(World);
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	const FVector Centre = HomeTestMapCentre(Map);
	const FVector2D Forward = ARTCameraPawn::ComputeEdgePanAxis(EdgePanTopCursor(0.0), EdgePanViewport, EdgePanBand);
	const FVector2D Rightward = ARTCameraPawn::ComputeEdgePanAxis(FVector2D(EdgePanLastX, 540.0), EdgePanViewport, EdgePanBand);
	for (int32 Step = 0; Step < 8; ++Step)
	{
		const float Yaw = Cam->GetCameraYaw();
		const FVector ViewForward = FRotator(0.f, Yaw, 0.f).RotateVector(FVector::ForwardVector);
		const FVector ViewRight = FRotator(0.f, Yaw, 0.f).RotateVector(FVector::RightVector);

		Cam->SetCameraPivot(Centre);
		Cam->ApplyEdgePan(Forward, EdgePanFrame60);
		const FVector DeltaForward = (Cam->GetCameraPivot() - Centre).GetSafeNormal2D();
		TestTrue(FString::Printf(TEXT("yaw %.0f: il bordo alto va avanti nella vista"), Yaw),
			FVector::DotProduct(DeltaForward, ViewForward) > 0.99);

		Cam->SetCameraPivot(Centre);
		Cam->ApplyEdgePan(Rightward, EdgePanFrame60);
		const FVector DeltaRight = (Cam->GetCameraPivot() - Centre).GetSafeNormal2D();
		TestTrue(FString::Printf(TEXT("yaw %.0f: il bordo destro va a destra nella vista"), Yaw),
			FVector::DotProduct(DeltaRight, ViewRight) > 0.99);

		Cam->AddYaw(+1.f);
	}
	TestEqual(TEXT("otto passi riportano la vista dove era partita"), Cam->GetCameraYaw(), 0.f);

	DestroyCameraWorld(World);
	return true;
}

/**
 * 🔴 A penetrazione piena l'edge pan va alla stessa velocita' di `WASD`, allo stesso zoom (criterio 4).
 *
 * E' il discriminante della issue: un test che verifica solo «l'edge pan muove» passa anche su una seconda
 * formula di velocita' scritta accanto — esattamente il difetto che #1834 esiste per prevenire. Il confronto
 * si fa a due zoom, perche' una formula che ignorasse lo zoom coinciderebbe con `WASD` in un punto solo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraEdgePanMatchesWasdTest,
	"RefactorTactics.Camera.EdgePanMovesLikeWasdAtTheSameZoom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraEdgePanMatchesWasdTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnEdgePanTestMap(World);
	ARTCameraPawn* Cam = SpawnEdgePanCamera(World);
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	const FVector Centre = HomeTestMapCentre(Map);
	const FVector2D Full = ARTCameraPawn::ComputeEdgePanAxis(EdgePanTopCursor(0.0), EdgePanViewport, EdgePanBand);

	double FirstStep = 0.0;
	for (const float Alpha : { 0.1f, 0.6f })
	{
		Cam->SetZoomAlpha(Alpha);
		Cam->SetCameraPivot(Centre);
		Cam->ApplyEdgePan(Full, EdgePanFrame60);
		const double EdgeStep = FVector::Dist2D(Cam->GetCameraPivot(), Centre);

		Cam->SetCameraPivot(Centre);
		Cam->AddPlanarVelocity(FVector2D(0.0, 1.0), EdgePanFrame60); // `W` tenuto: lo stesso ingresso che `OnPan` usa
		const double WasdStep = FVector::Dist2D(Cam->GetCameraPivot(), Centre);

		TestTrue(FString::Printf(TEXT("alpha %.1f: l'edge pan muove"), Alpha), EdgeStep > 1.0);
		TestTrue(FString::Printf(TEXT("alpha %.1f: alla stessa velocita' di WASD"), Alpha), FMath::IsNearlyEqual(EdgeStep, WasdStep, 0.01));
		if (FirstStep == 0.0) { FirstStep = EdgeStep; }
		else { TestTrue(TEXT("e la velocita' cambia con lo zoom, come quella di WASD"), EdgeStep > FirstStep); }
	}

	// A meta' penetrazione, meno della penetrazione piena: la velocita' viene dall'asse, non da una costante.
	Cam->SetCameraPivot(Centre);
	Cam->ApplyEdgePan(ARTCameraPawn::ComputeEdgePanAxis(EdgePanTopCursor(EdgePanBand * 0.5), EdgePanViewport, EdgePanBand), EdgePanFrame60);
	const double Half = FVector::Dist2D(Cam->GetCameraPivot(), Centre);
	Cam->SetCameraPivot(Centre);
	Cam->ApplyEdgePan(Full, EdgePanFrame60);
	TestTrue(TEXT("a meta' banda va piu' piano che sul bordo"), Half > 0.0 && Half < FVector::Dist2D(Cam->GetCameraPivot(), Centre));

	DestroyCameraWorld(World);
	return true;
}

/**
 * #3792 - il pan percorre la stessa distanza a 30, 60 e 144 fps.
 *
 * Il difetto: la distanza era per CHIAMATA, e le chiamate sono una per fotogramma. Si tiene lo stesso input
 * per un sesto di secondo (5, 10 e 24 fotogrammi), spezzato in fotogrammi di durata diversa: la distanza deve coincidere. Vale per i due
 * ingressi a velocita' — `AddPlanarVelocity` (`WASD`) ed `ApplyEdgePan` — e `Camera.EdgePanMovesLikeWasdAtTheSameZoom`
 * resta vero a ogni frame rate.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraPanIsFrameRateIndependentTest,
	"RefactorTactics.Camera.PanCoversTheSameDistanceAtAnyFrameRate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraPanIsFrameRateIndependentTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnEdgePanTestMap(World);
	ARTCameraPawn* Cam = SpawnEdgePanCamera(World);
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	const FVector Centre = HomeTestMapCentre(Map);
	const FVector2D Full = ARTCameraPawn::ComputeEdgePanAxis(EdgePanTopCursor(0.0), EdgePanViewport, EdgePanBand);
	const FVector2D Forward(0.0, 1.0);
	Cam->SetZoomAlpha(0.3f);

	double WasdAt60 = 0.0;
	double EdgeAt60 = 0.0;
	for (const int32 Fps : { 30, 60, 144 })
	{
		const float Dt = 1.f / static_cast<float>(Fps);

		Cam->SetCameraPivot(Centre);
		for (int32 Frame = 0; Frame < Fps / 6; ++Frame) { Cam->AddPlanarVelocity(Forward, Dt); }
		const double Wasd = FVector::Dist2D(Cam->GetCameraPivot(), Centre);

		Cam->SetCameraPivot(Centre);
		for (int32 Frame = 0; Frame < Fps / 6; ++Frame) { Cam->ApplyEdgePan(Full, Dt); }
		const double Edge = FVector::Dist2D(Cam->GetCameraPivot(), Centre);

		TestTrue(FString::Printf(TEXT("%d fps: WASD muove"), Fps), Wasd > 1.0);
		if (Fps == 60) { WasdAt60 = Wasd; EdgeAt60 = Edge; }
		else
		{
			// Si confronta con i 60 fps dopo il ciclo: serve prima il valore di riferimento.
			Cam->SetCameraPivot(Centre);
			for (int32 Frame = 0; Frame < 10; ++Frame) { Cam->AddPlanarVelocity(Forward, 1.f / 60.f); }
			const double Ref = FVector::Dist2D(Cam->GetCameraPivot(), Centre);
			TestTrue(FString::Printf(TEXT("%d fps: WASD percorre quanto a 60 (%.2f vs %.2f)"), Fps, Wasd, Ref),
				FMath::IsNearlyEqual(Wasd, Ref, Ref * 0.001));
			Cam->SetCameraPivot(Centre);
			for (int32 Frame = 0; Frame < 10; ++Frame) { Cam->ApplyEdgePan(Full, 1.f / 60.f); }
			const double EdgeRef = FVector::Dist2D(Cam->GetCameraPivot(), Centre);
			TestTrue(FString::Printf(TEXT("%d fps: l'edge pan percorre quanto a 60 (%.2f vs %.2f)"), Fps, Edge, EdgeRef),
				FMath::IsNearlyEqual(Edge, EdgeRef, EdgeRef * 0.001));
			TestTrue(FString::Printf(TEXT("%d fps: edge pan e WASD restano allo stesso profilo"), Fps),
				FMath::IsNearlyEqual(Edge, Wasd, Wasd * 0.001));
		}
	}
	TestTrue(TEXT("a 60 fps edge pan e WASD coincidono"), FMath::IsNearlyEqual(EdgeAt60, WasdAt60, WasdAt60 * 0.001));

	// Non vacuo: se il pivot avesse toccato un limite, la distanza sarebbe uguale a ogni frame rate per il limite,
	// non per il tempo. Un sesto di secondo sono 10 fotogrammi a 60 fps, e la distanza e' lineare: 10 passi singoli.
	Cam->SetCameraPivot(Centre);
	Cam->AddPlanarVelocity(Forward, 1.f / 60.f);
	const double OneFrame = FVector::Dist2D(Cam->GetCameraPivot(), Centre);
	TestTrue(TEXT("la prova non e' limitata dai bordi: 10 fotogrammi a 60 fps sono 10 passi"),
		FMath::IsNearlyEqual(WasdAt60, OneFrame * 10.0, OneFrame * 0.01));

	DestroyCameraWorld(World);
	return true;
}

/**
 * #3792 - a 60 fps il pan va come prima di #3792: 25 unita' per fotogramma, a distanza di default.
 * E' la taratura che la riscrittura di `PanSpeed` in unita' al secondo (1500) deve conservare, e lo spostamento
 * a trascinamento (`AddPlanarMovement`) resta per spostamento: non prende il tempo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraPanKeepsTheSixtyFpsFeelTest,
	"RefactorTactics.Camera.PanAtSixtyFpsMovesAsItDidPerFrame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraPanKeepsTheSixtyFpsFeelTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnEdgePanTestMap(World);
	ARTCameraPawn* Cam = SpawnEdgePanCamera(World);
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	const FVector Centre = HomeTestMapCentre(Map);
	const FVector2D Forward(0.0, 1.0);

	Cam->SetCameraPivot(Centre);
	Cam->AddPlanarMovement(Forward);
	const double Displacement = FVector::Dist2D(Cam->GetCameraPivot(), Centre);

	Cam->SetCameraPivot(Centre);
	Cam->AddPlanarVelocity(Forward, EdgePanFrame60);
	const double Velocity = FVector::Dist2D(Cam->GetCameraPivot(), Centre);

	TestTrue(TEXT("lo spostamento di un'unita' d'asse e' cio' che il fotogramma a 60 fps percorre"),
		FMath::IsNearlyEqual(Displacement, Velocity, 0.01));
	TestTrue(TEXT("un tempo non positivo non muove"), [&]
	{
		Cam->SetCameraPivot(Centre);
		Cam->AddPlanarVelocity(Forward, 0.f);
		Cam->AddPlanarVelocity(Forward, -0.1f);
		return FVector::Dist2D(Cam->GetCameraPivot(), Centre) < KINDA_SMALL_NUMBER;
	}());

	DestroyCameraWorld(World);
	return true;
}

/**
 * Fuori dalla banda nessun movimento e nessun aggiornamento; uscendo dalla banda il pivot si ferma nello
 * stesso fotogramma (criteri 2 e 5). Il contatore e' il «lavoro» che la issue chiede di misurare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraEdgePanRestTest,
	"RefactorTactics.Camera.EdgePanDoesNoWorkAtRestAndStopsTheSameFrame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraEdgePanRestTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnEdgePanTestMap(World);
	ARTCameraPawn* Cam = SpawnEdgePanCamera(World);
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	Cam->SetCameraPivot(HomeTestMapCentre(Map));

	const FVector2D Rest = ARTCameraPawn::ComputeEdgePanAxis(EdgePanViewport * 0.5, EdgePanViewport, EdgePanBand);
	const FVector Start = Cam->GetCameraPivot();
	for (int32 i = 0; i < 30; ++i) { TestFalse(TEXT("a riposo non muove"), Cam->ApplyEdgePan(Rest, EdgePanFrame60)); }
	TestEqual(TEXT("trenta fotogrammi a riposo: zero aggiornamenti"), Cam->GetEdgePanUpdatesForTest(), 0);
	TestEqual(TEXT("e il pivot e' fermo"), Cam->GetCameraPivot(), Start);

	// Nella banda per qualche fotogramma, poi fuori: niente deriva.
	const FVector2D InBand = ARTCameraPawn::ComputeEdgePanAxis(EdgePanTopCursor(2.0), EdgePanViewport, EdgePanBand);
	for (int32 i = 0; i < 5; ++i) { Cam->ApplyEdgePan(InBand, EdgePanFrame60); }
	TestEqual(TEXT("nella banda: un aggiornamento per fotogramma"), Cam->GetEdgePanUpdatesForTest(), 5);
	const FVector Left = Cam->GetCameraPivot();
	Cam->ApplyEdgePan(Rest, EdgePanFrame60);
	TestEqual(TEXT("uscito dalla banda, il pivot si ferma nello stesso fotogramma"), Cam->GetCameraPivot(), Left);
	TestEqual(TEXT("senza un aggiornamento in piu'"), Cam->GetEdgePanUpdatesForTest(), 5);

	DestroyCameraWorld(World);
	return true;
}

/** Con l'interruttore spento l'edge pan non si attiva in nessuna posizione del cursore (criterio 7). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraEdgePanSwitchTest,
	"RefactorTactics.Camera.EdgePanSwitchedOffNeverMoves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraEdgePanSwitchTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnEdgePanTestMap(World);
	ARTCameraPawn* Cam = SpawnEdgePanCamera(World);
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	// Il default acceso e' nel codice (`bEdgePanEnabled = true`); qui non si asserisce, perche' e' `Config` e lo
	// puo' cambiare un `Game.ini` locale.

	Cam->SetEdgePanEnabled(false);
	Cam->SetCameraPivot(HomeTestMapCentre(Map));
	const FVector Start = Cam->GetCameraPivot();
	for (const FVector2D& Cursor : { EdgePanTopCursor(0.0), FVector2D(0.0, 540.0), FVector2D(EdgePanLastX, 540.0),
		FVector2D(960.0, EdgePanLastY), FVector2D(0.0, 0.0), FVector2D(EdgePanLastX, EdgePanLastY) })
	{
		TestFalse(TEXT("spento non muove"), Cam->ApplyEdgePan(ARTCameraPawn::ComputeEdgePanAxis(Cursor, EdgePanViewport, EdgePanBand), EdgePanFrame60));
	}
	TestEqual(TEXT("il pivot e' fermo"), Cam->GetCameraPivot(), Start);
	TestEqual(TEXT("e nessun aggiornamento"), Cam->GetEdgePanUpdatesForTest(), 0);

	DestroyCameraWorld(World);
	return true;
}

/**
 * Il pivot resta dentro i limiti del viewport (criterio 6): l'edge pan passa dalla porta unica, e tenuto
 * contro un bordo arriva **esattamente** dove arriva `WASD` tenuto per lo stesso numero di fotogrammi.
 *
 * ⚠️ Il «dominio di conoscenza» del criterio e' `N/A`: era #1836, chiusa senza codice per D-373.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraEdgePanBoundsTest,
	"RefactorTactics.Camera.EdgePanStaysInsideTheViewportBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraEdgePanBoundsTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnCameraTestMap(World); // piccola: il limite DEVE mordere
	ARTCameraPawn* Cam = SpawnEdgePanCamera(World);
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	Cam->SetViewportMetricsForTest(16.f/9.f, 90.f);
	const FVector Centre = HomeTestMapCentre(Map);
	const FVector2D Full = ARTCameraPawn::ComputeEdgePanAxis(EdgePanTopCursor(0.0), EdgePanViewport, EdgePanBand);

	// Un passo libero, dal centro: e' la misura con cui si vede che il limite morde.
	Cam->SetCameraPivot(Centre);
	Cam->ApplyEdgePan(Full, EdgePanFrame60);
	const double OneStep = FVector::Dist2D(Cam->GetCameraPivot(), Centre);

	Cam->SetCameraPivot(Centre);
	for (int32 i = 0; i < 400; ++i) { Cam->ApplyEdgePan(Full, EdgePanFrame60); }
	const FVector EdgeEnd = Cam->GetCameraPivot();

	Cam->SetCameraPivot(Centre);
	for (int32 i = 0; i < 400; ++i) { Cam->AddPlanarMovement(FVector2D(0.0, 1.0)); }
	const FVector WasdEnd = Cam->GetCameraPivot();

	if (!TestTrue(TEXT("precondizione: il limite ha morso (400 passi liberi andrebbero molto piu' lontano)"),
		OneStep > 1.0 && FVector::Dist2D(EdgeEnd, Centre) < 400.0 * OneStep * 0.5))
	{
		DestroyCameraWorld(World);
		return false;
	}
	TestTrue(TEXT("l'edge pan arriva dove arriva WASD: la stessa porta, lo stesso limite"), FVector::Dist2D(EdgeEnd, WasdEnd) < 0.01);
	Cam->SetCameraPivot(EdgeEnd);
	const int32 Lavoro = Cam->GetEdgePanUpdatesForTest();
	TestFalse(TEXT("contro il limite ApplyEdgePan risponde che non ha mosso"), Cam->ApplyEdgePan(Full, EdgePanFrame60));
	TestEqual(TEXT("ma il lavoro c'e' stato"), Cam->GetEdgePanUpdatesForTest(), Lavoro + 1);
	Cam->SetCameraPivot(EdgeEnd);
	TestTrue(TEXT("e quel punto e' dentro i limiti: riscriverlo non lo sposta"), FVector::Dist2D(Cam->GetCameraPivot(), EdgeEnd) < 0.01);

	DestroyCameraWorld(World);
	return true;
}

/**
 * Con un modale aperto o un gesto camera in corso l'edge pan non si attiva (criterio 8), e il controller lo
 * applica davvero: dal suo ingresso, con cursore e viewport iniettati.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraEdgePanRefusalsTest,
	"RefactorTactics.Camera.EdgePanIsRefusedDuringModalsAndCameraGestures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraEdgePanRefusalsTest::RunTest(const FString&)
{
	// La regola. «Bloccato» e' `IsGameplayInputBlocked()`, cioe' il contesto `Modal` del contratto del puntatore:
	// headless non c'e' un navigatore per aprire la pausa, quindi il ramo del modale si prova qui.
	TestFalse(TEXT("modale: no"), ARTPlayerController::EdgePanAllowed(/*bGameplayInputBlocked=*/ true, false));
	TestTrue(TEXT("nessun modale, nessun gesto: si'"), ARTPlayerController::EdgePanAllowed(false, false));
	TestFalse(TEXT("gesto camera in corso: no"), ARTPlayerController::EdgePanAllowed(false, /*bCameraGestureActive=*/ true));

	// Il controller: dal suo ingresso il pan parte, e un gesto camera lo ferma.
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* Map = SpawnEdgePanTestMap(World);
	ARTCameraPawn* Cam = SpawnEdgePanCamera(World);
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, 0);
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("camera"), Cam) || !TestNotNull(TEXT("controller"), PC))
	{
		DestroyCameraWorld(World);
		return false;
	}
	PC->Possess(Cam);
	Cam->SetCameraPivot(HomeTestMapCentre(Map));

	FVector Start = Cam->GetCameraPivot();
	TestTrue(TEXT("dal controller, il cursore al bordo muove la camera"), PC->UpdateEdgePanForTest(EdgePanTopCursor(0.0), EdgePanViewport));
	TestTrue(TEXT("e il pivot si e' mosso"), FVector::Dist2D(Cam->GetCameraPivot(), Start) > 1.0);

	PC->SetCameraModifierForTest(true);
	Start = Cam->GetCameraPivot();
	TestFalse(TEXT("con Ctrl premuto (un gesto camera) no"), PC->UpdateEdgePanForTest(EdgePanTopCursor(0.0), EdgePanViewport));
	TestEqual(TEXT("e il pivot e' fermo"), Cam->GetCameraPivot(), Start);
	PC->SetCameraModifierForTest(false);

	PC->SetOrbitingForTest(true);
	TestFalse(TEXT("con un'orbita in corso no"), PC->UpdateEdgePanForTest(EdgePanTopCursor(0.0), EdgePanViewport));
	TestEqual(TEXT("e il pivot e' fermo"), Cam->GetCameraPivot(), Start);
	PC->SetOrbitingForTest(false);

	Cam->SetEdgePanEnabled(false);
	TestFalse(TEXT("spento, dal controller no"), PC->UpdateEdgePanForTest(EdgePanTopCursor(0.0), EdgePanViewport));
	TestEqual(TEXT("e il pivot e' fermo"), Cam->GetCameraPivot(), Start);
	Cam->SetEdgePanEnabled(true);

	// I tasti hanno la precedenza: nello stesso fotogramma un `W` e il bordo alto non si sommano.
	PC->PanForTest(FVector2D(0.0, 1.0));
	const FVector AfterKeys = Cam->GetCameraPivot();
	TestFalse(TEXT("con W nello stesso fotogramma l'edge pan cede"), PC->UpdateEdgePanForTest(EdgePanTopCursor(0.0), EdgePanViewport));
	TestEqual(TEXT("e la vista si e' mossa una volta sola"), Cam->GetCameraPivot(), AfterKeys);
	TestTrue(TEXT("al fotogramma dopo, senza tasti, l'edge pan riparte"), PC->UpdateEdgePanForTest(EdgePanTopCursor(0.0), EdgePanViewport));

	TestFalse(TEXT("al centro dello schermo no"), PC->UpdateEdgePanForTest(EdgePanViewport * 0.5, EdgePanViewport));

	DestroyCameraWorld(World);
	return true;
}

// --- #1770 · il pivot e' uno stato, e il peek non lo tocca -------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraPeekDoesNotMoveThePivotTest,
	"RefactorTactics.Camera.PeekOffsetsTheViewWithoutMovingThePivot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraPeekDoesNotMoveThePivotTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	const FVector Pivot(120.f, -80.f, 40.f);
	Cam->SetCameraPivotForTest(Pivot);

	// Un offset **dentro** il limite, cosi' il test misura la separazione fra pivot e posizione e non il
	// clamp: quello ha il proprio test qui sotto.
	const FVector Peek(60.f, 0.f, 0.f);
	Cam->SetPeekOffset(Peek);

	TestEqual(TEXT("il pivot NON si e' mosso"), Cam->GetCameraPivot(), Pivot);
	TestEqual(TEXT("la posizione e' pivot + peek"), Cam->GetActorLocation(), Pivot + Peek);

	// Il rientro deve riportare **esattamente** al punto di partenza: e' la proprieta' per cui il pivot
	// esiste come campo separato invece di essere la posizione.
	Cam->SetPeekOffset(FVector::ZeroVector);
	TestEqual(TEXT("al rientro la posizione torna esattamente sul pivot"), Cam->GetActorLocation(), Pivot);
	TestEqual(TEXT("e il pivot e' ancora quello"), Cam->GetCameraPivot(), Pivot);

	DestroyCameraWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraPeekIsLimitedInLengthTest,
	"RefactorTactics.Camera.PeekIsLimitedInLengthNotPerAxis",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraPeekIsLimitedInLengthTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	Cam->SetCameraPivotForTest(FVector::ZeroVector);
	Cam->SetMaxPeekDistanceForTest(100.f);

	// In DIAGONALE, che e' il caso che distingue un limite sulla lunghezza da un clamp per asse: con il
	// secondo questo vettore passerebbe intatto (ogni componente vale 90, sotto il limite) e il peek
	// diagonale sarebbe piu' lungo di quello sui lati.
	Cam->SetPeekOffset(FVector(90.f, 90.f, 0.f));

	TestTrue(TEXT("il peek diagonale e' limitato in lunghezza"),
		FMath::IsNearlyEqual(Cam->GetPeekOffset().Size(), 100.f, 0.1f));
	TestEqual(TEXT("il pivot resta fermo"), Cam->GetCameraPivot(), FVector::ZeroVector);

	DestroyCameraWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraFocusPassesThroughBoundsTest,
	"RefactorTactics.Camera.FocusIsClampedLikeEveryOtherPivotWrite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraFocusPassesThroughBoundsTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	Cam->SetBoundsMarginForTest(1.f);

	// 🔴 Prima di `#1770` `FocusOn` scriveva la posizione **nuda**: era l'unica delle cinque scritture di
	// pivot senza clamp, quindi inquadrare un punto lontanissimo portava la camera dove il pan si rifiuta
	// di andare. Un invariante che vale in quattro punti su cinque non e' un invariante.
	const FVector FarAway(500000.f, 500000.f, 0.f);
	Cam->FocusOn(FarAway);

	TestTrue(TEXT("il focus lontano viene limitato come il pan"),
		Cam->GetCameraPivot().X < FarAway.X * 0.5);

	DestroyCameraWorld(World);
	return true;
}

// --- #1771 / #1772 · i gesti del modificatore camera -------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraModifierDragOrbitsOnlyPastThresholdTest,
	"RefactorTactics.Camera.ModifierDragOrbitsOnlyPastTheClickThreshold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraModifierDragOrbitsOnlyPastThresholdTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("camera"), Cam) || !TestNotNull(TEXT("controller"), PC))
	{
		DestroyCameraWorld(World);
		return false;
	}
	PC->Possess(Cam);
	Cam->SetSensitivitiesForTest(1.f, 1.f);
	PC->SetClickDragThresholdForTest(10.f);

	const float YawBefore = Cam->GetCameraYaw();

	// Sotto soglia: il gesto e' ancora un click potenziale, e la vista NON deve muoversi. Senza questa
	// guardia un click con un pixel di tremolio ruoterebbe la vista di un grado, e il set-pivot
	// arriverebbe su un'inquadratura gia' cambiata.
	PC->SetCameraModifierForTest(true);
	PC->SetModifierPrimaryDownForTest(true, 0.f);
	TestTrue(TEXT("il gesto Ctrl consuma il movimento"), PC->RouteCameraGestureForTest(FVector2D(3.f, 0.f)));
	TestEqual(TEXT("sotto soglia lo yaw non cambia"), Cam->GetCameraYaw(), YawBefore);

	// Oltre soglia: da qui in poi e' un'orbita.
	PC->RouteCameraGestureForTest(FVector2D(20.f, 0.f));
	TestNotEqual(TEXT("oltre soglia la vista orbita"), Cam->GetCameraYaw(), YawBefore);

	DestroyCameraWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraPeekReturnsToZeroTest,
	"RefactorTactics.Camera.PeekReturnsToZeroOnlyWhenTheModifierIsReleased",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraPeekReturnsToZeroTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("camera"), Cam) || !TestNotNull(TEXT("controller"), PC))
	{
		DestroyCameraWorld(World);
		return false;
	}
	PC->Possess(Cam);

	const FVector Pivot(100.f, 200.f, 0.f);
	Cam->SetCameraPivotForTest(Pivot);

	// `Ctrl` da solo + movimento = peek.
	PC->SetCameraModifierForTest(true);
	TestTrue(TEXT("Ctrl da solo consuma il movimento come peek"),
		PC->RouteCameraGestureForTest(FVector2D(20.f, 0.f)));
	TestFalse(TEXT("il peek si e' aperto"), Cam->GetPeekOffset().IsNearlyZero());
	TestEqual(TEXT("e il pivot non si e' mosso"), Cam->GetCameraPivot(), Pivot);

	// Con `Ctrl` ancora premuto il rientro NON deve partire: tirare contro il giocatore mentre sta
	// guidando e' la forma piu' fastidiosa di camera automatica.
	const FVector Held = Cam->GetPeekOffset();
	PC->UpdatePeekReturnForTest(0.1f);
	TestEqual(TEXT("con Ctrl premuto il peek resta dov'e'"), Cam->GetPeekOffset(), Held);

	// Rilasciato `Ctrl`, rientra fino a zero — e il pivot e' sempre quello.
	PC->SetCameraModifierForTest(false);
	for (int32 i = 0; i < 200 && !Cam->GetPeekOffset().IsNearlyZero(); ++i)
	{
		PC->UpdatePeekReturnForTest(1.f / 60.f);
	}
	TestTrue(TEXT("il peek rientra a zero"), Cam->GetPeekOffset().IsNearlyZero());
	TestEqual(TEXT("e la camera torna esattamente sul pivot"), Cam->GetActorLocation(), Pivot);

	DestroyCameraWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraActiveLayerStaysWithinMapTest,
	"RefactorTactics.Camera.ActiveLayerStaysWithinTheLayersTheMapHas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraActiveLayerStaysWithinMapTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }

	// Mappa a DUE piani: senza il secondo, «il piano attivo si ferma» sarebbe vero anche di
	// un'implementazione che non cambia mai piano.
	URTHexMapAsset* Asset = NewObject<URTHexMapAsset>();
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), /*Radius=*/ 1))
	{
		Asset->AddOrUpdateCell(FRTHexCellData(Id));
		Asset->AddOrUpdateCell(FRTHexCellData(FRTCellId(Id.X, Id.Y, 1)));
	}
	Asset->SortCells();
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	if (!TestNotNull(TEXT("mappa"), MapActor)) { DestroyCameraWorld(World); return false; }
	MapActor->MapAsset = Asset;

	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("controller"), PC)) { DestroyCameraWorld(World); return false; }

	TestEqual(TEXT("si parte dal piano 0"), PC->GetActiveLayer(), 0);
	TestTrue(TEXT("si sale al piano 1"), PC->StepActiveLayer(+1));
	TestEqual(TEXT("il piano attivo e' 1"), PC->GetActiveLayer(), 1);

	// Il piano 2 non esiste: salirci darebbe un hover che non trova mai niente senza dire perche'.
	TestFalse(TEXT("non si sale a un piano che la mappa non ha"), PC->StepActiveLayer(+1));
	TestEqual(TEXT("il piano attivo resta 1"), PC->GetActiveLayer(), 1);

	TestTrue(TEXT("si scende"), PC->StepActiveLayer(-1));
	TestFalse(TEXT("e non si scende sotto il piano piu' basso"), PC->StepActiveLayer(-1));
	TestEqual(TEXT("il piano attivo resta 0"), PC->GetActiveLayer(), 0);

	DestroyCameraWorld(World);
	return true;
}

// --- #1774 · lo stato strategico e la sua isteresi ---------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraStrategicHysteresisTest,
	"RefactorTactics.Camera.StrategicViewHasHysteresisAndDoesNotOscillate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraStrategicHysteresisTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	Cam->SetArmLengthRangeForTest(/*Default=*/ 800.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);
	Cam->SetStrategicThresholdsForTest(/*Enter=*/ 2400.f, /*Exit=*/ 1900.f);

	// ⚠️ **Il passo di zoom e' fissato QUI, e senza questa riga il test mentirebbe per omissione.**
	// «Un solo scatto indietro non fa uscire» distingue l'isteresi da una soglia sola soltanto se un passo
	// resta piu' piccolo del divario fra le due soglie (500). Con `ZoomStep` lasciato al default il test
	// pinnerebbe implicitamente un valore di **taratura aperta**, e cadrebbe il giorno in cui il playtest
	// lo cambiasse — per una decisione di tuning, non per un difetto.
	Cam->SetZoomStepForTest(150.f);

	Cam->RecenterView(); // riporta il braccio a 800 e rilegge lo stato

	TestFalse(TEXT("a distanza di default la vista e' tattica"), Cam->IsStrategicView());

	// Allontanandosi si entra in strategica...
	for (int32 i = 0; i < 40 && !Cam->IsStrategicView(); ++i) { Cam->AddZoom(+1.f); }
	TestTrue(TEXT("allontanandosi si entra in vista strategica"), Cam->IsStrategicView());

	// ...e avvicinandosi di poco NON si esce: e' l'isteresi. Con una soglia sola, un solo scatto di
	// rotella sotto il valore farebbe uscire, e nell'intorno la vista sfarfallerebbe.
	Cam->AddZoom(-1.f);
	TestTrue(TEXT("un solo scatto indietro non fa uscire: c'e' isteresi"), Cam->IsStrategicView());

	// Solo scendendo sotto la soglia di uscita si torna tattici.
	for (int32 i = 0; i < 40 && Cam->IsStrategicView(); ++i) { Cam->AddZoom(-1.f); }
	TestFalse(TEXT("sotto la soglia di uscita si torna tattici"), Cam->IsStrategicView());

	DestroyCameraWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraStrategicThresholdsAreOrderedTest,
	"RefactorTactics.Camera.StrategicThresholdsAreOrderedInCodeNotOnlyInDocs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraStrategicThresholdsAreOrderedTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	Cam->SetArmLengthRangeForTest(800.f, 100.f, 4000.f);
	Cam->SetZoomStepForTest(150.f);

	// Soglie ROVESCIATE: i due campi sono `BlueprintReadWrite` e il loro `meta = (ClampMin)` vincola il
	// Details, non un `Set` da Blueprint. Se l'ordine vivesse solo nella documentazione, qui lo stato
	// entrerebbe e non uscirebbe piu'.
	Cam->SetStrategicThresholdsForTest(/*Enter=*/ 1900.f, /*Exit=*/ 2400.f);
	Cam->RecenterView();

	for (int32 i = 0; i < 40 && !Cam->IsStrategicView(); ++i) { Cam->AddZoom(+1.f); }
	TestTrue(TEXT("con le soglie rovesciate si entra comunque"), Cam->IsStrategicView());

	for (int32 i = 0; i < 40 && Cam->IsStrategicView(); ++i) { Cam->AddZoom(-1.f); }
	TestFalse(TEXT("e si esce comunque: l'ordine e' imposto in codice"), Cam->IsStrategicView());

	DestroyCameraWorld(World);
	return true;
}

// --- D-488 / D-495 · `Tab`: una scorciatoia verso la distanza, non una modalita' ----------------------

/**
 * `Tab` porta il braccio alla soglia d'ingresso, e il secondo `Tab` lo riporta ESATTAMENTE da dove era partito.
 *
 * 🔑 **L'uguaglianza esatta del braccio e' l'asserzione che porta il peso.** Un ritorno «tattico ma altrove»
 * sarebbe verde su `IsStrategicView` e sbagliato per chi gioca, che ritroverebbe la camera a un'altra
 * distanza. E la soglia raggiunta ESATTAMENTE prova che `Tab` non passa dall'alpha, dove l'arrotondamento
 * di `Lerp` potrebbe lasciare il braccio un ulp sotto `Enter` e la vista tattica.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraTabTogglesStrategicTest,
	"RefactorTactics.Camera.TabReachesTheStrategicThresholdAndTheSecondTabReturns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraTabTogglesStrategicTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { DestroyCameraWorld(World); return false; }

	Cam->SetArmLengthRangeForTest(/*Default=*/ 800.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);
	Cam->SetStrategicThresholdsForTest(/*Enter=*/ 2400.f, /*Exit=*/ 1900.f);
	Cam->SetZoomStepForTest(150.f);
	Cam->RecenterView();

	// Partenza tattica a una distanza che NON e' il default: un ritorno a `DefaultArmLength` non basterebbe.
	Cam->AddZoom(+1.f);
	const float Start = Arm->TargetArmLength;
	TestFalse(TEXT("si parte tattici"), Cam->IsStrategicView());
	TestNotEqual(TEXT("la partenza non e' il default"), Start, 800.f);

	Cam->ToggleStrategicView();
	TestTrue(TEXT("il primo Tab apre la vista strategica"), Cam->IsStrategicView());
	TestEqual(TEXT("e porta il braccio esattamente alla soglia d'ingresso"), Arm->TargetArmLength, 2400.f);

	Cam->ToggleStrategicView();
	TestFalse(TEXT("il secondo Tab torna tattici"), Cam->IsStrategicView());
	TestEqual(TEXT("ed esattamente alla distanza di partenza"), Arm->TargetArmLength, Start);

	DestroyCameraWorld(World);
	return true;
}

/**
 * In strategica si entra anche con la rotella: allora l'ultima distanza tattica cade fra le due soglie, e il
 * secondo `Tab` deve comunque USCIRE.
 *
 * 🔴 **E' il caso che D-488 non copre** («riporta alla distanza da cui era partita»), e senza il tetto su
 * `StrategicExitThreshold` tornare a quella distanza lascerebbe la vista strategica per isteresi: un `Tab`
 * che non fa niente. Verifica di mutazione: tolto il `Min` con `Exit`, l'asserzione su `IsStrategicView` cade.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraTabAfterWheelEntryTest,
	"RefactorTactics.Camera.TabAfterWheelEntryLeavesTheStrategicView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraTabAfterWheelEntryTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { DestroyCameraWorld(World); return false; }

	Cam->SetArmLengthRangeForTest(/*Default=*/ 800.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);
	Cam->SetStrategicThresholdsForTest(/*Enter=*/ 2400.f, /*Exit=*/ 1900.f);
	Cam->SetZoomStepForTest(150.f);
	Cam->RecenterView();

	for (int32 i = 0; i < 40 && !Cam->IsStrategicView(); ++i) { Cam->AddZoom(+1.f); }
	if (!TestTrue(TEXT("con la rotella si entra in strategica"), Cam->IsStrategicView()))
	{
		DestroyCameraWorld(World);
		return false;
	}

	// Il controllo che rende il caso quello giusto: l'ultima lettura tattica sta FRA le soglie.
	const float LastTactical = Cam->GetLastTacticalArmLength();
	TestTrue(TEXT("l'ultima distanza tattica cade fra le due soglie"), LastTactical > 1900.f && LastTactical < 2400.f);

	Cam->ToggleStrategicView();
	TestFalse(TEXT("Tab esce dalla strategica anche se ci si e' entrati con la rotella"), Cam->IsStrategicView());
	TestEqual(TEXT("e si ferma alla soglia d'uscita, la distanza tattica piu' vicina"), Arm->TargetArmLength, 1900.f);

	DestroyCameraWorld(World);
	return true;
}

/**
 * 🔴 **#3646 — il secondo `Tab` torna alla distanza da cui si e' partiti anche se nessuno l'ha letta.**
 *
 * `FrameOwnTeam` apre la partita scrivendo `TargetArmLength` direttamente, senza passare da `UpdateStrategicState`:
 * qui lo stesso gesto e' la scrittura diretta sul braccio. Trovato in PIE (seduta `U73`): partenza a 450, ritorno a
 * 800, cioe' alla distanza di `RecenterView`. Verifica di mutazione: togliendo la registrazione in
 * `ToggleStrategicView` il ritorno va a 800 e l'ultima asserzione cade.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraTabReturnsToUnreadDistanceTest,
	"RefactorTactics.Camera.TabReturnsToADistanceWrittenOutsideTheZoomGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraTabReturnsToUnreadDistanceTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { DestroyCameraWorld(World); return false; }

	Cam->SetArmLengthRangeForTest(/*Default=*/ 800.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);
	Cam->SetStrategicThresholdsForTest(/*Enter=*/ 2400.f, /*Exit=*/ 1900.f);
	Cam->RecenterView(); // l'ultima lettura tattica e' 800

	// Lo scrittore che non passa dalla porta dello zoom, come `FrameOwnTeam` all'avvio.
	Arm->TargetArmLength = 450.f;

	Cam->ToggleStrategicView();
	TestTrue(TEXT("il primo Tab apre la strategica"), Cam->IsStrategicView());
	Cam->ToggleStrategicView();
	TestFalse(TEXT("il secondo Tab torna tattici"), Cam->IsStrategicView());
	TestEqual(TEXT("e alla distanza da cui si era partiti, non a quella dell'ultima lettura"), Arm->TargetArmLength, 450.f);

	DestroyCameraWorld(World);
	return true;
}

// --- #3630 · l'orbita che lo zoom non cancella ------------------------------------------------------------------

/**
 * 🔴 **L'inclinazione data con l'orbita sopravvive allo zoom** (#3630).
 *
 * Fino al 2026-10-10 `AddOrbit` scriveva `CameraPitch` direttamente, e il primo `AddZoom` la cancellava:
 * `RecomputePitchFromZoom` riscrive il pitch come derivato + offset. Verifica di mutazione: rimettere la scrittura
 * diretta deve far cadere la seconda asserzione. La rampa strategica della stessa issue si e' ritirata con D-498
 * (#3662); questa correzione resta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraOrbitPitchSurvivesZoomTest,
	"RefactorTactics.Camera.OrbitPitchSurvivesTheNextZoom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraOrbitPitchSurvivesZoomTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	Cam->SetArmLengthRangeForTest(/*Default=*/ 800.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);
	Cam->SetPitchForTest(/*Default=*/ -40.f, /*Current=*/ -40.f);
	Cam->SetPitchZoomDeltaForTest(0.f);
	Cam->SetSensitivitiesForTest(/*Yaw=*/ 1.f, /*Pitch=*/ 1.f);
	Cam->SetZoomStepForTest(150.f);
	Cam->RecenterView();

	const float Prima = Cam->GetCameraPitch();
	Cam->AddOrbit(/*DeltaYaw=*/ 0.f, /*DeltaPitch=*/ -10.f);
	const float DopoOrbita = Cam->GetCameraPitch();
	TestNotEqual(TEXT("l'orbita ha inclinato la vista"), DopoOrbita, Prima);

	Cam->AddZoom(+1.f);
	TestEqual(TEXT("e il primo zoom non la cancella"), Cam->GetCameraPitch(), DopoOrbita);

	DestroyCameraWorld(World);
	return true;
}

// --- D-498 (#3662) · la vista strategica isometrica dimetrica: ortografica, pitch fisso, orbita verticale inerte ---

namespace
{
	/**
	 * Le tarature che questi test fissano, perche' ognuna e' un ingresso di cio' che misurano: soglie 2400/1900, braccio
	 * 800 in [100, 4000], passo di zoom 150, pitch tattico -40 senza curva di zoom, sensibilita' 1. La vista parte
	 * tattica, a yaw 0. `StrategicPitch` resta il default della classe: e' il valore di D-498, e i test lo pinnano.
	 */
	void RTPreparaVistaD498(ARTCameraPawn* Cam)
	{
		Cam->SetArmLengthRangeForTest(/*Default=*/ 800.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);
		Cam->SetStrategicThresholdsForTest(/*Enter=*/ 2400.f, /*Exit=*/ 1900.f);
		Cam->SetZoomStepForTest(150.f);
		Cam->SetPitchForTest(/*Default=*/ -40.f, /*Current=*/ -40.f);
		Cam->SetPitchZoomDeltaForTest(0.f);
		Cam->SetSensitivitiesForTest(/*Yaw=*/ 1.f, /*Pitch=*/ 1.f);
		Cam->RecenterView();
	}

	/** Porta il braccio a una distanza passando dalla porta dello zoom: e' cio' che il giocatore ottiene. */
	void RTBraccioA(ARTCameraPawn* Cam, float Arm)
	{
		const float Range = Cam->GetMaxArmLength() - Cam->GetMinArmLength();
		Cam->SetZoomAlpha(Range > 0.f ? (Arm - Cam->GetMinArmLength()) / Range : 0.f);
	}
}

/**
 * Pitch e proiezione seguono lo **stato**, con la sua isteresi, nei due versi (D-498 (2)).
 *
 * 🔑 **La distanza 2100 compare due volte, ed e' il caso che porta il peso.** Fra le soglie lo stato dipende da dove
 * si viene: dal basso e' tattico, dall'alto strategico. Una regola letta dalla DISTANZA — come la rampa di #3630 —
 * darebbe lo stesso pitch le due volte; una letta dallo STATO li separa. E l'offset manuale, che la strategica non
 * deve toccare, si ritrova intatto alla fine.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraStrategicFollowsTheStateTest,
	"RefactorTactics.Camera.StrategicPitchAndProjectionFollowTheStateInBothHysteresisDirections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraStrategicFollowsTheStateTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	RTPreparaVistaD498(Cam);

	// Un'inclinazione manuale in tattica: deve attraversare tutto il giro senza perdersi.
	Cam->AddPitch(+5.f);
	const float Tattico = Cam->GetCameraPitch();
	TestEqual(TEXT("premessa: l'input ha inclinato la tattica"), Tattico, -35.f, 1e-3f);

	// Fra le soglie, ENTRANDOCI DAL BASSO: la vista resta tattica, e con lei pitch e proiezione.
	RTBraccioA(Cam, 2100.f);
	TestFalse(TEXT("fra le soglie, dal basso, la vista e' tattica"), Cam->IsStrategicView());
	TestFalse(TEXT("e prospettica"), Cam->IsOrthographicNow());
	TestEqual(TEXT("con il pitch tattico"), Cam->GetCameraPitch(), Tattico, 1e-3f);

	// Oltre la soglia d'ingresso: nella STESSA chiamata che cambia lo stato cambiano pitch e proiezione (D-495 (4)).
	RTBraccioA(Cam, 3000.f);
	TestTrue(TEXT("oltre la soglia la vista e' strategica"), Cam->IsStrategicView());
	TestTrue(TEXT("ortografica"), Cam->IsOrthographicNow());
	TestEqual(TEXT("e a -30 gradi, il dimetrico 2:1 di D-498"), Cam->GetCameraPitch(), -30.f, 1e-3f);

	// Fra le soglie, ENTRANDOCI DALL'ALTO: l'isteresi tiene la strategica, e con lei pitch e proiezione.
	RTBraccioA(Cam, 2100.f);
	TestTrue(TEXT("fra le soglie, dall'alto, la vista resta strategica"), Cam->IsStrategicView());
	TestTrue(TEXT("e resta ortografica"), Cam->IsOrthographicNow());
	TestEqual(TEXT("a -30 gradi: il pitch segue lo stato, non la distanza"), Cam->GetCameraPitch(), -30.f, 1e-3f);

	// Sotto la soglia d'uscita: tattica, prospettica, con l'inclinazione di prima.
	RTBraccioA(Cam, 1500.f);
	TestFalse(TEXT("sotto la soglia d'uscita si torna tattici"), Cam->IsStrategicView());
	TestFalse(TEXT("e prospettici"), Cam->IsOrthographicNow());
	TestEqual(TEXT("con l'inclinazione di prima"), Cam->GetCameraPitch(), Tattico, 1e-3f);
	TestEqual(TEXT("perche' l'offset manuale non si e' mosso"), Cam->GetManualPitchOffset(), 5.f, 1e-3f);

	// La rotella: lo scatto che attraversa la soglia porta gia' il pitch strategico, non quello del fotogramma prima.
	RTBraccioA(Cam, 2300.f);
	Cam->AddZoom(+1.f);
	TestTrue(TEXT("uno scatto di rotella oltre la soglia entra in strategica"), Cam->IsStrategicView());
	TestEqual(TEXT("e nello stesso scatto il pitch e' -30"), Cam->GetCameraPitch(), -30.f, 1e-3f);

	DestroyCameraWorld(World);
	return true;
}

/**
 * In strategica l'orbita non agisce, ne' la verticale (D-498 (4)) ne' l'orizzontale (D-499): in iso si ruota solo a
 * scatti di 60. Tornando tattici l'inclinazione e' quella di prima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraStrategicFreeOrbitInertTest,
	"RefactorTactics.Camera.FreeOrbitIsInertInStrategicAndTheTacticalTiltReturns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraStrategicFreeOrbitInertTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	RTPreparaVistaD498(Cam);

	Cam->AddOrbit(/*DeltaYaw=*/ 0.f, /*DeltaPitch=*/ -10.f);
	const float Tattico = Cam->GetCameraPitch();
	const float Offset = Cam->GetManualPitchOffset();
	TestNotEqual(TEXT("premessa: l'orbita ha inclinato la tattica"), Tattico, -40.f);

	Cam->ToggleStrategicView();
	if (!TestTrue(TEXT("Tab apre la strategica"), Cam->IsStrategicView())) { DestroyCameraWorld(World); return false; }
	TestEqual(TEXT("a -30 gradi"), Cam->GetCameraPitch(), -30.f, 1e-3f);

	Cam->AddOrbit(/*DeltaYaw=*/ 0.f, /*DeltaPitch=*/ +15.f);
	Cam->AddPitch(+15.f);
	TestEqual(TEXT("in strategica ne' l'orbita ne' l'inclinazione da tastiera muovono il pitch"),
		Cam->GetCameraPitch(), -30.f, 1e-3f);
	TestEqual(TEXT("e non toccano l'offset della tattica"), Cam->GetManualPitchOffset(), Offset, 1e-3f);

	const float YawPrima = Cam->GetCameraYaw();
	Cam->AddOrbit(/*DeltaYaw=*/ 20.f, /*DeltaPitch=*/ 0.f);
	Cam->AddYawContinuous(20.f);
	TestEqual(TEXT("e nemmeno l'orbita orizzontale libera muove l'imbardata (D-499)"), Cam->GetCameraYaw(), YawPrima, 1e-3f);

	Cam->ToggleStrategicView();
	TestFalse(TEXT("il secondo Tab torna tattici"), Cam->IsStrategicView());
	TestEqual(TEXT("con l'inclinazione di prima"), Cam->GetCameraPitch(), Tattico, 1e-3f);

	// Controllo positivo: in tattica la stessa orbita orizzontale agisce.
	const float YawTattico = Cam->GetCameraYaw();
	Cam->AddOrbit(/*DeltaYaw=*/ 20.f, /*DeltaPitch=*/ 0.f);
	TestEqual(TEXT("in tattica l'orbita orizzontale agisce"), Cam->GetCameraYaw(),
		static_cast<float>(FRotator::ClampAxis(YawTattico + 20.f)), 1e-3f);

	DestroyCameraWorld(World);
	return true;
}

/**
 * `D-499`: in iso `Q`/`E` ruotano di 60; entrando l'imbardata si allinea al multiplo di 60 piu' vicino, e uscendo torna
 * quella tattica. In tattica lo scatto resta 45 (D-142).
 *
 * 🔑 Due ingressi: da 10, che si allinea per difetto (0), e da 45, che si allinea per eccesso (60). Un allineamento che
 * troncasse invece di arrotondare darebbe 0 anche al secondo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraStrategicYawStepTest,
	"RefactorTactics.Camera.YawSnapsTo60InStrategicAndTheTacticalYawReturns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraStrategicYawStepTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	RTPreparaVistaD498(Cam);

	Cam->AddYawContinuous(10.f);
	TestEqual(TEXT("premessa: imbardata a 10"), Cam->GetCameraYaw(), 10.f, 1e-3f);

	Cam->ToggleStrategicView();
	if (!TestTrue(TEXT("Tab apre la strategica"), Cam->IsStrategicView())) { DestroyCameraWorld(World); return false; }
	TestEqual(TEXT("entrando da 10 l'imbardata si allinea a 0"), Cam->GetCameraYaw(), 0.f, 1e-3f);

	Cam->AddYaw(+1.f);
	TestEqual(TEXT("in strategica E ruota di 60 gradi"), Cam->GetCameraYaw(), 60.f, 1e-3f);
	Cam->AddYaw(-1.f);
	Cam->AddYaw(-1.f);
	TestEqual(TEXT("e Q torna indietro di 60, anche oltre lo zero"), Cam->GetCameraYaw(), 300.f, 1e-3f);

	Cam->ToggleStrategicView();
	TestFalse(TEXT("il secondo Tab torna tattici"), Cam->IsStrategicView());
	TestEqual(TEXT("uscendo torna l'imbardata tattica"), Cam->GetCameraYaw(), 10.f, 1e-3f);
	Cam->AddYaw(+1.f);
	TestEqual(TEXT("e in tattica lo scatto resta 45 (D-142)"), Cam->GetCameraYaw(), 55.f, 1e-3f);

	// Secondo ingresso, da 45: si allinea per eccesso.
	Cam->AddYaw(-1.f);
	Cam->AddYawContinuous(35.f);
	TestEqual(TEXT("premessa: imbardata a 45"), Cam->GetCameraYaw(), 45.f, 1e-3f);
	Cam->ToggleStrategicView();
	TestEqual(TEXT("entrando da 45 l'imbardata si allinea a 60"), Cam->GetCameraYaw(), 60.f, 1e-3f);
	Cam->ToggleStrategicView();
	TestEqual(TEXT("e uscendo torna 45"), Cam->GetCameraYaw(), 45.f, 1e-3f);

	DestroyCameraWorld(World);
	return true;
}

/** `D-499`: l'allineamento dell'imbardata, come funzione pura. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraSnapYawTest,
	"RefactorTactics.Camera.SnapYawToStepRoundsToTheNearestMultiple",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraSnapYawTest::RunTest(const FString&)
{
	struct FCaso { float Da; float Atteso; };
	for (const FCaso& C : { FCaso{ 0.f, 0.f }, FCaso{ 10.f, 0.f }, FCaso{ 29.f, 0.f }, FCaso{ 30.f, 60.f }, FCaso{ 45.f, 60.f },
	                        FCaso{ 89.f, 60.f }, FCaso{ 91.f, 120.f }, FCaso{ 330.f, 0.f }, FCaso{ 350.f, 0.f }, FCaso{ -20.f, 0.f },
	                        FCaso{ -40.f, 300.f } })
	{
		TestEqual(*FString::Printf(TEXT("%.0f si allinea a %.0f"), C.Da, C.Atteso),
			ARTCameraPawn::SnapYawToStep(C.Da, 60.f), C.Atteso, 1e-3f);
	}
	return true;
}

/**
 * `Home` dalla strategica torna a imbardata 0, anche se l'imbardata tattica salvata all'ingresso era un'altra (D-499).
 *
 * 🔑 Si ruota in TATTICA prima di entrare, cosi' il valore salvato e' 45 e non 0: senza l'azzeramento in
 * `RecenterView`, l'uscita dalla strategica che `Home` provoca ripristinerebbe 45. Verifica di mutazione: tolta la riga
 * `TacticalYawBeforeStrategic = 0.f` da `RecenterView`, l'ultima asserzione cade.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraHomeFromStrategicYawTest,
	"RefactorTactics.Camera.HomeFromStrategicResetsTheSavedTacticalYaw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraHomeFromStrategicYawTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	RTPreparaVistaD498(Cam);

	Cam->AddYaw(+1.f);
	TestEqual(TEXT("premessa: in tattica l'imbardata e' 45"), Cam->GetCameraYaw(), 45.f, 1e-3f);
	Cam->ToggleStrategicView();
	if (!TestTrue(TEXT("Tab apre la strategica"), Cam->IsStrategicView())) { DestroyCameraWorld(World); return false; }
	TestEqual(TEXT("premessa: entrando si allinea a 60"), Cam->GetCameraYaw(), 60.f, 1e-3f);

	Cam->RecenterView();
	TestFalse(TEXT("Home riporta alla distanza di default, cioe' in tattica"), Cam->IsStrategicView());
	TestEqual(TEXT("e l'imbardata e' 0, non la 45 salvata all'ingresso"), Cam->GetCameraYaw(), 0.f, 1e-3f);

	DestroyCameraWorld(World);
	return true;
}

/**
 * Il secondo `Tab` restituisce la tattica INTERA: distanza, pitch, proiezione e imbardata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraSecondTabRestoresTacticalTest,
	"RefactorTactics.Camera.SecondTabRestoresTheWholeTacticalView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraSecondTabRestoresTacticalTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { DestroyCameraWorld(World); return false; }
	RTPreparaVistaD498(Cam);

	// Una tattica che non e' quella di default su nessuno dei quattro assi.
	Cam->AddZoom(+1.f);
	Cam->AddPitch(+5.f);
	Cam->AddYawContinuous(10.f);
	const float Braccio = Arm->TargetArmLength;
	const float Pitch = Cam->GetCameraPitch();
	const float Yaw = Cam->GetCameraYaw();
	TestFalse(TEXT("premessa: si parte prospettici"), Cam->IsOrthographicNow());

	Cam->ToggleStrategicView();
	TestTrue(TEXT("in strategica la camera e' ortografica"), Cam->IsOrthographicNow());
	TestEqual(TEXT("e a -30 gradi"), Cam->GetCameraPitch(), -30.f, 1e-3f);

	Cam->ToggleStrategicView();
	TestFalse(TEXT("il secondo Tab torna tattici"), Cam->IsStrategicView());
	TestFalse(TEXT("prospettici"), Cam->IsOrthographicNow());
	TestEqual(TEXT("alla stessa distanza"), Arm->TargetArmLength, Braccio);
	TestEqual(TEXT("con lo stesso pitch"), Cam->GetCameraPitch(), Pitch, 1e-3f);
	TestEqual(TEXT("e la stessa imbardata"), Cam->GetCameraYaw(), Yaw, 1e-3f);

	DestroyCameraWorld(World);
	return true;
}

/**
 * 🔴 **Al cambio di proiezione la scala sul pivot non salta — misurato sulla MATRICE DI PROIEZIONE** (#3662).
 *
 * Il test che c'era prima (`StrategicOrthographicKeepsTheScaleAtThePivot`, #3631) confrontava `OrthoWidth` con la
 * formula che la calcolava, ed era verde mentre a schermo l'ortografica inquadrava 1,78× la prospettica: con il
 * vincolo `MaintainYFOV` `OrthoWidth` e' l'estensione VERTICALE. Qui la camera consegna la sua vista
 * (`GetCameraView`) nell'ultimo istante tattico e nel primo strategico, e UE costruisce la matrice come la
 * costruirebbe per il render, con il vincolo d'aspetto del giocatore locale.
 *
 * 🔑 **Il vincolo viene dal default di `ULocalPlayer`, non da una costante**: se il progetto lo cambiasse, o cambiasse
 * `AspectRatio` della camera, questo test cadrebbe invece di confermare una garanzia di UE. Tre quadri, perche' con
 * `MaintainYFOV` l'uguaglianza deve valere a ogni aspetto e non solo a 16:9. Verifica di mutazione: senza la
 * divisione per l'aspetto in `OrthoWidthForArm` il rapporto torna 1,78 e le asserzioni cadono.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraStrategicOrthoProjectionTest,
	"RefactorTactics.Camera.StrategicOrthographicMatchesThePerspectiveOnTheProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraStrategicOrthoProjectionTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	UCameraComponent* View = Cam->FindComponentByClass<UCameraComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm) || !TestNotNull(TEXT("camera component"), View))
	{
		DestroyCameraWorld(World);
		return false;
	}
	RTPreparaVistaD498(Cam);

	// L'ultimo istante tattico: un'unita' sotto la soglia d'ingresso.
	RTBraccioA(Cam, 2399.f);
	if (!TestFalse(TEXT("premessa: sotto la soglia si e' tattici"), Cam->IsStrategicView())) { DestroyCameraWorld(World); return false; }
	const float DistanzaTattica = Arm->TargetArmLength;
	FMinimalViewInfo Prospettica;
	View->GetCameraView(0.f, Prospettica);

	// Il primo istante strategico: `Tab` porta il braccio ESATTAMENTE alla soglia.
	Cam->ToggleStrategicView();
	if (!TestTrue(TEXT("premessa: Tab apre la strategica"), Cam->IsStrategicView())) { DestroyCameraWorld(World); return false; }
	const float DistanzaStrategica = Arm->TargetArmLength;
	FMinimalViewInfo Ortografica;
	View->GetCameraView(0.f, Ortografica);
	TestTrue(TEXT("premessa: la vista tattica e' prospettica"), Prospettica.ProjectionMode == ECameraProjectionMode::Perspective);
	TestTrue(TEXT("premessa: quella strategica ortografica"), Ortografica.ProjectionMode == ECameraProjectionMode::Orthographic);

	const TEnumAsByte<EAspectRatioAxisConstraint> Vincolo = GetDefault<ULocalPlayer>()->AspectRatioAxisConstraint;
	auto Matrice = [Vincolo](FMinimalViewInfo Info, const FIntRect& Quadro)
	{
		FSceneViewProjectionData Dati;
		Dati.SetViewRectangle(Quadro);
		FMinimalViewInfo::CalculateProjectionMatrixGivenViewRectangle(Info, Vincolo, Quadro, Dati);
		return Dati.ProjectionMatrix;
	};

	for (const FIntRect& Quadro : { FIntRect(0, 0, 1920, 1080), FIntRect(0, 0, 2560, 1080), FIntRect(0, 0, 1280, 800) })
	{
		const FMatrix P = Matrice(Prospettica, Quadro);
		const FMatrix O = Matrice(Ortografica, Quadro);
		// Mezze estensioni sul piano perpendicolare allo sguardo, alla distanza del pivot. In prospettiva un punto a
		// profondita' `d` va in `x * M[0][0] / d`, quindi il bordo del quadro sta a `d / M[0][0]`; in ortografica a
		// `1 / M[0][0]`. Lo stesso su Y con `M[1][1]`.
		const double AltezzaP = DistanzaTattica / P.M[1][1];
		const double AltezzaO = 1.0 / O.M[1][1];
		const double LarghezzaP = DistanzaTattica / P.M[0][0];
		const double LarghezzaO = 1.0 / O.M[0][0];
		// La tolleranza assorbe l'unita' di braccio fra i due istanti (1/2400) e niente di piu'. La larghezza non e' una
		// seconda prova: le due matrici la dividono per lo stesso `XAxisMultiplier`, quindi segue l'altezza. Resta come
		// controllo di coerenza del calcolo.
		const double Tolleranza = 0.002;
		const FString Q = FString::Printf(TEXT("%dx%d"), Quadro.Width(), Quadro.Height());
		TestEqual(*FString::Printf(TEXT("%s: l'ortografica alta quanto la prospettica sul pivot"), *Q),
			AltezzaO, AltezzaP * DistanzaStrategica / DistanzaTattica, AltezzaP * Tolleranza);
		TestEqual(*FString::Printf(TEXT("%s: e larga quanto lei"), *Q),
			LarghezzaO, LarghezzaP * DistanzaStrategica / DistanzaTattica, LarghezzaP * Tolleranza);
	}

	DestroyCameraWorld(World);
	return true;
}

namespace
{
	/**
	 * La matrice vista×proiezione che UE costruirebbe per il render dalla camera ferma alla distanza corrente: la stessa
	 * strada di `ULocalPlayer::GetProjectionData`, compresi i piani ortografici automatici e la loro correzione.
	 *
	 * ⚠️ La camera si mette dove il braccio la porterebbe, invece di leggerne la posizione: il braccio ha il ritardo di
	 * camera, e nel mondo di prova non ticca. Tutto il resto viene da `GetCameraView`.
	 */
	FMatrix RTVistaProiezione(ARTCameraPawn* Cam, UCameraComponent* View, float Arm, const FIntRect& Quadro)
	{
		FMinimalViewInfo Info;
		View->GetCameraView(0.f, Info);
		Info.Rotation = FRotator(Cam->GetCameraPitch(), Cam->GetCameraYaw(), 0.f);
		const FVector Bersaglio = Cam->GetActorLocation();
		Info.Location = Bersaglio - Info.Rotation.Vector() * Arm;
		Info.SetCameraToViewTarget(Bersaglio);

		FSceneViewProjectionData Dati;
		Dati.ViewOrigin = Info.Location;
		Dati.ViewRotationMatrix = FInverseRotationMatrix(Info.Rotation) * FMatrix(
			FPlane(0, 0, 1, 0),
			FPlane(1, 0, 0, 0),
			FPlane(0, 1, 0, 0),
			FPlane(0, 0, 0, 1));
		Dati.SetViewRectangle(Quadro);
		FMinimalViewInfo::CalculateProjectionMatrixGivenViewRectangle(
			Info, GetDefault<ULocalPlayer>()->AspectRatioAxisConstraint, Quadro, Dati);
		return Dati.ComputeViewProjectionMatrix();
	}
}

/**
 * 🔴 **In ortografica i piani della mappa restano dentro i piani di clipping** (#3662, review di #3671).
 *
 * In ortografica il quadro e' alto `OrthoWidth` anche all'altezza della camera, e a -30 il suo bordo basso parte quasi
 * dal suolo: la geometria piu' alta del pivot, li', sta dietro la camera. Con i piani automatici di UE il near copriva
 * fino a circa `0,23 · Arm` di quota, cioe' 547 alla soglia d'ingresso, e il piano 1 con la separazione 3 sta a 750.
 *
 * Per ogni distanza strategica che conta — la soglia d'uscita raggiunta dall'alto, quella d'ingresso di `Tab`, il fondo
 * corsa — e per cinque quote, da due piani sotto il pivot a due sopra, i punti che cadono DENTRO il quadro devono
 * cadere anche fra i piani (Z in [0, 1], Z rovesciata). Il controllo positivo e' che ogni quota abbia punti nel quadro.
 * Verifica di mutazione: senza lo spostamento dei piani (`StrategicOrthoPlaneShiftArms`) il test cade.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraStrategicOrthoPlanesTest,
	"RefactorTactics.Camera.StrategicOrthographicPlanesKeepTheMapLayersInView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraStrategicOrthoPlanesTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	UCameraComponent* View = Cam->FindComponentByClass<UCameraComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm) || !TestNotNull(TEXT("camera component"), View))
	{
		DestroyCameraWorld(World);
		return false;
	}
	RTPreparaVistaD498(Cam);

	const FIntRect Quadro(0, 0, 1920, 1080);
	// Con `LayerHeight` 250 e la separazione 3 di #3632 i piani stanno a 750 l'uno dall'altro.
	const float Quote[] = { -1500.f, -750.f, 0.f, 750.f, 1500.f };

	struct FCaso { float Arm; const TCHAR* Come; };
	for (const FCaso& Caso : { FCaso{ 1950.f, TEXT("alla soglia d'uscita, dall'alto") },
	                           FCaso{ 2400.f, TEXT("alla soglia d'ingresso") },
	                           FCaso{ 4000.f, TEXT("a fondo corsa") } })
	{
		RTBraccioA(Cam, 3000.f); // si entra dall'alto: 1950 e' strategico solo per isteresi
		RTBraccioA(Cam, Caso.Arm);
		if (!TestTrue(*FString::Printf(TEXT("%s: la vista e' strategica"), Caso.Come), Cam->IsStrategicView()))
		{
			continue;
		}
		const float Distanza = Arm->TargetArmLength;
		const FMatrix M = RTVistaProiezione(Cam, View, Distanza, Quadro);
		const FVector Pivot = Cam->GetActorLocation();
		const double Raggio = 3.0 * Distanza; // oltre l'impronta del quadro a terra, su ogni quota
		for (const float Quota : Quote)
		{
			int32 Dentro = 0;
			int32 Tagliati = 0;
			for (int32 IX = -40; IX <= 40; ++IX)
			{
				for (int32 IY = -40; IY <= 40; ++IY)
				{
					const FVector P = Pivot + FVector(Raggio * IX / 40.0, Raggio * IY / 40.0, Quota);
					const FVector4 C = M.TransformFVector4(FVector4(P, 1.0));
					if (FMath::Abs(C.X) <= 1.0 && FMath::Abs(C.Y) <= 1.0)
					{
						++Dentro;
						if (C.Z < 0.0 || C.Z > 1.0) { ++Tagliati; }
					}
				}
			}
			TestTrue(*FString::Printf(TEXT("%s, quota %+.0f: ci sono punti nel quadro"), Caso.Come, Quota), Dentro > 100);
			TestEqual(*FString::Printf(TEXT("%s, quota %+.0f: nessuno e' tagliato dai piani"), Caso.Come, Quota), Tagliati, 0);
		}
	}

	DestroyCameraWorld(World);
	return true;
}

/**
 * 🔴 **Le porte del braccio rileggono lo stato PRIMA di clampare il pivot** (#3662, review di #3671).
 *
 * I limiti del pivot dipendono dal pitch (`ComputeEffectivePivotBounds`: l'estensione lungo lo sguardo e'
 * `VerticalHalf / sin(pitch)`), e a -30 sono piu' stretti che a -40. Se `AddZoom` clampasse il pivot col pitch
 * tattico e rileggesse lo stato solo dopo, attraversando la soglia il pivot resterebbe fuori dai limiti strategici:
 * ri-clamparlo lo sposterebbe. L'area si allarga col margine, perche' sulla mappa di prova i limiti a questa distanza
 * collasserebbero al centro con entrambi i pitch. Verifica di mutazione: rimettere `UpdateStrategicState` in fondo ad
 * `AddZoom` fa cadere l'ultima asserzione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraArmGateOrderTest,
	"RefactorTactics.Camera.ArmGatesReadTheStateBeforeClampingThePivot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraArmGateOrderTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	RTPreparaVistaD498(Cam);
	Cam->SetViewportMetricsForTest(16.f / 9.f, 90.f);
	Cam->SetBoundsMarginForTest(11.f);

	// Tattici appena sotto la soglia d'ingresso, col pivot al bordo lungo lo sguardo (+X a yaw 0).
	RTBraccioA(Cam, 2300.f);
	if (!TestFalse(TEXT("premessa: si parte tattici"), Cam->IsStrategicView())) { DestroyCameraWorld(World); return false; }
	Cam->SetCameraPivot(Cam->GetCameraPivot() + FVector(1.0e6, 0.0, 0.0));
	const double BordoTattico = Cam->GetCameraPivot().X;

	Cam->AddZoom(+1.f);
	if (!TestTrue(TEXT("uno scatto di rotella entra in strategica"), Cam->IsStrategicView())) { DestroyCameraWorld(World); return false; }
	const FVector Dopo = Cam->GetCameraPivot();
	TestTrue(TEXT("premessa: allontanandosi il pivot rientra"), Dopo.X < BordoTattico - 1.0);

	Cam->SetCameraPivot(Dopo);
	TestEqual(TEXT("e sta gia' nei limiti del pitch strategico: ri-clamparlo non lo sposta"),
		Cam->GetCameraPivot().X, Dopo.X, 0.5);

	DestroyCameraWorld(World);
	return true;
}

// --- #1778 · i limiti che conoscono il viewport ------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraEffectiveBoundsTest,
	"RefactorTactics.Camera.EffectivePivotBoundsShrinkWithZoomPitchAndAspect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraEffectiveBoundsTest::RunTest(const FString&)
{
	// Pura: nessun mondo, nessun attore, nessun viewport. E' la ragione per cui prende le metriche come
	// parametri invece di leggerle — in headless un viewport non esiste, ed e' un limite gia' dichiarato
	// da `ZoomTowards`.
	const FBox2D Area(FVector2D(-5000.f, -5000.f), FVector2D(5000.f, 5000.f));
	const float Fov = 90.f;
	const float Fraction = 0.35f;

	const FBox2D Near = ARTCameraPawn::ComputeEffectivePivotBounds(Area, /*Arm=*/ 500.f, -45.f, 0.f, Fov, 16.f/9.f, Fraction);
	const FBox2D Far  = ARTCameraPawn::ComputeEffectivePivotBounds(Area, /*Arm=*/ 2000.f, -45.f, 0.f, Fov, 16.f/9.f, Fraction);
	TestTrue(TEXT("piu' si e' lontani, meno il pivot puo' avvicinarsi al bordo"),
		Far.Max.X < Near.Max.X);

	// Pitch verso l'orizzonte: il quadro si allunga **lungo lo sguardo**, quindi il limite si stringe su
	// quell'asse. E' il caso che rompe il clamp fisso, ed e' la ragione per cui il pitch entra nella formula.
	//
	// 🔴 **Questa asserzione guardava l'asse SBAGLIATO, ed era verde per il difetto che avrebbe dovuto
	// prendere** (code review, 2026-08-30). A yaw 0 la direzione di sguardo e' **+X** —
	// `Camera.PanIsRelativeToTheView` la pinna: *«yaw 0: avanti e' +X»* — quindi il termine del pitch vive
	// su **X**, e verificarlo su `Max.Y` confermava lo scambio di assi invece di falsificarlo. Un test che
	// misura l'asse sbagliato non e' debole: e' d'accordo con il bug.
	const FBox2D Steep = ARTCameraPawn::ComputeEffectivePivotBounds(Area, 2000.f, -80.f, 0.f, Fov, 16.f/9.f, Fraction);
	const FBox2D Shallow = ARTCameraPawn::ComputeEffectivePivotBounds(Area, 2000.f, -20.f, 0.f, Fov, 16.f/9.f, Fraction);
	TestTrue(TEXT("a yaw 0 il pitch stringe l'asse di SGUARDO (X): radente piu' stretto che a picco"),
		Shallow.Max.X < Steep.Max.X);

	// E il gemello sull'altro asse: la profondita' non deve entrare in Y a yaw 0, quindi il pitch non lo
	// tocca. Senza questa riga lo scambio potrebbe tornare e meta' delle assertion resterebbero verdi.
	TestTrue(TEXT("a yaw 0 il pitch NON tocca l'asse laterale (Y)"),
		FMath::IsNearlyEqual(Shallow.Max.Y, Steep.Max.Y, 0.5f));

	// 🔑 **A 16:9 i limiti restano quelli di prima di #3661**, e lo dicono i numeri, non una disuguaglianza:
	// semiasse laterale `2000 · tan 45° = 2000`, inset `2000 · 0,65 = 1300`, quindi `Max.Y = 3700`; semiasse
	// verticale `2000 / 1,778 = 1125`, profondita' `1125 / sin 45° = 1591`, inset `1034`, quindi `Max.X = 3966`.
	TestEqual(TEXT("a 16:9 il limite laterale e' quello di prima di #3661"), static_cast<float>(Far.Max.Y), 3700.f, 1.f);
	TestEqual(TEXT("a 16:9 il limite lungo lo sguardo e' quello di prima di #3661"), static_cast<float>(Far.Max.X), 3966.f, 1.f);

	// ⌫ **Qui c'era un DIFETTO PINNATO** (`Wide.Max.Y == Far.Max.Y`): il modello teneva fisso il FOV
	// orizzontale e il limite laterale non dipendeva dall'aspetto. Il progetto pero' usa `MaintainYFOV`, default
	// di UE, e `FMinimalViewInfo::CalculateProjectionMatrixGivenViewRectangle` tiene fisso il semiangolo
	// VERTICALE `atan(tan(FOV/2) / AspectRatio)`. La correzione e' arrivata con #3661, quando la scena ha smesso
	// di essere 16:9: e questa riga, com'era annunciato, e' diventata rossa.
	//
	// ∴ Con l'aspetto che cresce la profondita' inquadrata non cambia (`Max.X` fermo) e il quadro si allarga di
	// lato (`Max.Y` si stringe). Con la scena di #3661, piu' stretta di 16:9, succede il contrario sul lato.
	const FBox2D Wide = ARTCameraPawn::ComputeEffectivePivotBounds(Area, 2000.f, -45.f, 0.f, Fov, 32.f/9.f, Fraction);
	TestEqual(TEXT("MaintainYFOV: a 32:9 la profondita' inquadrata (X a yaw 0) e' quella di 16:9"),
		static_cast<float>(Wide.Max.X), static_cast<float>(Far.Max.X), 0.5f);
	TestTrue(TEXT("MaintainYFOV: a 32:9 il quadro si allarga di lato, e il limite laterale (Y) si stringe"),
		Wide.Max.Y < Far.Max.Y - 100.f);

	const float AspettoScena = 1167.36f / 691.2f; // la scena di #3661 a 1920x1080, circa 1,69
	const FBox2D Scena = ARTCameraPawn::ComputeEffectivePivotBounds(Area, 2000.f, -45.f, 0.f, Fov, AspettoScena, Fraction);
	TestEqual(TEXT("nella scena di #3661 la profondita' inquadrata e' quella di 16:9"), static_cast<float>(Scena.Max.X), static_cast<float>(Far.Max.X), 0.5f);
	TestTrue(TEXT("e il limite laterale si allarga, perche' il quadro e' piu' stretto"), Scena.Max.Y > Far.Max.Y + 10.f);

	// Yaw 90°: il quadro e' ruotato, e gli assi dell'ingombro si scambiano. Senza questo passaggio i
	// limiti sarebbero corretti solo con la vista dritta — cioe' proprio dove la rotazione non serve.
	const FBox2D Turned = ARTCameraPawn::ComputeEffectivePivotBounds(Area, 2000.f, -45.f, 90.f, Fov, 16.f/9.f, Fraction);
	// ⚠️ Regge in entrambe le convenzioni — con gli assi scambiati sarebbe passata comunque — quindi da
	// sola **non** avrebbe preso il difetto: e' il gemello del test sul pitch a distinguere le due.
	TestTrue(TEXT("ruotando di 90 gradi gli assi del limite si scambiano"),
		FMath::IsNearlyEqual(Turned.Max.X, Far.Max.Y, 1.f) && FMath::IsNearlyEqual(Turned.Max.Y, Far.Max.X, 1.f));

	// Aspect ratio ignoto (headless): l'area passa intatta. Caso degenere **dichiarato**.
	const FBox2D Unknown = ARTCameraPawn::ComputeEffectivePivotBounds(Area, 2000.f, -45.f, 0.f, Fov, 0.f, Fraction);
	TestEqual(TEXT("senza aspect ratio i limiti restano quelli per sole celle"), Unknown.Max, Area.Max);

	// Area piu' piccola dell'inquadratura: il pivot si inchioda al CENTRO invece di produrre un intervallo
	// rovesciato, che un `Clamp` bloccherebbe a un angolo senza dirlo.
	const FBox2D Tiny(FVector2D(-100.f, -100.f), FVector2D(100.f, 100.f));
	const FBox2D Pinned = ARTCameraPawn::ComputeEffectivePivotBounds(Tiny, 4000.f, -45.f, 0.f, Fov, 16.f/9.f, Fraction);
	TestEqual(TEXT("su un'area minuscola il pivot si inchioda al centro"), Pinned.Min, Pinned.Max);
	TestEqual(TEXT("e il centro e' quello dell'area"), Pinned.Min, Tiny.GetCenter());

	return true;
}

// --- code review 2026-08-30 · i difetti riparati prendono un test ------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraZoomOutReclampsThePivotTest,
	"RefactorTactics.Camera.ZoomingOutPullsThePivotBackInsideTheNewBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraZoomOutReclampsThePivotTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	// Serve un viewport dichiarato, altrimenti i limiti di `D-251` non si applicano e il test misurerebbe
	// il caso degenere invece della regola.
	Cam->SetViewportMetricsForTest(16.f/9.f, 90.f);
	Cam->SetArmLengthRangeForTest(400.f, 100.f, 4000.f);
	Cam->SetBoundsMarginForTest(1.f);
	Cam->RecenterView();

	// Si porta il pivot al limite corrente scorrendo a lungo in una direzione.
	for (int32 i = 0; i < 60; ++i) { Cam->AddPlanarMovement(FVector2D(1.f, 0.f)); }
	const FVector AtEdge = Cam->GetCameraPivot();

	// Allontanandosi, l'area legale si stringe: il pivot **deve** rientrare da solo.
	for (int32 i = 0; i < 20; ++i) { Cam->AddZoom(+1.f); }

	TestTrue(TEXT("allontanandosi il pivot rientra nei limiti nuovi"),
		Cam->GetCameraPivot().Y < AtEdge.Y - 1.0);

	DestroyCameraWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraRecenterClampsWithTheResetViewTest,
	"RefactorTactics.Camera.RecenterClampsWithTheViewItIsAboutToHaveNotTheOldOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraRecenterClampsWithTheResetViewTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	ARTHexMapActor* Map = SpawnCameraTestMap(World);
	if (!TestNotNull(TEXT("mappa"), Map)) { DestroyCameraWorld(World); return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	Cam->SetViewportMetricsForTest(16.f/9.f, 90.f);
	Cam->SetArmLengthRangeForTest(/*Default=*/ 800.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);

	// Stato di partenza «peggiore»: zoom al massimo e vista radente, cioe' l'inquadratura piu' grande —
	// quella in cui il clamp con i valori vecchi divergeva di piu' da quello con i valori nuovi.
	for (int32 i = 0; i < 40; ++i) { Cam->AddZoom(+1.f); }
	Cam->SetPitchForTest(/*Default=*/ -40.f, /*Current=*/ -5.f);
	Cam->AddYaw(+1.f);

	Cam->RecenterView();

	// `Home` promette **una** cosa: l'inquadratura di partenza. Se il clamp avesse usato i valori vecchi,
	// il pivot sarebbe rimasto dove l'area piu' grande lo aveva inchiodato.
	FVector Origin; float HexSize; float LayerHeight;
	const URTHexMapAsset* Asset = Map->GetHexContext(Origin, HexSize, LayerHeight);
	if (!TestNotNull(TEXT("asset"), Asset)) { DestroyCameraWorld(World); return false; }
	const FVector Centre = URTHexLibrary::AxialToWorld(Asset->GetCenterCell(), Origin, HexSize, LayerHeight);

	TestTrue(TEXT("Home riporta il pivot sul centro della mappa"),
		FVector::Dist2D(Cam->GetCameraPivot(), Centre) < 1.0);
	TestEqual(TEXT("e l'orientamento e' quello di partenza"), Cam->GetCameraYaw(), 0.f);

	DestroyCameraWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------
// #1834 - `ZoomAlpha`: la sorgente unica del profilo di zoom.
// ---------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraZoomAlphaRangeTest,
	"RefactorTactics.Camera.ZoomAlphaSpansTheRealLimitsAndIsMonotonic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraZoomAlphaRangeTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { DestroyCameraWorld(World); return false; }

	// Intervallo ASIMMETRICO rispetto al default: e' cio' che rende il test discriminante. Con
	// `Default` a meta' corsa, un'implementazione che normalizzasse su `DefaultArmLength` invece che sui
	// limiti darebbe gli stessi numeri a meta' e passerebbe.
	Cam->SetArmLengthRangeForTest(/*Default=*/ 800.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);

	Arm->TargetArmLength = 100.f;
	TestEqual(TEXT("al braccio minimo alpha vale 0"), Cam->GetZoomAlpha(), 0.f);

	Arm->TargetArmLength = 4000.f;
	TestEqual(TEXT("al braccio massimo alpha vale 1"), Cam->GetZoomAlpha(), 1.f);

	// A meta' dell'INTERVALLO, non a `DefaultArmLength`: qui le due normalizzazioni divergono.
	Arm->TargetArmLength = 2050.f;
	TestEqual(TEXT("a meta' corsa alpha vale 0.5"), Cam->GetZoomAlpha(), 0.5f, 1e-3f);

	// Al default, alpha NON e' 0.5: se lo fosse, la sorgente starebbe normalizzando sul default.
	Arm->TargetArmLength = 800.f;
	TestTrue(TEXT("al default alpha non e' meta' corsa"), Cam->GetZoomAlpha() < 0.3f);

	// Monotonia: alpha cresce con la distanza, senza eccezioni lungo la corsa.
	float Previous = -1.f;
	for (int32 Step = 0; Step <= 10; ++Step)
	{
		Arm->TargetArmLength = FMath::Lerp(100.f, 4000.f, Step / 10.f);
		const float Alpha = Cam->GetZoomAlpha();
		TestTrue(TEXT("alpha cresce con la distanza"), Alpha > Previous);
		Previous = Alpha;
	}

	// Fuori dominio: il clamp e' della sorgente, non del chiamante.
	Arm->TargetArmLength = -500.f;
	TestEqual(TEXT("sotto il minimo resta 0"), Cam->GetZoomAlpha(), 0.f);
	Arm->TargetArmLength = 99999.f;
	TestEqual(TEXT("oltre il massimo resta 1"), Cam->GetZoomAlpha(), 1.f);

	DestroyCameraWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraZoomAlphaRoundTripTest,
	"RefactorTactics.Camera.ZoomAlphaWriteAndReadCannotDiverge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraZoomAlphaRoundTripTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { DestroyCameraWorld(World); return false; }

	Cam->SetArmLengthRangeForTest(/*Default=*/ 800.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);

	const float Probes[] = { 0.f, 0.25f, 0.5f, 0.75f, 1.f };
	for (const float Wanted : Probes)
	{
		Cam->SetZoomAlpha(Wanted);
		TestEqual(TEXT("cio' che si scrive si rilegge"), Cam->GetZoomAlpha(), Wanted, 1e-3f);

		// E il braccio e' davvero la a cui l'alpha corrisponde: se le due porte divergessero, questa
		// riga sarebbe l'unica ad accorgersene.
		TestEqual(TEXT("il braccio corrisponde all'alpha"), Arm->TargetArmLength,
			FMath::Lerp(100.f, 4000.f, Wanted), 1e-2f);
	}

	// Fuori dominio: clampa invece di estrapolare.
	Cam->SetZoomAlpha(-1.f);
	TestEqual(TEXT("alpha negativo clampa a 0"), Cam->GetZoomAlpha(), 0.f);
	Cam->SetZoomAlpha(2.f);
	TestEqual(TEXT("alpha oltre 1 clampa a 1"), Cam->GetZoomAlpha(), 1.f);

	// La porta di scrittura riallinea anche lo stato strategico: senza, `SetZoomAlpha(1)` porterebbe la
	// camera a vista larga lasciando lo stato indietro di un comando.
	Cam->SetStrategicThresholdsForTest(/*Enter=*/ 2400.f, /*Exit=*/ 1900.f);
	Cam->SetZoomAlpha(1.f);
	TestTrue(TEXT("a fondo corsa lo stato e' strategico"), Cam->IsStrategicView());
	Cam->SetZoomAlpha(0.f);
	TestFalse(TEXT("a zoom minimo lo stato esce"), Cam->IsStrategicView());

	DestroyCameraWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraPanScaleUnchangedTest,
	"RefactorTactics.Camera.PanScaleGoesThroughTheSourceWithoutChangingFeel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraPanScaleUnchangedTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm)) { DestroyCameraWorld(World); return false; }

	Cam->SetArmLengthRangeForTest(/*Default=*/ 800.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);
	Cam->SetBoundsMarginForTest(100000.f); // i limiti non sono il soggetto: qui si misura la scala

	// \U0001f534 Il criterio che rende questa issue un refactoring e non un cambio di feel: alla distanza
	// di default una unita' di input deve coprire ESATTAMENTE il terreno di prima. Il rapporto storico
	// era `TargetArmLength / DefaultArmLength`, cioe' 1 al default.
	Arm->TargetArmLength = 800.f;
	Cam->SetCameraPivot(FVector::ZeroVector);
	// `Axis.Y` e' «avanti sullo schermo»: con yaw a zero coincide con `ForwardVector`, quindi con
	// l'asse X del mondo. `Axis.X` sarebbe «a destra», cioe' Y — leggere l'asse sbagliato darebbe zero,
	// e il test fallirebbe per la propria convenzione invece che per un difetto.
	Cam->AddPlanarMovement(FVector2D(0.f, 1.f));
	const double AtDefault = FMath::Abs(Cam->GetCameraPivot().X);
	TestTrue(TEXT("al default lo scorrimento e' non nullo"), AtDefault > KINDA_SMALL_NUMBER);

	// Al doppio della distanza, il doppio del terreno: la proporzionalita' e' la proprieta' che la
	// sorgente deve conservare.
	Arm->TargetArmLength = 1600.f;
	Cam->SetCameraPivot(FVector::ZeroVector);
	Cam->AddPlanarMovement(FVector2D(0.f, 1.f));
	const double AtDouble = FMath::Abs(Cam->GetCameraPivot().X);
	TestEqual(TEXT("al doppio della distanza si copre il doppio"), AtDouble, AtDefault * 2.0, AtDefault * 0.01);

	DestroyCameraWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraPitchOffsetSurvivesZoomTest,
	"RefactorTactics.Camera.ZoomDrivenPitchDoesNotEraseTheManualOrbit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraPitchOffsetSurvivesZoomTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	Cam->SetArmLengthRangeForTest(/*Default=*/ 800.f, /*Min=*/ 100.f, /*Max=*/ 4000.f);
	Cam->SetSensitivitiesForTest(/*Yaw=*/ 0.5f, /*Pitch=*/ 1.f);
	Cam->SetPitchForTest(/*Default=*/ -40.f, /*Current=*/ -40.f);

	// \U0001f534 La curva si TARA nel test, e non e' un dettaglio: il default e' `0`, quindi senza questa
	// riga la derivazione sarebbe una costante e ogni asserzione qui sotto passerebbe anche su
	// un'implementazione che ignora del tutto lo zoom.
	Cam->SetPitchZoomDeltaForTest(-20.f);
	// D-498: oltre la soglia d'ingresso il pitch e' fisso. Il soggetto qui e' la derivazione TATTICA, quindi la
	// strategica si porta oltre il fondo corsa invece di lasciarla decidere il valore a `SetZoomAlpha(1)`.
	Cam->SetStrategicThresholdsForTest(/*Enter=*/ 1.0e6f, /*Exit=*/ 1.0e6f - 500.f);

	Cam->SetZoomAlpha(0.f);
	TestEqual(TEXT("a zoom minimo il pitch e' quello di default"), Cam->GetCameraPitch(), -40.f, 1e-3f);

	Cam->SetZoomAlpha(1.f);
	TestEqual(TEXT("a fondo corsa il pitch ha guadagnato il delta"), Cam->GetCameraPitch(), -60.f, 1e-3f);

	// L'orbita manuale e' un OFFSET sopra il derivato, e sopravvive al cambio di zoom.
	Cam->SetZoomAlpha(0.f);
	Cam->AddPitch(+10.f);
	TestEqual(TEXT("l'input inclina di dieci gradi"), Cam->GetCameraPitch(), -30.f, 1e-3f);
	TestEqual(TEXT("e l'offset lo registra"), Cam->GetManualPitchOffset(), 10.f, 1e-3f);

	Cam->SetZoomAlpha(1.f);
	TestEqual(TEXT("allontanandosi l'orbita manuale resta"), Cam->GetCameraPitch(), -50.f, 1e-3f);

	Cam->SetZoomAlpha(0.f);
	TestEqual(TEXT("e tornando indietro non si e' persa"), Cam->GetCameraPitch(), -30.f, 1e-3f);

	// Windup: un input enorme non deve accumulare un offset che il clamp nasconde. Dopo essere andati a
	// fondo corsa, UN passo nel verso opposto deve gia' rispondere.
	Cam->AddPitch(+1000.f);
	const float AtCeiling = Cam->GetCameraPitch();
	Cam->AddPitch(-5.f);
	TestTrue(TEXT("dopo il fondo corsa il verso opposto risponde subito"), Cam->GetCameraPitch() < AtCeiling);

	// La taratura riazzera l'orbita: `ApplyViewSettings` e' il ritorno allo stato tarato.
	Cam->SetZoomAlpha(0.f);
	Cam->AddPitch(+7.f);
	Cam->RecenterView();
	TestEqual(TEXT("Home riazzera l'orbita manuale"), Cam->GetManualPitchOffset(), 0.f, 1e-3f);

	DestroyCameraWorld(World);
	return true;
}

/**
 * 🔴 **Nel rettangolo della scena la camera inquadra la stessa ESTENSIONE VERTICALE dello schermo intero** (#3661,
 * D-497 (4)) — misurato sulla MATRICE DI PROIEZIONE, non su una formula.
 *
 * La camera consegna la sua vista (`GetCameraView`) e UE costruisce la matrice come la costruirebbe per il render,
 * con il vincolo d'aspetto del giocatore locale e due rettangoli: lo schermo intero e la scena fra le fasce. Con
 * `MaintainYFOV` l'altezza inquadrata sul piano del pivot dev'essere la stessa, e la larghezza seguire l'aspetto:
 * celle e unita' appaiono circa 0,64x, e soglie e zoom non si ritarano.
 *
 * 🔑 **Il vincolo e l'aspetto vengono da UE e dalla camera, non da costanti**: se il progetto cambiasse
 * `AspectRatioAxisConstraint` o `AspectRatio`, questo test cadrebbe invece di ripetere una garanzia di UE.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraSceneRectVerticalExtentTest,
	"RefactorTactics.Camera.SceneRectKeepsTheVerticalExtentOfTheFullScreen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraSceneRectVerticalExtentTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	SpawnCameraTestMap(World);

	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }
	USpringArmComponent* Arm = Cam->FindComponentByClass<USpringArmComponent>();
	UCameraComponent* View = Cam->FindComponentByClass<UCameraComponent>();
	if (!TestNotNull(TEXT("braccio"), Arm) || !TestNotNull(TEXT("camera component"), View))
	{
		DestroyCameraWorld(World);
		return false;
	}
	const float Distanza = Arm->TargetArmLength;
	FMinimalViewInfo Info;
	View->GetCameraView(0.f, Info);
	if (!TestTrue(TEXT("premessa: la vista tattica e' prospettica"), Info.ProjectionMode == ECameraProjectionMode::Perspective))
	{
		DestroyCameraWorld(World);
		return false;
	}

	const TEnumAsByte<EAspectRatioAxisConstraint> Vincolo = GetDefault<ULocalPlayer>()->AspectRatioAxisConstraint;
	auto Matrice = [Vincolo, &Info](const FIntRect& Quadro)
	{
		FSceneViewProjectionData Dati;
		Dati.SetViewRectangle(Quadro);
		FMinimalViewInfo::CalculateProjectionMatrixGivenViewRectangle(Info, Vincolo, Quadro, Dati);
		return Dati.ProjectionMatrix;
	};

	const FIntPoint Vp(1920, 1080);
	const FBox2D R = RTHudLayout::SceneRectPixels(Vp, RTHudLayout::UiScaleFor(Vp), RTHudLayout::Bands());
	const FIntRect Intero(0, 0, Vp.X, Vp.Y);
	const FIntRect Scena(FMath::RoundToInt(R.Min.X), FMath::RoundToInt(R.Min.Y), FMath::RoundToInt(R.Max.X), FMath::RoundToInt(R.Max.Y));

	// Mezze estensioni sul piano perpendicolare allo sguardo, alla distanza del pivot: in prospettiva il bordo del quadro
	// sta a `d / M[1][1]` in altezza e a `d / M[0][0]` in larghezza.
	const FMatrix ProiezioneIntero = Matrice(Intero);
	const FMatrix ProiezioneScena = Matrice(Scena);
	const double AltezzaIntero = Distanza / ProiezioneIntero.M[1][1];
	const double AltezzaScena = Distanza / ProiezioneScena.M[1][1];
	const double LarghezzaIntero = Distanza / ProiezioneIntero.M[0][0];
	const double LarghezzaScena = Distanza / ProiezioneScena.M[0][0];
	AddInfo(FString::Printf(TEXT("braccio %.0f: schermo intero %.1f x %.1f, scena %dx%d %.1f x %.1f (mezze estensioni)"),
		Distanza, LarghezzaIntero, AltezzaIntero, Scena.Width(), Scena.Height(), LarghezzaScena, AltezzaScena));

	TestEqual(TEXT("l'altezza inquadrata e' la stessa"), AltezzaScena, AltezzaIntero, AltezzaIntero * 1e-4);
	const double AspettoScena = static_cast<double>(Scena.Width()) / Scena.Height();
	const double AspettoIntero = static_cast<double>(Intero.Width()) / Intero.Height();
	TestEqual(TEXT("e la larghezza segue l'aspetto della scena"), LarghezzaScena / LarghezzaIntero,
		AspettoScena / AspettoIntero, 1e-3);
	TestTrue(TEXT("anti-vacuita': la scena non ha l'aspetto dello schermo"), FMath::Abs(AspettoScena - AspettoIntero) > 0.05);

	DestroyCameraWorld(World);
	return true;
}

// --- #3798 · D-525: il pivot sta nell'involucro convesso delle celle, non nel rettangolo ------------------

namespace
{
	/** I centri di un esagono di celle di raggio `Radius` attorno all'origine, sul piano. */
	TArray<FVector2D> CameraHexCentres(int32 Radius, float HexSize)
	{
		TArray<FVector2D> Out;
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			const FVector W = URTHexLibrary::AxialToWorld(Id, FVector::ZeroVector, HexSize, 0.f);
			Out.Add(FVector2D(W.X, W.Y));
		}
		return Out;
	}

	double CameraDistanceToHull(const TArray<FVector2D>& Hull, const FVector2D& P)
	{
		return FVector2D::Distance(P, ARTCameraPawn::ClampToConvexPolygon(Hull, P));
	}

	FBox2D CameraBoxOf(const TArray<FVector2D>& Points, double Margin)
	{
		FBox2D Box(ForceInit);
		for (const FVector2D& P : Points) { Box += P; }
		return FBox2D(Box.Min - FVector2D(Margin, Margin), Box.Max + FVector2D(Margin, Margin));
	}
}

/** L'involucro di un esagono di celle ha sei vertici, antiorari, e non conta i punti sui lati. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraConvexHullTest,
	"RefactorTactics.Camera.ConvexHullOfAHexagonOfCellsHasSixCorners",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraConvexHullTest::RunTest(const FString&)
{
	const TArray<FVector2D> Centres = CameraHexCentres(3, 150.f);
	const TArray<FVector2D> Hull = ARTCameraPawn::ComputeConvexHull(Centres);

	// Precondizione: i centri sono molti, e la maggior parte sta DENTRO o sui lati — un involucro che li
	// restituisse tutti non avrebbe nulla da escludere.
	TestTrue(TEXT("precondizione: molti piu' centri dei vertici"), Centres.Num() > 6 * 2);
	TestEqual(TEXT("sei vertici: i punti sui lati e dentro non contano"), Hull.Num(), 6);

	double Area2 = 0.0;
	for (int32 I = 0; I < Hull.Num(); ++I)
	{
		const FVector2D& A = Hull[I];
		const FVector2D& B = Hull[(I + 1) % Hull.Num()];
		Area2 += A.X * B.Y - B.X * A.Y;
	}
	TestTrue(TEXT("antiorario: l'area con segno e' positiva"), Area2 > 0.0);

	for (const FVector2D& C : Centres)
	{
		TestTrue(TEXT("ogni centro e' dentro l'involucro"), CameraDistanceToHull(Hull, C) < 1e-6);
	}

	// Il caso degenere dichiarato: meno di tre punti distinti, o tutti allineati, non e' un involucro.
	TArray<FVector2D> Line;
	for (int32 I = 0; I < 5; ++I) { Line.Add(FVector2D(I * 100.0, I * 50.0)); }
	TestTrue(TEXT("punti allineati: meno di tre vertici"), ARTCameraPawn::ComputeConvexHull(Line).Num() < 3);
	TArray<FVector2D> Same;
	Same.Add(FVector2D(5, 5));
	Same.Add(FVector2D(5, 5));
	TestEqual(TEXT("un solo punto ripetuto: un vertice"), ARTCameraPawn::ComputeConvexHull(Same).Num(), 1);
	return true;
}

/**
 * La zona del pivot taglia gli angoli del rettangolo e **non ne esce mai**; sui lati il margine e' quello deciso,
 * e col quadro ogni lato si ritira del 65% del supporto — la regola del rettangolo (`ComputeEffectivePivotBounds`)
 * generalizzata a ogni normale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraPivotPolygonTest,
	"RefactorTactics.Camera.PivotRegionCutsTheCornersOfTheBoxAndNeverLeavesIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraPivotPolygonTest::RunTest(const FString&)
{
	const float HexSize = 150.f;
	const double Margin = 3.0 * UE_SQRT_3 * HexSize;
	const TArray<FVector2D> Hull = ARTCameraPawn::ComputeConvexHull(CameraHexCentres(3, HexSize));
	const FBox2D Box = CameraBoxOf(Hull, Margin);
	const FVector2D NoFrame(0.0, 0.0);

	const TArray<FVector2D> Region = ARTCameraPawn::ComputeEffectivePivotPolygon(Box, Hull, Margin, NoFrame, 0.f, 0.35f);
	if (!TestTrue(TEXT("la zona esiste"), Region.Num() >= 3)) { return false; }

	// L'angolo del rettangolo: e' il punto che il rettangolo ammetteva e l'esagono no. Misurato in PIE: a
	// braccio 800 stava a sei celle dalla cella piu' vicina.
	const FVector2D Corner(Box.Max.X, Box.Max.Y);
	const double CornerBefore = CameraDistanceToHull(Hull, Corner);
	const FVector2D CornerAfter = ARTCameraPawn::ClampToConvexPolygon(Region, Corner);
	TestTrue(TEXT("anti-vacuita': l'angolo del rettangolo e' lontano dalle celle (piu' di una volta e mezza il margine)"),
		CornerBefore > 1.5 * Margin);
	// Un vertice della zona sta a `Margin / cos(30°)` dall'involucro (i lati si incontrano a spigolo vivo).
	TestTrue(TEXT("dopo il taglio il pivot sta entro il margine, a meno dello spigolo"),
		CameraDistanceToHull(Hull, CornerAfter) <= Margin / FMath::Cos(UE_DOUBLE_PI / 6.0) + 0.5);
	TestTrue(TEXT("e il taglio ha mosso il punto di piu' di mezzo margine"),
		FVector2D::Distance(Corner, CornerAfter) > 0.5 * Margin);

	// Contenuta nel rettangolo: nessun punto della griglia di prova e' ammesso dalla zona ed escluso dal rettangolo.
	int32 Dentro = 0;
	for (double X = Box.Min.X - 400.0; X <= Box.Max.X + 400.0; X += 97.0)
	{
		for (double Y = Box.Min.Y - 400.0; Y <= Box.Max.Y + 400.0; Y += 97.0)
		{
			const FVector2D P(X, Y);
			if (FVector2D::Distance(ARTCameraPawn::ClampToConvexPolygon(Region, P), P) < 1e-9)
			{
				++Dentro;
				TestTrue(TEXT("un punto della zona e' anche nel rettangolo"),
					P.X >= Box.Min.X - 1e-6 && P.X <= Box.Max.X + 1e-6 && P.Y >= Box.Min.Y - 1e-6 && P.Y <= Box.Max.Y + 1e-6);
			}
		}
	}
	TestTrue(TEXT("anti-vacuita': la zona contiene punti"), Dentro > 10);

	// Su ogni lato dell'involucro, dal suo punto medio spingendo lungo la normale: ci si ferma a `Margin`.
	for (int32 I = 0; I < Hull.Num(); ++I)
	{
		const FVector2D& A = Hull[I];
		const FVector2D& B = Hull[(I + 1) % Hull.Num()];
		const FVector2D Mid = (A + B) * 0.5;
		const FVector2D Edge = (B - A).GetSafeNormal();
		const FVector2D Normal(Edge.Y, -Edge.X);
		const FVector2D Reached = ARTCameraPawn::ClampToConvexPolygon(Region, Mid + Normal * (3.0 * Margin));
		TestTrue(*FString::Printf(TEXT("lato %d: il limite e' a un margine dal lato"), I),
			FMath::Abs(FVector2D::DotProduct(Reached - Mid, Normal) - Margin) < 0.5);
	}

	// Col quadro: ogni lato si ritira di `AllowedOutsideFraction` del SUPPORTO del quadro nella sua normale. Sui
	// lati orizzontali e verticali del rettangolo il supporto e' `ExtentX`/`ExtentY` di D-251: stessa cifra.
	const FVector2D Frame = ARTCameraPawn::ComputeFrameHalfExtents(800.f, -40.f, 90.f, 1.675f);
	TestTrue(TEXT("precondizione: il quadro e' noto"), Frame.X > 0.0 && Frame.Y > 0.0);
	for (const float Yaw : {0.f, 90.f, 37.f})
	{
		const FBox2D Rect = ARTCameraPawn::ComputeEffectivePivotBounds(Box, 800.f, -40.f, Yaw, 90.f, 1.675f, 0.35f);
		const TArray<FVector2D> Inset = ARTCameraPawn::ComputeEffectivePivotPolygon(Rect, Hull, Margin, Frame, Yaw, 0.35f);
		if (!TestTrue(TEXT("la zona col quadro esiste"), Inset.Num() >= 3)) { continue; }
		const double YawRad = FMath::DegreesToRadians(static_cast<double>(Yaw));
		const FVector2D Fwd(FMath::Cos(YawRad), FMath::Sin(YawRad));
		const FVector2D Rgt(-FMath::Sin(YawRad), FMath::Cos(YawRad));
		for (int32 I = 0; I < Hull.Num(); ++I)
		{
			const FVector2D& A = Hull[I];
			const FVector2D& B = Hull[(I + 1) % Hull.Num()];
			const FVector2D Mid = (A + B) * 0.5;
			const FVector2D Edge = (B - A).GetSafeNormal();
			const FVector2D Normal(Edge.Y, -Edge.X);
			const double Support = Frame.X * FMath::Abs(FVector2D::DotProduct(Normal, Fwd))
				+ Frame.Y * FMath::Abs(FVector2D::DotProduct(Normal, Rgt));
			const FVector2D Reached = ARTCameraPawn::ClampToConvexPolygon(Inset, Mid + Normal * (3.0 * Margin));
			TestTrue(*FString::Printf(TEXT("yaw %.0f, lato %d: margine meno il 65%% del supporto"), Yaw, I),
				FMath::Abs(FVector2D::DotProduct(Reached - Mid, Normal) - (Margin - 0.65 * Support)) < 0.75);
		}

		// Contenuta nel rettangolo ristretto anche col quadro e con yaw diversi da zero (code review di #3798).
		int32 DentroConQuadro = 0;
		for (double X = Box.Min.X - 300.0; X <= Box.Max.X + 300.0; X += 83.0)
		{
			for (double Y = Box.Min.Y - 300.0; Y <= Box.Max.Y + 300.0; Y += 83.0)
			{
				const FVector2D P(X, Y);
				if (FVector2D::Distance(ARTCameraPawn::ClampToConvexPolygon(Inset, P), P) < 1e-9)
				{
					++DentroConQuadro;
					TestTrue(*FString::Printf(TEXT("yaw %.0f: un punto della zona e' anche nel rettangolo ristretto"), Yaw),
						P.X >= Rect.Min.X - 1e-6 && P.X <= Rect.Max.X + 1e-6 && P.Y >= Rect.Min.Y - 1e-6 && P.Y <= Rect.Max.Y + 1e-6);
				}
			}
		}
		TestTrue(TEXT("anti-vacuita': anche la zona col quadro contiene punti"), DentroConQuadro > 10);
	}

	// 🔴 Braccio piccolo (misura in PIE del 2026-10-18, #3798): il margine di 3 celle e' assoluto, il quadro no. Senza
	// tetto il pivot si fermava a `Margin - 0.65 * Support` dal lato, oltre l'arrivo del quadro: scena vuota agli
	// spigoli a braccio 450. Con il tetto sta a non piu' di `MaxPivotOutsideSupport * Support`, quindi una cella e'
	// sempre nel quadro; e dove il quadro e' grande (braccio 800, sopra) il tetto non morde.
	{
		const FVector2D Small = ARTCameraPawn::ComputeFrameHalfExtents(450.f, -40.f, 90.f, 1.675f);
		TestTrue(TEXT("precondizione: il quadro piccolo e' noto"), Small.X > 0.0 && Small.Y > 0.0);
		for (const float Yaw : {0.f, 90.f, 37.f})
		{
			const FBox2D Rect = ARTCameraPawn::ComputeEffectivePivotBounds(Box, 450.f, -40.f, Yaw, 90.f, 1.675f, 0.35f);
			const TArray<FVector2D> Zone = ARTCameraPawn::ComputeEffectivePivotPolygon(Rect, Hull, Margin, Small, Yaw, 0.35f);
			if (!TestTrue(TEXT("la zona a braccio piccolo esiste"), Zone.Num() >= 3)) { continue; }
			const double YawRad = FMath::DegreesToRadians(static_cast<double>(Yaw));
			const FVector2D Fwd(FMath::Cos(YawRad), FMath::Sin(YawRad));
			const FVector2D Rgt(-FMath::Sin(YawRad), FMath::Cos(YawRad));
			double MaxOutside = 0.0;
			for (const FVector2D& V : Zone)
			{
				for (int32 I = 0; I < Hull.Num(); ++I)
				{
					const FVector2D& A = Hull[I];
					const FVector2D Edge = (Hull[(I + 1) % Hull.Num()] - A).GetSafeNormal();
					const FVector2D Normal(Edge.Y, -Edge.X);
					const double Support = Small.X * FMath::Abs(FVector2D::DotProduct(Normal, Fwd))
						+ Small.Y * FMath::Abs(FVector2D::DotProduct(Normal, Rgt));
					const double Out = FVector2D::DotProduct(Normal, V - A);
					TestTrue(*FString::Printf(TEXT("yaw %.0f: oltre il lato %d non piu' di meta' del supporto"), Yaw, I),
						Out <= ARTCameraPawn::MaxPivotOutsideSupport * Support + 0.75);
					MaxOutside = FMath::Max(MaxOutside, Out);
				}
			}
			// Anti-vacuita': senza tetto la zona sarebbe andata piu' in la' (oltre il margine meno il 65% del supporto).
			TestTrue(*FString::Printf(TEXT("yaw %.0f: la zona esce comunque dall'involucro"), Yaw), MaxOutside > 1.0);
			const FVector2D Vertex = ARTCameraPawn::ClampToConvexPolygon(Zone, FVector2D(Box.Max.X, Box.Max.Y));
			TestTrue(*FString::Printf(TEXT("yaw %.0f: dal pivot all'angolo la cella piu' vicina e' nel quadro"), Yaw),
				CameraDistanceToHull(Hull, Vertex) < FMath::Max(Small.X, Small.Y));
		}
	}

	// 🔴 Il quadro piu' grande dell'area (mappa piccola, zoom al massimo): il rettangolo collassa al centro e la
	// zona NON deve diventare un poligono degenere che ammette ogni punto (code review di #3798).
	{
		const FBox2D Piccola(FVector2D(-300.0, -300.0), FVector2D(300.0, 300.0));
		const FBox2D Collassato = ARTCameraPawn::ComputeEffectivePivotBounds(Piccola, 4000.f, -45.f, 0.f, 90.f, 1.675f, 0.35f);
		TestTrue(TEXT("precondizione: il rettangolo e' collassato in un punto"),
			FMath::IsNearlyEqual(Collassato.Min.X, Collassato.Max.X) && FMath::IsNearlyEqual(Collassato.Min.Y, Collassato.Max.Y));
		const TArray<FVector2D> SmallHull = ARTCameraPawn::ComputeConvexHull(CameraHexCentres(1, HexSize));
		// Quadro nullo di proposito: col quadro vero i semipiani si ritirano oltre il centro, la zona e' vuota
		// per un'altra ragione, e la guardia sul rettangolo senza area non verrebbe mai esercitata.
		TestEqual(TEXT("un rettangolo senza area non e' una zona: chi chiama ripiega sul rettangolo che lo inchioda"),
			ARTCameraPawn::ComputeEffectivePivotPolygon(Collassato, SmallHull, Margin, NoFrame, 0.f, 0.35f).Num(), 0);

		// E un poligono senza area non contiene nulla: restituisce il suo punto, non quello che gli si chiede.
		TArray<FVector2D> Punto;
		for (int32 I = 0; I < 4; ++I) { Punto.Add(FVector2D(10.0, 10.0)); }
		const FVector2D Lontano(500.0, -500.0);
		TestTrue(TEXT("un poligono di vertici coincidenti riporta il punto al vertice"),
			ARTCameraPawn::ClampToConvexPolygon(Punto, Lontano).Equals(FVector2D(10.0, 10.0), 1e-6));
	}

	// Senza involucro (meno di tre vertici) la zona non esiste e chi chiama ripiega sul rettangolo.
	TArray<FVector2D> TwoPoints;
	TwoPoints.Add(FVector2D(0, 0));
	TwoPoints.Add(FVector2D(1, 1));
	TestEqual(TEXT("senza involucro la zona e' vuota"),
		ARTCameraPawn::ComputeEffectivePivotPolygon(Box, TwoPoints, Margin, NoFrame, 0.f, 0.35f).Num(), 0);
	return true;
}

/**
 * Nel mondo: tenuto verso ciascuna diagonale, il pivot si ferma a un margine dalle celle e non a sei.
 * Prima di #3798 si fermava sull'angolo del rettangolo, e il quadro guardava il vuoto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraPanCornerTest,
	"RefactorTactics.Camera.PanTowardsACornerStopsWithinTheMarginOfTheCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraPanCornerTest::RunTest(const FString&)
{
	UWorld* World = MakeCameraWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* HexMap = SpawnCameraTestMap(World, FRTCellId(0, 0, 0));
	ARTCameraPawn* Cam = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("mappa"), HexMap) || !TestNotNull(TEXT("camera"), Cam)) { DestroyCameraWorld(World); return false; }

	FVector Origin; float HexSize = 0.f; float LayerHeight = 0.f;
	const URTHexMapAsset* Asset = HexMap->GetHexContext(Origin, HexSize, LayerHeight);
	if (!TestNotNull(TEXT("asset"), Asset)) { DestroyCameraWorld(World); return false; }
	TArray<FVector2D> Centres;
	for (const FRTHexCellData& Cell : Asset->Cells)
	{
		const FVector W = URTHexLibrary::AxialToWorld(Cell.Id, Origin, HexSize, LayerHeight);
		Centres.Add(FVector2D(W.X, W.Y));
	}
	const TArray<FVector2D> Hull = ARTCameraPawn::ComputeConvexHull(Centres);
	const double Margin = 3.0 * UE_SQRT_3 * static_cast<double>(HexSize);
	const FBox2D Box = CameraBoxOf(Centres, Margin);
	const double SpigoloVivo = Margin / FMath::Cos(UE_DOUBLE_PI / 6.0);

	// Controllo positivo: l'angolo del rettangolo e' piu' lontano di cosi' dalle celle, cioe' il test PUO' cadere.
	TestTrue(TEXT("anti-vacuita': l'angolo del rettangolo sta oltre lo spigolo vivo del margine"),
		CameraDistanceToHull(Hull, FVector2D(Box.Max.X, Box.Max.Y)) > SpigoloVivo + 100.0);

	const FVector2D Diagonali[4] = { FVector2D(1, 1), FVector2D(1, -1), FVector2D(-1, 1), FVector2D(-1, -1) };
	for (const float Yaw : {0.f, 90.f})
	{
		// Il pan e' relativo alla vista: con yaw 90 le diagonali sono le stesse, in un altro ordine. Si ruota con
		// lo scatto, che arriva al multiplo di 45 piu' vicino.
		while (FMath::Abs(Cam->GetCameraYaw() - Yaw) > 1.f) { Cam->AddYaw(1.f); }
		for (int32 D = 0; D < 4; ++D)
		{
			Cam->SetCameraPivotForTest(FVector::ZeroVector);
			for (int32 I = 0; I < 400; ++I) { Cam->AddPlanarMovement(Diagonali[D]); }
			const FVector P = Cam->GetCameraPivot();
			const double Dist = CameraDistanceToHull(Hull, FVector2D(P.X, P.Y));
			TestTrue(*FString::Printf(TEXT("yaw %.0f, diagonale %d: il pivot sta a %.0f dalle celle (massimo %.0f)"), Yaw, D, Dist, SpigoloVivo),
				Dist <= SpigoloVivo + 0.5);
			TestTrue(*FString::Printf(TEXT("yaw %.0f, diagonale %d: ed e' arrivato al bordo (%.0f)"), Yaw, D, Dist),
				Dist > Margin * 0.5);
		}
	}

	DestroyCameraWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
