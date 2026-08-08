# Prompt per Claude — Consolidamento meccanica Transformation / Character State

Devi consolidare nel progetto **RefactorTactics** le informazioni di design relative alla meccanica di **trasformazione / cambio stato / configurazione dei personaggi**, partendo dal documento:

`TRANSFORMATION_MECHANIC_EXPLORATION.md`

e confrontandolo con **lo stato reale attuale del repository**, della documentazione e della wiki.

> IMPORTANTE: il documento di input contiene sia **raccomandazioni** sia **alternative di design da conservare**.  
> NON trasformare automaticamente tutte le proposte in decisioni definitive o feature da implementare subito.

---

# 1. Obiettivo generale

Integrare in modo coerente la meccanica di trasformazione dei personaggi nel progetto, aggiornando dove necessario:

- documentazione tecnica e di game design;
- wiki;
- issue;
- epic;
- roadmap;
- scenari;
- test PIE;
- eventuali file XLSX usati per generare / guidare la wiki.

Il risultato finale deve lasciare il repository in uno stato **coerente, non duplicato e tracciabile**, distinguendo chiaramente:

1. decisioni consolidate;
2. prototipi da validare;
3. alternative ancora aperte;
4. idee future / post-v0.x.

---

# 2. Prima fase obbligatoria: audit dello stato attuale

PRIMA di modificare qualsiasi file:

1. analizza il repository;
2. individua i documenti che già trattano:
   - personaggi;
   - skill;
   - passive;
   - stati;
   - stance;
   - trasformazioni;
   - Overdrive;
   - configurazioni;
   - terreno;
   - Action Ghost;
   - Planning;
   - Prep;
   - Fast Reaction;
   - scenari;
   - testing PIE;
   - roadmap;
   - issue/epic;
3. individua eventuali concetti equivalenti già presenti ma con nomi differenti;
4. identifica contraddizioni;
5. evita di creare documenti nuovi se esiste già un documento canonico appropriato.

Non duplicare specifiche esistenti.

Se il concetto esiste già, **estendilo o consolidalo**.

---

# 3. Fonte di design da consolidare

Usa come fonte principale:

`TRANSFORMATION_MECHANIC_EXPLORATION.md`

La meccanica non va interpretata come semplice "trasformazione estetica" o "buff".

Il concetto centrale da consolidare è un framework generale:

# Character State / Configuration System

che può supportare diverse categorie:

- `STANCE`
- `FORM`
- `OVERDRIVE`
- `ENVIRONMENTAL STATE`
- `CONFIGURATION`

Il framework deve permettere a personaggi diversi di usare la stessa infrastruttura tecnica pur presentando al giocatore meccaniche tematicamente differenti.

Esempi:

- Steel cambia **stance**;
- GRIM.exe cambia **kernel/configurazione**;
- Serath cambia **forma**;
- Khaimera entra in **Frenzy/Overdrive**;
- Aurora entra in uno stato ambientale;
- Howitzer/Vektor entrano in **Siege**.

---

# 4. Principio di design da preservare

La trasformazione deve:

> aumentare le decisioni strategiche più di quanto aumenti le informazioni da ricordare.

Il sistema NON deve diventare:

- un semplice `+X% damage`;
- un doppio kit completo per ogni personaggio;
- un toggle gratuito continuamente ottimizzato;
- una meccanica obbligatoria per tutto il roster.

Il framework può essere comune.

La percezione lato giocatore deve invece rimanere **specifica per personaggio**.

---

# 5. Complexity Budget

Consolida il concetto di **budget di complessità del personaggio** se coerente con l'architettura/documentazione attuale.

Una trasformazione pesante consuma parte rilevante del budget di complessità.

Di conseguenza un personaggio trasformista dovrebbe, in generale, evitare di avere contemporaneamente troppi altri sistemi pesanti, ad esempio:

- summon;
- pet;
- trappole;
- molte Fast Reaction;
- molte risorse/stack;
- skill ambientali complesse;
- doppio kit;
- numerose eccezioni alle fasi.

Non è necessario cristallizzare valori numerici rigidi se il progetto non usa ancora un sistema quantitativo.

Puoi mantenere i valori presenti nel documento come **euristica di design**, non come regola matematica vincolante.

---

# 6. Integrazione con il sistema delle fasi

Preserva il vincolo canonico:

**Decision / Planning → Prep → Dash → Blast → Move**

Il normale **Move è sempre l'ultima fase volontaria**.

La trasformazione/configurazione dovrebbe normalmente:

1. essere scelta durante Planning;
2. risolversi in Prep;
3. modificare le azioni/preview delle fasi successive;
4. aggiornare gli Action Ghost;
5. non introdurre sequenze arbitrarie che violano il modello delle fasi.

Valuta eccezioni solo per casi esplicitamente classificati come:

- trigger;
- Fast Reaction;
- Environmental State;
- Emergency Configuration.

---

# 7. Action Ghost e UI/UX

Consolida il requisito che una trasformazione pianificata sia leggibile PRIMA del Commit.

L'Action Ghost deve poter rappresentare:

- stato/form prevista;
- range aggiornato;
- movimento aggiornato;
- skill mutate;
- eventuali skill non disponibili;
- posizione prevista;
- eventuali nuove aree di controllo.

Lo stato attivo deve inoltre essere facilmente leggibile tramite:

- HUD;
- icona;
- colore/VFX;
- silhouette;
- tooltip;
- preview.

Evitare stati importanti che il giocatore debba ricordare senza supporto visivo.

---

# 8. Commitment / Revert

Consolida il concetto di **commitment**.

Una forma significativa non dovrebbe essere gratuitamente reversibile in qualsiasi momento.

Possibili strumenti:

- Transform in Prep;
- Revert in Prep;
- cooldown;
- durata minima;
- costo di risorsa;
- limitazione temporanea.

NON scegliere automaticamente un'unica soluzione globale.

Il framework deve consentire regole differenti per personaggio.

---

# 9. Alternative Light / Medium / Signature

Il documento contiene per molti personaggi tre livelli:

- **Light**
- **Medium**
- **Signature**

Queste NON sono tre feature da assegnare contemporaneamente.

Devono essere considerate **alternative di design**.

Conservale nella documentazione appropriata come:

- candidates;
- design alternatives;
- future evolution;
- rejected-for-now ma non eliminate;
- possibili conversioni in passive/skill/ultimate/scenario mechanic.

Non perdere queste alternative durante il consolidamento.

---

# 10. Priorità attuale suggerita

Mantieni come raccomandazione, NON necessariamente come decisione irreversibile:

## v0.1

Roster:

- Flux;
- Riva;
- Bastion;
- Vektor.

### Vektor
Candidato principale per una vera **Alternate Form**:

`Mobile ↔ Siege`

Obiettivo:
validare il framework con un cambio di configurazione leggibile e con commitment.

### Flux
Candidato per **Environmental State**:

`Normal → Charged`

Obiettivo:
validare stato derivato/interconnesso col terreno.

### Bastion
Candidato per **Stance / Configuration**:

`Normal ↔ Bulwark`

Obiettivo:
validare un personaggio che diventa pseudo-cover / elemento tattico.

### Riva
Candidato per **Lightweight State**:

`Normal → Flow`

Obiettivo:
validare uno stato leggero senza doppio kit.

---

# 11. Personaggi futuri / alternative

Conserva le alternative del documento per tutto il roster.

In particolare, mantieni come candidati forti da studiare:

## Tier alto

- GRIM.exe — Kernel Override;
- Kwang — Stormbound;
- Serath — Radiant / Corrupted;
- Crunch — Power Routing;
- Aurora — Winter Avatar;
- Terra — Living Rampart;
- Wukong — Clone Form;
- Gideon — Event Horizon;
- Morigesh — Swamp Avatar;
- Howitzer — Siege Platform;
- Riktor — Chain Warden;
- Iggy & Scorch — Inferno Engine.

Non trasformare questa lista in scope immediato.

Deve entrare nella roadmap/design backlog nella forma appropriata.

---

# 12. Documentazione da aggiornare

Individua i file canonici prima di intervenire.

Aggiorna, se presenti e pertinenti:

- documentazione core gameplay;
- character architecture;
- ability system;
- character state/status system;
- action phases;
- Planning;
- Action Ghost;
- terrain/environment interactions;
- Fast Reaction;
- Overwatch se coinvolto;
- testing;
- scenario framework;
- roadmap;
- release/version scope;
- character roster docs.

Se manca un documento canonico per il `Character State / Configuration System`, valuta la creazione di un nuovo documento dedicato.

In tal caso:

- inseriscilo nella struttura documentale corretta;
- collegalo dai documenti correlati;
- evita duplicazioni;
- definisci chiaramente il livello di maturità della specifica.

---

# 13. Wiki

La wiki si trova in una cartella/repository separata:

`refactor-tactics-main.wiki`

Analizzala separatamente.

Aggiorna la wiki SOLO dove migliora la documentazione rivolta a:

- giocatori;
- designer;
- tester;
- contributor.

Possibili aree:

- meccaniche;
- stati/form;
- pagine personaggio;
- terminologia;
- scenari dimostrativi;
- pagina generale Transformation / Character States.

## Importante

Non riversare nella wiki interna tutte le alternative sperimentali se rendono la pagina confusa.

Distingui:

### Wiki player-facing
Solo meccaniche consolidate e leggibili.

### Wiki design/dev
Se la wiki supporta contenuti di sviluppo, può contenere riferimenti più dettagliati.

Le alternative di design complete possono rimanere prevalentemente nei docs del repository.

---

# 14. XLSX che guidano la wiki

Verifica se esistono file `.xlsx` utilizzati per:

- elenco pagine wiki;
- mapping contenuti;
- generazione automatica;
- metadata;
- categorie;
- immagini;
- riferimenti tra personaggi/meccaniche/scenari.

Se le modifiche alla wiki richiedono aggiornamenti agli XLSX:

1. aggiorna gli XLSX;
2. preserva struttura, formule, sheet e convenzioni esistenti;
3. evita divergenza tra XLSX e wiki generata;
4. documenta cosa hai modificato.

Se NON sono coinvolti, non modificarli inutilmente.

---

# 14A. XLSX / Design Matrices da verificare, estendere o creare

Oltre ad aggiornare eventuali XLSX già usati per guidare la wiki, verifica se il progetto possiede già matrici equivalenti.

NON creare duplicati.

Se esistono sheet o workbook che svolgono già queste funzioni:

- estendili;
- mantieni naming e convenzioni esistenti;
- aggiungi le colonne mancanti;
- conserva formule, validation list, named range, formatting e automazioni.

Se non esiste una struttura equivalente e gli XLSX sono già parte del workflow di design/wiki, aggiungi le matrici seguenti nel workbook canonico più appropriato.

Le matrici devono servire sia come **strumento di design** sia come **fonte di tracciabilità**.

## Matrix 1 — Character × State Type

Scopo: vedere rapidamente quali categorie del `Character State / Configuration System` sono utilizzate da ciascun personaggio.

Colonne minime suggerite:

```text
CharacterId
CharacterName
Version
Faction
HasStance
HasForm
HasOverdrive
HasEnvironmentalState
HasConfiguration
PrimaryStateType
RecommendedCandidate
DesignStatus
Notes
```

Valori `DesignStatus` consigliati, adattandoli agli enum già esistenti:

```text
IDEA
PROPOSED
PROTOTYPE
VALIDATING
APPROVED
DEFERRED
REJECTED
```

Questa matrice NON implica che ogni personaggio debba possedere uno stato speciale.

---

## Matrix 2 — Transformation Candidate Matrix

Questa è la matrice principale per conservare tutte le alternative discusse.

Struttura suggerita:

```text
CharacterId
CharacterName
LightCandidate
LightType
LightWeight
MediumCandidate
MediumType
MediumWeight
SignatureCandidate
SignatureType
SignatureWeight
RecommendedCandidate
RecommendedTier
CurrentDecision
TargetVersion
PrototypePriority
Rationale
DocsRef
WikiRef
```

Esempio concettuale:

| Character | Light | Medium | Signature | Recommended | Status | Target |
|---|---|---|---|---|---|---|
| Flux | Charged | Conductor | Living Current | Charged | Prototype | v0.1 |
| Riva | Flow | Mist Form | Tidal Form | Flow | Prototype | v0.1 |
| Bastion | Fortified | Bulwark | Citadel | Bulwark | Prototype | v0.1 |
| Vektor | Stabilized | Siege | Weapons Platform | Siege | Priority Prototype | v0.1 |
| Steel | Guard | Assault | Juggernaut | Guard/Assault | Proposed | v0.2 |
| Aurora | Frostbound | Crystal | Winter Avatar | Frostbound | Proposed | v0.2 |
| Murdock | Targeting | Hunter | Deadeye | Targeting | Proposed | v0.2 |
| Kwang | Blade | Storm | Stormbound | Stormbound | Future Signature | v0.2+ |

IMPORTANTE:

- Light / Medium / Signature sono **alternative**, non feature cumulative;
- non eliminare candidate non selezionate;
- una candidata non scelta può in futuro diventare Passive, Skill, Ultimate/Super, Scenario mechanic, Future Form, PvE modifier o Experimental feature;
- la wiki player-facing dovrebbe normalmente leggere solo candidate `APPROVED` o equivalenti.

---

## Matrix 3 — State × Gameplay System Interaction

Scopo: mostrare quali sistemi di gameplay vengono modificati da ogni stato.

Colonne suggerite:

```text
StateId
CharacterId
StateName
StateType
ActivationPhase
PlanningImpact
PrepImpact
DashImpact
BlastImpact
MoveImpact
BasicAttackImpact
SkillOverrides
PassiveOverrides
OverwatchImpact
FastReactionImpact
TerrainImpact
CoverImpact
LOSImpact
FacingImpact
CollisionImpact
PathfindingImpact
ResourceImpact
CooldownImpact
ActionGhostRequired
UIStateRequired
NetworkImpact
Notes
```

Esempio:

| State | Prep | Dash | Blast | Move | Terrain | Cover | Overwatch | Ghost |
|---|---|---|---|---|---|---|---|---|
| Vektor Siege | Transform | disabled | modified | reduced | — | — | enhanced | required |
| Flux Charged | env trigger | modified | modified | normal | strong | — | possible | required |
| Bastion Bulwark | stance | normal | normal | reduced | — | pseudo-cover | modified | required |
| Riva Flow | trigger | normal | possible bonus | normal | possible | — | — | recommended |

Questa matrice deve aiutare a individuare rapidamente il costo sistemico di una trasformazione.

---

## Matrix 4 — Character Complexity Matrix

Scopo: evitare personaggi troppo caricati di sistemi complessi.

Non trasformare necessariamente questa matrice in un punteggio matematico rigido. Può essere usata come **euristica comparativa**.

Colonne suggerite:

```text
CharacterId
CharacterName
PassiveComplexity
SkillComplexity
TerrainComplexity
ReactionComplexity
OverwatchComplexity
SummonComplexity
TrapComplexity
ResourceStackComplexity
TransformationComplexity
MovementComplexity
TotalIndicativeComplexity
ComplexityBand
RiskNotes
Mitigation
```

`ComplexityBand` possibile:

```text
LOW
MEDIUM
HIGH
VERY_HIGH
```

Se il progetto possiede già un sistema di complexity rating, usare quello.

Obiettivo: un personaggio con `TransformationComplexity = HIGH` dovrebbe normalmente avere minore complessità in altri sottosistemi.

---

## Matrix 5 — State Validation Matrix

Scopo: collegare ogni candidata/prototipo alla validazione effettiva.

Colonne suggerite:

```text
StateId
CharacterId
StateName
DesignStatus
TargetVersion
ScenarioId
ScenarioStatus
PIETestId
PIETestStatus
AutomationStatus
LoggingCoverage
AcceptanceCriteriaRef
KnownRisks
ValidationResult
LastValidated
```

Deve essere possibile capire immediatamente:

- se esiste uno scenario;
- se esiste un PIE test;
- se il test è automatizzato;
- se il logging è sufficiente;
- se la feature è stata validata;
- se è solo una proposta.

---

## Matrix 6 — Feature Traceability Matrix

Questa matrice è fortemente consigliata se non esiste già un equivalente.

Scopo: mantenere sincronizzati:

```text
Feature
→ Character
→ State
→ Epic
→ Issue
→ Roadmap
→ Version
→ Scenario
→ PIE Test
→ Docs
→ Wiki
```

Colonne suggerite:

```text
FeatureId
FeatureName
CharacterId
StateId
EpicId
IssueIds
RoadmapItemId
TargetVersion
ScenarioIds
PIETestIds
DocsRefs
WikiRefs
XlsxSource
Status
Owner
Notes
```

Esempio concettuale:

```text
Character State / Alternate Form
→ Vektor
→ Siege
→ EPIC Character State
→ ISSUE-xxx
→ v0.1 Roadmap
→ SCN_TRANSFORM_VEKTOR_01
→ PIE_STATE_VEKTOR_001
→ Docs/...
→ Wiki/...
```

Questa matrice deve ridurre la possibilità che roadmap, issue, wiki, scenari e test divergano.

---

## Matrix 7 — Wiki Publication Matrix

Crearla solo se il workflow XLSX/wiki già utilizza matrici di pubblicazione o metadata.

Scopo: decidere quali informazioni del design sono pubblicabili nella wiki.

Colonne suggerite:

```text
CharacterId
StateId
DesignStatus
PlayerFacing
DeveloperFacing
PublishToWiki
WikiPage
WikiSection
ShowAlternativeCandidates
ShowTargetVersion
ShowRoadmapStatus
ScenarioLinks
GeneratedFromXlsx
Notes
```

Regola consigliata:

- `APPROVED` → pubblicabile;
- `PROTOTYPE` → pubblicabile solo se la wiki mostra chiaramente lo stato sperimentale;
- `PROPOSED` → normalmente docs/design only;
- alternative Light/Medium/Signature non selezionate → normalmente NON player-facing.

---

# 14B. Regole di sincronizzazione XLSX ↔ Docs ↔ Wiki

Quando gli XLSX fungono da fonte o supporto per la generazione della wiki:

1. identificare la **source of truth** per ogni campo;
2. non modificare manualmente nella wiki valori generati da XLSX se verrebbero sovrascritti;
3. aggiornare prima il datasource canonico;
4. rigenerare/aggiornare la wiki secondo il workflow esistente;
5. verificare il diff finale;
6. evitare che `RecommendedCandidate`, `DesignStatus` e `TargetVersion` divergano fra XLSX, docs e wiki.

Pipeline concettuale consigliata:

```text
Design Candidate
        ↓
Transformation Candidate Matrix
        ↓
Recommended / Approved decision
        ↓
Feature Traceability Matrix
        ↓
Roadmap / Issue / Scenario / PIE
        ↓
Wiki Publication Matrix
        ↓
Wiki
```

Non imporre questa pipeline se il repository usa già una pipeline differente: adattarla al sistema esistente.

---

# 14C. Formule / controlli XLSX consigliati

Se coerente con gli XLSX esistenti, aggiungere controlli automatici per evidenziare inconsistenze.

### Missing validation
Segnalare uno stato `APPROVED` senza ScenarioId, PIETestId o DocsRef.

### Missing roadmap linkage
Segnalare un prototipo con TargetVersion valorizzata ma nessun RoadmapItem/Issue.

### Wiki inconsistency
Segnalare `PublishToWiki = TRUE` quando `DesignStatus = IDEA/PROPOSED`, salvo override esplicito.

### Complexity risk
Segnalare personaggi con TransformationComplexity, SummonComplexity, ReactionComplexity e TerrainComplexity tutte alte contemporaneamente.

Questi controlli devono seguire lo stile degli XLSX esistenti e non rompere compatibilità con eventuali generatori.

---

# 14D. Requisito nel report finale relativo alle matrici

Nel report finale Claude deve specificare:

```text
XLSX / MATRICES

Workbook analizzati:
- ...

Sheet esistenti riutilizzati:
- ...

Sheet creati:
- ...

Colonne aggiunte:
- ...

Matrici non create perché già coperte da:
- ...

Formule / validation / conditional formatting aggiunti:
- ...

Riferimenti sincronizzati con wiki:
- ...

Gap residui:
- ...
```

Se nessun XLSX è realmente parte del workflow di questa feature, dichiararlo esplicitamente e NON creare workbook isolati privi di integrazione.

---

# 15. Issue ed Epic

Analizza issue ed epic già esistenti PRIMA di crearne di nuovi.

Per ogni elemento:

- aggiorna issue esistenti quando coprono già il tema;
- evita duplicati;
- collega le issue alle epic appropriate;
- crea nuove issue solo per gap reali.

Possibili workstream da rappresentare, se mancanti:

## Epic candidata
`Character State / Configuration System`

Possibili child issue:

- core state framework;
- activation/revert rules;
- ability override system;
- movement override;
- Action Ghost state preview;
- HUD/VFX state representation;
- environmental state triggers;
- Fast Reaction integration;
- serialization/data-driven config;
- debugging/logging;
- PIE automation coverage.

## Character prototype issues

Eventualmente:

- Vektor Siege prototype;
- Flux Charged prototype;
- Bastion Bulwark prototype;
- Riva Flow prototype.

Le issue devono avere:

- obiettivo;
- scope;
- acceptance criteria;
- dependencies;
- test strategy;
- target version/milestone se coerente.

Non creare issue per tutte le Signature future se non sono necessarie.

Le alternative future possono essere raccolte in design backlog / epic notes / future roadmap.

---

# 16. Roadmap

Aggiorna la roadmap per riflettere:

1. framework generico;
2. prototipi;
3. validazione;
4. eventuale estensione futura.

La roadmap dovrebbe distinguere chiaramente:

### v0.1
Validazione minima del framework.

Possibili casi:

- Vektor — Alternate Form;
- Flux — Environmental State;
- Bastion — Stance/Configuration;
- Riva — Lightweight State.

### v0.2+
Espansione solo dopo validazione.

### Future
Signature transformations / advanced map-transforming states.

Non espandere lo scope della v0.1 senza una motivazione forte.

---

# 17. Scenari

Aggiorna il catalogo degli scenari da creare.

Devono esistere scenari specifici per testare almeno:

## Scenario A — Vektor Siege

Validare:

- Planning;
- Transform;
- Prep;
- range override;
- ability override;
- movement restrictions;
- Action Ghost;
- Revert.

---

## Scenario B — Flux Charged

Validare:

- trigger ambientale;
- Electrified Tile;
- Charged state;
- skill mutation;
- conduzione;
- eventuali interazioni con Wet Tile.

---

## Scenario C — Bastion Bulwark

Validare:

- stance/configuration;
- cover;
- line of sight;
- projectile interaction;
- ally protection;
- collision/pathing.

---

## Scenario D — Riva Flow

Validare:

- stato leggero;
- activation condition;
- duration;
- expiration;
- UI readability.

---

## Scenario E — Multi-State Stress Test

Almeno un test/scenario futuro dovrebbe verificare più tipi di stato contemporaneamente.

Obiettivo:

misurare la leggibilità e il carico sistemico, non solo la correttezza tecnica.

---

# 18. Test PIE

Aggiorna il piano dei test PIE.

Preferire test riproducibili e automatizzabili.

Aggiungere test per:

## Core State

- activation;
- invalid activation;
- duration;
- revert;
- forced revert;
- cooldown;
- resource cost;
- state replacement;
- mutually exclusive states;
- death/reset;
- round transition.

## Overrides

- stats;
- movement;
- Dash;
- Blast;
- range;
- ability;
- passive;
- Overwatch se applicabile.

## Phase correctness

Verificare che:

- Planning registri correttamente lo stato futuro;
- Prep applichi Transform;
- Dash usi la nuova configurazione;
- Blast usi la nuova configurazione;
- Move rispetti le nuove restrizioni.

## Action Ghost

Verificare:

- ghost form;
- range preview;
- movement preview;
- disabled actions;
- final predicted position.

## Environment

Verificare:

- trigger da terreno;
- entrata;
- uscita;
- terreno modificato durante il turno;
- conflitti fra Environmental State.

## Fast Reaction

Solo dove supportato:

- trigger;
- timeout;
- accept;
- decline;
- mutually exclusive reactions.

## Networking / determinism

Se pertinente all'architettura attuale:

- stessa sequenza;
- stesso stato;
- stesso risultato;
- replay/log consistente.

---

# 19. Logging per test automatici

Se il progetto utilizza log per permettere a Claude di valutare test PIE, assicurati che gli eventi di stato siano tracciabili.

Idealmente loggare eventi simili a:

```text
CharacterStateRequested
CharacterStateActivated
CharacterStateRejected
CharacterStateExpired
CharacterStateReverted
CharacterStateAbilityOverrideApplied
CharacterStateMovementOverrideApplied
CharacterStateEnvironmentTrigger
```

Ogni evento dovrebbe essere sufficiente a ricostruire:

- character;
- previous state;
- new state;
- trigger;
- phase;
- round;
- reason;
- relevant target/tile.

Adatta naming e struttura agli standard già presenti nel progetto.

---

# 20. Metriche di validazione gameplay

Se esiste un documento di playtest/telemetry, aggiungere metriche suggerite:

- tempo di Planning aggiunto;
- numero medio di cambi forma;
- frequenza Transform/Revert;
- stato dominante;
- azioni invalidate;
- errori di previsione;
- uso delle skill mutate;
- win rate per stato;
- numero di stati contemporanei;
- frequenza di consultazione tooltip, se misurabile.

Segnale positivo:

> "Devo scegliere quale configurazione usare."

Segnale negativo:

> "Non ricordo cosa cambia."

---

# 21. Terminologia

Prima di introdurre nuovi termini, verifica glossario e naming esistente.

Possibile gerarchia:

```text
Character State
├── Stance
├── Form
├── Overdrive
├── Environmental State
└── Configuration
```

NON rinominare sistemi già consolidati senza necessità.

Se esiste una terminologia migliore nel progetto, adattare questa proposta mantenendo il concetto.

---

# 22. Deliverable richiesto

Alla fine del lavoro devi produrre un report sintetico contenente:

## A. Audit
- documenti trovati;
- sistemi equivalenti;
- conflitti rilevati.

## B. Docs
- file modificati;
- file creati;
- decisioni consolidate;
- alternative preservate.

## C. Wiki
- pagine modificate;
- pagine create;
- motivazione.

## D. XLSX
- file modificati;
- sheet/range interessati;
- oppure dichiarazione "nessuna modifica necessaria".

## E. Issues / Epic
Per ogni issue:

- ID;
- titolo;
- nuova/modificata;
- epic;
- milestone/version;
- motivazione.

## F. Roadmap
- elementi aggiunti/spostati;
- versioni interessate.

## G. Scenari
- scenari nuovi;
- scenari aggiornati;
- obiettivo di ciascuno.

## H. PIE
- test aggiunti;
- test aggiornati;
- coverage ancora mancante.

## I. Open questions
Solo dubbi realmente non risolvibili dal repository.

## J. Diff concettuale finale
Riassumi:

```text
PRIMA
→

DOPO
```

per spiegare come la meccanica Transformation / Character State è stata integrata.

---

# 23. Regole operative

- Non cancellare alternative solo perché non selezionate.
- Non trasformare brainstorming in scope obbligatorio.
- Non duplicare documentazione.
- Non creare issue duplicate.
- Non espandere v0.1 senza motivo.
- Non modificare XLSX se non necessari.
- Non introdurre naming parallelo a sistemi già esistenti.
- Preferire configurazione data-driven.
- Preservare la sequenza delle fasi canoniche.
- Collegare docs ↔ roadmap ↔ issue ↔ scenario ↔ PIE test quando possibile.
- Dove una scelta non è ancora validata, marcarla esplicitamente come `PROPOSED`, `EXPERIMENTAL` o equivalente già usato nel progetto.

---

# 24. Priorità finale

L'obiettivo NON è "implementare tutte le trasformazioni".

L'obiettivo è:

1. consolidare il concetto;
2. costruire una base architetturale coerente;
3. preservare le alternative;
4. identificare prototipi minimi;
5. collegarli a roadmap, issue, scenari e test;
6. permettere di decidere in seguito quali personaggi meritano davvero una trasformazione completa.

Procedi direttamente con l'audit e il consolidamento del repository.
