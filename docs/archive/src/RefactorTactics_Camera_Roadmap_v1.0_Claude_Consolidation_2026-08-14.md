# RefactorTactics — Camera Roadmap v1.0
## Handoff operativo per Claude Code: consolidare roadmap, Epic/issue, Feature Registry, scenari, Editor Map, Wiki e test

**Data:** 2026-08-14  
**Repository atteso:** `DegrassiAaron/refactor-tactics-main`  
**Baseline UE da verificare su `main`:** Unreal Engine 5.8.1  
**Tipo documento:** handoff di consolidamento, NON nuova source of truth.

---

# 0. Mandato

Consolida nel repository reale tutte le decisioni correnti sulla **camera tattica di RefactorTactics** e trasformale in una roadmap eseguibile fino alla **v1.0**.

NON creare una roadmap parallela.

Devi:

1. verificare `origin/main`, issue, PR, Decision Log/ADR, Feature Registry e roadmap correnti;
2. cercare prima Feature/Epic/issue già esistenti;
3. **REUSE / UPDATE** prima di **CREATE**;
4. integrare le nuove decisioni camera negli owner reali;
5. collegare:
   `Release -> Feature -> Epic/Issue -> Scenario/Test -> Evidence -> Wiki/Spec`;
6. creare nuove issue solo per gap reali;
7. non assegnare numeri Epic/issue a memoria;
8. non reimplementare la camera base se è già presente;
9. non trasformare questa roadmap in feature creep della v0.1;
10. arrivare a una camera production-ready entro v1.0 con privacy, replay/spectator, performance, accessibilità e packaged validation.

---

# 1. Preflight obbligatorio

Prima di modificare:

```bash
git status
git branch --show-current
git fetch --all --prune
git rev-parse HEAD
git rev-parse origin/main
git log -10 --oneline --decorate
git worktree list
```

Leggi almeno, usando i path reali se sono cambiati:

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
docs/roadmap/feature-registry.yaml
docs/roadmap/execution-graph.yaml
docs/roadmap/editor-sessions.yaml

docs/technical/scenario-map.md
docs/technical/test-manuali-pie.md
docs/technical/test-automatico-unreal.md
docs/technical/spec-turnlog.md

docs/technical/brief-planning-visuale.md
docs/technical/progettazione-hud.md
docs/gameplay/*
docs/technical/*camera*
docs/technical/*map*
docs/technical/*perception*
docs/technical/*replay*

Source/RefactorTactics/
Source/RefactorTactics/Tests/
Source/RefactorTactics/ScenarioHarness/
Source/RefactorTacticsEditor/
Content/RefactorTactics/
Scenarios/
```

Auditare inoltre:

```text
RT-FEAT-UI-TACTICAL-CAMERA
RT-FEAT-UI-CELL-SELECTION
RT-FEAT-UI-LAYER-FILTER
RT-FEAT-UI-PLANNING
RT-FEAT-UI-AOE-GHOST
RT-FEAT-UI-ACTION-GHOSTS
RT-FEAT-UI-FAST-DECISION
RT-FEAT-UI-CERTAINTY
RT-FEAT-UI-ACCESSIBILITY

RT-FEAT-PERCEPTION-TEAM-KNOWLEDGE
RT-FEAT-PERCEPTION-MEMORY
RT-FEAT-PERCEPTION-NOISE
RT-FEAT-PERCEPTION-SOUND-OVERLAY

eventuali Feature già esistenti per:
Strategic View / Strategic Map
Camera Director
Replay Camera
Spectator Camera
Camera Lab
Multilayer markers
Occlusion / Cutaway
```

Stato storico noto: `RT-FEAT-UI-TACTICAL-CAMERA` risultava già **IMPLEMENTED**, mentre `RT-FEAT-UI-LAYER-FILTER` risultava **DESIGNED**. NON sovrascrivere lo stato senza verifica live.

---

# 2. Roadmap corrente: non inventare un asse parallelo

La fotografia più recente disponibile prima di questo handoff usa:

```text
v0.1 — Vertical Slice
v0.2 — Struttura e finestre
v0.3 — Informazione
v0.4 — Operations
v0.5 — Online Foundation
v0.6 — Ability Runtime
v0.7 — Competitive Alpha
v0.8 — Beta / Balance
v0.9 — Release Candidate
v1.0 — Launch
```

e una snapshot recente associa:

```text
v0.5 -> E40
v0.6 -> E41
v0.7 -> E42
v0.8 -> E43
v0.9 -> E44
v1.0 -> E45
```

**Questi mapping vanno riverificati su `main`.**

Se la roadmap live differisce, mappa il lavoro camera alla roadmap reale senza creare nuove release o gate.

---

# 3. Decisioni camera da consolidare

## 3.1 Modello camera

- camera perspective quasi-isometrica;
- `CameraPivot` come punto osservato;
- camera presentation-only: nessuna authority su gameplay, LOS, targeting o visibility;
- pan relativo alla camera;
- MMB drag per trascinare il battlefield;
- zoom continuo verso il cursore;
- yaw libero 360°;
- pitch manuale limitato;
- Q/E come snap alle direzioni canoniche;
- niente snap automatico al rilascio;
- `Home` reset orientamento/pitch;
- soft map bounds, non hard wall;
- pan speed e zoom step scalano con la distanza.

## 3.2 Input

Decisione corrente:

```text
LMB                selezione / conferma primaria contestuale
RMB                azione secondaria contestuale / cancel dove previsto
MMB drag           pan
Alt + MMB drag X   yaw libero
Alt + MMB drag Y   pitch limitato
Wheel              zoom verso cursore
Q / E              snap direzione canonica precedente/successiva
F                  focus selezione
Double LMB         select + focus
PageUp/PageDown    layer sopra/sotto
Home               reset camera orientation
M                  Strategic Overview / ritorno
Back                ritorno a PreviousCameraState dove applicabile
```

Tutto rimappabile tramite Enhanced Input.

**RMB resta gameplay contestuale**: NON assegnarlo alla rotazione camera.

## 3.3 Zoom e Strategic View

Non creare tre modalità rigide Combat/Tactical/Strategic.

Usare:

```text
zoom continuo
+
una sola soglia funzionale Strategic
```

con isteresi:

```text
StrategicEnterThreshold
StrategicExitThreshold
```

I valori sono tuning da `L_CameraLab`, non decisioni da hardcodare ora.

La Strategic View:
- è lo stesso sistema camera;
- abilita gradualmente una rappresentazione multipiano;
- usa semantic LOD;
- non concede più informazione della Tactical View;
- mantiene un `ActiveLayer`;
- può usare separazione verticale presentation-only tra layer;
- semplifica geometria, marker, path e hazard.

## 3.4 Multilayer

In Tactical:
- un solo layer geometrico pienamente attivo;
- altri layer NON vengono aperti solo perché una skill è selezionata;
- unità/obiettivi conosciuti su altri layer possono avere marker autorizzati `▲n / ▼n`;
- posizione precisa solo se realmente nota;
- Last Known = marker hollow/dashed;
- rumore = direzione/area/marker coerente con la precisione acustica;
- unknown = niente.

In Strategic:
- più layer possono essere mostrati contemporaneamente;
- geometria semplificata;
- `ActiveLayer` resta evidenziato;
- click su un altro layer/oggetto può renderlo nuovo `ActiveLayer`;
- zoom-in conserva il layer scelto.

## 3.5 Selezione multilivello

In Tactical:
- il mouse interagisce per default con `ActiveLayer`;
- un raycast fisico NON decide da solo il layer logico;
- nessuna autoselezione del layer “più vicino” se la cella non esiste sul layer attivo;
- marker di altri layer sono entità UI/knowledge separate;
- click marker -> switch layer + focus;
- transizioni verticali sono edge/interactions espliciti del grafo;
- movement planning cambia layer solo tramite transizione valida.

In Strategic:
- i layer visibili sono direttamente interattivi;
- click può cambiare `ActiveLayer`.

## 3.6 Targeting e privacy

Regola competitiva:

> Selezionare una skill NON aumenta la conoscenza del giocatore, salvo una esplicita ability di perception.

Separare:

```text
Geometric Reach
Known Space
Valid Target
```

Skill standard:
- non mostrano piani nascosti;
- non aprono stanze/zone non note;
- non rivelano hidden enemy tramite valid/invalid cursor;
- non trasformano noise/last-known in target preciso;
- non interrogano hidden state come oracle.

Policy concettuali da riconciliare con il sistema reale:

```text
VisibleOnly
KnownSpace
PerceptionAbility
```

Il server può rivalidare sullo stato autorevole; il client preview deve usare esclusivamente Authorized/Team Knowledge.

## 3.7 Cutaway / occlusion

Combat/Tactical:
- roof/ceiling sopra l'ActiveLayer -> cutaway locale;
- wall/large structure che copre il focus -> fade/dissolve;
- decorative occluder -> fade più aggressivo;
- bridge -> fade locale mantenendo outline/struttura;
- tunnel -> cutaway locale della superficie sopra;
- usare `FocusRegion`, non singolo raycast fragile;
- hysteresis per evitare flicker;
- cover faded resta tatticamente leggibile;
- camera occlusion != gameplay LOS.

Strategic:
- niente cutaway complesso per muro;
- usare semantic LOD multilayer.

## 3.8 Collisione camera

- evitare comportamento standard SpringArm che accorcia continuamente la camera;
- gli ostacoli normali non devono cambiare involontariamente zoom/distanza;
- fade/cutaway è la soluzione primaria;
- safety collision solo per impedire camera dentro volumi invalidi/terrain;
- zoom min/max indipendente dagli ostacoli;
- Strategic quasi priva di collisioni geometriche.

## 3.9 Focus

- single click seleziona, non centra;
- `F` focalizza;
- double click = select + focus;
- focus conserva yaw/pitch e, se possibile, zoom;
- focus può usare bounds e fare piccolo zoom-out;
- planning: autofocus quasi zero;
- ping/notification non spostano camera automaticamente;
- input manuale cancella autofocus;
- breve suppression dopo input manuale;
- Fast Reaction può forzare focus e, se necessario, cambiare layer;
- `PreviousCameraState` supporta return/overview/reaction;
- focus usa solo informazioni autorizzate.

## 3.10 Ability Context

Durante Planning:

```text
Unit Context
-> Ability Context
-> Targeting Context
```

Selezionare un'abilità:
- cambia overlay/framing relevance;
- NON muove immediatamente camera;
- NON aumenta intel.

Framing suggerito:
- Line: source + line + impact;
- AoE: caster + AoE bounds;
- Dash: start + path + destination;
- Cone/Overwatch: origin + direction + area;
- Self/Stance: niente camera move;
- Map interaction: area/interactable.

Auto-reframe solo se il target/preview sta realmente uscendo dal frame.

## 3.11 Camera Director

Durante Resolution:
- consuma presentation events derivati da TurnLog;
- raggruppa eventi simultanei e spazialmente vicini;
- prima domanda: “è già leggibile nel frame corrente?”;
- default = KEEP;
- seconda scelta = REFRAME (pan + piccolo zoom);
- Hard Focus solo per decisioni critiche/Fast Reaction;
- non modificare yaw/pitch nella baseline;
- movimento normale non genera camera cut;
- eventi secondari off-screen -> marker cliccabile;
- simultaneous distant events restano simultanei logicamente anche se mostrati in sequenza;
- Fast Reaction può interrompere fast-forward e portare a real-time.

## 3.12 Playback / comfort

- transizioni brevi, senza overshoot;
- zoom automatico conservativo;
- shake basso;
- `Reduce Camera Motion`;
- playback 1x / 2x / 4x / Skip dove il sistema replay lo permette;
- Director meno aggressivo a velocità elevate;
- Fast Reaction sempre real-time;
- cambio automatico layer molto raro;
- settings persistenti.

## 3.13 Overview / minimap

**Nuova direzione da riconciliare contro i vecchi documenti Tactical Awareness/Minimap:**

- nessuna minimappa tradizionale obbligatoria per v0.1;
- Strategic View / `M -> Strategic Overview` è il primary overview;
- bussola/orientamento + off-screen markers restano persistenti;
- minimappa classica si rivaluta solo se playtest su mappe più grandi dimostra un problema reale.

Se il repository oggi dichiara una minimappa come feature locked, NON cancellarla in silenzio:
- registra la decisione/supersessione corretta;
- oppure mantienila come future/playtest se coerente.

## 3.14 Replay / spectator

Una sola camera core, più command sources:

```text
Player Input
Replay Director
Spectator Input/Director
        ↓
Camera State / Focus Requests
        ↓
Tactical Camera
```

- TurnLog/replay usa Stable IDs/FRTCellId, non Actor pointer come authority;
- replay manuale e automatico condividono il sistema;
- spectator live deve avere Information Policy sanitizzata;
- Strategic spectator non implica full-information senza autorizzazione;
- `FRTCameraState`/equivalente concettuale salva:
  Pivot, Yaw, Pitch, Zoom, ActiveLayer, view state.

---

# 4. Strategia Epic

Prima di creare una nuova Epic, cercare ownership camera/UI esistente dentro:

```text
E11 HUD / Planning / Debug / Explainability
E13 TeamKnowledge / Perception
E14 Decision Window / Reaction
E15 Showcase / Integrated UX
E27 Perception completa
E30/E31 Operations / strategic scale
Epic replay
Epic spectator
Epic accessibility
Epic competitive / beta / launch
```

Possibile umbrella SOLO SE esiste un gap reale:

> **Camera, Strategic View & Multilayer Readability**

Non crearla se `RT-FEAT-UI-TACTICAL-CAMERA` + Epic UI/Operations già possono possedere il lavoro.

---

# 5. Roadmap Camera fino a v1.0 — issue title candidates

**Regola:** questi sono TITOLI candidati, non numeri GitHub.  
Per ognuno: `SEARCH -> REUSE/UPDATE -> CREATE only if gap`.

---

## v0.1 — Vertical Slice / baseline camera

Non allargare lo scope se la baseline è già sufficientemente chiusa.

### Candidate issues

1. **`[Camera] Reconcile tactical camera baseline with current input and focus contract`** — P0/P1  
2. **`[Camera] Add free yaw, constrained pitch and canonical snap rotation`** — P1  
3. **`[Camera] Add cursor-anchored zoom, zoom-scaled pan and soft map bounds`** — P1  
4. **`[Camera] Integrate selection, focus and planning ability camera contexts`** — P1  
5. **`[Camera QA] Add baseline camera state automation and Camera Lab smoke coverage`** — P1  

### Gate
- pan/zoom/rotate/focus prevedibili;
- nessuna selezione modificata dalla camera;
- no hidden-info query;
- packaged smoke;
- non regressione della camera già implementata.

Se questi comportamenti sono già coperti e testati, **aggiorna issue/docs esistenti e non crearne di nuove**.

---

## v0.2 — Struttura e finestre

Focus: map interactions, Reaction UX, interaction context.

### Candidate issues

6. **`[Camera] Integrate contextual RMB, map interactions and transition focus without camera ownership leaks`** — P1  
7. **`[Camera] Add ability-aware framing for line, AoE, cone, dash and map interaction previews`** — P1  
8. **`[Camera] Add reaction focus contract and PreviousCameraState return path`** — P1  
9. **`[Camera QA] Validate camera behavior across doors, cover windows and interaction graph changes`** — P1  

### Gate
- ability selection cambia overlay, non intel;
- map interactions leggibili;
- reaction focus pronto senza Camera Director completo;
- cambio struttura/porta non crea selection/focus bug.

---

## v0.3 — Informazione

Focus: TeamKnowledge, Last Known, Noise, stealth/detection, anti-oracle.

### Candidate issues

10. **`[Camera][Knowledge] Add authorized vertical unit markers for above/below layers`** — P0/P1  
11. **`[Camera][Knowledge] Add Last Known, acoustic and uncertain vertical marker grammar`** — P1  
12. **`[Camera][Privacy] Prevent targeting and camera focus from acting as hidden-state oracle`** — P0  
13. **`[Camera][Knowledge] Bind camera markers and focus exclusively to sanitized TeamKnowledge view models`** — P0  
14. **`[Camera QA] Add hidden-enemy, acoustic-area and last-known no-leak scenarios`** — P0  

### Gate
- visible / other-layer / last-known / acoustic / unknown chiaramente distinti;
- skill base non rivela piani o target nascosti;
- nessun hover/validity/focus oracle;
- Knowledge debug può spiegare da quale dato autorizzato nasce un marker.

---

## v0.4 — Operations / multilayer strategic scale

Questa è la release principale della camera avanzata.

### Candidate issues

15. **`[Camera][Multilayer] Add ActiveLayer-driven selection and explicit cross-layer focus policy`** — P0  
16. **`[Camera][Multilayer] Add local roof, wall, bridge and tunnel cutaway/occlusion system`** — P0/P1  
17. **`[Camera][Multilayer] Add safety collision without SpringArm zoom popping`** — P1  
18. **`[Camera][Strategic] Add zoom-triggered Strategic View with hysteresis`** — P0  
19. **`[Camera][Strategic] Add semantic multilayer LOD and presentation-only layer separation`** — P0/P1  
20. **`[Camera][Strategic] Add strategic unit tokens, simplified team paths, transitions and hazard overlays`** — P1  
21. **`[Camera][Strategic] Add M-toggle Strategic Overview with CameraState save/restore`** — P1  
22. **`[Camera][Targeting] Add knowledge-safe cross-layer targeting without exposing hidden geometry`** — P0  
23. **`[Camera Lab] Build multilayer Camera Lab with street, bridge, roof, interior and tunnel fixtures`** — P0/P1  
24. **`[Camera QA] Add multilayer selection, cutaway, Strategic hysteresis and cross-layer targeting tests`** — P0  

### Gate
- player capisce sempre quale layer è attivo;
- Tactical resta single-layer salvo marker;
- Strategic visualizza più layer senza diventare wallhack;
- no raycast ambiguity;
- no occlusion flicker significativo;
- cross-layer targeting è leggibile e knowledge-safe;
- Camera Lab copre vertical stack reale.

---

## v0.5 — Online Foundation

Focus: client view state, privacy, reconnect, packaged online.

### Candidate issues

25. **`[Camera][Network] Make camera and strategic overlays consume only owner/public/team-sanitized online DTOs`** — P0  
26. **`[Camera][Network] Add packaged two-team camera marker privacy canary coverage`** — P0  
27. **`[Camera][Reconnect] Restore local CameraState safely after reconnect without replicating camera authority`** — P1  
28. **`[Camera][Network] Validate team-only planning markers and strategic overlays under preview sequencing`** — P1  

### Gate
- camera non legge CanonicalIntentStore;
- nessun marker/path/target avversario privato ricevuto dal client;
- reconnect non altera stato competitivo;
- packaged network canary verde.

---

## v0.6 — Ability Runtime

Focus: ability metadata/runtime integration; GAS/support layer NON diventa camera authority.

### Candidate issues

29. **`[Camera][Abilities] Drive ability framing from canonical targeting metadata instead of ability-specific camera branches`** — P0/P1  
30. **`[Camera][Abilities] Preserve knowledge-safe targeting while migrating ability runtime integrations`** — P0  
31. **`[Camera][Abilities] Add camera regression scenarios for migrated roster abilities and special movement`** — P1  

### Gate
- nessun `if HeroId`/camera hard-coded per skill;
- line/AoE/cone/dash/map interaction framing deriva dai contratti canonici;
- resolver/targeting decide legalità, camera presenta;
- GAS/montage non decide focus competitivo o hit.

---

## v0.7 — Competitive Alpha

Focus: Resolution Camera Director, Fast Reaction, replay/spectator foundations.

### Candidate issues

32. **`[Camera Director] Add TurnLog-driven event grouping and conservative resolution reframing`** — P0/P1  
33. **`[Camera Director] Add off-screen event markers and manual-input suppression policy`** — P1  
34. **`[Camera Director] Add Fast Reaction hard focus with real-time recovery from accelerated playback`** — P0  
35. **`[Camera][Replay] Decouple camera command sources for player, replay and spectator modes`** — P0/P1  
36. **`[Camera][Replay] Add stable CameraState bookmarks and event focus from Stable IDs/FRTCellId`** — P1  

### Gate
- camera non “telecronaca impazzita”;
- normal Move non genera continui cut;
- simultaneous event groups restano logicamente simultanei;
- reaction decision leggibile immediatamente;
- manual input vince sul Director;
- replay può usare la stessa camera core.

---

## v0.8 — Beta / Balance

Focus: replay UX, spectator, accessibility, stress/performance.

### Candidate issues

37. **`[Camera][Spectator] Add spectator camera modes with explicit information policy boundaries`** — P1  
38. **`[Camera][Replay] Add manual and automatic replay camera modes with timeline event focus`** — P1  
39. **`[Camera][Accessibility] Add Reduce Camera Motion, shake levels and camera comfort settings`** — P0/P1  
40. **`[Camera][Performance] Profile Strategic View, marker stacks, cutaway and overlay density under stress`** — P0  
41. **`[Camera][Settings] Persist remappable input, pan, rotation and zoom preferences`** — P1  
42. **`[Camera QA] Run 4v4/large-map readability and camera comfort playtest matrix`** — P1  

### Gate
- spectator non vede dati non autorizzati;
- replay/manual/auto usano stesso CameraState contract;
- accessibility pass;
- 60 FPS client target nel corpus Camera/Operations;
- marker/overlay leggibili in stress.

---

## v0.9 — Release Candidate

Niente nuovi framework camera.

### Candidate issues

43. **`[Camera RC] Lock camera input, focus, Strategic View and multilayer presentation contracts`** — P0  
44. **`[Camera RC] Finalize Camera Lab packaged regression matrix`** — P0  
45. **`[Camera RC] Complete privacy, accessibility, localization and settings audit for camera UI`** — P0/P1  
46. **`[Camera RC] Certify cutaway, Strategic LOD and Camera Director performance budgets`** — P0  
47. **`[Camera RC] Freeze camera tuning defaults and document remaining platform-specific overrides`** — P1  

### Gate
- nessuna nuova architettura;
- zero camera/privacy blocker;
- zero layer-selection ambiguity blocker;
- performance e comfort entro budget;
- settings/documentation frozen.

---

## v1.0 — Launch certification

v1.0 non introduce nuove camera feature core.

### Candidate issues

48. **`[Camera v1.0] Run launch packaged smoke matrix for tactical, strategic, replay and spectator camera modes`** — P0  
49. **`[Camera v1.0] Certify camera privacy and no-oracle behavior in production network flows`** — P0  
50. **`[Camera v1.0] Sign off Camera Lab, replay/spectator compatibility and accessibility evidence`** — P0  
51. **`[Camera v1.0] Publish final camera controls, strategic view and multilayer player documentation`** — P1  

### Gate
Una partita completa può essere:
- pianificata;
- risolta;
- osservata;
- rivista;
- spettata secondo policy;
- navigata su mappe multilivello;

senza:
- leak;
- selection ambiguity;
- camera popping grave;
- replay divergence causata dalla presentation;
- dipendenze dall'Editor;
- regressioni packaged.

---

# 6. Dipendenze principali

```text
Tactical Camera baseline
    ↓
Planning / Ability Context
    ↓
TeamKnowledge + Perception
    ↓
Multilayer + Strategic View
    ↓
Online sanitized DTO boundary
    ↓
Ability runtime integration
    ↓
TurnLog Camera Director
    ↓
Replay / Spectator
    ↓
Accessibility + Performance
    ↓
RC Certification
    ↓
v1.0 Launch
```

Cross-feature dependencies da collegare, non duplicare:

```text
Map multilayer / transitions
LOS / Targeting / Trajectory
TeamKnowledge / Detection / Memory / Noise
Planning UI / Action Ghosts / AoE Ghost
Reaction Opportunity / Fast Decision
TurnLog / Replay
Networking / Privacy
Accessibility
Scenario Harness
Map/Scenario authoring
```

---

# 7. Scenario Map — candidate coverage

Prima cerca scenari equivalenti.

Proposte:

```text
CAMERA-BASIC-PAN-ZOOM-ROTATE
CAMERA-CURSOR-ZOOM-ANCHOR
CAMERA-SOFT-BOUNDS
CAMERA-FOCUS-PRESERVES-ORIENTATION
CAMERA-ABILITY-LINE-FRAMING
CAMERA-ABILITY-AOE-FRAMING
CAMERA-ABILITY-DASH-FRAMING

CAMERA-LAYER-ACTIVE-SELECTION
CAMERA-LAYER-MARKER-ABOVE-BELOW
CAMERA-LAYER-MARKER-STACK
CAMERA-LAST-KNOWN-MARKER
CAMERA-ACOUSTIC-AREA-MARKER
CAMERA-HIDDEN-ENEMY-NO-ORACLE

CAMERA-CUTAWAY-ROOF
CAMERA-OCCLUSION-WALL
CAMERA-OCCLUSION-BRIDGE
CAMERA-TUNNEL-CUTAWAY
CAMERA-SAFETY-COLLISION

CAMERA-STRATEGIC-ENTER-EXIT-HYSTERESIS
CAMERA-STRATEGIC-MULTILAYER-LOD
CAMERA-STRATEGIC-ACTIVE-LAYER-SWITCH
CAMERA-STRATEGIC-OVERVIEW-RETURN
CAMERA-CROSS-LAYER-TARGETING-KNOWLEDGE-SAFE

CAMERA-REACTION-FOCUS
CAMERA-DIRECTOR-GROUPED-EVENTS
CAMERA-DIRECTOR-MANUAL-OVERRIDE
CAMERA-DIRECTOR-SIMULTANEOUS-DISTANT-EVENTS

CAMERA-REPLAY-MANUAL
CAMERA-REPLAY-AUTO
CAMERA-SPECTATOR-INFORMATION-POLICY

CAMERA-REDUCE-MOTION
CAMERA-STRESS-MULTILAYER
CAMERA-PACKAGED-SMOKE
CAMERA-NETWORK-PRIVACY-CANARY
```

Usare ID/convention reali del repository, non questi nomi se incompatibili.

---

# 8. Camera Lab / Editor Map

Creare o consolidare una mappa laboratorio, preferenza:

```text
L_CameraLab
```

NON duplicarla se esiste già una camera test map.

Aree richieste:

```text
A — Plain Field
    pan / zoom / yaw / pitch / snap / bounds

B — Urban Occlusion
    walls / roofs / props / fade / cutaway

C — Vertical Stack
    roof + floor + street + tunnel

D — Bridge
    over/under units + cross-layer targeting

E — Knowledge
    visible / other-layer / last-known / noise-only / unknown

F — Ability Framing
    line / circle / cone / dash / map interaction

G — Resolution Director
    grouped events / distant simultaneous / KO / objective / reaction

H — Stress
    many markers / paths / AoE / hazards / strategic multilayer
```

Editor sessions/manual checks devono includere almeno:

```text
camera feel at 1080p
yaw/pitch ergonomics
60° vs 90° snap A/B test
Strategic threshold tuning
Strategic layer separation tuning
cutaway readability
cover still readable while faded
marker stack readability
colorblind-safe vertical intel
reaction focus readability
motion-reduction comfort
4v4/large-map stress
```

I seguenti restano volutamente PLAYTEST/TUNING fino a evidenza:

```text
canonical snap = 60° vs 90°
Strategic enter/exit thresholds
Strategic presentation layer separation
exact pan/zoom/rotation default sensitivities
```

---

# 9. Test automatici minimi

Pure/automation dove possibile:

```text
Pitch clamp
Yaw canonical snap selection
Zoom normalization
Strategic enter/exit hysteresis
Soft bounds
CameraState save/restore
Focus preserves yaw/pitch
Manual input cancels autofocus
ActiveLayer selection policy
No fallback to hidden/other layer on invalid current-layer cell
Marker knowledge classification mapping
Targeting preview cannot reveal unauthorized hidden target
```

Functional/Scenario:

```text
cutaway
cross-layer selection
Strategic transition
marker stack
reaction focus
Camera Director grouping
replay CameraState
spectator information policy
network privacy canary
packaged smoke
```

---

# 10. Debug / telemetry

Consolidare un debug camera equivalente a:

```text
rt.Camera.Debug
rt.Camera.DumpState
```

Non inventare questi comandi se il repository usa convenzioni diverse.

Debug minimo:

```text
ZoomNormalized
StrategicWeight
ActiveLayer
Yaw
Pitch
Pivot
Focus type/bounds
Camera Director state/reason
Soft bounds
Occluders + fade weight
Strategic thresholds
presentation layer offsets
authorized marker source/class
```

Profiling:

```text
camera update
occlusion queries
marker count
Strategic primitives
overlay draw
Slate/GPU
Director reframe count
```

---

# 11. Feature Registry

NON creare una costellazione di Feature ID se l'umbrella esistente può possedere il lavoro.

Preferenza:

```text
RT-FEAT-UI-TACTICAL-CAMERA
```

come umbrella, con issue/checkpoint per granularità.

Riusa:

```text
RT-FEAT-UI-LAYER-FILTER
RT-FEAT-UI-ACCESSIBILITY
RT-FEAT-UI-PLANNING
RT-FEAT-UI-AOE-GHOST
RT-FEAT-UI-FAST-DECISION
RT-FEAT-PERCEPTION-TEAM-KNOWLEDGE
RT-FEAT-PERCEPTION-MEMORY
RT-FEAT-PERCEPTION-SOUND-OVERLAY
```

Crea una nuova Feature tipo `STRATEGIC-CAMERA` / `CAMERA-DIRECTOR` SOLO se lo schema/ownership reale richiede una capability separata.

Per ogni Feature aggiornata:

```text
target release
owner Epic
issue links
dependencies
scenario coverage
status
Wiki/spec refs
DoD
```

---

# 12. Documenti/tracker da aggiornare

Aggiornare solo owner reali.

Probabili:

```text
docs/roadmap/roadmap-post-v0.1.md
docs/roadmap/roadmap-v0.1.md                  # solo se camera baseline v0.1 realmente impattata
docs/roadmap/roadmap-checkpoint.md
docs/roadmap/feature-registry.yaml
docs/roadmap/execution-graph.yaml
docs/roadmap/editor-sessions.yaml

docs/technical/scenario-map.md
docs/technical/test-manuali-pie.md
docs/technical/progettazione-hud.md
docs/technical/brief-planning-visuale.md
owner camera spec, se esiste
owner multilayer/perception/replay specs

docs/decisions/RT_PDR_00_Decision_Log.md
docs/OPEN_DECISIONS.md
docs/DOC_CONFLICT_MATRIX.md

Wiki:
Camera / Controls
Strategic View
Multilayer
Planning / Targeting
Perception / Last Known / Noise
Replay
Spectator
Accessibility
```

Viste generate:

```text
feature-registry.md/json
roadmap.shortlist
featuremap.shortlist
scenariomap.shortlist
editormap.shortlist
milestonemap.shortlist
project-graph.json
Control Center data
```

NON editarle a mano.

---

# 13. Decision Log — decisioni da verificare/registrare

Se non già presenti, consolidare decisioni equivalenti a:

1. **Free yaw + constrained manual pitch; Q/E snap only as explicit keyboard shortcut.**
2. **RMB remains contextual gameplay input; camera orbit uses Alt+MMB or current mapped equivalent.**
3. **Strategic View is zoom-triggered semantic multilayer LOD of the same camera, not a second world/map authority.**
4. **Tactical targeting never reveals layers/areas outside authorized perception; ability selection is not a perception oracle.**
5. **ActiveLayer governs Tactical selection; Strategic can explicitly switch ActiveLayer.**
6. **Cutaway/occlusion is presentation-only and separate from LOS.**
7. **Camera Director is TurnLog-driven, conservative and manually interruptible.**
8. **No traditional minimap is required for v0.1; Strategic Overview is the default global-navigation solution; minimap is playtest/future unless repository has newer locked decision.**
9. **Player/replay/spectator share the camera core; information policy remains external and sanitized.**

Usare l'ID allocator/Decision Log process reale. Non inventare `D-nnn`.

---

# 14. Issue body template

Per ogni issue creata/modificata:

```text
Why

Scope

Out of scope

Current decisions / owner sources

Existing implementation to reuse

Technical approach

Input / UI impact

Map / multilayer impact

Targeting / perception impact

Networking / privacy impact

Replay / spectator impact

Performance impact

Accessibility impact

Acceptance criteria

Automation tests

Scenario coverage

PIE / Editor evidence

Packaged evidence

Dependencies

Feature IDs

Roadmap release / Epic

Wiki / technical docs

Definition of Done
```

Per issue knowledge/network aggiungere obbligatoriamente:

```text
No hidden-state oracle
No unauthorized data replication
Packaged canary where applicable
```

---

# 15. Ordine di esecuzione richiesto a Claude

## A — Audit
Produrre:

```text
CAMERA ROADMAP RECONCILIATION

HEAD:
Roadmap release map:
Existing Camera Feature IDs:
Existing Camera Epic/Issues:
Existing Strategic/Minimap issues:
Existing Perception dependencies:
Existing Replay/Spectator dependencies:
Existing Camera test map:
Relevant open PRs:

Requested concept | Existing owner | Existing issue | Action
```

## B — Reconcile decisions
- registra solo decisioni realmente nuove;
- risolvi conflitto minimap/Strategic View;
- non cambiare release/Epic numbering senza audit.

## C — Update roadmap
- integra i checkpoint camera nelle release correnti;
- non creare nuova lane/milestone parallela.

## D — GitHub issue reconciliation
Per ogni titolo candidato:

```text
REUSED
UPDATED
CREATED
DEFERRED
NOT NEEDED
```

con motivazione.

## E — Feature Registry / graph
Aggiorna owner e links, poi rigenera tutte le viste.

## F — Scenario / Editor Map
Aggiungi solo coverage realmente mancante.

## G — Wiki / docs
Aggiorna la navigazione e gli owner pertinenti.

## H — Gates
Esegui validator/generator/docs checks e test disponibili.

---

# 16. Report finale obbligatorio

Restituisci:

```text
REFACTORTACTICS CAMERA ROADMAP v1.0 — CONSOLIDATION REPORT

Repository:
Worktree:
Branch:
Base HEAD:
Final HEAD:

ROADMAP LIVE
release | epic | camera scope | gate

DECISIONS
decision | status | owner/ID

FEATURES
Feature ID | old status | new status | release | Epic

ISSUES
issue | title | release | REUSED/UPDATED/CREATED/DEFERRED | dependencies

SCENARIOS
Scenario | feature | automated/manual | status

EDITOR SESSIONS
Task | verifies | release | status

DOCS
path | action

WIKI
page | action

GENERATED VIEWS
path | command/result

TESTS
command | PASS/FAIL/NOT RUN

PRIVACY / NO-ORACLE
check | result

PERFORMANCE
check | result

OPEN TUNING
60 vs 90 snap:
Strategic thresholds:
Strategic layer separation:
Sensitivity defaults:

CONFLICTS
...

NEXT EXECUTABLE ISSUE
# / title:
why:
dependencies:
```

---

# 17. Guardrail finali

```text
NO seconda camera authority
NO seconda Strategic Map world state
NO minimap/Strategic hidden-state access
NO hidden target via hover/validity oracle
NO raycast che decide accidentalmente il Layer
NO camera collision che altera continuamente zoom
NO Camera Director che modifica il TurnLog
NO Actor pointer come replay authority
NO automatic yaw rotation durante normal Resolution
NO ability-specific camera hard-code se targeting metadata basta
NO Feature ID explosion
NO issue duplicate
NO nuove release parallele
NO edit manuale di file generati
NO v1.0 camera feature nuova dopo RC freeze
```

---

# 18. Definition of Done camera v1.0

La camera è production-ready quando:

- pan/zoom/yaw/pitch/focus sono prevedibili;
- camera non modifica gameplay state;
- ActiveLayer è sempre leggibile e non ambiguo;
- Tactical/Strategic transition non flickera;
- Strategic multipiano non rivela intel extra;
- marker verticali distinguono confirmed / last-known / acoustic / unknown;
- target selection non è un oracle;
- cutaway rende il focus leggibile mantenendo la presenza tattica di cover/strutture;
- Resolution Director non combatte con il giocatore;
- Fast Reaction è immediatamente leggibile;
- replay e spectator riusano la camera core;
- spectator rispetta la propria Information Policy;
- Reduce Motion / shake / input settings funzionano;
- target 60 FPS client è rispettato nel corpus previsto;
- privacy canary packaged è verde;
- Camera Lab e scenario corpus sono verdi;
- documentazione, roadmap, Feature Registry, Scenario Map, Editor Map, Wiki e GitHub raccontano lo stesso stato;
- v1.0 non dipende dall'Editor per il comportamento camera runtime.
