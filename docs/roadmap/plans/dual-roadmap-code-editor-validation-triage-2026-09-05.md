# Dual Roadmap «Code/Architecture + Editor/MCP/Human» — triage del secondo giro

> `CURRENT` · **Stato**: triage chiuso, nessuna roadmap nuova creata, tre gap reali **nominati con il loro owner** · **Data**: 2026-09-05
> **HEAD della misura**: `026850c0` (= `origin/main` al 2026-09-05, dopo `fetch --prune`)
> **Oggetto**: il work order esterno *RefactorTactics — Dual Roadmap: Code/Architecture + Editor/MCP/Human
> Validation*, che chiede di produrre due roadmap nuove più una `EDITOR VALIDATION MATRIX`.
> ⚠️ **Il documento di lavoro non è nel repository e non entra**: era una consegna effimera, consumata a fine
> run. Questo referto è l'unico posto in cui il suo contenuto resta citabile.
> 🔑 **Nessun numero qui è ricordato.** Ogni conteggio porta il comando che l'ha prodotto, in §2.

---

## 1. Il verdetto in una riga

Il work order è **il secondo giro dello stesso contratto in ventiquattro ore** — la sua struttura `A0…A6` /
`B0…B7` è quella che il referto
[`cloud-dual-roadmap-spec-panel-2026-09-04.md`](cloud-dual-roadmap-spec-panel-2026-09-04.md) ha revisionato
ieri e di cui ha salvato il ~15%, consegnato ad `AGENTS.md` §9, a `CLAUDE.md` §5 e a
[`D-330`](../../decisions/RT_PDR_00_Decision_Log.md) — e **la premessa tecnica su cui poggia è caduta la
notte scorsa**: il gap 2-vs-10 celle che dovrebbe alimentare la Roadmap A è chiuso da
[`#2370`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2370), mergiata alle **00:39 del
2026-09-05**, con test, mutazione eseguita e suite `1997/1997`.

| | Rilievo | Gravità |
|---|---|---|
| **R1** | è il **secondo** giro dello stesso contratto di processo, e non nomina il referto che lo ha già revisionato | 🔴 |
| **R2** | la premessa tecnica (**«il gap corrente è la durata per route/cella»**) è **chiusa su `main` da nove ore** al momento della richiesta | 🔴 |
| **R3** | §1 «Precedenza» ripropone la **scala universale** che `D-282` ha scartato per nome | 🔴 |
| **R4** | **13 dei 20** check `PACE-*` sono già automation verde e un quattordicesimo lo è a metà; la matrice ne sarebbe una seconda copia | 🟠 |
| **R5** | le sedute `E1…E5` proposte esistono già come `U44`/`U46`, con `shares_setup_with` che è il campo di batching che il §5 chiede di inventare | 🟠 |
| **R6** | «massima copertura Editor» contro un gate che chiede **4** voci su **216**: il selettore esiste ed è `RELEASE-V01` | 🟠 |
| **R7** | la famiglia di ID `PACE-*` non esiste nel registro, e il registro è l'owner degli ID delle verifiche manuali | 🟡 |
| **R8** | superficie MCP misurata: `StartPIE` esiste, ma `CaptureViewport` non vede PIE — `PACE-04/16/17/18` restano `C` per misura, non per prudenza | 🔵 |
| ~~**R9**~~ | ~~«nessuno scenario del corpus esercita percorsi di lunghezza diversa»~~ — 🔴 **smentito misurando**: sono **6 turni in 3 scenari**, e il migliore vale **4:1**. Vedi §5 | — |

**Cosa si salva, e vale la run**: tre gap reali che nessun owner possiede oggi — §6.

---

## 2. Come è stata misurata

Il checkout di sessione `refactor-tactics-technical-designer/refactor-tactics-main` era **94 commit
indietro** a inizio run. Nessuna citazione qui sotto viene dal working tree: si legge con
`git show origin/main:<path>` dopo `fetch`, e il documento è stato scritto in un worktree pulito su
`026850c0`.

```bash
git fetch --prune origin && git rev-parse origin/main        # 026850c0

# Registro PIE — voci e stato (il glifo è il PRIMO marcatore della cella, non uno qualsiasi:
# con `~ /🟡/` si contano anche gli «Stato precedente:», e vengono 8 invece di 4)
grep -c '^| \*\*PIE-' docs/technical/test-manuali-pie.md                                    # 216
awk -F'|' '/^\| \*\*PIE-/ {s=$(NF-1);
  if (match(s,/✅|🟡|⏳/)) c[substr(s,RSTART,RLENGTH)]++} END
  {printf "verde=%d parziale=%d aperta=%d\n", c["✅"],c["🟡"],c["⏳"]}' \
  docs/technical/test-manuali-pie.md                                    # 78 · 29 · 109

# Subset di release — gate G9, col grep ancorato che scenario-map.md §7 prescrive
grep -c '^| \*\*PIE-[A-Za-z0-9.-]*\*\* `RELEASE-V01`' docs/technical/test-manuali-pie.md    # 17
#   ...e il suo stato, con lo stesso awk ancorato al marcatore:  13 ✅ · 4 🟡 · 0 ⏳

grep -cE '^  - id:' docs/roadmap/editor-sessions.yaml                                       # 46 sedute
grep -cE '^    shares_setup_with: \[[^]]' docs/roadmap/editor-sessions.yaml                 # 25 con batching
find Scenarios -name '*.json' ! -name '_*' | wc -l                                          # 125
grep -c 'PIE-PACE\|\bPACE-[0-9]' docs/technical/test-manuali-pie.md                         # 0

# L'unico conteggio di R4 che non veniva da qui, corretto in review: erano 10, sono 8
git grep -ohE '"RefactorTactics[.]Replay[.]Seek[.][A-Za-z0-9.]+"' -- Source | sort -u | wc -l   # 8

# I banner di questa cartella, per la nota di §8 — letti dai file, mai incrementati
git ls-tree -r --name-only a27e99f5 docs/roadmap/plans/ | grep -c 'md$'                        # 127 (126 + README)
```

✅ **Rimisurato prima del merge, come `AGENTS.md` §11 impone.** `origin/main` è avanzato a `a27e99f5` (sei
commit) mentre il referto veniva scritto: `git diff --stat 026850c0 origin/main` sui file che i conteggi di
questa sezione leggono — registro PIE, `editor-sessions.yaml`, `scenario-map.md`, la DoD, `Scenarios/`,
`RTPlaybackLibrary.h`, `RTTurnManager.cpp` e il Decision Log — **non tocca nessuno di essi**.

🔴 **E la prima stesura di questo controllo era al livello sbagliato, trovato in review.** Enumerava i file
dietro i numeri **di questa sezione** e non quelli dietro *ogni* numero del documento: i sei commit toccano
`docs/roadmap/plans/` (+1 file) e `docs/OPEN_DECISIONS.md` (+13 righe), che sono esattamente ciò che §8 conta
e cita. La conseguenza si era già materializzata — la nota sui banner mescolava due SHA in una frase sola — ed
è corretta lì. ⚠️ **Un gate di rimisura si deriva dai numeri pubblicati, non dalla sezione che li ha
prodotti**: enumerare i secondi esenta in silenzio ogni conteggio introdotto altrove.

⛔ **`origin/main` non compila a `a27e99f5`**, e riguarda ciò che questo referto cita: `void
StandStill(ARTUnit*)` è definita in namespace anonimo **sia** in `RTStatusTests.cpp:97` **sia** in
`RTUnbalancedProneTests.cpp:103` — la collisione di unity build di
[`#2397`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2397) e
[`#2409`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2409), entrambe **OPEN**. ∴ il
`1997/1997` citato al §1 e in §6 è un **fatto storico** del merge di `#2370`, verificabile lì e **non
riproducibile** all'SHA che questo documento dichiara come proprio HEAD. Verificare l'identità dei file non è
verificarne la costruibilità, e dichiararle insieme sarebbe la confusione fra *«file modificato»* e *«file
verificato»* che `AGENTS.md` §12 vieta.

Superficie MCP, misurata **live** sull'unico ponte acceso (`127.0.0.1:8770`, l'Editor di un altro
workflow su `rt-wt-replay`), con `initialize` → `tools/list` → `list_toolsets` → `describe_toolset`:
**56 toolset**, **3** tool di dispatch (`list_toolsets`, `describe_toolset`, `call_tool`). Dettaglio in §4/R8.

⚠️ **Le porte del progetto erano chiuse**: `8765` (config del repository) e `8767` (config della sessione)
non rispondono, e nessun tool `mcp__unreal-mcp__*` è esposto a questa sessione. La misura viene dal ponte di
un altro worker, interrogato in sola lettura: dice **quali toolset l'engine espone**, non che questa
sessione possa chiamarli.

---

## 3. Cosa il work order prende giusto

Verificato, non concesso per cortesia:

| Claim del work order | Esito | Dove |
|---|---|---|
| «il catalogo PIE non è un backlog da rieseguire a ogni modifica» (§0.1) | ✅ | `test-manuali-pie.md`, intestazione: *«QUESTO FILE È UN CATALOGO, NON UN BACKLOG, e l'unico insieme che ha una data è `RELEASE-V01`»* |
| «MCP non equivale a un essere umano» (§0.3) | ✅ | `scenario-map.md` §2: la distinzione `A`/`B` è **dove sta l'oracolo**, non il tipo di file |
| «non creare `rt_run_automation_tests`, cercare il Toolset Epic» (§B1) | ✅ **e il toolset esiste** | `AutomationTestToolset`, 7 tool: `DiscoverTests` · `ListTests` · `RunTests` · `RunTestsByFilter` · `GetTestStatus` · `GetTestResults` · `StopTests` |
| le cinque capability RT da riusare (`ProjectStatus`, `GetCurrentMap`, `DumpCell`, `FindPath`, `ValidateTacticalMap`) | ✅ **esatte** | sono le **5** `UFUNCTION(meta=(AICallable))` di `RTDevToolset.h`, e il ponte ne espone esattamente cinque |
| «non usare un handoff Drive come autorità superiore al repository» | ✅ | è la correzione che il giro precedente non conteneva, ed è nella direzione di `D-282` |
| «raggruppare le verifiche per setup/seduta, non per issue» (§0.5) | ✅ **come principio** | già implementato: `shares_setup_with` in `editor-sessions.yaml`, vedi R5 |

🔑 **E una domanda che nessun owner pone oggi**: *quanto costa, in aperture d'Editor, validare un
cambiamento di presentazione?* Il repository ha il campo per rispondere (`shares_setup_with`) e non ha mai
scritto il conto. È il contributo che §6 raccoglie.

---

## 4. I rilievi

### R1 — 🔴 Il secondo giro dello stesso contratto

Sovrapposizione strutturale col documento revisionato il 2026-09-04:

| Dual Roadmap (oggi) | Dual Track Forge (ieri) | Owner reale |
|---|---|---|
| `A0` Audit e ownership | `A0` *Resolve Owner* | `AGENTS.md` §8 *Prima* · `CLAUDE.md` §1 (10 passi) |
| `A5` Gate automatici | `A3`/`A4` build e suite | `AGENTS.md` §9 *Build e test* |
| `A6` FREEZE GATE · §7 Evidence Pack | `A5` *Evidence Pack* · §9 *Handoff* | `CLAUDE.md` §9 *Output dopo ogni pass* |
| `B0` Preflight Editor | `B1` lifecycle MCP | `CLAUDE.md` §5 (9 passi) · `AGENTS.md` §9 (8 passi) |
| §8 Regole di verdetto | §8 stati finali | `AGENTS.md` §10 — quattordici gate della DoD |

Il work order **ha assorbito** tre correzioni del referto precedente — usa `NOT RUN` invece dei tre sinonimi
coniati (`EVIDENCE_GAP`/`MCP_BLOCKED`/`USER_REQUIRED`), usa `NON VALID` con la semantica di `rt-suite.ps1`, e
cita le quattro classi `A/B/C/D` di `scenario-map.md` invece di inventarne altre. Non nomina però il referto
che quelle correzioni le ha prodotte, e riapre per intero le due parti che erano state respinte: le fasi e il
catalogo.

⚠️ **Non è una critica alla qualità del testo**, che è migliore del precedente. È la constatazione che
riscriverlo una terza volta produce il difetto che il work order stesso vieta al §0.4 — *«Riusa. Non
duplicare»* — sull'asse del processo invece che su quello dei test.

🔑 **E i giri sono tre, non due: il terzo è arrivato mentre questo referto veniva scritto.** `origin/main` è
passato da `026850c0` a `a27e99f5` durante la run, e fra i sei commit c'è
[`#2391`](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2391) —
[`verticalita-ledge-fall-dual-roadmap-2026-09-05.md`](verticalita-ledge-fall-dual-roadmap-2026-09-05.md),
che consuma *«Dual Roadmap: Code/Architecture + Editor/MCP/User — Verticality, Ledge, Fall, Forced
Movement»*, **stesso giorno e stessa forma di work order, oggetto diverso**. ⛔ **I due referti non sono
duplicati** — quello copre la verticalità e ha prodotto un owner di capability (`#2388`), questo copre
playback e validazione Editor — ma insieme dicono che il contratto `A/B` è un **template ricorrente**, non
una proposta una tantum. È la ragione per cui `R1` vale più della singola richiesta: ciò che va deciso una
volta è se il template diventa un owner, non se questo testo entra.

### R2 — 🔴 La premessa tecnica è caduta prima della richiesta

Il §1 dice: *«Il gap corrente discusso è la possibile durata del movimento per singola route/cella: due unità
che percorrono 2 e 10 celle non devono necessariamente essere stirate sulla stessa durata visuale»*, e
l'intera Roadmap A (`A1` repro, `A2` contratto, `A3` implementazione) è costruita su di essa.

Misurato su `origin/main`:

| Fatto | Dove |
|---|---|
| `URTPlaybackLibrary::RouteAlpha(RouteSegments, PhaseElapsed, CellsPerSecond)` esiste | `Turn/RTPlaybackLibrary.h` |
| è applicata per-anim in `TickPlayback`, con il `Blast` escluso **di proposito** | `Turn/RTTurnManager.cpp:7581-7606` |
| commit che l'ha introdotta | `5f33c331` — *fix(2370): in parallelo e' partire insieme, non finire insieme*, **2026-09-05 00:39** |
| issue owner | [`#2370`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2370) — **CLOSED**, PR `#2377` |

E i criteri che la Roadmap A chiede di costruire (`A1` route 2 vs 10, stessa base rate, stesso periodo di
Resolution, prova che oggi il comportamento è errato) **sono già i criteri chiusi di quella issue**, con la
mutazione eseguita e non promessa: ripristinato l'`Alpha` condiviso, il test nuovo cede da solo con
`la corta arriva PRIMA della lunga (tick 138 < 138)`, cioè il difetto riprodotto come numero.

∴ **`A0`–`A5` non hanno un oggetto.** Ciò che resta della Roadmap A è `A6` — il freeze verso l'Editor — e
per quel commit il freeze è già stato pagato.

### R3 — 🔴 La §1 ripropone la scala che `D-282` ha scartato

Il work order elenca sotto il titolo **«Precedenza»**:

`ADR / Decision Log CURRENT → codice + test su main → owner docs CURRENT → roadmap CURRENT → Google Drive
handoff → storico`

[`D-282`](../../decisions/RT_PDR_00_Decision_Log.md) (**Accettata**, 2026-08-30) dice che la precedenza è
**tipizzata per genere di affermazione e relativo owner**, e che *«non esiste una scala universale»*; fra le
alternative scartate c'è per nome *«comporre le formulazioni in una scala unica totale»*, perché
*«ordinerebbe oggetti non confrontabili e produrrebbe risposte sbagliate con sicurezza»*.

⚠️ **Ed è la stessa forma del rilievo `R2` di ieri**, su un testo diverso: la lista in sé è innocua come
*ordine di lettura*, il titolo dice altro, e chi la applica in conflitto fa ciò che `D-282` vieta.

### R4 — 🟠 Tredici dei venti check esistono già e sono verdi, un quattordicesimo a metà

La §B6 chiede una «matrice minima» `PACE-01…PACE-20`. Mappata sul corpus di test di `origin/main`:

| Check richiesto | Copertura esistente | Classe |
|---|---|---|
| `PACE-01` 2 vs 10 celle | `Playback.ShortRouteArrivesBeforeLongRoute` — sul playback vero, non sulla libreria | **A** ✅ |
| `PACE-02` velocità per cella | `Playback.RouteAlphaIsPerRouteNotPerPhase` · `Playback.EffectiveSpeed` · `Playback.DeclaredVelocityFollowsTheVisualMove` | **A** ✅ |
| `PACE-03` completamento anticipato | `RouteAlpha` è `Clamp(...,0,1)`: l'overshoot non è rappresentabile. Più `Playback.StillUnitNeverRuns` | **A** ✅ |
| `PACE-04` niente teleport / slow-motion | — | **C** ⏳ |
| `PACE-05` `OnEnter` al boundary | `Playback.StepExecutesWholeMicroStep` · `Playback.MicroStepCountMatchesSegments` · `Terrain.Fire.DamagesAndBurnsOnEnter` | **A** ✅ |
| `PACE-06` hazard nella cella corretta | ⚠️ **solo sul ramo `Push`**: `Actions.Push.CrossesHazardsOfEveryCell` è scritto su una spinta, non su un `Move` volontario, e il suo commento dichiara che *«la cella d'ARRIVO non brucia: è deliberato»* perché la domanda è sulle **intermedie**. `Terrain.Fire.DamagesAndBurnsOnEnter` pinna l'ingresso, non l'attraversamento. Nessun test nomina *Move* **e** hazard insieme | **A** parziale |
| `PACE-07` reaction / `EventGroup` simultaneo | `Playback.StepDoesNotSplitSimultaneousEvents` · `Overwatch.TriggerReadsMicroStepFacing` | **A** ✅ |
| `PACE-08` collision / contest | `Scenarios/Movement/Collision.json` · `CollisionChoke.json` · `SwapRejectedByPlanning.json` | **A** ✅ |
| `PACE-09` viewer speed | `Playback.InstantEqualsNormalInEverythingButTime` · `Playback.SpeedScaleIsCompleteAndInstantIsNamed` · `Match.Autobattle.DeterminismIsIndependentOfPlayback` | **A** ✅ |
| `PACE-10` hot speed change | **risposto dal codice**: `TickPlayback` rilegge `EffectivePlaybackSpeed(ViewerPlaybackSpeed)` a **ogni tick** (`RTTurnManager.cpp:7555`) e scala solo `Dt`. Non c'è salto logico perché non c'è stato da riapplicare | **A** ✅ (per costruzione) |
| `PACE-11` Pause al safe boundary | `#1879` · `Playback.ControlsAreDeniedByDefault` · `Playback.StepOnAnEmptyPhaseDoesNotAdvance` | **A** ✅ |
| `PACE-12` StepMicroStep | `Playback.StepExecutesWholeMicroStep` — *«non esiste un ingresso che produca mezzo segmento»* | **A** ✅ |
| `PACE-13` Seek ≡ playback | `Replay.State.PlaybackToBoundaryEqualsSeek`, più **8** test `Replay.Seek.*` (comando in §2) | **A** ✅ |
| `PACE-14` Skip resta Skip | `RTAutobattleInputInertTests` (Spazio non chiama `RecordPlanningInput`) · verdetto umano già dato in `PIE-FACING-1` ✅ 2026-08-30 | **A+C** ✅ |
| `PACE-15` multilivello / transizione | `Spec.Map.MatchArenaPlatformClimb` (250,0 di dislivello, misurato sugli Actor) · `Spec.Map.BridgeBreaksThePath` | **A** ✅ |
| `PACE-16` leggibilità camera | — | **C** ⏳ |
| `PACE-17` qualità animazione | `Playback.DefaultRateMatchesTheRunClip` copre la **taratura** (`1.44` = 375 cm/s ÷ 259,8 cm); il foot sliding percepito no | **A**+**C** ⏳ |
| `PACE-18` UI / progress | il numero mostrato è tarato **all'avvio** e diverge dopo un hot change — *dichiarato in codice*, `RTTurnManager.cpp:7400` | **C** ⏳ |
| `PACE-19` ripetibilità | `Replay.Verifier.ResimulationIsDeterministic` (gate `G4`, 100 ripetizioni) | **A** ✅ |
| `PACE-20` stress v0.1 | `Perf.PathfindingMedian` · `Perf.PlanningPreviewMedian` · `Perf.TurnResolverMedian` — ⚠️ **nessuno misura il playback**: coprono resolver e pathfinding | **D** — vedi §6 |

**13 su 20 pienamente in classe `A` e verdi · 1 coperto solo sul ramo `Push` · 5 richiedono un occhio · 1 non
è coperto da niente.**

🔴 **Il conteggio è sceso da 14 a 13 rileggendo le proprie righe**, ed è la forma di errore che questo referto
denuncia altrove: `Actions.Push.CrossesHazardsOfEveryCell` *contiene* la parola giusta e verifica l'oggetto
sbagliato. ⚠️ **Non è promosso a quarto gap**, e la ragione è dichiarata: `AGENTS.md` §5 tiene `Move · Dash ·
Forced` in **una sola famiglia** — Traversal — che *«produce una sequenza di celle attraversate»*, quindi il
meccanismo è condiviso e il ramo `Push` lo esercita. Ciò che manca è la **prova** che il ramo `Move` lo
erediti, non l'implementazione. Vale una riga in un test esistente, non una issue.

⛔ **Scriverli in una matrice nuova li duplicherebbe senza owner**, che è precisamente il rilievo `R10` di
ieri: *«novanta voci ridicono a mano ciò che vive in `test-manuali-pie.md`, `scenario-map.md` ed
`editor-sessions.yaml`, senza una riga che citi l'owner: fra un mese sono false»*.

### R5 — 🟠 Le sedute `E1…E5` esistono già, e il campo di batching pure

La §5 chiede di *«proporre sedute, non una lista piatta»* e ne abbozza cinque. Misurato:
`editor-sessions.yaml` porta **46 sedute**, di cui **25** dichiarano `shares_setup_with` — cioè il campo che
risponde alla domanda *«quali verifiche condividono un allestimento»* senza doverlo ricostruire.

| Seduta proposta | Copre | Esiste già come |
|---|---|---|
| `E1` startup + MCP + map health | handshake, project status, mappa, validazione | i 5 tool `RTDevToolset` sono già la capability; nessuna seduta serve per chiamarli |
| `E2` movement / pacing | `PACE-01/02/03/04`, `09/10/14` | **`U44`** — *«Il playback dello scenario si legge»*, `verifies: [PIE-SCEN-PLAYBACK]`, `unblocked_by: [U21]` |
| `E3` boundary semantics | `PACE-05/06/07`, pause/step, seek | classe **A** per intero (tabella R4): non richiede una seduta |
| `E4` UI / camera / human | leggibilità, foot sliding, comprensione | `U44` per il playback; `PIE-HEXPLAY-4`/`-4b` per lo scorrimento in partita, **già ✅** |
| `E5` release / stress | subset `RELEASE-V01` | **`U46`** — *«I quattro residui del gate G9, in una sola apertura»*, `critical: true` |

`U46` fa esattamente ciò che il §0.5 chiede — *«aprire l'Editor il minor numero di volte possibile»* — e lo
documenta con la misura che lo giustifica: convocare le quattro voci dalle sedute d'origine costerebbe
**quattro aperture** per quattro giudizi.

### R6 — 🟠 «Massima copertura» ha già un selettore, ed è più stretto di quanto sembri

Il §0.1 costruisce la definizione di massima copertura in cinque punti e li lascia senza un criterio
misurabile. Il repository ne ha uno, datato: il gate **`G9`** della
[Definition of Done](../v0.1-definition-of-done.md).

| Insieme | Quante | Stato live su `026850c0` |
|---|---:|---|
| registro PIE | **216** | 78 ✅ · 29 🟡 · 109 ⏳ |
| subset `RELEASE-V01` (`G9`) | **17** | **13 ✅ · 4 🟡 · 0 ⏳** |

Le quattro che separano `G9` dal verde sono `PIE-HEXPLAY-6`, `PIE-HEXPLAY-8`, `PIE-V01-ROSTER`,
`PIE-V01-LOG` — **nessuna aspetta codice**, tutte aspettano un occhio, e tutte stanno in `U46`.

∴ per un cambiamento di presentazione come `#2370`, «massima copertura» misurata vale **quattro voci più il
residuo che §6 apre**, non duecentosedici.

### R7 — 🟡 `PACE-*` non è uno spazio di ID di questo repository

`grep` sul registro: **zero** occorrenze di `PIE-PACE` e **zero** di `PACE-<n>`. Gli ID delle verifiche
manuali li assegna `test-manuali-pie.md`, che ne è l'owner, con la famiglia nel nome (`PIE-HEXPLAY-*`,
`PIE-VIS-*`, `PIE-V01-*`, `PIE-GBX-*`, `PIE-TD-*`). ⚠️ E la parola *pacing* è già occupata in automation da
`RefactorTactics.Pacing.*`, che misura **il tempo di lock-in del giocatore** — telemetria di ritmo, non
locomozione: due famiglie omonime su oggetti diversi si confondono alla prima ricerca.

### R8 — 🔵 La superficie MCP, misurata: `StartPIE` esiste, ma il ponte non vede PIE

Il §B0 chiede di scoprire i tool reali e di non inventarne. Misurato live (§2):

| Toolset | Tool | Cosa dà |
|---|---:|---|
| `RTDeveloperTools.RTDevToolset` | **5** | `ProjectStatus` · `GetCurrentMap` · `DumpCell` · `FindPath` · `ValidateTacticalMap` |
| `AutomationTestToolset` | **7** | `DiscoverTests` · `ListTests` · `RunTests` · `RunTestsByFilter` · `GetTestStatus` · `GetTestResults` · `StopTests` |
| `EditorToolset.EditorAppToolset` | **21** | fra cui `StartPIE` · `StopPIE` · `IsPIERunning` · `CaptureViewport` · `SetCameraTransform` · `GetVisibleActors` · `FocusOnActors` |
| `EditorToolset.LogsToolset` | — | lettura dell'Output Log e verbosità per categoria |
| *altri 52 toolset* | — | Niagara, GAS, Sequencer, UMG, PCG, Slate, StateTree, Blueprint, asset… |

🔴 **E il limite che decide la Roadmap B è già misurato, il 2026-09-04, ed è nelle note di `U46`**:
`CaptureViewport` **renderizza il mondo dell'Editor, non quello di PIE**. La prova è diretta e sta lì:
`find_actors` trova `BP_Unit_Gadget_C_0` a `(-519,-30,-200)` e una cattura centrata esattamente su quel
punto mostra la cella e **non l'unità**. Via MCP si giudicano mappa, celle e quote; **non** unità, HUD, VFX
né playback.

∴ il §B3 — *«l'MCP deve preparare quanto possibile»* — è vero per l'allestimento (aprire la mappa, lanciare
lo scenario, avviare PIE, raccogliere il log) e **falso per l'osservazione**: su `PACE-04`, `-16`, `-17`,
`-18` la macchina non può nemmeno consegnare il fotogramma. Restano `C`, e non per prudenza: per misura.

⚠️ **Questa sessione non può chiamare nessuno di quei tool.** `8765` e `8767` sono chiuse e nessun
`mcp__unreal-mcp__*` è esposto; la lista viene dal ponte di un altro workflow, interrogato in sola lettura.
Per questa run: `NOT RUN` su ogni esecuzione MCP, con il motivo.

---

## 5. Il rilievo che si è corretto misurando

Il commento di chiusura di `#2370` dichiara, fra i residui:

> *«Nessuno scenario del corpus esercita percorsi di lunghezza diversa — `Movement.LongWalk` ne ha due da
> 3 celle ciascuno, quindi è proprio il caso che non copre.»*

La prima metà è vera: `Movement.LongWalk` muove `A1` da `(-4,0)` a `(-1,0)` e `B1` da `(4,-2)` a `(1,-2)`,
**3 celle ciascuno**, per due turni. La seconda — la generalizzazione al corpus — **è falsa**.

Misurato su tutti i **125** scenari, sommando la distanza esagonale fra i waypoint dichiarati di ogni
intento `move` e confrontando le lunghezze **dentro lo stesso turno**:

| Scenario | Turno | Route | Rapporto |
|---|---:|---|---:|
| **`RT_Showcase_Relay_v01`** | **1** | Wraith **4** · Gadget 1 · Phase 1 · Riktor 1 | **4:1** |
| `Spec/Overwatch/HoldThenFire` | 2 | R1 **3** · F1 1 | 3:1 |
| `RT_Showcase_Relay_v01` | 4 · 5 · 8 | 2 contro 1 | 2:1 |
| `Spec/Predictive/WhiffOnEmptyCell` | 1 | V1 2 · R1 1 | 2:1 |

**Sei turni in tre scenari**, non zero.

🔑 **E il correttivo cambia la conclusione operativa, non solo il numero.** Prima di `#2370` la velocità
visuale di un'unità con `S` segmenti in una fase da `M` valeva `CellsPerSecond × S/M`: sul turno 1 di
`RT_Showcase_Relay_v01`, con `PlaybackCellsPerSecond = 1.44`, i tre movers da 1 cella andavano a **0,36
celle/s** — quattro volte sotto il rate dichiarato — e restavano in moto per **2,8 s** invece di **0,7 s**,
aspettando il Wraith. È un divario percepibile, su **quattro** unità contemporaneamente, in uno scenario di
classe **A** già versionato e verde.

∴ **il banco per il giudizio umano del caso asimmetrico esiste già.** Non serve scrivere uno scenario nuovo,
che è ciò che una lettura letterale del residuo di `#2370` avrebbe fatto fare.

⚠️ **Due limiti dichiarati della misura, e il secondo è stato trovato in review.**

1. La distanza esagonale fra waypoint **ignora ostacoli e costi di terreno**, quindi è un **minimo**: la
   route reale può essere più lunga, mai più corta. Il rapporto 4:1 è un limite inferiore.
2. La battuta conta i soli intenti **`move`**, e non i **`dash`** — `10` file del corpus ne portano. Non è
   innocuo per costruzione: `TickPlayback` applica `RouteAlpha` a `Dash` **e** `Move` nello stesso ramo
   (`RTTurnManager.cpp:7576`), quindi il difetto vive anche lì. ✅ **Verificato che la conclusione non
   cambia**: nessun turno del corpus mette due unità in `Dash` nello stesso turno, quindi non esiste
   asimmetria di fase `Dash` da guardare. Chi rieseguisse la battuta su un corpus cresciuto di un secondo
   *dasher* otterrebbe un risultato diverso da quello che il metodo dichiarato lascia attendere: **il
   filtro va allargato prima, non il numero riletto dopo.**

---

## 6. Cosa resta davvero da fare

Tre gap, e nessuno dei tre è una roadmap.

⛔ **Nessuno dei tre è ancora arrivato al proprio owner, e va detto qui invece che nell'intestazione.** Il
write-set di questo pass è **questo file più la voce `GOV-4`**: `test-manuali-pie.md` ed `editor-sessions.yaml`
non sono toccati, `#801` non ha un commento nuovo, e per `G-1` non esiste una issue. 🔴 **È la stessa forma del
difetto che `G-1` denuncia** — *«sotto quella issue oggi non è un posto dove qualcuno guarderà»* — un livello
più su: un documento di `plans/` **non è un owner**, e questa cartella lo dichiara di sé nel proprio
[`README.md`](README.md). ∴ finché la riga nel registro e il commento a `#801` non sono scritti, i tre gap sono
**nominati**, non consegnati — ed è per questo che l'intestazione dice *«nominati con il loro owner»*.

### G-1 — Il giudizio umano sul caso asimmetrico non ha una casa

`PIE-HEXPLAY-4` porta già la nota del 2026-09-05 e dichiara la regola giusta: *«la seduta del 2026-08-29 non
ha interrogato il caso asimmetrico, e quel caso è ciò che `#2370` cambia a schermo: chi lo vuole giudicare
apre un'osservazione nuova, sotto quella issue, non riapre questa»*.

⚠️ **Ma `#2370` è `CLOSED`**, quindi «sotto quella issue» oggi non è un posto dove qualcuno guarderà. E la
regola di `AGENTS.md` §9 è netta: se la verifica **ha** una voce `PIE-*`, il verdetto va nel registro. Qui la
voce non esiste — `PIE-HEXPLAY-4` ha il proprio criterio, vero prima e dopo, e non è quello.

**Cosa serve**: una voce nuova nel registro (famiglia `PIE-HEXPLAY-*`, il banco è la partita/lo scenario) con
criterio falsificabile, e la sua convocazione in `U44` — che è la seduta del playback e ha già
`unblocked_by: [U21]` per la ragione delle luci. Il banco è `RT_Showcase_Relay_v01` turno 1 (§5).

⛔ **Non una issue per test**: il §9 del work order lo vieta e il repository pure. Una sola issue
implementativa, oppure la voce nel registro più la riga nella seduta.

### G-2 — `PACE-20`: nessun budget di performance misura il playback

I tre `Perf.*` esistenti misurano **pathfinding**, **anteprima di pianificazione** e **resolver**. Il
playback — `TickPlayback` con N unità, route di lunghezza diversa, `MoveAnims` scandito a ogni tick — non ha
un budget. `RouteAlpha` aggiunge una divisione per anim e per tick: è trascurabile in teoria e **non
misurato**, che è la distinzione che `A4` del work order chiede di rispettare (*«non ottimizzare senza
misura»* vale anche al contrario: non dichiarare senza misura).

**Owner esistente**: [`#801`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/801) — *CP 43.5 ·
Suite dei budget di performance, misurata su packaged*, **OPEN**. Il gap va lì, non in una issue nuova.

### G-3 — Il costo in aperture d'Editor non è scritto da nessuna parte

`shares_setup_with` dichiara **quali** sedute condividono un allestimento su 25 record. Nessuno somma
**quante aperture** costa un perimetro di validazione. Per il caso corrente il conto è:

| Apertura | Seduta | Copre |
|---|---|---|
| **1** | `U46` | i 4 residui `G9` + le 2 voci `Visual.*` orfane — `critical: true` |
| **2** | `U44` | leggibilità del playback + **G-1**, se la voce nuova ci viene convocata |

**Due aperture**, non cinque. ⚠️ È un conto per *questo* perimetro, non una metrica da versionare: scriverlo
come tabella permanente ricreerebbe la vista che `D-181` ha rimosso.

### Tabella issue / owner / dipendenze

| Work item | Owner esistente | Issue nuova? | Dipendenze | Roadmap | Gate |
|---|---|---|---|---|---|
| pacing per cella (2 vs 10) | `#2370` **CLOSED**, PR `#2377` | **no** | — | A — **chiusa** | suite `1997/1997`, mutazione eseguita |
| giudizio asimmetrico a schermo (**G-1**) | nessuno — `PIE-HEXPLAY-4` lo esclude per iscritto | **sì, una sola** | `U21` (luci), banco `RT_Showcase_Relay_v01` | B | fuori `G9` |
| budget playback (**G-2**) | `#801` **OPEN** | **no** — aggiornare | `#801` | A | `G11` (KPI) |
| 4 residui `G9` | `U46`, `critical: true` | **no** | nessuna: aspettano un occhio | B | **`G9`** |
| leggibilità playback | `U44`, `PIE-SCEN-PLAYBACK` | **no** | `U21` | B | fuori `G9` |
| `Blast` su `Alpha` di fase | dichiarato deliberato in `RTTurnManager.cpp:7588-7590` e in `#2370` | **no** — è una decisione separata | — | A | — |

---

## 7. `EDITOR VALIDATION MATRIX` — solo il residuo

Il work order chiede la matrice al §2/Deliverable C, e vieta al §0.4 di duplicare. Le due cose stanno insieme
solo se la matrice copre **ciò che non ha già un owner**: le 14 righe di classe `A` della tabella `R4` vivono
nei loro test, e le quattro di `G9` in `U46`.

| ID | Area | Rischio | Classe | Metodo | Tool / scenario | Setup | Expected | Evidence | Gate | Stato |
|---|---|---|---|---|---|---|---|---|---|---|
| **G-1** | playback / pacing | il corto che arriva prima e aspetta si legge come *impuntato* invece che come *arrivato* | **C** | occhio, in PIE | `RT_Showcase_Relay_v01` turno 1 (4:1, quattro unità) | `U44` — dopo `U21` | tre unità concludono e restano ferme mentre il Wraith prosegue; nessuna riparte, nessun drift, nessun overshoot | verdetto nel registro PIE | fuori `G9` | `NOT RUN` |
| **G-2** | performance | `RouteAlpha` per-anim per-tick non è misurata | **D** | budget suite | `#801` | ⚠️ **packaged, non `U44`** | il playback resta dentro un budget dichiarato | numero in `#801` | `G11` | `NOT RUN` |
| `PACE-04` | playback / pacing | teletrasporto o slow-motion artificiale percepiti | **C** | occhio, in PIE | stesso Play di **G-1** | `U44` | il movimento si legge come scorrimento continuo a ogni lunghezza di route; nessuno salta sulla destinazione, nessuno rallenta senza causa | verdetto nel registro PIE | fuori `G9` | `NOT RUN` |
| `PACE-16` | camera | route molto diverse rendono illeggibile uno dei due mover | **C** | occhio | stesso Play di **G-1** | `U44` | entrambi restano inquadrati | verdetto | fuori `G9` | `NOT RUN` |
| `PACE-17` | animazione | foot sliding fuori dalla taratura `1.44` | **B** | `Playback.DefaultRateMatchesTheRunClip` prepara il numero, l'occhio giudica | stesso Play di **G-1** | `U44` | nessuno scivolamento a nessuna delle lunghezze | verdetto | fuori `G9` | `NOT RUN` |
| `PACE-18` | UI | la stima è tarata all'avvio e diverge dopo un hot speed change | **C** | occhio | stesso Play di **G-1**, cambiando velocità a metà | `U44` | la barra non suggerisce che il mover corto stia ancora muovendo | verdetto | fuori `G9` | `NOT RUN` |

🔑 **Cinque righe in una sola apertura, più una che non è un'apertura d'Editor** — è l'obiettivo del §0.5
applicato invece che enunciato. ⚠️ **`G-2` sta a parte, e la riga di sintesi lo diceva sbagliato fino alla
review**: gira su **packaged** e risponde a `G11`, quindi non condivide l'allestimento `U44` con le altre
cinque. Sommarlo avrebbe gonfiato proprio la metrica che `G-3` introduce.

Le quattro `PACE-*` qui sopra **non sono ID nuovi**: sono i nomi del work order tenuti solo per tracciabilità
di questo referto, e diventano voci `PIE-*` — con gli ID che il registro assegna — quando qualcuno le esegue.

---

## 8. Cosa non è stato consegnato, e perché

- **Le due roadmap `A` e `B` come documenti nuovi**: sono `AGENTS.md` §8–§10 più `editor-sessions.yaml` con
  altri nomi (`R1`, `R5`), e la Roadmap A non ha un oggetto (`R2`).
- **La matrice completa `PACE-01…20`**: 14 righe sarebbero copie senza owner di test verdi (`R4`), che è il
  difetto che `D-181` ha pagato per rimuovere.
- **Una issue per ciascun check**: vietato dal §9 del work order e dal repository. Una sola, per **G-1**.
- **Le sedute `E1…E5`**: `U44` e `U46` le coprono, con il batching già dichiarato (`R5`).
- ~~**Una voce in `OPEN_DECISIONS.md`**~~ — 🔴 **la ragione scritta qui era falsa, trovata in review.**
  Diceva *«nessuna decisione è aperta: `G-1` aspetta un'esecuzione, non una scelta»*, ed è vero per `G-1` e
  **falso per `R1`**, che è il rilievo 🔴 di questo referto e si chiude con *«ciò che va deciso una volta è se
  il template diventa un owner»* — cioè una scelta, che aspetta una persona, e che non ha né issue né voce di
  registro. Il referto gemello di [`#2391`](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2391)
  ha usato lo stesso meccanismo lo stesso giorno (`REL-3`). ✅ **Aperta come
  [`GOV-4`](../../OPEN_DECISIONS.md)**, con le due uscite e il loro costo: `R1` sarebbe altrimenti morto col
  referto, mentre la tesi del referto è che un template ricorrente ha bisogno di un owner.
- **Un tool MCP nuovo**: i sei criteri del §0.4 non sono soddisfatti da nessun candidato. `AutomationTestToolset`
  e `EditorAppToolset` coprono esecuzione e allestimento; ciò che manca (`CaptureViewport` su PIE) è un limite
  dell'engine, non un tool da scrivere qui.
- **Il blocco `RT_PIANI_BANNER` di [`README.md`](README.md)**, che questa cartella prescriverebbe di
  rimisurare a ogni file aggiunto. 🔴 **È stato misurato e non riscritto, di proposito.** Tutto su
  **`a27e99f5`**, escluso questo referto: il blocco dichiara **69** documenti, la cartella ne contiene
  **126** — deriva di **57**, contro le 8 che la nota del 2026-08-30 registrava — e la scomposizione è
  `CURRENT 87 · SNAPSHOT 9 · nessun banner 29 · PLAN 1`, **che somma a 126**. L'archivio è a **44**, non 43.
  🔴 **La prima stesura di questa nota diceva «125» accanto a una scomposizione che ne sommava 126**, perché
  il totale era preso a `026850c0` e gli addendi a `a27e99f5`: due SHA in una frase sola, nel documento la
  cui intestazione promette che nessun numero è ricordato. Trovato in review e ancorato a un SHA solo.
  ⛔ **Resta non riscritto** perché rifarlo richiede la regola di classificazione dei banner, che il README
  stesso dichiara ambigua (*«i vocabolari sono tre, non due»*): i **29** senza banner sono troppi per essere
  pubblicati senza leggere i file uno a uno. Rimisurarli è un lavoro proprio, non un effetto collaterale di
  questo pass — e dichiararlo qui è meno dannoso che mettere un numero indifendibile nell'indice che tutti
  leggono per primo.

## 9. La tensione che resta aperta

`RouteAlpha` non è applicata al `Blast`, e la ragione — scritta in `RTTurnManager.cpp:7588-7590`, il commento
sopra l'esclusione che sta a `7592` — regge: lì
`PhaseDuration` vale `Max(colpi, spinta)` e non `MaxSeg / rate`, quindi la spinta del knockback si distende
**di proposito** sulla finestra dei colpi. ⚠️ Ma è la stessa forma del difetto che `#2370` ha corretto — una
durata decisa da altro che governa una velocità visuale — e la differenza è che lì è **voluta**. Chi la
riaprirà deve farlo come decisione con la propria evidenza, non come coerenza dedotta: è esattamente ciò che
`#2370` dichiara e che questo referto non tocca.
