# Terminali CODE / EDITOR e lane DIR-C — spec panel su tre mandati residui

> `CURRENT` · **Stato**: revisione chiusa. I tre sorgenti sono **consumati e rimossi**, non archiviati —
> §15. Questo referto è l'unico documento che ne conserva il contenuto ·
> **Data**: 2026-08-29
> **Base di misura**: `bbf0d780` (`origin/main` dopo `git fetch --prune`). Il checkout usato è a `4ca09bcd`,
> 6 commit indietro: nessuna misura presa lì, tutto letto con `git show origin/main:<path>` — §14.
> **Oggetto**: tre mandati che stavano alla radice di `refactor-tactics-technical-designer`, non versionati:
>
> | # | File | Righe | Cos'è |
> |---|---|---:|---|
> | **1** | `CLAUDE_REFACTORTACTICS_TACTICAL_DESIGNER_CODE.md` | 617 | il terminale **C++/runtime/test**, roadmap `TD-CODE-00`…`09` |
> | **2** | `CLAUDE_REFACTORTACTICS_TACTICAL_DESIGNER_EDITOR.md` | 911 | il terminale **Unreal/UMG/Graybox/PIE**, roadmap `TD-EDITOR-00`…`08` |
> | **3** | `REFACTORTACTICS — DIR-C · QA - SCENARIO - BOT - AUTOBATTLE v0.1.md` | 733 | la lane **QA/Scenario/Bot/Autobattle** di uno schema a tre worktree |
>
> Letti contro `Source/RefactorTactics/`, `Source/RefactorTacticsEditor/`, gli 88 scenari di `Scenarios/`,
> `Content/RT/`, `.gitignore`, i 1357 Automation Test, ADR-0010, `contratto-wbp-scenario-composer.md`,
> `test-manuali-pie.md`, `editor-sessions.yaml`, l'epic #1105 e le issue/PR/branch vivi.
> **Panel**: Wiegers (lead) · Fowler · Cockburn · Nygard · Crispin · Adzic
> **Modo**: critique
> **Referti gemelli dello stesso giorno**:
> [`td-trial-scenario-sandbox-spec-panel-2026-08-29.md`](td-trial-scenario-sandbox-spec-panel-2026-08-29.md) ·
> [`tactical-designer-devsandbox-launcher-spec-panel-2026-08-29.md`](tactical-designer-devsandbox-launcher-spec-panel-2026-08-29.md)

---

## 1. Il verdetto in una riga

Non sono tre proposte. Sono **due mandati già in esecuzione** — il repository ne porta il vocabolario in tre
documenti owner, e quattro delle dieci slice `TD-CODE` sono issue **chiuse** — **più una lane che descrive
un'organizzazione che non esiste**, la cui priorità 1 è un difetto chiuso con undici guardiani e quattro
branch vivi addosso.

| | Voci |
|---|---:|
| 🔴 Critico | **3** |
| 🟠 Alto | **4** |
| 🟡 Medio | **6** |
| ➕ Trovato misurando, non è dei sorgenti | **3** |

**Raccomandazione operativa**: **non rieseguire nessuno dei tre.** Il lavoro che descrivono ha già owner —
issue, ADR, contratti, sedute — e ciò che resta davvero da fare è **una riga**, non tre roadmap: aprire la
seduta d'editor che `PIE-SCEN-COMPOSER` aspetta da un giorno e che nessuno rivendica (§4). Le tabelle §12 e
§13 dicono dove è finito ogni pezzo dei tre mandati, perché è l'unica cosa che serve conservarne.

⚠️ Nessuna suite eseguita, nessuna build, nessun file di codice o documento owner toccato, nessuna scrittura
su GitHub. Issue, PR e branch letti lato server con `gh` il 2026-08-29.

---

## 2. Baseline misurata

I tre mandati elencano insieme **diciotto** file di preflight. Ne esistono **diciassette**.

| File citato | Esito |
|---|---|
| `AGENTS.md` · `CLAUDE.md` | ✅ |
| `docs/product/piano-canonico-mvp.md` | ✅ |
| `docs/decisions/RT_PDR_00_Decision_Log.md` | ✅ |
| `docs/DOC_CONFLICT_MATRIX.md` · `docs/OPEN_DECISIONS.md` | ✅ |
| `docs/roadmap/roadmap-checkpoint.md` · `roadmap-v0.1.md` · `v0.1-definition-of-done.md` | ✅ |
| `docs/roadmap/execution-graph.yaml` | ✅ |
| **`docs/roadmap/parallel-batch.yaml`** | ⛔ **non esiste** — `M1` |
| `docs/technical/tooling/spec-tactical-designer.md` · `convenzioni-contenuti-ue.md` · `asset-map.md` | ✅ |
| `docs/technical/systems/spec-graybox-placement-contract.md` | ✅ |
| `docs/technical/architecture/spec-asset-pipeline.md` | ✅ |
| `scripts/rt-suite.ps1` | ✅ |

E le strutture di codice che i tre danno per esistenti ci sono tutte: `Source/RefactorTactics/Bot/`,
`ScenarioHarness/`, `Tests/`, `FRTTestScenario`, `URTScenarioLoader`, `URTScenarioRunner`,
`FRTScenarioIntent`, `FRTScenarioVariant`. Va detto: è una base di partenza accurata, e il difetto dei tre
documenti non è che sbaglino il repository — è che **descrivono come lavoro futuro cose già consegnate**.

---

## 3. 🔴 C1 — i due terminali sono già in esecuzione, e il repository ne porta il vocabolario

`TD-CODE-*` e `TD-EDITOR-*` non sono etichette locali di un prompt: sono **dentro tre documenti owner**.

| Dove | Cosa dice |
|---|---|
| [ADR-0010](../../decisions/adr-0010-esposizione-blueprint-scenario-harness.md), intestazione | *«`CANONICAL` · **Stato**: Accettato — contratto implementato, operazioni di editing da implementare (**TD-CODE-02**)»* |
| ADR-0010, riga 103 | *«…con **TD-CODE-02** — aggiungere, spostare e togliere unità»* |
| [`contratto-wbp-scenario-composer.md`](../../technical/tooling/contratto-wbp-scenario-composer.md) | il **titolo** è *«Contratto — Widget Scenario Composer (**TD-EDITOR-01**)»*, e cita `TD-EDITOR-00` e `TD-EDITOR-03` |
| [`test-manuali-pie.md`](../../technical/test-manuali-pie.md) | `PIE-SCEN-COMPOSER` è *«il percorso PIE del DoD di **TD-EDITOR-01**»* |

E quattro delle dieci slice `TD-CODE` sono **issue chiuse**, che i mandati stessi nominano:

| Slice | Issue nominata dal mandato | Stato misurato |
|---|---|---|
| `TD-CODE-01` writer scenario | #1114 | **CLOSED** |
| `TD-CODE-02` initial state API | #1115 | **CLOSED** |
| `TD-CODE-03` Move/Wait | #1116 | **CLOSED** |
| `TD-CODE-04` Run/Reset/Result/TurnLog | #1117 | **CLOSED** |

**Wiegers**: un mandato che ha già prodotto documenti owner non è più una proposta — è una **sorgente il cui
consumo è iniziato**, e riaprirlo dall'inizio non produce il lavoro descritto: produce un secondo fronte
sullo stesso. Il difetto non è nei mandati, che si difendono bene (*«Riferimento iniziale: #1114, **se ancora
valido**»*, *«Non assumere che issue vecchie descrivano ancora `main`»*): è che chi li rilegge oggi non ha
modo di sapere, dai file, che quella clausola è già scattata quattro volte.

**Adzic**: e la porta che il §TD-CODE-04 chiede di esporre è misurabile in una riga. `URTScenarioAuthoring`
porta oggi **35 `UFUNCTION`**, e coprono il ciclo intero: `CreateScenarioDraft` · `NewScenario` · `OpenById` ·
`OpenFromFile` · `Close` · `Validate` · `SaveToFile` · `SaveInPlace` · `AddUnit` · `MoveUnit` · `RemoveUnit` ·
`SetUnitFacing` · `AddTurn` · `RemoveTurn` · `SetWaitIntent` · `RemoveIntent` · `AddExpectationUnitAtCell` ·
`ListIntents` · `ListExpectations` · `GetReachableCells` · **`Run`** · **`Reset`** · `GetLastRunReport` ·
`GetLastRunLog` · `GetSummary` · `ListHeroIds`.

Il gate `SYNC C4` — *«Place → Intent → Run → TurnLog → Reset → Modify → Run»* — è la **definizione di quel
blocco di funzioni**, ed è passato.

---

## 4. 🔴 C2 — `TD-EDITOR-01` ha uno stop misurato, dichiarato nel registro, e i mandati non lo sanno

È il difetto più utile del referto, perché è il solo che indica un'azione.

Il DoD di `TD-EDITOR-01` — *«piazza almeno due unità, salva, riapri, verifica Cell/Facing/ID»* — ha già la sua
voce nel registro delle verifiche interattive: **`PIE-SCEN-COMPOSER`**. E quella voce dichiara di sé, misurato
il 2026-08-28 e **ancora vero a `bbf0d780`**:

- ⛔ *«**Non soddisfacibile oggi**: l'asset non esiste — `git ls-files 'Content/RT/UI/**/WBP_RT_ScenarioComposer*'`
  dà **0**»*. Rimisurato: **0**.
- ⚠️ *«**E nessuna seduta la rivendica**: `grep -i composer docs/roadmap/editor-sessions.yaml` dà **0**, e
  altrettanto per `td-editor`»*. Rimisurato: **0**. Le sedute `U25`–`U30` coprono graybox, griglia di lavoro
  e frontend.
- 🔴 *«Finché quella riga non esiste, questa voce è **nel registro e fuori da ogni sequenza**»* — la
  condizione che [`spec-tactical-designer.md`](../../technical/tooling/spec-tactical-designer.md) §9 descrive
  come *«tende a non essere mai eseguita»*.

**Cockburn**: la lane Editor **non è bloccata sul C++**. `C1`–`C4` sono passati, la facade espone tutto,
`ADR-0010` ha dato la porta. È bloccata su due cose che nessun mandato nomina: **un widget che nessuno ha
creato** e **una seduta che nessuno ha aperto**. Il `TD-EDITOR-00` («Baseline + Graybox Audit») è già stato
fatto — il contratto §6 lo cita: *«L'audit TD-EDITOR-00 ha misurato che la presentazione graybox **esiste
già** ed è procedurale […] **Nessuna mesh nuova serve per TD-EDITOR-01**»*.

⚠️ **E c'è una domanda aperta che va decisa prima, non durante**: il contratto §6 tiene vivi **due candidati**
per la cartella del widget, ed è la ragione per cui l'oracolo di `PIE-SCEN-COMPOSER` cerca il **nome** e non
la cartella — *«un glob su `UI/Scenario/` continuerebbe a dare `0` anche se l'asset nascesse in `UI/Tools/`,
dichiarando assente una cosa che esiste»*.

### Correzione
L'unica azione che i tre mandati giustificano oggi: **aprire una seduta in
[`editor-sessions.yaml`](../editor-sessions.yaml)** che rivendichi `PIE-SCEN-COMPOSER` e decidere la
collocazione del widget. Massima seduta misurata: **U30**. Tutto il resto dei due terminali è consegnato,
bloccato su dato che non esiste (§12), o già assegnato a una issue aperta.

---

## 5. 🔴 C3 — DIR-C: la priorità 1 è chiusa da undici guardiani, e l'area ha quattro branch vivi

Il §6 di DIR-C dichiara *«un difetto storicamente vivo da riverificare»*:

```text
match reale/default → 12 round → 0 combat → draw
```

Misurato: **#1088 è `CLOSED`** — *«I bot non attaccano mai: 12 round, zero Combat, pareggio — e dal 2026-08-17
è il comportamento predefinito»*. E l'istruttoria che il §6 chiede — *«misura almeno: candidate generation,
target legality, LOS, path cost, range, attack score, tie-break…»* — **è stata fatta e lasciata nel repository
come test**:

```text
Bot.StalemateProbeAttackCandidateIsGeneratedAndScored
Bot.StalemateProbeContendersAreNamed
Bot.StalemateProbeFiringPositionsExist
Bot.StalemateProbeNoPerceivedEnemyMeansStandStill
Bot.StalemateProbeHeadlessMatchLosesContact
Bot.StalemateProbeDemoArenaHasMoreFiringCells
Bot.StalemateProbeAuthoredMapClosesCyclesOfSomePeriod
Bot.StalemateProbeAuthoredMapFilterStepsBack
Bot.StalemateProbeGeneratedArenaClosesNoCycleOfAnyPeriod
Bot.StalemateProbeGeneratedArenaFilterNeverStepsBack
Bot.StalemateBreaksWithTeamPlanning
Match.Autobattle.NobodyParksOnTheAuthoredMap
Match.Autobattle.EngagesOnTheAuthoredMap
Match.Autobattle.NobodyOscillatesOnTheAuthoredMap
```

**Nygard**: e il difetto **residuo** è più fine di quello che il mandato descrive. Aperte oggi:

- **#1551** — *«I due oracoli di parcheggio rispondono in modo opposto alla stessa domanda, e nessuno dei due
  lo sa»*
- **#1602** — *«Il margine dell'anti-parcheggio è zero e nessuno sa cosa lo produce»*

con **quattro branch remoti vivi** — `test/1550-orbita-geometria-e-parcheggio`,
`test/1551-misura-avanzamento`, `test/1602-margine-anti-parcheggio`, `test/1603-periodo-tre` — e la **PR
#1645** aperta (*«la (c) di `BOT-STALL-1` misurata, e su queste board non compra niente»*).

Chi entra con l'inquadramento del mandato misura un sintomo **che non si riproduce più**, non trova nulla, e
non vede quello aperto — che non è *«il bot non attacca»* ma *«due oracoli si contraddicono e il margine è
zero»*. Ed entrandoci **collide con quattro lavori in corso**.

✅ **Il divieto del §6 resta la riga migliore del documento**: *«NON correggerlo con
`if (TurnsWithoutCombat > N) ForceAttack();` — devi prima trovare la causa»*. È esattamente la disciplina che
ha prodotto le dieci sonde.

---

## 6. 🟠 A1 — «CODE non crea mesh graybox» è falsificato da `D-229`, e la ragione è scritta

Il §4 del mandato CODE stabilisce la divisione:

> *«Il Graybox Kit appartiene al terminale Editor. CODE non crea: Static Mesh graybox; Material graybox;
> Blueprint visuali graybox.»*

Misurato, le mesh graybox sono **generate da un commandlet C++**, nel modulo Editor:
[`RTBuildGrayboxMeshesCommandlet`](../../../Source/RefactorTacticsEditor/Private/Content/RTBuildGrayboxMeshesCommandlet.h),
che le costruisce e le **salva** come `.uasset`. Il perché è nel suo header:

> *«Perché **generate e non modellate** — è `D-229`: i budget di forma sono FRAZIONI (contratto graybox §6.3),
> e **una funzione che le applica si diffa e si testa mentre un binario autorato no**.»*

Sette asset sono già in repository e sono esattamente il `GB-03`/`GB-04` che il mandato EDITOR dice di creare
just-in-time:

```text
Content/RT/World/Graybox/Cover/SM_Graybox_Cover_High.uasset · _Low.uasset
Content/RT/World/Graybox/Doors/SM_Graybox_Door_Panel.uasset · _Locked.uasset
Content/RT/World/Graybox/Surfaces/SM_Graybox_Surface_Water.uasset · _Ice.uasset
Content/RT/World/Graybox/Volumes/BP_Graybox_CellPlacementVolume.uasset
```

**Fowler**: la divisione che i due mandati tracciano è **per tecnologia** — C++ di qua, Editor di là. Quella
del repository è **per autorità**: chi decide la regola contro chi la mostra. `D-229` dimostra che le due non
coincidono — una mesh i cui vertici vengono da `URTHexLibrary` è C++ **ed è presentazione**, e metterla dal
lato «Editor» perché è una mesh la toglierebbe dal diff e dal test, che è precisamente il costo che `D-229`
ha deciso di non pagare. Stessa forma per `RTBuildIconCatalogCommandlet`, che importa texture e crea il
`DA_IconCatalog`.

✅ **Da tenere, e vale per entrambi i mandati**: *«La mesh NON decide movement / cover / standability / path /
LOS / door state / targeting / damage»*. È il contratto di placement, citato correttamente, e il mandato
EDITOR rimanda al documento giusto — `spec-graybox-placement-contract.md`, che esiste, con la tassonomia
`CellBound` / `EdgeBound` / `SurfaceBound` / `EditorOnly` che nomina.

---

## 7. 🟠 A2 — il preflight LFS poggia su una premessa che il repository ha rifiutato, e un owner doc la conferma

Il §0 del mandato CODE prescrive `git lfs version` e, se serve, `git lfs install`. Misurato:

| | Esito |
|---|---|
| `.gitattributes` | **non esiste** |
| `git lfs ls-files` | **vuoto** |
| `.gitignore:43` | *«Asset UE binari: NON versionati (**niente Git LFS**, evita costi/repo pesante)»* |

⛔ **E il ciclo si chiude su un documento owner.**
[`spec-asset-pipeline.md`](../../technical/architecture/spec-asset-pipeline.md) §9 scrive:
*«**Git LFS**: mesh/anim/audio/texture sono binari → `.gitattributes` deve tracciarli (`.uasset`/`.umap`
**già in LFS**, roadmap CP 0.3). Verificare `git lfs ls-files` dopo l'import.»*

Quel documento è **nella lista di preflight del mandato EDITOR** (§0, voce 6). Chi esegue i due mandati in
ordine legge la prescrizione, la verifica sul documento owner che gliela conferma, e non incontra mai la riga
di `.gitignore` che dice il contrario. È `P3` al §11.

**Nygard**: e la politica reale è più esigente di quella che il mandato EDITOR immagina. I binari sotto
`Content/` sono **esclusi per default**, con riammissioni per **ruolo**:

```text
Content/**/*.uasset  *.umap  *.fbx  *.png ...        escluso
  !Content/RT/UI/**/*.uasset                          riammesso
  !Content/RT/Maps/**/*.umap  *.uasset                riammesso
    Content/RT/Maps/**/_Scratch/                      riescluso
  Content/RT/UI/_Generated/                           escluso (derivati)
  /Content/FabAsset/  /Content/Paragon*/              esclusi come directory
  + otto negazioni per singolo file
```

Il `DoD` §11 del mandato EDITOR chiede *«git status contiene solo binari attesi»*. Per un asset creato fuori
dai path riammessi quel controllo legge **vuoto**, e `.gitignore` scrive perché il fallimento è silenzioso:
*«dimenticare la riga **non produce nessun segnale** — l'asset semplicemente non entra nel repository, e te ne
accorgi il giorno in cui qualcun altro apre la mappa e non la trova»*.

### Correzione
Togliere il preflight LFS; sostituire il `DoD` *«git status contiene solo binari attesi»* con
*«l'asset è sotto un path riammesso da `.gitignore`, verificato con `git check-ignore -v <path>`»* — che è
l'unico controllo che distingue «non l'ho toccato» da «l'ho creato e sta sparendo».

---

## 8. 🟠 A3 — «Non usare worktree» e «Sei DIR-C, il worktree» stavano nella stessa cartella

Il mandato CODE §0 chiude con *«Non usare worktree»*, e prescrive che CODE ed EDITOR condividano
**una sola working directory e un solo branch**, con *«un solo writer alla volta»*. Il mandato DIR-C si apre
con *«Sei **DIR-C**, il **worktree** dedicato a…»* dentro uno schema a tre (`DIR-A` / `DIR-B` / `DIR-C`).

Nessuno dei due enuncia il vincolo che morde davvero, e che il repository ha già misurato e pagato:

- **`D-222`** accetta il parallelismo come regime — *«101 checkout e 6 sessioni in un giorno»* — e ciò che
  protegge **non è la directory, è la MISURA**: una suite vale solo se `HEAD`, l'albero, il binario e i
  processi del motore sono gli stessi all'inizio e alla fine; altrimenti non è rossa né verde, è **NON
  VALIDA**.
- **Il mutex del motore è globale sull'eseguibile**: due run di automation si uccidono anche da checkout
  diversi. Un worktree parallelizza l'*editing*, mai la *suite*.
- **`scripts/rt-suite.ps1`** esiste, legge le quattro invarianti prima e dopo e confronta `Test Completed`
  con `Found N`. È PowerShell e non Bash, perché MSYS traduce i path.

**Wiegers**: i protocolli `SYNC REQUEST` / `SYNC READY` dei due terminali sono ben costruiti — dichiarano
branch, `HEAD`, working tree, file previsti, binari coinvolti, gate d'uscita. ⚠️ Ma **non dichiarano il
binario del motore né i processi**, che sono due delle quattro invarianti di `D-222`: un handoff che passa
l'ownership con una build stantia consegna una misura non valida senza che nessuno dei due lati possa
accorgersene. *(Ed è il difetto che il repository ha già visto: una suite che non vede un binario vecchio
dichiara valido ciò che misura codice inesistente.)*

### Correzione
Sostituire entrambe le regole — *«non usare worktree»* e *«sei un worktree»* — con quella vera: **il
parallelismo è ammesso sull'editing e vietato sulla suite**, e i due blocchi `SYNC` acquistano due righe,
`Binario del motore:` e `Processi UnrealEditor attivi:`.

---

## 9. 🟠 A4 — «19 `UFUNCTION`» è scaduto in due documenti diversi, e nello stesso modo

[`contratto-wbp-scenario-composer.md`](../../technical/tooling/contratto-wbp-scenario-composer.md) §2 —
intitolato *«La superficie disponibile — **verificata, non ricordata**»* — scrive:

> *«Rimisurato il 2026-08-28: `grep -rln "UFUNCTION" ScenarioHarness/` dà un file, e `grep -c UFUNCTION` su
> quel file dà **19**.»*

Rimisurato oggi sullo stesso file: **35**.

Ed è la **seconda** occorrenza dello stesso numero fermo. `ADR-0010` scrive *«`URTReplayViewerSubsystem`
espone **diciannove** `UFUNCTION`»*, e il referto gemello di stamattina ne ha misurate **36**.

**Crispin**: il contratto fa la cosa giusta — dichiara la data **e** il comando, e si intitola «verificata,
non ricordata». Il difetto non è il numero: è che un documento che dichiara di essere una misura viene letto
come uno stato il giorno dopo. Dove il numero non porta la tesi — e in entrambi i casi non la porta, la tesi
è *«c'è una porta sola»* — si scrive il **comando** al posto del numero, e chi legge misura.

---

## 10. 🟡 Medi

| | Punto | Misura |
|---|---|---|
| **M1** | DIR-C §3 prescrive di leggere `docs/roadmap/parallel-batch.yaml`, e §15 di non modificarlo | ⛔ **non esiste**. È l'unico dei diciotto file di preflight assente (§2). `execution-graph.yaml`, citato accanto, esiste. Un file nominato due volte — una per leggerlo, una per proteggerlo — e presente zero volte |
| **M2** | Entrambi i terminali dichiarano **UE 5.8.1** come *«baseline atteso, da riconfermare»* | `RefactorTactics.uproject` dichiara `"EngineAssociation": "5.8"`, e il runbook PIE dice **5.8** in `D:\EpicGames\UE_5.8`. Il `.1` non ha sorgente nel repository. La forma della clausola («da riconfermare») è giusta: la riconferma dà `5.8` |
| **M3** | DIR-C §13 dubita dei nomi degli eroi: *«un handoff recente indicava Gadget, Phase, Riktor, Wraith ma il repository corrente è l'autorità»* | ✅ **il dubbio si risolve a favore**: `ARTGameMode` dichiara `Team0Heroes = { Hero.Gadget, Hero.Phase }` e `Team1Heroes = { Hero.Riktor, Hero.Wraith }`. Va detto perché il dubbio era gratuito e corretto, e in questo archivio è la disciplina che manca più spesso |
| **M4** | DIR-C §14 chiede di *«mantenere/estendere canary equivalenti a `HiddenEnemyFairness`, `KnowledgeEquivalentPlanning`, `NoHiddenStateInBotContext`»* | ✅ **esistono tutti e tre**: `HexBotPlay.HiddenEnemyFairness` (nome esatto), `HexBotPlay.PlansOnPartialKnowledge`, `Knowledge.ViewIsIndependentOfHiddenState` — più `Knowledge.ViewOmitsHidden`, `Bot.DecidesWithoutFutureKnowledge`, `Vision.TeamKnowledgeIsUnion`, `Reactions.WindowViewHiddenFromEnemyTeam`. Il §14 è **soddisfatto**: «mantenere» è il verbo giusto e non c'è niente da aprire |
| **M5** | DIR-C §12 chiede che *«il playback speed NON cambi snapshot / resolver / decision / outcome / hash»* | ✅ **è già un test**, e col nome che lo dice: `Match.Autobattle.DeterminismIsIndependentOfPlayback`. Accanto: `DeterminismSurvivesUnitPermutation`, `HexSim.ReplayDivergenceZero`, `Debug.VerifyReplayDetectsDivergence`, `Movement.StepperIsDeterministicUnderPermutation` |
| **M6** | DIR-C §9 chiede scenari `Reaction_Fire` · `Reaction_Hold` · `Reaction_Timeout` | La copertura esiste **con altri nomi**: `Overwatch.FireTruncatesFutureMovement`, `Overwatch.HoldKeepsArmed`, `Overwatch.HoldResumesSameMovementState`, `Overwatch.SecondFireOnDownedTargetLogsNoDamage`, più `Scenarios/Spec/Reaction/` e `Scenarios/Visual/Reaction/`. ⚠️ Aprire scenari con i nomi del mandato **duplicherebbe**: il §7 dello stesso documento lo vieta (*«NON creare un duplicato tipo `COMPLETE_MATCH_NEW` se uno scenario owner esiste già»*), e la regola vale anche per i suoi |

---

## 11. ➕ Trovato misurando

Tre difetti del **repository**, non dei mandati. Emersi verificandone le premesse.

### P1 — `DA_Format_Scratch.uasset` è orfano, e due documenti dicono ancora che non lo è

`.gitignore` riammette esplicitamente questo asset e ne spiega il ruolo:

> *«➕ **Riammesso il 2026-08-18 per #623.** `L_DevSandbox` lo referenzia dal commit `7db11313` (#937) ma non
> era mai entrato nel repository: chiunque apra la sandbox riceve un `LoadErrors`. ⚠️ Il nome inganna: NON è
> un formato di partita, è un `URTHexMapAsset` con **le celle vere della sandbox** (20583 byte) — mentre
> `DA_HexMap_Sandbox.uasset` qui sopra è versionato ma **VUOTO** (1416 byte, zero celle).»*

Misurato oggi sulla tabella dei riferimenti di `L_DevSandbox.umap`, il livello **non lo nomina più**. Nomina:

```text
/Game/RT/Core/Grid/M_HexCell
/Game/RT/Maps/Dev/_Scratch/DA_HexMap_Scratch_Basin      ← non DA_Format_Scratch
/Game/RT/UI/Framework/WBP_RT_ErrorModal
/Game/Maps/L_Prototype                                   ← metadato noto, #1280
```

Lo scambio è **databile**, leggendo i quattro commit che hanno toccato la `.umap`:

| Commit | Asset mappa referenziato |
|---|---|
| `7db11313` (#937) | `L_DevSandbox/Data/DA_Format_Scratch` |
| `8c4bd70e` (#623) | `L_DevSandbox/Data/DA_Format_Scratch` |
| `21f4042f` (#956, *«la fixture RelayBasin entra nel repository, in deroga dichiarata a `.gitignore`»*) | **`_Scratch/DA_HexMap_Scratch_Basin`** |
| `bbe11833` (corrente) | `_Scratch/DA_HexMap_Scratch_Basin` |

Ne seguono tre cose:

1. Il commento di `.gitignore` — *«`L_DevSandbox` lo referenzia dal commit `7db11313`»* — era **vero quando è
   stato scritto** ed è **falso da `21f4042f`**.
2. `tools/asset-refs/check.ts`, cioè **lo strumento che esiste per controllare i riferimenti fra asset**,
   porta la stessa lista scaduta nel proprio `KNOWN_EXCEPTIONS`: *«Il Reference Viewer non lo mostra fra le
   dipendenze del livello (materiale, **DA_Format_Scratch**, `WBP_RT_ErrorModal` e nient'altro)»*.
3. ⚠️ **L'asset da cui la sandbox ora dipende sta sotto `Content/RT/Maps/**/_Scratch/`**, che `.gitignore`
   esclude come directory — resta nel repository solo perché era già tracciato — ed è quello che
   `GenerateFixtureIntoAsset` **sovrascrive a ogni rigenerazione** (`RTHexMapTests.cpp`). Mentre 20 KB di
   celle autorate, riammesse apposta, non li usa nessuno.

> **Metodo, dichiarato**: misurato estraendo le stringhe stampabili dalla tabella dei riferimenti del
> package, non aprendo il livello. Prova che il nome **non compare nel package**; a quale property
> dell'`ARTHexMapActor` sia assegnato si legge solo in Editor.

**Azione**: una issue. Non è lavoro dei tre mandati e non va nel loro gate — ma va aperta prima che qualcuno
rigeneri la fixture e la sandbox cambi mappa senza che nulla lo dica.

### P2 — l'intestazione di ADR-0010 descrive uno stato di due giorni fa

`CANONICAL`, riga 3: *«Stato: Accettato — contratto implementato, **operazioni di editing da implementare**
(TD-CODE-02)»*. Misurato: `AddUnit`, `MoveUnit`, `RemoveUnit`, `SetUnitFacing` sono sulla facade, con
`OutError` e i codici d'esito; e **#1115, #1116, #1117 — le tre issue che l'ADR dichiara di abilitare — sono
`CLOSED`**. Una riga, e sta nell'intestazione di un documento `CANONICAL`, cioè il primo punto in cui un
lettore cerca lo stato.

### P3 — `spec-asset-pipeline.md` §9 dichiara gli asset in LFS, contro `.gitignore`

Vedi `A2`. È la stessa classe di P1 e P2: un documento owner che **conferma una premessa sbagliata** a chi
segue la lista di preflight di un mandato.

---

## 12. Dove è finito ogni pezzo — terminali CODE ed EDITOR

Ciò che vale la pena conservare dei due file, ora che non esistono più.

| Slice | Owner misurato | Stato |
|---|---|---|
| `TD-CODE-00` baseline | — | ✅ fatta: la superficie è in `contratto-wbp-scenario-composer.md` §2 |
| `TD-CODE-01` writer JSON | **#1114** | ✅ **CLOSED** |
| `TD-CODE-02` initial state API | **#1115** · ADR-0010 | ✅ **CLOSED** — `AddUnit`/`MoveUnit`/`RemoveUnit`/`SetUnitFacing` (`P2`) |
| `TD-CODE-03` Move/Wait | **#1116** | ✅ **CLOSED** — più `GetReachableCells` per path/reachable |
| `TD-CODE-04` Run/Reset/Result/TurnLog | **#1117** | ✅ **CLOSED** — guardiano `Scenario.RunFromTheEditorMatchesTheHeadlessRun` |
| `TD-CODE-05` Variant / Skill Test Lite | `FRTScenarioVariant` | ⏸️ **bloccata sul dato**: la variante varia **solo le celle**, e l'override di abilità *«davvero non c'è»* (#1105). Innesco: Skill Workbench, `TD 0.3` |
| `TD-CODE-06` ability intent + targeting query | #1626 (`T2`) | 🔄 aperta. Le sonde LOS/targeting sono **dichiarate post-Trial** in #1105, non aperte |
| `TD-CODE-07` corpus combat | `Scenarios/` — **88** scenari | 🟡 in gran parte esistente. Il mandato dice *«prima cerca equivalenti»*: è la clausola giusta |
| `TD-CODE-08` Baseline↔Variant diff | **#1630** (`T7` State Diff) | 🔄 aperta |
| `TD-CODE-09` *stop building the tool* | #1105, «post-Trial» | ✅ già la politica dell'epic |
| `TD-EDITOR-00` baseline + graybox audit | `contratto-...` §6 | ✅ fatta: *«nessuna mesh nuova serve per TD-EDITOR-01»* |
| `TD-EDITOR-01` graybox unit + initial state UI | `PIE-SCEN-COMPOSER` | 🔴 **il collo di bottiglia** — widget assente, nessuna seduta (`C2`) |
| `TD-EDITOR-02` move + intent UI | #1626 · #1627 | 🔄 dietro `TD-EDITOR-01` |
| `TD-EDITOR-03` Run/Reset/TurnLog | #1625 (`T1`) · #1630 | 🔄 dietro `TD-EDITOR-01` |
| `TD-EDITOR-04` Skill Test Lite | — | ⏸️ post-Trial come `TD-CODE-05` |
| `TD-EDITOR-05` targeting preview | #1626 | 🔄 |
| `TD-EDITOR-06` combat scenario lab | `Scenarios/` | 🟡 come `TD-CODE-07` |
| `TD-EDITOR-07` diff UI | **#1630** | 🔄 |
| `TD-EDITOR-08` *stop e usa il tool* | #1105 gate d'uscita | ✅ è **lo stesso testo**, in sostanza, del gate d'uscita della TD Trial |

**Il `GB-01`…`GB-07` del mandato EDITOR** non va perso: `GB-03` (cover/porte) e `GB-04` (superfici) sono
**consegnati** nei sette asset di `Content/RT/World/Graybox/`; `GB-02` (unit placeholder) esiste come
presentazione procedurale secondo `contratto-...` §6; `GB-05`/`GB-06`/`GB-07` (targeting shapes, path,
effect marker) sono la **resa graybox degli eventi** che #1625 nomina come *«ciò che resta davvero da
costruire»*.

---

## 13. Dove è finito ogni pezzo — DIR-C

| Sezione | Owner misurato | Stato |
|---|---|---|
| §5 bot v0.1 scope | `Source/RefactorTactics/Bot/` · **~60** test `Bot.*`/`HexBot.*` | ✅ esistente |
| §6 autobattle stall | **#1088 CLOSED** · 14 guardiani · **#1551**, **#1602** aperte | 🔴 **inquadramento superato** (`C3`) |
| §7 complete match | `Scenarios/AutoBattle/*` (5) · `freeRun` + `MaxTurns` col divieto di `Pass` sul tetto | 🟡 esistente |
| §8 showcase | `Scenarios/RT_Showcase_Relay_v01.json` · **12** test `ShowcaseRelay.*` | ✅ esiste, e il *«non assumere invariato»* è la clausola giusta |
| §9 reaction FIRE/HOLD/timeout | `Overwatch.*` · `Spec/Reaction/` · `Visual/Reaction/` | 🟡 coperto con **altri nomi** (`M6`) |
| §10 objective | `Scenarios/AutoBattle/Objective.json` · `Spec/Objective/PointSurvivesKO.json` | 🟡 esistente |
| §11 determinism corpus | `HexSim.ReplayDivergenceZero` · `RepeatCount` · `bExpectSameAcrossVariants` · golden | ✅ esistente (`M5`) |
| §12 autobattle = pipeline normale | `Match.Autobattle.DeterminismIsIndependentOfPlayback` | ✅ **già un test** |
| §14 fairness | tre canary su tre | ✅ **soddisfatto** (`M4`) |
| §16 test che richiedono Editor | `test-manuali-pie.md` + `editor-sessions.yaml` | ✅ il meccanismo esiste, e la regola è *«la voce la crea la PR che implementa»* |

⛔ **Ciò che di DIR-C non ha controparte**: l'intera organizzazione `DIR-A` / `DIR-B` / `DIR-C`, i write-set
per directory, il protocollo di consegna fra lane (§15), il `parallel-batch.yaml` (`M1`) e il formato
`DIR-C HANDOFF` del §19. Il repository coordina **misure**, non directory: un difetto trovato fuori
ownership diventa **una issue**, non una consegna a una lane. È la stessa conclusione a cui era arrivato il
referto di `DIR-A` il 2026-08-28.

---

## 14. Come è stata protetta la misura

`D-222`: il checkout usato — `D:\Repositories\refactor-tactics-technical-designer\refactor-tactics-main` —
era pulito su `main` a `4ca09bcd`, **6 commit indietro** rispetto a `origin/main` (`bbf0d780`) dopo
`git fetch --prune`. Nessun checkout, nessun `pull`, nessuna scrittura sull'albero prima delle misure: tutto
letto con `git show origin/main:<path>`, che non tocca la working directory, non muove il binario e non
sfiora il mutex del motore. Le sole scritture di questa passata sono al §15.

---

## 15. Cose non fatte, e la sorte dei tre sorgenti

- ⛔ **I tre file sono stati RIMOSSI, non archiviati**, su richiesta esplicita — a differenza del handoff del
  DevSandbox Launcher, archiviato lo stesso giorno in
  [`../../archive/src/handoff/2026-08-29-tactical-designer-devsandbox-launcher.md`](../../archive/src/handoff/2026-08-29-tactical-designer-devsandbox-launcher.md).
  ⚠️ **Questo referto è quindi l'unico documento che ne conserva il contenuto**, ed è la ragione per cui le
  §12 e §13 mappano ogni singola slice invece di riassumerle: chi cercherà `TD-CODE-02` o `TD-EDITOR-01`
  dopo averli letti in ADR-0010, nel contratto del widget o in `PIE-SCEN-COMPOSER` non troverà i sorgenti, e
  deve poter trovare qui che cos'erano.
- ⛔ **Nessuna issue creata.** `P1` (l'asset orfano), `P2` (l'intestazione di ADR-0010) e `P3` (LFS nello
  `spec-asset-pipeline`) sono tre correzioni piccole e indipendenti, e non sono lavoro dei mandati: vanno
  aperte come tali, non infilate in un gate.
- ⛔ **Nessuna seduta aperta in `editor-sessions.yaml`**, che è l'unica azione che il §4 raccomanda. Richiede
  di scegliere fra i due candidati di collocazione del widget, e quella scelta è dell'autore.
- ⛔ **Nessun `D-nnn` assegnato**: niente qui richiede una decisione nuova. L'ultimo misurato su
  `origin/main` è **D-238**, da riverificare sui ref remoti prima di qualunque merge (CLAUDE.md §7) — ci
  sono **11** branch remoti vivi e **4** PR aperte.
- ⛔ **Nessun documento owner aggiornato**, incluse le quattro righe scadute che questo referto misura
  (`.gitignore`, `check.ts`, ADR-0010, `spec-asset-pipeline.md`).
- ⛔ **Nessun codice, nessuna build, nessuna suite.**
- ⏸️ **Non misurato**: se `Reaction_Timeout` abbia una controparte — `DecisionOnTimeout` esiste come
  comportamento di ripiego nel formato, ma non ho cercato il test che lo copre.
- ⏸️ **Non misurato**: quale property dell'`ARTHexMapActor` di `L_DevSandbox` porti
  `DA_HexMap_Scratch_Basin` (`P1`). Si legge aprendo il livello.
