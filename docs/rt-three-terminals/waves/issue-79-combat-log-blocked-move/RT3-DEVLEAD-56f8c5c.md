=== RT3 HANDOFF ===

FROM:          DEV-LEAD
TO:            EDITOR
FEATURE:       issue-79-combat-log-blocked-move
WAVE_ID:       issue-79-combat-log-blocked-move/2      (ciclo di rientro dopo i Finding di VALIDATION)

BRANCH:        fix/79-blocked-move-turnlog
PARENT_BRANCH: main
BASE_SHA:      56f8c5cd3b501df003260591017fac27b5da8636
               = HEAD misurato al lancio con `git rev-parse HEAD`. Il mandato non ne
               dichiarava il valore, e per una ragione misurata: HEAD si è mosso cinque
               volte da quando VALIDATION ha chiuso.
PRODUCED_SHA:  56f8c5cd3b501df003260591017fac27b5da8636
               = BASE_SHA. Questo consolidamento non ha scritto codice: il write-set è
               questo handoff, la sezione di chiusura del WORK-ORDER e la issue #2638.
               Il commit che li porta è successivo e non tocca nessun path misurato.

INPUT_HANDOFF: docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/RT3-VALIDATION-fa9e3ed.md
               letto dal filesystem, STATUS: PARTIAL

WRITE_SET:     docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/RT3-DEVLEAD-56f8c5c.md
               docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/WORK-ORDER.md
BINARY_ASSETS: nessuno

STATUS: PARTIAL

⚠️ `PARTIAL` e non `READY`. I quattro punti dell'`UNBLOCK` di VALIDATION sono **tutti chiusi**, e
nessuno chiedeva codice. Ma `READY` direbbe che l'ingresso è pulito, e non lo è per due fatti
misurati che non appartengono a questa wave e che nessun ruolo di questa wave può rimuovere:

1. al momento della consegna il working tree porta **sette file di C++ di un'altra sessione** sotto
   `Source/`, in una directory compilata — §5 terza condizione, vedi §2.3;
2. la PR **non è mergiabile**: dieci conflitti `add/add`, tutti su `tools/rt3/` e
   `RT3_CONTROL_PLANE.md`, zero su `Source/` — vedi §5.

Entrambi sono precondizioni del ruolo a valle, non difetti del fix.

---

```text
RT3 INIT

Tipo:               DEV-LEAD (consolidamento)
Feature:            issue-79-combat-log-blocked-move
Wave:               issue-79-combat-log-blocked-move/2
Branch:             fix/79-blocked-move-turnlog
Parent branch:      main
Base SHA:           56f8c5cd
HEAD:               56f8c5cd  (stabile durante l'intera sessione, ricontrollato tre volte)
Working tree:       NON pulito — 6 file M in tools/rt3/, 7 voci sotto Source/ (2 M + 5 ??)
                    Nessuna è di questa wave. Vedi §2.3.
Contributi letti:   contrib/DEV-MAIN-39180-02.md · contrib/DEV-TEST-60316-02.md
                    (già consolidati in RT3-DEVLEAD-55e3140.md, nessun contributo nuovo)
Scope assegnati:    nessuno — questo ciclo non assegna lavoro DEV: nessuno dei quattro punti
                    dell'UNBLOCK chiede una modifica al codice della wave
Write-set:          2 file, entrambi documentali
Sistemi in scope:   invariati rispetto a RT3-DEVLEAD-55e3140.md — il write-set di codice
                    non è cambiato
Attempt:            1-F8  -> 2   (aperto da EDITOR, ripresentato da VALIDATION, risposto qui)
                    1-F11 -> 1   1-F12 -> 1   1-F13 -> 1
```

⚠️ **`1-F8` è ad `ATTEMPT 2`.** §12 ferma il ciclo a 3 ed escala. È la ragione per cui questo
consolidamento gli ha dato una **sede** (#2638) invece di ratificarlo una terza volta dentro una
wave: una ratifica non è una decisione, e alla terza il contatore chiude.

---

# 1. Preflight §4

| Campo | Valore | Esito |
|---|---|---|
| `FEATURE` | `issue-79-combat-log-blocked-move` | risolto |
| `BRANCH` | `fix/79-blocked-move-turnlog` | risolto, `= git rev-parse --abbrev-ref HEAD` |
| `BASE_SHA` | `56f8c5cd…` | risolto **per misura**, non per inferenza — vedi sotto |
| `INPUT_HANDOFF` | `…/RT3-VALIDATION-fa9e3ed.md` | risolto, file esistente, letto dal disco |
| `PARENT_BRANCH` | `main` | risolto — **dichiarato** dall'handoff in ingresso, non assunto |

## 1.1 `BASE_SHA` risolto per misura, e perché non è un input inventato

§4 vieta di dedurre un campo mancante *«dal contesto, dalla cronologia o dal working tree»*, e
`BASE_SHA` non arrivava con un valore.

⚠️ **Non era un placeholder**: il mandato ne dichiarava la **regola di risoluzione** — *«rilevalo
al lancio con `git rev-parse HEAD`»* — che è una procedura deterministica con un solo esito,
eseguita e registrata. Ciò che §4 vieta è l'inferenza, cioè scegliere un valore plausibile; qui il
valore è **misurato**, e il comando che lo produce è nel referto.

🔑 **E la ragione per cui il mandato l'ha fatto è la stessa che apre `1-F13`**: un valore scritto a
mano sarebbe già scaduto. Il mandato citava `77233ea3` come HEAD; alla misura HEAD era `56f8c5cd`,
**cinque commit più avanti**. Un `BASE_SHA` letterale avrebbe prodotto un `BLOCKED` §5 su una
divergenza di documentazione.

## 1.2 §11 — l'ingresso non è `BLOCKED`

`RT3-VALIDATION-fa9e3ed.md` è `PARTIAL`. §11: non blocca. E vale la seconda metà della regola —
i sistemi lasciati `NOT RUN` o `OBSERVED` da VALIDATION **non diventano `PASS`** qui per
ereditarietà. Questo handoff non li tocca: DEV-LEAD non ha colonna in §7.

---

# 2. Precondizioni §5 — tre divergenze, tutte misurate

## 2.1 `HEAD` ≠ `PRODUCED_SHA` dell'handoff in ingresso

```text
PRODUCED_SHA in ingresso   fa9e3ed2
HEAD misurato              56f8c5cd
git log --oneline fa9e3ed2..HEAD                                        -> 5 commit
git diff --name-only fa9e3ed2..HEAD -- Source/ Content/ Config/ Scenarios/  -> (vuoto)
git diff --stat     fa9e3ed2..HEAD   -> 50 file, tutti tools/rt3/ e docs/, più .gitignore
                                        e scripts/rt3.ps1
```

§9 dice che un mismatch su `PRODUCED_SHA` *«è `BLOCKED`, non un avviso»*. **Non ho bloccato**, e
dichiaro la lettura invece di applicarla in silenzio: è la **quarta** occorrenza della stessa
divergenza in questa wave, ed è esattamente la materia di `1-F8`. La decisione è in §4 di questo
handoff, e la sede in cui vale oltre questa wave è #2638.

## 2.2 `HEAD` è rimasto fermo durante la sessione

Ricontrollato tre volte — apertura, metà, prima della scrittura — sempre `56f8c5cd`. Nessuna
finestra di misura è stata attraversata da un movimento di `HEAD`.

## 2.3 🔴 Il working tree, invece, si è mosso — e sotto `Source/`

È la terza condizione di §5, ed è il reperto di questo consolidamento.

```text
SNAPSHOT 2026-09-06T18:07:03Z   HEAD 56f8c5cd

git diff --name-only fa9e3ed2..HEAD -- Source/ Content/ Config/ Scenarios/   -> 0 file

git status --porcelain              -- Source/ Content/ Config/ Scenarios/   -> 7 voci
   M Source/RefactorTactics/Map/RTHexMapActor.cpp
   M Source/RefactorTactics/Map/RTHexMapActor.h
  ?? Source/RefactorTactics/Map/RTMapTemplateLibrary.cpp
  ?? Source/RefactorTactics/Map/RTMapTemplateLibrary.h
  ?? Source/RefactorTactics/Map/RTSpawnPoint.cpp
  ?? Source/RefactorTactics/Map/RTSpawnPoint.h
  ?? Source/RefactorTactics/Tests/RTMapTemplateTests.cpp
```

All'apertura della sessione le voci sotto `Source/` erano **zero**; alle 20:00 e alle 20:06 locali
sono comparse, mentre misuravo. Un'altra sessione sta scrivendo C++ adesso.

⛔ **Non sono inerti.** `Source/RefactorTactics/Map/` è una directory del modulo `RefactorTactics`
— porta già decine di `.cpp` tracciati e compilati — quindi quei file entrano nella compilazione di
qualunque build avviata ora, e `RTMapTemplateTests.cpp` entra nella raccolta di qualunque suite.

🔑 **E il predicato con cui la wave aveva scritto la lettura di `1-F8` non li vede.** Quel predicato
è `git diff BASE_SHA..HEAD`, che risponde alla **prima** condizione di §5 e tace sulla **terza**.
Le due righe sopra sono la prova: il `diff` dice `0`, `status` dice `7`. È il difetto che questo
consolidamento ha trovato rispondendo, ed è ciò che #2638 chiede di correggere.

⚠️ **Non ho toccato quei file.** `CLAUDE.md` §9 e `TERMINAL_DEV.md` proteggono il lavoro non
committato di altre istanze; rimuoverli o committarli sarebbe la peggiore delle azioni disponibili.
Sono una **precondizione dichiarata** per chi misura dopo, non un difetto da riparare qui.

## 2.4 Staging

Nessun `git add -A`, nessun `commit -am`, nessun `reset`, `restore`, `clean`, `switch` o
`pull --rebase`. Staging per i due path espliciti del write-set. Le tredici modifiche altrui
attraversano questa sessione **intatte**.

---

# 3. I quattro punti dell'`UNBLOCK` — esito

| # | Punto | Esito | Sede |
|---:|---|---|---|
| 1 | decidere `1-F11` e **scrivere** la decisione | ✅ **CHIUSO** — fuori wave, e la decisione è ora scritta dove vive la DoD | #2628 · commento su #79 |
| 2 | sanare o **dichiarare** `1-F12` | ✅ **DICHIARATO** — sanare non compete a questa wave; la misura conclusiva resta `NOT RUN` e ha un esecutore nominato | #2551 · #2629 · commento su #79 |
| 3 | decidere `1-F8` | ✅ **DECISO**, e il predicato è stato **corretto**: quello scritto era cieco a metà di §5 | #2638 |
| 4 | PR sul `PARENT_BRANCH` dichiarato | ✅ **APERTA** su `main`, in draft — e misurata non mergiabile per cause estranee | PR #2631 |

⛔ **Nessuno dei quattro ha richiesto una riga di codice**, come VALIDATION aveva previsto. Il
write-set di codice della wave è identico a quello che EDITOR e VALIDATION hanno misurato:
`git diff --name-only a59671c8..HEAD -- Source/ Scenarios/` restituisce gli stessi **8 file**.

---

# 4. `1-F8` + `1-F13` — decisi, e la decisione precedente era incompleta

Le risposte scritte in `WORK-ORDER.md` il 2026-09-06 le **ratifico nel merito** e le **correggo
nella forma**. La lettura era:

> *Una misura è valida se il delta fra `BASE_SHA` e `HEAD` non tocca i path che la misura osserva,
> e la sessione lo dichiara con il comando che l'ha verificato.*

Il merito regge: §5 protegge la stabilità del **soggetto** misurato, non l'uguaglianza di una
stringa, e in quattro occasioni su quattro il soggetto non è cambiato.

⛔ **La forma no.** §5 ha quattro condizioni; il predicato ne verifica **una**. §2.3 di questo
handoff è il controesempio misurato: `diff` vuoto, `status` con sette file di C++ sotto una
directory compilata. Un predicato che nomina una condizione su quattro **autorizza esattamente ciò
che la condizione taciuta vietava**.

**Decisione, operativa per questa wave e proposta come emendamento in #2638:**

> Una misura è valida se, sui path che osserva, **entrambi** questi comandi tacciono:
>
> ```text
> git diff --name-only <BASE_SHA>..<HEAD> -- <path misurati>   -> vuoto
> git status --porcelain                  -- <path misurati>   -> vuoto
> ```
>
> e la sessione li dichiara **con il loro output**, non con la loro conclusione.

⚠️ **Cosa la decisione continua a non coprire**, e `1-F13` ha ragione a chiamarla coincidenza:
nulla impedisce a un commit — o a un salvataggio — di arrivare **durante** la finestra. Entrambi i
comandi si eseguono prima. Il rilevatore esiste già ed è il marker `[RT-MEASURE]` di `rt-suite`,
che in questa wave ha segnalato il movimento senza che nessuno glielo chiedesse; la protezione, no.

## 4.1 Perché non ho modificato `RT3_CONTRACT.md`

Due ragioni, la seconda misurata:

1. **il contratto ha un owner**, e la sua sezione `Aperti` esiste per registrare le domande sul
   contratto senza risolverle da una wave. È la disciplina di `GOV-5` (→ `D-335`) e `GOV-6`
   (→ `D-342`);
2. 🔴 **il branch è 45 commit indietro rispetto a `origin/main`**, e su `origin/main` `D-342` ha
   **già riscritto la sezione `Aperti`** — che ora dichiara *«Entrambe sono chiuse»*. Una riga
   aggiunta da qui colpisce la stessa tabella: o conflitto, o `D-342` cancellata in un auto-merge.

```text
git rev-list --left-right --count origin/main...HEAD   -> 45  23
git log --oneline <merge-base>..origin/main -- docs/rt-three-terminals/prompts/RT3_CONTRACT.md
   cf790ff0 docs(rt3): GOV-6 si chiude separando il vocabolario dalla forma — D-342
   441ba7df docs(rt3): il contratto nominava come barriera un file versionato e identico ovunque
```

✅ **§5 è però invariato su `origin/main`**: verificato leggendo il testo corrente. La domanda di
`1-F8` è ancora aperta sul contratto vivo, non su una copia stantia.

⚠️ **Conseguenza da leggere per §13**: il `RT3_CONTRACT.md` presente su questo branch **non è la
versione viva**. Chi chiude la wave rilegga il contratto da `origin/main`.

---

# 5. Punto 4 — la PR, e cosa la blocca davvero

**PR [#2631](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2631)** → base `main`,
head `fix/79-blocked-move-turnlog`, **draft**.

✅ Il `PARENT_BRANCH` è quello **dichiarato** dall'handoff in ingresso, non `main` per default:
`RT3-VALIDATION-fa9e3ed.md` e `RT3-DEVLEAD-55e3140.md` dichiarano entrambi `PARENT_BRANCH: main`,
e la PR ci corrisponde.

## 5.1 Il merge è stato misurato, non presunto

```text
git merge-tree --write-tree --name-only origin/main HEAD
  -> 10 CONFLICT (add/add):
       docs/rt-three-terminals/RT3_CONTROL_PLANE.md
       tools/rt3/README.md
       tools/rt3/rt3/{__init__,cli,daemon,errors,model,store}.py
       tools/rt3/tests/{harness,test_daemon}.py
  -> Auto-merging Source/RefactorTactics/Turn/RTTurnManager.cpp    (nessun conflitto)
  -> conflitti su Source/ Content/ Scenarios/ Config/: NESSUNO
```

🔑 **Il codice della wave mergia pulito.** `RTTurnManager.cpp` è l'unico file toccato da entrambi i
lati, e le regioni sono disgiunte: `origin/main` ha cambiato `RunReactionPass` e
`ResolveCombatPasses` (righe ~4966–5983), la wave ha aggiunto in `ResolveMovement` (righe
~7150–7609).

⛔ **I dieci conflitti sono al 100% estranei alla #79**: sono il control plane `rt3`, atterrato su
questo branch da altre sessioni. È il costo, ora **misurato**, di ciò che il consolidamento
precedente aveva già dichiarato — il branch ha ricevuto commit che non gli appartengono — e che
`1-F13` descrive sulla consegna invece che sulla misura.

⚠️ **Non li ho risolti**, e non per prudenza: risolverli significa arbitrare due stesure parallele
di `tools/rt3/` che appartengono a un'altra wave e a un altro owner. Un DEV-LEAD della #79 che
sceglie quale `store.py` vince sta decidendo fuori dal proprio scope.

## 5.2 Cosa NON risolve i conflitti

⛔ **Riscrivere la storia del branch.** È la scelta che il consolidamento precedente ha rifiutato,
e la ragione resta valida: `55e31404`, `8dcd3a76` e `fa9e3ed2` sono gli SHA su cui EDITOR e
VALIDATION hanno **misurato**, e ogni `EVIDENCE_REF` della wave smetterebbe di essere raggiungibile
dalla PR che dovrebbe portarla.

**Uscite disponibili, nessuna eseguibile da qui:**

| Uscita | Chi | Costo |
|---|---|---|
| `main` assorbe prima il control plane `rt3`, poi la PR si aggiorna | owner di `tools/rt3/` | nessuno per la #79 |
| la #79 si consegna con un merge di `origin/main` nel branch | DEV, poi **rebuild + risuite** | ⚠️ invalida le misure: `RTTurnManager.cpp` cambia da `main`, e nessuna evidenza della wave copre il risultato del merge |

⚠️ **La seconda uscita non è gratis** e va detta: `origin/main` ha modificato `RTTurnManager.cpp` in
45 commit. Nessun ruolo ha misurato il **risultato del merge** — né build, né suite, né PIE. Quella
misura è `NOT RUN` e non la produce nessuno dei gate già eseguiti.

---

# 6. Punto 2 — `1-F12`, dichiarato con le misure che ho potuto fare

VALIDATION ha nominato la misura mancante: *«la full suite su `origin/main` deve mostrare gli STESSI
tre rossi»*. **Non l'ho eseguita e non potevo**: `TERMINAL_DEV.md` vieta a questo ruolo di avviare
`rt-suite`, e §7 non assegna a DEV-LEAD nessuna colonna.

Ciò che ho misurato senza occupare il motore, e che restringe l'ipotesi:

```text
merge-base origin/main HEAD = a59671c8

git rev-list --count a59671c8..origin/main -- .../RTStallDefinitionMeasureTests.cpp   -> 0
git rev-list --count a59671c8..origin/main -- .../RTUnbalancedProneTests.cpp          -> 0
git rev-list --count a59671c8..origin/main -- .../RTMatchAutobattleTests.cpp          -> 0
git diff --name-only a59671c8..origin/main | grep -iE "IconCatalog|DA_Icon|Icons/"    -> (vuoto)
git diff --name-only a59671c8..origin/main -- Content/ Scenarios/ Config/             -> 0 file
```

**Nei 45 commit che `origin/main` ha accumulato dopo la biforcazione, nessuno tocca i tre test né
il catalogo icone né i dati su cui misurano.** ∴ non c'è ragione di aspettarsi che i tre rossi
siano stati sanati, e `#2551` — aperta stamattina, tuttora `OPEN` — dice di sé *«il gate è rosso su
`main`»*.

⚠️ **Resta un argomento, non una misura.** Che i file di test siano identici non prova che l'esito
lo sia: la wave ha cambiato codice di produzione, e l'unico modo di chiudere la domanda è eseguire
la suite su `origin/main`. La prescrizione è nella DoD di **#2629** e l'esecutore è una sessione
`VALIDATION`.

➕ Una precisazione che i 45 commit rendono necessaria: la misura va fatta su un `origin/main`
**nominato per SHA**, perché `main` si è mosso 45 volte da quando la wave è nata. Al
2026-09-06 `origin/main` = `11e55f57`.

🔴 **Il reperto che il consolidamento precedente ha trovato e che vale più del Finding resta
aperto**: il test prescrive per il proprio rosso di *«rimisurare `BOT-STALL-1` e aggiornare
`OPEN_DECISIONS`»*, ma `BOT-STALL-1` è **chiusa dal 2026-08-29** (`D-244`). Chi raccoglie quel
rosso non ha una sede dove scrivere l'esito. Verificato: `OPEN_DECISIONS.md` riga 293 la porta
barrata, e `D-244` la registra. È nella DoD di #2629.

---

# 7. Punto 1 — `1-F11`, e come si legge la DoD della #79

**Decisione: fuori wave, #2628.** Ratifico la risposta già scritta, e la porto dove la DoD vive.

Il criterio che l'ha determinata, e che non è «costa meno»:

- il difetto è **preesistente e misurato tale** — lo stesso messaggio è in `rt-suite.log` delle
  08:32, sette ore prima che il fix fosse scritto;
- è **fuori dal write-set**: `RTTurnLogLibrary.cpp` non è nel diff della wave;
- ha lo **stesso precedente già applicato**: `1-F9` → #2627, stessa famiglia, canale diverso;
- il ramo di fallback **non va tolto**: è la rete che ha reso il reperto visibile. Ciò che manca è
  un test di **esaustività dell'enum**, ed è nella DoD di #2628.

## 7.1 La lettura della DoD, scritta e non sottintesa

VALIDATION lo chiedeva esplicitamente: *«se `1-F11` esce in una issue separata come `1-F9`, la DoD
di questa issue va letta di conseguenza e la decisione va scritta, non sottintesa»*.

L'obiettivo della #79 è *«ogni esito deve essere spiegabile leggendo il log, senza aprire il
debugger»*. Alla lettera, tre `ERTMoveOutcome` senza testo lo violano.

**Lettura adottata**: l'obiettivo della #79 è **delimitato dalla sua stessa DoD**, che nomina due
fallback specifici — *«percorso bloccato → fermo»* e *«bersaglio non valido → annullato»* — e
quattro criteri. L'**esaustività** di `ERTMoveOutcome` è un requisito distinto, oggi senza test e
senza owner: da #2628 ce l'ha.

⛔ **Il costo si scrive, non si nasconde**: chiudere la #79 lascia tre esiti su sedici illeggibili,
e quel debito è **nominato e assegnato**, non silenzioso. Se l'owner preferisce che la #79 non
chiuda finché l'enum non è esaustivo, la decisione va rovesciata su #79 — e questo handoff è il
posto in cui la scelta è visibile.

---

# 8. `CONTRATTO COMPORTAMENTALE`

Invariato rispetto a [`RT3-DEVLEAD-55e3140.md`](RT3-DEVLEAD-55e3140.md) § *Contratto
comportamentale*, e non ricopiato qui: il write-set di codice non è cambiato di un byte, e due
stesure dello stesso contratto sono due contratti.

Verificato, non presunto:

```text
git diff --name-only a59671c8..HEAD -- Source/ Scenarios/   -> gli stessi 8 file
git diff --stat      fa9e3ed2..HEAD -- Source/ Scenarios/   -> (vuoto)
```

I campi che i ruoli a valle leggono, riportati perché non richiedano un secondo file:

| Campo | Valore |
|---|---|
| `SEED_SOURCE` | `none` — nessun RNG sul percorso, dal click alla voce |
| `Privacy` | non degradata, misurata: **zero** `UPROPERTY(Replicated)` aggiunte, delta 0 su tre misure indipendenti |
| `Replay` | nessun campo nuovo, nessun enumeratore nuovo, **nessun bump di formato** |
| `Editor-visible expectation` | in PIE, al turno 4 di `PIE-V01-COLL` si legge «fermo: cella occupata» invece di «resta». È un'**attesa**, mai un risultato |

⚠️ `SEED_SOURCE: none` è la grafia che `D-342` ha reso canonica su `origin/main` (§6 del contratto
vivo): `canonical <sorgente>` · `none` · `generated`. `fixed`, che compare in
`RT3-VALIDATION-fa9e3ed.md` §6.3 riferito allo scenario, è **ritirato** da `D-342` — è la stessa
cosa che DEV chiama `canonical`, e qui non cambia nessun verdetto perché non c'è RNG.

---

# 9. `SISTEMI IN SCOPE`

Derivati dal write-set con §8, **senza verdetto**. Invariati: il write-set di codice non è
cambiato.

In scope perché toccati:
`PLANNING` · `READY/COMMIT` · `MOVEMENT` · `COMBAT LOG` · `AUTOMATION/SCENARIO`

In scope per §8 punto 3, a valle del produttore modificato:
`TURNLOG/REPLAY` · `DETERMINISM` · `UI/HUD` · `SAVE/RELOAD` · `BUILD`

In scope come verifica di non-degrado:
`NETWORK AUTHORITY` · `PRIVACY`

`N/A — REASON: fuori write-set`:
`PROJECT` · `ARCHITECTURE` · `ASSETS` · `BLUEPRINT` · `DATA` · `DATA VALIDATORS` · `MAP` ·
`GRID/GRAPH` · `INPUT` · `CAMERA` · `SNAPSHOT` · `TARGETING` · `LOS/COVER` · `DAMAGE` ·
`STATUS/CONTROL` · `DISPLACEMENT` · `REACTIONS` · `ENVIRONMENT` · `OBJECTIVES` · `KO/CLEANUP` ·
`CERTAINTY` · `ERRORS` · `PERFORMANCE` · `PACKAGED`

⚠️ VALIDATION ha lasciato `NETWORK AUTHORITY` e `PRIVACY` a `OBSERVED` pur avendo tetto `PASS`,
perché la prova richiede canary lato connessione e non l'ha eseguita. §11: **non diventano `PASS`
qui**. Restano `OBSERVED`.

---

# 10. `CONTRIBUTI CONSOLIDATI`

| File | `CREATED` | `STATUS` | Esito |
|---|---|---|---|
| `contrib/DEV-MAIN-39180-01.md` | 15:18 | `BLOCKED / MISSING_INPUT` | **SUPERSEDED** da `-02` |
| `contrib/DEV-TEST-60316-01.md` | 15:18 | `BLOCKED / MISSING_INPUT` | **SUPERSEDED** da `-02` |
| `contrib/DEV-MAIN-39180-02.md` | 15:56 | `READY` | consolidato in `55e31404` |
| `contrib/DEV-TEST-60316-02.md` | 16:02 | `READY` | consolidato in `55e31404` |

**Nessun contributo nuovo in questo ciclo**: `contrib/` è invariato dalle 16:02, e nessuno dei
quattro punti dell'`UNBLOCK` chiedeva lavoro DEV. Nessuno scope assegnato, quindi nessuna
sovrapposizione possibile.

---

# 11. `RISPOSTE`

Una per ciascun Finding aperto verso DEV-LEAD. Nessuno era un contributo `BLOCKED`: in questo ciclo
i richiedenti sono EDITOR e VALIDATION.

| Finding | Severità | Risposta | Sede |
|---|---|---|---|
| `…/1-F8` | `P3` | **DECISO — e corretto.** Il predicato scritto verificava una delle quattro condizioni di §5; la decisione operativa ora ne verifica due, e la domanda di contratto ha una sede | #2638 |
| `…/1-F9` | `P3` | **CONFERMATO, fuori wave.** Nessuna riapertura | #2627 |
| `…/1-F11` | `P2` | **ACCOLTO, fuori wave.** Con la lettura della DoD scritta su #79, non sottintesa | #2628 |
| `…/1-F12` | `P2` | **ACCOLTO e DICHIARATO.** Ereditarietà misurata; i 45 commit di `main` non toccano i tre soggetti; la misura conclusiva resta `NOT RUN` con esecutore nominato | #2551 · #2629 |
| `…/1-F13` | `P3` | **DECISO insieme a `1-F8`.** La protezione mancante — la finestra di misura — è proposta, non applicata | #2638 |

Nessun `REJECTED`. Nessun `DEFERRED`: §*Obbligo di risposta* di `WAVE_DEV_LEAD.md` lo dichiara
malformato senza condizione esplicita, e qui ogni punto ha una sede raggiungibile.

---

# 12. `USER_REQUIRED`

Check a oracolo umano **previsti**. Nessuno è un verdetto di questo handoff: §9 li colloca nel
payload, e DEV-LEAD non ne porta.

| Check | Chi | `Result` |
|---|---|---|
| `PIE-V01-LOG` — la verifica PIE che la #79 nomina fra i propri test | EDITOR | `NOT RUN` |
| `PIE-V01-COLL` clausola **(c)** — registrata `❌` in `docs/technical/test-manuali-pie.md:1001`, voce ferma a 🟡 in attesa di questa issue. Va **rimisurata** ora che il fix è atterrato | EDITOR | `NOT RUN` |
| Lettura **in partita** della riga del combat log al turno 4 di `CollisionChoke`: «fermo: cella occupata» ≠ «resta» | EDITOR | `NOT RUN` |

⛔ **Questi tre sono il collo di bottiglia della #79, non i Finding.** Nessuna suite li produce,
nessun consolidamento li chiude, e §13 lega `DONE` alla Definition of Done **viva**, che li nomina.
Con tutti i Finding chiusi la wave resta a `PARTIAL` finché non esiste una seduta.

📘 La guida operativa esiste: [`docs/technical/runbooks/guida-seduta-chiusura-79-denial-e-log.md`](../../../technical/runbooks/guida-seduta-chiusura-79-denial-e-log.md),
riconciliata il 2026-09-06 con i tre `USER EDITOR CHECK` dell'handoff EDITOR.

🔴 **Precondizione della seduta, dalla misura di §2.3**: la guida si apre con *«il passo che viene
prima di tutto: ricompilare»*. Compilare **adesso** significa compilare anche i sette file di
un'altra sessione. Prima della seduta, `git status --porcelain -- Source/` deve tacere — o la
seduta deve dichiarare cosa c'era dentro il binario che ha misurato.

---

# 13. `NOT RUN`

| Elemento | Motivo |
|---|---|
| BUILD / compile | ruolo DEV: non occupa Unreal. §7 non assegna a DEV-LEAD nessuna colonna |
| Automation, suite mirate e full | come sopra — `TERMINAL_DEV.md` vieta `rt-suite` a questo ruolo |
| Full suite su `origin/main` (`11e55f57`) | **la misura che chiude `1-F12`**. Esecutore: sessione `VALIDATION`. Prescritta nella DoD di #2629 |
| Build e suite sul **risultato del merge** `origin/main` ← branch | nessuno l'ha eseguita, e `RTTurnManager.cpp` cambia da entrambi i lati. Vedi §5.2 |
| PIE / packaged | dominio EDITOR e VALIDATION. Vedi §12 |

`RT3_CONTRACT.md` §6: nessuna di queste voci si legge `PASS`. §3: `file modificato != build/test/PIE/packaged verificato`.

---

STATUS:   PARTIAL
REASON:   I quattro punti dell'UNBLOCK di VALIDATION sono chiusi e nessuno ha richiesto codice:
          il write-set della wave è identico a quello misurato da EDITOR e VALIDATION. `READY`
          direbbe che l'ingresso è pulito, e due misure dicono il contrario — entrambe per cause
          estranee alla #79: sette file di C++ non committati di un'altra sessione sotto `Source/`
          (§2.3), e dieci conflitti `add/add` che rendono la PR non mergiabile, tutti su
          `tools/rt3/` e zero su `Source/` (§5.1).
UNBLOCK:  Per portare la wave a DONE, in quest'ordine:
          1. il working tree sotto `Source/` deve tacere — appartiene a un'altra sessione e non
             è rimuovibile da qui; senza, ogni build della seduta compila codice estraneo;
          2. una seduta PIE esegue `PIE-V01-LOG`, rimisura la clausola (c) di `PIE-V01-COLL` e
             legge la riga del turno 4. Sono i tre USER_REQUIRED, e non li produce nessuna suite;
          3. i dieci conflitti della PR si risolvono da parte dell'owner di `tools/rt3/`, oppure
             `main` assorbe prima quel lavoro. Non da questa wave: arbitrare due stesure di
             `store.py` è fuori scope per un DEV-LEAD della #79;
          4. la full suite su `origin/main` `11e55f57` chiude `1-F12` (#2629), e una sessione
             VALIDATION la esegue.
          ⛔ Nessuno dei quattro chiede una modifica al codice della #79.

RISULTATO: PARTIAL
