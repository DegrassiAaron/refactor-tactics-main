# TASK — PROGETTARE E IMPLEMENTARE RT AUTOMATED SCENARIO TEST HARNESS

Stai lavorando sulla repository di RefactorTactics, progetto Unreal Engine 5.

Prima di modificare codice:
1. analizza la repository;
2. leggi AGENTS.md / CLAUDE.md se presenti;
3. leggi la documentazione sotto Docs/, in particolare quella relativa a:
   - architettura UE;
   - simulazione deterministica;
   - TurnLog;
   - roadmap/QA;
   - mappe e pathfinding;
   - planning;
   - Fast Action / Fast Reaction;
4. considera la documentazione della repository come fonte di verità;
5. segnala eventuali conflitti tra questa richiesta e l'implementazione/documentazione esistente.

Non inventare API Unreal.
Usa la versione Unreal bloccata realmente nella repository.
Se una API Unreal varia tra versioni, verifica quella effettivamente utilizzata dal progetto.

---

# OBIETTIVO

Voglio introdurre molto presto nello sviluppo un sistema automatico di test di gameplay chiamato provvisoriamente:

RT Automated Scenario Test Harness

Il sistema deve permettere questo workflow:

Claude Code
    ↓
crea/modifica uno scenario di test testuale
    ↓
Unreal Editor
    ↓
io eventualmente modifico alcuni parametri nel Details Panel
    ↓
premuto Play
    ↓
il test parte automaticamente
    ↓
le unità vengono spawnate/configurate
    ↓
si muovono e scelgono/eseguono azioni senza input umano
    ↓
Planning / Ready / Snapshot / Resolution avvengono automaticamente
    ↓
eventuali Fast Action / Fast Reaction vengono risolte tramite policy di test
    ↓
Unreal genera TurnLog + report machine-readable
    ↓
Claude Code legge il report e i log
    ↓
Claude determina PASS/FAIL e diagnostica la causa del fallimento

Durante un test io NON devo:
- cliccare unità;
- selezionare celle;
- scegliere abilità;
- premere Ready;
- rispondere manualmente alle reaction.

Devo poter premere Play e osservare la simulazione.

---

# PRINCIPIO ARCHITETTURALE FONDAMENTALE

NON creare una seconda simulazione specifica per i test.

Il Test Harness deve usare lo stesso percorso del gameplay reale.

Architettura desiderata:

Human UI ---------+
                  |
Test Scenario ----+----> Intent / Commands
                  |
Bot --------------+
                  |
Replay -----------+
                  |
                  v
              Planning
                  |
                Commit
                  |
               Snapshot
                  |
               Resolver
                  |
              TurnLog
                  |
             Game State

Il Test Harness deve produrre gli stessi tipi di Intent/Command che produrrebbe il gameplay normale.

NON sono accettabili scorciatoie come:

Test -> SetActorLocation()

oppure:

Test -> ApplyDamage()

se il gameplay reale passa attraverso movimento, resolver, ability resolution, environment resolver ecc.

Il test deve esercitare il codice reale.

---

# OBIETTIVI A LUNGO TERMINE

Il sistema dovrà supportare tre modalità.

## 1. VISUAL

Avviabile dall'Unreal Editor.

Premo Play e vedo fisicamente:

- unità muoversi;
- abilità partire;
- dash;
- attacchi;
- reaction;
- collisioni;
- effetti terreno;
- acqua;
- elettricità;
- fuoco;
- cover;
- KO;
- objective;
- turni successivi.

La presentazione può essere rallentata per leggibilità, ma NON deve influenzare la simulazione.

---

## 2. FAST

Stesso scenario e stesso simulatore, ma:

- animazioni ridotte/skippate;
- nessuna attesa reale;
- reaction automatiche immediate;
- risoluzione più veloce possibile.

Serve per regression test locali.

---

## 3. HEADLESS

Stesso scenario eseguibile:

- da command line;
- tramite Unreal Automation;
- eventualmente in CI;
- senza interazione umana;
- possibilmente senza rendering.

Il risultato deve essere identico dal punto di vista logico.

---

# SCENARI TESTUALI

Gli scenari NON devono essere .uasset generati da Claude.

Preferire file testuali versionabili in Git.

Valuta JSON come prima opzione, salvo che il progetto abbia già una soluzione migliore.

Struttura indicativa:

Tests/
  Scenarios/
    Movement/
    Combat/
    Environment/
    Reactions/
    Visibility/
    Objectives/
    Network/

oppure una posizione coerente con la repository esistente.

Esempio concettuale:

{
  "scenarioId": "Environment.WaterElectric.Basic",
  "version": 1,
  "seed": 12345,

  "map": "L_DevSandbox",

  "units": [
    {
      "id": "Unit_Drift",
      "characterId": "Drift",
      "team": 0,
      "cell": [0, 0, 0]
    },
    {
      "id": "Unit_Vex",
      "characterId": "Vex",
      "team": 1,
      "cell": [2, 0, 0]
    }
  ],

  "turns": [
    {
      "intents": [
        {
          "unit": "Unit_Drift",
          "ability": "Floodgate",
          "targetCell": [1, 0, 0]
        },
        {
          "unit": "Unit_Vex",
          "move": [
            [2, 0, 0],
            [1, 0, 0]
          ]
        }
      ]
    }
  ],

  "expect": [
    {
      "type": "SurfaceHasStatus",
      "cell": [1, 0, 0],
      "status": "Wet"
    }
  ]
}

Questo schema è solo concettuale.

NON implementarlo ciecamente.

Prima studia i tipi già presenti nella repository e costruisci un formato aderente al modello reale.

---

# SCRIPTED TEST VS AGENT TEST

Il framework deve essere progettato per supportare due categorie.

## SCRIPTED SCENARIO

Le decisioni sono completamente definite.

Esempio:

Turn 1:
Drift -> Floodgate C4
Vex -> Move C4

Turn 2:
Nyx -> Arc Lance C4

Serve per:

- golden test;
- regression;
- determinismo;
- bug reproduction;
- verifica puntuale di una regola.

Questi test devono avere priorità iniziale.

---

## AGENT SCENARIO

Estensione successiva.

Invece di specificare esattamente l'azione, assegniamo una policy.

Esempio:

Aegis = Defensive
Nyx = Aggressive
Drift = EnvironmentControl
Vex = Objective

Il Test Agent:

1. enumera azioni legalmente disponibili;
2. assegna uno score deterministico;
3. seleziona un'azione;
4. produce un normale Intent;
5. passa attraverso il normale sistema di planning.

NON implementare subito una AI complessa se aumenta lo scope.

Per la prima iterazione basta progettare un'interfaccia che permetta di aggiungerla in seguito.

---

# RT TEST DIRECTOR

Valuta l'introduzione di un Actor o servizio equivalente, ad esempio:

ARTTestDirector

da poter inserire in:

L_DevSandbox

Il nome definitivo deve seguire le convenzioni della repository.

Il Test Director deve essere soltanto un ORCHESTRATORE.

NON deve contenere logica competitiva.

Possibili proprietà Editor:

Scenario
AutoRun
SeedOverride
AutoReady
PlaybackMode
PlaybackSpeed
StopOnFailure
ShowDebug
RepeatCount

e un sistema limitato di override di configurazione.

Esempio concettuale:

Scenario:
Environment.WaterElectric.Basic

AutoRun:
true

Mode:
Visual

Seed:
12345

ShowDebug:
true

Gli override devono essere tracciati nel report.

---

# WORKFLOW EDITOR DESIDERATO

Voglio poter fare:

1. apro L_DevSandbox;
2. seleziono RTTestDirector;
3. scelgo Scenario;
4. eventualmente modifico parametri;
5. premo Play.

A questo punto tutto parte automaticamente.

Non voglio dover premere altri pulsanti.

---

# ESECUZIONE AUTOMATICA

Flusso concettuale:

BeginPlay
    ↓
Detect Test Mode
    ↓
Load Scenario
    ↓
Validate Scenario
    ↓
Reset / Build Initial State
    ↓
Apply Explicit Test Overrides
    ↓
Spawn / Configure Units
    ↓
Start Turn
    ↓
Submit Intents
    ↓
Auto Ready
    ↓
Commit
    ↓
Build Immutable Snapshot
    ↓
Resolve
    ↓
Collect TurnLog
    ↓
Evaluate Assertions
    ↓
Next Turn
    ↓
Final Assertions
    ↓
Write Test Report
    ↓
PASS / FAIL

Riutilizzare i sistemi reali esistenti.

---

# FAST ACTION / FAST REACTION

Il framework deve essere predisposto per le decision window.

Un test deve poter definire policy come:

CommitFirstValid
Hold
Timeout
HoldFirstThenCommit
TargetHighestPriority
TargetSpecificUnit

Esempio Overwatch:

Enemy A entra nell'area
-> HOLD

Enemy B entra nell'area
-> FIRE

La Test Policy deve rispondere alla stessa Reaction Opportunity utilizzata dal gameplay reale.

NON creare codice speciale tipo:

if (IsTest)
    SkipOverwatchLogic();

Il test deve esercitare:

Trigger
-> Reaction Opportunity
-> Decision
-> Validation
-> Commit/Hold
-> Resolver
-> TurnLog

---

# TEMPO REALE

In Visual Mode può essere utile lasciare brevi pause per capire cosa sta succedendo.

In Fast/Headless:

NON aspettare:

- planning timer;
- countdown Ready;
- animazioni;
- 3 secondi di Fast Reaction;
- montage;
- VFX.

La simulazione logica deve avanzare immediatamente tra i decision boundary.

Timer real-time e animazioni non devono decidere il risultato.

---

# ASSERTION SYSTEM

Non limitarti a verificare "il gioco non è crashato".

Serve un sistema di assertion di dominio.

Esempi futuri:

UnitAtCell
UnitHpEquals
UnitHpRange
UnitHasStatus
UnitNotHasStatus
UnitKO
SurfaceHasStatus
EdgeEnabled
EdgeDisabled
CoverExists
CoverDestroyed
AbilityResolved
AbilityFizzled
EventExists
EventNotExists
EventCount
ObjectiveState
TurnCompleted
StateHashEquals
LogHashEquals

Ogni assertion deve produrre:

- Expected;
- Actual;
- Turn;
- Phase;
- MicroStep;
- Unit/Cell/Event coinvolto;
- reason code quando disponibile.

Prima iterazione: implementa solo quelle necessarie per i primi test.

Non creare un mega-framework prematuramente.

---

# OUTPUT MACHINE-READABLE

Questo è CRITICO.

Claude Code deve poter leggere il risultato senza interpretare migliaia di righe di log Unreal.

Ogni esecuzione deve generare una directory tipo:

Saved/RTTests/
  <ScenarioId>/
    <RunId>/
      result.json
      turnlog.jsonl
      state_initial.json
      state_final.json

Se alcuni file non sono ancora implementabili, partire da:

result.json
turnlog.jsonl

Il formato deve essere stabile e versionato.

---

# RESULT.JSON

Esempio concettuale:

{
  "schemaVersion": 1,

  "scenario": "Environment.WaterElectric.Basic",

  "runId": "...",

  "result": "FAIL",

  "engineVersion": "...",
  "projectVersion": "...",
  "rulesVersion": "...",

  "seed": 12345,

  "assertions": {
    "passed": 11,
    "failed": 1
  },

  "failures": [
    {
      "assertion": "UnitDamage",
      "unit": "Vex",
      "expected": 20,
      "actual": 0,

      "turn": 2,
      "phase": "Blast",
      "microStep": 3,

      "reason": "RequiredSurfaceMissing"
    }
  ],

  "stateHash": "...",
  "logHash": "..."
}

Usa nomenclatura reale della repository.

---

# TURNLOG

Il TurnLog deve restare il log canonico della simulazione.

NON creare un secondo event system parallelo solo per i test.

I report di test devono analizzare il TurnLog reale.

Esempio concettuale:

Turn=2
Phase=Blast
Event=AbilityDeclared
Source=Nyx
Ability=ArcLance

Turn=2
Phase=Blast
Event=EnvironmentPropagationCheck
Cell=[1,0,0]
Expected=Wet
Actual=None

Turn=2
Phase=Blast
Event=AbilityFizzled
Reason=RequiredSurfaceMissing

La struttura deve permettere a Claude di risalire alla causa.

---

# DETERMINISMO

Uno scenario deve dichiarare o ricevere un seed esplicito.

Stesso:

- scenario;
- initial state;
- intents;
- rules version;
- resolver configuration;
- content manifest;
- seed;

deve produrre:

- stesso stato finale;
- stesso TurnLog canonico;
- stesso StateHash;
- stesso LogHash.

Aggiungere in futuro Repeat Test:

Run scenario 100 / 1000 volte
-> assert zero divergence.

NON usare:

FMath::Rand()

o timing di frame/animazioni per risultati competitivi.

---

# CLAUDE ANALYSIS WORKFLOW

Progettare l'output in modo che dopo un'esecuzione io possa dire a Claude Code:

"analizza ultimo test"

e Claude possa:

1. trovare l'ultima run;
2. aprire result.json;
3. individuare assertion fallite;
4. leggere il sottoinsieme rilevante del TurnLog;
5. confrontare initial/final state se necessario;
6. cercare nel codice le classi coinvolte;
7. proporre root cause;
8. indicare file/linee probabilmente coinvolte;
9. suggerire una fix;
10. indicare quali regression test rieseguire.

NON serve implementare un'integrazione Claude dentro Unreal.

Claude Code legge semplicemente file prodotti dal gioco.

---

# LOGGING

Usare categorie di log RefactorTactics esistenti quando disponibili.

Il log umano deve essere utile, ma la fonte principale per Claude deve essere il report strutturato.

Ogni evento importante dovrebbe avere:

- EventType;
- Stable IDs;
- Turn;
- Phase;
- MicroStep;
- ReasonCode;
- before/after essenziale quando utile.

Evitare messaggi testuali non strutturati come unica fonte.

---

# PRIMA ITERAZIONE — SCOPE RIGOROSO

NON implementare subito:

- AI avanzata;
- monte carlo;
- migliaia di assertion;
- editor visuale degli scenari;
- distributed testing;
- cloud testing;
- reinforcement learning;
- matchmaking;
- networking completo se F1 non è ancora iniziata;
- sistema generico gigantesco.

Prima iterazione minima:

1. uno scenario testuale;
2. un Test Director;
3. AutoRun da Play;
4. due unità;
5. movimento pianificato automaticamente;
6. Auto Ready;
7. normale Snapshot;
8. normale Movement Resolver;
9. normale TurnLog;
10. una o due assertion;
11. result.json;
12. PASS/FAIL;
13. scenario eseguibile anche tramite Automation Test se possibile senza duplicare logica.

Primo scenario consigliato:

Movement.Basic

Unit A:
Cell 0,0,0

Intent:
Move -> cella adiacente valida

Expected:
- MoveStarted;
- MoveStep;
- UnitAtCell expected;
- TurnCompleted;
- deterministic hashes disponibili se già implementati.

Secondo scenario:

Movement.Blocked

Terzo:

Movement.Collision

Solo dopo passare a abilità/ambiente.

---

# INTEGRAZIONE CON UNREAL AUTOMATION FRAMEWORK

Il RT Scenario Harness NON sostituisce Unreal Automation Framework.

Usare entrambi.

Automation Framework:

- FRTCellId;
- hash;
- A*;
- serializzazione;
- resolver puro;
- validator.

RT Scenario Harness:

- Planning -> Snapshot -> Resolution;
- gameplay integrato;
- scenari visuali;
- più turni;
- abilità;
- ambiente;
- reaction;
- objective.

Functional Test:

- integrazione con mappe/Actor/presentazione quando appropriato.

Packaged Test:

- milestone/release;
- networking/privacy quando entreranno nello scope.

Idealmente un Automation Test deve poter richiamare lo stesso Scenario Runner senza duplicare lo scenario.

---

# ARCHITETTURA DESIDERATA

Valuta una separazione simile a:

FRTTestScenario
    dati scenario

FRTTestScenarioLoader
    parsing + validazione

FRTScenarioRunner
    state machine del test

ARTTestDirector
    bridge Editor / Visual Mode

FRTTestAssertion
    assertion data

FRTTestResult
    risultato

FRTTestReportWriter
    output JSON

IRTTacticalAgent / IRTTestDecisionPolicy
    futura estensione per decisioni automatiche

I nomi sono proposte.

Riusa tipi/classi già presenti se svolgono queste responsabilità.

Evita UObject quando una struct/service C++ semplice è sufficiente.

---

# REQUIREMENTS EDITOR

Le proprietà principali devono essere modificabili dal Details Panel quando sensato.

Devo poter cambiare velocemente:

- scenario;
- seed;
- mode;
- playback speed;
- repeat count;
- debug;
- eventuali override consentiti.

Gli override di gameplay devono essere espliciti e registrati nel result.json.

Non alterare silenziosamente Data Assets globali.

---

# TESTABILITY FIRST

Da questo momento l'architettura dovrebbe seguire questo requisito:

OGNI nuova meccanica competitiva deve poter essere esercitata senza input umano tramite uno scenario automatico.

Questo vale in futuro per:

- movimento;
- collisioni;
- dash;
- attack;
- AoE;
- Overwatch;
- Fast Reaction;
- Fast Action;
- acqua;
- elettricità;
- fuoco;
- ghiaccio;
- cover;
- LOS;
- visibility;
- noise;
- objectives;
- KO;
- cooldown;
- network privacy.

La UI NON deve essere necessaria per far funzionare la simulazione.

---

# DEFINITION OF DONE DELLA PRIMA ITERAZIONE

Considera il task completato soltanto se posso fare:

1. apro L_DevSandbox;
2. seleziono RTTestDirector;
3. scelgo Movement.Basic;
4. imposto AutoRun=true;
5. premo Play;
6. non tocco mouse/tastiera;
7. le unità vengono configurate automaticamente;
8. parte il planning;
9. vengono inviati gli intent;
10. Ready viene eseguito automaticamente;
11. viene creato lo snapshot;
12. il resolver muove l'unità;
13. la rappresentazione visiva riproduce il movimento;
14. il TurnLog registra gli eventi;
15. viene valutata l'assertion;
16. viene creato result.json;
17. il test termina PASS;
18. posso aprire result.json e capire chiaramente cosa è successo.

Deve inoltre esistere almeno un caso FAIL intenzionale per verificare che il report diagnostico funzioni.

---

# SICUREZZA ARCHITETTURALE

Il Test Harness non deve contaminare codice shipping.

Valuta:

#if WITH_DEV_AUTOMATION_TESTS

oppure moduli/configurazioni appropriate alla struttura del progetto.

Ma non mettere macro ovunque.

Separare chiaramente:

Runtime simulation
vs
Development/Test orchestration.

Il resolver non deve conoscere ARTTestDirector.

---

# FILE DA PRODURRE

Prima di implementare, proponi esattamente:

- file nuovi;
- file modificati;
- dipendenze Build.cs;
- eventuali Config;
- posizione degli scenario;
- posizione output Saved/.

Poi procedi per piccoli commit logici.

---

# TEST AUTOMATICI DEL TEST FRAMEWORK

Il framework stesso deve essere testato.

Minimo:

Scenario loader:
valid file -> success

Scenario loader:
invalid Stable ID -> failure chiara

Runner:
Movement.Basic -> PASS

Runner:
Movement intentionally wrong expectation -> FAIL

Repeat:
stesso scenario + seed -> stesso result/hash

Assicurarsi che un FAIL del gameplay non venga confuso con un errore infrastrutturale.

Distinguere:

PASS
FAIL
ERROR

FAIL:
simulazione completata, assertion non soddisfatta.

ERROR:
scenario invalido, crash logico, risorsa mancante, impossibile avviare il test.

---

# DEBUG

Aggiungere strumenti sufficienti per capire:

CurrentScenario
Turn
Phase
MicroStep
CurrentIntent
CurrentAssertion
RunnerState

Possibile console/debug command:

rt.Test.Status
rt.Test.Run <ScenarioId>
rt.Test.DumpResult

Solo se coerenti con il sistema console esistente.

---

# IMPLEMENTATION STRATEGY

Lavora incrementalmente.

STEP 1:
analisi repository.

STEP 2:
proposta architettura minima.

STEP 3:
implementare solo Movement.Basic.

STEP 4:
farlo funzionare in Visual Mode.

STEP 5:
result.json.

STEP 6:
Automation wrapper.

STEP 7:
Movement.Blocked / Collision.

NON procedere ad abilità, environment o AI finché Movement.Basic non è stabile.

---

# OUTPUT CHE VOGLIO DA TE ORA

Prima di scrivere codice dammi:

1. stato attuale della repository relativo a testing, planning, snapshot, resolver e TurnLog;
2. cosa esiste già e cosa manca;
3. eventuali conflitti con la documentazione;
4. architettura minima proposta;
5. diagramma del flusso;
6. elenco esatto dei file da creare/modificare;
7. formato iniziale dello scenario;
8. formato iniziale di result.json;
9. come ARTTestDirector si integra con L_DevSandbox;
10. come eseguire il primo test da Unreal Editor;
11. come eseguirlo da command line;
12. rischi tecnici;
13. acceptance criteria;
14. sequenza di commit proposta.

DOPO questa analisi, se non emergono blocchi reali, implementa la prima iterazione.

Non chiedermi conferma per dettagli minori:
scegli default ragionevoli e documentali.

Mantieni scope rigoroso.

---

# COMMIT SUGGERITI

Usa commit piccoli e focalizzati, ad esempio:

feat(test): add scenario test data model and loader

feat(test): add automated scenario runner

feat(test): add visual test director for dev sandbox

feat(test): add movement basic scenario

feat(test): write structured scenario result reports

test(test): add scenario runner automation coverage

Adatta i nomi alla repository reale.

---

# RISULTATO FINALE DESIDERATO

La base di RefactorTactics deve diventare testabile così:

modifico una regola
        ↓
premio Play
        ↓
i personaggi giocano automaticamente lo scenario
        ↓
Unreal produce log e report
        ↓
Claude Code analizza FAIL/PASS
        ↓
posso correggere rapidamente la regressione

Il sistema deve essere sufficientemente generale da crescere insieme al gioco, ma la prima implementazione deve essere piccola, compilabile e immediatamente utile.