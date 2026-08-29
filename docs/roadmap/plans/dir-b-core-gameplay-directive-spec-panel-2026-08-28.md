# DIR-B · Core/Gameplay — spec panel sulla direttiva operativa v0.1

> `CURRENT` · **Stato**: revisione chiusa e **applicata nella v0.2**, che è versionata e vive accanto a
> questo referto: [`dir-b-core-gameplay-directive-v0.2.md`](dir-b-core-gameplay-directive-v0.2.md).
> *(Fino al 2026-08-28 questa riga diceva «non applicata — la direttiva è untracked e non è stata
> modificata», ed era vera quel giorno: la v0.1 era untracked e questo referto l'aveva lasciata intatta di
> proposito. La v0.2 è arrivata poche ore dopo, in radice; il 2026-08-30 è scesa qui.)* · **Data**:
> 2026-08-28
> **HEAD della revisione**: `707a8d95` (`main`)
> **Oggetto**: `REFACTORTACTICS — DIR-B · CORE - GAMEPLAY v0.1.md` (untracked, 648 righe), letta contro
> la DoD reale di [`#166`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166) (CP 14.6),
> `roadmap-v0.1.md` §E10/§E14, [`D-178`](../../decisions/RT_PDR_00_Decision_Log.md),
> [`D-222`](../../decisions/RT_PDR_00_Decision_Log.md) e `Source/`.
> **Panel**: Wiegers (lead) · Adzic · Cockburn · Fowler · Nygard · Crispin
> **Modo**: discussion
> **Particolarità**: è la prima revisione della serie che legge un **prompt operativo** invece di una issue
> o di un piano. Il soggetto non è ciò che va costruito, è **chi costruisce**. I difetti che ne escono non
> sono requisiti scritti male: sono **premesse false su come questo repository lavora oggi**, e una premessa
> falsa in un documento che si rilegge a ogni sessione costa una volta per sessione.

---

## 1. Il verdetto in una riga

La direttiva è **ben disciplinata e mal puntata**: i suoi divieti reggono quasi tutti, ma le tre priorità
che assegna sono **una fuori DoD, una già consegnata e una già specificata altrove con più precisione** —
e l'impianto di ownership su cui poggiano descrive un isolamento che il progetto ha misurato come
inesistente il giorno prima.

| | Voci |
|---|---:|
| 🔴 Critico | **4** |
| 🟠 Alto | **5** |
| 🟡 Medio | **5** |

**Raccomandazione operativa**: **non eseguire la direttiva come scritta.** Le correzioni `C1`–`C4` sono
quattro riscritture di testo, nessuna richiede lavoro di codice, e senza di esse una sessione DIR-B produce
codice non richiesto lasciando aperto il gate che dichiara di voler chiudere. `A1`, `A4` e `A5` riducono il
lavoro invece di aggiungerlo: tre delle voci di §12 esistono già.

---

## 2. 🔴 C1 — «worktree» è una premessa falsa, e §6 ci poggia sopra per intero

Il documento apre con:

> *«Sei **DIR-B**, il **worktree** dedicato al Core Gameplay C++ di RefactorTactics.»*

**Cockburn**: l'attore non è un worktree. `D-222`, misurato il **2026-08-27** — il giorno prima di questa
revisione — dice: **101** checkout di `HEAD` in 24 ore, **6** sessioni distinte a committare, **4** nella
stessa finestra di 6 minuti, e **un solo worktree**. Le sei condividono disco, `HEAD` e binario. `CLAUDE.md`
lo ripete come guardrail: *«⚠️ Niente worktree per parallelizzare: il mutex del motore è globale
sull'eseguibile, quindi due run di automation si uccidono anche da checkout diversi.»*

La conseguenza non è terminologica. Tutto il §6 — *«Non modificare file assegnati a DIR-A o DIR-C»* — è
scritto nella grammatica dell'**isolamento fisico**: un worktree che non vede gli altri. Il confine reale è
una **convenzione fra pari sullo stesso albero**, che nessun meccanismo fa rispettare e che un `git checkout`
altrui attraversa senza rumore.

**Evidenza vissuta in questa stessa revisione**: la sessione è partita su `feat/1467-velo-memoria-celle` a
`11f02b41`; alla prima verifica era su `main` a `707a8d95`. Qualcuno ha mosso `HEAD` mentre leggevo il
documento in review. La premessa non è falsa in teoria: è stata falsificata durante la lettura.

**Correzione**:

```diff
- Sei **DIR-B**, il worktree dedicato al **Core Gameplay C++ di RefactorTactics**.
+ Sei **DIR-B**, una **track di lavoro** sul Core Gameplay C++ di RefactorTactics.
+ ⚠️ Non hai un worktree tuo. D-222 misura sei sessioni su UNA sola working directory, che
+ condividono HEAD, albero, binario e i processi del motore (mutex globale sull'eseguibile).
+ Il confine con DIR-A e DIR-C è una CONVENZIONE, non un isolamento: `HEAD` può muoversi
+ sotto di te a metà lavoro, e va riletto — non ricordato.
```

---

## 3. 🔴 C2 — L'arbitro dei write-set è un file che il progetto ha rimosso

§6 chiude delegando la decisione più pesante che il documento contiene:

> *«Se `parallel-batch.yaml` o governance corrente assegna write-set differenti, vince la governance reale.»*

E §3 lo elenca fra i file *«CURRENT realmente esistenti»* da leggere in pre-flight.

**Wiegers**: verificato. `docs/roadmap/parallel-batch.yaml` **non esiste**. È stato rimosso da `D-178`
(`f90f9a01`, *«il sistema di lavoro parallelo esce dal repository, e lo sviluppo torna sequenziale»*),
insieme a `workflow-parallel-claude.md`, `rt_shared_id.py` e `test_rt_shared_id.py` — **8720 righe**. La
motivazione registrata è precisamente quella che rende inutile resuscitarlo:

> *«Il write-set è una misura che **scade**. La regola chiedeva di dichiarare i path prima di aprire la
> sessione, ma il write-set di un branch cambia a ogni commit: la dichiarazione era vera quando veniva
> scritta e falsa poco dopo.»*

Due difetti in uno. Il primo: §3 è **autocontraddittorio** — chiede di leggere i file che esistono davvero e
poi ne nomina uno che non esiste. Il secondo, più serio: il documento **delega** l'assegnazione dei diritti
di scrittura a un arbitro assente e **non dichiara il fallback**. Cosa fa l'esecutore quando va a cercarlo e
non lo trova? Il testo tace, e il silenzio in quel punto significa «decidi tu chi possiede cosa».

**Correzione**: togliere `parallel-batch.yaml` da §3 e da §6, e sostituire la delega con la governance che
esiste davvero — `AGENTS.md`, `docs/decisions/RT_PDR_00_Decision_Log.md`, e la regola di `CLAUDE.md` su
`git fetch --prune origin` più `gh pr list --state open` prima del merge.

---

## 4. 🔴 C3 — Niente in questo documento rende una suite **VALIDA**

§12 chiede di aggiungere test C++. §15 chiede di riportare *«Test eseguiti localmente»*.

**Nygard**: in un regime a sei sessioni sullo stesso binario, **«test eseguiti» non è un'evidenza**, ed è
`D-222` a dirlo con i numeri:

| Fallimento registrato | Cosa mostrava |
|---|---|
| `1233/1233, 0 fail` | aveva letto il **turno vuoto**: `HEAD` si era mosso a run iniziata |
| `641/1175`, `Fail = 0` | suite **morta a metà**, con l'aria di essere verde |
| `662/1191`, `Fail = 0` | idem |
| binario da un commit **cancellato da ogni branch** | vivo solo nel reflog |

> *«Il difetto non è il parallelismo, è il **silenzio**. Nessuna collisione produce un errore: producono
> verde che misura un'altra cosa. Una suite che fallisce si nota, una che mente no.»*

Il progetto ha già la contromisura: `scripts/rt-suite.ps1` legge le quattro invarianti — `HEAD`, albero,
binario, processi del motore — **prima e dopo** la run, confronta `Test Completed` con `Found N`, e dichiara
`VALIDA` o `NON VALIDA` nominando ciò che è cambiato. È l'unico file rimasto in `scripts/`, e l'esito è
leggibile a macchina: `0` verde · `1` test falliti · `2` non avviata (motore occupato) · **`3` NON VALIDA,
esito non registrabile**. Con `-Filter RefactorTactics.Scenario` si misura una sola area, molto più in fretta.

**La direttiva non la nomina.** Il §15 raccoglie quindi numeri che possono mentire e li consegna a DIR-A
**con la forma dell'evidenza**, che è il modo peggiore di sbagliare: DIR-A integra fidandosi.

**Correzione**, §12 e §15:

```diff
+ La suite si esegue SOLO tramite `scripts/rt-suite.ps1` (PowerShell, non Bash: MSYS traduce i path).
+ Una run senza verdetto di validità non si registra e non si riporta.

  Test eseguiti localmente:
- - ...
+ - <nome>: <esito>
+
+ Verdetto di validità della run (`rt-suite.ps1`):
+ - VALIDA / NON VALIDA
+ - se NON VALIDA: quale delle quattro invarianti è cambiata (HEAD, albero, binario, processi)
+ - `Test Completed` vs `Found N`
```

⚠️ Va riportato anche il limite che `D-222` dichiara e non risolve: dei tre esempi, **lo script ne copre
due**. Un binario già stantio *all'avvio* resta identico dall'inizio alla fine e **passa**. Chi esegue deve
saperlo, perché è l'unico buco che resta a suo carico.

---

## 5. 🔴 C4 — La Priorità 1 non è la Definition of Done di CP 14.6

§7 dedica la sezione più lunga e prescrittiva del documento a un **«Reaction Outcome Preview»** con
`AppliedDamage`, `Certainty`, `ReasonCodes` e breakdown player-facing.

**Adzic**: la DoD reale di CP 14.6 è `#166` in `v0.1-issue-plan.md:2917`. Ha cinque voci aperte:

| # | Voce della DoD | È nel documento? |
|---|---|---|
| 1 | UI `FIRE`/`HOLD` con countdown e bersaglio, alimentata da un **DTO sanitizzato** | 🟡 solo il DTO, in §8 |
| 2 | Slow-motion durante la finestra = **sola presentazione** | ❌ |
| 3 | Finestra visibile in sola lettura alla squadra; l'avversario non riceve **nulla** | ✅ §8 |
| 4 | **Durata reale della resolution misurata e registrata con 1, 2 e 3 unità armate** | ❌ |
| 5 | Se stabilmente `> 20 s` → revisione di ADR-0004 aperta **con i dati** | ❌ |

Il «Reaction Outcome Preview» **non compare in nessuna delle cinque**. E specularmente la voce **4** — la
sola che si chiude **headless**, quindi la sola perfettamente compatibile col vincolo «niente Editor» del §1,
quindi la più DIR-B di tutte — **non compare da nessuna parte nel documento**.

Eseguita alla lettera, la direttiva lascia CP 14.6 aperto **con più codice dentro**.

**Wiegers, sul rincaro**: e la sezione confligge col §5 dello stesso documento, che vieta *«nuove
macro-meccaniche»*. Un preview con danno risolto, certezza e breakdown autorizzato **è** una superficie di
prodotto nuova. Il testo si vieta al §5 ciò che si ordina al §7.

**Correzione**, testa del §7:

```diff
- # 7. PRIORITÀ 1 — REACTION CORE PER CP 14.6
+ # 7. PRIORITÀ 1 — LE VOCI CORE DELLA DoD DI CP 14.6 (#166)
+
+ Dalla DoD reale di #166, ciò che si chiude senza Editor:
+   1. Durata reale della resolution MISURATA e registrata con 1, 2 e 3 unità armate,
+      con la revisione di ADR-0004 aperta se stabilmente > 20 s.
+   2. Il DTO sanitizzato che alimenta la UI di DIR-A (contenuto autorevole: §8).
+   3. `Overwatch.SlowMotionDoesNotChangeOutcome` — il test che la DoD nomina e che non esiste.
+
+ Il "Reaction Outcome Preview" con AppliedDamage/Certainty/breakdown NON è in DoD.
+ Non implementarlo senza una decisione di scope esplicita: §5 vieta le macro-meccaniche nuove.
```

---

## 6. 🟠 A1 — Il decisore che il documento presuppone mancante **è atterrato**

`roadmap-v0.1.md:161`, chiusura di CP 14.5, dice che in partita la finestra si chiude in `HoldNoDecider` e
che `Spec.Overwatch.HoldThenFire` resta `BLOCKED`, sbloccabile *«dalla UI di CP 14.6 (`#166`) **o** dal
decisore iniettabile dell'harness (`#512`)»*.

**Cockburn**: applicando la scala di precedenza che il documento stesso dichiara al §2 — *codice prima della
roadmap* — quella riga risulta **superata**. In `Source/`:

| Simbolo | Dove | Cosa prova |
|---|---|---|
| `RefactorTactics.Scenario.OverwatchHoldThenFireConsumesBothDecisions` | `RTScenarioCorpusTests.cpp:585` | lo scenario **si esegue**, e verifica che **due** decisioni siano consumate |
| commento `#512` fase B | `RTScenarioCorpusTests.cpp:574` | *«`Spec.Overwatch.HoldThenFire` non è più una specifica in attesa: si esegue»* |
| `RefactorTactics.ShowcaseRelay.DecisionProviderIsInjectable` | `RTShowcaseScenarioTests.cpp:1025` | il seam del decisore esiste |

Due conseguenze, entrambe operative. La prima: **la roadmap è indietro rispetto al codice** su questa riga,
e chi la legge come stato corrente sopravvaluta il lavoro residuo. La seconda: dei due sblocchi alternativi
di CP 14.6, quello core **è già stato speso** — quindi ciò che resta a DIR-B è più stretto di quanto la
direttiva lasci credere, e coincide esattamente con le voci elencate in `C4`.

Il documento non nomina né `HoldThenFire`, né `HoldNoDecider`, né `#512`. Sono i tre termini con cui il
progetto parla di questo problema.

---

## 7. 🟠 A2 — `Preview = Commit` non è un'invariante verificabile

§7 pone:

```text
Confirmed Preview(boundary X, response R) = Commit(boundary X, response R)
salvo input esplicitamente Predicted o Uncertain
```

**Fowler**: come specifica non è falsificabile, per tre ragioni indipendenti.

1. **Uguaglianza di cosa.** Un `Commit` muta stato, scrive nel TurnLog e spende charge; un `Preview` per
   costruzione no — lo dice il §7 stesso, undici righe più sotto. I due non sono confrontabili finché non si
   dichiara **quali campi** entrano nel confronto.
2. **Come si osserva il `Commit`** senza commettere. Se il test deve committare per verificare l'uguaglianza,
   distrugge la premessa read-only che la sezione difende.
3. **Chi marca un input `Uncertain`.** Senza un predicato definito, l'esenzione assorbe qualunque divergenza:
   l'invariante non fallisce mai, e un'invariante che non può fallire non è un test.

La formulazione giusta è quella che il documento **quasi** dice due righe sopra — *«estrarre/riusare funzioni
pure condivise col commit reale»*. Detta bene: **una funzione pura, due chiamanti**. Non esiste «la formula
del preview»; esiste la formula del commit, e il preview la chiama.

```diff
- Confirmed Preview(boundary X, response R) = Commit(boundary X, response R)
+ Una funzione pura, due chiamanti. L'invariante diventa strutturale e verificabile:
+   - stesso snapshot + stessa response → stesso valore (test di purezza);
+   - nessuna seconda occorrenza della formula nel percorso di commit (verifica per lettura).
+ Se serve una clausola `Uncertain`, definisci il PREDICATO che la decide.
```

**Adzic**: e sessantotto righe di §7 non contengono **un solo numero**. Un esempio eseguibile dice più
dell'intera sezione:

```gherkin
Dato   un boundary con un watcher armato e il bersaglio in copertura bassa dal lato riparato
Quando si interroga il preview per la response FIRE
Allora il valore applicato è quello che la stessa funzione pura restituisce al commit
E      HP, Shield, charge, occupancy e seed sono invariati dopo la query
```

---

## 8. 🟠 A3 — `AppliedDamage` non esiste, quindi non è un riuso

§7 attenua: *«NON imporre questi nomi se strutture equivalenti esistono già.»*

**Wiegers**: verificato — `grep -rn "AppliedDamage" Source/` restituisce **zero occorrenze**. Non esistono
strutture equivalenti da riusare. La clausola di attenuazione, che nel testo ha la funzione di far sembrare
la sezione un allineamento a ciò che c'è, copre in realtà **la creazione di una struttura nuova**.

È lo stesso conflitto di `C4` visto dal lato del dato invece che dello scope: §5 vieta le macro-meccaniche
nuove, §7 ne ordina una, e la formulazione impedisce all'esecutore di accorgersene. Il documento deve
scegliere: o dichiara `AppliedDamage` come feature nuova con l'autorizzazione di scope che la copre, o la
toglie.

---

## 9. 🟠 A4 — Il corpus di test è fuori convenzione, e tre voci esistono già

§12 propone `ReactionPreview_Hit`, `ReactionReplay_Fire`, `Objective_MatchEnd`.

**Crispin**: la convenzione reale, letta sui file, è `RefactorTactics.<Categoria>.<CamelCase>` — categoria
puntata, **zero underscore**:

```text
RefactorTactics.Reactions.ArmedZoneFollowsCurrentCell     RTReactionOpportunityTests.cpp:404
RefactorTactics.Overwatch.OpportunityLeaksNoFuture        RTReactionOpportunityTests.cpp:218
RefactorTactics.Reactions.NoResolverWait                  RTReactionTests.cpp:286
```

⚠️ Esiste anche `RefactorTactics.Predictive.NoResolverWait` (`RTPredictiveTests.cpp:173`): due test con lo
stesso suffisso sotto categorie diverse. È un argomento in più per la convenzione — il nome **completo** è
l'unica cosa che li distingue.

Non è cosmesi: gate e selezioni girano su **pattern di nome**, e un corpus scritto con la convenzione
sbagliata non viene selezionato da nulla.

Peggio, il §3 predica «SEARCH → REUSE → UPDATE → CREATE solo se manca realmente» e il §12 consegna una lista
che quel SEARCH non l'ha fatto:

| Voce proposta | Stato reale |
|---|---|
| `ReactionPreview_NoHiddenLeak` | ampiamente coperta da `Overwatch.OpportunityLeaksNoFuture` |
| `ReactionReplay_NoLivePrompt` | adiacente a `Reactions.NoResolverWait` |
| `ReactionReplay_Fire` / `_Hold` | il percorso è già esercitato da `Scenario.OverwatchHoldThenFireConsumesBothDecisions` |
| **assente** | `Overwatch.SlowMotionDoesNotChangeOutcome` — **nominata dalla DoD di #166**, e non esiste in `Source/` |
| **assente** | `Reactions.ArmedZoneFollowsCurrentCell` — nominata dalla DoD, e **già verde** |

---

## 10. 🟠 A5 — La Priorità 3 è già specificata altrove, meglio

§10 dice: *«Se manca un pezzo core, chiudere soltanto il minimo necessario per: objective state, contest,
score/progress, winner/draw, match-end reason, TurnLog, replay.»*

**Wiegers**: sette sostantivi, zero criteri di accettazione, nessun owner spec citato, e «minimo necessario»
non è misurabile da nessuno. È la sezione col rapporto rischio/specifica peggiore: manda un esecutore a
toccare la condizione di fine partita senza dirgli quando ha finito.

E non serviva scriverla. La roadmap ha già la DoD **con i nomi dei test**:

| CP | Stato | Contenuto | Test nominati |
|---|---|---|---|
| **10.1** | ⏳ | `Action.Interact` su elemento adiacente — porta, consolle, ponte, obiettivo; legalità da tre filtri indipendenti | — |
| **10.2** | ⏳ | **Obiettivo contestabile**: contestazione anche con `Wait`, verifica nel **Cleanup**, contestazione paritaria = nessun progresso | `Objectives.ContestedNoProgress`, `Objectives.CheckedInCleanup` |
| **10.3** | ✅ | **Fine partita a tre vie**: eliminazione, obiettivo, `RoundLimit` da formato; parità = pareggio dichiarato | **27 test `Match*.*`** |

Metà della Priorità 3 — *winner/draw*, *match-end reason* — **è chiusa da CP 10.3**, con
`Turn/RTTurnRules.*`, `Turn/RTMatchFormatData.h` e la voce `PIE-V01-MATCHEND` già registrata. Il residuo di
E10 è dichiarato in una riga: *«⏳ nessun oggetto da attivare in mappa»*, cioè **CP 10.1 e CP 10.2**.

Sostituire i sette sostantivi con «CP 10.1 e CP 10.2, DoD e nomi dei test in `roadmap-v0.1.md:801-802`»
rende la sezione misurabile e le toglie il rischio di riscrivere ciò che è verde.

---

## 11. 🟡 I cinque rilievi medi

| # | Sezione | Rilievo | Evidenza | Correzione |
|---|---|---|---|---|
| **M1** | §9 | Priorità 2 **è già implementata** e il documento non fornisce il puntatore: il costo della riscoperta si paga a ogni sessione | `RecordedDecisions` e `ARTTurnManager::ReportOrphanRecordedDecisions()` in `Turn/RTTurnManager.cpp`, consumate da `Tests/RTSimulationDeterminismTests.cpp` | Incorporare i **simboli** nel testo; §9 diventa «aggiungi i test mancanti a questo», non «audita» |
| **M2** | §12, §15 | La verifica PIE si consegna in forma inventata (`test name / command / expected / prerequisite`) invece che con l'ID del sistema esistente | `PIE-V01-OVERWATCH` in `docs/technical/test-manuali-pie.md`; la DoD di `#166` la nomina | Chiedere **l'ID della voce PIE**; l'esito atteso vive già lì e non va duplicato |
| **M3** | §2 | Il `.pdf` compare come livello **6** di una scala di precedenza, il che gli attribuisce un'autorità che `D-009` gli nega — *«un `.pdf` non è **mai** autoritativo»* | `AGENTS.md`, `CLAUDE.md` §1 | Toglierlo dalla scala: è provenienza e rationale storico, non una fonte in graduatoria |
| **M4** | §3, §15 | Pre-flight senza `gh pr list --state open` (collisione `D-nnn`); handoff senza il **branch padre** della PR né l'eventuale `D-nnn` rivendicato | `CLAUDE.md` §4; regola PR: base = branch padre, **non** `main` | Aggiungere entrambi ai due elenchi |
| **M5** | intero doc | Nessuna data, versione o `HEAD` di riferimento, e il file è untracked: un documento di regime che scade in silenzio | `git status` | Intestazione con data più `HEAD` osservato; decidere se versionarlo o dichiararlo effimero |

---

## 12. Cosa regge, misurato

Voci controllate che **non** hanno prodotto rilievi, elencate perché una revisione che nomina solo i difetti
non dice quanto ha guardato:

| Proprietà | Come regge |
|---|---|
| §1 divieto Unreal Editor | Netto, verificabile per elenco di eseguibili, e con la via d'uscita già prevista: preparare → documentare → consegnare a DIR-A. È il vincolo meglio costruito del documento |
| §7 «Preview read-only» | Undici divieti espliciti (`muti HP`, `spenda cooldown`, `cambi seed`, `apra un'altra Decision Window`…), ciascuno falsificabile **singolarmente**. È la sottosezione scritta meglio |
| §8 privacy / sanitizzazione | Allineata a `Overwatch.OpportunityLeaksNoFuture`, già verde, e alla voce di DoD di `#166` *«l'avversario non riceve nulla, nemmeno l'esistenza della finestra»* |
| §13 criterio di stop | `BLOCKED_BY_DIR_A` / `BLOCKED_BY_DIR_C` **con evidenza concreta**: è il meccanismo giusto, e i rilievi `A1`/`A5` suggeriscono di usarlo più spesso, non meno |
| §4 principio deterministico | Coincide con la formulazione canonica (stesso stato + intenti + regole + config + seed), e il divieto di `Sleep`/`Delay`/wall-clock/`Timeline` nel resolver puro è quello di `CLAUDE.md` §4 |
| §11 TurnLog | *«Non cambiare versione/schema/hash senza auditare tutti i consumer e golden»* è coerente con la regola del CP 12.6 (rigenerazione solo con flag esplicito, e la PR dichiara perché l'esito è cambiato) |
| §14 stile di commit | I cinque esempi seguono la forma conventional-commit realmente in uso, e la clausola *«non usarli se la governance impone naming diverso»* è la subordinazione giusta |

**Non verificato in questa revisione, e va detto**: nessuna suite eseguita, nessuna compilazione, nessuna
lettura dello stato GitHub delle issue — `#166`, `#512` e `#152` sono stati letti **solo** nei documenti
locali, che su `A1` si sono già dimostrati indietro rispetto a `Source/`. Inoltre `roadmap-checkpoint.md`
porta *«ultimo aggiornamento 2026-08-08»*: la mappa delle milestone usata per il contesto è la meno fresca
delle fonti citate qui.

---

## 13. Cosa fare, in ordine

| # | Azione | Dove | Blocca l'esecuzione di DIR-B? |
|---|---|---|---|
| 1 | Sostituire «worktree» e dichiarare che il confine è convenzione (`C1`) | §RUOLO, §6 | **sì** |
| 2 | Togliere `parallel-batch.yaml` da pre-flight e da delega; nominare la governance reale (`C2`) | §3, §6 | **sì** |
| 3 | Imporre `scripts/rt-suite.ps1` e il campo «verdetto di validità» nell'handoff (`C3`) | §12, §15 | **sì** |
| 4 | Ripuntare la Priorità 1 sulle voci di DoD di `#166`; declassare il preview (`C4`, `A3`) | §7 | **sì** |
| 5 | Registrare che `#512` è atterrato e che `HoldThenFire` si esegue (`A1`) | §7, §9 | no, ma cambia il residuo |
| 6 | Riformulare l'invariante come «una funzione pura, due chiamanti» più un esempio con numeri (`A2`) | §7 | no |
| 7 | Riscrivere il corpus in `RefactorTactics.<Cat>.<CamelCase>`, dedotto dalla DoD, togliendo le voci già coperte (`A4`) | §12 | no |
| 8 | Sostituire i sette sostantivi con `CP 10.1` più `CP 10.2` e i loro test nominati (`A5`) | §10 | no |
| 9 | I cinque medi: puntatore a `RecordedDecisions`, ID PIE, `.pdf` fuori scala, `gh pr list` più branch padre, intestazione con data/`HEAD` (`M1`–`M5`) | §2, §3, §9, §12, §15 | no |

⚠️ **Perché nessuna azione tocca il codice.** Tutti e nove i rilievi si chiudono modificando **testo**. È il
punto: la direttiva non ha difetti di implementazione perché non è ancora stata implementata, e correggerla
ora costa nove edit; correggerla dopo costa una sessione di lavoro core buttata più il gate ancora aperto.

---

## 14. Nota di regime

**Perché quasi nessun riferimento porta un numero di riga.** Durante la stesura di questo referto `HEAD` si
è mosso **quattro volte** (`11f02b41` → `707a8d95` → `9018b5c3` → `ad7f212b`), `CLAUDE.md` è cambiato, e le
righe di `RTTurnManager.cpp` sono slittate di due mentre le citavo. Le ancore sono **simboli e ID**, che
sopravvivono a un commit altrui; i pochi `path:riga` rimasti (`:218`, `:247`, `:404`, `:585`, `:1025`,
`roadmap-v0.1.md:801-803`) sono stati riverificati a `ad7f212b` e vanno trattati come suggerimenti di
ricerca, non come indirizzi.

**Write-set**: la nozione non esiste più (`D-178`), e questo referto non ne dichiara uno. Ciò che vale al suo
posto è `D-222`: chiunque riesegua le verifiche di questo referto deve rileggere `HEAD` prima e dopo, perché
quello di questa revisione — `707a8d95` — si era già mosso una volta durante la sola lettura.

⛔ **La v0.1 in review non esiste più su disco, e non è recuperabile.** Era untracked, non è mai stata
committata (`git log --all` sul path: vuoto), ed è sparita durante questa sessione. Non è stata cancellata da
questa revisione, che l'aveva lasciata deliberatamente intatta scrivendo la correzione in un file separato.
È la stessa lezione di `M5` pagata sul campo: **un documento di regime che nessuno traccia non ha nessuno che
lo difenda.** Il testo originale sopravvive solo qui, sezione per sezione, e nelle parti che la
**v0.2** — [`dir-b-core-gameplay-directive-v0.2.md`](dir-b-core-gameplay-directive-v0.2.md), che le nove
correzioni le applica — conserva.
Quella, per la stessa ragione, è **versionata**.

**Nessun `D-nnn` riservato.** La revisione non introduce una regola nuova né supera una decisione esistente:
applica `D-178`, `D-222`, `D-009` e la DoD di `#166` a un documento che le contraddice. Se il triage
successivo ritenesse che serva comunque un id, si legge l'ultimo assegnato nel Decision Log e **si riverifica
prima del merge** con `git fetch --prune origin` più `gh pr list --state open` — lo strumento che lo faceva
in automatico (`rt_shared_id.py`) è uscito col resto di `D-178`, e il controllo è tornato a vista.
