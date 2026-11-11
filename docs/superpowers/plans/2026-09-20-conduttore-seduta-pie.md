# Conduttore di seduta PIE — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** una seduta PIE diventa «apri una volta, guarda, premi un tasto per voce, esci», con un file macchina-leggibile che dichiara cosa è stato giudicato, su quale albero e con quale esito.

**Architecture:** `URTPieSessionSubsystem` (GameInstance) possiede playlist, passo corrente e verdetti, e dipende da **due porte** (`Launch`/`TearDown`) invece che dal coordinator. `ARTGameMode` resta chi avvia e installa le porte; `FRTScenarioCoordinator` resta il *come* e guadagna due sole aggiunte. L'overlay è un `UUserWidget` scritto in C++, senza `.uasset`.

**Tech Stack:** Unreal Engine 5.8, C++, `IMPLEMENT_SIMPLE_AUTOMATION_TEST` con flag `EditorContext | EngineFilter`, JSON via `FJsonObject`/`TJsonWriter`.

**Spec:** [`docs/superpowers/specs/2026-09-20-conduttore-seduta-pie-design.md`](../specs/2026-09-20-conduttore-seduta-pie-design.md) · issue [#3208](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3208)

## Global Constraints

- **Niente `.uasset`.** Nessun task di questo piano tocca il Content. Se un task sembra chiederlo, è sbagliato il task.
- **Ogni gate dev'essere rosso senza aprire l'Editor**, con la sola eccezione dichiarata del Task 9.
- **Namespace dei test:** `RefactorTactics.PieSession.<Nome>`; per il gate di catalogo `RefactorTactics.PieSession.Catalog<Nome>`.
- **Nomi unici per translation unit:** la unity build condivide la TU, quindi gli helper in `namespace {}` dei file di test portano un prefisso proprio (`PieSession…`), come già fa `MakeAutoRunWorld` in `RTScenarioAutoRunTests.cpp`.
- **Comando della suite** (aggiusta il percorso del progetto):
  ```powershell
  & "D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
    "D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" `
    "-ExecCmds=Automation RunTests RefactorTactics.PieSession;Quit" `
    -unattended -nopause -nosplash -nullrhi -NoLiveCoding -log
  ```
- **Build:** `D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat RefactorTacticsEditor Win64 Development -Project="D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" -WaitMutex`
- **Il motore è uno.** Prima di compilare o lanciare la suite: `Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, CommandLine`. Se un'altra sessione lo tiene, si aspetta o si dichiara `NOT RUN` col nome del clone.
- **`verdict` ha cinque valori** — `PASS`, `FAIL`, `NotJudgeable`, `Blocked`, `NotRun` — e `reason` è obbligatorio per i tre che non vengono da una persona.

## File Structure

| File | Responsabilità |
|---|---|
| `Source/RefactorTactics/ScenarioHarness/RTTestScenario.h` *(modifica)* | il campo `Verifies` nel modello dello scenario |
| `Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp` *(modifica)* | leggerlo dal JSON |
| `Source/RefactorTactics/ScenarioHarness/RTScenarioWriter.cpp` *(modifica)* | riscriverlo, così il round-trip non lo cancella |
| `Source/RefactorTactics/PieSession/RTPieSessionTypes.h` *(nuovo)* | enum del verdetto, `FRTPieSessionStep`, `FRTPieSessionPorts` |
| `Source/RefactorTactics/PieSession/RTPieSessionSubsystem.h/.cpp` *(nuovi)* | la conduzione: stati, coda, verdetti |
| `Source/RefactorTactics/PieSession/RTPieSessionWriter.h/.cpp` *(nuovi)* | il file di seduta e il commit letto da `.git/HEAD` |
| `Source/RefactorTactics/PieSession/RTPieSessionPlaylist.h/.cpp` *(nuovi)* | comporre la coda da un selettore, e dire cosa resta fuori |
| `Source/RefactorTactics/PieSession/RTPieSessionConsole.cpp` *(nuovo)* | `rt.Pie.Session`, `rt.Pie.Verdict`, `rt.Pie.Session.Abort` |
| `Source/RefactorTactics/PieSession/RTPieVerdictOverlay.h/.cpp` *(nuovi)* | overlay C++ e cattura dei tasti |
| `Source/RefactorTactics/ScenarioHarness/RTScenarioCoordinator.h/.cpp` *(modifica)* | `OnScenarioFinished` + `TearDown()` pubblico |
| `Source/RefactorTactics/RTGameMode.h/.cpp` *(modifica)* | installa le porte, inoltra il delegate, quarta sorgente in `ResolveScenarioToRun` |
| `Source/RefactorTactics/Tests/RTPieSession*Tests.cpp` *(nuovi)* | i gate |
| `docs/technical/runbooks/guida-conduttore-seduta-pie.md` *(nuovo)* | come si conduce una seduta |

---

### Task 1: `verifies` nel formato scenario, andata e ritorno

**Files:**
- Modify: `Source/RefactorTactics/ScenarioHarness/RTTestScenario.h` (testata di `FRTTestScenario`, dopo `Tags`, ~riga 903)
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp:1936` (dove legge `tags`)
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioWriter.cpp:532` (dove scrive `tags`; nessun header da toccare — la serializzazione si raggiunge da `URTScenarioLoader::SaveToFile`)
- Test: `Source/RefactorTactics/Tests/RTPieSessionFormatTests.cpp`

**Interfaces:**
- Consumes: niente
- Produces: `FRTTestScenario::Verifies` — `TArray<FString>`, gli id delle voci PIE come scritti nel file

- [ ] **Step 1: Write the failing test**

```cpp
// Source/RefactorTactics/Tests/RTPieSessionFormatTests.cpp
#include "Misc/AutomationTest.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Prefisso proprio: la unity build condivide la translation unit.
	const TCHAR* PieSessionMinimalJson = TEXT(R"({
		"scenarioId": "Spec.PieSession.Fixture",
		"version": 1,
		"mapRadius": 3,
		"verifies": ["PIE-VIS-SIGHTWALL", "PIE-V01-LOG"],
		"units": [],
		"turns": []
	})");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionVerifiesLoadsTest,
	"RefactorTactics.PieSession.VerifiesSurvivesLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionVerifiesLoadsTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	FString Error;
	TestTrue(TEXT("il JSON si carica"),
		URTScenarioLoader::LoadFromString(PieSessionMinimalJson, Scenario, Error));
	TestEqual(TEXT("due voci dichiarate"), Scenario.Verifies.Num(), 2);
	TestEqual(TEXT("la prima e' quella scritta"), Scenario.Verifies[0], TEXT("PIE-VIS-SIGHTWALL"));
	return true;
}

// 🔑 Il round-trip e' il gate che conta: il campo `Tags` fu letto e non riscritto, e un
// `load -> save` cancellava i tag di ogni scenario che ne aveva. Stessa trappola, stesso posto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionVerifiesSurvivesRoundTripTest,
	"RefactorTactics.PieSession.VerifiesSurvivesRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionVerifiesSurvivesRoundTripTest::RunTest(const FString&)
{
	FRTTestScenario Andata;
	FString Error;
	URTScenarioLoader::LoadFromString(PieSessionMinimalJson, Andata, Error);

	// ⚠️ Non esiste `URTScenarioWriter::ToJson`: `RTScenarioWriter.cpp` non ha header, e la
	// serializzazione passa da `URTScenarioLoader::SaveToFile` e da nessun altro posto
	// (`RTScenarioAuthoring.h:21`). Il round-trip si fa quindi su un file temporaneo.
	const FString Temp = FPaths::Combine(FPaths::ProjectSavedDir(),
		TEXT("RTPieSessionTests"), TEXT("roundtrip.json"));
	TestTrue(TEXT("si salva"), URTScenarioLoader::SaveToFile(Andata, Temp, Error));

	FRTTestScenario Ritorno;
	TestTrue(TEXT("e si ricarica"), URTScenarioLoader::LoadFromFile(Temp, Ritorno, Error));
	TestEqual(TEXT("le voci sopravvivono al salvataggio"), Ritorno.Verifies, Andata.Verifies);
	IFileManager::Get().Delete(*Temp);
	return true;
}

#endif
```

🔑 **Le firme, verificate il 2026-09-20 e non supposte**: `URTScenarioLoader::LoadFromString(JsonText, OutScenario, OutError)`, `::LoadFromFile(FilePath, …)`, `::SaveToFile(Scenario, FilePath, OutError)` — tutte `static bool` (`RTScenarioLoader.h:95,98,161`). ⛔ `RTScenarioWriter` **non ha un header** e non espone `ToJson`: è l'implementazione che `SaveToFile` usa, e si modifica lì dentro.

- [ ] **Step 2: Run test to verify it fails**

Build, poi la suite filtrata su `RefactorTactics.PieSession`.
Expected: FAIL di compilazione — `Verifies` non è un membro di `FRTTestScenario`.

- [ ] **Step 3: Il campo nel modello**

```cpp
// RTTestScenario.h, subito dopo Tags
/**
 * ID delle voci PIE che questo allestimento permette di giudicare — `PIE-V01-LOG`.
 *
 * ⛔ **Solo gli ID, mai l'esito atteso.** E' la stessa regola che `editor-sessions.yaml` esiste per far
 * rispettare: l'esito atteso vive in `docs/technical/test-manuali-pie.md`, che ne resta l'unico owner.
 * Una domanda scritta qui sarebbe una terza copia derivata di un testo che gia' ne ha due divergenti.
 */
UPROPERTY()
TArray<FString> Verifies;
```

- [ ] **Step 4: La lettura**

```cpp
// RTScenarioLoader.cpp, accanto al blocco che legge `tags`
const TArray<TSharedPtr<FJsonValue>>* VerifiesJson = nullptr;
if (Root->TryGetArrayField(TEXT("verifies"), VerifiesJson))
{
    for (const TSharedPtr<FJsonValue>& Value : *VerifiesJson)
    {
        FString Voce;
        if (Value.IsValid() && Value->TryGetString(Voce) && !Voce.IsEmpty())
        {
            OutScenario.Verifies.Add(Voce);
        }
    }
}
```

- [ ] **Step 5: La scrittura**

```cpp
// RTScenarioWriter.cpp, accanto al blocco che scrive `tags`
if (Scenario.Verifies.Num() > 0)
{
    Writer->WriteArrayStart(TEXT("verifies"));
    for (const FString& Voce : Scenario.Verifies) { Writer->WriteValue(Voce); }
    Writer->WriteArrayEnd();
}
```

- [ ] **Step 6: Run tests to verify they pass**

Expected: PASS entrambi.

- [ ] **Step 7: Commit**

```bash
git add Source/RefactorTactics/ScenarioHarness/RTTestScenario.h \
        Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp \
        Source/RefactorTactics/ScenarioHarness/RTScenarioWriter.cpp \
        Source/RefactorTactics/Tests/RTPieSessionFormatTests.cpp
git commit -m "feat(3208): lo scenario dichiara quali voci PIE permette di giudicare"
```

---

### Task 2: il gate di catalogo — nessun id inventato

**Files:**
- Create: `Source/RefactorTactics/Tests/RTPieSessionCatalogTests.cpp`

**Interfaces:**
- Consumes: `FRTTestScenario::Verifies` (Task 1)
- Produces: niente codice di produzione — è un gate

- [ ] **Step 1: Write the failing test**

```cpp
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "ScenarioHarness/RTScenarioLoader.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	TSet<FString> PieSessionVociDelRegistro()
	{
		TSet<FString> Voci;
		FString Registro;
		const FString Path = FPaths::ProjectDir() / TEXT("docs/technical/test-manuali-pie.md");
		if (!FFileHelper::LoadFileToString(Registro, *Path)) { return Voci; }

		TArray<FString> Righe;
		Registro.ParseIntoArrayLines(Righe, /*CullEmpty=*/ false);
		for (const FString& Riga : Righe)
		{
			// La riga-voce apre con `| **PIE-...**`; qualunque altra menzione e' prosa.
			if (!Riga.StartsWith(TEXT("| **PIE-"))) { continue; }
			const int32 Inizio = 4;
			int32 Fine = INDEX_NONE;
			if (Riga.FindLastChar('*', Fine)) {}
			const int32 Chiusura = Riga.Find(TEXT("**"), ESearchCase::CaseSensitive,
				ESearchDir::FromStart, Inizio + 2);
			if (Chiusura == INDEX_NONE) { continue; }
			Voci.Add(Riga.Mid(Inizio + 2, Chiusura - Inizio - 2).TrimStartAndEnd());
		}
		return Voci;
	}

	TArray<FString> PieSessionFileScenario()
	{
		TArray<FString> Files;
		IFileManager::Get().FindFilesRecursive(Files, *(FPaths::ProjectDir() / TEXT("Scenarios")),
			TEXT("*.json"), /*Files=*/ true, /*Directories=*/ false);
		return Files;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionCatalogNoInventedIdTest,
	"RefactorTactics.PieSession.CatalogDeclaresOnlyRealItems",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionCatalogNoInventedIdTest::RunTest(const FString&)
{
	const TSet<FString> Registro = PieSessionVociDelRegistro();
	TestTrue(TEXT("il registro si legge e porta delle voci"), Registro.Num() > 0);

	for (const FString& File : PieSessionFileScenario())
	{
		FString Json;
		if (!FFileHelper::LoadFileToString(Json, *File)) { continue; }
		FRTTestScenario Scenario;
		FString Error;
		if (!URTScenarioLoader::LoadFromString(Json, Scenario, Error)) { continue; }

		for (const FString& Voce : Scenario.Verifies)
		{
			TestTrue(FString::Printf(TEXT("%s dichiara %s, che nel registro non esiste"),
				*Scenario.ScenarioId, *Voce), Registro.Contains(Voce));
		}
	}
	return true;
}

// Una voce dichiarata da due scenari non e' vietata, ma la coda non puo' sceglierne uno in silenzio:
// il conduttore si ferma e chiede. Questo gate rende visibile quando accade.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionCatalogAmbiguityIsVisibleTest,
	"RefactorTactics.PieSession.CatalogAmbiguityIsVisible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionCatalogAmbiguityIsVisibleTest::RunTest(const FString&)
{
	TMap<FString, TArray<FString>> Dichiaranti;
	for (const FString& File : PieSessionFileScenario())
	{
		FString Json;
		if (!FFileHelper::LoadFileToString(Json, *File)) { continue; }
		FRTTestScenario Scenario;
		FString Error;
		if (!URTScenarioLoader::LoadFromString(Json, Scenario, Error)) { continue; }
		for (const FString& Voce : Scenario.Verifies)
		{
			Dichiaranti.FindOrAdd(Voce).Add(Scenario.ScenarioId);
		}
	}

	for (const TPair<FString, TArray<FString>>& Coppia : Dichiaranti)
	{
		if (Coppia.Value.Num() > 1)
		{
			AddInfo(FString::Printf(TEXT("%s e' dichiarata da %s"),
				*Coppia.Key, *FString::Join(Coppia.Value, TEXT(", "))));
		}
	}
	return true;
}

#endif
```

- [ ] **Step 2: Run to verify it passes trivially, then prove it can fail**

Expected: PASS (nessuno scenario dichiara ancora `verifies`).

⚠️ **Un gate che passa su un insieme vuoto non ha ancora dimostrato niente.** Prima di commitare, aggiungi a mano `"verifies": ["PIE-NON-ESISTE"]` a `Scenarios/Visual/Core/PhaseOrder.json`, rilancia, **verifica che il test sia ROSSO**, poi togli la riga. Senza questo passaggio il gate è cieco e non lo sai.

- [ ] **Step 3: Popola due scenari veri**

Scegli due scenari il cui allestimento copre una voce esistente e dichiaralo. Verifica prima che l'id esista:

```bash
grep -c '^| \*\*PIE-VIS-SIGHTWALL\*\*' docs/technical/test-manuali-pie.md   # deve dare 1
```

- [ ] **Step 4: Run tests**

Expected: PASS con i due scenari popolati.

- [ ] **Step 5: Commit**

```bash
git add Source/RefactorTactics/Tests/RTPieSessionCatalogTests.cpp Scenarios/
git commit -m "test(3208): ogni voce dichiarata da uno scenario esiste nel registro"
```

---

### Task 3: gli stati del conduttore, con le porte finte

**Files:**
- Create: `Source/RefactorTactics/PieSession/RTPieSessionTypes.h`
- Create: `Source/RefactorTactics/PieSession/RTPieSessionSubsystem.h`
- Create: `Source/RefactorTactics/PieSession/RTPieSessionSubsystem.cpp`
- Test: `Source/RefactorTactics/Tests/RTPieSessionConductionTests.cpp`

**Interfaces:**
- Consumes: `ERTScenarioStart` (`ScenarioHarness/RTScenarioCoordinator.h`), `FRTTestResult`
- Produces:
  - `enum class ERTPieVerdict : uint8 { Pending, Pass, Fail, NotJudgeable, Blocked, NotRun }`
  - `struct FRTPieSessionStep { FString PieItem; int32 Criterion = INDEX_NONE; FString ScenarioId; FString RunId; FString ReportDir; FString MachineOutcome; ERTPieVerdict Verdict = ERTPieVerdict::Pending; FString Reason; FDateTime At; }`
  - `struct FRTPieSessionPorts { TFunction<ERTScenarioStart(const FString&)> Launch; TFunction<void()> TearDown; }`
  - `URTPieSessionSubsystem::Begin(TArray<FRTPieSessionStep>, FRTPieSessionPorts)`, `::OnScenarioFinished(const FRTTestResult&)`, `::SubmitVerdict(ERTPieVerdict, const FString& Reason)`, `::Abort()`, `::CurrentStep() const`, `::State() const`, `::Steps() const`

- [ ] **Step 1: Write the failing test**

```cpp
#include "Misc/AutomationTest.h"
#include "PieSession/RTPieSessionSubsystem.h"
#include "ScenarioHarness/RTTestResult.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	struct FPieSessionBanco
	{
		TArray<FString> Lanciati;
		int32 TearDowns = 0;
		ERTScenarioStart Esito = ERTScenarioStart::Started;

		FRTPieSessionPorts Porte()
		{
			FRTPieSessionPorts P;
			P.Launch = [this](const FString& Id) { Lanciati.Add(Id); return Esito; };
			P.TearDown = [this]() { ++TearDowns; };
			return P;
		}
	};

	FRTPieSessionStep PieSessionPasso(const TCHAR* Voce, const TCHAR* Scenario)
	{
		FRTPieSessionStep S;
		S.PieItem = Voce;
		S.ScenarioId = Scenario;
		return S;
	}

	FRTTestResult PieSessionEsito(const TCHAR* ScenarioId, bool bPassed)
	{
		FRTTestResult R;
		R.ScenarioId = ScenarioId;
		FRTAssertionResult A;
		A.bPassed = bPassed;
		A.Description = TEXT("finta");
		R.Assertions.Add(A);
		return R;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionAdvancesInOrderTest,
	"RefactorTactics.PieSession.AdvancesInOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionAdvancesInOrderTest::RunTest(const FString&)
{
	URTPieSessionSubsystem* Conduttore = NewObject<URTPieSessionSubsystem>();
	FPieSessionBanco Banco;

	Conduttore->Begin({ PieSessionPasso(TEXT("PIE-A"), TEXT("Scen.A")),
	                    PieSessionPasso(TEXT("PIE-B"), TEXT("Scen.B")) }, Banco.Porte());

	TestEqual(TEXT("parte il primo"), Banco.Lanciati.Num(), 1);
	TestEqual(TEXT("ed e' Scen.A"), Banco.Lanciati[0], TEXT("Scen.A"));

	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.A"), true));
	TestTrue(TEXT("ora aspetta un verdetto"),
		Conduttore->State() == ERTPieSessionState::AwaitingVerdict);
	TestEqual(TEXT("nessun secondo lancio prima del verdetto"), Banco.Lanciati.Num(), 1);

	Conduttore->SubmitVerdict(ERTPieVerdict::Pass, FString());
	TestEqual(TEXT("il teardown precede il passo dopo"), Banco.TearDowns, 1);
	TestEqual(TEXT("e parte il secondo"), Banco.Lanciati.Num(), 2);
	TestEqual(TEXT("ed e' Scen.B"), Banco.Lanciati[1], TEXT("Scen.B"));

	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.B"), true));
	Conduttore->SubmitVerdict(ERTPieVerdict::Fail, FString());
	TestTrue(TEXT("coda vuota -> Done"), Conduttore->State() == ERTPieSessionState::Done);
	TestTrue(TEXT("i due verdetti sono quelli dati"),
		Conduttore->Steps()[0].Verdict == ERTPieVerdict::Pass);
	TestTrue(TEXT("e il secondo e' rosso"),
		Conduttore->Steps()[1].Verdict == ERTPieVerdict::Fail);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionNotLoadableIsNotAskedTest,
	"RefactorTactics.PieSession.NotLoadableIsNotAsked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionNotLoadableIsNotAskedTest::RunTest(const FString&)
{
	URTPieSessionSubsystem* Conduttore = NewObject<URTPieSessionSubsystem>();
	FPieSessionBanco Banco;
	Banco.Esito = ERTScenarioStart::NotLoadable;

	Conduttore->Begin({ PieSessionPasso(TEXT("PIE-A"), TEXT("Scen.Assente")),
	                    PieSessionPasso(TEXT("PIE-B"), TEXT("Scen.B")) }, Banco.Porte());

	// Non si chiede un giudizio su una scena che non si e' allestita: il passo si chiude da solo.
	TestTrue(TEXT("chiuso NotJudgeable"),
		Conduttore->Steps()[0].Verdict == ERTPieVerdict::NotJudgeable);
	TestTrue(TEXT("con un motivo scritto"), !Conduttore->Steps()[0].Reason.IsEmpty());
	TestEqual(TEXT("e si prosegue"), Banco.Lanciati.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionAbortKeepsWhatWasGivenTest,
	"RefactorTactics.PieSession.AbortKeepsWhatWasGiven",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionAbortKeepsWhatWasGivenTest::RunTest(const FString&)
{
	URTPieSessionSubsystem* Conduttore = NewObject<URTPieSessionSubsystem>();
	FPieSessionBanco Banco;
	Conduttore->Begin({ PieSessionPasso(TEXT("PIE-A"), TEXT("Scen.A")),
	                    PieSessionPasso(TEXT("PIE-B"), TEXT("Scen.B")) }, Banco.Porte());

	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.A"), true));
	Conduttore->SubmitVerdict(ERTPieVerdict::Pass, FString());
	Conduttore->Abort();

	TestTrue(TEXT("il verdetto dato resta"), Conduttore->Steps()[0].Verdict == ERTPieVerdict::Pass);
	TestTrue(TEXT("il resto e' NotRun"), Conduttore->Steps()[1].Verdict == ERTPieVerdict::NotRun);
	TestTrue(TEXT("e la seduta e' finita"), Conduttore->State() == ERTPieSessionState::Done);
	return true;
}

#endif
```

- [ ] **Step 2: Run to verify it fails**

Expected: FAIL di compilazione — `RTPieSessionSubsystem.h` non esiste.

- [ ] **Step 3: I tipi**

```cpp
// Source/RefactorTactics/PieSession/RTPieSessionTypes.h
#pragma once
#include "CoreMinimal.h"
#include "ScenarioHarness/RTScenarioCoordinator.h" // ERTScenarioStart
#include "RTPieSessionTypes.generated.h"

/** Il verdetto di un passo. ⛔ Nessuno dei cinque significa «verde perche' il gate era verde». */
UENUM()
enum class ERTPieVerdict : uint8
{
	Pending,       // non ancora giudicato
	Pass,          // lo da' una persona
	Fail,          // lo da' una persona
	NotJudgeable,  // persona, oppure conduttore quando la voce non era allestibile
	Blocked,       // solo conduttore: la scena non e' mai partita
	NotRun         // solo conduttore: la seduta e' finita prima
};

UENUM()
enum class ERTPieSessionState : uint8 { Idle, Playing, AwaitingVerdict, Done };

USTRUCT()
struct FRTPieSessionStep
{
	GENERATED_BODY()

	UPROPERTY() FString PieItem;
	/** `INDEX_NONE` = la voce si giudica intera; altrimenti il numero del criterio. */
	UPROPERTY() int32 Criterion = INDEX_NONE;
	UPROPERTY() FString ScenarioId;
	UPROPERTY() FString RunId;
	UPROPERTY() FString ReportDir;
	/** Verbale della run, non uno stato: una run e' immutabile e datata. */
	UPROPERTY() FString MachineOutcome;
	UPROPERTY() ERTPieVerdict Verdict = ERTPieVerdict::Pending;
	/** Obbligatorio per NotJudgeable automatico, Blocked e NotRun. */
	UPROPERTY() FString Reason;
	UPROPERTY() FDateTime At;
};

/**
 * Le due porte. 🔴 **Non sostituirle con un puntatore al GameMode**: con un puntatore i casi limite
 * tornano a richiedere scenari veri, e il gate della non-deduzione diventa inscrivibile.
 */
struct FRTPieSessionPorts
{
	TFunction<ERTScenarioStart(const FString& ScenarioId)> Launch;
	TFunction<void()> TearDown;
};
```

- [ ] **Step 4: Il subsystem**

```cpp
// RTPieSessionSubsystem.h
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PieSession/RTPieSessionTypes.h"
#include "RTPieSessionSubsystem.generated.h"

struct FRTTestResult;

UCLASS()
class REFACTORTACTICS_API URTPieSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void Begin(TArray<FRTPieSessionStep> InSteps, FRTPieSessionPorts InPorts);
	void OnScenarioFinished(const FRTTestResult& Result);
	void SubmitVerdict(ERTPieVerdict Verdict, const FString& Reason);
	void Abort();

	ERTPieSessionState State() const { return SessionState; }
	const TArray<FRTPieSessionStep>& Steps() const { return SessionSteps; }
	const FRTPieSessionStep* CurrentStep() const
	{
		return SessionSteps.IsValidIndex(Cursor) ? &SessionSteps[Cursor] : nullptr;
	}
	/** Vero mentre una seduta e' in corso: e' la quarta sorgente di `ResolveScenarioToRun`. */
	bool IsConducting() const { return SessionState != ERTPieSessionState::Idle
	                                && SessionState != ERTPieSessionState::Done; }

private:
	void LaunchCurrent();

	TArray<FRTPieSessionStep> SessionSteps;
	FRTPieSessionPorts Ports;
	int32 Cursor = INDEX_NONE;
	ERTPieSessionState SessionState = ERTPieSessionState::Idle;
};
```

```cpp
// RTPieSessionSubsystem.cpp
#include "PieSession/RTPieSessionSubsystem.h"
#include "ScenarioHarness/RTTestResult.h"

void URTPieSessionSubsystem::Begin(TArray<FRTPieSessionStep> InSteps, FRTPieSessionPorts InPorts)
{
	SessionSteps = MoveTemp(InSteps);
	Ports = MoveTemp(InPorts);
	Cursor = 0;
	SessionState = ERTPieSessionState::Playing;
	LaunchCurrent();
}

void URTPieSessionSubsystem::LaunchCurrent()
{
	while (SessionSteps.IsValidIndex(Cursor))
	{
		SessionState = ERTPieSessionState::Playing;
		const ERTScenarioStart Start = Ports.Launch
			? Ports.Launch(SessionSteps[Cursor].ScenarioId)
			: ERTScenarioStart::NotLoadable;

		if (Start == ERTScenarioStart::Started) { return; }

		// Non si chiede un giudizio su una scena che non si e' allestita.
		SessionSteps[Cursor].Verdict = ERTPieVerdict::NotJudgeable;
		SessionSteps[Cursor].Reason = TEXT("scenario non caricabile");
		SessionSteps[Cursor].At = FDateTime::UtcNow();
		++Cursor;
	}
	SessionState = ERTPieSessionState::Done;
}

void URTPieSessionSubsystem::OnScenarioFinished(const FRTTestResult& Result)
{
	if (!SessionSteps.IsValidIndex(Cursor)) { return; }

	FRTPieSessionStep& Step = SessionSteps[Cursor];
	Step.MachineOutcome = FString::Printf(TEXT("%s %d/%d"), *Result.OutcomeString(),
		Result.PassedCount(), Result.Assertions.Num());

	if (!Result.ErrorMessage.IsEmpty())
	{
		// La sessione e' esistita ma non ha mai giocato: chiedere un giudizio sarebbe chiedere
		// di guardare una scena mai partita.
		Step.Verdict = ERTPieVerdict::Blocked;
		Step.Reason = Result.ErrorMessage;
		Step.At = FDateTime::UtcNow();
		if (Ports.TearDown) { Ports.TearDown(); }
		++Cursor;
		LaunchCurrent();
		return;
	}

	SessionState = ERTPieSessionState::AwaitingVerdict;
}

void URTPieSessionSubsystem::SubmitVerdict(ERTPieVerdict Verdict, const FString& Reason)
{
	if (SessionState != ERTPieSessionState::AwaitingVerdict) { return; }

	FRTPieSessionStep& Step = SessionSteps[Cursor];
	Step.Verdict = Verdict;
	Step.Reason = Reason;
	Step.At = FDateTime::UtcNow();

	if (Ports.TearDown) { Ports.TearDown(); }
	++Cursor;
	LaunchCurrent();
}

void URTPieSessionSubsystem::Abort()
{
	for (int32 I = FMath::Max(Cursor, 0); I < SessionSteps.Num(); ++I)
	{
		if (SessionSteps[I].Verdict == ERTPieVerdict::Pending)
		{
			SessionSteps[I].Verdict = ERTPieVerdict::NotRun;
			SessionSteps[I].Reason = TEXT("seduta interrotta");
		}
	}
	if (Ports.TearDown) { Ports.TearDown(); }
	SessionState = ERTPieSessionState::Done;
}
```

⚠️ `FRTTestResult::ErrorMessage`, `OutcomeString()` e `PassedCount()` esistono già (`RTTestResult.h`): verifica le firme prima di usarle, non fidarti di questo blocco.

- [ ] **Step 5: Run tests**

Expected: PASS tutti e tre.

- [ ] **Step 6: Commit**

```bash
git add Source/RefactorTactics/PieSession/ Source/RefactorTactics/Tests/RTPieSessionConductionTests.cpp
git commit -m "feat(3208): il conduttore avanza la coda e chiude da solo cio' che non si allestisce"
```

---

### Task 4: il file di seduta, e il gate che vieta la deduzione

**Files:**
- Create: `Source/RefactorTactics/PieSession/RTPieSessionWriter.h/.cpp`
- Modify: `Source/RefactorTactics/PieSession/RTPieSessionSubsystem.cpp` (scrive alla fine)
- Test: `Source/RefactorTactics/Tests/RTPieSessionWriterTests.cpp`

**Interfaces:**
- Consumes: `FRTPieSessionStep`, `ERTPieVerdict`
- Produces: `URTPieSessionWriter::ToJson(const FString& SessionId, const FString& Commit, const TArray<FRTPieSessionStep>&)`, `::Write(...) -> bool`, `::ReadHeadCommit() -> FString` (vuoto se non leggibile)

- [ ] **Step 1: Write the failing test**

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionVerdictIsNotDeducedTest,
	"RefactorTactics.PieSession.VerdictIsNotDeducedFromMachineOutcome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionVerdictIsNotDeducedTest::RunTest(const FString&)
{
	// La coppia che il progetto produce davvero: expect rosse e verdetto umano verde.
	// Se qualcuno facesse scendere `verdict` da `machineOutcome`, questo test diventa ROSSO.
	FRTPieSessionStep Step;
	Step.PieItem = TEXT("PIE-A");
	Step.ScenarioId = TEXT("Scen.A");
	Step.MachineOutcome = TEXT("FAIL 3/7");
	Step.Verdict = ERTPieVerdict::Pass;

	const FString Json = URTPieSessionWriter::ToJson(TEXT("2026-09-20-000000"), TEXT("deadbeef"), { Step });

	TestTrue(TEXT("porta l'esito macchina"), Json.Contains(TEXT("\"machineOutcome\": \"FAIL 3/7\"")));
	TestTrue(TEXT("e il verdetto umano, opposto"), Json.Contains(TEXT("\"verdict\": \"PASS\"")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionUnknownCommitIsDeclaredTest,
	"RefactorTactics.PieSession.UnknownCommitIsDeclaredNotOmitted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionUnknownCommitIsDeclaredTest::RunTest(const FString&)
{
	FRTPieSessionStep Step;
	Step.PieItem = TEXT("PIE-A");
	Step.Verdict = ERTPieVerdict::Pass;

	const FString Json = URTPieSessionWriter::ToJson(TEXT("2026-09-20-000000"), FString(), { Step });

	// Un campo assente si legge come «non pertinente»; `null` si legge come «non lo so», che e' il fatto.
	TestTrue(TEXT("il commit ignoto e' null, non assente"), Json.Contains(TEXT("\"commit\": null")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionReasonRequiredTest,
	"RefactorTactics.PieSession.NonHumanVerdictCarriesAReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionReasonRequiredTest::RunTest(const FString&)
{
	FRTPieSessionStep Step;
	Step.PieItem = TEXT("PIE-A");
	Step.Verdict = ERTPieVerdict::Blocked;   // senza Reason: e' l'errore che il writer deve rendere visibile
	const FString Json = URTPieSessionWriter::ToJson(TEXT("s"), TEXT("c"), { Step });
	TestTrue(TEXT("un Blocked senza motivo si dichiara tale"),
		Json.Contains(TEXT("\"reason\": \"(motivo non registrato)\"")));
	return true;
}
```

- [ ] **Step 2: Run to verify it fails**

Expected: FAIL di compilazione — `URTPieSessionWriter` non esiste.

- [ ] **Step 3: Il writer**

```cpp
// RTPieSessionWriter.cpp — nucleo
FString URTPieSessionWriter::ToJson(const FString& SessionId, const FString& Commit,
	const TArray<FRTPieSessionStep>& Steps)
{
	FString Out;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Out);

	Writer->WriteObjectStart();
	Writer->WriteValue(TEXT("sessionId"), SessionId);
	if (Commit.IsEmpty()) { Writer->WriteNull(TEXT("commit")); }
	else { Writer->WriteValue(TEXT("commit"), Commit); }
	Writer->WriteValue(TEXT("buildVersion"), FApp::GetBuildVersion());

	Writer->WriteArrayStart(TEXT("steps"));
	for (const FRTPieSessionStep& S : Steps)
	{
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("pieItem"), S.PieItem);
		if (S.Criterion == INDEX_NONE) { Writer->WriteNull(TEXT("criterion")); }
		else { Writer->WriteValue(TEXT("criterion"), S.Criterion); }
		Writer->WriteValue(TEXT("scenarioId"), S.ScenarioId);
		Writer->WriteValue(TEXT("runId"), S.RunId);
		Writer->WriteValue(TEXT("reportDir"), S.ReportDir);
		Writer->WriteValue(TEXT("machineOutcome"), S.MachineOutcome);
		Writer->WriteValue(TEXT("verdict"), VerdictToString(S.Verdict));
		const bool bRichiedeMotivo = S.Verdict == ERTPieVerdict::Blocked
			|| S.Verdict == ERTPieVerdict::NotRun
			|| S.Verdict == ERTPieVerdict::NotJudgeable;
		if (bRichiedeMotivo)
		{
			Writer->WriteValue(TEXT("reason"),
				S.Reason.IsEmpty() ? TEXT("(motivo non registrato)") : *S.Reason);
		}
		else if (S.Reason.IsEmpty()) { Writer->WriteNull(TEXT("reason")); }
		else { Writer->WriteValue(TEXT("reason"), S.Reason); }
		Writer->WriteValue(TEXT("at"), S.At.ToIso8601());
		Writer->WriteObjectEnd();
	}
	Writer->WriteArrayEnd();
	Writer->WriteObjectEnd();
	Writer->Close();
	return Out;
}
```

`ReadHeadCommit()` legge `FPaths::ProjectDir() / ".git/HEAD"`; se la prima riga è `ref: <path>` legge `.git/<path>` e ne prende la prima parola, altrimenti è già uno SHA. Fallisce in silenzio restituendo `FString()` — il `null` nel file è la dichiarazione.

- [ ] **Step 4: Il subsystem scrive a fine seduta**

In `Abort()` e nel ramo `Done` di `LaunchCurrent()`: `URTPieSessionWriter::Write(...)` in
`FPaths::ProjectSavedDir() / TEXT("RTPieSessions") / SessionId / TEXT("session.json")`, e una riga di log con il percorso e, se il commit è vuoto, **l'avviso** «commit non dichiarato — questa seduta non è attribuibile a un albero».

- [ ] **Step 5: Run tests**

Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add Source/RefactorTactics/PieSession/ Source/RefactorTactics/Tests/RTPieSessionWriterTests.cpp
git commit -m "feat(3208): il file di seduta tiene separati esito macchina e verdetto umano"
```

---

### Task 5: comporre la coda da un selettore

**Files:**
- Create: `Source/RefactorTactics/PieSession/RTPieSessionPlaylist.h/.cpp`
- Test: `Source/RefactorTactics/Tests/RTPieSessionPlaylistTests.cpp`

**Interfaces:**
- Consumes: `FRTTestScenario::Verifies`, `URTScenarioIndex`
- Produces: `struct FRTPieSessionPlan { TArray<FRTPieSessionStep> Steps; TArray<FString> Excluded; TArray<FString> Ambiguous; }` e `URTPieSessionPlaylist::Compose(const FString& Selector) -> FRTPieSessionPlan`

- [ ] **Step 1: Write the failing test**

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionPrefixTakesEveryItemTest,
	"RefactorTactics.PieSession.PrefixTakesEveryItemOfTheScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionPrefixTakesEveryItemTest::RunTest(const FString&)
{
	// Uno scenario che dichiara piu' voci produce piu' passi: e' il guadagno principale.
	const FRTPieSessionPlan Plan = URTPieSessionPlaylist::Compose(TEXT("Visual.Perception.*"));
	TestTrue(TEXT("almeno un passo"), Plan.Steps.Num() >= 1);
	for (const FRTPieSessionStep& S : Plan.Steps)
	{
		TestTrue(TEXT("ogni passo nomina la propria voce"), !S.PieItem.IsEmpty());
		TestTrue(TEXT("e il proprio scenario"), S.ScenarioId.StartsWith(TEXT("Visual.Perception.")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionUnknownItemIsExcludedNotSilentTest,
	"RefactorTactics.PieSession.UnknownItemIsExcludedWithAReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionUnknownItemIsExcludedNotSilentTest::RunTest(const FString&)
{
	const FRTPieSessionPlan Plan = URTPieSessionPlaylist::Compose(TEXT("PIE-NON-DICHIARATA-DA-NESSUNO"));
	TestEqual(TEXT("nessun passo"), Plan.Steps.Num(), 0);
	TestEqual(TEXT("ma compare fra le escluse"), Plan.Excluded.Num(), 1);
	return true;
}
```

- [ ] **Step 2: Run to verify it fails** — Expected: FAIL di compilazione.

- [ ] **Step 3: Implementa `Compose`**

Un selettore che finisce con `*` o contiene un punto senza il prefisso `PIE-` è un **prefisso di scenario**: si interrogano gli scenari dell'indice, e ogni voce del loro `verifies` diventa un passo, nell'ordine in cui il file la dichiara. Altrimenti è un elenco di **id di voce** separati da virgola: per ciascuno si cerca lo scenario che lo dichiara; zero dichiaranti → `Excluded`; più d'uno → `Ambiguous` e `Steps` resta vuoto, perché una coda che sceglie in silenzio è peggio di nessuna coda.

- [ ] **Step 4: Run tests** — Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git commit -am "feat(3208): la coda si compone da un selettore, e dichiara cosa lascia fuori"
```

---

### Task 6: i tre comandi console

**Files:**
- Create: `Source/RefactorTactics/PieSession/RTPieSessionConsole.cpp`
- Test: `Source/RefactorTactics/Tests/RTPieSessionConsoleTests.cpp`

**Interfaces:**
- Consumes: `URTPieSessionPlaylist::Compose`, `URTPieSessionSubsystem`
- Produces: i comandi `rt.Pie.Session`, `rt.Pie.Verdict`, `rt.Pie.Session.Abort`

- [ ] **Step 1: Write the failing test**

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionDryRunDoesNotStartTest,
	"RefactorTactics.PieSession.NoArgsPrintsWithoutStarting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionDryRunDoesNotStartTest::RunTest(const FString&)
{
	// `rt.Pie.Session` senza argomenti DICHIARA la coda e non avvia niente: chi apre deve sapere
	// prima cosa la playlist non copre.
	TestTrue(TEXT("il comando esiste"),
		IConsoleManager::Get().FindConsoleObject(TEXT("rt.Pie.Session")) != nullptr);
	TestTrue(TEXT("e anche il verdetto"),
		IConsoleManager::Get().FindConsoleObject(TEXT("rt.Pie.Verdict")) != nullptr);
	TestTrue(TEXT("e l'interruzione"),
		IConsoleManager::Get().FindConsoleObject(TEXT("rt.Pie.Session.Abort")) != nullptr);
	return true;
}
```

- [ ] **Step 2: Run to verify it fails** — Expected: FAIL, i comandi non esistono.

- [ ] **Step 3: Registra i comandi**, nella forma già usata da `rt.Test.Run`:

```cpp
static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTPieSession(
	TEXT("rt.Pie.Session"),
	TEXT("rt.Pie.Session [<prefisso scenario>|<id voce>,<id voce>] — senza argomenti stampa la coda e NON avvia."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTPieSessionCommand));

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTPieVerdict(
	TEXT("rt.Pie.Verdict"),
	TEXT("rt.Pie.Verdict pass|fail|na [motivo] — chiude il passo corrente e passa al successivo."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTPieVerdictCommand));

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTPieAbort(
	TEXT("rt.Pie.Session.Abort"),
	TEXT("rt.Pie.Session.Abort — scrive i verdetti gia' dati, il resto resta NotRun."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTPieAbortCommand));
```

- [ ] **Step 4: Run tests** — Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git commit -am "feat(3208): rt.Pie.Session, rt.Pie.Verdict e l'interruzione che non perde verdetti"
```

---

### Task 7: l'innesto — due aggiunte al coordinator, le porte nel GameMode, la quarta sorgente

**Files:**
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioCoordinator.h/.cpp`
- Modify: `Source/RefactorTactics/RTGameMode.h/.cpp` (`ResolveScenarioToRun` a `RTGameMode.cpp:952`; il coordinator è `RTGameMode.h:480`)
- Test: `Source/RefactorTactics/Tests/RTPieSessionIntegrationTests.cpp`

**Interfaces:**
- Consumes: `URTPieSessionSubsystem::IsConducting()`, `CurrentStep()`
- Produces: `FRTScenarioCoordinator::OnScenarioFinished` (multicast, `const FRTTestResult&`), `FRTScenarioCoordinator::TearDown()`

- [ ] **Step 1: Write the failing test**

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionBeatsConsoleCVarTest,
	"RefactorTactics.PieSession.ConductingSessionBeatsTheConsoleCVar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionBeatsConsoleCVarTest::RunTest(const FString&)
{
	// Una CVar rimasta impostata da una prova precedente dirotterebbe in silenzio ogni passo,
	// e chi guarda crederebbe di giudicare la voce che il conduttore annuncia.
	// La forma reale del guard, come la usa RTScenarioAutoRunTests.cpp:52-54.
	extern TAutoConsoleVariable<FString> CVarRTTestScenario;   // definita in RTTestConsole.cpp
	RTTestConsoleVariable::TGuardia<FString> Guard(*CVarRTTestScenario.AsVariable(),
		TEXT("Scen.Vecchia"));
	// ... allestisci un GameMode con una seduta in corso su `Scen.DellaSeduta`
	TestEqual(TEXT("vince la seduta"), GameMode->ResolveScenarioToRun(), TEXT("Scen.DellaSeduta"));
	return true;
}
```

⚠️ `RTConsoleVariableGuardForTest.h` esiste già e va usato: scrive a priorità corrente e rilegge, che è il difetto corretto in #2235. Per il mondo di prova, copia `MakeAutoRunWorld`/`DestroyAutoRunWorld` da `RTScenarioAutoRunTests.cpp` **rinominandoli** con prefisso proprio.

- [ ] **Step 2: Run to verify it fails** — Expected: FAIL, vince ancora la CVar.

- [ ] **Step 3: Le due aggiunte al coordinator**

`DECLARE_MULTICAST_DELEGATE_OneParam(FRTOnScenarioFinished, const FRTTestResult&);` come membro pubblico, sparato in `Tick` subito dopo il log `FINITO` (`RTScenarioCoordinator.cpp:84-86`); `void TearDown()` che chiama `Session->TearDown()` **e** rilascia `Session`, perché la sbindatura del decisore fa parte del teardown (vedi il commento in testa a `RTScenarioSession.h`).

- [ ] **Step 4: Le porte nel GameMode e la quarta sorgente**

In `BeginPlay`, prima di `ScenarioCoordinator.Start` (`RTGameMode.cpp:523`), il GameMode installa le porte sul subsystem e sottoscrive `OnScenarioFinished`. In `ResolveScenarioToRun` (`RTGameMode.cpp:952`), **prima** del blocco della console:

```cpp
if (const URTPieSessionSubsystem* Conduttore = /* GameInstance subsystem */)
{
    if (Conduttore->IsConducting())
    {
        if (const FRTPieSessionStep* Passo = Conduttore->CurrentStep())
        {
            // La seduta vince: una CVar dura quanto il processo, la seduta dura meno.
            return Passo->ScenarioId;
        }
    }
}
```

- [ ] **Step 5: Run tests** — Expected: PASS, e la suite intera resta verde.

- [ ] **Step 6: Commit**

```bash
git commit -am "feat(3208): la seduta in corso vince su ogni altra sorgente di scenario"
```

---

### Task 8: l'overlay, in C++ e senza asset

**Files:**
- Create: `Source/RefactorTactics/PieSession/RTPieVerdictOverlay.h/.cpp`
- Test: `Source/RefactorTactics/Tests/RTPieSessionOverlayTests.cpp`

**Interfaces:**
- Consumes: `URTPieSessionSubsystem::CurrentStep()`, `::State()`, `::SubmitVerdict()`
- Produces: `URTPieVerdictOverlay::PromptText() const -> FText` e `::HandleVerdictKey(const FKey&) -> bool`

- [ ] **Step 1: Write the failing test**

Il testo del prompt è calcolabile senza schermo, quindi **è** il pezzo da testare:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieOverlayNamesTheItemTest,
	"RefactorTactics.PieSession.OverlayNamesTheItemBeingJudged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieOverlayNamesTheItemTest::RunTest(const FString&)
{
	// L'ancora e' l'ID: chi giudica deve poter ritrovare la voce nel registro.
	// ⛔ Il prompt NON riporta l'esito atteso: quello ha un owner solo.
	const FText Prompt = URTPieVerdictOverlay::ComposePrompt(TEXT("PIE-V01-LOG"), INDEX_NONE,
		TEXT("Guarda la colonna di destra: le righe compaiono mentre i turni scorrono."));
	TestTrue(TEXT("nomina la voce"), Prompt.ToString().Contains(TEXT("PIE-V01-LOG")));
	TestTrue(TEXT("e dice dove guardare"), Prompt.ToString().Contains(TEXT("colonna di destra")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieOverlayKeysMapToVerdictsTest,
	"RefactorTactics.PieSession.OverlayKeysMapToVerdicts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieOverlayKeysMapToVerdictsTest::RunTest(const FString&)
{
	TestTrue(TEXT("1 = PASS"), URTPieVerdictOverlay::VerdictForKey(EKeys::One) == ERTPieVerdict::Pass);
	TestTrue(TEXT("2 = FAIL"), URTPieVerdictOverlay::VerdictForKey(EKeys::Two) == ERTPieVerdict::Fail);
	TestTrue(TEXT("3 = non giudicabile"),
		URTPieVerdictOverlay::VerdictForKey(EKeys::Three) == ERTPieVerdict::NotJudgeable);
	TestTrue(TEXT("un tasto qualunque non e' un verdetto"),
		URTPieVerdictOverlay::VerdictForKey(EKeys::W) == ERTPieVerdict::Pending);
	return true;
}
```

- [ ] **Step 2: Run to verify it fails** — Expected: FAIL di compilazione.

- [ ] **Step 3: Implementa** `ComposePrompt` (statica e pura), `VerdictForKey` (statica e pura), e il `UUserWidget` che le usa: `RebuildWidget` costruisce un `SBorder` con un `STextBlock`, `NativeOnKeyDown` inoltra a `VerdictForKey` e, se diverso da `Pending`, chiama `SubmitVerdict`. `Esc` chiama `Abort()`.

- [ ] **Step 4: Run tests** — Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git commit -am "feat(3208): l'overlay chiede la voce per nome, e i tasti sono una funzione pura"
```

---

### Task 9: il runbook, e la prima seduta condotta

**Files:**
- Create: `docs/technical/runbooks/guida-conduttore-seduta-pie.md`
- Modify: `docs/superpowers/specs/2026-09-20-conduttore-seduta-pie-design.md` (statuto → implementato, con l'SHA)

- [ ] **Step 1: Suite intera, non solo il filtro**

```powershell
& "D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" `
  "-ExecCmds=Automation RunTests RefactorTactics;Quit" `
  -unattended -nopause -nosplash -nullrhi -NoLiveCoding -log
```

Registra `Success` / `Fail` **prima** e **dopo**: un conteggio che cala senza che nessuno lo noti è il modo in cui un gate muore.

- [ ] **Step 2: Il runbook**

Come si apre una seduta: `L_DevSandbox` → console → `rt.Pie.Session <selettore>` (dry-run) → rileggere la coda → `rt.Pie.Session <selettore>` → Play → giudicare. Dove esce il file. Cosa significano i cinque verdetti. Cosa il conduttore **non** copre: voci che richiedono un'altra mappa, e voci che richiedono click o `TAB`.

- [ ] **Step 3: La prima seduta vera**

Conduci una seduta reale su uno dei due scenari popolati nel Task 2. È l'unica verifica a schermo di questo piano, ed è dichiarata tale nella spec §4.5: che l'overlay sia leggibile sopra la scena e che i tasti si premano senza pensarci non ha un gate headless.

- [ ] **Step 4: Commit e PR**

```bash
git commit -am "docs(3208): il runbook del conduttore, e la prima seduta condotta"
gh pr create --base main --title "..." --body "..."
```

---

## Self-review di questo piano

**Copertura della spec:** §2 architettura → Task 3 e 7 · §2.2 porte → Task 3 · §2.3 quarta sorgente → Task 7 · §3.1 `verifies` → Task 1 · §3.2 selettori → Task 5 · §3.3 stati → Task 3 · §3.4 casi limite → Task 3 (tre su quattro) e Task 4 (`Blocked` nel file) · §4.1 file → Task 4 · §4.2 commit → Task 4 · §4.3 overlay → Task 8 · §4.4 i tre gate → Task 3, Task 2, Task 4 · §4.5 ciò che resta a schermo → Task 9.

**Buco trovato e chiuso in revisione:** il caso «`expect` fallite, si chiede lo stesso» non aveva un test *di conduzione* — il gate del Task 4 lo prova sul writer, non sul flusso. Aggiungi al Task 3 un quarto test:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionRedExpectStillAsksTest,
	"RefactorTactics.PieSession.FailedExpectStillAsksForAVerdict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionRedExpectStillAsksTest::RunTest(const FString&)
{
	URTPieSessionSubsystem* Conduttore = NewObject<URTPieSessionSubsystem>();
	FPieSessionBanco Banco;
	Conduttore->Begin({ PieSessionPasso(TEXT("PIE-A"), TEXT("Scen.A")) }, Banco.Porte());
	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.A"), /*bPassed=*/ false));
	TestTrue(TEXT("expect rosse non chiudono il passo da sole"),
		Conduttore->State() == ERTPieSessionState::AwaitingVerdict);
	return true;
}
```
