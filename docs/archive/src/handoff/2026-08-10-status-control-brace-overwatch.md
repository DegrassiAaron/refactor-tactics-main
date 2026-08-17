# RefactorTactics — Status, Buff/Debuff, Control, Brace & Overwatch
> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

## Handoff operativo per Claude

**Data:** 2026-08-10  
**Scopo:** consolidare nella documentazione di progetto quanto deciso in questa chat, aggiornare Wiki, roadmap, Feature Map, Scenario Map, Editor Map e repository; creare o aggiornare Epic/Issue coerenti con il piano di implementazione.

---

# 1. Contesto e obiettivo del focus

Questo focus ha definito la grammatica di gameplay per:

- Buff
- Debuff
- Soft Control
- Hard Control
- Status ambientali
- Prepared / Reaction states
- Cleanse
- Resistance
- Immunity
- Applicazione degli status tramite abilità
- Distribuzione di status/control tra archetipi e personaggi
- Integrazione con Brace
- Integrazione con Overwatch
- Interazione con movement, facing, mappa, reaction system e TurnLog

Obiettivo generale:

> Gli status non devono essere semplici bonus/malus numerici. Devono essere modifiche temporanee e leggibili alle regole del combattimento.

Il sistema deve rimanere:
- deterministico;
- server-authoritative;
- data-driven;
- compatibile con replay;
- spiegabile tramite TurnLog e reason code;
- coerente con Planning → Prep → Dash → Blast → Move;
- privo di leak di informazioni private;
- compatibile con Gameplay Tags e GAS come layer di abilità/effetti, senza cedere al GAS l'autorità del resolver.

---

# 2. Distinzione fondamentale dei domini di stato

NON mettere tutto dentro un unico concetto generico di `Status`.

Separare almeno:

```text
UNIT STATUS
- Guarded
- Marked
- Exposed
- Slowed
- Suppressed
- Rooted
- Dazed
- Wet
- Burning
- ecc.

CELL / SURFACE STATE
- Water
- Ice
- Conductive
- Fire
- Smoke
- Steam
- ecc.

EDGE / STRUCTURE STATE
- Door.Open
- Door.Closed
- Bridge.Disabled
- Cover.Destroyed
- ecc.

PREPARED / REACTION STATE
- Brace.Armed
- Overwatch.Armed
- Counter.Armed
- ecc.

TEAM KNOWLEDGE
- Visible
- Detected
- Heard
- LastKnown
- Unknown
```

`Visible`, `Heard`, `Detected`, ecc. NON sono status globali dell'unità: sono conoscenza per-team.

---

# 3. Buff/Debuff non come tipo tecnico principale

Non usare un modello tecnico rigido:

```text
Buff.X
Debuff.Y
```

Preferire categorie funzionali:

```text
Status
├── Modifier
├── Control
├── Environment
├── Stance
├── Reaction
└── Special
```

Con proprietà separata:

```text
Polarity:
- Positive
- Negative
- Neutral
- Contextual
```

Esempio chiave:

```text
Status.Wet
Category = Environment
Polarity = Contextual
```

Wet non è un semplice debuff:
- può spegnere Burning;
- può abilitare skill alleate;
- può interagire con freddo;
- può essere un payoff specifico per skill elettriche;
- NON rende automaticamente un'unità un ponte conduttivo.

---

# 4. Primitive ufficiali del resolver per gli status

Formalizzare queste primitive:

```text
MODIFY
DEGRADE
RESTRICT
INTERRUPT
CONVERT
CONSUME
```

## MODIFY
Modifica un parametro senza invalidare una capability.

Esempio:
```text
Slowed
→ MovementCost +1
```

## DEGRADE
Converte una capability in una versione inferiore.

Esempio:
```text
Suppressed
→ Sprint -> Normal Move
```

## RESTRICT
Vieta una capability specifica.

Esempio:
```text
Rooted
→ Restrict VoluntaryPhysicalMove
```

## INTERRUPT
Cancella qualcosa già armed o in corso.

Esempio:
```text
Knockdown
→ Interrupt Overwatch.Armed
```

## CONVERT
Trasforma stati.

Esempio:
```text
Chilled + Cold
→ Frozen
```

## CONSUME
Usa uno stato e lo rimuove.

Esempio:
```text
Guarded
→ first qualifying hit
→ consume Guarded
```

Queste primitive devono essere riutilizzabili da più status, evitando codice speciale per ogni effetto.

---

# 5. Control Severity

Definire una severity esplicita.

## C0 — Modifier
Non rimuove capability.

Esempi:
- Marked
- Exposed
- Guarded
- Wet

## C1 — Soft Control
Riduce o degrada.

Esempi:
- Slowed
- Suppressed
- Chilled

## C2 — Hard Capability Control
Rimuove una categoria precisa.

Esempi:
- Rooted
- Dazed
- Disarmed
- Jammed

## C3 — Incapacitating Control
Interrompe o vieta più sistemi contemporaneamente.

Esempi:
- KnockedDown
- Stunned
- Incapacitated

Regola:

> Più capability un control rimuove, più setup/costo/gate deve richiedere.

---

# 6. Regola anti-stun-lock

Regola normativa proposta:

> Nessuno status comune e ripetibile deve togliere contemporaneamente Movement + Main Action + Reaction.

Quindi:

```text
Rooted
→ blocca movimento fisico volontario
→ Attack e Reaction restano

Dazed
→ blocca/degrada Fast Reaction manuale
→ Move e Attack restano

Disarmed
→ blocca Ability.Weapon.*
→ Move e altre ability restano

Suppressed
→ degrada Sprint e controllo offensivo
→ il personaggio continua a giocare
```

C3 deve essere:
- raro;
- breve;
- normalmente condizionale;
- preferibilmente payoff di combo, geometry, Super o setup costoso.

---

# 7. Status v0.1 candidato

Per il primo runtime non implementare decine di stati.

Core consigliato:

```text
Guarded
Marked
Exposed
Slowed
Suppressed
Rooted
Dazed
Wet
Burning
Overwatch.Armed
```

Durante il redesign dei quattro personaggi del vertical slice è emerso che si può ridurre ulteriormente il set realmente usato nel roster v0.1 a:

```text
Wet
Marked
Exposed
Suppressed
Dazed
Burning
Prepared Reaction States
```

`Rooted`, `Jammed`, `Disarmed`, `Frozen`, `KnockedDown`, `Stunned` restano nel framework/backlog ma non devono essere forzati dentro il vertical slice solo per testare il sistema.

---

# 8. Status principali — definizioni consolidate/proposte

## Guarded

```text
Category: Modifier
Polarity: Positive
Stack: Unique
Duration: UntilConsumed / EndOfTurn
```

Non è Brace.

Guarded:
- modifica l'impatto;
- può mitigare il primo colpo valido;
- può essere consumato.

Non deve automaticamente dare:
- anti-push;
- anti-root;
- shield;
- armor;
- resistance universale.

---

## Shielded

Separato da Guarded.

```text
Shielded
= pool protettivo

Guarded
= modifica della regola dell'impatto
```

Baseline:
```text
StackPolicy = Replace
```

Non sommare automaticamente più shield.

---

## Focused

Buff posizionale, non semplice +Accuracy.

Proposta:

```text
Stationary:
    beneficio pieno

Move:
    beneficio ridotto o rimosso

Sprint:
    rimosso

Dash:
    rimosso

ForcedMovement:
    rimosso

Knockdown:
    rimosso
```

Può abilitare:
- migliore targeting;
- ignore cover step;
- migliore Overwatch;
- penetrazione;
- altri payoff specifici.

---

## Marked

Keyword di setup.

```text
Intrinsic effects:
NONE
```

Le ability decidono il payoff.

Esempi:
```text
If Target.Has(Marked)
→ alternate effect
```

Baseline:
```text
StackPolicy = Refresh
MaxStacks = 1
```

Non usare un +damage universale come significato globale di Marked.

---

## Exposed

Preferire interazione con geometria/cover:

```text
EffectiveCover -= 1 step
```

Esempio:

```text
High Cover -> Low Cover
Low Cover  -> No Cover
No Cover   -> No further effect
```

Evitare un generico +X% damage received come regola base.

---

## Slowed

Preferire costi interi:

```text
PhysicalMovementCost +1
```

Non percentuali.

Esempio:
```text
edge cost 1
Slowed -> cost 2
```

Appartiene a `MODIFY`.

---

## Suppressed

Soft Control identitario.

Baseline:

```text
Sprint
→ DEGRADE -> Normal Move

Standard Overwatch
→ unavailable o soft-blocked

Normal Move
→ allowed

Attack
→ allowed

Brace
→ allowed
```

Optional futuro:
```text
PivotMaxSteps -1
```

Da introdurre solo dopo stabilizzazione del Facing.

Suppression deve significare:

> Puoi ancora giocare, ma non puoi operare alla massima aggressività.

---

## Rooted

Definizione precisa:

> Rooted impedisce il movimento fisico volontario che richiede locomozione propria.

```text
Move            ❌
Sneak           ❌
Sprint          ❌
Physical Dash   ❌
Charge          ❌
Ladder          ❌

Blink           ability-specific
Teleport        ability-specific

Push subito     ✅
Pull subito     ✅
Knockback subito✅

Attack          ✅
Brace           ✅
Overwatch       ✅
Facing/Pivot    ✅
```

Rooted NON significa immobilità assoluta nello spazio.
Forced Movement resta possibile.

---

## Dazed

Reaction Control.

Definizione preferita:

```text
Move       ✅
Attack     ✅
Brace      ✅
Overwatch può essere armed
Fast Reaction manuale ❌
```

Quando compare una Reaction Opportunity:

```text
Opportunity exists
↓
Dazed
↓
skip manual choice
↓
apply Reaction Default Policy
```

Per Overwatch:
```text
Default = HOLD
```

Dazed non è uno stun.

---

# 9. Status ambientali — regole fondamentali

## Separare sempre:

```text
Damage Type
Environmental Hazard
Unit Status
```

Esempi:

```text
Damage.Electric
Hazard.Electrified
Status.Shocked
```

sono concetti distinti.

Stesso discorso:

```text
Damage.Thermal
Cell.Fire
Status.Burning
```

---

# 10. Wet e conduttività

Decisione importante:

```text
Wet(unit) != Conductive(cell/edge)
```

Un'unità Wet su terreno asciutto:
- può essere Wet;
- può subire payoff specifici;
- NON diventa automaticamente nodo del grafo elettrico.

La propagazione elettrica deve usare:
- celle conduttive;
- archi conduttivi;
- superfici conduttive;
- regole topologiche del grafo.

Non introdurre:

```cpp
if (Unit.HasStatus(Wet))
    ConductElectricity();
```

---

# 11. Electric

Non fare:

```text
Electric Damage
→ automatically Shocked
```

`Shocked` deve essere applicato esplicitamente da:
- ability;
- hazard;
- interaction rule.

La baseline elettrica deve privilegiare:
- conduction;
- chain;
- topology;
- reaction disruption;
- tech disruption.

Non “ogni fulmine = stun”.

---

# 12. Burning

Separare:

```text
Thermal Damage
Fire Surface
Burning Status
```

Una skill può avere:
- danno termico senza Fire;
- Fire senza Burning diretto;
- Burning senza grande danno iniziale.

Wet rimuove Burning nella baseline.

---

# 13. Cold / Ice / Frozen

Framework previsto, ma non scope v0.1.

Distinguere:

```text
Cell.Ice
!=
Status.Frozen
```

Possibile grammatica futura:

```text
Cold + Wet -> Chilled
Chilled + Cold -> Frozen
Water + Cold -> Ice
Frozen + Heat -> Wet
Ice + Heat -> Water
```

Frozen è payoff di setup, non effetto gratuito del primo colpo Cold.

---

# 14. Stacking policy

Baseline:

```text
Unique
Refresh
Stack
Replace
Convert
```

Esempi:

```text
Guarded      Unique
Shielded     Replace
Focused      Refresh
Marked       Refresh
Exposed      Refresh
Slowed       Refresh
Suppressed   Refresh
Rooted       Refresh
Dazed        Refresh
Burning      Stack
Chilled      Convert
Frozen       Unique
Wet          Refresh
```

Evitare stacking numerico ovunque.

---

# 15. Durate

Preferire phase boundary a secondi o turni generici.

Supportare:

```text
UntilEndOfPhase
UntilEndOfTurn
UntilStartOfOwnerTurn
UntilTriggered
UntilConsumed
PermanentUntilRemoved
```

Boundary utili:

```text
EndOfPrep
EndOfDash
EndOfBlast
EndOfMove
EndOfTurn
UntilConsumed
UntilTriggered
```

Hard Control:
- C2: breve;
- C3: molto breve;
- evitare multi-turn stun/root lunghi.

---

# 16. Applicazione status da ability

Pipeline unica:

```text
ABILITY IMPACT
      |
      v
Target valid?
      |
      v
Application Requirements
      |
      v
Immunity
      |
      v
Resistance
      |
      v
Status Interaction / Conversion
      |
      v
Apply / Degrade / Reject
      |
      v
Revalidate capabilities
      |
      v
Revalidate prepared reactions
      |
      v
TurnLog
```

---

# 17. Modi di applicazione

Supportare:

```text
Direct
Conditional
Setup + Payoff
Positional
Environmental
Triggered
```

Esempi:

```text
Direct:
Hit -> Slow

Conditional:
Wet + special electric -> Dazed

Setup + Payoff:
Marked -> next skill -> stronger result

Positional:
Push into wall -> Exposed / future Knockdown

Environmental:
target on Ice -> stronger control

Triggered:
cross controlled zone -> Suppressed
```

---

# 18. Hard Control deve avere Gate

Per C2/C3 richiedere normalmente almeno uno:

```text
Target State Gate
Environment Gate
Position Gate
Facing Gate
Resource Gate
Charge Gate
Combo Gate
Reaction Gate
Prior Hit Gate
```

Esempi:

```text
Suppressed + PinDown -> Rooted
Wet + Conductive setup + Overload -> Dazed
Push into wall -> Exposed / future Knockdown
Rear/Flank condition -> Dazed
Marked + payoff skill -> stronger control
```

---

# 19. Niente RNG nascosto per il control base

Evitare:

```text
65% chance to Root
```

Preferire:

```text
IF condition
    Apply Root
ELSE
    Apply Slow
```

L'applicazione base deve essere prevedibile e deterministica.

---

# 20. Successo parziale

Le status application devono poter essere:

```text
Full
Degraded
Rejected
```

Esempio:

```text
Root requested

Target normal:
Root

Target resistant:
Slow

Target immune:
No status
```

Damage e Status devono essere risolti separatamente.

Una ability può fare danno anche se il control viene resistito o negato.

---

# 21. Resistance

Niente percentuale generica.

Preferire profili deterministici:

```text
Resistance.Control.Movement
```

Esempi:

```text
Root -> Slow
Slow -> shorter duration
Push 2 -> Push 1
```

Resistance = degrada.

---

# 22. Immunity

Immunity = nega.

Usarla poco e in modo specifico:

```text
Immune.Root
Immune.Displacement
Immune.Burning
```

Evitare passive tipo:

```text
Immune.Control
```

sempre attive.

---

# 23. Cleanse

Niente:

```text
RemoveAllDebuffs()
```

Preferire:

```text
Cleanse.Control
Cleanse.Environment
Cleanse.Tech
Cleanse.Mark
Cleanse.Physical
```

Supportare:
- remove specific status;
- remove one status by category;
- eventualmente scelta del target/status in Planning.

Se più status sono eleggibili, usare ordine deterministico:
```text
RemovalPriority
Severity
AppliedAt
StableStatusId
```

oppure fare scegliere il giocatore.

---

# 24. Counter vs Cleanse

Distinzione:

```text
COUNTER / PREVENTION
prima dell'applicazione

CLEANSE
dopo l'applicazione
```

Tre famiglie:

```text
Passive Counter
- Resistance
- Immunity

Prepared Counter
- Brace
- Guard
- Counter stance

Fast Reaction Counter
- Decision Window su evento incoming
```

---

# 25. Setup Consumption

Ogni interaction rule può dichiarare:

```text
Keep
Consume
Transform
Reduce
```

Esempi:

```text
Marked + Execute
→ Consume Marked

Wet + electric payoff
→ Keep Wet

Chilled + Cold
→ Transform into Frozen

Burning stack future
→ Reduce
```

---

# 26. Reapply Policy

Per evitare chain infinite:

```text
Ignore
Refresh
Escalate
Replace
ConsumeSetup
```

Hard Control deve preferire spesso:

```text
ConsumeSetup
```

Esempio:

```text
Suppressed + PinDown
→ remove Suppressed
→ apply Rooted
```

Così non si refresha Root all'infinito con lo stesso setup.

---

# 27. Loop prevention

Non usare:

```text
while (somethingChanged)
    ProcessEverythingAgain();
```

Proposta:

```text
Effect Event E123
→ collect matching rules
→ stable sort
→ apply each allowed rule once
→ emit new events for NEXT resolver boundary
```

Ogni transition deve essere processata in modo deterministico per EventId.

Validator deve trovare:
- self-loop;
- cycle nello stesso boundary;
- conversioni ambigue.

---

# 28. Reason Codes / TurnLog

Normalizzare eventi come:

```text
StatusApplicationRequested
StatusApplicationBlocked
StatusApplicationResisted
StatusApplicationDegraded
StatusApplied
StatusRefreshed
StatusConsumed
StatusConverted
StatusRemoved

CapabilityModified
CapabilityRestricted
ActionDegraded
ActionInvalidated

ReactionInvalidated
ReactionDefaulted
```

Reason code candidati:

```text
Status.Blocked.Immunity
Status.Degraded.Resistance
Status.Applied.Direct
Status.Applied.Combo
Status.Applied.Environment
Status.Removed.Cleanse
Status.Removed.Consumed
Status.Converted.SetupPayoff

Action.Degraded.Suppressed
Action.Invalid.Rooted

Reaction.Defaulted.Dazed
Reaction.Cancelled.Knockdown
```

Prima di creare nuovi tipi, verificare eventi/enum esistenti nel repository ed evitare duplicati.

---

# 29. UI

Non mostrare 20 icone allo stesso livello.

Raggruppare:

```text
BUFF
[Guarded] [Focused]

CONTROL
[Suppressed]

ENVIRONMENT
[Wet]

PREPARED
[Brace] [Overwatch]
```

Tooltip deve spiegare regola, non percentuali criptiche.

Esempio:

```text
SUPPRESSED

Sprint → Normal Move
Standard Overwatch unavailable
Expires: End of turn
Source: Enemy_04
```

Planning preview:
se lo stato è noto e deterministico, mostrare `Confermato`.

Esempio:

```text
SPRINT
Current Status: Suppressed
This action will resolve as NORMAL MOVE
```

Se invece il possibile status futuro dipende da piano avversario non noto:
`Incerto`, senza leak.

---

# 30. Distribuzione per archetipi

Regola:

> Ogni personaggio deve essere forte in 1–2 domini di control/status, competente in un terzo, debole negli altri.

Domini:

```text
Movement
Position
Facing
Visibility
Reaction
Weapon
Technology
Environment
Information
Defense
```

---

# 31. Identità dei ruoli

```text
GUARDIAN
"I preserve plans."

CONTROLLER
"I reshape plans."

SUPPORT
"I restore plans."

MARKSMAN
"I punish predictable plans."

SKIRMISHER
"I disrupt timing."

ASSASSIN
"I exploit information gaps."

BRUISER
"I punish proximity."

ENGINEER
"I change the rules of the area."
```

Evitare la semplificazione:
`Tank / DPS / Heal / CC`.

---

# 32. Regole roster

1. Ogni personaggio: 1–2 domini primari.
2. Normalmente massimo una fonte C2 nel kit base.
3. C3 principalmente fuori dalle basic ability normali.
4. Controller non significa tutti i CC.
5. Support non significa healbot.
6. Guardian preserva piani.
7. Marksman controlla anche info, linee e reaction.
8. Skirmisher usa disruption breve.
9. Hard Control arriva da setup, geometry o combo.
10. Gran parte del control deve poter avvenire modificando la mappa, senza status.
11. Cleanse specialistico; Resistance più diffusa.
12. Nessun kit dovrebbe possedere forte setup + hard control + payoff + cleanse tutto insieme.
13. Ogni personaggio deve comunque avere almeno un'azione utile indipendente.

---

# 33. Vertical slice — personaggi attuali

Roster discusso:

```text
Gadget
Phase
Riktor
Wraith
```

Non rinominare o sostituire automaticamente questi personaggi senza verificare il repository/source of truth.

---

# 34. Gadget — status/control identity

Identità:
- disruption elettrica;
- conduzione;
- topology;
- reaction disruption;
- non “mago dello stun”.

Kit discusso:
- ArcPulse
- LinearDischarge
- ConductiveNode
- Overload

Direzione:

```text
ArcPulse
→ basic elettrico semplice
→ no status

LinearDischarge
→ payoff contro Wet
→ no hard CC

ConductiveNode
→ Cell.Conductive
→ setup ambientale

Overload
→ AoE elettrica
→ Dazed SOLO con gate conduttivo/elettrico appropriato
```

Non:
```text
ogni attacco elettrico -> Dazed/Shocked
```

---

# 35. Phase — status/control identity

Identità:
- water shaping;
- push/pull;
- route control;
- terrain;
- visibility;
- control tramite mappa più che tramite icone.

Kit discusso:
- PressureJet
- CircularTide
- FluidTrail
- MistVeil

Direzione:

```text
PressureJet
→ Damage
→ Wet
→ Push

CircularTide
→ Wet area
→ displacement / soft control
→ no Root

FluidTrail
→ Dash
→ create water cells

MistVeil
→ Smoke/Mist
→ LOS / visibility / targeting interaction
→ no generic Blind
```

---

# 36. Riktor — status/control identity

Identità:
- preserva posizione;
- modifica cover;
- canali/rotte;
- interposition;
- geometry control.

Kit discusso:
- ImpactShot
- KineticPanel
- Reconfigure
- Ram
- Interposition

Direzione:

```text
ImpactShot
→ basic semplice
→ no CC

KineticPanel
→ crea cover
→ no automatic Guarded aura

Reconfigure
→ modifica cover/structure/graph

Ram
→ Push
→ if collision against solid geometry:
   Apply Exposed
```

Per v0.1:
`Exposed` come collision payoff.

Future heavy variant:
`Knockdown` possibile ma non baseline.

Interposition:
- redirige/intercetta;
- non semplicemente “apply Guarded”.

---

# 37. Wraith — status/control identity

Identità:
- predictive duelist;
- Mark;
- prediction;
- movement punish;
- reaction pressure.

Kit discusso:
- PulseShot
- PassingBlade
- Feint
- InterceptShot
- Deflection

Direzione:

```text
PulseShot
→ basic semplice
→ no Mark

Feint
→ Apply Marked
→ setup

PassingBlade
→ damage / mobility
→ conditional Exposed via flank/rear/Marked

InterceptShot
→ Damage
→ MovementInterrupted
→ candidate Suppressed if needed
```

Nella discussione più recente è stata preferita una versione più pulita:

```text
InterceptShot / Predictive Overwatch
→ Damage
→ MovementInterrupted
```

senza Suppressed nella baseline, perché l'interruzione del movimento è già un payload forte.

Quindi:
**non aggiungere Suppressed a InterceptShot senza esplicita conferma di design.**

---

# 38. Matrice status/control vertical slice proposta

```text
Gadget
- Dazed gated
- Conductive
- Electric payoff

Phase
- Wet
- Push / ForcedMovement
- Water/Smoke

Riktor
- Exposed via collision
- Cover/Structure
- Anchor/Interposition

Wraith
- Marked
- Exposed conditional
- MovementInterrupt
- Predictive Overwatch
```

Rooted può rimanere fuori dai quattro kit v0.1.

---

# 39. Brace — baseline

Brace è una Prepared Reaction.

Lifecycle:

```text
Planning
→ Brace selected
→ Prep: Brace.Armed
→ Incoming Forced Movement
→ Reaction Opportunity
→ Character Response / HOLD
→ Resolve
```

Trigger:
- Push
- Pull
- Knockback
- Forced Movement

Brace:
- NON è Guard;
- NON riduce automaticamente il danno;
- NON è immunity generica;
- NON cancella qualsiasi control.

Proposta prototype:
```text
Timeout -> HOLD
```

---

# 40. Brace e Move

Decisione proposta:

```text
Brace NON riduce il normale Move.
```

Costo di Brace:
- rinuncia alla Main Action/offensiva prevista dalla sua action economy;
- reaction armed.

Se Forced Movement avviene durante un Move in corso:

```text
ForcedMovement
→ MovementInterrupted
→ no auto-repath
```

---

# 41. Quattro Brace Profiles

## Gadget — Grounding

```text
GROUND / HOLD
```

Identità:
- scarica/stabilizza tramite terreno;
- riduce displacement;
- possibile interaction con conductive context;
- non tank universale.

---

## Phase — Flow

```text
FLOW LEFT / FLOW RIGHT / HOLD
```

Proposta di playtest:

Push E di 2:

```text
Step 1 -> E
Step 2 -> Phase può deviare ultimo step verso NE o SE
```

Quindi Flow:
- accetta displacement;
- modifica endpoint;
- non riduce necessariamente la distanza;
- destinazioni illegali non vengono offerte al client.

---

## Riktor — Anchor

```text
ANCHOR / HOLD
```

Riduce più degli altri il displacement.

Possibile:
```text
Push 2 -> Push 0/1
```

La quantità esatta resta balance.

Identità:
> mantenere posizione.

---

## Wraith — Deflection

```text
DEFLECT LEFT / DEFLECT RIGHT / HOLD
```

Differenza proposta rispetto a Flow:

```text
Flow
→ curva endpoint / ultima transizione

Deflection
→ devia il vettore completo
```

Esempio:

```text
Push E
→ DEFLECT LEFT
→ Push NE
```

Magnitudine invariata.

---

# 42. Facing durante Brace

Per il primo playtest:

```text
FacingPolicy = Preserve
```

per tutti.

Niente pivot gratuito dentro Brace finché displacement + reaction non sono stabili.

Possibili policy future:
- Preserve
- FaceThreat
- FaceMovement

---

# 43. Overwatch — lifecycle scelto

Direzione preferita:

```text
WATCH
→ END WATCH
→ REPOSITION
```

Durante Planning:

```text
Overwatch Profile
Watch Origin
Prepared Watch Facing
Controlled Area
Reaction Policy

+

Reposition Path
Reposition Final Facing
```

Durante Move Phase:

```text
Stage A — WATCH / STANDARD MOVE

Overwatch units:
    stationary

Other units:
    normal Move micro-steps

Overwatch:
    active

END WATCH

Stage B — REPOSITION

former Overwatch units:
    execute limited pre-planned movement
```

Non creare nuove macro-fasi: è una segmentazione interna della Move Phase.

---

# 44. Overwatch commitment

Overwatch sostituisce l'azione offensiva/ability prevista dal relativo sistema.

In più:

```text
Normal Move
→ replaced by limited Reposition
```

No refund se:
- nessuno triggera;
- HOLD;
- Overwatch cancellata;
- FIRE non produce risultato desiderato.

Reposition:
- pre-planned;
- reduced budget;
- normal movement legality;
- no Sprint;
- no Dash;
- no teleport implicito;
- no destination reservation;
- no auto-reroute.

Budget:
`OPEN_BALANCE`.

---

# 45. Trigger Overwatch

Trigger basato su vera transizione di Move volontario.

Candidate condition:

```text
FromCell controlled
OR
ToCell controlled
OR
CrossedEdge controlled
```

Target fermo:
```text
no Move -> no trigger
```

Standard Overwatch v0.1:
- non reagisce al Dash già avvenuto prima della Watch stage.

Cadence consigliata:

```text
OncePerTargetPerReactionInstance
```

Quindi:
- Tank entra -> HOLD
- continua a muoversi -> no prompt aggiuntivo
- esce e rientra -> ancora no prompt
- Scout entra -> nuova opportunity
- Carry entra -> nuova opportunity

Preserva bait/bluff e impedisce prompt storm.

---

# 46. Overwatch — HOLD

HOLD:
- perde solo quell'opportunity;
- mantiene reaction armed se ancora valida;
- non informa su future opportunities;
- non rivela quanti target arriveranno dopo.

Timeout Overwatch:
```text
HOLD
```

---

# 47. Simultaneous trigger

Se più unità triggerano nello stesso micro-step:

NON:
```text
prompt A
prompt B
```

in base all'ordine di iterazione.

Creare una singola opportunity con più target validi:

```text
FIRE A
FIRE B
HOLD
```

Ordine deterministico e stabile.

---

# 48. Quattro Overwatch Profiles

## Gadget — Conductive Overwatch

Geometria:
- medium directional sector.

Response:
```text
DISCHARGE target / HOLD
```

Payload:
- electric attack;
- riusa il sistema conduction;
- niente Dazed automatico nella baseline.

---

## Phase — Pressure Overwatch

Geometria:
- medium-short directional sector.

Response:
```text
PUSH target / HOLD
```

Payload:
- Wet;
- Push;
- possibile interruzione del Move tramite Forced Movement.

Regola importante:

Se target ha mosso:

```text
A -> B
```

e Phase triggera a B:

```text
B -> X via Push
```

NON tornare ad A.

Poi:

```text
MovementInterrupted
no repath toward original destination
```

---

## Riktor — Frontline Overwatch

Geometria:
- short;
- wide;
- frontal.

Response:
```text
FIRE target / HOLD
```

Payload baseline:
- simple/basic attack;
- no Suppressed.

La forza viene dalla geometria:
- cover;
- choke;
- porte;
- Reconfigure;
- KineticPanel.

Non rendere Riktor contemporaneamente “miglior Brace + miglior Overwatch”.

---

## Wraith — Predictive Overwatch

Geometria:
- narrow;
- long;
- corridor-like.

Response:
```text
INTERCEPT target / HOLD
```

Payload baseline proposta:

```text
Damage
+
MovementInterrupted
```

Non aggiungere Suppressed nella baseline senza nuova conferma.

Distinguere:

```text
Predictive Action:
scommessa pre-planned su cella/traiettoria
no scelta live

Predictive Overwatch:
controllo di una traiettoria
INTERCEPT / HOLD se realmente attraversata
```

---

# 49. FIRE non libera movimento in anticipo

Regola fondamentale:

```text
Watch
→ FIRE early
→ owner remains stationary
→ Watch continues logically / owner no longer armed if charge consumed
→ waits until EndWatchStage
→ then Reposition
```

Non permettere:

```text
"ho sparato presto quindi ora mi muovo mentre gli altri stanno ancora risolvendo"
```

---

# 50. Status × Prepared Reactions

Candidate rules:

```text
Suppressed
→ Overwatch soft-blocked
→ Brace normal

Dazed
→ Opportunity can exist
→ no manual decision
→ fallback policy, e.g. HOLD

Rooted
→ Brace allowed
→ Overwatch Watch allowed
→ physical Reposition unavailable while Root persists

KnockedDown / Stunned
→ cancel Prepared Reaction

Forced Movement before Watch
→ candidate cancel Overwatch

Forced Facing before Watch
→ candidate cancel Overwatch v0.1

Smoke / LOS / Detection failure
→ reaction remains armed
→ target not eligible
```

Distinguere:
- hard cancellation;
- soft eligibility block.

---

# 51. Reaction Clash

NON implementare subito.

Prima validare Single Responder Reactions:
- Grounding
- Flow
- Anchor
- Deflection
- Conductive OW
- Pressure OW
- Frontline OW
- Predictive OW

Poi creare un'epic/fase separata per `Reaction Clash`.

Primo scenario candidato:

```text
Phase Pressure Jet
vs
Riktor Brace
```

Pattern futuro:
```text
READ > COMMIT > SHIFT > READ
```

---

# 52. Test automatici minimi

Creare golden/automation tests.

## Status

1. Suppressed degrada Sprint -> Move.
2. Rooted invalida Move ma non Attack/Reaction.
3. Dazed fa default della Fast Reaction.
4. Wet rimuove Burning.
5. Wet su cella Dry NON conduce elettricità.
6. Resistance Root -> Slow.
7. Immunity Root -> Reject.
8. Cleanse.Control non rimuove Marked/Burning.
9. Setup consumption deterministico.
10. Conversion no-loop.

## Brace

11. Riktor Anchor riduce Push.
12. Phase Flow modifica endpoint, no illegal destination.
13. Wraith Deflection modifica vettore.
14. Gadget Grounding usa regola deterministica.
15. Forced Movement durante Move -> MovementInterrupted, no repath.

## Overwatch

16. HOLD preserva reaction.
17. Timeout -> HOLD.
18. OncePerTargetPerReactionInstance.
19. Multiple targets same micro-step -> single opportunity.
20. No future trigger leak.
21. Pressure OW push -> interrupt movement, no repath.
22. Predictive OW -> MovementInterrupted.
23. FIRE early does not unlock movement.
24. Watch ends before Reposition.
25. Reposition uses preplanned path, no Sprint/Dash.
26. Dazed -> no manual Fast Reaction, fallback HOLD.
27. Suppressed soft-block behavior.
28. Rooted prevents physical Reposition while active.
29. Deterministic order across repeated runs.
30. TurnLog/StateHash identical for same snapshot/rules/seed.

---

# 53. Validator / design lint

Aggiungere validator/lint per:

```text
C2 control without duration
C3 control without gate/cost
Status without StackPolicy
Status without ExpirationPolicy
Unknown Cleanse tag
Resistance without degradation rule
Conversion self-loop
Conversion cycle
Unknown StatusId
Status blocking all capabilities without C3
Repeatable basic ability with C2+ and no gate
AoE C3 without resource/setup gate
Overwatch profile without trigger geometry
Overwatch profile without timeout policy
Brace profile without valid response/fallback
Reaction profile with nondeterministic target ordering
```

---

# 54. Metriche playtest

Registrare almeno:

```text
StatusApplications / match
StatusResisted / match
StatusCleansed / match

C1 uptime
C2 uptime
C3 uptime

ActionsModified
ActionsDegraded
ActionsRestricted
ActionsInterrupted

ReactionOpportunities
ReactionCommits
ReactionHolds
ReactionTimeouts

OverwatchTriggers
OverwatchFires
OverwatchHolds
OverwatchExpiredUnused

BraceTriggers
BraceCommits
BraceHolds

TurnsWithZeroMeaningfulAgency
```

`TurnsWithZeroMeaningfulAgency` è metrica critica anti-frustrazione.

---

# 55. Aggiornamenti documentazione richiesti

Claude deve aggiornare/consolidare almeno:

## Wiki

Creare o aggiornare pagine:

```text
Gameplay/Status-System
Gameplay/Buffs-Debuffs-Control
Gameplay/Control-Severity
Gameplay/Status-Application
Gameplay/Cleanse-Resistance-Immunity
Gameplay/Environmental-Status-Interactions
Gameplay/Brace
Gameplay/Overwatch
Gameplay/Reaction-System
Gameplay/Character-Control-Domains
Characters/Gadget
Characters/Phase
Characters/Riktor
Characters/Wraith
Architecture/Status-Resolver
Architecture/Reaction-Resolver
Testing/Status-Reaction-Golden-Tests
```

Verificare naming e struttura Wiki esistenti prima di creare duplicati.

---

# 56. PDR / Docs

Aggiornare i documenti pertinenti, in particolare:

- Abilità / personaggi / GAS
- Simulazione deterministica
- Networking/privacy
- UI/UX
- Mappa/pathfinding
- Roadmap/QA/rischi
- Gameplay Tags / content validation

Non alterare una source of truth superiore senza verificare ADR/registry correnti.

Se esistono conflitti tra documenti:
1. elencarli;
2. non nasconderli;
3. proporre una decisione;
4. creare ADR/open decision se necessario.

---

# 57. Roadmap

Aggiungere o riallineare milestone/epic per almeno:

```text
Status Framework
Control Primitives
Status Data Definitions
Status Resolver
Status UI
Status TurnLog / Reason Codes
Status Validator / Lint

Brace Profiles
Overwatch Profiles
Watch -> Reposition Lifecycle
Decision Window integration
Reaction status interaction
Reaction privacy tests
Reaction golden tests

Character v0.1 status pass
Gadget status/control integration
Phase status/control integration
Riktor status/control integration
Wraith status/control integration

Reaction Clash — FUTURE
Advanced Control — FUTURE
Disarm/Jam/Knockdown/Stun — FUTURE
Cold/Ice/Frozen — FUTURE
```

Non anticipare sistemi non necessari nella milestone corrente.

---

# 58. Feature Map

Aggiornare Feature Map con nodi/feature:

```text
Status System
├─ Modifier Status
├─ Control Status
├─ Environmental Status
├─ Prepared Reaction State
├─ Status Application
├─ Resistance
├─ Immunity
├─ Cleanse
├─ Status Conversion
├─ Capability Revalidation
├─ Status UI
└─ Status Debug/TurnLog

Reaction System
├─ Brace
│  ├─ Grounding
│  ├─ Flow
│  ├─ Anchor
│  └─ Deflection
│
├─ Overwatch
│  ├─ Conductive
│  ├─ Pressure
│  ├─ Frontline
│  └─ Predictive
│
├─ Decision Window
├─ HOLD / timeout
├─ Watch -> Reposition
├─ Reaction Opportunity
├─ Status interaction
└─ Reaction Clash [future]
```

Aggiungere link:
- Wiki;
- Issue;
- Epic;
- scenario;
- implementation docs;
- milestone.

---

# 59. Scenario Map

Aggiungere scenari verificabili.

Candidate IDs da adattare al naming esistente:

```text
STATUS-001 Suppressed Sprint Degrade
STATUS-002 Rooted Move Restriction
STATUS-003 Dazed Reaction Default
STATUS-004 Wet Extinguishes Burning
STATUS-005 Wet Is Not Conductive
STATUS-006 Resistance Degrades Root
STATUS-007 Cleanse Category Filtering

BRACE-001 Riktor Anchor vs Push
BRACE-002 Phase Flow Endpoint
BRACE-003 Wraith Deflection Vector
BRACE-004 Gadget Grounding

OW-001 HOLD preserves reaction
OW-002 timeout HOLD
OW-003 once per target
OW-004 simultaneous targets
OW-005 Pressure push interrupts Move
OW-006 Wraith Intercept interrupts Move
OW-007 FIRE does not unlock movement
OW-008 Watch then Reposition
OW-009 Root blocks Reposition
OW-010 Dazed forces fallback
OW-011 privacy no future opportunity leak

CHAR-STATUS-001 Gadget Conductive Overload setup
CHAR-STATUS-002 Phase PressureJet Wet + Push
CHAR-STATUS-003 Riktor Ram collision Exposed
CHAR-STATUS-004 Wraith Feint Marked
```

Scenario Map deve linkare:
- feature;
- issue;
- automated test;
- wiki;
- eventuale map/test fixture.

---

# 60. Editor Map

Aggiungere task Editor/manual-only per:

```text
Status icons
Status grouping HUD
Status tooltip visuals
Prepared Reaction icons
Brace response UI
Overwatch cone/sector preview
Watch origin marker
Reposition ghost path
Reaction Decision Window
Countdown presentation
HOLD / FIRE / profile-specific response buttons
Status debug overlay
Reaction debug overlay
Facing arrows during Brace/OW
Controlled area visualization
Smoke/Mist readability
Wet/Conductive distinction visual
MovementInterrupted feedback
```

Per ogni task indicare:
- asset/widget;
- owner/manual step;
- blocking issue;
- test scenario;
- screenshot/video acceptance evidence.

---

# 61. Epic / Issue richieste

Claude deve controllare repository e roadmap correnti PRIMA di crearle.

## Epic candidati

```text
EPIC — Status & Control Framework
EPIC — Prepared Reactions: Brace
EPIC — Overwatch Profiles & Lifecycle
EPIC — Character Status Integration v0.1
EPIC — Status/Reaction UI & Explainability
EPIC — Status/Reaction Testing & Validation
```

## Issue candidate — Status Framework

```text
Define FRTStatusInstance runtime model
Define StatusDefinition data model
Implement Status Resolver primitives
Implement MODIFY
Implement DEGRADE
Implement RESTRICT
Implement INTERRUPT
Implement CONVERT
Implement CONSUME
Implement status duration boundaries
Implement status stacking policies
Implement capability revalidation
Implement reaction revalidation
Implement resistance degradation
Implement immunity rejection
Implement cleanse categories
Implement setup consumption
Implement deterministic reapply policy
Add status reason codes
Add TurnLog status events
Add status validator/lint
```

## Issue candidate — Brace

```text
Implement Brace prepared state
Implement ForcedMovement trigger
Implement Brace Decision Window
Implement Gadget Grounding profile
Implement Phase Flow profile
Implement Riktor Anchor profile
Implement Wraith Deflection profile
Implement Brace timeout HOLD prototype
Implement Brace facing Preserve baseline
Add Brace golden tests
Add Brace debug overlay
```

## Issue candidate — Overwatch

```text
Implement Watch -> Reposition lifecycle
Implement watch origin/facing
Implement controlled area geometry
Implement once-per-target cadence
Implement simultaneous target opportunity
Implement HOLD semantics
Implement timeout HOLD
Implement preplanned Reposition
Prevent Sprint/Dash in Reposition
Prevent movement unlock after FIRE
Implement Gadget Conductive OW
Implement Phase Pressure OW
Implement Riktor Frontline OW
Implement Wraith Predictive OW
Implement MovementInterrupted event/path stop
Implement Dazed fallback behavior
Implement Suppressed OW soft-block
Implement Rooted Reposition restriction
Add Overwatch privacy tests
Add Overwatch golden tests
Add Watch/Reposition debug tooling
```

## Issue candidate — Character integration

```text
Gadget: ConductiveNode + status hooks
Gadget: gated Dazed on Overload
Phase: Wet/Push status hooks
Phase: Mist/visibility interaction
Riktor: Ram collision -> Exposed
Riktor: Anchor/Interposition integration
Wraith: Feint -> Marked
Wraith: PassingBlade conditional Exposed
Wraith: Intercept movement interruption
```

---

# 62. Issue hygiene

Prima di creare nuove issue:

1. cercare issue esistenti;
2. aggiornare quelle sovrapposte;
3. evitare duplicati;
4. mantenere Epic/Parent coerenti;
5. assegnare milestone appropriata;
6. aggiungere acceptance criteria;
7. linkare wiki/feature/scenario/editor map;
8. aggiungere test richiesti;
9. indicare dipendenze;
10. indicare `OPEN_BALANCE` dove il numero non è ancora fissato.

---

# 63. Acceptance Criteria comuni

Ogni issue gameplay/status/reaction è Done solo se:

```text
[ ] server-authoritative
[ ] deterministic
[ ] same snapshot/rules/seed -> same result
[ ] no TMap/TSet iteration dependence
[ ] no animation timing authority
[ ] TurnLog event present
[ ] reason code present
[ ] UI can explain result
[ ] privacy classification verified
[ ] Automation Test present
[ ] packaged build verified where relevant
[ ] documentation updated
[ ] Feature Map link updated
[ ] Scenario Map link updated
[ ] roadmap updated
```

Per team/private data aggiungere:

```text
[ ] zero data leak to unauthorized client
```

---

# 64. Open decisions da NON inventare

Mantenere esplicitamente aperti, salvo dati canonici già presenti nel repository:

```text
Exact damage values
Exact status durations
Exact Overwatch Reposition budget
Exact Brace displacement reduction
Exact Grounding electric payoff
Exact Flow geometry
Exact Deflection geometry
Exact Overwatch arc width/range
Exact Prompt cap
Exact Suppression interaction with Facing
Exact Shield values
Exact Focus benefits
Exact C2/C3 cooldown/resource budgets
```

Usare label:
```text
OPEN_BALANCE
OPEN_DESIGN
OPEN_TECH
```

dove appropriato.

---

# 65. Decisioni da documentare come ADR / Design Decision

Candidate decision records:

```text
ADR — Status domains are separated from surfaces, structures and team knowledge
ADR — Status effects use resolver primitives instead of per-status hardcode
ADR — Wet unit status does not imply conductive graph connectivity
ADR — Control application is deterministic by default
ADR — C2/C3 control requires setup/cost/gate
ADR — Control should preserve meaningful agency
ADR — Brace is separate from Guarded
ADR — Overwatch uses Watch -> Reposition lifecycle
ADR — Overwatch FIRE does not unlock early movement
ADR — Forced Movement interrupts remaining Move without auto-repath
ADR — Reaction Opportunity simultaneous targets are grouped deterministically
ADR — Dazed removes manual reaction choice rather than deleting the opportunity
```

Verificare formato ADR già usato nel repository.

---

# 66. Git / repository

Claude deve:

1. ispezionare branch e working tree;
2. leggere roadmap/maps/wiki esistenti;
3. individuare source of truth attuali;
4. aggiornare file esistenti prima di crearne di duplicati;
5. creare commit focalizzati;
6. non mischiare implementazione C++ con massivo cleanup docs nello stesso commit se evitabile.

Commit candidati:

```text
docs(status): consolidate buff debuff and control grammar
docs(reaction): consolidate brace and overwatch profiles
docs(roadmap): add status and reaction implementation plan
docs(wiki): add status and prepared reaction pages
docs(maps): update feature scenario and editor maps
chore(github): align epics and issues for status and reaction systems
```

Se vengono già implementate parti tecniche:

```text
feat(status): add deterministic status resolver primitives
feat(reaction): add brace profile framework
feat(overwatch): add watch reposition lifecycle
test(status): add status golden scenarios
test(reaction): add brace and overwatch deterministic tests
```

---

# 67. Ordine consigliato di lavoro per Claude

```text
1. Inspect repository / source of truth
2. Find existing docs and issues
3. Reconcile duplicates/conflicts
4. Update Wiki
5. Update design/PDR docs
6. Update Roadmap
7. Update Feature Map
8. Update Scenario Map
9. Update Editor Map
10. Create/update Epic and Issues
11. Add cross-links
12. Produce final change report
13. Commit focused changes
```

---

# 68. Output finale richiesto a Claude

Alla fine Claude deve restituire un report con:

```text
Updated files
Created files
Wiki pages changed
Roadmap items changed
Feature Map changes
Scenario Map changes
Editor Map changes
Epics created/updated
Issues created/updated
ADRs created/updated
Open decisions
Conflicts found
Tests/scenarios added
Git commits
Next recommended implementation issue
```

Includere link/ID GitHub reali quando disponibili.

---

# 69. Principio finale

Il sistema deve produrre questo tipo di gameplay:

```text
STATUS
≠
"hai -15%"

STATUS
=
"le regole che puoi sfruttare in questo momento sono cambiate"
```

E il Control deve vincere attraverso:

```text
previsione
setup
geometria
tempo
positioning
reaction
environment
```

non attraverso:

```text
"il nemico non può giocare"
```

Questa è la linea di design da preservare durante implementazione, documentazione e future estensioni.
