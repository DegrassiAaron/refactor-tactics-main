#include "Camera/RTCameraPawn.h"
#include "RefactorTactics.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Map/RTHexLibrary.h"
#include "Engine/GameViewportClient.h"

/** Definita in `ScenarioHarness/RTTestConsole.cpp`: vista di misura a picco (`#1290`). */
extern TAutoConsoleVariable<int32> CVarRTCameraTopDown;
/** Definita nello stesso file: scatto automatico per il confronto fra catture (`#1290`). */
extern TAutoConsoleVariable<int32> CVarRTCameraTopDownShot;
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Player/RTPlayerController.h" // squadra del giocatore da inquadrare all'avvio
#include "Unit/RTUnit.h"
#include "EngineUtils.h"               // TActorIterator
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

ARTCameraPawn::ARTCameraPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(SceneRoot);
	// Clampata anche qui: e' il **quinto** punto che scrive questa distanza, e la prima stesura di #873 lo
	// aveva mancato — il commento diceva «clampata ovunque si scriva» mentre questa riga non lo faceva.
	// Oggi e' innocuo perche' i default di classe sono in scala, ma il giorno in cui il costruttore
	// ricevesse un valore da config o da un archetipo Blueprint tornerebbe a passare nudo.
	SpringArm->TargetArmLength = FMath::Clamp(DefaultArmLength, MinArmLength, MaxArmLength);
	// L'inclinazione corrente parte da quella di default: sono due campi da #863, e se nascessero
	// scollegati la camera aprirebbe a un'inclinazione che nessuno ha tarato.
	CameraPitch = FMath::Clamp(DefaultPitch, MinPitch, MaxPitch);
	SpringArm->SetRelativeRotation(FRotator(CameraPitch, 0.f, 0.f)); // inclinazione tunabile (era fissa a -55)
	SpringArm->bDoCollisionTest = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 10.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
}

void ARTCameraPawn::FocusOn(const FVector& WorldLocation)
{
	// Il pivot va **dove sta il bersaglio**, quota compresa. Lo zoom corrente non si tocca: quello resta
	// del giocatore, ed e' la meta' di contratto che #887 lascia esplicitamente intatta.
	//
	// 🔴 **Prima teneva la `Z` del pawn**, con il commento «la quota del pawn resta la sua (il braccio ci
	// pensa da se')». L'intento era ragionevole ma poggiava su una premessa mai verificata — che quella
	// quota fosse gia' quella del piano di gioco. Non lo era: `ARTGameMode` spawna il pawn al PlayerStart
	// del livello, e da li' **nessuno** la correggeva. `AddPlanarMovement` produce un delta con `Z = 0`
	// per scelta, `AddZoom` tocca solo il braccio, `AddYaw` solo la rotazione: l'unica funzione che
	// stabiliva una quota vera era `RecenterView`, cioe' il tasto `Home`.
	//
	// Risultato in partita (playtest M6.8, `#38`): premendo `F` la camera orbitava attorno a un punto
	// sospeso sopra o sotto l'unita' e si vedeva il vuoto — tanto piu' fuori quadro quanto piu' il
	// PlayerStart era distante dal piano. `Home` «riparava», ed e' il motivo per cui il difetto si notava
	// su `F` e non su `Home`.
	//
	// ⚠️ **La quota che arriva deve essere quella del PIANO, e non tutti i chiamanti la avevano.**
	// `FrameOwnTeam` passa `CellsCentroidWorld`, che somma `AxialToWorld` e la calcola. Il controller
	// passava `Unit->GetActorLocation()`, che sta **mezzo corpo sopra** il piano (`VisualZOffset`): ora
	// converte la cella. La prima stesura di #887 dava per buoni entrambi — sbagliato, e la divergenza
	// sarebbe stata una costante fra l'inquadratura di `F` e quella di `Home`.
	//
	// ⚠️ **Da `#1770` scrive il PIVOT e non la posizione.** Chi inquadra sposta cio' che la camera guarda;
	// un peek in corso resta un peek, e non viene inghiottito dalla nuova inquadratura.
	SetCameraPivot(WorldLocation);
}

void ARTCameraPawn::SetCameraPivot(const FVector& NewPivot)
{
	// Il clamp e' QUI, in un punto solo. Prima viveva in tre chiamanti — `AddPlanarMovement`,
	// `ZoomTowards` e nessun altro — e `FocusOn` non ce l'aveva affatto: inquadrare un'unita' oltre il
	// bordo portava la camera dove il pan si rifiuta di andare. Un invariante che vale in tre punti su
	// cinque non e' un invariante.
	CameraPivot = ClampToSoftBounds(NewPivot);
	ApplyPivot();
}

void ARTCameraPawn::SetPeekOffset(const FVector& Offset)
{
	// Limitato in LUNGHEZZA e non componente per componente: un clamp per asse produrrebbe un quadrato,
	// cioe' un peek piu' lungo in diagonale che sui lati — e il giocatore lo sentirebbe come una camera
	// che accelera quando guarda in un angolo.
	PeekOffset = Offset.GetClampedToMaxSize(FMath::Max(MaxPeekDistance, 0.f));
	ApplyPivot();
}

void ARTCameraPawn::ApplyPivot()
{
	// ⚠️ Il peek NON passa dai soft bounds: e' un offset di presentazione che rientra da se', e clamparlo
	// lo renderebbe irregolare proprio al bordo, che e' dove serve. Il **pivot** resta dentro i limiti, ed
	// e' quello che definisce dove ci si puo' fermare.
	SetActorLocation(CameraPivot + PeekOffset);
}

void ARTCameraPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	// Il pawn nasce dove lo mette il `PlayerStart` (o `SpawnActor` nei test): senza questa riga il pivot
	// resterebbe a zero e la prima scrittura di trasformata porterebbe la camera all'origine del mondo.
	CameraPivot = GetActorLocation();
}

void ARTCameraPawn::ApplyViewSettings()
{
	if (!SpringArm)
	{
		return;
	}
	// La distanza iniziale deve stare nello stesso intervallo che lo zoom rispetta: un default fuori range
	// darebbe una partenza che il primo scroll "corregge" di scatto.
	SpringArm->TargetArmLength = FMath::Clamp(DefaultArmLength, MinArmLength, MaxArmLength);

	// 🔴 **Il pitch corrente si riallinea alla taratura QUI, non nel costruttore.** La prima stesura di
	// #863 lo faceva nel costruttore, che gira sul CDO: un `DefaultPitch` messo nel Details di un'istanza
	// in livello — o in un archetipo Blueprint — non sarebbe mai arrivato al runtime, e la partita si
	// sarebbe aperta all'inclinazione serializzata invece che a quella tarata. `ApplyViewSettings` e'
	// chiamata da `BeginPlay` e da `PostEditChangeProperty`, cioe' esattamente nei due momenti in cui la
	// taratura deve prendere effetto. Trovato in code review.
	CameraPitch = FMath::Clamp(DefaultPitch, MinPitch, MaxPitch);
	SpringArm->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.f));

	// La distanza e' appena cambiata: lo stato strategico e' una **lettura** di quella distanza, e va
	// riletto qui invece di aspettare il prossimo scroll — altrimenti `Home` da vista strategica
	// lascerebbe lo stato indietro di un comando.
	RefreshViewportMetrics();
	UpdateStrategicState();
}

bool ARTCameraPawn::FrameOwnTeam()
{
	const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(GetWorld());
	if (!HexMap)
	{
		return false;
	}

	// La squadra da inquadrare e' quella del giocatore; senza controller si assume la 0 (demo).
	int32 TeamId = 0;
	if (const ARTPlayerController* PC = Cast<ARTPlayerController>(GetController()))
	{
		TeamId = PC->PlayerTeamId;
	}

	TArray<FRTCellId> Cells;
	for (TActorIterator<ARTUnit> It(GetWorld()); It; ++It)
	{
		if (It->TeamId == TeamId && It->IsAlive())
		{
			Cells.Add(It->Cell);
		}
	}
	if (Cells.Num() == 0)
	{
		return false; // unita' non ancora nel mondo: il chiamante riprova
	}

	FVector Origin; float HexSize; float LayerHeight;
	HexMap->GetHexContext(Origin, HexSize, LayerHeight);

	// Vista di MISURA: scavalca l'apertura sulla propria squadra e inquadra l'intera board dall'alto.
	// Serve alle verifiche che si fanno su un'immagine (`#1290`), dove la camera obliqua inquadrerebbe
	// bordi, lati e ombre — varieta' di luminanza che non viene dalla tavolozza.
	if (CVarRTCameraTopDown.GetValueOnGameThread() > 0 && ApplyTopDownView(HexMap, Origin, HexSize, LayerHeight))
	{
		return true;
	}

	FocusOn(URTHexLibrary::CellsCentroidWorld(Cells, Origin, HexSize, LayerHeight));
	if (SpringArm)
	{
		SpringArm->TargetArmLength = FMath::Clamp(MatchStartArmLength, MinArmLength, MaxArmLength);
	}
	UE_LOG(LogRT, Log, TEXT("[RT] Camera sulla squadra %d (%d unita', arm=%.0f)"),
		TeamId, Cells.Num(), SpringArm ? SpringArm->TargetArmLength : -1.f);
	return true;
}

bool ARTCameraPawn::ApplyTopDownView(const ARTHexMapActor* HexMap, const FVector& Origin, float HexSize, float LayerHeight)
{
	if (HexMap == nullptr || HexMap->MapAsset == nullptr || SpringArm == nullptr)
	{
		return false;
	}

	const FBox Box = URTHexLibrary::CellsBoundsWorld(HexMap->MapAsset->Cells, Origin, HexSize, LayerHeight);
	if (!Box.IsValid)
	{
		return false;
	}

	// L'inquadratura si DERIVA dal box, non da un numero scritto a mano: cosi' vale su qualunque fixture e
	// sopravvive a un cambio di `HexSize` — che e' appena successo, da 100 a 150.
	const FVector Extent = Box.GetExtent();
	const float Needed = FMath::Max(Extent.X, Extent.Y) * 2.2f;   // margine perche' il bordo non tocchi lo schermo

	FocusOn(FVector(Box.GetCenter().X, Box.GetCenter().Y, Box.Min.Z));
	SpringArm->TargetArmLength = FMath::Clamp(Needed, MinArmLength, MaxArmLength);

	// ⚠️ `MinPitch` e' `-89`, non `-90`: a picco pieno lo yaw diventa indeterminato e la camera puo'
	// ribaltarsi. Un grado di scarto non sposta la misura e toglie il caso degenere.
	CameraPitch = MinPitch;
	SpringArm->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.f));

	ScheduleTopDownShot();

	UE_LOG(LogRT, Warning, TEXT("[RT] rt.Camera.TopDown: vista di MISURA a picco sull'intera board "
		"(arm=%.0f, pitch=%.0f). Non e' la vista di gioco."),
		SpringArm->TargetArmLength, CameraPitch);
	return true;
}

void ARTCameraPawn::ScheduleTopDownShot()
{
	const int32 Seconds = CVarRTCameraTopDownShot.GetValueOnGameThread();
	if (Seconds <= 0 || GetWorld() == nullptr)
	{
		return;
	}

	// Il ritardo e' parte della misura: l'esposizione automatica impiega qualche decimo ad adattarsi, e uno
	// scatto immediato fotograferebbe una board a meta' adattamento — cioe' misurerebbe il transitorio.
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, [WeakThis = TWeakObjectPtr<ARTCameraPawn>(this)]()
	{
		ARTCameraPawn* Self = WeakThis.Get();
		if (Self == nullptr || GEngine == nullptr)
		{
			return;
		}
		// ⚠️ **Sul viewport client, non su `GEngine`.** `HighResShot` e' gestito da
		// `UGameViewportClient::Exec`, e `GEngine->Exec` non ce lo inoltra: il comando veniva accettato
		// senza produrre niente, e il log diceva «richiesto» perche' NON verificava il valore di ritorno.
		// Misurato: nessun file in `Saved/Screenshots/` e nessuna riga del motore.
		UGameViewportClient* Viewport = Self->GetWorld() ? Self->GetWorld()->GetGameViewport() : nullptr;
		const bool bAccepted = Viewport != nullptr
			&& Viewport->Exec(Self->GetWorld(), TEXT("HighResShot 1920x1080"), *GLog);

		if (bAccepted)
		{
			UE_LOG(LogRT, Warning,
				TEXT("[RT] rt.Camera.TopDownShot: HighResShot accettato. Il file esce in Saved/Screenshots/."));
		}
		else
		{
			// Fail-loud: uno scatto che non avviene deve dirlo, altrimenti si misura l'assenza del file
			// invece dell'immagine.
			UE_LOG(LogRT, Error,
				TEXT("[RT] rt.Camera.TopDownShot: HighResShot RIFIUTATO (viewport %s). Nessuna immagine."),
				Viewport ? TEXT("presente") : TEXT("assente"));
		}
	}, static_cast<float>(Seconds), /*bLoop=*/ false);
}

void ARTCameraPawn::BeginPlay()
{
	Super::BeginPlay();
	RefreshViewportMetrics();      // #1778: l'aspect ratio serve gia' alla prima inquadratura
	ApplyViewSettings(); // applica i valori correnti (anche se modificati in editor)

	// Si parte guardando le proprie unita', non il centro della mappa. Le unita' possono non esistere ancora:
	// l'ordine di BeginPlay fra actor non e' garantito, quindi si riprova al tick successivo — una volta sola —
	// e solo se anche quello fallisce si ripiega sull'inquadratura d'insieme. E' presentazione: il ritardo di un
	// frame non si vede e non tocca la simulazione.
	if (!FrameOwnTeam())
	{
		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (!FrameOwnTeam())
			{
				RecenterView();
			}
		}));
	}

	UE_LOG(LogRT, Log, TEXT("[RT] CameraPawn BeginPlay (arm=%.0f, pitch=%.0f)"),
		SpringArm ? SpringArm->TargetArmLength : -1.f, CameraPitch);
}

#if WITH_EDITOR
void ARTCameraPawn::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyViewSettings(); // tarare distanza/inclinazione dal Details ha effetto subito, anche in PIE
}
#endif

void ARTCameraPawn::AddPlanarMovement(const FVector2D& Axis)
{
	// Sul piano, ma RELATIVO a dove la camera guarda: `Axis.Y` e' «avanti sullo schermo», `Axis.X` e' «a
	// destra sullo schermo». Con yaw 0 il risultato e' identico a prima (nessuna inquadratura esistente
	// cambia comportamento); appena si ruota, e' cio' che impedisce a W di spostare la vista di lato.
	//
	// L'inclinazione non entra: si scorre sul piano della mappa, non lungo la direzione di sguardo, altrimenti
	// scorrere avvicinerebbe anche il terreno.
	const FVector Forward = FRotator(0.f, CameraYaw, 0.f).RotateVector(FVector::ForwardVector);
	const FVector Right   = FRotator(0.f, CameraYaw, 0.f).RotateVector(FVector::RightVector);
	// Lo scorrimento scala con la DISTANZA: a vista larga la stessa quantita' di input copre piu' terreno.
	// Senza, allontanandosi la mappa scorre a passi che sullo schermo diventano impercettibili — e' il
	// motivo per cui `PanSpeed` da solo non basta. Normalizzato su `DefaultArmLength`, cosi' alla distanza
	// di default il comportamento e' quello storico.
	const float DistanceScale = (SpringArm && DefaultArmLength > 0.f)
		? SpringArm->TargetArmLength / DefaultArmLength
		: 1.f;
	const FVector Delta = (Forward * Axis.Y + Right * Axis.X) * PanSpeed * DistanceScale;
	// Dal **pivot**, non dalla posizione: con un peek attivo, sommare alla posizione farebbe entrare
	// l'offset temporaneo nel movimento permanente — e lo scorrimento diventerebbe piu' veloce da un lato.
	SetCameraPivot(CameraPivot + Delta);
}

void ARTCameraPawn::AddYaw(float AxisValue)
{
	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}
	// Normalizzata subito: ruotando a lungo nella stessa direzione il valore crescerebbe senza fine, e il
	// campo in editor diventerebbe illeggibile pur descrivendo la stessa vista.
	CameraYaw = FRotator::ClampAxis(CameraYaw + FMath::Sign(AxisValue) * YawStep);

	// SOLO la rotazione, non `ApplyViewSettings()`: quella riporta anche il braccio a `DefaultArmLength`, e
	// ruotare avrebbe annullato lo zoom a ogni pressione. Sono due cose diverse che vivevano nella stessa
	// funzione perche' finora nessuno cambiava l'inquadratura senza volerla anche ripristinare.
	if (SpringArm)
	{
		SpringArm->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.f));
	}
}

void ARTCameraPawn::ApplyArmRotation()
{
	if (SpringArm)
	{
		SpringArm->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.f));
	}
}

// --- #1774 · lo stato strategico, con isteresi (D-252) -----------------------------------------------

bool ARTCameraPawn::UpdateStrategicState()
{
	if (!SpringArm)
	{
		return false;
	}

	// 🔑 **`Exit < Enter` e' imposto QUI, non solo documentato.** I due campi sono `BlueprintReadWrite` e il
	// loro `meta = (ClampMin)` vincola il Details e non un `Set` da Blueprint: con le soglie invertite una
	// disuguaglianza scritta a mano avrebbe prodotto uno stato che entra e non esce, o che sfarfalla — lo
	// stesso genere di difetto che l'isteresi esiste per evitare. Con `Max`/`Min` la coppia e' ordinata per
	// costruzione, e chi le inverte ottiene comunque un'isteresi valida.
	const float Enter = FMath::Max(StrategicEnterThreshold, StrategicExitThreshold);
	const float Exit  = FMath::Min(StrategicEnterThreshold, StrategicExitThreshold);

	const float Distance = SpringArm->TargetArmLength;
	const bool bWas = bStrategicView;

	if (!bStrategicView && Distance >= Enter)
	{
		bStrategicView = true;
	}
	else if (bStrategicView && Distance <= Exit)
	{
		bStrategicView = false;
	}

	if (bWas != bStrategicView)
	{
		// ⏳ **Non ha ancora un consumatore visivo**, e il log lo dice invece di lasciarlo dedurre: la
		// presentazione strategica — separazione verticale dei piani, densita' dei marker — e' `#1775`.
		// Questo e' lo stato, non la vista.
		UE_LOG(LogRT, Log, TEXT("[RT] Vista %s (arm=%.0f, enter=%.0f, exit=%.0f)"),
			bStrategicView ? TEXT("STRATEGICA") : TEXT("tattica"), Distance, Enter, Exit);
		return true;
	}
	return false;
}

// --- #1778 · i limiti del pivot che conoscono il viewport (D-251) -----------------------------------

FBox2D ARTCameraPawn::ComputeEffectivePivotBounds(const FBox2D& AllowedArea, float ArmLength,
	float PitchDegrees, float YawDegrees, float HorizontalFovDegrees, float AspectRatio,
	float AllowedOutsideFraction)
{
	// Aspect ratio ignoto (headless, o viewport non ancora creato): non c'e' una geometria di schermo su cui
	// ragionare, e l'area passa intatta. E' il **caso degenere dichiarato** — il clamp per sole celle di
	// `#864` resta il comportamento, e non e' un ripiego silenzioso.
	if (!AllowedArea.bIsValid || AspectRatio <= 0.f || ArmLength <= 0.f || HorizontalFovDegrees <= 0.f)
	{
		return AllowedArea;
	}

	const float HalfFovH = FMath::DegreesToRadians(FMath::Clamp(HorizontalFovDegrees, 1.f, 179.f) * 0.5f);
	const double HalfWidth = static_cast<double>(ArmLength) * FMath::Tan(HalfFovH);

	// L'estensione **lungo la direzione di sguardo** cresce quando la camera si abbassa: a picco il quadro
	// e' un rettangolo, all'orizzonte si allunga senza limite. E' il caso che rompe il clamp fisso, ed e'
	// per questo che il pitch entra nella formula.
	//
	// `sin` del pitch al denominatore, con un pavimento: a `MaxPitch = 0` la camera guarda l'orizzonte e la
	// proiezione a terra divergerebbe. Il pavimento non e' una tolleranza numerica — e' la dichiarazione
	// che oltre una certa inclinazione la nozione di «quanto mondo entra» smette di essere finita.
	const double VerticalHalf = HalfWidth / FMath::Max(static_cast<double>(AspectRatio), 0.01);
	const double SinPitch = FMath::Max(FMath::Abs(FMath::Sin(FMath::DegreesToRadians(PitchDegrees))), 0.15);
	const double HalfDepth = VerticalHalf / SinPitch;

	// Il quadro e' orientato dallo yaw: l'ingombro su X/Y e' l'AABB del rettangolo ruotato. Senza questo
	// passaggio i limiti sarebbero corretti solo con la vista dritta — cioe' proprio nel caso in cui la
	// rotazione, che `D-142` esiste per rendere libera, non e' stata usata.
	//
	// 🔴 **I due assi erano SCAMBIATI, e il test lo rendeva verde** (code review, 2026-08-30). A yaw 0 la
	// camera guarda lungo **+X** — non e' una deduzione, e' pinnato da
	// `Camera.PanIsRelativeToTheView`: *«yaw 0: avanti e' +X»*. Quindi lungo X va la profondita'
	// (`HalfDepth`, l'estensione nella direzione di sguardo) e lungo Y la larghezza di schermo
	// (`HalfWidth`); il codice scriveva l'opposto.
	//
	// ⚠️ **L'errore non era piccolo e cresceva dove serve di piu'**: `HalfDepth` diverge avvicinandosi a
	// `MaxPitch = 0`, quindi a vista radente il limite sull'asse di sguardo restava quello — stretto — di
	// una vista a picco, ed e' esattamente il caso che `D-251` esiste per coprire. Con arm 2000, pitch -45,
	// FOV 90 e 16:9: `HalfWidth = 2000`, `HalfDepth = 1591`, e i due finivano sull'asse sbagliato.
	const double Yaw = FMath::DegreesToRadians(YawDegrees);
	const double C = FMath::Abs(FMath::Cos(Yaw));
	const double S = FMath::Abs(FMath::Sin(Yaw));
	const double ExtentX = HalfDepth * C + HalfWidth * S;
	const double ExtentY = HalfDepth * S + HalfWidth * C;

	// Quanta parte del semiquadro puo' cadere fuori. `1` = nessun limite, `0` = quadro incollato dentro.
	const double Keep = 1.0 - static_cast<double>(FMath::Clamp(AllowedOutsideFraction, 0.f, 1.f));
	const FVector2D Inset(static_cast<float>(ExtentX * Keep), static_cast<float>(ExtentY * Keep));

	FBox2D Result(AllowedArea.Min + Inset, AllowedArea.Max - Inset);

	// ⚠️ **L'inquadratura puo' essere piu' grande dell'area** — mappa piccola, o zoom al massimo. Li'
	// l'intervallo si rovescerebbe, e un `Clamp` su estremi invertiti inchioda il valore a un estremo
	// **senza dirlo**: la camera resterebbe incollata a un angolo e nessuno saprebbe perche'. Il
	// comportamento giusto e' il centro: e' l'unico punto che minimizza il fuori-zona.
	const FVector2D Centre = AllowedArea.GetCenter();
	if (Result.Min.X > Result.Max.X) { Result.Min.X = Result.Max.X = Centre.X; }
	if (Result.Min.Y > Result.Max.Y) { Result.Min.Y = Result.Max.Y = Centre.Y; }
	return Result;
}

void ARTCameraPawn::RefreshViewportMetrics()
{
	ViewportHorizontalFov = Camera ? Camera->FieldOfView : ViewportHorizontalFov;

	// Nessun viewport (headless, o prima che ne esista uno): l'aspect ratio resta ignoto, e i limiti
	// tornano a essere quelli per sole celle. Dichiarato, non dedotto.
	const UWorld* World = GetWorld();
	const UGameViewportClient* Client = World ? World->GetGameViewport() : nullptr;
	if (!Client || !Client->Viewport)
	{
		return;
	}
	const FIntPoint Size = Client->Viewport->GetSizeXY();
	ViewportAspectRatio = (Size.Y > 0) ? static_cast<float>(Size.X) / static_cast<float>(Size.Y) : 0.f;
}

FVector ARTCameraPawn::ClampToSoftBounds(const FVector& Desired) const
{
	const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(GetWorld());
	if (!HexMap)
	{
		return Desired; // senza mappa non c'e' un bordo: niente da limitare
	}

	FVector Origin; float HexSize; float LayerHeight;
	const URTHexMapAsset* Map = HexMap->GetHexContext(Origin, HexSize, LayerHeight);
	if (!Map || Map->Cells.Num() == 0)
	{
		return Desired;
	}

	// Estensione reale delle celle, non un raggio assunto: una mappa dipinta a mano non e' un esagono
	// pieno, e dedurne i limiti dal numero di celle darebbe un bordo che non esiste.
	FVector Min(TNumericLimits<double>::Max());
	FVector Max(TNumericLimits<double>::Lowest());
	for (const FRTHexCellData& Cell : Map->Cells)
	{
		const FVector W = URTHexLibrary::AxialToWorld(Cell.Id, Origin, HexSize, LayerHeight);
		Min.X = FMath::Min(Min.X, W.X); Max.X = FMath::Max(Max.X, W.X);
		Min.Y = FMath::Min(Min.Y, W.Y); Max.Y = FMath::Max(Max.Y, W.Y);
	}

	// Il margine e' in CELLE (`#864`): un numero in unita' mondo sarebbe legato a `HexSize`, che e' un
	// dato della mappa. `√3 * HexSize` e' la distanza fra centri di celle adiacenti.
	// `FMath::Max` come per le sensibilita': il `meta = (ClampMin)` vincola il Details e **non** un `Set`
	// da Blueprint, e un margine negativo rovescerebbe l'intervallo — con `FMath::Clamp` su un intervallo
	// rovesciato la camera si inchioderebbe a un punto fisso e lo scorrimento smetterebbe di funzionare.
	const double Margin = static_cast<double>(FMath::Max(BoundsMarginCells, 0.f))
		* UE_SQRT_3 * static_cast<double>(HexSize);

	// La zona che si puo' mostrare: celle piu' il margine. ⏳ Quando `ScenicBufferArea` esistera' come dato
	// (`D-250`, `#1777`) e' **questo** il punto che deve leggerlo — il buffer allarga l'area consentita, non
	// il margine in celle.
	const FBox2D Allowed(FVector2D(Min.X - Margin, Min.Y - Margin), FVector2D(Max.X + Margin, Max.Y + Margin));

	// `#1778` / `D-251`: il limite vero dipende anche da quanto mondo entra nello schermo. Con l'aspect
	// ratio ignoto — headless, o viewport non ancora creato — `ComputeEffectivePivotBounds` restituisce
	// l'area intatta, e questo torna a essere il clamp per sole celle di `#864`.
	const FBox2D Effective = ComputeEffectivePivotBounds(Allowed,
		SpringArm ? SpringArm->TargetArmLength : 0.f, CameraPitch, CameraYaw,
		ViewportHorizontalFov, ViewportAspectRatio, AllowedOutsideFraction);

	return FVector(
		FMath::Clamp(Desired.X, static_cast<double>(Effective.Min.X), static_cast<double>(Effective.Max.X)),
		FMath::Clamp(Desired.Y, static_cast<double>(Effective.Min.Y), static_cast<double>(Effective.Max.Y)),
		Desired.Z);
}

void ARTCameraPawn::ZoomTowards(float AxisValue, const FVector& WorldAnchor)
{
	if (!SpringArm || FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	const float Before = SpringArm->TargetArmLength;
	// Un braccio a zero renderebbe `Ratio` infinito e la posizione non finita. `MinArmLength` non ha un
	// `ClampMin` nel `meta`, ed e' `BlueprintReadWrite`: nessuno impedisce lo zero.
	if (Before <= KINDA_SMALL_NUMBER)
	{
		AddZoom(AxisValue); // niente da ancorare: si comporta come lo zoom semplice
		return;
	}
	const float After = FMath::Clamp(Before + AxisValue * ZoomStep, MinArmLength, MaxArmLength);

	// Gia' a fondo corsa: la distanza non cambia, quindi non c'e' niente da compensare. Senza questa
	// guardia continuare a girare la rotellina al limite trascinerebbe la vista verso il cursore
	// all'infinito — uno scorrimento che nessuno ha chiesto, dal comando sbagliato.
	if (FMath::IsNearlyEqual(Before, After))
	{
		return;
	}
	SpringArm->TargetArmLength = After;

	// Il pivot si avvicina all'ancora in proporzione a quanto il braccio si e' accorciato: cosi' l'ancora
	// resta ferma sullo schermo. Solo X/Y — la quota del piano non cambia zoomando (`#887`).
	const FVector Pivot = CameraPivot; // il riferimento, non la posizione: il peek non entra nello zoom
	const float Ratio = After / Before;
	const FVector Moved = WorldAnchor + (Pivot - WorldAnchor) * Ratio;

	// 🔴 **Anche lo zoom passa dai limiti.** La prima stesura scriveva il pivot diretto, e allontanandosi
	// il fattore `Ratio > 1` lo spingeva via **cumulativamente**: da braccio 100 a 4000 sono quaranta
	// volte l'offset iniziale, cioe' centinaia di celle fuori mappa. Riapriva esattamente il buco che
	// questa issue chiude — «ci si puo' allontanare fino a perdere la mappa» — dal comando che avrebbe
	// dovuto risolverlo. Trovato in code review.
	// ⚠️ **Le metriche PRIMA di scrivere il pivot**: `SetCameraPivot` clampa leggendo aspect ratio e FOV, e
	// aggiornarle dopo faceva usare al primo zoom della sessione — o al primo dopo un ridimensionamento
	// della finestra — i valori precedenti. Con `ViewportAspectRatio` ancora a zero i limiti di `D-251`
	// venivano saltati del tutto per quella scrittura. Trovato in code review.
	RefreshViewportMetrics();
	SetCameraPivot(FVector(Moved.X, Moved.Y, Pivot.Z));
	UpdateStrategicState();
}

void ARTCameraPawn::AddYawContinuous(float AxisValue)
{
	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}
	// Stessa normalizzazione dello scatto: il valore resta leggibile in editor invece di crescere senza
	// fine. E stesso stato — `CameraYaw` — quindi trascinare e poi premere `Q` non sono due sistemi che si
	// contendono la vista: il tasto riparte da dove il trascinamento ha lasciato.
	CameraYaw = FRotator::ClampAxis(CameraYaw + AxisValue * FMath::Max(YawSensitivity, 0.f));
	ApplyArmRotation();
}

void ARTCameraPawn::AddPitch(float AxisValue)
{
	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}
	// Clampata QUI. Il `meta = (ClampMin/ClampMax)` sul campo vincola il widget del Details e non le
	// assegnazioni da codice: #863 lo dava per un vincolo a runtime, e non lo e'. Senza questa riga
	// trascinare a lungo porterebbe la vista oltre lo zenit, dove `FRotator` si rovescia.
	//
	// ⚠️ E la sensibilita' e' portata a zero se qualcuno la mettesse negativa: e' `BlueprintReadWrite`, e
	// il suo `meta = (ClampMin)` vincola il Details **e non un `Set` da Blueprint** — la stessa lezione,
	// applicata al campo che la enuncia invece che solo a quello che la subiva.
	CameraPitch = FMath::Clamp(CameraPitch + AxisValue * FMath::Max(PitchSensitivity, 0.f),
		MinPitch, MaxPitch);
	ApplyArmRotation();
}

void ARTCameraPawn::AddOrbit(float DeltaYaw, float DeltaPitch)
{
	// I due campi si scrivono qui e la trasformata **una volta sola**: chiamare i due `Add*` in fila
	// aggiornerebbe il braccio due volte per frame di trascinamento, e la seconda scrittura scarterebbe
	// il risultato della prima.
	if (!FMath::IsNearlyZero(DeltaYaw))
	{
		CameraYaw = FRotator::ClampAxis(CameraYaw + DeltaYaw * FMath::Max(YawSensitivity, 0.f));
	}
	if (!FMath::IsNearlyZero(DeltaPitch))
	{
		const float Signed = bInvertOrbitPitch ? -DeltaPitch : DeltaPitch;
		CameraPitch = FMath::Clamp(CameraPitch + Signed * FMath::Max(PitchSensitivity, 0.f),
			MinPitch, MaxPitch);
	}
	ApplyArmRotation();
}

void ARTCameraPawn::AddZoom(float AxisValue)
{
	if (!SpringArm)
	{
		return;
	}
	SpringArm->TargetArmLength = FMath::Clamp(SpringArm->TargetArmLength + AxisValue * ZoomStep, MinArmLength, MaxArmLength);
	RefreshViewportMetrics();

	// 🔴 **Il pivot va RI-CLAMPATO, perche' i suoi limiti dipendono dalla distanza** (code review,
	// 2026-08-30). Da `D-251` l'area legale si stringe quando ci si allontana: allontanandosi stando al
	// bordo, il pivot restava dov'era — fuori dai limiti nuovi — e nessuno lo riportava dentro fino al pan
	// successivo. `ZoomTowards` non aveva il difetto perche' scrive il pivot per conto suo; questo ramo
	// toccava solo il braccio.
	SetCameraPivot(CameraPivot);
	UpdateStrategicState();
}

void ARTCameraPawn::RecenterView()
{
	// Centra sulla mappa ESAGONALE del livello (se presente) e ripristina lo zoom di default. La mappa non e'
	// per forza centrata sull'origine: il centro viene dal bounding box delle sue celle.
	// 🔑 **Orientamento e distanza si azzerano PRIMA di scrivere il pivot** (code review, 2026-08-30).
	// `SetCameraPivot` clampa usando yaw, pitch e braccio **correnti**: scrivendo il pivot per primo, il
	// clamp descriveva la camera che stava per sparire invece di quella che sarebbe esistita un istante
	// dopo. Premendo `Home` da zoom massimo su una mappa piccola l'inquadratura superava l'area, il pivot
	// si inchiodava al centro calcolato con i valori vecchi, e nessuno lo ricalcolava dopo il reset.
	CameraYaw = 0.f;
	CameraPitch = FMath::Clamp(DefaultPitch, MinPitch, MaxPitch);
	ApplyViewSettings();

	if (const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(GetWorld()))
	{
		FVector Origin; float HexSize; float LayerHeight;
		if (const URTHexMapAsset* Map = HexMap->GetHexContext(Origin, HexSize, LayerHeight))
		{
			SetCameraPivot(URTHexLibrary::AxialToWorld(Map->GetCenterCell(), Origin, HexSize, LayerHeight));
		}
		else
		{
			SetCameraPivot(Origin);
		}
	}

	// `Home` e' «riportami a un'inquadratura che conosco», quindi azzera anche il peek: tornare al centro
	// con un offset residuo lascerebbe la vista storta senza che nulla lo dica.
	PeekOffset = FVector::ZeroVector;
	ApplyPivot();
	// Anche la ROTAZIONE torna a zero — **applicata sopra**, prima della scrittura del pivot. `Home` e' il
	// tasto del «riportami a un'inquadratura che conosco»: se riportasse la posizione ma lasciasse la vista
	// girata, chi si e' perso ruotando resterebbe perso.

	// ⚠️ **Inclinazione e distanza sono gia' state applicate in testa alla funzione**, e non e' una scelta
	// di stile: `SetCameraPivot` clampa leggendo yaw, pitch e braccio, quindi devono descrivere la camera
	// che esistera' dopo il reset e non quella che sta per sparire. Le righe stavano qui, e da qui il clamp
	// vedeva i valori vecchi.
	//
	// Restano le due ragioni per cui esistono, che valgono ancora:
	// — l'inclinazione torna alla taratura perche' da #863 il pitch e' regolabile a runtime, e
	//   «un'inquadratura che conosco» include quanto la vista e' inclinata;
	// — la distanza la applica `ApplyViewSettings` e non una copia locale: il difetto che #873 chiude era
	//   `DefaultArmLength` scritto **nudo** mentre gli altri punti lo clampavano, e la prima stesura della
	//   correzione copiava le due righe invece di chiamarla — ricreando un livello piu' in la' la
	//   divergenza che #873 esiste per chiudere.
}
