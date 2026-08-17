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

**Base di misura di ogni numero in questo documento**: `origin/main` = **`e638061a`** (merge PR #1125), misurato
il **2026-08-17** con `A1 = 0 0`. Ogni cifra qui sotto è un'istantanea datata: si **rimisura**, non si cita.

> ⚠️ **La regola di §0.1 si è applicata a questa pagina prima che a chiunque altro, e due volte.** La prima
> stesura era ancorata a `a8f7f626`; eseguendo §19 Fase A, `origin/main` era già **25 commit più avanti** —
> sette merge, fra cui la PR **#1112** di `feat/833-interaction-graph`. Due baseline erano false
> (`RTTurnManager.cpp` 5988 → **6002**, `parallel-batch.yaml` 5188 → **5227**), un comando di §11 **non era
> eseguibile**, e una premessa di §13 era stata superata da un gate migliorato nel frattempo.
> **Poi è successo di nuovo**: fra la correzione e il commit `origin/main` è passato a `e638061a` e
> `parallel-batch.yaml` era già **5305** (+78 in un'ora). Tre basi in una sessione — `a8f7f626` → `94575ef4`
> → `e638061a`. Non è un incidente: è il ritmo normale di questo repository, ed è la ragione per cui un DoD
> ancorato a un numero letterale invecchia più in fretta di quanto si riesca a scriverlo. L'appendice §25
> riporta l'audit eseguito.

---

## Correzioni rispetto alla stesura di root

| # | La stesura di root diceva | Misura | Qui |
|---|---|---|---|
| C1 | «verificare `origin/main`» | eseguibile da un worktree 87 commit indietro senza che nulla protesti | §19 Fase A è una sequenza di comandi con oracolo |
| C2 | #886 è «P0» | label GitHub = `P1`; la label `P0` **esiste** («Blocca la release») | §2 prescrive l'atto, non una scala privata |
| C3 | #833 «alimenta la strada verso il golden» | `issue:833` **non è un nodo** dell'Execution Graph | §2bis: tre atti su tre owner |
| C4 | «BASE è il quinto processo» | **nessuna** track BASE nel batch | §3 BASE = `meta.integrator` |
| C5 | tetto 3+1+1 senza procedura di rientro | il tetto non è **falsificabile**: «stream mutante attivo» non è definito, e le due letture legittime danno **4** e **2** | §4 definisce cosa si conta, poi la procedura di rientro |
| C6 | WS-B «parte da #833» | il lavoro è **già atterrato** (PR #1112 mergiata, ramo cancellato) e #833 resta `OPEN` | §3 WS-B riparte da `main`, non da un ramo |
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
| `docs/roadmap/parallel-batch.yaml` | **5305** | 5188 | 5227 | lockfile che è diventato anche diario |
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

# 2bis. #833 è dentro la v0.1 — tre atti, tre owner

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

Conseguenza operativa: chiudere #833 sul solo **dato** lascia T5 dello showcase non eseguibile. Il gate
`scenario` della feature va trattato secondo la regola di `RTScenarioSession`: una capability si dichiara
disponibile solo quando **l'harness non è il primo produttore**. Uno scenario scritto prima sarebbe verde e
bugiardo.

---

# 3. Workstream

Limitare il parallelismo reale. Massimo **3** stream mutanti di codice/scenario + **1** lane umana
Editor/Asset + **1** integratore. Le altre track restano registrate e `IDLE`.

## Integratore (ex «BASE») — un turno, non una track

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

## WS-A — Simulation / Replay

Ordine: **#886 → #166**. Dopo #886, #166 può introdurre UI e decisione umana senza rendere falso il replay
verifier. Non iniziare un refactor generale di `RTTurnManager` dentro #886; se emerge un confine naturale,
estrarre il minimo servizio necessario e solo se: riduce il diff · non cambia semantica · ha test
equivalenti prima/dopo · il write-set lo consente.

## WS-B — Map / Interaction

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

## WS-C — Scenario / Golden / Assertions

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

## WS-D — Human / Editor / Asset

Lane umana. Sedute rilevanti (owner: `docs/roadmap/editor-sessions.yaml`, che dichiara già
`unblocks` / `unblocked_by` / `shares_setup_with` / `verifies`): **U21** luci graybox/framing → **U22**
geometry ghost/snap/undo e **U25** cell placement volume, entrambe `unblocked_by: [U21]` e
`shares_setup_with: [U21]`.

Produce `.uasset`/`.umap`, esegue controlli visivi, registra QA manuale. **Non implementa regole del
simulatore.**

---

# 4. Regola di parallelismo — con procedura di rientro

Configurazione preferita:

```text
integratore  +  WS-A CODE  +  WS-B CODE  +  WS-C SCENARIO/CODE  +  WS-D HUMAN/EDITOR
```

Tetto: **3** stream mutanti codice/scenario · **1** lane umana · **1** integratore.
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
| «le sorgenti `.yaml` restano fuori» | ✅ **ancora vera** — analizza *file markdown*, e le sorgenti che rigenerano le viste sono YAML |

Quindi il lavoro residuo non è più «misurare la copertura»: è **estendere la proprietà protetta oltre il
markdown**. Ed è più stretto di quanto la prima stesura credesse.

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

- [ ] `wc -l Source/RefactorTactics/Turn/RTTurnManager.cpp` ≤ **6002**
- [ ] `wc -l Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp` ≤ **1645**
- [ ] `wc -l Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp` ≤ **1459**
- [ ] `wc -l docs/roadmap/parallel-batch.yaml` ≤ **5305**
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
