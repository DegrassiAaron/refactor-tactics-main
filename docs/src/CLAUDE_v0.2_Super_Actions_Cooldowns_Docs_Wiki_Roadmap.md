# CLAUDE TASK — RefactorTactics v0.2
## Super Actions + Cooldown System — consolidamento Docs, Wiki e Roadmap

**Data handoff:** 2026-08-08  
**Scope:** design e pianificazione della **v0.2**, roster totale di 8 personaggi.  
**Non scope di questo task:** implementare subito il runtime C++/GAS. Prima allineare fonti di verità, Wiki, dati di authoring e roadmap.

---

# 0. Modalità di lavoro obbligatoria

Prima di modificare file:

1. leggi `AGENTS.md`, `CLAUDE.md` e gli eventuali file di istruzioni della repository;
2. identifica la versione Unreal realmente bloccata nel repository;
3. leggi Decision Log / ADR correnti;
4. individua le fonti canoniche correnti per:
   - roster;
   - personaggi;
   - ability/action catalog;
   - risorse personali;
   - cooldown esistenti;
   - fasi del turno;
   - Fast Reaction / Decision Boundary;
   - Character State / Configuration;
   - Wiki;
   - roadmap;
5. cerca la repository Wiki separata, se presente, ad esempio `refactor-tactics-main.wiki`;
6. individua l'eventuale workbook/dataset che guida la generazione delle pagine personaggio;
7. NON trattare vecchi PDR o vecchi roster come più autorevoli delle decisioni correnti.

Non inventare API Unreal, ID runtime, valori canonici o percorsi se la repository usa nomi differenti.

Se trovi un conflitto:
- non risolverlo silenziosamente;
- indica fonte A, fonte B e decisione corrente più recente;
- applica la regola di prevalenza del progetto.

---

# 1. Obiettivo

Consolidare per **RefactorTactics v0.2** due nuovi domini di design:

1. **Super Actions / Super Colpi**
2. **Cooldown System esteso**

La v0.2 deve considerare il roster totale:

```text
v0.1
- Gadget
- Phase
- Riktor
- Wraith

v0.2
- Steel
- Aurora
- Murdock
- Kwang
```

La v0.2 NON sostituisce il roster v0.1: porta il roster pianificato a **8 personaggi**.

Le fonti recenti indicano inoltre queste Signature:

```text
Gadget    -> Charge / Conductive Network
Phase    -> Flow / Wet Territory
Riktor -> Directional Structures / Protection
Wraith  -> Prediction / Interception

Steel   -> Guard Meter / Effective Protection
Aurora  -> Frozen Domain
Murdock -> Focus + Fire Sector
Kwang   -> Electric Anchor
```

Preservare queste identità.

---

# 2. Decisione di design — Super Actions

Una Super NON è semplicemente:

```text
attacco molto forte
+
cooldown lungo
```

La Super deve rappresentare il massimo commitment della meccanica firma del personaggio.

Principio:

> Una Super è potente perché cambia fortemente lo stato tattico, ma è difficile, costosa, lenta, rischiosa o situazionale da rendere valida.

Ogni Super deve avere almeno uno dei seguenti gate:

```text
Resource Gate
State Gate
Environment Gate
Geometry Gate
Prediction Gate
Setup Gate
Risk / Recovery
```

Preferire due o più leve quando il payoff è molto alto.

---

# 3. La Super usa il normale Ability Framework

NON creare concettualmente un secondo motore “Ultimate”.

Modello desiderato:

```text
Ability Definition
    +
Super metadata / tags
    +
Activation Requirements
    +
Costs
    +
Cooldowns
    +
Commit Policy
    +
Resolver
    =
Super Action
```

Il runtime futuro può usare un tag o classificazione equivalente a:

```text
Ability.Super
```

ma il nome reale deve seguire la tassonomia della repository.

GAS, quando presente, può gestire/mirrorare:

- ownership;
- costi;
- cooldown;
- tag;
- effect state.

Il **resolver deterministico resta autorità dell'esito**.

Non introdurre GAS come prerequisito documentale se la repository corrente non lo usa ancora.

---

# 4. Fasi e “lentezza” di una Super

Preservare l'ordine canonico:

```text
Planning
-> Prep
-> Dash
-> Blast
-> Move
-> Cleanup
```

Il normale `Move` resta l'ultima azione volontaria.

Una Super “lenta” NON usa il tempo dell'animazione per decidere il risultato.

Esempio:

```text
Planning
  -> Super committata

Prep
  -> telegraph / setup / stance / anchor

Dash
  -> il campo può cambiare secondo le normali regole

Blast
  -> impatto / payoff

Move
  -> eventuale Move limitato o vietato dalla Super

Cleanup
  -> cooldown / recovery / stato finale
```

Animazioni e VFX sono presentazione.

---

# 5. Commit e consumo

Decisione di baseline da consolidare:

## Intent illegale / rifiutato prima del Commit

```text
Server/validator REJECT
-> nessun costo
-> nessun cooldown
```

## Intent valido e committato, poi fallito durante Resolution

Esempi:

- target si sposta;
- LOS viene persa;
- prediction sbagliata;
- setup viene distrutto;
- condizione non è più valida all'impatto.

Baseline:

```text
Commit valido
-> costo consumato secondo AbilityDefinition
-> cooldown consumato
-> eventuale Super può WHIFF/FIZZLE
```

Motivazione:

> le scommesse tattiche non devono essere gratuite.

Eccezioni devono essere esplicite e data-driven, mai implicite.

---

# 6. Cooldown System — modello v0.2

Non trattare tutto come un singolo `CooldownTurns`.

Distinguere almeno:

## 6.1 Ability Cooldown

Cooldown della singola abilità.

Esempio:

```text
Ability A = CD 2
```

Le skill base possono avere cooldown diversi.

Non assumere che “base skill” significhi `CD = 0`.

---

## 6.2 Shared Cooldown Group

Più abilità possono condividere un lockout parziale.

Esempio:

```text
PhaseDash:
  AbilityCooldown = 2
  ApplyCooldownGroup Mobility = 1

EscapeVector:
  Requires CooldownGroup.Mobility == 0
```

Questo consente di limitare combinazioni troppo efficienti senza rendere inutilizzabile l'intero kit.

NON introdurre un global cooldown stile MMO.

---

## 6.3 Charges / Recharge

Una ability può avere:

```text
MaxCharges
CurrentCharges
RechargeTurns
RechargePolicy
```

Possibili policy:

```text
OneChargeAtATime
AllChargesTogether
ExplicitEventRecharge
```

Non implementare tutte le policy se non necessarie; documentare il modello.

---

## 6.4 Recovery / Lockout

Conseguenza dell'azione, distinta dal cooldown della skill.

Esempi:

```text
Overheated
Anchored
Drained
Reacquiring
Ungrounded
Thawing
```

La Recovery può:

- bloccare un gruppo di abilità;
- ridurre movimento;
- impedire Dash;
- modificare temporaneamente una meccanica firma.

Deve essere:
- deterministica;
- leggibile;
- visibile nella UI quando autorizzato.

---

## 6.5 Conditional Cooldown Modification

Consentire riduzioni/aumenti solo tramite eventi espliciti.

Esempio:

```text
EffectiveProtection
-> Protection cooldown -1
```

oppure:

```text
Hit >= 2 Wet targets
-> ElectricHeavy cooldown -1
```

Regole:

- niente proc probabilistici nascosti;
- niente decrementi derivati da animazioni;
- clamp espliciti;
- evento nel TurnLog;
- fonte della modifica spiegabile in UI.

---

# 7. Semantica temporale del cooldown

Fissare una sola convenzione globale e documentarla.

Baseline proposta:

Se una skill con `Cooldown = N` viene committata nel Turno `T`:

```text
Turn T:
  uso valido
  RemainingCooldown = N

Turn T+1 ... T+N:
  skill non disponibile

Planning T+N+1:
  skill disponibile
```

Il turno di utilizzo NON conta come uno dei turni di recupero.

Il decremento logico avviene nel `Cleanup` dei turni successivi.

Se il codice corrente usa già una convenzione diversa:
- NON cambiarla solo per uniformarsi a questo esempio;
- documentare la convenzione reale;
- verificare che UI, test e Wiki usino la stessa semantica;
- creare una decisione esplicita se serve una migrazione.

---

# 8. UI richiesta

Durante Planning il giocatore deve poter capire immediatamente:

```text
READY
CD 2
GROUP LOCK 1
2/3 CHARGES
REQUIRES: WET NETWORK
RECOVERY: OVERHEATED
SUPER READY
SUPER CONDITION NOT MET
```

Non affidarsi solo ai colori.

Per ogni ability mostrare in tooltip/pannello almeno:

- cooldown personale;
- group cooldown rilevanti;
- charges;
- costi;
- activation requirements;
- recovery conseguente;
- perché una skill è bloccata;
- quando tornerà disponibile.

Per le Super mostrare separatamente:

```text
Available
Conditionally Available
Unavailable
Committed
Telegraphed
Resolved
Whiffed/Fizzled
Recovery
```

La UI non deve conoscere intenti privati avversari.

---

# 9. Baseline Super v0.2 — roster completo di 8

I nomi seguenti sono **design names v0.2**.  
Se la repository ha già un nome migliore/canonico, preservare la meccanica e riconciliare il naming.

I numeri sono **baseline da playtest**, non valori retail definitivi.

---

## 9.1 FLUX — `Grid Overload`

### Fantasia tattica

Gadget porta la propria rete conduttiva al limite e scarica contemporaneamente la topologia preparata.

### Gate

Proposta:

```text
Charge >= 3
AND
esiste una rete valida Wet / Conductive / Metal preparata
```

Adattare la soglia alla risorsa canonica corrente.

### Fase

```text
Prep:
  telegraph della rete che verrà sovraccaricata

Blast:
  propagazione deterministica
```

### Effetto

- propaga elettricità attraverso la rete valida;
- colpisce più punti/bersagli secondo ordine stabile;
- applica cap per evitare duplicate hit;
- friendly fire secondo ruleset corrente;
- non crea collegamenti “magici” non presenti nello stato della mappa.

### Costo / rischio

```text
consuma Charge
Recovery: Overheated 1 turno
```

`Overheated` limita le abilità Electric Heavy ma non spegne l'intero personaggio.

### Cooldown baseline

```text
Super CD: 5
Shared Group: ElectricHeavy = 2
Recovery: Overheated = 1
```

### Counterplay

- rompere la rete;
- uscire dalle celle conduttive;
- togliere Wet;
- distruggere nodi/dispositivi;
- costringere Gadget a committare con geometria incompleta.

---

## 9.2 RIVA — `Tidal Collapse`

### Fantasia tattica

Phase converte un territorio d'acqua già preparato in una singola ondata di controllo.

### Gate

Proposta:

```text
Riserva Idrica >= soglia alta
AND
area Water/Wet connessa sufficiente
```

Preferire un gate ambientale reale, non una semplice barra Ultimate.

### Fase

```text
Prep:
  telegraph del bacino/territorio coinvolto

Blast:
  surge / redistribuzione dell'acqua
```

### Effetto

- seleziona un'area d'acqua connessa;
- la trasforma in una spinta/onda direzionale;
- può spostare unità entro limiti deterministici;
- modifica la distribuzione dell'acqua;
- le celle di origine possono prosciugarsi o perdere intensità;
- crea opportunità Wet per combo successive.

### Costo / rischio

Phase **consuma parte del setup** che ha costruito.

Quindi la Super non è soltanto payoff: riduce il territorio preparato.

### Cooldown baseline

```text
Super CD: 5
Shared Group: WaterMajor = 2
Recovery: Drained = 1
```

`Drained` riduce temporaneamente la capacità di rigenerare/creare acqua, senza disabilitare tutto il kit.

### Counterplay

- interrompere la continuità del territorio;
- occupare geometrie che rendono la spinta meno utile;
- sfruttare l'acqua prima della Super;
- usare elettricità contro la stessa area.

---

## 9.3 BASTION — `Fortress Protocol`

### Fantasia tattica

Riktor trasforma le proprie strutture preparate in una configurazione difensiva di massimo controllo geometrico.

### Gate

Proposta:

```text
Integrità Strutturale >= soglia alta
AND
almeno una struttura/cover controllata valida
```

### Fase

```text
Prep:
  Riktor si ancora
  telegraph delle strutture coinvolte

Blast/Prep effect:
  riconfigurazione strutturale coordinata
```

### Effetto

- riconfigura/collega più segmenti di cover già esistenti;
- può ruotare un numero limitato di archi;
- crea una zona molto leggibile e fortemente difendibile;
- NON genera una fortezza arbitraria dal nulla;
- modifica graph/cover revision secondo le regole reali.

### Costo / rischio

```text
consuma Integrità Strutturale
Riktor non può usare Dash
Move finale = 0 o fortemente limitato
```

### Cooldown baseline

```text
Super CD: 5
Shared Group: StructureMajor = 2
Recovery: Anchored = fino al Cleanup / 1 turno secondo playtest
```

### Counterplay

- flank;
- distruzione cover;
- displacement;
- attacco da direzioni non protette;
- ignorare la zona e giocare l'obiettivo altrove.

---

## 9.4 VEKTOR — `Perfect Intercept`

### Fantasia tattica

La Super più “scommessa” del roster: Wraith sceglie dove e quando il nemico passerà.

### Gate

Proposta:

```text
Momentum >= soglia moderata
```

Il vero costo principale resta la prediction.

### Planning

Dichiarare:

```text
Cell / Line
+
Phase or Decision Boundary
+
Targeting policy
```

### Effetto

Se un bersaglio valido attraversa il boundary previsto:

- attacco ad alto impatto;
- possibile stop/penalità al movimento secondo bilanciamento;
- payoff superiore all'Intercept standard.

Se nessuno attraversa:

```text
WHIFF
Super consumata
Cooldown consumato
```

### Cooldown baseline

```text
Super CD: 4
Shared Group: PredictionHeavy = 2
Recovery: nessuna obbligatoria
```

Qui il rischio di fallimento è già una parte sostanziale del costo.

### Counterplay

- cambiare percorso;
- bait;
- fermarsi;
- usare Dash/cover/fumo per cambiare la geometria;
- costringere Wraith a predire troppo presto.

---

## 9.5 STEEL — `Citadel Protocol`

### Fantasia tattica

Steel sacrifica una grossa parte della propria Integrità Scudo per trasformarsi temporaneamente nel punto di protezione della squadra.

Preservare la differenza con Riktor:

```text
Riktor = struttura la MAPPA
Steel   = protegge le UNITÀ
```

### Gate

Proposta:

```text
RES_SHIELD >= 60
AND
almeno una EffectiveProtection recente
```

La seconda condizione può essere “nel turno precedente” o altra finestra semplice da playtestare.

NON introdurre una seconda barra Guard se la Signature corrente usa già `RES_SHIELD`.

### Fase

```text
Prep:
  Steel si ancora e dichiara facing/settore protetto
```

### Effetto

Per il turno:

- amplia il proprio settore di protezione;
- migliora la capacità di interposizione;
- può creare un singolo forte boundary di protezione per alleati vicini;
- mantiene rivalidazione reale di LOS/cover/facing sul target effettivo;
- nessuna nested reaction arbitraria.

Evitare “invulnerabilità di squadra”.

### Costo / rischio

```text
Shield Cost alto, baseline 40
No Dash
Move fortemente ridotto o nullo
```

### Cooldown baseline

```text
Super CD: 5
Shared Group: ProtectionHeavy = 2
Recovery: GuardRecovery = 1
```

### Counterplay

- attaccare da più direzioni;
- displacement;
- separare Steel dagli alleati;
- ignorare il settore protetto;
- esaurire Shield prima del commitment.

---

## 9.6 AURORA — `Winter Avatar`

### Fantasia tattica

Aurora entra per un turno in una configurazione ambientale estrema che trasforma una porzione significativa della mappa.

È compatibile con il futuro `Character State / Configuration System`, ma resta una Super: non creare un doppio kit completo.

### Gate

Proposta:

```text
Cariche Termiche >= soglia alta
OR
Frozen Domain sufficientemente sviluppato
```

Preferire la forma che premia il setup ambientale.

### Fase

```text
Prep:
  Winter Avatar
  telegraph forte

Blast / Environment:
  trasformazione del dominio
```

### Effetto

- congela Water valida nell'area;
- estende Ice/Frozen Domain;
- può creare Whiteout limitato;
- modifica movement cost / slip / LOS tramite sistemi esistenti;
- NON applica semplicemente +X% damage.

### Costo / rischio

- consuma gran parte/tutte le Cariche Termiche;
- il ghiaccio rimane simmetrico: può aiutare anche il nemico;
- Fire può degradare/sciogliere il setup.

### Cooldown baseline

```text
Super CD: 5
Shared Group: CryoTerrainMajor = 2
Recovery: Thawing = 1
```

### Counterplay

- fuoco;
- uscire dal dominio;
- sfruttare i percorsi di ghiaccio;
- obbligare Aurora a trasformare una zona poco rilevante;
- interrompere il setup prima del payoff.

---

## 9.7 MURDOCK — `Kill Box`

### Fantasia tattica

Murdock rinuncia alla mobilità per dominare un settore con il massimo livello di preparazione e sorveglianza.

### Gate

Proposta:

```text
Focus attivo
AND
Murdock ha mantenuto una posizione/facing utile
AND
settore dichiarabile valido
```

Il Focus deve derivare dalla Signature corrente, non da una nuova barra duplicata.

### Fase

```text
Prep:
  dichiara il Fire Sector
  forte telegraph

Dash/Move:
  Murdock resta vincolato alla posizione

Resolution:
  opportunità nel settore
```

### Effetto

Proposta:

- arma una versione potenziata di Fire Sector / Overwatch;
- 2 charge massime nel turno;
- le opportunità restano separate e sanitizzate;
- HOLD non rivela trigger futuri;
- trigger simultanei nello stesso boundary diventano una singola opportunity set;
- ogni colpo usa LOS/facing correnti.

La Super NON deve prevedere in anticipo percorsi nemici sul client.

### Costo / rischio

```text
consuma Munizioni Speciali
No Dash
Move = 0
```

### Cooldown baseline

```text
Super CD: 5
Shared Group: FireSectorHeavy = 2
Recovery: Reacquire = 1
```

### Counterplay

- evitare il settore;
- fumo/Whiteout;
- flank;
- forced movement;
- bait delle charge;
- rompere tracking/LOS.

---

## 9.8 KWANG — `Stormbound`

### Fantasia tattica

Kwang usa contemporaneamente sé stesso e la Sword Anchor come poli della stessa rete elettrica.

### Gate

Proposta:

```text
Sword Anchor attiva
AND
link Kwang <-> Anchor valido
AND
Carica Tempesta >= soglia alta
AND
esiste almeno una conduzione utile Water/Metal/Conductive
```

### Fase

```text
Prep:
  entra in Stormbound
  rete/anchor telegraphed

Blast:
  Tempest payoff
```

### Effetto

- crea un circuito fra Kwang e Anchor;
- propaga elettricità attraverso celle valide;
- può far partire la scarica da entrambi i poli;
- usa ordine stabile e hit cap;
- aumenta controllo territoriale più che puro burst.

Possibile costo strutturale:

```text
a fine Super la Sword Anchor viene richiamata/disattivata
```

Questo evita che il payoff lasci anche tutto il setup intatto.

### Cooldown baseline

```text
Super CD: 5
Shared Group: StormHeavy = 2
Recovery: Ungrounded = 1
```

### Counterplay

- separare Kwang dall'Anchor;
- disabilitare/forzare il link;
- togliere Water/Conductive;
- spostare Kwang;
- occupare geometrie che rendono la rete poco utile.

---

# 10. Cooldown Group — baseline per i personaggi

NON è necessario implementare tutti questi group immediatamente.

Usarli come modello di authoring e introdurre solo quelli che risolvono una reale relazione fra skill.

Baseline:

| Hero | Group A | Group B |
|---|---|---|
| Gadget | `ElectricHeavy` | `Network` |
| Phase | `WaterMajor` | `WaterMobility` |
| Riktor | `StructureMajor` | `Protection` |
| Wraith | `PredictionHeavy` | `Mobility` |
| Steel | `ProtectionHeavy` | `ControlHeavy` |
| Aurora | `CryoTerrainMajor` | `CryoMobility` |
| Murdock | `FireSectorHeavy` | `Devices` |
| Kwang | `StormHeavy` | `Anchor` |

Regola:

> non assegnare automaticamente ogni ability a un group solo perché esiste il group.

Ogni relazione deve avere una motivazione di balance/gameplay leggibile.

---

# 11. Cooldown individuali esistenti — NON distruggere

La repository possiede già cooldown individuali per molte skill.

Per Steel/Aurora/Murdock/Kwang esistono valori design/source nel dataset corrente.

Esempi noti dalle matrici recenti:

```text
Steel
Shield Bash     1
Bulwark Arc     2
Interpose       3
Seismic Lock    4

Aurora
Glacial Lance       1
Frozen Simulacrum   3
Hoarfrost Ring      4
Whiteout            4

Murdock
Rail Shot          1
Suppressive Lane   3
Tracer Beacon      2
Concussive Mine    4

Kwang
Storm Slash       1
Sword Anchor      3
Lightning Return  3
Tempest Circuit   5
```

Trattare questi come **baseline design esistente**, non come numeri da cancellare.

Prima di modificarli:

1. verificare la fonte più recente;
2. verificare se sono `CANONICAL`, `SOURCE_VALUE`, `DESIGN_SPEC`, ecc.;
3. se il nuovo modello richiede una variazione, registrarla come nuova decisione di bilanciamento.

Per Gadget/Phase/Riktor/Wraith:
- preservare i cooldown canonici correnti della v0.1;
- la v0.2 può aggiungere Super/group/recovery senza riscrivere arbitrariamente il kit v0.1.

---

# 12. Relazione con le trasformazioni/configurazioni

Esiste già esplorazione del framework:

```text
Character State / Configuration System
```

Possibili collegamenti:

```text
Gadget   -> Charged / Overheated
Riktor-> Anchored / Fortress
Steel  -> Guard stance
Aurora -> Winter Avatar
Kwang  -> Stormbound
```

NON trasformare automaticamente tutte le Super in vere “forme”.

Regola:

```text
Super Action
    may apply
Character State / Configuration

ma

Character State / Configuration
    != automaticamente Super
```

Una Super può essere:
- una action singola;
- un setup + payoff;
- una predictive action;
- un overdrive;
- una configuration temporanea.

---

# 13. Privacy e networking

Le Super non cambiano le regole di privacy.

Durante Planning:

- intenti completi solo server;
- preview solo team;
- nessun AbilityId/target/path privato deve raggiungere il nemico.

Durante Resolution:

- telegraph pubblico solo quando la regola dice che la Super è ormai osservabile;
- Reaction Opportunity sanitizzate;
- non inviare trigger futuri;
- non inviare percorsi futuri;
- non inviare condizioni future che il giocatore non potrebbe conoscere.

Particolare attenzione a:

```text
Murdock.KillBox
Hero.Wraith.PerfectIntercept
```

Questi sistemi NON devono diventare un canale per leggere il planning avversario.

---

# 14. Determinismo e TurnLog

Cooldown, Super, Recovery e group lock sono stato competitivo.

Devono quindi essere inclusi nel modello deterministico e, quando pertinenti:

```text
Snapshot
StateHash
Replay
TurnLog
```

Eventi concettuali possibili:

```text
SuperCommitted
SuperTelegraphed
SuperGateValidated
SuperResolved
SuperWhiffed
SuperFizzled

CooldownApplied
CooldownTicked
CooldownExpired

CooldownGroupApplied
CooldownGroupModified

ChargeSpent
ChargeRecharged

RecoveryApplied
RecoveryExpired

ConditionalCooldownModified
```

NON creare automaticamente tutti questi event type se il TurnLog corrente possiede un modello più generico.

Prima riusare/generalizzare l'event model esistente.

Ogni modifica significativa deve essere explainable:

```text
what
who
when
before
after
reason
source
```

---

# 15. Test e scenari da inserire nella roadmap v0.2

Ogni Super deve avere almeno:

```text
Happy Path
Gate Failure
Counterplay
Whiff/Fizzle se applicabile
Cooldown Start
Cooldown Expiry
Shared Cooldown interaction
Recovery interaction
Determinism Repeat
UI/Tooltip reason
```

Scenario ID indicativi, adattare alla convenzione reale:

```text
Character.Hero.Gadget.Super.GridOverload.*
Character.Hero.Phase.Super.TidalCollapse.*
Character.Hero.Riktor.Super.FortressProtocol.*
Character.Hero.Wraith.Super.PerfectIntercept.*

Character.Steel.Super.CitadelProtocol.*
Character.Aurora.Super.WinterAvatar.*
Character.Murdock.Super.KillBox.*
Character.Kwang.Super.Stormbound.*
```

Aggiungere interaction scenarios v0.2, senza creare “ability di coppia”:

```text
Aurora ice -> Kwang electric interaction
Phase water -> Kwang electric interaction
Riktor route shaping -> Murdock sector
Steel protection -> Murdock stationary commitment
Gadget conductive network -> Kwang anchor interaction
Aurora domain -> Wraith prediction route change
```

Queste sono **interazioni sistemiche**.

NON creare:

```text
Ability.GadgetKwang.Combo
Ability.AuroraKwang.TeamAttack
```

Producer e consumer restano indipendenti e comunicano tramite:
- terrain;
- status;
- edge;
- persistent entity;
- event;
- visibility;
- legal shared state.

---

# 16. Balance telemetry da pianificare

La roadmap v0.2 deve prevedere raccolta dati per:

```text
Super availability turn
Super uses / match
Super commit rate
Super whiff/fizzle rate
Super effective value
Super affected units/cells
Recovery turns
Turns ability unavailable
Shared cooldown conflicts
Charges wasted at match end
Time between meaningful uses
```

Per Murdock/Wraith:

```text
prediction/reaction opportunities
HOLD count
commit target
whiff
bait success
```

Per Gadget/Kwang:

```text
network size
propagation count
duplicate-hit prevented
friendly-fire exposure
```

Per Phase/Aurora/Riktor:

```text
cells/edges modified
duration
enemy usage of created terrain
setup destroyed before payoff
```

Per Steel:

```text
effective protections
shield spent
shield refunded
protection opportunities declined
```

---

# 17. Aggiornamento Docs

Dopo audit, allineare almeno i domini equivalenti a:

## Nuova spec

Creare una specifica unica, nome adattato alle convenzioni reali, ad esempio:

```text
docs/gameplay/spec-super-actions-cooldowns.md
```

Deve contenere:

1. definizioni;
2. principi;
3. cooldown model;
4. semantica temporale;
5. Super model;
6. commit/fizzle policy;
7. fasi;
8. UI;
9. networking/privacy;
10. determinismo;
11. data model;
12. roster 8;
13. test;
14. balance telemetry;
15. stato `v0.2 DESIGN BASELINE`.

## Aggiornare

Cercare e aggiornare i documenti correnti equivalenti a:

```text
ability system / ability definition
character roster
character mechanics
turn sequence
fast reaction
deterministic simulation
UI/UX planning
balance
data governance
showcase / scenarios
roadmap
decision log
```

Non duplicare numeri competitivi in cinque documenti.

Definire ownership:

```text
runtime catalog / authoring dataset
    = numeri competitivi

gameplay spec
    = semantica e invarianti

character docs/wiki
    = spiegazione e reference

roadmap
    = stato / sequencing / exit gate
```

---

# 18. Decision Log / ADR

Creare una decisione numerata secondo la sequenza REALE della repository, senza inventare `D-XXX`.

Decisione da registrare:

## Titolo concettuale

```text
Super Actions and Layered Cooldowns for v0.2
```

La decisione deve fissare:

- Super come specializzazione dell'Ability Framework;
- nessun secondo motore Ultimate;
- commitment consuma cooldown anche su whiff/fizzle post-commit;
- cooldown individuali + shared group + charges + recovery;
- nessun global cooldown;
- decremento in turni logici/Cleanup;
- determinismo e TurnLog;
- v0.2 usa gli 8 personaggi;
- le 8 Super sono baseline di design da playtestare.

Se l'architettura non richiede un ADR, non crearne uno per forza.  
Il Decision Log è obbligatorio.

---

# 19. Aggiornamento Character Docs

Per ciascuno degli 8 personaggi, aggiungere una sezione standard:

```text
## Super Action

Name
Design Status
Fantasy
Activation Gate
Phase
Telegraph
Commit Policy
Cost
Effect
Whiff/Fizzle
Counterplay
Ability Cooldown
Shared Cooldown
Recovery
Dependencies
Scenarios
```

Aggiungere inoltre una sezione:

```text
## Cooldown Profile
```

che spiega:

- skill cooldown individuali correnti;
- eventuali shared group;
- charge ability;
- recovery/lockout;
- interazioni con Signature resource.

NON duplicare numeri canonici se la pagina è generata da un workbook: aggiornare la fonte e rigenerare.

---

# 20. Aggiornamento Wiki

Nella Wiki creare o aggiornare una pagina meccanica centrale:

```text
Super Actions e Cooldown
```

oppure naming coerente con la Wiki attuale.

Deve spiegare al giocatore:

- cosa rende una Super diversa;
- come si sblocca;
- telegraph;
- rischio/commitment;
- cooldown personale;
- cooldown condiviso;
- charges;
- recovery;
- perché una skill può essere bloccata;
- che una Super può fallire dopo il commit.

Non usare linguaggio tecnico C++ nella pagina giocatore salvo se la Wiki ha una sezione Technical Notes separata.

## Pagine personaggio

Per tutti gli 8:

```text
Super
Cooldown
Counterplay
Related mechanics
Scenario links
```

Link bidirezionali:

```text
Character
  -> Super/Cooldown mechanics
  -> Signature mechanic
  -> relevant scenario

Mechanic
  -> characters using it

Scenario
  -> characters/mechanics demonstrated
```

---

# 21. Dataset / XLSX che guida la Wiki

Se la Wiki viene generata o alimentata da un workbook tipo:

```text
RefactorTactics_Characters_Wiki_Data_*.xlsx
```

NON modificare solo i `.md` generati.

Aggiornare la vera fonte di authoring.

Aggiungere, solo se coerente con lo schema reale, campi equivalenti a:

```text
Super_ID
Super_Name
Super_Design_Status
Super_Activation_Gate
Super_Phase
Super_Cost
Super_Cooldown
Super_Cooldown_Group
Super_Recovery
Super_Whiff_Policy
Super_Counterplay

Ability_Cooldown_Group
Ability_Charges
Ability_Recharge
Ability_Recovery
```

Non aggiungere colonne duplicate se esistono già concetti equivalenti.

Aggiornare validator/dashboard se la pipeline lo richiede.

Lo stato delle nuove Super deve essere qualcosa di equivalente a:

```text
V0_2_DESIGN_BASELINE
```

e NON `IMPLEMENTED` finché il runtime non esiste.

---

# 22. Roadmap v0.2

NON espandere retroattivamente lo scope della v0.1.

Inserire il lavoro nella **v0.2**.

Struttura consigliata, da adattare alla roadmap esistente:

## V2-A — Cooldown Semantics & Data

Deliverable:

- semantica cooldown unica;
- authoring individual cooldown;
- shared cooldown groups;
- charges/recharge;
- recovery;
- validator;
- tooltip requirements.

Exit gate:

```text
cooldown state deterministic
same input -> same cooldown timeline
UI reason codes defined
```

---

## V2-B — Generic Super Action Contract

Deliverable:

- Ability/Super data contract;
- activation gates;
- commit/fizzle policy;
- telegraph;
- TurnLog contract;
- scenario schema.

Exit gate:

```text
1 generic test Super
no hero hard-code
whiff/fizzle correctly consumes resources/cooldown
```

---

## V2-C — Super Set: legacy roster

Personaggi:

```text
Gadget
Phase
Riktor
Wraith
```

Scopo:

dimostrare che le Super estendono sistemi già esistenti:

```text
network
territory
structure
prediction
```

Exit gate:

- 4 happy path;
- 4 counterplay;
- 4 cooldown tests;
- repeat determinism.

---

## V2-D — v0.2 Hero Kits + Super Set

Personaggi:

```text
Steel
Aurora
Murdock
Kwang
```

Scopo:

```text
protection/resource
terrain/configuration
facing/reaction
persistent anchor/network
```

Exit gate:

- 4 kit integrated;
- 4 Super;
- interaction tests;
- no privacy leak;
- TurnLog explainable.

---

## V2-E — UI / Wiki / Balance Readability

Deliverable:

- cooldown UI;
- Super readiness/gate UI;
- telegraph;
- tooltip reasons;
- action ghost;
- Wiki completa;
- scenario links;
- telemetry dashboard/schema.

---

## V2-F — v0.2 Eight-Hero Stress Validation

Validare roster completo.

Non è necessario che una singola partita usi tutti e 8, ma la suite deve coprire:

- mirror/non-mirror compositions;
- cross-system interactions;
- cooldown pressure;
- Super overlap;
- multiple telegraphs;
- multiple decision boundaries;
- map readability;
- match duration;
- replay determinism;
- packaged build.

---

# 23. Issue / Epic planning

Se la repository mantiene issue/epic come parte della roadmap, creare/aggiornare il piano senza duplicare lavoro esistente.

Possibili work item:

```text
EPIC: v0.2 Super Actions & Cooldown Framework

- cooldown semantics
- shared cooldown groups
- charges/recharge
- recovery states
- super ability metadata
- activation requirements
- commit/fizzle policy
- super UI/telegraph
- TurnLog/replay
- 8 hero super definitions
- scenario suite
- balance telemetry
- wiki/data generation
```

Cercare prima issue equivalenti.

Se esistono:
- aggiornarle;
- collegarle;
- non crearne copie.

---

# 24. Rischi di design da documentare

## Snowball

Evitare:

```text
deal damage -> ultimate meter
```

come default universale.

Le risorse Super devono premiare il gameplay firma, non soltanto chi sta già vincendo.

## Downtime

Cooldown lunghi non devono creare personaggi inutili.

Preferire:

```text
Super CD lungo
+
group lock breve
+
recovery breve
```

invece di bloccare l'intero kit.

## Cognitive Load

Non assegnare 4 cooldown personali + 4 group + 3 charges + 2 recovery a ogni hero solo perché il framework lo permette.

Il framework è ricco.

Ogni personaggio deve usarne un sottoinsieme leggibile.

## Hidden State

Cooldown, charges e recovery autorizzati devono essere leggibili.

Non creare vantaggio da memoria obbligatoria dell'utente.

## Super spam

La Super non deve apparire automaticamente ogni N turni se il giocatore non ha costruito la condizione tattica.

## Guaranteed payoff

Le Super devono avere counterplay.

Alcune, soprattutto Wraith e Murdock, devono poter essere baitate o evitate.

---

# 25. Audit di consistenza finale

Cercare almeno:

```text
Ultimate
Super
Super Action
Cooldown
CooldownTurns
Cooldown Group
Charges
Recharge
Recovery
Lockout
Overheated
Drained
Anchored
Winter Avatar
Stormbound
Focus
Guard Meter
```

Verificare:

1. nessuna fonte corrente dica ancora che una Super è solo una barra Ultimate automatica;
2. nessun documento dica che tutte le skill base sono senza cooldown;
3. nessuna pagina Wiki mostri cooldown differenti dalla fonte authoring;
4. nessun vecchio roster sovrascriva gli 8 personaggi;
5. Steel/Aurora/Murdock/Kwang siano classificati v0.2;
6. Gadget/Phase/Riktor/Wraith rimangano v0.1 ma disponibili nel roster totale v0.2;
7. nessuna Super sia marcata `IMPLEMENTED` senza codice/test reale;
8. roadmap v0.1 non venga gonfiata;
9. roadmap v0.2 contenga exit gate misurabili;
10. i nomi temporanei siano chiaramente distinti dagli ID runtime.

---

# 26. Output richiesto a Claude

Alla fine restituisci un report sintetico con:

## A. Audit

```text
Files inspected
Conflicts found
Decisions applied
Open questions
```

## B. Files changed

Separati in:

```text
Docs
Wiki
Roadmap
Decision Log / ADR
Dataset / XLSX
Generated pages
```

## C. Roster Super matrix

Tabella finale:

```text
Hero
Super
Gate
Cost
Phase
Cooldown
Shared Group
Recovery
Primary Counterplay
Status
```

## D. Cooldown matrix

Per ciascun hero:

```text
Ability
Individual CD
Shared Group
Charges
Recovery
Source/Status
```

## E. Roadmap delta

```text
Milestone
New/updated work item
Dependencies
Exit gate
```

## F. Gaps

Tutto ciò che resta:

```text
TBD
needs playtest
needs runtime implementation
needs data migration
needs scenario
needs validator
```

---

# 27. Guardrail finale

NON fare in questo task:

- mega-refactor C++;
- implementazione completa GAS;
- cambio dei cooldown canonici v0.1 senza decisione;
- nuova barra Ultimate globale;
- hard-code `if (Hero == Gadget)` nel resolver;
- Super di coppia/fazione;
- nuove statistiche inventate per riempire celle vuote;
- promozione automatica di design baseline a `IMPLEMENTED`;
- modifica dello scope v0.1 per far entrare la v0.2.

Obiettivo finale:

> La repository deve descrivere in modo coerente una v0.2 con 8 personaggi, Super Actions fortemente identitarie e un sistema di cooldown stratificato ma leggibile, con documentazione, Wiki, dati di authoring e roadmap che raccontano la stessa cosa.
