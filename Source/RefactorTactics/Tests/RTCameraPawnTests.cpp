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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraAltDragOrbitsOnlyPastThresholdTest,
	"RefactorTactics.Camera.AltDragOrbitsOnlyPastTheClickThreshold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraAltDragOrbitsOnlyPastThresholdTest::RunTest(const FString&)
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
	PC->SetAltPrimaryDownForTest(true, 0.f);
	TestTrue(TEXT("il gesto Alt consuma il movimento"), PC->RouteCameraGestureForTest(FVector2D(3.f, 0.f)));
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

	// `Alt` da solo + movimento = peek.
	PC->SetCameraModifierForTest(true);
	TestTrue(TEXT("Alt da solo consuma il movimento come peek"),
		PC->RouteCameraGestureForTest(FVector2D(20.f, 0.f)));
	TestFalse(TEXT("il peek si e' aperto"), Cam->GetPeekOffset().IsNearlyZero());
	TestEqual(TEXT("e il pivot non si e' mosso"), Cam->GetCameraPivot(), Pivot);

	// Con `Alt` ancora premuto il rientro NON deve partire: tirare contro il giocatore mentre sta
	// guidando e' la forma piu' fastidiosa di camera automatica.
	const FVector Held = Cam->GetPeekOffset();
	PC->UpdatePeekReturnForTest(0.1f);
	TestEqual(TEXT("con Alt premuto il peek resta dov'e'"), Cam->GetPeekOffset(), Held);

	// Rilasciato `Alt`, rientra fino a zero — e il pivot e' sempre quello.
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

	// Ultrawide. ⚠️ Il messaggio di questa riga ha gia' mentito una volta (diceva «stringe» mentre
	// l'asserzione verifica il contrario) ed e' stato corretto il 2026-08-30 rileggendo il diff. Ora dice
	// anche l'asse giusto: a yaw 0 la profondita' vive su **X**.
	//
	// Cio' che l'asserzione dice, ed e' geometricamente corretto a FOV **orizzontale** fisso: un ultrawide
	// ha un FOV verticale minore, quindi vede meno in profondita' e il limite sull'asse di sguardo si
	// allarga.
	const FBox2D Wide = ARTCameraPawn::ComputeEffectivePivotBounds(Area, 2000.f, -45.f, 0.f, Fov, 32.f/9.f, Fraction);
	TestTrue(TEXT("a FOV orizzontale fisso, 32:9 ALLARGA il limite sull'asse di sguardo (X)"),
		Wide.Max.X > Far.Max.X);

	// 🔴 **Questa riga PINNA UN DIFETTO NOTO, non un comportamento voluto — e va letta prima di
	// «correggerla».**
	//
	// A yaw 0 `ExtentX` vale `HalfWidth`, che non dipende dall'aspect ratio: allargando lo schermo il
	// limite orizzontale non si muove di un'unita'. E' corretto solo se il FOV **orizzontale** e' davvero
	// fisso — ma il progetto non imposta `AspectRatioAxisConstraint` (`grep -rn` su `Source/` e `Config/`
	// non trova nulla), quindi vale il default di UE `AspectRatio_MaintainYFOV`: su schermi larghi resta
	// fisso il FOV **verticale** e cresce quello orizzontale effettivo, mentre `Camera->FieldOfView` —
	// che e' cio' che questa funzione riceve — non cambia.
	//
	// ∴ su 32:9 l'inset X e' probabilmente **piu' piccolo del dovuto**, cioe' il limite e' piu' permissivo
	// proprio sul caso che `D-251` esiste per coprire. Non e' stato corretto qui perche' la formula giusta
	// dipende da quale asse UE consideri «il FOV» sotto quel constraint, e quello si verifica **guardando**
	// (verifica PIE ultrawide di `#1778`), non deducendo: riscrivere la matematica su una supposizione
	// produrrebbe numeri diversi con la stessa fiducia.
	//
	// ⚠️ Il giorno in cui la formula terra' conto dell'aspect sull'asse X, **questo test diventera' rosso —
	// ed e' il segnale che la correzione e' arrivata**, non un fallimento da aggirare.
	TestEqual(TEXT("DIFETTO PINNATO: a yaw 0 il limite LATERALE non dipende dall'aspect ratio"),
		Wide.Max.Y, Far.Max.Y);

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

#endif // WITH_DEV_AUTOMATION_TESTS
