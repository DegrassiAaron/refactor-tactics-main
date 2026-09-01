# RefactorTactics — Cover, Melee, AoE & Facing
## Consolidation Pack for Claudia — v0.1
**Data:** 2026-09-01  
**Stato:** Consolidation / Decision Record draft  
**Scopo:** inserire e consolidare nel repository le decisioni emerse sulla geometria della cover, interazione melee/AoE e Facing.

---

## 0. Fonte e perimetro

Questa nota consolida decisioni emerse nel lavoro su:

- Tactical Segment / Cover;
- Wall, Low Cover, High Cover, No-Cross;
- melee e validità del contatto;
- Blast / AoE / Cone / Line;
- Cover mitigation;
- Facing;
- Planning / Resolution / Snapshot;
- segmenti interni all’hex.

Le fonti di progetto già disponibili confermano:
- Tactical Segment con C→V, C→E e segmenti sul bordo;
- separazione concettuale tra cover, LOS, projectile blocking e traversal;
- `Facing` come stato competitivo;
- separazione tra Shape, TargetKind, Delivery e Hit Rule;
- ordine macro della resolution con Prep → Dash → Blast → Move;
- necessità di mantenere il resolver deterministico.

Questa nota NON introduce modding pubblico, matchmaking o progressione.

---

# 1. Core Principle

La geometria di un segmento e il suo comportamento tattico sono assi distinti.

Non usare:

```text
CoverLevel
```

per dedurre automaticamente:

```text
BlocksLOS
BlocksProjectile
BlocksTraversal
BlocksMelee
```

La domanda deve sempre essere valutata separatamente:

1. Il segmento fornisce cover?
2. Blocca LOS?
3. Blocca proiettili / propagazione?
4. Blocca movimento?
5. Blocca il contatto melee?
6. Può essere attraversato tramite una traversal speciale?

Questo vale sia per segmenti sul bordo dell'hex sia per segmenti interni all'hex.

---

# 2. Tactical Segment Presets

## 2.1 Low Cover

Default:

```text
Cover = Low
BlocksLOS = false
BlocksProjectiles = false
BlocksNormalTraversal = true
BlocksMeleeContact = false
Vaultable = true, se la unit/abilità possiede traversal compatibile
```

Comportamento:

- fornisce mitigazione direzionale;
- non blocca automaticamente la linea di vista;
- non blocca automaticamente proiettili;
- non impedisce un attacco melee se il contatto è geometricamente possibile;
- impedisce il normale attraversamento del segmento.

Low Cover NON significa “25% cover” a livello semantico. La percentuale/quantità di mitigazione è una regola separata di Cover Policy.

---

## 2.2 High Cover

Default:

```text
Cover = High
BlocksLOS = false
BlocksProjectiles = false
BlocksNormalTraversal = true
BlocksMeleeContact = false
Vaultable = false
```

Comportamento:

- fornisce mitigazione direzionale più forte della Low Cover;
- non blocca automaticamente LOS;
- non blocca automaticamente i proiettili;
- non blocca la melee se il contatto è geometricamente possibile;
- non è normalmente attraversabile senza una traversal speciale.

High Cover NON significa “muro”.

---

## 2.3 Wall

Default:

```text
Cover = optional High Cover for adjacent legal CoverOption
BlocksLOS = true
BlocksProjectiles = true
BlocksNormalTraversal = true
BlocksMeleeContact = true
Vaultable = false
```

Un Wall è una barriera fisica completa.

Importante:

```text
Wall != High Cover
```

Un Wall può generare una CoverOption High per un'unità adiacente, ma quando un effetto deve attraversare fisicamente il muro, il risultato è:

```text
BLOCKED
```

non:

```text
Damage * 0%
```

Questo permette di distinguere correttamente propagazione, LOS, cover e contatto.

---

## 2.4 No-Cross

Default:

```text
Cover = None
BlocksLOS = false
BlocksProjectiles = false
BlocksNormalTraversal = true
BlocksMeleeContact = false
```

No-Cross rappresenta un vincolo di traversal, non necessariamente una barriera fisica.

Una specifica istanza può impostare:

```text
BlocksMeleeContact = true
```

se rappresenta una voragine, vuoto, campo fisicamente invalicabile, ecc.

Non dedurre automaticamente `BlocksMeleeContact` da `BlocksNormalTraversal`.

---

# 3. Melee Cover Rule

Regola canonica:

> Gli attacchi classificati Melee ignorano qualsiasi mitigazione o modificatore derivante direttamente da `CoverLevel` e `CoverArc`. Devono però avere un contatto melee geometricamente valido con il bersaglio.

Quindi:

```text
Melee
 ├─ Cover mitigation      → ignored
 ├─ CoverArc mitigation   → ignored
 ├─ Physical blockers     → respected
 └─ Melee contact test   → required
```

Non implementare:

```cpp
if (Distance == 1)
    IgnoreEverything();
```

L'adiacenza di cella non garantisce il contatto.

---

# 4. Internal Segment Rule

Un segmento interno all'hex segue le stesse regole di un segmento sul bordo.

È possibile avere:

```text
Same FRTCellId
+
Different CoverSide
+
Wall between units
```

e ottenere:

```text
Melee = BLOCKED_BY_GEOMETRY
```

Questo evita il paradosso per cui un muro interno blocca traversal/LOS ma può essere attraversato gratuitamente da un melee solo perché entrambe le unità appartengono allo stesso hex.

`FRTCellId` identifica la posizione logica; non sostituisce la geometria intra-hex.

---

# 5. Melee Contact

Il resolver deve usare una verifica concettualmente equivalente a:

```text
HasValidMeleeContact(Attacker, Target)
```

La verifica considera almeno:

- distanza melee;
- layer / quota compatibile;
- segmenti che separano i due;
- `BlocksMeleeContact`;
- eventuali traversal/reach policy dell'abilità.

Non usare una simulazione fisica frame-based.

Il risultato deve essere deterministico.

---

# 6. Melee Reach

Per armi/abilità con reach superiore:

```text
MeleeContact
```

rimane distinto da:

```text
MeleeReach
```

Esempio:

```text
Spear:
Range > normal melee
```

La cover continua a non fornire mitigazione, ma i blocker applicabili alla traiettoria/reach vengono rispettati.

Un Wall continua a bloccare salvo policy esplicita dell'abilità.

---

# 7. Dash Strike / Charge

Un Dash Strike non attraversa automaticamente cover.

Pipeline:

```text
Dash
→ resolved position
→ resolved Facing
→ MeleeContact
→ Attack
```

Se il Dash incontra Low Cover:

```text
CanVault == true
```

può essere utilizzata una traversal speciale.

Se incontra Wall:

```text
default = blocked
```

La posizione effettiva risolta è autorità; non usare la destinazione prevista dal planning per calcolare il contatto finale.

---

# 8. Vault Attack

Una capacità di tipo Vault può esplicitamente consentire:

```text
Start
→ cross Low Cover
→ landing cell
→ attack
```

Questo non significa che “Melee ignora traversal”.

La traversal speciale appartiene all'abilità.

---

# 9. Teleport / Blink

Default:

```text
Teleport
→ position changes
→ Facing unchanged
```

Un'abilità può dichiarare:

```text
FaceAimAfterTeleport
```

o equivalente.

Un Blink Strike può quindi fare:

```text
Teleport
→ Face Target
→ MeleeContact
→ Attack
```

---

# 10. Forced Movement

Default:

```text
Push/Pull/Knockback
→ position changes
→ Facing unchanged
```

Il forced movement non ruota automaticamente il bersaglio.

Un'abilità può esplicitamente dichiarare:

```text
FaceSource
FaceAwayFromSource
RotateTarget
```

ecc.

---

# 11. Cover Mitigation Model

La cover è una mitigazione deterministica e direzionale.

Non usare RNG per la cover nella v0.1.

Valori placeholder proposti:

```text
None = 100%
Low  = 75%
High = 50%
```

Questi sono valori di balance iniziali, NON ancora valori canonici di produzione se non approvati dal balance owner.

L'importante è il modello:

```text
Cover = deterministic effect multiplier
```

e non:

```text
Cover = miss chance
```

o deviazione casuale.

---

# 12. Cover Direction

La cover è orientata tramite:

```text
CoverArc
```

e la normale/orientamento del segmento.

La domanda corretta è:

```text
Is the incoming effect direction inside CoverArc?
```

non:

```text
Does target have cover?
```

Quindi una stessa unità può essere:

```text
High Cover
```

da una direzione e:

```text
No Cover
```

da un'altra.

---

# 13. Cover Length

La lunghezza del segmento NON moltiplica direttamente il valore della cover.

Separare:

```text
CoverLevel
→ quanto protegge

CoverArc / normal
→ da quali direzioni protegge

Segment geometry / length
→ quali linee di effetto può fisicamente intercettare
```

Conseguenza:

- mezzo lato e lato intero possono avere lo stesso `CoverLevel`;
- il lato intero può comunque schermare più linee/bersagli perché occupa più geometria;
- C→V e mezzo lato possono avere cover tatticamente molto diversa perché hanno normali/archi diversi.

---

# 14. Blast / AoE

Shape != Delivery != Cover Policy.

Una `Area` non deve automaticamente ignorare la cover.

Per un Blast:

```text
EffectOrigin = BlastCenter
```

e per ogni target:

```text
BlastOrigin → Target
```

si valuta propagazione e cover.

Non usare:

```text
Caster → Target
```

per determinare la cover di un'esplosione se l'effetto è realmente originato altrove.

---

# 15. Blast and Wall

Se:

```text
BlastOrigin
→ Wall
→ Target
```

e il Wall blocca la propagazione:

```text
Target = not hit
```

Questo è un blocker, non una mitigazione cover.

Il segmento interno all'hex è trattato allo stesso modo.

---

# 16. Blast Shadow

La geometria deve essere valutata per target.

Un muro corto può schermare un bersaglio ma non un altro:

```text
BlastOrigin
      |
      +---- Wall ---- Target A
      |
      +--------------- Target B
```

A:

```text
blocked
```

B:

```text
affected
```

Non simulare nella v0.1 una propagazione fisica continua che “gira attorno” agli angoli.

Default:

```text
Propagation = direct geometric relation
```

---

# 17. Cone

Il Cone produce candidate cells tramite la propria geometria.

Dopo la geometria:

```text
candidate cells
→ propagation/blocker
→ cover
→ effect
```

Il Wall può creare un'ombra per specifici target/celle.

Non troncare automaticamente tutto il Cone al primo segmento incontrato.

La policy dipende dalla Delivery dell'abilità.

---

# 18. Line

La Line corrente:

```text
Origin → Target
```

termina sul target.

Un Wall fisico normalmente interrompe la linea.

Il piercing è un override dell'abilità, non una proprietà automatica della Shape.

---

# 19. Area non significa Blast

Queste sono entità diverse:

```text
Shape = Area
Delivery = RadialBlast
```

contro:

```text
Shape = Area
Delivery = HealPulse
```

oppure:

```text
Shape = Area
Delivery = EnvironmentalPropagation
```

Quindi non inserire:

```cpp
if (Shape == Area)
    IgnoreCover();
```

Il repository contiene già un comportamento legacy in cui `Area` è accoppiata a una hit rule che ignora cover; questo va trattato come accoppiamento da refactor, non come principio di design.

---

# 20. Facing

`Facing` è uno stato competitivo discreto a sei direzioni.

Non confonderlo con:

```text
CameraYaw
VisualPose
Animation
```

Il Facing è incluso nello stato simulato/snapshot.

---

# 21. Planned Facing

Durante Planning:

```text
CurrentFacing
PlannedFacing
```

Il giocatore può scegliere:

```text
Auto Facing
Manual Facing
```

Default: Auto.

---

# 22. Auto Facing

Priorità proposta:

1. abilità con target/direzione esplicita → verso Aim/Target;
2. Dash + attack → verso Aim/Target dell'attacco;
3. solo Dash → direzione finale del Dash;
4. solo Move → direzione dell'ultima transizione realmente eseguita;
5. nessuna azione direzionale → mantiene Facing corrente;
6. ability-specific override → policy dell'abilità.

Le priorità possono essere formalizzate nel resolver una volta approvate.

---

# 23. Facing During Resolution

Il Facing non deve essere congelato per tutto il turno.

È stato mutabile della simulazione.

Esempio:

```text
Snapshot
Facing = E

Dash
→ Facing = NE

Combat
→ Attack policy
→ Facing = N

Move
→ Facing = SW
```

Ogni cambio significativo deve essere un evento deterministico.

Lo snapshot iniziale resta immutabile.

---

# 24. Facing and Dash

Poiché il macro ordine corrente prevede:

```text
Prep
→ Dash
→ Blast
→ Move
```

un Dash risolto prima del Combat aggiorna la posizione e il Facing che Combat legge.

Il Facing previsto dal planning non è autorità se il Dash viene:

- accorciato;
- bloccato;
- deviato secondo regole;
- invalidato.

Il resolver usa lo stato effettivamente risolto.

---

# 25. Facing and Normal Move

Il normale Move avviene dopo Combat nel modello corrente.

Quindi:

```text
Attack
→ Facing usato dall'attacco

Move
→ Facing finale del movimento
```

Un Move successivo può quindi lasciare l'unità rivolta in una direzione diversa da quella usata durante l'attacco dello stesso turno.

---

# 26. Forced Movement / Teleport and Facing

Default:

```text
Forced Movement → Facing unchanged
Teleport        → Facing unchanged
```

Solo una policy esplicita dell'abilità modifica Facing.

---

# 27. Facing and Cover

Facing e Cover sono ortogonali.

Esempio:

```text
CoverArc = North
Facing   = East
```

non ruota la cover.

La cover appartiene al segmento/cover option.

Il Facing del personaggio entra in gioco solo se una regola/abilità dichiara esplicitamente una dipendenza dal Facing.

---

# 28. Facing Categories

Usare le sei direzioni hex canoniche.

Per un consumer che richiede classificazione relativa:

```text
delta = TargetFacing vs IncomingDirection
```

Mappatura proposta:

```text
0        = Front
±1       = Side / Oblique
±2, ±3   = Rear
```

Non memorizzare necessariamente Front/Side/Rear nello stato: derivarli.

---

# 29. Ability-Specific Facing

Le abilità possono dichiarare policy specifiche:

```text
Basic Rifle
→ Ignore Facing

Backstab
→ Require Rear

Counter Stance
→ Require Front/Side

Directional Shield
→ Apply Defense From Front
```

Questo evita di applicare un bonus Facing universale a tutti gli attacchi.

---

# 30. Reaction / Counter

Una reaction legge il Facing presente al proprio Decision Boundary.

Non necessariamente il Facing iniziale del turno.

Esempio:

```text
Start Facing N
→ Dash E
→ Facing E
→ Reaction opens
→ Reaction reads Facing E
```

---

# 31. AoE Cover Policy

Default:

### Direct attack

```text
CoverOrigin = Attacker
```

### Melee

```text
Cover = ignored
Physical blockers = respected
```

### Blast

```text
CoverOrigin = BlastOrigin
```

### Cone

```text
CoverOrigin = ConeOrigin
```

### Line

```text
CoverOrigin = LineOrigin
```

### IgnoreCover ability

```text
Cover mitigation = bypassed
```

ma i blocker fisici restano applicabili salvo policy esplicita.

---

# 32. Penetration

Penetration è una proprietà della Delivery/Ability, non della Shape.

Esempi concettuali:

```text
No penetration
Penetrate Low Cover
Penetrate High Cover
Penetrate Units
Penetrate selected blockers
```

Un'abilità può ignorare un certo blocker solo se dichiarato esplicitamente.

---

# 33. Elevation

Melee e Reach devono considerare quota/layer.

Non usare soltanto:

```text
HexDistance
```

Un attacco melee richiede una compatibilità verticale.

Una capacità `MeleeReach` può avere una policy verticale più ampia.

Questo prepara il modello multilivello senza introdurre eccezioni al sistema hex.

---

# 34. Environmental Effects

Gli stessi principi si applicano a effetti ambientali:

```text
Water
Fire
Electricity
Smoke
Hazard
```

Non assumere che tutti abbiano le stesse regole di:

```text
Projectile
Blast
LOS
Cover
```

Ogni Delivery/Propagation policy deve dichiarare i propri blocker.

---

# 35. Combat Log / Explainability

Il resolver deve poter spiegare il risultato.

Esempio:

```text
Attack: Rail Shot
Base Damage: 100
Cover: High
Cover Multiplier: 50%
Final Damage: 50
```

Wall:

```text
Attack: Rail Shot
Propagation: BlockedByWall
Final Damage: 0
```

Melee:

```text
Attack: Sword Strike
Cover: Ignored (Melee)
MeleeContact: Valid
Final Damage: 80
```

Blast:

```text
Attack: Grenade
EffectOrigin: Cell X
Target: Cell Y
Wall: Blocking
Result: No Hit
```

Questi reason codes devono essere parte del TurnLog/event explanation e non dipendere da UI/animation timing.

---

# 36. Required Automation Scenarios

Da aggiungere/consolidare:

```text
Combat.Cover.Low.Front
Combat.Cover.Low.Flank
Combat.Cover.High.Front
Combat.Cover.High.Flank
Combat.Cover.Wall.BlocksProjectile
Combat.Cover.Wall.BlocksMelee
Combat.Cover.NoCross.DoesNotBlockMeleeByDefault

Combat.Melee.IgnoresLowCover
Combat.Melee.IgnoresHighCover
Combat.Melee.BlockedByInternalWall
Combat.Melee.SameCellBlockedByInternalWall
Combat.Melee.ReachOverLowCover

Combat.Blast.OriginBasedCover
Combat.Blast.WallBlocksPropagation
Combat.Blast.InternalWallBlocksPropagation
Combat.Blast.PartialWallShadow

Combat.Cone.CoverShadow
Combat.Line.WallStops
Combat.Line.PiercingOverride

Combat.Facing.AutoAfterMove
Combat.Facing.AutoAfterDash
Combat.Facing.ForcedMovementUnchanged
Combat.Facing.TeleportUnchanged
Combat.Facing.ReactionReadsResolvedFacing
```

Golden fixture:

```text
Snapshot
+
AcceptedIntents
+
RulesVersion
+
ResolverConfig
=
Expected TurnLog
+
Expected StateHash
```

---

# 37. Implementation Boundary

La responsabilità dovrebbe essere distribuita così:

```text
Map/Grid
→ Tactical Segment geometry

Cover Service
→ CoverOption
→ CoverArc
→ Cover result

LOS Service
→ visibility

Targeting Service
→ legal target

Trajectory/Propagation Service
→ blockers / delivery path

Melee Contact Service
→ melee reach/contact

Facing Service
→ discrete Facing transitions / relative direction

Ability Data
→ policy / overrides

Turn Resolver
→ order + deterministic application

Presentation
→ animation / VFX / UI only
```

Non far diventare il Cover Service responsabile di LOS, movement e targeting.

---

# 38. Open Decisions

Questa nota NON deve fingere che siano già decise:

1. valori finali Low/High (`75%/50%` sono placeholder);
2. esatte bande angolari Front/Oblique/Flank/Rear per CoverArc;
3. eventuale armor/accuracy vs damage interaction;
4. dettagli finali di piercing;
5. traversal cost/vault cost;
6. interaction di singole abilità con Wall/High Cover;
7. policy definitiva di Facing auto per tutte le categorie di azione;
8. eventuali eccezioni ambientali.

Questi restano decisioni di design/balance da approvare.

---

# 39. Recommended next block

Dopo questa consolidazione il prossimo blocco naturale è:

## Reaction / Counter / Intercept Windows

Da decidere:

```text
quando si apre una Reaction Window;
quali eventi possono triggerarla;
se Dash, Move e Forced Movement hanno trigger diversi;
come interagiscono con Facing;
come interagiscono con Cover;
come si risolvono reaction simultanee;
come funziona il Time Bank;
come si evita che il timing riveli informazioni private;
come viene determinato il Decision Boundary;
come entra tutto nel TurnLog.
```

Questo è il punto in cui Cover + Facing + Movement + Simultaneous Resolution finalmente si incontrano.

---

## 40. Consolidation instructions for Claudia

1. Trattare questa nota come **consolidation input**, non come sostituzione cieca di documenti canonici.
2. Verificare ogni voce contro gli owner document esistenti.
3. Se esiste un conflitto, mantenere il conflitto esplicito e creare una decisione da approvare.
4. Non duplicare sistemi o documenti già esistenti.
5. Aggiornare il Product Map / Feature Map / Scenario Map dove appropriato.
6. Aggiungere gli Automation Scenario ID sopra elencati se non esistono già.
7. Non promuovere i valori placeholder di balance a valori canonici senza approvazione.
8. Conservare il principio:
   `Shape != TargetKind != Delivery != Hit Rule`.
9. Conservare il principio:
   `Cover != LOS != Projectile Blocking != Traversal != Melee Contact`.
10. Mantenere il resolver deterministico e snapshot-based.

---

## Status

```text
Cover / Melee semantic rule:          PROPOSED TO FREEZE
Internal segment rule:                PROPOSED TO FREEZE
Preset semantics:                     PROPOSED TO FREEZE
Blast/Cone/Line separation:           PROPOSED TO FREEZE
Facing state model:                   PROPOSED TO FREEZE
Facing auto-policy:                  PROPOSED — needs final owner review
Cover mitigation numbers:             BALANCE PLACEHOLDER
Reaction/Counter/Intercept:            NEXT DECISION BLOCK
```
