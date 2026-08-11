# RefactorTactics — Roadmap sincronizzata a 5 processi

> ⛔ **Archiviato il 2026-08-11 — revisionato e NON applicato.** L'esito del panel vive in
> [`roadmap/plans/five-lane-roadmap-spec-panel-2026-08-11.md`](../../../roadmap/plans/five-lane-roadmap-spec-panel-2026-08-11.md).
> **Questo file non è autorità.** Delle sue voci classificate: **4 `WRONG`** (falsificate da un fatto
> misurabile), **9 `DUPLICATE`** (hanno già codice, test e owner), **6 `CONFLICT`**, **4 `CURRENT`** e
> **3 `PROPOSED`**. Ne sopravvivono tre proposte, elencate al §7 del referto.
>
> ⚠️ **La premessa non regge: la «roadmap a 3 lane» che il documento dichiara di estendere non esiste.**
> `grep` di `lane/spatial`, `lane/simulation`, `lane/client`, `A — Spatial` su tutto `docs/` → **zero
> occorrenze** (`6bdeb9a`). Non c'è baseline da estendere.
>
> ⚠️ **Dei 51 path `Source/`/`Content/`/`Scripts/` che assegna alle lane, 45 non esistono.** Il modulo runtime
> **non ha** lo split `Public/`/`Private/` (è piatto: `Source/RefactorTactics/Map/`, `Turn/`, `Replay/`…); il
> content root è **`Content/RT/`**, non `Content/RefactorTactics/`; i test stanno in
> `Source/RefactorTactics/Tests/` — 88 file, unica sottocartella `Golden/` — non in `Private/Tests/<dominio>/`.
> I 6 path corretti sono lo scheletro standard di un modulo UE, l'unica parte indovinabile senza aprire
> l'albero. Ogni assegnazione di ownership del §2, §3, §4 e §5 va riletta sapendolo.
>
> ⚠️ **Tutti e 11 gli identificatori di gate che introduce sono già in uso** in
> [`v0.1-definition-of-done.md`](../../../roadmap/v0.1-definition-of-done.md) con significato diverso, e tre
> coppie sono *quasi* la stessa cosa — il che è peggio di una collisione netta: il `G3` del documento è il
> `G4` del repository (determinismo, ma **1000** ripetizioni contro le **100** già rilasciate), il suo `G4` è
> il `G8` (intent leak), il suo `G9` collide con un `G9` che esiste e chiede altro (subset `RELEASE-V01`,
> 17 verifiche manuali). Tabella completa nel referto §4.2.
>
> ⚠️ **Il §33 punti 10–12 chiede di aggiornare a mano tre viste `GENERATA`** — `featuremap`, `scenariomap`,
> `editormap` shortlist — che si riscrivono con `python scripts/feature_registry.py shortlist` e portano
> l'avviso «non si modifica a mano». La `editormap` **è già morta una volta** proprio per questo. Il §28 dello
> stesso documento descrive correttamente il modello derivato: ha ragione sul modello e torto sull'operazione.
>
> ⚠️ **Il §33 punto 5 non è eseguibile come scritto**: `FILE_OWNERSHIP.md` **non esiste** nel repository —
> l'unica occorrenza della stringa era dentro questo file. È una creazione da decidere, non un aggiornamento.
>
> ⚠️ **Il dominio replay è il più deciso del repository e il documento lo riapre**: `D-077` (manifest per
> partita, **implementato**, `ERTReplayManifestVersion::Initial = 1`), `D-078`/[ADR-0009](../../../decisions/adr-0009-replay-logico-canonico.md)
> (Player vs Verifier), `D-083` (`ContentManifestHash`/`RulesVersion` → **v0.2**, perimetro già deciso,
> `ResolverConfigHash` non è un campo separato). `Source/RefactorTactics/Replay/` ha **16 test** che coprono
> ciò che il §9 E0, il §12 E3 e il §18 E9 pianificano per tre milestone. Dettaglio nel referto §5 e §6.
>
> ⚠️ **Il §13 E4 (modello privacy replay a tre classi + canary) è fuori scope v0.1**: la v0.1 è **2v2 offline
> contro bot** — `D-078` lo scrive, «nessun avversario, nessun server». La privacy degli intenti ha già il
> suo gate (`G8`) e il suo test. Materiale v0.2.
>
> ⚠️ **Il §6 (cinque worktree, cinque sessioni concorrenti) va pesato contro un guasto misurato**: il Decision
> Log registra l'**ottava** collisione di contatore `D-nnn` con meno parallelismo di quello proposto. E
> l'attore primario è **uno** — ADR-0009, «Decisore: utente (dev singolo)».
>
> ✅ **Cosa sopravvive**, e vale la sessione: il livello **`DoD Replay`** come gate di feature (§23 — da
> aggiungere a `feature-registry.yaml`, non alle viste), la **checklist di gate a cinque caselle** con
> `N/A + reason` (§25 — da innestare sui gate `G1`–`G15` esistenti, senza nuovi ID), la **classificazione dei
> dati replay** (§29 — da registrare per la v0.2). Il §28 non è lavoro: è una convergenza indipendente sulla
> regola già in vigore in `editor-sessions.yaml`.
>
> 📌 **Il documento contiene il proprio antidoto** ed è la lettura più giusta che se ne possa dare: il §33
> chiede di «verificare i path reali» (3), «non creare directory speculative se esiste già un equivalente»
> (4), «deduplicare» (16) e «creare solo gap reali» (17). Il referto è l'esecuzione di quei quattro punti.

## Spatial + Simulation + Client + Editor/Tooling + Replay/Audit
### Handoff operativo per Claude Code

**Scopo:** estendere la roadmap a 3 lane di RefactorTactics aggiungendo due processi paralleli, **Editor/Tooling** e **Replay/Audit**, mantenendo ownership esclusiva dei file, sincronizzazione tramite Integration Gate e milestone comuni.

**Decisione:** Editor e Replay NON diventano progetti laterali indipendenti. Sono due lane trasversali che avanzano insieme alle tre lane gameplay.

Le cinque lane sono:

```text
A — Spatial / Map
B — Simulation / Turn
C — Client / UX
D — Editor / Tooling
E — Replay / Audit
```

Principio centrale:

> Ogni ciclo produce cinque incrementi compatibili che convergono nello stesso Integration Gate.

Il gate non verifica solo che il gameplay funzioni: verifica anche che sia **authorable/debuggable nell'Editor** e **registrabile/riproducibile tramite Replay** quando applicabile.

---

# 1. Motivazione architetturale

Editor e Replay devono restare consumer dei sistemi core.

```text
                  +------------------+
                  | A Spatial / Map  |
                  +--------+---------+
                           |
                           v
+------------------+   Contracts   +----------------------+
| D Editor/Tooling |<------------- | Runtime/Core         |
+------------------+               +----------+-----------+
                                              |
                                              v
                                    +---------+----------+
                                    | B Simulation/Turn |
                                    +---------+----------+
                                              |
                                  TurnLog / Snapshot
                                              |
                    +-------------------------+--------------------+
                    |                                              |
                    v                                              v
          +---------+---------+                          +---------+---------+
          | C Client / UX     |                          | E Replay / Audit |
          +-------------------+                          +-------------------+
```

Vincoli:

- il runtime NON dipende dal modulo Editor;
- il resolver NON dipende dal Replay;
- Replay consuma `TurnLog`, snapshot, versioni e hash;
- Editor consuma API pubbliche/runtime, non duplica regole;
- Client e Replay possono condividere concetti di presentazione tramite contratti, ma non modificano gli stessi file;
- nessuna lane modifica file posseduti da un'altra lane;
- modifiche ai contratti condivisi passano da Integration Request + Gate.

---

# 2. Ownership delle 5 lane

## A — Spatial / Map

Ownership invariata:

```text
Source/RefactorTactics/Public/Map/**
Source/RefactorTactics/Private/Map/**
Source/RefactorTactics/Public/Query/**
Source/RefactorTactics/Private/Query/**

Source/RefactorTactics/Private/Tests/Map/**
Source/RefactorTactics/Private/Tests/Path/**

Content/RefactorTactics/Art/Graybox/**
Content/RefactorTactics/Maps/Grid/**
Content/RefactorTactics/Data/Maps/**
```

Responsabilità:

```text
FRTCellId
hex geometry
MapState
graph
edges
A*
LOS
trajectory
cover geometry
GraphRevision
surface topology
layer
porte
ponti
tunnel
spatial environment
```

---

## B — Simulation / Turn

Ownership invariata:

```text
Source/RefactorTactics/Public/Planning/**
Source/RefactorTactics/Private/Planning/**
Source/RefactorTactics/Public/Turn/**
Source/RefactorTactics/Private/Turn/**
Source/RefactorTactics/Public/Log/**
Source/RefactorTactics/Private/Log/**
Source/RefactorTactics/Public/Reaction/**
Source/RefactorTactics/Private/Reaction/**

Source/RefactorTactics/Private/Tests/Turn/**
Source/RefactorTactics/Private/Tests/Resolver/**
Source/RefactorTactics/Private/Tests/Reaction/**
```

Responsabilità:

```text
Intent
Ready
Commit
Snapshot
Resolver
TurnLog producer
StateHash
LogHash
Reaction Engine
determinism
network planning authority
```

**Importante:** B produce il TurnLog canonico, ma NON implementa persistenza, browser o playback Replay.

---

## C — Client / UX

Ownership invariata:

```text
Source/RefactorTactics/Public/Presentation/**
Source/RefactorTactics/Private/Presentation/**
Source/RefactorTactics/Public/UI/**
Source/RefactorTactics/Private/UI/**

Content/RefactorTactics/UI/**
Content/RefactorTactics/Input/**
Content/RefactorTactics/Blueprints/Camera/**
Content/RefactorTactics/Blueprints/UI/**
Content/RefactorTactics/VFX/**
```

Responsabilità:

```text
camera
selection
planning UX
ghost previews
HUD
Fast Reaction UX
combat log live
resolution presentation
team intent visualization
```

**Importante:** C non possiede il Replay browser/UI. Gli asset replay-specifici appartengono a E.

---

# 3. Nuova lane D — Editor / Tooling

## Ownership

```text
Source/RefactorTacticsEditor/**
Content/RefactorTactics/Editor/**

Scripts/Editor/**              // solo se questa directory esiste/è approvata
Source/RefactorTacticsEditor/Private/Tests/**
```

Possibili sottodomini:

```text
RefactorTacticsEditor/
  Validators/
  Inspectors/
  MapTools/
  ScenarioTools/
  AbilityTools/
  ReactionTools/
  EnvironmentTools/
  ReplayTools/        // solo tooling editor; runtime replay resta lane E
```

## Responsabilità

```text
editor-only validation
debug inspectors
map authoring helpers
scenario authoring
visualizzazione grafo
visualizzazione layer
validator Data Asset
scenario launcher
test launcher
reaction debugger
environment authoring/inspection
content audit
cook/packaging diagnostics editor-side
```

## Divieti

D NON deve:

- implementare regole gameplay;
- modificare `Map/**`, `Turn/**`, `Planning/**`, `Replay/**`;
- creare una seconda logica per pathfinding;
- creare una seconda logica per resolution;
- simulare risultati diversi dal resolver reale;
- rendere un asset valido solo perché l'Editor lo accetta: la validazione competitiva resta runtime/server-side.

## Regola di dipendenza

```text
RefactorTacticsEditor
        |
        v
RefactorTactics runtime APIs
```

Mai:

```text
RefactorTactics runtime
        |
        X
        v
RefactorTacticsEditor
```

---

# 4. Nuova lane E — Replay / Audit

## Ownership

```text
Source/RefactorTactics/Public/Replay/**
Source/RefactorTactics/Private/Replay/**
Source/RefactorTactics/Private/Tests/Replay/**

Content/RefactorTactics/Replay/**
```

Possibili file/concetti:

```text
RTReplayTypes.*
RTReplayHeader.*
RTReplayRecorder.*
RTReplayReader.*
RTReplaySession.*
RTReplayCheckpoint.*
RTReplayCompatibility.*
RTReplayPrivacyFilter.*
RTReplayAudit.*
```

Nomi esatti devono essere adattati alla repository reale.

## Responsabilità

```text
recording
serialization
replay format version
TurnLog ingestion
snapshot reference/checkpoint
playback data source
seeking/checkpoints
compatibility
hash verification
privacy filtering
audit competitivo
replay browser data
replay-specific UI/assets
```

## Divieti

E NON deve:

- produrre il risultato del turno;
- ricalcolare regole competitive come autorità;
- modificare `TurnLog` direttamente per esigenze del replay;
- accedere a planning privato non autorizzato;
- usare timing delle animazioni per ricostruire lo stato;
- usare Unreal Demo Recording come fonte di verità logica.

Il replay logico è:

```text
Initial State / Checkpoint
        +
TurnLog canonico
        +
RulesVersion
        +
ContentManifestHash
        +
ResolverConfigHash
        =
Replay verificabile
```

Se E necessita nuovi dati nel TurnLog:

```text
Integration Request -> B / Integration Gate
```

---

# 5. File Integration Owned

Restano riservati:

```text
Source/RefactorTactics/RefactorTactics.Build.cs
Source/RefactorTactics/RefactorTactics.cpp
Source/RefactorTactics/RefactorTactics.h

Config/**
Source/RefactorTactics/Public/Core/**
Source/RefactorTactics/Private/Core/**
Source/RefactorTactics/Public/Framework/**
Source/RefactorTactics/Private/Framework/**
Source/RefactorTactics/Public/Data/**
Source/RefactorTactics/Private/Data/**

RefactorTactics.uproject
```

### Eccezione controllata

`Source/RefactorTacticsEditor/RefactorTacticsEditor.Build.cs` può essere D-owned **dopo** che il modulo Editor è stato creato durante I0.

La prima aggiunta del modulo a `.uproject` è Integration Owned.

---

# 6. Worktree

Roadmap raccomandata:

```text
RefactorTactics/
RefactorTactics-spatial/
RefactorTactics-simulation/
RefactorTactics-client/
RefactorTactics-editor/
RefactorTactics-replay/
```

Branch:

```text
lane/spatial
lane/simulation
lane/client
lane/editor
lane/replay
```

Assegnazione:

```text
Claude #1 -> Spatial
Claude #2 -> Simulation
Claude #3 -> Client
Claude #4 -> Editor
Claude #5 -> Replay
```

---

# 7. Sincronizzazione

Ogni ciclo segue:

```text
Spatial ─────┐
Simulation ──┤
Client ──────┼── GATE N ── Automation ── Packaged ── main
Editor ──────┤
Replay ──────┘
```

Dopo ogni Gate:

```text
main
 ├─ reset/rebase lane/spatial
 ├─ reset/rebase lane/simulation
 ├─ reset/rebase lane/client
 ├─ reset/rebase lane/editor
 └─ reset/rebase lane/replay
```

---

# 8. Strategia milestone

Editor e Replay vengono integrati nelle milestone esistenti.

Non creare due roadmap isolate.

Roadmap principale:

```text
F0 Foundations
F1 Private Networking
F2 Abilities
F3 Multilevel / Environment
F4 Vertical Slice
F4.5 Tooling & Replay Hardening    <-- nuova milestone consigliata
F5 Dedicated Server / Audit
F6 Beta Systems
```

## Perché aggiungere F4.5

Editor e Replay devono esistere già prima.

Ma dopo il vertical slice serve un gate dedicato per trasformare:

```text
"funziona internamente"
```

in:

```text
"è authorable, debuggable, registrabile, ricercabile e verificabile"
```

F4.5 NON introduce nuove regole di combattimento.

Serve a consolidare:

- authoring map/scenario;
- validator;
- scenario catalog;
- replay completo di match;
- seeking/checkpoint;
- compatibilità/versioning;
- audit privacy;
- workflow QA.

Se il progetto vuole mantenere meno milestone, F4.5 può essere trattata come sub-milestone obbligatoria tra F4 e F5.

---

# 9. I0 — Bootstrap / Contracts

## A/B/C

Come roadmap precedente.

## D0 — Editor module scaffold

Durante Integration:

```text
Source/RefactorTacticsEditor/
  RefactorTacticsEditor.Build.cs
  Public/
  Private/
```

Obiettivi:

- modulo `Editor` separato dal runtime;
- nessuna dipendenza runtime -> Editor;
- test build Development Editor;
- nessuna logica gameplay.

Dopo la creazione, il modulo è D-owned.

## E0 — Replay namespace/contracts

Creare soltanto cartelle/contratti minimi:

```text
Source/RefactorTactics/Public/Replay/
Source/RefactorTactics/Private/Replay/
Source/RefactorTactics/Private/Tests/Replay/
```

NON congelare ancora un formato persistente.

Concetti iniziali:

```text
ReplayFormatVersion = experimental
Replay source = TurnLog + initial state
```

### G0 — Bootstrap Gate

Verifica:

- runtime compila senza Editor module dependency;
- Editor target compila;
- Replay folder non influenza resolver;
- 5 worktree configurabili;
- ownership documentata.

---

# 10. P1 — Foundations: Grid / Turn / Client

## A1
Hex Spatial Foundation.

## B1
Turn Foundation + primi `FRTTurnEvent`.

## C1
Camera + select.

## D1 — Tactical Debug Inspector

Creare tooling editor-only per osservare:

```text
CellId
Layer
world anchor
blocked state
surface
GraphRevision
occupant
```

Prima versione semplice:

- details/debug inspector;
- debug commands/menu solo se già supportati;
- nessun custom editor complesso prematuro.

## E1 — TurnLog Replay Smoke

Obiettivo:

```text
FRTTurnLog
   ↓
Replay ingest in-memory
   ↓
enumerate events
```

Nessuna persistenza definitiva.

Test:

- replay reader accetta TurnLog vuoto;
- mantiene ordine eventi;
- non modifica eventi.

### G1 — Cell + Event Visibility Gate

Acceptance:

```text
Grid visibile
Cell selectable
Cell inspectable nell'Editor
TurnLog prodotto
Replay lane riesce a consumarlo in-memory
```

---

# 11. P2 — Movement

## A2
A*.

## B2
Movement Resolver.

## C2
Movement Planning UX.

## D2 — Scenario Authoring MVP

Editor tool per creare/configurare scenari deterministici di movimento.

Deve usare definizioni/fixture reali del progetto.

Supporto minimo:

```text
ScenarioId
Map/fixture
Unit spawn CellId
Movement intent
Ready policy
Expected event/hash reference
```

NON duplicare il resolver.

## E2 — Movement Replay

Supportare:

```text
MoveStarted
MoveStep
MoveBlocked
MoveFinished
```

Playback data:

```text
initial unit position
+
movement TurnEvents
=
same logical route
```

### G2 — Playable + Reproducible Movement Gate

Il Gate deve dimostrare:

1. scenario creato/configurato;
2. scenario eseguito;
3. TurnLog generato;
4. replay ricostruisce il movimento;
5. stesso scenario produce lo stesso risultato.

---

# 12. P3 — Determinism / F0

## A3
GraphRevision + cache.

## B3
Snapshot normalization + StateHash + LogHash + golden tests.

## C3
TurnLog live playback.

## D3 — Golden Scenario / Validation Tools

Tooling:

```text
run selected scenario
show PASS/FAIL
show expected vs actual StateHash
show expected vs actual LogHash
open TurnLog
open snapshot diagnostics
```

D non ricalcola hash: visualizza risultati delle API runtime/test.

## E3 — Replay Format v1

> ⚠️ **Già fatto, e con campi diversi.** `FRTReplayManifest` esiste e il formato **è congelato**
> (`ERTReplayManifestVersion::Initial = 1`, `D-077`, issue `#414`). I campi decisi sono: id partita
> (`FGuid`, fuori da ogni hash), `FormatId`, topologia, hash ordinati **per turno**, checksum di fine
> partita, esito, numero di turni, wall-clock — non quelli elencati qui sotto. La traccia è **per turno**,
> non per partita, quindi `TurnCount` e `InitialSnapshotRef` non hanno la forma che il documento presume.
> ⚠️ `ContentManifestHash` e `RulesVersion` **si costruiscono alla v0.2**, con innesco dichiarato
> (`D-083`); `ResolverConfigHash` non è un campo separato — la config del resolver è *dentro* il perimetro
> di `ContentManifestHash`, già deciso.

Solo qui congelare il primo formato persistente.

Header minimo:

```text
ReplayFormatVersion
Build/RulesVersion
ContentManifestHash
ResolverConfigHash
MatchId
InitialSnapshotRef or InitialSnapshot
TurnCount
StateHash / LogHash per checkpoint quando previsto
```

Scelte:

- serializzazione versionata;
- schema deterministico;
- niente timestamp realtime nel canonical hash;
- compatibility policy esplicita.

### G3 — F0 Determinism + Replay Gate

> ⚠️ **`G3` è già preso, e questo requisito è il `G4` del repository — con 100 ripetizioni, non 1000.**
> `v0.1-definition-of-done.md`: *«G4 · Determinismo: 100 ripetizioni, checksum identico ·
> `RefactorTactics.Simulation.DeterministicReplay`»*. Il numero `1000` qui sotto **decuplica di nascosto una
> soglia già rilasciata**, dentro un gate che ha un ID diverso e che nel repository significa altro (`G3` =
> «i 10 test nominati dal catalogo esistono con quei nomi»).
> ⚠️ E quel test **è già segnalato per rinomina** da `D-078`: si chiama «replay» ma è un test del *Verifier*
> — ri-simula, non riproduce una traccia.

Acceptance:

```text
1000 repeat executions -> 0 divergence
record replay
load replay
replay final logical state == original final state
StateHash match
LogHash match
frame-rate playback 30/60/144 -> same logical result
```

**F0 non è più considerata pienamente consolidata se il TurnLog non è consumabile da Replay e i golden scenario non sono diagnosticabili.**

---

# 13. P4 — F1 Private Networking

## A4
Public spatial state.

## B4
CanonicalIntentStore, commit, team-only relay.

## C4
Team preview UX.

## D4 — Multiplayer Test Matrix Tool

Tool editor/test per avviare configurazioni:

```text
Team A client
Team B client
listen/dedicated mode quando disponibile
scenario ID
network role
```

Obiettivo:

- rendere ripetibili i privacy test;
- evidenziare canary leak;
- non fare affidamento su click manuali.

## E4 — Replay Privacy Model

Definire classi replay:

```text
Server Audit Replay
Player/Team Replay
Public/Spectator Replay
```

Regola:

> Il fatto che il server conosca gli intenti completi non significa che ogni replay possa contenerli.

Implementare redazione/sanitizzazione in base al tipo.

Test canary:

```text
CANARY_TEAM_A_PRIVATE
```

non deve apparire in replay pubblico/team B prima del momento in cui l'informazione diventa legittimamente pubblica.

### G4 — F1 Privacy + Replay Gate

Acceptance:

- zero planning leak via rete;
- zero planning leak via replay non autorizzato;
- replay server-audit classificato separatamente;
- Editor test matrix ripete il test;
- reconnect/late join non bypassano privacy.

---

# 14. P5 — F2 Abilities

## A5
LOS/Targeting.

## B5
Ability Resolver.

## C5
Ability UX.

## D5 — Ability/Data Validation Tooling

Editor validator per:

```text
AbilityId
version
tags
range
AoE
target policy
moving-target policy
cost
cooldown
priority
requirements
effect references
```

Non inventare schema: usare il Data Asset reale.

Tooling utile:

```text
ability preview diagnostics
affected cell debug
target rejection reason
```

Le ragioni devono provenire dai servizi runtime.

## E5 — Ability Replay

Supportare eventi:

```text
AbilityDeclared
impact/affected cells
DamageApplied
StatusChanged
fizzle / reason code
```

Il replay deve riprodurre l'esito registrato.

Non deve rivalutare la validità come fonte di verità.

### G5 — Combat + Replay Gate

Scenario:

```text
Ability
 -> resolve
 -> TurnLog
 -> record
 -> replay
```

Acceptance:

- stessa dichiarazione;
- stessi impact event;
- stesso danno/stato;
- stesso final hash;
- validator Editor segnala asset ability invalidi.

---

# 15. P6 — Reactions / Overwatch

## A6
Spatial reaction triggers.

## B6
Reaction Engine.

## C6
Fast Reaction UI.

## D6 — Reaction Debugger

Editor/debug tool per mostrare:

```text
ReactionInstanceId
owner
state
trigger condition
micro-step
valid targets
opportunity
response
remaining charges
prompt count
expiration
reason
```

Deve essere visibile solo in contesti dev/test autorizzati.

Non usarlo nella UI giocatore.

## E6 — Reaction Replay

Registrare/riprodurre:

```text
opportunity boundary
legittimi target nello step
response Commit/Hold/Timeout
target scelto
outcome
```

Importante:

- non registrare "future opportunities" nella stream mostrabile al player;
- i trigger simultanei rimangono un singolo opportunity set;
- il tempo visuale può cambiare, l'ordine logico no.

### G6 — Reaction Audit Gate

Acceptance:

- HOLD/FIRE/timeout riprodotti;
- multi-target opportunity riprodotta;
- replay non anticipa target futuri;
- Reaction Debugger spiega il reason chain;
- original + replay final hash match.

---

# 16. P7 — Environment

## A7
Environment spatial graph/state.

## B7
Environment resolution.

## C7
Environment UX.

## D7 — Environment Authoring / Validation

Editor tools:

```text
paint/assign surface
inspect water/fire/electric state defaults
inspect edge interaction
validate missing/invalid transitions
validate cover direction
validate interactive element configuration
```

Prima authoring semplice e affidabile; tool grafici avanzati soltanto se riducono davvero errori.

## E7 — Environment Replay

Supportare:

```text
EnvironmentChanged
surface/hazard state change
door/edge change
water
fire
electric
cover change
```

Ogni cambiamento deve essere event-driven e ordinato.

### G7 — Environment Reproducibility Gate

Scenario combo:

```text
water
+
electricity
```

oppure altro scenario canonico.

Acceptance:

- Editor authoring produce dati validi;
- resolver produce eventi;
- replay ricostruisce lo stesso ambiente;
- graph revision coerente;
- hash finali uguali.

---

# 17. P8 — F3 Multilevel

## A8
Layer, bridge, tunnel, vertical edges, LOS height.

## B8
Vertical transitions / displacement.

## C8
Layer UX.

## D8 — Multilevel Graph Editor

Tooling:

```text
layer filter
node/edge visualization
vertical transition visualization
orphan edge detection
duplicate/overlap diagnostics
door/bridge/tunnel validation
GraphRevision diagnostics
```

Obiettivo:

> un designer deve poter capire il grafo tattico senza leggere strutture C++.

## E8 — Multilevel Replay

Supportare:

```text
Layer transition
stairs/ramp/bridge/tunnel traversal
vertical displacement
transition blocked
graph state changes
```

### G8 — F3 Multilevel Gate

Acceptance:

- ponte/tunnel scenario authorable;
- graph validation clean;
- path + resolver corretti;
- replay attraversa correttamente i layer;
- camera/replay layer focus coerente.

---

# 18. P9 — F4 Vertical Slice 2v2

## A9 — World
Demo map completa.

## B9 — Game
4 personaggi, objective, bots, reactions.

## C9 — Experience
Full planning/resolution HUD.

## D9 — Scenario Catalog & Playtest Tooling

Editor:

```text
Scenario Browser
Scenario categories
spawn presets
turn presets
bot policy presets
objective presets
run selected scenario
open latest result
open diagnostics
```

Categorie suggerite:

```text
Debug
Movement
Ability
Reaction
Environment
Character
Objective
Networking
Privacy
Replay
Regression
```

Il catalogo deve supportare la stessa simulazione del gameplay.

## E9 — Full Match Replay MVP

Funzioni:

```text
record full match
load match
play/pause
speed
turn navigation
event navigation
basic seek
camera/event focus integration
match metadata
hash verification
```

Non serve ancora un editor video.

### G9 — F4 Vertical Slice Gate

Il vertical slice deve produrre:

```text
Playable 2v2
+
Scenario reproducibility
+
Full TurnLog
+
Replay MVP
+
Editor diagnostics
```

Acceptance aggiuntivi:

- match completo registrabile;
- replay completo riproducibile;
- nessun planning leak;
- scenario demo eseguibile da catalogo;
- replay hash audit passa.

---

# 19. NUOVA F4.5 — Tooling & Replay Hardening

Questa è la nuova milestone proposta.

Non aggiunge gameplay.

Obiettivo:

> rendere il vertical slice sostenibile da sviluppare, testare, bilanciare e investigare.

## D10 — Editor Hardening

Implementare/maturare:

```text
Map validation dashboard
Scenario catalog
Batch validation
Ability/content validation
Reaction diagnostics
Environment diagnostics
Multilevel graph inspector
one-click regression scenarios
report machine-readable
editor error navigation
```

Possibile output:

```text
ValidationReport.json
ScenarioResult.json
```

Formati esatti da adattare alla repository.

## E10 — Replay Hardening

Implementare:

```text
checkpoint policy
seeking
compatibility diagnostics
corrupt replay handling
missing content diagnostics
privacy profile verification
replay metadata index
state/log hash audit
replay diff tool
```

## A/B/C durante F4.5

Le lane A/B/C non aggiungono grosse feature.

Si concentrano su:

```text
bugfix
reason codes
debug contract gaps
performance
stabilità
test corpus
```

### G9.5 — Authoring & Replay Quality Gate

Exit:

- batch scenario pass;
- validator clean sulle risorse vertical slice;
- full match replay seekabile;
- replay corruption gestita con errore leggibile;
- version/hash mismatch diagnosticato;
- privacy profiles testati;
- packaged regression scenario disponibile;
- nessun tool Editor richiesto dal runtime shipping.

---

# 20. P10 — F5 Dedicated Server / Production Audit

## A10
Spatial performance/hardening.

## B10
Dedicated resolver/server lifecycle.

## C10
Network UX/reconnect/result playback.

## D11 — Server/Packaged Test Tooling

Tool/command workflow per:

```text
launch server
launch N clients
select scenario
capture logs
collect test report
run soak setup
run privacy canary
run replay validation
```

Non deve richiedere Unreal Editor per i test che devono essere packaged/headless.

La lane D possiede il tooling di orchestrazione, non le regole server.

## E11 — Server Replay / Competitive Audit

Implementare:

```text
authoritative replay recording
server audit artifact
match metadata
hash chain / integrity metadata se scelta
replay retention hooks
compatibility policy
reconnect replay catch-up support se previsto
audit export
```

### G10 — F5 Dedicated + Audit Gate

Acceptance:

```text
packaged dedicated match
 -> authoritative TurnLog
 -> replay artifact
 -> reload
 -> hash verify
 -> privacy verify
 -> audit report
```

Soak:

- ripetere match;
- zero replay corruption;
- zero divergence;
- zero private intent leak.

---

# 21. F6 — Beta Systems

## D — Editor/Beta

Focus:

```text
designer usability
validation performance
content batch tooling
accessibility checks where authorable
balance table ingestion/export if approved
release validation
```

## E — Replay/Beta

Focus:

```text
replay backward compatibility policy
storage limits
user-facing replay browser polish
spectator-safe views
telemetry/audit integration
large corpus compatibility tests
```

Il modding pubblico resta fuori finché non entra la sua milestone dedicata.

---

# 22. Matrice sincronizzata completa

| Cycle / Milestone | A Spatial | B Simulation | C Client | D Editor | E Replay | Gate |
|---|---|---|---|---|---|---|
| I0 | Contracts | Contracts | Contracts | Editor module scaffold | Replay folders/contracts | G0 Build |
| P1 / F0 | Hex/Grid | Turn types | Camera/Select | Cell Debug Inspector | In-memory TurnLog ingest | G1 Cell/Event |
| P2 / F0 | A* | Move Resolver | Move Planning | Scenario Authoring MVP | Movement Replay | G2 Playable/Reproducible Move |
| P3 / F0 | Revision/Cache | Determinism/Hash | Turn Playback | Golden Scenario Tools | Replay Format v1 | G3 F0 Determinism |
| P4 / F1 | Public spatial state | Network Authority | Team Preview | Multiplayer Test Matrix | Replay Privacy Profiles | G4 F1 Privacy |
| P5 / F2 | LOS/Targeting | Ability Resolver | Ability UX | Ability Validator | Ability Replay | G5 Combat |
| P6 / F2 | Reaction Triggers | Reaction Engine | Fast Reaction UX | Reaction Debugger | Reaction Replay | G6 Reaction Audit |
| P7 / F3 | Environment graph | Environment Resolver | Environment UX | Environment Authoring | Environment Replay | G7 Environment |
| P8 / F3 | Multilevel graph | Transition Resolver | Layer UX | Multilevel Graph Editor | Multilevel Replay | G8 F3 Multilevel |
| P9 / F4 | Demo Map | 4 Chars/Objectives/Bots | Full HUD | Scenario Catalog | Full Match Replay MVP | G9 Vertical Slice |
| F4.5 | Stabilize | Stabilize | Stabilize | Editor Hardening | Replay Hardening | G9.5 Tooling/Replay |
| P10 / F5 | Spatial Perf | Dedicated Server | Network UX | Packaged/Server Test Tooling | Server Replay/Audit | G10 Dedicated Audit |
| F6 | Content support | Beta rules | UX polish | Batch validators | Compatibility/browser polish | Release gates |

---

# 23. Nuova Definition of Done

Una feature gameplay non è automaticamente bloccata perché l'Editor custom tool non esiste ancora.

Usare livelli.

## DoD Core

Sempre obbligatorio:

```text
correct runtime behavior
tests
logs/debug
privacy
determinism
packaged verification
```

## DoD Authoring

Obbligatorio quando la feature richiede contenuto configurabile:

```text
designer can configure it
invalid configuration is detectable
debug visualization exists
editor/runtime validation agree
```

## DoD Replay

Obbligatorio quando la feature produce stato/eventi che devono essere osservabili dopo la resolution:

```text
event is replay-representable
record/load test exists
no unauthorized data is stored/exposed
final state/hash matches where applicable
```

Esempi:

- A* internal optimization: replay requirement indiretto, non serve evento nuovo.
- `MoveBlocked`: deve essere replayable.
- nuova ability: deve avere event/reason replayable.
- nuova reaction: deve avere opportunity/response/outcome replayable.
- nuova porta dinamica: deve produrre environment/edge event replayable.

---

# 24. Regola per i contratti tra lane

## D necessita dato runtime

Non modifica il runtime.

```text
D -> Integration Request -> owner A/B/Integration
```

## E necessita nuovo TurnEvent

Non modifica `Log/**`.

```text
E -> Integration Request -> B
```

## C vuole usare replay

Non modifica `Replay/**`.

Definire API consumer.

## E vuole usare widget/client presentation

Non modifica `UI/**`.

Usare API/presentation contract oppure creare replay-specific asset sotto:

```text
Content/RefactorTactics/Replay/**
```

---

# 25. Gate contract checklist

Ogni Gate deve rispondere:

```text
A Spatial
[ ] dati e query valide?

B Simulation
[ ] risultato deterministico e loggato?

C Client
[ ] risultato leggibile?

D Editor
[ ] configurabile/ispezionabile senza hack?

E Replay
[ ] registrabile/riproducibile senza divergenza o leak?
```

Se una casella non è applicabile:

```text
N/A + reason
```

Non inventare lavoro inutile solo per riempire tutte le lane.

---

# 26. Epic / Issue taxonomy

Aggiungere label:

```text
lane:spatial
lane:simulation
lane:client
lane:editor
lane:replay

type:integration
type:gate
type:validator
type:scenario
type:replay
type:audit
```

Epic:

```text
EPIC F0 — Foundations
EPIC F1 — Private Networking
EPIC F2 — Abilities & Reactions
EPIC F3 — Environment & Multilevel
EPIC F4 — Vertical Slice
EPIC F4.5 — Tooling & Replay Hardening
EPIC F5 — Dedicated Server & Audit
EPIC F6 — Beta Systems
```

Esempio issue naming:

```text
F0-D1 Tactical Debug Inspector
F0-E1 TurnLog Replay Smoke

F0-D2 Scenario Authoring MVP
F0-E2 Movement Replay

F0-D3 Golden Scenario Tools
F0-E3 Replay Format v1

F1-D4 Multiplayer Test Matrix
F1-E4 Replay Privacy Profiles

F2-D5 Ability Validator
F2-E5 Ability Replay

F2-D6 Reaction Debugger
F2-E6 Reaction Replay

F3-D7 Environment Authoring
F3-E7 Environment Replay

F3-D8 Multilevel Graph Editor
F3-E8 Multilevel Replay

F4-D9 Scenario Catalog
F4-E9 Full Match Replay MVP

F4.5-D10 Editor Hardening
F4.5-E10 Replay Hardening

F5-D11 Packaged Test Tooling
F5-E11 Server Replay Audit
```

---

# 27. Scenario Map alignment

Ogni scenario importante deve essere collegato a:

```text
Feature
Issue A/B/C/D/E
Gate
Replay coverage
Editor tooling coverage
Automated test
```

Esempio:

```text
Scenario: OW-003 Simultaneous Overwatch Targets

Feature:
Overwatch

Simulation:
F2-B6

Spatial:
F2-A6

Client:
F2-C6

Editor:
F2-D6 Reaction Debugger

Replay:
F2-E6 Reaction Replay

Gate:
G6

Assertions:
- one opportunity
- two valid targets
- stable ordering
- no future leak
- replay reproduces selected response
```

---

# 28. Editor Map alignment

La `Editor Map` non deve diventare una lista di tutte le feature.

Deve contenere soltanto attività realmente manuali/editoriali.

Esempi:

```text
Create scenario asset
Assign surface
Configure edge
Inspect graph
Validate ability
Select replay fixture
Run scenario
Inspect failure
```

Se un'attività può essere interamente automatizzata, non marcarla come Editor Task obbligatorio.

---

# 29. Replay data policy

Classificare i dati prima della registrazione.

```text
ServerOnly
TeamOnly
OwnerOnly
Public
DerivedPresentation
```

Replay profiles:

```text
AuditReplay
  può conservare dati server-authoritative secondo policy interna.

TeamReplay
  conserva solo ciò che quella squadra era autorizzata a conoscere nel tempo.

PublicReplay
  conserva soltanto informazione pubblica/spettatore autorizzata.
```

Mai assumere:

```text
"match finito = tutto il planning può diventare pubblico"
```

Questa deve essere una decisione di prodotto esplicita.

Default prudente:

> non pubblicare automaticamente intenti segreti storici.

---

# 30. Replay vs TurnLog

Separare:

```text
TurnLog
= record canonico degli eventi logici del resolver

Replay
= contenitore/versionamento/checkpoint/privacy/playback costruito sopra il TurnLog
```

Quindi:

```text
B owns TurnLog production
E owns Replay consumption/storage/audit
```

Questo è il confine più importante per evitare conflitti tra Simulation e Replay.

---

# 31. Editor vs Runtime Data

Separare:

```text
Runtime validator
= decide se dati/regole sono validi per il gioco

Editor validator
= richiama/esegue/visualizza i controlli e aiuta l'autore a correggerli
```

Non creare:

```text
Editor says valid
Runtime says invalid
```

senza reason code condiviso.

Quando possibile:

```text
shared pure validation API
     |
     +--> runtime
     |
     +--> Editor presentation
```

La shared API è Integration/owner-domain, non D-owned.

---

# 32. Commit naming

D:

```text
feat(editor): ...
feat(tooling): ...
feat(validator): ...
test(editor): ...
```

E:

```text
feat(replay): ...
feat(audit): ...
test(replay): ...
```

Gate:

```text
chore(integration): gate Gx ...
```

---

# 33. Istruzioni a Claude per consolidare la repository

> ⛔ **Questa lista è stata eseguita ai punti 1–4, 15–17 e 20, e il risultato è il referto — non un
> consolidamento.** Le verifiche che i punti 3, 4, 16 e 17 richiedono sono precisamente ciò che invalida i
> punti 5–14. In dettaglio:
> **5** — `FILE_OWNERSHIP.md` non esiste: è una creazione da decidere, non un aggiornamento.
> **9** — `F4.5` aprirebbe un **quarto** asse di numerazione accanto a `M6`–`M11`, `E1`–`E36`, `CP x.y` e alle
> undici milestone GitHub per fetta di release.
> **10, 11, 12** — Feature Map, Scenario Map ed Editor Map sono viste **`GENERATA`**: si riscrivono con
> `python scripts/feature_registry.py shortlist` e una modifica a mano viene persa alla rigenerazione, con
> `--check` che la segnala. L'estensione si fa nel modello (`feature-registry.yaml`, `editor-sessions.yaml`).
> **13** — la spec replay ha già owner: `D-077`, `D-078`/ADR-0009, `D-083`, più il
> [conflict report](../../../roadmap/plans/replay-system-conflict-report-2026-08-10.md) del 2026-08-10.
> **8** — i gate `G0`–`G10` collidono tutti con `G1`–`G15` già in uso.
> ✅ Restano validi e non eseguiti: il livello `DoD Replay` (§23), la checklist a cinque caselle (§25) e la
> classificazione dati replay (§29) — vedi §7 e §8 del referto.

Claude deve:

1. ispezionare la roadmap a 3 lane corrente;
2. trattare questo documento come estensione/supersessione della parte relativa ai processi paralleli;
3. verificare i path reali;
4. non creare directory speculative se esiste già un equivalente;
5. aggiornare `FILE_OWNERSHIP.md`;
6. aggiungere lane D ed E;
7. aggiornare diagrammi della roadmap;
8. aggiornare milestone/gate;
9. aggiungere F4.5 se non esiste una milestone equivalente;
10. aggiornare Feature Map;
11. aggiornare Scenario Map;
12. aggiornare Editor Map;
13. aggiornare Replay spec/wiki;
14. aggiornare PDR/roadmap dove appropriato;
15. cercare Epic/Issue esistenti;
16. deduplicare;
17. creare solo gap reali;
18. aggiungere label lane/editor/replay/gate;
19. preservare privacy e determinismo;
20. produrre report finale.

---

# 34. Report finale richiesto a Claude

```text
1. Existing roadmap inspected
2. Existing editor tooling inspected
3. Existing replay implementation/spec inspected

4. Files changed
5. FILE_OWNERSHIP changes
6. Milestone changes
7. Gate changes

8. Spatial issues updated
9. Simulation issues updated
10. Client issues updated
11. Editor issues created/updated
12. Replay issues created/updated

13. Feature Map changes
14. Scenario Map changes
15. Editor Map changes
16. Replay docs/wiki changes

17. Conflicts found
18. Decisions superseded
19. Open decisions

20. Next 5 parallel tasks:
    A Spatial
    B Simulation
    C Client
    D Editor
    E Replay
```

---

# 35. Prossimo batch parallelo consigliato

Se la repository è ancora in Fondazioni, il primo batch deve essere:

```text
A — Hex Spatial Foundation
B — Turn Foundation
C — Tactical Client Foundation
D — Tactical Debug Inspector
E — TurnLog Replay Smoke
```

Gate:

```text
G1 — Cell + Event Visibility
```

Poi:

```text
A — A*
B — Movement Resolver
C — Movement Planning UX
D — Scenario Authoring MVP
E — Movement Replay
```

Gate:

```text
G2 — Playable + Reproducible Movement
```

Questa è la cadenza di riferimento per tutte le milestone successive.
