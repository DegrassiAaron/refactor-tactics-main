#include "RTGameMode.h"
#include "Camera/RTCameraPawn.h"
#include "Player/RTPlayerController.h"
#include "UI/RTHUD.h"
#include "Map/RTHexMapActor.h"
#include "Unit/RTUnit.h" // FClassFinder<ARTUnit> nel costruttore, e il tipo di `HeroUnitClasses`
#include "Turn/RTTurnManager.h"
#include "Frontend/RTFrontendNavigator.h"
#include "Frontend/RTMatchFrontendBridge.h" // la POLITICA del confine col frontend: qui resta il cablaggio
#include "Match/RTMatchBootstrapper.h"      // COME nasce una partita: qui resta il COSA e' stato chiesto
#include "ScenarioHarness/RTScenarioIndex.h"
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

	/**
	 * L'etichetta della fonte per il **log** dell'AUTO-RUN.
	 *
	 * ⚠️ Non e' quella della banda a schermo, e la differenza e' voluta: nel log la fonte si scrive per
	 * esteso perche' chi legge un log non ha il contesto, sulla banda si scrive corta perche' lo spazio e'
	 * quello di una riga. Le due tabelle restano due, ma leggono lo **stesso** `Winner()`.
	 */
	static const TCHAR* LogSourceLabel()
	{
		switch (Winner())
		{
		case EWinner::ConsoleVariable: return TEXT("console rt.Test.Scenario");
		case EWinner::CommandLine:     return TEXT("riga di comando -RTScenario=");
		case EWinner::Property:        break;
		}
		return TEXT("proprieta' del GameMode");
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
	 * L'etichetta della fonte, per il log dell'attivazione **e** per la banda a schermo.
	 *
	 * ⚠️ Una sola tabella qui, due in `RTScenarioEntry`, e la differenza non e' una svista: li' log e banda
	 * dicono la fonte con parole diverse — «riga di comando -RTScenario=» contro «-RTScenario=» — mentre qui
	 * dicono la stessa cosa. Due etichette identiche scritte due volte sarebbero il duplicato vero.
	 */
	static const TCHAR* SourceLabel()
	{
		switch (Winner())
		{
		case EWinner::ConsoleVariable: return TEXT("rt.Match.Autobattle");
		case EWinner::CommandLine:     return TEXT("-RTAutobattle");
		case EWinner::Property:        break;
		}
		return TEXT("BP_GameMode");
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
	{
		URTFrontendNavigator* Navigator = FRTMatchFrontendBridge::FindNavigator(GetGameInstance());
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
	// Il GameMode e' il posto giusto per questa DECISIONE: sceglie COSA allestire, che e' il suo mestiere —
	// e la precedenza fra le tre sorgenti resta sua, accanto a quelle di `MapSource` e dell'autobattle.
	// COME si esegue uno scenario lo sa `FRTScenarioCoordinator`: caricamento, sessione, avanzamento e
	// referto. Il resolver e il turn manager restano ignari dell'harness (nessun `if (IsTest)` nel gameplay).
	switch (ScenarioCoordinator.Start(World, ResolveScenarioToRun(),
		RTScenarioEntry::LogSourceLabel(), ScenarioTurnPauseSeconds))
	{
	case ERTScenarioStart::NotRequested:
		// Partita normale: si prosegue qui sotto.
		break;

	case ERTScenarioStart::NotLoadable:
		// 🔴 **Fail-closed, e la riga vale quanto il resto della funzione**: chi ha chiesto uno scenario che
		// non si carica non deve ritrovarsi a giocare una partita normale che non ha chiesto. Il motivo e'
		// gia' nel log del coordinatore.
		return;

	case ERTScenarioStart::Started:
		// Il Tick del GameMode esiste per questo e solo per questo: si accende quando c'e' qualcosa da far
		// avanzare, e una partita normale non lo paga.
		SetActorTickEnabled(true);
		RecenterCameraOnScenario();
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

	// 🔴 **Il consumatore del velo (`E13.8`), che a `#1467` mancava**: il meccanismo esisteva, era coperto
	// da cinque test, e nessuno lo chiamava — quindi in partita la board restava interamente disegnata e la
	// fog of war non si vedeva.
	HookKnowledgeVeil();
}

void ARTGameMode::HookKnowledgeVeil()
{
	ARTTurnManager* TurnManager =
		Cast<ARTTurnManager>(UGameplayStatics::GetActorOfClass(this, ARTTurnManager::StaticClass()));
	if (!TurnManager)
	{
		return;
	}

	// ⚠️ **`AddUniqueDynamic` e non `AddDynamic`**, per la ragione gia' registrata in
	// `Frontend/RTFrontendGameMode.cpp`: `BeginPlay` puo' correre piu' di una volta sullo stesso GameMode in
	// editor, e due iscrizioni stenderebbero il velo due volte per refresh — non un errore visivo, ma il
	// doppio del costo su ogni cella.
	//
	// 🔑 **Il sito di ISCRIZIONE sta nel GameMode, la fonte del TEAM no.** Qui perche' e' il GameMode a
	// spawnare il `TurnManager`, ed e' quindi l'unico punto in cui l'ordine e' garantito: un
	// `ARTPlayerController::BeginPlay` non ha nessuna garanzia di correre dopo. Il team invece lo risponde
	// `ViewerTeamId()`, che lo rilegge dal controller a ogni applicazione.
	TurnManager->OnTeamKnowledgeRefreshed.AddUniqueDynamic(
		this, &ARTGameMode::HandleTeamKnowledgeRefreshed);

	// 🔴 **E si stende SUBITO, non al primo refresh.** Senza questa riga la board nasce interamente visibile
	// e si vela al primo `RefreshTeamKnowledgeForPlanning`: il primo fotogramma e' quello che rivela tutta la
	// mappa, ed e' l'unico che nessun test potrebbe prendere dopo. E' una voce esplicita della DoD di `E13.8`.
	ApplyKnowledgeVeilForViewer();
}

int32 ARTGameMode::ViewerTeamId() const
{
	// Ripiego su `0` senza controller — headless, harness, test — con la stessa regola di
	// `ARTCameraPawn::FrameOwnTeam`, che il suo test pinna.
	if (const ARTPlayerController* PC = Cast<ARTPlayerController>(
			UGameplayStatics::GetPlayerController(this, 0)))
	{
		return PC->PlayerTeamId;
	}
	return 0;
}

void ARTGameMode::ApplyKnowledgeVeilForViewer()
{
	ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(GetWorld());
	if (!HexMap)
	{
		return;
	}

	ARTTurnManager* TurnManager =
		Cast<ARTTurnManager>(UGameplayStatics::GetActorOfClass(this, ARTTurnManager::StaticClass()));
	if (!TurnManager)
	{
		return;
	}

	// 🔑 **La via scelta e' `KnowledgeForTeamPublic`, e le altre due sono state scartate per ragioni
	// diverse** — `E13.8` chiede che la scelta sia dichiarata, non solo fatta:
	//
	//  - `MakeCurrentSnapshot` e' pubblica e consegnerebbe la conoscenza di ENTRAMBE le squadre, ma fa
	//    `GetAllActorsOfClass` e due `Sort` ed e' la sua parte cara: qui serve una squadra sola, a ogni
	//    refresh. Il suo stesso commento la dichiara «proibitiva a ogni frame».
	//  - **portare la conoscenza nel payload del delegate** sceglierebbe il team per conto di tutti i
	//    subscriber, cioe' farebbe alla firma esattamente cio' che [D-227] le vieta.
	//
	// ⚠️ `KnowledgeForTeamPublic` **non e' una `UFUNCTION`**, deliberatamente: esporla in Blueprint aprirebbe
	// un canale verso la conoscenza NON filtrata di una squadra qualunque. Da C++ va bene; da Blueprint la
	// porta resta `FRTKnowledgeView`.
	HexMap->ApplyKnowledgeVeil(TurnManager->KnowledgeForTeamPublic(ViewerTeamId()));

	// L'anello osservabile: `GetVeilCounts` dice com'e' la board, questo dice QUANTE VOLTE e' stata
	// ridipinta. Senza, «steso una volta sola» e «ridipinto con conoscenza vuota» sono indistinguibili.
	++KnowledgeVeilApplications;
}

void ARTGameMode::HandleTeamKnowledgeRefreshed(int32 /*TurnNumber*/)
{
	// Il numero di turno non serve: il velo non ha memoria e non interpola: ridipinge lo stato corrente.
	// Riceverlo e ignorarlo e' comunque giusto — e' la firma del delegate, non una scelta di questo sito.
	ApplyKnowledgeVeilForViewer();
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

void ARTGameMode::SetupHexMatch(ARTHexMapActor* HexMap)
{
	if (!HexMap)
	{
		return;
	}

	// CP 46.2: l'allestimento riparte da zero a ogni chiamata. Senza il reset, un secondo `SetupHexMatch`
	// nella stessa sessione — `Play Again` — accumulerebbe le note della partita precedente e il banner
	// mostrerebbe condizioni che non valgono piu'. Azzerarlo tocca al CHIAMANTE, perche' e' lui a sapere
	// quando una partita ricomincia: il bootstrapper allestisce una volta e non ha memoria.
	StartupReport.Reset();

	ARTTurnManager* TurnManager =
		Cast<ARTTurnManager>(UGameplayStatics::GetActorOfClass(this, ARTTurnManager::StaticClass()));

	// 🔴 **L'iscrizione all'annuncio di fine partita.** E' il punto in cui questo GameMode incontra il
	// `TurnManager`, quindi e' qui che si mette in ascolto: `BeginPlay` sarebbe troppo presto — il
	// `TurnManager` puo' non esistere ancora — e legarsi piu' tardi vorrebbe dire cercarlo una seconda volta.
	// `AddUniqueDynamic` perche' un allestimento ripetuto non deve aprire due Result.
	//
	// ⚠️ **Resta qui e non nel bootstrapper**: e' un delegate dinamico, e il receiver deve essere questo
	// `UObject`. E' la stessa riga di divisione dello Step precedente col frontend.
	if (TurnManager)
	{
		TurnManager->OnMatchEnded.AddUniqueDynamic(this, &ARTGameMode::HandleMatchEnded);
	}

	// 🔑 **COSA e' stato chiesto.** Le tre scale di precedenza del progetto — sorgente mappa, modalita' non
	// presidiata, ritmo del turno — si risolvono QUI e in nessun altro posto: hanno la stessa forma
	// (proprieta' < riga di comando < console), e tenerle vicine e' cio' che impedisce loro di divergere.
	// Il bootstrapper riceve **valori**, non sorgenti, e per questo un test puo' allestire una partita senza
	// toccare lo stato globale del processo.
	FRTMatchBootstrapConfig Config;
	Config.MapSource             = ResolveMapSource();
	Config.MapFixtureId          = CVarRTMapFixture.GetValueOnGameThread().TrimStartAndEnd();
	Config.DemoArenaRadius       = DemoArenaRadius;
	Config.MatchFormat           = MatchFormat;
	Config.ShippedFormatId       = ShippedFormatId;
	Config.Team0Heroes           = Team0Heroes;
	Config.Team1Heroes           = Team1Heroes;
	Config.HeroUnitClasses       = HeroUnitClasses;
	Config.bAutobattle           = ResolveAutobattle();
	Config.AutobattleSourceLabel = RTAutobattleEntry::SourceLabel();
	// ⚠️ **Dopo `ResolveAutobattle()`, e l'ordine non e' estetico**: il quarto gradino di questa scala e' *il
	// ripiego dell'autobattle*, quindi questa funzione chiama quella. Invertirle cambierebbe quante volte la
	// precedenza viene annunciata nel log, che e' un numero su cui i test asseriscono.
	Config.PlanningSeconds       = ResolveMatchPlanningSeconds();

	const FRTMatchBootstrapOutcome Outcome =
		FRTMatchBootstrapper::Bootstrap(HexMap, TurnManager, Config, StartupReport);

	// LA MODALITA' DELLA SESSIONE si scrive **solo se** l'allestimento e' arrivato a deciderla — vedi
	// `IsAutobattleInEffect()`. Con un formato non risolvibile non si allestisce niente, e latchare li'
	// farebbe annunciare alla banda un autobattle che non sta girando: e' la differenza fra cio' che si puo'
	// *chiedere* e cio' che la partita *e'*, ed e' la ragione per cui l'esito porta `bModeLatched` invece di
	// far scrivere il campo al bootstrapper.
	if (Outcome.bModeLatched)
	{
		bAutobattleInEffect = Outcome.bAutobattleInEffect;
		AutobattleSourceLabel = Config.AutobattleSourceLabel;
	}
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

	ScenarioCoordinator.Tick(DeltaSeconds);
}

void ARTGameMode::RecenterCameraOnScenario()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// INQUADRATURA. Il percorso dello scenario non passa da `SetupHexMatch`, dove la partita normale si
	// preoccupa di cio' che si vede: senza questo, la camera restava dove l'aveva lasciata il proprio
	// BeginPlay — troppo alta e fuori centro. E' presentazione, non simulazione: non tocca l'esito.
	//
	// ⚠️ **Sta nel GameMode e non nel coordinatore**, ed e' una scelta di ciclo di vita e non di dominio:
	// il timer va agganciato a un Actor vivo (`CreateWeakLambda` su `this`), e il coordinatore e' una classe
	// C++ pura senza vita propria da offrire al timer.
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
}

bool ARTGameMode::IsScenarioRunning() const
{
	return ScenarioCoordinator.IsRunning();
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

	// Vuoto = nessun verdetto ancora: e' un terzo stato, non un esito mancante. Vedi `OutcomeString()`.
	FString Esito = ScenarioCoordinator.OutcomeString();
	if (Esito.IsEmpty())
	{
		Esito = TEXT("in corso");
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
	// Adattatore sottile: il bridge dice COSA aprire, questo Actor sa COME — `OpenLevelByName` e' anche il
	// seam dei test, e vive qui perche' e' `virtual` su un `UObject`.
	const FString Level = FRTMatchFrontendBridge::ConsumeMatchLevel(GetGameInstance(), LevelName);
	if (!Level.IsEmpty())
	{
		OpenLevelByName(Level);
	}
}

void ARTGameMode::HandleReturnToFrontendRequested(const FString& LevelName)
{
	const FString Level = FRTMatchFrontendBridge::ConsumeFrontendLevel(GetGameInstance(), LevelName);
	if (!Level.IsEmpty())
	{
		OpenLevelByName(Level);
	}
}

void ARTGameMode::OpenLevelByName(const FString& LevelName)
{
	UGameplayStatics::OpenLevel(this, FName(*LevelName));
}

void ARTGameMode::HandleMatchEnded(const FRTMatchResult& Result, const FRTMatchState& State)
{
	// ⚠️ **Sta qui e non nel `TurnManager`** perche' questo e' l'unico punto che conosce sia il turno sia il
	// frontend: la simulazione annuncia il verdetto che ha gia' dato, e non deve sapere che esista una UI.
	// Cosa farne lo decide il bridge, che non ricalcola niente.
	FRTMatchFrontendBridge::ShowResult(GetGameInstance(), Result, State);
}
