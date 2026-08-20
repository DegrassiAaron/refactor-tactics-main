> 🔎 **ESITO DELLA REVISIONE — 2026-08-16.** Sorgente **recepito in parte**. Referto:
> [`menu-frontend-spec-panel-2026-08-16.md`](../../../roadmap/plans/menu-frontend-spec-panel-2026-08-16.md).
>
> ✅ **È il primo handoff della serie che non duplica niente**, e la premessa è stata verificata in cinque
> punti indipendenti su `4ab36b48`: zero asset `WBP_*` in `Content/`, **nove** file UI tutti in-match, nessuna
> feature di frontend fra le 90 del registry, nessuna epic fra E1–E45, zero occorrenze di *menu* o
> *frontend* in `roadmap-post-v0.1.md`. Il frontend era genuinamente assente.
>
> ✅ **Il suo contributo maggiore è il §2**: *«Main Menu avviabile in packaged build»*. È ciò che ha fatto
> entrare **E46** in v0.1 ([D-144](../../../decisions/RT_PDR_00_Decision_Log.md)), e nient'altro del
> documento lo ha fatto.
> 🔴 **Ma l'argomento con cui il referto lo sosteneva era falso, e va detto qui perché è il banner che si
> legge per primo.** Diceva che il gate **G13** è 🟡 *«esattamente per quello»*: falso. Le due riserve di
> `G13` sono **dati** — la mappa d'autore (`PIE-V01-ARENA`, seduta U1) e la via a punti mai esercitata — e
> nessuna delle due si chiude con un menu. **E46 è scope nuovo**, dichiarato come tale. Ciò che regge è
> un'affermazione di prodotto: una build che avvia direttamente in partita, senza modo di iniziarla o
> uscirne, non è un vertical slice consegnabile. Vedi §6.1 del referto.
>
> 🔴 **Lo scope del §26 non regge, e la ragione è già scritta nel repository.** Mette in v0.1 `P0`
> Scenario Browser, Scenario Detail, Scenario Runner UI e Bot Visual Simulation — che il documento stesso
> qualifica DEV/TEST (§5, §8). `RT-FEAT-UI-SCENARIO-BROWSER` porta un `out_of_release_scope` del
> 2026-08-08: *«serve a chi sviluppa, non è contenuto della release»*. Chi aprisse quelle voci con questo
> documento in mano metterebbe tooling sul percorso critico di una consegna.
>
> ⏱️ **Il referto portava un secondo argomento contro le §5–§8, ed è caduto lo stesso giorno.** Diceva che
> [`#926`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/926) rende quelle sezioni
> ineseguibili perché `Scenarios/` non è staged nel pacchetto. La PR
> [`#935`](https://github.com/DegrassiAaron/refactor-tactics-main/pull/935) ha chiuso quella causa il
> 2026-08-16: i **77** JSON entrano nel pak. E **#926 è chiusa per intero** dal `2026-08-15T23:46Z` — anche
> la causa 2 (`-dpcvars` fuori in Shipping) è caduta con #945.
> ∴ le §5–§8 restano fuori dalla v0.1 **per il solo motivo sopra**, che basta da solo. Vedi §4.6 del referto.
>
> ⚠️ **Il §15 collide su un naming già deciso.** Propone `WBP_FrontendRoot` e `WBP_GameHUDRoot`: il
> prefisso del repository è **`WBP_RT_`** (CP 11.7, *«su un `.uasset` il rename costa più che scriverlo
> giusto»*) e il root dell'HUD in-match è già `WBP_RT_TacticalHUD`. Il principio del §15 — `Frontend !=
> In-Match HUD` — è invece giusto ed è stato tenuto.
>
> ⚠️ **Il §18 sbaglia una release, e il documento se l'era chiesto.** Colloca Online Play e Lobby in v0.7
> con la riserva *«Se networking è canonico qui»*: la risposta è **no, è v0.5** — `E40 · Il turno
> simultaneo in rete`, con `lobby privata` già nel gate. In v0.7 c'è `E42 · Competitive Alpha`.
>
> ⚠️ **Il §22 apre un terzo vocabolario di tracking**: i suoi 11 campi descrivono in prosa i 10 gate del
> feature registry più il DoD trasversale. Ne sopravvivono **due** che il registry non ha — impatto di
> accessibilità e di performance della UI — e sono l'unica parte da portare.
>
> ✅ **Tre voci erano già vere e il documento le riscopre**: il determinismo del §7 è il gate **G4**
> (`Replay.Verifier.ResimulationIsDeterministic`); il replay minimo del §10 è già
> [`#472`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/472); *«bot tramite API reali,
> niente shortcut nel resolver»* del §8 è [D-101](../../../decisions/RT_PDR_00_Decision_Log.md).
>
> **Recepito**: E46 in v0.1 (6 checkpoint, epic
> [#934](https://github.com/DegrassiAaron/refactor-tactics-main/issues/934)), quattro feature
> `RT-FEAT-UI-FRONTEND-*`, l'owner
> [`spec-frontend-navigazione.md`](../../../technical/architecture/spec-frontend-navigazione.md), e la sezione
> *«Il frontend oltre la v0.1»* di [`roadmap-post-v0.1.md`](../../../roadmap/roadmap-post-v0.1.md), che
> riconcilia il §18 coi temi di release reali.

---

# Claude Handoff — Frontend/Menu Features, GitHub Issues e Tracking
## RefactorTactics — integrazione v0.1 con roadmap verso v1.0

**Data:** 2026-08-16  
**Obiettivo:** integrare nel progetto RefactorTactics il focus corrente sui menu/frontend, creare o aggiornare le Issue/Epic necessarie e sincronizzare il tracking senza duplicare sistemi o tassonomie esistenti.

## 0. Regola operativa
Prima di creare file, Epic, Issue o codice:
1. aggiornare e ispezionare `main`;
2. leggere `CLAUDE.md` / `AGENTS.md`;
3. leggere Decision Log / ADR;
4. leggere roadmap canonica;
5. leggere Feature Registry;
6. leggere Scenario/Test Registry;
7. leggere UI/HUD spec corrente;
8. leggere Bot/Scenario spec corrente;
9. cercare Issue/Epic aperte e chiuse;
10. verificare branch/PR già in corso;
11. verificare codice e `Content/` realmente esistenti;
12. classificare ogni feature come `AS_BUILT`, `PARTIAL`, `PLANNED`, `MISSING`, `SUPERSEDED`.

Per ogni elemento decidere: `REUSE`, `UPDATE`, `CREATE`, `DEFER`.

Non creare duplicati.

# 1. Scope da integrare

```text
MAIN
│
├── PLAY
│    └── Briefing
│          └── Match
│                └── Result
│
├── TRAINING
│    └── Training Scenario
│
├── SCENARIOS
│    ├── Browser
│    │    └── Scenario Detail
│    │          └── Runner
│    │                └── Result
│    └── Bot Simulation
│
├── SETTINGS
│
└── QUIT
```

In-match:

```text
ESC / PAUSE
├── Resume
├── Settings
├── Restart Scenario / Match dove consentito
└── Return to Main Menu
```

Post-match/post-scenario:

```text
RESULT
├── outcome
├── winner
├── turns
├── TurnLog
├── replay ultimo run
├── Run Again
└── Main Menu / Scenarios
```

# 2. Feature v0.1 da tracciare

## Frontend Shell
- Main Menu;
- navigazione centralizzata;
- Back stack;
- modal layer;
- loading state;
- error modal;
- version/build label;
- keyboard/mouse navigation;
- controller compatibility solo se già nello scope.

Target UI:

```text
PLAY
TRAINING
SCENARIOS
SETTINGS
QUIT
```

Acceptance criteria:
- nessun dead-end;
- Back ritorna alla schermata corretta;
- nessun widget crea arbitrariamente altri widget senza flow controller;
- loading e error state sono gestiti;
- Main Menu avviabile in packaged build.

# 3. PLAY

Flow:

```text
Main
 -> Play
 -> Match Setup minimo
 -> Briefing
 -> Vertical Slice 2v2
 -> Result
```

Il Match Setup v0.1 può essere preconfigurato.

Mostrare o preconfigurare:
- mode;
- map/scenario;
- team player;
- team bot;
- bot profile;
- seed;
- Start.

Non introdurre ora:
- matchmaking;
- ranked;
- lobby online;
- account;
- progression;
- cosmetics.

## Briefing
Mostrare soltanto informazioni autorizzate:
- map preview;
- objective;
- own team;
- known enemy info se consentita;
- terrain/map features;
- deploy/start.

Mai mostrare hidden enemy planning.

# 4. TRAINING

Training deve riusare lo Scenario System.

v0.1 minimo:

```text
TRAINING
├── Movement
└── Basic Combat
```

Ogni voce deve puntare a una ScenarioDefinition/scenario canonico, non a un framework separato.

Future da registrare senza implementare ora:
- Character Training;
- Reaction Training;
- Vision Training;
- Sound Training;
- Environment Training;
- Mechanics Library.

# 5. SCENARIOS

`SCENARIOS` è una sezione DEV/TEST, anche se può essere visibile in Development build.

## Scenario Browser
Ogni entry mostra almeno:
- Scenario ID;
- Display Name;
- Category;
- Map;
- Mode;
- Expected turns;
- Last result/status;
- Run.

Categorie solo se coerenti con il registry reale.

Acceptance criteria:
- legge il catalogo/registry reale;
- non hard-coda una seconda lista scenario;
- filtri funzionanti;
- stato `PASS/FAIL/NOT RUN/RUNNING/INVALID`;
- scenario invalido mostra reason leggibile.

# 6. Scenario Detail

Mostrare:
- Scenario Name;
- Scenario ID;
- Map;
- Category;
- Teams;
- Seed;
- Expected Result;
- Last Run.

Azioni v0.1:
- RUN;
- RUN WITH DEBUG se già supportato;
- VIEW LAST RESULT;
- BACK.

Future:
- Edit;
- Duplicate;
- Open Map;
- Run Headless;
- Run Batch.

Non implementare future tool se non già pianificati.

# 7. Scenario Runner

Deve usare lo Scenario System canonico.

Lo scenario deve essere owner o reference di:
- stable Scenario ID;
- map;
- initial state;
- units;
- spawn;
- teams;
- bot profiles;
- objective;
- turn limit;
- seed;
- scripted intent source, se previsto;
- expected assertions;
- category/tags.

Il tester non deve ricostruire manualmente lo scenario ogni volta.

Runner minimo:

```text
Load
Validate
Initialize
Run
Pause/Stop se visual
Restart
Result
```

Determinismo:

```text
same Scenario + same Seed
    ->
same StateHash + same LogHash
```

quando il test/scenario è deterministico.

# 8. BOT SIMULATION

Separata dal normale Scenario Browser come entry di sviluppo.

v0.1:

```text
BOT SIMULATION

Map
Scenario
Team A Bot Profile
Team B Bot Profile
Seed
Max Turns
Presentation = Visual

[ START ]
```

Controlli:
- Pause;
- Stop;
- Next Turn;
- 0.25x;
- 0.5x;
- 1x;
- 2x;
- 4x.

La velocità influenza solo presentation.

Acceptance criteria:
- bot producono intenti tramite API reali;
- niente shortcut dirette nel resolver;
- same seed/profile produce ordering stabile dove previsto;
- max turn cap;
- nessun crash su run consecutive;
- TurnLog valido;
- Result raggiungibile.

Batch/headless simulation è un hook futuro, non obbligatorio v0.1.

# 9. RESULT

Unificare Match Result e Scenario Result con un modello condiviso dove possibile.

## Match Result
Mostrare:
- Outcome;
- Winner;
- Turns;
- Objective;
- Team state summary.

Azioni:
- TurnLog;
- Replay;
- Play Again;
- Main Menu.

## Scenario Result
Mostrare:
- Scenario ID;
- PASS / FAIL / ERROR;
- Turns;
- Winner / outcome;
- Assertions;
- StateHash;
- LogHash.

Azioni:
- TurnLog;
- Replay;
- Run Again;
- Scenarios.

Regola: la UI non ricalcola il risultato.

# 10. REPLAY MINIMO v0.1

Non creare ancora un Replay Browser completo.

Scope:
- ultimo run;
- Play/Pause;
- playback speed;
- Next Event;
- Next Turn;
- focus event/unit;
- return to Result.

Replay deve essere presentation del risultato canonico e non deve decidere o ricalcolare gameplay.

# 11. PAUSE

Offline v0.1:

```text
RESUME
SETTINGS
RESTART MATCH / SCENARIO
RETURN TO MAIN MENU
```

Future multiplayer:
- niente pausa globale;
- eventuale Surrender / Leave Match.

Questa differenza va preservata nell'architettura.

# 12. SETTINGS

Struttura:

```text
SETTINGS
├── VIDEO
├── AUDIO
├── CONTROLS
├── GAMEPLAY
└── ACCESSIBILITY
```

v0.1 minimo:

### Video
- Display Mode;
- Resolution;
- VSync;
- FPS Limit;
- Quality Preset se già disponibile.

### Audio
- Master;
- Music;
- SFX;
- UI.

### Controls
- Pan;
- Rotate;
- Zoom;
- Select;
- Confirm;
- Cancel;
- Ready;
- Ping solo se già nello scope.

### Accessibility
- UI Scale;
- Reduced Camera Motion;
- Screen Shake;
- segnali non solo cromatici.

# 13. Loading / Transition

Flow comune:

```text
Menu
 ↓
Loading
 ↓
Map Load
 ↓
Scenario/Match Initialize
 ↓
Ready
 ↓
Fade In
```

Messaggi:
- Loading map...
- Initializing scenario...
- Preparing bots...

Non mostrare percentuali fittizie se non esiste un vero progress model.

# 14. Error Handling

Modal comune:

```text
SCENARIO COULD NOT START

Scenario:
RT_SCN_xxx

Reason:
...

[ BACK ]

[ DETAILS ]   // solo Development
```

In Dev può esserci `COPY DEBUG INFO`.
In Shipping solo messaggio utente leggibile.

# 15. Struttura UMG consigliata

Adattare ai nomi reali del repository.

Frontend:

```text
WBP_FrontendRoot
│
├── MainMenu
├── Play
├── Training
├── Scenarios
├── Settings
└── ModalLayer
```

In-match:

```text
WBP_GameHUDRoot
│
├── TacticalHUD
├── FastDecision
├── Pause
├── Result
└── DebugOverlay
```

Principio: `Frontend != In-Match HUD`.

# 16. Navigation Controller

Evitare logica sparsa `CreateWidget/RemoveFromParent/AddToViewport` in ogni widget.

Usare o introdurre un singolo owner del flow, coerente con il codice reale, con concetti:
- Push Screen;
- Pop Screen;
- Show Modal;
- Close Modal;
- Return Main.

# 17. Stati UI comuni

Button:
- Normal;
- Hover;
- Pressed;
- Selected;
- Disabled;
- Focused.

Scenario:
- PASS;
- FAIL;
- NOT RUN;
- RUNNING;
- INVALID.

Mode:
- AVAILABLE;
- LOCKED;
- COMING SOON;
- DEV ONLY.

Non affidarsi soltanto al colore.

# 18. Roadmap menu v0.1 -> v1.0

Questa roadmap deve essere integrata nella roadmap generale, non creare un piano parallelo.

## v0.1
- Main;
- Play;
- Match Setup/Briefing minimo;
- Training Lite;
- Scenario Browser;
- Scenario Detail;
- Scenario Runner;
- Bot Visual Simulation;
- Settings base;
- Pause;
- Result;
- TurnLog entry;
- last replay;
- loading;
- error modal.

## v0.2
- Character Browser;
- Character Training;
- Reaction Training/Lab;
- Tactical Bot controls;
- richer replay for reactions.

## v0.3
- Mechanics Library;
- Vision Lab;
- Sound Lab;
- Knowledge debug;
- Replay Browser iniziale;
- Tactical Analysis.

## v0.4
- Environment Lab;
- multilayer map inspection;
- layer-aware replay;
- performance dashboard iniziale.

## v0.5
- Custom Match;
- advanced combat lab;
- objective scenario tools;
- mature knowledge/fog inspector.

## v0.6
- Scenario Composer;
- Skill/Ability Workbench;
- Character Setup;
- Content Validator UI;
- Balance Lab.

## v0.7
Se networking è canonico qui:
- Online Play;
- Lobby;
- Party;
- reconnect UX;
- Network Privacy Lab;
- Spectator.

## v0.8
Se product roadmap lo conferma:
- Quick Play;
- Ranked;
- Match History;
- Stats;
- competitive replay/analysis.

## v0.9
- no nuove sezioni core;
- polish;
- accessibility;
- localization/layout;
- controller;
- error/loading hardening;
- UX freeze.

## v1.0
- final frontend;
- onboarding;
- mature training;
- replay/analysis;
- release UX certification.

# 19. Scope escluso dalla v0.1

Non creare ora:
- matchmaking;
- ranked;
- social;
- profile;
- progression;
- shop;
- cosmetics;
- battle pass;
- full replay browser;
- public mod browser;
- production multiplayer lobby;
- advanced loadout system.

# 20. Epic/Issue da verificare

Prima cercare owner già esistenti.

Possibili responsabilità:
- EPIC — Frontend Shell;
- EPIC — Scenario UX / Runner;
- EPIC — Bot Visual Simulation;
- EPIC — Result / Replay;
- EPIC — Training;
- EPIC — Settings / Accessibility.

NON creare sei Epic se il tracker corrente preferisce una sola Epic UI/Frontend con child issue.

# 21. Issue minime v0.1

Verificare se già esistono.

## Frontend
- Main Menu;
- navigation controller/router;
- modal/error/loading;
- Play entry;
- Training entry;
- Scenarios entry;
- Settings entry;
- Quit flow.

## Play
- Match Setup minimo;
- Briefing;
- launch vertical slice;
- Result return;
- Play Again.

## Scenario
- Scenario Browser;
- Scenario filters/status;
- Scenario Detail;
- Scenario Runner UI;
- invalid scenario state.

## Bot
- Bot Simulation menu;
- visual bot match launch;
- speed/pause/next turn;
- stop/restart.

## Result/Replay
- Result ViewModel;
- Scenario assertions view;
- TurnLog entry;
- last replay;
- Run Again.

## Training
- Training Browser/Lite;
- movement exercise;
- basic combat exercise se già supportato;
- reset/repeat.

## Settings
- tabs;
- base options;
- persistence;
- accessibility.

## Pause
- resume;
- settings;
- restart;
- return main.

# 22. Acceptance criteria trasversali

Ogni Issue deve dichiarare:
- owner code/UI;
- Feature ID;
- release;
- dependencies;
- scenario/test;
- manual PIE;
- packaged requirement;
- debug/log;
- privacy impact;
- performance impact;
- accessibility impact.

Minimo:

```text
[ ] no dead-end navigation
[ ] UI does not mutate resolver state directly
[ ] no duplicated scenario catalog
[ ] no hidden enemy data leak
[ ] repeatable scenario launch
[ ] error state is visible and actionable
[ ] 1080p readable
[ ] keyboard/mouse flow verified
[ ] packaged smoke where applicable
```

# 23. Test da aggiungere/associare

Navigation:
- Main -> Play -> Back;
- Main -> Training -> Back;
- Main -> Scenarios -> Detail -> Back;
- Main -> Settings -> Back;
- modal open/close;
- invalid route fallback.

Scenario:
- valid run;
- invalid scenario;
- same seed repeat;
- restart;
- return to browser.

Bot:
- visual simulation starts;
- pause does not mutate logical result;
- 0.25x/1x/4x same canonical result;
- stop returns cleanly;
- max turns respected.

Result:
- result matches resolver;
- assertions match Scenario Runner;
- hashes match canonical values;
- Run Again uses expected seed policy.

Replay:
- no state mutation;
- event order stable;
- playback speed independent.

Training:
- reset restores initial fixture;
- same exercise repeatable.

Settings:
- apply;
- cancel/back;
- persistence where supported;
- safe fallback for invalid values.

# 24. Tracking da aggiornare

Claude deve aggiornare i file reali equivalenti a:
- roadmap;
- Feature Registry;
- UI/frontend spec;
- Scenario Registry;
- Scenario Map;
- Editor Map;
- test matrix;
- asset task/registry;
- documentation/wiki;
- GitHub milestone/project;
- Epic/Issue dependency graph.

Se alcuni sono generated:
1. modificare la source canonica;
2. eseguire il generator;
3. verificare diff;
4. non editare manualmente il file generato.

# 25. Mapping richiesto

Alla fine creare una matrice:

| Feature | Feature ID | Release | Epic | Issue | Scenario | Test | Asset | Status |
|---|---|---|---|---|---|---|---|---|
| Main Menu | | v0.1 | | | | | | |
| Play Flow | | v0.1 | | | | | | |
| Briefing | | v0.1 | | | | | | |
| Training Lite | | v0.1 | | | | | | |
| Scenario Browser | | v0.1 | | | | | | |
| Scenario Detail | | v0.1 | | | | | | |
| Scenario Runner UI | | v0.1 | | | | | | |
| Bot Simulation UI | | v0.1 | | | | | | |
| Settings | | v0.1 | | | | | | |
| Pause | | v0.1 | | | | | | |
| Result | | v0.1 | | | | | | |
| Last Replay | | v0.1 | | | | | | |
| Loading | | v0.1 | | | | | | |
| Error Modal | | v0.1 | | | | | | |

# 26. Priorità v0.1

## P0
- navigation shell;
- Play launch;
- Scenario Browser;
- Scenario Runner;
- Bot Visual Simulation launch;
- Result;
- loading/error;
- packaged flow.

## P1
- Briefing;
- Settings;
- Pause;
- TurnLog entry;
- replay minimo;
- Training Lite.

## P2
- richer filters;
- last-run status;
- debug details;
- UI polish;
- additional training cards.

# 27. Ordine implementativo consigliato

```text
Frontend Root / Navigation
        ↓
Loading + Error modal
        ↓
Main Menu
        ↓
Play launch
        ↓
Scenario Browser
        ↓
Scenario Detail
        ↓
Scenario Runner integration
        ↓
Bot Simulation launch
        ↓
Result
        ↓
Pause / Settings
        ↓
Replay
        ↓
Training Lite
        ↓
Polish / Accessibility
        ↓
Packaged smoke
```

# 28. Parallelizzazione

Se i write-set lo consentono:

Track A — Flow
- Frontend Root;
- Navigation;
- Loading/Error;
- Main Menu.

Track B — Scenario UI
- Browser;
- Detail;
- Runner integration;
- Result assertions.

Track C — Bot / Replay
- Bot Simulation;
- playback controls;
- replay last run.

Track D — Settings / Training
- Settings;
- Pause;
- Training Lite.

Non lavorare in parallelo sullo stesso `.uasset` senza ownership chiara.

# 29. Definition of Done

Una feature menu non è Done solo perché il widget si apre.

Dove applicabile deve avere:
- SPEC;
- VIEW MODEL / DATA CONTRACT;
- UMG;
- INPUT;
- NAVIGATION;
- ERROR STATE;
- AUTOMATION / FUNCTIONAL TEST;
- MANUAL PIE;
- ACCESSIBILITY;
- PACKAGED;
- TRACKING;
- DOCS.

# 30. Output richiesto a Claude prima di modificare

Audit:

```text
Branch:
HEAD:
UE version:
Current frontend owner:
Current scenario UI:
Current tracking owners:
```

Reconciliation:

```text
Feature
Existing Epic/Issue
Existing code/widget
Status
REUSE/UPDATE/CREATE/DEFER
```

Tracking plan:
- Epic da aggiornare;
- Epic da creare;
- Issue da aggiornare;
- Issue da creare;
- Feature Registry changes;
- Scenario/test changes;
- roadmap changes.

# 31. Output finale richiesto

A. Files changed — path e motivo.

B. Epic — ID, title, status, release.

C. Issues — ID, title, parent, priority, dependencies, acceptance criteria, tests.

D. Tracking — Feature Registry, roadmap, scenario/test registry, asset tracking, docs/wiki.

E. Implementation status — `AS_BUILT`, `PARTIAL`, `PLANNED`, `BLOCKED`.

F. v0.1 critical path aggiornato dopo l'audit.

G. `NEXT EXECUTABLE ISSUE` — una sola Issue concreta da implementare subito.

# 32. Vincoli

- non creare un secondo Scenario System;
- non creare un secondo Replay model;
- non creare un secondo TurnLog;
- non duplicare Feature ID;
- non inventare naming se esiste già;
- non spostare hidden planning nei widget;
- non introdurre matchmaking/ranked/progression in v0.1;
- non rendere CommonUI obbligatorio se non già deciso;
- non accoppiare UI e resolver;
- non modificare file generated manualmente;
- non chiudere Issue senza evidence;
- non dichiarare Done senza test/tracking.

# Definition of Success

Il task è concluso quando il repository ha:
1. una frontend architecture v0.1 chiaramente documentata;
2. le feature Menu/Scenario/Training/Result tracciate;
3. Epic e Issue senza duplicati;
4. Feature Registry aggiornato;
5. roadmap v0.1→v1.0 coerente;
6. Scenario/Test tracking collegato;
7. acceptance criteria e test definiti;
8. un critical path v0.1 aggiornato;
9. una `NEXT EXECUTABLE ISSUE` pronta per l'implementazione.
