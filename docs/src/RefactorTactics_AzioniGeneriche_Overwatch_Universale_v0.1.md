# RefactorTactics — Azioni Generiche e Overwatch Universale
## Specifica di integrazione delle decisioni di design

**Documento:** RT_Design_GenericActions_Overwatch  
**Versione:** 0.1  
**Data:** 7 agosto 2026  
**Baseline tecnica:** Unreal Engine 5.8  
**Stato:** Decisioni consolidate da integrare nella documentazione di progetto

---

# 1. Scopo

Questo documento consolida le decisioni prese sulle **azioni generiche**, cioè le azioni disponibili a tutti i personaggi di RefactorTactics, e definisce **Overwatch come sistema universale**.

Principio guida:

> Tutti i personaggi condividono una grammatica comune di azioni.  
> Le differenze tra personaggi derivano da profili, abilità, equipaggiamento e varianti che modificano il modo in cui quelle azioni funzionano.

Le azioni generiche non devono sostituire l'identità dei kit. Devono invece fornire un linguaggio comune leggibile, prevedibile e riutilizzabile dal resolver.

---

# 2. Ordine delle fasi

La struttura principale del turno resta:

```text
Planning
  ↓
Prep
  ↓
Dash
  ↓
Blast
  ↓
Move
```

Regola vincolante:

> Il normale **Move è sempre l'ultima azione volontaria del turno**.

Non supportare sequenze arbitrarie come:

```text
Move → Attack
```

Gli spostamenti speciali possono avvenire prima del Move solo se appartengono alla loro fase specifica, ad esempio Dash.

---

# 3. Set di azioni generiche

Baseline iniziale:

```text
Wait
Basic Attack
Interact
Brace
Move
Overwatch
```

Queste sono azioni universali come concetto.

La loro implementazione concreta può dipendere da:

- Character Definition;
- equipaggiamento;
- profilo di movimento;
- profilo Overwatch;
- stato;
- superficie;
- requisiti della mappa;
- ruleset.

---

# 4. Wait

## Definizione

Il personaggio rinuncia volontariamente a una possibilità di azione.

Possibili usi:

- non eseguire azioni in una determinata fase;
- mantenere posizione;
- evitare rischi;
- non consumare risorse;
- effettuare una scelta intenzionalmente passiva.

Wait non deve produrre effetti nascosti o bonus impliciti salvo regola esplicita.

---

# 5. Basic Attack

## Decisione

Tutti i personaggi possiedono il concetto di **Basic Attack**, ma non necessariamente lo stesso attacco.

Il comando è universale:

```text
BasicAttack(Target)
```

La sua definizione dipende da personaggio/equipaggiamento.

Esempi:

- attacco ranged;
- attacco melee;
- colpo preciso;
- colpo pesante;
- colpo leggero;
- attacco con proprietà ambientali minime.

## Obiettivo di design

Basic Attack deve essere:

- sempre comprensibile;
- affidabile;
- più semplice di una skill;
- meno efficiente delle abilità signature;
- utile quando le abilità non sono disponibili o non convengono.

Non deve sostituire le abilità del kit.

---

# 6. Interact

## Decisione

**Interact** è un'azione universale e deve diventare uno dei principali punti di contatto tra unità e mappa.

Il comando concettuale è:

```text
Interact(InteractionId)
```

L'oggetto o la struttura della mappa definisce il risultato.

Esempi:

```text
Open Door
Close Door
Activate Console
Operate Valve
Toggle Bridge
Use Elevator
Pick Up Objective
Plant Objective
Carry / Drop Objective
Activate Generator
Disable Device
```

## Principio tecnico

Interact non deve hard-codificare il comportamento delle singole strutture.

La mappa espone interazioni valide; il resolver applica la relativa regola.

Porte, ponti, ascensori e altre strutture modificano il grafo tattico come dati logici, non soltanto come mesh.

---

# 7. Brace

## Decisione

Tutti i personaggi possono assumere una postura difensiva base.

Nome di lavoro:

```text
Brace
```

Brace deve essere deliberatamente più debole delle capacità difensive specifiche dei personaggi.

Possibili effetti da scegliere durante il bilanciamento:

- riduzione del primo danno ricevuto;
- maggiore resistenza al displacement;
- riduzione di una specifica categoria di impatto.

Per la prima versione evitare di sommare molti bonus.

## Vincolo di design

Brace non deve competere in potenza con:

- Counter;
- Guard dedicati;
- Intercept;
- parry;
- stance difensive signature;
- reaction specialistiche.

Serve come fallback universale.

---

# 8. Move come azione generica

Move resta l'azione volontaria finale del turno.

La proposta consolidata è trattare:

```text
Sneak
Move
Sprint
```

come **profili della stessa azione Move**, non come tre abilità indipendenti.

---

# 9. Profili di movimento

## 9.1 Sneak

Priorità:

- discrezione;
- rumore ridotto;
- minore esposizione acustica.

Trade-off:

- distanza ridotta;
- costo di movimento maggiore o budget inferiore;
- possibili limitazioni con altre azioni.

---

## 9.2 Move

Profilo standard.

Caratteristiche:

- distanza normale;
- rumore normale;
- nessun bonus speciale;
- riferimento principale per il bilanciamento.

---

## 9.3 Sprint

Priorità:

- distanza;
- pressione;
- raggiungimento rapido di obiettivi.

Trade-off:

- rumore elevato;
- esposizione;
- maggiore interazione con terreno pericoloso;
- possibile vulnerabilità a reaction.

---

# 10. Valori iniziali di riferimento

Valori esclusivamente da playtestare:

| Profilo | Distanza indicativa | Rumore indicativo |
|---|---:|---:|
| Sneak | 2 | 0–1 |
| Move | 3 | 2 |
| Sprint | 5 | 5 |

Il personaggio modifica questi valori attraverso il proprio profilo.

Esempio:

```text
Character A
SneakRange = 3

Character B
SprintRange = 4
SprintNoise = 7
```

---

# 11. Overwatch — decisione principale

## Decisione consolidata

**Overwatch è un'azione generica disponibile a tutti i personaggi.**

Non è però un effetto identico per tutto il roster.

Principio:

> Overwatch è universale come postura, framework e comando di Planning.  
> Trigger, area, opzioni di reaction ed effetto dipendono dal personaggio, equipaggiamento, build o stato.

In altre parole:

```text
Generic Action
    OVERWATCH
        |
        v
Character / Equipment Overwatch Profile
        |
        v
Reaction behavior
```

---

# 12. Overwatch non equivale a "sparare"

Non definire Overwatch come:

```text
Enemy enters cone → shoot
```

Quello è soltanto uno dei possibili profili.

Esempi:

| Archetipo | Possibile Overwatch |
|---|---|
| Marksman | Fire / Precision Shot |
| Tank | Intercept / Protect |
| Controller | Push / Displace |
| Assassin | Ambush |
| Electric specialist | Arc Trigger |
| Water controller | Pressure Jet / Wet |
| Engineer | Hack / Device reaction |
| Support | Guard Ally / Shield |

Questo permette di mantenere una regola universale senza rendere i personaggi omogenei.

---

# 13. Overwatch Profile

Ogni personaggio deve poter definire un profilo Overwatch data-driven.

Campi concettuali:

```text
OverwatchProfileId
WatchAreaShape
Range
ArcWidth
ValidTriggerTypes
ValidTargetTypes
DetectionRequirements
LineOfSightRequirements
Charges
MaxPrompts
AllowedResponses
ResolutionPolicy
ValidPhases
Priority
TimeoutPolicy
```

Questi campi sono concettuali e vanno adattati alle strutture runtime reali.

---

# 14. Trigger di Overwatch

Overwatch non deve osservare genericamente "qualsiasi cosa".

I trigger devono essere espliciti.

Possibili categorie:

```text
EnemyEnterArea
EnemyLeaveArea
EnemyMove
EnemyDash
EnemyAttack
EnemyUseAbility
EnemyInteract
EnemyApproachesAlly
ProjectileCrossesSector
DeviceActivated
NoiseDetected
```

Ogni profilo supporta soltanto i trigger appropriati.

---

# 15. Overwatch e fasi

Un profilo Overwatch può reagire solo in determinate fasi.

Esempio:

```text
Marksman Overwatch
ValidPhases:
    Move

Anti-Dash Specialist
ValidPhases:
    Dash

Guardian Overwatch
ValidPhases:
    Dash
    Move

Counter-Hacker
ValidPhases:
    Prep
```

La UI deve comunicare chiaramente quali fasi sono sorvegliate.

---

# 16. Reaction Opportunity

Overwatch usa il sistema generale di Reaction.

Flusso:

```text
Planning
  ↓
Overwatch armed
  ↓
Resolver reaches valid event
  ↓
Reaction Opportunity
  ↓
Resolution Policy
  ↓
Reaction commit / hold / automatic outcome
```

Una singola Overwatch può produrre zero o più opportunità finché resta armata.

---

# 17. HOLD

Decisione già consolidata:

> HOLD rifiuta soltanto l'opportunità corrente.

HOLD:

- non consuma la charge;
- non disarma automaticamente Overwatch;
- non garantisce che esista un trigger futuro;
- mantiene il mindgame.

Esempio:

```text
Enemy A enters
→ FIRE / HOLD

HOLD

Enemy B enters
→ FIRE / HOLD

HOLD

Enemy C enters
→ FIRE
```

Questo abilita:

- bait;
- bluff;
- attesa di un target migliore;
- rischio di perdere completamente la reaction.

---

# 18. Fast Reaction

Default aggiornato:

```text
FastReactionDuration = 3.0 seconds
```

Questa decisione **prevale sulle precedenti specifiche che riportavano 5 secondi**, salvo eccezione esplicita futura.

Fast Reaction non è una seconda fase di Planning.

Deve essere:

- immediata;
- leggibile;
- con pochissime opzioni;
- senza menu profondi;
- compatibile con decisioni sotto pressione.

Timeout di default:

```text
Timeout → HOLD
```

Motivazione:

il timeout non deve spendere automaticamente una risorsa irreversibile.

---

# 19. Tre policy di risoluzione Overwatch

Per evitare che Overwatch universale rallenti eccessivamente ogni turno, non tutte le opportunità devono aprire una finestra manuale.

Definire almeno tre policy concettuali.

## 19.1 Automatic

Il comportamento viene eseguito automaticamente al primo trigger valido.

Esempio:

```text
FireFirstEnemy
```

---

## 19.2 Conditional

Il giocatore imposta una regola durante il Planning.

Esempio:

```text
Fire only if target HP <= 50%
```

Il resolver applica deterministicamente la condizione.

Le condizioni disponibili devono essere limitate, leggibili e validate dal ruleset.

---

## 19.3 Fast Select

Quando compare una opportunità valida:

```text
FIRE
HOLD
```

oppure:

```text
PUSH LEFT
PUSH RIGHT
HOLD
```

La decisione deve avvenire entro la Fast Reaction Window.

---

# 20. Rischio: troppe interruzioni

Problema principale dell'Overwatch universale:

> se molti personaggi generano finestre manuali, la Resolution rischia di diventare continuamente interrotta.

Scenario da evitare:

```text
Animation
→ STOP
→ 3 seconds
→ animation
→ STOP
→ 3 seconds
→ STOP
...
```

Mitigazioni obbligatorie:

- non tutte le Overwatch usano Fast Select;
- usare Automatic e Conditional dove appropriato;
- limitare charges;
- limitare MaxPrompts;
- aggregare trigger simultanei;
- evitare reaction annidate nell'MVP;
- telegraphing chiaro durante Planning.

---

# 21. Trigger simultanei

Se più target generano lo stesso trigger nello stesso micro-step logico, non creare prompt sequenziali artificiali.

Corretto:

```text
Enemy A + Enemy B enter in same micro-step
        ↓
Single Reaction Opportunity

[FIRE A]
[FIRE B]
[HOLD]
```

Errato:

```text
Prompt A
then
Prompt B
```

se entrambi appartengono allo stesso boundary simulativo.

Questo evita dipendenza dall'ordine di iterazione.

---

# 22. Costo di Overwatch

Rischio:

> Se Overwatch non costa abbastanza, diventa la scelta di default quando il giocatore è indeciso.

Decisione proposta/consolidata per la baseline:

> Overwatch compete con l'azione offensiva principale del turno.

Quindi normalmente:

```text
Attack
OR
Ability
OR
Overwatch
```

Non:

```text
Attack + Overwatch
```

salvo abilità, personaggi o effetti che violano esplicitamente questa regola.

Questo crea un vero costo-opportunità:

```text
"Rinuncio a colpire ora perché penso che passerai di lì."
```

Se nessun trigger avviene, l'investimento può essere perso.

---

# 23. Charge e prompt

Baseline:

```text
Charges = 1
FastReactionDuration = 3.0 s
Timeout = HOLD
MaxPrompts = data-driven
```

Una reaction con una singola charge può produrre:

```text
0..N Opportunities
0..1 Commit
```

La charge viene consumata soltanto quando una risposta che la usa viene effettivamente committata.

---

# 24. Identità del personaggio

La caratterizzazione Overwatch può derivare da:

```text
Range
Arc
Trigger Types
Reaction Options
Charges
Max Holds
Detection Rules
Valid Phases
Target Types
Environmental Requirements
Priority
```

Esempi:

## Marksman

```text
Long range
Narrow arc
Trigger: enemy movement
Response: FIRE / HOLD
```

## Tank

```text
Short range
Wide arc
Trigger: enemy approaches ally
Response: INTERCEPT / HOLD
```

## Assassin

```text
Short range
Detection/stealth dependent
Trigger: enemy enters proximity
Response: AMBUSH / HOLD
```

## Controller

```text
Medium area
Trigger: enemy crosses watched edge
Response:
    PUSH LEFT
    PUSH RIGHT
    HOLD
```

## Hacker

```text
Trigger: enemy interacts with watched device
Response:
    HACK
    HOLD
```

---

# 25. Informazione e privacy

Overwatch deve rispettare integralmente Fog of War e privacy di rete.

Il server può conoscere:

- snapshot completo;
- intenti canonici;
- futuri eventi già determinabili dal resolver.

Il client NON deve ricevere informazioni future non autorizzate.

Quando nasce una Reaction Opportunity, il client riceve solo:

- evento corrente legittimamente osservabile;
- target validi;
- opzioni consentite;
- deadline;
- stato necessario alla UI.

Non inviare:

- futuri trigger;
- percorsi futuri;
- destinazioni future;
- planning avversario;
- numero di opportunità ancora possibili.

---

# 26. Overwatch e Detection

Essere geometricamente dentro un'area non implica necessariamente trigger.

Condizione concettuale:

```text
TargetInsideWatchArea
AND
ValidTriggerEvent
AND
TargetDetected
AND
LOS requirements satisfied
AND
ReactionStillArmed
AND
PhaseAllowed
```

Separare:

```text
LOS
Detection
Visibility
Awareness
```

---

# 27. Overwatch e rumore

Overwatch può in futuro usare anche trigger acustici.

Esempio:

```text
NoiseDetected
AND
NoiseIntensity >= Threshold
```

Possibili response:

```text
Prepare shot
Take cover
Investigate
Ambush
```

Il client può reagire a informazione acustica senza conoscere necessariamente la cella esatta del nemico.

---

# 28. UI di Planning

Overwatch deve essere leggibile prima del commit.

Visualizzare:

- area sorvegliata;
- facing;
- range;
- trigger supportati;
- fasi osservate;
- reaction options;
- charges;
- policy Automatic / Conditional / Fast Select;
- eventuali requisiti LOS/Detection.

La UI distingue:

- **Confermato**
- **Previsto**
- **Incerto**

Non usare informazioni del planning nemico per warning o preview.

---

# 29. UI durante Fast Reaction

La finestra deve essere minimale.

Esempi:

```text
OVERWATCH
Target: Enemy A

[FIRE]
[HOLD]

3.0
```

oppure:

```text
CONTROL OVERWATCH

[PUSH LEFT]
[PUSH RIGHT]
[HOLD]

3.0
```

La simulazione autorevole si arresta sul decision boundary.

La presentazione può continuare in slow motion, ma il rallentamento è esclusivamente visuale.

---

# 30. Determinismo

Overwatch non deve dipendere da:

- frame rate;
- Tick client;
- timing animazioni;
- ordine di TMap/TSet;
- arrivo casuale di pacchetti;
- durata reale di un montage.

Quando più reaction sono valide nello stesso boundary:

1. raccogliere tutte le opportunità;
2. raggruppare trigger simultanei;
3. ordinare con chiavi stabili;
4. applicare la policy;
5. produrre TurnEvents;
6. aggiornare lo stato logico.

---

# 31. Data-driven

Il framework Overwatch deve essere definito in C++, ma configurato tramite dati.

C++ definisce:

- trigger supportati;
- invarianti;
- validazione;
- resolver;
- ordinamento;
- autorizzazione;
- serialization;
- timeout behavior.

Data Assets definiscono:

- profilo del personaggio;
- area;
- range;
- trigger ammessi;
- response;
- costi;
- priorità;
- charge;
- policy;
- requisiti.

---

# 32. Possibile modello concettuale

Esempio architetturale, non firma API definitiva:

```cpp
enum class ERTOverwatchResolutionPolicy : uint8
{
    Automatic,
    Conditional,
    FastSelect
};

struct FRTOverwatchProfile
{
    FName ProfileId;

    int32 Range;
    int32 ArcWidth;
    int32 Charges;
    int32 MaxPrompts;

    ERTOverwatchResolutionPolicy ResolutionPolicy;

    TArray<FName> ValidTriggerIds;
    TArray<FName> AllowedResponseIds;
};
```

Runtime:

```text
FRTOverwatchInstance
    InstanceId
    OwnerUnitId
    ProfileId
    ArmedAtPhase
    RemainingCharges
    PromptCount
    CurrentState
    SourceIntentId
```

Questi frammenti sono concettuali e devono essere adattati alle convenzioni C++ effettive del progetto.

---

# 33. Networking

Flusso:

```text
Client Planning
    |
    v
ServerSubmitIntent
    |
    v
CanonicalIntentStore
    |
    v
Snapshot
    |
    v
Action Resolver
    |
    v
Reaction Opportunity
    |
    v
Sanitized Client RPC
    |
    v
Automatic / Conditional / Fast Select
    |
    v
Server validation
    |
    v
Commit
    |
    v
TurnLog
```

La Reaction Opportunity deve essere inviata soltanto al giocatore/team autorizzato.

Non replicare planning o opportunità private su Actor globali.

---

# 34. TurnLog

Registrare almeno:

```text
OverwatchArmed
OverwatchTriggerDetected
ReactionOpportunityCreated
ReactionHeld
ReactionCommitted
ReactionExpired
ReactionCancelled
OverwatchChargeConsumed
ReactionTimeout
```

Per ogni evento includere reason code e ordinamento necessario all'explainability.

---

# 35. Bilanciamento

Overwatch universale introduce tre rischi principali.

## 35.1 Dominanza

Problema:

```text
"Non so cosa fare → Overwatch"
```

Mitigazioni:

- costo-opportunità offensivo;
- charge limitata;
- area/direzione;
- trigger specifici;
- possibilità di azione sprecata.

---

## 35.2 Omogeneizzazione del roster

Problema:

tutti i personaggi sembrano avere la stessa quinta skill.

Mitigazione:

> Overwatch comune, effetto specifico.

Differenziare:

- area;
- trigger;
- response;
- phase;
- detection;
- interaction ambientali.

---

## 35.3 Resolution troppo lenta

Problema:

troppe Fast Reaction interrompono la partita.

Mitigazioni:

- Automatic;
- Conditional;
- Fast Select solo quando la decisione è significativa;
- MaxPrompts;
- trigger aggregation;
- una charge base;
- niente interrupt annidati nell'MVP.

---

# 36. Baseline v0.1 consigliata

Azioni generiche:

```text
Wait
Basic Attack
Interact
Brace
Move
Overwatch
```

Move profiles:

```text
Sneak
Normal
Sprint
```

Overwatch:

```text
Universal Planning Action
Character-specific Profile
1 base charge
3.0 s Fast Reaction when manual
Timeout = HOLD
HOLD keeps Overwatch armed
MaxPrompts data-driven
Simultaneous triggers aggregated
Competes with offensive action
No nested reactions in MVP
```

---

# 37. Acceptance criteria

La feature è corretta quando:

1. tutti i personaggi possono selezionare Overwatch durante Planning;
2. ogni personaggio usa un profilo Overwatch differente configurabile;
3. almeno due tipi di trigger differenti sono supportati;
4. almeno due tipi di response differenti sono supportati;
5. HOLD non consuma la charge;
6. un successivo trigger può creare una nuova opportunity;
7. trigger simultanei vengono aggregati;
8. timeout produce HOLD;
9. il client nemico non riceve opportunità o planning privati;
10. due esecuzioni dello stesso snapshot producono lo stesso TurnLog;
11. Automatic e Fast Select possono coesistere;
12. una Overwatch senza trigger può terminare senza effetto;
13. il Move normale resta l'ultima azione volontaria;
14. la UI mostra chiaramente area, trigger e fasi osservate;
15. il test packaged non mostra leak di informazioni.

---

# 38. Test minimi

## Test 1 — Hold e trigger successivo

```text
Enemy A triggers
Player HOLD
Enemy B triggers
Player COMMIT
```

Assert:

- charge non consumata dopo HOLD;
- charge consumata dopo COMMIT;
- un solo effetto applicato.

---

## Test 2 — Timeout

```text
Opportunity created
No response within 3.0 s
```

Assert:

```text
Result = HOLD
Charge remains
```

---

## Test 3 — Simultaneous targets

```text
Enemy A and B trigger at same micro-step
```

Assert:

- una sola opportunity;
- entrambi i target disponibili;
- nessuna dipendenza dall'ordine array.

---

## Test 4 — Automatic profile

```text
Trigger valid
Policy = Automatic
```

Assert:

- nessuna finestra UI necessaria;
- reaction deterministica;
- TurnLog completo.

---

## Test 5 — Privacy

Inserire canary data negli intenti nemici.

Assert:

- nessun canary nel payload dell'Opportunity inviata all'altra squadra;
- nessun futuro trigger visibile.

---

## Test 6 — No trigger

```text
Overwatch armed
No enemy enters valid condition
```

Assert:

- nessun effetto;
- nessuna compensazione automatica;
- TurnLog esplicativo.

---

# 39. Integrazione con documentazione esistente

Questa specifica deve aggiornare o integrare:

```text
RT_PDR_05_Simulazione_Deterministica
RT_PDR_07_Abilita_Personaggi_GAS
RT_PDR_08_UI_UX_Coordinazione
RT_PDR_04_Networking_Privacy
RefactorTactics_Overwatch_FastReaction
RefactorTactics_Rumore
```

Decisioni più recenti da considerare prevalenti:

1. **Overwatch è universale come azione.**
2. L'effetto di Overwatch è definito dal profilo del personaggio/equipaggiamento.
3. **FastReactionDuration default = 3.0 secondi.**
4. HOLD non consuma la charge.
5. Overwatch compete normalmente con l'azione offensiva.
6. Automatic / Conditional / Fast Select sono policy distinte.
7. I trigger possono appartenere a fasi differenti.
8. Move normale resta sempre l'ultima azione volontaria.

---

# 40. Questioni ancora aperte

Da non bloccare senza playtest:

- effetto esatto di Brace;
- numeri definitivi Sneak/Move/Sprint;
- valore definitivo MaxPrompts;
- quali condizioni siano ammesse nella policy Conditional;
- numero massimo di Overwatch contemporanee per team;
- quali profili usare per i quattro personaggi della vertical slice;
- eventuale costo risorsa oltre allo slot offensivo;
- comportamento a fine turno dopo tutti HOLD;
- quali reaction possano interrompere Dash, Attack o Interact;
- eventuali modificatori derivati da equipaggiamento.

---

# 41. Decisione finale sintetica

```text
RefactorTactics usa una grammatica comune di azioni.

Generic Actions:
    Wait
    Basic Attack
    Interact
    Brace
    Move
    Overwatch

Move:
    Sneak
    Normal
    Sprint

Overwatch:
    universal framework
    +
    character/equipment profile
    +
    explicit trigger
    +
    reaction policy
    =
    character-specific reaction
```

La conseguenza di design desiderata è:

> Il giocatore impara una sola volta cosa significa "andare in Overwatch", ma deve conoscere il personaggio per sapere **che cosa sta sorvegliando e come può reagire**.
