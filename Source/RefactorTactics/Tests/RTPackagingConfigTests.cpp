#include "Misc/AutomationTest.h"

#include "Containers/StringConv.h"
#include "HAL/FileManager.h"
#include "HAL/UnrealMemory.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "Unit/RTUnitAnimInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * **La configurazione di packaging del namespace d'authoring** (#1804, `D-280`).
 *
 * 🔴 **Cosa questi test NON provano, ed e' la prima cosa da dire.** Non provano che un pacchetto escluda
 * davvero gli strumenti d'authoring: leggono `DefaultGame.ini`, non un `.utoc`. L'oracolo di quel fatto e'
 * un pacchetto vero — `BuildCookRun` piu' `UnrealPak -List` — ed e' registrato fra le verifiche manuali,
 * validato per mutazione. Questi test sono **guardiani di regressione**: impediscono che la riga sparisca
 * senza che nulla lo dica, il che e' il modo in cui un'esclusione di packaging si perde davvero.
 *
 * 🔴 **E il divario si e' rivelato piu' largo di cosi', misurato il 2026-09-03 su pacchetti veri.**
 * Cotto due volte con `BuildCookRun`, una con `DirectoriesToNeverCook` e una senza, i container sono
 * risultati **identici**: `WBP_RT_ScenarioComposer` e `WBP_RT_GrayKitPlayground` sono `EditorUtilityWidget`,
 * quindi editor-only, e il cook li scarta **per classe** (`LogCook: SkipOnlyEditorOnly is enabled`) prima
 * che una directory conti qualcosa. ∴ **Oggi la riga che questo test presidia non e' cio' che tiene i due
 * strumenti fuori dal pacchetto**, e un verde qui non va letto come "gli strumenti sono esclusi".
 *
 * ✅ **Cio' che il test presidia resta necessario**, e anche questo e' misurato: un never-cook di controllo
 * su `/Game/RT/UI/Icons` fa sparire 62 package dal container, quindi il meccanismo funziona e batte
 * `DirectoriesToAlwaysCook` su una sottodirectory. La riga e' la garanzia della **famiglia aperta** — il
 * primo asset NON editor-only messo sotto `/Game/RT/Editor/` sara' escluso solo da lei.
 * ⛔ Quindi questo test difende un invariante **futuro**: e' il caso in cui un guardiano di regressione
 * vale di piu' del fatto che oggi verifica, non di meno.
 *
 * 🔴 **Riscritto in code review (#2136), che ha trovato la prima versione VACUA su tre fronti.** Vale la
 * pena elencarli, perche' sono tre modi diversi di scrivere un oracolo che non puo' fallire:
 *
 * 1. **Leggeva `GGameIni`, cioe' la gerarchia MERGED, mentre il docstring dichiarava di leggere il file.**
 *    `GConfig` fonde `Base*.ini` + `Config/DefaultGame.ini` + `Saved/Config/WindowsEditor/Game.ini`. Chi
 *    avesse quella sezione nel proprio `Saved/` — Unreal ce la scrive da se' con `SaveConfig()` — poteva
 *    cancellare la riga dal file **versionato**, vedere verde e mergiare. Cioe' esattamente *"la riga
 *    sparisce senza che nulla lo dica"*, l'unica regressione che questo test dichiara di impedire.
 *    ∴ ora legge `Config/DefaultGame.ini` dal disco, e il codice dice cio' che il commento prometteva.
 * 2. **Confrontava per SOTTOSTRINGA.** `Contains("/Game/RT/Editor")` e' vero anche per
 *    `(Path="/Game/RT/Editor/Scenario")` — cioe' restringere l'esclusione al solo Composer di oggi
 *    passava, perdendo la **famiglia aperta** su cui poggia tutto il disegno. Simmetricamente
 *    `Contains("/Game/RT")` sopravviveva a un `AlwaysCook` ristretto a `/Game/RT/UI`, che e' proprio il
 *    giorno in cui la coppia delle due righe smette di avere senso. ∴ ora il `Path` si estrae e si
 *    confronta per **uguaglianza esatta**.
 * 3. **Asseriva che nessuna voce contenesse `.uasset`.** `DirectoriesToNeverCook` e' un
 *    `TArray<FDirectoryPath>` di package path: `.uasset` non compare **mai**, nemmeno nel caso rotto —
 *    e il caso rotto che l'asserzione descriveva si scrive senza estensione
 *    (`(Path="/Game/RT/Editor/Scenario/WBP_RT_ScenarioComposer")`), quindi passava anche lui. Verde nel
 *    caso buono e verde in quello cattivo: la definizione di un non-oracolo.
 *    ∴ sostituita dal confronto esatto del punto 2, che il caso rotto lo **fa cadere**.
 *
 * ✅ **E il punto 2 e' stato VALIDATO PER MUTAZIONE, non dichiarato.** Ristretta la riga a
 * `+DirectoriesToNeverCook=(Path="/Game/RT/Editor/Scenario")` e rieseguita la suite: `1/1 completati,
 * 1 fallimenti`, con il messaggio *«e la FAMIGLIA /Game/RT/Editor e' esclusa, non un suo singolo ramo»*.
 * 🔑 **E' esattamente il caso che la versione a sottostringa lasciava passare verde**: la mutazione non
 * serviva a mostrare che il test sa fallire in generale, ma che sa fallire **su questo**.
 */

namespace
{
	const TCHAR* RTPackagingSection = TEXT("[/Script/UnrealEd.ProjectPackagingSettings]");

	/** Il file VERSIONATO, non la gerarchia merged: e' la differenza fra sorvegliare il repository e sorvegliare la macchina. */
	FString RTDefaultGameIniPath()
	{
		return FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultGame.ini"));
	}

	/**
	 * I `Path` dichiarati da `+<Key>=(Path="...")` nella sezione di packaging, **esattamente come scritti**.
	 *
	 * ⚠️ Traccia la sezione: la stessa chiave sotto un'altra intestazione non conterebbe, e un file di
	 * config e' fatto di sezioni prima che di righe.
	 */
	TArray<FString> ReadPackagingPaths(const TArray<FString>& Righe, const TCHAR* Key)
	{
		const FString Prefisso = FString::Printf(TEXT("+%s=(Path=\""), Key);
		TArray<FString> Paths;
		bool bDentroLaSezione = false;

		for (const FString& Riga : Righe)
		{
			const FString Pulita = Riga.TrimStartAndEnd();
			if (Pulita.StartsWith(TEXT("[")))
			{
				bDentroLaSezione = Pulita.Equals(RTPackagingSection);
				continue;
			}
			if (!bDentroLaSezione || !Pulita.StartsWith(Prefisso))
			{
				continue;
			}

			const int32 Inizio = Prefisso.Len();
			const int32 Fine = Pulita.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Inizio);
			if (Fine > Inizio)
			{
				Paths.Add(Pulita.Mid(Inizio, Fine - Inizio));
			}
		}
		return Paths;
	}
}

/**
 * 🔑 **Il namespace d'authoring e' escluso, e l'esclusione ha senso solo CONTRO l'inclusione.**
 *
 * ⛔ Un test che verificasse la sola riga di never-cook sarebbe verde anche il giorno in cui qualcuno
 * togliesse `DirectoriesToAlwaysCook`: a quel punto l'esclusione non escluderebbe piu' niente di
 * particolare, e il pannello sparirebbe dal pacchetto **per un'altra ragione**. Le due righe si leggono
 * insieme, e questo test le pretende entrambe — **per uguaglianza esatta**, perche' entrambe possono essere
 * ristrette invece che cancellate, e una restrizione non assomiglia a una regressione mentre lo e'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPackagingEditorNamespaceNeverCookedTest,
	"RefactorTactics.Packaging.EditorNamespaceIsNeverCooked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPackagingEditorNamespaceNeverCookedTest::RunTest(const FString&)
{
	TArray<FString> Righe;
	if (!TestTrue(TEXT("Config/DefaultGame.ini si legge"), FFileHelper::LoadFileToStringArray(Righe, *RTDefaultGameIniPath())))
	{
		return false;
	}

	const TArray<FString> AlwaysCook = ReadPackagingPaths(Righe, TEXT("DirectoriesToAlwaysCook"));
	const TArray<FString> NeverCook  = ReadPackagingPaths(Righe, TEXT("DirectoriesToNeverCook"));

	// ⛔ Senza questa, le asserzioni sotto sarebbero vacue: due liste vuote passerebbero un test che
	// cercasse solo l'assenza di qualcosa. ➕ E vale anche come controllo del PARSER: se un domani il
	// formato di queste righe cambiasse, `ReadPackagingPaths` restituirebbe liste vuote e questo test
	// direbbe "la riga non c'e'" per un difetto suo. Meglio rosso che verde su nulla.
	if (!TestTrue(TEXT("DefaultGame.ini dichiara delle directory da cuocere"), AlwaysCook.Num() > 0))
	{
		return false;
	}

	// 🔑 Uguaglianza, non sottostringa: `/Game/RT/UI` contiene `/Game/RT` e non e' la stessa promessa.
	TestTrue(TEXT("/Game/RT e' incluso nel cook, per intero e non una sua sottocartella"),
		AlwaysCook.Contains(TEXT("/Game/RT")));

	// 🔴 Qui l'uguaglianza e' il punto dell'intera issue: `/Game/RT/Editor/Scenario` proteggerebbe il
	// Composer di oggi e lascerebbe scoperto il prossimo strumento — che e' cio' che `D-280` istituisce il
	// namespace per NON dover fare a mano a ogni asset.
	TestTrue(TEXT("e la FAMIGLIA /Game/RT/Editor e' esclusa, non un suo singolo ramo"),
		NeverCook.Contains(TEXT("/Game/RT/Editor")));
	return true;
}


/**
 * **Nessun package fuori da `/Game/RT/Editor/` ne referenzia uno dentro** (#2150, `D-280`).
 *
 * 🔑 **La directory e' UNA delle due vie d'ingresso nel cook, e questo test presidia l'ALTRA.**
 * `EditorNamespaceIsNeverCooked` qui sopra guarda la configurazione; `PIE-PKG-EDITOR-NAMESPACE` guarda il
 * contenuto del container. Nessuno dei due vede un asset trascinato dentro **per riferimento**, che e' il
 * modo in cui un package escluso rientra comunque: se qualcosa di cotto lo referenzia, il cook lo segue.
 *
 * ⚠️ **Cosa protegge DAVVERO, perche' altrimenti sembra ridondante.** Gli strumenti di oggi sono
 * `EditorUtilityWidget`, quindi editor-only, e il cook li scarta **per classe** — #1804 lo ha misurato su
 * pacchetti veri: togliere il never-cook non li fa comparire. Un riferimento entrante verso di loro sarebbe
 * un difetto, ma non li porterebbe nel pacchetto. Il caso che questo test protegge e' il **prossimo** asset
 * messo sotto quel namespace che **non** sia editor-only — una texture, una mesh, un data asset: li'
 * `SkipOnlyEditorOnly` non aiuta, `DirectoriesToNeverCook` lo escluderebbe, e un riferimento da fuori lo
 * rimetterebbe dentro.
 *
 * 🔴 **NON usa l'`AssetRegistry`, e la ragione e' gia' scritta in questo repository.**
 * `RTHexMapTests.cpp` rifiuta quella strada per un gate con parole che valgono identiche qui: *«dipende
 * dall'AssetRegistry popolato e sarebbe verde per vacuita' il giorno in cui non lo trovasse — cioe' proprio
 * il modo in cui un gate smette di guardare senza dirlo»*. Un test `EditorContext` che lo interrogasse prima
 * della fine della scansione otterrebbe **zero riferimenti e passerebbe**. ∴ si leggono i byte da disco, col
 * pattern di `RTHexMapTests` e `RTGoldenCorpusTests`.
 *
 * ⚠️ **Si cerca in ANSI E in UTF-16LE.** Unreal salva le `FString` in entrambe a seconda del contenuto, e
 * una ricerca solo-ASCII darebbe uno **zero falso** — che qui sarebbe indistinguibile dal verde.
 *
 * ---
 *
 * ⛔ **I TRE LIMITI, dichiarati invece che lasciati dedurre:**
 *
 * 1. **L'invariante era gia' vero quando questo gate e' nato**, misurato il 2026-09-03: **0** riferimenti
 *    entranti su 115 package esterni, su cinque livelli di token e due codifiche. ∴ questo test **non ha
 *    scoperto un difetto**: e' un guardiano di regressione, e il suo valore e' tutto nel futuro. Chi lo
 *    trovasse verde e lo credesse inutile starebbe leggendo la sua riuscita come inutilita'.
 * 2. **Non vede i riferimenti da C++ ne' da `.ini`**: un `FSoftObjectPath` costruito a mano nel codice, o un
 *    path in `DefaultEngine.ini`, non stanno in nessun `.uasset`. Misurati a parte lo stesso giorno
 *    (`git grep "/Game/RT/Editor" -- Source/ Config/ Plugins/` da' **0**), ma questo test non li sorveglia.
 * 3. **Non ricostruisce le catene transitive**: se A (fuori) referenzia B (fuori) che referenzia C (dentro),
 *    l'arco B->C viene visto — ed e' quello che conta — ma la catena non viene tracciata.
 *
 * ⚠️ Il perimetro e' `Content/RT/`, il namespace proprietario, non l'intera `Content/`: e' cio' che
 * `+DirectoriesToAlwaysCook=(Path="/Game/RT")` copre, e gli asset di terze parti non possono referenziare
 * `/Game/RT/Editor/` per costruzione.
 */

namespace
{
	const TCHAR* RTEditorNamespace = TEXT("/Game/RT/Editor/");

	/** I `.uasset`/`.umap` sotto una radice, come percorsi assoluti. */
	TArray<FString> RTPackageFilesUnder(const FString& Root)
	{
		TArray<FString> Files;
		IFileManager::Get().FindFilesRecursive(Files, *Root, TEXT("*.uasset"), true, false);
		IFileManager::Get().FindFilesRecursive(Files, *Root, TEXT("*.umap"), true, false, false);
		return Files;
	}

	/**
	 * Il package path lungo di un file: `.../Content/RT/UI/X.uasset` diventa `/Game/RT/UI/X`.
	 *
	 * ⚠️ E' la chiave con cui un package ne nomina un altro nella propria tabella di import, quindi e'
	 * questa — e non il nome nudo — la stringa da cercare.
	 */
	FString RTPackagePathOf(const FString& AbsoluteFile, const FString& ContentDir)
	{
		FString Relative = AbsoluteFile;
		FPaths::MakePathRelativeTo(Relative, *ContentDir);
		Relative.ReplaceInline(TEXT("\\"), TEXT("/"));
		// ⚠️ `GetPath` + `GetBaseFilename` e non `SetExtension(TEXT(""))`: la seconda lascia il punto
		// finale, e un package path che finisse con `.` non corrisponderebbe a nessuna tabella di import.
		const FString Cartella = FPaths::GetPath(Relative);
		const FString Nome     = FPaths::GetBaseFilename(Relative);
		return Cartella.IsEmpty()
			? FString(TEXT("/Game/")) + Nome
			: FString(TEXT("/Game/")) + Cartella + TEXT("/") + Nome;
	}

	/** Cerca il token nei byte in **entrambe** le codifiche con cui Unreal salva le `FString`. */
	bool RTBytesMentionPath(const TArray<uint8>& Bytes, const FString& PackagePath)
	{
		auto Contains = [&Bytes](const uint8* Needle, int32 Len) -> bool
		{
			if (Len <= 0 || Bytes.Num() < Len)
			{
				return false;
			}
			for (int32 I = 0; I + Len <= Bytes.Num(); ++I)
			{
				if (FMemory::Memcmp(Bytes.GetData() + I, Needle, Len) == 0)
				{
					return true;
				}
			}
			return false;
		};

		const FTCHARToUTF8 Ansi(*PackagePath);
		if (Contains(reinterpret_cast<const uint8*>(Ansi.Get()), Ansi.Length()))
		{
			return true;
		}

		TArray<uint8> Wide;
		Wide.Reserve(PackagePath.Len() * 2);
		for (const TCHAR C : PackagePath)
		{
			const uint16 U = static_cast<uint16>(C);
			Wide.Add(static_cast<uint8>(U & 0xFF));
			Wide.Add(static_cast<uint8>((U >> 8) & 0xFF));
		}
		return Contains(Wide.GetData(), Wide.Num());
	}
}

/**
 * 🔑 **L'invariante, piu' i due controlli che gli impediscono di essere verde per vacuita'.**
 *
 * Un test il cui esito atteso e' **zero** non puo' distinguere «non c'e' nessuna violazione» da «non ho
 * guardato niente»: sono lo stesso numero. Per questo qui ci sono **due** asserzioni positive prima di
 * quella che conta — il soggetto esiste, e il metodo di ricerca sa trovare archi veri.
 *
 * ✅ **VALIDATO PER MUTAZIONE, e la prima mutazione era sbagliata — vale la pena dire perche'.**
 *
 * *Primo tentativo*: puntato l'oracolo su `/Game/RT/UI/Match/`, un namespace che SO essere referenziato
 * (`WBP_RT_ActionDock` -> `WBP_RT_ActionSlot`). **Resta VERDE**, e correttamente: i due asset stanno
 * **entrambi** in `Match/`, quindi quell'arco e' *dentro->dentro*, che questo test ignora per costruzione.
 * ⚠️ Una mutazione che non puo' cambiare l'esito non valida niente: sceglierla male e' facile quanto
 * scrivere l'oracolo male, e il verde che ne esce ha esattamente lo stesso aspetto di quello buono.
 *
 * *Secondo tentativo*: `/Game/RT/UI/Icons/`, dove `DA_IconCatalog` vive **fuori** (in `/Game/RT/UI/`) e
 * referenzia le icone **dentro**. **ROSSO**, `3/3 completati, 1 fallimenti`, con gli archi elencati uno per
 * uno: *«/Game/RT/UI/DA_IconCatalog referenzia /Game/RT/UI/Icons/RT_UI_Icon_Action_Anchor»*.
 *
 * 🔴 **E il rosso ha trovato un difetto nel messaggio d'errore**, che il verde non poteva mostrare: il
 * namespace era **trascritto a mano** nel testo, cosi' sotto mutazione l'oracolo diceva *«/Game/RT/Editor/
 * e' escluso dal cook»* mentre stava misurando `/Game/RT/UI/Icons/`. Nel funzionamento normale le due
 * stringhe coincidono, ed e' precisamente il motivo per cui un difetto del genere non si vede mai: ora il
 * messaggio nomina la **costante**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoReferencesIntoEditorNamespaceTest,
	"RefactorTactics.Packaging.NoReferencesIntoEditorNamespace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoReferencesIntoEditorNamespaceTest::RunTest(const FString&)
{
	const FString ContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	const FString Perimetro  = FPaths::Combine(ContentDir, TEXT("RT"));

	const TArray<FString> Files = RTPackageFilesUnder(Perimetro);

	// ⛔ **Anti-vacuita' (a): il soggetto esiste.** Un oracolo che perde i propri file e resta verde e'
	// peggio di un oracolo assente. La soglia non e' il conteggio esatto — che cresce a ogni asset nuovo —
	// ma abbastanza alta da cadere se la scansione guardasse la cartella sbagliata: 117 il 2026-09-03.
	if (!TestTrue(TEXT("i package di Content/RT/ si trovano"), Files.Num() >= 100))
	{
		AddError(FString::Printf(TEXT("trovati %d package sotto %s: la scansione guarda il posto sbagliato"),
			Files.Num(), *Perimetro));
		return false;
	}

	TArray<FString> Dentro;
	TArray<FString> Fuori;
	TMap<FString, FString> PathOf;
	for (const FString& File : Files)
	{
		const FString Package = RTPackagePathOf(File, ContentDir);
		PathOf.Add(File, Package);
		if (Package.StartsWith(RTEditorNamespace))
		{
			Dentro.Add(File);
		}
		else
		{
			Fuori.Add(File);
		}
	}

	// Nessun asset d'authoring: il test non ha soggetto, e non e' un difetto — e' un repository in cui il
	// namespace e' ancora vuoto. Lo si dichiara e si esce verdi, invece di fingere una verifica.
	if (Dentro.Num() == 0)
	{
		AddInfo(TEXT("nessun package sotto /Game/RT/Editor/: invariante vacuamente vero, niente da sorvegliare"));
		return true;
	}

	int32 ArchiVeri = 0;
	TArray<TPair<FString, FString>> Violazioni;

	for (const FString& File : Fuori)
	{
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *File))
		{
			AddError(FString::Printf(TEXT("%s non si legge"), *File));
			continue;
		}

		for (const FString& Bersaglio : Dentro)
		{
			if (RTBytesMentionPath(Bytes, PathOf[Bersaglio]))
			{
				Violazioni.Emplace(PathOf[File], PathOf[Bersaglio]);
			}
		}

		// ⛔ **Anti-vacuita' (b): il metodo sa trovare archi VERI.** Senza questo, un difetto nella ricerca
		// — codifica sbagliata, package path costruito male, `Memcmp` che non trova mai — darebbe zero
		// violazioni e un verde perfetto. Si contano gli archi verso QUALSIASI package, non solo verso il
		// namespace: il corpus ne ha per costruzione, e se il totale fosse zero il difetto sarebbe qui.
		if (ArchiVeri == 0)
		{
			for (const TPair<FString, FString>& Altro : PathOf)
			{
				if (Altro.Key != File && RTBytesMentionPath(Bytes, Altro.Value))
				{
					++ArchiVeri;
					break;
				}
			}
		}
	}

	AddInfo(FString::Printf(
		TEXT("perimetro Content/RT/: %d package, %d sotto /Game/RT/Editor/ e %d fuori; ")
		TEXT("almeno un riferimento reale trovato: %s"),
		Files.Num(), Dentro.Num(), Fuori.Num(), ArchiVeri > 0 ? TEXT("si") : TEXT("NO")));

	TestTrue(TEXT("la ricerca sa trovare riferimenti veri, quindi lo zero qui sotto vale qualcosa"),
		ArchiVeri > 0);

	for (const TPair<FString, FString>& V : Violazioni)
	{
		// ⚠️ Il namespace si NOMINA dalla costante, non si trascrive: una copia a mano qui sarebbe una
		// seconda fonte, e la verifica di mutazione l'ha mostrata mentire — puntato l'oracolo su
		// `/Game/RT/UI/Icons/`, il messaggio continuava a dire `/Game/RT/Editor/`.
		AddError(FString::Printf(
			TEXT("%s referenzia %s, che vive nel namespace d'authoring `%s`. Quel namespace e' escluso dal ")
			TEXT("cook (D-280, DefaultGame.ini): un riferimento da fuori lo trascina nel pacchetto, oppure ")
			TEXT("lascia il referente rotto a runtime. Sposta l'asset fuori dal namespace, o togli il ")
			TEXT("riferimento."),
			*V.Key, *V.Value, RTEditorNamespace));
	}
	TestEqual(TEXT("nessun package fuori dal namespace d'authoring ne referenzia uno dentro"),
		Violazioni.Num(), 0);
	return true;
}

/**
 * 🔴 **Il controllo positivo dell'oracolo qui sopra, e non e' un extra: e' l'unica evidenza che possa
 * fallire.**
 *
 * L'invariante era **gia' vero** quando il gate e' nato — 0 violazioni su 115 package, misurato — quindi il
 * test principale nasce verde e resta verde, e un verde che non ha mai potuto essere rosso non prova niente.
 * Una mutazione sugli asset versionati non e' praticabile: un `.uasset` non si edita a mano.
 *
 * ∴ la violazione si **fabbrica**, col pattern che `RTHexMapTests` usa gia' per il `.uasset` su disco:
 * si scrivono due finti package sotto `Saved/`, uno "dentro" il namespace e uno "fuori" che ne contiene il
 * path, si punta la stessa ricerca su quella directory e si verifica che **la trovi**. Poi si cancella.
 *
 * ⛔ La fixture vive in `Saved/` e **non** in `Content/`: file di prova sotto `Content/` rischiano di
 * finire versionati, ed e' esattamente il difetto che `.gitignore` di `D-304` e' stato scritto per evitare.
 * ⚠️ E si verifica **entrambe** le codifiche, perche' e' la meta' che un test scritto in fretta dimentica:
 * un finto package ANSI passerebbe anche con una ricerca che non sa leggere UTF-16.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEditorNamespaceScanDetectsAViolationTest,
	"RefactorTactics.Packaging.EditorNamespaceScanDetectsAViolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEditorNamespaceScanDetectsAViolationTest::RunTest(const FString&)
{
	const FString Bersaglio = TEXT("/Game/RT/Editor/Fixture/WBP_RT_FintoStrumento");

	// Un package che NON lo nomina: la ricerca deve tacere, altrimenti direbbe di si' su qualunque cosa.
	TArray<uint8> Innocente;
	const FTCHARToUTF8 Altro(TEXT("/Game/RT/UI/Match/WBP_RT_QualcosAltro"));
	Innocente.Append(reinterpret_cast<const uint8*>(Altro.Get()), Altro.Length());
	TestFalse(TEXT("un package che non nomina il bersaglio non viene segnalato"),
		RTBytesMentionPath(Innocente, Bersaglio));

	// ANSI: la forma piu' comune nella tabella di import.
	TArray<uint8> ColpevoleAnsi;
	ColpevoleAnsi.Add(0x00);
	const FTCHARToUTF8 Ansi(*Bersaglio);
	ColpevoleAnsi.Append(reinterpret_cast<const uint8*>(Ansi.Get()), Ansi.Length());
	ColpevoleAnsi.Add(0x00);
	TestTrue(TEXT("un riferimento ANSI al namespace d'authoring viene trovato"),
		RTBytesMentionPath(ColpevoleAnsi, Bersaglio));

	// UTF-16LE: la meta' che una ricerca scritta in fretta non vede, e che darebbe uno zero falso.
	TArray<uint8> ColpevoleWide;
	ColpevoleWide.Add(0x00);
	for (const TCHAR C : Bersaglio)
	{
		const uint16 U = static_cast<uint16>(C);
		ColpevoleWide.Add(static_cast<uint8>(U & 0xFF));
		ColpevoleWide.Add(static_cast<uint8>((U >> 8) & 0xFF));
	}
	TestTrue(TEXT("un riferimento UTF-16LE al namespace d'authoring viene trovato"),
		RTBytesMentionPath(ColpevoleWide, Bersaglio));

	// E ora la prova su FILE VERI, che e' cio' che il test principale fa: la scansione deve trovare l'arco
	// fabbricato quando la si punta su una directory che lo contiene.
	const FString Radice = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RTTests"), TEXT("Ns2150"));
	const FString FileDentro = FPaths::Combine(Radice, TEXT("RT"), TEXT("Editor"), TEXT("Fixture"), TEXT("WBP_RT_FintoStrumento.uasset"));
	const FString FileFuori  = FPaths::Combine(Radice, TEXT("RT"), TEXT("UI"), TEXT("WBP_RT_FintoReferente.uasset"));

	IFileManager::Get().Delete(*FileDentro, false, true);
	IFileManager::Get().Delete(*FileFuori, false, true);

	TArray<uint8> Vuoto;
	Vuoto.Add(0x00);
	const bool bScrittoDentro = FFileHelper::SaveArrayToFile(Vuoto, *FileDentro);
	const bool bScrittoFuori  = FFileHelper::SaveArrayToFile(ColpevoleAnsi, *FileFuori);
	if (!TestTrue(TEXT("la fixture si scrive sotto Saved/"), bScrittoDentro && bScrittoFuori))
	{
		return false;
	}

	const TArray<FString> Trovati = RTPackageFilesUnder(Radice);
	TestEqual(TEXT("la scansione trova entrambi i finti package"), Trovati.Num(), 2);

	int32 Violazioni = 0;
	for (const FString& File : Trovati)
	{
		const FString Package = RTPackagePathOf(File, Radice);
		if (Package.StartsWith(RTEditorNamespace))
		{
			continue;
		}
		TArray<uint8> Bytes;
		if (FFileHelper::LoadFileToArray(Bytes, *File) && RTBytesMentionPath(Bytes, Bersaglio))
		{
			++Violazioni;
		}
	}

	// ⛔ L'asserzione per cui questo test esiste: **uno**, non zero. Se la scansione non trovasse la
	// violazione fabbricata, il verde del test principale non significherebbe niente.
	TestEqual(TEXT("la scansione TROVA la violazione fabbricata: l'oracolo sa diventare rosso"),
		Violazioni, 1);

	IFileManager::Get().Delete(*FileDentro, false, true);
	IFileManager::Get().Delete(*FileFuori, false, true);
	IFileManager::Get().DeleteDirectory(*Radice, false, true);
	return true;
}


/**
 * **Le clip di animazione richieste dal vertical slice sono raggiungibili dal cook** (#1663, `D-262`).
 *
 * 🔴 **Cosa questo test presidia, nella forma che `D-262` ordina.** Quella voce dichiara che una build
 * che si avvia ma ripiega sulla posa di riferimento **non e' Done**, e che *«la validazione del packaged
 * FALLISCE quando una dipendenza di animazione richiesta manca»*. Questo e' quel gate.
 *
 * 🔑 **Il set NON e' scritto qui, e non e' scritto a mano da nessuna parte: si deriva dal CDO di
 * `URTUnitAnimInstance`.** E' la forma che lo spec panel del 2026-08-30 raccomandava — *«ogni
 * `TSoftObjectPtr` dichiarato da `URTUnitAnimInstance` risolve nel container: l'insieme si ricava dal
 * codice, e cresce da solo»* — ed e' l'unica che regge il vincolo che `D-262` si porta dietro: i dodici
 * montaggi `AM_<Pack>_{Attack,Hit,Death}` di #288 portano il perimetro da **8** a **20**, e una lista
 * scritta oggi andrebbe rifatta domani. Qui non c'e' niente da rifare: si aggiunge la clip al roster, e
 * questo test la pretende dal giorno dopo.
 *
 * ⚠️ **Perche' si guarda un riferimento DURO e non il `.utoc`.** Il cook segue le dipendenze: un asset
 * versionato sotto `/Game/RT` e' cotto per la riga `+DirectoriesToAlwaysCook=(Path="/Game/RT")`, e
 * trascina con se' cio' che referenzia duro. E' **gia' il meccanismo** con cui le mesh Paragon entrano
 * nel pacchetto mentre le clip no, ed e' misurato il 2026-09-03 sui quattro `.uasset`: i `BP_Unit_*`
 * nominano **una mesh ciascuno** e **zero** animazioni.
 *
 * ⛔ **E le due vie che sembravano possibili non lo sono, verificato sulla documentazione del motore e
 * non dedotto.** `DirectoriesToAlwaysCook` accetta **directory, mai singoli asset** — quindi un «set
 * minimo esplicito» non e' esprimibile li'; e l'Asset Manager governa i **Primary Asset**, mentre una
 * `UAnimSequence` e' un asset secondario, che non e' gestito direttamente e entra nel cook **solo se
 * referenziato**. ∴ il riferimento duro non e' una preferenza di stile: e' l'unica via che nomina otto
 * asset invece di una cartella.
 *
 * 🔴 **Cosa questo test NON prova**, ed e' la stessa riserva che i tre test qui sopra dichiarano: non
 * legge un `.utoc`. Prova che la clip e' **raggiungibile** dal cook, non che quel cook sia stato
 * eseguito — l'oracolo di quel fatto e' un pacchetto vero, `UnrealPak -List | findstr Animations`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRequiredAnimationClipsAreCookedTest,
	"RefactorTactics.Packaging.RequiredAnimationClipsAreCooked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRequiredAnimationClipsAreCookedTest::RunTest(const FString&)
{
	const URTUnitAnimInstance* Cdo = GetDefault<URTUnitAnimInstance>();
	if (!TestNotNull(TEXT("CDO del grafo di animazione"), Cdo))
	{
		return false;
	}

	// Il set RICHIESTO, derivato dal roster e non trascritto. Due clip per eroe oggi; il giorno che una
	// terza entra in `ClipsPerHero`, entra anche qui senza che nessuno tocchi questo file.
	TArray<FString> Richieste;
	for (const TPair<FName, FRTLocomotionClips>& Voce : Cdo->ClipsPerHero)
	{
		const TSoftObjectPtr<UAnimSequenceBase>* Due[] = { &Voce.Value.Idle, &Voce.Value.Run };
		for (const TSoftObjectPtr<UAnimSequenceBase>* Clip : Due)
		{
			const FSoftObjectPath Path = Clip->ToSoftObjectPath();
			if (Path.IsNull())
			{
				continue;
			}
			// Il PACKAGE path, non l'object path: `/.../Idle.Idle` non compare nella tabella di import di
			// chi lo referenzia — la chiave e' `/.../Idle`, la stessa lezione di `RTPackagePathOf`.
			Richieste.AddUnique(Path.GetLongPackageName());
		}
	}

	// Primo controllo anti-vacuita': un set vuoto renderebbe verde il ciclo qui sotto senza guardare
	// niente. Sono otto oggi, e l'asserzione e' «almeno una» per non impuntarsi su un numero che #288
	// fara' crescere.
	if (!TestTrue(TEXT("il roster dichiara almeno una clip richiesta"), Richieste.Num() > 0))
	{
		return false;
	}

	const FString ContentDir = FPaths::ProjectContentDir();
	const TArray<FString> Versionati = RTPackageFilesUnder(FPaths::Combine(ContentDir, TEXT("RT")));

	// Secondo controllo anti-vacuita': se non si leggesse nessun package, ogni clip risulterebbe scoperta
	// e il rosso non direbbe niente sul cook.
	if (!TestTrue(TEXT("ci sono package versionati sotto Content/RT da esaminare"), Versionati.Num() > 0))
	{
		return false;
	}

	// I byte, letti una volta sola: il ciclo sotto e' Clip x Package, e rileggere i file per ogni clip
	// moltiplicherebbe l'I/O per il numero di clip senza cambiare l'esito.
	TArray<TArray<uint8>> Byte;
	Byte.Reserve(Versionati.Num());
	for (const FString& File : Versionati)
	{
		TArray<uint8> Bytes;
		FFileHelper::LoadFileToArray(Bytes, *File);
		Byte.Add(MoveTemp(Bytes));
	}

	auto QualcunoReferenzia = [&Byte](const FString& PackagePath) -> bool
	{
		for (const TArray<uint8>& Bytes : Byte)
		{
			if (RTBytesMentionPath(Bytes, PackagePath))
			{
				return true;
			}
		}
		return false;
	};

	// 🔑 **Terzo controllo, e il piu' importante: il metodo sa trovare un riferimento VERO.**
	// Le mesh Paragon sono la famiglia SORELLA delle clip — stesso pack, stesso eroe, cartella accanto —
	// e sono referenziate duro dai `BP_Unit_*`: misurato il 2026-09-03, una mesh ciascuno e `Animations`
	// a zero. ⛔ Se la scansione non trovasse **nemmeno queste**, il rosso sulle clip significherebbe
	// «l'oracolo non sa guardare» invece di «la clip non e' raggiungibile» — la stessa confusione che il
	// registro PIE avverte per gli zeri di packaging.
	const FString MeshDiControllo =
		TEXT("/Game/FabAsset/Paragon/ParagonGadget/Characters/Heroes/Gadget/Meshes/Gadget");
	if (!TestTrue(
			TEXT("controllo positivo: la mesh Paragon di Gadget e' referenziata da un asset versionato — ")
			TEXT("se questo fallisce, il metodo di ricerca non sa guardare e il resto dell'esito non vale"),
			QualcunoReferenzia(MeshDiControllo)))
	{
		return false;
	}

	// L'asserzione per cui questo test esiste.
	TArray<FString> Scoperte;
	for (const FString& Richiesta : Richieste)
	{
		if (!QualcunoReferenzia(Richiesta))
		{
			Scoperte.Add(Richiesta);
		}
	}

	for (const FString& Scoperta : Scoperte)
	{
		AddError(FString::Printf(
			TEXT("%s e' richiesta dal roster e nessun asset versionato sotto Content/RT la referenzia: ")
			TEXT("il cook non ha nessuna dipendenza da seguire, e nel pacchetto l'unita' resta in posa ")
			TEXT("di riferimento (D-262)"), *Scoperta));
	}

	TestEqual(
		FString::Printf(TEXT("clip richieste senza un riferimento che le porti nel cook (su %d)"),
			Richieste.Num()),
		Scoperte.Num(), 0);

	return true;
}

/**
 * 🔑 **Che l'oracolo qui sopra sappia diventare rosso, e sappia diventare verde.**
 *
 * Un test che elenca «le clip scoperte» ha due modi di essere inutile, ed sono opposti: una ricerca che
 * non trova mai niente le dichiara tutte scoperte, una che trova sempre qualcosa non ne dichiara mai
 * nessuna. Qui si fabbricano **entrambi** i casi su file veri e si verifica che li distingua.
 *
 * ⛔ Le fixture vivono sotto `Saved/` e non sotto `Content/`, per la ragione che il test gemello
 * dichiara: un `.uasset` di prova dentro `Content/` rischia di finire versionato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimationClipScanDetectsAnUncoveredClipTest,
	"RefactorTactics.Packaging.AnimationClipScanDetectsAnUncoveredClip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimationClipScanDetectsAnUncoveredClipTest::RunTest(const FString&)
{
	const FString Coperta =
		TEXT("/Game/FabAsset/Paragon/ParagonFinto/Characters/Heroes/Finto/Animations/Idle");
	const FString Scoperta =
		TEXT("/Game/FabAsset/Paragon/ParagonFinto/Characters/Heroes/Finto/Animations/Run_Fwd");

	// Un finto package versionato che referenzia la PRIMA e non la seconda: e' la situazione reale, dove
	// un eroe ha l'idle agganciato e la corsa no.
	TArray<uint8> Referente;
	Referente.Add(0x00);
	const FTCHARToUTF8 Ansi(*Coperta);
	Referente.Append(reinterpret_cast<const uint8*>(Ansi.Get()), Ansi.Length());
	Referente.Add(0x00);

	TestTrue(TEXT("la clip referenziata viene trovata"), RTBytesMentionPath(Referente, Coperta));
	TestFalse(TEXT("la clip NON referenziata non viene trovata"), RTBytesMentionPath(Referente, Scoperta));

	// ⚠️ E in UTF-16LE, che e' la meta' che una ricerca scritta in fretta non vede: uno zero falso qui
	// direbbe «nessuna clip scoperta» su un progetto in cui nessuna e' agganciata.
	TArray<uint8> ReferenteWide;
	ReferenteWide.Add(0x00);
	for (const TCHAR C : Coperta)
	{
		const uint16 U = static_cast<uint16>(C);
		ReferenteWide.Add(static_cast<uint8>(U & 0xFF));
		ReferenteWide.Add(static_cast<uint8>((U >> 8) & 0xFF));
	}
	TestTrue(TEXT("la clip referenziata viene trovata anche in UTF-16LE"),
		RTBytesMentionPath(ReferenteWide, Coperta));

	// ⛔ E il package path non e' l'object path: cercare `/.../Idle.Idle` non trova niente, ed e' l'errore
	// che renderebbe il gate principale rosso su un progetto sano.
	TestFalse(TEXT("l'object path NON e' la chiave da cercare"),
		RTBytesMentionPath(Referente, Coperta + TEXT(".Idle")));

	// La prova su file veri: la scansione deve leggere cio' che si e' scritto.
	const FString Radice = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RTTests"), TEXT("Anim1663"));
	const FString Finto = FPaths::Combine(Radice, TEXT("RT"), TEXT("Fixture"), TEXT("BP_Finto.uasset"));
	IFileManager::Get().Delete(*Finto, false, true);

	if (!TestTrue(TEXT("la fixture si scrive sotto Saved/"),
			FFileHelper::SaveArrayToFile(Referente, *Finto)))
	{
		return false;
	}

	const TArray<FString> Trovati = RTPackageFilesUnder(Radice);
	TestEqual(TEXT("la scansione trova il finto package"), Trovati.Num(), 1);

	int32 Coperte = 0;
	int32 Mancanti = 0;
	for (const FString& File : Trovati)
	{
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *File))
		{
			continue;
		}
		Coperte += RTBytesMentionPath(Bytes, Coperta) ? 1 : 0;
		Mancanti += RTBytesMentionPath(Bytes, Scoperta) ? 0 : 1;
	}

	// Le due asserzioni per cui questo test esiste: **una** coperta e **una** scoperta, non zero e non due.
	TestEqual(TEXT("la scansione riconosce la clip coperta"), Coperte, 1);
	TestEqual(TEXT("la scansione riconosce la clip scoperta"), Mancanti, 1);

	IFileManager::Get().Delete(*Finto, false, true);
	IFileManager::Get().DeleteDirectory(*Radice, true, true);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
