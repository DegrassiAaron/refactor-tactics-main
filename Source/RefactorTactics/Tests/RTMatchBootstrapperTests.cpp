// COME NASCE UNA PARTITA, provato senza un GameMode e senza toccare lo stato globale del processo.
//
// I test esistenti dell'allestimento — `RTHeroSpawnTests`, `RTMatchFormatWorldTests`, `RTMatchSetupWorldTests`,
// `RTMatchAutobattleTests`, `RTStartupReportTests` — entrano da `ARTGameMode::SetupHexMatch`, cioe' dalla
// porta vera, e devono continuare a farlo: e' l'unico modo di sapere che il percorso di produzione funziona.
//
// Questo file guarda l'altra meta': che `FRTMatchBootstrapper` sia allestibile **da solo**, con una
// configurazione dichiarata invece che letta da console variable. Non e' un doppione — e' la prova che la
// linea «il GameMode risolve COSA, il bootstrapper costruisce COME» e' reale e non solo scritta nei commenti.
// Finche' l'allestimento leggeva `rt.Match.Autobattle` da se', un test della modalita' doveva sporcare una
// cvar che dura quanto l'editor, e dimenticarla accesa rompeva i test successivi in una unity build.

#include "Misc/AutomationTest.h"
#include "EngineUtils.h"
#include "Frontend/RTStartupReport.h"
#include "Map/RTHexMapActor.h"
#include "Match/RTMatchBootstrapper.h"
#include "Tests/RTWorldFixtures.h"
#include "Turn/RTMatchFormatData.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTBootstrapperTestsLocal
{
	/** La configurazione del vertical slice: arena generata, formato spedito, le due coppie di default. */
	FRTMatchBootstrapConfig MakeConfig()
	{
		FRTMatchBootstrapConfig Config;
		Config.MapSource = ERTMapSource::GeneratedDemoArena;
		Config.DemoArenaRadius = 4;
		Config.ShippedFormatId = FName(TEXT("Format.Skirmish2v2"));
		Config.Team0Heroes = { TEXT("Hero.Gadget"), TEXT("Hero.Phase") };
		Config.Team1Heroes = { TEXT("Hero.Riktor"), TEXT("Hero.Wraith") };
		return Config;
	}

	int32 CountBootstrappedUnits(UWorld* World, int32* OutBots = nullptr)
	{
		int32 N = 0;
		int32 Bots = 0;
		for (TActorIterator<ARTUnit> It(World); It; ++It)
		{
			++N;
			if (It->bIsBotControlled) { ++Bots; }
		}
		if (OutBots) { *OutBots = Bots; }
		return N;
	}
}

/**
 * L'allestimento non ha bisogno di un GameMode, e **nessuna console variable viene toccata**.
 *
 * 🔑 E' il punto dell'estrazione. La modalita' non presidiata arriva come un `bool` gia' risolto, quindi
 * questo test la esercita senza scrivere `rt.Match.Autobattle` — una cvar che dura quanto il processo
 * dell'editor e che, dimenticata accesa, cambia sotto i piedi i test che girano dopo nella stessa unity build.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBootstrapperNeedsNoConsoleVariablesTest,
	"RefactorTactics.Match.BootstrapperNeedsNoConsoleVariables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBootstrapperNeedsNoConsoleVariablesTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("mappa"), HexMap) || !TestNotNull(TEXT("TurnManager"), TM))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	FRTMatchBootstrapConfig Config = RTBootstrapperTestsLocal::MakeConfig();
	Config.bAutobattle = true;
	Config.AutobattleSourceLabel = TEXT("test");
	Config.PlanningSeconds = 1.f;

	// L'attivazione della modalita' e' rumorosa per costruzione: chi guarda deve sapere che non e' una
	// partita normale. L'avviso e' il comportamento voluto, quindi va dichiarato.
	AddExpectedError(TEXT("AUTOBATTLE"), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedError(TEXT("MapSource=GeneratedDemoArena"), EAutomationExpectedErrorFlags::Contains, 1);

	FRTStartupReport Report;
	const FRTMatchBootstrapOutcome Outcome = FRTMatchBootstrapper::Bootstrap(HexMap, TM, Config, Report);

	TestTrue(TEXT("la modalita' della sessione e' stata decisa"), Outcome.bModeLatched);
	TestTrue(TEXT("ed e' quella chiesta dalla configurazione, non da una cvar"), Outcome.bAutobattleInEffect);
	TestTrue(TEXT("le unita' sono entrate in campo"), Outcome.bUnitsSpawned);
	TestEqual(TEXT("l'allestimento e' arrivato in fondo"), Report.Phase, ERTLoadPhase::Ready);

	int32 Bots = 0;
	const int32 Units = RTBootstrapperTestsLocal::CountBootstrappedUnits(World, &Bots);
	TestEqual(TEXT("due squadre da due, come dichiara il formato"), Units, 4);
	TestEqual(TEXT("e in autobattle sono tutte del bot"), Bots, 4);

	// Il ritmo del turno arriva dalla configurazione e raggiunge il destinatario.
	TestEqual(TEXT("il Planning e' quello chiesto"), TM->GetPlanningSeconds(), 1.f);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **Formato non risolvibile ⇒ la modalita' NON viene latchata.**
 *
 * E' la ragione per cui l'esito porta `bModeLatched` invece di far scrivere il campo al bootstrapper. Con un
 * formato invalido non si allestisce niente: se la modalita' venisse decisa lo stesso, `IsAutobattleInEffect()`
 * direbbe di si' e la banda a schermo annuncerebbe un autobattle che non sta girando — un dato falso con
 * l'aria di essere aggiornato, che e' la famiglia di difetti che il rapporto d'avvio esiste per chiudere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBootstrapperDoesNotLatchOnFormatFailureTest,
	"RefactorTactics.Match.BootstrapperDoesNotLatchModeWhenFormatFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBootstrapperDoesNotLatchOnFormatFailureTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("mappa"), HexMap) || !TestNotNull(TEXT("TurnManager"), TM))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Limite di round non positivo: asset presente e invalido. Il ripiego copre l'ASSENZA, non il contenuto
	// sbagliato — sostituirlo in silenzio farebbe girare la partita con regole diverse da quelle scritte.
	URTMatchFormatData* Rotto = NewObject<URTMatchFormatData>();
	Rotto->FormatId = FName(TEXT("Format.BootstrapperTest"));
	Rotto->RoundLimit = 0;
	Rotto->ExpectedRounds = 0;
	Rotto->ScoreToWin = 0;

	FRTMatchBootstrapConfig Config = RTBootstrapperTestsLocal::MakeConfig();
	Config.MatchFormat = Rotto;
	Config.bAutobattle = true;
	Config.AutobattleSourceLabel = TEXT("test");

	AddExpectedError(TEXT("MapSource=GeneratedDemoArena"), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedError(TEXT("Formato di partita .* NON valido"), EAutomationExpectedErrorFlags::Contains, 1);

	FRTStartupReport Report;
	const FRTMatchBootstrapOutcome Outcome = FRTMatchBootstrapper::Bootstrap(HexMap, TM, Config, Report);

	TestFalse(TEXT("nessuna modalita' latchata su una partita che non esiste"), Outcome.bModeLatched);
	TestFalse(TEXT("e nessuna unita' in campo"), Outcome.bUnitsSpawned);
	TestEqual(TEXT("nessuna unita' allestita"), RTBootstrapperTestsLocal::CountBootstrappedUnits(World), 0);
	TestEqual(TEXT("nessuna regola applicata"), TM->GetMatchRules().RoundLimit, 0);
	// ⚠️ La fase resta dove ci si e' fermati, non torna a `Idle`: **dove** e' l'informazione che serve.
	TestEqual(TEXT("la fase dichiara dove si e' fermato"), Report.Phase, ERTLoadPhase::Scenario);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * FAIL-CLOSED sul roster: un `HeroId` inesistente non produce una lineup PARZIALE.
 *
 * Prima di `#1069` la guardia stava dentro il ciclo di spawn e faceva `continue`: un nome sbagliato produceva
 * una partita allestita a meta', con le unita' risolte in campo e le altre no — e a schermo sembrava una
 * partita normale. E' il difetto piu' caro da diagnosticare, perche' non ha sintomo.
 *
 * ⚠️ **Zero unita', non «meno unita'»**, ed e' la differenza che questo test misura: il conteggio esatto e'
 * l'unica asserzione che distingue «rifiutato» da «allestito a meta'».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBootstrapperFailsClosedOnUnknownHeroTest,
	"RefactorTactics.Match.BootstrapperFailsClosedOnUnknownHero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBootstrapperFailsClosedOnUnknownHeroTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("mappa"), HexMap) || !TestNotNull(TEXT("TurnManager"), TM))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	FRTMatchBootstrapConfig Config = RTBootstrapperTestsLocal::MakeConfig();
	// La SECONDA voce della prima squadra: cosi' la lineup ha gia' risolto un eroe valido quando incontra
	// quello sbagliato, ed e' esattamente lo stato da cui nasceva la partita a meta'.
	Config.Team0Heroes = { TEXT("Hero.Gadget"), TEXT("Hero.CheNonEsiste") };

	AddExpectedError(TEXT("MapSource=GeneratedDemoArena"), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedError(TEXT("non e' nel catalogo eroi"), EAutomationExpectedErrorFlags::Contains, 1);

	FRTStartupReport Report;
	const FRTMatchBootstrapOutcome Outcome = FRTMatchBootstrapper::Bootstrap(HexMap, TM, Config, Report);

	TestFalse(TEXT("nessuna unita' e' entrata in campo"), Outcome.bUnitsSpawned);
	TestEqual(TEXT("zero unita', non una lineup parziale"),
		RTBootstrapperTestsLocal::CountBootstrappedUnits(World), 0);

	// La condizione e' nel rapporto, non solo nel log: e' cio' che un widget puo' leggere.
	bool bDichiarato = false;
	for (const FRTStartupNote& Note : Report.Notes)
	{
		bDichiarato |= (Note.Outcome == ERTStartupOutcome::RosterHeroMissing);
	}
	TestTrue(TEXT("l'eroe mancante e' dichiarato nel rapporto d'avvio"), bDichiarato);

	// ⚠️ Il formato ERA valido, quindi la modalita' e' stata decisa: e' il caso opposto al test sopra, e
	// insieme dicono che `bModeLatched` segue il FORMATO e non l'esito complessivo dell'allestimento.
	TestTrue(TEXT("la modalita' era gia' stata decisa: il formato era valido"), Outcome.bModeLatched);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
