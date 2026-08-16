# CP 15.3 metà B — piano di implementazione (fase A)

> **Per chi esegue:** questo piano si esegue **un task alla volta**, con `superpowers:subagent-driven-development`
> o `superpowers:executing-plans`. I passi usano `- [ ]` per il tracciamento.

**Obiettivo:** dare al seam `ARTTurnManager::ReactionDecider`, che esiste dal CP 14.5 e che nessuno binda,
un iniettore che legga le decisioni di finestra **dallo scenario** e le traduca nel vocabolario del gioco.

**Architettura:** lo scenario dichiara per turno una coda di `decisions` indirizzate per **unità**;
`RTScenarioSession` binda il decisore soltanto se lo slot è libero, risale allo scenario id del
proprietario della finestra, consuma la prima decisione non usata che lo nomina, e costruisce il token con
`FireResponse(TargetUnit->StableUnitId)`. La legalità della risposta resta dove già sta, in
`AskReactionDecision`.

**Tech stack:** Unreal Engine 5.8.1 · C++ · `IMPLEMENT_SIMPLE_AUTOMATION_TEST` · nessun GAS.

**Spec:** [`cp153b-decision-provider-design-2026-08-16.md`](cp153b-decision-provider-design-2026-08-16.md)
— il piano argomenta dalla spec: si leggono entrambi.

## Vincoli globali

Valgono per **ogni** task, senza ripeterli:

- **Write-set.** Solo questi cinque path. Qualunque altro file è **STOP**, e si emenda
  [`../parallel-batch.yaml`](../parallel-batch.yaml) **prima** di scrivere, mai dopo:
  `Source/RefactorTactics/ScenarioHarness/` · `Source/RefactorTactics/Tests/RTShowcaseScenarioTests.cpp` ·
  `Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp` · questo piano · il referto di design.
- **`Scenarios/` è `integration_only`.** In fase A **nessun JSON del corpus si tocca**: gli scenari di prova
  si costruiscono in memoria, come già fa `RefactorTactics.ShowcaseRelay.ScriptedInputsDriveMatch`.
- **`DecisionBoundary` resta in `KnownUnavailableCapabilities()`** per tutta la fase A. Scoprirla è fase B:
  senza i dati farebbe cadere `Spec/Overwatch/HoldThenFire`, che si aspetta un movimento troncato da un `FIRE`.
- **Guardia obbligatoria**: ogni file di test apre con `#if WITH_DEV_AUTOMATION_TESTS` e chiude con
  `#endif // WITH_DEV_AUTOMATION_TESTS`. Ogni dichiarazione nuova sta **dentro**. È `#923`, e lo stesso
  difetto ha rotto la build Shipping due volte.
- **Unity build**: helper e costanti in namespace anonimo devono avere nomi **distinti da ogni altro file di
  test**. La translation unit è condivisa e due omonimi collidono al merge della unity.
- **Nomi eroe — misurato, non ereditato.** `RTHeroCatalogLibrary.cpp` dichiara **solo**
  `Hero.Flux`, `Hero.Riva`, `Hero.Bastion`, `Hero.Vektor` (tre occorrenze ciascuno), e
  `Gadget`/`Phase`/`Riktor`/`Wraith` hanno **zero** occorrenze in tutto `Source/`: la fetta 3 di `D-130`
  (`#753`) non è stata eseguita. Nei test si usano **quelli**, o il catalogo non risolve — e questo piano ne
  contiene **19** occorrenze, tutte dentro esempi di codice.
  ⛔ **Non «bonificarle»**: `check-docs-naming.py` classifica questo file fra i *registri datati* e le
  tollera proprio perché descrivono il codice com'è. La regola del gate è annotarle accanto
  all'affermazione, ed è ciò che questa riga fa. Il giorno in cui `#753` atterra, cambiano **insieme** al
  catalogo — non prima, o i test smettono di risolvere l'eroe.
  ⚠️ Da non confondere con gli **id di scenario** (`Guardia`, `Corsa`): quelli sono etichette locali al file
  e non hanno niente a che vedere col catalogo.
- **Niente `Delay`, Tick o `DeltaTime`** per decidere sequencing. Il decisore risponde subito.
- **Due `--check` ereditati**: `RTScenarioSession.cpp` alimenta `project-graph.json` e
  `scenariomap.shortlist.md`. Prima di ogni commit che lo tocca:
  `python scripts/feature_registry.py generate --check` e `... shortlist --check`.
- 🔴 **Uno scenario JSON con `expect` vuoto viene RIFIUTATO dal loader** — *«nessuna assertion dichiarata:
  lo scenario passerebbe sempre»* (`RTScenarioLoader.cpp:1051`). Scoperto eseguendo il task 1, non leggendo:
  la prima stesura del suo JSON aveva `"expect": []` e il test è fallito con quel messaggio. Ogni JSON di
  prova porta almeno un'assertion — `{ "type": "TurnsCompleted", "value": 1 }` basta.
  ⚠️ La guardia è **solo nel loader**: gli scenari costruiti in memoria (task 5-9) non la attraversano,
  perché `URTScenarioRunner::Run` riceve la struct già fatta. Non è un permesso a ometterla dove l'asserzione
  riguarda lo stato di gioco — è il motivo per cui in quei task le verifiche stanno sui contatori del
  referto e non su `expect`.

**Come si lancia un test** (una run per volta, `-abslog` distinto: l'engine è condiviso fra i worktree):

```bash
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/rt-simulation/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests <NomeDelTest>+Quit" \
  -unattended -nopause -nullrhi -NoSound \
  -abslog="D:/rt-simulation/Saved/Logs/rt-512-<task>.log"
```

L'esito sta nel log, **non** nello stdout: `grep -E "Test Completed|Success|Fail" <abslog>`.

---

## Struttura dei file

| File | Responsabilità | Task |
|---|---|---|
| `ScenarioHarness/RTTestScenario.h` | il tipo `FRTScenarioDecision` e il campo `Decisions` sul turno | 1 |
| `ScenarioHarness/RTScenarioLoader.cpp` | parsing e **validazione** dello schema; `KnownTurnKeys` | 1, 2, 3 |
| `ScenarioHarness/RTScenarioLoader.h` | `SupportedVersion` | 4 |
| `ScenarioHarness/RTScenarioSession.h/.cpp` | bind, traduzione, coda, residuo, rifiuto, vocabolario | 5, 6, 7, 9, 10 |
| `ScenarioHarness/RTTestResult.h` · `RTTestReportWriter.cpp` | contatori, token applicato, sorgente attiva | 5, 7 |
| `Tests/RTScenarioLoaderTests.cpp` | schema, validazione, divisione del vocabolario | 1-4, 10 |
| `Tests/RTShowcaseScenarioTests.cpp` | bind, coda, residuo, precedenza, rifiuto, mutazione | 5-9 |

**Copertura del DoD** — sei voci, e dove ciascuna si chiude:

| Voce di DoD | Task | Note |
|---|---|---|
| 1 · le decisioni sono uno script separato | 1-4 | il JSON dello showcase è **fase B** |
| 2 · il provider sostituisce `ReactionDecider` | 5 | non ne crea un secondo |
| 3 · `ShowcaseRelay.DecisionProviderIsInjectable` | 5 | |
| 4 · verifica di mutazione | 8 | comportamentale, non sul tipo |
| 5 · `DecisionBoundary` disponibile, showcase oltre il T4 | — | **fase B**: scoprirla senza i dati fa cadere due scenari |
| 6 · `Reaction` viene divisa | 10 | fra i due nomi che esistono, senza un terzo |

---

## Task 1: il tipo e il parsing minimo

**Files:**
- Modify: `Source/RefactorTactics/ScenarioHarness/RTTestScenario.h` (accanto a `FRTScenarioTurn`)
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp` (ciclo dei turni, ~riga 326-353)
- Test: `Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp`

**Interfaces:**
- Produce: `struct FRTScenarioDecision { FString Unit; FString Respond; FString Target; }` e
  `TArray<FRTScenarioDecision> FRTScenarioTurn::Decisions`. I task 5-8 leggono **questi** nomi.

- [x] **Passo 1 — scrivi il test che fallisce**

In `RTScenarioLoaderTests.cpp`, dentro il namespace anonimo:

```cpp
	// Nome distinto da ogni altro file di test: la unity build condivide la translation unit.
	const TCHAR* ScenarioLoaderDecisionsJson = TEXT(R"JSON(
	{
	  "scenarioId": "Spec.Decisions.Parse",
	  "version": 1,
	  "mapRadius": 3,
	  "units": [
	    { "id": "A1", "hero": "Hero.Bastion", "team": 0, "cell": [-2, 0, 0] },
	    { "id": "B1", "hero": "Hero.Vektor",  "team": 1, "cell": [ 2, 0, 0] }
	  ],
	  "turns": [ {
	    "intents": [],
	    "decisions": [
	      { "unit": "A1", "respond": "FIRE", "target": "B1" },
	      { "unit": "A1", "respond": "HOLD" }
	    ]
	  } ],
	  "expect": []
	}
	)JSON");
```

e, fuori dal namespace:

```cpp
/** Le decisioni di finestra si caricano come DATO, in ordine e col vocabolario dello scenario. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderDecisionsTest,
	"RefactorTactics.Scenario.LoaderAcceptsDecisions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderDecisionsTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("scenario con decisions accettato"),
		URTScenarioLoader::LoadFromString(ScenarioLoaderDecisionsJson, Scenario, Error)))
	{
		AddError(FString::Printf(TEXT("motivo del rifiuto: %s"), *Error));
		return false;
	}

	if (!TestEqual(TEXT("un turno"), Scenario.Turns.Num(), 1)) { return false; }
	const FRTScenarioTurn& T = Scenario.Turns[0];
	if (!TestEqual(TEXT("due decisioni"), T.Decisions.Num(), 2)) { return false; }

	// L'ORDINE e' significativo: la coda si consuma in ordine di dichiarazione (task 6).
	TestEqual(TEXT("prima: unita'"),   T.Decisions[0].Unit,    FString(TEXT("A1")));
	TestEqual(TEXT("prima: risposta"), T.Decisions[0].Respond, FString(TEXT("FIRE")));
	TestEqual(TEXT("prima: bersaglio"),T.Decisions[0].Target,  FString(TEXT("B1")));
	TestEqual(TEXT("seconda: risposta"), T.Decisions[1].Respond, FString(TEXT("HOLD")));
	TestTrue(TEXT("HOLD non porta bersaglio"), T.Decisions[1].Target.IsEmpty());
	return true;
}
```

- [x] **Passo 2 — eseguilo e verifica che fallisca**

Comando con `<NomeDelTest>` = `RefactorTactics.Scenario.LoaderAcceptsDecisions`.
Atteso: **errore di compilazione** — `Decisions` non è un membro di `FRTScenarioTurn`. È il fallimento
giusto: il tipo non esiste ancora.

- [x] **Passo 3 — il tipo**

In `RTTestScenario.h`, **prima** di `FRTScenarioTurn`:

```cpp
/**
 * Una decisione di finestra scriptata: la risposta che un'unita' dara' al prossimo decision boundary che
 * si apre per lei.
 *
 * ⚠️ Parla il vocabolario dello SCENARIO e non quello del gioco, e non e' una comodita': l'identita' di una
 * opportunity contiene `MicroStepIndex` e l'`OwnerId` di runtime, e il token di risposta e'
 * `FIRE:<TargetUnitId>` — anche quello un id di runtime. Nessuno dei due e' scrivibile in un JSON. La
 * traduzione avviene dove esiste la mappa, cioe' `UnitsById` in `FRTScenarioSession`.
 */
USTRUCT()
struct FRTScenarioDecision
{
	GENERATED_BODY()

	/** Chi risponde: l'id di scenario dell'unita' proprietaria della finestra. */
	UPROPERTY()
	FString Unit;

	/** `FIRE` oppure `HOLD`. Non e' il token del gioco: quello si costruisce in traduzione. */
	UPROPERTY()
	FString Respond;

	/**
	 * L'id di scenario del bersaglio. Obbligatorio con `FIRE`, **vietato** con `HOLD`: un campo che c'e' e
	 * non conta e' il modo in cui uno scenario dice una cosa e ne verifica un'altra.
	 */
	UPROPERTY()
	FString Target;
};
```

e dentro `FRTScenarioTurn`, sotto `Requires`:

```cpp
	/**
	 * Le risposte scriptate ai decision boundary di questo turno, in ordine. Vuoto = nessuna: le finestre
	 * ricadono su `DecisionOnTimeout`, che e' il comportamento di sempre.
	 */
	UPROPERTY()
	TArray<FRTScenarioDecision> Decisions;
```

- [x] **Passo 4 — il parsing**

In `RTScenarioLoader.cpp`, nel ciclo dei turni, **dopo** il blocco che legge `requires`:

```cpp
			const TArray<TSharedPtr<FJsonValue>>* DecisionsJson = nullptr;
			if (TurnObj->TryGetArrayField(TEXT("decisions"), DecisionsJson))
			{
				for (const TSharedPtr<FJsonValue>& Value : *DecisionsJson)
				{
					const TSharedPtr<FJsonObject> Obj = Value->AsObject();
					if (!Obj.IsValid())
					{
						OutError = TEXT("decisions: voce non valida");
						return false;
					}

					FRTScenarioDecision Decision;
					Obj->TryGetStringField(TEXT("unit"), Decision.Unit);
					Obj->TryGetStringField(TEXT("respond"), Decision.Respond);
					Obj->TryGetStringField(TEXT("target"), Decision.Target);
					Turn.Decisions.Add(Decision);
				}
			}
```

- [x] **Passo 5 — eseguilo e verifica che passi**

Atteso: `Success`. Se il turno non porta due decisioni, stampa `Error` e confronta col JSON.

- [x] **Passo 6 — commit**

```bash
git add Source/RefactorTactics/ScenarioHarness/RTTestScenario.h \
        Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp \
        Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp
git commit -m "feat(512): le decisioni di finestra sono un dato dello scenario, non un binding di test"
```

---

## Task 2: la validazione dello schema

**Files:**
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp` (il blocco del task 1)
- Test: `Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp`

**Interfaces:**
- Consuma: `FRTScenarioDecision` dal task 1.
- Produce: quattro messaggi d'errore che i task successivi non devono duplicare.

- [x] **Passo 1 — scrivi i test che falliscono**

Un solo test con quattro casi negativi, perché condividono la meccanica:

```cpp
/**
 * La validazione e' la meta' che conta. Uno scenario scritto male deve produrre un ERROR leggibile, non un
 * `HOLD` silenzioso: `HoldNoDecider` e' indistinguibile da «nessuno ha risposto», ed e' il modo in cui un
 * refuso resta verde per sempre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderDecisionsRejectTest,
	"RefactorTactics.Scenario.LoaderRejectsMalformedDecisions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderDecisionsRejectTest::RunTest(const FString&)
{
	auto Rifiuta = [this](const TCHAR* Cosa, const FString& Decision)
	{
		const FString Json = FString::Printf(TEXT(R"JSON(
		{
		  "scenarioId": "Spec.Decisions.Reject", "version": 1, "mapRadius": 3,
		  "units": [ { "id": "A1", "hero": "Hero.Bastion", "team": 0, "cell": [-2, 0, 0] } ],
		  "turns": [ { "intents": [], "decisions": [ %s ] } ],
		  "expect": []
		}
		)JSON"), *Decision);

		FRTTestScenario Scenario;
		FString Error;
		const bool bLoaded = URTScenarioLoader::LoadFromString(*Json, Scenario, Error);
		TestFalse(FString::Printf(TEXT("%s: rifiutato"), Cosa), bLoaded);
		// Il messaggio deve NOMINARE il difetto: un rifiuto muto manda a cercare nel posto sbagliato.
		TestFalse(FString::Printf(TEXT("%s: con un motivo"), Cosa), Error.IsEmpty());
	};

	Rifiuta(TEXT("chiave sconosciuta"),
		TEXT(R"({ "unit": "A1", "respond": "HOLD", "reason": "perche' si" })"));
	Rifiuta(TEXT("respond sconosciuto"),
		TEXT(R"({ "unit": "A1", "respond": "MAYBE" })"));
	Rifiuta(TEXT("FIRE senza bersaglio"),
		TEXT(R"({ "unit": "A1", "respond": "FIRE" })"));
	Rifiuta(TEXT("HOLD con bersaglio"),
		TEXT(R"({ "unit": "A1", "respond": "HOLD", "target": "B1" })"));
	Rifiuta(TEXT("unita' inesistente"),
		TEXT(R"({ "unit": "Z9", "respond": "HOLD" })"));
	return true;
}
```

- [x] **Passo 2 — eseguilo e verifica che fallisca**

`RefactorTactics.Scenario.LoaderRejectsMalformedDecisions`.
Atteso: **cinque `TestFalse` rossi** — oggi il loader accetta tutto, perché `TryGetStringField` ignora ciò
che non conosce.

- [x] **Passo 3 — la validazione**

Nel blocco del task 1, subito dopo i tre `TryGetStringField`:

```cpp
					// L'elenco delle chiavi attese si GENERA dal set e si ORDINA: le due copie sono divergite
					// alla prima aggiunta (`edge`), e un `TSet` non ha ordine — un messaggio che cambia testo
					// fra due esecuzioni identiche fa dubitare del file invece che di se' stesso.
					static const TSet<FString> KnownDecisionKeys = {
						TEXT("unit"), TEXT("respond"), TEXT("target")
					};
					TArray<FString> UnknownDecisionKeys;
					for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : Obj->Values)
					{
						if (Field.Key.StartsWith(TEXT("_"))) { continue; } // commenti, come altrove nel formato
						if (!KnownDecisionKeys.Contains(Field.Key)) { UnknownDecisionKeys.Add(Field.Key); }
					}
					if (UnknownDecisionKeys.Num() > 0)
					{
						UnknownDecisionKeys.Sort();
						TArray<FString> Expected = KnownDecisionKeys.Array();
						Expected.Sort();
						OutError = FString::Printf(
							TEXT("decisions: chiave sconosciuta '%s' (previste: %s)"),
							*UnknownDecisionKeys[0], *FString::Join(Expected, TEXT(", ")));
						return false;
					}

					const bool bFire = Decision.Respond.Equals(TEXT("FIRE"), ESearchCase::CaseSensitive);
					const bool bHold = Decision.Respond.Equals(TEXT("HOLD"), ESearchCase::CaseSensitive);
					if (!bFire && !bHold)
					{
						OutError = FString::Printf(
							TEXT("decisions: risposta '%s' sconosciuta (previste: FIRE, HOLD)"), *Decision.Respond);
						return false;
					}
					// `target` obbligatorio con FIRE e VIETATO con HOLD. Il secondo divieto e' la meta' che
					// conta: un bersaglio ignorato fa dichiarare allo scenario una cosa che non verifica.
					if (bFire && Decision.Target.IsEmpty())
					{
						OutError = TEXT("decisions: 'FIRE' richiede 'target'");
						return false;
					}
					if (bHold && !Decision.Target.IsEmpty())
					{
						OutError = FString::Printf(
							TEXT("decisions: 'HOLD' non ammette 'target' (dichiarato '%s')"), *Decision.Target);
						return false;
					}
					// I nomi si risolvono QUI: un'unita' che non esiste e' uno scenario scritto male, non una
					// capability mancante — e `Blocked` direbbe la seconda cosa.
					if (!OutScenario.FindUnit(Decision.Unit))
					{
						OutError = FString::Printf(TEXT("decisions: unita' '%s' non schierata"), *Decision.Unit);
						return false;
					}
					if (bFire && !OutScenario.FindUnit(Decision.Target))
					{
						OutError = FString::Printf(TEXT("decisions: bersaglio '%s' non schierato"), *Decision.Target);
						return false;
					}
```

⚠️ `FindUnit` richiede che `units` sia già stato letto: verifica che il ciclo dei turni venga **dopo** quello
delle unità nel loader. Se non è così, sposta i due controlli sui nomi in una passata finale invece di
riordinare il parsing.

- [x] **Passo 4 — eseguilo e verifica che passi**

Atteso: `Success`. Rilancia anche `RefactorTactics.Scenario.LoaderAcceptsDecisions`: il caso felice non
deve essere diventato rosso.

- [x] **Passo 5 — commit**

```bash
git add Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp \
        Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp
git commit -m "feat(512): uno scenario scritto male e' un errore leggibile, non un HOLD silenzioso"
```

---

## Task 3: `KnownTurnKeys`

Oggi il controllo sulle chiavi sconosciute esiste **solo dentro `intents`**. A livello di turno non c'è: un
refuso `desicions` verrebbe ignorato, il turno cadrebbe su `HoldNoDecider` e resterebbe verde.

**Files:**
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp` (ciclo dei turni)
- Test: `Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp`

- [ ] **Passo 1 — misura che il corpus regga, prima di scrivere**

```bash
python -c "
import json,os,collections
k=collections.Counter()
for r,_,ns in os.walk('Scenarios'):
    for n in [x for x in ns if x.endswith('.json')]:
        d=json.load(open(os.path.join(r,n),encoding='utf-8-sig'))
        for t in d.get('turns',[]):
            for key in t: k[key]+=1
print(dict(k))
"
```

Atteso: `{'_turno': 64, 'intents': 113, 'requires': 36, '_nota': 3}` — quattro chiavi, e le due con `_` sono
già la convenzione dei commenti. Se ne compare una quinta, **fermati**: l'elenco va allargato con lei prima
di introdurre il controllo, o rendi rosso il corpus.

- [ ] **Passo 2 — scrivi il test che fallisce**

```cpp
/** Un refuso a livello di turno non deve essere ignorato: `desicions` non e' un commento. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderTurnKeysTest,
	"RefactorTactics.Scenario.LoaderRejectsUnknownTurnKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderTurnKeysTest::RunTest(const FString&)
{
	const TCHAR* Json = TEXT(R"JSON(
	{
	  "scenarioId": "Spec.Decisions.TurnKey", "version": 1, "mapRadius": 3,
	  "units": [ { "id": "A1", "hero": "Hero.Bastion", "team": 0, "cell": [-2, 0, 0] } ],
	  "turns": [ { "intents": [], "desicions": [] } ],
	  "expect": []
	}
	)JSON");

	FRTTestScenario Scenario;
	FString Error;
	TestFalse(TEXT("refuso di turno rifiutato"), URTScenarioLoader::LoadFromString(Json, Scenario, Error));
	TestTrue(TEXT("il messaggio nomina la chiave"), Error.Contains(TEXT("desicions")));

	// E il commento resta un commento: `_turno` e `_nota` non devono cadere insieme al refuso.
	const TCHAR* ConCommento = TEXT(R"JSON(
	{
	  "scenarioId": "Spec.Decisions.TurnComment", "version": 1, "mapRadius": 3,
	  "units": [ { "id": "A1", "hero": "Hero.Bastion", "team": 0, "cell": [-2, 0, 0] } ],
	  "turns": [ { "_turno": "commento", "_nota": "altro", "intents": [] } ],
	  "expect": []
	}
	)JSON");
	FRTTestScenario ConCommentoScenario;
	FString CommentoError;
	if (!TestTrue(TEXT("i commenti di turno restano ammessi"),
		URTScenarioLoader::LoadFromString(ConCommento, ConCommentoScenario, CommentoError)))
	{
		AddError(FString::Printf(TEXT("motivo del rifiuto: %s"), *CommentoError));
	}
	return true;
}
```

- [ ] **Passo 3 — eseguilo e verifica che fallisca**

`RefactorTactics.Scenario.LoaderRejectsUnknownTurnKey`. Atteso: il primo `TestFalse` è rosso — il refuso oggi
viene accettato.

- [ ] **Passo 4 — il controllo**

Nel ciclo dei turni, **prima** di leggere `requires`:

```cpp
			// Le chiavi di turno ammesse. Misurato sul corpus del 2026-08-16: sono quattro, e le due con `_`
			// sono la convenzione dei commenti. Senza questo controllo un refuso — `desicions` per
			// `decisions` — viene ignorato e il turno cade su `HoldNoDecider`, che e' indistinguibile da
			// «nessuno ha risposto»: verde per il motivo sbagliato.
			{
				static const TSet<FString> KnownTurnKeys = {
					TEXT("intents"), TEXT("requires"), TEXT("decisions")
				};
				TArray<FString> UnknownTurnKeys;
				for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : TurnObj->Values)
				{
					if (Field.Key.StartsWith(TEXT("_"))) { continue; }
					if (!KnownTurnKeys.Contains(Field.Key)) { UnknownTurnKeys.Add(Field.Key); }
				}
				if (UnknownTurnKeys.Num() > 0)
				{
					UnknownTurnKeys.Sort();
					TArray<FString> Expected = KnownTurnKeys.Array();
					Expected.Sort();
					OutError = FString::Printf(TEXT("turns: chiave sconosciuta '%s' (previste: %s)"),
						*UnknownTurnKeys[0], *FString::Join(Expected, TEXT(", ")));
					return false;
				}
			}
```

- [ ] **Passo 5 — eseguilo, e poi esegui il corpus intero**

Prima `RefactorTactics.Scenario.LoaderRejectsUnknownTurnKey` → `Success`.
Poi **`RefactorTactics.Scenario`** per intero: `LoaderAcceptsValidScenario`,
`ShippedScenariosParse` e `EveryShippedScenarioRuns` non devono muoversi. Se un file del corpus cade, la
misura del passo 1 è stata saltata.

- [ ] **Passo 6 — commit**

```bash
git add Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp \
        Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp
git commit -m "feat(512): un refuso a livello di turno smette di essere ignorato"
```

---

## Task 4: `SupportedVersion` a 2

**Files:**
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.h:24`
- Test: `Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp`

- [ ] **Passo 1 — scrivi il test che fallisce**

```cpp
/**
 * Il formato cresce di una versione, e il gate serve nel verso che conta: una build VECCHIA deve
 * **rifiutare** uno scenario che non sa leggere, non ignorarne i campi. Da `#926` gli scenari viaggiano
 * dentro il pacchetto, quindi la coppia build/dato puo' disallinearsi davvero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLoaderVersionTwoTest,
	"RefactorTactics.Scenario.LoaderSupportsVersionTwo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLoaderVersionTwoTest::RunTest(const FString&)
{
	auto Prova = [this](int32 Versione, bool bAtteso)
	{
		const FString Json = FString::Printf(TEXT(R"JSON(
		{
		  "scenarioId": "Spec.Decisions.Version", "version": %d, "mapRadius": 3,
		  "units": [ { "id": "A1", "hero": "Hero.Bastion", "team": 0, "cell": [-2, 0, 0] } ],
		  "turns": [ { "intents": [] } ],
		  "expect": []
		}
		)JSON"), Versione);

		FRTTestScenario Scenario;
		FString Error;
		const bool bLoaded = URTScenarioLoader::LoadFromString(*Json, Scenario, Error);
		TestEqual(FString::Printf(TEXT("versione %d"), Versione), bLoaded, bAtteso);
	};

	Prova(1, true);   // i 76 file esistenti restano a 1 e non si toccano
	Prova(2, true);   // la versione che dichiara `decisions`
	Prova(3, false);  // il gate resta un gate
	return true;
}
```

- [ ] **Passo 2 — eseguilo e verifica che fallisca**

Atteso: il caso `2` è rosso — `SupportedVersion` è ancora 1.

- [ ] **Passo 3 — alza la costante**

In `RTScenarioLoader.h`:

```cpp
	// 1 → 2 con `#512`: la versione che ammette `decisions` a livello di turno. I 76 file esistenti restano
	// a 1 e non si toccano — il gate confronta con `>`, quindi una versione piu' bassa e' sempre leggibile.
	static constexpr int32 SupportedVersion = 2;
```

- [ ] **Passo 4 — eseguilo e verifica che passi**

Atteso: `Success`. Rilancia `RefactorTactics.Scenario` per intero.

- [ ] **Passo 5 — commit**

```bash
git add Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.h \
        Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp
git commit -m "feat(512): il formato scenario dichiara la versione che ammette le decisioni"
```

---

## Task 5: bind e traduzione — la prima finestra risponde

Qui nasce il test che il DoD nomina per nome.

**Files:**
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioSession.h` (stato della coda)
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp` (bind ~riga 495, unbind in `TearDown`)
- Test: `Source/RefactorTactics/Tests/RTShowcaseScenarioTests.cpp`

**Interfaces:**
- Consuma: `FRTScenarioDecision`, `FRTScenarioTurn::Decisions` (task 1).
- Produce: `FString FRTScenarioSession::DecideScriptedResponse(const FRTReactionOpportunity&, int32 OwnerUnitId)`
  — i task 6 e 7 estendono **questa** funzione.

- [ ] **Passo 1 — scrivi il test che fallisce**

In `RTShowcaseScenarioTests.cpp`:

```cpp
/**
 * Il test che `#512` nomina: un decisore INIETTATO risponde a una finestra vera, e la risposta viene
 * dallo scenario invece che da una persona.
 *
 * ⚠️ Lo scenario e' costruito qui e non caricato da file: `Scenarios/` e' `integration_only`, e in fase A
 * nessun JSON del corpus si tocca.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseDecisionProviderTest,
	"RefactorTactics.ShowcaseRelay.DecisionProviderIsInjectable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseDecisionProviderTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	Scenario.ScenarioId = TEXT("Internal.DecisionProviderIsInjectable");
	Scenario.MapRadius = 4;

	auto Unita = [](const TCHAR* Id, const TCHAR* Hero, int32 Team, const FRTCellId& Cell)
	{
		FRTScenarioUnit U; U.Id = Id; U.HeroId = FName(Hero); U.TeamId = Team; U.Cell = Cell; return U;
	};
	// Il guardiano e il bersaglio sulla stessa riga: il cono guarda a Ovest, il bersaglio lo attraversa.
	Scenario.Units.Add(Unita(TEXT("Guardia"), TEXT("Hero.Bastion"), 0, FRTCellId( 2, 0, 0)));
	Scenario.Units.Add(Unita(TEXT("Corsa"),   TEXT("Hero.Vektor"),  1, FRTCellId(-2, 0, 0)));

	{
		FRTScenarioTurn T;
		T.Requires.Add(TEXT("DecisionBoundary"));

		FRTScenarioIntent Arma;
		Arma.UnitId = TEXT("Guardia");
		Arma.Ability = FName(TEXT("Action.Overwatch"));
		T.Intents.Add(Arma);

		FRTScenarioIntent Attraversa;
		Attraversa.UnitId = TEXT("Corsa");
		Attraversa.Move.Add(FRTCellId(-1, 0, 0));
		Attraversa.Move.Add(FRTCellId( 0, 0, 0));
		T.Intents.Add(Attraversa);

		FRTScenarioDecision D;
		D.Unit = TEXT("Guardia"); D.Respond = TEXT("FIRE"); D.Target = TEXT("Corsa");
		T.Decisions.Add(D);

		Scenario.Turns.Add(T);
	}

	// `RunScenarioIsolated` crea il mondo, esegue e lo distrugge: e' l'API che `RTWorldFixtures.h` espone
	// proprio per questo, e usarla evita la coppia crea/distruggi da tenere allineata a mano.
	const FRTTestResult Result = RTWorldFixtures::RunScenarioIsolated(Scenario);

	// ⚠️ In fase A `DecisionBoundary` e' ancora fra le indisponibili, quindi il turno e' `Blocked` per
	// costruzione: cio' che si verifica qui NON e' l'esito del turno, e' che il decisore sia stato
	// interrogato e abbia risposto con la decisione dello scenario.
	TestEqual(TEXT("il decisore ha risposto una volta"), Result.ScriptedDecisionsApplied, 1);
	TestEqual(TEXT("nessuna decisione e' rimasta inutilizzata"), Result.ScriptedDecisionsUnused, 0);
	// La TRADUZIONE e' l'unica parte che il JSON non poteva esprimere: si verifica sul token, non
	// sull'esito. `Corsa` e' la seconda unita' schierata, e il suo `StableUnitId` e' assegnato allo spawn.
	TestTrue(TEXT("il token applicato e' un FIRE, non un HOLD"),
		Result.LastScriptedResponse.StartsWith(TEXT("FIRE:")));
	return true;
}
```

⚠️ **Se `Result.ScriptedDecisionsApplied` resta 0**, la finestra non si è aperta: non è il provider a essere
rotto, è la geometria. Stampa il TurnLog con
`AddInfo(URTTurnLogLibrary::ToText(Result.TurnLog))` e cerca la voce dell'Overwatch; se il watcher non è
armato, il problema è l'intent, non la decisione. Correggi le celle finché una finestra si apre — **non**
rilassare l'asserzione.

- [ ] **Passo 2 — eseguilo e verifica che fallisca**

`RefactorTactics.ShowcaseRelay.DecisionProviderIsInjectable`.
Atteso: **errore di compilazione** — `ScriptedDecisionsApplied` non esiste su `FRTTestResult`.

- [ ] **Passo 3 — i due contatori nel referto**

In `ScenarioHarness/RTTestResult.h`, dentro `FRTTestResult`:

```cpp
	/** Quante decisioni scriptate sono state effettivamente APPLICATE a una finestra. */
	int32 ScriptedDecisionsApplied = 0;

	/**
	 * Quante sono state dichiarate e non hanno mai trovato una finestra. ⚠️ Diverso da zero e' un
	 * FALLIMENTO (task 6), non un avanzo: descrive qualcosa che non e' successo.
	 */
	int32 ScriptedDecisionsUnused = 0;

	/**
	 * L'ultimo token restituito dal decisore scriptato, per come il gioco lo ha ricevuto — `HOLD` o
	 * `FIRE:<StableUnitId>`. Serve a verificare la **traduzione**, che e' l'unica parte che lo scenario non
	 * poteva esprimere: senza, si potrebbe solo osservare che «qualcosa e' successo».
	 */
	FString LastScriptedResponse;
```

- [ ] **Passo 4 — lo stato della coda nella session**

In `RTScenarioSession.h`, fra i membri privati:

```cpp
	/** Le decisioni del turno corrente, con l'indice della prima non ancora consumata per ciascuna unita'. */
	TArray<FRTScenarioDecision> PendingDecisions;
	TArray<bool> PendingConsumed;

	/**
	 * Chi risponde alle finestre in questa esecuzione: `scenario`, `test-override`, `none`. Deciso una volta
	 * al bind, e copiato nel referto (task 7).
	 */
	FString DecisionSource = TEXT("none");

	/** Il decisore scriptato: risponde con la coda del turno, stringa vuota se nulla combacia. */
	FString DecideScriptedResponse(const FRTReactionOpportunity& Opportunity, int32 OwnerUnitId);
```

- [ ] **Passo 5 — bind, traduzione, unbind**

In `RTScenarioSession.cpp`, subito dopo `TurnManager = TM;` (~riga 495):

```cpp
	// Il bind avviene DOPO `UnitsById`: la traduzione ha bisogno della mappa, o non ha con cosa tradurre.
	//
	// ⚠️ Solo se lo slot e' libero, ed e' cosi' che un test ha la precedenza: chi binda prima vince. Non
	// serve una catena ne' un flag — il delegate e' uno slot solo, e la forma E' la regola.
	if (!TM->ReactionDecider.IsBound())
	{
		TM->ReactionDecider.BindRaw(this, &FRTScenarioSession::DecideScriptedResponse);
		DecisionSource = TEXT("scenario");
	}
	else
	{
		DecisionSource = TEXT("test-override");
	}
```

e la funzione:

```cpp
FString FRTScenarioSession::DecideScriptedResponse(const FRTReactionOpportunity& Opportunity, int32 OwnerUnitId)
{
	// Risale allo scenario id del proprietario: `UnitsById` va nel verso opposto, e una scansione su quattro
	// unita' costa meno di una seconda mappa da tenere allineata.
	FString OwnerScenarioId;
	for (const TPair<FString, TWeakObjectPtr<ARTUnit>>& Pair : UnitsById)
	{
		const ARTUnit* Unit = Pair.Value.Get();
		// `StableUnitId`, non un indice: e' l'id che viaggia in `FRTSuppressionMover` e che
		// `FireResponse(int32)` incapsula nel token.
		if (Unit && Unit->StableUnitId == OwnerUnitId) { OwnerScenarioId = Pair.Key; break; }
	}
	if (OwnerScenarioId.IsEmpty()) { return FString(); }

	for (int32 Index = 0; Index < PendingDecisions.Num(); ++Index)
	{
		if (PendingConsumed[Index]) { continue; }
		const FRTScenarioDecision& D = PendingDecisions[Index];
		if (D.Unit != OwnerScenarioId) { continue; }

		PendingConsumed[Index] = true;
		++Result.ScriptedDecisionsApplied;

		if (D.Respond.Equals(TEXT("HOLD"), ESearchCase::CaseSensitive))
		{
			Result.LastScriptedResponse = URTReactionOpportunityLibrary::HoldResponse();
			return Result.LastScriptedResponse;
		}
		// `FIRE`: il token porta l'id di RUNTIME, che e' esattamente cio' che lo scenario non poteva scrivere.
		const TWeakObjectPtr<ARTUnit>* Found = UnitsById.Find(D.Target);
		const ARTUnit* TargetUnit = Found ? Found->Get() : nullptr;
		if (!TargetUnit) { return FString(); } // il loader lo ha gia' validato: qui e' difesa, non politica
		Result.LastScriptedResponse = URTReactionOpportunityLibrary::FireResponse(TargetUnit->StableUnitId);
		return Result.LastScriptedResponse;
	}

	// Nessuna decisione combacia: «non ho risposto». E' il comportamento di sempre — `DecisionOnTimeout` —
	// e tiene intatti i turni che non scriptano nulla.
	return FString();
}
```

Popola la coda all'inizio di ogni turno, dove il turno corrente viene letto:

```cpp
	PendingDecisions = Turn.Decisions;
	PendingConsumed.Init(false, PendingDecisions.Num());
```

E in `TearDown`, **prima** di distruggere il manager:

```cpp
	// Sbinda: un delegate che sopravvive a uno scenario risponderebbe al successivo con la coda del
	// precedente, e il secondo scenario sarebbe verde o rosso per il turno di un altro.
	if (ARTTurnManager* TM = TurnManager.Get())
	{
		TM->ReactionDecider.Unbind();
	}
```

- [ ] **Passo 6 — eseguilo e verifica che passi**

Atteso: `Success`, con `ScriptedDecisionsApplied >= 1`. Se resta 0, vedi l'avvertenza del passo 1.

- [ ] **Passo 7 — i due `--check` ereditati**

```bash
python scripts/feature_registry.py generate --check
python scripts/feature_registry.py shortlist --check
```

Entrambi devono restare verdi: `RTScenarioSession.cpp` alimenta `project-graph.json` e
`scenariomap.shortlist.md`. Se uno è rosso, **rigenera** e includi il generato nel commit.

- [ ] **Passo 8 — commit**

```bash
git add Source/RefactorTactics/ScenarioHarness/ Source/RefactorTactics/Tests/RTShowcaseScenarioTests.cpp
git commit -m "feat(512): il seam esisteva da CP 14.5 e nessuno lo bindava — ora lo scenario risponde"
```

---

## Task 6: la coda per unità e il residuo

**Files:**
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp`
- Test: `Source/RefactorTactics/Tests/RTShowcaseScenarioTests.cpp`

- [ ] **Passo 1 — scrivi il test che fallisce**

```cpp
/**
 * Il residuo e' un FALLIMENTO, non un avanzo. Senza questi due controlli uno scenario puo' scriptare due
 * decisioni, vederne applicare una, e restare verde: e' il modo in cui un test smette di verificare senza
 * dirlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseDecisionResidueTest,
	"RefactorTactics.ShowcaseRelay.UnusedScriptedDecisionFailsTheTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseDecisionResidueTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	Scenario.ScenarioId = TEXT("Internal.UnusedScriptedDecision");
	Scenario.MapRadius = 4;

	FRTScenarioUnit Sola;
	Sola.Id = TEXT("Sola"); Sola.HeroId = FName(TEXT("Hero.Bastion")); Sola.TeamId = 0;
	Sola.Cell = FRTCellId(0, 0, 0);
	Scenario.Units.Add(Sola);

	// Nessun intent, quindi nessuna finestra: la decisione non puo' trovare nulla da cui essere consumata.
	FRTScenarioTurn T;
	FRTScenarioDecision D;
	D.Unit = TEXT("Sola"); D.Respond = TEXT("HOLD");
	T.Decisions.Add(D);
	Scenario.Turns.Add(T);

	const FRTTestResult Result = RTWorldFixtures::RunScenarioIsolated(Scenario);

	TestEqual(TEXT("la decisione resta inutilizzata"), Result.ScriptedDecisionsUnused, 1);
	// `Error` e non `Fail`: e' lo stesso verso che la session usa gia' quando lo SCENARIO e' scritto male
	// (`RTScenarioSession.cpp:973`), distinto da un'aspettativa di gioco caduta.
	TestEqual(TEXT("e lo scenario e' in errore"), Result.Outcome, ERTTestOutcome::Error);
	TestTrue(TEXT("il messaggio nomina l'unita'"), Result.ErrorMessage.Contains(TEXT("Sola")));
	return true;
}
```

E il secondo test, che copre l'altra metà — la coda consumata **in ordine** e la finestra **scoperta**:

```cpp
/**
 * Due decisioni per la stessa unita' si consumano in ordine di dichiarazione, e una finestra in piu' delle
 * decisioni dichiarate non e' un timeout: e' una finestra scoperta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseDecisionQueueTest,
	"RefactorTactics.ShowcaseRelay.ScriptedDecisionsAreConsumedInOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseDecisionQueueTest::RunTest(const FString&)
{
	// Lo stesso allestimento del task 5, con DUE decisioni per `Guardia`: la prima `HOLD`, la seconda
	// `FIRE`. Se la coda fosse posizionale invece che per unita', l'ordine cambierebbe col movimento.
	FRTTestScenario Scenario;
	Scenario.ScenarioId = TEXT("Internal.ScriptedDecisionsInOrder");
	Scenario.MapRadius = 4;
	auto U = [](const TCHAR* Id, const TCHAR* Hero, int32 Team, const FRTCellId& C)
	{
		FRTScenarioUnit X; X.Id = Id; X.HeroId = FName(Hero); X.TeamId = Team; X.Cell = C; return X;
	};
	Scenario.Units.Add(U(TEXT("Guardia"), TEXT("Hero.Bastion"), 0, FRTCellId( 2, 0, 0)));
	Scenario.Units.Add(U(TEXT("Corsa"),   TEXT("Hero.Vektor"),  1, FRTCellId(-2, 0, 0)));

	FRTScenarioTurn T;
	T.Requires.Add(TEXT("DecisionBoundary"));
	FRTScenarioIntent Arma; Arma.UnitId = TEXT("Guardia"); Arma.Ability = FName(TEXT("Action.Overwatch"));
	T.Intents.Add(Arma);
	FRTScenarioIntent Corre; Corre.UnitId = TEXT("Corsa");
	Corre.Move.Add(FRTCellId(-1, 0, 0)); Corre.Move.Add(FRTCellId(0, 0, 0)); Corre.Move.Add(FRTCellId(1, 0, 0));
	T.Intents.Add(Corre);

	FRTScenarioDecision Prima; Prima.Unit = TEXT("Guardia"); Prima.Respond = TEXT("HOLD");
	FRTScenarioDecision Seconda; Seconda.Unit = TEXT("Guardia"); Seconda.Respond = TEXT("FIRE");
	Seconda.Target = TEXT("Corsa");
	T.Decisions.Add(Prima);
	T.Decisions.Add(Seconda);
	Scenario.Turns.Add(T);

	const FRTTestResult Result = RTWorldFixtures::RunScenarioIsolated(Scenario);

	TestEqual(TEXT("entrambe consumate"), Result.ScriptedDecisionsApplied, 2);
	TestEqual(TEXT("nessun residuo"), Result.ScriptedDecisionsUnused, 0);
	// La SECONDA e' il `FIRE`: se l'ordine fosse invertito l'ultimo token sarebbe `HOLD`.
	TestTrue(TEXT("l'ultima applicata e' il FIRE"), Result.LastScriptedResponse.StartsWith(TEXT("FIRE:")));
	return true;
}
```

⚠️ Questo test **dipende dal numero di finestre** che quell'allestimento apre: se ne apre tre invece di due,
la terza è scoperta e lo scenario va in `Error` — ed è il comportamento del passo 4, non un difetto.
Conta le finestre nel TurnLog **prima** di fissare il numero di decisioni, invece di dedurlo dai micro-step.

- [ ] **Passo 2 — eseguilo e verifica che fallisca**

`RefactorTactics.ShowcaseRelay.UnusedScriptedDecisionFailsTheTurn`.
Atteso: entrambe le asserzioni rosse — oggi il residuo non è né contato né valutato.

- [ ] **Passo 3 — conta il residuo e falla cadere**

A fine di ogni turno, dove il risultato del turno viene consolidato:

```cpp
	// Il residuo si valuta a fine turno, non a fine scenario: una decisione dichiarata al T2 e mai consumata
	// e' un difetto del T2, e attribuirla al T8 manderebbe a cercare nel posto sbagliato.
	for (int32 Index = 0; Index < PendingDecisions.Num(); ++Index)
	{
		if (PendingConsumed[Index]) { continue; }
		++Result.ScriptedDecisionsUnused;
		Result.Outcome = ERTTestOutcome::Error;
		Result.ErrorMessage = FString::Printf(
			TEXT("turno %d: decisione dichiarata per '%s' (%s) e mai consumata — nessuna finestra si e' "
			     "aperta per quell'unita'"),
			TurnIndex + 1, *PendingDecisions[Index].Unit, *PendingDecisions[Index].Respond);
		Notes.Add(Result.ErrorMessage);
	}
```

⚠️ **Non** usare `Result.Assertions.Add`: `FRTAssertionResult` porta un `ERTAssertionKind` che ha solo valori
di dominio (`UnitAtCell`, `TurnsCompleted`, `UnitHpEquals`, `UnitAlive`), e inventarne uno per un difetto
dell'harness renderebbe falso il `Kind` di ogni assertion letta dal referto. La forma qui sopra è quella che
la session usa già per «lo scenario è scritto male» — `RTScenarioSession.cpp:973-974`.

- [ ] **Passo 4 — il secondo controllo: finestra senza risposta**

Nella funzione del task 5, quando il ciclo non trova nulla **e il turno dichiarava decisioni**:

```cpp
	if (PendingDecisions.Num() > 0)
	{
		// Il turno dichiara decisioni e questa finestra non ne ha trovata nessuna: non e' il caso «turno non
		// scriptato», e' una finestra scoperta. Se restasse un timeout silenzioso, due decisioni scritte e una
		// applicata sarebbero verdi.
		Result.Outcome = ERTTestOutcome::Error;
		Result.ErrorMessage = FString::Printf(
			TEXT("turno %d: finestra aperta per l'unita' %d senza una decisione che la nomini"),
			TurnIndex + 1, OwnerUnitId);
		Notes.Add(Result.ErrorMessage);
	}
	return FString();
```

- [ ] **Passo 5 — eseguilo e verifica che passi**

Atteso: `Success`. Rilancia anche `DecisionProviderIsInjectable`: **non deve** essere diventato rosso —
se lo è, il suo turno apriva più finestre di quante decisioni dichiarasse, e il piano l'ha appena scoperto.

- [ ] **Passo 6 — commit**

```bash
git add Source/RefactorTactics/ScenarioHarness/ Source/RefactorTactics/Tests/RTShowcaseScenarioTests.cpp
git commit -m "feat(512): una decisione mai consumata fa cadere il turno invece di restare un avanzo"
```

---

## Task 7: precedenza al test e provenienza nel referto

**Files:**
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioSession.h/.cpp` (`DecisionSource`)
- Modify: `Source/RefactorTactics/ScenarioHarness/RTTestResult.h` · `RTTestReportWriter.cpp`
- Test: `Source/RefactorTactics/Tests/RTShowcaseScenarioTests.cpp`

- [ ] **Passo 1 — scrivi il test che fallisce**

```cpp
/**
 * Due sorgenti per la stessa decisione, e la precedenza e' del test. Il prezzo e' che uno scenario con
 * `decisions` viene ignorato in silenzio: percio' la provenienza si SCRIVE. Al replay serve quale
 * decisione, non chi l'ha fornita — ma a chi diagnostica una divergenza serve la seconda.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseDecisionSourceTest,
	"RefactorTactics.ShowcaseRelay.TestDeciderWinsAndIsRecorded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseDecisionSourceTest::RunTest(const FString&)
{
	// ⚠️ Qui NON si usa `RunScenarioIsolated`: il mondo serve **prima** dello scenario, perche' il manager
	// deve esistere e il decisore deve essere bindato prima che la session parta. È il caso che dimostra la
	// precedenza, e con l'API isolata non sarebbe esprimibile.
	UWorld* World = RTWorldFixtures::MakeWorld();

	// Il manager esiste prima della session: e' cosi' che un test binda PRIMA e vince.
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	int32 Interrogato = 0;
	TM->ReactionDecider.BindLambda([&Interrogato](const FRTReactionOpportunity&, int32) -> FString
	{
		++Interrogato;
		return URTReactionOpportunityLibrary::HoldResponse();
	});

	FRTTestScenario Scenario;
	Scenario.ScenarioId = TEXT("Internal.TestDeciderWins");
	Scenario.MapRadius = 4;
	FRTScenarioUnit Sola;
	Sola.Id = TEXT("Sola"); Sola.HeroId = FName(TEXT("Hero.Bastion")); Sola.TeamId = 0;
	Sola.Cell = FRTCellId(0, 0, 0);
	Scenario.Units.Add(Sola);
	Scenario.Turns.Add(FRTScenarioTurn());

	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	RTWorldFixtures::DestroyWorld(World);

	TestEqual(TEXT("il referto nomina la sorgente"), Result.DecisionSource, FString(TEXT("test-override")));
	TestEqual(TEXT("la session non ha applicato nulla di suo"), Result.ScriptedDecisionsApplied, 0);
	return true;
}
```

- [ ] **Passo 2 — eseguilo e verifica che fallisca**

Atteso: **errore di compilazione** — `DecisionSource` non esiste su `FRTTestResult`.

- [ ] **Passo 3 — il campo nel RISULTATO (quello nella session esiste dal task 5)**

In `RTTestResult.h`:

```cpp
	/**
	 * Chi ha risposto alle finestre: `scenario`, `test-override`, `none`.
	 *
	 * ⚠️ Sta QUI e non nel TurnLog, ed e' una scelta: al replay serve **quale** decisione — quella e' stato
	 * di gioco e c'e' gia' — non **chi** l'ha fornita, che e' diagnostica. Un campo nuovo in
	 * `FRTTurnLogEntry` muoverebbe i golden per un dato che il replay non legge.
	 */
	FString DecisionSource = TEXT("none");
```

In `RTScenarioSession.cpp`, dove oggi si scrive `DecisionSource` (task 5), copialo nel risultato prima di
restituirlo. In `RTTestReportWriter.cpp`, accanto agli altri campi di root:

```cpp
	Root->SetStringField(TEXT("decisionSource"), Result.DecisionSource);
	Root->SetNumberField(TEXT("scriptedDecisionsApplied"), Result.ScriptedDecisionsApplied);
	Root->SetNumberField(TEXT("scriptedDecisionsUnused"), Result.ScriptedDecisionsUnused);
```

e alza di uno la `SchemaVersion` del referto, con il motivo accanto.

- [ ] **Passo 4 — eseguilo e verifica che passi**

Atteso: `Success`. Rilancia `RefactorTactics.Scenario` e `RefactorTactics.ShowcaseRelay` per intero:
`result.json` cresce di tre campi e non è confrontato per intero da nessuno
(`RTScenarioRunnerTests.cpp:229` lo rilegge solo per esistenza) — se qualcosa cade, quella misura era
sbagliata e va detto invece che aggirato.

- [ ] **Passo 5 — commit**

```bash
git add Source/RefactorTactics/ScenarioHarness/ Source/RefactorTactics/Tests/RTShowcaseScenarioTests.cpp
git commit -m "feat(512): con due sorgenti, quale ha parlato si scrive invece di dedurlo"
```

---

## Task 8: la verifica di mutazione

Il DoD chiede che «sostituendo il provider con uno che restituisce un **esito**, cada almeno uno scenario».
La firma vieta già un esito, quindi quel test non avrebbe una premessa costruibile — lo stesso caso del test
rimosso in `DeriveOpportunityId`, tenuto verde pur non potendo fallire. La verifica diventa
**comportamentale**: prova che il turno *dipende* dalla decisione.

**Files:**
- Test: `Source/RefactorTactics/Tests/RTShowcaseScenarioTests.cpp`

- [ ] **Passo 1 — scrivi il test**

```cpp
/**
 * Mutazione: lo stesso scenario, con un provider che risponde sempre `HOLD`, deve dare un esito DIVERSO.
 * Se non lo desse, il turno sarebbe verde comunque e la decisione non conterebbe — che e' precisamente cio'
 * che il golden replay di `#170` ha bisogno di escludere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseDecisionMutationTest,
	"RefactorTactics.ShowcaseRelay.DecisionsChangeTheOutcome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseDecisionMutationTest::RunTest(const FString&)
{
	// Lo stesso allestimento del test `DecisionProviderIsInjectable`, costruito da una lambda per non
	// duplicarlo: due esecuzioni che differiscono per UNA cosa sola.
	auto Costruisci = [](bool bScriptaFire)
	{
		FRTTestScenario S;
		S.ScenarioId = TEXT("Internal.DecisionsChangeTheOutcome");
		S.MapRadius = 4;
		auto U = [](const TCHAR* Id, const TCHAR* Hero, int32 Team, const FRTCellId& C)
		{
			FRTScenarioUnit X; X.Id = Id; X.HeroId = FName(Hero); X.TeamId = Team; X.Cell = C; return X;
		};
		S.Units.Add(U(TEXT("Guardia"), TEXT("Hero.Bastion"), 0, FRTCellId( 2, 0, 0)));
		S.Units.Add(U(TEXT("Corsa"),   TEXT("Hero.Vektor"),  1, FRTCellId(-2, 0, 0)));

		FRTScenarioTurn T;
		T.Requires.Add(TEXT("DecisionBoundary"));
		FRTScenarioIntent Arma; Arma.UnitId = TEXT("Guardia"); Arma.Ability = FName(TEXT("Action.Overwatch"));
		T.Intents.Add(Arma);
		FRTScenarioIntent Corre; Corre.UnitId = TEXT("Corsa");
		Corre.Move.Add(FRTCellId(-1, 0, 0)); Corre.Move.Add(FRTCellId(0, 0, 0));
		T.Intents.Add(Corre);

		FRTScenarioDecision D;
		D.Unit = TEXT("Guardia");
		D.Respond = bScriptaFire ? TEXT("FIRE") : TEXT("HOLD");
		if (bScriptaFire) { D.Target = TEXT("Corsa"); }
		T.Decisions.Add(D);
		S.Turns.Add(T);
		return S;
	};

	// Due mondi distinti e isolati: se condividessero il mondo, la seconda esecuzione partirebbe dallo stato
	// lasciato dalla prima e la differenza fra i due hash non direbbe piu' nulla sulla decisione.
	const FRTTestResult ConFire = RTWorldFixtures::RunScenarioIsolated(Costruisci(true));
	const FRTTestResult ConHold = RTWorldFixtures::RunScenarioIsolated(Costruisci(false));

	TestTrue(TEXT("entrambe le esecuzioni hanno applicato la loro decisione"),
		ConFire.ScriptedDecisionsApplied == 1 && ConHold.ScriptedDecisionsApplied == 1);

	// La mutazione: `FIRE` tronca il movimento residuo, `HOLD` no. Se i due hash coincidono, la decisione
	// non ha cambiato niente e il test di sopra sarebbe verde a vuoto.
	TestNotEqual(TEXT("FIRE e HOLD producono stati diversi"), ConFire.StateHash, ConHold.StateHash);
	return true;
}
```

- [ ] **Passo 2 — eseguilo**

`RefactorTactics.ShowcaseRelay.DecisionsChangeTheOutcome`.

Se `TestNotEqual` è **rosso**, i due hash coincidono: la decisione non sta arrivando al resolver, oppure il
`FIRE` non tronca. Non rilassare l'asserzione — è l'unica che dimostra che il lavoro serve. Confronta i due
TurnLog e cerca la voce della decisione.

- [ ] **Passo 3 — commit**

```bash
git add Source/RefactorTactics/Tests/RTShowcaseScenarioTests.cpp
git commit -m "test(512): la mutazione che prova che la decisione conta, invece del divieto che il tipo gia' impone"
```

---

## Task 9: una risposta rifiutata non passa per un `HOLD`

La legalità la verifica `AskReactionDecision` con `IsResponseAllowed`, e **deve restare lì**: due politiche
in due posti possono divergere, e il codice lo dichiara per esteso. Ma se il manager rifiuta la risposta
scriptata produce `HoldRejected`, che nel TurnLog è indistinguibile da una scelta legittima di non sparare.
La session deve leggerlo e farlo cadere.

**Files:**
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp`
- Test: `Source/RefactorTactics/Tests/RTShowcaseScenarioTests.cpp`

- [ ] **Passo 1 — scrivi il test che fallisce**

```cpp
/**
 * Una risposta scriptata RIFIUTATA dal manager non deve passare per un `HOLD` qualunque: `HoldRejected` e
 * `HoldChosen` hanno lo stesso effetto sul gioco e significati opposti per chi legge il referto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShowcaseDecisionRejectedTest,
	"RefactorTactics.ShowcaseRelay.RejectedScriptedResponseFailsTheScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShowcaseDecisionRejectedTest::RunTest(const FString&)
{
	// `Corsa` non e' fra i bersagli offerti dalla finestra di `Guardia` — e' un'unita' della STESSA squadra
	// del guardiano, quindi `FIRE:<lei>` non e' fra le `AllowedResponses` e il manager la rifiuta.
	FRTTestScenario Scenario;
	Scenario.ScenarioId = TEXT("Internal.RejectedScriptedResponse");
	Scenario.MapRadius = 4;
	auto U = [](const TCHAR* Id, const TCHAR* Hero, int32 Team, const FRTCellId& C)
	{
		FRTScenarioUnit X; X.Id = Id; X.HeroId = FName(Hero); X.TeamId = Team; X.Cell = C; return X;
	};
	Scenario.Units.Add(U(TEXT("Guardia"), TEXT("Hero.Bastion"), 0, FRTCellId( 2, 0, 0)));
	Scenario.Units.Add(U(TEXT("Alleato"), TEXT("Hero.Flux"),    0, FRTCellId( 2, 1, 0)));
	Scenario.Units.Add(U(TEXT("Corsa"),   TEXT("Hero.Vektor"),  1, FRTCellId(-2, 0, 0)));

	FRTScenarioTurn T;
	T.Requires.Add(TEXT("DecisionBoundary"));
	FRTScenarioIntent Arma; Arma.UnitId = TEXT("Guardia"); Arma.Ability = FName(TEXT("Action.Overwatch"));
	T.Intents.Add(Arma);
	FRTScenarioIntent Corre; Corre.UnitId = TEXT("Corsa");
	Corre.Move.Add(FRTCellId(-1, 0, 0)); Corre.Move.Add(FRTCellId(0, 0, 0));
	T.Intents.Add(Corre);

	FRTScenarioDecision D;
	D.Unit = TEXT("Guardia"); D.Respond = TEXT("FIRE"); D.Target = TEXT("Alleato"); // non offerto
	T.Decisions.Add(D);
	Scenario.Turns.Add(T);

	const FRTTestResult Result = RTWorldFixtures::RunScenarioIsolated(Scenario);

	TestEqual(TEXT("lo scenario e' in errore"), Result.Outcome, ERTTestOutcome::Error);
	TestTrue(TEXT("il messaggio nomina la decisione rifiutata"),
		Result.ErrorMessage.Contains(TEXT("Alleato")));
	return true;
}
```

- [ ] **Passo 2 — eseguilo e verifica che fallisca**

`RefactorTactics.ShowcaseRelay.RejectedScriptedResponseFailsTheScenario`.
Atteso: `Outcome` è `Blocked` (la capability è ancora chiusa in fase A) o `Pass`, non `Error` — il rifiuto
passa in silenzio.

⚠️ Se il **loader** rifiuta già lo scenario, la premessa del test è sbagliata: `Alleato` è schierato, quindi
la validazione dei nomi del task 2 lo accetta. Se invece la finestra non si apre affatto, vale l'avvertenza
del task 5 sulla geometria.

- [ ] **Passo 3 — leggi l'esito dal TurnLog**

A fine turno, prima del controllo sul residuo:

```cpp
	// `HoldRejected` significa «il manager ha rifiutato la risposta»: sul gioco ha lo stesso effetto di un
	// `HOLD`, e nel referto deve avere il significato opposto. Si legge dal TurnLog invece di duplicare
	// `IsResponseAllowed` qui: la legalita' resta decisa in UN posto solo.
	for (const FRTTurnLogEntry& Entry : Result.TurnLog)
	{
		if (static_cast<ERTReactionDecisionOutcome>(Entry.Outcome) != ERTReactionDecisionOutcome::HoldRejected)
		{
			continue;
		}
		Result.Outcome = ERTTestOutcome::Error;
		Result.ErrorMessage = FString::Printf(
			TEXT("turno %d: la risposta scriptata '%s' e' stata rifiutata dal resolver — il bersaglio non era "
			     "fra quelli offerti dalla finestra"),
			TurnIndex + 1, *Result.LastScriptedResponse);
		Notes.Add(Result.ErrorMessage);
		break;
	}
```

⚠️ **Verifica i nomi del TurnLog prima di scrivere**: `grep -n "struct FRTTurnLogEntry" -A 30
Source/RefactorTactics/Turn/RTTurnLog.h` per il nome del campo (`Outcome`, `uint8`) e per come si distingue
una voce di decisione dalle altre. Il ciclo qui sopra assume che `Outcome` sia significativo solo su quelle
voci: se non è così, filtra prima per categoria. ⛔ `RTTurnLog.h` **non è nel write-set** — si legge, non si
tocca.

- [ ] **Passo 4 — eseguilo e verifica che passi**

Atteso: `Success`. Rilancia `DecisionProviderIsInjectable` e `ScriptedDecisionsAreConsumedInOrder`: nessuno
dei due deve diventare rosso — se lo diventa, le loro risposte non erano legali e i test passavano per il
motivo sbagliato. **È l'informazione più utile che questo task possa produrre.**

- [ ] **Passo 5 — commit**

```bash
git add Source/RefactorTactics/ScenarioHarness/ Source/RefactorTactics/Tests/RTShowcaseScenarioTests.cpp
git commit -m "feat(512): una risposta rifiutata smette di essere indistinguibile da un HOLD scelto"
```

---

## Task 10: la divisione di `Reaction`

È la sesta voce del DoD, ed è l'unica che non tocca comportamento: il commento di `AvailableCapabilities()`
si è scritto da solo la propria condizione di scadenza — *«il giorno in cui `ResolveCombat` chiamerà quella
funzione, questa riga smette di essere vera e va divisa»* — e quel giorno è arrivato.

**Files:**
- Modify: `Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp:40-53`
- Test: `Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp`

- [ ] **Passo 1 — rimisura il grep che il commento nomina, invece di fidarti della prosa**

```bash
grep -rn "BuildOverwatchTriggers" Source/ --include=*.cpp | grep -v /Tests/
```

Atteso: **cinque** righe, fra cui `RTTurnManager.cpp:5093`, che è di **produzione**. Il commento afferma
«nessun chiamante»: è quell'affermazione a essere scaduta. Se il grep desse zero righe, **fermati**: la
premessa della divisione non regge e la voce di DoD va rimessa in discussione, non spuntata.

- [ ] **Passo 2 — scrivi il test che pinna i due significati**

```cpp
/**
 * `Reaction` e `DecisionBoundary` sono due nomi con due regimi, e il confine e' la CARDINALITA' — non due
 * nomi per la stessa cosa. Il test esiste perche' la divisione, senza, vive solo in un commento: e un
 * commento che dichiara la propria scadenza l'ha gia' mancata una volta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioReactionSplitTest,
	"RefactorTactics.Scenario.ReactionAndDecisionBoundaryAreDistinct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioReactionSplitTest::RunTest(const FString&)
{
	// `Reaction` copre il regime E5 `AllowedResponses <= 1` ed e' DISPONIBILE.
	TestTrue(TEXT("Reaction e' disponibile"),
		FRTScenarioSession::IsCapabilityKnown(TEXT("Reaction")));
	// `DecisionBoundary` copre `>= 2` e in fase A e' ancora INDISPONIBILE: si scopre insieme ai dati che la
	// rendono rispondibile, che e' fase B.
	TestTrue(TEXT("DecisionBoundary e' un nome noto"),
		FRTScenarioSession::IsCapabilityKnown(TEXT("DecisionBoundary")));
	// E i tre scenari che chiedono `Reaction` restano nel regime E5: se uno di loro passasse a chiedere una
	// decisione, il nome che chiede dovrebbe cambiare con lui.
	TestFalse(TEXT("un nome inventato resta un errore, non un Blocked"),
		FRTScenarioSession::IsCapabilityKnown(TEXT("ReactionDecision")));
	return true;
}
```

⚠️ `IsCapabilityKnown` è la funzione a `RTScenarioSession.cpp:266` che unisce i due insiemi: **verifica come
è esposta** (`grep -n "IsCapabilityKnown" RTScenarioSession.h`). Se è interna al `.cpp`, esponila
nell'header — è nel write-set — invece di duplicare gli elenchi nel test.

- [ ] **Passo 3 — riscrivi il commento scaduto**

In `AvailableCapabilities()`, al posto delle righe che dichiarano «nessun chiamante»:

```cpp
			// E5: reazioni componibili che scattano o non scattano — il regime `AllowedResponses <= 1` di
			// ADR-0004 §2, e **solo quello**.
			//
			// ✅ **Diviso con `#512`, il 2026-08-16, alla condizione che questa riga si era scritta da sola.**
			// Diceva: «il giorno in cui `ResolveCombat` chiamera' `BuildOverwatchTriggers`, questa riga smette
			// di essere vera e va divisa». Quel giorno e' arrivato con CP 14.5 — `RTTurnManager.cpp:5093` e' un
			// chiamante di produzione, e il grep che il commento nominava da' oggi **cinque** righe.
			//
			// La divisione e' fra i due nomi che esistono gia', non con un terzo: la DECISIONE su
			// un'opportunity a due risposte e' `DecisionBoundary`. Misurato che i tre turni che chiedono
			// `Reaction` (`Combat/CounterStrikesBack`, `Visual/Reaction/Deflection`,
			// `Visual/Reaction/Interposition`) sono tutti nel regime `<= 1`: un nome nuovo li costringerebbe a
			// cambiare senza cambiare cio' che chiedono.
			TEXT("Reaction"),
```

e, nella motivazione di `DecisionBoundary` in `KnownUnavailableCapabilities()`, sostituisci «la finestra da
3 s è CP 14.5» — chiusa — con la ragione **vera** per cui resta indisponibile in fase A:

```cpp
			// ⚠️ La ragione NON e' piu' «manca il produttore»: CP 14.5 e' chiusa e la finestra si apre in
			// partita. Resta indisponibile perche' scoprirla sblocca tre turni, e due di essi
			// (`Spec/Overwatch/HoldThenFire`, `Spec/Brace/ProfileChangesResponse`) non hanno ancora le
			// `decisions` che li rendono rispondibili: passerebbero da `BLOCKED`, che
			// `EveryShippedScenarioRuns` accetta, a `FAIL`, che non accetta. Si scopre **insieme** ai dati.
			TEXT("DecisionBoundary"),
```

- [ ] **Passo 4 — eseguilo, e poi il corpus**

`RefactorTactics.Scenario.ReactionAndDecisionBoundaryAreDistinct` → `Success`.
Poi `RefactorTactics.Scenario` per intero: **nessuno scenario deve cambiare esito**. Questo task non muove
comportamento — se un file passa da `BLOCKED` a qualcos'altro, hai scoperto la capability per sbaglio.

- [ ] **Passo 5 — i due `--check`**, poi commit

```bash
python scripts/feature_registry.py generate --check
python scripts/feature_registry.py shortlist --check
git add Source/RefactorTactics/ScenarioHarness/ Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp
git commit -m "feat(512): Reaction si divide alla condizione che il suo commento aveva gia' scritto"
```

---

## Chiusura della fase A

- [ ] **Suite completa, non un filtro.** `Automation RunTests RefactorTactics` per intero, e **conta le
  dichiarazioni prima e dopo**: `grep -rc "IMPLEMENT_.*_AUTOMATION_TEST" Source/RefactorTactics/Tests/ | ...`
  Il totale deve crescere **esattamente** del numero di test aggiunti. È la verifica che il verde non arrivi
  da test scomparsi.
- [ ] **Guardia**: `grep -c "WITH_DEV_AUTOMATION_TESTS" ` su entrambi i file di test toccati, e verifica che
  ogni `IMPLEMENT_` nuovo stia **dentro** — è `#923`, e la suite gira sul target Editor che non lo vedrebbe.
- [ ] **Build Shipping**: `Build.bat RefactorTactics Win64 Shipping` → `Result: Succeeded`. Non c'è CI, e
  senza questo passo il difetto del 2026-08-09 torna una terza volta.
- [ ] **I sei gate documentali**, dopo `git add`: se il totale di `check-docs-links.py` non si muove, i file
  nuovi non sono stati controllati.
- [ ] **PR verso `main`**, e nel corpo **dichiara ciò che non chiude**: la voce 5 del DoD è di fase B, perché
  scoprire `DecisionBoundary` senza i dati farebbe cadere `Spec/Overwatch/HoldThenFire`.
- [ ] **Aggiorna il batch**: `simulation` torna `IDLE` con `issue: null` **se** #512 resta aperta per la sola
  fase B, e il `writable` esce come per le chiusure precedenti.
- [ ] **Consuntiva il DoD nel commento della issue**, non nel body: le spunte nel body non sono un segnale.
