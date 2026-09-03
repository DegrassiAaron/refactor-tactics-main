#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Un sorgente di `Tests/` con il suo contenuto: il soggetto comune dei due oracoli di questo file. */
	struct FRTSorgenteDiTest
	{
		FString Nome;
		FString Testo;
	};

	FString RTCartellaDeiTest()
	{
		return FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/RefactorTactics/Tests"));
	}

	/**
	 * Elenca e legge i sorgenti **una volta sola**.
	 *
	 * ⚠️ I due test qui sotto ispezionano lo stesso insieme di file, e prima di #2136 ne duplicavano
	 * enumerazione e lettura carattere per carattere: ~198 letture sincrone in piu' a ogni `rt-suite`, e
	 * soprattutto **due copie della regola che definisce il soggetto**, da tenere allineate a mano. Chi
	 * allargasse lo scope di un oracolo e non dell'altro non riceverebbe nessun segnale.
	 */
	TArray<FRTSorgenteDiTest> RTLeggiSorgentiDeiTest(FAutomationTestBase& Test)
	{
		const FString Cartella = RTCartellaDeiTest();
		TArray<FString> Nomi;
		IFileManager::Get().FindFiles(Nomi, *FPaths::Combine(Cartella, TEXT("*.cpp")), /*Files*/ true, /*Dirs*/ false);

		TArray<FRTSorgenteDiTest> Sorgenti;
		Sorgenti.Reserve(Nomi.Num());
		for (const FString& Nome : Nomi)
		{
			FString Testo;
			if (!FFileHelper::LoadFileToString(Testo, *FPaths::Combine(Cartella, Nome)))
			{
				Test.AddError(FString::Printf(TEXT("%s non si legge"), *Nome));
				continue;
			}
			Sorgenti.Add({ Nome, MoveTemp(Testo) });
		}
		return Sorgenti;
	}
}

/**
 * **In ogni file di `Tests/`, la guardia `WITH_DEV_AUTOMATION_TESTS` si chiude in FONDO al file.**
 *
 * 🔴 **Questo oracolo esiste perche' il difetto e' successo DUE VOLTE in due giorni, per mano diversa.**
 * `RTFrontendNavigationTests.cpp` (2026-08-23, PR #1292) e `RTScenarioRunnerTests.cpp` (2026-08-24, PR #1312)
 * hanno entrambi ricevuto test nuovi scritti **dopo** l'`#endif`, che stava a meta' di un file da 1300 righe.
 * Non e' distrazione: chi aggiunge un test lo scrive **in fondo**, che e' il posto naturale — e li' la guardia
 * era gia' chiusa.
 *
 * ⚠️ **E nessuno se ne accorge, perche' solo la SHIPPING lo rivela.** In Editor e in Development
 * `WITH_DEV_AUTOMATION_TESTS` vale 1: il codice fuori dalla guardia compila, i test girano, la suite e' verde.
 * In Shipping vale 0 — gli helper del namespace anonimo spariscono, i test restano, e la build muore con
 * `C3861: identificatore non trovato`. Il primo dei due casi e' sopravvissuto a un merge, il secondo a un
 * altro, e li ha trovati il gate `G1` di release, che e' l'unica cosa che builda in Shipping.
 *
 * ∴ l'oracolo non guarda il comportamento del gioco: guarda **la forma dei file di test**, che e' cio' che
 * si e' rotto. Costa una lettura di directory e fallisce **subito**, invece che alla prossima build di
 * release.
 *
 * ⚠️ **Se non trova i sorgenti FALLISCE**, e non e' pignoleria: un oracolo che perde il proprio soggetto e
 * resta verde e' peggio di un oracolo assente. Il test e' `EditorContext`, quindi gira dove i sorgenti ci
 * sono per costruzione — a partire da questo file, che e' uno dei suoi soggetti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTestGuardClosesAtEndOfFileTest,
	"RefactorTactics.Meta.TestGuardClosesAtEndOfFile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTestGuardClosesAtEndOfFileTest::RunTest(const FString&)
{
	const TArray<FRTSorgenteDiTest> Sorgenti = RTLeggiSorgentiDeiTest(*this);
	if (!TestTrue(TEXT("i sorgenti dei test sono leggibili"), Sorgenti.Num() > 0))
	{
		return false;
	}

	const FString Guardia = TEXT("#endif // WITH_DEV_AUTOMATION_TESTS");
	int32 ConGuardia = 0;
	int32 Difettosi = 0;

	for (const FRTSorgenteDiTest& Sorgente : Sorgenti)
	{
		const int32 Pos = Sorgente.Testo.Find(Guardia, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		if (Pos == INDEX_NONE)
		{
			// Un file senza la guardia non e' un difetto di questo oracolo: non tutti i file di `Tests/` sono
			// suite di automation — ci sono fixture e helper condivisi che vivono fuori dalla macro.
			continue;
		}

		++ConGuardia;
		const FString Coda = Sorgente.Testo.Mid(Pos + Guardia.Len()).TrimStartAndEnd();
		if (!Coda.IsEmpty())
		{
			++Difettosi;
			AddError(FString::Printf(
				TEXT("%s: dopo l'#endif della guardia restano %d caratteri di codice. In Shipping quel codice ")
				TEXT("resta senza gli helper che la guardia racchiude, e la build non compila. L'#endif va ")
				TEXT("in fondo al file."), *Sorgente.Nome, Coda.Len()));
		}
	}

	AddInfo(FString::Printf(TEXT("file di test ispezionati: %d, di cui con la guardia: %d"),
		Sorgenti.Num(), ConGuardia));

	// La guardia anti-vacuita': se un giorno nessun file portasse piu' quella riga, questo test starebbe
	// guardando il nulla e continuerebbe a passare.
	TestTrue(TEXT("almeno un file porta la guardia"), ConGuardia > 0);
	TestEqual(TEXT("nessun codice vive fuori dalla guardia"), Difettosi, 0);
	return true;
}

/**
 * **Nessun test del modulo RUNTIME chiama API che esistono solo `WITH_EDITOR`.**
 *
 * 🔴 **Anche questo oracolo esiste perche' il difetto e' successo DUE VOLTE**, e la seconda con l'avviso
 * gia' scritto venti righe piu' su nello stesso file. `RTGrayboxFixtureTests.cpp` ha chiamato
 * `RerunConstructionScripts` — che `Actor.h:3417` dichiara dentro `#if WITH_EDITOR` — prima con `#2072`,
 * e poi di nuovo il 2026-09-02 con `#2094`.
 *
 * ⚠️ **E nessuna suite lo rivela, perche' `rt-suite` gira sul target EDITOR.** Li' `WITH_EDITOR` vale 1:
 * il file compila, i test girano, 1819 su 1819 sono verdi. Il target **gioco** — quello che `BuildCookRun`
 * costruisce — non viene toccato da nessuna misura quotidiana, e li' la build muore con
 * `C2039: 'RerunConstructionScripts': non e' un membro`. La prima volta ha reso **nessun pacchetto
 * costruibile**; la seconda l'ha scoperta il packaging di `#1804`, per caso.
 *
 * ∴ un commento non e' un gate: l'avviso c'era, ed e' stato riletto e riscritto da chi ha rifatto lo stesso
 * errore. Questo lo trova alla prima `rt-suite` invece che al prossimo pacchetto.
 *
 * ⛔ **Cerca la forma di CHIAMATA (`->Nome(`), non il nome**: il nome compare nei commenti che spiegano la
 * trappola — compresi quelli di questo test — e un oracolo che li contasse sarebbe rosso per sempre,
 * quindi disattivato entro un giorno.
 *
 * ---
 *
 * 🔴 **I TRE LIMITI, trovati in code review (#2136) e dichiarati invece che nascosti.** Contano perche'
 * questo test e' facile da leggere come *"nessun test runtime usa API di solo editor"*, e non e' cio' che
 * misura:
 *
 * 1. **La lista ha UN nome, e la sua classe di difetti ne ha almeno DUE.** L'altro e' `GetBoolMetaData`
 *    (`UObject/Class.h:256`, sotto `#if WITH_METADATA`): il 2026-08-29 sei asserzioni in tre file hanno
 *    fatto fallire `RefactorTactics Win64 Development` esattamente cosi' — lo registra il gate `G1` di
 *    `docs/roadmap/v0.1-definition-of-done.md`, che conclude *"nessun oracolo copre ancora questa forma"*.
 *    ⛔ **E non e' aggiungibile qui**, non per pigrizia: quelle sei chiamate esistono ancora, corrette e
 *    racchiuse in `#if WITH_METADATA`. Una ricerca testuale non distingue una chiamata guardata da una
 *    nuda, quindi aggiungere il nome renderebbe il test rosso su codice giusto — e un oracolo rosso su
 *    codice giusto viene spento entro un giorno. Distinguerle vuol dire leggere i `#if`, cioe' un parser
 *    del preprocessore: piu' di quanto questo oracolo sia.
 *    ∴ **cio' che copre davvero quella forma resta `G1`**, cioe' buildare il target gioco. Questo test
 *    riduce la frequenza del difetto noto, non la classe.
 * 2. **Vede solo `->Nome(`.** `Fixture.RerunConstructionScripts()`, `(*Ptr).RerunConstructionScripts()`,
 *    una chiamata non qualificata da dentro un `AActor`, o anche solo uno spazio prima della parentesi,
 *    passano — e rompono il target gioco allo stesso identico modo. E' il prezzo dichiarato del punto
 *    precedente: allargare il pattern lo avvicina al nome nudo, che e' rosso per sempre.
 * 3. **Guarda `Source/RefactorTactics/Tests`, non ricorsivo.** Il difetto pero' non e' dei test: e' del
 *    **modulo runtime**, e la stessa chiamata in `World/`, `Map/` o `Unit/` produce lo stesso pacchetto
 *    impossibile da costruire. L'oracolo sta all'altitudine dei due file che si sono rotti, non a quella
 *    del guasto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRuntimeTestsAvoidEditorOnlyApiTest,
	"RefactorTactics.Meta.RuntimeTestsAvoidEditorOnlyApi",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRuntimeTestsAvoidEditorOnlyApiTest::RunTest(const FString&)
{
	const TArray<FRTSorgenteDiTest> Sorgenti = RTLeggiSorgentiDeiTest(*this);

	// ⚠️ Se non trova i sorgenti FALLISCE: un oracolo che perde il proprio soggetto e resta verde e' peggio
	// di un oracolo assente. Stessa disciplina del test qui sopra.
	if (!TestTrue(TEXT("i sorgenti dei test sono leggibili"), Sorgenti.Num() > 0))
	{
		return false;
	}

	// La lista e' corta di proposito: ci sta cio' che ha gia' morso, non tutto cio' che potrebbe.
	// `Actor.h:3417` per il primo — se un giorno se ne aggiunge un altro, si aggiunge qui con la sua storia.
	// ⛔ Perche' `GetBoolMetaData` NON e' qui, vedi il punto 1 del docstring: non e' una dimenticanza.
	const TCHAR* SoloEditor[] = { TEXT("RerunConstructionScripts") };

	// 🔑 **Il controllo POSITIVO, che il test gemello ha e questo non aveva.** Li' e' `ConGuardia > 0`; qui
	// non puo' esserlo, perche' il conteggio che questo test vuole e' **zero** — e uno zero non distingue
	// "nessuno chiama quell'API" da "il pattern non sa piu' riconoscerla". Un rename lato motore, o una
	// mano sulla `Printf` qui sotto, renderebbero l'oracolo cieco **e verde**. Una riga campione lo
	// falsifica: se smette di corrispondere, il test e' rosso prima di poter mentire.
	for (const TCHAR* Api : SoloEditor)
	{
		const FString Campione = FString::Printf(TEXT("\tAttore->%s();"), Api);
		const FString Chiamata = FString::Printf(TEXT("->%s("), Api);
		if (!TestTrue(FString::Printf(TEXT("il pattern riconosce la forma che cerca (%s)"), Api),
			Campione.Contains(Chiamata, ESearchCase::CaseSensitive)))
		{
			return false;
		}
	}

	int32 Difettosi = 0;
	for (const FRTSorgenteDiTest& Sorgente : Sorgenti)
	{
		for (const TCHAR* Api : SoloEditor)
		{
			const FString Chiamata = FString::Printf(TEXT("->%s("), Api);
			if (Sorgente.Testo.Contains(Chiamata, ESearchCase::CaseSensitive))
			{
				++Difettosi;
				AddError(FString::Printf(
					TEXT("%s chiama %s, che esiste solo WITH_EDITOR: il target gioco non compilera' e ")
					TEXT("nessun pacchetto sara' costruibile. Usa OnConstruction(GetTransform())."),
					*Sorgente.Nome, Api));
			}
		}
	}

	TestEqual(TEXT("nessun test runtime chiama API di solo editor"), Difettosi, 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
