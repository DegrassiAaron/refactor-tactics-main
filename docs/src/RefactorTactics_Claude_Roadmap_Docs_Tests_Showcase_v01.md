# REFACTORTACTICS — PROMPT ESECUTIVO PER CLAUDE CODE
## Consolidamento documentazione, roadmap, test e implementazione della showcase v0.1 “Relay Basin”

Stai lavorando **direttamente nella repository RefactorTactics**.

Questa attività NON è una semplice implementazione isolata. Devi usare i file di handoff allegati come input di design, confrontarli con lo stato reale della repository e produrre un consolidamento coerente di:

1. documentazione;
2. roadmap;
3. backlog/issue tecniche;
4. test automatici e API del Test Harness;
5. scenario automatico/golden scenario della v0.1;
6. implementazione incrementale necessaria a rendere lo scenario eseguibile;
7. Definition of Done e acceptance criteria della vertical slice.

Non inventare lo stato della repository. Prima fai un audit reale.

---

# 0. FILE DA USARE COME INPUT

Usa tutti i file forniti con questa attività, con particolare priorità ai seguenti.

## Scenario showcase appena definito

- `RT_Showcase_Relay_v01_ScenarioSpec_Claude.md`
- `RT_Showcase_Relay_v01_ScenarioDraft.json`
- `mappa_tattica_del_bacino_relay.png`
- `a_high_detail_infographic_game_scenario_board_im.png`

Questi quattro file definiscono la **showcase v0.1 target** e devono essere consolidati nella repository.

Il `.md` è la specifica di design principale dello scenario.
Il `.json` è un **design draft**, NON uno schema runtime da implementare ciecamente.
Le immagini sono riferimenti visuali e NON devono prevalere sulle coordinate/logiche definite nella specifica testuale.

## Reaction / Fast Decision

- `RefactorTactics_Overwatch_FastReaction_Claude.md`

Questo documento è normativo per:
- Overwatch universale;
- Reaction Opportunity;
- Decision Boundary;
- `FIRE / HOLD`;
- timeout = HOLD;
- durata di riferimento della Fast Reaction = **3 secondi**;
- nessun leak di informazioni future;
- nessuna reaction interattiva annidata nell'MVP;
- trigger multipli valutati per micro-step;
- più bersagli nello stesso micro-step raggruppati nella stessa opportunity.

## Scenario Test Harness

- `Testing automatico Cloud.txt`

Nota: nel testo storico può comparire “Cloud”; il riferimento corretto è **Claude Code + Unreal Engine**.

Il Test Harness deve usare lo stesso percorso del gameplay reale:

```text
UI / Test Scenario / Bot / Replay
                ↓
             Intent
                ↓
             Commit
                ↓
            Snapshot
                ↓
            Resolver
                ↓
             TurnLog
                ↓
             State
```

Sono vietate scorciatoie come:

```text
Scenario -> SetActorLocation
Scenario -> ApplyDamage
Scenario -> mutazione diretta dello stato bypassando resolver
```

## Sequenza turno

- `Revisione sequenza turno.txt`

La sequenza operativa corrente da consolidare è:

```text
PLANNING
→ READY / COMMIT
→ PREP
→ DASH
→ BLAST
→ MOVE
→ CLEANUP
```

Il normale `Move` resta l'ultima azione volontaria standard.

Le finestre Fast Action / Fast Reaction NON sono una nuova macro-fase:
sono **Decision Boundary** che segmentano la Resolution.

## PDR / documentazione tecnica

Usa come riferimento architetturale:

- `RT_PDR_03_Architettura_UE5_v0.1.pdf`
- `RT_PDR_04_Networking_Privacy_v0.1.pdf`
- `RT_PDR_05_Simulazione_Deterministica_v0.1.pdf`
- `RT_PDR_06_Mappa_Pathfinding_v0.1.pdf`
- `RT_PDR_07_Abilita_Personaggi_GAS_v0.1.pdf`
- `RT_PDR_08_UI_UX_Coordinazione_v0.1.pdf`
- `RT_PDR_09_Dati_Validazione_Modding_v0.1.pdf`
- `RT_PDR_10_Roadmap_QA_Rischi_v0.1.pdf`
- `RT_PDR_11_Demo_v0.1.pdf`
- `RT_PDR_11_Demo_v0.1(1).pdf`
- `RefactorTactics – Idee Ruoli Characters.pdf`
- `RefactorTactics_Balance_Matrices_v0.1.xlsx`
- `RefactorTactics_Rumore_Claude.md`

ATTENZIONE:
alcuni PDR sono precedenti alle decisioni più recenti e possono contenere:
- roster vecchio;
- nomi vecchi;
- finestra reaction da 5 secondi;
- sequenze di turno superate;
- vecchie assunzioni sul grid;
- meccaniche successivamente modificate.

NON copiarli meccanicamente.

---

# 1. REGOLA DI PREVALENZA

Quando trovi conflitti usa questo ordine:

```text
1. Decisioni esplicite più recenti già consolidate nella repository
2. RT_Showcase_Relay_v01_ScenarioSpec_Claude.md
3. RT_Showcase_Relay_v01_ScenarioDraft.json
4. RefactorTactics_Overwatch_FastReaction_Claude.md
5. Revisione sequenza turno.txt
6. Testing automatico Cloud.txt
7. documentazione corrente della repository
8. PDR storici / PDF
9. immagini concettuali
```

Se la repository contiene una decisione più recente e chiaramente approvata rispetto all'handoff,
NON sovrascriverla silenziosamente.

Documenta il conflitto.

---

# 2. OBIETTIVO DELLA V0.1

La v0.1 deve essere una **vertical slice 2v2 osservabile e testabile automaticamente**.

Roster showcase:

```text
TEAM BLUE
- Flux
- Riva

TEAM RED
- Bastion
- Vektor
```

Mappa:

```text
L_Showcase_Relay
scenario id:
RT_Showcase_Relay_v01
```

Obiettivo:

```text
Objective.Relay
Cell = FRTCellId(0,0,0)
```

La partita showcase deve durare **8 turni scripted** e mostrare il maggior numero possibile di sistemi core senza introdurre feature artificiali dedicate alla demo.

La stessa partita deve essere utilizzabile come:

```text
Visual showcase
Regression scenario
Golden test
Determinism test
Resolver benchmark
Bug reproduction fixture
Packaged smoke test
```

---

# 3. AUDIT OBBLIGATORIO PRIMA DELLE MODIFICHE

Prima di modificare qualsiasi file:

1. leggi `CLAUDE.md`, `AGENTS.md`, README e guide repository;
2. individua la versione UE realmente bloccata;
3. individua roadmap e decision log correnti;
4. individua i documenti `docs/` effettivamente attivi;
5. individua lo Scenario/Test Harness già presente;
6. individua:
   - `FRTCellId`;
   - MapState;
   - pathfinding;
   - LOS;
   - targeting;
   - planning/intents;
   - TurnManager;
   - Snapshot;
   - ActionResolver;
   - TurnLog;
   - actions;
   - reactions;
   - surfaces/status;
   - objective system;
   - hero/content catalog;
7. individua tutti i test esistenti;
8. individua eventuali API/editor tooling già usate dagli scenari.

Produci una tabella:

| Area | Stato | File principali | Test presenti | Gap verso showcase |
|---|---|---|---|---|
| Hex map | READY/PARTIAL/MISSING | ... | ... | ... |
| Turn phases | ... | ... | ... | ... |
| Scenario Harness | ... | ... | ... | ... |
| Overwatch | ... | ... | ... | ... |
| Predictive Action | ... | ... | ... | ... |
| Environment | ... | ... | ... | ... |
| Objective | ... | ... | ... | ... |

Classificazione ammessa:

```text
READY
PARTIAL
MISSING
SUPERSEDED
BLOCKED
```

---

# 4. CONSOLIDAMENTO DOCUMENTAZIONE

Devi aggiornare la documentazione corrente della repository affinché descriva **lo stato reale e il target v0.1**, non la storia delle decisioni.

Non duplicare la stessa regola in cinque file diversi.

## 4.1 Documenti da consolidare almeno per dominio

Assicurati che la documentazione corrente abbia una fonte chiara per:

```text
Architecture
Turn Sequence
Actions
Reactions / Fast Decisions
Map / Hex / Graph
Terrain / Surfaces / Environment
Cover / Structures
Abilities / Heroes
Objective
UI / Planning / Ghosts
Scenario Harness / Automated Testing
Vertical Slice Showcase
Roadmap / QA
```

Se esistono file vecchi che raccontano sistemi superati:
- spostali in archive se la struttura repository lo prevede;
- oppure marcali esplicitamente `SUPERSEDED`;
- evita due fonti normative concorrenti.

## 4.2 Documento Showcase

Crea o aggiorna un documento equivalente a:

```text
docs/gameplay/showcase-v0.1-relay-basin.md
```

adattando il path alle convenzioni reali della repository.

Deve contenere:

- obiettivo della showcase;
- roster;
- mappa;
- terreni;
- strutture;
- objective;
- 8 turni;
- feature dimostrate per turno;
- expected events;
- expected final state;
- dipendenze;
- test associati;
- Definition of Done.

NON copiare nel documento finale note temporanee tipo:

```text
USE_CATALOG_CURRENT
W_OR_SW
choose valid cell
```

Queste sono istruzioni di handoff.

Risolvile contro il catalogo reale o dichiarale come issue ancora aperta.

---

# 5. ROADMAP — RIELABORAZIONE RICHIESTA

Non limitarti ad aggiungere una riga "implementare showcase".

La roadmap deve essere **ricalcolata in funzione delle dipendenze necessarie a far passare RT_Showcase_Relay_v01**.

Mantieni il principio:

> ogni incremento deve essere giocabile, osservabile, testabile e non rompere il determinismo.

Proponi una roadmap verticale simile a questa, ma adattala allo stato reale:

```text
S0 — Scenario Harness baseline
S1 — Relay Basin map fixture
S2 — Turn 1 playable
S3 — Predictive Action thin slice
S4 — Moving-target + Fire/Burning
S5 — Universal Overwatch / Fast Reaction
S6 — Smoke + Structures + GraphRevision
S7 — Interposition / redirect
S8 — Environmental payoff
S9 — Relay objective + 8-turn full scenario
S10 — Golden replay / repeat / packaged smoke
```

Per ogni tranche indica:

| ID | Deliverable | Dipendenze | Test | Exit gate |
|---|---|---|---|---|

Non usare stime temporali se la repository non le usa già.

---

# 6. TEST HARNESS E TEST API

Aggiorna e consolida l'API del Test Harness.

L'obiettivo è che Claude Code possa creare/modificare scenari testuali e Unreal li esegua attraverso il gameplay reale.

Supportare progressivamente:

```text
VISUAL
FAST
HEADLESS
```

Lo stesso scenario deve produrre gli stessi risultati logici.

## 6.1 Scenario API

Non adottare automaticamente `RT_Showcase_Relay_v01_ScenarioDraft.json`.

Prima confrontalo con lo schema realmente esistente.

Poi:

- estendi lo schema esistente;
- oppure proponi una migrazione compatibile;
- oppure crea uno schema nuovo SOLO se quello esistente è realmente insufficiente.

Lo scenario deve poter esprimere almeno:

```text
ScenarioId
SchemaVersion
Ruleset
MapId
Seed
Initial Units
Initial Surfaces
Initial Structures
Initial Objective State
Turns[]
Intents[]
ReactionPolicy[]
PreCommitValidation[]
Assertions[]
Expected Final State
```

## 6.2 Reaction Policy API

Lo scenario deve poter automatizzare Fast Decision reali.

Almeno:

```text
Hold
CommitFirstValid
HoldFirstThenCommit
CommitSpecificTarget
Timeout
```

Per il Turno 4 della showcase serve:

```text
Opportunity #1 -> Flux -> HOLD
Opportunity #2 -> Riva -> FIRE
```

Questa policy deve rispondere alla **vera Reaction Opportunity** del runtime.

Non bypassare la finestra.

In FAST/HEADLESS la decisione può essere immediata ma deve attraversare lo stesso contratto logico.

## 6.3 Assertion API

Consolida progressivamente assertion di dominio.

Necessarie per la showcase:

```text
TurnCompleted
UnitAtCell
UnitHasStatus
UnitNotHasStatus
UnitKO
SurfaceHasStatus
SurfaceNotHasStatus
EdgeEnabled
EdgeDisabled
GraphRevisionChanged
CoverExists
AbilityResolved
AbilityFizzled
EventExists
EventCount
ReactionOpportunityExists
ReactionResponseEquals
ReactionConsumed
PredictionWhiffed
OriginalTargetEquals
EffectiveTargetEquals
ObjectiveUpdated
MatchEnded
StateHashEquals
LogHashEquals
```

Non implementare un mega-framework se molte possono essere costruite sopra primitive comuni.

## 6.4 Report machine-readable

Target:

```text
Saved/RTTests/<ScenarioId>/<RunId>/
    result.json
    turnlog.jsonl
    state_initial.json
    state_final.json
```

`result.json` deve permettere a Claude Code di diagnosticare il test senza leggere migliaia di righe di Unreal Log.

Contenere almeno:

```text
schemaVersion
scenarioId
runId
result
engineVersion
projectVersion
rulesVersion
contentManifestHash
resolverConfigHash
seed
assertions passed/failed
failures[]
stateHash
logHash
duration
```

Ogni failure deve avere:

```text
assertion
expected
actual
turn
phase
microStep
source/unit/cell/event
reasonCode
```

---

# 7. MAPPA CANONICA DELLA SHOWCASE

Usa la mappa logica definita in `RT_Showcase_Relay_v01_ScenarioSpec_Claude.md`.

Coordinate:

```text
FRTCellId { X=q, Y=r, Layer=0 }
```

Shape iniziale: 45 celle.

```text
r=-3: q=-1..+1
r=-2: q=-2..+2
r=-1: q=-3..+3
r= 0: q=-4..+4
r=+1: q=-4..+4
r=+2: q=-3..+3
r=+3: q=-2..+2
```

Objective:

```text
Relay = (0,0,0)
```

Spawn:

```text
Flux    = (-4,0,0)
Riva    = (-4,1,0)
Bastion = ( 4,0,0)
Vektor  = ( 4,1,0)
```

Terreni da rappresentare:

```text
Floor
Smoke
HighGround
Fire
Rough
ShallowWater
Conductive
Ice
```

Elementi:

```text
Directional Cover
Bridge Edge
Interactive Gate Edge / structure
Relay objective
```

IMPORTANTE:
verifica che nessuna cella sia classificata contemporaneamente in due Surface incompatibili a causa di una svista del design draft.

Se serve una correzione topologica minima per rendere lo scenario realmente valido:
- documentala;
- aggiorna spec + JSON + test;
- non cambiarne lo scopo tattico.

---

# 8. SCENARIO GOLDEN — RT_Showcase_Relay_v01

Implementa o prepara lo scenario come **scripted scenario di 8 turni**.

Le azioni esatte devono essere adattate agli ID reali presenti nel repository.

NON creare nuovi alias solo per far combaciare questo file.

---

## TURN 1 — MAPPA / POSIZIONAMENTO

### Flux
- Move dallo spawn verso il centro.
- attraversa Smoke;
- chiude con Facing verso Est.

### Riva
- usa `FluidTrail`;
- percorre la Water lane;
- dimostra special movement / terrain setup.

### Bastion
- usa `KineticPanel`;
- crea cover direzionale verso il centro;
- avanza.

### Vektor
- raggiunge High Ground.

Dimostra:

```text
Planning
Ghost Path
Ready/Commit
Hex movement
Smoke
Dash
Water
High Ground
Facing
Cover
TurnLog
```

---

## TURN 2 — WET + PREDICTION

### Flux
- `ConductiveNode` verso la lane acqua/bridge.

### Riva
- `PressureJet` su Vektor.
- Damage + Wet + Push se legalmente valido.

### Bastion
- `Reconfigure` su KineticPanel.

### Vektor
- `InterceptShot` su una cella prevista.
- Riva NON la attraversa.
- risultato = `PredictionWhiffed`.

Dimostra:

```text
Wet
Push
Conductive setup
Cover reconfigure
Predictive Action
Prediction WHIFF
Reason code
```

---

## TURN 3 — MOVING TARGET / FIRE

### Flux
- `LinearDischarge` su Vektor.

### Vektor
- usa `PassingBlade` / movimento speciale;
- attraversa Fire prima del Blast;
- riceve Burning.

Flux deve risolvere il moving target secondo la policy reale definita dal catalogo.

### Riva
- `CircularTide`.

### Bastion
- azione difensiva / Brace equivalente realmente esistente.

Dimostra:

```text
Dash before Blast
Fire
Burning
Moving Target
Fallback policy
AoE
```

---

## TURN 4 — OVERWATCH

### Vektor
- sceglie Overwatch universale;
- Facing controlla l'area.

### Flux
- entra per primo nel cono.

Reaction:

```text
Target Flux
Response HOLD
```

### Riva
- entra successivamente nel cono.

Reaction:

```text
Target Riva
Response FIRE
```

Assert:

```text
HOLD does not consume charge
second opportunity exists
FIRE consumes charge
no future information leak
```

Dimostra:

```text
Reaction Opportunity
Decision Boundary
3-second Fast Reaction semantic
HOLD
FIRE
Facing
Micro-step trigger
```

---

## TURN 5 — SMOKE / STRUCTURE

### Riva
- `MistVeil`.

### Flux
- crea un draft volutamente borderline/invalidabile via Smoke;
- planning validator deve fornire reason;
- poi committa un'azione valida.

### Bastion
- `Interact` sulla struttura/Gate;
- modifica la topologia;
- `GraphRevision++`.

### Vektor
- `Deflection`.

Dimostra:

```text
Smoke
LOS/Target validation
Planning correction
Deflect
Interact
Graph change
Path cache invalidation
```

---

## TURN 6 — INTERPOSITION

### Flux
- attacca Vektor.

### Bastion
- `Interposition`;
- target originale = Vektor;
- effective target = Bastion.

Rivalidare rispetto a Bastion:

```text
LOS
trajectory
cover
```

Nessuna reaction interattiva annidata.

### Riva
- `PressureJet` su Bastion.

### Vektor
- attacco su Flux.

Dimostra:

```text
Intercept
Target redirect
Cover revalidation
Wet
Push
No nested reaction
```

---

## TURN 7 — COMBO AMBIENTALE

### Riva
- porta acqua su una zona Fire usando l'azione realmente disponibile.

Expected:

```text
Water + Fire -> Fire removed / trasformazione definita dal ruleset
```

### Flux
- usa il miglior attacco elettrico disponibile per mostrare la propagazione.

Expected:

```text
Water / Wet / Conductive
→ electric propagation
→ stable ordering
→ no duplicate hit from same propagation event
```

### Vektor
- Normal Move su Ice.

Expected:

```text
deterministic slide
```

### Bastion
- draft `Ram` attraverso Rough.

Expected:

```text
planning invalidation
reason = terrain/profile restriction
```

Poi correggere il draft prima del Commit.

Dimostra:

```text
Water + Fire
Electricity
Wet
Conductive
Ice
Rough
Planning validation
EnvironmentChanged
```

---

## TURN 8 — OBJECTIVE > KO

### Vektor
- `InterceptShot` su accesso previsto al Relay.

### Riva
- usa una rotta alternativa;
- non attraversa la cella prevista;
- `PredictionWhiffed`;
- termina sul Relay.

### Bastion
- tenta di contestare;
- non riesce per una causa reale del ruleset/path/costi/stato.

### Flux
- completa un'ultima azione offensiva;
- viene poi messo KO legalmente.

Cleanup:

```text
Flux = KO
Riva = alive on Relay
Relay scored/controlled by Blue
Blue wins
```

Dimostra:

```text
Prediction
Alternative path
KO
Objective
Cleanup
MatchEnded
Objective > deathmatch
```

---

# 9. MATRICE TEST DA PRODURRE

Crea o aggiorna una matrice equivalente:

| Test ID | Tipo | Feature | Scenario/Turn | Mode | Expected |
|---|---|---|---|---|---|
| RT.Scenario.Showcase.T1 | Functional | opening | T1 | Fast | PASS |
| RT.Scenario.Showcase.T2 | Functional | prediction | T2 | Fast | PASS |
| RT.Scenario.Showcase.T4 | Functional | overwatch | T4 | Fast | PASS |
| RT.Scenario.Showcase.T7 | Functional | environment | T7 | Fast | PASS |
| RT.Scenario.Showcase.Full | Golden | full match | T1-T8 | Fast | PASS |
| RT.Scenario.Showcase.Repeat | Determinism | hashes | Full | Headless | 0 divergence |
| RT.Scenario.Showcase.Visual | Smoke | presentation | Full | Visual | completes |
| RT.Scenario.Showcase.Packaged | Smoke | packaged | Full | Packaged | completes |

Aggiungi test core mirati quando una feature è troppo importante per essere verificata solo tramite scenario end-to-end.

---

# 10. TEST DI DETERMINISMO

La fixture completa deve registrare:

```text
RulesVersion
ContentManifestHash
ResolverConfigHash
Seed
InitialStateHash
FinalStateHash
LogHash
```

Test richiesti progressivamente:

```text
Repeat 10
Repeat 100
Repeat 1000
Permutation
Visual vs Fast logical equivalence
Fast vs Headless logical equivalence
```

Non è necessario partire subito da 1000 se il runtime non lo consente ancora in CI.

Definisci una progressione nella roadmap.

---

# 11. NETWORK / PRIVACY

La v0.1 scenario fixture deve essere compatibile con il futuro/attuale modello autorevole.

Non mettere intenti completi su:

```text
GameState
globally replicated Actor
AlwaysRelevant component
```

Quando lo scenario viene usato in test networking:

```text
Team Blue client
must never receive
Team Red private planning
```

Le Reaction Opportunity devono contenere solo informazioni valide nel boundary corrente.

Aggiungi alla roadmap un packaged canary/privacy test se non esiste.

---

# 12. UI / DEBUG DELLA SHOWCASE

La modalità Visual deve poter mostrare almeno:

```text
cell coordinates
surface
path
facing
target/AoE
cover
current phase
turn
objective
reaction opportunity
TurnLog
reason code
```

Non serve styling finale.

Serve leggibilità.

Il playback deve derivare dal TurnLog/eventi autorevoli.

---

# 13. ISSUE / BACKLOG

Dopo l'audit e il consolidamento, crea o aggiorna una lista di issue implementative.

Ogni issue deve avere:

```text
Title
Goal
Scope
Non-goals
Dependencies
Files/systems involved
Acceptance criteria
Tests
Definition of Done
Suggested commit
```

Ordinale secondo la roadmap reale, NON per dominio teorico.

Preferire vertical slice incrementali:

```text
scenario -> feature -> automated test -> visible result
```

invece di:

```text
implement all surfaces
implement all abilities
implement all UI
```

---

# 14. NON IMPLEMENTARE ORA SE NON NECESSARIO

Non allargare la v0.1 con:

```text
full Fog of War
full Noise system
pets/summons
full traps framework
matchmaking
progression
public modding
production dedicated infrastructure
advanced bot AI
full 4v4 production support
```

Il sistema Rumore deve essere documentato/roadmapped se necessario, ma NON diventare un blocker della showcase salvo che il codice corrente lo richieda.

---

# 15. CONFLITTI STORICI DA RISOLVERE

Controlla esplicitamente almeno questi punti.

## Reaction duration

Se trovi:

```text
5 seconds
```

in vecchi documenti demo, la decisione più recente per Fast Reaction baseline è:

```text
3 seconds
timeout = HOLD
```

Aggiorna la documentazione normativa.
Mantieni la vecchia informazione solo in archive/changelog se serve.

## Vecchio roster

Se PDR/demo vecchi usano:

```text
Aegis
Nyx
Drift
Vex
```

non considerarli roster v0.1 corrente.

La showcase corrente usa:

```text
Flux
Riva
Bastion
Vektor
```

Verifica però i cataloghi reali della repository prima di rinominare asset/codice.

## Turn sequence

Elimina o archivia sequenze arbitrarie incompatibili con:

```text
Prep
Dash
Blast
Move
```

Normal Move resta alla fine.

## Grid

La mappa corrente è hex assiale.

Non reintrodurre logica square-grid.

---

# 16. OUTPUT RICHIESTO DA CLAUDE

Alla fine dell'attività voglio:

## A. Audit

```text
docs/implementation/showcase-v01-audit.md
```

o posizione equivalente.

Contiene:
- stato repo;
- conflitti;
- gap;
- decisioni prese;
- decisioni ancora bloccate.

## B. Showcase spec consolidata

```text
docs/gameplay/showcase-v0.1-relay-basin.md
```

## C. Roadmap aggiornata

Aggiorna la roadmap esistente, NON crearne una concorrente se già esiste una fonte canonica.

## D. Test plan aggiornato

Aggiorna la documentazione test/QA esistente con:
- test API;
- scenario harness;
- scenario matrix;
- deterministic/golden tests;
- packaged tests.

## E. Scenario

Crea/adatta il vero scenario repository:

```text
RT_Showcase_Relay_v01
```

nel formato effettivamente supportato dal progetto.

## F. Codice

Implementa solo la tranche ragionevolmente completabile senza fare una mega-PR.

Se l'intera showcase richiede più milestone:
- implementa la prima tranche;
- crea backlog preciso per le successive;
- lascia la repository compilabile e i test verdi.

## G. Report finale

Produci:

```text
What changed
What is now canonical
What remains missing
Roadmap impact
Tests added/updated
Scenario progress T1..T8
Known risks
Next recommended issue
```

---

# 17. DEFINITION OF DONE PER OGNI FEATURE DELLA SHOWCASE

Una feature non è Done solo perché si vede in PIE.

Richiede:

1. passa dal gameplay/resolver reale;
2. non usa special-case dello scenario;
3. deterministica;
4. TurnLog/reason code;
5. test automatico;
6. debug/visualizzazione sufficiente;
7. compatibile con rete/authority prevista;
8. nessun leak di planning;
9. documentazione aggiornata;
10. packaged test quando il livello della feature lo richiede.

---

# 18. GUARDRAIL DI IMPLEMENTAZIONE

NON fare:

```text
if (ScenarioId == RT_Showcase_Relay_v01)
{
    // risultato forzato
}
```

NON fare:

```text
if (UnitId == Flux)
```

nel resolver per implementare una regola che dovrebbe essere data-driven.

NON fare:

```text
SetActorLocation
ApplyDamage
SetSurface
```

dal Test Harness per produrre un esito che il gameplay dovrebbe risolvere.

NON usare:
- frame timing;
- montage timing;
- ordine TMap/TSet;
- packet arrival order;
- random globale;

come autorità competitiva.

---

# 19. STRATEGIA DI LAVORO

Lavora in questo ordine:

```text
1. AUDIT
2. CONFLICT REPORT
3. DOC CONSOLIDATION PLAN
4. ROADMAP REPLAN
5. TEST API/HARNESS PLAN
6. SCENARIO SCHEMA ADAPTATION
7. FIRST IMPLEMENTABLE VERTICAL SLICE
8. AUTOMATION TEST
9. DOC UPDATE
10. FINAL REPORT
```

Non fare una grande riscrittura prima dell'audit.

Mantieni commit piccoli e focalizzati.

Commit suggeriti, adattati allo stato reale:

```text
docs: consolidate v0.1 showcase and turn/reaction rules
test: add Relay Basin scenario fixture schema
feat: add showcase map fixture and T1 automation
feat: add predictive action showcase slice
feat: add universal overwatch decision boundary
feat: add environment showcase interactions
feat: add Relay objective full scenario
test: add golden/repeat/packaged showcase coverage
```

---

# 20. CRITERIO FINALE

L'obiettivo non è "avere una demo scriptata che sembra funzionare".

L'obiettivo è poter eseguire:

```text
RT_Showcase_Relay_v01
```

e usare quella singola fixture per dimostrare e verificare che RefactorTactics possiede davvero:

```text
hex tactical map
simultaneous planning
deterministic resolution
Prep/Dash/Blast/Move
terrain interaction
water/electricity
fire
ice
rough
smoke
cover
structures
facing
moving-target policies
predictive actions
Overwatch / Fast Reaction
Interposition
objective play
KO
TurnLog explainability
automated testability
replay determinism
```

Quando un elemento non è ancora implementato, NON fingere che lo sia.

Mettilo nella roadmap con:
- dipendenza;
- test;
- exit gate;
- scenario turn che sblocca.

La showcase deve diventare la **spina dorsale verificabile della roadmap v0.1**.
