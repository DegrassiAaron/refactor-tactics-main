# Scudo base e sorgente del danno — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or
> superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for
> tracking.

**Goal:** Ogni unità ha 5 punti di scudo base che si ricaricano a fine turno e fermano solo il danno diretto;
`Action.Shield` smette di essere irraggiungibile ed entra nei kit di Phase e Wraith.

**Architecture:** `ApplyDamage` guadagna la sorgente del danno e la quota di scudo temporaneo, così può
erodere temporaneo → base → salute saltando la base quando la sorgente è ambientale. Lo scudo base non è un
campo nuovo: è ciò che avanza da `Shield − TemporaryShield`, e una costante di `URTCombatLibrary` ne fissa il
valore. La ricarica è una riga in coda al `Cleanup`, dove il temporaneo scade già.

**Tech Stack:** Unreal Engine 5.8, C++, Automation Testing framework (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`).

**Spec:** [`../specs/2026-08-28-scudo-base-e-sorgente-danno-design.md`](../specs/2026-08-28-scudo-base-e-sorgente-danno-design.md)

## Global Constraints

- UE **5.8.1**. Motore in `D:\EpicGames\UE_5.8`.
- Build: `D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat RefactorTacticsEditor Win64 Development -Project="D:\Repositories\refactor-tactict-dev\RefactorTactics.uproject" -WaitMutex`
- Suite: `./scripts/rt-suite.ps1 -Filter <filtro>` da **PowerShell**, mai da Bash (MSYS traduce i path).
  Lo script dichiara `NON VALIDA` una run in cui HEAD, albero, binario o processi del motore sono cambiati.
- ⛔ **Prima di lanciare la suite**: `Get-Process -Name UnrealEditor-Cmd` deve essere vuoto. Due run di
  automation si uccidono a vicenda anche da checkout diversi — il mutex è globale sull'eseguibile.
- ⛔ Niente commit senza richiesta esplicita dell'utente (`CLAUDE.md`). I passi «Commit» di questo piano si
  eseguono **solo** se l'utente lo ha chiesto per quella sessione.
- Leggere `git branch --show-current` **subito prima** di ogni commit: altre sessioni condividono questa
  working directory e un `checkout` altrui sposta il branch sotto il lavoro in corso.
- Lingua di commenti e messaggi: **italiano**; identificatori in inglese.
- Il progetto usa `TestEqual`/`TestTrue` di `FAutomationTestBase`. ⚠️ L'automation **ignora il valore di
  ritorno** di `RunTest`: un `return false` anticipato riporta comunque `Success`. Ogni uscita anticipata
  deve essere preceduta da un `TestTrue` che fallisce.

---

### Task 1: La sorgente del danno

Refactor a **comportamento invariato**: finché lo scudo base è 0 (cioè fino al Task 2), `Direct` ed
`Environmental` producono risultati identici, perché non c'è base da saltare. La prova è che il corpus
golden non cambia.

**Files:**
- Modify: `Source/RefactorTactics/Combat/RTCombatLibrary.h` — enum, campo su `FRTDamageResult`, firma
- Modify: `Source/RefactorTactics/Combat/RTCombatLibrary.cpp:6-18` — implementazione
- Modify: `Source/RefactorTactics/Combat/RTCombatResolver.cpp:21`
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.cpp` — righe 177, 1391, 2346, 4749, 5118
- Test: `Source/RefactorTactics/Tests/RTCombatLibraryTests.cpp` — 4 chiamate esistenti + 5 test nuovi
- Test: `Source/RefactorTactics/Tests/RTFacingDefenseTests.cpp:200-201` — 2 chiamate esistenti

**Interfaces:**
- Produces: `ERTDamageSource { Direct, Environmental }`;
  `URTCombatLibrary::ApplyDamage(int32 Damage, ERTDamageSource Source, int32 Shield, int32 TemporaryShield, int32 Health) -> FRTDamageResult`;
  `FRTDamageResult { int32 Health; int32 Shield; int32 TemporaryShield; }`

- [ ] **Step 1: Dichiarare l'enum e il campo del risultato**

In `RTCombatLibrary.h`, sopra `FRTDamageResult`:

```cpp
/**
 * Da dove viene il danno. Decide se lo scudo BASE partecipa all'assorbimento: il cuscinetto passivo
 * ferma i colpi, non gli hazard. Lo scudo TEMPORANEO assorbe entrambi — quello te lo sei costruito.
 */
UENUM(BlueprintType)
enum class ERTDamageSource : uint8
{
	Direct,         // colpi: Blast, boundary, Overwatch
	Environmental   // Burning, terreno, propagazione elettrica
};
```

E dentro `FRTDamageResult`, dopo `Shield`:

```cpp
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 TemporaryShield = 0;

	FRTDamageResult() = default;
	FRTDamageResult(int32 InHealth, int32 InShield, int32 InTemporaryShield = 0)
		: Health(InHealth), Shield(InShield), TemporaryShield(InTemporaryShield) {}
```

- [ ] **Step 2: Scrivere i test che falliscono**

In `RTCombatLibraryTests.cpp`, dopo `FRTDamageNoShieldTest`:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageDirectErodesTemporaryFirstTest,
	"RefactorTactics.Combat.DirectDamageErodesTemporaryShieldBeforeBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageDirectErodesTemporaryFirstTest::RunTest(const FString&)
{
	// 5 base + 25 temporaneo = 30. Dieci danni diretti escono TUTTI dal temporaneo.
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(
		10, ERTDamageSource::Direct, /*Shield*/ 30, /*TemporaryShield*/ 25, /*Health*/ 100);
	TestEqual(TEXT("scudo totale 20"), R.Shield, 20);
	TestEqual(TEXT("temporaneo 15"), R.TemporaryShield, 15);
	TestEqual(TEXT("la base non e' stata toccata"), R.Shield - R.TemporaryShield, 5);
	TestEqual(TEXT("HP intatti"), R.Health, 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageDirectConsumesBaseAfterTemporaryTest,
	"RefactorTactics.Combat.DirectDamageConsumesBaseOnceTemporaryIsGone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageDirectConsumesBaseAfterTemporaryTest::RunTest(const FString&)
{
	// 5 base + 25 temporaneo, 28 danni diretti: 25 dal temporaneo, 3 dalla base, 0 agli HP.
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(
		28, ERTDamageSource::Direct, /*Shield*/ 30, /*TemporaryShield*/ 25, /*Health*/ 100);
	TestEqual(TEXT("temporaneo esaurito"), R.TemporaryShield, 0);
	TestEqual(TEXT("base ridotta a 2"), R.Shield, 2);
	TestEqual(TEXT("HP intatti"), R.Health, 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageEnvironmentalSkipsBaseTest,
	"RefactorTactics.Combat.EnvironmentalDamageSkipsBaseShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageEnvironmentalSkipsBaseTest::RunTest(const FString&)
{
	// 5 di sola base, 8 di Burning: la base NON assorbe, tutti e 8 arrivano agli HP.
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(
		URTCombatLibrary::BurningCleanupDamage, ERTDamageSource::Environmental,
		/*Shield*/ 5, /*TemporaryShield*/ 0, /*Health*/ 100);
	TestEqual(TEXT("la base resta intera"), R.Shield, 5);
	TestEqual(TEXT("HP 92"), R.Health, 92);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageEnvironmentalErodesTemporaryTest,
	"RefactorTactics.Combat.EnvironmentalDamageStillErodesTemporaryShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageEnvironmentalErodesTemporaryTest::RunTest(const FString&)
{
	// 5 base + 25 temporaneo, 8 di Burning: il temporaneo assorbe, la base resta, gli HP pure.
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(
		URTCombatLibrary::BurningCleanupDamage, ERTDamageSource::Environmental,
		/*Shield*/ 30, /*TemporaryShield*/ 25, /*Health*/ 100);
	TestEqual(TEXT("temporaneo 17"), R.TemporaryShield, 17);
	TestEqual(TEXT("la base resta intera"), R.Shield - R.TemporaryShield, 5);
	TestEqual(TEXT("HP intatti"), R.Health, 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageEnvironmentalCanKillThroughBaseTest,
	"RefactorTactics.Combat.EnvironmentalDamageCanKillThroughBaseShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageEnvironmentalCanKillThroughBaseTest::RunTest(const FString&)
{
	// Il caso che la decisione 4 esiste per preservare: 5 di base non salvano da un Burning letale.
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(
		URTCombatLibrary::BurningCleanupDamage, ERTDamageSource::Environmental,
		/*Shield*/ 5, /*TemporaryShield*/ 0, /*Health*/ 6);
	TestEqual(TEXT("HP a zero"), R.Health, 0);
	TestEqual(TEXT("lo scudo base e' ancora li', e non ha salvato nessuno"), R.Shield, 5);
	return true;
}
```

- [ ] **Step 3: Verificare che non compili**

Run (PowerShell):
```
D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat RefactorTacticsEditor Win64 Development -Project="D:\Repositories\refactor-tactict-dev\RefactorTactics.uproject" -WaitMutex
```
Expected: FAIL — `ApplyDamage` non accetta 5 argomenti.

- [ ] **Step 4: Cambiare la firma e l'implementazione**

In `RTCombatLibrary.h`, sostituire la dichiarazione di `ApplyDamage`:

```cpp
	/**
	 * Applica il danno: erode il temporaneo, poi — solo se la sorgente e' DIRETTA — lo scudo base, poi gli
	 * HP. Nessun valore scende sotto 0.
	 *
	 * `TemporaryShield` non e' ridondante con `Shield`: e' la QUOTA di `Shield` che scade nel Cleanup, e
	 * senza di essa la funzione non puo' sapere quanta protezione e' base — cioe' quanta ne deve saltare
	 * quando il danno viene dall'ambiente.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static FRTDamageResult ApplyDamage(int32 Damage, ERTDamageSource Source, int32 Shield,
		int32 TemporaryShield, int32 Health);
```

E la costante, accanto a `BurningCleanupDamage`:

```cpp
	/**
	 * Scudo BASE di ogni unita' (D-224): 5 punti che non crescono e tornano pieni a fine turno.
	 *
	 * Sta qui e non in `Config/` perche' non e' un parametro di formato come `RoundLimit`: e' una regola del
	 * combattimento, e le altre nove vivono in questa stessa lista.
	 */
	static constexpr int32 BaseShield = 5;
```

In `RTCombatLibrary.cpp`, sostituire il corpo:

```cpp
FRTDamageResult URTCombatLibrary::ApplyDamage(int32 Damage, ERTDamageSource Source, int32 Shield,
	int32 TemporaryShield, int32 Health)
{
	const int32 SafeDamage = FMath::Max(0, Damage);
	const int32 SafeShield = FMath::Max(0, Shield);
	// Clamp e non Max: un temporaneo maggiore del totale renderebbe la base negativa, e da li' in poi
	// l'aritmetica direbbe cose false in silenzio.
	const int32 SafeTemp   = FMath::Clamp(TemporaryShield, 0, SafeShield);
	const int32 SafeBase   = SafeShield - SafeTemp;

	// 1 — il temporaneo assorbe per primo, qualunque sia la sorgente: sta per scadere comunque.
	const int32 FromTemp = FMath::Min(SafeDamage, SafeTemp);
	int32 Remaining      = SafeDamage - FromTemp;
	const int32 NewTemp  = SafeTemp - FromTemp;

	// 2 — la base assorbe SOLO il danno diretto (D-224): Burning, terreno e propagazione la attraversano.
	const int32 FromBase = (Source == ERTDamageSource::Direct)
	                     ? FMath::Min(Remaining, SafeBase)
	                     : 0;
	Remaining           -= FromBase;
	const int32 NewBase  = SafeBase - FromBase;

	const int32 NewHealth = FMath::Max(0, Health - Remaining);

	return FRTDamageResult(NewHealth, NewBase + NewTemp, NewTemp);
}
```

- [ ] **Step 5: Aggiornare i sei chiamanti di produzione**

`RTCombatResolver.cpp:21` — i colpi del Blast sono diretti per costruzione:

```cpp
		const FRTDamageResult Damaged = URTCombatLibrary::ApplyDamage(
			Pair.Value, ERTDamageSource::Direct, Initial.Shield, Initial.TemporaryShield, Initial.Health);
		Result[Pair.Key] = FRTUnitCombatState(Damaged.Health, Damaged.Shield, Damaged.TemporaryShield);
```

⚠️ Questo richiede il campo su `FRTUnitCombatState` — vedi Step 6.

`RTTurnManager.cpp`, cinque punti, ognuno con la sua sorgente:

| Riga | Sostituzione |
|---|---|
| 177 | `ApplyDamage(Effect.Amount, ERTDamageSource::Environmental, Unit->Shield, Unit->GetTemporaryShield(), Unit->Health)` — è il danno da **terreno** |
| 1391 | `ApplyDamage(URTCombatLibrary::BurningCleanupDamage, ERTDamageSource::Environmental, Unit->Shield, Unit->GetTemporaryShield(), Unit->Health)` |
| 2346 | `ApplyDamage(Hit.Damage, ERTDamageSource::Environmental, Victim->Shield, Victim->GetTemporaryShield(), Victim->Health)` — propagazione elettrica |
| 4749 | `ApplyDamage(Dealt, ERTDamageSource::Direct, Victim->Shield, Victim->GetTemporaryShield(), Victim->Health)` — colpo al boundary |
| 5118 | `ApplyDamage(Dealt, ERTDamageSource::Direct, Target->Shield, Target->GetTemporaryShield(), Target->Health)` — Overwatch |

Le chiamate a `ApplyCombatState(Result.Health, Result.Shield)` **restano invariate**: quella funzione
ricalcola già `TemporaryShield` dalla differenza di scudo, e cambiarla qui duplicherebbe la contabilità.

- [ ] **Step 6: Aggiungere TemporaryShield a FRTUnitCombatState**

In `Source/RefactorTactics/Combat/RTCombatResolver.h`:

```cpp
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Combat")
	int32 TemporaryShield = 0;

	FRTUnitCombatState() = default;
	FRTUnitCombatState(int32 InHealth, int32 InShield, int32 InTemporaryShield = 0)
		: Health(InHealth), Shield(InShield), TemporaryShield(InTemporaryShield) {}
```

Il default a 0 tiene compilanti i costruttori a due argomenti già presenti nei test.

- [ ] **Step 7: Aggiornare le sei chiamate nei test esistenti**

`RTCombatLibraryTests.cpp` righe 16, 28, 40, 51 e `RTFacingDefenseTests.cpp` righe 200-201: inserire
`ERTDamageSource::Direct` come secondo argomento e `0` come `TemporaryShield`. Esempio (riga 16):

```cpp
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(30, ERTDamageSource::Direct, 20, 0, 100);
```

Gli asserti **non cambiano**: senza scudo base la vecchia e la nuova aritmetica coincidono. È la prova che
questo task non altera il comportamento.

- [ ] **Step 8: Compilare e girare i test di combattimento**

Run (PowerShell):
```
D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat RefactorTacticsEditor Win64 Development -Project="D:\Repositories\refactor-tactict-dev\RefactorTactics.uproject" -WaitMutex
./scripts/rt-suite.ps1 -Filter RefactorTactics.Combat
```
Expected: build `Succeeded`; suite `VALIDA` e verde, nuovi test inclusi.

- [ ] **Step 9: Verificare che il corpus golden non sia cambiato**

Run: `git status --short Content/`
Expected: **vuoto**. Questo task è a comportamento invariato; se il corpus cambia, l'aritmetica è stata
alterata e va trovato dove prima di proseguire.

- [ ] **Step 10: Commit** *(solo se l'utente ha autorizzato i commit)*

```bash
git branch --show-current
git add Source/RefactorTactics/Combat Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Tests
git commit -m "feat(D-224): il danno dichiara da dove viene, e ApplyDamage sa quanto scudo scade"
```

---

### Task 2: Lo scudo base

**Files:**
- Modify: `Source/RefactorTactics/Unit/RTUnit.h` — dichiarazione di `RechargeBaseShield`
- Modify: `Source/RefactorTactics/Unit/RTUnit.cpp` — implementazione + `BeginPlay`
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.cpp:1502` — la ricarica in coda al Cleanup
- Test: `Source/RefactorTactics/Tests/RTUnitTests.cpp`

**Interfaces:**
- Consumes: `URTCombatLibrary::BaseShield` (Task 1)
- Produces: `ARTUnit::RechargeBaseShield()`

- [ ] **Step 1: Scrivere i test che falliscono**

In `RTUnitTests.cpp`:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitRechargesBaseShieldTest,
	"RefactorTactics.Unit.RechargeRestoresBaseShieldToFive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitRechargesBaseShieldTest::RunTest(const FString&)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	TestNotNull(TEXT("mondo creato"), World);
	if (!World) { return false; }

	ARTUnit* Unit = World->SpawnActor<ARTUnit>();
	TestNotNull(TEXT("unita' creata"), Unit);
	if (!Unit) { return false; }

	Unit->ApplyCombatState(/*Health*/ 100, /*Shield*/ 2);   // base erosa a 2
	Unit->RechargeBaseShield();
	TestEqual(TEXT("la base torna a 5"), Unit->Shield, URTCombatLibrary::BaseShield);
	TestEqual(TEXT("nessun temporaneo introdotto"), Unit->GetTemporaryShield(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitDeadDoesNotRechargeTest,
	"RefactorTactics.Unit.DefeatedUnitDoesNotRechargeShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitDeadDoesNotRechargeTest::RunTest(const FString&)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	TestNotNull(TEXT("mondo creato"), World);
	if (!World) { return false; }

	ARTUnit* Unit = World->SpawnActor<ARTUnit>();
	TestNotNull(TEXT("unita' creata"), Unit);
	if (!Unit) { return false; }

	Unit->ApplyCombatState(/*Health*/ 0, /*Shield*/ 0);
	Unit->RechargeBaseShield();
	TestEqual(TEXT("un'unita' abbattuta non si ricarica"), Unit->Shield, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitRechargeKeepsTemporaryTest,
	"RefactorTactics.Unit.RechargeAddsBaseOnTopOfTemporaryShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitRechargeKeepsTemporaryTest::RunTest(const FString&)
{
	// La ricarica e' scritta per reggere anche se un giorno girasse PRIMA della scadenza del temporaneo.
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	TestNotNull(TEXT("mondo creato"), World);
	if (!World) { return false; }

	ARTUnit* Unit = World->SpawnActor<ARTUnit>();
	TestNotNull(TEXT("unita' creata"), Unit);
	if (!Unit) { return false; }

	Unit->ApplyCombatState(/*Health*/ 100, /*Shield*/ 0);
	Unit->AddTemporaryShield(25);
	Unit->RechargeBaseShield();
	TestEqual(TEXT("base 5 sopra i 25 temporanei"), Unit->Shield, 30);
	TestEqual(TEXT("il temporaneo non e' stato toccato"), Unit->GetTemporaryShield(), 25);
	return true;
}
```

⚠️ Se `RTUnitTests.cpp` non include già `FAutomationEditorCommonUtils`, seguire il pattern di spawn già usato
nel file invece di introdurne uno nuovo. I test esistenti alle righe 292-332 mostrano come il file crea le
unità: **riusare quello**.

- [ ] **Step 2: Verificare che non compili**

Run: la riga di build dello Step 3 del Task 1.
Expected: FAIL — `RechargeBaseShield` non dichiarata.

- [ ] **Step 3: Implementare la ricarica**

In `RTUnit.h`, accanto a `ExpireTemporaryShield`:

```cpp
	/**
	 * Riporta lo scudo BASE al suo valore pieno (D-224). Chiamata in coda al Cleanup, dove il temporaneo
	 * e' appena scaduto, e da `BeginPlay` perche' un'unita' entra in campo gia' protetta.
	 *
	 * La somma con `TemporaryShield` e' ridondante nella posizione attuale — li' vale sempre 0 — ma tiene
	 * l'invariante `Shield = base + temporaneo` vera se un giorno l'ordine delle due chiamate cambiasse.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Unit")
	void RechargeBaseShield();
```

In `RTUnit.cpp`, sotto `ExpireTemporaryShield`:

```cpp
void ARTUnit::RechargeBaseShield()
{
	// Un'unita' abbattuta non si ricarica: il suo stato logico e' finale finche' il turno non la rimuove.
	if (Health <= 0)
	{
		return;
	}
	Shield = URTCombatLibrary::BaseShield + TemporaryShield;
}
```

Aggiungere `#include "Combat/RTCombatLibrary.h"` a `RTUnit.cpp` se assente.

- [ ] **Step 4: Dare lo scudo iniziale in BeginPlay**

In `ARTUnit::BeginPlay()`, dopo `Super::BeginPlay()`:

```cpp
	// Lo scudo base non e' il default del campo: quel valore lo sovrascriverebbe chiunque costruisca
	// un'unita' a mano, test compresi. Un solo punto lo stabilisce, ed e' lo stesso che lo ripristina.
	RechargeBaseShield();
```

- [ ] **Step 5: Ricaricare in coda al Cleanup**

In `RTTurnManager.cpp:1502`, subito **dopo** `Unit->ExpireTemporaryShield();`:

```cpp
			Unit->RechargeBaseShield(); // D-224: la base torna piena per il turno che comincia
```

- [ ] **Step 6: Compilare e girare i test**

Run:
```
D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat RefactorTacticsEditor Win64 Development -Project="D:\Repositories\refactor-tactict-dev\RefactorTactics.uproject" -WaitMutex
./scripts/rt-suite.ps1 -Filter RefactorTactics.Unit
```
Expected: build `Succeeded`; i tre test nuovi passano.

- [ ] **Step 7: Girare la suite intera e leggere i rossi**

Run: `./scripts/rt-suite.ps1`
Expected: **molti test rossi**, ed è previsto — lo scudo iniziale passa da 0 a 5 e ogni test che assumeva
`Shield == 0` cambia esito. Non «aggiustare i numeri» in blocco: per ognuno decidere se il test misurava lo
scudo (allora il numero atteso cambia) o qualcos'altro che ora è disturbato (allora il test va reso esplicito
sullo scudo che si aspetta). I file più esposti sono `RTUnitTests.cpp`, `RTDefensiveReactionTests.cpp` e i
test del resolver.

- [ ] **Step 8: Commit** *(solo se autorizzato)*

```bash
git branch --show-current
git add Source/RefactorTactics/Unit Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Tests
git commit -m "feat(D-224): ogni unita' entra in campo con 5 di scudo, e li riprende a fine turno"
```

---

### Task 3: `Action.Shield` nei kit di Phase e Wraith

**Files:**
- Modify: `Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp` — dopo la riga 536 (Phase) e la 819 (Wraith)
- Modify: `Source/RefactorTactics/Tests/RTCatalogTests.cpp:501` — rimozione della riga di esclusione
- Test: `Source/RefactorTactics/Tests/RTCatalogTests.cpp`

**Interfaces:**
- Consumes: `URTHeroCatalogLibrary::MakeHeroActionFromCore(HeroActionId, CoreActionId, Cooldown, Shape, AreaRadius)`
- Produces: `Hero.Phase.TideGuard`, `Hero.Wraith.PhaseGuard` — entrambe con
  `Def.DerivedFromActionId == "Action.Shield"`

- [ ] **Step 1: Scrivere il test che fallisce**

In `RTCatalogTests.cpp`:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShieldIsCarriedOncePerTeamTest,
	"RefactorTactics.Catalog.ShieldIsReachableOncePerTeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShieldIsCarriedOncePerTeamTest::RunTest(const FString&)
{
	// D-224: uno scudo proattivo per squadra. Le formazioni sono fisse (RTGameMode.h): Team 0 = Gadget +
	// Phase, Team 1 = Riktor + Wraith. Il test guarda i PORTATORI, non i nomi delle abilita': un rename
	// non deve farlo cadere, un portatore spostato di squadra si'.
	const TMap<FName, int32> SquadraDi = {
		{ TEXT("Hero.Gadget"), 0 }, { TEXT("Hero.Phase"),  0 },
		{ TEXT("Hero.Riktor"), 1 }, { TEXT("Hero.Wraith"), 1 }
	};

	TMap<int32, int32> PortatoriPerSquadra;
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Hero) { continue; }
		const int32* Squadra = SquadraDi.Find(Hero->HeroId);
		TestNotNull(TEXT("ogni eroe del roster ha una squadra dichiarata qui"), Squadra);
		if (!Squadra) { continue; }

		for (const URTActionData* A : Hero->Actions)
		{
			if (A && A->Def.DerivedFromActionId == FName(TEXT("Action.Shield")))
			{
				PortatoriPerSquadra.FindOrAdd(*Squadra)++;
			}
		}
	}

	TestEqual(TEXT("Team 0 ha esattamente un portatore di Action.Shield"),
		PortatoriPerSquadra.FindRef(0), 1);
	TestEqual(TEXT("Team 1 ha esattamente un portatore di Action.Shield"),
		PortatoriPerSquadra.FindRef(1), 1);
	return true;
}
```

- [ ] **Step 2: Verificare che fallisca**

Run: `./scripts/rt-suite.ps1 -Filter RefactorTactics.Catalog.ShieldIsReachableOncePerTeam`
Expected: FAIL — zero portatori per entrambe le squadre.

- [ ] **Step 3: Dare l'azione ai due eroi**

In `RTHeroCatalogLibrary.cpp`, dopo `Hero.Phase.FlowReaction` (riga 536):

```cpp
	// `Hero.Phase.TideGuard` — lo scudo PROATTIVO (D-224). Deriva da `Action.Shield`: Preparation, 25 punti
	// di scudo temporaneo, cooldown 2. E' l'unico scudo del gioco che si sceglie PRIMA di sapere se sarai
	// colpito — gli altri (`ReactiveCapacitor`, `ReactiveShield`) rispondono a un colpo gia' partito.
	Phase->Actions.Add(URTHeroCatalogLibrary::MakeHeroActionFromCore(
		TEXT("Hero.Phase.TideGuard"), TEXT("Action.Shield"), /*Cooldown*/ 2));
```

E dopo `Hero.Wraith.Feint` (riga 819):

```cpp
	// `Hero.Wraith.PhaseGuard` — gemello di `Hero.Phase.TideGuard`, uno per squadra (D-224). Su Wraith
	// costa una scelta vera: la Preparation spesa qui e' quella che non prepara il tiro.
	Wraith->Actions.Add(URTHeroCatalogLibrary::MakeHeroActionFromCore(
		TEXT("Hero.Wraith.PhaseGuard"), TEXT("Action.Shield"), /*Cooldown*/ 2));
```

⚠️ `MakeHeroActionFromCore` ritorna `nullptr` se l'azione core non esiste. Verificare che entrambe le
chiamate producano un puntatore valido — il test dello Step 1 fallirebbe comunque, ma con un messaggio meno
diretto di un `TestNotNull` accanto alla costruzione.

- [ ] **Step 4: Togliere la riga di esclusione**

In `RTCatalogTests.cpp:501`, rimuovere:

```cpp
		{ TEXT("Action.Shield"),          TEXT("Aspetta il suo eroe: adr-0003 la da' per arrivata") },
```

e sostituirla con la nota di provenienza, nella forma già usata per `Action.Purge`:

```cpp
		// `Action.Shield` e' USCITA da questo elenco il 2026-08-28 ([D-224], `#1403`): la portano
		// `Hero.Phase.TideGuard` e `Hero.Wraith.PhaseGuard`, uno per squadra. La riga la toglie il gate
		// stesso, che dice «ORA e' raggiungibile: togli la riga» invece di lasciarla marcire.
```

- [ ] **Step 5: Compilare e girare i test di catalogo**

Run:
```
D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat RefactorTacticsEditor Win64 Development -Project="D:\Repositories\refactor-tactict-dev\RefactorTactics.uproject" -WaitMutex
./scripts/rt-suite.ps1 -Filter RefactorTactics.Catalog
```
Expected: `ShieldIsReachableOncePerTeam` verde e `EveryCoreActionIsReachableOrDeclared` verde **senza** la
riga rimossa. Se quest'ultimo protesta che l'azione è dichiarata *e* raggiungibile, la rimozione dello
Step 4 non è stata fatta.

- [ ] **Step 6: Commit** *(solo se autorizzato)*

```bash
git branch --show-current
git add Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp Source/RefactorTactics/Tests/RTCatalogTests.cpp
git commit -m "feat(D-224): Action.Shield trova i suoi portatori, uno per squadra"
```

---

### Task 4: Corpus, documenti e la riga di decisione

**Files:**
- Modify: `docs/decisions/RT_PDR_00_Decision_Log.md` — riga **D-224**
- Modify: `docs/balance/RT_ActionCatalog_v0.1.md` §4 — `Action.Shield` non è più senza portatore
- Modify: `docs/decisions/adr-0003-modello-azioni-v01.md:167` — la dava «per arrivata»
- Modify: `docs/DOC_CONFLICT_MATRIX.md` — se il catalogo diverge dal codice dopo la modifica
- Regenerate: corpus golden

- [ ] **Step 1: Riverificare il numero di decisione**

Run:
```bash
git fetch --prune origin
gh pr list --state open
grep -o "D-[0-9]\{3\}" docs/decisions/RT_PDR_00_Decision_Log.md | sort -u | tail -3
```
Expected: l'ultimo assegnato è `D-222`. Se una PR in volo rivendica già `D-224` con una tesi diversa, questa
diventa `D-224` — ovunque, inclusi i commenti nel codice scritti nei Task 1-3.

- [ ] **Step 2: Rigenerare il corpus golden**

Run:
```
./scripts/rt-suite.ps1 -Filter RefactorTactics.Scenario
```
poi, per la rigenerazione vera:
```
UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests RefactorTactics;Quit" -dpcvars="rt.Test.RegenerateGolden=1" -unattended -nopause -nosplash -nullrhi -NoLiveCoding -log
```
⚠️ **La CVar va passata con `-dpcvars`, mai dentro `-ExecCmds`**: lì farebbe saltare la coda e il log troncato
sembrerebbe verde.

Expected: `git status Content/` mostra le tracce cambiate. **Verificare l'effetto atteso** — lo scudo nelle
tracce passa da 0 a 5 — invece di fidarsi dell'esito del test.

- [ ] **Step 3: Scrivere D-224 nel Decision Log**

Una riga nella tabella, nella forma della casa: tesi in grassetto, misura con data, cosa **non** chiude,
owner e test. Deve contenere:
- che **supera** *«Shield regeneration: nessuna rigenerazione automatica»* del PRD di ricerca, e perché
- l'effetto misurato sul danno piccolo (contrattacco −50%, `Burning` −63%) e la ragione della decisione 4
- che l'erosione temporaneo-prima **era già corretta**: D-224 la pinna con un test, non la introduce
- che `#1403` resta aperta su `Action.Cleanse`, e che `CLEANSE-1` non si chiude
- che il TTK si allunga e **non è ancora stato misurato in playtest**

- [ ] **Step 4: Allineare catalogo e ADR**

`RT_ActionCatalog_v0.1.md` §4 e `adr-0003:167` vanno aggiornati: l'azione non «aspetta il suo eroe», ne ha
due. Se dopo la modifica il catalogo di `balance/` diverge dal codice su un altro asse, la divergenza si
registra in `DOC_CONFLICT_MATRIX.md` — per [D-210] il codice prevale, ma non in silenzio.

- [ ] **Step 5: Suite intera, e leggere il verdetto dello script**

Run: `./scripts/rt-suite.ps1`
Expected: `VALIDA` e verde. Se lo script dichiara `NON VALIDA`, l'esito **non si registra**: si ripete a
macchina libera. Confrontare `Test Completed` con `Found N` — un `Fail` a zero non basta.

- [ ] **Step 6: Commit** *(solo se autorizzato)*

```bash
git branch --show-current
git add docs/ Content/
git commit -m "docs(D-224): lo scudo base entra nel canone, e dichiara cosa supera"
```

---

## Self-Review

**Copertura della spec** — le sei decisioni sono coperte: 1 e 2 dal Task 2 (con la nota che la 2 è già
implementata e viene solo pinnata), 3 dall'assenza di qualunque cap sul temporaneo in `ApplyDamage`, 4 dal
Task 1, 5 dal Task 3, 6 dalla firma scelta nel Task 1. Il modello dati, il ciclo, il resolver, catalogo,
test e corpus hanno ciascuno il loro task.

**Gap dichiarati** — due, entrambi voluti:
- **L'assunzione sull'ambientale contro il temporaneo** resta come la spec la lascia: il temporaneo assorbe
  anche gli hazard. Il test `EnvironmentalDamageStillErodesTemporaryShield` la **pinna**, così se un giorno
  la si cambia il rosso dice quale decisione si sta superando.
- **Il TTK non viene misurato.** Il piano non contiene playtest: la spec lo elenca fra i rischi e D-224 deve
  dichiararlo non misurato. Un piano che promettesse di verificarlo con la suite mentirebbe.

---

## Esito (2026-08-28)

Eseguito. **Suite 1260/1260, 0 fallimenti, `VALIDA`.** Recepito come
[D-224](../../decisions/RT_PDR_00_Decision_Log.md) — non D-223, che un branch in volo rivendicava già.

Quattro punti in cui l'esecuzione ha **corretto il piano**, tutti trovati dai test e non da una rilettura:

| Il piano diceva | Il codice ha detto |
|---|---|
| `RechargeBaseShield` in `BeginPlay` | I mondi di test (`UWorld::CreateWorld`) non lo fanno partire: lo scudo sarebbe esistito in partita e **non dove lo si verifica**. Spostata nel **costruttore** |
| Task 3: `Action.Shield` a Phase e Wraith | **Ritirato.** 6 azioni fanno 11 voci di kit contro i **10** tasti di `AbilityHotkeys()`: l'azione sarebbe stata raggiungibile per il gate e impremibile dal giocatore |
| Solo `FRTUnitCombatState` cambia | Anche i suoi **due produttori** andavano aggiornati — lo snapshot del Blast e la reazione che concede scudo lasciavano `TemporaryShield = 0`, difetto invisibile sui colpi diretti e latente fino al primo hazard nel Blast |
| I 23 test di meccanica: 23 setup | **10 helper di spawn**, perché ogni file ne ha uno solo |

⚠️ **Il Task 4 è costato più di quanto il piano stimasse**: non 11 file di dati ma **46 attese** in
`Scenarios/`, e non tutte `+5` — `TemporaryCoverExpires` **+20**, `LogAssertionsReadTheTurnLog` **−5**.
E `Visual.Combat.Defeat` non era un numero: lo scenario aveva smesso di dimostrare il proprio nome, ed è
stato allungato da quattro a **sei** turni con i valori intermedi **misurati con una sonda**, perché una
stima lineare aveva sbagliato di un turno.
