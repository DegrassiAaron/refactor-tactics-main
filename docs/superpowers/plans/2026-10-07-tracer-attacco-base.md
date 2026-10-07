# Il tracer degli attacchi base — piano di implementazione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** nel playback del Blast, un attacco base `Single` lancia un proiettile e un attacco base `Line` un getto,
e il colpo (`Hit`, numero, barra, log) arriva **dopo** il lancio — senza allungare la fase e senza rivelare la
cella di un attaccante che chi guarda non vedeva.

**Architecture:** il produttore arricchisce ogni evento `Attack` con la geometria del colpo (`ResolveImpactOrigin`
+ cella della vittima) e con due verdetti di conoscenza congelati col predicato dei fatti puntuali (D-223). Il
playback percorre ogni colpo in **due battiti** — lancio e arrivo — con un cursore solo, e a ogni tick consegna
alla mappa il tracer in volo, che la mappa disegna col line batcher del mondo. Tutta la logica di tempo, idoneità e
stile vive in funzioni pure di `URTPlaybackLibrary`.

**Tech Stack:** Unreal Engine 5.8.1 (installed build), C++, Automation Test (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`),
scenari JSON dello Scenario Harness, registro PIE in Markdown/YAML.

**Spec:** [`docs/superpowers/specs/2026-10-07-tracer-attacco-base-design.md`](../specs/2026-10-07-tracer-attacco-base-design.md)
— il piano argomenta da lì; chi esegue legge entrambi. Owner: [#2454](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2454).

## Global Constraints

- Unreal Engine **5.8.1** fissato dal repository; nessun aggiornamento di Engine, plugin o toolchain.
- ⛔ Nessun `.uasset`, nessun Niagara, nessun asset VFX ([D-124](../../decisions/RT_PDR_00_Decision_Log.md)).
- Colore **solo** da `URTOverlayPalette::ColorFor(ERTOverlayMeaning::Attack)`; ⛔ nessun `FColor` letterale nuovo.
- Il disegno passa da `DisegnaLineaAnteprima` (line batcher del mondo, [D-467](../../decisions/RT_PDR_00_Decision_Log.md)); ⛔ mai `DrawDebug*`.
- ⛔ La presentazione non chiama `HexHitCells`, LOS o targeting.
- ⛔ Nessun dato nuovo in `StateHash`, TurnLog, snapshot o replay.
- `F_eff = Min(TracerFlightSeconds, 0.5 · AttackShowSeconds)` per un colpo idoneo, `0` altrimenti; `TracerFlightSeconds` default `0.25`; `AttackShowSeconds <= 0` → `F_eff = 0`.
- Idoneità: (`ActionId == Action.BasicAttack` **o** `BaseActionId == Action.BasicAttack`) **e** `Shape ∈ {Single, Line}` **e** `HitGeometry.bResolved`.
- Disegno: solo se `HitGeometry.FromVerdict.AllowsTeam(Viewer) && HitGeometry.ImpactVerdict.AllowsTeam(Viewer)`.
- Verdetti col predicato dei fatti puntuali: `FreezeVerdictFor(FRTLogSubject::UnitAt(...))`; ⛔ non `FreezeRouteCellVerdict`.
- Nomi dei test: `RefactorTactics.<Area>.<Nome>`. In unity build i test condividono la TU: gli helper in namespace anonimo hanno **nomi unici per file**.
- Commenti in italiano, densità e simboli (`🔑`, `⛔`, `⚠️`) come il codice accanto.
- **Motore uno per macchina** (`CLAUDE.md` §10, `AGENTS.md` §9 e §11): prima di ogni build o suite leggi le `CommandLine` dei processi `UnrealEditor*`; se una sessione sta misurando un **tempo**, aspetta; se il motore è occupato e non puoi aspettare, dichiara `NOT RUN` col nome del clone che lo tiene.
- Nessun totale volatile in documenti, issue o PR (`AGENTS.md` §14).

### Comandi (da `AGENTS.md` §9)

```powershell
# chi tiene il motore — leggi la CommandLine, non contare i processi
Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, Name, CommandLine

# build (Editor chiuso su QUESTO clone)
& "D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development `
    -Project="D:/Repositories/rt-wt-2454-tracer/RefactorTactics.uproject" -WaitMutex -NoHotReloadFromIDE

# suite (un gruppo) — dal worktree
python tools/suite/esegui.py RefactorTactics.Playback
```

## Review Focus

1. **Tick lungo** (velocità ×4, un fotogramma lento): i battiti escono `L0, A0, L1, A1` e mai `L0, L1, A0, A1` — Task 5, `Playback.AttackBeatsStayOrderedInOneTick`.
2. **Blast sospeso da una finestra di reazione e poi esteso** (`bPreserveClock`): nessun colpo arriva due volte — Task 5, `Reactions.Brace.ExtendedBlastDoesNotReplayHits`.
3. **`Next Action` che si ferma all'arrivo**: nessun tracer resta a mezz'aria accanto al suo numero — Task 6, estensione di `Playback.NextActionStopsAtTheActionBoundary`.
4. **`SkipPlayback` con un tracer in volo**: il canale si spegne — Task 6, `Playback.TracerChannelClearsAtBlastEnd`.
5. **Attaccante non visto dalla squadra colpita**: il verdetto dell'origine la esclude, e lo stile per quella squadra è `None` — Task 4, `Privacy.UnseenAttackerIsOutOfTheOriginVerdict`.

---

### Task 1: Il canale del tracer sulla mappa

**Files:**
- Create: `Source/RefactorTactics/Map/RTPlaybackTracer.h`
- Modify: `Source/RefactorTactics/Turn/RTPlaybackLibrary.h`, `Source/RefactorTactics/Turn/RTPlaybackLibrary.cpp`
- Modify: `Source/RefactorTactics/Map/RTHexMapActor.h` (accanto a `AddPlaybackFootprint`, `:862-918`), `Source/RefactorTactics/Map/RTHexMapActor.cpp` (`HasAnythingToDraw` `:1084-1097`; canale dopo `ClearPlaybackFootprint` `:1162-1166`; disegno dopo i colpi a struttura, `:1588-1625`)
- Test: `Source/RefactorTactics/Tests/RTPlaybackLibraryTests.cpp`, `Source/RefactorTactics/Tests/RTHexMapActorTests.cpp`, `Source/RefactorTactics/Tests/RTPreviewLineBatcherTests.cpp`

**Interfaces:**
- Produces: `enum class ERTTracerStyle : uint8 { None, Projectile, Jet }`; `struct FRTPlaybackTracer { FRTCellId From; FRTCellId To; float Alpha; ERTTracerStyle Style; }`; `static void URTPlaybackLibrary::TracerSegment(ERTTracerStyle Style, const FVector& From, const FVector& To, float Alpha, float DashLength, FVector& OutStart, FVector& OutEnd)`; `void ARTHexMapActor::SetPlaybackTracers(const TArray<FRTPlaybackTracer>&)`, `void ARTHexMapActor::ClearPlaybackTracers()`, `int32 ARTHexMapActor::NumPlaybackTracers() const`, `const TArray<FRTPlaybackTracer>& ARTHexMapActor::GetPlaybackTracers() const`.

- [ ] **Step 1: Write the failing tests**

In `RTPlaybackLibraryTests.cpp`, dopo il blocco `AttacksToShow`:

```cpp
// --- TracerSegment (`#2454`) ------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackTracerSegmentShapesTest,
	"RefactorTactics.Playback.TracerSegmentShapes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackTracerSegmentShapesTest::RunTest(const FString&)
{
	// 🔑 Le due forme si distinguono per GEOMETRIA, non per colore: il getto resta ancorato all'origine, il
	// proiettile se ne stacca. Se le due righe del ramo si scambiassero, cadono le asserzioni sulla coda.
	const FVector A(0.f, 0.f, 0.f);
	const FVector B(1000.f, 0.f, 0.f);
	FVector S, E;

	URTPlaybackLibrary::TracerSegment(ERTTracerStyle::Jet, A, B, 0.5f, 100.f, S, E);
	TestTrue(TEXT("il getto resta ancorato all'origine"), S.Equals(A, RTTol));
	TestTrue(TEXT("e arriva fin dove e' arrivato il volo"), E.Equals(FVector(500.f, 0.f, 0.f), RTTol));

	URTPlaybackLibrary::TracerSegment(ERTTracerStyle::Projectile, A, B, 0.5f, 100.f, S, E);
	TestTrue(TEXT("la testa del proiettile e' dove e' arrivato il volo"), E.Equals(FVector(500.f, 0.f, 0.f), RTTol));
	TestTrue(TEXT("la coda e' a un dardo di distanza, NON all'origine"), S.Equals(FVector(400.f, 0.f, 0.f), RTTol));

	URTPlaybackLibrary::TracerSegment(ERTTracerStyle::Projectile, A, B, 0.05f, 100.f, S, E);
	TestTrue(TEXT("in partenza la coda non scavalca l'origine"), S.Equals(A, RTTol));

	URTPlaybackLibrary::TracerSegment(ERTTracerStyle::Jet, A, B, 2.f, 100.f, S, E);
	TestTrue(TEXT("un avanzamento oltre 1 non supera l'impatto"), E.Equals(B, RTTol));

	URTPlaybackLibrary::TracerSegment(ERTTracerStyle::None, A, B, 0.5f, 100.f, S, E);
	TestTrue(TEXT("None non disegna un segmento: i due estremi coincidono"), S.Equals(E, RTTol));
	return true;
}
```

In `RTHexMapActorTests.cpp`, subito dopo `PlaybackFootprintIsItsOwnChannel` (`:1797-1840`), con gli helper del file
(`MakeMapActorWorld`, `MakeActorTestAsset`, `SpawnMapActor`, `DestroyMapActorWorld`):

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorPlaybackTracerChannelTest,
	"RefactorTactics.HexMapActor.PlaybackTracerIsItsOwnChannel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorPlaybackTracerChannelTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	URTHexMapAsset* Asset = MakeActorTestAsset(/*Radius*/ 1);
	ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
	TestNotNull(TEXT("actor spawnato"), Actor);
	if (!Actor) { DestroyMapActorWorld(World); return false; }

	FRTPlaybackTracer T;
	T.From = FRTCellId(0, 0);
	T.To = FRTCellId(1, 0);
	T.Alpha = 0.5f;
	T.Style = ERTTracerStyle::Projectile;

	TestEqual(TEXT("si parte senza tracer"), Actor->NumPlaybackTracers(), 0);
	Actor->SetPlaybackTracers({ T });
	TestEqual(TEXT("un tracer dopo la consegna"), Actor->NumPlaybackTracers(), 1);

	// 🔑 **SOSTITUZIONE, non accumulo**: il volo e' funzione dell'orologio, e ogni tick consegna lo stato
	// intero. Se `Set` diventasse un `Append`, la seconda consegna lascerebbe due tracer.
	Actor->SetPlaybackTracers({ T });
	TestEqual(TEXT("la seconda consegna sostituisce la prima"), Actor->NumPlaybackTracers(), 1);

	// ⛔ **I canali non si toccano**: impronta e anteprima non spengono il tracer, e il tracer non spegne loro.
	Actor->AddPlaybackFootprint({ FRTCellId(0, 0) });
	Actor->ClearPlaybackFootprint();
	Actor->SetPreviewHitCells({ FRTCellId(0, 0) }, {});
	Actor->SetPreviewHitCells({}, {});
	TestEqual(TEXT("spenti impronta e anteprima, il tracer resta"), Actor->NumPlaybackTracers(), 1);

	Actor->AddPlaybackFootprint({ FRTCellId(1, 0) });
	Actor->ClearPlaybackTracers();
	TestEqual(TEXT("dopo Clear non resta nessun tracer"), Actor->NumPlaybackTracers(), 0);
	TestEqual(TEXT("e l'impronta non e' stata toccata"), Actor->NumPlaybackFootprintCells(), 1);

	Actor->SetPlaybackTracers({});
	TestEqual(TEXT("una consegna vuota e' un canale vuoto"), Actor->NumPlaybackTracers(), 0);

	DestroyMapActorWorld(World);
	return true;
}
```

In `RTPreviewLineBatcherTests.cpp`, con gli helper del file (`MakeLineeAnteprimaWorld`, `DestroyLineeAnteprimaWorld`,
`RTTestConsoleVariable::TGuardia`) e `#include "Map/RTPlaybackTracer.h"`:

```cpp
// Il tracer del playback si disegna anche col debug spento (`#2454`), come l'anteprima (`#3508`, D-467).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPreviewTracerDrawsWithDebugDrawingOffTest,
	"RefactorTactics.Preview.TracerDrawsWithDebugDrawingOff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPreviewTracerDrawsWithDebugDrawingOffTest::RunTest(const FString&)
{
	UWorld* World = MakeLineeAnteprimaWorld();
	if (!TestNotNull(TEXT("World creato"), World)) { return false; }

	ULineBatchComponent* Primo = World->GetLineBatcher(UWorld::ELineBatcherType::Foreground);
	IConsoleVariable* Debug = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EnableDrawDebugHelpers"));
	if (!TestTrue(TEXT("premessa: batcher Foreground e CVar di debug presenti"), Primo != nullptr && Debug != nullptr))
	{
		DestroyLineeAnteprimaWorld(World);
		return false;
	}
	RTTestConsoleVariable::TGuardia<int32> DebugSpento(*Debug, 0);

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	if (!TestNotNull(TEXT("HexMap spawnato"), HexMap)) { DestroyLineeAnteprimaWorld(World); return false; }
	HexMap->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);

	FRTPlaybackTracer T;
	T.From = FRTCellId(0, 0);
	T.To = FRTCellId(2, 0);
	T.Alpha = 0.5f;
	T.Style = ERTTracerStyle::Projectile;
	HexMap->SetPlaybackTracers({ T });

	Primo->Flush();
	static_cast<AActor*>(HexMap)->Tick(0.f);

	// 🔑 Un tracer, UNA linea, nel batcher Foreground: attraversa le unita' come la linea di mira.
	TestEqual(TEXT("una linea nel batcher Foreground per il tracer"), Primo->BatchedLines.Num(), 1);
	if (Primo->BatchedLines.Num() == 1)
	{
		const FLinearColor Attacco(URTOverlayPalette::ColorFor(ERTOverlayMeaning::Attack));
		TestTrue(TEXT("col colore Attack della palette"), Primo->BatchedLines[0].Color.Equals(Attacco));
	}

	DestroyLineeAnteprimaWorld(World);
	return true;
}
```

- [ ] **Step 2: Run the build to verify the tests fail**

Run: il comando di build in testa al piano.
Expected: FAIL di compilazione — `ERTTracerStyle`, `FRTPlaybackTracer`, `TracerSegment`, `SetPlaybackTracers` non esistono.

- [ ] **Step 3: Write the types**

`Source/RefactorTactics/Map/RTPlaybackTracer.h`:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "RTPlaybackTracer.generated.h"

/**
 * La forma di un tracer di playback (`#2454`, spec `2026-10-07-tracer-attacco-base`).
 *
 * ⛔ **Il canale e' la GEOMETRIA, mai il solo colore**: le due forme hanno lo stesso colore della palette e si
 * distinguono perche' il getto resta ancorato all'origine e il proiettile se ne stacca.
 */
UENUM(BlueprintType)
enum class ERTTracerStyle : uint8
{
	/** Nessun tracer: colpo non idoneo, oppure chi guarda non conosceva uno dei due estremi. */
	None,
	/** Un segmento corto che si sposta dall'origine all'impatto (`ERTAbilityShape::Single`). */
	Projectile,
	/** Un segmento che si allunga dall'origine, ancorato a essa (`ERTAbilityShape::Line`). */
	Jet,
};

/**
 * Un tracer in volo nel fotogramma corrente: estremi in CELLE, avanzamento in [0,1].
 *
 * 🔑 Le celle e non le posizioni degli attori: la posizione visiva di un'unita' poco osservata puo' essere
 * stantia, e un estremo letto dall'attore sarebbe presentazione che si fa passare per fatto (spec §2.2).
 */
USTRUCT()
struct FRTPlaybackTracer
{
	GENERATED_BODY()

	UPROPERTY()
	FRTCellId From;

	UPROPERTY()
	FRTCellId To;

	UPROPERTY()
	float Alpha = 0.f;

	UPROPERTY()
	ERTTracerStyle Style = ERTTracerStyle::None;
};
```

In `RTPlaybackLibrary.h` aggiungi `#include "Map/RTPlaybackTracer.h"` e, nella classe, dopo `BlastPhaseIsActive`:

```cpp
	/**
	 * Il segmento da disegnare per un tracer (`#2454`). Pura: estremi nel mondo, avanzamento, lunghezza del dardo.
	 *
	 * - `Jet`: dall'origine fino al punto raggiunto — ancorato.
	 * - `Projectile`: un dardo lungo al piu' `DashLength` che termina nel punto raggiunto — staccato.
	 * - `None`: un segmento degenere (i due estremi coincidono).
	 * `Alpha` si taglia in [0,1]: un avanzamento oltre la fine non supera l'impatto.
	 */
	static void TracerSegment(ERTTracerStyle Style, const FVector& From, const FVector& To, float Alpha,
		float DashLength, FVector& OutStart, FVector& OutEnd);
```

In `RTPlaybackLibrary.cpp`:

```cpp
void URTPlaybackLibrary::TracerSegment(ERTTracerStyle Style, const FVector& From, const FVector& To, float Alpha,
	float DashLength, FVector& OutStart, FVector& OutEnd)
{
	const FVector Head = FMath::Lerp(From, To, FMath::Clamp(Alpha, 0.f, 1.f));
	if (Style == ERTTracerStyle::Jet)
	{
		OutStart = From;
		OutEnd = Head;
		return;
	}
	if (Style == ERTTracerStyle::Projectile)
	{
		// La coda non scavalca l'origine: in partenza il dardo e' piu' corto, non sporge dietro chi spara.
		const FVector Back = From - Head;
		const float Len = Back.Size();
		OutStart = Len > KINDA_SMALL_NUMBER ? Head + Back * (FMath::Min(FMath::Max(0.f, DashLength), Len) / Len) : Head;
		OutEnd = Head;
		return;
	}
	OutStart = Head;
	OutEnd = Head;
}
```

- [ ] **Step 4: Write the map channel**

In `RTHexMapActor.h` aggiungi `#include "Map/RTPlaybackTracer.h"` e, dopo il blocco dell'impronta di playback
(accanto a `NumPlaybackFootprintCells`, `:899`):

```cpp
	/**
	 * I tracer in volo nel fotogramma corrente (`#2454`). Li consegna `ARTTurnManager` a ogni tick del Blast e li
	 * SOSTITUISCE in blocco: il volo e' funzione dell'orologio del playback, non uno stato che si accumula.
	 *
	 * Separato dall'impronta come quella lo e' dall'anteprima: lo spegne `ClearPlaybackTracers`, chiamato a fine
	 * Blast e da `FinishPlayback`. ⛔ Nessun filtro qui: la conoscenza l'ha gia' applicata chi consegna.
	 */
	void SetPlaybackTracers(const TArray<FRTPlaybackTracer>& Tracers);

	/** Spegne il canale. */
	void ClearPlaybackTracers();

	int32 NumPlaybackTracers() const { return PlaybackTracers.Num(); }
	const TArray<FRTPlaybackTracer>& GetPlaybackTracers() const { return PlaybackTracers; }
```

e accanto a `PlaybackFootprintCells` (`:1061`):

```cpp
	/** I tracer del fotogramma corrente: vedi `SetPlaybackTracers`. */
	TArray<FRTPlaybackTracer> PlaybackTracers;
```

In `RTHexMapActor.cpp`:
- `#include "Turn/RTPlaybackLibrary.h"`;
- in `HasAnythingToDraw`, accanto a `PlaybackStructureHits.Num() > 0`: `|| PlaybackTracers.Num() > 0`;
- dopo `ClearPlaybackFootprint`:

```cpp
void ARTHexMapActor::SetPlaybackTracers(const TArray<FRTPlaybackTracer>& Tracers)
{
	PlaybackTracers = Tracers;
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::ClearPlaybackTracers()
{
	PlaybackTracers.Reset();
	SetActorTickEnabled(HasAnythingToDraw());
}
```

- nel namespace anonimo che dichiara `RTLiftPreview`, quattro costanti di resa, tarate in PIE:

```cpp
	// Tracer del playback (`#2454`): altezza sopra la cella, lunghezza del dardo in frazioni di `HexSize`,
	// spessori. ⚠️ Valori di GRAYBOX, tarati in PIE (`PIE-V01-TRACER`): il getto e' piu' spesso del proiettile
	// perche' le due forme devono separarsi anche in un fotogramma fermo.
	constexpr float RTTracerHeight = 60.f;
	constexpr float RTTracerDashFraction = 0.35f;
	constexpr float RTTracerProjectileThickness = 4.f;
	constexpr float RTTracerJetThickness = 7.f;
```

- in `DrawPlanningPreview`, dopo il blocco `if (PlaybackStructureHits.Num() > 0) { ... }` e prima di `if (bHoveredValid)`:

```cpp
	// Tracer degli attacchi base IN VOLO, durante il playback (`#2454`).
	//
	// 🔑 **Stesso significato di un colpo, quindi stesso colore**: `ERTOverlayMeaning::Attack`, come l'impronta e i
	// muri qui sopra. ⛔ Nessun `FColor` letterale e nessun significato nuovo (`#1941`): proiettile e getto si
	// separano per GEOMETRIA, che e' `URTPlaybackLibrary::TracerSegment`.
	// ⚠️ **Foreground**: il tracer attraversa le unita' come la linea di mira, o sparirebbe dentro chi spara.
	if (PlaybackTracers.Num() > 0)
	{
		const FColor TracerColor = URTOverlayPalette::ColorFor(ERTOverlayMeaning::Attack);
		for (const FRTPlaybackTracer& T : PlaybackTracers)
		{
			const FVector Da = URTHexLibrary::AxialToWorld(T.From, Origin, Size, LayerH)
				+ FVector(0, 0, CellLift(T.From) + RTTracerHeight);
			const FVector A = URTHexLibrary::AxialToWorld(T.To, Origin, Size, LayerH)
				+ FVector(0, 0, CellLift(T.To) + RTTracerHeight);
			FVector Inizio, Fine;
			URTPlaybackLibrary::TracerSegment(T.Style, Da, A, T.Alpha, Size * RTTracerDashFraction, Inizio, Fine);
			DisegnaLineaAnteprima(World, Inizio, Fine, TracerColor, SDPG_Foreground,
				T.Style == ERTTracerStyle::Jet ? RTTracerJetThickness : RTTracerProjectileThickness);
		}
	}
```

⚠️ Verifica che `World`, `Origin`, `Size`, `LayerH` e `CellLift` siano i nomi in scope nel blocco dei muri qui sopra
(`:1606-1611`); se differiscono, usa quelli e non introdurne di nuovi.

- [ ] **Step 5: Build and run the tests**

Run: build; poi `python tools/suite/esegui.py RefactorTactics.Playback.TracerSegmentShapes+RefactorTactics.HexMapActor.PlaybackTracerIsItsOwnChannel+RefactorTactics.Preview`
Expected: PASS su tutti, compreso `Preview.DrawsWithDebugDrawingOff` invariato (il canale è vuoto lì).

- [ ] **Step 6: Mutation check**

Cambia `PlaybackTracers = Tracers;` in `PlaybackTracers.Append(Tracers);`, ricompila, lancia
`RefactorTactics.HexMapActor.PlaybackTracerIsItsOwnChannel`. Expected: FAIL su *«la seconda consegna sostituisce la
prima»*. Ripristina, ricompila, rilancia: PASS.

- [ ] **Step 7: Commit**

```bash
git add Source/RefactorTactics/Map/RTPlaybackTracer.h Source/RefactorTactics/Turn/RTPlaybackLibrary.h Source/RefactorTactics/Turn/RTPlaybackLibrary.cpp Source/RefactorTactics/Map/RTHexMapActor.h Source/RefactorTactics/Map/RTHexMapActor.cpp Source/RefactorTactics/Tests/RTPlaybackLibraryTests.cpp Source/RefactorTactics/Tests/RTHexMapActorTests.cpp Source/RefactorTactics/Tests/RTPreviewLineBatcherTests.cpp
git commit -m "feat(2454): il canale del tracer sulla mappa — proiettile e getto, col line batcher del mondo"
```

---

### Task 2: Il tempo del colpo, puro

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTPlaybackLibrary.h`, `Source/RefactorTactics/Turn/RTPlaybackLibrary.cpp`
- Test: `Source/RefactorTactics/Tests/RTPlaybackLibraryTests.cpp`

**Interfaces:**
- Consumes: nulla dai task precedenti.
- Produces: `static float URTPlaybackLibrary::TracerFlightFor(bool bEligible, float TracerFlightSeconds, float AttackShowSeconds)`; `static float URTPlaybackLibrary::AttackBeatSeconds(int32 Beat, float AttackShowSeconds, const TArray<float>& Flights)`; `static int32 URTPlaybackLibrary::AttackBeatsDue(float PhaseElapsed, float AttackShowSeconds, const TArray<float>& Flights)`; `static float URTPlaybackLibrary::TracerAlpha(int32 AttackIndex, float PhaseElapsed, float AttackShowSeconds, float Flight)`. Convenzione: il battito `2i` è il **lancio** del colpo `i`, il battito `2i+1` il suo **arrivo**.

- [ ] **Step 1: Write the failing tests**

```cpp
// --- Il tempo del tracer (`#2454`, spec §2.3) --------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackTracerArrivesAfterFlightTest,
	"RefactorTactics.Playback.TracerArrivesAfterFlight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackTracerArrivesAfterFlightTest::RunTest(const FString&)
{
	const float A = 0.5f;
	const float F = URTPlaybackLibrary::TracerFlightFor(/*bEligible=*/ true, 0.25f, A);
	TestEqual(TEXT("un colpo idoneo vola per il tempo dichiarato"), F, 0.25f, RTTol);
	TestEqual(TEXT("un colpo non idoneo non vola"), URTPlaybackLibrary::TracerFlightFor(false, 0.25f, A), 0.f, RTTol);

	const TArray<float> Flights = { F };
	TestEqual(TEXT("il lancio e' a fase appena iniziata"), URTPlaybackLibrary::AttackBeatSeconds(0, A, Flights), 0.f, RTTol);
	TestEqual(TEXT("l'arrivo e' dopo il volo"), URTPlaybackLibrary::AttackBeatSeconds(1, A, Flights), 0.25f, RTTol);

	// 🔴 **La mutazione dichiarata**: con il volo a zero il lancio e l'arrivo cadono nello stesso istante, e
	// queste due righe cadono — e' la forma di oggi, quella che #2454 chiede di superare.
	TestEqual(TEXT("a 0.1 s e' uscito il lancio e non l'arrivo"), URTPlaybackLibrary::AttackBeatsDue(0.1f, A, Flights), 1);
	TestEqual(TEXT("a 0.25 s e' uscito anche l'arrivo"), URTPlaybackLibrary::AttackBeatsDue(0.25f, A, Flights), 2);

	TestEqual(TEXT("a meta' volo l'avanzamento e' 0.5"), URTPlaybackLibrary::TracerAlpha(0, 0.125f, A, F), 0.5f, RTTol);
	TestEqual(TEXT("prima del lancio e' 0"), URTPlaybackLibrary::TracerAlpha(1, 0.1f, A, F), 0.f, RTTol);
	TestEqual(TEXT("senza volo e' gia' 1"), URTPlaybackLibrary::TracerAlpha(0, 0.f, A, 0.f), 1.f, RTTol);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackTracerFlightNeverOutlastsTheSlotTest,
	"RefactorTactics.Playback.TracerFlightNeverOutlastsTheSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackTracerFlightNeverOutlastsTheSlotTest::RunTest(const FString&)
{
	// 🔑 **Il Blast non si allunga** (spec §2.3): per ogni volo chiesto, `F_eff <= A/2`, quindi l'arrivo di un
	// colpo precede il lancio del successivo e l'ultimo arrivo cade prima di `N·A`.
	const float A = 0.5f;
	for (const float Chiesto : { 0.f, 0.1f, 0.25f, 0.49f, 0.5f, 3.f })
	{
		const float F = URTPlaybackLibrary::TracerFlightFor(true, Chiesto, A);
		TestTrue(FString::Printf(TEXT("volo chiesto %.2f: F_eff %.3f <= A/2"), Chiesto, F), F <= 0.5f * A + RTTol);

		// Idonei e non idonei alternati: gli arrivi restano monotoni anche mescolandoli.
		const TArray<float> Flights = { F, 0.f, F, F };
		for (int32 Beat = 0; Beat + 1 < 2 * Flights.Num(); ++Beat)
		{
			TestTrue(FString::Printf(TEXT("volo %.2f: battito %d non dopo il %d"), Chiesto, Beat, Beat + 1),
				URTPlaybackLibrary::AttackBeatSeconds(Beat, A, Flights)
					<= URTPlaybackLibrary::AttackBeatSeconds(Beat + 1, A, Flights) + RTTol);
		}
		const float UltimoArrivo = URTPlaybackLibrary::AttackBeatSeconds(2 * Flights.Num() - 1, A, Flights);
		TestTrue(FString::Printf(TEXT("volo %.2f: ultimo arrivo %.3f < N*A"), Chiesto, UltimoArrivo),
			UltimoArrivo < Flights.Num() * A);
		TestEqual(FString::Printf(TEXT("volo %.2f: a N*A tutti i battiti sono usciti"), Chiesto),
			URTPlaybackLibrary::AttackBeatsDue(Flights.Num() * A, A, Flights), 2 * Flights.Num());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackTracerZeroFlightKeepsTodaysRhythmTest,
	"RefactorTactics.Playback.TracerZeroFlightKeepsTodaysRhythm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackTracerZeroFlightKeepsTodaysRhythmTest::RunTest(const FString&)
{
	// Senza volo, gli arrivi escono come i colpi di oggi: `AttacksToShow` e' l'oracolo, non un secondo calcolo.
	const float A = 0.5f;
	const TArray<float> Flights = { 0.f, 0.f, 0.f, 0.f };
	for (const float T : { 0.f, 0.49f, 0.5f, 1.f, 1.6f, 10.f })
	{
		TestEqual(FString::Printf(TEXT("t=%.2f: arrivi = colpi di oggi"), T),
			URTPlaybackLibrary::AttackBeatsDue(T, A, Flights) / 2,
			URTPlaybackLibrary::AttacksToShow(Flights.Num(), T, A));
	}
	// `AttackShowSeconds <= 0`: nessuno scaglionamento, tutti i battiti subito — come `AttacksToShow`.
	TestEqual(TEXT("A<=0: niente volo"), URTPlaybackLibrary::TracerFlightFor(true, 0.25f, 0.f), 0.f, RTTol);
	TestEqual(TEXT("A<=0: tutti i battiti subito"), URTPlaybackLibrary::AttackBeatsDue(0.f, 0.f, Flights), 8);
	TestEqual(TEXT("nessun colpo, nessun battito"), URTPlaybackLibrary::AttackBeatsDue(5.f, A, {}), 0);
	return true;
}
```

- [ ] **Step 2: Run the build to verify the tests fail**

Expected: FAIL di compilazione — le quattro funzioni non esistono.

- [ ] **Step 3: Write the implementation**

`RTPlaybackLibrary.h`, dopo `TracerSegment`:

```cpp
	/**
	 * Il volo effettivo di un colpo (`#2454`, spec §2.3): `Min(TracerFlightSeconds, A/2)` se idoneo, 0 altrimenti.
	 *
	 * 🔑 Il tetto a `A/2` e' cio' che tiene il Blast della sua durata: l'arrivo di un colpo precede il lancio del
	 * successivo, e l'ultimo arrivo cade prima di `N·A` — la durata che `PhaseTime` gia' calcola.
	 * Con `AttackShowSeconds <= 0` non c'e' scaglionamento, quindi nemmeno volo.
	 */
	static float TracerFlightFor(bool bEligible, float TracerFlightSeconds, float AttackShowSeconds);

	/**
	 * L'istante di un battito, misurato dall'inizio del Blast. Il battito `2i` e' il LANCIO del colpo `i`
	 * (`i·A`, lo stesso istante di `AttacksToShow`), il `2i+1` il suo ARRIVO (`i·A + Flights[i]`).
	 */
	static float AttackBeatSeconds(int32 Beat, float AttackShowSeconds, const TArray<float>& Flights);

	/**
	 * Quanti battiti sono usciti a `PhaseElapsed`: la lunghezza del prefisso con istante `<= t`.
	 * ⚠️ E' un PREFISSO perche' la sequenza e' monotona (`Flights[i] <= A/2`): chi la percorre con un cursore
	 * solo vede `L0, A0, L1, A1, ...` anche in un tick lungo. Con `A <= 0` escono tutti.
	 */
	static int32 AttackBeatsDue(float PhaseElapsed, float AttackShowSeconds, const TArray<float>& Flights);

	/** L'avanzamento in [0,1] del tracer del colpo `AttackIndex`; senza volo vale 1. */
	static float TracerAlpha(int32 AttackIndex, float PhaseElapsed, float AttackShowSeconds, float Flight);
```

`RTPlaybackLibrary.cpp`:

```cpp
float URTPlaybackLibrary::TracerFlightFor(bool bEligible, float TracerFlightSeconds, float AttackShowSeconds)
{
	if (!bEligible || AttackShowSeconds <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(TracerFlightSeconds, 0.f, 0.5f * AttackShowSeconds);
}

float URTPlaybackLibrary::AttackBeatSeconds(int32 Beat, float AttackShowSeconds, const TArray<float>& Flights)
{
	const int32 Index = Beat / 2;
	const float Lancio = Index * FMath::Max(0.f, AttackShowSeconds);
	if (Beat % 2 == 0)
	{
		return Lancio;
	}
	return Lancio + (Flights.IsValidIndex(Index) ? FMath::Max(0.f, Flights[Index]) : 0.f);
}

int32 URTPlaybackLibrary::AttackBeatsDue(float PhaseElapsed, float AttackShowSeconds, const TArray<float>& Flights)
{
	const int32 NumBeats = 2 * Flights.Num();
	if (AttackShowSeconds <= 0.f)
	{
		return NumBeats; // nessuno scaglionamento richiesto: come `AttacksToShow`
	}
	const float T = FMath::Max(0.f, PhaseElapsed);
	int32 Due = 0;
	while (Due < NumBeats && AttackBeatSeconds(Due, AttackShowSeconds, Flights) <= T)
	{
		++Due;
	}
	return Due;
}

float URTPlaybackLibrary::TracerAlpha(int32 AttackIndex, float PhaseElapsed, float AttackShowSeconds, float Flight)
{
	if (Flight <= 0.f)
	{
		return 1.f;
	}
	const float Lancio = AttackIndex * FMath::Max(0.f, AttackShowSeconds);
	return FMath::Clamp((PhaseElapsed - Lancio) / Flight, 0.f, 1.f);
}
```

- [ ] **Step 4: Build and run the tests**

Run: `python tools/suite/esegui.py RefactorTactics.Playback`
Expected: PASS, comprese `AttacksToShowStagger`, `PhaseDurationBlastTakesTheLongerOfShotsAndKnockback` ed
`EveryChannelIsFullyRevealedByPhaseEnd` invariate.

- [ ] **Step 5: Mutation check**

In `TracerFlightFor` sostituisci il `return FMath::Clamp(...)` con `return 0.f;`. Expected: FAIL su
`TracerArrivesAfterFlight` (*«a 0.1 s e' uscito il lancio e non l'arrivo»*). Ripristina: PASS.

- [ ] **Step 6: Commit**

```bash
git add Source/RefactorTactics/Turn/RTPlaybackLibrary.h Source/RefactorTactics/Turn/RTPlaybackLibrary.cpp Source/RefactorTactics/Tests/RTPlaybackLibraryTests.cpp
git commit -m "feat(2454): il tempo del colpo in due battiti, puro — lancio, arrivo, volo tagliato a meta' slot"
```

---

### Task 3: Il dato del colpo, l'idoneità e lo stile

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTResolvedEvent.h` (prima di `FRTResolvedEvent`, `:182`; intestazione di `Shape`, `:334`; in coda alla struct, prima di `FRTResolvedEvent() = default;`, `:515`)
- Modify: `Source/RefactorTactics/Turn/RTPlaybackLibrary.h`, `Source/RefactorTactics/Turn/RTPlaybackLibrary.cpp`
- Test: `Source/RefactorTactics/Tests/RTPlaybackLibraryTests.cpp`

**Interfaces:**
- Consumes: `ERTTracerStyle` (Task 1).
- Produces: `struct FRTHitGeometry { bool bResolved; FRTCellId From; FRTCellId Impact; FRTKnowledgeVerdict FromVerdict; FRTKnowledgeVerdict ImpactVerdict; }`; campo `FRTHitGeometry FRTResolvedEvent::HitGeometry`; `static bool URTPlaybackLibrary::IsTracerEligible(const FRTResolvedEvent& Ev)`; `static ERTTracerStyle URTPlaybackLibrary::TracerStyleFor(const FRTResolvedEvent& Ev, int32 ViewerTeamId)`.

- [ ] **Step 1: Write the failing tests**

In `RTPlaybackLibraryTests.cpp` (aggiungi `#include "Turn/RTResolvedEvent.h"` se manca), un helper e tre test:

```cpp
namespace
{
	/** Un `Attack` idoneo e visibile alle squadre 0 e 1 (nome unico per file: unity build). */
	FRTResolvedEvent MakeTracerAttackEvent(ERTAbilityShape Shape)
	{
		FRTResolvedEvent Ev;
		Ev.Phase = ERTMatchPhase::Blast;
		Ev.Type = ERTResolvedEventType::Attack;
		Ev.ActionId = TEXT("Hero.Ivrin.PulseShot");
		Ev.BaseActionId = TEXT("Action.BasicAttack");
		Ev.Shape = Shape;
		Ev.HitGeometry.bResolved = true;
		Ev.HitGeometry.From = FRTCellId(0, 0);
		Ev.HitGeometry.Impact = FRTCellId(3, 0);
		Ev.HitGeometry.FromVerdict.AllowTeam(0);
		Ev.HitGeometry.FromVerdict.AllowTeam(1);
		Ev.HitGeometry.ImpactVerdict.AllowTeam(0);
		Ev.HitGeometry.ImpactVerdict.AllowTeam(1);
		return Ev;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackTracerStyleFollowsShapeTest,
	"RefactorTactics.Playback.TracerStyleFollowsShapeForBasicAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackTracerStyleFollowsShapeTest::RunTest(const FString&)
{
	TestTrue(TEXT("Single -> proiettile"),
		URTPlaybackLibrary::TracerStyleFor(MakeTracerAttackEvent(ERTAbilityShape::Single), 0) == ERTTracerStyle::Projectile);
	TestTrue(TEXT("Line -> getto"),
		URTPlaybackLibrary::TracerStyleFor(MakeTracerAttackEvent(ERTAbilityShape::Line), 0) == ERTTracerStyle::Jet);
	TestTrue(TEXT("Area -> niente"),
		URTPlaybackLibrary::TracerStyleFor(MakeTracerAttackEvent(ERTAbilityShape::Area), 0) == ERTTracerStyle::None);
	TestTrue(TEXT("Cone -> niente"),
		URTPlaybackLibrary::TracerStyleFor(MakeTracerAttackEvent(ERTAbilityShape::Cone), 0) == ERTTracerStyle::None);

	FRTResolvedEvent Abilita = MakeTracerAttackEvent(ERTAbilityShape::Single);
	Abilita.ActionId = TEXT("Hero.Ivrin.PassingBlade");
	Abilita.BaseActionId = NAME_None;
	TestFalse(TEXT("un'azione che non e' un attacco base non e' idonea"), URTPlaybackLibrary::IsTracerEligible(Abilita));

	FRTResolvedEvent Generica = MakeTracerAttackEvent(ERTAbilityShape::Single);
	Generica.ActionId = TEXT("Action.BasicAttack");
	Generica.BaseActionId = NAME_None;
	TestTrue(TEXT("l'attacco base GENERICO e' idoneo dal suo ActionId"), URTPlaybackLibrary::IsTracerEligible(Generica));

	// ⛔ **`bResolved` e non le celle**: `FRTCellId()` e' `(0,0,0)`, una cella VALIDA.
	FRTResolvedEvent Irrisolto = MakeTracerAttackEvent(ERTAbilityShape::Single);
	Irrisolto.HitGeometry.bResolved = false;
	TestFalse(TEXT("senza geometria risolta non c'e' tracer, anche con celle valide"),
		URTPlaybackLibrary::IsTracerEligible(Irrisolto));

	FRTResolvedEvent Movimento = MakeTracerAttackEvent(ERTAbilityShape::Single);
	Movimento.Type = ERTResolvedEventType::Move;
	TestFalse(TEXT("solo un Attack ha un tracer"), URTPlaybackLibrary::IsTracerEligible(Movimento));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrivacyTracerHiddenWhenOriginUnknownTest,
	"RefactorTactics.Privacy.TracerHiddenWhenOriginUnknown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrivacyTracerHiddenWhenOriginUnknownTest::RunTest(const FString&)
{
	// La squadra 1 e' stata colpita da un attaccante che NON vedeva: il verdetto dell'origine la esclude.
	FRTResolvedEvent Ev = MakeTracerAttackEvent(ERTAbilityShape::Single);
	Ev.HitGeometry.FromVerdict = FRTKnowledgeVerdict::NoOne();
	Ev.HitGeometry.FromVerdict.AllowTeam(0);

	TestTrue(TEXT("chi spara vede il proprio tracer"),
		URTPlaybackLibrary::TracerStyleFor(Ev, 0) == ERTTracerStyle::Projectile);
	// 🔴 **La mutazione dichiarata**: ignorare `FromVerdict` fa disegnare qui un proiettile, cioe' rivela la
	// cella di chi spara a chi non la conosceva — la riga che deve cadere.
	TestTrue(TEXT("chi non vedeva l'attaccante NON vede il tracer"),
		URTPlaybackLibrary::TracerStyleFor(Ev, 1) == ERTTracerStyle::None);

	FRTResolvedEvent Cieco = MakeTracerAttackEvent(ERTAbilityShape::Single);
	Cieco.HitGeometry.ImpactVerdict = FRTKnowledgeVerdict::NoOne();
	Cieco.HitGeometry.ImpactVerdict.AllowTeam(1);
	TestTrue(TEXT("chi non conosceva la cella d'impatto non vede il tracer"),
		URTPlaybackLibrary::TracerStyleFor(Cieco, 0) == ERTTracerStyle::None);
	TestTrue(TEXT("un osservatore fuori intervallo non legge"),
		URTPlaybackLibrary::TracerStyleFor(MakeTracerAttackEvent(ERTAbilityShape::Single), -1) == ERTTracerStyle::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrivacyTracerRhythmIsTheSameForEveryViewerTest,
	"RefactorTactics.Privacy.TracerRhythmIsTheSameForEveryViewer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrivacyTracerRhythmIsTheSameForEveryViewerTest::RunTest(const FString&)
{
	// 🔑 Il RITMO dipende dall'idoneita' (spec §2.1, condizioni 1-3), il DISEGNO dalla conoscenza (condizione 4).
	// Due squadre con verdetti opposti vedono l'arrivo nello stesso istante.
	FRTResolvedEvent Ev = MakeTracerAttackEvent(ERTAbilityShape::Single);
	Ev.HitGeometry.FromVerdict = FRTKnowledgeVerdict::NoOne();
	Ev.HitGeometry.FromVerdict.AllowTeam(0);

	TestTrue(TEXT("premessa: le due squadre hanno disegni diversi"),
		URTPlaybackLibrary::TracerStyleFor(Ev, 0) != URTPlaybackLibrary::TracerStyleFor(Ev, 1));
	const float Volo = URTPlaybackLibrary::TracerFlightFor(URTPlaybackLibrary::IsTracerEligible(Ev), 0.25f, 0.5f);
	TestEqual(TEXT("e lo stesso volo: l'idoneita' non legge chi guarda"), Volo, 0.25f, RTTol);
	return true;
}
```

- [ ] **Step 2: Run the build to verify the tests fail**

Expected: FAIL di compilazione — `HitGeometry`, `IsTracerEligible`, `TracerStyleFor` non esistono.

- [ ] **Step 3: Write the data type**

In `RTResolvedEvent.h`, prima di `USTRUCT(BlueprintType) struct FRTResolvedEvent` (`:182`):

```cpp
/**
 * La geometria di un colpo per il tracer del playback (`#2454`, spec `2026-10-07-tracer-attacco-base` §3).
 * ⚠️ **Solo `Attack`, solo playback**: vive in `ResolvedTimeline`, fuori da `StateHash`, TurnLog e replay.
 */
USTRUCT(BlueprintType)
struct FRTHitGeometry
{
	GENERATED_BODY()

	/** ⛔ **L'unico indicatore di presenza**: `FRTCellId()` e' `(0,0,0)`, una cella VALIDA, quindi un estremo
	 *  mancante non si riconosce dalle celle. Falso = il produttore non ha risolto l'origine o la vittima. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	bool bResolved = false;

	/** L'origine dichiarata del colpo, da `ResolveImpactOrigin` ([D-302] punto 3). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	FRTCellId From;

	/** La cella della vittima nell'istante del colpo, prima di ogni spostamento forzato. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	FRTCellId Impact;

	/** Chi conosceva l'ATTACCANTE in `From` quando il colpo e' partito ([D-223], fatto puntuale). */
	UPROPERTY()
	FRTKnowledgeVerdict FromVerdict;

	/** Chi conosceva la VITTIMA in `Impact` quando il colpo e' arrivato ([D-223], fatto puntuale). */
	UPROPERTY()
	FRTKnowledgeVerdict ImpactVerdict;
};
```

La dichiarazione di `FRTResolvedEvent` diventa:

```cpp
USTRUCT(BlueprintType, meta = (RTServerOnly))
struct FRTResolvedEvent
```

con, sopra, una riga di commento:

```cpp
// ⛔ **`RTServerOnly`** (spec §0.3, P5): con la geometria e i verdetti di ogni squadra l'evento porta
// l'informazione COMPLETA. Un client ricevera' una proiezione, mai questo tipo:
// `Privacy.ServerOnlyTypesAreNotReplicated` lo misura.
```

L'intestazione `// --- Solo per `AttackFootprint` ([D-301]). Vuoti/di default per ogni altro `Type`. ---` (`:334`)
diventa:

```cpp
	// --- `AttackFootprint` ([D-301]); `Shape` vale anche per `Attack` (`#2454`, la forma dell'INTENTO).
	//     Vuoti/di default per ogni altro `Type`. ---
```

In coda alla struct, prima di `FRTResolvedEvent() = default;`:

```cpp
	/** Solo `Attack` (`#2454`): da dove e verso dove il colpo e' andato, e chi lo sapeva. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	FRTHitGeometry HitGeometry;
```

- [ ] **Step 4: Write eligibility and style**

`RTPlaybackLibrary.h` (aggiungi `#include "Turn/RTResolvedEvent.h"` se manca):

```cpp
	/**
	 * Le condizioni 1-3 della spec §2.1: un `Attack` di un attacco base (`ActionId` OPPURE `BaseActionId` ==
	 * `Action.BasicAttack`), di forma `Single` o `Line`, con geometria risolta. Decide il RITMO: non legge chi guarda.
	 * ⚠️ Idoneita' PROVVISORIA e dichiarata: la sostituisce la tabella `ActionId -> profilo` del sotto-progetto 4.
	 */
	static bool IsTracerEligible(const FRTResolvedEvent& Ev);

	/**
	 * Lo stile del tracer per chi guarda: `None` se non idoneo, o se la squadra non conosceva l'attaccante in
	 * `From` OPPURE la vittima in `Impact` (spec §0.3, P1). Decide il DISEGNO, mai il ritmo.
	 */
	static ERTTracerStyle TracerStyleFor(const FRTResolvedEvent& Ev, int32 ViewerTeamId);
```

`RTPlaybackLibrary.cpp`:

```cpp
bool URTPlaybackLibrary::IsTracerEligible(const FRTResolvedEvent& Ev)
{
	static const FName BasicAttack(TEXT("Action.BasicAttack"));
	return Ev.Type == ERTResolvedEventType::Attack
		&& (Ev.ActionId == BasicAttack || Ev.BaseActionId == BasicAttack)
		&& (Ev.Shape == ERTAbilityShape::Single || Ev.Shape == ERTAbilityShape::Line)
		&& Ev.HitGeometry.bResolved;
}

ERTTracerStyle URTPlaybackLibrary::TracerStyleFor(const FRTResolvedEvent& Ev, int32 ViewerTeamId)
{
	if (!IsTracerEligible(Ev)
		|| !Ev.HitGeometry.FromVerdict.AllowsTeam(ViewerTeamId)
		|| !Ev.HitGeometry.ImpactVerdict.AllowsTeam(ViewerTeamId))
	{
		return ERTTracerStyle::None;
	}
	return Ev.Shape == ERTAbilityShape::Line ? ERTTracerStyle::Jet : ERTTracerStyle::Projectile;
}
```

- [ ] **Step 5: Build and run the tests**

Run: `python tools/suite/esegui.py RefactorTactics.Playback+RefactorTactics.Privacy+RefactorTactics.Presentation`
Expected: PASS, compreso `Privacy.ServerOnlyTypesAreNotReplicated` (oggi nessuna proprietà `Replicated` contiene
l'evento). ⛔ Se quel test diventa rosso, **fermati**: è un'esposizione di rete reale, e va riportata, non aggirata.

- [ ] **Step 6: Mutation check**

In `TracerStyleFor` togli la riga `|| !Ev.HitGeometry.FromVerdict.AllowsTeam(ViewerTeamId)`. Expected: FAIL su
`Privacy.TracerHiddenWhenOriginUnknown`. Ripristina: PASS.

- [ ] **Step 7: Commit**

```bash
git add Source/RefactorTactics/Turn/RTResolvedEvent.h Source/RefactorTactics/Turn/RTPlaybackLibrary.h Source/RefactorTactics/Turn/RTPlaybackLibrary.cpp Source/RefactorTactics/Tests/RTPlaybackLibraryTests.cpp
git commit -m "feat(2454): la geometria del colpo, l'idoneita' e lo stile del tracer — FRTResolvedEvent server-only"
```

---

### Task 4: Il produttore riempie la geometria del colpo

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.cpp` (emissione `Attack`, `:6254-6276`)
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.h` (accanto agli altri `…ForTest`, `:515-660`)
- Create: `Source/RefactorTactics/Tests/RTAttackTracerTests.cpp`

**Interfaces:**
- Consumes: `FRTHitGeometry`, `HitGeometry`, `URTPlaybackLibrary::TracerStyleFor` (Task 3).
- Produces: ogni `Attack` in `ResolvedTimeline` porta `Shape` dell'intento e `HitGeometry`; `bool ARTTurnManager::bSkipHitGeometryForTest` (pubblico, default `false`).

- [ ] **Step 1: Write the failing tests**

`Source/RefactorTactics/Tests/RTAttackTracerTests.cpp`:

```cpp
// La geometria del colpo per il tracer del playback (`#2454`, spec `2026-10-07-tracer-attacco-base` §3).
//
// 🔑 Qui si misura il PRODUTTORE: che ogni `Attack` porti l'origine di `ResolveImpactOrigin`, la cella della
// propria vittima, la forma dell'intento e i due verdetti dei fatti puntuali. Il disegno e' di altri test.
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Kismet/GameplayStatics.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Nomi distinti per file: in unity build i test condividono la translation unit.

	URTHexMapAsset* SpawnTracerMap(UWorld* World, int32 Radius = 8)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return M;
	}

	ARTUnit* SpawnTracerUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Copertura sul bordo di una cella (la stessa forma di `RTHexCoverTests.cpp`). */
	void SetTracerCoverEdge(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge, ERTHexCoverType Type)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Covers.Add(FRTHexCover(Edge, Type, FRTHexCover::DefaultIntegrity(Type)));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	TArray<FRTResolvedEvent> TracerAttacks(const ARTTurnManager* TM)
	{
		TArray<FRTResolvedEvent> Out;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::Attack) { Out.Add(Ev); }
		}
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatAttackCarriesHitGeometryTest,
	"RefactorTactics.Combat.AttackCarriesHitGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatAttackCarriesHitGeometryTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	// Un getto di Muiren che attraversa DUE vittime in linea: un intento, due `Attack`.
	SpawnTracerMap(World);
	ARTUnit* Muiren = SpawnTracerUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(0, 2));
	ARTUnit* Vicina = SpawnTracerUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(1, 2));
	ARTUnit* Lontana = SpawnTracerUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Muiren || !Vicina || !Lontana) { AddError(TEXT("allestimento fallito")); return false; }

	Muiren->PlannedAbilityIndex = 0; // Hero.Muiren.PressureJet, Line
	Muiren->PlannedAttackTarget = Lontana;
	Muiren->PlannedCell = Muiren->Cell;

	// La cella di ciascuna vittima PRIMA del turno: il getto le spinge, e l'impatto e' prima della spinta.
	TMap<int32, FRTCellId> CellaIniziale;
	CellaIniziale.Add(Vicina->StableUnitId, Vicina->Cell);
	CellaIniziale.Add(Lontana->StableUnitId, Lontana->Cell);

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	const TArray<FRTResolvedEvent> Colpi = TracerAttacks(TM);
	if (!TestEqual(TEXT("premessa: il getto ha colpito le due vittime"), Colpi.Num(), 2)) { return false; }

	for (const FRTResolvedEvent& Ev : Colpi)
	{
		const FRTCellId* Attesa = CellaIniziale.Find(Ev.TargetStableUnitId);
		TestTrue(TEXT("la geometria e' risolta"), Ev.HitGeometry.bResolved);
		TestTrue(TEXT("la forma e' quella dell'INTENTO"), Ev.Shape == ERTAbilityShape::Line);
		TestTrue(TEXT("l'origine e' la cella di Muiren"), Ev.HitGeometry.From == FRTCellId(0, 2));
		TestTrue(TEXT("l'impatto e' la cella della PROPRIA vittima, prima della spinta"),
			Attesa != nullptr && Ev.HitGeometry.Impact == *Attesa);
		TestTrue(TEXT("chi spara conosce la propria origine"), Ev.HitGeometry.FromVerdict.AllowsTeam(0));
		TestTrue(TEXT("chi e' colpito conosce il proprio impatto"), Ev.HitGeometry.ImpactVerdict.AllowsTeam(1));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatCoveredHitCarriesHitGeometryTest,
	"RefactorTactics.Combat.CoveredHitCarriesHitGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatCoveredHitCarriesHitGeometryTest::RunTest(const FString&)
{
	// Copertura BASSA: il colpo arriva ridotto, e il tracer arriva sul bersaglio. Copertura ALTA: niente `Attack`,
	// quindi niente tracer, e il ritmo del Blast non ha un colpo da scandire.
	for (const ERTHexCoverType Tipo : { ERTHexCoverType::Low, ERTHexCoverType::High })
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		URTHexMapAsset* Map = SpawnTracerMap(World);
		ARTUnit* Ivrin = SpawnTracerUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(1, 2));
		ARTUnit* Branth = SpawnTracerUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(3, 2));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Ivrin || !Branth) { AddError(TEXT("allestimento fallito")); return false; }
		SetTracerCoverEdge(Map, FRTCellId(3, 2), ERTHexDirection::W, Tipo);

		Ivrin->PlannedAbilityIndex = 0; // Hero.Ivrin.PulseShot, Single
		Ivrin->PlannedAttackTarget = Branth;
		Ivrin->PlannedCell = Ivrin->Cell;

		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

		const TArray<FRTResolvedEvent> Colpi = TracerAttacks(TM);
		if (Tipo == ERTHexCoverType::Low)
		{
			if (!TestEqual(TEXT("bassa: premessa, il colpo e' arrivato"), Colpi.Num(), 1)) { return false; }
			TestTrue(TEXT("bassa: geometria risolta"), Colpi[0].HitGeometry.bResolved);
			TestTrue(TEXT("bassa: l'impatto e' la cella di Branth"), Colpi[0].HitGeometry.Impact == FRTCellId(3, 2));
			TestTrue(TEXT("bassa: proiettile per chi spara"),
				URTPlaybackLibrary::TracerStyleFor(Colpi[0], 0) == ERTTracerStyle::Projectile);
		}
		else
		{
			TestEqual(TEXT("alta: nessun Attack, quindi nessun tracer"), Colpi.Num(), 0);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrivacyUnseenAttackerTest,
	"RefactorTactics.Privacy.UnseenAttackerIsOutOfTheOriginVerdict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrivacyUnseenAttackerTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	// Branth spara a Ivrin DA DIETRO, a 3 celle: oltre la consapevolezza ravvicinata (2, `RTPerceptionLibrary.h`)
	// e fuori dall'arco frontale di Ivrin, che guarda a Est come Branth.
	SpawnTracerMap(World);
	ARTUnit* Branth = SpawnTracerUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
	ARTUnit* Ivrin = SpawnTracerUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(5, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Branth || !Ivrin) { AddError(TEXT("allestimento fallito")); return false; }
	Branth->Facing = ERTHexDirection::E;
	Ivrin->Facing = ERTHexDirection::E;

	Branth->PlannedAbilityIndex = 0; // Hero.Branth.ImpactShot, Single, portata 3
	Branth->PlannedAttackTarget = Ivrin;
	Branth->PlannedCell = Branth->Cell;

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	const TArray<FRTResolvedEvent> Colpi = TracerAttacks(TM);
	if (!TestEqual(TEXT("premessa: il colpo alle spalle e' arrivato"), Colpi.Num(), 1)) { return false; }
	const FRTHitGeometry& G = Colpi[0].HitGeometry;

	TestTrue(TEXT("la geometria e' risolta"), G.bResolved);
	TestTrue(TEXT("chi spara conosce la propria origine"), G.FromVerdict.AllowsTeam(0));
	// 🔴 Il punto del test: la squadra colpita non vedeva chi sparava, e il verdetto lo dice.
	TestFalse(TEXT("la squadra colpita NON conosceva l'attaccante"), G.FromVerdict.AllowsTeam(1));
	// D-380: chi colpisce conosce la vittima prima che il verdetto si congeli (`RevealHitTargetsToAttackers`).
	TestTrue(TEXT("chi spara conosce la cella d'impatto"), G.ImpactVerdict.AllowsTeam(0));

	TestTrue(TEXT("per chi spara: proiettile"), URTPlaybackLibrary::TracerStyleFor(Colpi[0], 0) == ERTTracerStyle::Projectile);
	TestTrue(TEXT("per chi e' colpito: nessun tracer"), URTPlaybackLibrary::TracerStyleFor(Colpi[0], 1) == ERTTracerStyle::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDeterminismHitGeometryOutOfHashesTest,
	"RefactorTactics.Determinism.HitGeometryStaysOutOfHashes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDeterminismHitGeometryOutOfHashesTest::RunTest(const FString&)
{
	// Lo stesso turno due volte: una con la geometria, una con il produttore che la lascia vuota. Se la geometria
	// entrasse in uno dei due hash, i numeri differirebbero.
	int64 StatoHash[2] = { 0, 0 };
	uint32 LogHash[2] = { 0, 0 };
	bool bRisolta[2] = { false, false };
	for (int32 Run = 0; Run < 2; ++Run)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		SpawnTracerMap(World);
		ARTUnit* Ivrin = SpawnTracerUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(1, 2));
		ARTUnit* Branth = SpawnTracerUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(3, 2));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Ivrin || !Branth) { AddError(TEXT("allestimento fallito")); return false; }
		TM->bSkipHitGeometryForTest = (Run == 1);

		Ivrin->PlannedAbilityIndex = 0;
		Ivrin->PlannedAttackTarget = Branth;
		Ivrin->PlannedCell = Ivrin->Cell;

		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

		StatoHash[Run] = TM->GetPendingFinalStateHash();
		LogHash[Run] = URTTurnLogLibrary::HashTurnLog(TM->GetTurnLog());
		const TArray<FRTResolvedEvent> Colpi = TracerAttacks(TM);
		bRisolta[Run] = Colpi.Num() == 1 && Colpi[0].HitGeometry.bResolved;
	}

	// Controllo positivo: il gancio ha davvero tolto la geometria, o il confronto non misurerebbe niente.
	TestTrue(TEXT("controllo: con la geometria, risolta"), bRisolta[0]);
	TestFalse(TEXT("controllo: col gancio, assente"), bRisolta[1]);
	TestNotEqual(TEXT("premessa: lo StateHash e' stato catturato"), StatoHash[0], (int64)0);
	TestEqual(TEXT("StateHash identico con e senza geometria"), StatoHash[0], StatoHash[1]);
	TestEqual(TEXT("HashTurnLog identico con e senza geometria"), LogHash[0], LogHash[1]);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 2: Run the build to verify the tests fail**

Expected: FAIL di compilazione — `bSkipHitGeometryForTest` non esiste.

- [ ] **Step 3: Add the test hook**

In `RTTurnManager.h`, accanto a `ResolvedTimelineForTest()` (`:634`):

```cpp
	/**
	 * Spegne il riempimento di `FRTResolvedEvent::HitGeometry` (`#2454`). Esiste per un test solo —
	 * `Determinism.HitGeometryStaysOutOfHashes` — che confronta gli hash dello stesso turno con e senza.
	 */
	bool bSkipHitGeometryForTest = false;
```

- [ ] **Step 4: Fill the geometry at the Attack emission**

In `RTTurnManager.cpp`, nel blocco *«Evento per il playback: colpo Attacker -> Victim»* (`:6254`), subito dopo
`Ev.Amount = Hit.Power;`:

```cpp
		// `#2454` (spec `2026-10-07-tracer-attacco-base` §3): la forma e la geometria del colpo, per il tracer.
		//
		// 🔑 **La forma dall'INTENTO, l'origine da `ResolveImpactOrigin`**: e' la stessa lettura che, qualche riga
		// sopra, racconta da che lato e' arrivato il colpo. Quella funzione esiste *«perche' i chiamanti sono DUE e
		// devono restare d'accordo»*: questo e' il terzo, e una terza copia della regola si separerebbe alla prima
		// modifica.
		// ⛔ **Il predicato dei FATTI PUNTUALI** ([D-223]): `FreezeVerdictFor` con la cella del fatto, lo stesso che
		// congela le righe di combattimento. Non `FreezeRouteCellVerdict`: quello esiste perche' una rotta non e'
		// un fatto puntuale. E `RevealHitTargetsToAttackers` ([D-380]) e' gia' passato: chi colpisce conosce la vittima.
		if (Intents.IsValidIndex(Hit.IntentIndex))
		{
			Ev.Shape = Intents[Hit.IntentIndex].Shape;
		}
		FRTCellId HitFrom;
		if (!bSkipHitGeometryForTest && Attacker && Victim && HexUnits.IsValidIndex(Hit.TargetId)
			&& ResolveImpactOrigin(Intents, Plan, HexUnits, Hit, HitFrom))
		{
			Ev.HitGeometry.bResolved = true;
			Ev.HitGeometry.From = HitFrom;
			Ev.HitGeometry.Impact = HexUnits[Hit.TargetId].Cell;
			Ev.HitGeometry.FromVerdict = FreezeVerdictFor(FRTLogSubject::UnitAt(Attacker, HitFrom));
			Ev.HitGeometry.ImpactVerdict = FreezeVerdictFor(FRTLogSubject::UnitAt(Victim, Ev.HitGeometry.Impact));
		}
```

⚠️ Verifica sul posto che `Intents`, `Plan`, `HexUnits`, `Hit`, `Attacker` e `Victim` siano i nomi in scope: sono
quelli che `ResolveImpactOrigin(Intents, Plan, HexUnits, Hit, SideOrigin)` usa a `:6085`, nello stesso ciclo. Se
`Victim`/`Attacker` sono `const ARTUnit*`, `FRTLogSubject::UnitAt` li accetta così (`Turn/RTCombatLog.h:41`).

- [ ] **Step 5: Build and run the tests**

Run: `python tools/suite/esegui.py RefactorTactics.Combat+RefactorTactics.Privacy+RefactorTactics.Determinism+RefactorTactics.Replay`
Expected: PASS. ⚠️ Se fallisce la **premessa** di un test (un `Attack` mancante), il difetto è nella fixture, non
nel produttore: leggi il motivo nel log (`BlockedIntents`, portata, LOS) e correggi la posizione, scrivendo il
perché in un commento. ⛔ Non indebolire l'asserzione.

⚠️ **Il tiro alla cieca non è testato qui, e non per dimenticanza**: su `main` l'attacco base richiede ancora la
linea di tiro, e il tiro verso un esagono non visibile arriva con la PR #3230 (D-415), aperta. Il caso — chi colpisce
alla cieca vede il proprio tracer perché `RevealHitTargetsToAttackers` precede il verdetto — resta della seduta
`PIE-V01-BLINDFIRE` e di un test da scrivere quando quella PR atterra.

- [ ] **Step 6: Mutation check**

Sostituisci `FreezeVerdictFor(FRTLogSubject::UnitAt(Attacker, HitFrom))` con `FRTKnowledgeVerdict::Everyone()`.
Expected: FAIL su `Privacy.UnseenAttackerIsOutOfTheOriginVerdict`. Ripristina: PASS.

- [ ] **Step 7: Commit**

```bash
git add Source/RefactorTactics/Turn/RTTurnManager.h Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Tests/RTAttackTracerTests.cpp
git commit -m "feat(2454): ogni Attack porta la geometria del colpo e chi la conosceva, col predicato dei fatti puntuali"
```

---

### Task 5: Il Blast in due battiti per colpo

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.h` (`AttacksShown`, `:3347`; `AttackShowSeconds`, `:1263-1265`; dichiarazioni private accanto a `RevealPlaybackFootprints`)
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.cpp` (`BeginPlayback` raccolta `:7741-7744` e `ViewerTeamId` `:7685`; `EnterPlaybackPhase` `:7979-7998`; ramo Blast `:8449-8505`; rete di finalizzazione `:8540-8561`; `FinishPlayback` `:8772-8790`)
- Create: `Source/RefactorTactics/Tests/RTAttackTracerPlaybackTests.cpp`
- Modify: `Source/RefactorTactics/Tests/RTDefensiveReactionTests.cpp` (dopo `Reactions.Brace.WindowSuspendsBlast`)

**Interfaces:**
- Consumes: `TracerFlightFor`, `AttackBeatsDue`, `IsTracerEligible` (Task 2-3).
- Produces: `float ARTTurnManager::TracerFlightSeconds` (`UPROPERTY`, default `0.25f`); stato privato `int32 AttackBeatsDone`, `TArray<float> PlaybackAttackFlights`, `int32 PlaybackViewerTeamId`; `void LaunchPlaybackAttack(int32 Index)`, `void ArrivePlaybackAttack(int32 Index, bool bWithLog)` (indice in `PlaybackAttacks`); test-only `bool bRecordAttackBeatsForTest`, `const TArray<FString>& AttackBeatTraceForTest() const`, `ERTMatchPhase CurrentPlaybackPhaseForTest() const`.

- [ ] **Step 1: Write the failing tests**

`Source/RefactorTactics/Tests/RTAttackTracerPlaybackTests.cpp`:

```cpp
// Il Blast in due battiti per colpo, e il tracer che lo racconta (`#2454`, spec §2.3).
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Kismet/GameplayStatics.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Nomi distinti per file: in unity build i test condividono la translation unit.

	ARTUnit* SpawnBattitoUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/**
	 * Il turno di prova: attaccanti della squadra 0 — quella di chi guarda in un mondo senza player controller
	 * (`ARTPlayerState::TeamIdOf(nullptr)` risponde 0) — contro Ivrin. Con `bDue`, Aevik spara anche lui.
	 */
	ARTTurnManager* SetUpBattitoTurn(UWorld* World, bool bDue)
	{
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 8);
		ARTUnit* Branth = SpawnBattitoUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 2));
		ARTUnit* Bersaglio = SpawnBattitoUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(3, 2));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Branth || !Bersaglio) { return nullptr; }
		Branth->PlannedAbilityIndex = 0;
		Branth->PlannedAttackTarget = Bersaglio;
		Branth->PlannedCell = Branth->Cell;
		if (bDue)
		{
			ARTUnit* Aevik = SpawnBattitoUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(3, 4));
			if (!Aevik) { return nullptr; }
			Aevik->PlannedAbilityIndex = 0;
			Aevik->PlannedAttackTarget = Bersaglio;
			Aevik->PlannedCell = Aevik->Cell;
		}
		TM->bRecordAttackBeatsForTest = true;
		return TM;
	}

	/** Tick piccoli finche' il playback e' nel Blast; falso se la risoluzione finisce prima. */
	bool TickUntilBlast(ARTTurnManager* TM)
	{
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			if (TM->CurrentPlaybackPhaseForTest() == ERTMatchPhase::Blast) { return true; }
			TM->Tick(0.02f);
		}
		return TM->IsResolving() && TM->CurrentPlaybackPhaseForTest() == ERTMatchPhase::Blast;
	}

	/** L'indice del tick in cui un battito compare per la prima volta nella traccia, o -1. */
	int32 TickOfBeat(const TArray<TPair<int32, FString>>& Visti, const FString& Battito)
	{
		for (const TPair<int32, FString>& V : Visti) { if (V.Value == Battito) { return V.Key; } }
		return -1;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackHitArrivesAfterLaunchTest,
	"RefactorTactics.Playback.HitArrivesAfterTheLaunch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackHitArrivesAfterLaunchTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ false);
	if (!TestNotNull(TEXT("turno di prova"), TM)) { return false; }

	TM->LockInAndResolve();
	TArray<TPair<int32, FString>> Visti;
	for (int32 I = 0; I < 600 && TM->IsResolving(); ++I)
	{
		const int32 Prima = TM->AttackBeatTraceForTest().Num();
		TM->Tick(0.02f);
		for (int32 K = Prima; K < TM->AttackBeatTraceForTest().Num(); ++K)
		{
			Visti.Add(TPair<int32, FString>(I, TM->AttackBeatTraceForTest()[K]));
		}
	}

	const int32 Lancio = TickOfBeat(Visti, TEXT("L0"));
	const int32 Arrivo = TickOfBeat(Visti, TEXT("A0"));
	if (!TestTrue(TEXT("premessa: il colpo e' partito ed e' arrivato"), Lancio >= 0 && Arrivo >= 0)) { return false; }
	// 🔴 **La mutazione dichiarata**: con `TracerFlightSeconds = 0` lancio e arrivo cadono nello stesso tick.
	TestTrue(FString::Printf(TEXT("l'arrivo (tick %d) e' DOPO il lancio (tick %d)"), Arrivo, Lancio), Arrivo > Lancio);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackAttackBeatsOrderedInOneTickTest,
	"RefactorTactics.Playback.AttackBeatsStayOrderedInOneTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackAttackBeatsOrderedInOneTickTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ true);
	if (!TestNotNull(TEXT("turno di prova"), TM)) { return false; }

	TM->LockInAndResolve();
	if (!TestTrue(TEXT("premessa: il playback arriva al Blast"), TickUntilBlast(TM))) { return false; }

	// Un tick LUNGO, che copre tutti i battiti del Blast: velocita' x4, o un fotogramma lento.
	TM->Tick(5.f);

	// 🔴 **La mutazione dichiarata**: due cicli separati — prima i lanci, poi gli arrivi — danno
	// `L0, L1, A0, A1`, e `Next Action` fermerebbe sull'arrivo di 0 con 1 gia' in volo.
	const TArray<FString> Attesa = { TEXT("L0"), TEXT("A0"), TEXT("L1"), TEXT("A1") };
	TestEqual(TEXT("i battiti escono L0, A0, L1, A1"),
		FString::Join(TM->AttackBeatTraceForTest(), TEXT(",")), FString::Join(Attesa, TEXT(",")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

In `RTDefensiveReactionTests.cpp`, dopo `Reactions.Brace.WindowSuspendsBlast` e con i suoi helper
(`MakeDefWorld`, `SpawnDefMap`, `SpawnDefUnit`, `RTAbilityFixtures::AddCoreAbilityInSlot`, `DestroyDefWorld`):

```cpp
/**
 * **Il Blast esteso dopo una finestra non rigioca i colpi** (`#2454`, spec §5).
 *
 * 🔑 Il cursore dei battiti si azzera in `EnterPlaybackPhase` e in `FinishPlayback`, MAI in `BeginPlayback`:
 * l'estensione con `bPreserveClock` salta `EnterPlaybackPhase`, e un azzeramento li' farebbe arrivare due volte
 * i colpi gia' mostrati.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBraceExtendedBlastDoesNotReplayHitsTest,
	"RefactorTactics.Reactions.Brace.ExtendedBlastDoesNotReplayHits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBraceExtendedBlastDoesNotReplayHitsTest::RunTest(const FString&)
{
	UWorld* World = MakeDefWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnDefMap(World, /*Radius*/ 8);

	ARTUnit* Bracer = SpawnDefUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Pusher = SpawnDefUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Bracer"), Bracer) || !TestNotNull(TEXT("Pusher"), Pusher) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyDefWorld(World);
		return false;
	}
	Bracer->bIsBotControlled = false;
	Bracer->ReactionProfileId = TEXT("Profile.Sidestep");
	Bracer->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Bracer, TEXT("Action.Brace"), 3);
	Bracer->PlannedCell = Bracer->Cell;
	Pusher->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Pusher, TEXT("Action.Push"), 3);
	Pusher->PlannedAttackTarget = Bracer;
	Pusher->PlannedCell = Pusher->Cell;

	TM->bEnablePlayback = true;
	TM->bRecordAttackBeatsForTest = true;
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("premessa: la finestra ha sospeso il Blast"), TM->IsResolutionSuspended()))
	{
		DestroyDefWorld(World);
		return false;
	}
	for (int32 I = 0; I < 20; ++I) { TM->Tick(0.05f); } // il playback avanza sulla timeline parziale
	for (int32 Scadenze = 0; TM->IsResolutionSuspended() && Scadenze < 8; ++Scadenze) { TM->ExpireReactionWindow(); }
	for (int32 I = 0; I < 600 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	// Ogni battito al piu' una volta: nessun lancio e nessun arrivo ripetuto dall'estensione.
	TSet<FString> Unici;
	for (const FString& B : TM->AttackBeatTraceForTest()) { Unici.Add(B); }
	TestEqual(TEXT("nessun battito ripetuto"), Unici.Num(), TM->AttackBeatTraceForTest().Num());
	TestEqual(TEXT("e ogni colpo e' arrivato"),
		TM->AttackBeatTraceForTest().FilterByPredicate([](const FString& B) { return B.StartsWith(TEXT("A")); }).Num(),
		TM->ResolvedEventCountOfTypeForTest(ERTResolvedEventType::Attack));

	DestroyDefWorld(World);
	return true;
}
```

- [ ] **Step 2: Run the build to verify the tests fail**

Expected: FAIL di compilazione — `bRecordAttackBeatsForTest`, `AttackBeatTraceForTest`, `CurrentPlaybackPhaseForTest`
non esistono.

- [ ] **Step 3: Declare state, property and helpers**

In `RTTurnManager.h`, accanto ad `AttackShowSeconds` (`:1263-1265`):

```cpp
	/**
	 * Tempo di volo del tracer di un attacco base (`#2454`): il colpo parte col lancio e il numero compare
	 * all'arrivo. ⚠️ Tagliato a `AttackShowSeconds / 2` da `URTPlaybackLibrary::TracerFlightFor`, cosi' il Blast
	 * non si allunga; con `AttackShowSeconds <= 0` non c'e' volo.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	float TracerFlightSeconds = 0.25f;
```

Accanto agli altri `…ForTest` pubblici (`:634`):

```cpp
	/** Registra i battiti del Blast in `AttackBeatTraceForTest` (`L<i>` lancio, `A<i>` arrivo). Solo test. */
	bool bRecordAttackBeatsForTest = false;
	const TArray<FString>& AttackBeatTraceForTest() const { return AttackBeatTrace; }

	/** La fase in riproduzione, o `Planning` se non si sta riproducendo. Solo test. */
	ERTMatchPhase CurrentPlaybackPhaseForTest() const
	{
		return (bIsResolving && PlaybackPhases.IsValidIndex(PlaybackPhaseIdx)) ? PlaybackPhases[PlaybackPhaseIdx]
			: ERTMatchPhase::Planning;
	}
```

Sostituisci `int32 AttacksShown = 0;                 // colpi gia' rivelati nel Blast corrente` (`:3347`) con:

```cpp
	/**
	 * Battiti gia' eseguiti nel Blast corrente (`#2454`): il `2i` e' il LANCIO del colpo `i`, il `2i+1` il suo
	 * ARRIVO. ⛔ Un cursore solo: due contatori separati, in un tick lungo, lancerebbero `i+1` prima dell'arrivo di
	 * `i`. Si azzera in `EnterPlaybackPhase` e in `FinishPlayback`, MAI in `BeginPlayback` (l'estensione con
	 * `bPreserveClock` salta `EnterPlaybackPhase`).
	 */
	int32 AttackBeatsDone = 0;
	/** Volo effettivo di ogni colpo, parallelo a `PlaybackAttacks` (`URTPlaybackLibrary::TracerFlightFor`). */
	TArray<float> PlaybackAttackFlights;
	/** La squadra di chi guarda, fissata in `BeginPlayback`: decide il DISEGNO del tracer, mai il ritmo. */
	int32 PlaybackViewerTeamId = 0;
	/** Traccia dei battiti per i test (`bRecordAttackBeatsForTest`). */
	TArray<FString> AttackBeatTrace;
```

Fra le dichiarazioni private, accanto a `RevealPlaybackFootprints`:

```cpp
	/** Il LANCIO di un colpo (`#2454`): il ruolo `Attack` sull'attaccante. */
	void LaunchPlaybackAttack(int32 Index);
	/**
	 * L'ARRIVO di un colpo: `Hit`, numero, `OnAttackResolved` e, con `bWithLog`, la riga `Colpo:` del log.
	 * ⚠️ La rete di finalizzazione passa `false`: oggi non scrive il log, e non comincia a farlo qui.
	 */
	void ArrivePlaybackAttack(int32 Index, bool bWithLog);
```

- [ ] **Step 4: Implement launch, arrival, cursor and flights**

In `RTTurnManager.cpp`, accanto a `RevealPlaybackStructureHits`:

```cpp
void ARTTurnManager::LaunchPlaybackAttack(int32 Index)
{
	const FRTResolvedEvent& Atk = PlaybackAttacks[Index];
	if (ARTUnit* const AtkSrc = UnitByStableId(Atk.SourceStableUnitId))
	{
		AtkSrc->PlayPresentationRole(ERTPresentationRole::Attack);
	}
	if (bRecordAttackBeatsForTest) { AttackBeatTrace.Add(FString::Printf(TEXT("L%d"), Index)); }
}

void ARTTurnManager::ArrivePlaybackAttack(int32 Index, bool bWithLog)
{
	const FRTResolvedEvent& Atk = PlaybackAttacks[Index];
	ARTUnit* const AtkSrc = UnitByStableId(Atk.SourceStableUnitId);
	ARTUnit* const AtkTgt = UnitByStableId(Atk.TargetStableUnitId);
	if (bWithLog)
	{
		AddLogEvent(FString::Printf(TEXT("Colpo: %s -> %s (%d)"),
			AtkSrc ? *AtkSrc->GetName() : TEXT("?"),
			AtkTgt ? *AtkTgt->GetName() : TEXT("(eliminato)"),
			Atk.Amount), FRTLogSubject::Unit(AtkSrc));
	}
	if (AtkTgt)
	{
		AtkTgt->PlayPresentationRole(ERTPresentationRole::Hit);
		AtkTgt->ShowDamageToken(Atk.Amount);
	}
	OnAttackResolved.Broadcast(AtkSrc, AtkTgt, Atk.Amount);
	if (bRecordAttackBeatsForTest) { AttackBeatTrace.Add(FString::Printf(TEXT("A%d"), Index)); }
}
```

⚠️ Porta in `ArrivePlaybackAttack` i commenti che oggi stanno sul corpo del ciclo (`#2455`, `FRTLogSubject::Unit`,
*«Sul BERSAGLIO e mai sull'attaccante»*): spostano di sede, non si perdono.

In `BeginPlayback`, dove si calcola `ViewerTeamId` (`:7685`), subito dopo:

```cpp
	PlaybackViewerTeamId = ViewerTeamId;
```

e, dopo il ciclo che riempie `PlaybackAttacks`, prima che il ciclo di `BeginPlayback` passi alle fasi:

```cpp
	// `#2454`: il volo di ogni colpo, deciso dall'idoneita' e MAI da chi guarda — cosi' il ritmo e' lo stesso
	// per entrambe le squadre (spec §2.1). Si ricalcola anche estendendo: e' funzione pura degli eventi.
	PlaybackAttackFlights.Reset(PlaybackAttacks.Num());
	for (const FRTResolvedEvent& Atk : PlaybackAttacks)
	{
		PlaybackAttackFlights.Add(URTPlaybackLibrary::TracerFlightFor(
			URTPlaybackLibrary::IsTracerEligible(Atk), TracerFlightSeconds, AttackShowSeconds));
	}
```

⚠️ Trova il punto esatto leggendo il ciclo `for (const FRTResolvedEvent& Ev : ResolvedTimeline)` che contiene
`PlaybackAttacks.Add(Ev)` (`:7741-7744`): il blocco va **dopo** la chiusura di quel ciclo.

In `EnterPlaybackPhase` (`:7982`), `AttacksShown = 0;` diventa:

```cpp
	AttackBeatsDone = 0;
	AttackBeatTrace.Reset();
```

Nel ramo Blast, il blocco da `const int32 ShouldShow = URTPlaybackLibrary::AttacksToShow(` fino alla chiusura del suo
`while` (`:8449-8504`) diventa:

```cpp
		// 🔑 **Un cursore solo, sui battiti** (`#2454`, spec §2.3): lancio e arrivo di ogni colpo formano la
		// sequenza `L0, A0, L1, A1, …`, monotona perche' `TracerFlightFor` taglia il volo a `A/2`.
		// ⛔ Due cicli separati — prima i lanci, poi gli arrivi — in un tick lungo lancerebbero `i+1` prima che
		// l'arrivo di `i` fermi `Next Action`: `Playback.AttackBeatsStayOrderedInOneTick` cade.
		const int32 BeatsDue = URTPlaybackLibrary::AttackBeatsDue(
			PlaybackPhaseElapsed, AttackShowSeconds, PlaybackAttackFlights);
		while (AttackBeatsDone < BeatsDue)
		{
			const int32 Index = AttackBeatsDone / 2;
			const bool bArrivo = (AttackBeatsDone % 2) == 1;
			++AttackBeatsDone;
			if (!bArrivo)
			{
				LaunchPlaybackAttack(Index);
				continue;
			}
			ArrivePlaybackAttack(Index, /*bWithLog=*/ true);

			// `#2855`: il confine di AZIONE dentro il `Blast` — e con `#2454` cade all'ARRIVO, perche' il colpo
			// e' mostrato quando arriva, non quando parte.
			// [qui il blocco di commento esistente da «🔑 **Si ferma DOPO aver mostrato il colpo**» a
			//  «⚠️ L'atto in corso segue la riproduzione da TUTTI i canali (`#3292`)», invariato]
			const FRTResolvedEvent& Atk = PlaybackAttacks[Index];
			if (!Atk.ActionId.IsNone()) { PlaybackLastShownAction = Atk.ActionId; }
			if (PlaybackStopAt == ERTPlaybackStopAt::NextAction
				&& URTPlaybackLibrary::IsActBoundary(Atk, PlaybackStopFromAction))
			{
				PausePlaybackAtActBoundary();
				return; // i battiti che questo tick avrebbe ancora eseguito restano per la ripresa
			}
		}
```

La rete di finalizzazione (`:8540-8561`) diventa:

```cpp
			// `#2454`: la rete passa per BATTITI. Un colpo mai lanciato riceve lancio e arrivo; uno lanciato e
			// non arrivato riceve SOLO l'arrivo — ⛔ mai un secondo ruolo `Attack` sull'attaccante.
			while (AttackBeatsDone < 2 * PlaybackAttacks.Num())
			{
				const int32 Index = AttackBeatsDone / 2;
				const bool bArrivo = (AttackBeatsDone % 2) == 1;
				++AttackBeatsDone;
				if (bArrivo) { ArrivePlaybackAttack(Index, /*bWithLog=*/ false); }
				else { LaunchPlaybackAttack(Index); }
			}
```

In `FinishPlayback`, accanto a `FootprintsShown = 0;` (`:8773`):

```cpp
	AttackBeatsDone = 0;
	PlaybackAttackFlights.Reset();
```

Infine cerca residui: `git grep -n "AttacksShown" -- Source/RefactorTactics` deve rispondere solo righe di
**commento** storiche (`:8143`, `RTTurnManager.h:3378`), che restano come prosa datata.

- [ ] **Step 5: Build and run the tests**

Run: `python tools/suite/esegui.py RefactorTactics.Playback+RefactorTactics.Reactions+RefactorTactics.Match.Autobattle`
Expected: PASS, compresi `Match.Autobattle.AttackShowSecondsStagesTheBlast`,
`Match.Autobattle.EveryRevealedAttackLeavesItsDamageTokenOnTheTarget` e tutto `RTPlaybackStopPredicateTests`.

- [ ] **Step 6: Mutation checks**

1. In `TracerFlightFor` restituisci sempre `0.f` → FAIL su `Playback.HitArrivesAfterTheLaunch`. Ripristina.
2. Sostituisci il `while` del ramo Blast con due cicli — prima tutti i lanci dovuti, poi tutti gli arrivi dovuti →
   FAIL su `Playback.AttackBeatsStayOrderedInOneTick`. Ripristina.
3. Aggiungi `AttackBeatsDone = 0;` all'inizio di `BeginPlayback` → FAIL su
   `Reactions.Brace.ExtendedBlastDoesNotReplayHits`. Ripristina.

Ogni ripristino: ricompila e rilancia, PASS. ⚠️ Se il punto 3 resta verde, il test non attraversa l'estensione:
verifica che il playback avanzi **durante** la sospensione (i 20 tick prima di `ExpireReactionWindow`) e correggi
la fixture prima di procedere.

- [ ] **Step 7: Commit**

```bash
git add Source/RefactorTactics/Turn/RTTurnManager.h Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Tests/RTAttackTracerPlaybackTests.cpp Source/RefactorTactics/Tests/RTDefensiveReactionTests.cpp
git commit -m "feat(2454): il Blast in due battiti per colpo — lancio col Cast, Hit e numero all'arrivo, un cursore solo"
```

---

### Task 6: Il tracer consegnato alla mappa

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.h`, `Source/RefactorTactics/Turn/RTTurnManager.cpp` (ingresso del ramo Blast `:8423`; finalizzazione Blast; `FinishPlayback` `:8778-8786`)
- Modify: `Source/RefactorTactics/Turn/RTPresentationBinding.cpp` (voce `Attack`, `:35-37`)
- Modify: `Source/RefactorTactics/Tests/RTAttackTracerPlaybackTests.cpp`, `Source/RefactorTactics/Tests/RTPlaybackStopPredicateTests.cpp` (`:252-276`)

**Interfaces:**
- Consumes: `SetPlaybackTracers`/`ClearPlaybackTracers`/`NumPlaybackTracers` (Task 1); `TracerStyleFor`, `TracerAlpha` (Task 2-3); `AttackBeatsDone`, `PlaybackAttackFlights`, `PlaybackViewerTeamId` (Task 5).
- Produces: `void ARTTurnManager::PushPlaybackTracers()` (privata).

- [ ] **Step 1: Write the failing tests**

In `RTAttackTracerPlaybackTests.cpp`, prima di `#endif`:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackTracerInFlightTest,
	"RefactorTactics.Playback.TracerIsInFlightBetweenLaunchAndArrival",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackTracerInFlightTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ false);
	ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(World);
	if (!TestTrue(TEXT("turno e mappa di prova"), TM != nullptr && Mappa != nullptr)) { return false; }

	TM->LockInAndResolve();
	bool bVistoInVolo = false;
	bool bInVoloDopoArrivo = false;
	for (int32 I = 0; I < 600 && TM->IsResolving(); ++I)
	{
		TM->Tick(0.02f);
		const TArray<FString>& Traccia = TM->AttackBeatTraceForTest();
		const bool bLanciato = Traccia.Contains(TEXT("L0"));
		const bool bArrivato = Traccia.Contains(TEXT("A0"));
		if (bLanciato && !bArrivato && Mappa->NumPlaybackTracers() == 1)
		{
			bVistoInVolo = true;
			const FRTPlaybackTracer& T = Mappa->GetPlaybackTracers()[0];
			TestTrue(TEXT("ImpactShot e' un proiettile"), T.Style == ERTTracerStyle::Projectile);
			TestTrue(TEXT("parte dalla cella di Branth"), T.From == FRTCellId(1, 2));
			TestTrue(TEXT("e va su quella di Ivrin"), T.To == FRTCellId(3, 2));
		}
		bInVoloDopoArrivo |= (bArrivato && Mappa->NumPlaybackTracers() > 0);
	}
	// 🔴 **La mutazione dichiarata**: senza la consegna alla mappa nessun tracer e' mai in volo.
	TestTrue(TEXT("fra lancio e arrivo il tracer e' in volo"), bVistoInVolo);
	TestFalse(TEXT("dopo l'arrivo nessun tracer resta"), bInVoloDopoArrivo);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackTracerChannelClearsTest,
	"RefactorTactics.Playback.TracerChannelClearsAtBlastEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackTracerChannelClearsTest::RunTest(const FString&)
{
	// (a) `SkipPlayback` con un tracer in volo: il canale si spegne.
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo A"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ false);
		ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(World);
		if (!TestTrue(TEXT("turno e mappa A"), TM != nullptr && Mappa != nullptr)) { return false; }

		TM->LockInAndResolve();
		for (int32 I = 0; I < 600 && TM->IsResolving() && Mappa->NumPlaybackTracers() == 0; ++I) { TM->Tick(0.02f); }
		if (!TestEqual(TEXT("premessa: un tracer in volo"), Mappa->NumPlaybackTracers(), 1)) { return false; }
		TM->SkipPlayback();
		TestEqual(TEXT("dopo SkipPlayback il canale e' spento"), Mappa->NumPlaybackTracers(), 0);
	}
	// (b) Fine del Blast: nessun tracer sopravvive alla fase.
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo B"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ true);
		ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(World);
		if (!TestTrue(TEXT("turno e mappa B"), TM != nullptr && Mappa != nullptr)) { return false; }

		TM->LockInAndResolve();
		if (!TestTrue(TEXT("premessa: il playback arriva al Blast"), TickUntilBlast(TM))) { return false; }
		for (int32 I = 0; I < 600 && TM->IsResolving() && TM->CurrentPlaybackPhaseForTest() == ERTMatchPhase::Blast; ++I)
		{
			TM->Tick(0.02f);
		}
		TestEqual(TEXT("uscito dal Blast, il canale e' spento"), Mappa->NumPlaybackTracers(), 0);
	}
	return true;
}
```

In `RTPlaybackStopPredicateTests.cpp`, estendi `Playback.NextActionStopsAtTheActionBoundary`: dopo le asserzioni
esistenti, prima di `return true;`:

```cpp
	// `#2454`: la fermata dopo un ARRIVO non lascia il tracer a mezz'aria accanto al suo numero. Si riprende
	// finche' la fermata cade su un colpo arrivato, poi si guarda la mappa.
	ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(World);
	if (!TestNotNull(TEXT("mappa di prova"), Mappa)) { return false; }
	for (int32 Giro = 0; Giro < 20 && TM->IsResolving(); ++Giro)
	{
		if (TM->IsPlaybackPaused() && TM->AttackBeatTraceForTest().Contains(TEXT("A0"))) { break; }
		TM->RequestPlaybackStopAt(ERTPlaybackStopAt::NextAction);
		TM->ResumePlayback();
		AdvanceUntilPausedOrDone(TM);
	}
	if (TM->IsPlaybackPaused() && TM->AttackBeatTraceForTest().Contains(TEXT("A0")))
	{
		TestEqual(TEXT("fermo dopo l'arrivo: nessun tracer in volo"), Mappa->NumPlaybackTracers(), 0);
	}
	else
	{
		AddError(TEXT("premessa: il playback non si e' mai fermato dopo l'arrivo del colpo"));
	}
```

e in `SetUpTwoPhaseTurn` (`:59`) aggiungi, prima di `return TM;`: `TM->bRecordAttackBeatsForTest = true;`.

- [ ] **Step 2: Build and run to verify the new tests fail**

Run: build, poi `python tools/suite/esegui.py RefactorTactics.Playback`
Expected: FAIL su `TracerIsInFlightBetweenLaunchAndArrival` (*«fra lancio e arrivo il tracer e' in volo»*) — nessuno
consegna ancora i tracer.

- [ ] **Step 3: Implement the delivery**

`RTTurnManager.h`, fra le dichiarazioni private accanto a `LaunchPlaybackAttack`:

```cpp
	/**
	 * Consegna alla mappa il tracer in volo (`#2454`). Col cursore unico ce n'e' AL PIU' UNO: il lancio di `i+1`
	 * segue l'arrivo di `i`. Il disegno dipende da chi guarda (`TracerStyleFor`), il ritmo no.
	 */
	void PushPlaybackTracers();
```

`RTTurnManager.cpp`, accanto a `ArrivePlaybackAttack`:

```cpp
void ARTTurnManager::PushPlaybackTracers()
{
	ARTHexMapActor* const MapActor = ARTHexMapActor::FindInWorld(GetWorld());
	if (!MapActor) { return; }

	TArray<FRTPlaybackTracer> InVolo;
	if (AttackBeatsDone % 2 == 1) // lanciato, non ancora arrivato
	{
		const int32 Index = AttackBeatsDone / 2;
		if (PlaybackAttacks.IsValidIndex(Index) && PlaybackAttackFlights.IsValidIndex(Index))
		{
			const FRTResolvedEvent& Atk = PlaybackAttacks[Index];
			const ERTTracerStyle Style = URTPlaybackLibrary::TracerStyleFor(Atk, PlaybackViewerTeamId);
			if (Style != ERTTracerStyle::None)
			{
				FRTPlaybackTracer T;
				T.From = Atk.HitGeometry.From;
				T.To = Atk.HitGeometry.Impact;
				T.Style = Style;
				T.Alpha = URTPlaybackLibrary::TracerAlpha(
					Index, PlaybackPhaseElapsed, AttackShowSeconds, PlaybackAttackFlights[Index]);
				InVolo.Add(T);
			}
		}
	}
	MapActor->SetPlaybackTracers(InVolo);
}
```

All'ingresso del ramo `if (Ph == ERTMatchPhase::Blast)` (`:8423`), come prima istruzione:

```cpp
		// `#2454`: il tracer si consegna a OGNI uscita di questo ramo — i tre `return` delle fermate compresi —
		// o a una fermata resterebbe disegnato nella posizione del tick prima. In pausa il tick non arriva qui
		// (`:8175`), e il tracer resta fermo dove l'ultima consegna lo ha lasciato.
		ON_SCOPE_EXIT{ PushPlaybackTracers(); };
```

(`#include "Misc/ScopeExit.h"` in testa al file se manca.)

Nella finalizzazione, dentro `if (Ph == ERTMatchPhase::Blast)`, dopo il `while` della rete:

```cpp
			// `#2454`: nessun tracer sopravvive alla fase. ⚠️ Qui e non solo in `FinishPlayback`, che esce presto
			// quando e' trattenuto da una finestra di reazione.
			if (ARTHexMapActor* const TracerMap = ARTHexMapActor::FindInWorld(GetWorld()))
			{
				TracerMap->ClearPlaybackTracers();
			}
```

In `FinishPlayback`, dentro `if (ARTHexMapActor* const FootprintMap = ...)` (`:8778`), accanto a
`ClearPlaybackFootprint()`:

```cpp
		FootprintMap->ClearPlaybackTracers(); // `#2454`: e passa di qui anche `SkipPlayback`
```

In `RTPresentationBinding.cpp`, la voce `Attack` (`:35-37`) diventa:

```cpp
	// `#2454`: `SetPlaybackTracers` — il tracer fra lancio e arrivo, solo per gli attacchi base `Single`/`Line` e
	// solo se chi guarda conosceva entrambi gli estremi. ⚠️ Il gate conta i nomi, non le chiamate: che la cue sia
	// chiamata lo dice `Playback.TracerIsInFlightBetweenLaunchAndArrival`.
	Out.Add(FRTPresentationBinding(ERTResolvedEventType::Attack,
		{ FName(TEXT("PlayAttackMontage")), FName(TEXT("PlayHitMontage")),
		  FName(TEXT("ShowDamageToken")), FName(TEXT("PulseHealthBar")),
		  FName(TEXT("SetPlaybackTracers")) }));
```

- [ ] **Step 4: Build and run the tests**

Run: `python tools/suite/esegui.py RefactorTactics.Playback+RefactorTactics.Presentation+RefactorTactics.Preview+RefactorTactics.HexMapActor`
Expected: PASS.

- [ ] **Step 5: Mutation checks**

1. Togli la riga `ON_SCOPE_EXIT{ PushPlaybackTracers(); };` → FAIL su `TracerIsInFlightBetweenLaunchAndArrival`.
2. Rimetti la riga ma sposta la consegna **dopo** il `while` dei battiti (non sulle uscite) → FAIL sulla parte
   `#2454` di `NextActionStopsAtTheActionBoundary`.
3. Togli `TracerMap->ClearPlaybackTracers();` dalla finalizzazione → FAIL su `TracerChannelClearsAtBlastEnd` (b).

Ripristina dopo ognuna, ricompila, PASS.

- [ ] **Step 6: Commit**

```bash
git add Source/RefactorTactics/Turn/RTTurnManager.h Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Turn/RTPresentationBinding.cpp Source/RefactorTactics/Tests/RTAttackTracerPlaybackTests.cpp Source/RefactorTactics/Tests/RTPlaybackStopPredicateTests.cpp
git commit -m "feat(2454): il tracer in volo consegnato alla mappa a ogni uscita del Blast, spento a fine fase"
```

---

### Task 7: Lo scenario e la voce PIE

**Files:**
- Create: `Scenarios/Visual/Combat/TracerHiddenFromUnseenAttacker.json`
- Modify: `docs/technical/test-manuali-pie.md` (riga nuova dopo `PIE-V01-FOOTPRINT`, `:1654`)
- Modify: `docs/roadmap/editor-sessions.yaml` (seduta nuova in `sessions:`, **prima** di `not_schedulable:`)
- Modify: `docs/superpowers/specs/2026-10-07-tracer-attacco-base-design.md` (§6.2 punto 3: lo scenario ha un nome)

- [ ] **Step 1: Write the scenario**

```json
{
  "scenarioId": "Visual.Combat.TracerHiddenFromUnseenAttacker",
  "tags": ["animation", "combat", "tracer", "privacy", "2454", "gadget", "riktor"],
  "version": 1,
  "seed": 0,
  "mapRadius": 5,

  "_nota": "BANCO VFX per il tracer (#2454, PIE-V01-TRACER scena 3). Branth spara ad Aevik DA DIETRO a 3 celle: oltre la consapevolezza ravvicinata (2) e fuori dall'arco frontale di Aevik, che guarda a Est. Chi guarda in PIE e' la squadra 0, quella di Aevik, e NON conosce la cella di Branth: il tracer non deve partire da li'. Si guarda il colpo che arriva senza una linea che lo preceda.",

  "_nota_loadout_vuoto": "Branth senza loadout: il default Weapon.Impact toglie una cella di portata (3 -> 2) e il colpo da 3 celle non partirebbe. La variabile deve stare ferma.",

  "_nota_forma": "ImpactShot e' Single di proposito: l'impronta di un Line rivela gia' oggi la cella adiacente a chi spara (spec §5, limite preesistente), e la domanda di questa scena si confonderebbe con quel limite.",

  "units": [
    { "id": "F1", "hero": "Hero.Aevik",  "team": 0, "cell": [0, 0, 0],  "facing": "E" },
    { "id": "B1", "hero": "Hero.Branth", "team": 1, "cell": [-3, 0, 0], "facing": "E", "loadout": [] }
  ],

  "turns": [
    {
      "_turno": "T1 — il colpo alle spalle: si guarda che NON compaia una linea dalla cella di Branth.",
      "intents": [
        { "unit": "B1", "ability": "Hero.Branth.ImpactShot", "target": "F1" }
      ]
    }
  ],

  "expect": [
    { "type": "UnitAlive",      "unit": "F1", "value": true },
    { "type": "UnitHpEquals",   "unit": "F1", "value": 87 },
    { "type": "TurnsCompleted", "value": 1 }
  ]
}
```

`87` = 90 HP di partenza di Aevik (`Visual.Combat.Defeat`, `_numeri_letti`) − (8 di `ImpactShot` − 5 di `BaseShield`,
D-224). ⚠️ Se `Scenario.EveryShippedScenarioRuns` lo misura diverso, **non** correggere il numero a occhio: leggi
dal log quale colpo non è arrivato o quale riduzione è intervenuta, e scrivilo in `_numeri_letti` come fa
`Defeat.json`.

- [ ] **Step 2: Run the scenario gates**

Run: `python tools/suite/esegui.py RefactorTactics.Scenario`
Expected: PASS, compresi `EveryShippedScenarioRuns`, `ShippedScenariosAreTagged`, `WriterRoundTripsShippedScenarios`.

- [ ] **Step 3: Write the PIE entry**

Riga nuova in `test-manuali-pie.md`, subito dopo quella di `PIE-V01-FOOTPRINT`, a **cinque** celle come le sorelle.
⚠️ Nessun `|` nudo dentro le celle; la nota sullo stato va **in coda** alla cella di stato (il registro legge la
prima issue citata come causa).

```markdown
| **PIE-V01-TRACER** | **Il colpo di un attacco base parte dall'attaccante e arriva sul bersaglio, prima del numero** ([#2454](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2454)) | da `L_DevSandbox`, in console: `rt.Test.Scenario <Id>` e Play. Tre scene, una domanda ciascuna | (1) `Visual.Combat.Defeat`, primo turno: *prima che compaia **ciascuno** dei due numeri, vedi qualcosa viaggiare dall'attaccante al bersaglio?* (2) `Visual.Combat.WaterElectricCoordinated`, il `PressureJet` di Muiren: *vedi una linea che si allunga da Muiren fino al bersaglio, ancorata a Muiren?* (3) `Visual.Combat.TracerHiddenFromUnseenAttacker`: *vedi una linea partire da una cella che non vedi?* — atteso **no**. ⚠️ In (1) il bersaglio scivola per la spinta di `Weapon.Impact`: è il limite noto della spec §5, non il criterio. | ⏳ **Aperta insieme alla feature.** La parte headless è coperta da `Playback.HitArrivesAfterTheLaunch`, `Playback.TracerIsInFlightBetweenLaunchAndArrival` e `Privacy.UnseenAttackerIsOutOfTheOriginVerdict`, validati per mutazione. ⚠️ Nessuno di quei test dice se si **legga** a schermo, ed è ciò che questa voce giudica. Spec: `docs/superpowers/specs/2026-10-07-tracer-attacco-base-design.md`. |
```

- [ ] **Step 4: Write the PIE session**

Misura l'id libero, contando anche i branch aperti (un `U<n>` è una risorsa contesa):

```bash
grep -o -E "^  - id: U[0-9]+" docs/roadmap/editor-sessions.yaml | sed 's/.*U//' | sort -n | tail -1
git log --all --oneline -S "id: U68" -- docs/roadmap/editor-sessions.yaml
```

Se il primo dice `66` e il secondo trova solo il branch del banco (`U67`, #3532), l'id è `U68`; altrimenti il
primo libero dopo entrambi. Inserisci la seduta come **ultimo elemento di `sessions:`**, cioè subito **prima** della
riga `not_schedulable:` — non in fondo al file:

```yaml
  - id: U68
    title: Il colpo che parte e arriva — il tracer degli attacchi base
    block: 6
    critical: false
    execution_lane: pie
    produces: verdetto su PIE-V01-TRACER, aperta insieme alla sua feature
    artifacts: []
    unblocked_by: []
    # Vuoto e misurato al merge: la feature e' in `main` con la PR di #2454, e la voce nomina i propri test
    # headless verdi. Cio' che manca e' un occhio.
    verifies:
      - PIE-V01-TRACER
    issues: [2454]
    done_when: ogni voce elencata in `verifies` ha esito reale nel registro
    unblocks: []
    notes: >-
      Tre scene su scenari del corpus, una domanda ciascuna. La terza e' la privacy: un tracer che partisse
      dalla cella di un attaccante non visto la rivelerebbe. Per il tiro alla cieca vedi PIE-V01-BLINDFIRE.
```

- [ ] **Step 5: Name the scenario in the spec**

In §6.2, punto 3, sostituisci *«Se il corpus non ha già questa scena, il piano la scrive.»* con
*«Scenario: `Visual.Combat.TracerHiddenFromUnseenAttacker`.»*

- [ ] **Step 6: Run the document gates**

```powershell
Set-Location D:\Repositories\rt-wt-2454-tracer
$skip='cli.ts','files.ts','git.ts','wiki-alt.ts'
Get-ChildItem tools/radar/*.ts | Where-Object { $_.Name -notlike '*.test.ts' -and $skip -notcontains $_.Name } | ForEach-Object { node $_.FullName --check *> $null; "{0,-24} exit={1}" -f $_.BaseName, $LASTEXITCODE }
python -m unittest discover -s tools/editor-sessions -p '*_test.py'
```

Expected: tutti `exit=0` tranne un eventuale `pie-verdict-age` rosso **già su `main`** — verificalo lanciando lo
stesso controllo su `origin/main` e confrontando le uscite: un rosso che cambia voce o testo è tuo.

- [ ] **Step 7: Commit**

```bash
git add Scenarios/Visual/Combat/TracerHiddenFromUnseenAttacker.json docs/technical/test-manuali-pie.md docs/roadmap/editor-sessions.yaml docs/superpowers/specs/2026-10-07-tracer-attacco-base-design.md
git commit -m "docs(2454): PIE-V01-TRACER e la seduta che la convoca, con lo scenario dell'attaccante non visto"
```

---

### Task 8: Verifica completa e chiusura

**Files:** nessun file nuovo; aggiornamenti su GitHub.

- [ ] **Step 1: Full suite on the declared commit**

Motore libero (comando in testa). Annota lo sha di `HEAD`, poi:

Run: `python tools/suite/esegui.py RefactorTactics`
Expected: `EXIT CODE: 0`, nessun `Fail`. ⚠️ La misura vale se `HEAD` e il binario sono gli stessi a inizio e fine
(`AGENTS.md` §9): annota `git rev-parse HEAD` e l'hash della DLL dell'editor prima e dopo.

- [ ] **Step 2: Merge `origin/main` and re-measure**

```bash
git fetch origin
git merge origin/main
```

Se il merge porta commit su `Source/RefactorTactics/Turn/`, `Map/` o `Tests/`, ricompila e rilancia la suite
completa: il gate deve appartenere al commit che si mergia.

- [ ] **Step 3: Open the PR**

Base: il branch padre. Il lavoro nasce da `docs/2454-tracer-attacco-base`, che nasce da `main`: verifica con
`git config branch.issue/2454-tracer-attacco-base.parent` o `git merge-base`, e apri la PR verso **quel** padre.
Corpo con `--body-file` (mai `--body` inline: esegue i backtick). Il corpo riporta, per ogni gate:
`Compile`, `Tests`, `Determinism`, `Replay`, `Privacy` = `PASS/FAIL` con lo sha; `PIE` = `NOT RUN` — appartiene alla
seduta; `Packaged` = `NOT RUN` — la resa in Shipping del line batcher si conferma sul pacchetto (D-467). Il corpo
dice che la PR **non chiude** #2454: restano `ReactionResolved`, l'attivazione e la sconfitta.

- [ ] **Step 4: Code review**

Lancia `/code-review` sulla PR; accogli o confuta ogni finding con evidenza (`superpowers:receiving-code-review`).

- [ ] **Step 5: Merge and update the issue**

Dopo il merge: commento su #2454 con lo sha mergiato, i gate e la riga della tabella dell'epic #2453 che cambia
(`Single`/`Line` → consegnate per gli attacchi base). Spunta in #2454 i criteri visivi che **restano** del registro
PIE solo come `⏳`, mai come fatti. Branch locale cancellato, worktree rimosso, `git remote prune origin`.

---

## Esecuzione — divergenze registrate

Il testo dei Task sopra resta quello scritto prima di eseguire. Dove il codice committato se ne scosta, la ragione è
qui, una riga per scarto: chi rilegge il piano per rifare un passo parte da ciò che è nel commit, non dalla bozza.

- **Task 1** — `PlaybackTracerIsItsOwnChannel` asserisce anche che il `Tick` dell'attore si accenda e si spenga con il
  canale. Il piano non lo prevedeva: l'attore nasce con il tick spento e nessun test guardava `HasAnythingToDraw`, quindi
  togliere `|| PlaybackTracers.Num() > 0` lasciava tutto verde mentre in partita il tracer non sarebbe mai stato disegnato.
- **Task 3** — la motivazione di `RTServerOnly` sta nel blocco `/** */` di `FRTResolvedEvent`, non in righe `//` come
  scrive il piano: UHT fondeva le righe `//` nel tooltip del tipo. `FRTPlannedIntent` e `FRTReactionOpportunity` fanno lo
  stesso.
- **Task 4** — anche `FRTHitGeometry` è `RTServerOnly`: porta i verdetti di ogni squadra, ed è il payload sensibile.
- **Task 4** — `AttackCarriesHitGeometry` indicizza le vittime **dopo** il turno: `StableUnitId` vale `0` prima di
  `LockInAndResolve`, che è dove si assegna: le due vittime avevano entrambe chiave `0`, la mappa collassava in una voce
  e `Find` non ne trovava nessuna.
- **Task 4** — `HitGeometryStaysOutOfHashes` ricalcola lo `StateHash` con `HashMatchState` + `BuildUnitDigests`:
  `GetPendingFinalStateHash` vale `0` senza registrazione del replay, e `0 == 0` era un confronto vacuo.
- **Task 5** — `Reactions.Brace.ExtendedBlastDoesNotReplayHits` lega `OnReactionWindowOpened`, perché la finestra che
  sospende il Blast si apre solo con un delegate legato, e asserisce `A0` **prima** dell'estensione: senza un arrivo
  già avvenuto il test non attraversa niente e la mutazione resterebbe verde per costruzione.
- **Task 6** — la parte `#2454` di `NextActionStopsAtTheActionBoundary` gira su `SetUpTwoActTurn` e non su
  `SetUpTwoPhaseTurn`: su un turno a un atto il colpo ha lo stesso `ActionId` dell'impronta, quindi un arrivo non è mai
  un confine di atto e il blocco prescritto dal piano non poteva fermarsi.
- **Task 6** — `TracerChannelClearsAtBlastEnd` ha un caso (c), assente dal piano: con (a) e (b) togliere
  `ClearPlaybackTracers` dalla finalizzazione lasciava tutto verde, perché `PhaseTime` dimensiona il Blast in modo che
  all'ultimo tick l'ultimo arrivo sia già passato e la consegna in uscita abbia già svuotato il canale. (c) abbassa
  `AttackShowSeconds` con un colpo in volo e mette un `Move` dopo il Blast: è l'unico regime in cui la pulizia di fine
  Blast conta.
- **Task 6** — `TracerIsInFlightBetweenLaunchAndArrival` pinna anche `OnAttackResolved` all'arrivo (decisione V1: *«il
  numero compare all'arrivo»*): `HitArrivesAfterTheLaunch` misura la traccia dei battiti e non cade se il broadcast
  torna al lancio.
- **Task 6** — test nuovo `Privacy.UnseenAttackerTracerIsNotDelivered`: nessun test guardava il filtro di privacy alla
  consegna, perché ogni fixture di playback aveva l'attaccante nella squadra dello spettatore e togliere la guardia
  `Style != None`, o sostituire `TracerStyleFor` con uno stile costante, lasciava tutto verde.
