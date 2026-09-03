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

/**
 * Senza mappa non nasce niente, e l'esito lo dice invece di lasciarlo indovinare.
 *
 * 🔴 **E' la prima guardia di `Bootstrap`, e non aveva un test.** Un chiamante che passa un `HexMap` nullo
 * riceve un `FRTMatchBootstrapOutcome` di default — `bModeLatched` falso, `bUnitsSpawned` falso — e non un
 * crash: e' cio' che permette a `ARTGameMode` di trattare l'assenza della mappa come una condizione, non
 * come un incidente. Senza questo test, sostituire il `return` con un `check()` passerebbe la suite.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBootstrapperFailsClosedWithoutHexMapTest,
	"RefactorTactics.Match.BootstrapperFailsClosedWithoutHexMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBootstrapperFailsClosedWithoutHexMapTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("TurnManager"), TM))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	FRTStartupReport Report;
	const FRTMatchBootstrapOutcome Outcome = FRTMatchBootstrapper::Bootstrap(
		/*HexMap=*/ nullptr, TM, RTBootstrapperTestsLocal::MakeConfig(), Report);

	TestFalse(TEXT("senza mappa la modalita' non si aggancia"), Outcome.bModeLatched);
	TestFalse(TEXT("senza mappa non si spawna nessuno"), Outcome.bUnitsSpawned);
	TestEqual(TEXT("e nel mondo non compare nessuna unita'"),
		RTBootstrapperTestsLocal::CountBootstrappedUnits(World), 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * Un'arena a raggio zero non e' una mappa piccola: e' una mappa assente, e la partita non si allestisce.
 *
 * 🔴 **Fail-closed su una causa DIVERSA da quella di `DoesNotLatchModeWhenFormatFails`.** Quel test rompe
 * il formato (`RoundLimit = 0`, asset presente e invalido); questo lascia il formato sano e toglie la
 * **mappa**, e il rifiuto arriva dalla validazione incrociata: «formato e mappa non combaciano: mappa
 * assente». Sono i due lati della stessa porta, e coprirne uno solo lascerebbe passare l'altro.
 *
 * ⚠️ **Il ramo `Start.Num() != CellsNeeded` NON e' quello che questo test attraversa, ed e' stato misurato.**
 * Con `GeneratedDemoArena` non e' raggiungibile: a raggio 0 l'arena ha **zero** celle e la validazione
 * formato/mappa esce prima; a raggio 1 ne ha sette, e il 2v2 ne chiede quattro. Per raggiungerlo servirebbe
 * una mappa popolata con celle **non percorribili** — `PickStartCells` filtra quelle — cioe' una fixture che
 * oggi non esiste. Il ramo resta scoperto, e questa nota e' la sua registrazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBootstrapperFailsClosedOnEmptyArenaTest,
	"RefactorTactics.Match.BootstrapperFailsClosedOnEmptyArena",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBootstrapperFailsClosedOnEmptyArenaTest::RunTest(const FString&)
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

	FRTMatchBootstrapConfig Vuota = RTBootstrapperTestsLocal::MakeConfig();
	Vuota.DemoArenaRadius = 0; // zero celle: non una mappa piccola, una mappa che non c'e'

	// Il rifiuto e' un `Error` dichiarato, e l'automation lo tratta come fallimento se non lo si attende.
	AddExpectedError(TEXT("Formato e mappa non combaciano"), EAutomationExpectedErrorFlags::Contains, 1);

	FRTStartupReport Report;
	const FRTMatchBootstrapOutcome Outcome = FRTMatchBootstrapper::Bootstrap(HexMap, TM, Vuota, Report);

	TestFalse(TEXT("mappa assente: la modalita' non si aggancia"), Outcome.bModeLatched);
	TestFalse(TEXT("mappa assente: nessuna unita' schierata"), Outcome.bUnitsSpawned);
	TestEqual(TEXT("e nel mondo non ne compare nessuna"),
		RTBootstrapperTestsLocal::CountBootstrappedUnits(World), 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * La formazione e il formato devono dire lo stesso numero, e chi non li allinea lo scopre subito.
 *
 * 🔴 **Non e' una preferenza di stile: e' il vincolo che tiene insieme due sorgenti indipendenti.**
 * `Rules.UnitsPerTeam` viene dal formato (un data asset o un formato spedito); `Team0Heroes`/`Team1Heroes`
 * vengono dalla configurazione del chiamante. Se divergono, `Bootstrap` esce senza allestire — e il log
 * nomina entrambe le vie di riparazione, «allinea `Team0Heroes` al formato, o il formato alla formazione».
 *
 * Il test schiera **un** eroe dove il formato ne chiede due, cioe' il caso che si presenta davvero: un
 * roster modificato a mano senza toccare il formato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBootstrapperNeedsFormationAlignedToFormatTest,
	"RefactorTactics.Match.BootstrapperNeedsFormationAlignedToFormat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBootstrapperNeedsFormationAlignedToFormatTest::RunTest(const FString&)
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

	FRTMatchBootstrapConfig Disallineata = RTBootstrapperTestsLocal::MakeConfig();
	Disallineata.Team0Heroes = { TEXT("Hero.Gadget") }; // uno solo, dove il formato ne chiede due

	// Il disallineamento e' un `Error` dichiarato: va atteso, o l'automation lo conta come fallimento.
	AddExpectedError(TEXT("la formazione della squadra 0 ne dichiara"),
		EAutomationExpectedErrorFlags::Contains, 1);

	FRTStartupReport Report;
	const FRTMatchBootstrapOutcome Outcome = FRTMatchBootstrapper::Bootstrap(HexMap, TM, Disallineata, Report);

	TestFalse(TEXT("formazione disallineata: non si schiera nessuno"), Outcome.bUnitsSpawned);
	TestEqual(TEXT("nemmeno la squadra che era allineata"),
		RTBootstrapperTestsLocal::CountBootstrappedUnits(World), 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * Su un livello con unita' proprie l'allestimento non interviene, **ma la modalita' si applica lo stesso**.
 *
 * 🔴 **E' la correzione di un difetto trovato in code review, e finora nessun test la teneva.** Il commento
 * al ramo lo racconta: il blocco del Planning era stato spostato sopra il `return` per questo scenario,
 * l'assegnazione di `bIsBotControlled` no — quindi il log dichiarava «entrambe le squadre al bot» mentre la
 * squadra 0 non pianificava nessuno, e la partita macinava turni vuoti fino al `RoundLimit`.
 *
 * ⚠️ **Non e' il doppione di `Match.Autobattle.AppliesToUnitsAlreadyInTheLevel`**, ed e' misurato: quello
 * entra da `ARTGameMode::SetupHexMatch` — la porta di produzione — e conta le unita' rimaste umane; questo
 * entra da `Bootstrap` e legge l'**esito**, cioe' i tre campi che il GameMode non guarda. E' la meta' che
 * il commento in testa a questo file dichiara di coprire: «che `FRTMatchBootstrapper` sia allestibile da
 * solo». La mutazione del 2026-09-03 li ha fatti cadere **entrambi**, ed e' la prova che guardano lo stesso
 * ramo da due porte diverse invece di ripetersi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBootstrapperAppliesAutobattleToUnitsAlreadyInTheLevelTest,
	"RefactorTactics.Match.BootstrapperAppliesAutobattleToUnitsAlreadyInTheLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBootstrapperAppliesAutobattleToUnitsAlreadyInTheLevelTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Posata = World->SpawnActor<ARTUnit>();
	if (!TestNotNull(TEXT("mappa"), HexMap) || !TestNotNull(TEXT("TurnManager"), TM)
		|| !TestNotNull(TEXT("unita' posata a mano"), Posata))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Premessa: l'unita' del livello NON e' del bot. Senza questa riga il test passerebbe anche se
	// l'assegnazione tornasse sotto il `return`, perche' il default potrebbe essere gia' quello giusto.
	Posata->bIsBotControlled = false;

	FRTMatchBootstrapConfig Autobattle = RTBootstrapperTestsLocal::MakeConfig();
	Autobattle.bAutobattle = true;
	Autobattle.AutobattleSourceLabel = TEXT("test");

	FRTStartupReport Report;
	const FRTMatchBootstrapOutcome Outcome = FRTMatchBootstrapper::Bootstrap(HexMap, TM, Autobattle, Report);

	TestTrue(TEXT("la modalita' si aggancia anche senza allestire"), Outcome.bModeLatched);
	TestTrue(TEXT("e l'autobattle risulta in effetto"), Outcome.bAutobattleInEffect);
	TestFalse(TEXT("l'allestimento automatico NON interviene"), Outcome.bUnitsSpawned);
	TestTrue(TEXT("l'unita' gia' nel livello e' passata al bot"), Posata->bIsBotControlled);
	TestEqual(TEXT("e resta l'unica nel mondo: nessuno spawn aggiuntivo"),
		RTBootstrapperTestsLocal::CountBootstrappedUnits(World), 1);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * Lo stesso eroe schierato due volte entra una volta sola, e la seconda copia non prende una cella.
 *
 * 🔴 **Il duplicato passa il controllo di allineamento**: due voci contro `UnitsPerTeam = 2` sono il numero
 * giusto, quindi il ramo della formazione non scatta e il difetto arriva fino allo spawn. La guardia sta
 * dentro il ciclo (`Spawned.Contains`) e fa `continue`, non `return`: la partita si allestisce, con
 * un'unita' in meno da un lato.
 *
 * ⚠️ E' un fail-**open** dichiarato, diverso dagli altri rami di questa suite: si sceglie una squadra
 * incompleta invece di nessuna partita. Il test lo pinna com'e', senza giudicarlo — se un giorno la scelta
 * cambiasse in un `return`, questo test cadrebbe e sarebbe la sede giusta per discuterlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBootstrapperIgnoresTheSecondCopyOfAHeroTest,
	"RefactorTactics.Match.BootstrapperIgnoresTheSecondCopyOfAHero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBootstrapperIgnoresTheSecondCopyOfAHeroTest::RunTest(const FString&)
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

	FRTMatchBootstrapConfig Doppia = RTBootstrapperTestsLocal::MakeConfig();
	Doppia.Team0Heroes = { TEXT("Hero.Gadget"), TEXT("Hero.Gadget") }; // due voci, un solo eroe

	FRTStartupReport Report;
	const FRTMatchBootstrapOutcome Outcome = FRTMatchBootstrapper::Bootstrap(HexMap, TM, Doppia, Report);

	TestTrue(TEXT("il duplicato non impedisce l'allestimento"), Outcome.bUnitsSpawned);
	TestEqual(TEXT("ma le unita' sono tre, non quattro: la seconda copia e' ignorata"),
		RTBootstrapperTestsLocal::CountBootstrappedUnits(World), 3);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
