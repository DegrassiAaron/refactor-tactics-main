# Onda 3 della sequenza di chiusura — misure senza persona, 2026-10-26

> **Che cosa è**: l'evidenza dei gate `G1` `G2` `G3` `G4` `G5` `G6` `G7` `G8` `G11` `G12` e `G14` della
> [`v0.1-definition-of-done.md`](../../../roadmap/v0.1-definition-of-done.md) §3, misurati sul candidate v2
> ([`D-541`](../../../decisions/RT_PDR_00_Decision_Log.md)). Le celle dei gate portano il verdetto e rimandano qui
> per i comandi e i numeri. Non è un owner: dove diverge dalla DoD, vale la DoD.
>
> **Convenzione sui numeri** ([`AGENTS.md`](../../../../AGENTS.md) §14): i numeri sono **esiti di questa tornata**, ciascuno
> col comando che lo produce.

## 0. Su che cosa si è misurato

| | |
|---|---|
| **Candidate v2** | `7450ead647cfce43ae419218302bdf8bd183cc73` (`D-541`) |
| **`HEAD` delle misure** | `ad06c1cfce82f14344cbd5ca6832f617c688cb11`, albero `16a97d52641b4fb6f1baec41a5e8645fe587cae5`, `git status --porcelain` → **0** righe prima e dopo |
| **Rapporto col candidate** | `git diff --stat 7450ead64 HEAD -- Source Content` → **due** file, entrambi `Source/RefactorTactics/Tests/`: la rinomina di `Reactions.Intercept.NotAcrossLayers` e il test `Naming.NoTestNameIsAPrefixOfAnother` ([#3854](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3854)). Il blocco `rev-parse` della finestra ([`D-542`](../../../decisions/RT_PDR_00_Decision_Log.md)) risponde `OK`, perché esclude i `Tests/` |
| **Albero di `Source`** | `15ce4d328400784ea763cec43100b0de9cf660d8` (quello del candidate è `15b2cd28815a95989f84054f7d4c2046a93955dc`: la differenza è solo nei `Tests/`). `Content` = `6c0cd69ff4c13fa379c5a6871e4f83c476aa4c49`, identico |
| **Pacchetto** | cotto da `HEAD` (`RunUAT BuildCookRun`, §4), `Saved/StagedBuilds/Windows/RefactorTactics`: `RefactorTactics.exe` (Development) 380.946.944 byte `sha256=080C6B394DEF74FD…`; `RefactorTactics-Win64-Shipping.exe` 189.588.992 byte `sha256=000CA59F11786CFD…`; `RefactorTactics-Windows.pak` 11.604.815 byte; `.ucas` 1.065.070.736 byte |
| **Macchina** | Unreal e Live Coding spenti prima di ogni lancio (`Get-CimInstance Win32_Process`); `C:` 72 GB liberi; RTX 4070, adapter 0 scelto dal RHI. ⚠️ **Finestra NON pulita**: processi estranei a questo progetto (`VBCSCompiler` e `dotnet test` di un altro progetto .NET, più quattro `bash.exe` di un plugin di Claude Code sotto `~/.claude/plugins/cache/superpowers-marketplace/`, circa mezzo core ciascuno) hanno tenuto il carico di fondo fra il 4% e il 100%. Vale per tutto ciò che segue, e pesa soprattutto su `G11` |

## 1. `G1` — tre build a freddo

`Build.bat <target> Win64 <cfg> -Project=… -Clean -WaitMutex -NoHotReloadFromIDE`, poi lo stesso senza `-Clean`, in PowerShell leggendo `EXIT=`. Per ciascun target la pulizia precede la build, e la non-incrementalità è provata dagli `.obj` del blob unity.

| Target | `Result` | Warning (`warning` qualunque / `C4996`) | Azioni di compilazione | `.obj` `Module.RefactorTactics.*` più vecchi dell'inizio build |
|---|---|---|---|---|
| `RefactorTacticsEditor Win64 Development` | `Succeeded` | 0 / 0 | 47 | 0 su 38 |
| `RefactorTactics Win64 Development` | `Succeeded` | 0 / 0 | 40 | 0 su 38 |
| `RefactorTactics Win64 Shipping` | `Succeeded` | 0 / 0 | 40 | 0 su 38 |

Una prima passata identica era già stata fatta su `fad26dd06`, prima della correzione di `#3854`: stessi esiti. Quella qui sopra è sul `HEAD` dichiarato.

## 2. `G2` `G3` `G4` `G6` `G8` — la suite lato Editor

`python -X utf8 tools/suite/esegui.py RefactorTactics --timeout 25 --log …` → **3439 trovati, 3439 avviati, 3439 completati, nessun rosso**, `**** TEST COMPLETE. EXIT CODE: 0 ****`, 310 s. La DLL dell'Editor ha lo stesso `sha256` a inizio e a fine run. Nessuna cartella nuova in `Saved/Crashes/`.

Letta con due metodi: l'oracolo `completati >= trovati - 1` di [#3048](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3048) e il conteggio indipendente dei `Result=` nel log (3439 `Result={Success}`, 0 `Result={Fail}`, 3439 percorsi distinti). L'elenco `esito → percorso` è in [`g2-editor-esiti-2026-10-26.txt`](g2-editor-esiti-2026-10-26.txt).

I nomi vincolanti, verificati **uno per uno sul `Path` completo** del log (non dedotti dal verde):

- **`G3`, i dieci**: `Actions.Dash.BlockedArc` · `Actions.Dash.DeniedByTerrain` · `Actions.Move.CellConflict` · `Actions.Move.PathBlocked` · `Actions.Push.InvalidDestination` · `Cover.DirectionalDamageReduction` · `Environment.WaterElectricPropagation` · `Environment.WaterExtinguishesFire` · `Reactions.Intercept` · `Reactions.SingleActivation` — tutti `Success`. Con i due che portano i `ClientContext` da dieci a dodici: `Cover.DefaultIntegrityMatchesCatalog`, `Environment.Propagation.ElectrifiedIsLoggedAsInstantLabel`.
- **`G4`**: `Replay.Verifier.ResimulationIsDeterministic` e `Combat.DeflectPoolIsPermutationInvariant` — `Success`.
- **`G6`**: `Catalog.IdsAreUnique` — `Success`, insieme agli altri `Catalog.*` della stessa famiglia (tutti `Success`).
- **`G8`**: `Reactions.IntentNotVisibleToEnemy` · `Facing.IntentIsTeamFiltered` · `Combat.IntentVisibleToAlliesAlwaysEnemiesOnlyIfRevealed` · `UI.IntentViewFieldsAreClassified` · `UI.EnemyViewCarriesNoAllyOnlyField` — `Success`.

🔴 **`Reactions.Intercept` non girava nella prima suite di questa tornata** (`HEAD` `fad26dd06`): era dichiarato e assente dal log, e `trovati = avviati = completati` restava vero. Causa: `Reactions.Intercept.NotAcrossLayers` (commit `9d0d1d2f9`, 2026-10-13) lo rendeva nodo padre. Trovato confrontando i nomi dichiarati nei `.cpp` con i `Path` del log; corretto da [#3854](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3854) e **qui rimisurato sul `HEAD` corretto**: ora compare, `Success`. Il test nuovo `Naming.NoTestNameIsAPrefixOfAnother` è stato visto rosso (nominando solo `Reactions.Intercept`) prima della rinomina.

### `G2`, la metà packaged

`Saved/StagedBuilds/Windows/RefactorTactics/Binaries/Win64/RefactorTactics.exe -ExecCmds="Automation RunTests RefactorTactics;Quit" -unattended -nopause -RenderOffScreen -NoSound -abslog=…` (⛔ **non** `-nullrhi` sul binario cotto: crasha, vedi la cella di `G2`) → **`Found 13`, 13 `Result={Success}`, 0 `Result={Fail}`**, fra cui `Reactions.Intercept`. Il processo esce con **3** dopo gli esiti (ultimo `Result` alla riga 6641 del log, primo segnale `DXGI_ERROR_DEVICE_REMOVED` alla 6656) e lascia **una** cartella nuova in `Saved/StagedBuilds/…/Saved/Crashes/`: è [#3424](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3424), già noto. `Error:` nel log: 13 righe, tutte `LogRHI`/`LogD3D12RHI` posteriori ai test; `Critical error`, `Fatal error`, `Assertion failed` → 0. Il verdetto si legge dai conteggi e non dall'uscita; **il verde non copre la chiusura del processo**.

## 3. `G5` e `G7`

- **`G5`**: `git grep -l FRTGridCoord -- Source/` → **0**; controllo positivo `git grep -l FRTCellId -- Source/` → **415** file.
- **`G7`**: i tre congiunti. (1) validator: `Catalog.NoFloatInIntegerFields`, `Catalog.PlacedUnitGameplayNumbersAreIntegers`, `Catalog.BlueprintSurfaceOfTheG7PerimeterIsEnumerated` → `Success` nella suite di §2. (2) revisione dei data asset: **non rieseguita**, accettata e dichiarata da [`D-450`](../../../decisions/RT_PDR_00_Decision_Log.md). (3) `grep -rnE '"[A-Za-z][A-Za-z0-9]*"[[:space:]]*:[[:space:]]*"?-?[0-9]+([.][0-9]+|[eE][-+]?[0-9]+)' Scenarios/` → **0** righe su 161 file (`find Scenarios -name "*.json" -not -name "_*"`); rilevatore sulla sonda a nove forme: **6 su 6** pericolose colpite, **0 su 3** lecite.

## 4. `G12` — packaging

`RunUAT.bat BuildCookRun -project=… -noP4 -platform=Win64 -clientconfig=Development+Shipping -cook -build -stage -pak` da `HEAD` → **`BUILD SUCCESSFUL`**, `AutomationTool exiting with ExitCode=0 (Success)`, 137 s. Prima: `git status --porcelain` → 0, `Content/FabAsset` → **37482** `.uasset`; dopo: `HEAD` e stato invariati, `LogZenStoreWriter: Error` → 0.

Controllo positivo sul contenuto, con `UnrealPak RefactorTactics-Windows.utoc -List`: `L_HexArena` → 2 righe, `L_Frontend` → 1, e la controprova `L_NonEsiste_Controprova` → **0**.

⚠️ **`Error compiling` nel log di cottura: 924 righe**, della stessa famiglia che la cella di `G12` registrava il 2026-10-02 ("sei righe su nodi di materiale"): `DistanceToSurface`, `DistanceFieldIsValid`, `Less Than`, `Multiply`, `Logic AND`, ripetute per 154 compilazioni. Non hanno fatto fallire la cottura. Nessun asset di `Content/RT` contiene quei nomi (`grep -rl -a` → 0), quindi vengono da materiali del motore o di `FabAsset`. **Il numero non è confrontabile con "sei" e la causa non è stata indagata** (ipotesi non verificata: cache degli shader fredda).

## 5. `G11` — i KPI rimisurabili senza persona

⚠️ **Finestra non pulita** (§0). Il controllo che rende leggibile una misura di tempo su questa macchina è `Perf.PathfindingMedian`, nelle stesse esecuzioni: se la macchina fosse il doppio più lenta, il `Path` raddoppierebbe.

**Famiglia `RefactorTactics.Perf`**, tre esecuzioni sul `HEAD` (più la suite intera di §2), 5 test su 5 `Success` ogni volta:

| | pathfinding (mediana) | preview (mediana / p90) | resolver 2v2 (mediana, ms/turno) | `ResolverAt4v4` | carico CPU prima/dopo |
|---|---|---|---|---|---|
| esecuzione 1 | 0,074 ms | 0,126 / 0,147 ms | 11,169 | 11,927 | 26% / 13% |
| esecuzione 2 | 0,115 ms | 0,145 / 0,158 ms | 15,225 | 23,992 | 21% / 96% |
| esecuzione 3 | 0,078 ms | 0,157 / 0,194 ms | 12,212 | 10,837 | 99% / 3% |
| suite intera (§2) | 0,105 ms | 0,099 / 0,154 ms | 13,362 | 13,385 | non campionato per esecuzione |

Contro i target: pathfinding `< 2 ms`, preview `< 50 ms`, resolver `< 100 ms`. Il `Path` sta dove stava il 2026-10-03 (0,113 ms): **la macchina non è due volte più lenta**.

**Client FPS** — `RefactorTactics.exe /Game/RT/Maps/Dev/L_HexArena/L_HexArena -RTAutobattle -dpcvars=rt.Map.Source=LevelAsset -csvCaptureFrames=4000 -ExitAfterCsvProfiling -windowed -ResX=1280 -ResY=720 -unattended -nopause -nosplash -NoSound`, pacchetto Development di §0, sei catture da **4000** frame in due serie, `FrameTime` cercata **per nome** nell'intestazione (colonna 127–163 su 324–333, mai la stessa), percentili nearest-rank sui frame dopo i primi 300 (3700):

| Serie | Cattura | p50 | p95 | p99 | frame > 16,67 ms | carico sistema (medio / max) |
|---|---|---|---|---|---|---|
| a | 1 | 13,617 | 26,999 | **111,471** | 22,97% | 45% prima, non campionato |
| a | 2 | 12,225 | 21,600 | **69,810** | 15,73% | 64% prima, non campionato |
| a | 3 | 11,196 | 17,613 | **24,639** | 6,73% | 18% prima, non campionato |
| b | 1 | 10,428 | 15,944 | **20,184** | 3,49% | 27% / 37% |
| b | 2 | 10,114 | 14,429 | **18,826** | 2,08% | 20% / 31% |
| b | 3 | 10,137 | 15,306 | **20,561** | 3,22% | 25% / 45% |

La serie `b` è la più pulita (carico di fondo misurato durante la cattura). **Il target è 60 fps, cioè p99 ≤ 16,67 ms: nella serie più pulita non è raggiunto** (49–53 fps). Il 2026-10-03 era `9,326–9,683` ms, il 2026-09-12 `7,154–7,858`. **Non attribuito.** Due letture dal CSV, senza concludere: il frame è legato al **game thread** (`GameThreadTime` p50 ≈ 9,4–9,6 ms; `GPUTime` p50 ≈ 2,3 ms, `RHIThreadTime` ≈ 2,5 ms) e non alla GPU; e il `Path` di controllo non si è mosso. Tra `95eddfd37` e `HEAD` sono cambiati anche `DefaultEngine.ini` (`r.CustomDepth=3`, `GameViewportClientClassName`) e 36 `.uasset` di `Content/`.

## 6. `G14`

Misurato **dopo** l'aggiornamento delle celle (il gate allinea anche i loro esiti), con i due comandi del criterio:

- `node tools/radar/doc-coherence.ts --check` → `PASS A1` (284 voci del registro PIE, 116/34/7/127) · `PASS A2` (20 sedute critiche su 93) · `PASS A3` · `PASS A4` · `PASS A5` (24 epic, somma CP 124, 124 id unici); exit 0.
- `node tools/radar/doc-links.ts --check` → 8314 link esaminati in 522 documenti, *tutti i percorsi citati risolvono*; exit 0.
- `RefactorTactics.Meta.SatisfiedGatesAgreeWithTheDefinitionOfDone` (famiglia `Meta`, 15 test su 15 `Success`) rieseguito sulla DoD modificata.

## 7. Cosa NON è stato misurato

`G9` (subset `RELEASE-V01` della PIE), `G10` (partita 2v2 non degenere), `G13` (partita giocabile senza editor), `G16` (giro `U60`): sono sedute con una persona o presidiate, Onda 4. **NOT RUN.** Nessuna PIE è stata eseguita in questa tornata.
