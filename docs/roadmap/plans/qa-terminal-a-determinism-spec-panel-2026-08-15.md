# QA Terminal A — spec panel sul prompt «Deterministic Test Foundations»

> `CURRENT` · **Stato**: revisione chiusa, **non applicata** — il prompt non è stato eseguito e il
> documento recensito non è versionato ·
> **Data**: 2026-08-15
> **HEAD della revisione**: `28df2a96` (`origin/main`, worktree `D:/rt-simulation`, working tree pulito)
> **Oggetto**: `RT_QA_Claude_Terminal_A_Determinism.md` — 371 righe, **untracked** nella root di
> `D:/Repositories/refactor-tactics-main`, quindi non linkabile da qui e non incluso in nessun commit.
> Recensito insieme alle sue premesse condivise con i due file gemelli
> `RT_QA_Claude_Terminal_B_Scenario_CI.md` (519 righe) e
> `RT_QA_Claude_Terminal_C_Architecture_Roadmap.md` (662 righe), anch'essi untracked.
> **Panel**: Wiegers (lead) · Crispin · Adzic · Nygard · Fowler
> **Modo**: critique

## 0. Cosa è questo documento

Il file recensito non è una specifica del repository: è il **prompt operativo** di una sessione Claude —
uno di tre, pensati per girare in parallelo su Test/QA. Recensirlo come specifica è comunque il modo
giusto di leggerlo, perché è ciò che è: un mandato con ownership dichiarata, task numerati, vincoli e
criteri di consegna.

Il panel ha applicato una regola sola prima di parlare: **ogni premessa verificabile del documento è
stata misurata su `main`**, e nessun giudizio qui sotto poggia su ciò che il prompt afferma di sé.

## 1. Le premesse, misurate

Tutte le misure su `28df2a96` = `origin/main`, worktree pulito.

| Premessa del prompt | Misura | Esito |
|---|---|---|
| Ownership su `Tests/{Core,Map,Path,Turn,Helpers}` | `Tests/` è **piatto**: 105 `.cpp`, 1 `.h`; uniche sottocartelle `Golden/Movement.Basic` e `Golden/Movement.Collision` | ❌ i path non esistono |
| §1 «costruire il primo strato della macchina di test» | **859** test dichiarati in `Tests/`; `FRTCellId` nominato in **78** file di test | ❌ lo strato esiste |
| A5 — `StateHash` | `Turn/RTMatchStateHash.{h,cpp}` + `RTMatchStateHashTests.cpp`; **87** occorrenze; consumato da `Replay/`, `ScenarioHarness/`, `RTTurnManager` | ✅ esiste |
| A5 — `LogHash` | **20** occorrenze, **tutte in `Tests/`**, zero produttori fuori. Il produttore reale è `URTTurnLogLibrary::HashTurnLog`, chiamato da 5 file | ⚠️ esiste la semantica, non il nome |
| A6 — framework golden | `Tests/Golden/Movement.Basic` e `Tests/Golden/Movement.Collision` già a terra | ⚠️ da estendere, non da creare |
| A7 — permutazioni | `Simulation.ChecksumStableAcrossPermutations`, `Simulation.StateHashDistinguishesOutcomes`, `Replay.Verifier.ResimulationIsDeterministic` in `RTSimulationDeterminismTests.cpp` | ✅ esistono |
| A7 — «same seed → same output» | `ScenarioHarness/RTTestScenario.h:509` — «Seed dichiarato ma **non consumato**: oggi il progetto non ha alcun RNG»; idem `RTTestResult.h:74`. [`test-automatico-unreal.md`](../../technical/tooling/test-automatico-unreal.md) §4.1: «Il `seed` non fa niente, e va bene così» | ❌ vacuo per costruzione |
| A2 — «se manca una convenzione chiara» | [`test-automatico-unreal.md`](../../technical/tooling/test-automatico-unreal.md) (328 righe, spec del RT Scenario Test Harness), [`test-e-diagnosi.md`](../../technical/test-e-diagnosi.md), [`RT_TestMatrix_v0.1.md`](../../balance/RT_TestMatrix_v0.1.md) | ❌ la convenzione ha già owner |
| §11 «esegui la FAST suite se esiste» | nessuna FAST suite in `test-e-diagnosi.md`; `scripts/` sono 10 gate Python documentali, zero runner C++; `.github/workflows/` **assente** | ❌ non esiste |
| §0 «verifica la versione UE bloccata» | `RefactorTactics.uproject` → `"EngineAssociation": "5.8"`; il prompt dice `5.8.1` due volte | ⚠️ due risposte |
| Tre terminali in parallelo | nei tre prompt: `parallel-batch` **0** · `D-139` **0** · `writable` **0** · `AGENTS.md` **0** · `CLAUDE.md` **0** occorrenze | ❌ ignorano il meccanismo di allocazione |

**Sei premesse su undici sono falsificate dalla misura**, due sono ambigue, tre reggono.

## 2. 🔴 Critici

### C1 — L'ownership è dichiarata a una granularità che il repository non usa

**FOWLER** — Il documento definisce un confine per *cartella*, su cartelle che non esistono. Il
repository definisce i confini **per file**: [`../parallel-batch.yaml`](../parallel-batch.yaml) elenca i
test uno per uno, e registra esplicitamente il caso opposto — *«`Tests/RTCameraPawnTests.cpp` non è nel
`writable` di nessuna»*. Un confine dichiarato a una granularità diversa da quella del meccanismo non è
un confine più largo: è un confine che non morde.

**NYGARD** — E la formulazione è la modalità di fallimento. Il prompt dice *«Evita di modificare file di
Scenario Runner/CI/documentazione salvo stretta necessità»*. «Evita» e «salvo stretta necessità» sono una
raccomandazione; **D-139** dice *file non assegnato = STOP*. La differenza si vede solo quando due
terminali toccano lo stesso file — cioè quando è tardi. Nessuno dei tre prompt nomina il file che
deciderebbe.

📝 **Raccomandazione** — prima di eseguire A: o si apre una track in `parallel-batch.yaml` con `writable`
**per path di file**, o si dichiara esplicitamente che il batch è sospeso per questo giro. Non è un
dettaglio procedurale: senza, la prima modifica a un file di test è una violazione che nessun gate rileva.

### C2 — §0 e §START si contraddicono, e il documento non risolve la contraddizione

**WIEGERS** — §0 ordina *«verifica prima di modificare, non duplicare»*. §1 e §START ordinano *«costruire
il primo strato»* e *«implementa il primo vertical slice delle Deterministic Test Foundations v0.1»*: la
seconda presuppone assenza, e la misura dice 859 test, `StateHash` canonico, golden a terra, permutazioni
testate. Chi segue §0 resta senza compito; chi segue §START produce duplicati. Un mandato non può lasciare
questa scelta all'esecutore.

**ADZIC** — §10 «PRIMO MERGE TARGET» è la parte migliore del documento — sei condizioni concrete — ed è
anche la prova del problema. Confrontate una per una con `main`:

| Condizione §10 | Stato su `main` |
|---|---|
| 1 chiamata al resolver reale | ✅ `RTStress4v4Tests.cpp`, `RTShowcaseScenarioTests.cpp` |
| 1 assertion su stato finale o `StateHash` | ✅ `Simulation.StateHashDistinguishesOutcomes` |
| 1 assertion su TurnLog o `LogHash` | ✅ `URTTurnLogLibrary::HashTurnLog`, 5 file |
| 1 Repeat test | ✅ nella forma «due run confrontate» — `RTStress4v4Tests.cpp:333` |
| 1 Permutation test | ✅ `Simulation.ChecksumStableAcrossPermutations` |
| 1 snapshot fixture riutilizzabile | ⚠️ **l'unico delta** — vedi §5 |

**Cinque su sei sono già dimostrabili prima che il terminale parta.**

### C3 — «Same seed → same output» passerebbe sempre, anche a regole rotte

**CRISPIN** — Il repository dichiara due volte che il seed non è consumato (`RTTestScenario.h:509`,
`RTTestResult.h:74`) e la spec del harness ci dedica un paragrafo intitolato *«Il seed non fa niente, e va
bene così»*. Un test «stesso seed → stesso output» su un sistema **senza RNG** confronta una funzione
deterministica con sé stessa: verde per costruzione, verde anche a resolver rotto. È la classe peggiore —
quella che smette di verificare senza dirlo. Stesso vuoto per il vincolo §13 *«non usare random non
deterministico»*: non c'è random affatto.

📝 **Raccomandazione** — cassare la voce, oppure convertirla in **test di guardia**: *«se un RNG viene
introdotto, deve consumare il `Seed` dello scenario»*, con un'assertion che fallisce quando un RNG
compare. Così il campo `Seed` smette di essere decorativo e diventa un contratto.

## 3. 🟡 Maggiori

### M1 — `Repeat x100` non dichiara cosa varia fra le ripetizioni

**CRISPIN** — Oggi la ripetizione esiste nella forma giusta: due esecuzioni confrontate hash-per-turno,
con `FirstDivergentTurn` come diagnostica (`RTStress4v4Tests.cpp`, `RTShowcaseScenarioTests.cpp`). Senza
RNG e con ordine stabile, la centesima ripetizione **nello stesso processo** esegue lo stesso codice sugli
stessi dati della seconda: non aggiunge potere di falsificazione, aggiunge minuti. Il non-determinismo che
una ripetizione può catturare — ordine di container dipendente da indirizzi, hash seed per-processo — si
manifesta **fra** processi.

📝 Sostituire con un criterio che dica **cosa varia**: (a) N processi distinti; (b) ordine d'inserimento
permutato, che è già `ChecksumStableAcrossPermutations`; (c) enumerazione dei vicini, che §8 elenca a
parte. Se `x100` resta in-process, dichiarare quale invariante protegge **in più** rispetto a quelle già
coperte. Idem per il `x1000` «nightly»: non c'è nightly e non c'è CI.

### M2 — §12 chiede quattro numeri che nessun comando emette

**WIEGERS** — `Passed / Failed / Skipped / Duration` è un formato d'output senza una fonte. §11 dice
*«esegui la FAST suite se esiste»*: non esiste, e `.github/workflows/` è assente **per scelta** in questo
repository. Quattro campi senza produttore sono quattro campi che verranno riempiti a memoria.

📝 Fissare **il comando esatto** (invocazione `UnrealEditor-Cmd`, filtro automation, `-abslog`) prima di
chiedere il report, oppure ridurre il blocco a ciò che quel comando stampa davvero.

### M3 — Il prompt cerca `LogHash` per nome; nel repository esiste per semantica

**FOWLER** — *«Verifica se il progetto ha già `StateHash` / `LogHash`. Se esistono, usali.»* Una ricerca
letterale risponde «`StateHash` sì, `LogHash` no» e porta dritto a implementare un secondo hash del
TurnLog accanto a `URTTurnLogLibrary::HashTurnLog`, che ha già cinque consumatori. Il §13 vieta di
duplicare i **test**; il duplicato arriverebbe dal lato che il vincolo non copre.

📝 Nel prompt riferire i **simboli reali** invece dei concetti, e istruire la ricerca per semantica
(*«chi produce l'hash del TurnLog?»*) invece che per identificatore.

## 4. 🟢 Minori

- **m1** — Baseline UE: il prompt fissa `5.8.1`, il `.uproject` dichiara `5.8`. Chi esegue la verifica
  letterale del §0 trova due risposte e non sa quale sia autorevole. Va dichiarato.
- **m2** — §3 (A2) chiede di *consolidare* una struttura di cartelle e nello stesso paragrafo vieta i
  refactor massivi. Con 105 file piatti ogni consolidamento **è** massivo: le due istruzioni non sono
  entrambe soddisfacibili, va scelta una.
- **m3** — Gli handoff verso B e C sono richiesti come output ma **senza destinazione**: `docs/` compare
  0 volte in A e B, 1 volta in C. Un handoff senza path non ha un lettore.
- **m4** — §7 (A6) dice «*aggiungi* un primo framework golden»: i golden esistono. La parola giusta è
  «estendi», altrimenti nasce un secondo meccanismo accanto al primo.

## 5. Il delta reale — l'unico punto che sopravvive intatto alla misura

§4 (A3, fixture builders) e §5 (A4, `ResolveFixture`).

- `Tests/` ha **un solo** header condiviso: `RTOccupancyFixtures.h`, 187 righe, un namespace —
  **incluso da 1 solo file di test su 105**.
- Gli altri costruiscono mondo, unità e intenti con lambda locali: `Wall(...)`, `PlayAndHash(...)` sono
  ricorrenti.

La fixture condivisa esiste come **precedente**, non come **infrastruttura**: 1 consumatore su 105. È lì
che il lavoro di A avrebbe valore non duplicato — generalizzare quel precedente, non fondare uno strato
che c'è già.

⚠️ Con un avvertimento che chiude il cerchio su C1: la generalizzazione tocca **molti** file di test, cioè
è esattamente il lavoro che l'assenza di allocazione rende impossibile eseguire in sicurezza.

## 6. Cosa il documento fa bene — da tenere parola per parola

- **§7, la regola ferrea sui golden**: *golden cambiato → bug o cambio di regola intenzionale → decisione
  esplicita → nuovo golden*, mai aggiornamento automatico. È l'invariante che separa una suite che
  protegge da una decorativa.
- **§8**: *«ogni test deve dichiarare quale invariante/regressione protegge»*. È già come i test di questo
  repository sono scritti; metterlo per iscritto lo rende esigibile.
- **§10**: sei criteri d'accettazione falsificabili. Il difetto non è la forma — è che nessuno li ha
  confrontati con `main` prima di scriverli.
- **§13**: niente fake resolver, niente ordine implicito di `TMap`/`TSet`, niente dipendenza dal
  rendering. Allineato a [`../../../CLAUDE.md`](../../../CLAUDE.md).

## 7. Vincoli di allocazione misurati mentre si scriveva questo referto

Registrati qui perché sono la prova pratica di C1 — la stessa regola che il prompt ignora ha vincolato
questo documento.

- ⚠️ **La track `simulation` non ha un blocco `writable`.** In `parallel-batch.yaml` porta solo
  `status: IDLE`, `issue: null`, `branch: null` e la nota. Il suo write-set è dichiarato *«libero e non
  assegnato»*: letteralmente, **nessun path** è assegnato a questa track.
- ⚠️ **`parallel-batch.yaml` è `integration_only`**: non è modificabile da una track, quindi la track non
  può auto-assegnarsi il permesso che le manca. La riattivazione è un atto d'integrazione.
- ✅ **Il file di questo referto non collide con nessuno.** Cinque branch remoti aperti toccano
  `docs/roadmap/plans/` — write-set misurati con `git diff --name-only origin/main...origin/<branch>`:
  `wip/icon-visual-language` **8**, `docs/lane-6-7` **8**, `docs/five-lane-roadmap` **6**,
  `docs/38-spec-panel-playtest-hex` **2**, `docs/lane-7-vault` **2**. Nessuno tocca un file
  `qa-terminal-a-*`: intersezione **vuota**.
- ⚠️ **`docs/38-spec-panel-playtest-hex` esiste ancora lato server** benché la PR #875 risulti mergiata,
  e il suo write-set verso `main` è **2**, non 0. La voce di `parallel-batch.yaml` che lo dà a `3` file è
  invecchiata.
- 🔴 **`plans/README.md` dichiara 53 documenti; la cartella su `main` ne ha 57.** Fuori sincrono di
  quattro **prima** che questo file esista, e con questo file diventano cinque. Non è riparabile da qui:
  `docs/roadmap/plans/README.md` è `integration_only`. La riconciliazione — totale, e riga `SNAPSHOT`/
  `CURRENT` della tabella dei banner — spetta all'integrazione, che rimisura con i due comandi che il
  README stesso pubblica.

## 8. Sintesi

| Dimensione | Giudizio del panel |
|---|--:|
| Chiarezza del mandato | 4/10 |
| Completezza | 6/10 |
| Fondatezza delle premesse | 3/10 |
| Testabilità dei criteri | 8/10 |
| Compatibilità col repository | 2/10 |

Sono **giudizi del panel, non metriche calcolate**: non esiste uno strumento in questo repository che le
produca, e presentarle come misure sarebbe l'errore che il referto contesta al documento.

**Consenso** — il prompt è **eseguibile solo dopo tre correzioni**: C1 (allocazione dei file), C2
(riscrivere il mandato come *delta* invece che come *fondazione*), C3 (togliere il test vacuo sul seed).
Eseguito così com'è, il risultato più probabile è duplicazione d'infrastruttura esistente più una
violazione silenziosa del batch.

## 9. Limiti di questo referto

- `RT_QA_Claude_Terminal_A_Determinism.md` è stato letto **per intero**; dei due gemelli B e C sono state
  misurate **solo le premesse condivise** (le cinque grep della tabella §1). B e C possono avere difetti
  propri, non cercati.
- Nessuna build e nessuna esecuzione della suite: tutte le misure sono statiche (`grep`, `find`, `git`).
  Il numero **859** conta le dichiarazioni `IMPLEMENT_*_AUTOMATION_TEST`, non i test eseguiti né passati.
- I tre file recensiti sono **untracked**: non sono in `main`, non hanno una storia e possono cambiare
  senza lasciare traccia. Questo referto fotografa il loro contenuto al 2026-08-15, dimensioni
  `7758` / `9498` / `11915` byte.
