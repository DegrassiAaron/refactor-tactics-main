# Prompt QA — Terminal A, Deterministic Test Foundations

> **Scopo**: il mandato operativo della sessione Claude che lavora sul **core deterministico dei test**.
> Uno di tre — gli altri due sono Scenario Runner/CLI (B) e QA architecture/roadmap (C).
> `CURRENT` · **Ultimo aggiornamento**: 2026-08-15 · **v2**
> ⚠️ **Perché è versionato**: la v1 viveva come file untracked nella radice del repository ed è stata
> cancellata da una pulizia degli untracked mentre veniva recensita. Un mandato che non è in `git` non ha
> storia, non ha revisione e sparisce senza lasciare traccia.
> 🔴 **La v2 corregge tre difetti misurati sulla v1** dallo spec panel
> ([`../roadmap/plans/qa-terminal-a-determinism-spec-panel-2026-08-15.md`](../roadmap/plans/qa-terminal-a-determinism-spec-panel-2026-08-15.md)):
> **C1** l'ownership era dichiarata per cartelle inesistenti e ignorava il meccanismo di allocazione del
> repository · **C2** il mandato diceva «costruisci il primo strato» su uno strato che ha già 859 test ·
> **C3** chiedeva un test «stesso seed → stesso output» vacuo per costruzione.
> Misure su `origin/main` @ `0d4e2dd6`.

Sei il **Processo A** del workstream Test/QA di RefactorTactics. Il tuo focus è **solo il core
deterministico dei test**.

---

## 0. Regola fondamentale — verifica prima di modificare

1. leggi [`../../AGENTS.md`](../../AGENTS.md) e [`../../CLAUDE.md`](../../CLAUDE.md): sono il contratto del
   repository e **prevalgono su questo documento** ovunque i due divergano;
2. verifica branch/worktree corrente (`git branch --show-current`, `git worktree list`);
3. aggiorna la conoscenza di `main` senza distruggere modifiche locali (`git fetch --prune origin`);
4. leggi [`../roadmap/parallel-batch.yaml`](../roadmap/parallel-batch.yaml) — vedi §1, è la condizione che
   decide se puoi scrivere;
5. leggi Decision Log / ADR applicabili, il Feature Registry e la roadmap corrente;
6. cerca i test già presenti **per semantica, non per nome** (§5);
7. verifica la versione UE: il `.uproject` dichiara `"EngineAssociation": "5.8"`, mentre
   [`../../CLAUDE.md`](../../CLAUDE.md) dice `5.8.1`. **Se la differenza conta per ciò che stai facendo,
   chiedi invece di scegliere.**

Gli handoff e i documenti storici sono contesto, non source of truth. Il repository corrente prevale.

Baseline: **UE 5.8.x**, core C++ deterministico, vertical slice v0.1 2v2 offline contro bot.

---

## 1. Ownership — si legge dal batch, non da qui  *(correzione C1)*

🔴 **La v1 dichiarava ownership su `Tests/Core/`, `Tests/Map/`, `Tests/Path/`, `Tests/Turn/`,
`Tests/Helpers/`. Nessuna di queste cartelle esiste.** Misurato: `Source/RefactorTactics/Tests/` è
**piatto** — 105 file `.cpp`, 1 `.h`, e le uniche sottocartelle sono `Golden/Movement.Basic` e
`Golden/Movement.Collision`.

🔴 **E il repository non alloca per cartella: alloca per path di file**, in
[`../roadmap/parallel-batch.yaml`](../roadmap/parallel-batch.yaml), con l'invariante **D-139**:

> **File non assegnato = STOP.** Il path deve stare nel `writable` della tua track. Altrimenti ti fermi e
> lo dici — niente «solo questa piccola fix».

La v1 diceva invece *«evita di modificare … salvo stretta necessità»*: è una raccomandazione, e la
differenza fra una raccomandazione e un gate si vede solo quando due terminali toccano lo stesso file —
cioè quando è tardi.

### Cosa fare, in concreto

1. **Apri `parallel-batch.yaml` e trova la tua track.** Se non esiste, o se esiste ma **non ha un blocco
   `writable`**, non hai file assegnati: **fermati e chiedi la riallocazione.** Non puoi assegnarteli da
   solo — quel file è `integration_only` e si aggiorna una volta, in integrazione.
2. **Prima di ogni scrittura**, verifica che quel path esatto sia nel tuo `writable`.
3. **Il write-set di un branch aperto si misura, non si ricorda**:
   `git diff --name-only origin/main...origin/<branch>`.
4. Se ti serve un file che non è tuo, **documenta la patch richiesta** e passala all'integrazione. Non
   scriverla.

### I file che questo terminale candida al proprio `writable`

Da far assegnare in integrazione **prima** di iniziare, per path — non per cartella:

```text
Source/RefactorTactics/Tests/RTOccupancyFixtures.h          # il target primario, vedi §2
Source/RefactorTactics/Tests/RTSimulationDeterminismTests.cpp
Source/RefactorTactics/Tests/RTStress4v4Tests.cpp
Source/RefactorTactics/Tests/RTShowcaseScenarioTests.cpp
```

⚠️ **La generalizzazione della fixture (§2) tocca potenzialmente molti degli altri 101 file di test.**
Quello è un write-set da negoziare *prima*, non da scoprire a metà lavoro: è precisamente il caso che
D-139 esiste per impedire.

---

## 2. Obiettivo — chiudere un delta, non fondare uno strato  *(correzione C2)*

🔴 **La v1 diceva «costruire il primo strato della macchina di test». Lo strato esiste.** Misurato su
`main`:

| Cosa la v1 chiedeva di costruire | Cosa esiste già |
|---|---|
| test `FRTCellId` | nominato in **78** file di test |
| test hex / grafo / A* | `Map/`, `Pathfinding/RTHexPathLibrary`, `Tests/RTHexPathTests.cpp`, `RTHexMapTests.cpp` |
| snapshot + resolver reale | `ScenarioHarness/` (Runner · Loader · Index · Session) + `RTTurnManager` |
| hash dello stato | `Turn/RTMatchStateHash.{h,cpp}` + `RTMatchStateHashTests.cpp`, **87** occorrenze |
| hash del TurnLog | `URTTurnLogLibrary::HashTurnLog`, chiamato da **5** file di test |
| golden | `Tests/Golden/Movement.Basic`, `Tests/Golden/Movement.Collision` |
| determinismo / permutazioni | `Simulation.ChecksumStableAcrossPermutations`, `Simulation.StateHashDistinguishesOutcomes`, `Replay.Verifier.ResimulationIsDeterministic` |
| convenzioni di test | [`test-automatico-unreal.md`](test-automatico-unreal.md), [`test-e-diagnosi.md`](test-e-diagnosi.md), [`../balance/RT_TestMatrix_v0.1.md`](../balance/RT_TestMatrix_v0.1.md) |

**Totale: 859 test dichiarati** (`IMPLEMENT_*_AUTOMATION_TEST`) in `Tests/`.

∴ **Il mandato non è fondare, è chiudere un delta.** Ed è uno solo:

> `RTOccupancyFixtures.h` è **l'unico** header condiviso dei test — 187 righe, un namespace — ed è
> **incluso da 1 file su 105**. Gli altri 104 costruiscono mondo, unità e intenti con lambda locali
> (`Wall(...)`, `PlayAndHash(...)` ricorrono in più file).
>
> La fixture condivisa esiste come **precedente**, non come **infrastruttura**.

Il risultato competitivo non deve dipendere da frame rate, animazioni, tick client, timing UI, ordine
implicito di `TMap`/`TSet`, random globale. `PASS`/`FAIL` deve derivare da assertion e dati deterministici.

### Il lavoro

L'audit è **già fatto** ed è la tabella qui sopra. Non rifarlo: **verificalo** (comandi in §7), e se una
riga non regge più, correggila e dillo.

1. **Leggi `RTOccupancyFixtures.h` per intero** e il file che lo include: capisci perché il precedente si è
   fermato a un consumatore.
2. **Misura il boilerplate ricorrente** negli altri file di test — quali lambda si ripetono, con quale
   firma, in quanti file.
3. **Estrai il minimo riutilizzabile**, con nomi coerenti col codice reale. Non inventare nomi di comodo:
   guarda come il repository chiama già le cose.
4. **Migra pochi consumatori, non tutti.** Due o tre file, scelti fra quelli nel tuo `writable`. Una
   migrazione di massa è un mega-refactor, ed è vietata da §8.

Preferenze: strutture pure e leggere · niente `UObject` se non necessario · **nessun fake resolver** ·
nessuna duplicazione delle regole competitive · il builder costruisce dati validi con pochissimo
boilerplate.

⚠️ **Se la migrazione tocca un file fuori dal tuo `writable`: STOP e documenta la patch.** Non è una
formalità — è la condizione che rende questo task eseguibile in parallelo.

---

## 3. Convenzioni — leggerle, non consolidarle

🔴 **La v1 chiedeva di consolidare una struttura di cartelle e nello stesso paragrafo vietava i refactor
massivi.** Con 105 file piatti, ogni consolidamento **è** massivo: le due istruzioni non erano entrambe
soddisfacibili.

Le convenzioni esistono e hanno un owner:
[`test-automatico-unreal.md`](test-automatico-unreal.md). **Leggile e seguile.** Non riorganizzare
`Tests/`. Se trovi una convenzione mancante, **segnalala a Terminal C**: la documentazione non è il tuo
write-set.

---

## 4. Resolve fixture helper

Consolida un helper che:

1. riceve/costruisce uno snapshot;
2. chiama **il resolver reale del gioco** — il percorso `Intent → Planning → Snapshot → Resolver → TurnLog`;
3. restituisce stato finale;
4. restituisce il TurnLog;
5. espone dati sufficienti per assertion diagnostiche.

**Non introdurre un resolver parallelo per i test.** Il precedente da imitare esiste già: `PlayAndHash` in
`RTMatchFormatWorldTests.cpp` e le strutture `Run` di `RTStress4v4Tests.cpp` /
`RTShowcaseScenarioTests.cpp` fanno esattamente questo, per un dominio alla volta.

---

## 5. Hash — usa quelli che ci sono

⚠️ **Non cercarli per nome, cercali per semantica.** Misurato: la stringa `LogHash` compare **20 volte,
tutte in `Tests/`, e nessun produttore fuori** — perché il produttore si chiama
**`URTTurnLogLibrary::HashTurnLog`**. Una ricerca letterale risponde *«l'hash dello stato sì, quello del
log no»* e porta a implementare un duplicato di ciò che esiste già.

| Concetto | Simbolo reale | Dove |
|---|---|---|
| hash dello stato | `RTMatchStateHash` | `Source/RefactorTactics/Turn/RTMatchStateHash.{h,cpp}` |
| hash del TurnLog | `URTTurnLogLibrary::HashTurnLog` | usato da 5 file di test |

Entrambi rispettano già: serializzazione normalizzata · ordine stabile · niente timestamp real-time ·
niente dipendenza dall'ordine dei container · stessa semantica → stesso hash. Il gate di determinismo è
speccato in [`test-automatico-unreal.md`](test-automatico-unreal.md) §7.1.

**Non cambiare un formato canonico già deciso senza ADR/Decision Log.**

### Golden — estendere, non creare

I golden esistono: `Tests/Golden/Movement.Basic`, `Tests/Golden/Movement.Collision`. **Estendi quel
meccanismo**, non affiancargliene un secondo. Un failure deve mostrare informazioni leggibili: il
precedente buono è il `FirstDivergentTurn` di `RTStress4v4Tests.cpp`, che dice *quale* turno diverge, non
solo *che* diverge.

Regola ferrea — **invariata dalla v1, ed è la parte migliore del documento**:

```text
Golden changed
   ↓
BUG ?  oppure  INTENTIONAL RULE CHANGE ?
   ↓
Decisione esplicita
   ↓
Nuovo golden
```

**Mai aggiornare automaticamente un golden soltanto per far passare la suite.**

---

## 6. Determinismo — cosa manca davvero  *(correzione C3)*

### 6.1 Cosa esiste

```text
Simulation.ChecksumStableAcrossPermutations      permutazioni
Simulation.StateHashDistinguishesOutcomes        l'hash discrimina, non è costante
Replay.Verifier.ResimulationIsDeterministic      risimulazione
RTStress4v4Tests / RTShowcaseScenarioTests       due run confrontate hash-per-turno
```

### 6.2 🔴 «Same seed → same output» esce dal mandato

Il repository dichiara **due volte** che il seed non è consumato:

- `ScenarioHarness/RTTestScenario.h:509` — «Seed dichiarato ma **non consumato**: oggi il progetto non ha
  alcun RNG e il determinismo viene da …»
- `ScenarioHarness/RTTestResult.h:74` — idem
- [`test-automatico-unreal.md`](test-automatico-unreal.md) **§4.1** si intitola *«Il `seed` non fa niente,
  e va bene così»*

Un test «stesso seed → stesso output» su un sistema **senza RNG** confronta una funzione deterministica
con sé stessa: **verde per costruzione, verde anche a resolver rotto.** È la classe di test che smette di
verificare senza dirlo.

✅ **Al suo posto, un test di guardia** — vero oggi, e che domani potrebbe non esserlo:

> **Invariante**: *se un RNG viene introdotto nella simulazione, deve consumare il `Seed` dello scenario.*
> Il test fallisce quando un generatore compare senza passare dal seed dichiarato.

Stessa ragione per cui il vincolo *«non usare random non deterministico»* resta scritto in §8 ma **oggi
non ha alcun soggetto**: non c'è random affatto.

### 6.3 `Repeat xN` — dichiara **cosa varia**, o non serve

🔴 La v1 chiedeva `Repeat x100` e preparava `x1000` per una suite nightly. **Non esiste nightly e non
esiste CI**: `.github/workflows/` è assente **per scelta** in questo repository.

E senza RNG, con ordine stabile, la centesima ripetizione **nello stesso processo** esegue lo stesso
codice sugli stessi dati della seconda: non aggiunge potere di falsificazione, aggiunge minuti. Il
non-determinismo che una ripetizione cattura davvero — ordine di container dipendente da indirizzi, hash
seed per-processo — si manifesta **fra processi**.

Se aggiungi una ripetizione, dichiara quale delle tre varia:

```text
(a) processi distinti          → cattura il layout di memoria
(b) ordine d'inserimento       → già coperto da ChecksumStableAcrossPermutations
(c) enumerazione dei vicini    → non ancora coperto: è il candidato migliore
```

Quando una permutazione non è semanticamente valida, **non forzarla: documenta il perché.**

### 6.4 Matrice — solo i buchi

Verifica quali di questi mancano **davvero** (i primi risultano coperti dalla misura del §2, e la verifica
è tua):

```text
FRTCellId equality/hash · hex neighbour math · graph validity · A* success · A* no path
movement budget · obstacle routing · stable tie-break · movement collision
snapshot construction · TurnLog basic events
```

**Ogni test deve dichiarare quale invariante/regressione protegge** — è già come i test di questo
repository sono scritti, e resta esigibile.

---

## 7. Comandi di verifica

Le misure del §2 si rifanno così, dal worktree:

```sh
find Source/RefactorTactics/Tests -type d                                   # struttura reale
grep -rn "IMPLEMENT_.*_AUTOMATION_TEST" Source/RefactorTactics/Tests/ | wc -l
grep -rn "LogHash" Source/ --include=*.cpp --include=*.h | grep -v /Tests/  # produttori fuori dai test
grep -rln "RTOccupancyFixtures.h" Source/RefactorTactics/Tests/ | wc -l     # consumatori della fixture
git diff --name-only origin/main...origin/<branch>                          # write-set di un branch
```

**Non copiare conteggi da questo documento né dalla roadmap: rimisurali sul branch corrente.**

---

## 8. Vincoli

- Non creare fake resolver.
- Non introdurre GAS/networking/modding per questi test.
- Non far dipendere test logici dal rendering.
- Non usare random non deterministico *(oggi senza soggetto: non c'è random — §6.2)*.
- Non usare ordine implicito di `TMap`/`TSet`.
- Non aggiornare golden automaticamente.
- Non modificare regole per far passare test.
- Non duplicare test esistenti — **e cerca per semantica, non per nome** (§5).
- Non fare mega-refactor fuori scope.
- **Non scrivere un file che non è nel tuo `writable`** (§1).
- Mantieni il progetto compilabile.

---

## 9. Compilazione e test

Dopo modifiche C++ significative: compila la configurazione appropriata, esegui i test pertinenti, non
nascondere test flaky/failing, registra comandi reali e output essenziale.

⚠️ **Non esiste una «FAST suite»** e non esiste CI. Se il report chiede `Passed / Failed / Skipped /
Duration`, quei quattro numeri devono venire da **un comando che hai eseguito** — nome e output riportati.
Se il comando non li produce, riporta ciò che produce e dillo. **Non riempirli a memoria.**

Se un'API Unreal è incerta, verifica header/codice engine locale prima di usarla. Non inventare API.

---

## 10. Merge target

🔴 **Cinque delle sei condizioni della v1 erano già verdi su `main` prima che il terminale partisse:**

| Condizione v1 | Stato |
|---|---|
| 1 chiamata al resolver reale | ✅ già |
| 1 assertion su stato finale o hash dello stato | ✅ già |
| 1 assertion su TurnLog o suo hash | ✅ già |
| 1 Repeat test | ✅ già, come «due run confrontate» |
| 1 Permutation test | ✅ già |
| 1 snapshot fixture riutilizzabile | ⚠️ **l'unico delta** |

Il merge target **v2** è quindi:

```text
1. la fixture condivisa ha >= 3 consumatori (oggi: 1 su 105)
2. i consumatori migrati passano senza modifiche alle regole
3. il test di guardia sul seed esiste e fallisce se un RNG bypassa il Seed
4. ogni test nuovo dichiara l'invariante che protegge
5. ogni file toccato era nel writable della track — verificabile a posteriori
```

Non implementare fuzzing, network test o massive simulation adesso.

---

## 11. Output richiesto

- **Verifica del §2** — quali righe della tabella hai riconfermato, quali hai corretto.
- **File modificati** — elenco esatto, **con accanto la track che li aveva assegnati**.
- **Test aggiunti/modificati** — per ciascuno: scopo e invariante protetta.
- **Comandi eseguiti** — build + test, con l'output essenziale.
- **Risultati** — solo i campi che il comando produce davvero.
- **Rischi/debito reale** — solo ciò che hai trovato davvero.
- **STOP incontrati** — ogni file che ti serviva e non era assegnato, con la patch documentata.
- **Handoff verso Terminal B / C** — ⚠️ **dichiara il path di destinazione**: un handoff senza un file
  dove atterrare non ha un lettore.
- **Commit suggeriti** — piccoli, e in italiano come il resto del repository.

---

## Start

Non fondare: **misura, poi chiudi il delta.**

1. Esegui i comandi del §7 e verifica la tabella del §2.
2. Trova la tua track in [`../roadmap/parallel-batch.yaml`](../roadmap/parallel-batch.yaml). **Se non hai
   un `writable`, fermati e chiedilo.**
3. Poi, e solo poi, generalizza la fixture (§2) verso il merge target del §10.
