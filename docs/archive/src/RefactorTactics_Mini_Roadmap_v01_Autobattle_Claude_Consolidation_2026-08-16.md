# REFACTORTACTICS — CONSOLIDAMENTO MINI ROADMAP v0.1 AUTOBATTLE

> `HISTORICAL` · **Materiale NON autorevole**, archiviato il **2026-08-16** dopo il consolidamento.
> **Non si applica**: si legge per sapere da dove viene una decisione. Le fonti autorevoli restano
> [`RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md),
> `feature-registry.yaml` e le due roadmap.
>
> **Recepito da**: `D-145` (execution slice, epic **E47**, nessuna milestone nuova) · `D-146` (grammatica
> visiva derivata, encoding ridondante) · epic [#952](https://github.com/DegrassiAaron/refactor-tactics-main/issues/952)
> con i checkpoint [#954](https://github.com/DegrassiAaron/refactor-tactics-main/issues/954)–[#959](https://github.com/DegrassiAaron/refactor-tactics-main/issues/959) ·
> `RNG-1`/`RNG-2` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), issue
> [#960](https://github.com/DegrassiAaron/refactor-tactics-main/issues/960).
> **Referto del filtro**: [`mini-roadmap-autobattle-spec-panel-2026-08-16.md`](../../roadmap/plans/mini-roadmap-autobattle-spec-panel-2026-08-16.md)
> — **9 sezioni su 38 applicate**.
>
> 🔴 **Cosa NON è entrato, e perché.** Tre sezioni chiedono **meno di ciò che è consegnato**, ed è la
> ragione per cui questo documento va filtrato e non applicato: la **§5** limita il gioco a
> `Move · BasicAttack · Wait` «senza aspettare il kit dei personaggi» — l'epic **E6** è chiusa e
> `Action.Wait` è già a catalogo; la **§7** e la **§12** propongono un bot a scelta casuale fra mosse
> legali da promuovere *poi* a utility scoring — `RT-FEAT-BOT-BASE` è `RELEASE_READY` con
> `BuildCandidates → ScorePlan → ChooseBestPlan` dal 2026-08-06. Eseguirle sarebbe stata una **rimozione**.
>
> ⚠️ Quattro sezioni **contraddicono un modello già deciso**: la **§4** e la **§11** trattano `Cover` come
> categoria di cella, mentre la copertura è direzionale **per bordo** dal CP 9.1 (`ERTHexSurface` ha nove
> valori, non cinque, e gli obiettivi sono entità di mappa di E10) — la tassonomia entra come **legenda di
> presentazione**, mai come dato; la **§18** fissa «massimo tre processi» mentre `parallel-batch.yaml` ne
> dichiara sei e il modello a quattro era già stato triagiato il 2026-08-14; la **§24** chiede otto Release
> field `0.1-a1 … 0.1` che sarebbero un **terzo** spazio di numerazione sulle stesse issue.
>
> ⏸️ **Il seed della §6 è differito, non respinto**: il runtime non ha alcun RNG e
> `FRTTestScenario::Seed` è documentato in codice come «dichiarato ma non consumato». La domanda è di
> prodotto e vive in `OPEN_DECISIONS.md`.
>
> ✅ **Ciò che resta è piccolo e vero**: nessuno può *guardare* la partita 2v2 bot-contro-bot che
> `RefactorTactics.HexMatch.PlaysToCompletion` gioca già headless, perché `ARTGameMode::SpawnHero` assegna
> il bot alla sola squadra 1. La mini-roadmap, ridotta a ciò che il repository non ha, è **un interruttore
> e un ritmo** — e sblocca il costo di quattro gate di release.

Lavora sul repository reale di **RefactorTactics**.

L'obiettivo di questo incarico è **consolidare e implementare una corsia accelerata verso la v0.1**, ottenendo il prima possibile una demo completamente automatica e osservabile del core gameplay.

Questa mini-roadmap:

- NON sostituisce la roadmap generale;
- NON crea una roadmap parallela scollegata;
- deve essere integrata nelle Epic, issue, Feature Map, Scenario Map, Editor Map, documentazione e Wiki già esistenti;
- deve riutilizzare quanto già implementato;
- deve evitare duplicazioni architetturali;
- può essere sviluppata usando **massimo 3 processi paralleli**.

---

# 1. PRIMA REGOLA: REPOSITORY REALITY CHECK

Prima di modificare codice, documentazione o GitHub:

1. aggiorna `main`;
2. registra lo SHA di partenza;
3. verifica la versione esatta di Unreal Engine usata dal repository;
4. analizza il codice attuale;
5. leggi Decision Log / ADR;
6. leggi roadmap corrente;
7. leggi Feature Registry / Feature Map;
8. leggi Scenario Map;
9. leggi Editor Map;
10. controlla Wiki e documentazione;
11. controlla Epic e issue GitHub aperte/chiuse;
12. identifica implementation plan e handoff già presenti;
13. individua sistemi esistenti riutilizzabili.

Gerarchia della source of truth:

```text
main / codice reale
> ADR / Decision Log
> registri e mappe correnti
> roadmap corrente
> issue GitHub live
> documentazione consolidata
> vecchi PDR / vecchi handoff
```

Non assumere che un vecchio documento rappresenti lo stato corrente del progetto.

Se esiste una feature equivalente:

```text
REUSE
```

Se è incompleta:

```text
UPDATE
```

Solo se realmente assente:

```text
NEW
```

Se qualcosa è stato superato:

```text
OBSOLETE / SUPERSEDED
```

---

# 2. TARGET DELLA MINI v0.1

Voglio poter lanciare RefactorTactics e osservare:

```text
Launch
    ↓
Load abstract hex map
    ↓
Spawn Team A + Team B
    ↓
Bot generate intents
    ↓
All intents committed
    ↓
Immutable Turn Snapshot
    ↓
Simultaneous Resolution
    ↓
TurnLog
    ↓
Presentation / Playback
    ↓
Next Turn
    ↓
KO / Objective / Turn Limit
    ↓
Winner
```

Dopo l'avvio dello scenario:

> **nessun input umano deve essere necessario per completare il match.**

Il giocatore deve poter semplicemente guardare la battaglia.

---

# 3. PRESENTAZIONE DELLA MAPPA

Questa NON è una milestone di art.

La mappa deve essere una rappresentazione tattica astratta.

Usare:

- esagoni;
- colori;
- pattern;
- glyph;
- bordi;
- primitive;
- semplici mesh graybox;
- overlay di debug.

Gli unici elementi che possono avere una rappresentazione più riconoscibile sono le unità/personaggi.

Non creare un Actor autorevole per ogni cella.

La mappa deve continuare a essere rappresentata logicamente tramite dati compatti e grafo tattico.

Il rendering è presentazione.

---

# 4. GRAMMATICA VISIVA DELLE CELLE

Non usare soltanto il colore.

Ogni categoria deve poter essere distinta attraverso almeno:

```text
Color
+
Pattern / Glyph / Shape
```

Prima tassonomia MVP:

```text
Normal
Rough
Cover
Hazard
Objective
```

Esempio indicativo:

```text
Normal
gray
plain

Rough
brown
striped

Cover
green
chevron

Hazard
red
triangle

Objective
yellow
ring
```

I colori specifici sono placeholder e devono poter essere cambiati facilmente.

---

# 5. GAMEPLAY MINIMO

Per arrivare rapidamente alla prima autobattle implementare inizialmente soltanto:

```text
Move
Basic Attack
Wait
```

Non aspettare il kit completo dei personaggi.

I bot devono utilizzare le stesse primitive/intenti che in futuro useranno giocatori e altre AI.

Nessuna pipeline speciale parallela dedicata ai bot.

---

# 6. DETERMINISMO

Il comportamento può avere varietà pseudo-casuale ma deve essere deterministico.

Regola fondamentale:

```text
Same initial state
+
Same content/rules version
+
Same bot policy version
+
Same seed

=

Same accepted intents
+
Same canonical TurnLog
+
Same final state
```

NON usare casualità globale incontrollata per gameplay o AI.

Derivare gli stream RNG dal seed e da identificatori stabili.

Registrare almeno:

```text
MatchSeed
TurnNumber
RulesVersion
BotPolicyVersion
```

e, quando già supportato dall'architettura:

```text
StateHash
LogHash
```

---

# 7. BOT MVP

Non costruire subito Tactical AI avanzata.

Prima creare un bot:

- legale;
- deterministico;
- ripetibile;
- capace di completare un match;
- facilmente debuggabile.

Prima policy:

```text
Enumerate legal actions

if legal attacks exist:
    choose one

else if legal moves exist:
    choose one

else:
    Wait
```

La scelta tra alternative può essere deterministica usando il seed.

Successivamente:

```text
Legal Actions
    ↓
Score
    ↓
Weighted deterministic choice
```

Primi fattori:

```text
Attack opportunity
Distance
Objective value
Hazard risk
Position preference
```

Non aggiungere behavior tree complessi se non già presenti e utili.

---

# 8. RELEASE TRAIN CONSOLIDATO

## v0.1-a1 — HEX BOARD

Obiettivo:

> avere il campo tattico visibile.

Include:

- mappa esagonale piatta;
- Layer 0;
- `FRTCellId`;
- coordinate;
- neighbour lookup;
- Cell ↔ World;
- renderer graybox;
- camera tattica;
- quattro unità;
- team distinguibili;
- debug CellId.

Exit gate:

```text
PLAY
→ board visible
→ four units visible
→ camera works
→ cells identifiable
→ CellId/debug correct
```

---

# 9. v0.1-a2 — AUTO MOVE

Obiettivo:

> far muovere automaticamente le unità usando la pipeline reale.

Include:

- hex graph;
- A*;
- traversabilità;
- movement cost;
- Move Intent;
- destinazione bot;
- commit automatico;
- snapshot immutabile;
- movement resolution;
- micro-step;
- occupancy;
- collision policy;
- TurnLog del movimento;
- playback movimento.

Exit gate:

```text
Seed 1234
→ bot plans
→ snapshot
→ resolution
→ playback
→ TurnLog

Restart Seed 1234
→ identical result
```

---

# 10. v0.1-a3 — AUTO SKIRMISH

## PRIORITÀ ASSOLUTA

Questa è la prima release veramente importante.

Voglio arrivarci il prima possibile.

Include:

- Legal Action Enumerator;
- Basic Attack Intent;
- attack resolution;
- HP;
- Damage;
- KO;
- Wait;
- Bot Policy MVP;
- deterministic seeded selection;
- AutoReady;
- AutoCommit;
- turn progression;
- team elimination;
- turn limit;
- match result.

Exit gate:

```text
PLAY

→ no user input
→ bots plan
→ bots move
→ bots attack
→ damage happens
→ units can die
→ turns continue
→ match ends
→ winner is shown
```

A questo punto il gioco deve essere già registrabile in video.

---

# 11. v0.1-a4 — TACTICAL BOARD

Obiettivo:

> dimostrare che la mappa è gameplay e non soltanto sfondo.

Implementare:

### Normal

```text
MovementCost = baseline
```

### Rough

```text
MovementCost > baseline
```

### Cover

Prima regola semplice e deterministica.

Riutilizzare eventuali primitive di cover già implementate.

Non creare un secondo sistema.

### Hazard

Effetto semplice e deterministico.

Esempio:

```text
enter / occupy
→ damage or penalty
```

secondo le primitive già esistenti.

### Objective

Sistema minimo per:

```text
contest
capture / hold
score
```

o altra semantica già approvata nel repository.

Exit gate:

> osservando due match con layout differenti deve risultare evidente che la disposizione della mappa modifica le decisioni e gli esiti.

---

# 12. BOT TACTICAL MVP

Dopo `a3`, passare da:

```text
random legal choice
```

a:

```text
Enumerate Legal Actions
    ↓
Evaluate
    ↓
Score
    ↓
Weighted deterministic choice
```

Score iniziale semplice:

```text
AttackScore
ObjectiveScore
DistanceScore
HazardPenalty
MovementCostPenalty
```

Non cercare di rendere i bot intelligenti prima che siano robusti.

---

# 13. v0.1-b1 — SCENARIO RUNNER

Creare o estendere l'infrastruttura scenario esistente.

Deve poter configurare almeno:

```text
ScenarioId
SeedOverride
AutoRun
AutoReady
AutoCommit
PlaybackMode
PlaybackSpeed
RepeatCount
RestartSameSeed
RerollSeed
```

Scenari iniziali:

```text
Scenario.AutoBattle.OpenField
Scenario.AutoBattle.Obstacles
Scenario.AutoBattle.Hazard
Scenario.AutoBattle.Objective
```

NON creare quattro mappe duplicate se è sufficiente usare una sandbox configurabile.

Preferire:

```text
L_DevSandbox / equivalente reale
+
Scenario Definition
```

---

# 14. v0.1-b2 — WATCHABLE BUILD

Non ampliare significativamente le regole.

Concentrarsi sulla leggibilità.

Implementare:

- Turn number;
- Phase;
- Team state;
- HP;
- unit labels;
- movimento visibile;
- attack indication;
- damage indication;
- KO indication;
- objective state;
- compact combat log;
- playback speed;
- match seed;
- winner screen;
- restart;
- same-seed restart;
- reroll seed.

Opzionale/debug:

```text
StateHash
LogHash
Bot Decision
Chosen Intent
Intent Score
```

La UI non deve ricalcolare il gameplay.

La presentazione consuma gli eventi prodotti dalla simulazione.

---

# 15. v0.1-rc1 — DETERMINISTIC AUTOBATTLE

Feature freeze.

Nessuna nuova meccanica salvo bug blocker.

Test obbligatori:

```text
SameSeedSameResult
```

Eseguire lo stesso scenario ripetutamente.

Atteso:

```text
zero divergence
```

---

```text
DifferentSeedVariation
```

Seed diversi devono poter generare comportamenti diversi.

---

```text
PermutationTest
```

Cambiare ordine di inserimento delle unità/dati.

Atteso:

```text
same canonical result
```

quando semanticamente equivalente.

---

```text
PlaybackIndependence
```

Playback:

```text
x1
x2
x4
```

Atteso:

```text
same logical result
```

---

```text
NoPath
```

Il bot non trova percorso.

Atteso:

```text
legal fallback
no deadlock
```

---

```text
AllWait
```

Atteso:

```text
turn terminates normally
```

---

```text
SimultaneousKO
```

Atteso:

```text
explicit deterministic policy
```

---

```text
TurnLimit
```

Atteso:

```text
match terminates
```

---

```text
PackagedAutoBattle
```

Atteso:

```text
scenario runs outside Editor
```

---

# 16. v0.1 — MINI AUTOBATTLE VERTICAL SLICE

La release deve dimostrare:

```text
2v2 Bot vs Bot

Abstract Hex Board

Move
Basic Attack
Wait

Normal
Rough
Cover
Hazard
Objective

Seeded Bot Decision

Simultaneous Turn Resolution

Immutable Snapshot

Deterministic Resolver

TurnLog

Scenario Runner

Playback

Minimal HUD

KO

Winner

Packaged Build
```

---

# 17. COSA NON APPARTIENE ALLA MINI ROADMAP

Non cancellare queste feature dalla roadmap principale.

Semplicemente NON devono bloccare la Mini v0.1 Autobattle.

Fuori scope:

```text
full networking implementation
dedicated server production
matchmaking
progression
public modding
full GAS integration
complete 4x4 character kits
Fast Reaction complete system
Overwatch complete system
Fog of War
sound perception
stealth
multilayer production map
elevators
complex doors
full elemental system
complete fire/water/electric propagation
advanced destruction
final VFX
final animations
final art
final UI
CommonUI migration
```

Se parti di questi sistemi esistono già:

> non rimuoverle.

Semplicemente evita di trasformarle in dipendenze per l'Autobattle MVP.

---

# 18. TRE PROCESSI PARALLELI

Utilizza massimo TRE processi.

---

## PROCESSO A — SPATIAL / BOARD

Responsabilità principali:

```text
FRTCellId
Hex coordinates
Neighbour lookup
Cell ↔ World
Graph
Traversal
A*
Movement costs
Occupancy queries
Surface metadata
Board renderer contract
Spatial debug
```

Può occuparsi anche dello scaffold tecnico necessario ai tile visuali.

Non deve diventare owner della simulazione.

---

## PROCESSO B — SIMULATION / BOT

Responsabilità:

```text
Unit logical state
Intent
Turn lifecycle
Snapshot
Movement resolver
Collision
Basic Attack
HP
Damage
KO
Objective logic
Legal Action Enumerator
Bot scoring
Seeded decision
AutoReady
AutoCommit
Match end
TurnLog
Determinism tests
```

È il processo critico per arrivare ad `a3`.

---

## PROCESSO C — CLIENT / TOOLING / CONTENT INTEGRATION

Responsabilità:

```text
Camera
Unit presentation
Board presentation integration
Movement playback
Attack playback
Damage feedback
KO feedback
HUD
Combat Log
Scenario Runner
Seed controls
Playback controls
Match result
Editor integration
Functional scenario setup
Packaged verification
```

I task che richiedono manipolazione manuale dell'Editor devono essere raccolti e passati all'utente.

---

# 19. OWNERSHIP E PARALLELISMO

I tre processi NON devono modificare liberamente gli stessi file.

Per ogni batch creare un manifest.

Esempio:

```yaml
batch: mini01-a2
base_sha: <SHA>

process_a:
  branch: mini01/spatial-a2
  goal: hex graph and path
  write:
    - <files>
  read:
    - <files>

process_b:
  branch: mini01/simulation-a2
  goal: snapshot and movement resolution
  write:
    - <files>
  read:
    - <files>

process_c:
  branch: mini01/client-a2
  goal: movement playback
  write:
    - <files>
  read:
    - <files>

shared_contracts:
  - <interfaces>

integration_order:
  - A
  - B
  - C
```

Regola:

```text
ONE SHARED FILE
=
ONE WRITER
```

Se l'ownership non è determinabile:

```text
STOP
```

prima di modificare.

---

# 20. BRANCH

Usa branch/worktree brevi.

Naming indicativo:

```text
mini01/spatial-a1
mini01/simulation-a1
mini01/client-a1
```

Poi:

```text
mini01/spatial-a2
mini01/simulation-a2
mini01/client-a2
```

Non mantenere tre mega-branch vive per tutta la roadmap.

Preferire batch piccoli:

```text
branch
→ PR
→ integration
→ tests
→ merge main
→ next batch from new main
```

---

# 21. CRITICAL PATH

Considera questa la catena principale:

```text
FRTCellId
    ↓
Hex Graph
    ↓
A*
    ↓
Move Intent
    ↓
Immutable Snapshot
    ↓
Movement Resolver
    ↓
TurnLog
    ↓
Legal Action Enumerator
    ↓
Bot Intent Generation
    ↓
Basic Attack
    ↓
HP / KO
    ↓
Match End
    ↓
AUTO SKIRMISH
    ↓
Tactical Terrain
    ↓
Scenario Runner
    ↓
Watchable Playback
    ↓
Deterministic Tests
    ↓
Packaged v0.1
```

Ottimizza il lavoro per accorciare questa critical path.

---

# 22. EDITOR CHECKPOINT

Quando serve un intervento manuale nell'Unreal Editor, non interrompere continuamente l'esecuzione.

Accumula operazioni compatibili.

Formato:

```text
EDITOR CHECKPOINT #N

Release:
Purpose:

Map:
<map>

Assets:
<assets>

Steps:
1.
2.
3.

Expected result:

Verification:

Screenshot/log requested:

Blocking:
YES / NO
```

Preferire pochi checkpoint corposi.

---

# 23. GITHUB TRACKING

Consolidare la mini-roadmap nel tracking esistente.

Controllare prima:

- Epic;
- milestones;
- issue;
- labels;
- dependency;
- Feature Registry;
- Scenario Registry;
- Editor Map;
- roadmap YAML;
- eventuale Project Control Center.

Se esiste già una Epic adatta:

```text
REUSE
```

Altrimenti valutare:

```text
Mini 0.1 — Deterministic Autobattle
```

ma solo se realmente utile.

---

# 24. RELEASE FIELD

Associare le issue a:

```text
0.1-a1 Hex Board
0.1-a2 Auto Move
0.1-a3 Auto Skirmish
0.1-a4 Tactical Board
0.1-b1 Scenario Runner
0.1-b2 Watchable Build
0.1-rc1 Deterministic Autobattle
0.1 Mini Vertical Slice
```

---

# 25. OGNI ISSUE DEVE CONTENERE

Dove pertinente:

```text
Goal
Release
Track
Parent Epic
Dependencies
Scope
Out of Scope
Files / ownership area
Acceptance Criteria
Automation Test
Functional Scenario
Editor Verification
Packaged Verification
Documentation Impact
Feature Registry Impact
Scenario Registry Impact
```

---

# 26. FEATURE MAP

La Feature Map deve riflettere almeno:

```text
Hex Board
Pathfinding
Turn Snapshot
Movement Resolution
TurnLog
Basic Combat
HP / KO
Bot Legal Actions
Bot Decision
Auto Match
Terrain Types
Objective
Scenario Runner
Playback
Combat Log
Determinism Verification
```

Non duplicare feature già presenti.

Aggiornarne status e collegamenti.

---

# 27. SCENARIO MAP

Creare o consolidare:

```text
AutoBattle.OpenField
AutoBattle.Obstacles
AutoBattle.Hazard
AutoBattle.Objective
```

Ogni scenario dovrebbe collegarsi a:

```text
Feature coverage
Issue
Automation test
Map/config
Expected outcome
Release
```

---

# 28. EDITOR MAP

Registrare soltanto attività che richiedono davvero l'Editor.

Esempi:

```text
Create/configure sandbox map
Assign materials
Configure instanced renderer assets
Place/configure unit visual archetypes
Configure HUD widget assets
Verify camera
Verify scenario assets
PIE smoke test
Packaged smoke test
```

Non mettere nella Editor Map attività C++ automatizzabili.

---

# 29. DOCUMENTAZIONE E WIKI

Aggiornare la documentazione corrente con la nuova mini-roadmap.

Non creare cinque documenti che descrivono la stessa cosa.

Preferire:

```text
one canonical execution-plan page
+
links from roadmap / wiki / feature / scenario tracking
```

Documentare chiaramente che:

> Mini 0.1 Autobattle è un execution slice della roadmap generale, non una nuova direzione del prodotto.

Aggiungere collegamenti bidirezionali dove possibile:

```text
Wiki
↔ Feature
↔ Scenario
↔ Epic
↔ Issue
↔ Release
```

---

# 30. PROJECT CONTROL CENTER

Se il repository contiene già il sistema/pagina di controllo basata su YAML:

aggiornare le sorgenti necessarie affinché siano visibili:

```text
Release
Feature
Scenario
Issue
Editor task
Progress
Dependencies
Links
```

Non creare una seconda dashboard.

---

# 31. TEST STRATEGY

Per ogni release mantenere almeno uno scenario verificabile.

## a1

```text
Map loads
Cells valid
4 units present
```

## a2

```text
same seed movement repeat
```

## a3

```text
full match completes automatically
```

## a4

```text
terrain affects decisions/result
```

## b1

```text
scenario can run unattended
```

## b2

```text
TurnLog playback matches logical result
```

## rc1

```text
determinism corpus
```

---

# 32. DEFINITION OF DONE

Per questa mini-roadmap una feature è Done quando, dove applicabile:

1. compila;
2. passa Automation Test;
3. è esercitata da uno scenario;
4. produce log/debug sufficiente;
5. non duplica primitive già esistenti;
6. non dipende dalla presentazione per gli esiti;
7. è verificata in PIE se necessario;
8. è verificata packaged al gate di release;
9. documentazione/tracking sono aggiornati;
10. il commit è focalizzato.

Il networking NON è un gate della mini autobattle offline se non viene toccato dalla feature.

Non modificare la Definition of Done generale del progetto per questo motivo.

---

# 33. COMMIT STRATEGY

Commit piccoli.

Esempi:

```text
feat(map): add deterministic hex graph

feat(path): add hex movement query

feat(turn): resolve movement from immutable snapshot

feat(bot): generate deterministic legal intents

feat(combat): resolve minimal basic attacks and ko

feat(terrain): add autobattle tactical cell rules

feat(scenario): add deterministic autobattle runner

feat(ui): add TurnLog driven autobattle playback

test(sim): add autobattle determinism corpus
```

---

# 34. ORDINE DI PRIORITÀ

Quando devi scegliere tra due attività:

## PRIORITÀ 1

Qualcosa che avvicina a:

```text
0.1-a3 AUTO SKIRMISH
```

## PRIORITÀ 2

Testabilità e determinismo.

## PRIORITÀ 3

Leggibilità/debug.

## PRIORITÀ 4

Varietà tattica della mappa.

## PRIORITÀ 5

Polish.

---

# 35. REGOLA ANTI-SCOPE-CREEP

Prima di aggiungere una nuova feature chiediti:

> È necessaria per vedere quattro unità completare automaticamente un match tattico?

Se NO:

spostarla fuori dalla Mini v0.1.

Eccezione:

infrastruttura piccola necessaria per non introdurre debito strutturale evidente.

---

# 36. PRIMA ESECUZIONE RICHIESTA

Procedi inizialmente con una fase di consolidamento, NON implementando tutta la roadmap in una volta.

Restituisci:

## A — Repository Reality Check

```text
Main SHA
UE version
Current milestone
Current relevant systems
Existing tests
Existing scenarios
Existing bot systems
Current sandbox/map
```

---

## B — Existing vs Required

Tabella:

| System | State | Action |
|---|---|---|
| Hex Map | ... | REUSE/UPDATE/NEW |
| A* | ... | ... |
| Snapshot | ... | ... |
| Resolver | ... | ... |
| TurnLog | ... | ... |
| Basic Attack | ... | ... |
| Bot | ... | ... |
| Scenario Runner | ... | ... |
| Playback | ... | ... |

---

## C — Consolidated Release Plan

Mostra:

```text
a1
↓
a2
↓
a3
↓
a4
↓
b1
↓
b2
↓
rc1
↓
0.1
```

con stato corrente di ciascuna release.

---

## D — Issue Delta

Non mostrarmi semplicemente un backlog nuovo.

Mostra:

```text
EXISTING — reuse as-is
EXISTING — modify
NEW — required
SUPERSEDED
```

---

## E — Dependency Graph

Identifica la vera critical path dal repository corrente.

---

## F — Parallel Batch #1

Prepara esattamente TRE processi.

### Process A

```text
Goal:
Branch/worktree:
Owned files:
Read dependencies:
Acceptance:
Tests:
Expected merge:
```

### Process B

stesso formato.

### Process C

stesso formato.

---

## G — Integration Plan

Dichiara:

```text
base SHA
merge order
expected conflicts
shared API contracts
tests after each merge
```

---

## H — Editor Checkpoints

Mostra soltanto gli interventi manuali realmente necessari.

---

## I — Tracking Consolidation

Indica cosa cambierai in:

```text
Roadmap
GitHub Epic
Issues
Feature Map
Scenario Map
Editor Map
Wiki
Docs
Project Control Center
```

---

## J — Risks

Massimo 10.

Ordine:

```text
highest risk first
```

---

## K — First Recommended Merge

Identifica il più piccolo incremento che dovrebbe essere integrato su `main` per primo.

---

# 37. DOPO IL REALITY CHECK

Se non esiste una decisione di design realmente bloccante:

NON aspettare altre istruzioni.

Procedi con:

```text
Batch #1
→ 3 processi paralleli
→ integration
→ tests
→ tracking update
→ report
```

Poi prepara Batch #2 partendo dal nuovo `main`.

---

# 38. RISULTATO CHE VOGLIO OTTENERE IL PRIMA POSSIBILE

La milestone operativa più importante è:

# `0.1-a3 — AUTO SKIRMISH`

Devo poter premere Play e vedere:

```text
Team A             Team B
  ● ●                 ● ●
   \                   /
    \                 /
     move / attack
          ↓
 simultaneous resolution
          ↓
       TurnLog
          ↓
       next turn
          ↓
          KO
          ↓
        WINNER
```

Quando questo funziona in maniera deterministica, testabile e ripetibile:

la base della Mini v0.1 esiste.

Da quel momento si procede con:

```text
Tactical Board
→ Scenario Runner
→ Watchable Build
→ Determinism RC
→ v0.1
```

Non sacrificare questo obiettivo per implementare prematuramente sistemi appartenenti alle milestone successive.