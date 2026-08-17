# RefactorTactics — Handoff per Claude
## Consolidamento Tactical Designer v0.1 · Map Editor + Scenario Composer

> `HISTORICAL` · **Materiale NON autorevole**, archiviato il **2026-08-17** dopo il consolidamento.
> **Non si applica**: si legge per sapere da dove viene una decisione. Le fonti autorevoli sono
> [`spec-tactical-designer.md`](../../technical/spec-tactical-designer.md) — l'owner del concetto — e
> [`feature-registry.yaml`](../../roadmap/feature-registry.yaml).
>
> **Recepito in due tempi, e vale la pena dirlo perché non è la forma solita.** La parte concettuale —
> owner documentale, `D-154`, epic [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105),
> feature `RT-FEAT-TOOL-SCENARIO-COMPOSER` e `RT-FEAT-TOOL-SKILL-WORKBENCH`, checkpoint `M9.4`, seduta
> `U26` — era già atterrata con [#1108](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1108),
> che consumava un **sorgente diverso e adiacente** (`TacticalDesigner_03`). Quando questo handoff è stato
> aperto, il suo §5.2 chiedeva una feature che esisteva da mezz'ora.
>
> **Il residuo era il suo §6, ed è entrato**: le quattro slice `SC-1`…`SC-4` diventano
> [#1114](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1114) ·
> [#1115](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1115) ·
> [#1116](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1116) ·
> [#1117](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1117), sub-issue di #1105. La
> feature aveva `issues: []`, cioè un owner senza lavoro sotto.
>
> ✅ **Il suo §4 chiedeva di riconfermare il gap su HEAD prima di creare la issue, e la riconferma ha
> retto**: `URTScenarioLoader` espone `LoadFromString`, `LoadFromFile`, `Validate` e **nessun writer**;
> l'unico `ToJson` del modulo serializza `FRTTestResult`, non lo scenario. `SC-1` non era un duplicato.
>
> ⚠️ **Ciò che non è entrato**: il §10 (parallelizzazione) proponeva due lane e un elenco di file da non
> toccare in contemporanea — è governato da [`parallel-batch.yaml`](../../roadmap/parallel-batch.yaml), che
> lo fa con un write-set misurabile invece che con una raccomandazione. Il §5.1 e il §13 (scope guard)
> descrivono correttamente il repository e non avevano niente da applicare.

**Data:** 2026-08-16  
**Repository:** `DegrassiAaron/refactor-tactics-main`  
**HEAD visto durante questo handoff:** `a9b674b3631bc0146f3b9de08feced954547a8d8`  
**Scopo:** integrare nel repository le decisioni sul Tactical Designer v0.1, aggiornare tracking e documentazione, riusare le issue esistenti, creare solo le issue realmente mancanti e impedire la nascita di sistemi paralleli.

> Questo file è un **handoff operativo**, non una nuova fonte di verità permanente. Prima di modificare qualunque cosa, ri-auditare `origin/main`, issue, PR, Decision Log, Feature Registry e write-set correnti. Dopo il consolidamento, archiviare questo handoff secondo le convenzioni del repository.

---

# 0. Obiettivo

Vogliamo consolidare una prima maturity del **Tactical Designer** che unisca in modo coerente:

```text
Hex Map Editor
    +
runtime probes
    +
Scenario Composer Lite
    +
Scenario Harness reale
    +
TurnLog / result
```

Il Tactical Designer è una **suite concettuale di strumenti**, non un nuovo runtime e non un secondo simulatore.

Principio architetturale:

```text
canonical runtime/data
      ↓
pure query / canonical DTO
      ↓
editor tooling / visualization / authoring
```

Vietato:

```text
Editor-only rule
      ↓
duplicazione di pathfinding / targeting / validation / resolution
```

Il risultato finale deve permettere a un designer di compiere questo loop:

```text
MAP
 ↓
SCENARIO INITIAL STATE
 ↓
TURN / INTENT
 ↓
EXPECTATION
 ↓
RUN
 ↓
TURNLOG / RESULT
 ↓
RESET / MODIFY / RUN AGAIN
```

con lo **stesso dato canonico** che può essere eseguito anche headless.

---

# 1. Prima di scrivere: audit obbligatorio

Eseguire nell'ordine:

1. aggiornare e misurare `origin/main`;
2. annotare HEAD;
3. cercare PR aperte relative a:
   - `#711`
   - `#622`
   - `#712`
   - `#623`
   - Scenario Composer
   - Scenario Writer / serializer
   - Scenario Unit Placement
   - Scenario Turn Editor
   - Run/Reset/TurnLog editor;
4. controllare `docs/OPEN_DECISIONS.md` e Decision Log;
5. controllare `docs/roadmap/parallel-batch.yaml` e qualunque meccanismo di ownership/write-set corrente;
6. verificare Feature Registry e viste generate;
7. verificare se, dal momento di questo handoff, Claude o un'altra sessione abbia già creato issue equivalenti.

**Non creare una issue prima di questa ricerca.**

Nel momento della preparazione di questo handoff, la ricerca GitHub non trovava issue aperte chiamate Scenario Composer / Scenario Writer / unit placement equivalenti alle quattro slice proposte sotto.

---

# 2. Stato consolidato del Map Editor

## #711 — Movement Probe

**Esiste già ed è OPEN. Non creare un duplicato.**

È la slice corretta per:

- start cell;
- profilo/unità reale;
- budget reale;
- `URTHexSimLibrary::ReachableCells`;
- percorso ricostruito tramite `FRTHexReachableCell::FromCell`;
- costo;
- reason code runtime per celle escluse;
- refresh quando cambia la revisione della mappa.

### Azione richiesta

- verificare se esiste una PR nuova;
- se no, mantenere #711 come issue proprietaria;
- collegarla esplicitamente alla maturity Tactical Designer / Map Editor;
- aggiornare corpo/commento **solo se lo stato reale è cambiato**;
- non introdurre un secondo A*/Dijkstra nell'Editor;
- non creare reason code Editor-only.

### Tracking

Assicurarsi che #711 sia rappresentata correttamente in:

- Feature Registry dell'Hex Map Editor / tooling;
- roadmap o vista operativa che possiede il Map Editor;
- `editor-sessions.yaml` quando entra la verifica visuale;
- `test-manuali-pie.md` per la voce PIE prevista.

---

## #622 — Workspace Grid

**Esiste già ed è OPEN. Non creare un duplicato.**

Serve a mostrare:

> dove il designer potrebbe creare celle, senza far credere che quelle celle esistano già.

Vincoli:

- ghost/transient;
- distinguibile dalle celle vere;
- bounded;
- nessun Actor per cella;
- niente dati nel `.umap`;
- nessuna collisione/picking ambiguo.

### Azione richiesta

- verificare PR/stato corrente;
- mantenere #622 come owner;
- collegare la issue alla maturity Tactical Designer;
- evitare una seconda grid authority;
- registrare la verifica manuale nella seduta editor appropriata.

---

## #712 — Geometry authoring gesture

**Esiste già ed è OPEN, ma gran parte del codice è già stata mergiata.**

Stato consolidato noto:

- `URTHexGeometryTool` esiste;
- drag authoring esiste;
- `SnapToGrammar` vive nel runtime;
- validator e bake vengono riusati;
- PR #768 ha consegnato il codice principale;
- restano verifiche visuali/manuali: ghost, snap UX, Undo atomico, residui.

### Azione richiesta

Non riscrivere il Geometry Tool.

Verificare:

- stato reale delle voci U22;
- eventuale dipendenza ancora valida da #623;
- se le prove manuali sono state eseguite;
- se tutte le DoD sono realmente chiuse.

**Chiudere #712 solo quando le verifiche residue sono registrate come eseguite**, non perché il codice compila.

Se il corpo della issue contiene fatti superati, aggiornarlo o aggiungere un commento di consolidamento con stato misurato e commit/PR.

---

## #623 — Lighting + frame whole map

**Esiste ed è OPEN.**

È importante perché la leggibilità della sandbox condiziona la valutazione di ghost, snap e geometria.

### Azione richiesta

- verificare se `Home -> frame whole map` esiste già;
- verificare se `L_DevSandbox` è stata aggiornata/committata;
- verificare la seduta manuale;
- se #623 blocca ancora U22, esplicitarlo nel tracking;
- non toccare `RTCameraPawn` per risolvere la camera dell'Editor.

---

# 3. Stato consolidato dello Scenario System

Esistono già e sono da riusare:

- `FRTTestScenario`;
- `FRTScenarioUnit`;
- `FRTScenarioTurn`;
- `FRTScenarioIntent`;
- `FRTTestExpectation`;
- varianti;
- decisioni;
- `URTScenarioLoader`;
- `URTScenarioIndex`;
- `URTScenarioRunner`;
- `FRTScenarioSession`;
- `FRTTestResult`;
- `Scenarios/**/*.json`;
- TurnLog;
- report/result;
- Scenario Harness.

Il runner deve continuare a entrare dal percorso reale:

```text
scenario intent
  -> planning/input path reale
  -> LockInAndResolve
  -> resolver reale
  -> TurnLog
```

Vietati:

- `SetActorLocation` come scorciatoia;
- danni applicati manualmente dal Composer;
- resolver alternativo;
- branch competitivo `if Editor`;
- secondo schema scenario.

## #209 — Scenario Index

**CHIUSA e da riusare. Non riaprire.**

Ha già separato:

```text
ScenarioId = identità
Path       = storage
Tags       = discovery
```

Il Composer deve consumare questo modello.

---

# 4. Gap reale: Scenario Composer visuale

L'audit precedente ha trovato un gap reale:

- nessun Scenario Composer visuale nel modulo Editor;
- nessun percorso canonico chiaramente identificato per `FRTTestScenario -> JSON` equivalente al loader.

**Prima di creare issue, riconfermare il secondo punto su HEAD.**

Se un writer è comparso nel frattempo, SC-1 va eliminata o trasformata nel solo gap residuo.

---

# 5. Tracking: cosa aggiornare e cosa NON creare

## 5.1 Non creare una nuova “mega feature Tactical Designer”

Il Tactical Designer è una vista/maturity trasversale.

Non deve diventare una seconda fonte che duplica:

- Map Editor;
- Scenario Harness;
- TurnLog;
- Scenario data.

Usare gli owner già esistenti.

## 5.2 Nuova feature solo se manca davvero

Se non esiste una voce equivalente per il Composer, creare una feature dedicata, ad esempio:

`RT-FEAT-TOOL-SCENARIO-COMPOSER`

Nome indicativo; seguire naming reale del repository.

La feature deve essere chiaramente **developer tooling**, non una UI player-facing.

Se il registry supporta `out_of_release_scope`, usarlo per chiarire che:

> “Tactical Designer v0.1” è una maturity degli strumenti; non trasforma automaticamente lo Scenario Composer in un gate della release giocabile v0.1.

Non creare `RT-FEAT-TACTICAL-DESIGNER-RUNTIME`.

## 5.3 Fonti di tracking da usare

Aggiornare solo quelle pertinenti:

- `docs/roadmap/feature-registry.yaml` — owner stato feature;
- `docs/roadmap/roadmap-v0.1.md` — solo se il lavoro è realmente parte della release/maturity tracciata lì;
- `docs/roadmap/editor-sessions.yaml` — prove in Editor;
- `docs/technical/test-manuali-pie.md` — definizione delle prove visuali/manuali;
- issue GitHub — unità di lavoro;
- Scenario JSON — acceptance eseguibile;
- Decision Log — solo se emerge una nuova decisione reale;
- `execution-graph.yaml` / `parallel-batch.yaml` — solo se esiste una dipendenza o un lavoro parallelo reale.

Le viste `*.shortlist.md` generate **non si modificano a mano**: rigenerarle con lo script ufficiale.

Non introdurre un nuovo foglio di stato manuale del Tactical Designer.

---

# 6. Nuove issue da creare SOLO se ancora mancanti

Preferire una feature dedicata + 4 issue piccole rispetto a una epic gigantesca.

Creare una epic parent solo se la convenzione corrente del repository lo richiede per comparire nelle viste di roadmap. In quel caso l'epic è tracking, non un quinto lavoro tecnico.

---

## SC-1 — Canonical Scenario Writer

### Titolo suggerito

`Scenario authoring: writer JSON canonico e round-trip di FRTTestScenario`

### Why

Il Composer non deve conoscere il formato JSON.

Serve un seam runtime/tooling:

```text
FRTTestScenario
   -> Validate
   -> canonical JSON
   -> explicit save
```

### Scope

- writer canonico fuori da `RefactorTacticsEditor`;
- output stabile;
- preservazione `ScenarioId`;
- preservazione tags/header dell'indice;
- preservazione Stable IDs;
- errore leggibile;
- save esplicito;
- round-trip.

### DoD

- [ ] `load -> write -> load` è semanticamente equivalente
- [ ] cambiare path non cambia `ScenarioId`
- [ ] tags preservati
- [ ] Stable Unit IDs preservati
- [ ] scenario invalido rifiutato prima della scrittura
- [ ] due write consecutive producono forma canonica stabile
- [ ] zero dipendenza dal modulo Editor
- [ ] mutation test su almeno un campo significativo

### Out of scope

- UI;
- nuovo formato;
- `.uasset`;
- visual scripting;
- player browser.

---

## SC-2 — Scenario Composer Lite: Initial State / Unit Placement

### Titolo suggerito

`Scenario Composer Lite: initial state e piazzamento unità nel Hex Map viewport`

### Scope

- aprire/creare un scenario canonico;
- scegliere fixture/map source già supportata;
- mostrare unità;
- `+ Unit`;
- `HeroId`;
- `TeamId`;
- click cella;
- facing iniziale;
- move/delete unit;
- save tramite SC-1;
- reload senza perdita ID.

### DoD

- [ ] scenario esistente visualizza le unità sulle celle corrette
- [ ] click cella produce `FRTScenarioUnit::Cell`
- [ ] spostare l'unità modifica il dato canonico
- [ ] facing usa enum/runtime vocabulary esistente
- [ ] cella invalida / ID duplicato produce errore leggibile
- [ ] Save/Reload conserva lo scenario semanticamente
- [ ] Actor visuale non è authority
- [ ] nessuna regola duplicata nell'Editor

---

## SC-3 — Scenario Composer Lite: Turn Move/Wait + Expectations

### Titolo suggerito

`Scenario Composer Lite: authoring Turn Move/Wait ed expectations canoniche`

### Thin slice

Turn:

```text
Turn 01
  A1 -> Move -> Pick Cell
  B1 -> Wait
```

Expectation minima:

- `UnitAtCell`
- `LogEventCount`

Espandere alle altre assertion già supportate solo se il lavoro resta piccolo e usa lo stesso modello.

### DoD

- [ ] creare Turn 1
- [ ] assegnare Move/Wait
- [ ] scegliere target cell dal viewport
- [ ] preview Move usa servizio runtime
- [ ] aggiungere expectation canonica
- [ ] `URTScenarioLoader::Validate` passa sullo scenario salvato
- [ ] save/reload preserva Turn/Intent/Expect
- [ ] capability mancante usa Error/Blocked esistente, non fallback silenzioso

---

## SC-4 — Scenario Composer Lite: Run / Reset / Result / TurnLog

### Titolo suggerito

`Scenario Composer Lite: Run, Reset, Result e TurnLog tramite Scenario Harness reale`

### Scope

```text
Validate
 -> clean session/world
 -> existing Scenario Runner
 -> gameplay path reale
 -> TurnLog
 -> FRTTestResult
```

UI minima:

```text
[RUN] [RESET]

PASS / FAIL / ERROR / BLOCKED
Assertions
StateHash / hash disponibile
TurnLog
```

### DoD

- [ ] Run usa `URTScenarioRunner` o il suo percorso canonico
- [ ] nessun `SetActorLocation`/danno manuale
- [ ] risultato espone Pass/Fail/Error/Blocked
- [ ] assertion fallita mostra expected/actual
- [ ] TurnLog consultabile
- [ ] Reset ricrea initial state canonico, non fa undo della partita
- [ ] Run -> Reset -> Run dà lo stesso risultato per input invariato
- [ ] lo stesso file headless dà lo stesso risultato logico
- [ ] nessun branch `if Editor` nel resolver
- [ ] mutation test su esito/expectation

---

# 7. Acceptance scenario del Composer

Creare un primo scenario eseguibile piccolo.

Nome da scegliere secondo la tassonomia reale del repository; concetto:

`TacticalDesigner.Map.Move.Basic`

Initial state:

```text
A1 = un eroe v0.1 valido
Cell = una cella valida della fixture
Facing = E
```

Turn 1:

```text
A1 -> Move -> cella adiacente valida
```

Expect:

```text
UnitAtCell
LogEventCount(Move/...)
```

Questo scenario deve dimostrare nella stessa catena:

- placement;
- cell picking;
- Move authoring;
- writer;
- loader;
- runner;
- TurnLog;
- expectation;
- Reset;
- editor/headless parity.

Non creare subito un 2v2 complesso.

---

# 8. Aggiornamento delle issue esistenti

Per ogni issue #711, #622, #712, #623:

1. fetch issue + commenti + eventuale PR;
2. confrontare il corpo con `main`;
3. NON riscrivere la storia se è ancora utile;
4. correggere solo fatti diventati falsi;
5. aggiungere commento di consolidamento con:
   - HEAD misurato;
   - cosa è già consegnato;
   - cosa manca realmente;
   - issue/feature/PIE/session owner;
6. evitare di spuntare manualmente DoD non osservate;
7. chiudere solo quando la prova richiesta esiste.

Non riaprire issue chiuse per “collegarle” al Tactical Designer.

---

# 9. Documentazione da consolidare

Dopo l'audit, trovare l'owner documentale corretto.

Minimo atteso:

### Technical

Aggiungere o aggiornare una sezione che dica:

```text
Scenario Composer
  is an editor client of Scenario Harness
  and never a second Scenario Runtime.
```

Documentare le dipendenze:

```text
Hex cell picking
      ↓
Scenario canonical model
      ↓
Scenario writer/loader/index
      ↓
Scenario runner
      ↓
TurnLog/result
```

### Roadmap / Feature Registry

Registrare:

- #711 e #622 come residue reali Map Editor;
- #712 come authoring già implementato ma con verifica manuale residua, se ancora vero;
- #623 come leggibilità/session dependency, se ancora vero;
- nuova feature Scenario Composer se manca;
- issue SC-1..SC-4;
- scenario acceptance.

### Wiki

Aggiornare la Wiki solo dopo che il modello tecnico è consolidato.

La Wiki deve spiegare cosa può fare un designer, non diventare una seconda specifica C++.

### Archive

Una volta consolidato, spostare/copiare questo handoff e l'audit precedente nella posizione `docs/archive/src/...` conforme alle convenzioni, con nota:

> provenienza storica; non owner delle regole.

---

# 10. Parallelizzazione

Map lane e Scenario lane possono procedere in parallelo **solo dopo write-set audit**.

Ordine logico:

```text
MAP LANE
#623 ──► U22/#712
#711
#622

SCENARIO LANE
SC-1
  ↓
SC-2
  ↓
SC-3
  ↓
SC-4
```

#711 e SC-1 sono ottimi candidati paralleli perché toccano domini diversi, ma il repository deve confermare che i write-set non collidono.

Non mettere due worktree a modificare contemporaneamente:

- Feature Registry;
- `test-manuali-pie.md`;
- `editor-sessions.yaml`;
- stessi file del toolkit/editor mode.

Gli aggiornamenti di tracking condivisi vanno preferibilmente integrati in una fase di reconciliation.

---

# 11. Definition of Done del consolidamento

Il lavoro di Claude su questo handoff è completo quando:

- [ ] `main` è stato ri-auditato e HEAD registrato
- [ ] issue/PR duplicate sono state escluse
- [ ] #711/#622/#712/#623 sono allineate allo stato reale
- [ ] nessuna nuova issue duplica lavoro esistente
- [ ] SC-1..SC-4 sono create solo se realmente mancanti
- [ ] Scenario Composer ha un owner nel Feature Registry, se prima mancava
- [ ] tracking manuale/PIE/sessioni è coerente
- [ ] dipendenze sono nel graph ufficiale, se necessarie
- [ ] viste generate rigenerate tramite script
- [ ] validator del registry/documentazione passa
- [ ] Decision Log aggiornato solo se emersa una vera decisione
- [ ] primo acceptance scenario è registrato/planned
- [ ] handoff archiviato come provenance
- [ ] viene restituito un report finale con link a issue/PR/file modificati

---

# 12. Report finale richiesto a Claude

Restituire:

```text
AUDITED_HEAD:

EXISTING_ISSUES_UPDATED:
- #...
- #...

ISSUES_CREATED:
- #...
- #...

ISSUES_NOT_CREATED_BECAUSE_DUPLICATE:
- ...

FEATURE_REGISTRY:
- ...

ROADMAP:
- ...

EDITOR_SESSIONS:
- ...

PIE_TESTS:
- ...

SCENARIOS:
- ...

DECISIONS:
- none / D-...

DOCS:
- ...

GENERATED_VIEWS:
- ...

VALIDATION:
- feature_registry validate:
- docs links:
- docs naming:
- tests:

CONFLICTS / WRITE-SET:
- ...

NEXT EXECUTABLE ISSUE:
- #...

WHY THIS ONE:
- ...
```

Il report deve distinguere sempre:

- **misurato su `main`**;
- **decisione presa ora**;
- **lavoro pianificato**.

---

# 13. Scope guard

Non introdurre durante questo consolidamento:

- Skill Workbench completo;
- Balance analytics;
- modding pubblico;
- nuovo Scenario Runtime;
- nuovo MapDefinition;
- secondo pathfinder;
- secondo LOS/Targeting;
- secondo validator;
- Scenario Browser player-facing;
- reaction editor completo;
- ability editor completo;
- mega framework di assertion;
- persistence `.uasset` per gli scenari.

Obiettivo: rendere esplicita e tracciata la catena già scelta:

```text
Map data
  + Scenario data
  + canonical runtime
  + Editor tooling
  = Tactical Designer v0.1
```

senza duplicare nessuna authority.
