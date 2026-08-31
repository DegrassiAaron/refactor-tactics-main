// IL COMPAGNO PIANIFICATO DAL BOT (`rt.Match.BotAllies`).
//
// Fino a qui `bIsBotControlled` era vero per SQUADRA: o la squadra 1, o — con l'autobattle — tutte e due.
// Questi test coprono la prima configurazione in cui due unita' della STESSA squadra ricevono risposte
// diverse, che e' l'unico delta della feature: il planner, l'Intent, lo snapshot e il resolver restano
// quelli di sempre.
//
// ⚠️ **Perche' un file a parte e non `RTHeroSpawnTests.cpp`**: li' si misura la formazione SPEDITA, e quel
// contratto — «il giocatore comanda i suoi» — non cambia. Qui si misura cosa succede quando qualcuno chiede
// esplicitamente il contrario. Tenerli insieme farebbe sembrare il default una delle due varianti.
//
// ⛔ **Il determinismo NON si misura chiamando `PlanBots()` due volte sullo stesso mondo.** La seconda
// chiamata parte da una conoscenza di squadra che la prima ha gia' aggiornato
// (`RefreshTeamKnowledgeForPlanning`), quindi lo stato d'ingresso non e' lo stesso e un piano diverso non
// proverebbe indeterminismo. Due mondi costruiti identici rispondono alla domanda vera: *stesso stato ⇒
// stesso risultato*.

#include "Misc/AutomationTest.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTCellId.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "RTGameMode.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"

#if WITH_DEV_AUTOMATION_TESTS

/** Definite in RTGameMode.cpp: le due sorgenti «di adesso» che scavalcano la proprieta' del GameMode. */
extern TAutoConsoleVariable<int32> CVarRTBotAllies;
extern TAutoConsoleVariable<int32> CVarRTAutobattle;

namespace
{
	// ⚠️ Nomi tutti prefissati `BotAlly`: la unity build condivide la translation unit con gli altri file di
	// test, dove `MakeRosterWorld` e `SpawnTeamPlanningUnit` esistono gia' con un altro corpo.

	/**
	 * Riporta riga di comando e console variable com'erano, qualunque cosa succeda nel test.
	 *
	 * Stessa guardia e stessa ragione misurata di `RTAutobattleInputInertTests.cpp`: sono stato del
	 * PROCESSO, e uno sporco qui non fallisce in questo file ma nel prossimo, che in una unity build puo'
	 * essere qualsiasi cosa. Qui serve anche in ENTRATA: questi test decidono con la proprieta' del
	 * GameMode, e una CVar lasciata accesa da un'altra sessione deciderebbe al posto loro.
	 */
	struct FRTScopedBotAllyState
	{
		FString SavedCommandLine;
		int32 SavedBotAllies;
		int32 SavedAutobattle;

		FRTScopedBotAllyState()
			: SavedCommandLine(FCommandLine::Get())
			, SavedBotAllies(CVarRTBotAllies.GetValueOnGameThread())
			, SavedAutobattle(CVarRTAutobattle.GetValueOnGameThread())
		{
			CVarRTBotAllies->Set(-1, ECVF_SetByCode);
			CVarRTAutobattle->Set(-1, ECVF_SetByCode);
		}

		~FRTScopedBotAllyState()
		{
			FCommandLine::Set(*SavedCommandLine);
			CVarRTBotAllies->Set(SavedBotAllies, ECVF_SetByCode);
			CVarRTAutobattle->Set(SavedAutobattle, ECVF_SetByCode);
		}
	};

	UWorld* MakeBotAllyWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyBotAllyWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	TArray<ARTUnit*> CollectBotAllyUnits(UWorld* World)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Found);

		TArray<ARTUnit*> Units;
		for (AActor* Actor : Found)
		{
			if (ARTUnit* Unit = Cast<ARTUnit>(Actor)) { Units.Add(Unit); }
		}
		return Units;
	}

	ARTUnit* FindBotAllyHero(const TArray<ARTUnit*>& Units, const TCHAR* HeroId)
	{
		for (ARTUnit* Unit : Units)
		{
			if (Unit->HeroId == FName(HeroId)) { return Unit; }
		}
		return nullptr;
	}

	/**
	 * Allestisce la partita spedita con `BotAllyCount` compagni al bot, attraverso `SetupHexMatch`.
	 *
	 * Il percorso VERO e non una sua imitazione, per la ragione gia' scritta in `RTHeroSpawnTests`: un test
	 * che ricostruisse a mano cio' che fa `SpawnHero` resterebbe verde anche togliendo il cap.
	 */
	TArray<ARTUnit*> BootstrapWithBotAllies(UWorld* World, int32 BotAllyCount)
	{
		URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 4);
		ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
		if (!HexMap) { return TArray<ARTUnit*>(); }
		HexMap->MapAsset = Map;

		ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
		if (!GameMode) { return TArray<ARTUnit*>(); }
		GameMode->BotAllyCount = BotAllyCount;
		GameMode->SetupHexMatch(HexMap);

		return CollectBotAllyUnits(World);
	}
}

/**
 * IL GATE. Un compagno pianificato dal bot non e' comandabile, e la squadra da sola non basta a dirlo.
 *
 * ⚠️ Le due asserzioni sul team avversario ci sono per lo stesso motivo per cui `RTCombatLibraryTests` le
 * ha: senza, un predicato che rispondesse sempre `!bUnitIsBotControlled` sarebbe verde qui e avrebbe perso
 * la regola di squadra.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotAllyIsNotCommandableTest,
	"RefactorTactics.Bot.Ally.PlayerCannotControlBotTeammate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotAllyIsNotCommandableTest::RunTest(const FString&)
{
	TestTrue(TEXT("il giocatore comanda un proprio compagno umano"),
		URTCombatLibrary::CanPlayerControlUnit(0, 0, /*bUnitIsBotControlled=*/ false));
	TestFalse(TEXT("ma NON un proprio compagno pianificato dal bot"),
		URTCombatLibrary::CanPlayerControlUnit(0, 0, /*bUnitIsBotControlled=*/ true));

	// La regola di squadra sopravvive: il flag la restringe, non la sostituisce.
	TestFalse(TEXT("un'avversaria umana resta fuori comando"),
		URTCombatLibrary::CanPlayerControlUnit(1, 0, /*bUnitIsBotControlled=*/ false));
	TestFalse(TEXT("e un'avversaria del bot anche"),
		URTCombatLibrary::CanPlayerControlUnit(1, 0, /*bUnitIsBotControlled=*/ true));

	// Il default del parametro conserva il comportamento storico per chi chiama con due argomenti.
	TestTrue(TEXT("con due argomenti vale ancora la sola regola di squadra"),
		URTCombatLibrary::CanPlayerControlUnit(0, 0));

	return true;
}

/**
 * L'ALLESTIMENTO. `BotAllyCount = 1` su `[Gadget, Phase]` da' Phase al bot e lascia Gadget al giocatore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotAllyBootstrapTest,
	"RefactorTactics.Bot.Ally.BootstrapPutsLastTeammateUnderBot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotAllyBootstrapTest::RunTest(const FString&)
{
	FRTScopedBotAllyState Guard;

	UWorld* World = MakeBotAllyWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	const TArray<ARTUnit*> Units = BootstrapWithBotAllies(World, /*BotAllyCount=*/ 1);
	if (!TestEqual(TEXT("quattro unita' in campo"), Units.Num(), 4))
	{
		DestroyBotAllyWorld(World);
		return false;
	}

	ARTUnit* Gadget = FindBotAllyHero(Units, TEXT("Hero.Gadget"));
	ARTUnit* Phase = FindBotAllyHero(Units, TEXT("Hero.Phase"));
	ARTUnit* Riktor = FindBotAllyHero(Units, TEXT("Hero.Riktor"));
	ARTUnit* Wraith = FindBotAllyHero(Units, TEXT("Hero.Wraith"));
	if (!TestNotNull(TEXT("Gadget"), Gadget) || !TestNotNull(TEXT("Phase"), Phase)
		|| !TestNotNull(TEXT("Riktor"), Riktor) || !TestNotNull(TEXT("Wraith"), Wraith))
	{
		DestroyBotAllyWorld(World);
		return false;
	}

	// LA SQUADRA NON CAMBIA: Phase resta un'alleata, e questo e' il punto della feature. Se cambiasse
	// squadra sarebbe un'avversaria in piu', cioe' un'altra partita.
	TestEqual(TEXT("Phase resta nella squadra del giocatore"), Phase->TeamId, 0);
	TestEqual(TEXT("Gadget anche"), Gadget->TeamId, 0);

	// ...ma chi la pianifica si': e' la prima volta che due unita' della stessa squadra si dividono qui.
	TestFalse(TEXT("Gadget resta al giocatore"), Gadget->bIsBotControlled);
	TestTrue(TEXT("Phase passa al bot"), Phase->bIsBotControlled);
	TestTrue(TEXT("gli avversari restano al bot"), Riktor->bIsBotControlled && Wraith->bIsBotControlled);

	// E il gate lo vede: senza questa riga il test proverebbe l'assegnazione e non il suo effetto.
	TestTrue(TEXT("Gadget e' comandabile"),
		URTCombatLibrary::CanPlayerControlUnit(Gadget->TeamId, 0, Gadget->bIsBotControlled));
	TestFalse(TEXT("Phase no"),
		URTCombatLibrary::CanPlayerControlUnit(Phase->TeamId, 0, Phase->bIsBotControlled));

	DestroyBotAllyWorld(World);
	return true;
}

/**
 * IL DEFAULT NON CAMBIA. Senza configurazione la squadra 0 resta tutta di chi gioca.
 *
 * ⚠️ Duplica di proposito un'asserzione di `RTHeroSpawnTests`: li' e' una premessa della formazione
 * spedita, qui e' il criterio di NON-REGRESSIONE della feature. Il giorno in cui il default cambiasse per
 * sbaglio, i due file dicono cose diverse — quello che la partita e', e quello che questa feature ha
 * promesso di non toccare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotAllyDefaultIsZeroTest,
	"RefactorTactics.Bot.Ally.DefaultLeavesTeamZeroToThePlayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotAllyDefaultIsZeroTest::RunTest(const FString&)
{
	FRTScopedBotAllyState Guard;

	UWorld* World = MakeBotAllyWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	const TArray<ARTUnit*> Units = BootstrapWithBotAllies(World, /*BotAllyCount=*/ 0);
	if (!TestEqual(TEXT("quattro unita' in campo"), Units.Num(), 4))
	{
		DestroyBotAllyWorld(World);
		return false;
	}

	int32 UmaneInSquadraZero = 0;
	for (const ARTUnit* Unit : Units)
	{
		if (Unit && Unit->TeamId == 0 && !Unit->bIsBotControlled) { ++UmaneInSquadraZero; }
	}
	TestEqual(TEXT("senza configurazione la squadra 0 e' tutta del giocatore"), UmaneInSquadraZero, 2);

	DestroyBotAllyWorld(World);
	return true;
}

/**
 * IL CAP. Chiedere piu' compagni di quanti la formazione ne abbia non lascia il giocatore senza unita'.
 *
 * 🔴 E' il caso che rende la feature sicura: una squadra 0 interamente al bot con l'autobattle SPENTO
 * produce una partita che nessuno puo' chiudere — l'input non e' inerte, ma non c'e' niente da selezionare
 * e il turno si chiude solo a timer scaduto. Sembrerebbe un difetto del turno, non una configurazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotAllyCapTest,
	"RefactorTactics.Bot.Ally.CountIsCappedToLeaveOneCommandableUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotAllyCapTest::RunTest(const FString&)
{
	FRTScopedBotAllyState Guard;

	UWorld* World = MakeBotAllyWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	// Due su una formazione di due: piu' di quanti se ne possano concedere.
	const TArray<ARTUnit*> Units = BootstrapWithBotAllies(World, /*BotAllyCount=*/ 2);
	if (!TestEqual(TEXT("quattro unita' in campo"), Units.Num(), 4))
	{
		DestroyBotAllyWorld(World);
		return false;
	}

	int32 ComandabiliInSquadraZero = 0;
	for (const ARTUnit* Unit : Units)
	{
		if (Unit && Unit->TeamId == 0
			&& URTCombatLibrary::CanPlayerControlUnit(Unit->TeamId, 0, Unit->bIsBotControlled))
		{
			++ComandabiliInSquadraZero;
		}
	}
	TestEqual(TEXT("al giocatore resta esattamente un'unita' da comandare"), ComandabiliInSquadraZero, 1);

	// E la prima della formazione: il cap toglie dal FONDO, quindi il capo formazione e' l'ultimo a cadere.
	const ARTUnit* Gadget = FindBotAllyHero(Units, TEXT("Hero.Gadget"));
	if (TestNotNull(TEXT("Gadget"), Gadget))
	{
		TestFalse(TEXT("ed e' Gadget, primo in formazione"), Gadget->bIsBotControlled);
	}

	DestroyBotAllyWorld(World);
	return true;
}

/**
 * L'INTENT E IL DETERMINISMO, in una misura sola.
 *
 * Due mondi costruiti identici: il compagno bot produce un piano, ed e' lo STESSO piano nei due. La prima
 * meta' dimostra che partecipa alla pianificazione, la seconda che la sua scelta non dipende da nulla che
 * i due mondi non condividano.
 *
 * ⚠️ La destinazione dev'essere DIVERSA dalla cella di partenza, altrimenti «identiche» sarebbe verde anche
 * per due bot che non hanno pianificato niente — il caso in cui `PlanBots` esce subito lasciando il default
 * `PlannedCell = Cell`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotAllyDeterministicIntentTest,
	"RefactorTactics.Bot.Ally.PlansSameIntentFromSameState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotAllyDeterministicIntentTest::RunTest(const FString&)
{
	FRTScopedBotAllyState Guard;

	// Un mondo, il piano del compagno bot. Ripetuto due volte su mondi distinti ma identici.
	auto PianoDelCompagnoBot = [this](FRTCellId& OutPartenza, FRTCellId& OutPianificata,
		bool& bOutTrovato) -> UWorld*
	{
		bOutTrovato = false;
		UWorld* World = MakeBotAllyWorld();
		if (!World) { return nullptr; }

		const TArray<ARTUnit*> Units = BootstrapWithBotAllies(World, /*BotAllyCount=*/ 1);
		ARTUnit* Phase = FindBotAllyHero(Units, TEXT("Hero.Phase"));
		if (!Phase || !Phase->bIsBotControlled) { return World; }

		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM) { return World; }

		// Lo stesso ingresso della partita: `PlanBots()`, non una scorciatoia sul planner. Con una chiamata
		// diretta a `URTHexBotLibrary::PlanUnit` il test resterebbe verde anche se il TurnManager smettesse
		// di considerare il compagno — che e' proprio la cosa che questa feature cambia.
		TM->PlanBotsForTest();

		OutPartenza = Phase->Cell;
		OutPianificata = Phase->PlannedCell;
		bOutTrovato = true;
		return World;
	};

	FRTCellId PartenzaA, PianificataA, PartenzaB, PianificataB;
	bool bTrovatoA = false, bTrovatoB = false;

	UWorld* MondoA = PianoDelCompagnoBot(PartenzaA, PianificataA, bTrovatoA);
	UWorld* MondoB = PianoDelCompagnoBot(PartenzaB, PianificataB, bTrovatoB);

	if (TestTrue(TEXT("il compagno bot e' in campo in entrambi i mondi"), bTrovatoA && bTrovatoB))
	{
		TestTrue(TEXT("i due mondi partono dalla stessa cella"), PartenzaA == PartenzaB);

		// Ha pianificato davvero: un Intent di movimento, non il default «fermo».
		TestFalse(TEXT("il compagno bot ha scelto una destinazione diversa dalla propria cella"),
			PianificataA == PartenzaA);

		// E la scelta e' deterministica.
		TestTrue(TEXT("stesso stato, stessa destinazione"), PianificataA == PianificataB);
	}

	DestroyBotAllyWorld(MondoA);
	DestroyBotAllyWorld(MondoB);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
