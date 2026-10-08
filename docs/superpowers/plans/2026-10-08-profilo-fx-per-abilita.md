# Il profilo FX per abilità — attivazione e colpo, una tabella `ActionId → profilo` sopra la forma — piano di implementazione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** ogni `AbilityActivated` disegna sulla cella della sorgente il segnale di attivazione del suo profilo (`Ring`, `Pulse`, `Flash`); ogni `Attack` disegna all'arrivo il `Marker` sulla vittima e, una volta per impronta, la cue d'impronta (`AreaPulse` sul centro, `ConeSweep` lungo l'asse); il tracer degli attacchi diventa **un caso** del profilo (`Zigzag` per `Hero.Aevik.LinearDischarge`, `None` per `Ram`/`PassingBlade`/`Action.Charge`) con il **volo deciso dalla sola forma di default** (R13) e nessun profilo per gli attacchi legacy senza id (R12). Tutto con primitive del line batcher (D-467), nessun asset (D-124), dichiarato in D-278.

**Architecture:** il profilo (`FRTAbilityFxProfile`, quattro enum) vive accanto al tracer in `Map/RTPlaybackTracer.h`; la tabella (default per forma, righe d'override approvate in D5, ripiego `ActionId → BaseActionId → forma`) è un gruppo di funzioni statiche C++ in `URTPresentationBindingLibrary`. La matematica è pura in `URTPlaybackLibrary`: idoneità al volo, stile del tracer, `TracerPolyline` per lo `Zigzag`, le durate con i tetti di R2, le cue per evento con i loro verdetti, l'associazione impronta → primo colpo (`FootprintFxForSequence`, R14) e la geometria (`CueSegments`). `ARTTurnManager::PushPlaybackCues(Phase)`, gemella di `PushPlaybackTracers`, consegna in blocco a un canale nuovo di `ARTHexMapActor` (`SetPlaybackCues`/`ClearPlaybackCues`/`GetPlaybackCues`), che disegna con `DisegnaLineaAnteprima`. Il produttore cambia in un punto solo: `EmitAbilityActivated` scrive `Ev.Origin`.

**Tech Stack:** Unreal Engine 5.8.1, C++ (modulo `RefactorTactics`), Automation Test framework, scenari JSON in `Scenarios/`, registro PIE in Markdown, sedute in YAML.

**Spec:** `docs/superpowers/specs/2026-10-08-profilo-fx-per-abilita-design.md` (rivista dal panel, `➕ rev.`, e dalla re-review, `➕ rev2.`). Decisioni D1–D5, `Ruling` R1–R14 e le revisioni sono chiusi: non si riaprono qui. Dove il piano se ne discosta lo dice con `➕ piano.` e la ragione. Le mutazioni (1)–(22) sono quelle di spec §5.1; quelle che il piano aggiunge sono (P1)–(P5).

**Stato misurato in pianificazione:** checkout `D:/Repositories/rt-wt-sp4-fx`, detached su `origin/main` = `ef49b454d`. Alla review `origin/main` era avanzato a `210f056ac` (solo `RTPlanPreview`): il Task 0 parte dall'`origin/main` del momento, e le citazioni restano valide su `210f056ac`. Ogni `file:riga` è stato letto su quel commit; chi esegue più tardi lo **rimisura** prima di toccare il file (`grep -n` sull'ancora testuale citata accanto al numero). Percorsi del codice relativi a `Source/RefactorTactics/`.

## Global Constraints

- Engine: **UE 5.8.1** in `D:/EpicGames/UE_5.8`. Nessun aggiornamento di Engine, plugin o dipendenze.
- ⛔ **Nessun `.uasset`**, nessun Blueprint, nessun Niagara/Cascade (D-124), nessun authoring nel clone principale. Gli FX sono linee del line batcher.
- ⛔ **Nessun `DrawDebug*`** (D-467): ogni linea passa da `DisegnaLineaAnteprima` (`Map/RTHexMapActor.cpp:1422-1435`), che tace sul server dedicato.
- ⛔ **Nessuna `UFUNCTION` nuova né rinominata, nessuna `UPROPERTY` nuova su `ARTUnit`**: `Unit.BlueprintSurfaceIsCensused` (`Tests/RTUnitBlueprintSurfaceTests.cpp:317`) non cambia. Le funzioni nuove di `URTPresentationBindingLibrary` e `URTPlaybackLibrary` sono **statiche C++ non `UFUNCTION`**, come `TracerSegment` (`Turn/RTPlaybackLibrary.h:238`). Le due manopole nuove sono `UPROPERTY` di `ARTTurnManager` (R2), non di `ARTUnit`.
- ⛔ **`Play*Montage` invariati** e nessuna clip toccata: le cue sono funzione dell'orologio del playback (`PlaybackPhaseElapsed`) e del cursore (`BlastBeatsDone` nel Blast, `ActivationsShown` in Prep e Dash), **mai** di un anim notify né della durata di una clip (R6).
- ⛔ **Nessun dato nuovo in snapshot, TurnLog, `StateHash` o replay**, nessun tipo di evento nuovo. L'unico campo scritto in più è `Ev.Origin` di `AbilityActivated`, su `FRTResolvedEvent` che è `RTServerOnly` (`Turn/RTResolvedEvent.h:241`): `Determinism.FxFieldsStayOutOfHashes` (Task 3) lo **dimostra** con lo stesso metodo di `Determinism.HitGeometryStaysOutOfHashes` (`Tests/RTAttackTracerTests.cpp:214`).
- ⛔ **Il canale è la geometria, mai il solo colore** (#2453): ogni cue usa `URTOverlayPalette::ColorFor(ERTOverlayMeaning::Attack)` (R1, nessun significato nuovo) e gli stili si separano per numero di segmenti, distanza dall'ancora o estensione verticale — pinnato da `Fx.CueStylesDifferByGeometry`.
- **Un evento → un segnale**: `Activation` da `AbilityActivated`, `Tracer` e `Impact` da ogni `Attack`, `Footprint` dall'`AttackFootprint` del suo atto, consegnata dal primo `Attack` che la segue (spec §2.1, tabella dei campi).
- **Privacy fail-closed** (`CLAUDE.md` §7): cue d'attivazione dietro `SourceVerdict` (R7), `Marker` dietro `ImpactVerdict`, tracer dietro `FromVerdict ∧ ImpactVerdict` (invariato), `AreaPulse`/`ConeSweep` dietro `FromVerdict` del colpo. Il ritmo (volo) non legge chi guarda: **stesso volo a parità di indice nella sequenza** (➕ rev2.; l'indice dipende dalle attivazioni che chi guarda vede, `Turn/RTTurnManager.cpp:7952-7956`).
- **Regressione zero sul tracer degli attacchi base** (D5/R3): pinnata come **regola** da `Playback.BasicAttackTracersEqualShapeDefault` (Task 2) e come istanza da `Playback.BasicAttackTracerIsUnchanged`.
- **Ordinamento solo esplicito**: nessun esito dipende dall'ordine di iterazione di un `TMap`/`TSet`. Le mappe nuove (`DeclaredFxOverrides`, le impronte aperte di `FootprintFxForSequence`) si usano solo con `Find`/`Add`/`Remove`; l'ordine delle righe è quello di `DeclaredFxOverrideRows()`.
- ⛔ **Nessun totale volatile** in commenti, documenti, celle del registro PIE, YAML, issue, PR (`AGENTS.md` §14): niente «N righe d'override», «N abilità». I conteggi di un test restano nel test; le misure di un passaggio si scrivono col comando che le produce.
- ⛔ **Nessun glifo di stato (⏳ ✅ ❌ 🟡) accanto al nome di una voce PIE in ciò che si pubblica su GitHub**: issue, titolo e corpo della PR, commenti, messaggi di commit. Lo stato si scrive **in parole** (`PIE: NOT RUN`). Il glifo resta solo nella cella di stato del registro `test-manuali-pie.md` (Task 7).
- Stile: commenti in **italiano**, nella forma dei file toccati (🔴 ⚠️ 🔑 ⛔ ✅ ⏱️ dove il file li usa). Nomi dei test nelle famiglie esistenti `RefactorTactics.<Famiglia>.<Nome>`: `Playback`/`Privacy` (`Tests/RTPlaybackLibraryTests.cpp`, `Tests/RTPlaybackActivationTests.cpp`, `Tests/RTAttackTracerPlaybackTests.cpp`), `Determinism` (`Tests/RTAttackTracerTests.cpp`), `HexMapActor` (`Tests/RTHexMapActorTests.cpp`), `Preview` (`Tests/RTPreviewLineBatcherTests.cpp`), `Presentation` (`Tests/RTPresentationBindingTests.cpp`), più `Fx` (nuova, spec §2.8) per le funzioni pure del profilo nel file nuovo `Tests/RTAbilityFxProfileTests.cpp`. Helper dei test con nomi **distinti per file** (unity build). ➕ piano. Il brief proponeva nomi `Presentation.FxProfile…`/`Playback.CuesAreAFunctionOfTheClock`: vincono i nomi di spec §5.1, che è l'autorità.
- Line ending: sorgenti, Markdown e YAML toccati sono **CRLF** (misurato con `file`: `Turn/RTPlaybackLibrary.cpp`, `docs/technical/test-manuali-pie.md`, `docs/roadmap/editor-sessions.yaml`, `docs/technical/architecture/capability-map.yaml`). I file nuovi (`Tests/RTAbilityFxProfileTests.cpp`) nascono CRLF; lo scenario JSON nuovo segue `Scenarios/Visual/Ability/CastBeat.json` (`file` → `JSON text data`, misuralo e rispettalo). Dopo ogni modifica: `file <percorso>` dice la stessa cosa di prima. ⛔ Niente `sed -i` sui file CRLF.
- Commit: `<type>(3578): <descrizione>`, chiuso da `Co-Authored-By: Claude <modello> <noreply@anthropic.com>`. Un commit per task. ⚠️ **Il trailer nomina il modello che ESEGUE il task**: l'implementatore sostituisce `<modello>` con il proprio in ogni blocco.
- Branch: `issue/3578-profilo-fx-per-abilita`, da `origin/main` (Task 0). `3578` è il numero della issue aperta nel Task 0 sotto #2453, e si sostituisce ovunque compaia (codice, commenti, documenti).
- `<checkout>` = la cartella in cui sta il branch. `<scratchpad>` = la cartella scratchpad della sessione.
- **Motore uno per macchina** (`CLAUDE.md` §10, `AGENTS.md` §11): prima di ogni build o run, `Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, Name, CommandLine`. Se un altro clone misura **tempi**, si aspetta (e qui la suite di playback **è** una misura di tempi: i test di ritmo dipendono dall'orologio); altrimenti si procede. Se non si può aspettare: `NOT RUN` col nome del clone che tiene il motore.
- Build (Editor chiuso su questo clone):
  ```powershell
  & "D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -Project="<checkout>/RefactorTactics.uproject" -WaitMutex -NoHotReloadFromIDE
  ```
- Test (filtro dopo `RunTests`, `+` fra più filtri, `;Quit` separato, `-abslog` **fra virgolette**):
  ```powershell
  & "D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<checkout>/RefactorTactics.uproject" "-ExecCmds=Automation RunTests <filtro>;Quit" -unattended -nopause -nosplash -nullrhi -NoLiveCoding "-abslog=<scratchpad>/<nome-parlante>.log"
  ```
  ⚠️ `-NoLiveCoding` resta identico al piano SP3 perché è l'opzione che le suite di questo repository usano; per l'Editor l'opzione propria è `-LiveCoding=false` (`-NoLiveCoding` è di UBT). Cambiarla è una decisione a parte, per tutti i piani insieme.
  Un esito vale **solo** se il log porta `**** TEST COMPLETE`. Conteggio (Git Bash): `grep -c '\*\*\*\* TEST COMPLETE' <log>` (atteso 1), `grep -c 'Result={Success}' <log>`, `grep -c 'Result={Fail}' <log>` (atteso 0). Un rosso si legge con `grep -n 'Result={Fail}' -A3 <log>`.
- **Una mutazione** è: modifica della riga indicata, build, test indicato, `Result={Fail}` sull'asserto indicato, ripristino **dopo il commit** del task (`git checkout -- <file>`, `git diff` vuoto), build, verde. Se una mutazione **non** fa cadere il test, ci si ferma: il test è vacuo, e si riporta — non si allenta l'asserto. Dove una mutazione fa cadere **più** test del dichiarato, lo step li elenca tutti (➕ rev2.) e la PR li riporta.
- **Le quattro trappole delle fixture di playback** (memoria di progetto), e come le tratta questo piano:
  1. **Il viewer è la squadra 0 senza `World->InitializeActorsForPlay(FURL())`**: `GetPlayerController(this, 0)` è nullo e `TeamIdOf` ripiega su 0. I test con viewer ≠ 0 usano **solo** `CostruisciBeat` (`Tests/RTPlaybackActivationTests.cpp:63-107`), che inizializza il mondo e **asserisce** il viewer letto (`:91-98`); i test con viewer 0 usano il ripiego e lo dicono.
  2. **`StableUnitId` vale 0 prima del turno**: gli eventi della timeline si cercano per `SourceStableUnitId == Unit->StableUnitId` **dopo** `LockInAndResolve()`, mai prima.
  3. **Lo StateHash pendente è 0 senza registrazione**: `Determinism.FxFieldsStayOutOfHashes` calcola lo hash con `HashMatchState` + `BuildUnitDigests`, come `Tests/RTAttackTracerTests.cpp:243-252`, e asserisce lo hash non nullo.
  4. **La finestra di reazione si apre solo con un delegate legato** (`OnReactionWindowOpened.IsBound()`): nessuna fixture di questo piano apre finestre; la misura di `ReactiveCapacitor` (Task 4 Step 1) lo tiene presente e non conclude «nessun `Attack`» da un turno in cui la reazione si è decisa da sola.

## Review Focus

Cinque ingressi che la spec implica e che i test di §5.1 non esercitano da soli. Ciascuno ha il suo test, **scritto per intero** nello Step 1 del task che possiede il codice; chi rivede controlla che ci sia e che sia rosso prima del codice:

1. **(a) Viewer che non vede la sorgente** → nessuna cue d'attivazione su nessun tick, e la riga `Attiva:` resta congelata sul verdetto (assente per quella squadra) → Task 3 Step 1, `Privacy.HiddenSourceDeliversNoActivationCue` (controllo positivo col viewer 0; la funzione pura è difesa da `Privacy.ActivationCueNeedsTheSource`, mutazione (8)).
2. **(b) `Attack` con `HitGeometry.bResolved == false`** → nessun tracer né `Marker` né cue d'impronta, e la clip di ruolo/azione suona comunque → Task 4 Step 1, `Playback.UnresolvedGeometryHasNoFxButPlaysTheClip` (mutazione (P4)).
3. **(c) Seek diretto al battito X == playback lineare fino a X** (stesse cue sulla mappa, `Alpha` entro `1e-3`) → Task 4 Step 1, `Playback.FxCuesAreAFunctionOfTheClock` (mutazione (14)).
4. **(d) `Area` con `AimCell` non vista dal viewer** → nessun `AreaPulse`, nessuna rivelazione del centro → Task 4 Step 1, `Privacy.AreaPulseNeedsTheCenter` (mutazione (P3)).
5. **(e) Profilo per `ActionId` assente ma `BaseActionId` presente** → il profilo della generica (`Action.Charge`: `Ring · None · Marker · None`), non il default della forma → Task 1 Step 1, `Fx.BaseActionOverrideWinsOverShapeDefault` (mutazione (3)).

### Dove si eseguono le mutazioni

| Mutazione | Task / Step | Test che cade |
|---|---|---|
| (1) riga `Line` → `Projectile` in `DefaultFxProfileFor` | Task 1 Step 6 | `Fx.DefaultProfileFollowsShape`; dal Task 2 anche `Playback.TracerStyleFollowsShapeForBasicAttack` (`:428-429`), ripetuta al Task 2 Step 7 |
| (2) forma letta prima di `ActionId` | Task 1 Step 6 | `Fx.ProfileFallsBackActionThenBaseThenShape` |
| (3) livello `BaseActionId` saltato | Task 1 Step 6 | `Fx.ProfileFallsBackActionThenBaseThenShape`, `Fx.BaseActionOverrideWinsOverShapeDefault` |
| (4) riga `TideGuard` tolta | Task 1 Step 6 | `Fx.DeclaredOverridesMatchTheProposal` |
| (18) guardia R12 tolta da `FxProfileForIn` | Task 1 Step 6 | `Fx.NoActionNoProfile` («profilo tutto `None`») |
| (P1) riga duplicata | Task 1 Step 6 | `Fx.DeclaredOverridesHaveNoDuplicateKeys` |
| (5) vecchio corpo di `IsTracerEligible` | Task 2 Step 7 | `Playback.TracerFollowsTheProfile`, `Privacy.FlightDependsOnShapeNotOverride` |
| (6) override `Zigzag` su `PressureJet` (righe **e** copia) | Task 2 Step 7 | `Playback.BasicAttackTracersEqualShapeDefault`; (4) resta verde |
| (17) volo dall'override | Task 2 Step 7 | `Privacy.FlightDependsOnShapeNotOverride`, `Playback.TracerFollowsTheProfile` |
| (19) scarto dello zigzag da `Alpha` | Task 2 Step 7 | `Playback.TracerZigzagGrowsAsAPrefix` |
| (22) clausola R12 tolta da `IsTracerEligible` | Task 2 Step 7 | `Fx.NoActionNoProfile` («`IsTracerEligible` falso») |
| (7) cella d'attivazione da `Src->Cell` | Task 3 Step 8 | `Playback.ActivationCueAtTheSourceCell` |
| (8) `SourceVerdict` tolto | Task 3 Step 8 | `Privacy.ActivationCueNeedsTheSource` |
| (15) `SetPlaybackCues` accoda | Task 3 Step 8 | `HexMapActor.PlaybackCueIsItsOwnChannel` |
| (P5) canale fuori da `HasAnythingToDraw` | Task 3 Step 8 | `HexMapActor.PlaybackCueIsItsOwnChannel` |
| (9) `Marker` al battito `2k` | Task 4 Step 8 | `Playback.ImpactCueComesAtTheArrival` (premessa `F_eff > 0`) |
| (10) `ImpactVerdict` ignorato | Task 4 Step 8 | `Privacy.ImpactMarkerNeedsTheVictim` |
| (11) `AreaPulse` su `Impact` | Task 4 Step 8 | `Fx.AreaPulseIsOnTheFootprintAim` (premessa `AimCell` ≠ ogni `Impact`) |
| (12) `Marker` saltato sul primo colpo dell'atto | Task 4 Step 8 | `Playback.EveryAttackGetsItsProfileMarker`, `Playback.ImpactCueComesAtTheArrival` |
| (13) tetti tolti | Task 4 Step 8 | `Playback.FxCuesNeverOverlapInTheBlast` |
| (14) `Alpha` da un accumulatore | Task 4 Step 8 | `Playback.FxCuesAreAFunctionOfTheClock` |
| (16) `ClearPlaybackCues` tolto da `FinishPlayback` | Task 4 Step 8 | `Playback.CueChannelClearsOnSkip` |
| (21) impronta non consumata | Task 4 Step 8 | `Playback.FootprintCueOncePerFootprint` |
| (P3) `FromVerdict` ignorato nella cue d'impronta | Task 4 Step 8 | `Privacy.AreaPulseNeedsTheCenter` |
| (P4) `bResolved` ignorato nel `Marker` | Task 4 Step 8 | `Playback.UnresolvedGeometryHasNoFxButPlaysTheClip` |
| (20) `AreaPulse` come `Ring` | Task 5 Step 7 | `Fx.CueStylesDifferByGeometry` |
| (P2) `SetPlaybackCues` tolto da `AbilityActivated` | Task 5 Step 7 | `Presentation.FxCueIsDeclaredForAttackAndActivation` |

### Dove sono i sei punti della re-review (`➕ rev2.`)

| Punto | Task / Step |
|---|---|
| (9) e (11) con premessa asserita prima; (18) sull'asserto «profilo tutto `None`»; (22) nuova; test multipli dichiarati | Task 4 Step 1 (premesse nei test), Task 1 Step 6 (18), Task 2 Step 1 e Step 7 (22), tabella qui sopra |
| Grep dei test di mondo esteso a `Ram`, `Action.Charge`, `LinearDischarge`, `PassingBlade`, lanciati PRIMA e DOPO | Task 2 Step 0 e Step 6 |
| R14 e `Playback.SecondFootprintReplacesTheFirst` | Task 4 Step 1 (test), Step 3 (`FootprintFxForSequence` col log `Verbose`) |
| Variante a tabella iniettata del test riscritto | Task 2 Step 1 (`TracerStyleForIn`) |
| Misura di `ReactiveCapacitor` (F22) | Task 4 Step 0 |
| Ritmo = «stesso volo a parità di indice» | Global Constraints; Task 2 Step 3 (commento di `IsTracerEligible`); Task 7 Step 5 (riga della conoscenza parziale) |

---

### Task 0: Issue, branch, spec e piano nel repository

**Files:** `docs/superpowers/specs/2026-10-08-profilo-fx-per-abilita-design.md` (create), `docs/superpowers/plans/2026-10-08-profilo-fx-per-abilita.md` (create). GitHub: una issue nuova sotto l'epic [#2453](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2453).

**Interfaces:**
- Produces: il numero `3578`, usato da branch, commit, commenti e documenti.

- [ ] **Step 1: Apri la issue** (azione verso l'esterno: chiedere conferma all'autore se non già data)

Corpo in `<scratchpad>/issue-fx.md`, poi `--body-file` (mai `--body` inline: i backtick vengono eseguiti). ⛔ Nessun glifo di stato accanto al nome di una voce PIE nel corpo.

```markdown
> **Epic**: #2453 · **Refs**: #2454 (tracer degli attacchi base, owner della grammatica) · #3532 (sotto-progetto 1) · #3549 (sotto-progetto 2) · #3563 (sotto-progetto 3) · **Spec**: `docs/superpowers/specs/2026-10-08-profilo-fx-per-abilita-design.md`

## Why

Nessun segnale visivo di attivazione esiste: `ShowActivation` scrive la riga `Attiva:` e suona la clip `Cast`, sul terreno non si vede niente. Il tracer è l'unico FX del colpo, cablato sugli attacchi base da un'idoneità dichiarata provvisoria; `Area` e `Cone` non hanno cue di colpo oltre all'impronta. Quarto dei quattro sotto-progetti di «associare animazioni e FX alle skill e vederle in azione».

## Scope

1. Il profilo: `FRTAbilityFxProfile` (attivazione, tracer, impatto, impronta), default per forma, override per `ActionId` approvati dall'autore (D5), ripiego `ActionId` → `BaseActionId` → forma; nessun profilo senza id (R12).
2. Il tracer come caso del profilo: volo dalla sola forma di default (R13), disegno dall'override; `Zigzag` per la scarica lineare di Aevik.
3. Il segnale di attivazione sulla cella della sorgente (`Ev.Origin`, scritto dal produttore), dietro `SourceVerdict`.
4. Le cue di colpo: `Marker` all'arrivo di ogni colpo, `AreaPulse`/`ConeSweep` una volta per impronta, dietro i verdetti della cella che disegnano.
5. Un canale di mappa nuovo, disegnato col line batcher (D-467); D-278 aggiornato.
6. Uno scenario visivo nuovo per il banco Ability Lab; voce PIE e seduta.

## Out of scope

`ReactionResolved` e `Defeated` (#2454); arco balistico (#2825); `ArcHit` (#3293); suono; Niagara, Cascade, materiali, decal (D-124); profili come dati esterni; varianti di tracer sugli attacchi base (D5); un'azione `Cone` nel catalogo.

## DoD

- [ ] I test della spec §5.1 verdi sul commit reale, log con `**** TEST COMPLETE`.
- [ ] Le mutazioni (1)–(22) della spec e (P1)–(P5) del piano eseguite: ciascuna fa cadere il test dichiarato.
- [ ] Il test esistente `Playback.TracerStyleFollowsShapeForBasicAttack` riscritto sull'asserto di `PassingBlade`, eccezione dichiarata nella PR; nessun altro test esistente riscritto.
- [ ] `Unit.BlueprintSurfaceIsCensused`, `Presentation.*`, i test del tracer e la suite di determinismo verdi.
- [ ] Build `RefactorTacticsEditor` verde.
- [ ] Voce `PIE-FX-ABILITA` e seduta nel file delle sedute; nota in coda alla voce del tracer; il verdetto a schermo è dell'autore.
- [ ] Nessun `.uasset`, nessun `DrawDebug*`; nessun totale volatile in ciò che si pubblica.
```

```powershell
gh issue create --repo DegrassiAaron/refactor-tactics-main --title "Il profilo FX per abilita': attivazione e colpo, una tabella ActionId sopra la forma" --body-file "<scratchpad>/issue-fx.md" --label enhancement
```

Collega la issue all'epic come le sorelle (commento su #2453 o sub-issue, nella forma che #3563 ha usato: `gh issue view 3563 --json body,projectItems` per leggerla). Rileggi dal server e conta i backtick: `gh issue view 3578 --json body -q .body | grep -o '`' | wc -l` deve coincidere con `grep -o '`' <scratchpad>/issue-fx.md | wc -l`.

- [ ] **Step 2: Branch da `origin/main`, e spec e piano nel repository**

```bash
git -C <checkout> fetch -q origin
git -C <checkout> switch -c issue/3578-profilo-fx-per-abilita origin/main
git -C <checkout> config branch.issue/3578-profilo-fx-per-abilita.parent main
```

Copia la spec rivista (`<checkout>/.superpowers/sp4-spec.md`) in `docs/superpowers/specs/2026-10-08-profilo-fx-per-abilita-design.md` e questo piano (`<checkout>/.superpowers/sp4-plan.md`) in `docs/superpowers/plans/2026-10-08-profilo-fx-per-abilita.md`. Sostituisci `3578` in entrambi, e nella spec ogni «Task N del piano» con il numero vero (Task 4 per la misura di F22). Line ending come il piano SP3 (misurato: CRLF):

```powershell
foreach ($f in "docs/superpowers/specs/2026-10-08-profilo-fx-per-abilita-design.md","docs/superpowers/plans/2026-10-08-profilo-fx-per-abilita.md") {
  $t = [IO.File]::ReadAllText("<checkout>/$f") -replace "`r`n","`n" -replace "`n","`r`n"
  [IO.File]::WriteAllText("<checkout>/$f", $t, [Text.UTF8Encoding]::new($false))
}
```

`file` su entrambi → `with CRLF line terminators`. `.superpowers/` è ignorata da git: non entra nel commit. I link relativi della spec (`../../decisions/...`) valgono da `docs/superpowers/specs/`: `node tools/radar/doc-links.ts --check` verde prima del commit.

```bash
git add docs/superpowers/specs/2026-10-08-profilo-fx-per-abilita-design.md docs/superpowers/plans/2026-10-08-profilo-fx-per-abilita.md
git commit -F - <<'EOF'
docs(3578): spec e piano del profilo FX per abilita'

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
```

---

### Task 1: Il profilo e la tabella — tipi, default, override approvati, ripiego (puri)

**Files:**
- Modify: `Map/RTPlaybackTracer.h:13-22` (`Zigzag` in coda a `ERTTracerStyle`) e in coda al file dopo `FRTPlaybackTracer` (`:30-46`): gli enum e le struct nuove
- Modify: `Turn/RTPresentationBinding.h:5` (include) e `:257-259` (dichiarazioni prima della `};` di `URTPresentationBindingLibrary`)
- Modify: `Turn/RTPresentationBinding.cpp` in coda (`:445`): le definizioni
- Create: `Tests/RTAbilityFxProfileTests.cpp`

**Interfaces:**
- Produces (in `Map/RTPlaybackTracer.h`):
  ```cpp
  enum class ERTTracerStyle : uint8 { None, Projectile, Jet, Zigzag };
  enum class ERTActivationFxStyle : uint8 { None, Ring, Pulse, Flash };
  enum class ERTImpactFxStyle : uint8 { None, Marker };
  enum class ERTFootprintFxStyle : uint8 { None, AreaPulse, ConeSweep };
  struct FRTAbilityFxProfile { ERTActivationFxStyle Activation; ERTTracerStyle Tracer; ERTImpactFxStyle Impact; ERTFootprintFxStyle Footprint; bool operator==(const FRTAbilityFxProfile&) const; };
  enum class ERTPlaybackCueKind : uint8 { Ring, Pulse, Flash, Marker, AreaPulse, ConeSweep };
  struct FRTPlaybackCue { ERTPlaybackCueKind Kind; FRTCellId At; FRTCellId Toward; float Alpha; };
  ```
- Produces (statiche C++, non `UFUNCTION`, in `URTPresentationBindingLibrary`):
  ```cpp
  static FRTAbilityFxProfile MakeFxProfile(ERTActivationFxStyle Activation, ERTTracerStyle Tracer, ERTImpactFxStyle Impact, ERTFootprintFxStyle Footprint);
  static FRTAbilityFxProfile DefaultFxProfileFor(ERTAbilityShape Shape);
  static const TArray<TPair<FName, FRTAbilityFxProfile>>& DeclaredFxOverrideRows();
  static const TMap<FName, FRTAbilityFxProfile>& DeclaredFxOverrides();
  static FRTAbilityFxProfile FxProfileForIn(const TMap<FName, FRTAbilityFxProfile>& Overrides, FName ActionId, FName BaseActionId, ERTAbilityShape Shape);
  static FRTAbilityFxProfile FxProfileFor(FName ActionId, FName BaseActionId, ERTAbilityShape Shape);
  ```
- Consumes: `URTHeroCatalogLibrary::GetHeroRoster()` (`Ability/RTHeroCatalogLibrary.h:161`), `URTHeroData::Actions` (`Ability/RTHeroData.h:164`), `URTActionData::Shape` (`Ability/RTActionData.h:128`), `FRTActionDef::ActionId`/`BaseActionId` (`Ability/RTActionDef.h:513`, `:528`).

➕ piano. `FRTPlaybackCue` e `ERTPlaybackCueKind` nascono qui (sono tipi, non comportamento) perché i Task 3, 4 e 5 li consumano con la stessa firma; nessun codice li usa prima del Task 3.

- [ ] **Step 0: Misure di premessa (sola lettura)**

```bash
git grep -n "ERTAbilityShape::Cone" -- Source ':!Source/RefactorTactics/Tests'
git grep -n "case ERTTracerStyle" -- Source
grep -ohE 'TEXT\("Hero\.(Aevik|Muiren|Branth|Ivrin)\.[A-Za-z]+"\)' Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp | sort -u
```

Attesi: il primo risponde solo `Combat/RTHexCombatLibrary.cpp:42` e `UI/RTHudViewModel.cpp:464` (il `Cone` senza contenuto, spec §1); il secondo **nessuna** riga (nessuno `switch` da completare per `Zigzag`: `TracerSegment` usa `if`, `Turn/RTPlaybackLibrary.cpp:56-77`; misurato in pianificazione); il terzo contiene ogni `ActionId` `Hero.*` delle righe d'override qui sotto. Se una premessa cade, ci si ferma e si riporta.

- [ ] **Step 1: I test rossi**

Crea `Tests/RTAbilityFxProfileTests.cpp`. ⚠️ Uno strumento di scrittura produce LF: subito dopo la creazione converti a CRLF e verifica con `file` → `with CRLF line terminators`:

```powershell
$f = "<checkout>/Source/RefactorTactics/Tests/RTAbilityFxProfileTests.cpp"
$t = [IO.File]::ReadAllText($f) -replace "`r`n","`n" -replace "`n","`r`n"
[IO.File]::WriteAllText($f, $t, [Text.UTF8Encoding]::new($false))
```

Il contenuto:

```cpp
// IL PROFILO FX PER ABILITA' (#3578, spec «il profilo FX per abilita'» §2.1-§2.4, §5.1).
//
// 🔑 **Funzioni pure**: nessun mondo, nessun Actor. La tabella vera si legge solo dove il test giudica la tabella
// vera (`DeclaredOverridesMatchTheProposal`); il ripiego si prova su una tabella INIETTATA (`FxProfileForIn`), come
// R11 della spec della clip: un asserto che regge su una riga giudicata cade quando l'autore cambia idea.
// ⚠️ Nomi distinti per file: in unity build i test condividono la translation unit.

#include "Misc/AutomationTest.h"
#include "Map/RTPlaybackTracer.h"
#include "Turn/RTPresentationBinding.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Turn/RTResolvedEvent.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Perception/RTTeamKnowledge.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	using FxA = ERTActivationFxStyle;
	using FxT = ERTTracerStyle;
	using FxI = ERTImpactFxStyle;
	using FxF = ERTFootprintFxStyle;

	FRTAbilityFxProfile FxP(FxA A, FxT T, FxI I, FxF F)
	{
		return URTPresentationBindingLibrary::MakeFxProfile(A, T, I, F);
	}

	/** Il profilo come testo: un `TestEqual` su due stringhe dice QUALE campo e' diverso. */
	FString FxTesto(const FRTAbilityFxProfile& P)
	{
		return FString::Printf(TEXT("%s · %s · %s · %s"),
			*UEnum::GetValueAsString(P.Activation), *UEnum::GetValueAsString(P.Tracer),
			*UEnum::GetValueAsString(P.Impact), *UEnum::GetValueAsString(P.Footprint));
	}

	FRTAbilityFxProfile FxNessuno() { return FxP(FxA::None, FxT::None, FxI::None, FxF::None); }
}

/**
 * Il default di ogni forma e' la riga di spec §2.2 (D3, grammatica di #2454).
 * ✅ Validato per mutazione (1): la riga `Line` con `Projectile` fa cadere «Line → Jet».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxDefaultProfileFollowsShapeTest,
	"RefactorTactics.Fx.DefaultProfileFollowsShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxDefaultProfileFollowsShapeTest::RunTest(const FString&)
{
	const UEnum* Forme = StaticEnum<ERTAbilityShape>();
	if (!TestNotNull(TEXT("premessa: reflection di ERTAbilityShape"), Forme)) { return false; }
	// ⛔ Una quinta forma vuole una riga in `DefaultFxProfileFor` e una qui: questo asserto lo ricorda.
	TestEqual(TEXT("⛔ premessa: le forme sono quelle della tabella di §2.2"), Forme->NumEnums() - 1, 4);

	TestEqual(TEXT("Single"), FxTesto(URTPresentationBindingLibrary::DefaultFxProfileFor(ERTAbilityShape::Single)),
		FxTesto(FxP(FxA::Ring, FxT::Projectile, FxI::Marker, FxF::None)));
	TestEqual(TEXT("🔴 Line → Jet"), FxTesto(URTPresentationBindingLibrary::DefaultFxProfileFor(ERTAbilityShape::Line)),
		FxTesto(FxP(FxA::Ring, FxT::Jet, FxI::Marker, FxF::None)));
	TestEqual(TEXT("Area: nessun tracer, AreaPulse"), FxTesto(URTPresentationBindingLibrary::DefaultFxProfileFor(ERTAbilityShape::Area)),
		FxTesto(FxP(FxA::Ring, FxT::None, FxI::Marker, FxF::AreaPulse)));
	TestEqual(TEXT("Cone: nessun tracer, ConeSweep"), FxTesto(URTPresentationBindingLibrary::DefaultFxProfileFor(ERTAbilityShape::Cone)),
		FxTesto(FxP(FxA::Ring, FxT::None, FxI::Marker, FxF::ConeSweep)));
	return true;
}

/**
 * Il ripiego `ActionId` → `BaseActionId` → forma, su una tabella INIETTATA (spec §2.2, R4).
 * ✅ Validato per mutazioni (2) — la forma letta prima — e (3) — il livello `BaseActionId` saltato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxProfileFallsBackTest,
	"RefactorTactics.Fx.ProfileFallsBackActionThenBaseThenShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxProfileFallsBackTest::RunTest(const FString&)
{
	TMap<FName, FRTAbilityFxProfile> Prova;
	Prova.Add(FName(TEXT("Hero.Prova.Colpo")), FxP(FxA::Flash, FxT::None, FxI::None, FxF::None));
	Prova.Add(FName(TEXT("Action.Prova")), FxP(FxA::Pulse, FxT::None, FxI::None, FxF::None));
	const FRTAbilityFxProfile Forma = URTPresentationBindingLibrary::DefaultFxProfileFor(ERTAbilityShape::Single);

	TestEqual(TEXT("🔴 ActionId vince"), FxTesto(URTPresentationBindingLibrary::FxProfileForIn(Prova,
		TEXT("Hero.Prova.Colpo"), TEXT("Action.Prova"), ERTAbilityShape::Single)), FxTesto(Prova[TEXT("Hero.Prova.Colpo")]));
	TestEqual(TEXT("🔴 senza ActionId in tabella, vince BaseActionId"), FxTesto(URTPresentationBindingLibrary::FxProfileForIn(Prova,
		TEXT("Hero.Prova.Altro"), TEXT("Action.Prova"), ERTAbilityShape::Single)), FxTesto(Prova[TEXT("Action.Prova")]));
	TestEqual(TEXT("ActionId vuoto: il livello si salta, vince BaseActionId"), FxTesto(URTPresentationBindingLibrary::FxProfileForIn(Prova,
		NAME_None, TEXT("Action.Prova"), ERTAbilityShape::Single)), FxTesto(Prova[TEXT("Action.Prova")]));
	TestEqual(TEXT("nessuno dei due in tabella: la forma"), FxTesto(URTPresentationBindingLibrary::FxProfileForIn(Prova,
		TEXT("Hero.Prova.Altro"), TEXT("Action.Altra"), ERTAbilityShape::Single)), FxTesto(Forma));
	// 🔑 `BaseActionId` si legge dall'evento e non si indovina (`RTResolvedEvent.h:393-396`): `Action.Prova` e' in
	// tabella, ma l'evento non la dichiara.
	TestEqual(TEXT("BaseActionId = NAME_None non indovina la generica"), FxTesto(URTPresentationBindingLibrary::FxProfileForIn(Prova,
		TEXT("Hero.Prova.Altro"), NAME_None, ERTAbilityShape::Single)), FxTesto(Forma));
	// R4: il profilo e' intero per livello, nessuna fusione per campo con la forma.
	TestTrue(TEXT("nessuna fusione: la riga senza tracer resta senza tracer anche su Line"),
		URTPresentationBindingLibrary::FxProfileForIn(Prova, TEXT("Hero.Prova.Colpo"), NAME_None, ERTAbilityShape::Line).Tracer
			== ERTTracerStyle::None);
	return true;
}

/**
 * Review Focus (e): `ActionId` senza riga, `BaseActionId` con riga → il profilo della generica, sulla tabella VERA.
 * Il caso reale e' `Action.Charge` (spec §2.2, riga fuori roster). ✅ Validato per mutazione (3).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxBaseActionOverrideWinsTest,
	"RefactorTactics.Fx.BaseActionOverrideWinsOverShapeDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxBaseActionOverrideWinsTest::RunTest(const FString&)
{
	const FRTAbilityFxProfile Forma = URTPresentationBindingLibrary::DefaultFxProfileFor(ERTAbilityShape::Single);
	if (!TestTrue(TEXT("⛔ premessa: il default di Single ha un proiettile, quindi la differenza e' misurabile"),
			Forma.Tracer == ERTTracerStyle::Projectile))
	{
		return false;
	}
	TestEqual(TEXT("🔴 una carica senza riga propria, con BaseActionId = Action.Charge: il profilo della generica"),
		FxTesto(URTPresentationBindingLibrary::FxProfileFor(TEXT("Hero.Prova.Carica"), TEXT("Action.Charge"), ERTAbilityShape::Single)),
		FxTesto(FxP(FxA::Ring, FxT::None, FxI::Marker, FxF::None)));
	TestEqual(TEXT("controllo: senza BaseActionId, il default della forma"),
		FxTesto(URTPresentationBindingLibrary::FxProfileFor(TEXT("Hero.Prova.Carica"), NAME_None, ERTAbilityShape::Single)),
		FxTesto(Forma));
	return true;
}

/**
 * R12 — nessuna azione, nessun profilo (spec §2.2, F8). Gli attacchi legacy di `ARTUnit::MakeAbility`
 * (`Unit/RTUnit.cpp:1606-1615`) non scrivono `ActionId`.
 * ✅ Validato per mutazione (18): la guardia tolta da `FxProfileForIn` fa cadere «profilo tutto None».
 * ➕ piano. Il Task 2 aggiunge a QUESTO test l'asserto su `IsTracerEligible` (mutazione (22)): quella funzione ha
 * la propria clausola R12 e cambia nel Task 2.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxNoActionNoProfileTest,
	"RefactorTactics.Fx.NoActionNoProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxNoActionNoProfileTest::RunTest(const FString&)
{
	TestEqual(TEXT("🔴 legacy Single: profilo tutto None"),
		FxTesto(URTPresentationBindingLibrary::FxProfileFor(NAME_None, NAME_None, ERTAbilityShape::Single)), FxTesto(FxNessuno()));
	TestEqual(TEXT("🔴 legacy Area: profilo tutto None"),
		FxTesto(URTPresentationBindingLibrary::FxProfileFor(NAME_None, NAME_None, ERTAbilityShape::Area)), FxTesto(FxNessuno()));
	TestEqual(TEXT("controllo: con un solo id, la forma torna"),
		FxTesto(URTPresentationBindingLibrary::FxProfileFor(NAME_None, TEXT("Action.BasicAttack"), ERTAbilityShape::Single)),
		FxTesto(URTPresentationBindingLibrary::DefaultFxProfileFor(ERTAbilityShape::Single)));
	return true;
}

/**
 * La mappa degli override APPROVATA dall'autore (spec §2.2, D5), ricopiata qui come seconda copia DICHIARATA.
 * La lista delle abilita' e' una FUNZIONE del catalogo (`GetHeroRoster`): un'abilita' nuova senza riga prende il
 * default della sua forma, e questo test lo verifica senza essere toccato.
 *
 * 🔴 Ogni riga si cambia con un commit di DUE righe: quella di `DeclaredFxOverrideRows` e la sua gemella qui.
 * ✅ Validato per mutazione (4): la riga `TideGuard` tolta dalla tabella vera fa cadere la sua abilita'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxDeclaredOverridesMatchTheProposalTest,
	"RefactorTactics.Fx.DeclaredOverridesMatchTheProposal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxDeclaredOverridesMatchTheProposalTest::RunTest(const FString&)
{
	TMap<FName, FRTAbilityFxProfile> Attesi;
	Attesi.Add(TEXT("Hero.Aevik.LinearDischarge"), FxP(FxA::Ring,  FxT::Zigzag, FxI::Marker, FxF::None));
	Attesi.Add(TEXT("Hero.Aevik.Overload"),        FxP(FxA::Flash, FxT::None,   FxI::Marker, FxF::AreaPulse));
	Attesi.Add(TEXT("Hero.Muiren.CircularTide"),   FxP(FxA::Pulse, FxT::None,   FxI::Marker, FxF::AreaPulse));
	Attesi.Add(TEXT("Hero.Muiren.TideGuard"),      FxP(FxA::Pulse, FxT::None,   FxI::None,   FxF::None));
	Attesi.Add(TEXT("Hero.Branth.Ram"),            FxP(FxA::Ring,  FxT::None,   FxI::Marker, FxF::None));
	Attesi.Add(TEXT("Hero.Ivrin.InterceptShot"),   FxP(FxA::Flash, FxT::None,   FxI::None,   FxF::None));
	Attesi.Add(TEXT("Hero.Ivrin.PassingBlade"),    FxP(FxA::Ring,  FxT::None,   FxI::Marker, FxF::None));
	Attesi.Add(TEXT("Hero.Ivrin.Feint"),           FxP(FxA::Flash, FxT::None,   FxI::None,   FxF::None));
	Attesi.Add(TEXT("Hero.Ivrin.PhaseGuard"),      FxP(FxA::Pulse, FxT::None,   FxI::None,   FxF::None));
	Attesi.Add(TEXT("Action.Charge"),              FxP(FxA::Ring,  FxT::None,   FxI::Marker, FxF::None));

	TSet<FName> NelRoster;
	int32 Viste = 0;
	for (const URTHeroData* Eroe : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Eroe) { continue; }
		for (const URTActionData* Azione : Eroe->Actions)
		{
			if (!Azione || Azione->Def.ActionId.IsNone()) { continue; }
			const FName Id = Azione->Def.ActionId;
			NelRoster.Add(Id);
			++Viste;
			const FRTAbilityFxProfile* Riga = Attesi.Find(Id);
			const FRTAbilityFxProfile Atteso = Riga ? *Riga : URTPresentationBindingLibrary::DefaultFxProfileFor(Azione->Shape);
			TestEqual(FString::Printf(TEXT("%s: il profilo e' quello approvato"), *Id.ToString()),
				FxTesto(URTPresentationBindingLibrary::FxProfileFor(Id, Azione->Def.BaseActionId, Azione->Shape)), FxTesto(Atteso));
			if (Riga)
			{
				// R13: un override non AGGIUNGE un tracer a una forma che non ne ha (non avrebbe volo).
				TestTrue(FString::Printf(TEXT("⛔ %s: nessun tracer su una forma senza volo"), *Id.ToString()),
					Riga->Tracer == ERTTracerStyle::None
					|| URTPresentationBindingLibrary::DefaultFxProfileFor(Azione->Shape).Tracer != ERTTracerStyle::None);
			}
		}
	}
	TestTrue(TEXT("⛔ premessa: il roster dichiara azioni"), Viste > 0);

	// Il verso opposto: ogni riga VERA ha la gemella qui, e ogni riga `Hero.*` e' un'abilita' del roster.
	for (const TPair<FName, FRTAbilityFxProfile>& Riga : URTPresentationBindingLibrary::DeclaredFxOverrideRows())
	{
		TestTrue(FString::Printf(TEXT("%s: la riga vera ha la gemella nel test"), *Riga.Key.ToString()), Attesi.Contains(Riga.Key));
		if (Riga.Key.ToString().StartsWith(TEXT("Hero.")))
		{
			TestTrue(FString::Printf(TEXT("%s: e' un'abilita' del roster"), *Riga.Key.ToString()), NelRoster.Contains(Riga.Key));
		}
		else
		{
			TestTrue(FString::Printf(TEXT("%s: fuori roster, nessun tracer (la forma non si conosce qui)"), *Riga.Key.ToString()),
				Riga.Value.Tracer == ERTTracerStyle::None);
		}
	}
	TestEqual(TEXT("Action.Charge, fuori roster"),
		FxTesto(URTPresentationBindingLibrary::FxProfileFor(TEXT("Action.Charge"), NAME_None, ERTAbilityShape::Single)),
		FxTesto(Attesi[TEXT("Action.Charge")]));
	return true;
}

/**
 * Nessuna chiave duplicata fra le righe (spec §5.1, F17): in una mappa costruita dalle righe una duplicata vincerebbe
 * in silenzio. ✅ Validato per mutazione (P1): una riga ripetuta fa cadere il primo asserto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxDeclaredOverridesHaveNoDuplicateKeysTest,
	"RefactorTactics.Fx.DeclaredOverridesHaveNoDuplicateKeys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxDeclaredOverridesHaveNoDuplicateKeysTest::RunTest(const FString&)
{
	TSet<FName> Viste;
	for (const TPair<FName, FRTAbilityFxProfile>& Riga : URTPresentationBindingLibrary::DeclaredFxOverrideRows())
	{
		TestFalse(FString::Printf(TEXT("🔴 %s compare una volta sola"), *Riga.Key.ToString()), Viste.Contains(Riga.Key));
		Viste.Add(Riga.Key);
	}
	TestEqual(TEXT("la mappa ha una chiave per riga"),
		URTPresentationBindingLibrary::DeclaredFxOverrides().Num(), URTPresentationBindingLibrary::DeclaredFxOverrideRows().Num());
	return true;
}

// --- I test dei Task 3, 4 e 5 si aggiungono QUI, prima di `#endif` ----------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 2: Compila e verifica che fallisca**

Run: build. Expected: **rosso di compilazione** — `FRTAbilityFxProfile`, `ERTActivationFxStyle` e `URTPresentationBindingLibrary::MakeFxProfile` non esistono (`C2065`/`C2039`). È il rosso di questo passo.

- [ ] **Step 3: I tipi**

`Map/RTPlaybackTracer.h`, in `ERTTracerStyle` dopo `Jet,` (`:21`):

```cpp
	/**
	 * ➕ #3578 (spec «il profilo FX» §2.1). Un getto SPEZZATO: ancorato all'origine come `Jet`, con i vertici funzione
	 * di (From, To) soltanto — nessun `Rand`, nessun orologio — quindi a `α` minore e' un PREFISSO di quello a `α`
	 * maggiore (`URTPlaybackLibrary::TracerPolyline`). Un solo consumatore oggi: `Hero.Aevik.LinearDischarge` (D5).
	 */
	Zigzag,
```

In coda al file, dopo la `};` di `FRTPlaybackTracer` (`:46`):

```cpp

/** Il segnale di attivazione sulla sorgente (`#3578`, spec §2.1, D1, D3). */
UENUM(BlueprintType)
enum class ERTActivationFxStyle : uint8
{
	None,
	/** Un esagono che si allarga. Il default di ogni forma. */
	Ring,
	/** Due esagoni concentrici che si stringono: sostegno, guardia. */
	Pulse,
	/** Sei raggi inclinati di 45° verso l'alto e l'esterno: un lampo, nessun verso (F21). */
	Flash,
};

/** La cue di OGNI `Attack`, all'arrivo, sulla sua vittima (spec §2.1, F5, F6). */
UENUM(BlueprintType)
enum class ERTImpactFxStyle : uint8
{
	None,
	/** Quattro raggi a X che crescono dal centro della vittima. */
	Marker,
};

/** La cue dell'IMPRONTA dell'atto: una per `AttackFootprint`, all'arrivo del primo colpo che la segue (spec §2.4). */
UENUM(BlueprintType)
enum class ERTFootprintFxStyle : uint8
{
	None,
	/** Un esagono a raggi che si allarga dal centro dell'area (`AimCell` dell'impronta). */
	AreaPulse,
	/** Un braccio che ruota da −60° a +60° attorno all'asse `Origin → AimCell` dell'impronta. */
	ConeSweep,
};

/**
 * Il profilo FX di un'azione (`#3578`, spec §2.1, D2). Presentazione pura: non entra in snapshot, TurnLog, StateHash,
 * replay.
 *
 * 🔑 **Default TUTTI `None`** (F17): una voce si costruisce con `URTPresentationBindingLibrary::MakeFxProfile`,
 * quattro argomenti obbligatori, cosi' una riga che dimentica un campo non eredita in silenzio.
 * ⛔ Le durate NON sono qui: sono ritmo (R2, D-287 punto 7), manopole di `ARTTurnManager`.
 */
USTRUCT()
struct FRTAbilityFxProfile
{
	GENERATED_BODY()

	UPROPERTY()
	ERTActivationFxStyle Activation = ERTActivationFxStyle::None;

	UPROPERTY()
	ERTTracerStyle Tracer = ERTTracerStyle::None;

	UPROPERTY()
	ERTImpactFxStyle Impact = ERTImpactFxStyle::None;

	UPROPERTY()
	ERTFootprintFxStyle Footprint = ERTFootprintFxStyle::None;

	bool operator==(const FRTAbilityFxProfile& Other) const
	{
		return Activation == Other.Activation && Tracer == Other.Tracer && Impact == Other.Impact
			&& Footprint == Other.Footprint;
	}
};

/** Il tipo di una cue di playback (spec §2.4, R10): UN campo di tipo, non un'unione con un invariante. */
UENUM()
enum class ERTPlaybackCueKind : uint8
{
	Ring, Pulse, Flash, Marker, AreaPulse, ConeSweep,
};

/**
 * Una cue nel fotogramma corrente: celle, non posizioni di attori (la stessa ragione di `FRTPlaybackTracer`).
 * La consegna `ARTTurnManager::PushPlaybackCues` e la SOSTITUISCE in blocco: e' funzione dell'orologio.
 */
USTRUCT()
struct FRTPlaybackCue
{
	GENERATED_BODY()

	UPROPERTY()
	ERTPlaybackCueKind Kind = ERTPlaybackCueKind::Ring;

	/** Sorgente (attivazione) | vittima (`Marker`) | centro (`AreaPulse`) | origine del ventaglio (`ConeSweep`). */
	UPROPERTY()
	FRTCellId At;

	/** Solo `ConeSweep`: l'`AimCell` dell'impronta. Per gli altri tipi vale `At`. */
	UPROPERTY()
	FRTCellId Toward;

	UPROPERTY()
	float Alpha = 0.f;
};
```

- [ ] **Step 4: La tabella**

`Turn/RTPresentationBinding.h`, dopo `#include "Turn/RTResolvedEvent.h"` (`:5`):

```cpp
// #3578: il profilo FX vive accanto al tracer (`FRTAbilityFxProfile`). Header di soli tipi di valore.
#include "Map/RTPlaybackTracer.h"
```

Dopo la dichiarazione di `StyleForMovement` (`:257-258`), prima della `};` della classe (`:259`):

```cpp

	// --- Il profilo FX per abilita' (#3578, spec «il profilo FX per abilita'» §2.2, D2) -------------------------
	//
	// 🔑 **Sta qui per la stessa ragione di `StyleForMovement`**: questa libreria e' l'owner dell'asse
	// «evento -> presentazione» ([D-278]). Funzioni statiche C++, NON `UFUNCTION`: nessuna superficie Blueprint.
	// Ogni cambio ricompila; una migrazione ad asset non cambia il gate (spec §3).

	/** Un profilo con TUTTI e quattro i campi: l'unico costruttore delle righe (F17). */
	static FRTAbilityFxProfile MakeFxProfile(ERTActivationFxStyle Activation, ERTTracerStyle Tracer,
		ERTImpactFxStyle Impact, ERTFootprintFxStyle Footprint);

	/** Il default della forma (spec §2.2, D3): Single → proiettile, Line → getto, Area → AreaPulse, Cone → ConeSweep. */
	static FRTAbilityFxProfile DefaultFxProfileFor(ERTAbilityShape Shape);

	/**
	 * Le righe d'override APPROVATE dall'autore (D5), in ordine. Una riga per override; la gemella sta in
	 * `Fx.DeclaredOverridesMatchTheProposal`. ⛔ Nessun override di tracer sugli attacchi base (R3).
	 */
	static const TArray<TPair<FName, FRTAbilityFxProfile>>& DeclaredFxOverrideRows();

	/** La mappa costruita UNA volta dalle righe. Si usa solo con `Find`. */
	static const TMap<FName, FRTAbilityFxProfile>& DeclaredFxOverrides();

	/**
	 * Il ripiego `Overrides[ActionId]` → `Overrides[BaseActionId]` → default della forma, su una tabella data (pura:
	 * i test le passano la propria). Un id vuoto salta il proprio livello; `BaseActionId` non si deriva mai.
	 * 🔴 **R12**: con `ActionId` E `BaseActionId` vuoti il profilo e' tutto `None`, senza passare dalla forma.
	 * ⛔ `DerivedFromActionId` non e' un livello: non e' sull'evento.
	 */
	static FRTAbilityFxProfile FxProfileForIn(const TMap<FName, FRTAbilityFxProfile>& Overrides,
		FName ActionId, FName BaseActionId, ERTAbilityShape Shape);

	/** `FxProfileForIn` sulla tabella vera. */
	static FRTAbilityFxProfile FxProfileFor(FName ActionId, FName BaseActionId, ERTAbilityShape Shape);
```

`Turn/RTPresentationBinding.cpp`, in coda al file (dopo `:445`):

```cpp

// --- Il profilo FX per abilita' (#3578) -----------------------------------------------------------------------

FRTAbilityFxProfile URTPresentationBindingLibrary::MakeFxProfile(ERTActivationFxStyle Activation, ERTTracerStyle Tracer,
	ERTImpactFxStyle Impact, ERTFootprintFxStyle Footprint)
{
	FRTAbilityFxProfile P;
	P.Activation = Activation;
	P.Tracer = Tracer;
	P.Impact = Impact;
	P.Footprint = Footprint;
	return P;
}

FRTAbilityFxProfile URTPresentationBindingLibrary::DefaultFxProfileFor(ERTAbilityShape Shape)
{
	// La grammatica di #2454 per forma (spec §2.2). ⚠️ `Single`/`Line` sono le sole forme con un tracer di default,
	// quindi le sole con un VOLO (R13, `URTPlaybackLibrary::IsTracerEligible`).
	switch (Shape)
	{
	case ERTAbilityShape::Single:
		return MakeFxProfile(ERTActivationFxStyle::Ring, ERTTracerStyle::Projectile, ERTImpactFxStyle::Marker, ERTFootprintFxStyle::None);
	case ERTAbilityShape::Line:
		return MakeFxProfile(ERTActivationFxStyle::Ring, ERTTracerStyle::Jet, ERTImpactFxStyle::Marker, ERTFootprintFxStyle::None);
	case ERTAbilityShape::Area:
		return MakeFxProfile(ERTActivationFxStyle::Ring, ERTTracerStyle::None, ERTImpactFxStyle::Marker, ERTFootprintFxStyle::AreaPulse);
	case ERTAbilityShape::Cone:
		return MakeFxProfile(ERTActivationFxStyle::Ring, ERTTracerStyle::None, ERTImpactFxStyle::Marker, ERTFootprintFxStyle::ConeSweep);
	}
	return FRTAbilityFxProfile();
}

const TArray<TPair<FName, FRTAbilityFxProfile>>& URTPresentationBindingLibrary::DeclaredFxOverrideRows()
{
	// La mappa approvata dall'autore il 2026-10-08 (spec §2.2, D5). Notazione: Activation · Tracer · Impact · Footprint.
	// Le ragioni stanno nella spec, una riga per override; qui solo i dati.
	// ⚠️ `Hero.Aevik.ReactiveCapacitor` NON ha riga (F22, ➕ rev2.): il suo contrattacco non emette un evento `Attack`
	// (`RTTurnManager.cpp:6357` contro `:6460-6468`), quindi non ha colpo da disegnare; resta il default della forma.
	static const TArray<TPair<FName, FRTAbilityFxProfile>> Righe = []()
	{
		using A = ERTActivationFxStyle;
		using T = ERTTracerStyle;
		using I = ERTImpactFxStyle;
		using F = ERTFootprintFxStyle;
		TArray<TPair<FName, FRTAbilityFxProfile>> R;
		auto Riga = [&R](const TCHAR* Id, const FRTAbilityFxProfile& P) { R.Emplace(FName(Id), P); };
		Riga(TEXT("Hero.Aevik.LinearDischarge"), URTPresentationBindingLibrary::MakeFxProfile(A::Ring,  T::Zigzag, I::Marker, F::None));
		Riga(TEXT("Hero.Aevik.Overload"),        URTPresentationBindingLibrary::MakeFxProfile(A::Flash, T::None,   I::Marker, F::AreaPulse));
		Riga(TEXT("Hero.Muiren.CircularTide"),   URTPresentationBindingLibrary::MakeFxProfile(A::Pulse, T::None,   I::Marker, F::AreaPulse));
		Riga(TEXT("Hero.Muiren.TideGuard"),      URTPresentationBindingLibrary::MakeFxProfile(A::Pulse, T::None,   I::None,   F::None));
		Riga(TEXT("Hero.Branth.Ram"),            URTPresentationBindingLibrary::MakeFxProfile(A::Ring,  T::None,   I::Marker, F::None));
		Riga(TEXT("Hero.Ivrin.InterceptShot"),   URTPresentationBindingLibrary::MakeFxProfile(A::Flash, T::None,   I::None,   F::None));
		Riga(TEXT("Hero.Ivrin.PassingBlade"),    URTPresentationBindingLibrary::MakeFxProfile(A::Ring,  T::None,   I::Marker, F::None));
		Riga(TEXT("Hero.Ivrin.Feint"),           URTPresentationBindingLibrary::MakeFxProfile(A::Flash, T::None,   I::None,   F::None));
		Riga(TEXT("Hero.Ivrin.PhaseGuard"),      URTPresentationBindingLibrary::MakeFxProfile(A::Pulse, T::None,   I::None,   F::None));
		Riga(TEXT("Action.Charge"),              URTPresentationBindingLibrary::MakeFxProfile(A::Ring,  T::None,   I::Marker, F::None));
		return R;
	}();
	return Righe;
}

const TMap<FName, FRTAbilityFxProfile>& URTPresentationBindingLibrary::DeclaredFxOverrides()
{
	static const TMap<FName, FRTAbilityFxProfile> Mappa = []()
	{
		TMap<FName, FRTAbilityFxProfile> M;
		for (const TPair<FName, FRTAbilityFxProfile>& Riga : URTPresentationBindingLibrary::DeclaredFxOverrideRows())
		{
			M.Add(Riga.Key, Riga.Value);
		}
		return M;
	}();
	return Mappa;
}

FRTAbilityFxProfile URTPresentationBindingLibrary::FxProfileForIn(const TMap<FName, FRTAbilityFxProfile>& Overrides,
	FName ActionId, FName BaseActionId, ERTAbilityShape Shape)
{
	// R12 (spec §2.2, F8): nessuna azione, nessun profilo — gli attacchi legacy restano senza FX e senza volo.
	if (ActionId.IsNone() && BaseActionId.IsNone())
	{
		return FRTAbilityFxProfile();
	}
	if (!ActionId.IsNone())
	{
		if (const FRTAbilityFxProfile* P = Overrides.Find(ActionId))
		{
			return *P;
		}
	}
	if (!BaseActionId.IsNone())
	{
		if (const FRTAbilityFxProfile* P = Overrides.Find(BaseActionId))
		{
			return *P;
		}
	}
	return DefaultFxProfileFor(Shape);
}

FRTAbilityFxProfile URTPresentationBindingLibrary::FxProfileFor(FName ActionId, FName BaseActionId, ERTAbilityShape Shape)
{
	return FxProfileForIn(DeclaredFxOverrides(), ActionId, BaseActionId, Shape);
}
```

- [ ] **Step 5: Build e verde**

Run: build; test `RefactorTactics.Fx`, log `<scratchpad>/t1-fx.log`. Expected: i sei test `Fx.*` `Result={Success}`, `**** TEST COMPLETE`. Poi `RefactorTactics.Presentation+RefactorTactics.Playback.Tracer` (log `<scratchpad>/t1-regressione.log`): verde come su `origin/main` (`Zigzag` in coda all'enum non cambia i valori esistenti; il tracer non è ancora toccato).

- [ ] **Step 6: Commit, poi mutazioni (1), (2), (3), (4), (18), (P1)**

```bash
git add Source/RefactorTactics/Map/RTPlaybackTracer.h Source/RefactorTactics/Turn/RTPresentationBinding.h Source/RefactorTactics/Turn/RTPresentationBinding.cpp Source/RefactorTactics/Tests/RTAbilityFxProfileTests.cpp
git commit -F - <<'EOF'
feat(3578): il profilo FX per abilita' — tipi, default per forma, override approvati, ripiego ActionId/BaseActionId/forma

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
```

Ogni mutazione: modifica, build, test `RefactorTactics.Fx`, `Result={Fail}` sull'asserto indicato, `git checkout -- Source/RefactorTactics/Turn/RTPresentationBinding.cpp`, build, verde.

1. In `DefaultFxProfileFor`, `case ERTAbilityShape::Line:` → `ERTTracerStyle::Jet` diventa `ERTTracerStyle::Projectile`. Cade `Fx.DefaultProfileFollowsShape` «🔴 Line → Jet». (Dal Task 2 cade anche `Playback.TracerStyleFollowsShapeForBasicAttack` «Line -> getto», `Tests/RTPlaybackLibraryTests.cpp:428-429`: lo si ripete al Task 2 Step 7, ➕ rev2.)
2. In `FxProfileForIn`, sposta `return DefaultFxProfileFor(Shape);` subito dopo la guardia R12, prima di `if (!ActionId.IsNone())`. Cade `Fx.ProfileFallsBackActionThenBaseThenShape` «🔴 ActionId vince». ➕ esecuzione (Task 1). Alla lettera **non compila**: il codice dopo il `return` è irraggiungibile, e `C4702` in questo progetto è un errore. La forma eseguita, con la stessa semantica, è una guardia sempre vera subito dopo la guardia R12: `if (Overrides.Num() >= 0) { return DefaultFxProfileFor(Shape); }`. Cadono **tre** test, perché leggono la forma al posto della riga: `Fx.ProfileFallsBackActionThenBaseThenShape` «🔴 ActionId vince», `Fx.BaseActionOverrideWinsOverShapeDefault` e `Fx.DeclaredOverridesMatchTheProposal`.
3. In `FxProfileForIn`, cancella il blocco `if (!BaseActionId.IsNone()) { … }`. Cadono `Fx.ProfileFallsBackActionThenBaseThenShape` «🔴 senza ActionId in tabella, vince BaseActionId» **e** `Fx.BaseActionOverrideWinsOverShapeDefault` «🔴 una carica senza riga propria…».
4. In `DeclaredFxOverrideRows`, cancella `Riga(TEXT("Hero.Muiren.TideGuard"), …);`. Cade `Fx.DeclaredOverridesMatchTheProposal` «Hero.Muiren.TideGuard: il profilo e' quello approvato».
18. In `FxProfileForIn`, cancella `if (ActionId.IsNone() && BaseActionId.IsNone()) { return FRTAbilityFxProfile(); }`. Cade `Fx.NoActionNoProfile` sugli asserti «🔴 legacy Single: profilo tutto None» e «🔴 legacy Area: profilo tutto None» (➕ rev2.: il profilo torna quello della forma; `IsTracerEligible` non è toccata, è la mutazione (22)).
P1. In `DeclaredFxOverrideRows`, duplica la riga di `Hero.Ivrin.Feint`. Cade `Fx.DeclaredOverridesHaveNoDuplicateKeys` «🔴 Hero.Ivrin.Feint compare una volta sola» e «la mappa ha una chiave per riga».

---

### Task 2: Il tracer segue il profilo — volo dalla forma (R13), disegno dall'override, `Zigzag` puro

**Files:**
- Modify: `Turn/RTPlaybackLibrary.h:276-288` (commenti e dichiarazioni di `IsTracerEligible`/`TracerStyleFor`, «PROVVISORIA» `⌫`) e, dopo `:288`, le dichiarazioni nuove
- Modify: `Turn/RTPlaybackLibrary.cpp:1` (include) e `:130-148` (i due corpi), più le definizioni nuove dopo `:148`
- Modify: `Turn/RTTurnManager.h:685` (seam `PlaybackPhaseElapsedForTest`)
- Modify: `Turn/RTTurnManager.cpp:7951-7959` (il commento del volo: idoneità = forma di default)
- Test: `Tests/RTPlaybackLibraryTests.cpp` (include `:1-5`; riscrittura dichiarata di `:433-438`; test nuovi dopo la `}` di `FRTPrivacyTracerRhythmIsTheSameForEveryViewerTest::RunTest`, `:500`), `Tests/RTAbilityFxProfileTests.cpp` (un asserto in `Fx.NoActionNoProfile`), `Tests/RTPlaybackActivationTests.cpp` (un test prima di `#endif`, `:688`)

**Interfaces:**
- Produces (statiche C++ in `URTPlaybackLibrary`):
  ```cpp
  static bool IsTracerEligible(const FRTResolvedEvent& Ev);   // corpo nuovo, firma invariata
  static ERTTracerStyle TracerStyleFor(const FRTResolvedEvent& Ev, int32 ViewerTeamId);   // firma invariata
  static ERTTracerStyle TracerStyleForIn(const TMap<FName, FRTAbilityFxProfile>& Overrides, const FRTResolvedEvent& Ev, int32 ViewerTeamId);
  static void TracerPolyline(ERTTracerStyle Style, const FVector& From, const FVector& To, float Alpha, float HexSize, TArray<FVector>& OutPoints);
  ```
- Produces (in `ARTTurnManager`): `float PlaybackPhaseElapsedForTest() const;`
- Consumes: `URTPresentationBindingLibrary::DefaultFxProfileFor`, `FxProfileFor`, `FxProfileForIn`, `DeclaredFxOverrides` (Task 1).

➕ piano. `TracerStyleForIn` non è nella spec: serve alla **variante a tabella iniettata** del test riscritto (➕ rev2., punto 4), con la stessa disciplina di `FxProfileForIn`. `TracerStyleFor` la chiama con `DeclaredFxOverrides()`: nessuna seconda strada.

- [ ] **Step 0: Misura PRIMA — i test di mondo il cui ritmo può cambiare (spec §2.8, F8, ➕ rev2.)**

```bash
git grep -ln "EnsureDefaultAbilities\|AttackBeatTraceForTest\|NumPlaybackTracers" -- Source/RefactorTactics/Tests
git grep -n "Hero.Branth.Ram\|Action.Charge\|Hero.Aevik.LinearDischarge\|Hero.Ivrin.PassingBlade" -- Source/RefactorTactics/Tests
```

Alla re-review il secondo dava `RTPlaybackActivationTests.cpp:363`, `:497`, `:637`, `:650` e `RTPlaybackStopPredicateTests.cpp:975`. ➕ esecuzione (Task 2). All'esecuzione dava **anche** `Tests/RTAbilityActivatedTests.cpp` (righe con `Hero.Branth.Ram`): test `Turn.*` sulla timeline risolta, nessun tick del playback. E R13 dà un volo a **ogni** `Attack` con un id e forma di default `Single`/`Line`, non ai soli nomi del grep (`Ram`, `Charge`, `LinearDischarge`, `PassingBlade`): il filtro di questo step e dello Step 6 si è allargato alle famiglie `Playback`, `Privacy`, `Reactions`, `Turn`, `HexMatch`, `HexBotPlay`, `Actions`, `HexBlast`, `HexMapActor`, senza differenze d'esito. Rileggi **ogni** occorrenza e annota in `<scratchpad>/t2-ritmo.md` se asserisce un istante del Blast (un tick, un `L<i>`/`A<i>` in un tick dato, una durata). Poi lancia i file che ne contengono, **prima** del codice:

```text
RefactorTactics.Playback.ActivationPlaysTheCastCue+RefactorTactics.Playback.DashStepLandsOnCellsAfterTheActivations+RefactorTactics.Playback.DashPhaseOpensForActivationsOnly+RefactorTactics.Playback.ChargeImpactPlaysTheDashAttackClip+RefactorTactics.Playback.NextActionStopsOnPrepAndDashActivations+RefactorTactics.Playback.NextActionStopsAtTheActionBoundary+RefactorTactics.Playback.NextActionDoesNotStopTwiceWithinOneIntent+RefactorTactics.Playback.PhaseEndNetStopsWithThePause+RefactorTactics.Playback.NextPhaseStopsAtThePhaseBoundary+RefactorTactics.Playback.Tracer+RefactorTactics.Playback.HitArrivesAfterTheLaunch+RefactorTactics.Playback.AttackBeatsStayOrderedInOneTick+RefactorTactics.Privacy.UnseenAttacker+RefactorTactics.Playback.EveryAttackArrivesByPhaseEnd+RefactorTactics.Playback.EveryChannelIsFullyRevealedByPhaseEnd
```

(log `<scratchpad>/t2-prima.log`; i cinque nomi `NextAction…`/`PhaseEndNet…`/`NextPhase…` sono i test di `Tests/RTPlaybackStopPredicateTests.cpp` — nessuno comincia con `Stop`, misurato in review; la riga `:975` sta in uno di loro e usa `Ram` nel Dash. Rimisura l'elenco con `grep -n '"RefactorTactics\.' Source/RefactorTactics/Tests/RTPlaybackStopPredicateTests.cpp` e aggiungi un nome nuovo se ce n'è). Annota esiti per nome: lo Step 6 li rilancia e confronta.

- [ ] **Step 1: I test rossi**

`Tests/RTPlaybackLibraryTests.cpp`, dopo `#include "Turn/RTTurnRules.h"` (`:5`):

```cpp
#include "Turn/RTPresentationBinding.h" // #3578: il profilo FX, la tabella e il default per forma
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
```

**La riscrittura dichiarata** (spec §5.1, eccezione ai gate invariati). In `FRTPlaybackTracerStyleFollowsShapeTest::RunTest`, sostituisci le righe `:435-438` — da `FRTResolvedEvent Abilita = MakeTracerAttackEvent(ERTAbilityShape::Single);` a `TestFalse(TEXT("un'azione che non e' un attacco base non e' idonea"), URTPlaybackLibrary::IsTracerEligible(Abilita));` — con:

```cpp
	// ➕ #3578 — ECCEZIONE DICHIARATA nella PR (spec «il profilo FX» §5.1). ⏱️ *Fino a #3578 qui c'era
	// `TestFalse(IsTracerEligible(PassingBlade))`: l'idoneita' era «attacco base», ed era dichiarata provvisoria.*
	// Con R13 il VOLO lo decide la forma di default (Single → idonea) e il DISEGNO l'override (PassingBlade → None).
	FRTResolvedEvent Abilita = MakeTracerAttackEvent(ERTAbilityShape::Single);
	Abilita.ActionId = TEXT("Hero.Ivrin.PassingBlade");
	Abilita.BaseActionId = NAME_None;
	TestTrue(TEXT("un'azione con un id e forma Single e' idonea al VOLO (R13)"), URTPlaybackLibrary::IsTracerEligible(Abilita));
	// ⚠️ Questo asserto regge su una riga GIUDICATA della tabella (spec §2.2): la variante qui sotto prova il meccanismo.
	TestTrue(TEXT("🔴 ma PassingBlade non lo DISEGNA: override Tracer = None"),
		URTPlaybackLibrary::TracerStyleFor(Abilita, 0) == ERTTracerStyle::None);

	// ➕ rev2. La variante a tabella INIETTATA: un'azione Single sintetica con `Tracer = None` vola come la forma e
	// non si disegna, senza dipendere da nessuna riga dell'autore.
	TMap<FName, FRTAbilityFxProfile> Prova;
	Prova.Add(FName(TEXT("Hero.Prova.Lama")), URTPresentationBindingLibrary::MakeFxProfile(
		ERTActivationFxStyle::Ring, ERTTracerStyle::None, ERTImpactFxStyle::Marker, ERTFootprintFxStyle::None));
	FRTResolvedEvent Sintetica = MakeTracerAttackEvent(ERTAbilityShape::Single);
	Sintetica.ActionId = TEXT("Hero.Prova.Lama");
	Sintetica.BaseActionId = NAME_None;
	TestEqual(TEXT("tabella iniettata: il volo e' quello della forma"),
		URTPlaybackLibrary::TracerFlightFor(URTPlaybackLibrary::IsTracerEligible(Sintetica), 0.25f, 0.5f), 0.25f, RTTol);
	TestTrue(TEXT("tabella iniettata: lo stile e' None"),
		URTPlaybackLibrary::TracerStyleForIn(Prova, Sintetica, 0) == ERTTracerStyle::None);
	TestTrue(TEXT("controllo: senza la riga, il proiettile della forma"),
		URTPlaybackLibrary::TracerStyleForIn(TMap<FName, FRTAbilityFxProfile>(), Sintetica, 0) == ERTTracerStyle::Projectile);
```

Il resto del test (`:440-454`: la generica, l'irrisolto, il `Move`) **non cambia** e resta verde: `Action.BasicAttack` con `BaseActionId` vuoto ha un id, forma `Single`, nessuna riga → `Projectile`.

Dopo la `}` di `FRTPrivacyTracerRhythmIsTheSameForEveryViewerTest::RunTest` (`:500`; ancora: `TestEqual(TEXT("e lo stesso volo: l'idoneita' non legge chi guarda")`):

```cpp

// --- Il tracer come caso del profilo FX (#3578, spec «il profilo FX per abilita'» §2.2) --------------------------

/**
 * L'override decide il disegno: `LinearDischarge` (Line, non base) e' idonea e da' `Zigzag`; `Ram` e `Action.Charge`
 * volano come la loro forma e non si disegnano.
 * ✅ Validato per mutazioni (5) — il vecchio corpo di `IsTracerEligible` — e (17) — il volo letto dall'override.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackTracerFollowsTheProfileTest,
	"RefactorTactics.Playback.TracerFollowsTheProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackTracerFollowsTheProfileTest::RunTest(const FString&)
{
	FRTResolvedEvent Scarica = MakeTracerAttackEvent(ERTAbilityShape::Line);
	Scarica.ActionId = TEXT("Hero.Aevik.LinearDischarge");
	Scarica.BaseActionId = NAME_None;
	TestTrue(TEXT("🔴 LinearDischarge idonea"), URTPlaybackLibrary::IsTracerEligible(Scarica));
	TestTrue(TEXT("e disegnata a zigzag"), URTPlaybackLibrary::TracerStyleFor(Scarica, 0) == ERTTracerStyle::Zigzag);

	for (const TCHAR* Id : { TEXT("Hero.Branth.Ram"), TEXT("Action.Charge") })
	{
		FRTResolvedEvent Carica = MakeTracerAttackEvent(ERTAbilityShape::Single);
		Carica.ActionId = Id;
		Carica.BaseActionId = NAME_None;
		TestTrue(FString::Printf(TEXT("🔴 %s idonea al volo"), Id), URTPlaybackLibrary::IsTracerEligible(Carica));
		TestTrue(FString::Printf(TEXT("%s non disegnata"), Id), URTPlaybackLibrary::TracerStyleFor(Carica, 0) == ERTTracerStyle::None);
	}
	TestTrue(TEXT("controllo: un attacco base resta un proiettile"),
		URTPlaybackLibrary::TracerStyleFor(MakeTracerAttackEvent(ERTAbilityShape::Single), 0) == ERTTracerStyle::Projectile);
	return true;
}

/**
 * D5/R3 come REGOLA: per ogni attacco base del roster (lista = funzione del catalogo, `MakeHeroBasicAttack` scrive
 * `BaseActionId`, `Ability/RTHeroCatalogLibrary.cpp:162-168`) il tracer e' il default della forma e volo e stile sono
 * quelli di prima di #3578.
 * ✅ Validato per mutazione (6): un override `Zigzag` su `PressureJet` (righe E copia del test della tabella).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackBasicAttackTracersEqualShapeDefaultTest,
	"RefactorTactics.Playback.BasicAttackTracersEqualShapeDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackBasicAttackTracersEqualShapeDefaultTest::RunTest(const FString&)
{
	static const FName Base(TEXT("Action.BasicAttack"));
	int32 Visti = 0;
	for (const URTHeroData* Eroe : URTHeroCatalogLibrary::GetHeroRoster())
	{
		for (const URTActionData* Azione : Eroe ? Eroe->Actions : TArray<TObjectPtr<URTActionData>>())
		{
			if (!Azione || Azione->Def.BaseActionId != Base) { continue; }
			++Visti;
			const FString Id = Azione->Def.ActionId.ToString();
			const ERTAbilityShape Forma = Azione->Shape;
			TestTrue(FString::Printf(TEXT("🔴 %s: tracer == default della forma"), *Id),
				URTPresentationBindingLibrary::FxProfileFor(Azione->Def.ActionId, Base, Forma).Tracer
					== URTPresentationBindingLibrary::DefaultFxProfileFor(Forma).Tracer);

			FRTResolvedEvent Ev = MakeTracerAttackEvent(Forma);
			Ev.ActionId = Azione->Def.ActionId;
			Ev.BaseActionId = Base;
			// Lo stile «di oggi» e' il corpo di `RTPlaybackLibrary.cpp:147` prima di #3578: Line → getto, Single → proiettile.
			const ERTTracerStyle DiOggi = Forma == ERTAbilityShape::Line ? ERTTracerStyle::Jet
				: (Forma == ERTAbilityShape::Single ? ERTTracerStyle::Projectile : ERTTracerStyle::None);
			TestTrue(FString::Printf(TEXT("%s: lo stile di prima"), *Id), URTPlaybackLibrary::TracerStyleFor(Ev, 0) == DiOggi);
			TestEqual(FString::Printf(TEXT("%s: il volo di prima"), *Id),
				URTPlaybackLibrary::TracerFlightFor(URTPlaybackLibrary::IsTracerEligible(Ev), 0.25f, 0.5f),
				DiOggi != ERTTracerStyle::None ? 0.25f : 0.f, RTTol);
		}
	}
	TestTrue(TEXT("⛔ premessa: il catalogo dichiara attacchi base"), Visti > 0);
	return true;
}

/**
 * R13 e privacy (F9): il volo — quindi il ritmo — e' funzione della sola forma di default. `Ram` (override senza
 * tracer) vola come `ImpactShot`: un attaccante non visto non rivela col ritardo l'override della sua azione. E' la
 * stessa espressione che `BeginPlayback` usa per `PlaybackBlastFlights` (`RTTurnManager.cpp:7973-7974`).
 * ⚠️ ➕ rev2. Il ritmo e' «stesso volo a parita' di indice nella sequenza», non «lo stesso per ogni squadra»: l'indice
 * dipende dalle attivazioni visibili (`RTTurnManager.cpp:7952-7956`). Qui si prova il VOLO, che non legge chi guarda.
 * ✅ Validato per mutazioni (17) e (5).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrivacyFlightDependsOnShapeNotOverrideTest,
	"RefactorTactics.Privacy.FlightDependsOnShapeNotOverride",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrivacyFlightDependsOnShapeNotOverrideTest::RunTest(const FString&)
{
	FRTResolvedEvent Ram = MakeTracerAttackEvent(ERTAbilityShape::Single);
	Ram.ActionId = TEXT("Hero.Branth.Ram");
	Ram.BaseActionId = NAME_None;
	FRTResolvedEvent Tiro = MakeTracerAttackEvent(ERTAbilityShape::Single);
	Tiro.ActionId = TEXT("Hero.Branth.ImpactShot");
	if (!TestTrue(TEXT("⛔ premessa: i due colpi hanno disegni diversi"),
			URTPlaybackLibrary::TracerStyleFor(Ram, 0) != URTPlaybackLibrary::TracerStyleFor(Tiro, 0)))
	{
		return false;
	}
	for (const float A : { 0.1f, 0.5f, 1.0f })
	{
		const float VoloRam = URTPlaybackLibrary::TracerFlightFor(URTPlaybackLibrary::IsTracerEligible(Ram), 0.25f, A);
		const float VoloTiro = URTPlaybackLibrary::TracerFlightFor(URTPlaybackLibrary::IsTracerEligible(Tiro), 0.25f, A);
		TestTrue(FString::Printf(TEXT("premessa A=%.1f: il tiro vola"), A), VoloTiro > 0.f);
		TestEqual(FString::Printf(TEXT("🔴 A=%.1f: il volo di Ram e' quello di ImpactShot"), A), VoloRam, VoloTiro, RTTol);
	}
	return true;
}

/**
 * Lo `Zigzag` e' deterministico e CRESCE come un prefisso (spec §2.4, F11, F12): nessun `Rand`, nessun orologio, e
 * la linea a `α = 0.3` e' l'inizio di quella a `α = 0.6`. Ancorata all'origine, mai oltre l'impatto.
 * ✅ Validato per mutazione (19): lo scarto laterale calcolato da `Alpha` invece che dall'indice del vertice.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackTracerZigzagGrowsAsAPrefixTest,
	"RefactorTactics.Playback.TracerZigzagGrowsAsAPrefix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackTracerZigzagGrowsAsAPrefixTest::RunTest(const FString&)
{
	const FVector Da(0.f, 0.f, 60.f);
	const FVector A(300.f, 0.f, 60.f);
	const float S = 100.f;
	TArray<FVector> Corta, Lunga, Ancora, Piena;
	URTPlaybackLibrary::TracerPolyline(ERTTracerStyle::Zigzag, Da, A, 0.3f, S, Corta);
	URTPlaybackLibrary::TracerPolyline(ERTTracerStyle::Zigzag, Da, A, 0.6f, S, Lunga);
	URTPlaybackLibrary::TracerPolyline(ERTTracerStyle::Zigzag, Da, A, 0.3f, S, Ancora);
	URTPlaybackLibrary::TracerPolyline(ERTTracerStyle::Zigzag, Da, A, 1.0f, S, Piena);
	if (!TestTrue(TEXT("⛔ premessa: polilinee non degeneri"), Corta.Num() >= 2 && Lunga.Num() > Corta.Num()))
	{
		return false;
	}

	TestEqual(TEXT("deterministica: stessa chiamata, stessi punti"), Ancora.Num(), Corta.Num());
	for (int32 I = 0; I < FMath::Min(Ancora.Num(), Corta.Num()); ++I)
	{
		TestTrue(FString::Printf(TEXT("deterministica: punto %d"), I), Ancora[I].Equals(Corta[I], 0.01f));
	}
	TestTrue(TEXT("ancorata all'origine"), Corta[0].Equals(Da, 0.01f) && Lunga[0].Equals(Da, 0.01f));

	const int32 Ultimo = Corta.Num() - 1;
	for (int32 I = 0; I < Ultimo; ++I)
	{
		TestTrue(FString::Printf(TEXT("🔴 prefisso: il vertice %d della corta e' quello della lunga"), I), Corta[I].Equals(Lunga[I], 0.01f));
	}
	TestTrue(TEXT("🔴 prefisso: la punta della corta sta sul segmento corrispondente della lunga"),
		FMath::PointDistToSegment(Corta[Ultimo], Lunga[Ultimo - 1], Lunga[Ultimo]) < 0.01f);

	const FVector Asse = (A - Da).GetSafeNormal();
	for (const FVector& P : Piena)
	{
		TestTrue(TEXT("nessun punto oltre l'impatto"), FVector::DotProduct(P - Da, Asse) <= (A - Da).Size() + 0.01f);
	}
	TestEqual(TEXT("a α = 1: otto segmenti, nove vertici"), Piena.Num(), 9);
	TestTrue(TEXT("a α = 1 finisce sull'impatto"), Piena.Num() == 9 && Piena.Last().Equals(A, 0.01f));
	TestTrue(TEXT("e' davvero spezzata: il primo vertice interno e' scostato di 0.12 s"),
		Piena.Num() == 9 && FMath::IsNearlyEqual(FMath::Abs(Piena[1].Y), 0.12f * S, 0.01f));

	TArray<FVector> Getto;
	URTPlaybackLibrary::TracerPolyline(ERTTracerStyle::Jet, Da, A, 0.5f, S, Getto);
	TestEqual(TEXT("Projectile e Jet restano su TracerSegment: nessuna polilinea"), Getto.Num(), 0);
	return true;
}
```

`Tests/RTAbilityFxProfileTests.cpp`, in `FRTFxNoActionNoProfileTest::RunTest`, prima di `return true;` (mutazione (22), ➕ rev2.):

```cpp

	// 🔴 La clausola R12 PROPRIA di `IsTracerEligible`: legge `DefaultFxProfileFor`, non `FxProfileFor`, quindi la
	// guardia di `FxProfileForIn` non la copre (mutazione (22)).
	FRTResolvedEvent Legacy;
	Legacy.Type = ERTResolvedEventType::Attack;
	Legacy.Shape = ERTAbilityShape::Single;
	Legacy.HitGeometry.bResolved = true;
	TestFalse(TEXT("🔴 IsTracerEligible falso anche con geometria risolta"), URTPlaybackLibrary::IsTracerEligible(Legacy));
	TestEqual(TEXT("quindi nessun volo: il ritmo dei test con unita' legacy non cambia"),
		URTPlaybackLibrary::TracerFlightFor(URTPlaybackLibrary::IsTracerEligible(Legacy), 0.25f, 0.5f), 0.f);
	FRTResolvedEvent ConId = Legacy;
	ConId.ActionId = TEXT("Action.BasicAttack");
	TestTrue(TEXT("controllo: con un id la stessa geometria e' idonea"), URTPlaybackLibrary::IsTracerEligible(ConId));
```

`Tests/RTPlaybackActivationTests.cpp`, prima di `#endif // WITH_DEV_AUTOMATION_TESTS` (`:688`):

```cpp

/**
 * Regressione zero sul tracer di un attacco base, a mondo (spec §5.1, D5): a meta' volo il canale ha UN tracer con
 * `From`, `To`, `Style = Projectile` e `Alpha` uguali alle formule di prima di #3578.
 * 🔑 Fixture `CostruisciBeat` col viewer 0 (mondo inizializzato, viewer asserito): il Tiratore e' Branth in (0,0)
 * con `ImpactShot` sul bersaglio in (1,0). L'indice del colpo nella sequenza si legge dopo `LockInAndResolve`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackBasicAttackTracerIsUnchangedTest,
	"RefactorTactics.Playback.BasicAttackTracerIsUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackBasicAttackTracerIsUnchangedTest::RunTest(const FString&)
{
	FRTBeatDiProva B;
	const bool bOk = CostruisciBeat(*this, /*Viewer*/ 0, B);
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
	if (!bOk) { return false; }
	ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(B.World);
	if (!TestNotNull(TEXT("mappa"), Mappa)) { return false; }
	B.TM->LockInAndResolve();

	int32 K = INDEX_NONE;
	const TArray<int32> Sequenza = B.TM->PlaybackBlastSequenceIndicesForTest();
	for (int32 I = 0; I < Sequenza.Num(); ++I)
	{
		const FRTResolvedEvent& Ev = B.TM->ResolvedTimelineForTest()[Sequenza[I]];
		if (Ev.Type == ERTResolvedEventType::Attack && Ev.SourceStableUnitId == B.Tiratore->StableUnitId) { K = I; }
	}
	if (!TestTrue(TEXT("⛔ premessa: il colpo del tiratore e' nella sequenza del Blast"), K != INDEX_NONE)) { return false; }
	const float A = B.TM->AttackShowSeconds;
	const float Volo = FMath::Clamp(B.TM->TracerFlightSeconds, 0.f, 0.5f * A); // la formula di `TracerFlightFor`

	bool bVisto = false;
	for (int32 I = 0; I < 600 && B.TM->IsResolving(); ++I)
	{
		B.TM->Tick(0.02f);
		if (B.TM->CurrentPlaybackPhaseForTest() != ERTMatchPhase::Blast || Mappa->NumPlaybackTracers() != 1) { continue; }
		bVisto = true;
		const FRTPlaybackTracer& T = Mappa->GetPlaybackTracers()[0];
		TestTrue(TEXT("From = cella del tiratore"), T.From == FRTCellId(0, 0));
		TestTrue(TEXT("To = cella del bersaglio"), T.To == FRTCellId(1, 0));
		TestTrue(TEXT("🔴 Style = Projectile"), T.Style == ERTTracerStyle::Projectile);
		const float Atteso = FMath::Clamp((B.TM->PlaybackPhaseElapsedForTest() - K * A) / Volo, 0.f, 1.f);
		TestEqual(TEXT("Alpha = (t − k·A) / F"), T.Alpha, Atteso, 1e-3f);
	}
	TestTrue(TEXT("🔴 a meta' volo il tracer c'e'"), bVisto);
	return true;
}
```

- [ ] **Step 2: Compila e verifica che fallisca**

Run: build. Expected: **rosso di compilazione** — `TracerStyleForIn`, `TracerPolyline` e `PlaybackPhaseElapsedForTest` non esistono.

- [ ] **Step 3: Idoneità e stile**

`Turn/RTPlaybackLibrary.cpp`, dopo `#include "Turn/RTPlaybackLibrary.h"` (`:1`):

```cpp

#include "Turn/RTPresentationBinding.h" // #3578: il profilo FX — default per forma (volo) e override (disegno)
```

Sostituisci i corpi di `IsTracerEligible` e `TracerStyleFor` (`:130-148`, ancora: `static const FName BasicAttack(TEXT("Action.BasicAttack"));`) con:

```cpp
bool URTPlaybackLibrary::IsTracerEligible(const FRTResolvedEvent& Ev)
{
	// #3578 (spec «il profilo FX» §2.2, R12, R13): il VOLO — quindi il ritmo — e' la sola forma di DEFAULT di un'azione
	// con un id. Non legge l'override ne' chi guarda: un attaccante non visto non rivela col ritardo l'override della
	// sua azione. ⚠️ ➕ rev2. Il ritmo e' «stesso volo a parita' di indice nella sequenza», e l'indice dipende dalle
	// attivazioni che chi guarda ha il diritto di vedere (`BuildBlastSequence`, D6 del momento).
	// ⏱️ *Fino a #3578 l'idoneita' era «attacco base», dichiarata provvisoria (spec del tracer §2.1, condizione 1).*
	return Ev.Type == ERTResolvedEventType::Attack
		&& Ev.HitGeometry.bResolved
		&& !(Ev.ActionId.IsNone() && Ev.BaseActionId.IsNone()) // R12: nessuna azione, nessun volo
		&& URTPresentationBindingLibrary::DefaultFxProfileFor(Ev.Shape).Tracer != ERTTracerStyle::None;
}

ERTTracerStyle URTPlaybackLibrary::TracerStyleForIn(const TMap<FName, FRTAbilityFxProfile>& Overrides,
	const FRTResolvedEvent& Ev, int32 ViewerTeamId)
{
	if (!IsTracerEligible(Ev)
		|| !Ev.HitGeometry.FromVerdict.AllowsTeam(ViewerTeamId)
		|| !Ev.HitGeometry.ImpactVerdict.AllowsTeam(ViewerTeamId))
	{
		return ERTTracerStyle::None;
	}
	// Il DISEGNO: lo stile del profilo. `None` qui = stesso volo, nessun disegno (R13: `Ram`, `PassingBlade`).
	return URTPresentationBindingLibrary::FxProfileForIn(Overrides, Ev.ActionId, Ev.BaseActionId, Ev.Shape).Tracer;
}

ERTTracerStyle URTPlaybackLibrary::TracerStyleFor(const FRTResolvedEvent& Ev, int32 ViewerTeamId)
{
	return TracerStyleForIn(URTPresentationBindingLibrary::DeclaredFxOverrides(), Ev, ViewerTeamId);
}

void URTPlaybackLibrary::TracerPolyline(ERTTracerStyle Style, const FVector& From, const FVector& To, float Alpha,
	float HexSize, TArray<FVector>& OutPoints)
{
	OutPoints.Reset();
	if (Style != ERTTracerStyle::Zigzag)
	{
		return; // `Projectile` e `Jet` restano su `TracerSegment` (F11)
	}
	constexpr int32 Segmenti = 8;
	const FVector Asse = To - From;
	const FVector Laterale = FVector::CrossProduct(Asse, FVector::UpVector).GetSafeNormal();
	const float Scarto = 0.12f * HexSize;
	auto Vertice = [&](int32 I) -> FVector
	{
		if (I <= 0) { return From; }
		if (I >= Segmenti) { return To; }
		// 🔑 Il segno dall'INDICE del vertice, mai da `Alpha` (F12): e' cio' che rende la linea un prefisso che cresce.
		const float Segno = (I % 2 == 1) ? 1.f : -1.f;
		return From + Asse * (static_cast<float>(I) / Segmenti) + Laterale * (Segno * Scarto);
	};
	const float T = FMath::Clamp(Alpha, 0.f, 1.f) * Segmenti;
	const int32 Interi = FMath::Min(FMath::FloorToInt(T), Segmenti);
	for (int32 I = 0; I <= Interi; ++I)
	{
		OutPoints.Add(Vertice(I));
	}
	if (Interi < Segmenti && T - Interi > KINDA_SMALL_NUMBER)
	{
		OutPoints.Add(FMath::Lerp(Vertice(Interi), Vertice(Interi + 1), T - Interi));
	}
}
```

`Turn/RTPlaybackLibrary.h`, sostituisci i due commenti `:276-288` (ancora: `Idoneita' PROVVISORIA e dichiarata`) con:

```cpp
	/**
	 * Il VOLO di un colpo (#3578, spec «il profilo FX» §2.2, R12, R13): un `Attack` con geometria risolta, con un id
	 * (`ActionId` o `BaseActionId`), la cui forma di DEFAULT ha un tracer (`Single`, `Line`). Decide il RITMO: non
	 * legge l'override ne' chi guarda.
	 * ⌫ *Era «un attacco base, idoneita' PROVVISORIA»: la tabella del sotto-progetto 4 l'ha sostituita.*
	 */
	static bool IsTracerEligible(const FRTResolvedEvent& Ev);

	/**
	 * Lo stile del tracer per chi guarda: `None` se non idoneo o se la squadra non conosceva l'attaccante in `From`
	 * OPPURE la vittima in `Impact` (spec del tracer §0.3, P1); altrimenti il `Tracer` del profilo FX, che puo' essere
	 * `None` con lo stesso volo (R13). Decide il DISEGNO, mai il ritmo.
	 */
	static ERTTracerStyle TracerStyleFor(const FRTResolvedEvent& Ev, int32 ViewerTeamId);

	/** `TracerStyleFor` su una tabella d'override data (pura, per i test: come `FxProfileForIn`). */
	static ERTTracerStyle TracerStyleForIn(const TMap<FName, FRTAbilityFxProfile>& Overrides,
		const FRTResolvedEvent& Ev, int32 ViewerTeamId);

	/**
	 * La polilinea di uno `Zigzag` (#3578, spec §2.1, F11, F12): otto segmenti fra `From` e `To`, scarto laterale
	 * `±0.12·HexSize` alternato per INDICE di vertice, tagliata da `Alpha` — quindi a `α` minore e' un PREFISSO di quella
	 * a `α` maggiore. Pura: nessun `Rand`, nessun orologio. Per ogni altro stile `OutPoints` resta vuoto.
	 */
	static void TracerPolyline(ERTTracerStyle Style, const FVector& From, const FVector& To, float Alpha,
		float HexSize, TArray<FVector>& OutPoints);
```

`URTPlaybackLibrary.h` include già `Map/RTPlaybackTracer.h` (`:5`), quindi `FRTAbilityFxProfile` è visibile.

`Turn/RTTurnManager.h`, dopo `int32 PlaybackBlastShownForTest() const { return BlastElementsShown(); }` (`:685`):

```cpp

	/** L'orologio della fase di playback (#3578): l'oracolo con cui i test confrontano `Alpha` con le formule. Solo test. */
	float PlaybackPhaseElapsedForTest() const { return PlaybackPhaseElapsed; }
```

`Turn/RTTurnManager.cpp:7951`, nel commento sopra il ciclo dei voli (ancora: `// \`#2454\`: il volo di ogni elemento, deciso dall'idoneita'`), aggiungi in coda al blocco, prima di `int32 NumColpi = 0, …` (`:7960`):

```cpp
	// #3578: l'idoneita' e' la forma di DEFAULT di un'azione con un id (R12, R13, `IsTracerEligible`): un override che
	// toglie il disegno (`Ram`, `PassingBlade`) non toglie il volo, e un colpo non base con forma `Single`/`Line` ora vola.
```

- [ ] **Step 4: Build e verde**

Run: build; test `RefactorTactics.Fx+RefactorTactics.Playback.Tracer+RefactorTactics.Playback.BasicAttack+RefactorTactics.Privacy.FlightDependsOnShapeNotOverride+RefactorTactics.Privacy.TracerHiddenWhenOriginUnknown+RefactorTactics.Privacy.TracerRhythmIsTheSameForEveryViewer`, log `<scratchpad>/t2-verde.log`. Expected: tutti `Result={Success}`, compreso `Playback.TracerStyleFollowsShapeForBasicAttack` riscritto.

- [ ] **Step 5: I gate del tracer**

Run: `RefactorTactics.Playback+RefactorTactics.Privacy+RefactorTactics.Combat.AttackCarriesHitGeometry+RefactorTactics.Combat.CoveredHitCarriesHitGeometry+RefactorTactics.Determinism.HitGeometryStaysOutOfHashes+RefactorTactics.HexMapActor.PlaybackTracerIsItsOwnChannel+RefactorTactics.Preview.TracerDrawsWithDebugDrawingOff`, log `<scratchpad>/t2-gate.log`. Expected: verdi. Un rosso qui è una regressione o un cambio di ritmo: lo Step 6 lo classifica.

- [ ] **Step 6: Misura DOPO e confronto (➕ rev2.)**

Rilancia il filtro dello Step 0, log `<scratchpad>/t2-dopo.log`, e confronta per nome con `<scratchpad>/t2-prima.log` (`grep -o 'Test=.*Result={[A-Za-z]*}'` su entrambi, poi `diff`). Atteso: **nessuna** differenza d'esito. Se un test che usa `Ram`, `Action.Charge`, `LinearDischarge` o `PassingBlade` cambia esito, il volo nuovo (R13) gli ha spostato un istante: ci si ferma, si legge l'asserto con la nota di `<scratchpad>/t2-ritmo.md`, e si decide con l'autore se il test asseriva il ritmo vecchio (allora la PR lo dichiara come seconda eccezione) o se è una regressione (allora si corregge il codice). ⛔ Non si riscrive un test esistente senza dichiararlo.

- [ ] **Step 7: Commit, poi mutazioni (5), (6), (17), (19), (22), e (1) ripetuta**

```bash
git add Source/RefactorTactics/Turn/RTPlaybackLibrary.h Source/RefactorTactics/Turn/RTPlaybackLibrary.cpp Source/RefactorTactics/Turn/RTTurnManager.h Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Tests/RTPlaybackLibraryTests.cpp Source/RefactorTactics/Tests/RTAbilityFxProfileTests.cpp Source/RefactorTactics/Tests/RTPlaybackActivationTests.cpp
git commit -F - <<'EOF'
feat(3578): il tracer segue il profilo FX — volo dalla forma di default, disegno dall'override, zigzag puro

Riscrive l'asserto su PassingBlade di Playback.TracerStyleFollowsShapeForBasicAttack (eccezione dichiarata dalla spec).

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
```

Ogni mutazione: modifica, build, filtro `RefactorTactics.Fx+RefactorTactics.Playback.Tracer+RefactorTactics.Playback.BasicAttack+RefactorTactics.Privacy.FlightDependsOnShapeNotOverride`, `Result={Fail}` sugli asserti indicati, `git checkout -- <file>`, build, verde.

5. In `IsTracerEligible`, rimetti il corpo di prima (`static const FName BasicAttack(…)` e la condizione `(Ev.ActionId == BasicAttack || Ev.BaseActionId == BasicAttack) && (Ev.Shape == Single || Ev.Shape == Line) && Ev.HitGeometry.bResolved`). Cadono `Playback.TracerFollowsTheProfile` «🔴 LinearDischarge idonea» e «🔴 Hero.Branth.Ram idonea al volo», **e** (➕ rev2.) `Privacy.FlightDependsOnShapeNotOverride` «🔴 A=…: il volo di Ram e' quello di ImpactShot», **e** la riscrittura «un'azione con un id e forma Single e' idonea al VOLO».
6. Aggiungi `Riga(TEXT("Hero.Muiren.PressureJet"), URTPresentationBindingLibrary::MakeFxProfile(A::Ring, T::Zigzag, I::Marker, F::None));` in `DeclaredFxOverrideRows` **e** `Attesi.Add(TEXT("Hero.Muiren.PressureJet"), FxP(FxA::Ring, FxT::Zigzag, FxI::Marker, FxF::None));` in `Fx.DeclaredOverridesMatchTheProposal`. Cade `Playback.BasicAttackTracersEqualShapeDefault` «🔴 Hero.Muiren.PressureJet: tracer == default della forma» e «…: lo stile di prima»; `Fx.DeclaredOverridesMatchTheProposal` **resta verde** (la copia è allineata): è il punto della mutazione. Ripristina **entrambi** i file.
17. In `IsTracerEligible`, `URTPresentationBindingLibrary::DefaultFxProfileFor(Ev.Shape).Tracer` diventa `URTPresentationBindingLibrary::FxProfileFor(Ev.ActionId, Ev.BaseActionId, Ev.Shape).Tracer`. Cadono `Privacy.FlightDependsOnShapeNotOverride` «🔴 A=…: il volo di Ram e' quello di ImpactShot» **e** (➕ rev2.) `Playback.TracerFollowsTheProfile` «🔴 Hero.Branth.Ram idonea al volo».
19. In `TracerPolyline`, `const float Segno = (I % 2 == 1) ? 1.f : -1.f;` diventa `const float Segno = ((I + FMath::RoundToInt(Alpha * 10.f)) % 2 == 1) ? 1.f : -1.f;`. Cade `Playback.TracerZigzagGrowsAsAPrefix` «🔴 prefisso: il vertice 1 della corta e' quello della lunga» (a `α = 0.3` il segno è invertito rispetto a `α = 0.6`).
22. In `IsTracerEligible`, cancella la riga `&& !(Ev.ActionId.IsNone() && Ev.BaseActionId.IsNone()) // R12…`. Cade `Fx.NoActionNoProfile` «🔴 IsTracerEligible falso anche con geometria risolta» e «quindi nessun volo…» (➕ rev2.).
1 (ripetuta). La mutazione (1) del Task 1: ora cadono `Fx.DefaultProfileFollowsShape` «🔴 Line → Jet» **e** `Playback.TracerStyleFollowsShapeForBasicAttack` «Line -> getto» (`:428-429`) e `Playback.BasicAttackTracersEqualShapeDefault` «Hero.Muiren.PressureJet: lo stile di prima».

---

### Task 3: Il segnale di attivazione — `Ev.Origin`, la cue pura, il canale di mappa, la consegna

**Files:**
- Modify: `Turn/RTTurnManager.cpp:337-346` (`EmitAbilityActivated`: `Ev.Origin` e il soggetto letto una volta, ancora `Ev.SourceVerdict = FreezeVerdictFor(FRTLogSubject::Unit(Source));` a `:344`)
- Modify: `Turn/RTTurnManager.cpp:8853-8862` (ramo Prep/Dash del tick), `:8874` (`ON_SCOPE_EXIT` del Blast), `:8932-8940` e `:8963-8967` (spegnimento a fine fase), `:9193` e `:9199` (`FinishPlayback`), e dopo `:8366` (`PushPlaybackCues`, accanto a `PushPlaybackTracers`)
- Modify: `Turn/RTTurnManager.h:636-640` (gancio `bSkipFxFieldsForTest`), `:1316-1318` (due manopole), `:2441-2447` (dichiarazione), `:3480-3485` (flag del canale)
- Modify: `Turn/RTPlaybackLibrary.h` (dopo `TracerPolyline`, Task 2) e `.cpp` (dopo `TracerPolyline`)
- Modify: `Turn/RTResolvedEvent.h:407-409` (nota dei campi: `Origin` anche per `AbilityActivated`) e `:436-444` (commento di `Origin`)
- Modify: `Map/RTHexMapActor.h:917-930` (il canale) e `:1095` (il membro); `Map/RTHexMapActor.cpp:1093-1108` (`HasAnythingToDraw`) e dopo `:1188` (`Set`/`Clear`)
- Test: `Tests/RTAbilityFxProfileTests.cpp`, `Tests/RTPlaybackActivationTests.cpp`, `Tests/RTAttackTracerTests.cpp` (prima di `#endif`, `:270`), `Tests/RTHexMapActorTests.cpp` (prima di `#endif`, `:2169`)

**Interfaces:**
- Produces (statiche C++ in `URTPlaybackLibrary`):
  ```cpp
  static float ActivationCueDuration(float ActivationCueSeconds, float AttackShowSeconds);   // D_act
  static bool ActivationCueFor(const FRTResolvedEvent& Ev, int32 ViewerTeamId, float Alpha, FRTPlaybackCue& OutCue);
  static void ActivationCuesAt(const TArray<FRTResolvedEvent>& Activations, int32 Shown, float PhaseElapsed,
      float AttackShowSeconds, float ActivationCueSeconds, int32 ViewerTeamId, TArray<FRTPlaybackCue>& Out);   // Prep, Dash
  static void BlastActivationCuesAt(const TArray<FRTResolvedEvent>& Timeline, const TArray<FRTBlastSequenceElement>& Sequence,
      int32 BeatsDone, float PhaseElapsed, float AttackShowSeconds, float ActivationCueSeconds, int32 ViewerTeamId,
      TArray<FRTPlaybackCue>& Out);
  ```
- Produces (in `ARTTurnManager`): `float ActivationCueSeconds = 0.35f;`, `float ImpactCueSeconds = 0.20f;` (`UPROPERTY`), `bool bSkipFxFieldsForTest = false;`, `void PushPlaybackCues(ERTMatchPhase Phase);`, `bool bPlaybackCueChannelFull = false;`
- Produces (in `ARTHexMapActor`): `void SetPlaybackCues(const TArray<FRTPlaybackCue>& Cues);`, `void ClearPlaybackCues();`, `int32 NumPlaybackCues() const;`, `const TArray<FRTPlaybackCue>& GetPlaybackCues() const;`
- Consumes: `FRTPlaybackCue`, `ERTPlaybackCueKind` (Task 1); `FxProfileFor` (Task 1); `AttackLaunchSeconds` (`Turn/RTPlaybackLibrary.h:256`); `FRTLogSubject::Unit`, `GetFactCell` (`Turn/RTCombatLog.h:26`, `:82`).

➕ piano. (i) `PushPlaybackCues` prende la fase come argomento (la spec scrive `PushPlaybackCues()`): `TickPlayback` la conosce già come `Ph`, e un secondo modo di leggerla sarebbe una seconda risposta. (ii) Il canale della mappa (`Set`/`Clear`/`Get`, `HasAnythingToDraw`, mutazione (15)) sta **qui** e non nel Task 5: il test d'attivazione ne è il primo lettore; il Task 5 ne fa il **disegno**. (iii) Le due manopole nascono insieme (R2 è una coppia); `ImpactCueSeconds` ha il primo lettore nel Task 4. (iv) Le manopole usano gli specificatori **delle vicine reali** (`Turn/RTTurnManager.h:1305-1318`: `EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback"`, **senza** `meta = (ClampMin…)`): la spec (F18) chiede «gli stessi specificatori delle vicine» e cita un `meta` che le vicine non hanno; vince la regola, non la citazione. Il taglio a zero lo fa `ActivationCueDuration`.

- [ ] **Step 1: I test rossi**

`Tests/RTAbilityFxProfileTests.cpp`, prima di `// --- I test dei Task 3, 4 e 5 si aggiungono QUI`:

```cpp

// --- La cue d'attivazione (#3578, spec §2.3) -------------------------------------------------------------------

namespace
{
	/** Un'attivazione visibile alla squadra 0, con la sorgente in `Origine`. */
	FRTResolvedEvent FxAttivazione(const TCHAR* Azione, ERTAbilityShape Forma, const FRTCellId& Origine)
	{
		FRTResolvedEvent Ev;
		Ev.Phase = ERTMatchPhase::Prep;
		Ev.Type = ERTResolvedEventType::AbilityActivated;
		Ev.SourceStableUnitId = 1;
		Ev.ActionId = Azione;
		Ev.Shape = Forma;
		Ev.Origin = Origine;
		Ev.SourceVerdict.AllowTeam(0);
		return Ev;
	}
}

/**
 * R7: la funzione pura della cue d'attivazione RICONTROLLA `SourceVerdict` (le code sono gia' filtrate a monte, e
 * questa e' la seconda porta). Lo stile e' quello del profilo, la cella e' `Ev.Origin`.
 * ✅ Validato per mutazione (8): il controllo di `SourceVerdict` tolto fa cadere «chi non vede la sorgente».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrivacyActivationCueNeedsTheSourceTest,
	"RefactorTactics.Privacy.ActivationCueNeedsTheSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrivacyActivationCueNeedsTheSourceTest::RunTest(const FString&)
{
	const FRTResolvedEvent Scudo = FxAttivazione(TEXT("Hero.Muiren.TideGuard"), ERTAbilityShape::Single, FRTCellId(2, 1));
	FRTPlaybackCue Cue;
	TestFalse(TEXT("🔴 chi non vede la sorgente non riceve la cue"), URTPlaybackLibrary::ActivationCueFor(Scudo, 1, 0.5f, Cue));
	TestFalse(TEXT("un osservatore fuori intervallo non legge"), URTPlaybackLibrary::ActivationCueFor(Scudo, -1, 0.5f, Cue));
	if (TestTrue(TEXT("chi la vede riceve la cue"), URTPlaybackLibrary::ActivationCueFor(Scudo, 0, 0.5f, Cue)))
	{
		TestTrue(TEXT("TideGuard: Pulse (override)"), Cue.Kind == ERTPlaybackCueKind::Pulse);
		TestTrue(TEXT("sulla cella dell'evento, non dell'attore"), Cue.At == FRTCellId(2, 1));
		TestEqual(TEXT("con l'Alpha data"), Cue.Alpha, 0.5f);
	}
	FRTPlaybackCue Altra;
	TestTrue(TEXT("Overload: Flash"), URTPlaybackLibrary::ActivationCueFor(
		FxAttivazione(TEXT("Hero.Aevik.Overload"), ERTAbilityShape::Area, FRTCellId(0, 0)), 0, 0.f, Altra)
		&& Altra.Kind == ERTPlaybackCueKind::Flash);
	TestTrue(TEXT("un attacco base: Ring (default)"), URTPlaybackLibrary::ActivationCueFor(
		FxAttivazione(TEXT("Hero.Branth.ImpactShot"), ERTAbilityShape::Single, FRTCellId(0, 0)), 0, 0.f, Altra)
		&& Altra.Kind == ERTPlaybackCueKind::Ring);
	FRTResolvedEvent Colpo = Scudo;
	Colpo.Type = ERTResolvedEventType::Attack;
	TestFalse(TEXT("solo un AbilityActivated ha una cue d'attivazione"), URTPlaybackLibrary::ActivationCueFor(Colpo, 0, 0.f, Altra));
	return true;
}
```

`Tests/RTPlaybackActivationTests.cpp`: nel namespace anonimo, prima della sua `}` di chiusura (`:138`, ancora: `.PerAction.FindOrAdd(ActionId).PerRole.Add(Ruolo, Pool);` seguita da `}` e `}`), aggiungi:

```cpp

	// --- Le cue d'attivazione (#3578, spec «il profilo FX» §2.3) ------------------------------------------------

	/** L'`AbilityActivated` della sorgente data, letto DOPO `LockInAndResolve` (prima il `StableUnitId` vale 0). */
	const FRTResolvedEvent* BeatAttivazioneDi(const ARTTurnManager* TM, const ARTUnit* Sorgente)
	{
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Sorgente && Ev.SourceStableUnitId == Sorgente->StableUnitId)
			{
				return &Ev;
			}
		}
		return nullptr;
	}

	bool BeatHaCue(const ARTHexMapActor* Mappa, ERTPlaybackCueKind Tipo, const FRTCellId& Cella)
	{
		return Mappa && Mappa->GetPlaybackCues().ContainsByPredicate([&](const FRTPlaybackCue& C)
		{
			return C.Kind == Tipo && C.At == Cella;
		});
	}

	bool BeatHaCueDiAttivazione(const ARTHexMapActor* Mappa)
	{
		return Mappa && Mappa->GetPlaybackCues().ContainsByPredicate([](const FRTPlaybackCue& C)
		{
			return C.Kind == ERTPlaybackCueKind::Ring || C.Kind == ERTPlaybackCueKind::Pulse || C.Kind == ERTPlaybackCueKind::Flash;
		});
	}
```

Prima di `#endif` (dopo il test del Task 2):

```cpp

/**
 * La cue d'attivazione esce sulla cella dell'EVENTO, nell'istante della rivelazione, e si spegne dopo `D_act` — spec
 * «il profilo FX» §2.3, R5, R6. Tre fasi: `Pulse` di TideGuard in Prep, `Ring` di ImpactShot nel Blast, `Ring` di Ram
 * nel Dash. E a ogni cambio di fase il canale e' vuoto.
 *
 * 🔴 **La premessa di F15**: nella prima fixture nessuna sorgente si sposta, quindi `Ev.Origin == Src->Cell` e una
 * cue letta dall'attore passerebbe. La seconda meta' usa la carica di `Playback.ChargeImpactPlaysTheDashAttackClip`:
 * Branth parte da (1,0) e finisce addosso al bersaglio — `Ev.Origin != Src->Cell` a fine risoluzione, ASSERITO.
 * ✅ Validato per mutazione (7): la cella presa da `Src->Cell` fa cadere «Ring di Ram su Ev.Origin».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackActivationCueAtTheSourceCellTest,
	"RefactorTactics.Playback.ActivationCueAtTheSourceCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackActivationCueAtTheSourceCellTest::RunTest(const FString&)
{
	{
		FRTBeatDiProva B;
		const bool bOk = CostruisciBeat(*this, /*Viewer*/ 0, B);
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
		if (!bOk) { return false; }
		ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(B.World);
		B.TM->LockInAndResolve();
		const FRTResolvedEvent* AttScudo = BeatAttivazioneDi(B.TM, B.Scudo);
		const FRTResolvedEvent* AttTiratore = BeatAttivazioneDi(B.TM, B.Tiratore);
		if (!TestTrue(TEXT("⛔ premessa: mappa e due attivazioni in timeline"), Mappa && AttScudo && AttTiratore)) { return false; }
		const FRTCellId OrigineScudo = AttScudo->Origin;
		const FRTCellId OrigineTiratore = AttTiratore->Origin;
		const float Dact = URTPlaybackLibrary::ActivationCueDuration(B.TM->ActivationCueSeconds, B.TM->AttackShowSeconds);

		// ⚠️ «A fine fase il canale e' vuoto» si misura per CELLA E TIPO della fase lasciata, non come canale vuoto al
		// primo tick della fase dopo: il Blast rivela la sua prima attivazione a t = 0, nello stesso tick dell'ingresso.
		bool bPulse = false, bRing = false, bPulseOltre = false, bPulseFuoriDallaPrep = false, bRingFuoriDalBlast = false;
		float RivScudo = -1.f;
		for (int32 I = 0; I < 600 && B.TM->IsResolving(); ++I)
		{
			B.TM->Tick(0.02f);
			const ERTMatchPhase Fase = B.TM->CurrentPlaybackPhaseForTest();
			const float T = B.TM->PlaybackPhaseElapsedForTest();
			bPulseFuoriDallaPrep |= RivScudo >= 0.f && Fase != ERTMatchPhase::Prep
				&& BeatHaCue(Mappa, ERTPlaybackCueKind::Pulse, OrigineScudo);
			bRingFuoriDalBlast |= bRing && Fase != ERTMatchPhase::Blast
				&& BeatHaCue(Mappa, ERTPlaybackCueKind::Ring, OrigineTiratore);
			if (RivScudo < 0.f && B.Scudo->CastCuesPlayedForTest() == 1)
			{
				RivScudo = T;
				bPulse = BeatHaCue(Mappa, ERTPlaybackCueKind::Pulse, OrigineScudo);
			}
			else if (RivScudo >= 0.f && Fase == ERTMatchPhase::Prep && T >= RivScudo + Dact)
			{
				bPulseOltre |= BeatHaCue(Mappa, ERTPlaybackCueKind::Pulse, OrigineScudo);
			}
			if (!bRing && Fase == ERTMatchPhase::Blast && B.Tiratore->CastCuesPlayedForTest() == 1)
			{
				bRing = BeatHaCue(Mappa, ERTPlaybackCueKind::Ring, OrigineTiratore);
			}
		}
		TestTrue(TEXT("🔴 Pulse su Ev.Origin dello scudo, al tick della rivelazione in Prep"), bPulse);
		TestTrue(TEXT("🔴 Ring su Ev.Origin del tiratore, al tick della rivelazione nel Blast"), bRing);
		TestFalse(TEXT("dopo D_act il Pulse non c'e' piu'"), bPulseOltre);
		TestFalse(TEXT("finita la Prep, il Pulse non sopravvive"), bPulseFuoriDallaPrep);
		TestFalse(TEXT("finito il Blast, il Ring non sopravvive"), bRingFuoriDalBlast);
		TestEqual(TEXT("a fine playback il canale e' vuoto"), Mappa->NumPlaybackCues(), 0);
	}
	{
		// La carica: viewer di ripiego (squadra 0, mondo non inizializzato), come `ChargeImpactPlaysTheDashAttackClip`.
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		BeatMappa(World, 8);
		ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(World);
		ARTUnit* Caricatore = SpawnBeatUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0));
		ARTUnit* Bersaglio  = SpawnBeatUnit(World, 1, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(-1, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Caricatore || !Bersaglio || !Mappa) { return false; }
		const int32 IndiceRam = BeatIndiceAbilita(Caricatore, TEXT("Hero.Branth.Ram"));
		if (!TestTrue(TEXT("⛔ premessa: Branth ha Ram"), IndiceRam != INDEX_NONE)) { return false; }
		Caricatore->PlannedDashAbility = IndiceRam;
		Caricatore->PlannedDashCell = Bersaglio->Cell;
		Caricatore->PlannedCell = Caricatore->Cell;
		TM->RefreshTeamKnowledgeNow();
		TM->LockInAndResolve();

		const FRTResolvedEvent* AttRam = BeatAttivazioneDi(TM, Caricatore);
		if (!TestTrue(TEXT("⛔ premessa: l'attivazione di Ram e' in timeline, nel Dash"),
				AttRam && AttRam->Phase == ERTMatchPhase::Dash)
			|| !TestTrue(TEXT("⛔ premessa F15: Ev.Origin e' la cella di partenza"), AttRam->Origin == FRTCellId(1, 0))
			|| !TestFalse(TEXT("⛔ premessa F15: e a fine risoluzione l'attore NON e' piu' li'"), Caricatore->Cell == AttRam->Origin))
		{
			return false;
		}
		const FRTCellId OrigineRam = AttRam->Origin;
		// ⚠️ Nessun `break`: il ciclo arriva alla fine del playback, cosi' si misura anche che il `Ring` del Dash non
		// sopravvive alla sua fase (spec §5.1: canale vuoto a fine Prep, Dash e Blast).
		bool bRingRam = false, bRingFuoriDalDash = false, bRivelata = false;
		for (int32 I = 0; I < 600 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.02f);
			const ERTMatchPhase Fase = TM->CurrentPlaybackPhaseForTest();
			if (!bRivelata && Caricatore->CastCuesPlayedForTest() == 1)
			{
				bRivelata = true;
				bRingRam = BeatHaCue(Mappa, ERTPlaybackCueKind::Ring, OrigineRam);
			}
			bRingFuoriDalDash |= bRivelata && Fase != ERTMatchPhase::Dash
				&& BeatHaCue(Mappa, ERTPlaybackCueKind::Ring, OrigineRam);
		}
		TestTrue(TEXT("🔴 Ring di Ram su Ev.Origin, non sulla cella dell'attore"), bRingRam);
		TestFalse(TEXT("finito il Dash, il Ring di Ram non sopravvive"), bRingFuoriDalDash);
		TestEqual(TEXT("a fine playback il canale e' vuoto"), Mappa->NumPlaybackCues(), 0);
	}
	return true;
}

/**
 * Review Focus (a): chi non vede la sorgente non riceve NESSUNA cue d'attivazione su nessun tick, e la riga
 * `Attiva:` resta congelata sul verdetto (assente per quella squadra). Controllo positivo col viewer 0.
 * 🔑 Fixture di `Playback.HiddenSourceHasNoActivationBeat` (`:182`): viewer 7, squadra senza unita' in campo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrivacyHiddenSourceDeliversNoActivationCueTest,
	"RefactorTactics.Privacy.HiddenSourceDeliversNoActivationCue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrivacyHiddenSourceDeliversNoActivationCueTest::RunTest(const FString&)
{
	for (const int32 Viewer : { 7, 0 })
	{
		FRTBeatDiProva B;
		const bool bOk = CostruisciBeat(*this, Viewer, B);
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
		if (!bOk) { return false; }
		ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(B.World);
		B.TM->LockInAndResolve();
		bool bVista = false;
		for (int32 I = 0; I < 600 && B.TM->IsResolving(); ++I)
		{
			B.TM->Tick(0.02f);
			bVista |= BeatHaCueDiAttivazione(Mappa);
		}
		if (Viewer == 7)
		{
			TestFalse(TEXT("🔴 sorgente non osservata: nessuna cue d'attivazione su nessun tick"), bVista);
			TestFalse(TEXT("e la riga Attiva: resta nascosta a quella squadra (verdetto congelato)"), HaRigaAttiva(B.TM, 7));
		}
		else
		{
			TestTrue(TEXT("controllo positivo: il viewer 0 vede le cue d'attivazione"), bVista);
			TestTrue(TEXT("e la riga Attiva:"), HaRigaAttiva(B.TM, 0));
		}
	}
	return true;
}
```

`Tests/RTAttackTracerTests.cpp`, prima di `#endif` (`:270`):

```cpp

/**
 * Il solo campo nuovo di #3578 — `Ev.Origin` di `AbilityActivated` — non entra in `StateHash` ne' in `HashTurnLog`.
 * Gemello di `HitGeometryStaysOutOfHashes` (sopra): stessa fixture, stesso calcolo dello hash (⚠️ non
 * `GetPendingFinalStateHash()`, che vale 0 senza registrazione), gancio `bSkipFxFieldsForTest`.
 * ⚠️ **E' un TRIPWIRE, come il gemello**: oggi ne' `HashMatchState` ne' `HashTurnLog` leggono `ResolvedTimeline`, quindi
 * con il codice attuale non puo' diventare rosso; il controllo positivo prova che il gancio agisce, non che il confronto
 * vedrebbe una fuga. Cade il giorno in cui qualcuno scrive `Origin` in un dato hashato — e la PR lo dichiara cosi'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDeterminismFxFieldsOutOfHashesTest,
	"RefactorTactics.Determinism.FxFieldsStayOutOfHashes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDeterminismFxFieldsOutOfHashesTest::RunTest(const FString&)
{
	int64 StatoHash[2] = { 0, 0 };
	uint32 LogHash[2] = { 0, 0 };
	FRTCellId Origine[2];
	bool bTrovata[2] = { false, false };
	for (int32 Run = 0; Run < 2; ++Run)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		const URTHexMapAsset* Map = SpawnTracerMap(World);
		ARTUnit* Ivrin = SpawnTracerUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(1, 2));
		ARTUnit* Branth = SpawnTracerUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(3, 2));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Ivrin || !Branth) { AddError(TEXT("allestimento fallito")); return false; }
		TM->bSkipFxFieldsForTest = (Run == 1);
		Ivrin->PlannedAbilityIndex = 0;
		Ivrin->PlannedAttackTarget = Branth;
		Ivrin->PlannedCell = Ivrin->Cell;

		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

		TArray<ARTUnit*> Unita;
		Unita.Add(Ivrin);
		Unita.Add(Branth);
		TArray<int32> Punteggi;
		Punteggi.Add(TM->GetTeamScore(0));
		Punteggi.Add(TM->GetTeamScore(1));
		StatoHash[Run] = static_cast<int64>(URTMatchStateHashLibrary::HashMatchState(
			Map, URTMatchStateHashLibrary::BuildUnitDigests(Unita), Punteggi));
		LogHash[Run] = URTTurnLogLibrary::HashTurnLog(TM->GetTurnLog());
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.SourceStableUnitId == Ivrin->StableUnitId)
			{
				Origine[Run] = Ev.Origin;
				bTrovata[Run] = true;
			}
		}
	}
	// Controllo positivo: il gancio ha davvero tolto il campo, o il confronto non misurerebbe niente.
	TestTrue(TEXT("premessa: l'attivazione di Ivrin c'e' in entrambe le run"), bTrovata[0] && bTrovata[1]);
	TestTrue(TEXT("controllo: senza gancio, Origin e' la cella di Ivrin"), Origine[0] == FRTCellId(1, 2));
	TestTrue(TEXT("controllo: col gancio, Origin e' vuoto"), Origine[1] == FRTCellId());
	TestNotEqual(TEXT("premessa: lo StateHash e' stato catturato"), StatoHash[0], (int64)0);
	TestEqual(TEXT("🔴 StateHash identico con e senza Origin"), StatoHash[0], StatoHash[1]);
	TestEqual(TEXT("🔴 HashTurnLog identico con e senza Origin"), LogHash[0], LogHash[1]);
	return true;
}
```

`Tests/RTHexMapActorTests.cpp`, in coda al file, subito prima di `#endif // WITH_DEV_AUTOMATION_TESTS` (`:2169`; rimisura con `grep -n "^#endif" Source/RefactorTactics/Tests/RTHexMapActorTests.cpp | tail -1`). ⚠️ La `}` di `:2167` chiude il `RunTest` dell'ultimo test, non un namespace: il namespace anonimo degli helper (`MakeMapActorWorld`, `SpawnMapActor`, `MakeActorTestAsset`) chiude a `:352`, e i test stanno fuori da esso. Il test nuovo va **dopo** quella `}` e **prima** di `#endif`:

```cpp

/**
 * Il canale delle cue (#3578, spec «il profilo FX» §2.4, R10): SOSTITUISCE in blocco, si spegne da solo, non tocca
 * tracer, impronta ne' anteprima, e accende il Tick (il gemello di `PlaybackTracerIsItsOwnChannel`, qui sopra).
 * ✅ Validato per mutazioni (15) — `Append` invece dell'assegnazione — e (P5) — il canale fuori da `HasAnythingToDraw`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorPlaybackCueChannelTest,
	"RefactorTactics.HexMapActor.PlaybackCueIsItsOwnChannel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorPlaybackCueChannelTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }
	ARTHexMapActor* Actor = SpawnMapActor(World, MakeActorTestAsset(/*Radius*/ 1));
	TestNotNull(TEXT("actor spawnato"), Actor);
	if (!Actor) { DestroyMapActorWorld(World); return false; }

	FRTPlaybackCue C;
	C.Kind = ERTPlaybackCueKind::Ring;
	C.At = FRTCellId(0, 0);
	C.Toward = C.At;
	C.Alpha = 0.5f;
	FRTPlaybackTracer T;
	T.From = FRTCellId(0, 0);
	T.To = FRTCellId(1, 0);
	T.Alpha = 0.5f;
	T.Style = ERTTracerStyle::Projectile;

	TestEqual(TEXT("si parte senza cue"), Actor->NumPlaybackCues(), 0);
	TestFalse(TEXT("premessa: l'actor parte col Tick spento"), Actor->IsActorTickEnabled());
	Actor->SetPlaybackCues({ C });
	TestEqual(TEXT("una cue dopo la consegna"), Actor->NumPlaybackCues(), 1);
	TestTrue(TEXT("🔴 consegnata una cue, il Tick si accende"), Actor->IsActorTickEnabled());
	Actor->SetPlaybackCues({ C });
	TestEqual(TEXT("🔴 la seconda consegna sostituisce la prima"), Actor->NumPlaybackCues(), 1);

	Actor->SetPlaybackTracers({ T });
	Actor->ClearPlaybackTracers();
	Actor->AddPlaybackFootprint({ FRTCellId(1, 0) });
	Actor->ClearPlaybackFootprint();
	Actor->SetPreviewHitCells({ FRTCellId(0, 0) }, {});
	Actor->SetPreviewHitCells({}, {});
	TestEqual(TEXT("spenti tracer, impronta e anteprima, la cue resta"), Actor->NumPlaybackCues(), 1);

	Actor->SetPlaybackTracers({ T });
	Actor->ClearPlaybackCues();
	TestEqual(TEXT("dopo Clear non resta nessuna cue"), Actor->NumPlaybackCues(), 0);
	TestEqual(TEXT("e il tracer non e' stato toccato"), Actor->NumPlaybackTracers(), 1);
	Actor->ClearPlaybackTracers();
	TestFalse(TEXT("a tutti i canali vuoti il Tick e' spento"), Actor->IsActorTickEnabled());

	Actor->SetPlaybackCues({ C });
	Actor->SetPlaybackCues({});
	TestEqual(TEXT("una consegna vuota e' un canale vuoto"), Actor->NumPlaybackCues(), 0);
	TestFalse(TEXT("e spegne il Tick, ultimo canale acceso"), Actor->IsActorTickEnabled());

	DestroyMapActorWorld(World);
	return true;
}
```

- [ ] **Step 2: Compila e verifica che fallisca**

Run: build. Expected: **rosso di compilazione** — `ActivationCueFor`, `ActivationCueDuration`, `NumPlaybackCues`, `SetPlaybackCues`, `bSkipFxFieldsForTest`, `ActivationCueSeconds` non esistono.

- [ ] **Step 3: Il produttore — `Ev.Origin`, il soggetto letto una volta**

`Turn/RTTurnManager.cpp`, in `EmitAbilityActivated`, sostituisci le righe `:342-344` (da `// [D-223], spec §2.1: il verdetto si decide ADESSO` a `Ev.SourceVerdict = FreezeVerdictFor(FRTLogSubject::Unit(Source));`) con:

```cpp
	// [D-223], spec §2.1: il verdetto si decide ADESSO, quando l'unita' agisce, e si trasporta. Lo stesso valore
	// fa da verdetto alla riga `Attiva:` del playback (`ShowActivation`): calcolato una volta, mai due.
	// #3578 (spec «il profilo FX» §2.3, R5, F16): **il soggetto si legge UNA volta**, come `GetFactCell` chiede
	// (`RTCombatLog.h:82`). `Unit()` non dichiara una cella, quindi `GetFactCell()` e' quella dell'Actor — ed e' su
	// quella che `FreezeVerdictFor` congela: cella della cue e verdetto coincidono per costruzione. Nel Dash e' la cella
	// da cui lo scatto PARTE. `Origin` e' presentazione: `FRTResolvedEvent` e' `RTServerOnly`, fuori dagli hash
	// (`Determinism.FxFieldsStayOutOfHashes`).
	const FRTLogSubject Soggetto = FRTLogSubject::Unit(Source);
	if (!bSkipFxFieldsForTest)
	{
		Ev.Origin = Soggetto.GetFactCell();
	}
	Ev.SourceVerdict = FreezeVerdictFor(Soggetto);
```

`Turn/RTTurnManager.h`, dopo `bool bSkipHitGeometryForTest = false;` (`:640`):

```cpp

	/**
	 * Lascia vuoto `Origin` di `AbilityActivated` (#3578), l'unico campo che il profilo FX aggiunge al produttore. Esiste
	 * per un test solo — `Determinism.FxFieldsStayOutOfHashes`. Membro C++ nudo, non `UPROPERTY` (F20).
	 */
	bool bSkipFxFieldsForTest = false;
```

`Turn/RTResolvedEvent.h`, nella nota dei campi (`:407-409`, ancora: `\`AimCell\` e \`Shape\` anche per \`AbilityActivated\` (#3549)`), sostituisci `//     \`AimCell\` e \`Shape\` anche per \`AbilityActivated\` (#3549). Gli altri campi: vuoti/di default per ogni` con:

```cpp
	//     `AimCell` e `Shape` anche per `AbilityActivated` (#3549), e `Origin` anche per `AbilityActivated` (#3578: la
	//     cella della sorgente quando agisce, per la cue d'attivazione). Gli altri campi: vuoti/di default per ogni
```

e nel commento di `Origin` (`:436-442`), dopo la riga `ragione per cui \`FRTBlastPreview\` porta un \`Origin\` proprio.`:

```cpp
	 *
	 * ➕ #3578: su `AbilityActivated` e' la cella della SORGENTE nell'istante in cui agisce (spec «il profilo FX» §2.3,
	 * R5), letta dallo stesso soggetto su cui si congela `SourceVerdict`.
```

- [ ] **Step 4: La cue pura**

`Turn/RTPlaybackLibrary.h`, dopo la dichiarazione di `TracerPolyline` (Task 2):

```cpp

	// --- Le cue del profilo FX (#3578, spec «il profilo FX per abilita'» §2.1, §2.3-§2.4) ----------------------
	// 🔑 Funzioni dell'orologio (`PhaseElapsed`) e del cursore (`Shown`, `BeatsDone`), e di nient'altro (§2.6): un tick
	// unico fino a t e molti tick fino a t danno le stesse cue.

	/** `D_act = A > 0 ? Min(Max(0, ActivationCueSeconds), A) : 0` (R2): la cue finisce prima dell'elemento dopo. */
	static float ActivationCueDuration(float ActivationCueSeconds, float AttackShowSeconds);

	/**
	 * La cue d'attivazione di un `AbilityActivated`: lo stile `Activation` del profilo, sulla cella `Ev.Origin`.
	 * ⛔ Falso se chi guarda non e' in `SourceVerdict` (R7: seconda porta dopo le code), o se lo stile e' `None`.
	 */
	static bool ActivationCueFor(const FRTResolvedEvent& Ev, int32 ViewerTeamId, float Alpha, FRTPlaybackCue& OutCue);

	/** Prep e Dash: le attivazioni gia' rivelate (`Shown`) la cui finestra `[k·A, k·A + D_act)` contiene `PhaseElapsed`. */
	static void ActivationCuesAt(const TArray<FRTResolvedEvent>& Activations, int32 Shown, float PhaseElapsed,
		float AttackShowSeconds, float ActivationCueSeconds, int32 ViewerTeamId, TArray<FRTPlaybackCue>& Out);

	/** Blast: gli `AbilityActivated` della sequenza rivelati (`BeatsDone > 2k`) nella loro finestra `[k·A, k·A + D_act)`. */
	static void BlastActivationCuesAt(const TArray<FRTResolvedEvent>& Timeline,
		const TArray<FRTBlastSequenceElement>& Sequence, int32 BeatsDone, float PhaseElapsed, float AttackShowSeconds,
		float ActivationCueSeconds, int32 ViewerTeamId, TArray<FRTPlaybackCue>& Out);
```

`Turn/RTPlaybackLibrary.cpp`, dopo `TracerPolyline`:

```cpp

float URTPlaybackLibrary::ActivationCueDuration(float ActivationCueSeconds, float AttackShowSeconds)
{
	return AttackShowSeconds > 0.f ? FMath::Min(FMath::Max(0.f, ActivationCueSeconds), AttackShowSeconds) : 0.f;
}

bool URTPlaybackLibrary::ActivationCueFor(const FRTResolvedEvent& Ev, int32 ViewerTeamId, float Alpha, FRTPlaybackCue& OutCue)
{
	if (Ev.Type != ERTResolvedEventType::AbilityActivated
		|| !Ev.SourceVerdict.AllowsTeam(ViewerTeamId)) // R7: fail-closed anche se la coda e' gia' filtrata
	{
		return false;
	}
	switch (URTPresentationBindingLibrary::FxProfileFor(Ev.ActionId, Ev.BaseActionId, Ev.Shape).Activation)
	{
	case ERTActivationFxStyle::Ring:  OutCue.Kind = ERTPlaybackCueKind::Ring;  break;
	case ERTActivationFxStyle::Pulse: OutCue.Kind = ERTPlaybackCueKind::Pulse; break;
	case ERTActivationFxStyle::Flash: OutCue.Kind = ERTPlaybackCueKind::Flash; break;
	default: return false;
	}
	OutCue.At = Ev.Origin;
	OutCue.Toward = Ev.Origin;
	OutCue.Alpha = FMath::Clamp(Alpha, 0.f, 1.f);
	return true;
}

void URTPlaybackLibrary::ActivationCuesAt(const TArray<FRTResolvedEvent>& Activations, int32 Shown, float PhaseElapsed,
	float AttackShowSeconds, float ActivationCueSeconds, int32 ViewerTeamId, TArray<FRTPlaybackCue>& Out)
{
	const float Durata = ActivationCueDuration(ActivationCueSeconds, AttackShowSeconds);
	if (Durata <= 0.f)
	{
		return; // `A <= 0`: tutto in un frame, nessuna cue (spec §4)
	}
	for (int32 K = 0; K < FMath::Min(Shown, Activations.Num()); ++K)
	{
		const float Inizio = AttackLaunchSeconds(K, AttackShowSeconds);
		if (PhaseElapsed < Inizio || PhaseElapsed >= Inizio + Durata)
		{
			continue;
		}
		FRTPlaybackCue Cue;
		if (ActivationCueFor(Activations[K], ViewerTeamId, (PhaseElapsed - Inizio) / Durata, Cue))
		{
			Out.Add(Cue);
		}
	}
}

void URTPlaybackLibrary::BlastActivationCuesAt(const TArray<FRTResolvedEvent>& Timeline,
	const TArray<FRTBlastSequenceElement>& Sequence, int32 BeatsDone, float PhaseElapsed, float AttackShowSeconds,
	float ActivationCueSeconds, int32 ViewerTeamId, TArray<FRTPlaybackCue>& Out)
{
	const float Durata = ActivationCueDuration(ActivationCueSeconds, AttackShowSeconds);
	if (Durata <= 0.f)
	{
		return;
	}
	for (int32 K = 0; K < Sequence.Num() && BeatsDone > 2 * K; ++K) // il battito `2k` rivela l'elemento `k`
	{
		const int32 Indice = Sequence[K].TimelineIndex;
		if (!Timeline.IsValidIndex(Indice) || Timeline[Indice].Type != ERTResolvedEventType::AbilityActivated)
		{
			continue;
		}
		const float Inizio = AttackLaunchSeconds(K, AttackShowSeconds);
		if (PhaseElapsed < Inizio || PhaseElapsed >= Inizio + Durata)
		{
			continue;
		}
		FRTPlaybackCue Cue;
		if (ActivationCueFor(Timeline[Indice], ViewerTeamId, (PhaseElapsed - Inizio) / Durata, Cue))
		{
			Out.Add(Cue);
		}
	}
}
```

- [ ] **Step 5: Il canale della mappa**

`Map/RTHexMapActor.h`, dopo `const TArray<FRTPlaybackTracer>& GetPlaybackTracers() const { return PlaybackTracers; }` (`:930`):

```cpp

	/**
	 * Le cue del profilo FX nel fotogramma corrente (#3578, spec «il profilo FX» §2.4, R10): attivazione, `Marker`,
	 * `AreaPulse`, `ConeSweep`. Le consegna `ARTTurnManager::PushPlaybackCues` e le SOSTITUISCE in blocco, come i
	 * tracer: sono funzione dell'orologio. Un canale proprio, separato dal tracer, che resta com'e'.
	 * ⛔ Nessun filtro qui: la conoscenza l'ha gia' applicata chi consegna.
	 */
	void SetPlaybackCues(const TArray<FRTPlaybackCue>& Cues);

	/** Spegne il canale. */
	void ClearPlaybackCues();

	int32 NumPlaybackCues() const { return PlaybackCues.Num(); }
	/** L'oracolo headless: QUALE cue, su QUALE cella (spec §2.5 — il gate D-278 non vede una cue mai chiamata). */
	const TArray<FRTPlaybackCue>& GetPlaybackCues() const { return PlaybackCues; }
```

Dopo `TArray<FRTPlaybackTracer> PlaybackTracers;` (`:1095`):

```cpp

	/** #3578: le cue del profilo FX consegnate dal playback. */
	TArray<FRTPlaybackCue> PlaybackCues;
```

`Map/RTHexMapActor.cpp`, in `HasAnythingToDraw` dopo `|| PlaybackTracers.Num() > 0` (`:1105`):

```cpp
		|| PlaybackCues.Num() > 0
```

Dopo `ClearPlaybackTracers` (`:1188`):

```cpp

void ARTHexMapActor::SetPlaybackCues(const TArray<FRTPlaybackCue>& Cues)
{
	PlaybackCues = Cues;
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::ClearPlaybackCues()
{
	PlaybackCues.Reset();
	SetActorTickEnabled(HasAnythingToDraw());
}
```

- [ ] **Step 6: La consegna**

`Turn/RTTurnManager.h`, dopo `float TracerFlightSeconds = 0.25f;` (`:1318`):

```cpp

	/**
	 * Durata della cue d'attivazione (#3578, spec «il profilo FX» §2.1, R2). ⚠️ Tagliata ad `AttackShowSeconds` da
	 * `URTPlaybackLibrary::ActivationCueDuration`: l'elemento dopo esce a `(k+1)·A`. Proposta da playtest (D-287).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	float ActivationCueSeconds = 0.35f;

	/**
	 * Durata delle cue di colpo — `Marker` e cue d'impronta (#3578, R2). ⚠️ Tagliata a `A − F_eff` da
	 * `URTPlaybackLibrary::ImpactCueDuration`: l'arrivo cade a `k·A + F_eff`, il lancio dopo a `(k+1)·A`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Playback")
	float ImpactCueSeconds = 0.20f;
```

Dopo `void PushPlaybackTracers();` (`:2447`):

```cpp

	/**
	 * Consegna alla mappa le cue del profilo FX della fase (#3578, spec §2.4): Prep e Dash dalle code di attivazione,
	 * il Blast dalla sequenza e dal cursore dei battiti. Gemella di `PushPlaybackTracers`, con lo stesso flag: non tocca
	 * la mappa se non c'e' nulla da dire e l'ultima consegna era gia' vuota (`bPlaybackCueChannelFull`).
	 */
	void PushPlaybackCues(ERTMatchPhase Phase);
```

Dopo `bool bPlaybackTracerChannelFull = false;` (`:3485`):

```cpp
	/** #3578: come `bPlaybackTracerChannelFull`, per `SetPlaybackCues`. Si azzera dove il canale si spegne. */
	bool bPlaybackCueChannelFull = false;
```

`Turn/RTTurnManager.cpp`, dopo la `}` di `PushPlaybackTracers` (`:8366`):

```cpp

void ARTTurnManager::PushPlaybackCues(ERTMatchPhase Phase)
{
	TArray<FRTPlaybackCue> Cues;
	if (Phase == ERTMatchPhase::Prep || Phase == ERTMatchPhase::Dash)
	{
		const TArray<FRTResolvedEvent>& Coda = (Phase == ERTMatchPhase::Prep) ? PlaybackActivationsPrep : PlaybackActivationsDash;
		URTPlaybackLibrary::ActivationCuesAt(Coda, ActivationsShown, PlaybackPhaseElapsed, AttackShowSeconds,
			ActivationCueSeconds, PlaybackViewerTeamId, Cues);
	}
	else if (Phase == ERTMatchPhase::Blast)
	{
		URTPlaybackLibrary::BlastActivationCuesAt(ResolvedTimeline, PlaybackBlastSequence, BlastBeatsDone,
			PlaybackPhaseElapsed, AttackShowSeconds, ActivationCueSeconds, PlaybackViewerTeamId, Cues);
	}
	const bool bPiene = !Cues.IsEmpty();
	if (!bPiene && !bPlaybackCueChannelFull)
	{
		return;
	}
	ARTHexMapActor* const MapActor = ARTHexMapActor::FindInWorld(GetWorld());
	if (!MapActor) { return; }
	MapActor->SetPlaybackCues(Cues);
	bPlaybackCueChannelFull = bPiene;
}
```

Nel ramo Prep/Dash del tick (`:8853-8855`), subito dopo `{` di `if ((Ph == ERTMatchPhase::Prep || Ph == ERTMatchPhase::Dash) && !bRevealActivationsOnlyAtPhaseEndForTest)`:

```cpp
		// #3578: la cue d'attivazione si consegna a OGNI uscita di questo ramo — il `return` delle fermate compreso —
		// come il tracer nel Blast. ⚠️ E' il primo ramo di Prep/Dash che consegna qualcosa alla mappa.
		ON_SCOPE_EXIT{ PushPlaybackCues(Ph); };
```

Nel ramo Blast, `ON_SCOPE_EXIT{ PushPlaybackTracers(); };` (`:8874`) diventa:

```cpp
		ON_SCOPE_EXIT{ PushPlaybackTracers(); PushPlaybackCues(Ph); }; // #3578: due consegne per tick (R10)
```

Nella finalizzazione di Prep/Dash (`:8932-8940`), dopo la `}` di `if (RevealPlaybackActivations(Attivazioni, Attivazioni.Num())) { return; }`, ancora dentro `if (Ph == ERTMatchPhase::Prep || Ph == ERTMatchPhase::Dash)`:

```cpp
			// #3578: nessuna cue sopravvive alla fase (spec §2.4). ⚠️ Coi tetti di R2 qui il canale e' gia' vuoto: e' la
			// rete del caso in cui una fase venga accorciata.
			if (ARTHexMapActor* const CueMap = ARTHexMapActor::FindInWorld(GetWorld()))
			{
				CueMap->ClearPlaybackCues();
			}
			bPlaybackCueChannelFull = false;
```

Nella finalizzazione del Blast, dopo `TracerMap->ClearPlaybackTracers();` (`:8965`):

```cpp
				TracerMap->ClearPlaybackCues(); // #3578: nessuna cue sopravvive al Blast
```

e dopo `bPlaybackTracerChannelFull = false;` (`:8967`):

```cpp
			bPlaybackCueChannelFull = false;
```

In `FinishPlayback`, dopo `FootprintMap->ClearPlaybackTracers();` (`:9193`):

```cpp
		FootprintMap->ClearPlaybackCues(); // #3578: e passa di qui anche `SkipPlayback` — nessuna cue rigiocata (§2.6)
```

e dopo `bPlaybackTracerChannelFull = false;` (`:9199`):

```cpp
	bPlaybackCueChannelFull = false; // #3578: il flag segue il canale che si spegne qui
```

- [ ] **Step 7: Build e verde**

Run: build; test `RefactorTactics.Privacy.ActivationCueNeedsTheSource+RefactorTactics.Playback.ActivationCueAtTheSourceCell+RefactorTactics.Privacy.HiddenSourceDeliversNoActivationCue+RefactorTactics.Determinism.FxFieldsStayOutOfHashes+RefactorTactics.HexMapActor.PlaybackCueIsItsOwnChannel`, log `<scratchpad>/t3-verde.log`. Expected: verdi. Poi `RefactorTactics.Playback+RefactorTactics.Determinism+RefactorTactics.HexMapActor+RefactorTactics.Unit.BlueprintSurfaceIsCensused`, log `<scratchpad>/t3-gate.log`: verdi (`Unit.BlueprintSurfaceIsCensused` invariato: nessuna proprietà nuova su `ARTUnit`). Se `ActivationCueAtTheSourceCell` fallisce su una **premessa** (la carica non parte, `Ev.Origin == Caricatore->Cell`), non si allenta: si estende la fixture (cella di partenza più lontana, `PlannedDashCell` letto da `Turn.ChargeActivatesInDashNotInBlast`, `Tests/RTAbilityActivatedTests.cpp`) finché la premessa vale.

- [ ] **Step 8: Commit, poi mutazioni (7), (8), (15), (P5)**

```bash
git add Source/RefactorTactics/Turn/RTTurnManager.h Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Turn/RTResolvedEvent.h Source/RefactorTactics/Turn/RTPlaybackLibrary.h Source/RefactorTactics/Turn/RTPlaybackLibrary.cpp Source/RefactorTactics/Map/RTHexMapActor.h Source/RefactorTactics/Map/RTHexMapActor.cpp Source/RefactorTactics/Tests/RTAbilityFxProfileTests.cpp Source/RefactorTactics/Tests/RTPlaybackActivationTests.cpp Source/RefactorTactics/Tests/RTAttackTracerTests.cpp Source/RefactorTactics/Tests/RTHexMapActorTests.cpp
git commit -F - <<'EOF'
feat(3578): il segnale di attivazione — Ev.Origin dal produttore, la cue del profilo dietro SourceVerdict, il canale di mappa

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
```

Ogni mutazione: modifica, build, test indicato, `Result={Fail}` sull'asserto indicato, `git checkout -- <file>`, build, verde.

7. In `PushPlaybackCues`, dentro il ramo `if (Phase == ERTMatchPhase::Prep || Phase == ERTMatchPhase::Dash)`, dopo la chiamata a `ActivationCuesAt`, inserisci la mutante:
   ```cpp
   for (FRTPlaybackCue& C : Cues) { for (const FRTResolvedEvent& Att : Coda) { if (ARTUnit* S = UnitByStableId(Att.SourceStableUnitId)) { C.At = S->Cell; } } } // MUTANTE (7)
   ```
   Test `RefactorTactics.Playback.ActivationCueAtTheSourceCell`. Cade «🔴 Ring di Ram su Ev.Origin, non sulla cella dell'attore» (la premessa F15 è asserita prima, quindi la mutante non è vacua); il `Pulse` dello scudo resta verde (non si muove), ed è atteso.
8. In `ActivationCueFor`, cancella `|| !Ev.SourceVerdict.AllowsTeam(ViewerTeamId)`. Test `RefactorTactics.Privacy.ActivationCueNeedsTheSource`: cadono «🔴 chi non vede la sorgente non riceve la cue» e «un osservatore fuori intervallo non legge». (`Privacy.HiddenSourceDeliversNoActivationCue` resta verde: la coda è filtrata a monte, `RTTurnManager.cpp:7923-7926` e `BuildBlastSequence` — è la difesa in profondità di R7, e la PR lo dice.)
15. In `ARTHexMapActor::SetPlaybackCues`, `PlaybackCues = Cues;` diventa `PlaybackCues.Append(Cues);`. Test `RefactorTactics.HexMapActor.PlaybackCueIsItsOwnChannel`: cade «🔴 la seconda consegna sostituisce la prima».
P5. In `HasAnythingToDraw`, cancella `|| PlaybackCues.Num() > 0`. Stesso test: cade «🔴 consegnata una cue, il Tick si accende».

---

### Task 4: Le cue di colpo — `Marker` all'arrivo, `AreaPulse`/`ConeSweep` una volta per impronta, R14

**Files:**
- Modify: `Turn/RTPlaybackLibrary.h` / `.cpp` (dopo `BlastActivationCuesAt`, Task 3; include di `RefactorTactics.h` per `LogRT`)
- Modify: `Turn/RTTurnManager.h:3486-3490` (membro `PlaybackBlastFootprintFx` accanto a `PlaybackBlastFlights`)
- Modify: `Turn/RTTurnManager.cpp:7969-7975` (calcolo accanto ai voli in `BeginPlayback`), `PushPlaybackCues` (Task 3), `:9187` (reset in `FinishPlayback`), `:6374-6377` (commento `⌫`)
- Modify: `Turn/RTResolvedEvent.h:196-197` e `:205-207` (commenti `⌫`: il `FromVerdict` del centro ora serve)
- Test: `Tests/RTAbilityFxProfileTests.cpp`, `Tests/RTAttackTracerPlaybackTests.cpp` (include `:1-14`, test prima di `#endif`, `:365`)

**Interfaces:**
- Produces (statiche C++ in `URTPlaybackLibrary`):
  ```cpp
  static float ImpactCueDuration(float ImpactCueSeconds, float AttackShowSeconds, float Flight);   // D_imp(k)
  static bool ImpactCueFor(const FRTResolvedEvent& Atk, int32 ViewerTeamId, float Alpha, FRTPlaybackCue& OutCue);
  static bool FootprintCueFor(const FRTResolvedEvent& Footprint, const FRTResolvedEvent& Atk, int32 ViewerTeamId, float Alpha, FRTPlaybackCue& OutCue);
  static TArray<int32> FootprintFxForSequence(const TArray<FRTResolvedEvent>& Timeline, const TArray<FRTBlastSequenceElement>& Sequence);
  static void BlastHitCuesAt(const TArray<FRTResolvedEvent>& Timeline, const TArray<FRTBlastSequenceElement>& Sequence,
      const TArray<float>& Flights, const TArray<int32>& FootprintFx, int32 BeatsDone, float PhaseElapsed,
      float AttackShowSeconds, float ImpactCueSeconds, int32 ViewerTeamId, TArray<FRTPlaybackCue>& Out);
  ```
- Produces (in `ARTTurnManager`): `TArray<int32> PlaybackBlastFootprintFx;` (parallelo a `PlaybackBlastSequence`, indice di timeline dell'impronta o `INDEX_NONE`)
- Consumes: `ActivationCuesAt`/`BlastActivationCuesAt`/`PushPlaybackCues`/`SetPlaybackCues` (Task 3), `AttackBeatSeconds`/`AttackBeatsDue`/`TracerFlightFor`/`IsTracerEligible` (esistenti e Task 2), `FxProfileFor` (Task 1).

- [ ] **Step 0: Misure (F22 e la cura ad area) — sola lettura, esito nel report e nella PR**

1. **`Hero.Aevik.ReactiveCapacitor` emette `Attack`? — CONFERMA: l'esito è già misurato (review del piano).** Il contrattacco **non** emette un evento `Attack`: l'unico `Ev.Type = ERTResolvedEventType::Attack` del Blast è a `Turn/RTTurnManager.cpp:6357`, nel ciclo sui colpi del **piano**; i contrattacchi entrano negli array dei colpi **dopo** (`:6460-6468`: `Attacks.Add(Counter.Attack)`, `AttackSrc.Add(Counter.SourceCell)`, …; commento a `:6470`). `Action.Counter` ha portata 0 e trigger `HitByDirectAttack`. La conferma sono due righe: `grep -n "ERTResolvedEventType::Attack;" Source/RefactorTactics/Turn/RTTurnManager.cpp` (atteso: la sola `:6357`) e `grep -n "Reactions.CounterAttacks" Source/RefactorTactics/Turn/RTTurnManager.cpp` (atteso: il `for` a `:6460`, dopo la `}` del ciclo dei colpi). Esito in `<scratchpad>/t4-misure.md` con le righe. **Azione: nessuna riga d'override, nessun `Tracer = None`** — `ReactiveCapacitor` non ha un colpo da disegnare; la PR lo dichiara come chiusura di F22. Se la conferma fallisce (il contrattacco è diventato un `Attack`), ci si ferma e si riporta: la riga è una decisione dell'autore.
2. **`Hero.Muiren.CircularTide` emette `Attack` e `AttackFootprint`?** (`Ability/RTHeroCatalogLibrary.cpp:518-522`, fase `Attack`, priorità 60; variante `Healing` a `:664`). Leggi gli effetti e, se serve, il ciclo dell'impronta (`Turn/RTTurnManager.cpp:5598-5611`). Esito in `<scratchpad>/t4-misure.md`: se la cura non emette `Attack`, il suo `AreaPulse` non ha consumatore (spec §7, follow-up), e la PR lo dice; nessun codice cambia qui.

- [ ] **Step 1: I test rossi**

`Tests/RTAbilityFxProfileTests.cpp`, prima di `// --- I test dei Task 3, 4 e 5 si aggiungono QUI`:

```cpp

// --- Le cue di colpo (#3578, spec §2.4) ------------------------------------------------------------------------

namespace
{
	FRTResolvedEvent FxImpronta(int32 Sorgente, const TCHAR* Azione, ERTAbilityShape Forma, const FRTCellId& Origine,
		const FRTCellId& Mira)
	{
		FRTResolvedEvent Ev;
		Ev.Phase = ERTMatchPhase::Blast;
		Ev.Type = ERTResolvedEventType::AttackFootprint;
		Ev.SourceStableUnitId = Sorgente;
		Ev.ActionId = Azione;
		Ev.Shape = Forma;
		Ev.Origin = Origine;
		Ev.AimCell = Mira;
		return Ev;
	}

	/** Un colpo risolto, visibile alle squadre 0 e 1 su entrambi gli estremi. */
	FRTResolvedEvent FxColpo(int32 Sorgente, const TCHAR* Azione, ERTAbilityShape Forma, const FRTCellId& Da,
		const FRTCellId& Vittima)
	{
		FRTResolvedEvent Ev;
		Ev.Phase = ERTMatchPhase::Blast;
		Ev.Type = ERTResolvedEventType::Attack;
		Ev.SourceStableUnitId = Sorgente;
		Ev.ActionId = Azione;
		Ev.Shape = Forma;
		Ev.HitGeometry.bResolved = true;
		Ev.HitGeometry.From = Da;
		Ev.HitGeometry.Impact = Vittima;
		Ev.HitGeometry.FromVerdict.AllowTeam(0);
		Ev.HitGeometry.FromVerdict.AllowTeam(1);
		Ev.HitGeometry.ImpactVerdict.AllowTeam(0);
		Ev.HitGeometry.ImpactVerdict.AllowTeam(1);
		return Ev;
	}

	/** La sequenza «come viene»: un elemento per evento, nell'ordine dato (il test costruisce gia' l'ordine di §2.4). */
	TArray<FRTBlastSequenceElement> FxSequenza(const TArray<FRTResolvedEvent>& T)
	{
		TArray<FRTBlastSequenceElement> S;
		for (int32 I = 0; I < T.Num(); ++I)
		{
			FRTBlastSequenceElement E;
			E.TimelineIndex = I;
			E.SourceStableUnitId = T[I].SourceStableUnitId;
			E.ActionId = T[I].ActionId;
			S.Add(E);
		}
		return S;
	}

	/** I voli come li calcola `BeginPlayback` (`RTTurnManager.cpp:7973-7974`). */
	TArray<float> FxVoli(const TArray<FRTResolvedEvent>& T, float A)
	{
		TArray<float> V;
		for (const FRTResolvedEvent& Ev : T)
		{
			V.Add(URTPlaybackLibrary::TracerFlightFor(URTPlaybackLibrary::IsTracerEligible(Ev), 0.25f, A));
		}
		return V;
	}

	/** Le cue del Blast all'istante `t`, per chi guarda: la stessa composizione di `PushPlaybackCues`. */
	TArray<FRTPlaybackCue> FxCueAl(const TArray<FRTResolvedEvent>& T, float t, float A, int32 Viewer = 0)
	{
		const TArray<FRTBlastSequenceElement> S = FxSequenza(T);
		const TArray<float> V = FxVoli(T, A);
		const TArray<int32> Impronte = URTPlaybackLibrary::FootprintFxForSequence(T, S);
		const int32 Battiti = URTPlaybackLibrary::AttackBeatsDue(t, A, V);
		TArray<FRTPlaybackCue> Out;
		URTPlaybackLibrary::BlastActivationCuesAt(T, S, Battiti, t, A, 0.35f, Viewer, Out);
		URTPlaybackLibrary::BlastHitCuesAt(T, S, V, Impronte, Battiti, t, A, 0.20f, Viewer, Out);
		return Out;
	}

	bool FxHa(const TArray<FRTPlaybackCue>& C, ERTPlaybackCueKind Tipo, const FRTCellId& Cella)
	{
		return C.ContainsByPredicate([&](const FRTPlaybackCue& X) { return X.Kind == Tipo && X.At == Cella; });
	}

	/** Quante volte una cue del tipo compare, contando le CORSE di presenza su tutto il Blast (passo 5 ms). */
	int32 FxCorse(const TArray<FRTResolvedEvent>& T, float A, ERTPlaybackCueKind Tipo)
	{
		int32 Corse = 0;
		bool bPrima = false;
		for (float t = 0.f; t <= T.Num() * A + 0.01f; t += 0.005f)
		{
			const bool bOra = FxCueAl(T, t, A).ContainsByPredicate([&](const FRTPlaybackCue& X) { return X.Kind == Tipo; });
			Corse += (bOra && !bPrima) ? 1 : 0;
			bPrima = bOra;
		}
		return Corse;
	}

	/** L'atto `Area` di prova: impronta di Branth centrata su (3,0), due vittime NON al centro. */
	TArray<FRTResolvedEvent> FxAttoArea(bool bConImpronta = true)
	{
		TArray<FRTResolvedEvent> T;
		if (bConImpronta)
		{
			T.Add(FxImpronta(1, TEXT("Hero.Branth.MortarShot"), ERTAbilityShape::Area, FRTCellId(0, 0), FRTCellId(3, 0)));
		}
		T.Add(FxColpo(1, TEXT("Hero.Branth.MortarShot"), ERTAbilityShape::Area, FRTCellId(3, 0), FRTCellId(4, 0)));
		T.Add(FxColpo(1, TEXT("Hero.Branth.MortarShot"), ERTAbilityShape::Area, FRTCellId(3, 0), FRTCellId(2, 1)));
		return T;
	}
}

/**
 * Il `Marker` solo per chi conosceva la vittima nella sua cella (`ImpactVerdict`, spec §2.3), e solo se il profilo lo
 * dice. ✅ Validato per mutazione (10): `ImpactVerdict` ignorato fa cadere «chi non conosceva la vittima».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrivacyImpactMarkerNeedsTheVictimTest,
	"RefactorTactics.Privacy.ImpactMarkerNeedsTheVictim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrivacyImpactMarkerNeedsTheVictimTest::RunTest(const FString&)
{
	FRTResolvedEvent Colpo = FxColpo(1, TEXT("Hero.Branth.ImpactShot"), ERTAbilityShape::Single, FRTCellId(0, 0), FRTCellId(2, 0));
	Colpo.HitGeometry.ImpactVerdict = FRTKnowledgeVerdict::NoOne();
	Colpo.HitGeometry.ImpactVerdict.AllowTeam(0);
	FRTPlaybackCue C;
	TestFalse(TEXT("🔴 chi non conosceva la vittima non vede il Marker"), URTPlaybackLibrary::ImpactCueFor(Colpo, 1, 0.5f, C));
	if (TestTrue(TEXT("chi la conosceva lo vede"), URTPlaybackLibrary::ImpactCueFor(Colpo, 0, 0.5f, C)))
	{
		TestTrue(TEXT("e' un Marker"), C.Kind == ERTPlaybackCueKind::Marker);
		TestTrue(TEXT("sulla cella d'impatto"), C.At == FRTCellId(2, 0));
	}
	const FRTResolvedEvent Finta = FxColpo(1, TEXT("Hero.Ivrin.Feint"), ERTAbilityShape::Single, FRTCellId(0, 0), FRTCellId(2, 0));
	TestFalse(TEXT("un profilo con Impact = None non ha Marker"), URTPlaybackLibrary::ImpactCueFor(Finta, 0, 0.5f, C));
	return true;
}

/**
 * L'`AreaPulse` sta sull'`AimCell` dell'IMPRONTA, non su un `Impact` (spec §2.4, F5).
 * ➕ rev2. **Premessa asserita prima**: l'`AimCell` e' diversa da OGNI `Impact` dell'atto, o la mutante (11) sarebbe
 * indistinguibile. ✅ Validato per mutazione (11).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxAreaPulseIsOnTheFootprintAimTest,
	"RefactorTactics.Fx.AreaPulseIsOnTheFootprintAim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxAreaPulseIsOnTheFootprintAimTest::RunTest(const FString&)
{
	const TArray<FRTResolvedEvent> T = FxAttoArea();
	const FRTCellId Mira = T[0].AimCell;
	for (int32 I = 1; I < T.Num(); ++I)
	{
		if (!TestFalse(TEXT("⛔ premessa: l'AimCell non e' l'Impact di nessun colpo"), T[I].HitGeometry.Impact == Mira)) { return false; }
	}
	const float A = 0.5f;
	const float Arrivo = URTPlaybackLibrary::AttackBeatSeconds(3, A, FxVoli(T, A)); // il battito 2·1+1: arrivo del primo colpo
	const TArray<FRTPlaybackCue> C = FxCueAl(T, Arrivo + 0.01f, A);
	TestTrue(TEXT("🔴 l'AreaPulse e' sull'AimCell dell'impronta"), FxHa(C, ERTPlaybackCueKind::AreaPulse, Mira));
	TestFalse(TEXT("e non sull'Impact del colpo che lo porta"), FxHa(C, ERTPlaybackCueKind::AreaPulse, T[1].HitGeometry.Impact));
	return true;
}

/**
 * OGNI `Attack` ha il suo `Marker`, il primo dell'atto compreso (spec §2.4, F6: la R8 e' caduta), su un'`Area` con
 * due vittime e su una `Line` con due vittime.
 * ✅ Validato per mutazione (12).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackEveryAttackGetsItsProfileMarkerTest,
	"RefactorTactics.Playback.EveryAttackGetsItsProfileMarker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackEveryAttackGetsItsProfileMarkerTest::RunTest(const FString&)
{
	TArray<FRTResolvedEvent> T = FxAttoArea();
	T.Add(FxImpronta(2, TEXT("Hero.Aevik.LinearDischarge"), ERTAbilityShape::Line, FRTCellId(0, 2), FRTCellId(3, 2)));
	T.Add(FxColpo(2, TEXT("Hero.Aevik.LinearDischarge"), ERTAbilityShape::Line, FRTCellId(0, 2), FRTCellId(1, 2)));
	T.Add(FxColpo(2, TEXT("Hero.Aevik.LinearDischarge"), ERTAbilityShape::Line, FRTCellId(0, 2), FRTCellId(2, 2)));
	const float A = 0.5f;
	const TArray<float> V = FxVoli(T, A);
	TestTrue(TEXT("premessa: la scarica vola (Line con un id), il mortaio no"), V[4] > 0.f && V[1] == 0.f);
	for (int32 K = 0; K < T.Num(); ++K)
	{
		if (T[K].Type != ERTResolvedEventType::Attack) { continue; }
		const float Arrivo = URTPlaybackLibrary::AttackBeatSeconds(2 * K + 1, A, V);
		TestTrue(FString::Printf(TEXT("🔴 elemento %d: Marker sulla sua vittima all'arrivo"), K),
			FxHa(FxCueAl(T, Arrivo + 0.01f, A), ERTPlaybackCueKind::Marker, T[K].HitGeometry.Impact));
	}
	return true;
}

/**
 * UNA cue d'impronta per impronta, portata dal primo colpo, contata su TUTTI gli istanti del Blast (➕ rev2.); con
 * l'impronta tolta nessun pulse, nessun errore, e i `Marker` restano.
 * ✅ Validato per mutazione (21): l'impronta non consumata fa portare il pulse a ogni colpo — due corse.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackFootprintCueOncePerFootprintTest,
	"RefactorTactics.Playback.FootprintCueOncePerFootprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackFootprintCueOncePerFootprintTest::RunTest(const FString&)
{
	TestEqual(TEXT("🔴 un solo AreaPulse in tutto il Blast"), FxCorse(FxAttoArea(), 0.5f, ERTPlaybackCueKind::AreaPulse), 1);
	TestEqual(TEXT("e due Marker, uno per colpo"), FxCorse(FxAttoArea(), 0.5f, ERTPlaybackCueKind::Marker), 2);
	TestEqual(TEXT("impronta tolta: nessun pulse"), FxCorse(FxAttoArea(false), 0.5f, ERTPlaybackCueKind::AreaPulse), 0);
	TestEqual(TEXT("impronta tolta: i Marker restano"), FxCorse(FxAttoArea(false), 0.5f, ERTPlaybackCueKind::Marker), 2);
	return true;
}

/**
 * R14 (➕ rev2.): due `AttackFootprint` con la stessa chiave prima del colpo → la cue usa le celle della SECONDA.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackSecondFootprintReplacesTheFirstTest,
	"RefactorTactics.Playback.SecondFootprintReplacesTheFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackSecondFootprintReplacesTheFirstTest::RunTest(const FString&)
{
	TArray<FRTResolvedEvent> T;
	T.Add(FxImpronta(1, TEXT("Hero.Branth.MortarShot"), ERTAbilityShape::Area, FRTCellId(0, 0), FRTCellId(3, 0)));
	T.Add(FxImpronta(1, TEXT("Hero.Branth.MortarShot"), ERTAbilityShape::Area, FRTCellId(0, 0), FRTCellId(5, 0)));
	T.Add(FxColpo(1, TEXT("Hero.Branth.MortarShot"), ERTAbilityShape::Area, FRTCellId(5, 0), FRTCellId(6, 0)));
	const TArray<int32> Impronte = URTPlaybackLibrary::FootprintFxForSequence(T, FxSequenza(T));
	TestEqual(TEXT("🔴 il colpo consuma la SECONDA impronta"), Impronte.IsValidIndex(2) ? Impronte[2] : INDEX_NONE, 1);
	const float Arrivo = URTPlaybackLibrary::AttackBeatSeconds(5, 0.5f, FxVoli(T, 0.5f));
	const TArray<FRTPlaybackCue> C = FxCueAl(T, Arrivo + 0.01f, 0.5f);
	TestTrue(TEXT("il pulse e' sul centro della seconda"), FxHa(C, ERTPlaybackCueKind::AreaPulse, FRTCellId(5, 0)));
	TestFalse(TEXT("e non su quello della prima"), FxHa(C, ERTPlaybackCueKind::AreaPulse, FRTCellId(3, 0)));
	return true;
}

/**
 * Review Focus (d): il centro di un'`Area` non visto da chi guarda non si rivela col pulse. `FromVerdict` di un colpo
 * `Area` e' congelato sul CENTRO (`RTTurnManager.cpp:6390`, spec §2.4); il `Marker` segue invece la vittima.
 * ✅ Validato per mutazione (P3): `FromVerdict` ignorato in `FootprintCueFor`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrivacyAreaPulseNeedsTheCenterTest,
	"RefactorTactics.Privacy.AreaPulseNeedsTheCenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrivacyAreaPulseNeedsTheCenterTest::RunTest(const FString&)
{
	TArray<FRTResolvedEvent> T = FxAttoArea();
	T[1].HitGeometry.FromVerdict = FRTKnowledgeVerdict::NoOne();
	T[1].HitGeometry.FromVerdict.AllowTeam(0);
	FRTPlaybackCue C;
	TestFalse(TEXT("🔴 chi non vede il centro non riceve l'AreaPulse"), URTPlaybackLibrary::FootprintCueFor(T[0], T[1], 1, 0.5f, C));
	TestTrue(TEXT("ma vede il Marker della vittima che conosce"), URTPlaybackLibrary::ImpactCueFor(T[1], 1, 0.5f, C));
	TestTrue(TEXT("controllo: chi vede il centro riceve il pulse"), URTPlaybackLibrary::FootprintCueFor(T[0], T[1], 0, 0.5f, C)
		&& C.Kind == ERTPlaybackCueKind::AreaPulse && C.At == FRTCellId(3, 0));
	return true;
}

/**
 * Le finestre delle cue di elementi diversi non si sovrappongono e finiscono entro `N·A` (spec §2.1, «conseguenza dei
 * tetti»), sulla griglia dichiarata: `A ∈ {0.1, 0.5, 1.0}`, `F ∈ {0, A/4, A/2, A}`, durate `∈ {0, A/2, 2A}`.
 * Sequenza mista: attivazione, colpo con volo, colpo senza volo, attivazione, colpo con volo.
 * ✅ Validato per mutazione (13): i `Min` tolti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackFxCuesNeverOverlapInTheBlastTest,
	"RefactorTactics.Playback.FxCuesNeverOverlapInTheBlast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackFxCuesNeverOverlapInTheBlastTest::RunTest(const FString&)
{
	for (const float A : { 0.1f, 0.5f, 1.0f })
	{
		for (const float Fq : { 0.f, 0.25f * A, 0.5f * A, A })
		{
			for (const float Act : { 0.f, 0.5f * A, 2.f * A })
			{
				for (const float Imp : { 0.f, 0.5f * A, 2.f * A })
				{
					const float F = URTPlaybackLibrary::TracerFlightFor(true, Fq, A);
					// (tipo, volo): true = attivazione, false = colpo
					const TArray<TPair<bool, float>> Elementi = { { true, 0.f }, { false, F }, { false, 0.f }, { true, 0.f }, { false, F } };
					TArray<FVector2D> Finestre; // [inizio, fine)
					for (int32 K = 0; K < Elementi.Num(); ++K)
					{
						const float Lancio = URTPlaybackLibrary::AttackLaunchSeconds(K, A);
						const float Inizio = Elementi[K].Key ? Lancio : Lancio + Elementi[K].Value;
						const float Durata = Elementi[K].Key ? URTPlaybackLibrary::ActivationCueDuration(Act, A)
							: URTPlaybackLibrary::ImpactCueDuration(Imp, A, Elementi[K].Value);
						Finestre.Add(FVector2D(Inizio, Inizio + Durata));
					}
					for (int32 I = 0; I < Finestre.Num(); ++I)
					{
						TestTrue(FString::Printf(TEXT("A=%.2f F=%.2f act=%.2f imp=%.2f: elemento %d entro N·A"), A, F, Act, Imp, I),
							Finestre[I].Y <= Elementi.Num() * A + 1e-4f);
						for (int32 J = I + 1; J < Finestre.Num(); ++J)
						{
							TestTrue(FString::Printf(TEXT("🔴 A=%.2f F=%.2f act=%.2f imp=%.2f: %d finisce prima che %d cominci"),
								A, F, Act, Imp, I, J), Finestre[I].Y <= Finestre[J].X + 1e-4f);
						}
					}
				}
			}
		}
	}
	return true;
}
```

`Tests/RTAttackTracerPlaybackTests.cpp`, dopo `#include "RTAttackPlaybackProbeForTest.h"` (`:14`):

```cpp
#include "EngineUtils.h" // #3578: `TActorIterator`, per ritrovare l'attaccante della fixture
#include "Turn/RTResolvedEvent.h"
```

Nel namespace anonimo, prima della `}` che lo chiude (dopo `TickOfBeat`, `:102`):

```cpp

	// --- Le cue di colpo (#3578, spec «il profilo FX» §2.4) ---------------------------------------------------------

	bool BattitoHaCue(const ARTHexMapActor* Mappa, ERTPlaybackCueKind Tipo, const FRTCellId& Cella)
	{
		return Mappa && Mappa->GetPlaybackCues().ContainsByPredicate([&](const FRTPlaybackCue& C)
		{
			return C.Kind == Tipo && C.At == Cella;
		});
	}

	bool BattitoHaCueDiColpo(const ARTHexMapActor* Mappa)
	{
		return Mappa && Mappa->GetPlaybackCues().ContainsByPredicate([](const FRTPlaybackCue& C)
		{
			return C.Kind == ERTPlaybackCueKind::Marker || C.Kind == ERTPlaybackCueKind::AreaPulse
				|| C.Kind == ERTPlaybackCueKind::ConeSweep;
		});
	}

	/** L'indice nella sequenza del Blast del primo `Attack` della timeline, letto dopo `LockInAndResolve`. */
	int32 BattitoIndiceDelColpo(const ARTTurnManager* TM)
	{
		const TArray<int32> Sequenza = TM->PlaybackBlastSequenceIndicesForTest();
		for (int32 K = 0; K < Sequenza.Num(); ++K)
		{
			if (TM->ResolvedTimelineForTest()[Sequenza[K]].Type == ERTResolvedEventType::Attack) { return K; }
		}
		return INDEX_NONE;
	}
```

Prima di `#endif` (`:365`):

```cpp

/**
 * Il `Marker` arriva con l'ARRIVO, mai col lancio (spec §2.4, V1 del tracer): durante il volo il canale non ha il
 * `Marker`, al tick dell'arrivo si', sulla cella d'impatto. Fixture di `TracerIsInFlightBetweenLaunchAndArrival`.
 * ➕ rev2. **Premessa asserita prima**: il colpo ha `F_eff > 0`, quindi lancio e arrivo cadono in tick DIVERSI; con
 * volo nullo cadono nello stesso tick (`RTTurnManager.cpp:8207-8262`) e la mutante (9) sopravviverebbe.
 * ✅ Validato per mutazioni (9) e (12) (atto con una vittima: e' il primo colpo dell'atto).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackImpactCueComesAtTheArrivalTest,
	"RefactorTactics.Playback.ImpactCueComesAtTheArrival",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackImpactCueComesAtTheArrivalTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ false);
	ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(World);
	if (!TestTrue(TEXT("turno e mappa di prova"), TM != nullptr && Mappa != nullptr)) { return false; }
	if (!TestTrue(TEXT("⛔ premessa: F_eff > 0"),
			URTPlaybackLibrary::TracerFlightFor(true, TM->TracerFlightSeconds, TM->AttackShowSeconds) > 0.f))
	{
		return false;
	}
	TM->LockInAndResolve();
	bool bMarkerInVolo = false, bMarkerAllArrivo = false;
	int32 TickLancio = -1, TickArrivo = -1;
	for (int32 I = 0; I < 600 && TM->IsResolving(); ++I)
	{
		TM->Tick(0.02f);
		const bool bLanciato = TM->AttackBeatTraceForTest().Contains(TEXT("L0"));
		const bool bArrivato = TM->AttackBeatTraceForTest().Contains(TEXT("A0"));
		if (bLanciato && TickLancio < 0) { TickLancio = I; }
		if (bLanciato && !bArrivato) { bMarkerInVolo |= BattitoHaCue(Mappa, ERTPlaybackCueKind::Marker, FRTCellId(3, 2)); }
		if (bArrivato && TickArrivo < 0)
		{
			TickArrivo = I;
			bMarkerAllArrivo = BattitoHaCue(Mappa, ERTPlaybackCueKind::Marker, FRTCellId(3, 2));
		}
	}
	if (!TestTrue(TEXT("⛔ premessa: lancio e arrivo in tick diversi"), TickLancio >= 0 && TickArrivo > TickLancio)) { return false; }
	TestFalse(TEXT("🔴 al battito 2k (in volo) nessun Marker"), bMarkerInVolo);
	TestTrue(TEXT("🔴 al battito 2k+1 il Marker sulla cella d'impatto"), bMarkerAllArrivo);
	return true;
}

/**
 * Review Focus (b): un colpo con `HitGeometry.bResolved == false` non ha tracer, `Marker` ne' cue d'impronta, e la
 * clip `Attack` suona comunque. Prima la funzione pura (con verdetti APERTI: a mondo il verdetto di default e'
 * gia' chiuso e nasconderebbe il difetto), poi il turno col gancio `bSkipHitGeometryForTest`.
 * ✅ Validato per mutazione (P4): `bResolved` ignorato in `ImpactCueFor`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackUnresolvedGeometryHasNoFxTest,
	"RefactorTactics.Playback.UnresolvedGeometryHasNoFxButPlaysTheClip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackUnresolvedGeometryHasNoFxTest::RunTest(const FString&)
{
	FRTResolvedEvent Irrisolto;
	Irrisolto.Type = ERTResolvedEventType::Attack;
	Irrisolto.ActionId = TEXT("Hero.Branth.ImpactShot");
	Irrisolto.Shape = ERTAbilityShape::Single;
	Irrisolto.HitGeometry.bResolved = false;
	Irrisolto.HitGeometry.FromVerdict.AllowTeam(0);
	Irrisolto.HitGeometry.ImpactVerdict.AllowTeam(0);
	FRTPlaybackCue C;
	TestFalse(TEXT("🔴 geometria irrisolta: nessun Marker, anche coi verdetti aperti"), URTPlaybackLibrary::ImpactCueFor(Irrisolto, 0, 0.5f, C));
	TestFalse(TEXT("e nessun tracer"), URTPlaybackLibrary::IsTracerEligible(Irrisolto));

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ false);
	ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(World);
	if (!TestTrue(TEXT("turno e mappa di prova"), TM != nullptr && Mappa != nullptr)) { return false; }
	TM->bSkipHitGeometryForTest = true;
	TM->LockInAndResolve();
	bool bFx = false;
	for (int32 I = 0; I < 600 && TM->IsResolving(); ++I)
	{
		TM->Tick(0.02f);
		bFx |= Mappa->NumPlaybackTracers() > 0 || BattitoHaCueDiColpo(Mappa);
	}
	ARTUnit* Attaccante = nullptr;
	for (TActorIterator<ARTUnit> It(World); It; ++It) { if (It->TeamId == 0) { Attaccante = *It; } }
	if (!TestTrue(TEXT("⛔ premessa: il colpo e' partito ed e' arrivato"),
			TM->AttackBeatTraceForTest().Contains(TEXT("L0")) && TM->AttackBeatTraceForTest().Contains(TEXT("A0")))
		|| !TestNotNull(TEXT("⛔ premessa: l'attaccante"), Attaccante))
	{
		return false;
	}
	TestFalse(TEXT("🔴 nessun tracer, Marker o cue d'impronta su nessun tick"), bFx);
	TestFalse(TEXT("e la clip Attack e' stata risolta comunque"),
		Attaccante->LastResolvedClipPathForTest(ERTPresentationRole::Attack).IsNull());
	return true;
}

/**
 * Review Focus (c), spec §2.6: le cue sono funzione dell'orologio e del cursore. Due mondi identici, entrati nel
 * Blast con gli stessi tick; poi uno avanza con UN tick fino a t, l'altro con tick da 1/60 s fino allo stesso t,
 * dentro la finestra del `Marker`. Stesse cue, `Alpha` uguale entro 1e-3.
 * 🔑 Premesse misurate in review: `TickPlayback` non taglia `DeltaSeconds` (`Dt = DeltaSeconds × velocita'`,
 * `RTTurnManager.cpp:8606`; `PlaybackPhaseElapsed += Dt`, `:8629`); nel Blast un tick lungo esegue tutti i battiti
 * dovuti (`:8890`) e finalizza al piu' una fase per tick. Il salto resta dentro il Blast, quindi un tick basta.
 * ✅ Validato per mutazione (14): `Alpha` da un accumulatore azzerato al tick della rivelazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackFxCuesAreAFunctionOfTheClockTest,
	"RefactorTactics.Playback.FxCuesAreAFunctionOfTheClock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackFxCuesAreAFunctionOfTheClockTest::RunTest(const FString&)
{
	TArray<FRTPlaybackCue> Esito[2];
	float Orologio[2] = { 0.f, 0.f };
	for (int32 Run = 0; Run < 2; ++Run)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ false);
		ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(World);
		if (!TestTrue(TEXT("turno e mappa di prova"), TM != nullptr && Mappa != nullptr)) { return false; }
		TM->LockInAndResolve();
		const int32 K = BattitoIndiceDelColpo(TM);
		if (!TestTrue(TEXT("⛔ premessa: il colpo e' in sequenza"), K != INDEX_NONE) || !TestTrue(TEXT("⛔ premessa: si entra nel Blast"), TickUntilBlast(TM)))
		{
			return false;
		}
		const float A = TM->AttackShowSeconds;
		const float F = URTPlaybackLibrary::TracerFlightFor(true, TM->TracerFlightSeconds, A);
		const float Delta = (K * A + F + 0.1f) - TM->PlaybackPhaseElapsedForTest(); // dentro [arrivo, arrivo + D_imp)
		if (!TestTrue(TEXT("⛔ premessa: il salto e' lungo"), Delta > 0.05f)) { return false; }
		if (Run == 0)
		{
			TM->Tick(Delta);
		}
		else
		{
			const int32 N = FMath::CeilToInt(Delta * 60.f);
			for (int32 I = 0; I < N; ++I) { TM->Tick(Delta / N); }
		}
		Esito[Run] = Mappa->GetPlaybackCues();
		Orologio[Run] = TM->PlaybackPhaseElapsedForTest();
	}
	if (!TestEqual(TEXT("⛔ premessa: i due mondi sono allo stesso orologio"), Orologio[0], Orologio[1], 1e-3f)
		|| !TestTrue(TEXT("⛔ premessa: il Marker e' nella sua finestra in entrambi"),
			Esito[0].ContainsByPredicate([](const FRTPlaybackCue& C) { return C.Kind == ERTPlaybackCueKind::Marker; })
			&& Esito[1].ContainsByPredicate([](const FRTPlaybackCue& C) { return C.Kind == ERTPlaybackCueKind::Marker; })))
	{
		return false;
	}
	TestEqual(TEXT("stesso numero di cue"), Esito[0].Num(), Esito[1].Num());
	for (int32 I = 0; I < FMath::Min(Esito[0].Num(), Esito[1].Num()); ++I)
	{
		TestTrue(FString::Printf(TEXT("cue %d: stesso tipo e stessa cella"), I),
			Esito[0][I].Kind == Esito[1][I].Kind && Esito[0][I].At == Esito[1][I].At);
		TestEqual(FString::Printf(TEXT("🔴 cue %d: Alpha uguale (salto %.3f, lineare %.3f)"), I, Esito[0][I].Alpha, Esito[1][I].Alpha),
			Esito[0][I].Alpha, Esito[1][I].Alpha, 1e-3f);
	}
	return true;
}

/**
 * `SkipPlayback` a meta' della finestra di un `Marker` spegne il canale: nessuna cue rigiocata, nessuna che resta
 * (spec §2.6, F2). E a fine Blast, con una fase dopo, la cue non sopravvive.
 * ✅ Validato per mutazione (16): `ClearPlaybackCues` tolto da `FinishPlayback`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackCueChannelClearsOnSkipTest,
	"RefactorTactics.Playback.CueChannelClearsOnSkip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackCueChannelClearsOnSkipTest::RunTest(const FString&)
{
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ false);
		ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(World);
		if (!TestTrue(TEXT("turno e mappa di prova"), TM != nullptr && Mappa != nullptr)) { return false; }
		TM->LockInAndResolve();
		bool bPieno = false;
		for (int32 I = 0; I < 600 && TM->IsResolving() && !bPieno; ++I)
		{
			TM->Tick(0.02f);
			bPieno = BattitoHaCue(Mappa, ERTPlaybackCueKind::Marker, FRTCellId(3, 2));
		}
		if (!TestTrue(TEXT("⛔ premessa: un tick prima il canale ha il Marker"), bPieno)) { return false; }
		TM->SkipPlayback();
		// `SkipPlayback` (`RTTurnManager.cpp:9219-9230`) chiude in sincrono via `FinishPlayback`, salvo finestra di
		// reazione aperta (`:9061`); qui nessuna finestra si apre (trappola 4), e la premessa lo asserisce.
		if (!TestFalse(TEXT("⛔ premessa: lo Skip ha chiuso il playback in sincrono"), TM->IsResolving())) { return false; }
		TestEqual(TEXT("🔴 dopo SkipPlayback il canale e' vuoto"), Mappa->NumPlaybackCues(), 0);
	}
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ false, /*bConMove=*/ true);
		ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(World);
		if (!TestTrue(TEXT("turno con Move e mappa"), TM != nullptr && Mappa != nullptr)) { return false; }
		TM->LockInAndResolve();
		bool bDopoIlBlast = false, bCueDopoIlBlast = false;
		for (int32 I = 0; I < 600 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.02f);
			const ERTMatchPhase Fase = TM->CurrentPlaybackPhaseForTest();
			if (Fase == ERTMatchPhase::Move) { bDopoIlBlast = true; bCueDopoIlBlast |= BattitoHaCueDiColpo(Mappa); }
		}
		TestTrue(TEXT("⛔ premessa: dopo il Blast c'e' una fase Move"), bDopoIlBlast);
		// ⚠️ Regressione, rete senza casi: coi tetti di R2 l'ultima finestra finisce prima di `N·A` e `PushPlaybackCues`
		// svuota gia' il canale; il `ClearPlaybackCues` della finalizzazione del Blast e' la rete, come per il tracer.
		TestFalse(TEXT("a fine Blast nessuna cue di colpo sopravvive"), bCueDopoIlBlast);
	}
	return true;
}
```

- [ ] **Step 2: Compila e verifica che fallisca**

Run: build. Expected: **rosso di compilazione** — `ImpactCueFor`, `FootprintCueFor`, `FootprintFxForSequence`, `BlastHitCuesAt`, `ImpactCueDuration` non esistono.

- [ ] **Step 3: Le funzioni pure**

`Turn/RTPlaybackLibrary.cpp`, dopo l'include del Task 2:

```cpp
#include "RefactorTactics.h" // #3578: `LogRT`, per il log `Verbose` di R14
```

`Turn/RTPlaybackLibrary.h`, dopo `BlastActivationCuesAt` (Task 3):

```cpp

	/** `D_imp = A > 0 ? Min(Max(0, ImpactCueSeconds), A − F_eff) : 0` (R2): finisce prima del lancio dopo. */
	static float ImpactCueDuration(float ImpactCueSeconds, float AttackShowSeconds, float Flight);

	/**
	 * Il `Marker` di un `Attack`: sulla `HitGeometry.Impact`, se il profilo dice `Marker`, la geometria e' risolta e
	 * chi guarda e' in `ImpactVerdict`. Ogni vittima e' un evento: ogni `Attack` ha il suo (F6).
	 */
	static bool ImpactCueFor(const FRTResolvedEvent& Atk, int32 ViewerTeamId, float Alpha, FRTPlaybackCue& OutCue);

	/**
	 * La cue d'impronta dell'atto, portata dal colpo che la consuma: `AreaPulse` sull'`AimCell` dell'impronta,
	 * `ConeSweep` da `Origin` verso `AimCell`. Il VERDETTO e' quello del colpo (`FromVerdict`: per un'`Area` e' il
	 * centro), le CELLE quelle dell'impronta, e coincidono per costruzione (spec §2.4).
	 */
	static bool FootprintCueFor(const FRTResolvedEvent& Footprint, const FRTResolvedEvent& Atk, int32 ViewerTeamId,
		float Alpha, FRTPlaybackCue& OutCue);

	/**
	 * Per ogni elemento della sequenza, l'indice di timeline dell'impronta che consuma, o `INDEX_NONE` (spec §2.4):
	 * un `AttackFootprint` apre la chiave `(SourceStableUnitId, ActionId)` del suo atto, il PRIMO `Attack` successivo con
	 * la stessa chiave la consuma. Sorgente `0` (D-063): nessuna associazione. R14: una seconda impronta con la stessa
	 * chiave, ancora aperta, SOSTITUISCE la prima (log `Verbose`). Pura: si ricalcola anche estendendo.
	 */
	static TArray<int32> FootprintFxForSequence(const TArray<FRTResolvedEvent>& Timeline,
		const TArray<FRTBlastSequenceElement>& Sequence);

	/**
	 * Blast: per ogni `Attack` ARRIVATO (`BeatsDone > 2k+1`) nella sua finestra `[k·A + F_k, k·A + F_k + D_imp(k))`, il
	 * `Marker` e, se l'elemento consuma un'impronta, la cue d'impronta. Con volo nullo l'arrivo coincide col lancio.
	 */
	static void BlastHitCuesAt(const TArray<FRTResolvedEvent>& Timeline, const TArray<FRTBlastSequenceElement>& Sequence,
		const TArray<float>& Flights, const TArray<int32>& FootprintFx, int32 BeatsDone, float PhaseElapsed,
		float AttackShowSeconds, float ImpactCueSeconds, int32 ViewerTeamId, TArray<FRTPlaybackCue>& Out);
```

`Turn/RTPlaybackLibrary.cpp`, dopo `BlastActivationCuesAt`:

```cpp

float URTPlaybackLibrary::ImpactCueDuration(float ImpactCueSeconds, float AttackShowSeconds, float Flight)
{
	return AttackShowSeconds > 0.f
		? FMath::Max(0.f, FMath::Min(FMath::Max(0.f, ImpactCueSeconds), AttackShowSeconds - FMath::Max(0.f, Flight)))
		: 0.f;
}

bool URTPlaybackLibrary::ImpactCueFor(const FRTResolvedEvent& Atk, int32 ViewerTeamId, float Alpha, FRTPlaybackCue& OutCue)
{
	if (Atk.Type != ERTResolvedEventType::Attack
		|| !Atk.HitGeometry.bResolved
		|| !Atk.HitGeometry.ImpactVerdict.AllowsTeam(ViewerTeamId))
	{
		return false;
	}
	if (URTPresentationBindingLibrary::FxProfileFor(Atk.ActionId, Atk.BaseActionId, Atk.Shape).Impact != ERTImpactFxStyle::Marker)
	{
		return false;
	}
	OutCue.Kind = ERTPlaybackCueKind::Marker;
	OutCue.At = Atk.HitGeometry.Impact;
	OutCue.Toward = Atk.HitGeometry.Impact;
	OutCue.Alpha = FMath::Clamp(Alpha, 0.f, 1.f);
	return true;
}

bool URTPlaybackLibrary::FootprintCueFor(const FRTResolvedEvent& Footprint, const FRTResolvedEvent& Atk,
	int32 ViewerTeamId, float Alpha, FRTPlaybackCue& OutCue)
{
	if (Footprint.Type != ERTResolvedEventType::AttackFootprint || Atk.Type != ERTResolvedEventType::Attack
		|| !Atk.HitGeometry.bResolved
		|| !Atk.HitGeometry.FromVerdict.AllowsTeam(ViewerTeamId))
	{
		return false;
	}
	switch (URTPresentationBindingLibrary::FxProfileFor(Atk.ActionId, Atk.BaseActionId, Atk.Shape).Footprint)
	{
	case ERTFootprintFxStyle::AreaPulse:
		OutCue.Kind = ERTPlaybackCueKind::AreaPulse;
		OutCue.At = Footprint.AimCell;
		OutCue.Toward = Footprint.AimCell;
		break;
	case ERTFootprintFxStyle::ConeSweep:
		OutCue.Kind = ERTPlaybackCueKind::ConeSweep;
		OutCue.At = Footprint.Origin;
		OutCue.Toward = Footprint.AimCell;
		break;
	default:
		return false;
	}
	OutCue.Alpha = FMath::Clamp(Alpha, 0.f, 1.f);
	return true;
}

TArray<int32> URTPlaybackLibrary::FootprintFxForSequence(const TArray<FRTResolvedEvent>& Timeline,
	const TArray<FRTBlastSequenceElement>& Sequence)
{
	TArray<int32> Out;
	Out.Init(INDEX_NONE, Sequence.Num());
	// ⛔ Solo `Find`/`Add`/`Remove`: l'esito non dipende dall'ordine d'iterazione della mappa.
	TMap<TPair<int32, FName>, int32> Aperte;
	for (int32 K = 0; K < Sequence.Num(); ++K)
	{
		const int32 Indice = Sequence[K].TimelineIndex;
		if (!Timeline.IsValidIndex(Indice))
		{
			continue;
		}
		const FRTResolvedEvent& Ev = Timeline[Indice];
		if (Ev.SourceStableUnitId == 0)
		{
			continue; // D-063: un atto non attribuibile non si associa (spec §2.4, degrado)
		}
		const TPair<int32, FName> Chiave(Ev.SourceStableUnitId, Ev.ActionId);
		if (Ev.Type == ERTResolvedEventType::AttackFootprint)
		{
			if (Aperte.Contains(Chiave))
			{
				UE_LOG(LogRT, Verbose, TEXT("FootprintFxForSequence: una seconda impronta per (%d, %s) sostituisce la prima (R14)"),
					Ev.SourceStableUnitId, *Ev.ActionId.ToString());
			}
			Aperte.Add(Chiave, Indice);
		}
		else if (Ev.Type == ERTResolvedEventType::Attack)
		{
			if (const int32* Impronta = Aperte.Find(Chiave))
			{
				Out[K] = *Impronta;
				Aperte.Remove(Chiave); // il PRIMO colpo la consuma: una cue d'impronta per impronta
			}
		}
	}
	return Out;
}

void URTPlaybackLibrary::BlastHitCuesAt(const TArray<FRTResolvedEvent>& Timeline,
	const TArray<FRTBlastSequenceElement>& Sequence, const TArray<float>& Flights, const TArray<int32>& FootprintFx,
	int32 BeatsDone, float PhaseElapsed, float AttackShowSeconds, float ImpactCueSeconds, int32 ViewerTeamId,
	TArray<FRTPlaybackCue>& Out)
{
	for (int32 K = 0; K < Sequence.Num(); ++K)
	{
		const int32 BattitoArrivo = 2 * K + 1;
		if (BeatsDone <= BattitoArrivo)
		{
			break; // la sequenza dei battiti e' monotona: dopo il primo non arrivato, nessuno lo e'
		}
		const int32 Indice = Sequence[K].TimelineIndex;
		if (!Timeline.IsValidIndex(Indice) || Timeline[Indice].Type != ERTResolvedEventType::Attack)
		{
			continue;
		}
		const FRTResolvedEvent& Atk = Timeline[Indice];
		const float Volo = Flights.IsValidIndex(K) ? Flights[K] : 0.f;
		const float Durata = ImpactCueDuration(ImpactCueSeconds, AttackShowSeconds, Volo);
		const float Inizio = AttackBeatSeconds(BattitoArrivo, AttackShowSeconds, Flights);
		if (Durata <= 0.f || PhaseElapsed < Inizio || PhaseElapsed >= Inizio + Durata)
		{
			continue;
		}
		const float Alpha = (PhaseElapsed - Inizio) / Durata;
		const int32 Impronta = FootprintFx.IsValidIndex(K) ? FootprintFx[K] : INDEX_NONE;
		FRTPlaybackCue Cue;
		if (ImpactCueFor(Atk, ViewerTeamId, Alpha, Cue))
		{
			Out.Add(Cue);
		}
		if (Timeline.IsValidIndex(Impronta) && FootprintCueFor(Timeline[Impronta], Atk, ViewerTeamId, Alpha, Cue))
		{
			Out.Add(Cue);
		}
	}
}
```

- [ ] **Step 4: Il turno**

`Turn/RTTurnManager.h`, dopo `TArray<float> PlaybackBlastFlights;` (`:3490`):

```cpp
	/**
	 * #3578: per ogni elemento di `PlaybackBlastSequence`, l'indice di timeline dell'impronta che consuma (o
	 * `INDEX_NONE`), da `URTPlaybackLibrary::FootprintFxForSequence`. Parallelo alla sequenza come i voli, e come loro si
	 * ricalcola anche estendendo: funzione pura degli eventi.
	 */
	TArray<int32> PlaybackBlastFootprintFx;
```

`Turn/RTTurnManager.cpp`, in `BeginPlayback` dopo la `}` del ciclo dei voli (`:7975`, ancora `URTPlaybackLibrary::IsTracerEligible(Ev), TracerFlightSeconds, AttackShowSeconds));` seguita da `}`):

```cpp
	// #3578 (spec «il profilo FX» §2.4): quale impronta porta ogni colpo. ⚠️ Si ricostruisce sull'INTERA sequenza
	// anche estendendo: un'impronta consumata nel prefisso congelato resta consumata.
	PlaybackBlastFootprintFx = URTPlaybackLibrary::FootprintFxForSequence(ResolvedTimeline, PlaybackBlastSequence);
```

In `PushPlaybackCues` (Task 3), nel ramo `else if (Phase == ERTMatchPhase::Blast)`, dopo la chiamata a `BlastActivationCuesAt`:

```cpp
		URTPlaybackLibrary::BlastHitCuesAt(ResolvedTimeline, PlaybackBlastSequence, PlaybackBlastFlights,
			PlaybackBlastFootprintFx, BlastBeatsDone, PlaybackPhaseElapsed, AttackShowSeconds, ImpactCueSeconds,
			PlaybackViewerTeamId, Cues);
```

In `FinishPlayback`, dopo `PlaybackBlastFlights.Reset();` (`:9187`):

```cpp
	PlaybackBlastFootprintFx.Reset(); // #3578: parallelo alla sequenza, si svuota con lei
```

- [ ] **Step 5: I commenti superati (spec §2.9)**

`Turn/RTTurnManager.cpp:6374-6377` (ancora: `Oggi non conta, perche' \`Area\` non e' idonea al`): sostituisci le due frasi da `Oggi non conta` a `non come «dove sta chi spara».` con:

```cpp
		// ⌫ *Fino a #3578 qui c'era «oggi non conta, perche' `Area` non e' idonea al tracer».* Da #3578 conta: per
		// un'`Area` questo `FromVerdict` e' il verdetto dell'`AreaPulse` sul centro (spec «il profilo FX» §2.4) — aperto
		// alla squadra dell'attaccante e, per le altre, solo se vedono il centro.
```

`Turn/RTResolvedEvent.h:196-197`: `irrilevante finche' \`Area\` non e' idonea al tracer, ma e' il significato del campo.` diventa `da #3578 e' il centro dell'\`AreaPulse\` (spec «il profilo FX» §2.4).`; `:205-207`, in coda al commento di `FromVerdict`: `Da #3578 e' il verdetto dell'\`AreaPulse\`.`

- [ ] **Step 6: Build e verde**

Run: build; test `RefactorTactics.Fx+RefactorTactics.Privacy.ImpactMarkerNeedsTheVictim+RefactorTactics.Privacy.AreaPulseNeedsTheCenter+RefactorTactics.Playback.EveryAttackGetsItsProfileMarker+RefactorTactics.Playback.FootprintCueOncePerFootprint+RefactorTactics.Playback.SecondFootprintReplacesTheFirst+RefactorTactics.Playback.FxCuesNeverOverlapInTheBlast+RefactorTactics.Playback.ImpactCueComesAtTheArrival+RefactorTactics.Playback.UnresolvedGeometryHasNoFxButPlaysTheClip+RefactorTactics.Playback.FxCuesAreAFunctionOfTheClock+RefactorTactics.Playback.CueChannelClearsOnSkip`, log `<scratchpad>/t4-verde.log`. Expected: verdi. `TickPlayback` non taglia `DeltaSeconds` (`RTTurnManager.cpp:8606`, `:8629`, misurato in review): se `FxCuesAreAFunctionOfTheClock` cade sulla **premessa** dell'orologio, la causa è un'altra (una fase finalizzata nel salto) e non si allenta: si riporta.

- [ ] **Step 7: I gate**

Run: `RefactorTactics.Playback+RefactorTactics.Privacy+RefactorTactics.Determinism+RefactorTactics.HexMapActor+RefactorTactics.Presentation`, log `<scratchpad>/t4-gate.log`. Expected: verdi.

- [ ] **Step 8: Commit, poi mutazioni (9), (10), (11), (12), (13), (14), (16), (21), (P3), (P4)**

```bash
git add Source/RefactorTactics/Turn/RTPlaybackLibrary.h Source/RefactorTactics/Turn/RTPlaybackLibrary.cpp Source/RefactorTactics/Turn/RTTurnManager.h Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Turn/RTResolvedEvent.h Source/RefactorTactics/Tests/RTAbilityFxProfileTests.cpp Source/RefactorTactics/Tests/RTAttackTracerPlaybackTests.cpp
git commit -F - <<'EOF'
feat(3578): le cue di colpo — Marker all'arrivo di ogni colpo, AreaPulse e ConeSweep una volta per impronta (R14)

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
```

Ogni mutazione: modifica, build, test indicato, `Result={Fail}` sull'asserto indicato, `git checkout -- <file>`, build, verde.

➕ esecuzione (Task 4). Eseguito con **un solo** verde finale, sul commit pulito dopo l'ultimo ripristino, invece di un verde dopo ogni ripristino. La review l'ha accettato con tre misure: l'albero finale è il commit e la build finale è posteriore all'ultima scrittura sui sorgenti; il binario finale contiene i sorgenti ripristinati; ogni log di mutazione mostra **solo** le cadute della propria mutante, quindi nessun ripristino è rimasto incompleto. Lo scarto dal protocollo si dichiara nella PR.

9. In `BlastHitCuesAt`, `const int32 BattitoArrivo = 2 * K + 1;` diventa `const int32 BattitoArrivo = 2 * K;` (cursore **e** inizio della finestra al lancio). Test `RefactorTactics.Playback.ImpactCueComesAtTheArrival`: cade «🔴 al battito 2k (in volo) nessun Marker» (la premessa `F_eff > 0` è asserita prima: la mutante non è vacua). Nello stesso test cade **anche** «🔴 al battito 2k+1 il Marker sulla cella d'impatto»: la finestra mutata è `[k·A, k·A + 0.2)` e l'arrivo cade a `k·A + 0.25`, fuori. Cadono anche `Fx.AreaPulseIsOnTheFootprintAim` e `Playback.EveryAttackGetsItsProfileMarker` se il loro istante di prova esce dalla finestra spostata: la PR li elenca come esito misurato (➕ rev2.).
10. In `ImpactCueFor`, cancella `|| !Atk.HitGeometry.ImpactVerdict.AllowsTeam(ViewerTeamId)`. Test `RefactorTactics.Privacy.ImpactMarkerNeedsTheVictim`: cade «🔴 chi non conosceva la vittima non vede il Marker».
11. In `FootprintCueFor`, ramo `AreaPulse`, `OutCue.At = Footprint.AimCell;` diventa `OutCue.At = Atk.HitGeometry.Impact;`. Test `RefactorTactics.Fx.AreaPulseIsOnTheFootprintAim`: cade «🔴 l'AreaPulse e' sull'AimCell dell'impronta» (premessa «l'AimCell non e' l'Impact di nessun colpo» asserita prima, ➕ rev2.). Cadono anche `Playback.SecondFootprintReplacesTheFirst` «il pulse e' sul centro della seconda» e `Privacy.AreaPulseNeedsTheCenter` «controllo…»: la PR li elenca.
12. In `BlastHitCuesAt`, `if (ImpactCueFor(Atk, ViewerTeamId, Alpha, Cue))` diventa `if (Impronta == INDEX_NONE && ImpactCueFor(Atk, ViewerTeamId, Alpha, Cue))` (il `Marker` saltato sul colpo che consuma l'impronta, cioè il primo dell'atto). Cadono `Playback.EveryAttackGetsItsProfileMarker` «🔴 elemento 1: Marker sulla sua vittima all'arrivo» (e «elemento 4») **e** (➕ rev2.) `Playback.ImpactCueComesAtTheArrival` «🔴 al battito 2k+1 il Marker…» (un atto con una vittima) e `Playback.FootprintCueOncePerFootprint` «e due Marker, uno per colpo».
13. In `ActivationCueDuration` e `ImpactCueDuration`, togli i tetti: `FMath::Min(FMath::Max(0.f, ActivationCueSeconds), AttackShowSeconds)` → `FMath::Max(0.f, ActivationCueSeconds)` e `FMath::Max(0.f, FMath::Min(FMath::Max(0.f, ImpactCueSeconds), AttackShowSeconds - FMath::Max(0.f, Flight)))` → `FMath::Max(0.f, ImpactCueSeconds)`. Test `RefactorTactics.Playback.FxCuesNeverOverlapInTheBlast`: cade «🔴 A=… act=2A…: 0 finisce prima che 1 cominci».
14. In `PushPlaybackCues`, nel ramo Blast dopo `BlastHitCuesAt`, inserisci la mutante e verifica **prima** nel log che i due `Alpha` stampati dall'asserto siano diversi (o la mutazione è vacua):
    ```cpp
    { // MUTANTE (14): Alpha da un accumulatore azzerato al tick della rivelazione, non dall'orologio
        static int32 UltimoBattito = -1; static float UltimoT = 0.f; static float CueElapsed = 0.f;
        if (BlastBeatsDone != UltimoBattito) { UltimoBattito = BlastBeatsDone; CueElapsed = 0.f; }
        else { CueElapsed += PlaybackPhaseElapsed - UltimoT; }
        UltimoT = PlaybackPhaseElapsed;
        for (FRTPlaybackCue& C : Cues) { if (C.Kind == ERTPlaybackCueKind::Marker) { C.Alpha = FMath::Clamp(CueElapsed / FMath::Max(ImpactCueSeconds, KINDA_SMALL_NUMBER), 0.f, 1.f); } }
    }
    ```
    Test `RefactorTactics.Playback.FxCuesAreAFunctionOfTheClock`: cade «🔴 cue …: Alpha uguale (salto 0.000, lineare ≈0.5)» — col tick unico l'arrivo avviene nello stesso tick, l'accumulatore vale 0.
16. In `FinishPlayback`, cancella `FootprintMap->ClearPlaybackCues();`. Test `RefactorTactics.Playback.CueChannelClearsOnSkip`: cade «🔴 dopo SkipPlayback il canale e' vuoto». Premessa (misurata in review): `SkipPlayback` passa da `FinishPlayback` in sincrono (`RTTurnManager.cpp:9219-9230`) fuori da una finestra di reazione (`:9061`), quindi la mutante non è vacua; il test lo asserisce («⛔ premessa: lo Skip ha chiuso…»).
21. In `FootprintFxForSequence`, cancella `Aperte.Remove(Chiave);`. Test `RefactorTactics.Playback.FootprintCueOncePerFootprint`: cade «🔴 un solo AreaPulse in tutto il Blast» (due corse, ➕ rev2. contate su tutti gli istanti).
P3. In `FootprintCueFor`, cancella `|| !Atk.HitGeometry.FromVerdict.AllowsTeam(ViewerTeamId)`. Test `RefactorTactics.Privacy.AreaPulseNeedsTheCenter`: cade «🔴 chi non vede il centro non riceve l'AreaPulse».
P4. In `ImpactCueFor`, cancella `|| !Atk.HitGeometry.bResolved`. Test `RefactorTactics.Playback.UnresolvedGeometryHasNoFxButPlaysTheClip`: cade «🔴 geometria irrisolta: nessun Marker, anche coi verdetti aperti».

---

### Task 5: Il disegno sulla mappa e D-278 — `CueSegments`, lo `Zigzag` disegnato, le voci dichiarate

**Files:**
- Modify: `Turn/RTPlaybackLibrary.h` / `.cpp` (`CueSegments`, dopo `BlastHitCuesAt`)
- Modify: `Map/RTHexMapActor.cpp:172-179` (costanti graybox) e `:1659-1679` (il disegno: `Zigzag` nel ciclo dei tracer, poi le cue)
- Modify: `Turn/RTPresentationBinding.cpp:36-42` (voce `Attack`) e `:265-272` (voce `AbilityActivated`)
- Test: `Tests/RTAbilityFxProfileTests.cpp`, `Tests/RTPreviewLineBatcherTests.cpp` (prima di `#endif`, `:186`), `Tests/RTPresentationBindingTests.cpp` (prima di `#endif`, `:777`)

**Interfaces:**
- Produces (statica C++ in `URTPlaybackLibrary`):
  ```cpp
  static void CueSegments(ERTPlaybackCueKind Kind, const FVector& At, const FVector& Toward, float HexSize, float Alpha,
      TArray<FVector>& OutStarts, TArray<FVector>& OutEnds);
  ```
- Consumes: `FRTPlaybackCue` (Task 1), `GetPlaybackCues`/`PlaybackCues` (Task 3), `TracerPolyline` (Task 2), `FootprintCueFor`/`FootprintFxForSequence` (Task 4), `DisegnaLineaAnteprima` (`Map/RTHexMapActor.cpp:1422-1435`), `URTOverlayPalette::ColorFor` (`Map/RTOverlayPalette.h`).

La geometria (spec §2.1, F7, F21), con `s = HexSize`, vertici dell'esagono a `30° + 60°·i` nel piano:

| Tipo | Segmenti | a `α = 0.5`, `s = 100` | Spessore |
|---|---|---|---|
| `Ring` | 6 (esagono, raggio `Lerp(0.55, 0.95, α)·s`) | distanze 75 · 75, verticale 0 | 3 |
| `Pulse` | 12 (raggi `Lerp(1.00, 0.60, α)·s` e `Lerp(0.75, 0.35, α)·s`) | 80 · 55, verticale 0 | 3 |
| `Flash` | 6 raggi da `0.5 s` sui vertici, lunghi `0.4 s`, a 45° verso l'alto e l'esterno | 83 · 50, verticale 28 | 4 |
| `Marker` | 4 raggi a X dal centro, lunghi `Lerp(0, 0.35, α)·s` | 17.5 · 0, verticale 0 | 3 |
| `AreaPulse` | 12 (esagono `Lerp(0.3, 1.7, α)·s` + 6 raggi centro → vertici) | 100 · 0, verticale 0 | 3 |
| `ConeSweep` | 3 (bordi a `±60°` lunghi `0.3 L`, braccio lungo `L` a `Lerp(−60°, +60°, α)`) | `L` · 0, verticale 0 | 4 |
| `Zigzag` (tracer) | 8 (`TracerPolyline`) | — | 5 |

Le coppie con lo stesso numero di segmenti (`Ring`/`Flash`, `Pulse`/`AreaPulse`) differiscono di ≥ `0.1 s` nella distanza minima o massima: è ciò che `Fx.CueStylesDifferByGeometry` asserisce.

- [ ] **Step 1: I test rossi**

`Tests/RTAbilityFxProfileTests.cpp`, prima di `// --- I test dei Task 3, 4 e 5 si aggiungono QUI`:

```cpp

// --- La geometria delle cue (#3578, spec §2.1, F7) -------------------------------------------------------------

namespace
{
	struct FFxFirma { int32 Segmenti = 0; float Max = 0.f; float Min = TNumericLimits<float>::Max(); float Verticale = 0.f; };

	FFxFirma FxFirmaDi(ERTPlaybackCueKind Tipo)
	{
		const FVector Ancora(0.f, 0.f, 0.f);
		TArray<FVector> Da, A;
		URTPlaybackLibrary::CueSegments(Tipo, Ancora, FVector(300.f, 0.f, 0.f), 100.f, 0.5f, Da, A);
		FFxFirma F;
		F.Segmenti = Da.Num();
		float ZMin = TNumericLimits<float>::Max(), ZMax = -TNumericLimits<float>::Max();
		for (int32 I = 0; I < Da.Num(); ++I)
		{
			for (const FVector& P : { Da[I], A[I] })
			{
				const float D = FVector::Dist(P, Ancora);
				F.Max = FMath::Max(F.Max, D);
				F.Min = FMath::Min(F.Min, D);
				ZMin = FMath::Min(ZMin, P.Z);
				ZMax = FMath::Max(ZMax, P.Z);
			}
		}
		F.Verticale = Da.Num() > 0 ? ZMax - ZMin : 0.f;
		return F;
	}
}

/**
 * Le sei cue sono DIVERSE in geometria, a coppie (spec §2.1, F7, #2453: il canale e' la geometria, mai il solo
 * colore): per ogni coppia, numero di segmenti diverso, oppure ≥ 0.1 s di differenza nella distanza massima o minima
 * dall'ancora, oppure nell'estensione verticale. A `α = 0.5`, `s = 100`.
 * ✅ Validato per mutazione (20): `AreaPulse` disegnato come `Ring` fa cadere la coppia `Ring`/`AreaPulse`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxCueStylesDifferByGeometryTest,
	"RefactorTactics.Fx.CueStylesDifferByGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxCueStylesDifferByGeometryTest::RunTest(const FString&)
{
	const TArray<ERTPlaybackCueKind> Tipi = { ERTPlaybackCueKind::Ring, ERTPlaybackCueKind::Pulse, ERTPlaybackCueKind::Flash,
		ERTPlaybackCueKind::Marker, ERTPlaybackCueKind::AreaPulse, ERTPlaybackCueKind::ConeSweep };
	const float Soglia = 0.1f * 100.f;
	for (int32 I = 0; I < Tipi.Num(); ++I)
	{
		const FFxFirma Fi = FxFirmaDi(Tipi[I]);
		TestTrue(FString::Printf(TEXT("%s: almeno un segmento"), *UEnum::GetValueAsString(Tipi[I])), Fi.Segmenti > 0);
		for (int32 J = I + 1; J < Tipi.Num(); ++J)
		{
			const FFxFirma Fj = FxFirmaDi(Tipi[J]);
			const bool bDiversi = Fi.Segmenti != Fj.Segmenti || FMath::Abs(Fi.Max - Fj.Max) >= Soglia
				|| FMath::Abs(Fi.Min - Fj.Min) >= Soglia || FMath::Abs(Fi.Verticale - Fj.Verticale) >= Soglia;
			TestTrue(FString::Printf(TEXT("🔴 %s e %s si distinguono per geometria"),
				*UEnum::GetValueAsString(Tipi[I]), *UEnum::GetValueAsString(Tipi[J])), bDiversi);
		}
	}
	return true;
}

/**
 * Il `ConeSweep` (spec §2.4, F4): da un atto `Cone` costruito dal test — nessuna azione del catalogo dichiara `Cone`
 * (spec §1) — la cue e' ancorata all'`Origin` dell'impronta e punta all'`AimCell`; la geometria ha i due bordi
 * simmetrici attorno all'asse e il braccio lungo `|Origin → AimCell|`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFxConeSweepAxisIsTheAimTest,
	"RefactorTactics.Fx.ConeSweepAxisIsTheAim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFxConeSweepAxisIsTheAimTest::RunTest(const FString&)
{
	TArray<FRTResolvedEvent> T;
	T.Add(FxImpronta(3, TEXT("Hero.Prova.Ventaglio"), ERTAbilityShape::Cone, FRTCellId(0, 0), FRTCellId(2, 0)));
	T.Add(FxColpo(3, TEXT("Hero.Prova.Ventaglio"), ERTAbilityShape::Cone, FRTCellId(0, 0), FRTCellId(1, 0)));
	const float Arrivo = URTPlaybackLibrary::AttackBeatSeconds(3, 0.5f, FxVoli(T, 0.5f));
	const TArray<FRTPlaybackCue> C = FxCueAl(T, Arrivo + 0.01f, 0.5f);
	const FRTPlaybackCue* Sweep = C.FindByPredicate([](const FRTPlaybackCue& X) { return X.Kind == ERTPlaybackCueKind::ConeSweep; });
	if (!TestNotNull(TEXT("🔴 l'atto Cone ha il suo ConeSweep"), Sweep)) { return false; }
	TestTrue(TEXT("ancorato all'Origin dell'impronta"), Sweep->At == FRTCellId(0, 0));
	TestTrue(TEXT("verso l'AimCell dell'impronta, non verso la vittima"), Sweep->Toward == FRTCellId(2, 0));

	const FVector Da(0.f, 0.f, 0.f), Verso(200.f, 0.f, 0.f);
	TArray<FVector> S, E;
	URTPlaybackLibrary::CueSegments(ERTPlaybackCueKind::ConeSweep, Da, Verso, 100.f, 0.5f, S, E);
	if (!TestEqual(TEXT("tre segmenti"), S.Num(), 3)) { return false; }
	for (const FVector& P : S) { TestTrue(TEXT("ogni segmento parte dall'origine"), P.Equals(Da, 0.01f)); }
	// ⚠️ Letterali `double`: `FVector` e' in doppia precisione (LWC), e `TestEqual(double, float, float)` e' ambiguo.
	TestEqual(TEXT("bordo sinistro lungo 0.3 L"), FVector::Dist(Da, E[0]), 0.3 * 200.0, 0.5);
	TestEqual(TEXT("bordo destro lungo 0.3 L"), FVector::Dist(Da, E[1]), 0.3 * 200.0, 0.5);
	TestEqual(TEXT("i bordi sono simmetrici attorno all'asse"), E[0].Y, -E[1].Y, 0.5);
	TestEqual(TEXT("🔴 il braccio e' lungo |Origin → AimCell|"), FVector::Dist(Da, E[2]), 200.0, 0.5);
	TestEqual(TEXT("a α = 0.5 il braccio sta sull'asse"), E[2].Y, 0.0, 0.5);
	return true;
}
```

➕ esecuzione (Task 5, commit `e835c5fdd`). Con i letterali `float` della prima stesura il test non compilava: `C2666`, perché `FVector::Dist` e `FVector::Y` sono `double` (LWC) e `TestEqual(double, float, float)` è ambiguo. Il blocco qui sopra porta già i letterali `double`; asserti e tolleranze sono gli stessi.

`Tests/RTPreviewLineBatcherTests.cpp`, prima di `#endif` (`:186`):

```cpp

// Le cue del profilo FX e lo `Zigzag` si disegnano col debug spento (#3578, D-467), nel batcher Foreground, col colore
// `Attack` (R1): gemello di `TracerDrawsWithDebugDrawingOff`. Un tipo per volta, col numero di segmenti di §2.1.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPreviewFxCuesDrawWithDebugDrawingOffTest,
	"RefactorTactics.Preview.FxCuesDrawWithDebugDrawingOff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPreviewFxCuesDrawWithDebugDrawingOffTest::RunTest(const FString&)
{
	UWorld* World = MakeLineeAnteprimaWorld();
	if (!TestNotNull(TEXT("World creato"), World)) { return false; }
	ULineBatchComponent* Primo = World->GetLineBatcher(UWorld::ELineBatcherType::Foreground);
	IConsoleVariable* Debug = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EnableDrawDebugHelpers"));
	if (!TestTrue(TEXT("premessa: batcher Foreground e CVar di debug presenti"), Primo != nullptr && Debug != nullptr))
	{
		DestroyLineeAnteprimaWorld(World);
		return false;
	}
	RTTestConsoleVariable::TGuardia<int32> DebugSpento(*Debug, 0);
	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	if (!TestNotNull(TEXT("HexMap spawnato"), HexMap)) { DestroyLineeAnteprimaWorld(World); return false; }
	HexMap->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);
	const FLinearColor Attacco(URTOverlayPalette::ColorFor(ERTOverlayMeaning::Attack));

	const TArray<TPair<ERTPlaybackCueKind, int32>> Attesi = {
		{ ERTPlaybackCueKind::Ring, 6 }, { ERTPlaybackCueKind::Pulse, 12 }, { ERTPlaybackCueKind::Flash, 6 },
		{ ERTPlaybackCueKind::Marker, 4 }, { ERTPlaybackCueKind::AreaPulse, 12 }, { ERTPlaybackCueKind::ConeSweep, 3 } };
	for (const TPair<ERTPlaybackCueKind, int32>& Atteso : Attesi)
	{
		FRTPlaybackCue C;
		C.Kind = Atteso.Key;
		C.At = FRTCellId(0, 0);
		C.Toward = FRTCellId(2, 0);
		C.Alpha = 0.5f;
		HexMap->SetPlaybackCues({ C });
		Primo->Flush();
		static_cast<AActor*>(HexMap)->Tick(0.f);
		const FString Nome = UEnum::GetValueAsString(Atteso.Key);
		TestEqual(FString::Printf(TEXT("🔴 %s: le sue linee nel batcher Foreground"), *Nome), Primo->BatchedLines.Num(), Atteso.Value);
		bool bTutteAttack = Primo->BatchedLines.Num() > 0;
		for (const FBatchedLine& L : Primo->BatchedLines) { bTutteAttack &= L.Color.Equals(Attacco); }
		TestTrue(FString::Printf(TEXT("%s: col colore Attack della palette"), *Nome), bTutteAttack);
	}
	HexMap->ClearPlaybackCues();

	FRTPlaybackTracer T;
	T.From = FRTCellId(0, 0);
	T.To = FRTCellId(3, 0);
	T.Alpha = 1.f;
	T.Style = ERTTracerStyle::Zigzag;
	HexMap->SetPlaybackTracers({ T });
	Primo->Flush();
	static_cast<AActor*>(HexMap)->Tick(0.f);
	TestEqual(TEXT("🔴 lo Zigzag intero: otto linee"), Primo->BatchedLines.Num(), 8);

	DestroyLineeAnteprimaWorld(World);
	return true;
}
```

`Tests/RTPresentationBindingTests.cpp`, prima di `#endif` (`:777`):

```cpp

/**
 * D-278 (#3578, spec «il profilo FX» §2.5): le voci `Attack` e `AbilityActivated` dichiarano `SetPlaybackCues`.
 * ⚠️ Il gate `FindMissingBindings` conta i nomi, non le chiamate: QUALE cue e su QUALE cella lo dicono i test di
 * `Playback.*` che leggono `GetPlaybackCues()`. `AttackFootprint` NON guadagna la cue: la consegna il colpo.
 * ✅ Validato per mutazione (P2): `SetPlaybackCues` tolto da `AbilityActivated`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPresentationFxCueIsDeclaredTest,
	"RefactorTactics.Presentation.FxCueIsDeclaredForAttackAndActivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPresentationFxCueIsDeclaredTest::RunTest(const FString&)
{
	const TArray<FRTPresentationBinding> Tabella = URTPresentationBindingLibrary::DeclaredBindings();
	const FName Cue(TEXT("SetPlaybackCues"));
	for (const ERTResolvedEventType Tipo : { ERTResolvedEventType::Attack, ERTResolvedEventType::AbilityActivated })
	{
		const FRTPresentationBinding* Voce = Tabella.FindByPredicate([Tipo](const FRTPresentationBinding& B) { return B.Type == Tipo; });
		if (!TestNotNull(FString::Printf(TEXT("premessa: la voce %s"), *URTPresentationBindingLibrary::EventTypeName(Tipo)), Voce)) { return false; }
		TestTrue(FString::Printf(TEXT("🔴 %s dichiara SetPlaybackCues"), *URTPresentationBindingLibrary::EventTypeName(Tipo)),
			Voce->Cues.Contains(Cue));
	}
	const FRTPresentationBinding* Impronta = Tabella.FindByPredicate([](const FRTPresentationBinding& B)
	{
		return B.Type == ERTResolvedEventType::AttackFootprint;
	});
	TestTrue(TEXT("AttackFootprint non dichiara la cue: la consegna il colpo che la consuma"),
		Impronta != nullptr && !Impronta->Cues.Contains(Cue));
	TestEqual(TEXT("e il gate resta senza mancanze"), URTPresentationBindingLibrary::FindMissingBindings(Tabella).Num(), 0);
	return true;
}
```

- [ ] **Step 2: Compila e verifica che fallisca**

Run: build. Expected: **rosso di compilazione** — `URTPlaybackLibrary::CueSegments` non esiste. (Il test di D-278 compila e, lanciato dopo lo Step 4 con la sola libreria, sarebbe rosso sull'asserto «🔴 Attack dichiara SetPlaybackCues»: lo si vede allo Step 6 se si lancia prima dello Step 5.)

- [ ] **Step 3: La geometria pura**

`Turn/RTPlaybackLibrary.h`, dopo `BlastHitCuesAt`:

```cpp

	/**
	 * I segmenti di una cue (#3578, spec §2.1, F7, F21), in coppie `OutStarts[i] → OutEnds[i]`, attorno ad `At` (nel mondo).
	 * `Toward` serve solo a `ConeSweep`. Scale in frazioni di `HexSize`, graybox (D-287 punto 7) ma DIVERSE a coppie:
	 * lo pinna `Fx.CueStylesDifferByGeometry`. Pura: la usa il disegno di `ARTHexMapActor`.
	 */
	static void CueSegments(ERTPlaybackCueKind Kind, const FVector& At, const FVector& Toward, float HexSize, float Alpha,
		TArray<FVector>& OutStarts, TArray<FVector>& OutEnds);
```

`Turn/RTPlaybackLibrary.cpp`, dopo `BlastHitCuesAt`:

```cpp

void URTPlaybackLibrary::CueSegments(ERTPlaybackCueKind Kind, const FVector& At, const FVector& Toward, float HexSize,
	float Alpha, TArray<FVector>& OutStarts, TArray<FVector>& OutEnds)
{
	OutStarts.Reset();
	OutEnds.Reset();
	const float A = FMath::Clamp(Alpha, 0.f, 1.f);
	const float S = HexSize;
	auto Direzione = [](float Gradi)
	{
		const float R = FMath::DegreesToRadians(Gradi);
		return FVector(FMath::Cos(R), FMath::Sin(R), 0.f);
	};
	auto Esagono = [&](float Raggio)
	{
		for (int32 I = 0; I < 6; ++I)
		{
			OutStarts.Add(At + Direzione(30.f + 60.f * I) * Raggio);
			OutEnds.Add(At + Direzione(30.f + 60.f * (I + 1)) * Raggio);
		}
	};
	switch (Kind)
	{
	case ERTPlaybackCueKind::Ring:
		Esagono(FMath::Lerp(0.55f, 0.95f, A) * S);
		break;
	case ERTPlaybackCueKind::Pulse:
		Esagono(FMath::Lerp(1.00f, 0.60f, A) * S);
		Esagono(FMath::Lerp(0.75f, 0.35f, A) * S);
		break;
	case ERTPlaybackCueKind::Flash:
		for (int32 I = 0; I < 6; ++I)
		{
			// F21: inclinati di 45° verso l'alto e l'esterno — un raggio verticale, dalla camera tattica, e' un punto.
			const FVector Fuori = Direzione(30.f + 60.f * I);
			const FVector Base = At + Fuori * (0.5f * S);
			OutStarts.Add(Base);
			OutEnds.Add(Base + (Fuori + FVector::UpVector).GetSafeNormal() * (0.4f * S));
		}
		break;
	case ERTPlaybackCueKind::Marker:
		for (int32 I = 0; I < 4; ++I)
		{
			OutStarts.Add(At);
			OutEnds.Add(At + Direzione(45.f + 90.f * I) * (FMath::Lerp(0.f, 0.35f, A) * S));
		}
		break;
	case ERTPlaybackCueKind::AreaPulse:
	{
		const float R = FMath::Lerp(0.3f, 1.7f, A) * S;
		Esagono(R);
		for (int32 I = 0; I < 6; ++I)
		{
			OutStarts.Add(At);
			OutEnds.Add(At + Direzione(30.f + 60.f * I) * R);
		}
		break;
	}
	case ERTPlaybackCueKind::ConeSweep:
	{
		FVector Asse = Toward - At;
		Asse.Z = 0.f;
		const float L = Asse.Size();
		if (L < KINDA_SMALL_NUMBER)
		{
			break; // asse degenere: niente da spazzare, nessun errore (spec §4)
		}
		const float Base = FMath::RadiansToDegrees(FMath::Atan2(Asse.Y, Asse.X));
		OutStarts.Add(At);
		OutEnds.Add(At + Direzione(Base - 60.f) * (0.3f * L));
		OutStarts.Add(At);
		OutEnds.Add(At + Direzione(Base + 60.f) * (0.3f * L));
		OutStarts.Add(At);
		OutEnds.Add(At + Direzione(Base + FMath::Lerp(-60.f, 60.f, A)) * L);
		break;
	}
	}
}
```

- [ ] **Step 4: Il disegno**

`Map/RTHexMapActor.cpp`, dopo `constexpr float RTTracerJetThickness = 7.f;` (`:179`):

```cpp

	// Profilo FX (#3578, spec «il profilo FX» §2.1): spessori per tipo e sollevamento sopra la cella. ⚠️ Graybox,
	// tarati in PIE (`PIE-FX-ABILITA`); le scale vivono in `URTPlaybackLibrary::CueSegments`.
	constexpr float RTTracerZigzagThickness = 5.f;
	constexpr float RTCueLift = 6.f;
	constexpr float RTCueThickness = 3.f;
	constexpr float RTCueThickThickness = 4.f; // `Flash` e `ConeSweep`
```

Nel ciclo dei tracer (`:1667-1678`), sostituisci il corpo del `for (const FRTPlaybackTracer& T : PlaybackTracers)` (ancora: `URTPlaybackLibrary::TracerSegment(T.Style, Da, A, T.Alpha, Size * RTTracerDashFraction, Inizio, Fine);`) con:

```cpp
		for (const FRTPlaybackTracer& T : PlaybackTracers)
		{
			const FVector Da = URTHexLibrary::AxialToWorld(T.From, Origin, Size, LayerH)
				+ FVector(0, 0, CellLift(T.From) + RTTracerHeight);
			const FVector A = URTHexLibrary::AxialToWorld(T.To, Origin, Size, LayerH)
				+ FVector(0, 0, CellLift(T.To) + RTTracerHeight);
			if (T.Style == ERTTracerStyle::Zigzag)
			{
				// #3578: lo zigzag e' una polilinea pura (`TracerPolyline`), un prefisso che cresce.
				TArray<FVector> Punti;
				URTPlaybackLibrary::TracerPolyline(T.Style, Da, A, T.Alpha, Size, Punti);
				for (int32 P = 1; P < Punti.Num(); ++P)
				{
					DisegnaLineaAnteprima(World, Punti[P - 1], Punti[P], TracerColor, SDPG_Foreground, RTTracerZigzagThickness);
				}
				continue;
			}
			FVector Inizio, Fine;
			URTPlaybackLibrary::TracerSegment(T.Style, Da, A, T.Alpha, Size * RTTracerDashFraction, Inizio, Fine);
			DisegnaLineaAnteprima(World, Inizio, Fine, TracerColor, SDPG_Foreground,
				T.Style == ERTTracerStyle::Jet ? RTTracerJetThickness : RTTracerProjectileThickness);
		}
```

Subito dopo la `}` che chiude `if (PlaybackTracers.Num() > 0)` (`:1679`):

```cpp

	// Le cue del profilo FX, durante il playback (#3578, spec «il profilo FX» §2.1, §2.4).
	//
	// 🔑 **Stesso colore del colpo** (R1): `ERTOverlayMeaning::Attack`, nessun significato nuovo (#1941). Gli stili si
	// separano per GEOMETRIA (`CueSegments`), mai per colore. ⚠️ Foreground, come il tracer: un anello sotto chi agisce
	// sparirebbe dentro la sua mesh. ⛔ Nessun `DrawDebug*` (D-467): `DisegnaLineaAnteprima` tace sul server dedicato.
	if (PlaybackCues.Num() > 0)
	{
		const FColor CueColor = URTOverlayPalette::ColorFor(ERTOverlayMeaning::Attack);
		for (const FRTPlaybackCue& C : PlaybackCues)
		{
			const FVector Ancora = URTHexLibrary::AxialToWorld(C.At, Origin, Size, LayerH)
				+ FVector(0, 0, CellLift(C.At) + RTCueLift);
			const FVector Verso = URTHexLibrary::AxialToWorld(C.Toward, Origin, Size, LayerH)
				+ FVector(0, 0, CellLift(C.Toward) + RTCueLift);
			TArray<FVector> Inizi, Fini;
			URTPlaybackLibrary::CueSegments(C.Kind, Ancora, Verso, Size, C.Alpha, Inizi, Fini);
			const float Spessore = (C.Kind == ERTPlaybackCueKind::Flash || C.Kind == ERTPlaybackCueKind::ConeSweep)
				? RTCueThickThickness : RTCueThickness;
			for (int32 I = 0; I < Inizi.Num(); ++I)
			{
				DisegnaLineaAnteprima(World, Inizi[I], Fini[I], CueColor, SDPG_Foreground, Spessore);
			}
		}
	}
```

- [ ] **Step 5: D-278**

`Turn/RTPresentationBinding.cpp`, voce `Attack`: il commento `:36-38` (da `// \`#2454\`: \`SetPlaybackTracers\` — il tracer fra lancio e arrivo, solo per gli attacchi base` a `\`Playback.TracerIsInFlightBetweenLaunchAndArrival\`.`) diventa:

```cpp
	// `#2454`: `SetPlaybackTracers` — il tracer fra lancio e arrivo, secondo il profilo FX (#3578: il volo dalla forma di
	// default di un'azione con un id, il disegno dall'override) e solo se chi guarda conosceva entrambi gli estremi.
	// #3578: `SetPlaybackCues` — il `Marker` all'arrivo di ogni colpo e, sul primo colpo dell'atto, la cue d'impronta
	// (`AreaPulse`, `ConeSweep`): `AttackFootprint` non ha una cue propria, la consegna il colpo che la consuma.
	// ⚠️ Il gate conta i nomi, non le chiamate: che le cue siano chiamate lo dicono
	// `Playback.TracerIsInFlightBetweenLaunchAndArrival` e `Playback.ImpactCueComesAtTheArrival`.
```

e l'elenco `:41-42` guadagna la cue: `FName(TEXT("SetPlaybackTracers")) }));` diventa `FName(TEXT("SetPlaybackTracers")), FName(TEXT("SetPlaybackCues")) }));`.

Voce `AbilityActivated`: la frase `:266-267` `Il segnale visivo (ring, pulse) resta\n\t// di #2454 e non e' dichiarato qui.` diventa:

```cpp
	// clip gia' risolta dal CDO: la stessa forma di `PlayAttackMontage`. ➕ #3578: il segnale visivo (`Ring`, `Pulse`,
	// `Flash`) e' il profilo d'attivazione del sotto-progetto 4, grammatica di #2454, consegnato con `SetPlaybackCues`
	// sulla cella della sorgente (`Playback.ActivationCueAtTheSourceCell`). ⌫ *Diceva «resta di #2454».*
```

e `{ FName(TEXT("PlayCastMontage")) }));` (`:272`) diventa `{ FName(TEXT("PlayCastMontage")), FName(TEXT("SetPlaybackCues")) }));`.

- [ ] **Step 6: Build e verde**

Run: build; test `RefactorTactics.Fx+RefactorTactics.Preview+RefactorTactics.Presentation+RefactorTactics.HexMapActor`, log `<scratchpad>/t5-verde.log`. Expected: verdi, compresi `Preview.TracerDrawsWithDebugDrawingOff` e `Presentation.*` esistenti invariati (nessun test esistente pinna l'elenco delle cue di `Attack` o di `AbilityActivated`: `grep -n "SetPlaybackTracers\|PlayCastMontage" Source/RefactorTactics/Tests/RTPresentationBindingTests.cpp` lo conferma, misurato in pianificazione: solo commenti).

- [ ] **Step 7: Commit, poi mutazioni (20), (P2)**

```bash
git add Source/RefactorTactics/Turn/RTPlaybackLibrary.h Source/RefactorTactics/Turn/RTPlaybackLibrary.cpp Source/RefactorTactics/Map/RTHexMapActor.cpp Source/RefactorTactics/Turn/RTPresentationBinding.cpp Source/RefactorTactics/Tests/RTAbilityFxProfileTests.cpp Source/RefactorTactics/Tests/RTPreviewLineBatcherTests.cpp Source/RefactorTactics/Tests/RTPresentationBindingTests.cpp
git commit -F - <<'EOF'
feat(3578): il disegno delle cue FX e dello zigzag col line batcher; D-278 dichiara SetPlaybackCues per Attack e AbilityActivated

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
```

20. In `CueSegments`, `case ERTPlaybackCueKind::AreaPulse:` diventa uguale a `Ring`: sostituisci il blocco `{ const float R = …; Esagono(R); for … }` con `Esagono(FMath::Lerp(0.55f, 0.95f, A) * S);`. Test `RefactorTactics.Fx.CueStylesDifferByGeometry`: cade «🔴 Ring e AreaPulse si distinguono per geometria». Cade anche `Preview.FxCuesDrawWithDebugDrawingOff` «🔴 AreaPulse: le sue linee…» (6 invece di 12): la PR lo elenca.
P2. In `DeclaredBindings`, la voce `AbilityActivated` torna `{ FName(TEXT("PlayCastMontage")) }`. Test `RefactorTactics.Presentation.FxCueIsDeclaredForAttackAndActivation`: cade «🔴 AbilityActivated dichiara SetPlaybackCues». (`Presentation.EveryEventTypeIsCovered` resta verde: è il limite dichiarato del gate, spec §2.5.)

---

### Task 6: Lo scenario del banco — `Visual.Ability.FxProfile`

**Files:**
- Create: `Scenarios/Visual/Ability/FxProfile.json`
- Modify: `docs/technical/runbooks/scenari-validazione-visiva.md` (una riga **dopo** quella di `Visual.Ability.CastBeat`, `:216`)

**Interfaces:** nessuna di codice. Consuma il loader degli scenari (`ScenarioHarness/`), il banco Ability Lab (#3532) e `rt.Test.Scenario`. ⛔ `Scenarios/Visual/Ability/CastBeat.json` **non si tocca** (R11: è la fixture di `PIE-CAST-BEAT`, ancora senza verdetto); lo `Zigzag` si guarda in `Visual.Combat.WaterElectricCoordinated`, che lo contiene già (F1): qui non si ripete. ⛔ Nessuna `Cone`: nessuna azione la dichiara.

- [ ] **Step 1: Lo scenario, prima stesura**

`Scenarios/Visual/Ability/FxProfile.json` (line ending e codifica come `CastBeat.json`: misura con `file` prima e dopo):

```json
{
  "scenarioId": "Visual.Ability.FxProfile",
  "tags": ["vfx", "ability", "fx", "branth", "aevik"],
  "version": 1,
  "seed": 0,
  "mapRadius": 5,

  "_nota": "Il profilo FX per abilita' (#3578, spec 2026-10-08-profilo-fx-per-abilita-design.md §2.7). Branth lancia MortarShot (Area r1) sul centro (1,0,0): due nemici adiacenti dentro l'area, quindi UN AreaPulse sul centro e UNA X su ciascuna vittima, all'arrivo. Aevik attiva Overload (Area r1): un Flash di raggi inclinati sulla sua cella prima dell'onda, poi il suo AreaPulse. Il Cone non c'e': nessuna azione del catalogo lo dichiara. Lo Zigzag di LinearDischarge si guarda in Visual.Combat.WaterElectricCoordinated.",

  "_nota_geometria": "B1 in (-2,0,0) e A1 in (-2,2,0), squadra 0, fuori da entrambe le aree. N1 in (1,0,0) e N2 in (2,0,0), squadra 1. MortarShot (portata 3, Area raggio 1: RTHeroCatalogLibrary.cpp:828-831) su (1,0,0), a distanza 3 da B1: l'area r1 contiene N1 (il centro) e N2. targetCell [q, r, layer] come in Scenarios/Spec/Combat/AreaGuardFromImpactCenterIsBypassed.json:26. Overload su (1,1,0), a distanza 3 da A1 (la portata di Overload, RTHeroCatalogLibrary.cpp:405-407): l'area r1 contiene N1 e N2. Celle ed expect si fissano col run headless (Step 2).",

  "units": [
    { "id": "B1", "hero": "Hero.Branth", "team": 0, "cell": [-2, 0, 0] },
    { "id": "A1", "hero": "Hero.Aevik",  "team": 0, "cell": [-2, 2, 0] },
    { "id": "N1", "hero": "Hero.Ivrin",  "team": 1, "cell": [ 1, 0, 0] },
    { "id": "N2", "hero": "Hero.Muiren", "team": 1, "cell": [ 2, 0, 0] }
  ],

  "turns": [
    {
      "intents": [
        { "unit": "B1", "ability": "Hero.Branth.MortarShot", "targetCell": [1, 0, 0] },
        { "unit": "A1", "ability": "Hero.Aevik.Overload",    "targetCell": [1, 1, 0] }
      ]
    }
  ],

  "expect": [
    { "type": "UnitAtCell",     "unit": "B1", "cell": [-2, 0, 0] },
    { "type": "UnitAtCell",     "unit": "A1", "cell": [-2, 2, 0] },
    { "type": "TurnsCompleted", "value": 1 }
  ]
}
```

- [ ] **Step 2: Il run headless fissa celle ed `expect`**

Run: test `RefactorTactics.Scenario.EveryShippedScenarioRuns`, log `<scratchpad>/t6-scenari.log`. Verifica che lo scenario sia stato **caricato e giocato**: `grep -n "Visual.Ability.FxProfile" <scratchpad>/t6-scenari.log` deve dare almeno una riga; se il test filtra il corpus `Visual.*`, cerca il test che li esegue (`grep -n "Visual" Source/RefactorTactics/Tests/RTScenarioCorpusTests.cpp Source/RefactorTactics/Tests/RTShowcaseScenarioTests.cpp`) e lancia quello. Poi misura **dal log o dalla timeline** che: (a) MortarShot abbia colpito **due** vittime (due `Attack` con la chiave di Branth) e Overload abbia un'impronta e almeno un colpo; (b) le celle di `expect` siano quelle vere a fine turno. La portata (3) e il raggio (1) di MortarShot sono misurati in review (`Ability/RTHeroCatalogLibrary.cpp:828-831`), e la sintassi `targetCell` è quella di `Scenarios/Spec/Combat/AreaGuardFromImpactCenterIsBypassed.json:26`; il run resta necessario per i colpi e le spinte. Se un colpo non parte, o un'area tocca un alleato (`Scenarios/Combat/FriendlyFire.json`), sposta B1/A1 e riscrivi `_nota_geometria`. Aggiungi agli `expect` le celle di N1/N2 dopo eventuali spinte, **misurate** (la fixture vuole `units` **e** `expect` di cui ogni riga sia una misura). Esito finale: `Result={Success}` sullo scenario.

- [ ] **Step 3: La voce nell'indice degli scenari**

`docs/technical/runbooks/scenari-validazione-visiva.md`, dopo la riga di `Visual.Ability.CastBeat` (`:216`), nella stessa forma a quattro colonne:

```markdown
| `Visual.Ability.FxProfile` | r5 | il **profilo FX** di un'abilità ([#3578](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3578)): al primo numero del mortaio di Branth un esagono a raggi si allarga **una volta** dal centro dell'area, e una X compare su **ciascuna** delle due vittime; prima dell'onda di `Overload`, raggi inclinati sulla cella di Aevik. Lo zigzag della scarica lineare si guarda in `Visual.Combat.WaterElectricCoordinated` | scritto |
```

`file docs/technical/runbooks/scenari-validazione-visiva.md` prima e dopo: stesso line ending. `node tools/radar/doc-tables.ts --check` verde.

- [ ] **Step 4: Commit**

```bash
git add Scenarios/Visual/Ability/FxProfile.json docs/technical/runbooks/scenari-validazione-visiva.md
git commit -F - <<'EOF'
feat(3578): lo scenario Visual.Ability.FxProfile per il banco — AreaPulse, Marker e Flash a schermo

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
```

---

### Task 7: Documenti — voce PIE, seduta, nota sul tracer, capability map, conoscenza parziale, spec superate, radar

**Files:**
- Modify: `docs/technical/test-manuali-pie.md` — la cella di stato di `PIE-V01-TRACER` (`:1670`, **in coda**), una riga **in coda** alla tabella del corpus `Visual.*` (`:1896`; ultima riga oggi `PIE-CLIP-ABILITA`, `:1945`), `## Stato in numeri` (`:90-94`)
- Modify: `docs/roadmap/editor-sessions.yaml` — la seduta `U71` **dentro `sessions:`**, dopo l'ultima (`U70`, `:5138`) e prima di `not_schedulable:` (`:5159`)
- Modify: `docs/technical/architecture/capability-map.yaml` — `RT-CAP-VFX` (`:1707-1735`) e `RT-CAP-COMBAT-FEEDBACK` (`:1610-1640`)
- Modify: `docs/technical/systems/conoscenza-parziale-visibile-spec.md` — tabella di §1.3, dopo la riga «Beat di attivazione» (`:120`)
- Modify: `docs/superpowers/specs/2026-10-07-tracer-attacco-base-design.md` (§2.1 `:123-147`, §2.2 `:149`, §7 `:336-337`, §8 `:347`), `docs/superpowers/specs/2026-10-07-momento-ability-activated-design.md` (D2, `:24`), `docs/superpowers/specs/2026-10-08-profilo-fx-per-abilita-design.md` (statuto, `:3-21`)

**Interfaces:** nessuna di codice. Regole: l'esito atteso vive **solo** in `test-manuali-pie.md`; il file delle sedute cita gli **ID**, mai l'esito; nessun totale volatile; i file sono CRLF; nessuna pipe `|` dentro le celle; una nota con un numero di issue in una cella PIE va **in coda** alla cella.

- [ ] **Step 1: La nota in coda a `PIE-V01-TRACER` (F1) e la voce PIE nuova**

➕ esecuzione (Task 7). Prima di questo step `origin/main` aveva eseguito `PIE-V01-TRACER` (seduta `U68`): F1 è chiusa dal fatto. La nota in coda alla sua cella dice che quel verdetto, dato prima di #3578, vale per il tracer degli attacchi base, e che la scarica di Aevik e le cue le giudica `PIE-FX-ABILITA`; la scena (0) della voce nuova si confronta con quel verdetto. La voce nuova ha in più la scena (8), il colpo a contatto senza proiettile su `Visual.Movement.Charge` (R15: i controlli core a contatto non li porta nessuna unità). Il branch ha fuso `origin/main` (`867a4d0a3`) prima di toccare i documenti.

Prima di toccare il file: `node tools/radar/doc-coherence.ts --check` e annota il ricalcolo A1 che stampa (`<prima>`).

Cella di stato di `PIE-V01-TRACER` (`:1670`, ultima cella; in pianificazione finisce con `Spec: \`docs/superpowers/specs/2026-10-07-tracer-attacco-base-design.md\`. |`): **in coda** al testo della cella, prima del ` |` finale, aggiungi:

```markdown
 ➕ **Dipendenza da [#3578](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3578)** (profilo FX, F1): da eseguire su `main` **prima** del merge di #3578, altrimenti la domanda della scena (2) si riformula — la scarica di Aevik viaggia a zigzag (`Hero.Aevik.LinearDischarge`, override approvato); il getto di Muiren resta una linea liscia ancorata a Muiren. Le scene (1) e (3) non cambiano: la regola sugli attacchi base del roster (`BaseActionId == Action.BasicAttack`) la pinna `Playback.BasicAttackTracersEqualShapeDefault`.
```

⛔ Il glifo ⏳ in testa alla cella **resta dov'è**: la nota sta in coda (memoria di progetto: una nota in testa sposta il marcatore che A1 legge).

Riga nuova in coda alla tabella del corpus `Visual.*`, dopo `PIE-CLIP-ABILITA` (`:1945`), cinque colonne:

```markdown
| **PIE-FX-ABILITA** | Un'abilità ha il suo segno sulla mappa: attivazione sulla sorgente, colpo sulla vittima, onda sul centro ([#3578](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3578)) | Da `L_DevSandbox` con `rt.Test.Scenario <Id>` o dal banco Ability Lab ([#3532](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3532)), velocità 1×; una scena, una domanda | **(0)** Regressione: le scene (1) e (3) di `PIE-V01-TRACER` danno le stesse risposte di quella voce; la (2) si giudica riformulata — *il getto di Muiren è una linea liscia ancorata a Muiren, e la scarica di Aevik viaggia a zigzag?* **(1)** `Visual.Ability.CastBeat` — durante la Prep vedi due anelli che si stringono sulla cella di Muiren, e nessun proiettile? **(2)** `Visual.Ability.CastBeat` — quando il proiettile di Branth arriva su Muiren, compare una X sulla cella di Muiren dopo il proiettile e non prima? **(3)** `Visual.Combat.WaterElectricCoordinated` — la scarica di Aevik è una linea spezzata che cresce da Aevik fino al bersaglio, distinguibile dal getto liscio di Muiren? **(4)** `Visual.Ability.FxProfile` — al primo numero del mortaio di Branth vedi un esagono a raggi che si allarga dal centro dell'area, una sola volta, e una X su ciascuna delle due vittime? **(5)** `Visual.Ability.FxProfile` — prima dell'onda di `Overload`, sulla cella di Aevik vedi dei raggi inclinati, diversi dall'anello di un'altra attivazione? **(6)** `Visual.Combat.TracerHiddenFromUnseenAttacker` — vedi un anello, una X o un'onda su una cella che non vedi? Atteso **no**. **(7)** `Cone`: N/A — forma senza contenuto in v0.1. ⚠️ Scale e durate sono graybox (D-287): si tarano a schermo, ma restano diverse a coppie. Coperto headless: `Playback.ActivationCueAtTheSourceCell`, `Playback.ImpactCueComesAtTheArrival`, `Playback.FootprintCueOncePerFootprint`, `Fx.CueStylesDifferByGeometry`, `Privacy.HiddenSourceDeliversNoActivationCue`; qui resta ciò che nessun test vede — che i segni si **leggano** | ⏳ da eseguire — scritta il 2026-10-08 insieme al codice di [#3578](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3578); il verdetto è dell'autore |
```

Poi rilancia `node tools/radar/doc-coherence.ts --check` (`<dopo>`): il ricalcolo A1 deve dare **una voce in più, nuova e ⏳**, nessun'altra voce cambiata (la nota in coda a `PIE-V01-TRACER` non ne cambia lo stato).

La riga dei totali di `## Stato in numeri` (`:92`) **è il valore che A1 misura**: riscrivila con i numeri che il tool stampa **dopo** l'inserimento, nella stessa forma. La riga «Rimisurato» nuova va **subito sotto** la riga dei totali, sopra quella di #3563 (`:94`):

```markdown
➕ **Rimisurato il <data> ([#3578](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3578), il profilo FX per abilità): il ricalcolo di `doc-coherence` A1 dava `<prima>` **prima** di toccare il file e `<dopo>` **dopo**.** Delta **uno, nuova e ⏳** — `PIE-FX-ABILITA`: nessuna voce esistente cambia stato; la nota in coda a `PIE-V01-TRACER` è testo, non stato. Misurato sul branch `issue/3578-profilo-fx-per-abilita` e sull'albero modificato, con lo stesso comando.
```

(`<prima>` e `<dopo>` sono le due stringhe A1 misurate in questo step: esiti del passaggio corrente, ammessi da `AGENTS.md` §14.)

- [ ] **Step 2: La seduta `U71`**

Misura l'ultimo id: `grep -n "^  - id: U" docs/roadmap/editor-sessions.yaml | tail -1` (in pianificazione `U70`, `:5138`) e conta prima `not_schedulable`:

```powershell
python -c "import yaml; d=yaml.safe_load(open('docs/roadmap/editor-sessions.yaml',encoding='utf-8')); print([x['id'] for x in d['sessions']][-1], len(d['not_schedulable']))"
```

Inserisci **dopo l'ultima seduta di `sessions:` e prima della riga `not_schedulable:`** — mai in fondo al file (finirebbe in `not_schedulable:` senza errore):

```yaml
  - id: U71
    title: Il profilo FX per abilita' — attivazione, colpo e onda sulla mappa
    block: 6
    critical: false
    execution_lane: pie
    produces: >-
      il verdetto su `PIE-FX-ABILITA`; nessun asset
    artifacts: []
    unblocked_by: []
    shares_setup_with: []
    verifies: [PIE-FX-ABILITA]
    issues: [3578]
    done_when: >-
      la voce `PIE-FX-ABILITA` ha un verdetto nel registro, dato a schermo da una persona dopo il merge del codice
    notes: |
      Scritta il 2026-10-08 dal piano `docs/superpowers/plans/2026-10-08-profilo-fx-per-abilita.md`.
      L'esito atteso vive in `test-manuali-pie.md`; qui solo l'allestimento: `rt.Test.Scenario` con
      `Visual.Ability.CastBeat`, `Visual.Combat.WaterElectricCoordinated`, `Visual.Ability.FxProfile` e
      `Visual.Combat.TracerHiddenFromUnseenAttacker`, da `L_DevSandbox` o dal banco Ability Lab (#3532), a velocita' 1x.
      Le cue sono linee del line batcher: nessun asset, nessuna dipendenza dai pack gitignorati.
      La scena (0) dipende da `PIE-V01-TRACER`: se quella voce e' ancora senza verdetto, si esegue prima, su `main`.
```

Verifica: rilancia il comando python — l'ultimo id è `U71`, il secondo numero è quello di prima. `file docs/roadmap/editor-sessions.yaml` → `with CRLF line terminators`.

- [ ] **Step 3: Capability map**

`RT-CAP-VFX` (`:1707-1735`):
- `name: VFX (Niagara)` → `name: VFX (line batcher in v0.1; Niagara fuori, D-124)`
- `status: { implementation: missing, validation: missing, presentation: missing }` → `status: { implementation: partial, validation: partial, presentation: missing }` (`presentation` resta `missing` finché `PIE-FX-ABILITA` non ha un verdetto)
- `implementation: { files: [] }` → `implementation: { files: [Source/RefactorTactics/Map/RTPlaybackTracer.h, Source/RefactorTactics/Turn/RTPresentationBinding.h, Source/RefactorTactics/Turn/RTPresentationBinding.cpp, Source/RefactorTactics/Turn/RTPlaybackLibrary.h, Source/RefactorTactics/Turn/RTPlaybackLibrary.cpp, Source/RefactorTactics/Map/RTHexMapActor.cpp] }`
- `issues: { open: [288, 2453], closed: [] }` → `issues: { open: [288, 2453, 2454, 3578], closed: [] }`
- `tests: { automation: [], … }` → `tests: { automation: ["RefactorTactics.Fx.*", "RefactorTactics.Playback.*", "RefactorTactics.Privacy.*", "RefactorTactics.HexMapActor.*", "RefactorTactics.Preview.*", "RefactorTactics.Presentation.*", "RefactorTactics.Determinism.*"], scenarios: ["Visual.Ability.FxProfile"], mutation: [] }`
- `verification: { pie: [], packaged: [] }` → `verification: { pie: [PIE-V01-TRACER, PIE-FX-ABILITA], packaged: [] }`
- `owner.epic` **resta `null`** (decisione di ownership: il rischio `OWNER GAP` si restringe, non si chiude). In coda a `notes:`, una voce: `"#3578: il profilo FX per abilita' (attivazione, colpo, impronta) e' la prima implementazione, col line batcher (D-467); Niagara resta fuori (D-124). La misura di Niagara qui sopra e' storica."`

`RT-CAP-COMBAT-FEEDBACK` (`:1610-1640`): `issues: { open: [2453, 2456, 2457, 2828, 2505], … }` → aggiungi `2454, 3578` in coda a `open`; `verification: pie: []` → `pie: [PIE-V01-TRACER, PIE-FX-ABILITA]`.

`node tools/radar/doc-coherence.ts --check` verde dopo la modifica (la capability map è fra le sue sorgenti).

- [ ] **Step 4: Conoscenza parziale (§1.3)**

Prima, la premessa (spec §2.8, `NOT RUN` in pianificazione): la tabella ha ancora quattro colonne `Canale | Cosa rivelerebbe | Stato | Misura` (l'intestazione della seconda tabella di §1.3; rimisurala con `grep -n "^| Canale | Cosa rivelerebbe"`) e l'ultima riga è «Beat di attivazione» (`:120`). Se è cambiata, ci si adatta alla forma nuova. Dopo `:120`, tre righe:

```markdown
| Cue d'attivazione del profilo FX ([#3578](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3578)) | la cella della sorgente nell'istante in cui agisce | ✅ **chiuso per costruzione** | solo con sorgente osservata: la coda è filtrata a monte (`BeginPlayback`, `BuildBlastSequence`) e `URTPlaybackLibrary::ActivationCueFor` ricontrolla `SourceVerdict` (R7); `Privacy.ActivationCueNeedsTheSource`, `Privacy.HiddenSourceDeliversNoActivationCue` |
| Cue di colpo del profilo FX (`Marker`, `AreaPulse`, `ConeSweep`, #3578) | la vittima nella sua cella; il centro di un'area; l'attaccante e la direzione di un ventaglio | ✅ **chiuso per costruzione** | col verdetto della cella che disegnano: `ImpactVerdict` per il `Marker`, `FromVerdict` del colpo per le cue d'impronta (per un'area è il centro); `Privacy.ImpactMarkerNeedsTheVictim`, `Privacy.AreaPulseNeedsTheCenter` |
| Ritmo del Blast (volo del tracer, #2454 e #3578) | la classe di forma di un colpo con un id (`Single`/`Line` contro `Area`/`Cone`), mai l'override | ⚠️ **aperto e DICHIARATO** | stesso volo a parità di indice nella sequenza: il volo è la sola forma di default (R13, `Privacy.FlightDependsOnShapeNotOverride`), l'indice dipende dalle attivazioni che chi guarda vede; chiuderlo vorrebbe un volo uguale per ogni colpo (spec del profilo FX §7) |
```

- [ ] **Step 5: Le spec superate (spec §2.9) e lo statuto**

`docs/superpowers/specs/2026-10-07-tracer-attacco-base-design.md`:
- §2.1, condizione 1 e il suo ⚠️ «idoneità provvisoria e dichiarata» (`:130`): in coda al paragrafo, `⌫ Superata da #3578 (spec del profilo FX §2.2, R12, R13): il volo è la forma di default di un'azione con un id; un override toglie il disegno, mai il volo, e non può aggiungerli.`
- §2.1, la frase «rivela che un attaccante … ha usato un attacco base `Single` o `Line`»: in coda, `⌫ Allargata da #3578 a «un'azione con un id»; il ritmo è lo stesso a parità di indice nella sequenza (spec del profilo FX §2.3).`
- §2.2 (`:149`), dopo il titolo: `⌫ La tabella «Single = proiettile, Line = getto» è da #3578 la riga di default del profilo FX (spec del profilo FX §2.2); lo Zigzag è un override.`
- §7 (`:336-337`) e §8: in coda a «Per eroe …» e a «Area e Cone …», `⌫ Consegnati da #3578, come cue d'impronta e override, non come tracer.`

`docs/superpowers/specs/2026-10-07-momento-ability-activated-design.md`, D2 (`:24`): in coda alla cella, `⌫ Il segnale visivo (ring, pulse) lo consegna #3578 (profilo FX), non più #2454.`

`docs/superpowers/specs/2026-10-08-profilo-fx-per-abilita-design.md`, statuto (`:3-7`): «design **proposto**» → «design **accettato** il 2026-10-08, rivisto dal panel (`➕ rev.`, `➕ rev2.`), **implementato** in [#3578](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3578) (PR #<pr>)»; «**Non implementato.**» si toglie. In §5.1 e §6, ogni «Task N del piano» diventa «Task 4 del piano» (già fatto al Task 0 Step 2: verifica con `grep -n "Task N" docs/superpowers/specs/2026-10-08-profilo-fx-per-abilita-design.md` → 0 righe, uno zero che è il difetto). `<pr>` si scrive nel Task 8 dopo l'apertura della PR.

- [ ] **Step 6: Il radar**

L'elenco si misura, non si ricorda: `ls tools/radar/*.test.ts` dà i controlli; lancia con `--check` ciascuno che lo accetta (memoria di progetto: sono più di due). Almeno:

```powershell
node tools/radar/doc-coherence.ts --check
node tools/radar/doc-tables.ts --check
node tools/radar/doc-links.ts --check
```

Expected: nessun rosso nuovo; A3 verde (nessun glifo accanto a un nome di voce PIE in un corpo GitHub — qui si toccano solo file). Un rosso preesistente su `origin/main` si dichiara col nome del controllo, non si corregge qui.

- [ ] **Step 7: Commit**

```bash
git add docs/technical/test-manuali-pie.md docs/roadmap/editor-sessions.yaml docs/technical/architecture/capability-map.yaml docs/technical/systems/conoscenza-parziale-visibile-spec.md docs/superpowers/specs/2026-10-07-tracer-attacco-base-design.md docs/superpowers/specs/2026-10-07-momento-ability-activated-design.md docs/superpowers/specs/2026-10-08-profilo-fx-per-abilita-design.md
git commit -F - <<'EOF'
docs(3578): voce PIE-FX-ABILITA e seduta U71; nota in coda alla voce del tracer; capability map, conoscenza parziale e spec superate

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
```

---

### Task 8: Chiusura — suite, PR, review, merge, issue

**Files:** nessuno nuovo. GitHub.

- [ ] **Step 1: Suite completa sul commit finale**

`git fetch -q origin` e `git merge-base --is-ancestor origin/main HEAD`: se `origin/main` è avanzato, fondilo prima di misurare (un branch indietro falsa la misura e, al merge, cancella); se è avanzato **sullo stesso sottosistema** (playback, tracer, `RTTurnManager.cpp`), è un task d'integrazione: suite sull'albero unito e review d'insieme. Run: test con filtro `RefactorTactics`, log `<scratchpad>/t8-suite.log`. Expected: `**** TEST COMPLETE`; `Result={Fail}` = solo i rossi che il log di `origin/main` già porta (confrontali con una run su `origin/main` o col log di una suite recente sullo stesso commit di base, nominandoli). Annota lo SHA (`git rev-parse HEAD`).

- [ ] **Step 2: Push e PR verso il parent**

Il parent è `main` (`git config branch.issue/3578-profilo-fx-per-abilita.parent`). Corpo in `<scratchpad>/pr-fx.md`, `--body-file`, con:
- cosa: profilo, tabella, tracer come caso del profilo, attivazione, cue di colpo, canale, D-278, scenario;
- **l'eccezione dichiarata**: `Playback.TracerStyleFollowsShapeForBasicAttack` (`Tests/RTPlaybackLibraryTests.cpp`) è riscritto sull'asserto di `PassingBlade` — da `IsTracerEligible == false` a «idonea al volo, `TracerStyleFor == None`», più la variante a tabella iniettata — perché R13 separa volo e disegno; nessun altro test esistente è riscritto (o, se il Task 2 Step 6 ne ha trovato uno, la seconda eccezione col suo perché);
- i gate eseguiti (`PASS` col nome del log e lo SHA); il confronto prima/dopo del Task 2 Step 0/6; `Determinism.FxFieldsStayOutOfHashes` dichiarato **tripwire** (oggi nessun hash legge `ResolvedTimeline`);
- le mutazioni (1)–(22) e (P1)–(P5) con i test caduti, compresi quelli **in più** del dichiarato (➕ rev2.);
- gli esiti delle misure del Task 4 Step 0 (`ReactiveCapacitor`, `CircularTide`) con `file:riga`;
- i gate non eseguiti **in parole**: `PIE: NOT RUN` — voce `PIE-FX-ABILITA`, seduta `U71`; `PIE: NOT RUN` — voce `PIE-V01-TRACER`, da eseguire su `main` prima del merge (F1) o da giudicare riformulata; `Packaged: NOT RUN`; `Performance: NOT RUN` (graybox);
- ⛔ nessun glifo di stato accanto al nome di una voce PIE, né nel titolo né nel corpo; nessun totale volatile; chiusura `🤖 Generated with [Claude Code](https://claude.com/claude-code)`.

```bash
git push -u origin issue/3578-profilo-fx-per-abilita
gh pr create --repo DegrassiAaron/refactor-tactics-main --base main --title "Il profilo FX per abilita' (#3578)" --body-file "<scratchpad>/pr-fx.md"
```

Rileggi il corpo dal server e conta i backtick come per la issue. Poi il commit di una riga dello statuto della spec (Task 7 Step 5) col numero della PR, e push.

- [ ] **Step 3: Code review**

`/code-review` sulla PR; accogli i findings con lo skill `receiving-code-review` (verifica, non adesione). Un fix è un commit nuovo e un giro di test sullo SHA nuovo — chi ha scritto la correzione non ne firma da solo il verdetto sui sistemi che tocca (privacy, determinismo).

- [ ] **Step 4: Merge, pulizia, issue**

Prima del merge, come **ultimo commit del branch**: in `docs/technical/architecture/capability-map.yaml`, sposta `3578` da `open` a `closed` in `RT-CAP-VFX` e in `RT-CAP-COMBAT-FEEDBACK` (rimisura le righe con `grep -n`). Diventa vero nell'istante del merge, che chiude la issue. `node tools/radar/doc-coherence.ts --check` verde, poi:

```bash
git add docs/technical/architecture/capability-map.yaml
git commit -F - <<'EOF'
docs(3578): la issue passa fra le chiuse della capability map

Co-Authored-By: Claude <modello> <noreply@anthropic.com>
EOF
git push
```

Prima di premere merge: `git fetch -q origin`, e se `origin/main` è avanzato si torna allo Step 1 (cattura l'exit code della guardia **subito**, non dopo un altro comando). Merge dopo review verde. Poi `git branch -D issue/3578-profilo-fx-per-abilita`, `git remote prune origin`. Aggiorna la issue: DoD spuntato nel **commento** di chiusura con lo SHA del merge; lo stato della seduta **in parole** — `PIE: NOT RUN (PIE-FX-ABILITA, verdetto dell'autore a schermo)` — ⛔ mai un glifo accanto al nome della voce. Su #2454 un commento che dice cosa SP4 ha consegnato della sua grammatica (attivazione e colpo per forma) e cosa resta suo (`ReactionResolved`, `Defeated`).

---

## Self-review del piano (eseguito in scrittura)

**Copertura della spec, sezione per sezione.**
- §0 D1–D5 → Global Constraints (canale, line batcher), Task 1 (tabella C++ accanto a D-278, mappa approvata), Task 2 (tracer come caso), Task 3–4 (attivazione e colpo per le quattro forme).
- §1 misure → Task 1 Step 0 (`Cone` senza contenuto, nessuno `switch` su `ERTTracerStyle`), Task 2 Step 0 (test di mondo il cui ritmo cambia).
- §2.1 profilo, enum, scale, tetti, durate → Task 1 Step 3 (tipi), Task 3 Step 6 (manopole), Task 3–4 (durate e tetti), Task 5 (scale in `CueSegments`); «conseguenza dei tetti» → `Playback.FxCuesNeverOverlapInTheBlast`.
- §2.2 default, ripiego, R4, R12, R13, mappa D5, F1 → Task 1 Step 4, Task 2 Step 3, Task 7 Step 1 (nota F1).
- §2.3 `Ev.Origin` (R5, F16), finestre, R6, R7, confine di privacy, ritmo (➕ rev2.) → Task 3 Step 3–6, Task 7 Step 4.
- §2.4 `Impact` per ogni colpo, `Footprint` una volta per impronta, associazione, degrado, R14, canale R10, `PushPlaybackCues`, spegnimenti, ricalcolo estendendo → Task 3 Step 5–6, Task 4 Step 3–4.
- §2.5 D-278 → Task 5 Step 5. §2.6 seek/pausa/passo/fermata → `Playback.FxCuesAreAFunctionOfTheClock`, `Playback.CueChannelClearsOnSkip`; pausa e `Step` non hanno codice nuovo (il tick esce prima del ramo) e restano coperti dai test esistenti del tracer. §2.7 → Task 6. §2.8 → Task 7, Task 2 Step 0/6, capability map. §2.9 → Task 2 Step 3 (`RTPlaybackLibrary.h`), Task 4 Step 5 (`RTTurnManager.cpp:6374-6377`, `RTResolvedEvent.h:196-197`), Task 5 Step 5 (`RTPresentationBinding.cpp:36`, `:265-267`), Task 7 Step 5 (spec del tracer e del momento).
- §3 fuori scope → Global Constraints e issue. §4 errori e degrado → `ActivationCueDuration`/`ImpactCueDuration` (A ≤ 0, tetti), `FootprintFxForSequence` (impronta assente, sorgente 0, consumata, R14), `ImpactCueFor`/`FootprintCueFor` (verdetti, `bResolved`), `CueSegments` (asse degenere), `DisegnaLineaAnteprima` (server dedicato).
- §5.1 → ogni test elencato ha il suo step: `Fx.*` (Task 1, 4, 5), `Playback.*`/`Privacy.*` (Task 2–4), `HexMapActor.PlaybackCueIsItsOwnChannel` (Task 3), `Preview.FxCuesDrawWithDebugDrawingOff` e `Presentation.FxCueIsDeclaredForAttackAndActivation` (Task 5), `Determinism.FxFieldsStayOutOfHashes` (Task 3). §5.2 → Task 7 Step 1–2. §6 limiti → PR (Task 8 Step 2). §7 follow-up → PR e misure del Task 4 Step 0.

**Le 22 mutazioni della spec** sono tutte collocate (tabella in testa): Task 1 → (1), (2), (3), (4), (18); Task 2 → (5), (6), (17), (19), (22), (1) ripetuta; Task 3 → (7), (8), (15); Task 4 → (9), (10), (11), (12), (13), (14), (16), (21); Task 5 → (20). Più (P1)–(P5) del piano.

**Placeholder.** Nessun «TBD»; `3578`, `<pr>`, `<checkout>`, `<scratchpad>`, `<modello>`, `<prima>`/`<dopo>`, `<data>` sono variabili definite nei Global Constraints o nello step che le misura. Le celle e gli `expect` dello scenario sono una prima stesura **dichiarata**, fissata dalla misura del Task 6 Step 2 (la spec stessa lo dà `NOT RUN`).

**Tipi fra task.** `FRTAbilityFxProfile`, gli enum, `FRTPlaybackCue`/`ERTPlaybackCueKind` (Task 1) hanno gli stessi nomi di campo nei Task 2–5; `FxProfileFor(FName, FName, ERTAbilityShape)` e `FxProfileForIn(TMap, FName, FName, ERTAbilityShape)` hanno la stessa firma ovunque; `BlastActivationCuesAt` (Task 3) e `BlastHitCuesAt` (Task 4) sono due funzioni con firme definitive, e `PushPlaybackCues(ERTMatchPhase)` guadagna nel Task 4 una chiamata, non un parametro; `PlaybackBlastFootprintFx` è `TArray<int32>` parallelo a `PlaybackBlastSequence`; `PlaybackPhaseElapsedForTest()` (Task 2) è usato dai Task 2–4.

**Review Focus.** (a) Task 3 `Privacy.HiddenSourceDeliversNoActivationCue` (+ (8) sulla funzione pura); (b) Task 4 `Playback.UnresolvedGeometryHasNoFxButPlaysTheClip` + (P4); (c) Task 4 `Playback.FxCuesAreAFunctionOfTheClock` + (14); (d) Task 4 `Privacy.AreaPulseNeedsTheCenter` + (P3); (e) Task 1 `Fx.BaseActionOverrideWinsOverShapeDefault` + (3).

**Deviazioni dichiarate (`➕ piano.`).** La premessa F15 di `Playback.ActivationCueAtTheSourceCell` (`Ev.Origin != Src->Cell`) si misura sulla carica di `Ram` nel Dash, non sullo Scudo, che non si sposta (la spec ammette di estendere la fixture finché la premessa vale); tipi della cue nel Task 1; canale di mappa nel Task 3; `PushPlaybackCues(Phase)`; manopole senza `meta` (le vicine reali non lo hanno); `TracerStyleForIn` per la variante iniettata; `Fx.NoActionNoProfile` nasce nel Task 1 e guadagna nel Task 2 l'asserto di (22); `Playback.FxCuesAreAFunctionOfTheClock` nel Task 4 (il codice che prova nasce lì) e non nel Task 5 del brief; nomi dei test dalla spec, non dal brief.
