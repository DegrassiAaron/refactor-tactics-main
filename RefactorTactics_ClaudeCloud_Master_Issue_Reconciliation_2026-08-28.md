# RefactorTactics — Claude Cloud Master Handoff
## Consolidamento globale delle issue descritte nelle chat · CREATE / UPDATE / LINK / DEFER

**Data:** 2026-08-28  
**Repository:** `DegrassiAaron/refactor-tactics-main`  
**Engine canonico atteso:** Unreal Engine 5.8.1 — verificare sempre sul repository reale  
**Scopo:** trasformare in tracking GitHub coerente tutto il lavoro descritto nelle chat recenti di RefactorTactics, senza creare duplicati né una seconda roadmap.

> Questo file è un **input di riconciliazione per Claude Cloud**, non una nuova source of truth.
> Il repository, le decisioni correnti, le roadmap correnti e le issue live prevalgono sempre.

---

# 0. MISSIONE

Devi effettuare una passata di **issue management + roadmap reconciliation** sull'intero progetto RefactorTactics.

Obiettivi:

1. sincronizzare `origin/main`;
2. leggere source of truth, roadmap e issue correnti;
3. auditare tutti i domini elencati in questo documento;
4. cercare prima issue/Epic/checkpoint esistenti;
5. classificare ogni attività come:
   - `UPDATE_EXISTING`
   - `CREATE_NEW`
   - `LINK_ONLY`
   - `ALREADY_DONE`
   - `DEFER`
   - `DECISION_REQUIRED`
6. aggiornare issue esistenti quando possiedono già il lavoro;
7. creare issue solo per gap reali e senza owner;
8. collegare le issue alle release canoniche v0.1→v1.0;
9. aggiornare i riferimenti incrociati fra Epic/issue quando il vecchio owner è stale;
10. produrre alla fine un report verificabile con link GitHub.

**NON implementare il codice in questa passata**, salvo attività documentali strettamente necessarie al tracking.

---

# 1. REGOLE NON NEGOZIABILI

## 1.1 Search before create

Prima di creare una issue:

- cerca titolo, sinonimi e simboli nel repository;
- cerca issue OPEN e CLOSED;
- cerca Epic/checkpoint owner;
- cerca ADR/Decision/Open Decision;
- cerca PR recenti che possono avere già implementato la feature.

Una issue CLOSED non va riaperta per aggiungere nuovo scope: crea child/follow-up solo se serve davvero.

## 1.2 Nessuna seconda roadmap

Preservare la ladder canonica corrente:

| Release | Tema |
|---|---|
| v0.1 | Vertical Slice / gioco fondamentale end-to-end |
| v0.2 | Struttura e finestre / formato principale 3v3 |
| v0.3 | Informazione / perception, belief, prediction |
| v0.4 | Operations / scala e multilivello |
| v0.5 | Online Foundation |
| v0.6 | Ability Runtime / GAS senza authority |
| v0.7 | Competitive Alpha / Dedicated |
| v0.8 | Beta / Balance / batch |
| v0.9 | Release Candidate / freeze + hardening |
| v1.0 | Launch / produzione + ranked |

Non aggiungere release intermedie o date.

## 1.3 Governing principles

Preservare sempre:

- snapshot immutabile;
- resolution deterministica;
- stessa snapshot + rules + version + seed = stesso risultato;
- client propone, server valida/applica;
- planning avversario mai replicato ai client nemici;
- simulatore decide, animazioni/UI mostrano;
- dati discreti/interi/fixed-point per regole competitive;
- Stable ID/version/hash/validator;
- GAS non è authority del resolver;
- bot e HUD consumano TeamKnowledge autorizzata, non stato nemico completo.

## 1.4 Tracking rimosso

Verifica le decisioni recenti prima di usare vecchi file.

Non introdurre di nuovo automaticamente:

- Feature Registry se rimosso;
- `parallel-batch.yaml` se rimosso;
- vecchie shortlists generate;
- path storici di docs spostati.

Se le issue vecchie li citano, aggiungi rettifica datata invece di restaurare il sistema rimosso.

---

# 2. PREFLIGHT OBBLIGATORIO

```bash
git status
git branch --show-current
git fetch --all --prune
git rev-parse HEAD
git rev-parse origin/main
git log -20 --oneline --decorate origin/main
```

Leggere almeno, usando i path reali se sono cambiati:

```text
AGENTS.md
CLAUDE.md
README.md
docs/README.md
docs/CONTEXT_INDEX.md

docs/product/piano-canonico-mvp.md
docs/decisions/RT_PDR_00_Decision_Log.md
docs/decisions/adr-*.md
docs/OPEN_DECISIONS.md
docs/DOC_CONFLICT_MATRIX.md

docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-post-v0.1.md
docs/roadmap/roadmap-checkpoint.md
docs/roadmap/v0.1-definition-of-done.md

docs/gameplay/**
docs/technical/**
docs/balance/**

Source/RefactorTactics/**
Source/RefactorTacticsEditor/**
Source/RefactorTactics/Tests/**
Source/RefactorTactics/ScenarioHarness/**
Scenarios/**
Content/**
```

---

# 3. DOMINI DA CONSOLIDARE

Questa passata deve coprire **tutti** i domini sotto. Non fermarti al primo gruppo di issue.

---

# 4. HUD / SCREEN HUD / TACTICAL WORLD OVERLAY

## 4.1 Owner già emersi nelle chat

Auditare almeno:

- E11 / issue `#25` — HUD, log e debug;
- `#77`;
- `#78` — certainty / stati UI;
- `#79`;
- `#172` — Ghost Timeline / world tactical preview;
- `#173` — scrubbing/focus;
- `#613` — Screen HUD in UMG;
- `#705` — Pointer/interaction contract;
- eventuali issue nuove successive al 2026-08-28.

## 4.2 Workstream HUD da mappare

La roadmap discussa comprende:

```text
Panels
→ Buttons
→ Slots
→ Portraits
→ Bars
→ Icons
→ Planning HUD
→ Warning system
→ Combat Log
→ Confirmation / Ready
→ Fast Reaction UI
→ Last Known / acoustic uncertainty
→ accessibility / packaged
```

Non creare una nuova Epic HUD parallela se E11/E20/E25 possiedono già il lavoro.

## 4.3 Stati UI canonici

Preservare:

- `Confermato`
- `Previsto`
- `Incerto`

Se il repository ha anche uno stato `Invalid`, verificare owner e terminologia corrente prima di documentarlo come canonico.

## 4.4 Confine UMG / world overlay

Mantenere la separazione:

**UMG / Screen HUD**:
- turn/phase/timer;
- roster;
- selected unit;
- action dock;
- warning;
- combat log;
- Ready/confirmation;
- fast decision controls.

**Tactical World Overlay**:
- path;
- waypoint;
- Dash;
- AoE;
- friendly fire;
- facing;
- legal sectors;
- Overwatch cone/controlled area;
- target/placement anchors.

Se vecchie issue rimandano il legal Facing allo Screen HUD ma l'owner reale è Ghost/World Overlay, aggiungere cross-link/rettifica.

---

# 5. HUD ICON GRAMMAR / ICON CATALOG

## 5.1 Owner noti da verificare

- E20 — HUD Icon Language — `#217`;
- CP 20.1 — `#218`;
- CP 20.2 — `#219`;
- CP 20.3 — `#220`;
- E25 — Icon Language completo — verificare issue Epic corrente;
- checkpoint E25 storicamente associati a `#265–#269`, rimisurare prima di usare.

## 5.2 Decisioni delle chat da consolidare

La grammatica deve coprire almeno:

- actions;
- movement;
- reaction;
- perception;
- environment;
- objective;
- team coordination;
- warning;
- hero/faction;
- status;
- planning states.

Pipeline preferita:

```text
Semantic IconId
→ governed catalog / data asset
→ asset
→ widget/world consumer
```

Non hardcodare path asset nei consumer.

## 5.3 Visual grammar

Preservare le decisioni recenti sulle icone:

- linguaggio geometrico coerente con esagono centrale;
- sub-icon piccoli posizionati sui vertici/settori del master hex quando appropriato;
- non usare una moltitudine di colori piccoli per sostituire la semantica;
- colori fase possono essere un canale, non l'unico;
- niente dipendenza dal solo colore;
- leggibilità a dimensioni HUD reali;
- esportazione/naming UE coerenti.

Auditare anche il consolidamento icon grammar più recente e aggiornare Decision Log/docs/issue owner senza creare un secondo icon system.

---

# 6. FACING / PIVOT / DECLARED ROTATION

## 6.1 Canone

E16 è già la fondazione Facing: non creare una nuova Epic Facing.

ADR-0008 accettato:

| Hero | Move Pivot | Dash Pivot |
|---|---:|---:|
| Gadget | 2 | 2 |
| Phase | 2 | 3 |
| Riktor | 1 | 0 |
| Wraith | 3 | 3 |

Stationary Pivot = 3.

Durante il movimento:

```text
Facing = direzione dell'ultimo micro-step realmente completato
```

Pivot finale:

```text
dopo l'ultimo micro-step
non retroattivo
```

FAC-12 — costo MP del Pivot — resta OPEN salvo decisione successiva.

## 6.2 Divergenza runtime da verificare

Auditare:

```text
MoveEndPivotMaxSteps
DashEndPivotMaxSteps
LegalFacings
TryApplyDeclaredFacing
FacingFromPath
MovementStyleThisTurn
WalkedThisTurn
```

Se il runtime usa ancora il vecchio schema:

```text
Linear* → 1
Budget  → D,D±1
None    → 6
```

mentre ADR-0008 richiede cap per eroe, creare/aggiornare un owner esplicito.

Prima verificare:

- `#291` — declared rotation;
- E16 / `#175`;
- `#176` / `#177` storiche;
- issue recenti su ADR-0008.

Non riaprire E16.

## 6.3 Legal Facing UI

Auditare owner fra:

- `#291`;
- `#172`;
- `#613`;
- `#705`;
- eventuali issue successive.

Richiesto:

- insieme legale visibile;
- illegal non cliccabile / reject motivato;
- ghost orientato;
- legalità UI = legalità resolver;
- nessun duplicate facing table nel client.

---

# 7. END PLACEMENT / PLACEMENT SECTOR / COVER ANCHOR

## 7.1 Nuova proposta descritta nelle chat

Dopo il movimento, il giocatore sceglie **come si sistema nella cella finale**, tramite uno spicchio o gruppo di spicchi.

Vocabolario proposto:

- `Cell` = `FRTCellId`;
- `Facing` = orientamento;
- `PlacementSector` = posizione discreta intra-cella;
- `CoverAnchor` = anchor discreto associato a un riparo;
- `ResolvedCoverState` = risultato logico;
- `CoverPose` = presentation-only.

Questa parte è **PROPOSED** finché il repository non contiene una decisione accettata.

## 7.2 Vincoli

Non introdurre:

- `FVector` intra-cella autorevole;
- mini-navmesh dentro ogni esagono;
- Actor per ogni settore;
- secondo Cover Resolver;
- secondo Facing system.

Il modello deve restare discreto/deterministico.

## 7.3 Owner graybox già esistente

Auditare:

- `#1094`;
- E21 / `#286`;
- `docs/technical/systems/spec-graybox-placement-contract.md` o path corrente.

Questi possiedono già:

- Cell Placement Volume;
- Safe Placement Volume;
- unit footprint;
- spazio visivo per cover/path/facing;
- mesh non authority.

Distinguere:

```text
Graybox Placement
= authoring / dimensioni

Gameplay PlacementSector
= scelta discreta di simulazione
```

## 7.4 Roadmap proposta

### v0.1

Solo thin slice UX:

- selector a spicchi come modo di scegliere Facing;
- ghost;
- legal sectors;
- nessuna nuova cover intra-cell gameplay.

### v0.2

Formalizzazione gameplay:

- PlacementSector;
- CoverAnchor;
- rapporto Placement/Facing;
- cover da bordo/interno/adiacente solo se decisa;
- Low/High cover state;
- stance presentation;
- scenario discriminante;
- bot baseline.

### v0.3+

Portare il sistema attraverso:

- TeamKnowledge/privacy;
- multilivello;
- online authority;
- GAS presentation;
- dedicated;
- balance metrics;
- freeze;
- production validation.

## 7.5 Open decisions da cercare prima di creare

Solo se non esistono equivalenti, creare decision issue per:

- Placement e Facing possono divergere?
- Placement può cambiare senza movimento?
- quali geometrie producono CoverAnchor?
- ingombro interno blocca LOS/movement o solo cover?
- gruppo di spicchi è UI o entità logica?
- forced displacement cosa fa al Placement?
- Placement entra in snapshot/hash o è derivato?

---

# 8. COVER / GUARD / BRACE / DIRECTIONAL DEFENSE

Auditare almeno:

- `#69` — low directional cover;
- `#888` — boundary damage / cover question;
- `#649` — cover bypass trace;
- `#1430` — Guard/Cover outcome split;
- `#1392` — cover readability at boundary;
- `#726` — relative directions futuro;
- `#339` — FAC open decisions.

Preservare:

- Cover = geometria/regola;
- Guard = defense separate;
- Brace = anti Dash/Dodge baseline, omnidirezionale salvo decisione diversa;
- Armor = sistema separato;
- no global backstab/rear damage bonus;
- #726 non va implementata “per comodità”.

Se `PlacementSector` arriverà, non trasformare automaticamente Cover in una proprietà del settore:

```text
Placement
+ CoverGeometry
+ attacker/source geometry
+ current Facing/policy
→ ResolvedCoverState
```

---

# 9. OVERWATCH / FAST REACTION / DECISION BOUNDARY

## 9.1 Owner da auditare

- E14 — `#152`;
- CP14.5 — `#165` — osservato CLOSED nelle chat recenti;
- CP14.6 — `#166` — osservato OPEN;
- CP14.7 — `#314`;
- CP14.8 — `#319`;
- `#583` — condition producer;
- `#657` — bot policy conditions;
- `#888` — cover al decision boundary;
- issue recenti successive.

## 9.2 Stato consolidato da verificare

CP14.5 / FIRE-HOLD core è stato discusso come chiuso.

CP14.6 deve coprire il vero human decision loop:

- FIRE / HOLD UI;
- autorità del decision window;
- countdown reale **3.0 s**;
- timeout = HOLD / `HoldTimeout`, mai FIRE;
- slow motion = presentation-only;
- owner decide;
- teammates read-only;
- opponent non riceve DTO privato;
- misura separata `ReactionDecisionSeconds` vs playback;
- p50/p90;
- test 1/2/3 armed units, incluse multiple dello stesso player;
- PIE evidence.

Resolver non deve aspettare il timer: orchestrator/window gestisce il tempo umano.

## 9.3 Facing / Overwatch

Non creare `OverwatchDirection`.

Il cone/controlled area deriva dal Facing autorevole.

Forced movement del watcher:

- lo rilocalizza;
- ricostruisce cone da cell/facing correnti;
- non cancella automaticamente Overwatch.

Reaction pivot resta decisione separata se ancora aperta.

## 9.4 Trigger

Standard Overwatch punisce Move transitions, non automaticamente Dodge/Dash o forced movement, salvo decisione aggiornata.

Visual trigger richiede detection, non solo LOS.

---

# 10. RUMORE / PERCEZIONE ACUSTICA / TEAM KNOWLEDGE

## 10.1 Owner noti da verificare

Lane consolidata storicamente:

```text
#686 + #690 → #159 → #160
```

Auditare stato live.

- `#686` — HearingThreshold nei cataloghi;
- `#690` — NoiseIntensity nei cataloghi/data/validator;
- `#159` — CP13.4 Rumore → contatto incerto;
- `#160` — bot + HUD su conoscenza parziale.

## 10.2 Principio fondamentale

```text
Il suono non rivela nemici.
Rivela eventi.
Gli eventi producono ipotesi spaziali nella TeamKnowledge.
```

Un rumore base NON concede automaticamente:

- identità;
- cella esatta;
- AbilityId;
- target;
- intent nemico.

## 10.3 Runtime

Noise producer deve nascere da **fatti risolti**:

- micro-step realmente eseguiti;
- azioni realmente risolte;
- environment realmente accaduto.

Non da:

- planned path;
- preview;
- hidden enemy intent.

## 10.4 Data

Auditare decisioni recenti:

- `NoiseIntensity` per action/producer;
- signature ability comprese;
- valori non vanno inventati se non canonici;
- surface modifiers separati.

## 10.5 TeamKnowledge

HUD e bot devono consumare la stessa conoscenza autorizzata.

Awareness corrente da preservare se ancora canonica:

- Hidden;
- Uncertain;
- Detected.

Non introdurre un secondo confidence axis per la stessa semantica.

## 10.6 Privacy/replay

- TurnLog autorevole può restare completo;
- la vista per osservatore va filtrata;
- AcousticContact non deve trasportare identità non autorizzata;
- replay/hash non dipendono dall'osservatore.

---

# 11. LAST KNOWN / LIGHTING / DETECTION

Auditare le issue e roadmap correnti su:

- tactical deterministic lighting;
- detection;
- obscuration;
- Last Known Ghost;
- HUD uncertainty;
- TeamKnowledge.

Non creare una perception parallela.

Catena concettuale:

```text
LOS
→ illumination / obscuration
→ detection
→ TeamKnowledge
→ targeting / bot / HUD
```

Il lighting visuale UE non deve diventare authority della detection.

---

# 12. CAMERA TATTICA / CAMERA LAB / LIGHTING PRESENTATION

Le chat/handoff precedenti hanno definito una roadmap Camera fino alla v1.0.

Claude deve cercare owner già esistenti prima di creare issue.

Copertura richiesta:

### v0.1
- pan;
- zoom;
- rotate;
- focus unit/cell;
- layer handling;
- selection readability;
- Camera Lab/editor evidence.

### post-v0.1
- spectator/replay compatibility;
- accessibility;
- clipping/occlusion handling;
- multilayer transitions;
- performance;
- packaged validation;
- production polish.

Non creare una seconda camera base se il runtime già la possiede.

---

# 13. FRONTEND SHELL / PLAYER-FACING FLOW

Auditare la roadmap v0.1 più recente per Epic aggiunte dopo i vecchi handoff, incluse almeno se ancora presenti:

- E46 — Frontend Shell;
- E47 — autobattle/non-presidiato/runtime automation.

v0.1 player-facing flow richiesto:

```text
Main Menu
→ Play
→ Match
→ Result
→ Back / Quit
```

Non spostare automaticamente frontend fuori v0.1 solo perché handoff storici non lo conoscevano.

---

# 14. SPATIAL TRANSFER / TELEPORT

Auditare E39 e issue correnti prima di creare altro.

Storicamente:

- E39 / `#704`;
- `#700` resolver puro;
- `#701` Short Blink / planning;
- `#702` TurnManager/TurnLog/replay;
- `#703` scenario/gate;
- eventuali issue successive.

Preservare:

```text
Traversal percorre lo spazio.
Transfer cambia posizione senza percorrerlo.
```

Non implementare Blink come LinearLeap rinominato.

Non creare un secondo movement engine.

---

# 15. STATUS / REACTIONS / SUPER / DELAYED / PREDICTIVE

Auditare gli handoff e issue correnti su:

- Status framework;
- reaction triggers;
- delayed actions;
- predictive actions;
- conditional intent;
- Super staged actions;
- revalidation fra stage;
- self displacement/recoil;
- TurnLog coverage.

Owner storicamente rilevanti da cercare:

- E18 / `#225`;
- `#226`;
- `#227`;
- E29 / `#329`;
- E33 / `#330`;
- `#505` reaction triggers;
- `#541` displacement primitive;
- `#605` plan validation;
- `#610` signature resource;
- `#625` hazard TurnLog;
- issue nuove su Super/Ultimate/staged actions.

Non creare un secondo trigger/reaction framework.

---

# 16. EQUIPMENT / WEAPON VARIANTS / CUSTOMIZATION

Auditare almeno:

- E7 / `#21`;
- `#60` weapon variants;
- `#61` gadgets;
- `#62` reaction modules;
- `#63` loadout 1+1+1;
- `#505`;
- `#509` damage bands;
- `#510` se ancora pertinente;
- `#602` Scenario Harness loadout/variant;
- issue successive.

Preservare:

- horizontal tradeoffs;
- no pure upgrades;
- no `HeroId` branches nel resolver;
- current v0.1 loadout contract se ancora canonico;
- build affinity non deve collidere con environmental `Affinity`.

---

# 17. INTERACTIVE MAP / MAP EDITOR / GRAYBOX / VERTICALITY

Auditare i domini:

- walls;
- doors;
- cover;
- interaction graph;
- map authoring;
- graybox placement;
- Cell Placement Volume;
- 3D layers/elevation;
- falls/void;
- bridges/tunnels/elevators;
- map editor roadmap.

Preservare:

- tactical graph 3D;
- cells compact data;
- walls non necessariamente vincolati agli edge degli hex;
- Editor usa gli stessi runtime services;
- niente secondo A*/LOS/targeting solo per Editor;
- map geometry può essere libera, gameplay topology resta esplicita/deterministica.

Auditare issue note come:

- `#324` E23;
- `#1094` graybox questions;
- `#1095/#1096` se ancora aperte;
- `#48` falls baseline;
- `#621` geometry bake / Void;
- issue recenti su height/elevation/landing.

---

# 18. OBJECTIVES / CONTESTED AREAS / TEAM ADVANTAGE

Auditare owner correnti prima di creare:

- E10;
- `#24`;
- `#75`;
- E26 / `#326`;
- E31 / `#332`;
- issue successive su contested area, team advantage, objective scoring.

Non creare:

- secondo Objective System;
- seconda TeamKnowledge;
- seconda economia risorse.

Integrare sempre:

- bot;
- TurnLog/replay;
- determinismo;
- networking/privacy;
- balance;
- HUD/world feedback.

---

# 19. BOT / AI

Auditare bot roadmap corrente e tutti gli owner reali.

Principi:

- bot usa la stessa TeamKnowledge del giocatore;
- difficoltà maggiore = più ragionamento, non più hidden info;
- stessa action economy e resolver;
- niente scorciatoie che saltano Planning/Commit;
- bot competence va misurata per capability;
- bot-vs-bot batch della v0.8 usa lo stesso gioco.

Workstreams da verificare:

- tactical bot baseline;
- expert bot;
- perception fairness;
- Facing/cover scoring;
- Overwatch conditions;
- Placement/CoverAnchor futuro;
- objective reasoning;
- noise/Last Known reasoning.

---

# 20. ROADMAP RELEASE v0.5 → v1.0

Non creare una seconda tassonomia. Integrare ogni nuovo workstream nei package già esistenti.

## v0.5 — Online Foundation / E40

Per ogni feature con planning/private state:

- authoritative server;
- team-only relay;
- enemy privacy canary;
- sequence numbers per preview;
- reliable commit;
- offline vs online deterministic equivalence.

## v0.6 — Ability Runtime / E41

- GAS lifecycle/presentation;
- stable ActionId binding;
- no resolver authority;
- no privacy leak via ASC;
- determinism regression gate.

## v0.7 — Competitive Alpha / E42

- dedicated target;
- match lifecycle;
- private lobby;
- reconnect/resync;
- 3v3 soak;
- no privileged client.

## v0.8 — Beta / Balance / E43

- batch runner;
- bot competence schema;
- provenance;
- performance suite;
- long packaged soak;
- win-rate informs design, not CI gate.

## v0.9 — Release Candidate / E44

- content freeze;
- hostile-client hardening;
- real save/replay migration;
- RC soak.

## v1.0 — Launch / E45

- master release gate;
- production dedicated deployment;
- matchmaking rollout;
- observability;
- security/privacy audit;
- replay audit;
- performance certification;
- content validation;
- packaged smoke matrix;
- rollback tested.

Auditare le issue E40–E45 live, storicamente nell'area `#773–#816`, senza assumere che numeri/titoli siano immutati.

---

# 21. RANKED / RATING / MATCHMAKING GAP

Le chat recenti hanno richiesto esplicitamente di verificare che la roadmap v1.0 non abbia solo “matchmaking” ma copra davvero il percorso competitivo.

Auditare:

- `#810` o owner corrente del matchmaking;
- eventuale Ranked Queue;
- rating/MMR;
- placement matches se previsti;
- season/reset se davvero in scope;
- result persistence;
- reconnect/forfeit policy;
- abuse/hardening;
- osservabilità.

Non inventare un sofisticato rating system se la v1.0 corrente lo tiene minimal.

Se `ranked` è promesso dalla roadmap ma nessuna issue lo possiede, questo è un gap reale da tracciare.

---

# 22. DOCUMENTAZIONE / DECISION LOG / WIKI

Per ogni workstream con decisioni nuove o chat consolidate:

classificare:

```text
Decision Log: UPDATE / N/A
ADR: UPDATE / NEW / N/A
OPEN_DECISIONS: UPDATE / N/A
Owner Spec: UPDATE / N/A
Roadmap: UPDATE / N/A
Issue/Epic: UPDATE / CREATE / LINK
Scenario Map: UPDATE / N/A
PIE/Editor evidence: UPDATE / N/A
Wiki: UPDATE / DEFER / N/A
```

Non creare documenti paralleli quando un owner esiste.

---

# 23. TEMPLATE OBBLIGATORIO PER NUOVE ISSUE

```markdown
## Why
Fatto misurato sul repository o gap reale.

## Canon / Decision
ADR, Decision Log, Open Decision o PROPOSED.

## Scope
Cosa deve esistere.

## Out of scope
Cosa non entra.

## Dependencies
Issue reali.

## Definition of Done
Checklist verificabile.

## Automation
Test discriminanti / mutation quando appropriato.

## Scenario / PIE
Caso end-to-end e/o evidence.

## TurnLog / Replay impact
Esplicito.

## Privacy impact
Esplicito.

## UI / UX impact
Esplicito.

## Performance impact
Esplicito se pertinente.

## Packaged gate
Esplicito.

## Tracking
Release / Epic / parent / related issues.
```

Una issue non deve contenere un “test verde” se il test non esiste.

Usare:

```text
PLANNED
NOT VERIFIED
```

quando necessario.

---

# 24. COME DECIDERE CREATE VS UPDATE

## UPDATE_EXISTING

Usa se l'issue esistente:

- possiede la stessa regola;
- possiede lo stesso consumer;
- è ancora OPEN;
- il nuovo scope è naturale estensione del suo DoD.

## CREATE_NEW

Solo se:

- nessun owner esiste;
- il lavoro è indipendente e chiudibile;
- non è solo un dettaglio di un checkpoint esistente;
- ha test/gate propri;
- non richiede riaprire una CLOSED.

## LINK_ONLY

Usa se il lavoro è già tracciato ma manca il collegamento fra due issue/Epic.

## DECISION_REQUIRED

Usa per game design non deducibile dal codice.

Non nascondere una decisione dentro una issue di implementazione.

---

# 25. PRIORITÀ DELLA PASSATA

Ordine consigliato:

1. **audit globale**;
2. **deduplica**;
3. **correggi owner stale**;
4. **v0.1 blockers e missing player producers/UI**;
5. **issue di decisione necessarie per nuovi sistemi**;
6. **work package v0.2–v0.4**;
7. **cross-release v0.5–v1.0**;
8. **ranked gap**;
9. **report**.

Non implementare 100 issue durante questa sessione.

Devi creare/aggiornare il tracking affinché possano essere implementate una alla volta.

---

# 26. REPORT FINALE OBBLIGATORIO

Produrre una tabella master:

| Domain | Release | Epic/Parent | Existing Issue | Action | New/Updated Issue | Status | Blocking Decision | Dependencies |
|---|---|---|---|---|---|---|---|---|

Poi sezioni:

## A. Updated

Per ogni issue:

```text
#ID — title
WHY UPDATED:
FIELDS/SECTIONS CHANGED:
RELATED:
```

## B. Created

Per ogni nuova issue:

```text
#ID — title
WHY NO EXISTING OWNER WAS ENOUGH:
RELEASE:
PARENT:
DEPENDENCIES:
GATE:
```

## C. Link-only

Cross-link creati senza nuovo lavoro.

## D. Already done

Issue/chat item già implementati: evidenza commit/test/issue.

## E. Deferred

Con release/trigger esplicito.

## F. Decisions required

Domande che richiedono decisione dell'autore.

## G. Conflicts

| Source A | Source B | Conflict | Authority | Resolution |
|---|---|---|---|---|

## H. Counts

```text
ISSUES READ:
ISSUES UPDATED:
ISSUES CREATED:
LINK-ONLY:
ALREADY DONE:
DEFERRED:
DECISIONS OPENED:
```

---

# 27. DEFINITION OF DONE DEL MASTER CONSOLIDATION

La passata è completa solo quando:

- [ ] HEAD `origin/main` registrato;
- [ ] tutte le Epic correnti v0.1 e post-v0.1 sono state rimisurate;
- [ ] HUD roadmap ha owner completi;
- [ ] icon grammar E20/E25 è allineata;
- [ ] Facing ADR-0008/runtime divergence ha un owner;
- [ ] legal Facing/ghost/pointer ownership è coerente;
- [ ] End Placement/CoverAnchor è tracciato come PROPOSED/decisione, non finto canone;
- [ ] graybox placement non è duplicato;
- [ ] Overwatch CP14.6 ha scope UI/privacy/pacing completo;
- [ ] Rumore lane #686/#690/#159/#160 è rimisurata;
- [ ] perception/HUD/bot condividono TeamKnowledge;
- [ ] Camera workstream è mappato senza duplicate camera base;
- [ ] Frontend Shell/current v0.1 flow è verificato;
- [ ] Spatial Transfer non ha doppio movement engine;
- [ ] Status/Reaction/Predictive/Super non hanno framework paralleli;
- [ ] Equipment/loadout owner sono riallineati;
- [ ] Map/Interactive/Graybox/Height owner sono riallineati;
- [ ] Objective/Contested Area non crea Objective System parallelo;
- [ ] Bot roadmap rispetta fair knowledge;
- [ ] E40–E45 contengono i cross-cutting requirement dei nuovi sistemi;
- [ ] Ranked/Rating gap verificato;
- [ ] nessuna issue CLOSED è stata riaperta per scope nuovo;
- [ ] nessuna nuova release parallela;
- [ ] nessun vecchio registry rimosso è stato reintrodotto;
- [ ] ogni nuova issue ha DoD/test/replay/privacy/UI/packaged impact;
- [ ] report finale contiene link GitHub reali.

---

# 28. OUTPUT CONCLUSIVO DI CLAUDE CLOUD

Concludi con:

```text
BASE COMMIT:
CURRENT MAIN:

ISSUES READ:
ISSUES UPDATED:
ISSUES CREATED:
ISSUES LINKED:
ISSUES ALREADY DONE:
ISSUES DEFERRED:
DECISIONS OPENED:

ROADMAP CHANGES:
DOC CHANGES:
CONFLICTS:

TOP 10 NEXT IMPLEMENTATION ISSUES:
1.
2.
3.
4.
5.
6.
7.
8.
9.
10.
```

Per ogni voce non verificata usare `NOT VERIFIED`.

---

# 29. PRINCIPIO FINALE

Il compito non è massimizzare il numero di issue.

Il compito è trasformare tutte le decisioni e attività descritte nelle chat in un backlog **senza doppioni**, dove ogni lavoro abbia:

```text
Decision/Canon
→ Release
→ Epic/Owner
→ Issue
→ Scenario/Test
→ Replay/Privacy
→ UI/Editor
→ Packaged Gate
→ Evidence
```

Se una chat descrive qualcosa che è già stato implementato o tracciato, aggiorna/collega l'owner invece di crearne un altro.
