# H5c.5 — Rimozione archi via tool (click-to-remove) — Implementation Plan

> ## 📦 `DELIVERED PLAN` — PIANO GIA' ESEGUITO, NON NORMATIVO
>
> Il piano di esecuzione, gia' eseguito. **Il corpo qui sotto non va aggiornato**: comandi, nomi di branch e percorsi sono quelli di allora, e
> correggerli falsificherebbe la storia invece di renderla utile.
>
> Stato corrente: [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md) · indice dei documenti correnti:
> [`../../README.md`](../../README.md). Banner aggiunto il 2026-08-08.

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development o executing-plans. Step con checkbox.

**Goal:** Rimuovere una transizione cliccandoci sopra nel viewport (tool Arch: Add-only → Add+Remove).

**Architecture:** 5a runtime: `URTHexLibrary::DistanceRayToSegment` (geometria pura, TDD) + `ARTHexMapActor::RemoveTransitionData` (estratto da `RemoveVerticalTransition`). 5b tool: `ERTHexArchOp Operation {Add,Remove}` nel property set; `OnClicked` in Remove fa hit-test ray↔segmento su tutte le transizioni e rimuove la più vicina entro soglia.

**Tech Stack:** UE 5.8.1 C++. Spec: [`h5c5-arch-remove-spec.md`](h5c5-arch-remove-spec.md).

## Global Constraints
- UE 5.8.1; `EngineAssociation` deve restare `"5.8"` (ripristina con `git checkout -- RefactorTactics.uproject` se risporcato).
- Prefissi `RT`/`URT`. NO `Build.cs` change. `FScopedTransaction` resta `#if WITH_EDITOR`.
- Branch **corrente** `feat/hex-grid`; no worktree/switch.
- **Staging solo-hex**: solo i file dello Step di commit; user's `docs/use-case-list.md` e `docs/PDR/*.pdf` NON committare.
- **Editor CHIUSO** durante il rebuild (gotcha ricorrente).
- Build: `"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex` → `Result: Succeeded`.
- Test headless: `"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -ExecCmds="Automation RunTests <PATTERN>; Quit" -unattended -nopause -nosplash -nullrhi` → `Fail` = 0.

---

## Task 1 (H5c.5a): `DistanceRayToSegment` + `RemoveTransitionData`

**Files:**
- Modify: `Source/RefactorTactics/Map/RTHexLibrary.h` (dichiara `DistanceRayToSegment`)
- Modify: `Source/RefactorTactics/Map/RTHexLibrary.cpp` (implementa `DistanceRayToSegment`)
- Modify: `Source/RefactorTactics/Map/RTHexMapActor.h` (dichiara `RemoveTransitionData`)
- Modify: `Source/RefactorTactics/Map/RTHexMapActor.cpp` (implementa `RemoveTransitionData`; `RemoveVerticalTransition` wrapper)
- Test: `Source/RefactorTactics/Tests/RTHexTests.cpp` (nuovo `DistanceRayToSegment`)

**Interfaces:**
- Produces: `static float URTHexLibrary::DistanceRayToSegment(const FVector& RayOrigin, const FVector& RayDir, const FVector& A, const FVector& B)`; `bool ARTHexMapActor::RemoveTransitionData(const FRTCellId& From, const FRTCellId& To, bool bBothDirections)`.
- Consumes: `URTHexMapAsset::RemoveTransition`.

- [ ] **Step 1: Scrivere il test che fallisce (`DistanceRayToSegment`)**

In `Source/RefactorTactics/Tests/RTHexTests.cpp`, prima di `#endif // WITH_DEV_AUTOMATION_TESTS`, aggiungere:
```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexDistanceRaySegTest,
	"RefactorTactics.Hex.DistanceRayToSegment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexDistanceRaySegTest::RunTest(const FString&)
{
	// 1. Ray attraversa il segmento -> ~0.
	{
		const float D = URTHexLibrary::DistanceRayToSegment(FVector(1, 0, -10), FVector(0, 0, 1), FVector(0, 0, 0), FVector(2, 0, 0));
		TestTrue(TEXT("interseca -> 0"), FMath::IsNearlyZero(D, 1e-3f));
	}
	// 2. Ray parallelo al segmento, offset 5 in Y -> ~5.
	{
		const float D = URTHexLibrary::DistanceRayToSegment(FVector(0, 5, 0), FVector(1, 0, 0), FVector(0, 0, 0), FVector(10, 0, 0));
		TestTrue(TEXT("parallelo offset 5"), FMath::IsNearlyEqual(D, 5.f, 1e-3f));
	}
	// 3. Closest oltre l'estremo B -> sqrt(10^2 + 3^2) = sqrt(109).
	{
		const float D = URTHexLibrary::DistanceRayToSegment(FVector(20, 3, 0), FVector(0, 0, 1), FVector(0, 0, 0), FVector(10, 0, 0));
		TestTrue(TEXT("oltre estremo"), FMath::IsNearlyEqual(D, FMath::Sqrt(109.f), 1e-2f));
	}
	// 4. Segmento degenere (A==B): distanza punto-ray = 5.
	{
		const float D = URTHexLibrary::DistanceRayToSegment(FVector(0, 0, 5), FVector(0, 1, 0), FVector(0, 0, 0), FVector(0, 0, 0));
		TestTrue(TEXT("segmento degenere"), FMath::IsNearlyEqual(D, 5.f, 1e-3f));
	}
	return true;
}
```

- [ ] **Step 2: Build → verificare che fallisce a compilazione**

Run build → **FAIL** («`DistanceRayToSegment` is not a member of `URTHexLibrary`»).

- [ ] **Step 3: Dichiarare `DistanceRayToSegment`**

In `Source/RefactorTactics/Map/RTHexLibrary.h`, subito dopo la dichiarazione di `WorldToLayer` (`static int32 WorldToLayer(double WorldZ, double OriginZ, float LayerHeight);`), aggiungere:
```cpp
	/** Distanza minima tra la semi-retta (RayOrigin + t*RayDir, t>=0) e il segmento A..B. Pura, per hit-test archi. */
	static float DistanceRayToSegment(const FVector& RayOrigin, const FVector& RayDir, const FVector& A, const FVector& B);
```

- [ ] **Step 4: Implementare `DistanceRayToSegment`**

In `Source/RefactorTactics/Map/RTHexLibrary.cpp`, dopo l'implementazione di `WorldToLayer` (dopo la sua `}` di chiusura), aggiungere:
```cpp
float URTHexLibrary::DistanceRayToSegment(const FVector& RayOrigin, const FVector& RayDir, const FVector& A, const FVector& B)
{
	// Closest points tra semi-retta (s>=0, dir unitaria) e segmento (t in [0,1]). Adattato da Ericson, con doppio clamp.
	const FVector D1 = RayDir.GetSafeNormal(); // direzione ray (unitaria) -> s = distanza lungo il ray
	const FVector D2 = B - A;                   // direzione segmento -> t in [0,1]
	const double E = FVector::DotProduct(D2, D2);
	const FVector R = RayOrigin - A;
	const double B1 = FVector::DotProduct(D1, D2);
	const double C = FVector::DotProduct(D1, R);
	const double F = FVector::DotProduct(D2, R);

	double S = 0.0; // lungo il ray (>=0)
	double T = 0.0; // lungo il segmento ([0,1])
	if (E <= (double)SMALL_NUMBER)
	{
		// Segmento degenere (A==B): T=0, S = proiezione di A sul ray (clamp>=0).
		T = 0.0;
		S = FMath::Max(0.0, -C);
	}
	else
	{
		const double Denom = E - B1 * B1; // = |D2|^2 - dot(D1,D2)^2 (>=0)
		T = (Denom > (double)SMALL_NUMBER) ? ((F - C * B1) / Denom) : (F / E);
		T = FMath::Clamp(T, 0.0, 1.0);
		S = FMath::Max(0.0, T * B1 - C);
		T = FMath::Clamp((F + S * B1) / E, 0.0, 1.0);
	}

	const FVector PRay = RayOrigin + static_cast<float>(S) * D1;
	const FVector QSeg = A + static_cast<float>(T) * D2;
	return static_cast<float>(FVector::Dist(PRay, QSeg)); // FVector::Dist ritorna double in UE5: cast esplicito (no C4244)
}
```

- [ ] **Step 5: Build + eseguire il test → passa**

Run build → `Result: Succeeded`. Poi:
`... -ExecCmds="Automation RunTests RefactorTactics.Hex.DistanceRayToSegment; Quit" ...` → **Success**, 0 Fail.

- [ ] **Step 6: Estrarre `RemoveTransitionData` sull'actor**

In `Source/RefactorTactics/Map/RTHexMapActor.h`, subito dopo la dichiarazione di `RemoveVerticalTransition()` (riga ~129), aggiungere:
```cpp
	/** Rimuove la transizione From->To (e l'inversa se bBothDirections) dall'asset. Vero se ha rimosso. Annullabile. */
	bool RemoveTransitionData(const FRTCellId& From, const FRTCellId& To, bool bBothDirections);
```

In `Source/RefactorTactics/Map/RTHexMapActor.cpp`, sostituire l'INTERO metodo `RemoveVerticalTransition()` (righe ~240-259). Da:
```cpp
void ARTHexMapActor::RemoveVerticalTransition()
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato."));
		return;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexRemoveTransition", "Hex: Remove Vertical Transition"));
#endif
	MapAsset->Modify();
	const bool bRemoved = MapAsset->RemoveTransition(TransitionFrom, TransitionTo, bTransitionBidirectional);
	if (bRemoved)
	{
		MapAsset->MarkPackageDirty();
		RebuildInstances();
	}
	UE_LOG(LogRT, Log, TEXT("[HexMap] Rimozione transizione %s -> %s: %s."),
		*TransitionFrom.ToString(), *TransitionTo.ToString(), bRemoved ? TEXT("rimossa") : TEXT("non trovata"));
}
```
a (wrapper + nuovo metodo):
```cpp
void ARTHexMapActor::RemoveVerticalTransition()
{
	RemoveTransitionData(TransitionFrom, TransitionTo, bTransitionBidirectional);
}

bool ARTHexMapActor::RemoveTransitionData(const FRTCellId& From, const FRTCellId& To, bool bBothDirections)
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato."));
		return false;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexRemoveTransition", "Hex: Remove Vertical Transition"));
#endif
	MapAsset->Modify();
	const bool bRemoved = MapAsset->RemoveTransition(From, To, bBothDirections);
	if (bRemoved)
	{
		MapAsset->MarkPackageDirty();
		RebuildInstances();
	}
	UE_LOG(LogRT, Log, TEXT("[HexMap] Rimozione transizione %s -> %s: %s."),
		*From.ToString(), *To.ToString(), bRemoved ? TEXT("rimossa") : TEXT("non trovata"));
	return bRemoved;
}
```
> Comportamento invariato rispetto all'attuale `RemoveVerticalTransition` (stessa guardia/transazione/log), solo parametrizzato.

- [ ] **Step 7: Build + suite completa → verde**

Run build → `Result: Succeeded`. Poi `... -ExecCmds="Automation RunTests RefactorTactics; Quit" ...` → **0 Fail** (incluso il nuovo `DistanceRayToSegment`).

- [ ] **Step 8: Commit**

```bash
git checkout -- RefactorTactics.uproject   # se risporcato
git add Source/RefactorTactics/Map/RTHexLibrary.h Source/RefactorTactics/Map/RTHexLibrary.cpp \
        Source/RefactorTactics/Map/RTHexMapActor.h Source/RefactorTactics/Map/RTHexMapActor.cpp \
        Source/RefactorTactics/Tests/RTHexTests.cpp
git commit -m "feat(hex): H5c.5a - DistanceRayToSegment (test) + RemoveTransitionData"
```

---

## Task 2 (H5c.5b): Tool Remove (Operation {Add,Remove} + hit-test)

**Files:**
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.h` (enum `ERTHexArchOp`; property `Operation`; dichiara `RemoveNearestArch`)
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.cpp` (branch Remove in `OnClicked`; def. `RemoveNearestArch`)
- Modify: `docs/design/test-manuali-pie.md` (voce PIE-HEX-MODE-L)
- Modify: `docs/design/hex-map-roadmap.md` (riga H5: H5c.5 fatta)

**Interfaces:**
- Consumes: `URTHexLibrary::{DistanceRayToSegment,AxialToWorld}`; `ARTHexMapActor::{MapAsset,GetActorLocation,RemoveTransitionData}`; `URTHexMapAsset::{Transitions,HexSize,LayerHeight}`; `RTHexEditor::FindTargetMapActor`.

- [ ] **Step 1: Nota testing (nessun test headless)**

La geometria è coperta dal Task 1; la selezione dell'arco più vicino + soglia sono editor-bound → PIE-HEX-MODE-L (Step 5). *(Dichiarazione DoD.)*

- [ ] **Step 2: Header — enum, property `Operation`, `RemoveNearestArch`**

In `Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.h`:

(a) Dopo `class UCombinedTransformGizmo;` (riga ~10), aggiungere l'enum:
```cpp
/** Operazione del tool archi: crea (gizmo) o rimuove (click sull'arco). */
UENUM()
enum class ERTHexArchOp : uint8
{
	Add,
	Remove
};
```
(b) In `URTHexArchToolProperties`, come PRIMA proprietà (prima di `Kind`), aggiungere:
```cpp
	UPROPERTY(EditAnywhere, Category = "Hex|Arco")
	ERTHexArchOp Operation = ERTHexArchOp::Add;
```
(c) In `URTHexArchTool`, nella sezione `protected`, dopo `void DestroyPendingGizmo();`, aggiungere:
```cpp
	/** Rimuove la transizione la cui linea From->To e' piu' vicina al ray del click, entro soglia. */
	void RemoveNearestArch(ARTHexMapActor* Actor, const FInputDeviceRay& ClickPos);
```

- [ ] **Step 3: `OnClicked` — branch Remove + `RemoveNearestArch`**

In `Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.cpp`, in `OnClicked`, subito dopo il blocco che verifica `Actor` (dopo `if (!Actor) { ... return; }`, prima di `FRTCellId Cell;`), inserire il branch Remove:
```cpp
	if (Properties && Properties->Operation == ERTHexArchOp::Remove)
	{
		RemoveNearestArch(Actor, ClickPos);
		return;
	}
```
E, dopo il metodo `ClearPending()` (o prima di `Render`), aggiungere la definizione:
```cpp
void URTHexArchTool::RemoveNearestArch(ARTHexMapActor* Actor, const FInputDeviceRay& ClickPos)
{
	DestroyPendingGizmo(); // esci da un eventuale Add pendente

	const URTHexMapAsset* Map = Actor->MapAsset;
	if (!Map || Map->Transitions.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[HexMode] Remove: nessuna transizione nell'asset."));
		return;
	}

	const FVector Origin = Actor->GetActorLocation();
	const float HexSize = Map->HexSize;
	const float LayerH = Map->LayerHeight;
	const FVector RayO = ClickPos.WorldRay.Origin;
	const FVector RayD = ClickPos.WorldRay.Direction;

	int32 BestIdx = INDEX_NONE;
	float BestDist = TNumericLimits<float>::Max();
	for (int32 I = 0; I < Map->Transitions.Num(); ++I)
	{
		const FRTHexEdge& E = Map->Transitions[I];
		const FVector A = URTHexLibrary::AxialToWorld(E.From, Origin, HexSize, LayerH);
		const FVector B = URTHexLibrary::AxialToWorld(E.To, Origin, HexSize, LayerH);
		const float Dist = URTHexLibrary::DistanceRayToSegment(RayO, RayD, A, B);
		if (Dist < BestDist) { BestDist = Dist; BestIdx = I; }
	}

	if (BestIdx != INDEX_NONE && BestDist <= HexSize * 0.6f)
	{
		// Copia From/To PRIMA di rimuovere (RemoveTransitionData muta l'array Transitions).
		const FRTCellId F = Map->Transitions[BestIdx].From;
		const FRTCellId T = Map->Transitions[BestIdx].To;
		Actor->RemoveTransitionData(F, T, /*bBothDirections=*/true);
		UE_LOG(LogTemp, Log, TEXT("[HexMode] Arco rimosso %s -> %s (dist %.1f)."), *F.ToString(), *T.ToString(), BestDist);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[HexMode] Nessun arco entro soglia (min dist %.1f)."), BestDist);
	}
}
```

- [ ] **Step 4: Build del target Editor (editor chiuso)**

Run build → `Result: Succeeded`. Poi (sanity) run suite `RefactorTactics` → **0 Fail**.

- [ ] **Step 5: Voce PIE + roadmap**

Aggiungere a `docs/design/test-manuali-pie.md`:
```markdown
| **PIE-HEX-MODE-L** | Rimuovi arco via tool (H5c.5b) | mode Hex Map, tool Arch, `ARTHexMapActor` con transizioni | Con `Operation=Remove`, click su un arco disegnato lo rimuove (Undo lo ripristina); click nel vuoto (nessun arco entro soglia) non fa nulla; con `Operation=Add` il flusso gizmo resta invariato | ⏳ (branch `feat/hex-grid`, H5c.5b) |
```
In `docs/design/hex-map-roadmap.md`, riga **H5** Stato, aggiungere in coda: `H5c.5: rimozione archi via tool - ERTHexArchOp{Add,Remove}; in Remove OnClicked fa hit-test URTHexLibrary::DistanceRayToSegment (test) su tutte le transizioni e rimuove la piu' vicina entro HexSize*0.6 via ARTHexMapActor::RemoveTransitionData. Verifica editor PIE-HEX-MODE-L aperta.`

- [ ] **Step 6: Commit**

```bash
git checkout -- RefactorTactics.uproject   # se risporcato
git add Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.h \
        Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.cpp \
        docs/design/test-manuali-pie.md docs/design/hex-map-roadmap.md
git commit -m "feat(hex): H5c.5b - rimozione archi via tool (Operation Add/Remove + hit-test)"
```

---

## Self-Review (eseguita)
- **Copertura spec**: §3.1 geometria → T1 Step 3-4 (+test 1-2,5); §3.2 RemoveTransitionData → T1 Step 6; §3.3 tool (Operation + branch Remove + hit-test) → T2 Step 2-3; PIE → T2 Step 5; roadmap → T2 Step 5. Nessun gap.
- **Placeholder**: nessuno. `DistanceRayToSegment` validato a mano sui 4 casi del test.
- **Consistenza**: `DistanceRayToSegment(FVector,FVector,FVector,FVector)→float`, `RemoveTransitionData(FRTCellId,FRTCellId,bool)→bool` coerenti tra dichiarazione (T1) e uso (T2). Copia From/To prima di `RemoveTransitionData` (no dangling sull'array Transitions).

## Rischi noti
- Soglia world-space `HexSize*0.6` (limite dichiarato). Geometria coperta da test; hit-test/soglia editor-bound (PIE).
- `RemoveVerticalTransition` ri-espresso: comportamento invariato (guardia/transazione/log). Rebuild a editor chiuso.
