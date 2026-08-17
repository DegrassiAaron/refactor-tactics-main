# RefactorTactics — Delayed Actions, Phase Boundaries, Fast Actions e Fast Reactions
> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

## Specifica consolidata di design e implementazione per Claude Code

**Data consolidamento:** 2026-08-07  
**Progetto:** RefactorTactics  
**Baseline tecnica documentale:** Unreal Engine 5.8 / canone repo 5.8.1 dove indicato dal repository  
**Scopo:** consolidare le decisioni correnti su ordine delle fasi, azioni ritardate/predittive, Action Ghosts, Fast Actions, Fast Reactions, reaction opportunities, privacy, determinismo, logging e test.

---

# 0. Regola di prevalenza delle fonti

Prima di modificare codice, Claude deve verificare il repository corrente.

Ordine di prevalenza:

```text
1. Decisioni esplicite più recenti del progetto / conversazione corrente
2. docs/design/piano-canonico-mvp.md e ADR/cataloghi correnti del repository
3. Handoff operativi recenti per Claude Code
4. Specifiche consolidate recenti su Action Ghosts / Fast Reaction
5. PDR tecnici
6. Vecchi documenti demo / matrici storiche
```

In particolare, NON reintrodurre automaticamente regole vecchie solo perché presenti in PDF o fogli di bilanciamento.

Conflitti già noti:

- vecchi PDR demo: interrupt/reaction window da 5 secondi;
- vecchie matrici: alcune Fast Reaction da 5–7 secondi;
- specifica Fast Reaction più recente: **3.0 secondi baseline**;
- vecchi PDR demo: roster Aegis/Nyx/Drift/Vex;
- canone operativo recente: **Gadget/Phase/Riktor/Wraith**;
- vecchi PDR: sequenze di resolution più generiche;
- canone recente delle macro-fasi: **Planning → Prep → Dash → Blast → Move**.

Per questo documento, le decisioni più recenti prevalgono.

---

# 1. Vincoli non negoziabili

## 1.1 Ordine principale delle fasi

RefactorTactics usa una struttura a fasi ispirata ai principi di Atlas Reactor:

```text
DECISION / PLANNING
        ↓
PREP
        ↓
DASH
        ↓
BLAST
        ↓
MOVE
        ↓
CLEANUP / fasi successive definite dal ruleset
```

Regola fondamentale:

> **Il normale MOVE è sempre l'ultima fase/azione volontaria standard del turno.**

Non progettare sequenze arbitrarie del tipo:

```text
Move → Attack → Move → Jump
```

Il giocatore non costruisce una timeline libera. Ogni azione appartiene alla propria fase.

Esempio:

```text
PREP:  Shield / Barricade / Stance
DASH:  Dash speciale oppure nessuna azione
BLAST: Attacco / abilità offensiva
MOVE:  Movimento normale finale
```

Dash, Blink, Charge, Leap classificato come Dash, displacement e movement reattivo NON sono la normale Move Phase.

---

## 1.2 Tutte le azioni vengono preparate prima della Resolution

Decisione corrente:

> **Le azioni vengono scelte/configurate durante Planning. La Resolution non riapre il Planning.**

Dopo il commit del piano:

- non si sceglie una nuova azione generica;
- non si cambia liberamente il target di un'azione già pianificata;
- non si decide liberamente “cosa fare dopo aver visto il Dash nemico”;
- non si programma una nuova azione completa a metà Resolution.

Le sole eccezioni sono **finestre interattive esplicitamente previste dal sistema**, come:

- Fast Action;
- Fast Reaction;
- Fast Select;
- altre Decision Window future, se dichiarate dal ruleset.

Queste finestre NON sono una seconda fase di Planning.

---

# 2. Terminologia canonica

Usare questi concetti in modo distinto.

## 2.1 Normal Action

Azione configurata interamente in Planning e risolta nella propria fase canonica.

Esempio:

```text
Blast Shot
ResolvePhase = Blast
Target = EnemyUnitId oppure CellId secondo policy
```

Nessuna nuova scelta durante Resolution.

---

## 2.2 Delayed Action

Azione configurata interamente in Planning ma risolta a un **boundary successivo** rispetto al suo setup.

Scopo principale:

> **prevedere lo stato futuro del campo**, non osservare il futuro e poi scegliere.

Esempio:

```text
Delayed Shot
ResolveBoundary = EndMove
TargetCell = H12
```

Il giocatore scommette che, a fine Move, un bersaglio utile sarà in H12.

Se H12 è vuota al boundary, l'azione può fizzle/fallire secondo la sua policy.

---

## 2.3 Predictive Action

Famiglia semantica di Delayed Action in cui il payoff dipende esplicitamente dalla previsione della posizione/stato futuro.

Tipicamente usa:

- Cell Lock;
- Line Lock;
- Area Lock;
- Direction Lock.

Il bersaglio futuro NON viene rivelato dal server.

---

## 2.4 Prepared Reaction

Reaction configurata in Planning e armata per la Resolution.

Esempio:

```text
Overwatch
Direction = NorthEast
Shape = Cone
Range = 5
Trigger = EnemyEnterArea
Charge = 1
```

La Reaction può non scattare mai.

---

## 2.5 Fast Reaction

Decisione rapida generata da un evento esterno valido durante Resolution.

Esempio:

```text
Enemy enters Overwatch area
→ Reaction Opportunity
→ 3 sec
→ FIRE / HOLD
```

La Fast Reaction è una scelta live consentita perché era stata preparata/abilitata dal sistema.

---

## 2.6 Fast Action

Decisione rapida generata come continuazione limitata di una propria azione o da una finestra interattiva esplicitamente prevista.

Esempio:

```text
Ability resolves
→ 3 sec
→ DASH LEFT / DASH RIGHT
```

La Fast Action NON permette di scegliere liberamente una nuova azione dal kit.

---

# 3. Delayed Actions — specifica di design

## 3.1 Principio fondamentale

Una Delayed Action serve a dire:

> “Preparo adesso un effetto che si risolverà in un punto futuro e provo a prevedere quale sarà lo stato del campo in quel momento.”

NON significa:

> “Aspetto di vedere cosa fanno i nemici e poi scelgo dove colpire.”

Quella seconda frase descriverebbe una nuova decisione durante Resolution e richiederebbe una Fast Action/Reaction esplicita.

---

## 3.2 Boundaries supportati

Il ruleset deve poter dichiarare boundary nominati.

Baseline:

```text
EndPrep
EndDash
EndBlast
EndMove
```

Questi boundary sono punti logici deterministici, non tempi di animazione.

Possibili usi:

```text
DelayedAction.ResolveBoundary = EndDash
DelayedAction.ResolveBoundary = EndBlast
DelayedAction.ResolveBoundary = EndMove
```

Non è necessario esporre tutti i boundary a ogni abilità.

Ogni ability definition dichiara quelli consentiti.

---

## 3.3 Cosa deve essere deciso in Planning

Una Delayed Action, salvo eccezioni esplicite, deve avere già definito:

- AbilityId / ActionId;
- owning unit;
- boundary di risoluzione;
- targeting policy;
- cella/linea/area/direzione/target consentito;
- costi;
- cooldown;
- eventuale facing;
- eventuale AoE;
- eventuali condizioni legali;
- fallback/fizzle policy;
- friendly fire policy;
- priorità di risoluzione.

Il client non deve ricevere informazioni nemiche future per completare questi campi.

---

## 3.4 Targeting consigliato per azioni predittive

Per preservare il valore della previsione, preferire:

```text
LockCell
LockLine
LockArea
LockDirection
```

Esempio:

```text
Intercept Bomb
Boundary = EndMove
TargetArea = Circle(center=H12, radius=1)
```

Il giocatore tenta di prevedere dove convergeranno i nemici.

`TrackUnit` può esistere per abilità specifiche ma riduce fortemente l'identità predittiva della Delayed Action e va usato come trade-off esplicito.

---

## 3.5 Esito se il bersaglio non è più valido

Ogni Delayed Action deve dichiarare una moving-target/fallback policy.

Esempi:

```text
Fizzle
AttackCell
AttackTarget
RetargetStable
PartialEffect
```

Per una vera azione predittiva baseline, preferire:

```text
LockCell + Fizzle/AttackCell
```

così una previsione sbagliata ha un costo reale.

---

# 4. Esempi di Delayed Action per fase

## 4.1 EndDash prediction

Planning:

```text
Hero.Wraith.InterceptShot
Boundary = EndDash
TargetCell = H8
```

Resolution:

```text
DASH
Enemy A → H8
Enemy B → H7

END DASH
InterceptShot resolves at H8
```

Se Enemy A è realmente in H8 al boundary e tutti i requisiti sono ancora validi → hit/effect.

Se H8 è vuota → fizzle o cell impact secondo definizione.

Nessuna nuova scelta viene aperta.

---

## 4.2 EndBlast prediction

Planning:

```text
Delayed Detonation
Boundary = EndBlast
Area = H10 radius 1
```

Il giocatore cerca di prevedere:

- chi resterà esposto dopo gli attacchi;
- quale cover verrà distrutta;
- quale area sarà Wet/Electrified;
- dove una spinta potrebbe lasciare un nemico.

Il resolver usa lo stato reale a `EndBlast`, ma il giocatore aveva scelto la geometria prima.

---

## 4.3 EndMove prediction

Planning:

```text
Mine Burst
Boundary = EndMove
TargetCell = H12
```

Resolution:

```text
MOVE micro-steps
...
END MOVE
Mine Burst @ H12
```

Questa è la forma più pura di previsione:

> “Dove saranno i nemici quando tutti avranno finito il normale Move?”

---

# 5. Delayed Action NON è Reaction

Una distinzione necessaria:

```text
Delayed Action:
  tempo/boundary noto
  target/geometria già pianificati
  nessuna nuova scelta

Reaction:
  trigger condizionale
  può non avvenire
  può generare una Fast Reaction
```

Esempio Delayed:

```text
At EndMove, shoot H12.
```

Esempio Reaction:

```text
If a detected enemy enters my cone, create an Overwatch opportunity.
```

Non implementare una Delayed Action come Reaction solo perché avviene più tardi.

Non implementare una Reaction come Delayed Action solo perché il trigger cade a un boundary.

---

# 6. Fast Action e Fast Reaction — limiti della scelta live

## 6.1 Baseline temporale

Nuovo baseline corrente:

```text
FastDecisionDuration = 3.0 seconds
```

La durata può diventare data-driven in futuro, ma per il sistema v0.1 usare 3 secondi salvo ability-specific decision già approvata.

---

## 6.2 La finestra deve essere stretta

Una Fast Action/Reaction deve offrire pochissime opzioni.

Esempi validi:

```text
FIRE / HOLD
LEFT / RIGHT
INTERPOSE / BRACE
COMMIT A / COMMIT B / HOLD
```

Esempi da evitare:

```text
scegli una delle 12 abilità
scegli una cella qualunque della mappa
ricostruisci il path completo
apri inventario
cambia build
```

La Decision Window non è un mini-turno.

---

## 6.3 Timeout

Il timeout è server-authoritative.

Per Overwatch:

```text
Timeout → HOLD
```

Motivazione:

- FIRE consuma una risorsa irreversibile;
- un timeout non deve spendere automaticamente la reaction;
- replay e server devono registrare la scelta canonica risultante.

---

# 7. Overwatch come caso di riferimento

Overwatch è il primo caso concreto del sistema generale di Reaction Opportunity.

Planning:

```text
Overwatch
- area/cone
- direction
- range
- trigger
- charges
- duration
- reaction policy
```

Resolution:

```text
Enemy enters valid area
        ↓
Reaction Opportunity
        ↓
FAST REACTION — 3 sec
        ↓
FIRE / HOLD
```

## 7.1 HOLD

```text
HOLD
```

significa:

- si perde solo l'opportunità corrente;
- Overwatch resta armata;
- una futura unità può generare un'altra opportunity;
- il giocatore NON sa se un altro trigger arriverà davvero.

## 7.2 FIRE

```text
FIRE(target)
```

significa:

- commit della Reaction;
- consumo della charge;
- validazione server;
- applicazione dell'effetto;
- transizione a Resolved/Expired secondo definition.

## 7.3 Trigger simultanei

Se due target triggerano nello stesso micro-step logico:

NON:

```text
prompt A
poi prompt B
```

MA:

```text
Reaction Opportunity
Targets = [A, B]

[FIRE A]
[FIRE B]
[HOLD]
```

L'ordine non deve dipendere dall'iterazione di `TMap`/`TSet`.

---

# 8. Phase Boundaries e Decision Boundaries

Non confondere i due concetti.

## 8.1 Phase Boundary

Confine deterministico tra fasi:

```text
EndPrep
EndDash
EndBlast
EndMove
```

Può attivare Delayed Actions già pianificate.

## 8.2 Decision Boundary

Punto deterministico in cui la simulazione deve attendere una risposta autorizzata.

Esempio:

```text
MOVE micro-step 3
Enemy enters Overwatch cone
→ Decision Boundary
→ Fast Reaction
→ resume same logical phase
```

Una Decision Boundary NON aggiunge una quinta fase.

---

# 9. La Resolution non deve “mostrare il futuro”

Il server può conoscere l'intero CanonicalIntentStore, ma il client non deve ricevere informazioni future non autorizzate.

Durante Resolution il giocatore vede soltanto ciò che è già diventato legittimamente osservabile.

Per esempio, se un nemico ha appena completato un Dash visibile:

```text
Confermato:
Enemy is now at H8
```

Ma il client NON riceve:

```text
Enemy will Blast H10
Enemy final Move destination = H14
Enemy future path = ...
```

Una Delayed Action già pianificata non viene aggiornata sfruttando questi dati.

Se esiste una Fast Reaction valida, la sua Opportunity contiene solo il presente informativo necessario.

---

# 10. Fog of War, Detection e informazione incompleta

Delayed Actions e Reactions devono rispettare:

- LOS;
- Detection;
- Visibility;
- Awareness, quando introdotta;
- Rumore/percezione acustica;
- stato pubblico;
- Team Knowledge.

Un trigger non deve equivalere automaticamente a conoscenza completa del bersaglio.

Esempio acustico futuro:

```text
Enemy Noise >= threshold
→ acoustic Reaction Opportunity
```

La UI può mostrare:

```text
Movement detected in sector NE
```

senza rivelare:

- unità esatta;
- path futuro;
- destinazione finale;
- ability intent privata.

---

# 11. Action Ghosts / Ghost Timeline

La Ghost Timeline deve rappresentare il piano, non inventare una simulazione autorevole.

Baseline:

```text
[PREP] [DASH] [BLAST] [MOVE]
```

Il giocatore può scrubbare le fasi per capire:

- posizione prevista;
- facing;
- endpoint Dash;
- origine di Blast;
- target/AoE;
- Move finale;
- cover;
- hazard;
- Reaction armata;
- Delayed Action e suo boundary.

## 11.1 Visualizzazione Delayed Action

Esempio timeline:

```text
PREP      DASH      BLAST      MOVE
 |          |          |          |
 |          |          |          +---- Move H12
 |          |          |
 |          |          +---- Normal Blast
 |          |
 |          +---- Dash
 |
 +---- Arm Delayed Shot @ EndMove → Cell J9
```

Oppure:

```text
[PREP] [DASH] [BLAST] [MOVE]
                         |
                         +-- ⏱ Delayed: Shot J9
```

La UI deve comunicare chiaramente:

- azione già configurata;
- boundary futuro;
- target fisso;
- esito incerto.

---

# 12. Confermato / Previsto / Incerto

Usare la grammatica UI già consolidata.

## Confermato

Stato pubblico + regola deterministica già nota.

Visuale:

```text
linea piena
colore pieno
```

## Previsto

Include il proprio piano e gli intenti sanitizzati della squadra.

Visuale:

```text
linea tratteggiata
icona team
```

## Incerto

Dipende da:

- nemici;
- collisioni future;
- target che può muoversi;
- Reaction;
- Fog of War;
- stato ambientale futuro.

Visuale:

```text
gradient / fade / ?
```

Le Delayed Actions sono spesso **Incerto** proprio perché puntano a uno stato futuro non garantito.

---

# 13. Privacy degli intenti

Requisito critico:

> Gli intenti avversari privati non devono mai essere replicati ai client nemici.

Non è sufficiente “nascondere il widget”.

Il client nemico non deve ricevere:

- enemy path pianificato;
- enemy destination;
- enemy Action Ghost privati;
- enemy target pianificato;
- enemy AoE pianificata;
- enemy AbilityId non osservabile;
- enemy DelayedAction target/boundary se privato;
- future Reaction Opportunities;
- future positions;
- future trigger count.

Architettura:

```text
Client Proposal
      ↓
Server Validation
      ↓
CanonicalIntentStore          // server-only
      ↓
Team Sanitized DTO            // team-only
      ↓
Planning ViewModel
```

Dopo la resolution, pubblicare solo i risultati/eventi che il ruleset autorizza.

---

# 14. Modello deterministico

Delayed Actions, Fast Action e Fast Reaction devono essere parte del resolver logico.

Non dipendere da:

- Tick client;
- frame rate;
- durata montage;
- ordine implicito di container hash;
- tempo di arrivo pacchetti per l'ordine logico;
- physics real-time.

Usare:

- StableUnitId;
- ActionId / ReactionInstanceId stabili;
- RulesVersion;
- ContentManifestHash;
- ResolverConfigHash;
- phase/boundary enum espliciti;
- micro-step index;
- priority deterministica;
- canonical decision command per le Fast Window.

---

# 15. Modello dati concettuale

Questi tipi sono proposte architetturali da adattare alle API e alle convenzioni reali del repository.

## 15.1 Boundary

```cpp
enum class ERTResolutionBoundary : uint8
{
    EndPrep,
    EndDash,
    EndBlast,
    EndMove
};
```

Valutare se usare un enum C++ o Gameplay Tags governati in base al modello corrente del repo.

## 15.2 Delayed Action Intent

```cpp
struct FRTDelayedActionIntent
{
    FName ActionId;
    int32 OwnerUnitId;

    ERTResolutionBoundary ResolveBoundary;

    // Target già deciso nel Planning.
    FRTCellId TargetCell;
    int32 Direction;

    FName TargetingPolicyId;
    FName FallbackPolicyId;
};
```

Non aggiungere contemporaneamente tutti i campi se `FRTActionDef`/intent correnti li possiedono già. Preferire estendere il modello esistente.

## 15.3 Fast Decision Type

```cpp
enum class ERTFastDecisionType : uint8
{
    Action,
    Reaction
};
```

## 15.4 Reaction Opportunity

```cpp
struct FRTReactionOpportunity
{
    FName OpportunityId;
    FName ReactionInstanceId;
    int32 OwningUnitId;
    int32 TurnIndex;
    int32 MicroStepIndex;

    TArray<int32> TriggeringUnitIds;
    TArray<FName> AllowedResponses;
};
```

NON contenere:

- future positions;
- remaining enemy path;
- future triggers;
- enemy canonical intent;
- result of unresolved future phases.

---

# 16. Resolver — pipeline concettuale

Non creare un secondo resolver parallelo.

Estendere il resolver esistente con boundary espliciti.

Concept:

```text
Validate snapshot + accepted intents
        ↓
Build stable queues by phase
        ↓
Resolve PREP
        ↓
Resolve DelayedActions@EndPrep
        ↓
Resolve DASH
        ↓
Resolve DelayedActions@EndDash
        ↓
Resolve BLAST
        ↓
Resolve DelayedActions@EndBlast
        ↓
Resolve MOVE micro-steps
        ├─ evaluate reaction triggers
        ├─ maybe create Decision Boundary
        ├─ receive canonical FastDecision
        └─ resume movement
        ↓
Resolve DelayedActions@EndMove
        ↓
Environment/Objectives/Cleanup according to canonical ruleset
        ↓
StateHash + LogHash
```

Attenzione:

- una Reaction può interrompere una fase a un micro-step;
- una Delayed Action normalmente aspetta il boundary dichiarato;
- le due meccaniche non vanno fuse.

---

# 17. TurnLog

Aggiungere eventi sufficienti a spiegare e riprodurre il sistema.

## 17.1 Delayed Action

Minimo:

```text
DelayedActionDeclared
DelayedActionArmed
DelayedActionBoundaryReached
DelayedActionResolved
DelayedActionFizzled
```

Campi utili:

```text
TurnId
ActionId
OwnerUnitId
ResolveBoundary
TargetPolicy
TargetCell/Area/Direction
ResultReason
AffectedUnitIds
```

## 17.2 Reaction

Minimo:

```text
ReactionArmed
ReactionOpportunityCreated
ReactionOpportunityTargets
ReactionDecisionCommit
ReactionDecisionHold
ReactionDecisionTimeout
ReactionTriggered
ReactionResolved
ReactionCancelled
ReactionExpired
```

## 17.3 Privacy del log

Il TurnLog canonico server può contenere dati di audit necessari, ma i DTO inviati ai client devono essere sanitizzati secondo audience/knowledge.

Non usare un log pubblico prematuro per leakare planning privato.

---

# 18. Networking

Principio:

```text
Client proposes
Server validates
Server applies
```

## 18.1 Planning

- preview: team-only, sequenziata, target 8–12 Hz;
- commit/Ready: reliable;
- CanonicalIntentStore: server-only.

## 18.2 Fast Reaction

Flow:

```text
1. Server reaches deterministic trigger/boundary.
2. Server builds sanitized Opportunity.
3. Server sends only to authorized owner/team client.
4. Client shows 3 s Decision Window.
5. Client sends FIRE/HOLD/etc.
6. Server validates ownership, OpportunityId, deadline, target legality, remaining charge.
7. Decision becomes canonical input.
8. Resolver resumes.
```

Il server decide il timeout.

---

# 19. Slow-motion e presentazione

Durante una Decision Window:

```text
Logical simulation:
PAUSED at deterministic boundary

Presentation:
may continue at 10–20% speed

UI countdown:
3 real-time seconds
```

La slow-motion NON modifica:

- seed;
- ordine;
- collisioni;
- path;
- LOS logica;
- danno;
- boundary;
- opportunity;
- risultati.

---

# 20. Bilanciamento delle Delayed Actions

Delayed Actions ricevono un vantaggio tattico: possono colpire uno stato futuro potenzialmente più favorevole.

Ma non ricevono nuova informazione interattiva.

Possibili trade-off data-driven:

- danno ridotto;
- cooldown maggiore;
- range minore;
- area più stretta;
- costo risorsa;
- telegraph;
- limitazione a una Delayed Action per turno;
- target solo cell/line/area;
- fizzle se previsione errata;
- friendly fire;
- vulnerabilità durante setup.

Non imporre tutti questi costi contemporaneamente.

Bilanciare per:

```text
Power = payoff + reliability + information advantage - setup risk - opportunity cost
```

---

# 21. Pattern di gameplay desiderati

## 21.1 Prevedere il Dash

```text
Enemy has a strong Dash kit.
Player predicts choke point H8.
Delayed Shot @ EndDash → H8.
```

## 21.2 Forzare una previsione tramite cover

Un alleato modifica una cover/porta per rendere alcune destinazioni più probabili.

Il secondo personaggio prepara una Delayed Action su una delle uscite.

La combo premia coordinazione e lettura del nemico.

## 21.3 Push + delayed AoE

Planning team-only:

```text
Phase: Pressure Jet → push toward H12
Wraith/Gadget: Delayed AoE @ EndBlast → H12
```

L'esito resta incerto perché:

- il target può Dashare;
- può essere spostato da altro effetto;
- il push può fizzle;
- una cover può cambiare;
- una Reaction può intervenire.

## 21.4 Overwatch bait

```text
Tank enters cone
→ FIRE / HOLD
```

HOLD conserva la reaction, ma il giocatore non sa se arriverà un bersaglio migliore.

Questo supporta bait/bluff/commitment senza rivelare il futuro.

---

# 22. Errori di design da evitare

NON implementare:

```text
“Dopo Dash scegli un target qualsiasi.”
```

come comportamento standard di una Delayed Action.

NON implementare:

```text
If Tank → HOLD
If Carry → FIRE
```

come macro automatica standard della Fast Reaction.

NON implementare:

```text
PREP → DASH → BLAST → MOVE → REACTION
```

come quinta fase lineare.

NON implementare:

```text
Enemy intent replicated globally but hidden in UI
```

NON implementare:

```text
DelayedAction target updated from enemy future position on client
```

NON implementare:

```text
reaction after full movement playback
```

se la reaction deve interrompere realmente un micro-step.

NON hardcodare:

```cpp
if (ActionId == "Hero.Wraith.InterceptShot")
```

nel TurnManager se il comportamento può essere espresso da dati/policy generali.

---

# 23. Relazione con il codice corrente

L'handoff operativo recente indica che il progetto possiede già concetti utili:

- `FRTActionDef` con `ResolutionPhase`, `Priority`, `Fallback`, `Slot`, `MovementStyle`, reaction metadata;
- `RTActionQueue`;
- `RTActionEffectLibrary`;
- `RTReactionLibrary`;
- soppressione/intercept a micro-step;
- Action catalog data-driven;
- resolver esagonale deterministico;
- TurnLog e golden tests.

Regola:

> **Estendere questi sistemi; non creare un secondo motore di Delayed/Reaction parallelo.**

Prima dell'implementazione Claude deve verificare i nomi/file reali sul branch attuale.

---

# 24. Strategia di implementazione UE5 consigliata

## Step 1 — Boundary model

Aggiungere un concetto di boundary alla queue/resolver.

Possibile approccio:

```text
ResolutionPhase + BoundaryTiming
```

oppure:

```text
ExecutionPoint enum/tag
```

Scegliere la soluzione minima compatibile con `FRTActionDef` corrente.

## Step 2 — Delayed Action data

Aggiungere solo i dati mancanti:

- ResolveBoundary;
- target policy;
- fixed target geometry;
- fizzle/fallback.

## Step 3 — Queue

Le azioni ritardate vengono inserite in code stabili ordinate per:

```text
Boundary
Priority
StableUnitId
ActionInstanceId
```

## Step 4 — Resolver

Al boundary:

1. raccogliere azioni eleggibili;
2. rivalidare precondizioni;
3. calcolare target sul working state attuale;
4. risolvere in ordine stabile;
5. produrre TurnEvents;
6. applicare stato;
7. continuare.

## Step 5 — UI

Ghost Timeline mostra:

```text
Action + boundary + target + certainty
```

## Step 6 — Reactions

Mantenere separato il flusso Reaction Opportunity / Fast Decision.

---

# 25. Test automatici minimi — Delayed Actions

Creare Automation Tests/golden tests almeno per:

### Test 1 — EndDash hit

Enemy termina Dash nella cella prevista.

```text
Expected: delayed action hits.
```

### Test 2 — EndDash miss

Enemy termina Dash altrove.

```text
Expected: fizzle/AttackCell according to policy.
```

### Test 3 — EndMove prediction

Enemy termina normale Move nella cella prevista.

```text
Expected: delayed action resolves only after Move completion.
```

### Test 4 — Target moves between boundaries

Target passa nella cella durante Dash ma non è lì a EndMove.

```text
Delayed@EndMove must NOT hit merely because target crossed earlier.
```

### Test 5 — Cover changes before boundary

Cover viene distrutta/creata prima del boundary.

```text
Expected: LOS/target validity uses current logical state at resolution boundary.
```

### Test 6 — Owner KO before boundary

Owner viene KO prima della resolution della Delayed Action.

Policy data-driven:

```text
Cancel/Fizzle according to definition.
```

### Test 7 — Stable ordering

Due Delayed Actions allo stesso boundary.

Permutare insertion order.

```text
Expected: identical TurnLog + state hash.
```

### Test 8 — No enemy planning leak

Il client nemico non riceve target/boundary dell'azione privata durante Planning.

### Test 9 — Replay

Stesso snapshot + intents + Fast Decisions.

```text
Expected: same LogHash and StateHash.
```

---

# 26. Test automatici minimi — Fast Reaction

Mantenere almeno:

```text
HoldKeepsReactionArmed
CommitConsumesCharge
TimeoutMapsToHold
SimultaneousTargetsShareOpportunity
PermutationInvariant
KOCancelsArmedReaction
DoesNotExposeFutureTriggers
FireTruncatesFutureMovement
HoldResumesSameMovementState
InterruptionAffectsLaterCollision
```

Attenzione al naming Unreal: evitare un test eseguibile che sia prefisso gerarchico di altri test.

---

# 27. Debug

Aggiungere strumenti development-only per vedere:

## Delayed Action

- owner;
- ActionId;
- target cell/area/line;
- resolve boundary;
- priority;
- current state;
- fizzle reason;
- current boundary.

## Reaction

- reaction state;
- cone/area;
- remaining charges;
- prompt count;
- current opportunity;
- trigger reason;
- micro-step;
- LOS/Detection;
- stable priority.

Il debug rendering non è mai fonte della simulazione.

---

# 28. Acceptance criteria della feature

Una prima implementazione di Delayed Actions è Done solo se:

1. un'azione può essere configurata interamente in Planning;
2. il target non cambia liberamente durante Resolution;
3. può risolversi almeno a `EndDash` e `EndMove`;
4. usa il working state reale del boundary;
5. una previsione errata produce un esito leggibile/fizzle;
6. il normale Move resta ultima fase volontaria;
7. Fast Reactions restano rami condizionali separati;
8. nessun intent nemico viene replicato per aiutare la previsione;
9. TurnLog spiega dichiarazione, boundary ed esito;
10. replay con stesso input produce stesso hash;
11. test packaged/network privacy passano quando la feature entra nel percorso multiplayer;
12. Action Ghost mostra chiaramente il boundary e l'incertezza.

---

# 29. Decisioni consolidate da trattare come canone corrente

Claude deve considerare le seguenti regole come vincoli correnti finché un ADR successivo non le sostituisce:

1. `Planning → Prep → Dash → Blast → Move` è l'ordine principale.
2. Il normale Move è sempre l'ultima azione/fase volontaria standard.
3. Dash/Blink/Charge/Leap/displacement non sono Normal Move.
4. Le azioni vengono scelte e configurate durante Planning.
5. Dopo commit NON esiste una seconda fase di planning durante Resolution.
6. Le Delayed Actions vengono preparate completamente in Planning.
7. Una Delayed Action risolve a un phase boundary futuro.
8. Lo scopo della Delayed Action è la **previsione**, non ottenere informazione gratuita.
9. Il target/geometria della Delayed Action non viene scelto liberamente dopo aver visto il comportamento nemico.
10. Le uniche nuove decisioni live sono finestre esplicitamente previste: Fast Action / Fast Reaction / Fast Select.
11. Fast Action e Fast Reaction possono condividere infrastruttura, ma restano semanticamente distinte.
12. Fast Reaction baseline: 3.0 s.
13. Overwatch usa Reaction Opportunity; FIRE consuma, HOLD conserva l'arma per opportunità successive.
14. Il giocatore non conosce future opportunities o future path.
15. Trigger simultanei nello stesso micro-step formano una singola opportunity multi-target.
16. Una Fast Reaction crea un Decision Boundary dentro la fase corrente, non una nuova macro-fase.
17. Action Ghosts/Ghost Timeline sono presentation-only.
18. Ghost nemici non vengono generati da intenti privati.
19. UI usa Confermato / Previsto / Incerto.
20. Resolver, snapshot, log e decisioni canoniche devono restare deterministici.
21. Server authoritative: client propone, server valida/applica.
22. Non creare sistemi hard-coded per il singolo eroe/scenario quando la meccanica può essere data-driven.

---

# 30. Istruzione operativa finale per Claude Code

Quando implementi questa specifica:

```text
1. verifica repository/branch/HEAD;
2. leggi piano canonico + ADR azioni + catalogo azioni;
3. individua il modello reale di FRTActionDef / intent / queue / resolver;
4. estendi il sistema più vicino invece di duplicarlo;
5. aggiungi il concetto minimo di phase boundary;
6. implementa una Delayed Action cell-locked;
7. crea golden tests deterministici;
8. integra Action Ghost / debug solo dopo che il resolver è corretto;
9. mantieni Fast Reaction separata;
10. verifica privacy e nessun future-state leak.
```

A fine task riportare:

- file modificati;
- ragione di ogni modifica;
- nuovo modello dati;
- test aggiunti;
- test eseguiti;
- build Editor/Game;
- eventuali regression;
- debiti tecnici;
- commit Git proposto;
- prossimo checkpoint minimo.

Commit indicativo:

```text
feat(turn): add predictive delayed actions at deterministic phase boundaries
```

---

# 31. Documenti consolidati / materiale di riferimento

Questa specifica consolida e aggiorna i concetti presenti in:

- `RefactorTactics_ActionGhosts_Phases_FastReactions_Claude.md`
- `RefactorTactics_Overwatch_FastReaction_Claude.md`
- `CLAUDE_Showcase_v0.1_Integration_CurrentCode.md`
- `RT_PDR_05_Simulazione_Deterministica_v0.1.pdf`
- `RT_PDR_04_Networking_Privacy_v0.1.pdf`
- `RT_PDR_08_UI_UX_Coordinazione_v0.1.pdf`
- `RT_PDR_07_Abilita_Personaggi_GAS_v0.1.pdf`
- `RT_PDR_11_Demo_v0.1.pdf` — materiale storico, con valori in parte superati
- `RefactorTactics_Balance_Matrices_v0.1.xlsx` — utile per bilanciamento, ma alcune durate Fast Reaction sono legacy rispetto al baseline 3 s più recente

Il repository corrente resta la fonte operativa da verificare prima di cambiare codice.
