# RefactorTactics — Project Graph v1 / Handoff per Claude

> **PROPOSTA DI IMPLEMENTAZIONE — NON AUTOREVOLE finché non viene consolidata nel repository**
>
> Obiettivo: trasformare roadmap, feature, issue/checkpoint, scenari, PIE e asset in un unico grafo
> interrogabile e visualizzabile nel Project Control Center, senza introdurre una nuova fonte di stato.
>
> Data handoff: 2026-08-13.
>
> Prima di applicare qualsiasi modifica: riallinearsi a `origin/main`, controllare PR aperte e rigenerare
> sempre gli artefatti derivati dalle sorgenti correnti.

---

## 1. Decisioni di questa conversazione

### 1.1 Tre Execution Lane principali

Il progetto deve distinguere **come** viene eseguito il lavoro:

- `CODE` — lavoro sul repository eseguibile da agente/sviluppatore.
- `PIE` — lavoro che richiede una persona davanti a Unreal Editor / PIE.
- `ASSET` — download, migrazione, import, configurazione e verifica di asset esterni.

Queste NON sostituiscono i domini di prodotto.

Un elemento può quindi essere, ad esempio:

- `execution_lane: code` + `domain_group: actions_reactions`
- `execution_lane: pie` + `domain_group: actions_reactions`
- `execution_lane: asset` + `domain_group: characters_content`

### 1.2 CODE è suddivisa per raggruppamenti

Proposta v1, pensata per reggere v0.1 → v0.4:

1. `core_simulation`
2. `map_environment`
3. `actions_reactions`
4. `perception_network`
5. `characters_content`
6. `objectives_match`
7. `ui_presentation`
8. `ai_bots`
9. `tooling_data_qa`

Questi sono raggruppamenti di lettura/esecuzione. Non sono ownership di file, branch, milestone o gate.

### 1.3 Feature, lavoro ed evidenza sono tre cose diverse

- **Feature** = capacità del prodotto.
- **Work item** = lavoro concreto da realizzare.
- **Evidence** = prova che la capacità esiste e funziona.

Il grafo deve poter mostrare almeno:

`Release → Feature → WorkItem → Scenario/Test/PIE → Gate → Release`

ma senza trasformare tutto in una issue.

### 1.4 Scenari come nodi di prima classe

Il repository possiede già:
- scenari reali sotto `Scenarios/`;
- scenari `planned` dichiarati dal Feature Registry;
- classificazione automatica/umana nella Scenario Map.

Il grafo deve distinguere almeno:

- `RUNNABLE`
- `BLOCKED`
- `PLANNED`

e conservare la distinzione esistente fra:
- macchina esegue / macchina giudica;
- macchina esegue / umano giudica;
- umano esegue / umano giudica.

### 1.5 Capability come nodo/relazione

Non forzare sempre una dipendenza `issue → issue`.

Quando un consumer richiede solo una capacità specifica, usare una relazione di capability:

`WorkItem → provides Capability → required_by Scenario/WorkItem`

Questo evita dipendenze troppo rigide tra epic.

### 1.6 Conteggi sempre derivati

Mai scrivere a mano valori come:
- totale issue;
- totale checkpoint;
- totale work item;
- totale scenari;
- totale blocked/ready/done;
- totale per release/lane/workstream.

Il generatore deve produrre i conteggi e le viste devono solo visualizzarli.

---

## 2. Stato del repository che NON va ignorato

### 2.1 Feature Registry

`docs/roadmap/feature-registry.yaml` è già fonte canonica delle feature.

Contiene già, fra gli altri:
- `feature_id`
- `area`
- `kind`
- `release`
- `priority`
- `gates`
- `roadmap`
- `dependencies`
- `issues`
- `tests`
- `scenarios`
- `pie_refs`

NON trasformarlo in un database universale del progetto.

### 2.2 Editor sessions

`docs/roadmap/editor-sessions.yaml` esiste già come owner delle sedute manuali.

Decisione esistente del repository: NON inserire `editor_tasks:` nel Feature Registry.

La nuova Execution Lane `PIE` deve quindi evolvere/consumare `editor-sessions.yaml`, non duplicarlo.

La nuova Execution Lane `ASSET` dovrebbe preferibilmente usare lo stesso schema delle sessioni umane,
aggiungendo un campo `execution_lane`, invece di creare un terzo registro parallelo.

### 2.3 Project Control Center

Il Control Center è già implementato ed esiste `docs/roadmap/project-graph.json`.

Vincolo già deciso:
> la dashboard non calcola stato; riceve stato già derivato dal generatore e lo visualizza.

Quindi:
- niente regole di `READY/BLOCKED/DONE` nel JavaScript;
- niente parsing ad hoc dei markdown;
- se serve un dato nuovo, estendere Python e `project-graph.json`.

### 2.4 Le vecchie lane esistono già, ma sono SNAPSHOT

Il repository possiede viste datate:
1. Spatial / Map
2. Simulation / Turn
3. Client / UX
4. Editor / Tooling
5. Replay / Audit
6. Character
7. VFX / AssetFab

Sono esplicitamente una **chiave di lettura del backlog**, non una fonte di stato né ownership.

Non cancellarle e non copiarle alla lettera nel nuovo schema.

Mappatura concettuale proposta:

| Legacy lane | Nuovo modello |
|---|---|
| Spatial / Map | `CODE + map_environment` |
| Simulation / Turn | `CODE + core_simulation` e/o `actions_reactions` |
| Client / UX | `CODE + ui_presentation` |
| Editor / Tooling | codice → `CODE + tooling_data_qa`; lavoro manuale → `PIE` |
| Replay / Audit | `CODE + core_simulation` oppure `tooling_data_qa` |
| Character | `CODE + characters_content`, più eventuali PIE/ASSET |
| VFX / AssetFab | import/config → `ASSET`; runtime/presentation → dominio proprietario |

La vecchia lane 7 dimostra esattamente perché `ASSET` deve essere una dimensione di esecuzione separata dal dominio.

---

## 3. Release da visualizzare

Il primo grafo utile deve arrivare almeno a:

- `v0.1`
- `v0.2`
- `v0.3`
- `v0.4`
- `future`

Attenzione: il Feature Registry corrente usa ancora principalmente `v0.1 | v0.2 | future`.
La roadmap post-v0.1 possiede già v0.3 e v0.4.

Prima di migrare feature da `future` a `v0.3` / `v0.4`:
1. usare la roadmap post-v0.1 come owner di release;
2. verificare ogni Feature ID coinvolto;
3. non promuovere brainstorming/proposte a release canonica.

Non introdurre v0.5–v1.0 in questo slice: possono rimanere `future/north-star` finché non sono consolidate.

---

## 4. Modello dei nodi v1

Tipi minimi:

| Tipo | Owner |
|---|---|
| `release` | roadmap release |
| `feature` | `feature-registry.yaml` |
| `epic` | roadmap |
| `work_item` | nuovo source execution graph |
| `human_session` | `editor-sessions.yaml` |
| `scenario` | `Scenarios/` + planned del registry |
| `test` | test discovery/registry |
| `capability` | generator / scenario capability model |
| `gate` | Definition of Done / release gate |

Non rendere `Gate`, `Scenario` o `Feature` delle issue finte.

---

## 5. Relazioni v1

Set piccolo e verificabile:

### Hard
- `requires`
- `requires_capability`

### Soft / ordering
- `follows`

### Scope parziale
- `partial_block`

### Aggregazione
- `implements`
- `verifies`
- `provides`
- `contributes_to_gate`

### Navigazione
- `related`

`converges_into` può essere DERIVATO: se più workstream/lane puntano allo stesso nodo, quel nodo è un junction.
Non serve un booleano `junction: true` scritto a mano.

---

## 6. Stato operativo derivato

Il modello non deve contenere un `status` manuale per i work item se la sorgente reale è GitHub o una
condizione verificabile.

Stati di visualizzazione suggeriti:

- `READY`
- `BLOCKED`
- `WAITING_FOR_PIE`
- `WAITING_FOR_ASSET`
- `DONE`
- `UNKNOWN/STALE`

Derivazione indicativa:

- `WAITING_FOR_PIE`: primo prerequisito non risolto è una sessione `execution_lane: pie`.
- `WAITING_FOR_ASSET`: primo prerequisito non risolto è una sessione `execution_lane: asset`.
- `READY`: tutti i prerequisiti hard sono risolti e il lavoro non è chiuso.
- `BLOCKED`: almeno un prerequisito hard non è risolto.
- `DONE`: fonte operativa dichiara chiuso/verificato.
- `UNKNOWN/STALE`: sorgente non aggiornata o riferimento non risolvibile.

La funzione di derivazione deve vivere in Python.

---

## 7. File proposti

### Nuovi source file

`docs/roadmap/workstreams.yaml`
- tassonomia Execution Lane;
- domain group;
- mapping legacy;
- relazioni ammesse.

`docs/roadmap/work-items.yaml`
- soli work item che richiedono relazione/ordine esplicito;
- NON copia di tutte le issue GitHub;
- può partire dalla v0.1 e crescere fino a v0.4.

### File esistenti da estendere

`docs/roadmap/editor-sessions.yaml`
- aggiungere `execution_lane: pie|asset`;
- default compatibile: se assente, le sessioni attuali sono `pie`.

`scripts/feature_registry.py`
- carica i due YAML nuovi;
- valida riferimenti e cicli;
- deriva graph stats/stati;
- serializza nel `project-graph.json`.

`docs/control-center/*`
- nuova vista `Dependency Map` / `Execution Graph`;
- solo rendering e filtri;
- nessuna logica di stato.

### Generati

`docs/roadmap/project-graph.json`
- non editare a mano.

Eventuale:
`docs/roadmap/workmap.shortlist.md`
- vista testuale generata, utile per review e fallback.

---

## 8. Schema minimo di un WorkItem

Esempio concettuale:

```yaml
- id: WI-E14-5
  type: checkpoint
  checkpoint: E14.5
  issue: 165
  release: v0.1
  execution_lane: code
  domain_group: actions_reactions

  features:
    - RT-FEAT-CORE-DECISION-BOUNDARY
    - RT-FEAT-REACTION-OVERWATCH

  requires:
    - WI-E14-4

  provides_capabilities:
    - DecisionBoundary
```

Non assumere che gli ID e i titoli di questo esempio siano ancora il miglior mapping su `main`:
verificarli prima della migrazione.

---

## 9. Schema minimo di una sessione PIE/ASSET

PIE:

```yaml
- id: U-REACTION-VERIFY
  execution_lane: pie
  title: Verifica Decision Window
  unblocked_by:
    - E14.5
  verifies:
    - PIE-V01-REACTION
  issues:
    - 165
```

ASSET:

```yaml
- id: U-ASSET-CHARACTER
  execution_lane: asset
  title: Import/configurazione asset personaggio
  unblocked_by: []
  artifacts:
    - Content/RT/Characters/...
  issues:
    - 287
  done_when: asset richiesti presenti e configurati secondo le convenzioni
```

Non inventare asset o path reali: derivarli da `convenzioni-contenuti-ue.md`, issue live e contenuto corrente.

---

## 10. Cosa deve mostrare il Control Center

Niente grossi ASCII diagram.

### Vista Overview

Contatori derivati:
- Feature
- Work items
- Issue open/closed
- PIE ready/waiting
- Asset ready/waiting
- Scenari runnable/blocked/planned
- Gate
- warning diagnostici

### Vista Execution Graph

Filtri:
- Release
- Execution Lane: CODE / PIE / ASSET
- Domain group
- State
- Feature
- Epic
- Scenario
- Open only
- Upstream / downstream del nodo selezionato

### Presentazione

Preferire:
- colonne/lane;
- DAG orientato sinistra → destra;
- nodo selezionato con upstream/downstream evidenziati;
- edge type visibile;
- default “open + blocked/ready”, non tutto il corpus insieme.

Evitare un force graph globale con centinaia di nodi: diventa un hairball.

---

## 11. Diagnostiche obbligatorie

Il validator deve almeno rilevare:

1. Work item con ID duplicato.
2. Riferimento a Feature ID inesistente.
3. Riferimento a issue/checkpoint/sessione inesistente.
4. Cicli fra dipendenze `requires`.
5. `execution_lane` sconosciuta.
6. `domain_group` sconosciuto.
7. Work item senza feature/epic/razionale, se richiesto dallo schema.
8. Feature con gate aperti ma nessun lavoro mappato (warning, non sempre error).
9. Scenario `BLOCKED` senza provider noto della capability (warning).
10. Sessione PIE/ASSET orfana.
11. Dipendenza hard su lavoro già sostituito/deprecato.
12. Generated artifacts stale rispetto ai source YAML.

Non creare warning rumorosi senza actionable fix.

---

## 12. Conteggi da generare

Aggiungere a `project-graph.json`, senza hard-code:

```json
{
  "project_stats": {
    "features": {},
    "work_items": {},
    "issues": {},
    "scenarios": {},
    "human_sessions": {},
    "dependencies": {},
    "by_release": {},
    "by_execution_lane": {},
    "by_domain_group": {}
  }
}
```

Le viste Markdown e il Control Center leggono questi numeri. Nessun README deve mantenere una copia manuale.

---

## 13. Sequenza di implementazione

### Slice A — Contratto dati
1. Audit `main` + PR aperte.
2. Aggiungere `workstreams.yaml`.
3. Aggiungere `work-items.yaml` con un piccolo seed v0.1.
4. Estendere `editor-sessions.yaml` con default `PIE`.
5. Validator e test Python.

### Slice B — Generator
6. Integrare i nuovi source nel generatore.
7. Produrre nodi/edge/stats in `project-graph.json`.
8. Aggiungere `--check`.
9. Generare una shortlist testuale.

### Slice C — UI
10. Aggiungere Dependency/Execution Map al Control Center.
11. Filtri e detail drawer.
12. Focus upstream/downstream.
13. Nessuna logica di stato in JS.

### Slice D — Migrazione
14. Popolare tutta v0.1.
15. Popolare scheletro v0.2.
16. Riallineare Feature Registry a v0.3/v0.4 solo dove la roadmap è inequivocabile.
17. Popolare v0.3/v0.4.
18. Lasciare il resto `future`.

### Slice E — Pulizia
19. Confrontare le vecchie lane SNAPSHOT col grafo.
20. Se il nuovo sistema copre le stesse domande, marcare le snapshot come superseded/archived; non cancellarle
    prima di aver dimostrato la parità informativa.

---

## 14. Acceptance criteria

La prima versione è riuscita quando:

- si può scegliere `v0.1`, `v0.2`, `v0.3`, `v0.4`;
- si vedono separatamente CODE, PIE e ASSET;
- CODE è filtrabile per domain group;
- una Feature porta ai work item e alle prove;
- uno Scenario porta alle Feature che verifica;
- una sessione PIE/ASSET mostra cosa la sblocca e cosa sblocca;
- un WorkItem mostra upstream/downstream;
- le capability mancanti rendono visibili i blocchi;
- i conteggi sono generati;
- nessuna vista ricalcola stato;
- `generate --check` rileva artefatti vecchi;
- nessun file generato viene editato a mano.

---

## 15. Vincoli di migrazione importanti

- Non fidarsi dei conteggi scritti in snapshot datate.
- GitHub resta owner dello stato issue.
- Feature Registry resta owner dello stato feature.
- `editor-sessions.yaml` resta owner del lavoro umano.
- Le vecchie 7 lane sono input di comprensione, non schema canonico.
- Non aprire issue nuove solo perché un nodo manca: prima stabilire se è veramente lavoro autonomo o solo
  una relazione/capability mancante.
- Non introdurre ownership di cartelle per lane.
- Non introdurre nuovi spazi di numerazione per gate/milestone.
- Non modificare `project-graph.json` a mano.
- Non trasformare la dashboard in source of truth.

---

## 16. Commit suggeriti

1. `feat(roadmap): add execution graph source schemas`
2. `feat(roadmap): derive work graph and project stats`
3. `test(roadmap): validate execution graph references and cycles`
4. `feat(control-center): add execution dependency map`
5. `docs(roadmap): migrate v0.1 execution graph`
6. `docs(roadmap): map v0.2-v0.4 product graph`

Tenere separata la migrazione massiva dei dati dalla modifica del generatore, per rendere la review leggibile.
