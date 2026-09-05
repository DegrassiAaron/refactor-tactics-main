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


/**
 * **Nessuna funzione libera in namespace anonimo ha nome E firma uguali a una di un altro file.**
 *
 * 🔴 **Questo oracolo esiste perche' il difetto ha fermato `main` il 2026-09-05** (#2397): `StandStill(ARTUnit*)`
 * era definita in namespace anonimo sia in `RTStatusTests.cpp:97` sia in `RTUnbalancedProneTests.cpp:103`.
 * In C++ e' legale — due unita' di traduzione separate, internal linkage — ma la **unity build** le concatena
 * nello stesso blob, e li' due definizioni identiche sono `C2084`, piu' una cascata di `C2264` su ogni
 * chiamata (tredici, quel giorno).
 *
 * ⚠️ **Il difetto e' LATENTE, e nessun gate poteva prenderlo al momento in cui veniva introdotto.** Non
 * dipende dal codice, dipende dal **raggruppamento**: i due file convivevano dal 2026-09-04 senza collidere,
 * finche' un merge che non toccava nessuno dei due ha spostato i confini dei blob. La suite era `VALIDA` un'ora
 * prima sullo stesso albero — quella misura era corretta, il difetto non era ancora raggiungibile.
 *
 * ⚠️ **E il modo di fallire e' il peggiore**: chi aggiunge un file qualsiasi vede un errore in un file di test
 * che non ha mai aperto, e lo legge come *«ho rotto qualcosa io»*.
 *
 * ∴ #2397 ha riparato **un esemplare** — con una verifica di chiusura, `git grep "void StandStill"`, che e'
 * specifica di un simbolo e non dell'invariante. Al momento in cui questo oracolo e' stato scritto ne
 * restavano **quattro** armate, tutte fra `RTFrontendWidgetAssetTests.cpp` e `RTMatchWidgetAssetTests.cpp`
 * (#2423): il giorno in cui l'unity li avesse messi insieme non sarebbe esplosa una collisione, ma quattro.
 *
 * ---
 *
 * 🔑 **Le TRE condizioni, che sono la specifica di questo oracolo.** Una `C2084` le richiede tutte insieme, e
 * un gate che ne verificasse meno sarebbe rosso su codice giusto — cioe' spento entro un giorno:
 *
 * 1. **stesso modulo** — l'unity blob non attraversa i moduli. Qui e' garantito dal soggetto: si legge solo
 *    `Source/RefactorTactics/Tests`, che sta tutto nel modulo runtime.
 * 2. **stessa firma** — nome **e** tipi dei parametri. Due omonime con parametri diversi sono **overload
 *    legali**, anche nello stesso blob: confrontare i soli nomi darebbe decine di falsi positivi (misurato:
 *    25 nomi contro 5 firme).
 * 3. **funzione libera** — i membri di `struct`/`class` dichiarati dentro il namespace anonimo non collidono
 *    mai fra loro. Ignorarlo produceva due falsi positivi su sei (`Clear()` e `Set(const TCHAR*)`, membri di
 *    `FRTScopedEntryCommandLine` e `FRTScopedEntryCVar`).
 *
 * ---
 *
 * ⚠️ **I QUATTRO LIMITI, dichiarati invece che scoperti dopo.** Questo test e' facile da leggere come
 * *«nessuna collisione di unity build»*, e non e' cio' che misura:
 *
 * 1. **Guarda `Source/RefactorTactics/Tests`, non tutto il modulo.** L'unity blob contiene **anche** `Turn/`,
 *    `Map/`, `Unit/`: un helper di test e uno di produzione con la stessa firma collidono allo stesso modo, e
 *    questo oracolo non li vedrebbe. Al momento in cui e' stato scritto non ce n'erano — misurato — ma e'
 *    un'assenza, non una garanzia. Allargare il perimetro significa cambiare `RTLeggiSorgentiDeiTest`, che ha
 *    altri due consumatori: e' una decisione, non un dettaglio.
 * 2. **Il modulo `RefactorTacticsEditor` non e' coperto.** Ha i propri test in `Private/Tests/` e nessun
 *    oracolo di questa specie. Non puo' collidere con questo modulo — vedi condizione 1 — ma puo' collidere
 *    con se stesso.
 * 3. **Solo funzioni.** Variabili, costanti e tipi omonimi in namespace anonimi seguono la stessa regola e
 *    non sono coperti.
 * 4. **Solo firme su una riga.** Una definizione spezzata su piu' righe non viene vista. Sono poche, e
 *    dichiararlo costa meno che fingere un parser completo.
 *
 * ⛔ **La guardia anti-vacuita' e' obbligatoria qui piu' che altrove**, perche' il conteggio atteso e' **zero**
 * — e uno zero non distingue *«non ci sono collisioni»* da *«l'estrattore non riconosce piu' un namespace
 * anonimo»*. Due campioni sintetici lo falsificano prima che possa mentire: uno che DEVE essere estratto, e
 * uno — un membro di `struct` — che NON deve esserlo. Il primo dei due prende il tranello che ha reso cieca la
 * prima stesura di questo estrattore: in questo repository `namespace` e la graffa stanno su **righe diverse**.
 */

namespace
{
	/** Una funzione libera trovata in un namespace anonimo: `Nome(tipo,tipo)` e dove sta. */
	struct FRTFirmaAnonima
	{
		/** La chiave del confronto: senza spazi, cosi' `const UWidget *` e `const UWidget*` coincidono. */
		FString Firma;
		/** La stessa firma come l'ha scritta l'autore: e' cio' che va nel messaggio d'errore. */
		FString Leggibile;
		FString File;
		int32 Riga = 0;
	};

	/** Le parole che aprono una parentesi senza essere una definizione di funzione. */
	bool RTEParolaChiave(const FString& Nome)
	{
		static const TCHAR* const Parole[] = {
			TEXT("if"), TEXT("for"), TEXT("while"), TEXT("switch"), TEXT("return"), TEXT("sizeof"),
			TEXT("catch"), TEXT("do"), TEXT("else"), TEXT("TEXT"), TEXT("check"), TEXT("ensure"),
			TEXT("NSLOCTEXT"), TEXT("LOCTEXT")
		};
		for (const TCHAR* Parola : Parole)
		{
			if (Nome == Parola)
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * I soli TIPI dei parametri, senza i loro nomi: e' cio' che decide l'overload, ed e' quindi cio' che
	 * decide la collisione. `const`, `*` e `&` restano — due firme che differiscono li' sono overload diversi.
	 */
	FString RTTipiDeiParametri(const FString& Parametri)
	{
		TArray<FString> Pezzi;
		Parametri.ParseIntoArray(Pezzi, TEXT(","), /*InCullEmpty*/ true);

		TArray<FString> Tipi;
		for (FString Pezzo : Pezzi)
		{
			Pezzo.TrimStartAndEndInline();
			// Il nome del parametro e' l'ultimo identificatore, e si toglie solo se resta un tipo davanti.
			int32 Spazio = INDEX_NONE;
			if (Pezzo.FindLastChar(TEXT(' '), Spazio) && Spazio > 0)
			{
				const FString Coda = Pezzo.Mid(Spazio + 1);
				bool bSoloIdentificatore = !Coda.IsEmpty();
				for (const TCHAR Carattere : Coda)
				{
					if (!FChar::IsAlnum(Carattere) && Carattere != TEXT('_'))
					{
						bSoloIdentificatore = false;
						break;
					}
				}
				if (bSoloIdentificatore)
				{
					Pezzo = Pezzo.Left(Spazio);
				}
			}
			Pezzo.TrimStartAndEndInline();
			Pezzo.ReplaceInline(TEXT(" "), TEXT(""));
			Tipi.Add(Pezzo);
		}
		return FString::Join(Tipi, TEXT(","));
	}

	/**
	 * Estrae le firme delle funzioni libere definite nei namespace anonimi di un sorgente.
	 *
	 * ⚠️ Riconosce **entrambi** gli stili di graffa. Quello di questo repository e' `namespace` e `{` su righe
	 * diverse: una prima stesura che cercava il solo `namespace {` restituiva **zero** anche sull'albero in cui
	 * `StandStill` era duplicata — verde, e cieca.
	 */
	void RTEstraiFirmeAnonime(const FString& NomeFile, const FString& Testo, TArray<FRTFirmaAnonima>& Fuori)
	{
		TArray<FString> Righe;
		Testo.ParseIntoArrayLines(Righe, /*InCullEmpty*/ false);

		bool bDentroAnonimo = false;
		int32 Livello = 0;        // graffe aperte dentro il namespace anonimo
		int32 LivelloRecord = 0;  // >0 = dentro una struct/class: i membri non collidono

		for (int32 Indice = 0; Indice < Righe.Num(); ++Indice)
		{
			FString Riga = Righe[Indice];
			Riga.TrimStartAndEndInline();

			if (!bDentroAnonimo)
			{
				const bool bInLinea = (Riga == TEXT("namespace {"));
				const bool bAllman = (Riga == TEXT("namespace"))
					&& (Indice + 1 < Righe.Num())
					&& (Righe[Indice + 1].TrimStartAndEnd() == TEXT("{"));
				if (bInLinea || bAllman)
				{
					bDentroAnonimo = true;
					Livello = 1;
					LivelloRecord = 0;
					if (bAllman)
					{
						++Indice;
					}
				}
				continue;
			}

			int32 Delta = 0;
			for (const TCHAR Carattere : Riga)
			{
				Delta += (Carattere == TEXT('{')) ? 1 : ((Carattere == TEXT('}')) ? -1 : 0);
			}

			if (Riga.StartsWith(TEXT("struct "), ESearchCase::CaseSensitive)
				|| Riga.StartsWith(TEXT("class "), ESearchCase::CaseSensitive)
				|| Riga.StartsWith(TEXT("union "), ESearchCase::CaseSensitive)
				|| Riga.StartsWith(TEXT("enum "), ESearchCase::CaseSensitive))
			{
				++LivelloRecord;
			}

			Livello += Delta;
			if (Livello <= 0)
			{
				bDentroAnonimo = false;
				continue;
			}
			if (LivelloRecord > 0)
			{
				if (Delta < 0 && Livello <= 1)
				{
					LivelloRecord = FMath::Max(0, LivelloRecord - 1);
				}
				continue;
			}

			// Commenti e direttive non definiscono funzioni.
			if (Riga.StartsWith(TEXT("//")) || Riga.StartsWith(TEXT("*")) || Riga.StartsWith(TEXT("/*"))
				|| Riga.StartsWith(TEXT("#")))
			{
				continue;
			}

			int32 Apre = INDEX_NONE;
			if (!Riga.FindChar(TEXT('('), Apre) || Apre == 0)
			{
				continue;
			}
			const int32 Chiude = Riga.Find(TEXT(")"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			if (Chiude < Apre)
			{
				continue;
			}

			// Dopo la parentesi chiusa: solo la graffa o niente. Una chiamata finisce con `;` ed esce di qui.
			const FString Coda = Riga.Mid(Chiude + 1).TrimStartAndEnd();
			if (!Coda.IsEmpty() && Coda != TEXT("{"))
			{
				continue;
			}

			// Il nome e' l'identificatore che precede la parentesi; davanti deve esserci un tipo di ritorno.
			int32 Inizio = Apre;
			while (Inizio > 0)
			{
				const TCHAR Precedente = Riga[Inizio - 1];
				if (FChar::IsAlnum(Precedente) || Precedente == TEXT('_'))
				{
					--Inizio;
					continue;
				}
				break;
			}
			if (Inizio == Apre || Inizio == 0)
			{
				continue;
			}
			const TCHAR PrimaDelNome = Riga[Inizio - 1];
			if (PrimaDelNome != TEXT(' ') && PrimaDelNome != TEXT('*') && PrimaDelNome != TEXT('&'))
			{
				continue;
			}

			const FString Nome = Riga.Mid(Inizio, Apre - Inizio);
			if (Nome.IsEmpty() || RTEParolaChiave(Nome))
			{
				continue;
			}

			const FString Parametri = Riga.Mid(Apre + 1, Chiude - Apre - 1);
			FRTFirmaAnonima Trovata;
			Trovata.Firma = FString::Printf(TEXT("%s(%s)"), *Nome, *RTTipiDeiParametri(Parametri));
			Trovata.Leggibile = FString::Printf(TEXT("%s(%s)"), *Nome, *Parametri.TrimStartAndEnd());
			Trovata.File = NomeFile;
			Trovata.Riga = Indice + 1;
			Fuori.Add(MoveTemp(Trovata));
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnonymousHelpersDoNotCollideTest,
	"RefactorTactics.Meta.AnonymousHelpersDoNotCollideUnderUnity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnonymousHelpersDoNotCollideTest::RunTest(const FString&)
{
	// ⛔ Il controllo POSITIVO viene prima di tutto: se l'estrattore e' cieco, il conteggio zero piu' sotto
	// sarebbe verde e falso. Il campione porta lo stile di graffa di questo repository, che e' il tranello.
	{
		const FString Campione =
			TEXT("namespace\n")
			TEXT("{\n")
			TEXT("\tvoid RTCampioneDaEstrarre(int32 Valore)\n")
			TEXT("\t{\n")
			TEXT("\t}\n")
			TEXT("\n")
			TEXT("\tstruct FRTCampioneRecord\n")
			TEXT("\t{\n")
			TEXT("\t\tvoid RTMembroDaIgnorare(int32 Valore)\n")
			TEXT("\t\t{\n")
			TEXT("\t\t}\n")
			TEXT("\t};\n")
			TEXT("}\n");

		TArray<FRTFirmaAnonima> Estratte;
		RTEstraiFirmeAnonime(TEXT("<campione>"), Campione, Estratte);

		if (!TestEqual(TEXT("il campione produce una firma sola"), Estratte.Num(), 1))
		{
			for (const FRTFirmaAnonima& Firma : Estratte)
			{
				AddError(FString::Printf(TEXT("estratta inattesa: %s"), *Firma.Firma));
			}
			return false;
		}
		if (!TestEqual(TEXT("l'estrattore riconosce la funzione libera"),
			Estratte[0].Firma, FString(TEXT("RTCampioneDaEstrarre(int32)"))))
		{
			return false;
		}
	}

	const TArray<FRTSorgenteDiTest> Sorgenti = RTLeggiSorgentiDeiTest(*this);

	// ⚠️ Se non trova i sorgenti FALLISCE: stessa disciplina dei due oracoli qui sopra.
	if (!TestTrue(TEXT("i sorgenti dei test sono leggibili"), Sorgenti.Num() > 0))
	{
		return false;
	}

	TArray<FRTFirmaAnonima> Tutte;
	for (const FRTSorgenteDiTest& Sorgente : Sorgenti)
	{
		RTEstraiFirmeAnonime(Sorgente.Nome, Sorgente.Testo, Tutte);
	}

	// La stessa firma nello stesso file e' un'altra cosa (overload dichiarato e definito, o una svista che il
	// compilatore prende da solo): qui interessa solo cio' che attraversa due unita' di traduzione.
	TMap<FString, TArray<FRTFirmaAnonima>> PerFirma;
	for (const FRTFirmaAnonima& Firma : Tutte)
	{
		PerFirma.FindOrAdd(Firma.Firma).Add(Firma);
	}

	int32 Collisioni = 0;
	for (const TPair<FString, TArray<FRTFirmaAnonima>>& Voce : PerFirma)
	{
		TSet<FString> FileDistinti;
		for (const FRTFirmaAnonima& Firma : Voce.Value)
		{
			FileDistinti.Add(Firma.File);
		}
		if (FileDistinti.Num() < 2)
		{
			continue;
		}

		++Collisioni;
		FString Dove;
		for (const FRTFirmaAnonima& Firma : Voce.Value)
		{
			Dove += FString::Printf(TEXT("\n    %s:%d"), *Firma.File, Firma.Riga);
		}
		AddError(FString::Printf(
			TEXT("%s e' definita in namespace anonimo in %d file diversi dello stesso modulo:%s\n")
			TEXT("  Il namespace anonimo NON protegge sotto unity build: le due definizioni finiscono nello ")
			TEXT("stesso blob e la compilazione muore con C2084, piu' un C2264 per ogni chiamata. Rinomina ")
			TEXT("quella del file piu' recente, oppure — se i CORPI sono identici — spostala in un header ")
			TEXT("condiviso con namespace NOMINATO. Se i corpi differiscono NON unificarle: vedi #2397."),
			*Voce.Value[0].Leggibile, FileDistinti.Num(), *Dove));
	}

	AddInfo(FString::Printf(TEXT("file ispezionati: %d, firme in namespace anonimo: %d"),
		Sorgenti.Num(), Tutte.Num()));

	// Secondo controllo positivo: se l'estrattore smettesse di trovare firme sul corpus reale, il conteggio
	// delle collisioni resterebbe zero senza che nulla sia stato guardato.
	TestTrue(TEXT("il corpus contiene firme in namespace anonimo"), Tutte.Num() > 0);
	TestEqual(TEXT("nessuna firma anonima e' definita in due file"), Collisioni, 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
