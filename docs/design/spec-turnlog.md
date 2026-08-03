# Spec — TurnLog strutturato + reason codes (Movimento + Combat)

> `/sc:spec-panel` + brainstorming del **2026-08-03**. Obiettivo utente (task **P3** della roadmap): *«reason codes +
> TurnLog»* — rendere ogni esito del turno **spiegabile** e produrre una traccia deterministica del round.
> Panel: **Nygard** (osservabilità/robustezza), **Wiegers** (DoD misurabile), **Fowler** (confini), **Crispin/Adzic** (test per esempi).
> Ancorata al codice (`RTTurnManager`, `RTMovementResolver`, `RTCombatResolver`, `RTCombatLibrary`, `RTResolvedEvent`),
> al canone ([`piano-canonico-mvp.md`](piano-canonico-mvp.md) invarianti #1/#3/#4/#7, §5.1, §6) e alla roadmap
> ([`roadmap-checkpoint.md`](roadmap-checkpoint.md)).
> **Documentale: questo file non modifica il codice.**

> ✅ **Nessun conflitto di scope**: P3 **non** è north-star. La roadmap lo traccia già come mitigazione del rischio
> *«Resolver difficile da spiegare → TurnLog reason codes»* (risk register) e come base del KPI *«Replay divergence = 0»*
> (KPI/Performance). Questo slice ne realizza la **prima parte** (log strutturato + reason), **rimandando** la
> serializzazione/hash di replay a uno slice successivo.

---

## 1. Obiettivo & scope

Produrre un **TurnLog**: sequenza **autoritativa, deterministica e permutazione-invariante** degli esiti del turno
per **Movimento** e **Combat**, con un **reason code** (enum intero) per ogni esito, **inclusi i non-eventi**
(mosse bloccate, danno assorbito, assenza di LOS). I reason sono riflessi anche nel **combat log HUD** esistente
(stringhe arricchite). Il TurnLog è **in-memory** e coperto da **test**.

**Fuori scope (dichiarato):**
- Serializzazione versionata + **hash di replay** (`Replay divergence = 0`) → slice successivo.
- Reason codes per **Hazard** (terreno/lava) e **Status** (Root/Slow/Reveal) → estensione.
- UI dedicata del TurnLog oltre l'arricchimento del combat log già presente.

---

## 2. Stato attuale (verificato dal codice)

| Fatto | Evidenza |
|---|---|
| Combat log = `RecentEvents: TArray<FString>` + `AddLogEvent(FString)` (scrive su `LogRT` e accoda alla HUD, `MaxLogLines=6`) | `RTTurnManager.h:63-64,131-135,141` |
| `FRTResolvedEvent` = sequenza **già deterministica** di eventi risolti per il **playback** (`Move/Attack/HazardDamage/Defeated`; Phase/Source/Target/Path/Amount) | `RTResolvedEvent.h:11-51` |
| `URTMovementResolver` calcola contesa/scambio/blocco **internamente** ma restituisce **solo** celle finali (nessun outcome) | `RTMovementResolver.h:57-58,67`; `FRTPathResult{Final,Entered}` :39-48 |
| `URTCombatResolver::ResolveAttacks` applica scudo→HP e somma; `FRTAttack{TargetIndex,Power}` **non** porta source né reason | `RTCombatResolver.h:24-37,52-53` |
| Copertura/LOS sono decise **a monte** del resolver (in `URTCombatLibrary`, prima di costruire `FRTAttack`) | canone §6; roadmap CP 3.6 |
| `ARTUnit` non ha id esplicito, ma ha `TeamId` e `GridCell`; vale «max 1 unità/cella» | `RTUnit.h:36,47`; canone §6 |

**Conseguenza di design**: l'outcome nasce in **due luoghi** — Movimento **dentro** il resolver puro; Combat in
parte **a monte** (LOS/copertura in `URTCombatLibrary`) e in parte **nel** resolver (scudo/letale). Va **esposto**
tramite funzioni pure, non ricostruito nell'Actor.

---

## 3. Principio fondante

> **Osservabilità autoritativa, separata dalla presentazione.** Il TurnLog descrive *cosa e perché* è successo
> secondo le regole (C++), non *come si mostra*. È coerente con gli invarianti:
> #1 (le regole decidono), #3 («raccogli poi applica» + ordinamento §5.1), #4 (determinismo, niente float),
> #7 (combat math = funzioni pure).

Il TurnLog **non** riusa `FRTResolvedEvent` (che resta la struttura di *playback*): sono due assi di cambiamento
distinti (osservabilità vs presentazione). Vedi decisione **D-TL-1** (§12).

---

## 4. Componenti — nuovo file `Turn/RTTurnLog.h`

```cpp
UENUM(BlueprintType) enum class ERTLogCategory : uint8 { Move, Combat };

UENUM(BlueprintType) enum class ERTMoveOutcome : uint8 {
    Moved, BlockedContested, BlockedOccupied, SwapAllowed, OutOfBudget, BlockedByCover
};

UENUM(BlueprintType) enum class ERTCombatOutcome : uint8 {
    DirectHit, CoverReduced, ShieldAbsorbed, Lethal, NoLineOfSight
};

USTRUCT(BlueprintType)
struct FRTTurnLogEntry {
    GENERATED_BODY()
    ERTMatchPhase   Phase   = ERTMatchPhase::Move;
    ERTLogCategory  Category = ERTLogCategory::Move;
    uint8           Outcome = 0;   // cast dell'enum secondo Category (intero — invariante #4)
    FRTGridCoord    SrcCell;       // chiave STABILE: cella di partenza dell'unità nel turno
    FRTGridCoord    TgtCell;       // bersaglio (Combat) o destinazione (Move); = SrcCell se n/a
    int32           Amount  = 0;   // danno effettivo (Combat) / n. celle percorse (Move)
};
```

Il TurnLog è un `TArray<FRTTurnLogEntry>`, membro di `ARTTurnManager`, con getter `GetTurnLog()`.

---

## 5. Classificazione pura (il cuore testabile)

- **Movimento** — estendere `URTMovementResolver` per **esporre** l'outcome per-unità (nuova
  `FRTMoveResolution{Final, Entered, ERTMoveOutcome}` **oppure** campo `Outcome` su `FRTPathResult`, con default per
  non rompere i chiamanti esistenti). La derivazione contesa/scambio/blocco/budget **è già dentro** il resolver: la si
  restituisce invece di scartarla. Rimane funzione pura → **test diretto**.
- **Combat** — due funzioni pure in `URTCombatLibrary`:
  - `ClassifyPreCombat(...)` → `NoLineOfSight` / `CoverReduced` / `DirectHit`, dal confronto power nominale vs
    effettivo + esito LOS (dati già calcolati dalla library).
  - `ClassifyPostCombat(Pre, Post)` → `ShieldAbsorbed` / `Lethal`, dal confronto stato pre/post del resolver.

`ARTTurnManager` fa **solo da collettore**: invoca le funzioni pure e accoda le entry (nessuna decisione nell'Actor).

---

## 6. Data flow & ordinamento

`LockInAndResolve` → per fase i resolver girano come oggi → le funzioni pure classificano → `ARTTurnManager`:
1. accoda `FRTTurnLogEntry`,
2. chiama `AddLogEvent` con la stringa **arricchita** dal reason (es. *«Guardian: mossa bloccata (cella contesa)»*,
   *«Ranger → Bot: 15 (copertura -50%)»*, *«Bot eliminato (HP 0)»*).

**Ordinamento del TurnLog** (deterministico, invariante #3/§5.1): **fase → categoria → `SrcCell`** (StableTieBreak
per-coord già usato nel path finding). Non dipende **mai** dall'ordine d'inserimento nel container.

---

## 7. Determinismo & invarianti

- **Chiave unità = cella di partenza del turno** (max 1/cella ⇒ univoca), **mai** pointer/spawn-order.
- **Nessun float** nel log/ordinamento; `Outcome` intero (invariante #4).
- **Permutazione-invarianza** (requisito cardine): permutare l'array di `FRTMoveRequest` / `FRTAttack` in input
  **non cambia** il TurnLog (né l'ordine né i valori).
- Il TurnLog è **additivo**: i 66 test esistenti restano verdi (estensioni con campi a default).

**Requisiti vincolanti (SMART):**
- **`FR-TURNLOG-01`** — ogni esito di Movimento e Combat produce **una** `FRTTurnLogEntry` con reason code corretto,
  inclusi i non-eventi. *Verifica: test per ogni valore di enum.*
- **`FR-TURNLOG-02`** — il TurnLog è **permutazione-invariante**. *Verifica: permutare l'input → log identico.*
- **`FR-TURNLOG-03`** — nessun float nell'entry/ordinamento (coerente con invariante #4).
- **`FR-TURNLOG-04`** — il combat log HUD mostra il reason per ogni evento (stringa arricchita), senza cambiare la
  logica (invariante #1).

---

## 8. Testing & Definition of Done

Nuovo `Tests/RTTurnLogTests.cpp` (Automation, **headless**). Esempi (Given/When/Then):

- Movimento: 2 unità verso la stessa cella → **entrambe** `BlockedContested`; A↔B → `SwapAllowed`; destinazione
  oltre il budget → `OutOfBudget`; su cella di copertura → `BlockedByCover`; mossa libera → `Moved`.
- Combat: colpo pieno → `DirectHit`; con copertura → `CoverReduced` (+`Amount` ridotto); scudo assorbe tutto →
  `ShieldAbsorbed`; HP a ≤0 → `Lethal`; senza LOS → `NoLineOfSight` (nessun danno).
- Determinismo: permutare gli array di input → **TurnLog identico** (bit per bit).

**DoD dello slice:** ☐ i **66 automation test** esistenti restano verdi ☐ nuovi test TurnLog verdi (uno per enum +
permutazione) ☐ build target Editor/Game **Succeeded** ☐ combat log HUD arricchito col reason (verifica PIE)
☐ nessun float nell'entry/ordinamento ☐ `spec`/roadmap aggiornate ☐ nessun file generato/segreto committato.

---

## 9. File coinvolti

**Nuovi**: `Turn/RTTurnLog.h`, `Tests/RTTurnLogTests.cpp`, questa spec.
**Modificati**: `Turn/RTMovementResolver.h/.cpp` (espone outcome), `Combat/RTCombatLibrary.h/.cpp` (2 funzioni
`Classify*`), `Turn/RTTurnManager.h/.cpp` (colleziona TurnLog + `GetTurnLog()` + `AddLogEvent` arricchito).

---

## 10. Rischi / da confermare nel piano

- **Punto esatto di classificazione combat**: leggere `RTCombatLibrary.cpp/.h` per confermare dove nascono
  copertura/LOS (assunzione: funzioni pure già presenti — `HasLineOfSight`, riduzione da copertura). Circoscritto.
- **`ResolveMoves` vs `ResolvePaths`**: confermare quale usa il flusso reale (movimento v2 a waypoint) per agganciare
  l'outcome nel posto giusto; possibile che entrambe vadano estese.
- **Interazione con lo State-Based (`FR-RESOLVE-02`, §5.1)**: nel combat attuale «danni sommati», la morte è dopo il
  batch → `Lethal` si determina **dopo** l'applicazione. Nessun «attacco su morto» intra-batch da gestire in questo slice.

---

## 11. Roadmap / slicing

| ID | Cosa | Verifica |
|----|------|----------|
| **TL.1** | `RTTurnLog.h` (enum + `FRTTurnLogEntry`) + `URTMovementResolver` espone `ERTMoveOutcome` | test movimento (uno per enum) + permutazione; 66 test verdi |
| **TL.2** | `URTCombatLibrary::Classify{Pre,Post}Combat` (`ERTCombatOutcome`) | test combat (uno per enum) + permutazione |
| **TL.3** | `ARTTurnManager` colleziona il TurnLog + `AddLogEvent` arricchito col reason | build + PIE: combat log mostra i reason |

Slice successivo (spec separata): **serializzazione versionata + hash** (chiude `Replay divergence = 0`).

---

## 12. Decisioni

**Prese (2026-08-03):**
- **D-TL-1** — TurnLog come **struttura autoritativa separata** (`FRTTurnLogEntry`), non estensione di
  `FRTResolvedEvent` (che resta playback). Motivo: separare osservabilità da presentazione (invariante #1, SRP) e non
  destabilizzare il playback.
- **D-TL-2** — chiave unità = **cella di partenza del turno** (deterministica, permutazione-invariante), non
  pointer/spawn-order.
- **D-TL-3** — outcome esposti da **funzioni pure** (resolver + `URTCombatLibrary`); l'Actor è solo collettore.
- **D-TL-4** — scope = **Movimento + Combat**; hazard/status e hash di replay **rimandati**.

**Aperte (da confermare nel piano/PIE):**
- Estendere `FRTPathResult` con `Outcome` **oppure** nuova `FRTMoveResolution` (dipende dai chiamanti reali).
- Formato esatto delle stringhe arricchite del combat log (presentazione, tunabile).

---

## 13. Riferimenti

- Canone: [`piano-canonico-mvp.md`](piano-canonico-mvp.md) — invarianti #1/#3/#4/#7, §5.1 (APNAP/tie-break), §6 (conflitti/copertura/morte).
- Roadmap: [`roadmap-checkpoint.md`](roadmap-checkpoint.md) — risk register («Resolver difficile da spiegare»), KPI («Replay divergence = 0»).
- Codice: `RTTurnManager.h/.cpp`, `RTMovementResolver.h/.cpp`, `RTCombatResolver.h/.cpp`, `RTCombatLibrary.h/.cpp`, `RTResolvedEvent.h`, `RTUnit.h`.
- Playback (struttura sorella, non riusata): [`spec-anima-risoluzione.md`](spec-anima-risoluzione.md).
