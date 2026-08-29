#include "RTGameMode.h"
#include "Camera/RTCameraPawn.h"
#include "Player/RTPlayerController.h"
#include "UI/RTHUD.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Unit/RTUnit.h"
#include "Ability/RTCatalogLibrary.h" // DefaultLoadoutFor: l'equipaggiamento con cui un eroe entra in partita
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Turn/RTTurnManager.h"
#include "Frontend/RTFrontendNavigator.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestResult.h"
#include "ScenarioHarness/RTScenarioSession.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTTestReportWriter.h"
#include "UObject/ConstructorHelpers.h" // FClassFinder: i BP_Unit_* dei quattro eroi (CP E21.1)
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

/** Definita in Test/RTTestConsole.cpp: scenario da eseguire all'avvio invece della partita normale. */
extern TAutoConsoleVariable<FString> CVarRTTestScenario;

/**
 * LE TRE SORGENTI dello scenario da eseguire, e quale vince.
 *
 * In ordine di specificita' crescente:
 *
 *     proprieta' del GameMode  <  -RTScenario=<Id>  <  rt.Test.Scenario
 *     (configurazione            (intento di        (intento di adesso, e si puo'
 *      persistente)               questo avvio)      digitare a meta' sessione)
 *
 * La regola e' quella di sempre — il piu' specifico vince — applicata al TEMPO: la console si puo' digitare
 * dopo l'avvio, quindi deve poter scavalcare cio' che l'avvio aveva chiesto. Se fosse il contrario, un flag
 * di riga di comando renderebbe impossibile cambiare scenario senza riavviare.
 *
 * ⚠️ **Perche' esiste la sorgente di mezzo, che sembra un doppione della console.** In una build **Shipping**
 * `-dpcvars=rt.Test.Scenario=...` NON arriva: in `DeviceProfileManager.cpp` tutto il parsing di `-dpcvars=`
 * sta dentro `#if !UE_BUILD_SHIPPING`, quindi la variabile non viene mai impostata e il GameMode legge vuoto.
 * `FParse::Value` sulla riga di comando non ha quella guardia. Senza questa sorgente, l'unico modo di scegliere
 * uno scenario in Shipping sarebbe la proprieta' di `BP_GameMode` — cioe' un `.uasset` da modificare
 * nell'editor, che per giunta cambierebbe il comportamento predefinito del gioco che si distribuisce.
 * Misurato eseguendo, non dedotto: `#926`.
 */
namespace RTScenarioEntry
{
	/** Il valore di `-RTScenario=<Id>`, vuoto se il flag non c'e'. */
	static FString FromCommandLine()
	{
		FString Value;
		FParse::Value(FCommandLine::Get(), TEXT("RTScenario="), Value);
		return Value;
	}

	enum class EWinner : uint8 { Property, CommandLine, ConsoleVariable };

	/**
	 * Chi vince, in un posto solo.
	 *
	 * Il log dell'AUTO-RUN e la banda a schermo dicono entrambi la fonte, con etichette diverse. Prima
	 * calcolavano la risposta ciascuno per conto proprio, con due ternari identici: aggiungere una terza
	 * sorgente li avrebbe fatti divergere, e si sarebbe visto solo leggendo la banda accanto al log.
	 */
	static EWinner Winner()
	{
		if (!CVarRTTestScenario.GetValueOnGameThread().IsEmpty()) { return EWinner::ConsoleVariable; }
		if (!FromCommandLine().IsEmpty())                        { return EWinner::CommandLine; }
		return EWinner::Property;
	}
}

// `LogRT` serve gia' qui — `RTAutobattleEntry::FromCommandLine` avvisa su un valore che non riconosce — e la
// categoria e' dichiarata in questo header, che il file include piu' sotto insieme agli altri di dominio.
// Non e' una duplicazione: l'include ha la sua guardia, e la riga dice a chi legge da dove viene `LogRT`.
#include "RefactorTactics.h"

/**
 * LA MODALITA' NON PRESIDIATA e i suoi secondi di Planning (CP 47.1, #954).
 *
 * Non `static`: i test dichiarano `extern` su queste due, come gia' fanno per `CVarRTTestScenario`. Una
 * console variable che i test non possono pilotare si verifica solo a mano.
 *
 * ⚠️ **`-1` significa «non impostata», e non e' un dettaglio di implementazione.** Con `0` come sentinella
 * la console non potrebbe **spegnere** l'autobattle acceso dalla proprieta': «zero» sarebbe insieme
 * «nessuna richiesta» e «richiesta di partita normale», cioe' due domande diverse con la stessa risposta.
 * Lo stesso vale per i secondi: `0` e' un valore legittimo (turni incatenati, nessuna attesa) e non puo'
 * voler dire anche «non ho chiesto nulla».
 */
TAutoConsoleVariable<int32> CVarRTAutobattle(
	TEXT("rt.Match.Autobattle"),
	-1,
	TEXT("Partita non presidiata: entrambe le squadre al bot. -1 = non impostata (vale il resto), 0 = partita normale, 1 = autobattle."),
	ECVF_Default);

TAutoConsoleVariable<float> CVarRTPlanningSeconds(
	TEXT("rt.Match.PlanningSeconds"),
	-1.f,
	TEXT("Secondi della fase di Planning. Negativo = non impostata: vale la riga di comando, poi la proprieta', poi il TurnManager."),
	ECVF_Default);

/**
 * LE TRE SORGENTI della modalita' non presidiata, con la stessa forma e lo stesso ordine di
 * `RTScenarioEntry`:
 *
 *     proprieta' del GameMode  <  -RTAutobattle[=0|1]  <  rt.Match.Autobattle
 *
 * La sorgente di mezzo non e' un doppione della console, ed e' lo stesso motivo misurato in `#926`: in una
 * build **Shipping** `-dpcvars=` e' compilato fuori (`DeviceProfileManager.cpp`, dentro
 * `#if !UE_BUILD_SHIPPING`), quindi la console non arriva mai e resterebbe solo la proprieta' — cioe' un
 * `.uasset` da modificare, che per giunta cambierebbe il comportamento predefinito del gioco spedito.
 * `FParse` non ha quella guardia.
 */
namespace RTAutobattleEntry
{
	/**
	 * Cosa chiede la riga di comando, se chiede qualcosa.
	 *
	 * Due forme, e la prima e' quella che si scrive istintivamente: `-RTAutobattle` nudo vale «accendi».
	 * `FParse::Param` la riconosce **solo** nuda — con `-RTAutobattle=1` il carattere dopo il nome e' `=`,
	 * quindi torna falso e decide `FParse::Value`. Sono due letture disgiunte, non due tentativi in fila.
	 */
	static TOptional<bool> FromCommandLine()
	{
		// ⚠️ **Letto come STRINGA, non con l'overload `int32`.** Quello passa da `FCString::Atoi`, e `Atoi`
		// di `true` vale **0**: `-RTAutobattle=true` — la forma che si scrive per prima — avrebbe SPENTO la
		// modalita' invece di accenderla, in silenzio e proprio sulla sorgente che esiste per il pacchetto
		// Shipping, dove non c'e' una console per accorgersene. Trovato in code review.
		FString Raw;
		if (FParse::Value(FCommandLine::Get(), TEXT("RTAutobattle="), Raw))
		{
			const FString Value = Raw.TrimStartAndEnd();
			if (Value == TEXT("1") || Value.Equals(TEXT("true"), ESearchCase::IgnoreCase)
				|| Value.Equals(TEXT("on"), ESearchCase::IgnoreCase))
			{
				return true;
			}
			if (Value == TEXT("0") || Value.Equals(TEXT("false"), ESearchCase::IgnoreCase)
				|| Value.Equals(TEXT("off"), ESearchCase::IgnoreCase))
			{
				return false;
			}

			// Un valore che non si capisce non ripiega in silenzio: e' la stessa cura gia' presa da
			// `ResolveMapSource` per `rt.Map.Source`, e per la stessa ragione — una modalita' decisa da un
			// refuso e' un playtest attribuito a una configurazione che non era in vigore.
			UE_LOG(LogRT, Warning,
				TEXT("[RT] -RTAutobattle='%s' non e' un valore riconosciuto (1/0, true/false, on/off, "
					 "oppure il flag nudo). Ignorato: decide la proprieta' del GameMode."),
				*Raw);
			return TOptional<bool>();
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("RTAutobattle")))
		{
			return true;
		}
		return TOptional<bool>();
	}

	enum class EWinner : uint8 { Property, CommandLine, ConsoleVariable };

	/**
	 * Chi vince, in un posto solo — la banda a schermo e il log dell'attivazione lo chiedono entrambi con
	 * etichette diverse. Calcolarlo due volte e' il modo in cui due risposte divergono e se ne accorge solo
	 * chi legge la banda accanto al log: e' gia' la ragione per cui `RTScenarioEntry::Winner` esiste.
	 */
	static EWinner Winner()
	{
		if (CVarRTAutobattle.GetValueOnGameThread() >= 0) { return EWinner::ConsoleVariable; }
		if (FromCommandLine().IsSet())                    { return EWinner::CommandLine; }
		return EWinner::Property;
	}

	/**
	 * Secondi di Planning quando la modalita' e' accesa e **nessuno** ha chiesto un valore.
	 *
	 * Non e' un numero di gusto: `PlanningSeconds` vale 30 di default, e mezzo minuto di attesa fra un turno
	 * e l'altro rende la demo inguardabile. Due secondi lasciano il tempo di vedere dove sono le unita'
	 * prima che si muovano — la stessa scelta gia' fatta per `ScenarioTurnPauseSeconds` (1,5 s), qui un filo
	 * piu' larga perche' qui il Planning copre anche la decisione dei bot.
	 */
	static constexpr float FallbackPlanningSeconds = 2.f;

	/**
	 * Il minimo che tiene VIVO il turno in una partita non presidiata.
	 *
	 * 🔴 **`0` non significa «turni incatenati»: significa fermo per sempre.** `SetPlanningSeconds` e
	 * `StartPlanningTimer` armano il timer solo `if (PlanningSeconds > 0.f)` — con zero non lo arma nessuno,
	 * `OnPlanningTimeout` non scatta mai, `LockInAndResolve` non viene chiamato, e in una partita non
	 * presidiata **non c'e' nessuno che possa premere il lock-in**. La partita resta al turno 1 mentre la
	 * banda dichiara che si sta giocando da sola.
	 *
	 * ⚠️ Zero e' legittimo altrove, ed e' da li' che veniva la convinzione sbagliata: `RTScenarioSession`
	 * chiama `SetPlanningSeconds(0.f)` per le run headless — ma li' il turno lo pompa l'harness. La
	 * differenza non e' il valore, e' chi fa avanzare il turno.
	 *
	 * ∴ l'intento «il piu' veloce possibile» resta onorato e non viene riportato al ripiego di 2 s: viene
	 * alzato al minimo che l'orologio del motore sa ancora far scattare, e la correzione e' dichiarata nel
	 * log invece che applicata in silenzio. Trovato in code review, non da un playtest.
	 */
	static constexpr float MinUnattendedPlanningSeconds = 0.1f;
}
/** Definita in ScenarioHarness/RTTestConsole.cpp: scavalca `MapSource` da riga di comando. */
extern TAutoConsoleVariable<FString> CVarRTMapSource;
/** Definita nello stesso file: la fixture per nome, che vince su `rt.Map.Source` (`#1290`). */
extern TAutoConsoleVariable<FString> CVarRTMapFixture;

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

	// PRESENTAZIONE (CP E21.1, #287): i quattro eroi entrano in campo con la loro skeletal Paragon.
	//
	// 🔴 **Il pezzo che mancava era questo, e non era l'asset.** I quattro `BP_Unit_*` esistono da tempo,
	// sono versionati e portano gia' la mesh giusta — `Phase_GDC` inclusa, che e' il caso che non si chiama
	// come il suo eroe. Ma `HeroUnitClasses` nasceva VUOTA e nessuno la riempiva: `BP_GameMode` non la
	// tocca, quindi `SpawnHero` ricadeva sempre sul fallback e in partita si vedevano quattro cilindri
	// mentre gli asset erano pronti a due passi.
	//
	// ⚠️ **Il fallback non cambia.** `SpawnHero` usa `ARTUnit::StaticClass()` per ogni `HeroId` assente da
	// questa mappa, e un finder che fallisce lascia la voce ASSENTE invece di aggiungerne una nulla: senza
	// il Blueprint si torna al cilindro colorato e la partita resta giocabile.
	//
	// ⚠️ **Resta `EditAnywhere`, quindi questo e' un DEFAULT e non un vincolo**: `BP_GameMode` puo' ancora
	// scavalcare una voce o svuotare la mappa, ed e' la via per provare un personaggio senza ricompilare.
	{
		// `Succeeded()` non e' const (fa `!!*Class`), quindi il finder si legge dal solo campo `Class`:
		// e' la stessa condizione, e permette di passare il finder per riferimento costante.
		auto Assegna = [this](const TCHAR* HeroId, const ConstructorHelpers::FClassFinder<ARTUnit>& Finder)
		{
			if (Finder.Class != nullptr)
			{
				HeroUnitClasses.Add(FName(HeroId), Finder.Class);
			}
		};

		static ConstructorHelpers::FClassFinder<ARTUnit> GadgetBP(TEXT("/Game/RT/Characters/Gadget/Blueprints/BP_Unit_Gadget"));
		static ConstructorHelpers::FClassFinder<ARTUnit> PhaseBP(TEXT("/Game/RT/Characters/Phase/Blueprints/BP_Unit_Phase"));
		static ConstructorHelpers::FClassFinder<ARTUnit> RiktorBP(TEXT("/Game/RT/Characters/Riktor/Blueprints/BP_Unit_Riktor"));
		static ConstructorHelpers::FClassFinder<ARTUnit> WraithBP(TEXT("/Game/RT/Characters/Wraith/Blueprints/BP_Unit_Wraith"));

		Assegna(TEXT("Hero.Gadget"), GadgetBP);
		Assegna(TEXT("Hero.Phase"),  PhaseBP);
		Assegna(TEXT("Hero.Riktor"), RiktorBP);
		Assegna(TEXT("Hero.Wraith"), WraithBP);
	}
}

void ARTGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 🔴 **Il lato di partita del confine col frontend (CP 46.6).** Sta in `BeginPlay` e non in
	// `SetupHexMatch` perche' non ha niente a che vedere con l'allestimento della mappa: vale anche in uno
	// scenario auto-run, dove la partita normale non viene allestita affatto e il `return` piu' sotto
	// salterebbe l'iscrizione.
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		URTFrontendNavigator* Navigator = GameInstance->GetSubsystem<URTFrontendNavigator>();
		ListenForLevelRequests(Navigator);

		// 🔴 **La partita si DICHIARA al flow, e non farlo produceva due dead-end** (vedi
		// `RTScreenIds::Match`). Il navigatore sopravvive al cambio di livello ma il suo stack no: senza
		// questa riga restava `[Main]` — la radice lasciata dal menu — e `RESUME` ripresentava il Main Menu
		// **sopra la partita viva**; oppure restava vuoto, avviando il gioco direttamente su `L_HexArena`,
		// e `ESC` impilava `Pause` come radice da cui `ResumeMatch` non usciva piu'.
		//
		// ⚠️ **Dopo `ListenForLevelRequests` e non prima**: un `EnterMatch` che fallisse non deve portarsi
		// via l'iscrizione, che con lo stato del flow non c'entra.
		if (Navigator)
		{
			Navigator->EnterMatch();
		}
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
	// (i BP_Unit possono agganciarsi ai suoi delegate di playback senza attese).
	//
	// ⚠️ **La ragione di sicurezza e' cambiata con CP 13.5, e va scritta perche' era un'invariante.** Fino al
	// 2026-08-11 questo commento diceva «il TurnManager non tocca le unita' al proprio BeginPlay (fa solo
	// StartPlanningTimer)»: non e' piu' vero, perche' `StartPlanningTimer` chiama `PlanBots`, che ora chiama
	// `EnsureMatchRoster` e rinfresca la conoscenza di squadra.
	//
	// Resta sicuro, ma per un motivo DIVERSO e piu' fragile: a questo punto nel mondo non c'e' ancora
	// nessuna `ARTUnit` (le spawna `SetupHexMatch`, piu' sotto), e le due funzioni sono scritte per
	// sopportarlo — `EnsureMatchRoster` con un roster vuoto **ritorna senza marcare `bMatchRosterBuilt`**,
	// quindi ritenta al primo `LockInAndResolve`; il rinfresco su zero unita' produce uno stato vuoto, che e'
	// gia' il default. Se una di quelle due guardie venisse indebolita, la prima partita si romperebbe **qui**
	// e in silenzio. Trovato in code review della PR #540 come commento diventato falso.
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
		const TCHAR* Source = TEXT("proprieta' del GameMode");
		switch (RTScenarioEntry::Winner())
		{
		case RTScenarioEntry::EWinner::ConsoleVariable: Source = TEXT("console rt.Test.Scenario"); break;
		case RTScenarioEntry::EWinner::CommandLine:     Source = TEXT("riga di comando -RTScenario="); break;
		case RTScenarioEntry::EWinner::Property:        break;
		}
		// ⚠️ Un free-run non enumera turni: `Turns.Num()` li' vale **zero**, e questa riga annuncerebbe «0 turni»
		// un istante prima di giocarne dieci. La riga esiste perche' chi guarda possa dire «sta girando quello
		// che ho scelto io» senza dedurlo dallo schermo, quindi dire il falso la rende peggio che assente.
		const int32 TurniAnnunciati = Scenario.bFreeRun ? Scenario.MaxTurns : Scenario.Turns.Num();
		UE_LOG(LogRT, Warning, TEXT("[RT-Test] AUTO-RUN %s (da: %s): %s%d turni, pausa %.1fs — avanza un passo per frame"),
			*TestScenario, Source, Scenario.bFreeRun ? TEXT("free-run, tetto ") : TEXT(""),
			TurniAnnunciati, ScenarioTurnPauseSeconds);

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

	// La registrazione del replay comincia QUI (`#469`), e la posizione e' stata corretta due volte perche'
	// «dove si sa che e' una partita vera» e' piu' stretto di quanto sembri:
	//
	//  - non nel `BeginPlay` del TurnManager: lo spawnano a mano ventisette file di test e lo
	//    `ScenarioHarness`, e li avremmo fatti scrivere archivi tutti;
	//  - non dentro `SetupHexMatch`: **`RTHeroSpawnTests` lo chiama direttamente**, perche' verifica lo
	//    spawn del roster attraverso il percorso vero — e cosi' due test lasciavano un `history.rtindex`
	//    nella `Saved/` del progetto a ogni run.
	//
	// Qui invece ci si arriva **solo** avviando il gioco: `SetupHexMatch` ha questo unico chiamante in
	// produzione, e il ramo dello scenario e' gia' uscito con un `return` piu' sopra. Dopo `SetupHexMatch`
	// anche il formato e' risolto, quindi il manifest nasce con quello vero.
	//
	// ⚠️ `SetupHexMatch` e' `void` e ha cinque uscite anticipate: da qui non si sa se ha allestito davvero.
	// Il caso che conta — formato non risolto — lo intercetta `BeginReplayRecording` stessa, che si rifiuta
	// senza toccare il disco. Restano fuori i casi in cui il formato E' valido ma l'allestimento fallisce
	// dopo (celle di partenza insufficienti, composizione che non torna): li' la registrazione parte per una
	// partita con zero unita'. E' un comportamento **preesistente** — valeva anche con la chiamata dentro
	// `SetupHexMatch`, che stava comunque prima di quei controlli — e non lo cambia questa riga.
	if (ARTTurnManager* TurnManager =
			Cast<ARTTurnManager>(UGameplayStatics::GetActorOfClass(this, ARTTurnManager::StaticClass())))
	{
		TurnManager->BeginReplayRecording();
	}
}

ERTMapSource ARTGameMode::ResolveMapSource() const
{
	// Stessa regola di `ResolveScenarioToRun`: il piu' specifico vince. La proprieta' e' la configurazione
	// persistente del progetto, la console variable e' l'intento di chi lancia adesso — e serve perche' la
	// proprieta' vive in un `.uasset`, quindi cambiarla richiede l'editor.
	const FString FromConsole = CVarRTMapSource.GetValueOnGameThread().TrimStartAndEnd();
	if (FromConsole.IsEmpty())
	{
		return MapSource;
	}

	const UEnum* Enum = StaticEnum<ERTMapSource>();
	const int64 Value = Enum ? Enum->GetValueByNameString(FromConsole) : INDEX_NONE;
	if (Value == INDEX_NONE)
	{
		// Fail-closed sul VALORE, non sulla partita: si gioca con la proprieta' e si dice perche'. Ripiegare in
		// silenzio farebbe girare un playtest sulla mappa sbagliata credendo di averla scelta.
		UE_LOG(LogRT, Warning,
			TEXT("[RT] rt.Map.Source='%s' non e' un valore di ERTMapSource: ignorata, uso la proprieta' del "
				 "GameMode. Valori validi: LevelAsset, GeneratedTestArena, GeneratedDemoArena."),
			*FromConsole);
		return MapSource;
	}

	const ERTMapSource Resolved = static_cast<ERTMapSource>(Value);
	if (Resolved != MapSource)
	{
		// Non in silenzio, per la stessa ragione di `rt.Test.Scenario`: una console variable dura quanto il
		// processo, e continuerebbe a scavalcare la proprieta' a ogni Play senza che nulla lo dica.
		UE_LOG(LogRT, Warning,
			TEXT("[RT] La console variable rt.Map.Source='%s' SCAVALCA la proprieta' MapSource del GameMode."),
			*FromConsole);
	}
	return Resolved;
}

void ARTGameMode::ApplyMapSource(ARTHexMapActor* HexMap)
{
	if (!HexMap)
	{
		return;
	}

	// La FIXTURE vince su tutto il resto, ed e' il piu' specifico dei tre livelli: proprieta' del GameMode,
	// `rt.Map.Source`, e questa. Serve perche' le due arene generabili non portano superfici — `MakeDemoArena`
	// le lascia tutte `Default`, `MakeTestArena` tutte `Rough` — quindi nessuna delle due permette di
	// guardare a schermo se le nove tinte della tavolozza si distinguono (`#1290`).
	const FString FixtureId = CVarRTMapFixture.GetValueOnGameThread().TrimStartAndEnd();
	if (!FixtureId.IsEmpty())
	{
		if (URTHexMapAsset* Fixture = URTMatchSetupLibrary::MakeFixtureArena(HexMap, FixtureId))
		{
			HexMap->MapAsset = Fixture;
			HexMap->RebuildInstances();
			UE_LOG(LogRT, Warning, TEXT("[RT] rt.Map.Fixture='%s': uso quella fixture (%d celle). "
				"La mappa del livello e rt.Map.Source sono ignorate."),
				*FixtureId, HexMap->MapAsset->NumCells());
			return;
		}

		// Fail-closed sul VALORE, non sulla partita: stessa cura di `rt.Map.Source`. Un nome sbagliato che
		// ripiegasse in silenzio farebbe attribuire un playtest a una board che non era in vigore.
		// ⚠️ I nomi si CHIEDONO, non si riscrivono (`#1459`). Questa riga era il QUARTO elenco a mano
		// delle stesse fixture, e come gli altri tre nominava `DemoArena` — che non ha un ramo nel
		// dispatcher. `rt.Map.Fixture=DemoArena` produceva quindi un warning che elencava fra i nomi validi
		// esattamente quello appena rifiutato.
		UE_LOG(LogRT, Warning,
			TEXT("[RT] rt.Map.Fixture='%s' non e' una fixture nota: ignorata, si prosegue con la sorgente "
				 "configurata. Nomi validi: %s."),
			*FixtureId, *FString::Join(URTMatchSetupLibrary::KnownFixtureIds(), TEXT(", ")));
	}

	switch (ResolveMapSource())
	{
	case ERTMapSource::GeneratedTestArena:
		// Scelta esplicita: prevale anche su una mappa d'autore presente nel livello. Va dichiarato, non subito.
		HexMap->MapAsset = URTMatchSetupLibrary::MakeTestArena(HexMap);
		HexMap->RebuildInstances();
		UE_LOG(LogRT, Warning, TEXT("[RT] MapSource=GeneratedTestArena: uso la mappa di PROVA generata "
			"(%d celle, con ostacoli, muri, terreno costoso e piattaforma). La mappa del livello e' ignorata."),
			HexMap->MapAsset ? HexMap->MapAsset->NumCells() : 0);
		// CP 46.2: la stessa condizione, in una forma che un widget puo' leggere. E' la **prima riserva
		// di `G13`**, e finora esisteva solo in questa riga di log.
		StartupReport.Add(ERTStartupOutcome::UsingTestArena,
			FString::Printf(TEXT("%d celle"), HexMap->MapAsset ? HexMap->MapAsset->NumCells() : 0));
		return;

	case ERTMapSource::GeneratedDemoArena:
		HexMap->MapAsset = URTMatchSetupLibrary::MakeDemoArena(HexMap, DemoArenaRadius);
		HexMap->RebuildInstances();
		UE_LOG(LogRT, Warning, TEXT("[RT] MapSource=GeneratedDemoArena: uso l'arena di ripiego "
			"(esagono r=%d, %d celle). La mappa del livello e' ignorata."),
			DemoArenaRadius, HexMap->MapAsset ? HexMap->MapAsset->NumCells() : 0);
		StartupReport.Add(ERTStartupOutcome::UsingDemoArena,
			FString::Printf(TEXT("esagono r=%d"), DemoArenaRadius));
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
		StartupReport.Add(ERTStartupOutcome::LevelMapMissing,
			FString::Printf(TEXT("arena di ripiego r=%d"), DemoArenaRadius));
	}
}

bool ARTGameMode::ApplyMatchFormat(ARTTurnManager* TurnManager, const URTHexMapAsset* Map, FRTMatchRules& OutRules)
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
			// CP 46.2: fatale. `Reason` e' gia' prodotto dal validator — si **trasporta**, non si ricompone.
			StartupReport.Add(ERTStartupOutcome::FormatAssetInvalid, Reason);
			return false;
		}
	}
	else if (const URTMatchFormatData* Shipped = URTMatchFormatLibrary::FindShippedFormat(ShippedFormatId))
	{
		// Nessun asset, ma un formato SPEDITO con quell'identita': si gioca quello. E' la stessa strada con
		// cui eroi e azioni arrivano in partita senza che nessuno debba creare un `.uasset` in editor, ed e'
		// cio' che separa una build pacchettizzata «che gira» da una «che gioca il formato della release».
		if (!URTMatchFormatLibrary::ResolveRules(Shipped, Rules, Reason))
		{
			// Un formato spedito che non passa il proprio validator e' un difetto di CODICE, non di dato:
			// rifiutare e' l'unica risposta onesta, perche' ripiegare lo nasconderebbe fino al playtest.
			UE_LOG(LogRT, Error,
				TEXT("[RT] Il formato spedito '%s' non e' valido: %s. Partita non allestita."),
				*ShippedFormatId.ToString(), *Reason);
			StartupReport.Add(ERTStartupOutcome::ShippedFormatInvalid, Reason);
			return false;
		}
		UE_LOG(LogRT, Log,
			TEXT("[RT] Nessun MatchFormat assegnato: uso il formato SPEDITO '%s'. Assegna un "
				 "URTMatchFormatData al GameMode per sovrascriverlo."),
			*Rules.FormatId.ToString());
	}
	else
	{
		Rules = URTMatchFormatLibrary::MakeFallbackRules();
		UE_LOG(LogRT, Warning,
			TEXT("[RT] Nessun MatchFormat assegnato e nessun formato spedito per '%s': uso il RIPIEGO '%s' "
				 "(RoundLimit %d, soglia obiettivo %d). Assegna un URTMatchFormatData al GameMode per giocare "
				 "un formato dichiarato: le misure di playtest vanno attribuite al formato giusto."),
			*ShippedFormatId.ToString(), *Rules.FormatId.ToString(), Rules.RoundLimit, Rules.ScoreToWin);
		// CP 46.2. ⚠️ **Ramo raro, e vale la pena dire perche'**: ci si arriva solo se non esiste nemmeno un
		// formato **spedito** — e `Format.Skirmish2v2` e' spedito da C++ dal commit `9f44570d`. In una build
		// normale questa riga non scatta, ed e' giusto cosi'.
		// 🔴 Una stesura precedente la chiamava «seconda riserva di `G13`»: **falso**. Le due riserve sono
		// l'arena di test e il fatto che *«la via a punti non e' mai stata esercitata, perche' la soglia
		// obiettivo e' 0»* — che e' un valore del formato, non il formato di ripiego. Connessione plausibile
		// e sbagliata, trovata da un test rosso.
		StartupReport.Add(ERTStartupOutcome::UsingFallbackFormat, Rules.FormatId.ToString());
	}

	// CP 19.1: l'accoppiata formato/mappa si verifica QUI, prima di schierare. Un 3v3 Standard su una mappa
	// disegnata per il 2v2 non e' una partita piu' stretta, e' una partita sbagliata — e scoprirlo al terzo
	// turno costa un playtest. Vale anche per il ripiego: se il livello porta una mappa Operations, il 2v2 di
	// ripiego non e' la partita giusta da avviarci sopra.
	const TArray<FString> Mismatch = URTMatchFormatLibrary::ValidateAgainstMap(Rules, Map);
	if (Mismatch.Num() > 0)
	{
		UE_LOG(LogRT, Error,
			TEXT("[RT] Formato e mappa non combaciano: %s. Partita non allestita: assegna una mappa della "
				 "classe richiesta, oppure un formato disegnato per questa mappa."),
			*FString::Join(Mismatch, TEXT("; ")));
		StartupReport.Add(ERTStartupOutcome::FormatMapMismatch, FString::Join(Mismatch, TEXT("; ")));
		return false;
	}

	OutRules = Rules;

	if (!TurnManager)
	{
		// Le regole non hanno destinatario: la partita girerebbe senza limite di round e nessuno lo saprebbe.
		UE_LOG(LogRT, Warning,
			TEXT("[RT] Nessun ARTTurnManager nel livello: il formato '%s' non e' stato applicato."),
			*Rules.FormatId.ToString());
		// Degradato e non fatale: `return true` — la partita prosegue, ma senza limite di round e nessuno
		// lo saprebbe. E' esattamente il caso che il banner esiste per rendere visibile.
		StartupReport.Add(ERTStartupOutcome::NoTurnManager, Rules.FormatId.ToString());
		return true;
	}

	TurnManager->SetMatchRules(Rules);
	UE_LOG(LogRT, Log,
		TEXT("[RT] Formato di partita in vigore: '%s' (RoundLimit %d, soglia obiettivo %d, %d unita' per squadra)"),
		*Rules.FormatId.ToString(), Rules.RoundLimit, Rules.ScoreToWin, Rules.UnitsPerTeam);
	return true;
}

void ARTGameMode::SetupHexMatch(ARTHexMapActor* HexMap)
{
	if (!HexMap)
	{
		return;
	}

	// CP 46.2: l'allestimento riparte da zero a ogni chiamata. Senza il reset, un secondo `SetupHexMatch`
	// nella stessa sessione — `Play Again` — accumulerebbe le note della partita precedente e il banner
	// mostrerebbe condizioni che non valgono piu'.
	StartupReport.Reset();

	StartupReport.Phase = ERTLoadPhase::Map;
	ApplyMapSource(HexMap);

	// Le regole di formato prima delle unita': se il formato e' invalido non si allestisce nulla, e la mappa
	// resta a schermo con il motivo nel log (stesso trattamento delle celle di partenza insufficienti).
	StartupReport.Phase = ERTLoadPhase::Scenario;
	FRTMatchRules Rules;
	ARTTurnManager* TurnManager =
		Cast<ARTTurnManager>(UGameplayStatics::GetActorOfClass(this, ARTTurnManager::StaticClass()));

	// 🔴 **L'iscrizione all'annuncio di fine partita.** E' il punto in cui questo GameMode incontra il
	// `TurnManager`, quindi e' qui che si mette in ascolto: `BeginPlay` sarebbe troppo presto — il
	// `TurnManager` puo' non esistere ancora — e legarsi piu' tardi vorrebbe dire cercarlo una seconda volta.
	// `AddUniqueDynamic` perche' un allestimento ripetuto non deve aprire due Result.
	if (TurnManager)
	{
		TurnManager->OnMatchEnded.AddUniqueDynamic(this, &ARTGameMode::HandleMatchEnded);
	}

	if (!ApplyMatchFormat(TurnManager, HexMap->MapAsset, Rules))
	{
		// ⚠️ La fase resta a `Scenario`, non torna a `Idle`: **dove** ci si e' fermati e' l'informazione
		// che serve a chi guarda. Il fatale e' gia' nelle note.
		return;
	}

	StartupReport.Phase = ERTLoadPhase::Bots;

	// LA MODALITA' SI DECIDE QUI, una volta, PRIMA che le unita' entrino in campo — vedi
	// `IsAutobattleInEffect()`. Da questo punto in poi la sessione ha una risposta sola, e la banda non puo'
	// piu' descrivere una partita diversa da quella che si sta giocando.
	bAutobattleInEffect = ResolveAutobattle();
	AutobattleSourceLabel = TEXT("BP_GameMode");
	switch (RTAutobattleEntry::Winner())
	{
	case RTAutobattleEntry::EWinner::ConsoleVariable: AutobattleSourceLabel = TEXT("rt.Match.Autobattle"); break;
	case RTAutobattleEntry::EWinner::CommandLine:     AutobattleSourceLabel = TEXT("-RTAutobattle"); break;
	case RTAutobattleEntry::EWinner::Property:        break;
	}

	// RITMO DEL TURNO, prima del ritorno anticipato qui sotto: e' configurazione del turno, non
	// dell'allestimento, e vale anche su un livello che porta gia' le proprie unita' — dove l'allestimento
	// non interviene ma il Planning e' comunque quello con cui si giochera'.
	if (TurnManager)
	{
		float PlanningSeconds = ResolveMatchPlanningSeconds();

		// Zero fermerebbe la partita per sempre invece di incatenare i turni: nessuno arma il timer, e qui
		// non c'e' una mano umana che possa premere il lock-in. Vedi `MinUnattendedPlanningSeconds`.
		if (bAutobattleInEffect && PlanningSeconds == 0.f)
		{
			UE_LOG(LogRT, Warning,
				TEXT("[RT] AUTOBATTLE: Planning 0s bloccherebbe la partita al turno 1 — nessun timer viene "
					 "armato e non c'e' nessuno che possa chiudere il turno. Alzato a %.2fs."),
				RTAutobattleEntry::MinUnattendedPlanningSeconds);
			PlanningSeconds = RTAutobattleEntry::MinUnattendedPlanningSeconds;
		}

		if (PlanningSeconds >= 0.f)
		{
			TurnManager->SetPlanningSeconds(PlanningSeconds);
		}

		// #971 — stessa natura e stesso posto: e' configurazione del turno, non allestimento, e sta prima
		// del ritorno anticipato perche' vale anche su un livello che porta gia' le proprie unita'. Il
		// TurnManager non interroga il GameMode (vedi `SetUnattendedSession`): viene informato qui, dove la
		// modalita' e' appena stata latchata.
		TurnManager->SetUnattendedSession(bAutobattleInEffect);
	}

	// L'ATTIVAZIONE NON E' SILENZIOSA. Chi guarda vede le proprie unita' muoversi da sole, e la spiegazione
	// non deve stare solo in una riga di log che non si ha motivo di andare a cercare: la banda a schermo la
	// dichiara (`GetScenarioBannerText`), e questa riga la mette anche nel log con la FONTE — perche' una
	// console variable impostata una volta resta attiva a ogni Play successivo, e senza saperlo si cerca il
	// difetto nella proprieta' sbagliata.
	if (bAutobattleInEffect)
	{
		UE_LOG(LogRT, Warning,
			TEXT("[RT] AUTOBATTLE (da: %s): entrambe le squadre al bot, nessun input richiesto. "
				 "Planning %.2fs — questa NON e' una partita normale."),
			*AutobattleSourceLabel, TurnManager ? TurnManager->GetPlanningSeconds() : -1.f);
	}

	// Il livello puo' avere unita' gia' posate a mano: in quel caso l'allestimento automatico non interviene.
	if (UGameplayStatics::GetActorOfClass(this, ARTUnit::StaticClass()))
	{
		// ...ma la MODALITA' si applica lo stesso, e questo ramo e' l'unico posto in cui puo' farlo.
		//
		// 🔴 Su un livello con unita' proprie `SetupHexMatch` ritorna prima di arrivare a `SpawnHero`, quindi
		// quelle unita' tengono il valore che si portano da sole — il default della loro dichiarazione: il
		// log dichiarava «entrambe le squadre al bot» mentre la squadra 0 non pianificava nessuno, e la
		// partita macinava turni vuoti fino al `RoundLimit` con la banda che asseriva il contrario. Trovato
		// in code review: il blocco del Planning era gia' stato spostato sopra questo ritorno *per questo
		// scenario*, l'assegnazione no.
		//
		// ⚠️ La riga che stava qui dichiarava `SpawnHero` **unico** sito di scrittura di `bIsBotControlled`,
		// e non lo era. Chi lo scrive e' elencato alla dichiarazione del campo, `ARTUnit::bIsBotControlled`:
		// qui non si duplica.
		if (bAutobattleInEffect)
		{
			TArray<AActor*> Existing;
			UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Existing);
			int32 Switched = 0;
			for (AActor* Actor : Existing)
			{
				ARTUnit* Unit = Cast<ARTUnit>(Actor);
				if (Unit && !Unit->bIsBotControlled)
				{
					Unit->bIsBotControlled = true;
					++Switched;
				}
			}
			UE_LOG(LogRT, Warning,
				TEXT("[RT] AUTOBATTLE su unita' gia' presenti nel livello: %d di %d passate al bot. "
					 "L'allestimento automatico non interviene, la modalita' si'."),
				Switched, Existing.Num());
		}
		return;
	}

	// La composizione la dichiara il FORMATO (CP 19.2), non l'orchestratore: finche' il `2` viveva qui, «2v2»
	// era una proprieta' del codice di allestimento, e lo stress 4v4 di E17 doveva essere un caso speciale del
	// `GameMode` invece di un formato che dichiara 4.
	const URTHexMapAsset* Map = HexMap->MapAsset;
	const int32 CellsNeeded = Rules.UnitsPerTeam * 2;
	const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Map, Rules.UnitsPerTeam, /*Layer=*/ 0);
	if (Start.Num() != CellsNeeded)
	{
		UE_LOG(LogRT, Warning,
			TEXT("[RT] Mappa esagonale senza celle percorribili sufficienti: il formato '%s' ne chiede %d "
				 "(%d per squadra) e la mappa ne offre %d. Partita non allestita."),
			*Rules.FormatId.ToString(), CellsNeeded, Rules.UnitsPerTeam, Start.Num());
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

	// La formazione deve dichiarare tanti eroi quanti il formato ne schiera (CP 19.2). Senza questo controllo
	// il formato direbbe 4 e il campo ne vedrebbe 2: la partita girerebbe, e il numero dichiarato sarebbe un
	// dato che nessuno onora — il difetto ricorrente di questo repository.
	for (int32 TeamId = 0; TeamId < Formations.Num(); ++TeamId)
	{
		if (Formations[TeamId]->Num() != Rules.UnitsPerTeam)
		{
			UE_LOG(LogRT, Error,
				TEXT("[RT] Il formato '%s' schiera %d unita' per squadra, ma la formazione della squadra %d ne "
					 "dichiara %d. Partita non allestita: allinea Team%dHeroes al formato, o il formato alla "
					 "formazione."),
				*Rules.FormatId.ToString(), Rules.UnitsPerTeam, TeamId, Formations[TeamId]->Num(), TeamId);
			return;
		}
	}

	// Le formazioni si risolvono TUTTE prima che entri in campo qualcuno (#1069). Prima la guardia stava
	// dentro il ciclo di spawn e faceva `continue`: un nome sbagliato produceva una partita allestita a
	// META', con le unita' risolte in campo e le altre no — e a schermo sembrava una partita normale.
	// E' lo stesso dato che il controllo del conteggio qui sopra protegge dall'altro lato, e riceve lo
	// stesso trattamento: `Error` e nessuna unita' spawnata.
	TArray<TArray<const URTHeroData*>> Lineups;
	for (int32 TeamId = 0; TeamId < Formations.Num(); ++TeamId)
	{
		TArray<const URTHeroData*>& Lineup = Lineups.AddDefaulted_GetRef();
		for (const FName& HeroId : *Formations[TeamId])
		{
			const URTHeroData* Hero = FindHero(HeroId);
			if (Hero == nullptr)
			{
				UE_LOG(LogRT, Error,
					TEXT("[RT] '%s' non e' nel catalogo eroi: partita non allestita. Correggi Team%dHeroes, "
						 "o aggiungi l'eroe al catalogo."),
					*HeroId.ToString(), TeamId);
				StartupReport.Add(ERTStartupOutcome::RosterHeroMissing, HeroId.ToString());
				return;
			}
			Lineup.Add(Hero);
		}
	}

	for (int32 TeamId = 0; TeamId < Formations.Num(); ++TeamId)
	{
		for (int32 Slot = 0; Slot < Lineups[TeamId].Num(); ++Slot)
		{
			const FName& HeroId = (*Formations[TeamId])[Slot];
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

			SpawnHero(TeamId, Lineups[TeamId][Slot], Start[CellIndex], Origin, HexSize, LayerHeight);
			Spawned.Add(HeroId);
			++CellIndex;
		}
	}

	UE_LOG(LogRT, Log, TEXT("[RT] Board 2v2 esagonale avviata su %d celle con %d eroi"),
		Map ? Map->NumCells() : 0, Spawned.Num());

	// CP 46.2: allestimento concluso. `Ready` **non** significa «senza problemi» — le note degradate
	// restano, ed e' proprio la combinazione «pronto, ma con due ripieghi» il caso di `G13`.
	StartupReport.Phase = ERTLoadPhase::Ready;
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
		// Team 1 al bot: e' il default di sempre, pinnato da `RTHeroSpawnTests`. La modalita' non presidiata
		// (#954) lo ESTENDE — mette al bot anche la squadra 0 — e non lo sostituisce: con l'autobattle spento
		// questa riga vale esattamente quanto valeva prima.
		//
		// Legge la decisione LATCHATA e non il resolver: le quattro unita' di una partita devono ricevere la
		// stessa risposta, e una console variable digitata fra uno spawn e l'altro produrrebbe una squadra
		// mista. Vale anche per il costo — questa riga gira una volta per unita'.
		Unit->bIsBotControlled = (TeamId == 1) || bAutobattleInEffect;
		Unit->ConfigureFromHeroData(Hero);

		// EQUIPAGGIAMENTO (`#1054`, CP 7.4). Fino al 2026-08-16 nessuno equipaggiava: `DefaultLoadoutFor`
		// aveva chiamanti solo nei test, e la spinta di 2 di `Weapon.Impact` esisteva nei dati e in nessuna
		// partita — la ragione misurata per cui `Guard` e `Brace` non si distinguevano a video (`#403`).
		//
		// ⚠️ **Qui e non in `ConfigureFromHeroData`**, che e' la copia dei DATI dell'eroe: metterla la'
		// equipaggerebbe anche le decine di unita' che i test unitari costruiscono per avere «un'unita'
		// qualunque», cambiando sotto i piedi misure che non parlano di equipaggiamento.
		//
		// ⚠️ **E nessun ramo per il bot**: questa funzione e' l'ingresso dello spawn di partita ed e'
		// dove si decide `bIsBotControlled` due righe sopra, quindi le due squadre che passano di qui sono
		// allestite dallo stesso codice. Chi altro scrive quel campo: `ARTUnit::bIsBotControlled`.
		// Un `if (!bIsBotControlled)` sarebbe la forma in cui «il bot gioca un altro gioco» rientra.
		//
		// `DefaultLoadoutFor` risponde VUOTO per un eroe i cui pezzi non sono spediti — oggi Gadget e
		// Wraith, che §4 assegna a due gadget che v0.1 non costruisce — e un array vuoto qui non fa nulla.
		Unit->EquipLoadout(URTCatalogLibrary::DefaultLoadoutFor(Hero->HeroId));

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
		// Sorgente di mezzo: l'unica che arriva anche in Shipping, dove `-dpcvars` e' compilato fuori. Vedi il
		// commento di `RTScenarioEntry` in testa al file per il perche' non sia un doppione della console.
		const FString FromCmdLine = RTScenarioEntry::FromCommandLine();
		if (FromCmdLine.IsEmpty())
		{
			return ScenarioToRun;
		}

		// Stessa regola del caso sotto, e per la stessa ragione: una precedenza silenziosa manda a cercare il
		// difetto nella property sbagliata. Il messaggio nomina il flag, non la console, perche' e' quello che
		// chi legge deve togliere dalla riga di comando per tornare alla proprieta'.
		if (!ScenarioToRun.IsEmpty() && ScenarioToRun != FromCmdLine)
		{
			UE_LOG(LogRT, Warning,
				TEXT("[RT-Test] La riga di comando -RTScenario='%s' SCAVALCA la proprieta' "
					 "ScenarioToRun='%s' del GameMode. Per tornare a usare la proprieta': togli il flag."),
				*FromCmdLine, *ScenarioToRun);
		}
		return FromCmdLine;
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

bool ARTGameMode::ResolveAutobattle() const
{
	// La console PREVALE, e sa spegnere oltre che accendere: `>= 0` significa «ha detto qualcosa», e cosa
	// abbia detto lo dice il valore. Con `0` come sentinella di «non impostata» non ci sarebbe modo di
	// chiedere una partita normale dalla console mentre la proprieta' e' accesa.
	const int32 FromConsole = CVarRTAutobattle.GetValueOnGameThread();
	if (FromConsole >= 0)
	{
		// ...ma NON in silenzio, e **in tutti e due i versi**. Una console variable dura quanto il processo
		// dell'editor: digitata una volta, resta attiva a ogni Play successivo. Il verso che spegne e' quello
		// che costa di piu' — un `BP_GameMode` configurato per la demo che parte come partita normale non
		// lascia alcuna traccia a schermo, perche' la banda tace proprio quando la modalita' e' spenta, e si
		// finisce a cercare il difetto nella proprieta' sbagliata. E' la stessa cura gia' presa da
		// `ResolveScenarioToRun` e `ResolveMapSource`; qui mancava.
		const bool bFromConsole = FromConsole != 0;
		if (bAutobattle != bFromConsole)
		{
			UE_LOG(LogRT, Warning,
				TEXT("[RT] La console variable rt.Match.Autobattle=%d SCAVALCA la proprieta' bAutobattle=%s "
					 "del GameMode. Per tornare a usare la proprieta': `rt.Match.Autobattle -1`."),
				FromConsole, bAutobattle ? TEXT("true") : TEXT("false"));
		}
		return bFromConsole;
	}

	// Sorgente di mezzo: l'unica che arriva anche in Shipping (vedi `RTAutobattleEntry`).
	const TOptional<bool> FromCmdLine = RTAutobattleEntry::FromCommandLine();
	if (FromCmdLine.IsSet())
	{
		if (bAutobattle != FromCmdLine.GetValue())
		{
			UE_LOG(LogRT, Warning,
				TEXT("[RT] La riga di comando -RTAutobattle SCAVALCA la proprieta' bAutobattle=%s del "
					 "GameMode: la partita e' %s. Per tornare a usare la proprieta': togli il flag."),
				bAutobattle ? TEXT("true") : TEXT("false"),
				FromCmdLine.GetValue() ? TEXT("non presidiata") : TEXT("normale"));
		}
		return FromCmdLine.GetValue();
	}

	return bAutobattle;
}

float ARTGameMode::ResolveMatchPlanningSeconds() const
{
	// Stessa scala di specificita' dell'altra configurazione. Il negativo qui significa «non impostata» in
	// ognuna delle tre sorgenti, perche' `0` e' un valore legittimo: turni incatenati, nessuna attesa.
	const float FromConsole = CVarRTPlanningSeconds.GetValueOnGameThread();
	if (FromConsole >= 0.f)
	{
		return FromConsole;
	}

	float FromCmdLine = 0.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("RTPlanningSeconds="), FromCmdLine) && FromCmdLine >= 0.f)
	{
		return FromCmdLine;
	}

	if (MatchPlanningSeconds >= 0.f)
	{
		return MatchPlanningSeconds;
	}

	// Quarto gradino, e vale SOLO a modalita' accesa: senza, l'autobattle erediterebbe i 30 s del
	// `TurnManager` e sarebbe acceso e inguardabile. In partita normale il timer resta di chi lo possiede.
	return ResolveAutobattle() ? RTAutobattleEntry::FallbackPlanningSeconds : -1.f;
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
		// ⚠️ **Questa funzione e' la banda dello STATO ANOMALO della sessione, non solo dello scenario**, e il
		// nome porta ancora il primo dei due casi che serve. E' deliberato: `UI/RTHUD.cpp` legge di qui, ed e'
		// nel `writable` di un'altra track (`client_tools`, #78) — rinominare il metodo significherebbe
		// toccare un file che questa sessione non possiede, per un guadagno che e' solo di nome. Chi apre
		// E47.2/E47.3, che quel file lo possiedono, ha qui la ragione per rinominarlo.
		//
		// Una sola banda per volta, e vince lo scenario: con `ScenarioToRun` valorizzato la partita normale
		// non viene allestita **affatto**, quindi non c'e' nessun autobattle da annunciare — dire entrambe le
		// cose descriverebbe una sessione che non esiste.
		// ⚠️ Legge lo stato LATCHATO, non il resolver, ed e' il punto in cui la differenza si vede: questa
		// funzione gira da `DrawHUD` a ogni fotogramma, e una console variable digitata a meta' partita
		// cambierebbe la risposta del resolver mentre le unita' gia' in campo restano come sono. La banda
		// descrive **la partita che si sta giocando**, non l'ultima cosa che qualcuno ha digitato.
		if (!bAutobattleInEffect)
		{
			return FString();
		}

		return FString::Printf(
			TEXT("AUTOBATTLE [%s]  -  entrambe le squadre al bot  -  nessun input richiesto"),
			*AutobattleSourceLabel);
	}

	// La FONTE va detta anche a schermo, per la stessa ragione per cui il log la dice: una console variable
	// impostata una volta scavalca la tendina a ogni Play successivo, e senza saperlo si cerca il difetto
	// nella property sbagliata.
	// ⚠️ Le etichette sono quelle che i test di `RTScenarioAutoRunTests.cpp` cercano dentro la banda: la
	// stringa e' il contratto, non un dettaglio di presentazione.
	const TCHAR* Source = TEXT("BP_GameMode");
	switch (RTScenarioEntry::Winner())
	{
	case RTScenarioEntry::EWinner::ConsoleVariable: Source = TEXT("rt.Test.Scenario"); break;
	case RTScenarioEntry::EWinner::CommandLine:     Source = TEXT("-RTScenario="); break;
	case RTScenarioEntry::EWinner::Property:        break;
	}

	FString Esito = TEXT("in corso");
	if (ScenarioSession.IsValid() && ScenarioSession->IsFinished())
	{
		Esito = ScenarioSession->GetResult().OutcomeString();
	}

	return FString::Printf(TEXT("SCENARIO %s [%s]  -  %s  -  la partita normale NON e' allestita"),
		*ScenarioId, Source, *Esito);
}

void ARTGameMode::ListenForLevelRequests(URTFrontendNavigator* Navigator)
{
	if (!Navigator)
	{
		// ⚠️ **Non e' un errore, e la differenza con `ARTFrontendGameMode` e' voluta.** Li' un navigatore
		// assente significa un menu che non aprirebbe niente; qui uno scenario headless o un test di
		// simulazione girano senza frontend, e la partita deve poter girare lo stesso.
		UE_LOG(LogRT, Verbose,
			TEXT("[RT] Partita senza frontend: nessuna richiesta di livello da raccogliere."));
		return;
	}

	// `AddUniqueDynamic`: `BeginPlay` puo' correre piu' di una volta sullo stesso GameMode in editor, e due
	// iscrizioni aprirebbero il livello due volte.
	Navigator->OnMatchRequested.AddUniqueDynamic(this, &ARTGameMode::HandleMatchRequested);
	Navigator->OnReturnToFrontendRequested.AddUniqueDynamic(this, &ARTGameMode::HandleReturnToFrontendRequested);
}

void ARTGameMode::HandleMatchRequested(const FString& LevelName)
{
	UGameInstance* GameInstance = GetGameInstance();
	URTFrontendNavigator* Navigator = GameInstance ? GameInstance->GetSubsystem<URTFrontendNavigator>() : nullptr;

	// Si CONSUMA, non si legge: una richiesta che resta li' fa rifiutare il `PLAY` successivo con
	// «mai consumata», che punta il dito su chi non consuma invece che su chi non si e' iscritto.
	const FString Consumed = Navigator ? Navigator->ConsumePendingMatchLevel() : FString();
	if (Consumed.IsEmpty())
	{
		// ⚠️ «di partita» e' il consumatore, non l'evento: `ARTFrontendGameMode` emette la riga gemella dal
		// mondo del menu, e senza questa distinzione il log non dice quale dei due ha parlato.
		UE_LOG(LogRT, Error,
			TEXT("[RT] PLAY AGAIN dal mondo di partita: annuncio per '%s' senza richiesta pendente, "
				 "nulla da aprire."), *LevelName);
		return;
	}

	UE_LOG(LogRT, Log, TEXT("[RT] PLAY AGAIN: riapertura del livello di partita '%s'"), *Consumed);
	OpenLevelByName(Consumed);
}

void ARTGameMode::HandleReturnToFrontendRequested(const FString& LevelName)
{
	UGameInstance* GameInstance = GetGameInstance();
	URTFrontendNavigator* Navigator = GameInstance ? GameInstance->GetSubsystem<URTFrontendNavigator>() : nullptr;

	const FString Consumed = Navigator ? Navigator->ConsumePendingFrontendLevel() : FString();
	if (Consumed.IsEmpty())
	{
		UE_LOG(LogRT, Error,
			TEXT("[RT] Annuncio di ritorno al menu per '%s' senza richiesta pendente: nulla da aprire."),
			*LevelName);
		return;
	}

	// ⛔ **Qui la partita finisce davvero, e non perche' qualcuno la spenga**: cambiare livello distrugge il
	// mondo, e con lui `ARTTurnManager`, le `ARTUnit` e questo stesso GameMode. E' il motivo per cui il DoD
	// puo' chiedere «nessuno stato vivo» invece di un elenco di cose da azzerare: cio' che vive nel mondo se
	// ne va con il mondo, e cio' che sopravvive — i subsystem della `GameInstance` — e' un elenco corto e
	// noto. Il navigatore l'ha gia' ripulito in `ReturnMain`, e i suoi widget li smonta `OnWorldCleanup`.
	UE_LOG(LogRT, Log, TEXT("[RT] RETURN TO MAIN MENU: smontaggio della partita, apertura di '%s'"), *Consumed);
	OpenLevelByName(Consumed);
}

void ARTGameMode::OpenLevelByName(const FString& LevelName)
{
	UGameplayStatics::OpenLevel(this, FName(*LevelName));
}

void ARTGameMode::HandleMatchEnded(const FRTMatchResult& Result, const FRTMatchState& State)
{
	UGameInstance* GameInstance = GetGameInstance();
	URTFrontendNavigator* Navigator = GameInstance ? GameInstance->GetSubsystem<URTFrontendNavigator>() : nullptr;
	if (!Navigator)
	{
		// ⚠️ Non e' un errore: uno scenario headless o un test di simulazione girano senza frontend, e la
		// partita deve poter finire lo stesso. Chi ha bisogno del Result e' il gioco, non il resolver.
		UE_LOG(LogRT, Verbose,
			TEXT("[RT] Partita finita senza frontend: nessuna schermata di Result da aprire."));
		return;
	}

	const ERTNavResult NavResult = Navigator->ShowResult(Result, State);
	if (NavResult != ERTNavResult::Ok)
	{
		UE_LOG(LogRT, Warning, TEXT("[RT] Fine partita: il Result non si e' aperto (%s)."),
			*UEnum::GetValueAsString(NavResult));
	}
}
