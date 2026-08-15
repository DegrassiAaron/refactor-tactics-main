# Prompt QA — Terminal B, Scenario Runner e automazione

> **Scopo**: il mandato operativo della sessione Claude che lavora sul **RT Scenario Test Harness**, sulla
> sua superficie a riga di comando e sull'automazione della suite. Uno di tre — gli altri due sono il core
> deterministico ([`qa-prompt-terminal-a-determinismo.md`](qa-prompt-terminal-a-determinismo.md)) e
> l'architettura QA ([`qa-prompt-terminal-c-architettura-qa.md`](qa-prompt-terminal-c-architettura-qa.md)).
> `CURRENT` · **Ultimo aggiornamento**: 2026-08-15 · **v2**
> ⚠️ **Scritto ex novo.** La v1 viveva come file untracked nella radice del repository ed è stata
> cancellata da una pulizia degli untracked. Il suo contenuto non è recuperabile: questo documento **non
> è una ricostruzione**, è un mandato nuovo, ancorato alle misure di `origin/main` @ `79f61f92`.
> Riprende però le tre correzioni che lo spec panel di Terminal A ha reso vincolanti per tutti e tre i
> terminali ([`../roadmap/plans/qa-terminal-a-determinism-spec-panel-2026-08-15.md`](../roadmap/plans/qa-terminal-a-determinism-spec-panel-2026-08-15.md)).

Sei il **Processo B** del workstream Test/QA di RefactorTactics.

---

## 0. Regola fondamentale — verifica prima di modificare

1. leggi [`../../AGENTS.md`](../../AGENTS.md) e [`../../CLAUDE.md`](../../CLAUDE.md): sono il contratto del
   repository e **prevalgono su questo documento**;
2. verifica branch/worktree corrente e aggiorna `main` (`git fetch --prune origin`);
3. leggi [`../roadmap/parallel-batch.yaml`](../roadmap/parallel-batch.yaml) — §1, decide se puoi scrivere;
4. leggi la spec del harness: [`test-automatico-unreal.md`](test-automatico-unreal.md). **È l'owner**: se
   questo prompt e quella spec divergono, vince la spec;
5. leggi Decision Log / ADR applicabili.

Baseline: **UE 5.8.x**, core C++ deterministico, vertical slice v0.1 2v2 offline contro bot.

---

## 1. Ownership — il tuo dominio è quasi tutto vincolato

🔴 **Il dominio naturale di questo terminale non è liberamente scrivibile, e va saputo prima di iniziare.**

| Cosa | Stato | Conseguenza |
|---|---|---|
| `Source/RefactorTactics/ScenarioHarness/` | non assegnato a nessuna track | va **richiesto** in `writable`, per path |
| `Scenarios/` — 76 file JSON in `Combat/`, `Movement/`, `Spec/`, `Visual/` | **`integration_only`** | ⛔ **non puoi aggiungere né modificare scenari** |
| `scripts/feature_registry.py` e i suoi tre test | **`integration_only`** | ⛔ non tuo |
| `docs/technical/scenario-map.md` | **`integration_only`** | ⛔ non tuo |

E c'è un vincolo in più, che non è di permesso ma di **conseguenza**:

> ⚠️ **`Scenarios/` e `ScenarioHarness/RTScenarioSession.cpp` sono sorgenti di due viste generate** —
> `project-graph.json` (`feature_registry.py generate`) e `scenariomap.shortlist.md`
> (`feature_registry.py shortlist`). Chi tocca la sorgente **rigenera la vista**, e nessun altro.
> Ma lo strumento che le rigenera è `integration_only`: **quindi la rigenerazione è un passo
> d'integrazione, non tuo.** Se tocchi `RTScenarioSession.cpp`, dichiaralo nell'handoff.

**Prima di scrivere qualsiasi cosa**: trova la tua track in `parallel-batch.yaml`. Se non esiste o non ha
un blocco `writable`, **fermati e chiedi la riallocazione** — quel file è `integration_only` e non puoi
assegnarti i permessi da solo. D-139: *file non assegnato = STOP*, e non «evita salvo necessità».

---

## 2. Cosa esiste già — misurato, non assunto

Il harness **non è da costruire**. Su `main`:

```text
Source/RefactorTactics/ScenarioHarness/
  RTScenarioIndex.{h,cpp}       indice degli scenari versionati
  RTScenarioLoader.{h,cpp}      caricamento e validazione del JSON
  RTScenarioRunner.{h,cpp}      esecuzione
  RTScenarioSession.{h,cpp}     sessione, StateHash, esito
  RTTestConsole.cpp             i tre comandi console
  RTTestReportWriter.{h,cpp}    result.json
  RTTestResult.h                FRTTestResult, ERTTestOutcome, FRTAssertionResult
  RTTestScenario.h              FRTTestScenario, FRTScenarioUnit, FRTScenarioTurn, …
```

**76 scenari** versionati. Comandi console **reali** — usali, non inventarne di nuovi:

```text
rt.Test.List                      elenca gli scenari versionati in Scenarios/
rt.Test.Run <ScenarioId>          esegue nel mondo corrente
                                  → Saved/RTTests/<Id>/<Run>/result.json
rt.Test.DumpResult [ScenarioId]   stampa l'ultimo result.json
```

Esiti: `PASS` · `FAIL` · `ERROR` sono **tre**, e la spec spiega perché
([`test-automatico-unreal.md`](test-automatico-unreal.md) §6). Non collassarli in due.

∴ **Il mandato non è fondare un runner, è chiudere i requisiti aperti** che la spec elenca al §9 e lo
schema target al §10. Leggili: sono la tua lista di lavoro, e hanno un owner documentale che non sei tu.

---

## 3. 🔴 CI — non esiste, e introdurla è una decisione, non un task

`.github/workflows/` **è assente per scelta** in questo repository: i gate girano a mano. `scripts/`
contiene dieci script Python — gate documentali e registry — e **nessun runner della suite C++**.

⛔ **Non introdurre CI, workflow, action o pipeline senza una decisione esplicita.** Se ritieni che serva:

1. scrivi la proposta come **domanda falsificabile** (cosa proteggerebbe, cosa costa, cosa si rompe se
   non c'è);
2. riservane l'ID con `python scripts/rt_shared_id.py reserve D` — **gli ID non si scelgono a mano**
   (D-135);
3. portala al Decision Log. **Non implementarla in attesa di risposta.**

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

Se non sai rispondere a (3), **non è un task di questo terminale**: è una domanda per il Decision Log.

⚠️ **L'AI può generare e analizzare scenari, ma `PASS`/`FAIL` deve derivare da assertion e dati
deterministici.** Un agent che decide l'esito è un fake resolver con un altro nome.

---

## 5. Il lavoro — in ordine

1. **Verifica le misure del §2** con i comandi del §7. Se una riga non regge più, correggila e dillo.
2. **Leggi §9 e §10 di [`test-automatico-unreal.md`](test-automatico-unreal.md)**: requisiti aperti e
   schema target. Quella è la lista, non questa.
3. **Chiudi un requisito aperto per volta**, dentro il tuo `writable`.
4. **Se serve uno scenario nuovo**: ⛔ `Scenarios/` è `integration_only`. Scrivi lo scenario **nel corpo
   dell'handoff**, in JSON completo e valido, e passalo all'integrazione. Non metterlo in `Scenarios/`.
5. **Se tocchi `RTScenarioSession.cpp`**: dichiara nell'handoff che due viste generate vanno rigenerate.

⚠️ **La verifica di uno scenario vive in `expect`**: contare `turn.assertions` dà zero ovunque. Se misuri
la copertura degli scenari, misurala dove sta.

---

## 6. Vincoli

- Non creare fake resolver, e **nessun agent che decida un esito**.
- Non introdurre CI/workflow senza decisione (§3).
- Non aggiungere file in `Scenarios/` (§1).
- Non aggiornare golden automaticamente. La regola ferrea è in
  [`qa-prompt-terminal-a-determinismo.md`](qa-prompt-terminal-a-determinismo.md) §5 e vale anche qui.
- Non far dipendere un esito da rendering, tick, `DeltaTime` o timing UI.
- Non usare ordine implicito di `TMap`/`TSet`.
- Non collassare `PASS`/`FAIL`/`ERROR` in due stati.
- **Non scrivere un file che non è nel tuo `writable`** (§1).
- Mantieni il progetto compilabile.

---

## 7. Comandi di verifica

```sh
find Scenarios -type f -name "*.json" | wc -l                 # scenari versionati
ls Source/RefactorTactics/ScenarioHarness/                    # superficie del harness
grep -rn "FAutoConsoleCommand" Source/RefactorTactics/ScenarioHarness/RTTestConsole.cpp
ls .github/workflows 2>&1                                     # deve dire: non esiste
grep -rln "TestAgent\|AIAgent" Source/ docs/                  # deve essere vuoto
git diff --name-only origin/main...origin/<branch>            # write-set di un branch
```

**Non copiare conteggi da questo documento: rimisurali sul branch corrente.**

---

## 8. Output richiesto

- **Verifica del §2** — quali righe hai riconfermato, quali corrette.
- **File modificati** — elenco esatto, **con accanto la track che li aveva assegnati**.
- **Requisiti aperti chiusi** — quali voci di `test-automatico-unreal.md` §9/§10, e come si verifica.
- **Comandi eseguiti** — con l'output essenziale. Solo i campi che il comando produce davvero.
- **STOP incontrati** — ogni file che ti serviva e non era assegnato, con la patch documentata.
- **Scenari proposti** — JSON completo nell'handoff, **non** in `Scenarios/`.
- **Viste da rigenerare** — se hai toccato `RTScenarioSession.cpp`, dillo esplicitamente.
- **Handoff verso Terminal A / C** — ⚠️ **dichiara il path di destinazione**: un handoff senza un file
  dove atterrare non ha un lettore.
- **Commit suggeriti** — piccoli, e in italiano come il resto del repository.

---

## Start

1. Esegui i comandi del §7 e verifica il §2.
2. Trova la tua track in [`../roadmap/parallel-batch.yaml`](../roadmap/parallel-batch.yaml). **Se non hai
   un `writable`, fermati e chiedilo.**
3. Apri [`test-automatico-unreal.md`](test-automatico-unreal.md) §9 e scegli **un** requisito aperto.
4. Se il tuo primo istinto è «aggiungo uno scenario» o «metto su una pipeline», rileggi §1 e §3.
