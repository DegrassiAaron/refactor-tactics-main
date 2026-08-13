# Quattro processi paralleli — triage del consolidamento

> `CURRENT` · **Stato**: revisione chiusa, residuo applicato · **Data**: 2026-08-14
> **HEAD della revisione**: `ce9e4365`, poi riallineato a `84cbb70c` — ⚠️ `origin/main` si è mosso di **9
> commit** *durante* la revisione (#836), e le misure di questo documento sono state rieseguite sull'albero
> unito, non incrementate. Vedi §7.1. · branch `docs/consolidamento-4-processi`
> **Sorgente revisionato**: `RefactorTactics_4_Process_Parallel_Roadmap_Claude_Consolidation.md`
> (1552 righe, 57 sezioni, untracked), archiviato a fine sessione in
> [`../../archive/src/RefactorTactics_4_Process_Parallel_Roadmap_Claude_Consolidation.md`](../../archive/src/RefactorTactics_4_Process_Parallel_Roadmap_Claude_Consolidation.md)
> **Scopo**: classificare ogni sezione contro il repository **prima** che qualcuno la applichi a ownership,
> roadmap, tassonomia, gate o codice.
> **Regola applicata**: un handoff è l'ultima fonte della gerarchia. Dove contraddice una `D-nnn`, un gate
> definito o un fatto misurabile sul branch, prevale il repository e la proposta si **registra**.

---

## 1. Il verdetto in una riga

Il documento descrive un modello di processo corretto — **quattro processi paralleli, tre agenti e un umano
davanti all'Editor** — e lo indirizza a un repository più vecchio di sé stesso: chiede di ritirare sette file
di lane che sono stati **archiviati stamattina** (§3), propone un owner di classificazione che ne
**duplicherebbe uno già validato** (§21–§27), e riscrive per track una roadmap `v0.1→v1.0` che esiste già
tutta (§30–§39). Ciò che resta è piccolo e buono: **il write-set di batch e la lease sui binari Unreal** —
l'unico meccanismo che il repository non ha, per un problema che ha avuto **oggi stesso**.

Il documento contiene il proprio antidoto e va citato, perché è la lettura più giusta che se ne possa dare:
§22 chiede di *«verificare se il Feature Registry supporta già elegantemente»* prima di creare il file, e di
*«scegliere **un solo owner** della classificazione»*; §45 dice *«solo se non esiste un equivalente»*; §46
*«estendere l'infrastruttura, non costruirne una seconda»*. Questo triage è l'esecuzione di quei tre punti.

---

## 2. Il conto

| | Sezioni | Significato |
|---|---:|---|
| `CURRENT` | **17** | descrive come da fare una pratica che il repository ha già, spesso senza saperlo |
| `DUPLICATE` | **10** | riscrive contenuto che esiste, in un secondo posto |
| `CONFLICT` | **6** | creerebbe un secondo owner di una classificazione già validata |
| `WRONG` | **1** | l'affermazione è falsificata da un fatto misurabile sul branch |
| `PROPOSED` → applicato | **13** | idea nuova, nessun conflitto, entra in questa PR |
| `PROPOSED` → differito | **2** | idea nuova, ma il write-set è conteso: diventa issue |
| meta / procedura | **8** | audit, DoD, formato del report: eseguiti, non recepiti |

**57 sezioni.** Si rimisura con `grep -c '^# [0-9]' <sorgente>`.

Il rapporto ha la forma che i triage precedenti di questa cartella hanno già misurato: **il repository era
avanti alla fonte**, e il prodotto vero è ciò che il triage ha **impedito** di aggiungere — un file di
classificazione, quattro shortlist generate, un campo nel Feature Registry, un filtro nel Control Center e
dieci sezioni di roadmap.

---

## 3. `WRONG` — le sette lane sono già archiviate, da stamattina

La §3 apre con:

```text
Sono presenti:
docs/roadmap/plans/roadmap-lane-index.md
docs/roadmap/plans/roadmap_lane_1.md
...
docs/roadmap/plans/roadmap_lane_7.md
```

e chiede sei passi: cercare i riferimenti, non cancellare alla cieca, marcare `SUPERSEDED/HISTORICAL`,
aggiornare i link, mantenere la provenienza, sostituire la vista operativa.

**Nessuno di quei path esiste.** I file sono in
[`../../archive/roadmap-plans/`](../../archive/roadmap-plans/README.md), il cui README dichiara
`Archiviato il 2026-08-14` — cioè **lo stesso giorno** del sorgente. E i sei passi sono tutti già eseguiti,
con una precisione che il sorgente non chiedeva:

| passo chiesto dalla §3 | stato misurato |
|---|---|
| cercare i riferimenti | `grep -rln "roadmap_lane\|roadmap-lane-index" docs/ AGENTS.md CLAUDE.md` → **nessun owner li cita**, solo l'archivio e tre sorgenti |
| non cancellare alla cieca | i 7 file esistono, integri |
| marcare `SUPERSEDED/HISTORICAL` | banner `SNAPSHOT` · data · HEAD `59fa6f8a` |
| aggiornare i link | riga d'indice nel README dell'archivio |
| mantenere la provenienza | *«fotografate il 2026-08-12»* |
| sostituire la vista operativa | *«la topologia viva è [`execution-graph.yaml`](../../roadmap/execution-graph.yaml), lo stato il Feature Registry»* |

⚠️ **E il README dell'archivio ha già respinto la premessa della §3 con un argomento più forte di quello che
il sorgente usa**: *«Nessuno stato è stato riscritto per farli entrare qui. È la differenza fra archiviare e
dichiarare superato: la prima è una riorganizzazione, la seconda è un'affermazione su un documento.»* La §3
chiede di marcarli `SUPERSEDED` perché *«la nuova decisione cambia la premessa»* — che è esattamente
l'operazione che l'archivio ha deciso di non fare. Le lane non sono superate da un modello nuovo: erano
**già** una fotografia, e lo dichiaravano in testa.

🔴 **La lezione operativa non è che il sorgente è vecchio di un giorno: è che era vecchio di poche ore.**
Il sorgente è datato 2026-08-14 e dichiara come snapshot *«`main` successivo al consolidamento roadmap fino
a v1.0 (2026-08-13)»* — cioè è **onesto** sulla propria base. Ciò che non poteva sapere è che fra la sua base
e la sua lettura è atterrata `ce9e4365`. Nessuna cura nella scrittura del sorgente elimina questo:
**la fotografia si misura al momento in cui si applica, non a quello in cui si scatta.**

---

## 4. `CONFLICT` — un terzo vocabolario sugli stessi oggetti

Le §21–§27 chiedono, nell'ordine: quattro shortlist generate (`track-*.shortlist.md`), un file
`parallel-tracks.yaml` come owner della classificazione, un campo `work_tracks` nel Feature Registry, un
overlay su Roadmap/Scenario/Editor Map e un filtro `track` nel Control Center.

Il repository ha già **due** tassonomie ortogonali sugli stessi oggetti, entrambe in
[`../execution-graph.yaml`](../execution-graph.yaml), entrambe validate da
`feature_registry.py validate_execution_graph()`:

| tassonomia | valori | domanda a cui risponde |
|---|---|---|
| `execution_lanes` | `code` · `pie` · `asset` | **chi esegue**: un agente, oppure una persona davanti all'Editor |
| `domain_groups` | 8 gruppi sulle `area` del registry | **di cosa parla** il lavoro |

La copertura di `domain_groups` sulle aree è **totale ed esclusiva**, e il validator lo verifica: un'area
nuova non finisce in nessun gruppo in silenzio (`[execution-graph] area X del registry non appartiene a
nessun gruppo`). È la proprietà che un secondo file di classificazione non può avere senza duplicare il
controllo.

### Il quarto processo esiste già, e si chiama `pie` + `asset`

Il processo CONTENT/EDITOR della §7 — *«il lavoro davanti a Unreal Editor»* — è per definizione
`execution_lane: pie` (l'uscita è un verdetto) più `asset` (l'uscita è un asset committato). Le note del file
lo dicono con le stesse parole del sorgente: *«serve una persona davanti all'editor»*. Non c'è niente da
creare.

### I tre processi Claude NON sono una funzione di `domain_group`, ed è la misura che decide

Mappando gli 8 gruppi sui 3 track della §4/§5/§6:

| `domain_group` | aree | track secondo il sorgente |
|---|---|---|
| `core_simulation` | Core, Gameplay | simulation |
| `map_environment` | Map, Environment | spatial |
| `actions_reactions` | Actions, Reactions | simulation |
| `perception_network` | Perception, Networking | simulation |
| `objectives_match` | Objectives | simulation |
| `ui_presentation` | UI | client_tools |
| `characters_content` | Characters, Factions | ⚠️ **tre track** |
| `tooling_data_qa` | Tools, Data, Production | ⚠️ **due track** |

Due gruppi su otto **si spezzano**: la §5 assegna a SIMULATION *«il gameplay logico dei Character»*, la §6 a
CLIENT il *«presentation-side character code»*, la §7 a CONTENT *«gli asset Character»* — tre destinazioni per
un gruppo solo. Lo stesso per `tooling_data_qa`, diviso fra CLIENT (validators, scenario launcher, debug
panels) e CONTENT (cook validation, asset audit).

**Conseguenza:** il track non è derivabile dalla classificazione esistente. Sarebbe un attributo **per nodo**,
esattamente come `execution_lane` lo è già (`issue:171` lo dichiara, gli altri ereditano il default). Il che
riduce la proposta a *«aggiungere un campo facoltativo a un file che ce l'ha già in forma equivalente»*.

### E soprattutto: oggi nessuno lo legge

La catena di un dato è **dichiarato · trasportato · letto**. `parallel-tracks.yaml` + quattro shortlist + un
campo nel registry + un filtro nel Control Center costruisce i primi due anelli per un lettore che non
esiste: nessun gate, nessuna vista, nessuna persona chiede oggi *«quali feature sono spatial?»*. La domanda
che **viene fatta davvero**, e che oggi non ha risposta, è un'altra:

> Sto per aprire tre sessioni: **quali file può scrivere ciascuna senza sovrascrivere le altre?**

È la §8, non la §22. Ed è l'unica parte del cluster che entra.

---

## 5. `DUPLICATE` — la roadmap per track (§30–§39)

Dieci sezioni riscrivono `v0.1→v1.0` affettata per processo. Il contenuto è corretto — le epic citate
esistono tutte, `E37` è davvero chiusa ([#555](https://github.com/DegrassiAaron/refactor-tactics-main/issues/555))
e le **24 issue** nominate dalla §30 sono tutte **aperte e reali** (verificate con `gh issue view` il
2026-08-14) — ma è una seconda copia di
[`../roadmap-post-v0.1.md`](../roadmap-post-v0.1.md), organizzata per un asse che il §1.9 dello stesso
sorgente vieta di introdurre:

> *«Non creare un nuovo asse di milestone o gate. Restano le nomenclature correnti.»*

Una vista per track che descrive a parole cosa fa ogni processo in ogni release è **stato duplicato**: si
disallinea al primo checkpoint chiuso, e nessun `--check` se ne accorge perché non è generata da niente.
Ciò che la §30 chiede davvero — *«verificare live»* — è un'operazione, ed è stata eseguita: il risultato è
il batch della §7 qui sotto.

---

## 6. `CURRENT` — 17 sezioni descrivono pratiche già in vigore

| § | Chiede | Dove è già |
|---:|---|---|
| 2 | rispettare release ed epic correnti | ✓ verificato: `E1`–`E45`, epic GitHub [#14](https://github.com/DegrassiAaron/refactor-tactics-main/issues/14)–[#778](https://github.com/DegrassiAaron/refactor-tactics-main/issues/778) |
| 14 | Claude modifica asset solo tramite Unreal | `AGENTS.md` §Unreal: *«non spostarli da filesystem: Content Browser + Fix Up Redirectors»* |
| 15 | preservare vault/Fab e ignore policy | [`convenzioni-contenuti-ue.md`](../../technical/convenzioni-contenuti-ue.md) + `.gitignore` |
| 16 | le collisioni di ID sono un problema generale | `D-135`, sedici collisioni registrate nelle Note del Decision Log |
| 17 | `rt_shared_id.py` è già la soluzione per `D-nnn` | ✓ esatto — `reserve` · `check` · `audit-refs` |
| 19 | policy transitoria per namespace non supportati | `AGENTS.md` §Git: *«Restano a mano gli altri contatori — `Enn` e `XXX-n`: si verificano sul remote subito prima del merge»* |
| 20 | più cloni: `fetch` + `check` + `audit-refs` prima del merge | `AGENTS.md` §Git, [`workflow-parallel-claude.md`](../../technical/workflow-parallel-claude.md) §8 |
| 26 | EditorMap resta owner della coda umana | [`editor-sessions.yaml`](../editor-sessions.yaml) sorgente, [`editormap.shortlist.md`](../editormap.shortlist.md) vista generata |
| 28 | GitHub è già l'ID del task, niente `SP-01` | `execution-graph.yaml` usa `issue:<numero>` come identità dei nodi |
| 29 | nessun nuovo `Gxx` per il parallelismo | ✓ nessun gate nuovo introdotto |
| 44 | riallineare il tracking ecosystem | ✓ i 14 path verificati, nessuno incoerente (§8) |
| 46 | estendere l'infrastruttura, non costruirne una seconda | ✓ principio applicato: **nessun tool nuovo** in questa PR |
| 47 | le versioni serializzate sono a rischio superiore | già scritto **nel codice**: `RTReplayManifest.h:25` cita `#471`, `RTTurnLog.h` documenta perché la `v7` fu presa dopo aver controllato tutti i branch |
| 50 | sintesi *Parallel work* e *Shared IDs* in AGENTS/CLAUDE | già presenti; entra solo il blocco *Unreal binaries* + write-set |
| 52 | usare solo comandi realmente presenti | ✓ tutti e nove esistono e sono stati eseguiti |
| 54 | dodici cose da NON fare | ✓ tutte già regole del repository o di questo triage |
| 57 | ottimizzare il parallelismo reale, non il numero di branch | ✓ è il criterio con cui il batch della §7 è stato costruito |

---

## 7. `PROPOSED` — cosa entra, e perché proprio questo

### 7.1 Il write-set di batch (§8, §9, §10, §40, §41, §42, §43)

Il repository sa che **una sessione = un worktree** (`D-135`, `AGENTS.md`), e non sa altro: due worktree
possono editare lo stesso file, e l'unica difesa è che se ne accorga il merge.

🔴 **Il problema non è ipotetico: è successo mentre scrivevo questo documento.** Il piano iniziale prevedeva
di estendere `scripts/feature_registry.py` col validator della §48. Il write-set del branch
`docs/consolidamento-walls-doors-v1` ([PR #836](https://github.com/DegrassiAaron/refactor-tactics-main/pull/836)),
allora **aperto** e misurato con `git diff --name-only origin/main...<branch>`, contiene **16 file**, fra cui:

```text
scripts/feature_registry.py            ← il file che stavo per riscrivere
scripts/test_feature_registry_releases.py
docs/roadmap/feature-registry.yaml
docs/roadmap/feature-registry.json
docs/roadmap/roadmap-post-v0.1.md
docs/OPEN_DECISIONS.md
```

Sei file su sedici erano nel mio piano. **Il validator è quindi uscito da questa PR** ed è diventato una
issue: applicare la regola che si sta scrivendo è l'unica prova che valga qualcosa.

⚠️ **E poi #836 è atterrata mentre questa PR era aperta** — `84cbb70c`, 22:56 UTC — portando su `main` **9**
commit. Il vincolo che aveva prodotto resta valido, ma la riga che lo descriveva è diventata falsa in
un'ora, in **quattro** documenti che dicevano «viva». È il limite del meccanismo, e va scritto invece che
scoperto: **un `preexisting` è una fotografia di `gh pr list`, non un lock.** Nessun gate lo vede — i link
risolvono, i totali tornano, e il documento descrive un repository che non c'è più. L'unica difesa è
rileggere `origin/main` **dopo ogni passo lungo e prima del merge**.

✅ **Il dividendo della previsione è però misurabile.** `git merge origin/main` ha dato **tre** conflitti, e
tutti e tre erano righe che questo ramo aveva già dichiarato contese in `integration_only`: il Decision Log
(risolto tenendo entrambe le decisioni, `D-138` e `D-139`, in ordine numerico) e i due totali degli archivi,
rimisurati col comando invece che scegliendo un lato — **72** e **52**, esattamente i valori previsti. Nessun
file di codice, nessuna vista generata, nessuna scelta arbitraria.

Ne segue la forma minima del meccanismo, in [`../parallel-batch.yaml`](../parallel-batch.yaml): un file che
descrive **il batch**, non lo stato delle feature — niente `status`, `done`, `progress`, conteggi di test.
Lo stato resta nel Feature Registry e su GitHub.

### 7.2 Binari Unreal: human-first, non human-only (§11, §12, §13, §14) → **D-139**

`AGENTS.md` oggi dice *«non modificare `.uasset`/`.umap` a mano»*, che è una regola sul **come** e non dice
niente sul **chi**. Il sorgente propone la distinzione giusta: il processo umano è **holder predefinito**,
e Claude può toccare un binario solo con una lease esclusiva. Registrata come `D-139`, con i quattro
vincoli che la rendono verificabile: chiave semantica legata a GitHub (mai `LEASE-001`), holder unico, path
esatti, validità legata al `base_sha`.

⚠️ **La §13 aggiunge la cosa che rende la lease necessaria invece che burocratica**: due `.uasset` non si
fondono. Un conflitto binario non è un conflitto di merge da risolvere — è una delle due versioni da
**rifare a mano dentro Unreal**. La lease non previene un conflitto: previene un lavoro perso.

🔴 **E la §13 ha ragione anche sul dettaglio che sembra pedante — misurato su questo repository.** Chiede di
verificare i *package correlati* prima di emettere una lease su un `.umap`. Una mappa non è un file: è una
**cartella**, e quante cose contenga non si deduce dal nome.

```text
git ls-files "Content/RT/Maps/Dev/L_DevSandbox/"
    Content/RT/Maps/Dev/L_DevSandbox/L_DevSandbox.umap
    Content/RT/Maps/Dev/L_DevSandbox/Data/DA_HexMap_Sandbox.uasset   ← 2 package
    ...
    Content/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena.uasset       ← 2 package
    Content/RT/Maps/Dev/L_Prototype/L_Prototype.umap                 ← 1 package, e basta
```

⚠️ **Due delle tre ne hanno un secondo, la terza no**, ed è precisamente ciò che rende la regola necessaria
invece che decorativa: se il numero fosse uniforme si potrebbe scriverlo nella lease una volta e non
guardare più. La prima stesura di questo triage aveva generalizzato a *«tre cartelle da due package»*
guardandone **due su tre** — lo stesso difetto di metodo che il §5 rimprovera al sorgente, commesso qui.

Una lease scritta sul solo `.umap` resta **incompleta per costruzione** dove il secondo package esiste, e lo
sarebbe stata anche seguendo alla lettera l'esempio della §12, che il path lo scrive piatto —
`Content/RT/Maps/Dev/L_DevSandbox.umap`, che nel repository **non esiste**. La regola operativa che ne esce è
di una riga, e vale in entrambi i casi: *una lease su una mappa si emette sulla **cartella**, non sul file*.

### 7.3 Audit dei namespace monotoni (§16, §18, §19, §47)

La §18 chiede una tabella reale. Misurata sul branch, non trascritta:

| Namespace | Globale | Serializzato | Owner | Policy oggi |
|---|:---:|:---:|---|---|
| `D-nnn` | sì | no | Decision Log | ✅ `rt_shared_id.py reserve D` (`D-135`) |
| `E-nn` (epic) | sì | no | `roadmap-post-v0.1.md` + GitHub | manuale, `gh issue list --search "EPIC in:title"` prima del merge |
| `CP x.y` | dentro l'epic | no | roadmap owner | manuale — ⚠️ **due spazi che collidono già per costruzione**: `CP 10.1` è «Activate e Interact» in `E10` e «listen server» in `M10` |
| `ADR-nnnn` | sì | no | `docs/decisions/` | manuale — `0001`…`0009`, prossimo `0010` |
| `XXX-n` (OPEN_DECISIONS) | sì | no | `OPEN_DECISIONS.md` | manuale, verifica sul remote |
| `ERTTurnLogFormatVersion` | sì | **sì** | `RTTurnLog.h:467` | **v7** (`WithPriority`) — audit forte, già praticato e documentato nel codice |
| `URTHexMapAsset::CurrentFormatVersion` | sì | **sì** | `RTHexMapAsset.h:70` | **v8** — in migrazione a `FCustomVersionRegistry` (`D-137`, [#687](https://github.com/DegrassiAaron/refactor-tactics-main/issues/687)) |
| `ERTReplayManifestVersion` | sì | **sì** | `RTReplayManifest.h:16` | **1** (`Initial`) — il commento cita già `#471` |
| `URTMatchFormatData::FormatVersion` | no (per-asset) | sì | `RTMatchFormatData.h:81` | default fisso `= 1`, difetto noto (`D-137` §⚠️) |
| `RTTestReportWriter::SchemaVersion` | sì | sì (report) | `RTTestReportWriter.h:25` | **1** |
| `schema_version` dei YAML | per-file | no | i rispettivi owner | **1** ovunque |

✅ **Il risultato dell'audit è che non serve estendere l'allocatore.** Dei quattro namespace serializzati,
tre hanno **una sola sorgente di scrittura** e il quarto è per-asset; la difesa che serve è quella già
scritta nei commenti — *controllare tutti i branch remoti prima di prendere il numero* — non un lock. E per
`E-nn`/`XXX-n` la decisione di tenerli a mano è **già presa e motivata** in `AGENTS.md`: *«deliberatamente
rimandata a dopo che `D-nnn` avrà dimostrato il meccanismo»*.

⚠️ **La riga più utile della tabella non è un rischio di collisione: è `CP x.y`.** I due spazi di
numerazione collidono **oggi**, per costruzione, e il Feature Registry lo documenta nel proprio header. Non
è un difetto da correggere — è un vincolo da non dimenticare quando si scrive un checkpoint.

### 7.4 `IDLE` è un esito valido (§40)

*«Il parallelismo minimizza wall-clock time, non massimizza branch count.»* Entra in
`workflow-parallel-claude.md` come regola, ed è applicata nel batch qui sotto: **due track su quattro sono
IDLE**, e non è un difetto del batch.

### 7.5 `workflow-parallel-claude.md` diventa owner tecnico (§51)

Il documento c'era e copriva worktree + ID condivisi. Ora copre anche i quattro processi, il write-set e la
lease binaria. Le garanzie di `D-135` sono preservate: nessuna riga delle §1–§10 esistenti è stata riscritta.

---

## 8. `PROPOSED` differito — cosa diventa issue, e perché

| § | Cosa | Perché non ora |
|---:|---|---|
| 48 | validator del parallelismo (7 controlli: doppia assegnazione, lease doppia, lease senza issue/`base_sha`, integration-only assegnato, track sconosciuto, destinazione duplicata) | il file da estendere — `scripts/feature_registry.py` — era nel write-set di [PR #836](https://github.com/DegrassiAaron/refactor-tactics-main/pull/836), aperta al momento del calcolo. → [#840](https://github.com/DegrassiAaron/refactor-tactics-main/issues/840) |
| 49 | test di batch e lease | seguono il validator |

Gli altri test della §49 non servono: quelli sugli **shared ID** esistono già
(`scripts/test_rt_shared_id.py`), e quelli sul **track mapping** presuppongono la classificazione respinta
al §4.

---

## 9. Il batch corrente, calcolato e non proposto

La §41 vieta di scegliere il primo batch senza verifica live. Eseguita: `git fetch --prune origin`,
`git worktree list`, `gh pr list --state open`, `gh issue view` sulle 24 issue della §30.

**Lo stato di partenza non era quattro track libere: era una track già occupata.** Il worktree
`rt-wt-walls-doors` lavorava `docs/consolidamento-walls-doors-v1` con un write-set di 16 file, **12** dei
quali coperti dalle categorie non assegnabili di questo batch — `integration_only` (7) e `generated_only`
(5) — e `wt-cap` tiene `Source/RefactorTactics/Tests/RTScenarioCorpusTests.cpp`.

⚠️ **Quel branch è poi atterrato** (§7.1), e il batch è stato riallineato a `84cbb70c`. La track resta
occupata finché il worktree esiste: `git worktree list` e `gh pr list` rispondono a due domande diverse, e
va guardata anche la prima.

Il batch è in [`../parallel-batch.yaml`](../parallel-batch.yaml). In forma leggibile:

```text
SPATIAL
  issue:            #41 — CP 3.3, misurazione dei budget su hex
  branch:           feat/41-kpi-hex
  writable:         docs/roadmap/v0.1-definition-of-done.md
                    Source/RefactorTactics/Map/, Source/RefactorTactics/Pathfinding/
                    Tests/RTHexMapTests.cpp, Tests/RTHexPathTests.cpp
  binary leases:    nessuna

SIMULATION
  issue:            #583 — il produttore della condizione dichiarata di D-109
  branch:           feat/583-produttore-d109
  writable:         Source/RefactorTactics/Turn/, Bot/
                    Tests/RTReactionTests.cpp
  binary leases:    nessuna
  ⚠️                ScenarioHarness/ TOLTA dal write-set: la tiene wt-cap (misurato)

CLIENT / REPLAY / TOOLS
  IDLE per DIPENDENZA, non per adiacenza di dominio.
  #170 (CP 15.4) dichiara «Dipende da: CP 15.3» = #512, aperta: il gate non è
  raggiungibile in questo giro. #77 (CP 11.1) tocca UI/ ma anche i test che le
  altre due track già scrivono.
  Source/RefactorTactics/Replay/ resta LIBERA e non assegnata.

CONTENT / EDITOR
  IDLE — la coda umana è ferma su un binario già conteso.
  #623 / U21 richiede una lease su Content/RT/Maps/Dev/L_DevSandbox/, che qui è DUE
  package (L_DevSandbox.umap + Data/DA_HexMap_Sandbox.uasset) — misurati, non dedotti, e
  Content/RT/Characters/Gadget/Animation/ è untracked nella working directory principale:
  finché non è committata o scartata, la destinazione non è prenotabile.
```

⚠️ **Due track su quattro sono IDLE, e questo batch è quindi da due processi, non da quattro.** È il §40
applicato alla lettera: il conto giusto non è quante track hai riempito, è quanti merge conflict non avrai.

### 9-bis. Il primo batch violava il proprio invariante, e l'ha trovato la code review

🔴 **La prima stesura assegnava `Source/RefactorTactics/ScenarioHarness/` a `simulation` mentre `wt-cap` la
stava riscrivendo.** Il write-set di quel branch era stato **dichiarato a memoria** — «tiene
`RTScenarioCorpusTests.cpp`» — invece che misurato col comando che la §12 di
[`workflow-parallel-claude.md`](../../technical/workflow-parallel-claude.md) prescrive tre righe sopra:

```powershell
git diff --name-only 84cbb70c...fix/capability-sconosciuta-non-e-blocked
```

Restituisce **sette** file, non uno. `WritableSet(simulation) ∩ WritableSet(wt-cap) ≠ ∅`, alla **prima
applicazione** del meccanismo, e per il difetto esatto contro cui il meccanismo esiste. Corretto togliendo
la directory e riportando i sette path misurati in `preexisting.holds`.

🔴 **E la regola su `generated_only` era insoddisfacibile.** Scritta come *«non si assegnano a nessuno»*,
lasciava chi tocca una sorgente senza uscite legali: né rigenerare (path non assegnato) né lasciar stare
(`--check` rosso). Il caso non è teorico — `wt-cap` possiede `RTScenarioSession.cpp`, sorgente delle
capability, e ha già rigenerato **due** viste (`scenariomap.shortlist.md` e `project-graph.json`),
correttamente. La regola è stata riscritta: *una vista non si assegna, **segue la propria sorgente***, e il
campo `derives_from` rende la catena verificabile invece che ricordata.

⚠️ **Le due correzioni successive di questa stessa regola sono la parte istruttiva.** La seconda stesura
diceva *«due track che alimentano la stessa vista: una esce dal batch»* — troppo stretta, perché
`project-graph.json` ha **dieci** sorgenti e quel caso è la norma. La terza la giustificava con
`Source/RefactorTactics/Tests/`, «sorgente delle viste per qualunque track scriva un test»: **falso, e
misurato** — toccare un test lascia `generate --check` e `shortlist --check` verdi, perché `TESTS_DIR` è
letto solo da `validate`. Quell'arco era stato dedotto da una **costante letta nel codice** invece che
dall'esperimento, cioè lo stesso difetto di metodo, al terzo giro, dentro la correzione del secondo.

Il metodo che funziona è uno solo, e costa un minuto: **sposta via la sorgente, rigenera, guarda cosa
cambia.** Applicato alle sei sorgenti candidate riproduce esattamente la tabella della code review — e per
`project-graph.json` non serve nemmeno, perché **l'artefatto pubblica le proprie sorgenti** nella chiave
`sources`. Un elenco scritto a mano accanto a uno che il generatore già dichiara è una seconda verità: è
precisamente ciò che questo triage ha respinto al §4.

⚠️ **Anche questa PR violava la regola che stava scrivendo**: modificava ~200 righe di
`workflow-parallel-claude.md` senza che il file fosse in nessuna categoria del batch. Aggiunto a
`integration_only`, dove stanno già `AGENTS.md` e il Decision Log.

Il valore di queste tre righe non è la correzione: è che **un meccanismo dichiarativo senza gate produce
esattamente i difetti che descrive**, e li produce nel documento che li descrive. È l'argomento più forte
per [#840](https://github.com/DegrassiAaron/refactor-tactics-main/issues/840), e non era disponibile prima
di provare a usarlo.

---

## 10. Cosa questo triage NON decide

- **Non riapre la classificazione per track.** Se in futuro un consumatore reale lo chiede — una vista, un
  gate, una persona che pone la domanda — la casa è `execution-graph.yaml`, per nodo, accanto a
  `execution_lane`. Non un file nuovo.
- **Non tocca il perimetro dei quattro processi.** Le §4–§7 sono descrizioni utili e sono state recepite in
  `workflow-parallel-claude.md` come *prosa*, non come dato: nessun elenco di path è stato copiato, perché
  è la parte che invecchia per prima.
- **Non estende `rt_shared_id.py`.** L'audit del §7.3 dice che non serve; se servirà, la condizione è
  scritta lì.
- **Non introduce CI.** Il repository ha scelto gate locali/manuali, e il sorgente lo riconosce (§52).
