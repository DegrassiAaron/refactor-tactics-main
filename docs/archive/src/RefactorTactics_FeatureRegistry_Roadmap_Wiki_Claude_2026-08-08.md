> 🔎 **ESITO DELLA REVISIONE — 2026-08-30.** Sorgente **recepito per intero, e poi ritirato**. È il solo
> documento di questo archivio il cui esito non è *«quanto ne è passato»* ma *«quanto ne è sopravvissuto
> alla rimozione di ciò che aveva costruito»*.
>
> ✅ **Recepito**: il Feature Registry canonico è esistito. `docs/roadmap/feature-registry.yaml` (7.621
> righe, 84 feature), `scripts/feature_registry.py` (5.017 righe), otto artefatti generati
> (`feature-registry.json`, `project-graph.json`, `roadmap-map.svg`, cinque `*.shortlist.md`), il Project
> Control Center in `docs/control-center/`, **102** dei 134 test del repository e i **21** blocchi
> `RT_FEATURE_STATUS` che il §12 descrive, generati dentro documenti scritti a mano.
>
> 🔴 **Ritirato il 2026-08-21 da [D-181](../../decisions/RT_PDR_00_Decision_Log.md)**, con la misura che
> l'ha innescata scritta nella decisione: una giornata intera in cui **ogni** commit ha riparato il sistema
> di tracciamento e **nessuno** ha toccato `Source/`. Sono usciti tutti gli artefatti dell'elenco sopra.
> Il §17 — *«validate-feature-registry»*, *«generate-feature-status»* — è caduto poche ore dopo con
> [D-182](../../decisions/RT_PDR_00_Decision_Log.md), che ha portato via l'intera cartella `scripts/`.
>
> ⚠️ **Quindi le sezioni §2, §4, §12, §13, §14, §16, §17 e §21 descrivono cose che non esistono più**, e
> §19–§20 sono il mandato di un lavoro già fatto e già disfatto. Non vanno rieseguite: chi le leggesse come
> istruzioni ricostruirebbe esattamente il sistema che una decisione d'autore ha rimosso. È la stessa
> classe di rischio che `AGENTS.md` chiama *issue che presuppongono tooling rimosso*.
>
> ✅ **Ciò che è sopravvissuto è il vocabolario, ed è la parte più citata del documento.** La tassonomia
> `RT-FEAT-<AREA>-<NOME>` del §3 è ancora in uso: **482** occorrenze in **64** documenti vivi
> (esclusi `docs/archive/` e `docs/research/`) e **10** in **7** file fra `Source/` e `Scenarios/`,
> misurate il 2026-08-30 su `fff33020`. `RT-FEAT-CHAR-PRESENTATION` regge `GBX-5` in
> [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), `RT-FEAT-UI-GRAYBOX-KIT` regge una famiglia di
> [`asset-map.md`](../../technical/tooling/asset-map.md), `RT-FEAT-UI-SCENARIO-BROWSER` regge il verdetto
> di scope del [`README`](README.md) di questa cartella. **Gli ID non hanno più un registro che li
> definisca**: sono nomi condivisi che vivono in prosa, e nessuno verifica più che due documenti intendano
> lo stesso.
>
> ✅ **Sopravvivono anche tre discipline**, che non dipendono dal tooling e valgono ancora:
> §5 *«niente percentuali soggettive: gate contati, non stime»*; §7 *«una feature non è una epic — la
> roadmap è la vista temporale, la feature è la vista di capability»*; §18 *«non aggiornare una roadmap
> storica per farla sembrare corrente: mettici un banner»*, che è il meccanismo con cui questo archivio
> funziona.
>
> ⚠️ **Il §9 portava una baseline che è invecchiata due volte.** Il roster è cambiato con
> [D-120](../../decisions/RT_PDR_00_Decision_Log.md)/[D-130](../../decisions/RT_PDR_00_Decision_Log.md) e i
> nomi sono stati sostituiti in questo file (riga di provenienza qui sotto); e *«Fast Reaction 3,0 s,
> timeout HOLD»*, *«High Ground senza bonus alla vista»*, *«formato competitivo non bloccato»* vanno
> riletti su `CLAUDE.md` §3, non da qui.

<!-- rename-exempt: la riga dichiara la mappatura -->
> **Nomi del roster sostituiti il 2026-08-30 per [D-130](../../decisions/RT_PDR_00_Decision_Log.md)**:
> `Flux` → `Gadget`, `Riva` → `Phase`, `Bastion` → `Riktor`, `Vektor` → `Wraith`, secondo la mappatura di
> [D-037](../../decisions/RT_PDR_00_Decision_Log.md). Il documento del 2026-08-08 usava i nomi di allora;
> le issue e i commit dell'epoca li usano ancora, ed è ciò che va cercato per risalire alla provenienza.

---

# RefactorTactics — Feature Registry, Roadmap e Wiki Traceability
## Handoff operativo per Claude Code

**Data:** 2026-08-08  
**Scopo:** creare una singola catena di tracciabilità `Feature -> Roadmap -> Issue/Test/Scenario -> Wiki`, evitando stato duplicato e drift documentale.

---

# 1. Obiettivo

RefactorTactics deve avere una **lista canonica delle feature** che sia anche l'indice usato da:

- roadmap;
- GitHub Issues / Epic;
- scenari di test;
- test automatici / PIE;
- documentazione;
- Wiki / GitHub Wiki;
- workbook usati per generare la Wiki.

Ogni feature mostrata o linkata nella Wiki deve poter rispondere immediatamente a:

1. **Che feature è?**
2. **In quale release/milestone è prevista?**
3. **Quale Epic/checkpoint la implementa?**
4. **Qual è lo stato verificato?**
5. **Quali gate mancano?**
6. **Quali issue la realizzano?**
7. **Quali test/scenari la dimostrano?**
8. **Dove si trova la specifica owner?**

La Wiki NON deve diventare una seconda roadmap.

---

# 2. Prima regola: una sola fonte di verità per lo stato

Creare o consolidare un **Feature Registry canonico**.

Percorso raccomandato, salvo struttura equivalente già esistente:

```text
docs/roadmap/feature-registry.yaml
```

Se il repository usa già JSON, CSV o un altro formato machine-readable adatto, riusarlo.

NON creare un secondo registry se esiste già un equivalente.

Il registry possiede:

```text
FeatureId
Title
Area
Kind
Release
Priority
Status
Gates
RoadmapRef
Dependencies
OwnerSpecs
Issues
Tests
Scenarios
WikiRefs
LastVerified
```

Roadmap, Wiki e workbook devono **referenziare il FeatureId**, non copiare manualmente lo stato.

---

# 3. Modello di Feature ID

Usare ID stabili, leggibili e indipendenti dal titolo visuale.

Esempi:

```text
RT-FEAT-CORE-TURN
RT-FEAT-CORE-DETERMINISM
RT-FEAT-MAP-HEXGRAPH
RT-FEAT-MAP-COVER
RT-FEAT-REACTION-OVERWATCH
RT-FEAT-PERCEPTION-NOISE
RT-FEAT-UI-ACTION-GHOSTS
RT-FEAT-TEST-SCENARIO-HARNESS
RT-FEAT-CHAR-TRANSFORMATION
RT-FEAT-FACTION-SYSTEM
```

Non usare il numero di una issue come FeatureId.

Le issue possono cambiare o essere spezzate; il FeatureId deve restare stabile.

---

# 4. Schema raccomandato

Esempio concettuale:

```yaml
- feature_id: RT-FEAT-REACTION-OVERWATCH
  title: Overwatch
  area: Reactions
  kind: gameplay
  release: v0.1
  priority: P0

  status: TESTABLE

  gates:
    spec: done
    runtime: done
    data: done
    log_debug: done
    automation: done
    scenario: partial
    ui_wiki: partial
    packaged: todo
    network_privacy: na

  roadmap:
    epic: E14
    checkpoint: null

  dependencies:
    - RT-FEAT-REACTION-OPPORTUNITY
    - RT-FEAT-MAP-FACING
    - RT-FEAT-MAP-LOS

  owner_specs:
    - docs/gameplay/brief-overwatch-reazioni.md

  issues: []
  tests: []
  scenarios: []
  wiki_refs:
    - game/overwatch.md

  last_verified:
    date: 2026-08-08
    commit: "<HEAD verificato>"
```

Lo schema deve essere adattato alla struttura reale del repository.

---

# 5. NON usare percentuali soggettive

Non scrivere manualmente:

```text
Overwatch = 73%
```

Preferire progressi derivati da gate verificabili:

```text
6 / 8 gate completati
Status: TESTABLE
```

Possibili stati:

```text
IDEA
DESIGNED
SPECIFIED
IMPLEMENTING
TESTABLE
INTEGRATED
RELEASE_READY
DONE
DEFERRED
BLOCKED
```

`DONE` deve rispettare la Definition of Done reale del progetto.

Per feature che non richiedono networking nella release corrente, il gate può essere `na`, non falsamente `done`.

---

# 6. Gate minimi

Usare un set stabile di gate, con eventuali estensioni per dominio:

| Gate | Significato |
|---|---|
| `spec` | Regole e ownership documentate |
| `data` | Cataloghi/Data Asset/config coerenti |
| `runtime` | Implementazione gameplay presente |
| `log_debug` | TurnLog/reason/debug disponibili |
| `automation` | Test automatici pertinenti verdi |
| `scenario` | Scenario visuale/regression disponibile |
| `ui_wiki` | UI e documentazione utente collegate |
| `packaged` | Verificata in build packaged |
| `network_privacy` | Solo quando la feature deve funzionare online |

Per una feature di gameplay competitiva online, `DONE` richiede anche privacy/rete corretta.

---

# 7. Roadmap: separare FEATURE da EPIC/CHECKPOINT

Non trasformare il Feature Registry in una roadmap lineare.

Modello:

```text
Feature Registry
    |
    +--> Roadmap Epic / Milestone
    |       |
    |       +--> checkpoint
    |       +--> issue
    |
    +--> test
    +--> scenario
    +--> wiki
```

Una feature può attraversare più checkpoint.

Una Epic può implementare più feature.

La roadmap resta la vista temporale/esecutiva.

Il Feature Registry resta la vista di prodotto/capability.

---

# 8. Stato corrente della documentazione da correggere

Durante l'audit verificare almeno:

```text
docs/roadmap/roadmap-checkpoint.md
docs/roadmap/roadmap-v0.1.md
docs/roadmap/v0.1-definition-of-done.md
docs/roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md
docs/roadmap/v0.1-issue-plan.md
```

Obiettivo:

```text
roadmap-checkpoint.md
    = stato operativo corrente

roadmap-v0.1.md
    = scope/gate della release

PDR roadmap
    = north-star / requisiti di lungo periodo

issue-plan
    = snapshot/generated, non source of truth
```

Rimuovere snapshot duplicati e contraddittori.

---

# 9. Baseline corrente da rispettare

Prima di aggiornare tutto, verificare repository, `AGENTS.md`, `CLAUDE.md`, piano canonico, ADR, Decision Log, cataloghi e test.

Baseline conosciuta da verificare contro HEAD:

```text
UE 5.8.1

v0.1:
2v2 offline vs bot

Roster v0.1:
Gadget
Phase
Riktor
Wraith

Roster v0.2:
Steel
Aurora
Murdock
Kwang

Round:
Planning
→ Prep
→ Dash
→ Blast
→ Move
→ Cleanup

Move normale:
ultima fase volontaria

Fast Reaction:
3.0 s
timeout HOLD

Wraith:
InterceptShot = predictive thin slice v0.1

High Ground v0.1:
nessun bonus numerico automatico alla vista

Formato competitivo finale:
non ancora bloccato
3v3 = baseline di studio
4v4 = stress test
```

Non reintrodurre vecchi roster o vecchi interrupt da 5 secondi.

---

# 10. Initial Feature Map da consolidare

Questa è una **seed list** da confrontare con codice, roadmap, issue e docs.

Non dichiarare automaticamente ogni voce `DONE`.

## A. Core simulation

```text
RT-FEAT-CORE-TURN
Turn pipeline simultanea

RT-FEAT-CORE-DETERMINISM
Snapshot / resolver deterministico

RT-FEAT-CORE-TURNLOG
TurnLog, reason codes, hash/replay

RT-FEAT-CORE-DECISION-BOUNDARY
Resolution segmentata con Decision Boundary
```

## B. Map / Spatial

```text
RT-FEAT-MAP-HEXGRAPH
FRTCellId + grafo esagonale multilivello

RT-FEAT-MAP-PATHFINDING
A* autorevole

RT-FEAT-MAP-LOS
LOS / targeting / trajectory separati

RT-FEAT-MAP-FACING
Facing gameplay autorevole

RT-FEAT-MAP-COVER
Cover direzionale

RT-FEAT-MAP-DYNAMIC-COVER
Cover modificabile / Open-Fire-Seal

RT-FEAT-MAP-INTERACTIVE-EDGES
Muri, porte, archi e trigger

RT-FEAT-MAP-SPECIAL-TRANSITIONS
Ponti, tunnel, ascensori, transizioni multilivello
```

## C. Action economy / planning

```text
RT-FEAT-ACTION-GENERIC
Wait / Move / BasicAttack / Guard / Brace / Interact / Overwatch

RT-FEAT-ACTION-MOVE-PROFILES
Sneak / Move / Sprint dove previsti dal canone

RT-FEAT-ACTION-DASH-DISPLACEMENT
Dash e forced movement separati dal Move normale

RT-FEAT-ACTION-DELAYED
Delayed Action

RT-FEAT-ACTION-PREDICTIVE
Predictive Action thin slice

RT-FEAT-ACTION-TRAPS
Traps / Tactical Gambits

RT-FEAT-ACTION-COOLDOWNS
Cooldown model

RT-FEAT-ACTION-SUPERS
Super / high-commitment abilities
```

## D. Reactions

```text
RT-FEAT-REACTION-OPPORTUNITY
Opportunity -> Commit model

RT-FEAT-REACTION-FAST
Fast Reaction bounded decision window

RT-FEAT-REACTION-OVERWATCH
Overwatch universale profilabile

RT-FEAT-REACTION-FAST-ACTION
Fast Action come continuazione limitata di una propria azione
```

## E. Environment

```text
RT-FEAT-ENV-WATER
Wet / water

RT-FEAT-ENV-ELECTRIC
Electricity / conductive interaction

RT-FEAT-ENV-FIRE
Fire

RT-FEAT-ENV-STEAM
Steam / visual cover

RT-FEAT-ENV-ICE
Ice / friction / slip rules

RT-FEAT-ENV-SYSTEMIC-COMBOS
Interazioni producer/consumer senza hard-code tra eroi
```

## F. Perception / information

```text
RT-FEAT-PERCEPTION-TEAM-KNOWLEDGE
TeamKnowledge / partial information

RT-FEAT-PERCEPTION-VISION
Vision + facing + awareness

RT-FEAT-PERCEPTION-NOISE
Noise / acoustic perception

RT-FEAT-PERCEPTION-MEMORY
Last known / heard knowledge
```

## G. UI / coordination

```text
RT-FEAT-UI-TACTICAL-CAMERA
Camera tattica

RT-FEAT-UI-PLANNING
Planning HUD

RT-FEAT-UI-ACTION-GHOSTS
Action Ghosts

RT-FEAT-UI-CERTAINTY
Confermato / Previsto / Incerto

RT-FEAT-UI-WARNINGS
Collision/friendly-fire/resource/uncertainty warnings

RT-FEAT-UI-COMBAT-LOG
Combat log / explainability

RT-FEAT-UI-SCENARIO-BROWSER
Scenario selector/browser
```

## H. Characters / content

```text
RT-FEAT-CHAR-V01-ROSTER
Gadget / Phase / Riktor / Wraith

RT-FEAT-CHAR-V02-ROSTER
Steel / Aurora / Murdock / Kwang

RT-FEAT-CHAR-TRANSFORMATION
Character State / Stance / Form / Overdrive framework

RT-FEAT-CHAR-AUXILIARY-UNITS
Pet / summon / gadget units
```

## I. Factions / identity

```text
RT-FEAT-FACTION-SYSTEM
Fazioni, identità, colori, iconografia, membership

RT-FEAT-FACTION-SCENARIOS
Scenari di cooperazione collegati dalla Wiki
```

Le fazioni NON devono creare bonus impliciti di coppia/fazione salvo decisione esplicita.

Le sinergie devono preferire sistemi comuni:

```text
Wet
Electric
Cover
Facing
Noise
Surface
Transition
Status
Event
```

## J. Objectives / match

```text
RT-FEAT-OBJECTIVE-SYSTEM
Objective System

RT-FEAT-MATCH-PACING
Turn/round/match pacing

RT-FEAT-STRESS-4V4
4v4 stress validation
```

## K. Bot / test / tooling

```text
RT-FEAT-BOT-BASE
Bot deterministic/basic utility

RT-FEAT-BOT-TACTICAL
Bot con TeamKnowledge, scoring e reaction policy

RT-FEAT-TEST-SCENARIO-HARNESS
Automated Scenario Test Harness

RT-FEAT-TEST-GOLDEN
Golden scenario / deterministic repeat

RT-FEAT-TOOL-SCENARIO-SELECTOR
BP_GameMode / RTTestDirector scenario selection

RT-FEAT-TOOL-MAP-EDITOR
Map editor / graybox authoring

RT-FEAT-TOOL-BALANCE-GROUND
Skill/balance testing ground

RT-FEAT-TOOL-VALIDATION
Data validators / scenario validators
```

## L. Data / production infrastructure

```text
RT-FEAT-DATA-STABLE-IDS
Stable IDs + versions

RT-FEAT-DATA-ASSET-PIPELINE
Primary Data Assets / catalogs

RT-FEAT-DATA-HASH
Rules/content manifest/hash

RT-FEAT-NET-PRIVATE-PLANNING
Team-only intents

RT-FEAT-NET-AUTHORITY
Server authoritative multiplayer

RT-FEAT-NET-DEDICATED
Dedicated server

RT-FEAT-PROD-PERFORMANCE
Performance budgets / profiling

RT-FEAT-PROD-PACKAGED
Packaged release verification
```

Networking deve essere schedulato in base al canone di release reale; non forzarlo dentro v0.1 offline.

---

# 11. Roadmap proposta: vista per release

Non inventare date.

Usare dipendenze e gate.

## R0 — Governance & Traceability

Obiettivo:

```text
Feature Registry canonico
+
roadmap senza snapshot contraddittori
+
Wiki cross-reference
+
validator
```

Exit gate:

- ogni feature Wiki possiede FeatureId valido;
- nessun FeatureId duplicato;
- roadmap refs validi;
- status derivabile dai gate;
- workbook non duplica lo stato.

## v0.1 — Core Closure

Consolidare/chiudere:

- turno canonico;
- deterministic resolver;
- TurnLog;
- hex graph;
- pathfinding;
- LOS;
- cover;
- environment v0.1;
- generic actions;
- facing;
- TeamKnowledge minimo;
- Reaction Opportunity;
- Fast Reaction;
- Overwatch;
- Wraith predictive thin slice;
- v0.1 roster.

## v0.1 — Showcase Integration

Integrare:

- objective;
- Action Ghost;
- certainty UI;
- warning;
- bot;
- scenario harness;
- full showcase scripted/visual;
- open/modify/use cover dove realmente in scope;
- release PIE subset.

## v0.1 — Release Hardening

Gate:

- deterministic repeat;
- no divergence;
- scenario assertions;
- packaged offline build;
- performance budgets pertinenti;
- docs/wiki allineate;
- nessuna feature dichiarata Done solo perché "visibile in PIE".

## v0.2 — Content & System Expansion

Pianificare con dipendenze esplicite:

- Steel / Aurora / Murdock / Kwang;
- nuove fazioni;
- faction icons / identity;
- faction scenarios;
- supers;
- cooldown variants;
- Character State / Transformation framework;
- richer noise/acoustic perception;
- auxiliary units;
- richer traps/predictive actions;
- scenario browser/tooling;
- expanded map interactions;
- 4v4 stress validation.

Non trasformare automaticamente ogni idea v0.2 in feature committed.

Usare `DESIGNED`, `SPECIFIED` o `DEFERRED` dove appropriato.

## Successivo

- multiplayer authority;
- team-only network planning;
- privacy canary tests;
- dedicated server;
- reconnect;
- telemetry;
- release hardening;
- modding solo quando entra realmente in scope.
```

---

# 12. Wiki integration

Ogni pagina che presenta una feature deve contenere un blocco generato.

Esempio:

```markdown
<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-REACTION-OVERWATCH -->

> **Roadmap**
> Feature: `RT-FEAT-REACTION-OVERWATCH`
> Release: `v0.1`
> Epic: `E14`
> Status: `TESTABLE`
> Progress: `6/8 gate`
> Scenario: `Reaction.Overwatch.HoldThenFire`
> Last verified: `<commit>`

<!-- RT_FEATURE_STATUS:END RT-FEAT-REACTION-OVERWATCH -->
```

Il blocco deve essere generato/aggiornato dal registry.

NON editarlo manualmente.

Una pagina può avere più FeatureId.

Esempio pagina personaggio Gadget:

```text
RT-FEAT-CHAR-V01-ROSTER
RT-FEAT-ENV-ELECTRIC
RT-FEAT-ENV-SYSTEMIC-COMBOS
```

Non creare una roadmap item per ogni riga di lore o per ogni valore numerico.

---

# 13. Pagina Wiki centrale

Creare o aggiornare una pagina tipo:

```text
Roadmap
Feature Status
Development Status
```

La pagina deve mostrare almeno:

| Feature | Release | Epic | Status | Gates | Scenario |
|---|---|---|---|---:|---|

Filtri/sezioni:

```text
v0.1
v0.2
Future

Gameplay
Map
Environment
Reactions
Perception
Characters
UI
Tools
Networking
```

Questa pagina deve essere una vista del Feature Registry.

---

# 14. Character/Faction Wiki workbook

Il workbook corrente di character authoring deve referenziare i FeatureId senza duplicare lo stato.

Aggiungere, se coerente con la struttura esistente:

```text
FeatureIds
RoadmapFeatureIds
DemonstratedFeatureIds
```

oppure una sheet relazionale:

```text
Wiki_Feature_Refs
```

Esempio:

| WikiEntityId | FeatureId | Relation |
|---|---|---|
| Hero.Gadget | RT-FEAT-CHAR-V01-ROSTER | release |
| Hero.Gadget | RT-FEAT-ENV-ELECTRIC | demonstrates |
| Faction.Conflux | RT-FEAT-FACTION-SYSTEM | belongs-to |
| Scenario.Team.Conflux... | RT-FEAT-FACTION-SCENARIOS | validates |

NON aggiungere nel workbook colonne manuali:

```text
FeatureStatus = 70%
```

Lo stato viene letto dal registry.

---

# 15. Scenari come prova della feature

Ogni feature che può essere mostrata deve poter referenziare uno o più ScenarioId.

Esempi:

```text
RT-FEAT-REACTION-OVERWATCH
    -> Reaction.Overwatch.HoldThenFire

RT-FEAT-ENV-SYSTEMIC-COMBOS
    -> Environment.WaterElectric.Basic

RT-FEAT-MAP-DYNAMIC-COVER
    -> Coordination.Cover.OpenFireSeal

RT-FEAT-FACTION-SCENARIOS
    -> Team.<Faction>.<Scenario>

RT-FEAT-STRESS-4V4
    -> Stress.4v4.CoreRoster
```

La Wiki deve poter offrire:

```text
"Vedi scenario giocabile"
```

senza duplicare setup/valori competitivi.

---

# 16. Validator obbligatorio

Creare o estendere il validator documentale/tooling.

Errori:

```text
FeatureId duplicato
Wiki FeatureId inesistente
RoadmapRef inesistente
Epic/checkpoint inesistente
ScenarioId inesistente
owner spec inesistente
release non valida
status non valido
DONE con gate obbligatorio non completato
last_verified assente per status >= TESTABLE
```

Warning:

```text
feature senza WikiRefs
feature senza scenario quando è marcata demonstrable
feature SPECIFIED senza issue/roadmap assignment
wiki page con feature refs obsolete
```

---

# 17. CI / verifica locale

Se esiste infrastruttura script, aggiungere un comando simile a:

```text
validate-feature-registry
validate-wiki-feature-links
generate-feature-status
```

Usare stack già presente nel repository.

Non introdurre una nuova runtime dependency pesante solo per questo.

Output machine-readable preferito.

---

# 18. Regola sui documenti storici

Non aggiornare roadmap storiche per farle sembrare correnti.

Usare banner:

```text
HISTORICAL / DELIVERED
Current owner: <roadmap corrente>
```

Lo stato giornaliero vive solo in:

```text
Feature Registry
roadmap-checkpoint
issue tracker
```

---

# 19. Migrazione richiesta

Claude deve:

1. fare audit del repo;
2. costruire matrice `feature -> docs -> code -> tests -> scenarios -> wiki -> roadmap`;
3. deduplicare le feature equivalenti;
4. assegnare Stable FeatureId;
5. classificare ogni feature:
   - current v0.1;
   - committed v0.2;
   - proposed;
   - future/deferred;
6. NON promuovere una idea a committed senza fonte;
7. creare/aggiornare il registry;
8. riscrivere la sezione current delle roadmap;
9. aggiornare DoD;
10. integrare Wiki;
11. integrare workbook/generator se usato;
12. aggiungere validator;
13. produrre report finale.

---

# 20. Output finale richiesto a Claude

## A. Feature audit

Tabella:

```text
FeatureId
Title
Release
Current Status
Roadmap Ref
Owner Spec
Code
Tests
Scenarios
Wiki
Conflicts
```

## B. Roadmap

```text
Milestone
Epic
FeatureIds
Dependencies
Exit Gate
Status
```

## C. Wiki

Elencare:

```text
Pagine aggiornate
FeatureId aggiunti
Blocchi status generati
Link roadmap
Link scenari
```

## D. Workbook

Elencare:

```text
Sheet/colonne modificate
FeatureId aggiunti
Nessuno stato duplicato
```

## E. Validator

Mostrare:

```text
command
numero feature
refs wiki
refs roadmap
refs scenario
errori
warning
```

## F. Conflitti

Non risolvere silenziosamente.

Esempio:

```text
Feature
Source A
Source B
Decision required
Impact
```

## G. Git

Commit focalizzati, per esempio:

```text
docs(roadmap): add canonical feature registry
docs(roadmap): consolidate current v0.1 status
docs(wiki): link pages to feature roadmap status
tools(docs): validate feature and wiki references
docs(v0.2): map planned systems to stable feature ids
```

---

# 21. Definition of Done di questa attività

L'attività è chiusa quando:

- esiste un solo Feature Registry canonico;
- ogni feature Wiki usa un FeatureId valido;
- ogni feature committed ha RoadmapRef;
- la Wiki mostra status e gate senza mantenerli manualmente;
- roadmap-checkpoint non contiene snapshot concorrenti;
- roadmap-v0.1 non contiene vecchie tabelle che contraddicono la current view;
- v0.2 è distinta da idee future;
- scenari possono referenziare le feature che dimostrano;
- workbook Wiki usa FeatureId, non copia lo stato;
- validator rileva riferimenti rotti;
- il report finale indica HEAD verificato;
- nessuna feature è dichiarata Done senza i gate richiesti.

---

# 22. Principio finale

La catena deve essere:

```text
FEATURE ID
   |
   +--> SPEC
   +--> ROADMAP
   +--> ISSUE
   +--> CODE
   +--> TEST
   +--> SCENARIO
   +--> WIKI
```

Lo stato si aggiorna **una volta sola**.

Tutte le altre viste lo leggono.

Questo deve impedire che fra sei settimane la Wiki dica "implementato", la roadmap dica "da fare" e il codice abbia già una terza realtà.
