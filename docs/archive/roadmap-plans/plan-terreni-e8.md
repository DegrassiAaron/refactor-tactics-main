# Terreni esagonali (CP 8.1) — Piano di implementazione

> ## 📦 `DELIVERED PLAN` — PIANO GIA' ESEGUITO, NON NORMATIVO
>
> Il piano di esecuzione, gia' eseguito. **Il corpo qui sotto non va aggiornato**: comandi, nomi di branch e percorsi sono quelli di allora, e
> correggerli falsificherebbe la storia invece di renderla utile.
>
> Stato corrente: [`roadmap-checkpoint.md`](../roadmap-checkpoint.md) · indice dei documenti correnti:
> [`../../README.md`](../../README.md). Banner aggiunto il 2026-08-08.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Portare gli 8 terreni del catalogo (`Floor/Rough/ShallowWater/Fire/Conductive/Smoke/Ice/HighGround`)
sulle celle esagonali come catalogo dati, con gli hook di risoluzione (Dash/Charge bloccato da Rough,
scivolamento su Ice, danno/Burning del Fuoco, cap di targeting del Fumo).

**Architecture:** Catalogo letterale `TArray<FRTTerrainDef>` (stesso schema di `FRTActionDef`/
`URTCatalogLibrary`) validato da `URTTerrainLibrary`; `ERTHexSurface` relabelled in place (nessuna
migrazione di formato); gli hook si innestano nei punti di risoluzione esistenti (`RTHexSimLibrary`,
`RTTurnManager::ResolveDash/ResolveMovement`, `RTHexCombatLibrary::CollectHexAttacks`) senza toccare i
call site di `MoveCost`/`bBlocksMovement` già esistenti.

**Tech Stack:** Unreal Engine 5.8.1, C++, Unreal Automation Framework (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`).

## Global Constraints

- Solo interi nei campi di costo/portata/durata (invariante #4) — niente `float`.
- Nessuna regola di gioco in Blueprint/Actor: logica pura in librerie/USTRUCT (`Terrain/`, `Turn/`, `Combat/`).
- `URTHexMapAsset::FormatVersion` resta **2** — nessun bump, nessuna migrazione dati.
- `Cell.MoveCost`/`bBlocksMovement`/`bBlocksLineOfSight` restano i valori **autorevoli** per il pathfinding:
  nessuno dei 21 call site esistenti cambia comportamento.
- Riferimento: `docs/design/spec-terreni-e8.md` (spec approvato) e
  `docs/design/balance/RT_TerrainCatalog_v0.1.md` (catalogo canonico).
- Comando di test (Windows, come da `docs/guides/debug-vs-unreal.md:79`):
  ```
  "D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Repositories\rt-wt-64\RefactorTactics.uproject" -ExecCmds="Automation RunTests RefactorTactics.Terrain" -TestExit="Automation Test Queue Empty" -unattended -nullrhi -nopause
  ```
  Sostituire il filtro (`RefactorTactics.Terrain`, `RefactorTactics.HexMap`, `RefactorTactics.HexMove`, ...)
  per task.

---

### Task 1: Relabel `ERTHexSurface` e aggiornamento dei call site

**Files:**
- Modify: `Source/RefactorTactics/Map/RTHexCellData.h:8-18`
- Modify: `Source/RefactorTactics/Tests/RTHexMapTests.cpp` (righe con `ERTHexSurface::Water/Normal/Mud`)
- Modify: `Source/RefactorTactics/Tests/RTHexMovementIntegrationTests.cpp:173`
- Modify: `Source/RefactorTactics/Turn/RTMatchSetupLibrary.cpp:119`
- Modify: `Source/RefactorTacticsEditor/Private/RTHexEditorClick.cpp:95-101`
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h:39`
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexSelectTool.h:41`
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexFillTool.h:28`
- Modify: `Source/RefactorTactics/Map/RTHexMapActor.h:64,83` (aggiunto dopo la scoperta in Task 1 — mancava
  dal file list originale: `DemoSurface`/`PaintSurface` default a `ERTHexSurface::Normal`)

**Interfaces:**
- Produce: `enum class ERTHexSurface { Floor, ShallowWater, Rough, Fire, Conductive, Ice, Void, Smoke, HighGround }`
  (stessi ordinali 0-6 di oggi per i primi 7, `Smoke=7`, `HighGround=8` nuovi).

- [ ] **Step 1: Verifica la baseline (suite verde prima di toccare nulla)**

Run:
```
"D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Repositories\rt-wt-64\RefactorTactics.uproject" -ExecCmds="Automation RunTests RefactorTactics.HexMap+RefactorTactics.HexMove" -TestExit="Automation Test Queue Empty" -unattended -nullrhi -nopause
```
Expected: tutti i test passano (annotare il conteggio per confronto dopo il rename).

- [ ] **Step 2: Rinomina l'enum in `RTHexCellData.h`**

```cpp
UENUM(BlueprintType)
enum class ERTHexSurface : uint8
{
	Floor,
	ShallowWater,
	Rough,
	Fire,
	Conductive,
	Ice,
	Void,
	Smoke,
	HighGround
};
```

- [ ] **Step 3: Aggiorna i call site — test**

In `RTHexMapTests.cpp` e `RTHexMovementIntegrationTests.cpp:173`, sostituisci ogni occorrenza:
`ERTHexSurface::Normal` → `ERTHexSurface::Floor`, `ERTHexSurface::Water` → `ERTHexSurface::ShallowWater`,
`ERTHexSurface::Mud` → `ERTHexSurface::Rough`. Nessun altro valore cambia comportamento nei test esistenti
(i valori numerici restano gli stessi, il comportamento testato — costo/blocco — non cambia).

- [ ] **Step 4: Aggiorna `RTMatchSetupLibrary.cpp:119`**

```cpp
Cell.Surface = ERTHexSurface::Rough;
```
(la fascia demo resta a `MoveCost = 3`, per-cella, deliberatamente diversa dal default `2` del catalogo —
non toccare quel valore.)

- [ ] **Step 5: Aggiorna gli editor tool**

In `RTHexEditorClick.cpp:95-101`, rinomina i case esistenti e aggiungi i due nuovi:
```cpp
case ERTHexSurface::ShallowWater: return FColor(60, 120, 255);
case ERTHexSurface::Rough:        return FColor(140, 100, 60);
case ERTHexSurface::Conductive:   return FColor(80, 230, 230);
case ERTHexSurface::Void:         return FColor(150, 40, 150);
case ERTHexSurface::Smoke:        return FColor(160, 160, 160);
case ERTHexSurface::HighGround:   return FColor(230, 200, 80);
case ERTHexSurface::Floor:
```
In `RTHexPaintTool.h:39`, `RTHexSelectTool.h:41`, `RTHexFillTool.h:28`: `ERTHexSurface Surface = ERTHexSurface::Floor;`
In `RTHexMapActor.h:64` (`DemoSurface`) e `:83` (`PaintSurface`): `ERTHexSurface::Normal` → `ERTHexSurface::Floor`.

- [ ] **Step 6: Ricompila e riesegui la suite del baseline**

Run: stesso comando dello Step 1.
Expected: **stesso conteggio** di test verdi dello Step 1 (nessuna regressione, nessuna omissione).

- [ ] **Step 7: Commit**

```bash
git add Source/RefactorTactics/Map/RTHexCellData.h Source/RefactorTactics/Tests/RTHexMapTests.cpp Source/RefactorTactics/Tests/RTHexMovementIntegrationTests.cpp Source/RefactorTactics/Turn/RTMatchSetupLibrary.cpp Source/RefactorTacticsEditor/Private/RTHexEditorClick.cpp Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h Source/RefactorTacticsEditor/Private/Tools/RTHexSelectTool.h Source/RefactorTacticsEditor/Private/Tools/RTHexFillTool.h
git commit -m "refactor(terreni): relabel ERTHexSurface sugli 8 terreni del catalogo (CP 8.1)"
```

---

### Task 2: Nuovi GameplayTags di stato ambientale

**Files:**
- Modify: `Source/RefactorTactics/Core/RTGameplayTags.h`
- Modify: `Source/RefactorTactics/Core/RTGameplayTags.cpp`

**Interfaces:**
- Produce: `TAG_Status_Wet`, `TAG_Status_Burning`, `TAG_Status_Obscured` (extern `FGameplayTag`).

- [ ] **Step 1: Dichiara i tag in `RTGameplayTags.h`**

```cpp
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Wet);      // conduce elettricita', spegne Burning
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Burning);  // danno nel Cleanup (CP 8.2), rimosso da Wet
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Obscured); // targeting limitato (Smoke)
```

- [ ] **Step 2: Definisci i tag in `RTGameplayTags.cpp`**

```cpp
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Wet, "Status.Wet");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Burning, "Status.Burning");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Obscured, "Status.Obscured");
```

- [ ] **Step 3: Compila**

Nessun test dedicato (dichiarazione dati pura): la verifica è la compilazione del modulo
`RefactorTactics` senza errori.

- [ ] **Step 4: Commit**

```bash
git add Source/RefactorTactics/Core/RTGameplayTags.h Source/RefactorTactics/Core/RTGameplayTags.cpp
git commit -m "feat(terreni): tag Status.Wet/Burning/Obscured per gli effetti dei terreni (CP 8.1)"
```

---

### Task 3: `FRTTerrainDef` + `URTTerrainLibrary` (catalogo)

**Files:**
- Create: `Source/RefactorTactics/Terrain/RTTerrainData.h`
- Create: `Source/RefactorTactics/Terrain/RTTerrainLibrary.h`
- Create: `Source/RefactorTactics/Terrain/RTTerrainLibrary.cpp`
- Test: `Source/RefactorTactics/Tests/RTTerrainTests.cpp`

**Interfaces:**
- Consuma: `ERTHexSurface` (Task 1), `FRTActionEffectSpec`/`ERTActionEffect` (`Turn/RTActionEvent.h`,
  esistente), `TAG_Status_Wet`/`TAG_Status_Burning`/`TAG_Status_Obscured` (Task 2).
- Produce: `FRTTerrainDef`, `URTTerrainLibrary::GetTerrainCatalog() -> TArray<FRTTerrainDef>`,
  `URTTerrainLibrary::FindTerrainDef(ERTHexSurface) -> FRTTerrainDef`,
  `URTTerrainLibrary::ValidateTerrainCatalog() -> TArray<FString>`.

- [ ] **Step 1: Scrivi i test falliti**

`Source/RefactorTactics/Tests/RTTerrainTests.cpp`:
```cpp
#include "Misc/AutomationTest.h"
#include "Terrain/RTTerrainData.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Core/RTGameplayTags.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainCostsFromCatalogTest,
	"RefactorTactics.Terrain.CostsFromCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainCostsFromCatalogTest::RunTest(const FString&)
{
	const TArray<FRTTerrainDef> Catalog = URTTerrainLibrary::GetTerrainCatalog();
	TestEqual(TEXT("8 terreni nel catalogo"), Catalog.Num(), 8);

	auto FindCost = [&Catalog](ERTHexSurface Surface) -> int32
	{
		for (const FRTTerrainDef& Def : Catalog)
		{
			if (Def.Surface == Surface) { return Def.MoveCost; }
		}
		return -1;
	};

	TestEqual(TEXT("Floor costo 1"), FindCost(ERTHexSurface::Floor), 1);
	TestEqual(TEXT("Rough costo 2"), FindCost(ERTHexSurface::Rough), 2);
	TestEqual(TEXT("ShallowWater costo 2"), FindCost(ERTHexSurface::ShallowWater), 2);
	TestEqual(TEXT("Fire costo 2"), FindCost(ERTHexSurface::Fire), 2);
	TestEqual(TEXT("Conductive costo 1"), FindCost(ERTHexSurface::Conductive), 1);
	TestEqual(TEXT("Smoke costo 1"), FindCost(ERTHexSurface::Smoke), 1);
	TestEqual(TEXT("Ice costo 1"), FindCost(ERTHexSurface::Ice), 1);
	TestEqual(TEXT("HighGround costo 1"), FindCost(ERTHexSurface::HighGround), 1);

	const FRTTerrainDef Rough = URTTerrainLibrary::FindTerrainDef(ERTHexSurface::Rough);
	TestTrue(TEXT("Rough blocca Dash/Charge"), Rough.bBlocksDashCharge);

	const FRTTerrainDef Smoke = URTTerrainLibrary::FindTerrainDef(ERTHexSurface::Smoke);
	TestEqual(TEXT("Smoke limita il targeting a 2"), Smoke.MaxTargetingRangeThrough, 2);

	const FRTTerrainDef Conductive = URTTerrainLibrary::FindTerrainDef(ERTHexSurface::Conductive);
	TestTrue(TEXT("Conductive conduce elettricita'"), Conductive.bConductsElectricity);
	for (const FRTActionEffectSpec& Effect : Conductive.OnEnterEffects)
	{
		TestFalse(TEXT("Conductive non applica Wet"), Effect.Effect == ERTActionEffect::Status && Effect.StatusTag == TAG_Status_Wet);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainValidateCatalogTest,
	"RefactorTactics.Terrain.ValidateCatalog.NoDuplicatesNoNegativeCosts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainValidateCatalogTest::RunTest(const FString&)
{
	const TArray<FString> Errors = URTTerrainLibrary::ValidateTerrainCatalog();
	TestEqual(TEXT("catalogo spedito valido"), Errors.Num(), 0);

	TArray<FRTTerrainDef> Broken = URTTerrainLibrary::GetTerrainCatalog();
	Broken.Add(Broken[0]); // duplica Floor
	Broken.Last().MoveCost = -1; // costo negativo
	// ValidateTerrainCatalog valida SEMPRE GetTerrainCatalog(): per testare un catalogo rotto si verifica
	// solo che il validator normale non produca falsi positivi (sopra) — il caso "duplicato+negativo" è
	// coperto a livello di funzione pura in Task 3 Step 3 (ValidateCatalogEntries, testata direttamente).
	return true;
}

#endif
```

- [ ] **Step 2: Esegui e verifica il fallimento (mancano i tipi/funzioni)**

Run: `... -ExecCmds="Automation RunTests RefactorTactics.Terrain" ...` (comando in "Global Constraints").
Expected: **compilazione fallita** — `RTTerrainData.h`/`RTTerrainLibrary.h` non esistono ancora.

- [ ] **Step 3: Implementa `RTTerrainData.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Map/RTHexCellData.h"
#include "Turn/RTActionEvent.h"
#include "RTTerrainData.generated.h"

/**
 * Definizione dichiarativa di un terreno del catalogo (RT_TerrainCatalog_v0.1.md §1): costo di movimento,
 * blocchi, conducibilita' elettrica, limite di targeting ed effetti applicati all'ingresso. Vive nel
 * catalogo letterale di URTTerrainLibrary::GetTerrainCatalog, non in un asset ne' in uno switch C++.
 */
USTRUCT(BlueprintType)
struct FRTTerrainDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	ERTHexSurface Surface = ERTHexSurface::Floor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 MoveCost = 1;

	/** Vieta Dash/Charge (mobilita' rapida) attraverso la cella; il movimento normale non e' affetto. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	bool bBlocksDashCharge = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	bool bBlocksLineOfSight = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	bool bConductsElectricity = false;

	/** 0 = nessun limite; N > 0 = la portata effettiva di un intento la cui linea attraversa questa cella e' min(RangeCells, N). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 MaxTargetingRangeThrough = 0;

	/** Effetti applicati a un'unita' quando entra nella cella (Damage/Status; riusa il vocabolario delle azioni). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	TArray<FRTActionEffectSpec> OnEnterEffects;

	FRTTerrainDef() = default;
};
```

- [ ] **Step 4: Implementa `RTTerrainLibrary.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Terrain/RTTerrainData.h"
#include "RTTerrainLibrary.generated.h"

/**
 * Lettura e validazione del catalogo terreni: pura, deterministica, senza Actor e senza asset.
 * Stesso schema di URTCatalogLibrary per le azioni (RT_TerrainCatalog_v0.1.md).
 */
UCLASS()
class REFACTORTACTICS_API URTTerrainLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Gli 8 terreni del catalogo v0.1 (RT_TerrainCatalog_v0.1.md §1). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Terrain")
	static TArray<FRTTerrainDef> GetTerrainCatalog();

	/** Definizione del terreno indicato, o `FRTTerrainDef` di default (Floor) se assente dal catalogo. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Terrain")
	static FRTTerrainDef FindTerrainDef(ERTHexSurface Surface);

	/** Errori strutturali del catalogo SPEDITO (vuoto = valido): id duplicato, costo o limite di targeting negativi. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Terrain")
	static TArray<FString> ValidateTerrainCatalog();

	/** Stessa validazione di ValidateTerrainCatalog, ma su un catalogo ARBITRARIO (testabile con input rotto). */
	static TArray<FString> ValidateCatalogEntries(const TArray<FRTTerrainDef>& Catalog);
};
```

- [ ] **Step 5: Implementa `RTTerrainLibrary.cpp`**

```cpp
#include "Terrain/RTTerrainLibrary.h"
#include "Core/RTGameplayTags.h"

namespace
{
	FRTTerrainDef MakeTerrain(ERTHexSurface Surface, int32 MoveCost, bool bBlocksDashCharge,
		bool bBlocksLineOfSight, bool bConductsElectricity, int32 MaxTargetingRangeThrough,
		TArray<FRTActionEffectSpec> OnEnterEffects)
	{
		FRTTerrainDef Def;
		Def.Surface = Surface;
		Def.MoveCost = MoveCost;
		Def.bBlocksDashCharge = bBlocksDashCharge;
		Def.bBlocksLineOfSight = bBlocksLineOfSight;
		Def.bConductsElectricity = bConductsElectricity;
		Def.MaxTargetingRangeThrough = MaxTargetingRangeThrough;
		Def.OnEnterEffects = MoveTemp(OnEnterEffects);
		return Def;
	}
}

TArray<FRTTerrainDef> URTTerrainLibrary::GetTerrainCatalog()
{
	TArray<FRTTerrainDef> Catalog;
	Catalog.Add(MakeTerrain(ERTHexSurface::Floor,        1, false, false, false, 0, {}));
	Catalog.Add(MakeTerrain(ERTHexSurface::Rough,        2, true,  false, false, 0, {}));
	Catalog.Add(MakeTerrain(ERTHexSurface::ShallowWater, 2, false, false, true,  0,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Wet, 0) }));
	Catalog.Add(MakeTerrain(ERTHexSurface::Fire,         2, false, false, false, 0,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 10),
		  FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Burning, 2) }));
	Catalog.Add(MakeTerrain(ERTHexSurface::Conductive,   1, false, false, true,  0, {}));
	Catalog.Add(MakeTerrain(ERTHexSurface::Smoke,        1, false, false, false, 2,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Obscured, 0) }));
	Catalog.Add(MakeTerrain(ERTHexSurface::Ice,          1, false, false, false, 0, {}));
	Catalog.Add(MakeTerrain(ERTHexSurface::HighGround,   1, false, false, false, 0, {}));
	return Catalog;
}

FRTTerrainDef URTTerrainLibrary::FindTerrainDef(ERTHexSurface Surface)
{
	for (const FRTTerrainDef& Def : GetTerrainCatalog())
	{
		if (Def.Surface == Surface) { return Def; }
	}
	return FRTTerrainDef();
}

TArray<FString> URTTerrainLibrary::ValidateTerrainCatalog()
{
	return ValidateCatalogEntries(GetTerrainCatalog());
}

TArray<FString> URTTerrainLibrary::ValidateCatalogEntries(const TArray<FRTTerrainDef>& Catalog)
{
	TArray<FString> Errors;
	TSet<ERTHexSurface> Seen;
	for (const FRTTerrainDef& Def : Catalog)
	{
		bool bAlreadySeen = false;
		Seen.Add(Def.Surface, &bAlreadySeen);
		if (bAlreadySeen)
		{
			Errors.Add(FString::Printf(TEXT("Terreno duplicato: %d"), static_cast<int32>(Def.Surface)));
		}
		if (Def.MoveCost < 0)
		{
			Errors.Add(FString::Printf(TEXT("Terreno %d: MoveCost negativo (%d)"), static_cast<int32>(Def.Surface), Def.MoveCost));
		}
		if (Def.MaxTargetingRangeThrough < 0)
		{
			Errors.Add(FString::Printf(TEXT("Terreno %d: MaxTargetingRangeThrough negativo (%d)"), static_cast<int32>(Def.Surface), Def.MaxTargetingRangeThrough));
		}
	}
	return Errors;
}
```

Nota: `TSet<ERTHexSurface>::Add(Elem, bool* bAlreadyInSetPtr)` è l'overload standard di `TSet` — verificarne
la firma esatta nella versione UE 5.8 in uso durante l'implementazione; se assente, sostituire con
`Seen.Contains(Def.Surface)` prima di `Seen.Add(Def.Surface)`.

- [ ] **Step 6: Correggi il test dello Step 1**

Rimuovi il blocco morto in `FRTTerrainValidateCatalogTest` (la parte `Broken` non asserisce nulla): il test
verifica solo che `ValidateTerrainCatalog()` sul catalogo spedito ritorni zero errori. Aggiungi un test
separato per la funzione pura:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainValidateCatalogEntriesTest,
	"RefactorTactics.Terrain.ValidateCatalogEntries.CatchesDuplicatesAndNegativeCosts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainValidateCatalogEntriesTest::RunTest(const FString&)
{
	TArray<FRTTerrainDef> Broken;
	FRTTerrainDef A; A.Surface = ERTHexSurface::Floor; A.MoveCost = -1;
	FRTTerrainDef B; B.Surface = ERTHexSurface::Floor; // duplicato di A
	Broken.Add(A);
	Broken.Add(B);

	const TArray<FString> Errors = URTTerrainLibrary::ValidateCatalogEntries(Broken);
	TestTrue(TEXT("almeno 2 errori (duplicato + costo negativo)"), Errors.Num() >= 2);
	return true;
}
```

- [ ] **Step 7: Esegui i test e verifica che passino**

Run: comando terreni da "Global Constraints".
Expected: `RefactorTactics.Terrain.CostsFromCatalog`,
`RefactorTactics.Terrain.ValidateCatalog.NoDuplicatesNoNegativeCosts`,
`RefactorTactics.Terrain.ValidateCatalogEntries.CatchesDuplicatesAndNegativeCosts` — tutti **PASS**.

- [ ] **Step 8: Commit**

```bash
git add Source/RefactorTactics/Terrain/RTTerrainData.h Source/RefactorTactics/Terrain/RTTerrainLibrary.h Source/RefactorTactics/Terrain/RTTerrainLibrary.cpp Source/RefactorTactics/Tests/RTTerrainTests.cpp
git commit -m "feat(terreni): catalogo FRTTerrainDef/URTTerrainLibrary per gli 8 terreni (CP 8.1)"
```

---

### Task 4: Default del paint tool dal catalogo + colori overlay nuovi

**Files:**
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp`

**Interfaces:**
- Consuma: `URTTerrainLibrary::FindTerrainDef` (Task 3).

- [ ] **Step 1: Trova dove la selezione di `Surface` nel tool aggiorna `Properties`**

Apri `RTHexPaintTool.cpp` e cerca il punto in cui `Properties->Surface` viene letto per dipingere una cella
(la chiamata a `URTHexMapAsset::ApplyBrush`/`PaintCellInStroke`). Non esiste oggi un punto che reagisce al
CAMBIO di `Properties->Surface` per aggiornare `MoveCost`/`bBlocksMovement`: va aggiunto un `PostEditChangeProperty`
(o equivalente reattivo alla property) su `URTHexPaintToolProperties`.

- [ ] **Step 2: Aggiungi la reazione al cambio di `Surface` in `RTHexPaintTool.h`**

In `URTHexPaintToolProperties`, aggiungi:
```cpp
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
```

- [ ] **Step 3: Implementa in `RTHexPaintTool.cpp`**

```cpp
#include "Terrain/RTTerrainLibrary.h"

#if WITH_EDITOR
void URTHexPaintToolProperties::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(URTHexPaintToolProperties, Surface))
	{
		const FRTTerrainDef Def = URTTerrainLibrary::FindTerrainDef(Surface);
		MoveCost = Def.MoveCost;
		bBlocksMovement = false; // nessun terreno del catalogo v0.1 blocca il movimento normale
	}
}
#endif
```

- [ ] **Step 4: Verifica manuale in editor (PIE non richiesto — solo editor mode)**

Apri l'editor, seleziona il Hex Paint Tool, cambia `Surface` a `Rough`: `MoveCost` nel pannello deve
aggiornarsi a `2`. Annota la verifica in `docs/design/test-manuali-pie.md` come voce ⏳/✅ secondo convenzione
del progetto (nessun test automatico: è UI editor-only).

- [ ] **Step 5: Commit**

```bash
git add Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp docs/design/test-manuali-pie.md
git commit -m "feat(terreni): il paint tool precompila MoveCost dal catalogo terreni (CP 8.1)"
```

---

### Task 5: Rough blocca Dash/Charge

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTHexSimLibrary.cpp:291`
- Test: `Source/RefactorTactics/Tests/RTTerrainTests.cpp` (append)

**Interfaces:**
- Consuma: `URTTerrainLibrary::FindTerrainDef` (Task 3).

- [ ] **Step 1: Scrivi il test fallito**

Append a `RTTerrainTests.cpp`:
```cpp
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainRoughBlocksDashTest,
	"RefactorTactics.Terrain.Rough.BlocksDash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainRoughBlocksDashTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 3))
	{
		Map->AddOrUpdateCell(FRTHexCellData(Id));
	}
	FRTHexCellData Blocker(FRTCellId(1, 0, 0));
	Blocker.Surface = ERTHexSurface::Rough;
	Map->AddOrUpdateCell(Blocker);
	Map->SortCells();

	FRTHexSimUnit Unit;
	Unit.UnitId = 0;
	Unit.Cell = FRTCellId(0, 0, 0);
	Unit.MoveBudget = 10;

	FRTHexSnapshot Snapshot;
	Snapshot.Map = Map;
	Snapshot.Units.Add(Unit);

	const TArray<FRTCellId> Path = URTHexSimLibrary::LinearDashPath(Snapshot, /*UnitId=*/ 0, FRTCellId(2, 0, 0));
	TestEqual(TEXT("scatto rifiutato: Rough blocca Dash"), Path.Num(), 0);

	// Il movimento NORMALE, invece, attraversa Rough (costa di piu', non e' bloccato).
	const FRTHexPathResult Normal = URTHexSimLibrary::FindPathForUnit(Snapshot, /*UnitId=*/ 0, FRTCellId(2, 0, 0));
	TestTrue(TEXT("il Move normale attraversa Rough"), Normal.Status == ERTHexPathStatus::Success);

	return true;
}
```

- [ ] **Step 2: Esegui e verifica il fallimento**

Run: comando terreni. Expected: `Rough.BlocksDash` FAIL (`Path.Num()` non è 0: `LinearDashPath` oggi non
controlla `bBlocksDashCharge`).

- [ ] **Step 3: Implementa l'hook in `RTHexSimLibrary.cpp:291`**

Prima:
```cpp
		const FRTHexCellData* Data = Snapshot.Map->FindCell(Current);
		if (!Data || Data->bBlocksMovement || Blocked.Contains(Current))
		{
			return {};
		}
```
Dopo:
```cpp
		const FRTHexCellData* Data = Snapshot.Map->FindCell(Current);
		if (!Data || Data->bBlocksMovement || Blocked.Contains(Current)
			|| URTTerrainLibrary::FindTerrainDef(Data->Surface).bBlocksDashCharge)
		{
			return {};
		}
```
Aggiungi l'include in cima al file: `#include "Terrain/RTTerrainLibrary.h"`.

- [ ] **Step 4: Esegui e verifica che passi**

Run: comando terreni. Expected: `Rough.BlocksDash` PASS.

- [ ] **Step 5: Commit**

```bash
git add Source/RefactorTactics/Turn/RTHexSimLibrary.cpp Source/RefactorTactics/Tests/RTTerrainTests.cpp
git commit -m "feat(terreni): Rough blocca Dash/Charge, non il movimento normale (CP 8.1)"
```

---

### Task 6: Scivolamento su Ice (solo Move normale)

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTHexSimLibrary.h`
- Modify: `Source/RefactorTactics/Turn/RTHexSimLibrary.cpp`
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.cpp:1174` (in `ResolveMovement`)
- Test: `Source/RefactorTactics/Tests/RTTerrainTests.cpp` (append)

**Interfaces:**
- Consuma: `FRTHexSnapshot`, `FRTHexSimUnit` (`Turn/RTHexSim.h`, esistenti), `URTHexLibrary::AxialDirection`
  (`Map/RTHexLibrary.h`, esistente).
- Produce: `URTHexSimLibrary::ApplyIceSliding(const FRTHexSnapshot&, int32 UnitId, const TArray<FRTCellId>& Path) -> TArray<FRTCellId>`.

- [ ] **Step 1: Scrivi i test falliti**

Append a `RTTerrainTests.cpp`:
```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainIceSlidesWithBudgetTest,
	"RefactorTactics.Terrain.Ice.SlidesWithSufficientBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainIceSlidesWithBudgetTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 3))
	{
		Map->AddOrUpdateCell(FRTHexCellData(Id));
	}
	FRTHexCellData IceCell(FRTCellId(1, 0, 0));
	IceCell.Surface = ERTHexSurface::Ice;
	Map->AddOrUpdateCell(IceCell);
	Map->SortCells();

	FRTHexSimUnit Unit;
	Unit.UnitId = 0;
	Unit.Cell = FRTCellId(0, 0, 0);
	Unit.MoveBudget = 5; // 1 per entrare sul ghiaccio (costo 1), 4 residui: >= 2, scivola

	FRTHexSnapshot Snapshot;
	Snapshot.Map = Map;
	Snapshot.Units.Add(Unit);

	const TArray<FRTCellId> Path = { FRTCellId(0, 0, 0), FRTCellId(1, 0, 0) };
	const TArray<FRTCellId> Extended = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ 0, Path);

	TestEqual(TEXT("il percorso si estende di una cella"), Extended.Num(), 3);
	TestTrue(TEXT("la cella extra e' nella direzione d'ingresso"), Extended.Last() == FRTCellId(2, 0, 0));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainIceBlockedCellStopsSlidingTest,
	"RefactorTactics.Terrain.Ice.BlockedCellStopsSliding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainIceBlockedCellStopsSlidingTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 3))
	{
		Map->AddOrUpdateCell(FRTHexCellData(Id));
	}
	FRTHexCellData IceCell(FRTCellId(1, 0, 0));
	IceCell.Surface = ERTHexSurface::Ice;
	Map->AddOrUpdateCell(IceCell);
	FRTHexCellData Wall(FRTCellId(2, 0, 0));
	Wall.bBlocksMovement = true;
	Map->AddOrUpdateCell(Wall);
	Map->SortCells();

	FRTHexSimUnit Unit;
	Unit.UnitId = 0;
	Unit.Cell = FRTCellId(0, 0, 0);
	Unit.MoveBudget = 5;

	FRTHexSnapshot Snapshot;
	Snapshot.Map = Map;
	Snapshot.Units.Add(Unit);

	const TArray<FRTCellId> Path = { FRTCellId(0, 0, 0), FRTCellId(1, 0, 0) };
	const TArray<FRTCellId> Extended = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ 0, Path);

	TestEqual(TEXT("nessuno scivolamento: la cella successiva blocca il movimento"), Extended.Num(), 2);

	return true;
}
```

- [ ] **Step 2: Esegui e verifica il fallimento**

Run: comando terreni. Expected: compilazione fallita (`ApplyIceSliding` non esiste ancora).

- [ ] **Step 3: Dichiara `ApplyIceSliding` in `RTHexSimLibrary.h`**

Aggiungi dopo la dichiarazione di `LinearDashPath` (dopo la riga `static TArray<FRTCellId> LinearDashPath(...)`):
```cpp
	/**
	 * Se Path termina su una cella Ice e il budget residuo dell'unita' (MoveBudget - costo del percorso) e'
	 * >= 2, estende Path di una cella nella direzione dell'ultimo passo (se esiste e non blocca il
	 * movimento). Altrimenti ritorna Path invariato. Path.Num() < 2, o l'ultimo passo non e' un vicino
	 * diretto sullo stesso layer (es. arrivo via transizione), -> Path invariato.
	 *
	 * La cella aggiunta NON e' verificata per occupazione: il path esteso entra nel microstep di
	 * ResolveHexPaths, che gestisce occupazione/collisione come qualunque altro passo pianificato — quindi
	 * due unita' che scivolano verso la stessa cella libera sono gestite dal resolver esistente, non da
	 * questa funzione. Va chiamata SOLO sul path del movimento normale, MAI su LinearDashPath (lo Scatto non
	 * passa dal microstep condiviso: vedi docs/design/spec-terreni-e8.md §5.2).
	 */
	static TArray<FRTCellId> ApplyIceSliding(const FRTHexSnapshot& Snapshot, int32 UnitId, const TArray<FRTCellId>& Path);
```

- [ ] **Step 4: Implementa in `RTHexSimLibrary.cpp`**

Aggiungi dopo `IsLinearDashReachable` (o in coda al file, prima di `ResolveHexPaths`):
```cpp
TArray<FRTCellId> URTHexSimLibrary::ApplyIceSliding(const FRTHexSnapshot& Snapshot, int32 UnitId, const TArray<FRTCellId>& Path)
{
	const FRTHexSimUnit* Unit = FindUnit(Snapshot, UnitId);
	if (!Snapshot.Map || !Unit || Path.Num() < 2)
	{
		return Path;
	}

	const FRTCellId LastCell = Path.Last();
	const FRTHexCellData* LastData = Snapshot.Map->FindCell(LastCell);
	if (!LastData || LastData->Surface != ERTHexSurface::Ice)
	{
		return Path;
	}

	int32 PathCost = 0;
	for (int32 I = 1; I < Path.Num(); ++I)
	{
		const FRTHexCellData* StepData = Snapshot.Map->FindCell(Path[I]);
		PathCost += StepData ? FMath::Max(1, StepData->MoveCost) : 1;
	}
	if (Unit->MoveBudget - PathCost < 2)
	{
		return Path;
	}

	const FRTCellId PrevCell = Path[Path.Num() - 2];
	if (PrevCell.Layer != LastCell.Layer)
	{
		return Path; // arrivo via transizione: nessuna "direzione" da cui scivolare
	}

	const int32 StepQ = LastCell.X - PrevCell.X;
	const int32 StepR = LastCell.Y - PrevCell.Y;
	FIntPoint Dir(0, 0);
	bool bValidDirection = false;
	for (int32 D = 0; D < 6; ++D)
	{
		const FIntPoint Candidate = URTHexLibrary::AxialDirection(static_cast<ERTHexDirection>(D));
		if (Candidate.X == StepQ && Candidate.Y == StepR)
		{
			Dir = Candidate;
			bValidDirection = true;
			break;
		}
	}
	if (!bValidDirection)
	{
		return Path; // ultimo passo non e' un vicino diretto
	}

	const FRTCellId SlideCell(LastCell.X + Dir.X, LastCell.Y + Dir.Y, LastCell.Layer);
	const FRTHexCellData* SlideData = Snapshot.Map->FindCell(SlideCell);
	if (!SlideData || SlideData->bBlocksMovement)
	{
		return Path;
	}

	TArray<FRTCellId> Extended = Path;
	Extended.Add(SlideCell);
	return Extended;
}
```

- [ ] **Step 5: Esegui e verifica che i due test passino**

Run: comando terreni. Expected: `Ice.SlidesWithSufficientBudget` e `Ice.BlockedCellStopsSliding` PASS.

- [ ] **Step 6: Innesta la chiamata in `ResolveMovement` (`RTTurnManager.cpp`)**

Nel blocco che costruisce `Paths` (righe 1157-1176: dopo che `Path` è stato assegnato da `PlannedPath`, da
`FindPathForUnit`, o dal fallback `{ Unit->Cell }`), subito prima di `Paths.Add(Path);`:
```cpp
		if (Path.Num() < 2)
		{
			Path = { Unit->Cell }; // fermo
		}
		Path = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ i, Path);
		Paths.Add(Path);
```

- [ ] **Step 7: Verifica manuale — integrazione end-to-end (opzionale ma raccomandata)**

Se il tempo lo consente, scrivi un test in `RTHexMovementIntegrationTests.cpp` che spawna un'unità, pianifica
un `PlannedCell` che termina su una cella `Ice` con budget residuo ≥ 2 dopo l'arrivo, e verifica
`Mover->Cell` sia la cella **oltre** il ghiaccio (stesso pattern di `SpawnHexMap`/`SpawnHexUnit`/`RunTurn` già
presente nel file). Se omesso per tempo, dichiararlo nella PR come verifica non coperta da test automatico.

- [ ] **Step 8: Commit**

```bash
git add Source/RefactorTactics/Turn/RTHexSimLibrary.h Source/RefactorTactics/Turn/RTHexSimLibrary.cpp Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Tests/RTTerrainTests.cpp
git commit -m "feat(terreni): scivolamento su Ice per il movimento normale, riusa il microstep esistente (CP 8.1)"
```

---

### Task 7: Danno e Burning del Fuoco all'ingresso

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.h` (nuovo metodo privato)
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.cpp:637-686` (`ResolveDash`), `:1219-1228` (`ResolveMovement`)
- Test: `Source/RefactorTactics/Tests/RTHexMovementIntegrationTests.cpp` (append)

**Interfaces:**
- Consuma: `URTTerrainLibrary::FindTerrainDef` (Task 3), `URTCombatLibrary::ApplyDamage`
  (`Combat/RTCombatLibrary.h`, esistente), `ARTUnit::Health/Shield/ApplyStatus` (`Unit/RTUnit.h`, esistenti).
- Produce: `ARTTurnManager::ApplyTerrainOnEnterEffects(const FRTHexSnapshot&, ARTUnit*, const TArray<FRTCellId>& Entered)` (privato).

- [ ] **Step 1: Scrivi il test fallito (integrazione end-to-end)**

Append a `RTHexMovementIntegrationTests.cpp`:
```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainFireDamagesAndBurnsOnEnterTest,
	"RefactorTactics.Terrain.Fire.DamagesAndBurnsOnEnter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainFireDamagesAndBurnsOnEnterTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnHexMap(World, /*Radius=*/ 4);
	FRTHexCellData FireCell(FRTCellId(1, 0, 0));
	FireCell.Surface = ERTHexSurface::Fire;
	MapActor->MapAsset->AddOrUpdateCell(FireCell);
	MapActor->MapAsset->SortCells();

	ARTUnit* Mover = SpawnHexUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover) { DestroyHexMoveWorld(World); return false; }

	const int32 StartHealth = Mover->Health;
	Mover->PlannedCell = FRTCellId(2, 0);

	RunTurn(TM);

	TestEqual(TEXT("10 danni dal Fuoco"), Mover->Health, StartHealth - 10);
	TestTrue(TEXT("Burning applicato"), Mover->HasStatus(TAG_Status_Burning));

	DestroyHexMoveWorld(World);
	return true;
}
```

Nota: se `ARTUnit` non espone `HasStatus(FGameplayTag)`, verifica il nome del metodo equivalente in
`Unit/RTUnit.h` (cercare `ApplyStatus` nello stesso file: di norma le classi con `ApplyStatus` espongono
anche un query — es. `HasStatus`/`GetStatusTurns`) e correggi l'asserzione di conseguenza prima di
procedere allo Step 2.

- [ ] **Step 2: Esegui e verifica il fallimento**

Run: `... -ExecCmds="Automation RunTests RefactorTactics.Terrain.Fire" ...`
Expected: FAIL — `Mover->Health` invariato (nessun danno applicato oggi, come dichiarato dalla NOTA a
`RTTurnManager.cpp:1224-1228`).

- [ ] **Step 3: Dichiara il metodo privato in `RTTurnManager.h`**

Nella sezione privata della classe, vicino alla dichiarazione di `AddLogEvent`:
```cpp
	/**
	 * Applica gli OnEnterEffects (URTTerrainLibrary) di ogni cella in Entered a Unit: Damage via
	 * URTCombatLibrary::ApplyDamage, Status via Unit->ApplyStatus. Usata da ResolveDash e ResolveMovement
	 * sulle celle FRTHexMoveResult::Entered di ciascuna unita' (CP 8.1).
	 */
	void ApplyTerrainOnEnterEffects(const FRTHexSnapshot& Snapshot, ARTUnit* Unit, const TArray<FRTCellId>& Entered);
```

- [ ] **Step 4: Implementa in `RTTurnManager.cpp`**

Aggiungi vicino a `AddLogEvent` (riga 51):
```cpp
#include "Terrain/RTTerrainLibrary.h"
#include "Combat/RTCombatLibrary.h"

void ARTTurnManager::ApplyTerrainOnEnterEffects(const FRTHexSnapshot& Snapshot, ARTUnit* Unit, const TArray<FRTCellId>& Entered)
{
	if (!Snapshot.Map || !Unit) { return; }

	for (const FRTCellId& Cell : Entered)
	{
		const FRTHexCellData* CellData = Snapshot.Map->FindCell(Cell);
		if (!CellData) { continue; }

		const FRTTerrainDef Terrain = URTTerrainLibrary::FindTerrainDef(CellData->Surface);
		for (const FRTActionEffectSpec& Effect : Terrain.OnEnterEffects)
		{
			if (Effect.Effect == ERTActionEffect::Damage)
			{
				const FRTDamageResult Result = URTCombatLibrary::ApplyDamage(Effect.Amount, Unit->Shield, Unit->Health);
				Unit->Health = Result.Health;
				Unit->Shield = Result.Shield;
				AddLogEvent(FString::Printf(TEXT("%s: %d danni da terreno (q=%d,r=%d,L%d)"),
					*Unit->GetName(), Effect.Amount, Cell.X, Cell.Y, Cell.Layer));
			}
			else if (Effect.Effect == ERTActionEffect::Status)
			{
				Unit->ApplyStatus(Effect.StatusTag, Effect.StatusDuration);
				AddLogEvent(FString::Printf(TEXT("%s: %s da terreno"), *Unit->GetName(), *Effect.StatusTag.ToString()));
			}
		}
	}
}
```

- [ ] **Step 5: Innesta la chiamata in `ResolveDash` (`RTTurnManager.cpp:637-654`)**

Nel loop `for (int32 i = 0; i < Units.Num(); ++i) { if (DashAbilityIdx[i] == INDEX_NONE) continue; ... }`,
subito dopo `Unit->SetVisualLocation(...)` (riga 647) e prima di `Unit->PlannedPath.Reset();`:
```cpp
		Unit->SetVisualLocation(Unit->WorldForCell(Final, Origin, CellSize, LayerH));
		ApplyTerrainOnEnterEffects(Snapshot, Unit, Resolved[i].Entered);
		Unit->PlannedPath.Reset();
```

- [ ] **Step 6: Innesta la chiamata in `ResolveMovement` (`RTTurnManager.cpp:1219-1222`)**

```cpp
	// Applica le posizioni finali.
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		Units[i]->PlaceOnCell(Resolved[i].Final, Origin, HexSize, LayerH);
		ApplyTerrainOnEnterEffects(Snapshot, Units[i], Resolved[i].Entered);
	}
```
Rimuovi (o aggiorna, marcandola risolta per il Fuoco) la NOTA a righe 1224-1228: il cross-damage di CP 8.1
copre solo `Fire` tramite `OnEnterEffects` — se restano terreni pericolosi non ancora dichiarati con effetti
(nessuno nel catalogo v0.1 oltre Fire/ShallowWater/Smoke), aggiorna il commento per riflettere lo stato
reale invece di lasciarlo a descrivere un gap ormai chiuso.

- [ ] **Step 7: Esegui e verifica che il test passi**

Run: comando terreni. Expected: `Fire.DamagesAndBurnsOnEnter` PASS. Riesegui anche
`RefactorTactics.HexMove` e `RefactorTactics.HexMap` per escludere regressioni sul movimento esistente.

- [ ] **Step 8: Test aggiuntivi — ShallowWater applica Wet, Conductive no**

Append a `RTTerrainTests.cpp` (verifica sul catalogo, non richiede UWorld — più economica dell'integrazione):
```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainShallowWaterAppliesWetTest,
	"RefactorTactics.Terrain.ShallowWater.AppliesWet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainShallowWaterAppliesWetTest::RunTest(const FString&)
{
	const FRTTerrainDef Def = URTTerrainLibrary::FindTerrainDef(ERTHexSurface::ShallowWater);
	bool bAppliesWet = false;
	for (const FRTActionEffectSpec& Effect : Def.OnEnterEffects)
	{
		if (Effect.Effect == ERTActionEffect::Status && Effect.StatusTag == TAG_Status_Wet) { bAppliesWet = true; }
	}
	TestTrue(TEXT("ShallowWater applica Status.Wet"), bAppliesWet);
	return true;
}
```
(Il caso "Conductive non applica Wet" è già coperto da `FRTTerrainCostsFromCatalogTest`, Task 3 Step 1 —
non duplicarlo.)

- [ ] **Step 9: Esegui tutta la suite terreni**

Run: comando terreni completo. Expected: tutti i test `RefactorTactics.Terrain.*` PASS.

- [ ] **Step 10: Commit**

```bash
git add Source/RefactorTactics/Turn/RTTurnManager.h Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Tests/RTHexMovementIntegrationTests.cpp Source/RefactorTactics/Tests/RTTerrainTests.cpp
git commit -m "feat(terreni): danno e Burning del Fuoco all'ingresso, Move e Dash (CP 8.1)"
```

---

### Task 8: Cap di targeting del Fumo

**Files:**
- Modify: `Source/RefactorTactics/Combat/RTHexCombatLibrary.cpp` (`CollectHexAttacks`)
- Test: `Source/RefactorTactics/Tests/RTTerrainTests.cpp` (append) o `RTHexCombatTests.cpp` se il pattern di
  setup del combat esiste già lì (verificare prima di scegliere il file: preferire quello con il setup più
  simile per non duplicare helper).

**Interfaces:**
- Consuma: `URTTerrainLibrary::FindTerrainDef` (Task 3).

- [ ] **Step 1: Implementazione attuale di `CollectHexAttacks` (per riferimento, righe 85-101)**

```cpp
		if (URTHexLibrary::HexDistance(Attacker.Cell, AimCell) > Intent.RangeCells)
		{
			continue; // fuori portata: scartato in silenzio (come il quadrato)
		}

		// FAIL-CLOSED: senza mappa autorevole non si colpisce. Il motivo resta pero' DISTINTO da una
		// copertura: «non valutabile» e' un difetto di configurazione del livello, non un esito di gioco.
		if (Map == nullptr)
		{
			Plan.UnverifiableIntents.Add(IntentIdx);
			continue;
		}
		if (!URTHexVisionLibrary::HasLineOfSight(Map, Attacker.Cell, AimCell))
		{
			Plan.BlockedIntents.Add(IntentIdx);
			continue;
		}
```
Il check di LOS (`HasLineOfSight`) non espone le celle della linea: per il cap del Fumo va ricalcolata con
`URTHexLibrary::HexLine(Attacker.Cell, AimCell)` (stessa funzione usata da `HexHitCells` per `Shape::Line`,
riga 34). Il check di portata avviene PRIMA del controllo `Map == nullptr`: il cap va quindi guardato da
`Map != nullptr` inline, senza spostare l'ordine dei controlli esistenti (se `Map` è nullo, il path
fail-closed a valle scarta comunque l'intento).

- [ ] **Step 2: Scrivi il test fallito**

Append a `RTTerrainTests.cpp`:
```cpp
#include "Combat/RTHexCombatLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainSmokeLimitsTargetingTest,
	"RefactorTactics.Terrain.Smoke.LimitsTargeting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainSmokeLimitsTargetingTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 5))
	{
		Map->AddOrUpdateCell(FRTHexCellData(Id));
	}
	FRTHexCellData Smoke(FRTCellId(2, 0, 0));
	Smoke.Surface = ERTHexSurface::Smoke;
	Map->AddOrUpdateCell(Smoke);
	Map->SortCells();

	TArray<FRTHexCombatUnit> Units;
	FRTHexCombatUnit Attacker; Attacker.UnitId = 0; Attacker.TeamId = 0; Attacker.Cell = FRTCellId(0, 0, 0);
	FRTHexCombatUnit Target;   Target.UnitId = 1;   Target.TeamId = 1;   Target.Cell = FRTCellId(4, 0, 0);
	Units.Add(Attacker);
	Units.Add(Target);

	FRTHexAttackIntent Intent;
	Intent.AttackerId = 0;
	Intent.TargetId = 1;
	Intent.RangeCells = 6; // la portata dichiarata basterebbe, ma la linea attraversa il Fumo a q=2
	Intent.Power = 10;

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(Intent);

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
	TestEqual(TEXT("intento scartato: oltre il cap di targeting del Fumo (2 celle)"), Plan.Hits.Num(), 0);

	return true;
}
```

- [ ] **Step 3: Esegui e verifica il fallimento**

Run: comando terreni. Expected: FAIL — `Plan.Hits.Num()` è 1 (il cap non esiste ancora, la distanza (4) sta
dentro `RangeCells` (6)).

- [ ] **Step 4: Implementa il cap in `CollectHexAttacks` (sostituisce le righe 85-88 di Step 1)**

```cpp
		int32 EffectiveRange = Intent.RangeCells;
		if (Map != nullptr)
		{
			for (const FRTCellId& LineCell : URTHexLibrary::HexLine(Attacker.Cell, AimCell))
			{
				const FRTHexCellData* LineCellData = Map->FindCell(LineCell);
				if (LineCellData)
				{
					const FRTTerrainDef Terrain = URTTerrainLibrary::FindTerrainDef(LineCellData->Surface);
					if (Terrain.MaxTargetingRangeThrough > 0)
					{
						EffectiveRange = FMath::Min(EffectiveRange, Terrain.MaxTargetingRangeThrough);
					}
				}
			}
		}

		if (URTHexLibrary::HexDistance(Attacker.Cell, AimCell) > EffectiveRange)
		{
			continue; // fuori portata (anche per il cap del Fumo): scartato in silenzio (come il quadrato)
		}
```
Il resto del blocco (righe 90-101 di Step 1: fail-closed su `Map == nullptr`, poi `HasLineOfSight`) resta
**invariato**, subito dopo. Aggiungi `#include "Terrain/RTTerrainLibrary.h"` e
`#include "Map/RTHexCellData.h"` (per `FRTHexCellData`, se non già incluso) in cima al file.

- [ ] **Step 5: Esegui e verifica che passi**

Run: comando terreni. Expected: `Smoke.LimitsTargeting` PASS. Riesegui anche la suite combat esistente
(`RefactorTactics.HexCombat` o equivalente trovato allo Step 1) per escludere regressioni sul targeting
normale (bersagli fuori portata senza Fumo devono restare scartati come prima).

- [ ] **Step 6: Commit**

```bash
git add Source/RefactorTactics/Combat/RTHexCombatLibrary.cpp Source/RefactorTactics/Tests/RTTerrainTests.cpp
git commit -m "feat(terreni): il Fumo limita il targeting a 2 celle lungo la linea di tiro (CP 8.1)"
```

---

## Chiusura CP 8.1

Dopo Task 8: eseguire l'intera suite (`-ExecCmds="Automation RunTests RefactorTactics"`), verificare il
conteggio totale contro la baseline pre-CP 8.1, aggiornare `docs/design/v0.1-issue-plan.md` (checkbox DoD
di `#64` + dipendenze dichiarate: HighGround "bonus visuale" non consumato, Scatto su Ice non innesca
sliding), aprire la PR verso `main` con i limiti dichiarati (stesso pattern di `#57`/`#58`), aggiornare lo
stato dell'issue `#64` su GitHub.
