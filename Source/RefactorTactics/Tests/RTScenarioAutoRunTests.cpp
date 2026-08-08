// Configurazione dell'AUTO-RUN: quale scenario parte al Play, e con che ritmo.
//
// Due cose che si sbagliano facilmente e che nessuno nota finche' non fanno perdere tempo davvero:
// - la PRECEDENZA fra la proprieta' persistente e l'override da riga di comando;
// - il timer di pianificazione lasciato a 30 secondi mentre si guarda uno scenario che dura un secondo.

#include "Misc/AutomationTest.h"
#include "RTGameMode.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnRules.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "ScenarioHarness/RTScenarioRunner.h"

#if WITH_DEV_AUTOMATION_TESTS

/** Definita in ScenarioHarness/RTTestConsole.cpp. */
extern TAutoConsoleVariable<FString> CVarRTTestScenario;

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	UWorld* MakeAutoRunWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyAutoRunWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/**
	 * Ripristina la console variable qualunque cosa succeda nel test.
	 *
	 * Senza, un test che la imposta e fallisce a meta' la lascerebbe sporca per TUTTI i test successivi —
	 * e in una unity build il successivo potrebbe essere qualsiasi cosa. Un test che rompe gli altri e' peggio
	 * di un test assente, perche' manda a cercare il difetto nel posto sbagliato.
	 */
	struct FScopedScenarioCVar
	{
		FString Saved;
		FScopedScenarioCVar() : Saved(CVarRTTestScenario.GetValueOnGameThread()) {}
		~FScopedScenarioCVar() { CVarRTTestScenario->Set(*Saved, ECVF_SetByCode); }
		void Set(const TCHAR* Value) { CVarRTTestScenario->Set(Value, ECVF_SetByCode); }
	};
}

/**
 * La console variable PREVALE sulla proprieta'.
 *
 * La proprieta' e' configurazione persistente («questo progetto, per ora, esegue questo scenario»); la console
 * variable e' l'intento estemporaneo di chi lancia («adesso, solo per questa volta, un altro»). Il piu'
 * specifico vince — la stessa regola di ogni override di configurazione. Se si invertisse, impostare la
 * proprieta' renderebbe impossibile eseguire uno scenario diverso da riga di comando, che e' esattamente cio'
 * che serve in CI.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutoRunScenarioPrecedenceTest,
	"RefactorTactics.Scenario.AutoRunConsoleOverridesProperty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutoRunScenarioPrecedenceTest::RunTest(const FString&)
{
	UWorld* World = MakeAutoRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("game mode"), GameMode)) { DestroyAutoRunWorld(World); return false; }

	FScopedScenarioCVar Guard;

	// Nessuna delle due: partita normale.
	Guard.Set(TEXT(""));
	GameMode->ScenarioToRun.Reset();
	TestTrue(TEXT("niente proprieta' e niente console -> partita normale"),
		GameMode->ResolveScenarioToRun().IsEmpty());

	// Solo la proprieta': e' lei a decidere. E' il caso «imposto una volta in BP_GameMode e al Play parte».
	GameMode->ScenarioToRun = TEXT("Movement.Basic");
	TestEqual(TEXT("solo proprieta' -> vale la proprieta'"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Movement.Basic")));

	// Entrambe: vince la console variable.
	Guard.Set(TEXT("Movement.Collision"));
	TestEqual(TEXT("console + proprieta' -> vince la console"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Movement.Collision")));

	// Solo la console: vale comunque, anche senza proprieta' (il caso della riga di comando).
	GameMode->ScenarioToRun.Reset();
	TestEqual(TEXT("solo console -> vale la console"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Movement.Collision")));

	DestroyAutoRunWorld(World);
	return true;
}

/**
 * Cambiare la durata della pianificazione vale SUBITO, non dal turno dopo.
 *
 * E' il punto della modifica: uno scenario lascia il turn manager in pianificazione, e col timer normale si
 * resterebbe a guardare un turno vuoto per mezzo minuto. Se il valore nuovo valesse solo dal turno successivo,
 * il primo mezzo minuto lo si aspetterebbe lo stesso — cioe' proprio nel turno in cui serviva non farebbe niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningSecondsAppliesImmediatelyTest,
	"RefactorTactics.Turn.PlanningSecondsAppliesImmediately",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningSecondsAppliesImmediatelyTest::RunTest(const FString&)
{
	UWorld* World = MakeAutoRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyAutoRunWorld(World); return false; }

	TestEqual(TEXT("valore di partenza: 30 s, quelli della partita normale"), TM->GetPlanningSeconds(), 30.f);

	TM->SetPlanningSeconds(3.f);
	TestEqual(TEXT("il valore nuovo e' quello letto"), TM->GetPlanningSeconds(), 3.f);

	// 0 = nessuna scadenza: serve a fermare l'immagine e guardare con calma, e non deve essere confuso con
	// «un timer da zero secondi» che farebbe scattare il lock-in immediatamente.
	TM->SetPlanningSeconds(0.f);
	TestEqual(TEXT("zero e' ammesso (nessuna scadenza)"), TM->GetPlanningSeconds(), 0.f);

	// Un valore negativo non deve diventare un timer impossibile: si porta a zero.
	TM->SetPlanningSeconds(-5.f);
	TestEqual(TEXT("un negativo diventa zero, non un timer assurdo"), TM->GetPlanningSeconds(), 0.f);

	DestroyAutoRunWorld(World);
	return true;
}

/** Il ritmo dello scenario e' configurabile dal GameMode, con un default che non fa aspettare. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioPacingDefaultTest,
	"RefactorTactics.Scenario.AutoRunPacingHasShortDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioPacingDefaultTest::RunTest(const FString&)
{
	UWorld* World = MakeAutoRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("game mode"), GameMode)) { DestroyAutoRunWorld(World); return false; }

	// Il default deve essere BREVE ma non nullo: qualche secondo per vedere il campo, non mezzo minuto di
	// attesa ne' un salto istantaneo che non lascia guardare niente.
	TestTrue(TEXT("il ritmo di default e' positivo"), GameMode->ScenarioTurnPauseSeconds >= 0.f);
	TestTrue(TEXT("ed e' molto piu' corto della pianificazione normale (30 s)"),
		GameMode->ScenarioTurnPauseSeconds < 10.f);

	DestroyAutoRunWorld(World);
	return true;
}

/**
 * Il menu a tendina di `ScenarioToRun` elenca gli scenari REALI, e permette di tornare alla partita normale.
 *
 * Legge i file invece di un elenco scritto a mano: se domani qualcuno aggiunge uno scenario, deve comparire
 * nel menu senza toccare il codice. Un elenco mantenuto a mano invecchierebbe alla prima aggiunta, e il primo
 * a soffrirne sarebbe chi cerca uno scenario che c'e' ma non si vede.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioOptionsAreRealFilesTest,
	"RefactorTactics.Scenario.AutoRunOptionsComeFromFiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioOptionsAreRealFilesTest::RunTest(const FString&)
{
	UWorld* World = MakeAutoRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("game mode"), GameMode)) { DestroyAutoRunWorld(World); return false; }

	const TArray<FString> Options = GameMode->GetScenarioOptions();
	DestroyAutoRunWorld(World);

	// La voce vuota e' in TESTA: e' come si torna alla partita normale dal menu.
	if (TestTrue(TEXT("il menu non e' vuoto"), Options.Num() > 0))
	{
		TestTrue(TEXT("la prima voce e' vuota (= partita normale)"), Options[0].IsEmpty());
	}

	// Gli ID veri ci sono, e vengono dai file: se qualcuno ne aggiunge uno, compare qui da solo.
	TestTrue(TEXT("Movement.Basic e' selezionabile"), Options.Contains(TEXT("Movement.Basic")));
	TestTrue(TEXT("Movement.Collision e' selezionabile"), Options.Contains(TEXT("Movement.Collision")));

	// Il menu combacia con l'elenco dei file, meno la voce vuota aggiunta apposta: nessun ID inventato,
	// nessuno scenario esistente lasciato fuori.
	TestEqual(TEXT("tante voci quanti gli scenari, piu' quella vuota"),
		Options.Num(), URTScenarioRunner::ListScenarioIds().Num() + 1);
	return true;
}

/**
 * I filtri restringono la tendina, e NON toccano la selezione.
 *
 * Sono una vista, non un vincolo: filtrare dice «cosa sto cercando adesso», non «a cosa questo scenario
 * appartiene». Uno scenario gia' scelto deve restare scelto anche mentre i filtri mostrano altro —
 * azzerarlo sarebbe un modo elaborato di perdere una configurazione salvata nel `.uasset`.
 *
 * E' anche la ragione per cui non esiste piu' un errore di «category mismatch»: con una lente non c'e'
 * niente da cui uno scenario possa essere incoerente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioOptionsFilterTest,
	"RefactorTactics.Scenario.AutoRunOptionsAreFilteredByTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioOptionsFilterTest::RunTest(const FString&)
{
	UWorld* World = MakeAutoRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("game mode"), GameMode)) { DestroyAutoRunWorld(World); return false; }

	const TArray<FString> SenzaFiltri = GameMode->GetScenarioOptions();

	// Il vocabolario dei filtri viene dai file, con la voce vuota in testa per non filtrare.
	const TArray<FString> Vocabolario = GameMode->GetScenarioTagOptions();
	const bool bVocabolarioOk = Vocabolario.Num() > 1 && Vocabolario[0].IsEmpty();
	TestTrue(TEXT("il vocabolario dei tag ha la voce vuota in testa"), bVocabolarioOk);
	TestTrue(TEXT("'movement' e' fra i filtri offerti"), Vocabolario.Contains(TEXT("movement")));

	// Uno scenario scelto PRIMA di filtrare: e' quello che non deve muoversi.
	GameMode->ScenarioToRun = TEXT("Combat.BasicAttack");

	GameMode->ScenarioFilterA = TEXT("movement");
	const TArray<FString> Filtrato = GameMode->GetScenarioOptions();

	GameMode->ScenarioFilterB = TEXT("core");
	const TArray<FString> Incrociato = GameMode->GetScenarioOptions();

	const FString SelezioneDopo = GameMode->ScenarioToRun;
	DestroyAutoRunWorld(World);

	TestTrue(TEXT("un filtro restringe il menu"), Filtrato.Num() < SenzaFiltri.Num());
	TestTrue(TEXT("due filtri restringono ancora"), Incrociato.Num() < Filtrato.Num());
	TestTrue(TEXT("la voce vuota resta in testa anche filtrando"), Incrociato.Num() > 0 && Incrociato[0].IsEmpty());
	TestTrue(TEXT("Movement.Basic passa movement+core"), Incrociato.Contains(TEXT("Movement.Basic")));
	TestFalse(TEXT("Combat.BasicAttack non passa il filtro movement"), Filtrato.Contains(TEXT("Combat.BasicAttack")));

	// Il punto del test: la selezione e' sopravvissuta al filtro che la esclude dalla vista.
	TestEqual(TEXT("filtrare non tocca lo scenario selezionato"), SelezioneDopo, FString(TEXT("Combat.BasicAttack")));
	return true;
}

/**
 * L'override NON deve essere silenzioso.
 *
 * Una console variable dura quanto il processo dell'editor: digitata una volta resta attiva per ogni Play
 * successivo e continua a scavalcare la tendina del Details Panel senza che nulla lo dica. E' successo
 * davvero — si sceglieva uno scenario e ne partiva un altro, con l'unico indizio nel comportamento a schermo,
 * e la diagnosi e' costata la lettura dei log invece di uno sguardo.
 *
 * La precedenza resta quella giusta; ad essere sbagliato era il silenzio. Qui si verifica che il caso
 * «entrambe impostate e diverse» produca un avviso, e che i casi innocui NON lo producano — un avviso che
 * compare sempre e' rumore, e il rumore si impara a ignorare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutoRunOverrideIsAnnouncedTest,
	"RefactorTactics.Scenario.AutoRunOverrideIsNotSilent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutoRunOverrideIsAnnouncedTest::RunTest(const FString&)
{
	UWorld* World = MakeAutoRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("game mode"), GameMode)) { DestroyAutoRunWorld(World); return false; }

	FScopedScenarioCVar Guard;

	// CASO CHE SCOTTA: proprieta' e console impostate su scenari DIVERSI -> avviso.
	GameMode->ScenarioToRun = TEXT("Movement.LongWalk");
	Guard.Set(TEXT("Movement.Collision"));
	AddExpectedError(TEXT("SCAVALCA la proprieta'"), EAutomationExpectedErrorFlags::Contains, 1);
	TestEqual(TEXT("vince comunque la console"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Movement.Collision")));

	// CASI INNOCUI: nessun avviso, altrimenti diventa rumore.
	// (Se ne producessero uno, il test fallirebbe: un errore non atteso e' un fallimento in Automation.)
	Guard.Set(TEXT(""));
	TestEqual(TEXT("solo proprieta': nessun conflitto da segnalare"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Movement.LongWalk")));

	GameMode->ScenarioToRun.Reset();
	Guard.Set(TEXT("Movement.Collision"));
	TestEqual(TEXT("solo console: nessun conflitto da segnalare"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Movement.Collision")));

	GameMode->ScenarioToRun = TEXT("Movement.Collision");
	TestEqual(TEXT("entrambe UGUALI: niente da segnalare"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Movement.Collision")));

	DestroyAutoRunWorld(World);
	return true;
}

/**
 * La banda a schermo dice che questa NON e' una partita normale.
 *
 * Il difetto che chiude: con `ScenarioToRun` impostato, `BeginPlay` esegue lo scenario e ritorna, quindi la
 * partita normale non viene allestita — niente unita' proprie, niente selezione, niente barra abilita'. Chi
 * guarda vede uno schermo quasi vuoto. La spiegazione esisteva gia', ma solo nell'Output Log, che non si ha
 * motivo di aprire quando il sintomo sembra «il gioco non parte».
 *
 * Verificato il 2026-08-08 dall'utente, seguendo le istruzioni di questa stessa checklist: uscendo da
 * `PIE-SCEN-KEEP` la property resta impostata, e il Play successivo non e' una partita.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutoRunBannerTest,
	"RefactorTactics.Scenario.AutoRunBannerDeclaresItIsNotAMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutoRunBannerTest::RunTest(const FString&)
{
	FScopedScenarioCVar Guard;
	Guard.Set(TEXT(""));

	UWorld* World = MakeAutoRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("game mode"), GameMode)) { DestroyAutoRunWorld(World); return false; }

	// Partita normale: nessuna banda. Una banda sempre accesa sarebbe rumore, e il rumore non si legge.
	GameMode->ScenarioToRun.Reset();
	TestTrue(TEXT("partita normale: nessuna banda"), GameMode->GetScenarioBannerText().IsEmpty());

	// Scenario dalla property.
	GameMode->ScenarioToRun = TEXT("Combat.BasicAttack");
	const FString FromProperty = GameMode->GetScenarioBannerText();
	TestFalse(TEXT("con uno scenario impostato la banda c'e'"), FromProperty.IsEmpty());
	TestTrue(TEXT("la banda nomina lo scenario"), FromProperty.Contains(TEXT("Combat.BasicAttack")));
	// E' la frase che risolve la confusione: senza, la banda direbbe cosa sta girando ma non cosa MANCA.
	TestTrue(TEXT("la banda dice che la partita normale non e' allestita"),
		FromProperty.Contains(TEXT("NON e' allestita")));
	TestTrue(TEXT("la banda attribuisce la scelta al BP_GameMode"), FromProperty.Contains(TEXT("BP_GameMode")));

	// La console variable prevale, e la banda deve dirlo: e' la fonte che si dimentica di aver impostato,
	// perche' dura quanto il processo dell'editor e scavalca la tendina a ogni Play.
	Guard.Set(TEXT("Movement.Basic"));
	const FString FromCVar = GameMode->GetScenarioBannerText();
	TestTrue(TEXT("la banda segue la cvar che prevale"), FromCVar.Contains(TEXT("Movement.Basic")));
	TestTrue(TEXT("la banda attribuisce la scelta alla cvar"), FromCVar.Contains(TEXT("rt.Test.Scenario")));
	TestFalse(TEXT("non attribuisce anche al BP_GameMode"), FromCVar.Contains(TEXT("BP_GameMode")));

	// Nessuna sessione avviata: lo stato e' «in corso», non un esito inventato.
	TestTrue(TEXT("senza sessione la banda non dichiara un esito"), FromCVar.Contains(TEXT("in corso")));

	DestroyAutoRunWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
