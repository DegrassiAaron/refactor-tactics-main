# RefactorTactics — Overwatch, Fast Action e Fast Reaction
## Specifica di design e implementazione per Claude

### Contesto progetto
RefactorTactics è un tattico competitivo PC-first sviluppato in Unreal Engine 5, basato su turni simultanei.

Principi rilevanti:
- Planning privato per squadra.
- Resolution deterministica basata su snapshot immutabile.
- Il client propone, il server valida e applica.
- Nessuna informazione privata del planning avversario deve essere replicata ai client nemici.
- Le animazioni non determinano gli esiti.
- La simulazione produce eventi; i client riproducono animazioni, VFX e UI.
- Le azioni durante la Resolution devono essere brevi e leggibili.
- La UI distingue informazioni Confermate, Previste e Incerte.
- Il sistema deve supportare Fog of War, stealth, terreno, cover, quota e LOS.
- Stesso snapshot + regole + versione + seed = stesso risultato.

---

# 1. Obiettivo

Implementare Overwatch come primo caso concreto di un sistema generale di reaction.

Overwatch NON deve essere una semplice abilità speciale hard-coded.

Deve essere modellata come:

Reaction Definition
+
Player Intent
+
Snapshot
+
Reaction Opportunities
+
Commit opzionale
=
Reaction Resolution

Il sistema deve essere riutilizzabile in futuro per:
- Overwatch
- Guard
- Counter
- Dodge
- Intercept
- Ambush
- Trap
- Opportunity Attack
- Counterspell
- Shield Reaction
- Hack Reaction
- Environmental Reaction

---

# 2. Concetto chiave di Overwatch

Durante il Planning il giocatore prepara Overwatch.

Esempio:

Overwatch
- area/cone
- direzione
- range
- trigger
- charges
- durata
- reaction policy

Durante la Resolution l'abilità viene armata.

Quando un nemico soddisfa le condizioni, viene generata una Reaction Opportunity.

Il giocatore decide in una finestra molto breve se consumare Overwatch oppure lasciar passare il bersaglio.

IMPORTANTE:

Se il giocatore sceglie HOLD:
- perde soltanto quella specifica opportunità;
- Overwatch rimane armata;
- una successiva unità può generare un nuovo trigger;
- il giocatore non sa necessariamente se arriverà un'altra occasione.

Esempio:

Enemy A entra nell'area
→ Fast Reaction
→ FIRE oppure HOLD

HOLD

Enemy B entra
→ Fast Reaction
→ FIRE oppure HOLD

HOLD

Enemy C entra
→ Fast Reaction
→ FIRE

Questo crea una decisione di commitment:

"Uso ora una risorsa limitata oppure rischio di aspettare un bersaglio migliore?"

---

# 3. Fast Reaction Window

Default di sistema:

FastReactionDuration = 3.0 secondi

La finestra deve essere MOLTO breve.

Fast Reaction non è una seconda fase di Planning.

Deve essere:
- immediata;
- leggibile;
- con pochissime opzioni;
- senza menu complessi;
- senza navigazione profonda;
- adatta a decisioni sotto pressione.

Per Overwatch standard:

[ FIRE ]
[ HOLD ]

Countdown:

3.0
2.0
1.0
0.0

Default in caso di timeout:

Timeout -> HOLD

Motivazione:
FIRE consuma una risorsa irreversibile.
Un timeout non deve automaticamente spendere la reaction.

---

# 4. Fast Action vs Fast Reaction

Usare lo stesso sistema tecnico di Decision Window, ma distinguere semanticamente i due casi.

## Fast Action

Decisione rapida generata come continuazione di una propria azione.

Esempio:

Ability risolta
→ scegli entro 3 secondi:
- DASH LEFT
- DASH RIGHT

## Fast Reaction

Decisione rapida generata da un evento/trigger esterno durante la Resolution.

Esempio:

Enemy enters Overwatch
→ entro 3 secondi:
- FIRE
- HOLD

Entrambe possono usare una struttura comune:

FRTDecisionWindow / FRTFastSelectWindow

ma devono avere un tipo esplicito.

Esempio:

enum class ERTFastDecisionType
{
    Action,
    Reaction
};

---

# 5. Overwatch MVP

Prima versione rigorosamente limitata.

Overwatch v0.1:

- scelta durante Planning;
- cono direzionale;
- range fisso;
- richiede LOS;
- trigger su EnemyEnterArea;
- massimo 1 charge;
- trigger valutato per ogni micro-step di movimento;
- può generare più Reaction Opportunities fino al consumo/scadenza;
- Fast Reaction da 3 secondi;
- FIRE consuma la charge;
- HOLD mantiene l'abilità armata;
- timeout = HOLD;
- nessun interrupt annidato;
- cancellabile da KO/Stun;
- deterministicamente ordinata;
- supporto a massimo N prompt configurabile;
- nessuna probabilità richiesta per l'MVP.

Baseline proposta:

FastReactionDuration = 3.0 s
MaxPromptsPerReaction = 3
DefaultTimeoutBehavior = Hold
Charges = 1

MaxPromptsPerReaction deve essere data-driven.

---

# 6. Reaction Opportunity

Non modellare FastSelect come "interrupt arbitrario".

Creare un concetto esplicito:

FRTReactionOpportunity

La semantica è:

"È comparsa un'opportunità valida. Vuoi consumare la reaction?"

Una reaction può quindi produrre:

Reaction
→ 0..N Reaction Opportunities
→ 0..1 Commit

per una reaction con singola charge.

Una Reaction Opportunity deve contenere soltanto informazioni valide per l'istante simulativo corrente.

Esempio concettuale:

struct FRTReactionOpportunity
{
    OpportunityId;
    ReactionInstanceId;
    OwningUnitId;
    TriggeringUnitIds;
    AllowedResponses;
    CurrentSimulationStep;
    Deadline;
};

NON deve contenere:
- future trigger;
- future positions;
- altre informazioni provenienti dal futuro dello snapshot;
- intenti nemici privati.

---

# 7. Privacy e anti-information leak

Questo requisito è CRITICO.

Il server può conoscere l'intero snapshot e gli intenti canonici.

Il client NON deve ricevere informazioni sul futuro della Resolution che il giocatore non dovrebbe conoscere.

Quando Enemy A genera un trigger, il client deve ricevere solo:

- Enemy A se visibile/legittimamente noto;
- stato pubblico corrente;
- opzioni legali;
- durata della finestra;
- dati necessari alla UI.

NON inviare:

- "ci saranno altri due trigger";
- percorsi futuri;
- destinazioni future;
- future Reaction Opportunities;
- intenti privati degli avversari.

Il protocollo deve comportarsi come se il giocatore osservasse la Resolution in tempo reale.

Server:

CanonicalIntentStore
        |
        v
Authoritative Snapshot
        |
        v
Action Resolver
        |
        v
Current Micro-Step
        |
        v
Reaction Trigger
        |
        v
Reaction Opportunity sanitizzata
        |
        v
Owning Client / Team Client
        |
        v
FIRE / HOLD
        |
        v
Server Validation + Commit

---

# 8. Trigger multipli nel tempo

Esempio:

Tank
→ trigger #1
→ HOLD

Scout
→ trigger #2
→ HOLD

Carry
→ trigger #3
→ FIRE

Il sistema non deve informare in anticipo il giocatore che esistono tre trigger.

Questo abilita:
- bait;
- bluff tattico;
- mindgame;
- uso del tank per "bruciare" la reaction;
- attesa di un bersaglio più importante;
- rischio di perdere completamente la reaction.

Se MaxPromptsPerReaction = 3 e il giocatore sceglie HOLD anche al terzo:

Reaction expires

oppure segue la policy configurata dalla specifica abilità.

---

# 9. Trigger simultanei nello stesso micro-step

Caso importante.

Se due o più unità generano il trigger nello stesso micro-step simulativo, NON creare prompt sequenziali artificiali basati sull'ordine di iterazione.

Esempio errato:

Enemy A
→ prompt

poi Enemy B
→ prompt

se A e B sono entrati nello stesso step logico.

Creare invece una singola Reaction Opportunity:

TriggeringTargets:
- Enemy A
- Enemy B

UI:

OVERWATCH

[ FIRE A ]
[ FIRE B ]
[ HOLD ]

Il giocatore può scegliere solo fra i target realmente validi nello stesso boundary simulativo.

Questo evita:
- vantaggi derivanti dall'ordine di iterazione;
- leak temporali;
- nondeterminismo;
- comportamenti dipendenti da TMap/TSet.

---

# 10. Determinismo

Non affidarsi mai a:
- frame rate;
- timing delle animazioni;
- ordine implicito di TMap;
- ordine di arrivo dei pacchetti;
- tick client.

Quando più reaction scattano nello stesso micro-step:

1. raccogliere tutte le reaction valide;
2. creare gli opportunity set;
3. ordinare con una regola stabile;
4. risolvere secondo priorità deterministica.

Possibile ordine:

ReactionPriority
AbilityPriority
UnitInitiative
StableUnitId
ReactionInstanceId

L'ordine esatto deve essere data-driven dove opportuno, ma bloccato per ranked.

---

# 11. Resolution pipeline

Pipeline iniziale:

1. Persistent effects / reactions setup
2. Arm reactions
3. Movement micro-step
   3.1 transition
   3.2 environmental effects
   3.3 occupancy
   3.4 visibility / LOS update
   3.5 evaluate reaction triggers
   3.6 create Reaction Opportunities
   3.7 resolve Fast Reaction decision windows
   3.8 apply committed reactions
4. Control / Defense / Interruptions
5. Attacks / Abilities
6. Environmental propagation
7. KO / Objectives
8. Cooldowns / Cleanup
9. TurnLog

Il controllo Overwatch deve avvenire durante i micro-step, non soltanto a fine movimento.

---

# 12. Presentazione durante i 3 secondi

Durante la Fast Reaction:

- la simulazione autorevole si ferma su un decision boundary;
- il gameplay logico NON avanza;
- il client può continuare a mostrare animazioni/VFX in slow-motion;
- il countdown è in tempo reale;
- la decisione viene inviata al server;
- il server valida;
- la resolution riparte.

Possibile presentazione:

Simulation:
PAUSED AT DECISION BOUNDARY

Presentation:
10-20% speed

UI:
3 second real-time countdown

La slow-motion è soltanto presentazione.

NON deve influenzare:
- esiti;
- seed;
- ordine azioni;
- collisioni;
- path;
- timing logico.

---

# 13. Fog of War e Overwatch

Overwatch deve rispettare lo stato di informazione della squadra.

Possibili casi UI:

## Confermato
La squadra ha visto il nemico preparare Overwatch.

## Previsto
Deducibile da informazioni pubbliche o comportamento noto.

## Incerto
La squadra ha perso visibilità o non possiede informazioni sufficienti.

Non mostrare una Overwatch nemica se la sua esistenza è privata e mai osservata.

Il warning del path deve usare soltanto:
- stato pubblico;
- informazioni osservate;
- intenti della propria squadra.

---

# 14. Stealth

Una unità in Overwatch non vede automaticamente qualunque target nel cono.

Condizione base:

TargetInsideArea
AND
HasLineOfSight
AND
TargetDetected
AND
ReactionStillArmed

Lo stealth può impedire il trigger se il target non viene rilevato.

In futuro separare chiaramente:
- LOS geometrica;
- Detection;
- Visibility;
- Awareness.

---

# 15. Terreno e interazioni

Overwatch deve interagire con il sistema ambientale.

## Fumo / vapore
Possono bloccare o degradare LOS.
Se il target non è più valido, Overwatch non scatta.

## Cover
La cover può impedire o modificare l'attacco.
Se viene distrutta prima nello stesso ordine di resolution, la LOS/validità viene ricalcolata.

## Ghiaccio
Un colpo di Overwatch può causare effetti indiretti:
- suppression;
- perdita stabilità;
- slip;
- interruzione sprint.

## Quota
La quota può modificare:
- LOS;
- celle visibili;
- copertura efficace;
- range effettivo.

Evitare bonus danno automatico alle alture salvo design esplicito.

## Porte, ponti, tunnel, ascensori
Sono ottimi choke point per reaction direzionali.

---

# 16. Overwatch direzionale

Preferire un cono/arco rispetto a un raggio 360°.

Esempio:

             X X X
          X X X X X
Shooter → X X X X X
          X X X X X
             X X X

Benefici:
- facing rilevante;
- flank possibile;
- maggiore leggibilità;
- sinergia con choke point;
- valore della quota;
- uso tattico di porte/corridoi/ponti.

---

# 17. Counterplay

Overwatch armata può essere invalidata da:

- KO;
- Stun;
- Disarm;
- Knockdown;
- Forced Movement;
- perdita dell'arma;
- perdita dei requisiti;
- cambio di stato;
- altri effetti esplicitamente dichiarati.

Non deve essere garantita fino a fine turno.

---

# 18. Reaction baiting

Meccanica emergente desiderata.

Una squadra può fare entrare intenzionalmente un bersaglio poco importante nella killzone per tentare di far consumare Overwatch.

Esempio:

Tank
→ Scout
→ Carry

Tank entra:
FIRE / HOLD

Il difensore può aspettare il Carry.

Ma non sa se il Carry passerà davvero.

Questo crea:
- bluff;
- bait;
- controllo spazio;
- pressione psicologica;
- lettura dell'avversario.

Mantenere questa proprietà nel design.

---

# 19. Varianti future

Il sistema deve consentire reaction diverse senza cambiare la pipeline.

## Suppressive Overwatch
Trigger: movimento
Effetto:
- danno basso;
- Slow/Suppression;
- interrompe Sprint;
- penalizza Accuracy.

## Patient Hunter
Può ignorare trigger successivi.
Possibile variante:
+bonus per opportunità saltate, con cap.

## Ambush
Trigger:
enemy becomes exposed / enters zone

FIRE:
rompe stealth

HOLD:
mantiene stealth

## Guard
Trigger:
ally attacked

## Interceptor
Trigger:
projectile crosses controlled area

## Opportunity Attack
Trigger:
enemy leaves adjacency

Queste sono future estensioni, non scope MVP.

---

# 20. Data model suggerito

Usare nomi compatibili con le convenzioni RefactorTactics.

Esempio iniziale:

```cpp
UENUM(BlueprintType)
enum class ERTReactionTrigger : uint8
{
    EnemyEnterArea,
    EnemyLeaveArea,
    EnemyAttack,
    EnemyUseAbility
};

UENUM(BlueprintType)
enum class ERTFastDecisionType : uint8
{
    Action,
    Reaction
};

UENUM(BlueprintType)
enum class ERTReactionResponse : uint8
{
    Commit,
    Hold
};

USTRUCT(BlueprintType)
struct FRTReactionSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName ReactionId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ERTReactionTrigger Trigger = ERTReactionTrigger::EnemyEnterArea;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Range = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxTriggers = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bRequiresLineOfSight = true;
};
```

Overwatch Intent:

```cpp
USTRUCT(BlueprintType)
struct FRTOverwatchIntent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FRTCellId Origin;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Direction = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 ArcWidth = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName ReactionId;
};
```

Questi frammenti sono una base concettuale.
Prima di usarli, verificare le API e le convenzioni effettive del progetto UE5.

---

# 21. Stato runtime suggerito

Una Reaction Instance runtime può contenere:

- ReactionInstanceId
- OwnerUnitId
- ReactionDefinitionId
- ArmedAtStep
- RemainingCharges
- PromptCount
- HoldCount
- ExpirationBoundary
- CurrentState
- StablePriority
- SourceIntentId

Possibili stati:

Inactive
Armed
OpportunityPending
Committed
Resolved
Expired
Cancelled

---

# 22. Networking

Principio:

Client proposes.
Server validates.
Server applies.

Fast Reaction flow:

1. Server rileva trigger.
2. Server costruisce opportunity sanitizzata.
3. Server invia opportunity solo al client/team autorizzato.
4. Client mostra UI per 3 secondi.
5. Client invia:
   - Commit(target)
   - Hold
6. Server verifica:
   - OpportunityId;
   - ownership;
   - scadenza;
   - target valido;
   - reaction ancora armata;
   - charge disponibile.
7. Server registra la scelta nel TurnLog.
8. Server risolve o continua.

Ready/commit importanti:
- Reliable.

Preview non critica:
- eventualmente unreliable/sequenziata.

Non replicare Reaction Opportunity avversarie a client non autorizzati.

---

# 23. Timeout server-authoritative

Il timeout deve essere deciso dal server.

Il client mostra il countdown, ma non decide autonomamente quando l'opportunità è scaduta.

Il server mantiene:
- Opportunity start;
- deadline;
- risposta valida/non valida.

Se nessuna risposta valida arriva entro il limite:

Response = Hold

Registrare nel log:

ReactionOpportunityTimeout
ReactionId
OpportunityId
OwnerUnitId
SimulationStep

---

# 24. TurnLog

Registrare almeno:

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

Il log deve permettere:
- replay;
- debug;
- verifica deterministica;
- spiegazione UI dell'esito.

Non registrare nei dati visibili ai client avversari informazioni private che non possedevano al momento.

---

# 25. Test automatici richiesti

Creare Automation Tests almeno per:

## Test 1
Enemy entra nel cono.
Opportunity generata.

## Test 2
Enemy fuori cono.
Nessuna opportunity.

## Test 3
Enemy nel cono ma senza LOS.
Nessuna opportunity.

## Test 4
Player sceglie HOLD.
Reaction rimane Armed.

## Test 5
Secondo enemy entra dopo HOLD.
Nuova opportunity generata.

## Test 6
Player sceglie FIRE.
Charge consumata.
Reaction passa a Resolved/Expired.

## Test 7
Timeout.
Risultato = HOLD.

## Test 8
Due enemy entrano nello stesso micro-step.
Una sola opportunity con due target.

## Test 9
KO dell'owner prima del trigger.
Reaction cancelled.

## Test 10
Ordine identico con stesso snapshot/seed.
TurnLog identico.

## Test 11
Client avversario non riceve opportunity o dati privati.

## Test 12
MaxPrompts raggiunto.
Reaction segue policy di scadenza.

---

# 26. Debug tools

Aggiungere debug overlay o console command per visualizzare:

- Overwatch cone;
- range;
- owner;
- reaction state;
- remaining charges;
- prompt count;
- current opportunity;
- LOS;
- trigger reason;
- resolution micro-step;
- stable priority.

Colorazione/debug solo in sviluppo.

Mai usare il debug rendering come fonte della simulazione.

---

# 27. UX

Fast Reaction deve essere leggibile in meno di un secondo.

Overwatch UI consigliata:

OVERWATCH

Target:
EnemyName

Info sintetiche:
- HP;
- cover;
- distanza;
- eventuale status rilevante.

Azioni:

[ FIRE ]
[ HOLD ]

Countdown molto evidente:

3.0 s

Niente:
- inventario;
- menu skill completo;
- path editing;
- target futuri;
- informazioni non note.

La scelta deve poter essere fatta rapidamente anche con tastiera/controller.

---

# 28. Principio di design da preservare

FastSelect non deve diventare Planning 2.0.

La sua identità è:

VEDI L'OPPORTUNITÀ
→ VALUTA RAPIDAMENTE
→ COMMIT
oppure
→ LASCIA PASSARE

Overwatch è interessante proprio perché HOLD non garantisce una seconda occasione.

Il giocatore può:
- sparare subito;
- aspettare;
- essere baitato;
- ottenere un bersaglio migliore;
- perdere completamente la reaction.

Questa incertezza deve essere preservata.

---

# 29. Scope di implementazione per Claude

Implementare inizialmente solo:

1. base Reaction System;
2. Overwatch v0.1;
3. EnemyEnterArea;
4. directional cone;
5. LOS requirement;
6. micro-step trigger evaluation;
7. FRTReactionOpportunity;
8. FIRE/HOLD;
9. finestra server-authoritative da 3 secondi;
10. timeout = HOLD;
11. massimo 3 prompt configurabile;
12. simultaneous targets;
13. TurnLog;
14. debug visualization;
15. Automation Tests;
16. privacy/network test.

NON implementare ancora:
- Fast Action complesse;
- nested reactions;
- chain reaction;
- suppression avanzata;
- Patient Hunter;
- Ambush;
- reaction AI;
- reaction prediction UI avanzata;
- ranked tuning;
- modding pubblico.

---

# 30. Output richiesto a Claude

Quando implementi questa feature:

1. dichiara versione UE5 e assunzioni;
2. indica file nuovi/modificati;
3. mostra dipendenze con Turn Manager, Snapshot, Movement Resolver, LOS e Networking;
4. usa la soluzione più semplice scalabile;
5. fornisci codice C++ compilabile;
6. limita nuove macro Unreal;
7. spiega setup Editor;
8. spiega come provare Overwatch in L_DevSandbox;
9. aggiungi Automation Tests;
10. aggiungi logging/debug;
11. verifica server/client;
12. verifica zero leak degli intenti o dei trigger futuri;
13. proponi commit Git;
14. indica il prossimo passo.

Non inventare API Unreal Engine.
Segnala qualsiasi parte dipendente dalla versione UE5.
Non usare Blueprint come autorità della simulazione.
