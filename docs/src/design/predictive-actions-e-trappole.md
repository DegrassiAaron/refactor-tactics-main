# REFACTORTACTICS — Predictive Actions, Traps & Tactical Gambits
## Prompt operativo per Claude Code: consolidamento documentazione, codice, test, issue e roadmap

> **Scopo di questo file**  
> Usare questo documento come task/prompt per Claude Code dentro la repository RefactorTactics.  
> Claude deve analizzare la repository reale, consolidare le nuove decisioni di design relative a trappole, azioni predittive e “scommesse tattiche”, aggiornare la documentazione esistente, proporre/implementare il codice coerente con la milestone corrente e creare/aggiornare le issue integrate nella roadmap.

---

# 1. Prima di modificare qualsiasi cosa

1. Analizza la repository completa.
2. Leggi `AGENTS.md`, `CLAUDE.md`, `README.md` e altre istruzioni locali se presenti.
3. Considera `Docs/` e i documenti di design/versionati nella repository come fonte di verità.
4. Individua in particolare documentazione e codice relativi a:
   - architettura UE5;
   - simulazione deterministica;
   - ordine delle fasi del turno;
   - planning;
   - Fast Action / Fast Reaction;
   - Overwatch;
   - mappe, celle, archi e pathfinding;
   - Fog of War e percezione;
   - rumore;
   - abilità e GAS;
   - TurnLog;
   - automated scenario testing;
   - roadmap e milestone;
   - issue/backlog esistenti.
5. Usa la versione Unreal Engine realmente bloccata nella repository.
6. Non inventare API Unreal.
7. Se la documentazione contiene conflitti, **non scegliere silenziosamente una versione**:
   - elenca i conflitti;
   - indica quale documento sembra più recente/autorevole;
   - proponi una decisione;
   - aggiorna i documenti solo dopo aver reso esplicita la scelta nel changelog/commit.
8. Non rinominare arbitrariamente personaggi, abilità o sistemi già consolidati nella repository.
9. Mantieni lo scope coerente con la milestone corrente: design e fondamenta possono essere consolidate ora, ma non introdurre sistemi troppo avanzati prima dei prerequisiti.

---

# 2. Contesto di design consolidato

RefactorTactics è un tattico competitivo PC-first a turni simultanei.

Principi rilevanti:

- Planning privato per squadra.
- Ordine principale delle fasi ispirato ad Atlas Reactor:
  - `Planning / Decision`
  - `Prep`
  - `Dash`
  - `Blast`
  - `Move`
- **Il normale Move è sempre l'ultima fase volontaria del turno.**
- Dash e spostamenti speciali possono avvenire prima perché appartengono alla loro fase.
- Snapshot immutabile per la resolution.
- Resolver autorevole e deterministico.
- Animazioni/VFX non determinano il risultato.
- Il client propone; il server valida e applica.
- Fog of War e informazione incompleta sono parte fondamentale del gameplay.
- La UI distingue:
  - Confermato;
  - Previsto;
  - Incerto.
- Gli intenti avversari non vengono mai replicati ai client nemici.
- Stesso snapshot + rules/version + config/hash + seed => stesso stato finale e stesso TurnLog canonico.
- La mappa è un grafo tattico esagonale/multilivello con celle e archi come dati di prima classe.
- Overwatch e le Fast Reaction sono già pensate come sistema generale di `Reaction Opportunity`.
- Il rumore è una risorsa informativa e può essere usato come trigger senza rivelare intenti nascosti.

---

# 3. Nuova decisione di design: famiglia delle Predictive Actions

Introdurre formalmente una famiglia generale di meccaniche provvisoriamente chiamata:

- **Predictive Actions**
- oppure **Tactical Gambits**
- oppure altro nome coerente con il vocabolario già presente nella repository.

Non fissare il nome finale senza verificare naming e tassonomia esistenti.

## Concetto

Durante il Planning il giocatore dedica un'azione, una risorsa, una stance o un dispositivo a una previsione sul comportamento avversario.

Esempio concettuale:

> “Scommetto che un nemico attraverserà questa cella durante Move.”

Se la previsione è corretta:
- si attiva un payoff superiore a quello di un'azione generica equivalente.

Se la previsione è errata:
- l'azione può essere sprecata;
- può persistere;
- può essere parzialmente rimborsata;
- può consumare soltanto una charge;
- dipende dalla definizione data-driven.

Il valore strategico deve derivare dalla **lettura dell'avversario**, non da RNG nascosto.

---

# 4. Obiettivo sistemico

Le Predictive Actions non devono diventare una collezione di abilità hard-coded.

Serve una grammatica riutilizzabile che possa supportare:

- trappole;
- mine;
- tripwire;
- attacchi di intercetto;
- anti-dash;
- stance punitive;
- guardie predittive;
- counter preparati;
- ambush;
- sabotaggi;
- dispositivi ambientali;
- reazioni a rumore;
- interaction traps;
- control zones;
- ability punish;
- target prediction;
- path prediction;
- future character kits;
- gadget e unità ausiliarie leggere.

Non è necessario implementare subito ogni categoria.

---

# 5. Distinzione obbligatoria tra tre pattern

Non unificare semanticamente tutto sotto “Reaction”.

## A. Predictive Action

La decisione è completamente presa durante il Planning.

Esempio:

```text
IF Enemy enters Cell H7 during Move
THEN Fire at that enemy
```

Quando il trigger avviene:

```text
Trigger
-> Validation
-> Resolve effect
-> TurnLog
```

Nessun nuovo input umano.

---

## B. Trap / Persistent Triggered Effect

Il giocatore crea un oggetto, stato, dispositivo, hazard o condizione persistente.

Esempio:

```text
Place Mine H7
-> Armed
-> Enemy enters H7
-> Trigger
-> Explosion
```

La trappola può:

- durare uno o più turni;
- avere charge;
- essere visibile, rilevabile o nascosta secondo regole esplicite;
- essere disinnescabile;
- essere distruttibile;
- reagire a celle, archi, azioni, rumore o interazioni.

---

## C. Fast Reaction

Il giocatore ha preparato una capacità, ma sceglie quando compare una `Reaction Opportunity`.

Esempio Overwatch:

```text
Enemy enters Overwatch
-> Reaction Opportunity
-> FIRE / HOLD
```

Questa famiglia mantiene il sistema di Fast Reaction già definito.

### Regola UX importante

NON trasformare ogni trappola o predictive action in una Fast Reaction.

La Resolution non deve diventare una sequenza continua di pause.

Usare Fast Reaction solo quando esiste una scelta interessante **al momento del trigger**.

---

# 6. Tassonomia delle previsioni

Il sistema deve poter rappresentare almeno concettualmente queste categorie.

| Categoria | La previsione riguarda | Esempio |
|---|---|---|
| Position | presenza in una cella | Interception Shot |
| Path | attraversamento di un arco/percorso | Tripwire |
| Movement | uso del Move | Punish Movement |
| Dash | uso/attraversamento durante Dash | Anti-Dash Snare |
| Attack | esecuzione di un attacco | Return Fire / Punish Attack |
| Ability | utilizzo di una ability | Disruption Protocol |
| Target | attacco verso un alleato specifico | Guardian Read |
| Direction | ingresso da una direzione/settore | Directional Ambush |
| Interaction | uso di porta, console, relay, objective | Sabotaged Console |
| Surface | ingresso/stato su acqua, ghiaccio, fuoco ecc. | Conductive Trap |
| EndPosition | posizione finale del Move | Delayed Blast |
| Noise | evento acustico sopra soglia | Acoustic Ambush |
| Visibility/Detection | acquisizione/perdita visibilità | Ambush/Reveal trap |
| Projectile/Trajectory | attraversamento di settore/traiettoria | Interceptor |

La tassonomia finale deve usare Gameplay Tags governati o enum/ID dove appropriato, evitando una gerarchia rigida e ingestibile.

---

# 7. Principio di bilanciamento: precisione vs payoff

Regola di design proposta:

> **Più precisa è la previsione richiesta, più alto può essere il payoff.**

Esempio:

```text
“Punisci qualunque nemico che si muove”
-> previsione larga
-> payoff basso

“Punisci un nemico che entra in questa zona”
-> precisione media
-> payoff medio

“Punisci questo specifico nemico che attraversa questa linea”
-> precisione alta
-> payoff alto

“Punisci questo specifico nemico se attraversa questa specifica cella durante Move”
-> precisione molto alta
-> payoff molto alto
```

Questa relazione deve restare una linea guida di balance, non necessariamente una formula rigida.

---

# 8. Archetipi iniziali da usare per il prototipo

Non implementare subito decine di trigger.

Per validare il sistema bastano tre archetipi:

## 8.1 Intercept Cell

```text
Planning:
Choose Cell

Trigger:
EnemyEnterCell

Phase:
Move

Payoff:
Attack / damage / suppression

Failure:
Consumed / Whiff
```

Serve a testare:
- cell trigger;
- fase;
- target dinamico derivato dal trigger;
- whiff;
- TurnLog.

---

## 8.2 Tripwire / Edge Trap

```text
Planning:
Choose Edge A -> B

Trigger:
EnemyCrossEdge

Phase:
Dash or Move

Payoff:
Stop / Root / Damage / Reveal

Failure:
Persist or expire by rule
```

Serve a testare:
- archi come entità logiche;
- direzionalità;
- porte/ponte/tunnel futuri;
- interaction con movimento per micro-step.

---

## 8.3 Punish Action

```text
Planning:
Choose target/unit/area

Prediction:
Attack | Ability | Dash

Trigger:
Target performs predicted action

Payoff:
Counter / debuff / exposed / damage / interrupt-lite

Failure:
Whiff / partial persistence
```

Serve a testare:
- trigger non spaziali;
- action event model;
- bluff;
- distinzione tra Predictive Action e Reaction.

---

# 9. Esempi futuri da documentare, NON necessariamente da implementare ora

## Proximity Mine
Trigger: `EnemyEnterCell`.

## Anti-Dash Snare
Trigger: `EnemyCrossArea` durante `Dash`.
Il normale `Move` non attiva la trappola.

## Counter Battery
Trigger: attacco ranged verso un alleato protetto.

## Guardian Read
Il giocatore predice quale alleato verrà attaccato.
Payoff maggiore di una guardia generica perché la previsione può fallire.

## Disruption Protocol
Predice che un bersaglio utilizzerà una Ability in una fase specifica.

## Live Wire
Trigger: nemico entra in una regione Wet/connessa preparata.
Possibile propagazione elettrica ambientale.

## Acoustic Mine
Trigger:
```text
Noise.Movement >= Threshold
within controlled area
```

Sneak può non attivarla.
Sprint/Dash può attivarla.

## Sabotaged Interaction
Trigger:
- apertura porta;
- uso generator;
- relay;
- objective console;
- ascensore;
- switch.

## Delayed End-Position Blast
Scommessa sulla posizione finale di Move, non sulle celle attraversate.

---

# 10. Bluff, bait e counterplay come proprietà desiderate

Il sistema deve produrre gameplay emergente, non soltanto danno automatico.

Esempio:

```text
Team A mette pressione su un ponte.
Team B sospetta una trap.
Team B:
- devia;
- manda il tank;
- usa un drone/decoy;
- usa Scan;
- forza il trigger con una unità sacrificabile;
- usa Dash o Move a seconda della condizione;
- cambia target;
- rinuncia temporaneamente all'azione.
```

## Regola importante

Una trap può essere efficace anche **senza scattare**.

Se controlla una zona e forza il nemico a:
- deviare;
- rallentare;
- spendere risorse;
- usare Scan;
- cambiare piano;

ha già prodotto valore strategico.

---

# 11. Fog of War, visibilità e anti-frustrazione

Evitare un meta basato su:

```text
cammino
-> esplosione invisibile enorme
-> nessuna informazione precedente
-> nessun counterplay
```

Le trappole devono avere regole esplicite di informazione.

Usare il modello esistente:

- **Confermato**
- **Previsto**
- **Incerto**

Possibili stati di conoscenza:

```text
Confirmed:
trap osservata / rilevata.

Predicted:
presenza plausibile dedotta da informazione lecita.

Unknown:
nessuna informazione.
```

Possibili sistemi di counterplay:

- Scout / detection;
- hearing/perception;
- Scan;
- line of sight;
- trap signature;
- terreno modificato;
- dispositivi visibili;
- rumore;
- distruzione remota;
- path alternativo;
- unità ausiliarie;
- disarmo.

Non deve esserci alcun leak di intenti avversari.

---

# 12. Rumore come trigger predittivo

Integrare il sistema con il modello di rumore esistente.

Esempio:

```text
Acoustic Trap
Area: radius N
Trigger:
Noise.Movement >= 5
```

Possibili conseguenze:

- Sprint attiva la trappola;
- Sneak la evita;
- un decoy sonoro può attivarla;
- rumore ambientale può mascherare o alterare condizioni;
- tunnel e superfici modificano la propagazione.

Il trigger deve usare **eventi acustici realmente prodotti dal resolver/perception system**, non gli intenti nemici.

---

# 13. Unità ausiliarie, gadget e bait

Prevedere la compatibilità futura con unità leggere/addizionali:

- drone;
- decoy;
- pet;
- summon semplice;
- gadget mobile;
- dispositivo remoto.

Possibili usi:

- triggerare una mine;
- consumare Overwatch;
- testare una reaction;
- esplorare un choke;
- generare rumore;
- verificare un sospetto.

Non implementare un sistema summon complesso solo per questa feature.
La nuova architettura deve però evitare assunzioni del tipo “ogni trigger coinvolge solo i quattro eroi principali”.

---

# 14. Grammatica concettuale comune

Valutare un modello concettuale tipo:

```text
PREDICTIVE EFFECT

WHEN
    Trigger

WHO
    SubjectPredicate

WHERE
    SpatialPredicate

DURING
    Phase / Window

IF
    AdditionalConditions

THEN
    Effects

ON_TRIGGER
    Consume | KeepArmed | ConsumeCharge

ON_FAILURE
    Consume | Persist | RefundPartial | Expire

VISIBILITY
    Public | Detectable | HiddenWithRules

DURATION
    Phase | Turn | N Turns | UntilTriggered
```

Non implementare necessariamente questa struttura 1:1.

Prima confrontarla con:
- Ability Definition;
- Effect Definition;
- Reaction Spec;
- TurnEvent;
- Gameplay Tags;
- existing data assets.

Preferire composizione e dati riutilizzabili rispetto a enum giganteschi.

---

# 15. Eventi del resolver

Verificare se il TurnLog/event model esistente espone eventi sufficienti.

Il sistema dovrebbe poter valutare trigger da eventi canonici come:

- `MoveStep`;
- `EdgeCrossed`;
- `DashStep`;
- `AttackDeclared`;
- `AttackResolved`;
- `AbilityDeclared`;
- `AbilityResolved`;
- `InteractionUsed`;
- `NoiseGenerated`;
- `VisibilityChanged`;
- `SurfaceChanged`;
- `ProjectileCrossed`;
- `UnitEnteredArea`.

Non creare eventi duplicati se equivalenti esistono già.

Quando possibile:

```text
Resolver Event
-> Trigger Evaluation
-> Triggered Effect
-> New Canonical Events
```

Evitare polling generico dello stato se un evento canonico risolve il problema in modo più deterministico e spiegabile.

---

# 16. Resolution e micro-step

I trigger spaziali devono rispettare la simulazione per micro-step.

Esempio:

```text
Enemy:
H5 -> H6 -> H7 -> H8
```

Se una trap è su H7:

```text
MoveStep H6 -> H7
-> evaluate triggers
-> trap fires
-> apply effect
-> movement may stop/change
```

Non controllare soltanto la destinazione finale.

Per trigger su archi:

```text
CrossEdge(H6, H7)
```

deve essere distinto da:
- ingresso in H7 da un altro lato;
- teleport/phase che non percorre l'arco;
- displacement;
- forced movement;
- Dash;
- Move.

Ogni abilità deve dichiarare chiaramente quali transition types considera validi.

---

# 17. Ordine e determinismo

Quando più trigger scattano allo stesso micro-step:

1. raccogliere trigger validi;
2. raggruppare quelli logicamente simultanei;
3. ordinarli con chiave stabile;
4. risolverli secondo ruleset;
5. produrre reason code;
6. appendere eventi canonici.

Non dipendere da:
- Tick;
- frame;
- ordine TMap/TSet;
- animation notify;
- packet arrival;
- latency;
- actor iteration order.

Se esiste già una priorità delle reaction/effects, riusarla o estenderla invece di creare un secondo scheduler.

---

# 18. Networking e privacy

Requisito critico.

Il server può conoscere:

- intenti canonici;
- stato completo;
- trappole nascoste;
- condizioni future che emergeranno dalla Resolution.

Il client nemico NON deve ricevere:

- trap placement nascosto;
- trigger futuri;
- intenti predittivi avversari;
- “questa cella sarà pericolosa perché il nemico l'ha scelta”;
- numero di trigger futuri;
- target predetto;
- azione predetta;
- path o destination avversari.

La UI deve ricevere solo conoscenza autorizzata.

Per una trap nascosta:

```text
Server canonical state
-> Team Knowledge / Detection
-> Sanitized DTO
-> Client UI
```

Non replicare “hidden=true” con il resto dei dati a tutti i client.

---

# 19. GAS e confine con il resolver

Mantenere il principio esistente:

- GAS:
  - ownership;
  - costi;
  - cooldown;
  - tag;
  - status/effects mirror;
  - validazione planning dove utile.

- Resolver:
  - trigger;
  - condizioni;
  - micro-step;
  - target effettivo;
  - effetti competitivi;
  - ordine;
  - risultato;
  - TurnLog.

Una Predictive Action non deve dipendere da:
- montage;
- anim notify;
- prediction GAS client-side;
- callback temporali presentation-driven.

---

# 20. UI / Planning

Durante il Planning una predictive action deve mostrare chiaramente:

- trigger;
- area/cella/arco;
- fase rilevante;
- durata;
- cosa consuma;
- cosa succede se fallisce;
- condizioni note;
- livello di certezza.

Esempio:

```text
INTERCEPT CELL

Target Cell: H7
Trigger: Enemy enters
Phase: Move
Duration: This Turn
On Trigger: Fire
On Miss: Action lost
```

Per Tripwire:

```text
TRIPWIRE

Edge: H6 -> H7
Triggers On:
- Move
- Dash

Does Not Trigger On:
- Teleport
- Forced Push
```

Usare ghost/overlay leggibili e filtri coerenti con la UI tattica esistente.

---

# 21. Explainability e Combat Log

Il giocatore deve capire perché una predictive action:

- è scattata;
- non è scattata;
- è stata invalidata;
- ha perso il target;
- è stata disarmata;
- ha ignorato un movimento.

Esempi reason code:

```text
TriggerMatched
WrongPhase
WrongTransitionType
SubjectNotEnemy
TargetNotDetected
TrapDisarmed
TrapDestroyed
ConditionNotMet
ActionTypeMismatch
NoiseBelowThreshold
Expired
AlreadyConsumed
NoValidTargetAtTrigger
```

I reason code devono essere dati strutturati, non solo testo localizzato.

---

# 22. Automated Scenario Test Harness

Integrare fin dall'inizio le Predictive Actions nel sistema di test automatico.

Scenari minimi futuri:

## Test 1 — Intercept Cell Hit

```text
Enemy path crosses target cell
Expected:
- trigger matched;
- attack event;
- action consumed;
- deterministic final hash.
```

## Test 2 — Intercept Cell Miss

```text
Enemy uses alternate path
Expected:
- no attack;
- predictive action whiffs/expires according to rule.
```

## Test 3 — Tripwire Correct Edge

```text
Enemy crosses armed edge
Expected:
- trap triggers exactly at crossing micro-step.
```

## Test 4 — Tripwire Wrong Direction/Edge

```text
Enemy enters same destination from another edge
Expected:
- trap does not trigger.
```

## Test 5 — Anti-Dash

```text
Move through zone -> no trigger
Dash through zone -> trigger
```

## Test 6 — Punish Attack

```text
Target attacks -> trigger
Target moves/uses non-matching action -> no trigger
```

## Test 7 — Acoustic Trap

```text
Sneak noise below threshold -> no trigger
Sprint noise >= threshold -> trigger
```

## Test 8 — Determinism

Ripetere scenario N volte:
- stesso `StateHash`;
- stesso `LogHash`;
- stesso trigger order.

## Test 9 — Privacy

Canary trap/intent Team A:
- non deve apparire nei payload Team B prima della rivelazione lecita.

---

# 23. Roadmap: integrazione proposta

NON creare una nuova mega-milestone separata se può essere integrata nelle milestone esistenti.

Usare la roadmap reale della repository come fonte di verità.

In assenza di una roadmap più recente, integrare concettualmente così:

## F0 — Fondazioni

Nessuna implementazione completa delle trap.

Assicurare soltanto che:
- TurnLog sia estendibile;
- micro-step movement sia stabile;
- celle/archi siano identificabili;
- resolver sia event-driven/deterministico;
- automated test harness possa in futuro pilotare gli scenari.

**Exit gate invariato.**

---

## F1 — Rete privata

Preparare classificazione dei dati:

- server-only predictive intent;
- team-only preview propria;
- public only after legitimate reveal;
- privacy test con canary.

Non implementare ancora tutte le trap.

---

## F2 — Abilities / Reactions / Predictive Core

Questa è la milestone naturale per introdurre:

1. modello `Predictive Action / Triggered Effect` minimo;
2. trigger event-driven;
3. `Intercept Cell`;
4. `Punish Action`;
5. integrazione AbilityDefinition/GAS;
6. TurnLog + reason code;
7. scenario automation;
8. eventuale riuso del sistema Reaction senza confonderne la semantica.

### Exit gate aggiuntivo proposto

- due predictive abilities diverse;
- un hit scenario;
- un miss scenario;
- stesso risultato su repeat test;
- nessuna Fast Reaction superflua.

---

## F3 — Mappa multilivello / Environment

Introdurre:

- `Tripwire` su Edge;
- porte;
- ponte;
- tunnel;
- superfici;
- acqua/elettricità;
- noise-triggered trap;
- graph revision e invalidazione se la trap modifica transizioni;
- detection/disarm base se necessario.

### Exit gate aggiuntivo proposto

- edge trap test;
- special transition test;
- water/electric trap test;
- noise threshold test.

---

## F4 — Vertical Slice

Portare il sistema a livello giocabile:

- UX completa;
- telegraphing;
- Confermato/Previsto/Incerto;
- counterplay;
- trap detection;
- almeno un personaggio/gadget con gameplay predittivo;
- bot base capace di considerare rischio pubblico;
- scenario showcase;
- balance pass.

### Exit gate aggiuntivo proposto

Playtest deve verificare:

- leggibilità;
- bluff;
- bait;
- counterplay;
- assenza di frustrazione da trap invisibili;
- durata resolution accettabile;
- numero di prompt Fast Reaction contenuto.

---

# 24. Issue/backlog da creare o aggiornare

Prima cerca issue esistenti per evitare duplicati.

Se GitHub è collegato e il repository/permessi lo consentono, crea/aggiorna issue reali.
Altrimenti genera file/backlog markdown pronto da applicare.

Proposta di epic:

```text
EPIC — Predictive Actions & Trap System
```

Issue figlie indicative:

```text
DESIGN
- Define Predictive Action terminology and taxonomy
- Define predictive payoff/failure policies
- Define trap visibility/detection rules
- Define phase/transition trigger semantics
- Define counterplay and anti-frustration rules

CORE
- Add canonical trigger evaluation contract
- Add stable trigger ordering
- Add trigger reason codes to TurnLog
- Add Predictive Action runtime instance
- Add persistent trap runtime state

ABILITIES
- Implement Intercept Cell prototype
- Implement Punish Action prototype
- Integrate predictive definitions with Ability Data
- Validate GAS/resolver ownership boundary

MAP
- Add EdgeCrossed canonical event if missing
- Implement Tripwire edge prototype
- Support special transition filtering
- Add trap interaction with graph revision where required

NOISE / PERCEPTION
- Expose canonical NoiseGenerated event
- Prototype Acoustic Trap
- Add detection/sanitized team knowledge path

NETWORK
- Classify predictive intent data
- Add team-only preview DTO if needed
- Add trap reveal/detection DTO
- Add canary privacy test

UI
- Add Predictive Action planning preview
- Add cell/edge/phase trigger visualization
- Add trap knowledge states
- Add trigger/fizzle explanation in combat log

TEST
- Add Intercept Hit/Miss scenario
- Add Tripwire edge scenario
- Add Anti-Dash scenario
- Add Punish Action scenario
- Add Acoustic Trap threshold scenario
- Add deterministic repeat tests
- Add packaged privacy test
```

Usare label/milestone reali della repository.

---

# 25. Acceptance criteria generali

La feature non è Done se funziona solo “visivamente”.

Per ogni predictive feature implementata verificare:

1. planning valido;
2. commit valido;
3. snapshot include i dati necessari;
4. resolver valuta il trigger deterministicamente;
5. target/effect risolti senza dipendere dalla presentation;
6. TurnLog contiene trigger e reason code;
7. replay riproducibile;
8. automated test;
9. miss/failure test;
10. privacy test se contiene informazioni nascoste;
11. packaged test;
12. nessun uso di intenti avversari nel client;
13. nessun risultato dipendente da Tick/frame/animation timing.

---

# 26. Definition of Done specifica

Una Predictive Action è Done solo se:

```text
Planning
-> Commit
-> Snapshot
-> Trigger Event
-> Trigger Validation
-> Resolve
-> Logical State
-> TurnLog
-> Presentation
-> Automated Test
```

funziona end-to-end.

Se è una Fast Reaction:

```text
Trigger
-> Reaction Opportunity
-> Decision Window
-> Response
-> Server Validation
-> Resolve
-> TurnLog
```

Se è una trap nascosta:

```text
Canonical Trap State
-> Detection / Knowledge
-> Sanitized DTO
-> UI
```

senza replication leak.

---

# 27. Cose da NON fare

Non:

- creare una seconda simulazione per trap;
- usare collision overlap Actor come autorità competitiva;
- usare `SetActorLocation()` per risolvere gameplay;
- usare timer real-time per trigger competitivi;
- usare Animation Notify per decidere il risultato;
- inviare dati hidden a tutti i client e nasconderli soltanto in UI;
- fare polling generico per ogni trap ogni Tick;
- introdurre un mega-enum con ogni possibile trigger;
- creare subito editor visuale complesso;
- creare subito 30 trap;
- aggiungere RNG nascosto per “chance to trigger”;
- aggiungere Fast Reaction a ogni trigger;
- duplicare eventi già presenti nel TurnLog;
- modificare la roadmap senza mantenere exit gate e dipendenze.

---

# 28. Deliverable richiesti a Claude

Alla fine del lavoro, produrre:

## A. Analisi repository

- file rilevanti trovati;
- classi coinvolte;
- milestone corrente;
- issue esistenti correlate;
- conflitti documentali.

## B. Decisioni consolidate

Elenco netto di:

- decisioni approvate da questo task;
- assunzioni;
- punti ancora aperti;
- elementi rinviati.

## C. Documentazione aggiornata

Aggiornare i documenti reali più appropriati.

Probabili aree:

- Abilities;
- Simulation;
- Map;
- Networking;
- UI/UX;
- Noise/Perception;
- Roadmap/QA;
- Vertical Slice;
- Automated Testing.

Non duplicare la stessa specifica in dieci file.
Preferire:
- una specifica primaria;
- riferimenti incrociati dagli altri documenti.

## D. Codice

Implementare solo ciò che appartiene alla milestone corrente e ha prerequisiti disponibili.

Per ogni file:
- percorso;
- responsabilità;
- dipendenze;
- test.

## E. Test

Aggiungere Automation/Scenario tests pertinenti.

## F. Issue

Creare/aggiornare backlog integrato nelle milestone reali.

## G. Roadmap

Mostrare chiaramente:

```text
Now
Next
Later
```

e la milestone di appartenenza di ogni issue.

## H. Changelog

Aggiornare changelog/decision log se il repository ne usa uno.

---

# 29. Output finale richiesto a Claude Code

Chiudere il task con un report strutturato:

```text
# Summary

# Repository findings

# Documentation changes

# Code changes

# Tests added

# Issues created/updated

# Roadmap integration

# Open questions

# Risks

# Files changed

# Commands executed

# Test results

# Suggested commits
```

Proporre commit piccoli e focalizzati, ad esempio:

```text
docs(gameplay): define predictive actions and trap taxonomy

docs(roadmap): integrate predictive actions into F2-F4 milestones

feat(resolver): add deterministic predictive trigger contract

feat(ability): add intercept cell prototype

test(scenarios): add predictive hit and miss regression cases

feat(map): add edge-triggered tripwire prototype

test(net): add hidden trap privacy canary coverage
```

Non fare un unico mega-commit se il lavoro attraversa documentazione, core, UI e test.

---

# 30. Punto chiave da preservare

Queste meccaniche devono creare:

- lettura dell'avversario;
- bluff;
- contro-bluff;
- bait;
- controllo spazio;
- deviazione del percorso;
- rischio consapevole;
- payoff per previsioni precise;
- counterplay leggibile;
- informazione incompleta lecita.

La finalità NON è aggiungere semplicemente “mine che fanno danno”.

La finalità è creare un sottosistema in cui il giocatore può dire:

> “Penso che farai X, quindi preparo Y.”

e l'avversario può rispondere:

> “So che potresti aver previsto X, quindi cambio comportamento.”

Questa dinamica deve integrarsi con:
- turni simultanei;
- Fog of War;
- rumore;
- ambiente;
- Overwatch;
- Fast Reaction;
- mappa esagonale/multilivello;
- unità ausiliarie/gadget;
- simulazione deterministica;
- networking team-private;
- TurnLog;
- automated testing.

---

# 31. Decisione di scope consigliata

Per la prima implementazione reale limitarsi a:

```text
1. Intercept Cell
2. Punish Action
3. Tripwire Edge
```

Ordine suggerito:

```text
Intercept Cell
-> Punish Action
-> Tripwire Edge
```

Motivazione:

- Intercept Cell valida trigger spaziale semplice.
- Punish Action valida trigger semantico su eventi del resolver.
- Tripwire valida archi/transizioni e prepara F3.

Solo dopo questi tre prototipi valutare:
- Acoustic Trap;
- Live Wire;
- anti-Dash avanzato;
- objective sabotage;
- persistent minefields;
- detection/disarm;
- character specialization;
- bot reasoning su rischio predittivo.

---

**Fine task.**
