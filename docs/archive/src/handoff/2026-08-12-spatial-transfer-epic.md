> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

> 📦 `HISTORICAL` · **Sorgente archiviato il 2026-08-12** · **Revisionato, recepito in larga parte.**
>
> Il testo originale **non è stato riscritto**: quanto segue è l'esito della revisione. Referto completo:
> [`../../roadmap-plans/spatial-transfer-epic-2026-08-12.md`](../../roadmap-plans/spatial-transfer-epic-2026-08-12.md).
>
> **È il secondo handoff sul trasferimento nella stessa giornata**, e presuppone il primo —
> [`2026-08-12-teleport-instant-movement.md`](2026-08-12-teleport-instant-movement.md), che aveva chiuso con
> due domande aperte e una sola issue. Questo le chiude e apre l'epic.
>
> | Esito | Sezioni |
> |---|---|
> | ✅ **Recepito** | §0 · §2 (il difetto architetturale, confermato) · §3 (nessuna seconda tassonomia) · §7 (**E39** creata dopo l'audit del numero libero) · §10 (nove dipendenze collegate) · §11 (l'ordine di implementazione) · §12 (close gate) · §13 · §14 · §16 (la verifica di mutazione — il punto migliore del kit) · §17 · §18 · §19 · §21 |
> | ✅ **Verificato e corretto** | §1: `ERTMovementStyle` a sei valori, `Result.Entered = { destinazione }`, lo scenario `BLOCKED`, e **nove stati di issue su nove**. Un handoff che misura prima di scrivere, e ci prende |
> | 🔁 **Diventa decisione** | §4: `MOV-1` → **[D-118](../../../decisions/RT_PDR_00_Decision_Log.md)** (famiglia propria) · `MOV-2` → **[D-119](../../../decisions/RT_PDR_00_Decision_Log.md)** (post-v0.1, v0.2). Il kit ordina di chiuderle prima dell'implementazione, e così è stato |
> | ⚠️ **Riscritto** | §8: la Definition of Done era una lista di **quattordici sostantivi** — «bot», «determinism», «targeting / visibility policy» — non di criteri verificabili. Convertita in checkpoint con DoD misurabile |
> | ✂️ **Filtrato** | §9: tredici checkpoint proposti → **quattro issue aperte** (il percorso critico #700 #701 #702 #703), **una già chiusa** (39.1, da D-118/D-119), **otto rinviate** nel corpo dell'epic. Otto hanno la stessa prima riga: *serve un consumatore che non esiste* |
> | ⚠️ **Non applicabile** | §15: le sei liste di nomi di test sono **proposte**, e il Feature Registry di questo progetto verifica che ogni pattern dichiarato matchi un test reale. Restano nelle issue, non in `tests:` |
> | ❌ **Assente nel kit** | **chi** si teletrasporta. Tredici checkpoint descrivono un sistema completo e nessuno nomina un eroe — e in questo progetto un'abilità ha **un solo owner** ([D-029](../../../decisions/RT_PDR_00_Decision_Log.md)). Senza quello il Blink nasce come capacità del motore, cioè il difetto che [#645](https://github.com/DegrassiAaron/refactor-tactics-main/issues/645) documenta per `Action.Leap` |
>
> ⚠️ **La raccomandazione più utile del kit è una che vieta a sé stesso**: §7 scrive *«NON hardcodare E39
> prima dell'audit live»*. L'audit gli ha dato ragione sul numero — E38 era l'ultima assegnata — ma la
> cautela valeva comunque: il repository ha già pagato una collisione su `E21` e **tredici** su `D-nnn`.

---

# RefactorTactics — Spatial Transfer / Teleport / Blink
## Prompt operativo per Claude Code — integrazione Epic, issue, roadmap, scenari e test

> Data handoff: 2026-08-12  
> Repository: `DegrassiAaron/refactor-tactics-main`  
> Baseline UE: Unreal Engine 5.8  
> Obiettivo: integrare nel repository il piano completo per **Spatial Transfer / Teleport / Blink**, riusando ciò che esiste e senza creare tassonomie, subsystem, issue o roadmap duplicate.

---

# 0. Regola operativa

**NON partire creando codice o una nuova epic.**

Prima:
1. aggiorna `main`;
2. crea branch/worktree dedicato;
3. leggi `CLAUDE.md`, `AGENTS.md`, `README.md`, `docs/README.md`;
4. leggi gli owner canonici di movimento, azioni, reaction, TurnLog, roadmap e scenario harness;
5. misura il codebase reale;
6. controlla issue **open e closed**, PR, milestone ed epic già presenti;
7. riusa/modifica ciò che già possiede il lavoro;
8. crea nuova issue/epic solo per delta realmente non posseduti;
9. aggiorna registry/roadmap/scenario map/milestone map solo attraverso le sorgenti canoniche;
10. chiudi con build/test/validator e un referto misurato.

Non introdurre un nuovo “Teleport subsystem” solo perché il focus usa un nome nuovo.

---

# 1. Stato reale già misurato — RIVERIFICARE LIVE

Questo handoff deriva da un audit live del repository del 2026-08-12. Riconfermare sempre sul commit corrente.

## 1.1 La semantica di transfer esiste già

`ERTMovementStyle` contiene:

```text
None
Budget
LinearDash
LinearCharge
LinearLeap
LinearPass
```

`LinearLeap` è già semanticamente un **transfer destination-only**:

```cpp
if (Style == ERTMovementStyle::LinearLeap)
{
    ...
    Result.Final = Target;
    Result.Entered.Add(Target);
    Result.Stop = ERTLinearStop::Completed;
    return Result;
}
```

Quindi:
- guarda solo la destinazione;
- ignora unità intermedie;
- ignora celle intermedie;
- non applica hazard intermedi;
- collide solo all’arrivo;
- non cambia layer;
- non produce una sequenza reale di celle percorse.

Questo è già coperto da:

```text
RefactorTactics.Actions.Leap.IgnoresIntermediateCells
```

in `Source/RefactorTactics/Tests/RTMovementActionTests.cpp`.

---

# 2. Problema architetturale attuale

Anche se `LinearLeap` produce semanticamente solo:

```text
Origin → Destination
```

`ARTTurnManager` continua a trasformare il risultato in:

```text
Path = [Origin, Destination]
```

e lo passa al resolver simultaneo `ResolveHexPaths`.

Il resolver simultaneo è progettato per il **movimento a micro-step**:
- destinazione contesa;
- priorità;
- collisione frontale;
- occupazione cella per cella;
- progresso `Prog`;
- `MicroStepIndex`.

Quindi oggi esiste una semantica “transfer” dentro un motore concettualmente “traversal”.

Questo è il primo punto da separare prima di aggiungere:

```text
Blink
Reactive Blink
Swap
Recall
Forced Teleport
```

---

# 3. Owner documentale già esistente

L’owner della tassonomia è:

```text
docs/gameplay/spec-tassonomia-movimento.md
```

Contiene già:

```text
Move
Dash
Forced
Teleport
Reaction
```

e la matrice già dice per Teleport:

```text
micro-step                 no
celle intermedie           no
MoveBudget                 no
costo terreno              no
collisione                 solo all’arrivo
hazard intermedi           no
trigger spaziali           solo all’arrivo
rumore                     nessun passo
```

NON creare una seconda tassonomia parallela.

---

# 4. Decisioni aperte già presenti

In:

```text
docs/OPEN_DECISIONS.md
```

esistono già:

## MOV-1

```text
LinearLeap è un’eccezione del Dash
oppure
è il primo membro di una famiglia Spatial Transfer?
```

## MOV-2

```text
Un Blink entra in v0.1
oppure
Teleport resta post-v0.1?
```

Queste due voci devono essere **chiuse prima dell’implementazione**.

Non aprire nuove “decision issue” equivalenti.

---

# 5. Issue già esistente da riusare

## #645 — OPEN

Titolo:

```text
Action.Leap è la quarta capacità del motore irraggiungibile dal roster — e l'unica con semantica di trasferimento
```

Possiede già il problema:

```text
Action.Leap / LinearLeap esistono
ma nessun eroe li usa
```

Non duplicarla.

La raccomandazione del focus è:

```text
NON rimuovere LinearLeap come ramo morto.
Riconoscerlo come primo consumer della semantica Spatial Transfer.
```

Ma questa scelta va registrata come decisione canonica, non applicata silenziosamente.

---

# 6. Precedenti da NON duplicare

## #307 — CLOSED
Possiede la causa dello spostamento nel TurnLog.

## #308 — CLOSED
Possiede Forced Movement che attraversa davvero le celle e applica hazard intermedi.

È il contrasto canonico con Teleport:

```text
Forced traversal:
A → B → C → D

Spatial Transfer:
A → D
```

## #146 — CLOSED
Precedente sul fatto che `ERTMovementStyle` deve rappresentare semanticamente il comportamento e non affidarsi a `if` su ActionId.

## #425 — CLOSED
Precedente sulle capacità del motore irraggiungibili dal roster.

---

# 7. Epic proposta

Creare una nuova epic **solo se nessuna epic esistente possiede davvero questo dominio**.

Titolo suggerito:

```text
Spatial Transfer — Teleport, Blink e movimento istantaneo
```

Numero candidato:

```text
E39
```

⚠️ **NON hardcodare E39 prima dell’audit live.**

Il repository ha già avuto collisioni di numerazione epic. Verificare il prossimo ID libero al momento della creazione.

Target raccomandato:

```text
v0.2
```

non v0.1.

Motivo:
- il Teleport non esiste come azione giocabile nella v0.1;
- la semantica esiste solo come `LinearLeap`;
- `MOV-2` non è ancora chiusa;
- la roadmap post-v0.1 ha già il dominio giusto per nuove primitive strutturali.

---

# 8. Definition of Done dell’epic

L’epic è chiusa solo quando sono implementati e verificati, dove applicabile:

```text
Spatial Transfer core
Short Blink
Reactive Blink
Swap
Recall / Return Point
Portal
Forced Teleport
targeting / visibility policy
TurnLog + replay
UI / preview
bot
scenario corpus
network privacy
determinism
packaged verification
```

`Blind Teleport` e `Spatial Blockers / Jammer` possono essere l’ultimo checkpoint P3/tagliabile.

---

# 9. Serie di issue/checkpoint proposta

## 39.1 — Contratto canonico Spatial Transfer e chiusura MOV-1/MOV-2

### Obiettivo

Chiudere prima le decisioni.

Raccomandazione:

```text
MOV-1:
Spatial Transfer = famiglia semantica propria.

MOV-2:
primo contenuto giocabile = v0.2.
```

La tassonomia diventa concettualmente:

```text
Traversal
  Move
  Dash
  Forced traversal

Transfer
  Leap
  Blink
  Swap
  Recall
  Forced transfer
```

`Reaction` resta una **causa**, non una famiglia geometrica.

`Portal` resta **topologia**, non transfer personale.

### DoD
- MOV-1 chiusa con `D-nnn`;
- MOV-2 chiusa con `D-nnn`;
- owner movimento aggiornato;
- roadmap post-v0.1 aggiornata;
- feature registry aggiornato;
- #645 collegata;
- nessun runtime nuovo finché la decisione non è registrata.

---

## 39.2 — Resolver puro dei trasferimenti e conflitti simultanei

### Obiettivo

Separare il transfer dal resolver a micro-step.

Preferire una primitive pura, non Actor/subsystem:

```text
FRTSpatialTransferRequest
FRTSpatialTransferResult
URTSpatialTransferLibrary::ResolveTransfers(...)
```

I nomi non sono vincolanti: adattarli alle convenzioni del repository.

### Regole minime

```text
source valida
destination valida
destination standable
destination libera
nessuna cella intermedia
nessun MoveBudget
nessun costo intermedio
nessun hazard intermedio
nessun crossed-boundary intermedio
ordine input irrilevante
```

### Conflitto simultaneo raccomandato

Baseline:

```text
same destination → FailAll
```

Non usare ordine di iterazione come tie-break.

Se il repository decide che la priorità deve valere anche per Transfer, registrarlo come decisione esplicita.

### Test minimi

```text
SpatialTransfer.NoIntermediateCells
SpatialTransfer.OccupiedDestinationFails
SpatialTransfer.SameDestinationFailsAll
SpatialTransfer.PermutationInvariant
SpatialTransfer.DoesNotApplyIntermediateHazards
```

---

## 39.3 — Short Blink: targeting, validation e Planning

### Primo consumer giocabile

Baseline consigliata:

```text
range 2
same layer
destination visibile
destination libera
nessun path
nessuna cella intermedia
```

### Importante

Un vero Blink NON deve essere solo `LinearLeap` rinominato.

`LinearLeap` richiede una delle sei direzioni lineari.

Un Blink deve poter scegliere una cella valida entro range anche se non allineata.

### Riusa #605

`URTPlanValidationLibrary::ValidatePlan` possiede il punto canonico:

```text
LEGALE
oppure
ILLEGALE + reason code
```

Non creare una seconda autorità `ValidateTeleportPlan`.

### DoD
- action definition;
- targeting cell;
- range;
- visibilità;
- same-layer;
- occupancy;
- reason codes;
- controller;
- bot;
- intent team-only;
- automation.

---

## 39.4 — Integrazione nel TurnManager, TurnLog, Facing e Replay

### Obiettivo

Il transfer deve passare dal turno reale senza essere trasformato in un falso traversal.

### TurnLog

Riusa #307.

Deve essere ricostruibile almeno:

```text
Origin
Destination
Movement/Transfer family
Cause
ActionId
Outcome
```

Non registrare celle intermedie inesistenti.

### Facing

Un transfer non ha un ultimo passo reale.

NON usare automaticamente:

```text
FacingFromPath(...)
```

Proporre policy dell’azione, usando l’infrastruttura facing esistente:

```text
Keep
FaceTarget
Declared
```

Non creare un secondo sistema Facing.

### Replay

Il Replay Player deve riprodurre:

```text
A → B
```

dalla traccia.

Non deve rieseguire la validazione o chiedere al resolver come arrivare.

---

## 39.5 — Arrival Trigger, Overwatch e Reactive Blink

### Dipendenza

Riusa:

```text
#165 — Decision Boundary / Fast Reaction
```

Non creare una seconda macchina di reaction.

### Regola

```text
Teleport attraversa geometricamente un cono?
NO.

Teleport arriva dentro una zona controllata?
PUÒ generare EnteredArea / Appeared.
```

Quindi:

```text
CrossedBoundary → no
ArrivedInside   → sì, se la reaction definition lo dichiara
```

### Reactive Blink

È una response del sistema Fast Reaction esistente.

Test:

```text
Teleport.DoesNotTriggerCrossedBoundary
Teleport.ArrivalTriggersEnteredArea
ReactiveBlink.UsesDecisionBoundary
ReactiveBlink.ReplayDoesNotRequeryPlayer
```

---

## 39.6 — Rumore, Perception e Privacy

### Dipendenza

Riusa:

```text
#159 — Rumore → conoscenza filtrata per squadra
```

### Eventi

Un Transfer può produrre:

```text
DepartureNoise
ArrivalNoise
```

Mai:

```text
noise lungo il percorso
```

### Privacy

Durante Planning:

```text
destinazione futura del Blink = team-only
```

Il client nemico non deve riceverla.

Dopo la risoluzione può ricevere solo ciò che il sistema di visibilità/perception autorizza.

---

## 39.7 — UI/UX, preview e bot

### Planning UI

Non disegnare un falso path.

NO:

```text
A ─ ─ ─ → B
```

SI:

```text
A      ◎ B
```

Mostrare:
- origine;
- destinazioni valide;
- destinazione selezionata;
- AoE di arrivo se presente;
- reason code su cella invalida;
- intent alleati;
- nessun intent avversario.

### Bot

Il bot valuta:

```text
destination utility
```

non:

```text
intermediate path utility
```

perché le celle intermedie non esistono semanticamente.

---

## 39.8 — Swap atomico

### Regola

NON implementare come:

```text
Teleport A → B
Teleport B → A
```

Implementare come operazione atomica:

```text
A@X + B@Y
→
A@Y + B@X
```

### DoD
- atomicità;
- nessun overlap intermedio;
- validazione di entrambi;
- deterministic;
- replay;
- arrival trigger per entrambi;
- scenario.

Scenario:

```text
Spec.Movement.SwapIsAtomic
```

---

## 39.9 — Recall / Return Point

NON usare il nome `Anchor`.

`Action.Anchor` esiste già con il significato di resistenza/negazione displacement.

Usare naming temporaneo:

```text
ReturnPoint
RecallPoint
Beacon
```

fino a decisione finale.

Pattern:

```text
T0 PlaceReturnPoint(A)
T1 Recall → A
```

Validare:
- point exists;
- point not expired;
- destination free;
- destination still legal;
- durata;
- removal;
- TurnLog;
- replay;
- counterplay.

---

## 39.10 — Portal come transizione del grafo

### Importante

Portal ≠ Blink.

Il codebase ha già:

```text
URTHexMapAsset::Transitions
Revision
AddTransition
RemoveTransition
UpdateTransitions
GraphNeighbors()
ERTHexArcState
```

Quindi un portal appartiene al **grafo**.

### Possibile estensione

Aggiungere in coda a:

```text
ERTHexTransitionKind
```

un valore:

```text
Portal
```

solo dopo audit della serializzazione/versioning.

### DoD
- Portal = edge;
- può collegare layer diversi;
- pathfinding lo vede;
- active/inactive/destroyed;
- `Revision` aggiornata;
- cache invalidation;
- validator;
- scenario reachability before/after.

---

## 39.11 — Forced Teleport

### Contrasto canonico

Forced Movement normale:

```text
A → B → C → D
```

attraversa e applica hazard intermedi.

Forced Teleport:

```text
A → D
```

non attraversa.

### DoD
- hostile target;
- friendly target se consentito;
- resistenza/immunità;
- occupancy;
- destination legality;
- capability blocking;
- TurnLog cause/source;
- no intermediate hazard;
- scenario.

Coordinare con:

```text
#436 — capability taxonomy
```

per `Root`, `Suppressed`, immunità ecc.

---

## 39.12 — Corpus scenari, determinismo e gate finale

### Scenario già esistente

NON ricreare:

```text
Scenarios/Spec/Movement/TeleportSkipsIntermediateCells.json
```

Contiene già:

```text
Gadget 90 HP
due celle Fire intermedie
Teleport oltre la corsia
HP finale atteso = 90
```

È volutamente `BLOCKED` perché:

```text
requires: ["Teleport"]
```

e la capability non è ancora disponibile nello Scenario Harness.

Quando il gioco produce davvero Teleport:

1. aggiungere `Teleport` alla capability reale dell’harness;
2. completare l’intent;
3. aggiungere assertion della cella finale;
4. mantenere HP 90.

### Corpus finale minimo

```text
Spec.Movement.TeleportSkipsIntermediateCells
Spec.Movement.TeleportOccupiedDestinationFails
Spec.Movement.TeleportConflictIsPermutationInvariant
Spec.Movement.TeleportArrivalTriggersZone
Spec.Movement.SwapIsAtomic
Spec.Movement.RecallReturnsToDeclaredPoint
Spec.Map.PortalChangesReachability
Spec.Movement.ForcedTeleportSkipsIntermediateHazards
Spec.Reaction.ReactiveBlinkUsesDecisionBoundary
```

### Gate
- repeat determinism;
- permutation invariance;
- TurnLog hash;
- replay;
- packaged;
- scenario runner;
- privacy where applicable.

---

## 39.13 — Blind/Known Teleport e Spatial Blockers

Ultimo checkpoint, P3/tagliabile.

Prima consegnare la baseline:

```text
Visible destination only
```

Poi valutare:

```text
Visible
Known
Blind
```

e blocker:

```text
SpatialJammer
NoTransferZone
TransferBlocked
```

Non introdurre questi concetti prima che esista almeno un consumer reale.

---

# 10. Dipendenze GitHub da collegare, NON duplicare

Controllare e collegare almeno:

```text
#645  LinearLeap irraggiungibile
#605  Validazione del piano
#436  Capability taxonomy
#165  Decision Boundary / Fast Reaction
#159  Rumore / conoscenza filtrata
#307  TurnLog movement cause        CLOSED
#308  Forced traversal hazards      CLOSED
#146  MovementStyle precedent       CLOSED
#425  unreachable engine branches   CLOSED
```

Controllare anche le issue post-v0.1 su:

```text
movement profiles
action compatibility
reaction
perception
UI
map graph
editor
```

prima di creare nuove issue.

---

# 11. Ordine raccomandato di implementazione

Non implementare in ordine numerico cieco.

```text
#645
  ↓
39.1  decisions / contract
  ↓
39.2  pure transfer resolver
  ↓
39.3  Short Blink + Planning
  ↓
39.4  TurnManager + TurnLog + Replay
  ↓
39.12 base scenario becomes GREEN
  ↓
┌────────────┬────────────┬────────────┐
39.5         39.6         39.7
Reaction     Noise        UI/Bot
└────────────┴────────────┴────────────┘
             ↓
39.8  Swap
39.9  Recall
39.10 Portal
39.11 Forced Teleport
             ↓
39.12 full corpus
             ↓
39.13 Blind / blockers
```

---

# 12. Epic close gate

Non chiudere l’epic quando “Blink funziona”.

Chiuderla solo quando:

```text
[ ] MOV-1 chiusa
[ ] MOV-2 chiusa
[ ] #645 risolta
[ ] Spatial Transfer core deterministico
[ ] Blink giocabile
[ ] Reactive Blink riusa E14/#165
[ ] Swap atomico
[ ] Recall
[ ] Portal usa il grafo
[ ] Forced Teleport
[ ] TurnLog completo
[ ] Replay representable
[ ] Privacy team-only
[ ] Nessun evento intermedio fantasma
[ ] Bot usa le stesse regole
[ ] UI non mostra falso path
[ ] Scenario corpus verde
[ ] Feature Registry allineato
[ ] Roadmap allineata
[ ] Wiki allineata
[ ] Game build verde
[ ] Editor build verde
[ ] automation suite verde
[ ] packaged verification eseguita
```

---

# 13. Aggiornamento roadmap / registry / milestone

Dopo l’audit live:

## Feature Registry

Creare o estendere un feature ID coerente, ad esempio:

```text
RT-FEAT-MOVEMENT-SPATIAL-TRANSFER
```

ma SOLO se non esiste già una feature owner equivalente.

## Roadmap

Aggiornare:

```text
docs/roadmap/roadmap-post-v0.1.md
```

e le viste generate collegate.

## Milestone GitHub

Target raccomandato:

```text
v0.2
```

Finché MOV-2 non dice diversamente.

## Epic / sub-issue

Collegare le issue come sub-issue dell’epic reale.

Se il repository mantiene anche una checklist epic, mantenerla allineata.

Non creare due fonti discordanti.

---

# 14. Scenario Map e Capability Harness

Controllare:

```text
docs/technical/scenario-map.md
Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp
```

Oggi `Teleport` non è disponibile nella capability allowlist.

La regola è corretta:

> lo Scenario Harness non deve diventare il primo produttore di una feature che il gioco non sa ancora fare.

Aggiungere `Teleport` soltanto quando:
- un controller/bot può dichiarare realmente il Blink;
- il TurnManager lo risolve;
- il TurnLog lo osserva.

---

# 15. Test matrix

## Pure

```text
SpatialTransfer.NoIntermediateCells
SpatialTransfer.OccupiedDestinationFails
SpatialTransfer.InvalidDestinationFails
SpatialTransfer.SameDestinationFailsAll
SpatialTransfer.PermutationInvariant
SpatialTransfer.SameInputSameOutput
```

## Runtime

```text
Teleport.PlayedTurnHasNoIntermediateMoveSteps
Teleport.DoesNotApplyIntermediateHazards
Teleport.ArrivalRecomputesLOS
Teleport.ArrivalUpdatesOccupancy
Teleport.ArrivalCanTriggerZone
```

## Reaction

```text
ReactiveBlink.UsesDecisionBoundary
ReactiveBlink.ReplayDoesNotRequery
Teleport.DoesNotTriggerCrossedBoundary
```

## Replay

```text
Teleport.LogDistinguishesTransferFromTraversal
Teleport.ReplayDoesNotRecomputeTransfer
Teleport.HashIsStableAcrossPermutation
```

## Privacy

```text
Teleport.IntentIsTeamFiltered
Teleport.EnemyDoesNotReceiveDestinationDuringPlanning
```

## Portal

```text
Portal.ActiveChangesReachability
Portal.InactiveRemovesReachability
Portal.RevisionInvalidatesPathCache
```

## Swap

```text
Swap.IsAtomic
Swap.DoesNotProduceIntermediateOverlap
```

---

# 16. Mutation tests

Per almeno i punti più delicati:

1. far comparire una cella intermedia;
2. applicare hazard intermedi;
3. usare l’ordine input per decidere un conflitto;
4. far scattare Overwatch per una geometria “attraversata” ma non occupata;
5. far ricalcolare il Replay Player.

Dichiarare prima quale test deve fallire.

Se resta verde, la suite non dimostra la regola.

---

# 17. Privacy

Regola inderogabile del progetto:

```text
planning avversario non deve arrivare al client
```

Per Blink:

```text
Origin attuale     → pubblico se visibile
Destination futura → team-only
Ability intent     → team-only
Resolved arrival   → pubblico solo secondo visibilità/perception
```

Riusa:

```text
CanonicalIntentStore
FilterForTeam
team relay / sanitized RPC
```

Non mettere la destinazione di Blink in un Actor globalmente replicato.

---

# 18. Portal e GraphRevision

Il Portal è un caso diverso dagli altri consumer dell’epic.

Deve riusare:

```text
FRTHexEdge
URTHexMapAsset::Transitions
ERTHexArcState
GraphNeighbors()
Map::Revision
```

Se un portal nasce/muore:

```text
GraphRevision cambia
path cache pertinente invalida
```

Non trasformarlo in:

```text
Unit->SetCell(PortalExit)
```

perché bypasserebbe il grafo.

---

# 19. Naming collision: Anchor

NON chiamare il Recall:

```text
Anchor
```

Il repository ha già:

```text
Action.Anchor
Reaction.Anchor
```

con semantica di resistenza/negazione displacement.

Se il design vuole “metto un punto e torno lì”, usare un nome distinto.

---

# 20. Errori da evitare

Non fare:

```text
Teleport = Dash con velocità enorme
Teleport = path di due nodi dentro il micro-step resolver
Blink = LinearLeap rinominato
Swap = due teleport consecutivi
Portal = teleport ability
Recall = Anchor
ValidateTeleportPlan = secondo validatore completo
TeleportPerceptionSystem = secondo sistema perception
TeleportReactionManager = seconda reaction machine
```

---

# 21. Issue creation policy

Per ogni checkpoint:

1. cerca issue open;
2. cerca issue closed;
3. cerca PR;
4. cerca epic;
5. cerca Feature ID;
6. cerca milestone;
7. cerca scenario.

Solo se nessuno possiede il delta, crea una nuova issue.

La nuova issue deve dichiarare:

```text
Owner
Feature ID
Epic
Milestone
Dependencies
Why existing issues do not own this
Scope
Out of scope
DoD
Automation
Scenario
TurnLog/replay impact
Privacy impact
Packaged gate
```

---

# 22. Output finale richiesto a Claude

Chiudi l’attività con una tabella:

| Tipo | ID | Stato iniziale | Azione | Stato finale |
|---|---|---|---|---|
| Open Decision | MOV-1 | OPEN | ... | ... |
| Open Decision | MOV-2 | OPEN | ... | ... |
| Issue | #645 | OPEN | ... | ... |
| Epic | E?? | missing/existing | ... | ... |
| Issue | CP ??.1 | ... | ... | ... |
| Scenario | TeleportSkipsIntermediateCells | BLOCKED | ... | ... |
| Feature Registry | ... | ... | ... | ... |
| Milestone | v0.2 | ... | ... | ... |

E poi:

```text
NEW ISSUES CREATED: N
EXISTING ISSUES UPDATED: N
CLOSED ISSUES REFERENCED: N
SCENARIOS CREATED: N
SCENARIOS UNBLOCKED: N
TESTS CREATED: N
MILESTONES UPDATED: N
```

Infine:

```text
NEXT ISSUE:
#<numero> — <titolo>
```

Preferire sempre una issue esistente se rappresenta già il prossimo passo.

---

# Principio finale

> **Traversal percorre lo spazio. Spatial Transfer cambia posizione senza percorrerlo.**

Il repository possiede già il primo embrione di questa distinzione in `LinearLeap`.

Il lavoro dell’epic non è creare un secondo motore di movimento: è **rendere esplicita quella semantica, darle un resolver deterministico appropriato e costruire sopra consumer diversi senza duplicare Planning, Reaction, Perception, TurnLog, Graph o UI architecture**.
