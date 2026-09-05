# RefactorTactics --- Claudia Handoff v0.1

## Refresh stato issue e checkpoint --- 03/09/2026

**Repository:** `DegrassiAaron/refactor-tactics-main`

> **Regola:** questo file è un handoff operativo, non una fonte di
> verità. Prima di modificare GitHub, verificare lo stato LIVE delle
> issue, label, milestone, assignee, PR e dependency graph. Se un'issue
> è già CLOSED/SUPERSEDED, non riaprirla: aggiornare il grafo e spostare
> eventuali verifiche residue nel corretto owner.

## 1. Obiettivo

Consolidare la roadmap v0.1 in quattro gate:

``` text
D0 Decision Gate
    ↓
FLOW A — Core / Determinism
    ↓
CP-A
    ↓
FLOW B — Knowledge / Presentation / HUD
    ↓
CP-B
    ↓
FLOW C — Complete Match
    ↓
CP-C
    ↓
G13 Packaged
    ↓
v0.1
```

Minimizzare le aperture Editor: - **E1:** authoring/assets/graybox; -
**G1:** build + automation con Editor chiuso; - **E2:** CP-B + CP-C
PIE + frontend; - **G13:** packaged con Editor chiuso.

## 2. Audit obbligatorio

Prima di aggiornare issue:

``` bash
git fetch --prune origin
git rev-parse origin/main
gh repo view DegrassiAaron/refactor-tactics-main
gh issue list --repo DegrassiAaron/refactor-tactics-main --state all --limit 2000
```

Controllare almeno:

``` text
#14 #26 #75 #77 #151 #159 #160 #170
#219 #220 #291 #613 #625 #686 #690 #705
#784 #938 #940 #1496 #1497 #1499 #1525 #1535
#1663 #1665 #1800 #1801 #1922 #1933 #1945 #1957
```

E le Epic:

``` text
#14 v0.1
#25 E11 UI/HUD
#26 E12 determinism/QA/release
#151 E13 perception
#152 E14 reactions/Overwatch
#327 E27 perception v0.3
#773 E40 networking
#774 E41 GAS
#775 E42 dedicated
#776 E43 balance/soak
#777 E44 freeze
#778 E45 production
```

**Non usare conteggi/stati copiati in vecchi handoff come stato
corrente.**

## 3. Stato operativo da verificare LIVE

### Flow A --- Core

  Issue   Ruolo                          Gate
  ------- ------------------------------ -----------------------
  #1957   primo turno / planning lock    CP-A HARD
  #1922   swap / closed cycles           CP-A HARD
  #1800   Facing nel digest              CP-A HARD
  #1933   Overwatch facing nel TurnLog   follow-up CP-A / CP-B

#1933 non deve bloccare il Core Freeze se il suo residuo è solo fidelity
Overwatch/TurnLog.

### Flow B --- Knowledge

  Issue   Ruolo
  ------- ----------------------------------
  #686    HearingThreshold catalog
  #690    NoiseIntensity catalog/validator
  #159    noise → uncertain contact
  #160    partial knowledge HUD/bot
  #1496   knowledge temporal rule
  #1497   movement trace leak
  #1499   presentation authorization
  #1525   playback movement leak
  #1535   perception → PIE
  #784    anti-leak canary

### Flow B --- Presentation/HUD

``` text
#1801 → #1945
#219 / #637 → #220 → #77 / #613 → #705 → #291
```

Verificare LIVE quali edge siano realmente HARD/SOFT/RELATED.

### Flow C --- Match/Release

``` text
#75 → #170
#938 / #940 → frontend acceptance
#1663 + #1665 → G13
```

## 4. D0 --- Decision Gate

Prima di CP-A congelare:

### AUTHOR-RESOLUTION-001

Un solo ordine canonico di resolution v0.1.

### AUTHOR-DAMAGE-001

Un solo contratto v0.1 per: - BaseShield; -
Reduction/Vulnerability/Armor se inclusi; - bypass hazard/ambientale.

**PASS:** nessun contratto v0.1 ambiguo.

## 5. CP-A --- Core Contract Freeze

### Prerequisiti HARD

``` text
#1957
#1922
#1800
```

### Test

``` powershell
.\scriptst-suite.ps1 -Filter HexSim
.\scriptst-suite.ps1 -Filter Determinism
.\scriptst-suite.ps1 -Filter Replay
```

Usare i filtri realmente presenti nel repository.

### Scenario

``` text
seed = 0

direct swap:
A ↔ B = BLOCKED

closed cycle:
A → B → C → A = BLOCKED

free-tail convoy:
A → B → C → FREE = ALLOWED

Facing:
same state + same facing = same hash
same state + different facing = different hash
```

### PASS

-   [ ] primo planning non si chiude prematuramente;
-   [ ] swap bloccato;
-   [ ] cycle bloccato;
-   [ ] free-tail convoy corretto;
-   [ ] Facing nel digest;
-   [ ] stesso input → stesso TurnLog;
-   [ ] stesso input → stesso hash;
-   [ ] snapshot immutabile;
-   [ ] nessuna dipendenza da frame/timing/container order.

### Evidence

``` text
cp-a/
  head.txt
  git-status.txt
  core-tests.log
  determinism.log
  replay.log
  hash-evidence.txt
  cp-a-verdict.md
```

## 6. E1 --- Authoring Marathon

Una sola apertura Editor.

Accumulare: - #219/#220; - #938/#940; - E21 asset se ancora necessari; -
graybox/map authoring; - materiali/presentation; - widget; - eventuali
setup PIE.

Poi chiudere completamente Editor.

## 7. G1 --- Clean Validation

Editor chiuso:

``` bash
git status --short
git rev-parse HEAD
```

Poi build + suite completa secondo gli script reali del repository.

**G1 FAIL → fix → rebuild → G1. Non aprire E2.**

## 8. CP-B --- Knowledge / Presentation / HUD Freeze

### Prerequisiti da verificare LIVE

Core knowledge/presentation:

``` text
#1496
#1497
#1499
#1801
#1945
```

Perception:

``` text
#686
#690
#159
#160
#1535
```

UI:

``` text
#219
#220
#77
#613
#705
#291
```

Anti-leak:

``` text
#784
#1525
```

La classificazione HARD/SOFT deve seguire il graph LIVE.

### Scenario HUD

``` text
Visual.Hud.FirstPlayable
```

Se esposto dal repository:

``` text
rt.Test.Scenario Visual.Hud.FirstPlayable
```

Controllare: - identity; - team; - HP; - cooldown; - action slots; -
phase; - selection; - certainty; - assenza di dati privati nemici.

### Scenario perception

``` text
Visual.Perception.Acceptance
```

Controllare:

``` text
Observed
Remembered / Last Contact
Never Seen
```

PASS: - Observed mostra solo dati autorizzati; - Remembered non mostra
la posizione attuale non osservata; - Never Seen non compare; - nessuna
hidden route; - nessun playback privato; - viewer/team corretti.

### Pointer

``` text
Modal / Reaction
      >
HUD
      >
World
```

PASS: - hover non committa; - RMB annulla preview; - LMB agisce solo
quando autorizzato; - nessun click-through UI→world.

### Event log

Semantico:

``` text
Move
Blocked
Attack
Reaction
Damage Absorbed
KO
Objective
```

La UI non deve ricostruire gameplay.

### CP-B PASS

-   [ ] HUD leggibile;
-   [ ] knowledge corretta;
-   [ ] zero privacy leak;
-   [ ] event log semantico;
-   [ ] attack footprint authoritative;
-   [ ] UI non ricalcola gameplay;
-   [ ] pointer contract;
-   [ ] Menu → Play → Loading → Match;
-   [ ] nessuna duplicazione Canvas legacy incompatibile.

### Evidence

``` text
cp-b/
  g1-clean-suite.log
  hud-first-playable.mp4
  hud-first-playable.png
  perception-observed.png
  perception-remembered.png
  perception-never-seen.png
  pointer-evidence.mp4
  frontend-flow.mp4
  event-log.png
  RefactorTactics.log
  cp-b-verdict.md
```

## 9. CP-C --- Complete Match

### Prerequisiti HARD

``` text
CP-A PASS
CP-B PASS
#75
#170
frontend blockers necessari
```

Non rendere HARD issue che il graph LIVE classifica SOFT/RELATED.

### Baseline match da verificare LIVE

Attesa:

``` text
FormatId       = Format.Skirmish2v2
UnitsPerTeam   = 2
RoundLimit     = 12
ScoreToWin     = 5
MapClass       = Skirmish
```

Se il codice corrente differisce, riportare il valore reale e decidere
se è contratto nuovo o regressione.

### Authored match

Nella stessa apertura E2:

``` text
L_HexArena
```

oppure la mappa canonica corrente del repository.

Verificare:

``` text
Planning
Ready
Snapshot
Resolution
Cleanup
Objective
Combat
Reaction
KO / End
Result
TurnLog
Replay
```

### AutoBattle

Scenario:

``` text
AutoBattle.ArenaV01
```

Valori documentati da verificare LIVE:

``` text
scenarioId = AutoBattle.ArenaV01
version    = 4
seed       = 0
fixture    = ArenaV01
maxTurns   = 40

A1 Gadget (-4, 0,0)
A2 Phase  (-4, 1,0)
B1 Riktor ( 4, 0,0)
B2 Wraith ( 4,-1,0)
```

Non fissare il turno esatto della vittoria.

Invariant: - \[ \] termina prima di maxTurns; - \[ \] nessun illegal
intent; - \[ \] nessun deadlock; - \[ \] TurnLog completo; - \[ \]
stessa build/config/input → stessa traccia.

### CP-C PASS

Outcome valido:

``` text
Team0Wins
Team1Wins
Draw non-degenere
```

Un Draw senza attività significativa è FAIL.

Evidence: - combat/reaction; - objective progress; - TurnLog; -
Result; - replay/hash.

## 10. G13 --- Packaged Gate

Prerequisiti HARD:

``` text
#1663 CLOSED
#1665 CLOSED
```

Poi package.

Percorso:

``` text
EXE
→ Main Menu
→ Settings / Back
→ PLAY
→ Loading
→ Match 2v2
→ Result
→ Play Again
→ Result
→ Main Menu
→ Quit
```

Non usare `-RTScenario` come sostituto del frontend/loading/result
quando questi sono sotto test.

PASS:

``` text
0 missing required asset
0 Ensure
0 Check
0 Fatal
board visible
hero animations present
match completes
result valid
play again valid
main menu valid
quit clean
```

## 11. Issue fuori critical path

Non spostare automaticamente nella v0.1:

``` text
#314
#319
#728
#729
```

e, salvo necessità del Golden Scenario:

``` text
#327
#773
#774
#775
#776
#777
#778
```

Networking, dedicated server, GAS completo, modding pubblico e tuning
avanzato non sono prerequisiti del baseline v0.1 salvo nuova decisione
esplicita.

## 12. Regola per nuove issue

Non creare issue per CP-A/CP-B/CP-C se un owner esistente è sufficiente.

Creare una nuova issue solo se l'audit LIVE dimostra lavoro non
posseduto.

Ogni nuova issue deve avere:

``` text
Why / measured problem
Scope
Out of scope
Dependencies
Definition of Done falsificabile
Automation test
PIE test
Packaged test se necessario
Privacy impact
Determinism/replay impact
Epic
Checkpoint
Evidence path
```

## 13. Dependency graph target

``` text
D0
 |
 +---------------------+
 |                     |
 v                     v
#1957               #1922
 |                     |
 +----------+----------+
            |
           #1800
            |
           CP-A
            |
            +-----------------------------+
            |                             |
            v                             v
       PERCEPTION                   PRESENTATION
   #686 + #690                      #1801
         |                              |
        #159                           #1945
         |                              |
        #160                       #219 / #637
                                       |
                                      #220
                                       |
                                  #77 / #613
                                       |
                                      #705
                                       |
                                      #291
            \                           /
             +--------- CP-B -----------+
                         |
                         v
                        #75
                         |
                        #170
                         |
                        CP-C
                         |
                    #1663 + #1665
                         |
                        G13
                         |
                        v0.1
```

Questo è il target concettuale. Prima di scriverlo nel graph reale,
confrontare ogni edge con issue/roadmap/Feature Registry LIVE.

## 14. Policy Editor

``` text
E1 = authoring/assets/graybox
G1 = clean validation, Editor chiuso
E2 = CP-B + CP-C PIE
G13 = packaged, Editor chiuso
```

Non aprire una nuova sessione per singola issue.

## 15. Report finale richiesto a Claudia

``` text
BASE COMMIT:
CURRENT BRANCH:
WORKTREE:

LIVE ISSUE STATUS:
OPEN:
CLOSED:
SUPERSEDED:

ISSUES UPDATED:
-

ISSUES CREATED:
-

ISSUES NOT TOUCHED:
-

DEPENDENCY GRAPH CHANGES:
-

ROADMAP CHANGES:
-

SCENARIO MAP CHANGES:
-

CP-A:
  prerequisites:
  PASS/FAIL:
  evidence:

CP-B:
  prerequisites:
  PASS/FAIL:
  evidence:

CP-C:
  prerequisites:
  PASS/FAIL:
  evidence:

G13:
  prerequisites:
  PASS/FAIL:
  evidence:

EDITOR SESSIONS:
E1:
G1:
E2:
G13:

BLOCKERS:
-

NEXT EXECUTABLE ISSUE:
-

NEXT EXECUTABLE CHECKPOINT:
-
```

## 16. Regola conclusiva

Ottimizzare per:

``` text
un solo dependency graph
+
un owner per ogni lavoro
+
zero duplicate issue
+
zero knowledge leak
+
zero determinism divergence
+
CP-A verificabile
+
CP-B verificabile
+
CP-C verificabile
+
G13 riproducibile
+
minimo numero di aperture Editor
```

**Prima misura lo stato LIVE, poi modifica.**
