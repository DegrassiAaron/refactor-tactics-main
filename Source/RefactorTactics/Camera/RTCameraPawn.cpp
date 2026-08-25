#include "Camera/RTCameraPawn.h"
#include "RefactorTactics.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Map/RTHexLibrary.h"

/** Definita in `ScenarioHarness/RTTestConsole.cpp`: vista di misura a picco (`#1290`). */
extern TAutoConsoleVariable<int32> CVarRTCameraTopDown;
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
	SetActorLocation(WorldLocation);
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

	UE_LOG(LogRT, Warning, TEXT("[RT] rt.Camera.TopDown: vista di MISURA a picco sull'intera board "
		"(arm=%.0f, pitch=%.0f). Non e' la vista di gioco."),
		SpringArm->TargetArmLength, CameraPitch);
	return true;
}

void ARTCameraPawn::BeginPlay()
{
	Super::BeginPlay();
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
	SetActorLocation(ClampToSoftBounds(GetActorLocation() + Delta));
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

	return FVector(
		FMath::Clamp(Desired.X, Min.X - Margin, Max.X + Margin),
		FMath::Clamp(Desired.Y, Min.Y - Margin, Max.Y + Margin),
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
	const FVector Pivot = GetActorLocation();
	const float Ratio = After / Before;
	const FVector Moved = WorldAnchor + (Pivot - WorldAnchor) * Ratio;

	// 🔴 **Anche lo zoom passa dai limiti.** La prima stesura scriveva il pivot diretto, e allontanandosi
	// il fattore `Ratio > 1` lo spingeva via **cumulativamente**: da braccio 100 a 4000 sono quaranta
	// volte l'offset iniziale, cioe' centinaia di celle fuori mappa. Riapriva esattamente il buco che
	// questa issue chiude — «ci si puo' allontanare fino a perdere la mappa» — dal comando che avrebbe
	// dovuto risolverlo. Trovato in code review.
	SetActorLocation(ClampToSoftBounds(FVector(Moved.X, Moved.Y, Pivot.Z)));
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
}

void ARTCameraPawn::RecenterView()
{
	// Centra sulla mappa ESAGONALE del livello (se presente) e ripristina lo zoom di default. La mappa non e'
	// per forza centrata sull'origine: il centro viene dal bounding box delle sue celle.
	if (const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(GetWorld()))
	{
		FVector Origin; float HexSize; float LayerHeight;
		if (const URTHexMapAsset* Map = HexMap->GetHexContext(Origin, HexSize, LayerHeight))
		{
			SetActorLocation(URTHexLibrary::AxialToWorld(Map->GetCenterCell(), Origin, HexSize, LayerHeight));
		}
		else
		{
			SetActorLocation(Origin);
		}
	}
	// Anche la ROTAZIONE torna a zero. `Home` e' il tasto del «riportami a un'inquadratura che conosco»: se
	// riportasse la posizione ma lasciasse la vista girata, chi si e' perso ruotando resterebbe perso.
	CameraYaw = 0.f;

	// E l'INCLINAZIONE torna alla taratura, per la stessa ragione: da #863 il pitch e' regolabile a
	// runtime, quindi «un'inquadratura che conosco» include anche quanto la vista e' inclinata. Prima
	// bastava non toccarlo — era `CameraPitch` a fare da default *e* da stato, e nessun input lo muoveva.
	CameraPitch = FMath::Clamp(DefaultPitch, MinPitch, MaxPitch);

	// Distanza e orientamento di default li applica `ApplyViewSettings`, che e' esattamente cio' che
	// serve qui: `Home` e' «riportami all'inquadratura di partenza». Il difetto che #873 chiude era che
	// questa funzione scriveva `DefaultArmLength` **nudo**, mentre gli altri punti lo clampavano — con
	// `DefaultArmLength` fuori scala, `Home` portava il braccio dove lo zoom non lo lascia stare.
	//
	// ⚠️ La prima stesura della correzione **copiava** le due righe di `ApplyViewSettings` invece di
	// chiamarla, clamp compreso. Il fix funzionava e il test era verde, ma lasciava la stessa espressione
	// in due punti da tenere allineati a mano — cioe' ricreava la divergenza che #873 esiste per chiudere,
	// un livello piu' in la'. Trovato in code review.
	ApplyViewSettings();
}
