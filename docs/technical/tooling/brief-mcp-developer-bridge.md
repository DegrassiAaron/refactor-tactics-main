# Brief — Unreal MCP Developer Bridge per RefactorTactics (PoC)

> `CURRENT` · **Stato**: **implementato il 2026-08-27** — l'esito misurato è in §15
> **Natura**: brief di implementazione. Dice *cosa costruire e su quali fatti*; lo stato corrente del bridge
> lo dicono il codice in `Plugins/RTDeveloperTools/` e i suoi test, non questo documento.
> ⚠️ **La §1 è una fotografia datata** — engine, plugin e porte cambiano. Riverificala prima di rimetterci
> mano: ogni voce porta il comando che l'ha prodotta apposta.

Lavori sul repository **RefactorTactics** (`D:\Repositories\refactor-tactics-main`), tactical game a turni
simultanei in Unreal Engine 5. Devi costruire un ponte **Claude Code ↔ Unreal Editor** via MCP, per il solo
sviluppo/debug. Nessuna AI e nessun Python nel runtime del gioco.

Prima di leggere il resto: `AGENTS.md` e `CLAUDE.md` del repository sono normativi e vincono su questo
documento in caso di conflitto.

---

## 0. Come leggere questo prompt

La sezione **§1 Fatti verificati** è stata misurata sul disco il **2026-08-27**. Non è memoria e non è
supposizione: ogni voce ha il comando che l'ha prodotta. Ma è una **fotografia datata** — engine, plugin e
branch cambiano. **Riverifica §1 prima di scrivere codice** e, se una voce non regge più, fermati e dillo
invece di adattare il codice a una premessa morta.

Il resto del prompt è costruito *sopra* §1. Se §1 cade, cade anche ciò che ne discende.

---

## 1. Fatti verificati (riverificali)

### 1.1 Engine

| Fatto | Comando di verifica |
|---|---|
| Engine usato: **UE 5.8.1** in `D:\EpicGames\UE_5.8` | `cat "/c/ProgramData/Epic/UnrealEngineLauncher/LauncherInstalled.dat"` → `"AppVersion": "5.8.1-56057345+++UE5+Release-5.8-Windows"` |
| `.uproject` dichiara `"EngineAssociation": "5.8"` | `cat RefactorTactics.uproject` |
| ⚠️ Il registry `HKLM:\SOFTWARE\EpicGames\Unreal Engine` elenca **solo 5.4** (`D:\EpicGames\UE_5.4`) | La 5.8 è registrata dal Launcher, non da HKLM. **Non usare la 5.4.** |

La baseline UE 5.8.x **regge**: nessuna migrazione di versione è necessaria né consentita.

### 1.2 Il plugin MCP esiste ed è ufficiale Epic

`D:\EpicGames\UE_5.8\Engine\Plugins\Experimental\ModelContextProtocol\ModelContextProtocol.uplugin`

```
FriendlyName:   "Unreal MCP"
Description:    "Anthropic MCP (Model Context Protocol) server implementation for Unreal Engine."
IsExperimentalVersion: true    NoRedist: true    EnabledByDefault: false
Moduli:  ModelContextProtocol (Runtime) · ModelContextProtocolEngine (Runtime)
         ModelContextProtocolEditor (Editor) · +3 moduli di test
Plugin richiesti:  EngineAssetDefinitions · ToolsetRegistry
```

**Non devi scrivere un server MCP.** Esiste, è di Epic, si abilita.

### 1.3 La via corrente per esporre tool è `UToolsetDefinition`, non Python

Header letto: `ModelContextProtocolEngine/Public/ModelContextProtocolToolLibrary.h`

```
UModelContextProtocolToolLibrary ... meta=(DeprecatedNode,
  DeprecationMessage="Use UToolsetDefinition (ToolsetRegistry plugin) instead.")
```

La via viva è il plugin **`ToolsetRegistry`**:
`Engine/Plugins/Experimental/ToolsetRegistry/Source/ToolsetRegistry/Public/ToolsetRegistry/ToolsetDefinition.h`

```cpp
// UFUNCTions che definiscono tool devono essere STATIC e marcate meta=(AICallable).
// UFUNCTions da ignorare vanno marcate meta=(AIIgnore) per silenziare gli errori.
UCLASS(BlueprintType, Abstract, MinimalAPI)
class UToolsetDefinition : public UObject { ... };
```

Registrazione (letta da `Toolsets/GameplayTagsToolset/.../Module.cpp`):

```cpp
#include "ToolsetRegistry/UToolsetRegistry.h"
virtual void StartupModule()  override { UToolsetRegistry::RegisterToolsetClass(UMyToolset::StaticClass()); }
virtual void ShutdownModule() override { UToolsetRegistry::UnregisterToolsetClass(UMyToolset::StaticClass()); }
```

**Conseguenza sull'architettura**: il livello Python previsto dal piano originale **non serve e va rimosso dal
disegno**. Il percorso è `Claude Code → server MCP dell'engine → UToolsetDefinition (C++) → API RefactorTactics`.
`PythonScriptPlugin` esiste nell'engine ma non è sul percorso richiesto; non abilitarlo.

Documenta questa scelta nel report: è una deviazione consapevole dal piano iniziale, motivata da un
`DeprecationMessage` nell'header dell'engine.

### 1.4 Due esempi da leggere prima di scrivere

Sono il tuo modello di riferimento. Leggili davvero:

- **`Engine/Plugins/Experimental/Toolsets/GameplayTagsToolset/`** — il più semplice: tool sincroni, ritorno di
  un `USTRUCT(BlueprintType)` (`FGameplayTagInfo`), doc-comment `///` e `/** @param */` che diventano
  descrizione del tool e dei parametri. Il `Module.cpp` è il template della registrazione.
- **`Engine/Plugins/Experimental/Toolsets/AutomationTestToolset/`** — `.uplugin` con `"EditorOnly": true`,
  `Build.cs`, tool async (`UToolCallAsyncResultString*`).

I tipi di ritorno visti nei toolset dell'engine: `USTRUCT(BlueprintType)`, `FString`, `TArray<FString>`,
`bool`, `UToolCallAsyncResult*`. **Preferisci un `USTRUCT`**: lo schema JSON di output viene generato dalla
reflection, non a mano.

### 1.5 Configurazione client e settings del server

`ModelContextProtocolEngine/Public/ModelContextProtocolSettings.h` — `UModelContextProtocolSettings`,
`config = EditorPerProjectUserSettings`, DisplayName **"Model Context Protocol"**:

| Proprietà | Default | Nota |
|---|---|---|
| `ServerUrlPath` | `/mcp` | |
| `ServerPortNumber` | `8000` | |
| `bAutoStartServer` | **`false`** | va acceso, o il server non parte |
| `bEnableToolSearch` | **`true`** | 🔴 vedi §9.1: cambia cosa vede Claude Code |

`ModelContextProtocolEngine/Public/ModelContextProtocolClientConfig.h`:

```cpp
enum class EModelContextProtocolClient : uint8 { ClaudeCode, Cursor, VSCode, Gemini, Codex };
// ClaudeCode → `.mcp.json` nella root del progetto
MODELCONTEXTPROTOCOLENGINE_API bool WriteClientConfiguration(
    EModelContextProtocolClient Client, uint32 Port, const FString& UrlPath,
    const FString& BaseDirectory = FString());
```

Il doc-comment dichiara che i formati JSON sono *"created or updated, preserving existing entries"*, e che
`BaseDirectory` vuoto risolve a `FPaths::RootDir()` nelle source build o `FPaths::ProjectDir()` nelle installed
build. Il nostro è un engine da Launcher (installed), quindi atteso `ProjectDir` — **verificalo, non darlo per
scontato**. Usa questa funzione invece di scrivere `.mcp.json` a mano. `.mcp.json` **non esiste ancora** nel
repository (`cat .mcp.json` → assente) e non è in `.gitignore`: decidi e dichiara se versionarlo.

### 1.6 Il repository

```
Source/RefactorTactics/         modulo Runtime  (gameplay autorevole)
Source/RefactorTacticsEditor/   modulo Editor   (Editor Mode hex, UEdMode + Interactive Tools)
Plugins/                        NON ESISTE
```

Esiste quindi un **modulo** Editor-only, non un **plugin** Editor. Vedi §3 per la scelta di collocazione.

### 1.7 API RefactorTactics che devi riusare (firme reali)

**Cella** — `Source/RefactorTactics/Map/RTCellId.h`

```cpp
USTRUCT(BlueprintType)
struct FRTCellId { int32 X = 0; int32 Y = 0; int32 Layer = 0; ... };
```

X = **q**, Y = **r** (coordinate **assiali**, hex pointy-top), terza cubica derivata `CubeZ() = -X-Y`.
Celle su layer diversi **non sono adiacenti**: servono archi espliciti.

🔴 **Trappola**: `FRTCellId::IsValid()` verifica la coerenza cubica `q+r+s==0` — è vera per costruzione e **non
dice nulla sull'esistenza della cella nella mappa**. Per "questa cella esiste" usa `URTHexMapAsset::ContainsCell`
/ `FindCell`. Un tool che risponde `valid: true` chiamando `IsValid()` risponderebbe `true` sempre.

**Mappa** — `Source/RefactorTactics/Map/RTHexMapAsset.h` (`URTHexMapAsset`)

```cpp
const FRTHexCellData* FindCell(const FRTCellId& Id) const;
bool                  ContainsCell(const FRTCellId& Id) const;
int32                 NumCells() const;
TArray<int32>         GetLayers() const;
TArray<FRTCellId>     CellsInLayer(int32 Layer) const;
TArray<FString>       ValidateMap() const;     // ← il validator ESISTE GIÀ
uint32                ComputeHash() const;
FRTCellId             GetCenterCell() const;
int32                 Revision;                // sale a ogni modifica strutturale
static constexpr int32 CurrentFormatVersion = 10;
```

I campi della cella stanno in `FRTHexCellData` (`Map/RTHexCellData.h`): **leggili e mappa quelli reali**. Non
inventare un campo `standable` se il dato si chiama diversamente.

**Actor di mappa** — `Source/RefactorTactics/Map/RTHexMapActor.h`

```cpp
static ARTHexMapActor* FindInWorld(const UWorld* World);
const URTHexMapAsset*  GetHexContext(FVector& OutOrigin, float& OutHexSize, float& OutLayerHeight) const;
const TArray<FRTCellId>& GetUnreachableCells() const;
```

In Editor il world è quello dell'editor (`GEditor->GetEditorWorldContext().World()` — verifica l'API sulla 5.8).

**Pathfinding** — `Source/RefactorTactics/Pathfinding/RTHexPathLibrary.h` + `RTHexPath.h`

```cpp
static FRTHexPathResult URTHexPathLibrary::FindPath(const URTHexMapAsset* Map,
    const FRTCellId& Start, const FRTCellId& Goal, int32 MaxCost = 0, int32 MaxNodes = 100000);

static TArray<TPair<FRTCellId,int32>> GraphNeighbors(const URTHexMapAsset* Map, const FRTCellId& Cell);

struct FRTHexPathResult { ERTHexPathStatus Status; TArray<FRTCellId> Path; int32 TotalCost; int32 NodesVisited; };
enum class ERTHexPathStatus : uint8 { Success, NoPath, StartInvalid, GoalInvalid, NodeLimit };
```

A* deterministico, **costi interi**, tie-break sull'ID cella, nessuna dipendenza dall'ordine di `TMap`/`TSet`.
`NodesVisited` copre già il requisito "nodi espansi". **Non scrivere un A*.** Fai da facade e basta.

🔴 `ERTHexPathStatus` **è già il modello d'errore del pathfinding**: non inventarne un secondo, mappalo.

**Test** — 1218 macro automation nel repo (`git grep -c "IMPLEMENT_SIMPLE_AUTOMATION_TEST\|IMPLEMENT_COMPLEX_AUTOMATION_TEST" -- Source`).
Convenzione reale del nome (da `Tests/RTHexPathTests.cpp`):

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexPathSimpleTest,
    "RefactorTactics.HexPath.SimplePath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
```

Prefisso **`RefactorTactics.`**, non `RT.`.

### 1.8 Cosa NON è disponibile per un PoC Editor-only

- **TurnLog**: `FRTTurnLogEntry` e `URTTurnLogLibrary` esistono, ma il log vive nel `URTTurnManager` durante
  una partita (PIE), non nell'editor world. `rt_get_turn_log` è **fuori scope**.
- **Automation test via MCP**: `Engine/Plugins/Experimental/Toolsets/AutomationTestToolset` di Epic espone già
  `DiscoverTests` / `ListTests` / `RunTestsByFilter` / `GetTestResults`. **Non riscriverlo**: se serve, si
  abilita quel plugin. `rt_run_automation_tests` è fuori scope per duplicazione.

---

## 2. Conflitti col piano originale — già risolti, non riaprirli

| Piano iniziale | Realtà misurata | Decisione |
|---|---|---|
| "Python adapter sottile" | `UModelContextProtocolToolLibrary` deprecato → `UToolsetDefinition` in C++ | **Niente Python.** Tool in C++. |
| `URTDeveloperSubsystem` come boundary | I tool sono `static UFUNCTION(meta=(AICallable))` su una `UToolsetDefinition` | Classe tool = `URTDevToolset : public UToolsetDefinition`. Un subsystem è opzionale e solo se serve stato. |
| Nome test `RT.DeveloperTools.CellLookup` | Convenzione repo = `RefactorTactics.<Area>.<Caso>` | `RefactorTactics.DevToolset.<Caso>` |
| `standable`, `movementCost` come campi certi | Da leggere in `FRTHexCellData` | Mappa i campi **reali** |
| `rt_validate_tactical_map` da costruire | `URTHexMapAsset::ValidateMap()` esiste | Facade sopra il validator esistente |
| `rt_get_turn_log`, `rt_run_automation_tests` | Non disponibili / già coperti da Epic | Fuori scope |
| "verifica se il progetto è su 5.8" | Confermato 5.8.1 | Nessuna migrazione |

---

## 3. Dove va il codice — decisione da prendere e motivare

Due opzioni reali. Il piano originale diceva "se esiste già un plugin Editor, estendilo": **esiste un modulo
Editor (`RefactorTacticsEditor`), non un plugin**. Quindi la scelta è aperta.

**A — Plugin nuovo `Plugins/RTDeveloperTools/`** (raccomandato)

```
Plugins/RTDeveloperTools/
├── RTDeveloperTools.uplugin        "EditorOnly": true, EnabledByDefault false o true (decidi)
│                                   Plugins: ModelContextProtocol, ToolsetRegistry
└── Source/RTDeveloperTools/
    ├── RTDeveloperTools.Build.cs
    ├── Public/RTDevToolset.h
    └── Private/RTDevToolset.cpp + RTDeveloperToolsModule.cpp
```

Pro: il bridge si accende e si spegne con un plugin solo; `ModelContextProtocol` è `NoRedist` +
Experimental e resta confinato lì; `RefactorTacticsEditor` non eredita dipendenze MCP.

**B — Estendere `Source/RefactorTacticsEditor/`**

Pro: zero nuove cartelle, il modulo è già `Type: Editor` e già dipende dal runtime.
Contro: lega l'Editor Mode hex — che serve a tutti — a un plugin sperimentale `NoRedist`.

Scegli **A** salvo che tu trovi un motivo migliore misurato sul repo. Motiva in una riga nel report.

In entrambi i casi vale: `TargetAllowList: ["Editor"]`, `LoadingPhase` allineata all'esempio Epic
(`PostEngineInit` per i toolset), **nessuna dipendenza MCP nel modulo `RefactorTactics` runtime**, e ogni
dipendenza aggiunta in `Build.cs` deve essere giustificata (niente "per sicurezza").

---

## 4. Obiettivo del PoC

Percorso minimo, end-to-end:

```
Claude Code → server MCP (engine) → URTDevToolset (C++) → URTHexMapAsset / URTHexPathLibrary → risultato strutturato
```

Alla fine devono funzionare davvero, contro `L_DevSandbox`:

```
rt_project_status
rt_get_current_map
rt_dump_cell        x=4  y=7  layer=0
rt_find_path        (1,1,0) → (8,5,0)
rt_validate_tactical_map
```

Nient'altro. I nomi dei tool seguiranno la convenzione del `ToolsetRegistry` (toolset + funzione): verifica come
il registry compone il nome esposto e allineati, invece di forzare il prefisso `rt_` se il framework ne impone
un altro.

---

## 5. I cinque tool

Per ciascuno: input, output (`USTRUCT`), backend reale. Adatta i campi ai dati che esistono davvero.

**`ProjectStatus()`** — nessun input. Nome progetto, versione engine, se una mappa hex è caricata, nome
dell'asset mappa, `NumCells`, `Revision`, `CurrentFormatVersion`. Backend: `FApp` / `FPaths` /
`ARTHexMapActor::FindInWorld`.

**`GetCurrentMap()`** — nessun input. Path dell'asset, `NumCells`, `GetLayers()`, bounding delle celle se
ricavabile, `Revision`, `ComputeHash()`, `GetCenterCell()`. Backend: `URTHexMapAsset`.

**`DumpCell(int32 X, int32 Y, int32 Layer)`** — esistenza (`ContainsCell`), campi reali di `FRTHexCellData`,
`Revision`, vicini via `URTHexPathLibrary::GraphNeighbors` con il costo di ciascun arco.
Documenta nel doc-comment che X/Y sono **assiali (q, r)**, o chi chiama il tool userà coordinate cartesiane.

**`FindPath(FRTCellId Start, FRTCellId Goal, ...)`** — facade su `URTHexPathLibrary::FindPath`. Restituisci
`Status` (stringa dall'enum), `Path` ordinato, `TotalCost` intero, `NodesVisited`, `Revision` al momento della
query, e la durata misurata **del solo pathfinder** (§7). Espone `MaxCost`/`MaxNodes` con i default della firma.

**`ValidateTacticalMap()`** — facade su `URTHexMapAsset::ValidateMap()` + `GetUnreachableCells()`.

### 5.1 Modello di errore

Niente prosa come API. Niente eccezioni come controllo di flusso ordinario.

Per il pathfinding **usa `ERTHexPathStatus`**, non un modello parallelo. Per gli altri casi (nessuna mappa
caricata, cella inesistente) scegli **una** convenzione e applicala a tutti e cinque i tool. Prima di
scegliere, guarda come lo fanno i toolset Epic: `UGameplayTagsToolset::GetTagInfo` dichiara *"Raises a script
error if the tag does not exist"* — leggi `GameplayTagsToolset.cpp` e
`ToolsetRegistry/ToolCallExceptionHandler.h` per capire il meccanismo reale prima di decidere fra "script
error" e "campo di esito nella struct". Documenta la scelta.

---

## 6. Sicurezza — vincoli non negoziabili

```
Editor-only · localhost-only · development-only · nessuna dipendenza nel runtime packaged
```

Non esporre: `CanonicalIntentStore`, intenti della squadra avversaria (esiste
`Turn/RTIntentPrivacyLibrary.h` e i suoi test: il progetto tratta la privacy degli intenti come invariante),
token, credenziali, variabili d'ambiente, filesystem arbitrario, shell, socket arbitrari, API runtime
competitive.

Nessun Actor replicato per MCP. MCP non partecipa al networking di gioco.
Verifica che il server sia bindato a loopback e **dichiara nel report come l'hai verificato** (non "è
localhost per default": mostra dove l'hai letto o misurato).

Non implementare in questa milestone: python arbitrario, console command arbitrari, shell, scrittura asset
arbitraria, modifica Blueprint arbitraria, dump di intenti canonici, dump di pacchetti di rete, MCP remoto,
autenticazione MCP, resolver di turno via MCP, GAS inspection, editing di massa.

---

## 7. Logging e misura

Categoria dedicata (es. `LogRTDevTools`) se non ne esiste una adatta — cerca prima nel repo.
Per ogni chiamata: nome tool, esito, durata. Per `FindPath` anche start, goal, `TotalCost`, `NodesVisited`.

Niente spam per nodo visitato a livello normale: `Verbose`/`VeryVerbose`. Niente dati sensibili nei log.

Il target del progetto è **query di path con mediana < 2 ms**. Il round-trip MCP è un'altra cosa: **misura i
due tempi separatamente** e non attribuire al pathfinder la latenza del trasporto. Se non hai una misura, non
scrivere un numero.

---

## 8. Automation test

Almeno due, con la convenzione reale del repo:

```
RefactorTactics.DevToolset.CellLookup
RefactorTactics.DevToolset.PathQuery
```

Devono coprire: cella valida · cella inesistente · path valido · destinazione irraggiungibile o inesistente ·
**determinismo**.

Per il determinismo: stessa query ripetuta, stessa `Revision` del grafo ⇒ **stessa sequenza ordinata di celle**
e stesso `TotalCost`. Asserisci l'**ordine esatto** dei nodi, non solo che il path arrivi a destinazione.

⚠️ Un test di determinismo che confronta due chiamate a una funzione già deterministica può passare senza
provare nulla. Prima di dichiararlo verde, fai **una** verifica di mutazione: rompi deliberatamente l'ordine
(o il tie-break) nel tuo layer di facade, controlla che il test diventi **rosso**, ripristina. Riporta l'esito
di quella verifica nel report — non il fatto che il test passi.

I test devono girare sul boundary/facade C++, senza passare dal trasporto MCP.

---

## 9. Manual test — procedura riproducibile

1. Abilita i plugin necessari (`ModelContextProtocol`, `ToolsetRegistry`, + il tuo) — vedi §3.
2. Compila Game + Editor con la 5.8 (`D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat`, target
   `RefactorTacticsEditor Win64 Development` — verifica i nomi target reali in `Source/*.Target.cs`).
3. Avvia l'Editor e apri `L_DevSandbox`.
4. In *Project Settings → Model Context Protocol*: accendi `bAutoStartServer` (default `false`) o avvia il
   server a mano; annota porta e path effettivi.
5. Genera la configurazione con `WriteClientConfiguration(EModelContextProtocolClient::ClaudeCode, ...)`.
   Cerca prima se il modulo `ModelContextProtocolEditor` espone già un comando/menu che la invoca: se c'è,
   **usa quello**. Verifica che `.mcp.json` finisca nella root del progetto e che eventuali server già
   configurati siano preservati.
6. Avvia Claude Code nella root del progetto e verifica la discovery.
7. Esegui i cinque tool e confronta `DumpCell` / `FindPath` con ciò che la DevSandbox mostra a schermo.

### 9.1 🔴 La trappola numero uno

`bEnableToolSearch` è **`true`** di default. Con quel valore, `tools/list` espone **solo** `list_toolsets`,
`describe_toolset` e `call_tool`: i tool RT **non compaiono** come tool MCP nativi, e vanno scoperti e
invocati attraverso `call_tool`.

Quindi il criterio "il toolset RT compare fra i tool" è **falso per costruzione** con i default, e leggerlo
come un fallimento del tuo codice ti farebbe inseguire un difetto che non esiste. Verifica per la via giusta:
`list_toolsets` → `describe_toolset` → `call_tool`; oppure metti `bEnableToolSearch = false` per la
registrazione nativa. **Prova entrambe** e riporta quale hai usato per ciascun risultato.

---

## 10. Vincoli di lavoro sul repository

**Stato git al 2026-08-27** (riverificalo con `git status`, sarà cambiato):

```
branch: fix/1497-lastmoveroutes-porta-identita
modificati e NON committati:
  Content/RT/Maps/Dev/L_DevSandbox/L_DevSandbox.umap     ← BINARIO
  Content/RT/UI/Match/WBP_RT_TurnHeader.uasset           ← BINARIO
```

Questo elenco è cambiato **due volte nell'ora** in cui il brief è stato scritto: i file C++ che conteneva
sono stati committati sotto di esso, ed è comparso un secondo binario. Non fidartene, misuralo.

C'è **lavoro altrui non committato**, e sono **due binari**. Regole:

- **Non toccare `L_DevSandbox.umap` né gli altri `.uasset` modificati.** I binari sono human-first: si
  modificano solo su richiesta esplicita e solo dentro Unreal. Due `.uasset` non si fondono, quindi un
  binario si modifica da un lavoro solo per volta.
- Non committare, stashare, resettare o riportare al branch le modifiche che non hai fatto tu.
- **Mai** `git reset --hard`, `git clean -fd/-fdx` (⚠️ `Content/FabAsset` contiene decine di GB di asset di
  terze parti), `git checkout --`, force push.
- **D-178, sviluppo sequenziale**: una sessione, una working directory, un branch. Niente worktree per
  parallelizzare. Se il branch corrente non è tuo, **chiedi** prima di cambiarlo: non decidere tu di spostare
  il lavoro di qualcun altro.
- Non committare/pushare senza richiesta esplicita.
- Il progetto **non ha CI** (`.github/workflows/` assente per scelta): i gate girano a mano, quindi
  nessuno ti dirà che hai rotto qualcosa. Misura tu.
- ⚠️ `scripts/` **non esiste più** (D-181/D-182, 2026-08-21): se trovi documenti o issue che rimandano a
  `scripts/*.py`, sono stantii — non ricostruirli. Il tooling vivo sta in `tools/radar/` (Node).
- Documentazione: `docs/` con la sottocartella pertinente (`docs/technical/` per un documento tecnico).
- Non rinominare classi fuori scope, non fare refactor opportunistici, non cancellare asset.

---

## 11. Ordine di lavoro

**A — Discovery.** Riverifica §1. Leggi `FRTHexCellData`, `GameplayTagsToolset`, `AutomationTestToolset`,
`ToolCallExceptionHandler.h`. Produci un preflight breve:
**Obiettivo · Stato verificato · Assunzioni · File · Approccio · Rischi · Test**.
Se una premessa di §1 è caduta, fermati qui e dillo.

**B — Il canale minimo.** Plugin/modulo + `URTDevToolset` con il solo `ProjectStatus`. Compila, avvia, verifica
end-to-end da Claude Code (attenzione a §9.1). **Non proseguire finché questo non risponde davvero.**

**C — Ispezione.** `GetCurrentMap` + `DumpCell`. Testa contro la DevSandbox.

**D — Pathfinding.** `FindPath` come facade pura su `URTHexPathLibrary`.

**E — Validazione.** `ValidateTacticalMap` sopra `ValidateMap()` + `GetUnreachableCells()`.

**F — Test.** Automation test (§8, inclusa la verifica di mutazione) + manual test (§9).

Procedi in autonomia: non chiedere conferma file per file. Davanti a un problema: analizzalo, scegli la
soluzione meno invasiva, implementala, documenta la decisione. Se una fase è bloccata, **completa tutte le
altre** e dichiara esplicitamente cosa hai lasciato fuori e perché.

---

## 12. Il boundary deve reggere le estensioni future (senza implementarle)

Progetta i tipi e la registrazione in modo che si possano aggiungere più tardi, senza rifare il boundary:
`SpawnDevFixture` / `ClearDevFixture`, `SubmitTestIntent`, `BuildSnapshot`, `ResolveTurn`, `GetTurnLog`,
`RunDeterminismTest`. **Non implementarne nessuno adesso.**

---

## 13. Definition of Done

Done solo se, tutto verificato e non asserito:

- il progetto compila (Game + Editor);
- l'Editor si avvia e apre `L_DevSandbox`;
- Claude Code raggiunge il server MCP;
- il toolset RT è raggiungibile (per la via di §9.1 che hai usato);
- i cinque tool rispondono con dati reali, non finti;
- `FindPath` passa dal pathfinder C++ esistente — zero A* riscritto;
- output strutturato, nessuna prosa come API;
- nessun gameplay duplicato fuori dal C++ autorevole; nessun Python;
- Editor-only, localhost-only, nessuna dipendenza MCP nel modulo runtime;
- gli automation test passano, e la verifica di mutazione è stata fatta;
- manual test documentato e riproducibile;
- nessun nuovo warning/errore rilevante;
- nessun file estraneo modificato, nessun binario toccato.

**Non dichiarare "funziona", "completo", "deterministico" o "production ready" senza l'evidenza accanto.**
Se non hai compilato, scrivi che non hai compilato.

---

## 14. Report finale — sezioni obbligatorie, in quest'ordine

1. **Environment detected** — versione Unreal · implementazione MCP · trasporto Claude Code · porta e path effettivi
2. **Existing systems reused** — classi e API RefactorTactics riusate
3. **Files created**
4. **Files modified**
5. **Architecture** — diagramma testuale finale (senza il livello Python)
6. **MCP tools implemented** — per ciascuno: nome esposto · input · output · backend C++
7. **Build result** — esito reale. Nessun PASS senza compilazione.
8. **Automated tests** — test → PASS/FAIL, **più l'esito della verifica di mutazione**
9. **Manual MCP test** — le chiamate realmente eseguite e cosa hanno restituito
10. **Problems encountered** — inclusi scostamenti di API/versione fra questo prompt e la 5.8.1 reale
11. **Security review** — Editor-only · localhost-only · nessuna dipendenza runtime · nessuna esposizione di
    intenti canonici, ciascuno con **come** l'hai verificato
12. **Git diff summary**
13. **Suggested commit** — `feat(devtools): add Unreal MCP bridge for tactical debugging`
14. **Next recommended step** — **uno solo**. Preferibilmente:
    `MCP deterministic movement scenario: fixture → intents → snapshot → resolution → TurnLog`

---

## 15. Esito — misurato il 2026-08-27

### 15.1 Cosa esiste

`Plugins/RTDeveloperTools/` — plugin `"EditorOnly": true`, modulo `Type: "Editor"` con
`TargetAllowList: ["Editor"]`, dipendente da `ToolsetRegistry` e `ModelContextProtocol`. Otto file:
`.uplugin`, `Build.cs`, `RTDevToolset.{h,cpp}`, `RTDeveloperToolsModule.cpp`, `RTDeveloperToolsLog.h`,
`Tests/RTDevToolsetTests.cpp`. Il modulo runtime `RefactorTactics` **non è stato toccato**.

Scelta **A** del §3 (plugin separato): `RefactorTacticsEditor` serve a tutti e non doveva ereditare una
dipendenza da un plugin `NoRedist` sperimentale.

Ogni tool è `static UFUNCTION(meta = (AICallable))` su `URTDevToolset : public UToolsetDefinition`, registrata
in `StartupModule` con `UToolsetRegistry::RegisterToolsetClass`. **Zero Python**, come previsto da §1.3.
I report sono `USTRUCT` — lo schema JSON esce dalla reflection, non da serializzazione a mano.

### 15.2 Build e test

| | esito |
|---|---|
| `Build.bat RefactorTacticsEditor Win64 Development` | `Result: Succeeded`, **0 warning**, 0 error |
| `RefactorTactics.DevToolset.*` (headless, `-nullrhi`) | **6 trovati, 6 eseguiti, 6 Success** |

⚠️ L'exit code non è un oracolo: la prima run è uscita **0 con un test fallito**. Leggi
`Found N automation tests` e i `Result={...}`, non il codice di ritorno.

**Verifica di mutazione** (invertire `Report.Path` nel facade), fatta e riportata perché il risultato
smentisce un'attesa: `PathQuery` è diventato rosso, **`PathDeterminism` è rimasto verde**. Un percorso
invertito è identico a sé stesso a ogni esecuzione e resta contiguo, quindi né il confronto run-contro-run né
il controllo di contiguità lo vedono. Il test è stato rafforzato con gli estremi (parte da `Start`, arriva a
`Goal`); con la mutazione riattivata cadono **entrambi**, e dopo il ripristino tornano verdi tutti e sei.

### 15.3 Verifica MCP end-to-end

Fatta senza Claude Code, parlando al server in JSON-RPC con `curl` — così la prova non dipende dalla
configurazione di un client:

```
initialize            -> HTTP 200, protocolVersion 2025-06-18, Mcp-Session-Id
tools/list            -> list_toolsets, describe_toolset, call_tool   (solo i 3 meta-tool: vedi §9.1)
list_toolsets         -> "RTDeveloperTools.RTDevToolset: ..."
call_tool ProjectStatus       -> engine 5.8.1, level L_DevSandbox, 45 celle, revision 1
call_tool GetCurrentMap       -> DA_HexMap_Scratch_Basin, 45 celle, 0 transizioni, layer [0], formato v10
call_tool DumpCell {0,0,0}    -> Floor, costo 1, 1 copertura, 6 vicini con costi 2/2/1/1/2/2
call_tool DumpCell {4,7,0}    -> isError: true, "Cell (q=4,r=7,L=0) does not exist ... (45 cells)"
call_tool FindPath (0,0,0)->(2,0,0) -> Success, 3 celle, totalCost 4, nodesVisited 8, 0.074 ms
call_tool ValidateTacticalMap -> bValid true, 0 issue
```

Il tempo del pathfinder — **0,074 ms**, ben sotto il target di 2 ms — è misurato dentro il facade attorno
alla sola `URTHexPathLibrary::FindPath`, come vuole §7: non include il round-trip MCP.

### 15.4 Quattro cose che il brief non poteva sapere

- 🔴 **La porta 8000 è occupata da Docker su questa macchina** (`com.docker.backend`, insieme a 8001-8004,
  8025, 8080, 8082). Il primo handshake ha risposto `404` da **uvicorn**: sembrava un difetto del bridge, ed
  era un altro servizio. Il server MCP accetta `-ModelContextProtocolPort=N`, e il log lo dichiara da sé.
- ⚠️ **`ModelContextProtocol.GenerateClientConfig ClaudeCode` legge la porta dai SETTINGS**
  (`GetServerPortNumber()`), non dall'override da riga di comando: generare la config mentre il server gira su
  una porta di override avrebbe scritto una `.mcp.json` che punta a Docker. La porta va messa nei settings —
  qui via `-ini:EditorPerProjectUserSettings:[...]:ServerPortNumber=8765`.
- ⚠️ **Git Bash riscrive gli argomenti che iniziano con `/`**: `/Game/RT/Maps/...` è arrivato all'editor come
  `C:/Program Files/Git/Game/RT/Maps/...` e la mappa non si è caricata. Serve `MSYS_NO_PATHCONV=1`.
- 🔴 **`RefactorTactics.uproject` ha il flag `skip-worktree`** (`git ls-files -v` → `S`): git ignora per scelta
  le modifiche locali a quel file. La riga che abilita `RTDeveloperTools` **non comparirà in nessun diff e non
  si committa** finché il flag resta. Chi clona deve abilitare il plugin da sé.

### 15.5 Cosa resta fuori

`rt_get_turn_log` e `rt_run_automation_tests`, per le ragioni di §1.8 — il TurnLog vive in PIE e
l'`AutomationTestToolset` di Epic esiste già. La configurazione `bAutoStartServer` è
`EditorPerProjectUserSettings`, cioè **per utente e non versionata**: va accesa una volta per macchina.

## Principio finale

Il bridge serve a far **osservare e testare** RefactorTactics a Claude, non a diventare un secondo motore.

```
Claude decide cosa ispezionare/testare
   → MCP trasporta la richiesta
   → il tooling Editor la valida
   → il C++ RefactorTactics esegue la logica reale
   → lo stato strutturato torna a Claude
```

La logica competitiva mantiene **una sola fonte di verità: il C++ di RefactorTactics**.

Rispondi e commenta in italiano; identificatori e termini tecnici restano in inglese dove naturale.
