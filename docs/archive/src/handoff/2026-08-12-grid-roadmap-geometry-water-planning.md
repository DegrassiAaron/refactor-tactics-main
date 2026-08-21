> 🔎 **ESITO DELLA REVISIONE — 2026-08-13.** Archiviato come **inventario di design**, ⛔ **respinto come
> roadmap**. Referto:
> [`hexgeometry-editor-spec-panel-2026-08-13.md`](../../../roadmap/plans/hexgeometry-editor-spec-panel-2026-08-13.md).
>
> ⛔ **`R0`–`R11` con venticinque work item numerati è una roadmap manuale che duplica il Feature Registry** —
> l'errore che il brief HexGeometry, revisionato lo stesso giorno, elenca al suo §40 fra quelli da non fare.
> Le otto feature che questo documento vuole tracciare **esistono già** in
> `feature-registry.yaml` con i loro gate e il loro stato derivato:
> `RT-FEAT-MAP-WATER-DYNAMICS`, `RT-FEAT-MAP-STRUCTURAL`, `RT-FEAT-MAP-VERTICALITY`, `RT-FEAT-UI-PLANNING`,
> `RT-FEAT-UI-CERTAINTY`, `RT-FEAT-UI-ACTION-GHOSTS`, `RT-FEAT-TOOL-MAP-EDITOR`, `RT-FEAT-TOOL-MAP-GEOMETRY`.
> Il §16 introduce inoltre un **terzo** vocabolario di milestone (`G`, `M`, `V`, `S`, `W`, `E`, `I`, `U`, `Q`)
> accanto a `M<n>`/`E<n>` già in uso.
>
> ✅ **Il §1 regge ed è utile**: i quindici vincoli «già consolidati da non riaprire» sono stati verificati e
> sono corretti, `D-071`/`D-081`/`D-082` incluse.
>
> ✅ **I gate che propone al §11 del suo `APPLY_WITH_CLAUDE` sono già verdi**, e nessuno li aveva misurati:
> `WaterDepth` ha **zero** occorrenze nel codice e le nove nei documenti stanno in archivio, nei referti, o —
> l'unica viva — nella §6-ter di [`spec-terreni-e8.md`](../../../gameplay/spec-terreni-e8.md), che è
> **la sezione che lo dichiara superseded da `D-081`**. Idem `BreachSlot` e `TransitOnly`. La convenzione
> `Spec.<Area>.<Name>` è già rispettata da 35 scenari.
>
> ⚠️ I sette scenari `Spec.Environment.*` che propone **non esistono** e restano `planned`: descrivono
> capability non implementate, e uno scenario scritto prima della capability verifica il nulla.

# Roadmap completa — Geometry · Movement · Verticality · Structures · Water/Electric · Intel · Planning UI

> **Data:** 2026-08-12  
> **Stato:** `PROPOSED CONSOLIDATION ROADMAP`  
> **Origine:** decisioni consolidate nella chat su griglia, muri, cover, traversal, acqua/elettricità e Planning HUD.  
> **Scopo:** trasformare tutte le decisioni discusse in lavoro tracciabile senza duplicare feature, issue o owner esistenti.
>
> Questo file è una **roadmap di integrazione**, non una nuova fonte di gameplay. Le regole diventano
> canoniche solo quando vengono recepite negli owner appropriati: Decision Log, ADR, gameplay/technical spec,
> cataloghi e `feature-registry.yaml`.

---

## 0. Obiettivo finale

Arrivare a una pipeline unica:

```text
Architectural Authoring
        ↓ bake deterministico
Hex Tactical Graph + Logical Geometry
        ↓
Snapshot
        ↓
Path / LOS / Cover / Traversal / Conductivity / Structural State
        ↓
Planning Preview
        ↓
Intent
        ↓
Authoritative Resolver
        ↓
TurnLog / Replay / UI explanation
```

Lo stesso stato logico deve alimentare pathfinding, LOS, cover, movement/traversal, collisioni simultanee,
verticalità, distruzione, acqua/corrente, propagazione elettrica, Planning Preview, Team Knowledge/Intel,
replay e debug.

Nessun sistema parallelo basato su mesh, Chaos, widget o euristiche visuali.

---

# 1. Vincoli già consolidati da non riaprire

| Area | Vincolo |
|---|---|
| Grid | `FRTCellId{q,r,Layer}` è l'unica posizione gameplay |
| Geometry | `HexEdge != WallSegment`; la geometria architettonica non è vincolata ai bordi dell'hex |
| Bake | il runtime competitivo consuma dati baked; mesh/collisione non sono authority |
| Standability | prevale **D-071**: footprint/cerchio inscritto; non center-only |
| Hex size | configurabile; nessun `1.5m` hardcoded |
| Doorway | niente `TransitOnly` baseline |
| Occupancy | una unità occupa un hex; alleati/nemici non sono attraversabili di default |
| Cover | sei settori tattici; strongest wins; environmental cover non dipende dal facing |
| High ground | niente bonus numerico universale; vantaggio geometrico / ability-specific |
| ZoC | nessuna Zone of Control universale |
| Overwatch | enemy planned Overwatch segreto fino a trigger/reveal autorizzato |
| Electric | CP8.3 = BFS conduttiva + `PropagationLimit`; non raggio euclideo |
| Multilayer electric | CP9.4 = archi `bConductsElectricity`, anche fra Layer |
| Water depth | prevale **D-081**: future depth = composite surfaces |
| Structural slot | prevale **D-082**: nome `Bulkhead`, non `BreachSlot` |
| Planning UI | preview usa query di gameplay + dati autorizzati; nessun enemy-intent leak |

---

# 2. Sequenza di consegna

Non implementare tutto come una singola epic.

```text
R0 Canonical consolidation
 → R1 Geometry/Bake
 → R2 Cover/LOS/Openings
 → R3 Movement/Traversal/Occupancy
 → R4 Verticality/Forced Movement
 → R5 Structural destruction
 → R6 Water dynamics
 → R7 Conductive network
 → R8 Intel/Overwatch secrecy
 → R9 Planning Preview
 → R10 Editor/Scenario/QA
 → R11 Replay/Packaged/Network readiness
```

Le tranche possono sovrapporsi solo quando i contratti dati a monte sono stabili.

---

# 3. R0 — Consolidamento canonico e Project Control

**Priorità:** P0 · **Target:** immediato · **Blocca:** tutto il resto.

## R0.1 — Recepire le decisioni della chat

Aggiornare, se non già recepite:

- `docs/decisions/RT_PDR_00_Decision_Log.md`;
- `docs/DOC_CONFLICT_MATRIX.md`;
- `docs/OPEN_DECISIONS.md`;
- owner gameplay/technical pertinenti.

Da registrare esplicitamente: geometry-vs-hex; opening/door conservative bake + editor override; cover sectors;
occupancy baseline; traversal failure semantics; MoveEnd/cause summary; no universal ZoC; Overwatch secrecy +
reveal levels; elevation/floor separation; forced fall/jump-down/ledges; structural dependencies/collapse/rubble;
conductive connector model; persistent electric hazard; Planning semantic grammar.

**DoD:** nessuna decisione `LOCKED` della chat vive solo in un handoff; nessuna regola superseded è ancora
proposta dal registry/wiki; D-071/D-081/D-082 sono linkate dagli owner dipendenti.

## R0.2 — Allineare Feature Registry

**Source:** `docs/roadmap/feature-registry.yaml`.

Riusare prima di creare:
`RT-FEAT-MAP-WATER-DYNAMICS`, `RT-FEAT-MAP-STRUCTURAL`, `RT-FEAT-MAP-VERTICALITY`, feature geometry/E23,
feature environment/electric esistente, `RT-FEAT-UI-PLANNING`, `RT-FEAT-UI-CERTAINTY`,
`RT-FEAT-UI-ACTION-GHOSTS`, private planning/Team Knowledge/reactions.

**Regola:** search before create.

**Gate:**

```bash
python scripts/feature_registry.py validate
python scripts/feature_registry.py generate
python scripts/feature_registry.py shortlist
python scripts/feature_registry.py wiki
```

poi i relativi `--check`.

---

# 4. R1 — Geometry & Bake Foundation

**Priorità:** P0 · **Owner:** E23/geometry feature esistente · **Target consigliato:** v0.2 foundation.

## R1.1 Logical architectural geometry

Definire dati authoring per wall segment, junction/corner, aperture, door/window profile, logical height/
occlusion class, destructibility profile, Stable ID.

Vincoli: angoli architettonici inclusi 90°; runtime non legge mesh; bake deterministico; hex size parametrico.

## R1.2 Standability bake

Chiudere D-071:

```text
Architectural geometry → footprint test → Cell Walkable / Blocked
```

Test: wall fuori footprint, wall che interseca footprint, permutation invariance, hex-size scaling.

## R1.3 Transition bake

Derivare per ogni collegamento: pass/block, opening binding, traversal type, cost, override source, revision.
Apertura valida solo con clear interior; boundary touch = blocked. Editor override persistente e validato.

## R1.4 Openings

Supportare 1 opening → 0..N cells/transitions, large doors, windows con Movement/LOS/Projectile policy separati.
Corner opening solo profile esplicito futuro.

---

# 5. R2 — LOS, Cover e Openings Runtime

**Priorità:** P0/P1 · **Dipende da:** R1.

## R2.1 LOS logical geometry

Closed door blocca; open aperture passa se clear interior. Nessun mesh raycast authority.

## R2.2 Cover bake a sei settori

Cover locale al target; non stacka; strongest wins `None < Light < Heavy`; stessa geometry può proteggere più
settori; environmental cover indipendente dal facing; tie-break deterministico.

## R2.3 Elevation response profiles

Cover/occlusion degradano per differenza discreta di quota, data-driven.

## R2.4 Occlusion classes

Classi logiche discrete tipo `Low / Medium / Tall / FullLevel`, senza soglie centimetriche runtime.

---

# 6. R3 — Occupancy, simultaneous movement e Traversal

**Priorità:** P0 · **Riusa:** resolver movimento esistente.

## R3.1 Occupancy dependencies

```text
Collect proposals → Build occupancy dependencies → Resolve → Atomic commit
```

Baseline: occupied cell blocker; niente ally pass-through; contested destination senza hidden initiative winner;
open chain verso free cell può riuscire; stationary terminal fallisce; direct swap e closed cycles bloccati.

Test: open chain, blocked terminal, contested destination, swap, cycle, permutation invariance.

## R3.2 Special CollisionPolicy

Solo ability esplicite possono derogare: Phase, Swap, Teleport, Push.

## R3.3 Vault

Special transition dentro Move; costo extra; `MaxVaultsPerMove` da movement profile; StandardUnit baseline 1;
facing preservato; reaction/Overwatch normali; occupied target = Block; fail resta SourceHex; no auto-landing.

## R3.4 Transition hazards

Sequenza canonica:

```text
Validate → PreTraversal → PreEntryHazard → TransitionReaction
→ Commit → OnEnterReaction → PostEntryHazard
```

## R3.5 Movement interruption

Status/effect dichiarano `MovementInterruptionPolicy`; path residuo cancellato; residual budget perso; nessuna
ripresa automatica del vecchio path.

## R3.6 MoveEnd semantics

Sempre generato con `ActualMoveEndCell`, `MoveEndReason`, `PrimaryCause`, `ContributingCauses[]`.
Reason: Completed, Blocked, Interrupted, Cancelled.

---

# 7. R4 — Verticality, Drop, Fall e Forced Movement

**Priorità:** P1 · **Feature:** `RT-FEAT-MAP-VERTICALITY` · **Target:** post-v0.1/v0.2+.

## R4.1 Explicit vertical transitions

Stairs, Ramp, Ladder, Climb, VaultUp, Elevator o ability. Same q/r different Layer non implica adjacency.

## R4.2 JumpDown / Drop

Traversal esplicito con max height/profile, cost, landing legality, optional damage/knockdown. Landing occupata
= fail, nessuna deviazione automatica.

## R4.3 ForcedFall

Destruction/knockback può produrre ForcedFall deterministico, anche concatenato su più floor mancanti.

## R4.4 LethalDrop

Formalizzare `SafeLanding / DamagingFall / LethalDrop / OutOfBounds`.
Se non ancora registrato, chiudere il comportamento esatto; raccomandazione: KO/out-of-play deterministico,
distinto da DamagingFall.

## R4.5 FallCollision

No stacking: impact + displacement deterministico dell'occupante, usando momentum direction quando presente.

## R4.6 Forced displacement chains

Catene atomiche se terminano libere; se non risolvibili, residual energy → `ImpactStrength`
`None/Light/Medium/Heavy`.

---

# 8. R5 — Structural Destruction, Bulkhead, Collapse e Rubble

**Priorità:** P1 · **Feature:** `RT-FEAT-MAP-STRUCTURAL` · **Target:** v0.2+.

## R5.1 Bulkhead model

Usare `Bulkhead`. Dati: StableId, Integrity, StructuralResistance, State, ControlledCells,
ControlledTransitions, LOS/Cover consequences, destruction/repair profile.

## R5.2 Structural states

`Intact → Damaged → Critical → Breached/Opened → Collapsed`, con eventuali Patched/Reinforced.
Niente percentuali continue di passabilità.

## R5.3 BreakAndContinue

Push può rompere struttura, aggiornare graph/LOS/cover e continuare nello stesso resolver solo se il profile
lo consente.

## R5.4 Structural dependency DAG

Support → Floor → Wall; policy `None/Damaged/Critical/Collapse/DelayedCollapse`.
Validator: cycle detection, stable ordering, cascade limits.

## R5.5 CollapsePending

Immediate/delayed collapse con timing deterministico `EndOfStage / EndOfTurn / N turns`, mai animation time.

## R5.6 Rubble

`Empty / Rubble / HeavyRubble / DebrisField / BlockedDebris / Custom`, con effetti su cost/cover/LOS/projectile/
hazards/transitions/revision.

## R5.7 Secondary destruction

Esempi: `HeavyRubble → Rubble → Clear`, `Rubble → BurningDebris → Ash`.

## R5.8 Connectivity validation

Una unità può restare isolata; è valido finché il match/objective non entra in soft-lock non dichiarato.

---

# 9. R6 — Water Dynamics

**Priorità:** P2 dopo geometry foundation · **Feature:** `RT-FEAT-MAP-WATER-DYNAMICS`.

## R6.1 Modello D-081

Non introdurre `Surface + WaterDepth axis`. Baseline corrente: superfici composte, incluse future
`DeepWater/ImpassableWater`.

## R6.2 Flooding

Transition table deterministica tra superfici. Ogni cambio aggiorna MapState, revision/cache se necessario,
TurnLog e replay.

## R6.3 Flow graph

Separare water presence/surface, flow direction, forced movement e conductivity.

## R6.4 Movement profiles

Standard, Amphibious, Hover, Swim, special traversal. Nessuna deduzione dal volume water mesh.

---

# 10. R7 — Generalized Conductive Network

**Priorità:** P1/P2 · **Riusa:** CP8.3 (#66) + CP9.4 conductive arcs.

## R7.1 ConnectedConductiveGraph

È un concetto unificante. Prima scelta: estendere cell/arc esistenti, non creare un secondo engine.

## R7.2 ConductiveConnector

Representation solo se gli archi baked non bastano. Connector multi-port con branching.

## R7.3 Long conductor segmentation

Rail/tubo/struttura lunga = più hop logici; `PropagationLimit` mantiene significato tattico.

## R7.4 No automatic air arc

Nessun edge attraverso gap. Future `ArcLink` solo ability esplicita.

## R7.5 Conductive items

Item/gadget aggiunge/rimuove connessioni ed estende Chain Lightning.

## R7.6 PersistentElectricHazard

Separato dalla scarica standard. A ogni activation boundary rivaluta snapshot/topologia e pulsa.
Nuova connessione non scarica fuori sequenza.
Policy: `PulseOnly / OnEnterOnly / PulseAndOnEnter`; baseline design: `PulseAndOnEnter`.

## R7.7 Chain Lightning query

DTO/query condivisa con `PrimaryReach`, `PrimaryTarget`, `ChainReach`, `ConductivePath`, `SecondaryTargets`,
`RemainingPropagationBudget`.

---

# 11. R8 — Overwatch secrecy, Reveal e Team Intel

**Priorità:** P1 network/privacy.

## R8.1 Hidden reaction invariant

Hidden enemy Overwatch non altera icon, stance, cone, pathfinding, threat map, cost, hover response o timing.

## R8.2 Reveal levels

`0 Hidden / 1 Presence / 2 Type / 3 Approximate Area / 4 Full Tactical Reveal`.

## R8.3 Snapshot vs Continuous intel

Default reveal = Snapshot; piano cambiato dopo reveal non aggiorna automaticamente l'avversario.

## R8.4 TeamIntelStore

Distinguere `CurrentAuthoritativeState / TeamKnownState / LastKnownState`. Intel scoperta da un ally viene
condivisa con il team autorizzato.

## R8.5 Expiration

Intent/reaction intel end-of-turn; persistent intel finché logicamente valido; hidden moving object = last-known,
non live tracking.

---

# 12. R9 — Planning Preview Visual Grammar

**Priorità:** P1 · **Feature:** `RT-FEAT-UI-PLANNING`, `RT-FEAT-UI-CERTAINTY`,
`RT-FEAT-UI-ACTION-GHOSTS`.

**Issue esistenti da riesaminare prima di crearne altre:** #77, #78, #172, #173, #613.

## R9.1 Semantic tokens

Move green; Attack red; Utility blue; Control amber; Overwatch/Reaction purple; Post turquoise;
Chain/Electric electric blue; Invalid gray. Centralizzati in theme data, non hardcoded per ability.

## R9.2 CVD/accessibility

Ogni semantic ha secondo canale: pattern, outline, line style, icon/shape. Verifica visuale dedicata.

## R9.3 Active-step focus

Selected step full emphasis; previous planned ghosted; future unselected hidden/minimal.

## R9.4 Projected-state preview

Move→Attack usa projected final cell, non current Actor transform.

## R9.5 Preview/resolver parity

Stesso snapshot + stessa action + stessa authorized knowledge ⇒ preview set coerente con resolver.

## R9.6 Electric explanation

Mostrare primary range/target, conductive path, connector, branch, secondary targets, propagation limit.

## R9.7 Certainty independent axis

Semantic e certainty restano indipendenti: `Move+Confirmed`, `Attack+Predicted`, `Electric+Uncertain`.

## R9.8 Warning composition

Collision, friendly fire, resource, topology risk, hazard e uncertainty sono overlay, non sostituti del semantic.

---

# 13. R10 — Editor Map, Scenario Map, Wiki, Validators

**Priorità:** P1 continuo.

## R10.1 Editor authoring

Aggiornare `editor-sessions.yaml`, non il tracker storico. Pianificare sessioni per geometry/opening bake,
cover sectors, vertical transitions, Bulkhead DAG, composite water, conductive graph, Planning overlay/CVD.

## R10.2 Scenario corpus

Usare `Spec.<Area>.<Name>` e tag liberi; niente `SCN-*`.

Casi minimi da mappare agli scenari esistenti o creare come `planned`: Geometry footprint/opening/cover/elevation;
occupancy chain/swap/cycle; vault/reaction/PreEntry/MoveEnd; JumpDown/ForcedFall/LethalDrop/FallCollision;
Bulkhead/cascade/rubble; flooding/current; conductive bridge/long conductor/gap/branch/persistent hazard;
Planning Move→Attack/Chain/no-leak/CVD.

## R10.3 Validators

Coprire duplicate geometry IDs, invalid opening binding, invalid transition target, structural dependency cycle,
cascade limits, impossible fall destination, invalid water transitions, connector invalid ports, zero-cost
teleport conductor, unknown UI token, invalid feature/scenario refs.

## R10.4 Wiki

Aggiornare pagine di geometry, cover/elevation, movement/traversal, verticality, destruction, acqua/elettricità,
Planning Preview, Overwatch/Intel. Wiki spiega, non possiede numeri.

---

# 14. R11 — TurnLog, Replay, Determinism, Network/Privacy, Packaged

Ogni tranche è Done solo con i gate applicabili.

## R11.1 TurnLog / provenance

Stable EventId, CauseEventIds ordinati, DAG, Origin/Immediate source dove serve, GraphRevision corrente,
stable IDs. MoveEnd summary ridotto; causal graph completo nel TurnLog.

## R11.2 Replay

Stesso snapshot + rules/content versions + seed + decision inputs ⇒ stesso result/hash.
Coprire dynamic graph, structural state, water surface, conductive connector, persistent hazard e intel/reveal.

## R11.3 Privacy tests

`NoEnemyIntentExposed`, `NoHiddenOverwatchLeak`, `NoUnauthorizedIntelFields`,
`PreviewUsesAuthorizedKnowledgeOnly`.

## R11.4 Performance

Misurare bake, A*, preview, resolver, conductive BFS e cascade bounds.

## R11.5 Packaged

Development + Shipping + scenario/PIE pertinente + asset migration reale dove richiesto.

---

# 15. Sequenza issue consigliata

Questa tabella non inventa numeri GitHub.

| # | Work item | Azione |
|---:|---|---|
| 1 | Consolidate chat decisions into owners | update/create docs issue |
| 2 | Geometry bake + D-071 | update E23/checkpoint esistente |
| 3 | Opening/transition bake | update E23; new issue solo se gap |
| 4 | Cover 6-sector + elevation | reconcile con cover issues |
| 5 | Occupancy atomic chains | verificare movement issues/tests |
| 6 | Vault + transition hazards | new post-v0.1 issue se assente |
| 7 | MoveEnd/cause model | reconcile TurnLog/reason-code work |
| 8 | Vertical transitions/drop/fall | update `RT-FEAT-MAP-VERTICALITY` |
| 9 | FallCollision + forced chains | child verticality |
| 10 | Bulkhead model | update `RT-FEAT-MAP-STRUCTURAL` |
| 11 | Structural DAG/collapse | child structural |
| 12 | Rubble transformations | child structural |
| 13 | Composite water/flooding | update `RT-FEAT-MAP-WATER-DYNAMICS` |
| 14 | Water current/forced movement | child water |
| 15 | Generalized conductive connector | update electric feature |
| 16 | Long conductor / branching | child electric |
| 17 | Persistent electric hazard | future electric issue |
| 18 | TeamIntel snapshot model | update perception/intel |
| 19 | Hidden Overwatch no-leak | update reaction/privacy |
| 20 | Planning semantic tokens | update UI Planning |
| 21 | Projected-state preview | update UI Planning/Ghosts |
| 22 | Chain preview parity | UI + electric |
| 23 | CVD/accessibility | UI/editor |
| 24 | Scenario/validator completion | per owner |
| 25 | Replay/network/packaged gates | chiusura con evidenza |

---

# 16. Milestone logiche

La numerazione reale resta quella del repository.

**G — Geometry foundation:** bake, standability, openings, cover/LOS.  
**M — Movement semantics:** occupancy, vault, hazards, MoveEnd.  
**V — Verticality:** vertical arcs, Drop/Fall, FallCollision, forced chains.  
**S — Structural map:** Bulkhead, DAG, collapse, rubble, BreakAndContinue.  
**W — Dynamic water:** composite surfaces, flooding, current, movement profiles.  
**E — Conductive network:** connectors, branches, long conductors, Chain Lightning, persistent hazard.  
**I — Intel/privacy:** hidden reactions, reveal, snapshot/last-known, TeamIntel, no-leak.  
**U — Planning UX:** semantic tokens, active step, projected preview, chain explanation, certainty, CVD.  
**Q — Release quality:** scenarios, validators, wiki, replay, packaged, performance, privacy.

---

# 17. Definition of Done globale

Il focus della chat è completato solo quando:

- [ ] ogni decisione ha un owner canonico;
- [ ] `feature-registry.yaml` copre tutte le feature senza duplicati;
- [ ] shortlists/maps sono rigenerate e verdi;
- [ ] geometry authoring produce tactical bake deterministico;
- [ ] LOS/cover/path/traversal consumano lo stesso MapState;
- [ ] movement simultaneo è permutation-invariant;
- [ ] verticalità/falls non usano physics come authority;
- [ ] destruction aggiorna graph/LOS/cover nello stesso resolver;
- [ ] water dynamics segue D-081 o una ADR che la supersede;
- [ ] electricity riusa conductive cells/arcs e non introduce un secondo graph divergente;
- [ ] persistent electric hazard è distinto dalla scarica standard;
- [ ] hidden enemy Overwatch/intents non producono leak;
- [ ] Planning Preview usa authoritative/shared queries;
- [ ] semantic, certainty e warning sono assi UI separati;
- [ ] scenari automatici e PIE/manual checks sono mappati;
- [ ] validator coprono dati impossibili/cicli/referenze;
- [ ] TurnLog/replay rappresentano i nuovi stati;
- [ ] Game + Editor build verdi;
- [ ] packaged gate verificato dove richiesto;
- [ ] nessun workbook RESEARCH è promosso accidentalmente a fonte canonica.

---

# 18. Cosa NON fare

- non modellare i muri come soli bordi hex;
- non reintrodurre center-only standability;
- non introdurre posizione continua/subcell per le unità;
- non permettere pass-through automatico degli alleati;
- non introdurre ZoC universale;
- non usare mesh raycast/Chaos per risolvere gameplay;
- non usare `WaterDepth` parallelo contro D-081;
- non creare un secondo conductive engine ignorando CP8.3/CP9.4;
- non far saltare Chain Lightning attraverso gap senza edge;
- non rendere persistente ogni scarica elettrica;
- non usare hidden enemy Overwatch per threat map/path preview;
- non riscrivere range/BFS in UMG/Blueprint;
- non editare a mano feature/scenario/milestone/editor shortlist generate;
- non usare il Balance workbook v0.1 come source of truth.

---

# 19. Prossimo passo operativo per Claude

La prossima attività è **R0**:

1. audit HEAD contro questa roadmap;
2. mappare ogni work item a `feature_id`, owner e issue esistente;
3. aggiornare gli owner;
4. creare solo le issue realmente mancanti;
5. aggiornare `feature-registry.yaml`;
6. rigenerare Feature/Scenario/Milestone/Editor maps;
7. restituire:

```text
Roadmap Item
→ Feature ID
→ Existing Issue / New Issue
→ Owner
→ Release
→ Dependencies
→ Tests
→ Status
```

Solo dopo il mapping iniziare implementazione.

## Commit suggerito

```text
docs(roadmap): consolidate geometry water electricity and planning workstreams
```
