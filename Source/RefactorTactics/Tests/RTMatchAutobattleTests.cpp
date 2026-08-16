// LA PARTITA CHE SI GUARDA: entrambe le squadre sotto il bot, per configurazione (CP 47.1, #954).
//
// Il motore non e' la novita' — `StartPlanningTimer` chiama gia' `PlanBots`, `OnPlanningTimeout` chiama
// `LockInAndResolve` e a fine risoluzione il timer riparte, quindi il turno avanza da solo dal 2026-08-06.
// Cio' che mancava e' **una configurazione**: `SpawnHero` metteva sotto il bot la sola squadra 1, e la 0
// restava ad aspettare una mano umana che in una demo non c'e'.
//
// ⚠️ QUESTI TEST NON DUPLICANO `RTHeroSpawnTests.cpp`. Quello pinna il comportamento PREDEFINITO — «il
// giocatore comanda i suoi», «il bot comanda i propri» — e deve restare verde: qui si copre solo cio' che
// la configurazione aggiunge sopra quel default, e il primo test lo verifica proprio *da questo lato*,
// perche' un'estensione che cambia il default sarebbe una regressione mascherata da feature.

#include "Misc/AutomationTest.h"
#include "RTWorldFixtures.h"
#include "RTGameMode.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnRules.h"
#include "Unit/RTUnit.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "HAL/IConsoleManager.h"

#if WITH_DEV_AUTOMATION_TESTS

/** Definite in RTGameMode.cpp: le due sorgenti «di adesso» della modalita' non presidiata. */
extern TAutoConsoleVariable<int32> CVarRTAutobattle;
extern TAutoConsoleVariable<float> CVarRTPlanningSeconds;

/**
 * Definita in ScenarioHarness/RTTestConsole.cpp.
 *
 * ⚠️ Serve **anche qui**, e non e' dominio estraneo: la banda a schermo e' una sola, e con uno scenario
 * impostato vince lo scenario. Un test del banner che non azzerasse questa console variable fallirebbe per
 * colpa del test precedente — e in una unity build il precedente puo' essere qualsiasi cosa.
 */
extern TAutoConsoleVariable<FString> CVarRTTestScenario;

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.

	/**
	 * Ripristina la riga di comando qualunque cosa succeda nel test.
	 *
	 * `FCommandLine` e' stato globale del PROCESSO: un flag lasciato sporco qui non fallisce in questo file,
	 * fallisce nel prossimo — e in una unity build il prossimo puo' essere qualsiasi cosa. E' la stessa
	 * guardia gia' scritta in `RTGameModeScenarioEntryTests.cpp`, per la stessa ragione misurata.
	 */
	struct FRTScopedAutobattleCommandLine
	{
		FString Saved;
		FRTScopedAutobattleCommandLine() : Saved(FCommandLine::Get()) {}
		~FRTScopedAutobattleCommandLine() { FCommandLine::Set(*Saved); }

		/**
		 * ⚠️ **`Set`, non `Append`: riparte sempre dalla riga salvata e ne tiene UNO alla volta.**
		 * Si chiamava `Append` e non appendeva — chi avesse reso vero il nome (accumulando invece di
		 * ripartire da `Saved`) avrebbe cambiato cosa misurano tre assertion **senza far diventare rosso
		 * niente**: con `-RTAutobattle -RTAutobattle=0` sulla stessa riga vince comunque `FParse::Value`, e
		 * il test continuerebbe a passare per un motivo diverso da quello scritto. Trovato in code review.
		 */
		void SetArgs(const TCHAR* Args) { FCommandLine::Set(*(Saved + FString(TEXT(" ")) + Args)); }
		void Clear() { FCommandLine::Set(*Saved); }
	};

	/** Come sopra, per le console variable: stesso motivo, stato che sopravvive al test. */
	struct FRTScopedAutobattleCVars
	{
		int32 SavedMode;
		float SavedPlanning;
		FString SavedScenario;
		FRTScopedAutobattleCVars()
			: SavedMode(CVarRTAutobattle.GetValueOnGameThread())
			, SavedPlanning(CVarRTPlanningSeconds.GetValueOnGameThread())
			, SavedScenario(CVarRTTestScenario.GetValueOnGameThread()) {}
		~FRTScopedAutobattleCVars()
		{
			CVarRTAutobattle->Set(SavedMode, ECVF_SetByCode);
			CVarRTPlanningSeconds->Set(SavedPlanning, ECVF_SetByCode);
			CVarRTTestScenario->Set(*SavedScenario, ECVF_SetByCode);
		}
		void SetMode(int32 Value) { CVarRTAutobattle->Set(Value, ECVF_SetByCode); }
		void SetPlanning(float Value) { CVarRTPlanningSeconds->Set(Value, ECVF_SetByCode); }
		/** Nessuno scenario in corso: la banda deve poter parlare della partita, non di una run di test. */
		void ClearScenario() { CVarRTTestScenario->Set(TEXT(""), ECVF_SetByCode); }
	};

	/** Mappa esagonale piena: abbastanza celle percorribili per le quattro posizioni di partenza. */
	ARTHexMapActor* SpawnAutobattleMap(UWorld* World, int32 Radius = 5)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	TArray<ARTUnit*> CollectAutobattleUnits(UWorld* World)
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

	/** Quante unita' in campo NON sono comandate dal bot: zero e' la definizione di «non presidiata». */
	int32 CountAutobattleHumanUnits(UWorld* World)
	{
		int32 Human = 0;
		for (const ARTUnit* Unit : CollectAutobattleUnits(World))
		{
			if (!Unit->bIsBotControlled) { ++Human; }
		}
		return Human;
	}
}

/**
 * IL DEFAULT NON CAMBIA: senza configurazione la squadra 0 resta del giocatore.
 *
 * E' la controprova che rende significative tutte le altre. Un'implementazione che mettesse tutti sotto il
 * bot passerebbe ogni test «l'autobattle funziona» e romperebbe il gioco — e i due test di
 * `RTHeroSpawnTests` che pinnano il default sono in un altro file, quindi il legame va scritto qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattleDefaultTest,
	"RefactorTactics.Match.Autobattle.DefaultLeavesPlayerTeamHuman",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattleDefaultTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	FRTScopedAutobattleCVars CVarGuard;
	FRTScopedAutobattleCommandLine CmdGuard;
	CVarGuard.SetMode(-1);
	CmdGuard.Clear();

	ARTHexMapActor* HexMap = SpawnAutobattleMap(World);
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	TestFalse(TEXT("nessuna delle tre sorgenti -> partita normale"), GameMode->ResolveAutobattle());

	GameMode->SetupHexMatch(HexMap);

	const TArray<ARTUnit*> Units = CollectAutobattleUnits(World);
	TestEqual(TEXT("quattro unita' in campo"), Units.Num(), 4);
	TestEqual(TEXT("due unita' restano del giocatore"), CountAutobattleHumanUnits(World), 2);
	for (const ARTUnit* Unit : Units)
	{
		TestEqual(FString::Printf(TEXT("%s: comando secondo la squadra"), *Unit->HeroId.ToString()),
			Unit->bIsBotControlled, Unit->TeamId == 1);
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * La PROPRIETA' del GameMode mette entrambe le squadre sotto il bot.
 *
 * E' la configurazione persistente — si imposta una volta nei default di `BP_GameMode` e ogni Play e' una
 * demo — ed e' la sola delle tre sorgenti che sopravvive alla sessione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattlePropertyTest,
	"RefactorTactics.Match.Autobattle.PropertyPutsBothTeamsUnderBot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattlePropertyTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	FRTScopedAutobattleCVars CVarGuard;
	FRTScopedAutobattleCommandLine CmdGuard;
	CVarGuard.SetMode(-1);
	CmdGuard.Clear();

	ARTHexMapActor* HexMap = SpawnAutobattleMap(World);
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	GameMode->bAutobattle = true;
	TestTrue(TEXT("la proprieta' da sola attiva la modalita'"), GameMode->ResolveAutobattle());

	GameMode->SetupHexMatch(HexMap);

	const TArray<ARTUnit*> Units = CollectAutobattleUnits(World);
	TestEqual(TEXT("quattro unita' in campo"), Units.Num(), 4);
	TestEqual(TEXT("nessuna unita' aspetta una mano umana"), CountAutobattleHumanUnits(World), 0);

	// Le squadre restano DUE: mettere tutti sotto il bot non deve collassare i team, altrimenti la
	// condizione di vittoria («una squadra e' stata eliminata») non avrebbe piu' due lati.
	TSet<int32> Teams;
	for (const ARTUnit* Unit : Units) { Teams.Add(Unit->TeamId); }
	TestEqual(TEXT("le due squadre restano distinte"), Teams.Num(), 2);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * LE TRE SORGENTI e il loro ordine: proprieta' < `-RTAutobattle` < `rt.Match.Autobattle`.
 *
 * E' la stessa regola gia' in vigore per lo scenario (`ResolveScenarioToRun`), e per la stessa ragione
 * applicata al tempo: la console si digita a meta' sessione, quindi deve poter scavalcare cio' che l'avvio
 * aveva chiesto.
 *
 * ⚠️ Il caso che conta piu' degli altri e' l'ultimo: la console deve poter **spegnere** una proprieta'
 * accesa. Una precedenza che sa solo accendere e' meta' precedenza, e obbligherebbe a modificare un
 * `.uasset` per giocare una partita normale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattlePrecedenceTest,
	"RefactorTactics.Match.Autobattle.SourcePrecedence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattlePrecedenceTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode)) { RTWorldFixtures::DestroyWorld(World); return false; }

	FRTScopedAutobattleCVars CVarGuard;
	FRTScopedAutobattleCommandLine CmdGuard;
	CVarGuard.SetMode(-1);
	CmdGuard.Clear();

	// Niente di niente: partita normale.
	GameMode->bAutobattle = false;
	TestFalse(TEXT("niente proprieta', niente flag, niente console -> partita normale"),
		GameMode->ResolveAutobattle());

	// Solo il flag, nella forma nuda: e' quella che si scrive istintivamente, ed e' il caso del pacchetto
	// dove la proprieta' e' quella spedita. Se non funzionasse fallirebbe in SILENZIO — il difetto misurato
	// di #926, dove `-dpcvars` non arriva in Shipping e nessuno lo dice.
	CmdGuard.SetArgs(TEXT("-RTAutobattle"));
	TestTrue(TEXT("il flag nudo da solo attiva"), GameMode->ResolveAutobattle());

	// E nella forma con valore, che e' quella simmetrica a `-RTScenario=`.
	CmdGuard.SetArgs(TEXT("-RTAutobattle=0"));
	TestFalse(TEXT("il flag con valore 0 spegne"), GameMode->ResolveAutobattle());
	CmdGuard.SetArgs(TEXT("-RTAutobattle=1"));
	TestTrue(TEXT("il flag con valore 1 accende"), GameMode->ResolveAutobattle());

	// Flag contro proprieta': vince il flag, che e' l'intento di QUESTO avvio.
	GameMode->bAutobattle = false;
	TestTrue(TEXT("flag acceso + proprieta' spenta -> vince il flag"), GameMode->ResolveAutobattle());

	// Console contro flag: vince la console, che e' l'intento di ADESSO.
	CVarGuard.SetMode(0);
	TestFalse(TEXT("console spenta + flag acceso -> vince la console"), GameMode->ResolveAutobattle());

	// E la console spegne anche la proprieta': e' il caso che rende la precedenza utile in editor.
	GameMode->bAutobattle = true;
	CmdGuard.Clear();
	TestFalse(TEXT("console spenta + proprieta' accesa -> vince la console"), GameMode->ResolveAutobattle());

	// Tolta la console, la proprieta' torna a valere: la precedenza non l'ha consumata.
	CVarGuard.SetMode(-1);
	TestTrue(TEXT("tolta la console, vale di nuovo la proprieta'"), GameMode->ResolveAutobattle());

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * I SECONDI DI PLANNING arrivano davvero al TurnManager, e con la stessa precedenza.
 *
 * `PlanningSeconds` vale **30** di default: una demo che aspetta mezzo minuto fra un turno e l'altro non e'
 * guardabile, ed e' il motivo per cui il DoD di #954 chiede questa configurazione accanto all'altra. Il
 * ripiego dell'autobattle esiste perche' senza di lui la modalita' sarebbe accesa e inutilizzabile, cioe'
 * il difetto sarebbe *dentro* la feature che lo introduce.
 *
 * ⚠️ Il test verifica il valore **sul TurnManager**, non il ritorno del resolver: e' la catena
 * *dichiarato → trasportato → letto*, e i due anelli precedenti possono essere giusti mentre il terzo non
 * esiste. `SetPlanningSeconds` e' gia' pinnata da `RefactorTactics.Turn.PlanningSecondsAppliesImmediately`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattlePlanningSecondsTest,
	"RefactorTactics.Match.Autobattle.PlanningSecondsReachTheTurnManager",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattlePlanningSecondsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	FRTScopedAutobattleCVars CVarGuard;
	FRTScopedAutobattleCommandLine CmdGuard;
	CVarGuard.SetMode(-1);
	CVarGuard.SetPlanning(-1.f);
	CmdGuard.Clear();

	ARTHexMapActor* HexMap = SpawnAutobattleMap(World);
	ARTTurnManager* TurnManager = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("TurnManager"), TurnManager)
		|| !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const float Default = TurnManager->GetPlanningSeconds();
	TestTrue(TEXT("il default del TurnManager e' un'attesa da partita umana"), Default > 10.f);

	// Partita normale: nessuno tocca il timer. Un autobattle che accorcia il Planning anche quando non e'
	// attivo cambierebbe il gioco di tutti per comodita' della demo.
	GameMode->bAutobattle = false;
	GameMode->SetupHexMatch(HexMap);
	TestEqual(TEXT("partita normale -> il Planning resta quello del TurnManager"),
		TurnManager->GetPlanningSeconds(), Default);

	// Autobattle senza altra configurazione: il ripiego dichiarato, non 30 secondi.
	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(HexMap);
	const float Unattended = TurnManager->GetPlanningSeconds();
	TestTrue(TEXT("autobattle -> il Planning si accorcia"), Unattended < Default);
	TestTrue(TEXT("e resta non negativo"), Unattended >= 0.f);

	// La proprieta' scavalca il ripiego...
	GameMode->MatchPlanningSeconds = 7.f;
	GameMode->SetupHexMatch(HexMap);
	TestEqual(TEXT("la proprieta' decide i secondi di Planning"), TurnManager->GetPlanningSeconds(), 7.f);

	// ...la riga di comando scavalca la proprieta'...
	CmdGuard.SetArgs(TEXT("-RTPlanningSeconds=3"));
	GameMode->SetupHexMatch(HexMap);
	TestEqual(TEXT("la riga di comando scavalca la proprieta'"), TurnManager->GetPlanningSeconds(), 3.f);

	// ...e la console scavalca tutto, con la stessa regola dell'altra configurazione.
	CVarGuard.SetPlanning(1.5f);
	GameMode->SetupHexMatch(HexMap);
	TestEqual(TEXT("la console scavalca la riga di comando"), TurnManager->GetPlanningSeconds(), 1.5f);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * L'ATTIVAZIONE NON E' SILENZIOSA: la banda a schermo dichiara che questa non e' una partita normale.
 *
 * Il sintomo non punta alla causa. Con l'autobattle acceso chi guarda vede le proprie unita' muoversi da
 * sole e non ha modo di sapere perche' — la spiegazione c'e', ma e' in una riga di Output Log che non si ha
 * motivo di andare a cercare. E' la stessa ragione, e lo stesso canale, di `GetScenarioBannerText`.
 *
 * ⚠️ La banda nomina la SORGENTE, come gia' fa per lo scenario: una console variable impostata una volta
 * resta attiva per ogni Play successivo, e senza saperlo si cerca il difetto nella property sbagliata.
 *
 * 🔴 **E la banda descrive la partita CHE SI STA GIOCANDO, non l'ultima cosa digitata.** `DrawHUD` la
 * ridisegna a ogni fotogramma mentre `bIsBotControlled` e' scritto una volta sola allo spawn: leggendo il
 * resolver, una console variable cambiata a meta' sessione avrebbe fatto comparire «AUTOBATTLE» su una
 * partita con la squadra 0 ancora umana — e sparire la banda da una partita in cui i bot continuano a
 * giocare. Trovato in code review; l'ultimo blocco di questo test copre entrambi i versi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattleBannerTest,
	"RefactorTactics.Match.Autobattle.BannerDeclaresUnattendedMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattleBannerTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	FRTScopedAutobattleCVars CVarGuard;
	FRTScopedAutobattleCommandLine CmdGuard;
	CVarGuard.SetMode(-1);
	CVarGuard.ClearScenario();
	CmdGuard.Clear();

	ARTHexMapActor* HexMap = SpawnAutobattleMap(World);
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	GameMode->ScenarioToRun.Reset();

	// Partita normale: nessuna banda. Una banda sempre accesa e' rumore, e il rumore si impara a ignorare.
	GameMode->bAutobattle = false;
	GameMode->SetupHexMatch(HexMap);
	TestTrue(TEXT("partita normale -> nessuna banda"), GameMode->GetScenarioBannerText().IsEmpty());

	// Dalla proprieta': la banda c'e' e attribuisce al BP_GameMode.
	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(HexMap);
	const FString FromProperty = GameMode->GetScenarioBannerText();
	TestTrue(TEXT("autobattle -> la banda c'e'"), !FromProperty.IsEmpty());
	TestTrue(TEXT("e dice che non e' una partita normale"), FromProperty.Contains(TEXT("AUTOBATTLE")));
	TestTrue(TEXT("e attribuisce alla proprieta'"), FromProperty.Contains(TEXT("BP_GameMode")));

	// Dalla console: stessa banda, sorgente diversa. E' l'unico punto in cui l'utente vede da dove viene la
	// modalita', e una banda che nomina la fonte sbagliata e' peggio di una banda assente.
	CVarGuard.SetMode(1);
	GameMode->SetupHexMatch(HexMap);
	const FString FromConsole = GameMode->GetScenarioBannerText();
	TestTrue(TEXT("la banda nomina la console"), FromConsole.Contains(TEXT("rt.Match.Autobattle")));
	TestFalse(TEXT("e non attribuisce al BP_GameMode"), FromConsole.Contains(TEXT("BP_GameMode")));

	// LA BANDA NON MENTE A META' PARTITA. Spenta la console DOPO l'allestimento, le unita' in campo restano
	// del bot: la banda deve continuare a dichiararlo, perche' descrive la partita e non la richiesta.
	CVarGuard.SetMode(0);
	TestFalse(TEXT("il resolver ora direbbe di no"), GameMode->ResolveAutobattle());
	TestTrue(TEXT("ma la sessione E' non presidiata"), GameMode->IsAutobattleInEffect());
	TestTrue(TEXT("e la banda resta quella della partita in corso"),
		GameMode->GetScenarioBannerText().Contains(TEXT("AUTOBATTLE")));
	TestEqual(TEXT("le unita' non sono cambiate sotto i piedi"), CountAutobattleHumanUnits(World), 0);

	// E il verso opposto: acceso a meta' di una partita normale, la banda non deve annunciare una demo che
	// non e' in corso — le unita' della squadra 0 sono ancora di chi gioca.
	ARTGameMode* Normale = World->SpawnActor<ARTGameMode>();
	if (TestNotNull(TEXT("secondo GameMode"), Normale))
	{
		CVarGuard.SetMode(-1);
		Normale->bAutobattle = false;
		Normale->SetupHexMatch(HexMap);   // il livello ha gia' le unita': nessun nuovo allestimento
		CVarGuard.SetMode(1);
		TestTrue(TEXT("il resolver ora direbbe di si'"), Normale->ResolveAutobattle());
		TestFalse(TEXT("ma questa sessione non e' partita come demo"), Normale->IsAutobattleInEffect());
		TestTrue(TEXT("e la banda tace"), Normale->GetScenarioBannerText().IsEmpty());
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * ZERO SECONDI DI PLANNING NON SONO «TURNI INCATENATI»: sono la partita ferma per sempre.
 *
 * 🔴 `SetPlanningSeconds` e `StartPlanningTimer` armano il timer solo `if (PlanningSeconds > 0.f)`. Con zero
 * non lo arma nessuno, `OnPlanningTimeout` non scatta, `LockInAndResolve` non viene chiamato — e in una
 * partita non presidiata non esiste una mano che possa chiudere il turno. La partita resta al turno 1
 * mentre la banda dichiara che si sta giocando da sola. Il commento di questa stessa PR annunciava zero
 * come valore legittimo: lo e' in `RTScenarioSession`, dove il turno lo pompa l'harness.
 *
 * ⚠️ In partita NORMALE zero resta zero, e non e' una svista: li' il lock-in lo preme chi gioca, e alzare il
 * valore cambierebbe il gioco di tutti per un problema che ha solo la demo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattleNoStallTest,
	"RefactorTactics.Match.Autobattle.PlanningSecondsNeverStallAnUnattendedMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattleNoStallTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	FRTScopedAutobattleCVars CVarGuard;
	FRTScopedAutobattleCommandLine CmdGuard;
	CVarGuard.SetMode(-1);
	CVarGuard.SetPlanning(-1.f);
	CmdGuard.Clear();

	ARTHexMapActor* HexMap = SpawnAutobattleMap(World);
	ARTTurnManager* TurnManager = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("TurnManager"), TurnManager)
		|| !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Zero da ognuna delle tre sorgenti, con l'autobattle acceso: nessuna delle tre puo' fermare il turno.
	GameMode->bAutobattle = true;

	GameMode->MatchPlanningSeconds = 0.f;
	GameMode->SetupHexMatch(HexMap);
	TestTrue(TEXT("proprieta' a 0 -> il timer resta armabile"), TurnManager->GetPlanningSeconds() > 0.f);

	GameMode->MatchPlanningSeconds = -1.f;
	CmdGuard.SetArgs(TEXT("-RTPlanningSeconds=0"));
	GameMode->SetupHexMatch(HexMap);
	TestTrue(TEXT("riga di comando a 0 -> idem"), TurnManager->GetPlanningSeconds() > 0.f);

	CmdGuard.Clear();
	CVarGuard.SetPlanning(0.f);
	GameMode->SetupHexMatch(HexMap);
	TestTrue(TEXT("console a 0 -> idem"), TurnManager->GetPlanningSeconds() > 0.f);

	// ...e resta comunque il valore piu' basso che l'orologio sa far scattare, non il ripiego di 2 s:
	// l'intento «il piu' veloce possibile» va onorato, non sostituito.
	TestTrue(TEXT("l'intento resta onorato: molto meno del ripiego"), TurnManager->GetPlanningSeconds() < 1.f);

	// In partita NORMALE zero e' legittimo e non viene toccato: c'e' chi preme il lock-in.
	GameMode->bAutobattle = false;
	CVarGuard.SetMode(0);
	GameMode->SetupHexMatch(HexMap);
	TestEqual(TEXT("partita normale -> 0 resta 0"), TurnManager->GetPlanningSeconds(), 0.f);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * `-RTAutobattle=true` ACCENDE. Sembra ovvio, e con l'overload sbagliato faceva l'opposto.
 *
 * 🔴 `FParse::Value(..., int32&)` passa da `FCString::Atoi`, e `Atoi("true")` vale **0**: la forma che si
 * scrive per prima avrebbe SPENTO la modalita', scavalcando in silenzio una proprieta' accesa. E su questa
 * sorgente non c'e' rete di sicurezza — esiste apposta per il pacchetto **Shipping**, dove la console non
 * arriva e non c'e' modo di accorgersi che il flag e' stato letto al contrario.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattleFlagValueTest,
	"RefactorTactics.Match.Autobattle.CommandLineValueIsUnderstood",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattleFlagValueTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode)) { RTWorldFixtures::DestroyWorld(World); return false; }

	FRTScopedAutobattleCVars CVarGuard;
	FRTScopedAutobattleCommandLine CmdGuard;
	CVarGuard.SetMode(-1);
	GameMode->bAutobattle = false;

	for (const TCHAR* Acceso : { TEXT("-RTAutobattle=1"), TEXT("-RTAutobattle=true"),
								 TEXT("-RTAutobattle=True"), TEXT("-RTAutobattle=on") })
	{
		CmdGuard.SetArgs(Acceso);
		TestTrue(FString::Printf(TEXT("%s accende"), Acceso), GameMode->ResolveAutobattle());
	}

	GameMode->bAutobattle = true;
	for (const TCHAR* Spento : { TEXT("-RTAutobattle=0"), TEXT("-RTAutobattle=false"),
								 TEXT("-RTAutobattle=off") })
	{
		CmdGuard.SetArgs(Spento);
		TestFalse(FString::Printf(TEXT("%s spegne"), Spento), GameMode->ResolveAutobattle());
	}

	// Un valore che non si capisce NON decide: ripiega sulla proprieta' e lo dichiara nel log. Silenzio e
	// «off» sono le due risposte peggiori, perche' somigliano a una scelta.
	GameMode->bAutobattle = true;
	CmdGuard.SetArgs(TEXT("-RTAutobattle=banana"));
	TestTrue(TEXT("un valore incomprensibile lascia decidere la proprieta'"), GameMode->ResolveAutobattle());

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * LA MODALITA' RAGGIUNGE ANCHE LE UNITA' GIA' POSATE NEL LIVELLO.
 *
 * 🔴 `SpawnHero` e' l'unico sito che scrive `bIsBotControlled`, e su un livello che porta le proprie unita'
 * `SetupHexMatch` ritorna **prima** di arrivarci: il log dichiarava «entrambe le squadre al bot» mentre la
 * squadra 0 restava con il valore cotto nel `.umap`, nessuno pianificava per lei e la partita macinava turni
 * vuoti fino al `RoundLimit`. Trovato in code review — il blocco del Planning era gia' stato spostato sopra
 * quel ritorno *per questo stesso scenario*, l'assegnazione no.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattleExistingUnitsTest,
	"RefactorTactics.Match.Autobattle.AppliesToUnitsAlreadyInTheLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattleExistingUnitsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	FRTScopedAutobattleCVars CVarGuard;
	FRTScopedAutobattleCommandLine CmdGuard;
	CVarGuard.SetMode(-1);
	CmdGuard.Clear();

	ARTHexMapActor* HexMap = SpawnAutobattleMap(World);
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Il livello porta gia' le sue unita': e' il caso per cui esiste il ritorno anticipato di `SetupHexMatch`.
	GameMode->bAutobattle = false;
	GameMode->SetupHexMatch(HexMap);
	if (!TestEqual(TEXT("due unita' sono del giocatore"), CountAutobattleHumanUnits(World), 2))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Adesso la demo, sullo STESSO livello: l'allestimento non interviene, la modalita' si'.
	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(HexMap);

	TestEqual(TEXT("nessuna unita' aspetta piu' una mano umana"), CountAutobattleHumanUnits(World), 0);
	TestEqual(TEXT("e non ne sono state allestite altre"), CollectAutobattleUnits(World).Num(), 4);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * IL GATE DI CP 47.1: premuto Play e non toccato piu' nulla, compare un vincitore.
 *
 * `RefactorTactics.HexMatch.PlaysToCompletion` dimostra gia' che una partita 2v2 bot-contro-bot arriva
 * all'eliminazione, ma costruisce le unita' **a mano** e le mette sotto il bot una per una: verifica il
 * motore, non la configurazione. Questo test entra dalla porta vera — `SetupHexMatch` con l'autobattle
 * acceso — ed e' la differenza fra «il gioco puo' giocarsi da solo» e «il gioco SI gioca da solo».
 *
 * ⚠️ **Cosa il TurnLog deve mostrare, e come e' stato interpretato il DoD.** #954 chiede «almeno una voce
 * `Move` e una `Combat` per round giocato» come modo di distinguere una partita finita da una **bloccata**
 * che il `RoundLimit` ha chiuso. Preteso round per round sarebbe falso: al primo round le unita' sono
 * lontane e nessuno puo' colpire, e il test fallirebbe su un comportamento corretto. Quindi si verifica
 * cio' che quella richiesta vuole davvero escludere:
 *   · **ogni** round lascia almeno una voce — nessun round muto, che e' la firma dello stallo;
 *   · sulla partita intera compaiono **entrambe** le categorie — i bot si muovono *e* combattono;
 *   · la partita finisce per **eliminazione**, non per esaurimento dei round.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattlePlaysToCompletionTest,
	"RefactorTactics.Match.Autobattle.PlaysToCompletionWithoutInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattlePlaysToCompletionTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	FRTScopedAutobattleCVars CVarGuard;
	FRTScopedAutobattleCommandLine CmdGuard;
	CVarGuard.SetMode(-1);
	CmdGuard.Clear();

	ARTHexMapActor* HexMap = SpawnAutobattleMap(World);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("TurnManager"), TM)
		|| !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(HexMap);

	// Senza BeginPlay i cooldown non vengono inizializzati (`AbilityCooldowns` resta vuoto) e OGNI abilita'
	// risulta sempre pronta: una partita di prova che non lo chiama misura un gioco che non esiste. Qui le
	// unita' le spawna `SetupHexMatch`, quindi il dispatch va fatto dopo — e solo se il mondo non l'ha gia'
	// fatto, perche' due BeginPlay sono un'altra partita ancora.
	for (ARTUnit* Unit : CollectAutobattleUnits(World))
	{
		if (!Unit->HasActorBegunPlay()) { Unit->DispatchBeginPlay(); }
	}

	if (!TestEqual(TEXT("nessuna unita' aspetta una mano umana"), CountAutobattleHumanUnits(World), 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Tetto di sicurezza, non una regola di gioco: serve a fallire invece di girare all'infinito. Lo stesso
	// numero di `PlaysToCompletion`, che sulla stessa arena misura la decisione al turno 10.
	const int32 MaxTurns = 40;
	int32 TurnsPlayed = 0;
	int32 MuteTurns = 0;
	bool bSawMove = false;
	bool bSawCombat = false;

	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && TurnsPlayed < MaxTurns)
	{
		TM->PlanBotsForTest();
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
		++TurnsPlayed;

		const TArray<FRTTurnLogEntry>& Log = TM->GetTurnLog();
		if (Log.Num() == 0) { ++MuteTurns; }
		for (const FRTTurnLogEntry& Entry : Log)
		{
			if (Entry.Category == ERTLogCategory::Move)   { bSawMove = true; }
			if (Entry.Category == ERTLogCategory::Combat) { bSawCombat = true; }
		}
	}

	TestTrue(TEXT("la partita si e' decisa entro il limite di turni"),
		TM->GetPhase() == ERTMatchPhase::MatchEnded);
	TestTrue(TEXT("non si e' decisa al primo turno"), TurnsPlayed > 1);
	TestEqual(TEXT("nessun round muto: il TurnLog racconta ogni turno"), MuteTurns, 0);
	TestTrue(TEXT("i bot si sono mossi"), bSawMove);
	TestTrue(TEXT("e hanno combattuto"), bSawCombat);

	// Un vincitore, e per ELIMINAZIONE: una partita chiusa dal limite di round e' la partita bloccata che
	// il DoD chiede di distinguere, non una demo riuscita.
	const FRTMatchResult Result = TM->GetMatchResult();
	TestTrue(TEXT("la partita ha un esito"), Result.Outcome != ERTMatchOutcome::InProgress);
	TestEqual(TEXT("e ha vinto qualcuno per eliminazione"),
		Result.Reason, ERTMatchEndReason::Elimination);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
