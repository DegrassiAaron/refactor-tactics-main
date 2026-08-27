# REFACTORTACTICS — HANDOFF PER CLAUDE / CLAUDE CODE
## Action Phases, Dodge, Guard, Brace, Interact, Overwatch e Reaction Economy — Epic/Issue roadmap fino a v1.0

**Data:** 2026-08-26  
**Destinatario:** Claude / Claude Code  
**Repository atteso:** `DegrassiAaron/refactor-tactics-main`  
**Branch:** verificare `main` prima di qualsiasi modifica  
**Engine atteso dal contesto più recente:** Unreal Engine 5.8.1, ma verificare la patch bloccata nel repository  
**Tipo documento:** handoff operativo per audit, consolidamento e creazione/aggiornamento di Epic/Issue

---

# 0. OBIETTIVO

Consolidare nel repository le decisioni più recenti sull'economia delle azioni e sulle macro-fasi del turno, con particolare attenzione a:

- `Dash` come **macro-fase**, non come nome dell'azione generica di spostamento;
- `Dodge` come nome del movimento rapido/generico che avviene nella fase `Dash`;
- `Move` come ultima fase volontaria standard prima di `Cleanup`;
- `Guard`, `Brace`, `Interact` e `Overwatch` come scelte con costi di compatibilità forti e leggibili;
- `Overwatch` come controllo del movimento della fase `Move`, non come trigger universale su ogni cambio di cella;
- `Brace` come principale risposta/counter alle azioni della fase `Dash` quando queste interagiscono con l'unità o la zona braced;
- `Reaction` come risorsa forte e non gratuita;
- una singola `ResolvePhase` per ogni azione concreta;
- roadmap e tracking fino a `v1.0` senza creare fonti parallele.

Questo file NON autorizza a sovrascrivere ciecamente il repository. Prima auditare `main`, Decision Log/ADR, Feature Registry, roadmap e GitHub.

---

# 1. REGOLA DI PREVALENZA

Usare questo ordine:

1. repository `main` corrente e codice/test misurabili;
2. ADR / Decision Log più recenti;
3. Feature Registry / roadmap / owner spec correnti;
4. decisioni esplicite consolidate in questo handoff, se non ancora recepite;
5. PDR/documenti storici;
6. vecchi handoff/chat.

Se una decisione di questo file confligge con una decisione più recente già approvata nel repository:

- NON sovrascrivere silenziosamente;
- aprire/aggiornare una Decision Issue;
- segnare il conflitto nel report finale;
- proporre la riconciliazione;
- mantenere il repository buildabile e i tracker coerenti.

---

# 2. AUDIT OBBLIGATORIO PRIMA DI CREARE EPIC/ISSUE

Prima di modificare tracking o documentazione:

1. aggiornare `main` e registrare HEAD;
2. leggere `AGENTS.md`, `CLAUDE.md`, `README.md` e gli owner spec pertinenti;
3. verificare almeno:
   - `docs/gameplay/spec-economia-del-turno.md`;
   - Decision Log e ADR su turn phases/reactions;
   - `docs/OPEN_DECISIONS.md`;
   - roadmap v0.1/post-v0.1;
   - `feature-registry.yaml` e viste generate;
   - Scenario Map / Scenario Harness;
   - Editor Map / test manuali PIE;
   - balance/data definitions;
   - Wiki corrente;
4. cercare Epic/Issue aperte e chiuse per:
   - action economy;
   - movement compatibility;
   - Dash/Dodge;
   - Guard;
   - Brace;
   - Interact;
   - Overwatch;
   - Reaction framework;
   - Decision Window;
   - Prep/Dash/Blast/Move;
5. verificare in particolare se esistono già equivalenti di E38 o altre Epic su economy/movement compatibility;
6. **aggiornare l'esistente prima di creare nuovi Epic/Issue**;
7. non inventare numeri di Issue/Epic: lasciare che GitHub li assegni.

Principio di tracking:

```text
Decisione
  -> Feature ID
  -> Epic / Issue
  -> Release
  -> Scenario
  -> Test
  -> Wiki / Docs
  -> Debug / TurnLog
```

---

# 3. GRAMMATICA DEL TURNO DA PRESERVARE

Ordine macro corrente:

```text
PLANNING
  -> COMMIT / SNAPSHOT
  -> PREP
  -> DASH
  -> BLAST
  -> MOVE
  -> CLEANUP
```

Regole:

- `Move` è l'ultima fase volontaria standard prima di `Cleanup`.
- `Reaction` NON è una macro-fase.
- Fast Action / Fast Reaction sono `Decision Boundary` aperti da eventi validi durante la Resolution.
- presentation timing, animazioni, slow-motion e countdown non cambiano l'ordine logico.
- ogni azione concreta deve dichiarare **una sola** `ResolvePhase`.
- una prepared action può armare uno stato in una fase e generare effetti/opportunity in una fase successiva, ma l'azione pianificata resta assegnata a una sola fase.

---

# 4. DECISIONI PIÙ RECENTI DA CONSOLIDARE

Usare gli stati:

- `LOCKED_CHAT`: decisione esplicita emersa nella conversazione del 2026-08-26; deve essere riconciliata col repository;
- `CURRENT_REPO_EXPECTED`: già coerente con il contesto repository più recente, ma va riverificata;
- `OPEN_DESIGN`: non inventare una soluzione definitiva;
- `OPEN_BALANCE`: meccanica chiara, numeri/potenza da playtest.

## 4.1 Dash è la fase; Dodge è il movimento generico della fase Dash

**Stato:** `LOCKED_CHAT`

Nuova terminologia:

```text
DASH = macro-fase
DODGE = movimento rapido/generico che avviene nella fase Dash
```

Non usare più `Dash` contemporaneamente come:

- nome della macro-fase;
- nome dell'azione generica universale.

Azioni hero-specific della fase Dash possono mantenere nomi propri:

```text
Dodge
Charge
Blink
Leap
Breach
Special Reposition
```

**OPEN_DESIGN:** verificare se `Dodge` è sempre disponibile a tutti i personaggi o se il profilo base è character/data-driven.

## 4.2 Il movimento della fase Dash non è un Move gratuito aggiuntivo

**Stato:** `LOCKED_CHAT` per il principio, `OPEN_BALANCE` per i valori.

Se una unità usa un'azione nella fase Dash:

- il movimento può essere già contenuto nell'azione Dash-phase;
- oppure la unità conserva soltanto un `Move` ridotto nella fase Move;
- non deve essere assunto automaticamente `Dodge/Dash completo + Move completo`.

Modello data-driven suggerito, da adattare alle strutture reali:

```text
PostDashMovePolicy:
- None
- Reduced
- Normal   // solo se esplicitamente consentito
```

Oppure un equivalente già esistente nel repository.

**Non canonizzare ora**:

- numero celle Dodge;
- percentuale/riduzione Move;
- costo MP esatto;
- eccezioni per personaggio.

## 4.3 Overwatch standard controlla la fase Move, non la fase Dash

**Stato:** `LOCKED_CHAT`

Regola di gameplay da consolidare:

```text
Enemy changes cell in DASH via Dodge/Charge/Blink/Leap
-> standard Overwatch DOES NOT trigger solely because of that movement

Enemy changes cell because of Forced Movement during Blast/Dash
-> standard Overwatch DOES NOT trigger solely because of that movement

Enemy performs normal Move-family transition during MOVE
-> standard Overwatch MAY create a Reaction Opportunity if all other conditions are valid
```

La famiglia Move comprende, se corrente nel repository:

```text
Move / Normal
Sprint
Withdraw
Sneak (se/quanto definito)
```

Overwatch continua a rispettare:

- area/cone;
- facing;
- LOS;
- detection/visibility;
- charges;
- opportunity sanitization;
- FIRE/HOLD;
- timeout `HOLD`;
- stable deterministic ordering.

Il trigger deve essere esplicito per **movement phase/kind**, non inferito da un generico `position changed`.

## 4.4 Brace è il principale counter delle azioni Dash che entrano nella sua interazione

**Stato:** `LOCKED_CHAT` per il ruolo; `OPEN_DESIGN/OPEN_BALANCE` per il payload esatto.

Intento di design:

> Overwatch punisce il Move finale; Brace deve essere la risposta più naturale contro un engage/reposition della fase Dash che interagisce con la posizione/area del personaggio braced.

Brace NON deve essere interpretato come aura globale che annulla tutta la fase Dash sulla mappa.

Deve avere effetto quando la Dash-phase action:

- tenta di attraversare/occupare la cella controllata;
- tenta Charge/Ram/engage sul braced unit;
- tenta displacement contro il braced unit;
- attraversa una eventuale zone-of-control prevista dalla definizione.

Possibili payload da decidere separatamente:

- resistenza/annullamento displacement;
- arresto della Charge/Dodge in collisione;
- mitigazione danno da engage;
- Stagger sull'attaccante;
- modifica della transizione.

**Non implementare tutto automaticamente.** Aprire balance/design issue per il payload v0.x.

## 4.5 Guard sacrifica Move e Reaction

**Stato:** `LOCKED_CHAT`

Baseline di compatibilità:

```text
GUARD
ResolvePhase: PREP (salvo owner spec più recente)
Main: consumed
Move: unavailable
Reaction: unavailable
```

Guard è quindi un forte commitment difensivo.

Il personaggio non deve poter fare una combinazione base tipo:

```text
Move + Guard + Hero Reaction
```

salvo eccezione character-specific esplicitamente data-driven e approvata.

**OPEN_BALANCE:** potenza/direzionalità/mitigazione di Guard.

## 4.6 Interact consente Move ma sacrifica Reaction

**Stato:** `LOCKED_CHAT`

Baseline:

```text
INTERACT
Main: consumed
Move: allowed
Reaction: unavailable
```

Quindi il giocatore può riposizionarsi nella fase Move e interagire nella fase dichiarata dalla specifica interazione, ma non conserva una forte Reaction nello stesso turno.

Ogni interazione concreta deve avere una sola `ResolvePhase`, per esempio solo se supportato dal design corrente:

```text
Open Door     -> PREP or BLAST according to definition
Hack Console  -> BLAST or dedicated declared phase according to definition
Use Elevator  -> MOVE according to transition definition
```

Non introdurre un mapping universale inventato: la regola importante è **una phase per concrete interaction**.

## 4.7 Reaction è una risorsa forte, non una free action

**Stato:** `LOCKED_CHAT`

Le Reaction comprendono capacità forti di risposta/guardia/counter e non devono essere implicitamente disponibili dopo qualunque scelta.

Conseguenza:

- Guard -> Reaction OFF;
- Interact -> Reaction OFF;
- le azioni Dash/Dodge possono disabilitare o riservare Reaction secondo policy;
- Overwatch usa/prepara la propria capacità reattiva e non deve stackare gratuitamente con una seconda forte Reaction non prevista;
- Hero-specific Reaction devono rispettare compatibility rules data-driven.

**OPEN_DESIGN:** compatibilità di `BasicAttack + Reaction` e delle singole Hero Main Abilities. Non decidere in questo handoff.

## 4.8 Prep, Guard, Brace, Overwatch

**Stato:** misto.

Preservare, se confermato dal repository:

```text
Guard      resolves/prepares in PREP
Brace      resolves/prepares in PREP
Overwatch  arms in PREP
```

Ma distinguere nettamente i ruoli:

```text
Brace     -> anti-engage / anti-Dash interaction
Guard     -> difesa pianificata forte; exact anti-Blast identity da confermare
Overwatch -> area control contro Move-phase movement
```

`Guard = anti-Blast` è una **direzione proposta**, non considerarla locked finché non viene approvata da owner spec/Decision Log.

## 4.9 Sprint non è Dodge

**Stato:** `CURRENT_REPO_EXPECTED`

```text
Sprint = Move Profile, resolves in MOVE
Dodge  = Dash-phase movement/action
```

Non spostare Sprint nella fase Dash per similitudine visiva.

---

# 5. MATRICE DI COMPATIBILITÀ — BASELINE DA IMPLEMENTARE/VALIDARE

Questa tabella descrive la baseline target delle decisioni recenti. Se il repository contiene eccezioni approvate, conservarle in dati/validator e segnalarle.

| Azione / famiglia | ResolvePhase | Main | Move finale | Reaction | Trigger/ruolo principale |
|---|---|---:|---:|---:|---|
| Wait | none/pass | libera | secondo piano | secondo piano | rinuncia esplicita |
| Dodge | Dash | dipende dal modello reale | None/Reduced di default | da validare | reposition rapido pre-Blast |
| Charge/Blink/Leap | Dash | ability-specific | None/Reduced di default | ability-specific | special movement |
| BasicAttack | Blast | occupata | allowed salvo vincolo | **OPEN_DESIGN** | attacco standard |
| Guard | Prep | occupata | **NO** | **NO** | commitment difensivo forte |
| Brace | Prep | occupata | **NO** come baseline corrente da riconciliare | da riconciliare con nuova economy; non stackare gratis | anti-Dash/anti-displacement |
| Interact | definition-specific | occupata | **YES** baseline | **NO** | map/objective interaction |
| Overwatch | Prep arm | occupata | policy dedicata/reduced se esiste | reserved/consumed by OW framework | punisce Move-phase transitions |
| Move/Normal | Move | non occupa Main | azione stessa | allowed salvo altre scelte | movement finale |
| Sprint | Move | non occupa Main | azione stessa | compatibility da owner spec | high-mobility Move profile |
| Forced Movement | event phase | n/a | non consuma automaticamente Move | non disabilita automaticamente Reaction | effetto causato da terzi |

### Nota critica su Brace

La documentazione storica può trattare Brace come Reaction Profile. La decisione più recente della chat sposta il design verso un commitment forte anti-Dash. Claude deve **auditare** il modello reale e creare una Decision Issue se serve cambiare ownership/costi, senza rompere E14/Reaction Framework in modo ad hoc.

---

# 6. REGOLE TECNICHE DEL DATA MODEL

Non hard-codare una matrice di `if (Action == X)` sparsa nel resolver/UI.

Preferire definizioni data-driven compatibili con il modello reale del progetto, per esempio concettualmente:

```text
ActionDefinition
- ActionId
- ResolvePhase
- ActionKind
- MovementKind
- ConsumesMain
- MoveCompatibility
- ReactionCompatibility
- PostDashMovePolicy
- TriggerEligibilityTags
- Requirements
- Priority
- Version
```

Per Overwatch/reactions:

```text
ReactionDefinition
- TriggerPhaseMask / AllowedMovementKinds
- OpportunityRules
- Charges
- TimeoutPolicy
- Visibility/Detection requirements
- StablePriority
```

Non inventare nomi C++ se esistono già equivalenti.

Il validator deve produrre reason code leggibili, ad esempio equivalenti a:

```text
ActionConflict.MainOccupied
ActionConflict.MoveDisabledByGuard
ActionConflict.ReactionDisabledByInteract
ActionConflict.ReactionDisabledByGuard
ActionConflict.PostDashMoveReduced
ActionConflict.OverwatchOnlyTriggersMovePhase
```

Usare gli enum/tag/reason code reali se esistono già.

---

# 7. TURNLOG / EXPLAINABILITY

Ogni scelta o blocco importante deve essere spiegabile.

Aggiungere/riusare eventi/reason code per:

```text
DodgeDeclared
DodgeResolved
PostDashMoveReduced
GuardPrepared
GuardDisabledMovement
GuardDisabledReaction
BracePrepared
BraceInterceptedDashInteraction
InteractDeclared
InteractDisabledReaction
OverwatchArmed
OverwatchIgnoredNonMoveTransition
OverwatchOpportunityCreated
ReactionUnavailableByPlanCompatibility
```

Non creare necessariamente un evento per ogni riga se il TurnLog corrente ha una grammatica più generale. Mappare ai tipi canonici.

UI e replay devono consumare il TurnLog/risultato autorevole, non ricalcolare la ragione lato client.

---

# 8. SCENARI MINIMI DA COLLEGARE

Creare/aggiornare scenari equivalenti, senza duplicare ScenarioId esistenti.

## PHASE-AE-001 — Dodge before Blast

Setup:
- unità A usa Dodge in Dash;
- unità A ha un Blast pianificato o un nemico ha Blast sulla cella originaria.

Assert:
- Dodge risolve in Dash;
- Blast usa la posizione post-Dodge;
- normal Move non è applicato prima del Blast.

## PHASE-AE-002 — Dodge reduces final Move

Assert:
- dopo Dodge, Move finale usa la policy `None` o `Reduced` configurata;
- nessun doppio movimento pieno implicito.

## PHASE-AE-003 — Overwatch ignores Dodge

Assert:
- target attraversa cono OW durante Dash/Dodge;
- nessuna Reaction Opportunity standard OW.

## PHASE-AE-004 — Overwatch catches Move

Assert:
- stesso target attraversa il cono durante Move;
- opportunity generata se LOS/detection/charges validi.

## PHASE-AE-005 — Overwatch ignores forced movement by default

Assert:
- target viene spinto nel cono fuori dalla fase Move;
- nessuna opportunity standard OW, salvo variante esplicita.

## PHASE-AE-006 — Brace vs Charge/Dodge interaction

Assert:
- Dash-phase action interagisce con braced cell/unit;
- applicata la policy Brace corrente;
- TurnLog spiega l'esito.

## PHASE-AE-007 — Brace does not globally cancel Dash

Assert:
- nemico usa Dodge lontano dalla zona braced;
- nessun effetto Brace.

## PHASE-AE-008 — Guard disables Move

Assert:
- piano `Guard + normal Move` rifiutato o normalizzato secondo validator canonico;
- reason code esplicito.

## PHASE-AE-009 — Guard disables Reaction

Assert:
- piano con Guard + Hero Reaction incompatibile;
- nessuna strong Reaction armata gratuitamente.

## PHASE-AE-010 — Interact allows Move, disables Reaction

Assert:
- `Interact + Move` legale;
- `Interact + Reaction` illegale;
- phase dell'interazione concreta rispettata.

## PHASE-AE-011 — One concrete action, one ResolvePhase

Validator:
- una ActionDefinition competitiva non può avere due macro-fasi autorevoli.

## PHASE-AE-012 — Determinism permutation

Assert:
- ordine di inserimento action/reaction definitions non cambia StateHash/LogHash.

---

# 9. EPIC / ISSUE ROADMAP FINO A v1.0

IMPORTANTE:

- questi sono **Epic/issue group di dominio proposti**, non numeri GitHub;
- prima cercare equivalenti esistenti;
- non creare una seconda roadmap parallela;
- rispettare la ladder globale corrente, da riverificare su `main`:

```text
v0.1 — Vertical Slice
v0.2 — Standard Game / Struttura e finestre
v0.3 — Information Warfare
v0.4 — Product Alpha / Operations
v0.5 — Online Foundation
v0.6 — Ability Runtime
v0.7 — Competitive Alpha / Dedicated
v0.8 — Beta / Balance
v0.9 — Release Candidate
v1.0 — Launch
```

- se esistono già Epic globali E40–E45 o equivalenti, **NON riusare quei numeri** per questo dominio: collegare le issue action-economy alle Epic globali corrette;
- se esiste E38 o Epic equivalente per economy/movement compatibility, estenderla invece di duplicarla;
- una stessa release può contenere più issue group di dominio senza creare una nuova release.

---

# EPIC / ISSUE GROUP AE-PHASE-v0.1 — Canonical Action Phase Grammar & Regression Safety

**Target release:** v0.1, solo se la release è ancora modificabile; altrimenti documentazione/decisione immediata + implementazione nella prima release aperta.  
**Priorità:** P0

## Goal

Bloccare la grammatica senza regressioni:

```text
Planning -> Prep -> Dash -> Blast -> Move -> Cleanup
```

con `Move` finale e singola `ResolvePhase` per azione concreta.

## Scope

- audit Action Economy owner spec;
- registrare terminologia `Dash phase` vs `Dodge action/movement`;
- validator singola phase;
- TurnLog phase reason;
- scenario base Dodge-before-Blast;
- documentazione e Wiki.

## Out of scope

- tuning numerico Dodge;
- multiplayer;
- perception avanzata;
- roster variants.

## Child Issues candidate

1. `actions-audit-current-phase-and-economy-owners`
2. `actions-rename-generic-dash-movement-to-dodge`
3. `actions-enforce-single-resolve-phase`
4. `actions-preserve-move-as-final-voluntary-phase`
5. `test-action-phase-order-and-dodge-before-blast`
6. `docs-action-phase-glossary-dash-vs-dodge`

## Acceptance criteria

- nessuna ActionDefinition competitiva generica usa `Dash` ambiguamente come phase e action name;
- Move risolve post-Blast;
- scenario golden verde;
- docs/Feature/Scenario/Wiki allineati;
- nessuna divergenza replay.

---

# EPIC / ISSUE GROUP AE-COMPAT-v0.2 — Action Compatibility, Dodge Cost & Prepared Defense

**Target release:** v0.2 — Standard Game / Struttura e finestre  
**Priorità:** P0  
**Probabile relazione:** consolidare/estendere E38 se equivalente.

## Goal

Rendere esplicita e data-driven la compatibilità fra Main, Move, Reaction e Dash-phase movement.

## Scope

- `PostDashMovePolicy` o equivalente;
- Guard: Move OFF, Reaction OFF;
- Interact: Move ON, Reaction OFF;
- Brace: commitment anti-Dash da riconciliare con Reaction framework;
- Overwatch: trigger Move-only;
- reason codes;
- Planning HUD disabled states;
- validator;
- scenarios PHASE-AE-002..010.

## Open questions da tracciare

- `BasicAttack + Reaction`;
- `Sprint + Reaction`;
- exact Dodge range;
- exact reduced Move;
- Brace main/reaction ownership finale;
- eccezioni character-specific.

## Child Issues candidate

1. `economy-data-driven-action-compatibility-contract`
2. `dodge-post-move-policy-none-reduced-normal`
3. `guard-disable-move-and-reaction`
4. `interact-allow-move-disable-reaction`
5. `brace-dash-interaction-contract`
6. `overwatch-filter-trigger-by-move-phase-kind`
7. `planning-ui-action-disable-reason-codes`
8. `test-compatibility-matrix-golden-suite`
9. `decision-basicattack-reaction-compatibility`
10. `decision-sprint-reaction-compatibility`

## Acceptance criteria

- compatibility non hard-coded in UI;
- server/local validator condividono la stessa regola logica;
- OW non scatta su Dodge/forced movement standard;
- Guard/Interact non conservano strong Reaction per default;
- packaged Standard Game test verde secondo gate di release.

---

# EPIC / ISSUE GROUP AE-INFO-v0.3 — Phase-Aware Reactions, Perception & Team Knowledge

**Target release:** v0.3 — Information Warfare  
**Priorità:** P1

## Goal

Integrare eligibility delle reazioni con visibilità/detection/noise senza trasformare `position changed` in leak o trigger universale.

## Scope

- OW eligibility = Move-phase + LOS/detection;
- TeamKnowledge-safe preview;
- hidden movement/stealth interaction;
- sound-triggered reactions separate da OW;
- certainty UI;
- sanitized ReactionOpportunity contract, anche se il trasporto network completo arriva dopo.

## Child Issues candidate

1. `reaction-phase-aware-eligibility-service`
2. `overwatch-los-detection-move-trigger`
3. `teamknowledge-reaction-preview-sanitization`
4. `sound-triggered-reaction-separate-from-overwatch`
5. `ui-certainty-for-phase-reaction-threats`
6. `test-hidden-dodge-vs-move-overwatch-information-rules`

## Acceptance criteria

- OW non usa enemy intent per predire trigger;
- stessa conoscenza autorizzata -> stessa preview;
- hidden state non viene promosso a knowledge;
- privacy contract pronto per Online Foundation.

---

# EPIC / ISSUE GROUP AE-MAP-v0.4 — Dash/Move Interaction with Multilayer Map & Environment

**Target release:** v0.4 — Product Alpha / Operations  
**Priorità:** P1

## Goal

Far rispettare la distinzione Dash/Dodge/Move su porte, ponti, tunnel, elevatori, livelli e hazard.

## Scope

- transition metadata per allowed phase/movement kind;
- Dodge/Charge/Blink legality su special edges;
- Move-only OW trigger su multilayer transitions;
- Brace interaction in choke point;
- environment reason codes;
- editor validation;
- forced movement semantics necessarie alle mappe/Operations.

## Child Issues candidate

1. `map-transition-allowed-movement-kind-and-phase`
2. `dodge-special-edge-legality`
3. `overwatch-move-trigger-on-doors-bridges-tunnels`
4. `brace-chokepoint-dash-interaction`
5. `forced-movement-kind-preserve-cause`
6. `editor-phase-movement-transition-validator`
7. `scenario-bridge-brace-vs-dodge`
8. `scenario-tunnel-overwatch-move-only`

## Acceptance criteria

- passability non implica reaction eligibility;
- path/LOS/targeting/reaction restano servizi separati;
- forced movement è distinguibile da voluntary Move/Dodge;
- GraphRevision/cache non altera determinismo;
- scenario multilayer verde.

---

# EPIC / ISSUE GROUP AE-ONLINE-v0.5 — Online Foundation for Action Phases & Reaction Compatibility

**Target release:** v0.5 — Online Foundation  
**Priorità:** P0

## Goal

Portare action compatibility, Dodge, Guard/Brace/Interact e Overwatch nel primo loop online autorevole senza leak.

## Scope

- canonical intent validation server-side;
- team-only action preview;
- reliable Ready/Commit;
- sanitized ReactionOpportunity;
- phase/movement-kind fields necessari al protocollo;
- standard OW ignores Dash/Forced Movement;
- network reason codes;
- packaged two-team privacy canary.

## Child Issues candidate

1. `net-action-intent-phase-validation`
2. `net-team-preview-action-compatibility-sanitized`
3. `net-ready-commit-action-plan-reliable`
4. `net-reaction-opportunity-move-phase-sanitization`
5. `net-dodge-guard-brace-overwatch-contract`
6. `net-privacy-canary-action-intents`
7. `packaged-two-team-action-phase-suite`

## Acceptance criteria

- zero planning avversario consegnato al client nemico;
- server rifiuta combinazioni invalide;
- timeout/esito Reaction resta server-authoritative;
- OW trigger eligibility viene rivalidata sullo stato autorevole;
- packaged canary verde.

---

# EPIC / ISSUE GROUP AE-ABILITY-v0.6 — Ability Runtime & Data-Driven Character Action Profiles

**Target release:** v0.6 — Ability Runtime  
**Priorità:** P0/P1 secondo owner E41 equivalente

## Goal

Integrare il contratto Action Phase con l'Ability Runtime/GAS mirror senza spostare l'autorità dal resolver.

## Scope

- Stable ActionId binding;
- character action profile;
- Dodge profile;
- Guard/Brace/Overwatch profile data;
- resolved events -> GAS lifecycle/mirror;
- no pure upgrades;
- validators;
- Skill Workbench / Character Setup integration se previsti dalla release/tool lane.

## Child Issues candidate

1. `ability-runtime-stable-actionid-phase-binding`
2. `character-action-profile-data-contract`
3. `character-dodge-profile`
4. `character-guard-brace-overwatch-profiles`
5. `gas-mirror-action-phase-resolved-events`
6. `validator-action-profile-invariants`
7. `skill-workbench-phase-and-compatibility-authoring`
8. `scenario-template-action-profile-matrix`

## Acceptance criteria

- GAS non decide legalità/esito competitivo;
- core resolver non branch-a su HeroId;
- varianti dichiarano trade-off;
- authoring invalido fallisce validator;
- determinism regression verde.

---

# EPIC / ISSUE GROUP AE-COMPETITIVE-v0.7 — Dedicated, Reconnect & Competitive Action Rules

**Target release:** v0.7 — Competitive Alpha / Dedicated  
**Priorità:** P0

## Goal

Hardening del sistema su dedicated server, reconnect/resync, spectator/replay e regole competitive stabili.

## Scope

- dedicated authoritative action loop;
- reconnect/resync phase/action state;
- late join/spectator visibility policy;
- replay contract per Dodge/Brace/Overwatch;
- traffic canary;
- abuse/rate-limit;
- packaged dedicated soak.

## Child Issues candidate

1. `dedicated-action-phase-authority`
2. `reconnect-resync-action-plan-and-reaction-state`
3. `spectator-action-information-policy`
4. `replay-dodge-brace-overwatch-contract`
5. `traffic-canary-action-intents-and-opportunities`
6. `abuse-rate-limit-reaction-responses`
7. `packaged-dedicated-action-phase-soak`

## Acceptance criteria

- reconnect non crea doppio commit/reaction;
- spectator non vede info non autorizzata;
- replay produce stesso StateHash/LogHash;
- dedicated soak verde;
- zero leak nel canary corpus.

---

# EPIC / ISSUE GROUP AE-BETA-v0.8 — Bot, Balance, Batch & Performance for Phase Commitments

**Target release:** v0.8 — Beta / Balance  
**Priorità:** P1

## Goal

Misurare se i trade-off Guard/Brace/Interact/Dodge/Overwatch creano scelte reali e non false choice.

## Scope

- bot legality + utility;
- metrics per usage;
- batch simulation;
- performance ReactionOpportunity;
- matchup matrix;
- no omniscience;
- soak/load.

## Metriche candidate

```text
Dodge pick rate
Dodge -> reduced Move conversion
Guard pick rate / prevented damage
Brace vs Dash encounter rate
Brace successful stop/mitigation rate
Interact exposure/death rate
Overwatch opportunity rate
Overwatch FIRE/HOLD rate
Move reroute/avoidance rate
Reaction unavailable by compatibility count
```

## Child Issues candidate

1. `bot-understand-phase-action-compatibility`
2. `bot-evaluate-dodge-vs-move-tradeoff`
3. `bot-evaluate-brace-vs-dash-threat`
4. `bot-evaluate-overwatch-move-threat`
5. `telemetry-action-phase-choice-metrics`
6. `batch-balance-phase-commitments`
7. `performance-reaction-trigger-filtering`
8. `soak-action-phase-high-density`

## Acceptance criteria

- bot usa solo conoscenza permessa;
- metriche machine-readable;
- nessun gate di balance basato solo su win-rate;
- performance entro budget corrente;
- nessuna false choice evidente senza issue di balance aperta.

---

# EPIC / ISSUE GROUP AE-RC-v0.9 — Rules Freeze, Accessibility & Full Regression

**Target release:** v0.9 — Release Candidate  
**Priorità:** P0

## Goal

Congelare la grammatica competitiva e impedire drift tra docs, data, UI, server e replay.

## Scope

- schema/ruleset freeze;
- compatibility matrix freeze;
- accessibility labels;
- reason code freeze;
- migration rehearsal;
- security/privacy regression;
- full canonical regression;
- issue/feature/scenario coverage audit.

## Child Issues candidate

1. `freeze-action-phase-schema-and-enums`
2. `freeze-action-compatibility-ruleset`
3. `freeze-reaction-trigger-eligibility`
4. `accessibility-action-disabled-reasons`
5. `regression-action-phase-full-corpus`
6. `migration-rehearsal-action-definitions`
7. `privacy-security-regression-action-system`
8. `traceability-audit-action-economy`

## Acceptance criteria

- nessun conflitto docs/data/code;
- full golden corpus verde;
- UI non dipende solo dal colore;
- schema versionato;
- ranked rules locked;
- nessun nuovo framework introdotto.

---

# EPIC / ISSUE GROUP AE-LAUNCH-v1.0 — Action Economy Launch Certification

**Target release:** v1.0 — Launch  
**Priorità:** P0

## Goal

Certificare l'intero sistema Action Phase / Dodge / Guard / Brace / Interact / Overwatch / Reaction per produzione.

## Scope

- release certification;
- production dedicated deployment check;
- privacy/security certification;
- final golden corpus;
- clean-machine build;
- replay audit;
- observability;
- rollback readiness;
- final docs/player wiki;
- balance final tuning only.

## Child Issues candidate

1. `certify-action-phase-golden-corpus`
2. `certify-zero-replay-divergence-action-system`
3. `certify-zero-intent-leak-action-system`
4. `certify-production-dedicated-actions-and-reactions`
5. `certify-observability-action-phase-reason-codes`
6. `certify-player-facing-action-language`
7. `certify-clean-machine-action-data-and-assets`
8. `certify-rollback-action-ruleset`
9. `release-signoff-action-economy`

## Release gate

```text
- zero replay divergence
- zero intent leak
- no unresolved P0/P1 phase/economy bugs
- action compatibility deterministic
- OW only triggers permitted movement kinds/phases
- Guard/Interact compatibility matches shipping ruleset
- Brace/Dash interaction has approved, tested payload
- production dedicated certification green
- player-facing terminology uses Dash=phase / Dodge=action consistently
- docs/wiki/reason codes match shipping behavior
```

---

# 10. DIPENDENZE FRA ISSUE GROUP DI DOMINIO

```text
AE-PHASE-v0.1
   -> AE-COMPAT-v0.2
      -> AE-INFO-v0.3
      -> AE-MAP-v0.4
         -> AE-ONLINE-v0.5
            -> AE-ABILITY-v0.6
               -> AE-COMPETITIVE-v0.7
                  -> AE-BETA-v0.8
                     -> AE-RC-v0.9
                        -> AE-LAUNCH-v1.0
```

Questa è una dipendenza logica del dominio, NON una nuova roadmap globale. Le issue devono essere agganciate alle Epic/milestone reali della release corrente.

# 11. TRACKING IMPACT PASS PER OGNI ISSUE

Per ogni Issue creata o modificata valutare obbligatoriamente:

1. Milestone / Epic;
2. Feature Registry / Feature Map;
3. Scenario Map;
4. Automation / Functional Test;
5. Editor Map / manual PIE;
6. Asset/icon/VFX necessari;
7. content/data definitions;
8. Wiki/docs;
9. ADR/Decision Log;
10. UI/UX;
11. debug/observability/TurnLog;
12. dipendenze.

Per ogni categoria:

```text
SEARCH -> LINK -> UPDATE -> CREATE only if missing -> N/A with reason
```

---

# 12. UI / HUD — COMPORTAMENTO ATTESO

La UI deve spiegare immediatamente perché un'azione viene spenta.

Esempi concettuali:

```text
GUARD SELECTED
- MOVE disabled: Guard commits movement
- REACTION disabled: Guard commits reaction capacity

INTERACT SELECTED
- MOVE available
- REACTION disabled: Interact occupies reactive readiness

DODGE SELECTED
- resolves in DASH
- final MOVE: Reduced / None according to profile

OVERWATCH SELECTED
- armed in PREP
- watches MOVE-phase transitions
- Dodge/Charge/Blink do not trigger standard Overwatch
```

Non mostrare semplicemente icone grigie senza reason.

Stati UI:

```text
Confermato
Previsto
Incerto
```

come da grammatica corrente del progetto.

---

# 13. DEBUG / OSSERVABILITY

Aggiungere/estendere strumenti per visualizzare:

```text
CurrentPhase
Action.ResolvePhase
MovementKind
PostDashMovePolicy
ReactionCompatibility
OverwatchTriggerEligibility
BraceControlArea / interaction boundary
ValidationReason
```

Comandi/overlay devono usare naming reale del repository.

Profilare:

- reaction trigger evaluations;
- path transitions;
- phase filtering;
- Decision Window count;
- TurnLog volume.

---

# 14. DEFINITION OF DONE DEL DOMINIO

Una modifica a queste regole è Done solo se:

1. è documentata in owner spec/Decision Log quando necessario;
2. è data-driven o centralizzata, non hard-coded in UI;
3. server/client applicano la stessa legalità;
4. privacy degli intenti è preservata;
5. TurnLog/reason code spiegano l'esito;
6. ha Automation/Functional Test pertinente;
7. ha scenario riproducibile;
8. replay con stesso snapshot/regole/seed non diverge;
9. funziona in packaged build;
10. tracking impact pass completo;
11. Wiki/UI usano terminologia `Dash phase` / `Dodge action` coerente;
12. non introduce un secondo resolver o una seconda source of truth.

---

# 15. OPEN DESIGN / OPEN BALANCE — NON INVENTARE

Lasciare esplicite almeno queste domande finché non vengono approvate:

1. Dodge è universale per tutti i personaggi?
2. Quante celle/base budget ha Dodge?
3. Qual è la formula del Move ridotto dopo Dodge/Charge/Blink?
4. Quali Dash-phase actions consentono `Normal` Move dopo?
5. `BasicAttack + Reaction` è legale per default?
6. `Sprint + Reaction` è legale per default?
7. Brace occupa definitivamente Main, Reaction o entrambe nel ruleset corrente?
8. Brace è automatico quando valido o apre Fast Reaction in alcuni profili?
9. Qual è il payload v0.x di Brace contro Charge/Dodge?
10. Guard è formalmente il counter principale di Blast oppure resta difesa generalista?
11. Quali Interact risolvono in Prep/Blast/Move?
12. Esistono profili Overwatch speciali autorizzati a reagire a Dash o Forced Movement?

Queste domande devono diventare Decision/Balance Issue, non branch sparsi nel codice.

---

# 16. REPORT FINALE OBBLIGATORIO PER CLAUDE

Dopo l'esecuzione, riportare:

```text
HEAD before
HEAD after
Engine version verified

Existing Epic reused/updated
New Epic created
Issues updated
Issues created
Real GitHub IDs/URLs

Decision Log / ADR changes
Owner spec changes
Feature Registry changes
Roadmap changes
Scenario Map changes
Editor Map changes
Wiki changes
Data/Balance changes
UI/UX changes
Debug/TurnLog changes

Tests run
Results
Packaged verification
Privacy tests
Replay/determinism results

Conflicts found
Conflicts resolved
Open design decisions
Open balance decisions
Blocked items

Next recommended issue
Suggested commit(s)
```

Non dichiarare test PASS senza output reale.

---

# 17. SHORT CANONICAL STATEMENT DA PROPAGARE

Usare come sintesi solo dopo averla riconciliata con `main`:

```text
RefactorTactics resolves planned actions through:
Planning -> Prep -> Dash -> Blast -> Move -> Cleanup.

Dash is a phase. The generic rapid movement inside that phase is Dodge.
Move is the final standard voluntary movement phase.
Each concrete action owns exactly one ResolvePhase.

Dodge/Dash-phase movement does not automatically trigger standard Overwatch.
Standard Overwatch controls valid movement transitions during Move.
Brace is the primary prepared counter to Dash-phase engage/displacement when the action interacts with the braced unit/control area.
Guard is a strong commitment: no Move and no Reaction.
Interact can coexist with Move but not with Reaction by default.
Reaction capacity is a strong tactical resource and is not implicitly free.

Exact Dodge budget, reduced post-Dodge Move, BasicAttack+Reaction, Sprint+Reaction and Brace payload remain explicit design/balance decisions until approved.
```

---

# 18. REVIEW GATE PRIMA DI APPLICARE

Prima di creare/aggiornare GitHub:

- [ ] verificare che `Dash` non venga usato ambiguamente come generic action;
- [ ] verificare che `Dodge` non venga confuso con Sprint;
- [ ] verificare Move post-Blast;
- [ ] verificare Overwatch Move-only baseline;
- [ ] verificare Guard = no Move + no Reaction;
- [ ] verificare Interact = Move yes + Reaction no;
- [ ] verificare Brace anti-Dash senza aura globale;
- [ ] verificare che forced movement non attivi OW standard per default;
- [ ] verificare ogni concrete action con una sola ResolvePhase;
- [ ] verificare OPEN_DESIGN non trasformati in decisioni arbitrarie;
- [ ] cercare duplicate Epic/Issue;
- [ ] completare Tracking Impact Pass.

