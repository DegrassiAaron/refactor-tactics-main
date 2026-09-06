---
name: issue-run
description: Executes a RefactorTactics GitHub issue inside ONE RT3 figure (DEV, EDITOR or VALIDATION), never all three in the same session. Defines the issue through sc:spec-panel, keeps code/docs/issues/references coherent, does the work that belongs to the current role, and hands off what belongs to another figure.
argument-hint: "[issue-number]"
arguments:
  - issue
disable-model-invocation: true
---

# RefactorTactics — Issue Run

Execute GitHub issue **$issue** end-to-end.

Invocation:

`/issue-run $issue`

The command is an orchestrator. It MUST NOT jump directly from reading the issue to editing code.

## Phase -1 — Role gate, prima di tutto

⛔ **Questa skill esegue una sola figura RT3 per sessione.** Non salta da `DEV` a Editor a validation dentro la stessa finestra: quella catena e' l'invariante che `CLAUDE.md` §10 e `RT3_CONTRACT.md` §12 tengono in piedi, e una skill che la percorre da sola la annulla.

Prima di qualunque altra cosa, leggi:

```powershell
$env:RT_TERMINAL_ROLE
$env:RT_TASK_ID
```

### Se `RT_TERMINAL_ROLE` non c'e'

`ROLE_MISSING`. Fermati. `CLAUDE.md` §2 e' esplicito: fail-closed, nessun lavoro mutante finche' il ruolo non e' risolto.

Dillo in una riga e proponi le due strade: aprire un terminale RT (`RT: Open DEV terminal`, oppure `RT: Open next task terminal` se il lavoro e' instradato) o invocare `/rt-session-role <dev|editor|validation>`.

Non dedurre il ruolo dal contenuto della issue.

### Se c'e', vale questo confine

| Ruolo | Fa | Non fa |
|---|---|---|
| `DEV` | C++, test, scenari come dati, tooling headless, docs, Git/GitHub, gate statici (`node tools/...`) | UnrealEditor, PIE, `rtsuite`, `rtbuild`, packaging, MCP asset write |
| `EDITOR` | `.uasset`/`.umap`, Blueprint, UMG, Material, wiring visivo, PIE, acceptance percettiva | suite, build, verdetti su determinismo/privacy/replay/packaged |
| `VALIDATION` | build, Automation mirata, regression, Scenario Harness, determinismo, replay, privacy, packaged, performance | authoring asset; riparare il difetto che ha trovato e poi approvarlo |

Cio' che non appartiene al tuo ruolo non diventa `N/A` e non diventa `PASS`: diventa `NOT RUN` con il nome della figura che lo possiede, e un handoff.

### Se `RT_TASK_ID` c'e'

Il lavoro e' instradato. Prima di iniziare:

```powershell
rttask status -TaskId $env:RT_TASK_ID
rttask assignment -TaskId $env:RT_TASK_ID
```

Se `next_actor` non e' il tuo ruolo: `TASK_ROUTE_MISMATCH`, fermati. Non correggere il routing — il router lo rifiuterebbe comunque da una sessione con ruolo.

Lavora dentro l'assignment. Al termine, invece di inventare il passo successivo:

```powershell
rttask report -TaskId <id> -Status <DONE|PARTIAL|BLOCKED|FAILED> `
    -Summary "..." -Changes "..." -Evidence "..." -NotRun "..." `
    -NextActorRecommended <actor>
```

⛔ `NEXT_ACTOR_RECOMMENDED` e' una **raccomandazione**. Il prossimo actor lo decide il Coordinator.

Semantica: `docs/rt-three-terminals/TASK_ROUTING.md`.

---

## Core invariants

1. The issue must be claimed before specification or implementation starts — **quando il claim spetta a questa sessione**. Vedi Phase 1: in una catena RT3 il claim e' gia' stato preso a monte.
2. Due sessioni non lavorano la stessa issue **contemporaneamente**. La **successione** `DEV -> EDITOR -> VALIDATION` sulla stessa issue non e' concorrenza: e' il flusso normale.
3. Run the existing `sc:spec-panel` workflow with the same issue argument before implementation.
   - Equivalent invocation: `/sc:spec-panel $issue`
   - If `sc:spec-panel` is exposed as a Claude Code Skill, invoke that Skill.
   - Do not silently replace it with an improvised specification if the command/Skill is available.
4. The GitHub issue is the operational source of truth for ownership and progress.
5. Code, tests, documentation, architecture references, roadmap and related issues must remain mutually coherent.
6. Do not mark work complete because the code merely compiles.
7. Epic/Unreal MCP e PIE appartengono alla figura `EDITOR`, e la scrittura asset via MCP solo al workspace `MAIN`. Da `DEV` o `VALIDATION` non si usano.
8. Never allow Unreal Editor animations, timing or presentation state to become gameplay authority.
9. Never expose or weaken RefactorTactics planning privacy, determinism or server authority requirements.
10. Always perform cleanup. Il lifecycle dell'Editor riguarda solo la figura che l'ha aperto: chi non lo apre non lo chiude, e non chiude quello di un'altra sessione.

---

# Phase 0 — Normalize and preflight

Normalize `$issue` to a numeric GitHub issue number. Accept `123` or `#123`.

Determine:
- repository root;
- current branch/worktree;
- GitHub repository;
- current Git status;
- current Claude session id `${CLAUDE_SESSION_ID}`;
- available GitHub integration: GitHub MCP first, otherwise `gh` CLI if already configured;
- `RT_TERMINAL_ROLE` e `RT_TASK_ID`, gia' letti in Phase -1;
- available Epic/Unreal MCP tools — **solo se il ruolo e' `EDITOR`**;
- whether Unreal Editor is currently running for this project — in sola lettura (`rtlease -Action status`), che e' informazione utile a ogni ruolo perche' dice se il motore e' occupato.

Read the issue completely, including:
- title;
- body;
- comments;
- labels;
- assignees;
- linked PRs;
- dependencies/blockers;
- GitHub Project status when available;
- parent epic/milestone when available.

Do not overwrite unrelated local changes.

If the current worktree contains unrelated uncommitted changes, preserve them. Prefer an isolated branch/worktree for this issue rather than stashing, discarding or rewriting somebody else's work.

Before claiming, abort safely if:
- the issue does not exist;
- the issue is closed;
- it is already actively claimed by another worker/user;
- its Project state clearly indicates active work by another owner.

Do not steal a claim automatically.

---

# Phase 1 — Atomic claim

Claim the issue before doing specification work.

Preferred operational state:

`Ready -> Claimed`

Perform as many of these as the repository supports:

- set GitHub Project `Status` to `Claimed`;
- assign the current GitHub actor/worker when appropriate;
- add label `claimed` if the project uses that label;
- record the Claude session id in an issue comment;
- record the start timestamp;
- record the intended branch/worktree.

Add/update one machine-readable ownership comment similar to:

```md
<!-- rt-issue-run -->
## 🤖 Issue claimed

- Worker: Claude Code
- Session: <session-id>
- Phase: Claimed
- Branch: <branch-or-pending>
- Started: <ISO-8601 timestamp>

This issue is currently owned by this workflow. Do not start a second implementation unless the claim is explicitly released.
```

Immediately re-read the issue after the claim.

If ownership/status no longer belongs to this run, stop. This is the race-condition guard.

## Il claim in una catena RT3

⛔ **Non tutti i ruoli claimano.** Se il claim esiste gia' ed e' di un workflow a monte della stessa catena — tipicamente `DEV` ha claimato e ora tocca a `EDITOR` o `VALIDATION` — non rubarlo e non riscriverlo.

Comportamento:

| Situazione | Cosa fai |
|---|---|
| nessun claim, e sei il primo actor del task | claima normalmente |
| claim esistente, `RT_TASK_ID` presente e `next_actor` == il tuo ruolo | **non** claimare. Aggiungi un commento di fase (`Phase: <ruolo> in progress`) senza toccare l'ownership |
| claim esistente, nessun `RT_TASK_ID` che ti autorizzi | fermati. E' il caso «gia' claimata da un altro worker» che Phase 0 gia' prevede |

⚠️ **CONTRACT CONFLICT — a chi appartiene il claim.**

Le fonti accettate non lo dicono, e le due letture portano a comportamenti diversi:

- `CLAUDE.md` §3 e `implement-feature` I2 dicono che una issue ha **un solo primary owner**;
- `CLAUDE.md` §10 fa passare la stessa issue attraverso `DEV`, `EDITOR` e `VALIDATION`;
- questa skill, storicamente, claimava per **sessione** e abortiva davanti a un claim altrui.

Se il claim appartiene alla **sessione**, la catena e' impossibile: il secondo ruolo trova sempre un claim non suo. Se appartiene al **task**, serve dire come si verifica che due sessioni appartengano allo stesso task — e oggi l'unica prova disponibile e' `RT_TASK_ID` piu' il verdetto del router.

Nessun Decision Log, ADR o owner specification risolve la domanda. **Non inventarla.**

Finche' la decisione non esiste, vale la tabella sopra: fail-closed, il claim non si ruba e non si riscrive, e il report lo dichiara. Se la situazione non rientra in nessuna riga, fermati e chiedi.

---

# Phase 2 — Define

Transition operational state:

`Claimed -> Defining`

Update the ownership comment to `Phase: Defining`.

Now invoke the existing specification workflow with the same argument:

`/sc:spec-panel $issue`

Use the resulting specification as the basis for implementation.

The definition must be reflected back into the GitHub issue using stable DNNN sections. Preserve existing useful issue content; do not destroy human context.

Minimum expected definition:

- **D001 — Scope**
- **D002 — Expected behavior / authority**
- **D003 — Data, APIs and state**
- **D004 — Dependencies and affected systems**
- **D005 — Files/assets expected to change**
- **D006 — Validation rules and failure cases**
- **D007 — Automated tests**
- **D008 — Scenario/editor validation**
- **D009 — Documentation and related-issue impact**
- **D010 — Definition of Done**

For RefactorTactics, explicitly check when relevant:
- server authority;
- deterministic resolution;
- stable IDs/versioning;
- integer/fixed-point gameplay costs;
- snapshot boundaries;
- TurnLog/replay impact;
- planning privacy/team-only data;
- pathfinding/LOS/targeting separation;
- GAS boundary vs authoritative simulator;
- data-driven definitions and validators;
- network/replay compatibility.

If the spec reveals missing prerequisite work, update/link the related issue(s) and set this issue to `Blocked` instead of fabricating the dependency.

---

# Phase 3 — Coherence impact scan

Before implementation, inspect the repository and GitHub for references that can become stale.

Search for:
- issue number and issue title;
- affected class/type/function names;
- affected gameplay tags and stable IDs;
- relevant docs/ADR/PDR/design notes;
- roadmap/checklists;
- test plans;
- schemas/data assets/catalog definitions;
- open issues that depend on, duplicate, contradict or extend this work.

Build a short internal impact set:

`code + tests + assets + docs + issues + roadmap + references`

This set is part of the work. Documentation is not an optional final polish.

Rules:
- Update source documentation, not generated/exported artifacts, when a canonical source exists.
- Do not casually rewrite historical decision records. Add a superseding note/decision when history must be preserved.
- If a public API, schema, gameplay rule, status flow, ID, tag, asset contract or architecture boundary changes, update all live references that describe it.
- Update related GitHub issues when their assumptions, dependencies, acceptance criteria or status changed.
- Add cross-links between issues when a dependency or follow-up is discovered.
- Do not close unrelated issues merely because this issue touched the same area.
- If a new follow-up is necessary and cannot reasonably fit this issue, create/link a focused follow-up issue rather than silently expanding scope.

After the impact scan, the issue must contain the final DNNN definition before coding begins.

---

# Phase 4 — Start implementation

Only now transition:

`Defining -> In Progress`

Create/reuse an issue branch using the repository convention. Default if no convention exists:

`issue/$issue-<short-slug>`

Do not commit unrelated changes.

Implement the smallest scalable solution that satisfies the DNNN definition and the existing RefactorTactics architecture.

During implementation:
- keep C++ authority for simulation/network/serialization/pathfinding/validation/competitive rules;
- keep Blueprint/data for configuration/presentation where appropriate;
- keep logical state separate from presentation;
- never make animation timing decide simulation outcomes;
- preserve deterministic ordering and stable identifiers;
- preserve team-planning privacy;
- add useful logs/debug visibility;
- avoid hard-coded content references when the project expects catalog/data-driven IDs.

If implementation invalidates any DNNN item, update the issue definition before continuing. The issue must describe what is actually being built.

---

# Phase 5 — Compile and automated tests

Transition:

`In Progress -> Validating`

Esegui **solo i gate che il tuo ruolo possiede**.

| Ruolo | Puo' eseguire | Deve dichiarare `NOT RUN` |
|---|---|---|
| `DEV` | gate statici e headless che non occupano Unreal: `node tools/radar/*.ts`, `node --test`, parse PowerShell, `git diff --check` | compile Unreal, Automation, scenario, determinismo, replay, packaged, performance |
| `EDITOR` | acceptance visiva e PIE | suite, build, gate headless |
| `VALIDATION` | `rtbuild`, `rtsuite`, Automation mirata, scenario, determinismo, replay, privacy, packaged, performance | authoring asset |

⛔ Da `DEV`, «compilo solo per controllare» occupa il motore e puo' rendere `NON VALIDA` la misura di un'altra sessione. Non e' una scorciatoia: e' il difetto che `rtbuild` esiste per chiudere.

Un gate che non ti appartiene si dichiara `NOT RUN` col nome della figura che lo possiede. **`NOT RUN` non e' `PASS`.**

Minimum checks when applicable, dentro il proprio ruolo:
- formatting/static validation already used by the repo;
- Unreal C++ compilation for the milestone's pinned UE5 version;
- targeted Automation Tests;
- regression tests for affected systems;
- data validators;
- deterministic/replay checks if simulation changes;
- network/privacy tests if replication or intents change.

Do not claim success based only on compilation.

Record:
- commands run;
- pass/fail;
- relevant logs;
- known warnings;
- test names.

Fix failures caused by this change.

If blocked by an external/unrelated failure, document the exact blocker and distinguish it from failures introduced by this issue.

---

# Phase 6 — Editor e PIE — **solo dalla figura EDITOR**

⛔ **Se `RT_TERMINAL_ROLE` non e' `EDITOR`, questa fase e' `NOT RUN`.** Non aprire l'Editor, non avviare PIE, non chiamare MCP per mutare. Registra cosa servirebbe e a chi, e passa a Phase 7.

Da `DEV` questo e' vietato da `TERMINAL_DEV.md`; da `VALIDATION` sarebbe authoring dentro il verificatore.

⚠️ Anche dentro `EDITOR`, la **scrittura** asset via MCP e' consentita solo dal workspace `MAIN`, che ospita l'unico bridge della macchina. Preflight obbligatorio:

```powershell
rtmcp -Operation MCP_ASSET_WRITE -TaskId <id> -AssetWriteSet <path>
rtlease -Action acquire -Operation EDITOR -TaskId <id>
```

Il lease si prende just-in-time e si rilascia (`rtlease -Action release`). Aprire un terminale non lo acquisisce.

`MCP command sent != verified`: una risposta vuota non e' un `PASS`. Serve un oracolo positivo — rilettura della property, riapertura dell'asset, compile esplicito, PIE.

Use the connected Epic/Unreal MCP when the issue affects behavior that should be verified in Unreal Editor or in a gameplay scenario.

Do not invent MCP tool names. Discover and use the tools actually exposed by the connected Epic/Unreal MCP server.

## Editor ownership and safety

Before using Unreal Editor:

1. Detect whether an Unreal Editor instance for this project is already running.
2. Determine whether that instance was launched by this workflow or pre-existed.
3. Do not force-close a pre-existing user-owned Editor instance.
4. Do not take exclusive control of an Editor that is actively being used by another process/person.
5. If an existing Editor can be safely reused through the MCP without disrupting work, reuse it.
6. Otherwise, launch a dedicated Editor instance for this workflow.
7. Track `editor_started_by_issue_run = true|false`.

Never discard unsaved user work.

## Scenarios

Inspect existing scenario/test infrastructure first.

When suitable scenarios already exist:
- run the smallest relevant scenario set;
- capture outcome and useful evidence/logs.

When a behavior is scenario-testable but no suitable scenario exists:
- create the smallest reusable scenario/test needed;
- place it in the project's established test/scenario location;
- keep it deterministic where possible;
- document what the scenario proves;
- run it.

Examples of scenario concerns when relevant:
- simultaneous movement/resolution;
- collision/interruption;
- targeting and LOS;
- cover/height interaction;
- hazards/environment propagation;
- ready/commit/snapshot flow;
- team-only intent visibility;
- ability resolution;
- replay/TurnLog equivalence.

A scenario is verification, not a replacement for lower-level automated tests.

If the issue changes multiplayer/privacy behavior, prefer a scenario that includes server plus appropriate clients rather than a single-editor visual check.

---

# Phase 7 — Reconcile the whole environment

After implementation and validation, repeat the coherence scan.

This is mandatory.

Re-check:
- implementation vs DNNN;
- tests vs behavior;
- documentation vs implementation;
- diagrams/specs/roadmap vs implementation;
- issue body vs implementation;
- related issues vs new reality;
- stable IDs/tags/schemas/catalogs;
- README/setup/build instructions if changed;
- TODO/FIXME references;
- generated files vs canonical sources;
- version/hash/validator expectations if relevant.

Update all affected live references.

For every related issue materially impacted, add a concise comment or edit explaining the new dependency/status/assumption.

Do not leave known contradictory documentation behind.

---

# Phase 8 — Final issue update

Update the primary issue with an execution report:

```md
## Implementation result

### Implemented
- ...

### DNNN status
- D001: ✅
- D002: ✅
- ...

### Tests
- `<test>` — PASS
- ...

### Unreal/Epic MCP scenarios
- `<scenario>` — PASS / NOT APPLICABLE
- ...

### Documentation updated
- ...

### Related issues updated
- #...

### Remaining risks / follow-ups
- ...

### Git
- Branch: `...`
- Commit(s): `...`
- PR: <link-or-not-created>
```

The report must be factual. Do not mark unrun tests as passed.

---

# Phase 9 — Final state

Choose the final issue state from actual evidence.

### `In Review`
Use when:
- implementation is complete;
- required tests/scenarios pass;
- docs/issues are coherent;
- a PR exists but has not been merged.

### `Done`
Use only when repository policy allows completion at this stage and all Definition of Done requirements are actually satisfied. If the project requires merge before Done, do not mark Done before merge.

### `Blocked`
Use when:
- a real unresolved dependency prevents completion;
- required validation cannot be completed;
- a non-owned Editor or external resource prevents required scenario verification;
- infrastructure failure prevents proving the Definition of Done.

### `In Progress`
Keep only when useful implementation work is genuinely incomplete and this same owner/session is expected to continue.

Never leave the issue in `Claimed` or `Defining` after the run has moved beyond those phases.

---

# Phase 10 — Mandatory cleanup

Always execute cleanup, including on failure.

## Unreal Editor

Riguarda **solo** la figura `EDITOR`, e solo se ha aperto lei l'Editor. Da `DEV` e `VALIDATION` questa sezione e' `N/A`: non hai aperto niente, e non chiudi quello di un'altra sessione.

If this workflow launched Unreal Editor:

1. save only intentional project changes;
2. ensure scenario/test execution has completed;
3. request a normal Editor shutdown;
4. wait for the process to terminate;
5. if normal shutdown fails, report it before considering force termination;
6. never force-kill if doing so risks unsaved project data.

At the end, an Editor instance started by this workflow must not be left running.

If the Editor was already running before this command, leave it running unless explicit exclusive ownership was established for this run. Do not close a user's pre-existing session just to satisfy cleanup.

## Git/GitHub

- Do not discard unrelated work.
- Do not delete useful logs required for debugging.
- Ensure issue status/comment reflects the actual final phase.
- Ensure branch/worktree ownership is clear.
- Release temporary resources created only for validation.

---

# Failure protocol

On any unrecoverable failure:

1. stop making unrelated changes;
2. preserve useful diagnostics;
3. reconcile any partial documentation/issue edits so they are not misleading;
4. update the main issue with:
   - completed work;
   - failing step;
   - exact blocker;
   - test/scenario evidence;
   - recommended next action;
5. set the issue to `Blocked` when appropriate;
6. run mandatory cleanup;
7. shut down any Unreal Editor instance started by this workflow.

Never hide a partial failure behind a `Done` state.

---

# Completion gate

⚠️ **«Applicable» qui significa: appartiene al mio ruolo.** Una voce che appartiene a un'altra figura non si spunta e non si cancella: si marca `NOT RUN` con il nome della figura che la possiede. Spuntarla sarebbe fabbricare evidenza; cancellarla sarebbe far sparire un gate.

## Ogni ruolo

- [ ] Il ruolo RT3 era risolto prima di iniziare (Phase -1).
- [ ] Il claim e' coerente con la tabella di Phase 1: preso, oppure riconosciuto e non riscritto.
- [ ] `sc:spec-panel` was executed with `$issue` — quando questo ruolo possiede la definizione.
- [ ] DNNN definition is present in the issue.
- [ ] Implementation matches the DNNN — per la parte che appartiene a questo ruolo.
- [ ] Documentation is coherent.
- [ ] Related issues are coherent and cross-linked.
- [ ] Roadmap/spec references are coherent.
- [ ] Git changes contain no unrelated work.
- [ ] Main issue contains the execution report.
- [ ] Final GitHub status is accurate.
- [ ] Cio' che appartiene a un'altra figura e' elencato come `NOT RUN`, con la figura che lo possiede.
- [ ] Se `RT_TASK_ID` era presente, `rttask report` e' stato eseguito.

## `DEV`

- [ ] Gate statici/headless eseguiti, con il loro esito reale.
- [ ] Nessun processo Unreal avviato da questa sessione.

## `EDITOR`

- [ ] Preflight `rtmcp` eseguito prima di ogni scrittura asset, e workspace `MAIN` verificato.
- [ ] Lease acquisito just-in-time e rilasciato.
- [ ] Evidenza PIE con un oracolo positivo, non una risposta vuota.
- [ ] Unreal Editor lifecycle cleanup completo, e nessuna istanza aperta da questo workflow lasciata viva.
- [ ] Nessun verdetto emesso su sistemi il cui tetto per `EDITOR` e' `OBSERVED`.

## `VALIDATION`

- [ ] Relevant build succeeds — dentro il lease.
- [ ] Relevant automated tests pass, con `found` / `performed` / `passed` / `failed` / exit code dichiarati.
- [ ] `HEAD`, working tree e binari invariati durante la misura; altrimenti l'esito e' `NON VALIDA`.
- [ ] Nessun difetto riparato e poi approvato da questa stessa sessione.
