# TD Skill / Action Lab — roadmap `TD 0.1 → TD 1.0` — spec panel

> `CURRENT` · **Referto di revisione**, non owner. Consuma
> [`../../archive/src/handoff/2026-08-31-td-skill-actionlab-roadmap-prompt.md`](../../archive/src/handoff/2026-08-31-td-skill-actionlab-roadmap-prompt.md)
> (arrivato in radice come `Prompt_Claude_TD_Skill_ActionLab_Roadmap_v0.1_to_v1.0.md`, **untracked**, 923 righe, 15 sezioni).
>
> **Data**: 2026-08-31 · **Base**: `feat/bot-ally-planning` @ `0c0ee87c`, **0 avanti / 0 dietro**
> `origin/main` = `0c0ee87c` dopo `git fetch --all --prune` · **Modo**: critique ·
> **Focus**: requirements + architecture + testing
>
> **Cosa è**: il verdetto su un mandato di consolidamento che chiede di mappare una capability
> *Skill + Action Lab + Scenario* su una scala `TD 0.1 → TD 1.0` e di aprire dieci issue candidate.
> Il referto giudica il mandato e **non esegue il lavoro che ordina**: nessuna issue creata, chiusa o
> modificata, nessun documento owner riscritto, nessun `.uasset` aperto.
>
> **Cosa non è**: un'autorità. Se una riga qui diverge da
> [`../../technical/tooling/spec-tactical-designer.md`](../../technical/tooling/spec-tactical-designer.md),
> dal corpo di [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105) o dal
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md), **ha ragione l'owner**.
>
> ⚠️ **`HEAD` si è mosso durante questa revisione.** All'apertura della sessione il ramo era a `16b295ae`;
> alla misura successiva era a `0c0ee87c`, mossa da un'altra sessione, con un `UnrealEditor.exe` vivo
> (4,7 GB). Tutte le misure qui sotto sono state **rifatte** su `0c0ee87c`, non riportate dalla prima lettura.

---

## 1. Il verdetto in una riga

> **Il mandato è metodologicamente ineccepibile e materialmente arretrato: impone «REUSE / UPDATE prima di
> CREATE», e applicando quella sua stessa regola le sue dieci issue candidate danno CREATE = 0 — sei sono
> già consegnate, tre hanno una issue aperta che le porta, una è esplicitamente post-Trial.**

Il kit non sbaglia architettura: il §2 riscrive correttamente l'invariante che il repository ha già
(`ADR-0010`, l'editor chiama la facade e non possiede il resolver). Sbaglia **tre identificatori** e **una
scala numerica**, e ciascuno dei quattro, eseguito alla lettera, produrrebbe un danno misurabile:

1. la sua *Golden Skill* si chiama `Flux.ArcPulse`, un token **ritirato senza redirect** da `D-134`;
2. la sua *presentation source* — «Gadget — Plasma Blast» — attribuisce a Gadget un'abilità **di un altro
   eroe Paragon**;
3. il suo *owner documentale* punta a un path che non esiste, e chiede a quell'owner di portare **stato**,
   che il banner in testa a quel file dichiara essere un difetto;
4. la sua scala `TD 0.1 … TD 1.0` **esiste già**, decisa da `D-154`, e la sua `TD 0.1` **significa un'altra
   cosa** rispetto alla `TD 0.1` dell'owner.

✅ **Ciò che il kit vale davvero non è nella sua lista di titoli.** La domanda che pone — *«quale singola
azione ha un percorso completo catalogo → resolver → scenario → TurnLog → automation → presentation?»* — è
buona, ed è rispondendole che è emerso un difetto vero che nessuna issue teneva: **tre documenti owner
dichiarano un'attesa di `98` HP per uno scenario che ne asserisce `103`** (§8).

---

## 2. Base di misura

Tutto ciò che segue è **misurato** su `0c0ee87c`, stato issue letto lato server con `gh`.

```text
git fetch --all --prune
git rev-parse HEAD                      -> 0c0ee87c
git rev-parse origin/main               -> 0c0ee87c
git rev-list --count origin/main..HEAD  -> 0
git rev-list --count HEAD..origin/main  -> 0
```

| Misura | Comando | Esito |
|---|---|---|
| `Flux.` in `Source/` | `rg -c "Flux\." Source` | **0 occorrenze, 0 file** |
| `Hero.Gadget.ArcPulse` negli scenari | `rg -c "Hero\.Gadget\.ArcPulse" Scenarios` | **34 occorrenze in 21 file** |
| Test di run/reset | `rg -c IMPLEMENT_SIMPLE_AUTOMATION_TEST Source/RefactorTactics/Tests/RTScenarioRunResetTests.cpp` | **10** |
| Abilità Paragon di Gadget | `ls Content/FabAsset/Paragon/ParagonGadget/FX/Abilities` | `ElectroGate` · `Primary` · `RollingBot` · `StickyBomb` · `Ultimate` · `VisionBot` |
| Clip di animazione Gadget | `find …/Heroes/Gadget/Animations -maxdepth 1 -type d` | solo `AimOffsets/` e `Blendspaces/` |

Stato issue citate dal kit, letto il 2026-08-31:

| Issue | Stato | Titolo (troncato) |
|---:|---|---|
| #1105 | **OPEN** | `[EPIC] Tactical Designer — un solo loop fra mappa, skill e scenario` |
| #1114 · #1115 · #1116 · #1117 | **CLOSED** | Scenario Composer Lite — writer, piazzamento, authoring turni, **Run/Reset/Result/TurnLog** |
| #1625 … #1630 | **OPEN** | TD Trial `T1`, `T2`, `T3`, `T4`, `T5`, `T7` |
| #711 | **CLOSED** (2026-08-31) | Sonda di movimento nell'editor |
| #695 · #622 · #1186 | **OPEN** | residui `TD 0.1` |
| #1657 | **CLOSED** | `DA_Format_Scratch` orfano |
| #1678 | **OPEN** | DevSandbox Launcher: entrare nel workflow da `L_DevSandbox` |
| #1663 | **OPEN** | «gli eroi non hanno animazioni: 756 asset Paragon cotti, **zero clip**» |
| #1753 · #1754 · #1755 | **CLOSED** | Scenario Bootstrap · Tactical View · LOS Inspector |
| #776 | **OPEN** | `E43` — misura a lotti (owner di `TD 0.7`) |

---

## 3. Il panel

### 🔨 WIEGERS — qualità dei requisiti

❌ **CRITICO — un requisito normativo costruito su un identificatore ritirato.** Il §3 e il TD01-E fissano
`Flux.ArcPulse` come contratto. `D-130` rinomina il token in `Hero.Gadget.ArcPulse`; `D-134` **cancella il
redirect** (`URTCatalogLibrary::ResolveLegacyActionId` non esiste più) perché non aveva lettori. Misura:
`Flux.` ha **zero** occorrenze in `Source/`; le uniche superstiti nel repository sono commenti che
*dichiarano la rinomina* (`tools/radar/parse-catalog.ts:48,275,282`) e la riga di mappatura in
`piano-migrazione-roster.md:96`. Un'acceptance scritta su quel token non è ambigua: è **falsificabile e
falsa**. <!-- rename-exempt: la riga dichiara la rinomina del token: sostituirla la renderebbe muta -->
📝 **Raccomandazione**: `Hero.Gadget.ArcPulse` ovunque, e il nome scenario `SCN_Gadget_ArcPulse_Baseline`
cade con esso (§5, TD01-F).

⚠️ **MAGGIORE — il kit vieta a sé stesso ciò che poi fa.** Il §3 scrive *«non rinominare Stable ID in questo
task senza migration owner»* e nella riga sopra usa il nome pre-rinomina. La regola è giusta; applicata,
avrebbe fermato il kit all'ingresso.

⚠️ **MAGGIORE — la DoD del §7 è già verde per metà, e il kit non lo sa.** Dei dodici passi, i punti 7-9 e 11
sono coperti da `#1117` e dal test `RefactorTactics.Scenario.RunFromTheEditorMatchesTheHeadlessRun`
(`RTScenarioRunResetTests.cpp:91`). Una DoD che non distingue *«da fare»* da *«fatto»* non misura niente.

### ⚔️ FOWLER — confini e nomi

⚠️ **MAGGIORE — «Action Lab» è il quarto nome di una cosa che ne ha già uno.** Il giro che il kit descrive —
*apri → scegli mappa/eroe/skill/target → PLAY → TurnLog → PLAY → stesso hash* — è la **TD Trial / Scenario
Sandbox**, definita al §6.1 dell'owner e decomposta in `T0`–`T8` nel corpo di `#1105`. Il repository ha già
respinto questa forma due volte: `D-154` rifiuta *«una epic per ogni pannello»*, e il consolidamento *Skill
Plus* ha registrato un `CONFLICT` proprio su un nome nuovo per un concetto con owner.

❌ **CRITICO — `candidate` è un nome occupato, e la decisione che lo dice è la stessa che fonda questo
lavoro.** `D-154` misura cinque header di `Source/RefactorTactics` che usano già `Candidate` con **due**
significati (la mossa valutata dal bot; ciò che il raycast trova sotto il cursore) e conclude: lo Skill
Workbench userà **`Variant`**, che è già il vocabolario dello scenario (`FRTScenarioVariant`). Il kit usa
`candidate` in `TD 0.3`, `TD 0.4`, `TD 0.9` e in tre acceptance.
📝 **Raccomandazione**: sostituzione meccanica `candidate → variant` prima di qualunque uso del kit.

✅ **Il §2 è corretto e non aggiunge nulla.** «Mai un resolver/targeting/pathfinder d'editor» è
`ADR-0010` più il §3 dell'owner. Nessun errore, nessun contenuto nuovo.

### 🕸️ NYGARD — stato, determinismo, modi di fallimento

❌ **CRITICO — TD01-C descrive una capability consegnata.** «Reset deterministico, seed fisso, StateHash
repeat» è `#1117`, ed è protetta da **dieci** test in `RTScenarioRunResetTests.cpp`, fra cui
`FRTScenarioResetReturnsToTheDeclaredStateTest` (riga 228),
`FRTScenarioRunNeverShowsAStaleReportTest` (526) e
`FRTScenarioRunReportDoesNotSurviveAScenarioChangeTest` (578) — che è esattamente il *«nessun residuo del run
precedente»* della sua acceptance. Lo `StateHash` non è un concetto da introdurre: è
`Source/RefactorTactics/Turn/RTMatchStateHash.{h,cpp}`, consumato da `RTScenarioRunner.cpp`,
`RTScenarioSession.cpp` e `RTScenarioDraft.{h,cpp}`.

⚠️ **Il corpo di `#1105` dichiara `T0` consegnato *senza issue*** — la parità editor/headless su esito,
turni, passed/failed e **StateHash** è già un test che gira. TD01-I chiede di costruire quel test.

### 🧪 CRISPIN — testabilità e corpus

❌ **CRITICO — TD01-F chiede di creare uno scenario che esiste.** `Scenarios/Combat/BasicAttack.json`:
`scenarioId: Combat.BasicAttack`, `seed: 0`, `mapRadius: 4`, Gadget `A1` contro Riktor `B1` a distanza 2,
intent `Hero.Gadget.ArcPulse`, aspettative dure (`UnitHpEquals` ×2, `UnitAlive`, `TurnsCompleted`). Il kit
premette *«Creare SOLO se non esiste scenario equivalente»*: la clausola scatta.

⚠️ **TD01-G è per metà già scritto e per metà dichiarato post-Trial.** La parte LOS ha il suo complemento
canonico in `Scenarios/Combat/BlockedByWall.json` — *«stesso attacco, ma con un muro: la linea di tiro è
bloccata»* — che è precisamente il *«non testare solo che fa danno»* del kit. La parte **copertura** è
elencata fra i *«post-Trial»* nel corpo di `#1105` («sonde LOS, copertura, legalità di bersaglio»: owner
diversi da `#711`, **non aperte**). Il kit stesso autorizza il defer: *«se l'owner dice post-slice, defer e
registra il motivo»*. Registrato.

📊 **Copertura reale della Golden Skill**: `Hero.Gadget.ArcPulse` compare **34 volte in 21 scenari** —
percezione, facing, guard/brace, copertura, HUD, reazioni. Non è una skill da promuovere ad *anchor*: **è
già** l'anchor del corpus.

### 💬 DOUMONT — dove va lo stato

⚠️ **MAGGIORE — il §9 indirizza un file che non esiste e gli chiede la cosa vietata.** Path del kit:
`docs/technical/spec-tactical-designer.md`. Path reale:
[`docs/technical/tooling/spec-tactical-designer.md`](../../technical/tooling/spec-tactical-designer.md). E
il banner in testa a quel file dice: *«Non è un tracker. […] Se una riga di questo file dichiara uno stato,
è un difetto»*. Lo stato vive in **due** posti dichiarati: le issue sotto `#1105` e il checkpoint **M9.4**
di [`../roadmap-checkpoint.md`](../roadmap-checkpoint.md).

---

## 4. La scala `TD 0.x` esiste, e la `TD 0.1` del kit non è la `TD 0.1` dell'owner

Il §4 del kit propone dieci stadi. Nove su dieci coincidono, nel significato, con la tabella del **§6**
dell'owner e con quella del corpo di `#1105` — entrambe decise da `D-154` il 2026-08-17, entrambe già
mappate su **M9.4**. Il decimo è il primo, e la divergenza non è cosmetica:

| | Owner (`§6` + `#1105`) | Kit |
|---|---|---|
| **`TD 0.1`** | aprire una mappa canonica, **disegnarla**, caricare ed eseguire uno scenario, vedere perché un ordine è invalido — 🟡 residui `#622`, `#695`, `#1186` | *Golden Skill Execution & Scenario Validation Foundation* |
| `TD 0.2` | authoring scenario senza JSON — ✅ **consegnato** `#1114`→`#1117` | *Visual Scenario Authoring for Skill Tests* |
| `TD 0.3` … `TD 1.0` | Skill Workbench · binding variante/scenari · explainability · replay→scenario · confronto a lotti (**E43**, `#776`) · matrice di regressione · promozione · ambiente completo | stessi otto, stessi significati |

🔴 **Perché non si può «integrare» rinumerando**: `TD 0.1` è citata come *stadio con residui* in tre luoghi
che non si aggiornano insieme — la tabella del §6, la sezione *«Il lavoro aperto oggi»* di `#1105`, e la
riga `M9.1` di `roadmap-checkpoint.md`. Riassegnare quel numero a *«Golden Skill Execution»* renderebbe
**silenziosamente false** tre righe che nessun gate legge. È la stessa classe di difetto delle sedici
collisioni di contatore `D-nnn` registrate nel Decision Log: un identificatore condiviso riassegnato senza
misurare chi lo cita.

✅ **La forma corretta esiste già ed è quella del §6.1**: ciò che il kit chiama `TD 0.1` non è uno *stadio*,
è un **taglio trasversale** — e quel taglio ha un nome, una definizione e otto slice con le loro issue.

---

## 5. Matrice delle issue candidate — l'output obbligatorio del §11

Applicato il criterio del kit: *cerca l'equivalente semantico → OPEN = UPDATE/LINK · CLOSED con capability
presente = REUSE · CREATE solo per gap reale.*

| Candidate | Issue esistente | Stato | Owner | Azione |
|---|---:|---|---|---|
| **TD01-A** Launcher Scenario / Action Lab | **#1678** | OPEN | `#1105` · §4.1 dell'owner | **UPDATE / LINK** — il launcher da `L_DevSandbox` è già la sua materia; il kit aggiunge solo il selettore di modalità |
| **TD01-B** Esecuzione canonica via Harness | **#1117** | CLOSED | `ADR-0010` · `URTScenarioAuthoring` | **REUSE** — la facade, il draft e il TurnLog esistono |
| **TD01-C** Reset deterministico, seed, StateHash | **#1117** + `T0` | CLOSED | `RTScenarioRunResetTests.cpp` (10 test) · `RTMatchStateHash` | **REUSE** |
| **TD01-D** Controlli Target / Context | **#1626** · **#1629** | OPEN | `T2` intent · `T5` status iniziali | **LINK** — sono le due metà del suo «minimo utile»; il resto è già nel formato o dichiarato assente |
| **TD01-E** Golden Skill come contratto | — | — | `ADR-0007` · `RT_HeroCatalog_v0.1.md:87` | **REUSE, con ID corretto** — `Hero.Gadget.ArcPulse`, 22 danni, range 4; nessun `if Gadget` da evitare perché nessuno esiste |
| **TD01-F** Scenario baseline | `Scenarios/Combat/BasicAttack.json` | esiste | `scenario-map.md` | **NON CREARE** — clausola del kit soddisfatta |
| **TD01-G** Scenario tattico range/cover | `Combat.BlockedByWall` (LOS) | esiste / post-Trial | `#1105` §post-Trial | **DEFER** — la metà copertura è dichiarata non aperta dall'owner |
| **TD01-H** Save as Scenario Draft | **#1114** + gate d'uscita Trial | CLOSED | writer canonico + round-trip Stable ID | **REUSE** |
| **TD01-I** TurnLog, automation, parità | `T0` | consegnato | `RefactorTactics.Scenario.RunFromTheEditorMatchesTheHeadlessRun` | **REUSE** |
| **TD01-J** Audit presentation Paragon | **#1663** | OPEN | asset di terze parti | **LINK** — l'audit è già fatto e l'esito è `NONE`: *«756 asset Paragon cotti, zero clip»* |

**CREATE = 0.**

### 5.1 Le issue non create, nel formato richiesto dal §11

```text
Candidate:  TD01-B · TD01-C · TD01-H · TD01-I
Covered by: #1117 (Run/Reset/Result/TurnLog dal Scenario Harness reale), #1114 (writer e round-trip),
            slice T0 del corpo di #1105 (parita' editor/headless con StateHash, senza issue perche' e' un test)
Reason:     capability consegnata e protetta da automation; una issue nuova ridescriverebbe codice mergiato

Candidate:  TD01-A · TD01-D · TD01-J
Covered by: #1678 · (#1626 + #1629) · #1663
Reason:     issue OPEN semanticamente equivalenti; il contributo del kit e' materiale per il loro corpo,
            non per una issue nuova

Candidate:  TD01-E · TD01-F
Covered by: ADR-0007 + RT_HeroCatalog_v0.1.md + Scenarios/Combat/BasicAttack.json (34 occorrenze
            di Hero.Gadget.ArcPulse in 21 scenari)
Reason:     il contratto end-to-end esiste; cio' che manca non e' uno scenario, e' la coerenza dei suoi
            derivati documentali — vedi §8

Candidate:  TD01-G
Covered by: Combat.BlockedByWall per la meta' LOS; la meta' copertura e' nell'elenco «post-Trial» di #1105
Reason:     defer autorizzato dal kit stesso, motivo registrato qui
```

---

## 6. I titoli finali — l'output obbligatorio del §12

| Stage | Titolo candidato del kit | Owner reale dopo l'audit |
|---|---|---|
| `TD 0.1` | Golden Skill Execution & Scenario Validation Foundation | ⛔ **non recepito come stadio**: è la **TD Trial / Scenario Sandbox**, §6.1 dell'owner + sezione omonima di `#1105`. Lo stadio `TD 0.1` resta *«mappa canonica, disegno, caricamento/esecuzione, ordine invalido»* |
| `TD 0.2` | Visual Scenario Authoring for Skill Tests | §6 dell'owner, `TD 0.2` — ✅ consegnato `#1114`→`#1117` |
| `TD 0.3` | Skill Workbench MVP | §6, `TD 0.3` — ⬜ nessun owner, e il vincolo è dichiarato: *«prima esiste il dato che la UI compila»*, l'override di abilità in una variante non c'è |
| `TD 0.4` | Integrated Skill + Scenario Experiment Loop | §6, `TD 0.4` |
| `TD 0.5` | Skill Explainability & Tactical Probes | §6, `TD 0.5` — 🟡 `#711` chiusa (movimento); LOS/targeting non aperti |
| `TD 0.6` | Replay-to-Scenario Skill Authoring | §6, `TD 0.6` |
| `TD 0.7` | Batch Skill Comparison & Balance Analytics | §6, `TD 0.7` — resta di **E43** (`#776`), *«non si duplica qui»* |
| `TD 0.8` | Skill Regression Matrix & Impact Analysis | §6, `TD 0.8` |
| `TD 0.9` | Production Skill Promotion & Governance | §6, `TD 0.9` — l'unico stadio con rischio di produzione |
| `TD 1.0` | Production Skill Design & Validation Environment | §6, `TD 1.0` + la DoD verificabile in sei punti già scritta sotto la tabella |

**Nessun `E48`, nessuna epic nuova, nessun `docs/roadmap/skill-lab-roadmap-new.md`** — le tre cose che il
kit stesso vieta, e che l'audit conferma non servire.

---

## 7. La presentation: due errori di fatto in tre righe

Il §3 e il TD01-J fissano *«Paragon presentation source: Gadget — Plasma Blast»*.

🔴 **`Plasma Blast` non è un'abilità di Gadget.** `Content/FabAsset/Paragon/ParagonGadget/FX/Abilities/`
contiene sei cartelle: `ElectroGate`, `Primary`, `RollingBot`, `StickyBomb`, `Ultimate`, `VisionBot`. Il
nome che il kit cerca appartiene a un altro eroe del pack — `Content/FabAsset/Paragon/ParagonTwinblast/`
esiste come pacchetto separato, e le uniche occorrenze di quel nome sotto `ParagonGadget/` sono battute
vocali **su** di lui (`Gadget_Lore_Kill_Twinblast`). L'attacco base di Gadget, lato Paragon, si chiama
**`Primary`** (`Gadget_Effort_Ability_Primary_Fire`).

🔴 **L'audit che il kit ordina è già stato fatto, e l'esito è `NONE`.** `#1663` è OPEN e si intitola *«Nel
pacchetto gli eroi non hanno animazioni: 756 asset Paragon cotti, zero clip»*. Misura indipendente su
`ParagonGadget`: sotto `Characters/Heroes/Gadget/Animations/` esistono solo `AimOffsets/` e `Blendspaces/`,
nessuna cartella di `AnimSequence`/`AnimMontage`.

⚠️ Il kit chiude quella sezione con *«non fare affidamento su nomi asset inventati»*. È la sua riga migliore
e la sua premessa la viola.

---

## 8. Il difetto che il kit ha fatto trovare senza cercarlo

Chiedendo *«qual è il contratto end-to-end della Golden Skill»* si finisce sull'unico scenario che asserisce
un **danno**, e lì i numeri non tornano fra loro:

| Fonte | Dichiara |
|---|---|
| `Scenarios/Combat/BasicAttack.json:24` | `UnitHpEquals B1 = 103` |
| `Scenarios/Combat/BasicAttack.json` (`_nota`) | «Riktor parte da 120 HP e **ne perde 22**» → 98 |
| `docs/technical/runbooks/test-e-diagnosi.md:542` | «Gadget colpisce Riktor: **120 → 98 HP**» |
| `docs/technical/runbooks/scenari-validazione-visiva.md:171` | «`UnitHpEquals B1 **98**`» |
| `docs/characters/v0.1/gadget.md:188` | «`Combat.BasicAttack` — **120 − 22 = 98** su Riktor» |

Lo scarto è **5**, ed è il `BaseShield` di [**D-224**](../../decisions/RT_PDR_00_Decision_Log.md): *«ogni
unità porta 5 punti di scudo BASE […] e quello scudo ferma solo il danno DIRETTO»* → `22 − 5 = 17`,
`120 − 17 = 103`. Il dato eseguibile si è mosso con la decisione; **quattro righe di prosa in tre documenti
owner più la nota interna dello scenario stesso** sono rimaste al valore precedente.

> 🔵 **Eseguito il 2026-08-31, e l'inferenza ha retto — ma il difetto era più grande di così.** Questo blocco
> diceva `NOT RUN`, e la riserva era giusta: senza run il `103` restava una deduzione da `D-224`.
> `./scripts/rt-suite.ps1 -Filter RefactorTactics.Scenario.EveryShippedScenarioRuns` → **VALIDA**, `1/1`,
> `0 fallimenti`, *«corpus eseguito: 84 PASS, 11 BLOCKED, 1 expected-fail»*, e `Combat.BasicAttack` **non è
> fra i BLOCKED**. Il log mette i due numeri in righe consecutive:
>
> ```text
> LogRT: [RT] Colpo: RTUnit_0 -> RTUnit_1 (22)
> LogRT: [RT] (q=-1,r=0,L=0) -> (q=1,r=0,L=0): 17 danni (Action.BasicAttack · Hero.Gadget.ArcPulse, p50)
> ```
>
> Il `22` che i documenti citano è reale — è il danno **dichiarato** del colpo, non la differenza di HP.
>
> ➕ **E cercando le gemelle ne sono uscite altre due**, che questo referto non aveva viste perché guardava
> un solo scenario: `Visual.Movement.Charge` (prosa `70`, asserisce **`75`** — stessa causa) e
> `Visual.Environment.FireOnEnter` (prosa `82`, asserisce **`72`** — causa diversa: la sua stessa cella
> vicina scrive *«10 danni all'ingresso, 8 nel Cleanup»*, cioè `90 − 18 = 72`). Tre righe, **tre** ragioni.
> Aperta [#1904](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1904), che le tiene tutte.

➕ **Difetto minore, stessa famiglia**: il corpo di `#1105` elenca `#1753`, `#1754`, `#1755` come `[ ]`
aperte; tutte e tre risultano **CLOSED**.

---

## 9. Report finale nel formato richiesto dal §14

```text
TD SKILL / ACTION LAB ROADMAP CONSOLIDATION REPORT

Repository:  DegrassiAaron/refactor-tactics-main
Branch:      feat/bot-ally-planning
HEAD:        0c0ee87c
origin/main: 0c0ee87c   (0 avanti / 0 dietro)
Audit date:  2026-08-31

A. EXISTING CAPABILITIES
   - Facade d'authoring e ViewModel: URTScenarioAuthoring + FRTScenarioDraft (ADR-0010)
   - Esecuzione canonica via Scenario Harness, Reset, Result, TurnLog          (#1117)
   - StateHash runtime: RTMatchStateHash, consumato da Runner/Session/Draft
   - Parita' editor/headless: RefactorTactics.Scenario.RunFromTheEditorMatchesTheHeadlessRun
   - Writer canonico e round-trip con Stable ID                                (#1114)
   - Golden Skill a catalogo: Hero.Gadget.ArcPulse, 22 danni / range 4 (ADR-0007)
   - Corpus: 34 occorrenze in 21 scenari, baseline Combat.BasicAttack + BlockedByWall
   - Launcher d'ingresso: SRTLauncherScenarioPanel, RTLauncherScenarioBrowser,
     RTDevSandboxLauncherSubsystem
   - Sonde runtime di movimento nell'editor                                    (#711, chiusa)
   - Viewport: bootstrap stato iniziale, tactical view, LOS inspector   (#1753/#1754/#1755)

B. PARTIAL CAPABILITIES
   - Launcher come entry point unico con selezione di modalita'                (#1678, OPEN)
   - Authoring degli intent di combattimento e degli status iniziali    (#1626, #1629, OPEN)
   - Playback visuale, multi-turno, FIRE/HOLD, State Diff    (#1625, #1627, #1628, #1630, OPEN)
   - Explainability: solo movimento; LOS/targeting/cover non aperti

C. MISSING CAPABILITIES
   - Skill Workbench (TD 0.3): manca il DATO, non la UI — nessun override di abilita'
     in una variante. E' il vincolo dichiarato dal corpo di #1105.
   - Conversione replay -> scenario editabile (TD 0.6)
   - Clip di animazione per gli eroi: zero                                     (#1663, OPEN)

D. ROADMAP TD 0.1 -> TD 1.0
   Vedi §6. Nessuno stadio nuovo: la scala esiste al §6 dell'owner e nel corpo di #1105,
   decisa da D-154, con checkpoint M9.4. La "TD 0.1" del kit e' la TD Trial, §6.1.

E. ISSUE AUDIT TD 0.1
   Vedi §5. CREATE 0 · REUSE 6 · LINK/UPDATE 3 · DEFER 1.

F. ISSUES CREATED
   nessuna.

G. ISSUES UPDATED
   nessuna. Questo referto e' documentale: nessuna scrittura su GitHub.

H. ISSUES NOT CREATED
   Vedi §5.1 — tutte e dieci, con la ragione per ciascuna.

I. ROADMAP / DOCS UPDATED
   docs/roadmap/plans/td-skill-actionlab-roadmap-spec-panel-2026-08-31.md   | referto  | CREATE
   docs/archive/src/handoff/2026-08-31-td-skill-actionlab-roadmap-prompt.md | sorgente | ARCHIVE
   docs/archive/src/README.md                                              | tabella handoff/ | +1 riga
   Nessun documento owner modificato.

J. SCENARIOS
   Combat.BasicAttack   | REUSE, nessuna modifica | e' gia' la baseline che TD01-F chiede
   Combat.BlockedByWall | REUSE, nessuna modifica | e' la meta' LOS di TD01-G
   Nessuno scenario creato.

K. TESTS
   PASS:    RefactorTactics.Scenario.EveryShippedScenarioRuns — VALIDA, 1/1, 0 fallimenti,
            durata 02:05, HEAD b498afec / albero 400d185b invariati fra prima e dopo.
            Corpus: 84 PASS, 11 BLOCKED, 1 expected-fail. Combat.BasicAttack fra i PASS
            (non compare nell'elenco dei BLOCKED). Log: Saved/Logs/rt-basicattack.log.
            ⚠️ Base dichiarata: binario costruito alle 10:38 da 5426209f, albero b498afec.
            Il punto cieco dello script e' chiuso a misura per QUESTA domanda —
            `git diff 5426209f b498afec -- Turn/ Combat/ ScenarioHarness/` e' vuoto.
   FAIL:    nessuno.
   NOT RUN: la suite intera (`-Filter RefactorTactics`) — eseguito il solo filtro sopra.
            Nessuna build, nessuna sessione Editor aperta da questo lavoro.

L. FIRST EXECUTABLE ISSUE
   🔵 CORRETTA il 2026-08-31: questa voce diceva «#1678 — l'unica candidata P0 del kit (TD01-A)
   che ha una issue APERTA e nessuna dipendenza non soddisfatta», ed era falsa su DUE punti che
   il criterio usato non poteva vedere. #1678 e' una PARENT con sei figli — #1679, #1680, #1681
   e #1705 gia' CHIUSI — quindi «aperta» non distingue «c'e' lavoro qui» da «c'e' lavoro sotto».
   Ed e' etichettata P2, non P0: le sub-issue del Tactical Designer stanno a P2/P3 perche' lo
   strumento e' out_of_release_scope (D-154), mentre P0/P1 sono legate alla release.

   #1682 — Launcher L6: dalla sessione al workspace
   Perche' questa: era l'unico figlio di #1678 con le dipendenze soddisfatte (#1680 e #1681
   chiuse) e senza codice consegnato — misurato: `Start Session` aveva UNA occorrenza in
   `Source/`, un commento che la assegnava esplicitamente a questa slice.
   ✅ CHIUSA il 2026-08-31, PR #1911. Resta #1683 (L8), che dipendeva da questa.

M. RECOMMENDED NEXT 3 ISSUES
   1. #1683 — L8, la sessione ricordata per utente (sbloccata da #1682)
   2. #1626 — T2 authoring degli intent (meta' di TD01-D)
   3. #1625 — T1 playback visuale (sblocca #1628, e toglie una delle due superfici
      «pendenti» dal registro del launcher)

N. COMMITS
   Nessun commit eseguito da questa revisione: vedi §11.
```

---

## 10. Cosa del kit sopravvive

✅ **Il §2 e il §8.** L'invariante *«l'Editor possiede i controlli, il runtime possiede l'esito»* e la lista
dei non-goal (`no GAS`, no balance dashboard, no mass simulation, no promotion workflow) coincidono riga per
riga con `ADR-0010`, con il §3 dell'owner e con l'elenco *«esplicitamente post-Trial»* di `#1105`. Zero
contenuto nuovo, zero errori: è la parte che si può citare senza verificarla.

✅ **La disciplina del §5 e del §11.** *«REUSE / UPDATE prima di CREATE»* più una matrice obbligatoria è
esattamente ciò che ha impedito a questo consumo di aprire dieci issue duplicate. Il kit fornisce lo
strumento che lo confuta.

✅ **La domanda del §3**, che è il suo unico contributo davvero nuovo: *quale singola azione ha un percorso
completo dal catalogo alla presentation?* La risposta misurata è: `Hero.Gadget.ArcPulse` lo ha **intero fino
al TurnLog e agli scenari**, e lo perde alla presentation — dove non è `PARTIAL`, è `NONE` (`#1663`). E
cercandola è emerso il §8.

⛔ **Non recepito**: la scala `TD 0.x` come nuova numerazione, il nome «Action Lab», il termine `candidate`,
l'ID ritirato della Golden Skill, la presentation source «Plasma Blast», il path owner, e le dieci issue
candidate come issue.

---

## 11. Stato di questo lavoro, e cosa NON è stato fatto

⚠️ **Il checkout è condiviso, e si è mosso tre volte durante il lavoro**: `16b295ae` → `0c0ee87c` →
`5426209f` (`fix/711-sonda-roster-per-hover`) → `b498afec` (`main` locale), per mano di altre sessioni, con
un `UnrealEditor.exe` vivo e un `.uasset` altrui modificato. Creare un branch con `switch` avrebbe spostato
`HEAD` sotto quelle sessioni. I due commit di questo lavoro sono stati costruiti con **plumbing** su
`origin/main` — indice temporaneo, `commit-tree`, `git branch` — quindi `HEAD` e il working tree non si sono
mai mossi, e il `.uasset` altrui non è mai stato indicizzato.

⚠️ **`NOT RUN`**: la suite intera, la build, le sessioni Editor. Eseguito il solo filtro dichiarato al §9 K.

### Prossimo passo

⛔ **Fatto.** `Combat.BasicAttack` è stato eseguito e passa a `103` (§8), e il difetto — cresciuto da una a
**tre** righe con tre cause diverse — è tenuto da
[#1904](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1904).

> 🔵 **E il passo che questa riga indicava non esisteva: corretto il 2026-08-31.** Diceva *«**#1678**, il
> launcher come ingresso unico — l'unica candidata `P0` del kit con una issue aperta e nessuna dipendenza non
> soddisfatta»*. #1678 è una **parent** `P2` con quattro figli su sei già chiusi: *«aperta»* non distingue
> *«c'è lavoro qui»* da *«c'è lavoro sotto»*, e la label di una parent descrive il tema, non la priorità del
> lavoro rimasto. Il criterio che questo referto ha applicato — `OPEN` più dipendenze soddisfatte — è vero e
> **insufficiente**, ed è la stessa forma di difetto che il §5 aveva rimproverato al kit: un criterio che
> misura ciò che si può leggere in fretta invece di ciò che si sta chiedendo.
>
> ✅ **Il figlio eseguibile era #1682**, chiuso il 2026-08-31 con la PR
> [#1911](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1911) — e la misura che l'ha
> identificato costa una riga: `rg "Start Session" Source` dava **una** occorrenza, un commento che
> assegnava la slice a quella issue.

Il prossimo passo è **#1683** (`L8`), che dipendeva da #1682 ed è ora sbloccata.
