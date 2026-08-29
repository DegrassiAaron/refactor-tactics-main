# RefactorTactics — Claude Handoff
## Tactical Designer DevSandbox Launcher Roadmap

**Data:** 2026-08-29  
**Repository:** `DegrassiAaron/refactor-tactics-main`  
**Epic owner:** GitHub #1105 — Tactical Designer  
**Scope:** Editor-only / out_of_release_scope, salvo dipendenze runtime canoniche già esistenti.

---

# 0. Obiettivo

Implementare il nuovo ingresso operativo del **Tactical Designer**.

Quando Unreal Editor apre `L_DevSandbox`, il Technical/Tactical Designer deve diventare il punto d'ingresso della sessione di lavoro.

Il designer deve poter scegliere:

1. **Mappa**
2. **Formato** (`1v1`, `2v2`, `3v3`, `Custom`)
3. **Scenario**
4. avviare una **Tactical Designer Session**
5. entrare nel workspace esistente e usare:
   - Map Editor
   - Scenario Composer
   - Scenario Harness
   - playback
   - TurnLog
   - validation
   - State Diff

Il launcher **NON deve eseguire automaticamente la partita**.

Flusso target:

```text
Unreal Editor
    │
    ▼
L_DevSandbox
    │
    ▼
Tactical Designer Launcher
    │
    ├── Map
    ├── Format
    ├── Scenario
    └── Start Session
            │
            ▼
Tactical Designer Workspace
            │
            ├── Map Editor
            ├── Scenario Composer
            ├── Validation
            ├── Scenario Harness
            ├── Playback
            ├── TurnLog
            └── State Diff
```

---

# 1. Vincoli architetturali NON negoziabili

Leggere prima:

- `docs/technical/tooling/spec-tactical-designer.md`
- ADR-0010 — esposizione Blueprint / Scenario Harness
- ADR-0009 — replay logico canonico
- epic GitHub `#1105`
- issue `#1186`
- issue `#1625`–`#1630`

## 1.1 Tactical Designer non è una nuova authority

Il Tactical Designer è un workflow/editor surface.

NON creare un secondo simulatore.

NON duplicare:

- pathfinding
- validation
- targeting
- LOS
- resolver
- scenario execution
- TurnLog
- replay logic

La catena resta:

```text
Canonical Data + Runtime Rules
        │
        ├── Resolver
        ├── Scenario Harness
        ├── TurnLog / Replay
        └── Pure Query / DTO
                    │
                    ▼
              Editor UI
```

## 1.2 NON creare `URTTacticalDesignerSubsystem`

La spec corrente dichiara esplicitamente che **Tactical Designer è il nome del workflow, non di un modulo o subsystem**.

Se serve un entry point editor, usare un oggetto editor-specifico piccolo e nominato per il suo compito reale, ad esempio:

```text
RTDevSandboxLauncher
RTDevSandboxSession
RTDevSandboxBootstrap
```

Nome definitivo da scegliere solo dopo aver verificato le convenzioni del repository.

## 1.3 Scenario authoring

Passare dalla porta canonica già esistente:

```text
URTScenarioAuthoring
```

NON permettere alla UI di manipolare direttamente il modello scenario interno se ADR-0010 non lo espone.

## 1.4 Map metadata

Riutilizzare il lavoro / contratto di `#1186`.

NON creare un secondo metodo per calcolare:

- numero celle
- layer
- active layer
- asset associato

La UI deve consumare la stessa risposta canonica.

## 1.5 Scenario discovery

Usare il sistema di Scenario Index / discovery già esistente.

NON implementare una scansione parallela di directory se il repository ha già un indice canonico.

---

# 2. Issue tree da creare

Creare una nuova sub-issue principale sotto `#1105`.

Titolo consigliato:

```text
Tactical Designer — DevSandbox Launcher e session bootstrap
```

Questa è l'issue parent della roadmap seguente.

---

## TD-L0 — Contract e bootstrap UX

**Titolo suggerito**

```text
Tactical Designer Launcher — definire contract di L_DevSandbox e session bootstrap
```

### Scope

Formalizzare che:

```text
L_DevSandbox
```

è il punto d'ingresso del workflow Tactical Designer.

Definire:

- comportamento all'apertura
- cosa viene mostrato
- differenza fra `Start Session` e `Run`
- comportamento con asset/scenario mancanti
- comportamento su mappe diverse da `L_DevSandbox`
- stato locale/per-user

### Acceptance Criteria

- `L_DevSandbox` è documentata come bootstrap environment.
- Aprirla NON avvia automaticamente uno scenario.
- `Start Session` e `Run` sono concetti distinti.
- nessuna logica gameplay entra nell'Editor.
- nessuna nuova authority.
- nessun dirty del level per semplice uso del launcher.

### Dipendenze

- `#1105`
- ADR-0010
- spec Tactical Designer

---

## TD-L1 — Auto-open launcher

**Titolo suggerito**

```text
DevSandbox — aprire automaticamente il Tactical Designer Launcher
```

### Scope

Quando `L_DevSandbox` viene aperta nell'Editor:

```text
→ mostrare Tactical Designer Launcher
```

Quando si apre una normale gameplay map:

```text
→ NON mostrare automaticamente il launcher
```

### DoD

- apertura deterministica
- nessun polling
- nessun Tick necessario per detect della mappa
- launcher dockabile o integrato secondo le convenzioni editor esistenti
- nessun GameMode/HUD runtime coinvolto
- `L_DevSandbox.umap` non viene modificata

### Test

Creare test equivalente a:

```text
RefactorTactics.Editor.DevSandboxLauncher.OpensOnDevSandbox
RefactorTactics.Editor.DevSandboxLauncher.DoesNotOpenOnNormalMaps
RefactorTactics.Editor.DevSandboxLauncher.DoesNotDirtyDevSandboxMap
```

Nomi esatti devono seguire naming esistente.

---

## TD-L2 — Map Selector

**Titolo suggerito**

```text
Tactical Designer Launcher — selezione mappa e metadata canonici
```

### UI minima

```text
MAP
[ DA_HexMap_Arena ▼ ]

Asset: ...
Cells: ...
Layers: ...
Active Layer: ...
Validation: ...
```

### Vincoli

Riutilizzare la query/runtime data usata da `#1186`.

NON duplicare `GetLayers()` o `Cells.Num()` in logica alternativa.

### DoD

- elenco mappe disponibili
- selezione mappa
- metadata aggiornati senza polling temporale
- stato esplicito quando non c'è MapAsset
- zero nuova logica di conteggio

### Test

```text
RefactorTactics.Editor.DevSandboxLauncher.MapMetadataUsesCanonicalQuery
RefactorTactics.Editor.DevSandboxLauncher.MapSelectionUpdatesMetadata
RefactorTactics.Editor.DevSandboxLauncher.MissingMapAssetIsExplicit
```

### Dipendenze

- `#1186`

---

## TD-L3 — Match Format Filter

**Titolo suggerito**

```text
Tactical Designer Launcher — filtro formato 1v1 / 2v2 / 3v3 / Custom
```

### Formati UI iniziali

```text
1v1
2v2
3v3
Custom
```

### Regola importante

Per questa slice il `Format` deve essere trattato come:

```text
UI filter / authoring preset
```

NON introdurre automaticamente una nuova enum runtime se non esiste un reale owner canonico del concetto.

Derivare quando possibile il formato da:

```text
Team A unit count
Team B unit count
```

### DoD

- il filtro non modifica le regole del resolver
- il filtro non altera scenario esistente
- `Custom` gestisce scenari non riconducibili a 1v1/2v2/3v3
- 2v2 è facilmente selezionabile per Vertical Slice
- 3v3 è facilmente selezionabile per main format

### Test

```text
RefactorTactics.Editor.DevSandboxLauncher.FormatFiltersScenarioList
RefactorTactics.Editor.DevSandboxLauncher.FormatDoesNotMutateScenario
```

---

## TD-L4 — Scenario Browser

**Titolo suggerito**

```text
Tactical Designer Launcher — Scenario Browser filtrato per mappa e formato
```

### UI minima

```text
SCENARIO

[ Search.................... ]

Compatible
- Scenario A
- Scenario B
- Scenario C

Other
- Scenario D
```

### Filtri MVP

- map
- format
- search string

Posticipare:

- character
- system
- tags avanzati
- regression classification
- capability matrix

a meno che l'indice esistente li renda praticamente gratuiti.

### Vincoli

Usare Scenario Index esistente.

NON fare un secondo catalogo.

### Test

```text
RefactorTactics.Editor.DevSandboxLauncher.FiltersScenariosByMap
RefactorTactics.Editor.DevSandboxLauncher.FiltersScenariosByFormat
RefactorTactics.Editor.DevSandboxLauncher.SearchFiltersScenarioList
```

---

## TD-L5 — Session Bootstrap

**Titolo suggerito**

```text
Tactical Designer Launcher — bootstrap di scenario esistente e nuovo scenario
```

Gestire due percorsi.

### Existing Scenario

```text
Select Scenario
    ↓
Load canonical scenario
    ↓
URTScenarioAuthoring
    ↓
Validate
    ↓
Start Session
```

### New Scenario

```text
Map
+
Format
+
New Scenario
    ↓
Create FRTScenarioDraft
    ↓
URTScenarioAuthoring
    ↓
Scenario Composer
```

### Regola fondamentale

Quando viene selezionato uno scenario esistente:

```text
lo scenario è source of truth
```

Map e Format devono essere:

- derivati
- verificati
- mostrati

NON devono sovrascrivere silenziosamente lo scenario.

### Test

```text
RefactorTactics.Editor.DevSandboxLauncher.ExistingScenarioIsNotSilentlyOverridden
RefactorTactics.Editor.DevSandboxLauncher.NewScenarioUsesAuthoringFacade
RefactorTactics.Editor.DevSandboxLauncher.InvalidScenarioCannotStartSession
```

---

## TD-L6 — Tactical Designer Workspace entry

**Titolo suggerito**

```text
Tactical Designer — entrare nel workspace dalla DevSandbox Session
```

### Scope

Dopo `Start Session`, il designer deve avere accesso coerente alle superfici esistenti:

```text
Map
Scenario
Validation
Playback
TurnLog
```

Non è necessario creare una mega-UI nuova.

Riutilizzare tool e mode esistenti.

### Obiettivo

Ridurre questo:

```text
open Unreal
→ find tool
→ find map
→ find scenario
→ configure
→ test
```

a:

```text
open Unreal
→ L_DevSandbox
→ choose Map
→ choose Format
→ choose Scenario
→ Start Session
```

---

## TD-L7 — Run integration

**Titolo suggerito**

```text
Tactical Designer Session — Run tramite Scenario Harness canonico
```

### Flusso

```text
Start Session
    ↓
Edit
    ↓
Run
    ↓
Scenario Harness
    ↓
TurnLog
    ↓
Playback
    ↓
Result / State Diff
```

### Dipendenze già esistenti

- `#1625` — visual playback
- `#1626` — combat intent authoring
- `#1627` — multi-turn
- `#1628` — FIRE/HOLD
- `#1629` — initial status
- `#1630` — State Diff

Questa issue NON deve duplicare quel lavoro.

Deve solo collegare il launcher/session flow a tali superfici.

### Test

Il risultato deve continuare a rispettare il guardiano già esistente:

```text
RefactorTactics.Scenario.RunFromTheEditorMatchesTheHeadlessRun
```

Non crearne una seconda versione con semantica diversa.

---

## TD-L8 — Session persistence

**Titolo suggerito**

```text
Tactical Designer Launcher — ricordare l'ultima sessione per-user
```

### UX

```text
Last Session

Map: ...
Format: ...
Scenario: ...

[ Resume ]
[ New Session ]
```

### Vincoli

Salvare come:

```text
Editor per-user state
```

NON in:

```text
L_DevSandbox.umap
scenario canonical data
production assets
```

### DoD

- chiudere/riaprire Editor permette Resume
- stato corrotto/mancante degrada a New Session
- nessun binary asset dirty
- nessun dato locale entra in source control

### Test

```text
RefactorTactics.Editor.DevSandboxLauncher.RestoresLastSession
RefactorTactics.Editor.DevSandboxLauncher.InvalidSavedSessionFallsBackSafely
```

---

# 3. Dipendenze GitHub esistenti

Collegare la nuova parent issue almeno a:

```text
#1105  Tactical Designer epic
#1186  map metadata/readout
#1625  visual playback
#1626  combat intent authoring
#1627  multi-turn
#1628  FIRE/HOLD
#1629  initial statuses
#1630  State Diff
```

Riferimenti di capability già consegnate:

```text
#1114
#1115
#1116
#1117
```

Scenario Composer Lite esiste già.

NON riaprire o duplicare quel lavoro.

---

# 4. Ordine di implementazione consigliato

Implementare in questo ordine:

```text
L0
↓
L1
↓
L2
↓
L3
↓
L4
↓
L5
↓
L6
↓
L7
↓
L8
```

Parallelizzazione ammessa:

```text
L3 + L4
```

solo quando il contract del formato e lo Scenario Index sono stati misurati.

`L7` dipende dal lavoro Trial esistente ma deve poter essere sviluppato incrementalmente.

---

# 5. Prima di modificare codice

Claude deve misurare HEAD.

Eseguire ricerche equivalenti a:

```bash
git status
git branch --show-current

rg "L_DevSandbox" .
rg "URTScenarioAuthoring" Source docs
rg "ScenarioIndex|Scenario Index" Source docs
rg "URTHexEditorMode" Source
rg "GetLayers" Source
rg "SetModeSettingsObject" Source
rg "EditorSubsystem" Source/RefactorTacticsEditor
rg "LevelEditor" Source/RefactorTacticsEditor
```

NON assumere path/classi da questo documento se HEAD dice altro.

Riportare ogni divergenza.

---

# 6. File probabili

Verificare prima di creare qualsiasi file.

Aree probabili:

```text
Source/RefactorTacticsEditor/
Source/RefactorTactics/
docs/technical/tooling/
docs/roadmap/
```

Il launcher appartiene al modulo Editor.

La logica pura/canonica appartiene al runtime solo quando rappresenta una risposta che deve essere condivisa con runtime/headless.

---

# 7. UI proposta

MVP:

```text
┌──────────────────────────────────────────┐
│ RefactorTactics — Tactical Designer      │
├──────────────────────────────────────────┤
│ Map                                      │
│ [ DA_HexMap_Arena                 ▼ ]    │
│ Cells: 64     Layers: 2                  │
│                                          │
│ Format                                   │
│ [ 2v2                             ▼ ]    │
│                                          │
│ Scenario                                 │
│ [ Search............................. ]   │
│ [ RT_SCN_WaterElectricity_01       ▼ ]   │
│                                          │
│ Validation                               │
│ ✓ Map compatible                         │
│ ✓ 2 Team A / 2 Team B                    │
│ ✓ Scenario valid                         │
│                                          │
│ [ Edit Map ] [ Edit Scenario ]           │
│                                          │
│              [ START SESSION ]           │
└──────────────────────────────────────────┘
```

Dopo Start Session:

```text
Map | Scenario | Validation | Playback | TurnLog
```

Non creare design finale.

Graybox/editor-native è sufficiente.

---

# 8. Failure states da rendere espliciti

Gestire almeno:

```text
No Map Asset
No Scenario
Scenario missing
Scenario parse error
Scenario incompatible with selected map
Unknown/custom format
Invalid team size
Missing required capability
Map changed after scenario load
Stale per-user session
Scenario validation failed
```

Mai:

```text
silent fallback
silent mutation
implicit conversion
```

---

# 9. Logging / Observability

Aggiungere log strutturati per:

```text
Launcher opened
Map selected
Format selected
Scenario selected
Session started
Scenario validation failed
Saved session restored
Saved session rejected
```

Non loggare ogni frame.

Usare category già esistente quando semanticamente corretta; altrimenti crearne una sola dedicata al bootstrap editor.

---

# 10. Automation Tests

Minimo:

```text
OpensOnDevSandbox
DoesNotOpenOnNormalMaps
DoesNotDirtyDevSandboxMap

MapMetadataUsesCanonicalQuery
MapSelectionUpdatesMetadata

FormatFiltersScenarioList
FormatDoesNotMutateScenario

FiltersScenariosByMap
FiltersScenariosByFormat

ExistingScenarioIsNotSilentlyOverridden
NewScenarioUsesAuthoringFacade

InvalidScenarioCannotStartSession

RestoresLastSession
InvalidSavedSessionFallsBackSafely
```

Rispettare naming e test conventions già esistenti.

Il modulo Editor ha già Automation Tests: NON dichiarare che i test editor sono impossibili.

---

# 11. Verifica manuale

Aggiungere una sessione manuale/PIE/editor nel registro appropriato.

Smoke flow:

```text
1. aprire Unreal Editor
2. aprire L_DevSandbox
3. verificare auto-open del launcher
4. selezionare mappa
5. verificare celle/layer
6. selezionare 2v2
7. selezionare scenario compatibile
8. Start Session
9. modificare scenario
10. Run
11. vedere risultato
12. leggere TurnLog
13. playback
14. Reset
15. modificare
16. Run di nuovo
17. chiudere Editor
18. riaprire
19. Resume
20. verificare che L_DevSandbox non sia dirty
```

---

# 12. Git discipline

Una issue = una slice verificabile.

Commit suggeriti:

```text
feat(editor): add DevSandbox Tactical Designer launcher
feat(editor): add canonical map selection to DevSandbox launcher
feat(editor): filter Tactical Designer scenarios by session format
feat(editor): add Tactical Designer scenario browser
feat(editor): bootstrap Tactical Designer authoring session
feat(editor): integrate DevSandbox session with Scenario Harness
feat(editor): persist Tactical Designer session per user
test(editor): cover DevSandbox launcher workflow
```

Ogni commit deve compilare.

---

# 13. Non-goals

NON implementare in questa roadmap:

- Skill Workbench completo
- visual ability scripting
- mass simulation
- bot tournament
- production promotion
- public modding
- multiplayer editor tooling
- matchmaking
- progression
- final art UI
- replay player-facing
- nuovo gameplay resolver
- nuovo scenario format parallelo
- nuovo map format parallelo

---

# 14. Definition of Done complessiva

La roadmap è completa quando un technical designer può:

```text
Open Unreal
→ L_DevSandbox
→ launcher opens automatically
→ choose Map
→ choose Format
→ choose/create Scenario
→ Start Session
→ edit
→ validate
→ Run through canonical Scenario Harness
→ inspect playback
→ inspect TurnLog
→ inspect result/state diff
→ Reset
→ modify
→ Run again
→ close Editor
→ reopen
→ Resume
```

e contemporaneamente:

- nessun gameplay rule è duplicato nell'Editor
- `RunFromTheEditorMatchesTheHeadlessRun` resta verde
- `L_DevSandbox.umap` non viene sporcata dal launcher
- lo scenario esistente non viene modificato implicitamente
- i metadata mappa provengono dalla query canonica
- nessun nuovo leak di dati/runtime authority
- Automation Tests verdi
- verifica manuale registrata
- build Editor compilabile
- packaged/runtime build non include UI editor-only

---

# 15. Prima azione richiesta a Claude

1. Misurare HEAD.
2. Leggere `#1105`, `#1186`, `#1625`–`#1630`.
3. Verificare i documenti owner.
4. Identificare le API reali disponibili.
5. NON modificare codice ancora.
6. Proporre:
   - parent issue
   - sub-issue L0–L8
   - dipendenze reali
   - eventuali merge/split necessari rispetto a issue già esistenti.
7. Solo dopo la verifica, creare/aggiornare le issue.
8. Iniziare da L0/L1 con patch minima compilabile.

---

# 16. Regola finale

Se una scelta del launcher richiede di conoscere:

```text
"cosa è valido?"
"qual è il percorso?"
"quale scenario è eseguibile?"
"come finisce il turno?"
"quale stato è corretto?"
```

la risposta deve provenire dal runtime/canonical layer già proprietario di quella domanda.

Il launcher può decidere:

```text
cosa mostrare
cosa filtrare
quale sessione aprire
quale pannello attivare
```

Il launcher NON decide il gioco.
