# RefactorTactics — Consolidamento Reaction System, Brace e Reaction Clash
## Handoff operativo per Claude Code

**Data:** 2026-08-09  
**Obiettivo:** portare nel repository le decisioni e le proposte emerse sul Reaction System, consolidandole con la documentazione esistente e aggiornando **Product Map / Product docs, Feature Map/Feature Registry, Roadmap, Scenario Map/Scenario Registry, Wiki, test e Decision Log** senza creare fonti concorrenti.

---

# 0. Regola primaria: audit prima di modificare

Prima di applicare qualunque modifica:

1. verifica repository, branch, HEAD e versione UE realmente bloccata;
2. leggi `CONTEXT_INDEX.md` se presente;
3. identifica le source of truth correnti;
4. confronta questa handoff con documentazione, codice, test, issue e scenari esistenti;
5. **non trattare questa handoff come prova di runtime implementato**;
6. non creare un secondo Feature Registry, una seconda Roadmap o un secondo Scenario Registry.

Fonti da cercare per prime, se presenti:

```text
docs/product/piano-canonico-mvp.md
docs/decisions/RT_PDR_00_Decision_Log.md
docs/DOC_CONFLICT_MATRIX.md
docs/OPEN_DECISIONS.md

docs/gameplay/spec-sequenza-turno.md
docs/gameplay/spec-motore-azioni-e4.md
docs/gameplay/spec-reazioni-componibili-cp55.md
docs/decisions/adr-0004-finestre-di-reazione.md
docs/gameplay/brief-overwatch-reazioni.md
docs/gameplay/brief-azioni-generiche-overwatch.md

docs/roadmap/feature-registry.yaml
docs/roadmap/feature-registry.json
docs/roadmap/feature-registry.md
docs/roadmap/roadmap-checkpoint.md
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-post-v0.1.md
docs/roadmap/v0.1-definition-of-done.md

Source/RefactorTactics/Turn/
Source/RefactorTactics/Tests/
Source/RefactorTactics/ScenarioHarness/
Scenarios/

docs/wiki/
docs/characters/
```

Se i path reali differiscono, usa quelli reali.

---

# 1. Stato delle decisioni da consolidare

Separare esplicitamente tre livelli.

## 1.1 APPROVED / CANONICAL — decisioni esplicite da preservare

### A. Brace come accesso al Reaction Profile

`Brace` non deve essere ridotto a un semplice buff difensivo fisso.

Significato di prodotto:

> **Brace significa preparare il personaggio a reagire agli eventi. Il modo in cui reagisce dipende dal suo Reaction Profile.**

La risposta base/universale di Brace è orientata a **tenere la posizione**.

Esempio concettuale:

```text
Brace
  -> Hold Ground
     -> prova a mantenere la cella
     -> resiste a Push / Pull / Knockback
     -> può preservare Facing / settore difensivo
```

Ma classi/personaggi differenti possono convertire Brace in risposte diverse, per esempio:

```text
Tank / Guardian
  -> Hold Ground
  -> Interpose
  -> Shield Counter

Skirmisher / Assassin
  -> Dodge
  -> Sidestep
  -> Break LOS

Engineer / Trapper
  -> Trigger Trap
  -> Deploy/Activate Device
  -> Redirect

Duelist
  -> Parry
  -> Counter
  -> Riposte
```

Quindi **Brace descrive la preparazione**, mentre la Reaction descrive **come quel personaggio risponde**.

Non tutte le reaction devono essere disponibili in ogni Opportunity: snapshot, trigger, stato corrente, costi e requisiti determinano le risposte legali.

---

### B. Reaction Clash / morra tattica

Aggiungere al Reaction System un sottotipo di Reaction Opportunity in cui **due partecipanti hanno agency nello stesso decision boundary**.

Entrambi effettuano una scelta **nascosta e simultanea**, quindi il server esegue reveal e risoluzione.

Flusso concettuale:

```text
Resolution event
   -> Reaction Opportunity
   -> opportunity is contested
   -> Reaction Clash
   -> Participant A chooses hidden
   -> Participant B chooses hidden
   -> both choices locked
   -> simultaneous reveal
   -> deterministic comparison
   -> outcome applied to real tactical state
   -> resolution resumes
```

Questa meccanica deve evocare la logica della **morra cinese**: semplice da capire, ma profonda per bluff, lettura dell'avversario, pattern, bait e rischio.

Non deve diventare un minigioco separato dal simulatore: il risultato deve modificare stato reale come posizione, Facing, damage, displacement, cover, status, risorse, expose, trap/device state, ecc.

---

### C. Reaction Clash non sostituisce tutte le Reaction

Il Reaction System deve mantenere più modalità di risoluzione:

```text
Reaction Opportunity
  -> Automatic / precommitted outcome
  -> Single-responder Fast Reaction
  -> Contested Reaction Clash
```

Esempio:

```text
Overwatch standard
  -> FIRE / HOLD
  -> Single responder

Brace vs incoming counterable action
  -> Reaction Clash, se entrambi hanno una scelta significativa

Trap automatica
  -> Automatic
```

Non trasformare ogni attacco in un Clash.

---

# 2. BASELINE ESISTENTE DA CONSERVARE

Consolidare senza regredire le regole già esistenti del Reaction System.

## Reaction Opportunity

Il concetto owner deve restare esplicito:

```text
Reaction Definition
+ prepared/player intent
+ current authoritative state/snapshot
+ trigger
= Reaction Opportunity
```

Una Opportunity contiene solo dati validi all'attuale boundary simulativo e non deve rivelare il futuro.

## Fast Reaction

Baseline corrente da verificare nella repo:

```text
FastReactionDuration ~= 3.0 s
Timeout conservativo per Overwatch = HOLD
```

La Fast Reaction non è una seconda fase di Planning.

## Overwatch

Conservare almeno:

- preparata nel Planning;
- reaction armata durante Resolution;
- trigger valutati al boundary corretto;
- HOLD perde solo l'opportunità corrente, non necessariamente tutta la reaction;
- trigger simultanei nello stesso micro-step vanno aggregati nello stesso opportunity set;
- niente informazione anticipata su future opportunities;
- no nested interactive reaction stack nell'MVP, salvo decisione successiva esplicita.

## Determinismo

Nessun risultato competitivo deve dipendere da:

- frame rate;
- timing animazioni;
- tick client;
- ordine implicito `TMap/TSet`;
- ordine di arrivo pacchetti.

## Privacy

Il client deve ricevere solo la Decision Window sanitizzata che è autorizzato a conoscere.

Non inviare:

- intenti futuri avversari;
- percorsi futuri;
- future Reaction Opportunities;
- scelte Clash dell'avversario prima del reveal.

---

# 3. PROPOSED FOR PLAYTEST — grammatica universale del Reaction Clash

La seguente parte è la grammatica progettata nella discussione. Consolidarla come **proposta di sistema/playtest**, non trasformarla silenziosamente in runtime canonico se la repository non contiene ancora approvazione formale.

## 3.1 Triade universale

Usare tre intenzioni astratte sotto il cofano:

```text
COMMIT
READ
SHIFT
```

Relazione ciclica proposta:

```text
READ   > COMMIT
COMMIT > SHIFT
SHIFT  > READ
```

### COMMIT

Significato:

> mantengo il piano, la posizione, la linea, lo spazio o la forza dell'azione.

Esempi:

- Hold Ground;
- Shield Block;
- Stand & Fire;
- Heavy Strike;
- Suppressive control;
- mantenere una porta/choke;
- resistere a displacement.

Debolezza concettuale: è leggibile/predicibile, quindi READ lo può battere.

### READ

Significato:

> anticipo o leggo la scelta avversaria.

Esempi:

- Parry;
- Counter;
- Predictive Counter;
- Tracking Shot;
- Intercept;
- Trigger Trap;
- Redirect;
- attendere il dodge.

Forte contro COMMIT, vulnerabile a SHIFT.

### SHIFT

Significato:

> cambio geometria, posizione, angolo, facing, timing o vettore.

Esempi:

- Dodge;
- Sidestep;
- Blink;
- Pivot;
- Flank;
- Roll;
- cambio lato cover.

Forte contro READ, vulnerabile a COMMIT/space control.

---

# 4. Matrice proposta

Matrice canonica di grammatica:

| A \\ B | COMMIT | READ | SHIFT |
|---|---|---|---|
| **COMMIT** | Tie | B Advantage | A Advantage |
| **READ** | A Advantage | Tie | B Advantage |
| **SHIFT** | B Advantage | A Advantage | Tie |

Regola di design proposta:

> **I personaggi possono cambiare maneuver, costo, trigger e payoff Win/Tie/Lose, ma non dovrebbero cambiare la relazione universale READ > COMMIT > SHIFT > READ.**

Motivazione: la grammatica deve restare imparabile anche con roster grande.

Se il playtest dimostra che la matrice universale è troppo rigida, registrare la decisione come OPEN; non introdurre eccezioni sparse per personaggio.

---

# 5. Reaction Maneuver

La UI non deve necessariamente mostrare i termini astratti come nomi principali.

Il personaggio espone maneuver tematiche che mappano a una grammar intent.

Esempio concettuale:

```text
Bastion
  Hold Ground        -> COMMIT
  Shield Read        -> READ
  Pivot Step         -> SHIFT

Vektor
  Stand & Fire       -> COMMIT
  Predictive Counter -> READ
  Sidestep           -> SHIFT
```

La stessa grammatica deve poter rappresentare anche:

- hacking;
- trappole;
- shield duel;
- melee;
- dispositivi;
- reaction ambientali;
- reposition.

---

# 6. Win / Tie / Lose non significa binary success/failure

Il confronto produce soltanto:

```text
Advantage A
Stalemate
Advantage B
```

Poi la `Reaction Maneuver` scelta definisce il payoff.

Ogni maneuver deve poter dichiarare concettualmente:

```text
WinOutcome
TieOutcome
LoseOutcome
```

Esempio tank:

```text
Hold Ground / COMMIT

Win:
  no displacement
  strong mitigation
  preserve facing

Tie:
  no displacement
  no extra mitigation

Lose:
  partial displacement resistance
  no mitigation
```

Esempio duelist/read:

```text
Predictive Counter / READ

Win:
  reduce/avoid impact
  counterattack
  attacker exposed

Tie:
  partial mitigation
  no counter

Lose:
  original action resolves
  possible self-exposure
```

Questa struttura deve permettere profili diversi:

```text
Tank:      Lose tollerabile
Duelist:   Win enorme / Lose pesante
Assassin:  Win = positional advantage / Lose = exposed
Controller:Win = geometry/control shift
```

---

# 7. Direzione come payload, non come grammatica

LEFT / CENTER / RIGHT non devono essere la grammatica universale.

La direzione è un parametro della maneuver quando serve.

Esempio:

```text
Maneuver = Dodge
Grammar  = SHIFT
Direction = Left
```

oppure:

```text
Maneuver = HoldGround
Grammar  = COMMIT
FacingDirection = NorthEast
```

Questo permette un sistema generico anche quando la scelta non è spaziale.

---

# 8. UX proposta

Obiettivo: **una gesture / un click per scelta**.

Evitare flow:

```text
scegli SHIFT
 -> scegli Dodge
 -> scegli direzione
 -> confirm
```

Preferire legal responses già materializzate dal server:

```text
[ <- DODGE LEFT ]   SHIFT
[ HOLD GROUND ]     COMMIT
[ COUNTER ]         READ
```

Baseline proposta:

```text
max visible choices: 2-3
window: circa 3 s
single lock
simultaneous reveal
no second round on tie
```

Il reveal può essere enfatizzato dalla presentation, ma non deve modificare il timing logico.

---

# 9. Hidden simultaneous selection

Per Reaction Clash online:

```text
A -> hidden choice -> Server
B -> hidden choice -> Server
Server waits/locks both
Server reveals simultaneously
Server resolves
```

Non trasmettere la scelta A a B o viceversa prima del reveal.

Valutare anche il leak temporale:

- non mostrare necessariamente il momento esatto in cui l'avversario ha lockato;
- non lasciare che packet timing/size riveli la maneuver;
- usare DTO distinti e sanitizzati.

Nel vertical slice offline/single-machine, mantenere comunque il modello dati compatibile con la futura authority di rete.

---

# 10. Timeout

Overwatch conserva la sua policy specifica (`HOLD` se vigente).

Per Clash ogni partecipante deve avere un fallback deterministico.

Esempi concettuali:

```text
Brace defender timeout -> Hold Ground / COMMIT
Attacker timeout        -> Continue Original Action / default maneuver
```

Niente scelta casuale.

Non usare il timeout per assegnare automaticamente la risposta ottimale.

---

# 11. Costi e commitment

Una maneuver con costo deve consumare la risorsa quando la scelta viene validamente lockata, anche se perde il Clash, salvo policy esplicitamente diversa.

Possibili costi:

- Reaction Charge;
- energy/resource;
- cooldown;
- trap/device charge;
- shield integrity;
- facing lock;
- movement/pivot restriction.

Obiettivo:

> evitare scelte senza rischio del tipo “provo comunque, tanto se perdo non costa nulla”.

Non introdurre una nuova resource universale se il progetto ha già un modello equivalente.

---

# 12. No prompt storm

Reaction Clash deve essere usato solo quando **entrambi i partecipanti hanno almeno due risposte strategicamente significative**.

Se la risposta è praticamente obbligata, preferire:

- automatic reaction;
- conditional/precommitted policy;
- single-responder Fast Reaction.

Non trasformare ogni basic attack in un Clash.

Conservare/aggiungere limiti:

- niente nested interactive Clash nell'MVP;
- MaxPrompts/ReactionBudget dove già previsto;
- aggregazione dei trigger simultanei;
- instrumentation per prompt frequency e tempo totale di resolution.

---

# 13. Modello dati concettuale da confrontare con il codice reale

NON copiare ciecamente questi nomi in C++.

Adattare alle strutture già presenti.

```text
ReactionOpportunity
  OpportunityId
  Type = SingleResponder | Clash | Automatic
  CurrentSimulationBoundary
  Participants[]
  LegalResponses[]
  Deadline
  Defaults[]
```

Per Clash:

```text
ReactionClashOpportunity
  ParticipantA
  ParticipantB
  A.LegalManeuvers[]
  B.LegalManeuvers[]
  Deadline
  DefaultA
  DefaultB
```

Maneuver:

```text
ReactionManeuver
  ManeuverId
  GrammarIntent = Commit | Read | Shift
  Cost
  OptionalDirection
  OptionalTarget
  WinOutcome
  TieOutcome
  LoseOutcome
```

Resolver:

```text
ChoiceA + ChoiceB
  -> compare GrammarIntent
  -> AdvantageA | Stalemate | AdvantageB
  -> resolve maneuver outcome payloads
  -> canonical TurnEvents
  -> apply logical state
  -> resume resolution
```

Preferire outcome/eventi espressi tramite primitive già esistenti, non callback arbitrarie non deterministiche.

---

# 14. Aggiornamento Product Map / Product docs

Prima trova la vera Product Map o owner equivalente.

NON crearne una seconda.

Se `docs/product/piano-canonico-mvp.md` è owner, aggiorna lì la capability di prodotto.

La mappa prodotto deve rappresentare almeno:

```text
Simultaneous Planning
  -> Prepared Reactions
  -> Reaction Opportunities
  -> Fast Reaction
  -> Contested Reaction / Reaction Clash
  -> Character Reaction Profiles
```

Messaggio prodotto da consolidare:

> RefactorTactics non risolve tutte le reazioni come bonus passivi. Alcune decisioni critiche diventano confronti reattivi brevi e simultanei, nei quali i personaggi traducono una grammatica comune in maneuver specifiche del proprio kit.

Aggiungere anche i benefici di prodotto:

- bluff;
- lettura dell'avversario;
- bait;
- commitment;
- identità dei personaggi;
- posizione/facing come risorse;
- decisioni brevi senza riaprire il Planning.

Rischi da registrare:

- prompt storm;
- aumento durata resolution;
- UI overload;
- vantaggio di latenza se networking non correttamente progettato;
- matrice troppo astratta o poco leggibile;
- matchup knowledge eccessiva se i personaggi alterano la matrice.

---

# 15. Aggiornamento Feature Map / Feature Registry

## Regola assoluta

Lo stato owner resta il Feature Registry canonico, se presente:

```text
docs/roadmap/feature-registry.yaml
```

La Feature Map umana è input/audit, non seconda source of truth.

Prima cerca FeatureId esistenti equivalenti.

Probabili capability già presenti:

```text
Reaction Opportunity
Decision Window / Fast Reaction
Overwatch
Composable / Prepared Reactions
Brace / Guard
Facing
```

Non creare duplicati.

Se mancano sub-feature, valuta ID stabili coerenti con il naming reale per concetti come:

```text
Reaction Clash
Clash Grammar
Reaction Maneuver Profile
Hidden Simultaneous Choice
Brace Reaction Profile
```

**Non usare questi nomi come ID definitivi senza audit del registry.**

Per ogni feature aggiornata registrare:

```text
FeatureId
Title
Area
Release / maturity
Status derivato dai gate
Dependencies
OwnerSpecs
RoadmapRef
Issues
Tests
Scenarios
WikiRefs
LastVerified
```

### Dipendenze concettuali Reaction Clash

```text
Reaction Opportunity
Decision Boundary / Fast Decision Window
Deterministic Resolver
TurnLog / reason codes
Facing, quando la maneuver lo usa
LOS/Targeting, quando richiesto
TeamKnowledge/Perception, quando il trigger dipende da info parziale
Networking/privacy, quando online
```

### Release policy

Non promuovere automaticamente Reaction Clash a `DONE` o a una release runtime precisa.

- documentazione e design possono essere consolidati ora;
- runtime va collocato dopo audit della roadmap;
- E14 / Decision Window è il naturale workstream da valutare, ma non gonfiare lo scope v0.1 senza decisione esplicita;
- se il Clash è troppo grande per v0.1, registrarlo come capability successiva che riusa E14.

---

# 16. Roadmap

Auditare prima `E14` e l'issue reale associata.

Lo stato noto dal corpus precedente era:

```text
E14 / #152
Decision Window / Fast Reaction / Overwatch
```

Verificare che sia ancora corretto a HEAD.

Aggiornare la roadmap in modo da distinguere:

```text
E14 baseline
  -> Reaction Opportunity
  -> Decision Window
  -> Overwatch single-responder
  -> Fast Reaction policies

Reaction Clash extension
  -> contested opportunity
  -> hidden simultaneous choice
  -> grammar/outcome resolver
  -> UI reveal
  -> scenario/playtest metrics
```

Non creare una nuova Epic se un checkpoint/sub-feature di E14 è sufficiente.

Creare nuova Epic solo se l'audit dimostra che il lavoro è realmente indipendente e troppo grande per E14.

---

# 17. Decision Log / ADR

Registrare le nuove decisioni usando il prossimo ID disponibile reale; non inventare un numero hard-coded se la sequenza è cambiata.

Decisioni da rappresentare separatamente:

### Decisione A — Brace usa il Reaction Profile

```text
Brace prepara una risposta reattiva; il comportamento effettivo è definito dal Reaction Profile del personaggio e dalle risposte legali dell'Opportunity.
```

### Decisione B — Reaction Clash

```text
Una Reaction Opportunity può essere contested: entrambi i partecipanti scelgono nascosto nello stesso boundary, quindi il server/replay rivela e risolve deterministicamente.
```

### Decisione C — grammatica COMMIT / READ / SHIFT

Stato suggerito:

```text
PROPOSED FOR PLAYTEST
```

finché non esiste una conferma esplicita ulteriore o un playtest che la promuove.

Non confondere una proposta di bilanciamento/grammatica con una decisione runtime già implementata.

---

# 18. Scenario Map / Scenario Registry

Prima inventaria scenari esistenti e riusa Stable ScenarioId quando equivalenti.

Categoria primaria prevista:

```text
Reactions
```

Metadata utili:

```text
FeatureIds
CharacterIds
Purpose
Milestone
ExecutionMode
AutomatedTest
RequiredCapabilities
```

## Scenari minimi da aggiungere o mappare

Usare nomi reali coerenti con il registry. Questi ID sono proposte se non esistono equivalenti.

### 1. Brace base

```text
Reaction.Brace.HoldGround
```

Dimostra:

- Brace preparato;
- trigger di displacement;
- Hold Ground come risposta base;
- position/facing outcome nel TurnLog.

### 2. Brace con Reaction Profile diverso

```text
Reaction.Brace.CharacterProfile
```

Dimostra che due personaggi con Brace possono avere risposte differenti senza hard-code nel sistema Brace.

### 3. Clash — READ batte COMMIT

```text
Reaction.Clash.ReadBeatsCommit
```

### 4. Clash — COMMIT batte SHIFT

```text
Reaction.Clash.CommitBeatsShift
```

### 5. Clash — SHIFT batte READ

```text
Reaction.Clash.ShiftBeatsRead
```

### 6. Clash — Tie

```text
Reaction.Clash.TieOutcome
```

Assert:

- nessun secondo round;
- TieOutcome applicato una volta sola;
- resolution riprende.

### 7. Hidden simultaneous choice

```text
Reaction.Clash.HiddenChoice
```

Assert:

- scelta A non influenza legal choices di B tramite ordine UI;
- entrambe vengono lockate prima del reveal;
- canonical event order stabile.

### 8. Timeout

```text
Reaction.Clash.TimeoutFallback
```

Assert fallback deterministico.

### 9. Direction payload

```text
Reaction.Clash.DirectionalShift
```

Dimostra:

```text
SHIFT + DodgeLeft
```

senza trasformare Left/Right nella grammatica universale.

### 10. Cost consumed on lock

```text
Reaction.Clash.CostCommitment
```

Assert che la risorsa sia consumata secondo policy anche se la maneuver perde.

### 11. No nesting

```text
Reaction.Clash.NoNestedInteractive
```

Assert che un Clash non apra ricorsivamente un'altra Decision Window interattiva nell'MVP.

### 12. Determinism repeat

```text
Reaction.Clash.Determinism
```

Ripetere N volte con stesso snapshot/seed/choices:

- StateHash identico;
- LogHash identico;
- event order identico.

### 13. Prompt budget / pacing

```text
Reaction.Clash.PromptBudget
```

Misurare:

- numero prompt per turno;
- tempo reale totale fermo in Decision Window;
- numero Clash per match;
- percentuale di scelte timeout;
- eventuale chain di opportunities.

### 14. Network privacy — futuro/quando disponibile

```text
Reaction.Clash.Network.HiddenChoice
```

Canary test:

- Team/Client B non riceve la scelta A prima del reveal;
- nessun payload futuro;
- nessun leak da proprietà replicate globali;
- test packaged quando il networking entra nella DoD applicabile.

---

# 19. Character mapping per playtest

Non inventare kit canonici se i character data owner non li contengono.

Se il roster corrente verificato è ancora:

```text
Flux
Riva
Bastion
Vektor
```

usare questi solo come fixture di playtest e segnare le maneuver non approvate come `PROPOSED`.

Mapping concettuale proposto:

| Character | COMMIT | READ | SHIFT |
|---|---|---|---|
| Bastion | Hold Ground | Shield Read | Pivot Step |
| Vektor | Stand & Fire | Predictive Counter | Sidestep |
| Riva | Anchor Current | Redirect | Slipstream |
| Flux | Ground | Reactive Capacitor | Arc Step |

Questi nomi non sono canonici finché non risultano nei dati/decision log reali.

Obiettivo del playtest:

- Bastion: profilo stabile, Lose tollerabile;
- Vektor: READ high-risk/high-reward;
- Riva: SHIFT con payoff ambientale;
- Flux: READ/counter che interagisce con resource/elettricità.

Non cambiare la matrice universale per differenziare i personaggi: differenziare i payoff.

---

# 20. UI / Wiki

Aggiornare le pagine utente appropriate per spiegare:

```text
Reaction
Prepared Reaction
Brace
Fast Reaction
Reaction Clash
```

La Wiki deve spiegare la grammatica senza duplicare numeri competitivi posseduti dai Data Assets/cataloghi.

Per Reaction Clash mostrare:

```text
READ > COMMIT > SHIFT > READ
```

solo se il Decision Log la promuove almeno a regola di playtest ufficiale.

UI requirements:

- 2-3 scelte massimo;
- un click;
- countdown chiaro;
- scelta lockata senza mostrare quella avversaria;
- reveal simultaneo;
- outcome leggibile;
- TurnLog spiega perché;
- non affidarsi solo al colore.

Aggiornare i blocchi `RT_FEATURE_STATUS` solo tramite il generatore/registry esistente.

---

# 21. TurnLog / explainability

Aggiungere o estendere reason/event model senza creare log paralleli.

Il log deve permettere di ricostruire almeno:

```text
ReactionOpportunityCreated
ReactionChoiceLocked A
ReactionChoiceLocked B
ReactionClashRevealed
ReactionClashCompared
ReactionOutcomeResolved
ReactionCostConsumed
ReactionExpired/Cancelled
```

Usare nomi/eventi reali se equivalenti esistono.

Il log canonico non deve dipendere dai timestamp realtime della UI.

Per debug/playtest deve essere possibile vedere:

```text
OpportunityId
participants
legal maneuver ids
chosen maneuver ids
GrammarIntent A/B
comparison result
Win/Tie/Lose branch
state changes
reason codes
```

Attenzione alla privacy: non pubblicare eventi di scelta prima del reveal quando online.

---

# 22. Test automatici minimi

Aggiungere test solo usando framework reali del repo.

## Pure/core tests

- matrix exhaustive 3x3;
- symmetry/anti-symmetry del vantaggio;
- tie same-intent;
- stable sorting participants/opportunities;
- timeout fallback;
- cost on lock;
- illegal maneuver rejected;
- stale OpportunityId rejected;
- same snapshot + same choices = same result;
- permutation test di container/order.

## Scenario/functional tests

Usare gli ScenarioId della sezione precedente o equivalenti esistenti.

## UI/PIE

- one-click selection;
- countdown;
- lock;
- simultaneous reveal;
- no accidental double-submit;
- timeout presentation;
- controller/remapping solo se già in scope.

## Network future

- hidden choice canary;
- packet/repl inspection;
- reconnect during window policy;
- disconnect fallback;
- spectator/replay policy.

---

# 23. Metriche di playtest

Aggiungere metriche di valutazione, senza usarle come correttezza automatica.

```text
ClashesPerMatch
ClashesPerTurn
AvgDecisionTime
TimeoutRate
ChoiceDistributionCommitReadShift
WinTieLoseDistribution
RepeatChoiceRate per player
PromptTimeTotal
ReactionPromptCount
```

Domande di design:

1. il giocatore capisce perché ha vinto/perso?
2. la scelta sembra una lettura dell'avversario o un guess casuale?
3. COMMIT/READ/SHIFT è intuitivo dopo 1-2 match?
4. i personaggi sembrano diversi pur usando la stessa matrice?
5. il Clash rallenta troppo la resolution?
6. le scelte sono davvero equilibrate o una domina?
7. il Tie è interessante senza richiedere un secondo prompt?
8. i payoff Lose sono troppo punitivi?

---

# 24. Product Map / Feature Map / Scenario Map — tracciabilità obbligatoria

Alla fine il repository deve poter rispondere:

```text
PRODUCT CAPABILITY
  Reaction Clash
       |
       v
FEATURE ID
       |
       +-> Owner Spec / Decision
       +-> Roadmap / Epic
       +-> Issue
       +-> Runtime
       +-> Automation Test
       +-> ScenarioId
       +-> Wiki
```

Non copiare manualmente stato tra mappe.

- Product Map = vista di prodotto/capability.
- Feature Registry = owner dello stato verificabile.
- Roadmap = vista temporale/esecutiva.
- Scenario Map/Registry = dimostrazione/test tramite Stable ScenarioId.
- Wiki = vista utente derivata/riferita.

---

# 25. Guardrail

- Non creare un secondo Reaction System parallelo.
- Non implementare Reaction Clash dentro singole abilità con `if CharacterId`.
- Non hard-codare coppie di personaggi.
- Non cambiare la matrice per singolo personaggio senza nuova decisione di sistema.
- Non usare animazioni come autorità.
- Non avanzare la simulazione durante un decision boundary non risolto.
- Non usare RNG per timeout o confronto base.
- Non aprire nested interactive reaction stack nell'MVP.
- Non esporre choice avversaria prima del reveal.
- Non inviare future opportunities.
- Non mettere canonical reaction choices in Actor globalmente replicati.
- Non trasformare ogni attack in Reaction Clash.
- Non trasformare PROPOSED in CANONICAL senza fonte.
- Non assegnare `DONE` senza gate reali.
- Non rinominare ScenarioId/FeatureId esistenti senza migrazione/redirect.
- Non duplicare valori competitivi nella Wiki.

---

# 26. Output finale richiesto a Claude

Restituisci un report strutturato.

## A. Audit

```text
Repository
Branch
HEAD
UE version
Product owner docs
Reaction owner specs
Decision log
Feature Registry owner
Roadmap owner
Scenario Registry/Harness
Wiki owner
Relevant runtime/tests
```

## B. Decisioni consolidate

Tabella:

```text
Decision
Status: CANONICAL | PROPOSED FOR PLAYTEST | OPEN
Owner
Conflicts found
Resolution/action
```

## C. File modificati

Per ogni file:

```text
Path
Why
What changed
Source of truth / generated view
```

## D. Product Map

Mostrare capability aggiunte/aggiornate e dipendenze.

## E. Feature Registry

Per ogni feature/sub-feature:

```text
FeatureId
Title
Release/Maturity
Status
Gates
Dependencies
RoadmapRef
Issue
OwnerSpec
Tests
Scenarios
WikiRefs
```

## F. Roadmap

```text
Epic/Checkpoint
FeatureIds
Scope now
Deferred scope
Dependencies
Exit gate
```

## G. Scenario Map

Tabella:

```text
ScenarioId
PrimaryCategory
FeatureIds
Characters
Purpose
Automated
Status
```

## H. Tests

```text
Test
Level
Feature
Expected
Status
```

## I. Wiki

- pagine aggiornate;
- FeatureId collegati;
- scenario links;
- eventuali blocchi generati;
- nessun numero competitivo duplicato.

## J. Open decisions

Elencare solo questioni realmente ancora aperte dopo audit, per esempio:

- COMMIT/READ/SHIFT viene promosso a canonical o resta playtest grammar?
- Reaction Clash entra nel runtime v0.1 o dopo la baseline E14?
- exact payoff Win/Tie/Lose per le maneuver dei personaggi;
- Reaction cost/budget definitivo;
- reveal presentation duration;
- disconnect/network timeout policy futura.

## K. Validation

Eseguire i comandi reali del repo, inclusi se ancora validi:

```bash
python scripts/feature_registry.py validate
python scripts/feature_registry.py generate --check
python scripts/feature_registry.py wiki --check
python scripts/feature_registry.py workbook --check
```

Poi build/test pertinenti realmente disponibili.

Non dichiarare successo se un comando non è stato eseguito o non esiste.

## L. Commit consigliati

Preferire commit focalizzati, per esempio:

```text
docs(reactions): consolidate brace and contested reaction model
docs(product): map reaction clash capability
docs(roadmap): trace reaction clash through E14 and feature registry
test(reactions): add reaction clash grammar and determinism coverage
test(scenarios): add brace and reaction clash scenarios
docs(wiki): explain prepared reactions and reaction clash
```

Separare documentazione, runtime e test se il diff diventa grande.

---

# 27. Definition of Done di questo consolidamento

Il task è chiuso solo quando:

1. Brace ha un significato unico e non contraddittorio nella documentazione;
2. Reaction Clash è rappresentato come estensione del Reaction System, non sistema parallelo;
3. CANONICAL e PROPOSED sono chiaramente separati;
4. Product Map/docs riflettono la nuova capability;
5. Feature Registry canonico contiene o collega le feature senza duplicati;
6. Roadmap/E14 rappresenta correttamente baseline vs estensione;
7. Scenario Map/Registry contiene coverage dimostrativa con Stable ScenarioId;
8. Wiki spiega il sistema senza diventare source of truth competitiva;
9. test e TurnLog necessari sono registrati/implementati secondo scope reale;
10. nessun leak di choice/futuro viene introdotto;
11. determinismo resta verificabile;
12. nessuna feature è dichiarata `DONE` senza gate reali;
13. eventuali conflitti restano espliciti nel report e/o `OPEN_DECISIONS`.

