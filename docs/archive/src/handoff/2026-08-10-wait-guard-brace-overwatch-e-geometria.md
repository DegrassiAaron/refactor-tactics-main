# RefactorTactics — Handoff per Claude
## Focus: Wait, Guard, Brace, Overwatch, Reaction Framework, Facing e geometria Hex/Wall

**Scopo del file**  
Questo documento consolida le decisioni e il brainstorming emersi nella chat corrente e deve essere usato da Claude come handoff operativo per aggiornare repository, roadmap, issue, documentazione tecnica e Wiki/Wikipedia interna di RefactorTactics.

**Regola di prevalenza**
1. Decisioni esplicite più recenti di questa chat.
2. Decisioni già consolidate nel repository.
3. PDR/documentazione esistente.
4. Proposte precedenti e brainstorming storico.

Quando esiste conflitto, **non mantenere due versioni ambigue**: segnare la decisione vecchia come superata/deprecata, aggiornare i riferimenti e riportare la decisione corrente come canonica o come `PLAYTEST REQUIRED` se non ancora chiusa.

---

# 1. Executive summary

Questa chat ha consolidato soprattutto cinque aree:

1. **Wait, Guard, Brace e Overwatch devono avere identità tattiche distinte.**
2. **Brace e Overwatch restano azioni/skill universali distinte per il giocatore**, ma condividono lo stesso Reaction Framework tecnico.
3. Durante la Resolution il giocatore non può modificare liberamente lo stato; può intervenire solo in **Decision Windows** esplicite, con risposte prevalidate dal server.
4. Il Facing usa 6 direzioni discrete sulla griglia hex, ma **non deve essere forzato ad allinearsi alla geometria reale dei muri**.
5. La griglia hex discretizza **posizioni e transizioni**, non la geometria fisica: muri a 90°, edifici rettangolari, porte e aperture possono attraversare liberamente la proiezione visiva degli hex.

Decisione chiave più recente:

> **Brace e Overwatch sono due azioni universali distinte, appartenenti alla stessa famiglia di Prepared Reactions e implementate sullo stesso framework. Devono restare due scelte visibili separate; la loro economia può essere comune, ma non vanno fuse in un unico pulsante "Prepare Reaction".**

---

# 2. Vocabolario da usare in documentazione

Usare in modo coerente i seguenti termini.

## Main / Common Actions
- Wait
- Guard
- Brace
- Overwatch
- Basic Attack
- Ability
- Move / movement profile

## Reaction Framework
- Prepared Reaction
- Reaction Definition
- Reaction Instance
- Reaction Trigger
- Reaction Opportunity
- Decision Window
- Legal Response
- HOLD
- Commit
- Automatic Response
- Single-responder Reaction
- Contested Reaction / Reaction Clash

## Spatial
- Cell Anchor
- Unit Footprint
- Clearance
- Standable Cell
- Tactical Graph Node
- Tactical Graph Transition / Edge
- Blocking Geometry
- Facing
- Incoming Direction
- Physical Cover
- LOS
- Trajectory

Non usare "hex occupato al 30%" o percentuali simili come regola competitiva.

---

# 3. WAIT

## 3.1 Identità

`Wait` è una **azione universale esplicita di No-Op**.

Significato:

> Il giocatore rinuncia volontariamente alla possibilità di eseguire quella Main Action.

Non significa necessariamente saltare l'intero turno.

Esempio concettuale:

```text
Main: Wait
Move: B4 -> C4
Final Facing: NE
```

Wait non deve automaticamente:
- dare armor;
- recuperare energia;
- aumentare accuracy;
- concedere reaction;
- produrre buff;
- generare risorse.

## 3.2 Wait e Move

Wait deve poter convivere con il normale Move finale, salvo regole specifiche del ruleset/personaggio.

## 3.3 Wait e Reaction

Wait non abilita una reaction da solo.

Una reaction già posseduta indipendentemente dalla Main Action può comunque attivarsi se i suoi requisiti restano validi.

Quindi:

```text
Wait != Brace
Wait != Guard
Wait != Overwatch
Wait != Hide
Wait != Stealth
```

## 3.4 Wait come fallback

Distinguere:
- `PlayerSelected`
- `TimeoutFallback`
- `InvalidIntentFallback`
- `DisconnectedFallback`
- eventuali altre policy.

Nel TurnLog la ragione deve essere esplicita.

Possibile evento:

```text
ActionWaited
- UnitId
- Slot
- Reason
```

---

# 4. GUARD

## 4.1 Identità

Baseline di design:

> **Guard protegge il corpo / mitiga il danno o l'impatto diretto.**

Non deve diventare automaticamente:
- Brace;
- Counter;
- Intercept;
- Interpose;
- Parry.

## 4.2 Guard baseline

Proposta corrente per il playtest:

- Guard viene scelto nel Planning.
- È una Prepared Response.
- Il Guard base è **automatico**, senza Decision Window.
- Questo evita di trasformare ogni attacco in un prompt.
- Usa un `GuardProfile` character-specific.

## 4.3 Guard e Facing

Guard usa il Facing dell'unità.

**Non introdurre un `GuardDirection` indipendente**.

Baseline:

```text
FacingBinding = FollowCurrentFacing
```

In futuro può esistere:

```text
LockFacingWhenArmed
```

per stance speciali, torrette, scudi piantati ecc.

## 4.4 Guard e Cover

Cover fisica e Guard sono sistemi distinti.

- Cover = proprietà geometrica della mappa.
- Guard = stato/protezione dell'unità.

Un muro continua a proteggere geometricamente indipendentemente da dove guarda il personaggio.

Guard può invece essere frontale/laterale/posteriore rispetto al Facing.

Guard baseline **non crea e non aumenta automaticamente Cover**.

Se Bastion o un altro personaggio crea/fortifica una vera copertura, deve essere un effetto esplicito che modifica MapState/geometry/cover, non la semantica universale di Guard.

## 4.5 Guard vs Brace

Identità sintetica da mantenere:

```text
GUARD
"Se mi colpisci, reggo meglio il colpo."

BRACE
"Se provi a spostarmi o alterare la mia geometria,
posso reagire."
```

Esempio di test:

```text
Pressure Jet:
Damage + Push

Guard:
mitiga la componente di danno
Push invariato

Brace:
danno invariato
modifica / contesta il Push
```

Questo è un modello di design da playtestare; i valori numerici non sono ancora canonici.

---

# 5. BRACE

## 5.1 Identità

Brace è una **azione universale distinta**.

Baseline:

> Brace prepara una Defensive Reaction orientata a displacement, posizione, facing, stabilità e geometria.

Brace non deve automaticamente:
- ridurre danno generico;
- dare armor globale;
- annullare Stun;
- essere equivalente a Guard.

## 5.2 Lifecycle

```text
Planning
-> Brace selected
-> Reaction armed
-> valid trigger occurs
-> Reaction Opportunity
-> Decision Window
-> legal response / HOLD
-> server validates
-> resolver applies outcome
```

## 5.3 Bastion — profilo di playtest

Le maneuver discusse per il prototipo sono:

### Hold Ground
Grammar: `COMMIT`

Idea:
- difendere la cella;
- assorbire displacement;
- tentare di preservare facing.

### Shield Read
Grammar: `READ`

Idea:
- leggere il vettore della minaccia;
- orientare la difesa verso la sorgente;
- può cambiare Facing come parte della risposta.

### Pivot Step
Grammar: `SHIFT`

Idea:
- accettare di cedere geometria in modo controllato;
- scegliere una destinazione valida;
- impostare Facing tramite una policy esplicita.

## 5.4 Facing policy per response

Policy proposte:

```text
Preserve
FaceThreat
FaceMovement
```

Importante:

> Non esiste una correzione libera del Facing dopo aver visto l'esito.

Un cambio Facing è legale se deriva da:
- Planning;
- Move;
- Fast Action;
- Fast Reaction;
- Reaction Clash maneuver;
- ability/effect.

---

# 6. OVERWATCH

## 6.1 Identità

Overwatch è una **azione universale distinta da Brace**.

Baseline:

> Prepara una reaction di controllo dello spazio e punisce un evento futuro in una zona/direzione preparata.

Non è una sottoskill di Brace.

Non è corretto fondere:

```text
Brace
└── Overwatch
```

o:

```text
Overwatch
└── Brace
```

## 6.2 Planning vs Reaction

Overwatch ha due momenti di agency.

### Planning
Il giocatore decide la scommessa tattica:
- profilo;
- area / direzione;
- facing;
- eventuale shape;
- eventuale policy specifica.

### Resolution / Fast Reaction
Il giocatore decide se consumare l'opportunità concreta.

Baseline MVP:

```text
[FIRE]
[HOLD]
```

Con target simultanei:

```text
[FIRE A]
[FIRE B]
[HOLD]
```

Non creare prompt sequenziali artificiali se A e B triggerano nello stesso micro-step.

## 6.3 HOLD

HOLD significa solamente:

> Non consumo la reaction su questa Opportunity.

HOLD non deve:
- ruotare il personaggio;
- modificare il cono;
- dare bonus;
- cambiare trigger;
- riposizionare l'unità.

## 6.4 Facing

La normale risposta FIRE non concede rotazione gratuita.

Una variante può offrire una response esplicita, per esempio:

```text
Snap Fire
Step + Fire
Push Left
Push Right
Intercept
```

ma solo se la maneuver è prevista dal profilo preparato.

## 6.5 Overwatch non è un menu di ability durante la Resolution

Non fare:

```text
Enemy enters
-> scegli qualsiasi ability del kit
```

Le opzioni della Fast Reaction devono essere maneuver appartenenti alla reaction già preparata.

---

# 7. BRACE VS OVERWATCH — DECISIONE CANONICA DA CONSOLIDARE

## 7.1 Decisione

**Tenere Brace e Overwatch distinti come azioni/skill visibili al giocatore.**

Motivi:
- intenzioni tattiche molto diverse;
- maggiore leggibilità del Planning;
- migliore onboarding;
- ghost/UI alleata più chiara;
- facilita il mindgame;
- evita un menu generico "Prepare Reaction" con molte sottovoci.

## 7.2 Ma stesso framework

Architetturalmente devono condividere:

```text
Reaction Definition
Reaction Instance
Trigger evaluation
Reaction Opportunity
Decision Window
Legal Response
HOLD
Charges
Expiration
Priority
Server validation
Networking/privacy
TurnLog
```

Cambiano:
- Definition;
- trigger;
- geometry;
- facing rules;
- legal responses;
- effects.

## 7.3 Stessa famiglia UI

Possibile UI:

```text
PREPARED ACTIONS
[ BRACE ] [ OVERWATCH ]
```

Sono due pulsanti distinti ma appartengono visivamente alla stessa famiglia.

## 7.4 Non confondere skill identity con action economy

Essere due azioni distinte non implica costi diversi.

È possibile che entrambe competano per lo stesso `Main Commitment`.

Questa parte deve essere ancora validata dal playtest e non va spacciata per definitivamente chiusa se il repository non contiene già una decisione esplicita.

---

# 8. REACTION FRAMEWORK

## 8.1 Tre modalità

Usare un solo framework con tre modalità:

```text
REACTION
├── Automatic
├── Single Responder
└── Contested / Clash
```

Esempi:

```text
Guard baseline -> Automatic
Overwatch -> Single Responder
Brace -> Single Responder
Reaction Clash -> Contested
```

## 8.2 Decision Boundary

Durante la Resolution il gameplay logico avanza deterministicamente.

Se compare una Decision Window:

```text
Simulation reaches boundary
-> authoritative logic pauses
-> server builds legal responses
-> authorized client chooses
-> server validates
-> resolver applies
-> simulation resumes
```

Il client non può inventare risposta, target, destinazione o facing.

## 8.3 Legal Response

Una response può includere:

```text
ResponseId
Grammar
OptionalTarget
OptionalDestination
FacingPolicy
EffectProfile
```

Il client seleziona una response server-authorized.

---

# 9. REACTION CLASH

## 9.1 Quando esiste

Non ogni coppia di reaction crea un Clash.

Regola:

> Un Reaction Clash nasce quando due partecipanti hanno contemporaneamente agency significativa sullo stesso outcome tattico.

Quindi:
- due Overwatch alleate sullo stesso target: non per forza Clash;
- attacco già committed + difensore con Brace: normalmente single-responder;
- attacker con scelta live + defender con scelta live sullo stesso contatto: Clash.

## 9.2 Niente nested reaction stack

Non creare:

```text
Reaction
-> Counter Reaction
-> Counter Counter Reaction
...
```

Se due agency live insistono sullo stesso boundary:

```text
collect legal agency
-> one contested opportunity
-> both choose
-> reveal
-> resolve once
```

## 9.3 Grammar di playtest

Proposta:

```text
READ > COMMIT
COMMIT > SHIFT
SHIFT > READ
```

Questa grammatica è **PLAYTEST REQUIRED**, non da dichiarare definitiva.

La UI dovrebbe usare nomi tematici:
- Hold Ground
- Shield Read
- Pivot Step
- Drive Current
- Read the Anchor
- Flow Vector

e mantenere COMMIT/READ/SHIFT sotto il cofano.

## 9.4 Prototype REACT-010

Fixture proposta:

```text
Riva Pressure Jet
vs
Bastion Brace
```

Riva:
- Drive Current = COMMIT
- Read the Anchor = READ
- Flow Vector = SHIFT

Bastion:
- Hold Ground = COMMIT
- Shield Read = READ
- Pivot Step = SHIFT

Il Clash deve modificare soprattutto:
- displacement;
- destination;
- facing;
- geometria;

senza necessariamente decidere se il danno base esiste.

## 9.5 Hidden simultaneous choice

Online:
- ogni client riceve solo le proprie legal responses;
- nessun leak della scelta avversaria;
- nessun "opponent locked" se può creare leak temporale;
- reveal solo dopo lock di entrambe le risposte;
- nessun secondo round su tie.

---

# 10. AGENCY DEL GIOCATORE DURANTE IL TURNO

## 10.1 Planning

Agency ampia:
- Action;
- Ability;
- Target;
- AoE;
- Move;
- Destination;
- Facing;
- Overwatch setup;
- Brace setup;
- labels/ping;
- Ready.

## 10.2 Resolution normale

Nessun input arbitrario.

Il giocatore non può:
- cambiare path;
- cambiare target;
- ruotarsi liberamente;
- fermarsi "a mano";
- cambiare ability;

salvo Decision Window o effetto che lo consente.

## 10.3 Fast Action / Fast Reaction / Clash

Agency breve e limitata alle legal responses.

Principio canonico:

> Il giocatore può modificare posizione, facing, target o comportamento durante la Resolution solo se una Decision Window esplicitamente generata dal resolver concede quella scelta.

---

# 11. ACTION ECONOMY — STATO ATTUALE

Questa parte è ancora parzialmente aperta.

Modello concettuale:

```text
TURN PLAN
├── Main Commitment
└── Move
```

Main possibili da valutare:
- Wait
- Basic Attack
- Ability
- Guard
- Brace
- Overwatch

Move resta la fase volontaria finale.

È stata proposta una matrice di playtest in cui:
- Wait e Basic Attack consentono Sneak / Normal / Sprint;
- Brace e Overwatch consentono Sneak / Normal ma non Sprint.

**Non rendere questa matrice canonica senza verificare il repository e i playtest.**

Non introdurre ancora percentuali o riduzioni numeriche del Move se non necessarie.

Decisione più sicura da consolidare subito:

> Brace e Overwatch possono condividere lo stesso costo/slot pur restando skill distinte.

---

# 12. FACING SULLA GRIGLIA ESAGONALE

## 12.1 Direzioni discrete

Facing logico:

```text
N
NE
SE
S
SW
NW
```

Idealmente ordinato circolarmente:

```text
N  = 0
NE = 1
SE = 2
S  = 3
SW = 4
NW = 5
```

Questo permette operazioni deterministiche:
- rotate +1 / -1;
- opposite +3;
- classificazione di incoming direction.

## 12.2 Facing non è geometria del muro

Il personaggio può guardare in una delle sei direzioni tattiche anche davanti a un muro:
- a 0°;
- a 90°;
- a 45°;
- a un altro angolo scelto dal level design.

Il Facing descrive l'orientamento tattico del personaggio, **non l'allineamento fisico con il muro**.

---

# 13. CORREZIONE FONDAMENTALE: MURI E HEX

La chat ha corretto un assunto precedente.

**NON assumere che un muro occupi un lato dell'hex.**

Gli edifici devono poter avere:
- muri rettilinei;
- angoli a 90°;
- stanze rettangolari;
- porte reali;
- geometria non allineata agli hex.

La griglia viene sovrapposta alla geometria, non viceversa.

---

# 14. NUOVO MODELLO SPAZIALE: ANCHOR + FOOTPRINT + TRANSITION

## 14.1 Hex come posizione tattica

L'hex visuale non deve essere interpretato come "piastrella fisica completamente posseduta".

Un nodo del tactical graph rappresenta una **posizione discreta valida**.

Il punto principale è il `Cell Anchor`.

## 14.2 Standability

Una cella può essere parzialmente attraversata visivamente da un muro ed essere comunque valida.

Regola:

```text
Place UnitFootprint at CellAnchor
-> intersects blocking geometry?
   YES -> non-standable
   NO  -> standable
```

Non interessa la percentuale dell'hex che cade dentro/fuori dalla casa.

## 14.3 Wall through center

Se il muro passa troppo vicino all'anchor o attraverso il footprint standard:
- cella non standable.

Per MVP:
- non creare mezze celle;
- non introdurre SubCellId.

## 14.4 Node validity != Transition validity

Possibile:

```text
Cell A = valid
Cell B = valid
Transition A->B = blocked
```

Questo è fondamentale per muri arbitrari.

Il grafo separa:

```text
CanStandHere?
CanTraverseFromAtoB?
```

## 14.5 Swept clearance

Per una transizione non basta verificare start e goal.

Serve verificare il volume/corridoio attraversato dal footprint.

Quindi:
- due anchor validi possono essere separati da muro;
- passaggio invalidato se lo swept volume interseca blocking geometry.

## 14.6 Porte

Una porta non deve coincidere con un lato dell'hex.

Muro e apertura appartengono alla geometria reale.

Quando aperta, la porta può rendere disponibile una o più transizioni che attraversano quel varco.

Quando chiusa:
- tali transizioni diventano non valide;
- GraphRevision viene incrementata;
- path cache invalidata secondo il modello già previsto dal progetto.

---

# 15. CASE A 90° SU HEX GRID

Gli edifici devono restare geometricamente naturali.

Non trasformare:

```text
90°
```

in uno zig-zag di:
- 60°;
- 120°;
- lati dell'hex.

Un angolo di edificio a 90° può attraversare:
- hex validi;
- hex invalidi;
- transizioni;
- LOS;
- cover lines.

È il tactical graph che viene derivato/adattato alla geometria.

---

# 16. COVER, LOS E GEOMETRIA

Separare rigorosamente:

```text
Pathfinding
LOS
Targeting
Trajectory
Cover
Facing
```

Una transizione percorribile non implica LOS.

Una cella standable non implica una traiettoria libera.

Facing non decide se un muro esiste.

## Cover fisica

Cover deve dipendere da:
- attacker position;
- target position;
- geometry;
- obstruction/height;
- trajectory/LOS policy.

Non da `WallSide == Facing`.

## Guard

Guard può usare il Facing per determinare il settore protetto.

## Overwatch

Facing può contribuire a determinare il settore controllato, secondo il profilo.

---

# 17. UNIT FOOTPRINT E CLEARANCE

Per MVP usare almeno:

```text
StandardUnitClearance
```

In futuro si può introdurre:

```text
SmallUnit
StandardUnit
LargeUnit
```

La stessa cella può essere:
- valida per una Small Unit;
- non valida per una Large Unit.

Questo deve integrarsi con `UnitMovementProfileId` e la query A*.

---

# 18. MAP EDITOR / VALIDATOR

La nuova geometria richiede strumenti editor.

Aggiungere/aggiornare la progettazione di:

## Overlay
- Cell Anchor.
- Footprint / clearance.
- Standable valid/invalid.
- Transition valid/blocked.
- Wall/geometry intersection.
- Door transition.
- GraphRevision.

## Stati visuali suggeriti
- Green = valid.
- Red = invalid.
- Yellow = warning / low clearance.

## Validator
Esempi:
- anchor troppo vicino a blocking geometry;
- footprint intersects geometry;
- transition swept volume intersects blocking geometry;
- porta senza transizioni coerenti;
- geometria modifica traversal ma non incrementa revision;
- cella isolata involontariamente;
- narrow passage non compatibile con StandardUnitClearance;
- LOS/trajectory ambiguity vicino ad angolo.

Non usare percentuali di occupazione dell'hex come regola runtime.

---

# 19. RENDER / VISUAL EXPLAINER GIÀ PRODOTTO

In questa chat è stato prodotto un render tecnico che mostra:

1. cella parzialmente tagliata dal muro ma valida;
2. cella non valida perché muro/footprint collidono;
3. due celle valide ma transizione bloccata;
4. transizione valida attraverso una porta.

Il concetto da trasferire nella Wiki è:

> **Hexes represent tactical positions, not exact floor tiles. A wall can cut across an hex. Cell validity depends on anchor + unit clearance. Movement validity depends on the path between anchors.**

Creare una versione interna/definitiva del diagramma nella documentazione se il repository supporta immagini nella Wiki.

---

# 20. IMPATTI TECNICI UE5

Baseline documentale: UE 5.8, salvo diversa patch bloccata nel repository.

## Map

Aggiornare la semantica di `FRTCellId`:

```text
FRTCellId
= stable logical node identifier
```

Non:
```text
= physical hex polygon ownership
```

## Cell data

Assicurarsi che il modello possa rappresentare:
- WorldAnchor;
- elevation;
- Layer;
- Standability by unit profile;
- clearance data/ref;
- surface;
- occupation;
- hazard;
- tags.

## Transition data

La transition è first-class:
- From;
- To;
- enabled;
- transition type;
- physical cost;
- clearance requirements;
- blocker IDs / source geometry if useful;
- revision;
- door/gate dependency.

## Geometry baking/generation

Serve una pipeline futura:

```text
World Geometry
-> Sample Cell Anchors
-> Validate Footprints
-> Build Candidate Neighbor Links
-> Sweep Clearance
-> Build Tactical Graph
-> Validate
-> Save/Bake compact data
```

La simulazione competitiva non deve dipendere da physics/frame-time runtime per decidere questi dati.

---

# 21. TEST DA AGGIUNGERE / AGGIORNARE

## Reactions

### REACT-010
Pressure Jet vs Brace, matrice 3x3 COMMIT/READ/SHIFT.

Assert:
- hidden simultaneous choice;
- deterministic result;
- no nested stack;
- destination/facing legal;
- same snapshot + choices => same StateHash/LogHash.

### Overwatch simultaneous targets
Due nemici triggerano nello stesso micro-step:
- una sola Opportunity;
- `FIRE A / FIRE B / HOLD`;
- nessun vantaggio da iteration order.

### Brace spatial response
- Pivot Left disponibile solo se destination valida;
- Pivot Right non mostrata se bloccata;
- FacingPolicy applicata deterministicamente.

## Common actions

### Guard vs Brace
- high damage / no displacement;
- low damage / strong displacement;
- mixed damage + push.

### Wait
- player-selected;
- timeout fallback;
- invalid intent fallback;
- same logical No-Op but distinct reason codes.

## Map geometry

Aggiungere nuovi test/fixture:

### GEO-001 — Partial Hex Wall
Muro attraversa visivamente l'hex ma non il footprint:
- node valid.

### GEO-002 — Footprint Collision
Muro attraversa footprint:
- node invalid.

### GEO-003 — Valid Nodes / Blocked Edge
Due node validi, wall tra loro:
- transition invalid.

### GEO-004 — Door Opening
Due node validi, porta aperta:
- transition valid.
Porta chiusa:
- transition invalid;
- revision increment;
- cache invalidation.

### GEO-005 — 90 Degree Corner
Angolo di casa reale 90°:
- node standability corretta;
- transition correctness;
- LOS/trajectory test dedicato.

### GEO-006 — Narrow Clearance
Passaggio valido per SmallUnit, invalido per Standard/Large se introdotto.

---

# 22. ISSUE / EPIC DA CREARE O CONSOLIDARE

Claude deve **prima cercare issue/epic già esistenti e aggiornarle**, evitando duplicati.

## EPIC — Common Actions & Prepared Reactions

Possibili issue:

### Action.Wait canonical behavior
- explicit no-op;
- fallback reasons;
- TurnLog;
- tests.

### Action.Guard baseline
- GuardProfile;
- automatic baseline;
- Facing binding;
- owner-only protection;
- cover separation.

### Action.Brace
- Prepared Defensive Reaction;
- Hold Ground / Shield Read / Pivot prototype;
- legal responses;
- facing policies.

### Action.Overwatch
- distinct universal action;
- Planning setup;
- FIRE/HOLD;
- simultaneous target Opportunity;
- HOLD lifecycle.

### Reaction Framework
- Automatic / Single / Contested;
- Decision Window;
- legal response payload;
- deterministic ordering;
- privacy.

### REACT-010 Reaction Clash prototype
- Pressure Jet vs Brace;
- 3x3 matrix;
- hidden simultaneous selection;
- TurnLog and tests.

## EPIC — Facing & Directional Combat

Issue candidates:
- Direction6 canonical enum N/NE/SE/S/SW/NW.
- Relative incoming direction classification.
- Guard facing integration.
- Overwatch facing integration.
- facing changes from Decision Windows.
- no free post-resolution correction.

## EPIC — Geometry-derived Tactical Graph

Issue candidates:
- Cell Anchor + StandardUnitClearance.
- Standability baking.
- Candidate neighbor generation.
- Swept transition clearance.
- Wall / blocking geometry integration.
- Door transition state.
- 90° building corner fixture.
- GraphRevision/cache invalidation.
- map debug overlay.
- validation commandlet/tests.

## EPIC — Map Editor Geometry Validation

Issue candidates:
- green/red/yellow anchor overlay;
- footprint display;
- blocked/allowed transition display;
- wall intersection diagnostics;
- door diagnostics;
- clearance warnings;
- 90° building validation scenario.

---

# 23. ROADMAP — AGGIORNAMENTI RICHIESTI

Claude deve aggiornare la roadmap senza anticipare sistemi fuori milestone.

## F0 / Foundations

Se coerente con lo stato reale del repository:
- confermare `FRTCellId` come nodo logico;
- aggiornare graybox/path proof per separare node validity e transition validity;
- aggiungere almeno una fixture con blocking geometry;
- evitare di sviluppare tutta la collision baking production-ready se fuori scope.

## Map milestone successiva

Aggiungere esplicitamente:
- geometry-derived standability;
- transition clearance;
- wall at arbitrary/orthogonal angles;
- doors not bound to hex edges;
- 90° corners;
- graph revision;
- LOS/trajectory interaction.

## Reaction milestone

Aggiungere:
- Brace e Overwatch come azioni separate;
- shared Prepared Reaction framework;
- Guard automatic baseline;
- Decision Window;
- REACT-010;
- privacy tests.

Non introdurre un "Prepare Reaction" unico nell'UX come sostituto di Brace/Overwatch.

---

# 24. FEATURE MAP

Se esiste una Feature Map YAML/MD nel repository, consolidare almeno:

```text
Feature.CommonAction.Wait
Feature.CommonAction.Guard
Feature.Reaction.Brace
Feature.Reaction.Overwatch
Feature.Reaction.Framework
Feature.Reaction.DecisionWindow
Feature.Reaction.Clash
Feature.Facing.Direction6
Feature.Map.CellAnchor
Feature.Map.UnitClearance
Feature.Map.GeometryDerivedStandability
Feature.Map.TransitionClearance
Feature.Map.DoorTransition
Feature.Map.OrthogonalWalls
```

Stato:
- `decided`
- `playtest`
- `planned`
- `implemented`
deve riflettere il repository reale, non questa handoff da sola.

---

# 25. SCENARIO MAP

Aggiungere/consolidare scenari:

```text
ACTION-004 Guard vs Brace
ACTION-WAIT-001 Wait behavior/fallbacks
OW-001 Overwatch single trigger
OW-002 Overwatch HOLD then later trigger
OW-003 simultaneous targets
REACT-010 Reaction Clash 3x3
FACE-001 Guard sectors
FACE-002 Facing changed by reaction
GEO-001 Partial wall / valid anchor
GEO-002 Wall / invalid footprint
GEO-003 Valid nodes / blocked transition
GEO-004 Door open/closed
GEO-005 90-degree building corner
```

Ogni scenario deve avere:
- purpose;
- initial snapshot;
- inputs;
- expected TurnLog;
- expected final state;
- determinism assertions;
- visual/debug verification.

---

# 26. EDITOR MAP

Se esiste una Editor Map / editor task registry, aggiungere le attività manuali non automatizzabili:

- costruire una stanza rettangolare con angolo a 90°;
- sovrapporre la griglia hex;
- controllare anchor valid/invalid;
- piazzare una porta non allineata a un lato dell'hex;
- verificare transizioni con porta open/closed;
- controllare clearance overlay;
- controllare LOS/cover vicino all'angolo;
- validare readability della griglia quando il muro taglia l'hex;
- verificare ghost Facing/Guard/Overwatch in presenza di muri non allineati.

---

# 27. WIKI / “WIKIPEDIA” INTERNA — PRIORITÀ ALTA

Il consolidamento della Wiki è uno degli obiettivi principali di questo handoff.

Creare o aggiornare pagine chiare e cross-linkate:

## Common Actions
- Wait
- Guard
- Brace
- Overwatch

## Reaction System
- Reaction Framework
- Decision Window
- Reaction Opportunity
- HOLD
- Reaction Clash
- agency during Resolution

## Spatial Model
- Hex Grid Philosophy
- Cell Anchor
- Unit Footprint / Clearance
- Tactical Graph Node
- Transition
- Standability
- Doors
- Walls & Buildings
- 90-degree geometry

## Facing
- six directions;
- relationship with Guard;
- relationship with Overwatch;
- relationship with cover;
- explicit statement: facing does not need to align with walls.

## Cover / LOS / Trajectory
Spiegare chiaramente che sono servizi/logiche separate.

### Pagina consigliata di alto livello

**“Hex Grid vs World Geometry”**

Messaggio centrale:

> La griglia esagonale discretizza dove una unità può stare e come può transitare. Non vincola la forma di muri, stanze, edifici o porte.

Inserire almeno 4 esempi illustrati:
1. partial overlap valid;
2. footprint invalid;
3. valid nodes blocked transition;
4. open door allowed transition.

---

# 28. DOCUMENTI DA CONSOLIDARE

Claude deve cercare nel repository e aggiornare dove presenti:

- Common Actions Master;
- Reaction Master;
- Overwatch spec;
- Brace spec;
- Facing spec;
- Map/Grid spec;
- Pathfinding spec;
- Cover/LOS spec;
- Map Editor spec;
- Scenario catalog;
- Feature Map;
- Roadmap;
- Wiki;
- ADR / decision log;
- balance matrices solo dove realmente impattate.

In particolare cercare e correggere documentazione vecchia che afferma o implica:

```text
wall == hex side
```

oppure che usa una cover/map architecture dipendente obbligatoriamente dal bordo dell'esagono.

Queste affermazioni sono superate dalla decisione corrente.

---

# 29. ADR / DECISION LOG DA AGGIUNGERE

Creare decisioni formali, se il repository usa ADR.

## ADR — Brace and Overwatch remain distinct universal actions

Decision:
- separate player-visible actions;
- same Prepared Reaction framework;
- no generic Prepare Reaction menu as replacement.

## ADR — Hex grid does not constrain world wall geometry

Decision:
- hex = tactical node/anchor;
- arbitrary/orthogonal walls allowed;
- standability based on footprint clearance;
- transition based on swept clearance.

## ADR — Facing uses six tactical directions independent of wall orientation

Decision:
- Direction6;
- geometry can use other world rotations;
- no mandatory wall/facing alignment.

---

# 30. OPEN QUESTIONS — NON INVENTARE RISPOSTE

Questi punti restano da validare o playtestare:

1. Brace/Overwatch/Guard action economy definitiva.
2. Compatibilità con Sprint e Move profile.
3. Guard mitigation numbers.
4. Guard protected arc per personaggio.
5. Cover + Guard stacking.
6. COMMIT/READ/SHIFT come grammatica finale.
7. Win/Tie/Lose numerici del REACT-010.
8. Pivot destination/facing UX definitiva.
9. Standard unit clearance in metri.
10. Hex spacing/dimensioni finali.
11. Regole esatte LOS quando il raggio sfiora un angolo di muro.
12. Regole projectile/cover sui corner cases.
13. Come bakeare in modo definitivo il tactical graph dalla world geometry.
14. Supporto futuro Small/Standard/Large footprints.
15. Se alcune reaction persistono o terminano prima della fase Move.
16. Lifecycle definitivo di Overwatch rispetto a ogni macrofase.
17. Se Guard debba competere sempre con Main Commitment o usare altra economia.

Creare issue `design-decision` / `playtest-required` invece di scegliere arbitrariamente.

---

# 31. CLAUDE — PROCEDURA OPERATIVA RICHIESTA

Eseguire nell'ordine:

1. **Audit repository**
   - trovare roadmap;
   - feature map;
   - scenario map;
   - editor map;
   - Wiki;
   - Reaction/Overwatch/Brace/Guard docs;
   - Map/Grid/Path/LOS/Cover docs;
   - ADR;
   - issue/epic esistenti.

2. **Diff semantico**
   - identificare contraddizioni con questa handoff;
   - classificare `obsolete`, `needs-update`, `already-aligned`.

3. **Consolidare documentazione**
   - una sola regola canonica;
   - cross-link;
   - redirect/deprecation delle pagine vecchie;
   - aggiornare diagrammi.

4. **Aggiornare Wiki**
   - priorità alta;
   - pagine concise ma complete;
   - link a ADR e scenari;
   - visual examples dove possibile.

5. **Aggiornare Roadmap**
   - non duplicare milestone;
   - inserire lavori nel milestone corretto;
   - mantenere lo scope F0 rigoroso.

6. **Aggiornare Feature Map / Scenario Map / Editor Map**
   - stato basato sul repository reale.

7. **Creare o aggiornare Epic/Issue**
   - cercare prima esistenti;
   - evitare duplicati;
   - inserire acceptance criteria;
   - link a Wiki/ADR/scenario;
   - milestone e labels corrette.

8. **Creare/aggiornare test plan**
   - soprattutto GEO-* e REACT-*.

9. **Aggiornare changelog / decision log**
   - evidenziare le decisioni che superano documenti precedenti.

10. **Produrre report finale**
    - file modificati;
    - pagine Wiki modificate;
    - issue create/aggiornate;
    - epic create/aggiornate;
    - decisioni ancora aperte;
    - conflitti trovati;
    - test/scenari aggiunti;
    - link GitHub.

---

# 32. DEFINITION OF DONE PER QUESTO CONSOLIDAMENTO

Il lavoro di Claude è Done solo se:

- Brace e Overwatch non risultano più ambigui nella documentazione.
- È chiaro che sono skill/azioni distinte ma condividono il Reaction Framework.
- Wait/Guard/Brace/Overwatch hanno pagine/definizioni coerenti.
- Agency durante Resolution è documentata.
- Facing a 6 direzioni è documentato.
- Nessun documento canonico afferma che i muri debbano seguire i lati dell'hex.
- `Cell Anchor + Unit Footprint + Clearance` è documentato come base della standability.
- Node validity e Transition validity sono distinti.
- Porte e muri a 90° sono coperti.
- Roadmap è aggiornata.
- Feature/Scenario/Editor maps sono aggiornate se presenti.
- Epic/issue esistenti sono consolidati e le mancanti create.
- Wiki/Wikipedia interna è aggiornata e cross-linkata.
- Le decisioni aperte sono marcate come tali, non inventate.
- Sono elencati i test necessari.
- Non vengono introdotti sistemi fuori scope nel codice senza issue/decisione.

---

# 33. COMMIT SUGGERITI

Preferire commit focalizzati, ad esempio:

```text
docs(actions): consolidate wait guard brace and overwatch
docs(reactions): separate brace and overwatch on shared reaction framework
docs(map): define anchor clearance model for arbitrary wall geometry
docs(facing): define six-direction tactical facing
docs(wiki): consolidate common actions reactions and spatial model
docs(roadmap): add reaction and geometry-derived graph work
test(map): add geometry clearance scenario specifications
test(reactions): add REACT-010 scenario specification
chore(issues): consolidate reaction and map geometry backlog
```

Se il repository usa PR separati per docs/roadmap/issues, rispettare le convenzioni esistenti.

---

# 34. RISULTATO ATTESO

Dopo il consolidamento, una persona che legge la Wiki deve capire rapidamente:

- cosa fa Wait;
- cosa distingue Guard da Brace;
- cosa distingue Brace da Overwatch;
- perché Brace e Overwatch sono due azioni diverse;
- perché condividono lo stesso Reaction Framework;
- quando il giocatore ha agency durante la Resolution;
- come funziona il Facing;
- perché un muro a 90° non deve seguire la griglia hex;
- come si decide se una cella è standable;
- perché due celle standable possono avere una transizione bloccata;
- come porte, clearance, pathfinding, LOS e cover si integrano.

La documentazione deve parlare una sola lingua e non lasciare in giro vecchie assunzioni incompatibili.
