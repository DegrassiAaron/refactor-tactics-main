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
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
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

// =================================================================================================
// IL CORPUS DI DETERMINISMO DELL'AUTOBATTLE (CP 47.5, #958)
// =================================================================================================
//
// Sette casi limite, e uno **escluso con la ragione scritta**. I test qui sotto stanno accanto a quelli di
// CP 47.1 e non in un file proprio per una ragione misurata: gli helper della partita non presidiata —
// `SpawnAutobattleMap`, `CollectAutobattleUnits`, le due guardie di stato globale — vivono nel namespace
// anonimo di QUESTO file, e in unity build due namespace anonimi con la stessa funzione collidono. Un file
// gemello avrebbe dovuto ribattezzarli tutti, che e' il duplicato che `RTWorldFixtures.h` esiste per chiudere.
//
// ## Cosa questo corpus NON e'
//
// Non sostituisce **G4**. Il gate di release («determinismo: 100 ripetizioni, checksum identico») e'
// `RefactorTactics.Replay.Verifier.ResimulationIsDeterministic` e resta dov'e': quello ri-simula uno scenario
// attraverso il resolver, questo esercita i **casi limite di una partita non presidiata**. Due perimetri
// diversi, e nessuno dei due copre l'altro.
//
// ## Perche' si confronta il TurnLog e non lo `StateHash`
//
// `FRTTestResult::StateHash` e' **permutazione-invariante per costruzione** — le unita' si ordinano prima di
// mescolare — quindi non saprebbe esprimere cio' che `PermutationTest` cerca: passerebbe anche se il roster
// avesse perso l'ordinamento. E il confronto e' **turno per turno**, non sul solo esito finale: due partite
// con lo stesso hash finale possono esserci arrivate per stati diversi, e «diverge dal turno N» e' l'unica
// diagnosi con cui si apre un debugger. E' la lezione gia' pagata da `Stress.ReplayDivergenceZeroAt4v4`.
//
// ## `DifferentSeedVariation` e' FUORI, e non per assenza di premessa
//
// Il runtime non ha alcun RNG e `FRTTestScenario::Seed` e' «dichiarato ma non consumato». Scrivere quel test
// non aggiungerebbe una riga a una suite che tace: farebbe **cadere un test verde**,
// `RefactorTactics.Simulation.SeedIsDeclaredAndUnconsumed`, che verifica esattamente l'opposto — due seed
// diversi danno lo stesso risultato. Quel test prescrive gia' la procedura per il giorno in cui un RNG
// entrera' nella simulazione, e va seguita invece che improvvisata. Il gancio e' `RNG-1` in
// `docs/OPEN_DECISIONS.md` (#960): finche' quella domanda e' aperta, il corpus resta a **sette** casi.

namespace
{
	/**
	 * La traccia CANONICA di una partita: il TurnLog serializzato di ogni turno, piu' come e' finita.
	 *
	 * Un array per turno e non un buffer unico: i confini fra i turni sono esattamente l'informazione con cui
	 * una divergenza sa dire «turno 3», e concatenare li perderebbe. Stessa forma di `FRTTurnTrace` nello
	 * Scenario Harness, e per la stessa ragione (CP 12.6, #178).
	 */
	struct FRTAutobattleTrace
	{
		/** Byte di `SerializeTurnLog`, checksum in coda incluso, uno per turno giocato. */
		TArray<TArray<uint8>> Turns;

		int32 TurnsPlayed = 0;
		ERTMatchOutcome Outcome = ERTMatchOutcome::InProgress;
		ERTMatchEndReason Reason = ERTMatchEndReason::None;

		/** La partita non si e' decisa entro il tetto di sicurezza. E' un difetto, non un esito. */
		bool bHitSafetyCap = false;

		/** Una risoluzione non e' finita entro i tick concessi: la traccia di quel turno e' monca. */
		bool bResolveStalled = false;

		/**
		 * Tick di risoluzione consumati in tutta la partita. NON e' un dato logico e
		 * `DescribeAutobattleDivergence` non lo confronta di proposito: due velocita' diverse DEVONO
		 * differire qui e coincidere su tutto il resto.
		 *
		 * ⚠️ Esiste perche' senza di lui il gate di CP 47.2 sarebbe soddisfacibile **non leggendo affatto**
		 * `ViewerPlaybackSpeed`: un campo dichiarato, mai consumato, lascia il TurnLog identico a tutte le
		 * velocita' e il test verde. E' l'anello «letto» della catena, e va misurato separatamente.
		 */
		int32 ResolveTicks = 0;
	};

	/**
	 * Un'unita' d'eroe sotto il bot, sulla cella data.
	 *
	 * `DispatchBeginPlay` non e' una formalita': senza, `AbilityCooldowns` resta vuoto e OGNI abilita' risulta
	 * sempre pronta — una partita di prova che non lo chiama misura un gioco che non esiste.
	 */
	ARTUnit* SpawnAutobattleUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell,
		bool bBot = true)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = bBot;
		U->DispatchBeginPlay();
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Quante unita' vive ha ciascuna squadra, adesso. */
	void CountAutobattleAlive(UWorld* World, int32& OutTeam0, int32& OutTeam1)
	{
		OutTeam0 = 0;
		OutTeam1 = 0;
		for (const ARTUnit* Unit : CollectAutobattleUnits(World))
		{
			if (!Unit->IsAlive()) { continue; }
			(Unit->TeamId == 0 ? OutTeam0 : OutTeam1)++;
		}
	}

	/**
	 * Un turno intero, e la sua traccia catturata NEL MOMENTO in cui e' completa.
	 *
	 * Il TurnLog vive per un turno: il prossimo `LockInAndResolve` lo azzera. Leggerlo a partita finita darebbe
	 * l'ultimo turno soltanto, e un confronto «turno per turno» sarebbe verde per il motivo sbagliato.
	 */
	/**
	 * Agganciato PRIMA di ogni tick di risoluzione, con l'indice del tick. Esiste per un solo caso —
	 * cambiare la velocita' di playback A META' risoluzione (CP 47.2, #955) — e resta `nullptr` per tutti
	 * gli altri chiamanti, che non cambiano.
	 */
	using FRTPlaybackTickHook = TFunction<void(ARTTurnManager*, int32)>;

	void PlayAutobattleTurn(ARTTurnManager* TM, FRTAutobattleTrace& Out,
		const FRTPlaybackTickHook& OnTick = nullptr)
	{
		TM->PlanBotsForTest();
		TM->LockInAndResolve();

		int32 Ticks = 0;
		for (; Ticks < 400 && TM->IsResolving(); ++Ticks)
		{
			if (OnTick) { OnTick(TM, Ticks); }
			TM->Tick(0.05f);
		}
		// Una risoluzione appesa somiglia a una risoluzione lenta, e la differenza si scopre solo aspettando.
		// Registrarla e' cio' che permette al test di dire «monca» invece di confrontare due tracce parziali.
		if (TM->IsResolving()) { Out.bResolveStalled = true; }
		Out.ResolveTicks += Ticks;

		Out.Turns.Add(URTTurnLogLibrary::SerializeTurnLog(TM->GetTurnLog(), ERTLogTopology::Hex));
		++Out.TurnsPlayed;
	}

	/**
	 * La partita, dal primo turno alla fine.
	 *
	 * `MaxTurns` e' un **tetto di sicurezza, non una regola di gioco**: serve a fallire invece di girare
	 * all'infinito, ed e' lo stesso 40 di `HexMatch.PlaysToCompletion` — che sulla stessa arena misura la
	 * decisione al turno 10. Raggiungerlo si registra in `bHitSafetyCap` e vale come difetto.
	 */
	FRTAutobattleTrace PlayAutobattleMatch(ARTTurnManager* TM, int32 MaxTurns = 40,
		const FRTPlaybackTickHook& OnTick = nullptr)
	{
		FRTAutobattleTrace Trace;
		while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Trace.TurnsPlayed < MaxTurns)
		{
			PlayAutobattleTurn(TM, Trace, OnTick);
		}
		Trace.bHitSafetyCap = (TM->GetPhase() != ERTMatchPhase::MatchEnded);

		const FRTMatchResult Result = TM->GetMatchResult();
		Trace.Outcome = Result.Outcome;
		Trace.Reason = Result.Reason;
		return Trace;
	}

	/**
	 * La PRIMA divergenza fra due tracce, o stringa vuota se sono identiche.
	 *
	 * Non ritorna un booleano di proposito: «diverse» costringe a rieseguire per capire, e a quel punto tanto
	 * varrebbe non aver confrontato. Il numero di turno viene prima di tutto il resto perche' e' il dato con
	 * cui si apre un debugger.
	 */
	FString DescribeAutobattleDivergence(const TCHAR* LabelA, const FRTAutobattleTrace& A,
		const TCHAR* LabelB, const FRTAutobattleTrace& B)
	{
		const int32 Common = FMath::Min(A.Turns.Num(), B.Turns.Num());
		for (int32 T = 0; T < Common; ++T)
		{
			if (A.Turns[T] == B.Turns[T]) { continue; }

			// I byte dicono CHE diverge, non COSA: si rilegge la traccia e si nomina la prima voce diversa.
			// Un offset in un buffer binario costringe a un hex dump per capire, e a quel punto la diagnosi
			// l'ha fatta chi legge invece del test.
			TArray<FRTTurnLogEntry> EntriesA, EntriesB;
			const bool bReadA = URTTurnLogLibrary::DeserializeTurnLog(A.Turns[T], EntriesA);
			const bool bReadB = URTTurnLogLibrary::DeserializeTurnLog(B.Turns[T], EntriesB);
			if (!bReadA || !bReadB)
			{
				return FString::Printf(
					TEXT("diverge dal turno %d: %s ha %d byte, %s ne ha %d (e la traccia non si rilegge: %s/%s)"),
					T + 1, LabelA, A.Turns[T].Num(), LabelB, B.Turns[T].Num(),
					bReadA ? TEXT("ok") : TEXT("illeggibile"), bReadB ? TEXT("ok") : TEXT("illeggibile"));
			}

			const int32 CommonEntries = FMath::Min(EntriesA.Num(), EntriesB.Num());
			for (int32 E = 0; E < CommonEntries; ++E)
			{
				const FString TextA = URTTurnLogLibrary::DescribeEntry(EntriesA[E]);
				const FString TextB = URTTurnLogLibrary::DescribeEntry(EntriesB[E]);
				if (TextA != TextB)
				{
					return FString::Printf(
						TEXT("diverge dal turno %d, voce %d di %d/%d:\n    %s: %s\n    %s: %s"),
						T + 1, E + 1, EntriesA.Num(), EntriesB.Num(), LabelA, *TextA, LabelB, *TextB);
				}
			}
			if (EntriesA.Num() != EntriesB.Num())
			{
				// Una traccia e' prefisso dell'altra: la prima voce IN PIU' e' quella che spiega la differenza.
				const bool bAShorter = EntriesA.Num() < EntriesB.Num();
				const TArray<FRTTurnLogEntry>& Longer = bAShorter ? EntriesB : EntriesA;
				return FString::Printf(
					TEXT("diverge dal turno %d: %s ha %d voci, %s ne ha %d — la prima in piu' e': %s"),
					T + 1, LabelA, EntriesA.Num(), LabelB, EntriesB.Num(),
					*URTTurnLogLibrary::DescribeEntry(Longer[CommonEntries]));
			}
			// Stesse voci leggibili, byte diversi: e' un campo che `DescribeEntry` non stampa.
			return FString::Printf(
				TEXT("diverge dal turno %d: %d voci identiche alla lettura ma %d byte contro %d — ")
				TEXT("un campo serializzato che la descrizione non mostra"),
				T + 1, EntriesA.Num(), A.Turns[T].Num(), B.Turns[T].Num());
		}

		if (A.TurnsPlayed != B.TurnsPlayed)
		{
			return FString::Printf(TEXT("stessi %d turni in comune, ma %s ne gioca %d e %s ne gioca %d"),
				Common, LabelA, A.TurnsPlayed, LabelB, B.TurnsPlayed);
		}
		if (A.Outcome != B.Outcome || A.Reason != B.Reason)
		{
			return FString::Printf(TEXT("stesse tracce, esiti diversi: %s finisce '%s' %s, %s finisce '%s' %s"),
				LabelA, *URTTurnRules::DescribeOutcome(A.Outcome), *URTTurnRules::DescribeEndReason(A.Reason),
				LabelB, *URTTurnRules::DescribeOutcome(B.Outcome), *URTTurnRules::DescribeEndReason(B.Reason));
		}
		return FString();
	}

	/** Le quattro posizioni di partenza del 2v2, agli estremi opposti: la stessa di `PlaysToCompletion`. */
	struct FRTAutobattleSlot
	{
		int32 TeamId;
		bool bIsWraith;      // il roster del 2v2 headless: un tiratore e un corpo a corpo per squadra
		FRTCellId Cell;
	};

	const TArray<FRTAutobattleSlot>& AutobattleStandardSlots()
	{
		static const TArray<FRTAutobattleSlot> Slots = {
			{ 0, true,  FRTCellId(-4, 2) },
			{ 0, false, FRTCellId(-4, 3) },
			{ 1, true,  FRTCellId(4, -2) },
			{ 1, false, FRTCellId(4, -3) },
		};
		return Slots;
	}

	/**
	 * Schiera il 2v2 seguendo l'ORDINE DI INSERIMENTO dato, e ritorna le unita' nell'ordine di spawn.
	 *
	 * `Order` e' una permutazione degli indici di `AutobattleStandardSlots()`: la configurazione di gioco —
	 * chi sta dove, con quale eroe — non cambia mai, cambia solo la sequenza in cui gli Actor entrano nel
	 * mondo. E' esattamente la variabile che l'invariante #3 dice non debba contare.
	 */
	TArray<ARTUnit*> DeployAutobattleRoster(UWorld* World, const TArray<int32>& Order)
	{
		TArray<ARTUnit*> Spawned;
		for (int32 Index : Order)
		{
			const FRTAutobattleSlot& Slot = AutobattleStandardSlots()[Index];
			const URTHeroData* Hero = Slot.bIsWraith
				? URTHeroCatalogLibrary::MakeWraith()
				: URTHeroCatalogLibrary::MakeRiktor();
			Spawned.Add(SpawnAutobattleUnit(World, Slot.TeamId, Hero, Slot.Cell));
		}
		return Spawned;
	}
}

/**
 * CASO 1/7 — `PermutationTest`: l'ORDINE DI INSERIMENTO delle unita' non cambia la partita.
 *
 * `Simulation.ChecksumStableAcrossPermutations` verifica gia' la permutazione, ma su uno scenario a turni
 * scritti e confrontando lo `StateHash`. Qui la partita e' **intera** e il confronto e' sul **TurnLog
 * canonico turno per turno**: l'hash e' permutazione-invariante per costruzione e passerebbe anche se
 * l'identita' delle unita' seguisse l'ordine di spawn.
 *
 * ⚠️ **CHE COSA PUO' ROMPERSI DAVVERO, e dove.** `ARTTurnManager::EnsureMatchRoster` legge le unita' con
 * `GetAllActorsOfClass` — il cui ordine e' quello in cui il livello tiene gli Actor, non un dato di gioco —
 * e poi le **ordina** con `MatchRosterLess` (`TeamId`, poi la cella, poi il nome) prima di assegnare
 * `StableUnitId = i + 1`. Quello `StableUnitId` finisce in ogni voce di TurnLog (`AppendLogEntry`). Tolto il
 * `Sort`, questo test diventa rosso e nessun altro se ne accorge: e' la mutazione che lo giustifica.
 *
 * ⛔ **Non passa da `SetupHexMatch`, ed e' dichiarato invece che dedotto.** L'allestimento spawna i quattro
 * eroi in un ordine fisso e non offre modo di permutarlo — e permutarlo *dall'esterno* e' il soggetto di
 * questo test. La configurazione dell'autobattle e' gia' coperta da `PlaysToCompletionWithoutInput`; qui il
 * percorso resta quello reale — `PlanBots -> LockInAndResolve -> snapshot -> resolver -> TurnLog` — con
 * l'unica differenza che le unita' le schiera il test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattlePermutationTest,
	"RefactorTactics.Match.Autobattle.DeterminismSurvivesUnitPermutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattlePermutationTest::RunTest(const FString&)
{
	// Diretto e inverso: la stessa partita, inserita nei due versi.
	const TArray<TArray<int32>> Orders = {
		{ 0, 1, 2, 3 },
		{ 3, 2, 1, 0 },
		{ 2, 0, 3, 1 },   // e uno mescolato, perche' l'inverso da solo non distingue un ordinamento parziale
	};

	TArray<FRTAutobattleTrace> Traces;
	for (const TArray<int32>& Order : Orders)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		SpawnAutobattleMap(World);

		const TArray<ARTUnit*> Units = DeployAutobattleRoster(World, Order);
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || Units.Contains(nullptr))
		{
			AddError(TEXT("allestimento fallito: turn manager o unita' mancanti"));
			RTWorldFixtures::DestroyWorld(World);
			return false;
		}

		Traces.Add(PlayAutobattleMatch(TM));
		RTWorldFixtures::DestroyWorld(World);
	}

	// Premesse: senza, un confronto fra tre partite mai giocate sarebbe verde.
	if (!TestTrue(TEXT("premessa: la partita di riferimento e' andata oltre il primo turno"),
		Traces[0].TurnsPlayed > 1))
	{
		return false;
	}
	TestFalse(TEXT("premessa: nessuna risoluzione e' rimasta appesa"), Traces[0].bResolveStalled);
	TestFalse(TEXT("premessa: la partita si e' decisa entro il tetto di sicurezza"), Traces[0].bHitSafetyCap);

	for (int32 I = 1; I < Traces.Num(); ++I)
	{
		const FString Divergence = DescribeAutobattleDivergence(
			TEXT("ordine diretto"), Traces[0], *FString::Printf(TEXT("permutazione %d"), I), Traces[I]);
		TestEqual(FString::Printf(TEXT("la permutazione %d gioca la stessa partita"), I),
			Divergence, FString());
	}
	return true;
}

/**
 * CASO 2/7 — `PlaybackIndependence`: il TEMPO DELLA PRESENTAZIONE non tocca il risultato logico.
 *
 * ⚠️ **LA PREMESSA DELLA ISSUE E' STATA CORRETTA, e va detto invece che lasciato dedurre.** #958 dichiarava
 * questo caso in attesa di **E47.2** perche' *«oggi non c'e' velocita' da variare»*. Misurato sul codice: c'e'.
 * `ARTTurnManager::PlaybackSpeed` esiste, deriva da `MaxPlaybackSeconds` (default 12 s) via
 * `URTPlaybackLibrary::SpeedMultiplierForCap`, e `bEnablePlayback` accende o spegne il playback per intero.
 *
 * E l'invariante disponibile allora era **piu' forte** di quella rinviata. `ResolveTurn` decide fra due strade:
 * con eventi e playback acceso chiama `BeginPlayback()`, altrimenti va dritto a `ConcludeTurn()`. Questo test
 * confronta quelle **due strade**, non due velocita' della stessa: se il risultato logico regge a «con
 * presentazione» contro «senza presentazione affatto», regge a fortiori a un moltiplicatore.
 *
 * ✅ **E47.2 (#955) e' arrivata, e le varianti passano da tre a sette.** `ViewerPlaybackSpeed` — la velocita'
 * SCELTA da chi guarda — si compone con il fattore di cap via `URTPlaybackLibrary::EffectivePlaybackSpeed`.
 * Le quattro nuove coprono cio' che le tre vecchie non potevano:
 *  · x2 e x4 su un round che il tetto NON accelera: e' la manopola da sola;
 *  · x4 su `MaxPlaybackSeconds = 2 s`: e' la composizione, dove il cap morde gia' per conto suo;
 *  · velocita' cambiata **a meta' risoluzione**: e' l'unica che verifica l'aggettivo «applicabile DURANTE»
 *    del DoD, e l'unica che romperebbe se `TickPlayback` congelasse la composizione in `BeginPlayback`
 *    invece di rileggerla a ogni tick. Le altre sei resterebbero verdi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattlePlaybackIndependenceTest,
	"RefactorTactics.Match.Autobattle.DeterminismIsIndependentOfPlayback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattlePlaybackIndependenceTest::RunTest(const FString&)
{
	struct FRTPlaybackVariant
	{
		const TCHAR* Label;
		bool bEnablePlayback;
		float MaxPlaybackSeconds;
		float ViewerSpeed;
		bool bChangeMidResolution;   // cambia ViewerPlaybackSpeed mentre la risoluzione e' in corso
	};
	const FRTPlaybackVariant Variants[] = {
		{ TEXT("playback spento"),               false, 12.f, 1.f, false },
		{ TEXT("playback acceso 12 s"),          true,  12.f, 1.f, false },
		{ TEXT("playback acceso 2 s"),           true,   2.f, 1.f, false },  // stessa strada, cap diverso
		{ TEXT("x2 scelta da chi guarda"),       true,  12.f, 2.f, false },  // la manopola da sola: il cap non morde
		{ TEXT("x4 scelta da chi guarda"),       true,  12.f, 4.f, false },
		{ TEXT("x4 con cap a 2 s"),              true,   2.f, 4.f, false },  // composizione: entrambi mordono
		{ TEXT("velocita' cambiata a meta'"),    true,  12.f, 1.f, true  },  // «applicabile DURANTE»
	};

	// Il cambio a caldo: x1 fino al terzo tick di ogni risoluzione, poi x4. Gli indici sono piccoli di
	// proposito — una risoluzione dura decine di tick, e cambiare al terzo cade dentro la PRIMA fase invece
	// che fra una fase e l'altra, dove il passaggio sarebbe indistinguibile da un cambio fra turni.
	// `SpeedChanges` non e' diagnostica: senza, una risoluzione che finisse entro tre tick lascerebbe la
	// settima variante identica alla seconda, e il test sarebbe verde per il motivo sbagliato — avrebbe
	// confrontato due volte x1. E' la stessa guardia che qui sotto protegge `bResolveStalled`.
	int32 SpeedChanges = 0;
	const FRTPlaybackTickHook ChangeMidResolution = [&SpeedChanges](ARTTurnManager* TM, int32 TickIndex)
	{
		if (TickIndex == 3) { ++SpeedChanges; }
		TM->ViewerPlaybackSpeed = (TickIndex < 3) ? 1.f : 4.f;
	};

	TArray<FRTAutobattleTrace> Traces;
	for (const FRTPlaybackVariant& Variant : Variants)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		SpawnAutobattleMap(World);

		const TArray<ARTUnit*> Units = DeployAutobattleRoster(World, { 0, 1, 2, 3 });
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || Units.Contains(nullptr))
		{
			AddError(TEXT("allestimento fallito: turn manager o unita' mancanti"));
			RTWorldFixtures::DestroyWorld(World);
			return false;
		}
		TM->bEnablePlayback = Variant.bEnablePlayback;
		TM->MaxPlaybackSeconds = Variant.MaxPlaybackSeconds;
		TM->ViewerPlaybackSpeed = Variant.ViewerSpeed;

		Traces.Add(PlayAutobattleMatch(TM, /*MaxTurns=*/ 40,
			Variant.bChangeMidResolution ? ChangeMidResolution : FRTPlaybackTickHook()));
		RTWorldFixtures::DestroyWorld(World);
	}

	if (!TestTrue(TEXT("premessa: la partita di riferimento e' andata oltre il primo turno"),
		Traces[0].TurnsPlayed > 1))
	{
		return false;
	}
	// Senza questa riga il test sarebbe verde anche se le sette varianti avessero tutte saltato il playback:
	// confronterebbe sette volte la stessa strada e non proverebbe niente.
	for (int32 I = 0; I < Traces.Num(); ++I)
	{
		TestFalse(FString::Printf(TEXT("%s: nessuna risoluzione appesa"), Variants[I].Label),
			Traces[I].bResolveStalled);
	}
	// La settima variante deve aver CAMBIATO velocita' davvero, non solo essere stata configurata per farlo.
	TestTrue(TEXT("premessa: il cambio a meta' risoluzione e' scattato almeno una volta"),
		SpeedChanges > 0);

	// --- La manopola DEVE fare qualcosa -----------------------------------------------------------
	//
	// Tutto il resto di questo test verifica che la velocita' NON cambi il risultato logico — ed e' una
	// proprieta' che un campo mai letto soddisfa alla perfezione. Senza le quattro righe qui sotto, il gate
	// di CP 47.2 sarebbe verde su un `ViewerPlaybackSpeed` dichiarato e ignorato.
	// I tick di risoluzione sono la misura dell'effetto: piu' veloce = meno tick per la stessa partita.
	enum : int32 { VarX1 = 1, VarX2 = 3, VarX4 = 4, VarHotSwap = 6 };

	TestTrue(FString::Printf(TEXT("x2 accorcia la presentazione rispetto a x1 (%d tick contro %d)"),
		Traces[VarX2].ResolveTicks, Traces[VarX1].ResolveTicks),
		Traces[VarX2].ResolveTicks < Traces[VarX1].ResolveTicks);

	TestTrue(FString::Printf(TEXT("x4 accorcia piu' di x2 (%d tick contro %d)"),
		Traces[VarX4].ResolveTicks, Traces[VarX2].ResolveTicks),
		Traces[VarX4].ResolveTicks < Traces[VarX2].ResolveTicks);

	// ⚠️ E' l'asserzione che rende il test sensibile al CONGELAMENTO: se `TickPlayback` leggesse la
	// composizione una volta sola in `BeginPlayback`, questa variante partirebbe a x1 e resterebbe a x1 —
	// stessi tick di `VarX1`, e la riga cade. Le altre sei resterebbero tutte verdi.
	TestTrue(FString::Printf(TEXT("il cambio a meta' risoluzione accorcia davvero (%d tick contro %d)"),
		Traces[VarHotSwap].ResolveTicks, Traces[VarX1].ResolveTicks),
		Traces[VarHotSwap].ResolveTicks < Traces[VarX1].ResolveTicks);

	for (int32 I = 1; I < Traces.Num(); ++I)
	{
		const FString Divergence = DescribeAutobattleDivergence(
			Variants[0].Label, Traces[0], Variants[I].Label, Traces[I]);
		TestEqual(FString::Printf(TEXT("«%s» gioca la stessa partita di «%s»"),
			Variants[I].Label, Variants[0].Label), Divergence, FString());
	}

	AddInfo(FString::Printf(
		TEXT("sette varianti: due strade del playback, due tetti, la velocita' scelta da chi guarda (x1/x2/x4) ")
		TEXT("e un cambio a meta' risoluzione. Turni giocati: %d · cambi a caldo scattati: %d · ")
		TEXT("tick di risoluzione x1/x2/x4/a-caldo: %d/%d/%d/%d"),
		Traces[0].TurnsPlayed, SpeedChanges,
		Traces[VarX1].ResolveTicks, Traces[VarX2].ResolveTicks,
		Traces[VarX4].ResolveTicks, Traces[VarHotSwap].ResolveTicks));
	return true;
}

/**
 * CASO 3/7 — `NoPath`: un bot senza percorso produce un ripiego LEGALE, e il turno non si blocca.
 *
 * Il caso limite e' quello che una partita non presidiata non puo' segnalare: se un'unita' murata mandasse
 * il turno in stallo, non ci sarebbe nessuno a premere niente. Quindi si verificano tre cose insieme —
 * il turno **finisce**, l'unita' resta su una cella **legale**, e la partita continua ad avanzare.
 *
 * ⚠️ Il muro e' costruito togliendo la percorribilita' a tutte e sei le vicine, sullo stesso layer: il bot
 * ha bersagli in vista ma nessuna mossa verso di loro. E' il caso in cui `ReachableCells` torna vuoto e le
 * candidate di movimento non esistono — non un caso in cui il pathfinding e' semplicemente lungo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattleNoPathTest,
	"RefactorTactics.Match.Autobattle.NoPathProducesLegalFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattleNoPathTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* MapActor = SpawnAutobattleMap(World);
	URTHexMapAsset* Map = MapActor ? MapActor->MapAsset : nullptr;
	if (!TestNotNull(TEXT("mappa di prova"), Map)) { RTWorldFixtures::DestroyWorld(World); return false; }

	// L'unita' murata sta al centro; le sei vicine diventano invalicabili.
	const FRTCellId Walled(0, 0, 0);
	for (const FRTCellId& Neighbor : URTHexLibrary::Neighbors(Walled))
	{
		if (const FRTHexCellData* Existing = Map->FindCell(Neighbor))
		{
			FRTHexCellData Blocked = *Existing;
			Blocked.bBlocksMovement = true;
			Map->AddOrUpdateCell(Blocked);
		}
	}
	Map->SortCells();

	ARTUnit* Trapped = SpawnAutobattleUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), Walled);
	ARTUnit* Free    = SpawnAutobattleUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(4, -2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Trapped || !Free)
	{
		AddError(TEXT("allestimento fallito"));
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Premessa: il muro c'e' davvero. Senza, il test misurerebbe un'unita' che sceglie di stare ferma.
	int32 BlockedNeighbors = 0;
	for (const FRTCellId& Neighbor : URTHexLibrary::Neighbors(Walled))
	{
		const FRTHexCellData* Data = Map->FindCell(Neighbor);
		if (Data && Data->bBlocksMovement) { ++BlockedNeighbors; }
	}
	if (!TestEqual(TEXT("premessa: tutte e sei le vicine bloccano il movimento"), BlockedNeighbors, 6))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Cinque turni: il ripiego deve reggere ripetuto, non solo la prima volta.
	FRTAutobattleTrace Trace;
	for (int32 Turn = 0; Turn < 5 && TM->GetPhase() != ERTMatchPhase::MatchEnded; ++Turn)
	{
		const int32 TurnBefore = TM->GetTurnNumber();
		PlayAutobattleTurn(TM, Trace);

		TestFalse(FString::Printf(TEXT("turno %d: la risoluzione non resta appesa"), Turn + 1),
			Trace.bResolveStalled);
		TestTrue(FString::Printf(TEXT("turno %d: il numero di turno avanza (%d -> %d)"),
				Turn + 1, TurnBefore, TM->GetTurnNumber()),
			TM->GetTurnNumber() > TurnBefore || TM->GetPhase() == ERTMatchPhase::MatchEnded);

		// Il ripiego e' LEGALE: si resta dov'e' possibile stare, non dentro un muro e non fuori mappa.
		if (Trapped->IsAlive())
		{
			const FRTHexCellData* Here = Map->FindCell(Trapped->Cell);
			TestNotNull(FString::Printf(TEXT("turno %d: l'unita' murata sta su una cella della mappa"), Turn + 1),
				Here);
			if (Here)
			{
				TestFalse(FString::Printf(TEXT("turno %d: e non dentro un ostacolo"), Turn + 1),
					Here->bBlocksMovement);
			}
			TestEqual(FString::Printf(TEXT("turno %d: senza percorso non si e' teletrasportata"), Turn + 1),
				Trapped->Cell.ToString(), Walled.ToString());
		}
	}

	TestTrue(TEXT("la partita ha prodotto tracce"), Trace.TurnsPlayed > 0);
	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * CASO 4/7 — `AllWait`: se nessuno agisce, il turno finisce lo stesso.
 *
 * ⚠️ **IL GIVEN, che la issue non scriveva.** «Tutte le unita' attendono» non si ottiene chiedendolo al bot:
 * il bot **sceglie**, e costruire uno stato in cui sceglie `Wait` significherebbe verificare le sue
 * preferenze invece del turno. Quindi le unita' qui sono **fuori dal bot e senza piano** — che e' la
 * definizione letterale di «tutte attendono» — e il soggetto del test e' `ARTTurnManager`.
 *
 * Conta per l'autobattle proprio perche' li' nessuno puo' intervenire: un turno vuoto che non si chiudesse
 * fermerebbe una partita non presidiata per sempre, e non ci sarebbe nessuno a notarlo. E' lo stesso difetto
 * che `PlanningSecondsNeverStallAnUnattendedMatch` copre dal lato del timer, dal lato della risoluzione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattleAllWaitTest,
	"RefactorTactics.Match.Autobattle.AllWaitEndsTheTurnNormally",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattleAllWaitTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	SpawnAutobattleMap(World);

	// Fuori dal bot e lontane: nessun piano viene scritto da nessuno.
	ARTUnit* A = SpawnAutobattleUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-4, 2), false);
	ARTUnit* B = SpawnAutobattleUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(4, -2), false);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !A || !B)
	{
		AddError(TEXT("allestimento fallito"));
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const FRTCellId AStart = A->Cell;
	const FRTCellId BStart = B->Cell;

	// Quattro turni di attesa di fila: uno solo non distingue «si chiude» da «si chiude la prima volta».
	for (int32 Turn = 1; Turn <= 4; ++Turn)
	{
		const int32 TurnBefore = TM->GetTurnNumber();

		TM->LockInAndResolve();   // niente `PlanBots`: nessuno decide, ed e' il punto
		int32 Ticks = 0;
		for (; Ticks < 400 && TM->IsResolving(); ++Ticks) { TM->Tick(0.05f); }

		TestFalse(FString::Printf(TEXT("turno %d: la risoluzione si chiude"), Turn), TM->IsResolving());
		TestEqual(FString::Printf(TEXT("turno %d: il numero di turno avanza di uno"), Turn),
			TM->GetTurnNumber(), TurnBefore + 1);
		TestTrue(FString::Printf(TEXT("turno %d: la partita non e' finita per errore"), Turn),
			TM->GetPhase() != ERTMatchPhase::MatchEnded);
	}

	// Nessuno si e' mosso e nessuno e' morto: attendere non e' un'azione.
	TestEqual(TEXT("chi attende resta dov'e' (squadra 0)"), A->Cell.ToString(), AStart.ToString());
	TestEqual(TEXT("chi attende resta dov'e' (squadra 1)"), B->Cell.ToString(), BStart.ToString());
	TestTrue(TEXT("nessuno e' caduto durante l'attesa"), A->IsAlive() && B->IsAlive());

	// E l'esito resta aperto: un turno vuoto non e' una condizione di fine partita.
	const FRTMatchResult Result = TM->GetMatchResult();
	TestEqual(TEXT("la partita resta in corso"), Result.Outcome, ERTMatchOutcome::InProgress);
	TestEqual(TEXT("e nessuna via di chiusura si e' attivata"), Result.Reason, ERTMatchEndReason::None);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * CASO 5/7 — `SimultaneousKO`: due cadute nello stesso Cleanup seguono una politica DICHIARATA.
 *
 * ⚠️ **La politica esiste gia', ed e' esplicita** — la issue la dava «da verificare».
 * `URTTurnRules::EvaluateMatchEnd` ha una precedenza fissa (eliminazione, poi obiettivo, poi `RoundLimit`) e
 * `EvaluateOutcome(0, 0)` restituisce `Draw`. Il commento di `RTTurnRules.h` scrive anche il perche':
 * *«senza una precedenza fissa, una squadra azzerata nello stesso Cleanup in cui l'altra tocca la soglia
 * darebbe un esito dipendente dall'ordine dei controlli»*.
 *
 * Quindi il soggetto qui **non e' che la politica esista**: e' che una partita vera ci **arrivi**. Cioe' che
 * due unita' che si uccidono a vicenda nello stesso Blast producano quell'esito e non un vincitore deciso
 * dall'ordine in cui il resolver le ha visitate — che e' precisamente l'«ordine emergente» che il DoD vieta.
 *
 * Le ripetizioni servono a distinguere una politica da una coincidenza: un ordine emergente stabile e' pur
 * sempre un ordine emergente, ma un esito che cambia fra due esecuzioni identiche lo dimostra da solo. La
 * permutazione dell'ordine di spawn e' il secondo verso della stessa domanda.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattleSimultaneousKOTest,
	"RefactorTactics.Match.Autobattle.SimultaneousKOFollowsDeclaredPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattleSimultaneousKOTest::RunTest(const FString&)
{
	// `bTeam0First` permuta l'ordine di inserimento: se l'esito dipendesse da chi entra prima, si vedrebbe qui.
	// Nessuna cattura: il lambda ESEGUE e riporta, le assertion restano fuori. Catturare `this` per non
	// usarlo suggerirebbe a chi legge che qui dentro qualcosa venga verificato, e non e' cosi'.
	auto RunLethalExchange = [](bool bTeam0First, ERTMatchOutcome& OutOutcome,
		ERTMatchEndReason& OutReason, int32& OutTeam0Alive, int32& OutTeam1Alive) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!World) { return false; }
		SpawnAutobattleMap(World);

		// Adiacenti, quindi entrambe a portata dell'attacco base. Fuori dal bot: i piani li scrive il test,
		// perche' il soggetto e' la POLITICA di fine partita, non le preferenze dell'utility.
		const FRTCellId CellA(0, 0, 0);
		const FRTCellId CellB(1, 0, 0);
		ARTUnit* First = nullptr;
		ARTUnit* Second = nullptr;
		if (bTeam0First)
		{
			First  = SpawnAutobattleUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), CellA, false);
			Second = SpawnAutobattleUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), CellB, false);
		}
		else
		{
			Second = SpawnAutobattleUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), CellB, false);
			First  = SpawnAutobattleUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), CellA, false);
		}
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !First || !Second) { RTWorldFixtures::DestroyWorld(World); return false; }

		// Un colpo solo basta per entrambe: e' cosi' che il KO diventa simultaneo invece che sequenziale.
		for (ARTUnit* U : { First, Second })
		{
			U->Health = 1;
			U->Shield = 0;
		}

		First->PlannedAbilityIndex = 0;              // l'attacco base e' sempre all'indice 0 del kit
		First->PlannedAttackTarget = Second;
		Second->PlannedAbilityIndex = 0;
		Second->PlannedAttackTarget = First;

		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

		CountAutobattleAlive(World, OutTeam0Alive, OutTeam1Alive);
		const FRTMatchResult Result = TM->GetMatchResult();
		OutOutcome = Result.Outcome;
		OutReason = Result.Reason;

		RTWorldFixtures::DestroyWorld(World);
		return true;
	};

	ERTMatchOutcome FirstOutcome = ERTMatchOutcome::InProgress;
	ERTMatchEndReason FirstReason = ERTMatchEndReason::None;
	int32 Team0Alive = -1, Team1Alive = -1;
	if (!TestTrue(TEXT("lo scambio letale si esegue"),
		RunLethalExchange(true, FirstOutcome, FirstReason, Team0Alive, Team1Alive)))
	{
		return false;
	}

	// PREMESSA: il KO e' davvero simultaneo. Se una delle due fosse sopravvissuta, tutto il resto del test
	// parlerebbe di un caso diverso da quello che dichiara — e sarebbe verde per il motivo sbagliato.
	if (!TestEqual(TEXT("premessa: la squadra 0 e' azzerata"), Team0Alive, 0)
		|| !TestEqual(TEXT("premessa: e anche la squadra 1, nello stesso turno"), Team1Alive, 0))
	{
		AddError(TEXT("il KO non e' stato simultaneo: il caso limite non e' stato esercitato. Non aggiustare ")
			TEXT("le attese di questo test — significa che la fase Blast ha applicato i danni in sequenza, ")
			TEXT("cioe' proprio l'ordine emergente che il DoD di #958 vieta."));
		return false;
	}

	// LA POLITICA, pinnata: entrambe azzerate significa pareggio, per eliminazione. Non un vincitore.
	TestEqual(TEXT("due squadre azzerate danno un pareggio DICHIARATO"), FirstOutcome, ERTMatchOutcome::Draw);
	TestEqual(TEXT("e la via e' l'eliminazione"), FirstReason, ERTMatchEndReason::Elimination);
	// La controprova sulla regola pura: il test sopra passa attraverso la partita, questa dice che l'esito
	// osservato e' quello che la regola prescrive, non una coincidenza dell'orchestratore.
	TestEqual(TEXT("ed e' cio' che la regola pura prescrive"),
		URTTurnRules::EvaluateOutcome(0, 0), ERTMatchOutcome::Draw);

	// DETERMINISMO: ripetuto, e con l'ordine di inserimento invertito.
	for (int32 Repetition = 1; Repetition <= 5; ++Repetition)
	{
		const bool bTeam0First = (Repetition % 2) == 1;
		ERTMatchOutcome Outcome = ERTMatchOutcome::InProgress;
		ERTMatchEndReason Reason = ERTMatchEndReason::None;
		int32 T0 = -1, T1 = -1;
		if (!RunLethalExchange(bTeam0First, Outcome, Reason, T0, T1))
		{
			AddError(FString::Printf(TEXT("ripetizione %d non eseguibile"), Repetition));
			return false;
		}
		TestEqual(FString::Printf(TEXT("ripetizione %d (%s per prima): stesso esito"),
				Repetition, bTeam0First ? TEXT("squadra 0") : TEXT("squadra 1")),
			Outcome, FirstOutcome);
		TestEqual(FString::Printf(TEXT("ripetizione %d: stessa via"), Repetition), Reason, FirstReason);
		// Non «stessi vivi»: ZERO vivi. Un esito uguale su due partite in cui qualcuno sopravvive sarebbe
		// stabile senza essere il caso limite che questo test dichiara di esercitare.
		TestEqual(FString::Printf(TEXT("ripetizione %d: entrambe le squadre azzerate"), Repetition),
			T0 + T1, 0);
	}
	return true;
}

/**
 * CASO 6/7 — `TurnLimit`: la partita non presidiata finisce per `RoundLimit`, e lo DICHIARA.
 *
 * La via esiste dal CP 10.3 (`RT-FEAT-MATCH-END-CONDITIONS`) ed e' coperta in `RTMatchEndTests` sulla regola
 * pura. Quello che mancava e' **in autobattle**: che una partita che nessuno guarda, in cui nessuno muore,
 * si chiuda comunque invece di girare fino al tetto di sicurezza del test.
 *
 * ⚠️ Distinguere le due chiusure e' l'intero punto. Un `MaxTurns` raggiunto e' un difetto — la partita e'
 * bloccata e il test lo maschera; un `RoundLimit` raggiunto e' la **regola** che ha deciso. Le due si
 * somigliano dall'esterno (in entrambe la partita smette) e si distinguono solo guardando `Reason`.
 *
 * Le unita' sono fuori dal bot e ferme: un bot le porterebbe allo scontro e la partita finirebbe per
 * eliminazione prima del limite, cioe' per l'altra via. Qui il soggetto e' la scadenza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattleRoundLimitTest,
	"RefactorTactics.Match.Autobattle.EndsOnRoundLimitWhenNobodyDies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattleRoundLimitTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	SpawnAutobattleMap(World);

	ARTUnit* A = SpawnAutobattleUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-4, 2), false);
	ARTUnit* B = SpawnAutobattleUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(4, -2), false);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !A || !B)
	{
		AddError(TEXT("allestimento fallito"));
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Un limite corto, per non far dipendere il test dalla durata di una partita vera. `ScoreToWin = 0`
	// tiene spenta la via obiettivo: con due vie attive non si saprebbe quale ha chiuso.
	FRTMatchRules Rules;
	Rules.FormatId = FName(TEXT("Format.AutobattleRoundLimitProbe"));
	Rules.RoundLimit = 3;
	Rules.ScoreToWin = 0;
	Rules.UnitsPerTeam = 1;
	TM->SetMatchRules(Rules);

	// Premessa: il limite e' davvero in vigore. `RoundLimit <= 0` disattiva la via, e il test misurerebbe
	// una partita che semplicemente non finisce.
	if (!TestEqual(TEXT("premessa: il limite di round e' in vigore"), TM->GetMatchRules().RoundLimit, 3))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Il tetto e' piu' alto del limite: se la partita finisse per esaurimento del tetto invece che per la
	// regola, il test lo direbbe con `bHitSafetyCap` invece di confonderlo con un successo.
	FRTAutobattleTrace Trace;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Trace.TurnsPlayed < 10)
	{
		TM->LockInAndResolve();   // nessuno pianifica: nessuno muore, e la sola via aperta e' la scadenza
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
		Trace.Turns.Add(URTTurnLogLibrary::SerializeTurnLog(TM->GetTurnLog(), ERTLogTopology::Hex));
		++Trace.TurnsPlayed;
	}
	Trace.bHitSafetyCap = (TM->GetPhase() != ERTMatchPhase::MatchEnded);

	TestFalse(TEXT("la partita NON e' finita per esaurimento del tetto di sicurezza"), Trace.bHitSafetyCap);
	TestEqual(TEXT("si e' chiusa al terzo round, cioe' al limite"), Trace.TurnsPlayed, 3);

	const FRTMatchResult Result = TM->GetMatchResult();
	TestEqual(TEXT("e la via dichiarata e' la scadenza dei round"), Result.Reason, ERTMatchEndReason::RoundLimit);
	// A punteggi pari il pareggio e' DICHIARATO, mai un vincitore scelto per posizione (spec §12).
	TestEqual(TEXT("a punteggi pari l'esito e' un pareggio, non un vincitore per posizione"),
		Result.Outcome, ERTMatchOutcome::Draw);
	TestTrue(TEXT("nessuno e' caduto: la partita non si e' chiusa per eliminazione"),
		A->IsAlive() && B->IsAlive());

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * CASO 7/7 — `SameSeedSameResult`: due esecuzioni identiche danno la stessa partita.
 *
 * ⚠️ **OGGI E' VERO PER COSTRUZIONE, e il test lo PINNA invece di introdurlo.** Il runtime non ha alcun RNG:
 * zero `FRandomStream`, zero `FMath::Rand`, e `FRTTestScenario::Seed` e' documentato come «dichiarato ma non
 * consumato». Finche' e' cosi', questo test confronta una funzione deterministica con se' stessa — e va
 * scritto sapendolo. **Il suo valore e' il giorno in cui smette di farlo**: quel giorno diventa l'unico
 * posto in cui si vede che un RNG e' entrato nella partita non presidiata.
 *
 * Non e' un doppione di `Replay.Verifier.ResimulationIsDeterministic` (G4): quello ri-simula uno **scenario**
 * attraverso il resolver e confronta il checksum su 100 ripetizioni. Qui il soggetto e' una **partita
 * autobattle intera**, guidata dal bot, e il confronto e' sul TurnLog turno per turno — che e' l'unica forma
 * in cui una divergenza sa dire dove.
 *
 * Le ripetizioni sono 10 e non 100: quel numero e' il gate G4 e vive dov'e'. Qui ogni ripetizione e' una
 * partita completa di dieci turni, e centinaia di partite pagherebbero il tempo di tutta la suite per una
 * proprieta' che il gate copre gia' nella sua forma vincolante.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattleSameSeedTest,
	"RefactorTactics.Match.Autobattle.SameSeedGivesSameResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattleSameSeedTest::RunTest(const FString&)
{
	// Nessuna cattura, come in `SimultaneousKO`: esegue e riporta, le assertion restano fuori.
	auto RunOnce = [](FRTAutobattleTrace& Out) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!World) { return false; }
		SpawnAutobattleMap(World);

		const TArray<ARTUnit*> Units = DeployAutobattleRoster(World, { 0, 1, 2, 3 });
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || Units.Contains(nullptr)) { RTWorldFixtures::DestroyWorld(World); return false; }

		Out = PlayAutobattleMatch(TM);
		RTWorldFixtures::DestroyWorld(World);
		return true;
	};

	FRTAutobattleTrace Reference;
	if (!TestTrue(TEXT("la partita di riferimento si esegue"), RunOnce(Reference))) { return false; }

	// Premesse: senza, dieci confronti fra partite mai giocate sarebbero dieci verdi che non provano nulla.
	if (!TestTrue(TEXT("premessa: la partita e' andata oltre il primo turno"), Reference.TurnsPlayed > 1))
	{
		return false;
	}
	TestFalse(TEXT("premessa: si e' decisa entro il tetto di sicurezza"), Reference.bHitSafetyCap);
	TestTrue(TEXT("premessa: ogni turno ha lasciato una traccia"),
		Reference.Turns.Num() == Reference.TurnsPlayed);
	// Una traccia vuota confrontata con un'altra traccia vuota e' identica, e non dice niente.
	int32 EmptyTurns = 0;
	for (const TArray<uint8>& Bytes : Reference.Turns) { EmptyTurns += (Bytes.Num() == 0) ? 1 : 0; }
	TestEqual(TEXT("premessa: nessun turno ha prodotto una traccia vuota"), EmptyTurns, 0);

	constexpr int32 Repetitions = 10;
	int32 Divergences = 0;
	for (int32 I = 1; I < Repetitions; ++I)
	{
		FRTAutobattleTrace Again;
		if (!RunOnce(Again))
		{
			AddError(FString::Printf(TEXT("ripetizione %d non eseguibile"), I));
			return false;
		}
		const FString Divergence = DescribeAutobattleDivergence(
			TEXT("riferimento"), Reference, *FString::Printf(TEXT("ripetizione %d"), I), Again);
		if (!Divergence.IsEmpty())
		{
			++Divergences;
			if (Divergences <= 3)   // le prime tre bastano a diagnosticare; oltre e' rumore
			{
				AddError(FString::Printf(TEXT("ripetizione %d: %s"), I, *Divergence));
			}
		}
	}

	TestEqual(FString::Printf(TEXT("nessuna divergenza su %d partite identiche"), Repetitions),
		Divergences, 0);
	AddInfo(FString::Printf(
		TEXT("%d turni per partita. Oggi questo test e' vero per costruzione (nessun RNG nel runtime): ")
		TEXT("diventa significativo il giorno in cui `RNG-1` (#960) viene deciso a favore della varieta'."),
		Reference.TurnsPlayed));
	return true;
}

/**
 * #1088 — LA MAPPA NON E' LA CAUSA: sulla sorgente che il giocatore ottiene, i bot ingaggiano al turno 2.
 *
 * Nato come riproduttore dello stallo, **e' diventato la sua confutazione** — ed e' il suo valore. #1088
 * elencava tre cause candidate, «nessuna misurata»: la linea di tiro interrotta dalla geometria, il pathing
 * che non trova una cella da cui sparare, i pesi dell'utility. Questo test le esclude tutte e tre.
 *
 * La differenza con `Match.Autobattle.PlaysToCompletionWithoutInput` e' UNA riga, ed e' il criterio 3 di
 * #1088: quel test allestisce la board con `SpawnAutobattleMap`, un esagono **liscio** costruito nel test.
 * Qui la mappa arriva da `MapSource = GeneratedTestArena`, cioe' da `ARTGameMode::ApplyMapSource` — 65
 * celle con tre ostacoli, una fascia `Rough` e **il muro di cinque celle che blocca la vista su `q=0`**,
 * che e' l'unica differenza geometrica fra le due arene.
 *
 * **Misurato il 2026-08-22, ed e' il numero che sposta l'indagine**: `primo Combat al turno 2 · 19 voci
 * Combat · 49 Move · 12 turni`. Con il muro, con lo spawn del GameMode, con la stessa sorgente di mappa:
 * i bot **ingaggiano**. Quindi lo stallo di #1088 non e' spiegato dalla board.
 *
 * ⛔ **Dove NON cercare piu', e perche'.** La geometria blocca davvero la linea di tiro — riprodotto:
 * le unita' si fermano a distanza 3 e la linea fra loro attraversa il muro — ma non basta: l'attacco vale
 * solo dalla cella in cui il bot si trova nel Blast (il `Move` risolve DOPO), e il movimento rapido
 * lineare offre comunque celle da cui sparare. L'aritmetica dei pesi le premia. Il bot le usa.
 *
 * 🔴 **Dove cercare invece.** Questo test istanzia `ARTGameMode`, la classe C++. La partita usa
 * **`BP_GameMode`**, che serializza due proprieta' che il C++ non ha: `MapSource = GeneratedTestArena` e
 * `MatchFormat = /Game/RT/Maps/Dev/L_DevSandbox/Data/DA_Format_Rotto` — un asset **non versionato e
 * assente da questo clone**, cioe' un puntatore rotto. E' esattamente cio' che #1069 descrive. Un difetto
 * che vive in un `.uasset` non e' riproducibile da un test C++ che quel `.uasset` non carica: e' la
 * ragione per cui #1088 e' aperta da giorni con tre ipotesi e nessuna misura.
 *
 * ⚠️ Il test resta **verde** e va tenuto: e' la regressione che impedisce di tornare a incolpare la mappa,
 * e il suo `AddInfo` stampa il turno del primo colpo — se un domani superasse i 12 round del formato, il
 * giocatore vedrebbe un pareggio senza che nessuna asserzione cambi. Per questo il turno e' asserito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattleEngagesOnShippedMapSourceTest,
	"RefactorTactics.Match.Autobattle.EngagesOnTheShippedMapSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattleEngagesOnShippedMapSourceTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	FRTScopedAutobattleCVars CVarGuard;
	FRTScopedAutobattleCommandLine CmdGuard;
	CVarGuard.SetMode(-1);
	CmdGuard.Clear();

	// La mappa NON si costruisce qui: si lascia che sia il GameMode a produrla dalla propria sorgente, che
	// e' il punto del test. L'actor entra vuoto, `ApplyMapSource` gli assegna `MakeTestArena`.
	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("TurnManager"), TM)
		|| !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// I due Warning che l'allestimento emette sono la scelta dichiarata, non un difetto — e vanno attesi
	// PRIMA che l'azione li produca: registrati dopo, il framework li conta come non avvenuti.
	AddExpectedError(TEXT("MapSource=GeneratedTestArena"), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedError(TEXT("AUTOBATTLE"), EAutomationExpectedErrorFlags::Contains, 1);

	GameMode->MapSource = ERTMapSource::GeneratedTestArena;
	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(HexMap);

	// La board deve essere quella con il muro, o il test misurerebbe un'altra partita.
	if (!TestNotNull(TEXT("il GameMode ha prodotto una mappa"), HexMap->MapAsset.Get()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	int32 SightBlockers = 0;
	for (const FRTHexCellData& Cell : HexMap->MapAsset->Cells)
	{
		if (Cell.bBlocksLineOfSight) { ++SightBlockers; }
	}
	TestEqual(TEXT("la board e' l'arena di prova: cinque celle bloccano la vista"), SightBlockers, 5);

	for (ARTUnit* Unit : CollectAutobattleUnits(World))
	{
		if (!Unit->HasActorBegunPlay()) { Unit->DispatchBeginPlay(); }
	}
	if (!TestEqual(TEXT("nessuna unita' aspetta una mano umana"), CountAutobattleHumanUnits(World), 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Tetto di sicurezza, non una regola: serve a fallire invece di girare all'infinito.
	const int32 MaxTurns = 40;
	int32 TurnsPlayed = 0;
	int32 CombatEntries = 0;
	int32 MoveEntries = 0;
	int32 FirstCombatTurn = 0;   // il turno del PRIMO colpo: se cade oltre il RoundLimit reale, in partita non si vede mai

	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && TurnsPlayed < MaxTurns)
	{
		TM->PlanBotsForTest();
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
		++TurnsPlayed;

		for (const FRTTurnLogEntry& Entry : TM->GetTurnLog())
		{
			if (Entry.Category == ERTLogCategory::Combat)
			{
				++CombatEntries;
				if (FirstCombatTurn == 0) { FirstCombatTurn = TurnsPlayed; }
			}
			if (Entry.Category == ERTLogCategory::Move)   { ++MoveEntries; }
		}
	}

	// Il movimento c'e' anche nello stallo — le unita' si avvicinano e poi si fermano. E' la meta' che NON
	// distingue una partita viva da una bloccata, e si misura per rendere leggibile il fallimento.
	TestTrue(TEXT("i bot si muovono"), MoveEntries > 0);

	// 🔴 L'ASSERZIONE CHE OGGI FALLISCE, ed e' il difetto: su questa sorgente di mappa i bot non ingaggiano
	// mai. Misurato il 2026-08-17: dodici round, ZERO `Combat`, pareggio allo scadere.
	TestTrue(FString::Printf(
		TEXT("su MapSource=GeneratedTestArena i bot ingaggiano: %d voci Combat in %d turni (attese > 0)"),
		CombatEntries, TurnsPlayed), CombatEntries > 0);

	// E il DoD di E47.1 chiede una conclusione per eliminazione, non per esaurimento dei round.
	//
	// 🔴 L'ORACOLO E' `Reason`, NON `Phase` — e la differenza e' esattamente il difetto di #1088.
	// `Phase = MatchEnded` si imposta per QUALUNQUE esito diverso da `InProgress` (`RTTurnManager.cpp`,
	// ramo `PendingResult.Outcome != ERTMatchOutcome::InProgress`), e `ERTMatchOutcome` contiene `Draw`
	// (`RTTurnRules.h`): un pareggio allo scadere dei round passava l'asserzione precedente, che leggeva
	// la fase. Il commento prometteva l'eliminazione e il codice non la verificava.
	//
	// L'idioma giusto era gia' in questo file, in `Match.Autobattle.PlaysToCompletionWithoutInput`:
	// «una partita chiusa dal limite di round e' la partita bloccata che il DoD chiede di distinguere,
	// non una demo riuscita». Il test che copre la sorgente SPEDITA usava l'oracolo piu' debole dei due.
	const FRTMatchResult Result = TM->GetMatchResult();
	TestTrue(FString::Printf(TEXT("la partita si decide entro il tetto: %d turni giocati"), TurnsPlayed),
		TM->GetPhase() == ERTMatchPhase::MatchEnded);
	TestEqual(FString::Printf(
		TEXT("e si decide per ELIMINAZIONE, non per esaurimento dei round: %s - %s al turno %d"),
		*URTTurnRules::DescribeOutcome(Result.Outcome),
		*URTTurnRules::DescribeEndReason(Result.Reason),
		TurnsPlayed),
		Result.Reason, ERTMatchEndReason::Elimination);

	// 🔴 IL NUMERO CHE CONTA: se il primo colpo cade oltre il limite del formato, in partita il giocatore
	// vede solo il pareggio di #1088 — e questo test, che ha un tetto piu' alto, non lo mostrerebbe.
	//
	// ⚠️ Il limite si CHIEDE al formato invece di scriverlo qui: era `<= 12` a mano, e il 2026-08-22 il
	// formato e' passato a 14 (#1088). Un numero letterale in un'asserzione invecchia da solo e nessun gate
	// se ne accorge, perche' resta verde mentre smette di misurare la cosa che nomina.
	const int32 FormatRoundLimit = TM->GetMatchRules().RoundLimit;
	AddInfo(FString::Printf(TEXT("primo Combat al turno %d · %d voci Combat · %d Move · %d turni totali"),
		FirstCombatTurn, CombatEntries, MoveEntries, TurnsPlayed));
	TestTrue(FString::Printf(TEXT("il primo colpo arriva entro i %d round del formato: turno %d"),
		FormatRoundLimit, FirstCombatTurn),
		FirstCombatTurn > 0 && FirstCombatTurn <= FormatRoundLimit);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
