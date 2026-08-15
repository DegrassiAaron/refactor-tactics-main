# Prompt QA — Terminal B, Scenario Runner e automazione

> **Scopo**: il mandato operativo della sessione Claude che lavora sul **RT Scenario Test Harness**, sulla
> sua superficie a riga di comando e sull'automazione della suite. Uno di tre — gli altri due sono il core
> deterministico ([`qa-prompt-terminal-a-determinismo.md`](qa-prompt-terminal-a-determinismo.md)) e
> l'architettura QA ([`qa-prompt-terminal-c-architettura-qa.md`](qa-prompt-terminal-c-architettura-qa.md)).
> `CURRENT` · **Ultimo aggiornamento**: 2026-08-15 · **v2.1**
> ⚠️ **Scritto ex novo.** La v1 viveva come file untracked nella radice del repository ed è stata
> cancellata da una pulizia degli untracked. Il suo contenuto non è recuperabile: questo documento **non
> è una ricostruzione**, è un mandato nuovo, ancorato alle misure di `origin/main` @ `79f61f92`.
> Riprende però le tre correzioni che lo spec panel di Terminal A ha reso vincolanti per tutti e tre i
> terminali ([`../roadmap/plans/qa-terminal-a-determinism-spec-panel-2026-08-15.md`](../roadmap/plans/qa-terminal-a-determinism-spec-panel-2026-08-15.md)).
> 🔴 **v2.1 — le affermazioni misurabili della v2 sono state rimisurate, e cinque non reggevano.**
> L'enum degli esiti ha **quattro** valori e non tre; i nomi console sono **cinque** e non tre;
> `ScenarioHarness/` **è** assegnato a una track; la ripartizione dei 76 JSON non sommava;
> `ERTTestOutcome` vive in un altro file. Ogni correzione porta sotto di sé la propria misura. Il
> referto le elenca una per una, insieme alla ragione per cui la v2 le aveva sbagliate tutte allo
> stesso modo — trascrivendo l'output del comando che si era già scritta:
> [`../roadmap/plans/qa-terminal-b-scenario-runner-spec-panel-2026-08-15.md`](../roadmap/plans/qa-terminal-b-scenario-runner-spec-panel-2026-08-15.md).

Sei il **Processo B** del workstream Test/QA di RefactorTactics.

---

## 0. Regola fondamentale — verifica prima di modificare

1. leggi [`../../AGENTS.md`](../../AGENTS.md) e [`../../CLAUDE.md`](../../CLAUDE.md): sono il contratto del
   repository e **prevalgono su questo documento**;
2. verifica branch/worktree corrente e aggiorna `main` (`git fetch --prune origin`);
3. leggi [`../roadmap/parallel-batch.yaml`](../roadmap/parallel-batch.yaml) — §1, decide se puoi scrivere;
4. leggi la spec del harness: [`test-automatico-unreal.md`](test-automatico-unreal.md). **È l'owner delle
   decisioni** — perché gli esiti sono distinti, perché il `seed` non fa niente, perché gli scenari sono
   JSON e non `.uasset`: su queste, se divergiamo, vince la spec.
   ⚠️ **Non è l'owner dei conteggi né degli enum.** Quelli si rimisurano sul codice (§7). La spec è stata
   indietro su entrambi fino al 2026-08-15 — diceva «cinque» scenari (§3) e argomentava «tre» esiti (§6)
   dove il codice ne ha 76 e quattro — e da lì l'errore è passato in questo mandato. Le due righe sono
   state corrette, ma il meccanismo che le aveva fatte invecchiare è ancora quello: una divergenza su un
   conteggio **si riporta all'owner, non si recepisce** — vedi §8;
5. leggi Decision Log / ADR applicabili
   ([`../decisions/RT_PDR_00_Decision_Log.md`](../decisions/RT_PDR_00_Decision_Log.md)).

Baseline: **UE 5.8.x**, core C++ deterministico, vertical slice v0.1 2v2 offline contro bot.

---

## 1. Ownership — il tuo dominio è quasi tutto vincolato

🔴 **Il dominio naturale di questo terminale non è liberamente scrivibile, e va saputo prima di iniziare.**

| Cosa | Stato | Conseguenza |
|---|---|---|
| `Source/RefactorTactics/ScenarioHarness/` | ✅ **assegnato**: `writable` della track `spatial`, che porta anche il `mandate` di questo documento | è il tuo write-set — ma la track è `IDLE`, vedi sotto |
| `Scenarios/` — 76 file JSON: **74** in `Combat/` (9), `Movement/` (6), `Spec/` (38), `Visual/` (21), più `RT_Showcase_Relay_v01.json` alla radice e `_redirects.json`, che **non è uno scenario** | **`integration_only`** | ⛔ **non puoi aggiungere né modificare scenari** |
| `scripts/feature_registry.py` e i suoi tre test | **`integration_only`** | ⛔ non tuo |
| `docs/technical/scenario-map.md` | **`integration_only`** | ⛔ non tuo |
| **questo file** | ✅ nel `writable` di `spatial`, dal 2026-08-15 | **correggilo quando lo misuri sbagliato** — è un passo del mandato, non un atto d'integrazione |
| `docs/technical/test-automatico-unreal.md` — la spec owner | **`integration_only`**: la usano più processi in parallelo | ⛔ la correzione si propone nell'handoff, anche quando hai ragione |

E c'è un vincolo in più, che non è di permesso ma di **conseguenza**:

> ⚠️ **`Scenarios/` e `ScenarioHarness/RTScenarioSession.cpp` sono sorgenti di due viste generate** —
> `project-graph.json` (`feature_registry.py generate`) e `scenariomap.shortlist.md`
> (`feature_registry.py shortlist`). Chi tocca la sorgente **rigenera la vista**, e nessun altro.
> Ma lo strumento che le rigenera è `integration_only`: **quindi la rigenerazione è un passo
> d'integrazione, non tuo.** Se tocchi `RTScenarioSession.cpp`, dichiaralo nell'handoff.
>
> ⚠️ **Due è il conto per il tuo write-set di oggi, non il conto della track.** `spatial` possiede anche
> `docs/roadmap/v0.1-definition-of-done.md` quando è attiva, e quello alimenta
> `milestonemap.shortlist.md` **e di nuovo** `project-graph.json`. Una sessione che tocca entrambe le
> sorgenti eredita **tre** viste — con `project-graph.json` contato una volta sola, non due. Il numero
> dipende da cosa hai scritto: ricavalo dal tuo write-set misurato, non da questa riga.

**Prima di scrivere qualsiasi cosa**: rileggi la tua track in `parallel-batch.yaml` — è `spatial`, ed è
lei a portare il `mandate` di questo documento. Misurato su `origin/main` il 2026-08-15:

```yaml
spatial:
  status: IDLE
  mandate: docs/technical/qa-prompt-terminal-b-scenario-runner.md
  writable:
    - Source/RefactorTactics/ScenarioHarness/
```

🔴 **`IDLE` con mandato non è `IDLE` e basta, e non è nemmeno un permesso di partire.** Il batch lo scrive
a chiare lettere: `status` passa ad `ACTIVE` quando una sessione parte **con una issue**, e uno
`status: ACTIVE` con `issue: null` è una contraddizione decidibile. ∴ il primo passo non è scrivere codice:
è **avere una issue** e portare la track ad `ACTIVE`. Senza issue non c'è niente da attivare, e il write-set
resta di nessuno.

⚠️ **Il `writable` è quello sopra, e finisce lì.** Non contiene `Scenarios/`, non contiene
`docs/technical/scenario-map.md`, non contiene questo file. Se ti serve altro, D-139 vale intero:
*file non assegnato = STOP*, e non «evita salvo necessità». Il write-set di un branch aperto si **misura**
(`git diff --name-only origin/main...<branch>`), non si ricorda.

---

## 2. Cosa esiste già — misurato, non assunto

Il harness **non è da costruire**. Su `main`:

```text
Source/RefactorTactics/ScenarioHarness/
  RTScenarioIndex.{h,cpp}       indice degli scenari versionati
  RTScenarioLoader.{h,cpp}      caricamento e validazione del JSON
  RTScenarioRunner.{h,cpp}      esecuzione
  RTScenarioSession.{h,cpp}     sessione, StateHash, esito
  RTTestConsole.cpp             tre comandi + due console variable
  RTTestReportWriter.{h,cpp}    result.json
  RTTestResult.h                FRTTestResult, FRTAssertionResult, ToString(esito)
  RTTestScenario.h              ERTTestOutcome (4 valori), ERTAssertionKind,
                                FRTTestScenario, FRTScenarioUnit, FRTScenarioTurn, …
```

**76 file JSON** in `Scenarios/`, di cui uno — `_redirects.json` — non è uno scenario (§1).

Comandi console **reali** — usali, non inventarne di nuovi:

```text
rt.Test.List                      elenca gli scenari versionati in Scenarios/
rt.Test.Run <ScenarioId>          esegue nel mondo corrente
                                  → Saved/RTTests/<Id>/<Run>/result.json
rt.Test.DumpResult [ScenarioId]   stampa l'ultimo result.json
```

E **due console variable nello stesso file**, che sono la superficie di automazione — cioè metà del titolo
di questo documento, e le prime due cose da conoscere qui dentro:

```text
rt.Test.Scenario <Id>     scenario eseguito AUTOMATICAMENTE all'avvio partita (vuoto = partita normale)
rt.Map.Source <Enum>      scavalca MapSource del GameMode (es. LevelAsset)
```

> ⚠️ **Da riga di comando servono `-dpcvars=`, non `-ExecCmds=`.** `-ExecCmds` gira *dopo*
> l'inizializzazione, quando il GameMode ha già allestito la partita: la variabile viene impostata, non
> serve a niente, e non c'è un errore che lo dica. Misurato sul pacchettizzato il 2026-08-10
> (`RTTestConsole.cpp`, commento sopra `CVarRTMapSource`).
>
>     RefactorTactics.exe -dpcvars=rt.Map.Source=LevelAsset

**Esiti — sono quattro, non tre.** `ERTTestOutcome` (`RTTestScenario.h`):

| Valore | Significato | Di chi è il difetto |
|---|---|---|
| `Pass` | simulazione completata, tutte le assertion soddisfatte | — |
| `Fail` | simulazione completata, almeno un'assertion non soddisfatta | **del gioco** |
| `Error` | impossibile eseguire: scenario invalido, eroe sconosciuto, mappa mancante | **del test** o dell'ambiente |
| `Blocked` | scenario valido, ha girato fin dove poteva, poi ha incontrato una capability **non ancora costruita** | **di nessuno** — è il progetto che non c'è ancora |

`Error` non è un `Fail`: confonderli fa cercare nel resolver un bug che è nel JSON. E `Blocked` non è
nessuno dei due — esiste per poter versionare uno showcase *prima* che tutti i suoi sistemi esistano,
senza tenere la suite rossa per settimane. Il valore è stato aggiunto **in coda** perché i precedenti non
cambiassero numero.

🔴 **`Blocked` è l'esito che tace, ed è quello che devi trattare con più sospetto.** Uno scenario `BLOCKED`
non è rosso e non è verde: passa senza dirlo. Se conti gli esiti, contalo a parte — mai fra i passati, mai
fra i falliti.

La spec owner ([`test-automatico-unreal.md`](test-automatico-unreal.md) §6) elencava **tre** esiti fino al
2026-08-15 ed è stata allineata: se leggi «e perché sono tre», stai leggendo una copia vecchia.

∴ **Il mandato non è fondare un runner, è portare avanti lo schema target** che la spec descrive al §10.
Leggilo: è la tua lista di lavoro, e ha un owner documentale che non sei tu. Il §9 è un'altra cosa — vedi
§5.2, che spiega perché non è la lista di partenza.

---

## 3. 🔴 CI — non esiste, e introdurla è una decisione, non un task

`.github/workflows/` **è assente per scelta** in questo repository: i gate girano a mano. `scripts/`
contiene dieci script Python — gate documentali e registry — e **nessun runner della suite C++**.

⛔ **Non introdurre CI, workflow, action o pipeline senza una decisione esplicita.** Se ritieni che serva:

1. scrivi la proposta come **domanda falsificabile** (cosa proteggerebbe, cosa costa, cosa si rompe se
   non c'è);
2. riservane l'ID con `python scripts/rt_shared_id.py reserve D` — **gli ID non si scelgono a mano**
   (D-135);
3. portala al Decision Log ([`../decisions/RT_PDR_00_Decision_Log.md`](../decisions/RT_PDR_00_Decision_Log.md)),
   che è `integration_only`: la scrivi **nell'handoff**, la registra l'integrazione.
   **Non implementarla in attesa di risposta.**

Lo stesso vale per una «FAST suite»: **non esiste**. Se un report ti chiede
`Passed / Failed / Skipped / Duration`, quei numeri vengono da **un comando che hai eseguito**, con nome
e output riportati. Se il comando non li produce, riporta ciò che produce e dillo.

---

## 4. 🔴 «AI Test Agent» — non ha soggetto: definiscilo prima di costruirlo

Misurato: nessun file in `Source/` o `docs/` contiene `TestAgent`, `AIAgent` o `AiAgent`. **Zero.**

È lo stesso difetto che lo spec panel ha trovato in Terminal A con il seed: un mandato che nomina una
cosa che non esiste, e che quindi non può essere né esteso né verificato.

Prima di scrivere codice, rispondi per iscritto:

```text
1. Che decisione prende l'agent che oggi prende una persona?
2. Su quale dato la prende — TurnLog? result.json? StateHash?
3. Come si falsifica il suo output? (se non si falsifica, non è un test)
4. Che cosa NON deve fare — in particolare: non deve poter cambiare un golden
   ne' riscrivere un'assertion per far passare uno scenario.
```

📍 **Le quattro risposte atterrano nel corpo dell'handoff**, sotto un titolo `## AI Test Agent — soggetto`.
Se diventano una domanda per il Decision Log, seguono la strada del §3.3. Vale qui la stessa regola che il
§8 impone agli handoff: *un testo senza un file dove atterrare non ha un lettore*.

Se non sai rispondere a (3), **non è un task di questo terminale**: è una domanda per il Decision Log.

⚠️ **L'AI può generare e analizzare scenari, ma l'esito deve derivare da assertion e dati
deterministici.** Un agent che decide l'esito è un fake resolver con un altro nome — e questo include
`Blocked`: decidere che uno scenario è bloccato è una lettura di capability, non un giudizio.

---

## 5. Il lavoro — in ordine

0. **Abbi una issue**, e porta `spatial` ad `ACTIVE` con essa (§1). Senza, non stai lavorando: stai
   scrivendo su un write-set che il batch considera di nessuno.
1. **Verifica le misure del §2** con i comandi del §7. Se una riga non regge più, correggila e dillo —
   ✅ e questo file **è tuo** (§1): correggilo qui, con la misura accanto. Un mandato che si sa sbagliato
   e aspetta l'integrazione resta sbagliato per tutti quelli che lo leggono nel frattempo.
2. **Leggi il §10 di [`test-automatico-unreal.md`](test-automatico-unreal.md)** — lo schema target. Quella
   è la lista, non questa.

   🔴 **Il §9 non è la lista di partenza, e va saputo prima di aprirlo.** Misurate il 2026-08-15, le sue
   quattro voci sono tutte non iniziabili da qui:

   | Voce §9 | Perché non parte |
   |---|---|
   | Assertion su HP, scudo, stati, TurnLog | *«da aggiungere quando uno scenario le richiede»* — ma `Scenarios/` è `integration_only` (§1). **Stallo** |
   | Intent diversi dal movimento | precondizione dichiarata, non un task: *«non procedere finché `Movement.Basic` non è stabile»* |
   | Politica per le Fast Reaction | dipende da **E14**, che non è atterrato |
   | Nessun bypass | invariante permanente — si rispetta, non si chiude |

   ∴ il lavoro eseguibile sta al **§10**, ed è tutto dentro il tuo `writable`: `mapId`, `facing`,
   `intents[].ability`, `surfaces[]`/`structures[]`/`objective`, `reactionPolicy[]`, `ruleset`, i tre modi
   del §10.5. Sono estensioni di `RTScenarioLoader` e `RTTestScenario.h`.
3. **Porta avanti un campo per volta**, dentro il tuo `writable`. Un campo è finito quando il loader lo
   legge, lo valida, e **uno scenario proposto in handoff lo esercita** — dichiarato, non promesso.
4. **Se serve uno scenario nuovo**: ⛔ `Scenarios/` è `integration_only`. Scrivi lo scenario **nel corpo
   dell'handoff**, in JSON completo e valido, e passalo all'integrazione. Non metterlo in `Scenarios/`.
5. **Se tocchi `RTScenarioSession.cpp`**: dichiara nell'handoff le viste generate da rigenerare, contandole
   sul tuo write-set misurato (§1: due per il solo `RTScenarioSession.cpp`, tre se hai toccato anche il DoD).

⚠️ **La verifica di uno scenario vive in `expect`**: contare `turn.assertions` dà zero ovunque. Se misuri
la copertura degli scenari, misurala dove sta.

⚠️ **Il §10.5 chiede che `HEADLESS`, `FAST` e `VISUAL` diano lo stesso esito logico, e che l'equivalenza
sia essa stessa un test.** Quel test ha bisogno di scenari, che non puoi scrivere: ∴ i due scenari
`Visual vs Fast` e `Fast vs Headless` si **propongono in handoff** insieme al codice dei modi, altrimenti
i modi atterrano senza ciò che li dimostra.

📋 **Il mandato è chiuso** quando un campo del §10 è nel loader, ha uno scenario proposto che lo esercita,
e l'handoff dichiara la track che ha scritto i file. Non «quando il §10 è completo»: quello è l'obiettivo,
non il criterio di una sessione.

---

## 6. Vincoli

- Non creare fake resolver, e **nessun agent che decida un esito**.
- Non introdurre CI/workflow senza decisione (§3).
- Non aggiungere file in `Scenarios/` (§1).
- Non aggiornare golden automaticamente. La regola ferrea è in
  [`qa-prompt-terminal-a-determinismo.md`](qa-prompt-terminal-a-determinismo.md) §5 e vale anche qui.
- Non far dipendere un esito da rendering, tick, `DeltaTime` o timing UI.
- Non usare ordine implicito di `TMap`/`TSet`.
- **Non collassare i quattro esiti** (§2). In particolare `BLOCKED` non si conta né fra i passati né fra i
  falliti: un esito che tace è il modo in cui una suite smette di misurare senza diventare rossa.
- **Non scrivere un file che non è nel tuo `writable`** (§1) — in particolare `Scenarios/` e la spec owner
  `test-automatico-unreal.md`, che sono `integration_only`. Questo documento invece è tuo: correggilo.
- Mantieni il progetto compilabile.

---

## 7. Comandi di verifica

```sh
# file JSON PER CARTELLA — il totale nudo (76) include `_redirects.json`, che scenario non è,
# e un file alla radice fuori dalle quattro categorie. Un totale nasconde entrambi.
git ls-tree -r --name-only origin/main -- Scenarios/ | grep '\.json$' \
  | sed 's|Scenarios/||; s|/.*||' | sort | uniq -c

git ls-tree -r --name-only origin/main -- Source/RefactorTactics/ScenarioHarness/   # superficie

# CINQUE nomi: tre comandi + due console variable. `FAutoConsoleCommand` da solo ne vede tre.
grep -n 'TEXT("rt\.' Source/RefactorTactics/ScenarioHarness/RTTestConsole.cpp

# QUATTRO valori. Leggi l'enum, non la spec: la spec §6 ne argomenta tre.
sed -n '/enum class ERTTestOutcome/,/};/p' Source/RefactorTactics/ScenarioHarness/RTTestScenario.h

git ls-tree -r --name-only origin/main -- .github/            # deve essere vuoto
grep -rln "TestAgent\|AIAgent\|AiAgent" Source/ docs/         # deve essere vuoto
git diff --name-only origin/main...origin/<branch>            # write-set di un branch
```

⚠️ **Blocco in Git Bash.** In PowerShell `wc` e `find` non esistono, e `Measure-Object -Line` **scarta le
righe vuote**: un conteggio fatto lì è più basso del vero senza dirlo. Le forme `git ls-tree` girano in
entrambe le shell e misurano **una revisione dichiarata** invece del working tree — che su un worktree di
feature non è `main`.

🔴 **Non copiare conteggi da questo documento: rimisurali.** E misurali **a una granularità più fine di
quella in cui l'affermazione è scritta** — è la lezione che la v2 ha pagato cinque volte: ogni suo comando
di verifica restituiva esattamente ciò che il testo affermava già, perché il testo era stato scritto *da*
quel comando. Un totale non falsifica una ripartizione; `FAutoConsoleCommand` non falsifica «tre comandi»;
la spec non falsifica un enum.

---

## 8. Output richiesto

- **Verifica del §2** — quali righe hai riconfermato, quali corrette **nel file** (§1: è tuo), e con quale
  misura. Le correzioni alla spec owner invece si **propongono** qui: quella è `integration_only`.
- **File modificati** — elenco esatto, **con accanto la track che li aveva assegnati**.
- **Campi del §10 portati avanti** — quali, e **con quale scenario si verificano**.
- **Comandi eseguiti** — con l'output essenziale. Solo i campi che il comando produce davvero.
- **Divergenze dall'owner** — se hai misurato un conteggio o un enum che `test-automatico-unreal.md`
  dichiara diverso, riportalo qui. Le due aperte al 2026-08-15 — §3 «cinque scenari», §6 «e perché sono
  tre» — sono state **chiuse**; aspettati che se ne riformino, perché una spec `as-built` invecchia da
  sola e nessun gate la rilegge. ⚠️ Il file è **`integration_only`** dal 2026-08-15, perché lo usano più
  processi in parallelo: la correzione **si propone nell'handoff**, non si applica — nemmeno quando
  l'hai misurata tu e hai ragione.
- **STOP incontrati** — ogni file che ti serviva e non era assegnato, con la patch documentata.
- *(se applicabile)* **Scenari proposti** — JSON completo nell'handoff, **non** in `Scenarios/`.
- *(se applicabile)* **Viste da rigenerare** — se hai toccato `RTScenarioSession.cpp`, dillo
  esplicitamente, con il conto ricavato dal write-set (§1).
- *(se applicabile)* **Handoff verso Terminal A / C** — ⚠️ **dichiara il path di destinazione**: un
  handoff senza un file dove atterrare non ha un lettore.
- **Commit suggeriti** — piccoli, e in italiano come il resto del repository.

---

## Start

1. Esegui i comandi del §7 e verifica il §2. **Cinque righe della v2 non reggevano**: aspettati di
   trovarne altre, e misura più fine di come è scritta l'affermazione.
2. Apri [`../roadmap/parallel-batch.yaml`](../roadmap/parallel-batch.yaml) alla track **`spatial`** — è
   quella che porta il `mandate` di questo documento, gira in `D:/rt-spatial`, e il suo `writable` è
   `Source/RefactorTactics/ScenarioHarness/` più questo file. **È `IDLE`: senza una issue con cui portarla
   ad `ACTIVE` non parti** (§1). Se il `writable` che leggi non è più quello, vince il file, non questa riga.
3. Apri [`test-automatico-unreal.md`](test-automatico-unreal.md) **§10** e scegli **un** campo dello
   schema target. ⛔ **Non il §9**: le sue quattro voci sono tutte bloccate, e §5.2 misura da cosa.
4. Se il tuo primo istinto è «aggiungo uno scenario» o «metto su una pipeline», rileggi §1 e §3.
5. Se trovi che questo prompt sbaglia una misura, **correggilo** — è nel tuo `writable`, e sei tu che stai
   guardando il codice. Ma correggi il file, non solo il tuo ricordo: cinque affermazioni della v2 erano
   false, e sono rimaste in circolo finché nessuno le ha riscritte.
