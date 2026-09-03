#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

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
	const FString Cartella = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/RefactorTactics/Tests"));
	TArray<FString> Nomi;
	IFileManager::Get().FindFiles(Nomi, *FPaths::Combine(Cartella, TEXT("*.cpp")), /*Files*/ true, /*Dirs*/ false);

	if (!TestTrue(TEXT("i sorgenti dei test sono leggibili"), Nomi.Num() > 0))
	{
		return false;
	}

	const FString Guardia = TEXT("#endif // WITH_DEV_AUTOMATION_TESTS");
	int32 ConGuardia = 0;
	int32 Difettosi = 0;

	for (const FString& Nome : Nomi)
	{
		FString Testo;
		if (!FFileHelper::LoadFileToString(Testo, *FPaths::Combine(Cartella, Nome)))
		{
			AddError(FString::Printf(TEXT("%s non si legge"), *Nome));
			continue;
		}

		const int32 Pos = Testo.Find(Guardia, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		if (Pos == INDEX_NONE)
		{
			// Un file senza la guardia non e' un difetto di questo oracolo: non tutti i file di `Tests/` sono
			// suite di automation — ci sono fixture e helper condivisi che vivono fuori dalla macro.
			continue;
		}

		++ConGuardia;
		const FString Coda = Testo.Mid(Pos + Guardia.Len()).TrimStartAndEnd();
		if (!Coda.IsEmpty())
		{
			++Difettosi;
			AddError(FString::Printf(
				TEXT("%s: dopo l'#endif della guardia restano %d caratteri di codice. In Shipping quel codice ")
				TEXT("resta senza gli helper che la guardia racchiude, e la build non compila. L'#endif va ")
				TEXT("in fondo al file."), *Nome, Coda.Len()));
		}
	}

	AddInfo(FString::Printf(TEXT("file di test ispezionati: %d, di cui con la guardia: %d"),
		Nomi.Num(), ConGuardia));

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
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRuntimeTestsAvoidEditorOnlyApiTest,
	"RefactorTactics.Meta.RuntimeTestsAvoidEditorOnlyApi",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRuntimeTestsAvoidEditorOnlyApiTest::RunTest(const FString&)
{
	const FString Cartella = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/RefactorTactics/Tests"));
	TArray<FString> Nomi;
	IFileManager::Get().FindFiles(Nomi, *FPaths::Combine(Cartella, TEXT("*.cpp")), /*Files*/ true, /*Dirs*/ false);

	// ⚠️ Se non trova i sorgenti FALLISCE: un oracolo che perde il proprio soggetto e resta verde e' peggio
	// di un oracolo assente. Stessa disciplina del test qui sopra.
	if (!TestTrue(TEXT("i sorgenti dei test sono leggibili"), Nomi.Num() > 0))
	{
		return false;
	}

	// La lista e' corta di proposito: ci sta cio' che ha gia' morso, non tutto cio' che potrebbe.
	// `Actor.h:3417` per il primo — se un giorno se ne aggiunge un altro, si aggiunge qui con la sua storia.
	const TCHAR* SoloEditor[] = { TEXT("RerunConstructionScripts") };

	int32 Difettosi = 0;
	for (const FString& Nome : Nomi)
	{
		FString Testo;
		if (!FFileHelper::LoadFileToString(Testo, *FPaths::Combine(Cartella, Nome)))
		{
			AddError(FString::Printf(TEXT("%s non si legge"), *Nome));
			continue;
		}
		for (const TCHAR* Api : SoloEditor)
		{
			const FString Chiamata = FString::Printf(TEXT("->%s("), Api);
			if (Testo.Contains(Chiamata, ESearchCase::CaseSensitive))
			{
				++Difettosi;
				AddError(FString::Printf(
					TEXT("%s chiama %s, che esiste solo WITH_EDITOR: il target gioco non compilera' e ")
					TEXT("nessun pacchetto sara' costruibile. Usa OnConstruction(GetTransform())."),
					*Nome, Api));
			}
		}
	}

	TestEqual(TEXT("nessun test runtime chiama API di solo editor"), Difettosi, 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
