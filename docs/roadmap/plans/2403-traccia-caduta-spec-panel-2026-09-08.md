# Spec panel — #2403 «Una caduta non lascia una catena causale», 2026-09-08

> `REFERTO` · **Oggetto**: [#2403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2403)
> **Modalità**: `critique` · **Focus**: requirements · architecture · testing
> **Misure**: su `main` = **`5327d514`**.
> **Esito**: 1 criterio **non soddisfacibile** riformulato, 1 decisione
> ([`D-354`](../../decisions/RT_PDR_00_Decision_Log.md)), 3 correzioni minori.
> **Precedente**: [`2388-caduta-spec-panel-2026-09-08.md`](2388-caduta-spec-panel-2026-09-08.md), che ha
> deciso `D-352` e `D-353`.

---

## 1. C1 — il seek al boundary non esiste per la caduta, e non deve esistere

Il criterio *«il seek al confine di micro-step coincide con il playback»* e il test
`Fall.SeekMatchesPlaybackAtBoundary` **non sono soddisfacibili**, e non per una svista della issue: per una
scelta architetturale dichiarata altrove.

| Fatto | Sede |
|---|---|
| `ApplyDisplacements` è chiamato nella coda della fase di attacco, dopo il danno | `RTTurnManager.cpp:6104` |
| Il ciclo dei micro-step è altrove: `= 0`, `++`, `ON_SCOPE_EXIT → INDEX_NONE` | `RTTurnManager.cpp:7300-7322` |
| Fuori da quel ciclo il campo vale `INDEX_NONE` | `RTTurnManager.h:1981` |
| Ogni voce lo copia: *«`INDEX_NONE` significa "nessun ciclo qui" — non "non lo so"»* | `RTTurnManager.cpp:2995` |
| `SeekToBoundary` rifiuta `MicroStepIndex < 0` **senza scandire la traccia**, e nomina *«Blast, status, hazard»* fra i casi che non sono un gruppo simultaneo | `RTReplaySeekLibrary.h:131` |

**GREGOR HOHPE**: *«Il test chiederebbe a un indirizzo di risolvere una coordinata che quel messaggio
deliberatamente non porta. E la via d'uscita ovvia — popolare `MicroStepIndex` nelle voci di Blast — è
esattamente l'errore che `SeekToBoundary` esiste per rifiutare: quelle voci condividono un valore, non una
barriera che le abbia decise insieme.»*

✅ **Tutte le cadute nascono nel Blast**: `Fell` è scritto in due soli punti,
`RTTurnManager_Blast.cpp:2435` e `:2521`, entrambi dentro `ApplyDisplacements`. Non esiste oggi un percorso
di caduta dentro un micro-step, quindi il criterio non è *«prematuro»*: è mal posto.

📝 **Riformulato**: il seek pertinente è `SeekToPhase`. Test → `Fall.SeekToPhaseFindsTheFallEntry`.

---

## 2. C2 — il parapetto, deciso `(a)` dall'autore → `D-354`

Il criterio *«dal log si distingue una caduta da una spinta terminata su un ostacolo»* nominava **due** casi
mentre ne esistono **tre**, e il terzo è la novità di #2401.

| Caso | Prima | Dopo `D-354` |
|---|---|---|
| caduta | `Fell` | i quattro valori di `D-352` |
| **parapetto**, con spostamento | `Displaced` — come un muro | `StoppedByEdgeGuard` |
| **parapetto adiacente**, senza spostamento | `DisplacementResisted` + `NoDestination` — come un muro | `DisplacementResisted` + `EdgeGuard` |
| muro · unità · bordo mappa | invariati | invariati |

**KARL WIEGERS**: *«Il parapetto è ciò che #2401 ha inventato — la qualifica autorata che nega una caduta
altrimenti implicita — e nel log era indistinguibile da un muro. `ERTDisplacementBlockReason` non aiutava:
viaggia solo con `DisplacementResisted`, cioè quando lo spostamento non ha spostato nessuno.»*

🔑 **Il codice sapeva già che i casi erano tre.** Il commento al punto 3 di `RisolviCadutaSeBordoAperto` dice
*«Muro, unità e parapetto rispondono `false`, ed è la distinzione che `StepUntilBlocked` da solo non può
dare»*. Mancava un posto dove scriverla, non la conoscenza.

⛔ **`Guarded` non si riusa**: significa *«`Action.Guard` ha retto»* — una **decisione dell'unità** — mentre
il parapetto è **geometria della mappa**. L'enum separa già le due famiglie di proposito.

### 2.1 Una nota sul follow-up respinto due panel fa

Il secondo giro aveva **respinto** l'overload di `HexKnockbackDestination` che restituiva la ragione
dell'arresto, perché *«il consumatore è uno»*. `D-354` crea il **secondo**. Il rigetto regge lo stesso, e per
la ragione che lo motivava: `FRTHexCellData::HasGuardOn(Edge)` esiste già (`RTHexCellData.h:463`) e la
direzione è già ricostruita da `DirezioneSpostamentoForzato` — la ragione si chiede **a valle**, senza
toccare una firma con molti chiamanti.

⚠️ Ciò che cambia è la **via di rientro**: con due consumatori, promuovere `DirezioneSpostamentoForzato` da
funzione anonima a membro di libreria diventa più probabile. Resta fuori scope finché il terzo non esiste.

---

## 3. Correzioni minori

| # | Rilievo | Correzione |
|---|---|---|
| M1 | il *«Costo da dichiarare»* dice formato **12** | è **13** (`WithSightBlocker`, 2026-09-06, #2534). La conclusione non cambia |
| M2 | *«il replay riproduce l'esito risolto, senza ricalcolare»* e *«serializzazione deterministica»* si sovrappongono | il primo diventa falsificabile come *«una traccia caricata **senza la mappa** dice quale esito ha avuto»*: se serve la mappa, è ricalcolato |
| m1 | i tre test nominati non asseriscono **quale valore in quale caso** | i quattro test esistenti asseriscono la **posizione**; servono le asserzioni d'esito, una per valore di `D-352` più due per `D-354` |

---

## 4. Verificato, e **non** è un problema

**La privacy.** Nominare celle di atterraggio non apre il canale che #2534 ha appena chiuso: la voce porta
già `Cell`, `SrcCell` e `TgtCell` per **ogni** spostamento, e la caduta non introduce un campo nuovo. Il
precedente filtrato — `SightBlockerCell` con `URTTurnLogLibrary::SightBlockerForLog`, provato da
`CombatLog.SightBlockerRespectsTeamKnowledge` — riguarda una cella **non posseduta da un'unità**, che è un
caso diverso. Nessun requisito nuovo per #2403.

---

## 5. Il lavoro, dopo il panel

1. **`Turn/RTTurnLog.h`** — cinque valori in coda: tre a `ERTMoveOutcome` da `D-352`
   (`FellToAlternative`, `FellToLastStable`, `FellWithoutLanding`), uno da `D-354`
   (`StoppedByEdgeGuard`), e `EdgeGuard` in coda a `ERTDisplacementBlockReason`.
2. **`Turn/RTTurnManager_Blast.cpp`** — `RisolviAtterraggio` sa già quale dei tre esiti ha preso: deve
   dirlo. Il parapetto si chiede con `HasGuardOn` sulla direzione già ricostruita.
3. **Test** — `Fall.TracePreservesCauseChain`, `Fall.ReplayMatchesResolvedOutcome`,
   `Fall.SeekToPhaseFindsTheFallEntry`, più le asserzioni d'esito per i cinque valori.

✅ **Nessun bump di formato**: tutto in coda. Il corpus golden non si rigenera.
