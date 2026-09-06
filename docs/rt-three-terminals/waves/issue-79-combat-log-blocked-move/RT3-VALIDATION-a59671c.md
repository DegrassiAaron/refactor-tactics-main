=== RT3 HANDOFF ===

FROM:          VALIDATION
TO:            DEV-LEAD
FEATURE:       issue-79-combat-log-blocked-move
WAVE_ID:       issue-79-combat-log-blocked-move/1

BRANCH:        fix/79-blocked-move-turnlog  (esiste solo in locale, assente su `origin`;
               3 commit propri, tutti di documentazione — nessun codice di produzione)
PARENT_BRANCH: main  — dichiarato dal WORK-ORDER emesso da DEV-LEAD durante questa sessione
BASE_SHA:      a59671c87a7fe3407c1d3158521280267857d20b
               ⚠️ NON era risolto quando questa sessione ha aperto: ricevuto come placeholder
               `<BASE_SHA_WAVE>`. È stato dichiarato da DEV-LEAD alle 15:27, mentre scrivevo.
PRODUCED_SHA:  a59671c8 — coincide con `BASE_SHA`: VALIDATION non ha scritto codice.
               L'unica scrittura è questo handoff, che resta **untracked**.
HEAD_AT_CLOSE: 586ad5942b286fcb655c0080c6578cc592390461  (≠ BASE_SHA: vedi §3, finestra 3)

WRITE_SET:     docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/RT3-VALIDATION-a59671c.md
BINARY_ASSETS: nessuno

STATUS: BLOCKED

---

```text
RT3 INIT

Tipo:            VALIDATION
Feature:         issue-79-combat-log-blocked-move
Wave:            issue-79-combat-log-blocked-move/1
Branch:          fix/79-blocked-move-turnlog
Parent branch:   main  (dal WORK-ORDER, emesso a sessione in corso)
Base SHA:        a59671c8  (dal WORK-ORDER; era un placeholder all'apertura)
Expected SHA:    NON RISOLTO — placeholder `<PRODUCED_SHA_EDITOR>`, e nessun EDITOR ha girato
HEAD:            586ad594  (75ab6287 -> a59671c8 -> 586ad594 durante la sessione: §3)
Working tree:    pulito salvo questo handoff (untracked)
Sistemi in scope: dichiarati dal WORK-ORDER, NON verificati da questa sessione — vedi MATRICE
```

⚠️ **Questo handoff descrive un bersaglio che si è mosso tre volte mentre lo misuravo.** La
wave è passata da «inesistente» a «ingresso emesso» nell'arco della sessione. Ogni misura è
datata e attribuita a un `HEAD` esplicito: leggi §3 prima di usare qualunque riga di §5.

---

## 1. Preflight §4 — fail-closed

`RT3_CONTRACT.md` §4 è fail-closed sui campi obbligatori. **Quattro campi su quattro
utilizzabili sono placeholder letterali**, e `WAVE_VALIDATION.md` § *Avvio* rende
`EXPECTED_SHA` e i due `INPUT_HANDOFF` **obbligatori per questo ruolo**.

| Campo | Valore ricevuto | Esito |
|---|---|---|
| Campo | Ricevuto dal mandato | Esito alla chiusura |
|---|---|---|
| `FEATURE` | `issue-79-combat-log-blocked-move` | risolto |
| `BRANCH` | `fix/79-blocked-move-turnlog` | esiste; **nessun codice di produzione** (§3) |
| `BASE_SHA` | `<BASE_SHA_WAVE>` | ✅ **risolto a sessione in corso** dal WORK-ORDER: `a59671c8` |
| `EXPECTED_SHA` | `<PRODUCED_SHA_EDITOR>` | 🔴 **non risolto** — nessun EDITOR ha girato, non esiste il valore |
| `INPUT_HANDOFF_DEV` | `<PATH_RT3_DEVLEAD>` | 🔴 **non risolto** — `RT3-DEVLEAD-<sha7>.md` non esiste |
| `INPUT_HANDOFF_EDITOR` | `<PATH_RT3_EDITOR>` | 🔴 **non risolto** — `RT3-EDITOR-<sha7>.md` non esiste |

```text
STATUS: BLOCKED
REASON: MISSING_INPUT
FIELDS: EXPECTED_SHA, INPUT_HANDOFF_DEV, INPUT_HANDOFF_EDITOR
```

Tre dei quattro campi che `WAVE_VALIDATION.md` § *Avvio* rende obbligatori **per questo ruolo**
restano non risolti — e non per una svista di compilazione: **non esistono ancora**. Nessun
EDITOR ha prodotto un commit, quindi non c'è `EXPECTED_SHA` da verificare.

§4 vieta di colmarli per inferenza: *«un placeholder risolto per inferenza è un input
inventato»*. Non ho dedotto `EXPECTED_SHA` da `HEAD`, né lo scope dal working tree, né gli
oracoli dal corpo della issue.

### Gate d'ingresso del mandato — non soddisfatto

Il mandato stesso apre con una precondizione esplicita:

> *«NON avviare questa sessione finché esistono entrambi `RT3-DEVLEAD-<sha7>.md` e
> `RT3-EDITOR-<sha7>.md`»*

Misurato — sweep su tutto il repository, non solo sulla directory di wave:

```text
find . -name 'RT3-*.md'
  -> docs/.../parsecell-arity/RT3-DEVLEAD-022977f.md
  -> docs/.../parsecell-arity/RT3-DEVLEAD-39f3ec9.md
  -> docs/.../parsecell-arity/RT3-VALIDATION-cdcf1da.md
  -> docs/.../parsecell-arity/RT3-VALIDATION-ADDENDUM-4d79e80.md
```

**Nessun `RT3-DEVLEAD-*.md` e nessun `RT3-EDITOR-*.md` per la #79.** Il gate d'ingresso è
violato, indipendentemente da §4. Rimisurato alla chiusura, dopo l'arrivo del WORK-ORDER:
invariato.

### ⚠️ Il WORK-ORDER non è l'handoff che questo gate richiede

Alle 15:27 DEV-LEAD ha emesso
`docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/WORK-ORDER.md` (commit
`ef3587c9`, poi `586ad594`). È l'**ingresso** della wave e risolve il blocco delle due
istanze DEV: porta `BASE_SHA`, `PARENT_BRANCH`, `CONTRACT SOURCE`, `EXPECTED BEHAVIOR`, gli
`ASSIGNED_SCOPE` e gli arbitrati.

Non sblocca VALIDATION, e il file stesso lo dice in apertura:

> *«Non è un handoff §9: non porta verdetti, e non li porterà — DEV-LEAD non compare in nessuna
> colonna della matrice §7.»*

`RT3-DEVLEAD-<sha7>.md` è l'handoff di **uscita** di DEV-LEAD: nasce a implementazione fatta,
porta il `WRITE_SET` reale e un `PRODUCED_SHA` che qui non esiste ancora. Un ingresso di wave
e un handoff di consegna non sono lo stesso artefatto, e §10 li distingue per nome. VALIDATION
sta a valle di **entrambi** i punti fissi che la precedono: DEV-LEAD *e* EDITOR.

---

## 2. §11 — propagazione dello stato upstream

`contrib/` della wave contiene due contributi DEV, entrambi depositati alle 15:18 di oggi:

| Contributo | STATUS | REASON |
|---|---|---|
| `contrib/DEV-MAIN-39180-01.md` | `BLOCKED` | `MISSING_INPUT` — `BASE_SHA`, `INPUT_HANDOFF`, `ASSIGNED_SCOPE` |
| `contrib/DEV-TEST-60316-01.md` | `BLOCKED` | `MISSING_INPUT` — `BASE_SHA`, `INPUT_HANDOFF`, `CONTRACT_SOURCE` |

Un contributo non è uno dei tre punti fissi, e §11 parla di handoff: formalmente non è la
loro `BLOCKED` a propagarsi. Sostanzialmente il quadro è più semplice — **le tre istanze
della wave hanno ricevuto lo stesso mandato con gli stessi placeholder, e tutte e tre si
sono fermate allo stesso punto.** Il difetto era nell'ingresso della wave, non nei ruoli.

⚠️ Entrambi i contributi misurano il branch come **assente**. È corretto per le 15:18: il
branch è stato creato dopo. Vedi §3.

✅ **Risolto durante la sessione.** Il WORK-ORDER delle 15:27 dichiara `BASE_SHA` e
`PARENT_BRANCH`, crea il branch e assegna gli scope: è esattamente l'`UNBLOCK` che i due
contributi chiedevano, ed entrambi possono ripartire con un contributo `-02`. Lo registro
perché cambia la lettura di questo handoff: **la catena a monte non è ferma, è al proprio
primo stadio.** VALIDATION resta a valle di due stadi che non sono ancora avvenuti.

---

## 3. §5 — precondizioni del repository, misurate

### Finestra 1 — NON VALIDA

```text
git rev-parse HEAD              -> 75ab62872f9c4e7f083f3328bb5b1213455b0898
git rev-parse --abbrev-ref HEAD -> main
git status --porcelain          -> 0 righe
```

### Finestra 2 — durante la review

```text
git rev-parse HEAD              -> a59671c87a7fe3407c1d3158521280267857d20b
git rev-parse --abbrev-ref HEAD -> fix/79-blocked-move-turnlog
git status --porcelain          -> ?? docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/
git rev-list --left-right --count origin/main...HEAD -> 0  0
```

### Finestra 3 — stato alla chiusura

```text
git rev-parse HEAD              -> 586ad5942b286fcb655c0080c6578cc592390461
git rev-parse --abbrev-ref HEAD -> fix/79-blocked-move-turnlog
git rev-list --left-right --count origin/main...HEAD -> 0  3
git log --oneline -3
  586ad594 docs(rt3): toglie gli escape spuri dalle due citazioni del DoD #79
  ef3587c9 docs(rt3): work order della wave issue-79-combat-log-blocked-move
  c3f18b98 docs(rt3): persiste i contributi DEV-MAIN e DEV-TEST della wave #79
```

🔴 **`HEAD` è cambiato due volte durante la sessione**, la prima insieme al branch corrente.
`CLAUDE.md` §6 e `RT3_CONTRACT.md` §5 sono espliciti: *«HEAD, working tree, binari o processi
Unreal cambiano durante una finestra di misura — in quel caso la misura è NON VALIDA, non
FAIL»*. **Tutto ciò che ho letto nella finestra 1 è NON VALIDO come misura di `a59671c8`** ed
è stato riverificato nella finestra 2 prima di comparire in questo handoff.

Violazioni §5 alla chiusura, ciascuna già sufficiente a `BLOCKED`:

1. `HEAD` (`586ad594`) non corrisponde a `EXPECTED_SHA`, che **non è un valore**. Non
   corrisponde nemmeno a `BASE_SHA` (`a59671c8`), ed è corretto che non lo faccia: i tre
   commit sopra la base sono di DEV-LEAD;
2. il branch di task **non porta codice di produzione**. I 3 commit sono `docs(rt3)`: work
   order e contributi. `git diff --stat a59671c8 586ad594 -- Source/ Scenarios/` è vuoto.
   Non esiste un diff da revisionare, non esiste un `PRODUCED_SHA` EDITOR da verificare, e
   `BINARY_ASSETS` non è confrontabile con niente;
3. il branch è **solo locale**: `git branch -r --list '*79*'` è vuoto dopo `git fetch --all`.

### Coerenza write-set e binari

`N/A — REASON: nessun WRITE_SET dichiarato in ingresso.` Non c'è insieme da confrontare.
Un `N/A` giustificato dal write-set assente, non dal costo (§6).

---

## 4. Capability RT3 — misurata, non presunta

Il mandato chiede di dichiarare `BLOCKED` se la sessione non ha accesso reale a build, test
runner e lease. Misurato:

| Elemento | Misura | Esito |
|---|---|---|
| `rtstatus` | `pwsh -NoProfile -c rtstatus` | **non riconosciuto** — «The term 'rtstatus' is not recognized» |
| `rtstatus`, `rtlease`, `rtsuite`, `rtws`, `rtmcp` | `Get-Command` **con** profilo | nessuno risolve |
| profilo PowerShell | `$PROFILE.CurrentUserAllHosts` | il path indicato **non esiste** |
| `scripts/rt-suite.ps1` | `ls` | **presente** (83 293 byte) |
| `scripts/rt-lease.ps1` | `ls` | **presente** |
| `RT_TERMINAL_ROLE` / `RT_WORKSPACE_ID` | `env` | `VALIDATION` / `MAIN` — coerenti col ruolo |

Gli **script** esistono; i **wrapper** che `TERMINAL_VALIDATION.md` impone di usare
(*«usa `rtsuite ...` invece di invocare direttamente `scripts/rt-suite.ps1`»*) non sono
risolvibili in questa shell. `rtws -Action verify`, che `CLAUDE.md` §2 dichiara **la sola
fonte autorevole** dell'identità di workspace, non è eseguibile: l'identità `MAIN` qui viene
dalle variabili d'ambiente, che lo stesso §2 dichiara non autorevoli.

⚠️ **Questa non è la ragione del `BLOCKED`.** Anche con la capability piena non ci sarebbe
nulla da misurare: il branch è vuoto. Lo registro perché il mandato lo chiede espressamente e
perché va risolto **prima** della vera finestra VALIDATION.

### Perché non ho acquisito il lease

Deliberato. Un `rtlease -Action acquire` avrebbe occupato Unreal per misurare un branch a
diff zero, mentre altre istanze lavorano sulla stessa macchina — con l'unico effetto di
bloccare il motore a chi ne ha bisogno. `performed = 0` non sarebbe stato `PASS` comunque.

---

## 5. Review avversariale anticipata — `OBSERVED`, non verdetti

Il mandato chiede la review prima della build. Non esiste un diff: quanto segue è la
**review della specifica del mandato contro il repository su `a59671c8`**, e per §6 vale
`OBSERVED` — un'osservazione registrata, che **non conta come `PASS`** e non anticipa alcun
verdetto sul codice che DEV produrrà.

Ha comunque valore adesso: cinque dei tredici punti di review sono già decidibili sui dati
in `main`, e due sono **decisioni aperte** che DEV non può risolvere da solo.

### 5.1 🔴 L'oracolo `BlockedByUnit = 1` del mandato è un obiettivo, non un contratto

Il mandato lo presenta come *«il contratto finale previsto»*. Misurato in
`Scenarios/Movement/CollisionChoke.json`, `expect`, su `a59671c8`:

```json
{ "type": "LogEventCount", "category": "Move", "outcome": "BlockedContested", "value": 4 }
{ "type": "LogEventCount", "category": "Move", "outcome": "BlockedByUnit",    "value": 0 }
{ "type": "LogEventCount", "category": "Move", "outcome": "Moved",            "value": 1 }
```

`BlockedByUnit` oggi è asserito **`0`**, non `1`. E quello `0` non è una svista: il file
dichiara accanto che è un valore **misurato** il 2026-09-04 e che la prima stesura asseriva
`1` derivandolo dalla regola. Il commento dell'owner sulla issue #79 (2026-09-04) è ancora
più preciso su cosa fare alla chiusura:

> *«Chi chiude #79 aggiorni quel valore nello scenario (**`1`, o il conteggio che il nuovo
> evento produce**).»*

⚠️ **Conseguenza per VALIDATION.** `1` è l'ipotesi da confermare, non l'oracolo da difendere.
Un `1` che arriva perché qualcuno ha scritto `1` nell'`expect` **è esattamente il punto di
review «golden rigenerati senza motivazione»**, applicato a sé stesso. Alla vera finestra di
validazione il numero va derivato dal contratto che DEV-LEAD dichiara — un evento per
tentativo rifiutato, e in `CollisionChoke` il tentativo rifiutato è **uno solo** (T4) — e poi
misurato. Se misura e contratto divergono, la divergenza è una scoperta.

✅ `BlockedContested = 4` e `Moved = 1` coincidono con l'`expect` corrente e restano
**in scope come regressione** (§8 punto 3): sono i due numeri che dicono se il fix ha
disturbato il percorso già funzionante.

✅ **Convergenza col WORK-ORDER, registrata perché conta.** Il §*CONTRACT SOURCE* emesso da
DEV-LEAD alle 15:27 prende, indipendentemente, la stessa posizione: *«i quattro numeri di
`CollisionChoke` non vanno presi dal prompt di chat […] il quarto (`BlockedByUnit`) è il solo
che questa wave cambia, e il suo valore nuovo si deriva dal contratto — non da una run»*, e
l'`ASSIGNED_SCOPE` di DEV-TEST aggiunge: *«Se la misura desse un numero diverso, è una
scoperta, non un valore da riallineare»*. Due ruoli sono arrivati alla stessa regola da due
letture separate. **Il rischio è chiuso a monte**; resta la verifica a valle — che il numero
finalmente scritto sia quello derivato, e che la nota accanto sia stata riscritta (criterio
di accettazione 8 del WORK-ORDER).

### 5.2 🔴 «Logging fatto solo nel Scenario Harness» non è un rischio: è il difetto attuale

Il mandato lo elenca fra le cose da cercare nel diff. È già lo stato di `main`, documentato
e misurato due volte. Verificato da me sul sorgente:

```text
Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp:1348
  Notes.Add(... "turno %d: percorso rifiutato per '%s': l'unita' resta ferma" ...)
Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp:1350
  UE_LOG(LogRT, Warning, TEXT("[RT-Test] %s: percorso rifiutato per '%s' (l'unita' resta ferma)"), ...)
```

L'unica traccia del denial è emessa **dal runner degli scenari**. In partita normale quella
riga non esiste — confermato in PIE, seduta `U14` del 2026-09-04 su `PIE-V01-COLL`, dove il
combat log mostra la **riga identica** per «non ho dichiarato niente» (T3) e «ho tentato il
varco occupato» (T4):

```text
Turno 3   Gadget: resta (q=-1,r=0,L=0) (Action.Move, p50)
Turno 4   Gadget: resta (q=-1,r=0,L=0) (Action.Move, p50)
```

📌 **Correzione a un puntatore che circola nella issue.** I due commenti citano
`RTScenarioSession.cpp:1338` e `FindPathForUnit`. Su `a59671c8` le righe reali sono
**1348-1350** e la chiamata a monte è `BuildCompositeHexPath` (riga 1339), non
`FindPathForUnit`. Il meccanismo descritto regge; il puntatore no. Chi implementa parta dalle
righe misurate qui.

### 5.3 🔴 «Bot o altro produttore che crea lo stesso stato»: sono **tre**, misurati

Il punto di review più concreto della lista. Il rifiuto del piano nasce in tre chiamanti
distinti, e nessuno dei tre passa dagli altri:

| Produttore | Sito | Funzione |
|---|---|---|
| Player | `Source/RefactorTactics/Player/RTPlayerController.cpp:1626, 1638, 2053` | `BuildCompositeHexPath` |
| Scenario Harness | `Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp:1339` | `BuildCompositeHexPath` |
| Bot | `Source/RefactorTactics/Bot/RTHexBotLibrary.cpp:688` | `FindPathForUnit` |

⛔ Un evento emesso in `RTScenarioSession` renderebbe verde `CollisionChoke` **lasciando il
difetto intatto in partita** — è la situazione di oggi, spostata di un livello.

✅ **Chiuso a monte per i primi due.** Il WORK-ORDER esclude il punto 5 dei falsi punti di
partenza (*«correggere il solo `RTScenarioSession`»*) e impone al criterio 4 che *«il percorso
del Scenario Harness produca lo stesso esito del percorso del controller — una sola regola,
due chiamanti»*, serializzando il lavoro su `RTScenarioSession.cpp` come `INTEGRATION-OWNED`.

🔴 **Residuo non coperto: il terzo chiamante.** Il criterio 4 dice «due chiamanti». Ne ho
misurati tre, e il terzo — `RTHexBotLibrary.cpp:688` — usa una funzione **diversa**
(`FindPathForUnit`, non `BuildCompositeHexPath`) e non compare né negli `ASSIGNED_SCOPE` né
fra gli `INTEGRATION-OWNED FILES`.

⚠️ **Non ho dimostrato che il bot possa produrre il caso B**, e non lo affermo. Ciò che ho
misurato è che il bot ha un ramo esplicito per «percorso assente» — `ReservePlannedRoute`,
`Found.Path.Num() < 2` (`RTHexBotLibrary.cpp:695-713`) — che distingue «resto fermo» da «non
c'è rotta», logga un `Warning` e prenota la sola destinazione. Il commento a `:719` dichiara
che `FindPathForUnit` *«le celle altrui le evita»*, il che rende il caso B improbabile per
costruzione — **ma è un'affermazione del commento, non una misura.**

La domanda per DEV-LEAD è una sola e costa poco: *se un bot dichiara una destinazione che a
risoluzione risulta occupata da un'unità ferma, quale voce produce?* Se la risposta è
`Stayed`, il criterio 4 va esteso al terzo chiamante o va dichiarato `N/A` con la ragione. Se
il bot non può generare lo stato, dirlo chiude il punto per iscritto invece di lasciarlo
implicito in «due chiamanti».

### 5.4 ⚠️ «Stayed duplicato con BlockedByUnit» è una **decisione aperta**, non un difetto da cercare

Le due righe del PIE qui sopra mostrano che un evento «resta» **esiste già** e viene emesso
anche quando l'unità non ha dichiarato nulla. Aggiungere `BlockedByUnit` senza decidere il
rapporto fra i due produce, nel caso T4, o due voci per un fatto solo o una voce che ne
sostituisce un'altra in un percorso e non nell'altro.

`contrib/DEV-MAIN-39180-01.md` chiede lo stesso arbitrato con parole proprie — *«se `Stayed`
e `BlockedByUnit` sono alternativi o coesistenti nel TurnLog»* — e si ferma invece di
sceglierlo. Ha ragione: è contratto, non implementazione.

✅ **Deciso alle 15:27, prima che questo handoff fosse consegnato.** Il WORK-ORDER §*Arbitrati
DEV-LEAD* punto 1 risponde **alternativi**: la voce di blocco **sostituisce** `Stayed`, con
tre ragioni verificabili — `BuildMoveLog` emette una voce per unità per fase
(`RTHexSimLibrary.cpp:1146-1160`), il precedente `BlockedByTopology` **riscrive** l'outcome
dopo `BuildMoveLog` (`RTTurnManager.cpp:7320`), e `Stayed` significa per dichiarazione «non
pianificava movimento» (`RTTurnLog.h:450`), quindi lasciarlo su chi ha pianificato **è** il
difetto.

Il rischio che il mandato chiedeva di cercare — «evento `Stayed` duplicato con
`BlockedByUnit`» — è quindi già normato: **due voci `Move` per la stessa unità nella stessa
fase sono una violazione del contratto**, non un dettaglio di stile. Alla vera finestra di
validazione è un oracolo binario, e va misurato sui campi della voce, non sul conteggio.

### 5.5 Punti non decidibili senza il diff

Restano aperti e li rimetto in coda alla vera finestra di validazione, senza fingere di
averli valutati:

| Punto di review | Stato |
|---|---|
| evento su ogni click invalido invece che sul piano finale | `NOT RUN — nessun diff` |
| pending rejection non cancellato dal replan | `NOT RUN — nessun diff` |
| pending rejection non resettato al nuovo turno | `NOT RUN — nessun diff` |
| `ActionId`/`Priority` hardcoded | `NOT RUN` — ⚠️ nota: `Priority` per voce è in `main` dalla #419, formato **v7**, funzione del catalogo. Un letterale in un nuovo sito sarebbe una regressione su una decisione già presa |
| `TMap` iterata per produrre ordine | `NOT RUN` — invariante `CLAUDE.md` §7 |
| TurnLog creato senza wrapper canonico | `NOT RUN — nessun diff` |
| leak di planning prima del commit | `NOT RUN` — ⚠️ il denial rivela l'**occupazione** di una cella avversaria: se l'evento nasce in pianificazione, il confine privacy va rivisto esplicitamente (`CLAUDE.md` §7) |
| cambi di serializzazione non dichiarati | `NOT RUN` — ⚠️ il TurnLog è a **v7**; un outcome nuovo tocca formato, `EntryLess`, hash e corpus golden `*.rttl`. Da dichiarare nell'handoff, non da scoprire a valle |
| golden rigenerati senza motivazione | vedi §5.1 — qui la rigenerazione è **attesa e già motivata in anticipo**; ciò che va motivato è il **numero** |
| PlayerController vs Scenario Harness | vedi §5.2 e §5.3 — oggi **divergono**, ed è il difetto |

---

## 6. Definition of Done viva della #79 — riletta oggi

§13 impone di rileggerla alla chiusura, non di citarla da un handoff. Riletta il 2026-09-06
da `gh issue view 79` e dal repository.

⚠️ **Attenzione a una citazione che circola.** Il primo commento della issue si chiude con
*«Il DoD di codice è completo. Resta la sola `PIE-V01-LOG`»*. È del **2026-08-10** ed è stata
**superata**: i due commenti del 2026-09-04 riaprono un difetto di codice. Chi legge solo
quella riga conclude che manchi soltanto una seduta PIE. Non è così.

| Voce DoD | Stato misurato |
|---|---|
| `ActionId`, `Priority`, coord `(q,r,L)`, bersaglio, `ValidationResult` per voce | consegnato — #408 e #419, formato v7 |
| **Fallback applicati espliciti** («percorso bloccato → fermo») | 🔴 **APERTO** — è il bersaglio di questa wave |
| Modifiche ambientali e strutturali con la loro causa | non misurato in questa sessione |
| Log coerente col TurnLog serializzato: stesse info, stesso ordine | test presenti, vedi sotto |
| `RefactorTactics.UI.LogContainsReasonAndCoords` | **esiste** — `Source/RefactorTactics/Tests/RTCombatLogTests.cpp:155-156` |
| `RefactorTactics.UI.LogMatchesTurnLogOrder` | **esiste** — `Source/RefactorTactics/Tests/RTCombatLogTests.cpp:193-194` |
| Verifica PIE `PIE-V01-LOG` | 🔴 **`NOT RUN`** — `docs/roadmap/plans/hud-v01-editor-verification-roadmap.md:71`, sedute U15 · U43 · U46 |

E due conseguenze di chiusura che l'owner ha dichiarato in anticipo, entrambe **fuori dalla
portata di una suite headless**:

1. aggiornare l'`expect` di `Movement.CollisionChoke` al conteggio che il nuovo evento
   produce (§5.1);
2. **rimisurare la clausola (c) di `PIE-V01-COLL`**, oggi registrata `❌` in
   `docs/technical/test-manuali-pie.md:1001`, con la voce ferma a 🟡 in attesa della #79.

🔑 **Conseguenza operativa, ed è la risposta alla domanda del mandato.** Anche a wave
completata e suite mirata verde, **la #79 non è tecnicamente chiudibile da VALIDATION**: due
voci della sua DoD viva hanno oracolo umano in PIE. Restano `USER_REQUIRED`. Una chiusura su
sola suite verde chiuderebbe la issue sopra la parte di DoD che nessun test headless osserva
— ed è precisamente il difetto che la seduta U14 ha trovato.

---

## MATRICE

Lo scope §8 non è più vuoto: il WORK-ORDER lo dichiara. Ma è lo scope **previsto** da un
write-set che non esiste ancora — nessun file di `Source/` o `Scenarios/` è stato toccato
(§3). Compilo la colonna VALIDATION per i sistemi che il WORK-ORDER mette in scope, con
l'unico verdetto che ho titolo a emettere:

| # | Sistema | VALIDATION | REASON |
|---:|---|---|---|
| 3 | BUILD | `NOT RUN` | nessun codice da compilare; wrapper `rtsuite` non risolvibile (§4) |
| 12 | PLANNING | `NOT RUN` | write-set vuoto |
| 13 | READY/COMMIT | `NOT RUN` | write-set vuoto |
| 15 | MOVEMENT | `NOT RUN` | write-set vuoto |
| 25 | UI/HUD | `NOT RUN` | write-set vuoto — tetto `OBSERVED` per §7 |
| 27 | COMBAT LOG | `NOT RUN` | write-set vuoto |
| 28 | TURNLOG/REPLAY | `NOT RUN` | write-set vuoto |
| 29 | DETERMINISM | `NOT RUN` | write-set vuoto |
| 32 | AUTOMATION/SCENARIO | `NOT RUN` | nessuna suite eseguita; `performed = 0` |
| 35 | SAVE/RELOAD | `NOT RUN` | write-set vuoto |
| 30 | NETWORK AUTHORITY | `NOT RUN` | non-degrado da verificare, nulla da verificare ancora |
| 31 | PRIVACY | `NOT RUN` | idem |

Gli altri sistemi: `N/A — REASON: fuori write-set previsto`, come li elenca il WORK-ORDER.

§6: **`NOT RUN` non conta come `PASS`.** §11: nessuna di queste voci diventa `PASS` a valle
per ereditarietà. **Nessun sistema della matrice canonica è stato verificato da questa
sessione.**

## FINDINGS

```text
FINDING_ID:   issue-79-combat-log-blocked-move/1-F1
SEVERITY:     P2   (era P1 all'apertura — declassato: vedi STATO)
EVIDENCE_REF: docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/contrib/DEV-MAIN-39180-01.md
              docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/contrib/DEV-TEST-60316-01.md
              docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/WORK-ORDER.md
              (questo file, §1 e §2)
ROOT_CAUSE:   I quattro terminali della wave sono stati avviati IN PARALLELO all'ingresso,
              non dopo, distribuendo un mandato con i campi §4 non compilati. Tre istanze —
              DEV-MAIN, DEV-TEST, VALIDATION — hanno speso una sessione ciascuna per fermarsi
              al preflight su un ingresso che non era ancora stato emesso.
OWNER:        DEV-LEAD
STATO:        RISOLTO PER DEV, APERTO PER VALIDATION. Il WORK-ORDER delle 15:27 sblocca le due
              istanze DEV. Non sblocca questa: VALIDATION sta a valle di DEV-LEAD *e* di
              EDITOR, e nessuno dei due handoff di consegna esiste.
REQUIRED_FIX: Sequenziare l'avvio dei ruoli: l'ingresso prima dei DEV, i due handoff di
              consegna prima di VALIDATION. Un terminale VALIDATION avviato prima che EDITOR
              abbia prodotto un commit non può che emettere BLOCKED.
REGRESSION:   nessun test automatico: è processo. Il controllo è il preflight §4 stesso, che
              ha funzionato — tre istanze su tre lo hanno applicato senza inventare input.
ATTEMPT:      1
```

```text
FINDING_ID:   issue-79-combat-log-blocked-move/1-F2
SEVERITY:     P3   (era P2 all'apertura — declassato: risolto a monte)
EVIDENCE_REF: Scenarios/Movement/CollisionChoke.json (expect: BlockedByUnit = 0)
              gh issue view 79 --comments  (commento 2026-09-04T10:07:26Z)
              WORK-ORDER.md §CONTRACT SOURCE e §DEV-TEST ASSIGNED_SCOPE punto 4
              (questo file, §5.1)
ROOT_CAUSE:   Il mandato dichiara `BlockedByUnit = 1` come «contratto finale previsto». Lo
              scenario asserisce `0`, ed è un valore MISURATO che il file documenta come «la
              misura di un buco, non un invariante». Un oracolo dato per acquisito prima del
              contratto trasforma la rigenerazione del golden in un adeguamento silenzioso.
OWNER:        DEV-LEAD
STATO:        RISOLTO A MONTE, prima della consegna di questo handoff. Il WORK-ORDER prescrive
              che il valore si derivi dal contratto e non da una run, e che una divergenza sia
              «una scoperta, non un valore da riallineare». Resta come voce perché il rischio
              si realizza a valle, non a monte: la disciplina va verificata sul file finale.
REQUIRED_FIX: nessuno a monte. A valle: verificare che il numero scritto sia quello derivato
              dal contratto e che la nota dell'assertion sia stata riscritta (criterio 8).
REGRESSION:   `Movement.CollisionChoke` deve restare rosso finché il contratto non è
              implementato, e diventare verde sul valore derivato — non su quello misurato.
ATTEMPT:      1
```

```text
FINDING_ID:   issue-79-combat-log-blocked-move/1-F3
SEVERITY:     P2
EVIDENCE_REF: Source/RefactorTactics/Player/RTPlayerController.cpp:1626,1638,2053
              Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp:1339,1348,1350
              Source/RefactorTactics/Bot/RTHexBotLibrary.cpp:688
              (questo file, §5.3 e §5.4)
ROOT_CAUSE:   Due decisioni di contratto che l'implementazione non può prendere da sola:
              (a) quale dei produttori di «piano rifiutato» è il punto di estensione;
              (b) se `Stayed` e `BlockedByUnit` sono alternativi o coesistenti nel TurnLog.
OWNER:        DEV-LEAD
STATO:        (a) e (b) RISOLTE dal WORK-ORDER (§Arbitrati 1 e 3; criterio 4; falso punto di
              partenza 5). RESIDUO APERTO: i chiamanti misurati sono TRE, non due. Il bot
              (`Bot/RTHexBotLibrary.cpp:688`) usa `FindPathForUnit`, non
              `BuildCompositeHexPath`, e non compare in nessuno scope né fra gli
              INTEGRATION-OWNED. Non è dimostrato che possa produrre il caso B — né che non
              possa. Vedi §5.3: il commento a `:719` lo esclude a parole, non con una misura.
REQUIRED_FIX: Rispondere per iscritto: un bot che dichiara una destinazione risultata occupata
              da un'unità ferma quale voce produce? Se `Stayed`, estendere il criterio 4 al
              terzo chiamante o marcarlo `N/A` con la ragione.
REGRESSION:   un caso che copra il percorso NON-harness per lo stesso stato, altrimenti
              `CollisionChoke` verde non prova niente sulla partita.
ATTEMPT:      1
```

```text
FINDING_ID:   issue-79-combat-log-blocked-move/1-F4
SEVERITY:     P3
EVIDENCE_REF: (questo file, §4)
ROOT_CAUSE:   I wrapper `rtstatus`, `rtlease`, `rtsuite`, `rtws`, `rtmcp` non sono risolvibili
              in questa shell; il path del profilo PowerShell non esiste. Gli script in
              `scripts/` ci sono. `TERMINAL_VALIDATION.md` impone `rtstatus` prima di occupare
              Unreal e `rtsuite` al posto della chiamata diretta: entrambi ineseguibili qui.
              `CLAUDE.md` §2 dichiara non autorevole l'identità di workspace da variabile
              d'ambiente, e `rtws -Action verify` è l'unica fonte autorevole.
OWNER:        DEV-LEAD
REQUIRED_FIX: Rendere risolvibili i wrapper nella shell del terminale VALIDATION prima della
              finestra di misura, oppure autorizzare esplicitamente la chiamata diretta agli
              script con il mutex equivalente.
REGRESSION:   `rtstatus` deve rispondere `terminal role: VALIDATION` / `engine mode: VALIDATION`
              prima di qualunque `PASS` su BUILD, AUTOMATION o PACKAGED.
ATTEMPT:      1
```

## EVIDENCE

```text
git:      git rev-parse HEAD -> 586ad5942b286fcb655c0080c6578cc592390461  (chiusura)
git:      git rev-parse --abbrev-ref HEAD -> fix/79-blocked-move-turnlog
git:      git rev-list --left-right --count origin/main...HEAD -> 0  3
git:      git diff --stat a59671c8 586ad594 -- Source/ Scenarios/ -> vuoto (0 file)
git:      git diff --stat a59671c8 586ad594 -> 3 file, 674 inserzioni, tutti sotto waves/
git:      git branch -r --list '*79*' -> vuoto (dopo git fetch --all --prune)
git:      git status --porcelain -> ?? .../RT3-VALIDATION-a59671c.md  (questo file)
fs:       find . -name 'RT3-*.md' -> 4 file, tutti in waves/parsecell-arity/; nessuno per la #79
file:     docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/WORK-ORDER.md (commit ef3587c9)
source:   Source/RefactorTactics/Bot/RTHexBotLibrary.cpp:695-719  (ramo «percorso assente» del bot)
file:     docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/contrib/DEV-MAIN-39180-01.md
file:     docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/contrib/DEV-TEST-60316-01.md
scenario: Scenarios/Movement/CollisionChoke.json (expect: BlockedContested=4, BlockedByUnit=0, Moved=1)
source:   Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp:1339,1348,1350
source:   Source/RefactorTactics/Player/RTPlayerController.cpp:1626,1638,2053
source:   Source/RefactorTactics/Bot/RTHexBotLibrary.cpp:688
source:   Source/RefactorTactics/Tests/RTCombatLogTests.cpp:155-156, 193-194
doc:      docs/technical/test-manuali-pie.md:1001  (PIE-V01-COLL, clausola (c) ❌, voce 🟡)
doc:      docs/roadmap/plans/hud-v01-editor-verification-roadmap.md:71  (PIE-V01-LOG, NOT RUN)
issue:    gh issue view 79 --comments  (commenti 2026-09-04T10:07:26Z e 2026-09-04T16:01:41Z)
shell:    pwsh -NoProfile -c rtstatus -> «The term 'rtstatus' is not recognized»
```

Nessun `EVIDENCE_REF` di build, suite, Automation, Scenario Harness, TurnLog dump, replay,
determinismo, packaged o PIE. Non ne esistono: nessuno di quei gate è stato eseguito.

## USER_REQUIRED

```text
PIE-V01-LOG                     Result: NOT RUN   — DoD #79, oracolo umano, sedute U15/U43/U46
PIE-V01-COLL clausola (c)       Result: NOT RUN   — da rimisurare alla chiusura #79; oggi ❌
Riga di combat log del turno 4  Result: NOT RUN   — «fermo: cella occupata» invece di «resta»
                                                    (attesa Editor-visible del WORK-ORDER)
```

Non `USER_REQUIRED`, ma da chiudere prima della riapertura di VALIDATION:

```text
Arbitrato Stayed vs BlockedByUnit      RISOLTO   — WORK-ORDER §Arbitrati 1: alternativi
Sito di estensione fra i produttori    RISOLTO   — WORK-ORDER §Arbitrati 3 + criterio 4
Il bot è un terzo chiamante?           APERTO    — F3, residuo. Risposta scritta, non misura
```

---

## Nota di processo — dichiarata, non nascosta

§4 prescrive l'arresto **prima** di ispezionare il repository. Questa sessione ha ispezionato
in sola lettura dopo aver rilevato i placeholder. La motivo e la registro invece di
presentarla come conforme:

- §4 blocca su un campo *«vuoto, placeholder, o non risolvibile»*: accertare la
  **non risolvibilità** richiede di guardare. Sweep dei branch, degli handoff e dei cloni è
  ciò che distingue «input non compilati» da «wave reale, path non incollati» — e qui ha
  cambiato la diagnosi, perché il branch è comparso a metà sessione;
- l'ispezione è andata **oltre** quel minimo: ho letto scenario, sorgenti e issue. Nessun
  valore mancante è stato inferito da ciò che ho letto — `BASE_SHA` ed `EXPECTED_SHA` restano
  non risolti e la matrice resta vuota — ma il confine di §4 è stato superato, e chi rilegge
  deve saperlo per pesare §5 come `OBSERVED` anziché come preflight conforme.

Il materiale che ne è uscito è in §5 e §6, etichettato `OBSERVED`, e serve a DEV-LEAD per
emettere l'ingresso della wave con gli arbitrati già presi.

---

STATUS:   BLOCKED
REASON:   MISSING_INPUT — `EXPECTED_SHA`, `INPUT_HANDOFF_DEV`, `INPUT_HANDOFF_EDITOR` non
          risolti (§4), e non per svista: **non esistono ancora**. `RT3-DEVLEAD-<sha7>.md` e
          `RT3-EDITOR-<sha7>.md` non esistono; il WORK-ORDER emesso a sessione in corso è
          l'INGRESSO della wave, non l'handoff §9 di consegna, e lo dichiara da sé. In
          aggiunta §5: il branch non porta codice di produzione — `git diff a59671c8 586ad594
          -- Source/ Scenarios/` è vuoto — quindi non esiste diff da revisionare né
          `PRODUCED_SHA` EDITOR da verificare.
          ⛔ Questo BLOCKED non è un difetto della wave: è la sua posizione nel tempo.
          VALIDATION è stata avviata al primo stadio di una catena di tre.
UNBLOCK:  Nell'ordine, e i primi due punti sono già fatti:
          1. ✅ DEV-LEAD emette l'ingresso della wave — `WORK-ORDER.md`, commit `ef3587c9`,
             con `BASE_SHA`, `PARENT_BRANCH`, `CONTRACT SOURCE`, `EXPECTED BEHAVIOR`, gli
             `ASSIGNED_SCOPE` e gli arbitrati;
          2. ✅ i contributi DEV-MAIN e DEV-TEST ripartono con un `-02` sui campi risolti;
          3. ⏳ DEV implementa; DEV-TEST scrive i casi; il branch acquisisce commit di
             produzione. DEV-LEAD consolida e emette `RT3-DEVLEAD-<sha7>.md` — l'handoff §9
             di consegna, con `WRITE_SET` reale e `PRODUCED_SHA`;
          4. ⏳ EDITOR esegue la sua parte ed emette `RT3-EDITOR-<sha7>.md` con il proprio
             `PRODUCED_SHA` e i `BINARY_ASSETS`;
          5. ⏳ i wrapper `rtstatus` / `rtsuite` / `rtlease` diventano risolvibili nella shell
             VALIDATION (F4), altrimenti build e suite non sono eseguibili secondo contratto e
             il gate resta `NOT RUN` per ragioni di ambiente invece che di codice;
          6. ⏳ prima della riapertura, DEV-LEAD risponde al residuo di F3: il terzo chiamante
             (bot) può produrre il caso B?
          7. ⏳ VALIDATION riparte con `EXPECTED_SHA` = `PRODUCED_SHA` di EDITOR e i due path
             degli handoff, ed emette `RT3-VALIDATION-<nuovo sha7>.md`.
          ⛔ La chiusura tecnica della #79 richiede in ogni caso `PIE-V01-LOG` e la rimisura
          della clausola (c) di `PIE-V01-COLL`: sono `USER_REQUIRED`, li riconosce anche il
          WORK-ORDER, e non li produce nessuna suite headless.

RISULTATO: BLOCKED
