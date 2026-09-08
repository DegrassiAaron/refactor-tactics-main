# RefactorTactics — CLAUDE.md

Overlay operativo **Claude Code** per RefactorTactics.

Questo file non sostituisce:

* `AGENTS.md`;
* Decision Log / ADR;
* owner specification;
* GitHub issue e milestone;
* test eseguibili.

`AGENTS.md` possiede i guardrail tool-agnostic.

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

# 2. Avvio sessione

All'avvio della sessione:

1. leggi `AGENTS.md`;
2. determina dal repository e da GitHub lo stato reale: branch, `HEAD`, `git status`, issue e milestone correnti.

> ⚠️ **Non esiste più un ruolo di sessione da dichiarare.** Fino al 2026-09-08 una sessione assumeva una figura fra `DEV`, `EDITOR` e `VALIDATION`, la dichiarava in `RT_TERMINAL_ROLE`, e gli script di `scripts/` la facevano rispettare. Ruoli e script sono stati rimossi ([`D-346`](docs/decisions/RT_PDR_00_Decision_Log.md) e [`D-347`](docs/decisions/RT_PDR_00_Decision_Log.md)). Se trovi `RT_TERMINAL_*` o `RT_WORKSPACE_*` nell'ambiente, sono residui di una finestra aperta prima: non significano nulla, e nessuno script li legge più.

Quello che i ruoli separavano resta separato **dai fatti**, non da un guard:

* Unreal è **uno per macchina**. Prima di aprire l'Editor, lanciare PIE, compilare o misurare, accertati che nessun'altra sessione lo stia usando: due job insieme si invalidano le misure a vicenda;
* i checkout **non isolano** le risorse di macchina. Working tree e `HEAD` sì, Unreal e Live Coding no;
* l'**authoring asset** appartiene al clone principale — un worktree non ha i file gitignorati, e salvare un asset i cui riferimenti duri leggono `None` **li azzera**, senza errore;
* chi scrive una correzione **non ne firma da solo il verdetto** sui sistemi che tocca: la misura vale se avviene dopo, su un commit dichiarato.

Nessuno di questi punti è più verificato da uno script. Sono a carico di chi lavora.

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

Una misura preliminare può produrre evidenza utile, ma non equivale al sign-off finale.

Chi verifica non deve, nello stesso passaggio:

1. modificare un problema;
2. validare autonomamente il proprio fix;
3. dichiararlo approvato.

Se una verifica trova un difetto che richiede modifica, la correzione e la sua misura sono **due momenti**: si corregge, si dichiara il commit, e si rimisura su quello. Non è una regola di ruolo — i ruoli non esistono più — ma di indipendenza della misura.

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
* artifact/evidence condivisi.

Non coordinare tramite copie locali non tracciate come source of truth.

Preferisci commit logicamente isolati:

`<type>(<scope>): <description>`

Non fare push o modifiche GitHub distruttive salvo autorizzazione della sessione.

---

## Handoff minimo

Ogni handoff significativo deve identificare almeno:

* branch;
* commit SHA / build;
* scope;
* issue / milestone;
* file o asset rilevanti;
* gate già eseguiti;
* gate `NOT RUN`;
* failure note;
* istruzione successiva.

Un handoff che non identifica questi elementi non e' un handoff: e' un messaggio.

---

# 10. Unreal, asset e risorse di macchina

I ruoli operativi sono stati rimossi ([`D-347`](docs/decisions/RT_PDR_00_Decision_Log.md)). I vincoli che li avevano motivati no: erano fatti della macchina, non convenzioni.

## Il motore è uno

Unreal è **uno** e lo condividono tutti i checkout. Non c'è più un lease che serializzi gli accessi, quindi:

* prima di aprire l'Editor, lanciare PIE, compilare o misurare, **verifica che nessun altro lo stia usando** — `Get-Process UnrealEditor*, UnrealEditor-Cmd*`;
* una build lanciata mentre un altro checkout misura riscrive il binario sotto quella misura e la rende `NON VALIDA`;
* un worktree separato **non** elimina il mutex globale di Unreal e Live Coding: due misure non diventano parallele, diventano una coda.

## Authoring asset: il clone principale

Una chiamata che crea, modifica, rinomina, sposta, cancella, importa o salva un asset Unreal via MCP appartiene al **clone principale**, quello che ospita il bridge.

Due ragioni, entrambe tecniche:

* il bridge MCP è **uno solo**. Usarlo da un altro checkout muta gli asset del principale mentre si legge il `git status` del proprio;
* un worktree **non ha i file gitignorati**. I riferimenti duri di un asset vi leggono `None`, e salvarlo **li azzera** — senza errore, e ce ne si accorge dopo.

Preparazione, ispezione e query read-only non hanno questo vincolo.

⛔ **Nessuno script lo verifica più.** Il preflight che esisteva prima autorizzava e non intercettava: il trasporto MCP è HTTP diretto, e chi lo saltava raggiungeva il bridge lo stesso. Ora non c'è nemmeno l'autorizzazione — resta solo la verifica che fai tu.

Misurato il 2026-09-06: dietro `call_tool` ci sono **56 toolset**, di cui 55 non sono di RefactorTactics. Fra questi `AssetTools` (`write_file`, `delete`, `move`), `AutomationTestToolset` (`RunTests`, `StopTests`) e `ProgrammaticToolset`, che esegue Python. Una chiamata MCP può quindi avviare o fermare una suite, e rendere `NON VALIDA` la misura di un'altra sessione.

## Chi ripara non firma

Non è più una regola di ruolo, ma resta una regola di indipendenza della misura:

* chi ha scritto una correzione non ne emette da solo il verdetto sui sistemi che quella correzione tocca — determinismo, privacy, autorità, replay;
* un difetto trovato durante una verifica torna a chi possiede il codice, con la sua evidenza, e si rimisura su un commit successivo.

Vale anche quando la stessa persona fa entrambe le cose: ciò che conta non è chi digita, è che la misura avvenga **dopo** e su un artefatto dichiarato.

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
