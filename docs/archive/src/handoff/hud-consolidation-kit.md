> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

> ## 🗄️ `HISTORICAL` — SORGENTE RECEPITO IN PARTE
>
> **Archiviato il 2026-08-10.** Era in `todo/consolidazione-chat-openai/`, untracked. E' il *kit* di dettaglio
> del master UI / UX, e lo **precede**: dove i due divergono, prevale il master.
> Triage: [`consolidamento-chat-openai-triage-2026-08-09.md`](../../../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md).
>
> ⚠️ **Tre dei sette conflitti del pacchetto nascono qui**:
>
> | Qui il kit dice | Ma il canone dice |
> |---|---|
> | §3.5 — `TEAM READY 2/3` come componente persistente | non si simula un falso stato finche' non e' supportato — [`progettazione-hud.md`](../../../technical/systems/progettazione-hud.md) |
> | §11, §30, §31 — «Fog of War» | non e' FoW: conoscenza parziale a tre livelli, geometria statica **nota** — [`brief-conoscenza-parziale.md`](../../../gameplay/brief-conoscenza-parziale.md) |
> | §15 — `Eligible: Gadget / Riktor` | l'eleggibilita' e' per **capability**, mai per nome d'eroe — [`ADR-0006`](../../../decisions/adr-0006-ownership-abilita-sinergie.md) |
>
> Le 38 feature `HUD.*` di §28 non esistono: il registro usa `RT-FEAT-UI-*`.

# RefactorTactics — HUD System Consolidation Brief for Claude

## Scopo

Questo documento trasferisce e consolida le decisioni emerse sul sistema HUD di **RefactorTactics**.

Claude deve usare queste informazioni come input di progetto e **non limitarsi ad aggiungere una pagina isolata**. Deve integrare il contenuto nella struttura esistente del repository e consolidarlo con:

- documentazione tecnica;
- wiki;
- roadmap;
- Feature Map / Feature Registry;
- Scenario Map;
- Epic e Issue;
- test e acceptance criteria;
- dipendenze tra feature;
- milestone già esistenti.

L'obiettivo è trasformare l'HUD da insieme di widget ad hoc in un **sistema UI stratificato, data-driven, privacy-safe e coerente con planning simultaneo, reaction, fog of war, rumore e resolution deterministica**.

---

# 1. Principio generale HUD

L'HUD di RefactorTactics deve essere progettato come un sistema a strati.

Non creare un singolo `WBP_TacticalHUD` monolitico contenente logica di gameplay.

Separare almeno:

1. HUD persistente;
2. HUD di Planning;
3. overlay sulla mappa;
4. coordinazione alleata;
5. warning;
6. informazione certa / prevista / incerta;
7. percezione e Fog of War;
8. Fast Action / Fast Reaction;
9. Time Bank;
10. interazioni con la mappa;
11. objective UI;
12. resolution UI;
13. combat log / explainability;
14. notification feed;
15. debug HUD.

La UI deve ricevere stato già sanitizzato tramite ViewModel / DTO appropriati.

La UI non deve diventare fonte di verità del simulatore.

---

# 2. Vincoli architetturali

Mantenere i principi già consolidati di RefactorTactics:

- server authoritative;
- client proposes, server validates, server applies;
- simulazione deterministica basata su snapshot;
- TurnLog autorevole;
- animazioni/VFX/UI non determinano gli esiti;
- planning nemico mai replicato ai client avversari;
- ally intent team-only;
- warning calcolati esclusivamente da:
  - stato pubblico;
  - stato proprio;
  - intenti della propria squadra;
  - conoscenza lecita della squadra;
- nessun warning deve usare segretamente il `CanonicalIntentStore` nemico;
- preview aggiornate tipicamente a 8–12 Hz;
- overlay pesanti non devono essere aggiornati tutti ogni Tick;
- PC-first, Enhanced Input;
- CommonUI solo dopo il proof of concept.

---

# 3. HUD persistente

Questa parte costituisce la cornice sempre visibile della partita.

## 3.1 Turn / Phase Header

Mostrare:

- numero turno;
- fase corrente;
- stato del match.

La UI deve essere compatibile con il ciclo di fase di RefactorTactics e con la grammatica:

```text
Decision / Planning
→ Prep
→ Dash
→ Blast
→ Move
```

Il normale `Move` resta l'ultima fase volontaria.

Non rappresentare il turno come una sequenza arbitraria del tipo:

```text
Move → Attack → Move
```

---

## 3.2 Planning Timer

Mostrare il tempo rimanente della fase di Planning.

Target corrente:

```text
Planning ≈ 30 s
```

Il timer deve essere chiaramente distinguibile dal Time Bank.

---

## 3.3 Time Bank

Aggiungere un componente HUD dedicato per il Time Bank.

Il Time Bank è una riserva temporale personale utilizzata soprattutto durante finestre di decisione rapide / reaction.

Esempio UI:

```text
Reaction: 2.1 s
Time Bank: 24.8 s
```

Il design definitivo del consumo deve rimanere data-driven e deve consolidarsi con la feature Time Bank già discussa nel progetto.

---

## 3.4 Objective Panel

Mostrare almeno:

- objective corrente;
- stato;
- progresso;
- controllo;
- punteggio;
- eventuale contestazione.

Questo è il riepilogo HUD.

La posizione fisica dell'obiettivo deve essere rappresentata anche tramite World Marker.

---

## 3.5 Team Ready

Mostrare:

- Ready personale;
- Ready degli alleati;
- conteggio squadra.

Esempio:

```text
Team Ready 2/3
```

Deve essere sempre leggibile durante Planning.

---

## 3.6 Unit Roster

Mostrare le unità controllate dal giocatore con:

- ritratto;
- HP;
- stato critico;
- selezione;
- eventuale indicatore Ready;
- KO / unavailable.

---

## 3.7 Selected Unit Panel

Per l'unità selezionata:

- HP;
- risorse;
- status rilevanti;
- facing;
- eventuale reaction disponibile/armata;
- cooldown rilevanti.

---

## 3.8 Action / Ability Bar

Deve includere:

- azioni comuni;
- abilità specifiche del personaggio;
- cooldown;
- charge;
- costo;
- unavailable state;
- eventuali shortcut.

Le azioni comuni devono poter variare per personaggio, classe, stance, equipaggiamento o stato.

---

# 4. HUD di Planning

Il Planning HUD è uno dei sistemi centrali del gioco.

Quando il giocatore seleziona un'azione deve poter comprendere:

- cosa sta pianificando;
- in quale fase avverrà;
- da quale posizione;
- in quale direzione;
- quale parte è certa;
- quale parte è prevista;
- quale parte è incerta.

---

## 4.1 Action Details

Per l'azione selezionata mostrare:

- nome;
- icona;
- tipo;
- costo;
- cooldown;
- charge;
- range;
- AoE;
- requisiti;
- targeting policy;
- moving-target policy;
- fase di resolution;
- eventuali effetti ambientali;
- eventuali warning.

---

## 4.2 Action Sequence / Phase Strip

Creare una piccola timeline visiva per il piano dell'unità.

Esempio:

```text
PREP → DASH → BLAST → MOVE
```

Mostrare l'icona dell'azione prevista nella fase corrispondente.

La UI deve rendere esplicito che:

- Dash è uno spostamento speciale che può avvenire prima;
- Move normale è l'ultima fase;
- reaction non sono normali azioni inseribili arbitrariamente nella sequenza.

---

# 5. Movement Planner

Sulla mappa mostrare:

- celle raggiungibili;
- path selezionato;
- destinazione;
- costo movimento;
- eventuale costo per superficie/transizione;
- facing finale;
- riposizionamento finale se previsto;
- segmenti invalidi;
- punto in cui il piano diventa incerto.

Il path autorevole usa il grafo tattico.

La UI può mostrare preferenze tattiche locali, ma non deve cambiare la legalità del path.

---

# 6. Ability Preview

Il sistema deve supportare preview diverse in funzione del targeting:

- linea;
- cono;
- arco;
- cella;
- unità;
- AoE circolare;
- traiettoria;
- zona Overwatch;
- displacement;
- propagazione ambientale;
- modifica di archi/coperture;
- interazione con oggetti mappa.

Separare concettualmente:

```text
Path
LOS
Targeting
Trajectory
```

Questi sistemi non sono equivalenti.

---

# 7. Action Ghosts

Gli Action Ghosts sono una feature di prima classe.

Lo scopo non è mostrare soltanto:

> "dove andrà l'unità"

ma anche:

> "dove si troverà quando eseguirà una determinata azione".

Possibili ghost:

- posizione dopo Prep;
- posizione dopo Dash;
- posizione durante Blast;
- posizione finale Move;
- orientamento/facing;
- animazione/pose semplificata;
- attacco;
- Brace;
- Overwatch;
- interazione;
- uso di oggetto.

Il ghost deve supportare la grammatica Confermato / Previsto / Incerto.

---

# 8. Coordinazione della squadra

Durante Planning ogni giocatore deve poter vedere i piani dei compagni.

Queste informazioni sono **team-only**.

## 8.1 Ally Intent Overlay

Per ogni alleato mostrare secondo focus/filtri:

- path;
- destinazione;
- ghost;
- abilità;
- bersaglio;
- AoE;
- direzione;
- facing;
- label;
- Ready.

Non mostrare automaticamente tutto a piena opacità.

Servono filtri per evitare overload visivo.

---

## 8.2 Intent Labels

Esempi:

```text
Blocca porta
Bagno zona
Overwatch ingresso
Spingo Gadget
Focus Steel
```

Le label devono essere:

- brevi;
- rate limited;
- team-only;
- eventualmente mostrate su hover/focus.

---

## 8.3 Ping

Prevedere almeno:

- attenzione;
- movimento;
- attacca;
- difendi;
- interagisci;
- pericolo;
- generico.

Devono avere TTL e rate limit.

---

## 8.4 Tactical Drawing

Feature successiva ma già prevista dall'architettura:

- freccia;
- linea;
- area;
- erase;
- TTL;
- mute/visibility controls.

Team-only.

---

# 9. Warning System

Creare un sistema centralizzato di warning.

Non implementare warning sparsi direttamente nei widget.

Classificazione proposta:

```text
Info
Warning
Error
Block Commit
```

## Warning iniziali

### Collisione alleata

- path incompatibili;
- stessa destinazione;
- crossing conflict;
- occupancy problem.

### Friendly Fire

- AoE prevista su alleato;
- linea di tiro che attraversa un alleato;
- propagazione ambientale pericolosa.

### Risorse

- costo non disponibile;
- charge esaurita;
- cooldown;
- requisito mancante.

### Targeting

- LOS bloccata;
- range insufficiente;
- cella invalida;
- target non più noto;
- targeting dipendente da posizione futura.

### Path

- GraphRevision cambiata;
- porta/ponte/transizione modificata;
- path diventato invalido;
- budget superato.

### Planning state

- draft più recente del commit;
- unità non pronta;
- piano non committato;
- commit incompleto.

### Tactical warning

- facing sfavorevole;
- zona di pericolo conosciuta;
- probabile collisione fra intenti alleati;
- possibile fallimento legato a comportamento nemico.

Importantissimo:

> i warning non possono usare intenti avversari privati.

---

# 10. Confermato / Previsto / Incerto

Questa classificazione deve diventare una grammatica grafica comune a tutto l'HUD.

Non è un singolo widget.

## Confermato

Informazione basata su stato pubblico/conosciuto e regola deterministica.

Esempi:

- proprio path;
- porta attualmente chiusa;
- nemico visibile;
- cover esistente.

Visual language proposta:

```text
linea piena
icona piena
nessun ?
```

---

## Previsto

Informazione derivata da intenti alleati o previsione lecita.

Esempi:

```text
Phase dovrebbe bagnare questa cella prima dell'attacco di Gadget.
```

Visual language:

```text
linea tratteggiata
opacity differente
team marker
```

---

## Incerto

Dipende da:

- azione nemica;
- Fog of War;
- conflitto non risolto;
- posizione futura sconosciuta;
- rumore non localizzato.

Visual language:

```text
fade / gradient
?
pattern distinto
```

Non affidarsi soltanto al colore.

---

# 11. Perception / Fog of War HUD

L'HUD deve supportare più forme di conoscenza.

## Visible

La squadra conosce:

- posizione;
- identità;
- stato osservabile.

## Last Known

Per unità perse dalla visione:

- ultima posizione conosciuta;
- età dell'informazione;
- eventuale decay visivo.

La posizione non deve aggiornarsi segretamente.

## Unknown

Nessuna informazione disponibile.

---

# 12. Sound HUD

Il sistema Rumore è una risorsa informativa parallela alla visione.

Il giocatore può ricevere livelli diversi di informazione:

1. direzione;
2. area larga;
3. area stretta;
4. posizione precisa;
5. identificazione.

Esempi UI:

```text
Rumore rilevato a Nord-Est
```

oppure:

```text
Possible source within this area
```

oppure:

```text
Movement noise @ H12
```

Non mostrare automaticamente l'unità nemica.

---

## 12.1 Sound Overlay

Prevedere una modalità overlay dedicata.

Può mostrare:

- sorgenti udite;
- direzione;
- area di incertezza;
- intensità;
- età;
- rumore ambientale;
- acoustic masking;
- zone di copertura acustica;
- memoria sonora;
- previsione del rumore generato dalle proprie azioni.

La previsione delle proprie azioni è permessa.

La previsione non deve usare informazioni nemiche non autorizzate.

---

# 13. Fast Reaction HUD

Fast Reaction è una UI modale/overlay temporanea.

Deve bloccare l'avanzamento logico sul decision boundary senza trasformarsi in un nuovo Planning.

Default attuale Overwatch:

```text
FastReactionDuration = 3.0 s
Timeout = HOLD
```

Esempio:

```text
OVERWATCH OPPORTUNITY

Target: Enemy A

[FIRE]
[HOLD]

3.0
2.0
1.0
0.0
```

---

## 13.1 Trigger simultanei

Se più bersagli generano lo stesso trigger nello stesso micro-step:

```text
[FIRE A]
[FIRE B]
[HOLD]
```

Non creare prompt sequenziali artificiali se i trigger sono logicamente simultanei.

---

## 13.2 Privacy

La Reaction Opportunity inviata al client deve contenere solo:

- stato corrente lecito;
- target validi e visibili/conosciuti secondo le regole;
- risposte disponibili;
- timer.

Non deve contenere:

- futuri trigger;
- percorsi futuri avversari;
- numero di opportunità future;
- intenti nemici;
- destinazioni nemiche future.

---

## 13.3 Time Bank integration

La Fast Reaction HUD deve essere predisposta per integrare il Time Bank.

Mostrare:

- tempo della finestra;
- bank personale;
- eventuale consumo;
- eventuale stato timeout.

---

# 14. Fast Action HUD

Usare la stessa infrastruttura tecnica della Fast Reaction, ma con semantica distinta.

Esempio:

```text
ABILITY FOLLOW-UP

[DASH LEFT]
[DASH RIGHT]
```

Struttura concettuale condivisa:

```text
FastDecisionWindow
  Type = Action | Reaction
```

Non creare due sistemi completamente scollegati.

---

# 15. Map Interaction HUD

Quando il giocatore seleziona una cella o un elemento interattivo mostrare un pannello contestuale.

Esempio:

```text
PORTA B-14

State: Closed
Interaction: Open
Cost: 1
Eligible: Gadget / Riktor
```

Azioni possibili dipendono dall'oggetto e dal personaggio:

- Open;
- Close;
- Lock;
- Unlock;
- Hack;
- Force;
- Destroy;
- Repair;
- Electrify;
- Activate;
- Deactivate;
- Rotate;
- Raise / Lower;
- Move.

---

## 15.1 Linked elements

Mostrare relazioni significative fra elementi:

```text
Switch A → Door A
Generator → Elevator
Valve → Water Network
Control Panel → Bridge
```

Possibilmente evidenziando entrambi gli elementi sulla mappa.

---

# 16. Terrain / Hazard HUD

Tooltip / inspector della cella selezionata.

Mostrare solo le informazioni utili al contesto.

Possibili dati:

- CellId;
- Layer;
- quota;
- surface;
- movement cost;
- cover;
- opacity;
- hazard;
- water;
- fire;
- electricity;
- ice;
- acoustic modifier;
- interaction.

Evitare un muro di numeri sempre visibile.

Usare overlay tematici.

---

# 17. Objective World HUD

Oltre al pannello persistente, gli obiettivi devono avere marker world-space.

Mostrare:

- posizione;
- stato;
- ownership;
- contested;
- progress;
- eventuale range;
- off-screen indicator.

---

# 18. Unit World HUD

Sopra o vicino alle unità mantenere la UI leggera.

## Alleati

Possibili dati:

- HP;
- status critici;
- Ready;
- reaction armata;
- intent marker quando utile.

## Nemici

Mostrare soltanto informazioni effettivamente conosciute:

- HP se le regole lo rendono pubblico/conosciuto;
- status osservabili;
- target selection;
- marker detection.

Mai mostrare dati interni o planning.

---

# 19. Target HUD

Quando si seleziona un possibile bersaglio mostrare:

- identità;
- HP noto;
- distanza;
- cover;
- LOS;
- status;
- relazione con targeting;
- informazioni sul possibile risultato.

---

## 19.1 Impact Preview

Esempi:

```text
Arc Lance
20 Damage
CONFIRMED
```

oppure:

```text
Arc Lance
Expected hit
UNCERTAIN: target may move
```

Non implementare un simulatore client che usa intenti nemici nascosti.

---

# 20. Resolution HUD

Durante Resolution il Planning HUD deve ridursi.

Restano visibili:

- turno;
- fase;
- objective;
- roster essenziale.

Aggiungere:

- timeline della resolution;
- evento corrente;
- fast decision window quando necessaria;
- event result.

---

## 20.1 Resolution Timeline

Esempio:

```text
Prep
Dash
Reaction
Blast
Move
Environment
Cleanup
```

La timeline deve riflettere la configurazione reale del resolver.

Non codificare una pipeline divergente dal ruleset.

---

## 20.2 Current Event

Esempio:

```text
Gadget
ARC LANCE
→ Steel
```

---

## 20.3 Result Popup

Possibili eventi:

- damage;
- blocked;
- interrupted;
- dodged;
- displaced;
- status;
- cover destroyed;
- environment changed;
- KO;
- objective update.

La presentazione deriva dal TurnLog.

Non ricalcolare gli esiti nel widget.

---

# 21. Combat Log

Il Combat Log deve essere collassabile.

Riga sintetica:

```text
Gadget → Arc Lance → Riktor: 18
```

Dettaglio espanso:

```text
Base Damage: 20
Cover: -4
Wet Chain: +2
Final: 18
```

Le spiegazioni derivano da:

- TurnEvent;
- modifier registrati;
- reason codes.

Non dal ricalcolo client.

---

# 22. Outcome Explanation

Creare un inspector dell'evento separato dal semplice feed.

Il giocatore deve poter chiedere:

> Perché questa azione è fallita?

E ricevere una spiegazione.

Esempi:

```text
Target moved from H12 to H13 before impact.
```

```text
Line of sight blocked by Door_04.
```

```text
Movement blocked by allied occupancy at micro-step 3.
```

```text
Reaction cancelled because the unit was stunned.
```

Questa feature è essenziale per la leggibilità dei turni simultanei.

---

# 23. Notification / Event Feed

Messaggi temporanei e brevi.

Esempi:

- Ally Ready;
- Overwatch Armed;
- Reaction Expired;
- Door Opened;
- Objective Contested;
- Enemy Detected;
- Noise Detected;
- Cooldown Ready;
- Bridge Destroyed.

Il feed non sostituisce il Combat Log.

---

# 24. Overlay System

Prevedere una infrastruttura di overlay centralizzata.

Modalità iniziali:

```text
Default
Movement
Vision
Threat
Sound
Terrain
Team Plan
```

Possibili future:

```text
Cover
Objectives
Interactions
Environment
Acoustic
Debug
```

Non è necessario implementare tutte le modalità nella stessa milestone.

L'architettura deve però evitare che ciascuna feature crei un renderer separato incontrollato.

---

# 25. Tactical Map Overlay Renderer

Path, AoE, ghost, LOS e marker sulla mappa non devono essere gestiti direttamente come logica dei widget UMG.

Prevedere un renderer / subsystem dedicato.

Struttura concettuale:

```text
Tactical Map Overlay Renderer
├── Movement
├── Paths
├── Action Ghosts
├── AoE
├── LOS
├── Threat
├── Sound
├── Terrain
├── Ally Intents
├── Pings
└── World Markers
```

Deve ricevere dati già elaborati / sanitizzati.

---

# 26. Struttura UMG proposta

Struttura iniziale suggerita:

```text
WBP_TacticalHUD
│
├── WBP_TopBar
│   ├── WBP_TurnPhase
│   ├── WBP_TurnTimer
│   ├── WBP_TimeBank
│   ├── WBP_ObjectiveSummary
│   └── WBP_TeamReady
│
├── WBP_LeftPanel
│   ├── WBP_UnitRoster
│   └── WBP_SelectedUnit
│
├── WBP_BottomActionPanel
│   ├── WBP_ActionBar
│   ├── WBP_ActionSequence
│   ├── WBP_ActionDetails
│   └── WBP_ReadyButton
│
├── WBP_ContextLayer
│   ├── WBP_TargetInfo
│   ├── WBP_CellInfo
│   ├── WBP_InteractionPanel
│   └── WBP_Warnings
│
├── WBP_CoordinationLayer
│   ├── WBP_IntentInfo
│   └── WBP_PingMenu
│
├── WBP_ResolutionLayer
│   ├── WBP_ResolutionTimeline
│   ├── WBP_EventBanner
│   └── WBP_CombatLog
│
├── WBP_DecisionLayer
│   └── WBP_FastDecisionWindow
│
└── WBP_DebugLayer
```

Non considerare questi nomi un vincolo assoluto se nel repository esiste già una naming convention diversa.

Consolidare, non duplicare.

---

# 27. ViewModel / dati UI

Creare o consolidare un layer di ViewModel.

Schema concettuale:

```text
Authoritative Simulation
        |
        +--> Public State
        |
        +--> Team Knowledge
        |
        +--> Owner-only validation
        |
        v
Sanitized UI ViewModel
        |
        +--> UMG
        |
        +--> Tactical Overlay Renderer
```

Classificazione dati:

```text
Server-only
Team-only
Owner-only
Public
Derived-local
```

Non far accedere direttamente i widget al `CanonicalIntentStore`.

---

# 28. Feature Registry / Feature Map

Claude deve inserire o consolidare almeno le seguenti feature.

Nomi suggeriti, adattare agli ID già presenti.

```text
HUD.Core
HUD.TurnPhase
HUD.PlanningTimer
HUD.TimeBank
HUD.Objective
HUD.TeamReady

HUD.UnitRoster
HUD.SelectedUnit
HUD.ActionBar
HUD.ActionDetails
HUD.ActionSequence

HUD.MovementPreview
HUD.AbilityPreview
HUD.ActionGhosts

HUD.TeamIntent
HUD.IntentLabels
HUD.Pings
HUD.TacticalDrawing

HUD.WarningSystem
HUD.CertaintySystem

HUD.Perception
HUD.FogOfWar
HUD.Sound
HUD.LastKnownPosition

HUD.FastDecision
HUD.FastAction
HUD.FastReaction

HUD.MapInteraction
HUD.TerrainInspector
HUD.TargetInspector

HUD.ResolutionTimeline
HUD.EventBanner
HUD.CombatLog
HUD.OutcomeExplanation
HUD.NotificationFeed

HUD.OverlaySystem
HUD.WorldMarkers

HUD.Debug
```

Per ciascuna feature registrare:

- stable feature ID;
- descrizione;
- milestone;
- stato;
- owner/domain;
- dipendenze;
- scenari;
- test;
- documentazione wiki;
- Epic/Issue;
- acceptance criteria;
- eventuale privacy classification.

---

# 29. Dipendenze principali

Consolidare nella Feature Map.

Esempi:

```text
HUD.MovementPreview
    -> Map.Graph
    -> Pathfinding
    -> Planning.Intent

HUD.AbilityPreview
    -> AbilityDefinition
    -> Targeting
    -> LOS
    -> Planning.Intent

HUD.TeamIntent
    -> Networking.TeamRelay
    -> Planning.IntentPreview
    -> Privacy.Sanitization

HUD.WarningSystem
    -> Planning
    -> TeamIntent
    -> PublicState
    -> Targeting
    -> Pathfinding

HUD.FastReaction
    -> ReactionSystem
    -> FastDecision
    -> Networking.OwnerRPC
    -> TimeBank

HUD.Sound
    -> NoisePropagation
    -> TeamKnowledge
    -> FogOfWar

HUD.ResolutionTimeline
    -> TurnLog
    -> ResolverPhase

HUD.OutcomeExplanation
    -> TurnLog
    -> ReasonCodes
```

---

# 30. Milestone / Roadmap

Non creare una roadmap parallela.

Consolidare con quella esistente.

## Fondazioni / F0

Scope HUD minimo:

- Tactical HUD root;
- Turn / Phase;
- Planning Timer;
- Unit selection;
- Selected Unit;
- basic Action Bar;
- movement reachable cells;
- path preview;
- destination ghost;
- Ready;
- basic TurnLog / Resolution Event;
- Debug HUD.

Acceptance:

- funziona in `L_DevSandbox`;
- planning movimento leggibile;
- snapshot/resolution visivamente osservabili;
- nessuna logica competitiva dentro UMG.

---

## F1 — Networking / Private Planning

Aggiungere:

- Team Ready;
- ally path preview;
- ally destination;
- ally intent;
- team-only labels;
- ping;
- privacy tests;
- sequence handling;
- stale preview handling.

Exit gate:

```text
Zero intent leak
```

---

## F2 — Abilities

Aggiungere:

- Action Bar completa;
- Ability Details;
- targeting preview;
- AoE;
- trajectory;
- cooldown/resource states;
- action sequence;
- warning targeting;
- Action Ghost avanzati;
- impact preview.

---

## F3 — Multilayer / Environment

Aggiungere:

- Layer filter;
- terrain overlay;
- cover visualization;
- map interaction UI;
- doors;
- bridges;
- tunnels;
- elevators;
- hazards;
- water/fire/electric overlay;
- environment warning.

---

## F4 — Vertical Slice

Aggiungere/consolidare:

- Objective HUD;
- complete Team Intent UX;
- warning system completo;
- certainty grammar;
- combat log;
- outcome explanation;
- resolution timeline;
- notification feed;
- accessibility pass;
- playtest readability.

---

## Reaction milestone / appropriate existing milestone

Consolidare con la roadmap reale.

Aggiungere:

- FastDecision infrastructure;
- FastAction;
- FastReaction;
- Overwatch prompt;
- Time Bank;
- reaction countdown;
- timeout behavior;
- simultaneous target selector;
- reaction privacy tests.

Non creare una milestone separata se esiste già una Epic Reaction/Overwatch/Time Bank.

---

## Fog / Perception milestone

Consolidare:

- vision overlay;
- last-known;
- Team Knowledge;
- Sound Overlay;
- sound uncertainty;
- memory markers;
- acoustic masking UI.

---

# 31. Epics da creare/consolidare

Claude deve verificare prima le Epic già esistenti.

Non duplicare.

Se non esistono, creare Epic equivalenti a:

### EPIC — Tactical HUD Core

Include:

- root HUD;
- phase;
- timer;
- roster;
- unit panel;
- action bar;
- Ready;
- objective summary.

### EPIC — Planning Visualization

Include:

- Movement Preview;
- Ability Preview;
- Action Ghosts;
- Action Sequence;
- Target Inspector.

### EPIC — Team Coordination HUD

Include:

- Ally Intent;
- Labels;
- Ping;
- Tactical Drawing;
- Team Ready.

### EPIC — Tactical Warning & Certainty

Include:

- Warning System;
- Info/Warning/Error/Block;
- Confirmed/Predicted/Uncertain;
- collision;
- friendly fire;
- resource;
- path invalidation.

### EPIC — Fast Decision UX

Include:

- shared decision window;
- Fast Action;
- Fast Reaction;
- Overwatch;
- Time Bank integration;
- countdown;
- timeout policy.

### EPIC — Perception & Information HUD

Include:

- Fog of War;
- Last Known Position;
- sound detection;
- sound overlay;
- uncertainty areas;
- Team Knowledge representation.

### EPIC — Map & Environment HUD

Include:

- Cell Inspector;
- Map Interactions;
- Terrain Overlay;
- Hazard;
- linked interactables;
- world markers.

### EPIC — Resolution & Explainability

Include:

- Resolution Timeline;
- Current Event;
- Result Popup;
- Combat Log;
- Outcome Explanation;
- Notification Feed.

### EPIC — HUD Debug & Instrumentation

Include:

- CellId;
- Layer;
- graph revision;
- path cost;
- LOS;
- target reason;
- snapshot/hash;
- TurnEvent;
- reaction state.

---

# 32. Issue breakdown

Claude deve creare Issue abbastanza piccole da poter essere implementate/testate separatamente.

Esempio Epic Planning Visualization:

```text
HUD-PLN-01 Create movement reachable-cell overlay
HUD-PLN-02 Render selected path
HUD-PLN-03 Render destination ghost
HUD-PLN-04 Add final-facing indicator
HUD-PLN-05 Create action phase strip
HUD-PLN-06 Create ability line preview
HUD-PLN-07 Create circular AoE preview
HUD-PLN-08 Create cone/arc preview
HUD-PLN-09 Add targeting certainty style
HUD-PLN-10 Add overlay pooling / update budget
```

Esempio Epic Warning:

```text
HUD-WRN-01 Define warning data model
HUD-WRN-02 Add allied destination collision warning
HUD-WRN-03 Add allied path collision warning
HUD-WRN-04 Add friendly-fire AoE warning
HUD-WRN-05 Add resource/cooldown blocking errors
HUD-WRN-06 Add stale GraphRevision error
HUD-WRN-07 Add uncommitted-plan Ready blocker
HUD-WRN-08 Add uncertain-target warning
HUD-WRN-09 Verify warning system has no enemy-intent dependency
```

Esempio Epic Fast Decision:

```text
HUD-FD-01 Create generic FastDecision window
HUD-FD-02 Implement reaction countdown
HUD-FD-03 Implement default timeout response
HUD-FD-04 Integrate Time Bank display
HUD-FD-05 Implement Overwatch FIRE/HOLD
HUD-FD-06 Support simultaneous triggering targets
HUD-FD-07 Add owner-only sanitized DTO
HUD-FD-08 Add stale/expired opportunity handling
HUD-FD-09 Add reaction TurnLog entries
HUD-FD-10 Add privacy packaged test
```

Adattare naming, numbering e issue format alle convenzioni reali del repository.

---

# 33. Scenario Map

Claude deve aggiornare la Scenario Map e collegare gli scenari alle feature.

Non creare solamente test tecnici.

Servono scenari visuali/playable utilizzabili dal `BP_GameMode` / scenario selector.

## Categoria suggerita: UI / HUD

Se la tassonomia scenario già esistente non prevede `UI`, consolidarla con la struttura corrente senza rompere le categorie esistenti.

Scenari suggeriti:

### SCN-HUD-001 — Basic Planning HUD

Verifica:

- Turn;
- timer;
- selected unit;
- Action Bar;
- Move;
- Ready.

---

### SCN-HUD-002 — Movement Ghost

Unità pianifica movimento.

Verifica:

- reachable cells;
- path;
- destination;
- ghost;
- facing;
- commit.

---

### SCN-HUD-003 — Ally Intent

Due alleati preparano path differenti.

Verifica:

- local intent;
- ally preview;
- label;
- Team Ready;
- filtering.

---

### SCN-HUD-004 — Allied Collision Warning

Due alleati scelgono la stessa destinazione.

Verifica:

- warning;
- severity;
- visual overlay;
- eventuale Block Commit policy.

---

### SCN-HUD-005 — Friendly Fire Warning

AoE include un alleato previsto.

Verifica:

- warning corretto;
- area evidenziata;
- nessun utilizzo di dati nemici privati.

---

### SCN-HUD-006 — Confirmed / Predicted / Uncertain

Una singola scena deve mostrare contemporaneamente esempi dei tre stati.

Acceptance:

il giocatore distingue i tre senza fare affidamento esclusivamente sul colore.

---

### SCN-HUD-007 — Overwatch Fast Reaction

Un nemico entra nel cono.

Verifica:

```text
FIRE
HOLD
Countdown
Time Bank
```

Timeout → HOLD.

---

### SCN-HUD-008 — Overwatch Multiple Targets Same Step

Due unità entrano nello stesso micro-step.

Verifica:

```text
FIRE A
FIRE B
HOLD
```

Nessun prompt sequenziale artificiale.

---

### SCN-HUD-009 — Overwatch Hold / Later Target

Enemy A → HOLD.

Successivamente Enemy B → nuova opportunity.

Verifica:

- nessun leak sul futuro;
- reaction rimane armata;
- nuova prompt corretta.

---

### SCN-HUD-010 — Sound Direction

Nemico invisibile produce rumore.

Verifica:

- nessun enemy marker preciso;
- direzione rilevata;
- stato Incerto.

---

### SCN-HUD-011 — Sound Area

Rumore con detection intermedia.

Verifica:

- area di incertezza;
- age marker;
- nessun aggiornamento nascosto.

---

### SCN-HUD-012 — Map Interaction

Porta controllata da switch.

Verifica:

- highlight porta;
- highlight switch;
- link visuale;
- azioni disponibili.

---

### SCN-HUD-013 — Terrain Overlay

Mostrare acqua + elettricità + cover.

Verifica:

- leggibilità;
- filtri;
- tooltip;
- overlay.

---

### SCN-HUD-014 — Resolution Explanation

Azione fallisce perché il target si sposta.

Verifica:

- TurnLog;
- result popup;
- reason code;
- Outcome Explanation.

---

### SCN-HUD-015 — Path Invalidated

Una porta cambia stato / GraphRevision cambia.

Verifica:

- path invalidato;
- warning;
- recompute;
- draft non committabile finché non aggiornato.

---

### SCN-HUD-016 — Privacy HUD

Due squadre.

Team A pianifica un attacco.

Team B non deve ricevere:

- path;
- ability;
- target;
- AoE;
- labels.

Verifica anche packet/log canary.

---

# 34. Automation / Functional Tests

Aggiungere test automatici per le parti logiche e Functional Test per UI/scenario.

## Core / data tests

- certainty classification;
- warning severity;
- warning reason;
- stale revision;
- reaction countdown state;
- timeout policy;
- simultaneous reaction choices;
- DTO sanitization;
- view model classification.

## Network tests

- TeamIntent solo stessa squadra;
- owner-only error;
- FastReaction solo owner autorizzato;
- nessun enemy intent in ViewModel;
- canary privacy test.

## Functional UI tests

- HUD root presente;
- Ready state;
- correct turn phase;
- path overlay;
- warning overlay;
- FastDecision modal;
- CombatLog entry;
- Outcome Explanation.

## Performance tests

Misurare:

- Slate cost;
- overlay primitive count;
- preview update cost;
- pool hit/miss;
- frame time;
- 8–12 Hz refresh target per preview non critica.

---

# 35. Debug HUD

Implementare o pianificare una modalità debug non shipping normale.

Mostrare secondo toggle:

```text
CellId
Layer
Elevation
GraphRevision
Occupant
Surface
Hazard
PathCost
PathReason
LOS
LOS blockers
Targeting result
StableUnitId
Current Intent
Turn
Phase
MicroStep
Snapshot Hash
State Hash
Log Hash
Reaction Instance
Reaction State
Current TurnEvent
```

Collegare ai comandi debug già previsti dal progetto.

---

# 36. Accessibilità

Consolidare nella wiki/UI specification.

Requisiti:

- non usare solo colore;
- icone;
- pattern;
- testo;
- scaling UI;
- font leggibili a 1080p;
- key remapping;
- riduzione camera shake;
- riduzione motion;
- opzioni per opacità overlay;
- distinguibilità Confermato / Previsto / Incerto.

---

# 37. Performance

Budget coerenti con il progetto.

Target:

```text
Client: 60 FPS
Preview: < 50 ms end-to-end
Intent preview: 8–12 Hz
```

Specifico HUD:

- pooling decals/lines;
- evitare ricostruzione completa ogni Tick;
- virtualizzare Combat Log;
- evitare binding UMG costosi;
- profiling Slate;
- profiling overlay renderer;
- focus/filter per limitare elementi simultanei.

---

# 38. Wiki da aggiornare

Claude deve cercare le pagine esistenti e consolidare.

Se mancanti, creare pagine equivalenti:

```text
HUD Overview
Planning HUD
Action Ghosts
Team Coordination
Warning System
Confirmed Predicted Uncertain
Fast Decisions
Fast Reaction UX
Time Bank UX
Fog of War UI
Sound & Acoustic UI
Map Interaction UI
Terrain & Hazard Overlay
Objective HUD
Resolution UI
Combat Log
Outcome Explanation
Tactical Overlay System
HUD Debugging
```

Ogni pagina deve avere link:

```text
Feature ID
Roadmap milestone
Epic
Issues
Scenarios
Tests
Dependencies
Related systems
```

La wiki deve funzionare anche come porta di accesso agli scenari dimostrativi.

---

# 39. Documentazione tecnica da aggiornare

Consolidare almeno:

- PDR UI/UX;
- PDR Networking & Privacy;
- PDR Simulation / TurnLog;
- PDR Map / Pathfinding;
- PDR Abilities;
- documenti Reaction / Overwatch;
- documenti Rumore / Perception;
- documentazione Time Bank;
- architecture docs;
- roadmap;
- feature registry.

Non duplicare specifiche.

Se una decisione appartiene a un altro PDR, aggiungere cross-reference.

---

# 40. Definition of Done HUD Feature

Una feature HUD non è Done solo perché "si vede".

Deve:

1. funzionare nella fase corretta;
2. ricevere dati dalla source corretta;
3. rispettare privacy e classificazione;
4. non determinare il gameplay;
5. supportare log/debug;
6. avere almeno un test;
7. essere collegata ad almeno uno scenario quando è visuale;
8. avere acceptance criteria;
9. rispettare performance budget;
10. essere verificata in packaged build quando entra networking;
11. essere documentata;
12. avere Feature ID;
13. essere collegata a Epic / Issue / Roadmap.

---

# 41. Ordine di implementazione raccomandato

Non implementare l'intero HUD in una volta.

## Wave 1 — Fondazioni

```text
HUD Root
Turn / Phase
Timer
Unit selection
Unit Panel
Basic Action Bar
Movement Overlay
Path
Destination Ghost
Ready
Basic TurnLog
Debug HUD
```

## Wave 2 — Planning multiplayer

```text
Team Ready
Ally Intent
Ally Ghost
Label
Ping
Warning core
Privacy
```

## Wave 3 — Ability planning

```text
Action Sequence
Ability Preview
Target Inspector
AoE
LOS
Trajectory
Cooldown / resources
Advanced Action Ghost
```

## Wave 4 — Reaction

```text
FastDecision
FastAction
FastReaction
Overwatch
Time Bank
Reaction Log
```

## Wave 5 — Information warfare

```text
Fog of War
Last Known
Sound
Uncertainty
Team Knowledge
Threat
```

## Wave 6 — Full vertical slice UX

```text
Objectives
Map Interaction
Terrain
Resolution Timeline
Combat Log
Outcome Explanation
Notification Feed
Accessibility
Polish
```

L'ordine reale deve essere consolidato con le milestone esistenti e le dipendenze tecniche.

---

# 42. Output richiesto a Claude

Claude deve eseguire il lavoro nel repository e produrre un report finale.

## A. Analisi repository

Identificare:

- documentazione esistente;
- wiki;
- roadmap;
- Feature Registry / Feature Map;
- Scenario Map;
- Epic / Issue representation;
- naming conventions;
- file duplicati;
- decisioni già presenti.

---

## B. Consolidamento documentazione

Aggiornare i documenti esistenti.

Evitare:

- pagine duplicate;
- specifiche contrastanti;
- vecchi timer/reaction values lasciati senza nota;
- feature con nomi differenti per lo stesso concetto.

Quando esistono decisioni storiche contrastanti, mantenere una nota di supersession / decision log.

---

## C. Feature Map

Aggiungere/consolidare tutte le feature HUD elencate.

Ogni feature deve avere dipendenze e milestone.

---

## D. Scenario Map

Aggiungere gli scenari HUD e collegarli alle feature.

Quando possibile rendere gli scenari selezionabili dal sistema scenari già usato nel progetto.

---

## E. Roadmap

Integrare:

- HUD core;
- planning visualization;
- team coordination;
- warnings;
- reaction UX;
- Time Bank;
- perception/sound;
- resolution/explainability.

Non creare una roadmap indipendente.

---

## F. Epic e Issue

Creare o consolidare Epic e Issue necessarie.

Regole:

- non duplicare Epic già esistenti;
- dividere le Epic troppo grandi;
- associare ogni Issue a milestone;
- aggiungere acceptance criteria;
- aggiungere test richiesti;
- aggiungere scenario;
- aggiungere dipendenze;
- aggiungere privacy requirement quando necessario.

---

## G. Test plan

Aggiornare:

- Automation Tests;
- Functional Tests;
- network privacy tests;
- packaged tests;
- UI performance tests;
- scenario playtests.

---

## H. Wiki

Ogni feature importante deve avere una pagina o una sezione raggiungibile dalla wiki.

Collegare:

```text
Wiki
↔ Feature
↔ Roadmap
↔ Epic
↔ Issue
↔ Scenario
↔ Test
```

---

# 43. Report finale richiesto

Alla fine Claude deve fornire:

```text
1. File modificati
2. File creati
3. Decisioni consolidate
4. Decisioni obsolete/superseded
5. Feature aggiunte/modificate
6. Scenario aggiunti/modificati
7. Epic create/modificate
8. Issue create/modificate
9. Roadmap changes
10. Wiki changes
11. Test aggiunti/pianificati
12. Dipendenze rilevate
13. Rischi / punti ancora aperti
14. Prossimo passo consigliato
```

---

# 44. Regole finali

- Non inventare API Unreal.
- Non implementare logica competitiva nei Widget.
- Non replicare intenti nemici.
- Non creare roadmap parallele.
- Non creare Feature duplicate.
- Non creare Epic duplicate.
- Consolidare prima di aggiungere.
- Usare ID stabili.
- Collegare sempre feature, scenario, test, roadmap e issue.
- Mantenere il vertical slice come riferimento di scope.
- Favorire implementazioni semplici e verificabili.
- Ogni feature visuale importante deve avere uno scenario dimostrativo.
- Ogni feature di rete deve includere test anti-leak.
- Ogni feature di resolution deve essere spiegabile attraverso TurnLog/reason code.

---

# Risultato atteso

Al termine del consolidamento il progetto deve avere una singola visione coerente del sistema HUD:

```text
SIMULATION / TEAM KNOWLEDGE
          |
          v
SANITIZED VIEW MODEL
          |
    +-----+------+
    |            |
    v            v
UMG HUD     MAP OVERLAYS
    |            |
    +-----+------+
          |
          v
PLAYER DECISION
```

e una tracciabilità completa:

```text
Feature
  ↕
Wiki
  ↕
Roadmap
  ↕
Epic / Issue
  ↕
Scenario
  ↕
Test
```

Questa tracciabilità deve diventare lo standard anche per le future feature UI di RefactorTactics.
