# RefactorTactics — CRUD Epic / Issue + GitHub MCP + Unreal MCP

> `HISTORICAL` · **Kit d'autore consumato**, non una fonte. · **Consumato**: 2026-08-30 · **Base**: `20d59973`
> (`origin/main` `417ecfb5`), branch `diag/1665-istanze-board`
>
> Archiviato da [`docs/archive/`](../../README.md): vale per la **provenienza** e il rationale, mai per la
> regola. Il file stava alla radice del repository come `Claude_RefactorTactics_v0.1_CRUD_Epic_Issue_MCP.md`
> (**1463** righe) — **senza data**, né nel nome né in testa: l'unico dei kit archiviati a non averne una.
>
> **Cosa possiede**: il mandato d'autore e le sue ventotto sezioni, verbatim.
> **Cosa non possiede**: nessuna autorità. Del mandato è stato eseguito **solo `R0`**, su conferma
> d'autore: una mutazione, il corpo di `#14`. Zero issue create, chiuse o riaperte. Il
> referto completo — misura per misura — è
> [`../../../roadmap/plans/crud-epic-issue-mcp-spec-panel-2026-08-30.md`](../../../roadmap/plans/crud-epic-issue-mcp-spec-panel-2026-08-30.md).
>
> **Il verdetto in tre righe.** ✅ Accurato su ciò che cita: **29/29** ancore GitHub esistono coi titoli
> giusti, i **10** percorsi del preflight ci sono, la baseline `G1–G14` è corretta (`G15` è ritirato da
> `D-181`), e «*il parent `#14` può essere indietro*» è vero — quattro epic v0.1 aperte non vi compaiono.
> 🔴 Il **§13** prescrive scritture che il ponte MCP non ha: `RTDevToolset` espone **5** tool `AICallable`,
> tutti di sola lettura, quindi §13, §22 (ultima riga) e §23 non sono eseguibili. 🔴 È il **terzo giro** sullo
> stesso terreno e non nomina i due gemelli consumati il **2026-08-28**, né `E48` (`#1408`), che è l'owner
> più recente del suo `R2`.

---

# CLAUDE CODE — REFACTORTACTICS
# v0.1 CRUD ROADMAP — EPIC / ISSUE + GITHUB MCP + UNREAL MCP

## SCOPO

Lavora direttamente su `DegrassiAaron/refactor-tactics-main` come Technical Product Owner, Unreal Engine Developer e maintainer GitHub, usando **GitHub MCP** e **Unreal MCP / Unreal Editor** quando disponibili.

L'obiettivo è ripulire, riallineare e rendere eseguibile la **roadmap v0.1 attraverso Epic e Issue GitHub**, senza creare una seconda tassonomia.

Questo è un mandato CRUD-like:

```text
C = CREATE
R = READ / SEARCH / INSPECT
U = UPDATE
D = DEACTIVATE
    → close as completed / duplicate / not_planned
    → MAI cancellare la storia
REOPEN = riaprire solo quando il DoD originale è falso su main
```

---

# 1. RISULTATO FINALE

La roadmap v0.1 deve essere leggibile come:

```text
CURRENT STATE
    ↓
P0 COMPLETE MATCH
    ↓
PLAYER UNDERSTANDS THE PLAN
    ↓
PLAYER UNDERSTANDS THE RESULT
    ↓
PRESENTATION / EDITOR WORK
    ↓
GOLDEN SHOWCASE
    ↓
QA + PACKAGED
    ↓
v0.1 DONE
```

La v0.1 resta:

```text
2v2
offline vs bot
hex multilivello
4 eroi
turni simultanei
planning
resolution deterministica
objective
TurnLog
presentazione leggibile
build packaged
```

Fuori scope del critical path:

```text
networking
dedicated server
GAS
progressione
modding pubblico
ranked
matchmaking
4v4 come formato di prodotto
```

---

# 2. REGOLA NUMERO UNO

```text
SEARCH BEFORE CREATE
```

Prima di creare QUALUNQUE Epic o Issue:

1. cerca per titolo;
2. cerca per concetto;
3. cerca per checkpoint;
4. cerca per owner;
5. cerca issue aperte;
6. cerca issue chiuse;
7. controlla Epic padre;
8. controlla milestone;
9. controlla roadmap corrente;
10. controlla codice, asset e test.

Una differenza di nome NON giustifica una nuova issue.

---

# 3. GERARCHIA DELLE FONTI

Quando due fonti divergono usa, in ordine:

```text
1. decisione esplicita più recente dell'autore
2. main / codice / asset / test realmente presenti
3. Decision Log / ADR correnti
4. docs/roadmap/roadmap-v0.1.md
5. docs/roadmap/roadmap-checkpoint.md
6. docs/roadmap/v0.1-definition-of-done.md
7. Epic / Issue GitHub live
8. altre spec CURRENT
9. documenti archiviati
10. vecchi handoff / prompt
```

## Tracking corrente

Non assumere che esistano ancora:

```text
docs/roadmap/feature-registry.yaml
scripts/feature_registry.py
docs/roadmap/parallel-batch.yaml
```

Il vecchio Feature Registry è stato rimosso. Riverifica `main`, ma la baseline corrente è:

```text
stato feature / release
→ docs/roadmap/roadmap-v0.1.md
→ docs/roadmap/roadmap-checkpoint.md
→ GitHub live
```

Non ricreare un Feature Registry solo perché un vecchio prompt lo nominava.

---

# 4. PREFLIGHT OBBLIGATORIO

Prima di fare CRUD:

```bash
git fetch --prune origin
git status --short
git rev-parse HEAD
git log --oneline --decorate -20
```

Leggi almeno:

```text
CLAUDE.md
AGENTS.md
README.md
docs/decisions/RT_PDR_00_Decision_Log.md
docs/OPEN_DECISIONS.md
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-checkpoint.md
docs/roadmap/v0.1-definition-of-done.md
docs/technical/test-manuali-pie.md
docs/technical/tooling/scenario-map.md
```

Se un path è cambiato: cerca l'owner corrente e usa quello. Non ripristinare vecchi path.

---

# 5. ANCORE GITHUB DA RIVERIFICARE

Questi numeri sono puntatori iniziali, non verità congelate.

## Parent release

```text
#14 — [EPIC v0.1] Vertical slice 2v2 su hex — release v0.1
```

## Epic particolarmente importanti

```text
#24  — E10 · Obiettivi dinamici e fine partita
#25  — E11 · HUD, log e debug
#26  — E12 · Determinismo, QA e release
#151 — E13 · Conoscenza parziale: vista e udito
#152 — E14 · Overwatch e reazioni interattive
#153 — E15 · Showcase «Il Relè» e golden replay
#217 — E20 · HUD Icon Language
#286 — E21 · Presentazione e leggibilità
#324 — E23 · Muri, porte e interaction graph
```

## Issue operative importanti

```text
#38  — CP 2.8 Playtest partita hex
#77  — HUD di partita
#78  — intenti / certainty
#79  — combat log
#80  — debug
#85  — Release interna v0.1
#166 — reaction UI / pacing
#171 — showcase presentation + readability
#172 — Ghost Timeline
#173 — phase scrubbing
#219 — icone v0.1
#220 — widget consumano catalogo icone
#287 — personaggi sui centri hex
#288 — animazioni
#289 — leggibilità tattica
#613 — Screen HUD UMG
#705 — Pointer Interaction Contract
```

Cerca inoltre live:

```text
E46
E47
frontend
complete match
autobattle
result
replay
training
packaged
golden
```

perché il parent #14 può essere indietro rispetto a Epic aggiunte successivamente.

---

# 6. AUDIT PRIMA DEL CRUD

Costruisci:

| ID | Type | Title | Open/Closed | Parent | Release | Priority | Code | Editor Asset | Automation | Human PIE | Packaged | Action |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

Valori `Action`:

```text
KEEP
UPDATE
CREATE
CLOSE_COMPLETED
CLOSE_DUPLICATE
CLOSE_NOT_PLANNED
REOPEN
DEFER
HUMAN_REVIEW
```

Mai decidere lo stato da un solo indizio.

```text
issue closed
+ code exists
+ required tests pass
+ required editor asset exists
+ required PIE evidence exists
+ packaged gate if required
→ DONE
```

---

# 7. CLASSIFICAZIONE DEL LAVORO

Ogni Issue v0.1 deve essere classificata con una o più modalità:

```text
CODE
EDITOR_MCP
EDITOR_HUMAN_REVIEW
AUTOMATION
PIE
PACKAGED
DOCS
ART_ASSET
```

Esempio:

```text
#613 Screen HUD UMG
→ CODE
→ EDITOR_MCP
→ PIE
→ EDITOR_HUMAN_REVIEW
```

---

# 8. POLITICA CRUD

## CREATE

Crea una Issue SOLO se:

```text
nessun owner esistente
AND nessuna issue semanticamente equivalente
AND il delta è necessario alla v0.1
AND ha un DoD testabile
```

Ogni nuova issue deve contenere:

```text
Parent Epic
Release v0.1
Priority
Why
Scope
Out of scope
Dependencies
Execution Mode
Definition of Done
Automation
PIE
Unreal MCP work
Human review
Packaged impact
TurnLog / Replay impact
Privacy impact
Documentation
```

Non inventare label o milestone: leggi quelle esistenti.

## READ

Per ogni candidata:

```text
fetch issue
fetch comments se servono
cerca duplicate
cerca issue chiuse equivalenti
leggi codice
leggi test
ispeziona Unreal asset se richiesto
```

Con Unreal MCP:

```text
inspect asset
inspect Blueprint
inspect Widget
inspect Level
inspect properties
compile Blueprint
read errors
```

## UPDATE

Aggiorna quando:

```text
scope corretto ma prosa/stato stantio
checkpoint mancanti non elencati
dipendenza cambiata
path docs cambiato
asset ora esiste
codice già atterrato
resta solo Editor / PIE / packaged
una vecchia premessa è falsa
```

Preferisci note additive datate quando la storia della issue ha valore.

## CLOSE_COMPLETED

Solo con evidenza del DoD.

## CLOSE_DUPLICATE

1. identifica owner canonico;
2. trasferisci eventuale scope unico;
3. commenta il riferimento;
4. chiudi con reason `duplicate`.

## CLOSE_NOT_PLANNED

Usalo quando lo scope è stato tagliato, spostato post-v0.1 o non ha più consumer. Scrivi il motivo.

## REOPEN

Riapri solo se il DoD originale è falso su `main`. Se il lavoro nuovo è diverso, crea un follow-up sotto l'owner corretto.

---

# 9. IDEMPOTENZA

Il mandato deve poter essere rieseguito.

Due esecuzioni consecutive senza cambi nel progetto devono produrre:

```text
Created = 0
Closed = 0
Reopened = 0
Updated = 0
```

Niente:

```text
HUD cleanup 2
UX Epic 2
follow-up follow-up
final final
```

---

# 10. ROADMAP OPERATIVA v0.1

## R0 — TRACKING TRUTH

### Obiettivo
GitHub descrive il progetto reale.

### Azioni
- audit #14;
- audit di tutte le Epic v0.1 live;
- confronta con `roadmap-v0.1.md`;
- confronta con `roadmap-checkpoint.md`;
- trova Epic aggiunte dopo l'ultimo aggiornamento di #14;
- trova Epic chiuse ancora segnate aperte;
- trova issue senza parent;
- trova duplicati;
- trova scope v0.1 finito erroneamente sotto post-v0.1.

### Gate

```text
GitHub ↔ roadmap
```

senza divergenze note non dichiarate.

CRUD atteso: `READ`, `UPDATE`, `CLOSE_DUPLICATE`; `CREATE` solo su gap reale.

---

## R1 — COMPLETE MATCH

### Obiettivo

```text
Main Menu
→ Play
→ 2v2
→ Planning
→ Ready
→ Resolution
→ nuovi round
→ Objective / KO / RoundLimit
→ Result
→ Replay / Run Again / Main
```

### Owner da riconciliare

Cerca e riusa owner esistenti per:

```text
E10 objective
frontend
match flow
bot/autobattle
result
replay
```

Non creare una mega-issue di implementazione se i pezzi esistono.

Se manca, è ammessa una sola issue **integration gate**, che non possiede i sistemi.

### Gate
Una partita completa termina con un esito dichiarato.

---

## R2 — PLANNING UX

### Obiettivo
Il giocatore capisce cosa sta dichiarando.

Deve leggere:

```text
unit selected
reachable cells
path
destination
facing
action
target
AoE
cost
cooldown
Ready state
```

e distinguere:

```text
Confirmed
Predicted
Uncertain
```

### Owner principali

Prima riusa:

```text
#25  E11 HUD/log/debug
#217 E20 Icon Language
#613 Screen HUD
#705 Pointer Interaction
#172 Ghost Timeline
```

Non creare `UX-E01A/B/C` se E11/E20 possiedono già il lavoro.

Le candidate UX precedenti sono una checklist semantica, non una richiesta di decine di issue.

### Unreal MCP

Usalo per:

```text
Widget Blueprint
layout
binding
properties
overlay
screen HUD
ghost presentation
icon consumer
compile
PIE setup
```

### Gate umano
Il giocatore può rispondere a:

> Cosa farà questa unità?

senza aprire debug UI.

---

## R3 — RESOLUTION & EXPLAINABILITY

### Obiettivo
Il giocatore capisce:

```text
cosa è successo
perché
cosa ha modificato il piano
```

### Owner da riusare

```text
#79 combat log
#171 showcase readability
E11
E15
TurnLog owner corrente
replay owner corrente
```

### Regola
UI e replay leggono:

```text
Canonical TurnLog
+ reason code
```

Mai ricalcolare il gameplay per spiegarlo.

### Gate

```text
Outcome
→ Reason
→ Detail
```

comprensibile a schermo.

---

## R4 — PRESENTATION / UNREAL EDITOR

### Obiettivo
La v0.1 non sembra più un debug harness.

### Owner

```text
#286 E21
#287 character placement
#288 animations
#289 tactical readability
#613 HUD UMG
```

### Unreal MCP deve fare il più possibile

Claude può:

```text
open level
inspect level
create / modify Blueprint
create / modify Widget
assign class
set properties
bind assets
compile Blueprint
save asset
run PIE
inspect errors
capture evidence se disponibile
```

### HUMAN REVIEW obbligatoria

Non chiudere automaticamente issue che richiedono:

```text
readability
feel
visual hierarchy
camera comfort
animation feel
clutter evaluation
```

Portale a:

```text
READY_FOR_HUMAN_REVIEW
```

Se la label non esiste, non inventarla: scrivi un commento.

### Gate
Sessione umana valida:

```text
leggibilità
camera
ghost
HUD
team identity
surface readability
```

---

## R5 — REACTIONS / INFORMATION

### Owner

```text
#151 E13
#152 E14
#166 reaction UI / pacing
```

### Regola scope
Le estensioni P3 non bloccano la release se la baseline è valida. Controlla live `#314` e `#319` e l'ordine di taglio corrente.

### Gate reaction

```text
Opportunity
→ FIRE / HOLD
→ Resolver
→ TurnLog
→ Playback/UI
```

La conoscenza incompleta non produce leak.

---

## R6 — GOLDEN SHOWCASE

### Owner

```text
#153 E15
#171 presentazione/playtest
```

Lo scenario è consumer delle regole.

Vietati:

```text
if Showcase
if Turn == 4
Hero-specific branch nel resolver
```

Deve esercitare dove possibile:

```text
movement
height/layer
LOS
cover
environment
reaction
objective
KO
TurnLog
replay
```

### Gate

```text
deterministic
readable
replayable
```

---

## R7 — QA / RELEASE

### Owner

```text
#26 E12
#85 Release interna v0.1
```

Leggi il DoD corrente: non usare un conteggio storico dei gate.

Baseline recente:

```text
G1–G14
```

ma riverifica.

### Pipeline

```text
Automation
↓
Complete Match
↓
Golden
↓
PIE
↓
Development Packaged
↓
Shipping Packaged
↓
StateHash / LogHash / Replay
↓
v0.1 DONE
```

### Gate
La v0.1 non è Done se richiede l'Editor per giocare.

---

# 11. PRIORITÀ

Dopo l'audit ricalcola le priorità live.

## P0 — BLOCKER RELEASE

Gap che impedisce:

```text
Complete Match
Planning comprensibile
Resolution comprensibile
Objective / end match
Determinismo
TurnLog
Golden
Packaged
```

## P1 — REQUIRED EXPERIENCE

```text
HUD
ghost
certainty
reaction baseline
presentation
combat log
frontend flow
result
replay minimo
```

## P2 — VALUABLE BUT CUTTABLE

```text
loadout se non necessario
polish secondario
extra icon set non consumato
extra scenario UX
```

## P3 — EXPERIMENT / STRESS

Non blocca v0.1 salvo decisione esplicita.

---

# 12. DEPENDENCY GRAPH

Costruisci quello reale dopo l'audit.

Baseline:

```text
TRACKING TRUTH
      ↓
COMPLETE MATCH
      ↓
PLANNING UX
      ↓
RESOLUTION EXPLAINABILITY
      ↓
PRESENTATION
      ↓
GOLDEN
      ↓
RELEASE QA
      ↓
PACKAGED
      ↓
v0.1 DONE
```

Le lane tecniche possono procedere in parallelo.

---

# 13. UNREAL MCP — REGOLA OPERATIVA

Se Unreal MCP è disponibile e l'Editor è aperto:

```text
NON produrre soltanto istruzioni manuali
```

Per una issue `EDITOR_MCP`:

1. ispeziona stato;
2. crea/modifica asset;
3. compila Blueprint/Widget;
4. salva;
5. esegui PIE quando sensato;
6. leggi errori;
7. correggi;
8. riesegui;
9. registra evidenza;
10. aggiorna GitHub.

Esempio:

```text
#613
READ issue
↓
inspect HUD assets
↓
reuse existing widget
↓
modify UMG
↓
compile
↓
PIE
↓
verify bindings
↓
update issue
↓
HUMAN REVIEW if visual DoD remains
```

---

# 14. QUANDO FERMARE L'AUTOMAZIONE

STOP e marca `HUMAN_REVIEW` quando il criterio è:

```text
"si legge bene?"
"è troppo affollato?"
"la camera è comoda?"
"l'animazione comunica bene il colpo?"
"il ghost confonde?"
"il warning è invasivo?"
```

Claude prepara la scena, non inventa l'approvazione umana.

---

# 15. TEMPLATE NUOVA EPIC

Usalo SOLO se manca davvero un owner.

```markdown
# [EPIC v0.1] Exx — Titolo

**Parent:** #14
**Release:** v0.1
**Priority:** P0/P1/P2/P3
**Execution:** CODE / EDITOR_MCP / HUMAN_REVIEW / PACKAGED

## Why
Problema concreto.

## Scope
Cosa possiede.

## Out of scope
Cosa non possiede.

## Existing systems reused
- ...

## Child issues
- [ ] ...

## Dependencies
- ...

## Exit gate
Condizione misurabile.

## Automation
- ...

## PIE
- ...

## Unreal MCP
- ...

## Human review
- ...

## Packaged
- ...

## TurnLog / Replay
- ...

## Privacy
- ...
```

---

# 16. TEMPLATE NUOVA ISSUE

```markdown
# Titolo

**Epic:** #...
**Release:** v0.1
**Priority:** P0/P1/P2/P3
**Execution:** ...

## Why
...

## Scope
...

## Out of scope
...

## Current measured state
...

## Dependencies
...

## Definition of Done
- [ ] ...
- [ ] ...

## Automation
...

## Unreal MCP / Editor
...

## Human review
...

## PIE
...

## Packaged
...

## TurnLog / Replay impact
...

## Privacy impact
...

## Documentation
...
```

---

# 17. POLICY SULLE CANDIDATE UX

Per ogni candidate:

```text
candidate
   ↓
semantic search
   ↓
existing Epic/Issue?
   ├─ YES + complete owner → REUSE
   ├─ YES + partial owner  → UPDATE
   └─ NO
       ↓
   required v0.1?
       ├─ YES → CREATE
       └─ NO  → DEFER / post-v0.1
```

Evita soprattutto Epic parallele a:

```text
E11 HUD/log/debug
E20 HUD Icon Language
E21 Presentation/readability
E15 Showcase
E12 Release/QA
```

---

# 18. CONSISTENCY CHECK DOPO OGNI BATCH

Dopo ogni gruppo di mutazioni:

```text
1. ricerca di nuovo le issue modificate
2. verifica stato
3. verifica parent
4. verifica milestone
5. verifica duplicate
6. verifica roadmap docs
7. verifica link
8. verifica che nessun owner sia stato duplicato
```

Se GitHub MCP non permette una relazione strutturale specifica, non fingere che sia stata creata. Registrala nel body/commento secondo il workflow esistente.

---

# 19. BATCH SIZE

Non fare 100 mutazioni alla cieca.

## Batch A — Audit + parent

```text
#14
Epic v0.1 live
roadmap
```

## Batch B — Complete Match

```text
objective
frontend
match loop
result
bot
```

## Batch C — UX

```text
E11
E20
E21
HUD
ghost
pointer
combat log
```

## Batch D — reactions / knowledge / golden

```text
E13
E14
E15
```

## Batch E — release

```text
E12
packaged
DoD
```

Dopo ogni batch produci un mini-report.

---

# 20. REPORT PER BATCH

```text
READ:
- ...

UPDATED:
- ...

CREATED:
- ...

CLOSED COMPLETED:
- ...

CLOSED DUPLICATE:
- ...

CLOSED NOT PLANNED:
- ...

REOPENED:
- ...

DEFERRED:
- ...

HUMAN REVIEW:
- ...

UNREAL MCP COMPLETED:
- ...

BLOCKERS:
- ...
```

---

# 21. CRITICAL PATH REPORT

Alla fine:

| Order | Issue | Epic | Priority | Mode | Blocked by | Can Claude do it? | Needs Aaron? |
|---:|---|---|---|---|---|---|---|

`Can Claude do it?`:

```text
YES
MOSTLY
PARTIAL
NO
```

`Needs Aaron?`:

```text
No
Visual sign-off
Playtest
Art choice
Product decision
Editor-only manual fallback
```

---

# 22. DISTINZIONE CLAUDE / AUTORE

Claude possiede:

```text
audit
GitHub CRUD
docs
C++
tests
Unreal MCP editor operations
Blueprint/UMG setup dove MCP lo consente
compilation
PIE setup
log inspection
packaging commands
evidence collection
```

Aaron possiede:

```text
product decision
UX judgement
visual sign-off
playtest judgement
art direction
scope override
final release approval
```

Non chiedere intervento umano per lavoro eseguibile via Unreal MCP.

Non fingere che l'agente possa sostituire una valutazione umana di UX.

---

# 23. REGOLA DI CHIUSURA EDITOR ISSUE

```text
MCP implementation
     ↓
Blueprint / Widget compiles
     ↓
PIE functional verification
     ↓
automation where possible
     ↓
human visual gate if required
     ↓
DONE
```

Se manca solo il giudizio umano:

```text
NON chiudere
→ READY FOR HUMAN REVIEW
```

---

# 24. RELEASE GATE FINALE

La master Epic #14 si chiude solo quando il DoD corrente è verde.

Non usare:

```text
"tutte le issue sono chiuse"
```

come sostituto del gate.

Frase finale:

> RefactorTactics v0.1 può essere avviato senza Editor, giocare una partita 2v2 completa contro bot, permettere al giocatore di capire il proprio piano, risolvere deterministicamente, spiegare l'esito attraverso TurnLog/playback, terminare correttamente e riprodurre il risultato senza divergenza.

Se una parte è falsa:

```text
#14 resta OPEN
```

---

# 25. PRIMO CICLO DA ESEGUIRE

## STEP 1 — READ ONLY

Non modificare nulla.

Misura:

```text
HEAD
#14
tutte le Epic v0.1
issue aperte v0.1
roadmap-v0.1
roadmap-checkpoint
DoD
```

Classifica:

```text
CURRENT
STALE
DUPLICATE
MISSING
HUMAN_ONLY
MCP_READY
```

## STEP 2 — CRUD PLAN

Proponi:

```text
UPDATE #...
CREATE ...
CLOSE DUPLICATE #...
REOPEN #...
DEFER #...
```

con motivazione di una riga.

## STEP 3 — APPLY

Esegui le mutazioni non ambigue.

Per decisioni di prodotto non già risolte:

```text
STOP
→ OPEN DECISION
```

## STEP 4 — EDITOR EXECUTION QUEUE

Ordina:

```text
MCP_READY_NOW
BLOCKED
HUMAN_SIGNOFF_AFTER_MCP
```

## STEP 5 — NEXT 5

Restituisci le prossime 5 issue realmente eseguibili.

---

# 26. OUTPUT FINALE OBBLIGATORIO

```text
Repository:
HEAD:

v0.1 master:
Epic live:
Epic closed:
Issues open:
Issues closed:

CRUD
Created:
Updated:
Closed completed:
Closed duplicate:
Closed not planned:
Reopened:
Deferred:

Tracking inconsistencies fixed:
Remaining inconsistencies:

P0 remaining:
P1 remaining:

Unreal MCP queue:
1.
2.
3.

Human review queue:
1.
2.
3.

Packaged blockers:

Next 5 executable issues:

v0.1 confidence:
LOW / MEDIUM / HIGH

Why:
...
```

---

# 27. DIVIETI

NON:

```text
creare una seconda master Epic v0.1
creare UX-E01A/B/C se E11/E20/E21 possiedono già il lavoro
ripristinare Feature Registry rimosso
usare vecchi path come autorità
inventare label
inventare milestone
inventare issue number
chiudere una issue solo perché il codice compila
chiudere un visual playtest senza review umana
duplicare il resolver nella UI
duplicare il gameplay nel Tactical Designer
far decidere un esito a UMG/Blueprint
considerare una preview una promessa sull'esito
considerare replay = resimulation
```

---

# 28. REGOLA FINALE

Ogni operazione CRUD deve migliorare una di queste proprietà:

```text
TRUTH
→ GitHub descrive ciò che esiste davvero

EXECUTABILITY
→ è evidente qual è il prossimo lavoro reale
```

Se una nuova Issue non migliora nessuna delle due:

```text
NON CREARLA
```

Procedi repository-first, GitHub-live-first e Unreal-MCP-first per il lavoro Editor automatizzabile.
