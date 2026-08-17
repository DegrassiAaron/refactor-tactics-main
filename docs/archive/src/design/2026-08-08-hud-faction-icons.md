> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

> ✅ **RECEPITO il 2026-08-08** (fondazione). Decisione: [D-031](../../../decisions/RT_PDR_00_Decision_Log.md).
> Il catalogo semantico delle icone è **E20** di [`../../../roadmap/roadmap-v0.1.md`](../../../roadmap/roadmap-v0.1.md),
> da fare **prima** dei widget di E11. Le dodici categorie complete, il world-space HUD e le icone di fazione
> per il roster 8 sono **E25** in [`../../../roadmap/roadmap-post-v0.1.md`](../../../roadmap/roadmap-post-v0.1.md).
> Immagini sorgente in [`../../../src/design/hud/`](../../../src/design/hud/). Verifica visiva: `PIE-ICON-01`.

# REFACTORTACTICS — HANDOFF CLAUDE CODE
## Faction Icons, HUD Icon Language, UI Icon Catalog e roadmap completa di implementazione

**Data:** 2026-08-08  
**Scopo:** consolidare documentazione, roadmap, dati, UI e implementazione Unreal Engine 5 del sistema iconografico di RefactorTactics.  
**Target iniziale:** v0.1 / vertical slice, con architettura estendibile a roster, fazioni, percezione, reaction, Wiki e future versioni.

---

# 0. ISTRUZIONI PER CLAUDE CODE

Stai lavorando nella repository reale di **RefactorTactics**.

Prima di modificare qualsiasi file:

1. leggi `AGENTS.md`, `CLAUDE.md`, README e Decision Log se presenti;
2. individua la versione Unreal Engine effettivamente bloccata nella repository e usa quella come baseline;
3. analizza almeno:
   - documentazione UI/UX;
   - documentazione fazioni;
   - documentazione personaggi/roster;
   - Gameplay Tags;
   - Data Assets / Asset Manager;
   - roadmap;
   - Fast Reaction / Overwatch;
   - Fog of War / Rumore / Perception;
   - HUD e widget esistenti;
   - eventuale Wiki/documentazione personaggi/fazioni;
4. tratta il codice e le decisioni più recenti della repository come fonte di verità;
5. non reintrodurre vecchi roster, nomi o regole archiviate;
6. non inventare appartenenze di personaggi a fazioni se non sono già definite;
7. se trovi un conflitto, registralo nel report finale e risolvilo solo se esiste una decisione successiva chiara;
8. non creare un sistema UI parallelo se esistono già ViewModel, cataloghi, Style Set o widget equivalenti;
9. non hardcodare riferimenti ad asset nei Blueprint quando possono essere risolti tramite ID/catalogo;
10. non affidare informazione competitiva soltanto al colore.

Questa attività NON richiede di rifare tutta la HUD in una singola PR.  
Richiede invece:

- specifica consolidata;
- architettura dati;
- primo set iconografico v0.1;
- integrazione progressiva;
- roadmap completa;
- test;
- documentazione aggiornata;
- backlog/issue coerente.

---

# 1. DECISIONE DI DESIGN

RefactorTactics deve avere una **grammatica iconografica unica e data-driven**.

Le icone non sono decorazione.

Devono comunicare rapidamente:

- identità;
- appartenenza;
- azioni;
- fase del turno;
- stato;
- terreno;
- informazione disponibile;
- reaction;
- coordinazione;
- livello di certezza;
- warning;
- obiettivi.

La stessa semantica deve poter essere riutilizzata in:

- HUD;
- roster;
- portrait;
- world-space markers;
- Action Ghosts;
- planning;
- Fast Reaction;
- combat log;
- tooltip;
- tutorial;
- scenari dimostrativi;
- Wiki;
- schermate personaggio;
- schermate fazione.

---

# 2. TEAM E FAZIONE SONO CONCETTI DIVERSI

Questa distinzione è obbligatoria.

```text
TEAM IDENTITY
- identifica alleato / nemico / squadra A / squadra B;
- dipende dal match;
- usa outline, HP bar, marker world-space, highlight, reticolo e altri indicatori di squadra.

FACTION IDENTITY
- identifica l'organizzazione narrativa del personaggio;
- non dipende dal lato del match;
- usa un badge/logo secondario;
- deve restare corretta anche in mirror match e team composti da personaggi di fazioni diverse.
```

NON implementare:

```text
Conflux = sempre blu
Constrine = sempre rosso
```

Una fazione non deve coincidere con il colore della squadra.

Il giocatore deve riconoscere in quest'ordine:

1. personaggio;
2. squadra;
3. stato tattico;
4. fazione.

L'icona di fazione è informazione secondaria durante il combattimento.

---

# 3. INTEGRAZIONE DELLE ICONE DI FAZIONE

Per **Conflux**, **Constrine** e future fazioni prevedere almeno:

## 3.1 Roster HUD

Piccolo badge vicino al portrait.

```text
┌─────────────────────────┐
│ ◇  [Portrait]           │
│    CHARACTER NAME       │
│    HP / Resource / Ready│
└─────────────────────────┘
 ^
 Faction badge
```

## 3.2 Character Detail / Tooltip

Mostrare:

- icona fazione;
- nome fazione;
- eventuale role icon;
- tooltip/localizzazione.

## 3.3 Pre-match / Team Composition

Qui la fazione può essere più visibile perché non compete con la leggibilità tattica.

## 3.4 Wiki

La stessa icona deve essere riutilizzabile nelle pagine:

- fazione;
- personaggio;
- roster;
- scenari di cooperazione;
- relazioni fra personaggi.

## 3.5 World-space HUD

Di default NON mostrare continuamente il badge fazione sopra ogni unità.

Valutarlo solo:

- su hover;
- su selezione;
- in modalità inspect;
- in tutorial/debug;
- dove non crea clutter.

---

# 4. HUD ICON LANGUAGE

Creare una tassonomia governata.

## 4.1 Identity

Esempi:

- faction;
- character;
- role;
- team relation quando serve come semantica distinta.

## 4.2 Action

Set minimo:

- Attack;
- Dash;
- Move;
- Overwatch;
- Interact;
- Wait.

Integrare eventuali Generic Actions effettivamente presenti nella repository senza duplicarle.

## 4.3 Phase

La UI deve poter distinguere almeno:

- Prep;
- Dash;
- Blast;
- Move.

La Reaction NON deve diventare una quinta fase se il modello corrente la tratta come decision boundary/branch condizionale.

## 4.4 Environment

Set iniziale:

- Water;
- Electric;
- Fire;
- Ice;
- Smoke;
- Metal;
- Cover.

Estendibile a:

- vegetation;
- rubble;
- mud;
- snow;
- steam;
- electrified water;
- burning surface;
- high ground;
- hazard;
- acoustic mask.

## 4.5 Map / Interaction

Prevedere:

- Cover;
- Door;
- Switch/Trigger;
- Bridge;
- Tunnel;
- Elevator;
- Elevation/Layer;
- objective interaction.

## 4.6 Status

Set iniziale da allineare ai Gameplay Tags reali.

Esempi concettuali:

- Wet;
- Electrified;
- Burning;
- Guarded;
- Stunned / Interrupted;
- Slowed;
- Suppressed;
- Marked.

NON creare tag nuovi soltanto perché esiste un'icona.

Prima verificare il dizionario dei Gameplay Tags.

## 4.7 Information / Perception

Prevedere semantiche distinte per:

- visible / visual contact;
- detected;
- heard;
- unknown;
- last known position;
- approximate area;
- acoustic contact.

Queste icone devono rispettare il Team Knowledge reale.

La UI non deve trasformare un dato acustico approssimativo in posizione precisa.

## 4.8 Reaction

Prevedere stati visivi per:

- reaction available;
- prepared/armed;
- opportunity triggered;
- hold/skipped opportunity;
- committed;
- consumed;
- expired;
- invalidated/interrupted.

Overwatch è il primo consumer importante, ma NON hardcodare il catalogo sul solo Overwatch.

## 4.9 Coordination

Prevedere:

- ally intent;
- ping;
- ready;
- unready;
- conflict;
- label;
- shared target / shared focus se già previsto.

## 4.10 Certainty

La UI deve distinguere:

- Confirmed;
- Predicted;
- Uncertain.

Non usare soltanto colore.

Usare anche:

- shape;
- border;
- fill;
- dash;
- pattern;
- opacity;
- simbolo secondario.

Esempio concettuale:

```text
Confirmed = bordo pieno / forma completa
Predicted = bordo tratteggiato / overlay specifico
Uncertain = fade / ? / area sfumata
```

## 4.11 Warning

Set minimo:

- Friendly Fire;
- Ally Collision;
- Invalid Target;
- Invalid Path;
- Resource Missing;
- Cooldown;
- Intent Not Committed;
- Path Invalidated;
- Uncertain Outcome.

## 4.12 Objective

Prevedere:

- Neutral;
- Capture;
- Contest;
- Owned;
- Locked;
- Dynamic Objective;
- interaction available.

Implementare solo ciò che v0.1 usa davvero.

---

# 5. CORE ICON SET v0.1

Non creare subito una libreria enorme.

Target indicativo:

**25–35 icone core**.

Il numero esatto dipende dagli asset e dalle feature effettivamente già presenti.

Priorità v0.1:

## Identity
- Conflux;
- Constrine;
- eventuali role icon necessarie.

## Action
- Attack;
- Dash;
- Move;
- Overwatch;
- Interact;
- Wait.

## Environment
- Water;
- Electric;
- Fire;
- Ice;
- Smoke;
- Cover.

## Status
- Wet;
- Electrified;
- Burning;
- Guarded;
- Stunned/Interrupted.

## Information
- Visible;
- Heard;
- Detected/Approximate;
- Unknown;
- Last Known.

## Coordination
- Ready;
- Ally Intent;
- Ping;
- Conflict.

## Certainty
- Confirmed;
- Predicted;
- Uncertain.

## Warning
- Friendly Fire;
- Collision;
- Invalid;
- Cooldown/Resource.

Ridurre o fondere icone solo se la semantica rimane inequivocabile.

---

# 6. REGOLE VISIVE

Definire una specifica breve e misurabile.

## 6.1 Silhouette-first

L'icona deve essere riconoscibile:

- senza colore;
- a dimensione HUD;
- in condizioni di contrasto diverse.

## 6.2 Colore come secondo canale

Il colore può rinforzare:

- team;
- tipo di danno;
- hazard;
- stato;
- warning;
- fazione.

Non deve essere l'unico canale.

## 6.3 Faction colors

Le fazioni possono avere una palette narrativa propria, ma:

- non deve sovrascrivere il team color;
- non deve compromettere la leggibilità;
- non deve creare ambiguità alleato/nemico;
- deve essere configurabile/data-driven.

## 6.4 Consistency

Definire:

- stroke;
- corner language;
- rapporto pieni/vuoti;
- uso di cerchi/triangoli;
- dimensioni base;
- safe area;
- stato disabled;
- stato selected;
- stato hover;
- stato uncertain;
- stato warning.

## 6.5 Accessibility

Testare almeno:

- grayscale;
- protanopia/deuteranopia simulation se il progetto dispone di strumenti;
- contrasto minimo ragionevole;
- leggibilità a 1080p;
- UI scale.

---

# 7. ICON STACK SYSTEM

Non mostrare tutti gli status contemporaneamente.

Implementare una policy di priorità.

Esempio:

```text
UNIT HUD
- Identity: max 1 elemento primario
- Critical Status: max 2
- Reaction: max 1
- Warning: max 1
```

Se gli status superano il limite:

```text
[Burning] [Electrified] [+4]
```

Hover / inspect:

```text
Burning
Electrified
Wet
Suppressed
Marked
Slowed
```

La priorità deve essere data-driven o deterministica, NON derivata dall'ordine di TMap/TSet.

Proprietà utile:

```text
IconPriority
```

e tie-break stabile tramite ID.

---

# 8. OVERWATCH / FAST REACTION

Usare la stessa grammatica iconografica.

Esempio di state machine visuale:

```text
Available
   ↓
Prepared / Armed
   ↓
Opportunity
   ├─ HOLD -> Armed
   └─ COMMIT -> Consumed

KO / Stun / invalidation
   ↓
Invalidated

Turn end / policy
   ↓
Expired
```

Il client mostra soltanto informazioni che la squadra è autorizzata a conoscere.

NON mostrare:

- numero di future opportunities;
- futuri trigger;
- path avversari;
- future target;
- dati dal CanonicalIntentStore.

La Reaction UI deve consumare DTO/view model sanitizzati.

---

# 9. RUMORE / FOG OF WAR / TEAM KNOWLEDGE

Il sistema iconografico deve rispettare la classificazione dell'informazione.

Esempio:

```text
Visual Contact
=> posizione esatta se autorizzata.

Acoustic Detection
=> direzione / area / livello di precisione reale.

Last Known
=> posizione storica, non posizione corrente.

Unknown
=> nessun dato.
```

Non usare una silhouette nemica sulla cella reale quando la squadra possiede soltanto un'informazione sonora approssimativa.

Prevedere varianti UI per:

- directional acoustic cue;
- wide uncertainty area;
- narrow uncertainty area;
- precise noise source;
- identified source.

Il sistema UI NON deve ricalcolare conoscenza dal full authoritative state.

Deve consumare il Team Knowledge sanitizzato.

---

# 10. DATA MODEL

La soluzione deve essere data-driven.

Verificare prima cosa esiste già nella repository.

Se non esiste un equivalente adeguato, introdurre un catalogo iconografico coerente con Asset Manager/Data Asset correnti.

Nome indicativo:

```text
URTUIIconCatalog
URTUIIconDefinition
```

oppure un singolo `PrimaryDataAsset` se più semplice e coerente con il progetto.

Campi concettuali:

```text
IconId
Category
GameplayTag / SemanticTag opzionale
Texture / Material / Brush reference
TooltipLocalizationKey
AccessibilityLabel
Priority
DefaultSize
StyleVariant
FallbackIconId
Version
```

NON implementare campi non necessari nella prima iterazione.

Usare soft references dove coerente con l'Asset Manager del progetto.

---

# 11. ID E TAG

Preferire ID stabili.

Esempi concettuali:

```text
UI.Icon.Faction.Conflux
UI.Icon.Faction.Constrine

UI.Icon.Action.Attack
UI.Icon.Action.Dash
UI.Icon.Action.Move
UI.Icon.Action.Overwatch

UI.Icon.Surface.Water
UI.Icon.Surface.Fire
UI.Icon.Surface.Electric

UI.Icon.Status.Wet
UI.Icon.Status.Burning

UI.Icon.Intel.Visible
UI.Icon.Intel.Heard
UI.Icon.Intel.LastKnown

UI.Icon.Reaction.Armed
UI.Icon.Reaction.Opportunity
UI.Icon.Reaction.Consumed

UI.Icon.Warning.FriendlyFire
UI.Icon.Warning.Collision

UI.Icon.Certainty.Confirmed
UI.Icon.Certainty.Predicted
UI.Icon.Certainty.Uncertain
```

IMPORTANTE:

- verificare la naming convention attuale;
- non creare Gameplay Tags indiscriminatamente;
- `IconId` e `GameplayTag` possono essere concetti distinti;
- non obbligare ogni icona ad avere un Gameplay Tag.

---

# 12. STRUTTURA CONTENT

Adattare alla repository esistente.

Esempio indicativo:

```text
Content/RefactorTactics/
  UI/
    Icons/
      Factions/
      Actions/
      Phases/
      Environment/
      Map/
      Status/
      Intel/
      Reactions/
      Coordination/
      Certainty/
      Warnings/
      Objectives/
    Data/
      DA_UIIconCatalog
    Materials/
    Widgets/
```

Non spostare asset esistenti in massa senza necessità.

Se la repository ha una struttura diversa, integrare invece di duplicare.

---

# 13. WIDGET / VIEWMODEL ARCHITECTURE

Le icone devono essere risolte da semantica/dati.

Esempio concettuale:

```text
Gameplay / TeamKnowledge / TurnLog
            ↓
        ViewModel
            ↓
      Semantic Icon ID
            ↓
       Icon Catalog
            ↓
    Brush / Texture / Material
            ↓
          Widget
```

NON:

```text
Gameplay code
    ↓
hardcoded /Game/UI/Icons/T_Wet.png
```

La UI deve restare presentazione.

Non deve decidere:

- regole;
- visibilità;
- validità tattica;
- reaction;
- certainty;
- target legality.

---

# 14. FALLBACK

Il catalogo deve avere un comportamento sicuro.

Se un'icona manca:

```text
Unknown / Missing Icon
```

e loggare:

```text
IconId
ConsumerWidget
Context
```

In Development/Editor il missing icon deve essere facilmente visibile.

In shipping evitare crash.

Aggiungere validator per:

- IconId duplicato;
- asset nullo;
- categoria invalida;
- fallback ciclico;
- tooltip/accessibility label mancante per icone user-facing importanti;
- asset non cookato se richiesto.

---

# 15. WIKI E DOCUMENTAZIONE

Il linguaggio iconografico deve essere documentato anche per la Wiki.

Creare o aggiornare una pagina tipo:

```text
docs/ui/icon-language.md
```

Contenuti:

1. principi;
2. tassonomia;
3. Team vs Faction;
4. certainty;
5. status;
6. environment;
7. intel;
8. reaction;
9. warnings;
10. accessibility;
11. asset naming;
12. authoring rules;
13. examples;
14. do/don't.

Aggiornare le pagine fazione per includere:

- canonical icon ID;
- palette narrativa se esiste;
- uso corretto;
- uso scorretto;
- legame con personaggi/scenari.

Aggiornare le pagine personaggio affinché possano mostrare:

- faction icon;
- role icon;
- ability icons;
- generic action icons;
- status/environment combo icon quando utile.

---

# 16. DOCUMENTI DA CONSOLIDARE

Claude deve individuare i file reali equivalenti e aggiornare almeno i domini seguenti.

## UI/UX
Integrare:

- faction badge;
- icon taxonomy;
- certainty icon language;
- icon stack;
- clutter budget;
- accessibility;
- reaction icons;
- perception icons.

## Factions
Integrare:

- canonical visual identity;
- IconId;
- palette;
- regole team-vs-faction;
- scenari di cooperazione già definiti;
- reference icon usage.

## Characters
Integrare:

- faction association solo se già canonica;
- role icon;
- ability icon hooks;
- Wiki rendering hooks.

## Perception / Noise
Integrare:

- visual vs acoustic icon semantics;
- uncertainty area;
- last known;
- sanitization requirement.

## Reaction / Overwatch
Integrare:

- Armed / Opportunity / Hold / Consumed / Expired / Invalidated.

## Data / Validation
Integrare:

- UI Icon Catalog;
- stable ID;
- validator;
- cook validation;
- localization/accessibility metadata.

## Roadmap
Inserire checkpoint e acceptance criteria descritti sotto.

---

# 17. ROADMAP DI IMPLEMENTAZIONE COMPLETA

La roadmap deve essere integrata nella roadmap reale della repository, non creata come documento scollegato.

Usare milestone/checkpoint già esistenti quando possibile.

---

## ICON-0 — Audit e consolidamento

### Obiettivo
Capire cosa esiste già e bloccare la grammatica.

### Deliverable
- inventario widget HUD;
- inventario icone esistenti;
- inventario Gameplay Tags correlati;
- inventario dati fazione;
- mappa dei documenti da aggiornare;
- decisione su catalogo;
- icon taxonomy consolidata.

### Exit gate
- nessuna duplicazione architetturale;
- Team vs Faction documentato;
- elenco v0.1 approvabile;
- naming deciso.

---

## ICON-1 — Fondazione dati

### Obiettivo
Implementare il resolver delle icone.

### Deliverable
- IconId type o convenzione stabile;
- UI Icon Catalog;
- lookup;
- fallback;
- logging;
- validator minimo;
- Automation Test lookup.

### Exit gate
- dato un IconId valido viene risolto lo stesso asset;
- duplicati rilevati;
- missing asset non crasha;
- test automatici passano;
- asset richiesti vengono cookati.

---

## ICON-2 — Core v0.1 Asset Set

### Obiettivo
Produrre/importare il primo set coerente.

### Deliverable
25–35 icone circa:

- faction;
- action;
- environment;
- status;
- intel;
- coordination;
- certainty;
- warning.

### Exit gate
- leggibili alle dimensioni reali;
- grayscale check;
- style consistency review;
- nessuna dipendenza solo-colore;
- catalogo completo per la v0.1.

---

## ICON-3 — Roster e Character HUD

### Obiettivo
Integrare identità e status senza clutter.

### Deliverable
- faction badge;
- action icons;
- icon stack;
- `+N`;
- tooltip/inspect;
- status priority;
- Ready indicator integrato.

### Exit gate
- 2v2 leggibile;
- 4v4 stress layout non collassa;
- nessun overflow importante a 1080p;
- UI scale test.

---

## ICON-4 — Planning / Action Ghosts

### Obiettivo
Usare le icone come parte del planning tattico.

### Deliverable
- selected action icon;
- ability marker;
- generic action marker;
- ally intent icon;
- conflict warning;
- certainty visual;
- path/AoE integration.

### Exit gate
Il giocatore può distinguere chiaramente:

- propria azione;
- intent alleato;
- warning;
- prediction;
- uncertainty.

---

## ICON-5 — Reaction / Overwatch

### Obiettivo
Comunicare reaction senza creare leak.

### Deliverable
- Available;
- Armed;
- Opportunity;
- Hold;
- Consumed;
- Expired;
- Invalidated.

### Exit gate
- la UI non mostra future opportunities;
- timeout/hold leggibile;
- stato coerente col TurnLog;
- privacy test.

---

## ICON-6 — Perception / Noise / Fog of War

### Obiettivo
Visualizzare informazione parziale correttamente.

### Deliverable
- visual contact;
- heard;
- approximate area;
- last known;
- unknown;
- confidence/precision variants se necessarie.

### Exit gate
Scenario test:

1. enemy visible;
2. enemy lost;
3. last known;
4. acoustic detection;
5. wide uncertainty;
6. precise acoustic source.

Assert: nessuna cella precisa viene mostrata senza autorizzazione.

---

## ICON-7 — Environment / Map Interaction

### Obiettivo
Integrare mappa come sistema strategico.

### Deliverable
- water;
- fire;
- electricity;
- ice;
- smoke;
- cover;
- door;
- switch;
- bridge/tunnel/elevator quando implementati.

### Exit gate
Hover/select di una cella comunica gli elementi rilevanti senza mostrare più informazioni del necessario.

---

## ICON-8 — Combat Log / Explainability

### Obiettivo
Riutilizzare il linguaggio nella spiegazione degli esiti.

### Deliverable
Esempio:

```text
[Arc Lance] -> [Wet] -> [Electric] -> 18 damage
```

oppure UI equivalente.

### Exit gate
Le icone sono supplementari al testo, non sostitutive dei reason code.

---

## ICON-9 — Wiki / Tutorial / Scenario Browser

### Obiettivo
Unificare linguaggio in-game e documentazione.

### Deliverable
- faction page integration;
- character page integration;
- mechanic pages;
- scenario category icons;
- tutorial references.

### Exit gate
La stessa semantic ID può essere mappata a icona coerente in HUD e documentazione, senza duplicare il significato.

---

## ICON-10 — Accessibility e Polish

### Obiettivo
Portare il sistema a qualità release.

### Deliverable
- contrast review;
- grayscale;
- color vision variants se necessarie;
- UI scaling;
- input/hover/focus;
- animation limits;
- disabled state;
- high contrast mode hook se previsto.

### Exit gate
Playtest con checklist accessibilità e nessuna informazione critica trasmessa soltanto via colore.

---

## ICON-11 — Performance / 4v4 Stress

### Obiettivo
Verificare clutter e costi.

### Test
- 8 unità;
- più status;
- reactions;
- objective;
- hazards;
- ally intents;
- warnings;
- FoW markers.

### Misurare
- widget count;
- Slate invalidation;
- allocazioni;
- aggiornamenti;
- frame time;
- overdraw;
- leggibilità.

### Regola
Non aggiornare ogni icona ogni Tick se lo stato non cambia.

Usare aggiornamenti event-driven/view model coerenti con l'architettura esistente.

---

## ICON-12 — Hardening / Packaged

### Obiettivo
Chiudere Definition of Done.

### Exit gate
- Editor;
- standalone;
- listen server;
- packaged;
- network privacy;
- cook;
- localization;
- fallback;
- tests;
- docs.

---

# 18. PRIORITÀ PER VERSIONE

## v0.1

Implementare:

- 2 faction icons canoniche;
- core action icons;
- core environment icons;
- core status icons;
- certainty;
- Ready;
- intent;
- conflict;
- warning principali;
- reaction Overwatch;
- perception minima se la feature è già in scope;
- catalogo;
- fallback;
- validator;
- HUD integration;
- docs.

## v0.2

Espandere:

- nuove fazioni;
- nuovi personaggi;
- nuove ability icon;
- nuovi status;
- nuovi terrain/hazard;
- richer perception;
- scenario browser;
- Wiki cross-reference.

## Post-v0.2

- completo authoring workflow;
- localization audit;
- modding data-only compatibility se e quando il modding entrerà realmente in scope;
- theme variants;
- high contrast pack;
- generated documentation/catalog export.

---

# 19. TEST AUTOMATICI

Aggiungere test proporzionati allo stato del progetto.

## Unit / Automation

### Icon catalog lookup
Input:

```text
UI.Icon.Status.Wet
```

Expected:

- definition exists;
- asset reference valid;
- category correct.

### Missing icon
Input:

```text
UI.Icon.Invalid.DoesNotExist
```

Expected:

- fallback;
- warning;
- no crash.

### Duplicate ID
Expected validator failure.

### Stable ordering
Status:

```text
A Priority 10
B Priority 10
C Priority 20
```

Expected:

- C first;
- A/B deterministic tie-break by stable ID.

---

# 20. FUNCTIONAL TESTS

Creare/estendere scenari del Test Harness quando disponibile.

## Scenario: UI.Icons.UnitStatus.Basic

Setup:

- unità Wet;
- unità Burning;
- unità Guarded.

Assert:

- icon IDs corretti;
- priority corretta;
- `+N` corretto.

## Scenario: UI.Icons.Certainty

Mostrare:

- Confirmed;
- Predicted;
- Uncertain.

Assert semantico sul ViewModel, non pixel-perfect se non esiste sistema screenshot testing stabile.

## Scenario: UI.Icons.Overwatch

Sequenza:

```text
Overwatch armed
Enemy A triggers
HOLD
Enemy B triggers
FIRE
```

Assert:

```text
Armed
Opportunity
Armed
Opportunity
Consumed
```

## Scenario: UI.Icons.Perception

Sequenza:

```text
Visible
Lost visual
LastKnown
Noise detected
Approximate area
```

Assert che il ViewModel non esponga dati superiori al TeamKnowledge disponibile.

---

# 21. NETWORK / PRIVACY TEST

Questa parte è obbligatoria per reaction e perception.

Test con due squadre.

Inserire canary data in:

- enemy planning;
- future enemy path;
- future reaction opportunity;
- exact hidden position.

Assert:

- il client avversario non riceve il dato;
- il widget non può risolverlo indirettamente;
- il catalogo iconografico riceve solo semantic state autorizzato.

Le icone NON devono diventare un side-channel.

---

# 22. DEBUG TOOLING

Aggiungere una modalità debug utile.

Esempio:

```text
UI Icon Debug
- IconId
- Category
- Source state
- Priority
- Fallback used
- TeamKnowledge source
- Widget consumer
```

Editor-only o Development-only.

Utile anche una `WBP_UIIconGallery` o equivalente per mostrare tutte le icone del catalogo.

Deve essere un tool, non gameplay runtime.

---

# 23. EDITOR SETUP

Se il catalogo è un Data Asset:

1. creare asset catalogo;
2. registrarlo con Asset Manager se necessario;
3. inserire core icon definitions;
4. configurare fallback;
5. agganciare i widget al resolver;
6. creare gallery/debug widget;
7. eseguire validator.

Documentare il workflow per aggiungere una nuova icona:

```text
1. Create/import asset
2. Assign canonical IconId
3. Add catalog entry
4. Add tooltip/accessibility metadata
5. Run validator
6. Verify icon gallery
7. Verify consumer widget
8. Add/update test
```

---

# 24. ERRORI DA EVITARE

Non fare:

```text
if Character == Gadget
    Texture = T_Conflux;
```

Non fare:

```text
if Status == Wet
    LoadObject("/Game/UI/T_Wet");
```

Non fare:

```text
TeamBlue -> Conflux
TeamRed -> Constrine
```

Non fare:

```text
red = enemy
green = ally
```

come unico segnale.

Non fare:

- 8 status icon contemporanee sopra una unità;
- dati nemici completi nel ViewModel e poi "nascosti" dal widget;
- icon priority basata su ordine TMap;
- texture mancanti che causano crash;
- Gameplay Tags creati solo per soddisfare l'UI;
- categorie iconografiche duplicate con nomi quasi uguali;
- icone Wiki indipendenti da quelle del gioco senza mapping canonico.

---

# 25. BACKLOG / ISSUE DA CREARE O AGGIORNARE

Claude deve adattare i titoli alla convenzione GitHub della repository.

Proposta:

1. `UI: audit existing HUD iconography and semantic states`
2. `UI: define canonical HUD Icon Language`
3. `Data: implement UI Icon Catalog and stable Icon IDs`
4. `Validation: add UI Icon Catalog validators`
5. `Art/UI: create v0.1 faction icon set`
6. `Art/UI: create v0.1 core action icon set`
7. `Art/UI: create v0.1 environment and status icon set`
8. `UI: integrate faction badge into roster and character details`
9. `UI: implement deterministic Unit Icon Stack`
10. `UI: integrate action and certainty icons into Planning`
11. `UI: integrate Overwatch and Reaction icon states`
12. `UI/Perception: implement visual/acoustic/last-known icon semantics`
13. `UI: integrate environment/map interaction icons`
14. `UI: integrate iconography into Combat Log`
15. `Docs/Wiki: reuse canonical icon semantics in faction and character pages`
16. `Accessibility: validate non-color-only icon language`
17. `QA: add UI icon Automation and Functional Tests`
18. `Network QA: verify icon/view-model privacy and no hidden-info leak`
19. `Performance: 4v4 HUD icon/clutter stress test`
20. `Tooling: implement UI Icon Gallery / debug overlay`

Non creare duplicati se issue equivalenti esistono già.

---

# 26. ROADMAP / DEPENDENZE

Dipendenze consigliate:

```text
Audit
  ↓
Icon Language
  ↓
Catalog + Validator
  ↓
Core Asset Set
  ↓
Roster / Unit HUD
  ↓
Planning
  ↓
Reaction
  ↓
Perception
  ↓
Environment
  ↓
Combat Log
  ↓
Wiki/Tutorial
  ↓
Accessibility
  ↓
4v4 Stress
  ↓
Packaged Hardening
```

Reaction dipende dal modello Reaction effettivo.

Perception dipende da TeamKnowledge effettivo.

Non anticipare sistemi gameplay ancora non implementati soltanto per poter mostrare un'icona.

---

# 27. DEFINITION OF DONE

Una feature iconografica è Done soltanto se:

1. usa semantic ID/data, non path hardcoded;
2. funziona nei widget reali;
3. non espone informazione non autorizzata;
4. ha fallback;
5. è validata;
6. è leggibile senza dipendere soltanto dal colore;
7. supporta tooltip/accessibility metadata quando user-facing;
8. ha debug sufficiente;
9. ha test pertinente;
10. funziona in packaged build;
11. documentazione aggiornata;
12. roadmap/issue aggiornata.

---

# 28. COMMIT PLAN CONSIGLIATO

Non fare un mega-commit.

Esempio:

```text
docs(ui): define canonical icon language and faction/team rules

feat(ui-data): add icon catalog and stable icon ids

test(ui-data): validate icon catalog and fallback behavior

feat(ui): integrate faction badges and unit icon stack

feat(ui): integrate planning and certainty icons

feat(ui): integrate reaction icon states

feat(ui): integrate perception icon semantics

feat(ui): integrate environment interaction icons

docs(wiki): reuse canonical faction and mechanic icon semantics

test(ui): add functional icon scenarios

perf(ui): validate icon HUD under 4v4 stress
```

Adattare agli standard reali.

---

# 29. REPORT FINALE RICHIESTO A CLAUDE

Al termine restituisci un report con:

## A. Audit
- file letti;
- UI esistente;
- sistemi riutilizzati;
- conflitti trovati.

## B. Decisioni consolidate
- naming;
- asset model;
- Team vs Faction;
- IconId;
- catalog;
- priority;
- fallback.

## C. Documentazione
Elenco file:

```text
CREATED
UPDATED
ARCHIVED
UNCHANGED
```

con breve motivazione.

## D. Roadmap
Mostrare milestone/checkpoint aggiunti e dipendenze.

## E. Issue
Elenco:

```text
Created
Updated
Already existing
Deferred
```

## F. Codice
Elenco file C++/Blueprint/Data Asset modificati.

## G. Test
Per ogni test:

```text
name
mode
expected
actual
PASS/FAIL
```

## H. Build
Indicare:

- Editor build;
- tests;
- packaged status.

## I. Debito / Follow-up
Separare:

- v0.1;
- v0.2;
- future polish.

---

# 30. RISULTATO ATTESO

Dopo questo lavoro RefactorTactics deve avere un sistema iconografico che non sia una raccolta di immagini sparse, ma una parte coerente dell'architettura UI:

```text
GAMEPLAY / KNOWLEDGE / TURNLOG
             ↓
          VIEWMODEL
             ↓
       SEMANTIC ICON ID
             ↓
         UI CATALOG
             ↓
       ICON + STYLE DATA
             ↓
 HUD / WORLD UI / LOG / WIKI
```

Principio finale:

> Una stessa informazione tattica deve avere la stessa identità visiva ovunque venga mostrata, senza rivelare dati che il giocatore non è autorizzato a conoscere.

