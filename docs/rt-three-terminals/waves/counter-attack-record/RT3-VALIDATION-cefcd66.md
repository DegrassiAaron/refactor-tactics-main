# RT3 VALIDATION — wave `counter-attack-record/1` (secondo passaggio, misura eseguita)

```text
ROLE:            VALIDATION
TERMINAL:        66468
WORKSPACE_ID:    DEV
ALBERO MISURATO: D:\rt-build-main @ cefcd66e   (worktree, branch fix/2587-f5-copertura-satelliti)
LEASE:           9bde7243de40  SUITE / VALIDATION / task 2587  — acquisito 2026-09-07T08:09:33Z
ISSUE:           2587  — CLOSED        PR #2609 — MERGED 2026-09-07T06:25:08Z
MEASURED_AT:     2026-09-07 08:09Z → 09:0xZ
```

> Il primo passaggio è [`RT3-VALIDATION-f609150.md`](RT3-VALIDATION-f609150.md): `BLOCKED`, motore occupato
> da un'altra figura. Questo referto lo **supera sui gate**, non sui finding di processo, che restano aperti.

---

## 0. Perché `cefcd66e` e non il tip di `main`

Scelta dichiarata, non ripiego silenzioso. `origin/main` è `f6091507`; l'albero misurato è `cefcd66e`. Sono
equivalenti **sui file della wave**:

```text
git merge-base --is-ancestor cefcd66e origin/main            -> 0
git diff cefcd66e origin/main -- RTDefensiveReactionTests.cpp RTTurnManager.cpp -> vuoto
```

La differenza `A → B` sono 13 file di `#2642` (lab panel) e `#2644` (map template), estranei alla wave.
Un worktree pinnato su `f6091507` era stato creato (`D:\rt-wt-val2587`) ma è **freddo**: nessun `Binaries/`,
nessun `Intermediate/PipInstall`. `rt-suite.ps1:1064` documenta il costo di quel primo avvio — 4,8 GB di
dipendenze Python — e il fatto che la run muore proprio lì. `D:\rt-build-main` era già caldo.

⚠️ **E l'HEAD di quel worktree si è mosso durante la sessione**, senza che io lo toccassi: da `cefcd66e`
(branch `fix/2587-f5-copertura-satelliti`) a **`49afee2f`** in *detached*, dentro cui è entrato anche
`#2645`. Me ne sono accorto perché `rt-suite` stampa l'HEAD che misura, e non era quello del mio referto.

L'impatto è stato **verificato, non assunto**:

```text
git diff cefcd66e 49afee2f -- RTTurnManager.cpp RTDefensiveReactionTests.cpp RTReactionPassResult.h
-> vuoto
```

I tre file della wave sono identici. Le misure dei §1–§2 restano valide; la suite del §3 ha girato su
`49afee2f`, che è dichiarato lì.

🔁 **Aggiornamento a fine sessione, e ribalta il §0.** Alla chiusura `origin/main` non è più `f6091507`:
un'altra sessione ha fatto `fetch`, e il ref locale ora vale **`49afee2f`** — lo stesso commit su cui
l'HEAD del worktree era stato spostato. (Io non ho fatto `fetch`: `main` non è stato avanzato da questa
sessione, né in remoto né in locale.)

∴ La suite del §3 **ha misurato esattamente il tip corrente di `main`**, non un albero laterale. Il
bersaglio B del work order — il sign-off su `main` — risulta coperto per accidente, non per disegno:
è la stessa deriva che ha prodotto F15, questa volta a favore. Resta vero che i gate mirati dei §1–§2 sono
stati eseguiti su `cefcd66e`, e che i tre file della wave sono identici fra i due commit.

---

## 1. 🔑 Anti-vacuità — `PASS`

Il gate che la wave non aveva mai superato: nessuno aveva visto quel test **rosso sotto mutazione**.

Il produttore (`RTTurnManager.cpp:4971`) è **un solo `Add` dentro un ciclo**, non due letterali: «scambiare
i due record» non è una modifica locale possibile. La si ottiene facendo scrivere al record un valore
sbagliato ma osservabile — stesso scopo, verificare che l'assertion su quel campo discrimini.

| # | mutazione (riga) | esito | assertion che è diventata rossa |
|---|---|---|---|
| M1 | `Unit->Cell` → `EffectTarget->Cell` (4973) | **ROSSO** | `518` «A: l'origine e' la cella di A» · `524` «B: l'origine e' la cella di B» |
| M2 | `Def.ActionId` → `Def.BaseActionId` (4974) | **ROSSO** | `519` «A: l'ActionId…» *to be Action.Counter, but it was None* · `525` «B: l'ActionId…» *to be Reaction.CounterShot, but it was Action.Counter* |
| M3 | `Unit` → `EffectTarget` (4977) | **ROSSO** | `523` «A: l'autore e' A» *to be 1, but it was 3* · `531` «B: l'autore e' B» *to be 2, but it was 4* |

Nessuna mutazione è passata dalla guardia di premessa: i rossi arrivano dalle righe attese, una per campo.

**Due osservazioni che il conteggio da solo non dà:**

1. **M1 fa cadere anche `TracesTheSideItCameFrom`** — coerente: quel test misura il lato di provenienza,
   che deriva da `SourceCell`. Due guardiani per quel campo.
2. **M2 e M3 fanno cadere `TwoCountersKeepTheirOwnOrigin` e nient'altro.** `ActionId` e `Actor` hanno un
   **unico** guardiano in tutta la famiglia `Reactions.Counter`. Il campo «che prima era cieco» ora è
   coperto da un test solo: se quel test viene disabilitato o indebolito, la copertura torna a zero senza
   che nulla diventi rosso.

---

## 2. ⚠️ Il difetto trovato NEL BANCO, non nel prodotto

La misura finale della sequenza è uscita **rossa**, e la diagnosi ha richiesto quattro run. La riporto
per intero perché il work order prescrive «disfare la mutazione e ricompilare prima della misura finale» —
e questa sessione mostra che **ricompilare non basta**.

| # | binario | partizionamento | `TwoCounters…` |
|---|---|---|---|
| 1 | build baseline (34 azioni) | unity | **verde** |
| 2 | dopo M1/M2/M3 + restore incrementale | `Excluded from unity file: RTTurnManager.cpp` | **ROSSO** |
| 3 | rerun, stesso binario | idem | **ROSSO** (riproducibile) |
| 4 | `-DisableAdaptiveUnity` | unity, ricompila `Module.RefactorTactics.24.cpp` | **verde** |
| 5 | sonda: commento neutro, quindi di nuovo fuori blob | `Excluded…` | **verde** |
| 6 | misura finale, albero pulito | — | **verde** |

Il modo di fallire ai punti 2–3 è preciso: `ActionId` e `BaseActionId` **scambiati fra loro** su entrambe
le unità — due `FName` adiacenti, quindi uno scambio posizionale è silenzioso.

**Il sorgente è corretto in ogni punto del percorso**, verificato riga per riga: `4971-4977` (ordine della
initializer list = ordine dei campi della struct), `5972-5973` (`AttackActionId ← Counter.ActionId`),
`6090-6091` (`E.ActionId ← AttackActionId[a]`). Una sola definizione di `FRTCounterAttack`
(`RTReactionPassResult.h:42`), nessuna compilazione condizionale nell'header.

**La sonda (punto 5) falsifica l'ipotesi ovvia.** Il file era di nuovo *fuori* dalla unity blob, esattamente
come ai punti 2–3, e il test era verde: **non è il partizionamento in sé**. L'unica differenza fra 3 e 5 è
che in mezzo il punto 4 aveva ricompilato `Module.RefactorTactics.24.cpp` — la blob che contiene
`RTTurnManager.cpp`. Tutte le build di mutazione compilavano **solo** `RTTurnManager.cpp` (`[1/4]`),
lasciando quella blob intatta.

⛔ **Causa radice: NON DETERMINATA.** Ciò che è misurato è che un binario prodotto da build incrementali
successive su un file estratto dalla unity blob ha calcolato valori diversi dal sorgente che dichiarava di
compilare, in modo riproducibile, e che una ricompilazione della blob lo ha risanato. Il meccanismo esatto
(object file stantio, doppia definizione risolta dal linker, altro) **non è stato isolato**: dichiararlo
sarebbe fabbricare evidenza.

∴ **Nessuna conclusione negativa sul codice consegnato**: su binario coerente il test è verde 4 volte su 4.

---

## 3. Suite completa — `VALIDA`

```text
[RT-MEASURE] HEAD     49afee2f   albero ae48caf4
[RT-MEASURE] binario  RefactorTactics=2026-09-07 10:39:46
[RT-MEASURE] VALIDA
[RT-MEASURE]   esito  2158/2158 completati, 3 fallimenti
[RT-MEASURE]   durata 02:44
```

`2158/2158`: nessuna troncatura — il difetto silenzioso che non produce rossi. I **tre** fallimenti sono
esattamente quelli che la baseline del work order dichiara preesistenti:

| test | stato |
|---|---|
| `Bot.StallDefinitionsOnTheGeneratedTestArena` | preesistente — `#2556`, tuttora OPEN |
| `Match.Autobattle.EngagesOnTheGeneratedTestArena` | preesistente — `#2556`, tuttora OPEN |
| `IconCatalog.RealCatalogCoversRequiredIds` | preesistente — d'ambiente, i PNG che il commandlet legge non nascono senza libcairo |

**Il quarto rosso della baseline non c'è più**: `Reactions.Counter.TwoCountersKeepTheirOwnOrigin` è
`Result={Success}` nella suite completa, come previsto da `b4a2eca4` (`#2612`).
**Nessun rosso nuovo.** Il conteggio sale da 2133 (2026-09-06) a 2158 per i test aggiunti da `#2642`/`#2644`/`#2645`.

Copertura dei sistemi trasversali dentro questa run, contata sui nomi dei test: determinismo 6 verdi/0 rossi,
replay 8/0, TurnLog 13/0, serializzazione 11/0, golden 12/0.

---

## 4. Matrice — sistemi in scope

| # | sistema | verdetto | evidenza |
|---|---|---|---|
| 2 | ARCHITECTURE | `PASS` | un `Add` unico a sei campi al posto dei sei `Append` paralleli; `checkf` di parallelismo a `5962`; nessun `TMap`/`TSet`/`USTRUCT`/`UObject` introdotto sul percorso |
| 3 | BUILD | `PASS` | `Result: Succeeded`, 116 s la completa, 18-22 s le incrementali — con la riserva di §2 |
| 18 | DAMAGE | `PASS` | nessun rosso di danno in 2158 test; `Counter.DealsDamageToAttacker` e `IgnoresEnvironmentalDamage` verdi |
| 21 | REACTIONS | `PASS` | 5/5 su `Reactions.Counter`, e i 3 campi discriminano sotto mutazione (§1) |
| 27 | COMBAT LOG | `PASS` | i cinque satelliti della voce verificati uno per uno, con l'ancora su `TgtCell` |
| 28 | TURNLOG/REPLAY | `PASS` | 13 TurnLog + 8 replay + 11 serializzazione verdi, 0 rossi |
| 29 | DETERMINISM | `PASS` | 6 determinismo + 12 golden verdi, 0 rossi |
| 32 | AUTOMATION/SCENARIO | `PASS` | run `VALIDA`, 2158/2158, 3 rossi tutti preesistenti e attribuiti — perimetro `49afee2f`, vedi §0 |

Fuori scope, non ampliati: 30 NETWORK AUTHORITY, 31 PRIVACY, 36 PACKAGED, 5 BLUEPRINT.

---

## FINDINGS

```text
FINDING_ID:   counter-attack-record/1-F11
SEVERITY:     MAGGIORE (metodo di misura)
EVIDENCE_REF: build-M1/M2/M3/restore.log -> tutte "[1/4] Compile RTTurnManager.cpp";
              build-unity.log -> "[1/4] Compile Module.RefactorTactics.24.cpp";
              run-restore.log e run-restore2.log rossi, run-unity/commento/finale verdi
ROOT_CAUSE:   NON DETERMINATA. Misurato: dopo una sequenza di mutazioni su un file estratto
              dalla unity blob, la build incrementale produce un binario che calcola valori
              diversi dal sorgente (ActionId/BaseActionId scambiati), in modo riproducibile.
              Ricompilare la blob risana. Il work order prescrive "disfare la mutazione e
              ricompilare prima della misura finale": ricompilare il FILE non basta.
OWNER:        processo / chi scrive i work order di mutation testing
REQUIRED_FIX: la misura finale dopo mutazioni va fatta con -DisableAdaptiveUnity o dopo aver
              forzato la ricompilazione della blob. Altrimenti l'ultima misura e' su un
              binario che nessun sorgente descrive - e qui sarebbe stata letta come una
              REGRESSIONE del refactor.
REGRESSION:   n/a (il codice e' verde su binario coerente)
ATTEMPT:      1
```

```text
FINDING_ID:   counter-attack-record/1-F12
SEVERITY:     MAGGIORE (governance)
EVIDENCE_REF: rt-build.ps1 -> "BLOCKED: WORKSPACE_NOT_REGISTERED" da D:\rt-wt-val2587 e da
              D:\rt-build-main; workspaces.json contiene 3 root, tutti cloni, nessun worktree
ROOT_CAUSE:   rt-build.ps1 e rt-lease.ps1 risolvono l'identita' del workspace dalla cwd contro
              un registro che ammette solo MAIN/DEV/TECHNICAL_DESIGNER. Nessun worktree e'
              registrabile senza sovrascrivere la voce di un clone esistente. Ma il work order
              prescrive proprio "cd D:\rt-build-main; .\scripts\rt-build.ps1": quel comando
              NON E' ESEGUIBILE, e non lo era nemmeno quando e' stato scritto.
OWNER:        processo / owner di rt-build.ps1
REQUIRED_FIX: o il registro accetta i worktree (con workspace_id ereditato dal clone padre), o
              i work order smettono di prescrivere build nei worktree. Oggi il contratto
              impone un isolamento che i suoi stessi strumenti rifiutano.
REGRESSION:   ogni gate di build prescritto in un worktree e' NOT RUN per costruzione
ATTEMPT:      1
```

```text
FINDING_ID:   counter-attack-record/1-F13
SEVERITY:     MINORE (attribuzione)
EVIDENCE_REF: lease.json @ 08:09:33Z -> branch "feat/rt3-task-router", head_sha 4d14d489,
              workspace_root D:\Repositories\refactor-tactict-dev; albero misurato cefcd66e
ROOT_CAUSE:   il lease registra branch e HEAD del WORKSPACE ROOT, non dell'albero su cui il
              motore sta effettivamente lavorando. Una sessione che leggesse il lease per
              sapere cosa gira sul motore leggerebbe l'albero sbagliato - qui, un branch di
              tooling che con la misura non c'entra nulla.
OWNER:        owner di rt-lease.ps1
REQUIRED_FIX: accettare un albero di lavoro esplicito e registrarne branch/sha, oppure
              dichiarare che quei due campi descrivono la sessione e non la misura.
REGRESSION:   n/a
ATTEMPT:      1
```

```text
FINDING_ID:   counter-attack-record/1-F14
SEVERITY:     MINORE (copertura)
EVIDENCE_REF: run-M2.log e run-M3.log -> un solo test rosso, TwoCountersKeepTheirOwnOrigin
ROOT_CAUSE:   ActionId e Actor del record hanno un unico guardiano in tutta la famiglia
              Reactions.Counter. La copertura di due dei sei campi dipende da un test solo.
OWNER:        DEV
REQUIRED_FIX: nessuno obbligatorio. Da sapere prima di toccare quel test: indebolirlo riporta
              a zero la copertura dei due campi senza rendere rosso niente.
REGRESSION:   n/a
ATTEMPT:      1
```

```text
FINDING_ID:   counter-attack-record/1-F15
SEVERITY:     MAGGIORE (isolamento)
EVIDENCE_REF: D:\rt-build-main HEAD cefcd66e alle 08:09Z (branch fix/2587-f5-copertura-satelliti)
              -> 49afee2f detached alle ~10:40Z, con dentro #2645. Rilevato solo perche'
              rt-suite stampa l'HEAD che misura.
ROOT_CAUSE:   il worktree che il work order assegna a VALIDATION e' scrivibile da altre
              sessioni, e nessuno avvisa. Se i file della wave fossero stati toccati, la
              sequenza di mutazioni avrebbe ripristinato da un albero diverso da quello
              misurato all'inizio - e il `git checkout --` del restore avrebbe rimesso
              silenziosamente un'altra versione.
OWNER:        processo / USER
REQUIRED_FIX: i gate si eseguono in un worktree `--detach` creato dalla sessione che misura, e
              l'HEAD si ricontrolla PRIMA e DOPO ogni fase. Qui e' andata bene per accertamento
              (i tre file sono identici fra cefcd66e e 49afee2f), non per costruzione.
REGRESSION:   n/a in questa sessione, verificato
ATTEMPT:      1
```

---

## P0:
- Nessuno sul codice della wave.

## P1:
- **F11** — la misura finale dopo mutazioni e' invalida se non si ricompila la blob unity. Ha prodotto in
  questa sessione un falso rosso perfettamente riproducibile, che senza le tre run di controllo sarebbe
  stato riportato come regressione del refactor.
- **F12** — i work order prescrivono build in worktree che gli strumenti rifiutano.
- **F15** — l'HEAD del worktree di misura si e' mosso sotto la sessione, senza avviso.

## P2:
- **F13** — il lease attribuisce la misura all'albero sbagliato.

## P3:
- **F14** — due campi su sei con un guardiano solo.

## USER_REQUIRED:
1. **F12 è una decisione, non un bug da correggere qui**: o il registro accetta i worktree, o i work order
   smettono di prescriverli. Le due cose oggi si contraddicono.
2. I finding di processo del primo passaggio restano aperti: **F9** (merge senza sign-off, due volte),
   **F8** (bersagli collassati), **F2** (untracked non sopravvivono nel checkout condiviso).
3. Questo referto è **untracked** nel checkout condiviso, su `feat/rt3-task-router` di un'altra sessione.
   Non l'ho committato. Copia di sicurezza in scratchpad.

## EVIDENCE:
Log in `…/scratchpad/`: `build-{baseline,M1,M2,M3,restore,unity,commento,finale}.log`,
`run-{baseline,M1,M2,M3,restore,restore2,unity,commento,finale}.log`.
Suite completa: `D:\rt-build-main\Saved\Logs\rt-suite-val2587.log` — run `VALIDA`, 2158/2158, 3 fallimenti.
Mutazioni applicate e ripristinate da `mutate.ps1`; `D:\rt-build-main` verificato **pulito**
(`git status --porcelain` vuoto) dopo ogni ripristino e alla chiusura.

Lease `9bde7243de40` acquisito alle 08:09:33Z e **rilasciato** a fine misura; motore restituito a
TECHNICAL_DESIGNER, che lo attendeva per `#2652`.

Nessun commit, push, merge o `fetch`: `main` non è stato avanzato.

---

## Verdetto

I gate in scope sono tutti `PASS` e il debito della wave — il test mai visto rosso sotto mutazione — è
**chiuso**: i tre campi discriminano, sulle assertion esatte.

Non è `DONE` per una ragione che non riguarda il codice: **il sign-off che questo referto avrebbe dovuto
dare precede un merge già avvenuto** (`#2594` il 06-09, `#2609` il 07-09). Ciò che qui è verificato è una
regressione post-merge, non l'autorizzazione a mergiare che il contratto prevede. Restano inoltre aperti
F8/F9/F2 dal primo passaggio e F11–F15 da questo.

RISULTATO: PARTIAL
NEXT_WAVE_AUTHORIZED: no
