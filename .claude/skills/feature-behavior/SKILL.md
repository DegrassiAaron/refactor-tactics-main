---
name: feature-behavior
description: Defines and critically reviews the intended RefactorTactics behavior before implementation planning. Produces falsifiable Behavior Contracts, Acceptance Criteria and evidence/scenario candidates. Read-only: never creates issues, code, tests, scenarios, assets or roadmap changes.
argument-hint: "<idea, feature or behavior change>"
disable-model-invocation: true
---

# RefactorTactics — Feature Behavior

Requested behavior:

`$ARGUMENTS`

This skill owns the **behavior definition layer**.

Its question is:

> WHAT should the game do, WHY should it do it, and HOW could that statement be falsified?

It does NOT decide where the implementation belongs and does NOT execute the work.

The normal pipeline is:

```text
IDEA
  ↓
/feature-behavior
  ↓
BEHAVIOR_READY
  ↓
/implement-feature
  ↓
RT3
  ↓
/issue-run
```

---

# 0 — Non-negotiable boundary

`/feature-behavior` is READ-ONLY.

It MAY inspect:

- repository;
- GitHub issues/PRs;
- Decision Log / ADR / PDR;
- gameplay and technical specs;
- existing code;
- tests;
- Scenario Harness;
- scenario index/map;
- PIE/manual-test registry;
- roadmap/checkpoint documents;
- existing behavior contracts.

It MUST NOT:

- create/update/close an Epic;
- create/update/close an Issue;
- modify roadmap;
- modify code;
- create tests;
- create scenario files;
- create or modify `.uasset/.umap`;
- invoke Unreal authoring;
- acquire an Unreal lease;
- assign DEV / EDITOR / VALIDATION;
- create RT3 tasks;
- implement the feature.

If implementation work starts during this skill, the boundary has been violated.

---

# 1 — Evidence language

Always distinguish:

## FATTO

Directly present in an accepted source or measured behavior.

Examples:

- an existing spec says X;
- a test asserts Y;
- an existing scenario has ScenarioId Z;
- code currently rejects a certain input;
- an issue owns a certain capability.

## INFERENZA

A reasonable consequence of facts, not directly specified.

Never present an inference as a project decision.

## PROPOSTA

A behavior/design direction suggested by this analysis.

## DOMANDA

A decision required from the user or an existing project owner.

Do not hide missing information by inventing a rule.

---

# 2 — Source ownership

Do not use one universal source precedence for every statement.

First classify the claim.

Use, when applicable:

```text
design decision
    → Decision Log / ADR / explicit accepted decision

behavior semantics
    → live gameplay/technical spec owner

operational GitHub status
    → live GitHub

implemented behavior
    → code + tests + measured evidence

scenario identity/classification
    → Scenario Harness / Scenario Index owner

historical design intent
    → handoff / Drive / superseded docs
```

Historical documents are provenance, not automatic authority.

If accepted sources conflict:

```text
BEHAVIOR_SOURCE_CONFLICT
```

Record:

- conflicting claim;
- source A;
- source B;
- systems affected;
- smallest decision required.

Do not silently choose.

---

# 3 — Normalize the request

Convert `$ARGUMENTS` into:

```text
INTENT
PLAYER OUTCOME
CURRENT BEHAVIOR
DESIRED DELTA
NON-GOALS
UNKNOWN
```

Do not inflate a small behavior request into a large system.

---

# 4 — Reconnaissance

Before defining new behavior, search the current project.

Search by:

- player-visible outcome;
- domain terminology;
- existing feature names;
- class/function/type names when known;
- open issues;
- closed issues that may already have delivered it;
- Decision Log / ADR;
- specs;
- tests;
- ScenarioIds;
- PIE/manual-test IDs;
- roadmap references.

Primary question:

> Who already defines or verifies this behavior?

An existing closed issue can be evidence of previous implementation without being the current owner.

Scenario tags are discovery metadata only.

Do NOT use scenario tags as feature ownership or as a new gameplay ontology.

---

# 5 — Critical behavior review

Before proposing the contract, analyze the behavior as game design.

When relevant, evaluate:

## Player decision

- What decision is the player actually making?
- What information is available?
- What information is intentionally unavailable?
- Is the decision meaningful or merely procedural?

## Simultaneous turn impact

Separate:

```text
DECISION
PLANNING
EXECUTION
RESOLUTION
```

Check prediction, bluff/counterplay, telegraphing, conflicts, timing/order, recovery and readability.

## Multiplayer / ownership

Check whether behavior depends on player, controlled character, team, match, server authority or local presentation.

Do not assume "player" and "character" are the same owner.

## Determinism

Ask whether:

```text
same snapshot
+ same rules/version
+ same seed
+ same intents
= same outcome
```

must remain true.

## Privacy

Ask whether the behavior can leak enemy plans, hidden occupancy, hidden targets, hidden state, or information through reachability/warnings/preview.

"Not visible in UI" is not enough for privacy.

## Replay / TurnLog

Ask whether the behavior changes simulation or only presentation, must be represented in TurnLog, must survive replay/seek, or must remain absent from StateHash if presentation-only.

## UI/UX

Ask:

> Can the player understand why this happened?

If not, record a behavior/readability risk.

---

# 6 — Behavior Contracts

Split the requested feature into the minimum number of independently understandable behavior contracts.

A contract is NOT an issue.

One feature can have multiple contracts owned by different systems.

Use:

```markdown
## BC-01 — <short name>

### Intent
...

### Owner level
PLAYER | CHARACTER | TEAM | MATCH | PRESENTATION | TOOLING

### Given
...

### When
...

### Then
...

### Must not
...

### Failure behavior
...

### Ordering / timing
...

### Information boundary
...

### Determinism impact
...

### Replay / TurnLog impact
...

### Presentation requirement
...

### Open decisions
...
```

Owner level describes semantics, not GitHub ownership.

---

# 7 — Acceptance Criteria

Every relevant behavior statement must become falsifiable.

For every AC ask:

> What observation would prove this false?

If there is no clear falsification condition, the AC is not ready.

Prefer a small number of discriminating ACs over a long checklist of implementation details.

---

# 8 — Acceptance → Evidence

The fundamental relation is:

```text
Acceptance Criterion
        ↓
Evidence
```

NOT:

```text
Issue
  ↓
Scenario
```

Evidence types may include UNIT, AUTOMATION, SCENARIO, PIE, EDITOR, STATIC, DETERMINISM, REPLAY, PRIVACY, NETWORK, PACKAGED, PERFORMANCE and HUMAN.

Also classify oracle:

```text
MACHINE
HUMAN
MIXED
```

A scenario being associated with an AC does not automatically prove it.

---

# 9 — Scenario reconciliation

Inspect existing Scenario Harness coverage before proposing a new scenario.

Allowed status:

```text
REUSE
EXTEND
CREATE_CANDIDATE
DEFER
NOT_APPLICABLE
RETIRE_CANDIDATE
```

Use `REUSE` when an existing scenario genuinely exercises the correct fixture, runtime path, behavior and useful oracle.

Use `EXTEND` when the scenario is conceptually the same case and additional assertions keep its identity coherent.

Use `CREATE_CANDIDATE` only when the capability exists, Scenario Harness can exercise it through the real path, no existing scenario gives adequate coverage, regression value is real and the oracle is clear. This skill does not create the file.

Use `DEFER` when the subject does not exist yet. Do not create phantom ScenarioIds/files.

Use `NOT_APPLICABLE` when another evidence type is better.

Use `RETIRE_CANDIDATE` only to flag existing coverage that appears obsolete or misleading. Do not delete anything here.

---

# 10 — Machine vs human oracle

Visual setup and visual judgment are different things.

For mixed evidence:

```text
MACHINE
proves that the logical game state is the intended one

HUMAN
judges whether the player can understand it
```

Never turn human judgment into automatic PASS.

---

# 11 — Avoid scenario inflation

Do NOT create one scenario candidate per AC.

Prefer one discriminating fixture with multiple clear assertions when setup and purpose are shared and failures remain diagnosable.

Optimize for diagnostic value, regression value and runtime-path fidelity — not scenario count.

---

# 12 — Consequences and risks

For the final proposal, explicitly report when relevant:

```text
GAMEPLAY CONSEQUENCES
DESIGN RISK
TECHNICAL RISK
NETWORK / PRIVACY RISK
DETERMINISM / REPLAY RISK
UI/UX RISK
PRODUCTION / SCOPE RISK
```

Risk:

```text
CRITICAL
HIGH
MEDIUM
LOW
N/A
```

Prefer the smallest prototype that can test the hypothesis.

---

# 13 — Verdict

Allowed final verdicts:

```text
BEHAVIOR_READY
NEEDS_DECISION
NO_CHANGE
CONFLICT
INSUFFICIENT_EVIDENCE
```

`NO_CHANGE` is a successful result when reconnaissance proves the requested behavior already matches the accepted contract.

---

# 14 — Final output

Always finish with:

```markdown
# FEATURE BEHAVIOR REPORT

## INTENT

## FATTI

## INFERENZE

## PROPOSTA

## DOMANDE

## BEHAVIOR CONTRACTS

## ACCEPTANCE CRITERIA

## ACCEPTANCE → EVIDENCE

| AC | Evidence | Oracle | Existing? | Action |
|---|---|---|---|---|

## SCENARIO RECONCILIATION

| Scenario | Status | Reason |
|---|---|---|

## SYSTEM IMPACT

- Determinism:
- Replay:
- Privacy:
- Networking:
- UI/UX:
- Editor:
- Packaged:

## RISKS

## IMPLEMENTATION IMPLICATIONS

Only consequences useful to `/implement-feature`.
Do not assign issues or RT3 roles here.

## VERDICT

BEHAVIOR_READY | NEEDS_DECISION | NO_CHANGE | CONFLICT | INSUFFICIENT_EVIDENCE
```

If the verdict is `BEHAVIOR_READY`, explicitly state:

```text
NEXT: /implement-feature
```

Do not perform that next step automatically.
