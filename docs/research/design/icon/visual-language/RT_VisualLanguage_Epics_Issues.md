# RefactorTactics — Epic / Issue Map per Tactical Visual Language

Questo file contiene **titoli candidati**. Claude CLI deve interrogare lo stato reale della repository/GitHub prima di creare, rinominare o duplicare qualunque elemento.

## Epic / issue già note

- **#217 — `[EPIC] E20 · HUD Icon Language`** · v0.1 · verify/update, never duplicate
- **#218 — `CP 20.1 · URTIconCatalogData`** · v0.1 · verify/update
- **#219 — `CP 20.2 · Categorie della v0.1`** · v0.1 · verify/update
- **#220 — `CP 20.3 · I widget consumano il catalogo`** · v0.1 · verify/update

## Epic post-v0.1 da verificare

- **`[EPIC] E25 · Icon Language completo`** · post-v0.1 · creare solo se non esiste già
  - `CP 25.1 · Tassonomia completa e governance`
  - `CP 25.2 · Catalogo completo, validator e authoring`
  - `CP 25.3 · HUD / world-space / reaction / perception integration`
  - `CP 25.4 · Accessibility, Wiki e documentazione`

## Issue candidate per checkpoint roadmap

### ICON-0 — Audit e consolidamento

**Release/Epic:** v0.1 / E20

**Scope:** Audit repository UI/icon state, map existing semantic IDs/tags/widgets, reconcile latest grammar with canonical docs; no duplicate systems.

- `UI: audit and consolidate tactical visual language`
- `Docs/UI: consolidate RT Visual Language Specification v0.1`

**Prompt Claude Design:** `ClaudeDesign/ICON-0_Audit_e_consolidamento.md`

### ICON-1 — Fondazione dati

**Release/Epic:** v0.1 / E20

**Scope:** Stable semantic IDs, catalog/resolver/fallback/logging, validators and deterministic lookup; preserve #218/#219/#220 ownership.

- `UI/Data: implement semantic visual token catalog and resolver`
- `Validation: add visual token catalog validation and fallback checks`

**Prompt Claude Design:** `ClaudeDesign/ICON-1_Fondazione_dati.md`

### ICON-2 — Core v0.1 Asset Set

**Release/Epic:** v0.1 / E20

**Scope:** Start with 16 monochrome proof-of-concept glyphs, then semantic palette/CVD only after silhouette approval.

- `Art/UI: create 16 monochrome tactical glyph masters`
- `Art/UI: validate 16/20/24/32 px optical glyph variants`
- `Art/UI: expand approved glyph language to the v0.1 core asset set`

**Prompt Claude Design:** `ClaudeDesign/ICON-2_Core_v0.1_Asset_Set.md`

### ICON-3 — Roster e Character HUD

**Release/Epic:** v0.1 consumer / E20; full post-v0.1 / E25

**Scope:** Identity, faction secondary badge, deterministic status stack, +N overflow, tooltip/inspect, 1080p/4v4-safe layout.

- `UI: integrate deterministic icon stack into roster and character HUD`
- `UI: integrate faction badge, Ready and critical status presentation`

**Prompt Claude Design:** `ClaudeDesign/ICON-3_Roster_e_Character_HUD.md`

### ICON-4 — Planning / Action Ghosts

**Release/Epic:** v0.1 consumer + post-v0.1 / E25

**Scope:** Own plan, sanitized ally plan, path/AoE/facing/ghost, certainty and team dependencies; no enemy private plan data.

- `UI: integrate visual grammar into planning and Action Ghosts`
- `UI: implement Confirmed / Predicted / Uncertain world-space styles`
- `UI: add ally-plan conflict and friendly-fire visual warnings`

**Prompt Claude Design:** `ClaudeDesign/ICON-4_Planning_Action_Ghosts.md`

### ICON-5 — Reaction / Overwatch

**Release/Epic:** post-v0.1 / E25

**Scope:** Available/Armed/Opportunity/Hold/Consumed/Expired/Invalidated; fast decision readability and privacy.

- `UI/Reaction: define and integrate Reaction / Overwatch visual lifecycle`
- `Network QA: verify reaction UI exposes no future enemy opportunities`

**Prompt Claude Design:** `ClaudeDesign/ICON-5_Reaction_Overwatch.md`

### ICON-6 — Perception / Noise / Fog of War

**Release/Epic:** post-v0.1 / E25

**Scope:** Exact visual contact vs acoustic areas/direction vs memory; uncertain info uses uncertain geometry.

- `UI/Perception: implement Visible / Heard / Last Known / Unknown visual semantics`
- `UI/Perception: implement acoustic precision and uncertainty-area presentation`
- `Network QA: verify knowledge UI never reveals unauthorized exact positions`

**Prompt Claude Design:** `ClaudeDesign/ICON-6_Perception_Noise_Fog_of_War.md`

### ICON-7 — Environment / Map Interaction

**Release/Epic:** post-v0.1 / E25

**Scope:** Water/Fire/Electric/Cold/Ice/Steam/Smoke, cell/unit/edge frames, map-state transitions and dependency previews.

- `UI/Map: integrate environment visual grammar`
- `UI/Map: integrate door, cover, bridge, lift and interaction state presentation`

**Prompt Claude Design:** `ClaudeDesign/ICON-7_Environment_Map_Interaction.md`

### ICON-8 — Combat Log / Explainability

**Release/Epic:** post-v0.1 / E25

**Scope:** Damage/mitigation/absorption/block/resistance/status/displacement and causal reason chain; icons supplement text/reason codes.

- `UI/CombatLog: render outcome ReasonChain with canonical visual tokens`
- `UI: implement compact WHY? outcome explanation`

**Prompt Claude Design:** `ClaudeDesign/ICON-8_Combat_Log_Explainability.md`

### ICON-9 — Wiki / Tutorial / Scenario Browser

**Release/Epic:** post-v0.1 / E25

**Scope:** Same SemanticId and meaning across game UI and docs, accessibility labels, canonical examples, no independent wiki-only icon language.

- `Docs/Wiki: publish canonical visual-token legend`
- `Docs/UI: reuse visual language in tutorial and scenario browser`

**Prompt Claude Design:** `ClaudeDesign/ICON-9_Wiki_Tutorial_Scenario_Browser.md`

### ICON-10 — Accessibility e Polish

**Release/Epic:** post-v0.1 / E25

**Scope:** Default accessible, CVD, High Contrast, grayscale, reduced-motion equivalents, pattern/outline/luminance validation.

- `Accessibility/UI: implement semantic CVD and High Contrast themes`
- `Accessibility/UI: validate grayscale, reduced motion, scaling and contrast`
- `Art/UI: complete visual-language accessibility review`

**Prompt Claude Design:** `ClaudeDesign/ICON-10_Accessibility_e_Polish.md`

### ICON-11 — Performance / 4v4 Stress

**Release/Epic:** post-v0.1 / E25

**Scope:** 8 units + statuses + reactions + objectives + hazards + ally intents + warnings + FoW; measure clutter, widget count, invalidation, overdraw and readability.

- `QA/Performance: 4v4 tactical UI clutter and rendering stress test`
- `Tooling/UI: add Visual Language Lab, icon gallery and presentation debug overlay`

**Prompt Claude Design:** `ClaudeDesign/ICON-11_Performance_4v4_Stress.md`


## Regole per Claude CLI

Ogni issue creata/aggiornata deve contenere almeno:
- Obiettivo
- Scope
- Fuori scope
- Dipendenze
- Acceptance criteria
- Test strategy
- Feature Registry refs
- Wiki/spec refs
- Prompt Claude Design associato, se esiste un deliverable visuale
- Evidenza richiesta (screenshot/board/export) quando manuale

Non inventare numeri issue. Dopo la creazione, aggiornare roadmap e Feature Registry con i numeri reali.
