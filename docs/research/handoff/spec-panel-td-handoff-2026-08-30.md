# Spec Panel — Revisione del "Technical Designer Integration Handoff"

**Documento sotto revisione**: RefactorTactics — Claude Cloud Execution Handoff (consolidato 2026-08-30)
**Modalità**: critique
**Focus**: requirements · architecture · testing
**Panel**: Wiegers (lead requisiti), Adzic, Cockburn, Fowler, Nygard, Crispin, Hightower
**Data revisione**: 2026-08-30

> Questa è una revisione della *specifica*, non l'esecuzione del piano. Nessuna issue è stata creata,
> nessun asset toccato, nessun branch modificato.

---

## ⚠️ Errata, scritta dopo l'esecuzione della passata

Questa revisione è stata prodotta su un checkout che risultò poi essere **141 commit indietro** rispetto a
`origin/main`. Due reperti ne portano il segno e vanno letti con questa correzione davanti.

| Reperto originale | Correzione misurata su `c0cc0693` |
|---|---|
| **F5** — «`URTDevSandboxLauncherSubsystem` non esiste; nessun file con `Launcher` o `DevSandbox` nel nome» | **Falso sul progetto, vero su quella copia.** Il subsystem esiste in `Source/RefactorTacticsEditor/`, è un `UEditorSubsystem` nativo, e #1680 è chiusa correttamente |
| **F7** — «non esistono test di launcher» | **Falso.** `RTDevSandboxLauncherTests.cpp`, cinque test nel namespace `RefactorTactics.DevSandboxLauncher.*` |

Ne segue che **C-02 è risolto e non era un difetto del piano**: la biforcazione «nativo / Blueprint /
assente» ha esito **NATIVO**, quindi #1705 e #1682 restano correttamente Lane A e l'ordine `R1 → R2 → R6`
dell'handoff non va riscritto. Il gradino `C0.5` proposto in C-02 conserva però il suo valore: è la
verifica che ha prodotto questa correzione, e costa dieci secondi.

🔴 **Ciò che il caso dimostra, e che nessuno dei findings diceva**: la revisione ha applicato «Evidence >
assumptions» ai contenuti del documento, ma non alla **freschezza del proprio punto di osservazione**. Un
checkout stantio non produce incertezza — produce fatti falsi, ordinati e citabili. Se il piano fosse
stato eseguito su quella base, #1680 sarebbe stata riaperta come regressione con evidenza a supporto. La
misura mancante era una sola riga: `git rev-list --count HEAD..origin/main`.

Tutti gli altri findings (C-01, C-03, C-04, C-05 e i maggiori) sono stati riconfermati durante
l'esecuzione — due di essi, C-01 e C-05, si sono materializzati alla lettera nel giro di un'ora. Il
referto della passata è in
[`td-integration-pass-audit-2026-08-30.md`](../../roadmap/plans/td-integration-pass-audit-2026-08-30.md).

---

## 0. Evidenza raccolta prima della revisione

Il documento fa affermazioni verificabili sul checkout. Sono state misurate, perché una revisione
di requisiti che non tocca la realtà produce raccomandazioni eleganti e inutili.

| # | Fatto misurato | Comando | Esito |
|---|---|---|---|
| F1 | `D:\Repositories\refactor-tactics-technical-designer` **non è un repository git** | `git rev-parse --show-toplevel` | `fatal: not a git repository` |
| F2 | Il checkout reale è `D:/Repositories/refactor-tactics-main`, branch `main`, HEAD `3c0123a9` | `git rev-parse` | confermato |
| F3 | Working tree **già sporco**: 6 `.uasset` modificati (`BP_Unit_Phase/Riktor/Wraith`, `BP_GameMode`, `WBP_RT_ErrorModal`, `WBP_RT_MenuEntry`), 1 `.cpp` di test, 8 `.md`/`.png` cancellati | `git status --short` | confermato |
| F4 | Esiste già un worktree registrato: `rt-wt-t6` su `docs/spec-panel-bot-stall`; su disco anche `rt-wt-overlap`, `wt-scenari`, `refactor-tactict-dev` | `git worktree list` + `ls` | confermato |
| F5 | **`URTDevSandboxLauncherSubsystem` non esiste**. Nessun file sorgente contiene `DevSandbox` o `Launcher` nel nome. Nessuna classe launcher in C++ | `grep -rIl`, `find Source` | assente |
| F6 | `DevSandbox` compare in C++ solo in `RTGameMode.cpp`, `RTScenarioSession.cpp`, `RTHexMapTests.cpp`, `RTMatchSetupWorldTests.cpp`, `Config/DefaultEngine.ini` | `grep -rIl` | confermato |
| F7 | **Non esistono test di launcher**. I test Frontend esistono con altro nome: `RTFrontendMainMenuTests`, `RTFrontendMatchHudTests`, `RTFrontendNavigationTests`, `RTFrontendPauseTests` | `ls Source/RefactorTactics/Tests/` | confermato |
| F8 | Esistono: `URTFrontendNavigator`, `InitializeFrontend`, `Skirmish2v2` (in `RTGameMode.cpp`, `RTStartupReport.h`), `ARTGameMode`, `URTScenarioAuthoring` | `grep -rIl` | confermato |
| F9 | `Content/RT/Maps/Dev/L_DevSandbox/L_DevSandbox.umap` esiste | `find Content` | confermato |
| F10 | Due soli moduli: `RefactorTactics`, `RefactorTacticsEditor` | `ls Source/` | confermato |

---

## 1. Valutazione di qualità

Giudizio del panel, non misura strumentale.

| Dimensione | Punteggio | Motivazione |
|---|---|---|
| Chiarezza | 8.0 / 10 | Linguaggio diretto, anti-goal espliciti, poche ambiguità lessicali. |
| Completezza | 6.5 / 10 | Copre tracking, processo, merge order. Mancano: criterio di stop, baseline dei test, definizione di "leggibile". |
| **Testabilità** | **4.0 / 10** | Il gate finale (§13) è composto da 15 asserzioni percettive senza oracolo. Il difetto dominante. |
| Coerenza interna | 5.5 / 10 | §12 confligge con §7; §13 esclude ciò che §12 R8 include; §14 confligge con §12 sullo scope del deliverable. |
| **Aderenza alla realtà** | **3.5 / 10** | Il documento presuppone un launcher che nel codice non esiste (F5), una cwd che non è un repo (F1) e un albero pulito che è sporco (F3). |
| Governance del processo | 8.5 / 10 | §1, §6, §17 sono di qualità notevole e vanno preservati letteralmente. |

**Sintesi**: il documento è forte come *disciplina di processo* e debole come *specifica verificabile*.
Regge sull'intenzione, cede sull'oracolo.

---

## 2. Findings critici

### C-01 · Il path operativo nominato non è un repository — §18, §5 C0, §8

**Wiegers** — Il documento apre con "Nome operativo: refactor-tactics-technical-designer" e chiude (§18)
con sei comandi git da eseguire per primi. Eseguiti in quella directory falliscono tutti (F1). La nota
"se il checkout locale si chiama letteralmente `refactor-tactics-techmical-designet`, NON rinominarlo"
tratta il problema come un refuso, mentre il problema è che **il documento non nomina mai il checkout
canonico**. §8 impone che ROOT/HEAD/BRANCH combacino fra i due terminali, ma non dice quale ROOT sia
quella giusta — e sul disco ci sono almeno cinque candidati (F2, F4).

**Nygard** — Questo è il primo comando che un esecutore digita. Una specifica il cui passo 1 fallisce
non è una specifica ambigua: è una specifica rotta.

**Riscrittura proposta (§8, sostituisce "NOTA PATH" e la HARD RULE)**:

```
CHECKOUT CANONICO DI QUESTO SLICE
  ROOT   = D:/Repositories/refactor-tactics-main
  BRANCH = feat/td-integration-pass   (da creare da main; NON lavorare su main)
  HEAD   = registrato all'apertura della passata

Ogni altra directory su D:/Repositories (refactor-tactics-technical-designer,
rt-wt-t6, rt-wt-overlap, wt-scenari, refactor-tactict-dev, *.vault, *.wiki)
è FUORI SCOPE per questo slice e non va aperta né dal terminale Code né dall'Editor.

Gate di apertura, in ENTRAMBI i terminali:
  git rev-parse --show-toplevel   -> deve stampare esattamente ROOT
  git branch --show-current       -> deve stampare esattamente BRANCH
  git rev-parse HEAD              -> i due valori devono coincidere
Se uno dei tre diverge: FERMARSI. Non è un avviso, è un blocco.
```

---

### C-02 · Il launcher su cui poggia metà del piano non esiste nel codice — §2, §3, §5 C1, §11

**Fowler** — §3 istruisce a cercare `URTDevSandboxLauncherSubsystem`. Non esiste (F5). Non esiste alcuna
classe, alcun file, alcun modulo il cui nome contenga `Launcher` o `DevSandbox`. §2 dichiara #1680
"CLOSED — launcher su L_DevSandbox — REUSE" e §7 dice "verifica launcher #1680": il documento assume
un artefatto di cui non stabilisce mai la natura.

Ne discendono due mondi possibili, che il documento non distingue:

- **(a) il launcher è Blueprint/UMG**, vive dentro `L_DevSandbox.umap` o in un `WBP_`. Allora #1705 e
  #1682 **non sono lavoro di Lane A (CODE)**: sono lavoro binario, Lane B. L'intera partizione del
  documento è assegnata al contrario, e §12 (R1, R2 prima di R6 Editor Session A) ordina di fare in
  Lane A ciò che si può fare solo in Lane B.
- **(b) il launcher non è mai stato implementato** e #1680 è chiusa senza artefatto. Allora #1705 e
  #1682 non sono estensioni di qualcosa che esiste, ma la prima implementazione — con stima, rischio e
  ordine completamente diversi.

**Cockburn** — Nessuna delle due letture è deducibile dal documento, e le due portano a piani
incompatibili. Questa è l'unica vera `DECISION_REQUIRED` della passata, e il documento non la nomina.

**Azione proposta**: inserire in §5 un gradino **C0.5 — Localizzare il launcher**, bloccante, *prima*
di C1 e prima di qualunque assegnazione di lane:

```
C0.5 — LOCALIZZARE IL LAUNCHER (bloccante, precede C1 e la scelta di lane)
Domanda: dove vive oggi il Tactical Designer Launcher consegnato da #1680?
Oracoli ammessi:
  - grep nei sorgenti (esito atteso oggi: nessun simbolo nativo);
  - ispezione di Content/RT/Maps/Dev/L_DevSandbox/L_DevSandbox.umap nell'Editor;
  - ricerca di WBP_* riconducibili al launcher in Content/RT/UI;
  - lettura del corpo e degli allegati di #1680.
Esiti e conseguenze:
  NATIVO   -> #1705/#1682 restano Lane A. Il piano prosegue come scritto.
  BLUEPRINT-> #1705/#1682 passano a Lane B. §12 va riordinato: la Sessione A
              precede R1/R2, non le segue.
  ASSENTE  -> #1680 è una regressione o una chiusura senza artefatto:
              aprire REGRESSION su #1680, NON nuove issue di feature.
Finché C0.5 non ha esito, non si stima, non si ordina e non si crea nulla.
```

---

### C-03 · Il gate finale non è verificabile — §13, §11, §5 C3/C4

**Wiegers** — I 15 punti del §13 sono in larga parte percettivi: "vedere terreno/celle, non fondo
indistinto", "vedere HUD player-facing", "non dipendere dal debug HUD". §11 ripete il difetto:
"se leggibile -> ALREADY_DONE". "Leggibile" non è definito da nessuna parte del documento. Due
persone oneste possono chiudere e non chiudere lo stesso gate.

**Adzic** — Mancano gli esempi. Una specifica di questo tipo si rende eseguibile in modo banale
separando ciò che una macchina può asserire da ciò che solo un umano può firmare.

**Crispin** — E la firma umana va comunque resa un artefatto: chi ha guardato, quando, su quale HEAD,
con quale screenshot allegato. Altrimenti a distanza di due settimane il gate non è riesaminabile.

**Riscrittura proposta (§13)** — spaccare il gate in due tabelle:

```
GATE A — AUTOMATICO (fallisce la passata se rosso)
 A1 Config/DefaultEngine.ini punta L_DevSandbox come mappa dev attesa   -> test
 A2 il match default parte con 4 pedine, 2 per team                     -> RTMatchSetupWorldTests
 A3 la fase iniziale del match è Planning                               -> test esistente
 A4 Ready transita a Resolution                                         -> test esistente
 A5 esiste UN SOLO entry point di start-match invocato dal launcher     -> test di seam (vedi C-04)
 A6 nessun asset dirty inatteso a fine passata                          -> git status --short vuoto
                                                                            salvo i path dichiarati
 A7 ogni mesh del Graybox Kit ha uno slot materiale assegnato           -> gate del commandlet (#1714)

GATE B — UMANO (checklist firmata, non automatizzabile)
 Per ciascuna voce: screenshot + HEAD + data + firma.
 B1 L_DevSandbox si apre senza modale di errore
 B2 il launcher compare
 B3 la lista scenari è popolata e selezionabile
 B4 Start Session apre il workspace di authoring
 B5 il terreno è distinguibile dallo sfondo (criterio dichiarato: le celle
    sono riconoscibili come griglia in uno screenshot non ritoccato)
 B6 il Graybox Kit minimo è presente e distinguibile per tipo
 B7 l'HUD player-facing è visibile con il debug overlay OFF
 B8 il centro schermo è libero
Nessuna voce di GATE B può essere dichiarata verde senza il suo screenshot.
```

---

### C-04 · L'unico requisito architetturale davvero importante è l'unico non testabile — §3

**Fowler** — L'acceptance criterion `[ ] Il click percorre lo stesso start path del Play frontend` è
il cuore della issue: è ciò che impedisce il secondo match setup. Ma così com'è scritto è
verificabile solo leggendo il codice a occhio, cioè non è verificabile in regressione. Fra sei mesi
qualcuno duplicherà il path e nessun test si accorgerà di nulla.

**Adzic** — Va reso concreto. Il seam esiste già (`URTFrontendNavigator`, `InitializeFrontend`,
`ARTGameMode`, `Skirmish2v2` sono tutti presenti — F8): basta nominarlo e asserirlo.

**Riscrittura proposta (§3, sostituisce l'AC 2 e l'AC 6)**:

```
AC2 (era "stesso start path")
  Dato il simbolo di start-match condiviso <NOME_ESATTO>, individuato in C0.5,
  Quando il launcher esegue Play Default 2v2,
  Allora la chiamata passa da <NOME_ESATTO>
  E un test asserisce che <NOME_ESATTO> ha esattamente un chiamante nel modulo
    RefactorTacticsEditor e uno nel percorso frontend.

AC6 (era "nessun match setup duplicato nel modulo Editor")
  Un test statico asserisce che il modulo RefactorTacticsEditor non contiene
  riferimenti a Format.Skirmish2v2, né a costruzione di roster, né a scelta
  di mappa o seed. Il modulo Editor può solo invocare <NOME_ESATTO>.
```

Questo trasforma due frasi di buona volontà in due gate meccanici, ed è il modo più economico per
far rispettare "no second match setup" (§13, gate tecnici) anche dopo che questa passata sarà finita.

---

### C-05 · La mutua esclusione sui binari è una convenzione, non un meccanismo — §9

**Nygard** — §9 è un protocollo di cortesia fra due terminali che condividono un working tree. Non
ha lock, non ha rilevamento, non ha ricovero. E parte da un presupposto già falso oggi: la baseline
dichiara implicitamente un albero pulito, mentre l'albero ha **sei `.uasset` già modificati** (F3),
fra cui `BP_GameMode.uasset` e `WBP_RT_ErrorModal.uasset` — cioè proprio due dei path che questa
passata dovrà toccare (Play 2v2 e il falso errore startup #1330).

Mancano tutti i modi di fallire:

- Editor che crasha con asset dirty: chi li recupera, e come si distingue un dirty legittimo da uno orfano?
- Windows che tiene il lock sul `.uasset`: `git checkout` fallisce a metà; nessuna procedura di ricovero.
- `Save` che riscrive un binario mentre Code ha già fatto `git add`: l'indice diverge dal file.

**Riscrittura proposta — da inserire come §9-bis**:

```
§9-bis — PRECONDIZIONE E RECUPERO

PRECONDIZIONE DI APERTURA (bloccante)
  Prima di iniziare, `git status --short` deve essere vuoto oppure ogni path
  sporco deve essere elencato in un blocco "dirty ereditato" con, per ciascuno:
  chi l'ha prodotto, se va committato, scartato o preservato.
  Stato misurato al 2026-08-30 su main@3c0123a9 (da riconciliare PRIMA di partire):
    M Content/RT/Characters/{Phase,Riktor,Wraith}/Blueprints/BP_Unit_*.uasset
    M Content/RT/Core/Framework/BP_GameMode.uasset
    M Content/RT/UI/Framework/WBP_RT_ErrorModal.uasset
    M Content/RT/UI/Framework/WBP_RT_MenuEntry.uasset
    M Source/RefactorTactics/Tests/RTFrontendWidgetAssetTests.cpp
    D  8 file .md/.png in radice
  Nota: BP_GameMode e WBP_RT_ErrorModal sono sulla rotta di questa passata.
  Committarli o scartarli è una decisione umana, non un dettaglio.

TOKEN DI SCRITTURA
  Un solo writer per volta su Content/. Il possesso è esplicito:
  chi apre l'Editor scrive `docs/roadmap/.binary-lock` con path e ora;
  lo cancella dopo Save + verifica. Code non tocca Content/ mentre il file esiste.

RECUPERO
  Editor crashato con dirty   -> riaprire, rileggere is_dirty, decidere Save/Discard
                                 PRIMA di qualunque comando git.
  git bloccato da lock Windows-> chiudere l'Editor, ripetere, mai --force.
  Divergenza indice/file      -> `git checkout -- <path>` solo a Editor chiuso.
```

---

## 3. Findings maggiori

### M-01 · Due deliverable incompatibili nello stesso documento — §12 vs §14

**Cockburn** — §14 chiede come output obbligatorio una tabella di riconciliazione delle issue: è un
deliverable da *audit*, misurabile in ore. §12 chiede R0..R10, cioè implementare #1705, #1682, il
bridge, #1714, il residuo #613, due sessioni Editor, il cooked #1665, test e package: è un
deliverable da *slice completo*, misurabile in giorni. Non è chiaro quale delle due cose la
"passata cloud" debba consegnare, e la risposta cambia tutto: priorità, rischio, criterio di stop.

**Raccomandazione**: dichiarare in §0 il livello del goal, alla Cockburn.
`Goal utente (kite)`: il technical designer arriva al loop giocabile. `Goal di questa passata (sea)`:
riconciliare il tracking e produrre l'audit del §14 + **al massimo** R1. Il resto è backlog ordinato,
non impegno di questa passata.

### M-02 · Il vincolo "massimo DUE aperture Editor" ottimizza la metrica sbagliata — §7 vs §10 vs §12

**Fowler** — L'obiettivo reale è evitare DLL incoerenti e binari contesi; il numero di aperture ne è
solo un indicatore. §10 ammette già che un rebuild nativo impone di chiudere e riaprire, e C4
(commandlet graybox) e C5 (cooked) impongono cicli build. Se C0.5 stabilisce che il launcher è
Blueprint (C-02), R1 e R2 diventano lavoro Editor e il tetto di due aperture salta il primo giorno.
Un vincolo che si sa già che verrà violato smette di guidare e comincia a essere aggirato.

**Raccomandazione**: sostituire il tetto duro con un vincolo sull'invariante vera:
"Ogni apertura dell'Editor deve avvenire su una DLL coerente con HEAD. Aperture consecutive senza
rebuild intermedio vanno accorpate. Ogni apertura oltre la seconda va registrata in §16 con la
ragione tecnica che l'ha imposta." Si conserva l'intento, si elimina la finzione.

### M-03 · "fail N" senza baseline non è un criterio di superamento — §5 C7

**Crispin** — Il formato di report (`command, HEAD, found N, performed N, fail N, exit code`) è fra
le cose migliori del documento. Ma manca la sola cosa che lo rende decidibile: **la baseline**. Con
`fail 3`, il documento non permette di distinguere tre fallimenti preesistenti da tre regressioni
nuove. E "suite completa corrente" non nomina i test: quelli di launcher non esistono (F7), e i test
Frontend si chiamano `RTFrontendMainMenuTests`, `RTFrontendMatchHudTests`, `RTFrontendNavigationTests`,
`RTFrontendPauseTests` — non "ScreenHud".

**Raccomandazione**: in C7, prima di ogni altra cosa, registrare la baseline su HEAD di partenza e
nominare i filtri esatti:

```
Baseline (HEAD di apertura): found N0, fail F0, con l'elenco dei test rossi.
Criterio: la passata è verde se fail <= F0 E nessun test rosso è nuovo.
Filtri nominati: RTScenario*, RTFrontend*, RTMatchSetupWorld*, RTHexMap*, <graybox: da nominare>.
Non esistono oggi test di launcher: se C1 produce comportamento, produce anche il suo test.
```

### M-04 · Il perimetro del gate non coincide con il perimetro del merge — §12 R8 vs §13

**Hightower** — §12 include R8 (#1665, board nera nel cooked) nell'ordine di merge, ma §13 verifica
solo il loop in Editor/PIE: nessuna voce del gate finale riguarda il packaged. Così #1665 è lavoro
dovuto ma non gate, cioè è lavoro che nessuno può dichiarare finito.

**Raccomandazione**: decidere e scrivere. O #1665 esce dallo slice e diventa backlog dichiarato
(coerente con "meno tracking, più loop reale"), o entra nel gate con la sua voce:
`A8 la board è leggibile in un build Development packaged, con screenshot`.
La seconda opzione allunga materialmente la passata: il panel raccomanda la prima.

### M-05 · Campi obbligatori mai definiti — §14

**Wiegers** — §14 impone che ogni nuova issue riporti `Determinism`, `Privacy`, `Packaged`. Nessuno
dei tre è definito nel documento, e per una issue di launcher due su tre sono verosimilmente "N/A".
Un campo obbligatorio che si compila con "N/A" nel 90% dei casi addestra a compilare senza pensare, e
il 10% in cui contava passa inosservato.

**Raccomandazione**: renderli condizionali con un criterio esplicito.
`Determinism`: obbligatorio se la issue tocca seed, ordine di iterazione o replay.
`Privacy`: obbligatorio se la issue introduce logging, telemetria o path utente.
`Packaged`: obbligatorio se la issue tocca cooking, asset registry o materiali.
Altrimenti si omettono, non si scrive "N/A".

### M-06 · "se esiste un solo formato" è una condizione non risolta — §3

**Adzic** — "nessun selettore finto se esiste un solo formato" lascia aperto il ramo che conta. Il
codice mostra `Skirmish2v2` in `RTGameMode.cpp` e `RTStartupReport.h` (F8), ma il documento non
stabilisce se sia l'unico formato. Se ne esistono due, il requisito cambia da "nessun selettore" a
"il launcher sceglie" — cioè cambia la UI e cambia l'AC.

**Raccomandazione**: risolvere la condizione in C0 come misura, non lasciarla al giudizio in corsa:
"Enumerare i MatchFormat registrati. Se == 1, nessun selettore (AC come scritto). Se > 1, il launcher
usa il default dichiarato dal sistema esistente e non introduce un proprio default."

### M-07 · Nessun criterio di arresto e nessun ramo di fallimento — §12

**Nygard** — R0..R10 è una catena felice. Non è scritto cosa succede se R1 si rivela più grande del
previsto, se la Sessione A trova il launcher assente, o se il gate B fallisce su una voce sola. In
assenza di un criterio d'arresto, l'esecutore o si ferma troppo presto o non si ferma mai.

**Raccomandazione**: aggiungere in §12: quali R sono indipendenti (R8 non dipende da R1-R5 e può
uscire dallo slice per intero); quale evento sospende la passata (esito ASSENTE in C0.5; gate A rosso;
dirty non riconciliato); e che cosa si consegna comunque quando ci si ferma (l'audit del §14, sempre).

---

## 4. Findings minori

- **m-01 (Wiegers)** — §2 dichiara stati di 20 issue con una data ma senza il comando e senza SHA di
  misura. Aggiungere due colonne `verificato con` / `quando`, e marcare esplicitamente ciò che è
  *dichiarato da un'altra issue* (es. "#623 dichiarata consegnata da #1105") come assunzione di
  secondo grado, non come osservazione.
- **m-02 (Crispin)** — §16 non chiede il risultato dei PIE check, solo la loro esistenza
  ("PIE checks:"). Cambiare in `PIE checks (esito + screenshot)`.
- **m-03 (Fowler)** — §15 elenca gli scope di commit ma non copre il caso più rischioso: il commit
  che mescola `.umap` e sorgenti. È già vietato in prosa; renderlo una riga di regola:
  "nessun commit contiene insieme `Content/**` e `Source/**`".
- **m-04 (Wiegers)** — §14 fissa "Default: 0-1 nuove issue". È una quota sull'esito di un'attività di
  scoperta, e sotto pressione produce sottodichiarazione. Riformulare: "se emergono più gap reali,
  non crearli: elencarli come DECISION_REQUIRED nella tabella del §14". L'intento è preservato senza
  incentivare il silenzio.
- **m-05 (convenzione locale)** — Né §14 né §15 menzionano la chiusura automatica delle issue.
  Le PR di questo repository sono in italiano e GitHub chiude solo su `closes/fixes/resolves`: una PR
  che dice "Chiude #1705" lascia la issue aperta. Aggiungere a §15:
  "il corpo della PR contiene una riga `Closes #N` in inglese, in aggiunta al testo italiano".
- **m-06 (Hightower)** — §6 elenca gli oracoli MCP in blocco. Renderli una mappa operazione -> oracolo
  (`set property` -> rileggi la property; `save` -> riapri l'asset e verifica is_dirty=false;
  `compile` -> ricompila esplicitamente e leggi l'esito, mai accettare `null`).

---

## 5. Quello che va difeso dalla riscrittura

Un critique che elenca solo difetti fa danno. Queste parti sono sopra la media e non vanno toccate:

- **§1 SEARCH BEFORE CREATE** con la tassonomia a otto stati (REUSE / UPDATE_EXISTING / CREATE_NEW /
  LINK_ONLY / ALREADY_DONE / DEFER / DECISION_REQUIRED / REGRESSION). È il pezzo migliore del
  documento: costringe a classificare invece che a produrre.
- **§6 REGOLA DI EVIDENZA** — `MCP command sent != verified`, ancorata a un precedente reale (#1719,
  asset salvati ma compile `null`). Una regola con la sua cicatrice attaccata si rispetta; una regola
  astratta no.
- **§17 NON FARE** — dodici anti-goal espliciti. Rari e preziosi: la maggior parte delle specifiche
  dice solo cosa fare, e lascia che l'ambito si allarghi dai bordi.
- **§5 C7 formato di report** — `command, HEAD, found N, performed N, fail N, exit code` con il divieto
  di copiare conteggi storici. Manca solo la baseline (M-03), il resto è corretto.
- **§5 C4** — "NON assegnare solo a mano sulle mesh generate: la rigenerazione lo perderebbe". È un
  requisito che nomina il proprio failure mode. Andrebbero scritti tutti così.

---

## 6. Consenso del panel

Su cosa i sette concordano, senza tensioni residue:

1. **Il documento non è pronto per l'esecuzione, ed è a poche ore dall'esserlo.** I difetti sono
   concentrati e riparabili: nominare il checkout, localizzare il launcher, dare un oracolo al gate.
2. **C-02 viene prima di tutto.** Finché non si sa dove vive il launcher, l'assegnazione Lane A /
   Lane B e l'ordine R1..R10 sono indecidibili — e sono metà del documento.
3. **Il gate finale va spaccato in automatico e umano.** Non per formalismo: perché altrimenti la
   chiusura dello slice dipende da chi guarda lo schermo quel giorno.
4. **Lo stato sporco del working tree è una precondizione, non un dettaglio.** Sei `.uasset`
   modificati, due dei quali sulla rotta della passata, sono un blocco all'apertura.
5. **La disciplina di processo del documento è superiore alla sua specificità tecnica.** Nella
   riscrittura, tagliare processo per fare spazio a dettaglio sarebbe il baratto sbagliato.

**Tensione non risolta, che resta una decisione umana**: Cockburn e Hightower spingono per restringere
questa passata all'audit + R1, lasciando #1665 e il packaged fuori. Nygard osserva che il cooked è il
solo posto dove i difetti di custom data si manifestano, e rinviarlo sposta rischio in avanti senza
ridurlo. Il panel non scioglie il nodo: dipende da quanto vale, ora, il loop giocabile rispetto alla
solidità del packaged.

---

## 7. Ordine di revisione del documento

| # | Intervento | Sezioni | Perché ora |
|---|---|---|---|
| 1 | Nominare ROOT/BRANCH canonici; dichiarare fuori scope le altre directory | §8, §18 | Senza, il passo 1 fallisce |
| 2 | Inserire C0.5 (localizzare il launcher) come gradino bloccante | §5, §12 | Sblocca l'assegnazione delle lane |
| 3 | Riconciliare il dirty ereditato; aggiungere §9-bis | §9 | Precondizione di apertura |
| 4 | Spaccare §13 in GATE A automatico / GATE B firmato | §13, §11 | Rende chiudibile lo slice |
| 5 | Rendere meccanici AC2 e AC6 della issue bridge | §3 | Protegge "no second match setup" nel tempo |
| 6 | Dichiarare il livello del goal della passata | §0, §12, §14 | Scioglie audit vs slice |
| 7 | Baseline dei test e filtri nominati | §5 C7 | Rende `fail N` un criterio |
| 8 | Decidere se #1665 è nello slice | §12, §13 | Elimina lavoro dovuto e non gate |
| 9 | Rendere condizionali Determinism/Privacy/Packaged; aggiungere `Closes #N` | §14, §15 | Igiene, basso costo |

---

## 8. Nota di metodo

I punteggi della sezione 1 sono giudizio strutturato del panel, non misure strumentali. I fatti F1-F10
sono invece riproducibili: ogni riga riporta il comando che li ha prodotti, su
`D:/Repositories/refactor-tactics-main` a `main@3c0123a9`, il 2026-08-30.
