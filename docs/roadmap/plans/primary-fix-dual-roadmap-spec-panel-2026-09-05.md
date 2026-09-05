# «Primary Fix» — doppia roadmap implementazione / validazione Editor — spec panel

> `CURRENT` · **Stato**: revisione chiusa. Il sorgente è **recensito, non applicato** · **Data**: 2026-09-05
> **HEAD della revisione**: `8a530c6e` — `git rev-list --left-right --count HEAD...origin/main` → `0 0`,
> cioè **identico a `origin/main`** dopo `fetch`.
> ⚠️ **Il branch non è mio**: il checkout è `refactor-tactict-dev`, il branch corrente è
> `chore/rt-terminals-multi-istanza` e a inizio sessione era `fix/1793-2167-posa-e-offset`. È cambiato sotto
> la run. Nulla è stato committato.
> **Oggetto**: il mandato *«Primary Fix: doppia roadmap implementazione e validazione Editor»*, fornito in
> chat **senza sha né data**, che chiede due roadmap nuove, una matrice di tracciabilità e sette issue `PFV-*`.
> **Panel**: Wiegers (lead) · Fowler · Nygard · Crispin · Adzic · Cockburn
> **Modo**: critique · **Focus**: requirements, architecture, testing
> ⚠️ **Il documento non è nel repository e non entra**: questo referto è l'unico posto in cui il suo
> contenuto resta citabile.
> 🔑 **Nessun numero qui è ricordato.** Ogni conteggio porta il comando che l'ha prodotto, in §2.

---

## 1. Il verdetto in una riga

Il mandato è **tecnicamente competente e metodologicamente severo** — le sue dieci tesi sono quasi tutte
vere — ma **non è eseguibile come mandato**: il programma che dice di estendere **non esiste con quel nome**,
le due premesse tecniche che lo giustificano **sono chiuse da 12 e da 32 ore**, e la Roadmap B che chiede di
creare **esiste già e dichiara la propria ripartizione**.

| | Rilievo | Gravità |
|---|---|---|
| **R1** | *«la roadmap Primary Fix già presente»* e *«l'epic Primary Fix già esistente»*: **zero occorrenze** della stringa nel repository, **zero** issue con quel titolo. Entrambe le premesse sono false — ma il **programma** esiste, e si chiama [`#1881`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881) | 🔴 |
| **R2** | è il **sesto giro dello stesso contratto in 48 ore**: cinque referti già in `docs/roadmap/plans/`, e il mandato non ne nomina **nessuno** | 🔴 |
| **R3** | le due premesse tecniche portanti sono **chiuse su `main`**: `A2` è [`#2272`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2272) (CLOSED 2026-09-04 11:02Z), il caso **2-vs-10 celle** è [`#2370`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2370) (CLOSED 2026-09-05 00:00Z) | 🔴 |
| **R4** | la **Roadmap B esiste già**: [`roadmap-esecuzione-pie-2026-09-03.md`](roadmap-esecuzione-pie-2026-09-03.md) occupa esattamente il ruolo descritto — *«il registro degli esiti resta `test-manuali-pie.md`; quale voce e quando resta `editor-sessions.yaml`; questo file dice in che ordine e cosa manca»* | 🔴 |
| **R5** | **capability MCP: zero.** Tre porte a `HTTP 000`, nessun tool `mcp__unreal*` esposto, **nessun processo `UnrealEditor` vivo**. Il *«Probe MCP obbligatorio»* è `NOT RUN` e l'intera classificazione `AUTO-MCP`/`SEMI-MCP` non è misurabile oggi | 🔴 |
| **R6** | la famiglia `PF-AC-*` / `PFV-*` **non esiste**, e il registro PIE è l'owner degli ID delle verifiche manuali (**223** voci). Introdurla crea una seconda tassonomia — è il rilievo `R7` del triage di ieri, sulla famiglia `PACE-*` | 🟠 |
| **R7** | `A7`, `V6` e `V8` (rete, privacy, packaged, multiclient) sono **fuori dalla v0.1** per `roadmap-v0.1.md:2093` — *«**M10** Rete e privacy · Fuori dalla v0.1 (resta post-release)»*. Pianificarli dentro un programma v0.1 sposta lo scope senza dichiararlo | 🟠 |
| **R8** | la tesi 7 userebbe il criterio sbagliato su `editor-sessions.yaml` e **retrocederebbe un file canonico**: consumer eseguibili **zero**, ma `AGENTS.md:384` lo prescrive come owner del batching | 🟡 |
| **R9** | il mandato dichiara di lavorare in `DegrassiAaron/refactor-tactics-main`: corretto come *remote*, ma **tre checkout** dello stesso repository sono vivi su questa macchina, e il mandato non dice quale | 🔵 |

**Cosa si salva, e vale la run**: **tre** contributi reali che nessun owner possiede oggi, più un
difetto trovato per strada in `scenario-map.md` — §6.

**Raccomandazione del panel**: **non creare nulla.** Usare `#1881` come epic, `roadmap-esecuzione-pie` come
Roadmap B, e portare i tre contributi di §6 ai loro owner esistenti.

---

## 2. Come è stata misurata

```bash
git fetch -q origin && git rev-parse --short HEAD                    # 8a530c6e
git rev-list --left-right --count HEAD...origin/main                 # 0  0
git branch --show-current                            # chore/rt-terminals-multi-istanza (non mio)

# R1 — la premessa nominale, con DUE metodi che concordano
grep -ril "primary fix" --exclude-dir=.git .                         # conteggio: 0
rg -il "primary fix"                                                 # nessun file
rg -il "PF-AC-|PFV-[0-9]"                                            # nessun file
gh issue list --search "Primary Fix in:title" --state all            # []
test -f docs/roadmap/roadmap-primary-fix.md                          # ASSENTE
test -f docs/roadmap/roadmap-primary-fix-validation.md               # ASSENTE

# R3 — le premesse tecniche
gh issue view 2272 --json state,closedAt   # CLOSED · 2026-09-04T11:02:17Z
gh issue view 2370 --json state,closedAt   # CLOSED · 2026-09-05T00:00:36Z

# R5 — capability MCP, misurata invece che presunta
for p in 8765 8767 8770; do curl -s -o /dev/null -w "%{http_code}" -m 3 \
  "http://127.0.0.1:$p/mcp"; done                                    # 000 · 000 · 000
Get-CimInstance Win32_Process -Filter "Name like '%Unreal%'"         # nessun processo

# R6 — il registro, coi grep ancorati che scenario-map.md §7 prescrive
grep -c '^| \*\*PIE-' docs/technical/test-manuali-pie.md                                 # 223
grep -c '^| \*\*PIE-[A-Za-z0-9.-]*\*\* `RELEASE-V01`' docs/technical/test-manuali-pie.md #  17
grep -cE '^  - id:' docs/roadmap/editor-sessions.yaml                                    #  48

# R8 — consumer di editor-sessions.yaml
grep -rn "editor-sessions" tools/ scripts/ .github/                  # vuoto (0 eseguibili)
grep -n  "editor-sessions" AGENTS.md                                 # 384 (owner del batching)
```

⚠️ **Il totale del registro si è mosso oggi**: il triage di stamattina misurava **216** voci e **46** sedute,
qui sono **223** e **48**. `RELEASE-V01` è invariato a **17**. Non è un errore di nessuno dei due: è il
repository che si muove mentre lo si misura, ed è la ragione per cui questi numeri portano il comando.

⛔ **Non eseguiti, e non stimati**: `./scripts/rt-suite.ps1`, qualunque build, qualunque PIE.
[`#2397`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2397) e
[`#2409`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2409) — le due `StandStill` omonime
che rompevano la unity build stamattina — risultano **CLOSED** (07:21Z e 08:17Z), ma *«issue chiusa»* non è
*«albero costruibile»*: la compilazione a `8a530c6e` è `NOT RUN`. Questo referto non ne ha bisogno, perché
non tocca codice.

---

## 3. Le dieci tesi, una per una

Il mandato chiede esplicitamente questo verdetto. Nessuna è stata concessa per cortesia.

| # | Tesi | Esito | Evidenza |
|---|---|---|---|
| **T1** | *«tutti i check possibili» non è un criterio finito* | ✅ **confermata, e già risolta** | Il selettore esiste ed è `RELEASE-V01`: **17** voci su **223**. Il gate di release non chiede la copertura totale |
| **T2** | *una sola sessione Editor alla fine è troppo tardi* | 🟠 **vera, ma diagnostica la causa sbagliata** | Misurato in `roadmap-esecuzione-pie`: per la seduta più ricca (**U42**, 21 voci) le issue bloccanti sono **zero** — *«il collo di bottiglia non è l'implementazione: è che nessuno convoca la seduta»*. Aggiungere smoke gate per issue non tocca quella causa |
| **T3** | *il successo di un comando MCP non è prova di gameplay* | ✅ **confermata, già canonica** | `CLAUDE.md` §5: *«una risposta vuota non è un no»*; `scenario-map.md` separa **chi esegue** da **dove sta l'oracolo** |
| **T4** | *l'Editor non prova tutto* | ✅ **confermata, già canonica** | `CLAUDE.md` §6 impone `NOT RUN` con motivo; `scenario-map.md` classifica per oracolo |
| **T5** | *l'automazione non prova tutto* | ✅ **confermata** | Classe **B** di `scenario-map.md`: *«la macchina esegue, l'umano giudica»*; classe **C** = *«solo input umano»*, le voci che nessuno scenario può sostituire. ⚠️ **Il totale di C non si cita**: vedi §6 · **C4** |
| **T6** | *PIE ≠ shipping* | ✅ **confermata, e già scritta in una issue** | [`#589`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/589) (CLOSED) dice esattamente che il canary di **M10.3** *«va eseguito packaged»* |
| **T7** | *`roadmap-editor.md` storica · `test-manuali-pie.md` canonico · `editor-sessions.yaml` solo con consumer* | ⚠️ **due terzi confermati, il terzo è un criterio sbagliato** | Vedi **R8** e §4 |
| **T8** | *le prove devono essere riproducibili (commit, build, mappa, seed)* | ✅ **confermata, ed è un gap reale** | Il registro PIE ha **cinque** colonne — ID · domanda · allestimento · osservazione attesa · stato. **Non ha** un campo *ultima esecuzione*: sha e date esistono **in prosa** dentro le celle, non come campo. Vedi §6 · **C3** |
| **T9** | *non correggere bug durante una sessione di validazione* | ✅ **confermata** | È la separazione fra *«file modificato»* e *«file verificato»* che `AGENTS.md` impone |
| **T10** | *la privacy non si verifica guardando la UI* | ✅ **confermata, ma fuori scope v0.1** | `roadmap-v0.1.md:2093` mette **M10** fuori dalla v0.1. Owner già esistenti: [`#759`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/759), [`#577`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/577) |

**Nessuna tesi richiede una decisione nuova.** Otto sono confermate e già canoniche, una (**T2**) è vera ma
punta alla causa sbagliata, una (**T7**) è per un terzo un criterio che danneggerebbe il repository.

---

## 4. I rilievi che cambiano la consegna

### R1 — «Primary Fix» non esiste, ma il programma sì

`grep -ril "primary fix"` sull'intero albero dà **conteggio 0**, `rg -il` concorda, e
`gh issue list --search "Primary Fix in:title" --state all` risponde `[]`. Il mandato costruisce sette issue
`PFV-*`, due roadmap e una famiglia di acceptance ID sopra un nome che il repository non ha mai usato.

🔑 **Il programma però esiste, e ha già un'epic owner**:
[`#1881 — [EPIC] Resolution Playback & Inspection — bot, replay e debug fino alla v1.0`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881),
`OPEN`, etichette `v0.1` · `epic` · `P1`. Copre replay, playback e ispezione — cioè `A2`, `A4`, `A5` e `A6`
del mandato. Aprire *«[Primary Fix] Timeline autoritativa…»* creerebbe **la seconda epic concorrente**, che è
esattamente ciò che il mandato vieta al proprio §«Regola di non duplicazione».

### R3 — le due premesse tecniche sono chiuse

| Item del mandato | Issue reale | Stato |
|---|---|---|
| `A2` — *«terza coordinata … cutoff per valore … fail-closed»* | [`#2272`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2272) *«lo stato si ricostruisce a due coordinate mentre il seek ne indirizza tre: `UnitsAtPosition` accetti il micro-step»* | 🔴 **CLOSED** 2026-09-04 11:02Z |
| `PF-AC-TIME-001` / `A6` / campagna `V1` — *«un percorso di 2 celle termina prima di uno di 10»* | [`#2370`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2370) *«durata movimento per cella e ingresso-cella sul micro-step canonico»* | 🔴 **CLOSED** 2026-09-05 00:00Z |

Il titolo di `#2272` **è** l'enunciato di `A2`, parola per parola. E il caso 2-vs-10 celle è insieme il
**criterio di uscita n. 3** del mandato e la **fixture obbligatoria** della sua campagna `V1`: era chiuso
prima che il mandato fosse scritto. Il triage di stamattina aveva già registrato lo stesso fatto contro un
altro work order — *«la premessa tecnica su cui poggia è caduta la notte scorsa»*.

⚠️ **Questo non azzera l'area**: [`#2403`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2403)
(`OPEN`, `P1`, `v0.1`) — *«una caduta non lascia una catena causale: il replay non distingue primario,
alternativa e fallback»* — è lavoro vero e vicino a `A4`/`V5`. Ma è **una issue con un owner**, non un
programma da otto work item.

### R4 — la Roadmap B esiste già, e lo dichiara

Il mandato definisce `roadmap-primary-fix-validation.md` come *«un piano di esecuzione e copertura, non una
seconda fonte dello stato dei test»*. Quel documento esiste:
[`roadmap-esecuzione-pie-2026-09-03.md`](roadmap-esecuzione-pie-2026-09-03.md), il cui header dichiara la
ripartizione a tre in una riga sola — registro degli esiti a `test-manuali-pie.md`, *quale voce e quando* a
`editor-sessions.yaml`, **ordine e mancanze** a sé stesso.

∴ Creare il file richiesto significa produrre **il quarto documento** che si contende quel ruolo, ed è
letteralmente l'ultimo criterio di uscita del mandato: *«non esistono documenti operativi concorrenti con
stati incompatibili»*.

### R5 — la Roadmap B non ha capability, oggi

Misurato, non presunto: `8765` (config del repository), `8767` (config di sessione) e `8770` (il ponte di un
altro worker, vivo stamattina) rispondono tutte `HTTP 000`; nessun tool `mcp__unreal*` è esposto a questa
sessione; `Get-CimInstance Win32_Process` non trova **alcun** processo `UnrealEditor` su questa macchina —
solo shell `rt-terminal` sui tre checkout.

Il mandato prescrive correttamente *«non lasciare che il piano dipenda da un MCP non installato»*. Applicando
la sua stessa regola: **ogni** check `AUTO-MCP` e `SEMI-MCP` nascerebbe oggi come `HUMAN`, e il *«Probe MCP
obbligatorio»* è `NOT RUN` per **assenza di ponte** — non per prudenza.

### R8 — il criterio sbagliato su `editor-sessions.yaml`

Il mandato scrive: *«va usato solo se esiste un consumer reale e documentato: il solo fatto che il file
esista non lo rende fonte di verità»*. Il sospetto è sano; la misura lo smentisce nel modo interessante:

- consumer **eseguibili**: `grep -rn "editor-sessions" tools/ scripts/ .github/` → **vuoto**;
- consumer **normativo**: `AGENTS.md:384` — *«quali sedute condividano un allestimento è già dichiarato in
  `docs/roadmap/editor-sessions.yaml`, campo `shares_setup_with`»*;
- e `roadmap-editor.md`, che il mandato cita correttamente come ritirata, **nomina quel file come proprio
  successore**: *«le sedute vivono in `editor-sessions.yaml`, che **resta**»*.

∴ Il criterio corretto non è *«esiste un tool che lo legge»* ma *«esiste un owner documentale che lo
prescrive»*. Applicato alla lettera, il criterio del mandato retrocederebbe un file che `AGENTS.md` rende
obbligatorio, e **48** sedute perderebbero la loro sede.

---

## 5. Fonte → ruolo → stato → azione

La tabella che il mandato chiede, misurata.

| Fonte | Ruolo reale | Stato | Azione |
|---|---|---|---|
| `docs/technical/test-manuali-pie.md` | **Registro degli esiti** delle verifiche manuali · 1727 righe · 223 voci | `CURRENT`, canonico | ✅ **Confermato** come il mandato assume. Nessuna riga aggiunta da questa run |
| `docs/roadmap/editor-sessions.yaml` | **Pianificazione** delle sedute · 48 sedute, campo `shares_setup_with` | `CURRENT`, prescritto da `AGENTS.md:384` | ⚠️ **Il mandato lo declasserebbe**: vedi **R8**. Nessuna azione |
| `docs/roadmap/roadmap-editor.md` | Provenienza del metodo | 🗄️ `HISTORICAL`, ritirata **2026-08-08** | ✅ Il mandato ha ragione. **Non riattivare** |
| [`roadmap-esecuzione-pie-2026-09-03.md`](roadmap-esecuzione-pie-2026-09-03.md) | **Ordine e mancanze** delle sedute | `CURRENT` | 🔴 **È la Roadmap B richiesta.** `REUSE` |
| [`#1881`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881) | **Epic owner** di replay / playback / ispezione | `OPEN` · `v0.1` `epic` `P1` | 🔴 **È l'epic richiesta.** `REUSE` |
| [`#1880`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1880) + `D-310` | Schema micro-step e rifiuto fail-closed | `D-310` **Accettata** 2026-09-01 | ✅ Il mandato cita `D-310` correttamente in `A3`. Owner già assegnato |
| `docs/decisions/adr-0009-replay-logico-canonico.md` | Contratto del replay — *«chi riproduce non calcola»* | `CANONICAL`, Accettato | ✅ È l'owner di `A1`. **Non serve un contratto nuovo** |
| `docs/roadmap/roadmap-v0.1.md:2093` | Perimetro di release | `CURRENT` | 🟠 Mette **M10** fuori dalla v0.1 → `A7`/`V6`/`V8` sono post-release |
| `docs/technical/tooling/scenario-map.md` | **Ripartizione** automatico/umano per oracolo (classi A/B/C/D) | `CURRENT` | 🔑 **Owner della classificazione** che il mandato vuole reinventare. Vedi §6 · **C1** |
| `docs/roadmap/roadmap-checkpoint.md` | Priorità | `CURRENT` · 532 righe | Nessuna azione: il mandato non la contraddice |
| `docs/technical/runbooks/debug-vs-unreal.md` | Come si lancia una sessione | `CURRENT` · 118 righe | Nessuna azione |
| `scripts/rt-suite.ps1` | Suite locale · 1443 righe | `CURRENT` | ⛔ `NOT RUN` in questa sessione |

---

## 6. Cosa si salva — tre contributi, più un difetto trovato per strada

`C1`–`C3` vengono dal mandato: non sono nel repository oggi, e valgono la lettura. `C4` no — è un difetto di
un documento owner, emerso verificando una delle dieci tesi.

### C1 — la quinta modalità: `PACKAGED/MULTICLIENT` 🔑

`scenario-map.md` classifica per **dove sta l'oracolo**, in quattro classi: **A** automatico · **B**
automatico + occhio · **C** solo umano · **D** dichiarato/bloccato. Le prime tre coprono le prime tre
modalità del mandato (`AUTO-MCP` ≈ A, `SEMI-MCP` ≈ B, `HUMAN` ≈ C).

⚠️ **La quinta modalità del mandato non ha classe**: *«deve uscire dall'Editor o usare più processi/client»*
è una proprietà **ortogonale** all'oracolo, e oggi una voce così finisce in **D** (*dichiarato*), dove si
confonde con *«scenario non ancora scritto»*. La distinzione è reale — `#589` la incontra e la risolve a
mano, in prosa, dicendo che il canary M10.3 va **packaged**.

🔎 **E il repository ha già una domanda aperta esattamente lì**: `scenario-map.md` chiede se le `PIE-MUT-*`
debbano diventare una **classe E** — *«oracolo automatico, precondizione umana»* — e la dichiara *«aperta,
non decisa qui»*. La quinta modalità del mandato è la **seconda candidata** a quella classe, e arriva con un
caso d'uso già incontrato.
**Owner**: `scenario-map.md`. **Azione**: portare la proposta lì, non in una roadmap nuova.

### C2 — «criterio di falsificazione scritto prima di aprire l'Editor»

Il registro PIE ha una colonna *osservazione attesa*. Non ha il suo complemento: **cosa si sarebbe visto se
la proprietà fosse rotta**. La differenza non è retorica — questo repository porta già il caso di un criterio
PIE scritto su una barra **già accesa**: verde senza aver provato niente.
**Owner**: `test-manuali-pie.md` (struttura della tabella). **Azione**: una decisione, non un documento.

### C3 — il campo «ultima esecuzione» ⚠️

La tabella del registro ha **cinque** colonne, e nessuna è *commit / data / operatore*. Gli sha e le date
esistono — ma **in prosa**, dentro le celle (*«il Task 5 è atterrato (`678cc8fc`)»*, *«misurato il
2026-09-04»*). Non sono interrogabili: nessun gate può chiedere *«quali voci verdi sono state misurate prima
dell'ultimo bump di formato?»*, che è la domanda che rende un verde **riutilizzabile** invece che storico.
**Owner**: `test-manuali-pie.md`. **Azione**: valutare un campo, col costo su 223 righe dichiarato prima.

### C4 — ✅ trovato per strada e **chiuso**: la classe C di `scenario-map.md`

Non viene dal mandato: è emerso verificandone la tesi **T5**.

🔴 **La prima stesura di questa voce sbagliava la diagnosi, e va detto perché l'errore è istruttivo.**
Diceva *«due totali per la stessa classe — 179 e 141, scarto 38, due rimisure che non si sono parlate»*, e
concludeva che serviva una riclassificazione dell'owner. **Falso.** Ricalcolando la tabella riga per riga:

```bash
python -c "v=[56,22,28,3,3,6,6,9,7,1,7,5,2,10,3,2,5,2,1,1]; print(sum(v))"   # 179
# ...e i totali PROGRESSIVI:
#   dopo la riga 10 → 141      dopo la riga 17 → 175      dopo la riga 20 → 179
```

Il **141** non era una misura rivale: era **la somma parziale delle prime dieci righe**, cioè il fossile di
uno stato precedente che la nota di rimisura del 2026-09-03 documenta esplicitamente (*«dichiarava 141 su
quattordici righe… da 141 a 175»*). Tabella e riga `C` della §2 **concordavano** a `179`. Stantia era solo
l'**intestazione** della sezione, che nessuno aggiornò quando la tabella fu rifatta.

🔑 **La lezione, che vale più del numero**: due cifre che differiscono non sono automaticamente due misure in
conflitto. Prima di convocare l'owner conviene chiedersi se una delle due sia un **prefisso** dell'altra —
qui la somma progressiva lo dimostrava in una riga di Python, e la mia prima lettura aveva saltato il
controllo più economico.

**Ma la verifica ha trovato un difetto vero, più in basso**: il *secondo* metodo che §7 dichiara —
`totale − PIE-VIS − PIE-MUT` — dà `223 − 28 − 2 = **193**`, contro le `179` della tabella. I due metodi
avevano smesso di misurare la stessa popolazione, perché la tabella contava **10** voci `E34` che essa
stessa etichettava *«classe D travestita da C»*.

✅ **Chiuso il 2026-09-05, su decisione dell'utente**, con due interventi in `scenario-map.md`:

| Intervento | Effetto |
|---|---|
| Il totale **esce** dall'intestazione di §5 e resta solo sotto la tabella | Un numero derivato mantenuto in **due** sedi era andato alla deriva quattro volte in tre settimane; ora la sede è **una** |
| Le dieci `PIE-STATE-*` (E34) passano da **C** a **D**, in una **§6.4** nuova | Classe C: `179 → 169`. La classe D acquisisce una **seconda popolazione** — voci di registro accanto agli scenari — e il documento dichiara che le due **non si sommano** |

⚠️ **Il metodo diretto di §7 è stato dichiarato non più valido** per la classe C invece che aggiustato in
silenzio: assumeva che ogni voce non-`VIS` non-`MUT` fosse umana, e da oggi dieci non lo sono. Resta uno
scarto di **14** che il documento **dichiara aperto** anziché riconciliare a intuito.

---

## 7. Matrice REUSE / UPDATE / CREATE

⛔ **Nessuna riga è stata eseguita.** Questa è la raccomandazione del panel, non un registro di azioni.

| Deliverable chiesto dal mandato | Verdetto | Perché |
|---|---|---|
| `docs/roadmap/roadmap-primary-fix.md` (Roadmap A) | ⛔ **NO ACTION** | Il lavoro che coprirebbe è chiuso (`#2272`, `#2370`) o già posseduto (`#1881`, `#2403`, `#1880`, `ADR-0009`) |
| `docs/roadmap/roadmap-primary-fix-validation.md` (Roadmap B) | 🔁 **REUSE** → [`roadmap-esecuzione-pie-2026-09-03.md`](roadmap-esecuzione-pie-2026-09-03.md) | Stesso ruolo, già dichiarato nel suo header |
| Epic *«[Primary Fix] Timeline autoritativa…»* | 🔁 **REUSE** → [`#1881`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881) | Epic owner esistente, `OPEN`, stesso perimetro |
| Famiglia ID `PF-AC-*` | ⛔ **NO ACTION** | Il registro possiede gli ID delle verifiche manuali: **223** voci `PIE-*` |
| `PFV-00` — capability probe MCP | 🚫 **BLOCKED** | Zero ponte, zero Editor: sarebbe una issue che nessuno può eseguire oggi |
| `PFV-01` — movimento / timing / boundary | ⛔ **NO ACTION** | `#2370` **CLOSED**; le voci PIE esistono |
| `PFV-02` — seek / resume / archive / viewer | 🔁 **REUSE** → `PIE-V01-REPLAY` · `PIE-REPLAY-ARCHIVE` · `PIE-V01-REPLAY-VIEWER` | Tutte presenti nel registro |
| `PFV-03` — Tactical Designer e UX | 🔁 **REUSE** → `PIE-SCEN-PLAYBACK` | Presente nel registro |
| `PFV-04` — reazioni / osservatori / privacy | 🔁 **REUSE** → [`#759`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/759) · [`#577`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/577) | Owner M10 esistenti, e M10 è post-release |
| `PFV-05` — performance / packaged / multiclient | 🚫 **BLOCKED** + 🔁 **REUSE** → [`#589`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/589) | Fuori dalla v0.1 per `roadmap-v0.1.md:2093` |
| `PFV-06` — regressione integrata e sign-off | 🔁 **REUSE** → subset `RELEASE-V01` (**17** voci) | Il gate di release esiste, ed è il suo selettore |
| Righe nuove nel registro marcate `NOT RUN` | ⛔ **NO ACTION** | Nessuna proprietà nuova è emersa che il registro non copra già |
| **C1 · C2 · C3** (§6) | ✅ **CREATE**, ma come proposte agli owner | Sono l'unico contenuto del mandato senza sede attuale |
| **C4** (§6) — la classe C di `scenario-map.md` | ✅ **UPDATE eseguito** su `scenario-map.md` | Non viene dal mandato: emerso verificando **T5**, e **chiuso** il 2026-09-05 su decisione dell'utente. È l'**unica** modifica che questa sessione ha scritto fuori da questo referto |

---

## 8. Cosa questa run NON ha fatto

- ⛔ **Nessuna issue creata, aggiornata o chiusa.** `gh` è autenticato (`gh auth status` ✓) e non è stato
  usato in scrittura: `/sc:spec-panel` revisiona, non applica.
- ⛔ **Nessun commit, nessun push.** Il branch corrente appartiene a un'altra sessione.
- ⛔ **Nessuna roadmap creata**, per il motivo che è il verdetto di §1.
- ✅ **Una sola eccezione, ed è dichiarata**: `docs/technical/tooling/scenario-map.md` è stato modificato per
  chiudere **C4** (§6), su decisione esplicita dell'utente presa dopo la consegna di questo referto. Non
  discende dal mandato — discende dalla verifica della sua tesi **T5**.
- ⛔ **`NOT RUN`**: build, `rt-suite.ps1`, PIE, packaged, multiclient, e il probe MCP che il mandato dichiara
  obbligatorio.
