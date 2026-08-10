# RefactorTactics — Project Control Center / Dashboard YAML
## Handoff operativo per Claude

## Obiettivo

Creare una pagina web di **Project Control Center** per RefactorTactics che mostri lo stato delle principali mappe operative del progetto usando **GitHub come sorgente dati**.

La pagina NON deve diventare una nuova source of truth.

La source of truth esistente va mantenuta e ampliata dove necessario.

Repository verificato:

```text
DegrassiAaron/refactor-tactics-main
```

Branch canonico:

```text
main
```

Feature Registry canonico verificato:

```text
docs/roadmap/feature-registry.yaml
```

Lo script esistente che lo usa è:

```text
scripts/feature_registry.py
```

Lo script dichiara esplicitamente che lo stato delle feature vive in:

```text
docs/roadmap/feature-registry.yaml
```

e che le viste derivate non devono duplicare manualmente lo stato.

Il file generato esistente è:

```text
docs/roadmap/feature-registry.json
```

Le viste derivate già presenti includono almeno:

```text
docs/roadmap/roadmap.shortlist.md
docs/roadmap/featuremap.shortlist.md
docs/roadmap/scenariomap.shortlist.md
docs/roadmap/milestonemap.shortlist.md
docs/wiki/feature-status.md
```

Altri documenti rilevanti già presenti:

```text
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-checkpoint.md
docs/technical/scenario-map.md
docs/roadmap/feature-registry.md
```

---

# 1. Principio architetturale

La dashboard deve essere una UI sopra i dati di progetto già versionati nel repository.

```text
GitHub / main
     |
     v
docs/roadmap/feature-registry.yaml
     |
     +--------------------+
     |                    |
     v                    v
validator / generator   dashboard loader
     |                    |
     v                    v
viste derivate         Project Control Center
```

Non creare un database duplicato.

Non creare copie manuali dello stato delle feature.

Git deve restare lo storico delle modifiche.

---

# 2. Le viste del Project Control Center

La pagina deve avere almeno queste viste.

## 2.1 Overview

Mostrare:

- avanzamento generale;
- milestone correnti;
- feature bloccate;
- issue aperte;
- scenari incompleti;
- task Editor che richiedono intervento umano;
- elementi che stanno bloccando altri elementi;
- warning di riferimenti mancanti o inconsistenti.

La Overview deve essere derivata dai dati canonici.

---

## 2.2 Roadmap

La Roadmap rappresenta il lavoro esecutivo.

Deve collegare:

```text
Milestone
 -> Epic
 -> Checkpoint
 -> Issue
 -> Feature
```

Quando disponibile, ogni elemento deve poter aprire la relativa pagina GitHub.

Esempi:

- GitHub issue;
- GitHub milestone;
- GitHub pull request, se introdotta come evidenza;
- documento roadmap nel repository.

---

## 2.3 Feature Map

La Feature Map rappresenta ciò che il gioco deve saper fare.

Il `feature-registry.yaml` possiede già campi utili come:

```yaml
feature_id:
title:
area:
kind:
release:
priority:
status:
gates:
roadmap:
dependencies:
owner_specs:
issues:
tests:
scenarios:
wiki_refs:
last_verified:
notes:
```

La pagina deve mostrare questi dati senza duplicarli.

Una card Feature dovrebbe poter visualizzare almeno:

```text
Feature ID
Titolo
Area
Release
Priority
Status
Gate completion
Milestone / Epic / checkpoint
Dependencies
Issues
Scenarios
Tests
Wiki / owner specs
Editor tasks
Last verified
Notes
```

Non introdurre una percentuale arbitraria come source of truth.

Il sistema esistente usa il concetto:

```text
N/M gate
```

ed è preferibile mantenerlo.

Il `status` deve continuare a essere coerente con i gate e validato dallo script esistente.

---

# 3. Scenario Map

La Scenario Map rappresenta demo, test di integrazione, casi di gameplay e prove funzionali.

Il progetto usa già `Scenarios/` e il Feature Registry può referenziare ScenarioId reali o pianificati.

La dashboard deve permettere:

```text
Scenario
 -> Feature validate
 -> Issue necessarie
 -> Editor task necessari
 -> Test automatici
 -> documentazione / Wiki
```

Esempio concettuale:

```text
SCN-OVERWATCH-BAIT

Features:
- Overwatch
- Fast Reaction

Issues:
- Reaction Opportunity lifecycle
- Fast Reaction UI

Editor:
- Overwatch cone/VFX validation

Docs:
- Overwatch Wiki
- Reaction spec
```

Gli scenari devono aiutare a rispondere alla domanda:

> "Cosa manca perché questo scenario sia realmente eseguibile e verificabile?"

---

# 4. Editor Map

Aggiungere una nuova vista chiamata **Editor Map**.

Questa vista contiene ESCLUSIVAMENTE task che richiedono davvero intervento umano nell'Unreal Editor o una decisione visuale/manuale.

Esempi validi:

- selezionare il modello di un personaggio;
- selezionare una skin;
- assegnare materiali;
- scegliere una mesh;
- configurare Animation Blueprint;
- verificare socket;
- impostare muzzle/socket;
- validare facing visivamente;
- piazzare elementi del graybox quando la scelta è intenzionalmente manuale;
- configurare una scena;
- validare VFX;
- verificare look & feel;
- selezionare asset;
- attività che Claude/C++/script/commandlet non possono completare in modo affidabile senza intervento umano.

NON devono finire nell'Editor Map attività automatizzabili come:

- modificare C++;
- aggiungere una USTRUCT;
- eseguire test;
- generare file;
- modificare JSON/YAML;
- creare dati tramite script;
- validare riferimenti tramite commandlet;
- operazioni che Claude può svolgere direttamente nel repository.

Regola:

```text
AUTOMATIZZABILE
Claude / C++ / script / CI
      |
      v
Issue / implementation task

MANUALE
Unreal Editor / scelta visuale / asset
      |
      v
Editor Task
```

---

# 5. My Editor Queue

La dashboard deve avere anche una vista/filtro operativo chiamato:

```text
My Editor Queue
```

Lo scopo è mostrare immediatamente i task che richiedono l'intervento del proprietario del progetto.

Raggruppare almeno:

```text
BLOCKING
READY
WAITING
DONE
```

Esempio UI:

```text
MY EDITOR QUEUE

BLOCKING
□ Selezionare mesh di Flux
□ Assegnare skin di Riva
□ Verificare Overwatch cone decal

READY
□ Configurare shield socket
□ Validare muzzle socket

WAITING
○ Water VFX
  blocked by issue #...

○ Character animations
  blocked by issue #...
```

Ogni task deve mostrare cosa sblocca.

---

# 6. Relazioni tra le mappe

Roadmap, Feature Map, Scenario Map ed Editor Map NON sono sistemi separati.

Devono essere viste dello stesso grafo di progetto.

Modello concettuale:

```text
Feature
   |
   | implemented by
   v
Issue / Roadmap
   |
   | validated by
   v
Scenario
   |
   | may require
   v
Editor Task
```

Aggiungere anche relazioni inverse nella UI.

Esempio:

clic su una Feature:

```text
Feature: Overwatch
 |
 +-> Issue
 |
 +-> Scenario
 |
 +-> Editor Task
 |
 +-> Wiki
 |
 +-> Owner Spec
 |
 +-> Milestone
```

Clic su uno Scenario:

```text
Scenario
 |
 +-> Feature coinvolte
 +-> Issue bloccanti
 +-> Editor Task
 +-> Wiki/spec
```

---

# 7. Collegamenti navigabili

Gli elementi delle varie liste devono avere link utilizzabili.

Tipi di collegamento richiesti:

```text
GitHub Issue
GitHub Milestone
GitHub Pull Request
Wiki page
Repository document
Owner spec
Feature interna
Scenario interno
Editor Task interno
Epic/checkpoint
External URL, solo se necessario
```

Non duplicare URL completi quando possono essere derivati.

Configurazione centrale suggerita:

```yaml
project:
  github:
    owner: DegrassiAaron
    repository: refactor-tactics-main
    branch: main
```

Esempio:

```yaml
issues:
  - 142
```

La UI deriva:

```text
https://github.com/DegrassiAaron/refactor-tactics-main/issues/142
```

Per i documenti repository:

```yaml
wiki_refs:
  - docs/wiki/meccaniche/overwatch.md
```

la pagina deve aprire:

```text
https://github.com/DegrassiAaron/refactor-tactics-main/blob/main/docs/wiki/meccaniche/overwatch.md
```

Per i documenti interni usare sempre path repository-relative quando possibile.

---

# 8. Relazioni semanticamente qualificate

Quando utile, evolvere le semplici liste verso relazioni con significato.

Esempio:

```yaml
issues:
  - id: 142
    relation: implements

  - id: 148
    relation: ui

  - id: 151
    relation: tests
```

La UI può quindi mostrare:

```text
Implementation
#142 Reaction lifecycle

UI
#148 Fast Reaction UI

Tests
#151 Overwatch deterministic tests
```

Per gli scenari:

```yaml
scenarios:
  - id: SCN-OVERWATCH-BASIC
    relation: validates

  - id: SCN-OVERWATCH-BAIT
    relation: demonstrates
```

IMPORTANTE:

prima di modificare lo schema attuale, valutare retrocompatibilità con `scripts/feature_registry.py`.

Non rompere il formato corrente senza migrazione e test.

---

# 9. Editor Task schema

La Editor Map non esiste ancora come vista canonica verificata.

Introdurre il minimo schema necessario.

Possibile struttura:

```yaml
editor_tasks:
  - task_id: RT-EDITOR-CHAR-FLUX-MODEL
    title: Select Flux character model
    status: TODO
    priority: P0

    category: character

    related_features:
      - RT-FEAT-CHAR-FLUX

    related_scenarios:
      - SCN-DEMO-2V2

    blocked_by_issues:
      - 123

    instructions:
      - Open character Blueprint
      - Select approved skeletal mesh
      - Assign materials
      - Verify skeleton
      - Verify Animation Blueprint

    evidence:
      - screenshot_required
      - manual_validation
```

Valutare se:

A. tenere `editor_tasks` nel Feature Registry;

oppure

B. creare un file dedicato referenziato dal registry.

Preferire la soluzione che evita duplicazione e mantiene semplice il validator.

Non creare cinque source of truth separate solo perché esistono cinque viste UI.

---

# 10. GitHub come sorgente della pagina

La pagina deve usare il file presente su GitHub `main`.

Sorgente canonica:

```text
https://raw.githubusercontent.com/DegrassiAaron/refactor-tactics-main/main/docs/roadmap/feature-registry.yaml
```

Per uso applicativo è preferibile valutare GitHub API/Contents API rispetto al raw URL se servono:

- metadata;
- SHA;
- gestione errori;
- autenticazione futura;
- scrittura;
- ETag/cache;
- branch esplicito.

La dashboard deve mostrare almeno:

```text
Source branch: main
Last loaded commit/SHA
Last refresh
Registry audit date
Registry schema version
```

---

# 11. Read-only prima, write dopo

Prima milestone del dashboard:

```text
READ ONLY
```

La pagina:

1. legge il registry da GitHub;
2. valida/parsa YAML;
3. costruisce il grafo;
4. mostra le viste;
5. genera link;
6. segnala riferimenti rotti;
7. NON modifica GitHub.

Solo dopo aver stabilizzato il modello introdurre editing.

Per la futura scrittura NON scrivere "magicamente" dal browser senza controllo.

Flusso raccomandato:

```text
Dashboard
   |
   v
Backend / GitHub integration
   |
   +-> fetch current file + SHA
   +-> validate proposed change
   +-> create branch
   +-> update YAML
   +-> regenerate derived files
   +-> run validator
   +-> open PR
```

Preferire PR rispetto a commit diretto su `main`.

---

# 12. Derivazione del progresso

Non salvare percentuali arbitrarie nel YAML.

Il progetto possiede già gate:

```text
spec
data
runtime
log_debug
automation
scenario
ui_wiki
packaged
network_privacy
```

La dashboard deve visualizzare il progresso come:

```text
6 / 8 gate
```

o simile, rispettando `na`.

Può aggiungere una barra visuale, ma la barra deve essere DERIVATA.

Esempio:

```text
Overwatch

SPEC             DONE
DATA             DONE
RUNTIME          PARTIAL
LOG/DEBUG        TODO
AUTOMATION       TODO
SCENARIO         PARTIAL
UI/WIKI          DONE
PACKAGED         TODO
NETWORK/PRIVACY  TODO
```

Lo status non deve essere una seconda verità.

---

# 13. Stato e Definition of Done

Mantenere la filosofia esistente del progetto:

una feature non è Done perché "si vede in PIE".

Devono essere rispettati i gate applicabili, inclusi dove richiesto:

- spec;
- runtime;
- log/debug;
- automation;
- scenario;
- UI/Wiki;
- packaged;
- networking/privacy.

La dashboard deve rendere evidente il motivo per cui una Feature non è ancora Done.

---

# 14. Filtri richiesti

Aggiungere almeno:

```text
Release
Milestone
Epic
Area
Priority
Kind
Status
Blocked / not blocked
Editor required
Scenario readiness
Gate missing
```

Ricerca testuale per:

```text
Feature ID
Titolo
Issue number
Scenario ID
Spec
Wiki ref
Editor task
```

---

# 15. Detail panel

Cliccando un elemento non aprire subito una nuova pagina per tutto.

Aprire un detail panel/drawer con:

```text
Summary
Status
Gates
Roadmap
Dependencies
Issues
Scenarios
Tests
Docs
Wiki
Editor Tasks
Last Verified
Notes
```

I link esterni possono poi aprire GitHub.

---

# 16. Warning e validazione

La dashboard deve evidenziare:

- FeatureId duplicati;
- issue reference non valida;
- scenario inesistente;
- wiki ref inesistente;
- dependency non esistente;
- milestone/epic/checkpoint non risolvibile;
- gate inconsistente;
- status superiore ai gate reali;
- Editor Task senza feature/scenario;
- feature bloccata senza blocker dichiarato;
- riferimenti circolari problematici;
- link GitHub non risolvibili.

Non reinventare tutta la validazione già presente in:

```text
scripts/feature_registry.py
```

Riutilizzare o estrarre logica comune.

---

# 17. UI suggerita

Layout principale:

```text
+------------------------------------------------------+
| RefactorTactics Project Control Center              |
| source: main @ <sha>                 Refresh         |
+------------------------------------------------------+
| Overview | Roadmap | Features | Scenarios | Editor  |
+------------------------------------------------------+
| Filters                                              |
+------------------------------------------------------+
|                                                      |
|                  Current view                        |
|                                                      |
+------------------------------------------------------+
```

Overview:

```text
Current release
Current milestone
Features done / total
Gate completion
Open issues
Blocked features
Ready scenarios
Editor blocking tasks
Validation warnings
```

Feature card:

```text
RT-FEAT-REACTION-OVERWATCH

Overwatch
IMPLEMENTING

Gate: 4/8

Epic: E...
Milestone: M...

Issues: 3
Scenarios: 2
Editor: 1

[Wiki] [Spec] [Issues] [Details]
```

---

# 18. Non duplicare Wiki e documentazione

Il dashboard deve collegare la documentazione, non sostituirla.

Sono già presenti documenti come:

```text
docs/wiki/meccaniche/overwatch.md
docs/wiki/meccaniche/coperture.md
docs/wiki/meccaniche/porte.md
docs/wiki/meccaniche/ponti.md
docs/wiki/game/azioni-e-movimento.md
docs/wiki/game/esempio-di-round.md
```

`wiki_refs` e `owner_specs` devono essere linkabili direttamente.

---

# 19. Aggiornamenti richiesti alla documentazione

Dopo avere definito il design:

1. aggiornare `docs/roadmap/feature-registry.md` se lo schema cambia;
2. aggiornare `scripts/feature_registry.py` se servono nuovi campi;
3. aggiungere validator per Editor Task;
4. aggiornare le viste shortlist generate;
5. introdurre la Editor Map derivata;
6. aggiornare la Wiki/documentazione del sistema;
7. aggiornare roadmap e feature map;
8. aggiornare scenario map;
9. aggiungere eventuale `editormap.shortlist.md`;
10. aggiornare `docs/CONTEXT_INDEX.md` se necessario;
11. aggiornare CLAUDE.md/AGENTS.md solo se le nuove regole operative devono diventare permanenti.

---

# 20. Issue / Epic da creare

Dopo avere ispezionato lo stato reale del repository, creare o consolidare gli item GitHub necessari.

Possibile Epic:

```text
Project Control Center / Living Project Maps
```

Issue indicative:

```text
1. Define dashboard data contract
2. Extend Feature Registry for Editor Tasks
3. Add Editor Task validator
4. Add editormap.shortlist generation
5. Implement GitHub registry loader
6. Implement Overview
7. Implement Roadmap view
8. Implement Feature Map view
9. Implement Scenario Map view
10. Implement Editor Map / My Editor Queue
11. Implement detail drawer and relationship navigation
12. Implement GitHub/Wiki/document links
13. Add validation diagnostics UI
14. Add tests for registry parsing and graph resolution
15. Add future PR-based write workflow
```

Prima di creare duplicati, cercare issue/epic esistenti e consolidare.

---

# 21. Test richiesti

Aggiungere test almeno per:

```text
YAML parse
FeatureId uniqueness
Gate/status derivation
Issue link derivation
Wiki/document link derivation
Scenario relation resolution
Dependency resolution
Editor Task references
Missing references
Cycles
Filters
Read-only GitHub source loading
Registry SHA/version display
```

Per il futuro write mode:

```text
fetch SHA
branch creation
file update
validation
derived-file regeneration
PR creation
conflict handling
```

---

# 22. Deliverable iniziale

Prima iterazione:

```text
Project Control Center v0.1
READ ONLY
```

Deve:

- leggere `docs/roadmap/feature-registry.yaml` da GitHub `main`;
- mostrare SHA/last refresh;
- visualizzare Overview;
- visualizzare Feature Map;
- visualizzare Roadmap;
- visualizzare Scenario Map;
- visualizzare Editor Map se lo schema viene introdotto;
- fornire My Editor Queue;
- offrire filtri;
- costruire link verso issue, milestone, Wiki e documenti;
- mostrare gate e motivi di incompletezza;
- evidenziare riferimenti rotti;
- non usare database;
- non duplicare lo stato;
- non scrivere su `main`.

---

# 23. Decisione chiave

Il dashboard NON sostituisce il Feature Registry.

È la sua UI web.

```text
feature-registry.yaml
        |
        +--> validator
        |
        +--> generated markdown/json
        |
        +--> Project Control Center
```

Qualunque nuova informazione strutturata deve essere introdotta nella source of truth o in un file canonico collegato, mai soltanto nella UI.

---

# 24. Passo operativo per Claude

Procedere così:

1. ispezionare `docs/roadmap/feature-registry.yaml`;
2. ispezionare completamente `scripts/feature_registry.py`;
3. ispezionare:
   - `docs/roadmap/feature-registry.md`
   - `docs/roadmap/roadmap-v0.1.md`
   - `docs/roadmap/roadmap-checkpoint.md`
   - `docs/technical/scenario-map.md`
   - `docs/roadmap/*.shortlist.md`;
4. cercare issue/epic esistenti relative a dashboard, maps, registry ed editor workflow;
5. proporre il minimo schema per Editor Task;
6. evitare duplicazioni;
7. implementare validator/migrazione;
8. implementare Project Control Center v0.1 read-only;
9. aggiornare Wiki, docs, roadmap, Feature Map, Scenario Map ed Editor Map;
10. creare/consolidare Epic e issue GitHub;
11. aggiungere test;
12. documentare il futuro write workflow basato su branch + PR;
13. non implementare ancora scrittura diretta su `main`.

## Commit suggeriti

```text
docs(registry): define project control center data contract
feat(registry): add editor task relations and validation
feat(dashboard): add read-only GitHub project control center
feat(dashboard): add roadmap feature scenario and editor views
test(dashboard): validate registry graph and link resolution
docs(project): integrate control center into roadmap and wiki
```
