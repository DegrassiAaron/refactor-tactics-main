# RefactorTactics — Showcase v0.1 e integrazione nel codice attuale
## Handoff operativo per Claude Code

**Data di consolidamento:** 2026-08-07  
**Repository:** `DegrassiAaron/refactor-tactics-main`  
**Branch di riferimento:** `main`  
**HEAD osservato durante la stesura:** `ea9009a52b145c645ee98dc84e65807ac6fda7b2`  
**Baseline UE:** Unreal Engine **5.8 / canone 5.8.1**  
**Scopo:** costruire la partita showcase della v0.1 usando il codice esistente, senza reintrodurre sistemi superati e senza creare una seconda implementazione parallela delle regole.

---

# 0. Leggere prima di modificare codice

Questo documento è un **handoff operativo**, non una licenza per implementare tutto in una PR.

Prima di toccare il codice:

1. eseguire `git status`;
2. verificare l'HEAD effettivo;
3. leggere:
   - `docs/design/piano-canonico-mvp.md`;
   - `docs/design/roadmap-checkpoint.md`;
   - `docs/design/roadmap-v0.1.md`;
   - `docs/design/adr-0003-modello-azioni-v01.md`;
   - `docs/design/balance/RT_ActionCatalog_v0.1.md`;
   - `docs/design/balance/RT_HeroCatalog_v0.1.md`;
   - `docs/design/balance/RT_TerrainCatalog_v0.1.md`;
   - `docs/design/spec-terreni-e8.md`;
4. cercare TODO/commenti obsoleti nel codice prima di considerarli verità;
5. controllare le issue ancora aperte delle epic E8, E9 ed E10;
6. compilare **Editor + Game** prima della modifica, così una regressione preesistente non viene attribuita alla feature;
7. eseguire la suite automatica corrente e annotarne il numero reale.

> **Regola di prevalenza:** il repository corrente e `docs/design/piano-canonico-mvp.md` prevalgono sui vecchi PDF.  
> I PDF precedenti sono materiale storico/north-star utile per il design, ma non devono reintrodurre roster, tempi o architetture superate.

---

# 1. Conflitti già risolti: NON reintrodurre vecchio canone

Esistono documenti precedenti della demo che descrivono:

- Aegis;
- Nyx;
- Drift;
- Vex;
- 100 HP per tutti;
- interrupt da 5 secondi;
- una mappa piatta specifica.

Questa versione **non è più il canone operativo**.

## Valori vigenti

| Tema | Vecchio materiale | Stato corrente |
|---|---|---|
| Roster vertical slice | Aegis / Nyx / Drift / Vex | **Flux / Riva / Bastion / Vektor** |
| Formazione default | variabile nei vecchi PDR | **Flux + Riva vs Bastion + Vektor** |
| Griglia | piatta / modelli precedenti | **esagonale assiale, `FRTCellId{X,Y,Layer}`** |
| Gameplay quadrato | presente nello storico | **rimosso** |
| Reazioni v0.1 | vecchie “interrupt window” | attualmente **reazioni pianificate automatiche**; Fast Reaction live è una nuova estensione |
| Finestra Fast Reaction più recente | 5 s nei vecchi PDR | **3 s** nella nuova proposta Fast Reaction |
| GAS | proposto in PDR north-star | **non introdurre ora**; il resolver corrente resta l'autorità |
| Network | north-star / M10 | **fuori scope della v0.1 offline**, ma privacy DTO già rispettata |

Se un commento o un PDF contraddice il codice corrente, verificare prima `piano-canonico-mvp.md` e il catalogo vigente.

---

# 2. Obiettivo di prodotto della showcase

Creare una partita dimostrativa 2v2 che faccia capire, senza una spiegazione teorica lunga, i pilastri di RefactorTactics:

1. pianificazione simultanea;
2. previsione e fallback;
3. movimento su esagoni e micro-step;
4. terreno attivo;
5. acqua + elettricità;
6. fumo / informazione incompleta;
7. reazioni;
8. struttura/topologia modificabile;
9. obiettivo contestabile;
10. TurnLog capace di spiegare il perché dell'esito;
11. determinismo/replay;
12. in prospettiva, Fast Reaction e rumore come informazione.

La showcase deve poter diventare contemporaneamente:

- demo interna;
- scenario di smoke test;
- fixture di integrazione;
- golden replay;
- benchmark del resolver;
- base per tutorial;
- scenario da usare nei playtest di leggibilità.

Nome di lavoro:

`RT_Showcase_Relay_v01`

Mappa/arena:

`L_Showcase_Relay`

Per il primo test headless, **non dipendere subito da un `.umap` complesso**. Il repository possiede già generatori di arena in `URTMatchSetupLibrary`; creare prima una fixture riproducibile in codice/dati, poi un asset d'autore equivalente per la presentazione.

---

# 3. Stato reale del progetto al punto di integrazione

## 3.1 GameMode e allestimento

File:

- `Source/RefactorTactics/RTGameMode.h`
- `Source/RefactorTactics/RTGameMode.cpp`

Comportamento corrente:

- `ARTCameraPawn` come pawn di default;
- `ARTPlayerController`;
- `ARTHUD`;
- cerca o crea `ARTHexMapActor`;
- crea `ARTTurnManager` prima delle unità;
- chiama `SetupHexMatch`;
- supporta:
  - `LevelAsset`;
  - `GeneratedDemoArena`;
  - `GeneratedTestArena`;
- fallback demo con raggio configurabile;
- roster letto dal catalogo degli eroi;
- Team 1 marcato bot.

Formazioni di default:

```cpp
Team0Heroes = { "Hero.Flux", "Hero.Riva" };
Team1Heroes = { "Hero.Bastion", "Hero.Vektor" };
```

### Conseguenza per la showcase

Non creare un secondo GameMode “showcase” con regole duplicate.

Preferire, in ordine:

1. introdurre una **sorgente scenario** o una fixture scenario accanto a `URTMatchSetupLibrary`;
2. lasciare `ARTGameMode` responsabile dell'allestimento;
3. spostare i dati specifici dello scenario fuori da `ARTTurnManager`;
4. solo quando esistono più scenari, estrarre un dominio `Scenario/`.

---

## 3.2 Mappa

File chiave:

- `Map/RTCellId.h`
- `Map/RTHexCellData.h`
- `Map/RTHexMapAsset.*`
- `Map/RTHexMapActor.*`
- `Pathfinding/RTHexPathLibrary.*`

Superfici correnti:

```text
Floor
ShallowWater
Rough
Fire
Conductive
Ice
Void
Smoke
HighGround
```

`FRTHexCellData` contiene oggi:

```text
Id
Height
Surface
MoveCost
bBlocksMovement
bBlocksLineOfSight
```

`FRTHexEdge` contiene:

```text
From
To
Cost
Kind
```

Tipi di transizione:

```text
Stair
Ramp
Bridge
Tunnel
Elevator
Jump
```

### Cosa manca ancora

Non assumere che esistano già:

- cover direzionale per sei bordi;
- integrità della cover;
- strutture runtime;
- stato Open/Closed/Destroyed di una porta;
- ponte attivabile/disattivabile a runtime;
- GraphRevision completa per queste strutture;
- oggetti interagibili;
- obiettivo contestabile.

Queste sono principalmente E9/E10.

---

## 3.3 Terreni: cosa è già attivo

La CP 8.1 ha portato gli otto terreni nel substrato esagonale.

### Già presente

- `Rough`
  - costo alto;
  - blocca Dash/Charge;
- `Ice`
  - scivolamento di 1 cella per il **Move normale** quando restano almeno 2 MP;
  - il movimento extra riusa il resolver a micro-step;
- `Fire`
  - effetto on-enter;
  - 10 danni;
  - `Burning`;
- `Smoke`
  - cap di targeting a 2;
- `ShallowWater`
  - definizione di `Wet`;
  - conducibilità dichiarata;
- `Conductive`
  - conducibilità dichiarata;
- `HighGround`
  - esiste come dato.

### Limiti importanti

- il Dash lineare che termina sul ghiaccio **non** scivola;
- HighGround non ha ancora il consumatore finale della sua identità tattica;
- Wet/Obscured e le durate complete entrano nella CP 8.2;
- propagazione elettrica vera è CP 8.3;
- interazioni acqua/fuoco sono CP 8.4;
- azioni che creano/modificano terreno arrivano dopo.

### Issue correnti da rispettare

**#65 — CP 8.2 Stati temporanei**

Target:

- Wet;
- Burning;
- Electrified;
- Obscured;
- Rooted;
- Exposed;
- Marked;
- Slow;
- scadenza deterministica nel Cleanup.

**#66 — CP 8.3 Propagazione elettrica**

Target:

- acqua/conduttivo;
- massimo 3 celle;
- 20 danni iniziali;
- 12 propagati;
- una unità colpita al massimo una volta per evento;
- ordine:
  `distanza -> CellId -> UnitId`.

**#67 — CP 8.4 Fuoco/acqua**

Target:

- acqua rimuove il fuoco;
- Wet cancella Burning;
- propagazione fuoco limitata e deterministica;
- ogni modifica ambientale va nel TurnLog.

### Regola per Claude

Non anticipare #65/#66/#67 dentro una funzione “showcase only”.

La showcase deve **consumare** i sistemi generali quando arrivano, non crearne versioni speciali.

---

# 4. Motore azioni corrente

File:

- `Ability/RTActionDef.h`
- `Ability/RTActionData.h`
- `Ability/RTCatalogLibrary.*`
- `Turn/RTActionQueue.*`
- `Turn/RTActionEffectLibrary.*`
- `Turn/RTActionFallbackLibrary.*`

## 4.1 Macro-fasi

Il progetto conserva la sequenza Atlas:

```text
Planning
-> Prep
-> Dash
-> Blast
-> Move
-> Cleanup
-> MatchEnded
```

Il catalogo possiede anche le fasi semantiche:

```text
Snapshot
Preparation
FastMovement
NormalMovement
Control
Attack
Environment
Cleanup
```

La fase 20 del catalogo è sdoppiata:

- FastMovement -> macrofase Dash;
- NormalMovement -> macrofase Move.

**Non invertire Blast e Move.**

---

## 4.2 `FRTActionDef`

Campi già disponibili:

```text
ActionId
ResolutionPhase
Priority
RangeCells
CostMP
CooldownTurns
Fallback
Slot
MovementStyle
PropagationLimit
Effects
bAllowsReaction
ReactionTrigger
bCanBeInterrupted
bFriendlyFire
```

Slot:

```text
None
Movement
Main
MovementAndMain
Reaction
```

Fallback:

```text
Stop
Wait
AttackCell
AttackTarget
BasicAttack
Cancel
```

Movement style:

```text
None
Budget
LinearDash
LinearCharge
LinearLeap
```

### Regola

Se una nuova meccanica può essere espressa come dato o policy generale, **non** aggiungere:

```cpp
if (ActionId == "Vektor.InterceptShot")
```

nel `TurnManager`.

L'obiettivo del motore azioni è precisamente evitare questo.

---

# 5. Reazioni: stato reale

File:

- `Turn/RTReactionLibrary.h`
- `Turn/RTReactionLibrary.cpp`
- `Turn/RTTurnManager.cpp`
- `Unit/RTUnit.h`
- test:
  - `RTReactionTests.cpp`
  - `RTDefensiveReactionTests.cpp`
  - `RTInterceptTests.cpp`

`ARTUnit` possiede già:

```cpp
int32 PlannedReactionAbility;
```

Trigger correnti:

```text
None
HitByDirectAttack
AllyHitByDirectAttack
```

Outcome correnti:

```text
Activated
NotTriggered
Unavailable
```

Reazioni generiche già implementate:

- Counter;
- Deflect;
- Intercept.

### Intercept

Ha un pass dedicato prima delle altre reazioni perché cambia il bersaglio del colpo.

Questo ordine è una regola di correttezza, non un dettaglio estetico.

### Limite architetturale attuale

Le reazioni correnti:

- vengono pianificate;
- vengono valutate sui colpi già raccolti del Blast;
- scattano deterministicamente;
- **non chiedono una scelta live al giocatore**;
- non sospendono la simulazione.

Questo è il punto di partenza per Fast Reaction.

---

# 6. Privacy degli intenti: già iniziata, non regredire

La v0.1 resta offline vs bot, ma il codice ha già una disciplina importante.

È stato introdotto il modello:

```text
FRTPlannedIntent
    -> FilterForTeam
    -> FRTIntentView
```

L'HUD non deve leggere il piano completo avversario e poi “nasconderlo graficamente”.

Principio:

> un client non deve poter rivelare un dato che non ha mai ricevuto.

Attualmente:

- alleato: vista completa, reazione compresa;
- avversario non rivelato: nessuna vista;
- avversario rivelato: intento eventualmente osservabile, reazione nascosta.

### Regola per Fast Reaction e rumore

Qualunque nuovo DTO deve essere **whitelist**, non “struttura completa + flag hidden”.

---

# 7. Roster canonico v0.1

# 7.1 Flux

```text
HP             90
MP              5
Vista           6
Push Resist     0
Affinità        Electricity
Debolezza       Water
```

Kit:

| AbilityId | Effetto di design | Stato integrazione |
|---|---|---|
| `Flux.ArcPulse` | 22 dmg, r4 | presente |
| `Flux.LinearDischarge` | 24 dmg, +8 vs Wet | base presente; verificare consumatore Wet |
| `Flux.ConductiveNode` | cella conduttiva 2 turni | identità presente, mutazione cella non completata |
| `Flux.Overload` | AoE 18 + interrupt dispositivi | danno presente; dispositivi futuri |
| `Flux.ReactiveCapacitor` | shield 15 + 10 all'attaccante | hero-specific reaction da riallineare al sistema E5 |

Variante LinearDischarge:

- Concentrated: +6 danni, niente chain;
- Branched: secondo target, -6 per target.

---

# 7.2 Riva

```text
HP             95
MP              5
Vista           5
Push Resist     0
Affinità        Water
Debolezza       Electricity
```

Kit:

| AbilityId | Effetto di design | Stato integrazione |
|---|---|---|
| `Riva.PressureJet` | 16 dmg + Wet + Push 1 | ampiamente rappresentabile |
| `Riva.CircularTide` | heal alleati + Wet nemici | dati presenti; routing effetti ally/enemy incompleto |
| `Riva.FluidTrail` | Dash 3 + acqua lungo percorso | stile lineare presente; creazione acqua manca |
| `Riva.MistVeil` | Smoke r1 | dato identità presente; creazione fumo manca |
| `Riva.FlowReaction` | Reposition 1 dopo attacco | trigger/movimento reaction non supportato |

Variante CircularTide:

- Healing: più cura, niente Wet;
- Impact: meno cura, Push 1.

---

# 7.3 Bastion

```text
HP            120
MP              4
Vista           5
Push Resist     1
Affinità        Structures
Debolezza       Movement
```

Kit:

| AbilityId | Effetto di design | Stato integrazione |
|---|---|---|
| `Bastion.ImpactShot` | 24 dmg, r3 | presente |
| `Bastion.KineticPanel` | cover 30 HP | richiede E9 |
| `Bastion.Reconfigure` | ruota/sposta cover | richiede E9 |
| `Bastion.Ram` | Charge, 20 dmg + Push 1 | buona base già presente |
| `Bastion.Interposition` | intercetta colpo diretto verso alleato | deve riusare semantica `Action.Intercept` |

### Debito importante

Il catalogo dell'eroe contiene ancora commenti risalenti a quando E5 non esisteva.

**E5 ora esiste.**

Prima di aggiungere un nuovo sistema, aggiornare l'eroe per consumare il sistema generico.

---

# 7.4 Vektor

```text
HP            100
MP              6
Vista           6
Push Resist     0
Affinità        Movement
Debolezza       Structures
```

Kit:

| AbilityId | Effetto di design | Stato integrazione |
|---|---|---|
| `Vektor.PulseShot` | 21 dmg, r4 | presente |
| `Vektor.InterceptShot` | reaction su movimento, 16 dmg + stop | non cablata all'eroe |
| `Vektor.PassingBlade` | Linear Dash 3 + 20 a unità attraversate | movement style presente; verificare through-path damage |
| `Vektor.Deflection` | reaction -20 dmg | deve riusare semantica `Action.Deflect` |
| `Vektor.Feint` | marca cella + Reposition | non rappresentata completamente |

Varianti InterceptShot:

- Precise: 20 danni, 1 cella;
- Extended: 14 danni, linea 3 celle.

---

# 8. NON duplicare le reazioni: riallineare gli eroi al motore E5

Prima fetta consigliata per il roster:

## Bastion.Interposition

Non creare una nuova funzione di intercetto.

Riutilizzare la semantica di:

```text
Action.Intercept
Slot = Reaction
Trigger = AllyHitByDirectAttack
Range = 2
```

Conservare però l'ID dell'eroe nel TurnLog.

Pattern consigliato:

```text
Core reaction semantics
    +
Hero-specific ActionId / cooldown / presentation
```

Non sostituire semplicemente `Bastion.Interposition` con `Action.Intercept` se così si perde l'identità dell'azione.

Possibile helper di costruzione:

```cpp
MakeHeroReactionFromCoreAction(
    HeroActionId,
    CoreActionId,
    HeroCooldown,
    HeroSpecificEffects
)
```

Il nome è proposto; adattarlo allo stile reale.

---

## Vektor.Deflection

Stessa strategia:

```text
semantica -> Action.Deflect
identità  -> Vektor.Deflection
```

---

## Flux.ReactiveCapacitor

È più complesso.

Target di design:

```text
on direct hit:
shield 15
counter damage 10
```

Il motore attuale distingue già concetti di Counter e Deflect, ma questa reazione combina più effetti.

Non implementarla con:

```cpp
if Flux...
```

nel `TurnManager`.

Serve una reazione con **lista di effetti on-commit/on-trigger**, o una policy generica capace di produrre più eventi.

---

## Riva.FlowReaction

Richiede:

```text
HitByDirectAttack
-> Reposition 1
```

Questa reazione produce movimento durante un boundary della resolution.

È molto più vicina alla futura infrastruttura Fast Reaction e va rimandata finché il movimento reattivo non ha una semantica deterministica chiara.

---

# 9. Precedente esistente per Overwatch: `FRTSuppressiveZone`

File:

- `Combat/RTOffensiveActionLibrary.h`
- `Combat/RTOffensiveActionLibrary.cpp`

Esistono già:

```text
FRTSuppressiveZone
FRTSuppressionMover
FRTSuppressionHit
MakeSuppressiveZone
ResolveSuppression
```

La soppressione:

- controlla celle;
- osserva i path a micro-step;
- trova il primo nemico che entra;
- applica un ordine totale:
  `StepIndex -> UnitId`;
- una sola attivazione;
- stop del movimento.

Questa è la base più vicina a:

- `Action.SuppressiveLine`;
- `Vektor.InterceptShot`;
- Overwatch.

### Regola

**NON creare una seconda geometria “Overwatch cone/line” indipendente se la logica di celle controllate può essere generalizzata.**

---

# 10. Fast Action / Fast Reaction — nuova decisione di design

La proposta più recente introduce una scelta live durante la resolution.

## Fast Reaction baseline

```text
durata reale: 3.0 s
risposte standard: FIRE / HOLD
timeout: HOLD
charge: 1
max prompt: data-driven, baseline 3
nested interrupt: NO per MVP
```

HOLD:

- scarta solo l'opportunità corrente;
- non consuma la reaction;
- una futura opportunità può ancora comparire.

FIRE:

- committa la reaction;
- consuma la charge;
- applica l'effetto.

Il giocatore non deve sapere in anticipo se arriveranno altri trigger.

---

# 11. Modello concettuale Fast Reaction

```text
Reaction Definition
+
Planning Intent
+
Snapshot / Working Resolution State
+
Current Decision Boundary
=
Reaction Opportunity
```

Una reaction con una charge:

```text
0..N opportunities
0..1 commit
```

## Tipo di decisione

Mantenere semanticamente distinti:

```cpp
enum class ERTFastDecisionType : uint8
{
    Action,
    Reaction
};
```

anche se usano la stessa UI/infrastruttura.

---

# 12. Opportunity, non “interrupt arbitrario”

Tipo proposto, da adattare al codice:

```cpp
USTRUCT(BlueprintType)
struct FRTReactionOpportunity
{
    GENERATED_BODY();

    FName OpportunityId;
    FName ReactionInstanceId;

    int32 OwningUnitId = INDEX_NONE;

    int32 TurnIndex = 0;
    int32 MicroStepIndex = 0;

    TArray<int32> TriggeringUnitIds;
    TArray<ERTReactionResponse> AllowedResponses;

    // NO future positions.
    // NO future triggers.
    // NO enemy canonical intents.
};
```

Per trigger simultanei nello stesso micro-step:

```text
Enemy A enters
Enemy B enters
```

creare **una sola opportunity**:

```text
FIRE A
FIRE B
HOLD
```

Non:

```text
prompt A
poi prompt B
```

perché quell'ordine sarebbe un artefatto dell'iterazione.

---

# 13. Punto architetturale CRITICO: il movimento corrente risolve tutto in una chiamata

Oggi il percorso passa, in sostanza, da:

```cpp
URTHexSimLibrary::ResolveHexPaths(...)
```

che produce il risultato del movimento.

Questo va bene per:

- collisioni simultanee;
- determinismo;
- playback;
- reazioni automatiche post-calcolo.

NON basta per un Overwatch interattivo che può:

```text
entrare in cella
-> trigger
-> FIRE
-> stop movimento
-> cambiare le collisioni dei micro-step successivi
```

Se si aspetta la fine di `ResolveHexPaths` e poi si mostra il prompt, è troppo tardi: il futuro del movimento è già stato calcolato usando uno stato che il colpo avrebbe dovuto modificare.

---

# 14. Integrazione corretta: rendere il resolver di movimento “step-able”

Obiettivo:

estrarre il concetto di **micro-step deterministico** senza perdere il wrapper esistente.

Pattern:

```text
FRTMovementResolutionState
    |
ResolveNextHexMicroStep()
    |
Result / events
    |
Update terrain / visibility
    |
Collect reaction opportunities
    |
if none:
    continue
else:
    suspend at boundary
```

## Wrapper compatibilità

`ResolveHexPaths` deve poter restare:

```text
initialize state
while not finished:
    ResolveNextHexMicroStep
return final result
```

In questo modo:

- tutti i test esistenti continuano a usare `ResolveHexPaths`;
- Fast Reaction usa lo stepper;
- non nascono due algoritmi di collisione;
- bot e resolver condividono ancora le stesse regole.

### NON fare

```cpp
ResolveHexPaths();
for (EnteredCell : FinalResult)
{
    MaybePromptOverwatch();
}
```

se FIRE può interrompere il movimento.

---

# 15. Ordine di un micro-step con Fast Reaction

Baseline proposta:

```text
1. Freeze pre-step movement state
2. Collect attempted transitions
3. Resolve simultaneous conflicts deterministically
4. Apply accepted transitions to working logical state
5. Apply on-enter terrain effects
6. Update derived visibility/detection for current boundary
7. Collect valid reaction triggers
8. Build sanitized opportunities
9. If no interactive opportunity -> continue
10. If opportunity -> suspend logical progression
11. Receive decision command
12. Validate decision against frozen opportunity
13. Apply committed reaction
14. If reaction stops/KO/root/etc -> truncate affected future movement
15. Continue next micro-step
```

È importante scegliere e documentare esattamente se la reaction vede il bersaglio:

- appena prima dell'ingresso;
- oppure nella cella appena raggiunta.

Per `Action.SuppressiveLine` il catalogo corrente dice che il nemico **resta nella cella appena raggiunta**.

Quindi per Vektor InterceptShot/Overwatch movimento la baseline più coerente è:

> il trigger avviene dopo l'ingresso valido nella cella controllata; FIRE colpisce il bersaglio in quella cella e può troncare i passi successivi.

---

# 16. Fast Reaction non deve rompere il determinismo

Oggi l'invariante concettuale è:

```text
same snapshot
+ same committed intents
+ same rules/version
+ same seed
=
same result
```

Con una scelta live durante la resolution, manca un input.

La nuova formula deve diventare:

```text
same snapshot
+ same planning intents
+ same reaction decision commands
+ same rules/version
+ same seed
=
same result
```

Oppure, semanticamente equivalente:

> ogni scelta Fast Reaction è un **nuovo comando autorevole append-only** della stessa esecuzione.

## Replay

Registrare almeno:

```text
OpportunityId
ReactionInstanceId
DecisionBoundary
Response
SelectedTargetId
```

Non registrare come parte dell'hash canonico:

- millisecondi wall-clock;
- durata del VFX;
- frame di presentazione;
- slow-motion.

Il timeout è semplicemente una risposta canonica:

```text
Response = Hold
Reason = Timeout
```

---

# 17. Nessun `Sleep`, `Delay` o timer dentro la logica pura

La parte pura deve poter dire:

```text
ResolutionResult = NeedsDecision(Opportunity)
```

L'orchestratore (`ARTTurnManager` o un oggetto di resolution session dedicato) gestisce:

- sospensione;
- timer reale da 3 s;
- UI;
- eventuale slow-motion di presentazione;
- resume.

Mai:

```cpp
FPlatformProcess::Sleep(...)
```

Mai una Timeline Blueprint come autorità.

Mai aspettare il termine di un montage per proseguire la logica.

---

# 18. Policy utile per migrazione senza regressioni

Non trasformare tutte le reazioni esistenti in prompt interattivi.

Introdurre una policy generica, ad esempio:

```text
AutoCommit
PromptOwner
```

Nomi da adattare.

Uso:

```text
Action.Counter       -> AutoCommit
Action.Deflect       -> AutoCommit
Action.Intercept     -> AutoCommit

Vektor.InterceptShot -> PromptOwner
Overwatch            -> PromptOwner
```

Vantaggi:

- E5 continua a funzionare;
- la showcase può sperimentare Fast Reaction senza riscrivere tutto;
- test legacy restano significativi;
- si può misurare il pacing prima di rendere altre reazioni interattive.

---

# 19. Timeout e bot

Per una opportunity interattiva:

### Giocatore

```text
3 s
FIRE / HOLD
timeout = HOLD
```

### Bot

Niente attesa reale.

Il bot usa una policy deterministica:

```text
ChooseReactionResponse(CurrentOpportunity, PublicKnowledge, BotWeights)
```

e restituisce immediatamente un comando.

Il bot NON deve leggere:

- future path;
- trigger futuri;
- risultato precalcolato del resto del turno.

Deve decidere con lo stesso “presente informativo” del giocatore.

---

# 20. Privacy di `FRTReactionOpportunity`

Il client autorizzato riceve soltanto informazioni relative al boundary corrente.

Può ricevere:

- target attualmente valido e percepibile;
- cella corrente osservabile;
- opzioni legali;
- durata della finestra;
- reaction propria;
- eventuali modificatori pubblici.

Non riceve:

- “mancano altri 2 trigger”;
- destinazione finale nemica;
- path rimanente;
- ability intent nemico non osservato;
- future opportunity;
- risultato già precalcolato della resolution.

Il server, in futuro, può conoscere tutto il CanonicalIntentStore. Il DTO non deve riflettere questa conoscenza.

---

# 21. Fast Reaction UI

UI minimale:

```text
┌──────────────────────────────┐
│ INTERCEPT SHOT               │
│ Target: Riva                 │
│ Cell: q,r,L                  │
│                              │
│ [ FIRE ]       [ HOLD ]      │
│             2.8              │
└──────────────────────────────┘
```

Se due target simultanei:

```text
[ FIRE FLUX ]
[ FIRE RIVA ]
[ HOLD ]
```

### Presentazione

Durante il decision boundary:

```text
logical simulation: frozen
presentation: optionally 10-20% speed
real-time countdown: 3 s
```

La presentazione non cambia:

- seed;
- ordine;
- collisioni;
- danno;
- LOS logica;
- opportunity.

---

# 22. Showcase target — “Il Relè”

## Squadre

### Team 0
- Flux
- Riva

### Team 1
- Bastion
- Vektor

## Mappa target

Elementi desiderati:

- Relay A centrale;
- Relay B su piattaforma/settore secondario;
- ShallowWater;
- Conductive;
- Smoke producibile;
- un tratto Ice;
- un tratto Rough;
- almeno un percorso alternativo;
- in versione E9:
  - low cover direzionale;
  - KineticPanel;
  - porta/ponte;
- in versione E10:
  - obiettivo contestabile.

### Importante

La **relocation del Relay** e il “primo a 4 punti” sono design specifico della showcase, non una regola globale già canonica.

Non scrivere:

```cpp
if (Turn == 4)
{
    MoveRelay();
}
```

in `ARTTurnManager`.

Quando arriverà, deve essere uno scenario/objective data-driven.

---

# 23. Sequenza narrativa target degli 8 turni

Questa è la **showcase finale**, non la lista di feature già giocabili oggi.

## Turno 1 — Mappa e planning simultaneo

Blu:

- Riva usa `FluidTrail` verso il centro;
- Flux prende posizione.

Rosso:

- Bastion crea `KineticPanel`;
- Vektor prende una linea utile.

Mostrare:

- path alleati;
- intent alleati;
- Dash;
- terreno modificato;
- struttura direzionale.

### Dipendenze mancanti

- FluidTrail deve creare acqua;
- KineticPanel richiede E9.

---

## Turno 2 — Setup Wet

Riva:

```text
PressureJet -> Vektor
```

Effetti:

```text
16 dmg
Wet
Push 1
```

Flux:

```text
ConductiveNode
```

Obiettivo UX:

il giocatore vede chiaramente che Riva sta preparando qualcosa per Flux.

### Stato corrente

PressureJet è un buon candidato da rendere completamente funzionante subito.

ConductiveNode richiede mutazione ambiente/cella.

---

## Turno 3 — Previsione e fallback

Flux pianifica:

```text
LinearDischarge -> Vektor
```

Vektor usa mobilità rapida.

Il bersaglio si sposta prima del Blast.

Mostrare:

```text
Target moved
Fallback.AttackCell
```

o il fallback realmente dichiarato dalla definizione corrente.

### Obiettivo

Far capire:

> non stai ordinando una sequenza cinematica; stai dichiarando un piano che verrà rivalidato.

---

## Turno 4 — InterceptShot / Fast Reaction

Il percorso verso l'obiettivo passa in una zona controllata da Vektor.

Flux entra per primo.

Prompt:

```text
FIRE / HOLD
```

Vektor HOLD.

Riva entra in seguito.

Seconda opportunity:

```text
FIRE / HOLD
```

Vektor FIRE.

Riva:

- prende danno;
- il movimento si ferma nella cella raggiunta.

### Questa è la feature Fast Reaction firma

Richiede il resolver micro-step sospendibile descritto sopra.

---

## Turno 5 — Smoke e certezza

Riva usa `MistVeil`.

La UI passa da:

```text
Confermato
```

a:

```text
Previsto / Incerto
```

per ciò che dipende dalla posizione nemica.

### Stato corrente

Smoke ha già un cap di targeting.

La vera classificazione visibilità/FoW è ancora una feature successiva.

Per la prima showcase tecnica si può mostrare solo il cap di targeting, senza fingere FoW che non esiste.

---

## Turno 6 — Bastion protegge

Riva usa PressureJet su Bastion:

- danno;
- Wet;
- Push.

Bastion mostra Push Resistance.

Flux attacca Vektor.

Bastion usa:

```text
Interposition
```

Il bersaglio del colpo viene rediretto su Bastion.

### Prima integrazione raccomandata

Mappare `Bastion.Interposition` al sistema `Action.Intercept` già esistente.

---

## Turno 7 — Combo acqua/elettricità

La rete di acqua/conduttivo è pronta.

Flux usa LinearDischarge / propagazione elettrica.

Mostrare:

```text
source damage
propagated damage
ordered propagation
each unit once
```

È il payoff ambientale principale.

### Dipendenza

CP 8.3.

---

## Turno 8 — Obiettivo > deathmatch

Flux può essere messo KO.

Riva sopravvive sull'obiettivo.

Bastion/Vektor non riescono a contestare nel Cleanup.

Il punto obiettivo viene assegnato **dopo ambiente/KO**, come previsto dall'E10.

La squadra può quindi vincere anche con Flux KO.

### Obiettivo

Far capire:

> la partita non è solo eliminazione.

---

# 24. Versione “Showcase Lite” giocabile prima del completamento

Non aspettare E8-E10 per costruire la prima fixture.

Creare prima una sequenza che usa soltanto funzioni già atterrate.

## Showcase Lite suggerita

1. spawn Flux/Riva vs Bastion/Vektor;
2. path su arena generata;
3. Rough nega un Dash;
4. Ice causa sliding nel Move;
5. Fire applica on-enter;
6. Smoke limita range;
7. Riva PressureJet applica danno/Wet/Push;
8. Bastion Ram usa LinearCharge;
9. generic Counter/Deflect/Intercept;
10. fallback su target mosso;
11. TurnLog;
12. ripetizione con stesso input produce stesso risultato.

Questa fixture deve essere aggiunta **prima** di nuove regole complesse.

---

# 25. Integrare la showcase per fette, non come mega-feature

## Fetta A — Scenario fixture senza nuove regole

Obiettivo:

```text
RT_Showcase_Relay_v01_Lite
```

File probabili:

- `Turn/RTMatchSetupLibrary.*`
- nuovo test `Tests/RTShowcaseScenarioTests.cpp`
- documento `docs/design/showcase-v0.1.md`

Fare:

- arena generata deterministica;
- coordinate documentate;
- surface esistenti;
- spawn canonico;
- seed fisso;
- nessuna cover dinamica;
- nessun objective nuovo.

Gate:

- Editor compile;
- Game compile;
- suite verde;
- scenario ripetuto N volte = stesso log/hash disponibile.

---

## Fetta B — Riallineamento hero reactions a E5

Prima di inventare Fast Reaction:

- Bastion.Interposition -> semantica Intercept;
- Vektor.Deflection -> semantica Deflect;
- audit dei commenti “E5 manca” nel hero catalog;
- test per HeroId + reaction semantics.

Possibili test:

```text
RefactorTactics.Heroes.BastionInterpositionUsesReactionSlot
RefactorTactics.Heroes.BastionInterpositionRedirectsDirectHit
RefactorTactics.Heroes.VektorDeflectionReducesDirectHit
```

---

## Fetta C — Stati E8.2

Seguire issue #65.

Non aggiungere requirement extra solo per la showcase.

Al termine:

- Wet coerente;
- Burning cleanup;
- Exposed/Marked/Slow coerenti;
- log e duration deterministici.

---

## Fetta D — Elettricità E8.3

Seguire issue #66.

Dopo questa fetta:

- la combo firma Flux/Riva può diventare un gate della showcase.

---

## Fetta E — Acqua/fuoco E8.4 e azioni ambiente

Abilitare:

- CreateWater;
- TerrainChange;
- MistVeil;
- ConductiveNode;
- FluidTrail realmente ambientale.

---

## Fetta F — Strutture E9

Issue #69 e seguenti.

Solo qui la showcase deve iniziare a dipendere da:

- cover direzionale;
- KineticPanel;
- Reconfigure;
- porte;
- ponti;
- graph revision.

---

## Fetta G — Obiettivo E10

Prima:

- objective statico contestabile.

Poi:

- eventuale relocation Relay come feature scenario.

Non costruire relocation prima dell'obiettivo base.

---

## Fetta H — Fast Reaction / Overwatch

Questa è una **decisione di scope nuova** rispetto all'attuale E5.

Prima del codice:

1. aggiornare/aggiungere ADR;
2. aggiornare il modello di determinismo/replay;
3. definire Opportunity;
4. estrarre movement stepper;
5. aggiungere policy AutoCommit/PromptOwner;
6. cablare Vektor.InterceptShot;
7. solo dopo creare Overwatch generico se serve.

---

## Fetta I — Rumore / percezione

Non bloccare la v0.1 base se non viene esplicitamente promosso a scope.

Costruirlo dopo avere almeno un modello chiaro di:

- visibility;
- detection;
- team knowledge.

---

# 26. Rumore — design da preservare

Il rumore non è un debuff.

È una seconda fonte di informazione.

Modello:

```text
Simulation
-> Noise Events
-> Acoustic Propagation
-> Unit Perception
-> Team Merge
-> Team Knowledge
-> Sanitized UI
```

Non:

```text
hidden enemy state
-> enemy UI
```

---

# 27. `FRTNoiseEvent` proposto

Tipo concettuale:

```cpp
USTRUCT()
struct FRTNoiseEvent
{
    GENERATED_BODY();

    int32 SourceUnitId = INDEX_NONE;
    FRTCellId OriginCell;

    FGameplayTag NoiseType;
    int32 Intensity = 0;

    int32 TurnIndex = 0;
    int32 MicroStepIndex = 0;
};
```

Evitare subito campi non consumati.

Possibili estensioni future:

- ability source;
- decoy flag;
- persistent duration;
- propagation profile.

---

# 28. Intensità iniziali — SOLO baseline di bilanciamento

Valori indicativi, data-driven:

| Evento | Noise |
|---|---:|
| Wait | 0 |
| Sneak | 0-1 |
| Walk | 1 |
| Move | 2 |
| Sprint | 5 |
| Dash | 6 |
| porta aperta | 2 |
| porta forzata | 7 |
| melee | 3 |
| rifle | 7 |
| explosion | 10 |
| collapse | 10 |

Non incidere questi numeri in `ARTTurnManager`.

---

# 29. Propagazione acustica

Usare il grafo esagonale/multilivello.

NON usare `SphereOverlap` come autorità.

Formula concettuale:

```text
Remaining =
    SourceIntensity
    - DistanceCost
    - AcousticOcclusion
    - AmbientMask
    + SurfaceModifiers
```

Tutto intero.

Niente:

```text
65% chance to hear
```

Baseline:

```text
ReceivedNoise >= HearingThreshold
```

---

# 30. Informazione sonora

Livelli possibili:

```text
1 Direction
2 BroadArea
3 NarrowArea
4 ExactCell
5 Identification
```

Una squadra può sapere:

```text
rumore a Nord-Est
```

senza sapere:

```text
Vektor è in (q=3,r=-1,L0)
```

---

# 31. Memoria sonora

Il Team Knowledge può contenere:

```text
LastHeardTurn
LastHeardMicroStep
LastHeardArea
NoiseType
Confidence
```

Ma non deve “seguire” segretamente una sorgente dopo che non viene più percepita.

---

# 32. Rumore + Fast Reaction

In futuro:

```text
Noise event
-> acoustic detection
-> valid reaction trigger
-> opportunity
```

Esempio:

```text
FOOTSTEPS DETECTED
possible hostile approaching

[ARM / FIRE OVERWATCH]
[TAKE COVER]
```

Ma il trigger deve derivare dall'evento realmente percepito, non dall'intento nemico.

---

# 33. Perception domain consigliato

Quando entra nello scope, creare un dominio dedicato, per esempio:

```text
Source/RefactorTactics/Perception/
    RTNoiseTypes.h
    RTAcousticPropagationLibrary.h/.cpp
    RTTeamKnowledge.h
```

Non mettere:

- sound propagation nel HUD;
- hearing nel TurnManager;
- conoscenza di squadra dentro gli Actor nemici.

Le funzioni di propagazione devono essere pure/testabili.

---

# 34. UI: Confermato / Previsto / Incerto

La UI deve distinguere:

## Confermato

Deriva da:

- stato pubblico;
- evento osservato;
- regola deterministica senza dipendenza avversaria nascosta.

## Previsto

Deriva da:

- propri intenti;
- intenti alleati;
- stato pubblico;
- una previsione ammessa.

## Incerto

Dipende da:

- azione avversaria;
- posizione futura non nota;
- rumore localizzato in area;
- target che potrebbe spostarsi;
- visibilità incompleta.

### Regola

I reason code devono arrivare dal resolver/TurnLog.

La UI non deve ricalcolare da sola il “perché”.

---

# 35. Objective System per il Relay

Issue #75 definisce già una baseline importante:

- controllo verificato in Cleanup;
- dopo ambiente e KO;
- tie = nessun progresso;
- progresso intero;
- TurnLog;
- HUD.

La showcase deve costruirsi sopra questa regola.

## Relocation del Relay

È una feature scenario separata.

Proposta dati:

```text
ObjectivePhase[]
- StartTurn
- ActiveCells
- Duration / EndTurn
```

oppure uno schedule scenario equivalente.

Non implementare la struttura precisa prima di aver letto l'implementazione E10 corrente al momento del lavoro.

---

# 36. Strutture e GraphRevision

Quando E9 entra:

- cover direzionale è associata a uno dei 6 bordi;
- low cover riduce 10 danni diretti dal lato protetto;
- integrità 30;
- aggiornare versione formato mappa;
- migrare asset;
- hash deterministico.

KineticPanel deve usare lo stesso modello di struttura/cover.

Reconfigure non deve muovere una mesh e poi “sperare” che path/LOS la seguano.

Flusso corretto:

```text
logical structure mutation
-> graph/cover revision
-> invalidate caches
-> TurnLog Environment/Structure event
-> presentation update
```

---

# 37. TurnLog della showcase

La showcase deve essere spiegabile.

Eventi minimi da poter ricostruire:

```text
ActionDeclared
MoveStarted
MoveStep
MoveBlocked
AbilityResolved / Fallback
DamageApplied
StatusChanged
ReactionArmed
ReactionOpportunity
ReactionDecision
ReactionResolved
EnvironmentChanged
StructureChanged
ObjectiveUpdated
UnitKO
TurnEnded
```

Non tutti questi tipi esistono oggi come enum dedicati.

Non aggiungerli tutti insieme.

Estendere il log per la feature che ne ha bisogno e aggiungere il test nello stesso checkpoint.

---

# 38. Stable IDs per Opportunity

Non usare GUID casuale generato a runtime se entra nel replay hash.

Un OpportunityId può essere derivato deterministicamente da:

```text
Turn
MacroPhase
MicroStep
ReactionOwnerId
ReactionDefinitionId
OpportunitySequenceWithinBoundary
```

Con serializzazione stabile.

Esempio concettuale:

```text
T07.Move.S03.U12.VektorInterceptShot.O00
```

La forma esatta è da decidere, ma deve essere:

- riproducibile;
- debug-friendly;
- indipendente dal frame;
- indipendente dall'ordine di una TMap.

---

# 39. Ordinamento delle opportunity

Quando più reaction diverse scattano allo stesso boundary, dichiarare un ordine totale.

Baseline:

```text
ReactionPriority
-> ActionPriority
-> StableUnitId
-> ReactionInstanceId
```

Se esiste un'iniziativa futura, inserirla esplicitamente.

Non dipendere da:

- `TMap`;
- `TSet`;
- ordine di spawn;
- pacchetti client;
- Tick.

---

# 40. Candidate target simultanei

Dentro una stessa opportunity:

```text
TriggeringUnitIds
```

ordinare con chiave stabile.

Per una reaction movement-entry potrebbe essere:

```text
StepIndex è uguale per costruzione
-> CellId
-> UnitId
```

Se due unità entrano nella stessa cella, il movimento simultaneo dovrebbe già aver risolto la contesa prima della reaction.

---

# 41. Knowledge boundary per il bot

La showcase è offline vs bot, ma il bot non deve “barare”.

Quando decide FIRE/HOLD:

input permessi:

- opportunity sanitizzata;
- public state;
- knowledge del team bot;
- cooldown/resource propri;
- weights.

Input vietati:

- `PlannedPath` completo del giocatore se non condivisibile;
- future movement steps;
- future reaction opportunities;
- target finali nascosti.

Questo rende il bot utile anche come test della futura privacy di rete.

---

# 42. Test automatici della showcase

Non fare affidamento solo su un test enorme di 8 turni.

Usare piramide:

## Unit/pure

- geometry;
- opportunity creation;
- deterministic ordering;
- response validation;
- terrain propagation.

## Integration

- TurnManager + 4 units;
- hero-specific reactions;
- movement interruption;
- objective cleanup.

## Golden scenario

- script 8 turni;
- same inputs/decisions;
- same log/state hash.

---

# 43. Nomi test proposti

## Showcase

```text
RefactorTactics.ShowcaseRelay.DeterministicReplay
RefactorTactics.ShowcaseRelay.WetEnablesFluxCombo
RefactorTactics.ShowcaseRelay.InterpositionRedirectsHit
RefactorTactics.ShowcaseRelay.ObjectiveCheckedAfterKO
RefactorTactics.ShowcaseRelay.FinalStateHashStable
```

## Fast Reaction

```text
RefactorTactics.FastReaction.HoldKeepsReactionArmed
RefactorTactics.FastReaction.CommitConsumesCharge
RefactorTactics.FastReaction.TimeoutMapsToHold
RefactorTactics.FastReaction.SimultaneousTargetsShareOpportunity
RefactorTactics.FastReaction.PermutationInvariant
RefactorTactics.FastReaction.KOCancelsArmedReaction
RefactorTactics.FastReaction.DoesNotExposeFutureTriggers
```

## Movement boundary

```text
RefactorTactics.FastReaction.FireTruncatesFutureMovement
RefactorTactics.FastReaction.HoldResumesSameMovementState
RefactorTactics.FastReaction.InterruptionAffectsLaterCollision
```

## Noise

```text
RefactorTactics.Noise.PropagationDeterministic
RefactorTactics.Noise.UsesGraphNotEuclideanRadius
RefactorTactics.Noise.OcclusionAttenuates
RefactorTactics.Noise.MemoryDoesNotTrackUnseenSource
RefactorTactics.Noise.NoHiddenIntentLeak
```

---

# 44. Attenzione al naming dei test Unreal

È già stato trovato un difetto in cui un test aveva un nome che era **prefisso gerarchico** di altri test e veniva saltato dal framework.

Quindi evitare una struttura tipo:

```text
Reactions.Intercept
Reactions.Intercept.RejectsAoE
```

se entrambi sono test eseguibili.

Usare foglie univoche:

```text
Reactions.InterceptRedirectsDirectHit
Reactions.InterceptRejectsAoE
```

o una convenzione che non crea un nodo eseguibile padre di altri nodi.

---

# 45. Golden scenario: formato degli input

Non legare il golden test a click o UMG.

Definire una fixture logica:

```text
Turn 1
  Flux:
    MoveIntent...
    MainAction...
    Reaction...
  Riva:
    ...
  Bastion:
    ...
  Vektor:
    ...

ReactionDecisions:
  Boundary X -> HOLD
  Boundary Y -> FIRE target Riva
```

Il test deve poter alimentare il resolver senza interazione reale.

La UI è un consumer degli stessi comandi.

---

# 46. Due modalità per Fast Reaction nei test

## Modalità deterministica immediata

`DecisionProvider` di test:

```text
OpportunityId -> canned response
```

Nessun timer reale.

## Modalità Functional/UI

- apre widget;
- attende input reale;
- verifica timer/timeout.

Non usare la seconda per golden determinism.

---

# 47. Proposed architecture — da adattare, non copiare alla cieca

Possibile separazione:

```text
Turn/
    RTReactionLibrary.*             // trigger già esistenti
    RTReactionOpportunityTypes.h    // nuovo
    RTReactionOpportunityLibrary.*  // puro
    RTResolutionSession.*           // solo se TurnManager diventa ingestibile

Combat/
    RTOffensiveActionLibrary.*      // suppression/controlled cells

UI/
    RTFastDecisionViewModel.*
    WBP_FastDecision                // Blueprint/UMG

Perception/                         // solo quando entra rumore/FoW
    RTNoiseTypes.h
    RTAcousticPropagationLibrary.*
    RTTeamKnowledge.*
```

Prima di creare `RTResolutionSession`, verificare se l'estrazione riduce davvero il `TurnManager`.

Non creare classi “per completezza”.

---

# 48. Build.cs

`RefactorTactics.Build.cs` corrente include già:

```text
Core
CoreUObject
Engine
InputCore
EnhancedInput
GameplayTags
```

e `UnrealEd` solo in build Editor.

### NON aggiungere automaticamente

- GameplayAbilities;
- GameplayTasks;
- CommonUI;
- AIModule;
- altri moduli;

solo perché compaiono in un PDR precedente.

Aggiungere una dipendenza **solo quando il codice nuovo include realmente un'API di quel modulo**.

Il pathfinding corrente è già implementato nel progetto; non migrare a un'altra API solo per la showcase.

---

# 49. GAS

Non introdurre GAS come prerequisito della showcase.

La disciplina resta:

```text
C++ resolver decides
Data defines variation
Presentation mirrors result
```

Se GAS verrà aggiunto dopo:

- costi;
- cooldown;
- attributi;
- gameplay effects;
- cues;

possono essere mirror/integrazione.

Non deve diventare l'autorità degli esiti deterministici.

---

# 50. Passo-passo operativo consigliato per Claude

## Passo 1 — Audit

Eseguire ricerche:

```text
rg "E5|reaction|Reaction" Source/RefactorTactics/Ability Source/RefactorTactics/Turn
rg "SuppressiveZone|ResolveSuppression" Source/RefactorTactics
rg "Hero.Bastion|Bastion.Interposition" Source/RefactorTactics
rg "Vektor.InterceptShot|Vektor.Deflection" Source/RefactorTactics
rg "Wet|Burning|Smoke|Conductive" Source/RefactorTactics
```

Annotare:

- commenti obsoleti;
- funzioni già riusabili;
- test già esistenti.

Non modificare ancora.

---

## Passo 2 — Documento scenario

Creare:

```text
docs/design/showcase-v0.1.md
```

Deve separare:

```text
CANONICAL CURRENT
TARGET SHOWCASE
SCOPE DELTA / EXPERIMENTAL
```

In particolare Fast Reaction e Noise devono essere marcati come **nuove decisioni** finché non sono promossi nel canone.

---

## Passo 3 — Generated Relay fixture

Estendere `RTMatchSetupLibrary` o introdurre il minimo helper scenario.

Obiettivo:

- layout esagonale deterministico;
- superficie;
- archi;
- start cells;
- nessuna nuova regola.

Aggiungere test che verificano coordinate/numero celle/superfici.

---

## Passo 4 — Hero reaction alignment

Rimuovere debito E5 dai quattro eroi una reaction alla volta.

Prima:

- Bastion Interposition;
- Vektor Deflection.

Dopo:

- Flux ReactiveCapacitor;
- Riva FlowReaction.

Ogni PR:

- modifica dati/helper;
- test behavior;
- update commenti/catalogo;
- niente branch speciale nel TurnManager se evitabile.

---

## Passo 5 — Attendere/implementare E8 in ordine

Non saltare:

```text
8.2 states
-> 8.3 electricity
-> 8.4 water/fire
-> 8.5 environmental actions
```

La showcase si aggiorna quando ogni gate è verde.

---

## Passo 6 — E9 structure

Integrare:

```text
KineticPanel
Reconfigure
cover
door/bridge
graph revision
```

come sistemi generali.

---

## Passo 7 — E10 objective

Aggiungere objective statico contestabile.

Solo dopo aggiungere relocation Relay scenario-specific.

---

## Passo 8 — ADR Fast Reaction

Prima di implementare l'interazione live:

documentare:

- nuovo input durante resolution;
- formula deterministica aggiornata;
- timeout;
- replay;
- opportunity;
- no nested interrupts;
- policy AutoCommit/PromptOwner;
- privacy;
- movement decision boundary.

Questa è una modifica architetturale, non solo UI.

---

## Passo 9 — Refactor micro-step

Estrarre dal resolver di movimento un'API step-able.

Gate:

- tutti i vecchi test movimento restano verdi;
- `ResolveHexPaths` produce gli stessi risultati byte/logicamente equivalenti;
- permutation tests restano verdi.

Solo dopo inserire Fast Reaction.

---

## Passo 10 — Vektor.InterceptShot

Prima reaction interattiva.

È perfetta perché:

- esiste già come identità hero;
- il catalogo ha una variante;
- `FRTSuppressiveZone` è già il precedente tecnico;
- l'effetto “stop movement” obbliga a provare davvero il boundary.

---

## Passo 11 — UI decision window

Solo dopo che la decisione funziona headless.

UI:

- DTO sanitizzato;
- FIRE/HOLD;
- countdown;
- timeout -> HOLD;
- no business logic nel widget.

---

## Passo 12 — Golden 8 turns

Quando i sistemi sono disponibili:

- costruire input script;
- decision script;
- expected events;
- final state;
- LogHash/StateHash.

---

# 51. Debug richiesto

Aggiungere logging utile, non spam.

Per Fast Reaction:

```text
[RT][Reaction]
Turn
Phase
MicroStep
OpportunityId
Owner
CandidateCount
Response
Target
Outcome
```

Per movimento:

```text
DecisionBoundary entered/resumed
Paths truncated by reaction
```

Per noise futuro:

```text
NoiseEvent
ReceivedIntensity
ListenerTeam
LocalizationLevel
```

Mai loggare sul client avversario dati che il client non dovrebbe ricevere.

---

# 52. Performance

Fast Reaction non deve trasformare il resolver in Tick-driven.

Il micro-step è una struttura logica, non un tick Unreal.

Il resolver può:

- processare molti micro-step immediatamente;
- fermarsi solo quando compare un boundary interattivo.

UI e animazione possono impiegare secondi.

La logica no.

---

# 53. Errori da evitare

1. Reintrodurre Aegis/Nyx/Drift/Vex.
2. Creare un nuovo sistema quadrato.
3. Hardcodare gli 8 turni nel TurnManager.
4. Creare `if (HeroId == ...)` per ogni hero effect.
5. Duplicare la geometria della SuppressiveLine per Overwatch.
6. Calcolare Overwatch dopo che tutto il movimento è già risolto.
7. Usare `Delay`/Timeline come logica.
8. Usare il frame rate per la finestra logica.
9. Fare timeout = FIRE.
10. Informare il player che “arriveranno altri trigger”.
11. Sequenziare due trigger simultanei usando l'ordine di array.
12. Far leggere al bot il path futuro.
13. Far leggere all'HUD il CanonicalIntentStore.
14. Usare `TMap`/`TSet` come ordine competitivo.
15. Generare GUID random dentro il replay canonico.
16. Introdurre network completo ora.
17. Introdurre GAS ora.
18. Anticipare E9 dentro un “KineticPanel showcase-only”.
19. Anticipare E10 dentro un `if Turn == 4`.
20. Implementare Rumore con `SphereOverlap` come autorità.
21. Nascondere dati segreti solo via widget.
22. Chiudere una feature senza Automation Test e packaged/PIE gate pertinente.
23. Fidarsi di commenti TODO obsoleti senza verificare il codice corrente.
24. Fare una PR con E8+E9+E10+FastReaction insieme.

---

# 54. Commit sequence consigliata

```text
docs(showcase): define Relay v0.1 golden scenario and scope deltas

test(showcase): add deterministic generated Relay fixture

feat(heroes): align Bastion and Vektor reactions with E5 core

feat(environment): complete temporary status interactions

feat(environment): add deterministic water-electric propagation

feat(environment): wire hero terrain creation abilities

feat(structures): add directional cover and kinetic panel

feat(objectives): add contestable Relay objective

docs(adr): define resolution decision boundaries and fast reactions

refactor(turn): expose deterministic hex movement micro-step state

feat(reactions): add reaction opportunities and decision commands

feat(vektor): wire InterceptShot through fast reaction opportunity

feat(ui): add three-second fast reaction window

test(showcase): add Relay golden replay and final hash
```

Adattare la sequenza alle issue già in corso: non duplicare checkpoint esistenti.

---

# 55. Definition of Done per Fast Reaction

La feature non è fatta finché:

- [ ] la reaction si prepara nel planning;
- [ ] trigger al micro-step corretto;
- [ ] HOLD non consuma charge;
- [ ] una nuova opportunity può comparire dopo HOLD;
- [ ] FIRE consuma charge;
- [ ] timeout = HOLD;
- [ ] due target simultanei = una opportunity multi-target;
- [ ] KO/Stun/cancel invalidano reaction;
- [ ] nessun nested interrupt MVP;
- [ ] TurnLog registra opportunity + decision + outcome;
- [ ] replay con decisioni registrate converge;
- [ ] permutation test verde;
- [ ] il bot decide senza future knowledge;
- [ ] il DTO non contiene future trigger/path;
- [ ] UI non decide l'esito;
- [ ] slow-motion non cambia il risultato;
- [ ] build Editor verde;
- [ ] build Game verde;
- [ ] test automatici verdi;
- [ ] PIE manuale della finestra da 3 secondi;
- [ ] packaged verification quando la milestone lo richiede.

---

# 56. Definition of Done per showcase finale

- [ ] Flux/Riva vs Bastion/Vektor.
- [ ] mappa Relay riproducibile.
- [ ] 8-turn scripted scenario disponibile come fixture.
- [ ] movimento/fallback leggibili.
- [ ] almeno un terrain interaction reale.
- [ ] Wet + Electricity reale.
- [ ] Smoke reale.
- [ ] Bastion Interposition reale.
- [ ] KineticPanel reale.
- [ ] Vektor InterceptShot/Fast Reaction reale, se Fast Reaction viene approvata per v0.1.
- [ ] objective contestabile reale.
- [ ] KO non implica automaticamente vittoria se l'obiettivo chiude il match.
- [ ] TurnLog spiega gli eventi chiave.
- [ ] expected StateHash.
- [ ] expected LogHash.
- [ ] ripetizione deterministica.
- [ ] test con ordine input/container permutato.
- [ ] nessuna dipendenza da frame rate.
- [ ] screenshot/video/playtest solo dopo che la logica è verificata.

---

# 57. Rumore — scope recommendation

Il sistema Rumore è forte e coerente con l'identità del prodotto, ma **non deve diventare una dipendenza obbligatoria della prima showcase tecnica** finché manca il dominio completo di FoW/TeamKnowledge.

Ordine consigliato:

```text
Showcase core
-> terrain/status
-> structures/objective
-> Fast Reaction
-> visibility/team knowledge
-> noise perception
```

Oppure promuovere Noise prima solo con una decisione esplicita di scope.

Se implementato:

- eventi di rumore dal resolver;
- propagazione pura sul grafo;
- TeamKnowledge sanitizzato;
- UI Sound overlay;
- eventuali reaction acustiche dopo.

---

# 58. Checklist finale per Claude prima di aprire una PR

```text
[ ] Ho letto il canone attuale?
[ ] Il mio HEAD è ancora quello atteso?
[ ] La feature esiste già parzialmente?
[ ] Sto riusando una libreria pura esistente?
[ ] Ho introdotto un secondo modo di fare la stessa cosa?
[ ] Ho aggiunto una regola hero-specific al TurnManager?
[ ] Il bot usa la stessa regola del resolver?
[ ] La preview usa la stessa regola del resolver?
[ ] Il TurnLog spiega failure/fallback?
[ ] Ho ordinato esplicitamente i container?
[ ] Ho aggiunto un input live? Se sì, entra nel replay?
[ ] Sto esponendo informazioni future?
[ ] Il test fallisce davvero se tolgo la feature?
[ ] Build Editor?
[ ] Build Game?
[ ] Automation suite?
[ ] PIE/manual checklist?
[ ] Documentazione aggiornata?
```

---

# 59. Prompt operativo per Claude Code

Copiare questa sezione come task iniziale.

> Stai lavorando su `DegrassiAaron/refactor-tactics-main`.
>
> Prima di implementare, leggi `docs/design/piano-canonico-mvp.md`, `roadmap-checkpoint.md`,
> `roadmap-v0.1.md`, `adr-0003-modello-azioni-v01.md` e i tre cataloghi v0.1.
> Poi ispeziona il codice reale delle aree `Ability/`, `Turn/`, `Combat/`, `Map/`, `Terrain/`, `Unit/`, `UI/`
> e i test pertinenti.
>
> Il roster vigente è **Flux + Riva vs Bastion + Vektor**. Non usare Aegis/Nyx/Drift/Vex né roster PDR
> superati.
>
> Obiettivo a lungo termine: costruire `RT_Showcase_Relay_v01`, una partita golden 2v2 di 8 turni che
> dimostri terreno, combo acqua-elettricità, fallback, reazioni, strutture, obiettivo e — se approvato —
> Fast Reaction.
>
> NON implementare tutto insieme.
>
> Per il primo task:
>
> 1. esegui un audit evidence-based dello stato corrente;
> 2. elenca ciò che la showcase può già usare e ciò che dipende da E8/E9/E10/FastReaction;
> 3. verifica la suite corrente;
> 4. crea `docs/design/showcase-v0.1.md`;
> 5. proponi una generated fixture Relay che non aggiunga nuove regole;
> 6. aggiungi un test deterministico della fixture;
> 7. non modificare il TurnManager oltre ciò che è strettamente necessario alla fixture;
> 8. mostra il diff prima di introdurre una nuova astrazione.
>
> Regola architetturale: il resolver è autorità, gli Actor/UMG presentano. Nessun risultato competitivo
> dipende da frame, montage, Timeline o Tick.
>
> Se lavori su Fast Reaction:
>
> - trattala come nuovo input durante la resolution;
> - aggiorna prima l'ADR/determinismo;
> - usa `FRTSuppressiveZone` come precedente per le zone controllate;
> - non valutare Overwatch dopo il movimento completo;
> - estrai un resolver di movimento step-able mantenendo `ResolveHexPaths` come wrapper compatibile;
> - crea una `ReactionOpportunity` sanitizzata;
> - HOLD mantiene armata la reaction;
> - FIRE consuma la charge;
> - timeout è HOLD;
> - target simultanei nello stesso micro-step condividono una singola opportunity;
> - nessun future path/future trigger è visibile al decision maker;
> - logga la decisione come comando canonico per replay.
>
> A fine task riporta:
>
> - file modificati;
> - motivazione per ogni modifica;
> - test aggiunti;
> - test eseguiti;
> - build Editor/Game;
> - debiti rimasti;
> - commit message proposto;
> - prossimo checkpoint più piccolo possibile.

---

# 60. Sintesi decisionale

La strada più sicura è:

```text
NON:
showcase -> codice speciale -> demo che funziona solo una volta

SÌ:
showcase
-> espone il gap
-> sistema generale
-> test del sistema
-> scenario lo consuma
-> golden replay
```

Per RefactorTactics la showcase non deve essere un “livello scriptato”.

Deve essere la **prova integrata che le regole generali producono una partita interessante**.

Il punto tecnico più importante della nuova idea Fast Reaction è questo:

> una scelta che interrompe il movimento non può essere appesa al playback; deve esistere come decision boundary
> dentro la progressione logica a micro-step, e la risposta deve diventare parte degli input canonici del replay.

Il punto tecnico più importante del Rumore è questo:

> il suono non deve interrogare lo stato segreto del nemico; deve nascere da eventi reali della simulazione,
> propagare sul grafo e produrre conoscenza di squadra sanitizzata.

Il punto tecnico più importante della showcase è questo:

> non deve costringere il codice a conoscere “Turno 4 del demo”: deve essere un consumer di sistemi
> data-driven e deterministici che restano validi anche fuori dalla demo.
