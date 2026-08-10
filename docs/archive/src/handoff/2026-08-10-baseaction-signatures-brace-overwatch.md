# RefactorTactics — Consolidamento Base Action Signatures, Brace e Overwatch
## Handoff operativo per Claude Code

> 📦 **Archiviato il 2026-08-10 — recepito, non applicato.** L'esito del triage vive in
> [`roadmap/plans/baseaction-signatures-spec-panel-2026-08-10.md`](../../../roadmap/plans/baseaction-signatures-spec-panel-2026-08-10.md),
> e le domande che ne restano aperte in [`OPEN_DECISIONS.md`](../../../OPEN_DECISIONS.md) come `BAS-1`…`BAS-5`.
> **Questo file non è autorità.** Tre dei suoi punti sono in conflitto con decisioni già consolidate — le otto
> azioni universali del §3.2 (`Activate` è assorbita da `Interact`, D-014 e D-025), «Brace non riduce il
> danno» del §10 (D-047: la risposta universale `Hold Ground` **riduce** −10, ed è in partita) e il
> vocabolario `Base Action Signature` del §4 (D-033: si chiama **profilo**) — e il §14 è superato
> dall'handoff pari-data sul lifecycle dell'Overwatch. Ciò che il triage ha tenuto sono le due tabelle dei
> profili d'eroe. Leggilo per la **provenienza**, mai per la regola.

**Data:** 2026-08-10  
**Scope:** personaggi v0.1, azioni universali, Base Action Signature, Brace, Overwatch, Character Reaction Profiles, Feature Map, Scenario Map, Wiki, Roadmap, Epic/Issue GitHub.  
**Roster v0.1 da preservare:** `Flux`, `Riva`, `Bastion`, `Vektor`.  
**Showcase v0.1:** `Flux + Riva` vs `Bastion + Vektor`.

---

# 0. OBIETTIVO

Consolidare nel repository quanto deciso/discusso sul fatto che le **azioni base siano universali ma caratterizzate per personaggio**.

Principio:

> Il giocatore impara una grammatica comune una volta sola, ma Flux deve continuare a sentirsi Flux, Riva deve continuare a sentirsi Riva, Bastion deve continuare a sentirsi Bastion e Vektor deve continuare a sentirsi Vektor anche quando le loro quattro signature ability sono indisponibili.

Le azioni base NON devono diventare altre 6-8 signature ability gratuite.

Usare tre livelli:

```text
STANDARD
VARIANT
SIGNATURE
```

- **STANDARD**: stessa semantica comune.
- **VARIANT**: stessa funzione tattica, payoff/geometria caratteristica.
- **SIGNATURE**: comportamento fortemente identitario, senza rompere la grammatica universale.

Target iniziale:

```text
max 1-2 Base Action fortemente SIGNATURE per personaggio
il resto STANDARD o VARIANT
```

I valori numerici di bilanciamento restano `PROPOSED/TBD` finché non approvati nei cataloghi/playtest.

---

# 1. PRIMA DI MODIFICARE: AUDIT OBBLIGATORIO

Eseguire:

```bash
git status
git branch --show-current
git rev-parse HEAD
```

Leggere prima, se presenti:

```text
CLAUDE.md
AGENTS.md
README.md
CONTEXT_INDEX.md

docs/product/piano-canonico-mvp.md
docs/product/showcase-v0.1.md

docs/decisions/RT_PDR_00_Decision_Log.md
docs/decisions/adr-0003-modello-azioni-v01.md
docs/decisions/adr-0004-finestre-di-reazione.md
docs/decisions/adr-0005-orientamento.md
docs/DOC_CONFLICT_MATRIX.md
docs/OPEN_DECISIONS.md

docs/gameplay/spec-sequenza-turno.md
docs/gameplay/spec-motore-azioni-e4.md
docs/gameplay/*
docs/technical/brief-planning-visuale.md

docs/balance/RT_ActionCatalog_v0.1.md
docs/balance/RT_HeroCatalog_v0.1.md
docs/balance/RT_TerrainCatalog_v0.1.md
docs/balance/RT_EquipmentCatalog_v0.1.md

docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-checkpoint.md
docs/roadmap/v0.1-definition-of-done.md
docs/roadmap/feature-registry.*
docs/roadmap/*

Scenarios/
Source/RefactorTactics/Turn/
Source/RefactorTactics/Tests/
Source/RefactorTactics/ScenarioHarness/
```

Cercare repository-wide e Wiki-wide:

```text
Wait
Move
BasicAttack
Basic Attack
Guard
Brace
Activate
Interact
Overwatch

Base Action Signature
Character Base Action
Reaction Profile
Overwatch Profile

Flux
Riva
Bastion
Vektor

Ground
Grounding
Flow
Anchor
Deflect
Deflection
Pressure Overwatch
Conductive Overwatch
Predictive Overwatch
Frontline Overwatch

ReactionOpportunity
Fast Reaction
FIRE
HOLD
BRACE
Forced Movement
Displacement

Sprint
Dash
Facing
Move phase
Prep
Blast

Feature.Reaction
Feature.Character
Scenario
Roadmap
Epic
Issue
```

NON fare search/replace cieco.

---

# 2. ORDINE DI PREVALENZA

Usare questa gerarchia:

```text
Decision Log / ADR più recenti
-> codice + cataloghi correnti
-> piano canonico MVP
-> Character/Action Master correnti
-> Feature Registry / Scenario Registry / Roadmap correnti
-> handoff recenti
-> PDR / workbook / research storici
```

Se trovi un conflitto tra questo handoff e una decisione PIÙ RECENTE nel repository:

1. NON sovrascrivere silenziosamente;
2. registra il conflitto;
3. conserva la decisione più recente come canone;
4. apri/aggiorna una decision issue se serve;
5. riporta il conflitto nel report finale.

---

# 3. CANONE DA PRESERVARE

## 3.1 Roster v0.1

```text
Flux
Riva
Bastion
Vektor
```

Showcase:

```text
Conflux:
- Flux
- Riva

Constrine:
- Bastion
- Vektor
```

Non reintrodurre come roster operativo:

```text
Aegis
Nyx
Drift
Vex
Mara
Ivo
Sol
Kairo
Morrow
Vela
Rook
```

Se compaiono in materiale storico, mantenerli come storico.

---

## 3.2 Azioni universali v0.1

Baseline da verificare nel Decision Log e, se ancora corrente, consolidare:

```text
Wait
Move
BasicAttack
Guard
Brace
Activate
Interact
Overwatch
```

Semantica:

```text
Wait       = rinuncia esplicita
Move       = movimento volontario standard
BasicAttack= attacco base character-specific
Guard      = protezione generale
Brace      = stance/reaction anti-displacement / geometry defense
Activate   = attivazione dispositivo/mappa
Interact   = interazione generica/obiettivo
Overwatch  = azione universale che prepara una reaction di controllo spazio
```

Non fondere `Guard` e `Brace`.

Non fondere `Activate` e `Interact` senza una nuova decisione esplicita.

---

## 3.3 Ordine macro-fasi

Preservare:

```text
Planning
-> Commit
-> Prep
-> Dash
-> Blast
-> Move
-> Cleanup
```

Regole:

- `Move` normale resta l'ultima azione volontaria standard.
- `Dash`, `Charge`, `Leap`, `Blink`, displacement e movement reattivo NON sono il normale Move.
- `Fast Action` e `Fast Reaction` sono Decision Boundary, non macro-fasi.
- `Sprint != Dash`.

---

# 4. CHARACTER BASE ACTION SIGNATURE

Ogni character deve poter esprimere un profilo data-driven almeno concettualmente per:

```text
Move / Movement Profile
Special Movement
Basic Attack
Guard
Brace
Overwatch
Activate / Interact affinity
Wait behavior solo se esplicito
Facing/orientation entro il canone
```

NON creare sottoclassi C++ per eroe.

Preferire:

```text
Universal Action Definition
+ Character BaseActionProfile
+ ReactionProfile
+ OverwatchProfile
+ EnvironmentAffinity
+ Data/Tags
= comportamento
```

Riutilizzare tipi e cataloghi esistenti.

---

# 5. BASE ACTION MATRIX v0.1

Questa matrice va consolidata come **direzione di playtest approvata**, senza inventare valori numerici.

| Azione | Flux | Riva | Bastion | Vektor |
|---|---|---|---|---|
| Wait | Standard | Standard | Standard | Standard |
| Move | Standard | Variant: Wet affinity | Standard | Standard |
| Basic Attack | Signature/Engine: elettrico setup | Variant/Setup: Wet + displacement | Utility/Emergency | Signature/Primary Weapon |
| Guard | Standard | Standard | Variant: protezione frontale | Standard |
| Brace | Variant: Grounding | Variant: Flow | Signature: Anchor | Variant: Deflection |
| Activate | Variant: electrical affinity | Variant: hydraulic affinity | Variant: structures | Standard |
| Interact | Standard | Variant: water/environment | Variant: structures/cover | Standard |
| Overwatch | Variant: Conductive | Signature: Pressure | Variant: Frontline | Signature: Predictive |

Vincolo:

> La differenziazione deve cambiare il “come” e il “perché”, non rendere ogni azione un sottosistema unico.

---

# 6. WAIT

Per v0.1:

```text
Wait = STANDARD per tutti
```

Nessun bonus nascosto.

NO:

```text
Flux gains Charge on Wait
Riva creates Water on Wait
Bastion Fortifies for free on Wait
Vektor gains Focus for free on Wait
```

salvo futura regola esplicitamente approvata e bilanciata.

---

# 7. MOVE

Profili comuni:

```text
Sneak
Normal
Sprint
```

## Flux

Per ora `STANDARD`.

La conduzione deve influenzare la scelta di path tramite il contesto della mappa, non generare automaticamente Charge solo camminando.

## Riva

`VARIANT — Wet Affinity`

Su Water/Wet:

- costo movimento migliore **oppure**
- penalità terreno ridotta.

Non applicare entrambe automaticamente nella prima iterazione.

Non fissare numeri senza catalogo/playtest.

## Bastion

`STANDARD`.

La sensazione di stabilità/peso deve derivare soprattutto da:

- Brace;
- Guard;
- structures;
- displacement resistance;
- special movement.

Non renderlo frustrante solo riducendo il Move di base senza evidenza di balance.

## Vektor

`STANDARD`.

La sua identità di mobilità/predizione deve vivere soprattutto in special movement, facing e prediction mechanics.

### Facing

Non introdurre pivot differenti per personaggio se confliggono con l'ADR Facing corrente.

Le vecchie matrici di pivot per eroe restano proposal finché non approvate.

---

# 8. BASIC ATTACK

`BasicAttack` è universale come categoria, character-specific come payload.

Pattern da preservare:

```text
PRIMARY WEAPON
ENGINE ATTACK
SETUP ATTACK
UTILITY / EMERGENCY ATTACK
```

Ogni personaggio deve avere almeno una situazione ripetibile in cui il Basic Attack è una scelta sensata.

## Flux — Engine/Setup Attack

Direzione:

```text
attacco elettrico lineare
danno non necessariamente massimo
interazione con Conductive / Wet
supporta la sua engine di Conduction
```

Non duplicare una seconda regola di conduction nel BasicAttack.

Usare primitive/effect esistenti.

## Riva — Setup Attack

Direzione:

```text
Pressure-style attack
danno basso/medio da bilanciare
Wet
piccolo displacement / Push
```

Obiettivo:

> Riva usa il Basic Attack anche per cambiare la geometria dello scontro.

## Bastion — Utility/Emergency

Direzione:

```text
attacco semplice
affidabile
payload minimo
non compete con Vektor come shooter
```

## Vektor — Primary Weapon

Direzione:

```text
buon attacco base
buon range/danno relativo
semplice e leggibile
scelta seria in molti turni
```

La sua identità speciale deve rendere più facile ottenere il tiro giusto, non rendere il Basic Attack irrilevante.

---

# 9. GUARD

Preservare:

```text
Guard != Brace
Guard != Counter
Guard != Intercept
Guard != Overwatch
```

## Flux
Standard.

## Riva
Standard.

## Bastion
`VARIANT — Directional/Frontal Guard`

Solo una variante leggera:

- protezione generale migliore dal fronte;
- niente Decision Window aggiuntiva;
- niente Intercept automatico;
- niente difesa signature gratis.

## Vektor
Standard.

---

# 10. BRACE — PRINCIPIO COMUNE

Brace resta:

> preparazione difensiva focalizzata su displacement / geometry defense.

Lifecycle:

```text
Planning
-> Brace armed
-> Forced Movement trigger
-> Reaction Opportunity
-> response / HOLD
-> server validation
-> outcome
```

Brace standard:

- NON riduce automaticamente il danno;
- NON riduce automaticamente Stun;
- NON è difesa universale;
- usa il Reaction Framework;
- `HOLD` rinuncia alla opportunity corrente e conserva la charge se la definition lo consente;
- effetto numerico preciso resta da playtestare.

La differenziazione per personaggio deve essere espressa tramite `Character Reaction Profile`.

---

# 11. BRACE PROFILES v0.1

## 11.1 Flux — Grounding Brace

Nome concettuale:

```text
Ground
Grounding
```

Trigger:

```text
incoming Push / Pull / Knockback / Forced Movement
```

Response:

```text
GROUND
HOLD
```

Effetto di design:

- mitiga/modifica il displacement secondo baseline Brace;
- se il contesto è realmente `Conductive` secondo regole già esistenti, può produrre una piccola interazione con Charge/Conduction.

Vincoli:

- NON trasformare ogni hit in Charge;
- NON dare riduzione danno generica;
- NON inventare numeri;
- usare la stessa primitive di Conduction/Charge già definita altrove.

Player question:

> Posso usare il terreno per stabilizzarmi e trasformare una pressione geometrica in setup?

---

## 11.2 Riva — Flow Brace

Trigger:

```text
incoming Forced Movement
```

Response:

```text
FLOW
HOLD
```

Direzione:

> Riva non è la migliore nel dire “non mi muovo”; è brava a trasformare dove finirà.

Proposta di playtest:

- il displacement avviene;
- può essere mitigato parzialmente;
- Riva può deviare la destinazione/traiettoria verso una direzione hex adiacente legalmente valida.

Non è:

- teleport;
- Move gratuito;
- Dash;
- completa immunità al Push.

Il resolver deve sempre rivalidare:

```text
destination
occupancy
edge validity
hazard rules
map bounds
```

Se una deviazione non è legale, quella scelta NON deve comparire tra le risposte autorizzate.

Player question:

> Posso assecondare la forza e finire in una posizione migliore di quella scelta dal nemico?

---

## 11.3 Bastion — Anchor Brace

Trigger:

```text
incoming Forced Movement
```

Response:

```text
ANCHOR
HOLD
```

Identità:

> Bastion è il riferimento del roster v0.1 per “tenere la posizione”.

Direzione:

- migliore anti-displacement dei quattro;
- possibile sinergia con facing/front sector se l'ADR Facing e i cataloghi lo consentono;
- particolarmente importante su objective, choke, edge/pericoli.

Vincoli:

- NON dare automaticamente riduzione generica di tutti i danni;
- NON renderlo contemporaneamente il miglior Overwatch del roster;
- numeri da playtest.

Player question:

> Questa posizione vale abbastanza da spendere la mia preparazione per restare qui?

---

## 11.4 Vektor — Deflection Brace

Trigger:

```text
incoming Forced Movement
```

Responses possibili, se legali:

```text
DEFLECT LEFT
DEFLECT RIGHT
HOLD
```

Direzione:

- Vektor non compete con Bastion sulla pura resistenza;
- converte/modifica parte della traiettoria in displacement laterale;
- mantiene la semantica di geometry defense.

Vincoli:

- non deve diventare Dodge universale agli attacchi;
- non deve annullare il displacement meglio di Bastion;
- mostrare solo risposte legalmente valide.

Player question:

> Posso cambiare la geometria finale dell'impatto senza fermarlo completamente?

---

# 12. MATRICE BRACE

| Character | Brace Profile | Identità |
|---|---|---|
| Flux | Grounding | trasforma condizione/terreno in setup |
| Riva | Flow | devia/asseconda |
| Bastion | Anchor | riduce/nega il displacement |
| Vektor | Deflection | cambia traiettoria |

Grammatica comune da preservare:

```text
Brace = qualcuno sta cercando di cambiare la mia posizione.
```

Il payoff cambia, il significato del pulsante no.

---

# 13. OVERWATCH — FRAMEWORK COMUNE

Overwatch resta azione universale che prepara una Reaction.

Costo-opportunità corrente da verificare e preservare se canonico:

```text
Attack OR Ability OR Overwatch
```

Lifecycle base:

```text
Planning
-> Overwatch Intent
-> Prep: ARM
-> Resolution movement boundaries
-> EnemyEnterControlledArea
-> Reaction Opportunity
-> FIRE/character response or HOLD
-> validation
-> commit/continue
```

Baseline v0.1 già esistente da preservare:

```text
1 charge
Fast Reaction circa 3s
HOLD non consuma
timeout = HOLD
MaxPromptsPerReaction data-driven
baseline playtest = 3
```

Trigger valutato sui micro-step reali.

Se più target entrano nello stesso micro-step:

```text
una sola Reaction Opportunity
con tutti i target validi
```

NO prompt sequenziali artificiali dipendenti da iterazione.

NO leak di future opportunity.

NO invio di path/intenti futuri nemici.

---

# 14. OVERWATCH LIFECYCLE E MOVE

Consolidare come regola di playtest v0.1, salvo conflitto più recente esplicito:

> Overwatch rappresenta il commitment del personaggio a controllare uno spazio durante quel tratto della Resolution; quando arriva il normale Move personale, l'Overwatch è terminata/disarmata.

Lifecycle:

```text
Planning
-> Prep: ARM OVERWATCH
-> Dash / Blast / movement events possono generare opportunity
-> prima del normale Move personale: DISARM
-> Move finale eventualmente limitato
```

Direzione di playtest:

```text
dopo Overwatch:
Sneak   = consentito
Normal  = consentito ma con budget ridotto
Sprint  = non consentito
```

IMPORTANTE:

- NON inventare ora il valore esatto del budget ridotto;
- se il data model non lo supporta, creare issue dedicata;
- se il repository ha una decisione più recente diversa, registrare conflitto e non forzare questa regola.

Scopo:

> Overwatch significa “per questo turno mi concentro su questa contromossa”, ma non immobilizza necessariamente il personaggio per tutto il turno.

---

# 15. OVERWATCH PROFILES v0.1

## 15.1 Flux — Conductive Overwatch

Geometria:

```text
medium sector / directional
```

Response:

```text
DISCHARGE <valid target>
HOLD
```

Payload:

- usa la stessa primitive di attacco elettrico/conduction già definita;
- se target/cella/sistema è `Wet` o `Conductive`, applica le normali regole di conduction;
- NON creare una seconda implementation della chain elettrica solo per Overwatch.

Player question:

> Ho preparato una zona in cui l'elettricità avrà payoff sufficiente?

Team synergy:

```text
Riva creates Wet
-> Flux arms Conductive Overwatch
-> enemy enters
-> discharge
-> normal conduction resolution
```

---

## 15.2 Riva — Pressure Overwatch

Geometria:

```text
medium-short sector
```

Response:

```text
PUSH <valid target>
HOLD
```

Direzione payload:

```text
small/zero damage TBD
Wet
Push 1 candidate
```

NON bloccare i numeri senza playtest.

Obiettivo primario:

> alterare la destinazione e rompere il piano geometrico del nemico.

Esempi di payoff:

- togliere da cover;
- spingere fuori objective;
- mandare verso acqua/hazard;
- preparare Flux;
- rompere una traiettoria se le regole del resolver lo consentono.

Questa deve essere la Base Action Signature più forte di Riva insieme al suo setup Water/Wet.

---

## 15.3 Bastion — Frontline Overwatch

Geometria:

```text
short
wide
frontal
```

Payload:

```text
semplice attacco/fallback
eventuale suppression SOLO se esiste già una primitive/catalogo approvata
```

Non aggiungere suppression solo per riempire il design.

Identità:

> Bastion controlla bene un choke vicino perché costruisce/canalizza lo spazio, non perché ha il miglior Overwatch numerico.

Synergy:

```text
Bastion structure / cover / edge control
-> restringe le rotte
-> Frontline Overwatch acquista valore
```

Non renderlo contemporaneamente il miglior Brace e il miglior Overwatch.

---

## 15.4 Vektor — Predictive Overwatch

Geometria:

```text
narrow
long
line/corridor oriented
```

Response:

```text
INTERCEPT <valid target>
HOLD
```

Identità:

> Gli altri controllano una zona; Vektor controlla una traiettoria.

Trade-off:

- controllo spaziale più stretto;
- payoff migliore contro movimento prevedibile;
- può whiffare se la previsione/line control non viene sfruttata;
- il whiff deve restare trade-off reale del personaggio.

NON confondere con una eventuale `Predictive Action` pianificata completamente in Planning.

Distinzione:

```text
Predictive planned action:
"credo che passerai QUI"
-> nessuna nuova scelta live al payoff

Overwatch:
"controllo QUESTA LINEA"
-> se un target realmente entra, Fast Reaction
```

---

# 16. MATRICE OVERWATCH

| Character | Geometria | Payload | Identità |
|---|---|---|---|
| Flux | medium sector | Electric / Conduction | environment combo |
| Riva | medium-short | Push + Wet | displacement/control |
| Bastion | short-wide frontal | simple frontline shot | presidio/choke |
| Vektor | narrow-long | intercept | prediction |

Player fantasy sintetico:

```text
Flux:
"C'è una rete conduttiva qui."

Riva:
"Non voglio che tu finisca dove avevi pianificato."

Bastion:
"Questa porta/choke è sotto controllo."

Vektor:
"So da dove passerai."
```

---

# 17. ACTIVATE / INTERACT AFFINITY

NON modificare action economy solo per differenziare i personaggi.

Differenziare prima di tutto **quali affordance sono disponibili o più interessanti**.

## Flux

Affinità:

```text
generatori
pannelli elettrici
powered devices
conductive systems
```

## Riva

Affinità:

```text
valvole
pompe
water flow
hydraulic elements
```

## Bastion

Affinità:

```text
cover
doors
barricades
bridges
structures
```

## Vektor

Standard salvo kit specifico approvato.

Usare capability/tag/data esistenti.

NO branch `if Hero == Flux`.

---

# 18. BUDGET D'IDENTITÀ

Direzione di design:

| Character | Base Action identity primaria | secondaria | supporto |
|---|---|---|---|
| Flux | Basic Attack | Conductive systems | Brace/Overwatch conditions |
| Riva | Overwatch | Basic Attack | Move/Interact |
| Bastion | Brace | Guard/Architecture | Overwatch |
| Vektor | Overwatch | Basic Attack | Brace |

Obiettivo:

- Bastion non deve dominare anche Overwatch;
- Vektor non deve diventare tank solo per la sua Brace;
- Riva deve vincere soprattutto via geometry control;
- Flux deve usare environment/conduction, non bonus gratuiti.

---

# 19. DATA MODEL — MODIFICA MINIMA SCALABILE

Prima verifica i tipi già presenti.

Deve essere possibile rappresentare concettualmente:

```text
CharacterDefinition
  BaseActionProfile
  MovementProfile
  GuardProfile
  BraceReactionProfile
  OverwatchProfile
  InteractionCapabilities
  EnvironmentAffinity
```

Possibili campi concettuali, NON nomi API obbligatori:

```text
ProfileId
CharacterId
BaseActionId
GeometryProfileId
ReactionDefinitionId
PayloadEffectIds
Requirements
AllowedResponses
FacingPolicy
MovementAfterUsePolicy
Tags
```

Preferire riferimenti a definition/effect riutilizzabili.

NON creare:

```text
UFluxBraceComponent
URivaBraceComponent
UBastionBraceComponent
UVektorBraceComponent
```

se il comportamento può essere data-driven.

---

# 20. GAMEPLAY TAGS — RIUTILIZZARE PRIMA DI AGGIUNGERE

Verificare i tag esistenti prima di aggiungerne.

Concettualmente potrebbero essere necessari:

```text
Action.Wait
Action.Move
Action.BasicAttack
Action.Guard
Action.Brace
Action.Activate
Action.Interact
Action.Overwatch

Reaction.Brace
Reaction.Overwatch

ReactionProfile.Grounding
ReactionProfile.Flow
ReactionProfile.Anchor
ReactionProfile.Deflection

OverwatchProfile.Conductive
OverwatchProfile.Pressure
OverwatchProfile.Frontline
OverwatchProfile.Predictive
```

Usare la tassonomia reale del repo.

NON creare duplicati semantici.

---

# 21. TURNLOG / EXPLAINABILITY

Il TurnLog deve poter spiegare:

```text
ActionDeclared
ReactionArmed
ReactionOpportunityCreated
ReactionResponseHeld
ReactionCommitted
ReactionExpired
ReactionCancelled

ForcedMovementDeclared
BraceModifiedDisplacement

OverwatchTargetEntered
OverwatchPayloadApplied
OverwatchDisarmedBeforeMove
MovementBudgetModifiedAfterOverwatch
```

Usare eventi/reason code reali se equivalenti esistono.

Esempi di explainability:

```text
Bastion ANCHOR:
Push 2 -> Push 0/1 because BraceProfile.Anchor

Riva FLOW:
Push direction N -> NE because selected legal Flow response

Vektor DEFLECT LEFT unavailable:
destination blocked

Flux Conductive Overwatch:
target entered sector -> DISCHARGE -> Wet network produced normal conduction chain
```

Il client non deve ricalcolare la spiegazione: deve riprodurre dati autorizzati del TurnLog.

---

# 22. FEATURE MAP / FEATURE REGISTRY

NON creare una seconda Feature Map.

Aggiornare quella esistente.

Prima cercare feature equivalenti.

Aggiungere/estendere almeno concettualmente:

```text
Feature.Character.BaseActionSignature
Feature.Character.BaseActionProfile
Feature.Character.InteractionAffinity

Feature.Reaction.Brace
Feature.Reaction.Brace.CharacterProfiles

Feature.Reaction.Overwatch
Feature.Reaction.Overwatch.CharacterProfiles
Feature.Reaction.Overwatch.PostUseMovement

Feature.Character.Flux.BaseActions
Feature.Character.Riva.BaseActions
Feature.Character.Bastion.BaseActions
Feature.Character.Vektor.BaseActions
```

Se il registry usa ID diversi, usare quelli reali.

Relazioni:

```text
BaseActionSignature
 -> Common Actions
 -> Character Data
 -> Reaction System
 -> Facing
 -> Environment
 -> TurnLog
 -> UI Preview
```

```text
Brace Character Profiles
 -> Forced Movement
 -> ReactionOpportunity
 -> DecisionWindow
 -> Facing
 -> TurnLog
```

```text
Overwatch Character Profiles
 -> ReactionOpportunity
 -> LOS/Detection
 -> movement micro-step
 -> Facing
 -> Environment
 -> privacy/network
```

Ogni feature deve collegare:

- Wiki page;
- roadmap item / Epic / Issue;
- scenario coverage;
- implementation status;
- milestone;
- dipendenze.

---

# 23. SCENARIO MAP / SCENARIO REGISTRY

NON creare una seconda Scenario Map.

Aggiornare quella esistente e riusare naming/schema corrente.

Aggiungere o consolidare:

## CHARACTER / BASE ACTION

### CHAR-BASE-001 — Flux Conductive Overwatch

Setup:

```text
Riva crea Wet in un choke
Flux arma Overwatch
nemico entra
Flux sceglie DISCHARGE
```

Assert:

- opportunity valida;
- conduction usa primitive standard;
- ordine stabile;
- TurnLog spiega chain;
- nessun future leak.

Features:

```text
Flux
Riva
Overwatch
Wet
Conduction
ReactionOpportunity
```

---

### CHAR-BASE-002 — Riva Pressure Overwatch

Setup:

```text
enemy path A -> B -> C -> D
enemy entra nel settore Riva a C
Riva sceglie PUSH
```

Assert:

- Push modifica lo stato reale;
- path/resolution successiva usa la nuova posizione;
- nessuna posizione “ghost prevista” viene trattata come autorevole;
- TurnLog spiega displacement.

---

### CHAR-BASE-003 — Bastion Anchor Brace

Setup:

```text
Bastion vicino a objective/edge
Forced Movement incoming
ANCHOR
```

Assert:

- anti-displacement applicato;
- nessuna riduzione danno generica se non esplicitamente prevista;
- determinismo;
- reason code leggibile.

---

### CHAR-BASE-004 — Riva Flow Brace

Setup:

```text
incoming Push
due deviazioni possibili
FLOW
```

Assert:

- solo risposte legali visibili;
- deviazione deterministica;
- destinazione rivalidata;
- no teleport semantics.

---

### CHAR-BASE-005 — Vektor Deflection Brace

Setup:

```text
Forced Movement
LEFT legal
RIGHT blocked
```

Assert:

- UI/opportunity contiene solo LEFT + HOLD;
- server rifiuta eventuale RIGHT stale/forzato;
- TurnLog corretto.

---

### CHAR-BASE-006 — Vektor Predictive Overwatch

Setup:

```text
enemy A entra -> HOLD
enemy B entra successivamente -> INTERCEPT/FIRE
```

Assert:

- HOLD non consuma;
- nessun leak che esisterà enemy B;
- seconda opportunity generata solo quando realmente avviene;
- charge consumata una volta.

---

### CHAR-BASE-007 — Bastion Frontline Overwatch + Architecture

Setup:

```text
Bastion restringe un choke tramite struttura/cover già prevista
arma Frontline Overwatch
enemy entra frontalmente
```

Assert:

- vantaggio deriva dalla geometria reale della mappa;
- nessun bonus nascosto;
- facing coerente.

---

### CHAR-BASE-008 — Overwatch Ends Before Own Move

Setup:

```text
character arms Overwatch
resolution genera/no trigger
arriva il normale Move personale
```

Assert:

- Overwatch disarmata prima del normale Move;
- nessun trigger dopo disarm;
- Sprint non disponibile se regola ancora approvata;
- Normal usa policy/budget ridotto data-driven;
- nessun numero hard-coded fuori catalogo.

---

### CHAR-BASE-009 — Guard vs Brace

Assert:

```text
Guard = protezione generale
Brace = anti-displacement/reaction geometry
```

Il test deve impedire regressioni in cui i due diventano alias.

---

### CHAR-BASE-010 — Basic Attack Identity

Fixture con i quattro character.

Assert concettuale/data:

```text
Flux -> Engine/Setup
Riva -> Setup/Wet
Bastion -> Utility/Emergency
Vektor -> Primary
```

Non serve asserire balance numerico finché non approvato.

---

# 24. TEST AUTOMATICI MINIMI

Usare framework reali già presenti.

## Core / resolver

- stessa snapshot + stessi response -> stesso log/state;
- permutazione ordine unità -> stesso risultato;
- Brace profile non dipende da Tick;
- risposta illegale di Flow/Deflect -> reject;
- Bastion Anchor non applica generic damage reduction per errore;
- Overwatch character profile usa stessa ReactionOpportunity infrastructure;
- HOLD non consuma se definition lo dichiara;
- multi-target same micro-step -> una opportunity aggregata;
- Overwatch disarm prima del proprio Move;
- Move-after-Overwatch policy data-driven;
- BasicAttack profile non richiede branch per HeroId nel TurnManager/Resolver.

## UI / Functional

- label/profile corretti;
- risposte illegali non mostrate;
- facing/sector ghost coerente;
- `Confermato / Previsto / Incerto` preservati;
- no overload visuale.

## Network quando applicabile

- opportunity solo al client/team autorizzato;
- nessun enemy future path/intent leak;
- stale OpportunityId rejected;
- risposta non autorizzata rejected;
- packaged canary test.

---

# 25. UI / WIKI

## Wiki — Generic Actions

Aggiornare la pagina che descrive le azioni comuni:

```text
Wait
Move
BasicAttack
Guard
Brace
Activate
Interact
Overwatch
```

Spiegare chiaramente:

```text
Universal action != identical payload
```

Aggiungere il concetto:

```text
Base Action Signature
STANDARD / VARIANT / SIGNATURE
```

Non duplicare numeri competitivi posseduti dai cataloghi/Data Assets.

---

## Wiki — Brace

Aggiornare:

```text
Brace = geometry/displacement defense
```

Mostrare i 4 profili v0.1:

```text
Flux    -> Grounding
Riva    -> Flow
Bastion -> Anchor
Vektor  -> Deflection
```

Segnalare numeri come playtest/TBD.

---

## Wiki — Overwatch

Spiegare framework comune e 4 profili:

```text
Flux    -> Conductive
Riva    -> Pressure
Bastion -> Frontline
Vektor  -> Predictive
```

Aggiungere distinzione:

```text
Predictive planned action != Predictive Overwatch
```

Aggiungere lifecycle post-use se approvato dal Decision Log:

```text
Overwatch ends before own normal Move
Sprint unavailable
Normal reduced
```

senza inventare il valore numerico del reduced movement.

---

## Wiki — pagine personaggio

Aggiornare pagine:

```text
Flux
Riva
Bastion
Vektor
```

Aggiungere blocco `Base Action Signature` con:

- Basic Attack identity;
- Guard;
- Brace;
- Overwatch;
- Movement affinity;
- Activate/Interact affinity;
- player question;
- counterplay;
- scenario links;
- roadmap/feature links.

---

# 26. DOCUMENTAZIONE DA AGGIORNARE

Auditare e aggiornare, dove esistono:

1. Decision Log / ADR;
2. Common Actions Master / action model;
3. Reaction System Master;
4. Character/Roster Master;
5. gameplay spec azioni;
6. gameplay spec turno/resolution;
7. balance Action Catalog;
8. balance Hero Catalog;
9. eventuale Character Definition / Action Definition authoring;
10. Feature Map / Feature Registry;
11. Scenario Map / Scenario Registry;
12. Roadmap;
13. Definition of Done se serve;
14. product/showcase v0.1;
15. Wiki;
16. test plan;
17. changelog / conflict matrix;
18. issue/epic plan.

NON creare una nuova fonte di verità parallela.

---

# 27. DECISION LOG / ADR

Aggiungere/aggiornare decisioni, usando gli ID reali disponibili.

NON inventare ADR number prima di aprire il Decision Log.

Registrare almeno:

## Decisione A — Base Action Signature

```text
Le azioni universali mantengono semantica comune.
Ogni character può avere Standard/Variant/Signature profile data-driven.
Max 1-2 base actions fortemente Signature per hero come guideline iniziale.
```

## Decisione B — Brace Character Profiles

```text
Brace resta geometry/displacement defense.
Flux    -> Grounding
Riva    -> Flow
Bastion -> Anchor
Vektor  -> Deflection
```

Numeri `TBD/playtest`.

## Decisione C — Overwatch Character Profiles

```text
Flux    -> Conductive
Riva    -> Pressure
Bastion -> Frontline
Vektor  -> Predictive
```

## Decisione D — Overwatch lifecycle

Se non esiste conflitto più recente:

```text
Overwatch disarma prima del proprio normale Move.
Post-Overwatch:
- Sneak allowed
- Normal allowed with reduced budget
- Sprint not allowed
```

Il valore del reduced budget resta OPEN BALANCE.

Se il repo ha già una decisione diversa, non modificarla automaticamente: aprire conflict/decision issue.

---

# 28. ROADMAP

NON creare una roadmap parallela.

Consolidare nel modello milestone esistente.

Distribuzione consigliata, da adattare alle milestone reali:

## F1 — Rete privata / Reaction transport

Solo aspetti necessari a:

- ReactionOpportunity team/owner-safe;
- anti-leak;
- validazione response;
- packaged privacy test.

Non spostare qui balance/character payload se non necessario.

## F2 — Abilities / Character mechanics

Natural home per:

- Character Base Action Profile;
- per-character Basic Attack;
- Brace Reaction Profiles;
- Overwatch Profiles;
- data-driven payload;
- Hero/Action Catalog;
- resolver integration;
- TurnLog;
- scenario core.

## F3 — Map / Environment

Dipendenze:

- Wet / Water;
- Conduction;
- structures/cover;
- interactive devices;
- movement displacement;
- facing/LOS.

## F4 — Vertical Slice

Exit gate:

- quattro character con Base Action Signature leggibili;
- 2v2 showcase;
- scenario coverage;
- UI;
- explainability;
- playtest;
- no false choice;
- counterplay.

## F5 — Dedicated

- packaged network/privacy;
- replay audit;
- determinism;
- soak;
- telemetry.

Se le milestone reali hanno nomi diversi, mappare senza crearne di nuove.

---

# 29. EPIC GITHUB DA CREARE/AGGIORNARE

Prima:

1. elencare Epic/Issue esistenti;
2. cercare duplicati;
3. riutilizzare Epic esistenti quando coprono lo stesso scope;
4. preservare milestone/label reali;
5. usare relazioni parent/child o linked issue supportate dal repository;
6. NON inventare label se esistono equivalenti.

Se non esiste già un Epic equivalente, creare:

## EPIC — Character Base Action Signatures v0.1

Scope:

- common grammar;
- character profiles;
- Action/Hero data;
- Basic Attack identities;
- Guard/Brace distinction;
- Activate/Interact affinities;
- Overwatch profiles;
- Wiki;
- scenarios;
- tests;
- roadmap integration.

Acceptance:

- tutti i 4 personaggi leggibili anche senza signature ability;
- niente branch per HeroId nei sistemi core se evitabile;
- feature/scenario/wiki/roadmap linkati;
- playtest scenario automatico disponibile.

---

## EPIC — Brace Character Reaction Profiles v0.1

Se `Reaction System` Epic già esiste, preferire SUB-EPIC / child work / issue group sotto quello invece di duplicare.

Scope:

```text
Grounding
Flow
Anchor
Deflection
legal response filtering
TurnLog
UI
determinism
```

---

## EPIC — Overwatch Character Profiles v0.1

Se `Overwatch` Epic già esiste, estenderlo.

Scope:

```text
Conductive
Pressure
Frontline
Predictive
post-use Move lifecycle
environment interaction
UI
privacy
tests
```

---

# 30. ISSUE PLAN

Creare o aggiornare issue reali, senza duplicati.

Titoli suggeriti; adattare naming convention del repo.

## Common/Data

```text
feat(character): add data-driven Base Action Profile to character definitions
feat(actions): expose BasicAttack role/profile per character
feat(actions): preserve Guard vs Brace semantics in catalog and resolver
feat(actions): add interaction capability/affinity metadata where needed
docs(actions): document Standard/Variant/Signature base-action grammar
```

## Brace

```text
feat(reaction): support character-specific Brace Reaction Profiles
feat(reaction): implement Flux Grounding Brace profile
feat(reaction): implement Riva Flow Brace legal displacement redirection
feat(reaction): implement Bastion Anchor Brace profile
feat(reaction): implement Vektor Deflection Brace profile
feat(ui): filter Brace Decision Window to legal responses only
test(reaction): add deterministic Brace profile coverage
```

## Overwatch

```text
feat(overwatch): support data-driven character Overwatch Profiles
feat(overwatch): implement Flux Conductive Overwatch payload
feat(overwatch): implement Riva Pressure Overwatch displacement payload
feat(overwatch): implement Bastion Frontline Overwatch geometry
feat(overwatch): implement Vektor Predictive Overwatch corridor profile
feat(overwatch): disarm Overwatch before own normal Move
feat(move): support data-driven post-Overwatch movement policy
test(overwatch): add character-profile and lifecycle scenarios
test(net): add Overwatch opportunity privacy canary coverage
```

## Docs/Registry/Wiki

```text
docs(character): update v0.1 Base Action Signature matrix
docs(reaction): consolidate Brace and Overwatch character profiles
docs(wiki): update Generic Actions, Brace, Overwatch and four hero pages
docs(feature): link Base Action Signature features to roadmap/issues/scenarios
docs(scenario): register CHAR-BASE scenarios
docs(roadmap): consolidate Base Action work into existing milestones
```

---

# 31. ISSUE RELATIONS / DEPENDENZE

Impostare dipendenze reali, se il sistema GitHub/Project del repo le supporta.

Indicativamente:

```text
BaseActionProfile data model
  -> per-character Basic Attack
  -> Brace Profiles
  -> Overwatch Profiles

ReactionOpportunity
  -> Brace Profiles
  -> Overwatch Profiles

Forced Movement
  -> Riva Flow
  -> Bastion Anchor
  -> Vektor Deflection

Wet/Water
  -> Riva BasicAttack
  -> Riva Pressure OW
  -> Flux Conductive OW

Conduction
  -> Flux BasicAttack
  -> Flux Conductive OW

Facing
  -> Bastion Frontline OW
  -> Vektor Predictive OW
  -> Brace directional behavior if approved

Structures/Cover
  -> Bastion Base Action identity
  -> Bastion/Vektor team synergy

Post-Overwatch movement policy
  -> Overwatch lifecycle
  -> Move UI
  -> scenario CHAR-BASE-008
```

---

# 32. ROADMAP CONSOLIDATION

Dopo la creazione/aggiornamento delle issue:

1. collegare ogni issue alla milestone corretta;
2. collegare al relativo Epic;
3. aggiornare la Roadmap canonica con link alle issue;
4. aggiornare Feature Registry con link all'Epic/issue;
5. aggiornare Scenario Registry con `FeatureIds` e issue di implementazione;
6. aggiornare Wiki con link a Feature e Scenario, dove il generatore lo supporta;
7. evitare liste manuali duplicate se il repository genera le viste dai registry;
8. aggiornare roadmap checkpoint/status.

Il risultato atteso è:

```text
Feature
 -> Epic
 -> Issues
 -> Milestone/Roadmap
 -> Scenarios
 -> Wiki
 -> Tests
```

navigabile in entrambe le direzioni dove l'infrastruttura lo consente.

---

# 33. GITHUB — CREAZIONE OPERATIVA

Se `gh` è disponibile e autenticato:

```bash
gh auth status
gh repo view
gh issue list --state all --limit 200
```

Prima di creare ogni issue, cercare per titolo/parole chiave.

Usare:

```text
existing issue -> update/extend
missing issue  -> create
```

NON creare doppioni solo perché il wording è diverso.

Per ogni issue includere:

```text
Context
Problem
Scope
Out of scope
Acceptance criteria
Dependencies
Feature IDs
Scenario IDs
Docs/Wiki affected
Tests
Milestone
Epic/parent
```

Se il repo usa GitHub Projects o una convenzione Epic custom, rispettarla.

Non inventare campi Project inesistenti.

---

# 34. ACCEPTANCE CRITERIA GLOBALI

Il consolidamento è Done quando:

1. esiste una sola grammatica corrente delle 8 azioni universali, se confermata dal Decision Log;
2. Guard e Brace sono distinti ovunque;
3. Base Action Signature è documentata e data-driven;
4. Flux/Riva/Bastion/Vektor hanno profilo base leggibile;
5. BasicAttack usa i quattro pattern previsti senza numeri inventati;
6. Brace usa Grounding/Flow/Anchor/Deflection;
7. Overwatch usa Conductive/Pressure/Frontline/Predictive;
8. Overwatch usa lo stesso ReactionOpportunity framework;
9. Overwatch non crea implementazioni parallele di Water/Conduction/Displacement;
10. lifecycle post-Overwatch è documentato o esplicitamente lasciato OPEN se c'è conflitto;
11. TurnLog spiega gli esiti;
12. UI mostra solo scelte legali;
13. privacy preservata;
14. Feature Map aggiornata;
15. Scenario Map aggiornata;
16. Wiki aggiornata;
17. Roadmap aggiornata;
18. Epic/Issue create o consolidate senza duplicati;
19. link fra feature/scenario/issue/wiki/roadmap validi;
20. test automatici pertinenti esistono o sono schedulati con issue;
21. nessun vecchio roster viene promosso accidentalmente;
22. nessun valore numerico `TBD` viene canonizzato senza evidenza.

---

# 35. OUT OF SCOPE

NON introdurre in questo lavoro:

```text
nuovi personaggi v0.2
Super Actions
progressione
modding pubblico
matchmaking
nuova action economy globale
nuova Reaction Clash grammar
nuovo Time Bank definitivo
nuovo sistema GAS parallelo
nuovo resolver
nuovo movement engine
nuova roadmap
nuovo Feature Registry
nuova Scenario Map
```

Se serve una dipendenza fuori scope, creare/linkare issue e lasciare il comportamento bloccato o `TBD`.

---

# 36. REPORT FINALE RICHIESTO A CLAUDE

Al termine fornire:

## Audit

```text
branch
HEAD iniziale/finale
working tree
file letti
conflitti trovati
decisioni applicate
decisioni lasciate OPEN
```

## File modificati

Per ciascun file:

```text
path
motivo
tipo modifica
```

## Feature Map

```text
feature aggiunte/aggiornate
status
dipendenze
issue links
scenario links
wiki links
```

## Scenario Map

```text
scenario aggiunti/aggiornati
FeatureIds
CharacterIds
milestone
test status
```

## GitHub

```text
Epic create/updated
Issue create/updated
Issue duplicate evitate
parent/child relations
milestone
labels
dependencies
URL/number
```

## Roadmap

Mostrare come le nuove issue sono state consolidate nelle milestone esistenti.

## Test

```text
test eseguiti
risultati
test non eseguibili
reason
```

## Open decisions

Elencare solo decisioni realmente non risolte.

Possibili candidate:

```text
valore esatto movement budget dopo Overwatch
valore numerico Brace resistance
Riva Flow exact displacement rule
Flux Grounding Charge amount/condition
Riva Pressure OW damage/no-damage
Bastion Frontline secondary effect
Vektor Predictive Overwatch exact geometry
```

NON inventare valori per chiuderle.

---

# 37. COMMIT SUGGERITI

Adattare alla reale struttura del repo.

Possibile sequenza:

```text
docs(actions): consolidate character base action signatures
docs(reaction): define v0.1 brace and overwatch profiles
docs(registry): link base actions to features and scenarios
docs(roadmap): consolidate character base action work
test(scenarios): add v0.1 base-action reaction fixtures
chore(github): sync epics and issues with roadmap
```

Se vengono fatte modifiche al codice:

```text
feat(character): add data-driven base action profiles
feat(reaction): support character brace profiles
feat(overwatch): support character overwatch profiles
test(reaction): cover v0.1 character reaction variants
```

Tenere i commit focalizzati.

---

# 38. RISULTATO ATTESO

A consolidamento concluso deve essere possibile partire da una pagina/personaggio o da una feature e navigare fino a:

```text
Character
-> Base Action Signature
-> Feature
-> Epic/Issue
-> Roadmap milestone
-> Scenario
-> Automated/Functional test
-> Wiki explanation
```

e viceversa.

La v0.1 deve mostrare chiaramente:

```text
Flux:
elettricità / setup / conduction

Riva:
Wet / displacement / geometry control

Bastion:
stability / protection / architecture

Vektor:
primary weapon / prediction / interception
```

anche quando le quattro signature ability non vengono utilizzate.

Questo è il criterio principale di successo del lavoro.
