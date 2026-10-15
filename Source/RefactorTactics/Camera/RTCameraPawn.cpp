#include "Camera/RTCameraPawn.h"
#include "RefactorTactics.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInstanceDynamic.h" // #3705: il post-process del contorno delle unita'
#include "Map/RTHexLibrary.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h" // #3661: il rettangolo del giocatore per l'aspetto del limite del pivot

/** Definita in `ScenarioHarness/RTTestConsole.cpp`: vista di misura a picco (`#1290`). */
extern TAutoConsoleVariable<int32> CVarRTCameraTopDown;
/** Definita nello stesso file: scatto automatico per il confronto fra catture (`#1290`). */
extern TAutoConsoleVariable<int32> CVarRTCameraTopDownShot;
#include "GameFramework/PlayerController.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Player/RTPlayerState.h" // squadra del giocatore da inquadrare all'avvio
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

	// #3723: l'esposizione nasce fissa, coi default. Le leve arrivano dopo, da `SyncExposure`.
	RTCameraExposure::Apply(AppliedExposureTuning, Camera->PostProcessSettings);

	// #3705: il percorso ha un owner, ed e' lo stesso che il commandlet scrive. Soft: non si carica finche' non serve.
	UnitLayerOutlineMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(
		FString::Printf(TEXT("%s.%s"), RTUnitLayerOutline::MaterialPackage, RTUnitLayerOutline::MaterialName)));
}

bool ARTCameraPawn::SyncUnitLayerOutline(const FRTUnitLayerOutlineTuning& Tuning)
{
	if (Camera == nullptr)
	{
		return false;
	}

	if (UnitLayerOutlineDynMaterial == nullptr)
	{
		if (bOutlineMaterialResolved)
		{
			return false; // gia' tentato, l'asset non c'e': nessun contorno, e nessun caricamento fallito a ogni fotogramma
		}
		bOutlineMaterialResolved = true;

		UMaterialInterface* Base = UnitLayerOutlineMaterial.LoadSynchronous();
		if (Base == nullptr)
		{
			UE_LOG(LogRT, Warning, TEXT("[RT] %s non risolve: le unita' non avranno il contorno del piano (#3705)."),
				*UnitLayerOutlineMaterial.ToString());
			return false;
		}
		UnitLayerOutlineDynMaterial = UMaterialInstanceDynamic::Create(Base, this);
		if (UnitLayerOutlineDynMaterial == nullptr)
		{
			return false;
		}

		// I tre colori NON stanno nell'asset: vengono dalla regola dei bordi, in lineare come il materiale li usa.
		const uint8 Stencils[] = { RTUnitLayerOutline::StencilActive, RTUnitLayerOutline::StencilAbove,
			RTUnitLayerOutline::StencilBelow };
		for (const uint8 Stencil : Stencils)
		{
			FColor Colore;
			if (RTUnitLayerOutline::ColorFor(Stencil, Colore))
			{
				UnitLayerOutlineDynMaterial->SetVectorParameterValue(RTUnitLayerOutline::ColorParamFor(Stencil),
					FLinearColor::FromSRGBColor(Colore));
			}
		}
		bOutlineTuningApplied = false;
	}

	if (bOutlineTuningApplied && AppliedOutlineTuning == Tuning)
	{
		return false; // il caso di ogni fotogramma
	}
	AppliedOutlineTuning = Tuning;
	bOutlineTuningApplied = true;

	UnitLayerOutlineDynMaterial->SetScalarParameterValue(RTUnitLayerOutline::ParamThickness, Tuning.ThicknessPx);
	// `AddBlendable` aggiorna il peso se l'oggetto c'e' gia': spento vuol dire peso zero, e il passaggio non gira.
	Camera->PostProcessSettings.AddBlendable(UnitLayerOutlineDynMaterial.Get(), Tuning.bEnabled ? 1.f : 0.f);
	return true;
}

bool ARTCameraPawn::SyncExposure(const FRTCameraExposureTuning& Tuning)
{
	if (Camera == nullptr || AppliedExposureTuning == Tuning)
	{
		return false; // il caso di ogni fotogramma
	}
	AppliedExposureTuning = Tuning;
	RTCameraExposure::Apply(Tuning, Camera->PostProcessSettings);
	return true;
}

float ARTCameraPawn::GetUnitLayerOutlineWeightForTest() const
{
	if (Camera != nullptr && UnitLayerOutlineDynMaterial != nullptr)
	{
		for (const FWeightedBlendable& Voce : Camera->PostProcessSettings.WeightedBlendables.Array)
		{
			if (Voce.Object == UnitLayerOutlineDynMaterial)
			{
				return Voce.Weight;
			}
		}
	}
	return -1.f;
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

	// La distanza e' appena cambiata: lo stato strategico e' una **lettura** di quella distanza, e va riletto
	// qui invece di aspettare il prossimo scroll — altrimenti `Home` da vista strategica lascerebbe lo stato
	// indietro di un comando. E va riletto **prima del pitch**, che da D-498 dipende dallo stato.
	UpdateStrategicState();

	// 🔴 **Il pitch corrente si riallinea alla taratura QUI, non nel costruttore.** La prima stesura di
	// #863 lo faceva nel costruttore, che gira sul CDO: un `DefaultPitch` messo nel Details di un'istanza
	// in livello — o in un archetipo Blueprint — non sarebbe mai arrivato al runtime, e la partita si
	// sarebbe aperta all'inclinazione serializzata invece che a quella tarata. `ApplyViewSettings` e'
	// chiamata da `BeginPlay` e da `PostEditChangeProperty`, cioe' esattamente nei due momenti in cui la
	// taratura deve prendere effetto. Trovato in code review.
	// L'input manuale accumulato si azzera insieme alla taratura: `ApplyViewSettings` e' il ritorno allo
	// stato tarato, e conservare l'offset qui farebbe ripartire la partita inclinata come l'ultima orbita.
	ManualPitchOffset = 0.f;
	RecomputePitchFromZoom();
	SpringArm->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.f));
	RefreshViewportMetrics();
}

bool ARTCameraPawn::CollectOwnTeamCells(int32 TeamId, TArray<FRTCellId>& OutCells) const
{
	// ⚠️ **Quinto canale di presentazione, e [#1500] non lo nominava: il gesto qui non e'
	// `GetAllActorsOfClass` ma `TActorIterator<ARTUnit>`.** Un confine espresso come «vietato chiamare quella
	// funzione» nascerebbe bucato proprio qui. Questo ciclo legge `It->TeamId` e `It->Cell` di **tutte** le
	// unita' del mondo, velate comprese, per tenere solo le proprie.
	//
	// ✅ **Il filtro pero' e' il TeamId dell'osservatore stesso**, quindi l'insieme che sopravvive e' fatto di
	// unita' che il giocatore possiede: la cella di un'unita' propria non e' un fatto che il velo nasconda, e
	// la risposta che l'Actor da' e' quella giusta. ⛔ Diventerebbe un canale il giorno in cui un chiamante
	// inquadrasse qualcosa di NON proprio — un nemico, un obiettivo conteso, l'ultimo contatto: in quel caso
	// il soggetto va preso da `FRTKnowledgeView`, e il debito e' gia' dichiarato in [D-143]. Lo chiamano
	// `FrameOwnTeam` e `FrameHome` (#1837), e nessuno dei due inquadra altro che la propria squadra.
	OutCells.Reset();
	for (TActorIterator<ARTUnit> It(GetWorld()); It; ++It)
	{
		if (IsOwnLiveUnit(*It, TeamId))
		{
			OutCells.Add(It->Cell);
		}
	}
	return OutCells.Num() > 0;
}

bool ARTCameraPawn::FrameOwnTeam()
{
	const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(GetWorld());
	if (!HexMap)
	{
		return false;
	}

	// La squadra da inquadrare e' quella del giocatore; senza PlayerState si assume la 0 (demo).
	const int32 TeamId = ARTPlayerState::TeamIdOf(Cast<APlayerController>(GetController()));

	TArray<FRTCellId> Cells;
	if (!CollectOwnTeamCells(TeamId, Cells))
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
	// Dalla porta comune del braccio, che rilegge lo stato prima di pitch e proiezione e ri-clampa il pivot: con
	// `MatchStartArmLength` oltre la soglia d'ingresso la partita partirebbe a distanza strategica con la vista tattica.
	ApplyArmLength(MatchStartArmLength);
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
	const float DistanceScale = GetPanDistanceScale();
	const FVector Delta = (Forward * Axis.Y + Right * Axis.X) * PanSpeed * DistanceScale;
	// Dal **pivot**, non dalla posizione: con un peek attivo, sommare alla posizione farebbe entrare
	// l'offset temporaneo nel movimento permanente — e lo scorrimento diventerebbe piu' veloce da un lato.
	SetCameraPivot(CameraPivot + Delta);
}

FVector2D ARTCameraPawn::ComputeEdgePanAxis(const FVector2D& Cursor, const FVector2D& ViewportSize, float BandPixels)
{
	if (ViewportSize.X <= 0.0 || ViewportSize.Y <= 0.0 || BandPixels <= 0.f)
	{
		return FVector2D::ZeroVector;
	}
	// Fuori dal viewport non c'e' una banda: il cursore sta altrove, e la camera non deve correre da sola.
	if (Cursor.X < 0.0 || Cursor.Y < 0.0 || Cursor.X > ViewportSize.X || Cursor.Y > ViewportSize.Y)
	{
		return FVector2D::ZeroVector;
	}

	// Penetrazione: 0 sul bordo interno della banda, 1 sul bordo del viewport, lineare in mezzo — quindi
	// monotona. La curva vera si tara in #1780; qui conta solo che cresca avvicinandosi al bordo.
	const double Band = BandPixels;
	auto Penetration = [Band](double DistanceFromEdge)
	{
		return FMath::Clamp(1.0 - DistanceFromEdge / Band, 0.0, 1.0);
	};
	const double Left = Penetration(Cursor.X);
	const double Right = Penetration(ViewportSize.X - Cursor.X);
	const double Top = Penetration(Cursor.Y);
	const double Bottom = Penetration(ViewportSize.Y - Cursor.Y);
	// Lo schermo ha la Y verso il basso; l'asse del pan ha `Y` «avanti», cioe' verso il bordo alto.
	return FVector2D(Right - Left, Top - Bottom);
}

bool ARTCameraPawn::ApplyEdgePan(const FVector2D& Axis)
{
	if (!bEdgePanEnabled || Axis.IsNearlyZero())
	{
		return false;
	}
	++EdgePanUpdates;
	// ⛔ Nessuna seconda formula di velocita': e' il difetto che #1834 esiste per prevenire. L'edge pan e' un
	// ingresso di `AddPlanarMovement`, come `WASD`.
	AddPlanarMovement(Axis);
	return true;
}

void ARTCameraPawn::AddYaw(float AxisValue)
{
	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}
	// Normalizzata subito: ruotando a lungo nella stessa direzione il valore crescerebbe senza fine, e il
	// campo in editor diventerebbe illeggibile pur descrivendo la stessa vista.
	// D-499: in iso il passo e' quello degli assi degli esagoni (60); in tattica resta D-142 (45).
	const float Passo = bStrategicView ? StrategicYawStep : YawStep;
	CameraYaw = FRotator::ClampAxis(CameraYaw + FMath::Sign(AxisValue) * Passo);

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

	float Enter = 0.f;
	float Exit = 0.f;
	GetOrderedStrategicThresholds(Enter, Exit);

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

	// Il ritorno del secondo `Tab` (D-488): l'ultima distanza letta da TATTICI, qualunque sia stata la via
	// d'ingresso. Si scrive dopo la valutazione, cosi' la lettura che entra in strategica non la sovrascrive.
	if (!bStrategicView)
	{
		LastTacticalArmLength = Distance;
	}

	// A ogni lettura e non solo al cambio di stato, perche' in ortografica la larghezza segue il braccio.
	ApplyStrategicProjection();

	if (bWas != bStrategicView)
	{
		// D-499: in iso l'imbardata sta sugli assi degli esagoni. Entrando si allinea al multiplo di 60 piu' vicino e
		// si ricorda quella tattica; uscendo torna quella, come l'inclinazione (l'offset manuale non si tocca).
		if (bStrategicView)
		{
			TacticalYawBeforeStrategic = CameraYaw;
			CameraYaw = SnapYawToStep(CameraYaw, StrategicYawStep);
		}
		else
		{
			CameraYaw = TacticalYawBeforeStrategic;
		}

		// D-498: il pitch e' una funzione dello STATO, e cambia nel fotogramma in cui cambia lo stato (il taglio netto
		// di D-495 (4)). Le porte del braccio lo ricalcolano gia' dopo questa lettura; qui vale per chi rilegge lo
		// stato da solo.
		RecomputePitchFromZoom();
		ApplyArmRotation();
		// Il consumatore visivo e' `ARTHUD::UpdateObserverVeil`, che a ogni fotogramma legge `IsStrategicView`
		// e lo consegna alle unita' (`ARTUnit::SetStrategicPresentation`, D-495). Il log resta: dice QUANDO
		// lo stato e' cambiato, che a schermo si vede ma non si data.
		UE_LOG(LogRT, Log, TEXT("[RT] Vista %s (arm=%.0f, enter=%.0f, exit=%.0f)"),
			bStrategicView ? TEXT("STRATEGICA") : TEXT("tattica"), Distance, Enter, Exit);
		return true;
	}
	return false;
}

float ARTCameraPawn::OrthoWidthForArm(float ArmLength, float HorizontalFovDegrees, float ReferenceAspectRatio)
{
	// L'ALTEZZA del quadro prospettico sul piano del pivot, perche' con `MaintainYFOV` e' quella che `OrthoWidth`
	// descrive (vedi l'header). Il semiangolo verticale e' quello che UE ricava in `CameraStackTypes.cpp`:
	// `atan(tan(FOV/2) / AspectRatio)`, quindi la mezza altezza a distanza `Arm` vale `Arm * tan(FOV/2) / AspectRatio`.
	// ⚠️ Fino a #3662 qui mancava la divisione, e l'ortografica inquadrava 1,78× la prospettica: il test di allora
	// confrontava questa formula con sé stessa. Quello di oggi misura la matrice di proiezione.
	const float HalfFov = FMath::DegreesToRadians(FMath::Clamp(HorizontalFovDegrees, 1.f, 179.f) * 0.5f);
	return 2.f * FMath::Max(ArmLength, 0.f) * FMath::Tan(HalfFov) / FMath::Max(ReferenceAspectRatio, 0.01f);
}

float ARTCameraPawn::SnapYawToStep(float YawDegrees, float Step)
{
	const float Passo = FMath::Max(Step, 1.f);
	return FRotator::ClampAxis(FMath::RoundToFloat(FRotator::ClampAxis(YawDegrees) / Passo) * Passo);
}

bool ARTCameraPawn::IsOrthographicNow() const
{
	return Camera != nullptr && Camera->ProjectionMode == ECameraProjectionMode::Orthographic;
}

void ARTCameraPawn::ApplyStrategicProjection()
{
	if (!Camera || !SpringArm)
	{
		return;
	}
	// D-498: la proiezione segue lo STATO, con la sua isteresi — anche fra le due soglie resta quella dello stato.
	if (bStrategicView)
	{
		// 🔑 **Lo zoom resta il braccio** (D-252): la larghezza si DERIVA dalla distanza, cosi' lo stato strategico
		// continua a leggersi da `TargetArmLength` e al cambio di proiezione la scala sul pivot non salta.
		Camera->SetOrthoWidth(OrthoWidthForArm(SpringArm->TargetArmLength, Camera->FieldOfView, Camera->AspectRatio));
		// I piani automatici di UE restano (servono alla profondita' a 16 bit), ma spostati indietro: vedi l'header.
		Camera->SetAutoPlaneShift(-StrategicOrthoPlaneShiftArms * SpringArm->TargetArmLength);
		if (Camera->ProjectionMode != ECameraProjectionMode::Orthographic)
		{
			Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
		}
	}
	else if (Camera->ProjectionMode != ECameraProjectionMode::Perspective)
	{
		Camera->SetProjectionMode(ECameraProjectionMode::Perspective);
	}
}

void ARTCameraPawn::GetOrderedStrategicThresholds(float& OutEnter, float& OutExit) const
{
	// 🔑 **`Exit < Enter` e' imposto QUI, non solo documentato.** I due campi sono `BlueprintReadWrite` e il
	// loro `meta = (ClampMin)` vincola il Details e non un `Set` da Blueprint: con le soglie invertite una
	// disuguaglianza scritta a mano avrebbe prodotto uno stato che entra e non esce, o che sfarfalla — lo
	// stesso genere di difetto che l'isteresi esiste per evitare. Con `Max`/`Min` la coppia e' ordinata per
	// costruzione, e chi le inverte ottiene comunque un'isteresi valida.
	OutEnter = FMath::Max(StrategicEnterThreshold, StrategicExitThreshold);
	OutExit  = FMath::Min(StrategicEnterThreshold, StrategicExitThreshold);
}

void ARTCameraPawn::ToggleStrategicView()
{
	if (!SpringArm)
	{
		return;
	}

	float Enter = 0.f;
	float Exit = 0.f;
	GetOrderedStrategicThresholds(Enter, Exit);

	const bool bWas = bStrategicView;
	if (bStrategicView)
	{
		// Mai letta (un pawn nato gia' oltre la soglia): la soglia d'uscita e' la distanza tattica piu' vicina
		// a quella strategica, cioe' il ritorno che sposta meno la vista.
		const float Back = (LastTacticalArmLength > 0.f) ? FMath::Min(LastTacticalArmLength, Exit) : Exit;
		ApplyArmLength(Back);
	}
	else
	{
		// 🔴 **#3646 — la distanza di partenza si registra QUI, non si eredita.** `LastTacticalArmLength` la scrive
		// `UpdateStrategicState`, ma non tutti gli scrittori del braccio passano di li': `FrameOwnTeam` apre la
		// partita a `MatchStartArmLength` scrivendo `TargetArmLength` direttamente. Senza questa riga il primo `Tab`
		// della partita tornava alla distanza di `BeginPlay` (800) invece che a quella d'avvio (450) — trovato in
		// PIE (seduta `U73`), invisibile ai test che partivano da `AddZoom`.
		LastTacticalArmLength = SpringArm->TargetArmLength;
		ApplyArmLength(Enter);
	}

	// ⚠️ Con `StrategicEnterThreshold` oltre `MaxArmLength` il braccio si ferma al limite e lo stato non cambia:
	// e' una taratura incoerente, e il log lo dice invece di lasciare un tasto che sembra rotto.
	if (bWas == bStrategicView)
	{
		UE_LOG(LogRT, Warning,
			TEXT("[RT] Tab non ha cambiato la vista: arm=%.0f, enter=%.0f, exit=%.0f, limiti [%.0f, %.0f]"),
			SpringArm->TargetArmLength, Enter, Exit, MinArmLength, MaxArmLength);
	}
}

void ARTCameraPawn::ApplyArmLength(float InArmLength)
{
	if (!SpringArm)
	{
		return;
	}
	SpringArm->TargetArmLength = FMath::Clamp(InArmLength, MinArmLength, MaxArmLength);

	// Gli stessi riallineamenti di `AddZoom`, e nello stesso ordine: lo stato prima del pitch, perche' da D-498 il
	// pitch ne dipende; il pitch prima del pivot, perche' entra nei limiti. Una scorciatoia qui farebbe divergere
	// le porte dello zoom.
	UpdateStrategicState();
	RecomputePitchFromZoom();
	ApplyArmRotation();
	RefreshViewportMetrics();
	SetCameraPivot(CameraPivot);
}

// --- #1778 · i limiti del pivot che conoscono il viewport (D-251) -----------------------------------

FBox2D ARTCameraPawn::ComputeEffectivePivotBounds(const FBox2D& AllowedArea, float ArmLength,
	float PitchDegrees, float YawDegrees, float HorizontalFovDegrees, float AspectRatio,
	float AllowedOutsideFraction, float ReferenceAspectRatio)
{
	// Aspect ratio ignoto (headless, o viewport non ancora creato): non c'e' una geometria di schermo su cui
	// ragionare, e l'area passa intatta. E' il **caso degenere dichiarato** — il clamp per sole celle di
	// `#864` resta il comportamento, e non e' un ripiego silenzioso.
	if (!AllowedArea.bIsValid || AspectRatio <= 0.f || ArmLength <= 0.f || HorizontalFovDegrees <= 0.f)
	{
		return AllowedArea;
	}

	// 🔴 **Il semiasse FISSO e' il verticale, non l'orizzontale** (#3661, D-497 (4)). Con `MaintainYFOV`, default
	// di `BaseEngine.ini` che il progetto non cambia, `FMinimalViewInfo::CalculateProjectionMatrixGivenViewRectangle`
	// prende il FOV come orizzontale all'aspetto della camera (16:9), ne ricava il semiangolo verticale e lo
	// tiene; l'orizzontale segue il rettangolo. Il modello di prima teneva fisso l'orizzontale e divideva per
	// l'aspetto: esatto solo a 16:9, e da #3661 la scena non e' piu' 16:9 (circa 1,69 a 1920×1080).
	const float HalfFovH = FMath::DegreesToRadians(FMath::Clamp(HorizontalFovDegrees, 1.f, 179.f) * 0.5f);
	const double AspettoRiferimento = FMath::Max(static_cast<double>(ReferenceAspectRatio), 0.01);
	const double VerticalHalf = static_cast<double>(ArmLength) * FMath::Tan(HalfFovH) / AspettoRiferimento;
	const double HalfWidth = VerticalHalf * FMath::Max(static_cast<double>(AspectRatio), 0.01);

	// L'estensione **lungo la direzione di sguardo** cresce quando la camera si abbassa: a picco il quadro
	// e' un rettangolo, all'orizzonte si allunga senza limite. E' il caso che rompe il clamp fisso, ed e'
	// per questo che il pitch entra nella formula.
	//
	// `sin` del pitch al denominatore, con un pavimento: a `MaxPitch = 0` la camera guarda l'orizzonte e la
	// proiezione a terra divergerebbe. Il pavimento non e' una tolleranza numerica — e' la dichiarazione
	// che oltre una certa inclinazione la nozione di «quanto mondo entra» smette di essere finita.
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
	// #3661, D-497 (3): l'aspetto e' quello del RETTANGOLO del giocatore, non del viewport. Da #3661 la scena
	// si rende fra le fasce dell'HUD (`URTGameViewportClient`), e a 1920×1080 il rettangolo e' circa 1,69,
	// non 1,78. `Size` e' la frazione che `LayoutPlayers` scrive a ogni fotogramma; senza un giocatore vale
	// l'intero viewport, come prima.
	const FIntPoint Size = Client->Viewport->GetSizeXY();
	FVector2D Frazione(1.0, 1.0);
	if (const ULocalPlayer* Giocatore = World->GetFirstLocalPlayerFromController())
	{
		if (Giocatore->Size.X > 0.0 && Giocatore->Size.Y > 0.0)
		{
			Frazione = Giocatore->Size;
		}
	}
	const double Larghezza = static_cast<double>(Size.X) * Frazione.X;
	const double Altezza = static_cast<double>(Size.Y) * Frazione.Y;
	ViewportAspectRatio = (Altezza > 0.0) ? static_cast<float>(Larghezza / Altezza) : 0.f;
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
		ViewportHorizontalFov, ViewportAspectRatio, AllowedOutsideFraction,
		Camera ? Camera->AspectRatio : 16.f / 9.f); // #3661: l'aspetto a cui il FOV e' orizzontale

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
	UpdateStrategicState(); // prima del pitch: da D-498 il pitch dipende dallo stato

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
	// Come in `AddZoom`: il pitch derivato entra nei limiti, quindi si aggiorna prima della scrittura.
	RecomputePitchFromZoom();
	ApplyArmRotation();
	RefreshViewportMetrics();
	SetCameraPivot(FVector(Moved.X, Moved.Y, Pivot.Z));
}

void ARTCameraPawn::AddYawContinuous(float AxisValue)
{
	// D-499: in iso si ruota solo a scatti di 60 (`AddYaw`); la rotazione libera lascerebbe la vista fuori dagli assi.
	if (FMath::IsNearlyZero(AxisValue) || bStrategicView)
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
	// D-498: in strategica l'inclinazione e' fissa e l'orbita verticale non agisce. Si esce PRIMA di toccare
	// l'offset, che e' della tattica e deve ritrovarsi intatto al ritorno.
	if (FMath::IsNearlyZero(AxisValue) || bStrategicView)
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
	// 🔴 **L'input scrive l'OFFSET, non l'inclinazione.** Da #1834 il pitch ha due contributi —
	// quello derivato dallo zoom e quello dell'utente — e sommare direttamente su `CameraPitch` li
	// renderebbe indistinguibili: il primo ricalcolo dopo uno scroll cancellerebbe l'orbita fatta a mano.
	ManualPitchOffset += AxisValue * FMath::Max(PitchSensitivity, 0.f);
	RecomputePitchFromZoom();
	ApplyArmRotation();
}

void ARTCameraPawn::AddOrbit(float DeltaYaw, float DeltaPitch)
{
	// I due campi si scrivono qui e la trasformata **una volta sola**: chiamare i due `Add*` in fila
	// aggiornerebbe il braccio due volte per frame di trascinamento, e la seconda scrittura scarterebbe
	// il risultato della prima.
	// D-499: in iso nemmeno l'orbita orizzontale agisce: si ruota solo a scatti di 60 (`AddYaw`).
	if (!FMath::IsNearlyZero(DeltaYaw) && !bStrategicView)
	{
		CameraYaw = FRotator::ClampAxis(CameraYaw + DeltaYaw * FMath::Max(YawSensitivity, 0.f));
	}
	// D-498: in strategica nemmeno l'orbita verticale agisce, come in `AddPitch` (l'orizzontale e' gia' esclusa sopra, D-499).
	if (!FMath::IsNearlyZero(DeltaPitch) && !bStrategicView)
	{
		// 🔴 **#3630 — l'orbita scrive l'OFFSET, come `AddPitch`.** Fino al 2026-10-10 sommava direttamente su
		// `CameraPitch`, e il primo zoom la cancellava: `RecomputePitchFromZoom` riscrive il pitch come
		// derivato + offset, e un'orbita che non stava nell'offset non c'era. Il clamp lo fa il ricalcolo, sull'offset
		// prima della somma, cioe' contro il windup.
		const float Signed = bInvertOrbitPitch ? -DeltaPitch : DeltaPitch;
		ManualPitchOffset += Signed * FMath::Max(PitchSensitivity, 0.f);
		RecomputePitchFromZoom();
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
	UpdateStrategicState(); // prima del pitch: da D-498 il pitch dipende dallo stato
	// ⚠️ **Prima di scrivere il pivot**: l'inclinazione e' un ingresso di `ComputeEffectivePivotBounds`
	// (`D-251`), quindi derivarla dopo il clamp userebbe per i limiti il pitch della distanza precedente.
	// E' lo stesso ordine che `RecenterView` documenta per yaw e braccio.
	RecomputePitchFromZoom();
	ApplyArmRotation();
	RefreshViewportMetrics();

	// 🔴 **Il pivot va RI-CLAMPATO, perche' i suoi limiti dipendono dalla distanza** (code review,
	// 2026-08-30). Da `D-251` l'area legale si stringe quando ci si allontana: allontanandosi stando al
	// bordo, il pivot restava dov'era — fuori dai limiti nuovi — e nessuno lo riportava dentro fino al pan
	// successivo. `ZoomTowards` non aveva il difetto perche' scrive il pivot per conto suo; questo ramo
	// toccava solo il braccio.
	SetCameraPivot(CameraPivot);
}

float ARTCameraPawn::GetZoomAlpha() const
{
	if (!SpringArm)
	{
		return 0.f;
	}
	// Il denominatore si misura sui limiti REALI, non su `DefaultArmLength`: normalizzare sul default
	// darebbe un valore che vale `1` a meta' corsa e supera `1` oltre, cioe' non una posizione ma un
	// rapporto — ed e' esattamente la forma che questa funzione sostituisce.
	const float Range = MaxArmLength - MinArmLength;
	if (Range <= KINDA_SMALL_NUMBER)
	{
		// Limiti coincidenti: non c'e' corsa, e ogni distanza e' il fondo. Restituire `0` invece di
		// dividere per zero tiene la funzione totale: un pawn tarato cosi' e' degenere, non invalido.
		return 0.f;
	}
	return FMath::Clamp((SpringArm->TargetArmLength - MinArmLength) / Range, 0.f, 1.f);
}

void ARTCameraPawn::SetZoomAlpha(float InAlpha)
{
	if (!SpringArm)
	{
		return;
	}
	const float Clamped = FMath::Clamp(InAlpha, 0.f, 1.f);
	ApplyArmLength(FMath::Lerp(MinArmLength, MaxArmLength, Clamped));
}

float ARTCameraPawn::GetPanDistanceScale() const
{
	if (!SpringArm || DefaultArmLength <= 0.f)
	{
		return 1.f;
	}
	// ⚠️ **Identica al rapporto che sostituisce**, e deliberatamente: #1834 sposta la derivazione dentro
	// una porta sola, non cambia il feel dello scorrimento. La distanza si ricostruisce dall'alpha invece
	// di leggere il braccio, cosi' il giorno in cui la scala diventera' una curva tarabile ci sara' un
	// solo punto da cambiare, ed e' questo.
	return FMath::Lerp(MinArmLength, MaxArmLength, GetZoomAlpha()) / DefaultArmLength;
}

void ARTCameraPawn::RecomputePitchFromZoom()
{
	if (bStrategicView)
	{
		// D-498: l'inclinazione e' fissa per tutto lo stato strategico. L'offset manuale non entra e non si clampa
		// contro un derivato che qui non vale: e' della tattica, e al ritorno deve essere quello di prima.
		CameraPitch = FMath::Clamp(StrategicPitch, MinPitch, MaxPitch);
		return;
	}

	const float Derived = FMath::Clamp(DefaultPitch + PitchZoomDelta * GetZoomAlpha(), MinPitch, MaxPitch);

	// 🔴 **L'offset si clampa PRIMA di sommarlo, contro il windup.** Clampare solo il risultato
	// lascerebbe crescere `ManualPitchOffset` oltre il limite: la vista resterebbe ferma a fondo corsa e
	// il primo trascinamento nel verso opposto non risponderebbe, perche' starebbe smaltendo un offset
	// invisibile. E' la stessa classe di difetto che `AddPitch` gia' evitava clampando lo stato.
	ManualPitchOffset = FMath::Clamp(ManualPitchOffset, MinPitch - Derived, MaxPitch - Derived);
	CameraPitch = FMath::Clamp(Derived + ManualPitchOffset, MinPitch, MaxPitch);
}

void ARTCameraPawn::ResetToDefaultView()
{
	// 🔑 **Orientamento e distanza si azzerano PRIMA di scrivere il pivot** (code review, 2026-08-30).
	// `SetCameraPivot` clampa usando yaw, pitch e braccio **correnti**: scrivendo il pivot per primo, il
	// clamp descriveva la camera che stava per sparire invece di quella che sarebbe esistita un istante
	// dopo. Premendo `Home` da zoom massimo su una mappa piccola l'inquadratura superava l'area, il pivot
	// si inchiodava al centro calcolato con i valori vecchi, e nessuno lo ricalcolava dopo il reset.
	CameraYaw = 0.f;
	// D-499: se `Home` arriva dalla strategica, l'uscita riprende l'imbardata tattica salvata: si azzera anche quella,
	// altrimenti `Home` tornerebbe orientato come prima di `Tab`.
	TacticalYawBeforeStrategic = 0.f;
	CameraPitch = FMath::Clamp(DefaultPitch, MinPitch, MaxPitch);
	ApplyViewSettings();
}

bool ARTCameraPawn::IsOwnLiveUnit(const ARTUnit* Unit, int32 TeamId)
{
	return Unit && Unit->TeamId == TeamId && Unit->IsAlive();
}

ERTHomeFraming ARTCameraPawn::FrameHome(const ARTUnit* Selection, bool bObservationOnly)
{
	// La squadra e' quella di `FrameOwnTeam`: una regola sola per «la propria squadra», anche per la guardia
	// sulla selezione.
	//
	// ⛔ **Ma senza `ARTPlayerState` la squadra non si indovina.** `TeamIdOf` ripiega sulla 0 quando lo stato
	// manca — una comodita' da demo che `FrameOwnTeam` usa all'avvio —, e per `Home` quel ripiego vorrebbe dire
	// inquadrare la squadra 0 per un giocatore della 1 il cui stato non e' ancora arrivato: il canale di [D-143]
	// aperto da un tasto. Senza stato, `Home` va sul centro mappa.
	const APlayerController* PC = Cast<APlayerController>(GetController());
	const bool bTeamKnown = PC && PC->GetPlayerState<ARTPlayerState>() != nullptr;
	const int32 TeamId = ARTPlayerState::TeamIdOf(PC);
	const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(GetWorld());

	// Si sceglie il ramo sulle CELLE, senza ancora una posizione nel mondo: la quota dei piani si legge dopo il
	// reset della vista (`ResetAndFrame`). Basta l'actor della mappa, come per `FrameOwnTeam`: su un graybox
	// senza asset la squadra si inquadra lo stesso.
	//
	// ⛔ La guardia sulla selezione sta qui, dove si scrive il pivot: un nemico passato come selezione cade
	// sull'anello successivo invece di essere inquadrato. Morta, la selezione non e' un'inquadratura utile — si
	// passa alla squadra, che e' cio' che il giocatore controlla ancora.
	//
	// In osservazione — autobattle e scenari, dove il giocatore non comanda una squadra — `Home` resta il campo
	// intero: e' la regola che la issue dava al `Replay`, e quella che `ARTGameMode::RecenterCameraOnScenario`
	// sceglie all'avvio di uno scenario.
	TArray<FRTCellId> Cells;
	ERTHomeFraming Branch = ERTHomeFraming::MapCenter;
	if (HexMap && bTeamKnown && !bObservationOnly)
	{
		if (IsOwnLiveUnit(Selection, TeamId))
		{
			Cells.Add(Selection->Cell);
			Branch = ERTHomeFraming::Selection;
		}
		else if (CollectOwnTeamCells(TeamId, Cells))
		{
			Branch = ERTHomeFraming::OwnTeam;
		}
	}

	ResetAndFrame(HexMap, Cells);

	// Il log dice QUALE anello ha scelto: dall'esito i tre non si distinguono sempre (vedi `ERTHomeFraming`).
	const TCHAR* BranchName = Branch == ERTHomeFraming::Selection ? TEXT("selezione")
		: Branch == ERTHomeFraming::OwnTeam ? TEXT("squadra")
		: bObservationOnly ? TEXT("centro mappa (osservazione)")
		: bTeamKnown ? TEXT("centro mappa") : TEXT("centro mappa (squadra non nota)");
	UE_LOG(LogRT, Log, TEXT("[RT] Home: %s (squadra %d, %d celle, arm=%.0f)"),
		BranchName, TeamId, Cells.Num(), SpringArm ? SpringArm->TargetArmLength : -1.f);
	return Branch;
}

void ARTCameraPawn::ResetAndFrame(const ARTHexMapActor* HexMap, const TArray<FRTCellId>& Cells)
{
	// Orientamento e distanza prima, il pivot dopo: vedi `ResetToDefaultView`.
	ResetToDefaultView();

	if (HexMap)
	{
		// 🔴 **La quota dei piani e' quella che la board avra' DOPO `Home`, non quella che ha adesso.** In
		// strategica la board e' posata con i piani separati (#3632), e torna alla quota tattica solo al
		// prossimo `ARTHUD::UpdateObserverVeil`. Se il reset ha fatto uscire dalla strategica, la quota giusta
		// e' quella d'authoring: con la posata il pivot finirebbe un piano sopra l'unita', il difetto che #887
		// ha chiuso per `F`. Se si resta in strategica, la posata e' quella giusta.
		FVector Origin; float HexSize; float LayerHeight;
		const URTHexMapAsset* Map = bStrategicView
			? HexMap->GetHexContext(Origin, HexSize, LayerHeight)
			: HexMap->GetAuthoringHexContext(Origin, HexSize, LayerHeight);
		if (Cells.Num() > 0)
		{
			// La CELLA e non l'attore, come in `FocusCameraOnUnit`: il pivot sta sul piano, non mezzo corpo sopra.
			SetCameraPivot(URTHexLibrary::CellsCentroidWorld(Cells, Origin, HexSize, LayerHeight));
		}
		else if (Map)
		{
			SetCameraPivot(URTHexLibrary::AxialToWorld(Map->GetCenterCell(), Origin, HexSize, LayerHeight));
		}
		else
		{
			SetCameraPivot(Origin);
		}
	}

	// `Home` e' «riportami a un'inquadratura che conosco», quindi azzera anche il peek: tornare con un offset
	// residuo lascerebbe la vista storta senza che nulla lo dica.
	PeekOffset = FVector::ZeroVector;
	ApplyPivot();
}

void ARTCameraPawn::RecenterView()
{
	// Centra sulla mappa ESAGONALE del livello (se presente) e ripristina lo zoom di default. La mappa non e'
	// per forza centrata sull'origine: il centro viene dal bounding box delle sue celle. E' l'ultimo anello di
	// `FrameHome`, e l'inquadratura con cui `ARTGameMode` apre uno scenario.
	ResetAndFrame(ARTHexMapActor::FindInWorld(GetWorld()), {});

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
