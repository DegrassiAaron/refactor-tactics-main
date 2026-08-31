# PROMPT PER CLAUDE CODE — REFACTORTACTICS
## Consolidamento roadmap Tactical Designer: Skill Authoring + Action Lab + Scenario Validation, TD 0.1 → TD 1.0

Repository atteso:

`DegrassiAaron/refactor-tactics-main`

Agisci come **technical product owner + Unreal gameplay engineer + maintainer GitHub**.

Questo task riguarda il loop di sviluppo e validazione delle skill dentro il **Tactical Designer**:

```text
Character
  ↓
Skill / Action
  ↓
Action Lab
  ↓
Scenario Draft
  ↓
Scenario Harness
  ↓
Resolver canonico
  ↓
TurnLog / Replay / StateHash
  ↓
Scenario salvabile
  ↓
Regression / Compare / Promotion
```

L'obiettivo NON è creare un nuovo skill runtime né una nuova roadmap parallela.

---

# 1. REGOLE NON NEGOZIABILI

## 1.1 Tactical Designer è l'owner

Verifica come primo riferimento:

- `#1105` — `[EPIC] Tactical Designer`
- `docs/technical/spec-tactical-designer.md`
- roadmap/checkpoint reali del repository
- eventuali handoff Tactical Designer già consolidati

Il Tactical Designer resta tooling trasversale.

NON creare:

- una nuova epic `E48`;
- una seconda roadmap release del gioco;
- una nuova numerazione che confonda `TD 0.x` con `RefactorTactics v0.x`.

Preserva:

```text
ROADMAP GAME
v0.1 → ... → v1.0

        ↑ supportata da

TACTICAL DESIGNER
TD 0.1 → ... → TD 1.0
```

---

## 1.2 Misura prima di creare issue

Prima di modificare GitHub o roadmap:

```bash
git status
git branch --show-current
git fetch --all --prune
git rev-parse HEAD
git rev-parse origin/main
git log -10 --oneline --decorate
git worktree list
```

Poi cerca issue OPEN e CLOSED per concetto, non solo per titolo.

Controlla almeno:

- `#1105` — Tactical Designer
- `#1114`
- `#1115`
- `#1116`
- `#1117`
- `#1625` — playback visuale
- `#1626` — intent combattimento
- `#1627` — multi-turn
- `#1628` — FIRE/HOLD
- `#1629` — status iniziali
- `#1630` — State Diff
- `#711` — runtime probes movimento
- `#695` — visualizzazione/cover-door ecosystem, se ancora pertinente
- `#1657` — possibile divergenza asset `L_DevSandbox`

Cerca anche:

- Action Lab
- Skill Workbench
- skill editor
- ability editor
- ActionData / ActionDef
- Scenario Composer
- Scenario Harness
- Scenario Draft
- Run / Reset
- StateHash
- TurnLog
- replay
- baseline / candidate
- variant
- scenario save
- Paragon / Gadget / Plasma Blast
- Arc Pulse

REUSE / UPDATE prima di CREATE.

---

# 2. ARCHITETTURA DA PRESERVARE

Non introdurre un secondo runtime.

Il principio è:

```text
Editor / Tactical Designer
        ↓
dati canonici / draft
        ↓
runtime services canonici
        ↓
Scenario Harness
        ↓
resolver
        ↓
TurnLog / Replay / StateHash
```

Mai:

```text
Action Lab
  → resolver proprio
  → formula damage propria
  → targeting proprio
  → pathfinder proprio
```

La logica competitiva deve rimanere runtime/core.

L'Editor possiede:

- controlli;
- gesture;
- form;
- preview;
- selection;
- readout;
- Run / Reset;
- visualizzazione risultato.

Il runtime possiede:

- legality;
- targeting;
- LOS;
- damage;
- status;
- displacement;
- scenario validation;
- scenario execution;
- TurnLog;
- StateHash;
- reason codes.

Prima di introdurre nuovi tipi ability/action, verifica il sistema attuale.
Se esistono già `FRTActionDef`, `URTActionData`, effect specs o equivalenti, ESTENDI quelli.
NON creare un secondo `URTAbilityDefinition` senza una decisione architetturale esplicita.

---

# 3. OBIETTIVO DEL PRIMO SLICE

Vogliamo un loop minimo realmente usabile:

```text
Open Tactical Designer
        ↓
Action Lab
        ↓
Select Map
Select Character
Select Skill
Select Target / Context
        ↓
PLAY
        ↓
Reset initial state
Fixed seed
Scenario Draft
Scenario Harness
Resolver
TurnLog
Playback
        ↓
STOP
```

Poi:

```text
PLAY di nuovo
→ stesso initial state
→ stesso seed
→ stesso risultato
→ stesso StateHash
```

E deve essere possibile validare la stessa skill in uno scenario canonico.

Golden Skill iniziale candidata:

```text
Gameplay action:
Flux.ArcPulse
```

Player-facing / presentation target corrente da verificare:

```text
Character:
Gadget

Paragon presentation source:
Gadget — Plasma Blast
```

IMPORTANTE:
non rinominare Stable ID in questo task senza migration owner.

La relazione prevista è:

```text
Arc Pulse / runtime gameplay
        +
Plasma Blast / presentation asset candidate
```

L'animazione/VFX NON decide l'esito.

---

# 4. ROADMAP DI MATURITÀ DA RICONCILIARE

Questi sono TITOLI CANDIDATI.

Non creare automaticamente dieci nuove epic.
Prima mappa ciascun titolo contro #1105, roadmap, issue e capability esistenti.

Se la governance reale preferisce una sola epic #1105 con checkpoint/sub-issue, usa quella.
Se esistono sub-epic equivalenti, aggiornale.

## TD 0.1 — Golden Skill Execution & Scenario Validation Foundation

Obiettivo:
chiudere un primo loop Character → Skill → Action Lab → Scenario → TurnLog deterministico usando il runtime esistente.

Questo è il focus operativo immediato.

## TD 0.2 — Visual Scenario Authoring for Skill Tests

Obiettivo:
creare e modificare scenari di skill senza JSON/manual data editing, con placement, action selection, target/context ed expectations.

Deve convergere con Scenario Composer, non duplicarlo.

## TD 0.3 — Skill Workbench MVP

Obiettivo:
configurare candidate/variant di una skill senza modificare production data.

Capacità:
- candidate definition;
- override/profiles;
- geometry probe;
- baseline vs candidate;
- validation.

## TD 0.4 — Integrated Skill + Scenario Experiment Loop

Obiettivo:
legare candidate skill a scenari, eseguire baseline/candidate e leggere diff riproducibili.

Capacità:
- candidate binding;
- scenario diff;
- experiment provenance;
- affected scenario hints dove già supportati.

## TD 0.5 — Skill Explainability & Tactical Probes

Obiettivo:
spiegare usando dati runtime perché una skill è legale/illegale e perché produce un risultato.

Capacità:
- range;
- target;
- LOS;
- cover;
- facing;
- path/cost se applicabile;
- displacement;
- reaction;
- environment;
- reason codes.

Riusare probes/runtime owner esistenti.

## TD 0.6 — Replay-to-Scenario Skill Authoring

Obiettivo:
trasformare una sessione interessante o un replay in uno scenario editabile e ripetibile.

Capacità:
- capture;
- replay selection;
- convert to scenario;
- clean initial state;
- preserve stable IDs;
- deterministic rerun.

## TD 0.7 — Batch Skill Comparison & Balance Analytics

Obiettivo:
confrontare candidate/variant su batch di scenari usando metriche derivate dal TurnLog.

Capacità:
- batch execution;
- baseline/candidate compare;
- matchup/scenario grouping;
- explainable metrics;
- heatmaps solo se supportate dagli eventi runtime.

Non creare un balance oracle.

## TD 0.8 — Skill Regression Matrix & Impact Analysis

Obiettivo:
sapere quali scenari verificano una skill/modifier e quali regressioni sono attese o inattese.

Capacità:
- scenario matrix;
- suite selection;
- dependency/impact mapping;
- stale baseline detection;
- expected-change workflow.

## TD 0.9 — Production Skill Promotion & Governance

Obiettivo:
promuovere una candidate a production in modo esplicito, validato e auditabile.

Capacità:
- provenance;
- validation gates;
- diff;
- schema/version checks;
- broken reference diagnostics;
- migration policy;
- promotion gate.

## TD 1.0 — Production Skill Design & Validation Environment

Obiettivo finale:

```text
Map
→ Character
→ Skill
→ Candidate
→ Action Lab
→ Scenario
→ Replay
→ Compare
→ Regression Suite
→ Promote
```

Una nuova persona deve poter completare il ciclo senza creare runtime paralleli e senza dover leggere il resolver per capire il workflow.

---

# 5. ISSUE CANDIDATE PER IL PRIMO SLICE TD 0.1

NON usare questi titoli come comando di creazione cieca.

Per ognuno:

1. cerca issue semanticamente equivalente;
2. se esiste OPEN → UPDATE/LINK;
3. se esiste CLOSED e capability è presente → REUSE;
4. crea una nuova issue solo per un gap reale.

## TD01-A — `[TD] Launcher: selezione Scenario / Action Lab senza biforcare il runtime`

Scope:
- entry point unico Tactical Designer;
- modalità `Scenario`;
- modalità `Action Lab`;
- dati canonici;
- errori di caricamento leggibili.

Out of scope:
- Skill Workbench completo;
- batch simulation;
- networking;
- visual scripting.

Acceptance candidate:
- launcher unico;
- nessun secondo resolver;
- test editor dove ragionevole.

---

## TD01-B — `[TD] Action Lab: esecuzione canonica Hero + Action + Context tramite Scenario Harness`

Flusso target:

```text
Map + Hero + Action + Target/Context
          ↓
FRTScenarioDraft o tipo corrente equivalente
          ↓
URTScenarioAuthoring o facade corrente equivalente
          ↓
Scenario Harness
          ↓
Resolver
          ↓
TurnLog
```

Acceptance:
- usa facade/runtime canonici;
- non duplica legality/targeting/damage;
- Run produce risultato osservabile;
- failure reason leggibile.

---

## TD01-C — `[TD] Action Lab: Reset deterministico, seed fisso e StateHash repeat`

Acceptance:
- PLAY parte da stato iniziale pulito;
- secondo PLAY riparte dallo stesso stato;
- stesso seed;
- stesso scenario draft;
- stesso risultato;
- stesso StateHash;
- nessun residuo del run precedente.

---

## TD01-D — `[TD] Action Lab: controlli Target / Context e validazione canonica`

Minimo utile:
- target unit/cell dove supportato;
- start cell;
- target cell;
- HP;
- Shield;
- Armor/Resistance SOLO se già nel runtime corrente;
- status iniziali se già supportati;
- seed;
- reason code / legality.

Non implementare campi simulati che il runtime non possiede.

---

## TD01-E — `[TD] Golden Skill: consolidare Arc Pulse come primo contratto end-to-end`

Candidate baseline da verificare contro gli owner correnti:

```text
Action ID:
Flux.ArcPulse

Character presentation:
Gadget

Paragon presentation source:
Plasma Blast

Role:
Basic ranged attack
```

Verifica numeri e schema attuali nel repository prima di modificarli.

Obiettivo:
- una skill/action canonica;
- definizione data-driven;
- resolver reale;
- TurnLog;
- Action Lab;
- scenario;
- automation.

Non creare codice speciale `if Gadget`.

---

## TD01-F — `[TD] Scenario: SCN_Gadget_ArcPulse_Baseline`

Creare SOLO se non esiste scenario equivalente.

Scenario minimo:
- Gadget;
- target dummy/enemy;
- posizione fissa;
- range valido;
- LOS valido;
- cover baseline esplicita;
- fixed seed;
- action Arc Pulse;
- hard expectation sul risultato compatibile con il runtime corrente.

Acceptance:
- headless/automation dove supportato;
- Editor run;
- repeat deterministico;
- TurnLog verificabile.

---

## TD01-G — `[TD] Scenario: Arc Pulse tactical range/cover choice`

Secondo scenario, più tattico.

Obiettivo:
non testare solo "fa danno", ma l'integrazione con:
- range;
- LOS;
- cover;
- target legality;
- resolver.

Se cover/damage model corrente non è pronto o l'owner dice post-slice:
defer e registra il motivo.

---

## TD01-H — `[TD] Action Lab: Save current setup as editable Scenario Draft`

Obiettivo:
trasformare un test interessante dell'Action Lab in scenario editabile senza riscriverlo a mano.

Acceptance:
- preserva Stable ID;
- preserva initial state necessario;
- preserva seed;
- non salva stato transient del run;
- apre/riusa Scenario Composer;
- il scenario salvato rerunna deterministicamente.

Se questa capability è troppo grande per TD 0.1:
spostala a TD 0.2, ma documenta esplicitamente il defer.

---

## TD01-I — `[TD] Golden Skill: TurnLog, automation e Editor/headless parity`

Obiettivo:
fare della Golden Skill un regression anchor.

Acceptance:
- automation test;
- Scenario Harness result;
- TurnLog;
- StateHash;
- Editor/headless parity;
- nessun test falso verde;
- documentare NOT RUN quando applicabile.

---

## TD01-J — `[TD] Paragon presentation audit: Gadget Plasma Blast animation/VFX binding`

Obiettivo:
censire asset reali del pack Paragon Gadget per il primo attacco.

Cercare:
- AnimSequence;
- AnimMontage;
- Animation Blueprint;
- projectile/muzzle/impact FX;
- audio se presente;
- skeletal/static meshes correlate.

Output:
- asset path verificati;
- `VERIFIED / PARTIAL / NONE`;
- binding presentation non-authoritative.

Non fare affidamento su nomi asset inventati.

Se serve Editor session per verificare `.uasset`, registrarla secondo governance.

---

# 6. PRIORITÀ DEL PRIMO SLICE

Dopo audit, l'ordine desiderato è concettualmente:

```text
P0
TD01-A Launcher
TD01-B Canonical Action Lab execution
TD01-C Deterministic Reset
TD01-E Golden Skill contract
TD01-F Baseline Scenario
TD01-I Regression / parity

P1
TD01-D Context controls
TD01-J Paragon presentation audit

P2 / candidate defer TD 0.2
TD01-G Tactical scenario
TD01-H Save As Scenario
```

Claude deve modificare questa priorità se il repository dimostra dipendenze diverse.

---

# 7. DEFINITION OF DONE DEL PRIMO SLICE

Il primo slice è riuscito quando un designer può:

```text
1. aprire Tactical Designer;
2. scegliere Action Lab;
3. selezionare una mappa canonica;
4. selezionare Gadget;
5. selezionare la Golden Skill;
6. scegliere target/context minimo;
7. premere PLAY;
8. vedere playback + TurnLog;
9. premere PLAY di nuovo e ottenere lo stesso risultato/hash;
10. aprire/eseguire lo scenario baseline della stessa skill;
11. ottenere parità fra Action Lab e Scenario Harness;
12. avere almeno un test automatico che protegge il contratto.
```

Presentation:

```text
Plasma Blast animation/VFX
```

può essere `PARTIAL` nel primo merge se il gameplay loop è verde, ma il risultato dell'audit deve essere registrato.

---

# 8. NON-GOALS TD 0.1

NON includere automaticamente:

- Skill Workbench completo;
- modifier editor completo;
- visual scripting;
- mass simulation;
- balance dashboard;
- bot tournament;
- networking;
- GAS migration;
- modding;
- promotion workflow;
- export video;
- art finale;
- tutte le skill dei quattro personaggi.

Una Golden Skill end-to-end vale più di 20 pannelli incompleti.

---

# 9. ROADMAP / DOC OWNER

NON creare:

`docs/roadmap/skill-lab-roadmap-new.md`

se esiste già un owner Tactical Designer.

Verifica e aggiorna solo i file owner reali, probabilmente fra:

- `docs/technical/spec-tactical-designer.md`
- `docs/roadmap/roadmap-checkpoint.md`
- `docs/roadmap/editor-sessions.yaml`
- Scenario Map corrente
- Wiki Tactical Designer / designer workflow
- eventuale tracking dell'epic `#1105`

Se una roadmap TD 0.1→1.0 esiste già:
integrala, non duplicarla.

---

# 10. STRUTTURA DELLE ISSUE NUOVE

Ogni nuova issue deve usare:

```markdown
# Why

Problema misurato.

# Evidence

Codice, asset, test, issue, scenario o output che dimostrano il gap.

# Owner

#1105 Tactical Designer oppure owner più specifico già esistente.

# Scope

# Out of scope

# Acceptance criteria

- [ ] criterio osservabile
- [ ] automation dove applicabile
- [ ] editor validation dove necessaria
- [ ] nessuna divergenza runtime/editor
- [ ] owner docs aggiornati

# Tests

# Dependencies

# Related

Epic / roadmap / scenario / ADR / Decision Log.
```

Titoli focalizzati, da una PR ragionevole.

---

# 11. CONSOLIDAMENTO GITHUB

Produci una matrice:

| Candidate | Existing issue | State | Owner | Action |
|---|---:|---|---|---|
| TD01-A | | | | REUSE / UPDATE / CREATE / DEFER |
| TD01-B | | | | |
| TD01-C | | | | |
| TD01-D | | | | |
| TD01-E | | | | |
| TD01-F | | | | |
| TD01-G | | | | |
| TD01-H | | | | |
| TD01-I | | | | |
| TD01-J | | | | |

Per le issue NON create indica:

```text
Candidate:
Covered by:
Reason:
```

Questo output è obbligatorio.

---

# 12. TITOLI ROADMAP FINALI DA RIPORTARE

Alla fine del consolidamento voglio una vista equivalente a:

| Stage | Candidate title | Real owner after audit |
|---|---|---|
| TD 0.1 | Golden Skill Execution & Scenario Validation Foundation | |
| TD 0.2 | Visual Scenario Authoring for Skill Tests | |
| TD 0.3 | Skill Workbench MVP | |
| TD 0.4 | Integrated Skill + Scenario Experiment Loop | |
| TD 0.5 | Skill Explainability & Tactical Probes | |
| TD 0.6 | Replay-to-Scenario Skill Authoring | |
| TD 0.7 | Batch Skill Comparison & Balance Analytics | |
| TD 0.8 | Skill Regression Matrix & Impact Analysis | |
| TD 0.9 | Production Skill Promotion & Governance | |
| TD 1.0 | Production Skill Design & Validation Environment | |

Se questi titoli devono essere sezioni/checkpoint e NON epic separate per aderire alla governance reale:
fallo.

Non inventare numeri `E*`.

---

# 13. TEST / VALIDAZIONE

Dopo modifiche repository:

- build applicabile;
- Automation pertinente;
- Scenario Harness pertinente;
- docs checks;
- eventuali registry validators;
- eventuale Scenario Map validator;
- editor tests se toccati;
- asset reference validation;
- verifica nessun `.uasset`/`.umap` cambiato accidentalmente.

Se un test non è stato eseguito:

`NOT RUN`

Mai dichiararlo PASS per supposizione.

---

# 14. REPORT FINALE OBBLIGATORIO

Restituisci:

```text
TD SKILL / ACTION LAB ROADMAP CONSOLIDATION REPORT

Repository:
Branch:
HEAD:
origin/main:
Audit date:

A. EXISTING CAPABILITIES
...

B. PARTIAL CAPABILITIES
...

C. MISSING CAPABILITIES
...

D. ROADMAP TD 0.1 → TD 1.0
Stage | Final title | Owner issue/section | State

E. ISSUE AUDIT TD 0.1
Candidate | Existing | State | Action | Reason

F. ISSUES CREATED
#ID — title
Parent:
Priority:
Reason:

G. ISSUES UPDATED
#ID — change

H. ISSUES NOT CREATED
Candidate:
Covered by:
Reason:

I. ROADMAP / DOCS UPDATED
path | section | action

J. SCENARIOS
scenario | action | test/evidence

K. TESTS
PASS:
FAIL:
NOT RUN:

L. FIRST EXECUTABLE ISSUE
#ID / title:
Why this one:

M. RECOMMENDED NEXT 3 ISSUES
1.
2.
3.

N. COMMITS
...
```

---

# 15. CRITERIO DI SUCCESSO

Il consolidamento è riuscito se:

1. esiste UN solo owner del Tactical Designer;
2. la nuova capability Skill + Action Lab + Scenario è mappata senza roadmap parallela;
3. il primo slice è piccolo e realmente eseguibile;
4. non è stato creato un secondo skill/runtime/scenario resolver;
5. la Golden Skill ha un percorso Action Lab + Scenario + test;
6. candidate skill e production data non vengono confusi;
7. presentation Paragon non diventa authority gameplay;
8. Claude identifica la prima issue realmente eseguibile in base a `origin/main`;
9. GitHub non riceve issue duplicate;
10. roadmap e tracking permettono a una nuova sessione di capire il prossimo lavoro in meno di cinque minuti.

Non implementare tutto questo task in una mega-PR.

Prima consolida.
Poi implementa una issue alla volta.
