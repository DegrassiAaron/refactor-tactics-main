# RefactorTactics — CLAUDE.md

Overlay operativo **Claude Code** per RefactorTactics.

Questo file non sostituisce:

* `AGENTS.md`;
* `docs/rt-three-terminals/prompts/RT3_CONTRACT.md`;
* Decision Log / ADR;
* owner specification;
* GitHub issue e milestone;
* test eseguibili.

`AGENTS.md` possiede i guardrail tool-agnostic.

`RT3_CONTRACT.md` possiede il contratto operativo RT3.

Questo file definisce esclusivamente come **Claude Code** applica tali regole durante una sessione.

---

# 1. Principio operativo

RefactorTactics è un tactical game competitivo Unreal Engine 5 basato su:

* turni simultanei;
* simulazione autoritativa;
* determinismo;
* tactical graph multilivello;
* planning nemico privato;
* coordinazione alleata;
* contenuti data-driven;
* separazione fra simulazione e presentazione.

Non ricostruire l'architettura da zero a ogni task.

Prima di lavoro sostanziale determina sempre dal repository e da GitHub:

* release corrente;
* milestone corrente;
* issue / Epic coinvolti;
* Domain owner;
* stato reale dell'implementazione.

Non dedurre il current scope da questo file.

---

# 2. Avvio sessione e ruolo RT3

All'avvio della sessione:

1. leggi `AGENTS.md`;
2. individua le variabili `RT_TERMINAL_*` e `RT_WORKSPACE_*`, se presenti;
3. esegui `rtstatus` quando disponibile;
4. leggi `docs/rt-three-terminals/prompts/RT3_CONTRACT.md`;
5. carica **un solo** `TERMINAL_*.md` coerente con il ruolo;
6. carica al massimo **un solo** `WAVE_*.md` compatibile con la sessione corrente.

Le variabili sono:

```text
RT_TERMINAL_ROLE       DEV | EDITOR | VALIDATION
RT_TERMINAL_INSTANCE   identificativo dell'istanza
RT_WORKSPACE_ROOT      checkout su cui la sessione lavora
RT_WORKSPACE_ID        MAIN | DEV | TECHNICAL_DESIGNER
RT_TASK_ID             task/issue, quando dichiarato
```

`RT_WORKSPACE_ID` non è il ruolo della sessione e non è il branch.

È l'identità del **workspace**: `MAIN` è il checkout che ospita l'unico bridge MCP della macchina.

`MAIN` non è il branch `main`.

Il valore autorevole vive nel registro per macchina, non nella variabile e non nel marker locale.

Per verificarlo:

```powershell
rtws -Action verify
```

Le figure canoniche sono:

* `DEV`;
* `EDITOR`;
* `VALIDATION`.

`DEV-LEAD`, `DEV-MAIN` e `DEV-TEST` sono funzioni DEV di una wave, non nuove figure.

Non dedurre il ruolo da:

* nome della directory;
* branch;
* terminal title;
* repository clone.

Se il ruolo non è determinabile:

`ROLE_MISSING`

Se più fonti assegnano ruoli incompatibili:

`ROLE_CONFLICT`

In entrambi i casi opera **fail-closed**: non iniziare lavoro mutante finché il ruolo non è risolto.

Una sessione Claude occupa una sola figura RT3.

---

## Task routing

Se `RT_TASK_ID` è presente, il ruolo da solo non basta: qualcuno ha deciso **quale** lavoro tocca a questa sessione.

Dopo aver risolto il ruolo:

1. chiama il router in **sola lettura**:

```powershell
rttask status -TaskId $env:RT_TASK_ID
rttask assignment -TaskId $env:RT_TASK_ID
```

2. confronta `RT_TERMINAL_ROLE` con `next_actor`;
3. se non corrispondono, **fermati**;
4. lavora solo sull'assignment corrente.

Errori, tutti fail-closed:

```text
TASK_NOT_FOUND            il task non esiste su questa macchina
TASK_ROUTE_MISMATCH       questo ruolo non è l'actor atteso
TASK_ASSIGNMENT_MISSING   nessuna consegna emessa
TASK_ALREADY_DONE         il task è chiuso
```

Non correggere il routing per proseguire: le mutazioni (`init`, `assign`, `close`) appartengono al **RT Coordinator**, e il router le rifiuta da una sessione con ruolo.

A fine lavoro deposita il risultato e torna al Coordinator:

```powershell
rttask report -TaskId <id> -Status <DONE|PARTIAL|BLOCKED|FAILED> -Summary "..." -Evidence "..."
```

⛔ `NEXT_ACTOR_RECOMMENDED` è una raccomandazione, non una decisione di routing.

Il task routing è un **quarto** concetto, distinto da ruolo di sessione, identità del workspace e lease del motore. Semantica completa: [`docs/rt-three-terminals/TASK_ROUTING.md`](docs/rt-three-terminals/TASK_ROUTING.md).

---

# 3. Autorità e source of truth

Mantieni distinti tre tipi di autorità.

## Stato live

Per sapere cosa esiste, cosa è aperto, chiuso o merged:

1. repository corrente;
2. GitHub milestone;
3. GitHub issue / PR;
4. build e test eseguibili.

Drive non possiede lo stato live delle issue.

## Contratto

Per sapere quale comportamento è corretto:

1. Decision Log accettato;
2. ADR accettato;
3. owner specification;
4. contract test intenzionale;
5. Domain Roadmap corrente;
6. documentazione storica.

Il codice corrente è evidenza dell'implementazione, non automaticamente del contratto corretto.

## Ownership

Per sapere chi possiede una responsabilità:

1. owner GitHub / Epic corrente;
2. Domain Roadmap canonica;
3. decisione cross-domain accettata.

Una issue ha un solo primary owner.

Le altre relazioni sono dipendenze.

Quando le fonti divergono non riconciliarle silenziosamente.

Usa:

* `STALE ROADMAP`;
* `IMPLEMENTATION DRIFT`;
* `CONTRACT CONFLICT`.

Descrivi sempre quale fonte governa la decisione corrente.

---

# 4. Search → Reuse → Create

Prima di creare una nuova responsabilità cerca ciò che esiste già.

Vale in particolare per:

* classi;
* subsystem;
* Actor;
* componenti;
* USTRUCT;
* enum;
* Data Asset;
* Gameplay Tag;
* resolver;
* validator;
* test;
* utility;
* debug command;
* Editor tool.

Ordine:

`SEARCH → REUSE → EXTEND → REFACTOR → CREATE`

Non creare architetture parallele.

Non introdurre placeholder per roadmap lontane.

Non fare refactor opportunistici.

I miglioramenti non necessari al task corrente vanno in:

`FOLLOW-UP CANDIDATES`

---

# 5. Comportamento Claude per Unreal e asset

Usa la versione Unreal Engine fissata dal repository.

Non aggiornare senza richiesta esplicita:

* Unreal Engine;
* plugin;
* toolchain;
* dipendenze principali.

Non inventare API Unreal.

Quando un'API è incerta, verificarla tramite:

* header disponibili;
* versione Engine;
* uso già presente nel repository.

Per file binari Unreal:

* non modificarli come testo;
* non simulare modifiche `.uasset` / `.umap`;
* usa EDITOR quando il cambiamento richiede realmente Unreal Editor;
* non dichiarare una verifica visiva senza averla eseguita.

C++ possiede normalmente:

* simulazione;
* networking;
* validazione;
* serialization;
* snapshot;
* resolver;
* pathfinding;
* replay;
* TurnLog;
* regole competitive.

Blueprint / UMG / asset possiedono principalmente:

* configurazione;
* UI;
* presentation;
* animation;
* VFX;
* content variation.

Regola:

**C++ definisce ciò che è permesso.
Data e Blueprint configurano una variante permessa.**

GAS non deve diventare una seconda autorità della simultaneous resolution.

---

# 6. Validità dei test e delle evidenze

Un risultato è valido solo se il gate corrispondente è stato realmente eseguito sul codice / asset / commit dichiarato.

Usa questi stati:

* `PASS`
* `FAIL`
* `NOT RUN`
* `N/A`

`NOT RUN` non equivale a `PASS`.

Non dichiarare mai:

* compile PASS;
* Automation PASS;
* determinism PASS;
* replay PASS;
* privacy PASS;
* PIE PASS;
* packaged PASS;
* performance PASS;

senza evidenza reale.

Regola generale:

**Automation** verifica regole e contratti.

**PIE** verifica interaction e presentation.

**Packaged** verifica ciò che viene realmente distribuito.

Uno non sostituisce automaticamente gli altri.

Una Validation Window preliminare può produrre evidenza utile, ma non equivale al sign-off finale previsto dal contratto RT3.

VALIDATION non deve:

1. modificare un problema;
2. validare autonomamente il proprio fix;
3. dichiararlo approvato.

Se VALIDATION trova un difetto che richiede modifica, produce handoff al ruolo appropriato e poi rivalida una build/commit indipendente.

---

# 7. Determinismo, autorità e privacy

Questi sono invarianti competitivi.

## Determinismo

A parità di:

* initial canonical state;
* snapshot;
* rules/config version;
* resolver version;
* seed, quando previsto;

devono risultare uguali:

* final state;
* ordered gameplay events;
* TurnLog;
* digest/hash quando definito.

Non dipendere da:

* frame rate;
* Actor iteration order;
* `TMap` / `TSet` order;
* pointer address;
* animation timing;
* wall-clock;
* async completion order non normalizzato.

Usa Stable ID e ordering esplicito.

Quando cambia lo stato canonico, considera sempre:

* snapshot schema;
* serialization;
* replay;
* TurnLog;
* hash/digest;
* versioning.

## Autorità

Invariante:

`CLIENT PROPOSES → SERVER VALIDATES → SERVER APPLIES`

Il client non decide esiti competitivi.

## Privacy

Gli intenti completi appartengono all'autorità.

Un client avversario non deve ricevere dati sufficienti per ricostruire hidden enemy planning.

Non collocare planning privato su Actor globalmente replicati.

Qualunque modifica a:

* planning;
* replication;
* networking;
* event projection;
* ally/enemy UI;

richiede revisione esplicita del boundary privacy.

---

# 8. Scope, task e milestone

La dimensione del task determina il processo.

## Local task

Esempi:

* compile fix;
* bug locale;
* test ristretto;
* config/documentation fix.

Flusso:

`inspect → change → relevant verification → report`

## Feature task

Flusso:

`preflight → search → plan → implement → compile → tests → relevant gates → report`

## Milestone task

Flusso:

`reconnaissance → dependency check → scoped implementation → verification → milestone report`

Non trasformare silenziosamente un local task in una milestone.

Non implementare automaticamente lavoro futuro emerso durante l'analisi.

Classifica il lavoro non corrente come:

* `CURRENT REQUIRED`;
* `CURRENT OPTIONAL`;
* `DEFERRED`.

---

## Milestone design

I 14 Domain Roadmaps definiscono ownership, non ordine di implementazione.

Non assumere:

`M1 = Domain 01`
`M2 = Domain 02`
ecc.

Preferisci milestone verticali verificabili.

Esempio concettuale:

`Intent → Validation → Snapshot → Resolution → Result → Test`

Una milestone può attraversare più domini, ma mantiene:

* un goal;
* un primary owner;
* acceptance criteria binari;
* scope delimitato.

Completa e verifica la milestone richiesta.

**Non iniziare automaticamente la milestone successiva**, salvo istruzione esplicita della sessione/wave.

---

# 9. Lavoro parallelo, Git e handoff

Più terminali nella stessa directory condividono lo stesso working tree.

Non considerarli isolamento.

Working tree separati possono avere filesystem Git distinti, ma condividono comunque risorse macchina e tool esterni.

Prima di lavoro sostanziale registra:

* ruolo RT3;
* workspace;
* branch;
* HEAD;
* `git status`;
* modifiche preesistenti;
* issue / milestone corrente.

Le modifiche preesistenti dell'utente o di altre sessioni sono protette.

Non:

* reset;
* discard;
* checkout distruttivo;
* stash non necessario;
* rebase di history condivisa;
* force-push;
* sovrascrittura di lavoro altrui.

Il coordinamento avviene tramite:

* branch;
* commit SHA;
* handoff persistiti;
* issue / PR;
* artifact/evidence condivisi secondo RT3.

Non coordinare tramite copie locali non tracciate come source of truth.

Preferisci commit logicamente isolati:

`<type>(<scope>): <description>`

Non fare push o modifiche GitHub distruttive salvo autorizzazione della sessione.

---

## Handoff minimo

Ogni handoff significativo deve identificare almeno:

* ruolo sorgente;
* ruolo destinazione;
* branch;
* commit SHA / build;
* scope;
* issue / milestone;
* file o asset rilevanti;
* gate già eseguiti;
* gate `NOT RUN`;
* failure note;
* istruzione successiva.

Usa le forme complete definite da `RT3_CONTRACT.md`; non duplicarne qui lo schema.

---

# 10. Routing rapido Claude

## DEV

Usa DEV per:

* C++;
* simulator;
* resolver;
* networking;
* pathfinding;
* serialization;
* replay;
* TurnLog;
* Automation;
* script/tooling non Editor-bound;
* documentazione tecnica quando non richiede asset verification.

DEV non deve dichiarare verifiche Editor-only che non ha realmente eseguito.

---

## EDITOR

Usa EDITOR per:

* `.umap`;
* `.uasset`;
* Blueprint;
* UMG;
* animation;
* visual setup;
* asset integration;
* Editor authoring;
* PIE;
* visual evidence.

EDITOR consuma i contratti gameplay.

Non sostituisce VALIDATION per:

* determinismo;
* privacy;
* replay correctness;
* authoritative logic.

### Authoring asset via MCP

Il ruolo EDITOR esiste in ogni workspace.

L'authoring asset via MCP no: è consentito **solo** dal workspace `MAIN`.

Il bridge MCP è uno solo e vive in MAIN. Usarlo da un altro checkout muta gli asset di MAIN mentre si legge il `git status` del proprio.

Condizioni, tutte necessarie:

```text
RT_TERMINAL_ROLE == EDITOR
RT_WORKSPACE_ID  == MAIN, verificato sul registro di macchina
branch           == branch di task, diverso da main
RT_TASK_ID       presente
write-set asset  dichiarato
lease Unreal     vivo, posseduto, per l'operazione giusta
```

Preflight:

```powershell
rtmcp -Operation MCP_ASSET_WRITE -TaskId <id> -AssetWriteSet <path>
```

Fuori da MAIN restano consentite preparazione, ispezione e query read-only.

Il motore si prende just-in-time:

```powershell
rtlease -Action acquire -Operation EDITOR -TaskId <id>
rtlease -Action release
```

Aprire un terminale non acquisisce Unreal.

⛔ Il preflight **autorizza, non intercetta**.

Il trasporto MCP è HTTP diretto: chi lo salta raggiunge il bridge lo stesso, e nessuno script può impedirlo.

Misurato il 2026-09-06: dietro `call_tool` ci sono **56 toolset**, di cui 55 non sono di RefactorTactics. Fra questi `AssetTools` (`write_file`, `delete`, `move`), `AutomationTestToolset` (`RunTests`, `StopTests`) e `ProgrammaticToolset` (esegue Python).

Conseguenza: una chiamata MCP può avviare o fermare una suite senza passare da `rt-suite.ps1`, dal lease e dal mutex — cioè può rendere `NON VALIDA` la misura di un'altra sessione.

Dettaglio: `docs/rt-three-terminals/prompts/RT3_CONTRACT.md` §14.

---

## VALIDATION

Usa VALIDATION per verifiche indipendenti come:

* Automation gate;
* replay regression;
* determinism;
* privacy;
* packaged build;
* scenario/golden test;
* performance;
* release evidence.

VALIDATION non implementa e approva autonomamente la stessa correzione.

Quando Unreal è una risorsa esclusiva, EDITOR e VALIDATION rispettano la mutua esclusione definita da RT3.

La catena canonica resta:

`DEV-LEAD → EDITOR → VALIDATION`

salvo Validation Window o routing esplicitamente consentiti da `RT3_CONTRACT.md`.

---

# 11. Regole permanenti di gameplay engineering

Queste regole sono abbastanza stabili da guidare Claude, ma i dettagli appartengono alle owner specification.

## Logical vs Presentation

Mantieni separati:

`CANONICAL / LOGICAL STATE`

e:

`PRESENTATION STATE`.

Animation, VFX, UI e audio consumano eventi della simulazione.

Non decidono:

* movement;
* hit;
* damage;
* reaction;
* KO;
* objective outcome.

---

## Turn architecture

Target:

`Planning`
→ `Ready`
→ `Commit`
→ `Validation`
→ `Immutable Snapshot`
→ `Deterministic Resolution`
→ `Cleanup`
→ `TurnLog / Result`

Non creare shortcut incompatibili con questo modello.

---

## Tactical map

`FRTCellId` identifica la cella logica tramite:

* `X`;
* `Y`;
* `Layer`.

Il tactical graph è l'autorità per movimento competitivo.

NavMesh/Recast non lo è.

Separare:

* pathfinding;
* LOS;
* targeting;
* projectile trajectory.

Le celle logiche sono dati compatti centralizzati, non migliaia di Actor.

---

## Cover / intra-hex

Salvo decisione successiva accettata:

* wedge = tactical geometry, non subcell;
* `CoverOption` non crea occupancy aggiuntiva;
* ogni `FRTCellId` ha un solo authoritative occupancy slot;
* wall side non implica automaticamente traversal;
* same-cell contest resta deterministico;
* cover selection non ridefinisce canonical occupancy.

Un conflitto con queste regole richiede contract investigation.

---

## Bots

I bot generano intent.

Usano gli stessi:

* legal-action rules;
* canonical state;
* simulator;
* resolution rules;

dei player.

Non duplicare gameplay competitivo nei bot.

---

## Editor tooling

Editor tooling produce o consuma gli stessi canonical data contract del runtime.

Non duplicare gameplay authority dentro tool Editor.

---

# 12. Domain e capability routing

Canonical Domain ownership:

1. Gameplay & Match Flow
2. Tactical Map, Graybox & Level Kit
3. Technical Design & Editor Tooling
4. Characters, Abilities & Combat
5. Environment Systems & Gameplay Effects
6. Graphics, Rendering, Materials & VFX
7. Animation & Presentation
8. UI, UX & Coordination
9. Assets & Content Pipeline
10. Audio & Feedback
11. AI, Bots & Autobattle
12. Networking, Privacy & Multiplayer
13. Data, Balance & Progression
14. QA, Automation, Packaging & Performance

Le capability trasversali non sono owner alternativi.

Fra esse:

* Core Simulation & Turn System;
* Tactical Map & Navigation;
* Combat & Abilities;
* Environment & Elemental Systems;
* Multiplayer & Networking;
* Planning UX & HUD;
* Camera & Map Presentation;
* Animation, VFX & Presentation;
* Tools, QA & Production.

Linka le dipendenze.

Non duplicare issue o responsabilità.

---

# 13. Decision conflict

Se due implementazioni possibili cambiano semanticamente il gameplay, non scegliere arbitrariamente.

Cerca:

1. Decision Log;
2. ADR;
3. owner specification;
4. contract test;
5. issue / PR discussion;
6. current implementation.

Se il conflitto resta reale:

`BLOCKED — DECISION REQUIRED`

Riporta:

* interpretazioni;
* conseguenze;
* sistemi coinvolti;
* decisione minima richiesta.

Continua solo lavoro indipendente.

---

# 14. Definition of Done

Non assumere:

`COMPILES = DONE`

né:

`ISSUE CLOSED = DONE`

né:

`PR MERGED = DONE`.

Dove rilevante, una feature richiede:

* comportamento corretto;
* corretta authority;
* determinismo;
* privacy;
* eventi/TurnLog coerenti;
* replay/serialization compatibility;
* version/hash correctness;
* test automatici;
* PIE evidence;
* packaged evidence;
* performance appropriata.

Applica solo i gate rilevanti al task e alla release corrente.

---

# 15. Reporting Claude

Per un normale feature task riporta almeno:

## Implemented

* ...

## Files

* ...

## Verification

* Compile: `PASS / FAIL / NOT RUN / N/A`
* Tests: `PASS / FAIL / NOT RUN / N/A`
* Determinism: `PASS / FAIL / NOT RUN / N/A`
* Replay: `PASS / FAIL / NOT RUN / N/A`
* Privacy: `PASS / FAIL / NOT RUN / N/A`
* PIE: `PASS / FAIL / NOT RUN / N/A`
* Packaged: `PASS / FAIL / NOT RUN / N/A`

## Known limitations

* ...

## Follow-up candidates

* ...

Per una milestone aggiungi:

* goal;
* primary Domain;
* supporting Domains;
* issue;
* acceptance criteria;
* blocker;
* recommended commit.

Non fabbricare evidenza.

---

# 16. Priorità

Quando devi scegliere fra compromessi, usa questo ordine:

1. correctness;
2. determinism;
3. authority / privacy;
4. testability;
5. maintainability;
6. player-facing clarity;
7. implementation speed.

L'obiettivo non è produrre più codice.

L'obiettivo è chiudere lavoro verificabile senza introdurre una seconda authority, scope creep o debito nascosto.
