# Spec — AS.5 Identità di team (anello a terra)

> Brainstorming del **2026-08-03** (dopo AS.1–AS.4). Obiettivo: rendere i due team distinguibili **dall'alto** con
> personaggi skeletal texturizzati, dove il MID `"Color"` sul cilindro non si vede.
> Ancorata al codice (`RTUnit.cpp/.h`), al canone ([`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) invariante #1 «le
> regole decidono, la presentazione riproduce»; §6 «leggibilità dall'alto, i due team riconoscibili»),
> alla pipeline asset ([`spec-asset-pipeline.md`](spec-asset-pipeline.md) §4.1, che rimandava a AS.5).
> **Documentale: questo file non modifica il codice.**

---

## 1. Obiettivo & scope

Un **anello/disco colorato a terra** sotto ogni unità (blu team 0, rosso team 1), sempre visibile dall'alto (camera a
pitch -55°), che identifica la squadra indipendentemente dalla mesh (cilindro o personaggio). **Presentazione**
(invariante #1): non tocca la logica. **Fallback**: senza `M_TeamRing`, l'anello è nascosto e il MID `"Color"` sul
cilindro resta (comportamento attuale invariato). **Fuori scope**: outline/decal (approcci alternativi scartati), VFX.

---

## 2. Stato attuale (verificato dal codice)

| Fatto | Evidenza |
|---|---|
| Identità team = MID `"Color"` (Team0=blu / Team1=rosso) sul cilindro `Mesh` (root), in `ApplyTeamColor()` (BeginPlay) | `RTUnit.cpp:54-75,34` |
| `Mesh` = root `UStaticMeshComponent` con `BaseMeshScale=(1.2,1.2,1.8)` (scala **non uniforme**) | `RTUnit.h:246,258` |
| `Team0Color`/`Team1Color` (`FLinearColor`), `VisualZOffset` (90 cilindro / 0 personaggio) | `RTUnit.h:252-255,216-217` |
| Selezione = `Mesh->SetRelativeScale3D(BaseMeshScale*1.15)` (la scala del root cambia) | `RTUnit.cpp:106-111` |
| Con `BP_Unit` skeletal il cilindro è **nascosto** → il MID Color non si vede | `spec-asset-pipeline.md §4.1` |

---

## 3. Componenti (C++ in `RTUnit`)

- Nuovo `TObjectPtr<UStaticMeshComponent> TeamRing` (costruttore), `SetupAttachment(Mesh)`, **NoCollision**,
  mesh engine `/Engine/BasicShapes/Cylinder` scalata a disco piatto.
- Nuovo `TObjectPtr<UMaterialInstanceDynamic> RingDynMaterial` (transient).
- Nuovo `TSoftObjectPtr<UMaterialInterface> TeamRingMaterial` (→ `M_TeamRing`, `EditAnywhere`).
- `static FLinearColor TeamColorFor(int32 TeamId, const FLinearColor& Team0, const FLinearColor& Team1)` — **pura, testabile**.

## 4. Colorazione, scala e posizione

- `ApplyTeamColor()` esteso: colore = `TeamColorFor(TeamId, Team0Color, Team1Color)` (usato sia per il cilindro sia
  per il ring). Se `TeamRingMaterial` carica → `RingDynMaterial = MID(M_TeamRing)`, `SetVectorParameterValue("Color", ...)`,
  `TeamRing->SetVisibility(true)`. Altrimenti `TeamRing->SetVisibility(false)` (fallback).
- **Scala indipendente** (il rischio chiave): il ring è figlio del cilindro scalato `(1.2,1.2,1.8)` e la selezione
  cambia quella scala → `TeamRing->SetUsingAbsoluteScale(true)` così il ring **non eredita** la deformazione né la
  selezione; `SetRelativeScale3D` fissa il raggio (~cella).
- **Posizione a terra**: il root è a `+VisualZOffset` sopra la cella; la `RelativeLocation.Z` del ring compensa la
  scala Z del genitore: `Z = (-VisualZOffset / BaseMeshScale.Z) + ε`. Con 90/1.8 → -50 → world = cella (terra); con
  `VisualZOffset=0` → 0 → terra. Impostata in `ApplyTeamColor` (VisualZOffset noto a BeginPlay).

## 5. Fallback & invarianti

- `TeamRingMaterial` nullo → ring nascosto; MID cilindro invariato → **comportamento attuale preservato**.
- Additivo: i **70 test** restano verdi (nessun test dipende dal ring).
- Presentazione: nessun effetto su griglia/logica (invariante #1).

**Requisiti (SMART):**
- **`FR-TEAM-01`** — `TeamColorFor(0,·,·)==Team0`, `TeamColorFor(1,·,·)==Team1` (e default = Team1). *Verifica: test.*
- **`FR-TEAM-02`** — senza `M_TeamRing`, avvio senza crash e cilindro colorato come oggi. *Verifica: PIE + test verdi.*
- **`FR-TEAM-03`** — con `M_TeamRing`, anello colorato a terra, per team, visibile dall'alto. *Verifica: PIE.*

## 6. Testabilità (onesta)

Slice **di presentazione**: l'unica parte automatizzabile è `TeamColorFor` (1 test). Ring visibile/colore/posizione =
**verifica PIE** (non headless). Dichiarato, non nascosto.

## 7. Piano di implementazione (TDD, conciso)

| Passo | Cosa | Verifica |
|---|---|---|
| **1** | `TeamColorFor` pura (RED→GREEN) in `RTUnit.h/.cpp` + test in `RTCombatLibraryTests`/nuovo | test `RefactorTactics.Unit.TeamColorFor` |
| **2** | `ApplyTeamColor` usa `TeamColorFor` per il cilindro (refactor, nessun cambio di comportamento) | 70 test verdi |
| **3** | `TeamRing` componente (costruttore) + colorazione/scala/posizione + fallback visibilità | build Editor/Game; PIE |

## 8. File

- **Modificati**: `Unit/RTUnit.h` (`TeamRing`, `RingDynMaterial`, `TeamRingMaterial`, `TeamColorFor`), `Unit/RTUnit.cpp`
  (costruttore, `ApplyTeamColor` esteso), `Tests/*` (test `TeamColorFor`).
- **Editor (utente)**: creare `M_TeamRing` (unlit/emissive + parametro `Color`), assegnarlo a `TeamRingMaterial` sui
  `BP_Unit` (o class default). Guida nei passi editor a valle.

## 9. Decisioni

- **D-TEAM-1** — anello a terra (mesh+MID), non outline/decal (semplice, robusto, leggibile dall'alto).
- **D-TEAM-2** — materiale dedicato `M_TeamRing` (unlit/emissive) con soft-ptr + fallback (ring nascosto se assente).
- **D-TEAM-3** — `TeamColorFor` pura (unico pezzo testabile headless); resto = PIE.
- **D-TEAM-4** — scala assoluta del ring per non ereditare la deformazione/selezione del cilindro.

## 10. Riferimenti

- Canone: [`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) — invariante #1, §6 (leggibilità/team).
- Pipeline asset: [`spec-asset-pipeline.md`](spec-asset-pipeline.md) §4.1 (identità team rimandata a AS.5).
- Codice: `RTUnit.cpp/.h` (`ApplyTeamColor`, `Mesh`, `VisualZOffset`, colori).
