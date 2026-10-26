# Misure sul candidate v3 — i gate delle Onde 3 e 4 rieseguiti, 2026-10-26

> **Che cosa è**: l'evidenza dei gate `G1` `G2` `G3` `G4` `G5` `G6` `G7` `G8` `G10` `G11` `G12` `G13` e della parte
> misurabile di `G16` della [`v0.1-definition-of-done.md`](../../../roadmap/v0.1-definition-of-done.md) §3, rieseguiti sul
> candidate v3 ([`D-543`](../../../decisions/RT_PDR_00_Decision_Log.md)) dopo la correzione di
> [#3859](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3859). Le misure sul candidate v2 restano in
> [`onda3-misure-2026-10-26.md`](onda3-misure-2026-10-26.md) e [`onda4-misure-2026-10-26.md`](onda4-misure-2026-10-26.md), e
> le celle della DoD le portano come *misura precedente*. Non è un owner: dove diverge dalla DoD, vale la DoD.
>
> **Convenzione sui numeri** ([`AGENTS.md`](../../../../AGENTS.md) §14): i numeri sono **esiti di questa tornata**, ciascuno
> col comando che lo produce. Un numero che non ho potuto ricontare è detto tale.
>
> ⚠️ **Chi ha guidato che cosa.** Tutte le misure di questo documento sono state eseguite **dall'assistente**, su richiesta
> dell'autore (*«esegui tu»*). Per `G13` questo significa **input sintetico del sistema operativo** (clic e tasti inviati da
> uno script PowerShell alla finestra del pacchetto), **non** una persona che gioca: i tre giudizi percettivi dell'autore non
> sono stati riespressi sul v3 (§7). `G16` non è stato eseguito (§8).

## 0. Su che cosa si è misurato

| | |
|---|---|
| **Candidate v3** | `947a783d5e423e23d8c6bc7a88b77dc50af0daa4` (`D-543`, merge di [#3864](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3864)) |
| **`HEAD` delle misure** | `4d90b5375b3435400e0f4b1dd3d10f70101ccbb4`, albero `e001bdf9f274c8afb845135d0bd6dfd0bedec0f7`. `git rev-parse HEAD:Source HEAD:Content` → `17af6090fbd103bca259beb610438d5ea35ab4f9` e `6c0cd69ff4c13fa379c5a6871e4f83c476aa4c49`, **gli stessi alberi del candidate** ([`D-543`](../../../decisions/RT_PDR_00_Decision_Log.md) (2)). `git diff --quiet 947a783d5 HEAD -- Source Content` → `OK`, **anche senza** le esclusioni dei `Tests/` che il blocco della finestra ([`D-542`](../../../decisions/RT_PDR_00_Decision_Log.md)) porta: fra candidate e `HEAD` non cambia nemmeno un test. `git status --porcelain` → 0 righe prima della cottura di `G12` (è la precondizione dello script) |
| **Rapporto col v2** | `git diff --name-only 7450ead64 HEAD -- Source Content ':(exclude)Source/RefactorTactics/Tests' ':(exclude)Source/RefactorTacticsEditor/Private/Tests'` → **un** file, `Source/RefactorTactics/Turn/RTReplayRecording.cpp`; il blocco della finestra contro il v2 risponde `DIVERSI`, contro il v3 `OK` |
| **DLL dell'Editor** | `Binaries/Win64/UnrealEditor-RefactorTactics.dll` 23.998.464 byte, `sha256` `6aa640d689c1ee87…`, scritta alle 09:19:05 dalla build a freddo di `G1`, **prima** dell'avvio della suite (09:22:35). Il `sha256` a inizio e a fine run è calcolato dallo script della suite, ma il suo output non è conservato: vale l'ora di scrittura |
| **Pacchetto** | cotto da `HEAD` (§4), `Saved/StagedBuilds/Windows/RefactorTactics`: `RefactorTactics.exe` (Development) 380.952.064 byte `sha256` `d17e2dc19de8a4ba…`; `RefactorTactics-Win64-Shipping.exe` 189.592.576 byte `sha256` `97ff84f056d0e134…`; `RefactorTactics-Windows.pak` 11.604.815 byte e `.ucas` 1.065.070.736 byte (**la stessa dimensione del v2**, che registra solo le dimensioni: `Content/` non è cambiato) |
| **Macchina** | Unreal e Live Coding spenti prima di ogni lancio (ogni script lo verifica con `Get-CimInstance Win32_Process` ed esce se occupato). ⚠️ **Il carico della macchina non è stato controllato, e non è sempre conservato.** Dove c'è, è `Win32_Processor.LoadPercentage` campionato durante la misura, **gioco o Editor inclusi**: non distingue il carico altrui da quello della misura stessa. È conservato per la suite lato Editor (32 campioni: medio 26%, massimo 60%, [`v3/carico-suite-editor.txt`](v3/carico-suite-editor.txt)) e per la serie `c` dei `Client FPS` (§6); per le serie `a` e `b` e per le tre esecuzioni `Perf` non è conservato. Pesa su `G11`, non sugli altri gate. Gli estratti dei log che i numeri citano stanno in [`v3/`](v3/) |

## 1. `G1` — tre build a freddo

`Build.bat <target> Win64 <cfg> -Project=… -Clean -WaitMutex -NoHotReloadFromIDE`, poi lo stesso senza `-Clean`, in PowerShell leggendo `EXIT=`. La non-incrementalità è provata dagli `.obj` del blob unity, **ricontati sul filesystem a fine tornata** (`find Intermediate/Build/Win64/x64/<…>/<cfg>/RefactorTactics -name 'Module.RefactorTactics.*.cpp.obj'`) contro l'ora del log della pulizia, che è l'inizio della build. I log integrali di pulizia e build sono in [`v3/`](v3/) (`g1-*.log`).

| Target | `Result` | Durata | Warning (`warning` qualunque / `C4996`) | Azioni di compilazione | `.obj` `Module.RefactorTactics.*` più vecchi dell'inizio build |
|---|---|---|---|---|---|
| `RefactorTacticsEditor Win64 Development` | `Succeeded` | 151,69 s | 0 / 0 | 54 | 0 su 38 (inizio 09:16:43) |
| `RefactorTactics Win64 Development` | `Succeeded` | 107,45 s | 0 / 0 | 43 | 0 su 38 (inizio 09:19:18) |
| `RefactorTactics Win64 Shipping` | `Succeeded` | 78,16 s | 0 / 0 | 43 | 0 su 38 (inizio 09:21:08) |

Le azioni non sono quelle dell'Onda 3 (47, 40, 40): non è un criterio del gate e non è stato indagato. Nella cartella `UnrealEditor/DebugGame/RefactorTactics` ci sono 14 `.obj` del 2026-08-26: sono di un'altra configurazione e non sono contati.

## 2. `G2` `G3` `G4` `G6` `G8` — la suite lato Editor

`python -X utf8 tools/suite/esegui.py RefactorTactics --timeout … --log …` → **3440 trovati** (`Found 3440 automation tests`), **3440 avviati** (`Test Started`), **3440 completati** (`Test Completed`), `**** TEST COMPLETE. EXIT CODE: 0 ****`, 191 s dalla prima all'ultima riga con timestamp.

Letta con due metodi: l'oracolo di [#3048](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3048) (`tools/mutation/misura.py`: `avviati >= trovati` e `completati >= avviati - 1`) e il conteggio indipendente dei `Result=` nel log: **3440** `Result={Success}`, 0 altri esiti, **3440 percorsi distinti**. L'elenco `esito → percorso` è in [`g2-editor-esiti-v3-2026-10-26.txt`](g2-editor-esiti-v3-2026-10-26.txt).

I nomi vincolanti, cercati **uno per uno sul `Path` completo** di quell'elenco (non dedotti dal verde), tutti `Success`:

- **`G3`, i dieci di `roadmap-v0.1.md` §6**: `Actions.Move.PathBlocked` · `Actions.Move.CellConflict` · `Actions.Dash.BlockedArc` · `Actions.Push.InvalidDestination` · `Environment.WaterElectricPropagation` · `Environment.WaterExtinguishesFire` · `Cover.DirectionalDamageReduction` · `Reactions.Intercept` · `Reactions.SingleActivation` · `Replay.Verifier.ResimulationIsDeterministic`. Più i tre che portano i `ClientContext` da dieci a tredici: `Actions.Dash.DeniedByTerrain`, `Cover.DefaultIntegrityMatchesCatalog`, `Environment.Propagation.ElectrifiedIsLoggedAsInstantLabel`.
- **`G4`**: `Replay.Verifier.ResimulationIsDeterministic` e `Combat.DeflectPoolIsPermutationInvariant`.
- **`G6`**: `Catalog.IdsAreUnique`, insieme agli altri `Catalog.*` della famiglia (nessuno diverso da `Success`).
- **`G8`**: `Reactions.IntentNotVisibleToEnemy` · `Facing.IntentIsTeamFiltered` · `Combat.IntentVisibleToAlliesAlwaysEnemiesOnlyIfRevealed` · `UI.IntentViewFieldsAreClassified` · `UI.EnemyViewCarriesNoAllyOnlyField`.
- **I test che il v3 aggiunge o sposta**: `Replay.Recording.TheHistoryRowIsCompletedWhenTheMatchEnds` (la correzione di #3859), `Naming.NoTestNameIsAPrefixOfAnother` e `Reactions.InterceptNotAcrossLayers` (#3854): `Success`. `Reactions.Intercept` **compare** nel log, che è ciò che la regola di nomenclatura 6.1 garantisce.

### `G2`, la metà packaged

`Saved/StagedBuilds/Windows/RefactorTactics/Binaries/Win64/RefactorTactics.exe -ExecCmds="Automation RunTests RefactorTactics;Quit" -unattended -nopause -RenderOffScreen -NoSound -abslog=…` (⛔ non `-nullrhi` sul binario cotto: crasha) → **`Found 13`, 13 `Result={Success}`, 0 `Result={Fail}`**, fra cui `Reactions.Intercept`. L'ultimo `Result` sta alla riga 6638 del log; poi `DXGI_ERROR_DEVICE_REMOVED` alla 6754 e `GPU Crashed or D3D Device Removed` alla 6764: è [#3424](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3424), già noto. `Error:` nel log: **13 righe, tutte posteriori all'ultimo test** (11 `LogD3D12RHI`, 2 `LogRHI`); `Critical error`, `Fatal error`, `Assertion failed` → 0. Lascia **una** cartella in `Saved/StagedBuilds/…/Saved/Crashes/`, `UECC-Windows-B11AD91F481A103CA48367A2F871D8F8_0000`, che porta lo stesso `CrashGUID` stampato all'inizio di quel log. **Il codice di uscita del processo non è conservato** (lo script lo stampava a console e l'output non è stato salvato): il verdetto si legge dai conteggi, e il verde non copre la chiusura del processo. Estratto con i numeri di riga: [`v3/g2-packaged-estratto.txt`](v3/g2-packaged-estratto.txt).

## 3. `G5` e `G7`

Misurati sul `HEAD` al momento di scrivere questo documento.

- **`G5`**: `git grep -l FRTGridCoord -- Source/ | wc -l` → **0**; controllo positivo `git grep -l FRTCellId -- Source/ | wc -l` → **415**.
- **`G7`**: i tre congiunti. (1) validator: `Catalog.NoFloatInIntegerFields`, `Catalog.PlacedUnitGameplayNumbersAreIntegers`, `Catalog.BlueprintSurfaceOfTheG7PerimeterIsEnumerated` → `Success` nella suite di §2. (2) revisione dei data asset: **non rieseguita**. [`D-450`](../../../decisions/RT_PDR_00_Decision_Log.md) non accetta quella revisione: accetta e dichiara il residuo del pannello Details su `.uasset`, la superficie che i congiunti (1) e (3) non coprono. (3) `grep -rnE '"[A-Za-z][A-Za-z0-9]*"[[:space:]]*:[[:space:]]*"?-?[0-9]+([.][0-9]+|[eE][-+]?[0-9]+)' Scenarios/ | wc -l` → **0** su **161** file (`find Scenarios -name "*.json" -not -name "_*" | wc -l`); lo stesso rilevatore sulla sonda a nove forme: **6 su 6** pericolose colpite (`2.7`, `"2.7"`, `2.0`, `27e-1`, `2.7E0`, `-2.7`), **0 su 3** lecite (`3`, `100`, `-1`).

## 4. `G12` — packaging

`RunUAT.bat BuildCookRun -project=… -noP4 -platform=Win64 -clientconfig=Development+Shipping -cook -build -stage -pak` da `HEAD` → **`BUILD SUCCESSFUL`**, `AutomationTool exiting with ExitCode=0 (Success)`, `BuildCookRun time: 77.33 s`. Precondizioni dello script, verificate prima di cuocere: `git status --porcelain` → 0 e `Content/FabAsset` ≥ 1000 `.uasset`; ricontati dopo, `Content/FabAsset` → **37482** `.uasset` (come nel v2). `LogZenStoreWriter: Error` → 0.

Controllo positivo sul contenuto, con `UnrealPak … -List` sul `.utoc`: `L_HexArena` → 2 righe, `L_Frontend` → 1, e la controprova `L_NonEsiste_Controprova` → **0**.

⚠️ **`Error compiling` nel log di cottura: 924 righe**, lo stesso numero dell'Onda 3, con gli stessi messaggi (`Less Than` ×308; `Output Function Pin: DistanceToSurface`, `Output Function Pin: DistanceFieldIsValid`, `Multiply` e `Logic AND` ×154 ciascuno). Non hanno fatto fallire la cottura, e **non è stato indagato di nuovo** perché la cottura produca queste righe: la lettura dell'Onda 3 (§4) resta quella, con i suoi limiti. Estratto: [`v3/g12-cottura-estratto.txt`](v3/g12-cottura-estratto.txt).

## 5. `G10` — partita 2v2 autobattle

`UnrealEditor-Cmd.exe <uproject> /Game/RT/Maps/Dev/L_HexArena/L_HexArena -game -RTAutobattle -useFixedTimeStep -fps=60 -unattended -nopause -nosplash -nullrhi -NoLiveCoding -abslog="…"`, **due volte** di seguito. In entrambe il log dichiara la fine 16 s dopo la sua prima riga; il processo non esce da solo, quindi l'ho terminato io dopo aver verificato sulla `CommandLine` che portasse il mio `-abslog`.

**Esito** (riga testuale, identica nelle due): *«Partita finita: Vince il team 0 (blu) - per eliminazione (round 9/12, obiettivo 0-2, formato Format.Skirmish2v2)»* e *«Fine partita al round 9: ERTMatchOutcome::Team0Wins»*; apertura *«Board 2v2 esagonale avviata su 64 celle con 4 eroi»*.

**Non degenere**: 26 righe con ` danni`; 2 `Eliminata:`; `obiettivo 0-2`; reazioni di `Hero.Aevik.ReactiveCapacitor` presenti nel flusso. **Determinismo**: il flusso `LogRT` senza prefissi di timestamp è **identico** fra le due run (407 righe, stesso `sha256` `84fdeebe8f4100dc…`). **Rispetto al v2** il flusso è identico **salvo una riga**, `Screen HUD 4.1 montato … modulo compilato il Oct 26 2026 alle 06:59:00` contro `…alle 09:18:22` (`diff` sui due file archiviati → una riga differente): stessa partita, stesso esito, stesso round. Era attesa: il v3 cambia un solo file fuori dai `Tests/`, e il flusso identico lo conferma per questa partita.

🔑 **Il fatto nuovo, ed è quello della correzione**: nell'indice del progetto, `Saved/Replays/history.rtindex`, le righe di queste due partite headless (08:28:49 e 08:29:15 UTC) dicono `Outcome 1`, `TurnCount 9`, `ReplayComplete true`. Le due partite headless di `G10` sul v2 erano `Outcome 0`, `TurnCount 0`, `ReplayComplete false` ([`onda4`](onda4-misure-2026-10-26.md) §3).

**Riserva che resta**, la stessa del v2: nessuna riga porta `L=1` (`grep -aoE '[( ,]L=?[0-9]+[),]'` → 208 `L=0)` e 31 `L0)`, nessun'altra classe). La mappa è multilivello, la partita no.

Log: [`../g10/partita-2v2-autobattle-candidate-v3-2026-10-26.log`](../g10/partita-2v2-autobattle-candidate-v3-2026-10-26.log).

## 6. `G11` — i KPI rimisurabili senza persona

**Famiglia `RefactorTactics.Perf`**, tre esecuzioni (`python -X utf8 tools/suite/esegui.py RefactorTactics.Perf --timeout 10`) più la suite intera di §2; 5 test su 5 `Success` ogni volta. Il carico della CPU durante queste esecuzioni **non è conservato** (§0). Le righe KPI di ciascun log: [`v3/perf-kpi-estratto.txt`](v3/perf-kpi-estratto.txt).

| | pathfinding (mediana) | preview (mediana / p90) | resolver 2v2 (mediana, ms/turno) | `ResolverAt4v4` (ms/turno) | transito, arena r=50 (ms per unità) |
|---|---|---|---|---|---|
| esecuzione 1 | 0,071 ms | 0,093 / 0,100 ms | 9,754 | 7,540 | 2,776 |
| esecuzione 2 | 0,071 ms | 0,094 / 0,128 ms | 9,299 | 8,576 | 2,821 |
| esecuzione 3 | 0,069 ms | 0,093 / 0,100 ms | 9,099 | 7,952 | 2,840 |
| suite intera (§2) | 0,072 ms | 0,092 / 0,145 ms | 10,221 | 8,613 | 2,791 |

Contro i target: pathfinding `< 2 ms`, preview `< 50 ms`, resolver `< 100 ms`. Il resolver è **dentro il target e fuori dalla banda `3,8–4,5`** che la riga della DoD porta dal 2026-08-14 (la lettura è quella del 2026-10-03, non rimessa in discussione qui). I valori sono più bassi e più stretti di quelli dell'Onda 3 (resolver 11,169–15,225 ms); **non è attribuito** e non è una prova di miglioramento: il carico della macchina non è confrontabile fra le due tornate (per queste esecuzioni non è conservato). Il test `E17` della suite intera: *«partita 4v4 decisa in 11 turni»*, *«divergenza replay 4v4: 0 su 11 turni confrontati»*.

**Client FPS** — `RefactorTactics.exe /Game/RT/Maps/Dev/L_HexArena/L_HexArena -RTAutobattle -dpcvars=rt.Map.Source=LevelAsset -csvCaptureFrames=4000 -ExitAfterCsvProfiling -windowed -ResX=1280 -ResY=720 -unattended -nopause -nosplash -NoSound -abslog=…`, pacchetto Development di §0, **nove catture** da 4000 frame in tre serie di tre, `FrameTime` cercata **per nome** nell'intestazione (la colonna cambia da una cattura all'altra: 91–185 su 315–319), percentili nearest-rank sui 3700 frame dopo i primi 300:

| Serie | Cattura | p50 | p95 | p99 | max | frame > 16,67 ms | carico CPU totale, gioco incluso (medio / max) |
|---|---|---|---|---|---|---|---|
| a | 1 | 5,835 | 7,249 | **8,400** | 29,4 | 0,05% | non conservato |
| a | 2 | 8,465 | 13,064 | **19,504** | 362,4 | 1,73% | non conservato |
| a | 3 | 6,442 | 8,430 | **9,604** | 18,8 | 0,05% | non conservato |
| b | 1 | 9,182 | 12,264 | **14,268** | 28,6 | 0,27% | non conservato |
| b | 2 | 8,696 | 11,397 | **12,913** | 22,5 | 0,14% | non conservato |
| b | 3 | 9,125 | 11,580 | **13,024** | 32,0 | 0,14% | non conservato |
| c | 1 | 6,194 | 7,799 | **9,358** | 28,4 | 0,05% | 39% / 69% (10 campioni) |
| c | 2 | 10,183 | 14,840 | **25,518** | 79,2 | 3,68% | **94% / 100%** (15 campioni) |
| c | 3 | 6,559 | 8,220 | **9,871** | 38,0 | 0,27% | 63% / 96% (11 campioni) |

**Lettura.** Il target è 60 fps, cioè `p99 ≤ 16,67 ms`. **Sette catture su nove** lo rispettano (`p99` 8,400–14,268 ms, cioè 70–119 fps); due no: `a2` (19,504 ms, 51 fps, carico non conservato, **non attribuita**) e `c2` (25,518 ms, 39 fps), che ha il carico CPU totale più alto fra le tre della serie `c` (medio 94%, massimo 100%, dal secondo campione in poi: il primo è 49%). ⚠️ Quel carico **include il gioco stesso**, quindi non distingue causa da effetto: `c1` e `c3`, stesso binario, danno 39% e 63%. Non provo che sia la causa di `c2`. Percentili ricalcolati dai CSV e campioni: [`v3/fps-percentili.txt`](v3/fps-percentili.txt). Come nell'Onda 4, **il valore da citare è l'insieme delle nove**, non la migliore né la peggiore; ripetere la misura in una finestra con carico registrato è ciò che manca per attribuire `a2`. In tutte le catture il log contiene `32008` righe `Icona non risolta` (`grep -a -c`, identico), come nel v2: [#3861](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3861); nell'Onda 4 non pesavano sul `p99`, e qui non è stato ripetuto il controllo.

## 7. `G13` — la seduta sul pacchetto, pilotata dall'assistente

**Pacchetto**: quello di §0. **Lancio**, come prescrive il foglio: `RefactorTactics.exe -windowed -ResX=1600 -ResY=900 -RTPlanningSeconds=600 -abslog=…\Saved\Logs\g13-seduta.log`, **senza** `-RTAutobattle` e senza mappa: il log dichiara la riga di comando ricevuta e il frontend. `rt.Debug.RecordPacing 1` armato dalla console del gioco **dopo l'inizio del turno 1** (08:42:53 UTC contro `Turno 1 - pianificazione` alle 08:42:17: dal frontend non si può armare prima, come dice il foglio; la riga del turno 1 è comunque nel CSV), con `Registrazione CSV del pacing: SPENTA -> ATTIVA`.

**Che cosa ho fatto, e che cosa no.** Ho pilotato la finestra con uno script PowerShell (`SetCursorPos` e `mouse_event`/`keybd_event` di `user32.dll`; schermate con `PrintWindow`), non una persona. **Turno 1**: selezione di `Aevik`, un waypoint, tasto destro (annulla il waypoint: `Back: livello ERTPointerBackStep::Waypoint`), di nuovo un waypoint, Invio (*«piano DICHIARATO»*), Spazio (*«Lock-in del giocatore -> risoluzione anticipata (turno 1)»*). **Turni 2–12**: solo lock-in con Spazio, **senza ordini**. Non ho giocato: ho percorso il percorso del foglio. La finestra è stata chiusa da me (`UGameEngine::Tick.ViewportClosed`), uscita normale: il log termina con `LogExit: Exiting.`.

**La partita**: 12 turni, `Pareggio - allo scadere dei round (round 12/12, obiettivo 0-0, formato Format.Skirmish2v2)`, `Fine partita al round 12: ERTMatchOutcome::Draw`; un'unità del team 0 eliminata al turno 8 (`Eliminata: BP_Unit_Aevik_C_… (team 0)`).

**Gli esiti meccanici**, da `python -X utf8 tools/seduta/esiti.py Saved/Logs/g13-seduta.log --csv <pacing_20261026-094554.csv> --crashes <Saved/Crashes del pacchetto>` (output integrale in [`../g13/v3/esiti-seduta-g13-v3-2026-10-26.txt`](../g13/v3/esiti-seduta-g13-v3-2026-10-26.txt)):

| esito | risultato |
|---|---|
| controllo del file (la ricerca sa rispondere) | PASS, `LogInit` ×92 |
| cancello: la partita è avvenuta | PASS: allestimento 1, fine partita del frontend 1, esito deciso 1; `Partita finita senza frontend` → 0 |
| nessun crash (rete storica / rete larga / GPU) | PASS 0 / PASS 0 / PASS 0 |
| nessun asset mancante, nessun overlay di debug, nessun `Travel Failure` | PASS 0 / PASS 0 / PASS 0 |
| CSV: colonne per nome | `SelectionCount` col. 6, `OrderCount` col. 7, `UndoCount` col. 8 |
| CSV: turni letti | PASS: 12 turni (righe scartate 0) |
| CSV: input funzionante | PASS: 1 turno soddisfa i tre congiunti (turno 1: selezioni 1 · ordini 2 · annulli 1) |
| cartelle di crash | **FAIL dello script, non della seduta** — vedi sotto |

⚠️ **Il `FAIL` sulle cartelle di crash** è il conteggio **assoluto**: la sola cartella presente, `UECC-Windows-B11AD91F481A103CA48367A2F871D8F8_0000` (09:28:24), è quella della metà packaged di `G2` (§2), preesistente alla seduta (09:41–09:47) e con lo stesso nome prima e dopo. Nessuna cartella nuova.

**Giudizi dell'autore: NON espressi sul v3.** Quelli sul v2 erano: interfaccia leggibile **sì**; affordance trovata senza leggere il foglio **sì**; nessun overlay di debug acceso che non avesse acceso lui **sì (assente)**. Li *riporto* come misura precedente senza riesprimerli: il v3 differisce dal v2 in un solo file fuori dai `Tests/`, `RTReplayRecording.cpp`, che completa la riga dell'indice delle partite alla chiusura. In partita l'unico cambiamento visibile atteso è l'elenco delle partite (§8, passo 5 di `G16`), non ciò che `G13` guarda; il flusso di `G10`, headless (`-nullrhi`) e quindi poco informativo sull'interfaccia, è identico al v2 salvo la riga con la data di compilazione del modulo (§5). È una **ragione per ritenere** che il resto a occhio non cambi, **non una prova**: l'autore può non accettarla. Il terzo (nessun overlay) ha un controllo meccanico che qui è PASS (`nessun overlay di debug` 0). Il secondo (affordance trovata **senza leggere il foglio**) **non può avere una risposta mia per costruzione**: il foglio l'ho letto. **Al momento della misura** (superato dal ➕ qui sotto): la DoD dice dell'occhio che *«non si scorpora»*, e per questo `G13` era `⏳` sul v3 e non ✅. **Per portarlo a ✅ serviva una persona** ([`D-543`](../../../decisions/RT_PDR_00_Decision_Log.md) (4)): rigiocare la seduta, oppure una deroga dichiarata dall'autore che accetti il riporto dei giudizi del v2.

➕ **Dopo, 2026-10-26: l'autore ha scelto la deroga** ([`D-544`](../../../decisions/RT_PDR_00_Decision_Log.md), «1. deroga 2.si 3.cosa?»; l'assistente legge il primo numero come la scelta della deroga e **non legge** il 2 e il 3: la lettura è in `D-544`). I suoi tre giudizi del v2 valgono sul v3 e la seduta non si rigioca; `G13` è ✅ **per deroga**. Ciò che questa riga dice, e che non dice: la meccanica di questo paragrafo è misurata sul v3; l'occhio è quello dell'autore sul v2, riportato per sua dichiarazione; **nessuna persona ha giocato il pacchetto v3**. Le osservazioni dell'assistente qui sotto restano osservazioni, non giudizi, e non sono coperte dalla deroga. L'autore le ha lette nel rapporto prima di rispondere. Il diff fra v2 e v3 non tocca nessun file di UI (un solo file fuori dai `Tests/`, il registratore), quindi è molto probabile che fossero già sul v2, alla stessa risoluzione; non l'ho verificato, perché nessuna schermata del v2 è stata archiviata.

**Osservazioni dell'assistente dalle schermate** (le schermate in [`../g13/v3/`](../g13/v3/)): **non sono giudizi dell'autore** e **non erano issue al momento della seduta**: sono state tracciate dopo, il 2026-10-26, sulla delega letta in `D-544` ([#3868](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3868) il Replay Viewer, [#3869](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3869) il prompt in basso, un commento su [#940](https://github.com/DegrassiAaron/refactor-tactics-main/issues/940#issuecomment-6414472004) per la fine partita).

- *Testo in basso tagliato*: *«Scegli un'azione»* e *«Seleziona un'unita'»* sono tagliati dal bordo inferiore a 1600×900 (schermate 02 e 04).
- *Schermata di fine partita*: *«Draw / Nessun vincitore / 12»* e i due pulsanti *«PLAY AGAIN»* e *«MAIN MENU»* si sovrappongono al pannello dei ruolini in alto a sinistra; *«Draw»*, *«PLAY AGAIN»* e *«MAIN MENU»* sono in inglese, il resto in italiano; la didascalia *«Pareggio - allo scadere dei round / premi R per rigiocare»* è poco contrastata sulla plancia.
- *Elenco delle partite* (§8): sotto la riga nuova, sette righe *«0 turni · interrotta · registrazione incompleta»*.

**Non giudicate**: le voci PIE `PIE-V01-FRONTEND-PLAY`, `-RESULT`, `-PAUSE`, `-REPLAY`.

File: [`pacing_20261026-094554.csv`](../g13/v3/pacing_20261026-094554.csv), [`esiti-seduta-g13-v3-2026-10-26.txt`](../g13/v3/esiti-seduta-g13-v3-2026-10-26.txt), [`estratto-log-seduta-g13-v3-2026-10-26.txt`](../g13/v3/estratto-log-seduta-g13-v3-2026-10-26.txt). Il log intero (17.197.696 byte) resta in `Saved/Logs/g13-seduta.log`, gitignorato.

## 8. `G16` — **non eseguito sul v3**

**Il giro `U60` (cinque passi in una sola apertura dell'Editor) non è stato eseguito.** Ho provato a pilotare l'Editor con lo stesso script di §7: il pannello `Ability / Hero Lab` non si è mai aperto, e ho abbandonato il tentativo senza salvare niente (un clic nel viewport ha selezionato un attore del livello; l'Editor è stato chiuso senza salvare, `git status` pulito, `Error:` del log di Editor al pavimento di 2). Pilotare dall'esterno un'interfaccia da giudicare a occhio non è una misura che posso firmare: **resta una seduta con una persona**.

**Che cosa ho potuto misurare del passo 5** (il difetto del v2), sul pacchetto, **senza** darne il verdetto:

- La partita nuova di §7 compare **in cima** all'elenco *«CRONOLOGIA PARTITE»* del frontend come *«10/26/26, 9:42 AM · Format.Skirmish2v2 · 12 turni · pareggio»*, **non** come *«0 turni · interrotta»*. Nell'indice del pacchetto la riga è `Outcome 3`, `TurnCount 12`, `ReplayComplete true`, uguale al manifest.
- Dall'elenco si apre la schermata del replay: mostra *«2»* e *«Cleanup»* e quattro pulsanti `< TURNO`, `TURNO >`, `< FASE`, `FASE >` che rispondono; **non mostra la plancia** e non ho trovato un pulsante Play o Pausa (Spazio non cambia la schermata); il pulsante per tornare indietro non ha etichetta. Tornando, l'elenco riappare con la riga evidenziata. **Non so se questa schermata sia il comportamento voluto**: non l'ho confrontata con la specifica.
- **L'altra metà della lamentela dell'autore resta**: l'elenco mostra anche sette righe *«0 turni · interrotta · registrazione incompleta»*. Sei corrispondono alle catture delle serie `a` e `b` di §6 (processo terminato a 4000 frame; la serie `c` è posteriore alla schermata): per ciascuna il manifest dice 3 o 4 turni e l'indice 0. La settima (9:37) non ha un manifest e non è attribuita. Le partite interrotte compaiono **per progetto** ([#416](https://github.com/DegrassiAaron/refactor-tactics-main/issues/416)), ma con *«0 turni»* anche quando il manifest ne ha di più (l'indice scrive i turni solo alla chiusura). **Se questo basti a rendere ✅ il passo 5 lo decide l'autore**, che sul v2 lo aveva giudicato ❌ con le parole «nel replay appaiono partite non concluse o non salvate».

I passi 1-4 sul v2 erano ✅ (3 e 4 con una richiesta, [#3860](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3860)); il v3 non cambia nulla che li riguardi, ma non è stato rieseguito. **`G16` è `⏳` sul v3**: la DoD dà a `⏳` il significato di *giro non eseguito*, ed è così. Non è ✅. Non lo chiamo 🔴 perché sul v3 non c'è un passo ❌ misurato; ma l'altra metà della lamentela del passo 5 resta (sopra), quindi **non va letto come *«il difetto è corretto»***: è corretta la riga dell'indice ([#3859](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3859)), non l'elenco delle partite interrotte.

➕ **Dopo, 2026-10-26: l'autore ha scelto la deroga per i passi 1-4** ([`D-545`](../../../decisions/RT_PDR_00_Decision_Log.md), «deroga alla decisioni, continuiamo con gli sviluppi»; lettura in `D-545`): valgono i suoi giudizi sul giro del 2026-10-26 sul v2. **Il passo 5 resta da guardare** e `G16` resta `⏳`. Confrontato con la specifica: [#472](https://github.com/DegrassiAaron/refactor-tactics-main/issues/472) chiede al viewer play e pausa, e nell'asset non sono cablati (nessuna occorrenza di `CallFunc_Play`, `CallFunc_Pause`, `GetCurrentPhaseEntries` in `Content/RT/UI/Framework/WBP_RT_ReplayViewer.uasset`; il pulsante di ritorno non ha etichetta): [#3868](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3868). Quindi il *«non so se sia il comportamento voluto»* qui sopra ha una risposta parziale: Play/Pausa sono richiesti da [#472](https://github.com/DegrassiAaron/refactor-tactics-main/issues/472) (chiusa) e mancano; se la v0.1 li abbia ridotti lo chiede [#3868](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3868).

## 9. `G14`

Misurato **dopo** l'aggiornamento di tutte le celle e di questo documento (il gate allinea anche i loro esiti) **e di nuovo dopo [`D-544`](../../../decisions/RT_PDR_00_Decision_Log.md) e [`D-545`](../../../decisions/RT_PDR_00_Decision_Log.md)**, che hanno cambiato le celle di `G13`, `G9` e `G16`, il banner della DoD e la sequenza di chiusura; sull'albero della PR di `D-545`, basato su `main` `06206c548`. Con i due comandi del criterio, ciascuno con l'`EXIT` letto subito e non dopo una pipe:

- `node tools/radar/doc-coherence.ts --check` → `PASS A1` (284 voci del registro PIE, 116/34/7/127) · `PASS A2` · `PASS A3` · `PASS A4` · `PASS A5` (24 epic, somma CP 124, 124 id unici); exit 0.
- `node tools/radar/doc-links.ts --check` → 8466 link esaminati in 524 documenti, *tutti i percorsi citati risolvono*; exit 0.
- `RefactorTactics.Meta.SatisfiedGatesAgreeWithTheDefinitionOfDone`, rieseguito sulla DoD modificata con `python -X utf8 tools/suite/esegui.py RefactorTactics.Meta --timeout 540 --log <file>` e lo stesso comando per `RefactorTactics.PieSession`: famiglia `Meta` 15 su 15, tutti `Success`; famiglia `PieSession` 38 su 38, tutti `Success`. Comandi, uscite e le due righe del log del test: [`v3/g14-famiglie-meta-piesession.txt`](v3/g14-famiglie-meta-piesession.txt).
- `node tools/radar/pie-verdict-age.ts --check` → exit 1 su `PIE-DEBUG-CONTEXT` (verdetto 2026-09-23, ultima modifica di `RTContextInspector.cpp` 2026-09-24): **già rosso prima di questa tornata** (stessa voce nell'Onda 3) e fuori dai due comandi di `G14`.

⚠️ Il test `Meta` confronta `roadmap-pia.md` §4 con la cella di stato di ciascun gate della DoD §3, che per risultare soddisfatta deve **aprire** con ✅ (`StartsWith`, `RTGateCoherenceTests.cpp`): nel suo log, *«§4 rivendica come soddisfatti: G8, G4»*, e restano ✅. Non giudica le altre celle, nemmeno `G13`, `G9` e `G16`, perché PIA non le rivendica: **non dice nulla sulle ✅ per deroga di `G13` e `G9`**.

## 10. Cosa NON è stato misurato

`G9` (subset `RELEASE-V01` della PIE): **non rieseguito sul v3**, nessuna PIE eseguita in questa tornata; vale per deroga d'autore ([`D-545`](../../../decisions/RT_PDR_00_Decision_Log.md), §11). `G16`: §8, giro **NOT RUN**; passi 1-4 per deroga, passo 5 da guardare (§11). I giudizi percettivi di `G13` sul v3: §7 (non riespressi; l'autore ha scelto la deroga, [`D-544`](../../../decisions/RT_PDR_00_Decision_Log.md)). Nessuna PIE è stata eseguita in questa tornata. **Packaged** per `G2` è la metà di §2; **Performance** è la misura di §6 con le riserve scritte.


## 11. `G9` e `G16` passi 1-4 — per deroga d'autore ([`D-545`](../../../decisions/RT_PDR_00_Decision_Log.md))

Le 17 voci di `G9` valgono sul v3 **coi verdetti scritti nel registro, accettati dall'autore**; i passi 1-4 di `G16` valgono **coi giudizi che l'autore ha già dato** sul giro del v2; il passo 5 di `G16` resta da guardare. La lettura di «deroga alla decisioni» e i suoi limiti sono in `D-545`.

**`G9`, lo stato delle 17 voci a oggi.** Le voci si contano col comando della DoD §3 (riga `G9`) → 17; lo stato si legge dalla quinta cella, scorrendo dalla fine al primo campo che porta un'icona. Esito: **17 ✅ · 0 🟡 · 0 ❌ · 0 ⏳**. Elenco, con la *prima data citata* nella cella di stato — un'euristica di lettura, **non** la data del verdetto — in [`v3/g9-voci-release-v01-2026-10-26.txt`](v3/g9-voci-release-v01-2026-10-26.txt): dal 2026-08-08 al 2026-10-15.

⚠️ **Una correzione al rapporto che l'autore ha letto**: citava «verdetti dal 2026-08-09 al 2026-10-15». Era la data più recente citata in ciascuna riga, non la data dei verdetti.

**Che cosa non è stato fatto, e perché la deroga di `G9` poggia solo sull'autore.** Nessuna voce giocata sul v3. I 17 verdetti sono anteriori anche al v2, e il motivo *«il v3 cambia un solo file»* riguarda l'intervallo v2 → v3: **non copre `G9`**, per cui l'intervallo che conta è verdetto → v3. `git log --first-parent --since=2026-08-08 --oneline 947a783d5 -- Source Content ':(exclude)Source/RefactorTactics/Tests' ':(exclude)Source/RefactorTacticsEditor/Private/Tests' | wc -l` → **984** (con `--since=2026-10-15`: 34). `node tools/radar/pie-verdict-age.ts --check` stampa `copertura: 5 verdetti ✅ su 116 dichiarano cosa osservano · 1 da rileggere` e ha un solo rosso, `PIE-DEBUG-CONTEXT`, fuori dal subset; nel subset un solo verdetto dichiara `osserva:`, `PIE-CAM-START`: il radar non garantisce che le altre 16 non siano scadute.

**La ricerca di parole.** `` awk -F'|' '/^\| \*\*PIE-[A-Za-z0-9.-]*\*\* `RELEASE-V01`/ {s=tolower($2"|"$3"|"$4"|"$5); if (s ~ /rtindex|history|cronologia|replay/) print $2}' docs/technical/test-manuali-pie.md `` → **nessuna riga**; controllo positivo, con `hex` al posto delle parole → 11 righe. Il solo file fuori dai `Tests/` che cambia fra v2 e v3, `RTReplayRecording.cpp`, scrive l'indice delle partite: nessuna delle 17 voci nomina l'indice. Ma `PIE-HEXPLAY-10` e `PIE-V01-MATCHEND` finiscono una partita, e a fine partita gira il codice che il v3 ha cambiato: sul v3 l'unica osservazione di quel percorso sono le partite complete di `G10` e `G13` (§5, §7), senza crash. È una ricerca di parole, non la rilettura delle 17 celle: una ragione, non una prova.

**Riserve che il registro già scrive per le 17 voci** (lette, non giudicate; promemoria in `v3/g9-voci-release-v01-2026-10-26.txt`): `PIE-HEXPLAY-8` è verde per riduzione di criterio ([`D-402`](../../../decisions/RT_PDR_00_Decision_Log.md)), «Non è stata tenuta una seduta nuova»; `PIE-V01-MATCHEND` è «✅ con riserva», osservata una sola delle tre vie di fine partita; `PIE-HEXPLAY-3` e `PIE-HEXPLAY-5` poggiano in parte su test headless; `PIE-HEXPLAY-4` porta una nota del 2026-09-05 ([#2370](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2370)) sul playback.

**`G16`.** Passi 1-4: valgono i giudizi dell'autore sul giro del v2 (passi 1-2 ✅, passo 3 ✅, passo 4 ✅ con una richiesta, [#3860](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3860)); la ✅ dice *giudizio d'autore riportato*, non *giro eseguito sul v3 in una sola apertura*. Passo 5: §8, con la misura sul pacchetto e senza verdetto. `G16` resta `⏳` finché il passo 5 non ha un verdetto dell'autore. ⚠️ **Su quale archivio**: la colonna *«Come si verifica»* di `G16` vuole che l'archivio del passo 5 sia *«quello prodotto dalla partita dei passi 3-4, identificato per `MatchId`»*; con la deroga quella partita non si rigioca, e la misura di §8 è su una partita pilotata dall'assistente (`G13`). **`D-545` non deroga quella clausola**: o l'autore la deroga per il passo 5, guardando la misura di §8, o il passo 5 si giudica su una partita giocata da lui.
