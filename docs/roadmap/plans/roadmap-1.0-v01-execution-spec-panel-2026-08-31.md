# Roadmap Epic → 1.0 + Issue Plan v0.1 — spec panel

> `CURRENT` · **Referto di revisione**, non owner. Consuma
> [`../../archive/src/handoff/2026-08-31-roadmap-1.0-v01-execution-prompt.md`](../../archive/src/handoff/2026-08-31-roadmap-1.0-v01-execution-prompt.md)
> (arrivato in radice come `CLAUDE_RT_Roadmap_1.0_and_v0.1_Execution.md`, **untracked**, fino a questo commit).
>
> **Data**: 2026-08-31 · **Base**: `feat/172-combat-preview-origine-e-bersaglio-cella` @ `ea6640e6`,
> **9 avanti / 3 dietro** `origin/main` = `98949775` dopo `git fetch --prune` · **Modo**: critique ·
> **Focus**: requirements + architecture documentale + compliance
>
> **Cosa è**: il verdetto su una **specifica di lavoro** che si autodichiara eseguibile («# 20. EXECUTE NOW»)
> e ordina di creare epic, creare 29 issue e implementare. Il referto giudica la specifica e **non esegue il
> lavoro che ordina**: `/sc:spec-panel` è task documentale, e **nessuna issue è stata creata, chiusa o
> modificata**, nessun `Build.cs` toccato, nessun asset aperto.
>
> **Cosa non è**: un'autorità. Se una riga qui diverge da [`../v0.1-definition-of-done.md`](../v0.1-definition-of-done.md),
> da [`../roadmap-v0.1.md`](../roadmap-v0.1.md) o dal [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md),
> **ha ragione l'owner**.
>
> ⚠️ **Secondo giro dello stesso mandato.** Il 2026-08-29 un work order della stessa famiglia è già stato
> consumato da [`roadmap-issues-v01-v10-spec-panel-2026-08-29.md`](roadmap-issues-v01-v10-spec-panel-2026-08-29.md),
> con verdetto *«rigorosa nel metodo e scaduta nei fatti»*. Questo kit è **più scaduto di quello**, e per una
> ragione diversa: non sbaglia gli identificatori, sbaglia la **release**.

---

## 1. Il verdetto in una riga

> **La specifica è un piano di fondazione corretto per un repository che non esiste più: descrive la v0.1
> come «griglia 2D, due unità, movimento», mentre quella milestone — `v0.1 · Fondamenta` — è chiusa su
> GitHub con 0 issue aperte e 52 chiuse, e la v0.1 reale consegna il 2v2 con abilità e reazioni che il kit
> colloca in v0.6.**

Non è una specifica sbagliata. È una specifica **giusta con molto ritardo**: quasi ogni sua prescrizione
tecnica è sensata, e **23 delle 29 candidate issue** descrivono codice che nel repository è già scritto,
testato e mergiato (§2.3). Eseguirla alla lettera produrrebbe, nell'ordine: una terza tassonomia di
identificatori, una `Build.cs` che non linka, e un `§16 Out of scope` che è una **lista di rimozione** di
feature consegnate.

⚠️ **La candidata che sembrava salvarlo non lo salva.** `F0-26` parte da un fatto vero — in tutto `Source/`
non esiste **un solo marker di profiling** (zero occorrenze di `TRACE_CPUPROFILER`, `SCOPE_CYCLE_COUNTER`,
`DECLARE_CYCLE_STAT`, `CSV_`) — ma la materia ha già un owner **aperto e `P0` nella milestone giusta**,
[#84](https://github.com/DegrassiAaron/refactor-tactics-main/issues/84), il cui DoD prescrive il `p99` con
un argomento migliore di quello del §14. Il residuo non coperto da nessuno si riduce a **due metriche**:
`queries/frame` ed `ExpandedNodes`. Vedi §5.1 — dove è annotato anche **come questo referto ha sbagliato
due volte** prima di misurarlo.

---

## 2. Base di misura

Tutto ciò che segue è **misurato**, non ricordato. Conteggi su albero locale, stato issue letto lato
server con `gh`.

```text
Repo      : DegrassiAaron/refactor-tactics-main
Branch    : feat/172-combat-preview-origine-e-bersaglio-cella
HEAD      : ea6640e6  (9 avanti / 3 dietro origin/main = 98949775)
Data      : 2026-08-31
Sorgente  : untracked in radice (git status --porcelain -> ??), 1960 righe (wc -l)
```

### 2.1 Le premesse della specifica

| Affermazione della specifica | Misura | Esito |
|---|---|---|
| La v0.1 è «F0 Foundations»: griglia 2D, 2 unità, movimento | `gh` milestone `v0.1 · Fondamenta` | 🔴 **0 open / 52 closed** — chiusa |
| La v0.1 esclude abilità, reazioni, perception, replay (§16) | `Source/RefactorTactics/{Ability,Perception,Replay}/` + `RTReactionLibrary` | 🔴 tutti **presenti e testati** |
| Il 2v2 vertical slice è **v0.6** (EPIC F4) | [`../../product/piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md), `D-256` | 🔴 il 2v2 **è ciò che la v0.1 consegna** |
| Le 17 fonti del §1 sono leggibili | `find docs -iname` su ciascuna delle 17 | 🔴 **15 su 17 assenti**; le 2 reperibili hanno **nome o versione diversi**, e una vive in `docs/archive/` |
| «`GameplayAbilities` non entra in F0» | pin [`../../../CLAUDE.md`](../../../CLAUDE.md) §2 | ✅ concorde — *No GAS nella v0.1* |
| Baseline `Build.cs` richiesta: include `AIModule` | `RefactorTactics.Build.cs` | 🔴 **assente**, e il bot (`RTHexBotLibrary`) funziona senza |
| «`Build.cs` minimo e coerente» (F0-02 DoD) | idem | 🔴 il minimo del kit **omette** `MeshDescription`, `StaticMeshDescription`, `AnimGraphRuntime` — vedi F-03 |
| `FRTCellId {X, Y, Layer}` va creata (F0-08) | `Map/RTCellId.h:28` | ✅ **esiste**, con `X=q`, `Y=r` assiali e `CubeZ()`; citata da **224 file** |
| «`Layer = 0` per F0 2D» | `RTCellId.h:24` | 🟠 il multilivello è **già semantico**: *«celle su layer diversi NON sono adiacenti: servono archi espliciti»* |
| Snapshot / resolver / TurnLog / hash vanno creati (F0-19…F0-23) | `Turn/` | 🔴 già presenti: `RTHexSim`, `RTTurnManager`, `RTTurnLog`, `RTMatchStateHash`, `RTPlaybackLibrary` |
| `L_DevSandbox` va creata (F0-05) | `Content/RT/Maps/Dev/L_DevSandbox/` | 🔴 **esiste**, con launcher dedicato in Editor |
| Automation da creare (F0-24) | `grep` su `IMPLEMENT_*_AUTOMATION_TEST` | 🔴 **1497 test in 163 file** (limite inferiore: 3 commit dietro) |
| La tassonomia `F0-00…F0-28` è libera | `gh issue list --search "F0-" --state all` | ⚠️ **zero risultati** — libera, ma **aliena**: vedi F-02 |

### 2.2 La tassonomia che il repository usa davvero

| Asse | Valore misurato |
|---|---|
| Issue totali / aperte | **1881** / **357** |
| Epic | **`E1` … `E51`** (`gh issue list --search "EPIC in:title"`) |
| Gate di release | **`G1` … `G15`** ([`../v0.1-definition-of-done.md`](../v0.1-definition-of-done.md)) |
| Milestone | **17**, di cui **7 interne alla v0.1** |

Le 7 milestone della v0.1 — e il loro stato al 2026-08-31:

| Milestone | Open | Closed |
|---|---:|---:|
| `v0.1 · Fondamenta` | **0** | 52 |
| `v0.1 · Mondo giocabile` | 14 | 14 |
| `v0.1 · Leggibilità` | 29 | 13 |
| `v0.1 · Percezione e reazioni` | 19 | 23 |
| `v0.1 · Prova integrata` | 6 | 8 |
| `v0.1 · Gate di release` | 14 | 26 |
| `v0.1 · Difetti e bilanciamento` | 4 | 36 |

### 2.3 Le 29 candidate, classificate una per una

Classificazione con il vocabolario che il kit stesso impone (§5), applicata alle sue 29 candidate.
Ogni riga poggia su una misura, non su una lettura del titolo.

| Classe | N. | Candidate | Evidenza |
|---|---:|---|---|
| **`ALREADY_DONE`** | **23** | `F0-02` … `F0-17`, `F0-19` … `F0-25` | sorgente presente in `Source/` (`RTCellId`, `RTHexPathLibrary`, `RTHexSim`, `RTTurnManager`, `RTTurnLog`, `RTMatchStateHash`, `RTPlaybackLibrary`, `Preview` in 44 file, …) + 1497 test + 97 scenari |
| **`REUSE`** (gate già coperto) | 2 | `F0-27` packaged · `F0-28` golden gate | coperti da `G1…G15` in [`../v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) |
| **processo, non prodotto** | 2 | `F0-00` audit · `F0-01` lock UE/toolchain | non producono codice; l'engine è pinnato a **5.8** in `.uproject` |
| **N/A — modello diverso** | 1 | `F0-18` Ready/Unready + countdown | il loop reale è `ERTMatchPhase{Planning, Prep, Dash, Blast, Move, Cleanup}` (`Turn/RTTurnRules.h:10`): non c'è un Ready di turno da annullare |
| 🟠 **`PARTIAL` — owner esistente** | 1 | `F0-26` performance + marker | **zero marker di profiling** in `Source/`, ma la materia è di [#84](https://github.com/DegrassiAaron/refactor-tactics-main/issues/84) (OPEN, `P0`, `v0.1 · Gate di release`), che prescrive già il `p99`. Scoperto da nessuno: solo `queries/frame` ed `ExpandedNodes` — §5.1 |

⚠️ **Un negativo di questa tabella è stato ottenuto due volte.** Il primo `grep` dava `F0-17` (ghost path)
come assente: era cieco ai nomi reali — `Preview` compare in **44 file**. Un secondo passaggio con pattern
più larghi l'ha ribaltato. I tre negativi rimasti (`queries/frame`, marker, Ready di turno) sono stati
verificati con almeno **due** famiglie di pattern ciascuno prima di essere scritti qui.

⛔ **Non misurato**: nessun `.uasset` aperto; nessun gate dichiarato verde o rosso da questo referto
(legge quelli che l'owner dichiara); **nessuna build e nessuna suite eseguita** — il referto non tocca
codice. I 3 commit di scarto da `origin/main` toccano `Combat/`, `Map/`, `Turn/`, `Unit/` ed Editor, non i
documenti citati: i conteggi di test e sorgenti sono quindi un **limite inferiore**, non un valore esatto.

---

## 3. Punteggi

| Dimensione | Voto | Ragione in una riga |
|---|---|---|
| **Chiarezza** (Doumont) | **7.5** / 10 | struttura numerata, grafo di dipendenze leggibile, blocchi `text` invece di prosa; manca un sommario esecutivo |
| **Completezza** | **8.0** / 10 | copre audit, epic, issue, DoD, performance, template e report finale; manca la **condizione di terminazione anticipata** (cosa fare se F0 risulta finito) |
| **Testabilità** (Wiegers · Adzic) | **6.5** / 10 | i DoD sono spesso misurabili (`path median < 2 ms`, `p50/p95/p99`), ma **nessun criterio è un comando eseguibile** e non c'è un solo esempio Given/When/Then |
| **Consistenza** | **4.0** / 10 | tre assi di identificatori incompatibili col repository; il `§16` vieta nella v0.1 ciò che il `§4` colloca in v0.4 |
| **Fedeltà misurata** | **1.5** / 10 | la release di riferimento è sbagliata; **15 fonti su 17 assenti**; **23 candidate su 29** sono `ALREADY_DONE`, e la sola `PARTIAL` (`F0-26`) ha già un owner aperto in [#84](https://github.com/DegrassiAaron/refactor-tactics-main/issues/84) — il residuo non coperto è **due metriche** |
| **Complessivo** | **5.5** / 10 | ingegneria buona, anagrafica scaduta — **da consumare per §14/§15, non da eseguire** |

---

## 4. Findings — critique

Severità: 🔴 critico (blocca o produce danno) · 🟠 maggiore (produce lavoro sbagliato) · 🟡 minore.

### 🔴 F-01 · Delle 17 fonti del §1, nessuna è reperibile come è scritta — COCKBURN, WIEGERS

> «Cerca nel repository … almeno: `RT_PDR_01_Visione_Game_Design_v0.1` … `RefactorTactics_FEATURE_MAP_*`» (§1)

Misura per nome su tutto `docs/`, **una voce alla volta** (`find docs -iname "*<nome>*"`), tutte e 17:

| Fonte prescritta | Esito |
|---|---|
| `RT_PDR_01` `_03` `_04` `_05` `_06` `_07` `_08` `_09` `_11` (v0.1) | 🔴 **9 su 9 assenti**: collassate in `docs/archive/pdr-v0.1/RT_PDR_v0.1_consolidato.md` |
| `RefactorTactics_3_Lane_…` · `_5_Lane_…` | 🔴 assenti |
| `RT_Reaction_System_Master_Consolidation_v0.1` · `RefactorTactics_Overwatch_FastReaction_Claude` | 🔴 assenti |
| `RT_Map_Environment_Master_Consolidation_v0.1` | 🔴 assente |
| `RefactorTactics_FEATURE_MAP_*` | 🔴 assente |
| `RT_PDR_10_Roadmap_QA_Rischi_v0.1` | 🟠 esiste in **v0.2**, non v0.1 — `docs/roadmap/` |
| `RefactorTactics_Rumore_Claude` | 🟠 esiste con **altro nome**, e **in `docs/archive/`**: `src/design/rumore-e-percezione-acustica.md` |

**15 su 17 sono assenti**, e le 2 reperibili non lo sono con il nome e la versione che il kit scrive: una
ha cambiato versione, l'altra nome *e* cartella. Nessuna delle 17 risponde alla ricerca così com'è posta.

Il danno non è la 404: è che le fonti v0.1 superstiti vivono in **`docs/archive/`**, e
[`CLAUDE.md`](../../../CLAUDE.md) §1 la elenca testualmente fra ciò che **non va usato come autorità
implicita**. La specifica quindi non manda l'esecutore a mani vuote — lo manda, con la forza di un
elenco puntato, verso l'unica cartella che il contratto del repository gli vieta.

**COCKBURN**: «Il primary actor qui è una sessione che deve decidere *cosa è vero oggi*. La specifica gli
dà una bibliografia e nessuna via per accorgersi che è scaduta: nessuna delle 15 righe porta una data, un
hash o un comando. Un elenco di nomi di file è la forma meno verificabile possibile di una fonte.»

**WIEGERS**: «"Cerca almeno questi" non è un requisito, è un augurio. La forma verificabile è *l'owner
corrente di X è il file che risponde a questa query*; se la query non torna nulla, il requisito ha fallito
rumorosamente invece di degradare in silenzio verso l'archivio.»

📝 **Riscrittura**: `le fonti sono quelle elencate in CLAUDE.md §1 (piano canonico, Decision Log, ADR, DOC_CONFLICT_MATRIX, OPEN_DECISIONS, roadmap owner). docs/archive/ e docs/research/ non sono autorità. Se un documento citato per nome non esiste, fermati e chiedi l'owner: non ripiegare sull'archivio.`
🎯 **Priorità**: alta — è il primo passo che la specifica ordina, quindi il difetto scatta per primo.

### 🔴 F-02 · La specifica descrive una release che il repository ha chiuso, e ne colloca il contenuto reale quattro release più avanti — COCKBURN, PORTER

Le due tassonomie, affiancate:

| Il kit dice | Il repository misura |
|---|---|
| **v0.1** = F0 Foundations: griglia 2D, 2 unità, path, snapshot, TurnLog | **`v0.1 · Fondamenta`**: 0 open / 52 closed — **chiusa** |
| **v0.2** = Private Networking | `v0.2 · Struttura e finestre`; il networking è **`v0.5 · Online Foundation`** |
| **v0.3** = Abilities | `v0.3 · Informazione`; l'ability runtime è **`v0.6`** |
| **v0.6** = 2v2 Vertical Slice | **è la v0.1** (`D-256`, piano canonico) |

Non è uno scarto di naming: è uno scarto di **quattro release sull'asse del prodotto**. Un esecutore
letterale del §12 («implementa la prima issue P0 non Done… normalmente sarà una fra F0-01…F0-05») aprirebbe
`F0-01 Lock UE patch` su un repository che ha 1881 issue, 51 epic e 15 gate, e lo farebbe **credendo di
essere al primo giorno**.

**COCKBURN**: «Il goal è raggiunto e la specifica non ha modo di saperlo. È il difetto di `F-02` del referto
del 29 agosto, ripetuto su scala maggiore: là era un documento già consegnato, qui è una milestone intera
chiusa da 52 issue. Una specifica eseguibile deve poter fallire in fretta: qui manca la domanda *"e se
questa release fosse già finita?"*, che è l'unica che salverebbe la sessione.»

**PORTER**: «Il kit propone una struttura a 5 lane e 8 epic come se lo spazio fosse vuoto. Il repository
occupa già quello spazio con `E1…E51` e `G1…G15`. Introdurne una terza non aggiunge capacità: aggiunge
costo di riconciliazione permanente, pagato da chiunque legga il backlog dopo.»

📝 **Riscrittura**: sostituire il §4 e il §7 con un rimando: `la tassonomia di release, epic e gate è quella di roadmap-v0.1.md, v0.1-definition-of-done.md (G1…G15) e delle 17 milestone GitHub. Questa specifica non ne introduce una nuova; se una release qui descritta non combacia con la milestone omonima, vince la milestone.`
🎯 **Priorità**: alta — è la premessa da cui discende tutto il §5 e il §12.

### 🔴 F-03 · Il «Build.cs minimo» del §F0-02, applicato, rompe il link — FOWLER, NYGARD

> «Required baseline: `Core CoreUObject Engine InputCore EnhancedInput AIModule GameplayTags UMG Slate SlateCore`» + DoD: «`Build.cs` **minimo** e coerente» (F0-02)

Confronto con `RefactorTactics.Build.cs`:

- **`AIModule`**: il kit lo richiede, il repository **non ce l'ha** — e il bot esiste (`Bot/RTHexBotLibrary`).
  La baseline prescrive quindi una dipendenza che il codice ha dimostrato superflua.
- **`MeshDescription` / `StaticMeshDescription`**: assenti dalla baseline del kit, presenti nel repository
  con un commento che spiega perché toglierli **non rompe la compilazione ma rompe il LINK** — nove
  `LNK2019` su `FMeshDescription` e `FStaticMeshAttributes::Register`.
- **`AnimGraphRuntime`**: assente dalla baseline, presente per `FAnimNode_TwoWayBlend`/`FAnimNode_Slot`.

**FOWLER**: «"Minimo" non è una proprietà della lista, è una relazione fra la lista e ciò che il codice
referenzia. Il kit ha scritto la lista e chiamato *minimo* il proprio elenco, che è l'errore opposto:
prescrive un `AIModule` che nessuno usa e tace su tre moduli senza cui il modulo non linka.»

**NYGARD**: «Il modo di fallire è pessimo. Non è un errore di compilazione con una riga e un file: è un
link error a valle, su simboli di engine, con un messaggio che non nomina la causa. Il repository ha già
pagato questa diagnosi una volta e l'ha scritta nel commento; un esecutore del kit la ripagherebbe.»

📝 **Riscrittura**: `la lista delle dipendenze è quella corrente in RefactorTactics.Build.cs, che porta in commento la ragione di ciascuna riga non ovvia. Non ridurla per estetica: verifica prima che nessun simbolo la referenzi, e ricorda che il modo di fallire è LNK2019, non un errore di compilazione.`
🎯 **Priorità**: alta — è una delle prime cinque issue che il §12 indica di implementare.

### 🟠 F-04 · Il §16 è scritto come divieto, ma sul repository di oggi è una lista di rimozione — CRISPIN

> «Claude NON deve usare F0 come scusa per implementare: … `Overwatch completo` `Fast Reaction online` `multilevel final` `environment propagation final` `noise/perception` `full replay browser`» (§16)

Su un progetto a F0 è un guardrail corretto. Su questo repository, diverse voci vietate hanno già un
sorgente e dei test:

| Voce vietata dal §16 | Misura |
|---|---|
| `noise/perception` | `Source/RefactorTactics/Perception/`, `RTKnowledgeView.h` |
| `Overwatch` / reazioni | `RTReactionLibrary`, `RTReactionWindowView`, `RTReactionOpportunityTypes` |
| `Fast Reaction` | pin `CLAUDE.md` §2: **baseline 3,0 s**, timeout `HOLD` |
| `replay` | `Source/RefactorTactics/Replay/`, `RTReplayManifest`, `RTReplayRecorderLibrary` |
| `multilevel` | `RTCellId.h:24` — semantica dei layer già definita |

Il §16 non causa danno se letto come *«non allargare lo scope»*. Lo causa se letto come **stato**: un
esecutore che lo prenda per una descrizione del presente conclude che quel codice non dovrebbe esserci.

**CRISPIN**: «Un elenco di out-of-scope invecchia esattamente al contrario di un elenco di requisiti:
il requisito non fatto resta vero, il divieto scaduto diventa **falso e pericoloso**. Questo qui non ha una
data né una condizione di uscita, e la voce che mi preoccupa di più è `noise/perception` — c'è una
directory intera che lo implementa, con i suoi test.»

📝 **Riscrittura**: `l'out of scope della v0.1 è quello del piano canonico (P1) e del DoD. Questa sezione non lo ridefinisce. Un divieto senza data va verificato contro Source/ prima di essere applicato.`
🎯 **Priorità**: media — il danno richiede una lettura letterale, ma è la lettura che il §20 «EXECUTE NOW» incoraggia.

### 🟠 F-05 · Nessuna delle condizioni di uscita è un comando, e il §20 ordina di eseguirle comunque — ADZIC, WIEGERS

Il §18 elenca dieci condizioni di *Done*; il §17 sette gate; il §21 un flusso di 13 passi. Nessuno dei tre
porta un comando. Il repository invece ne ha uno solo, documentato: `./scripts/rt-suite.ps1`.

Il caso peggiore è il §F0-28 «Golden Gate», la cui exit criteria include *«Automation: command line green»*
senza dire quale command line, e *«Determinism: repeated corpus zero divergence»* senza dire quale corpus —
mentre nel repository esistono `Source/RefactorTactics/Tests/Golden/` e **97 scenari** con il loro `expect`.

**ADZIC**: «`Determinism: repeated corpus zero divergence` non è verificabile: non nomina il corpus, non dice
quante ripetizioni, non dice cosa si confronta. Un criterio del genere si dichiara verde per stanchezza. La
forma eseguibile esiste già in casa — uno scenario con il suo `expect` — e il kit non la usa perché non sa
che c'è.»

**WIEGERS**: «Dieci condizioni di Done al §18 e sette gate al §17, con sovrapposizione parziale e nessuna
mappa fra le due liste: sono **due definizioni di done** nello stesso documento. Il repository ne ha una,
`v0.1-definition-of-done.md`, e ha ragione lei.»

📝 **Riscrittura**: `ogni criterio di uscita è un comando con il suo esito atteso. La suite è ./scripts/rt-suite.ps1; il determinismo si verifica con uno scenario in Scenarios/ e il suo expect. La Definition of Done è quella di v0.1-definition-of-done.md (G1…G15): questa sezione non ne scrive una seconda.`
🎯 **Priorità**: media.

### 🟡 F-06 · Il grafo di dipendenze del §6 è la parte più leggibile del documento, e non è collegato a nulla — DOUMONT

Il §6 disegna in una cinquantina di righe l'ordine `F0-00 → … → F0-28`, e si capisce al primo sguardo cosa
blocca cosa. Ma i nodi sono identificatori che non esistono su GitHub (§2.2), quindi il grafo non è
navigabile: non porta a issue, non porta a milestone, non porta a codice.

**DOUMONT**: «È un buon diagramma di un albero che non c'è. La struttura merita di sopravvivere — mostrare
le dipendenze come grafo invece che come prosa — ma va ridisegnata sui nodi veri: `E<n>` e `G<n>`. Peraltro
[`roadmap-v0.1-v1.0.md`](../roadmap-v0.1-v1.0.md) lo fa già in Mermaid, e quello è cliccabile.»

📝 **Nota**: nessuna riscrittura. La capacità esiste già ed è migliore.

---

## 5. Cosa sopravvive al consumo

Tre cose. Sono **proposte al proprio owner**, non decisioni prese da questo referto.

### 5.1 La coda della distribuzione e i marker runtime — §14 e §F0-26

> «Non misurare solo la media. Per path e planning registra: `p50 p95 p99 max query count expanded nodes`» (§14)

⚠️ **Metà di questa prescrizione è già in casa, e va detto prima di proporla.**
`Tests/RTHexPerfTests.cpp` misura i KPI del PDR (`path < 2 ms`, `resolver < 100 ms/turno`) e implementa già
`MedianMs()` — con il commento *«mediana, non media: un picco non deve spostarla»*, che è esattamente
l'argomento del §14 — e `PercentileMs()` per nearest-rank. Il kit non insegna nulla su questo punto.

Il residuo, misurato, è più stretto e più preciso:

| Prescritto dal §14 / §F0-26 | Misura | Residuo |
|---|---|---|
| `p50` / mediana | `MedianMs()`, asserzione su entrambi i KPI | ✅ presente |
| `p95` / `p99` | `PercentileMs()` esiste ma è chiamata **una sola volta**, con **`P90`** | 🟠 la funzione c'è, i due percentili di coda **non sono asseriti** |
| `query count` · `expanded nodes` | `grep` su tutto `Source` | 🔴 **0 file** — non esistono |
| Marker Insights (`RT.Path.Query`, `RT.Resolver.Total`, …) | `TRACE_CPUPROFILER` · `SCOPE_CYCLE_COUNTER` · `DECLARE_CYCLE_STAT` · `CSV_` | 🔴 **0 occorrenze in tutto `Source/`** |

⚠️ **E anche il residuo ha già un owner, cercato dopo la prima stesura di questa sezione.**
[#84](https://github.com/DegrassiAaron/refactor-tactics-main/issues/84) *«CP 12.4 — KPI misurati e
registrati»* è **OPEN**, `P0`, epic [#26](https://github.com/DegrassiAaron/refactor-tactics-main/issues/26),
milestone **`v0.1 · Gate di release`** — e il suo DoD dice già, testualmente:

> «Il numero è un **percentile del frame time — p99** e non la media mobile di `stat fps`: un `60` medio
> con p99 a 22 ms passa il gate e fallisce col giocatore.»

È l'argomento del §14, scritto meglio del §14, con un esempio numerico che il kit non ha. La stessa issue
chiede *«Profiling allegato nella sede dichiarata dal DoD»*.

**Il residuo effettivo si riduce a due metriche**, che non compaiono né in `Source/` né in `#84` né
nella sua epic: **`queries/frame`** ed **`ExpandedNodes`**. Sono la grandezza che spiega *perché* la coda
si allunga durante l'hover — utile, ma è un dettaglio dentro una voce già aperta, non una lacuna.

📝 **Nessuna issue da aprire.** L'azione proporzionata è al massimo una riga in
[#84](https://github.com/DegrassiAaron/refactor-tactics-main/issues/84) quando quel checkpoint verrà
lavorato. `SEARCH → REUSE / UPDATE → CREATE solo per gap reale` ([`CLAUDE.md`](../../../CLAUDE.md) §7):
qui il gap reale è di due metriche, e l'owner esiste.

⚠️ **Questa sezione ha sbagliato due volte prima di arrivare qui**, ed è la stessa specie di errore che il
`F-02` rimprovera al kit: alla prima stesura dichiarava i percentili «il contributo più solido del
documento» senza aver letto `RTHexPerfTests.cpp`; alla seconda dichiarava `F0-26` «lacuna reale senza
issue» senza aver cercato su GitHub. Entrambe le volte la correzione è venuta da una misura, non da un
ripensamento — ed entrambe le volte ha ridotto il credito del kit.

### 5.2 Il contratto Snapshot / TurnLog — §15

> «Runtime event: Stable IDs, enum/reason code, valori compatti, **niente testo UI nel hot path**.
> Testo e spiegazione: `TurnEvent → Presentation formatter → UI`» (§15)

Coincide con l'invariante di casa (*la simulazione decide, la UI mostra*) e ne aggiunge la conseguenza
operativa: il punto in cui la stringa entra è il **formatter**, non l'evento. È una riga che rende
falsificabile una regola altrimenti astratta — si può cercare `FString` nel resolver e avere una risposta.

### 5.3 La classificazione del non-lavoro — §5 e §10

Il kit obbliga a etichettare ogni candidata con `REUSE / UPDATE / PARTIAL_OVERLAP / DUPLICATE / STALE /
CLOSED_BUT_REOPEN_REQUIRED / CREATE`, e ad aggiungere `ALREADY_DONE` quando il codice invalida la
candidata (§20). È **la stessa capacità** che il referto del 2026-08-29 aveva isolato come contributo
(`§23-D`, *«Issue NON create perché duplicate»*), arrivata per una via indipendente. Due kit distinti che
convergono sulla stessa lacuna sono un argomento per renderla formato stabile dell'output di audit.

Applicata a sé stessa — è ciò che fa la §2.3 — questa classificazione dà il verdetto del §1: **23 candidate
su 29 sono `ALREADY_DONE`**, 2 sono gate già coperti, 2 sono processo, 1 è N/A per modello di turno, e
**1 è `PARTIAL` con owner già aperto**.

⚠️ **La classificazione vale se la si applica fino in fondo, e fino in fondo fa male al kit.** Applicata a
metà, `F0-26` sembrava «la lacuna che salva il documento»: zero marker di profiling è un fatto vero e
verificato. Cercato l'owner — che è il passo che la classificazione impone e che questo referto aveva
saltato — [#84](https://github.com/DegrassiAaron/refactor-tactics-main/issues/84) copre la materia con un
DoD migliore del §14. Resta un residuo di **due metriche**.

È la lezione del `F-02` applicata a chi scrive il referto: *«il goal è raggiunto e la specifica non ha modo
di saperlo»* vale anche per chi giudica la specifica, se non cerca prima di concludere.

---

## 6. Cosa questo referto non ha fatto

- **Nessuna issue** creata, chiusa, modificata o commentata. Il §20 del kit lo ordinava; è task documentale.
- **Nessun Epic** creato o riallineato.
- **Nessuna build, nessuna suite, nessun PIE**: `NOT RUN`. Il referto non tocca codice, quindi non ha un
  verde da dichiarare — e non ne dichiara nessuno per conto d'altri.
- **Nessun `.uasset` aperto.**
- **Nessuna verifica dei gate `G1…G15`**: il referto legge lo stato che l'owner dichiara, non lo rimisura.
- **Nessuna misura sui 3 commit di scarto** da `origin/main`: i conteggi di test e sorgenti sono un limite
  inferiore.

---

## 7. Provenienza

| | |
|---|---|
| **Sorgente** | `CLAUDE_RT_Roadmap_1.0_and_v0.1_Execution.md`, untracked in radice, 1960 righe (`wc -l`) |
| **Archiviato in** | [`../../archive/src/handoff/2026-08-31-roadmap-1.0-v01-execution-prompt.md`](../../archive/src/handoff/2026-08-31-roadmap-1.0-v01-execution-prompt.md) |
| **Precedente della stessa famiglia** | [`roadmap-issues-v01-v10-spec-panel-2026-08-29.md`](roadmap-issues-v01-v10-spec-panel-2026-08-29.md) |
| **Panel** | Wiegers · Adzic · Cockburn · Fowler · Nygard · Crispin · Doumont · Porter |
