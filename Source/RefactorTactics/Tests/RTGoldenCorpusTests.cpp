#include "Misc/AutomationTest.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "ScenarioHarness/RTTestResult.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Corpus golden di TurnLog (CP 12.6, issue #178) — la DIAGNOSI della divergenza.
 *
 * Il DoD e' esplicito sul punto che rende utile un corpus: «una divergenza **fallisce indicando turno, fase e
 * `ActionId`**: un test che dice solo "hash diverso" non serve a nessuno». Un confronto che risponde
 * `Divergence` e basta lascia a chi legge il lavoro di ricostruire *dove*, ed e' il motivo per cui i corpus
 * golden vengono rigenerati invece che letti.
 *
 * `URTTurnLogLibrary::CompareSerializedTraces` esiste gia' e risponde al livello del FORMATO
 * (`Identical`/`FormatMismatch`/`TopologyMismatch`/`Divergence`/`Unreadable`). Qui si aggiunge il livello
 * sotto: quale voce, in quale fase, di quale azione.
 *
 * ⚠️ **SE QUESTI TEST SONO DIVENTATI ROSSI DOPO UN RITOCCO DI BILANCIAMENTO, non e' una regressione.**
 * ([D-083](../../../docs/decisions/RT_PDR_00_Decision_Log.md), issue `#413`.)
 *
 * Il corpus golden e' protetto da una **convenzione**, non da un controllo: i file `.rttl` di `Golden/` vivono
 * nello stesso repository delle regole, quindi cambiare una costante di combat o un'azione del catalogo li fa
 * divergere: la convenzione e' che si **rigenerano nello stesso commit** che cambia la regola.
 *
 * Il rilevamento c'e' ed e' buono — la diagnosi qui sotto nomina turno, fase e `ActionId`. Cio' che manca e'
 * l'**attribuzione**: un rebalance legittimo e un difetto del resolver si presentano **identici**, e a
 * distinguerli e' solo chi ha il commit in mano. Sarebbe il mestiere di `ContentManifestHash`/`RulesVersion`,
 * che D-083 ha rinviato alla v0.2 **con il perimetro gia' deciso** — entra cio' che il resolver legge — perche'
 * in v0.1 quel «chi ha il commit in mano» c'e' sempre, ed e' una persona sola.
 *
 * L'innesco per costruirle: **quando un archivio esce dalla macchina che l'ha prodotto** (condivisione, bug
 * report, CI che confronta run di build diverse). Li' l'attribuzione smette di essere locale.
 */
namespace
{
	/** Voce minima: i campi che la diagnosi deve saper nominare. */
	FRTTurnLogEntry MakeGoldenEntry(ERTMatchPhase Phase, ERTLogCategory Category, uint8 Outcome,
		const TCHAR* ActionId, int32 Amount = 0)
	{
		FRTTurnLogEntry E;
		E.Phase = Phase;
		E.Category = Category;
		E.Outcome = Outcome;
		E.ActionId = FName(ActionId);
		E.Amount = Amount;
		E.SrcCell = FRTCellId(1, 0);
		E.TgtCell = FRTCellId(2, 0);
		return E;
	}

	/** Traccia di riferimento: tre voci, una per fase, con azioni distinguibili. */
	TArray<FRTTurnLogEntry> GoldenSample()
	{
		TArray<FRTTurnLogEntry> Log;
		Log.Add(MakeGoldenEntry(ERTMatchPhase::Prep, ERTLogCategory::Reaction,
			0, TEXT("Action.Guard")));
		Log.Add(MakeGoldenEntry(ERTMatchPhase::Blast, ERTLogCategory::Combat,
			static_cast<uint8>(ERTCombatOutcome::Hit), TEXT("Hero.Wraith.PulseShot"), 21));
		Log.Add(MakeGoldenEntry(ERTMatchPhase::Move, ERTLogCategory::Move,
			static_cast<uint8>(ERTMoveOutcome::Moved), TEXT("Action.Move"), 2));
		return Log;
	}
}

/**
 * Ogni campo che DISCRIMINA una voce viene nominato dalla diagnosi, coi suoi due valori.
 *
 * Non e' un elenco di casi scelti a mano: e' l'elenco COMPLETO di cio' che `GoldenEntriesMatch` guarda.
 * Quel confronto e' `HashTurnLogOrdered({A}) == HashTurnLogOrdered({B})`, quindi l'insieme dei campi che
 * possono far divergere due voci **e' per costruzione** l'insieme che l'hash mescola — e il report che ne
 * stampava quattro su dieci non era incompleto per distrazione: erano due elenchi della stessa cosa in due
 * posti, e sono divergiuti tre volte (`ActionId`, `TgtCell`, `GraphRevision`).
 *
 * Oggi l'elenco e' uno solo (`VisitDiscriminatingFields`) e lo percorrono sia l'hash sia il report, quindi
 * un campo nuovo entra in entrambi o in nessuno dei due. Questo test lo pinna dal lato che si vede.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGoldenDivergenceNamesTheFieldTest,
	"RefactorTactics.Simulation.GoldenDivergenceNamesTheField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGoldenDivergenceNamesTheFieldTest::RunTest(const FString&)
{
	const TArray<FRTTurnLogEntry> Golden = GoldenSample();

	// La voce 2 e' `Move/Moved`, `Action.Move`, amount 2, src (1,0,0), tgt (2,0,0): tutto il resto e' a zero.
	auto DiagFor = [&Golden](TFunctionRef<void(FRTTurnLogEntry&)> Mutate)
	{
		TArray<FRTTurnLogEntry> Actual = Golden;
		Mutate(Actual[2]);
		return URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 1, Golden, Actual);
	};

	TestTrue(TEXT("phase"),
		DiagFor([](FRTTurnLogEntry& E) { E.Phase = ERTMatchPhase::Cleanup; })
			.Contains(TEXT("phase atteso Move, trovato Cleanup")));

	TestTrue(TEXT("category"),
		DiagFor([](FRTTurnLogEntry& E) { E.Category = ERTLogCategory::Status; })
			.Contains(TEXT("category atteso Move, trovato Status")));

	// ⚠️ L'esito si legge per NOME, non come numero (`#1427`): e' l'unico campo il cui significato dipende
	// dalla categoria, e «atteso 2, trovato 5» mandava chi legge a cercare in `RTTurnLog.h` quale enum
	// fosse. Il valore MESCOLATO resta il numero — cambia la resa, non l'identita'.
	TestTrue(TEXT("outcome"),
		DiagFor([](FRTTurnLogEntry& E) { E.Outcome = static_cast<uint8>(ERTMoveOutcome::BlockedByUnit); })
			.Contains(TEXT("outcome atteso Moved, trovato BlockedByUnit")));

	TestTrue(TEXT("src"),
		DiagFor([](FRTTurnLogEntry& E) { E.SrcCell = FRTCellId(7, 7, 1); })
			.Contains(TEXT("src atteso (q=1,r=0,L=0), trovato (q=7,r=7,L=1)")));

	TestTrue(TEXT("tgt"),
		DiagFor([](FRTTurnLogEntry& E) { E.TgtCell = FRTCellId(9, 9); })
			.Contains(TEXT("tgt atteso (q=2,r=0,L=0), trovato (q=9,r=9,L=0)")));

	TestTrue(TEXT("amount"),
		DiagFor([](FRTTurnLogEntry& E) { E.Amount = 42; })
			.Contains(TEXT("amount atteso 2, trovato 42")));

	TestTrue(TEXT("actionId"),
		DiagFor([](FRTTurnLogEntry& E) { E.ActionId = FName(TEXT("Action.Sprint")); })
			.Contains(TEXT("actionId atteso 'Action.Move', trovato 'Action.Sprint'")));

	TestTrue(TEXT("graphRevision"),
		DiagFor([](FRTTurnLogEntry& E) { E.GraphRevision = 127; })
			.Contains(TEXT("graphRevision atteso 0, trovato 127")));

	// Un id di finestra VUOTO non entra nell'hash (e' il ciclo che non gira), quindi il campo compare da una
	// parte sola: la diagnosi lo dice invece di stampare due valori di cui uno non esiste.
	{
		const FString Diag = DiagFor([](FRTTurnLogEntry& E) { E.OpportunityId = TEXT("opp-1"); });
		TestTrue(TEXT("opportunityId"),
			Diag.Contains(TEXT("opportunityId atteso '', trovato 'opp-1'")));

		// Aprire la finestra da una parte sola fa comparire `selectedTarget` in un elenco solo: e' il ramo
		// `<assente>`, che senza questa riga non lo asserisce nessuno.
		TestTrue(TEXT("un campo presente da una parte sola si dichiara assente"),
			Diag.Contains(TEXT("selectedTarget atteso <assente>, trovato -1")));
	}

	// `SelectedTargetUnitId` e' mescolato SOLO dentro una finestra: fuori non discrimina, e infatti non
	// compare. Il caso va costruito con la finestra aperta da entrambe le parti.
	{
		TArray<FRTTurnLogEntry> Base = Golden;
		Base[2].OpportunityId = TEXT("opp-1");
		Base[2].SelectedTargetUnitId = 1; // esplicito da entrambe le parti: il default e' INDEX_NONE, non 0

		TArray<FRTTurnLogEntry> Actual = Base;
		Actual[2].SelectedTargetUnitId = 3;

		const FString Diag = URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 1, Base, Actual);
		TestTrue(TEXT("selectedTarget"), Diag.Contains(TEXT("selectedTarget atteso 1, trovato 3")));
	}

	// E il verso opposto: un campo che l'hash NON guarda non produce divergenza, quindi non ha niente da
	// nominare. `UnitId` e' il caso di D-063 — spiega la traccia, non la discrimina.
	{
		TArray<FRTTurnLogEntry> Actual = Golden;
		Actual[2].UnitId = 9;
		TestTrue(TEXT("UnitId non discrimina: nessuna divergenza"),
			URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 1, Golden, Actual).IsEmpty());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGoldenCorpusDivergenceTest,
	"RefactorTactics.Simulation.GoldenCorpusDetectsDivergence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGoldenCorpusDivergenceTest::RunTest(const FString&)
{
	const TArray<FRTTurnLogEntry> Golden = GoldenSample();

	// Nessuna divergenza: nessuna descrizione. Una diagnosi che parla anche quando tutto va bene e' rumore
	// che insegna a ignorarla.
	TestTrue(TEXT("tracce identiche: nessuna divergenza da descrivere"),
		URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 3, Golden, Golden).IsEmpty());

	// Divergenza di ESITO sulla voce di combattimento: stesso posto, stessa azione, esito diverso.
	{
		TArray<FRTTurnLogEntry> Actual = Golden;
		Actual[1].Outcome = static_cast<uint8>(ERTCombatOutcome::Lethal);

		const FString Diag = URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 3, Golden, Actual);

		TestFalse(TEXT("una divergenza va descritta"), Diag.IsEmpty());
		TestTrue(TEXT("la diagnosi dice il TURNO"), Diag.Contains(TEXT("turno 3")));
		TestTrue(TEXT("la diagnosi dice la FASE"), Diag.Contains(TEXT("Blast")));
		TestTrue(TEXT("la diagnosi dice l'ACTIONID"), Diag.Contains(TEXT("Hero.Wraith.PulseShot")));
	}

	// Divergenza SOLO nell'azione, su una voce di movimento. E' il caso che la verifica di mutazione ha
	// scoperto: `DescribeEntry` non stampa l'ActionId per le voci `Move`, quindi la diagnosi mostrava «atteso
	// [X], trovato [X]» — due stringhe identiche accanto alla parola «diverge». Chi legge conclude che il
	// confronto e' rotto, e ha ragione.
	{
		TArray<FRTTurnLogEntry> Actual = Golden;
		Actual[2].ActionId = FName(TEXT("Action.Sprint"));

		const FString Diag = URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 2, Golden, Actual);

		TestTrue(TEXT("nomina l'azione ATTESA"), Diag.Contains(TEXT("Action.Move")));
		TestTrue(TEXT("e quella TROVATA"), Diag.Contains(TEXT("Action.Sprint")));
	}

	// Divergenza su un campo che `DescribeEntry` NON stampa per quella categoria: qui `TgtCell` su una voce
	// di movimento che non e' `Moved` (il ramo «fermo: …» stampa solo la cella di partenza). Senza il
	// fallback sui campi grezzi la diagnosi direbbe «atteso [X], trovato [X]» — la stessa forma del difetto
	// trovato con l'ActionId, in un altro punto.
	{
		TArray<FRTTurnLogEntry> Base = Golden;
		Base[2].Outcome = static_cast<uint8>(ERTMoveOutcome::BlockedByUnit);

		TArray<FRTTurnLogEntry> Actual = Base;
		Actual[2].TgtCell = FRTCellId(9, 9);

		const FString Diag = URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 4, Base, Actual);

		TestFalse(TEXT("una divergenza va descritta"), Diag.IsEmpty());
		TestTrue(TEXT("quando la prosa non distingue, mostra i campi"), Diag.Contains(TEXT("campi:")));
		TestTrue(TEXT("e nomina QUELLO che diverge, coi due valori"),
			Diag.Contains(TEXT("tgt atteso (q=2,r=0,L=0), trovato (q=9,r=9,L=0)")));
	}

	// Divergenza su `GraphRevision`: il campo che NESSUNA prosa rende, per nessuna categoria — e che nessun
	// elenco di campi grezzi nominava, perche' quell'elenco era scritto a mano e ne portava quattro su dieci.
	// E' il caso di `#1423`: due arene costruite con un numero diverso di revisioni producono tracce identiche
	// in ogni parola e diverse nell'identita' del grafo su cui sono state validate. Il report mostrava
	// «atteso [X], trovato [X]» **su ogni campo che stampava**, e chi lo leggeva non aveva nessuna via d'uscita.
	{
		TArray<FRTTurnLogEntry> Actual = Golden;
		Actual[2].GraphRevision = 127;

		const FString Diag = URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 5, Golden, Actual);

		TestFalse(TEXT("una divergenza va descritta"), Diag.IsEmpty());
		TestTrue(TEXT("nomina il campo che diverge"),
			Diag.Contains(TEXT("graphRevision atteso 0, trovato 127")));
	}

	// La PRIMA divergenza, non l'ultima: chi legge parte dalla causa piu' probabile.
	{
		TArray<FRTTurnLogEntry> Actual = Golden;
		Actual[0].Outcome = 9;
		Actual[2].Amount = 99;

		const FString Diag = URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 1, Golden, Actual);
		TestTrue(TEXT("riporta la prima divergenza (Prep), non le successive"), Diag.Contains(TEXT("Prep")));
		TestTrue(TEXT("e nomina la sua azione"), Diag.Contains(TEXT("Action.Guard")));
	}

	// Lunghezze diverse: e' una divergenza, e va detta come tale invece di leggere fuori dall'array.
	{
		TArray<FRTTurnLogEntry> Shorter = Golden;
		Shorter.RemoveAt(2);

		const FString Diag = URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 7, Golden, Shorter);
		TestFalse(TEXT("meno voci del riferimento: divergenza"), Diag.IsEmpty());
		TestTrue(TEXT("e la diagnosi dice il turno"), Diag.Contains(TEXT("turno 7")));

		// E nel verso opposto, che e' il caso in cui una voce NUOVA compare senza essere attesa.
		TArray<FRTTurnLogEntry> Longer = Golden;
		Longer.Add(MakeGoldenEntry(ERTMatchPhase::Cleanup, ERTLogCategory::Environment,
			0, TEXT("Action.Ignite")));
		TestFalse(TEXT("piu' voci del riferimento: divergenza"),
			URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 7, Golden, Longer).IsEmpty());
	}

	return true;
}


// ---------------------------------------------------------------------------------------------------------
// Il corpus vero: partite di riferimento su disco
// ---------------------------------------------------------------------------------------------------------

/**
 * Rigenerazione del corpus: **esplicita, mai automatica** (DoD di CP 12.6).
 *
 * Un corpus che si riscrive da solo quando non torna non e' un corpus: e' un test che si adegua a qualunque
 * regressione. Con questa a 1 i file vengono riscritti e il confronto NON viene fatto; la PR che li rigenera
 * dichiara *perche'* l'esito e' cambiato.
 *
 *   UnrealEditor-Cmd.exe <progetto> -ExecCmds="Automation RunTests RefactorTactics.Simulation.GoldenCorpusMatches; Quit"
 *     -dpcvars="rt.Test.RegenerateGolden=1" -unattended -nopause -nosplash -nullrhi -NoLiveCoding
 *
 * ⚠️ **La CVar NON va in testa a `-ExecCmds`**, come diceva questa riga fino al 2026-08-26: quella forma fa
 * saltare la coda di automation, e il log troncato che ne esce **sembra verde**. Si passa da `-dpcvars`, e
 * si verifica che i `.rttl` siano davvero cambiati prima di crederci — `git status` basta.
 */
static TAutoConsoleVariable<int32> CVarRegenerateGolden(
	TEXT("rt.Test.RegenerateGolden"),
	0,
	TEXT("1 = riscrive il corpus golden invece di confrontarlo. Mai in automatico: la PR dichiara il perche'."),
	ECVF_Default);

namespace
{
	/** Radice del corpus. Sta nel SORGENTE, accanto ai test che lo leggono, non in Content. */
	FString GoldenRoot()
	{
		return FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/RefactorTactics/Tests/Golden"));
	}

	FString GoldenTurnPath(const FString& ScenarioId, int32 TurnNumber)
	{
		return FPaths::Combine(GoldenRoot(), ScenarioId, FString::Printf(TEXT("turn-%02d.rttl"), TurnNumber));
	}

	UWorld* MakeGoldenWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyGoldenWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Il nome della categoria, o il numero se l'enum non lo conosce: serve solo a rendere leggibile l'esito. */
	FString EnumNameOr(ERTLogCategory Category)
	{
		const UEnum* Enum = StaticEnum<ERTLogCategory>();
		const FString Nome = Enum ? Enum->GetNameStringByValue(static_cast<int64>(Category)) : FString();
		return Nome.IsEmpty() ? FString::FromInt(static_cast<int32>(Category)) : Nome;
	}

	/**
	 * Le partite di riferimento. **Poche e deterministiche**: un corpus lento non viene eseguito.
	 *
	 * ⚠️ Ogni voce e' qui perche' porta una CATEGORIA che le altre non portano — misurato, non scelto:
	 * `GoldenCorpusCoversItsCategories` stampa la copertura di ciascuna. Tre candidati sono stati provati e
	 * scartati (`Combat.BasicAttack`, `Spec.Environment.ElectricPropagation`,
	 * `Spec.ActionEconomy.CooldownBlocksWithSlotFree`): producono solo `Combat` e `Move`, che il corpus aveva
	 * gia'. Aggiungere tracce che non allargano la rete costa tempo di esecuzione e non compra niente.
	 *
	 *   Movement.Basic / Movement.Collision      Move          (le due storiche)
	 *   Combat.CounterStrikesBack                Combat, Facing, Reaction, Status
	 *                                            (`Combat` e `Facing` da `#1593` li porta anche il fallback)
	 *   Spec.Environment.WaterQuenchesFire       Environment
	 *   Spec.Predictive.WhiffOnEmptyCell         Predictive
	 *   Spec.Overwatch.HoldThenFire              ReactionDecision
	 *   Visual.Combat.FallbackTargetMoved        Fallback       (`#1593`)
	 *
	 * ⚠️ **`Combat.CounterStrikesBack` e' l'unico fornitore di `Reaction` e `Status`**: se smettesse di
	 * produrne una — la reazione che perde la voce, lo stato che non viene applicato — il test cade e il
	 * difetto sta li'. Misurato, non temuto.
	 *
	 * 🔴 **Questa riga diceva «vale da solo META' della soglia» e nominava QUATTRO categorie, ed e' stata
	 * falsificata dal diff di `#1593` che l'ha lasciata intatta.** `Visual.Combat.FallbackTargetMoved` porta
	 * anche `Combat` e `Facing`, quindi quelle due hanno ora due fornitori: perderle in `CounterStrikesBack`
	 * **non produce piu' nessun rosso**. L'avvertenza operativa — «non cadrebbe per una categoria ma per
	 * quattro insieme» — era diventata una regola di diagnosi che non vale piu', cioe' esattamente il modo in
	 * cui un commento fa cercare il difetto nel posto sbagliato. Trovato in code review.
	 *
	 * ⚠️ **UNA categoria resta scoperta, ed e' dichiarato**: `ReactionClash`. Non e' un lavoro rimandato, e' un
	 * lavoro **BLOCCATO** — sta in `KnownUnavailableCapabilities()` con owner `#314`, e uno scenario che la
	 * chiedesse non verrebbe eseguito affatto.
	 *
	 * 🔴 **E la prova sta nei quattro scenari che gia' la chiedono.** `Scenarios/Spec/Clash/*.json` dichiarano
	 * `requires: ["DecisionBoundary", "ReactionClash"]` ed escono **`Blocked`** — la loro stessa nota lo
	 * scrive: *«Esce BLOCKED finche' `ReactionClash` non esiste, e va bene»*. Questa riga sosteneva che
	 * *«risolvono senza emettere quelle voci»*, che e' falso e contraddiceva la frase sopra di lei nello
	 * stesso paragrafo: uno scenario BLOCKED non risolve, non gira affatto. Trovato in code review.
	 *
	 * ➕ **QUANDO `#314` ATTERRA**: uno dei quattro `Spec/Clash` entra in `GoldenScenarioIds` e
	 * `CategorieMinime` sale da **9** a **10**. L'istruzione stava qui prima di `#1593` — *«chi ne scrive uno
	 * che le tocca alzi la soglia»* — e quel diff l'ha tolta spiegando il blocco senza dire piu' cosa fare
	 * quando si sblocca. E' la stessa forma di difetto che `#1593` e' andato a correggere: una nota la cui
	 * formulazione decide se un lavoro pronto viene fatto.
	 *
	 * 🔴 **`Fallback` stava nella stessa riga e non era lo stesso caso** (`#1593`). Non e' una capability: e'
	 * una categoria che il runtime emette da SETTE siti (`RTTurnManager.cpp` x2, `RTTurnManager_Blast.cpp`
	 * x5), e uno scenario che la produce **esisteva gia'** — `Visual.Combat.FallbackTargetMoved`, il fallback
	 * `AttackCell` di un bersaglio uscito di portata. La nota diceva *«chi ne scrive uno»* e chiedeva un
	 * lavoro gia' fatto: la categoria e' rimasta scoperta per settimane non perche' fosse difficile, ma
	 * perche' questa frase metteva insieme un caso bloccato e uno pronto. **Due cose diverse nella stessa
	 * riga aspettano alla velocita' della piu' lenta.**
	 */
	/**
	 * Carica ed esegue uno scenario del corpus, con la guardia. Vero se il risultato e' utilizzabile.
	 *
	 * 🔴 **Le due copie di questo blocco erano divergite, ed e' la ragione per cui ora e' una sola.**
	 * `GoldenCorpusMatches` aveva la guardia su `Error`/`Blocked`, `GoldenCorpusCoversItsCategories` no: uno
	 * scenario che smettesse di girare — un `requires` aggiunto al JSON, una capability tornata fra le non
	 * disponibili — gli avrebbe dato zero tracce e zero categorie, e il rosso avrebbe detto solo «ne copre
	 * otto» senza nominare lo scenario che non e' partito. Cioe' proprio *«un file che nessuno esegue e che
	 * sembra copertura»*, lasciato senza presidio dentro il test che la copertura la misura. Trovato in code
	 * review, e la duplicazione era il veicolo della divergenza.
	 *
	 * ⚠️ Ogni voce del corpus costa **due** esecuzioni qui piu' quella di `Scenario.EveryShippedScenarioRuns`:
	 * e' il prezzo dichiarato di «poche e deterministiche», e va speso solo per una categoria in piu'.
	 */
	bool RTRunGoldenScenario(const TCHAR* ScenarioId, FAutomationTestBase& Test, FRTTestResult& OutResult)
	{
		FString Error;
		const FString Path = URTScenarioIndex::ResolvePath(ScenarioId, Error);
		FRTTestScenario Scenario;
		if (Path.IsEmpty() || !URTScenarioLoader::LoadFromFile(Path, Scenario, Error))
		{
			Test.AddError(FString::Printf(TEXT("scenario '%s' non caricabile: %s"), ScenarioId, *Error));
			return false;
		}

		UWorld* World = MakeGoldenWorld();
		if (!World)
		{
			Test.AddError(TEXT("world di prova non creato"));
			return false;
		}
		OutResult = URTScenarioRunner::Run(World, Scenario);
		DestroyGoldenWorld(World);

		// Uno scenario che non gira produrrebbe zero tracce, e una misura su zero tracce e' verde per il
		// motivo sbagliato.
		if (OutResult.Outcome == ERTTestOutcome::Error || OutResult.Outcome == ERTTestOutcome::Blocked)
		{
			Test.AddError(FString::Printf(TEXT("'%s' non eseguibile (%s): %s%s"),
				ScenarioId, *OutResult.OutcomeString(), *OutResult.ErrorMessage, *OutResult.BlockedReason));
			return false;
		}
		return true;
	}

	/**
	 * ⚠️ **`Visual.Combat.FallbackTargetMoved` e' di classe B, e diventando golden acquisisce un vincolo che
	 * il suo runbook non gli metteva.** `scenario-map.md` §2 classifica `Scenarios/Visual/` come *«la macchina
	 * esegue, l'umano giudica»*, e `runbooks/scenari-validazione-visiva.md` esiste per **rivederli** finche'
	 * l'effetto non si legge — quello scenario e' gia' stato rivisto una volta per quella ragione, e la sua
	 * voce PIE non e' ancora stata guardata. Da ora una revisione della geometria fa divergere la traccia, e
	 * per l'intestazione di questo file un rebalance legittimo e un difetto del resolver *«si presentano
	 * identici»*.
	 *
	 * Il presidio sta nel documento dove quella revisione si progetta: la riga del runbook porta lo stato
	 * **«gia' nel corpus»**, come le altre due voci golden. Chi la rivede rigenera la traccia nello stesso
	 * commit — la convenzione e' gia' scritta in testa a questo file. Trovato in code review.
	 */
	const TCHAR* GoldenScenarioIds[] = {
		TEXT("Movement.Collision"), TEXT("Movement.Basic"),
		TEXT("Combat.CounterStrikesBack"),
		TEXT("Spec.Environment.WaterQuenchesFire"),
		TEXT("Spec.Predictive.WhiffOnEmptyCell"),
		TEXT("Spec.Overwatch.HoldThenFire"),
		TEXT("Visual.Combat.FallbackTargetMoved"),
	};
}

/**
 * **Il corpus dichiara quante categorie copre, e cade se ne perde una** (`#1455`).
 *
 * Il corpus esiste per essere la rete che cattura un cambio d'identita' non dichiarato. Su **dieci**
 * categorie ne copriva **una** — i due `.rttl` di partenza portavano solo voci `Move` — e quattro cambi
 * d'identita' di fila gli sono passati sotto senza che suonasse: [D-196], [D-197], [D-198] e [D-199].
 *
 * ⚠️ **Il buco non era visibile**, ed e' la parte che questo test cambia: `GoldenCorpusMatches` e' verde sia
 * quando il corpus copre tutto sia quando non copre niente, perche' confronta cio' che c'e' con cio' che
 * c'era. Un corpus che si restringe non produce nessun segnale — si restringe e basta.
 *
 * ⚠️ **La soglia e' un NUMERO scritto qui**, non un `> 1` generico: e' il patto. Chi toglie uno scenario dal
 * corpus, o cambia uno scenario in modo che smetta di produrre una famiglia di voci, trova questo test rosso
 * e deve decidere — abbassare la soglia dichiarando cosa si perde, o rimettere la copertura. Un `>=` largo
 * lascerebbe scivolare via la copertura una categoria alla volta, che e' come e' arrivata a una.
 *
 * ⚠️ Si legge dalle tracce SERIALIZZATE e non dal TurnLog vivo, cioe' nella forma in cui il corpus le
 * archivia — ma sono le tracce che il runner ha appena PRODOTTO, non i file `.rttl` su disco: cancellando
 * `Golden/` questo test resta verde. La distinzione che regge e' serializzato-contro-struttura-viva; dire
 * «ed e' quindi cio' di cui si sta misurando la copertura» faceva credere che a essere misurato fosse
 * l'archivio, e su quella storia poggia una soglia. A confrontare i file e' `GoldenCorpusMatches`. Trovato
 * in code review.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGoldenCorpusCoverageTest,
	"RefactorTactics.Simulation.GoldenCorpusCoversItsCategories",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGoldenCorpusCoverageTest::RunTest(const FString&)
{
	TSet<ERTLogCategory> Coperte;
	int32 VociTotali = 0;

	for (const TCHAR* ScenarioId : GoldenScenarioIds)
	{
		FRTTestResult Result;
		if (!RTRunGoldenScenario(ScenarioId, *this, Result))
		{
			continue;
		}

		TSet<ERTLogCategory> DiQuesto;
		for (const FRTTurnTrace& Trace : Result.TurnTraces)
		{
			TArray<FRTTurnLogEntry> Entries;
			if (!URTTurnLogLibrary::DeserializeTurnLog(Trace.Bytes, Entries))
			{
				AddError(FString::Printf(TEXT("traccia di '%s' non rileggibile"), ScenarioId));
				continue;
			}
			VociTotali += Entries.Num();
			for (const FRTTurnLogEntry& E : Entries)
			{
				DiQuesto.Add(E.Category);
				Coperte.Add(E.Category);
			}
		}

		TArray<FString> Nomi;
		for (ERTLogCategory C : DiQuesto)
		{
			Nomi.Add(EnumNameOr(C));
		}
		Nomi.Sort();
		AddInfo(FString::Printf(TEXT("%s: %d turni, categorie {%s}"),
			ScenarioId, Result.TurnTraces.Num(), *FString::Join(Nomi, TEXT(", "))));
	}

	// La premessa: senza voci, «copre N categorie» sarebbe falso per il motivo sbagliato.
	if (!TestTrue(FString::Printf(TEXT("premessa: il corpus produce voci (%d)"), VociTotali), VociTotali > 0))
	{
		return false;
	}

	TArray<FString> Tutte;
	for (ERTLogCategory C : Coperte)
	{
		Tutte.Add(EnumNameOr(C));
	}
	Tutte.Sort();

	const UEnum* CategorieEnum = StaticEnum<ERTLogCategory>();
	const int32 Dichiarate = CategorieEnum ? CategorieEnum->NumEnums() - 1 : 0;
	AddInfo(FString::Printf(TEXT("il corpus copre %d categorie su %d: %s"),
		Coperte.Num(), Dichiarate, *FString::Join(Tutte, TEXT(", "))));

	// 🔴 **Il patto**: questa soglia e' cio' che il corpus promette di coprire. Alzarla e' un miglioramento;
	// abbassarla e' una decisione da dichiarare, non un effetto collaterale.
	// 🔴 **Nove**, misurato il 2026-08-28 (`#1593`): resta scoperta la sola `ReactionClash`, e la ragione —
	// BLOCCATA, owner `#314` — sta accanto a `GoldenScenarioIds`. Era **otto** finche' `Fallback` era
	// dichiarata scoperta insieme a lei, pur avendo gia' il proprio scenario.
	//
	// ⚠️ **Alzare questo numero e' cio' che rende la copertura nuova falsificabile.** Aggiungere lo scenario
	// all'elenco senza toccare la soglia avrebbe lasciato il test verde **prima e dopo**: la categoria in
	// piu' non sarebbe stata misurata da nessuno, e toglierla di nuovo non avrebbe fatto rumore. Verificato
	// per mutazione: togliendo `Visual.Combat.FallbackTargetMoved` dall'elenco, con la soglia a nove questo
	// test va rosso; con la soglia a otto sarebbe restato verde.
	constexpr int32 CategorieMinime = 9;
	TestTrue(*FString::Printf(TEXT("il corpus copre almeno %d categorie (ne copre %d)"),
		CategorieMinime, Coperte.Num()), Coperte.Num() >= CategorieMinime);

	// 🔴 **E l'IDENTITA' di cio' che resta scoperto, non solo il conteggio.** Un cambio che perdesse una
	// categoria guadagnandone un'altra terrebbe il numero a nove e questo test verde, mentre la nota accanto
	// a `GoldenScenarioIds` — «resta scoperta la sola `ReactionClash`» — diventerebbe falsa **senza segnale**.
	// E' lo stesso argomento della soglia applicato al NOME invece che alla cardinalita': una promessa che
	// nessuno misura non e' falsificabile. Una categoria NUOVA dell'enum, senza copertura, fa cadere anche
	// questa riga — ed e' voluto: va dichiarata, non ereditata. Trovato in code review.
	TArray<FString> Scoperte;
	if (CategorieEnum)
	{
		for (int32 I = 0; I < CategorieEnum->NumEnums() - 1; ++I)
		{
			const ERTLogCategory Categoria = static_cast<ERTLogCategory>(CategorieEnum->GetValueByIndex(I));
			if (!Coperte.Contains(Categoria))
			{
				Scoperte.Add(EnumNameOr(Categoria));
			}
		}
	}
	Scoperte.Sort();
	TestEqual(TEXT("e a restare scoperta e' esattamente `ReactionClash` (owner #314)"),
		FString::Join(Scoperte, TEXT(", ")), FString(TEXT("ReactionClash")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGoldenCorpusMatchesTest,
	"RefactorTactics.Simulation.GoldenCorpusMatches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGoldenCorpusMatchesTest::RunTest(const FString&)
{
	const bool bRegenerate = CVarRegenerateGolden.GetValueOnAnyThread() != 0;

	for (const TCHAR* ScenarioId : GoldenScenarioIds)
	{
		FRTTestResult Result;
		if (!RTRunGoldenScenario(ScenarioId, *this, Result))
		{
			continue;
		}
		if (!TestTrue(FString::Printf(TEXT("'%s' ha prodotto almeno un turno"), ScenarioId),
			Result.TurnTraces.Num() > 0))
		{
			continue;
		}

		for (int32 i = 0; i < Result.TurnTraces.Num(); ++i)
		{
			const int32 TurnNumber = i + 1;
			const FString TurnPath = GoldenTurnPath(ScenarioId, TurnNumber);
			const TArray<uint8>& Fresh = Result.TurnTraces[i].Bytes;

			if (bRegenerate)
			{
				if (!FFileHelper::SaveArrayToFile(Fresh, *TurnPath))
				{
					AddError(FString::Printf(TEXT("corpus non scrivibile: %s"), *TurnPath));
				}
				continue;
			}

			TArray<uint8> Golden;
			if (!FFileHelper::LoadFileToArray(Golden, *TurnPath))
			{
				AddError(FString::Printf(
					TEXT("manca la traccia di riferimento %s — rigenerala con `rt.Test.RegenerateGolden 1`, ")
					TEXT("e dichiara nella PR perche' l'esito e' cambiato"), *TurnPath));
				continue;
			}

			const ERTTraceComparison Verdict = URTTurnLogLibrary::CompareSerializedTraces(Golden, Fresh);
			if (Verdict == ERTTraceComparison::Identical)
			{
				continue;
			}

			// Il verdetto dice CHE COSA e' successo; la diagnosi dice DOVE. Un corpus che si limita al primo
			// finisce rigenerato invece che letto.
			FString Detail;
			TArray<FRTTurnLogEntry> GoldenEntries;
			TArray<FRTTurnLogEntry> FreshEntries;
			if (URTTurnLogLibrary::DeserializeTurnLog(Golden, GoldenEntries)
				&& URTTurnLogLibrary::DeserializeTurnLog(Fresh, FreshEntries))
			{
				Detail = URTTurnLogLibrary::DescribeFirstDivergence(TurnNumber, GoldenEntries, FreshEntries);
			}

			AddError(FString::Printf(TEXT("'%s' diverge dal corpus (%s). %s"),
				ScenarioId,
				Verdict == ERTTraceComparison::Divergence ? TEXT("divergenza")
					: Verdict == ERTTraceComparison::FormatMismatch ? TEXT("formato diverso")
					: Verdict == ERTTraceComparison::TopologyMismatch ? TEXT("topologia diversa")
					: TEXT("traccia illeggibile"),
				Detail.IsEmpty() ? TEXT("(nessuna differenza voce per voce: e' il contesto a divergere)") : *Detail));
		}
	}

	if (bRegenerate)
	{
		AddWarning(TEXT("corpus RIGENERATO: nessun confronto eseguito. Dichiara nella PR perche' l'esito e' cambiato."));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
