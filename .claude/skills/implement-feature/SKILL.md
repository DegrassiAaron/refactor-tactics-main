---
name: implement-feature
description: Critically analyzes a RefactorTactics objective, reconciles it with live GitHub issues and repository contracts, creates or updates the minimum set of independent work-unit issues, and routes them through the RT3 figures (DEV, EDITOR, VALIDATION) using the task router. Creates no more work units than the objective needs.
argument-hint: "<feature or objective description>"
disable-model-invocation: true
---

# RefactorTactics — Implement Feature

Objective supplied by the user:

`$ARGUMENTS`

Invocation example:

`/implement-feature Replay scenario`

This skill is an **orchestrator of issues and RT3 routing**.

It MUST NOT implement the requested feature itself.

Its job is:

1. understand the requested outcome;
2. inspect the repository and live GitHub state;
3. **criticize the proposed direction before producing a plan**;
4. find, create, or update the minimum useful set of GitHub issues;
5. make those issues independently executable;
6. decide, for each of them, which RT3 figure is the **minimum** one that can do the work;
7. hand the routing to the task router when it is available, instead of producing a stack of prompts the user has to remember and paste;
8. leave a dependency graph that can actually be executed without two workers owning the same issue or the same binary Editor asset.

---

# Non-negotiable invariants

## I1 — Work starts from an issue, and the worker stays inside its role

Every worker that performs work executes exactly one GitHub issue through:

`/issue-run <issue-number>`

Do not replace `/issue-run` with an ad-hoc implementation prompt.

The issue is the operational source of truth for **what** must be done.

`/issue-run` is role-aware: run inside a DEV terminal it does not open Unreal, and run inside EDITOR or VALIDATION it does not do DEV's job. Assigning an issue therefore means assigning it **to a figure**, not to an anonymous terminal.

## I2 — One terminal, one issue, one owner

`/issue-run` claims its issue.

Therefore:

- NEVER assign the same issue to two terminals;
- NEVER ask two terminals to solve different slices of one claimed issue concurrently;
- if a parent issue is too large for safe parallel work, create or reuse **focused child/work-unit issues**;
- the parent remains the capability/scope owner when that is already the repository model;
- child issues are execution units, not duplicate primary owners.

## I3 — EDITOR and VALIDATION are two figures, never one

⛔ **`VALIDATION_EDITOR` non esiste.** Una versione precedente di questa skill lo imponeva come terminale combinato: è incompatibile con le fonti accettate, e produceva esattamente il difetto che RT3 esiste per impedire.

Le fonti, tutte e tre concordi:

- `CLAUDE.md` §6 — «VALIDATION non deve: modificare un problema; validare autonomamente il proprio fix; dichiararlo approvato»;
- `RT3_CONTRACT.md` §7 — EDITOR ha tetto `OBSERVED` su `DETERMINISM`, `PRIVACY`, `TURNLOG/REPLAY`, `NETWORK AUTHORITY`, `PERFORMANCE`, `AUTOMATION/SCENARIO`, `BUILD`, `ARCHITECTURE`, e `N/A` su `PACKAGED`. Non ha lo strumento, non è un tetto disciplinare;
- `RT3_CONTRACT.md` §12 — chi trova un `P0`/`P1` non ripara e poi approva sé stesso.

Un terminale che authora l'asset **e** firma la validazione è entrambe le parti del difetto in una sola sessione.

Quindi:

| Figura | Possiede |
|---|---|
| `DEV` | C++, test, tooling headless, Git/GitHub. Non occupa Unreal |
| `EDITOR` | `.uasset`/`.umap`, Blueprint, UMG, PIE, acceptance visuale. Unico writer binario |
| `VALIDATION` | Automation, scenario, determinismo, privacy, replay, packaged, performance. Verificatore **indipendente** |

Regole che ne derivano:

- un solo writer binario per wave: `EDITOR`;
- `VALIDATION` verifica un commit che non ha prodotto;
- se `VALIDATION` trova un difetto che richiede una modifica, produce un finding per `DEV` o `EDITOR` e **rivalida** un commit nuovo. Non lo ripara.

I worker di implementazione usano automazione headless quando possibile. Se una issue richiede evidenza visiva/Editor, quella parte appartiene a `EDITOR` e la dipendenza va dichiarata.

## I4 — Critique before plan

Do not create or update issues until the objective has passed a **Critical Review Gate**.

The first output of the skill is analysis, not a task list.

Challenge:
- whether the requested outcome already exists;
- whether an open issue already owns it;
- whether the apparent feature is actually a bug, missing validation, tooling gap, data gap, authoring gap, or unresolved design decision;
- whether parallelization is safe;
- whether the work can be split without overlapping files/contracts;
- whether a scenario is actually needed;
- whether Unreal Editor/MCP is actually needed;
- whether the requested work would create a second authority, second simulator, duplicated feature owner, or unnecessary infrastructure;
- whether a dependency must land first;
- whether the smallest testable solution is smaller than the requested solution.

If evidence says the requested plan is wrong, say so and change the decomposition.

Do not create work merely to fill terminals.

## I5 — Minimum useful issue set

Reuse an existing issue when it already owns the required work.

Create a new issue only when:
- no existing issue owns the work;
- a large existing issue needs independently executable child work;
- a follow-up is genuinely outside the current issue scope;
- authoring o verifica hanno bisogno di un owner distinto perche' appartengono a un'altra figura.

Do not create duplicate issues for symmetry.

## I6 — MCP and scenarios are conditional tools

Use GitHub MCP when available for live issue reads/writes.

Use Epic/Unreal MCP only when it adds meaningful evidence.

Use Scenario Harness scenarios when the behavior is best proved end-to-end.

Do not add scenarios merely because the feature is gameplay-related.

Do not add Editor validation merely because Unreal is the engine.

Prefer:
- unit/automation tests for local logic;
- deterministic Scenario Harness coverage for cross-system gameplay;
- Editor/PIE validation for authoring, visualization, asset wiring, input, presentation, or behavior that cannot be proven headlessly;
- packaged/network scenarios only when the requirement depends on those environments.

## I7 — One owner for binary Unreal assets

No two parallel workers may edit the same:
- `.uasset`
- `.umap`
- other binary Unreal artifact.

Every Editor-created or Editor-modified binary asset belongs to the `EDITOR` figure. `VALIDATION` may read and verify them; it must not rewrite them.

⚠️ L'authoring asset via MCP è consentito **solo** dal workspace `MAIN`, che ospita l'unico bridge della macchina. Il ruolo EDITOR esiste in ogni checkout; l'authoring no. Preflight: `rtmcp -Operation MCP_ASSET_WRITE -TaskId <id> -AssetWriteSet <path>`.

## I8 — Preserve RefactorTactics authority boundaries

When relevant:
- runtime/resolver remains gameplay authority;
- Editor/UI is a consumer of canonical runtime/query/DTO surfaces;
- replay consumes canonical traces and does not recalculate gameplay;
- planning privacy is enforced at the data/replication boundary, not just visually;
- deterministic ordering and stable IDs are preserved;
- scenarios exercise the real gameplay path, not shortcuts such as direct teleport/damage unless the tested system explicitly owns that operation;
- presentation/animation timing never determines simulation outcome.

---

# Phase 0 — Normalize the objective

Normalize `$ARGUMENTS` into a short working objective.

Examples:

`Replay scenario`
`GrayKit action fallback`
`Semantic path preview`
`Ready/Commit reconnect flow`

Do not reinterpret a vague objective into a large feature without evidence.

Record:

- `OBJECTIVE`
- `SUCCESS OUTCOME`
- `KNOWN CONSTRAINTS`
- `UNKNOWN / NEEDS EVIDENCE`

Do not ask the user for information that can be discovered from the repository, GitHub, MCP, roadmap, ADRs, tests or scenarios.

---

# Phase 1 — Repository and issue discovery

Before proposing any plan, inspect the current state.

## Repository

Read when present and relevant:

- `AGENTS.md`
- `CLAUDE.md`
- `.claude/skills/issue-run/SKILL.md`
- relevant ADR/PDR/Decision Log entries
- roadmap/checkpoint documents
- related specs
- existing tests
- Scenario Harness infrastructure
- replay/TurnLog contracts
- Editor/tooling surfaces
- asset maps / editor session definitions
- affected runtime code

Search for the objective by:
- feature terminology;
- class/type/function names;
- test names;
- scenario names;
- issue references;
- roadmap references.

## GitHub

Use GitHub MCP first when available.

Search:
- open issues;
- recently closed issues when they may have delivered part of the objective;
- epics/parents;
- dependencies/blockers;
- PRs if relevant;
- roadmap/capability tracking issues.

For each relevant issue classify:

- `OWNER`
- `DEPENDENCY`
- `DUPLICATE / OVERLAP`
- `DELIVERED PART`
- `BLOCKER`
- `NOT RELEVANT`

Do not trust an old handoff over live repository + live GitHub state.

---

# Phase 2 — Critical Review Gate

Produce a concise but serious review BEFORE issue creation/update.

Use this structure:

## INTENTO
What the user is actually trying to achieve.

## FATTI
Only what repository/GitHub evidence proves.

## INFERENZE
Likely consequences, explicitly labeled.

## PROBLEMI / AMBIGUITÀ
Missing ownership, duplicate authority, unclear acceptance criteria, overlap, risky shared files, unresolved decisions.

## FAILURE MODES
What can go wrong if implemented as first imagined.

Check at least:
- fake parallelism caused by workers touching the same files;
- parent issue being claimed by multiple terminals;
- duplicate scenario/editor work;
- binary asset merge conflicts;
- duplicated runtime/editor rules;
- tests that prove implementation details instead of behavior;
- scenario that passes vacuously;
- replay/state hash drift;
- privacy leakage;
- nondeterministic iteration/order;
- validation only at the end when a contract mismatch could have been detected earlier.

## ALTERNATIVE PIÙ PICCOLA
The smallest safe/testable path.

## PARALLELIZATION VERDICT
Choose one:
- `SAFE`
- `SAFE WITH CONTRACT`
- `LIMITED`
- `BLOCKED`

Explain why.

### Stop condition

If `BLOCKED` because of an unresolved author/design decision or missing prerequisite:
- create/update the blocking issue if appropriate;
- do NOT fabricate an implementation plan;
- generate only safe preparatory issue-runs if they have independent value;
- otherwise stop with the blocker.

---

# Phase 3 — Build the issue graph

Only after the Critical Review Gate passes, create/update the execution graph.

⛔ **Non esiste un numero di work unit da raggiungere.** Una versione precedente chiedeva «3-6 terminali»: è un quorum, e un quorum si riempie inventando lavoro. Il numero corretto è quello che l'obiettivo richiede, e a volte è **uno**.

Per ogni work unit decidi la figura RT3 **minima** che può eseguirla:

| Se il lavoro è | Figura |
|---|---|
| C++, test, tooling headless, dati testuali, docs | `DEV` |
| `.uasset`/`.umap`, Blueprint, UMG, wiring visivo, PIE | `EDITOR` |
| Automation, scenario, determinismo, privacy, replay, packaged, performance | `VALIDATION` |
| un giudizio che nessuno strumento produce (leggibilità, feel, approvazione) | `USER` |

Non coinvolgere una figura perché «di solito c'è». `EDITOR` senza asset da toccare non ha lavoro; `VALIDATION` esiste quando c'è un gate da eseguire, non come timbro finale d'ufficio.

Forme tutte legittime, e la scelta dipende dal task:

```text
DEV -> VALIDATION                           bug C++ puro
EDITOR -> VALIDATION                        authoring di contenuto
DEV -> VALIDATION -> EDITOR -> VALIDATION   gate headless, poi PIE, poi sign-off
EDITOR -> USER                              check percettivo
VALIDATION -> DEV                           finding di codice emerso validando
```

⛔ `DEV -> EDITOR -> VALIDATION` non è cablato da nessuna parte. Non trattarlo come default.

## Allowed worker types

Choose based on evidence, not a template:

- `CORE_ARCHITECTURE`
- `INTEGRATION_DATA`
- `AUTOMATION_TESTS`
- `SCENARIOS`
- `TOOLING_UI`
- `NETWORKING`
- `CONTENT_PIPELINE`
- `REPLAY_TURNLOG`
- `DOCUMENTATION_MIGRATION`

Do not create a worker just because a category exists.

Tests may stay with implementation when separating them would create overlapping write sets.

## Parent/owner issue

If a live issue already owns the objective:
- keep it as the parent/owner;
- update it with the execution decomposition and dependencies;
- do not run the parent concurrently in multiple terminals.

If the parent itself is a small executable issue and can be one worker:
- one terminal may own it through `/issue-run`;
- other terminals must own distinct dependent issues.

If no owner exists:
- create the minimum correct owner issue or attach the work to the correct existing epic/capability.

---

# Phase 4 — Create or update each issue

Use GitHub MCP first when available; otherwise use the repository's configured GitHub path (`gh` only if already configured).

For every work-unit issue, ensure the issue contains enough information for `/issue-run` to define it further with `sc:spec-panel`.

Do NOT duplicate the entire D001-D010 definition that `/issue-run` will create.

Provide:

## Why
Why this work unit exists.

## Scope
What this terminal owns.

## Out of scope
What belongs to another terminal or owner.

## Dependencies
Explicit issue numbers.

## Contract
Inputs/outputs/shared API that other workers rely on.

## Expected write set
Expected modules/files/assets.

This is a guardrail, not permission to modify unrelated files.

## Acceptance criteria
Behavioral, observable criteria.

## Automation
Required headless tests.

## Scenario
One of:
- required, with what it proves;
- existing scenario to extend/reuse;
- `N/A` with reason.

## Editor / PIE
For a `DEV` work unit, normally write:

`Delegated to the EDITOR figure (#<issue> if a dedicated one exists). Do not open Unreal, do not touch binary assets in this issue.`

For a `VALIDATION` work unit:

`Automation/scenario/packaged only. Editor authoring belongs to EDITOR.`

Use `N/A` only when no Editor work exists anywhere in the objective.

## TurnLog / Replay impact
Required when relevant.

## Privacy / Networking impact
Required when relevant.

## DoD
What `/issue-run` must be able to prove before this worker hands off.

---

# Phase 5 — Chiudere la catena: chi authora, e chi verifica

Non esiste una issue combinata. Se il lavoro richiede sia authoring che verifica, sono due responsabilita' distinte, ed e' un requisito che le eseguano due figure diverse.

## Se serve authoring Editor

Owner: `EDITOR`. Possiede:

1. `.uasset` / `.umap` / Blueprint / UMG / Material;
2. il wiring visivo e l'acceptance percettiva;
3. la PIE evidence;
4. la persistenza: `Save -> Stop PIE -> Close Editor -> riapertura -> giudizio`.

Non possiede: determinismo, privacy, replay, packaged, performance. Su quei sistemi il suo verdetto massimo e' `OBSERVED` (`RT3_CONTRACT.md` §7), e un `OBSERVED` non e' un `PASS`.

⚠️ L'authoring via MCP avviene solo dal workspace `MAIN`, che ospita l'unico bridge della macchina. Preflight `rtmcp` obbligatorio.

## Se serve un gate

Owner: `VALIDATION`. Possiede:

1. build e Automation mirata;
2. regression e Scenario Harness;
3. determinismo, replay, privacy, packaged, performance;
4. l'evidenza rileggibile di ognuno.

Vincoli non negoziabili:

- verifica un commit che **non ha prodotto**;
- non ripara e poi approva se stesso. Un difetto produce un finding per `DEV` o `EDITOR`, e poi si rivalida un commit nuovo (`RT3_CONTRACT.md` §12);
- `performed = 0` non e' una validazione riuscita;
- se `HEAD`, working tree o binari cambiano durante la misura, l'esito e' `NON VALIDA`, non `FAIL`.

## Preflight

Prima che i worker finiscano, `VALIDATION` puo' soltanto: leggere gli acceptance criteria, preparare la matrice, individuare scenari riusabili, rilevare collisioni di contratto.

Non puo' validare codice parziale e chiamarlo completo.

## Se serve un giudizio umano

Owner: `USER`. Un check percettivo, una decisione di design, un'approvazione.

⛔ `USER_REQUIRED` non diventa `PASS` per silenzio.

---

# Phase 6 — File/write-set collision check

Before generating terminal prompts, build a collision table.

Example:

| Issue | Worker | Owned write set | Shared read-only contracts | Depends on |
|---|---|---|---|---|
| #A | CORE_ARCHITECTURE | `Source/.../Core/*` | `RTTurnLog.h` | — |
| #B | SCENARIOS | `Scenarios/...` | APIs from #A | #A contract |
| #C | AUTOMATION_TESTS | `Source/.../Tests/...` | API contract | #A |
| #D | EDITOR | `Content/...` (`.uasset`/`.umap`) | outputs A/B/C | #A #B #C |
| #E | VALIDATION | referto ed evidenza, nessun sorgente | tutti gli output | #A #B #C #D |

Rules:
- same writable file in two issues => redesign;
- same `.uasset`/`.umap` in two issues => redesign immediately;
- one worker changing an API that another worker must compile against is allowed only with an explicit contract;
- if the contract is not stable enough for parallel work, mark `LIMITED` and serialize those issues through dependencies.

Parallel does NOT mean all issues must start at the exact same second.

A valid graph may be:
- A and B parallel;
- C starts after A;
- EDITOR authora quando il contratto di #A e' stabile;
- VALIDATION prepara la matrice in parallelo, ma il gate misura solo commit completi.

---

# Phase 7 — Decide scenario and MCP usage critically

For each issue, explicitly decide:

## Scenario Harness
Use when:
- the behavior crosses multiple gameplay systems;
- ordering/resolution matters;
- determinism/replay/state hash matters;
- a regression is best represented as reusable data;
- the existing project already models that behavior through scenarios.

Do NOT use when:
- a pure function/unit test proves the contract better;
- the scenario would only restate a unit test;
- the format cannot express the required behavior yet;
- it would require bypassing the canonical runtime path.

## Epic/Unreal MCP
Use when:
- Editor authoring is part of the acceptance criteria;
- asset references/wiring must be verified;
- viewport visualization/readability matters;
- PIE behavior matters;
- an existing Editor scenario/session is the canonical validation surface.

Do NOT use when:
- behavior is headless and deterministic;
- validation is pure C++;
- the Editor would add no new evidence.

## GitHub MCP
Use for:
- finding owner issues;
- checking claim/status/dependencies;
- creating/updating work-unit issues;
- linking parent/children/dependencies;
- keeping roadmap issue state coherent.

---

# Phase 8 — Instrada, non impilare prompt

⛔ **Non generare una pila di prompt che l'utente deve ricordare e incollare a mano.** Era il difetto centrale della versione precedente: il percorso viveva nella testa di chi apriva le finestre, ed e' esattamente cio' che il task router toglie di mezzo.

## Se il task router e' disponibile

Verifica:

```powershell
pwsh -NoLogo -NoProfile -File scripts/rt-task-router.ps1 -Action list
```

Se risponde, questa e' la strada. Per ogni work unit:

1. crea il task, una volta:

```powershell
pwsh -NoLogo -NoProfile -File scripts/rt-task-router.ps1 -Action init -TaskId <issue> -Title "<titolo>"
```

2. emetti l'assignment per la figura minima:

```powershell
pwsh -NoLogo -NoProfile -File scripts/rt-task-router.ps1 -Action assign -TaskId <issue> `
    -Actor <DEV|EDITOR|VALIDATION|USER> -ExpectedSequence <sequence letta ora> `
    -Objective "..." -Context "..." -Inputs "..." `
    -Do "/issue-run <issue>", "..." -DoNot "..." `
    -ExpectedOutput "..." -NextIfPass <actor>
```

`-Do` porta `/issue-run <issue>` come primo passo: la issue resta la source of truth del **cosa**, l'assignment dice **chi** e **con quale confine**.

`-DoNot` deve nominare cio' che appartiene a un'altra figura. E' il campo che impedisce a un DEV di aprire l'Editor «tanto ci vuole un attimo».

3. all'utente consegna una riga, non un prompt:

```text
Terminal -> Run Task -> RT: Open next task terminal   ->  TaskId: <issue>
```

⛔ **Le mutazioni del routing appartengono al RT Coordinator.** Se stai girando dentro un terminale con ruolo, il router le rifiutera' con `TASK_MUTATION_ROLE_DENIED`, e ha ragione: in quel caso produci il piano e dillo all'utente, che lo esegue dal Coordinator (`RT: Open COORDINATOR`).

Semantica completa: `docs/rt-three-terminals/TASK_ROUTING.md`.

## Se il router non e' disponibile

Fallback, non default. Scrivi un prompt per work unit sotto l'area ignorata:

`Saved/Claude/ImplementFeature/<objective-slug>/`

Un prompt per figura, con il nome che la dichiara:

```md
# <DEV|EDITOR|VALIDATION> — issue #<number>

Objective: <slice>

## Execute

/issue-run <number>

## Confine di ruolo

Sei <figura>. Vale `docs/rt-three-terminals/prompts/TERMINAL_<figura>.md`.

Non fai il lavoro delle altre due:
- <cosa appartiene a DEV>
- <cosa appartiene a EDITOR>
- <cosa appartiene a VALIDATION>

## Write set

Possiedi: ...
Sola lettura: ...

## Dependencies

- ...

## Handoff

Al termine riporta: stato issue, branch, commit/PR, comandi eseguiti con il loro
esito, evidenza rileggibile, file cambiati, contract change che toccano altri,
blocker. Dichiara esplicitamente cio' che e' `NOT RUN`.
```

Non versionare questi file salvo richiesta esplicita.

⚠️ Nel fallback il percorso torna a vivere nella testa dell'utente. Dillo, invece di lasciarglielo scoprire.

---

# Phase 9 — Output the orchestration report

The `/implement-feature` command must finish with:

# CRITICAL REVIEW
Summary of the critique and chosen minimum approach.

# ISSUE GRAPH
For every issue:
- number;
- title;
- existing / updated / created;
- worker type;
- dependencies;
- parent/owner.

# PARALLEL WAVES
Example:

`Wave 0: VALIDATION preflight (sola lettura: matrice, scenari riusabili, collisioni)`
`Wave 1: #A + #B + #C   (DEV)`
`Wave 2: #D            (EDITOR, dopo il contratto di #A)`
`Wave 3: #E            (VALIDATION, gate su un commit che non ha prodotto)`

Una wave puo' anche essere una sola: `DEV -> VALIDATION` e' un grafo valido.

# COLLISION CHECK
Writable overlaps and how they were removed.

# SCENARIO / MCP DECISIONS
What will use:
- headless automation;
- Scenario Harness;
- GitHub MCP;
- Epic/Unreal MCP;
- PIE;
and why.

# ROUTING
Per ogni work unit: issue, figura RT3 scelta, e **perche' quella e non un'altra**.

Se il task router e' stato usato, riporta i TaskId creati e la sequence corrente.
Se e' stato usato il fallback, riporta i file di prompt generati e dichiaralo come
fallback.

# START NOW
Dichiara cosa puo' partire subito, e con quale comando esatto. Se il router e'
disponibile, e' una riga sola per work unit:

`RT: Open next task terminal  ->  TaskId: <id>`

---

# Example — `/implement-feature Replay scenario`

This is illustrative only. Discover the live repository before using it.

A healthy decomposition might become:

- existing replay/scenario owner issue updated;
- issue A — scenario format/fixture or replay-source contract;
- issue B — deterministic replay/scenario automation;
- issue C — scenario corpus/golden evidence if independently writable;
- issue D — `EDITOR`, se e solo se ci sono asset da authorare;
- issue E — `VALIDATION`, il gate, dipendente da tutte.

Possible waves:

`Wave 1: A + B`
`Wave 2: C after contract from A, if needed`
`Wave 3: D final`

But if live evidence shows an existing issue already owns the complete replay scenario and splitting it would create overlapping edits, DO NOT force this example. Use the smallest safe graph.

---

# Anti-patterns — reject these

## Tre worker sulla stessa issue nello stesso momento

Sbagliato:

`DEV-1 -> /issue-run 123`
`DEV-2 -> /issue-run 123`
`DEV-3 -> /issue-run 123`

Viola l'ownership esclusiva.

⚠️ Non e' lo stesso caso della **catena**: `DEV -> EDITOR -> VALIDATION` sulla stessa issue, uno dopo l'altro, e' il flusso normale. Cio' che e' vietato e' la concorrenza, non la successione.

## Riempire un quorum di worker

Creare una work unit perche' «ne servono tre» e' lavoro inventato. Se l'obiettivo ne richiede una, ne crei una.

## Un terminale che authora e poi valida se stesso

Sbagliato:

`Terminal 4 -> authora l'asset, apre il PIE, e firma il gate`

E' il difetto che `CLAUDE.md` §6 e `RT3_CONTRACT.md` §12 vietano per nome. `EDITOR` e `VALIDATION` restano due figure, e chi ripara non approva.

⚠️ Una versione precedente di questa skill prescriveva l'opposto, sotto il nome `VALIDATION_EDITOR`. Se lo trovi citato altrove, e' quel testo.

## Create issue for every technical layer

Wrong when all layers touch the same files and API repeatedly.

Parallelism must reduce elapsed work without multiplying integration risk.

## Scenario by default

A scenario is evidence, not ceremony.

## Editor as second simulator

Never reproduce resolver/pathfinding/LOS/targeting logic in Editor code to make validation easier.

## Final validation only by visual inspection

PIE/Editor evidence complements automation. It does not replace deterministic/headless gates.

## Plan before evidence

Never output terminal assignments before the Critical Review Gate.
