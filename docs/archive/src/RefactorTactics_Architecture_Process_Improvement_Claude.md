# RefactorTactics — Handoff Claude Code
## Applicazione dei miglioramenti architetturali e di processo emersi dall'audit del 2026-08-17

> `HISTORICAL` · **Materiale NON autorevole**, archiviato il **2026-08-17**.
> **Non si applica**: si legge per sapere da dove viene una decisione.
>
> ⚠️ **Questo sorgente è stato consumato da un'altra sessione, non da chi lo archivia.** La sua
> riscrittura è `docs/technical/piano-riduzione-hotspot.md` — 1293 righe, branch
> `docs/piano-riduzione-hotspot`, worktree `D:/rt-simulation`. Quel documento dichiara di correggere
> **sette** affermazioni di questa pagina misurate false o superate dallo stato live, e chiude con
> *«se esiste ancora in root, va rimosso»*: questa archiviazione esegue quell'atto conservando la
> provenienza, che è ciò per cui `docs/archive/src/` esiste.
>
> 🔴 **Al momento dell'archiviazione il branch che lo consuma NON era pushato**, quindi il piano non
> compare in `git ls-remote` né in una PR: lo vede solo `git worktree list`. È dichiarato qui perché
> chi legge questa pagina possa trovare il proprio successore anche se il branch non è ancora
> atterrato — e perché un write-set che nessuno può leggere non è, di fatto, dichiarato.
>
> ⏸️ **Nessun atto di questa pagina è stato applicato da questa archiviazione.** Le issue che nomina
> esistono già e restano dei loro owner: **#886** (P0, track `simulation`), **#833**, **#170**,
> **#950** (preflight), **#1109** (naming gate sui `.yaml`), **#1096** (handoff QA). La sua §3
> (workstream `BASE` + `WS-A`…`WS-D`) è la stessa materia di
> [`cinque-processi-paralleli-2026-08-17.md`](../../roadmap/plans/cinque-processi-paralleli-2026-08-17.md),
> scritto lo stesso giorno e da una sessione diversa: due letture indipendenti dello stesso problema,
> e il piano hotspot è la terza. Chi riconcilia le confronti invece di sommarle.

> **Scopo**
>
> Applicare in modo incrementale i miglioramenti emersi dall'audit dello stato reale di `main`, senza introdurre un mega-refactor e senza rallentare inutilmente la v0.1.
>
> Questo documento è un **mandato operativo per Claude Code**. Non è una nuova fonte di verità sullo stato delle issue: prima di eseguire qualunque modifica, rileggere `main`, GitHub e gli owner strutturati del repository.

---

# 0. Regole non negoziabili

1. **Repository prima del documento.**
   - Leggere `AGENTS.md`, `CLAUDE.md` o equivalenti se presenti.
   - Fare `git fetch --prune`.
   - Verificare `origin/main`.
   - Leggere gli owner correnti:
     - `docs/roadmap/feature-registry.yaml`
     - `docs/roadmap/execution-graph.yaml`
     - `docs/roadmap/parallel-batch.yaml`
     - `docs/roadmap/editor-sessions.yaml`
     - registro PIE/manuale corrente
     - issue GitHub coinvolte.
   - Questo handoff descrive **direzione e priorità**, non sostituisce lo stato live.

2. **Versione Unreal.**
   - Il `.uproject` corrente dichiara **Unreal Engine 5.8**.
   - Non inventare API UE.
   - Se la patch reale installata differisce, documentarla prima di usare API Editor/Runtime sensibili alla versione.

3. **Niente mega-refactor.**
   - Nessuna riscrittura generale di `RTTurnManager`.
   - Nessuna riscrittura generale di `RTScenarioSession`.
   - Nessuna migrazione generalizzata del GameMode in Subsystem.
   - Estrarre responsabilità **solo quando una feature corrente attraversa quel confine**.

4. **D-139 / parallelismo.**
   - Nessuna scrittura fuori dal `writable` della track.
   - Nessuna modifica a file `integration_only` da una track normale.
   - Nessun `.uasset`/`.umap` senza lease o regola corrente che ne autorizzi l'atto.
   - Una richiesta di release non è ownership.
   - Un file nuovo deve essere dichiarato **prima** di crearlo.

5. **Il gameplay reale resta l'unica pipeline.**
   - Scenario Harness non deve diventare un simulatore alternativo.
   - Nessun test-only `SetActorLocation`, `ApplyDamage` o bypass della pipeline Planning → Commit → Snapshot → Resolver → TurnLog.
   - Le animazioni/UI non decidono esiti.

6. **Determinismo e privacy restano invarianti.**
   - Same snapshot + rules + content version + seed + decisioni registrate ⇒ same result.
   - Nessun planning avversario deve essere replicato.
   - Nessuna modifica architetturale può indebolire StateHash, TurnLog o replay.

7. **Gli owner non si duplicano.**
   - GitHub possiede lo stato delle issue.
   - Feature Registry possiede lo stato feature.
   - Execution Graph possiede topologia hard/soft.
   - Parallel Batch possiede write-set/lease.
   - Editor Sessions possiede le sedute.
   - Scenario JSON possiede il contenuto scenario.
   - Le dashboard e i documenti derivati **leggono**, non copiano.

---

# 1. Obiettivo architetturale

L'architettura corrente è valida. Il lavoro richiesto è ridurre progressivamente hotspot e costo di coordinamento.

```text
                         REFACTORTACTICS

 Runtime
 ├── Core / Ability / Combat / Map / Pathfinding
 ├── Perception / Bot / Player / UI / Frontend
 ├── RTGameMode
 │    └── bootstrap + configurazione
 ├── RTTurnManager
 │    └── orchestrazione della resolution
 ├── TurnLog / StateHash / Replay
 └── Scenario Harness
      └── Index / Loader / Runner / Session / Assertions
                │
                └── usa SEMPRE il gameplay reale

 Editor
 └── RefactorTacticsEditor
      └── authoring / geometry / visual tools

 Development Control Plane
 ├── Feature Registry
 ├── Execution Graph
 ├── Parallel Batch
 ├── Editor Sessions
 ├── Scenario Corpus
 ├── PIE/Test registries
 └── GitHub
```

Target progressivo:

```text
RTGameMode
    └── orchestra bootstrap/config

RTTurnManager
    └── orchestra resolver specializzati

ScenarioSession
    └── orchestra il test, non contiene gameplay

BASE
    └── integra, rigenera, verifica e protegge i workstream
```

---

# 2. Priorità operative

## P0 — Replay delle decisioni di reazione

### Issue primaria

- **#886 — Le decisioni di reazione tornano dal TurnLog: il Verifier non le richiede**
- https://github.com/DegrassiAaron/refactor-tactics-main/issues/886

### Vincolo

Deve atterrare **prima o insieme** a:

- **#166 — CP 14.6 — Counterplay, UI della finestra e misura del pacing**
- https://github.com/DegrassiAaron/refactor-tactics-main/issues/166

### Problema

Oggi la decisione di reazione viene:

```text
Opportunity
→ decisione
→ TurnLog
→ serializzazione/hash
```

ma il Verifier può ri-simulare senza usare quella decisione come input.

Questo rompe il principio:

```text
stesso stato + stesse regole + stesso seed + stesse decisioni
= stesso risultato
```

appena entra una decisione umana.

### Implementazione richiesta

Seguire **il DoD live di #886**.

Principio da preservare:

```text
AskReactionDecision

1. RequiresDecisionBoundary?
      no  → HoldImmediate calcolato dallo stato
      yes → continua

2. se è presente una risposta registrata per OpportunityId
      usa QUELLA
      non interrogare bot/live decider

3. altrimenti
      normale ramo bot/live decider
```

### Non fare

- non consultare la traccia prima del gate di cardinalità;
- non trasformare `HoldImmediate` in una decisione registrata;
- non indicizzare per ordine di apparizione;
- non usare fallback `HOLD` per mismatch;
- non aggiungere campi al TurnLog se il formato live già porta l'identità necessaria;
- non allargare la modifica oltre `AskReactionDecision` se non dimostrato necessario.

### Test minimi

Devono esistere e fallire se la logica viene mutata:

- risposta registrata vince sul live decider;
- finestra collassata ignora la traccia;
- orphan recorded response viene segnalata;
- due finestre dimostrano che `OpportunityId` è identità, non posizione;
- `Replay.Verifier.ResimulationIsDeterministic` usa uno scenario con decisioni reali;
- mutazione “lookup per ordine” deve diventare rossa.

### Gate

Non chiudere #886 con la sola prova di serializzazione/hash.

La prova è:

```text
TurnLog decision
→ verifier
→ resolver
→ stesso risultato
```

---

# 3. Workstream consigliati

Limitare il parallelismo reale.

## BASE — Integration / Quality

Non implementa feature gameplay.

Responsabilità:

```text
fetch/rebase/merge
write-set reconciliation
binary leases
release requests
integration_only
generated views
preflight
build
automation smoke
human gate preparation
```

BASE è il quinto processo e deve essere trattato come tale.

---

## WS-A — Simulation / Replay

Ordine:

```text
#886
  ↓
#166
```

Dopo #886, #166 può introdurre UI/decisione umana senza rendere falso il replay verifier.

Non iniziare un refactor generale di `RTTurnManager` dentro #886.

Se durante #886 emerge un confine naturale, estrarre solo il minimo servizio necessario e solo se:
- riduce realmente il diff;
- non cambia semantica;
- ha test equivalenti prima/dopo;
- il write-set lo consente.

---

## WS-B — Map / Interaction

Issue cardine:

- **#833 — Chi apre D1? Il grafo sorgente → bersaglio non esiste**
- https://github.com/DegrassiAaron/refactor-tactics-main/issues/833

Obiettivo:
- completare la catena `Interact Source → Target`;
- mantenere relazione data-driven;
- ordine deterministico;
- reason code;
- niente coppie hard-coded;
- niente privacy fittizia prima che la rete la renda falsificabile.

Questa track abilita una parte del T5 dello showcase e alimenta la strada verso il golden.

Non trascinare in #833:
- rete;
- UI remota;
- privacy futura;
- ascensori;
- circuiti;
- semantiche N→1 non decise.

---

## WS-C — Scenario / Golden / Assertions

Target finale:

- **#170 — CP 15.4 — Golden replay degli 8 turni**
- https://github.com/DegrassiAaron/refactor-tactics-main/issues/170

Ma #170 legge il proprio stato dagli owner correnti.

Controllare live in particolare:
- #1060
- #75
- #833

Non copiare dentro documenti locali lo stato corrente delle tre issue.

Responsabilità di WS-C:
- estensioni dell'assertion vocabulary quando realmente necessarie;
- test discriminanti;
- capacità Scenario disponibili solo quando il percorso reale è davvero eseguibile;
- golden solo quando tutti gli otto turni sono eseguiti e asseriti.

Regola:

```text
scritto != eseguito != asserito
```

Un turno scritto ma non raggiunto non conta come copertura.

---

## WS-D — Human / Editor / Asset

È il workstream umano.

Sedute correnti rilevanti:
- U21 — luci graybox / framing;
- U22 — geometry ghost / snap / undo;
- U25 — cell placement volume / graybox readability;
- eventuali sessioni UI/asset attive nell'EditorMap live.

Questa lane:
- produce `.uasset`/`.umap`;
- esegue controlli visivi;
- registra QA manuale;
- non implementa regole del simulatore.

---

# 4. Nuova regola di parallelismo

Configurazione preferita:

```text
BASE
+
WS-A CODE
+
WS-B CODE
+
WS-C SCENARIO/CODE
+
WS-D HUMAN/EDITOR
```

Massimo:
- **3 workstream mutanti di codice/scenario simultanei**;
- **1 workstream umano Editor/Asset**;
- **1 BASE integratore**.

Tutte le altre track restano registrate ma `IDLE`.

Non massimizzare il numero di branch.
Minimizzare il wall-clock **senza creare contention**.

---

# 5. RTTurnManager — strangler refactor incrementale

## Problema

`ARTTurnManager` coordina oggi molti domini:
- movement;
- action queue;
- reaction;
- combat;
- terrain;
- cover;
- doors;
- perception;
- bot planning;
- TurnLog;
- replay;
- playback.

È il principale hotspot fisico del repository.

## Regola da adottare

```text
TurnManager ordina e coordina.
Resolver/librerie decidono.
```

### NON creare ora tutti questi tipi

Il target concettuale può essere:

```text
RTMovementResolver
RTReactionResolver
RTEnvironmentResolver
RTObjectiveResolver
RTKnowledgeService
RTPlaybackCoordinator
```

ma NON vanno creati in blocco.

### Quando estrarre

Solo durante una issue reale che modifica quella responsabilità.

Esempio:

```text
issue X modifica Reaction
    ↓
verifica se la logica è:
    a) orchestration → resta nel TurnManager
    b) pure domain rule → estrarre/riusare RTReactionLibrary
```

### Acceptance per ogni estrazione

- nessun cambiamento di StateHash;
- nessun cambiamento di TurnLog atteso;
- stessa suite verde;
- almeno un test che pinna la regola estratta;
- include dependency di `RTTurnManager.cpp` ridotta o responsabilità chiaramente ridotta;
- niente nuove dipendenze inverse dal resolver verso GameMode/UI.

---

# 6. Scenario Harness — impedire il secondo God Object

## Confini obbligatori

```text
Discovery        → RTScenarioIndex
Parsing          → RTScenarioLoader
Execution flow   → RTScenarioSession
Batch/headless   → RTScenarioRunner
Assertions       → assertion evaluator / tipi dedicati
Gameplay rule    → runtime gameplay, MAI ScenarioHarness
```

### Regola di code review

Se una feature Scenario richiede:
- un grande `switch` sulle regole gameplay;
- calcolo del danno;
- calcolo LOS;
- reazione;
- movimento;
- collisione;
- targeting;

allora il codice è nel posto sbagliato.

Scenario Harness deve:
1. leggere;
2. tradurre in intent/comandi reali;
3. avviare la pipeline reale;
4. osservare TurnLog/stato;
5. asserire.

### Refactor

Non fare un refactor generale ora.

Alla prossima modifica significativa di `RTScenarioSession.cpp`, valutare se il nuovo codice appartiene a:
- decision provider;
- assertion evaluator;
- capability registry;
- session lifecycle.

Estrarre solo quella responsabilità.

---

# 7. RTGameMode — ridurre responsabilità senza spaccarlo ora

## Stato

`ARTGameMode` gestisce oggi:
- map source;
- fallback arena;
- roster;
- Hero class mapping;
- match format;
- autobattle;
- planning override;
- scenario filter;
- scenario selection;
- scenario lifecycle;
- startup report.

## Decisione

Per la v0.1:
- non creare una costellazione di Subsystem;
- non cambiare il bootstrap senza necessità.

### Primo confine consigliato

Quando una issue reale tocca di nuovo scenario launching/configuration, introdurre un tipo dati:

```cpp
FRTScenarioRunConfig
```

o equivalente coerente con il repository.

Contenuto indicativo:

```text
ScenarioId
ExecutionMode
SeedOverride
AutoRun
AutoReady
RepeatCount
TurnPauseSeconds
Debug flags
```

Il tipo è configurazione, non autorità gameplay.

Poi il GameMode può delegare il lancio a un piccolo helper/service.

### Non fare

- non mettere Scenario Definition dentro GameMode;
- non duplicare il registry;
- non introdurre un nuovo simulatore;
- non cambiare la precedenza property / command line / cvar senza test;
- non modificare `BP_GameMode.uasset` da una track code normale.

---

# 8. Scenario metadata — evoluzione dopo la baseline corrente

## Stato attuale

L'indice usa correttamente:

```text
ScenarioId
Path
Tags[]
```

e separa ID stabile dal percorso fisico.

Questo è da preservare.

## Debito

`Tags[]` contiene contemporaneamente:
- categoria;
- personaggio;
- feature;
- milestone;
- purpose.

Per v0.1 è sufficiente.
Per crescita futura è fragile.

## Evoluzione proposta

NON implementare se aumenta lo scope di una issue corrente.

Preparare issue/ADR o task futuro per estendere il modello verso:

```text
ScenarioId
Path
PrimaryCategory
CharacterIds[]
FactionIds[]
MilestoneId
FeatureTags[]
PurposeTags[]
SearchTags[]
```

Regola:
- lo Stable ScenarioId resta l'identità;
- la classificazione non modifica simulazione;
- cambiare metadata non cambia StateHash/LogHash;
- nessuno scenario viene duplicato per apparire in due categorie.

Il browser Editor può continuare a usare `GetOptions`/filtri e diventare progressivamente più strutturato.

---

# 9. Data-driven content — non rimandare oltre il content freeze

Verificare lo stato live di U10/U11.

L'obiettivo è completare il passaggio:

```text
C++ = possibilità / invarianti
Data = variante / valori / catalogo
```

prima che troppe nuove ability vengano aggiunte hard-coded.

Priorità:
1. action/ability definitions come dati;
2. validator;
3. hero definitions;
4. catalog lookup;
5. fallback esplicito;
6. test di ID duplicato / riferimento mancante / contenuto invalido.

Non migrare tutto in un unico commit.

Una famiglia di dati alla volta.

---

# 10. Runtime / Editor boundary

## Stato

Esiste già:

```text
RefactorTactics          Runtime
RefactorTacticsEditor    Editor
```

Questo è corretto.

## Debito da verificare

Il Runtime `Build.cs` contiene ancora una dipendenza condizionale da `UnrealEd`.

Prima di rimuoverla:
1. cercare tutti gli include/simboli Editor usati dal Runtime;
2. verificare build Editor;
3. verificare Game Development;
4. verificare Game Shipping.

Solo se nessun runtime source la richiede:
- rimuovere `UnrealEd` dal modulo Runtime;
- tenere la dipendenza nel modulo Editor;
- aggiornare commenti obsoleti.

Priorità bassa.
Non farlo dentro una issue gameplay non correlata.

---

# 11. Preflight unico — miglioramento di processo P1

Issue correlata:

- **#950 — Il gate contro i test fuori dalla guardia ... manca lo script**
- https://github.com/DegrassiAaron/refactor-tactics-main/issues/950

## Obiettivo

Creare un singolo ingresso locale, ad esempio:

```text
scripts/rt-preflight.py
```

o forma già coerente con gli script del repository.

Il preflight NON deve inventare nuove regole.
Deve orchestrare gate esistenti.

Minimo consigliato:

```text
docs links
docs naming
docs symbols/tables se presenti
feature registry validate/check
generated views --check
scenario corpus validation
test guard validation
eventuali schema validator del parallel batch
fast automation subset se disponibile
```

### Modalità

```text
python scripts/rt-preflight.py --check
```

Output:
- PASS/FAIL per gate;
- comando realmente eseguito;
- exit code non-zero al primo fallimento o report aggregato coerente con gli script esistenti;
- nessun “green” inventato se un tool non è disponibile.

### Importante

Questo non sostituisce CI.
Deve essere documentato come **preflight locale/manuale**.

BASE lo esegue prima dell'integrazione.

---

# 12. Parallel Batch — semplificare l'uso, non riscriverne il modello

Il modello D-139 resta corretto.

Problema: il file sta diventando anche diario/post-mortem.

## Miglioramento richiesto

Non cambiare schema senza issue dedicata.

Da subito:
- preferire note corte e misurabili;
- spostare spiegazioni storiche lunghe in referti/PR quando non servono più a decidere ownership corrente;
- mantenere nel batch solo informazioni necessarie a:
  - status;
  - issue;
  - branch/worktree;
  - writable;
  - excludes;
  - blocked paths;
  - loans/release requests;
  - binary leases;
  - derives_from;
  - motivazione corrente.

### Regola

```text
parallel-batch.yaml = lockfile operativo del batch
non = archivio storico generale
```

Se il repository ha già una policy contraria o lo schema live richiede la provenienza in-place, NON rimuovere nulla arbitrariamente:
- creare issue di processo;
- proporre una strategia di compattazione;
- farla approvare prima.

---

# 13. Naming / source-of-truth gates

Problema misurato:
- alcuni rename erano corretti nelle viste `.md`;
- le sorgenti `.yaml` conservavano nomi legacy;
- rigenerando, i nomi vecchi tornavano.

## Direzione

I gate devono proteggere una proprietà:

```text
nessun identificatore/nome legacy nelle CURRENT SOURCES
```

non una singola estensione.

### Azione

Non ampliare `check-docs-naming.py` alla cieca.

Prima:
1. classificare quali YAML sono current sources;
2. escludere archivi/storici in modo esplicito;
3. misurare falsi positivi;
4. aggiungere test al gate;
5. solo dopo estendere.

Se non è scope di una issue corrente:
- aprire/aggiornare issue QA;
- non introdurre il cambiamento incidentalmente.

---

# 14. QA Human Gate Requests

Problema:
una track produce asset ma il registro PIE appartiene a un'altra track.

Non duplicare il registro.

## Target

Rendere strutturato l'handoff:

```text
Producer
RequestedVerification
Owner
RelatedIssue/Session
Status
```

Esempio:

```text
Producer: U24
Owner: playtest
Requested:
  - PIE-V01-FRONTEND-NAV
  - PIE-V01-FRONTEND-ERROR
```

Può essere:
- relazione in Editor Sessions;
- nodo derivato;
- campo già previsto dal repository.

NON creare un nuovo file se una struttura esistente può rappresentarlo.

Il requisito è:
> una richiesta di QA non deve esistere solo in prosa.

---

# 15. Human Gates da preservare

## H1 — Map / Authoring

Dopo U21 → U22 / U25 verificare:
- illuminazione;
- framing;
- ghost;
- snap;
- undo;
- `.umap` pulito;
- scala cella;
- scala unità;
- cover leggibile;
- porte/stati leggibili;
- acqua/ghiaccio/terreno distinguibili.

Il verdetto è umano.

## H2 — Reactions

Dopo #886 + #166:
- Overwatch armato;
- FIRE;
- HOLD;
- timeout;
- countdown 3 s;
- slow motion solo presentation;
- movement truncation;
- invalidazione KO/Stun/Disarm/forced move;
- TurnLog spiega il risultato;
- verifier replay invariato.

## H3 — Showcase

Quando gli owner reali di T5/T6/T8 sono chiusi:
- T1→T8 raggiungibili;
- ogni turno realmente eseguito;
- ogni evento chiave asserito;
- porta/interazione;
- interposition;
- combo ambientale;
- objective;
- KO;
- nessun turno “verde perché vuoto”.

## H4 — Release

- U16 KPI tecnici;
- U19 pacing/game feel;
- U17 packaged.

Verificare:
- 60 FPS target;
- path median < 2 ms;
- preview < 50 ms;
- resolver < 100 ms/turno;
- replay divergence = 0;
- durata/ritmo;
- Development packaged;
- Shipping packaged;
- partita conclusa fuori dall'Editor.

---

# 16. Issue / task da creare o aggiornare se mancanti

Non duplicare issue esistenti.

Prima cercare GitHub.

Se non esistono owner adeguati, creare task piccoli per:

1. **Scenario metadata strutturato**
   - PrimaryCategory + Character/Faction/Milestone/Purpose;
   - nessuna duplicazione scenario;
   - classification-only metadata non cambia hash.

2. **RTGameMode scenario config extraction**
   - solo dopo baseline v0.1 o quando una feature reale lo richiede.

3. **Strangler refactor RTTurnManager**
   - issue per singolo dominio, non “refactor TurnManager”.

4. **ScenarioSession responsibility extraction**
   - solo quando una modifica reale dimostra un blocco.

5. **Preflight locale**
   - collegare/assorbire #950 se quello è l'owner corretto.

6. **Naming gate su current sources**
   - solo dopo aver definito quali YAML sono normativi e quali storici.

7. **QA handoff strutturato**
   - usare EditorMap/Execution Graph esistenti se possibile.

---

# 17. Test architecture

Ogni miglioramento strutturale deve avere un oracolo.

## Refactor senza cambio semantico

Prima e dopo:

```text
same scenario
same seed
same intents
same reaction decisions
→ same StateHash
→ same TurnLog canonical hash
```

Se il progetto dispone già di golden:
- non rigenerarlo per far passare un refactor;
- una rigenerazione richiede una ragione gameplay esplicita.

## Test di ordine

Qualunque `TMap`/`TSet` iterata in output logico deve:
- essere ordinata;
- oppure avere un test di permutazione che dimostri indipendenza.

## Scenario metadata

Modificare solo metadata:
- stesso scenario;
- stesso seed;
- stesso StateHash;
- stesso TurnLog hash.

## Packaged

Per modifiche a:
- Build.cs;
- scenario staging;
- command line;
- runtime/editor boundary;

eseguire almeno:
- Editor Development build;
- Game Development;
- Game Shipping;
- scenario smoke packaged quando pertinente.

---

# 18. Debug / logging

Mantenere log per responsabilità.

Prefissi consigliati solo se coerenti con log category esistente:

```text
[RTScenario]
[RTReplay]
[RTReaction]
[RTIntegration]
```

Non introdurre log spam per-frame.

Per mismatch replay/reaction loggare:
- TurnNumber;
- MacroPhase;
- MicroStep;
- OpportunityId;
- expected recorded response;
- actual legal responses;
- reason code.

Non loggare informazioni di planning avversario su client non autorizzati.

---

# 19. Sequenza di implementazione consigliata

## Fase A — fotografia

Claude deve produrre prima un breve audit:

```text
origin/main SHA
issue live
track owner
write-set
blocked/release requests
open PR che toccano gli stessi path
build status disponibile
```

Nessuna modifica prima di questo punto.

## Fase B — P0

Chiudere o sbloccare #886.

Se i path necessari non sono ottenibili:
- NON aggirare D-139;
- aggiornare release request;
- lavorare solo sui file già posseduti;
- riportare esattamente il blocker.

## Fase C — quality infrastructure

Se il write-set lo consente:
- implementare/chiudere il gate di #950;
- creare preflight unico come orchestratore.

Non bloccare #886 per questo lavoro.

## Fase D — parallel work

Avviare contemporaneamente, se write-set disgiunti:

```text
A: replay/reaction
B: map interaction
C: scenario/assertions
D: editor/human
```

BASE integra.

## Fase E — refactor opportunistico

Quando una delle issue tocca:
- TurnManager;
- ScenarioSession;
- GameMode;

applicare la regola “orchestrator vs domain logic”.

Estrarre solo ciò che il diff dimostra.

## Fase F — content/data

Portare avanti U10/U11 o owner correnti equivalenti prima del content freeze.

---

# 20. Definition of Done di questo mandato

Il lavoro di miglioramento è considerato applicato quando:

- [ ] lo stato live è stato auditato contro `main`, non ereditato da questo file;
- [ ] #886 ha un percorso di chiusura reale e non è più un rischio nascosto per #166;
- [ ] il batch usa massimo tre stream code/scenario attivi + una lane human + BASE, salvo motivazione esplicita;
- [ ] nessuna nuova feature mette altra gameplay logic dentro ScenarioHarness;
- [ ] qualunque nuovo codice in TurnManager è stato valutato per estrazione verso libreria/resolver;
- [ ] nessun mega-refactor è stato introdotto;
- [ ] il preflight locale ha un owner e, se implementato, compone gate reali;
- [ ] le fonti di stato non vengono duplicate in nuovi documenti;
- [ ] i nuovi handoff QA non restano solo in prosa;
- [ ] eventuali cleanup Runtime/Editor sono separati dalle issue gameplay;
- [ ] U10/U11 o equivalenti hanno un piano reale prima del content freeze;
- [ ] suite automatica rilevante verde;
- [ ] build Development Editor verde;
- [ ] Game Development/Shipping eseguite quando il diff tocca build/runtime boundary;
- [ ] generated views sono rigenerate solo dalle rispettive sorgenti;
- [ ] nessun binary asset è stato modificato senza lease/atto autorizzato;
- [ ] `git status` finale pulito;
- [ ] report finale elenca cosa è stato **fatto**, cosa è stato **deferito**, cosa è **bloccato** e da chi.

---

# 21. Report finale obbligatorio di Claude

Usare questa struttura:

## Audit iniziale

| Campo | Valore |
|---|---|
| `origin/main` | SHA |
| UE | versione |
| worktree | path |
| branch | nome |
| track | nome |
| issue | numero |
| write-set | path |
| blockers | PR/issue/path |

## Modifiche applicate

| Area | Problema | Soluzione | File |
|---|---|---|---|

## Cambiamenti architetturali

Per ogni estrazione:

```text
prima
→ responsabilità mista

dopo
→ orchestrator
→ domain service/library
```

Spiegare perché il confine è stato estratto **ora**.

## Test

Elencare comandi reali ed esiti reali.

Non scrivere “pass” se non eseguito.

## Determinismo

Riportare:
- scenario;
- seed;
- StateHash;
- TurnLog hash;
- confronto prima/dopo quando applicabile.

## Build

```text
Editor Development:
Game Development:
Game Shipping:
Packaged smoke:
```

Scrivere `NOT RUN` quando non eseguito.

## Parallelism / ownership

- write-set usato;
- lease;
- release request;
- file integration_only;
- collisioni evitate.

## Debito rimasto

Separare:
- `BLOCKED`;
- `DEFERRED`;
- `OUT OF SCOPE`;
- `NEXT`.

---

# 22. Commit Git suggeriti

Adattare al lavoro realmente fatto.

Esempi:

```text
fix(replay): consume recorded reaction decisions during resimulation
test(replay): cover reaction trace precedence and orphan decisions

chore(qa): add local preflight orchestration
test(qa): enforce automation test guards

refactor(turn): extract reaction rule from turn orchestration
refactor(scenario): isolate scenario assertion evaluation

docs(architecture): record incremental hotspot reduction plan
```

Non creare commit vuoti o separazioni artificiali.

---

# 23. Principio finale

Il progetto non richiede una riscrittura.

Richiede di evitare che tre hotspot crescano oltre il punto di controllo:

```text
RTTurnManager
RTScenarioSession
parallel-batch.yaml
```

La strategia è:

```text
feature reale
    ↓
misura il confine
    ↓
estrai solo la responsabilità necessaria
    ↓
testa determinismo
    ↓
integra
```

e non:

```text
"l'architettura potrebbe essere più pulita"
    ↓
mega-refactor
```

La v0.1 deve continuare ad avanzare mentre l'architettura migliora.
