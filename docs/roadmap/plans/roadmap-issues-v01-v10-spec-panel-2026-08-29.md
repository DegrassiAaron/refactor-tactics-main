# Riconciliazione issue/roadmap v0.1 → v1.0 — spec panel

> `CURRENT` · **Referto di revisione**, non owner. Consuma
> [`../../archive/src/handoff/2026-08-29-roadmap-issues-v01-v10-prompt.md`](../../archive/src/handoff/2026-08-29-roadmap-issues-v01-v10-prompt.md)
> (in radice come `Prompt_Claude_RefactorTactics_Roadmap_Issues_v0.1_v1.0.md`, **untracked**, fino a questo commit).
>
> **Data**: 2026-08-29 · **Base**: `main` @ `76aec5dd` · **Modo**: critique · **Focus**: requirements + architecture documentale + compliance
>
> **Cosa è**: il verdetto su una **specifica di lavoro** — un work order che chiede a una sessione di
> misurare lo stato reale, riconciliare GitHub e aggiornare i documenti owner. Il referto giudica la
> specifica, **non esegue il lavoro che ordina**: `/sc:spec-panel` è task documentale ([`CLAUDE.md`](../../../CLAUDE.md) §6),
> e nessuna issue è stata creata, chiusa o modificata.
>
> **Cosa non è**: un'autorità. Se una riga qui diverge da
> [`../v0.1-definition-of-done.md`](../v0.1-definition-of-done.md) o dal
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md), **ha ragione l'owner**.
>
> ✅ **Aggiornato lo stesso giorno** con la decisione del product owner su **F-05**: *«partita completa, anche
> con pareggio»*. Il Complete Match **è `G10`**, e ne esce una riformulazione del gate — §4 F-05, §6.1-6.

---

## 1. Il verdetto in una riga

> **La specifica è rigorosa nel metodo e scaduta nei fatti: impone giustamente di misurare prima di
> decidere, poi vincola il lavoro a tre premesse che una misura di dieci minuti smonta — `E48` è già
> preso da un'epic v0.1 sana, `#1657` è chiusa, e la vista v0.1→v1.0 che chiede di produrre esiste da
> ieri.**

Il contributo che vale il consumo è il §23-D — **«Issue NON create perché duplicate»**. È l'unica parte del
documento che rende osservabile il *non-lavoro*: obbliga a dichiarare cosa si è deciso di non fare e perché,
che è esattamente ciò che un audit di deduplicazione normalmente perde. Quella sezione va conservata come
formato di output ricorrente, indipendentemente dal resto.

---

## 2. Base di misura

Tutto ciò che segue è stato **misurato**, non ricordato. Query `gh` lato server, letture su albero pulito.

```text
Repo      : DegrassiAaron/refactor-tactics-main
Branch    : main
HEAD      : 76aec5dd
Data      : 2026-08-29
Sorgente  : untracked in radice (`git status --porcelain` → `??`)
```

| Affermazione della specifica | Misura | Esito |
|---|---|---|
| Gli 8 documenti owner del §2.1 esistono | `ls` su ciascun path | ✅ **8/8 presenti** |
| La tabella release del §3 (v0.1…v1.0) | confronto con [`../roadmap-v0.1-v1.0.md`](../roadmap-v0.1-v1.0.md) §2 | ✅ combacia (v0.7 «Dedicated /» è cosmetico) |
| «NON creare `E48`» (§12) | `gh issue list --search "EPIC in:title"` | 🔴 **`E48` esiste già**: [#1408](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1408) `[EPIC v0.1] E48 · Il giocatore raggiunge il modello`, **OPEN**, e **non** è il Tactical Designer |
| «tutte le epic `E1...E47`» (§4) | idem | 🔴 range **scaduto**: la numerazione è arrivata a `E48` |
| Conteggio epic | idem, `--state all` | **54** issue con `EPIC` nel titolo — **42 OPEN · 12 CLOSED** |
| `#1657` DevSandbox «decisione da non nascondere» (§15) | `gh issue view` | 🔴 **CLOSED** |
| `#1114`–`#1117` Scenario Composer «già consegnati» (§13) | idem | ✅ **tutte e quattro CLOSED** |
| Trial TD `#1625`–`#1630`, `#711` (§13) | idem | ✅ tutte **OPEN**, come implicito nel testo |
| `#14` `#26` `#166` `#613` `#778` `#1105` | idem | ✅ tutte **OPEN**; `#1105` **senza milestone** — coerente con `out_of_release_scope` |
| «esiste una vista coerente v0.1→v1.0» (§1, obiettivo 1) | `head` del file | 🔴 **già consegnato il 2026-08-28** |
| «Complete Match» ha un owner su GitHub (§6, focus P0) | `gh issue list --search "Complete Match"` | 🔴 **zero issue**. Nei docs vive come *WAVE 2* di [`../roadmap-main-v0.1.md`](../roadmap-main-v0.1.md) e in due handoff `DIR-C` — **è `G10`**, vedi F-05 |
| Il **pareggio** è un esito terminale del dominio | `ERTMatchOutcome` in `Turn/RTTurnRules.h:23` | ✅ `Draw` esiste, con via `RoundLimit` distinta dall'esito |
| Il pareggio è presentabile | `MatchResultDrawHasNoWinner` · `RTFrontendNavigationTests` | ✅ `bHasWinner=false`, `bIsFinished=true`, e naviga a Result |
| `G10` copre il pareggio | lettura del DoD | 🔴 **no**: dice *«dall'avvio alla vittoria»* — vedi F-05 |
| I gate `G1…G14` esistono e sono riconciliati (§10, §20) | lettura del DoD | ✅ esistono, **con evidenza datata e rimisurata il 2026-08-29** |

⛔ **Non misurato**: nessuna suite eseguita, nessun build lanciato, nessun `.uasset` aperto. Il referto non
dichiara verde né rosso alcun gate — legge quelli che l'owner dichiara già.

---

## 3. Punteggi

| Dimensione | Voto | Ragione in una riga |
|---|---|---|
| **Chiarezza** (Doumont) | **6.0** / 10 | 24 sezioni, di cui **5** descrivono il prodotto (F0–F5) e 19 il processo; nessun sommario in testa |
| **Completezza** | **7.0** / 10 | copre metodo, template issue e formato di output; **mancano** budget, dry-run, rollback e semantica del gate parziale |
| **Testabilità** (Wiegers · Adzic) | **3.5** / 10 | **nessuna** delle quattro condizioni del §1 è verificabile con un comando; **zero** esempi eseguibili |
| **Consistenza** | **5.0** / 10 | due definizioni di *done* (§1 e §24); regole del repository **riscritte** invece che citate |
| **Fedeltà misurata** | **6.5** / 10 | i path reggono, la tassonomia release regge; **tre premesse operative su tre sono scadute** |
| **Complessivo** | **5.6** / 10 | metodo da conservare, premesse da rifare, autorità da chiarire |

---

## 4. Findings — critique

Severità: 🔴 critico (blocca o produce danno) · 🟠 maggiore (produce lavoro sbagliato) · 🟡 minore.

### 🔴 F-01 · Il divieto è scritto su un identificatore libero, non sul concetto — WIEGERS, FOWLER

> «NON creare: `E48`» (§12)

`E48` **è già assegnata** a [#1408](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1408), epic
v0.1 aperta che non ha nulla a che vedere col Tactical Designer. Un esecutore letterale trova la stringa
occupata e ha tre uscite, **tutte sbagliate**: conclude che la regola è già stata violata, tenta di ritirare
o rinumerare un'epic sana, oppure salta il §12 per intero perdendo anche la parte giusta.

**WIEGERS**: «Il requisito reale è *il Tactical Designer non riceve una epic di release*. `E48` era la sua
istanza il giorno in cui è stato scritto. Vincolare un numero libero significa scrivere un requisito che
scade da solo, e questo è scaduto in silenzio — nessun controllo poteva accorgersene.»

**FOWLER**: «È lo stesso difetto dell'hard-code di un ID dove serviva un tipo. La forma corretta non nomina
il prossimo numero: *nessuna epic di release ha come scope il Tactical Designer; il suo parent resta
[#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105) e la sua milestone resta vuota.*
Questa è verificabile con una query e non invecchia.»

📝 **Riscrittura**: `il Tactical Designer non acquisisce una epic numerata di release né una milestone di gioco; resta figlio di #1105.`
🎯 **Priorità**: alta — il difetto è già attivo oggi.

### 🔴 F-02 · L'obiettivo 1 è già consegnato, e la specifica non lo sa — COCKBURN

Il §1 chiede «esiste una vista coerente e aggiornata della roadmap **v0.1 → v1.0**».
[`../roadmap-v0.1-v1.0.md`](../roadmap-v0.1-v1.0.md) **esiste dal 2026-08-28**, si autodichiara *«vista di
navigazione, non owner»*, porta la propria base di misura (`HEAD`, avanti/dietro `origin/main`) e la regola di
precedenza (*«se questa pagina e un owner divergono, ha ragione l'owner»*). È precisamente ciò che il §18
prescrive di ottenere.

**COCKBURN**: «Il goal del primary actor è già raggiunto. Una specifica che ordina un goal raggiunto produce
uno di due esiti: lavoro nullo, oppure — peggio — un secondo documento che fa la stessa cosa. E il §2.1
vieta esplicitamente il terzo owner: la specifica si contraddice da sola nel giro di diciassette sezioni.»

📝 **Riscrittura**: sostituire l'obiettivo con la sua verifica — `la vista v0.1→v1.0 esiste, dichiara la propria base di misura e non duplica lo stato di alcun owner. Se già vera, l'obiettivo è chiuso senza scrivere.`
🎯 **Priorità**: alta.

### 🔴 F-03 · Nessuna delle quattro condizioni di uscita è misurabile — WIEGERS, ADZIC

| Condizione (§1) | Perché non è verificabile |
|---|---|
| «esiste una vista coerente e aggiornata» | *coerente* rispetto a cosa, misurato come? |
| «la v0.1 ha un critical path **molto chiaro**» | *chiaro* non ha unità |
| «GitHub non contiene issue duplicate» | universale non falsificabile: nessun protocollo dimostra l'**assenza** di duplicati su 42 epic aperte |
| «il Tactical Designer rimane tooling trasversale» | l'unica delle quattro rendibile in query — e non lo è |

**WIEGERS**: «Tre criteri su quattro non superano SMART, e il terzo è peggio: è una negazione universale. Non
esiste un lavoro finito che la soddisfi, quindi la specifica non può mai dichiararsi chiusa.»

**ADZIC**: «Mille righe e **nessun esempio eseguibile**. Una sola tabella `Given/When/Then` avrebbe fatto più
lavoro di tre sezioni di prosa:
> **Given** una proposta di issue con titolo *T* e concetto *C*
> **When** esiste una issue aperta o chiusa il cui Stable ID, epic padre o checkpoint coincidono
> **Then** non si crea, e la si registra sotto §23-D con `Coperta da: #N`.
Questo si può eseguire. "Non duplicare lavoro" no.»

📝 **Riscrittura**: rendere la terza condizione positiva e limitata — `ogni issue creata in questa sessione dichiara, nel proprio corpo, la ricerca di duplicazione eseguita (query e risultati); ogni proposta scartata compare in §23-D.`
🎯 **Priorità**: alta.

### 🟠 F-04 · Due definizioni di *done* — DOUMONT

Il §1 fissa quattro condizioni. Il §24 ne introduce **sei** diverse, sotto il titolo «criterio di successo»,
con una metrica nuova — *«una nuova sessione capisce in meno di cinque minuti»* — priva di protocollo: quale
sessione, con quale prompt, chi cronometra.

**DOUMONT**: «Un documento con due sezioni di chiusura ne ha zero. Il lettore che arriva al §24 dopo mille
righe adotta quella, perché è l'ultima cosa che legge — ed è la meno operativa delle due. Se le cinque
condizioni servono, si fondono in un solo posto; se il *criterio dei cinque minuti* è retorica, si scrive
come retorica e non come criterio.»

🎯 **Priorità**: media.

### 🟠 F-05 · Il focus P0 non ha un owner — COCKBURN, ADZIC

Il §6 dichiara **Complete Match** «il focus P0» e ne disegna il flusso in dieci passi. Misurato: **zero issue**
lo portano nel titolo. Vive come *WAVE 2* dentro [`../roadmap-main-v0.1.md`](../roadmap-main-v0.1.md) e in due
handoff `DIR-C`. Il §6 aggiunge che deve diventare «**integration/gate**, non una duplicazione delle
implementazioni» — giusto — ma **non nomina l'artefatto** che incarna quel gate.

**COCKBURN**: «Chi è l'attore, e qual è il suo goal osservabile? Se la risposta è *un tester avvia il gioco e
arriva a Result*, allora il gate è una verifica manuale e vive in `test-manuali-pie.md`. Se è *la suite prova
che una partita termina*, allora esiste già e si chiama `FreeRun.ArenaV01ReachesAWinner`. Sono due cose
diverse e la specifica non sceglie.»

**ADZIC**: «Finché *Complete Match superato* non ha un criterio scritto, ogni sessione lo interpreterà.»

#### ✅ Risolto — decisione del product owner, 2026-08-29

> **«Partita completa, anche con pareggio.»**

Il Complete Match **è `G10`**, non un gate nuovo: il §6 del sorgente è riconciliazione di un gate esistente.
Ma la decisione mostra che **`G10` è formulato male**, e la misura lo conferma — il codice sa già fare ciò che
il gate non nomina:

| Livello | Misura su `76aec5dd` | Esito |
|---|---|---|
| Dominio | `ERTMatchOutcome { InProgress · Team0Wins · Team1Wins · **Draw** }` — `Turn/RTTurnRules.h:23` | ✅ tre esiti terminali |
| Via | `ERTMatchEndReason { None · Elimination · Objective · **RoundLimit** }`, separata **apposta** dall'esito | ✅ il pareggio ha una via nominata |
| Presentazione | `MatchResultDrawHasNoWinner`: `bHasWinner = false` **e** `bIsFinished = true` | ✅ un pareggio *è* una partita finita |
| Frontend | `RTFrontendNavigationTests` naviga a Result con `Outcome = Draw` | ✅ arriva a Result |

`G10` dice *«dall'avvio alla **vittoria**»*: esclude per formulazione un esito che il dominio produce, il
ViewModel distingue e la UI presenta. **Nessuna implementazione manca; manca la parola nel gate.**

#### ⚠️ E il pareggio ha due nature — la trappola del gate

`RefactorTactics.Scenario.FreeRun.ArenaV01ReachesAWinner` **rifiuta esplicitamente** il pareggio:

```cpp
TestTrue("il tetto non e' stato raggiunto",  Result.TurnsPlayed < TettoDelFile);
TestFalse("non e' un pareggio allo scadere", Reached->Actual.Contains(TEXT("allo scadere")));
```

🔴 **E non è un difetto del test.** Quel test è una **sonda di non-degenerazione**: il pareggio allo scadere è
il sintomo dello stallo del bot che `RTBotStalemateProbeTests` studia — *«zero combattimento, pareggio allo
scadere»*. Se il Complete Match accettasse qualunque pareggio, il gate sarebbe superato da dodici round di
bot che si orbitano intorno senza colpirsi: **partita terminata ✅, gioco non dimostrato**.

**CRISPIN**: «Il criterio di terminazione e il criterio di qualità sono due cose diverse, e questo gate le
confonde. *La partita finisce* è necessario e non sufficiente. Il discriminante non è chi ha vinto — è se è
**successo qualcosa**: danni inflitti, almeno una reaction, progresso obiettivo. Ed è esattamente la lista che
il §6 del sorgente aveva già scritto senza sapere di star definendo la non-degenerazione.»

📝 **Riformulazione proposta di `G10`** *(modifica a un documento owner: non applicata — vedi F-09)*:

> **G10** · Partita completa 2v2 su mappa multilivello, dall'avvio a un **esito terminale dichiarato**
> — `Team0Wins · Team1Wins · Draw` — con la **via** registrata nel TurnLog (`Elimination · Objective ·
> RoundLimit`). Un pareggio chiude il gate **solo se** la partita non è degenere: almeno un danno inflitto,
> almeno una reaction risolta e il progresso obiettivo mosso. Un `RoundLimit` a zero eventi è **rosso**, ed è
> lo stallo che `RTBotStalemateProbeTests` misura.

🎯 **Priorità**: alta — è una riga di gate, e senza di essa `G10` non è chiudibile né con una vittoria
(perché la formulazione tace sul resto) né con un pareggio (perché la formulazione lo esclude).

✅ **Applicata al DoD il 2026-08-29** su conferma del product owner. Una riga, `G10`, con la formulazione
precedente dichiarata in coda alla voce.

#### ⛔ Il rename di `ArenaV01ReachesAWinner`: proposto, e ritirato dopo la lettura

Questo referto aveva proposto di rinominare il test, sul presupposto che «`ReachesAWinner` a quel punto ha il
nome sbagliato». **Il presupposto era falso, e va dichiarato invece che cancellato.**

Il docstring del test — venti righe — argomenta l'esclusione del pareggio e la ancora a due fonti:

> ⚠️ **Il caso discriminante e' proprio il pareggio.** `D-184` dichiara legittimo il pareggio allo scadere
> del free-run spedito, e questo test lo esclude PER NOME: un'evidenza che filma un pareggio non e'
> l'evidenza che `G10` e `G13` chiedono.

E **D-184** esisteva già: *«il pareggio allo scadere dei round è un esito legittimo della v0.1, e l'evidenza
di E47.6 viene da uno scenario, non dal free-run del default»*. 🔴 **La decisione del product owner non
introduce il pareggio: ratifica D-184 e la estende al gate**, che era rimasto indietro di formulazione.

Il nome del test quindi **non è un errore**: separa la **regola** (il pareggio è un esito valido) dalla
**scelta di evidenza** (un playtest che finisce in pareggio non dimostra il gioco). Il test è deliberatamente
più stretto del gate, ed è la stessa distinzione che la clausola di non-degenerazione rende esplicita.
Rinominarlo distruggerebbe un'intenzione scritta, e romperebbe i riferimenti in **D-220**, in
`Scenarios/AutoBattle/ArenaV01.json` e in due handoff `DIR-C`.

⚠️ **Resta una divergenza aperta, che il cambio di `G10` ha creato oggi**: il docstring afferma che un
pareggio *«non è l'evidenza che `G10` chiede»*, e da questo commit `G10` accetta un pareggio **non degenere**.
La stessa frase compare in `ArenaV01.json`. Sono **due punti di prosa** da riallineare — non il nome, non il
comportamento, non l'asserzione. **Non applicati**: toccano un file C++ e un corpus di scenario su un
checkout che altre sessioni condividono, e valgono una conferma separata.

### 🟠 F-06 · Regole del repository riscritte invece che citate — FOWLER

Tre casi misurati:

1. §10 — *«se la regola del repository dice che basta misurare, non trasformare un valore fuori budget in
   blocco automatico»*. **La regola esiste**, è `G11`, e il DoD la scrive già in forma assertiva: *«G11 non
   richiede di centrare i target: richiede di avere i numeri.»* Il condizionale «se la regola dice» crea una
   **seconda formulazione** della stessa regola.
2. §11 — il divieto di worktree paralleli che invalidano suite, mutex del motore e binari è **D-222**,
   citata per contenuto e non per ID.
3. §2.5 — l'elenco degli invarianti replica il canone di `piano-canonico-mvp.md`.

**FOWLER**: «Ogni riscrittura è un fork della verità che nessun controllo tiene allineato. La forma giusta è
il link: *vale G11 come scritto nel DoD*. Costa una riga e non deriva.»

🎯 **Priorità**: media.

### 🟠 F-07 · Un audit senza budget, senza campionamento e senza degradazione — NYGARD

Il §4 impone una tabella con nove colonne, fra cui **«Codice presente?»** e **«Test presenti?»**, su 22 issue
nominate **più** «tutte le epic `E1...E47`» — misurate: **54 issue** con `EPIC` nel titolo. Verificare codice e
test per ciascuna significa aprire il repository decine di volte. La specifica non fissa un tetto, non
autorizza un campione, non dice cosa fare a metà.

**NYGARD**: «Nessun timeout, nessun circuit breaker, nessuna risposta degradata. Il modo di fallire di questo
audit è il peggiore possibile: si esaurisce a due terzi e produce una tabella **parziale che sembra
completa**. Serve la regola esplicita — *sopra N epic si campiona, e il campione si dichiara* — e serve che
la tabella porti una colonna `verificato: sì/no` invece di lasciare una cella vuota che si legge come *no*.»

🎯 **Priorità**: media.

### 🟠 F-08 · Il report può classificare come PASS una misura NON VALIDA — CRISPIN, NYGARD

Il §23-H chiede l'esito test in **tre** stati: `PASS / FAIL / NOT RUN`. La suite di questo progetto ne ha
**quattro**: `scripts/rt-suite.ps1` esce `0` verde, `1` test falliti, `2` non avviata, **`3` NON VALIDA** —
`HEAD`, albero, binario o processi del motore cambiati fra inizio e fine (**D-222**). Una run non valida non
è né verde né rossa: **non si registra**.

**CRISPIN**: «Un formato di report che non può esprimere l'esito che il progetto considera più pericoloso
costringe chi compila a mentire per omissione. Finirà sotto `PASS` o sotto `NOT RUN`, e nei due casi il
lettore crede una cosa falsa.»

**NYGARD**: «Ed è il difetto che il progetto ha già pagato: una misura valida ma su un binario stantio.
Il campo esiste apposta.»

📝 **Riscrittura**: `PASS / FAIL / NOT RUN / NON VALIDA (exit 3 — dichiarare HEAD e binario)`.
🎯 **Priorità**: media — banale da correggere, costoso da subire.

### 🟠 F-09 · Autorità di scrittura auto-conferita, senza rollback — NYGARD, compliance

Il preambolo assegna il ruolo di «technical product owner + maintainer»; il §16 autorizza a **creare issue**,
il §17 a **cambiare priorità e milestone**, il §18 a **modificare i documenti owner**. Ma
[`CLAUDE.md`](../../../CLAUDE.md) §7 dice che *«un handoff/audit non è autorità e non autorizza da solo a
implementare tutto ciò che contiene»*, e §9 vieta commit, push e merge senza richiesta esplicita.

Le issue di GitHub, inoltre, **non si cancellano**: si chiudono. Venti issue create su premesse scadute — e ne
abbiamo misurate tre — restano visibili per sempre nella cronologia del repository.

**NYGARD**: «Qual è il rollback? Non c'è. Quando l'azione è irreversibile e la premessa è vecchia, l'ordine
giusto è **dry-run prima, scrittura dopo**: si produce l'elenco delle issue che *si creerebbero*, con il loro
§23-D accanto, e si scrive solo dopo conferma. La specifica ha già inventato il formato del dry-run al §23-B
e non se ne è accorta.»

📝 **Azione**: aggiungere un gate esplicito — `il §4 (audit) e il §23-B/D (elenco) si producono e si consegnano prima di qualsiasi scrittura su GitHub.`
🎯 **Priorità**: alta.

### 🟡 F-10 · Il §17 chiede di togliere dal critical path cose che non ci sono mai state — WIEGERS

L'elenco delle 13 voci da rimuovere include **GAS**, **networking**, **modding** e **progression**. Ma «No GAS
nella v0.1» è un pin di [`CLAUDE.md`](../../../CLAUDE.md) §3 e il networking è **v0.5** per struttura di
roadmap. Sono requisiti **già soddisfatti per costruzione**: chiederne la rimozione produce righe di report
che documentano non-lavoro.

📝 **Riscrittura**: separare *«fuori dalla v0.1 per decisione già presa — non toccare»* da *«dentro la v0.1
oggi e da spostare fuori — richiede un'azione»*. Solo la seconda lista è lavoro.
🎯 **Priorità**: bassa.

### 🟡 F-11 · Puntatori volatili dentro un documento normativo — DOUMONT, FOWLER

Il §2.2 dichiara — correttamente — *«non fidarti dello stato descritto in questo prompt»*. Poi il §15
**prescrive azioni** su una premessa di stato: *«è stata rilevata una possibile divergenza… se manca, creare
una issue decisionale»*. `#1657` **è chiusa**. La regola di igiene è giusta; l'applicazione la viola.

**FOWLER**: «È la separazione che [`CLAUDE.md`](../../../CLAUDE.md) applica a sé stesso: *"i numeri volatili
non stanno qui apposta: si misurano sul branch corrente quando servono, mai copiati da un documento"*. Un work
order dovrebbe avere due metà fisicamente separate — le **regole**, che durano, e i **puntatori**, che
scadono, con una data sopra. Qui sono intrecciati sezione per sezione, e l'unica difesa è la diffidenza del
lettore.»

🎯 **Priorità**: bassa — ma è la causa comune di F-01, F-02 e F-05.

### 🟡 F-12 · Il template issue del §16 non produce issue conformi alla tassonomia — GREGORY

Il template chiede `Why · Evidence · Owner · Scope · Out of scope · Acceptance criteria · Tests ·
Dependencies · Related` — buono, e migliore della media. Ma **non** chiede priorità (`P0…P3`), milestone, né
l'etichetta `post-v0.1`, che il §5 e il §17 usano come se esistessero nel template.

**GREGORY**: «E nessuna riga dice *chi altro ha guardato*. Un template scritto per un autore solitario produce
issue che nessuno ha discusso, in un repository che ha già pagato diciassette collisioni di ID.»

🎯 **Priorità**: bassa.

---

## 5. Cosa la specifica ha ragione di chiedere

Un referto che elencasse solo i difetti sarebbe disonesto: metà di questo documento va conservata.

- ✅ **§2.2 «Misura prima di decidere»** — `git fetch --prune`, `HEAD`, issue lette lato server e non
  trascritte. È lo stesso protocollo che [`../roadmap-v0.1-v1.0.md`](../roadmap-v0.1-v1.0.md) applica a sé.
- ✅ **§2.4 «Una feature ha un owner»** con il divieto esplicito dei titoli generici. L'esempio contrapposto
  — `[TD] Launcher: selezione Scenario / Action Lab senza biforcare il runtime` contro *«Migliorare Tactical
  Designer»* — insegna in due righe.
- ✅ **§12–13 riuso obbligatorio del Tactical Designer**: elenca i sistemi già consegnati e ordina di non
  ricrearli. Misurato: `#1114`–`#1117` sono chiuse davvero, quindi l'avvertimento era fondato.
- ✅ **§14 Action Lab**: gli acceptance criteria *«stesso seed · stesso draft · stesso StateHash · nessun
  branch di simulazione dedicato al tool · il tool chiama la facade canonica»* sono gli unici criteri
  **davvero misurabili** dell'intero documento, e sono anche i più importanti — proteggono l'invariante
  «l'editor non decide esiti».
- ✅ **§23-D «Issue NON create perché duplicate»** — il contributo migliore. Rende osservabile il non-lavoro.
- ✅ **§24 la gerarchia in caso di dubbio**: *«fra aggiungere una feature e chiudere l'integrazione del
  Complete Match, scegli il Complete Match»*. È una regola di priorità applicabile, e va tenuta.

---

## 6. La specifica riscritta — la parte che sopravvive al sorgente

Il documento originale esce dalla radice con questo commit. Ciò che di normativo vale la pena conservare sta
qui, in forma verificabile. **Non è un ordine di esecuzione**: è il testo da riusare quando quel lavoro verrà
effettivamente aperto.

### 6.1 Vincoli che non scadono

1. Il **Tactical Designer** non acquisisce una epic numerata di release né una milestone di gioco: resta
   figlio di [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105), `out_of_release_scope`.
   La build di gioco non dipende dall'Editor. *(sostituisce «non creare E48» — F-01)*
2. **Nessun terzo owner**: la vista v0.1→v1.0 è [`../roadmap-v0.1-v1.0.md`](../roadmap-v0.1-v1.0.md), lo stato
   dei gate è [`../v0.1-definition-of-done.md`](../v0.1-definition-of-done.md), le decisioni sono il
   [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md). Ogni altra pagina **linka**, non copia.
3. Gli **invarianti** valgono come scritti in [`../../product/piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md);
   il parallelismo e la validità della misura valgono come scritti in **D-222**. *(citazione, non riscrittura — F-06)*
4. Una issue nuova dichiara nel corpo **la ricerca di duplicazione eseguita**; una proposta scartata compare
   nel report con `Coperta da: #N`. *(F-03)*
5. Il **dry-run precede la scrittura**: l'audit e l'elenco delle issue proposte si consegnano prima di creare
   qualsiasi cosa su GitHub. *(F-09)*
6. Il **Complete Match è `G10`**, e la partita è completa quando raggiunge un esito terminale dichiarato —
   **vittoria di una delle due squadre oppure pareggio** — con la via registrata. Il pareggio chiude il gate
   solo se la partita **non è degenere**: un `RoundLimit` a zero eventi è lo stallo del bot, non una partita.
   *(decisione del product owner 2026-08-29 — F-05)*

### 6.2 Il formato di output, corretto

Le sezioni `A · B · C · D · E · F · G · I` del §23 si conservano invariate. La sola `H` cambia:

```text
H. Test
PASS:
FAIL:
NOT RUN:
NON VALIDA:   ← exit 3 di rt-suite.ps1; dichiarare HEAD e stato del binario
```

### 6.3 Le tre premesse da rifare prima di riusare il documento

| Premessa del sorgente | Stato al 2026-08-29 | Cosa fare |
|---|---|---|
| «creare la vista v0.1→v1.0» | già consegnata il 2026-08-28 | **verificarla**, non riscriverla |
| «`E48` non deve esistere» | `E48` = [#1408](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1408), epic v0.1 sana | usare §6.1-1 |
| «`#1657` DevSandbox è aperta» | **CLOSED** | rileggere la decisione presa prima di riaprire il tema |

---

## 7. Limiti dichiarati

- ⛔ **Nessuna suite eseguita, nessun build lanciato.** Il referto non dichiara verde né rosso alcun gate:
  riporta ciò che il DoD dichiara di sé, con le sue date.
- ⛔ **L'audit del §4 non è stato eseguito** — non era il compito. Le 22 issue puntatore sono state lette per
  `stato · milestone · titolo`; **non** è stato verificato «codice presente / test presenti» per nessuna epic.
- ⚠️ Il conteggio **54 epic (42 aperte · 12 chiuse)** viene da `gh issue list --search "EPIC in:title"`:
  conta i **titoli**, non la numerazione `E*`. Un'epic senza `EPIC` nel titolo non è contata.
- ⚠️ La convergenza fra la tabella release del §3 e la roadmap è stata verificata sui **nomi**, non sul
  contenuto di ciascuna release.

---

## 8. Provenienza

Il sorgente consumato è archiviato in
[`../../archive/src/handoff/2026-08-29-roadmap-issues-v01-v10-prompt.md`](../../archive/src/handoff/2026-08-29-roadmap-issues-v01-v10-prompt.md).
Era **untracked** in radice: senza l'archiviazione, rimuoverlo lo avrebbe perso senza traccia in git.
`docs/archive/src/` conserva i sorgenti già recepiti — **utile per la provenienza, mai per la regola**.
