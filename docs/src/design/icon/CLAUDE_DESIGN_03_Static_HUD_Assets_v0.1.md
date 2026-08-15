# CLAUDE DESIGN 03 — Static HUD Assets v0.1

## Obiettivo

Produrre gli asset statici che Unreal/UMG può realmente riusare senza trasformare la HUD in una grande immagine raster.

La regola è:

```text
STATIC ART = frame, mask, icon, texture, structural state
DYNAMIC UI = text, number, fill, path, AoE, world geometry, live state
```

---

# 1. Panels

Produrre come 9-slice o asset separabili.

| Asset | Uso |
|---|---|
| `RT_UI_Panel_Primary` | selected unit / main card |
| `RT_UI_Panel_Secondary` | roster/objective groups |
| `RT_UI_Panel_Compact` | compact info / micro panels |
| `RT_UI_Panel_Tooltip` | ability/target tooltip |
| `RT_UI_Panel_Warning` | warning block |
| `RT_UI_Panel_FastReaction` | urgent decision window |
| `RT_UI_Panel_TeamIntent` | ally intent / shared planning |
| `RT_UI_Panel_CombatLog` | collapsible log frame |
| `RT_UI_Panel_Objective` | objective panel |

Rules:

- dark translucent background;
- restrained metallic edge;
- subtle angular/hex cuts;
- no baked text;
- center must stretch safely;
- decorative corners must not distort.

---

# 2. Buttons

Families:

- Primary;
- Secondary;
- Icon;
- Ready / Confirm Plan;
- Fast Reaction choice.

States:

- Normal;
- Hover;
- Pressed;
- Selected where needed;
- Disabled;
- Warning/Invalid where needed.

Suggested assets:

```text
RT_UI_Button_Primary_Normal
RT_UI_Button_Primary_Hover
RT_UI_Button_Primary_Pressed
RT_UI_Button_Primary_Disabled

RT_UI_Button_Secondary_*
RT_UI_Button_Icon_*
RT_UI_Button_Ready_*
RT_UI_Button_FastReaction_*
```

Do not bake labels `READY`, `FIRE`, `HOLD`, keybinds or timers into texture.

---

# 3. Action Slot frames

## Families

### Movement Slot
For Move / Sprint / Dash.

### Basic Attack / Character Ability Slot
Center kit family.

### Reaction Slot
Brace / Overwatch.

### Context Action Slot
Interact / Wait.

Each slot is layered:

1. base frame;
2. icon/art;
3. focus border;
4. shortcut anchor;
5. resource/charge anchor;
6. cooldown overlay mask;
7. planned marker;
8. warning marker;
9. disabled/invalid overlay;
10. optional semantic accent line.

States:

- Available;
- Hover;
- Selected;
- Planned;
- Cooldown;
- Unavailable;
- Invalid;
- Warning;
- Armed for reaction slots.

Suggested:

```text
RT_UI_Slot_Movement_Available
RT_UI_Slot_Movement_Selected
RT_UI_Slot_Ability_Available
RT_UI_Slot_Ability_Selected
RT_UI_Slot_Reaction_Armed
RT_UI_Slot_Context_Available
RT_UI_Slot_Common_CooldownMask
RT_UI_Slot_Common_InvalidMask
RT_UI_Slot_Common_PlannedMarker
```

Avoid generating a full PNG for every possible combination of state + character + cooldown number.

---

# 4. Skill bar group frames

Produce lightweight frames/headers for:

- `MOVEMENT`;
- `CHARACTER KIT`;
- `REACTION`;
- contextual row.

Text remains dynamic; the asset is only the structural frame/divider.

The grouping must be visually obvious without suggesting three independent action economies.

---

# 5. Portrait frames

Variants:

- Own Character;
- Ally;
- Enemy / Neutral;
- Selected;
- KO / Unavailable.

Overlays separate from frame:

- Ready;
- Unready;
- Editing;
- Locked;
- Selected;
- Targeted;
- Reaction Armed;
- Low Health;
- KO;
- faction badge anchor;
- status stack anchor.

Never bake character portrait into reusable frame.

---

# 6. Resource bars

Families:

- Health;
- Shield;
- Hero Resource;
- Timer / Progress;
- Objective progress.

Separate:

1. frame;
2. background;
3. fill mask;
4. marker/ticks;
5. text anchor.

Do not create one texture per percentage.

Static art candidates:

```text
RT_UI_Bar_Health_Frame
RT_UI_Bar_Health_Background
RT_UI_Bar_Shield_Frame
RT_UI_Bar_Resource_Frame
RT_UI_Bar_Timer_Frame
RT_UI_Bar_Progress_Frame
RT_UI_Bar_TickMarker
```

---

# 7. Ghost Timeline static UI

Timeline phases:

```text
PREP — DASH — BLAST — MOVE
```

Produce:

- phase node empty;
- phase node populated;
- phase node selected;
- phase node inactive;
- phase node completed;
- connector;
- current-phase indicator;
- phase boundary divider;
- decision boundary divider;
- reaction branch anchor;
- delayed/predictive branch only as future/reference layer.

The timeline must look like **phase state**, not a free-form action queue.

---

# 8. Fast Reaction static assets

Produce:

- `RT_UI_Panel_FastReaction`;
- reaction badge/emblem;
- target choice button frame;
- FIRE button frame;
- HOLD button frame;
- countdown ring/frame;
- urgency pulse mask/glow layer;
- multi-target option frame;
- timeout/expired overlay.

Timer number is dynamic.

Do NOT show `Opportunity 1/3` or any future-opportunity count.

---

# 9. Warning system assets

For each warning semantic icon, support:

1. Icon-only;
2. Compact chip frame;
3. Full warning row frame.

Three severity chrome variants:

- Info;
- Warning;
- Critical.

Shape + icon + pattern + text + color.

Do not encode severity only through hue.

---

# 10. Tactical marker static source assets

Good static candidates:

- waypoint marker;
- destination marker;
- selected/focus marker;
- targeted marker;
- last-contact pin;
- sound-contact pin;
- facing anchor;
- cover marker;
- objective marker;
- ally-intent marker;
- ping marker;
- uncertainty `?` badge;
- confirmed/predicted/uncertain boundary pattern masks.

World position and path are dynamic.

---

# 11. Status stack assets

Produce:

- compact status chip frame;
- critical-status frame;
- reaction-status frame;
- `+N` overflow badge;
- tooltip/inspect frame.

Do not make `+2`, `+3`, `+4` as separate textures. Only the badge frame is static.

Recommended runtime stack:

```text
Identity: max 1 primary
Critical Status: max 2
Reaction: max 1
Warning: max 1
Overflow: +N
```

---

# 12. Skill Card / Tooltip chrome

Produce reusable sections:

- header frame;
- phase badge frame;
- availability badge;
- cost badge;
- semantic-token row frame;
- effects section divider;
- current-context inset;
- warning inset;
- target-preview row frame;
- WHY?/explanation row.

Text and values are dynamic.

Recommended skill card target:

- ~380 px width;
- 300–520 px height;
- 16 px outer padding;
- 12 px section gap;
- 6 px token gap;
- 8 px row gap.

---

# 13. Objective chrome

Produce static chips/frames for:

- Neutral;
- Capture;
- Contested;
- Owned;
- Locked;
- Completed;
- Mission Success;
- Mission Failure.

Objective name, score, countdown and progress are dynamic.

---

# 14. Input / keycap assets

Produce generic keycap frame(s), not one texture per key:

- keyboard key square;
- wide key / Space;
- mouse icon frame if used;
- controller button backing if needed later.

The `Q/W/E/1/2/3/R/T/Space` labels remain text/glyphs layered by UI.

---

# 15. Static effects / state masks

Useful reusable masks:

- selected glow mask;
- hover edge glow;
- planned diagonal/marker;
- cooldown radial darkening mask;
- invalid cross-hatch;
- predicted dash pattern;
- uncertain dots/fade mask;
- critical notch/pulse mask;
- disabled grain/neutral overlay;
- reaction armed ring;
- opportunity pulse ring.

Keep them tintable.

---

# 16. DO NOT create as static PNG

These must be dynamic in Unreal through Material/UI Material/decals/geometry/instancing/custom line/world primitives:

- movement path;
- individual path nodes along arbitrary route;
- reachable cells;
- AoE of arbitrary size;
- line targeting of arbitrary length;
- chain connection lines;
- facing cone;
- Overwatch cone;
- uncertainty area;
- fog/sound uncertainty footprint;
- Action Ghost character;
- projected future character position;
- dynamic cover relation line;
- long target ray;
- HP/resource fill percentage;
- cooldown number;
- timer number;
- charge number;
- objective progress percentage;
- labels, names and keybind text.

Static art may provide **masks/endcaps/patterns** for these systems, but not the final geometry.

---

# 17. Technical export checklist

Every static asset should declare:

```text
AssetName
Category
NativeSize
9SliceMargins if applicable
Tintable? yes/no
Alpha mode
Expected UMG usage
Expected world-space usage
CVD-safe? yes/no
HighContrast variant? yes/no
```

For glyphs:

- SVG master;
- PNG 16/20/24/32/48;
- transparent alpha;
- consistent crop;
- monochrome master first.

For frames:

- 1x and 2x source;
- 9-slice guide screenshot or metadata;
- transparent corners;
- no baked text.
