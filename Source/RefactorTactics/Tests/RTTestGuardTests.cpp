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
		// 🔑 **Una lettura per processo, non una per oracolo.** Il docstring qui sopra dice
		// «una volta sola» e con due consumatori era gia' due; col terzo di #2423 sarebbero tre
		// enumerazioni e ~600 letture sincrone su un corpus che non cambia durante la run.
		static TArray<FRTSorgenteDiTest> Cache;
		if (Cache.Num() > 0)
		{
			return Cache;
		}

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
		Cache = Sorgenti;
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


namespace
{
	/** Una funzione libera trovata in un namespace anonimo: `Nome(tipo,tipo)` e dove sta. */
	struct FRTAnonymousSignature
	{
		/** La chiave del confronto: senza spazi, cosi' `const UWidget *` e `const UWidget*` coincidono. */
		FString Key;
		/** La stessa firma come l'ha scritta l'autore: e' cio' che va nel messaggio d'errore. */
		FString Readable;
		FString File;
		int32 Line = 0;
	};

	/** Le parole che aprono una parentesi senza essere una definizione di funzione. */
	bool RTIsKeyword(const FString& Name)
	{
		static const TCHAR* const Words[] = {
			TEXT("if"), TEXT("for"), TEXT("while"), TEXT("switch"), TEXT("return"), TEXT("sizeof"),
			TEXT("catch"), TEXT("do"), TEXT("else"), TEXT("TEXT"), TEXT("check"), TEXT("ensure"),
			TEXT("NSLOCTEXT"), TEXT("LOCTEXT")
		};
		for (const TCHAR* const Word : Words)
		{
			if (Name == Word)
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * I soli TIPI dei parametri, senza i loro nomi: e' cio' che decide l'overload, ed e' quindi cio' che
	 * decide la collisione. `const`, `*` e `&` restano — due firme che differiscono li' sono overload diversi.
	 *
	 * ⚠️ **Il nome del parametro si toglie anche quando `*` o `&` gli e' incollato.** `const TCHAR *Path` e
	 * `const TCHAR* Path` sono la **stessa** firma per il compilatore: senza questo, la prima produceva la
	 * chiave `constTCHAR*Path` e la seconda `constTCHAR*`, e due file scritti nei due stili collidevano in
	 * build restando verdi qui.
	 *
	 * ⛔ **Non gestisce gli argomenti di default**, ed e' un limite dichiarato: `const TCHAR* Id = TEXT("x")`
	 * finisce nella chiave col proprio valore, quindi la stessa funzione dichiarata con e senza default da'
	 * due chiavi. Toglierli richiede di riconoscere una espressione arbitraria dopo `=`, cioe' piu' di quanto
	 * questo oracolo sia. Misurato sul corpus del 2026-09-05: **zero** definizioni in namespace anonimo con
	 * argomenti di default.
	 */
	FString RTParameterTypes(const FString& Parameters)
	{
		TArray<FString> Pieces;
		Parameters.ParseIntoArray(Pieces, TEXT(","), /*InCullEmpty*/ true);

		TArray<FString> Types;
		for (FString Piece : Pieces)
		{
			Piece.TrimStartAndEndInline();

			// Il nome del parametro e' l'ultimo identificatore. Si cerca all'indietro finche' i caratteri
			// sono alfanumerici, e si toglie solo se davanti resta qualcosa — cioe' un tipo.
			int32 Start = Piece.Len();
			while (Start > 0)
			{
				const TCHAR Previous = Piece[Start - 1];
				if (FChar::IsAlnum(Previous) || Previous == TCHAR('_'))
				{
					--Start;
					continue;
				}
				break;
			}
			if (Start > 0 && Start < Piece.Len())
			{
				const TCHAR Before = Piece[Start - 1];
				// `int32 Value` (spazio) ma anche `const TCHAR *Path` / `FString &Out`: in tutti e tre il
				// nome e' un'appendice del tipo, non parte di esso.
				if (Before == TCHAR(' ') || Before == TCHAR('*') || Before == TCHAR('&'))
				{
					Piece = Piece.Left(Start);
				}
			}

			Piece.TrimStartAndEndInline();
			Piece.ReplaceInline(TEXT(" "), TEXT(""));
			Types.Add(Piece);
		}
		return FString::Join(Types, TEXT(","));
	}

	/**
	 * La riga con il contenuto di `"..."`, `'...'` e dei COMMENTI sostituito da spazi.
	 *
	 * ⛔ **Le graffe dentro un letterale o un commento non sono graffe**, e ignorarlo falsa la profondita' per
	 * tutto cio' che segue nel file. Misurato: prima di questa funzione **un solo** file del corpus derivava,
	 * ed era `RTTestGuardTests.cpp` — cioe' questo, perche' i suoi docstring citano `namespace {` e
	 * `TEXT('{')`. L'oracolo era **cieco sul proprio sorgente**, e verde.
	 *
	 * ⚠️ `bInBlockComment` attraversa le righe: un `/* ... *\/` che si apre e chiude altrove non lascia
	 * graffe orfane dietro di se'.
	 */
	FString RTWithoutLiteralsAndComments(const FString& Line, bool& bInBlockComment)
	{
		FString Out;
		Out.Reserve(Line.Len());
		for (int32 Index = 0; Index < Line.Len(); ++Index)
		{
			const TCHAR Current = Line[Index];
			const TCHAR Next = (Index + 1 < Line.Len()) ? Line[Index + 1] : TCHAR('\0');

			if (bInBlockComment)
			{
				if (Current == TCHAR('*') && Next == TCHAR('/'))
				{
					bInBlockComment = false;
					++Index;
				}
				continue;
			}
			if (Current == TCHAR('/') && Next == TCHAR('*'))
			{
				bInBlockComment = true;
				++Index;
				continue;
			}
			if (Current == TCHAR('/') && Next == TCHAR('/'))
			{
				break; // commento di riga: il resto non esiste
			}
			if (Current == TCHAR('"') || Current == TCHAR('\''))
			{
				// Il letterale diventa uno spazio: la riga resta allineata come lunghezza logica.
				Out.AppendChar(TCHAR(' '));
				for (++Index; Index < Line.Len(); ++Index)
				{
					if (Line[Index] == TCHAR('\\'))
					{
						++Index;
						continue;
					}
					if (Line[Index] == Current)
					{
						break;
					}
				}
				continue;
			}
			Out.AppendChar(Current);
		}
		return Out;
	}

	/**
	 * Vero se il testo davanti al nome somiglia a un **tipo di ritorno**, non a un pezzo di espressione.
	 *
	 * ⛔ **Senza questo, `return Foo(x)` spezzata a capo e' indistinguibile da una definizione.** Misurate sul
	 * corpus: `return AnchorCellCenter(Ref.Cell)` e `return BlueprintPropertyCarriesTexture(AsMap->KeyProp)`
	 * venivano estratte come firme. Due file con la stessa continuazione avrebbero reso il gate **rosso su
	 * codice giusto** — cioe' spento entro un giorno, che e' il modo noto in cui un gate muore.
	 */
	bool RTLooksLikeReturnType(const FString& Prefix)
	{
		const FString Trimmed = Prefix.TrimStartAndEnd();
		if (Trimmed.IsEmpty())
		{
			return false;
		}
		static const TCHAR* const Forbidden[] = {
			TEXT("return"), TEXT("="), TEXT(","), TEXT("||"), TEXT("&&"),
			TEXT("+"), TEXT("?"), TEXT(":"), TEXT("!")
		};
		for (const TCHAR* const Word : Forbidden)
		{
			if (Trimmed.Contains(Word, ESearchCase::CaseSensitive))
			{
				return false;
			}
		}
		return true;
	}

	/**
	 * Estrae le firme delle funzioni libere definite nei namespace anonimi di un sorgente.
	 *
	 * 🔑 **Una funzione libera sta a profondita' ESATTAMENTE 1**, cioe' direttamente dentro il namespace
	 * anonimo; un membro di `struct` sta a 2 o piu'. E' la regola che sostituisce un contatore di
	 * `struct`/`class`, e non e' una semplificazione estetica: quel contatore non tornava a zero ne' dopo una
	 * struct **annidata** ne' dopo una scritta **su una riga sola** (`Delta == 0`, quindi nessun decremento),
	 * e da li' in poi l'estrattore restava cieco — verde, e senza dirlo. Misurate due occorrenze reali:
	 * `FRTProbeUnit` in `RTBotStalemateProbeTests.cpp` nascondeva `MakeProbeRoster()`, `FRTBasinRow` in
	 * `RTShowcaseScenarioTests.cpp` nascondeva `BasinRows()`.
	 *
	 * ⚠️ Riconosce **entrambi** gli stili di graffa. Quello di questo repository e' `namespace` e `{` su righe
	 * diverse — **161 file su 200**, e **zero** usano `namespace {` in linea: quel ramo esiste per non
	 * dipendere da una convenzione, non perche' sia esercitato dal corpus.
	 */
	void RTExtractAnonymousSignatures(const FString& FileName, const FString& Text,
		TArray<FRTAnonymousSignature>& Out)
	{
		TArray<FString> Lines;
		Text.ParseIntoArrayLines(Lines, /*InCullEmpty*/ false);

		bool bInsideAnonymous = false;
		bool bInBlockComment = false;
		int32 Depth = 0;

		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			// Letterali e commenti via PRIMA di qualunque conteggio: vedi `RTWithoutLiteralsAndComments`.
			const FString Line = RTWithoutLiteralsAndComments(Lines[Index], bInBlockComment).TrimStartAndEnd();

			if (!bInsideAnonymous)
			{
				const bool bSameLine = (Line == TEXT("namespace {"));
				bool bAllman = false;
				if (!bSameLine && Line == TEXT("namespace") && Index + 1 < Lines.Num())
				{
					bool bLookahead = bInBlockComment;
					bAllman = RTWithoutLiteralsAndComments(Lines[Index + 1], bLookahead).TrimStartAndEnd()
						== TEXT("{");
				}
				if (bSameLine || bAllman)
				{
					bInsideAnonymous = true;
					Depth = 1;
					if (bAllman)
					{
						++Index;
					}
				}
				continue;
			}

			int32 Delta = 0;
			for (const TCHAR Character : Line)
			{
				Delta += (Character == TCHAR('{')) ? 1 : ((Character == TCHAR('}')) ? -1 : 0);
			}

			// La profondita' PRIMA della riga: la firma di una funzione libera si trova a 1, sia che la
			// graffa apra sulla stessa riga sia che apra sotto.
			const int32 DepthBefore = Depth;
			Depth += Delta;
			if (Depth <= 0)
			{
				bInsideAnonymous = false;
				continue;
			}
			if (DepthBefore != 1 || Line.IsEmpty() || Line.StartsWith(TEXT("#")))
			{
				continue;
			}

			int32 Open = INDEX_NONE;
			if (!Line.FindChar(TCHAR('('), Open) || Open == 0)
			{
				continue;
			}
			const int32 Close = Line.Find(TEXT(")"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			if (Close < Open)
			{
				continue;
			}

			// Dopo la parentesi chiusa: solo la graffa o niente. Una chiamata finisce con `;` ed esce di qui.
			const FString Tail = Line.Mid(Close + 1).TrimStartAndEnd();
			if (!Tail.IsEmpty() && Tail != TEXT("{"))
			{
				continue;
			}

			// Il nome e' l'identificatore che precede la parentesi; davanti deve esserci un tipo di ritorno.
			int32 Start = Open;
			while (Start > 0)
			{
				const TCHAR Previous = Line[Start - 1];
				if (FChar::IsAlnum(Previous) || Previous == TCHAR('_'))
				{
					--Start;
					continue;
				}
				break;
			}
			if (Start == Open || Start == 0 || !RTLooksLikeReturnType(Line.Left(Start)))
			{
				continue;
			}

			const FString Name = Line.Mid(Start, Open - Start);
			if (Name.IsEmpty() || RTIsKeyword(Name))
			{
				continue;
			}

			const FString Parameters = Line.Mid(Open + 1, Close - Open - 1);
			FRTAnonymousSignature Found;
			Found.Key = FString::Printf(TEXT("%s(%s)"), *Name, *RTParameterTypes(Parameters));
			Found.Readable = FString::Printf(TEXT("%s(%s)"), *Name, *Parameters.TrimStartAndEnd());
			Found.File = FileName;
			Found.Line = Index + 1;
			Out.Add(MoveTemp(Found));
		}
	}
}

/**
 * **Nessuna funzione libera in namespace anonimo ha nome E firma uguali a una di un altro file.**
 *
 * 🔴 **Questo oracolo esiste perche' il difetto ha fermato `main` il 2026-09-05** (#2397): `StandStill(ARTUnit*)`
 * era definita in namespace anonimo sia in `RTStatusTests.cpp` sia in `RTUnbalancedProneTests.cpp`. In C++ e'
 * legale — due unita' di traduzione separate, internal linkage — ma la **unity build** le concatena nello
 * stesso blob, e li' due definizioni identiche sono `C2084`, piu' una cascata di `C2264` su ogni chiamata
 * (tredici, quel giorno).
 *
 * 🔑 **Il compilatore, nella build di chi introduce il difetto, e' cieco per costruzione.**
 * `bUseAdaptiveUnityBuild` e' `true` di default e UBT usa `git status` per il working set: il file che stai
 * modificando viene **escluso dal blob**, quindi compila. Esplode per il prossimo, il cui working set e'
 * diverso. ∴ non e' che «il raggruppamento cambia»: e' che l'unica build in cui il difetto sarebbe visibile
 * e' quella che nessuno esegue.
 *
 * ⚠️ **E la collisione dipende dal blob, non solo dai nomi**: al 2026-09-05 `RTStatusTests` sta nel blob 19 e
 * `RTUnbalancedProneTests` nel 20 — le due `StandStill` **non** collidere­bbero oggi. Un difetto armato che
 * non spara resta un difetto: e' il motivo per cui l'oracolo e' statico e non «prova a compilare».
 *
 * ---
 *
 * 🔑 **Le TRE condizioni, che sono la specifica.** Una `C2084` le richiede tutte insieme, e un gate che ne
 * verificasse meno sarebbe rosso su codice giusto — cioe' spento entro un giorno:
 *
 * 1. **stesso modulo** — l'unity blob non attraversa i moduli. Qui e' garantito dal soggetto: si legge solo
 *    `Source/RefactorTactics/Tests`, che sta tutto nel modulo runtime.
 * 2. **stessa firma** — nome **e** tipi dei parametri. Due omonime con parametri diversi sono **overload
 *    legali**: confrontare i soli nomi darebbe **25** candidati contro **5** firme reali (misurato).
 * 3. **funzione libera** — i membri di `struct`/`class` non collidono mai fra loro.
 *
 * ---
 *
 * ⚠️ **I SETTE LIMITI, misurati e dichiarati invece che scoperti dopo.** Questo test e' facile da leggere come
 * *«nessuna collisione di unity build»*, e non e' cio' che misura:
 *
 * 1. **Guarda `Source/RefactorTactics/Tests`, non tutto il modulo.** L'unity blob contiene **anche** `Turn/`,
 *    `Map/`, `Unit/`: un helper di test e uno di produzione con la stessa firma collidono allo stesso modo.
 *    Allargare il perimetro significa cambiare `RTLeggiSorgentiDeiTest`, che ha altri due consumatori.
 * 2. **Il modulo `RefactorTacticsEditor` non e' coperto.** Non puo' collidere con questo — vedi condizione 1
 *    — ma puo' collidere con se stesso, e li' non c'e' nessun oracolo.
 * 3. **Solo `*.cpp`.** ⛔ **Gli header di `Tests/` sono fuori, e sono quindici** — compreso
 *    `RTWidgetAssetTestHelpers.h`, nato con questo stesso oracolo. Una funzione **non** `inline` in un header
 *    incluso da due `.cpp` e' una ridefinizione nello stesso blob: e' la stessa `C2084` e questo test non la
 *    vede. Chi aggiunge helper in header li scriva `inline` o in un namespace nominato.
 * 4. **Solo funzioni.** Variabili, costanti e tipi omonimi seguono la stessa regola e non sono coperti.
 * 5. **Solo firme su UNA riga**, e non sono poche: misurate il 2026-09-05, **60** definizioni su ~730 stanno
 *    su piu' righe — l'**8%**, e sono gli helper piu' grossi. Il numero sta qui invece di *«sono poche»*
 *    perche' un limite non misurato non e' un limite dichiarato.
 * 6. **Le definizioni con il corpo sulla stessa riga** (`int32 Foo() { return 1; }`) non sono viste: l'ultima
 *    `)` cade dentro il corpo, quindi la coda non e' ne' vuota ne' `{`. Sono i nomi corti e generici, cioe'
 *    proprio quelli che un secondo file ricrea.
 * 7. **Le funzioni `static` a scope di file** hanno anch'esse internal linkage e collidono identicamente, ma
 *    stanno fuori da ogni `namespace {` e questo estrattore non le guarda. Misurate: **5** nel corpus.
 *
 * ⛔ **La guardia anti-vacuita' e' obbligatoria qui piu' che altrove**, perche' il conteggio atteso e' **zero**
 * — e uno zero non distingue *«non ci sono collisioni»* da *«l'estrattore non riconosce piu' un namespace
 * anonimo»*. Il campione sintetico porta quattro casi: due firme che **devono** essere estratte e due esche
 * che **non** devono esserlo (una continuazione `return`, un membro di `struct` scritta su una riga sola).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnonymousHelpersDoNotCollideTest,
	"RefactorTactics.Meta.AnonymousHelpersDoNotCollideUnderUnity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnonymousHelpersDoNotCollideTest::RunTest(const FString&)
{
	// ⛔ Il controllo POSITIVO viene prima di tutto: se l'estrattore e' cieco, il conteggio zero piu' sotto
	// sarebbe verde e falso. Il campione porta lo stile di graffa di questo repository, che e' il tranello.
	{
		const FString Sample =
			TEXT("namespace\n")
			TEXT("{\n")
			TEXT("\tstruct FRTSampleRecord { int32 Id; int32 Team; };\n")
			TEXT("\n")
			TEXT("\tvoid RTSampleToExtract(int32 Value)\n")
			TEXT("\t{\n")
			TEXT("\t}\n")
			TEXT("\n")
			TEXT("\tint32 RTSampleWithContinuation()\n")
			TEXT("\t{\n")
			TEXT("\t\treturn RTSampleToExtract(0)\n")
			TEXT("\t\t\t+ 1;\n")
			TEXT("\t}\n")
			TEXT("}\n");

		TArray<FRTAnonymousSignature> Extracted;
		RTExtractAnonymousSignatures(TEXT("<sample>"), Sample, Extracted);

		// Due firme libere; la struct su una riga sola non deve congelare nulla, e il `return` non e' una
		// definizione. E' il caso che una prima stesura sbagliava in tre modi diversi.
		if (!TestEqual(TEXT("il campione produce due firme libere"), Extracted.Num(), 2))
		{
			for (const FRTAnonymousSignature& Signature : Extracted)
			{
				AddError(FString::Printf(TEXT("estratta inattesa: %s"), *Signature.Key));
			}
			return false;
		}
		if (!TestEqual(TEXT("la prima e' la funzione libera dopo la struct di una riga"),
			Extracted[0].Key, FString(TEXT("RTSampleToExtract(int32)"))))
		{
			return false;
		}
		if (!TestEqual(TEXT("la seconda e' quella con la continuazione, non il suo `return`"),
			Extracted[1].Key, FString(TEXT("RTSampleWithContinuation()"))))
		{
			return false;
		}

		// La normalizzazione: `*` incollato al nome non deve cambiare la chiave.
		if (!TestEqual(TEXT("`const TCHAR *Path` e `const TCHAR* Path` hanno la stessa chiave"),
			RTParameterTypes(TEXT("const TCHAR *Path")), RTParameterTypes(TEXT("const TCHAR* Path"))))
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

	TArray<FRTAnonymousSignature> All;
	for (const FRTSorgenteDiTest& Sorgente : Sorgenti)
	{
		RTExtractAnonymousSignatures(Sorgente.Nome, Sorgente.Testo, All);
	}

	// La stessa firma nello stesso file e' un'altra cosa (overload dichiarato e definito, o una svista che il
	// compilatore prende da solo): qui interessa solo cio' che attraversa due unita' di traduzione.
	TMap<FString, TArray<FRTAnonymousSignature>> ByKey;
	for (const FRTAnonymousSignature& Signature : All)
	{
		ByKey.FindOrAdd(Signature.Key).Add(Signature);
	}

	int32 Collisions = 0;
	for (const TPair<FString, TArray<FRTAnonymousSignature>>& Entry : ByKey)
	{
		TSet<FString> DistinctFiles;
		for (const FRTAnonymousSignature& Signature : Entry.Value)
		{
			DistinctFiles.Add(Signature.File);
		}
		if (DistinctFiles.Num() < 2)
		{
			continue;
		}

		++Collisions;
		FString Where;
		for (const FRTAnonymousSignature& Signature : Entry.Value)
		{
			Where += FString::Printf(TEXT("\n    %s:%d"), *Signature.File, Signature.Line);
		}
		AddError(FString::Printf(
			TEXT("%s e' definita in namespace anonimo in %d file diversi dello stesso modulo:%s\n")
			TEXT("  Il namespace anonimo NON protegge sotto unity build: le due definizioni finiscono nello ")
			TEXT("stesso blob e la compilazione muore con C2084, piu' un C2264 per ogni chiamata. Rinomina ")
			TEXT("quella del file piu' recente, oppure — se i CORPI sono identici — spostala in un header ")
			TEXT("condiviso con namespace NOMINATO e funzioni `inline`. Se i corpi differiscono NON ")
			TEXT("unificarle: vedi #2397."),
			*Entry.Value[0].Readable, DistinctFiles.Num(), *Where));
	}

	AddInfo(FString::Printf(TEXT("file ispezionati: %d, firme in namespace anonimo: %d"),
		Sorgenti.Num(), All.Num()));

	// Secondo controllo positivo: se l'estrattore smettesse di trovare firme sul corpus reale, il conteggio
	// delle collisioni resterebbe zero senza che nulla sia stato guardato.
	TestTrue(TEXT("il corpus contiene firme in namespace anonimo"), All.Num() > 0);
	TestEqual(TEXT("nessuna firma anonima e' definita in due file"), Collisions, 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
