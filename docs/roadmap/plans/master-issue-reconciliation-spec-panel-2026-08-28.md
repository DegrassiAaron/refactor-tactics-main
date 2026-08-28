# Master Issue Reconciliation (Claude Cloud) — spec panel

> `CURRENT` · **Stato**: revisione chiusa · **consolidamento applicato il 2026-08-28** (§9) · **Data**: 2026-08-28
> **HEAD all'inizio della revisione**: `f8ea244b` · branch `feat/220-slot-consuma-catalogo` · `origin/main` `6ffd59c0`
> 🔴 **HEAD si è spostato due volte DURANTE questa revisione** — `fa10e27a` e poi `c1a7cd9d`, entrambi
> intitolati «updatge» — e il secondo ha **committato i due sorgenti nel branch** insieme al lavoro vivo su
> `WBP_RT_ActionSlot` e `RTScreenHudWidgets`. Le misure qui sotto sono state prese a `f8ea244b`; nessuna di
> esse dipende dai file toccati da quei due commit.
> **Sorgenti revisionate**: `RefactorTactics_ClaudeCloud_Master_Issue_Reconciliation_2026-08-28.md` (1273
> righe) e `RefactorTactics_ClaudeCloud_Issue_Domain_Manifest_2026-08-28.json` (54 righe), arrivati untracked
> a radice e **committati nel branch da `c1a7cd9d` mentre la revisione era in corso**; archiviati a fine
> sessione in
> [`../../archive/src/handoff/2026-08-28-master-issue-reconciliation.md`](../../archive/src/handoff/2026-08-28-master-issue-reconciliation.md)
> e [`../../archive/src/handoff/2026-08-28-master-issue-reconciliation-manifest.json`](../../archive/src/handoff/2026-08-28-master-issue-reconciliation-manifest.json).
> **Scopo**: classificare ogni affermazione dei due documenti contro il repository **prima** di applicarla a
> issue, epic o roadmap — che è l'ordine che il documento stesso prescrive al §1.1 e al §2.
> **Regola applicata**: un handoff AI è l'ultima fonte della gerarchia ([`CLAUDE.md`](../../../CLAUDE.md) §7).
> Dove contraddice un ADR, una `D-nnn`, un gate o un fatto misurabile sul branch, prevale il repository.

---

## 1. Il verdetto in una riga

Il kit è **accurato su ciò che cita e cieco su ciò che è appena successo**: tutti e **77** i numeri di issue
che nomina esistono davvero, la ladder delle release e la tabella dei pivot di ADR-0008 sono **identiche** al
canone — ma il suo manifest si ferma a `#1430` mentre **72 issue** vivono sopra quella soglia, e **31 di esse
sono state aperte lo stesso giorno** da un kit gemello già consumato e archiviato. Eseguirlo alla lettera
violerebbe la sua stessa regola §1.1 «*search before create*».

Non è un difetto di scrittura: è ciò che accade a **ogni** fotografia datata. Il kit non poteva sapere.
Il difetto è che **non dichiara di essere una fotografia** e non porta il comando per riaggiornarla.

---

## 2. Ciò che è stato misurato

| Affermazione del kit | Esito | Misura |
|---|---|---|
| I path del preflight §2 esistono | ✅ **vero** | 14 su 14, incluso `docs/technical/systems/spec-graybox-placement-contract.md` |
| Ladder v0.1→v1.0 e temi (§1.2) | ✅ **identica** | i nove titoli di sezione di [`roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) combaciano parola per parola |
| Tabella pivot ADR-0008 (§6.1) | ✅ **identica** | `adr-0008` righe 46–49: Gadget 2/2 · Phase 2/3 · Riktor 1/0 · Wraith 3/3 · Stationary 3 |
| I numeri di issue citati esistono | ✅ **77 su 77** | 47 OPEN · 30 CLOSED. Nessun numero inventato, nessuna PR scambiata per issue |
| E20=`#217` · E25=`#265` · E39=`#704` · E40–E45=`#773`–`#778` · E46=`#934` · E47=`#952` | ✅ **vero** | verificato sull'elenco reale delle 52 epic |
| «`#265`–`#269` da rimisurare» (§5.1) | ✅ **corretto a rimisurarlo** | `#265` è l'epic, `#266`–`#269` i quattro checkpoint |
| Feature Registry / `parallel-batch.yaml` / shortlist non vanno reintrodotti (§1.4) | ✅ **vero** | `git ls-files` non li trova più; sopravvive solo un audit archiviato |
| `PlacementSector` · `CoverAnchor` · `ResolvedCoverState` · `OverwatchDirection` non esistono | ✅ **vero** | **0** occorrenze in `Source/`. I divieti §7.2 e §9.3 descrivono lo stato reale |
| Divergenza ADR-0008 / runtime (§6.2) | ✅ **confermata, e senza owner** | `MoveEndPivotMaxSteps` e `DashEndPivotMaxSteps`: **0** occorrenze in `Source/`. L'ADR è accettato, il cap per eroe **non è implementato** |
| Gap `ranked` (§21) | ✅ **reale e verificato** | [`roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) riga 52 promette «*Standard 3v3 ranked*» e riga 1126 dice «*Ranked e rating stanno qui*». Nel tracker, **prima di questa passata**: **0** issue con `ranked`, **0** con `MMR`, una sola con `matchmaking` (`#810`). ⚠️ **Non rimisurare questa cella e aspettarti `0`**: la passata stessa l'ha invalidata aprendo `#1604` (§9). È il senso della riga — il gap c'era, e ora ha un owner |
| Manifest: `CAMERA` → `known_issues: []` | 🔴 **falso per assenza** | esistono **7** issue `[Camera]` (`#729`, `#863`, `#864`, `#865`, `#873`, `#874`, `#887`) e sono **tutte CLOSED**: la camera base della v0.1 c'è già |
| §20 «auditare le epic E40–E45» | ⛔ **già eseguito oggi** | il kit gemello archiviato in [`2026-08-28-github-issues-roadmap-v01-v10.md`](../../archive/src/handoff/2026-08-28-github-issues-roadmap-v01-v10.md) le ha verificate: tutte OPEN, 40 checkpoint, ladder **già completa e non toccata** |
| Manifest completo dei domini | 🔴 **stantio il giorno stesso** | tetto `#1430`; sopra c'erano **72** issue (47 OPEN) alla misura del 2026-08-28, di cui **31** (`#1557`→`#1587`) aperte quel giorno stesso. ⚠️ Il numero **cresce**: è una fotografia, e rileggerla domani darà una cifra diversa — che è precisamente la tesi di questa riga |
| Awareness = `Hidden` / `Uncertain` / `Detected` (§10.5) | ⚠️ **tre su quattro** | il canone è `Nascosto` / `ContattoIncerto` / `Rilevato` **+ `UltimoContatto`** (persistenza 1 turno) — [`brief-conoscenza-parziale.md`](../../gameplay/brief-conoscenza-parziale.md) §63 |
| Stati UI `Confermato` / `Previsto` / `Incerto` (§4.3) | ✅ **esistono** | [`brief-delayed-actions.md`](../../gameplay/brief-delayed-actions.md) riga 42, con ADR-0004 §6–§7 come owner |

---

## 3. Il panel

### 📚 KARL WIEGERS — qualità e verificabilità dei requisiti

> «Il §27 è una Definition of Done di ventiquattro caselle, e **nessuna porta un criterio di misura**.
> "HUD roadmap ha owner completi" — completi rispetto a quale elenco? "Facing ADR-0008/runtime divergence ha
> un owner" è l'unica casella che si può falsificare, e infatti è l'unica che ho potuto verificare: il
> simbolo non esiste in `Source/`, quindi la divergenza c'è. Le altre ventitré si dichiarano vere
> leggendole.
>
> Il difetto strutturale è che il documento **prescrive alla passata di essere verificabile** (§26: "report
> con link GitHub reali") **senza rendere verificabile sé stesso**. Un DoD i cui criteri sono aggettivi
> produce un report di aggettivi.»

**Correzione richiesta**: ogni casella del §27 diventa un comando — `gh issue list --search`, `git grep -c`,
il nome del gate. Il criterio è il comando, non la frase.

### 🧭 ALISTAIR COCKBURN — attore primario e obiettivo

> «Chi è l'attore? Il documento dice "Claude Cloud". Ma l'attore reale è **una sessione fra molte**, e il
> repository lo sa: `D-222` dichiara sviluppo parallelo misurato — 101 checkout e 6 sessioni in un giorno.
> Il kit scrive "sincronizzare `origin/main`" al §2 e poi non ci torna mai più, per ventisette sezioni.
>
> L'obiettivo dichiarato — «*trasformare in tracking coerente tutto il lavoro descritto nelle chat*» — ha un
> presupposto invisibile: che le chat siano l'unica sorgente di lavoro non tracciato. Al momento in cui
> scrivo, **tre PR aperte** lavorano nei domini del kit: `#1531` è un altro spec panel su menu, mappa e
> scenario con cinque decisioni (`D-213`…), `#1532` rinomina `Action.Dodge` sotto `D-230`, e la working
> directory in cui il kit è arrivato sta implementando `CP 20.3` — che il kit stesso elenca al §5.1 come
> owner da auditare.»

**Correzione richiesta**: il §2 non è un preflight, è un **loop**. `git fetch` + `gh pr list` prima di ogni
dominio, non una volta all'inizio — la lezione che l'indice dei sorgenti archiviati ha già pagato tredici
volte: «*il controllo è scaduto DENTRO il lavoro*».

> 🔴 **E la dimostrazione è arrivata da sola, durante questa revisione.** `HEAD` era `f8ea244b` quando ho
> preso le misure; a fine sessione era `c1a7cd9d`, dopo due commit «updatge» di un'altra sessione nella
> **stessa working directory** — il secondo dei quali ha committato nel branch proprio i due file del kit,
> che erano untracked all'inizio. Un preflight eseguito una volta al §2 sarebbe stato falso per il resto
> della passata, e nessun gate se ne sarebbe accorto.

### 🔨 GOJKO ADZIC — esempi eseguibili

> «Il §24 mi dà quattro definizioni — `UPDATE_EXISTING`, `CREATE_NEW`, `LINK_ONLY`, `DECISION_REQUIRED` — e
> **zero esempi**. "Il nuovo scope è naturale estensione del suo DoD": mostrami due casi, uno che lo è e uno
> che non lo è, presi da questo repository.
>
> Peggio: il documento **contiene già i suoi esempi e non li usa**. Il §6.2 descrive un caso perfetto —
> ADR-0008 accettato, runtime divergente, `#291` esiste — e si ferma a "creare/aggiornare un owner
> esplicito". Quale dei due? Con quale criterio? La stessa sezione che pone il problema ha in mano il dato
> per risolverlo e non lo spende.»

**Correzione richiesta**: ogni categoria del §24 porta un caso concreto **preso dai 77 numeri che il kit già
cita**. Il materiale c'è; manca la decisione di usarlo.

### 🎲 MICHAEL NYGARD — modi di fallimento

> «Cosa succede quando una premessa del kit è falsa? Il documento non lo dice mai, e ho trovato il caso
> reale: il §20 chiede di auditare `E40`–`E45`. Quel lavoro è **stato fatto oggi**, dal kit gemello che dorme
> nella stessa cartella d'archivio. Un esecutore che segue il §25 in ordine arriva al punto 7 ("cross-release
> v0.5–v1.0") e rifà una passata conclusa poche ore prima — nella migliore delle ipotesi spendendo il tempo,
> nella peggiore aprendo il duplicato di `CP 27.4` (`#1569`) o riaprendo la domanda sull'illuminazione che è
> già registrata in `#327`.
>
> E c'è un failure mode più silenzioso: il manifest dichiara `"CAMERA": {"known_issues": []}`. Un array vuoto
> **non significa "non ho cercato"**: si legge come "non esiste owner". È la forma peggiore di sentinella,
> perché è anche una risposta valida. Sette issue Camera esistono, tutte chiuse, e il runtime possiede già la
> camera base che il §12 vieta di duplicare.»

**Correzione richiesta**: distinguere `[]` da `null` da «non misurato». E ogni dominio dichiara **la data
della propria misura**, non la data del documento.

### 🧪 LISA CRISPIN — come si valida la passata

> «Il documento chiede di non implementare codice (§0) e pretende un report verificabile (§26). Bene: con
> cosa lo verifico? Il kit non nomina **nessuno** degli strumenti con cui questo repository si misura — non
> `scripts/rt-suite.ps1`, che è l'unica via che dichiara se una suite VALE (`D-222`), non i sei gate di
> `tools/`, non la regola dei due numeri nel log di automation.
>
> Il §23 fa una cosa giusta e rara: vieta di scrivere "test verde" se il test non esiste, e impone `PLANNED`
> / `NOT VERIFIED`. Quella riga da sola vale metà del documento. Ma è l'unico presidio di qualità in 1273
> righe, e riguarda **le issue nuove**, non la passata che le produce.»

**Correzione richiesta**: `PLANNED` / `NOT VERIFIED` si applica anche al **report** del §26 e alle caselle
del §27, non solo ai corpi delle issue.

### 🏗️ MARTIN FOWLER — accoppiamento dei due artefatti

> «Due file dicono la stessa cosa in due linguaggi. Il `.md` elenca `#25, #77, #78, #79, #172, #173, #613,
> #705` in prosa al §4.1; il `.json` li ripete come `{"id":"HUD","known_issues":[...]}`. Due rappresentazioni
> della stessa verità, **nessun meccanismo che le tenga allineate** — e infatti divergono già: il `.json`
> mette `#1429` in `FACING_PIVOT`, il `.md` non lo nomina mai; il `.md` cita `#1094` in due sezioni e il
> `.json` lo ripete in due domini.
>
> Se il JSON è la macchina, la prosa deve **derivare** da lui o citarlo, non riscriverlo. Così com'è, il
> lettore deve tenere aperti entrambi e non sa quale vince.»

**Correzione richiesta**: un solo elenco autorevole. Il JSON è il candidato naturale — è interrogabile. La
prosa cita il dominio, non i numeri.

---

## 4. Scorecard

| Dimensione | Voto | Perché |
|---|---:|---|
| **Accuratezza referenziale** | **9 / 10** | 77 numeri su 77 esistono, ladder e ADR identici. Il punto perso è `CAMERA: []` |
| **Freschezza** | **3 / 10** | tetto `#1430` con 72 issue sopra, 31 aperte lo stesso giorno; §20 già eseguito |
| **Verificabilità** | **4 / 10** | DoD di aggettivi, nessun comando, nessuno strumento del repository nominato |
| **Disciplina anti-duplicazione** | **8 / 10** | §1.1, §1.2, §1.4, §24 e i divieti «non creare un secondo…» sono la parte migliore del documento — e sono ciò che il documento stesso rischia di violare per obsolescenza |
| **Struttura** | **6 / 10** | due artefatti che si ripetono senza owner; §25 fissa un ordine che non regge alla concorrenza |

---

## 5. Ciò che sopravvive — le tre tesi che valgono, e sono già misurate

1. 🎯 **Il gap `ranked` era reale, ed è l'unico `CREATE_NEW` che questa revisione ha considerato fondato.**
   La roadmap **promette** ranked e rating (righe 52 e 1126 di
   [`roadmap-post-v0.1.md`](../roadmap-post-v0.1.md)); alla misura del 2026-08-28 il tracker non aveva
   **nessuna** issue che lo possedesse — `#810` copre il *rollout del matchmaking*, che non è la stessa cosa.
   ⚠️ **E il parent non è quello che il kit proponeva**: la frase «Ranked e rating stanno qui» è dentro
   `### E44` (righe 1103–1136), non nella sezione `## v1.0` che inizia a 1138. Va sotto **E44** (`#777`), e ci
   è andato: `CP 44.5` = `#1604` (§9).
2. 🎯 **La divergenza ADR-0008 / runtime è reale e senza owner.** `MoveEndPivotMaxSteps` e
   `DashEndPivotMaxSteps` non esistono in `Source/`: il cap per eroe accettato dall'ADR non è implementato.
   `#291` è aperta ma possiede *l'input della rotazione dichiarata*, non i budget per eroe. `E16` (`#175`) è
   CHIUSA e non va riaperta — il kit ha ragione a vietarlo.
3. 🎯 **`PlacementSector` / `CoverAnchor` sono `PROPOSED`, e il kit lo dichiara.** Nessuno dei simboli
   esiste, il §7.1 lo etichetta correttamente e il §7.2 vieta esattamente le scorciatoie che romperebbero il
   determinismo. Le sette domande del §7.5 sono materiale da `OPEN_DECISIONS`, non da issue di
   implementazione — che è ciò che il §24 `DECISION_REQUIRED` prescrive.

---

## 6. Gli owner che il kit non conosce

Otto epic e due issue vive stanno **dentro** i domini del kit e non sono citate da nessuno dei due file.
Chiunque esegua la passata deve leggerle **prima** di classificare qualcosa come `CREATE_NEW`:

| Owner | Dominio del kit che lo riguarda |
|---|---|
| **E13** — `#151` Conoscenza parziale: vista e udito | §10 rumore/percezione — è il **parent reale** di `#159` e `#160`, che il kit tratta come orfane |
| **E22** — `#323` Cover Window: `OPEN → FIRE → SEAL` (+ `CP 22.1`–`22.4`, `#1561`–`#1564`) | §8 cover/guard/brace e §9 decision boundary |
| **E27** — `#327` Percezione completa (+ `CP 27.3`/`27.4`, `#1568`/`#1569`) | §10 e §11 — il contratto dell'illuminazione **è già registrato qui** |
| **E28** — `#328` Expert Bot v2 (+ `CP 28.1`–`28.4`, `#1570`–`#1573`) | §19 bot/AI, incluso il vincolo «non legge il `BotProfile` avversario» |
| **E29** / **E33** — `#329` / `#330` (+ `CP 29.x` `#1574`–`#1576`, `CP 33.x` `#1577`–`#1579`) | §15 predictive / conditional intent |
| **E36** — `#435` Framework degli status | §15 — il kit cita `E18`, `E29`, `E33` e **salta il framework degli status** |
| **E38** — `#609` Economia del turno e accoppiamento col movimento | §6 `FAC-12` (costo MP del pivot) e §7 |
| **E48** — `#1408` Il giocatore raggiunge il modello | §4 HUD e §13 frontend — **epic v0.1 aperta il 2026-08-26**, invisibile al kit |
| `#637` — la tassonomia delle icone: 17 categorie del manifest non esistono nel codice | §5 icon grammar — è **esattamente** il difetto che il §5.2 chiede di prevenire, già aperto sotto `E20` |
| `#971` — in autobattle l'input non è impedito | §13 frontend / `E47` |

---

## 7. Cosa questa revisione ha lasciato fuori, e perché

- ⛔ **Nessuna issue CLOSED riaperta.** `E16` (#175), `CP 16.2` (#177), `#1094`, `#165` restano chiuse: il kit
  lo vieta al §1.1 e ha ragione. Ciò che mancava è diventato una issue nuova, non una riapertura.
- ⛔ **Nessuna delle 31 issue aperte oggi dal kit gemello è stata toccata.** `CP 22.x`, `CP 27.x`, `CP 28.x`,
  `CP 29.x`, `CP 30.x`–`CP 33.x`, `CP 35.x` portano nel proprio corpo il vincolo «*non si apre prima dei 15
  gate della v0.1*», e i gate erano `7 ✅ · 4 🟡 · 3 ⏳` lo stesso giorno. Sono **tracciate, non pronte**.
- ⛔ **Nessun `D-nnn` assegnato.** Nessuna decisione è stata **presa**: `RNK-1` e `PLC-1`…`PLC-7` sono
  *domande*, e stanno in `OPEN_DECISIONS.md`, che è il loro owner. Zero ID rivendicati significa zero
  superficie di collisione, che è il solo motivo per cui questa passata non ha dovuto interrogare la rete.
  ⚠️ **E il numero dell'ultimo assegnato non si scrive qui**: [`CLAUDE.md`](../../../CLAUDE.md) §7 lo vieta
  dal 2026-08-28 ([#1600](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1600)) perché scade in
  ore — e la rete da interrogare sono i **ref remoti**, non `gh pr list`, che elenca le PR e non gli ID che
  rivendicano. Fra il commit che prende un ID e l'apertura della sua PR c'è una finestra in cui l'ID è preso e
  invisibile.
- ⛔ **Nessun conteggio dell'indice d'archivio ricalcolato.** Il banner di
  [`archive/src/README.md`](../../archive/src/README.md) porta una domanda aperta dichiarata («la formula non
  ha mai contenuto `graytoolkit/`»); risolverla di passaggio la nasconderebbe. Registrata, non aggiustata.
- ⚠️ **La release di #1605 non è decisa, ed è scritta come domanda nel suo corpo.** L'ADR nasce in `E16`, che
  è v0.1 e chiusa; il pivot per eroe è un asse di bilanciamento nuovo. Le due uscite sono entrambe
  difendibili e nessuna misura le scioglie: è una scelta di scope dell'autore.

## 8. Il principio, riformulato

Il §29 del kit dice: «*Il compito non è massimizzare il numero di issue*». È giusto, e questa revisione lo
estende:

> Il compito non è nemmeno **auditare** il numero massimo di domini.
> È misurare **quando** l'ultima misura è stata presa, prima di fidarsene.

Un pacchetto di consolidamento è una fotografia datata. Questo lo era già il giorno in cui è stato scritto.

---

## 9. Cosa è stato consolidato — 2026-08-28

### Issue create — 3

| # | Titolo | Parent | Perché nessun owner esistente bastava |
|---|---|---|---|
| [#1604](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1604) | `CP 44.5 · Ranked e rating: l'epic che li dichiara non ha un checkpoint che li possieda` | **E44** #777 · `v0.9` · `P0` | La promessa è in tre posti e i quattro checkpoint di E44 sono freeze/hardening/migrazione/soak. `#810` rimandava a «`44`», che non aveva deciso niente: **il rimando era circolare** |
| [#1605](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1605) | `I budget di pivot per eroe di ADR-0008 non esistono a runtime` | nessuno — `E16` è **chiusa** e non si riapre · `P2` | L'ADR diceva «da implementare (E11/E16)»: `E16` era chiusa il giorno prima che fosse accettato, `E11` è HUD. `#291` possiede l'**input**, non la regola |
| [#1606](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1606) | `[DESIGN] End Placement e PlacementSector: sette domande (PLC-1…PLC-7)` | *Debito documentale e decisioni aperte* · `P2` | Nessuna `GBX-*` copre la simulazione: il contratto graybox è **authoring**. Sette domande non deducibili, e il §24 del kit vieta di nasconderle in una issue d'implementazione |

### Issue aggiornate — 8

| # | Cosa è cambiato |
|---|---|
| [#777](https://github.com/DegrassiAaron/refactor-tactics-main/issues/777) | corpo: `#1604` aggiunto ai checkpoint; la sezione «Ranked e rating stanno qui» dichiara i quindici giorni senza owner e la contraddizione «niente di nuovo» ancora da sciogliere |
| [#810](https://github.com/DegrassiAaron/refactor-tactics-main/issues/810) | rettifica: il suo *Out of scope* rimandava a un perimetro inesistente |
| [#291](https://github.com/DegrassiAaron/refactor-tactics-main/issues/291) | la regola che il suo input deve rispettare non esiste a runtime → `#1605` |
| [#339](https://github.com/DegrassiAaron/refactor-tactics-main/issues/339) · [#726](https://github.com/DegrassiAaron/refactor-tactics-main/issues/726) · [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613) | cross-link a `#1605`, con la ragione specifica per ciascuna: le `FAC-*` **non** sono toccate · stessa forma di difetto · l'insieme legale a schermo non ha ancora il dato da mostrare |
| [#286](https://github.com/DegrassiAaron/refactor-tactics-main/issues/286) · [#1095](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1095) · [#323](https://github.com/DegrassiAaron/refactor-tactics-main/issues/323) | cross-link a `#1606` con il confine `Graybox Placement` / `Gameplay Placement` scritto per esteso |

### Decisioni registrate — 8 domande, 0 `D-nnn`

`RNK-1` e `PLC-1`…`PLC-7` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), ciascuna con innesco e con la
ragione per cui non si deduce.

### Documenti corretti — 5

| File | Correzione |
|---|---|
| [`roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) § E44 | la frase «Ranked e rating stanno qui» ora punta a `CP 44.5`, e dichiara la contraddizione interna che ha impedito al checkpoint di nascere |
| [`adr-0008`](../../decisions/adr-0008-rotazione-e-policy-di-facing.md) | lo stato passa da «*Accettato — da implementare (E11/E16)*» a «*Accettato — **non implementato**, tracciato in #1605*», con le sei misure e il trigger di revisione già passato |
| [`DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) righe **50** e **51** | 🔴 **il documento si contraddiceva**: il suo changelog «Sesto passaggio, 2026-08-10» le dichiara «accettate e recepite in ADR-0008», e le righe portavano ancora `CONFIRMED` con la nota «*non si scarta e non si applica*». Passate a `SUPERSEDED`; il **testo** non è stato riscritto, perché è una misura datata `rename-exempt` |
| [`archive/src/README.md`](../../archive/src/README.md) § *Archiviare un documento* | dichiarava attivo `scripts/check-docs-naming.py`, rimosso da `D-182` il 2026-08-21. Portato al passato, con ciò che resta coperto (i due test C++ sugli ID) e ciò che non lo è più (i nomi in **prosa**) |
| `docs/research/design/hud/mock-elementi-hud-correzioni.md` | l'unico percorso rosso di `doc-links --check`, **preesistente**: puntava all'handoff Action Phases in `docs/research/handoff/`, rimosso il 2026-08-27 quando è stato archiviato col proprio verdetto. Ripuntato all'archivio. ✅ Il gate passa da **1 rotto** a **tutti i percorsi risolvono**, con e senza `--with-archive` |

### Il conto

```text
ISSUE LETTE:        77 citate dal kit + 52 epic + le 72 sopra il suo tetto
ISSUE CREATE:       3   (#1604, #1605, #1606)
ISSUE AGGIORNATE:   8   (1 corpo, 7 commenti)
DECISIONI APERTE:   8   (RNK-1, PLC-1…PLC-7)
D-nnn ASSEGNATI:    0
DOCUMENTI CORRETTI: 5
ISSUE RIAPERTE:     0
ROADMAP PARALLELE:  0

GATE            doc-links --check              EXIT 0  (era 1, rosso preesistente)
                doc-links --check --with-archive  EXIT 0
                doc-tables --check             EXIT 0
```

> ⚠️ **Il conteggio qui sopra ha detto `4` per qualche minuto, e va detto invece che aggiustato in silenzio.**
> Il quinto documento — il link ripuntato — è stato corretto *dopo* che la tabella era scritta, e il totale
> non si è mosso da solo. È la stessa forma di difetto che il §2 di questa revisione contesta al kit: un
> numero scritto a mano che nessun gate confronta con ciò che conta.

