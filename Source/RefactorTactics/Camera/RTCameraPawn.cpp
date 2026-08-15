#include "Camera/RTCameraPawn.h"
#include "RefactorTactics.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Map/RTHexLibrary.h"
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
	FocusOn(URTHexLibrary::CellsCentroidWorld(Cells, Origin, HexSize, LayerHeight));
	if (SpringArm)
	{
		SpringArm->TargetArmLength = FMath::Clamp(MatchStartArmLength, MinArmLength, MaxArmLength);
	}
	UE_LOG(LogRT, Log, TEXT("[RT] Camera sulla squadra %d (%d unita', arm=%.0f)"),
		TeamId, Cells.Num(), SpringArm ? SpringArm->TargetArmLength : -1.f);
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
	const FVector Delta   = (Forward * Axis.Y + Right * Axis.X) * PanSpeed;
	AddActorWorldOffset(Delta);
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

void ARTCameraPawn::AddYawContinuous(float AxisValue)
{
	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}
	// Stessa normalizzazione dello scatto: il valore resta leggibile in editor invece di crescere senza
	// fine. E stesso stato — `CameraYaw` — quindi trascinare e poi premere `Q` non sono due sistemi che si
	// contendono la vista: il tasto riparte da dove il trascinamento ha lasciato.
	CameraYaw = FRotator::ClampAxis(CameraYaw + AxisValue * YawSensitivity);
	if (SpringArm)
	{
		SpringArm->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.f));
	}
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
	CameraPitch = FMath::Clamp(CameraPitch + AxisValue * PitchSensitivity, MinPitch, MaxPitch);
	if (SpringArm)
	{
		SpringArm->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.f));
	}
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
