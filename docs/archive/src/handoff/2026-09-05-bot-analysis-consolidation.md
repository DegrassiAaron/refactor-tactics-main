# RefactorTactics — Bot System Analysis & Claude Consolidation Brief

**Repository:** `DegrassiAaron/refactor-tactics-main`  
**Scope:** current bot architecture, observed/verified weaknesses, consolidation of GitHub issues/epics, preparation of correction/improvement work.  
**Purpose:** this document is a handoff artifact. It must not be treated as an authority over the live repository. Claude must verify current code, owner docs, roadmap and GitHub issue state before changing anything.

---

# 1. Executive summary

The current RefactorTactics bot is not fundamentally broken at the level of legality, determinism or basic micro-combat. The main problem is architectural: a reasonably capable **local per-unit move evaluator is currently being used as the tactical brain for the whole team**.

The strongest verified weaknesses are:

1. **No true joint team planning.** `PlanBots()` plans one bot/unit at a time, then reserves routes. This prevents real optimization of focus fire, setup/payoff, crossfire, objective coverage, redundancy and contested resources.
2. **One-turn greedy reasoning.** `ScorePlan()` scores the current turn. This makes positioning for a future turn difficult to value robustly and encourages locally safe or immediately profitable actions.
3. **Objective reasoning is weak/not integrated into the current core utility.**
4. **No-contact search is intentionally primitive.** The current fallback tends toward restoring LOS to a known contact or moving toward map center/centroid, without coordinated area search or memory of searched sectors.
5. **Threat evaluation is multi-enemy but coarse.** It penalizes exposure to each threatening enemy, but does not model expected combined damage, lethality, control or team-value loss.
6. **Kiter panic is a hard pre-utility behavior in current planning.** It can bypass normal candidate competition and may cause tactically poor escapes.
7. **Reaction response selection is primitive.** The current simple response path can choose the first legal `FIRE:` response rather than the tactically best one.
8. **Scalar-weight tuning has structural limits.** Existing evidence shows that simply adding another constant utility term can create impossible tuning ranges because threat and damage live on incompatible scales.
9. **Opponent modeling / belief / prediction are not current bot features by design.** They belong to later work and must not be smuggled into E26 as ad-hoc code.

The repository already recognizes the architectural gap in **E26 · Tactical Bot v1 (#326)**, whose goal is explicitly to move the bot from “plays legally” to “plays as a team”.

The correct direction is therefore **not to replace `URTHexBotLibrary` and not to keep stacking new `WWhatever` terms into `ScorePlan()`**. The current local scorer should remain a candidate generator / micro-evaluator while a deterministic team-level planner evaluates combinations.

---

# 2. Verified current architecture

## 2.1 `URTHexBotLibrary`

The current bot library is deterministic and largely isolated from Actor/UWorld concerns.

Relevant structures include:

- `FRTHexBotPlan`
- `FRTHexBotContext`

The plan/context include data such as:

- destination cell;
- attack/no attack;
- target;
- damage;
- target health;
- shape / area / range;
- friendly fire;
- origin cell;
- enemy positions/ranges/health/facing;
- allies;
- ability parameters;
- kiting standoff;
- utility weights;
- idle-turn information.

The current default utility terms include concepts for:

- lethal bonus;
- damage;
- ally damage / friendly fire;
- threat;
- kite violation;
- approach;
- elevation;
- engage;
- engage decay.

This is not a trivial “nearest target only” bot. It already contains useful tactical micro-logic.

## 2.2 Candidate generation

Current candidate construction covers, depending on legality/availability:

- movement;
- attacks;
- offensive abilities;
- charge;
- dash + offensive ability where supported;
- dash repositioning;
- reactions.

`BuildCandidates` / current planning code should remain the source of legal local candidates unless the live repository now says otherwise.

## 2.3 Local scoring

`ScorePlan()` currently evaluates concepts including:

- exact attack geometry;
- damage;
- lethal bonus;
- directional cover bypass utility;
- ally/friendly-fire damage;
- per-enemy threat exposure;
- graph-distance based approach/kiting;
- engagement/LOS;
- elevation.

`ApproachSteps` uses graph-aware distance/caching with fallback behavior.

## 2.4 Tie-breaking

Current best-plan selection is deterministic.

On exact utility tie, the implementation prefers lower movement distance, which means “stay” can win an exact tie.

This is useful for stability but can amplify passive-looking behavior when the utility function does not value future positioning strongly enough.

---

# 3. Critical findings

## 3.1 Team coordination is structurally weak

### Current behavior

`PlanBots()` plans one unit at a time.

For each bot it roughly:

1. builds authorized team knowledge;
2. builds reachable/candidate plans;
3. scores/selects that unit’s best plan;
4. reserves the selected movement/path;
5. proceeds to the next bot.

The later unit reacts to reservations created by the earlier one, but **the system does not evaluate the pair/team plan as a single tactical object**.

### Consequence

The bot cannot correctly optimize cases such as:

- two units focus-fire one target but avoid overkill;
- one unit performs setup while another performs payoff;
- one unit yields a resource to another with greater marginal value;
- coordinated crossfire;
- coordinated objective coverage;
- one unit moves because another unit’s chosen action already guarantees a kill;
- redundant control / redundant movement;
- tactical role swapping.

Sequential reservations solve mainly spatial interference. They do not create tactical teamwork.

### Classification

**Architectural limitation — E26.**

---

# 4. One-turn greedy utility is a real limitation

The current local scorer mostly evaluates value inside the current turn.

That creates a known problem:

> moving into a tactically useful future position may create immediate exposure cost without an immediate attack payoff.

Historic/current repository evidence already identified this class of failure in issue **#149**.

Important: much of #149’s original charge scenario is now legacy/superseded by roster/action changes. Do **not** copy the historical scenario as if it were still current.

What remains valid is the deeper finding:

- `WThreat` is 100 per threat in the documented example;
- damage utility is `WDamage × damage`;
- a simple fixed “future in-range” bonus can require a value greater than 200 to beat two threats, while needing to remain below 200 to avoid beating a real 20-damage attack.

That means a new constant utility term cannot necessarily be tuned consistently.

### Conclusion

Do not solve tactical intelligence by adding a large collection of new scalar constants to `ScorePlan()`.

Use:

- better structured features;
- team-level marginal utility;
- opportunity completion;
- temporal compatibility;
- future-looking layers in their intended epic/scope.

---

# 5. Threat model: correction and current critique

An earlier simplistic characterization such as “the bot only looks at the nearest enemy” would be incorrect.

Current scoring penalizes exposure to **each enemy that can threaten the candidate destination**.

The actual weakness is different:

- threat is multi-enemy;
- but it is relatively coarse/binary;
- it does not adequately represent expected damage;
- it does not represent combined lethality;
- it does not represent control/disable severity;
- it does not represent tactical value of losing a particular unit;
- it does not naturally model focus-fire risk.

Nearest enemy is still relevant for approach/kiting behavior, but not as the sole threat source.

### Classification

Mostly **model/tuning limitation**, with some parts better handled later by predictive work rather than inflating E26.

---

# 6. Kiter panic requires explicit behavioral verification

The current planning path contains a hard “panic” style guard for a kiter when the nearest enemy gets sufficiently inside its desired standoff.

Conceptually this can trigger around:

`NearestDistance <= Standoff / 2`

The important architectural concern is that this branch can run **before the ordinary utility pool fully decides the action**.

Potential bad behavior:

- fleeing from a guaranteed lethal;
- fleeing when a high-value attack would be safe;
- abandoning a critical objective;
- spending dash to escape when another action has greater tactical value.

This is not yet proven to be a current runtime bug in every case.

### Required action

Create/verify behavioral scenarios for:

- kiter panic vs guaranteed lethal;
- kiter panic vs objective capture;
- kiter panic vs safe high-value attack.

If the guard overrides a clearly superior legal candidate, classify it as a **current behavior defect or design flaw**, not automatically as E26.

---

# 7. Objective blindness

Current planning does not appear to contain a serious integrated objective utility equivalent to the team-level strategic value needed by the game.

In no-contact/search behavior, existing code/comments already acknowledge primitive fallback logic such as:

- try to regain LOS to known contact;
- otherwise move toward map center/centroid.

Future concepts such as:

- `SecureObjective`;
- `GatherInformation`;

have been discussed/planned rather than forming a complete current planner.

### Consequences

A legal bot can still look very stupid because it:

- ignores a winning objective opportunity;
- chases an enemy when it should capture/deny;
- abandons objective coverage;
- sends both units into the same search path.

### Classification

Core tactical-team objective reasoning belongs naturally in **E26**, while richer hidden-information search belongs later.

---

# 8. Search without enemy contact is intentionally primitive

Current no-contact behavior does not provide:

- coordinated sector assignment;
- memory of already-searched areas;
- information gain scoring;
- complementary search paths;
- uncertainty-aware exploration.

Moving toward a centroid is legal and deterministic but not tactically intelligent.

Do not “fix” this with access to true hidden enemy positions.

The bot must continue to respect the same authorized team knowledge available under game rules.

### Scope guidance

- basic team search coordination can be represented in E26 if based only on authorized knowledge;
- belief maps / probabilistic hidden-state reasoning belong to E27;
- opponent prediction belongs to E28.

---

# 9. Reactions are still weak

Current reaction arming/selection has some scoring, but the simple response selection path is primitive.

A key case to verify is whether `DecideReactionResponse()` still effectively chooses the first legal `FIRE:` response.

If so, this can produce:

- firing at a low-value target instead of a lethal;
- firing at a tank instead of a vulnerable target;
- failing to exploit a high-value control interaction.

### Classification

A better reaction policy is explicitly part of E26.

A small isolated correctness bug can be fixed earlier if discovered, but do not build a second tactical planner inside reactions.

---

# 10. Existing E26 architecture must be preserved

## Epic #326 — Tactical Bot v1

The live Epic already fixes key architectural decisions.

### D-097

**Top-K per unit + centrally evaluated combination.**

Tactical roles such as:

- Setup;
- Payoff;
- Denier;
- ObjectiveRunner;

must **emerge from the selected plan**, not be assigned as input roles.

Contested resources are decided by **marginal utility**.

`StableUnitId` is a tie-break only.

### D-098

An intra-turn synergy counts only if the real phase order allows the setup to affect the payoff.

The bot asks the rules whether the pair is compatible.

It must not duplicate/re-implement phase order.

### D-096

Deterministic behavior uses fixed count budgets, not wall-clock milliseconds.

Time can determine when replanning is stopped, but never alter which plan wins for the same considered search space.

### D-095

The planner core is a custom utility planner.

StateTree may orchestrate macro behavior.

Behavior Tree / EQS are not tactical authority.

---

# 11. Existing E26 checkpoints

## #531 — CP 26.1 Team Planner: Top-K per unit and central combination

This checkpoint already covers the main structural correction.

Expected direction:

- current candidate pipeline remains;
- candidates are retained with diversity buckets;
- each unit exposes Top-K;
- deterministic team combinations are evaluated centrally;
- team score includes local scores plus synergy/opportunity/conflict/redundancy/risk concepts;
- deterministic fixed-count budget;
- permutation invariance test.

**Do not create a duplicate issue for “implement team planning”. Update/extend #531 only if the live analysis identifies missing acceptance criteria.**

## #532 — CP 26.2 Temporal compatibility

This checkpoint covers real resolver-compatible synergy.

Expected direction:

- shared pure predicate for temporal compatibility;
- same rule used by planner and other consumers;
- no duplicated phase constants in bot code;
- tests for both impossible and possible versions of the same synergy.

**Do not create a duplicate “water + electricity combo ordering” issue if it is already covered here.**

## #533 — CP 26.3

Expected responsibility from E26:

- hard conflicts;
- soft conflicts;
- redundancy;
- contested resource allocation;
- marginal utility.

Claude must fetch and inspect the live issue before changing it.

## #534 — CP 26.4

Expected responsibility from E26:

- hysteresis;
- stable replanning.

Claude must fetch and inspect the live issue before changing it.

---

# 12. Existing issue #149 must be handled carefully

Issue #149 contains valuable historical evidence but is partially obsolete.

### Superseded portions

Legacy assumptions/scenarios around Guardian / old charge composition are no longer current.

Current roster naming and abilities changed.

### Still-valid portion

The structural utility-scale finding remains valuable:

> a simple positioning bonus cannot necessarily be tuned as one constant against current threat/damage scales.

### Recommended consolidation action

Claude should inspect the current issue and decide whether to:

- update its body to make the surviving finding more prominent;
- close superseded parts if the repository’s issue conventions support that;
- move the live architectural implication into the appropriate E26 checkpoint;
- preserve historical evidence without making it look like current runtime behavior.

Do not delete useful history.

---

# 13. Required behavioral tests / scenario coverage

The bot improvements should be driven by visible tactical scenarios and Automation Tests.

At minimum consolidate coverage for:

## Team planning

1. **Focus fire without overkill**
   - Example: two allies can deal 15 + 15 to a 20 HP target.
   - If the first selected action guarantees the KO under actual phase ordering, the second unit should choose its best alternative rather than waste damage.

2. **Candidate diversity**
   - Strong offense must not prune all control/objective/setup options before team scoring.

3. **Unit permutation invariance**
   - Reordering bot units must not change the selected TeamPlan except where explicit deterministic tie-breaking applies.

4. **Hard conflict rejection**
   - TeamPlan rejects mutually impossible movement/resource combinations.

5. **Contested resource by marginal utility**
   - The unit deriving the highest team-level marginal value receives the contested resource.

## Objective behavior

6. **Objective capture vs immediate damage**
   - A winning/decisive objective play must beat a low-value attack when rules/score say it should.

7. **Objective denial**
   - Team planner recognizes denial as team value, not only damage.

## Synergy

8. **Setup/payoff**
   - A valid setup/payoff combination receives team synergy value.

9. **Temporal invalidity**
   - Same nominal combo receives no bonus if resolver phase order makes it impossible.

10. **Water + electricity**
    - Use only real current rules and the shared phase-compatibility predicate.

## Kiting

11. **Kiter panic vs guaranteed lethal**
    - A forced panic path must not suppress a clearly superior lethal unless the design explicitly requires that behavior.

12. **Kiter panic vs high-value objective**
    - Verify whether the current hard guard should become a candidate/penalty rather than an override.

## Threat

13. **One threat vs three threats**
    - Candidate preference should visibly react to aggregated tactical danger.

14. **Lethal threat vs weak threat**
    - Establish whether current scoring needs structured risk rather than another flat constant.

## Reaction

15. **Reaction chooses best target**
    - Legal response selection must not merely choose the first stable ID if another legal response is clearly superior.

## No-contact search

16. **Two bots search complementary areas**
    - No true enemy information is allowed.
    - Baseline E26 search coordination can avoid duplicating the same search effort.
    - Belief/probability remains E27.

---

# 14. What should NOT be done

Do not:

- replace `URTHexBotLibrary` wholesale;
- create a parallel candidate generation system;
- make Behavior Tree/EQS the tactical authority;
- add a long list of ad-hoc `WWhatever` constants as the primary solution;
- give bots private enemy intents;
- use true hidden enemy positions;
- duplicate resolver phase ordering in Bot code;
- introduce non-deterministic wall-clock cutoffs into candidate selection;
- assign tactical roles before plan evaluation if D-097 remains current;
- implement belief/prediction inside E26;
- create duplicate issues because the existing title is phrased differently;
- rewrite archived documents as current authority.

---

# 15. Priority recommendation

The three highest-impact changes for perceived bot intelligence are:

## Priority 1 — TeamPlan architecture

Implement/complete E26 CP 26.1:

- diverse Top-K candidate set per unit;
- central deterministic combination;
- team-level utility;
- overkill/redundancy/conflict awareness.

This is the biggest qualitative leap.

## Priority 2 — Objective + synergy value at team level

Use the TeamPlan to evaluate:

- objective capture/denial;
- setup/payoff;
- cross-unit tactical opportunity completion;
- temporal compatibility through live rules.

Do not force these into the local per-unit scorer when their value only exists as a combination.

## Priority 3 — Remove/contain hard-coded behavioral shortcuts

Measure and correct cases where hard paths bypass normal tactical competition, especially:

- kiter panic;
- primitive reaction response;
- primitive no-contact fallback.

Where possible, convert absolute shortcuts into candidates/constraints/structured utility rather than hidden tactical authority.

---

# 16. Instructions for Claude — consolidate GitHub issues/epics

Use the following instructions as an execution prompt.

---

## CLAUDE TASK

Work on repository:

`DegrassiAaron/refactor-tactics-main`

Your goal is to **consolidate the current bot-analysis findings into the live roadmap and GitHub issues**, then create/update only the issues required to execute the corrections and improvements.

This is an issue/roadmap consolidation task first, implementation second.

### Core rule

**The live repository is authoritative.**

This document is input evidence, not authority.

Before modifying GitHub:

1. pull/fetch the latest repository state;
2. inspect current branch and HEAD;
3. inspect live owner docs;
4. inspect current roadmap;
5. search all bot-related GitHub issues, open and closed;
6. fetch the full body and comments of relevant issues;
7. inspect recent PRs/commits touching bot planning if useful;
8. inspect current code before asserting that a defect still exists.

Do not act from issue titles alone.

---

# 17. Owner documents to inspect first

At minimum locate and read the current versions of:

- `docs/gameplay/spec-bot-tattico.md`
- current bot v0.1 specification, if still present
- `docs/roadmap/roadmap-v0.1.md`
- `docs/roadmap/roadmap-checkpoint.md`
- post-v0.1 roadmap / E26 owner section
- current Definition of Done / gates relevant to v0.1 and E26
- decision/ADR documents for D-095, D-096, D-097, D-098 if still live
- current Turn/Resolver phase-order authority

Archived consolidation documents may be read for provenance but are **not live authority**.

Do not treat the old Feature Registry as current if the live repo confirms it was removed.

---

# 18. Code to inspect

At minimum verify current state of:

- `RTHexBotLibrary.h`
- `RTHexBotLibrary.cpp`
- `RTTurnManager.cpp`
- `PlanBots()`
- candidate generation
- `ChooseBestPlan()`
- `ScorePlan()`
- `ReservePlannedRoute()`
- `BestKiteCell()`
- reaction planning/response
- TeamKnowledge access
- no-contact fallback
- objective integration
- bot-related Automation Tests
- scenario/PIE bot-vs-bot harness

Search by symbol rather than assuming paths remained unchanged.

---

# 19. GitHub items that must be inspected before creation

At minimum fetch:

- **#326 — E26 Tactical Bot v1**
- **#531 — CP 26.1**
- **#532 — CP 26.2**
- **#533 — CP 26.3**
- **#534 — CP 26.4**
- **#149 — bot balancing / utility-scale historical issue**
- **#327 — E27 belief**, if still current
- **#328 — E28 predictive**, if still current

Then search for all current issues containing concepts such as:

- bot;
- TeamPlan;
- Top-K;
- overkill;
- focus fire;
- objective;
- reaction;
- kite/kiter;
- search/contact;
- threat;
- synergy;
- water/electricity;
- candidate diversity;
- tactical planner;
- reservation/conflict;
- hysteresis.

Search both OPEN and CLOSED issues.

---

# 20. Consolidation rules

For every finding in this document, classify it as one of:

- already fully covered by an existing live issue;
- partially covered;
- obsolete;
- duplicate;
- missing;
- current v0.1 bug;
- E26 improvement;
- E27 belief work;
- E28 predictive work;
- tuning/measurement task.

Do not create an issue until this classification exists.

Prefer:

**update existing > reopen if appropriate > create new**

Never create a second issue just because the wording differs.

---

# 21. Specific consolidation expectations

## A. Team planning

The finding “bots plan units sequentially rather than selecting a team combination” should normally map to:

- E26 #326
- CP 26.1 #531

If #531 still accurately captures it, do not create another TeamPlanner issue.

Instead update #531 only if necessary to include missing acceptance criteria such as:

- focus-fire without overkill;
- objective alternative surviving candidate pruning;
- diagnostic TeamPlan trace;
- visible 2v2 behavioral scenario.

Preserve D-097.

---

## B. Temporal synergy

Map resolver-compatible setup/payoff behavior to #532.

Do not create separate issues for each elemental pair unless the issue represents:

- a rules bug specific to that interaction, or
- a missing data definition outside the generic planner capability.

Planner temporal compatibility itself belongs to CP 26.2.

Preserve D-098.

---

## C. Conflicts, redundancy and contested resources

Map to #533 if its live scope matches.

Ensure the live ticket covers, where appropriate:

- overkill;
- redundant target damage;
- mutually conflicting movement;
- duplicate control;
- contested cell/resource;
- marginal team utility;
- deterministic resolution.

Do not create a generic “bot coordination” issue if #533 already owns these concepts.

---

## D. Hysteresis

Map stable replanning to #534.

Do not mix generic passivity/stalling with hysteresis unless the evidence really shows replanning oscillation.

---

## E. Objective utility

Determine whether objective-aware TeamPlan scoring is already explicitly owned by #531/#533 or another E26 issue.

If only implied and there is no measurable DoD, either:

1. update the most appropriate existing E26 checkpoint; or
2. create one focused child issue under #326.

A new issue is justified only if it has a clear independent DoD and does not duplicate a checkpoint.

Required behavior should include at least one objective-vs-damage scenario.

---

## F. Kiter panic

Inspect current code first.

If the hard kiter guard still bypasses normal utility:

- create or update a focused **current behavior issue** if it produces incorrect decisions in v0.1;
- attach reproduction scenario;
- require an Automation Test;
- define expected behavior without prescribing an architecture prematurely.

Do not automatically bury this in E26 if it is a current defect.

If the behavior is intentional v0.1 policy and only E26 should improve it, document that instead.

---

## G. Reaction response

Inspect current response selection.

If “first legal FIRE response” is still the policy:

- confirm whether E26 already explicitly owns reaction-policy improvement;
- add measurable acceptance criteria to the existing E26 scope if absent;
- create a child issue only if the work is independently implementable/testable.

Required scenario:

- two legal reaction targets;
- one clearly dominates;
- bot selects the higher-value legal response deterministically.

---

## H. No-contact search

Do not accidentally implement E27 inside E26.

Separate:

**E26:**
- coordinated deterministic search using known/public/team-authorized information;
- avoid redundant movement where possible.

**E27:**
- belief map;
- uncertainty distribution;
- probabilistic/structured hidden-state reasoning.

**E28:**
- enemy action/policy prediction.

If issue scope mixes these layers, fix the issue wording.

---

## I. Threat evaluation

Do not create a vague “improve WThreat” ticket.

First gather behavioral evidence.

If needed, create/update a measurement/design issue with scenarios such as:

- 1 weak threat;
- 3 simultaneous threats;
- lethal threat;
- control-heavy threat.

The goal is to determine whether to introduce structured expected risk, not just retune a constant.

---

## J. Issue #149

Treat #149 as a mixed historical/live issue.

Preserve the useful historical evidence.

Do not reintroduce legacy Guardian assumptions.

The surviving utility-scale finding should either remain clearly marked as current or be referenced from the live planner design issue.

If most DoD items are obsolete, consider updating the body/status according to repository conventions rather than leaving an apparently current but misleading task.

---

# 22. Issue quality requirements

Every issue you create or substantially update must contain:

## Context

What player-visible bot failure this fixes.

## Current evidence

Code/log/scenario evidence from current HEAD.

## Root cause

Bug, model limitation, tuning, or architectural gap.

## Scope

Concrete work included.

## Non-goals

Especially E26 vs E27 vs E28 boundaries.

## Determinism constraints

Same state/rules/version => same result.

Fixed-count search budgets only.

## Privacy constraints

No hidden enemy intent or unauthorized state.

## Acceptance criteria

Behavioral and measurable.

## Automated tests

Exact intended test/scenario names where possible.

## Debug/observability

How to explain why a TeamPlan/candidate won.

## Dependencies

Epic/checkpoint/owner docs.

## Done

Must include packaged/runtime validation where appropriate, not only unit tests.

---

# 23. Required diagnostic observability

If not already owned by a live issue, ensure the work plan includes enough debug output to inspect:

- Top-K candidates per unit;
- local score decomposition;
- candidate bucket;
- selected TeamPlan;
- team score decomposition;
- synergy bonus;
- objective value;
- conflict penalties;
- redundancy/overkill penalty;
- contested-resource decision;
- rejected combinations and reason;
- deterministic tie-break.

This should be debug/development instrumentation, not replicated tactical information to enemy clients.

---

# 24. Recommended test names

Reuse existing naming conventions if the repository has them.

Useful intended behaviors include:

- `Spec.Bot.TeamPlanRejectsHardConflict`
- `Spec.Bot.CandidateDiversityKeepsControl`
- `Spec.Bot.TemporalSynergyRequiresPhaseOrder`
- `Spec.Bot.OverkillMovesSecondUnit`
- `Spec.Bot.PlanHysteresisIgnoresSmallDelta`

Additional names may be introduced if missing, for example:

- `Spec.Bot.TeamPlanPrefersObjectiveWin`
- `Spec.Bot.KiterDoesNotFleeGuaranteedLethal`
- `Spec.Bot.ReactionChoosesHigherUtilityTarget`
- `Spec.Bot.NoContactSearchAvoidsDuplicateCoverage`
- `Spec.Bot.TeamPlanIsUnitPermutationInvariant`

Follow repository test naming rules if they differ.

---

# 25. GitHub mutation procedure

Before editing:

1. produce a temporary consolidation matrix:

| Finding | Existing issue | Status | Action |
|---|---|---|---|

2. show/record why each mutation is needed;
3. update existing issues first;
4. create only genuinely missing issues;
5. link child issues to #326 where appropriate;
6. preserve milestone/release scope;
7. preserve labels unless they are demonstrably stale;
8. do not move E26 work into v0.1 casually;
9. do not silently remove prior decisions/rationale.

After changes, produce:

| Issue | Action | Reason | New/updated DoD |
|---|---|---|---|

---

# 26. Priority order for execution

Unless the live roadmap forbids opening E26 yet, the preferred technical order is:

1. **diagnostic/behavioral baseline**
2. **CP 26.1 — Top-K + TeamPlan**
3. **overkill/conflict/redundancy/objective team utility**
4. **CP 26.2 temporal synergy**
5. **reaction improvements**
6. **kiter hard-guard correction if still current**
7. **no-contact coordination**
8. **CP 26.4 hysteresis**
9. **E27 belief**
10. **E28 prediction**

However:

**Roadmap gates override this order.**

If E26 is still formally blocked by v0.1 gates, do not violate the roadmap. Prepare/update the issues and execute only current-scope defects/diagnostics that are allowed.

---

# 27. Expected final output from Claude

At the end, report:

## Repository state inspected

- HEAD
- branch
- relevant owner docs
- relevant code files

## Consolidation matrix

Every finding from this document mapped to a live issue/scope.

## GitHub changes made

For each issue:

- created / updated / reopened / unchanged;
- why;
- links/issue numbers;
- milestone/labels;
- DoD changes.

## Duplicates avoided

List issues that were *not* created because existing work already owns them.

## Remaining uncertainties

Only evidence-based unknowns.

## Recommended implementation sequence

No more than 5 immediate tasks.

## First implementation target

State exactly which issue should be implemented first once roadmap gates allow it, and why.

---

# 28. Final instruction to Claude

Do not optimize the bot by intuition.

Do not tune weights until the behavior is reproduced and measured.

Do not confuse “legal and deterministic” with “tactically intelligent”.

The central hypothesis to validate is:

> The current bot’s biggest perceived-intelligence problem is not the quality of its local legal move generation; it is the absence of a team-level planner that evaluates combinations and strategic opportunity.

Use the live code, logs, tests and current issue graph to prove or disprove that hypothesis, then make the GitHub plan reflect reality.
