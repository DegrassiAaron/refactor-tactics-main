# RefactorTactics — Project Graph v1 · Implementation Handoff per Claude

> **STATO: PROPOSTA DI IMPLEMENTAZIONE**
>
> Questo pacchetto consolida e **sostituisce** la precedente ipotesi con due nuovi source file
> `workstreams.yaml` + `work-items.yaml`.
>
> Dopo aver letto la spec reale del Project Control Center (§8–§9) e il generatore corrente,
> la soluzione v1 consigliata è più piccola:
>
> **un solo nuovo source `docs/roadmap/execution-graph.yaml`**, più l'estensione compatibile di
> `editor-sessions.yaml` con `execution_lane`.
>
> Nessun file di questo bundle va copiato alla cieca. Prima di applicare: audit di `origin/main`,
> PR aperte, baseline dei generatori e confronto coi source owner.
>
> Data: 2026-08-13.

---

## 1. Vincoli già decisi nel repository

### 1.1 Control Center = UI, non source of truth

La spec corrente impone:

- la pagina non calcola `status`;
- `project-graph.json` è il contratto generato delle viste non-feature;
- se una vista vuole un dato nuovo, si estende Python, non il browser;
- il Control Center resta statico/read-only, senza backend e senza build step.

Questa proposta NON riapre tali decisioni.

### 1.2 Editor work resta fuori dal Feature Registry

La decisione D-A della spec PCC dice che il lavoro umano vive in
`docs/roadmap/editor-sessions.yaml`, non in `feature-registry.yaml`.

Quindi:
- `PIE` e `ASSET` evolvono `editor-sessions.yaml`;
- non introdurre `editor_tasks:` nelle feature;
- non introdurre uno `status` manuale delle sessioni.

### 1.3 Relazioni qualificate nel Feature Registry erano rimandate

La decisione D-B rimandava strutture come:

```yaml
issues:
  - id: 165
    relation: implements
```

dentro il Feature Registry.

Questa proposta **non migra `issues:`**.

Le relazioni di esecuzione nuove vivono nel source dedicato `execution-graph.yaml`.
La Feature Registry resta invariata nel proprio ruolo.

---

## 2. Decisione v1: un solo source per la topologia di esecuzione

### Nuovo file

```text
docs/roadmap/execution-graph.yaml
```

Possiede soltanto:

1. tassonomia `CODE / PIE / ASSET`;
2. raggruppamenti CODE;
3. nodi di esecuzione che non hanno già un owner migliore;
4. relazioni tipizzate fra riferimenti esistenti;
5. capability esplicite;
6. release/order strutturali quando non derivabili.

NON possiede:
- stato Feature;
- stato issue;
- stato checkpoint;
- stato PIE;
- stato sessione;
- titolo di issue duplicato;
- scenario file;
- gate Feature;
- gate Release.

### Perché uno solo

Due source separati (`workstreams` + `work-items`) creerebbero:
- due validator;
- due punti di drift;
- due versioni schema;
- una relazione da mantenere fra file che descrivono lo stesso execution graph.

Il file singolo mantiene la regola del repository:
> **una responsabilità nuova → una sorgente dedicata, non una famiglia di mini-registry.**

---

## 3. Execution Lane

Tre lane v1:

```text
CODE
PIE
ASSET
```

### CODE

Default per `issue:` / `checkpoint:` / task di repository.

È suddivisa in domain group:

1. `core_simulation`
2. `map_environment`
3. `actions_reactions`
4. `perception_network`
5. `characters_content`
6. `objectives_match`
7. `ui_presentation`
8. `ai_bots`
9. `tooling_data_qa`

### PIE

Lavoro il cui output primario è:
- verdetto umano;
- verifica a schermo;
- configurazione/uso Editor necessaria per validare;
- playtest.

### ASSET

Lavoro il cui output primario è:
- asset migrato/importato;
- Blueprint/AnimBP/materiale/configurazione asset;
- file binario tracciato e validato.

Una sessione ASSET può avere `verifies: [PIE-*]`: l'evidence e il tipo di lavoro non sono la stessa cosa.

---

## 4. Compatibilità con `editor-sessions.yaml`

Aggiungere alla schema validation:

```yaml
execution_lane: pie | asset
```

Default retrocompatibile:

```text
campo assente → pie
```

Per la migrazione, NON classificare automaticamente tutte le sessioni in Asset.
Fare un audit sessione per sessione.

Thin slice già verificato:
- `U7 Personaggi Paragon` → `asset`
- `U8 Animazioni` → `asset`
- `U9 Leggibilità e riferimento visivo` → `pie`

Il resto resta `pie` finché non auditato.

Aggiornare:
- `SESSION_FIELDS`;
- `validate_editor_sessions()`;
- `build_graph()` serialization;
- `render_shortlist_editor()` solo se la lane è utile nella vista markdown.

Lo stato continua a provenire da:
`session_state(verifies + tracked artifacts)`.

---

## 5. Schema di `execution-graph.yaml`

### Principio

**Una sola direzione canonica delle relazioni.**

NON scrivere contemporaneamente:
- `requires`;
- e `blocks`.

Si autorializza `from → to`; il generator deriva l'inversa.

### Riferimenti

Forme ammesse:

```text
issue:<number>
checkpoint:E<n>.<m>
epic:E<n>
session:U<n>
feature:RT-FEAT-...
scenario:<ScenarioId>
capability:<Name>
gate:G<n>
release:v0.1
```

Non usare ID nudi quando esistono namespace diversi.

### Nodi

Un nodo `issue:` nel file NON duplica il titolo o lo stato GitHub.

Esempio:

```yaml
nodes:
  - id: issue:165
    release: v0.1
    domain_group: actions_reactions
    provides:
      - DecisionBoundary
```

`execution_lane` è opzionale:
- `issue:*` default `code`;
- `session:*` viene risolta da `editor-sessions.yaml`;
- altri tipi la richiedono solo se esecutivi.

### Relazioni

Tipi v1:

Hard:
- `requires`
- `requires_capability`

Soft:
- `follows`
- `related`

Evidence/trace:
- `implements`
- `verifies`
- `provides`
- `contributes_to_gate`

NON autorializzare:
- `blocks` → deriva da `requires`;
- `converges_into` → deriva dagli ingressi;
- `junction: true` → deriva;
- `release_gate` come edge generico → usare `contributes_to_gate`.

---

## 6. Stato e readiness

### Regola fondamentale

Il nuovo YAML **non contiene stato operativo**.

Il generator usa gli owner esistenti quando possibile.

### Fonti disponibili offline

- Feature: Feature Registry.
- Epic/checkpoint: roadmap owner.
- Sessione: `session_state()`.
- PIE: registro PIE.
- Scenario: corpus + capability disponibili.
- Issue non-checkpoint: **nessuna fonte live offline**.

Non aggiungere:
```yaml
state: open
```

per sopperire al fatto che GitHub non è letto dal generator.

### v1

Per un `issue:` che è anche checkpoint:
- usare lo stato del checkpoint owner.

Per un issue standalone:
- stato operativo può essere `unknown`;
- il link GitHub resta disponibile;
- il graph NON deve fingere `READY/DONE`.

### Readiness

Derivare soltanto quando i prerequisiti e lo stato del target sono risolvibili.

Valori UI suggeriti:
- `READY`
- `BLOCKED`
- `WAITING_FOR_PIE`
- `WAITING_FOR_ASSET`
- `DONE`
- `UNKNOWN`

`follows` e `related` non partecipano alla readiness.

---

## 7. Thin slice v0.1 da implementare per primo

### Reactions

Hard:
- #165 → #166
- #165 → #314
- #165 → #512

Soft:
- #166 → #314 (`follows`)
- #314 → #319 (`follows`)

Capability:
- #165 provides `DecisionBoundary`
- `DecisionBoundary` required by #512 e dagli scenari che la dichiarano.

Historical provider:
- #318 provides TurnLog assertions; issue chiusa.
- #361 chiusa: contratto Time Bank / TurnLog.

### Golden / Showcase

Hard:
- #512 → #170
- #66 → #170
- #75 → #170
- #170 → #171

Soft/related:
- #625 `follows` prima di #170 per evitare costo di rebaseline;
- #649 `related` a #170;
- #687 `related` a #170.

NON promuovere le ultime tre a hard dependency senza decisione owner.

### Character

Hard:
- #287 → #288
- #287 → #289

Sessions:
- U7 = ASSET
- U8 = ASSET
- U9 = PIE

Soft:
- U7 → U8 follows
- U8 → U9 follows

Related:
- #593 ↔ U7/U9 (la issue dichiara esplicitamente che non blocca U7)
- #715 ↔ E21

Evidence:
- U7 implements #287
- U8 implements #288
- U9 verifies #289

---

## 8. Estensione `project-graph.json`

Aggiungere:

```json
"execution": {
  "schema_version": 1,
  "lanes": [],
  "domain_groups": [],
  "nodes": [],
  "edges": [],
  "capabilities": [],
  "stats": {}
}
```

### Node generated contract

```json
{
  "id": "issue:165",
  "kind": "issue",
  "ref": 165,
  "release": "v0.1",
  "execution_lane": "code",
  "domain_group": "actions_reactions",
  "state": "⏳",
  "state_source": "checkpoint:E14.5",
  "readiness": "READY",
  "feature_ids": [],
  "provides": ["DecisionBoundary"],
  "incoming": [],
  "outgoing": []
}
```

Note:
- `state` / `readiness` sono generati.
- `feature_ids` deve essere ricavato dall'inversa delle `issues:` del Feature Registry quando possibile.
- non riscrivere manualmente i Feature ID nell'execution source se il registry possiede già il mapping.

### Edge generated contract

```json
{
  "from": "issue:165",
  "to": "issue:512",
  "type": "requires",
  "hard": true,
  "rationale": null
}
```

### Capability generated contract

```json
{
  "id": "DecisionBoundary",
  "providers": ["issue:165"],
  "consumers": ["issue:512", "scenario:..."],
  "available": false
}
```

`available` deve essere derivato:
- da provider soddisfatto;
- oppure dalla capability già disponibile nel Scenario Harness, se è una capability runtime esistente.

Se i due concetti non sono semanticamente identici, tenerli distinti:
`execution capability` vs `scenario capability`.
Non fonderli per somiglianza di stringa senza test.

---

## 9. `project_stats`

Aggiungere solo metriche che il generator può calcolare senza ambiguità.

```json
"project_stats": {
  "execution_nodes": 0,
  "execution_edges": 0,
  "hard_edges": 0,
  "soft_edges": 0,
  "by_execution_lane": {},
  "by_domain_group": {},
  "by_release": {},
  "readiness": {}
}
```

NON chiamare `total_issues` ciò che è soltanto “issue referenziate nel grafo”.

Se serve:
```text
referenced_issues
```

Il totale GitHub globale non è disponibile offline.

---

## 10. Validator Python

Nuove verifiche minime:

### Errori

- schema_version sconosciuta;
- lane sconosciuta;
- domain group sconosciuto;
- node id duplicato;
- relazione con endpoint non risolvibile;
- self dependency `requires`;
- ciclo nel sottografo hard `requires`;
- `requires_capability` senza capability valida;
- `session:` inesistente;
- `feature:` inesistente;
- `checkpoint:` inesistente;
- `gate:` inesistente;
- relation type sconosciuto;
- session `execution_lane` non `pie|asset`.

### Warning

- capability senza provider;
- nodo issue non mappabile a Feature;
- issue standalone senza stato offline;
- `related` duplicato;
- `follows` che contraddice una hard edge opposta;
- nodo non raggiungibile da nessuna release/feature/epic;
- feature con issue nel registry ma nessun work node nel grafo, **solo dopo che la migrazione v0.1 è dichiarata completa**.

Non attivare warning “copertura completa” durante il thin slice: sarebbero rumore intenzionale.

---

## 11. Test Python obbligatori

Creare fixture minime, non solo test sul file reale.

1. parse schema valido;
2. unknown lane → error;
3. unknown domain → error;
4. duplicate node → error;
5. unknown endpoint → error;
6. hard cycle A→B→A → error;
7. `follows` cycle non necessariamente error;
8. inverse `blocks` derivata, non autorializzata;
9. `related` non rende blocked;
10. `follows` non rende blocked;
11. hard prerequisite PIE incompleto → `WAITING_FOR_PIE`;
12. hard prerequisite ASSET incompleto → `WAITING_FOR_ASSET`;
13. provider capability soddisfatto → capability available;
14. provider non soddisfatto → unavailable;
15. due run generate identiche;
16. `generate --check` fallisce su project-graph stale.

Aggiungere poi contract test sui file reali.

---

## 12. Control Center — prima vista

### Nuovo tab

```text
Execution Map
```

NON sostituire:
- Roadmap;
- Feature Map;
- Scenario Map;
- Editor Map.

È una vista differente: risponde a “cosa blocca cosa e chi deve intervenire”.

### Default

Non mostrare tutto il progetto.

Filtri iniziali:
- Release
- Execution Lane
- Domain group
- Readiness
- Hard only
- Open/unfinished where known
- Search

Default raccomandato:
- release `v0.1`;
- unfinished;
- hard + soft visibili con distinzione;
- collapsed completed providers.

### Due modalità

**Roads**
- tabella leggibile;
- colonne: nodo, lane, domain, readiness, hard prerequisites, soft order, unlocks.

**Focus Graph**
- visualizza solo il nodo selezionato + upstream/downstream a distanza configurabile;
- evita il “hairball”.

Il grafo globale completo NON è la vista predefinita.

---

## 13. Layout grafico zero dipendenze

Non aggiungere D3/Cytoscape.

Layout deterministico:

- X = profondità topologica delle edge `requires`;
- Y = `execution_lane`;
- dentro CODE, Y secondario = `domain_group`;
- `follows` disegnato tratteggiato;
- `related` puntinato;
- `requires` linea piena;
- capability con nodo compatto dedicato;
- junction = nodo con >1 incoming hard da gruppi/lane distinti.

È lecito calcolare layout nel browser:
è struttura visiva, **non stato**.

---

## 14. Funzioni pure JS da aggiungere

In `graph.js`:

```text
executionIndex(graph)
executionNode(graph, id)
executionIncoming(index, id, types?)
executionOutgoing(index, id, types?)
executionNeighborhood(index, id, depth, filters)
topologicalDepths(nodes, hardEdges)
filterExecution(nodes, filters)
```

NON aggiungere:
```text
deriveReadiness()
isBlocked()
isDone()
```

Devono arrivare già dal JSON.

---

## 15. Test JS

Fixture:

1. indice forward/inverse;
2. hard vs soft edge;
3. filter release;
4. filter lane;
5. filter domain;
6. filter readiness;
7. neighborhood depth 1;
8. neighborhood depth 2;
9. topological depth stabile;
10. junction riconoscibile dalla struttura già serializzata o dal numero di incoming per layout;
11. issue ref rotto visibile;
12. capability ref rotto visibile.

Contract:
- `GRAPH.execution` esiste;
- lane `code/pie/asset` esistono;
- thin slice contiene #165, #170, U7/U8/U9;
- U7/U8 asset, U9 pie;
- #593→U7 non è hard;
- #165→#512 è hard;
- #166→#314 è soft;
- nessun JS calcola readiness.

---

## 16. Correzione della staleness

Oggi il banner confronta:
- `feature-registry.yaml`;
- `project-graph.json`.

Con un nuovo source deve confrontare almeno anche:
- `execution-graph.yaml`;
- `editor-sessions.yaml`.

Meglio: il generator inserisce in `project-graph.json` la lista completa delle source che alimentano l'execution graph.
La UI controlla la freschezza delle source dichiarate, non una lista hardcoded duplicata.

Se l'API rate limit rende il check troppo costoso, mantenere il check attuale e aggiungere un solo warning:
“freschezza execution source non verificata”.
Non introdurre 10 fetch senza misura.

---

## 17. PR aperta rilevata durante questo handoff

Al momento dell'audit risulta aperta la PR #734.

Tocca almeno:
- PIE / naming;
- `project-graph.json` rigenerato.

Quindi Claude deve:
1. verificare se #734 è ancora aperta;
2. se viene mergiata prima di questo lavoro, partire dal main aggiornato;
3. se resta aperta, evitare di risolvere il JSON generato a mano;
4. rigenerare `project-graph.json` dalla sorgente unita.

---

## 18. Ordine di commit consigliato

### Commit 1 — source + parser
`feat(roadmap): add execution graph source and validation`

- execution-graph.yaml thin slice;
- execution_lane in editor sessions;
- loader;
- validator;
- test Python.

### Commit 2 — generated contract
`feat(roadmap): generate execution topology in project graph`

- build_graph extension;
- project_stats;
- idempotence;
- --check.

### Commit 3 — pure JS
`feat(control-center): index and filter execution graph`

- graph.js pure functions;
- JS fixture + contract tests.

### Commit 4 — UI
`feat(control-center): add execution map thin slice`

- tab;
- Roads table;
- Focus Graph;
- drawer extensions.

### Commit 5 — docs
`docs(control-center): document execution lanes and graph ownership`

- README;
- PCC spec appendix / decision log as appropriate;
- CONTEXT_INDEX pointer if still outstanding.

NON fare la migrazione completa v0.1 nello stesso PR della nuova infrastruttura.

---

## 19. Gate di accettazione del thin slice

Il thin slice passa se:

- `validate` = 0 nuovi errori;
- test Python nuovi verdi;
- `generate --check` verde;
- due generate consecutive → diff vuoto;
- `node --test docs/control-center/` verde;
- #165 mostra upstream/downstream e `DecisionBoundary`;
- #170 appare come junction;
- #593 non blocca U7;
- U7/U8 = ASSET;
- U9 = PIE;
- `follows` non diventa BLOCKED;
- `related` non diventa BLOCKED;
- scenario con capability mancante resta BLOCKED per la regola Python esistente;
- nessun generated editato a mano;
- nessun `status:` nuovo nei YAML source.

Solo dopo questo gate: migrazione completa v0.1.

---

## 20. Dopo il thin slice

Ordine:

1. v0.1 completa;
2. v0.2 completa;
3. riallineamento release `future → v0.3/v0.4` solo con owner roadmap;
4. v0.3;
5. v0.4;
6. Future/North Star.

La prima dashboard utile deve poter zoomare:
- da release;
- a feature;
- a execution road;
- a PIE/ASSET;
- a scenario/evidence;
senza richiedere una seconda source of truth.
