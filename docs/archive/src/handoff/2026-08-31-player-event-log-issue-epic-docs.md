# Claude — RefactorTactics Player Event Log: create/update GitHub issues + docs

> ## 📸 `HISTORICAL` — SORGENTE CONSUMATO, NON NORMATIVO
>
> Work order arrivato come `Claude_RefactorTactics_PlayerEventLog_Issue_Epic_Docs_2026-08-31.md`,
> **untracked**, e consumato il **2026-08-31** dal referto
> [`../../../roadmap/plans/player-event-log-spec-panel-2026-08-31.md`](../../../roadmap/plans/player-event-log-spec-panel-2026-08-31.md).
>
> **Non è una fonte**, ed è il kit meglio misurato della sua famiglia: nove owner citati su nove **aperti**,
> dodici percorsi citati su dodici **esistenti**. Quello che sbaglia è dove smette di proporre e crede di
> descrivere — la «authorized knowledge view» che mette a monte del proiettore, in produzione, emette
> `TArray<FString>` (`ComposeVisibleLogLines`); la sua regola di privacy vieta l'annuncio di morte che
> **D-223** rende pubblico e un test verde protegge; e il titolo che impone come vincolante non compare in
> nessuna delle **771** issue del repository. Conservato per la **provenienza** — e perché la sua pipeline,
> la tassonomia `Minor/Important/Critical` e le regole di dominanza sono buone e sopravvivono nel §5 del
> referto, che è ciò che di normativo resta.

**Date:** 2026-08-31  
**Repository:** `DegrassiAaron/refactor-tactics-main`  
**Unique marker:** `RT-PLAYER-EVENT-LOG-2026-08-31`

## Mission

Consolidate the HUD/log decision into GitHub and live repository documentation **idempotently**.

Create or update:

1. one **v0.1 implementation issue** under the existing E11 HUD ownership;
2. one **cross-release epic through v1.0**;
3. the associated live technical documentation;
4. backlinks to the existing owners without destructively rewriting their history.

Do **not** implement gameplay/runtime code in this run unless explicitly requested after the tracking/docs work is complete.

The source-of-truth hierarchy is:

1. live repository ADRs / decisions / owner docs;
2. live code and tests;
3. live GitHub issues;
4. this handoff.

If the repository disagrees with this file, **measure and preserve repository truth**, then document the delta.

---

# 0. Mandatory preflight

```bash
git fetch --prune origin
git status --short
git rev-parse --short HEAD
gh auth status
gh repo view DegrassiAaron/refactor-tactics-main
```

Stop before any write if:
- the repository is not `DegrassiAaron/refactor-tactics-main`;
- `gh` cannot write issues;
- the working tree contains unrelated modifications that would be overwritten.

Do not invent `D-nnn`, `E<n>`, or a new checkpoint number.

The cross-release epic must follow the repository precedent used by cross-release epics such as `#1881`: **`[EPIC]` without an `E<n>` number**.

---

# 1. Existing owners that MUST remain distinct

Read these before creating anything:

```bash
gh issue view 25
gh issue view 79
gh issue view 613
gh issue view 1466
gh issue view 1496
gh issue view 265
gh issue view 268
gh issue view 1881
```

Also inspect:

```text
docs/technical/systems/progettazione-hud.md
docs/technical/architecture/spec-turnlog.md
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-post-v0.1.md
Source/RefactorTactics/UI/RTHUD.cpp
Source/RefactorTactics/UI/RTHUD.h
Source/RefactorTactics/UI/RTScreenHudWidgets.h
Source/RefactorTactics/UI/RTScreenHudWidgets.cpp
Source/RefactorTactics/Turn/RTTurnLog.h
Source/RefactorTactics/Turn/RTTurnManager.h
Source/RefactorTactics/Tests/RTCombatLogTests.cpp
```

## Ownership invariant

- **#79 remains the detailed diagnostic/combat log.** Its contract deliberately includes `ActionId`, `Priority`, axial coordinates `(q,r,L)`, target, reason/validation details and TurnLog order. DO NOT simplify it.
- **#613 owns Screen HUD / UMG (§4.1)**.
- **ARTHUD must retain Tactical World Overlay (§4.2)**: path, AoE, facing, world-space unit info and other map overlays are not moved into a static screen HUD just to delete the legacy Canvas panels.
- **#1466 / #1496 own knowledge/privacy semantics**. The Player Event Log consumes an already-authorized view/verdict; it must not invent a second knowledge model.
- **#265 / #268 own semantic icon language**.
- **#1881 / #472 own playback/replay transport/inspection**. The Player Event Log may become a consumer, never a second simulator.

---

# 2. Search first — CREATE OR UPDATE, never duplicate

Search both title fragments and the unique marker:

```bash
gh issue list --state all --limit 1000 --search "Player Event Log"
gh issue list --state all --limit 1000 --search "legacy Canvas"
gh issue list --state all --limit 1000 --search "RT-PLAYER-EVENT-LOG-2026-08-31"
```

Treat an existing issue as equivalent if its **semantic scope** matches, even if wording differs.

If equivalent:
- update it;
- preserve useful history;
- add an additive reconciliation section rather than deleting dated rationale.

If not equivalent:
- create it.

---

# 3. v0.1 issue

## Required title

`[v0.1][HUD] Dismettere il Canvas Screen HUD legacy e introdurre il Player Event Log sintetico`

This is **not a new CP 11.7**. It is a residual implementation issue under E11 / Screen HUD.

## Required metadata

- Parent / owner: **#25 — E11 HUD, log e debug**
- Related: **#613, #79, #1466, #1496**
- Milestone/label: use repository existing **v0.1** conventions after inspecting live metadata.
- Priority: use **P1** only if still consistent with E11 current policy.
- Assignee: keep repository convention; do not invent a user.

## Required body

```markdown
> **Parent**: #25 · **Screen HUD owner**: #613 · **Detailed diagnostic log**: #79  
> **Privacy/knowledge owners**: #1466 · #1496  
> **Marker**: `RT-PLAYER-EVENT-LOG-2026-08-31`

## Problem

The project currently has two screen-space presentation paths overlapping in purpose:

1. legacy Canvas panels in `ARTHUD`;
2. the UMG Screen HUD.

The legacy Canvas combat/log presentation is useful as developer diagnostics but is not a good player-facing event feed: it exposes low-level detail such as cells, reason codes and intermediate events.

The v0.1 needs a strict separation:

- **debug/audit**: complete, detailed, developer-facing;
- **Player Event Log**: concise, grouped, player-facing.

#79 remains deliberately detailed and is NOT simplified.

## Architectural boundary

```text
Resolver authoritative
        ↓
TurnLog structured / deterministic
        ↓
authorized knowledge/privacy view
        ↓
Player Event Projector
        ↓
FRTPlayerEvent[]
        ↓
UMG Player Event Log
```

Never:

- parse debug strings to infer player events;
- let a widget recompute gameplay, LOS, targeting, damage or reason codes;
- project private enemy planning and filter it only after text generation;
- make the Player Event Log an authority.

## Scope v0.1

### A — Remove the legacy Canvas Screen HUD

Delete the legacy **screen-space** Canvas panels and the compatibility CVar controlling them, including the legacy:

- turn/header panel;
- bottom-left combat/event text;
- ability/action bar;
- slot trio / equivalent duplicate screen panels.

Remove `rt.HUD.CanvasPanels` if it no longer controls anything.

**Preserve `ARTHUD::DrawHUD` for Tactical World Overlay (§4.2)**:
path, waypoint, destination, AoE, friendly-fire marks, facing, cones, world-space bars/labels and other map overlays that belong in the scene.

### B — Keep developer diagnostics

Preserve:

- canonical `TurnLog`;
- `UE_LOG` diagnostics;
- `rt.Debug.*`;
- detailed reason-code/coordinate output required by #79.

Player presentation must not weaken observability.

### C — Add typed Player Events

Introduce a presentation contract equivalent to:

```cpp
enum class ERTPlayerEventImportance : uint8
{
    Minor,
    Important,
    Critical
};

struct FRTPlayerEvent
{
    ERTPlayerEventType Type;
    ERTPlayerEventImportance Importance;
    int32 PrimaryUnitId;
    int32 SecondaryUnitId;
    FName ActionId;
    int32 Amount;
    // localization-ready semantic arguments are preferred to a text-only authority
};
```

Exact field names may adapt to live conventions. Do not duplicate stable-id wrappers/types that already exist.

Add a pure/projector-style component such as:

`URTPlayerEventProjector`

The projector derives presentation events from structured authorized facts.

### D — Initial aggregation policy

**Minor / normally silent**
- micro-step;
- individual cell crossed;
- technical facing update;
- LOS/check bookkeeping;
- cleanup;
- ordinary successful movement with no tactical consequence.

**Important**
- ability use;
- damage/heal;
- significant shield interaction;
- Dash;
- Overwatch / reaction outcome;
- Predictive hit/whiff;
- interruption;
- blocked movement;
- meaningful status/hazard;
- objective state change.

**Critical**
- KO;
- objective captured/lost;
- score/result change;
- decisive reaction;
- match end.

### E — Dominance / grouping

Baseline:

```text
KO > Damage > Hit
Blocked/Interrupted > Move
Reaction committed/fired > Reaction opened
Objective captured/lost > objective progress bookkeeping
```

Movement must never emit one player row per cell.

Examples:

```text
Phase si sposta.
Phase viene fermata dall'Overwatch di Wraith.
Wraith colpisce Phase · 24 danni.
Riktor prende fuoco.
Squadra Blu conquista Relay A.
```

Ordinary movement may be completely silent because the player already sees it animate.

Environment propagation is grouped semantically: no "water cell A / water cell B / electricity cell C" spam.

### F — UMG

Player Event Log:
- compact;
- bottom-right;
- **above** Undo/Confirm/plan controls;
- center of the map remains free;
- initial budget: max ~4 recent events;
- no permanent large scroll list in normal match HUD;
- debug display off by default.

## Privacy

Authorization happens **before** player-event projection.

A private or unknown enemy fact must not leak through:
- text;
- icon;
- count;
- event type;
- hidden actor identity;
- timing metadata.

The projector consumes the existing knowledge/privacy contract; it does not redefine it.

## Definition of Done

- [ ] Legacy Canvas **screen-space** panels removed, including dead compatibility CVar/code.
- [ ] Tactical World Overlay remains functional in `ARTHUD`.
- [ ] #79 detailed diagnostic contract remains intact.
- [ ] Typed Player Event contract exists.
- [ ] Player projector consumes structured authorized data, not formatted debug strings.
- [ ] Movement does not spam cells.
- [ ] Dominance rules prevent duplicate `Hit + Damage + KO` player narration.
- [ ] Compact UMG event log is integrated into `WBP_RT_TacticalHUD` above Confirm/Undo.
- [ ] Debug/player views are distinct and debug is off by default.
- [ ] Packaged Development launches without legacy/debug overlay enabled by default.
- [ ] Privacy test proves unauthorized enemy facts produce no player event.

## Automation

Keep existing:

- `RefactorTactics.UI.LogContainsReasonAndCoords`
- `RefactorTactics.UI.LogMatchesTurnLogOrder`

Add equivalent tests:

- `RefactorTactics.UI.PlayerEventLog.CollapsesMoveCells`
- `RefactorTactics.UI.PlayerEventLog.OmitsMinorMovement`
- `RefactorTactics.UI.PlayerEventLog.KODominatesDamage`
- `RefactorTactics.UI.PlayerEventLog.GroupsEnvironment`
- `RefactorTactics.UI.PlayerEventLog.OmitsUnauthorizedFacts`
- `RefactorTactics.UI.PlayerEventLog.PreservesSemanticOrder`

Use repository naming conventions if the live suite uses a different prefix.

## PIE acceptance

1. Start a normal match.
2. No legacy bottom-left Canvas Screen HUD.
3. Tactical world overlays still work.
4. A normal move produces no cell-by-cell player spam.
5. Dash / block / attack / KO / objective produce concise meaningful rows.
6. At most the configured recent-event budget is shown.
7. Player Event Log does not overlap Confirm/Undo.
8. Developer TurnLog/debug output still contains full detail.

## Non-goals

- simplify the canonical TurnLog;
- remove coordinates/reason codes from debug;
- build the complete replay/history UI;
- add gameplay rules;
- create a new phase;
- solve full localization in v0.1.

## Suggested files

- `Source/RefactorTactics/UI/RTHUD.{h,cpp}`
- `Source/RefactorTactics/UI/RTScreenHudWidgets.{h,cpp}`
- new `Source/RefactorTactics/UI/RTPlayerEvent*.{h,cpp}` if justified
- tests under `Source/RefactorTactics/Tests/`
- `Content/RT/UI/Match/WBP_RT_EventLog.uasset`
- `Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset`
- `docs/technical/systems/spec-player-event-log.md`
- minimal cross-link update in `docs/technical/systems/progettazione-hud.md`

## Suggested implementation commit

`feat(ui): replace legacy canvas panels with player event log`
```

---

# 4. Cross-release epic through v1.0

## Required title

`[EPIC] Player Event Log & Explainability — dal TurnLog alla UI fino alla v1.0`

## Required body

```markdown
> **Cross-release capability** · **v0.1 owner**: #25 / #613 · **Detailed diagnostic log**: #79  
> **Marker**: `RT-PLAYER-EVENT-LOG-2026-08-31`
>
> ⚠️ No `E<n>` identifier: this capability crosses releases and follows the cross-release epic convention already used in the repository.

## Problem

RefactorTactics has an authoritative, structured TurnLog and a deliberately detailed diagnostic/combat log. That is the correct source for debugging, replay verification and reason-code inspection, but it is not the correct player-facing narrative.

The player needs a different projection:

- few events;
- tactical significance;
- grouped outcomes;
- no micro-step/cell spam;
- no private-information leaks;
- no second source of truth.

## Invariant

```text
Resolver authoritative
  -> TurnLog / canonical structured facts
       -> observer-authorized knowledge/privacy view
            -> Player Event Projector
                 -> FRTPlayerEvent[]
                      -> Match HUD / History / Replay player-facing
```

Never:
- derive player events by parsing diagnostic strings;
- let presentation recompute rules;
- project first and sanitize later;
- expose private planning to a client to make summaries;
- let player events influence resolution.

## Presentation vocabulary

Initial importance:

- `Minor`
- `Important`
- `Critical`

This is a **presentation taxonomy**, not a gameplay taxonomy.

The canonical TurnLog categories remain canonical.

## Aggregation

An event shown to the player represents a tactical fact, not every technical operation used to produce it.

Baseline dominance:

```text
KO > Damage > Hit
Blocked/Interrupted > Move
Reaction commit/fire > Reaction opened
Objective captured/lost > progress bookkeeping
```

Environmental propagation may combine multiple cell-level changes into one player-facing event when they express one tactical outcome.

## Existing owners

| Owner | Relationship |
|---|---|
| #25 / #613 | v0.1 in-match Screen HUD and removal of duplicate Canvas panels |
| #79 | detailed diagnostic log; remains precise and complete |
| #1466 / #1496 | knowledge/privacy; player projector consumes their authorized result |
| #265 / #268 | semantic icon language |
| #1881 / #472 | playback/replay transport and player replay consumer |

## Progressive gates

### v0.1

- [ ] legacy Canvas Screen HUD removed while Tactical World Overlay remains;
- [ ] typed Player Event contract and projector;
- [ ] initial Move / Combat / Status-Environment / KO / Objective grouping;
- [ ] compact bottom-right UMG feed;
- [ ] no debug overlay active by default;
- [ ] privacy test with unauthorized enemy facts;
- [ ] detailed developer log stays intact.

### v0.2+

- [ ] extend policies only when real systems ship: new reactions, structures, multilayer, forced movement, environment and objectives;
- [ ] optional expandable history without turning normal HUD into a permanent large log;
- [ ] optional summary → inspect detail, still privacy-safe;
- [ ] semantic icon catalog integration where a real category/consumer exists;
- [ ] keep audit/debug trace separate from player narrative.

### v1.0 close gate

- [ ] every shipped gameplay event family has an explicit player policy: `silent / important / critical / grouped`;
- [ ] same canonical result + same authorized observer view => same semantic player-event sequence independent of FPS, Tick, Actor order and animation duration;
- [ ] localization-ready event payload: semantic key + arguments, not hardcoded prose as the only authority;
- [ ] accessibility: critical meaning is not conveyed by color/icon alone;
- [ ] match history and player-facing replay consume the same Player Event contract, not a new simulator;
- [ ] dedicated-server / multiplayer privacy proves no enemy planning or unauthorized knowledge leaks;
- [ ] bounded storage and performance measured;
- [ ] packaged verification green;
- [ ] no legacy/debug overlay enabled by default.

## Non-goals

- rewrite the TurnLog;
- weaken developer diagnostics;
- make this a full replay implementation;
- add gameplay to justify UI;
- create new phases/priorities;
- perform privacy filtering after composing player-facing text.

## Documentation owner

`docs/technical/systems/spec-player-event-log.md`

It owns **Player Event semantics**.

`docs/technical/systems/progettazione-hud.md` continues to own **layout and HUD composition**.
```

---

# 5. Live repository documentation

Create or update:

`docs/technical/systems/spec-player-event-log.md`

Use this structure:

```markdown
# Spec — Player Event Log

Status: live design/technical owner
Marker: RT-PLAYER-EVENT-LOG-2026-08-31

## 1. Purpose
Separate developer/audit observability from player-facing event narration.

## 2. Non-negotiable ownership
- TurnLog = authoritative structured facts.
- #79 detailed log = diagnostic.
- Player Event Log = derived presentation.
- Knowledge/privacy owner remains external to this spec.
- HUD layout owner remains `progettazione-hud.md`.

## 3. Pipeline
TurnLog -> authorized observer view -> Player Event Projector -> FRTPlayerEvent[] -> UMG/history/replay.

## 4. Privacy
Authorization before projection. No private-planning or unknown-enemy data can reach the projector on a client.

## 5. Player Event contract
Typed semantic event + importance + stable actor/action references + numeric arguments required for presentation.
Prefer localization-ready semantic keys/arguments over text-only storage.

## 6. Importance
Minor / Important / Critical.

## 7. Grouping and dominance
KO > Damage > Hit; Blocked/Interrupted > Move; committed reaction > opened window; objective outcome > bookkeeping.
No cell-by-cell movement narration.

## 8. v0.1 rendering policy
Compact feed, bottom-right above plan controls, small recent-event budget, center viewport free.

## 9. Debug boundary
Legacy Canvas Screen HUD removed. ARTHUD keeps Tactical World Overlay. TurnLog/UE_LOG/rt.Debug remain.

## 10. Determinism
Player-event derivation must have deterministic semantic ordering for a given canonical result and authorized observer view.
It does not enter simulation hash/state unless a future ADR explicitly changes that.

## 11. Tests
Unit/projector tests + privacy discrimination + PIE + packaged no-debug-default.

## 12. Evolution to v1.0
Explicit policy for every shipped event family; localization; accessibility; history/replay consumption; multiplayer privacy; bounded storage/perf.
```

Then minimally update `docs/technical/systems/progettazione-hud.md`:
- add a link to `spec-player-event-log.md`;
- state that Player Event semantics live there;
- state that Screen HUD legacy Canvas panels are being removed by the v0.1 residual issue;
- preserve §4.2 Tactical World Overlay ownership.

Do **not** duplicate the full spec into the HUD document.

Update roadmap documents only where their current live structure has a clear home:
- `docs/roadmap/roadmap-v0.1.md`: add/link the v0.1 residual under E11 without minting a new checkpoint;
- `docs/roadmap/roadmap-post-v0.1.md`: link the cross-release epic only if the document conventions support cross-release capabilities. Do not invent an E-number to force it into the table.

If no clean location exists, create:
`docs/roadmap/plans/player-event-log-consolidation-2026-08-31.md`
and link it from the two GitHub issues instead.

---

# 6. Backlinks / reconciliation

Once issue numbers are known:

- v0.1 issue body links epic;
- epic body links v0.1 issue;
- add a non-destructive comment/back-reference to #25 and #613;
- add a comment to #79 only if needed to make the distinction explicit:
  **#79 remains diagnostic; the new player log does not weaken its DoD.**

Avoid replacing entire long historical issue bodies only to add one line.

---

# 7. Validation

Run repository-appropriate checks discovered live.

At minimum:

```bash
git diff --check
git diff -- docs/technical/systems docs/roadmap
```

If the repository exposes doc/radar checks, run the current ones; do not resurrect removed scripts referenced only by historical issue text.

This orchestration run should remain issue/docs-only unless explicitly expanded.

---

# 8. Final output required from Claude

Return:

1. v0.1 issue number + URL;
2. epic number + URL;
3. whether each was **created** or **updated**;
4. exact repo documentation files created/changed;
5. backlinks/comments added;
6. any repository truth that required changing this handoff;
7. tests/checks run;
8. final commit SHA / PR URL if repository docs were committed.

Do not claim success for a GitHub write that `gh` did not confirm.
