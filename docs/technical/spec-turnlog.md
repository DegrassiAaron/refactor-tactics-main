# Spec — TurnLog strutturato + reason codes (Movimento + Combat)

> ℹ️ **Regola vigente, esempi datati.** Il TurnLog è **neutro rispetto alla topologia** e resta normativo: al CP 7.1 i suoi test sono stati classificati fra i **neutri** e sono sopravvissuti alla rimozione del quadrato.
> Gli snippet citano API rimosse al **CP 7.2** (`URTGridLibrary`, `FRTGridCoord`): vanno letti come pseudo-codice, non come firme correnti.

> `/sc:spec-panel` + brainstorming del **2026-08-03**. Obiettivo utente (task **P3** della roadmap): *«reason codes +
> TurnLog»* — rendere ogni esito del turno **spiegabile** e produrre una traccia deterministica del round.
> Panel: **Nygard** (osservabilità/robustezza), **Wiegers** (DoD misurabile), **Fowler** (confini), **Crispin/Adzic** (test per esempi).
> Ancorata al codice (`RTTurnManager`, `RTMovementResolver`, `RTCombatResolver`, `RTCombatLibrary`, `RTResolvedEvent`),
> al canone ([`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) invarianti #1/#3/#4/#7, §5.1, §6) e alla roadmap
> ([`roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md)).
> **Documentale: questo file non modifica il codice.**

> ✅ **Nessun conflitto di scope**: P3 **non** è north-star. La roadmap lo traccia già come mitigazione del rischio
> *«Resolver difficile da spiegare → TurnLog reason codes»* (risk register) e come base del KPI *«Replay divergence = 0»*.
> Questo slice ne realizza la **prima parte** (log strutturato + reason), rimandando la serializzazione/hash di replay.

> ⚠️ **Revisione 2026-08-03 (reason codes allineati al codice)**: la stesura iniziale ipotizzava reason inesistenti
> nel codice. Verifica (`RTTurnManager.cpp:565-724`, `RTCombatLibrary.h`, `RTPlayerController.cpp:231-277`):
> **la copertura NON riduce il danno** (blocca la LOS ⇒ attacco scartato); **budget/blocco-su-copertura avvengono in
> PIANIFICAZIONE** (non nel resolver); l'unico modificatore di danno è l'**altura** (`EffectiveAttackPower`, +danno).
> I reason sono stati corretti di conseguenza (rimossi `CoverReduced`/`BlockedByCover`/`OutOfBudget`; aggiunti
> `TerrainBonus`, `NoLineOfSight`, `Stayed`). Decisione utente: set **Core + NoLineOfSight + TerrainBonus** (§12, D-TL-5).

> ⚠️ **Emendamento 2026-08-08 — due affermazioni della revisione sopra non valgono più.**
> Erano **corrette al 3 agosto**, quando la copertura era solo un bloccante di LOS e non esisteva E9: non sono
> errori di allora, sono fotografie scadute. Il §2 «Stato attuale» va letto con questo cappello.
>
> | La revisione 2026-08-03 diceva | Oggi |
> |---|---|
> | «la copertura **non** riduce il danno, blocca la LOS ⇒ attacco scartato» | **Dipende dal tipo**: la copertura **bassa riduce** il danno (CP 9.1), la copertura **alta blocca** il bordo (CP 9.2). La riduzione decade se il colpo arriva fuori dall'arco frontale ([ADR-0005](../decisions/adr-0005-orientamento.md) §4a) |
> | «l'unico modificatore di danno è l'**altura** (`EffectiveAttackPower`, +danno)» | **Falso oggi.** `OccupantDamageBonus` è un parametro **generico** e ogni call site runtime passa `0` — verificato il 2026-08-08 in `RTTurnManager.cpp`. La quota **non** dà danno ([D-024](../decisions/RT_PDR_00_Decision_Log.md)): vale per geometria |
>
> **Categorie e outcome correnti** (da `Turn/RTTurnLog.h`, non da memoria):
> `ERTLogCategory = { Move, Combat, Fallback, Reaction, Environment }` ·
> `ERTEnvironmentOutcome = { SurfaceChanged, SurfaceRestored, SurfaceRejected, SurfaceExtinguished, CoverDamaged, CoverDestroyed }`.
>
> **Serve una nuova versione di formato per questi valori? No** — e la ragione è nel codice, non nella prudenza:
> i valori nuovi sono **accodati** e viaggiano come `uint8`, quindi le tracce già su disco restano leggibili.
> Si incrementa `ERTTurnLogFormatVersion` solo quando cambia il **layout** di header o voce; l'ultimo
> incremento è stato **v4 `WithFormatId`** (CP 10.3). Inserire un valore *in mezzo* rinumererebbe `Combat`,
> cioè riscriverebbe il significato dei file esistenti: quello sì richiederebbe una versione.
>
> Dettaglio delle regole di copertura: [`../gameplay/spec-copertura-cp91.md`](../gameplay/spec-copertura-cp91.md) ·
> [`../gameplay/spec-copertura-alta-cp92.md`](../gameplay/spec-copertura-alta-cp92.md).

---

## 1. Obiettivo & scope

Produrre un **TurnLog**: sequenza **autoritativa, deterministica e permutazione-invariante** degli esiti del turno
per **Movimento** e **Combat**, con un **reason code** (enum intero) per ogni esito, **inclusi i non-eventi**
(unità che non si muove/bloccata, attacco senza LOS, danno assorbito). I reason sono riflessi anche nel **combat log
HUD** esistente (stringhe arricchite). Il TurnLog è **in-memory** e coperto da **test**.

**Fuori scope (dichiarato):** serializzazione versionata + **hash di replay** (slice successivo); reason per Hazard
(terreno/lava) e Status (Root/Slow/Reveal); UI dedicata oltre l'arricchimento del combat log.

---

## 2. Stato attuale (verificato dal codice)

| Fatto | Evidenza |
|---|---|
| Combat log = `RecentEvents: TArray<FString>` + `AddLogEvent(FString)` (scrive su `LogRT` e accoda alla HUD, `MaxLogLines=6`) | `RTTurnManager.h:63-64,131-135` |
| `FRTResolvedEvent` = sequenza **già deterministica** per il **playback** (`Move/Attack/HazardDamage/Defeated`) | `RTResolvedEvent.h:11-51` |
| **Movimento** usa `URTMovementResolver::ResolvePaths` (microstep sincroni); congela per **contesa** (`Contenders>=2`) o **cella occupata da unità ferma** | `RTTurnManager.cpp:902`; `RTMovementResolver.cpp:41-57` |
| `ResolvePaths` restituisce `FRTPathResult{Final,Entered}` — **nessun outcome** esposto | `RTMovementResolver.h:39-48` |
| **Copertura = blocca la LOS** (`GetVisionBlockers`); senza LOS l'attacco è **scartato** (non ridotto) | `RTTurnManager.cpp:567-570,612-618` |
| **Budget / blocco-su-copertura** rifiutati in **PIANIFICAZIONE** (input), non nella risoluzione | `RTPlayerController.cpp:231-233,265-277` |
| Danno effettivo = `EffectiveAttackPower(Power, OccupantDamageBonus)` — bonus **generico di cella**, ~~altura +danno~~: ogni call site runtime passa `0` ([D-024](../decisions/RT_PDR_00_Decision_Log.md)) | `RTTurnManager.cpp`; `RTCombatLibrary.h` |
| `URTCombatResolver::ResolveAttacks` applica scudo→HP e somma; morte via `NewlyDefeated(BeforeHP,AfterHP)` | `RTTurnManager.cpp:703-724`; `RTCombatLibrary.h:70-71` |
| `ARTUnit` senza id esplicito; ha `TeamId` e `GridCell`; vale «max 1 unità/cella» | `RTUnit.h:36,47`; canone §6 |

**Conseguenza di design**: l'outcome del **Movimento** va **esposto** da `ResolvePaths` (che internamente conosce il
motivo del congelamento); l'outcome del **Combat** è in parte **funzione pura** (`Hit/ShieldAbsorbed/Lethal/TerrainBonus`
da stato pre/post + bonus) e in parte **wiring** nell'Actor (`NoLineOfSight` = attacco pianificato scartato per LOS).

---

## 3. Principio fondante

> **Osservabilità autoritativa, separata dalla presentazione.** Il TurnLog descrive *cosa e perché* secondo le regole
> (C++). Coerente con: #1 (le regole decidono), #3 («raccogli poi applica» + ordinamento §5.1), #4 (determinismo, niente
> float), #7 (funzioni pure). Il TurnLog **non** riusa `FRTResolvedEvent` (playback): assi di cambiamento distinti (D-TL-1).

---

## 4. Componenti — nuovo file `Turn/RTTurnLog.h`

```cpp
UENUM(BlueprintType) enum class ERTLogCategory : uint8 { Move, Combat };

UENUM(BlueprintType) enum class ERTMoveOutcome : uint8 {
    Stayed,           // non pianificava movimento (path < 2 celle)
    Moved,            // raggiunta la destinazione pianificata (scambio incluso)
    BlockedContested, // fermata (o parziale) per destinazione contesa
    BlockedByUnit     // fermata (o parziale) per cella occupata da unità ferma
};

UENUM(BlueprintType) enum class ERTCombatOutcome : uint8 {
    Hit,              // danno inflitto agli HP
    ShieldAbsorbed,   // danno assorbito interamente dallo scudo (HP invariati)
    Lethal,           // bersaglio portato a HP <= 0
    NoLineOfSight,    // attacco pianificato scartato per LOS bloccata
    TerrainBonus      // colpo andato a segno con bonus altura (+danno), non letale
};

USTRUCT(BlueprintType)
struct FRTTurnLogEntry {
    GENERATED_BODY()
    ERTMatchPhase   Phase    = ERTMatchPhase::Move;
    ERTLogCategory  Category = ERTLogCategory::Move;
    uint8           Outcome  = 0;   // cast dell'enum secondo Category (intero — invariante #4)
    FRTGridCoord    SrcCell;        // chiave STABILE: cella di partenza dell'unità nel turno
    FRTGridCoord    TgtCell;        // bersaglio (Combat) o destinazione (Move); = SrcCell se n/a
    int32           Amount   = 0;   // danno effettivo (Combat) / n. celle percorse (Move)
};
```

Il TurnLog è un `TArray<FRTTurnLogEntry>`, membro di `ARTTurnManager`, con getter `GetTurnLog()`.

**Priorità Combat (enum a valore singolo, deterministica):**
`Lethal` > `ShieldAbsorbed` > `TerrainBonus` > `Hit`. (`NoLineOfSight` è un ramo distinto: attacco non applicato.)

### 4.1 Categoria `Facing` — orientamento *(aggiunta il 2026-08-09, CP 16.1)*

`ERTLogCategory` guadagna `Facing` **in coda**, con la stessa disciplina di `Fallback`, `Reaction` ed
`Environment`: la categoria viaggia come `uint8`, nessun campo nuovo entra nella voce, quindi **il formato
serializzato non cambia versione** e le tracce già scritte restano leggibili.

```cpp
UENUM(BlueprintType) enum class ERTFacingOutcome : uint8 {
    DerivedFromMove,                  // FacingFinalAfterMove: derivato dall'ultimo passo
    DerivedFromDash,                  // FacingAfterDash
    DeclaredInPlanning,               // rotazione dichiarata e accettata
    DeclarationRejected,              // dichiarata fuori dall'insieme legale: rifiutata
    TargetingReoriented,              // FacingAfterPrepActionTargeting: bersaglio prima di risolvere
    TurnedToDisplacementSource,       // spinta subita: girata verso la sorgente
    KeptOnEnvironmentalDisplacement,  // trascinamento senza sorgente: invariato
    UsedByBlast,                      // LETTURA: il colpo ha usato questo valore
    UsedByOverwatch                   // LETTURA: l'overwatch ha usato questo valore (E14)
};
```

**Perché scritture e letture stanno nello stesso enum.** [D-020](../decisions/RT_PDR_00_Decision_Log.md)
stabilisce che il facing cambia più volte per round e che ogni consumatore legge il valore autorevole più
recente. Un log di sole scritture direbbe *quando è cambiato* ma non *cosa ha usato il Blast*, e un round con
scatto e colpo nella stessa sequenza resterebbe ambiguo. La domanda a cui il replay deve rispondere è una
sola — *quale valore valeva quando* — e la risposta ha due forme: chi lo ha scritto e chi lo ha letto.

**`Amount` porta la direzione**, come valore di `ERTHexDirection` (0..5). È un intero, quindi rispetta
l'invariante #4 ed entra nell'hash del replay senza modifiche a `HashTurnLog`. La convenzione è la stessa già
in uso: `Amount` è il payload numerico della categoria — danno per `Combat`, celle percorse per `Move`.

**Un non-cambiamento non è un evento.** Riscrivere il facing che l'unità ha già non produce nessuna voce:
altrimenti l'hash del replay diventerebbe sensibile a scritture che non decidono niente. L'unica eccezione è
`DeclarationRejected`, che è osservabile **proprio perché** non cambia nulla — e registra la direzione
**conservata**, non quella chiesta: il log dice cosa vale, non cosa era stato domandato.

---

## 5. Classificazione (il cuore testabile)

- **Movimento** — estendere `FRTPathResult` con `ERTMoveOutcome Outcome` (default `Stayed`); `ResolvePaths` lo popola:
  registra il motivo del congelamento (contesa vs unità ferma) e, a fine loop, classifica
  `Stayed` (path<2) / `Moved` (Final == ultima cella) / `BlockedContested` / `BlockedByUnit`. Resta funzione pura.
- **Combat** — funzione pura `URTCombatLibrary::ClassifyCombatOutcome(HealthBefore, ShieldBefore, HealthAfter, DamageApplied, AttackerDmgBonus)`
  → `Lethal` / `ShieldAbsorbed` / `TerrainBonus` / `Hit` secondo la priorità sopra.
  `NoLineOfSight` è aggiunto da `ARTTurnManager` quando un attacco pianificato è scartato **solo** per LOS (separando
  la condizione LOS dal `continue` cumulativo, `RTTurnManager.cpp:612-615`).

`ARTTurnManager` fa da **collettore**: invoca le funzioni pure e accoda le entry.

---

## 6. Data flow & ordinamento

`LockInAndResolve` → i resolver girano come oggi → classificazione → `ARTTurnManager`: (1) accoda `FRTTurnLogEntry`,
(2) chiama `AddLogEvent` con la stringa **arricchita** dal reason (es. *«Guardian: fermo (cella contesa)»*,
*«Ranger → Bot: 45 (bonus di cella)»*, *«Ranger → Bot: nessuna linea di tiro»*). *(L'esempio diceva «altura
+danno»: la quota non dà danno, [D-024](../decisions/RT_PDR_00_Decision_Log.md).)*

**Ordinamento del TurnLog** (deterministico, invariante #3/§5.1): **fase → categoria → `SrcCell`** (StableTieBreak
per-coord). Non dipende **mai** dall'ordine d'inserimento nel container.

---

## 7. Determinismo & invarianti

- **Chiave unità = cella di partenza del turno** (max 1/cella ⇒ univoca), **mai** pointer/spawn-order.
- **Nessun float** nel log/ordinamento; `Outcome` intero (invariante #4).
- **Permutazione-invarianza** (cardine): permutare l'array di path / attacchi **non cambia** il TurnLog.
- Il TurnLog è **additivo**: i test esistenti restano verdi (estensioni con campi a default).

**Requisiti vincolanti (SMART):**
- **`FR-TURNLOG-01`** — ogni esito di Movimento/Combat produce **una** `FRTTurnLogEntry` col reason corretto (inclusi i
  non-eventi). *Verifica: test per ogni valore di enum.*
- **`FR-TURNLOG-02`** — TurnLog **permutazione-invariante**. *Verifica: permutare l'input → log identico.*
- **`FR-TURNLOG-03`** — nessun float nell'entry/ordinamento (invariante #4).
- **`FR-TURNLOG-04`** — combat log HUD mostra il reason per ogni evento, senza cambiare la logica (invariante #1).

---

## 8. Testing & Definition of Done

Test in `Tests/RTTurnLogTests.cpp` (movimento + permutazione) e `Tests/RTCombatLibraryTests.cpp` (classify combat).

- Movimento: 2 unità verso la stessa cella → **entrambe** `BlockedContested`; A↔B → `Moved` (scambio); destinazione
  dietro un'unità ferma → `BlockedByUnit`; path libero → `Moved`; path<2 → `Stayed`; permutare i path → log identico.
- Combat (`ClassifyCombatOutcome`): danno < scudo → `ShieldAbsorbed`; danno che intacca HP → `Hit`; HP a ≤0 → `Lethal`;
  bonus altura non letale → `TerrainBonus`; priorità `Lethal`>`ShieldAbsorbed`>`TerrainBonus`>`Hit`.

**DoD dello slice:** ☐ i test esistenti restano verdi ☐ nuovi test TurnLog/combat verdi (uno per enum + permutazione)
☐ build target Editor/Game **Succeeded** ☐ combat log HUD arricchito col reason (verifica PIE) ☐ nessun float
nell'entry/ordinamento ☐ `spec`/roadmap aggiornate ☐ nessun file generato/segreto committato.

---

## 9. File coinvolti

**Nuovi**: `Turn/RTTurnLog.h`, `Tests/RTTurnLogTests.cpp`, questa spec.
**Modificati**: `Turn/RTMovementResolver.h/.cpp` (`FRTPathResult.Outcome`), `Combat/RTCombatLibrary.h/.cpp`
(`ClassifyCombatOutcome`), `Turn/RTTurnManager.h/.cpp` (TurnLog + `GetTurnLog()` + `NoLineOfSight` + `AddLogEvent` arricchito),
`Tests/RTCombatLibraryTests.cpp`.

---

## 10. Rischi / note

- `NoLineOfSight`: richiede di separare la condizione LOS dal `continue` cumulativo (riga 612-615) per distinguere
  «scartato per LOS» da «scartato per range/abilità». Modifica circoscritta all'Actor (wiring), verificata in PIE.
- `TerrainBonus` è un modificatore su un colpo a segno → gestito con la **priorità** di §4 (non coesiste con Lethal/ShieldAbsorbed).
- Wiring in `ARTTurnManager` (Task 3): non unit-testabile in automation → verifica = **build + PIE**; la logica
  classificatrice è coperta dalle funzioni pure (Task 1/2).

---

## 11. Roadmap / slicing

| ID | Cosa | Verifica |
|----|------|----------|
| **TL.1** | `RTTurnLog.h` (enum + `FRTTurnLogEntry`) + `FRTPathResult.Outcome` popolato da `ResolvePaths` | test movimento (Stayed/Moved/BlockedContested/BlockedByUnit) + permutazione; test esistenti verdi |
| **TL.2** | `URTCombatLibrary::ClassifyCombatOutcome` (`Hit/ShieldAbsorbed/Lethal/TerrainBonus`, priorità) | test combat (uno per enum + priorità) |
| **TL.3** | `ARTTurnManager`: colleziona il TurnLog, aggiunge `NoLineOfSight`, arricchisce `AddLogEvent` | build + PIE: combat log mostra i reason |

Slice successivo — **serializzazione versionata** — ✅ **fatto** (`SR`, merge `8b6dc32`): vedi
[`spec-turnlog-serialize.md`](spec-turnlog-serialize.md). Chiude `Replay divergence = 0`.

---

## 12. Decisioni

**Prese (2026-08-03):**
- **D-TL-1** — TurnLog come struttura autoritativa separata (`FRTTurnLogEntry`), non estensione di `FRTResolvedEvent`.
- **D-TL-2** — chiave unità = cella di partenza del turno (permutazione-invariante).
- **D-TL-3** — outcome esposti da funzioni pure (resolver + `URTCombatLibrary`); l'Actor è collettore.
- **D-TL-4** — scope = Movimento + Combat; hash di replay e hazard/status rimandati.
- **D-TL-5** — reason codes **allineati al codice reale** (§2): Move {Stayed, Moved, BlockedContested, BlockedByUnit};
  Combat {Hit, ShieldAbsorbed, Lethal, NoLineOfSight, TerrainBonus}. Rimossi CoverReduced/BlockedByCover/OutOfBudget.

**Aperte (da confermare in PIE):** formato esatto delle stringhe arricchite del combat log (presentazione, tunabile).

---

## 13. Riferimenti

- Canone: [`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) — invarianti #1/#3/#4/#7, §5.1 (APNAP/tie-break), §6.
- Roadmap: [`roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md) — risk register, KPI «Replay divergence = 0».
- Codice: `RTTurnManager.h/.cpp`, `RTMovementResolver.h/.cpp`, `RTCombatResolver.h/.cpp`, `RTCombatLibrary.h/.cpp`, `RTResolvedEvent.h`, `RTUnit.h`, `RTPlayerController.cpp`.
- Playback (struttura sorella, non riusata): [`spec-anima-risoluzione.md`](../gameplay/spec-anima-risoluzione.md).
