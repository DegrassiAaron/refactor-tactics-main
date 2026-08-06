#include "RTGameMode.h"
#include "Camera/RTCameraPawn.h"
#include "Player/RTPlayerController.h"
#include "UI/RTHUD.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Unit/RTUnit.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "RefactorTactics.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DirectionalLight.h"
#include "Components/LightComponent.h"

ARTGameMode::ARTGameMode()
{
	DefaultPawnClass = ARTCameraPawn::StaticClass();
	PlayerControllerClass = ARTPlayerController::StaticClass();
	HUDClass = ARTHUD::StaticClass();
}

void ARTGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Mappa esagonale: usa quella presente nel livello o ne crea una all'origine (graybox demo).
	ARTHexMapActor* HexMap = Cast<ARTHexMapActor>(
		UGameplayStatics::GetActorOfClass(this, ARTHexMapActor::StaticClass()));
	if (!HexMap)
	{
		HexMap = World->SpawnActor<ARTHexMapActor>(ARTHexMapActor::StaticClass(), FTransform::Identity);
	}

	// Luce direzionale (se assente) per rendere visibile la scena anche in un livello vuoto.
	if (!UGameplayStatics::GetActorOfClass(this, ADirectionalLight::StaticClass()))
	{
		if (ADirectionalLight* Light = World->SpawnActor<ADirectionalLight>(
				ADirectionalLight::StaticClass(), FTransform(FRotator(-50.f, -30.f, 0.f))))
		{
			if (ULightComponent* LightComp = Light->GetLightComponent())
			{
				LightComp->SetMobility(EComponentMobility::Movable);
				LightComp->SetIntensity(6.f);
			}
		}
	}

	// Orchestratore del turno: spawnato PRIMA delle unita' cosi' esiste gia' quando queste fanno BeginPlay
	// (i BP_Unit possono agganciarsi ai suoi delegate di playback senza attese). Sicuro: il TurnManager non
	// tocca le unita' al proprio BeginPlay (fa solo StartPlanningTimer); le raccoglie a ogni turno.
	if (!UGameplayStatics::GetActorOfClass(this, ARTTurnManager::StaticClass()))
	{
		World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass(), FTransform::Identity);
	}

	SetupHexMatch(HexMap);
}

void ARTGameMode::ApplyMapSource(ARTHexMapActor* HexMap)
{
	if (!HexMap)
	{
		return;
	}

	switch (MapSource)
	{
	case ERTMapSource::GeneratedTestArena:
		// Scelta esplicita: prevale anche su una mappa d'autore presente nel livello. Va dichiarato, non subito.
		HexMap->MapAsset = URTMatchSetupLibrary::MakeTestArena(HexMap);
		HexMap->RebuildInstances();
		UE_LOG(LogRT, Warning, TEXT("[RT] MapSource=GeneratedTestArena: uso la mappa di PROVA generata "
			"(%d celle, con ostacoli, muri, terreno costoso e piattaforma). La mappa del livello e' ignorata."),
			HexMap->MapAsset ? HexMap->MapAsset->NumCells() : 0);
		return;

	case ERTMapSource::GeneratedDemoArena:
		HexMap->MapAsset = URTMatchSetupLibrary::MakeDemoArena(HexMap, DemoArenaRadius);
		HexMap->RebuildInstances();
		UE_LOG(LogRT, Warning, TEXT("[RT] MapSource=GeneratedDemoArena: uso l'arena di ripiego "
			"(esagono r=%d, %d celle). La mappa del livello e' ignorata."),
			DemoArenaRadius, HexMap->MapAsset ? HexMap->MapAsset->NumCells() : 0);
		return;

	case ERTMapSource::LevelAsset:
	default:
		break;
	}

	// Mappa del livello. Quello che conta non e' avere un asset, ma avere delle CELLE: un asset assegnato ma
	// VUOTO non allestisce nulla e premere Play mostra una schermata nera senza spiegazione (osservato in PIE su
	// L_DevSandbox, il cui asset si e' ritrovato a 0 celle). Senza una mappa d'autore utilizzabile si ripiega
	// sull'arena demo: meglio un fondo di scena giocabile che il vuoto.
	// Attenzione: qui si tratta solo il caso "nessuna cella". Una mappa d'autore con POCHE celle non viene
	// rimpiazzata: e' un errore dell'autore e glielo si dice, invece di nascondergli la mappa sotto i piedi.
	if ((!HexMap->MapAsset || HexMap->MapAsset->NumCells() == 0) && DemoArenaRadius > 0)
	{
		HexMap->MapAsset = URTMatchSetupLibrary::MakeDemoArena(HexMap, DemoArenaRadius);
		HexMap->RebuildInstances();
		UE_LOG(LogRT, Warning,
			TEXT("[RT] Mappa esagonale del livello assente o senza celle: uso un'arena di ripiego "
				 "(esagono r=%d, %d celle). Posa un ARTHexMapActor con un MapAsset popolato per giocare su una "
				 "mappa d'autore."),
			DemoArenaRadius, HexMap->MapAsset ? HexMap->MapAsset->NumCells() : 0);
	}
}

void ARTGameMode::SetupHexMatch(ARTHexMapActor* HexMap)
{
	if (!HexMap)
	{
		return;
	}

	ApplyMapSource(HexMap);

	// Il livello puo' avere unita' gia' posate a mano: in quel caso l'allestimento automatico non interviene.
	if (UGameplayStatics::GetActorOfClass(this, ARTUnit::StaticClass()))
	{
		return;
	}

	const URTHexMapAsset* Map = HexMap->MapAsset;
	const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Map, /*NumPerTeam=*/ 2, /*Layer=*/ 0);
	if (Start.Num() != 4)
	{
		UE_LOG(LogRT, Warning,
			TEXT("[RT] Mappa esagonale senza celle percorribili sufficienti: partita non allestita"));
		return;
	}

	// Contesto geometrico dall'unica fonte (scala dall'asset autorevole, origine dall'actor).
	FVector Origin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerHeight = 0.f;
	HexMap->GetHexContext(Origin, HexSize, LayerHeight);

	// Ogni squadra: un Ranger (fragile, lunga gittata) e un Guardian (tanky, corta gittata).
	SpawnUnit(0, Start[0], /*bGuardian=*/ false, Origin, HexSize, LayerHeight);
	SpawnUnit(0, Start[1], /*bGuardian=*/ true,  Origin, HexSize, LayerHeight);
	SpawnUnit(1, Start[2], /*bGuardian=*/ false, Origin, HexSize, LayerHeight);
	SpawnUnit(1, Start[3], /*bGuardian=*/ true,  Origin, HexSize, LayerHeight);
	UE_LOG(LogRT, Log, TEXT("[RT] Board 2v2 esagonale avviata su %d celle"), Map ? Map->NumCells() : 0);
}

ARTUnit* ARTGameMode::SpawnUnit(int32 TeamId, const FRTCellId& InCell, bool bGuardian, const FVector& Origin,
	float HexSize, float LayerHeight)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Classe per archetipo: se assegnata (BP_Unit con skeletal) usala, altrimenti fallback al cilindro C++.
	UClass* RangerCls = RangerUnitClass ? RangerUnitClass.Get() : ARTUnit::StaticClass();
	UClass* GuardianCls = GuardianUnitClass ? GuardianUnitClass.Get() : ARTUnit::StaticClass();
	UClass* UnitClass = bGuardian ? GuardianCls : RangerCls;

	// Deferred: imposto team e archetipo prima di BeginPlay, cosi' colore e statistiche sono corretti.
	ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(UnitClass, FTransform::Identity);
	if (Unit)
	{
		Unit->TeamId = TeamId;
		Unit->bIsBotControlled = (TeamId == 1); // team 1 giocato dal bot
		Unit->ConfigureAsArchetype(bGuardian ? ERTArchetype::Guardian : ERTArchetype::Ranger);
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		Unit->PlaceOnCell(InCell, Origin, HexSize, LayerHeight);
	}
	return Unit;
}
