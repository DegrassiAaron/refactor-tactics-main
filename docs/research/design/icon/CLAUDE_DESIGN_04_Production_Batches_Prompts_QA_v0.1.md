# CLAUDE DESIGN 04 — Production Batches, Prompts & QA v0.1

## Strategia

Non generare 120 icone in una sola passata.

Produrre per batch, fare review, poi congelare silhouette e naming.

---

# BATCH 0 — Visual Calibration

## Deliverable

Una board con:

- 3 panel frame;
- 3 action slot;
- 8 core glyph;
- 3 warning;
- 3 certainty states;
- 1 portrait frame;
- 1 skill card;
- 1 Fast Reaction mini panel.

## Prompt Claude Design

```text
Design a visual calibration board for RefactorTactics HUD v0.1.

Use the repository file "Guida visiva HUD tattica fantascientifica.png" as the visual source of truth.
Use the included RefactorTactics HUD Skill Bar concept only as composition inspiration.

Style:
- premium tactical sci-fi/fantasy;
- original RefactorTactics identity;
- restrained Paragon-era influence, not a copy;
- dark translucent panels;
- thin metallic frames;
- subtle angular/hex cuts;
- controlled cyan/purple/gold accents;
- no cockpit UI;
- no MMO chrome;
- no excessive glow.

Create on one design board:
- Panel Primary, Compact, Tooltip;
- Movement Slot, Ability Slot, Reaction Slot;
- glyphs: Move, Sprint, Dash, Basic Attack, Brace, Overwatch, Water, Electric;
- warnings: Friendly Fire, Collision, Invalid Target;
- Confirmed, Predicted, Uncertain pattern samples;
- Own Character portrait frame;
- compact skill card;
- Fast Reaction panel with FIRE/HOLD using dynamic-text placeholders.

All semantic glyphs must first work in monochrome.
Show actual-size 24 px and 32 px samples.
```

Exit gate:

- style approved before full production.

---

# BATCH 1 — Semantic Primitive Glyphs

## Deliverable

First lock these masters:

- Ally
- Enemy
- Self
- Cell
- Line
- Circle
- Cone
- Chain
- Damage
- Heal
- Push
- Shield
- Cover
- Move
- Sprint
- Dash
- Water
- Electric
- Fire
- Ice
- Smoke
- Reaction
- Brace
- Overwatch
- Interact
- Wait
- Uncertain modifier

## Prompt

```text
Create the RefactorTactics v0.1 semantic glyph master set.

Rules:
- 24x24 master grid;
- preferred 20x20 visual bounds;
- ~2 px stroke;
- silhouette-first;
- monochrome first;
- transparent background;
- no baked glow;
- no text;
- max 3 dominant visual components;
- optical 16 px variant where needed.

Critical distinctions:
- Ally != Enemy without color;
- Line != Move;
- Move != Sprint != Dash;
- Electric != Reaction;
- Shield != Brace != Cover;
- Attack != Overwatch;
- Wait != Hold;
- Water != Fire.

After silhouette approval create Default Accessible, CVD and High Contrast previews without changing geometry.
```

---

# BATCH 2 — Skill Bar / Universal Actions

## Deliverable

Icons and slot art for:

- Move;
- Sprint;
- Dash;
- Basic Attack;
- Brace;
- Overwatch;
- Interact;
- Wait;
- Ready;
- Undo.

Also slot frame states:

- Available;
- Hover;
- Selected;
- Planned;
- Cooldown;
- Unavailable;
- Invalid;
- Warning;
- Reaction Armed.

## Prompt

```text
Design the RefactorTactics v0.1 character skill bar static asset set.

The bar groups are:
MOVEMENT: Move, Sprint, Dash
CHARACTER KIT: Basic Attack + four hero skills
REACTION: Brace, Overwatch
CONTEXT: Interact, Wait, Ready

Design original, reusable UMG-ready action slot frames.
Do not bake labels, keybinds, cooldown numbers or costs into the textures.

Movement slots should feel related but Move/Sprint/Dash must be distinct in silhouette.
Basic Attack should visually belong with the Character Kit.
Brace and Overwatch share the Reaction family but must be distinct actions.
Ready is not a skill and must use a separate button family.

Create all standard states and show a 1920x1080 actual-size HUD strip test.
```

---

# BATCH 3 — Four v0.1 Character Kits

## Gadget

- Arc Pulse
- Linear Discharge
- Conductive Node
- Overload
- Reactive Capacitor

## Phase

- Pressure Jet
- Circular Tide
- Fluid Trail
- Mist Veil
- Flow Reaction

## Riktor

- Impact Shot
- Kinetic Panel
- Reconfigure
- Ram
- Interposition

## Wraith

- Pulse Shot
- Intercept Shot
- Passing Blade
- Deflection
- Feint

## Prompt

```text
Create ability-slot icon art for the four RefactorTactics v0.1 characters.

Do not create miniature paintings. Each icon must read at 32-36 px inside a 56-60 px slot.
Use the shared semantic grammar and give each ability a recognisable combination of geometry + effect + character identity.

FLUX:
Arc Pulse = basic attack + electric pulse
Linear Discharge = line + electric
Conductive Node = cell/node + conductivity setup
Overload = circle + electric + damage
Reactive Capacitor = reaction + shield + electric return

RIVA:
Pressure Jet = line + water + push
Circular Tide = circle + water + heal/support
Fluid Trail = dash + water trail
Mist Veil = area + water-to-smoke/obscure
Flow Reaction = reaction + water + reposition

BASTION:
Impact Shot = basic attack + kinetic impact
Kinetic Panel = create cover + directional panel
Reconfigure = cover + rotate/shift
Ram = charge/dash + push + damage
Interposition = reaction + ally + intercept/redirect

VEKTOR:
Pulse Shot = basic attack + kinetic pulse
Intercept Shot = reaction/overwatch + controlled line/cell + stop movement
Passing Blade = dash + line + damage
Deflection = reaction + deflection, not a shield pool
Feint = predicted cell mark + reposition

Create monochrome silhouettes first, then semantic accent versions.
Use no more than 2-3 dominant motifs per icon.
```

---

# BATCH 4 — Environment + Status

## Deliverable

Environment/surface:

- Floor
- Rough
- Water/ShallowWater
- Conductive
- Fire
- Smoke
- Ice
- HighGround/Height
- Cover
- Hazard
- Steam if active

Status:

- Wet
- Electrified/Shocked
- Burning
- Shielded
- Guarded
- Stunned
- Interrupted
- Slowed
- Suppressed
- Marked
- Rooted
- Anchored
- Obscured
- Low Health
- KO

## Prompt

```text
Design RefactorTactics v0.1 environment and status icon families.

Environment icons describe world/surface semantics.
Status icons describe a unit/state and must not be simple recolors of the environment glyph.

Examples:
Water Surface = hex/cell + water
Wet Status = unit/status frame + droplet
Fire Surface = cell + flame
Burning = status frame + flame
Electric payload = bolt
Electrified = status frame + electric payload
Cover = directional barrier
Shielded = shield pool icon

All pairs must remain distinguishable in grayscale.
```

---

# BATCH 5 — Reaction, Certainty, Warning

## Deliverable

Reaction states:

- Generic
- Available
- Armed
- Opportunity
- FIRE/Commit
- HOLD
- Consumed
- Expired
- Invalidated
- Timeout

Certainty:

- Confirmed
- Predicted
- Uncertain
- Invalid

Warnings:

- Info
- Warning
- Critical
- Friendly Fire
- Collision
- Insufficient Resource
- Invalid Target
- Invalid Path
- Cooldown
- Intent Not Committed
- Path Invalidated
- Uncertain Outcome
- Hazard
- Plan Changed
- Missing Plan
- Target May Move

## Prompt

```text
Create the RefactorTactics reaction/certainty/warning visual language.

Reaction uses a broken circular trigger-ring grammar. Do NOT use a lightning bolt.
Overwatch FIRE/HOLD must be readable in under one second.
HOLD must not look like Wait.

Confirmed = solid.
Predicted = dashed/hollow.
Uncertain = dotted/fade + question-mark modifier.
Invalid = slash/cross-hatch/blocked mark.

Warning hierarchy must work with shape + icon + pattern + text + color.
Do not use red as the sole signal.

Produce icon-only, compact-chip and full-row examples.
```

---

# BATCH 6 — Objective, Coordination, Perception

## Deliverable

Coordination:

- Ready/Unready
- Editing
- Locked
- Ally Intent
- Ping
- Conflict
- Shared Target

Objective:

- Neutral
- Capture
- Contested
- Owned
- Locked
- Completed
- Success
- Failure

Perception/Intel:

- Visible
- Detected
- Heard
- Approximate Area
- Unknown
- Last Known
- Targeted
- Focus
- Facing

## Prompt

```text
Design the RefactorTactics objective, coordination and perception icon sets.

Privacy rule:
visual language must never imply more precision than the player's authorized Team Knowledge.

Heard / Sound Contact must not look like an exact enemy silhouette on a precise cell.
Approximate information uses area/question grammar.
Last Known must look historical/faded, not current.

Team relation and faction identity are separate.
```

---

# BATCH 7 — Equipment / Loadout

## Deliverable

Weapon variants:

- Precision
- Impact
- Overcharge
- Split
- Suppressive
- Environmental

Gadgets:

- Medkit
- Breach Charge
- Sprinkler
- Insulator
- Smoke Emitter
- Portable Cover
- Sensor
- Anchor

Reaction modules:

- Emergency Dash
- Reactive Shield
- Counter Shot
- Ally Intercept
- Hazard Escape
- Cleanse
- Anchor

## Prompt

```text
Create compact RefactorTactics v0.1 equipment/loadout icons using the same semantic language as the HUD.

These must work in character inspect, loadout, tooltip and Wiki.
Do not use photorealistic item thumbnails.
Prefer compact tactical pictograms with one recognisable payload plus one functional cue.
```

---

# BATCH 8 — HUD Chrome / Static Effects

Produce:

- panels;
- action slot frames;
- skill bar group frames;
- portrait frames;
- resource frames;
- keycap frame;
- status stack frames;
- +N badge;
- warning chips/rows;
- objective chips;
- Ghost Timeline nodes;
- Reaction branch;
- boundary dividers;
- marker endcaps;
- selected/hover/planned/cooldown/invalid masks.

Do not rasterize dynamic paths/AoE/cones/numbers.

---

# QA MASTER CHECKLIST

## A. Actual size

Test at:

- 16 px;
- 20 px;
- 24 px;
- 32 px;
- 48 px;
- ability slot 56–60 px;
- 1920×1080 full HUD.

## B. Grayscale

Must pass:

- Ally vs Enemy;
- Move/Sprint/Dash;
- Electric vs Reaction;
- Shield/Brace/Cover;
- Confirmed/Predicted/Uncertain;
- Water/Fire;
- Wet/Water Surface;
- Burning/Fire Surface.

## C. CVD

Simulate:

- Protanopia;
- Deuteranopia;
- Tritanopia.

## D. Background torture

Test on:

- black;
- white;
- mid-gray;
- grass;
- concrete;
- metal;
- water;
- fire;
- ice;
- smoke;
- busy gameplay screenshot.

## E. Clutter

Test:

- 4 units visible;
- 8 unit stress layout;
- 2 status per unit + `+N`;
- multiple ally intents;
- 3 warnings;
- objective active;
- one reaction armed;
- one Fast Reaction prompt.

## F. State recognition

A player should distinguish within ~1 second:

- selected vs planned;
- planned vs cooldown;
- cooldown vs unavailable;
- reaction armed vs reaction opportunity;
- predicted vs uncertain;
- invalid target vs insufficient resource.

## G. UMG reconstruction

Reject asset if it requires:

- baked text;
- baked cooldown number;
- baked HP percentage;
- one texture per dynamic state combination;
- stretching that destroys corners;
- huge full-screen raster.

---

# DELIVERY STRUCTURE REQUESTED FROM CLAUDE DESIGN

```text
/DesignSource/
  Icons/
    Target/
    Geometry/
    Effects/
    Actions/
    Characters/
    Environment/
    Status/
    Reaction/
    Intel/
    Coordination/
    Warnings/
    Objectives/
    Equipment/
  HUD/
    Panels/
    Buttons/
    Slots/
    Portraits/
    Bars/
    Timeline/
    Markers/
    Masks/

/Exports/
  SVG/
  PNG/16/
  PNG/20/
  PNG/24/
  PNG/32/
  PNG/48/
  Boards/

/QA/
  Grayscale/
  CVD/
  BackgroundTests/
  1080p/
```

Final report:

```text
DONE
DEFERRED_V01
REFERENCE_ONLY
CONFLICT_FOUND
MISSING_SOURCE
```

Do not silently invent a missing gameplay state just to complete the art set.
