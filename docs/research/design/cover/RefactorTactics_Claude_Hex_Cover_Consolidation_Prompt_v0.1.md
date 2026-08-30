# PROMPT CLAUDE — REFACTORTACTICS
## Consolidamento Hex Footprint / Cover / Facing / Animation / Overwatch

Agisci nel repository reale **RefactorTactics** e consolida le decisioni sotto in modo end-to-end. Non limitarti a scrivere un report: aggiorna le fonti canoniche e il tracking realmente usato dal progetto, ma **non implementare gameplay C++ in questa passata salvo piccoli validator/tooling documentali strettamente necessari**.

### Input visuale
Integra come diagramma esplicativo l'asset:

`RefactorTactics_Hex_Footprint_Cover_Examples_v0.1.svg`

Scegli il path coerente con la struttura corrente (es. `docs/wiki/images/gameplay/` solo se ancora valido). L'SVG NON è la source of truth: deve illustrare decisioni che vivono in spec/ADR/data contracts.

---

## 0. Preflight obbligatorio

Prima di modificare:

1. sincronizza `origin/main`;
2. verifica branch, HEAD, working tree, PR recenti e versione UE realmente bloccata;
3. leggi `AGENTS.md`, `CLAUDE.md`, `README.md`, docs index;
4. leggi Decision Log / ADR / Open Decisions / conflict matrix;
5. verifica roadmap e tracking correnti; **non assumere che Feature Registry o vecchie shortlist esistano ancora**;
6. cerca codice as-built, spec, Epic/Issue OPEN e CLOSED, milestone e Wiki;
7. cerca prima di creare: REUSE / UPDATE / LINK / ALREADY_DONE / DEFER / DECISION_REQUIRED;
8. non inventare numeri di Epic/Issue e non riaprire issue CLOSED solo per aggiungere scope.

Ordine di prevalenza:

`main + codice as-built > ADR/Decision Log corrente > spec canoniche > roadmap/tracking live > questo prompt > PDR/handoff storici`

Questo prompt contiene **nuove decisioni di chat da riconciliare**. Se non confliggono con il canone, promuovile nella source of truth appropriata. Se confliggono, NON scegliere silenziosamente: registra il conflitto e crea/aggiorna il decision owner.

---

# 1. Owner e materiale da auditare

Cerca almeno gli owner equivalenti correnti di:

- Facing / Pivot: E16 / `#175`, `#291` o successori;
- Graybox / Cell Placement / Safe Placement: E21 / `#286`, `#1094`, `spec-graybox-placement-contract.md` o path corrente;
- Cover: `#69`, `#888`, `#649`, `#1392`, `#1430` o successori;
- Overwatch / Fast Reaction: E14 / `#152`, CP14.5/14.6 (`#165`, `#166`) e successori;
- HUD/world overlay per legal facing/cover/readability;
- map authoring, structural objects, rubble/debris e validators;
- Scenario Harness / DevSandbox / Automation tests.

Non creare una seconda Epic Facing, Cover, Overwatch o Map Placement se esiste già un owner.

---

# 2. DECISIONI LOCKED DA QUESTA CHAT

## 2.1 Occupazione logica

- Una TacticalUnit standard occupa **una sola cella logica**.
- Una cella logica contiene **al massimo una unità viva**; niente stacking standard.
- Mesh, capsule, Chaos e collisioni visuali NON sono authority per occupancy/cover/LOS.
- Gli oggetti tattici devono avere footprint/data canonici deterministici.

## 2.2 Grammatica geometrica dell'hex

Riconcilia con la guida visuale corrente che usa:

- **12 spicchi di footprint da 30° + 1 core**;
- **6 Facing logici da 60°**;
- due spicchi adiacenti corrispondono geometricamente a un settore di Facing.

Questa separazione è intenzionale:

`spicchio = footprint / costruzione geometrica / ingombro`
`Facing = orientamento logico dell'unità`

NON trasformare automaticamente i 12 spicchi in 12 Facing.

Gli oggetti (roccia, cassa, detriti, pilastro, ecc.) devono essere rappresentabili come **solidi costruiti/snap sulle linee costruttive dell'hex**, marcando esplicitamente gli spicchi occupati.

Esempi da documentare:
- cella libera;
- occupazione parziale laterale;
- occupazione parziale ad angolo;
- masso/core occupato;
- low wall su edge;
- high wall/edge strutturale;
- footprint multi-cell se un asset grande lo richiede, senza introdurre sub-navmesh.

## 2.3 Occupabilità vs spicchi

NON assumere che “uno spicchio occupato = cella non occupabile”.

La decisione della chat è:
- un oggetto può occupare alcuni spicchi lasciando la cella utilizzabile;
- il personaggio non deve visivamente intersecare il solido;
- l'invasione del core / safe placement volume rende la cella non occupabile **secondo il contratto canonico di placement**.

Qui devi riconciliare il termine `core` con `Cell Placement Volume`, `Safe Placement Volume` e unit footprint già presenti. Non creare due sistemi paralleli.

## 2.4 Cover non coincide con occupancy

Tenere separati:
- `Occupancy / Standable`;
- `Visual/Tactical Solid Footprint`;
- `Low Cover`;
- `High Cover / wall`;
- `Movement Block`;
- `LOS Block`;
- `Projectile Block`;
- `Opacity`;
- `CoverAnchor / presentation`.

Non definire implicitamente `OccupiedSector => Cover`.

Cover deve continuare a essere risolta dal sistema canonico:

`placement/position + cover geometry + attacker/source geometry + Facing/policy -> ResolvedCoverState`

## 2.5 Cover presentation

### Generic / non-edge cover
Quando l'unità sceglie una normale cover che NON richiede aderenza a muro/edge:
- resta nella stessa cella logica;
- la presentazione principale è **crouch / posa abbassata**;
- nessun micro-posizionamento autorevole;
- non servono animazioni diverse per ogni Facing/spicchio.

### Wall / Edge cover
Muri/edge/corner possono in futuro usare:
- CoverAnchor;
- Peek;
- Motion Warping / presentation anchors;
ma questi sono presentation tools e non devono diventare authority.

## 2.6 Facing e corpo

LOCKED:
- corpo, busto e arma non hanno yaw tattici indipendenti;
- dove è girato il corpo, lì guarda/spara la posa;
- il Facing logico ha 6 direzioni da 60°;
- una posa può orientarsi **fino a ±30° dentro il settore di Facing corrente**;
- oltre il confine del settore deve risultare un altro Facing logico/pivot, secondo le regole canoniche.

Non implementare una free-aim upper-body che mostri un'arma in una direzione incompatibile con il corpo/Facing.

## 2.7 RiseToFire

Per un attacco che parte da cover crouched e richiede di alzarsi:
- `CrouchedCover -> RiseToFire -> Fire -> Recover -> CrouchedCover`;
- dal `BeginExposure` fino al recupero effettivo della cover l'unità è **Exposed**;
- la perdita/riacquisizione di cover è logica e deterministica, NON guidata da Animation Notify;
- animazioni/UI consumano eventi canonici del resolver.

Prevedere una policy data-driven equivalente a:
- `StayCovered`;
- `ExposeToFire`;
- `BreakCover`;
senza imporre questi nomi se il repository ha già una tassonomia.

## 2.8 Overwatch standard

LOCKED:
- Overwatch è “sto fermo, guardo e controllo il settore”;
- Overwatch standard usa una **standing ready pose**;
- per TUTTA la durata di Overwatch standard l'unità **non beneficia della Low Cover**;
- non usa RiseToFire quando scatta il trigger perché è già in piedi/ready;
- `HOLD` mantiene la ready stance;
- `FIRE` spara dalla ready stance;
- nessun automatico “crouch gratuito” deve essere inferito solo perché prima era in cover;
- High Wall / geometria strutturale continua comunque a bloccare LOS/proiettili: “no Low Cover benefit” NON significa ignorare la mappa;
- il cono/controlled area continua a derivare dal Facing canonico; NON creare `OverwatchDirection`.

---

# 3. OPEN DECISIONS — NON CHIUDERE SILENZIOSAMENTE

Creare/aggiornare il decision owner solo se non esiste già.

## OD-A — Attacco molto diverso dalla direzione di cover
È ancora APERTO.

Caso:
- unità in crouched cover verso un lato;
- ability richiede AttackFacing molto distante, anche 120°/180°.

Da decidere tramite scenario/playtest:
- perde cover solo durante pivot/fire/recover?
- torna automaticamente alla cover originale?
- l'attacco rompe definitivamente la cover?
- esistono policy ability-specific?
- quale costo/reaction opportunity produce un pivot grande?

NON promuovere a canonica la proposta “pivot, fire, pivot back gratis”.

## OD-B — Ruolo runtime dei 12 spicchi
LOCKED come **grammatica di footprint/authoring/visual solid**.
Da verificare se diventano anche un `PlacementSector` gameplay selezionabile.

Non introdurre automaticamente:
- FVector intra-cell authority;
- mini-navmesh;
- actor per spicchio;
- 12 posizioni unità;
- secondo pathfinder.

## OD-C — CoverAnchor
Definire quali geometrie richiedono realmente un CoverAnchor e quali usano solo crouch presentation.

## OD-D — Forced displacement / cover pose / placement
Riconciliare con gli owner già aperti.

---

# 4. Aggiornamenti documentali richiesti

Individua e aggiorna gli owner reali per:

1. Map/Grid authoring e graybox placement;
2. Cover;
3. Facing/Pivot;
4. Animation/Presentation contract;
5. Overwatch/Fast Reaction;
6. UI/world tactical overlay;
7. Decision Log / ADR / Open Decisions;
8. Wiki gameplay;
9. eventuale glossary.

Inserisci l'SVG in una pagina owner, evitando duplicazioni.

Aggiungi una legenda testuale che distingua chiaramente:

`12 footprint wedges`
`6 Facing sectors`
`core / safe placement`
`edge`
`low cover`
`high wall`
`visual solid`
`logical occupancy`

---

# 5. Roadmap / Epic / Issue reconciliation

NON creare una roadmap parallela.

Mappa il lavoro sulle release canoniche live. Come seed:

### v0.1 / vertical slice
- diagramma e authoring grammar;
- footprint data minimo;
- debug overlay spicchi/core/edge;
- generic crouch cover presentation;
- Facing 6-dir coerente;
- basic RiseToFire exposure contract;
- Overwatch standing-ready/no-low-cover presentation;
- scenario DevSandbox e Automation tests minimi.

### v0.2
- authoring tool/preset riutilizzabili per rocce/muri/casse;
- validator footprint/core/edge;
- CoverAnchor solo dove deciso;
- advanced cover animation policies;
- open decision AttackFacing vs CoverFacing risolta;
- bot/UI baseline.

### v0.3+
- TeamKnowledge/privacy;
- multilivello;
- online authority;
- replay/TurnLog hardening;
- data/balance metrics;
- packaged validation;
- dedicated;
- freeze/production hardening.

Questa è una seed roadmap: adattala alle release correnti senza inventare nuove release.

Per ogni work item:
- owner Epic/Feature;
- milestone/release;
- dipendenze;
- acceptance criteria;
- scenario;
- Automation/Functional/PIE test;
- privacy/determinism note;
- evidence richiesta;
- link docs/ADR.

---

# 6. Scenari obbligatori

Aggiungi o aggiorna scenari equivalenti a:

### HEX-FP-01 — Partial lateral rock
Cella con spicchi laterali occupati, core/safe placement libero.
Expected:
- cella occupabile;
- no mesh intersection nella presentation;
- cover solo se definita dalla cover geometry;
- pathfinding ancora cell-level.

### HEX-FP-02 — Corner rock
Solido su spicchi che toccano due lati.
Expected:
- footprint leggibile;
- cover sides coerenti;
- no auto-cover da occupied mask.

### HEX-FP-03 — Core blocked
Core/safe placement non valido.
Expected:
- cella non occupabile;
- A* non la sceglie come destination;
- debug reason code.

### HEX-FP-04 — Low wall edge
Expected:
- cella occupabile;
- low cover direzionale;
- generic crouch presentation;
- mesh non authority.

### HEX-FP-05 — High wall edge
Expected:
- edge block secondo regole;
- LOS/projectile/movement policy coerente;
- Overwatch non “vede attraverso” il muro.

### COVER-ANIM-01 — Generic crouch
TakeCover -> crouch, stessa cella/Facing coerente.

### COVER-ANIM-02 — RiseToFire exposure
Crouch -> rise -> attack -> recover.
Expected:
- `Exposed` logico durante la finestra prevista;
- replay identico;
- nessun Animation Notify decide l'esito.

### OW-POSE-01 — Overwatch from cover
TakeCover -> arm Overwatch.
Expected:
- unità passa standing ready;
- no Low Cover benefit per tutta Overwatch;
- HOLD rimane ready;
- FIRE non esegue RiseToFire;
- High Wall continua a bloccare.

### FACING-POSE-01 — ±30° pose
Target dentro stesso settore:
- pose/body può orientarsi entro ±30°;
- no torso-only aim;
- nessun Facing extra.

### OPEN-SCENARIO — AttackFacing far from CoverFacing
Creare scenario discriminante ma mantenerlo collegato a Open Decision finché non approvato.

---

# 7. Validator / Debug richiesti

Se esistono owner/tool adatti, pianifica o aggiorna:

- overlay `W0..W11`;
- core/safe placement;
- 6 Facing sectors;
- edge types;
- occupied mask;
- Low/High cover mask;
- cell standable reason;
- validation: footprint fuori costruzione;
- validation: core bloccato ma cell marked standable;
- validation: cover edge senza owner geometry;
- deterministic serialization/hash del footprint se runtime-authoritative;
- editor preview di rotazione/snap dell'oggetto.

Non introdurre Actor per spicchio.

---

# 8. Output finale richiesto a Claude

Produrre un report con:

1. branch/HEAD verificati;
2. file letti;
3. conflitti trovati;
4. matrice `REUSE / UPDATE / CREATE / DEFER / DECISION_REQUIRED`;
5. decisioni promosse a LOCKED e path dell'ADR/Decision Log;
6. Open Decisions aggiornate;
7. Epic/Issue aggiornate o create con link reali;
8. roadmap/milestone aggiornate;
9. scenari/test aggiunti;
10. path dell'SVG integrato;
11. validator/debug overlay pianificati o implementati;
12. file modificati;
13. test/lint/validator eseguiti e risultati;
14. gap residui;
15. proposta commit Git.

Obiettivo di chiusura:

`Decision -> Data/Spec -> Runtime Owner -> Presentation -> Roadmap -> Epic/Issue -> Scenario -> Test -> Evidence`

Non dichiarare “consolidato” se manca uno di questi collegamenti.
