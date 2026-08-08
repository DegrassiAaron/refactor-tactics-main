> 📅 **PIANIFICATO per la v0.2** il 2026-08-08 — **non** è materiale della v0.1.
> Diventa **E22** in [`../../../roadmap/roadmap-post-v0.1.md`](../../../roadmap/roadmap-post-v0.1.md), inclusi i 12
> scenari di test e i due core test. Dipende da E9 (coperture) ed E14 (overwatch), che in v0.1 sono P2 e
> ancora aperte: anticiparlo significherebbe costruire su fondamenta non verificate.

# RefactorTactics — Cover Window / Open → Fire → Seal
## Handoff per Claude Code: consolidamento documentazione, roadmap e scenari di test

Data: 2026-08-08  
Progetto: RefactorTactics  
Scopo: consolidare nella repository la decisione di design discussa in questa chat, aggiornare la documentazione e la roadmap, e produrre scenari automatici/visuali per verificarla.

---

# 1. ISTRUZIONI PER CLAUDE

Stai lavorando nella repository **RefactorTactics**.

Prima di modificare qualsiasi file:

1. leggi `AGENTS.md`, `CLAUDE.md` e le istruzioni repository se presenti;
2. individua la versione Unreal Engine realmente bloccata nel progetto;
3. leggi la documentazione corrente, in particolare:
   - sequenza del turno / phase order;
   - simulazione deterministica, snapshot e `TurnLog`;
   - mappa esagonale, archi/transizioni, cover e LOS;
   - abilità/personaggi/GAS;
   - UI/UX, Action Ghost, intenti alleati e livelli di certezza;
   - test automatici / Automated Scenario Test Harness;
   - roadmap e QA;
   - Decision Log / ADR relativi a turno, reaction, facing, cover e ambiente;
4. considera la repository corrente come fonte di verità per nomi di classi, file, tag, ID e stato di implementazione;
5. confronta il contenuto di questo handoff con ciò che è già implementato/documentato;
6. non duplicare specifiche già corrette: consolidale;
7. se trovi conflitti reali, riportali chiaramente e scegli la soluzione coerente con le decisioni più recenti del progetto;
8. non inventare API Unreal e non introdurre scope non necessario.

Obiettivo finale:

- integrare questa meccanica nella documentazione attiva;
- aggiornare la roadmap tecnica e i relativi exit gate;
- aggiungere o aggiornare test automatici;
- creare scenari visuali/scripted adatti a mostrare e verificare la meccanica;
- assicurare che la soluzione resti sistemica, deterministica, data-driven e riutilizzabile.

---

# 2. DECISIONE DI DESIGN

Introduciamo come pattern tattico generale una **finestra temporanea di copertura**, utilizzabile da due o tre elementi della stessa squadra.

Pattern base:

```text
A apre / rimuove / disabilita temporaneamente una copertura
                    ↓
B sfrutta la nuova linea di tiro / LOS
                    ↓
C ripristina / ricrea / richiude la copertura
```

Nome descrittivo consigliato nella documentazione:

**Cover Window**

Nome della combo dimostrativa:

**OPEN → FIRE → SEAL**

Non deve essere una combo hard-coded fra tre personaggi specifici.

Deve emergere da primitive sistemiche già esistenti:

- modifica di cover;
- modifica di un arco/transizione;
- ricalcolo LOS/targeting;
- attacco;
- ripristino/riparazione/ricostruzione della cover;
- ordine di risoluzione deterministico.

---

# 3. ESEMPIO BASE

Tre alleati pianificano nello stesso turno:

```text
Unit A
→ Open / Disable Cover

Unit B
→ Fire through resulting LOS

Unit C
→ Restore / Seal Cover
```

Durante il Planning la squadra deve poter vedere la dipendenza.

Durante la Resolution:

```text
PREP / fase appropriata
A modifica la cover
        ↓
      OPEN

BLAST
B risolve il tiro
        ↓
      HIT / MISS / FIZZLE secondo le regole

boundary appropriato
C ripristina la cover
        ↓
     CLOSED

MOVE
il normale movimento resta l'ultima fase volontaria del turno
```

IMPORTANTE: rispettare sempre l'ordine canonico attuale del progetto.

Baseline concettuale del progetto:

```text
Planning
→ Prep
→ Dash
→ Blast
→ Move
→ Cleanup
```

Se nella repository l'ordine più recente è descritto con nomi più precisi, usare quello.

Il normale `Move` resta l'ultima azione/fase volontaria.

---

# 4. PRINCIPIO FONDAMENTALE: LA COVER È STATO LOGICO

La copertura non è soltanto una mesh o un ostacolo visuale.

Una modifica di cover deve cambiare lo **stato logico autorevole della mappa**.

La mappa di RefactorTactics usa un grafo tattico esagonale/multilivello.

Gli archi/transizioni sono dati di prima classe.

Questa meccanica deve quindi essere modellata tramite dati del grafo / cover state, non tramite hack di presentazione.

Possibile modello concettuale:

```text
Closed
Open
Disabled
Destroyed
```

oppure gli equivalenti già presenti nel codice.

Non introdurre enum duplicati se esiste già un modello compatibile.

Possibili transizioni:

```text
Closed
  │ OpenCover
  ▼
Open
  │ RestoreCover
  ▼
Closed
```

e:

```text
Open
  │ HeavyDamage
  ▼
Destroyed
```

`Destroyed` e `Open` NON sono sinonimi.

- `Open`: stato reversibile.
- `Disabled`: stato temporaneo o funzionalmente spento.
- `Destroyed`: stato strutturale non ripristinabile con una semplice chiusura.

---

# 5. DURATA DELLA MODIFICA

Ogni effetto che modifica una cover deve dichiarare in modo esplicito quando termina.

Esempi di policy concettuali:

```text
UntilEndOfPrep
UntilEndOfBlast
UntilEndOfTurn
UntilExplicitlyRestored
Permanent
```

Non implementare tutti questi casi se il modello corrente non li richiede.

Per la v0.1 serve almeno poter rappresentare:

1. apertura temporanea;
2. ripristino esplicito nello stesso turno;
3. distruzione permanente.

La durata deve essere data-driven/versionata dove appropriato.

---

# 6. RICALCOLO LOS E TARGETING

Quando A apre la cover:

```text
MapState changes
→ cover/edge revision changes
→ LOS is recalculated
→ targeting is revalidated
```

Il tiro di B NON deve usare la LOS calcolata durante il Planning come verità definitiva.

La preview del Planning è una previsione.

Al momento della Resolution, B colpisce soltanto se lo stato reale corrente consente il tiro.

Esempio:

```text
A: Open Cover
→ SUCCESS

B: Vector Shot
→ LOS revalidation
→ VALID
→ resolve attack
```

Caso alternativo:

```text
A: Open Cover
→ INTERRUPTED / FAILED

B: Vector Shot
→ LOS revalidation
→ BLOCKED
→ FIZZLE / blocked result
```

Usare i reason code reali del progetto.

---

# 7. STATO UI: CONFERMATO / PREVISTO / INCERTO

La UI di RefactorTactics distingue:

- **Confermato**
- **Previsto**
- **Incerto**

Questa combo è un caso perfetto per usare il sistema.

Durante il Planning:

```text
A → Open Cover
```

è un intento alleato noto.

Il tiro di B dipende dal successo di A.

Quindi il tiro attraverso una cover ancora chiusa NON deve apparire come già garantito.

Mostrare concettualmente:

```text
A: OPEN        → previsto
B: FIRE        → previsto / dipendente
C: SEAL        → previsto / dipendente
```

Se il progetto usa Action Ghost / ghost timeline:

```text
[ OPEN ] → [ FIRE ] → [ SEAL ]
```

La visualizzazione deve rendere evidente la dipendenza.

Esempio di warning:

```text
Shot requires allied cover opening.
```

Non trasformare questo warning in conoscenza dell'intento nemico.

Usare esclusivamente:

- stato pubblico;
- stato proprio;
- intenti della propria squadra;
- regole deterministiche note.

---

# 8. SIMMETRIA E COUNTERPLAY

La cover aperta deve essere **aperta per tutti**, salvo abilità che dichiarino esplicitamente un comportamento asimmetrico.

Quindi:

```text
prima:

Enemy ███████ Team

dopo Open:

Enemy -------- Team
```

La nuova LOS può essere sfruttata anche dal nemico.

Questa è una proprietà desiderata.

Possibili counterplay:

- nemico prepara un tiro sulla stessa finestra;
- nemico usa Overwatch sulla nuova linea;
- A viene Stun/KO/interrupt prima dell'apertura;
- B viene spostato prima del proprio attacco;
- il bersaglio cambia posizione;
- il nemico distrugge la cover mentre è aperta;
- il nemico impedisce la richiusura;
- il nemico sfrutta il varco per una propria abilità;
- il nemico altera l'arco prima di C;
- un hazard si propaga attraverso il varco.

La combo NON deve essere automaticamente sicura.

---

# 9. VARIANTI SISTEMICHE

Il framework deve permettere varianti senza codice speciale per ogni combo.

## Variante A — stesso personaggio apre e richiude

```text
A opens
B fires
A seals
```

Trade-off:
A consuma risorse / ability economy maggiore.

---

## Variante B — l'attacco chiude automaticamente

```text
A opens
B fires
B ability effect seals
```

Esempio:

`Shoot-and-Seal`

---

## Variante C — più alleati sfruttano la finestra

```text
A opens
B fires
C fires
D seals
```

Serve definire chiaramente l'ordine deterministico degli attacchi.

---

## Variante D — distruzione + ricostruzione

```text
A destroys cover
B fires
C builds new cover
```

La nuova cover può anche avere orientamento differente.

---

## Variante E — rotazione cover

```text
A rotates cover
B exploits new angle
C rotates/restores cover
```

Molto interessante per la griglia esagonale e le cover direzionali.

---

## Variante F — elemento di mappa

Lo stesso pattern deve funzionare con:

- porta;
- saracinesca;
- blast shield;
- barriera energetica;
- ponte mobile;
- portello;
- pannello tattico.

Esempio:

```text
Engineer opens blast shield
Marksman fires
Hacker closes blast shield
```

---

# 10. VARIANTI AMBIENTALI

Questa meccanica deve potersi combinare con il sistema ambientale.

## Ghiaccio

```text
A melts ice wall
        ↓
Water / open line

B fires through opening
        ↓

C freezes water again
        ↓
Ice Cover
```

---

## Hardlight

```text
A overloads hardlight barrier
→ OFF

B fires

C reactivates barrier
→ ON
```

---

## Acqua / fuoco / elettricità

L'apertura di una cover o porta può anche modificare:

- propagazione acqua;
- propagazione fuoco;
- conduzione elettrica;
- rumore;
- visibilità;
- accessibilità del grafo.

Non aggiungere automaticamente questi effetti: devono derivare dai dati dell'elemento.

---

# 11. RELAZIONE CON OVERWATCH E FAST REACTION

La Cover Window deve poter interagire con il sistema di reaction.

Esempio:

```text
A opens cover
        ↓
Enemy Overwatch gains valid LOS
        ↓
Reaction Opportunity
        ↓
FIRE / HOLD
```

La simulazione deve fermarsi soltanto sui Decision Boundary previsti dal sistema di reaction.

Non creare logica speciale:

```text
if CoverWindow:
    ignore reactions
```

La modifica della cover aggiorna semplicemente lo stato corrente.

Le normali reaction valutano poi le proprie condizioni.

Questo permette emergenza sistemica.

---

# 12. DETERMINISMO

Questa meccanica deve rispettare completamente le regole del resolver.

Stesso:

- snapshot;
- accepted intents;
- map state;
- rules version;
- resolver config;
- content manifest;
- seed;

deve produrre:

- stesso ordine di modifica cover;
- stessa LOS;
- stessi attack result;
- stesso stato finale;
- stesso TurnLog;
- stesso StateHash / LogHash se già supportati.

Non dipendere da:

- frame rate;
- durata animazioni;
- montage;
- overlap fisici real-time;
- ordine TMap/TSet;
- timing client.

---

# 13. TURNLOG

Non creare un event system parallelo.

Usare il `TurnLog` canonico.

La combo deve risultare spiegabile.

Esempio concettuale:

```text
Turn=4
Phase=Prep
Event=EnvironmentChanged
Source=A
Edge=E_12_13
Before=Closed
After=Open
Reason=Ability.OpenCover

Turn=4
Phase=Blast
Event=LOSValidated
Source=B
Target=Enemy_02
Result=Valid

Turn=4
Phase=Blast
Event=AbilityResolved
Source=B
Ability=VectorShot
Target=Enemy_02

Turn=4
Phase=Blast
Event=EnvironmentChanged
Source=C
Edge=E_12_13
Before=Open
After=Closed
Reason=Ability.SealCover
```

Se A fallisce:

```text
Event=AbilityInterrupted
Source=A

Event=AbilityFizzled
Source=B
Reason=LOSBlocked

Event=EnvironmentChangeSkipped
Source=C
Reason=AlreadyClosed
```

Usare nomi/event type esistenti nella repository.

---

# 14. DATA-DRIVEN

Non implementare:

```cpp
if (Character == Engineer && Ally == Marksman)
{
    OpenThenShoot();
}
```

La combo deve emergere da ability/effect definition generiche.

Una ability può produrre effetti come:

```text
ChangeEdgeState
ChangeCoverState
RotateCover
CreateCover
DestroyCover
RestoreCover
```

Usare i tipi già esistenti se disponibili.

L'ability definition deve continuare a dichiarare:

- AbilityId;
- Version;
- Tags;
- cost;
- cooldown;
- category/priority;
- targeting;
- requirements;
- effects;
- duration;
- moving-target policy;
- eventuale map/edge policy.

Validator:

- edge target valido;
- stato iniziale compatibile;
- transizione permessa;
- durata valida;
- eventuale ripristino possibile;
- nessun riferimento hard-coded.

---

# 15. NETWORKING E PRIVACY

Il planning della combo è team-only.

I client avversari NON devono ricevere:

- chi aprirà la cover;
- chi sparerà attraverso;
- chi la richiuderà;
- target;
- ordine della combo;
- Action Ghost;
- dipendenze interne del piano.

Durante la Resolution diventano pubblici soltanto gli eventi che il normale modello di percezione rende osservabili.

La Cover Window NON modifica il modello di privacy.

Integrare i test di canary leak se il nuovo intent/effect introduce nuovi DTO.

---

# 16. COMBO SIGNATURE PER LA V0.1

Inserire nella showcase della v0.1 una combo molto leggibile:

# OPEN → FIRE → SEAL

Obiettivo dimostrativo:

mostrare in pochi secondi:

- coordinazione di squadra;
- intenti alleati;
- Action Ghost;
- dipendenza tra azioni;
- cover dinamica;
- ricalcolo LOS;
- ordine deterministico;
- counterplay;
- TurnLog explainable.

Configurazione raccomandata:

```text
Unit A = Controller / Engineer
Unit B = Marksman / Striker
Unit C = Guardian / Engineer
```

Non bloccare i nomi dei personaggi se il roster corrente della repository usa altri character ID.

Scenario visuale:

```text
Enemy dietro cover direzionale
        |
        | blocked LOS
        |
A ---- Cover ---- B

Planning:

A → OPEN
B → FIRE enemy
C → SEAL

Resolution:

A OPEN succeeds
Cover becomes Open
B LOS becomes valid
B fires
C seals
Cover becomes Closed
```

---

# 17. AGGIORNAMENTI DOCUMENTALI RICHIESTI

Claude deve individuare i documenti attivi corretti e consolidare questa decisione.

Come minimo verificare e aggiornare, se esistono equivalenti:

## Gameplay

- sequence / round / phase specification;
- actions / delayed actions;
- cover mechanics;
- environmental interactions;
- Overwatch / reactions;
- action economy;
- character/ability synergy.

## Map

- edge state;
- cover state;
- directional cover;
- door / interactive edge;
- graph revision;
- LOS invalidation;
- targeting revalidation.

## Simulation

- `MapState`;
- event ordering;
- deterministic resolver;
- snapshot;
- `TurnLog`;
- reason codes;
- simultaneous changes.

## UI/UX

- Action Ghost;
- teammate intent dependencies;
- Confirmed / Predicted / Uncertain;
- cover state visualization;
- LOS preview;
- warning for dependent actions.

## Networking

- team-only planning;
- sanitized preview DTO;
- no enemy intent leak.

## Testing

- scripted scenarios;
- golden tests;
- functional tests;
- visual test;
- headless/fast test;
- determinism repeat;
- packaged test where appropriate.

Non creare un nuovo documento se uno esistente è la fonte canonica adatta.

Se invece manca una specifica unificata delle cover dinamiche, valuta di crearla e linkarla dai documenti esistenti.

---

# 18. ROADMAP — RIELABORAZIONE RICHIESTA

Non limitarti ad aggiungere una riga.

Rivedi la roadmap corrente e inserisci le dipendenze reali.

La feature richiede almeno:

```text
Hex map / graph
        ↓
Directional Cover
        ↓
Edge/Cover runtime state
        ↓
Graph revision
        ↓
LOS revalidation
        ↓
Ability map effects
        ↓
Action dependency preview
        ↓
OPEN → FIRE → SEAL scenario
        ↓
Automated regression tests
```

Individua in quale milestone/checkpoint ciascun elemento appartiene.

Se parte dell'infrastruttura è già completata, NON riaprirla: registra la feature come consumer/estensione.

Possibili work item:

1. runtime cover state transitions;
2. reversible cover changes;
3. explicit duration / restore policy;
4. graph/cover revision invalidation;
5. LOS/target revalidation after environment changes;
6. ability effect for cover/edge state;
7. Action Ghost dependency visualization;
8. `TurnLog` environment before/after;
9. cover-window scripted scenario;
10. reaction interaction scenario;
11. deterministic golden test;
12. network privacy regression.

Per ogni work item indicare:

- milestone;
- dipendenze;
- acceptance criteria;
- test;
- Definition of Done;
- packaged verification se richiesta.

Rielabora anche gli exit gate delle milestone coinvolte.

---

# 19. SCENARI DI TEST OBBLIGATORI

Integrare questi scenari nel Test Harness esistente o nel sistema di test corrente.

Usare file testuali versionabili se il progetto ha già introdotto gli scenario JSON.

Non creare una seconda infrastruttura.

---

## SCENARIO 1 — Happy Path

ID suggerito:

```text
CoverWindow.OpenFireSeal.Basic
```

Setup:

```text
A -- cover -- Enemy
B has attack aligned
C can restore cover
```

Turn:

```text
A → OpenCover
B → Fire
C → SealCover
```

Assert:

- cover `Closed → Open`;
- LOS B→Enemy diventa valida;
- attacco di B viene risolto;
- cover `Open → Closed`;
- stato finale cover = Closed;
- TurnLog contiene gli eventi nell'ordine corretto.

---

## SCENARIO 2 — Opening Fails

```text
CoverWindow.OpenFails.AttackBlocked
```

A viene interrotto o l'abilità non è valida.

Assert:

- cover resta Closed;
- B non ottiene LOS;
- attacco fizzles/blocked con reason corretto;
- C non modifica in modo scorretto la cover.

---

## SCENARIO 3 — Shooter Displaced

```text
CoverWindow.ShooterDisplaced
```

A apre correttamente.

Prima del tiro B viene spostato.

Assert:

- LOS viene rivalidata dalla nuova posizione;
- nessuna LOS cache stale;
- esito coerente con la geometria corrente.

---

## SCENARIO 4 — Enemy Exploits Window

```text
CoverWindow.EnemyCounterfire
```

A apre la cover.

Un nemico possiede un attacco/reaction valido sulla linea.

Assert:

- il nemico può sfruttare la finestra;
- l'apertura non è team-only a livello fisico/logico;
- ordine deterministico;
- eventuale reaction segue il normale sistema Opportunity → Commit.

---

## SCENARIO 5 — Cover Destroyed Before Seal

```text
CoverWindow.DestroyedBeforeSeal
```

A apre.

B o un nemico distrugge la cover.

C prova a richiuderla.

Assert:

- stato `Destroyed`;
- `Seal` non ricrea automaticamente una cover distrutta;
- reason code chiaro;
- graph/cover revision corretta.

---

## SCENARIO 6 — Multiple Allied Shooters

```text
CoverWindow.MultipleShots
```

A apre.

B e C sparano.

D richiude.

Assert:

- entrambi gli attacchi vedono lo stesso stato Open se appartenenti alla finestra prevista;
- ordine attacchi stabile;
- nessuna dipendenza da ordine container.

---

## SCENARIO 7 — Door Variant

```text
CoverWindow.Door.OpenFireClose
```

Usare una porta/arco invece di una barriera.

Assert:

- edge state cambia;
- path/LOS vengono invalidati se necessario;
- tiro sfrutta la porta aperta;
- porta chiusa correttamente dopo.

Questo scenario collega Cover Window al sistema generale di interazioni porte.

---

## SCENARIO 8 — Ice Environmental Variant

```text
CoverWindow.Ice.MeltFireFreeze
```

Se le primitive ambientali necessarie sono già disponibili.

Sequenza:

```text
A melts ice cover
B fires
C freezes/rebuilds ice cover
```

Assert:

- cambio superficie/cover;
- LOS corretta;
- stato ambientale finale corretto.

Se non ancora implementabile, inserirlo nella roadmap e creare fixture/spec attesa senza falso PASS.

---

## SCENARIO 9 — Overwatch Trigger

```text
CoverWindow.OpenTriggersOverwatch
```

A apre una linea.

Un Overwatch nemico ottiene un target valido.

Assert:

```text
Environment change
→ LOS valid
→ Reaction Opportunity
→ policy test
→ reaction resolve
```

Nessuna informazione futura deve essere anticipata al client.

---

## SCENARIO 10 — Determinism Repeat

```text
CoverWindow.Determinism
```

Eseguire lo stesso scenario N volte.

Assert:

- stesso StateHash;
- stesso LogHash;
- stesso evento order;
- zero divergenze.

Usare almeno il repeat count già previsto dal Test Harness.

---

## SCENARIO 11 — Network Privacy

```text
CoverWindow.Network.NoIntentLeak
```

Team A prepara:

```text
OPEN → FIRE → SEAL
```

Team B non deve ricevere durante Planning:

- AbilityId privati;
- target;
- cover target;
- sequence;
- label;
- ghost;
- canary IDs.

Assert tramite test canary già previsto nel modello networking.

---

## SCENARIO 12 — Stale Preview / Revalidation

```text
CoverWindow.PreviewVsResolution
```

Durante il Planning la UI mostra il tiro come previsto valido a seguito di Open.

Lo stato cambia prima del Blast.

Assert:

- preview non decide l'esito;
- resolver usa stato corrente;
- UI/TurnLog spiega perché la previsione non si è verificata.

---

# 20. SCENARIO SHOWCASE VISUALE

Creare uno scenario visuale specificamente leggibile da un osservatore umano.

Obiettivo:

in meno di uno o due turni deve essere chiarissimo:

1. la cover inizialmente blocca il tiro;
2. A la apre;
3. compare visivamente il corridoio di LOS;
4. B spara;
5. C la richiude;
6. il TurnLog mostra perché il tiro è stato possibile.

Camera:

- focus sulla cover;
- niente movimenti inutili;
- colori/iconografia coerenti con la UI corrente;
- ghost timeline visibile durante Planning;
- playback abbastanza lento da capire la sequenza.

Non introdurre logica speciale nel resolver solo per il showcase.

---

# 21. ASSERTION MINIME

Usare assertion esistenti o aggiungere solo quelle necessarie.

Possibili assertion:

```text
EdgeStateEquals
CoverStateEquals
LOSValid
LOSBlocked
AbilityResolved
AbilityFizzled
EventExists
EventOrder
UnitHpEquals / UnitHpRange
StateHashEquals
LogHashEquals
```

Ogni failure deve riportare:

- scenario;
- turn;
- phase;
- micro-step/boundary;
- unit;
- edge/cell;
- expected;
- actual;
- reason code.

---

# 22. REPORT MACHINE-READABLE

Se l'Automated Scenario Test Harness è già presente o pianificato, integrare questa feature nel formato corrente.

Claude deve poter diagnosticare un fail da:

```text
Saved/RTTests/
  <ScenarioId>/
    <RunId>/
      result.json
      turnlog.jsonl
      state_initial.json
      state_final.json
```

Non creare un formato parallelo se il progetto ne ha già uno.

Nel report includere, quando utile:

```text
InitialCoverState
OpenEvent
LOSResult
AttackResult
SealResult
FinalCoverState
GraphRevisionBefore
GraphRevisionAfter
StateHash
LogHash
```

---

# 23. TEST DI IMPLEMENTAZIONE / AUTOMATION

Oltre agli scenari funzionali, aggiungere test core dove appropriato.

## Core Test A — Cover state transition

```text
Closed → Open → Closed
```

Assert transizioni valide.

---

## Core Test B — Destroyed cannot Seal

```text
Closed → Open → Destroyed
```

`Seal` deve fallire salvo ability esplicita di rebuild.

---

## Core Test C — LOS invalidation

Modifica cover:

```text
Closed → Open
```

deve invalidare/revisionare correttamente la query LOS.

---

## Core Test D — Container permutation

Inserire edge/effect/unit in ordine differente.

L'esito deve essere identico.

---

## Core Test E — TurnLog before/after

Ogni modifica competitiva deve riportare stato prima/dopo o dati sufficienti all'explainability.

---

# 24. DEBUG

Aggiungere debug coerente con gli strumenti esistenti.

Visualizzazione utile:

```text
EdgeId
CoverState
Direction
GraphRevision
LOS Blocker
LastChangeSource
LastChangeTurn/Phase
```

Possibile overlay:

```text
CLOSED
OPEN
DISABLED
DESTROYED
```

Non affidarsi solo al colore.

---

# 25. ERRORI DA EVITARE

Non:

- implementare la combo per nome di personaggio;
- modificare soltanto una mesh;
- usare SetActorHidden come autorità della cover;
- usare SetActorLocation per simulare il risultato;
- conservare una LOS calcolata in Planning come esito definitivo;
- chiudere automaticamente una cover distrutta;
- rendere il varco utilizzabile soltanto dal team che l'ha creato senza regola esplicita;
- saltare Overwatch/reaction perché la finestra è "temporanea";
- dipendere da animazioni o frame;
- replicare intenti nemici;
- creare un secondo event log per i test;
- introdurre un nuovo sistema di scenario se ne esiste già uno.

---

# 26. ACCEPTANCE CRITERIA DELLA FEATURE

La feature è Done soltanto quando:

1. una cover/arco può essere aperta/disabilitata in modo autorevole;
2. la modifica aggiorna il `MapState`;
3. graph/cover revision viene aggiornata correttamente;
4. LOS/targeting vengono rivalidati;
5. un alleato può sfruttare la nuova linea di tiro;
6. anche un nemico può sfruttarla se le regole lo consentono;
7. la cover può essere ripristinata se non è stata distrutta;
8. il TurnLog spiega tutta la catena;
9. la UI mostra correttamente la dipendenza in Planning;
10. gli intenti restano team-only;
11. gli scenari scripted passano;
12. il determinism repeat produce zero divergence;
13. il packaged/network test pertinente non mostra leak;
14. la documentazione è consolidata;
15. la roadmap contiene implementazione e test;
16. il showcase visuale è riproducibile automaticamente.

---

# 27. OUTPUT RICHIESTO A CLAUDE

Al termine del lavoro produrre un report Markdown con:

## A. Audit

```text
- file letti;
- documenti modificati;
- conflitti trovati;
- decisioni consolidate.
```

## B. Implementazione documentale

Per ogni file:

```text
File
Motivo modifica
Cosa è cambiato
```

## C. Roadmap

Mostrare:

```text
Milestone
Work item
Dependencies
Acceptance criteria
Tests
Status
```

## D. Test

Elenco di tutti gli scenari:

```text
ScenarioId
Purpose
Setup
Expected result
Automation level
```

## E. Codice

Se sono necessarie modifiche di codice, indicare:

```text
File
Class
Responsibility
Why
```

Non implementare codice non richiesto se la feature non è ancora nella milestone corrente: in quel caso aggiornare roadmap, test plan e specifica.

## F. Git

Proporre commit separati e focalizzati, per esempio:

```text
docs(gameplay): define cover window coordination pattern
docs(map): specify reversible cover state transitions
docs(ui): add dependent ally intent preview
docs(roadmap): schedule cover-window implementation and tests
test(scenarios): add open-fire-seal regression scenarios
```

Adattare i messaggi alle convenzioni reali della repository.

---

# 28. PRINCIPIO FINALE

La feature non è:

> "il personaggio A apre una barriera per il personaggio B."

La feature è:

> **Lo stato della mappa può cambiare durante la Resolution; più intenti alleati possono essere coordinati per creare, sfruttare e richiudere una finestra tattica temporanea.**

Da questa primitiva devono emergere:

```text
Open → Fire → Seal
Open → Dash → Seal
Rotate → Fire → Restore
Melt → Fire → Freeze
Open Door → Shoot → Close Door
Disable Shield → Ability → Reactivate Shield
Destroy → Attack → Rebuild
```

Questo deve rafforzare uno dei pilastri di RefactorTactics:

**la mappa è un sistema strategico attivo, non uno sfondo.**
