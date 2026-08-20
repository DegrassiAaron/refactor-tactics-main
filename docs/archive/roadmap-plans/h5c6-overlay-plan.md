# H5c.6 — Overlay debug superfici (colora celle per dato) — Implementation Plan

> ## 📦 `DELIVERED PLAN` — PIANO GIA' ESEGUITO, NON NORMATIVO
>
> Il piano di esecuzione, gia' eseguito. **Il corpo qui sotto non va aggiornato**: comandi, nomi di branch e percorsi sono quelli di allora, e
> correggerli falsificherebbe la storia invece di renderla utile.
>
> Stato corrente: [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md) · indice dei documenti correnti:
> [`../../README.md`](../../README.md). Banner aggiunto il 2026-08-08.

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development o executing-plans. Step con checkbox.

**Goal:** Rendere visibile ciò che il Paint scrive: con un toggle, ogni cella dell'asset è disegnata come esagono colorato per **superficie** (`ERTHexSurface`) + le celle **bloccate** (`bBlocksMovement`) marcate con un esagono rosso interno. Chiude il gap "dipingo superfici ma l'ISM le mostra tutte identiche".

**Architecture (approccio A — PDI solo-codice):** un helper editor condiviso `RTHexEditor::SurfaceColor` (mappa presentazione) + `RTHexEditor::DrawSurfaceOverlay` (disegna gli esagoni via PDI, riusa `DrawHexMarker`). Toggle `bShowOverlay` nei property set di **Select** e **Paint**; il loro `Render` chiama l'helper. Nessun cambio runtime, nessun asset, nessun nuovo test headless (overlay editor-bound → PIE). Filled/persistente (materiale) e costo/LOS = follow-up.

**Tech Stack:** UE 5.8.1 C++ (editor module only). Design deciso col dev: approccio A (PDI), overlay in Select+Paint, superficie+blocco.

## Global Constraints
- UE 5.8.1; `EngineAssociation` deve restare `"5.8"` (ripristina con `git checkout -- RefactorTactics.uproject` se risporcato).
- Prefissi `RT`/`URT`. NO `Build.cs`/runtime change. Editor deps confinate al modulo `RefactorTacticsEditor`.
- Branch **corrente** `feat/hex-grid`; no worktree/switch.
- **Staging solo-hex**: solo i file dello Step di commit; user's `docs/use-case-list.md` e `docs/PDR/*.pdf` NON committare.
- **Editor CHIUSO** durante il rebuild (gotcha ricorrente: questa sessione ha già avuto un blocco per editor aperto — se aperto, STOP e riporta, non killare).
- Build: `"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex` → `Result: Succeeded`.
- Test headless: `"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -ExecCmds="Automation RunTests RefactorTactics; Quit" -unattended -nopause -nosplash -nullrhi` → `Fail` = 0.

---

## Task 1 (H5c.6): overlay superfici (helper + toggle in Select/Paint)

**Files:**
- Modify: `Source/RefactorTacticsEditor/Private/RTHexEditorClick.h` (dichiara `SurfaceColor`, `DrawSurfaceOverlay`)
- Modify: `Source/RefactorTacticsEditor/Private/RTHexEditorClick.cpp` (implementa)
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexSelectTool.h` (property `bShowOverlay`)
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexSelectTool.cpp` (Render con overlay)
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h` (property `bShowOverlay`)
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp` (Render con overlay)
- Modify: `docs/design/test-manuali-pie.md` (voce PIE-HEX-MODE-M)
- Modify: `docs/design/hex-map-roadmap.md` (riga H5)

**Interfaces:**
- Produces: `FColor RTHexEditor::SurfaceColor(ERTHexSurface)`; `void RTHexEditor::DrawSurfaceOverlay(FPrimitiveDrawInterface*, const ARTHexMapActor*)`.
- Consumes: `URTHexLibrary::AxialToWorld`; `URTHexMapAsset::{Cells,HexSize,LayerHeight}`; `RTHexEditor::{DrawHexMarker,FindTargetMapActor}`.

- [ ] **Step 1: Nota testing (nessun test headless)**

`SurfaceColor` è mappatura di presentazione editor; `DrawSurfaceOverlay` è editor-bound (PDI). Nessuna logica pura runtime → nessun nuovo test headless. Verifica in editor → PIE-HEX-MODE-M (Step 5). *(Dichiarazione DoD.)*

- [ ] **Step 2: Helper — dichiarazioni**

In `Source/RefactorTacticsEditor/Private/RTHexEditorClick.h`, dopo `struct FInputDeviceRay;` (riga ~9), aggiungere la forward-declaration:
```cpp
enum class ERTHexSurface : uint8;
```
e dentro il `namespace RTHexEditor`, dopo la dichiarazione di `DrawHexMarker(...)`, aggiungere:
```cpp
	/** Colore d'overlay per una superficie cella (presentazione editor). */
	FColor SurfaceColor(ERTHexSurface Surface);

	/** Overlay debug: ogni cella dell'asset come esagono colorato per superficie; le bloccate con un esagono rosso interno. */
	void DrawSurfaceOverlay(FPrimitiveDrawInterface* PDI, const ARTHexMapActor* Actor);
```

- [ ] **Step 3: Helper — implementazioni**

In `Source/RefactorTacticsEditor/Private/RTHexEditorClick.cpp`, dentro il `namespace RTHexEditor` (prima della `}` di chiusura `// namespace RTHexEditor`), aggiungere:
```cpp
FColor SurfaceColor(ERTHexSurface Surface)
{
	switch (Surface)
	{
	case ERTHexSurface::Water:       return FColor(60, 120, 255);
	case ERTHexSurface::Mud:         return FColor(140, 100, 60);
	case ERTHexSurface::Fire:        return FColor(255, 130, 40);
	case ERTHexSurface::Electrified: return FColor(80, 230, 230);
	case ERTHexSurface::Ice:         return FColor(160, 220, 255);
	case ERTHexSurface::Void:        return FColor(150, 40, 150);
	case ERTHexSurface::Normal:
	default:                         return FColor(160, 160, 160);
	}
}

void DrawSurfaceOverlay(FPrimitiveDrawInterface* PDI, const ARTHexMapActor* Actor)
{
	if (!PDI || !Actor || !Actor->MapAsset) { return; }
	const URTHexMapAsset* Map = Actor->MapAsset;
	const FVector Origin = Actor->GetActorLocation();
	const float HexSize = Map->HexSize;
	const float LayerH = Map->LayerHeight;
	// Coerente con RebuildInstances: in ActiveOnly l'overlay mostra solo il layer attivo (niente piani impilati).
	const bool bActiveOnly = (Actor->LayerView == ERTLayerViewMode::ActiveOnly);
	const int32 ActiveLayer = Actor->ActiveLayer;
	for (const FRTHexCellData& Cell : Map->Cells)
	{
		if (bActiveOnly && Cell.Id.Layer != ActiveLayer) { continue; }
		const FVector Center = URTHexLibrary::AxialToWorld(Cell.Id, Origin, HexSize, LayerH);
		DrawHexMarker(PDI, Center, HexSize * 0.85f, SurfaceColor(Cell.Surface));
		if (Cell.bBlocksMovement)
		{
			DrawHexMarker(PDI, Center, HexSize * 0.45f, FColor::Red); // esagono interno rosso = bloccata
		}
	}
}
```
> `FRTHexCellData`/`ERTHexSurface`/`Cells`/`ERTLayerViewMode`/`LayerView`/`ActiveLayer` arrivano da `Map/RTHexMapActor.h` + `Map/RTHexMapAsset.h` (già inclusi nel .cpp); `AxialToWorld` da `Map/RTHexLibrary.h` (già incluso).

- [ ] **Step 4: Toggle + Render in Select e Paint**

(a) In `Source/RefactorTacticsEditor/Private/Tools/RTHexSelectTool.h`, in `URTHexSelectToolProperties` dopo l'ultima proprietà (`bool bBlocksMovement = false;`), aggiungere:
```cpp
	/** [Overlay] Colora le celle per superficie (debug read-only); le bloccate con esagono rosso. */
	UPROPERTY(EditAnywhere, Category = "Hex|Overlay")
	bool bShowOverlay = false;
```

(b) In `Source/RefactorTacticsEditor/Private/Tools/RTHexSelectTool.cpp`, sostituire l'INTERO metodo `Render`:
```cpp
void URTHexSelectTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!bHasSelection || !RenderAPI) { return; }
	RTHexEditor::DrawHexMarker(RenderAPI->GetPrimitiveDrawInterface(), SelectedWorldCenter, MarkerRadius, FColor::Yellow);
}
```
con:
```cpp
void URTHexSelectTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!RenderAPI) { return; }
	FPrimitiveDrawInterface* PDI = RenderAPI->GetPrimitiveDrawInterface();
	if (!PDI) { return; }

	if (Properties && Properties->bShowOverlay)
	{
		RTHexEditor::DrawSurfaceOverlay(PDI, RTHexEditor::FindTargetMapActor(TargetWorld));
	}
	if (bHasSelection)
	{
		RTHexEditor::DrawHexMarker(PDI, SelectedWorldCenter, MarkerRadius, FColor::Yellow);
	}
}
```

(c) In `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h`, in `URTHexPaintToolProperties` dopo l'ultima proprietà (`bool bLastExisted = false;`, con la sua UPROPERTY `VisibleAnywhere, Category = "Hex|Ultimo"`), aggiungere:
```cpp
	/** [Overlay] Colora le celle per superficie (debug read-only); le bloccate con esagono rosso. */
	UPROPERTY(EditAnywhere, Category = "Hex|Overlay")
	bool bShowOverlay = false;
```

(d) In `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp`, sostituire l'INTERO metodo `Render`:
```cpp
void URTHexPaintTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!bHasMarker || !RenderAPI) { return; }
	RTHexEditor::DrawHexMarker(RenderAPI->GetPrimitiveDrawInterface(), MarkerCenter, MarkerRadius, MarkerColor);
}
```
con:
```cpp
void URTHexPaintTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!RenderAPI) { return; }
	FPrimitiveDrawInterface* PDI = RenderAPI->GetPrimitiveDrawInterface();
	if (!PDI) { return; }

	if (Properties && Properties->bShowOverlay)
	{
		RTHexEditor::DrawSurfaceOverlay(PDI, RTHexEditor::FindTargetMapActor(TargetWorld));
	}
	if (bHasMarker)
	{
		RTHexEditor::DrawHexMarker(PDI, MarkerCenter, MarkerRadius, MarkerColor);
	}
}
```

- [ ] **Step 5: Build + suite + PIE + roadmap**

Build (editor chiuso) → `Result: Succeeded`. Poi run suite `RefactorTactics` → **0 Fail** (nessun cambio runtime → invariata).
Aggiungere a `docs/design/test-manuali-pie.md`:
```markdown
| **PIE-HEX-MODE-M** | Overlay debug superfici (H5c.6) | mode Hex Map, tool Select o Paint, `ARTHexMapActor` con celle di superfici diverse | Con `bShowOverlay` attivo, ogni cella appare come esagono colorato per superficie (Water blu, Fire arancio, Mud marrone, ...); le celle bloccate hanno un esagono rosso interno; `bShowOverlay` off = nessun overlay | ⏳ (branch `feat/hex-grid`, H5c.6) |
```
In `docs/design/hex-map-roadmap.md`, riga **H5** Stato, aggiungere in coda: `H5c.6: overlay debug superfici - RTHexEditor::SurfaceColor + DrawSurfaceOverlay (esagoni PDI colorati per superficie + rosso su bBlocksMovement); toggle bShowOverlay in Select e Paint. Verifica editor PIE-HEX-MODE-M aperta.`

- [ ] **Step 6: Commit**

```bash
git checkout -- RefactorTactics.uproject   # se risporcato
git add Source/RefactorTacticsEditor/Private/RTHexEditorClick.h \
        Source/RefactorTacticsEditor/Private/RTHexEditorClick.cpp \
        Source/RefactorTacticsEditor/Private/Tools/RTHexSelectTool.h \
        Source/RefactorTacticsEditor/Private/Tools/RTHexSelectTool.cpp \
        Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h \
        Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp \
        docs/design/test-manuali-pie.md docs/design/hex-map-roadmap.md
git commit -m "feat(hex): H5c.6 - overlay debug superfici (SurfaceColor + DrawSurfaceOverlay, toggle in Select/Paint)"
```

---

## Self-Review (eseguita)
- **Copertura design**: helper SurfaceColor+DrawSurfaceOverlay → Step 2-3; toggle+Render Select → Step 4a-b; toggle+Render Paint → Step 4c-d; PIE → Step 5; roadmap → Step 5. Nessun gap.
- **Placeholder**: nessuno; codice completo. `SurfaceColor` esaustivo su `ERTHexSurface` (Normal/Water/Mud/Fire/Electrified/Ice/Void) + default.
- **Consistenza**: `SurfaceColor(ERTHexSurface)→FColor`, `DrawSurfaceOverlay(FPrimitiveDrawInterface*, const ARTHexMapActor*)` coerenti tra dichiarazione (Step 2) e uso (Step 3-4). Le due Render riscritte preservano il marker esistente + aggiungono l'overlay senza early-return che lo sopprima.

## Rischi noti
- Solo overlay (contorni), non riempimento (approccio A dichiarato). `DrawSurfaceOverlay` disegna tutte le celle a ogni frame quando attivo: accettabile a scala di authoring. Rebuild a editor chiuso.
- `enum class ERTHexSurface : uint8;` forward-decl nel .h: valido perché usata solo per valore in una dichiarazione; il .cpp include il tipo completo.
