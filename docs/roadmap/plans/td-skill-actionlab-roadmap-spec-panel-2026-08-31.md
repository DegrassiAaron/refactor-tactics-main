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

⚠️ **`NOT RUN`**: lo scenario **non è stato eseguito** in questa sessione. Che il `103` sia il valore vivo è
un'inferenza da `D-224`, non una misura. Prima di correggere la prosa va eseguito `Combat.BasicAttack` — se
fallisse, il difetto starebbe dall'altra parte e sarebbe più grave.

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
   PASS:    nessuno eseguito in questa sessione.
   FAIL:    nessuno.
   NOT RUN: ./scripts/rt-suite.ps1 — non eseguita.
            Combat.BasicAttack — non eseguito (rilevante per §8).
            Nessuna build, nessuna sessione Editor aperta da questo lavoro.

L. FIRST EXECUTABLE ISSUE
   #1678 — Tactical Designer — DevSandbox Launcher: entrare nel workflow da L_DevSandbox
   Perche' questa: e' l'unica candidata P0 del kit (TD01-A) che ha una issue APERTA e nessuna
   dipendenza non soddisfatta. Tutto cio' che il launcher deve aprire — scenario, harness,
   run, reset, TurnLog, viewport, LOS inspector — e' consegnato.

M. RECOMMENDED NEXT 3 ISSUES
   1. #1678 — launcher come ingresso unico
   2. #1626 — T2 authoring degli intent (meta' di TD01-D)
   3. #1625 — T1 playback visuale (sblocca #1628)

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

⚠️ **Niente è stato committato.** Durante la revisione `HEAD` si è mosso da `16b295ae` a `0c0ee87c` per mano
di un'altra sessione, e un `UnrealEditor.exe` è vivo. Creare un branch qui sposterebbe `HEAD` sotto quella
sessione, che è il difetto che il repository ha già registrato più volte. I tre file di questo consumo sono
sul disco e **non tracciati**; il branch `docs/…`, il commit e la PR verso `main` restano da fare come atto
esplicito.

⚠️ **`NOT RUN`**: nessuna build, nessuna suite, nessuno scenario eseguito, nessun `.uasset` aperto, nessuna
scrittura su GitHub.

### Prossimo passo

Eseguire `Combat.BasicAttack` e, se passa a `103`, aprire una issue per le **quattro righe di prosa** del §8
— è un difetto misurato, piccolo, con owner evidente, e oggi nessuna issue lo tiene.
