# RefactorTactics — Claude Cloud Debug HUD Graybox — ALL IN ONE

> 📸 `HISTORICAL` · **Kit d'autore consumato**, non una fonte · **Consumato**: 2026-08-30 · **Base**:
> `fff33020` (`origin/main`) — lo stesso SHA che il kit dichiara di aver osservato.
>
> Archiviato da [`docs/archive/`](../../README.md): vale per la **provenienza** e il rationale, mai per la
> regola. Stava alla radice del repository come file untracked
> `RefactorTactics_ClaudeCloud_DebugHUD_Graybox_ALL_IN_ONE.md` — **1033 righe, 29 874 byte**, consolidamento
> di quattro sorgenti — ed è riprodotto qui **verbatim**. Per confrontarlo con un eventuale re-drop:
> `sed '3,15d' <questo file> | md5sum`, che rimuove esattamente questo banner.
>
> **Cosa possiede**: la specifica del graybox Turn Header, i gate `G01`–`G18` e `R01`–`R06`, la checklist
> d'Editor. **Cosa non possiede**: nessuna autorità, e nessuna esecuzione. Il referto — voce per voce,
> misura per misura — è [`debug-hud-graybox-spec-panel-2026-08-30.md`](../../../roadmap/plans/debug-hud-graybox-spec-panel-2026-08-30.md).

File unico consolidato per implementare il primo slice graybox/debug dello Screen HUD in Unreal Engine.

Contiene:
- istruzioni di esecuzione per Claude Cloud;
- regole architetturali;
- preflight repository;
- uso Unreal Editor / MCP;
- specifica UMG;
- criteri di acceptance;
- test Automation / PIE / packaged;
- formato del report finale.

Usa questo file come unico handoff operativo.


---

# SOURCE FILE: README.md

# RefactorTactics — Claude Cloud pack
## Debug / Graybox HUD — Planning Header v0.1

Use these files in this order:

1. `CLAUDE_CLOUD_EXECUTE_DebugHUD_Graybox_v0.1.md`
   - Main execution handoff.
   - This is the file to give Claude Cloud first.

2. `EDITOR_MCP_CHECKLIST_DebugHUD_Graybox.md`
   - Editor/Unreal MCP steps for the `.uasset` work.
   - Use while the Unreal Editor is open.

3. `ACCEPTANCE_AND_TESTS_DebugHUD_Graybox.md`
   - Exact visual, automation, PIE and packaged-build gates.

Scope is deliberately narrow:
- one graybox slice of the Screen HUD;
- turn / phase / timer;
- Ready only if the current authoritative/sanitized API already supports it or can be exposed with a minimal correct adapter;
- no final art;
- no action dock, roster, selected-unit card, objectives, Ghost Timeline, warnings or icon production in this pass.

Repository expected:
`DegrassiAaron/refactor-tactics-main`

Live HEAD observed while preparing this pack:
`fff33020230ab0ae15a174a97b29dbd37a8035d2`
(2026-08-29; Claude must fetch and verify again before editing.)

Known live owner:
GitHub issue `#613 — CP 11.7 — Screen HUD in UMG (layer §4.1)` is OPEN.

Important:
The repository/HEAD, `AGENTS.md`, `CLAUDE.md`, Decision Log, current roadmap and live issue bodies win over this handoff if anything has changed.


---

# SOURCE FILE: CLAUDE_CLOUD_EXECUTE_DebugHUD_Graybox_v0.1.md

# CLAUDE CLOUD EXECUTION HANDOFF
## RefactorTactics — Debug / Graybox HUD, Planning Header v0.1

**Date:** 2026-08-30  
**Repository:** `DegrassiAaron/refactor-tactics-main`  
**Engine expected:** Unreal Engine **5.8.1** — verify on current HEAD before work  
**Observed HEAD when this handoff was generated:** `fff33020230ab0ae15a174a97b29dbd37a8035d2`  
**Primary live owner:** `#613 — CP 11.7 — Screen HUD in UMG (layer §4.1)` — OPEN when checked  
**Intent:** implement only the first graybox slice of the real Screen HUD, using primitive UMG elements that will later be restyled/replaced without changing gameplay authority.

---

# 0. EXECUTION RULE

Do the work. Do not only write a plan.

Use the Unreal MCP if available for Editor operations. You are allowed to launch the Unreal Editor if it is not already running.

Do **not invent Unreal MCP tool names or Unreal APIs**. First discover the MCP capabilities actually exposed in the environment. If the MCP can launch/open the project, use it. If it can only manipulate an already-running Editor, launch the project using the repository's documented method and then use the MCP.

If an Editor/MCP operation is impossible in the environment, complete every code/document/test change that can be completed, record the exact blocked `.uasset` steps, and leave the branch in a buildable state. Do not fake completion.

---

# 1. USER GOAL

Create a deliberately simple **debug/graybox version of one HUD slice**.

Target visible result at 1920×1080:

```text
┌───────────────────────────────────────────────────────────────┐
│ TURN 01             PLANNING             00:27     [ READY ] │
└───────────────────────────────────────────────────────────────┘
```

When Ready is legally available and the current gameplay API supports it:

```text
┌───────────────────────────────────────────────────────────────┐
│ TURN 01             PLANNING             00:18    [ READY ✓ ]│
└───────────────────────────────────────────────────────────────┘
```

This is **not final HUD art**.

Use only Tier-0 primitives:
- Border;
- HorizontalBox / Overlay / SizeBox / Spacer as needed;
- TextBlock;
- Button only if Ready can be wired correctly;
- flat temporary colors;
- default/project font already available;
- no new textures;
- no new icon assets;
- no animation;
- no material/VFX work.

The center of the screen must remain free.

---

# 2. IMPORTANT CORRECTION: DO NOT CREATE A PARALLEL DEBUG WIDGET

Earlier brainstorming used the provisional name:

`WBP_RT_DebugPlanningHeader`

Do **not** create that by default.

Current repository architecture already defines the canonical Screen HUD widget family:

```text
WBP_RT_TacticalHUD
WBP_RT_TurnHeader
WBP_RT_TeamRoster
WBP_RT_SelectedUnitPanel
WBP_RT_ActionDock
WBP_RT_ActionSlot
```

The C++ base class `URTTurnHeaderWidget` already exists in:

`Source/RefactorTactics/UI/RTScreenHudWidgets.h`

and exposes sanitized header data through the existing HUD ViewModel.

Therefore the preferred implementation is:

> build the real `WBP_RT_TurnHeader` now, but style it as a primitive graybox.

Later art passes replace styling/composition, not gameplay plumbing.

Only create a differently named temporary widget if current HEAD has explicitly superseded this naming contract. If so, document the decision and point to the current source of truth.

---

# 3. ARCHITECTURAL BOUNDARIES — NON-NEGOTIABLE

Preserve current RefactorTactics rules:

```text
Simulation / authoritative state
        ↓
sanitized ViewModel / presentation state
        ↓
UMG Screen HUD
```

The widget must NOT:
- own the turn;
- increment the round;
- determine the phase;
- compute time remaining from its own gameplay clock;
- recalculate path validity;
- inspect enemy private planning;
- query `CanonicalIntentStore`;
- call `Get All Actors Of Class` to rebuild gameplay state;
- derive competitive rules from actors;
- directly mutate the `TurnManager`;
- migrate Tactical World Overlay logic into UMG.

The Screen HUD is a consumer.

Current repository already separates:
- **§4.1 Screen HUD / UMG**: turn, phase, timer, roster, selected unit, action dock, warning, combat log, plan confirmation;
- **§4.2 Tactical World Overlay**: path, waypoint, AoE, friendly-fire, facing, world-anchored bars, etc.

Keep §4.2 in the current world-overlay/Canvas path.

Do not migrate `ARTHUD::DrawHUD`.

---

# 4. PREFLIGHT — REQUIRED BEFORE EDITING

Run and record:

```bash
git status
git branch --show-current
git fetch --all --prune
git rev-parse HEAD
git rev-parse origin/main
git log -10 --oneline --decorate origin/main
```

If the working tree is dirty:
- inspect the changes;
- do not overwrite unrelated user work;
- branch safely.

Read current repository instructions:

```text
AGENTS.md
CLAUDE.md
README.md
docs/README.md
docs/CONTEXT_INDEX.md
docs/decisions/RT_PDR_00_Decision_Log.md
docs/OPEN_DECISIONS.md
docs/DOC_CONFLICT_MATRIX.md
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-checkpoint.md
docs/roadmap/v0.1-definition-of-done.md
docs/technical/systems/progettazione-hud.md
docs/technical/runbooks/guida-screen-hud-umg.md
docs/roadmap/plans/screen-hud-umg-2026-08-26.md
```

Read live issue:
- `#613`
- also inspect `#77` if Ready / planning content ownership is needed;
- inspect `#705` only if pointer/input precedence becomes relevant.

Do not trust old issue prose when it conflicts with current code. The historical #613 body contains premises that a later implementation plan already marked outdated.

---

# 5. AUDIT CURRENT IMPLEMENTATION BEFORE CREATING ASSETS

Verify current HEAD for:

```text
Source/RefactorTactics/UI/RTScreenHudWidgets.h
Source/RefactorTactics/UI/RTScreenHudWidgets.cpp
Source/RefactorTactics/UI/RTHudViewModel.h
Source/RefactorTactics/UI/RTHudViewModel.cpp
Source/RefactorTactics/UI/RTHUD.h
Source/RefactorTactics/UI/RTHUD.cpp
Source/RefactorTactics/Frontend/RTFrontendNavigator.h
Source/RefactorTactics/Frontend/RTFrontendNavigator.cpp
Config/DefaultGame.ini
Content/RT/UI/
Content/RT/UI/Match/
```

Search for:
- `URTTurnHeaderWidget`;
- `URTTacticalHUDWidget`;
- `GetHeader`;
- `GetRoundCounterText`;
- any current Ready/Commit API;
- any current `WBP_RT_TacticalHUD`;
- any current `WBP_RT_TurnHeader`;
- match-HUD presentation in `URTFrontendNavigator`;
- the console variable `rt.HUD.CanvasPanels`.

Known from the inspected repository state:
- `URTTurnHeaderWidget` exists;
- it exposes `GetHeader()` and `GetRoundCounterText()`;
- sanitized HUD views exist;
- UMG/Slate dependencies already exist;
- the frontend architecture intends a dedicated Match HUD layer;
- `ARTHUD` has `rt.HUD.CanvasPanels` to disable legacy **screen-space** Canvas panels without disabling the world overlay.

But verify every point again because HEAD may have moved.

---

# 6. SCOPE FOR THIS PASS

## P0 — must deliver

Implement a visible graybox Turn Header containing:

1. round/turn counter;
2. phase;
3. planning timer;
4. primitive background/frame;
5. sane top-center anchoring at 1920×1080;
6. center map unobstructed;
7. real ViewModel data;
8. no final textures/icons;
9. visible in `L_DevSandbox` through the current Match HUD lifecycle;
10. no duplicate legacy screen-space header during validation.

## P0.5 — Ready

Add Ready to this slice **only if it can be implemented without violating the current ownership model**.

First audit current HEAD:
- Is Ready already represented in `FRTMatchHeaderView` or another sanitized HUD view?
- Is there an existing Blueprint-safe command/request path from HUD → PlayerController/planning API?
- Is Ready already exposed in another canonical Screen HUD widget?
- Is Ready currently local-only, server-authoritative, or commit-based?

If the correct API already exists:
- use it.

If Ready data exists but the widget lacks a thin exposure:
- add the smallest correct C++ adapter/view field with tests.

If no correct Ready command path exists:
- do **not** make the Button call `ARTTurnManager` directly;
- do **not** invent local `bReady` state that lies about gameplay;
- show a disabled placeholder `READY — API PENDING` only if useful for layout, or omit Ready;
- record the exact blocker and owner issue.

The goal is a truthful graybox, not a fake interactive demo.

---

# 7. VISUAL SPEC — GRAYBOX ONLY

Preferred top-center footprint:

```text
approx width: 680–760 px
height:       56–72 px
top offset:   16–24 px
```

Suggested content:

```text
[ Round 01/12 ]   [ PLANNING ]   [ 00:27 ]   [ READY ]
```

If the project's canonical text is `TURN` rather than `ROUND`, use the current repository terminology. Do not rename domain terms from this handoff.

Use flat temporary hierarchy:
- one dark translucent Border;
- readable light text;
- simple spacing;
- Button uses normal UMG style or flat background;
- no art asset import.

Do not spend time polishing hover/pressed states beyond making them legible.

No final graphite frames, neon, 9-slice, icon grammar or portrait framing in this task.

---

# 8. RECOMMENDED UMG STRUCTURE

For `WBP_RT_TurnHeader`, derived from `URTTurnHeaderWidget`:

```text
Border_Root
└─ HorizontalBox_Main
   ├─ SizeBox_Round
   │  └─ Text_Round
   ├─ Spacer
   ├─ SizeBox_Phase
   │  └─ Text_Phase
   ├─ Spacer
   ├─ SizeBox_Timer
   │  └─ Text_Timer
   └─ [optional]
      Button_Ready
      └─ Text_Ready
```

If the current runbook already defines a different hierarchy, follow the runbook unless it conflicts with the user-visible target.

The C++ class intentionally does not require `BindWidget` names. Prefer existing BlueprintPure getters/property binding or the current supported update pattern.

Avoid expensive per-frame Blueprint gameplay queries.

---

# 9. ROOT HUD INTEGRATION

Do not create a second widget-instantiation owner.

The current implementation plan states that `URTFrontendNavigator` is the sole authorized owner of `CreateWidget` for frontend/match screen presentation.

Audit whether the Match HUD layer from `screen-hud-umg-2026-08-26.md` is already implemented on current HEAD.

Case A — already implemented:
- reuse it;
- add/place `WBP_RT_TurnHeader` in the current `WBP_RT_TacticalHUD`.

Case B — root HUD exists but header missing:
- create only `WBP_RT_TurnHeader`;
- wire it into the root.

Case C — neither canonical root nor Match HUD presentation exists:
- execute only the minimum prerequisite from the current #613 plan necessary to present the root/header;
- do not build the other five HUD modules in this pass;
- keep the root extremely thin.

Do not create a standalone `AddToViewport` call elsewhere just to make the header appear.

---

# 10. LEGACY CANVAS COEXISTENCE

During PIE validation use the existing console variable:

```text
rt.HUD.CanvasPanels 0
```

Expected effect:
- old Canvas **screen-space panels** off;
- Tactical World Overlay remains;
- new UMG header visible without duplicate round/phase/timer.

Re-enable:

```text
rt.HUD.CanvasPanels 1
```

Verify this behavior on current HEAD before relying on it.

Do not use this cvar as a reason to delete old Canvas code in this pass.

---

# 11. C++ CHANGE POLICY

Prefer **zero new gameplay classes**.

Expected ideal change set:
- `.uasset` work only if required C++ and Match HUD layer are already in HEAD;
- possibly a tiny C++ adapter/test only for missing Ready or display formatting;
- docs/test evidence update.

If C++ is necessary:
- use current code conventions;
- do not add a duplicate ViewModel;
- do not expose `ARTTurnManager*` to Blueprint;
- do not expose enemy intent;
- do not add direct texture references;
- write automation coverage first when practical;
- keep Build.cs unchanged unless HEAD proves a real dependency is missing.

The 2026-08-26 plan already reported `UMG`, `Slate`, `SlateCore` as present. Verify before editing Build.cs.

---

# 12. TIMER DISPLAY

Use the existing sanitized timer value.

Requirements:
- timer shows `MM:SS` or the current repository-approved format;
- no context / invalid timer displays a neutral placeholder, e.g. `--:--`;
- do not create a separate authoritative countdown in the widget;
- do not derive simulation transitions from the display timer.

If no existing formatting helper exists, add a presentation-only formatter in the correct UI layer, with unit/automation tests. Do not put simulation timing into UMG.

---

# 13. PHASE DISPLAY

Display current canonical phase.

Do not hard-code a fake `PLANNING` string if the ViewModel already supplies the phase.

For the first test scenario Planning should visibly read as Planning/Decision according to current canon.

When Resolution/other phases occur, the widget should update from the current state rather than keep showing Planning.

No phase logic belongs in Blueprint.

---

# 14. OPTIONAL DEBUG LINE

Only if it helps validation and can be gated by the existing debug policy:

```text
Round=<n> | Phase=<phase> | RemainingMs=<value> | Ready=<0/1>
```

Rules:
- development/debug only;
- default player view remains with debug off;
- do not expose private opponent data;
- do not add a permanent second HUD architecture.

If `bShowDebug` already exists on the base widget, reuse it instead of inventing another flag.

---

# 15. UNREAL MCP / EDITOR EXECUTION

If Unreal MCP is available:

1. discover actual MCP commands/capabilities;
2. open the `.uproject` in the expected UE version if needed;
3. wait only as required by the tool call itself — do not claim background work;
4. verify `L_DevSandbox`;
5. inspect current `Content/RT/UI/Match/`;
6. create or update `WBP_RT_TurnHeader`;
7. create/update `WBP_RT_TacticalHUD` only if required to host it;
8. compile each Blueprint;
9. save assets;
10. run PIE;
11. execute `rt.HUD.CanvasPanels 0`;
12. verify live round/phase/timer;
13. exercise Ready only if correctly wired;
14. stop PIE;
15. save all changed assets.

Do not use Python/Editor scripting to manufacture a Widget Blueprint unless this repository already has a supported workflow for it and the result is a normal editable `.uasset`.

---

# 16. TESTS

Run the repository's current documented build/test commands from `AGENTS.md` / `CLAUDE.md`.

At minimum preserve:
- existing `RefactorTactics.HUD.*`;
- existing Screen HUD widget API tests;
- frontend Match HUD lifecycle tests if present.

If C++ changes:
- add/update targeted Automation Test for the modified presentation API;
- then run the relevant broader suite.

Manual PIE test:
- `L_DevSandbox`;
- 1920×1080;
- start Planning;
- header visible;
- round value correct;
- phase correct;
- timer decreases based on authoritative/presentation state;
- legacy screen panels can be hidden;
- world overlay remains;
- next phase updates header;
- next round updates header;
- no red log errors.

Ready, if implemented:
- initial state truthful;
- click uses the correct request path;
- displayed state follows authoritative/sanitized state;
- unready works only if the current rules allow it;
- no purely local cosmetic toggle that can diverge from gameplay.

---

# 17. PACKAGED DEVELOPMENT GATE

This feature is not Done on PIE alone.

If current project workflow allows packaging in the available environment:
- build/package Development;
- load the matching test map/scenario;
- verify the header appears and updates;
- verify debug is off by default;
- verify no missing class/asset references.

If packaging is not possible in the environment, record it explicitly as an unexecuted gate. Do not mark it passed.

---

# 18. DOCUMENTATION / TRACKING

Do not create a parallel Epic.

Update existing owner documentation only if implementation materially changes the recorded state.

Likely owner:
- #613.

If Ready is blocked by a separate owner:
- cross-link the existing issue;
- create a new issue only if live audit proves no owner exists.

Do not restore deleted Feature Registry systems.

Record actual evidence:
- branch;
- commit SHA;
- files changed;
- Automation tests;
- PIE result;
- packaged result or explicit not-run;
- screenshot only if the environment supports producing one.

---

# 19. ACCEPTANCE CRITERIA

P0 acceptance:

- [ ] Canonical `WBP_RT_TurnHeader` exists or current-HEAD equivalent is used.
- [ ] Widget uses primitive graybox visuals only.
- [ ] Real round/turn data shown from sanitized presentation state.
- [ ] Real phase shown from sanitized presentation state.
- [ ] Real planning timer shown from sanitized presentation state.
- [ ] No gameplay authority moved into UMG.
- [ ] No direct texture dependency introduced.
- [ ] New Screen HUD does not cover the tactical center.
- [ ] Old Canvas screen-space panels can be disabled for comparison.
- [ ] World overlay remains functional.
- [ ] Blueprint compiles and saves.
- [ ] Relevant automation tests pass.
- [ ] PIE verification in `L_DevSandbox` passes.
- [ ] No red runtime errors.
- [ ] Packaged Development checked, or truthfully recorded as not runnable.

Ready acceptance only if implemented:

- [ ] Ready is backed by the correct current gameplay/request API.
- [ ] Ready visual state comes from current sanitized/authoritative state.
- [ ] No local fake `bReady` authority inside the widget.
- [ ] No direct TurnManager mutation from Blueprint.
- [ ] Ready/unready behavior matches current rules.

---

# 20. EXPLICIT OUT OF SCOPE

Do not implement now:
- Team Roster;
- Selected Unit panel;
- Action Dock;
- Action Slots;
- ability icons;
- portrait art;
- health/resources bars;
- Objective panel;
- Ghost Timeline;
- warnings;
- Combat Log;
- tactical world overlays;
- final icon catalog art;
- final graphite visual language;
- animations;
- CommonUI migration;
- reaction UI;
- Time Bank UI;
- networked ally intent;
- enemy information;
- modding.

Do not "helpfully" widen the pass.

---

# 21. FILES EXPECTED TO CHANGE

Exact list depends on HEAD audit.

Likely `.uasset`:
```text
Content/RT/UI/Match/WBP_RT_TurnHeader.uasset
Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset   # only if needed
```

Possible C++ only if current HEAD lacks required presentation plumbing:
```text
Source/RefactorTactics/UI/RTScreenHudWidgets.h
Source/RefactorTactics/UI/RTScreenHudWidgets.cpp
Source/RefactorTactics/UI/RTHudViewModel.h
Source/RefactorTactics/UI/RTHudViewModel.cpp
Source/RefactorTactics/Frontend/RTFrontendNavigator.h
Source/RefactorTactics/Frontend/RTFrontendNavigator.cpp
Source/RefactorTactics/Tests/...                 # targeted tests
```

Possible config only if current Match HUD binding is still missing:
```text
Config/DefaultGame.ini
```

Do not touch all of these automatically.

---

# 22. GIT

Suggested branch:

```text
feat/debug-graybox-turn-header
```

Suggested commit if the pass is mostly UMG:

```text
feat(ui): add graybox planning turn header
```

If Ready cannot be correctly wired, do not hide that fact in the commit message or completion report.

Before final response:
```bash
git status
git diff --stat
git diff --check
```

Run repository-required gates.

Do not push/merge unless the current user instruction / repository workflow authorizes it.

---

# 23. FINAL REPORT FORMAT

Return:

```text
STATUS
- completed / partial / blocked

HEAD / BRANCH
- base SHA
- branch

IMPLEMENTED
- exact visible behavior

FILES CHANGED
- list

UNREAL EDITOR / MCP
- tools actually used
- assets created/updated

TESTS
- automation commands + result
- PIE result
- packaged result

READY
- implemented correctly / omitted / blocked
- reason

ARCHITECTURE CHECK
- no gameplay authority in UMG
- no enemy intent exposure
- no direct textures
- world overlay untouched

OPEN ITEMS
- only real remaining blockers

COMMIT
- SHA if committed
```

No generic roadmap. No speculative extra HUD modules.

---

# 24. WHY THIS PASS IS SAFE

The project documentation defines the HUD as a layered, data-driven consumer of sanitized state and explicitly keeps tactical world overlays separate from Screen HUD. The UI MVP guidance also calls for primitive/generated placeholders before production art. This pass follows that model by implementing the canonical header as a graybox instead of creating a disposable parallel UI system.

The repository implementation plan for #613 already establishes:
- UE 5.8.1;
- canonical `WBP_RT_*` naming;
- C++ widget bases;
- sanitized views;
- Match HUD lifecycle through the frontend;
- no direct texture references;
- no gameplay recalculation inside widgets.

Use current HEAD to confirm these facts before editing.


---

# SOURCE FILE: EDITOR_MCP_CHECKLIST_DebugHUD_Graybox.md

# Unreal Editor / MCP Checklist
## RefactorTactics — Graybox Turn Header

Use this only after the main handoff preflight.

## A. Discover environment

- [ ] Confirm Unreal Engine version expected by the `.uproject`.
- [ ] Discover available Unreal MCP operations.
- [ ] Do not guess tool names.
- [ ] If Editor is closed and MCP can launch it, launch the project.
- [ ] Otherwise launch using the repository's documented local method, then connect MCP.

## B. Inspect existing assets first

Check:

```text
Content/RT/UI/
Content/RT/UI/Match/
```

- [ ] Is `WBP_RT_TacticalHUD` already present?
- [ ] Is `WBP_RT_TurnHeader` already present?
- [ ] What are their parent classes?
- [ ] Are there existing bindings/layout elements?
- [ ] Are there unsaved user modifications?

Do not overwrite a richer current asset blindly.

## C. Header asset

Preferred:
```text
WBP_RT_TurnHeader
Parent: URTTurnHeaderWidget
```

Build only primitive hierarchy:

```text
Border_Root
└─ HorizontalBox_Main
   ├─ Text_Round
   ├─ Spacer
   ├─ Text_Phase
   ├─ Spacer
   ├─ Text_Timer
   └─ optional Button_Ready
      └─ Text_Ready
```

Suggested geometry:
- width: 680–760 px
- height: 56–72 px
- top-center
- Y offset: 16–24 px

Rules:
- flat temporary colors only;
- no imported image;
- no Texture2D variables;
- no animation;
- no final style work.

## D. Bind real presentation data

Use current parent API / runbook.

Known canonical C++ API from the inspected repository includes:
- `URTTurnHeaderWidget::GetHeader()`
- `URTTurnHeaderWidget::GetRoundCounterText()`

Bind:
- Round → current helper/view;
- Phase → current sanitized phase;
- Timer → current sanitized remaining time/formatter.

Do not add gameplay queries to Blueprint.

## E. Ready

Audit before wiring.

If correct request + state APIs exist:
- [ ] bind visible state from sanitized state;
- [ ] OnClicked calls the approved UI/controller request function;
- [ ] do not write TurnManager state directly.

If no correct API:
- [ ] omit interactive Ready OR show a clearly disabled layout placeholder;
- [ ] record blocker.

## F. Root HUD

If `WBP_RT_TacticalHUD` exists:
- [ ] place TurnHeader at top-center;
- [ ] preserve center empty;
- [ ] preserve existing child widgets.

If root is missing:
- [ ] create only minimum root required by current #613 plan;
- [ ] do not create TeamRoster/SelectedUnit/ActionDock just to fill space.

## G. Compile/save

- [ ] Compile `WBP_RT_TurnHeader`.
- [ ] Compile `WBP_RT_TacticalHUD` if changed.
- [ ] Resolve all Blueprint errors.
- [ ] Save assets.

## H. PIE

Open:
```text
L_DevSandbox
```

At 1920×1080:

- [ ] Start PIE.
- [ ] Header visible.
- [ ] Round correct.
- [ ] Phase correct.
- [ ] Timer updates.
- [ ] Execute `rt.HUD.CanvasPanels 0`.
- [ ] Duplicate Canvas header disappears.
- [ ] Tactical world overlay remains.
- [ ] New UMG header remains.
- [ ] Advance phase/round using current scenario controls.
- [ ] Header follows.
- [ ] Ready works only if genuinely wired.
- [ ] No red log errors.

Restore if useful:
```text
rt.HUD.CanvasPanels 1
```

## I. End

- [ ] Stop PIE.
- [ ] Save all.
- [ ] Check asset paths.
- [ ] Run source-control status.
- [ ] Do not mark packaged gate passed unless actually run.


---

# SOURCE FILE: ACCEPTANCE_AND_TESTS_DebugHUD_Graybox.md

# Acceptance & Test Matrix
## RefactorTactics — Debug / Graybox HUD v0.1

| ID | Gate | Expected |
|---|---|---|
| G01 | Canonical widget | Uses `WBP_RT_TurnHeader` or current-HEAD canonical replacement; no parallel debug HUD by default |
| G02 | Parent | Correct C++ Screen HUD base |
| G03 | Graybox | Only primitive UMG elements; no new final art |
| G04 | Turn | Displays real current round/turn |
| G05 | Phase | Displays real current phase |
| G06 | Timer | Displays real remaining planning time |
| G07 | Authority | Widget does not own or advance simulation |
| G08 | Privacy | No enemy planning/private intent available to widget |
| G09 | Texture | No new direct Texture2D dependency |
| G10 | Layout | Top-center, tactical center remains free |
| G11 | Canvas coexistence | `rt.HUD.CanvasPanels 0` removes legacy screen panels without removing world overlay |
| G12 | Blueprint | All changed WBP compile and save |
| G13 | Automation | Existing HUD/ScreenHud/Frontend relevant tests green |
| G14 | PIE | `L_DevSandbox` visual smoke test passes |
| G15 | Phase transition | Header updates when phase changes |
| G16 | Turn transition | Header updates when next round begins |
| G17 | Log | No red runtime/Blueprint errors |
| G18 | Package | Development packaged verification run OR explicitly recorded not run |

## Ready-specific gates

Only apply if Ready is implemented in this pass.

| ID | Gate | Expected |
|---|---|---|
| R01 | Source of truth | Displayed Ready derives from authoritative/sanitized state |
| R02 | Command path | Click uses approved UI/controller/planning request API |
| R03 | No direct mutation | Blueprint never sets TurnManager Ready directly |
| R04 | No fake local state | UI does not toggle a private cosmetic bool as truth |
| R05 | Rules | Unready only appears/works when current rules allow it |
| R06 | Test | Targeted test or repeatable PIE evidence covers Ready |

## Regression focus

Preserve:
- `RefactorTactics.HUD.*`;
- current Screen HUD widget API tests;
- current frontend Match HUD lifecycle tests;
- Tactical World Overlay behavior;
- selection/path/AoE world rendering already present.

## Manual smoke scenario

1. Open `L_DevSandbox`.
2. Start match.
3. Confirm header appears once.
4. Confirm round/phase/timer match the actual game state.
5. Disable old Canvas panels with `rt.HUD.CanvasPanels 0`.
6. Confirm world overlays remain.
7. Let timer progress.
8. Trigger the normal phase transition.
9. Confirm header updates.
10. Start next round.
11. Confirm counter increments.
12. If Ready is wired, Ready → state updates from gameplay; Unready only if legal.
13. Inspect Output Log for errors.
14. Repeat in Development packaged build if environment supports it.

## Failure conditions

Do not call the task Done if any is true:
- hard-coded fake timer is used;
- `PLANNING` is permanently hard-coded while game changes phase;
- Ready is only a local UI bool;
- UMG calls TurnManager directly to force Ready/turn advance;
- a second HUD instantiation owner is introduced;
- world-overlay code is migrated to the Screen HUD;
- a new Texture2D/icon dependency is introduced for this graybox;
- Blueprint compile errors remain;
- duplicate old/new headers are accepted as final behavior;
- packaged verification is claimed without being run.

## Completion report evidence

Record:
- base SHA;
- branch;
- changed files/assets;
- Unreal version;
- MCP/Editor operations actually used;
- automation command(s) and pass/fail;
- PIE result;
- packaged result or NOT RUN;
- Ready status;
- commit SHA if committed.
