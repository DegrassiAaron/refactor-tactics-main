# Work order — `FirstHitDelta`: una provenienza che nomina la decisione uscita, e un bonus che non ne ha nessuna

Ingresso della wave `first-hit-provenance/1`. È il file che `RT3_CONTRACT.md` §4 richiede come `INPUT_HANDOFF`: un artefatto rileggibile, non testo incollato.

Questa wave parte come **audit + characterization**. Il perimetro non è una funzione: è il **confine in cui il breakdown comincia a registrare**.

## Origine

**Nessuna issue OPEN copre questa wave.** Misurato sui corpi di 981 issue (376 OPEN, 605 CLOSED) scaricati e cercati in locale, non con `--search`, che legge un indice asincrono. La issue va creata come primo passo della wave.

Misurato su `origin/main` = `9da33c59`, albero pulito.

Correlate, tutte CLOSED: [#1951](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1951) (nascita di `SourceId` e del breakdown) · [#2213](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2213) (**stessa etichetta hardcoded**, in `ApplyAbsorptionPool`) · [#2207](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2207) (deriva dei commenti su `ApplyFirstHitDelta`) · [#137](https://github.com/DegrassiAaron/refactor-tactics-main/issues/137) (`Status.Marked` non cablato).

Decisioni: `D-292`, `D-309`, `D-312`, `D-212` — [`RT_PDR_00_Decision_Log.md`](../../../decisions/RT_PDR_00_Decision_Log.md).

## Difetto

Tre fatti distinti, e nessuno è quello che il nome della funzione suggerisce.

**(1) La label è hardcoded e nomina la decisione che è USCITA dalla funzione.**

```cpp
// Source/RefactorTactics/Combat/RTCombatResolver.cpp:95
Attack.Breakdown.Emplace(ERTDamageStage::FirstHitDelta,
    FName(TEXT("D-292 · primo colpo")),
    Delta >= 0 ? ERTDamageOp::Add : ERTDamageOp::SubtractClamped, ...
```

`D-292` è la decisione della **Guardia**, e da `D-292` la Guardia non passa più di lì: è un pool, consumato da `ApplyAbsorptionPool` (`RTCombatLibrary.h:98`, `RTTurnManager.cpp:5367`). L'etichetta non è «storica e generica»: nomina l'unica sorgente che ha lasciato quella funzione.

**(2) L'unico produttore rimasto è `Status.Exposed`.**

```cpp
// Source/RefactorTactics/Turn/RTTurnManager.cpp:5371-5380 — unico += dell'array
TArray<int32> FirstHitDelta;
FirstHitDelta.Init(0, Units.Num());
for (int32 i = 0; i < Units.Num(); ++i)
{
    if (Units[i]->HasStatus(TAG_Status_Exposed))
    {
        FirstHitDelta[i] += URTCombatLibrary::ExposedFirstHitBonus;
    }
}
```

**(3) `Status.Marked` è DICHIARATO appartenere a quello stadio, e non ci passa.**

L'enum lo afferma — `RTCombatResolver.h:54`: *«Delta di primo colpo per bersaglio: `Status.Exposed` +5, `Status.Marked` +6. Vale una volta»*. L'implementazione fa altro:

```cpp
// Source/RefactorTactics/Turn/RTTurnManager.cpp:5306 — su Plan.Hits, PRIMA di ToAttacks
Hit.Power += URTCombatLibrary::MarkedFirstHitBonus;
```

Il `+6` entra nel danno **senza produrre alcuna voce di breakdown**: `ToAttacks` inizializza il registro da `Hit.NominalPower` e `Hit.CoverReduction` (`RTHexCombatLibrary.cpp:759-770`), e il bonus è già dentro `Hit.Power`.

Perché conta: per #1951 il breakdown è *«l'unico posto in cui un lettore del TurnLog scopre da dove viene un numero»*. Quel numero non ha una provenienza — e a differenza degli stadi 2 e 3 nessuno ha dichiarato che non ce l'abbia.

## ⛔ Falsi punti di partenza

Una stesura precedente di questo lavoro ordinava di caratterizzare `Exposed only`, `Marked only`, `Exposed + Marked` **attraverso `ApplyFirstHitDelta`**. Due di quei tre casi non sono raggiungibili da quella funzione: chi li scrive produce test verdi che non discriminano nulla, perché la condizione da distinguere non esiste sul banco.

Non è vero, ed è misurato, che:

- ⛔ `ApplyFirstHitDelta` riceva oggi contributi da più sorgenti — ne riceve **uno**;
- ⛔ `Status.Marked` passi dal resolver — muta `Plan.Hits` a monte;
- ⛔ la forma `(..., FName SourceId)` sia da evitare per principio — è la forma **ratificata** da #2213 per lo stesso difetto, una funzione più in là.

## Inventario misurato — stadi contro produttori

| `ERTDamageStage` | produttore runtime | dove |
|---|---|---|
| `Catalog` | ✅ | `RTHexCombatLibrary.cpp:764` |
| `AttackerCell` | ❌ **nessuno** | assorbito in `NominalPower` |
| `ConditionalBonus` | ❌ **nessuno** | `Wet` × gadget: `RTTurnManager.cpp:5359` |
| `Cover` | ✅ | `RTHexCombatLibrary.cpp:768` |
| `FirstHitDelta` | ⚠️ **parziale** — solo `Exposed`; label errata | `RTCombatResolver.cpp:95` |
| `EveryHitDelta` | ✅ | `RTCombatResolver.cpp:122` |
| `AbsorptionPool` | ✅ `SourceId` parametrico | `RTCombatResolver.cpp:163` |
| `TargetSum` | ✅ | `RTCombatResolver.cpp:61` |
| `ShieldAbsorption` | ✅ | `RTCombatResolver.cpp:69` |

🔑 **La distinzione che governa lo scope.** `AttackerCell` e `ConditionalBonus` sono lacune **dichiarate**: `RTHexCombatLibrary.h:231-233` scrive *«Quei due stadi non sono registrati da questa slice, ed è dichiarato invece che dedotto»*. `Marked` è la lacuna **non dichiarata** — l'unica in cui il documento afferma il contrario del codice. Solo quest'ultima è `CURRENT REQUIRED`.

## Contratto richiesto

Una voce di breakdown nomina la sorgente che l'ha prodotta, non una costante scritta nel punto che la emette. E un modificatore che cambia il danno finale ha una voce, oppure una dichiarazione scritta che spiega perché non ce l'ha.

## Cosa misurare prima di scrivere

1. se il `+6` di `Marked` sia visibile nel breakdown per qualche percorso (atteso: no);
2. se, con `CoverReduction > 0`, la voce `Cover` assorba silenziosamente il `+6` nel proprio `After`;
3. se `RefactorTactics.Damage.BreakdownFinalValueMatchesDamageDealt` (`RTDamageBreakdownTests.cpp:41`) possa vederlo — costruisce gli attacchi in modo sintetico e **non attraversa il Blast**: è la ragione per cui il difetto è sopravvissuto a #1951 e #2213;
4. se `Exposed` e `Marked` possano coesistere sullo stesso bersaglio nello stesso boundary, e in quale ordine si compongano oggi (`Marked` a `:5306`, `Exposed` a `:5378` — **Marked prima**);
5. **se `Marked` possa entrare ORA nella catena strumentata**: la giustificazione scritta per tenerlo fuori — `RTTurnManager.cpp:~5321`, *«`FRTAttack` conserva solo il bersaglio»* — è superata da `D-212`, che ha aggiunto `AttackerIndex` (`RTCombatResolver.h:177`).

Il punto 5 è il perno: se regge, la wave ha una soluzione; se non regge, ha un `BLOCKED` motivato.

## Uscite possibili

- **(A)** `ApplyFirstHitDelta(..., FName SourceId)` + costante `ExposedFirstHitSource` accanto a `GuardPoolSource` e `ReactionReductionPoolSource` (`RTCombatLibrary.h:203-204`). Riusa esattamente la forma di #2213. Basta **finché** il produttore è uno solo — e oggi lo è.
- **(B)** `Marked` migra dentro la catena strumentata usando `AttackerIndex`, e diventa un secondo produttore di `FirstHitDelta`: allora (A) non basta più e serve una voce per contributo, come `AbsorptionPool` già ammette.
- **(C)** `Marked` resta dove sta e prende uno stadio proprio, dichiarato nell'enum.

⛔ Non implementare (B) o (C) perché «più SOLID». (A) è l'unica realizzabile senza una decisione d'autore.

## Condizione di blocco

Se l'audit conferma che il `+6` è invisibile al breakdown, la scelta fra (A) con `DEFERRED`, (B) e (C) **cambia ciò che un lettore del TurnLog vede**, cioè il contratto di #1951:

```text
STATUS: BLOCKED
REASON: design decision missing — provenienza di Status.Marked nel breakdown
UNBLOCK: decisione owner (#1951 / #2213) fra (A) (B) (C)
```

Aggiorna la issue con le tre uscite e i costi misurati. Non inventare la policy.

## Scope

- la provenienza della voce `FirstHitDelta` smette di essere una costante scritta nel resolver;
- l'audit dei tre modificatori che mutano `Plan.Hits` prima di `ToAttacks`, con esito scritto;
- i test di caratterizzazione che oggi non esistono sul percorso Blast.

## Fuori scope

- ⛔ `AttackerCell` e `ConditionalBonus`: lacune già dichiarate, `DEFERRED`, nominate e non toccate;
- ⛔ qualunque cambiamento ai numeri — `Exposed` +5 e `Marked` +6 restano quelli che sono;
- ⛔ `IDamageModifier`, gerarchie Strategy, framework generico di modificatori;
- ⛔ riscrittura di `ARTTurnManager`;
- ⛔ l'ordine dei pool (`D-312`) e l'eleggibilità frontale (`D-206`).

## Acceptance criteria

1. La voce `FirstHitDelta` nomina la sorgente che l'ha prodotta; nessun `FName(TEXT(...))` di provenienza resta nel corpo di `ApplyFirstHitDelta`.
2. Esiste un Automation Test che lo verifica **leggendo il `SourceId`**, non solo il valore.
3. Anti-vacuità: rimettendo la costante hardcoded quel test diventa rosso, verificato per mutazione.
4. Esiste un test che percorre il **Blast** (`Plan.Hits` → `ToAttacks` → resolver) e confronta l'ultimo `After` del breakdown col danno inflitto. Senza questo, i primi due restano nel percorso sintetico che già oggi non vede il problema.
5. L'esito dell'audit su `Marked` è scritto: o una voce di breakdown, o una dichiarazione nel codice che spiega perché non ce n'è una — come `RTHexCombatLibrary.h:231-233` fa per gli stadi 2 e 3.
6. Il danno finale è invariato: nessuno scenario del corpus cambia numero.

## Test da scrivere (DEV scrive, VALIDATION esegue)

| nome | proprietà provata | atteso oggi |
|---|---|---|
| `RefactorTactics.Damage.BreakdownFirstHitNamesItsOwnSource` | la voce `FirstHitDelta` nomina la sorgente che l'ha prodotta | **rosso** — la label è `D-292 · primo colpo` |
| `RefactorTactics.Damage.BreakdownSurvivesTheBlastPath` | l'ultimo `After` coincide col danno inflitto **passando dal Blast** | anti-vacuità: è la lacuna che ha lasciato passare il difetto |
| `RefactorTactics.Damage.MarkedBonusAppearsInBreakdown` | il `+6` di `Marked` ha una voce che lo spiega | **rosso** — da scrivere solo se la decisione di blocco sceglie (B) o (C) |

Comando: `./scripts/rt-suite.ps1 -Filter RefactorTactics.Damage` — `NOT RUN — dominio VALIDATION`.

## Rischio dichiarato

`ApplyFirstHitDelta` è `UFUNCTION(BlueprintPure)` (`RTCombatResolver.h:244`): cambiarne la firma è una modifica di **API Blueprint**.

🔴 **Questa riga diceva che DEV non può misurare chi la chiami dagli asset, ed era falsa.** Corretta il 2026-09-06: la calibrazione che l'aveva prodotta non era omologa al bersaglio — cercava nomi di package e di classe, che vivono nella *import table*, per concludere su nomi di funzione **invocata**, che vivono come stringhe `CallFunc_<Nome>_*` prodotte dal compilatore Blueprint.

Con un controllo positivo della specie giusta la misura si fa **senza aprire l'Editor**: 67 `UFUNCTION` su 455 compaiono in almeno un asset, 111 occorrenze di `CallFunc_*` su 123 `.uasset` tracciati. Evidenza: `waves/counter-attack-record/evidence/RT3-EDITOR-reflection-0eeb5c1.md` §0.

⚠️ La misura va comunque **fatta**, non assunta: finché nessuno l'ha eseguita per `ApplyFirstHitDelta`, l'esito è `NOT MEASURED` — che non è «nessuno lo usa».

Il breakdown **non** entra in hash, salvataggio o replay (`RTCombatResolver.h:186-187`, misurato). Il **danno** sì: qualunque spostamento di `Marked` dentro la catena va provato invariante sul valore finale.

## Downstream in scope (RT3 §8)

`TURNLOG/REPLAY` — il breakdown è ciò che il TurnLog espone · `DETERMINISM` — l'ordine delle voci è parte del contratto (`RTCombatResolver.h:62-68`) · `AUTOMATION/SCENARIO` — `RTDamageBreakdownTests.cpp`, `Scenarios/Spec/Combat/`, `Scenarios/Spec/Reaction/`.

## Commit candidate

```text
refactor(combat): name the first-hit delta source instead of hardcoding a decision
```
