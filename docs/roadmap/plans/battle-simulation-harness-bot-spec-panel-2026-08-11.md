# Battle Simulation, Unified Scenario Harness e Bot Release Roadmap — spec panel

> `CURRENT` · **Stato**: revisione chiusa, **nessuna modifica applicata** al canone · **Data**: 2026-08-11
> **HEAD della revisione**: `6bdeb9a` · branch `feat/hex-layer-focus-view`
> **Sorgente revisionata**: `RefactorTactics_BattleSimulation_UnifiedScenarioHarness_BotReleaseRoadmap_Claude_2026-08-11.md`
> (2139 righe, 43 sezioni, untracked), archiviato a fine sessione in
> [`../../archive/src/handoff/2026-08-11-battle-simulation-scenario-harness-bot-roadmap.md`](../../archive/src/handoff/2026-08-11-battle-simulation-scenario-harness-bot-roadmap.md)
> **Scopo**: eseguire il punto 10 del mandato che il documento stesso si dà — classificare ogni suo elemento
> come `ALREADY_PRESENT` / `UPDATE` / `NEW` / `CONFLICT` / `DEFERRED` / `SUPERSEDED` — **prima** che qualcuno
> apra 96 issue, 6 epic e 48 scenari.
> **Regola applicata**: la §0.1 del sorgente. Un handoff sta sotto codice as-built, registry, ADR e owner
> spec. Dove contraddice un fatto misurabile sul branch, prevale il repository e la proposta si **registra**.
> **Cosa non è**: un piano di implementazione. Il §7 dice qual è l'unica modifica eseguibile oggi, e si
> applica solo se l'autore la vuole.

---

## 1. Il verdetto in una riga

Questo è il **miglior handoff della serie sul piano dell'accuratezza fattuale** — 16 path su 17 esistono
davvero, le due epic che nomina sono vive, le due Feature ID sono quelle giuste — e sbaglia comunque la cosa
che conta: **colloca il lavoro in uno spazio di release già occupato**, mentre il gate che governa quello
spazio è verde **1 volta su 15**.

Non è un difetto di contenuto. Il contenuto è buono: §18 (competence gate prima del balance) è l'idea più
forte arrivata in un handoff da settimane. È un difetto di **collocazione temporale**, e il documento non
poteva vederlo perché nessuna delle sue fonti gliel'ha detto.

---

## 2. Il conto

Unità di classificazione: le 43 sezioni, più i blocchi enumerati che portano lavoro (issue, scenari, path,
PDR). Una sezione può contribuire a più righe quando i suoi elementi si dividono.

| | Voci | Significato |
|---|---:|---|
| `ALREADY_PRESENT` | **14** | il repository lo fa già, spesso con nomi diversi |
| `UPDATE` | **11** | esiste un owner che va esteso, non un sistema da creare |
| `NEW` | **9** | proposta nuova senza conflitti: si registra o si costruisce |
| `CONFLICT` | **5** | collide con una milestone, un'epic o una struttura già allocata |
| `DEFERRED` | **3** | corretto, ma bloccato da un gate esplicito del repository |
| `SUPERSEDED` | **2** | punta a un owner che una decisione recente ha rimosso |

**Il rapporto che decide la revisione**: su **96 issue candidate**, **13 sarebbero duplicati** (§21),
**35 sono bloccate da policy** (§22–§23), **33 puntano a uno spazio di release occupato o inesistente**
(§24–§26). Restano **15** proposte che nessuna struttura corrente copre — e tutte e 15 stanno in due
sezioni: §18 e §16.

---

## 3. Le sette misure

Ripetibili sul branch, nell'ordine in cui sono state fatte.

**M1 — I path dichiarati esistono.** 17 citati in §1, **16 presenti**.

```bash
# l'unico mancante:
ls docs/wiki/feature-status.md   # → No such file
```

Non è un errore del documento: quel file **esisteva** e `D-076` (2026-08-10) l'ha rimosso spostando la wiki
in un repository separato. Il documento fotografa un repository di **due giorni prima**.

**M2 — Le epic nominate sono vive.** `#326` e `#328` sono `OPEN`. Delle 11 issue correlate citate in §1.2,
**7 sono aperte e 4 chiuse** (`#36`, `#202`, `#213`, `#337`): il documento le presenta tutte come
«dipendenze da riverificare» senza dire che un terzo del lavoro è già a terra.

**M3 — Il gate che blocca tutto.** Le milestone `v0.2` e `v0.3` portano scritto nella propria descrizione:

> «NESSUNA epic di questa milestone si apre prima che i 15 gate della v0.1 siano verdi.»

Stato reale di quei 15 gate (`docs/roadmap/v0.1-definition-of-done.md` §3):

| Verde | Giallo | Aperti |
|---:|---:|---:|
| **1** (`G12` packaging) | **1** (`G13`, con riserva: verificato sull'arena di test) | **13** |

Le §22 e §23 — 35 issue candidate — sono corrette nel merito e **inapplicabili oggi**. Il documento non
nomina mai questo vincolo.

**M4 — La v0.4 è già presa.** Il sorgente propone «GAME RELEASE v0.4 — Unified Battle Simulation & Balance
Lab», premettendo (§24) «se il roadmap reale non possiede già un nome/scopo v0.4 differente».
Lo possiede:

```
milestone 10 · "v0.4 · Operations" · 19 issue aperte
  #330 E33 Conditional Intent · #331 E30 Classe di mappa Operations
  #332 E31 Obiettivi multipli e logistica · #333 E32 Formato 4v4 competitivo
```

E le milestone `v0.5` / `v0.6` a cui le §25–§26 assegnano 27 issue **non esistono**: la numerazione reale si
ferma a `v0.4`. Il documento apre due release per collocarci del tooling.

**M5 — La CI su cui poggia la §19 non c'è.**

```bash
ls .github/workflows/   # → la directory non esiste
```

I quattro tier («per commit / PR», «nightly 256–512 paired seeds», «milestone: migliaia di match») descrivono
un'infrastruttura assente. Nessuna sezione la dichiara come prerequisito, e nessuna delle 96 issue candidate
la costruisce.

**M6 — Gli otto owner della §32 sono PDF archiviati.** `PDR-03`…`PDR-10` vivono in
`docs/archive/pdr-v0.1/` come **PDF**. L'ownership è passata agli `ADR-0001`…`ADR-0009` più le spec markdown;
`PDR-07` porta ancora «GAS» nel titolo, che la v0.1 ha escluso. «Integrare PDR-03 Architecture» non è
un'operazione eseguibile.

**M7 — Il bot as-built.** `URTHexBotLibrary`, **443 righe**. Il wiring è `ARTTurnManager::PlanBots()`
(`RTTurnManager.cpp:130`), e la riga che decide questa revisione è la **141**:

```cpp
const FRTHexSnapshot Snapshot = MakeCurrentSnapshot(Units); // solo unita' vive
```

Lo snapshot è **canonico e completo**. `Ctx.Enemies` contiene tutti i nemici vivi, visti o no. `TeamKnowledge`
non esiste come tipo runtime — la stringa compare in un solo file, `RTPerceptionTests.cpp` — e ciò che esiste
è `URTPerceptionLibrary` con `TeamVisibleCells()` e `ERTAwareness`, **non ancora consumata dal bot**.

---

## 4. Il panel

Sei revisori, un focus ciascuno. Le citazioni sono ricostruzioni della metodologia, non attribuzioni reali.

### 📋 Wiegers — testabilità dei gate

> «Confronta i tuoi exit gate con quelli che il repository ha già scritto, e vedrai il problema.»

Il documento (§21):

```
A 2v2 autonomous match can complete without direct state shortcuts.
```

Il repository (`G4`, stessa famiglia di affermazione):

```
| G4 | Determinismo: 100 ripetizioni, checksum identico
     | RefactorTactics.Simulation.DeterministicReplay | ⏳ |
```

La differenza non è di stile: la seconda **nomina l'oracolo**. Nessuno dei 6 blocchi di exit gate del
sorgente (§21, §22, §23, §24, §25, più i tre DoD §35–§37) dice *con quale comando* si verifica una singola
riga. Il `G9` del repository arriva a specificare che il conteggio va fatto con
`grep -c '^\| \*\*PIE-[A-Za-z0-9.-]*\*\* \`RELEASE-V01\`'` **e non** con `grep -c RELEASE-V01`, perché il
secondo conta anche la prosa — e spiega perché. Su questo asse **il repository è più maturo del documento
che vorrebbe migliorarlo**, ed è il motivo per cui i DoD §35–§37 vanno letti come checklist di intenti, non
copiati in un gate.

### 🔬 Adzic — dall'affermazione all'esempio

> «La sezione migliore è l'unica che porta un esempio con dei valori dentro.»

§18 lo fa:

```
Riva WR 42%
Bot competence: Water setup PASS · Displacement PASS · Steam FAIL · Noise UNTESTED
→ NON nerfare Riva. Prima certificare la competence AI.
```

Quello è un esempio falsificabile: dice quale dato guardare, quale conclusione *non* trarre, e perché. Le
altre 42 sezioni sono blocchi ` ```text ` di sostantivi — §17 elenca 40 metriche di telemetry senza un solo
valore atteso, §11 elenca 30 classi di assertion senza una sola assertion scritta. **La §18 va estratta e
scritta in un owner QA; il resto della telemetria non è pronto per diventare issue.**

Nota di merito: il documento **sa** di non dover bloccare troppo presto — §12: «non bloccare hash golden
mentre contenuti/regole sono ancora volutamente instabili». È la cosa giusta, ed è già la pratica del
repository (`E15`, golden replay come gate d'epica e non di ogni PR).

### 🎯 Cockburn — chi chiede questo, e quando

> «L'attore di questo documento non è chi sta consegnando la v0.1.»

Il goal dichiarato (§43) è: rispondere a «quale livello di Bot AI è verificabile a ciascun checkpoint». È il
goal di un **release manager con un gioco già giocabile**. L'attore reale del repository oggi ha 13 gate
aperti su 15, una partita packaged verificata *sull'arena di test* e non sulla mappa di release, e sta
lavorando alla vista a livelli dell'editor.

Non è un errore di analisi — è il documento che risponde a una domanda che il progetto **si porrà**, con la
struttura giusta per porsela. Ma la domanda «cosa sa fare il bot nella v0.1?» ha già una risposta canonica,
ed è una riga del registry: `RT-FEAT-BOT-BASE`, `RELEASE_READY`, epic `E2`, checkpoint `2.6`, test
`RefactorTactics.HexBot.*`. Il grafo che la §43 vuole costruire **esiste già e risponde**.

### 🏗️ Fowler — il seam

> «Il seam proposto è quello giusto. Il documento però lo descrive come se ci fosse.»

§5 e §7 dicono «usare i nomi reali del repository», e poi introducono `Decision Provider`, `Agent
specification`, `ExecutionMode`. Nessuno dei tre esiste:

```bash
git grep -nE "ExecutionMode|DecisionProvider" -- Source/RefactorTactics/ScenarioHarness/
# → una sola riga, ed è un commento su cosa fa la modalità headless
```

L'Harness as-built (7 file in `Source/RefactorTactics/ScenarioHarness/`) ha **un solo appiglio** verso il
gioco, e lo dichiara nel proprio header:

> `RTScenarioRunner.h:20` — «L'unico appiglio e' `ARTTurnManager::PlanBotsForTest()`, che esisteva gia' per
> i test d'integrazione.»

Il che significa: il seam «un provider per slot» è **un'estrazione da fare**, con un costo, non un contratto
da documentare. Chiamarlo consolidamento nasconde il costo. La direzione però è corretta, e il vincolo §7
— «non creare un provider che restituisce outcome, deve restituire decisioni» — è la formulazione più
precisa che questo invariante abbia ricevuto finora: **quella riga vale più delle 15 issue di §24**.

### 💥 Nygard — cosa si rompe

> «Tre delle sei release proposte poggiano su infrastruttura che nessuna delle 96 issue costruisce.»

Il batch runner di §15, i seed corpus di §16 e i tier di §19 presuppongono: una CI (assente, M5), una policy
di retention degli artefatti (nominata una volta in §26, mai specificata), e un budget di macchina per
«centinaia/migliaia di match» che nessuna sezione stima. §39 dice la cosa giusta — «misurare, non inventare
hard gate senza profiling» — e poi §19 fissa comunque `256–512` paired seeds nightly senza aver misurato
quanto dura un match headless, che oggi **non esiste**.

Il finding operativo: **il primo passo reale non è il batch, è il match headless singolo con un tempo
misurato**. Da quel numero si deriva se 512 è un nightly o un weekend.

### 🧪 Crispin — la piramide e il gate di competenza

> «§18 è la sezione che salva il documento. Va letta al contrario.»

«Un risultato Bot-v-Bot non è evidenza forte di balance finché il bot non è certificato sulle capability che
determinano quel risultato.» Letta al contrario: **finché il bot non è certificato, i risultati di balance
che il progetto già produce vanno etichettati come non-evidenza.** Questo è azionabile *oggi*, senza batch
runner, senza CI e senza le 96 issue — è una regola da scrivere accanto al workbook di bilanciamento.

Sul resto: §29 propone **48 scenari** contro i **62 già presenti** (9 `Combat`, 6 `Movement`, 26 `Spec`,
21 `Visual`), e la sovrapposizione è reale ma non catastrofica —

```bash
git ls-files 'Scenarios/**' | grep -ci bot     # → 0
git ls-files 'Scenarios/**' | grep -icE water  # → 4 (Spec/Environment ×2, Visual/Combat ×2)
```

**Zero scenari del corpus esercitano il bot come soggetto.** È il gap più concreto che il documento
identifica, e la §29 v0.1 (8 scenari) è la sua parte migliore.

---

## 5. I findings, per severità

### 🔴 F1 — §2.2 è presentata come decisione da preservare, ed è un requisito non implementato

Il documento la colloca sotto «Decisioni correnti da preservare» e ne deriva un invariante:

```
Same TeamKnowledge + same BotConfig + same BotSeed = same Intent
```

L'invariante **non è falsificabile oggi**: non esiste un `TeamKnowledge` da tenere fisso (M7), e il bot legge
lo snapshot canonico. Chi leggesse la §2.2 come stato corrente concluderebbe che il bot è già fair — e
scriverebbe la canary di §21 (`[v0.1][Bot QA] Add hidden-state anti-omniscience canary`) aspettandosi che
passi. **Fallirebbe**, correttamente.

La collocazione giusta esiste ed è precisa — `E13`, issue `#151`, `OPEN`, con il gate già scritto nel DoD:

> «**il bot decide sulla Team Knowledge della propria squadra**, mai su stato nemico nascosto»

→ `NEW`, dipendente da `E13`. **Non** `ALREADY_PRESENT`.

### 🔴 F2 — l'epic v0.1 di §21 duplicherebbe tre epic esistenti

Le 13 issue candidate mappano su struttura già allocata:

| Issue candidata §21 | Dove vive già |
|---|---|
| `Route bot decisions through normal FRTIntent validation` | `PlanBots()` scrive gli stessi campi `Planned*` dell'input umano (`RTTurnManager.cpp:151-153, 491-502`) |
| `Add deterministic legal move/objective candidate selection` | `RT-FEAT-BOT-BASE` `RELEASE_READY` · le candidate nascono da `ReachableCells` (commento `RTTurnManager.cpp:138`) |
| `Add basic position cover and hazard utility` | `RefactorTactics.HexBot.*` — i test di cover/esposizione esistono (`RTHexBotTests.cpp:127-141`) |
| `Add basic ability candidate scoring` | idem, `ScorePlan` |
| `Consolidate Stable ScenarioId execution path` | `RT-FEAT-TEST-SCENARIO-HARNESS` `INTEGRATED`, epic `E15` |
| `Run scripted intents through normal Planning/Commit` | idem, `RTScenarioRunner` |
| `Add visual scenario run and TurnLog/result output` | idem + `RTTestReportWriter` |
| `Validate bot/scenario flow in packaged Development build` | gate `G12`/`G13`, **l'unico verde** |

Restano **3** issue con contenuto non coperto: la canary anti-onniscienza (bloccata da F1), i test di
ripetizione/permutazione applicati *al bot*, e lo smoke 2v2 autonomo. Tre issue non sono un'epic: sono
checkpoint dentro `E2` o `E12`.

→ `CONFLICT`. Il documento chiede in §34 di «cercare prima di creare»; questa è la ricerca.

### 🟠 F3 — v0.4 è occupata, v0.5/v0.6 non esistono

Vedi M4. Il documento se ne dà l'antidoto (§42: «non trattare v0.4+ proposta come canonica se la release
roadmap reale ha già altre decisioni») ma costruisce comunque tre release complete sopra quel dubbio: 42
issue, 4 epic, 18 scenari. La riconciliazione non è un dettaglio di numerazione — «v0.4» significa
**Operations** per chi legge la milestone, e Battle Lab per chi legge questo documento.

→ `CONFLICT`. Se il tooling merita una release, va nominato fuori dall'asse `v0.x` — che è esattamente ciò
che il registry ha già fatto per il Project Control Center: *«**Fuori dai 15 gate della v0.1** e senza epic:
è tooling di processo, e collocarlo nella roadmap di release lo farebbe competere con la consegna.»*
**Il precedente esiste e risolve il problema.**

### 🟠 F4 — §33 rigenererebbe un file che una decisione ha rimosso

`docs/wiki/feature-status.md` è elencato fra le viste da rigenerare. `D-076` l'ha eliminato il 2026-08-10.
Chi eseguisse la §33 alla lettera ricreerebbe nel repository una pagina che ora vive nel clone della wiki.

→ `SUPERSEDED`. Stessa classe la §31 (16 pagine wiki «da consolidare»): l'elenco è utile, il *dove* no.

### 🟡 F5 — §32 chiede di aggiornare otto PDF archiviati

Vedi M6. → `SUPERSEDED`. Il contenuto delle otto voci è però riassegnabile agli owner correnti, ed è lavoro
di poche righe: «Scenario Harness come adapter, non simulatore» appartiene a `ADR-0009`/`test-automatico-unreal.md`;
«fair BotKnowledge» a `E13`; «Battle Simulation usa lo stesso resolver» a `ADR-0009`.

### 🟡 F6 — sovrapposizione con un handoff già consumato

`docs/archive/src/handoff/2026-08-08-master-scenari-qa-e-bot.md` (846 righe, tre giorni prima) copre già:
Execution Mode (§6 → §8 di oggi), Scenario Registry (§7 → §5), assertions (§10 → §11), golden corpus
(§12 → §12), determinismo (§14 → §2.3), **bot roadmap v0.1 / Tactical / Expert (§27 → §20-§23)**, test
scenarios (§28 → §29). Il suo esito è registrato: *«il più assorbito … il bot era già recepito il
2026-08-08»*.

Il documento di oggi non lo cita. Riproporre la stessa scala su un repository che nel frattempo si è mosso
è il modo in cui un consolidamento diventa un secondo sistema — che è precisamente ciò che la §0 vieta.

→ Il valore **incrementale** rispetto al master è misurabile, ed è quello della sezione 6 qui sotto.

---

## 6. Cosa sopravvive

Cinque contributi che il repository non ha e che nessun handoff precedente porta.

| # | Contributo | Dove | Perché regge |
|---|---|---|---|
| **S1** | **Competence gate prima del balance** | §18 | Regola falsificabile con un esempio numerico. Azionabile **oggi**, senza infrastruttura: è una riga da scrivere accanto al workbook di bilanciamento e una colonna `PASS/PARTIAL/FAIL/UNTESTED` per capability |
| **S2** | **«Un provider restituisce decisioni, mai outcome»** | §7 | La formulazione più precisa dell'invariante bot→intent. Va nell'owner dell'harness, indipendentemente da quando il seam si costruirà |
| **S3** | **Seed corpus versionato + paired run + side swap + mirror** | §16 | Diagnostica il side bias, che nessuno strumento corrente misura. È l'unico blocco di §24 che non dipende da CI: due run con lo stesso seed e i lati scambiati si fanno a mano |
| **S4** | **Ability pipeline `LegalOpportunity → … → OutcomeValue`** | §17.3 | Distingue «ability debole» da «bot non sa usarla». È il **prerequisito misurativo di S1**, e da solo giustifica la telemetria |
| **S5** | **Otto scenari bot v0.1** | §29 | Il corpus ha 62 scenari e **zero** con il bot come soggetto (M7 → Crispin). Il gap è reale e la lista è dimensionata bene |

Nota su S3/S4: entrambi assumono un match headless. **Non esiste.** Il primo passo che li abilita è uno solo
— far girare un match completo senza presentation, e **misurarne la durata** —, ed è anche il passo che
rende decidibile se `256–512` nightly (§19) sia realistico.

---

## 7. L'unica modifica eseguibile oggi

Il documento chiede in §33.1 di «preferire aggiornare `RT-FEAT-BOT-BASE` / `RT-FEAT-BOT-TACTICAL` e
collegare capability/release ai checkpoint». Su una delle due il collegamento **manca davvero**:

```yaml
# docs/roadmap/feature-registry.yaml:3533
- feature_id: RT-FEAT-BOT-TACTICAL
  status: IDEA
  roadmap:
    epic: null          # ← #326 è "[EPIC v0.2] E26 · Tactical Bot v1", OPEN, milestone v0.2
    checkpoints: []
  issues: []            # ← #326 e #464 esistono ed è il loro perimetro
```

La convenzione del campo è il **codice** dell'epic, non il numero di issue (`RT-FEAT-BOT-BASE` ha `epic: E2`).
Il valore corretto è `E26`.

Non l'ho applicata: `/sc:spec-panel` è un comando documentale, il branch corrente porta lavoro sulla vista a
livelli dell'editor, e toccare il registry impone la catena `validate && generate && wiki` più
l'aggiornamento di `last_verified` — una PR a sé. **È l'unico deliverable dell'handoff che si può eseguire
senza aprire una release.**

---

## 8. Classificazione per sezione

| § | Elemento | Classe | Nota |
|---|---|---|---|
| 0, 0.1, 41 | Mandato e gerarchia delle fonti | `ALREADY_PRESENT` | Coincide con `CLAUDE.md` §1 e con la prassi dell'archivio |
| 1 | 17 path | `ALREADY_PRESENT` ×16 · `SUPERSEDED` ×1 | M1 |
| 1.1 | 2 Feature ID | `ALREADY_PRESENT` | Righe 3488 e 3533 del registry |
| 1.2 | 11 issue correlate | `ALREADY_PRESENT` | 4 su 11 sono già chiuse (M2) |
| 2.1 | Bot = producer di Intent | `ALREADY_PRESENT` | `PlanBots()` scrive i campi `Planned*`, non muove attori |
| **2.2** | **Fair knowledge** | **`NEW`** | **F1 — dipende da `E13`/#151** |
| 2.3 | Determinismo senza CPU budget | `ALREADY_PRESENT` | Invariante #4 del DoD trasversale |
| 2.4 | Utility AI custom, no BT autoritativo | `ALREADY_PRESENT` | As-built |
| 3 | Roster e Fast Reaction 3,0 s | `ALREADY_PRESENT` | Pin di `CLAUDE.md` |
| 4, 4.1 | Un solo harness | `ALREADY_PRESENT` (principio) · `NEW` (9 delle 13 capability) | |
| 5, 5.1, 6.1, 7, 8, 9 | Architettura, provider, ExecutionMode, lifecycle | `NEW` | F-Fowler: estrazione, non documentazione |
| 6, 11, 12, 13 | Definition, assertions, golden, result | `UPDATE` | `RTScenarioLoader`, `RTTestResult`, `RTTestReportWriter` esistono |
| 10 | Reaction nell'harness | `UPDATE` | `E14` in corso (#152, #165) |
| 14 | Replay del seed in `L_DevSandbox` | `NEW` | La mappa esiste; il RunManifest no |
| 15, 16, 17 | Battle runner, seed corpus, telemetry | `NEW` | S3, S4 |
| **18** | **Competence gate** | **`NEW`** | **S1 — il contributo più forte** |
| 19 | CI tiers | `DEFERRED` | M5: nessuna CI |
| 20 | Due assi da non confondere | `UPDATE` | L'asse harness viene dal master già archiviato, non è canone del registry |
| **21** | **Epic v0.1 + 13 issue** | **`CONFLICT`** | **F2** |
| 22, 23 | v0.2 (#326) e v0.3 (#328), 35 issue | `DEFERRED` | M3: milestone bloccate dai gate v0.1 |
| **24, 25, 26, 28** | **v0.4/v0.5/v0.6, 42 issue** | **`CONFLICT`** | **F3** |
| 27 | R&D Learning Agents | `DEFERRED` | Coerente: il documento stesso lo esclude dallo shipping |
| 29 | 48 scenari | `UPDATE` ×40 · `NEW` ×8 | S5: gli 8 di v0.1 |
| 30 | Editor Map | `UPDATE` | `editor-sessions.yaml` esiste |
| 31 | 16 pagine wiki | `SUPERSEDED` (dove) · `UPDATE` (cosa) | F4 |
| 32 | 8 PDR | `SUPERSEDED` | F5 — sono PDF archiviati |
| 33 | Control Center | `ALREADY_PRESENT` · 1 `SUPERSEDED` | F4 |
| **33.1** | **Feature ID e epic** | **`UPDATE`** | **§7 — l'unico eseguibile** |
| 34, 40, 42 | Policy issue, commit plan, «non fare» | `ALREADY_PRESENT` | |
| 35, 36, 37, 38 | Tre DoD + test trasversali | `UPDATE` | F-Wiegers: nessun oracolo nominato |
| 39 | Metriche di performance | `ALREADY_PRESENT` | «misurare, non inventare gate» è già la prassi (`E17`) |
| 43 | Grafo di risposta finale | `ALREADY_PRESENT` | Il registry già risponde |

---

## 9. Cosa non è stato fatto

Dichiarato, non taciuto.

- **Nessuna modifica al canone.** Registry, roadmap, DoD, wiki, ADR e GitHub sono intatti. Le Fasi C–E del
  mandato (§41) non sono state eseguite: presuppongono decisioni di release che non sono mie.
- **Nessuna issue creata o aggiornata.** Comprese `#326`/`#328`, che il documento chiede di ampliare: farlo
  significherebbe scriverci dentro 35 issue bloccate da M3.
- **La §7 non è applicata.** È un `UPDATE` di due campi in `feature-registry.yaml`, e richiede la sua catena
  di rigenerazione e una PR propria.
- **Non ho misurato la durata di un match**, perché il match headless non esiste: è il primo passo di S3/S4
  e resta da fare.
- **Non ho verificato la §17 contro il workbook di bilanciamento** (`.xlsx`): le metriche già raccolte
  potrebbero sovrapporsi a parte delle 40 proposte. Va fatto prima di trasformare §17 in issue.

---

## 10. Provenienza

| | |
|---|---|
| Sorgente | `RefactorTactics_BattleSimulation_UnifiedScenarioHarness_BotReleaseRoadmap_Claude_2026-08-11.md`, 2139 righe |
| Predecessore assorbito | [`2026-08-08-master-scenari-qa-e-bot.md`](../../archive/src/handoff/2026-08-08-master-scenari-qa-e-bot.md), 846 righe |
| Revisione | Questo referto · HEAD `6bdeb9a` · branch `feat/hex-layer-focus-view` |
| Archiviazione | [`../../archive/src/handoff/2026-08-11-battle-simulation-scenario-harness-bot-roadmap.md`](../../archive/src/handoff/2026-08-11-battle-simulation-scenario-harness-bot-roadmap.md) |
| Owner che decidono il seguito | [`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) §3 (M3) · [`feature-registry.yaml`](../feature-registry.yaml) (§7) · milestone `v0.4 · Operations` (F3) |
