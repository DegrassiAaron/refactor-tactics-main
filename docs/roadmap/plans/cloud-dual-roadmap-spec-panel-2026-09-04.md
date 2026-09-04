# `cloud-dual-roadmap.md` (Dual Track Forge) — il terzo contratto operativo che non sa di esserlo — spec panel

> `CURRENT` · **Stato**: revisione chiusa, tre emendamenti consegnati agli owner · **Data**: 2026-09-04
> **HEAD della revisione**: `8d0a880a` (= `origin/main` al 2026-09-04, dopo `fetch`)
> **Oggetto**: la proposta esterna *RefactorTactics — Cloud Dual-Roadmap Contract* (nome breve *Dual Track
> Forge*), presentata per il path `docs/roadmap/cloud-dual-roadmap.md`, letta **come contratto di processo**
> e non come documento di roadmap.
> **Panel**: Fowler · Wiegers · Nygard · Adzic · Cockburn · Crispin
> **Modo**: critique · **Focus**: requirements, architecture
> ⚠️ **Il documento revisionato non è nel repository e non entra**: questo referto è l'unico posto in cui
> il suo contenuto resta citabile.

---

## 1. Il verdetto in una riga

La proposta diagnostica un problema reale e scrive **una regola che oggi nessun owner possiede** — non si
giudica un asset nello stesso processo che l'ha scritto — ma la incarta in un contratto di processo che
riscrive `AGENTS.md` con altri nomi, e la sua sezione sulla precedenza fra fonti **propone la scala che
[`D-282`](../../decisions/RT_PDR_00_Decision_Log.md) ha esplicitamente scartato**.

| | Rilievo | Chi | Gravità |
|---|---|---|---|
| **R1** | è il **terzo** contratto di processo, e non nomina mai i due che esistono | Fowler | 🔴 |
| **R2** | §2 «Fonti e precedenza» propone una scala universale — vietata da `D-282` — e omette i cinque owner di governance | Wiegers | 🔴 |
| **R3** | dice *«suite verde»* dove il repository pretende *«VALIDA»*, e conia tre sinonimi di `NOT RUN` | Nygard | 🔴 |
| **R4** | `G1` ed `E2` sono etichette già occupate: gate della DoD ed epic | Cockburn | 🟠 |
| **R5** | il protocollo asset ignora ownership concorrente e Binary Asset Lease | Nygard | 🟠 |
| **R6** | `DONE` dipende da *«tutti i check richiesti»* e il selettore di «richiesto» non esiste | Wiegers | 🟠 |
| **R7** | quindici sezioni di regole, **zero** esempi percorsi | Adzic | 🟠 |
| **R8** | il capability probe non distingue *assente* da *vuoto* | Nygard | 🟡 |
| ~~**R9**~~ | ~~le sedute con `verifies: []` sono un `EVIDENCE_GAP` misurabile~~ — 🔴 **smentito misurando**: su 46 sedute i gap reali sono **zero**, vedi §5 | Crispin | — |
| **R10** | §4 è un catalogo in prosa senza owner, e il path proposto contraddice la prima riga del documento | Crispin | 🟡 |

**Del testo si salva il ~15%**, ed è consegnato agli owner correnti: §6.

---

## 2. Come è stata misurata

Il checkout `refactor-tactics-main` era **169 commit indietro** a inizio revisione. Ogni citazione qui sotto
è letta con `git show origin/main:<path>` dopo `git fetch`, non dal working tree — la revisione è stata
scritta in un worktree pulito su `8d0a880a`.

```bash
git -C <repo> fetch origin
git -C <repo> show origin/main:AGENTS.md
git -C <repo> show origin/main:CLAUDE.md
git -C <repo> show origin/main:docs/decisions/RT_PDR_00_Decision_Log.md | grep -n 'D-282'
git -C <repo> show origin/main:docs/roadmap/editor-sessions.yaml > /tmp/es.yaml
grep -cE '^  - id:' /tmp/es.yaml            # 46 sedute
grep -oE '^    execution_lane: .*' /tmp/es.yaml | sort | uniq -c   # 33 pie · 10 asset · 3 senza lane
grep -cE '^    verifies: \[\]' /tmp/es.yaml # 10
```

⚠️ **Il primo conteggio dei `verifies` vuoti in questa revisione è stato sbagliato**, e la forma dell'errore
è quella che §5 racconta: `grep -c "verifies: \[\]"` **senza ancoraggio** conta anche la prosa che cita la
stringa e restituisce `21`. Con `^    ` sono **10**. Il numero pubblicato è il secondo.

---

## 3. Cosa la proposta prende giusto

Verificato, non concesso per cortesia:

| Claim della proposta | Esito | Dove |
|---|---|---|
| `roadmap-editor.md` fu ritirata perché terza vista da tenere allineata a mano | ✅ | header del file: *«VISTA RITIRATA IL 2026-08-08 … era la **terza** vista di stato»*. Sfumatura omessa: tornò *generata* il 2026-08-10, e la vista uscì di nuovo con `D-181` |
| `roadmap-v0.1-v1.0.md` è vista di navigazione, non owner | ✅ | *«**Tipo**: vista di navigazione, non owner … Se questa pagina e un owner divergono, **ha ragione l'owner**»* |
| `test-manuali-pie.md` è registro, non backlog | ✅ | *«QUESTO FILE È UN CATALOGO, NON UN BACKLOG, e l'unico insieme che ha una data è `RELEASE-V01`»* |
| executor ≠ oracle | ✅ | `scenario-map.md` §2: *«La distinzione fra A e B non è il tipo di file … ma **dove sta l'oracolo**»* |
| la migrazione `C → B → A` passa per l'owner canonico, non per una dichiarazione | ✅ | coerente con `scenario-map.md`, che tiene `C` come classe legittima e non come debito |

🔑 **E una regola che nessun owner scrive oggi**: *non si giudica un asset nello stesso processo che l'ha
scritto*. Cercata in `AGENTS.md` §7/§9 e in `CLAUDE.md` §5: non c'è. È il contributo che giustifica il
lavoro, ed è ciò che questa revisione consegna ad `AGENTS.md` (§6).

---

## 4. I rilievi

### R1 — 🔴 Il terzo contratto di processo (Fowler)

`AGENTS.md` (530 righe) si apre con *«Contratto operativo condiviso per coding agent nel repository»*;
`CLAUDE.md` (372 righe) è l'overlay Claude e in prima riga dice *«**Prima regola:** leggere `AGENTS.md`…
non duplica il contratto condiviso»*. La proposta non li nomina mai. Sovrapposizione misurata:

| Dual Track Forge | Esiste già in |
|---|---|
| A0 *Resolve Owner* + preflight | `AGENTS.md` §8 *Prima* · `CLAUDE.md` §1 *Context protocol* (10 passi) |
| A2 vincoli di implementazione | `AGENTS.md` §8 *Durante* — *«non creare una seconda source of truth»* |
| A3/A4 build e suite | `AGENTS.md` §9 *Build e test* |
| A5 *Evidence Pack* · §9 *Handoff* | `CLAUDE.md` §9 *Output dopo ogni pass* — Risultato · File · Decisioni · Verifiche · **NOT RUN** · Rischi · Prossimo passo |
| B1/B2 lifecycle MCP | `CLAUDE.md` §5 *Lifecycle Editor / MCP* (9 passi) · `AGENTS.md` §9 *Editor / PIE tramite MCP* (8 passi) |
| §8 stati finali | `AGENTS.md` §10 *Definition of Done* (16 voci) |

La §1.1 della proposta vieta di ricreare una vista di stato mantenuta a mano. Il difetto che introduce è
**della stessa specie sull'asse del processo**: due descrizioni della stessa procedura divergono al primo
aggiornamento di una delle due, e le tre PR chiuse senza merge citate da
[`DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) sono il precedente di cosa costa.

### R2 — 🔴 La §2 propone la scala che `D-282` ha scartato (Wiegers)

[`D-282`](../../decisions/RT_PDR_00_Decision_Log.md) (**Accettata**, 2026-08-30): *«La precedenza fra fonti
è **TIPIZZATA** per genere di affermazione e relativo owner: **non esiste una scala universale**»*, e fra le
alternative scartate: *«comporre le formulazioni in una scala unica totale — **scartata**, ordinerebbe
oggetti non confrontabili e produrrebbe risposte sbagliate **con sicurezza**»*.

La proposta presenta una lista numerata `1…9` sotto il titolo **«Fonti e precedenza»**. Letta come ordine di
lettura è innocua; il titolo dice altro, e chi la applica in conflitto fa ciò che `D-282` vieta.

L'omissione pesa più della scala. Confronto con `AGENTS.md` §2:

| `AGENTS.md` §2, prime cinque voci | Nella proposta |
|---|---|
| `docs/product/piano-canonico-mvp.md` | ❌ |
| `docs/decisions/RT_PDR_00_Decision_Log.md` | ❌ |
| ADR applicabili | ❌ |
| `docs/DOC_CONFLICT_MATRIX.md` | ❌ |
| `docs/OPEN_DECISIONS.md` | ❌ |

Conseguenza diretta: lo stato `DECISION_REQUIRED` che la proposta introduce **non ha un registro**, mentre
[`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) è quel registro e porta già il proprio protocollo — *«quando
una voce qui si chiude, diventa una `D-0xx` lì e **qui resta barrata**»*.

### R3 — 🔴 «Verde» non è un verdetto qui (Nygard)

`scripts/rt-suite.ps1` distingue quattro esiti: `0` VALIDA senza fallimenti · `1` VALIDA con test rossi ·
`2` non avviata (motore occupato o lock condiviso) · `3` **NON VALIDA**. `AGENTS.md` §9 lo scrive come
regola: *«Una misura è valida soltanto se osserva lo stesso `HEAD`, working tree, binario, stato del motore
dall'inizio alla fine. Se cambiano: **NON VALIDA**. Non equivale a verde»*, più *«dopo una lunga attesa,
**ricompila prima di registrare** il risultato»*.

La proposta scrive *«suite pertinente verde»* (A4, B3) e mette `Automation:` nell'Evidence Pack **senza un
campo di validità**. Un `CODE_READY` firmato su una run `NON VALIDA` è il falso positivo che il resto del
testo esiste per impedire. Nello stesso rilievo: il repository ha già la parola per *non eseguito* —
**`NOT RUN`** — e la proposta ne conia tre sinonimi (`EVIDENCE_GAP`, `MCP_BLOCKED`, `USER_REQUIRED`) senza
mapparli su di essa.

### R4 — 🟠 `G1` ed `E2` sono già occupati (Cockburn)

*«B3 — **G1** CLEAN GATE»*: in [`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) **G1 = «Build
Editor + Game Development + Game Shipping senza warning nuovi»**, primo di quattordici gate (G9 = subset
`RELEASE-V01`, G13 = giocabile senza editor). *«B4 — **E2** MEGA ACCEPTANCE»*: `roadmap-v0.1.md:637` →
**«E2 — Parità hex del substrato · P0»**.

Il «G1 clean gate» della proposta *contiene* il vero G1 e ci aggiunge altro: chi li confonde non sbaglia di
poco, sbaglia di perimetro.

### R5 — 🟠 Il protocollo asset presume una macchina a uso esclusivo (Nygard)

B3 dice *«chiudere Unreal»*, *«riaprire un processo Editor pulito»*. Su questo disco convivono **11 worktree
registrati** (`git worktree list`) e **19 directory `rt-wt-*`**. `CLAUDE.md` §5 va nella direzione opposta:
*«non modificare binari posseduti da un'altra sessione»*, *«verifica se esiste già un'istanza Editor … e
**chi la possiede**»*, *«non terminare un Editor preesistente posseduto da un altro workflow/persona»*, e
prescrive il **Binary Asset Lease** — che la proposta non nomina mai, pur dedicando una fase intera (B2)
agli asset binari. Un build con un Editor aperto su un'altra copia esce per Live Coding; una seconda run
automation resta ferma senza errore. B3 come scritto fallisce, e fallisce in silenzio.

### R6 — 🟠 Manca il selettore di «richiesto» (Wiegers)

`DONE` richiede *«all required `EDITOR_ACCEPTANCE` checks»*, e §4 dice *«Cloud deve selezionare solo quelli
pertinenti»*. Un catalogo di ~90 voci con selettore soggettivo produce due esecuzioni diverse sullo stesso
work item: non è testabile, quindi non è un requisito. Il repository ha già risolto questa domanda:
`test-manuali-pie.md` conta **209 voci, 132 non verdi**, e dichiara che **l'unico insieme con una scadenza
sono le 17 voci `RELEASE-V01`** del gate G9 — *«letto come backlog, diverge»*. La §4 riapre quel difetto in
forma nuova: un secondo catalogo, senza subset datato.

### R7 — 🟠 Zero esempi (Adzic)

Nessun passaggio è mostrato su un caso reale; le tabelle §10 sono vuote e i cinque work item DevSandbox
della §14 sono nominati, non percorsi. Un protocollo senza un esempio end-to-end non è verificabile: non si
sa se due esecutori producono lo stesso esito, che è l'unica proprietà per cui vale la pena scriverlo.

### R8 — 🟡 Il capability probe non ha failure mode (Nygard)

B1 conclude `MCP_UNAVAILABLE → USER_ROUTE` da una risposta negativa, senza distinguere *assente* da *vuoto*.
Il ponte MCP di questo progetto non è sempre acceso, e alcune query rispondono `[]` o `False` **senza
errore** quando l'Editor è in play mode: un elenco vuoto sembra una misura. Un probe eseguito nel momento
sbagliato spinge lavoro verso la persona senza motivo — l'opposto dell'obiettivo dichiarato al §12.
⚠️ **Provenienza**: rischio operativo noto dalla pratica su questo progetto, **non rimisurato** in questa
revisione. È consegnato a `CLAUDE.md` §5 come regola, non come misura.

### R10 — 🟡 Catalogo senza owner, e path che contraddice l'incipit (Crispin)

Novanta voci ridicono a mano ciò che vive in `test-manuali-pie.md` (209 voci), `scenario-map.md` (le quattro
classi) ed `editor-sessions.yaml` (46 sedute), senza una riga che citi l'owner: fra un mese sono false.
`scenario-map.md` §7 mostra l'alternativa — pubblica **i comandi per rimisurare** i propri conteggi.
E il path: `docs/roadmap/` è la casa delle roadmap, mentre la prima riga del documento dice *«non è una
roadmap»*. Il filesystem vince.

---

## 5. Il rilievo che si è smentito misurando

La proposta cita, come difetto reale del repository, le sedute *«che producono un verdetto ma hanno
`verifies: []` e nessun artifact»*, e lo eleva a caso d'uso dello stato `EVIDENCE_GAP`.

Misurato su `8d0a880a`, la catena è questa:

| Passo | Esito |
|---|---|
| sedute totali | **46** |
| con `verifies: []` (ancorato) | **10** |
| di lane `asset` o senza lane | **5** — `U10`, `U12`, `U17`, `U24`, `U27`: una seduta di *authoring* non ha voci PIE per costruzione |
| di lane `pie`, **con la ragione scritta accanto** | **3** — `U33`, `U34`, `U38`: *«Nessuna voce `PIE-*`: questa seduta NON entra in PIE … ciò che verifica sta nell'editor **prima** del Play»* |
| di lane `pie`, senza motivazione | **2** — `U40` (issue `#1992`), `U41` (issue `#1993`, `OPEN`) |
| di queste, senza **nessuna** casa per il verdetto | **0** — `#1992` porta il commento *«✅ Verdetto di leggibilità — 2026-09-02»*, scritto **prima** della chiusura |

🔑 **Il gap è zero, e la ragione è il selettore.** Il campo `verifies` nomina le voci `PIE-*`: non è il posto
dove vive la casa del verdetto, che può essere la **issue owner**. Un'assenza in `verifies` misura
l'assenza di una voce PIE, non l'assenza di evidenza — e chi cerca il difetto con quel campo lo trova dove
non c'è. È lo stesso errore di metodo che questa revisione ha commesso al primo conteggio (§2).

⛔ **Conseguenza sul piano di consegna**: la voce in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) prevista
dalla raccomandazione iniziale **non è stata scritta**, e nemmeno la micro-patch alle due sedute. Non c'era
niente da registrare: quel registro è *«ciò che aspetta una persona»*, e qui non aspetta nessuno.

---

## 6. Cosa è stato consegnato agli owner

Il delta reale della proposta, portato dove il processo ha già un proprietario:

| Regola | Owner che la riceve |
|---|---|
| `EDITOR_AUTHORING` ≠ `EDITOR_ACCEPTANCE`; l'asset caldo non si giudica; una nuova apertura si giustifica solo se cambia una precondizione; dove vive il verdetto | `AGENTS.md` §9, nuova sottosezione *Authoring e acceptance* |
| capability probe prima di affidare un passo a MCP, e *un vuoto non è un no* | `CLAUDE.md` §5, *Lifecycle Editor / MCP* |
| la regola dell'asset caldo come decisione esplicita, con ciò che **non** decide | Decision Log, `D-330` |

Non consegnato, e perché:

- **le fasi A0–A6 / B0–B7**: sono `AGENTS.md` §8–§10 con altri nomi (R1);
- **la scala di precedenza §2**: vietata da `D-282` (R2);
- **il catalogo §4**: senza owner e senza subset datato (R6, R10);
- **gli stati `EVIDENCE_GAP` / `MCP_BLOCKED` / `USER_REQUIRED`**: `NOT RUN` con il motivo li copre già (R3);
- **la voce in `OPEN_DECISIONS.md`**: la premessa non regge alla misura (§5).

---

## 7. La tensione che resta aperta nel panel

**Adzic** vorrebbe che ogni voce del catalogo §4 diventasse uno scenario `Given/When/Then` migrabile verso la
classe `A`. **Cockburn** obietta che metà di quelle voci non hanno né attore né goal — *«HUD leggibile»* è
una proprietà, non un caso d'uso — e che formalizzarle produrrebbe scenari verdi che non misurano ciò che
affermano. Il canone sta con Cockburn: `scenario-map.md` tiene `C` come classe permanente. Il §12 della
proposta (migrazione `C → B → A`) è giusto come **strategia** e sbagliato come **aspettativa di
svuotamento** — e la differenza è esattamente quella fra `test-manuali-pie.md` letto come registro e letto
come backlog.
