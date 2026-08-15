# CLAUDE DESIGN 01 — Icon Language & Visual Rules v0.1

## Missione

Disegnare una **grammatica iconografica**, non un set di pittogrammi casuali.

Formula visuale base:

```text
TARGET + GEOMETRY → EFFECT + MODIFIER / STATE
```

Esempi:

```text
Enemy + Line → Damage + Electric
Cell + Circle → Water
Self + Dash → Reposition
Enemy + Cone → Overwatch / Reaction
Ally + Circle → Heal + Water
```

Una skill complessa può avere una hero mark riconoscibile, ma deve restare semanticamente leggibile tramite primitive coerenti.

---

# 1. Principi LOCKED

1. Le icone sono primitive semantiche componibili.
2. Se una distinzione competitiva sparisce in grayscale, il design è da rifare.
3. Ally ed Enemy differiscono per **forma + colore**, non solo tint.
4. Evitare rosso/verde come coppia primaria Ally/Enemy.
5. `Electric` e `Reaction` devono avere silhouette completamente diverse.
6. `Line` e `Move` devono essere distinguibili:
   - Line = origine + segmento + punta;
   - Move = percorso con nodi.
7. Certainty:
   - Confirmed = solid;
   - Predicted = dashed;
   - Uncertain = dotted/fade + `?`.
8. Invalid = slash/cross-hatch/`⊘`, non solo colore rosso.
9. Buff/debuff non dipendono da verde/rosso: usare `↑`, `↓`, `⊘` o shape equivalenti.
10. Leggibilità minima target: **20–24 px a 1920×1080**.
11. Preparare Default Accessible, CVD e High Contrast senza cambiare semantica/layout.
12. Nessuna icona deve richiedere glow per essere riconoscibile.
13. Reaction NON usa un fulmine. Il fulmine appartiene a Electric.
14. Brace, Shield e Cover devono essere tre concetti visualmente diversi.
15. Dash e Sprint devono essere leggibili come due intenzioni diverse.

---

# 2. Griglia grafica

## Master glyph

- canvas: `24×24 px`;
- visual bounds preferiti: `20×20 px`;
- core mass: ~`18×18 px`;
- safe margin: ~`2 px`;
- centro logico: `12,12`;
- guide: `4 / 8 / 12 / 16 / 20`.

## Stroke

- 24 px: ~2 px;
- 20 px: ~2 px ottici;
- 16 px: variante semplificata dedicata;
- 32 px: ~2.5–3 px ottici;
- evitare hairline da 1 px per parti semantiche.

## Complessità

- 24 px: max 3 componenti dominanti;
- 20 px: 2–3;
- 16 px: max 2;
- niente micro-texture o ornamentazione interna.

## Linguaggio delle forme

- Ally / Support / Movement: leggermente rounded;
- Enemy / Attack / Warning: più angular/notched;
- Cell / Objective / Geometry: neutral-geometric;
- Reaction: circle/broken-ring grammar;
- Map interaction: structural/edge grammar;
- Status: compact enclosed/emblem grammar.

---

# 3. Palette

## Base HUD

- Background 0: `#080F14`
- Background 1: `#151A23`
- Panel: `#212733`
- Structural blue-gray: `#203542`
- Secondary border: `#4A5568`
- Primary text/icon: `#F2F4F7`
- Secondary text/icon: `#A7ADB5`

## Semantic accessible playtest

| Token | HEX | Uso |
|---|---|---|
| Ally | `#56B4E9` | team relation, quando autorizzata |
| Enemy | `#E69F00` | team relation, quando autorizzata |
| Water | `#0072B2` | water/wet |
| Fire | `#D55E00` | fire/burning |
| Electric | `#F0E442` | electricity; dark outline obbligatorio |
| Defense | `#009E73` | shield/defense accent |
| Reaction | `#CC79A7` | reaction/decision branch |
| UI Cyan | `#00E0FF` | focus/selected/info |
| UI Purple | `#7C5CFF` | special/reaction accent secondario |
| UI Gold | `#FFD456` | selected/important/ready |
| Critical | `#FF4D4D` | solo come rinforzo, mai unico segnale |
| White | `#FFFFFF` | high contrast |

### Regole

- colore = secondo canale;
- silhouette/pattern = primo canale;
- Fire/Water/Electric non cambiano in base al team;
- Electric usa core chiaro + outline scuro;
- Warning usa triangolo/notch/`!` oltre al colore;
- Ally/Enemy mantengono shape differente anche in monocromia.

---

# 4. Core glyph grammar

## Target

### Ally
Rounded unit marker + connection tabs. No `+`.

### Enemy
Angular unit marker + minimal reticle/notch.

### Self
Unit marker con inward focus ring, distinto da Ally.

### Cell
Esagono 2D pulito. Nessuna prospettiva.

### Object / Structure
Piccolo blocco/oggetto geometrico con base/anchor.

### Direction
Chevron singolo/sector arrow; non confondere con Move.

---

## Geometry

### Line
`● ─── ►` senza nodi intermedi.

### Circle
Outer ring + center point.

### Cone
Origin + due edge divergenti + boundary curvo.

### Chain
Nodo origine + 2/3 nodi collegati con salto segmentato. Non disegnare solo un fulmine.

### Arc / Sector
Curved sector boundary + origin/facing anchor.

---

## Effects

### Damage
Impact/crack astratto, 3–4 diramazioni max. No sword/bullet/skull/flame.

### Heal
Pulse/cross soft geometric, usato solo per Heal, non Ally.

### Push
`» ● ─►` / impulso esterno + unit dot + uscita.

### Pull
`◄─ ● «` / inverse impulse.

### Shield
Scudo semplice con grande negative space. Non Cover/Brace.

### Cover
Segmento/barriera tattica con side-protection cue.

### Interrupt
Break/notch che interrompe una linea/azione.

### Reposition
Short relocation arrow con origin/destination, distinta da Move path.

### Reveal / Detect
Sensor/radar reveal mark, distinto da Overwatch eye/sector.

---

# 5. Movement grammar

### Move
`●──•──•──►`

I nodi intermedi sono fondamentali.

### Sprint
Stessa famiglia di Move ma con:

- stride spacing più lungo;
- doppia speed trail;
- endpoint aggressivo;
- silhouette diversa da Dash.

Non usare semplicemente la stessa icona Move con `x2`.

### Dash
`●──»──»──►`

A 16 px: `● » ►`.

Dash comunica movimento speciale/evasivo/reposition rapido.

### Forced Movement
Unit dot + external impulse; deve usare Push/Pull grammar, non Dash.

---

# 6. Defense / Reaction grammar

### Brace
Postura difensiva/anchor stance:

- body/anchor + braced wedge;
- non usare lo stesso shield glyph;
- deve suggerire "preparo a ricevere l'impatto".

### Overwatch
Facing/controlled sector:

- eye/reticle + sector/cone;
- deve comunicare controllo direzionale;
- non usare una semplice crosshair identica a Basic Attack.

### Reaction generic
Broken circular arc + trigger dot/notch + inward cue.

No lightning bolt. No clock.

### FIRE
Commit/reticle hit marker.

### HOLD
Open hand / pause palm / retained-state mark. Non sembrare Wait.

---

# 7. Certainty & state renderer

Questi sono principalmente **style modifiers**, non icone separate per ogni oggetto.

## Confirmed

- solid border;
- normal fill;
- normal opacity;
- no `?`.

## Predicted

- dashed border;
- hollow/low fill;
- ghost opacity;
- optional team-intent micro-marker.

## Uncertain

- dotted/discontinuous boundary;
- fade;
- `?` secondary mark;
- se la posizione è incerta, rappresentare un'area, non una falsa cella esatta.

## Invalid

- slash / cross-hatch / `⊘`;
- muted neutral;
- optional critical accent.

## Disabled / Unavailable

- non abbassare tutto a 10–15% opacity;
- silhouette ancora leggibile;
- desaturazione + frame disabled + status glyph.

---

# 8. Slot art vs semantic glyph

## Semantic glyph

Usato per:

- tooltip;
- combat log;
- warning;
- wiki;
- composizione;
- piccoli badge.

Deve essere monocromatico/tintabile.

## Hero ability slot art

Può essere più caratterizzato, ma deve:

- usare max 2–3 elementi principali;
- mantenere una silhouette forte;
- richiamare le primitive semanticamente corrette;
- evitare mini-illustrazioni;
- funzionare a 32–36 px dentro slot 56–60 px.

Esempio:

`Flux.LinearDischarge` = Line + Electric + controlled branch hint.

Non creare un'immagine narrativa complessa di Flux che spara.

---

# 9. Export

## Preferred

- SVG/vector master per ogni glyph;
- PNG RGBA trasparente 16/20/24/32/48 px;
- source file/design file;
- artboard PNG review 2x/4x.

## Regole PNG

- alpha pulito;
- no matte;
- no testo incorporato;
- padding coerente;
- crop coerente;
- tintable dove possibile;
- glow su layer separato se serve;
- non rasterizzare numeri/cooldown/keybind nel glyph.

## Unreal naming

`RT_UI_<Category>_<Name>_<State>`

Esempi:

- `RT_UI_Icon_Action_Dash`
- `RT_UI_Icon_Status_Wet`
- `RT_UI_Icon_Reaction_Armed`
- `RT_UI_Icon_Warning_FriendlyFire`
- `RT_UI_Marker_Uncertain`
- `RT_UI_Slot_Ability_Selected`

---

# 10. Accessibilità obbligatoria

Preparare review in:

- Default Accessible;
- CVD;
- High Contrast;
- Grayscale.

Simulazioni:

- Protanopia;
- Deuteranopia;
- Tritanopia;
- Grayscale.

Background torture:

- black;
- white;
- gray;
- grass;
- metal;
- water;
- fire;
- ice;
- smoke/steam;
- busy game screenshot.

---

# 11. Collisioni semantiche da testare

Queste coppie devono essere distinguibili senza label:

- Line vs Move;
- Move vs Sprint vs Dash;
- Electric vs Reaction;
- Shield vs Brace vs Cover;
- Attack vs Overwatch;
- Wait vs Hold;
- Water vs Fire;
- Wet vs Water Surface;
- Electrified vs Electric payload;
- Burning vs Fire Surface;
- Ally vs Enemy;
- Confirmed vs Predicted vs Uncertain;
- Push vs Dash;
- Interact vs Objective Interaction;
- Detected vs Heard vs Last Known.
