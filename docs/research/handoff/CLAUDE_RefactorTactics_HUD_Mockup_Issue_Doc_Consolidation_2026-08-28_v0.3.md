# CLAUDE CLOUD HANDOFF — HUD Mockup, Asset, Issue & Documentation Consolidation v0.3

**Project:** RefactorTactics  
**Date:** 2026-08-28  
**Repository expected:** `DegrassiAaron/refactor-tactics-main`  
**Engine expected:** UE 5.8.1 — verify on HEAD  
**Purpose:** consolidate the latest HUD discussion and three HUD mockups into the existing RefactorTactics Epic/Issue/documentation structure without creating a parallel roadmap or duplicate UI systems.

---

# 0. STATUS OF THIS HANDOFF

This file is a **HUD-specific delta**.

It must be applied together with the latest master reconciliation file:

```text
RefactorTactics_ClaudeCloud_Master_Issue_Reconciliation_2026-08-28.md
```

This file supersedes only the HUD-specific operational instructions from the earlier:

```text
CLAUDE_RefactorTactics_HUD_Roadmap_Consolidation_2026-08-28.md
```

when they conflict with newer repository policy.

Important correction from the master consolidation:

```text
DO NOT automatically restore Feature Registry.
DO NOT automatically restore parallel-batch.yaml.
DO NOT restore deleted/generated tracking systems just because an older handoff names them.
```

HEAD + Decision Log + current owner docs + live GitHub issues win.

---

# 1. MISSION

Consolidate the information from the latest HUD chat into:

```text
Decision / canon
→ existing Epic
→ existing checkpoint / issue
→ owner documentation
→ asset production work package
→ Unreal consumer
→ test / PIE / packaged gate
```

The output must be an updated tracking/documentation system, not another standalone roadmap.

Claude must:

1. sync and inspect current `origin/main`;
2. inspect current live GitHub issues;
3. audit existing HUD/Icon/Reaction/Perception owners;
4. update existing issues before creating anything;
5. add the new mockup-derived asset-production scope to the correct owners;
6. update owner documentation;
7. create at most a small number of execution work-package issues if there is a real scheduling gap;
8. preserve the v0.1→v1.0 UI lane without creating a second release ladder;
9. leave a concrete next executable HUD task.

Do not implement gameplay in this pass.

---

# 2. PREFLIGHT

Run:

```bash
git status
git branch --show-current
git fetch --all --prune
git rev-parse HEAD
git rev-parse origin/main
git log -20 --oneline --decorate origin/main
```

Inspect current repo docs using real paths:

```text
AGENTS.md
CLAUDE.md
README.md
docs/README.md
docs/CONTEXT_INDEX.md

docs/product/piano-canonico-mvp.md
docs/decisions/RT_PDR_00_Decision_Log.md
docs/OPEN_DECISIONS.md
docs/DOC_CONFLICT_MATRIX.md

docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-post-v0.1.md
docs/roadmap/roadmap-checkpoint.md
docs/roadmap/v0.1-definition-of-done.md

docs/technical/**HUD**
docs/technical/**pointer**
docs/technical/**icon**
docs/research/design/icon/visual-language/**
```

Search live GitHub before any create:

```bash
gh issue list --repo DegrassiAaron/refactor-tactics-main --state all --limit 2000
```

Search specifically:

```text
HUD
Screen HUD
Action Dock
Action Slot
Selected Unit
Team Roster
Ready
Confirm Plan
Warning
Friendly Fire
Collision
Combat Log
Ghost Timeline
Pointer
Icon Language
Icon Catalog
HUD asset
UI asset
portrait
resource bar
button
panel
slot
```

---

# 3. KNOWN OWNERS TO VERIFY — DO NOT DUPLICATE

These are historically current but must be re-read live.

## E11 — HUD / log / debug

```text
#25   E11 — HUD, log e debug
#77   CP 11.1 — HUD di partita
#78   CP 11.2 — intenti / certainty
#79   CP 11.3 — Combat Log / reason code
#172  CP 11.5 — Ghost Timeline
#173  CP 11.6 — scrubbing / Reaction branch
#613  CP 11.7 — Screen HUD in UMG
#705  CP 11.8 — Pointer Interaction Contract
```

## E20 — HUD Icon Language v0.1

```text
#217  E20 — HUD Icon Language
#218  CP 20.1 — URTIconCatalogData
#219  CP 20.2 — categorie / asset set v0.1
#220  CP 20.3 — widget consumano Icon Catalog
```

## E25 — Icon Language full/post-v0.1

Historically:

```text
#265  E25 — Icon Language completo
#266  CP 25.1 — taxonomy/governance
#267  CP 25.2 — catalog/validator/authoring
#268  CP 25.3 — HUD/world/reaction/perception integration
#269  CP 25.4 — accessibility/wiki/docs
```

## Adjacent consumers

Verify:

```text
#166 — live Fast Reaction / FIRE-HOLD / countdown
#160 — partial knowledge / last-known / acoustic HUD
#289 — presentation/readability owner or successor
#472 — replay UI owner or successor
#607 — preview / explainability owner or successor
```

Rule:

```text
UPDATE EXISTING
→ LINK EXISTING
→ CREATE ONLY FOR A REAL UNOWNED EXECUTION GAP
```

Do not create a new `HUD v0.1` Epic.

---

# 4. NEW CHAT DECISION — ROLE OF THE THREE MOCKUPS

The three mockups are not gameplay canon. They are visual/reference material.

Expected reference files from the latest session:

```text
1000013420.png
1000013346.png
1000013348.png
```

Verify actual repository/vault names before documenting paths.

## Mockup 1 — in-game planning composition

Use for composite references:

```text
selected hero header
ability detail panel
initiative/team strip
objectives panel
battlefield modifiers
Movement / Character Kit / Reaction group containers
Interact / Wait / Ready controls
certainty legend
small utility buttons
```

Do not use it as a monolithic texture.

## Mockup 2 — visual style guide / atomic asset source

Primary reference for atomic families:

```text
panels
buttons
action slots
portrait frames
status overlays
resource bars
ghost-timeline primitives
core icons
warning icons
tactical markers
```

This is the most important source for reusable visual primitives.

## Mockup 3 — layout / composite module source

Use for:

```text
turn/planning header
squad cards
large selected-character panel
Ghost Timeline module
Universal Actions panel
Hero Kit panel
objectives/score
Ally Intent
Confirm Plan / Undo / Plan Valid
warning pills
collapsed Combat Log
```

Again: reference composition, not runtime monolithic images.

---

# 5. NON-NEGOTIABLE ART PRODUCTION RULE

Do **not** crop the mockups into production runtime assets.

Use the crop/reference only to instruct regeneration/recreation.

Production principle:

```text
one reusable visual primitive
≠
one screenshot fragment
```

For example:

```text
Action Slot
=
Frame Base
+ State Overlay
+ Semantic Glyph
+ Cooldown Mask
+ Charge/Cost runtime text
+ Warning
```

Character:

```text
Portrait
+ Portrait Frame
+ Team/Selection Overlay
+ Status Overlay
+ KO Overlay
+ runtime HP/text
```

Panel:

```text
scalable frame / 9-slice
+ runtime content
```

Do not bake:

```text
hero names
HP values
timers
cooldowns
round number
keyboard keys
ability labels
objective text
dynamic status values
```

into generic textures.

---

# 6. HUD v0.1 ASSET WORK PACKAGES

Do not create an issue per image.

Consolidate the visual-production scope into four work packages.

## HUD-A — Visual Foundation — P0

Contents:

```text
Panels
Buttons
Universal Action Slots
Hero Ability Slots
Portrait Frames
Resource Bars
```

Required reference families:

### Panels

```text
panel_primary
panel_secondary
panel_compact
panel_tooltip
panel_warning
```

### Buttons

```text
button_primary_normal
button_primary_hover
button_primary_pressed
button_primary_disabled

button_secondary_normal
button_secondary_hover
button_secondary_pressed
button_secondary_disabled

button_icon_normal
button_icon_hover
button_icon_pressed
button_icon_disabled
```

### Universal action slot

```text
slot_universal_available
slot_universal_hover
slot_universal_selected
slot_universal_planned
slot_universal_cooldown
slot_universal_unavailable
```

### Hero ability slot

```text
slot_hero_available
slot_hero_hover
slot_hero_selected
slot_hero_planned
slot_hero_cooldown
slot_hero_unavailable
```

### Portrait frame

```text
portrait_own
portrait_ally
portrait_enemy
portrait_selected
portrait_ko
```

### Resource bars

```text
bar_health
bar_shield
bar_resource
bar_timer
```

Gate:

> `WBP_RT_TacticalHUD` or current equivalent can be assembled in graybox with reusable visual primitives and no baked gameplay values.

Natural owner:

```text
E11 / #613
```

If no execution issue exists, candidate child work package:

```text
[ART/UI] HUD-A · Visual Foundation v0.1 — Panels, Buttons, Slots, Portrait Frames, Bars
```

Create only after live audit proves #613 cannot practically own the checklist directly.

---

## HUD-B — Semantic Core — P0

Contents:

```text
Core Icons
Warning Icons
Tactical Markers
Status Overlays
```

### Core icon artistic references

```text
icon_move
icon_wait
icon_guard
icon_overwatch
icon_attack
icon_dash
icon_cover
icon_height
icon_water
icon_fire
icon_electric
icon_reaction
```

These names are ART REFERENCE names only.

Runtime semantic keys must come from current Icon Catalog / RequiredIconIds / owner data.

Do not create runtime IDs just because the mockup contains a symbol.

### Warning icons

```text
warning_critical
warning_friendly_fire
warning_collision
warning_insufficient_resource
warning_invalid_target
warning_uncertain
```

### Tactical markers

```text
marker_confirmed
marker_predicted
marker_uncertain
marker_waypoint
marker_destination
marker_last_contact
```

### Status overlays

```text
status_ready
status_editing
status_selected
status_targeted
status_reaction_armed
status_low_health
status_ko
```

Gate:

> gameplay-facing semantic visuals resolve through the governed catalog/current semantic pipeline; color is not the only information channel.

Natural owner:

```text
E20 / #219
#220 for consumers
E25 only for full post-v0.1 maturation
```

Candidate child work package only if needed:

```text
[ART/UI] HUD-B · Core Semantic Asset Pack v0.1
```

Do not create separate issues for Move, Wait, Guard, etc.

---

## HUD-C — Planning Screen Integration — P0/P1

Composite references:

```text
reference_turn_planning_timer
reference_selected_character_panel
reference_squad_member_editing
reference_squad_member_ready
reference_confirm_plan
reference_undo
reference_plan_valid
```

Functional composition:

```text
Round / Phase / Timer
Team Roster
Selected Unit
Action Dock
Confirm / Ready / Undo
Plan validity
```

Gate:

> first v0.1 planning screen is usable without Debug HUD.

Natural owners:

```text
#77
#613
#705 where pointer precedence matters
#220 for semantic icon consumers
```

Candidate child issue only if missing:

```text
[UI] HUD-C · Assemble v0.1 Planning Screen from reusable HUD primitives
```

---

## HUD-D — Tactical Feedback — P1

Composite/reference scope:

```text
timeline_base
timeline_selected_phase
timeline_reaction_branch
timeline_delayed_branch

reference_ability_detail
reference_objectives_panel
reference_battlefield_modifiers
reference_ally_intent_panel
reference_single_ally_intent

warning_pill_friendly_fire
warning_pill_target_may_move
warning_pill_insufficient_resource

reference_combat_log_collapsed
```

Gate:

> planning is readable and Resolution is explainable without moving gameplay rules into UMG.

Natural owners:

```text
#78   certainty/team intent
#79   Combat Log
#172  Ghost Timeline
#173  focus/scrubbing/reaction branch
#613  screen composition
```

Candidate child issue only if none of those can execute the integration:

```text
[UI] HUD-D · Tactical Feedback integration v0.1
```

---

# 7. ISSUE UPDATE PLAN

Claude must first read the current bodies.

Do not blindly replace them.

## #25 — E11 HUD, log e debug

Add/link a concise v0.1 execution section if missing:

```text
Visual Foundation
Planning Screen
Tactical Feedback
Screen HUD vs World Overlay boundary
```

Link E20 for semantic art.

Do not place Icon Catalog ownership inside E11.

## #77 — HUD di partita

Ensure acceptance covers:

```text
round / phase / timer
selected unit
HP/resources/status
Movement / Main / Reaction readability
current plan / committed state
Ready/Confirm control
no private enemy info
```

Add HUD-C as execution grouping if useful.

## #78 — certainty / allied intent

Ensure:

```text
Confirmed / Predicted / Uncertain
not color-only
team-authorized intent only
uncertainty visual can be represented in both screen HUD and world overlay
```

Do not merge `IntentState` and `OutcomeCertainty`.

## #79 — Combat Log

Ensure:

```text
TurnLog authoritative source
reason code visible
no client recomputation
collapsed v0.1 presentation allowed
```

## #172 / #173 — Ghost Timeline

Add/reference the new visual-production split if missing:

```text
timeline rail
node
selected node
reaction branch
delayed branch
```

These should be dynamic primitives, not one baked timeline image.

Reaction is not a fifth macro-phase.

## #613 — Screen HUD UMG

This is the main Screen HUD integration owner.

Add/check:

```text
HUD-A Foundation
HUD-C Planning Screen
HUD-D screen-space feedback
Top / Left / Bottom / Right composition
center battlefield kept free
reusable primitives
no baked runtime text
9-slice/scalable panel strategy where appropriate
```

World tactical overlays remain outside large static UMG panels.

## #705 — Pointer Contract

Only cross-link:

```text
HUD must intercept its own controls
world pointer remains owner of tactical map interactions
```

Do not turn asset work into pointer runtime scope.

## #217 / #219 / #220

Consolidate HUD-B.

Preserve:

```text
Semantic IconId
→ catalog
→ asset
→ widget/world consumer
```

Mockup/artistic labels do not create runtime taxonomy.

## #265–#269

Only update if current E25 still owns post-v0.1 visual-language maturation.

Do not move v0.1 foundation work out of E20 just to centralize everything.

## #166

Cross-link future Decision Window visual consumer:

```text
FIRE
HOLD
countdown
target/outcome
```

Do not implement it inside HUD-A/C.

## #160

Cross-link future Last Known / acoustic uncertainty visuals.

Do not make mockup `Last Contact` markers read hidden canonical state.

---

# 8. DOCUMENTATION UPDATE PLAN

Prefer updating current owner docs.

## `progettazione-hud` owner

Add, if missing:

```text
HUD v0.1 visual-production architecture
atomic asset vs composite widget distinction
HUD-A/B/C/D work packages
mockup roles
Screen HUD / world overlay boundary
first-playable readability gate
```

Do not turn it into an image catalog dump.

## icon owner / `brief-icone-v01`

Add only:

```text
HUD-B semantic asset production link
mockup semantic glyphs are references
runtime keys remain governed
no hardcoded texture
```

Do not merge Card Grammar, runtime catalog and screen layout into one taxonomy.

## HUD art/asset production owner

Search for an existing live owner first.

Possible historical materials:

```text
CLAUDE_HUD_CONSOLIDATED.md
RefactorTactics_HUD_Claude_Consolidated_v0.1.md
```

If an authoritative asset-production doc already exists in repo, update it.

If no live owner exists, create **one** small owner document following the repo's current docs taxonomy, with a title equivalent to:

```text
HUD Asset Production v0.1
```

It owns:

```text
asset families
naming
modularity
reference images
generation batches
approval checklist
Unreal import constraints
```

It does NOT own:

```text
gameplay
Action Economy
Icon semantic taxonomy
certainty calculation
warning logic
```

## Roadmap docs

`roadmap-v0.1.md` should show only the execution dependencies/gates needed for the v0.1 HUD.

`roadmap-post-v0.1.md` should preserve the UI lane through v1.0 via existing release owners.

Do not paste the entire asset checklist into the roadmap.

Link to owner docs/issues.

---

# 9. UI ROADMAP LANE v0.1 → v1.0

This is a lane mapped onto the canonical releases, not a second release system.

## v0.1 — Vertical Slice

```text
Planning HUD
Ready / Confirm
Path / AoE ghosts
collision / Friendly Fire warnings
Combat Log
Confirmed / Predicted / Uncertain
core HUD icon language
HUD-A/B/C/D minimum needed for first playable
```

## v0.2 — Structure / Windows

```text
Team Intent labels/pings
live Fast Reaction visual window
FIRE / HOLD / timeout feedback
reaction presentation assets
```

Only include mechanics actually in the canonical v0.2 gameplay owner.

## v0.3 — Information

```text
Vision overlay
Sound overlay
TeamKnowledge
Last Known / Ghost Intel
privacy-safe information warnings
```

## v0.4 — Operations

```text
multi-layer / height UI
doors / bridges / tunnels / elevators
environment overlays
strategic/tactical camera presentation
```

## v0.5+

Map UI tasks onto the current canonical E40–E45/successor release owners.

Do not create release Epics from this handoff.

Expected UI themes:

```text
v0.5 online UX / forced-movement & objective consequence presentation where owned
v0.6 ability-authoring/character setup consumers where owned
v0.7 reconnect/spectator/competitive online states
v0.8 3v3/4v4 density and clarity
v0.9 accessibility/localization/freeze/hardening
v1.0 release UI certification
```

HEAD decides the real release owner.

---

# 10. HUD v0.1 FIRST-PLAYABLE GATE

Without Debug HUD, an observer must understand:

1. selected unit;
2. round/phase;
3. remaining planning time;
4. available action economy;
5. current planned action;
6. planned path/destination;
7. target/AoE;
8. allied collision/Friendly Fire warning;
9. Confirmed/Predicted/Uncertain status;
10. how to Undo / Confirm / Ready;
11. what actually happened during Resolution;
12. why a blocked/failed action failed when reason data exists.

This gate belongs in the current v0.1 acceptance/PIE owner, not a standalone new testing framework.

---

# 11. ASSET APPROVAL GATE

Each generated HUD asset should satisfy:

```text
[ ] reusable primitive
[ ] no dynamic text baked
[ ] no gameplay number baked
[ ] clean alpha if overlay
[ ] consistent padding/canvas
[ ] readable at intended HUD scale
[ ] state represented with overlay/tint where possible
[ ] color not sole semantic channel
[ ] UE naming consistent
[ ] scalable/9-slice where required
[ ] no duplicated near-identical frame if a state overlay is sufficient
```

For icons:

```text
[ ] silhouette works in grayscale
[ ] distinguishable at small size
[ ] semantic runtime ID comes from owner data, not filename guess
```

---

# 12. UNREAL IMPORT / COMPOSITION GUIDANCE

If still compatible with current project conventions:

```text
Texture Group: UI
Compression: UserInterface2D (RGBA) where clean alpha matters
avoid unnecessary mip bleeding on tiny icons
verify neon/glow alpha halos
use 9-slice / scalable brushes for resizable panels
```

Prefer compositing:

```text
Frame
+ overlay
+ icon
+ runtime text
+ progress/fill
```

over unique flattened textures.

Do not change project import policy if HEAD has newer explicit rules.

---

# 13. TRACKING POLICY CORRECTION

Older HUD handoffs referenced:

```text
Feature Registry
parallel-batch.yaml
```

The latest master consolidation explicitly says these may have been removed.

Therefore:

```text
if present and current → use them
if removed → DO NOT recreate them
if old issue references them → add a dated correction/link to the new owner
```

Same rule for generated roadmap views.

Edit source, not generated artifact.

---

# 14. CANDIDATE NEW ISSUES — DEFAULT IS ZERO

Expected normal result:

```text
0 new Epics
0–4 new child work-package issues
```

Create a child issue only when:

1. live audit finds no executable owner;
2. adding a huge asset checklist to the existing CP would make it unmanageable;
3. the child can close independently;
4. it has one clear parent;
5. it has an explicit v0.1 gate.

Candidates:

```text
HUD-A · Visual Foundation
HUD-B · Core Semantic Asset Pack
HUD-C · Planning Screen Integration
HUD-D · Tactical Feedback Integration
```

Prefer using checklists/comments/sub-issues under current owners instead of creating all four automatically.

Never create:

```text
one issue per icon
one issue per button state
one Epic per asset family
one issue per PNG
```

---

# 15. RECONCILIATION TABLE REQUIRED BEFORE EDIT

Build:

| Cluster | Release | Existing Epic | Existing Issue | Owner Doc | State | Action | Notes |
|---|---|---|---|---|---|---|---|

Minimum rows:

```text
HUD shell
Panels
Buttons
Action Slot
Portrait Frame
Resource Bar
Selected Unit
Team Roster
Turn Header
Action Dock
Confirm/Ready
Warnings
Certainty
Ghost Timeline
Combat Log
Core Icons
Warning Icons
Tactical Markers
Status Overlays
Ally Intent
Ability Detail
Objectives
Reaction Window
Last Known / Acoustic
Accessibility / density
```

Actions:

```text
UPDATE_EXISTING
LINK_ONLY
CREATE_CHILD
ALREADY_DONE
DEFER
DECISION_REQUIRED
```

---

# 16. DEFINITION OF DONE — THIS CONSOLIDATION

The pass is done only when:

```text
[ ] live HEAD recorded
[ ] live E11/E20/E25 and adjacent issues read
[ ] no duplicate HUD Epic created
[ ] mockup roles documented
[ ] HUD-A/B/C/D mapped to owners
[ ] #613 owns screen-HUD integration
[ ] E20 owns v0.1 semantic icon production
[ ] E25 remains post-v0.1/full visual-language owner if still current
[ ] #78 owns certainty/team-intent semantics
[ ] #79 owns Combat Log presentation contract
[ ] #172/#173 own Ghost Timeline/focus
[ ] #705 remains pointer contract
[ ] #166 and #160 only receive appropriate cross-links
[ ] current HUD owner doc updated
[ ] current icon owner doc updated only where relevant
[ ] HUD asset-production owner exists exactly once
[ ] v0.1 roadmap links the HUD gate without dumping asset details
[ ] post-v0.1 roadmap carries UI lane without a parallel ladder
[ ] no Feature Registry/parallel tracking resurrected if removed
[ ] no texture hardcoding introduced
[ ] asset-generation checklist is actionable
[ ] first-playable HUD gate is linked to PIE/test owner
[ ] final GitHub/report links are real
```

---

# 17. FINAL REPORT REQUIRED

Return:

```text
BASE COMMIT:
CURRENT MAIN:

EPICS READ:
ISSUES READ:

ISSUES UPDATED:
- #...

ISSUES CREATED:
- #... or NONE

LINK-ONLY:
- #...

ALREADY DONE:
- ...

DEFERRED:
- ...

DOCS UPDATED:
- path

ROADMAP UPDATED:
- path

ASSET OWNER:
- path / issue

CONFLICTS:
- ...

HUD-A:
status / owner

HUD-B:
status / owner

HUD-C:
status / owner

HUD-D:
status / owner

NEXT EXECUTABLE HUD TASK:
one concrete task
```

For anything not verified live, write:

```text
NOT VERIFIED
```

---

# 18. DO NOT DO

```text
NO new HUD v0.1 Epic
NO second Icon Catalog
NO second certainty model
NO second warning logic
NO one-issue-per-image
NO monolithic screenshot-derived runtime texture
NO hero name/HP/timer baked into generic art
NO gameplay calculation in UMG
NO private enemy intent in opponent warning/preview
NO Reaction as a fifth phase
NO Universal Action + Hero Ability as two independent Main actions
NO Sprint/Sneak invented from mockup art
NO new semantic IDs from filename guesses
NO restoring removed Feature Registry automatically
NO restoring removed parallel-batch tracking automatically
NO Git LFS if current project policy uses Local Asset Vault
NO editing generated views instead of their source
```

---

# 19. SOURCE MATERIAL TO CROSS-CHECK

At minimum compare this delta with:

```text
RefactorTactics_ClaudeCloud_Master_Issue_Reconciliation_2026-08-28.md
CLAUDE_RefactorTactics_HUD_Roadmap_Consolidation_2026-08-28.md
RefactorTactics_UI_HUD_Planning_BatchD_Consolidated_2026-08-17.md
RefactorTactics_IconLanguage_Consolidated_Claude_Handoff_2026-08-20.md
CLAUDE_RT_IconGrammar_Consolidation_Handoff_2026-08-28.md
CLAUDE_HUD_CONSOLIDATED.md
RefactorTactics_HUD_Claude_Consolidated_v0.1.md
```

If those documents conflict, do not silently merge.

Use current authority order and report the conflict.

---

# 20. START COMMAND FOR CLAUDE CLOUD

Use this handoff as a HUD-specific delta to the current master reconciliation.

> Audit current `DegrassiAaron/refactor-tactics-main` and live GitHub first. Consolidate the latest HUD mockup decisions into existing E11/E20/E25 owners, especially #25, #77, #78, #79, #172, #173, #613, #705, #217, #219 and #220, verifying all IDs live before modifying. Treat the three mockups as reference: mockup 2 owns atomic visual language; mockups 1 and 3 own composite/layout references. Create no new HUD Epic. Map the work into HUD-A Visual Foundation, HUD-B Semantic Core, HUD-C Planning Screen and HUD-D Tactical Feedback, preferably as checklists under current owners and only as child issues when a real execution gap exists. Update the current HUD owner doc, current icon owner doc where relevant, the v0.1 roadmap gate and post-v0.1 UI lane. Preserve Screen HUD vs Tactical World Overlay, Confirmed/Predicted/Uncertain, semantic Icon Catalog resolution, privacy, and presentation-not-authority. Do not restore Feature Registry or parallel-batch tracking if HEAD removed them. Finish with real issue/doc links and one next executable HUD task.
