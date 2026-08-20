# Spec — AS.5 Identità di team (anello a terra)

> ## 🧱 `AS-BUILT` — implementata, non un piano
>
> Questa spec parlava al futuro come se l'anello fosse ancora da costruire. **È realizzata**: l'anello di team
> è in codice (`ARTUnit`, con `RingLocalZ` che compensa `VisualZOffset`) e la verifica interattiva è
> registrata nel registro PIE.
>
> 🔴 **Riallineata il 2026-08-16 con `#593` (PR #977), ed era diventata attivamente dannosa.** Le §2–§4
> descrivevano `Mesh` come root, `TeamRing->SetupAttachment(Mesh)` e una `RelativeLocation.Z` divisa per
> `BaseMeshScale.Z`. Il root oggi è un `USceneComponent` neutro e nessuna di quelle tre cose è più vera:
> una spec `AS-BUILT` che descrive codice inesistente non è documentazione ferma, è l'istruzione a
> **rimettere** la compensazione appena tolta. Le righe superate restano ~~barrate~~ con la misura nuova
> accanto, perché il *perché* del cambio vale quanto lo stato finale.
>
> **Non aggancia una resa visiva definitiva del roster**: quale personaggio corrisponda a Gadget, Phase, Riktor
> o Wraith è una decisione **aperta** ([`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md)). L'anello funziona
> indipendentemente da quella scelta — è esattamente il motivo per cui fu preferito al MID `"Color"`.

> Brainstorming del **2026-08-03** (dopo AS.1–AS.4). Obiettivo: rendere i due team distinguibili **dall'alto** con
> personaggi skeletal texturizzati, dove il MID `"Color"` sul cilindro non si vede.
> Ancorata al codice (`RTUnit.cpp/.h`), al canone ([`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) invariante #1 «le
> regole decidono, la presentazione riproduce»; §6 «leggibilità dall'alto, i due team riconoscibili»),
> alla pipeline asset ([`spec-asset-pipeline.md`](../architecture/spec-asset-pipeline.md) §4.1, che rimandava a AS.5).
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
| Identità team = MID `"Color"` (Team0=blu / Team1=rosso) sul cilindro `Mesh`, in `ApplyTeamColor()` (BeginPlay) | `RTUnit.cpp` |
| **Root = `SceneRoot`**, `USceneComponent` a scala **unitaria**; `Mesh` è un suo figlio | `RTUnit.cpp` (costruttore) |
| `Mesh` = `UStaticMeshComponent` con `BaseMeshScale=(1.2,1.2,1.8)` (scala **non uniforme**) — ~~root~~ | `RTUnit.h` |
| `Team0Color`/`Team1Color` (`FLinearColor`), `VisualZOffset` (90 cilindro / 0 personaggio) | `RTUnit.h` |
| Selezione = `Mesh->SetRelativeScale3D(BaseMeshScale*1.15)` — tocca il cilindro, ~~non più il root~~ | `RTUnit.cpp` (`OnSelected`) |
| Con `BP_Unit` skeletal il cilindro è **nascosto** → il MID Color non si vede | `spec-asset-pipeline.md §4.1` |

⚠️ **I numeri di riga sono stati tolti, non aggiornati**, ed erano marci da prima. Erano **quattro**
riferimenti `file:riga`, e alla base di `#593` — cioè *prima* di toccare qualsiasi cosa — **tre su
quattro** puntavano già altrove: `RTUnit.cpp:54` era una parentesi chiusa, `:106-111` due parentesi
aperte, `RTUnit.h:252-255` era `PlannedPath`, cioè movimento e non colori di squadra. Solo
`RTUnit.cpp:34` cadeva davvero su `TeamRing->SetupAttachment(Mesh)`.

Nessun gate se n'era accorto, e non poteva: il file esiste e la riga esiste **sempre**. Un puntatore
`file:riga` non fallisce mai rumorosamente — smette di dire il vero e continua a sembrare preciso. Il
nome del simbolo (`OnSelected`, `ApplyTeamColor`) lo trova `grep`, e non scade.

---

## 3. Componenti (C++ in `RTUnit`)

- Nuovo `TObjectPtr<UStaticMeshComponent> TeamRing` (costruttore), `SetupAttachment(SceneRoot)`
  (~~`SetupAttachment(Mesh)`~~, cambiato da `#593`), **NoCollision**, mesh engine
  `/Engine/BasicShapes/Cylinder` scalata a disco piatto.
- Nuovo `TObjectPtr<UMaterialInstanceDynamic> RingDynMaterial` (transient).
- Nuovo `TSoftObjectPtr<UMaterialInterface> TeamRingMaterial` (→ `M_TeamRing`, `EditAnywhere`).
- `static FLinearColor TeamColorFor(int32 TeamId, const FLinearColor& Team0, const FLinearColor& Team1)` — **pura, testabile**.

## 4. Colorazione, scala e posizione

- `ApplyTeamColor()` esteso: colore = `TeamColorFor(TeamId, Team0Color, Team1Color)` (usato sia per il cilindro sia
  per il ring). Se `TeamRingMaterial` carica → `RingDynMaterial = MID(M_TeamRing)`, `SetVectorParameterValue("Color", ...)`,
  `TeamRing->SetVisibility(true)`. Altrimenti `TeamRing->SetVisibility(false)` (fallback).
- **Scala indipendente**: ~~il ring è figlio del cilindro scalato `(1.2,1.2,1.8)` e la selezione cambia quella
  scala~~ — dal `#593` il ring è **fratello** del cilindro sotto `SceneRoot`, quindi né la deformazione né
  l'ingrandimento del 15% lo raggiungono, per costruzione. `TeamRing->SetUsingAbsoluteScale(true)` **resta**, ma
  copre ormai un caso diverso: la scala dell'**attore** (che è la scala del root), modificabile per istanza in
  livello o in Blueprint. ⚠️ E la copre a metà — `bAbsoluteScale` congela la dimensione e lascia la traslazione
  moltiplicata dal genitore, quindi su un attore scalato l'anello tiene la taglia e sposta la quota. Valeva
  identico prima di `#593`. `SetRelativeScale3D` fissa il raggio (~cella).
- **Posizione a terra**: l'attore è a `+VisualZOffset` sopra la cella e la `RelativeLocation.Z` del ring lo
  riporta al piano. La formula è `Z = -VisualZOffset + RingGroundClearance`
  (~~`Z = (-VisualZOffset / BaseMeshScale.Z) + ε`~~): la divisione è sparita con il genitore che la rendeva
  necessaria, e con lei la guardia div-by-zero, che senza denominatore non proteggeva più niente.
  Con `VisualZOffset=90` → `-88.2`; con `0` → `1.8`. **Stessa quota-mondo nei due casi** — invariante che la
  vecchia formula otteneva solo perché entrambi i rami venivano moltiplicati per la stessa scala.
  Impostata in `ApplyTeamColor` (`VisualZOffset` noto a BeginPlay).

  🔴 **`RingGroundClearance = 1.8` non è un ε, ed è il numero che `#593` ha sbagliato una volta.** Il disco
  della cella è un cilindro engine appiattito da `RTCellFlatScale = 0.05`, quindi la sua faccia superiore sta a
  `50 × 0.05 = 2.5`. L'anello è alto `50 × 0.02 = 1.0` di semi-altezza: perché **emerga** serve
  `Clearance + 1.0 > 2.5`, cioè `> 1.5`. Una stesura aveva messo `1.0` — bordo a `2.0`, mezza unità **dentro**
  un disco opaco: anello di squadra e anello di selezione invisibili, con la suite **verde**, perché nessun
  test guardava da quel lato. Oggi lo falsifica `RefactorTactics.Unit.RingClearsCellDisc`.
  ⚠️ `RTCellTopZ` è `constexpr` in un namespace anonimo di `Map/RTHexMapActor.cpp`: il `2.5` qui e nel test è
  una **seconda copia**, e condividere la costante è **#983**.

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

- Canone: [`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) — invariante #1, §6 (leggibilità/team).
- Pipeline asset: [`spec-asset-pipeline.md`](../architecture/spec-asset-pipeline.md) §4.1 (identità team rimandata a AS.5).
- Codice: `RTUnit.cpp/.h` (`ApplyTeamColor`, `Mesh`, `VisualZOffset`, colori).
