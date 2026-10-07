# Banco Ability Lab → PIE — piano di implementazione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** un pulsante «Esegui in PIE» nel pannello Ability Lab che salva la fixture dell'abilità scelta come scenario, la rende risolvibile per Id e avvia PIE su `L_DevSandbox` con il playback vero, ripristinando le CVar a fine sessione.

**Architecture:** il modello del Lab (`FRTLabViewModel`, headless) guadagna `PrepareForPie`, che costruisce, valida, salva in `Saved/RTLab/Scenarios/` e verifica che l'Id risolva al file scritto. L'indice scenari guadagna `ScanAll` (radice versionata + radice del Lab) usata dalle sole ricerche, mentre `Scan` resta a una radice per i gate sul corpus. Un lanciatore Editor-only (`FRTLabPieLauncher`) imposta le CVar con priorità console, chiede PIE con `GlobalMapOverride` e ripristina su `EndPIE`/`CancelPIE`. Il pannello Slate cabla il pulsante e mostra lo stato.

**Tech Stack:** Unreal Engine 5.8.1, C++ (moduli `RefactorTactics` runtime e `RefactorTacticsEditor`), Automation Test framework, Slate, `IConsoleManager`, `UEditorEngine::RequestPlaySession`.

**Spec:** `docs/superpowers/specs/2026-10-07-banco-abilita-in-pie-design.md` — il piano argomenta dalla spec; chi esegue legge entrambi.

## Global Constraints

- Engine: **UE 5.8.1** in `D:/EpicGames/UE_5.8`. Nessun aggiornamento di Engine, plugin o dipendenze.
- ⛔ **Nessun `.uasset`**, nessuna modifica a TurnManager, resolver, formato scenario o `ARTGameMode`.
- ⛔ **`URTScenarioIndex::Scan` non cambia firma né semantica**: resta a una radice. Le ricerche passano a `ScanAll`.
- Le CVar si impostano e si ripristinano con **`ECVF_SetByConsole`**, mai `SetByCode`.
- Il ripristino scatta su **`FEditorDelegates::EndPIE` oppure `FEditorDelegates::CancelPIE`**, al primo dei due, poi il lanciatore si sgancia da entrambi.
- I test **non toccano `Saved/RTLab`**: la radice del Lab è sovrascrivibile per test e i file di prova vivono sotto `FPaths::AutomationTransientDir()`, rimossi con `ON_SCOPE_EXIT`.
- ⛔ **Nessun totale volatile** in commenti, documenti, issue, PR: nessun `Entries.Num()` sul corpus, nessun «N scenari» in prosa (`AGENTS.md` §14).
- Stile dei commenti: italiano, come i file toccati. Nomi dei test in italiano dove il file già li usa.
- Commit: `<type>(<scope>): <descrizione>`, chiuso da `Co-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>`.
- Branch di lavoro: `issue/<n>-banco-ability-lab-pie`, creato da `main` dopo la issue (Task 0). Il branch `docs/banco-abilita-in-pie-spec` porta spec e piano.
- **Motore uno per macchina**: prima di ogni build o run di test, `Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, Name, CommandLine` e, se un altro clone misura tempi, si aspetta (`CLAUDE.md` §10).
- Build (Editor chiuso):
  ```powershell
  & "D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -Project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -WaitMutex
  ```
- Test (il filtro dopo `RunTests`, `+` fra più filtri, `;Quit` separato):
  ```powershell
  & "D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" "-ExecCmds=Automation RunTests RefactorTactics.ScenarioIndex;Quit" -unattended -nopause -nosplash -nullrhi -NoLiveCoding "-abslog=<scratchpad>/<nome-parlante>.log"
  ```
  Un esito vale solo se il log porta `**** TEST COMPLETE`; `-abslog` fra virgolette, altrimenti il log non viene scritto.

## Review Focus

Cinque condizioni che la spec implica e che un utente incontrerà; ciascuna ha il suo test nel task che possiede il codice:

1. **Un file corrotto nella radice del Lab** non deve nascondere gli altri scenari né toccare i gate sul corpus → Task 1, `LabRootProblemsStayOutOfScan`.
2. **Due file nella radice del Lab con lo stesso Id** rendono quell'Id ambiguo: `PrepareForPie` deve rifiutare, non lasciare che il GameMode lo scopra → Task 3, `PrepareForPieRefusesAnAmbiguousId`.
3. **Un file stantio con lo stesso Id da una corsa precedente** va sovrascritto: la fixture su disco è quella dell'ultimo clic, non della prima → Task 3, `PrepareForPieOverwritesAStaleFixture`.
4. **La CVar già digitata in console** prima del clic: il banco deve scavalcarla e poi restituirla → non automatizzabile, criterio 0, 1 e 3 della seduta PIE (Task 6).
5. **PIE che non comincia** (compile error, Live Coding): le CVar devono tornare com'erano → non automatizzabile, dichiarato nella seduta PIE (Task 6) come prova facoltativa con Live Coding attivo.

---

### Task 0: Issue e branch

**Files:** nessuno nel repository. GitHub: una issue nuova sotto E21.

**Interfaces:**
- Produces: il numero `<n>` della issue, usato dal nome del branch e dai messaggi di commit.

- [ ] **Step 1: Apri la issue** (azione verso l'esterno: chiedere conferma all'autore se non già data)

Corpo in un file temporaneo nello scratchpad, poi `--body-file` (mai `--body` inline: i backtick vengono eseguiti dalla shell).

```markdown
> **Epic**: #286 (E21) · **Refs**: #2599 · #2453 · **Spec**: `docs/superpowers/specs/2026-10-07-banco-abilita-in-pie-design.md`

## Why

L'Ability Lab costruisce già la fixture di ogni abilità canonica, ma la esegue in un mondo transitorio senza grafica. Per guardare un'abilità servono uno scenario scritto a mano, la console, `rt.Test.Scenario`, Play, e ricordarsi di azzerare la CVar. Primo di quattro sotto-progetti della richiesta «associare animazioni e FX alle skill e vederle in azione»: il banco viene prima perché non dipende da niente e rende visibile il *prima*.

## Scope

1. `URTScenarioLoader::LabScenariosRoot()` → `Saved/RTLab/Scenarios`; `URTScenarioIndex::ScanAll` legge le due radici; `ResolvePath`, `ListIds`, `ListTags` passano a `ScanAll`. **`Scan` non cambia.**
2. `FRTLabViewModel::PrepareForPie`: costruisce, valida, salva, verifica che l'Id risolva al file scritto.
3. `FRTLabPieLauncher` (Editor): CVar con `ECVF_SetByConsole`, PIE con `GlobalMapOverride` su `L_DevSandbox`, ripristino su `EndPIE`/`CancelPIE`.
4. Pulsante «Esegui in PIE» e riga di stato in `SRTLabPanel`.
5. Voce PIE e seduta nel registro.

## Out of scope

Anim Browser (#2554), Gray Kit Playground (#1990), `rt.Debug.PlaybackStartPaused`, pulizia di `Saved/RTLab`, i sotto-progetti 2–4 della spec.

## DoD

- [ ] `ScenarioIndex.ScanAllSeesTheLabRoot`, `ScenarioIndex.LabRootAbsentIsNotAnError`, `ScenarioIndex.LabRootProblemsStayOutOfScan` verdi, e `ScanAllSeesTheLabRoot` **rosso per mutazione** togliendo la radice del Lab da `ScanAll`.
- [ ] `Lab.PrepareForPieWritesAResolvableScenario`, `Lab.PrepareForPieRefusesAndWritesNothing`, `Lab.PrepareForPieRefusesAnAmbiguousId`, `Lab.PrepareForPieOverwritesAStaleFixture` verdi.
- [ ] Le famiglie `RefactorTactics.ScenarioIndex`, `RefactorTactics.ScenarioWriter`, `RefactorTactics.Lab` verdi sul commit reale, log con `**** TEST COMPLETE`.
- [ ] Build `RefactorTacticsEditor` verde.
- [ ] Voce `PIE-LAB-PIE` nel registro (⏳) e seduta nel file delle sedute; il verdetto a schermo è dell'autore.
- [ ] Nessun `.uasset`; `Scan` invariata; nessun totale volatile in ciò che si pubblica.
```

```powershell
gh issue create --repo DegrassiAaron/refactor-tactics-main --title "Ability Lab: «Esegui in PIE» — la fixture dell'abilità scelta gioca nel playback vero" --body-file "<scratchpad>/issue-banco.md" --label enhancement
```

Poi rileggi dal server e conta i backtick: `gh issue view <n> --json body -q .body | grep -c '\`'` deve coincidere con il file.

- [ ] **Step 2: Crea il branch da `main`**

```bash
git fetch -q origin && git switch -c issue/<n>-banco-ability-lab-pie origin/main
git config branch.issue/<n>-banco-ability-lab-pie.parent main
```

---

### Task 1: Radice del Lab e `ScanAll` nell'indice

**Files:**
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.h:173-174` (accanto a `ScenariosRoot`)
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp:709-712`
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioIndex.h` (accanto a `Scan`)
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioIndex.cpp:124-152` (`Scan`)
- Test: `Source/RefactorTactics/Tests/RTScenarioIndexTests.cpp` (in coda, prima di `#endif`)

**Interfaces:**
- Produces:
  - `static FString URTScenarioLoader::LabScenariosRoot();` → `<Saved>/RTLab/Scenarios`, oppure l'override di test se impostato.
  - `static void URTScenarioLoader::SetLabScenariosRootOverrideForTest(const FString& Root);` — stringa vuota = nessun override.
  - `static TArray<FRTScenarioEntry> URTScenarioIndex::ScanAll(TArray<FString>& OutProblems);` — stessa forma di `Scan`, sulle due radici.

- [ ] **Step 1: Scrivi i tre test che falliscono**

In coda a `RTScenarioIndexTests.cpp`, prima di `#endif // WITH_DEV_AUTOMATION_TESTS`:

```cpp
namespace
{
	/** Una radice del Lab di prova, sotto la cartella transiente dell'automation: mai `Saved/RTLab` vero. */
	FString LabRootDiProva(const TCHAR* Nome)
	{
		return FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("RTLabIndex"), Nome);
	}

	/** Scrive un file di scenario minimo — solo header — nella cartella data. */
	bool ScriviHeaderScenario(const FString& Dir, const TCHAR* NomeFile, const TCHAR* Id, const TCHAR* Tags)
	{
		const FString Testo = FString::Printf(TEXT("{ \"scenarioId\": \"%s\", \"tags\": [%s] }"), Id, Tags);
		return FFileHelper::SaveStringToFile(Testo, *FPaths::Combine(Dir, NomeFile),
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	bool ContieneId(const TArray<FRTScenarioEntry>& Entries, const TCHAR* Id)
	{
		return Entries.ContainsByPredicate([Id](const FRTScenarioEntry& E) { return E.ScenarioId == Id; });
	}
}

/**
 * La radice del Lab si vede da `ScanAll` e NON da `Scan` (spec §2 passo 2, §3).
 *
 * 🔑 Il «non da `Scan`» e' la meta' che protegge i gate sul corpus: `ShippedScenariosAreTagged` e
 * `WriterRoundTripsShippedScenarios` passano da `Scan`, e un file stantio in `Saved/` di una macchina li
 * farebbe rossi li' e verdi altrove.
 * ✅ Validato per mutazione: togliere la radice del Lab da `ScanAll` deve far cadere il primo asserto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioIndexScanAllSeesTheLabRootTest,
	"RefactorTactics.ScenarioIndex.ScanAllSeesTheLabRoot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioIndexScanAllSeesTheLabRootTest::RunTest(const FString&)
{
	const FString Root = LabRootDiProva(TEXT("Vista"));
	IFileManager::Get().MakeDirectory(*Root, /*Tree=*/ true);
	URTScenarioLoader::SetLabScenariosRootOverrideForTest(Root);
	ON_SCOPE_EXIT
	{
		URTScenarioLoader::SetLabScenariosRootOverrideForTest(FString());
		IFileManager::Get().DeleteDirectory(*Root, false, true);
	};

	if (!TestTrue(TEXT("il file di prova si scrive"),
		ScriviHeaderScenario(Root, TEXT("AbilityLab.Prova.json"), TEXT("AbilityLab.Prova"), TEXT("\"ability-lab\""))))
	{
		return false;
	}

	TArray<FString> ProblemiAll;
	const TArray<FRTScenarioEntry> Tutti = URTScenarioIndex::ScanAll(ProblemiAll);
	TestTrue(TEXT("ScanAll vede lo scenario del Lab"), ContieneId(Tutti, TEXT("AbilityLab.Prova")));
	TestEqual(TEXT("ScanAll non segnala problemi"), ProblemiAll.Num(), 0);

	const FRTScenarioEntry* Voce = Tutti.FindByPredicate(
		[](const FRTScenarioEntry& E) { return E.ScenarioId == TEXT("AbilityLab.Prova"); });
	if (Voce)
	{
		TestTrue(TEXT("il percorso e' sotto la radice del Lab"), Voce->Path.Contains(TEXT("RTLabIndex")));
		TestTrue(TEXT("il tag e' letto"), Voce->Tags.Contains(TEXT("ability-lab")));
	}

	TArray<FString> ProblemiScan;
	const TArray<FRTScenarioEntry> Versionati = URTScenarioIndex::Scan(ProblemiScan);
	TestFalse(TEXT("Scan NON vede lo scenario del Lab"), ContieneId(Versionati, TEXT("AbilityLab.Prova")));

	// Rimosso il file, l'Id sparisce da `ScanAll`: niente cache fra una chiamata e l'altra.
	IFileManager::Get().Delete(*FPaths::Combine(Root, TEXT("AbilityLab.Prova.json")));
	TArray<FString> ProblemiDopo;
	TestFalse(TEXT("rimosso il file, ScanAll non lo vede piu'"),
		ContieneId(URTScenarioIndex::ScanAll(ProblemiDopo), TEXT("AbilityLab.Prova")));
	return true;
}

/** Radice del Lab mancante: nessun problema, nessuna voce in piu'. Le voci versionate sono le stesse di `Scan`, per Id. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioIndexLabRootAbsentTest,
	"RefactorTactics.ScenarioIndex.LabRootAbsentIsNotAnError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioIndexLabRootAbsentTest::RunTest(const FString&)
{
	const FString Root = LabRootDiProva(TEXT("CheNonEsiste"));
	IFileManager::Get().DeleteDirectory(*Root, false, true);
	URTScenarioLoader::SetLabScenariosRootOverrideForTest(Root);
	ON_SCOPE_EXIT{ URTScenarioLoader::SetLabScenariosRootOverrideForTest(FString()); };

	TArray<FString> ProblemiScan, ProblemiAll;
	const TArray<FRTScenarioEntry> DaScan = URTScenarioIndex::Scan(ProblemiScan);
	const TArray<FRTScenarioEntry> DaAll = URTScenarioIndex::ScanAll(ProblemiAll);

	TestEqual(TEXT("ScanAll ha gli stessi problemi di Scan"), ProblemiAll, ProblemiScan);
	// Confronto per Id, nei due versi, senza mai asserire un totale sul corpus.
	for (const FRTScenarioEntry& E : DaScan)
	{
		TestTrue(FString::Printf(TEXT("%s di Scan e' anche in ScanAll"), *E.ScenarioId), ContieneId(DaAll, *E.ScenarioId));
	}
	for (const FRTScenarioEntry& E : DaAll)
	{
		TestTrue(FString::Printf(TEXT("%s di ScanAll e' anche in Scan"), *E.ScenarioId), ContieneId(DaScan, *E.ScenarioId));
	}
	return true;
}

/** Un file rotto nella radice del Lab e' un problema di `ScanAll`, non di `Scan`, e non nasconde gli altri. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioIndexLabRootProblemsTest,
	"RefactorTactics.ScenarioIndex.LabRootProblemsStayOutOfScan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioIndexLabRootProblemsTest::RunTest(const FString&)
{
	const FString Root = LabRootDiProva(TEXT("Rotto"));
	IFileManager::Get().MakeDirectory(*Root, /*Tree=*/ true);
	URTScenarioLoader::SetLabScenariosRootOverrideForTest(Root);
	ON_SCOPE_EXIT
	{
		URTScenarioLoader::SetLabScenariosRootOverrideForTest(FString());
		IFileManager::Get().DeleteDirectory(*Root, false, true);
	};

	FFileHelper::SaveStringToFile(TEXT("{ questo non e' json"), *FPaths::Combine(Root, TEXT("Rotto.json")));
	ScriviHeaderScenario(Root, TEXT("AbilityLab.Sano.json"), TEXT("AbilityLab.Sano"), TEXT("\"ability-lab\""));

	TArray<FString> ProblemiScan, ProblemiAll;
	const TArray<FRTScenarioEntry> DaScan = URTScenarioIndex::Scan(ProblemiScan);
	const TArray<FRTScenarioEntry> DaAll = URTScenarioIndex::ScanAll(ProblemiAll);

	TestTrue(TEXT("ScanAll segnala il file rotto"), ProblemiAll.Num() > ProblemiScan.Num());
	TestTrue(TEXT("ScanAll vede comunque il file sano"), ContieneId(DaAll, TEXT("AbilityLab.Sano")));
	TestFalse(TEXT("Scan non vede nulla del Lab"), ContieneId(DaScan, TEXT("AbilityLab.Sano")));
	return true;
}
```

Aggiungi in testa al file, fra gli `#include`: `#include "ScenarioHarness/RTScenarioLoader.h"`, `#include "HAL/FileManager.h"`, `#include "Misc/FileHelper.h"`, `#include "Misc/Paths.h"`.

- [ ] **Step 2: Compila e verifica che fallisca per simboli mancanti**

Run: la build dei Global Constraints.
Expected: errore di compilazione su `SetLabScenariosRootOverrideForTest` e `ScanAll` non dichiarati. (Un test che non compila è il «rosso» di questo passo.)

- [ ] **Step 3: Dichiara e implementa la radice del Lab**

`RTScenarioLoader.h`, dopo `ScenariosRoot()`:

```cpp
	/**
	 * Radice degli scenari **del Lab**: `<Saved>/RTLab/Scenarios/`. Non versionata (`Saved/` e' in `.gitignore`).
	 *
	 * 🔑 La legge `URTScenarioIndex::ScanAll`, non `Scan`: i gate sul corpus misurano la sola radice
	 * versionata, e un file stantio qui non deve farli rossi su una macchina e verdi su un'altra.
	 */
	static FString LabScenariosRoot();

	/**
	 * Sovrascrive `LabScenariosRoot()` per un test. Stringa vuota = nessun override.
	 *
	 * ⚠️ Chi la imposta la azzera con `ON_SCOPE_EXIT`: `Saved/RTLab` e' condiviso con l'Editor e non e'
	 * un luogo di prova.
	 */
	static void SetLabScenariosRootOverrideForTest(const FString& Root);
```

`RTScenarioLoader.cpp`, dopo `ScenariosRoot()`:

```cpp
namespace
{
	/** L'override di test della radice del Lab. Vuoto = la radice vera. */
	FString GLabScenariosRootOverride;
}

FString URTScenarioLoader::LabScenariosRoot()
{
	if (!GLabScenariosRootOverride.IsEmpty())
	{
		return GLabScenariosRootOverride;
	}
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RTLab"), TEXT("Scenarios"));
}

void URTScenarioLoader::SetLabScenariosRootOverrideForTest(const FString& Root)
{
	GLabScenariosRootOverride = Root;
}
```

- [ ] **Step 4: Estrai `ScanRoots` e aggiungi `ScanAll`**

`RTScenarioIndex.h`, dopo la dichiarazione di `Scan`:

```cpp
	/**
	 * Come `Scan`, ma sulle **due** radici: quella versionata e quella del Lab (`URTScenarioLoader::LabScenariosRoot`).
	 *
	 * 🔑 E' la funzione delle RICERCHE — `ResolvePath`, `ListIds`, `ListTags` — cioe' del GameMode, della
	 * console e del Launcher. `Scan` resta a una radice perche' i gate sul corpus la usano come «tutto cio'
	 * che e' versionato». Una radice assente non e' un problema ne' una voce.
	 */
	static TArray<FRTScenarioEntry> ScanAll(TArray<FString>& OutProblems);

private:
	/** Il corpo comune di `Scan` e `ScanAll`: legge ricorsivamente i `.json` sotto ogni radice data. */
	static TArray<FRTScenarioEntry> ScanRoots(const TArray<FString>& Roots, TArray<FString>& OutProblems);
```

(Se la classe non ha già una sezione `private:`, questa la apre; `LoadRedirects`, `ApplyRedirects` e `NormalizeTag` devono restare pubbliche, quindi la sezione va **in coda** alla classe.)

`RTScenarioIndex.cpp`: sostituisci il corpo di `Scan` con la delega e aggiungi le due funzioni:

```cpp
TArray<FRTScenarioEntry> URTScenarioIndex::Scan(TArray<FString>& OutProblems)
{
	return ScanRoots({ URTScenarioLoader::ScenariosRoot() }, OutProblems);
}

TArray<FRTScenarioEntry> URTScenarioIndex::ScanAll(TArray<FString>& OutProblems)
{
	return ScanRoots({ URTScenarioLoader::ScenariosRoot(), URTScenarioLoader::LabScenariosRoot() }, OutProblems);
}

TArray<FRTScenarioEntry> URTScenarioIndex::ScanRoots(const TArray<FString>& Roots, TArray<FString>& OutProblems)
{
	TArray<FString> FoundFiles;
	for (const FString& Root : Roots)
	{
		// Una radice assente produce zero file e nessun errore: `FindFilesRecursive` non protesta.
		IFileManager::Get().FindFilesRecursive(FoundFiles, *Root, TEXT("*.json"),
			/*Files=*/ true, /*Directories=*/ false, /*bClearFileNames=*/ false);
	}

	TArray<TPair<FString, FString>> Loaded;
	TArray<FString> ReadProblems;
	for (const FString& File : FoundFiles)
	{
		// I file che cominciano per `_` non sono scenari: è così che la tabella di redirect vive accanto
		// agli scenari senza comparire nella tendina.
		if (FPaths::GetCleanFilename(File).StartsWith(TEXT("_")))
		{
			continue;
		}

		FString Text;
		const FString FullPath = FPaths::ConvertRelativePathToFull(File);
		if (!FFileHelper::LoadFileToString(Text, *FullPath))
		{
			ReadProblems.Add(FString::Printf(TEXT("%s: file non leggibile"), *FullPath));
			continue;
		}
		Loaded.Emplace(FullPath, MoveTemp(Text));
	}

	TArray<FRTScenarioEntry> Entries = BuildFrom(Loaded, OutProblems);
	OutProblems.Append(ReadProblems);
	return Entries;
}
```

⚠️ `FindFilesRecursive` ha un sesto parametro `bClearFileNames` che di default è `true` e svuoterebbe `FoundFiles` alla seconda radice: va passato `false`. Verificato il 2026-10-07 in `D:/EpicGames/UE_5.8/Engine/Source/Runtime/Core/Public/HAL/FileManager.h:146`.

- [ ] **Step 5: Compila e lancia la famiglia**

Run: build, poi test con filtro `RefactorTactics.ScenarioIndex`.
Expected: `ScanAllSeesTheLabRoot`, `LabRootAbsentIsNotAnError`, `LabRootProblemsStayOutOfScan` **Success**; tutti i test preesistenti della famiglia invariati (`ShippedScenariosAreTagged` compreso); log con `**** TEST COMPLETE`.

- [ ] **Step 6: Controllo di mutazione**

Togli temporaneamente `URTScenarioLoader::LabScenariosRoot()` dall'array in `ScanAll`, ricompila, rilancia `RefactorTactics.ScenarioIndex.ScanAllSeesTheLabRoot`.
Expected: **Fail** su «ScanAll vede lo scenario del Lab». Ripristina la riga, ricompila, rilancia: Success. Annota nel messaggio di commit che la mutazione è stata eseguita.

- [ ] **Step 7: Commit**

```bash
git add Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.h Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp Source/RefactorTactics/ScenarioHarness/RTScenarioIndex.h Source/RefactorTactics/ScenarioHarness/RTScenarioIndex.cpp Source/RefactorTactics/Tests/RTScenarioIndexTests.cpp
git commit -F - <<'EOF'
feat(<n>): la radice del Lab e ScanAll — l'indice legge Saved/RTLab/Scenarios solo per le ricerche

Scan resta a una radice: e' la superficie che i gate sul corpus misurano.
ScanAll la affianca alla radice del Lab, sovrascrivibile per test.
Validato per mutazione: senza la radice del Lab in ScanAll,
ScenarioIndex.ScanAllSeesTheLabRoot cade.

Co-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>
EOF
```

---

### Task 2: Le ricerche passano a `ScanAll`; messaggi e aiuto console

**Files:**
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioIndex.cpp:241` (`ResolvePath`), `:331-332` (messaggio «non trovato»), `:343` (`ListIds`), `:363` (`ListTags`)
- Modify: `Source/RefactorTactics/ScenarioHarness/RTTestConsole.cpp:138` e `:219`
- Test: `Source/RefactorTactics/Tests/RTScenarioIndexTests.cpp` (estende `ScanAllSeesTheLabRoot`)

**Interfaces:**
- Consumes: `URTScenarioIndex::ScanAll` (Task 1).
- Produces: `ResolvePath`, `ListIds`, `ListTags` vedono la radice del Lab. Nessuna firma cambia.

- [ ] **Step 1: Estendi il test con le ricerche**

In `FRTScenarioIndexScanAllSeesTheLabRootTest::RunTest`, **prima** della rimozione del file (`IFileManager::Get().Delete(...)`), aggiungi:

```cpp
	FString Errore;
	const FString Risolto = URTScenarioIndex::ResolvePath(TEXT("AbilityLab.Prova"), Errore);
	TestFalse(TEXT("ResolvePath trova lo scenario del Lab"), Risolto.IsEmpty());
	TestTrue(TEXT("e il percorso e' quello del file scritto"),
		FPaths::IsSamePath(Risolto, FPaths::ConvertRelativePathToFull(FPaths::Combine(Root, TEXT("AbilityLab.Prova.json")))));
	TestTrue(TEXT("ListIds elenca lo scenario del Lab"),
		URTScenarioIndex::ListIds(FString(), FString()).Contains(TEXT("AbilityLab.Prova")));
	TestTrue(TEXT("ListIds filtra per il suo tag"),
		URTScenarioIndex::ListIds(TEXT("ability-lab"), FString()).Contains(TEXT("AbilityLab.Prova")));
	TestTrue(TEXT("ListTags porta il tag del Lab"), URTScenarioIndex::ListTags().Contains(TEXT("ability-lab")));
```

E **dopo** la rimozione:

```cpp
	Errore.Reset();
	TestTrue(TEXT("rimosso il file, l'Id non risolve piu'"),
		URTScenarioIndex::ResolvePath(TEXT("AbilityLab.Prova"), Errore).IsEmpty());
	TestTrue(TEXT("e l'errore nomina la radice del Lab"), Errore.Contains(TEXT("RTLabIndex")));
```

- [ ] **Step 2: Compila e lancia: deve fallire**

Run: build, test `RefactorTactics.ScenarioIndex.ScanAllSeesTheLabRoot`.
Expected: **Fail** su «ResolvePath trova lo scenario del Lab» (le ricerche usano ancora `Scan`).

- [ ] **Step 3: Passa le tre ricerche a `ScanAll` e riscrivi il messaggio**

In `ResolvePath` (riga 241), `ListIds` (343) e `ListTags` (363): `Scan(Problems)` → `ScanAll(Problems)`.

Il messaggio «non trovato» (righe 331-332) diventa:

```cpp
	// Nessuna corrispondenza: il messaggio dice DOVE si è cercato, così chi legge distingue «ho sbagliato
	// l'ID» da «la cartella degli scenari non è quella che credevo». ⚠️ Nessun totale: un conteggio di
	// scenari in un messaggio invecchia da solo e si legge come corrente (AGENTS.md §14).
	OutError = FString::Printf(TEXT("scenario '%s' non trovato nell'indice (cercato sotto %s e sotto %s)"),
		*ScenarioId, *URTScenarioLoader::ScenariosRoot(), *URTScenarioLoader::LabScenariosRoot());
```

⚠️ Cerca nei test esistenti un asserto sul vecchio testo: `grep -n "non trovato nell'indice" Source/RefactorTactics/Tests/*.cpp`. Se c'è, adeguare l'asserto al nuovo testo (il senso non cambia).

- [ ] **Step 4: Aggiorna i due testi della console**

`RTTestConsole.cpp:138`:

```cpp
			Ar.Logf(TEXT("[RT-Test] nessuno scenario in %s ne' in %s"),
				*URTScenarioLoader::ScenariosRoot(), *URTScenarioLoader::LabScenariosRoot());
```

`RTTestConsole.cpp:219`:

```cpp
	TEXT("Elenca gli scenari di test: quelli versionati in Scenarios/ e quelli del Lab in Saved/RTLab/Scenarios/."),
```

- [ ] **Step 5: Compila e lancia le due famiglie**

Run: build, test `RefactorTactics.ScenarioIndex+RefactorTactics.ScenarioWriter`.
Expected: tutto Success, compresi `ShippedScenariosAreTagged` e `WriterRoundTripsShippedScenarios`; log con `**** TEST COMPLETE`.

- [ ] **Step 6: Commit**

```bash
git add Source/RefactorTactics/ScenarioHarness/RTScenarioIndex.cpp Source/RefactorTactics/ScenarioHarness/RTTestConsole.cpp Source/RefactorTactics/Tests/RTScenarioIndexTests.cpp
git commit -F - <<'EOF'
feat(<n>): ResolvePath, ListIds e ListTags vedono la radice del Lab; i messaggi nominano entrambe le radici

I gate sul corpus restano su Scan e restano verdi.

Co-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>
EOF
```

---

### Task 3: `FRTLabViewModel::PrepareForPie`

**Files:**
- Modify: `Source/RefactorTacticsEditor/Private/RTLabViewModel.h` (sezione «Esecuzione», dopo `Run`)
- Modify: `Source/RefactorTacticsEditor/Private/RTLabViewModel.cpp` (dopo `Run`)
- Test: `Source/RefactorTacticsEditor/Private/Tests/RTLabViewModelTests.cpp` (in coda, prima di `#endif`)

**Interfaces:**
- Consumes: `URTScenarioLoader::LabScenariosRoot`, `SetLabScenariosRootOverrideForTest`, `SaveToFile`, `LoadFromFile`; `URTScenarioIndex::ResolvePath` (Task 1-2); `FRTLabViewModel::BuildScenario` (esistente).
- Produces: `bool FRTLabViewModel::PrepareForPie(FString& OutScenarioId, FString& OutError);` — `true` con `OutScenarioId = AbilityLab.<AbilityId>` (o l'Id che `BuildHeroFixture` produce con filtro) e il file scritto; `false` con `OutError` e, salvo il caso ambiguo, nessun file nuovo.

- [ ] **Step 1: Scrivi i quattro test che falliscono**

In coda a `RTLabViewModelTests.cpp`, prima di `#endif`:

```cpp
namespace RTLabViewModelTestsInternal
{
	FString LabRootDiProva(const TCHAR* Nome)
	{
		return FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("RTLabPrepare"), Nome);
	}

	/** Imposta una radice del Lab di prova vuota. Chi la chiama la azzera con `ON_SCOPE_EXIT`. */
	FString ApriRadiceDiProva(const TCHAR* Nome)
	{
		const FString Root = LabRootDiProva(Nome);
		IFileManager::Get().DeleteDirectory(*Root, false, true);
		IFileManager::Get().MakeDirectory(*Root, /*Tree=*/ true);
		URTScenarioLoader::SetLabScenariosRootOverrideForTest(Root);
		return Root;
	}

	void ChiudiRadiceDiProva(const FString& Root)
	{
		URTScenarioLoader::SetLabScenariosRootOverrideForTest(FString());
		IFileManager::Get().DeleteDirectory(*Root, false, true);
	}

	int32 FileJsonIn(const FString& Root)
	{
		TArray<FString> Files;
		IFileManager::Get().FindFilesRecursive(Files, *Root, TEXT("*.json"), true, false);
		return Files.Num();
	}
}

/**
 * `PrepareForPie` scrive un file che l'indice risolve a QUEL percorso e che si rilegge uguale alla fixture
 * in memoria (spec §5.1). Il confronto campo per campo e' lo stesso di `RunWithoutHeroUsesAbilityLabFixture`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabPrepareForPieWritesAResolvableScenarioTest,
	"RefactorTactics.Lab.PrepareForPieWritesAResolvableScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabPrepareForPieWritesAResolvableScenarioTest::RunTest(const FString&)
{
	using namespace RTLabViewModelTestsInternal;
	const FString Root = ApriRadiceDiProva(TEXT("Scrive"));
	ON_SCOPE_EXIT{ ChiudiRadiceDiProva(Root); };

	FRTHeroLabEntry Eroe;
	FRTAbilityLabEntry Ability;
	if (!TestTrue(TEXT("un eroe con kit esiste"), PrimoEroeConKit(Eroe, Ability))) { return false; }

	FRTLabViewModel Modello;
	Modello.MutableSpec().Seed = 21;
	TestTrue(TEXT("l'ability si seleziona"), Modello.SelectAbility(Ability.AbilityId));

	FString Id, Errore;
	if (!TestTrue(TEXT("PrepareForPie riesce"), Modello.PrepareForPie(Id, Errore)))
	{
		AddError(Errore);
		return false;
	}
	TestEqual(TEXT("l'Id e' quello della fixture"), Id, FString::Printf(TEXT("AbilityLab.%s"), *Ability.AbilityId.ToString()));

	const FString Atteso = FPaths::ConvertRelativePathToFull(FPaths::Combine(Root, Id + TEXT(".json")));
	TestTrue(TEXT("il file esiste nella radice del Lab"), IFileManager::Get().FileExists(*Atteso));

	FString ErroreIndice;
	const FString Risolto = URTScenarioIndex::ResolvePath(Id, ErroreIndice);
	TestTrue(TEXT("l'indice risolve l'Id a QUEL file"), FPaths::IsSamePath(Risolto, Atteso));

	FRTTestScenario InMemoria, DaDisco;
	FString E1, E2;
	if (!TestTrue(TEXT("la fixture in memoria si costruisce"), Modello.BuildScenario(InMemoria, E1))) { return false; }
	if (!TestTrue(TEXT("il file si rilegge"), URTScenarioLoader::LoadFromFile(Atteso, DaDisco, E2))) { AddError(E2); return false; }
	TestTrue(TEXT("il file rilegge la stessa fixture, campo per campo"), FixtureCoincidono(*this, InMemoria, DaDisco));
	TestEqual(TEXT("e porta il tag del Lab"), DaDisco.Tags, InMemoria.Tags);
	return true;
}

/** Senza selezione: `false`, motivo scritto, NESSUN file. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabPrepareForPieRefusesAndWritesNothingTest,
	"RefactorTactics.Lab.PrepareForPieRefusesAndWritesNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabPrepareForPieRefusesAndWritesNothingTest::RunTest(const FString&)
{
	using namespace RTLabViewModelTestsInternal;
	const FString Root = ApriRadiceDiProva(TEXT("Rifiuta"));
	ON_SCOPE_EXIT{ ChiudiRadiceDiProva(Root); };

	FRTLabViewModel Modello; // nessuna ability selezionata
	FString Id, Errore;
	TestFalse(TEXT("senza selezione PrepareForPie rifiuta"), Modello.PrepareForPie(Id, Errore));
	TestFalse(TEXT("e dice perche'"), Errore.IsEmpty());
	TestTrue(TEXT("e l'Id resta vuoto"), Id.IsEmpty());
	TestEqual(TEXT("e non scrive nessun file"), FileJsonIn(Root), 0);
	return true;
}

/** Due file con lo stesso Id nella radice del Lab: l'Id e' ambiguo e `PrepareForPie` lo dice, invece di lasciarlo al GameMode. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabPrepareForPieRefusesAnAmbiguousIdTest,
	"RefactorTactics.Lab.PrepareForPieRefusesAnAmbiguousId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabPrepareForPieRefusesAnAmbiguousIdTest::RunTest(const FString&)
{
	using namespace RTLabViewModelTestsInternal;
	const FString Root = ApriRadiceDiProva(TEXT("Ambiguo"));
	ON_SCOPE_EXIT{ ChiudiRadiceDiProva(Root); };

	FRTHeroLabEntry Eroe;
	FRTAbilityLabEntry Ability;
	if (!TestTrue(TEXT("un eroe con kit esiste"), PrimoEroeConKit(Eroe, Ability))) { return false; }

	// Un secondo file, con nome diverso, che dichiara lo stesso Id della fixture.
	const FString IdFixture = FString::Printf(TEXT("AbilityLab.%s"), *Ability.AbilityId.ToString());
	FFileHelper::SaveStringToFile(
		FString::Printf(TEXT("{ \"scenarioId\": \"%s\", \"tags\": [\"ability-lab\"] }"), *IdFixture),
		*FPaths::Combine(Root, TEXT("Doppione.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

	FRTLabViewModel Modello;
	TestTrue(TEXT("l'ability si seleziona"), Modello.SelectAbility(Ability.AbilityId));

	FString Id, Errore;
	TestFalse(TEXT("con un doppione l'Id e' ambiguo e PrepareForPie rifiuta"), Modello.PrepareForPie(Id, Errore));
	TestTrue(TEXT("e il motivo dice che e' ambiguo"), Errore.Contains(TEXT("ambigu")));
	return true;
}

/** Una fixture stantia con lo stesso Id viene sovrascritta: su disco c'e' l'ultimo clic. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabPrepareForPieOverwritesAStaleFixtureTest,
	"RefactorTactics.Lab.PrepareForPieOverwritesAStaleFixture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabPrepareForPieOverwritesAStaleFixtureTest::RunTest(const FString&)
{
	using namespace RTLabViewModelTestsInternal;
	const FString Root = ApriRadiceDiProva(TEXT("Stantio"));
	ON_SCOPE_EXIT{ ChiudiRadiceDiProva(Root); };

	FRTHeroLabEntry Eroe;
	FRTAbilityLabEntry Ability;
	if (!TestTrue(TEXT("un eroe con kit esiste"), PrimoEroeConKit(Eroe, Ability))) { return false; }

	FRTLabViewModel Modello;
	TestTrue(TEXT("l'ability si seleziona"), Modello.SelectAbility(Ability.AbilityId));

	FString Id, Errore;
	Modello.MutableSpec().Seed = 7;
	if (!TestTrue(TEXT("prima corsa"), Modello.PrepareForPie(Id, Errore))) { AddError(Errore); return false; }
	Modello.MutableSpec().Seed = 11;
	if (!TestTrue(TEXT("seconda corsa, stesso Id"), Modello.PrepareForPie(Id, Errore))) { AddError(Errore); return false; }

	FRTTestScenario DaDisco;
	FString E;
	const FString Percorso = FPaths::Combine(Root, Id + TEXT(".json"));
	if (!TestTrue(TEXT("il file si rilegge"), URTScenarioLoader::LoadFromFile(Percorso, DaDisco, E))) { AddError(E); return false; }
	TestEqual(TEXT("su disco c'e' il seed dell'ultimo clic"), DaDisco.Seed, 11);
	TestEqual(TEXT("e c'e' un solo file"), FileJsonIn(Root), 1);
	return true;
}
```

In testa al file aggiungi gli include: `#include "ScenarioHarness/RTScenarioIndex.h"`, `#include "ScenarioHarness/RTScenarioLoader.h"`, `#include "HAL/FileManager.h"`, `#include "Misc/FileHelper.h"`, `#include "Misc/Paths.h"`.

⚠️ `FixtureCoincidono` non confronta `Expect` né `Tags`: per questo il test aggiunge l'asserto sui `Tags`. Non estendere l'helper, che altri test usano con il significato attuale.

- [ ] **Step 2: Compila: deve fallire per simbolo mancante**

Run: build. Expected: errore su `PrepareForPie` non dichiarata.

- [ ] **Step 3: Dichiara e implementa `PrepareForPie`**

`RTLabViewModel.h`, dopo `Run`:

```cpp
	/**
	 * Prepara la fixture per il PIE: la costruisce, la valida, la **salva** in
	 * `URTScenarioLoader::LabScenariosRoot()/<ScenarioId>.json` e verifica che `URTScenarioIndex::ResolvePath`
	 * risolva `ScenarioId` a QUEL file.
	 *
	 * ⛔ Fail closed: senza selezione, fixture invalida o scrittura fallita → `false`, nessun file nuovo.
	 * Con un Id ambiguo — lo stesso `scenarioId` dichiarato anche da un altro file — il file viene scritto
	 * ma la funzione ritorna `false` col motivo dell'indice: meglio qui che a schermo dal GameMode.
	 *
	 * ⚠️ Non sa nulla di PIE ne' di `GEditor`: l'avvio e' del lanciatore, questo e' il pezzo misurabile.
	 */
	bool PrepareForPie(FString& OutScenarioId, FString& OutError);
```

`RTLabViewModel.cpp`, dopo `Run` (aggiungi `#include "ScenarioHarness/RTScenarioIndex.h"`, `#include "ScenarioHarness/RTScenarioLoader.h"`, `#include "HAL/FileManager.h"`, `#include "Misc/Paths.h"`):

```cpp
bool FRTLabViewModel::PrepareForPie(FString& OutScenarioId, FString& OutError)
{
	OutScenarioId.Reset();
	OutError.Reset();

	FRTTestScenario Scenario;
	if (!BuildScenario(Scenario, OutError))
	{
		return false;
	}

	const FString Root = URTScenarioLoader::LabScenariosRoot();
	IFileManager::Get().MakeDirectory(*Root, /*Tree=*/ true);
	const FString Percorso = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(Root, Scenario.ScenarioId + TEXT(".json")));

	// `SaveToFile` valida prima di toccare il disco: una fixture invalida non lascia un file a meta'.
	if (!URTScenarioLoader::SaveToFile(Scenario, Percorso, OutError))
	{
		return false;
	}

	// 🔑 Lanciabile = l'indice risolve l'Id a QUESTO file. Un doppione altrove rende l'Id ambiguo, e il
	// GameMode lo rifiuterebbe a schermo senza che il pannello potesse dirlo prima.
	FString ErroreIndice;
	const FString Risolto = URTScenarioIndex::ResolvePath(Scenario.ScenarioId, ErroreIndice);
	if (Risolto.IsEmpty())
	{
		OutError = FString::Printf(TEXT("fixture scritta in '%s' ma non lanciabile: %s"), *Percorso, *ErroreIndice);
		return false;
	}
	if (!FPaths::IsSamePath(Risolto, Percorso))
	{
		OutError = FString::Printf(TEXT("'%s' risolve a '%s', non al file appena scritto '%s'"),
			*Scenario.ScenarioId, *Risolto, *Percorso);
		return false;
	}

	OutScenarioId = Scenario.ScenarioId;
	return true;
}
```

- [ ] **Step 4: Compila e lancia la famiglia**

Run: build, test `RefactorTactics.Lab`.
Expected: i quattro test nuovi Success, i preesistenti invariati; `**** TEST COMPLETE`.

- [ ] **Step 5: Commit**

```bash
git add Source/RefactorTacticsEditor/Private/RTLabViewModel.h Source/RefactorTacticsEditor/Private/RTLabViewModel.cpp Source/RefactorTacticsEditor/Private/Tests/RTLabViewModelTests.cpp
git commit -F - <<'EOF'
feat(<n>): FRTLabViewModel::PrepareForPie — la fixture si salva nella radice del Lab e si verifica lanciabile

Costruisce, valida, scrive e pretende che ResolvePath risolva l'Id a quel
file: un doppione altrove e' un rifiuto, non una sorpresa a schermo.

Co-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>
EOF
```

---

### Task 4: `FRTLabPieLauncher` (solo Editor)

**Files:**
- Create: `Source/RefactorTacticsEditor/Private/RTLabPieLauncher.h`
- Create: `Source/RefactorTacticsEditor/Private/RTLabPieLauncher.cpp`

**Interfaces:**
- Consumes: `GEditor` (`UEditorEngine::PlayWorld`, `RequestPlaySession`), `IConsoleManager`, `FEditorDelegates::EndPIE` / `CancelPIE`, `FRequestPlaySessionParams::GlobalMapOverride`.
- Produces: `static bool FRTLabPieLauncher::Launch(const FString& ScenarioId, FString& OutError);` e `static FString FRTLabPieLauncher::DevSandboxMapPath();`.

Nessun automation test: Slate e PIE su un Editor vivo non sono misurabili headless (spec §5.2). Il passo di verifica è la build.

- [ ] **Step 1: Scrivi l'header**

```cpp
// Il lanciatore PIE del Lab: la sola parte del banco che tocca `GEditor` (spec §3).
//
// 🔑 **E' separato dal modello per la stessa ragione per cui il modello e' separato dal widget**: cio' che
// qui succede non lo vede nessun automation test, quindi deve essere il meno possibile. Costruire la
// fixture, salvarla e verificarla e' di `FRTLabViewModel::PrepareForPie`; qui si chiede PIE e si
// ripristinano due CVar.
//
// ⛔ **Nessuna regola di gioco**: lo scenario lo risolve il GameMode dalla console, come per ogni Id.

#pragma once

#include "CoreMinimal.h"

class FRTLabPieLauncher
{
public:
	/**
	 * Imposta `rt.Test.Scenario` su `ScenarioId` e `rt.Debug.PlaybackControls` su `1` — entrambe con
	 * `ECVF_SetByConsole`, perche' un valore gia' digitato in console vince su un `Set` a priorita'
	 * inferiore — e chiede PIE su `L_DevSandbox` tramite `GlobalMapOverride`, senza toccare il livello
	 * aperto nell'Editor.
	 *
	 * I valori precedenti vengono catturati e **ripristinati** al primo fra `EndPIE` e `CancelPIE`.
	 *
	 * ⛔ Rifiuta, senza toccare niente, se: PIE e' gia' in corso; una delle due CVar non si trova; un
	 * lancio precedente e' ancora in attesa di ripristino.
	 */
	static bool Launch(const FString& ScenarioId, FString& OutError);

	/** `/Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox`: la mappa ospite di ogni scenario. */
	static FString DevSandboxMapPath();
};
```

- [ ] **Step 2: Scrivi l'implementazione**

```cpp
#include "RTLabPieLauncher.h"

#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "HAL/IConsoleManager.h"
#include "PlayInEditorDataTypes.h"

namespace
{
	/** Cio' che serve per rimettere le cose com'erano: i due valori e i due handle. */
	struct FRTLabPieRestore
	{
		FString ScenarioPrima;
		FString PlaybackControlsPrima;
		FDelegateHandle SuEndPIE;
		FDelegateHandle SuCancelPIE;
	};

	TUniquePtr<FRTLabPieRestore> GRipristino;

	IConsoleVariable* TrovaCVar(const TCHAR* Nome, FString& OutError)
	{
		IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Nome);
		if (!Var)
		{
			OutError = FString::Printf(TEXT("la console variable '%s' non esiste in questo binario"), Nome);
		}
		return Var;
	}

	/** Riapplica i valori catturati, con la stessa priorita' con cui erano stati scavalcati, e si sgancia. */
	void Ripristina()
	{
		if (!GRipristino)
		{
			return;
		}
		TUniquePtr<FRTLabPieRestore> R = MoveTemp(GRipristino);
		FEditorDelegates::EndPIE.Remove(R->SuEndPIE);
		FEditorDelegates::CancelPIE.Remove(R->SuCancelPIE);

		FString Ignorato;
		if (IConsoleVariable* Scenario = TrovaCVar(TEXT("rt.Test.Scenario"), Ignorato))
		{
			Scenario->Set(*R->ScenarioPrima, ECVF_SetByConsole);
		}
		if (IConsoleVariable* Controls = TrovaCVar(TEXT("rt.Debug.PlaybackControls"), Ignorato))
		{
			Controls->Set(*R->PlaybackControlsPrima, ECVF_SetByConsole);
		}
	}
}

FString FRTLabPieLauncher::DevSandboxMapPath()
{
	return TEXT("/Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox");
}

bool FRTLabPieLauncher::Launch(const FString& ScenarioId, FString& OutError)
{
	OutError.Reset();

	if (ScenarioId.IsEmpty())
	{
		OutError = TEXT("nessuno ScenarioId da lanciare");
		return false;
	}
	if (!GEditor)
	{
		OutError = TEXT("GEditor assente: il lanciatore vive solo nell'Editor");
		return false;
	}
	if (GEditor->PlayWorld != nullptr)
	{
		OutError = TEXT("PIE in corso: fermalo prima di lanciare il banco");
		return false;
	}
	if (GRipristino)
	{
		OutError = TEXT("un lancio precedente aspetta ancora il ripristino delle CVar");
		return false;
	}

	// Entrambe le CVar PRIMA di toccarne una: se la seconda manca, la prima non va cambiata.
	IConsoleVariable* Scenario = TrovaCVar(TEXT("rt.Test.Scenario"), OutError);
	if (!Scenario) { return false; }
	IConsoleVariable* Controls = TrovaCVar(TEXT("rt.Debug.PlaybackControls"), OutError);
	if (!Controls) { return false; }

	GRipristino = MakeUnique<FRTLabPieRestore>();
	GRipristino->ScenarioPrima = Scenario->GetString();
	GRipristino->PlaybackControlsPrima = Controls->GetString();

	// 🔑 `ECVF_SetByConsole`: un valore digitato in console ha quella priorita', e un `Set` a priorita'
	// inferiore verrebbe ignorato con un warning — il banco giocherebbe lo scenario sbagliato credendo di
	// aver scelto.
	Scenario->Set(*ScenarioId, ECVF_SetByConsole);
	Controls->Set(TEXT("1"), ECVF_SetByConsole);

	// Al primo dei due che scatta si ripristina e ci si sgancia da entrambi. `CancelPIE` copre il PIE che
	// non comincia: `RequestPlaySession` e' differita, ed `EndPIE` da sola scatterebbe solo per una
	// sessione partita.
	GRipristino->SuEndPIE = FEditorDelegates::EndPIE.AddLambda([](const bool) { Ripristina(); });
	GRipristino->SuCancelPIE = FEditorDelegates::CancelPIE.AddLambda([]() { Ripristina(); });

	FRequestPlaySessionParams Params;
	Params.GlobalMapOverride = DevSandboxMapPath();
	GEditor->RequestPlaySession(Params);
	return true;
}
```

⚠️ Verifica prima di compilare, negli header dell'Engine: `FEditorDelegates::CancelPIE` è `FSimpleMulticastDelegate` (lambda senza parametri), `EndPIE` è `FOnPIEEvent` (lambda `const bool`); `IConsoleVariable::Set(T, EConsoleVariableFlags)` è il template a riga ~700 di `IConsoleManager.h`; `UEditorEngine::PlayWorld` è `TObjectPtr<UWorld>`. Gli include esatti: `Editor.h` per `GEditor`/`FEditorDelegates`, `Editor/EditorEngine.h` per `UEditorEngine`, `PlayInEditorDataTypes.h` per `FRequestPlaySessionParams`. Se un include non risolve, cercalo con `grep -rl "struct FRequestPlaySessionParams" D:/EpicGames/UE_5.8/Engine/Source/Editor/UnrealEd/Public`.

- [ ] **Step 3: Compila**

Run: build. Expected: verde, zero warning nuovi nel modulo Editor (confronta con la build del Task 3).

- [ ] **Step 4: Commit**

```bash
git add Source/RefactorTacticsEditor/Private/RTLabPieLauncher.h Source/RefactorTacticsEditor/Private/RTLabPieLauncher.cpp
git commit -F - <<'EOF'
feat(<n>): FRTLabPieLauncher — CVar a priorita' console, PIE su L_DevSandbox via GlobalMapOverride, ripristino su EndPIE o CancelPIE

Solo Editor, nessun automation test: e' il pezzo che la seduta PIE giudica.

Co-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>
EOF
```

---

### Task 5: Il pulsante «Esegui in PIE» e la riga di stato

**Files:**
- Modify: `Source/RefactorTacticsEditor/Private/SRTLabPanel.h` (dopo `OnEsegui`; nuovo campo)
- Modify: `Source/RefactorTacticsEditor/Private/SRTLabPanel.cpp` (`CostruisciEsecuzione`, `TestoEsito`, nuovo handler)

**Interfaces:**
- Consumes: `FRTLabViewModel::PrepareForPie` (Task 3), `FRTLabPieLauncher::Launch` (Task 4).

- [ ] **Step 1: Header**

In `SRTLabPanel.h`, dopo `FReply OnEsegui();`:

```cpp
	/**
	 * Salva la fixture e chiede PIE su `L_DevSandbox` (spec §2). Il modello prepara, il lanciatore lancia:
	 * qui si riferisce il gesto e si mostra l'esito.
	 */
	FReply OnEseguiInPie();
```

Dopo `FString UltimoErrore;`:

```cpp
	/** L'Id lanciato in PIE dall'ultimo clic, per dire a chi guarda quale riga di log cercare. */
	FString UltimoIdLanciato;
```

- [ ] **Step 2: Il pulsante**

In `CostruisciEsecuzione()` aggiungi uno slot dopo quello di «Esegui» (aggiungi `#include "RTLabPieLauncher.h"` in testa al `.cpp`):

```cpp
	+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
	[
		SNew(SButton)
		.Text(LOCTEXT("EseguiInPie", "Esegui in PIE"))
		.ToolTipText(LOCTEXT("EseguiInPieTip",
			"Salva la fixture in Saved/RTLab/Scenarios, imposta rt.Test.Scenario e avvia PIE su L_DevSandbox. "
			"Le CVar tornano com'erano a fine PIE."))
		.OnClicked(this, &SRTLabPanel::OnEseguiInPie)
	]
```

- [ ] **Step 3: L'handler**

Dopo `OnEsegui()`:

```cpp
FReply SRTLabPanel::OnEseguiInPie()
{
	UltimoErrore.Reset();
	UltimoIdLanciato.Reset();

	FString Id, Errore;
	if (!Modello.PrepareForPie(Id, Errore))
	{
		UltimoErrore = Errore;
		return FReply::Handled();
	}
	if (!FRTLabPieLauncher::Launch(Id, Errore))
	{
		UltimoErrore = Errore;
		return FReply::Handled();
	}

	UltimoIdLanciato = Id;
	return FReply::Handled();
}
```

- [ ] **Step 4: La riga di stato**

In `TestoEsito()`, dopo il ramo che mostra `UltimoErrore` e **prima** di leggere `Modello.LastRun()`:

```cpp
	if (!UltimoIdLanciato.IsEmpty())
	{
		// La riga di log la scrive `FRTScenarioCoordinator` e COMINCIA cosi'; seguono turni e pausa.
		return FText::FromString(FString::Printf(
			TEXT("PIE richiesto per %s su L_DevSandbox.\nNel log cerca: [RT-Test] AUTO-RUN %s (da: console rt.Test.Scenario)\n"
				 "A fine PIE le CVar tornano com'erano."),
			*UltimoIdLanciato, *UltimoIdLanciato));
	}
```

- [ ] **Step 5: Compila e lancia tutte le famiglie toccate**

Run: build; test `RefactorTactics.ScenarioIndex+RefactorTactics.ScenarioWriter+RefactorTactics.Lab`.
Expected: build verde; tutte Success; `**** TEST COMPLETE`.

- [ ] **Step 6: Commit**

```bash
git add Source/RefactorTacticsEditor/Private/SRTLabPanel.h Source/RefactorTacticsEditor/Private/SRTLabPanel.cpp
git commit -F - <<'EOF'
feat(<n>): «Esegui in PIE» nel pannello Ability Lab, con l'Id lanciato e la riga di log da cercare

Co-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>
EOF
```

---

### Task 6: Registro PIE e seduta

**Files:**
- Modify: `docs/technical/test-manuali-pie.md` — una riga nella tabella della sezione `### Scenario Test Harness` (riga ~1547), **in coda** alla tabella
- Modify: `docs/roadmap/editor-sessions.yaml` — una seduta **subito prima di `^not_schedulable:`** (riga ~5068), non in fondo al file

**Interfaces:** nessuna di codice. Regole: l'esito atteso vive **solo** in `test-manuali-pie.md`; il file delle sedute cita gli **ID** delle voci, mai l'esito; nessun totale volatile; il file yaml è CRLF.

- [ ] **Step 1: La voce nel registro**

Trova la fine della tabella di `### Scenario Test Harness` (la prima riga vuota dopo la sua ultima riga `| **PIE-...`) e aggiungi **prima** di quella riga vuota, come unica riga di tabella con le stesse cinque colonne delle righe sopra:

```markdown
| **PIE-LAB-PIE** | Il pulsante «Esegui in PIE» dell'Ability Lab lancia la fixture dell'abilità scelta nel playback vero, e a fine PIE le CVar tornano com'erano ([#<n>](https://github.com/DegrassiAaron/refactor-tactics-main/issues/<n>)) | Editor aperto su una mappa **qualsiasi diversa da `L_DevSandbox`** (per provare `GlobalMapOverride`); pannello Ability Lab aperto; un'abilità con bersaglio selezionata, es. `Hero.Branth.ImpactShot`. **Prima** del clic, in console: `rt.Test.Scenario Core.PhaseOrder`, così la CVar porta già un valore a priorità console | **(0)** Il clic non produce errore nel pannello e la riga di stato mostra l'Id `AbilityLab.<AbilityId>`. **(1)** Il log porta una riga che comincia con `[RT-Test] AUTO-RUN AbilityLab.<AbilityId> (da: console rt.Test.Scenario)` — e **non** `Core.PhaseOrder`: è la prova che `ECVF_SetByConsole` ha scavalcato il valore digitato. **(2)** Il PIE gira su `L_DevSandbox` e il playback mostra l'abilità sulle due unità della fixture, con i controlli di playback accesi; il livello aperto nell'Editor non è cambiato. **(3)** Fermato il PIE, `rt.Test.Scenario` digitata da sola in console risponde `Core.PhaseOrder`, e un Play **senza** passare dal Lab avvia quello scenario, non il banco. **(4, facoltativo)** Con Live Coding attivo e una modifica C++ non compilata, il clic produce un PIE che non parte: `rt.Test.Scenario` deve comunque valere `Core.PhaseOrder` subito dopo. ⚠️ Il **(3)** dice *ripristina ciò che c'era*, non *azzera*. Coperto headless: `Lab.PrepareForPie*` e `ScenarioIndex.ScanAll*`; qui resta ciò che nessun test vede — il lancio, la mappa, il ripristino | ⏳ da eseguire — scritta il 2026-10-07 insieme al codice di [#<n>](https://github.com/DegrassiAaron/refactor-tactics-main/issues/<n>); il verdetto è dell'autore |
```

⚠️ Nessuna pipe `|` dentro le celle (romperebbe il conteggio delle voci); l'ultima cella è lo **stato** e comincia con un solo glifo.

- [ ] **Step 2: La seduta**

In `editor-sessions.yaml`, inserisci **subito prima** della riga `not_schedulable:`:

```yaml
  - id: U67
    title: Il banco Ability Lab → PIE — il pulsante «Esegui in PIE» e il ripristino delle CVar
    block: 6
    critical: false
    execution_lane: pie
    produces: >-
      il verdetto su `PIE-LAB-PIE`; nessun asset
    artifacts: []
    unblocked_by: []
    shares_setup_with: []
    verifies: [PIE-LAB-PIE]
    issues: [<n>]
    done_when: >-
      la voce `PIE-LAB-PIE` ha un verdetto nel registro, dato a schermo da una persona dopo il merge del codice
    notes: |
      Scritta il 2026-10-07 dal piano `docs/superpowers/plans/2026-10-07-banco-abilita-in-pie.md`.
      L'esito atteso vive in `test-manuali-pie.md`; qui solo l'allestimento: Editor su una mappa diversa da
      `L_DevSandbox`, pannello Ability Lab, un'abilità con bersaglio, e `rt.Test.Scenario Core.PhaseOrder`
      digitata PRIMA del clic. Il criterio (4) richiede Live Coding attivo ed è facoltativo.
```

Verifica che la seduta chiuda `sessions` e che la coda `not_schedulable` non cambi:

```powershell
python -c "import yaml; d=yaml.safe_load(open('docs/roadmap/editor-sessions.yaml',encoding='utf-8')); print([x['id'] for x in d['sessions']][-3:], len(d['not_schedulable']))"
```

Expected: l'elenco termina con `U67`; il secondo numero è lo stesso di prima dell'inserimento (misuralo prima). Controlla i line ending: `file docs/roadmap/editor-sessions.yaml` deve dire CRLF come prima.

- [ ] **Step 3: Il radar dei documenti**

```powershell
node tools/radar/doc-coherence.ts --check
```

Expected: nessun rosso nuovo. Se `tools/radar/` ha altri controlli elencati nel suo README, lanciali tutti (la memoria del progetto dice che sono venti, non due).

- [ ] **Step 4: Commit**

```bash
git add docs/technical/test-manuali-pie.md docs/roadmap/editor-sessions.yaml
git commit -F - <<'EOF'
docs(<n>): voce PIE-LAB-PIE e seduta U67 per il banco Ability Lab -> PIE

Co-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>
EOF
```

---

### Task 7: Chiusura — PR, review, merge, issue

**Files:** nessuno. GitHub.

- [ ] **Step 1: Suite completa sul commit finale**

Run: test con filtro `RefactorTactics` (tutta la suite), log in scratchpad.
Expected: `**** TEST COMPLETE`, zero Fail. Annota il SHA.

- [ ] **Step 2: Push e PR verso il parent**

Il parent è `main` (`git config branch.$(git branch --show-current).parent`). Corpo in un file, `--body-file`, con: cosa, i gate eseguiti (`PASS` con il nome del log), i gate `NOT RUN` (PIE: voce `PIE-LAB-PIE`, seduta `U67`), nessun totale volatile, chiusura con `🤖 Generated with [Claude Code](https://claude.com/claude-code)`.

```bash
git push -u origin issue/<n>-banco-ability-lab-pie
gh pr create --repo DegrassiAaron/refactor-tactics-main --base main --title "Ability Lab: «Esegui in PIE» (#<n>)" --body-file "<scratchpad>/pr-banco.md"
```

- [ ] **Step 3: Code review**

`/code-review` sulla PR; accogli i findings con lo skill `receiving-code-review` (verifica, non adesione).

- [ ] **Step 4: Merge, pulizia, issue**

Merge della PR nel parent; `git branch -D issue/<n>-banco-ability-lab-pie`; `git remote prune origin`. Aggiorna la issue: spunta i DoD misurati, lascia aperto il DoD della voce PIE finché l'autore non la giudica, oppure chiudi la issue se l'autore preferisce che il verdetto PIE viva nel solo registro — chiedilo nel commento di chiusura.

- [ ] **Step 5: Spec e piano**

Nel branch `docs/banco-abilita-in-pie-spec`, aggiorna lo **Statuto** della spec: «implementato il <data> sul branch `issue/<n>-…`, PR #<pr>», e apri la PR anche per quel branch.
