# Piano di riduzione degli hotspot — architettura e processo

> **Che cos'è**
>
> Mandato operativo per ridurre incrementalmente hotspot e costo di coordinamento, senza mega-refactor e
> senza rallentare la v0.1.
>
> **Che cosa non è**: una fonte di verità sullo stato. Gli owner restano `docs/roadmap/*.yaml`, GitHub e i
> documenti canonici. Questo file dichiara **direzione, atti richiesti e criteri di verifica**.

**Provenienza** — riscrittura di `RefactorTactics_Architecture_Process_Improvement_Claude.md` (handoff non
tracciato in root, audit del 2026-08-17). Quel file **non è autorità**: sette sue affermazioni sono state
misurate false o superate dallo stato live, ed è qui che sono corrette. Se esiste ancora in root, va rimosso.

**Base di misura di ogni numero in questo documento**: l'albero di questo branch, che integra `origin/main` a
**`c25e1cd0`**, misurato il **2026-08-17**. Ogni cifra qui sotto è un'istantanea datata: si **rimisura**, non si
cita.

> ⚠️ **La regola di §0.1 si è applicata a questa pagina prima che a chiunque altro, e due volte.** La prima
> stesura era ancorata a `a8f7f626`; eseguendo §19 Fase A, `origin/main` era già **25 commit più avanti** —
> sette merge, fra cui la PR **#1112** di `feat/833-interaction-graph`. Due baseline erano false
> (`RTTurnManager.cpp` 5988 → **6002**, `parallel-batch.yaml` 5188 → **5227**), un comando di §11 **non era
> eseguibile**, e una premessa di §13 era stata superata da un gate migliorato nel frattempo.
> **Poi è successo altre tre volte.** `origin/main` è passato per **cinque** sha nell'arco della stessa
> sessione — `a8f7f626` → `94575ef4` → `e638061a` → `c25e1cd0` → `d849029e` — e `parallel-batch.yaml` ha fatto
> **5188 → 5227 → 5305 → 5353**, cioè **+165 righe in poche ore**, quattro volte la crescita del TurnManager.
>
> A quel punto **inseguire è la scelta sbagliata**, e smettere è la tesi di questa pagina: un documento non può
> dichiarare una base più fresca del proprio commit. Quindi la base è dichiarata, l'sha è scritto, e chi legge
> **rimisura** invece di fidarsi. Un DoD ancorato a un numero letterale invecchia più in fretta di quanto si
> riesca a scriverlo — ed è la ragione per cui §20 chiede comandi con soglie datate, non cifre nude.
> L'appendice §25 riporta l'audit eseguito e i cinque difetti che ha trovato qui dentro.

---

## Correzioni rispetto alla stesura di root

| # | La stesura di root diceva | Misura | Qui |
|---|---|---|---|
| C1 | «verificare `origin/main`» | eseguibile da un worktree 87 commit indietro senza che nulla protesti | §19 Fase A è una sequenza di comandi con oracolo |
| C2 | #886 è «P0» | label GitHub = `P1`; la label `P0` **esiste** («Blocca la release») | §2 prescrive l'atto, non una scala privata |
| C3 | #833 «alimenta la strada verso il golden» | `issue:833` **non era un nodo** dell'Execution Graph — e la dipendenza da T5 è **falsa**, misurata | §2bis: tre atti **eseguiti**, un quarto owner, una decisione aperta |
| C4 | «BASE è il quinto processo» | **nessuna** track BASE nel batch — e il quinto processo canonico è **E, «Jolly»**, che non è l'integratore | §3: `meta.integrator`, senza sigle nuove |
| C5 | tetto 3+1+1 senza procedura di rientro | il tetto non è **falsificabile**: «stream mutante attivo» non è definito, e le due letture legittime danno **4** e **2** | §4 definisce cosa si conta, poi la procedura di rientro |
| C6 | il workstream mappa «parte da #833» | il lavoro è **già atterrato** (PR #1112 mergiata, ramo cancellato) e #833 resta `OPEN` | §3 processo **A** riparte da `main`, non da un ramo |
| — | le sigle `BASE`/`WS-*` sono un vocabolario nuovo | esiste già `cinque-processi-paralleli-2026-08-17.md` con i processi **A–E**, e il repo ne ha **già respinto** uno concorrente | §3/§4 **deferiscono**: nessuna sigla propria |
| M1 | DoD con 18 caselle non falsificabili | — | §20 ha baseline numeriche datate |
| M3 | elenco preflight scritto a mano | omette ≥3 gate reali | §11 lo **deriva** e stampa `MISSING` |
| M7 | §16 crea issue senza Tracking Impact Pass | CLAUDE.md §3 lo rende obbligatorio | §16 lo impone |

Corretti anche due **fatti** misurati imprecisi (non criteri): §10 e §1 — vedi le rispettive sezioni.
Le correzioni di criterio non applicate qui sono elencate in §24.

---

# 0. Regole non negoziabili

1. **Repository prima del documento.**
   - Leggere `AGENTS.md` e `CLAUDE.md`.
   - Eseguire §19 Fase A **per intero** prima di qualunque modifica.
   - Owner correnti: `docs/roadmap/feature-registry.yaml` · `execution-graph.yaml` · `parallel-batch.yaml` ·
     `editor-sessions.yaml` · `docs/technical/test-manuali-pie.md` · issue GitHub coinvolte.

2. **Versione Unreal.** Il `.uproject` dichiara `"EngineAssociation": "5.8"`; `CLAUDE.md` pinna **5.8.1**.
   `EngineAssociation` è un'associazione major.minor e **non** dichiara la patch: se la patch installata
   differisce da 5.8.1, si scrive nel report §21, non in un commento di codice. Non inventare API UE.

3. **Niente mega-refactor.** Nessuna riscrittura generale di `RTTurnManager`, `RTScenarioSession` o del
   GameMode verso Subsystem. Estrarre una responsabilità **solo quando una feature corrente attraversa quel
   confine**.

   > 🔴 **Questa regola è stata scavalcata una volta, con decisione d'autore del 2026-08-17**, dalla track
   > `hotspot_split` (branch `refactor/hotspot-split`). Il divieto è stato riportato all'autore **prima** di
   > iniziare, insieme alla misura che lo sosteneva; l'autore ha deciso di procedere comunque. La regola
   > resta in vigore: una decisione presa una volta non la abroga, e chi arriva dopo non eredita
   > l'autorizzazione. Che cosa è stato fatto, e con quale prova, sta in §26.
   >
   > Il fatto che il lavoro sia riuscito **non è un argomento per rifarlo**. La ragione per cui questa
   > regola esiste — un refactor grande consuma il tempo della v0.1 e produce un conflitto per ogni branch
   > vivo che tocca gli stessi file — si è verificata puntualmente: `feat/cp75-selfreposition` e `#886` la
   > stanno pagando adesso.

4. **D-139 / parallelismo.**
   - Nessuna scrittura fuori dal `writable` della propria track.
   - Nessuna modifica a `integration_only` da una track normale.
   - Nessun `.uasset`/`.umap` senza Binary Asset Lease esclusiva.
   - Una richiesta di release non è ownership.
   - **Un file nuovo si dichiara prima di crearlo** — inclusa questa pagina.

5. **Il gameplay reale resta l'unica pipeline.** Scenario Harness non diventa un simulatore alternativo.
   Nessun `SetActorLocation` / `ApplyDamage` / `if (IsTest)` che aggiri
   `Planning → Commit → Snapshot → Resolver → TurnLog`. Animazioni e UI non decidono esiti.

6. **Determinismo e privacy sono invarianti.** Stesso snapshot + regole + content version + seed + decisioni
   registrate ⇒ stesso risultato. Nessun planning avversario replicato. Nessuna modifica architetturale
   indebolisce StateHash, TurnLog o replay.

7. **Gli owner non si duplicano.** GitHub possiede lo stato issue · Feature Registry lo stato feature ·
   Execution Graph la topologia · Parallel Batch write-set e lease · Editor Sessions le sedute · Scenario
   JSON il contenuto. Le viste derivate **leggono**.

   > ⚠️ La stesura di root ha violato questa regola tre volte in due sezioni: una priorità privata (C2), un
   > arco di grafo affermato in prosa (C3) e un processo senza voce nel lockfile (C4). Un documento che
   > dichiara questa regola e poi la infrange produce esattamente lo stato che vuole evitare. Quando serve
   > un fatto che un owner non contiene, **l'atto è modificare l'owner**, non scriverlo qui.

---

# 1. Obiettivo architetturale

L'architettura corrente è valida. Il lavoro è ridurre progressivamente hotspot e costo di coordinamento.

```text
 Runtime
 ├── Core / Ability / Combat / Map / Pathfinding
 ├── Perception / Bot / Player / UI / Frontend
 ├── RTGameMode              → bootstrap + configurazione
 ├── RTTurnManager           → orchestrazione della resolution
 ├── TurnLog / StateHash / Replay
 └── Scenario Harness        → Index / Loader / Runner / Session / Assertions
                                 └── usa SEMPRE il gameplay reale
 Editor
 └── RefactorTacticsEditor   → authoring / geometry / visual tools

 Development Control Plane
 └── Feature Registry · Execution Graph · Parallel Batch · Editor Sessions
     · Scenario Corpus · registri PIE/test · GitHub
```

## Baseline degli hotspot — `e638061a`, 2026-08-17

| File | Righe | `a8f7f626` | `94575ef4` | Nota |
|---|---:|---:|---:|---|
| `Source/RefactorTactics/Turn/RTTurnManager.cpp` | **6002** | 5988 | 6002 | il più grande del repository, 3,2× il secondo sorgente non-test |
| `docs/roadmap/parallel-batch.yaml` | **5353** | 5188 | 5227 · 5305 | lockfile che è diventato anche diario |
| `Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp` | **1645** | 1645 | 1645 | |
| `Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp` | **1459** | 1459 | 1459 | **quarto candidato**, non nominato dalla stesura di root |
| `Source/RefactorTactics/RTGameMode.cpp` | **1050** | 1050 | 1050 | |

Le tre colonne coprono **poche ore della stessa giornata**. I due file che si muovono sono i due che questo
piano esiste per contenere, e `parallel-batch.yaml` cresce **due volte più in fretta** del TurnManager
(+117 contro +14): non è una coincidenza, è la misura del problema. Gli altri tre non si muovono affatto —
il che dice anche dove il piano *non* serve.

> Correzione di fatto: la stesura di root elencava tre hotspot. Il quarto è il **Loader**, a 186 righe dal
> terzo, e §8 (metadata strutturato) lo fa crescere per costruzione: il parsing è la sua responsabilità.
> Nominarlo ora costa una riga; scoprirlo dopo costa un'estrazione.

Target progressivo:

```text
RTGameMode        → orchestra bootstrap/config
RTTurnManager     → orchestra resolver specializzati
ScenarioSession   → orchestra il test, non contiene gameplay
integratore       → integra, rigenera, verifica, protegge i workstream
```

---

# 2. Prima voce della sequenza — replay delle decisioni di reazione

> Questa sezione **non** usa una scala di priorità privata. «Prima voce» descrive la posizione nella
> sequenza di §19; la priorità la possiede GitHub.

### Issue

- **#886 — Le decisioni di reazione tornano dal TurnLog: il Verifier non le richiede**
  https://github.com/DegrassiAaron/refactor-tactics-main/issues/886
  Stato misurato: `OPEN`, label `v0.1` + **`P1`**, milestone `v0.1 · Percezione e reazioni`.

### Atto richiesto su GitHub, prima di eseguire

Le label di priorità disponibili nel repository sono quattro, e **`P0` esiste**:

| Label | Significato dichiarato |
|---|---|
| `P0` | Blocca la release |
| `P1` | Core della v0.1 |
| `P2` | Tagliabile se il tempo stringe |
| `P3` | Priorità 3 — prima da tagliare |

Se #886 blocca davvero #166 — ed è la tesi di questa sezione — allora la sua priorità è `P0` e l'atto è
un comando, non una frase:

```bash
gh issue edit 886 --add-label P0 --remove-label P1
```

Se l'autore non intende bloccare la release, #886 resta `P1` e questa sezione perde la parola «blocca».
Le due cose non possono coesistere: una priorità che vive solo in un documento non è leggibile da nessun
owner. **Nessuna scala P*n* privata in questo file.**

Simmetricamente, **#950 resta `P3`** (misurato: label `bug` + `P3`, nessuna milestone). La stesura di root la
chiamava «miglioramento di processo P1»: la correzione è abbassare la pretesa del documento, non alzare la
label per far tornare la prosa. §11 dice esplicitamente che non blocca la sequenza.

### Vincolo

Deve atterrare **prima o insieme** a:

- **#166 — CP 14.6 — Counterplay, UI della finestra e misura del pacing**
  https://github.com/DegrassiAaron/refactor-tactics-main/issues/166 — `OPEN`, `v0.1` + `checkpoint` + `P2`,
  milestone `v0.1 · Percezione e reazioni`.

L'arco `issue:886 → issue:166` **esiste** già nell'Execution Graph. Non va aggiunto: va rispettato.

### Problema

Oggi la decisione di reazione percorre `Opportunity → decisione → TurnLog → serializzazione/hash`, ma il
Verifier può ri-simulare **senza usare quella decisione come input**. Questo rompe l'invariante

```text
stesso stato + stesse regole + stesso seed + stesse decisioni = stesso risultato
```

appena entra una decisione umana — cioè esattamente quando #166 la introduce.

### Implementazione richiesta

Seguire il **DoD live di #886**. Principio da preservare:

```text
AskReactionDecision
  1. RequiresDecisionBoundary?
        no  → HoldImmediate calcolato dallo stato
        yes → continua
  2. se esiste una risposta registrata per QUELL'OpportunityId
        usa QUELLA — non interrogare bot/live decider
  3. altrimenti
        normale ramo bot/live decider
```

### Non fare

- non consultare la traccia prima del gate di cardinalità;
- non trasformare `HoldImmediate` in una decisione registrata;
- non indicizzare per ordine di apparizione;
- non usare fallback `HOLD` per mismatch — un mismatch si **segnala**;
- non allargare la modifica oltre `AskReactionDecision` senza dimostrarne la necessità.

### Il formato del TurnLog: misura, non presunzione

La stesura di root vietava di «aggiungere campi al TurnLog **se** il formato live già porta l'identità
necessaria», senza dire come si stabilisce l'antecedente. Il divieto diventa un passo:

1. misurare se `OpportunityId` è **serializzato** nel formato live (non solo presente in memoria);
2. **se sì** → nessuna modifica di formato, e il punto 2 dell'algoritmo indicizza su quel campo;
3. **se no** → la modifica di formato **è in scope**, va dichiarata nella issue e nel Tracking, e porta con
   sé la domanda di versione del formato (§0.7: una versione di formato è una risorsa contesa — si verifica
   chi altro ne sta prendendo una).

### Test minimi

Devono esistere e **fallire** se la logica viene mutata:

- risposta registrata vince sul live decider;
- finestra collassata ignora la traccia;
- *orphan recorded response* viene segnalata;
- due finestre dimostrano che `OpportunityId` è **identità, non posizione**;
- `Replay.Verifier.ResimulationIsDeterministic` gira su uno scenario con decisioni reali;
- la mutazione «lookup per ordine» diventa **rossa**.

### Gate di chiusura

Non chiudere #886 con la sola prova di serializzazione/hash. La prova è la catena intera:

```text
TurnLog decision → verifier → resolver → stesso risultato
```

---

# 2bis. #833 è dentro la v0.1 — tre atti eseguiti, un quarto owner, una decisione aperta

**Decisione d'autore del 2026-08-17**: `#833` appartiene alla **v0.1**.

La stesura di root affermava in prosa che #833 «alimenta la strada verso il golden» (#170). Misurato: quella
dipendenza **non esiste in nessun owner**, e lo stato di #833 la contraddiceva.

⚠️ **Il lavoro di #833 è già atterrato su `main`** — PR **#1112** mergiata (`f82ad5f3`), ramo
`feat/833-interaction-graph` cancellato lato server. Gli atti qui sotto **restano tutti necessari**: rimisurati
su `94575ef4`, i tre owner sono ancora incoerenti. Un merge non riclassifica una issue.

| Owner | Stato misurato su `94575ef4` | Atto richiesto |
|---|---|---|
| **GitHub** | `#833 OPEN`, label `enhancement` + `P1` + **`post-v0.1`**, milestone **assente** | rimuovere `post-v0.1`, aggiungere `v0.1`, assegnare la milestone |
| **Feature Registry** | `RT-FEAT-MAP-INTERACTION-GRAPH`: `release: v0.2`, `priority: P1`, `status: DESIGNED` | `release: v0.1` |
| **Execution Graph** | `issue:833` **non è un nodo**; i `requires → issue:170` vengono da #512, #66, #75, #625, #649, #687 | aggiungere il nodo e l'arco verso `issue:170`, con `rationale` |

```bash
# 1. GitHub
gh issue edit 833 --remove-label post-v0.1 --add-label v0.1 --milestone "v0.1 · Mondo giocabile"
# 2/3. feature-registry.yaml + execution-graph.yaml: modifica + rigenerazione delle viste che alimentano
python scripts/feature_registry.py generate && python scripts/feature_registry.py shortlist
```

**Milestone consigliata**: `v0.1 · Mondo giocabile` — è dove vive `#75` (CP 10.2, obiettivo contestabile), cioè
lo stesso dominio di mondo/interazione. `v0.1 · Prova integrata` ospita #170 e va riservata al golden.
Se l'autore preferisce diversamente, cambia solo il valore del comando.

⚠️ **I tre atti sono uno solo, e non si spezzano.** Cambiare la label senza il registry produce una feature
`release: v0.2` con una issue `v0.1`; cambiare il registry senza il graph lascia #170 con un prerequisito che
la topologia non conosce. Tre owner incoerenti sono peggio di tre owner concordi sul valore sbagliato.

### Ciò che «dentro» **non** implica

Verificato, perché la lettura opposta era plausibile: la catena locale **non è una quarta issue**.
`#1014` («`Action.Interact` è inerte e nessuna azione dichiara `SetDoorState`») è `CLOSED` / `NOT_PLANNED`
dal 2026-08-16, **assorbita da #833**: le sue caselle sono nel DoD di #833. Il produttore lato giocatore
esiste già — `ARTPlayerController::HandleTargetCell` (consegnato da #737), che passa dal bersaglio-**cella**
e non da un handler col nome dell'azione.

Resta misurato oggi, e resta **dentro** il DoD di #833:

- `SetDoorState` esiste ed è applicato (`Turn/RTActionEffectLibrary.cpp`), ma
  `git grep SetDoorState -- Source/RefactorTactics/Ability/` dà **zero**: nessuna azione di catalogo né
  abilità del roster lo dichiara;
- `Action.Interact` è a catalogo (fase `Blast`, priorità 80, portata 1, slot `Main`) e **inerte**, perché la
  lista di effetti è `{}` — pinnato da `TestActionIsInert` in `Tests/RTCoreActionTests.cpp`.

🔴 **Correzione: «#833 abilita T5 dello showcase» è FALSO, e questa pagina lo affermava.** L'affermazione
veniva dall'handoff di root ed è stata ereditata senza misurarla — lo stesso difetto che §0.7 denuncia.
Misurato:

| Fonte | Dice |
|---|---|
| `../roadmap/roadmap-v0.1.md`, riga di **T5** | *«✅ dopo `S2-1` — il gate è una porta, **CP 9.3 è chiuso**»* |
| `../roadmap/plans/showcase-v01-audit.md` | *«il gate del turno 5 è una porta (CP 9.3): il meccanismo **esiste già** — stato di bordo, blocco del passo, revisione che sale»* |
| idem, tabella degli sblocchi | l'assertion di T5 è `S6-1` — `EdgeEnabled`/`EdgeDisabled`/`GraphRevisionChanged`, ✅ CP 9.3 |

T5 usa il **cambio di stato di un edge**, che esiste da CP 9.3; #833 è il **grafo sorgente → bersaglio**, cosa
diversa. Conseguenza: **nessun arco `issue:833 → issue:170`** è stato scritto nell'Execution Graph. L'arco che
le prove sostengono è `issue:833 → issue:74` — #74 (CP 10.1, `Activate`/`Interact` sugli oggetti) *consuma* il
dato, secondo `../roadmap/plans/cinque-processi-paralleli-2026-08-17.md` §3.

Resta vera la conseguenza sul **gate `scenario`** della feature, che è un'altra cosa: va trattato secondo la
regola di `RTScenarioSession` — una capability si dichiara disponibile solo quando **l'harness non è il primo
produttore**. Uno scenario scritto prima sarebbe verde e bugiardo.

⚠️ **E «dentro la v0.1» ha ramificazioni che questa pagina sottostimava: gli atti non sono tre.** Eseguendoli:

- il **quarto** owner è `../technical/scenario-map.md` (`integration_only`, `CURRENT`, scritto a mano), che
  dichiara ancora *«v0.2 · E23 · CP 23.4 (#833)»*;
- **CP 23.4 appartiene a E23, epic `v0.2`**, e il gate lo ha rifiutato: `feature_registry.py validate` risponde
  `ERROR — checkpoint 'E23.4' che nessun owner dichiara`. Una issue v0.1 con un checkpoint in un'epic v0.2 è
  un'incoerenza reale, **decisione d'autore aperta**: anticipare E23, riassegnare #833, o dichiarare
  l'eccezione. Nessuna delle tre è un atto meccanico, e nessuna è stata presa qui.

---

# 3. Workstream — questa sezione NON introduce un vocabolario

> 🔴 **Correzione strutturale prima del merge.** La stesura di root, e la prima riscrittura di questa pagina,
> nominavano i workstream `BASE` · `WS-A` · `WS-B` · `WS-C` · `WS-D`. **Sono stati rimossi.** Il repository ha
> già una mappatura canonica dei processi paralleli sulle track reali:
> [`../roadmap/plans/cinque-processi-paralleli-2026-08-17.md`](../roadmap/plans/cinque-processi-paralleli-2026-08-17.md)
> — `CURRENT`, con i processi **A** (Spatial/World) · **B** (Simulation/Rules) · **C** (Client/Replay/Tools) ·
> **D** (Content/Editor) · **E** (Jolly), e l'assegnazione delle 79 issue `v0.1`.
>
> Quel documento dichiara: *«Il precedente,* `quattro-processi-paralleli-triage-2026-08-14.md`*, di 57 sezioni
> ne applicò 13 — e le §21–§27 furono respinte perché proponevano un terzo vocabolario di classificazione.
> Questo documento non ne propone un quarto.»* Le sigle `WS-*` sarebbero state il **quinto**, e per la stessa
> ragione non hanno diritto di esistere.
>
> ⚠️ **L'argomento decisivo è che le lettere non coincidevano**: il mio `WS-A` era Simulation, che nella
> mappatura canonica è **B**; il mio `WS-B` era Map, che è **A**. Due documenti con una «A» che significa cose
> opposte è la forma peggiore di vocabolario concorrente — si legge come un riferimento e produce un errore
> silenzioso. Da qui in avanti si usano **solo** le lettere di quel documento.
>
> **Owner del write-set**: [`../roadmap/parallel-batch.yaml`](../roadmap/parallel-batch.yaml) — se le due fonti
> divergono vince il YAML, perché è quello che i gate leggono. Regola che governa: **D-139** e
> [`workflow-parallel-claude.md`](tooling/workflow-parallel-claude.md).

Questa sezione aggiunge **una** cosa a quella mappatura, e solo una: il ruolo che i processi A–E **non
contengono**.

## L'integratore — il ruolo che A–E non contiene

Nella mappatura canonica il quinto processo è **E, «Jolly»**, dichiarato `🔴 non dichiarato: un processo senza
`writable` è, per D-139, un processo che deve fermarsi al primo file`. **E non è l'integratore**, e nessuno
degli altri quattro lo è: A–D sono divisi per **materia**, e l'integrazione non è una materia.

**Non implementa feature gameplay.** Responsabilità: fetch/rebase/merge · riconciliazione dei write-set ·
binary leases · release requests · `integration_only` · viste generate sull'albero unito · preflight (§11) ·
build · automation smoke · preparazione dei gate umani.

### Perché non è una track — misurato

| Fatto | Conseguenza |
|---|---|
| l'header del batch definisce `integration_only` come *«non ownership — una proprietà di questo batch»* | una track con quel `writable` viola la regola che la definisce |
| `generated_only`: *«una vista non si assegna mai direttamente: segue la propria sorgente»*, e l'unica copia autoritativa è quella rigenerata **sull'albero unito** — *«passo 8 della chiusura del batch, non negoziabile»* | il passo 8 ha bisogno di un **soggetto**, non di un write-set |
| `meta:` porta già `status`, `base_sha`, `reconciled_count: 9` | la riconciliazione è già un oggetto di prima classe |
| `base_sha` (`86135adf`) è **31 PR indietro** rispetto a `origin/main` | il passo 8 non viene eseguito, e nessuno è in torto perché nessuno è di turno |

### Forma adottata

```yaml
meta:
  schema_version: 5          # bump obbligatorio: cambia il contratto, vedi la nota del file
  integrator:
    session: <chi è di turno>
    base_sha: <sha su cui il batch è stato rimisurato>
    reconciled_at: <timestamp>
```

Non ha `writable` perché non ne serve: i path che tocca non sono di nessuno **per costruzione**.
`parallel-batch.yaml` è a sua volta `integration_only`, quindi questo campo lo scrive chi è di turno — la
ricorsione è voluta e va detta, non nascosta.

> **`OPEN-DECISION` — reversibile.** Due alternative, con il loro costo:
> **(a) track registrata** — coerente con lo schema `tracks:`, ma nasce violando le due regole sopra.
> **(c) sessione umana** con `branch: null`, come le sedute editor — funziona, ma confonde una seduta
> (unità = un'apertura di Unreal) con un turno di integrazione (unità = un merge).
> La scelta si registra con `python scripts/rt_shared_id.py reserve D` e una voce in
> `docs/OPEN_DECISIONS.md`: **non scegliere un `D-nnn` a mano**. Fino a quel momento questa sezione è una
> proposta, non un invariante.

## Processo B — Simulation / Rules

Ordine: **#886 → #166**. Dopo #886, #166 può introdurre UI e decisione umana senza rendere falso il replay
verifier. Non iniziare un refactor generale di `RTTurnManager` dentro #886; se emerge un confine naturale,
estrarre il minimo servizio necessario e solo se: riduce il diff · non cambia semantica · ha test
equivalenti prima/dopo · il write-set lo consente.

## Processo A — Spatial / World

**Non apre e non si innesta: riparte da `main`.** Misurato su `94575ef4`:

- la PR **#1112** è mergiata (`f82ad5f3`) e il ramo `feat/833-interaction-graph` **non esiste più** — `git ls-remote`
  non lo elenca;
- **#833 resta `OPEN`**, con `post-v0.1` e senza milestone: ciò che è atterrato non ha chiuso la issue;
- la track `spatial` va rimisurata: il suo `branch:` nomina un ramo cancellato.

Quindi il residuo di #833 **si misura su `main`**, non su un ramo:

```bash
# che cosa la issue dichiara ancora aperto, contro cio' che e' atterrato
gh issue view 833 --json body            # le caselle del DoD, incluse quelle assorbite da #1014
git log --oneline f82ad5f3 -1            # il merge che ha portato la prima fetta
git grep -n SetDoorState -- Source/RefactorTactics/Ability/   # atteso: zero → l'anello manca ancora
```

⚠️ **Non dedurre il residuo dal `base_sha` del batch**: `86135adf` è **36 merge** dietro `origin/main`. Un write-set
si misura contro la base **reale** del proprio ramo, e una track il cui `branch:` è stato cancellato non ha più
una base — va riaperta, non ricalcolata.

Obiettivo: completare la catena `Interact Source → Target` come **dato**, con relazione data-driven, ordine
deterministico, reason code. Niente coppie hard-coded. Niente privacy fittizia prima che la rete la renda
falsificabile.

Fuori scope dichiarato: rete · UI remota · privacy futura · ascensori · circuiti · semantiche N→1 non decise.
La UI è di CP 23.5 (`#834`).

## Scenario / Golden — **non** è un processo, attraversa B e C

⚠️ Questa è la ragione più forte per non aver inventato una sigla: il lavoro su scenario e golden **non ha un
processo dedicato** nella mappatura canonica, e non dovrebbe averne uno. Le sue issue stanno in **B** (regole,
TurnLog, assertion) e in **C** (Scenario Composer: #1114→#1117, dichiarati *runtime, non Editor*). Un quinto
processo «Scenario» avrebbe creato un confine dove il vincolo reale è il **write-set**: `ScenarioHarness/` va
intera a una sola track, e `Scenarios/` è `integration_only`.

Target: **#170 — CP 15.4 — Golden replay degli 8 turni** (`OPEN`, `v0.1` + `checkpoint` + `P1`, milestone
`v0.1 · Prova integrata`).

#170 legge il proprio stato dagli owner. I prerequisiti `requires` **misurati** nel graph sono #512, #66,
#75, #625, #649, #687 — più `issue:833` una volta eseguito §2bis. Verificare live anche **#1060**
(`v0.1`, `P2`) e **#75** (`v0.1`, `checkpoint`, `P2`). **Non copiare qui il loro stato.**

Responsabilità: estensioni del vocabolario di assertion **solo quando necessarie** · test discriminanti ·
capability Scenario disponibili solo quando il percorso reale è eseguibile · golden solo quando tutti gli
otto turni sono eseguiti **e** asseriti.

```text
scritto != eseguito != asserito
```

Un turno scritto ma non raggiunto non è copertura.

## Processo D — Content / Editor *(l'autore davanti a Unreal)*

Lane umana. Sedute rilevanti (owner: `docs/roadmap/editor-sessions.yaml`, che dichiara già
`unblocks` / `unblocked_by` / `shares_setup_with` / `verifies`): **U21** luci graybox/framing → **U22**
geometry ghost/snap/undo e **U25** cell placement volume, entrambe `unblocked_by: [U21]` e
`shares_setup_with: [U21]`.

Produce `.uasset`/`.umap`, esegue controlli visivi, registra QA manuale. **Non implementa regole del
simulatore.**

---

# 4. Il tetto non è un conteggio di processi — è il write-set

> 🔴 **Seconda correzione strutturale.** La stesura di root prescriveva un tetto numerico —
> «3 stream code + 1 lane umana + 1 integratore» — e la mappatura canonica lo **supera già**, con una
> formulazione migliore:
>
> > *«La regola del batch non è "un dominio per processo": è "un `writable` per processo, e i file condivisi si
> > toccano una volta sola, in integrazione".»*
> > — [`cinque-processi-paralleli-2026-08-17.md`](../roadmap/plans/cinque-processi-paralleli-2026-08-17.md) §1
>
> E la misura che lo dimostra è nello stesso documento: la mattina del 2026-08-17 **tre PR si contendevano otto
> file, e nessuna delle tre condivideva una riga di `Source/`**. Due sessioni su domini diversi collidono
> comunque, perché scrivono lo stesso file di tracking. Contare i processi non prevede quella collisione;
> misurare i `writable` sì.
>
> **Quindi il tetto di questa pagina si riduce a un corollario**: più processi attivi ⇒ più probabile che due
> `writable` si intersechino sui file di tracking. Il numero non è la regola, è un indicatore del rischio — e
> il documento canonico osserva che la contesa non è sparita ma si è **concentrata** proprio su
> `parallel-batch.yaml`, conteso da due sessioni che stanno entrambe *rilasciando* un write-set.

Non massimizzare i branch: minimizzare il wall-clock **senza creare contention**.

## Che cosa si conta, e cosa no

> **Un worktree non è un processo, e un branch non è uno stream.** `git worktree list` elenca **directory di
> lavoro**: una directory ferma non consuma il tetto, e tredici worktree contro cinque processi non sono una
> violazione — sono tredici cartelle, alcune parcheggiate da giorni. Lo stesso vale per i rami: un ramo in
> attesa di merge non muta niente. La sola dimensione che misura questo tetto sono le **track `ACTIVE`**, e
> anche lì va detto come si conta una track *in chiusura*.

Misurato su `94575ef4`, 2026-08-17:

Le cinque track `ACTIVE` su `94575ef4`, con la loro composizione:

| Track | Issue | `branch:` dichiarato | Esiste su `origin`? | Conta come |
|---|---|---|---|---|
| `spatial` | 833 | `feat/833-interaction-graph` | ❌ mergiato (PR #1112) | code/scenario |
| `simulation` | 886 | `null` | — | code, **path chiesto e non posseduto** |
| `frontend_shell` | 937 | `null` · seduta **U24** | — | **lane umana** |
| `graybox_kit` | `null` | `fix/graybox-review-1099` | ❌ mergiato (PR #1104) | in **chiusura** — PR #1120 aperta |
| `replay_ui` | 472 | `feat/1085-cap-fumo` | ❌ assente | in **rilascio** — PR #1113 aperta |

Contesto (misure di supporto, **non** metriche del tetto): 9 rami vivi + `main`, 13 worktree, 2 PR aperte,
15 track `IDLE`, `schema_version` 4, `reconciled_count` 9, `base_sha` a **36 merge** da `origin/main`.

**Verdetto**: la lane umana è a posto (1/1), l'integratore **manca** (0/1 — è C4). Sulla dimensione
code/scenario il conto dipende da una definizione che questo documento non dà: **quattro** se una track in
chiusura conta, **due** se non conta. Due delle quattro hanno una PR aperta il cui scopo è chiudere o
rilasciare, quindi il rientro è già in corso e non richiede di parcheggiare nessuno.

⚠️ **Il difetto misurato non è il numero: è che «stream mutante attivo» non è definito.** Fino a quel momento
il tetto di §4 non è falsificabile — due letture legittime danno 4 e 2, e nessuna è scorretta.
Definizione proposta: *una track conta nel tetto se può ancora produrre un commit dentro il proprio `writable`;
una track la cui unica PR aperta rimuove path o chiude la track non conta.*

🔴 **E il dato che regge da solo**: **tre track su cinque nominano un `branch:` che non esiste su `origin`.**
Con `base_sha` a 36 merge di distanza, il lockfile descrive un mondo che non c'è più — e un write-set letto da
quei campi è un vincolo su rami cancellati. Questo va rimisurato prima di usare il file come autorità, e non è
un problema di tetto: è la riconciliazione che manca (§3).

Un tetto che lo stato corrente già viola, senza procedura di rientro, si legge come aspirazione e si comporta
come rumore. La procedura:

1. **Misurare** — `git ls-remote --heads origin` e `gh pr list --state open`, non la memoria del file.
2. **Ordinare** per distanza dalla chiusura: una track il cui DoD è a un gate dalla fine **chiude prima**.
   A parità, vince chi ha il write-set più piccolo (rientra più in fretta).
3. **Parcheggiare, non abbandonare** — la track eccedente passa a `IDLE` **conservando** il proprio
   `writable`: una track senza write-set è peggio di una track stantia, perché perde il vincolo che la
   protegge.
4. **Chi decide**: l'integratore di turno propone l'ordine nella riconciliazione; l'autore lo conferma.
   Un branch aperto non si cancella per rientrare in un tetto.
5. **Registrare** la ragione nel campo `note` della track parcheggiata, in forma **corta e misurabile**
   (§12).

Se rientrare richiede di chiudere il batch, quella è una decisione d'autore: `meta.note` già documenta i
dati su cui prenderla.

---

# 5. RTTurnManager — strangler refactor incrementale

## Problema

`ARTTurnManager` (**5988** righe) coordina movement · action queue · reaction · combat · terrain · cover ·
doors · perception · bot planning · TurnLog · replay · playback. È il principale hotspot fisico del
repository.

## Regola

```text
TurnManager ordina e coordina.  Resolver/librerie decidono.
```

### Non creare ora tutti questi tipi

Il target **concettuale** può essere `RTMovementResolver` · `RTReactionResolver` · `RTEnvironmentResolver` ·
`RTObjectiveResolver` · `RTKnowledgeService` · `RTPlaybackCoordinator`. **Non vanno creati in blocco.**

### Quando estrarre

Solo durante una issue reale che modifica quella responsabilità:

```text
issue X modifica Reaction
    → la logica è orchestration?  resta nel TurnManager
    → è pure domain rule?         estrarre/riusare RTReactionLibrary
```

### Acceptance per ogni estrazione

- nessun cambiamento di StateHash;
- nessun cambiamento di TurnLog atteso;
- stessa suite verde;
- almeno un test che **pinna** la regola estratta;
- include dependency di `RTTurnManager.cpp` **ridotta** (misurabile: conteggio `#include`);
- nessuna dipendenza inversa dal resolver verso GameMode/UI.

---

# 6. Scenario Harness — impedire il secondo God Object

## Confini obbligatori

```text
Discovery        → RTScenarioIndex
Parsing          → RTScenarioLoader
Execution flow   → RTScenarioSession
Batch/headless   → RTScenarioRunner
Assertions       → assertion evaluator / tipi dedicati
Gameplay rule    → runtime gameplay, MAI ScenarioHarness
```

## Criterio di code review

Se una feature Scenario richiede un grande `switch` sulle regole gameplay, o calcolo del danno, LOS,
reazione, movimento, collisione, targeting — **il codice è nel posto sbagliato**.

Scenario Harness deve: leggere → tradurre in intent/comandi reali → avviare la pipeline reale → osservare
TurnLog/stato → asserire.

Alla prossima modifica significativa di `RTScenarioSession.cpp` (**1645** righe) valutare se il nuovo codice
appartiene a decision provider · assertion evaluator · capability registry · session lifecycle. Estrarre
**solo** quella responsabilità.

⚠️ Vale anche per `RTScenarioLoader.cpp` (**1459** righe): §8 gli aggiunge campi per costruzione.

---

# 7. RTGameMode — ridurre responsabilità senza spaccarlo

`ARTGameMode` (**1050** righe) gestisce map source · fallback arena · roster · Hero class mapping · match
format · autobattle · planning override · scenario filter/selection/lifecycle · startup report.

Per la v0.1: **non** creare una costellazione di Subsystem, **non** cambiare il bootstrap senza necessità.

### Primo confine consigliato

Quando una issue reale tocca di nuovo scenario launching/configuration, introdurre un tipo dati
(`FRTScenarioRunConfig` o forma coerente col repository) con `ScenarioId` · `ExecutionMode` · `SeedOverride` ·
`AutoRun` · `AutoReady` · `RepeatCount` · `TurnPauseSeconds` · debug flags. È **configurazione**, non autorità
gameplay. Poi il GameMode delega il lancio a un helper.

### Non fare

- Scenario Definition dentro GameMode;
- duplicare il registry;
- introdurre un nuovo simulatore;
- cambiare la precedenza property / command line / cvar senza test — **citando il simbolo che la implementa**,
  non una riga: un puntatore `file:riga` non fallisce mai;
- modificare `BP_GameMode.uasset` da una track code normale.

---

# 8. Scenario metadata — evoluzione dopo la baseline

## Stato

L'indice usa `ScenarioId` · `Path` · `Tags[]` e separa ID stabile dal percorso fisico. **Da preservare.**

## Debito

`Tags[]` porta insieme categoria, personaggio, feature, milestone e purpose. Per v0.1 basta; per la crescita è
fragile.

## Evoluzione proposta — non implementare se allarga una issue corrente

```text
ScenarioId · Path · PrimaryCategory · CharacterIds[] · FactionIds[]
MilestoneId · FeatureTags[] · PurposeTags[] · SearchTags[]
```

Regole: lo Stable ScenarioId resta l'identità · la classificazione non modifica la simulazione · cambiare
metadata **non** cambia StateHash/LogHash · nessuno scenario viene duplicato per apparire in due categorie.

---

# 9. Data-driven content — non rimandare oltre il content freeze

Verificare lo stato live delle sedute **U10** (catalogo azioni) e **U11** (`URTHeroData::Actions`, che
`editor-sessions.yaml` dichiara `unblocked_by: [E6, U10]` — dipende da U10 e non solo da E6, perché il campo è
un `TArray<TObjectPtr<URTActionData>>` non popolabile senza il catalogo).

Obiettivo: `C++ = possibilità / invarianti`, `Data = variante / valori / catalogo`, prima che troppe ability
nuove nascano hard-coded.

Ordine: action/ability definitions come dati → validator → hero definitions → catalog lookup → fallback
esplicito → test di ID duplicato / riferimento mancante / contenuto invalido.

**Una famiglia di dati alla volta.** Non migrare tutto in un commit.

---

# 10. Runtime / Editor boundary

## Stato — correzione di fatto

La stesura di root descriveva la dipendenza `UnrealEd` come debito da smontare. **Misurato**: è già
correttamente condizionale e documentata.

```cpp
// Source/RefactorTactics/RefactorTactics.Build.cs
// Dipendenza SOLO in build Editor (FScopedTransaction/Undo per il generatore mappa hex, H2). Il runtime
// packaged NON la include: l'Editor non e' richiesto a runtime (ADR-0002 / documento hex §1). Il modulo
// Editor dedicato (documento §3) e' rimandato a H5.
if (Target.bBuildEditor)
{
    PrivateDependencyModuleNames.Add("UnrealEd");
}
```

Il residuo reale è **solo** verificare se il commento «rimandato a H5» è ancora vero. Priorità **sotto**
quella che la stesura di root le assegnava. Non farlo dentro una issue gameplay non correlata.

---

# 11. Preflight unico — orchestratore locale

Issue correlata: **#950** — `OPEN`, label `bug` + **`P3`**, nessuna milestone. **Non blocca la sequenza di
§19**, e non va promossa per far tornare la prosa.

## Obiettivo

Un singolo ingresso locale. Nome coerente con gli orchestratori esistenti — che sono **snake_case**
(`rt_shared_id.py`, `feature_registry.py`), mentre i gate sono kebab (`check-docs-*.py`):

```text
scripts/rt_preflight.py
```

Il preflight **non inventa regole**: orchestra gate esistenti.

## L'elenco si deriva, non si trascrive

La stesura di root elencava sei gate in forma generica e ne ometteva almeno tre reali. Un preflight che
dichiara verde su un sottoinsieme è peggio della sua assenza: sposta la fiducia senza spostare la copertura.

Gate presenti in `scripts/` su `94575ef4`, con l'esito **realmente eseguito** su albero pulito:

| Gate | Comando | Baseline |
|---|---|---|
| link documentali | `python scripts/check-docs-links.py` | OK — 3937 link, 397 file **versionati** |
| naming | `python scripts/check-docs-naming.py --check` | 🔴 **ROSSO su `e638061a`** — 392/392, copertura 100%, **1 difetto preesistente** |
| simboli | `python scripts/check-docs-symbols.py` | OK — 154 doc, 306 simboli |
| tabelle | `python scripts/check-docs-tables.py` | OK — 164 doc |
| capability owners | `python scripts/check-capability-owners.py` | OK — 9 non disponibili, 0 senza owner ⚠️ **parziale** |
| equipment defaults | `python scripts/check-equipment-defaults.py` | OK — 4 eroi, 12 confronti, 0 divergenze |
| registry: coerenza | `python scripts/feature_registry.py validate` | **errori 0 · warning 45** |
| registry: viste | `python scripts/feature_registry.py generate --check` | OK — 3 viste allineate |
| registry: shortlist | `python scripts/feature_registry.py shortlist --check` | OK |
| ID condivisi | `python scripts/rt_shared_id.py check` | OK — 150 dichiarazioni, 0 duplicati |

Totale: **dieci invocazioni**, non otto.

⚠️ **Due trappole misurate eseguendo davvero questa tabella**, e la prima era in questo documento:

- `python scripts/feature_registry.py --check` — la forma che la prima stesura prescriveva — esce con **codice 2,
  errore d'uso**: il flag esiste ma richiede un sottocomando. Un preflight che la invocasse così registrerebbe un
  fallimento come se fosse un gate rosso, oppure lo ignorerebbe. Sono **tre** invocazioni distinte.
- `check-capability-owners.py` dichiara da sé che **metà del proprio lavoro non lo fa** senza `--online`: prova che
  ogni capability *dichiara* un owner, non che l'owner sia aperto. Un `OK` parziale che non viene riportato come
  parziale è un verde che mente. Il preflight stampa la nota, non solo l'esito.

Un gate che dichiara la propria copertura (`392/392`) e uno che dichiara il proprio limite (`--online`) valgono
più di due verdi muti: sono il modello che i gate nuovi devono seguire.

🔴 **E su `e638061a` il gate naming è ROSSO — per un difetto che non appartiene a chi lo trova.**

```text
docs/archive/src/RefactorTactics_Claude_Replay_CanonicalIntent_Roadmap_v0.1-v1.0_2026-08-16.md:376
  Vektor -> Vektor `InterceptShot` e' gia' stato riclassificato in questo modello.
```

Il file è entrato in `main` col commit `2f85aa9c` («due handoff consumati»): un handoff archiviato **con il nome
legacy dentro**, vietato da **D-130**. Verificato preesistente con
`git grep -c Vektor origin/main -- <path>` → **1**, quindi il rosso c'è anche senza il lavoro in corso.

Due lezioni che questo caso rende concrete:

1. **La baseline di A7 non è un formalismo.** Senza averla misurata *prima*, chiunque tocchi un `.md` oggi vede
   un gate rosso e attribuisce a sé un difetto altrui — o peggio, «bonifica» un archivio storico che non va
   toccato.
2. **Migliorare un gate riapre difetti che l'esenzione nascondeva.** Finché `docs/archive/` era esente, un
   handoff archiviato poteva portare nomi legacy senza conseguenze; da quando la copertura è 100%,
   **archiviare è diventato un atto che il gate misura**. Non è una regressione del gate: è la sua prima
   applicazione onesta. Chi archivia un sorgente ora deve bonificarne la prosa player-facing o marcare la riga
   `rename-exempt` — e §16 vuole una issue, non una correzione silenziosa dentro un altro lavoro.

**Requisito di costruzione**: l'elenco si **scopre** a runtime (`scripts/check-*.py` + registry + shared-id),
non si scrive nel sorgente del preflight. Un gate aggiunto domani deve comparire senza toccare il preflight;
un gate rimosso deve sparire senza lasciare un verde fantasma.

## Modalità e output

```bash
python scripts/rt_preflight.py --check
```

- `PASS` / `FAIL` / **`MISSING`** per gate — `MISSING` quando lo script atteso non esiste o non è eseguibile.
  **Mai** assente dal report, **mai** contato come verde;
- il comando realmente eseguito, stampato;
- exit code non-zero al primo `FAIL` **o** report aggregato, coerente con gli script esistenti;
- conteggio finale `<verdi>/<totali>`: un preflight che non dice quanti gate ha eseguito non è misurabile.

⚠️ Prima di lanciarlo: `git add` dei file nuovi. I gate documentali vedono **solo i file versionati** — un
file non aggiunto non è stato controllato, e il verde non lo riguarda.

**Questo non sostituisce CI.** Il repository non ha `.github/workflows/` per scelta: i gate girano a mano.
Va documentato come **preflight locale/manuale**, e lo esegue l'integratore prima dell'integrazione.

---

# 12. Parallel Batch — semplificare l'uso, non riscrivere il modello

Il modello D-139 resta corretto. Problema misurato: **5188 righe**, e il file è diventato anche diario e
post-mortem.

Non cambiare schema senza issue dedicata. Da subito:

- note **corte e misurabili**;
- le spiegazioni storiche lunghe vanno in referti/PR quando non servono più a decidere l'ownership corrente;
- nel batch resta solo ciò che serve a decidere: status · issue · branch/worktree · `writable` · `excludes` ·
  blocked paths · loans/release requests · binary leases · `derives_from` · motivazione **corrente**.

```text
parallel-batch.yaml = lockfile operativo del batch
                    ≠ archivio storico generale
```

Se lo schema live richiede la provenienza in-place, **non rimuovere nulla arbitrariamente**: aprire una issue
di processo, proporre una strategia di compattazione, farla approvare prima.

⚠️ Aggiungere o **ridefinire** un campo bumpa `schema_version` (oggi **4**). Il file lo dichiara
esplicitamente: si bumpa anche quando cambia il *significato* di un campo esistente, perché un consumatore
pinnato non trova un campo sconosciuto da segnalare — legge quello giusto e conclude male.

---

# 13. Naming / source-of-truth gates

Problema misurato: alcuni rename erano corretti nelle viste `.md` mentre le sorgenti `.yaml` conservavano
nomi legacy; rigenerando, i nomi vecchi tornavano.

**Rimisurato su `94575ef4`, e metà della premessa è caduta.** Il gate è migliorato nei 25 commit: ora dichiara
`File markdown normativi analizzati: 387 (governance di root + docs/, **nessuna cartella esclusa**)` e
`Protetti dal gate: 387/387 · copertura 100% · esenti: 0`, più `Marcatori rename-exempt: 82, tutti su righe che
ne hanno ancora bisogno`.

| Parte della premessa | Stato |
|---|---|
| «il gate copre solo un sottoinsieme delle cartelle» | ❌ **superata** — copertura 100%, nessuna cartella esclusa |
| «il gate non dichiara la propria copertura» | ❌ **superata** — stampa `387/387` e i marcatori |
| «le sorgenti `.yaml` restano fuori» | ❌ **superata il 2026-08-18** — `#1109`/[#1170](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1170) hanno esteso il perimetro a `.md · .yaml · .yml`, e il referto lo dichiara **per estensione** |

✅ **E anche il lavoro residuo è chiuso.** Diceva: *«non è più "misurare la copertura": è estendere la
proprietà protetta oltre il markdown»* — fatto il 2026-08-18. Il perimetro è `.md · .yaml · .yml`, il
referto stampa il conteggio **per estensione** (senza, «zero occorrenze» e «zero file letti» restano
indistinguibili), e il gate ha per la prima volta dei test che possono fallire.

> ⏱️ **I numeri qui sopra restano quelli di `94575ef4`**, e vanno letti come tali: la stringa
> `File markdown normativi analizzati:` non esiste più — oggi è `File normativi analizzati:` seguita da una
> riga `per estensione:`. La misura è datata e resta valida per la sua data; il totale corrente si legge
> eseguendo lo script, non da qui.

## Direzione

Il gate deve proteggere una **proprietà**, non un'estensione:

```text
nessun identificatore/nome legacy nelle CURRENT SOURCES
```

## Azione — non ampliare alla cieca

1. classificare quali YAML sono *current sources*;
2. escludere archivi/storici in modo **esplicito**;
3. misurare i falsi positivi;
4. aggiungere test al gate;
5. **solo dopo** estendere.

⚠️ Il criterio che rende onesta l'estensione è **matched/total**, e il gate markdown **lo implementa già**
(`387/387`): l'estensione agli YAML deve dichiararlo allo stesso modo, altrimenti nasce meno onesta del gate che
estende. Un pattern scritto in una lingua sola vede una lingua sola.

Se non è scope di una issue corrente: aprire/aggiornare una issue QA. Non introdurlo incidentalmente.

---

# 14. QA Human Gate Requests

Problema: una track produce asset ma il registro delle verifiche manuali appartiene a un'altra track.
Owner del registro: **`docs/technical/test-manuali-pie.md`**. **Non duplicarlo.**

## Target

L'handoff diventa strutturato:

```text
Producer · RequestedVerification · Owner · RelatedIssue/Session · Status
```

Esempio:

```yaml
producer: U24
owner: playtest
requested_verification:
  - PIE-V01-FRONTEND-NAV
  - PIE-V01-FRONTEND-ERROR
```

**Sede**: `docs/roadmap/editor-sessions.yaml`, che porta già `unblocks` / `unblocked_by` /
`shares_setup_with` / `verifies` — e in cui `U25` documenta esplicitamente `verifies: []` **con motivazione**,
cioè il precedente esatto di «una richiesta dichiarata invece che dimenticata». Non creare un file nuovo se
una struttura esistente può rappresentarlo.

Il requisito è uno:

> una richiesta di QA non deve esistere solo in prosa.

---

# 15. Human Gates da preservare

## H1 — Map / Authoring (dopo U21 → U22 / U25)

illuminazione · framing · ghost · snap · undo · `.umap` pulito · scala cella · scala unità · cover leggibile ·
porte/stati leggibili · acqua/ghiaccio/terreno distinguibili. **Il verdetto è umano.**

## H2 — Reactions (dopo #886 + #166)

Overwatch armato · FIRE · HOLD · timeout · countdown 3 s · slow motion **solo** presentation · movement
truncation · invalidazione KO/Stun/Disarm/forced move · il TurnLog spiega il risultato · verifier replay
invariato.

## H3 — Showcase (quando gli owner di T5/T6/T8 sono chiusi)

T1→T8 raggiungibili · ogni turno realmente eseguito · ogni evento chiave asserito · porta/interazione ·
interposition · combo ambientale · objective · KO · **nessun turno «verde perché vuoto»**.

## H4 — Release

U16 (KPI tecnici) · U19 (pacing/game feel) · U17 (packaged). Verificare: 60 FPS target · path median < 2 ms ·
preview < 50 ms · resolver < 100 ms/turno · replay divergence = 0 · durata/ritmo · Development packaged ·
Shipping packaged · partita conclusa fuori dall'Editor.

---

# 16. Issue da creare o aggiornare

**Prima cercare su GitHub.** Non duplicare. Cercare **per sintomo**, non per titolo: una issue può avere già
misurato il difetto.

## Tracking Impact Pass — obbligatorio, non opzionale

`CLAUDE.md` §3 e `docs/technical/issue-tracking-completeness.md` lo impongono su ogni issue creata, spezzata o
modificata nella sostanza. Il blocco `## Tracking` di `.github/ISSUE_TEMPLATE/task.md` **si riempie, non si
cancella**: dodici categorie — milestone/epic · Feature Map · Scenario Map · test · Editor Map · asset ·
content/data · Wiki/docs · ADR/Decision · UI/UX · debug/observability · dipendenze.

Per ciascuna: **cerca → collega → aggiorna se lo scope cambia → crea solo se manca davvero → dichiara `N/A`**.
`N/A` è valido; un campo mancante no. Non inventare voci per riempire un campo.

Le tre trappole che questo repository ha già pagato:

- **`Test: N/A`** su simulazione, networking o regole competitive richiede motivazione esplicita. «Provata in
  PIE» non è una strategia di test.
- **Se restano passi dentro Unreal Editor, la Editor Map non è `N/A`.**
- **Prima di chiudere si riesegue il pass**, e il DoD si consuntiva **nel commento di chiusura**, non
  spuntando il body.

## Task da aprire se non esistono

1. **Scenario metadata strutturato** — `PrimaryCategory` + Character/Faction/Milestone/Purpose; nessuna
   duplicazione di scenario; metadata di sola classificazione non cambia gli hash.
2. **RTGameMode scenario config extraction** — solo dopo la baseline v0.1, o quando una feature reale lo
   richiede.
3. **Strangler refactor RTTurnManager** — una issue **per dominio**, mai «refactor TurnManager».
4. **ScenarioSession responsibility extraction** — solo quando una modifica reale dimostra un blocco.
5. **Preflight locale** — collegare o assorbire **#950** se è l'owner corretto.
6. **Naming gate su current sources** — solo dopo aver classificato quali YAML sono normativi.
7. **QA handoff strutturato** — estendere `editor-sessions.yaml` (§14).
8. **`meta.integrator` + `schema_version: 5`** — l'atto di §3, con il `D-nnn` riservato via
   `rt_shared_id.py reserve D`.

Ogni issue creata da questo mandato porta nel proprio Tracking il rimando a questa pagina.

---

# 17. Test architecture

Ogni miglioramento strutturale deve avere un **oracolo**.

## Refactor senza cambio semantico

```text
same scenario + same seed + same intents + same reaction decisions
    → same StateHash
    → same TurnLog canonical hash
```

Se esiste un golden: **non rigenerarlo** per far passare un refactor. Una rigenerazione richiede una ragione
gameplay esplicita.

⚠️ **Un oracolo mai visto fallire non prova nulla.** Prima di dichiarare l'equivalenza, rompere **una**
regola estratta per volta e riportare *quali* test cadono — come §2 richiede per la mutazione «lookup per
ordine».

## Test di ordine

Qualunque `TMap`/`TSet` iterata in output logico deve essere ordinata, **oppure** avere un test di
permutazione che dimostri l'indipendenza. Un digest che ordina il proprio input non può testare che l'ordine
non conti.

## Scenario metadata

Modificare solo metadata ⇒ stesso scenario, stesso seed, stesso StateHash, stesso TurnLog hash.

## Packaged

Per modifiche a `Build.cs`, scenario staging, command line o al confine runtime/editor: Editor Development ·
Game Development · Game Shipping · scenario smoke packaged quando pertinente.

---

# 18. Debug / logging

Mantenere log per responsabilità. Prefissi solo se coerenti con una log category esistente:
`[RTScenario]` · `[RTReplay]` · `[RTReaction]` · `[RTIntegration]`. Niente log spam per-frame.

Per un mismatch replay/reaction loggare: `TurnNumber` · `MacroPhase` · `MicroStep` · `OpportunityId` ·
expected recorded response · actual legal responses · reason code.

Non loggare informazioni di planning avversario su client non autorizzati.

---

# 19. Sequenza di implementazione

## Fase A — fotografia (eseguibile, con oracolo)

**Nessuna modifica prima che questa fase sia passata.** Ogni riga ha un esito atteso: un valore diverso è
**STOP**, non una nota.

```bash
# A1 — la base è fresca?  atteso: "0  <N>"  (zero commit di origin/main non presenti in HEAD)
git fetch --prune origin
git rev-list --left-right --count origin/main...HEAD

# A2 — dove sono?  atteso: un branch di track, NON un detached/temp
git branch --show-current
git rev-parse --short HEAD origin/main

# A3 — l'albero è pulito?  atteso: vuoto
git update-index --refresh >/dev/null 2>&1; git status --porcelain

# A4 — chi altro è vivo?  ogni ramo qui può contendere il mio write-set
git ls-remote --heads origin
gh pr list --state open --json number,headRefName,title

# A5 — il mio write-set è dichiarato?  il path DEVE stare nel writable della mia track
#      un write-set si MISURA, non si ricorda
git diff --name-only origin/main...HEAD

# A6 — il lockfile è fresco?  atteso: piccolo
git log --oneline <meta.base_sha>..origin/main --merges | wc -l

# A7 — baseline dei gate: un --check rosso può essere PREESISTENTE
python scripts/rt_preflight.py --check   # oppure i gate di §11 uno per uno
```

Il report §21 riporta **i valori ottenuti**, non «eseguito».

> Perché questa fase è una sequenza e non una frase: la stesura di root diceva «verificare `origin/main`», ed
> eseguendola da un worktree su `temp-detach` **87 commit indietro** l'audit sarebbe stato prodotto su un
> albero morto senza che nulla protestasse. A1 e A2 esistono per questo.

## Fase B — prima voce

Chiudere o sbloccare **#886**. Se i path necessari non sono ottenibili: **non aggirare D-139** — aggiornare la
release request, lavorare solo sui file già posseduti, riportare **esattamente** il blocker.

## Fase C — quality infrastructure

Se il write-set lo consente: implementare/chiudere il gate di #950 e creare il preflight come orchestratore.
**Non bloccare #886 per questo lavoro** — #950 è `P3`.

## Fase D — parallel work

Avviare in parallelo, **se i write-set sono disgiunti** e dopo la procedura di rientro di §4:
`A: replay/reaction` · `B: map interaction` · `C: scenario/assertions` · `D: editor/human`. L'integratore
integra.

## Fase E — refactor opportunistico

Quando una issue tocca TurnManager, ScenarioSession o GameMode, applicare «orchestrator vs domain logic».
**Estrarre solo ciò che il diff dimostra.**

## Fase F — content/data

Portare avanti U10/U11 prima del content freeze (§9).

---

# 20. Definition of Done — misurabile

Ogni casella è un comando con un esito, o non è una casella. Le baseline sono misurate su **`94575ef4`** il
**2026-08-17**: si rimisurano, non si citano — e in questo documento due di esse sono già invecchiate in poche
ore, quindi la rimisura non è un consiglio.

**Audit e ownership**

- [ ] Fase A eseguita per intero, con i **valori ottenuti** nel report; A1 ha dato `0 <N>` e A3 vuoto.
- [ ] `git diff --name-only origin/main...HEAD` ⊆ `writable` della propria track — **verificato riga per riga**.
- [ ] nessun `.uasset`/`.umap` nel diff senza una Binary Asset Lease dichiarata nel batch.
- [ ] le viste generate toccate sono **solo** quelle alimentate dalle sorgenti nel proprio `writable`.

**Hotspot: non crescono**

> ⚠️ **Tre di queste quattro soglie sono state rimisurate il 2026-08-17 dopo la track `hotspot_split`**
> (§26). Le nuove sono qui sotto; le vecchie restano citate perché un DoD che riscrive le proprie soglie
> senza dire quali erano diventa incontrollabile.

- [ ] `wc -l Source/RefactorTactics/Turn/RTTurnManager.cpp` ≤ **4875** *(era 6002)*
- [ ] `wc -l Source/RefactorTactics/Turn/RTTurnManager_Blast.cpp` ≤ **1318** *(file nuovo)*
- [ ] `wc -l Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp` ≤ **1653** *(era 1645)*
- [ ] `wc -l Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp` ≤ **1556** *(era 1459)*
- [ ] `wc -l docs/roadmap/parallel-batch.yaml` ≤ **5611** *(era 5353)*

> 🔴 **Due delle nuove soglie sono più ALTE delle vecchie, ed è un dato del piano, non una sconfitta
> nascosta.** Estrarre funzioni aggiunge firme, doc-comment e preamboli: il Loader guadagna 97 righe per
> perdere due funzioni da 775 e 464. Il file cresce, la funzione più grande crolla. Se la soglia da
> difendere è la dimensione del file, questo lavoro la peggiora; se è la dimensione dell'unità che si
> deve leggere per capire una regola, la migliora di due ordini di grandezza. **§26 riporta entrambe le
> misure**, perché sceglierne una sola sarebbe la stessa disonestà che §11 rimprovera ai verdi muti.
- [ ] se una soglia è superata: il commit porta l'**estrazione** che la riporta sotto, oppure la issue dichiara
      perché la crescita è irriducibile. «Valutato per estrazione» non è un esito.
- [ ] conteggio `#include` di `RTTurnManager.cpp` non cresciuto.

**Scope**

- [ ] nessun nuovo tipo fra i sei resolver di §5 creato **fuori** da una issue che modifica quel dominio.
- [ ] nessun `switch` su regole gameplay, calcolo danno, LOS, reazione, movimento, collisione o targeting
      aggiunto sotto `ScenarioHarness/` (§6).
- [ ] file toccati per issue ≤ quanto la issue dichiara; nessun refactor opportunistico nel diff.

**Sequenza**

- [ ] #886 ha un percorso di chiusura reale, provato dalla catena
      `TurnLog decision → verifier → resolver → stesso risultato`, e la mutazione «lookup per ordine» è rossa.
- [ ] §2bis eseguito: i **tre** owner di #833 concordano (`gh issue view 833` senza `post-v0.1`,
      registry `release: v0.1`, `issue:833` nodo del graph con arco verso `issue:170`).
- [ ] la priorità di #886 su GitHub è coerente con la parola usata in §2.

**Processo**

- [ ] track code/scenario `ACTIVE` ≤ **3** + 1 lane umana + 1 integratore, **oppure** la ragione
      dell'eccezione è scritta in `meta.note` con la misura che la giustifica.
- [ ] `meta.integrator` esiste (o l'`OPEN-DECISION` di §3 è registrata con un `D-nnn` riservato).
- [ ] ogni issue creata da questo mandato ha il blocco `## Tracking` **riempito**, dodici categorie, `N/A`
      motivati.
- [ ] nessuna richiesta di QA vive solo in prosa (§14).

**Verifica**

- [ ] `python scripts/rt_preflight.py --check` (o le **dieci** invocazioni di §11) — riportare `<verdi>/<totali>`,
      ogni `MISSING` e ogni `OK` parziale; confrontare con la baseline di A7 (`10/10`, **45 warning** preesistenti
      su `94575ef4`): un warning in più è un effetto tuo, i 45 no.
- [ ] suite automation rilevante verde: riportare `Found N tests` e l'esito, non «pass».
- [ ] build Editor Development verde.
- [ ] Game Development / Shipping eseguite **se** il diff tocca `Build.cs`, staging, command line o il confine
      runtime/editor; altrimenti `NOT RUN` con la ragione.
- [ ] determinismo: scenario, seed, StateHash, TurnLog hash — prima/dopo quando applicabile.
- [ ] `git status` finale pulito.
- [ ] il report §21 separa **fatto** / **deferito** / **bloccato e da chi**.

---

# 21. Report finale obbligatorio

## Audit iniziale — valori ottenuti

| Campo | Valore |
|---|---|
| `origin/main` (SHA) | |
| A1 `rev-list --left-right --count` | |
| branch / worktree | |
| track / issue | |
| UE: `EngineAssociation` / patch installata | |
| write-set misurato (A5) | |
| rami vivi che intersecano (A4) | |
| `base_sha` → `origin/main` (A6) | |
| baseline gate (A7) | |
| blockers | |

## Modifiche applicate

| Area | Problema | Soluzione | File |
|---|---|---|---|

## Cambiamenti architetturali

Per ogni estrazione: `prima → responsabilità mista` / `dopo → orchestrator + domain service`.
Righe e `#include` prima/dopo. **Perché il confine è stato estratto ora** — quale diff lo ha dimostrato.

## Test

Comandi reali ed esiti reali, con i conteggi. Non scrivere «pass» se non eseguito.

## Determinismo

Scenario · seed · StateHash · TurnLog hash · confronto prima/dopo. Quale mutazione è stata rotta, e quali
test sono caduti.

## Build

```text
Editor Development:
Game Development:
Game Shipping:
Packaged smoke:
```

`NOT RUN` quando non eseguito, con la ragione.

## Parallelism / ownership

Write-set usato · lease · release request · file `integration_only` toccati e da chi · collisioni evitate ·
viste rigenerate e da quale sorgente.

## Debito rimasto

`BLOCKED` (e da chi) · `DEFERRED` · `OUT OF SCOPE` · `NEXT`.

---

# 22. Commit suggeriti

Adattare al lavoro realmente fatto. Non creare commit vuoti o separazioni artificiali.

```text
fix(replay): consume recorded reaction decisions during resimulation
test(replay): cover reaction trace precedence and orphan decisions

chore(qa): add local preflight orchestration
test(qa): enforce automation test guards

refactor(turn): extract reaction rule from turn orchestration
refactor(scenario): isolate scenario assertion evaluation

chore(batch): register the integrator turn, schema_version 5
docs(architecture): record incremental hotspot reduction plan
```

Le keyword di chiusura issue sono **inglesi** (`Closes #N`); «Chiude #N» non chiude nulla. Il corpo si passa
con `--body-file`, non con `--body "…"`.

---

# 23. Principio finale

> 🔴 **Emendato il 2026-08-17.** Questa sezione diceva — e ancora dice — che la strategia non è
> *«l'architettura potrebbe essere più pulita» → mega-refactor*. Una decisione d'autore ha fatto esattamente
> quello, una volta, ed è documentata in §26. Il principio **non è stato ritirato**: è stato derogato con
> una misura davanti, e la deroga vale per quel lavoro, non in generale. La differenza fra derogare e
> abrogare è tutta qui, e cancellare questa sezione l'avrebbe persa.

Il progetto non richiede una riscrittura. Richiede di evitare che quattro hotspot crescano oltre il punto di
controllo:

```text
RTTurnManager.cpp          6002   (+14 in 25 commit)
parallel-batch.yaml        5227   (+39 in 25 commit)
RTScenarioSession.cpp      1645
RTScenarioLoader.cpp       1459
```

La strategia:

```text
feature reale → misura il confine → estrai solo la responsabilità necessaria
              → testa il determinismo → integra
```

e non:

```text
"l'architettura potrebbe essere più pulita" → mega-refactor
```

La v0.1 deve continuare ad avanzare mentre l'architettura migliora.

---

# 24. Correzioni dello spec panel non applicate in questa stesura

Applicate qui: **C1–C6 · M1 · M3 · M7**, più due correzioni di **fatto** (§1 quarto hotspot, §10 `Build.cs`
già condizionale) perché lasciare in un mandato una descrizione misurata come imprecisa significa scrivere
una premessa che sappiamo debole.

Residue, con la sede in cui vivrebbero:

| # | Difetto | Sede |
|---|---|---|
| M2 | «include ridotta **o** responsabilità chiaramente ridotta»: l'`or` con una clausola soggettiva rende il criterio sempre soddisfatto | §5 acceptance |
| M4 | **parzialmente risolta da altri**: il gate markdown dichiara già `387/387`. Resta l'estensione della proprietà agli YAML | §13 |
| M5 | la richiesta di mutazione è scritta in §17 come avvertenza, non come casella del DoD di ogni estrazione | §17 / §20 |
| M6 | §14 nomina la sede giusta ma non specifica il campo YAML da aggiungere né chi lo valida | §14 |
| M8 | il passo di misura sul formato TurnLog è scritto, ma senza il comando che lo esegue | §2 |
| m3 | la precedenza property/command line/cvar è citata per nome, non per simbolo | §7 |
| m5 | questa pagina è un file nuovo: va dichiarata in un `writable` prima del commit | §0.4 |

Chi le affronta le apre come issue con il Tracking Impact Pass di §16, non le corregge in silenzio qui.

---

# 25. Appendice — Fase A eseguita il 2026-08-17

Prima esecuzione reale della sequenza di §19, da un worktree fresco. Serve a due cose: dimostrare che la fase è
eseguibile, e mostrare che **trova qualcosa** — qui quattro difetti in questo stesso documento.

| Passo | Comando | Valore ottenuto | Esito |
|---|---|---|---|
| A1 | `git rev-list --left-right --count origin/main...HEAD` | `0  0` | ✅ base fresca |
| A2 | `git branch --show-current` · `rev-parse --short HEAD` | `docs/piano-riduzione-hotspot` · `94575ef4` | ✅ branch dedicato |
| A3 | `update-index --refresh` + `status --porcelain` | vuoto | ✅ albero pulito |
| A4 | `git ls-remote --heads origin` | **9** rami + `main` | ⚠️ vedi §4 |
| A4 | `gh pr list --state open` | **2** — #1120 `chore/chiusura-track-graybox`, #1113 `chore/rilascio-turnmanager-replay-ui` | ✅ nessuna interseca |
| A4 | `git worktree list` | **13** worktree, 6 su `detached HEAD` o `main` | ✅ **non** una metrica del tetto — vedi §4 |
| A5 | `git diff --name-only origin/main...HEAD` | vuoto, poi **1** file (questa pagina) | ✅ write-set dichiarato |
| A6 | `git rev-list --count --merges <base_sha>..origin/main` | **36** merge | 🔴 lockfile stantio |
| A7 | dieci invocazioni di §11, su `94575ef4` | **10/10 OK · 0 errori · 45 warning** | ✅ baseline registrata |
| A7 | rieseguito su `e638061a` al commit | **9/10 · naming ROSSO** (difetto preesistente, `2f85aa9c`) | 🔴 vedi §11 |

## Che cosa la fase ha trovato

1. **`origin/main` si era mosso di 25 commit** durante la stesura del documento (sette merge, incluso #1112).
   La base dichiarata — `a8f7f626` — era morta.
2. **Due baseline del DoD erano false**: `RTTurnManager.cpp` 5988 → **6002**, `parallel-batch.yaml` 5188 → **5227**.
   Un DoD ancorato a un numero morto passa mentre l'hotspot cresce.
3. **Un comando prescritto non era eseguibile**: `feature_registry.py --check` esce con codice **2**. Trascritto,
   non eseguito — l'errore esatto che §11 esiste per impedire.
4. **Una premessa era stata superata da un miglioramento altrui**: il gate naming dichiara ora `387/387`,
   copertura 100%. Metà di §13 è caduta, e la metà che resta è più stretta.

Nessuno dei quattro era visibile prima di eseguire. Tre riguardano **cifre**, uno un **comando**: sono le due
categorie che invecchiano da sole, e sono le due che i gate del repository non controllano nella prosa.

## Un quinto difetto, trovato dall'autore e non dall'audit

La prima lettura di A4 concludeva *«tredici worktree contro un tetto di cinque processi: la dimensione che
eccede più delle altre»*. **È sbagliata**, e nessun comando l'avrebbe detto: un worktree è una **directory**,
non un processo, e possono legittimamente essere più numerosi dei processi — una cartella parcheggiata non muta
niente. Contare le directory per misurare l'attività è l'errore di prendere il **contenitore** per il
**contenuto**.

Vale la pena registrarlo qui perché è la classe di difetto che questa pagina non copre: Fase A verifica che una
cifra sia **fresca**, non che sia la cifra **giusta per la domanda**. Sette comandi eseguiti correttamente
possono alimentare una conclusione falsa se la metrica non misura ciò che la regola vincola — e la correzione
è arrivata da chi conosce il dominio, non da un gate. Il residuo utile è in §4: il tetto è diventato
falsificabile solo dopo aver **definito** che cosa si conta.

## Quali gate hanno davvero letto questa pagina

Un verde non dice di aver controllato il tuo file. Misurato confrontando il conteggio dichiarato **prima e dopo**
aver messo il documento in albero (non versionato):

| Gate | Prima | Dopo | L'ha letta? |
|---|---:|---:|---|
| `check-docs-naming` | 387 | **388** | ✅ sì — cammina il filesystem |
| `check-docs-symbols` | 154 | **155** | ✅ sì — ma controlla solo le tabelle con un simbolo in prima cella, e questa pagina non ne ha: verde **vacuo** |
| `check-docs-tables` | 164 | **165** | ✅ sì |
| `check-docs-links` | 397 | **397** | ❌ **no** — conta i «file markdown **versionati**» |

`check-docs-links` tornerà utile solo dopo `git add`. Qui non cambia il verdetto — `grep -c "]("` su questa pagina
dà **0**, non ha un solo link markdown — ma il metodo vale in generale: **prima di lanciare i gate documentali,
`git add` dei file nuovi**, altrimenti il verde riguarda il repository e non il tuo lavoro. E un gate che dichiara
il proprio conteggio permette di scoprirlo in due esecuzioni; uno che non lo dichiara, mai.

## Limiti dichiarati di questa esecuzione

- **Il worktree non segue la convenzione del repository.** Vive in
  `.claude/worktrees/docs+piano-riduzione-hotspot` (creato dal tool nativo della sessione, `locked`), mentre le
  altre dodici directory di lavoro sono `D:/rt-*` — la forma che il campo `worktree:` di `parallel-batch.yaml`
  dichiara. È visibile a `git worktree list`, quindi non è invisibile alla riconciliazione, ma **non è
  dichiarato** in nessuna track: sotto §0.4 è esattamente il caso «file/percorso non assegnato».
- **Il branch è locale.** `docs/piano-riduzione-hotspot` non è su `origin`: fino al push il suo write-set non è
  misurabile da nessun'altra sessione.
- **A7 è una baseline documentale.** Nessuna build, nessuna suite automation: la Fase A non le richiede, e questo
  documento non le dichiara eseguite.

---

# 26. Appendice — la deroga del 2026-08-17: `hotspot_split`

Questa sezione esiste perché §0.3 e §23 sono stati **derogati una volta**, e un mandato contraddetto in
silenzio produce due verità sullo stesso file. Registra che cosa è stato fatto, con quale prova, e che cosa
resta aperto. Non è un permesso: chi arriva dopo trova §0.3 ancora in vigore.

**Come è nata.** L'autore ha chiesto un refactor ampio senza indicare un target. Il divieto di §0.3 e la
formulazione di §23 gli sono stati riportati **prima di iniziare**, con la misura degli hotspot rifatta sul
`main` del giorno. L'autore ha deciso di procedere, e ha scelto scope e approccio fra alternative
presentate. La sequenza — misura, obiezione, decisione — è la parte che vale la pena conservare: una deroga
presa così si può rivedere, una presa in silenzio no.

**Base**: `origin/main` a **`6380cbb3`**, misurata il 2026-08-17. Track `hotspot_split`, worktree
`D:/rt-refactor`, branch `refactor/hotspot-split`. Fase A: A1 `0 0`, albero pulito, A7 **10/10** con i 45
warning preesistenti.

## Che cosa è cambiato

Il metodo è uno solo, applicato tre volte: **lo stato condiviso da una funzione lunga diventa esplicito**
— una struct dichiarata o un parametro — e i blocchi che lo usavano diventano funzioni con un confine in
firma. Dentro ogni funzione estratta il corpo è **identico all'originale**: i campi si riprendono con alias
per riferimento che portano i nomi che le variabili avevano. Nessuna regola di gioco è stata riscritta.

| Funzione | Prima | Dopo | |
|---|---:|---:|---|
| `ARTTurnManager::ResolveCombat` | 1694 | **567** | −66% |
| `URTScenarioLoader::LoadFromString` | 775 | **55** | −93% |
| `URTScenarioLoader::Validate` | 464 | **34** | −93% |
| `FRTScenarioSession::BeginTurn` | 336 | **110** | −67% |

Le tre funzioni più grandi del repository non esistono più. La più grande oggi è `ARTTurnManager::PlanBots`
(**571**), che nessuno ha toccato.

| File | Prima | Dopo | |
|---|---:|---:|---|
| `Turn/RTTurnManager.cpp` | 6002 | **4875** | −1127 |
| `Turn/RTTurnManager.h` | 1065 | 1094 | +29 |
| `Turn/RTTurnManager_Blast.cpp` | — | 1318 | nuovo |
| `Turn/RTBlastContext.h` | — | 174 | nuovo |
| `Turn/RTReactionPassResult.h` | — | 91 | nuovo |
| `ScenarioHarness/RTScenarioLoader.cpp` | 1459 | 1556 | +97 |
| `ScenarioHarness/RTScenarioSession.cpp` | 1645 | 1653 | +8 |
| `ScenarioHarness/RTScenarioSession.h` | 241 | 255 | +14 |
| **Totale** | **10412** | **11016** | **+604** |

**Le due misure dicono cose opposte, ed entrambe sono vere.** Le funzioni crollano, il totale cresce del
5,8%: firme, doc-comment e preamboli costano più di quanto le funzioni perdano. Riportare solo la prima
sarebbe la stessa disonestà dei verdi muti di §11.

Tre confini, uno per file, e vale la pena nominarli perché sono ciò che il refactor ha davvero prodotto:

- **`FRTBlastContext`** — i 58 valori che i pass del Blast si passavano come variabili locali di una
  funzione sola. Erano invisibili: leggendo un pass non si poteva sapere che cosa il precedente gli avesse
  lasciato. Ora sono campi con un nome e un commento.
- **`SeenIds` / `BotIds` / `bUsesFixture`** — nel Loader attraversavano le sezioni. Sono diventati
  parametri, **misurati** prima di scrivere le firme: sono esattamente i tre nomi che comparivano in più
  di un blocco. Gli altri tre candidati (`Heroes`, `SeenCells`, `SeenVariantNames`) sono rimasti locali.
- **`ARTTurnManager&`** in `ApplyScenarioIntents` — per riferimento e non rileggendo il membro, perché
  `BeginTurn` ha già verificato che esista e chiude la sessione se non c'è.

Effetto collaterale voluto: spostando `FRTReactionPassResult` e `FRTDisplacementCause` in un header
proprio, il commento di `FRTArmedPrediction` — separato dalla sua struct da 83 righe di altre definizioni —
è tornato adiacente.

## Verifica

| Che cosa | Esito |
|---|---|
| `Build.bat RefactorTactics Win64 Development` | **Succeeded** |
| `Build.bat RefactorTacticsEditor Win64 Development` | **Succeeded** |
| `Automation RunTests RefactorTactics` | **1014 / 1014 Success**, 0 fallimenti |
| di cui test di scenario | **77** eseguiti, inclusi i `Scenario.Loader*` |
| Gate documentali (§11) | **10 / 10**, 45 warning — identici alla baseline |

La suite è stata eseguita **tre volte**: sui 14 pass del Blast, e poi sullo stato finale che include lo
split su file, il Loader e la Session. Non è una formalità: la prima esecuzione ha dato lo stesso 1014/1014
di quella finale, e senza la seconda il Loader sarebbe rimasto una modifica di 700 righe con la sola prova
di compilazione.

**Due difetti trovati dal compilatore, non dalla misura**, e vanno scritti perché sono la lezione:

1. `FRTReactionPassResult Reactions` dichiarata **due volte** — l'alias del preambolo più la dichiarazione
   originale rimasta nel corpo estratto.
2. `ARTTurnManager* TM` non passato ad `ApplyScenarioIntents`: lo script che misurava le locali condivise
   cercava i prefissi dei container (`TArray`, `TMap`, …) e **non i tipi puntatore del progetto**, quindi
   non l'ha vista.

Un'estrazione si progetta sulla misura, ma si conferma sul build. La misura che avesse dato «zero locali
condivise» era falsa, e nessuna rilettura l'avrebbe smentita.

## Che cosa resta aperto

- 🔴 **`RTGameMode` non è stato toccato.** Era nello scope chiesto dall'autore, ed è l'unica parte
  dichiarata **non fatta**: sta nel `writable` di `frontend_shell` (ACTIVE, `#937`), con perimetro
  ristretto all'emissione degli eventi. Prenderlo avrebbe richiesto una release request. §7 sconsiglia
  comunque di spaccarlo per la v0.1, e la misura è d'accordo: 16 definizioni, la più grande 205 righe —
  non ha il difetto degli altri tre.
- ⛔ **Conflitto vivo**: `feat/cp75-selfreposition` tocca `Turn/RTTurnManager.cpp` con **+67 righe**.
  Questo lavoro ne sposta ~1300 dello stesso file. Nessuna strategia automatica risolve il merge:
  l'ordine sensato è far atterrare `cp75` **prima**, perché il suo diff è di due ordini di grandezza più
  piccolo.
- ⚠️ **`#886` resta bloccata** finché la track tiene i path: `simulation` è ACTIVE proprio su quella issue.
  `AskReactionDecision` — il punto esatto che `#886` deve toccare — non è fra le funzioni estratte e non ha
  cambiato una riga, quindi il conflitto è di posizione nel file, non di semantica.
- **Funzioni ancora grandi**, per chi riprende: `PlanBots` 571, `ParseScenarioTurns` 461, `ResolveCoverStructures`
  338, `ResolveDash` 298, `CollectAttackIntents` 295, `FRTScenarioSession::Finish` 288. Nessuna di queste
  giustifica da sola una nuova deroga a §0.3.
