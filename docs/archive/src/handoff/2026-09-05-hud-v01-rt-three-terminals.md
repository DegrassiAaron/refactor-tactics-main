# REFACTORTACTICS — CLAUDE CLOUD EXECUTION HANDOFF
## HUD v0.1 · CODE + VALIDATION + EDITOR/MCP · Three terminals, same checkout

> ## 📸 `HISTORICAL` — SORGENTE CONSUMATO, NON NORMATIVO
>
> Work order arrivato in radice come `CLAUDE_HUD_V01_RT_THREE_TERMINALS_HANDOFF.md`, **untracked** e fuori
> da qualunque repository (`D:\Repositories\refactor-tactics-technical-designer` non è un checkout git).
> Consumato il **2026-09-05** da
> [`../../../roadmap/plans/hud-v01-three-terminals-audit-2026-09-05.md`](../../../roadmap/plans/hud-v01-three-terminals-audit-2026-09-05.md),
> che ne produce i tre deliverable del §15 — le due lane
> [A](../../../roadmap/plans/hud-v01-code-architecture-roadmap.md) e
> [B](../../../roadmap/plans/hud-v01-editor-verification-roadmap.md) e l'audit stesso.
>
> **Non è una fonte.** Quattro delle issue che il §3 elenca come «residuali da riconciliare» erano **già
> chiuse** quando è arrivato — `#78` (2026-08-25), `#2193`, `#2288`, `#2347` (2026-09-04) — e l'epic owner
> del Player Event Log, `#1937`, non è nominata. Il suo §1.4 («più check non significa più qualità») e il
> suo §1.5 (anti-vacuità: `performed > 0`) sono invece la parte che vale, e sono passati nelle due lane.
>
> ⚠️ Il §15 chiede **due roadmap**. Le due lane sono state scritte come viste `EPHEMERAL` che nominano
> l'owner riga per riga e **non registrano verdetti**, perché il rischio che il documento stesso dichiara al
> §1.1 — la seconda source of truth — è lo stesso che
> [`../../../roadmap/plans/hud-planning-prediction-dual-roadmap-spec-panel-2026-09-05.md`](../../../roadmap/plans/hud-planning-prediction-dual-roadmap-spec-panel-2026-09-05.md)
> aveva già contestato al brief gemello, lo stesso giorno.

**Repository:** `DegrassiAaron/refactor-tactics-main`  
**Target:** E11 HUD/log/debug + E20 icon language + residuali player-facing strettamente necessari alla v0.1  
**Operating model:** `RT Three Terminals`  
**Output del run:** due mini-roadmap esecutive temporanee:
- **Lane A — CODE / ARCHITECTURE**
- **Lane B — EDITOR / MCP / USER**

Le due lane NON diventano nuove roadmap canoniche. Lo stato ufficiale resta nei documenti/issue owner del repository.

---

# 0. MISSIONE

Riconciliare lo stato corrente di HUD v0.1 e costruire un percorso eseguibile che porti a:

```text
partita v0.1
→ HUD player-facing leggibile
→ stato turno/fase/timer comprensibile
→ roster + selected unit
→ ActionDock/slot
→ intenti/certainty
→ Ghost Timeline / prediction
→ Player Event Log leggibile
→ pointer coerente
→ Ready / countdown / Unready se ancora scope corrente
→ unit overlay/status leggibili
→ debug disponibile ma non primario
→ PIE verificato
→ packaged verificato solo dove aggiunge evidenza reale
```

Principio:

```text
Simulator / authoritative runtime decides
        ↓
ViewModel / DTO / semantic IDs expose
        ↓
UMG / HUD / overlay displays
        ↓
MCP verifies structure
        ↓
User PIE verifies perception/input
```

Nessun widget, Blueprint o asset Editor deve diventare una seconda authority.

---

# 1. CRITICA PRELIMINARE — VINCOLI DA NON PERDERE

## 1.1 Due roadmap, ma una sola source of truth

Lane A e Lane B sono viste esecutive temporanee.

- **Lane A produce contratti.**
- **Lane B consuma e falsifica quei contratti.**

Ogni task CODE player-facing deve indicare quale verifica Editor lo chiude.
Ogni check Editor deve indicare quale producer runtime sta verificando.

Vietato creare:
- roadmap CODE scollegata dai WBP reali;
- roadmap Editor che “aggiusta” gameplay;
- duplicati di issue già owner del lavoro.

## 1.2 “Editor alla fine” non significa “zero Editor prima”

Usare un **pre-flight minimo** prima del grosso del codice:

- root/HEAD/branch corretti;
- Editor apre;
- asset critici esistono;
- Blueprint/UMG caricabili;
- mappa di verifica apribile;
- MCP disponibile o classificato non disponibile;
- asset registry stabilizzato.

Poi tornare a `DEV`.

La vera campagna di accettazione va concentrata dopo il batch CODE + VALIDATION.

Eccezione: se una slice cambia una Blueprint API/base class o rende impossibile continuare senza un smoke test di integrazione, fare SOLO lo smoke necessario.

## 1.3 MCP non è un giocatore

Classificare ogni verifica:

- `AUTO_STATIC`
- `AUTO_UNREAL`
- `MCP_READ`
- `MCP_WRITE`
- `USER_PIE`
- `USER_PACKAGED`

Regola:

```text
“Quale proprietà ha?”        → MCP_READ
“Posso collegarla?”          → MCP_WRITE se supportato
“La regola è corretta?”      → AUTO
“Si vede / si capisce?”      → USER_PIE
“Il click si comporta bene?” → USER_PIE
“Funziona cooked?”           → USER_PACKAGED
```

`MCP command sent != verified`.

`result=null`, output vuoto o assenza di errore NON equivalgono a PASS.

Oracoli validi:
- reread property;
- reopen asset;
- explicit compile;
- test;
- PIE;
- packaged.

## 1.4 Più check non significa più qualità

Ogni check Editor deve dichiarare la failure class che può scoprire e che AUTO non copre.

| Check | Failure class |
|---|---|
| icona status visibile | texture non caricata / size 0 / clipping |
| centro HUD libero | layout valido ma board illeggibile |
| click-through | hit-test/Z-order |
| Ghost Timeline | preview semanticamente corretta ma visualmente ambigua |
| Ready countdown | dato corretto, feedback assente |
| Player Event Log | grouping corretto in test ma feed inutilizzabile |
| packaged | asset/editor-only/cook |

Se non aggiunge una failure class, non inserire il check.

## 1.5 Anti-vacuità

Per ogni validation run riportare:

```text
command
HEAD
found N
performed N
passed N
failed N
exit code
```

Hard rule:

```text
critical filter => performed > 0
```

Quando opportuno usare:
- mutation;
- planted failure;
- negative fixture;
- positive control.

“Suite verde” senza misure non è evidenza.

## 1.6 FAIL Editor → torna all'owner corretto

```text
EDITOR FAIL
   ↓
classify:
CODE | ASSET | SETUP | SPEC | TOOLING
   ↓
find existing owner
   ↓
fix nella lane appropriata
   ↓
targeted validation
   ↓
re-run affected editor check
```

Non correggere in silenzio il widget per compensare un contratto runtime errato.

---

# 2. SEARCH BEFORE CREATE

Prima di creare QUALSIASI issue:

1. `git fetch --all --prune`
2. misura:
   - `git rev-parse --show-toplevel`
   - `git branch --show-current`
   - `git status --short`
   - `git rev-parse HEAD`
   - `git rev-parse origin/main`
3. cerca issue OPEN e CLOSED per:
   - titolo;
   - sinonimi;
   - simboli C++;
   - WBP;
   - asset path;
   - semantic IconId;
   - PIE marker.
4. leggi:
   - epic/parent;
   - commenti più recenti;
   - document owner.
5. verifica:
   - Source reale;
   - Content reale;
   - test reali;
   - documentazione reale.
6. classifica:

```text
REUSE
UPDATE_EXISTING
LINK_ONLY
ALREADY_DONE
CREATE_NEW
DEFER
DECISION_REQUIRED
REGRESSION
```

Default desiderato: **0 nuove issue**, salvo gap misurato senza owner.

Non creare issue generiche:
- “fare HUD”;
- “aggiungere icone”;
- “fare Ghost”;
- “fare pointer”;
- “fare Player Log”;
- “fare Ready”.

Prima dimostrare che l'owner esistente non copre il residuo.

---

# 3. BASELINE HUD DA RIMISURARE LIVE

Questi anchor sono noti, ma NON vanno trattati come stato live senza verifica.

## Epic / checkpoint

- `#25` — E11 HUD, log e debug
- `#217` — E20 HUD Icon Language

Checkpoint E11 noti:
- `#77` — CP 11.1 HUD
- `#78` — CP 11.2 intenti/certainty
- `#79` — CP 11.3 combat/debug log
- `#80` — CP 11.4 `rt.Debug.*`
- `#172` — CP 11.5 Ghost Timeline
- `#173` — CP 11.6 scrubbing / reaction conditional branch
- `#613` — CP 11.7 Screen HUD UMG
- `#705` — CP 11.8 Pointer Interaction Contract

Residuali/consumer da riconciliare:
- `#1936` — Player Event Log
- `#2193` — Ready countdown / Unready
- `#2288` — UnitOverlay WidgetComponent
- `#2347` — icone status realmente viste a schermo
- eventuali issue più recenti nate dopo questo handoff.

## Lane UI canonica nota da verificare

```text
#219/#637 → #220 → #77/#613 → #705 → #291
```

NON inserire automaticamente #172/#173 dentro questa catena come dipendenze canoniche.
Possono essere critiche per UX ma la relazione deve essere classificata come:
- FACT dependency;
- sequencing dependency;
- UX dependency;
- inferred recommendation.

---

# 4. ASSET / WIDGET BASELINE DA RIMISURARE

Cercare e classificare almeno:

- `WBP_RT_TacticalHUD`
- `WBP_RT_TurnHeader`
- `WBP_RT_TeamRoster`
- `WBP_RT_SelectedUnitPanel`
- `WBP_RT_ActionDock`
- `WBP_RT_ActionSlot`
- `WBP_RT_UnitCard`
- `WBP_RT_UnitOverlay`
- `WBP_RT_EventLog` se ormai esiste
- `DA_IconCatalog`

Classificazione:

```text
EXISTS_COMPLETE
EXISTS_PARTIAL
EXISTS_STALE
PLACEHOLDER
UNCONNECTED
UNUSED
MISSING
UNKNOWN
```

Non proporre di ricreare un asset esistente.

## Icon rule

Contratto obbligatorio:

```text
WBP consumer
   ↓
semantic IconId
   ↓
DA_IconCatalog
   ↓
texture
```

Vietato introdurre:

```text
WBP → direct texture hardcode
```

Se esistono sorgenti SVG/PNG non importate:
- trattarle come source asset;
- non importare tutto;
- popolare solo ciò che la v0.1 richiede;
- prima risolvere collisioni/naming.

---

# 5. DUE MINI-ROADMAP, UNA SOLA SOURCE OF TRUTH

## TARGET

```text
HUD v0.1:
state readable
+ plan readable
+ prediction readable
+ outcome explainable
+ input coherent
+ no privacy leaks
+ packaged player view clean
```

**Lane A = CODE / ARCHITECTURE**  
**Lane B = EDITOR / MCP / USER**

Sono temporanee.
Non sostituiscono:
- `roadmap-v0.1.md`;
- `roadmap-checkpoint.md`;
- issue owner;
- decision log;
- `test-manuali-pie.md`.

---

# 6. MINI-ROADMAP A — CODE / ARCHITECTURE

Creare:

`docs/roadmap/plans/hud-v01-code-architecture-roadmap.md`

Ogni step deve avere:

| Campo | Contenuto |
|---|---|
| ID | `HUD-CODE-Cx` |
| Owner | issue / decision |
| State live | DONE / PARTIAL / TODO / BLOCKED / VERIFY |
| Intent | cosa rende leggibile/decidibile |
| Producer | classe/modulo runtime |
| Contract | DTO / VM / semantic ID / event |
| Consumer | WBP/HUD/overlay |
| Dependencies | hard / sequencing / UX |
| Implementation minimum | slice minima |
| Tests | targeted |
| Anti-vacuity | N>0 / mutation / control |
| Determinism | impact |
| Privacy | impact |
| Replay/TurnLog | impact |
| Editor handoff | `HUD-EDITOR-Ex` |
| Exit | `CODE READY` criteria |

## C0 — Audit

Eseguire:
- root/branch/status/HEAD/origin;
- document owner;
- issue state;
- Source/Content;
- test discovery.

Output minimo:

```text
Issue | live state | code | asset | test | residual | action
```

## C1 — Icon language / semantic contract

Owner da riconciliare: E20 / #220 e correlati.

Verificare:
- semantic keys correnti;
- catalog reale;
- missing required IDs;
- direct texture references;
- generated source assets;
- naming collisions.

CODE READY quando:
- catalog contract stabile;
- test reale sul catalogo;
- critical filters `N > 0`;
- nessun widget nuovo deve conoscere texture path.

Editor handoff:
- E1 Asset/Catalog Audit;
- E2 Widget bindings.

## C2 — HUD ViewModels / base state

Owner da riconciliare: #77 / #613.

Copertura minima:
- round;
- phase;
- timer;
- team roster;
- selected unit;
- HP;
- shield;
- energy;
- Movement/Main/Reaction slots;
- cooldown;
- Ready state dove previsto.

Hard rule:
widget non calcola formule o legality.

Editor handoff:
- E2 binding;
- E3 layout;
- E4 state matrix.

## C3 — Screen HUD UMG contract

Owner: #613.

Verificare separazione:

### Screen HUD
- header;
- roster;
- selected;
- dock;
- warning;
- player event feed;
- plan confirmation.

### Tactical World Overlay
- path;
- waypoint;
- destination;
- AoE;
- friendly fire;
- facing;
- cones;
- world anchored information.

Non migrare World Overlay nel Screen HUD solo per uniformità.

CODE READY:
- ogni consumer ha dato stabile;
- nessun gameplay rule nei widget;
- player debug off by default.

## C4 — Player Event Log

Owner da riconciliare: #1936 + #79/#613.

Target architecture:

```text
Resolver
  ↓
Canonical TurnLog
  ↓
authorization predicate
  ↓
Player Event Projector
  ↓
FRTPlayerEvent[]
  ↓
WBP_RT_EventLog
```

Vietato:
- parsing diagnostic strings;
- UI recomputation;
- privacy filter dopo la generazione di testo;
- semplificare il TurnLog debug per rendere il player log leggibile.

Test:
- grouping;
- dominance;
- movement spam;
- ordering;
- authorization parity;
- death/public exception se ancora corrente;
- hash independence.

Editor handoff:
- E8 Player Event Log.

## C5 — Ghost Timeline / prediction contract

Owner: #172/#173 da riconciliare.

Verificare campi reali correnti:
- Phase;
- UnitId;
- ActionId;
- PreviewOrigin;
- PreviewDestination;
- Facing;
- TargetCells;
- AffectedCells;
- Certainty;
- ReactionPreview se previsto.

Hard rule:
preview usa gli stessi dati/primitive canoniche del resolver.

Mai:
- secondo pathfinder;
- secondo targeting;
- seconda cover rule.

Test:
- preview origin;
- destination;
- residual move budget;
- planned Dash;
- Blast geometry;
- privacy;
- certainty;
- fallback/reason.

Editor handoff:
- E7 Ghost/Scrubbing.

## C6 — Pointer interaction contract

Owner: #705.

Rimisurare matrice corrente:
- contexts;
- Hover;
- LMB;
- RMB;
- HUD/world precedence;
- Cancel;
- Blocked(reason).

Hard rule:
input propone; runtime decide legality.

Test:
- hover never commits;
- RMB cancels preview only;
- no click-through logical regressions;
- hidden enemy privacy;
- playback restrictions;
- reason for refusal.

Editor handoff:
- E5 Pointer.

## C7 — Ready / Unready

Owner: #2193 se ancora scope corrente.

Contratto noto da riconciliare:

```text
Planning
 → ReadyCountdown
    → Cancel → Planning
    → expiry → LockInAndResolve
```

Vincoli:
- UX wall-clock;
- no snapshot field;
- no TurnLog state;
- no StateHash state;
- planning cap wins;
- plan preserved on cancel.

AUTO prima di Editor:
- delayed resolution;
- expiry;
- cancel;
- plan preserved;
- cap wins;
- zero countdown baseline;
- harness unchanged.

Editor handoff:
- E10 Ready/Unready.

## C8 — Unit overlay / status

Owner da riconciliare:
- #2288
- #2347
- eventuali successori.

Verificare:
- `BuildUnitCard`;
- `BuildStatusBadges`;
- catalog IDs;
- knowledge filter;
- overlay visibility;
- screen/world space choice;
- clamp/behind-camera contract attuale.

AUTO:
- badge construction;
- icon IDs;
- duration rules;
- cell-bound rules;
- observer knowledge.

Editor handoff:
- E9 UnitOverlay/status.

## C9 — `rt.Debug.*`

Owner: #80 e residuali.

Rimisurare numero/comandi LIVE.
Non copiare conteggi storici.

Per ogni command:
- registered;
- discovered;
- actual behavior;
- privacy;
- Development availability.

Se dice `DrawX`, verificare se disegna davvero o stampa soltanto.

Editor handoff:
- E11 Debug observation.

## C10 — CODE READY batch gate

Prima di cedere Unreal a VALIDATION:

- code changes complete;
- static checks;
- tests authored;
- no concurrent Editor;
- no binary dirty;
- docs updated se contract change;
- explicit list di validation commands;
- explicit list di Editor checks enabled.

Stato:
`CODE READY`, non `VALIDATED`.

---

# 7. MINI-ROADMAP B — EDITOR / MCP / USER

Creare:

`docs/roadmap/plans/hud-v01-editor-verification-roadmap.md`

Ogni check:

| Campo | Contenuto |
|---|---|
| ID | `HUD-EDITOR-Ex` |
| Consumes | `HUD-CODE-Cx` |
| Owner | issue / PIE / session |
| Class | MCP_READ / MCP_WRITE / USER_PIE / USER_PACKAGED |
| Preconditions | build/map/CVar |
| Asset | exact path |
| Setup | before Play |
| Action | what is done |
| Expected | observable |
| Failure class | what it can falsify |
| Evidence | dump/screenshot/video/log |
| Result | PASS/FAIL/BLOCKED/NOT RUN |
| Failure owner | existing issue |
| Regression set | what to rerun |

## E0 — Pre-flight

Target: minimo.

- same root/head/branch;
- Editor starts;
- correct checkout;
- MCP discovery;
- maps/assets load;
- no obvious compile errors;
- asset registry stable;
- restart if first-load issue is known.

Non giudicare ancora il layout.

## E1 — Asset / Catalog Audit

Preferenza:
`MCP_READ + AUTO evidence`.

Verificare:
- `DA_IconCatalog`;
- required keys;
- texture references;
- direct texture bypass;
- missing icons;
- naming collision;
- placeholder;
- redirector;
- generated vs authored source.

Output asset gap:

| Asset/key | Required by | Exists | Connected | PIE | Packaged | Action |
|---|---|---:|---:|---:|---:|---|

## E2 — UMG hierarchy / binding audit

Preferenza:
`MCP_READ`.

Target:
- TacticalHUD;
- TurnHeader;
- TeamRoster;
- SelectedUnitPanel;
- ActionDock;
- ActionSlot;
- EventLog;
- UnitOverlay.

Cercare:
- missing binding;
- static placeholder;
- `Text Block`;
- wrong default visibility;
- wrong parent class;
- invalid z-order;
- unexpected hit-test;
- widget not mounted;
- consumer bypassing VM/catalog.

MCP_WRITE solo se il run futuro è autorizzato all'implementazione.

## E3 — Static layout + visual layout

MCP per struttura.
USER_PIE per leggibilità.

Verificare:
- centro board libero;
- header;
- roster;
- selected panel;
- action dock;
- event feed;
- plan controls;
- no overlap critico;
- scale/resolution target v0.1;
- no clipped text essenziale.

Scope:
functional readability, non polish infinito.

## E4 — HUD state matrix

USER_PIE.

Costruire stati reali:
- Planning idle;
- selected unit;
- path preview;
- target preview;
- selected action;
- cooldown;
- invalid action;
- Ready;
- countdown;
- Resolution;
- ReactionWindow;
- Modal;
- damaged unit;
- shield/energy change;
- objective/event update se scope.

Per ogni stato:
- datum changed;
- expected widget response;
- widget that must NOT change.

## E5 — Pointer / hit-test

USER_PIE.

Rimisurare matrice #705 corrente.

Casi:
- Hover;
- LMB;
- RMB;
- HUD over world;
- Modal;
- ReactionWindow;
- Planning;
- Pathing;
- Targeting;
- ResolutionPlayback;
- invalid target;
- cancel;
- click-through;
- hidden enemy;
- read-only intent ghost.

Un rifiuto non deve essere solo “nothing happened” se il contract prevede reason.

## E6 — Privacy / knowledge

MIX:
- AUTO contract;
- MCP_READ consumers;
- USER_PIE final presentation.

Verificare:
- no enemy private intent;
- no private planned facing;
- player log no leak;
- hidden enemy no overlay;
- contact ghost behavior;
- preview no side channel;
- authorized event exception only where canonical.

## E7 — Ghost Timeline / Scrubbing

AUTO + USER_PIE.

Verificare:
- Prep;
- Dash;
- Blast;
- Move;
- origin;
- destination;
- facing;
- targets;
- AoE;
- certainty;
- reaction preview;
- scrub;
- conditional branch;
- residual budget;
- invalid reason.

Gate:
**preview shown to player == semantics used by resolver**.

Se falso:
fix CODE, non rendering-rule locale.

## E8 — Player Event Log

AUTO + MCP_READ + USER_PIE.

Verificare:
- concise feed;
- no per-cell spam;
- dominance;
- KO;
- damage;
- block/interruption;
- reaction;
- objective;
- privacy;
- order;
- debug/audit still detailed;
- feed leaves board/plan usable.

Preferire evidence pairing:
`screenshot/video + corresponding diagnostic log`.

## E9 — UnitOverlay / status

AUTO + MCP_READ + USER_PIE.

Fixture minima:
- one cell-bound status;
- one timed status;
- multiple statuses if order matters.

Check:
- actual icon visible;
- no unwanted text fallback;
- cell-bound no counter;
- timed has counter;
- HP/shield/energy visible;
- hidden enemy no overlay;
- viewport/behind-camera contract;
- reload persistence.

Un test icon resolver verde non chiude la visibilità.

## E10 — Ready / Unready

AUTO già passato prima.
USER_PIE:

- countdown visible;
- Ready != committed visually;
- Unready understandable;
- preview returns;
- no double commit;
- transition to Resolution readable.

## E11 — Developer debug

AUTO + USER_PIE.

Per ogni live `rt.Debug.*`:
- exists;
- command works;
- draw is visible if promised;
- privacy;
- no debug default on player view.

## E12 — Packaged subset

USER_PACKAGED solo dove aggiunge failure class:
- cook;
- HUD mounted;
- icons/materials/animations present;
- no debug default;
- startup to match;
- packaged-specific gate.

Non replicare tutta la PIE campaign.

## E13 — Evidence reconciliation

Aggiornare SOLO owner vivi:
- `test-manuali-pie.md`;
- `editor-sessions.yaml`;
- issue comments;
- roadmap/checkpoint se stato cambia davvero.

States:
- PASS;
- FAIL;
- BLOCKED;
- NOT RUN.

Mai:
“not executed = green”.

---

# 8. PROTOCOLLO TRE TERMINALI — STESSA DIRECTORY

Questa sezione è obbligatoria e governa l'esecuzione.

## 8.1 🟢 DEV

Uso:
- Source;
- Config;
- docs;
- test authoring;
- Git/GitHub;
- static tooling;
- Node/Python.

Regola primaria:
**Unreal deve restare libero.**

Durante DEV non avviare autonomamente:
- UnrealEditor;
- UnrealEditor-Cmd;
- full rt-suite;
- packaging;
- mutation heavy;
- long replay corpus.

Se Unreal validation serve:
- preparare command;
- marcare `VALIDATION PENDING`;
- non mettersi in attesa per partire appena l'utente chiude Editor.

Output tipico:
`CODE READY`.

## 8.2 🟡 VALIDATION

Uso:
finestra deliberata in cui Unreal appartiene ai test.

Ordine:
1. static/tool checks;
2. build;
3. targeted Unreal tests;
4. targeted Scenario Harness;
5. full suite **una volta sul batch**;
6. packaged/heavy solo se richiesto.

Preferire:

```text
C1 targeted
C2 targeted
C3 targeted
→ full suite ×1
```

Non:

```text
C1 full
C2 full
C3 full
```

Output tipico:
`VALIDATED FOR EDITOR`.

## 8.3 🔵 EDITOR

Uso:
- Unreal Editor;
- PIE;
- Unreal MCP;
- `.uasset/.umap`;
- Blueprint;
- UMG;
- Material/MI;
- visual acceptance;
- user evidence.

Durante EDITOR:
nessun altro agente lancia build/suite/commandlet/package concorrente.

Raggruppare i check nella stessa apertura.

Per asset scritti:
una verifica nello stesso processo non prova persistence.

Quando necessario:

```text
Save
→ Stop PIE
→ Close Editor
→ VALIDATION/build
→ Reopen same checkout
→ re-read/rejudge
```

## 8.4 MODALITÀ GLOBALE

Stato macchina esplicito:

```text
DEV | VALIDATION | EDITOR
```

Default:
`DEV`.

Usare i comandi locali correnti se esistono:
- `rtstatus`
- `rtmode DEV`
- `rtmode VALIDATION`
- `rtmode EDITOR`

Rimisurare prima di assumerli presenti.

Aprire tre terminali NON cambia la modalità globale.

## 8.5 STESSO CHECKOUT

In tutti e tre i terminali:

```text
git rev-parse --show-toplevel
git rev-parse HEAD
git branch --show-current
```

Devono combaciare.

Hard rule:

```text
3 terminals
= 1 checkout
= 1 Git state
= 1 binary state
```

Non usare worktree diversi per DEV / VALIDATION / EDITOR dello stesso HUD slice senza decisione esplicita.

## 8.6 CODE READY vs VALIDATED

### CODE READY
- implementation done;
- test authored;
- static checks done;
- Unreal validation may be pending.

### VALIDATED
- requested build done;
- targeted tests run;
- scenarios run when needed;
- included in validated batch;
- evidence recorded.

### EDITOR ACCEPTED
- relevant MCP reads done;
- relevant PIE/user checks done;
- evidence recorded;
- no unexplained fail.

Un gate non eseguito:
`NOT RUN`.

## 8.7 FLUSSO OPERATIVO

```text
DEV
→ VALIDATION
→ EDITOR
→ VALIDATION if final gate/reload/package is required
```

La policy decide QUANDO Unreal può essere usato.
Il mutex decide solo chi lo ottiene tecnicamente.

---

# 9. CONCENTRARE LE SESSIONI EDITOR

Target:
massimo **DUE aperture principali** salvo native rebuild/restart obbligatorio.

## SESSIONE A — HUD INTEGRATION PASS

Solo dopo Lane A + Validation batch.

Sequenza suggerita:
1. verify same checkout;
2. MCP tool discovery;
3. open target map;
4. asset/catalog audit;
5. widget hierarchy/binding audit;
6. layout pass;
7. HUD state matrix;
8. pointer;
9. privacy;
10. Ghost Timeline/scrubbing;
11. Player Event Log;
12. UnitOverlay/status;
13. Ready/Unready;
14. debug developer;
15. save only intentional assets;
16. dirty-state readback;
17. git status.

Se un native rebuild è necessario:
Save → close → VALIDATION → reopen.

## SESSIONE B — FINAL VERIFY

Crearla SOLO se Sessione A non può produrre l'evidenza finale dopo l'ultimo rebuild/fix.

Checklist:
- HUD mounted;
- center free;
- state readable;
- intents/certainty;
- prediction readable;
- input coherent;
- player event feed usable;
- status visible;
- Ready flow if in scope;
- privacy;
- no debug primary;
- no unexpected dirty asset.

Non creare una sessione separata per ogni issue/PIE marker.

---

# 10. MUTUA ESCLUSIONE SUI BINARI

Stessa directory NON significa due writer.

Prima di edit binario:

DEV:
`git status --short`

EDITOR:
- confirm asset clean;
- declare asset path;
- modify;
- Save;
- reread dirty state.

DEV:
- `git status --short`
- `git diff --stat`

Mentre `.uasset/.umap` è dirty:
DEV NON esegue sul path interessato:
- checkout;
- restore;
- reset;
- clean;
- mv;
- branch switch;
- pull/rebase.

Hard rule:
**un solo writer binario alla volta**.

---

# 11. LIVE CODING / NATIVE REBUILD

Ridurre aperture non significa mantenere DLL incoerenti.

Se UBT/Live Coding blocca:
1. Save assets;
2. close Editor;
3. switch global mode → VALIDATION;
4. build;
5. targeted tests;
6. reopen same checkout;
7. continue same integration pass.

---

# 12. ASSET GAP POLICY

Non scaricare/comprare asset prima dell'audit.

Per ogni gap reale:

| Field | Required |
|---|---|
| Asset | name |
| Consumer | WBP/overlay |
| Semantic key | if icon |
| Why needed | gameplay/readability |
| Existing fallback | yes/no |
| Procedural/generator possible | yes/no |
| External download required | yes/no |
| License | required if external |
| v0.1 blocker | yes/no |
| Editor check | ID |

Preferire:
1. existing asset;
2. catalog association;
3. generator/procedural;
4. GrayKit/fallback;
5. external asset.

---

# 13. ORDINE DI ESECUZIONE / MERGE DA PRODURRE

Claude deve derivarlo live.

Formato:

```text
R0 Audit
→ R1 semantic/icon contract
→ R2 HUD ViewModels
→ R3 Screen HUD integration contract
→ R4 Player Event Log
→ R5 Ghost/preview residual
→ R6 Pointer residual
→ R7 Ready/overlay residual if in scope
→ R8 targeted VALIDATION batch
→ R9 full suite ×1
→ R10 Editor Session A
→ R11 fixes from observed failures
→ R12 targeted revalidation
→ R13 Final Verify / packaged subset only if required
```

NON trattare questo esempio come ordine canonico.
Confermare issue state e dependency live.

---

# 14. GATE FINALE HUD v0.1

Un giocatore deve poter:

1. entrare in partita;
2. capire round/fase/timer;
3. vedere il proprio roster;
4. capire unità selezionata e risorse;
5. vedere gli slot azione e cooldown;
6. pianificare;
7. capire dove finirà e da dove agirà;
8. distinguere confermato/predetto/incerto dove applicabile;
9. capire perché un'azione non è valida;
10. confermare/cancellare in modo coerente;
11. vedere Resolution;
12. leggere gli eventi importanti senza log diagnostico;
13. leggere status/unit overlay;
14. non ricevere informazioni private nemiche;
15. continuare il match senza dipendere dal debug HUD.

Gate tecnici:
- no second resolver;
- no second pathfinder;
- no gameplay authority in widget/material;
- privacy before presentation;
- preview consumes canonical semantics;
- same checkout;
- no concurrent binary writers;
- all critical validations performed > 0;
- packaged only where required;
- evidence attached.

---

# 15. OUTPUT OBBLIGATORIO DELLA PASSATA CLOUD

Creare:

### A
`docs/roadmap/plans/hud-v01-code-architecture-roadmap.md`

### B
`docs/roadmap/plans/hud-v01-editor-verification-roadmap.md`

### C — consigliato
`docs/roadmap/plans/hud-v01-three-terminals-audit-YYYY-MM-DD.md`

Audit finale:

```text
Issue | Before | Action | After | Parent | Why | Evidence
```

Sezioni:
- REUSED
- UPDATED
- CREATED
- CLOSED
- DEFERRED
- NO ACTION
- REGRESSIONS FOUND

Per ogni nuova issue proposta:
- Title
- Parent
- Why
- Scope
- Out
- Dependencies
- AC
- Tests
- Editor/MCP verification
- Binary paths
- Determinism
- Privacy
- Packaged
- DoD

Default:
**0 nuove issue**.

---

# 16. REPORT PER SESSIONE EDITOR

Usare:

```text
SESSION:
ROOT:
BRANCH:
HEAD before:
HEAD after:
Global mode:
Unreal version:
MCP tools discovered:
MCP tools actually used:
Map(s) opened:
Assets opened:
Assets modified:
Assets saved:
Assets dirty at end:
PIE checks:
Packaged checks:
Screenshots/video:
Property dumps:
Relevant logs:
Issues verified:
Issues failed:
Issues blocked:
NOT RUN:
git status after:
```

Per ogni visual PASS:
allegare evidenza oppure spiegare perché un altro oracolo è sufficiente.

---

# 17. COMMIT STRATEGY

Piccoli commit coerenti per owner:

- `feat(ui)`
- `feat(hud)`
- `feat(editor)`
- `test(ui)`
- `test(hud)`
- `fix(ui)`
- `docs(hud)`

Separare:
- Source;
- tests;
- generated icon/catalog asset;
- HUD binary assets;
- unrelated map assets.

Evitare mega-commit:
`.umap + HUD + core resolver + unrelated docs`.

---

# 18. NON FARE

- nuova Epic per HUD;
- nuova roadmap canonica;
- nuovo resolver;
- nuovo pathfinder;
- nuovo privacy system;
- nuovo TurnLog player-facing al posto del TurnLog canonico;
- parser di stringhe debug per Player Event Log;
- gameplay calculation in WBP;
- direct texture hardcode;
- import massivo di asset sorgenti non necessari;
- asset esterni senza gap audit;
- una sessione Editor per ogni issue;
- MCP null interpretato come successo;
- “suite verde” senza `performed N`;
- `NOT RUN` interpretato come PASS;
- due worktree per DEV/VALIDATION/EDITOR dello stesso slice;
- due writer sullo stesso `.uasset/.umap`;
- fix packaged/visuale con una modifica di gameplay non giustificata;
- rifare asset/WBP che già esistono senza regression misurata.

---

# 19. PRIMO PASSO

Prima di creare o modificare qualsiasi tracking:

```text
git rev-parse --show-toplevel
git branch --show-current
git status --short
git fetch --all --prune
git rev-parse HEAD
git rev-parse origin/main
```

Poi:

1. audit issue E20/E11 + residuali;
2. audit `Content/RT/UI`;
3. audit test discovery;
4. audit `test-manuali-pie.md`;
5. audit `editor-sessions.yaml`;
6. produrre tabella baseline;
7. solo allora costruire Lane A e Lane B.

**OBIETTIVO FINALE:**  
meno tracking duplicato, meno aperture Unreal, una sola authority, più evidenza reale.
