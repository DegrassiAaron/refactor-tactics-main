#include "RTGameMode.h"
#include "Camera/RTCameraPawn.h"
#include "Player/RTPlayerController.h"
#include "UI/RTHUD.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestResult.h"
#include "ScenarioHarness/RTScenarioSession.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTTestReportWriter.h"

/** Definita in Test/RTTestConsole.cpp: scenario da eseguire all'avvio invece della partita normale. */
extern TAutoConsoleVariable<FString> CVarRTTestScenario;
#include "Turn/RTMatchFormatData.h"
#include "Turn/RTMatchFormatLibrary.h"
#include "RefactorTactics.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DirectionalLight.h"
#include "Components/LightComponent.h"

ARTGameMode::ARTGameMode()
{
	DefaultPawnClass = ARTCameraPawn::StaticClass();
	PlayerControllerClass = ARTPlayerController::StaticClass();
	HUDClass = ARTHUD::StaticClass();

	// Tick abilitabile ma SPENTO all'avvio: si accende solo se parte uno scenario. Una partita normale non ha
	// niente da far avanzare qui, e un GameMode che ticca a vuoto e' costo senza contropartita.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
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

	// AUTO-RUN di uno scenario di test: se `rt.Test.Scenario` e' impostata, la partita normale NON viene
	// allestita e al suo posto gira lo scenario. E' il workflow «premo Play e parte da solo» del documento
	// di specifica, senza Actor da trascinare in un livello e quindi senza toccare nessun `.umap`.
	//
	// Il GameMode e' il posto giusto per questa decisione: sceglie COSA allestire, che e' il suo mestiere.
	// Il resolver e il turn manager restano ignari dell'harness (nessun `if (IsTest)` nel gameplay).
	const FString TestScenario = ResolveScenarioToRun();
	if (!TestScenario.IsEmpty())
	{
		// La sessione parte QUI ma avanza in Tick, un passo per frame: e' cio' che rende lo scenario
		// osservabile. Risolvendo tutto dentro BeginPlay finiva prima del primo fotogramma, e quel che si
		// vedeva muoversi erano turni fantasma — misurato in PIE, non supposto.
		FString ScenarioError;
		FRTTestScenario Scenario;
		const FString ScenarioPath = URTScenarioIndex::ResolvePath(TestScenario, ScenarioError);
		if (ScenarioPath.IsEmpty() || !URTScenarioLoader::LoadFromFile(ScenarioPath, Scenario, ScenarioError))
		{
			UE_LOG(LogRT, Error, TEXT("[RT-Test] scenario '%s' non caricabile: %s"), *TestScenario, *ScenarioError);
			return;
		}

		// La FONTE va dichiarata sempre, non solo quando c'e' conflitto: chi legge il log deve poter dire
		// «sta girando quello che ho scelto io» senza dedurlo dal comportamento a schermo.
		const TCHAR* Source = CVarRTTestScenario.GetValueOnGameThread().IsEmpty()
			? TEXT("proprieta' del GameMode")
			: TEXT("console rt.Test.Scenario");
		UE_LOG(LogRT, Warning, TEXT("[RT-Test] AUTO-RUN %s (da: %s): %d turni, pausa %.1fs — avanza un passo per frame"),
			*TestScenario, Source, Scenario.Turns.Num(), ScenarioTurnPauseSeconds);

		ScenarioSession = MakeShared<FRTScenarioSession>();
		ScenarioSession->TurnPauseSeconds = ScenarioTurnPauseSeconds;
		if (!ScenarioSession->Start(World, Scenario))
		{
			UE_LOG(LogRT, Error, TEXT("[RT-Test] %s -> ERROR: %s"),
				*TestScenario, *ScenarioSession->GetResult().ErrorMessage);
		}
		SetActorTickEnabled(true);

		// INQUADRATURA. Il percorso dello scenario non passa da `SetupHexMatch`, dove la partita normale si
		// preoccupa di cio' che si vede: senza questo, la camera restava dove l'aveva lasciata il proprio
		// BeginPlay — troppo alta e fuori centro. E' presentazione, non simulazione: non tocca l'esito.
		//
		// Al tick successivo, non subito: la camera potrebbe non essere ancora nata (l'ordine di BeginPlay fra
		// actor non e' garantito, ed e' la stessa ragione per cui `ARTCameraPawn` gia' riprova una volta).
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (ARTCameraPawn* Cam = Cast<ARTCameraPawn>(
					UGameplayStatics::GetActorOfClass(this, ARTCameraPawn::StaticClass())))
			{
				// Lo scenario non ha una «propria squadra» da inquadrare: le unita' sono di entrambe, e quel che
				// interessa e' vedere il campo INTERO, con tutti i movimenti dentro. `RecenterView` centra sulla
				// mappa e riporta lo zoom d'insieme, che e' esattamente l'inquadratura giusta per guardare.
				Cam->RecenterView();
				UE_LOG(LogRT, Log, TEXT("[RT-Test] Camera centrata sulla mappa dello scenario."));
			}
		}));
		return;
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
	// COPIA di lavoro della mappa d'autore (CP 8.4): dal terreno dinamico in poi la partita **modifica** le
	// celle (fuoco che si accende e si spegne, acqua che arriva), e modificare l'asset su disco sporcherebbe
	// il contenuto del progetto — in PIE le modifiche resterebbero dopo lo Stop, e due partite di fila non
	// partirebbero dallo stesso campo, cioe' addio determinismo.
	//
	// Le due arene generate non hanno questo problema: `MakeTestArena`/`MakeDemoArena` costruiscono gia' un
	// oggetto nuovo a ogni partita. Qui si allinea il terzo caso agli altri due, invece di aggiungere un
	// secondo modello ("a volte la mappa si puo' modificare, a volte no") che qualcuno prima o poi sbaglierebbe.
	if (HexMap->MapAsset && HexMap->MapAsset->NumCells() > 0)
	{
		HexMap->MapAsset = DuplicateObject<URTHexMapAsset>(HexMap->MapAsset, HexMap);
	}

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

bool ARTGameMode::ApplyMatchFormat(ARTTurnManager* TurnManager)
{
	FRTMatchRules Rules;
	FString Reason;

	if (MatchFormat)
	{
		if (!URTMatchFormatLibrary::ResolveRules(MatchFormat, Rules, Reason))
		{
			// Contenuto sbagliato: si rifiuta, non si ripiega. Un formato invalido silenziosamente sostituito
			// dal ripiego farebbe girare la partita con regole diverse da quelle che il designer ha scritto.
			UE_LOG(LogRT, Error,
				TEXT("[RT] Formato di partita '%s' NON valido: %s. Partita non allestita: correggi l'asset "
					 "oppure lascia MatchFormat vuoto per giocare con il formato di ripiego."),
				*GetNameSafe(MatchFormat), *Reason);
			return false;
		}
	}
	else
	{
		Rules = URTMatchFormatLibrary::MakeFallbackRules();
		UE_LOG(LogRT, Warning,
			TEXT("[RT] Nessun MatchFormat assegnato: uso il formato di RIPIEGO '%s' (RoundLimit %d, "
				 "soglia obiettivo %d). Assegna un URTMatchFormatData al GameMode per giocare un formato "
				 "dichiarato: le misure di playtest vanno attribuite al formato giusto."),
			*Rules.FormatId.ToString(), Rules.RoundLimit, Rules.ScoreToWin);
	}

	if (!TurnManager)
	{
		// Le regole non hanno destinatario: la partita girerebbe senza limite di round e nessuno lo saprebbe.
		UE_LOG(LogRT, Warning,
			TEXT("[RT] Nessun ARTTurnManager nel livello: il formato '%s' non e' stato applicato."),
			*Rules.FormatId.ToString());
		return true;
	}

	TurnManager->SetMatchRules(Rules);
	UE_LOG(LogRT, Log, TEXT("[RT] Formato di partita in vigore: '%s' (RoundLimit %d, soglia obiettivo %d)"),
		*Rules.FormatId.ToString(), Rules.RoundLimit, Rules.ScoreToWin);
	return true;
}

void ARTGameMode::SetupHexMatch(ARTHexMapActor* HexMap)
{
	if (!HexMap)
	{
		return;
	}

	ApplyMapSource(HexMap);

	// Le regole di formato prima delle unita': se il formato e' invalido non si allestisce nulla, e la mappa
	// resta a schermo con il motivo nel log (stesso trattamento delle celle di partenza insufficienti).
	if (!ApplyMatchFormat(
			Cast<ARTTurnManager>(UGameplayStatics::GetActorOfClass(this, ARTTurnManager::StaticClass()))))
	{
		return;
	}

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

	// Il roster del catalogo eroi (CP 6.2-6.5), non piu' i due archetipi hard-coded. Le formazioni sono un
	// dato (`Team0Heroes`/`Team1Heroes`): qui si legge chi gioca, non si decide.
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	auto FindHero = [&Roster](const FName& HeroId) -> const URTHeroData*
	{
		for (const URTHeroData* Hero : Roster)
		{
			if (Hero && Hero->HeroId == HeroId) { return Hero; }
		}
		return nullptr;
	};

	// Un eroe non puo' stare in due posti: la stessa istanza spawnata due volte condividerebbe le azioni
	// (`Abilities` e' un array di puntatori), e due unita' finirebbero per ricaricare la stessa abilita'.
	TSet<FName> Spawned;
	int32 CellIndex = 0;
	const TArray<const TArray<FName>*> Formations = { &Team0Heroes, &Team1Heroes };

	for (int32 TeamId = 0; TeamId < Formations.Num(); ++TeamId)
	{
		for (const FName& HeroId : *Formations[TeamId])
		{
			if (CellIndex >= Start.Num())
			{
				UE_LOG(LogRT, Warning, TEXT("[RT] Celle di partenza insufficienti: %s non entra in campo"),
					*HeroId.ToString());
				continue;
			}
			if (Spawned.Contains(HeroId))
			{
				UE_LOG(LogRT, Warning, TEXT("[RT] %s e' schierato due volte: la seconda copia e' ignorata"),
					*HeroId.ToString());
				continue;
			}

			const URTHeroData* Hero = FindHero(HeroId);
			if (Hero == nullptr)
			{
				UE_LOG(LogRT, Warning, TEXT("[RT] %s non e' nel catalogo eroi: nessuna unita' spawnata"),
					*HeroId.ToString());
				continue;
			}

			SpawnHero(TeamId, Hero, Start[CellIndex], Origin, HexSize, LayerHeight);
			Spawned.Add(HeroId);
			++CellIndex;
		}
	}

	UE_LOG(LogRT, Log, TEXT("[RT] Board 2v2 esagonale avviata su %d celle con %d eroi"),
		Map ? Map->NumCells() : 0, Spawned.Num());
}

ARTUnit* ARTGameMode::SpawnHero(int32 TeamId, const URTHeroData* Hero, const FRTCellId& InCell,
	const FVector& Origin, float HexSize, float LayerHeight)
{
	UWorld* World = GetWorld();
	if (!World || Hero == nullptr)
	{
		return nullptr; // fail-closed: senza dati non si spawna un'unita' con statistiche inventate
	}

	// Classe visiva per eroe: se assegnata (BP_Unit con skeletal) usala, altrimenti fallback al cilindro C++.
	// E' il comportamento di ripiego di sempre, ora per HeroId invece che per archetipo.
	const TSubclassOf<ARTUnit>* Configured = HeroUnitClasses.Find(Hero->HeroId);
	UClass* UnitClass = (Configured && *Configured) ? Configured->Get() : ARTUnit::StaticClass();

	// Deferred: team e statistiche PRIMA di BeginPlay, cosi' colore e dati sono corretti al primo frame.
	ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(UnitClass, FTransform::Identity);
	if (Unit)
	{
		Unit->TeamId = TeamId;
		Unit->bIsBotControlled = (TeamId == 1); // team 1 giocato dal bot
		Unit->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		Unit->PlaceOnCell(InCell, Origin, HexSize, LayerHeight);
	}
	return Unit;
}

FString ARTGameMode::ResolveScenarioToRun() const
{
	// La console variable PREVALE sulla proprieta': la proprieta' e' la configurazione persistente («questo
	// progetto, per ora, esegue questo scenario»), la console variable e' l'intento estemporaneo di chi lancia
	// («adesso, solo per questa volta, eseguine un altro») — da riga di comando o in CI. Il piu' specifico
	// vince, che e' la stessa regola di ogni override di configurazione.
	const FString FromConsole = CVarRTTestScenario.GetValueOnGameThread();
	if (FromConsole.IsEmpty())
	{
		return ScenarioToRun;
	}

	// ...ma NON in silenzio. Una console variable dura quanto il processo dell'editor: digitata una volta,
	// resta attiva per ogni Play successivo e continua a scavalcare la tendina senza che nulla lo dica. E'
	// successo davvero — si sceglieva uno scenario nel Details Panel e ne partiva un altro, con l'unico
	// indizio nel comportamento a schermo. La precedenza resta giusta; ad essere sbagliato era il silenzio.
	if (!ScenarioToRun.IsEmpty() && ScenarioToRun != FromConsole)
	{
		UE_LOG(LogRT, Warning,
			TEXT("[RT-Test] La console variable rt.Test.Scenario='%s' SCAVALCA la proprieta' "
				 "ScenarioToRun='%s' del GameMode. Per tornare a usare la proprieta': `rt.Test.Scenario \"\"`."),
			*FromConsole, *ScenarioToRun);
	}
	return FromConsole;
}

TArray<FString> ARTGameMode::GetScenarioOptions() const
{
	// Voce vuota in TESTA: e' come si torna alla partita normale dal menu. Senza, l'unico modo per svuotare il
	// campo sarebbe cancellarne il testo a mano — proprio cio' che il menu a tendina dovrebbe evitare.
	TArray<FString> Options;
	Options.Add(FString());
	Options.Append(URTScenarioIndex::ListIds(ScenarioFilterA, ScenarioFilterB));
	return Options;
}

TArray<FString> ARTGameMode::GetScenarioTagOptions() const
{
	// Voce vuota in testa anche qui, e per lo stesso motivo: e' come si smette di filtrare. Senza, l'unico
	// modo per togliere un filtro sarebbe cancellarne il testo a mano.
	TArray<FString> Options;
	Options.Add(FString());
	Options.Append(URTScenarioIndex::ListTags());
	return Options;
}

void ARTGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!ScenarioSession.IsValid() || ScenarioSession->IsFinished())
	{
		return;
	}

	// `bPumpTurnManager = false`: qui il mondo ticca gia' il turn manager. Pomparlo anche da qui lo farebbe
	// correre al doppio della velocita', e il playback che si vuole GUARDARE passerebbe in meta' del tempo.
	ScenarioSession->Step(DeltaSeconds, /*bPumpTurnManager=*/ false);

	if (ScenarioSession->IsFinished())
	{
		const FRTTestResult& Result = ScenarioSession->GetResult();
		FString ReportDir, WriteError;
		if (!URTTestReportWriter::Write(Result, FString(), ReportDir, WriteError))
		{
			UE_LOG(LogRT, Error, TEXT("[RT-Test] report non scritto: %s"), *WriteError);
		}
		UE_LOG(LogRT, Warning, TEXT("[RT-Test] FINITO %s -> %s (%d/%d assertion, %d turni) · report: %s"),
			*Result.ScenarioId, *Result.OutcomeString(), Result.PassedCount(), Result.Assertions.Num(),
			Result.TurnsPlayed, ReportDir.IsEmpty() ? TEXT("non scritto") : *ReportDir);

		for (const FRTAssertionResult& A : Result.Assertions)
		{
			if (!A.bPassed)
			{
				UE_LOG(LogRT, Error, TEXT("[RT-Test]   FALLITA %s: atteso %s, ottenuto %s"),
					*A.Description, *A.Expected, *A.Actual);
			}
		}
	}
}

bool ARTGameMode::IsScenarioRunning() const
{
	return ScenarioSession.IsValid() && !ScenarioSession->IsFinished();
}

FString ARTGameMode::GetScenarioBannerText() const
{
	// La condizione e' «questa sessione E' una run di scenario», non «lo scenario sta girando»: la partita
	// normale non viene allestita nemmeno dopo che lo scenario e' finito, ed e' proprio allora che chi guarda
	// si chiede dove sia il gioco.
	const FString ScenarioId = ResolveScenarioToRun();
	if (ScenarioId.IsEmpty())
	{
		return FString();
	}

	// La FONTE va detta anche a schermo, per la stessa ragione per cui il log la dice: una console variable
	// impostata una volta scavalca la tendina a ogni Play successivo, e senza saperlo si cerca il difetto
	// nella property sbagliata.
	const TCHAR* Source = CVarRTTestScenario.GetValueOnGameThread().IsEmpty()
		? TEXT("BP_GameMode")
		: TEXT("rt.Test.Scenario");

	FString Esito = TEXT("in corso");
	if (ScenarioSession.IsValid() && ScenarioSession->IsFinished())
	{
		Esito = ScenarioSession->GetResult().OutcomeString();
	}

	return FString::Printf(TEXT("SCENARIO %s [%s]  -  %s  -  la partita normale NON e' allestita"),
		*ScenarioId, Source, *Esito);
}
