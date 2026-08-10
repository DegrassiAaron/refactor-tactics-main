> ## 🗄️ `HISTORICAL` — SORGENTE RECEPITO
>
> **Archiviato il 2026-08-10.** Era in `todo/consolidazione-chat-openai/`, untracked.
> Triage: [`consolidamento-chat-openai-triage-2026-08-09.md`](../../../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md) §«cluster UI / UX».
>
> **Esito diverso dagli altri cluster**: su piu' punti il repository era **piu' avanti della fonte**. Owner
> vivi: [`progettazione-hud.md`](../../../technical/progettazione-hud.md) e le epic E11/E20.
>
> ⚠️ **Non applicare** le 15 `RT-FEAT-UI-*` di §30: sei esistono, nove no, e ne omette due che esistono
> (`RT-FEAT-UI-ICON-LANGUAGE`, `RT-FEAT-UI-SCENARIO-BROWSER`).

# RefactorTactics — UI / UX Master Consolidation v0.1

**Data consolidamento:** 2026-08-09  
**Scope:** Tactical HUD, Planning, Action Ghosts, Ghost Timeline, Team Intent, Warning, Certainty, Reaction/Fast Decision, Map/Environment overlays, Perception, Resolution e Combat Log.  
**Stato:** master di consolidamento per cleanup; il repository/Feature Registry corrente resta la fonte operativa finale.

---

# 0. Principio

La UI deve rendere leggibile la simultaneità senza diventare autorità del gameplay.

```text
Authoritative State / Sanitized Team State / TurnLog
        ↓
ViewModel / DTO
        ↓
HUD / World Overlay / Ghost / Combat Log
```

NON:

```text
Widget
 -> decide path legality
 -> decide hit
 -> decide reaction
 -> read hidden enemy planning
```

Vincoli:
- server authoritative;
- intenti nemici privati mai replicati;
- ally intent team-only;
- warning solo da stato proprio/pubblico/team knowledge lecito;
- preview presentation-only;
- Enhanced Input;
- CommonUI solo dopo proof of concept;
- overlay pesanti non aggiornati indiscriminatamente ogni Tick.

---

# 1. Grammatica delle fasi

La UI usa:

```text
PLANNING
 -> PREP
 -> DASH
 -> BLAST
 -> MOVE
```

Il normale `Move` è l'ultima fase volontaria.

La UI non deve mai suggerire una coda arbitraria:

```text
Attack -> Move -> Attack -> Dash
```

Fast Action/Fast Reaction sono rami/Decision Boundary, non una quinta macrofase lineare.

---

# 2. Layer HUD

Organizzare almeno:

1. Persistent HUD
2. Planning HUD
3. World/Map overlays
4. Action Ghosts / Ghost Timeline
5. Ally Intent / Coordination
6. Warning
7. Certainty / Information
8. Reaction / Fast Decision
9. Time Bank
10. Map Interaction UI
11. Objective HUD
12. Perception / Sound
13. Resolution Timeline
14. Combat Log / Outcome Explanation
15. Notification Feed
16. Debug HUD

Non costruire un `WBP_TacticalHUD` monolitico con logica di simulazione.

---

# 3. Persistent HUD

Sempre o quasi sempre visibile:

## Turn / Phase
- Turn number
- current phase
- match state

## Timer
- planning countdown;
- decision window countdown quando applicabile.

## Objective summary
- name;
- score;
- progress;
- contested;
- recent change;
- countdown se reale.

## Selected Unit
- portrait;
- name;
- role;
- HP;
- shield/resource;
- relevant status;
- current action cost/state.

Non trasformarlo in character sheet RPG.

---

# 4. Action Dock

Separare visivamente, NON economicamente:

## Universal Actions
- Move
- Wait
- Guard
- Brace
- Activate
- Interact
- Overwatch

## Hero Kit
- Ability 1
- Ability 2
- Ability 3
- Ability 4

La UI non deve far pensare che il giocatore scelga:
```text
Universal Action + Hero Ability
```
se l'action economy non lo consente.

Ogni slot supporta:
```text
Available
Hover
Selected
Planned
Cooldown
Unavailable
Invalid
Warning
```

Layer dello slot:
1. background/frame
2. icon
3. shortcut
4. cost
5. cooldown/charge
6. state overlay
7. warning
8. planned marker

Testo/numeri non vanno baked dentro PNG.

---

# 5. Action Details

Per l'azione selezionata mostrare solo ciò che serve:

- name/icon;
- action type;
- phase;
- cost;
- cooldown/charges;
- targeting;
- range/AoE;
- requirements;
- moving-target policy;
- environment effect;
- warning;
- certainty where relevant.

Il pannello non deve replicare tutta la Data Asset.

---

# 6. Ghost Timeline

Componente identitario:

```text
[ PREP ] [ DASH ] [ BLAST ] [ MOVE ]
```

Ogni fase può essere:
```text
Empty
Populated
Selected
Inactive
Resolved
```

Il Move ghost rappresenta lo stato finale della normale Move.

La Reaction si mostra come ramo:

```text
PREP — DASH — BLAST — MOVE
              └── ⚡ REACTION
```

Delayed/Predictive branch, se/quanto in scope:
```text
⏱ Delayed @ named boundary
```

---

# 7. Scrubbing

Il giocatore deve poter selezionare una fase.

Esempio BLAST:
- Blast ghost prominente;
- attack origin;
- line/cone/AoE;
- target;
- facing;
- cover relevant;
- warning Blast;
- altri ghost attenuati.

La camera resta tattica/isometrica.

Non passare automaticamente a una modalità top-down separata.

---

# 8. Action Ghost

Un Action Ghost è idealmente:
- copia 3D semitrasparente del personaggio;
- pose/loop breve significativo;
- posizione prevista;
- facing;
- weapon/cast orientation;
- attack origin;
- ground anchor.

Il marker 2D è supporto, non la feature principale.

Planning:
- pose statiche;
- anticipation;
- landing;
- aiming;
- shield/cast;
- endpoint Dash.

Resolution:
- animazioni complete.

Il Ghost non calcola gli esiti.

---

# 9. Certainty grammar

Tre stati obbligatori:

## Confermato

Da:
- stato pubblico corrente;
- regola deterministica già nota;
- legalità locale.

Visuale:
```text
solid line
full opacity
no ?
```

## Previsto

Include:
- proprio piano;
- intenti sanitizzati della squadra;
- dipendenze team.

Visuale:
```text
dashed
team marker
slightly attenuated
```

## Incerto

Dipende da:
- enemy action;
- future collision;
- moving target;
- reaction;
- partial knowledge;
- future environment state.

Visuale:
```text
fade / gradient / ?
```

La preview non è una promessa.

---

# 10. Privacy Ghost

Permesso:
- local unit intent;
- ally sanitized intent;
- public state;
- observed/team knowledge.

Vietato:
- enemy hidden path;
- enemy destination;
- enemy target;
- enemy AoE;
- private enemy AbilityId;
- future enemy reaction opportunities.

Nessun enemy ghost derivato da `CanonicalIntentStore`.

“Nascondere il widget” non è sicurezza.

---

# 11. Facing UI

Facing è gameplay state.

La selezione va mostrata nel mondo.

## Stationary
Mostrare le direzioni legali, fino alle 6 hex directions se la regola corrente lo consente.

## Budget Move
Mostrare solo le direzioni finali legali.

## Linear Dash/Charge/Leap
Facing può derivare dalla direzione: evitare input ridondante.

## Overwatch
Il cono deriva dal Facing.

NON creare:
```text
Facing = NE
OverwatchDirection = E
```
come due controlli indipendenti se il canone usa un solo facing.

---

# 12. Movement Planner

Visualizzare:
- reachable cells;
- selected path;
- destination;
- move cost;
- surface/transition cost when relevant;
- final facing;
- invalid segments;
- uncertainty boundary;
- hazard/known reaction warning.

Il path autorevole viene dal grafo.

La UI può offrire preferenze, ma non cambiare la legalità.

---

# 13. Ability Preview

Supportare forme:
- line;
- cone;
- arc;
- cell;
- unit;
- circle;
- trajectory;
- Overwatch zone;
- displacement;
- environment propagation;
- structure/edge modification;
- map interaction.

Separare:
```text
Path
LOS
Targeting
Trajectory
```

Non usare un unico “preview ray” per tutto.

---

# 14. Team Intent

Pannello compatto/collassabile.

Per ally:
- unit;
- path/destination;
- action/ability;
- target;
- AoE;
- facing;
- label;
- plan state / Ready quando reale.

Dettaglio completo su hover/focus.

World overlay:
- ally Action Ghosts;
- ally path;
- ally AoE;
- labels/ping.

Quando entra networking:
- team-only DTO;
- stale sequence handling;
- rate limit;
- privacy tests.

---

# 15. Ready / Confirm

Nel flusso offline/local corrente usare label coerenti con lo stato reale, ad esempio:
```text
CONFIRM PLAN
LOCK IN
```

Non mostrare:
```text
TEAM READY 1/2
```
se il modello team-ready non è realmente implementato nel build corrente.

Quando entra F1/network:
- individual Ready;
- team status;
- cancelable countdown se previsto;
- stale plan blocker.

---

# 16. Warning

Warning derivano solo da dati leciti.

Categorie:
- ally destination collision;
- ally path conflict;
- friendly fire;
- insufficient resource;
- cooldown/charge;
- stale GraphRevision;
- invalid target;
- uncommitted latest draft;
- uncertain target;
- known/legit reaction risk;
- environment/hazard.

Severity:
```text
Info
Warning
Block/Error
```

Nessun warning può usare segretamente enemy future intent.

---

# 17. Fast Reaction UI

Prompt minimale.

Overwatch:

```text
⚡ OVERWATCH
Target enters controlled area
2.4 s

[FIRE] [HOLD]
```

Regole:
- countdown immediato;
- poche opzioni;
- non coprire la scena;
- timeout = HOLD per baseline Overwatch;
- nessun `Opportunity 1/3`;
- nessuna anticipazione di trigger futuri.

Simultaneous targets:
```text
[FIRE A]
[FIRE B]
[HOLD]
```
in una sola window.

---

# 18. Time Bank UI

Il Time Bank appartiene al Reaction/Fast Decision UX.

Mostrare:
- bank residuo;
- consumo corrente;
- warning low bank;
- timeout/default behavior.

NON fissare nella UI un valore definitivo finché la regola Time Bank resta OPEN.

Il widget consuma stato dal sistema Reaction/Match Timing.

---

# 19. Delayed / Predictive UI

Se la feature è in scope:

```text
⏱ Delayed
Target: fixed
Boundary: EndMove / named boundary
State: armed
Outcome: uncertain
```

Distinguere:
```text
⚡ live decision
⏱ precommitted future resolution
```

Non permettere al giocatore di scegliere liberamente un nuovo target dopo aver visto il futuro nemico.

---

# 20. Map Interaction UI

Per elemento:
- tactical label;
- state;
- legal verbs;
- reason unavailable;
- capability requirement;
- target linkage se conosciuta;
- affected systems in tooltip/inspect solo quando utile.

Esempio:
```text
D1 — Laboratory Door
Closed
[OPEN]
[FORCE] unavailable: requires Force
```

Source/target linkage:
```text
S1 -> D1
```
solo se autorizzato dal Team Knowledge.

---

# 21. Terrain / Environment Overlay

Layer/filtri possibili:
```text
Movement
Terrain
Cover
Threat
Sound
Vision
Interaction
```

Non mostrare tutto contemporaneamente.

Focus e filter sono obbligatori per evitare clutter.

Per environment:
- Wet/Water;
- Conductive;
- Fire/Burning;
- Ice/Rough se in scope;
- Smoke;
- hazard;
- cover;
- door/bridge state.

La UI deve distinguere:
```text
current actual
predicted own/team effect
uncertain future effect
```

---

# 22. Perception / Information HUD

Modello:
```text
Visible
Detected
Identified
Last Known
Acoustic Contact
Unknown
```

Sound overlay può mostrare:
- direction;
- uncertainty area;
- intensity category;
- age;
- ambient mask;
- confidence.

Non deve mostrare source/identity precisa se il Team Knowledge non la conosce.

Memory marker:
- si attenua nel tempo;
- non segue segretamente il target.

---

# 23. Resolution HUD

Durante Resolution ridurre il planning chrome.

Mostrare:
- current phase;
- current event;
- relevant units;
- objective changes;
- reaction window;
- resolution timeline;
- compact event feed.

Gli Action Ghost pianificati possono diventare meno prominenti o sparire in favore del playback reale.

---

# 24. Combat Log / Outcome Explanation

Il Combat Log usa TurnLog/reason codes.

Una riga deve poter spiegare:
```text
Arc Lance -> 18 damage
base 20
cover -4
wet chain +2
```
solo quando questi modificatori sono realmente presenti nei dati evento.

Per failure:
- target moved;
- blocked;
- interrupted;
- invalid state at impact;
- resource/cooldown;
- LOS changed;
- door remained closed.

La UI non ricalcola il perché.

---

# 25. Notification Feed

Usare per:
- objective changes;
- status applied/expired;
- map element changed;
- reaction consumed/expired;
- important perception event.

Non duplicare ogni TurnLog event in toast.

Serve una priority/filter policy.

---

# 26. Camera

PC-first:
- pan;
- zoom;
- rotate;
- focus selected unit/event/ping;
- layer filter.

Rotazione libera solo se non degrada leggibilità; step rotation resta baseline sicura.

Camera non modifica gameplay state.

---

# 27. Accessibility

- niente informazione solo-colore;
- icon + pattern + text;
- UI scale;
- readable 1080p;
- remapping;
- reduce camera shake/motion;
- grayscale check;
- icon fallback/tooltip;
- avoid tiny cooldown text.

Per Reaction:
- countdown visibile;
- high-contrast urgent state;
- input simple;
- no precision mouse mini-game.

---

# 28. Performance

Target:
- preview 8–12 Hz quando appropriato;
- pooling decal/lines/ghost assets;
- no expensive full overlay every Tick;
- virtualized Combat Log/lists;
- avoid costly UMG bindings;
- profile Slate/GPU.

Ghost rendering deve degradare bene in 4v4 stress.

---

# 29. Debug HUD

Development-only:
- CellId/Layer;
- GraphRevision;
- path cost;
- LOS;
- targeting reason;
- snapshot/hash;
- TurnEvent;
- reaction state/opportunity;
- Team Knowledge class;
- noise received;
- map element state;
- interaction capability.

Debug UI non è source of truth.

---

# 30. Feature Registry — stato recuperato

Feature già esistenti/da riusare:

```text
RT-FEAT-UI-TACTICAL-CAMERA
RT-FEAT-UI-CELL-SELECTION
RT-FEAT-UI-PLANNING
RT-FEAT-UI-ACTION-GHOSTS
RT-FEAT-UI-PATH-GHOST
RT-FEAT-UI-AOE-GHOST
RT-FEAT-UI-ALLY-INTENTS
RT-FEAT-UI-CERTAINTY
RT-FEAT-UI-WARNINGS
RT-FEAT-UI-COMBAT-LOG
RT-FEAT-UI-FAST-DECISION
RT-FEAT-UI-FACING-PREVIEW
RT-FEAT-UI-PING
RT-FEAT-UI-DRAWING
RT-FEAT-UI-LAYER-FILTER
```

Stati recuperati:
- Tactical Camera: IMPLEMENTED;
- Cell Selection: IMPLEMENTED;
- Planning HUD: IMPLEMENTED_PARTIAL;
- Action Ghosts: PARTIAL/SPECIFIED;
- Path Ghost: IMPLEMENTED;
- AoE Ghost: PARTIAL;
- Ally Intents: PARTIAL/DESIGNED;
- Certainty: SPECIFIED/PARTIAL;
- Warnings: PARTIAL;
- Combat Log: IMPLEMENTED_PARTIAL;
- Fast Decision: DEFERRED_E14;
- Facing Preview: SPECIFIED;
- Ping: DESIGNED/future network;
- Drawing: DESIGNED/future.

Non sovrascrivere questi status con supposizioni.

---

# 31. Roadmap UI

## F0
- root HUD;
- turn/phase;
- planning timer;
- selected unit;
- basic action bar;
- reachable cells;
- path;
- destination ghost;
- confirm/ready locale;
- basic TurnLog view;
- debug HUD.

## F1
- ally intent;
- team Ready;
- sequence handling;
- team-only label/ping baseline;
- privacy zero-leak.

## F2
- full Action Dock;
- ability detail;
- target/AoE/trajectory;
- cooldown/resource;
- advanced ghosts.

## F3
- layer filter;
- cover/terrain;
- map interaction;
- door/bridge/tunnel/elevator;
- environment overlay.

## F4
- objective;
- full warning;
- certainty;
- combat log/explanation;
- resolution timeline;
- notification;
- accessibility/playtest pass.

## Reaction workstream
- Fast Decision;
- Overwatch;
- Time Bank;
- simultaneous target;
- expired/stale opportunity.

## Perception workstream
- Team Knowledge;
- Last Known;
- Sound Overlay;
- uncertainty/memory.

Usare milestone reali del repository; non creare numerazione parallela.

---

# 32. Scenario Registry

## HUD-001 — Basic Planning HUD
Turn/phase/timer/unit/action/confirm.

## HUD-002 — Movement Ghost
Path, destination, facing, scrub.

## HUD-003 — Blast Ghost
Origin, line/AoE, certainty.

## HUD-004 — Ally Intent
Two-unit combo, no clutter.

## HUD-005 — Warning Collision
Ally path/destination conflict.

## HUD-006 — Friendly Fire
AoE warning from team state only.

## HUD-007 — Stale Graph
Door/map change invalidates preview.

## HUD-008 — Certainty Grammar
Same scene with Confirmed/Predicted/Uncertain.

## HUD-009 — Reaction Prompt
FIRE/HOLD 3s, timeout HOLD.

## HUD-010 — Simultaneous Reaction Targets
One prompt, multiple FIRE targets.

## HUD-011 — Time Bank Low
Visual warning without changing logic.

## HUD-012 — Map Interaction
Door + controller + legal verb.

## HUD-013 — Sound Contact
Acoustic contact without exact enemy info.

## HUD-014 — Combat Explanation
Reason-code driven outcome.

## HUD-015 — 4v4 Clutter Stress
8 units, ghost/intent/overlay readability.

---

# 33. Test

Automation/functional:
- view model doesn't access hidden enemy intent;
- stale DTO ignored;
- certainty style maps to data classification;
- warning source classification valid;
- path ghost update throttled;
- simultaneous reaction UI single window;
- timeout default;
- ally intent sequence;
- map interaction unknown link hidden;
- sound marker lacks unauthorized fields;
- Combat Log uses TurnLog reason.

Packaged privacy:
- canary enemy planning absent;
- enemy ghost absent;
- hidden controller links absent;
- acoustic source fields sanitized.

Performance:
- overlay count;
- Slate timing;
- 4v4 ghost load;
- log virtualization.

---

# 34. Conflitti / cleanup

## UI-READY-01
Vecchi mockup possono mostrare `TEAM READY 1/2` anche in build local/offline.

Azione:
mostrare stato reale; non simulare networking inesistente.

## UI-ACTION-TAXONOMY-01
Alcuni documenti UI vecchi elencano azioni universali incomplete.

Azione:
allineare al Common Actions Master / Decision Log corrente.

## UI-REACTION-01
Reaction non deve comparire come quinta fase.

Azione:
rappresentazione branch.

## UI-GHOST-01
Ghost 2D marker-only è troppo riduttivo rispetto alla direzione corrente.

Azione:
3D ghost/pose come feature primaria, marker 2D di supporto.

## UI-PERCEPTION-01
Fog/Noise UI non deve mostrare più informazione di Team Knowledge.

Azione:
DTO/view model sanitized.

---

# 35. Chat cleanup

Dopo integrazione canonica diventano candidate ad Archive/Delete:

```text
Action Ghosts e Pianificazione
Abbiamo diverse informazioni da fornire attraverso l'HUD
```

Documenti specialistici:
```text
RefactorTactics_HUD_Consolidation_Claude.md
RefactorTactics_ActionGhosts_Phases_FastReactions_Claude.md
progettazione-hud.md
```

possono essere archiviati dopo che il master e la Wiki/Feature Registry ne hanno assorbito le decisioni.

---

# 36. Epic suggerite

## Tactical HUD Core
- root;
- phase/timer;
- selected unit;
- action dock;
- objective;
- confirm/ready.

## Planning Visualization
- movement;
- target/AoE;
- Ghost Timeline;
- Action Ghost;
- facing;
- scrubbing.

## Team Coordination HUD
- ally intent;
- label;
- Ready;
- ping;
- drawing.

## Tactical Warning & Certainty
- warning model;
- Confirmed/Predicted/Uncertain;
- privacy validation.

## Fast Decision UX
- generic decision window;
- Overwatch;
- Time Bank;
- simultaneous target;
- timeout.

## Map & Environment HUD
- cell inspector;
- interaction;
- terrain;
- cover;
- linked devices.

## Perception & Information HUD
- visible/detected/last known;
- sound;
- uncertainty;
- memory.

## Resolution & Explainability
- timeline;
- current event;
- Combat Log;
- reason codes;
- notifications.

---

# 37. Exit criteria

UI/UX è consolidata quando:

1. esiste un unico owner della grammatica HUD;
2. Ghost Timeline usa PREP/DASH/BLAST/MOVE;
3. Reaction è branch, non fase;
4. Action Ghost è presentation-only;
5. enemy private planning non genera Ghost/Warning;
6. Certainty grammar è consistente;
7. Common Action taxonomy è aggiornata;
8. Map/Noise UI consuma knowledge sanitizzata;
9. Combat Log consuma TurnLog/reason;
10. Feature Registry status è allineato;
11. scenario HUD-* coprono planning/reaction/map/perception;
12. le chat HUD/Action Ghost possono uscire dal CORE.
