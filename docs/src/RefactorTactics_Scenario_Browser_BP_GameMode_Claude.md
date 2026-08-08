# RefactorTactics — Scenario Browser per categoria in BP_GameMode
## Handoff operativo per Claude Code

> Obiettivo: riorganizzare il selettore degli scenari usato da `BP_GameMode` nell'Editor Unreal in modo che gli scenari siano navigabili per **categoria primaria**, senza duplicare gli scenari e senza accoppiare la classificazione Editor alla simulazione runtime.

---

# 1. Contesto

RefactorTactics usa scenari per:

- debug tecnico;
- validazione delle azioni;
- test dei personaggi;
- test delle fazioni;
- combo di squadra;
- terreno e interazioni ambientali;
- reaction;
- test automatici;
- regressioni;
- showcase della v0.1/v0.2;
- in futuro network/privacy, bot e performance.

Il numero di scenari crescerà rapidamente.

Una lista piatta nel `BP_GameMode` non è più sufficiente.

Vogliamo che, nel Details Panel dell'Editor, la selezione di uno scenario sia organizzata in modo leggibile.

Principio:

```text
Categoria primaria
        +
Metadati / tag secondari
        +
Stable ScenarioId
        =
Scenario unico, ricercabile da più punti di vista
```

Uno scenario **NON deve essere duplicato** solo perché appartiene semanticamente a più domini.

Esempio:

```text
SCN_V01_Conflux_WaterElectric_Coordination

PrimaryCategory:
    TeamCombos

Characters:
    Flux
    Riva

Factions:
    Conflux

Features:
    Water
    Electricity
    EnvironmentalPropagation

Purpose:
    Showcase
    AutomatedTest

Milestone:
    v0.1
```

Lo stesso scenario deve poter essere trovato:

- dalla categoria `TeamCombos`;
- dalla pagina/wiki di Flux;
- dalla pagina/wiki di Riva;
- dalla pagina/wiki Conflux;
- cercando feature Water/Electricity;
- filtrando per milestone v0.1;
- nella suite automatica.

---

# 2. Regole prima di modificare il codice

Prima di implementare:

1. analizza l'intera repository;
2. leggi `AGENTS.md`, `CLAUDE.md` e documentazione equivalente;
3. individua:
   - classe C++ effettiva del GameMode;
   - `BP_GameMode` o Blueprint equivalente;
   - Test Director / Scenario Harness esistente;
   - definizione attuale degli scenari;
   - cataloghi/registry/Data Assets già presenti;
   - eventuale caricamento JSON degli scenari;
   - eventuali enum già usati per Scenario/Mode/Type;
4. usa la **versione Unreal realmente bloccata nella repository**;
5. non assumere API Unreal senza verificarle sulla patch usata dal progetto;
6. non creare una seconda architettura se il repository possiede già un sistema di scenario;
7. preserva gli Stable ID esistenti;
8. segnala prima eventuali conflitti strutturali importanti.

Baseline documentale storica: UE 5.8, ma **fa fede la versione effettivamente bloccata nella repository**.

---

# 3. Obiettivo UX nell'Editor

Nel `BP_GameMode` voglio una sezione simile a questa:

```text
▼ SCENARIO

    Enable Scenario             [✓]

    Category                    [Team Combos ▼]
    Scenario                    [SCN_V01_Conflux_WaterElectric ▼]

    ------------------------------------------------

    Execution Mode              [Visual ▼]
    Auto Run                    [✓]
    Auto Ready                  [✓]

    Seed Override               [0]
    Repeat Count                [1]

    Show Scenario Debug         [✓]
    Stop On Failure             [✓]
```

L'ordine importante è:

```text
Category
   ↓
Scenario filtrato per Category
```

Non voglio una singola combo contenente centinaia di scenari non classificati.

---

# 4. Categorie approvate

Creare o adattare un enum equivalente a:

```cpp
UENUM(BlueprintType)
enum class ERTScenarioCategory : uint8
{
    Debug,
    CoreSystems,
    Actions,
    Reactions,
    Characters,
    Factions,
    TeamCombos,
    Environment,
    MapInteractions,
    Perception,
    Objectives,
    Bots,
    Networking,
    UI,
    Regression,
    Performance,
    Showcase
};
```

Usare `DisplayName` leggibili nell'Editor.

Nomi visualizzati desiderati:

```text
Debug
Core Systems
Actions
Reactions
Characters
Factions
Team Combos
Environment
Map Interactions
Perception
Objectives
Bots / AI
Networking & Privacy
UI / Presentation
Regression
Performance / Stress
Showcase / Demo
```

Se esiste già un naming convention differente nella repository, mantenerla.

---

# 5. Significato delle categorie

## Debug

Micro-scenari tecnici.

Esempi:

- visualizzazione coordinate hex;
- spawn;
- facing debug;
- lookup cella;
- graph revision;
- LOS overlay.

---

## Core Systems

Regole fondamentali condivise dal gioco.

Esempi:

- Planning;
- Ready;
- Commit;
- Snapshot;
- resolver;
- collisioni di movimento;
- ordine `Prep -> Dash -> Blast -> Move`;
- Cleanup.

Un test di una regola fondamentale NON deve finire genericamente in Debug.

---

## Actions

Una singola azione o famiglia di azioni.

Esempi:

- Move;
- Dash;
- Basic Attack;
- Interact;
- Brace;
- Overwatch setup, se trattato come azione preparata.

---

## Reactions

Sistema di reaction e Decision Boundary.

Esempi:

- Overwatch;
- Counter;
- Guard;
- Fast Reaction;
- HOLD;
- HOLD -> successiva opportunity -> COMMIT;
- trigger simultanei.

---

## Characters

Scenario focalizzato principalmente su un singolo personaggio.

Esempi:

```text
Flux.BasicKit
Riva.WaterControl
Bastion.CoverManipulation
Vektor.Interception
Steel.BasicKit
Aurora.BasicKit
```

---

## Factions

Scenario che mostra identità, filosofia o cooperazione interna a una fazione.

Esempi:

```text
Conflux.Coordination.Basic
Constrine.Coordination.Basic
```

Deve supportare il futuro collegamento dalla wiki della fazione allo scenario lanciabile.

---

## Team Combos

Scenario focalizzato sulla cooperazione concreta fra due o più personaggi.

Esempio:

```text
Riva crea acqua
    ↓
Flux elettrifica
    ↓
propagazione deterministica
```

Può includere personaggi della stessa fazione o di fazioni differenti.

---

## Environment

Terreni, superfici, hazard e propagazioni.

Esempi:

- Water;
- Electric;
- Fire;
- Ice;
- Steam;
- Metal;
- Smoke;
- Acoustic Mask;
- ambient noise.

---

## Map Interactions

Elementi strutturali/interattivi della mappa.

Esempi:

- porte;
- pulsanti;
- muri;
- cover;
- cover dinamica;
- ponti;
- tunnel;
- ascensori;
- valvole;
- relay;
- generatori.

---

## Perception

Informazione incompleta e conoscenza della squadra.

Esempi:

- LOS;
- Facing;
- Fog of War;
- Stealth;
- rumore;
- detection;
- last known position;
- awareness;
- confirmed / predicted / uncertain.

---

## Objectives

Regole di obiettivo e scoring.

Esempi:

- Capture;
- Relay;
- Control Zone;
- Escort;
- Dynamic Objective.

---

## Bots

Scenari dedicati alle decisioni automatiche.

Esempi:

- scoring;
- aggressive policy;
- defensive policy;
- objective policy;
- deterministic choice;
- agent scenario.

---

## Networking

Validazione server/client e privacy.

Esempi:

- team-only intent;
- ready replication;
- reconnect;
- stale sequence;
- preview 8-12 Hz;
- enemy planning canary leak;
- dedicated server scenario.

---

## UI

Leggibilità e presentazione.

Esempi:

- Action Ghost;
- AoE ghost;
- path ghost;
- warning;
- TurnLog;
- certainty visualization;
- Decision Window;
- faction/character scenario launcher.

---

## Regression

Bug già risolti che devono avere un test permanente.

Naming consigliato:

```text
REG_<Area>_<BugOrRule>_<NNN>
```

Esempio:

```text
REG_MoveCollision_SameDestination_001
```

---

## Performance

Scenari di carico.

Esempi:

- molte unità;
- molte path query;
- molte reaction;
- molte celle;
- stress TurnLog;
- resolver budget.

---

## Showcase

Scenari creati principalmente per mostrare una feature o una milestone.

Esempi:

```text
SHOW_V01_CoreGameplay
SHOW_V01_FluxRiva_WaterElectric
SHOW_V01_DynamicCover
SHOW_V02_Conflux
SHOW_V02_Constrine
```

Uno Showcase può anche essere un test automatico.

`Showcase` descrive la categoria primaria di navigazione, non esclude `AutomatedTest`.

---

# 6. Categoria primaria != classificazione completa

Ogni scenario deve avere **una e una sola `PrimaryCategory`**.

Deve poi poter possedere altri metadata.

Schema concettuale:

```cpp
struct FRTScenarioMetadata
{
    ScenarioId;
    DisplayName;
    Description;

    PrimaryCategory;

    CharacterIds;
    FactionIds;

    FeatureTags;
    PurposeTags;

    MilestoneId;

    bAutomatedTest;
    ExpectedTurnCount;
};
```

NON implementare questa struct alla lettera se esistono già tipi equivalenti.

Riutilizzare:

- Stable IDs del progetto;
- Gameplay Tags governati;
- Character IDs esistenti;
- Faction IDs esistenti;
- versioning scenario esistente.

---

# 7. PrimaryCategory: regole

`PrimaryCategory` serve principalmente alla navigazione e organizzazione.

NON deve cambiare:

- regole di simulazione;
- snapshot;
- resolver;
- seed;
- damage;
- pathfinding;
- reaction;
- networking;
- risultati del test.

Cambiare:

```text
PrimaryCategory = TeamCombos
```

in:

```text
PrimaryCategory = Showcase
```

non deve cambiare il risultato logico dello scenario.

Lo Stable ScenarioId resta la vera identità runtime.

---

# 8. Metadata consigliati

Usare i tipi reali già presenti nel progetto.

## Identità

```text
ScenarioId
ScenarioVersion
DisplayName
Description
```

---

## Classificazione

```text
PrimaryCategory
CharacterIds[]
FactionIds[]
FeatureTags
PurposeTags
MilestoneId
```

---

## Test

```text
bAutomatedTest
ExpectedTurnCount
DefaultSeed
RecommendedExecutionMode
```

---

## Eventuali riferimenti

Se utile e già coerente con l'architettura:

```text
RelatedScenarioIds[]
DocumentationRefs[]
```

Non introdurre riferimenti hard-coded a pagine web.

---

# 9. Milestone/versione NON è una categoria

Non creare categorie:

```text
V0_1
V0_2
V0_3
```

La milestone deve essere metadata.

Esempio:

```text
PrimaryCategory:
    Showcase

MilestoneId:
    v0.1
```

Questo permette in futuro:

```text
Showcase
    ↓
v0.1
    ↓
Flux + Riva
```

senza sporcare l'enum delle categorie.

---

# 10. Execution Mode è separato

Non confondere la classificazione con il modo in cui lo scenario viene eseguito.

Usare l'enum esistente oppure equivalente a:

```cpp
UENUM(BlueprintType)
enum class ERTScenarioExecutionMode : uint8
{
    Visual,
    Fast,
    Headless
};
```

Semantica:

```text
Category:
    che scenario è?

ExecutionMode:
    come viene eseguito?
```

Esempio:

```text
Category = Reactions
Scenario = Overwatch.HoldThenCommit
ExecutionMode = Visual
```

oppure:

```text
Category = Reactions
Scenario = Overwatch.HoldThenCommit
ExecutionMode = Headless
```

Lo scenario logico deve essere lo stesso.

---

# 11. Editor selector

La soluzione preferita è:

```text
ScenarioCategory
       ↓
ScenarioId filtrato
```

Il filtro deve mostrare solo gli scenari la cui:

```text
PrimaryCategory == ScenarioCategory
```

## Priorità implementativa

Cercare la soluzione più semplice che funzioni nella patch Unreal del progetto.

Possibile ordine:

### Opzione A — built-in property metadata

Valutare se la patch UE bloccata supporta in modo affidabile una property `FName`/string con:

```text
GetOptions
```

o meccanismo equivalente per generare dinamicamente l'elenco filtrato.

Verificare l'API effettiva prima di usarla.

### Opzione B — selector UObject/struct già esistente

Se il progetto possiede già un selector/editor helper, estenderlo.

### Opzione C — Details Customization

Creare una `IDetailCustomization` soltanto se necessaria.

NON introdurre un nuovo Editor Module/plugin solo per eleganza se la soluzione A/B è sufficiente.

---

# 12. Comportamento quando cambia Category

Caso:

```text
Category = Characters
Scenario = Flux.BasicKit
```

L'utente cambia:

```text
Category = Environment
```

`Flux.BasicKit` non deve rimanere silenziosamente selezionato se non appartiene a Environment.

Comportamento preferito:

```text
Category changed
    ↓
Current Scenario no longer matches
    ↓
clear Scenario selection
```

oppure equivalente comportamento esplicito e facilmente comprensibile.

Non fare auto-selezione arbitraria del primo scenario.

---

# 13. Scenario Info read-only

Dopo la selezione vorrei una sezione informativa.

Esempio:

```text
▼ SCENARIO INFO

    Scenario ID
        TeamCombo.Conflux.WaterElectric.Basic

    Version
        1

    Characters
        Flux
        Riva

    Factions
        Conflux

    Features
        Water
        Electricity
        AoE
        EnvironmentalPropagation

    Milestone
        v0.1

    Automated Test
        Yes

    Expected Duration
        3 Turns
```

Implementarla solo con meccanismi Editor semplici.

Non creare un framework UI complesso.

Se un read-only panel dinamico richiede troppo scope, implementare almeno:

- ScenarioId;
- DisplayName;
- Description;
- Category;
- Milestone;
- Characters/Factions;
- AutomatedTest.

---

# 14. Separazione BP_GameMode / Scenario Definition

`BP_GameMode` NON deve diventare il database degli scenari.

Il GameMode deve contenere soltanto configurazione del run:

```text
Enable Scenario
Selected Category
Selected ScenarioId
Execution Mode
Auto Run
Auto Ready
Seed Override
Repeat Count
Show Debug
Stop On Failure
```

I dati dello scenario devono vivere nel sistema già previsto dal progetto:

- Scenario Definition;
- catalogo;
- registry;
- Data Asset;
- JSON scenario;
- o equivalente già esistente.

Non duplicare unità, turni, intent, assertions ecc. dentro `BP_GameMode`.

---

# 15. Stable ID

Il runtime deve risolvere lo scenario da Stable ID.

Esempio:

```text
TeamCombo.Conflux.WaterElectric.Basic
```

La categoria non deve essere necessaria per il caricamento runtime.

Desiderato:

```text
ScenarioId
    ↓
Scenario Registry
    ↓
Scenario Definition
```

NON:

```text
Category + array index
    ↓
Scenario
```

La seconda soluzione rende fragili rename, ordinamenti e test.

---

# 16. Catalogo / Registry

Se non esiste già, introdurre la minima struttura necessaria per:

```text
GetScenarioById()
GetScenariosByCategory()
GetScenariosForCharacter()
GetScenariosForFaction()
GetScenariosByMilestone()
GetScenariosWithFeature()
```

NON serve necessariamente implementare tutte le query ora.

Per questa task sono obbligatorie almeno:

```text
Resolve ScenarioId
List by PrimaryCategory
```

Ma il data model deve consentire le query future senza duplicare scenario.

---

# 17. Personaggi e fazioni

Il sistema deve supportare il progetto attuale.

Roster noto per milestone:

```text
v0.1
- Flux
- Riva
- Bastion
- Vektor

v0.2
- Steel
- Aurora
- Murdock
- Kwang
```

Fazioni già in discussione:

```text
Conflux
Constrine
```

NON hard-codare questi nomi nel framework.

Devono essere ID/dati.

Servono soltanto per classificare gli scenari esistenti e per creare alcuni esempi.

---

# 18. Wiki integration

Obiettivo futuro:

nella wiki di un personaggio o fazione deve essere possibile mostrare:

```text
Scenari correlati
```

Esempio pagina Flux:

```text
Scenarios:
- Flux.BasicKit
- TeamCombo.Conflux.WaterElectric.Basic
- SHOW_V01_FluxRiva_WaterElectric
```

Questo deve poter essere derivato dai metadata:

```text
CharacterIds contains Flux
```

Analogamente:

```text
FactionIds contains Conflux
```

NON creare adesso un secondo elenco manuale dentro ogni pagina wiki se può essere evitato.

Per questa task basta predisporre metadata/query.

---

# 19. Gameplay Tags

Il progetto usa Gameplay Tags governati.

Usarli dove hanno senso per feature/purpose.

Esempi concettuali:

```text
Scenario.Feature.Environment.Water
Scenario.Feature.Environment.Electric
Scenario.Feature.Reaction.Overwatch
Scenario.Feature.Map.Cover
Scenario.Feature.Perception.Noise

Scenario.Purpose.AutomatedTest
Scenario.Purpose.Showcase
Scenario.Purpose.Tutorial
```

Prima controllare la tassonomia già presente.

Non creare tag duplicati se esistono già root equivalenti.

Per Character/Faction preferire gli Stable ID già esistenti invece di duplicare necessariamente ogni ID come Gameplay Tag.

---

# 20. Scenari da riclassificare

Dopo aver implementato il framework:

1. inventaria tutti gli scenari attuali;
2. assegna a ognuno:
   - `PrimaryCategory`;
   - metadata disponibili;
3. preserva lo ScenarioId;
4. non rinominare scenari senza motivo;
5. produci una tabella nel report finale.

Formato:

| ScenarioId | PrimaryCategory | Characters | Factions | Milestone | Automated |
|---|---|---|---|---|---|

Se uno scenario è ambiguo, scegli la categoria in base al **motivo principale per cui lo apriremmo dall'Editor**.

---

# 21. Esempi di classificazione

## Water + Electricity

```text
ScenarioId:
    TeamCombo.Conflux.WaterElectric.Basic

PrimaryCategory:
    TeamCombos

CharacterIds:
    Flux
    Riva

FactionIds:
    Conflux

FeatureTags:
    Water
    Electricity
    EnvironmentalPropagation

Milestone:
    v0.1

AutomatedTest:
    true
```

---

## Flux kit

```text
ScenarioId:
    Character.Flux.BasicKit

PrimaryCategory:
    Characters

CharacterIds:
    Flux

Milestone:
    v0.1
```

---

## Conflux cooperation

```text
ScenarioId:
    Faction.Conflux.Coordination.Basic

PrimaryCategory:
    Factions

FactionIds:
    Conflux

CharacterIds:
    <personaggi coinvolti>
```

---

## Overwatch HOLD -> FIRE

```text
ScenarioId:
    Reaction.Overwatch.HoldThenCommit

PrimaryCategory:
    Reactions

FeatureTags:
    Overwatch
    FastReaction
    DecisionBoundary

AutomatedTest:
    true
```

---

## Dynamic cover

```text
ScenarioId:
    Map.DynamicCover.OpenFireClose

PrimaryCategory:
    MapInteractions

FeatureTags:
    Cover
    DynamicCover
    TeamCoordination
```

---

## v0.1 complete demo

```text
ScenarioId:
    Showcase.V01.FullMatch

PrimaryCategory:
    Showcase

Milestone:
    v0.1

AutomatedTest:
    true
```

---

# 22. Scenario automated test harness

La nuova classificazione deve integrarsi con il Test Harness.

Il flusso resta:

```text
BP_GameMode / Test Director
        ↓
ScenarioId
        ↓
Scenario Registry
        ↓
Scenario Definition
        ↓
Intent / Commands
        ↓
Planning
        ↓
Ready / Commit
        ↓
Snapshot
        ↓
Resolver
        ↓
TurnLog
        ↓
Assertions
        ↓
result.json
```

NON creare percorsi speciali per categoria.

---

# 23. result.json

Se esiste già il report test, aggiungere metadata utili senza rompere lo schema.

Esempio:

```json
{
  "scenarioId": "TeamCombo.Conflux.WaterElectric.Basic",
  "scenarioVersion": 1,
  "primaryCategory": "TeamCombos",
  "milestone": "v0.1",
  "characters": ["Flux", "Riva"],
  "factions": ["Conflux"],
  "executionMode": "Headless",
  "result": "PASS"
}
```

La categoria serve per diagnostica/reporting.

NON deve influenzare `StateHash`/`LogHash` competitivo se non faceva già parte dello stato logico.

---

# 24. Backward compatibility

Non rompere scenari e test esistenti.

Se oggi `BP_GameMode` contiene una proprietà tipo:

```text
Scenario
```

migrare con prudenza.

Possibili strategie:

- mantenere temporaneamente deprecated property;
- migrare il valore;
- redirect;
- PostLoad;
- script/editor migration.

Scegliere la più semplice in base allo stato reale del repository.

Obiettivo:

```text
aprire una mappa esistente
    ↓
nessuna perdita silenziosa dello scenario configurato
```

Documentare eventuali Blueprint/Data Asset che richiedono resave.

---

# 25. Validazione

Aggiungere validation minima.

Errori:

```text
ScenarioId vuoto
ScenarioId duplicato
Scenario Definition mancante
PrimaryCategory non valida
Selected Scenario non appartiene alla Category selezionata
CharacterId inesistente
FactionId inesistente
MilestoneId invalido, se catalogato
```

Warning:

```text
Scenario senza Description
Scenario Showcase senza Milestone
Scenario Characters senza CharacterIds
Scenario Factions senza FactionIds
Scenario Automated senza assertions, se applicabile
```

Non introdurre validator eccessivamente complessi.

---

# 26. Logging

Aggiungere log sintetici.

Esempio:

```text
[RTScenario] Category=TeamCombos
[RTScenario] Selected=TeamCombo.Conflux.WaterElectric.Basic
[RTScenario] Mode=Visual
[RTScenario] Version=1
[RTScenario] Seed=12345
```

Errore:

```text
[RTScenario][Error]
Scenario 'Character.Flux.BasicKit'
does not belong to selected category 'Environment'.
```

Usare categorie log già esistenti se disponibili.

---

# 27. Automation Tests obbligatori

Aggiungere test automatici sul nuovo sistema.

## Test 1 — Stable lookup

```text
ScenarioId
    ↓
registry
    ↓
stessa definition
```

---

## Test 2 — Category filtering

Dato:

```text
Flux.BasicKit -> Characters
WaterElectric -> TeamCombos
```

Query:

```text
Characters
```

deve restituire Flux e NON WaterElectric.

---

## Test 3 — Unique ID

Due scenario con stesso Stable ID devono causare validation failure.

---

## Test 4 — Category mismatch

Configurazione:

```text
Category = Environment
Scenario = Character.Flux.BasicKit
```

deve essere rifiutata/azzerata esplicitamente.

---

## Test 5 — Metadata does not change simulation

Stesso scenario, stesso seed, stessi intent.

Modificare soltanto un metadata di classificazione non competitivo.

Assert:

```text
StateHash identico
LogHash identico
```

se hash e harness sono già disponibili.

---

## Test 6 — Existing scenario regression

Almeno uno scenario già esistente deve continuare a:

```text
load
run
complete
PASS
```

---

# 28. Editor test manuale

Verificare in Editor:

1. aprire la mappa di sviluppo corrente;
2. selezionare `BP_GameMode` / World Settings / actor di configurazione reale;
3. aprire sezione `Scenario`;
4. scegliere `Characters`;
5. verificare che compaiano solo scenari Character;
6. scegliere `Team Combos`;
7. verificare che la lista cambi;
8. scegliere uno scenario;
9. verificare `Scenario Info`;
10. premere Play;
11. verificare che venga eseguito lo ScenarioId corretto;
12. cambiare categoria rendendo invalido lo scenario selezionato;
13. verificare che la selezione venga pulita o segnalata;
14. riaprire l'Editor / mappa e verificare persistenza configurazione.

---

# 29. Packaged / Headless

La categoria è principalmente UX Editor.

Verificare che:

```text
Headless run by ScenarioId
```

continui a funzionare senza dipendere dal widget/details panel.

Esempio concettuale:

```text
RunScenario TeamCombo.Conflux.WaterElectric.Basic
```

Usare la CLI/comando reale già presente.

Non inventare un comando se il progetto ne possiede già uno.

---

# 30. Cose da NON fare

NON:

- creare una copia dello stesso scenario in più cartelle per farlo apparire in più categorie;
- usare array index come ScenarioId;
- mettere tutti i dati scenario dentro `BP_GameMode`;
- hard-codare Flux/Riva/Conflux nel framework;
- cambiare il resolver;
- cambiare il TurnLog canonico senza necessità;
- introdurre un secondo Test Harness;
- creare un nuovo plugin solo per il selector se non serve;
- creare una UI runtime complessa;
- creare un editor visuale degli scenari;
- implementare ora ricerca full-text;
- introdurre modding pubblico;
- cambiare action economy;
- cambiare regole di reaction;
- modificare risultati competitivi.

---

# 31. File attesi

I file esatti dipendono dalla repository.

Prima individuarli.

Probabili aree:

```text
Source/RefactorTactics/
    Scenario/
    Tests/
    Framework/

Content/RefactorTactics/
    Blueprints/Framework/
    Tests/
    Scenarios/

Config/Tags/

Docs/
```

Possibili file nuovi/modificati:

```text
RTScenarioCategory.h
RTScenarioDefinition.h/.cpp
RTScenarioRegistry.h/.cpp
RTGameMode.h/.cpp
RTTestDirector.h/.cpp
RTScenarioTests.cpp
```

Questi nomi sono indicativi.

Riutilizzare classi esistenti invece di crearne di parallele.

---

# 32. Aggiornamento documentazione

Aggiornare la documentazione tecnica pertinente.

Deve risultare chiaro:

```text
ScenarioId = identità stabile runtime
PrimaryCategory = navigazione principale Editor
Metadata = classificazione multipla
ExecutionMode = modalità di esecuzione
Milestone = metadata, NON categoria
```

Aggiornare anche:

- documentazione Test Harness;
- documentazione BP_GameMode;
- eventuale scenario authoring guide;
- eventuale wiki generation guide;
- roadmap se la feature non era ancora pianificata.

---

# 33. Issue / roadmap

Se la repository usa GitHub Issues/roadmap:

creare o aggiornare una issue equivalente a:

```text
Scenario Browser: category + metadata filtering in BP_GameMode
```

Acceptance criteria:

```text
[ ] Category selector visibile nell'Editor
[ ] Scenario list filtrata
[ ] Stable ScenarioId preservato
[ ] metadata Character/Faction/Milestone supportati
[ ] nessuna duplicazione scenario
[ ] scenario esistenti riclassificati
[ ] validation
[ ] automation tests
[ ] visual run verificato
[ ] headless regression verificata
[ ] docs aggiornate
```

Integrarla nella milestone corretta senza alterare arbitrariamente le priorità globali.

---

# 34. Definition of Done

La task è Done quando:

1. `BP_GameMode` presenta una sezione Scenario organizzata;
2. posso scegliere una `Category`;
3. la proprietà Scenario mostra soltanto scenari coerenti;
4. ogni scenario mantiene uno Stable ScenarioId;
5. ogni scenario possiede una sola PrimaryCategory;
6. Character/Faction/Feature/Milestone sono metadata separati;
7. nessuno scenario è duplicato per apparire in più viste;
8. almeno gli scenari esistenti sono riclassificati;
9. uno scenario può essere lanciato normalmente dall'Editor;
10. Headless/Fast continuano a risolvere per ScenarioId;
11. validation intercetta ID duplicati e category mismatch;
12. Automation Tests passano;
13. build Development Editor passa;
14. se previsto dalla milestone, packaged smoke test passa;
15. docs sono aggiornate.

---

# 35. Output finale richiesto a Claude

Al termine produrre un report con:

## A. Audit iniziale

```text
- implementazione scenario trovata;
- BP/GameMode coinvolti;
- Test Harness coinvolto;
- sistema dati attuale;
- eventuali differenze rispetto a questo handoff.
```

## B. Decisioni implementative

Spiegare perché è stata scelta:

```text
GetOptions
oppure
existing selector
oppure
Details Customization
```

e perché è la soluzione minima scalabile.

## C. File modificati

Tabella:

| File | Modifica |
|---|---|

## D. Scenari riclassificati

| ScenarioId | Category | Characters | Factions | Milestone |
|---|---|---|---|---|

## E. Test eseguiti

Indicare comando + risultato.

## F. Eventuali asset da resave

Elencare Blueprint/Data Asset/mappa.

## G. Limitazioni rimaste

Solo problemi reali, non wishlist.

## H. Prossimo passo consigliato

Preferenza:

```text
aggiungere filtri secondari nell'Editor:
Milestone
Character
Faction
Feature
```

ma NON implementarli in questa task se aumentano sensibilmente lo scope.

---

# 36. Commit Git suggeriti

Preferire commit piccoli.

Esempio:

```text
feat(scenarios): add scenario primary categories and metadata
feat(editor): filter game mode scenario selector by category
test(scenarios): cover registry and category filtering
data(scenarios): classify existing scenarios
docs(scenarios): document categorized scenario workflow
```

Se la modifica reale è piccola, ridurre il numero di commit.

---

# 37. Principio finale

Il sistema deve restare semplice:

```text
             ┌───────────────────┐
             │ Scenario StableId │
             └─────────┬─────────┘
                       │
              ┌────────▼────────┐
              │ Scenario Data   │
              └────────┬────────┘
                       │
       ┌───────────────┼────────────────┐
       │               │                │
       ▼               ▼                ▼
PrimaryCategory   Character/Faction   Feature/Milestone
       │               │                │
       └───────────────┼────────────────┘
                       │
                       ▼
                Editor Browser
                       │
                       ▼
                 BP_GameMode
                       │
                       ▼
                Scenario Harness
                       │
                       ▼
              Real Gameplay Pipeline
```

**La classificazione serve a trovare lo scenario.  
Lo ScenarioId serve a identificarlo.  
Il resolver serve a giocarlo.**

Non mescolare queste tre responsabilità.
