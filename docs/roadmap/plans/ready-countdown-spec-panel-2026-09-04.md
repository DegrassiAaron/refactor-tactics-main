# Spec panel — Ready countdown e Unready (#2193)

**Data**: 2026-09-04 · **Misurato su**: `origin/main` `27d687d6` · **Modalità**: critique
**Panel**: Wiegers (requisiti) · Cockburn (attore/goal) · Fowler (confini) · Nygard (failure mode) · Adzic (esempi) · Crispin (testabilità)

> Referto di una sessione di specifica su una issue che dichiarava una decisione di design **aperta**. Non è
> un owner: la issue #2193 resta proprietaria del comportamento, `spec-durata-partita-e-scala-mappe.md` §7.2
> della regola.

---

## 0. Cosa esisteva prima del panel

| Fonte | Cosa dichiara |
|---|---|
| `spec-durata-partita-e-scala-mappe.md` §7.2 | `ReadyCountdown` **3 s**; Unready annulla e torna al planning; non sostituisce il timer massimo |
| `test-manuali-pie.md` → `PIE-V01-READY` | stesso criterio, stato ⏳ *«il countdown non esiste ancora»* |
| #2193 | l'assenza è misurata; il **dove** vive il countdown è dichiarato indeciso |

Misure rifatte su `27d687d6`, perché la issue misurava su `6aa51ec7`:

| Misura | Esito |
|---|---|
| `Unready` in `Source/` | **0 file** — la premessa regge |
| `countdown` in `Source/` | 4 file, **tutti** della finestra di reazione (ADR-0004 §8) — nessuno del Ready |
| chiamanti di `LockInAndResolve()` fuori dai test | **3**: `RTPlayerController.cpp:1620` (umano), `RTScenarioRunner.cpp:100`, `RTScenarioSession.cpp:1467` |
| ingresso del timeout | `OnPlanningTimeout` → stesso punto comune |
| `UInputAction` come asset | **nessuno**: sono creati in C++ (`NewObject<UInputAction>`), mappati con `MapKey` |

🔑 **L'ultima riga cambia il costo del lavoro**: un gesto nuovo non richiede un asset, quindi non richiede
l'Editor.

---

## 1. Findings

### 🔴 F1 — Il requisito nomina un gesto che non esiste — *Wiegers*

> *«premendo **Unready** durante il countdown»* — con quale tasto? Il criterio PIE non lo dice, la spec
> neanche. Un requisito che nomina un'azione senza un ingresso non è verificabile: chi esegue la voce PIE non
> sa cosa premere.

Aggravante misurata: `Spazio` è **già** a due significati (`OnLockIn` → lock-in in planning, `SkipPlayback`
durante la risoluzione). Il countdown aggiunge un terzo stato al medesimo tasto se non si decide altrimenti.

**Risolto in questa sessione** → vedi §2.

### 🔴 F2 — `LockInAndResolve()` è un punto di confluenza, non un punto di ingresso — *Fowler*

> Tre chiamanti, di cui **due non umani e headless**. Mettere un countdown dentro quella funzione
> significherebbe far aspettare tre secondi allo Scenario Harness e all'autobattle, cioè introdurre tempo
> reale in un percorso che oggi non ne ha.

∴ il countdown vive **a monte** del punto comune, e il punto comune resta sincrono.

### 🔴 F3 — Due orologi, e nessuno ha detto chi vince — *Nygard*

> Il criterio dice *«il countdown non sostituisce il timer massimo: sono due orologi distinti»*. Bene: e se il
> timer massimo scade **mentre** il countdown scorre? Non è un caso limite raro — è il caso normale di chi
> dichiara Ready a `PlanningSeconds - 1`.

Tre esiti possibili, tutti difendibili e **nessuno scritto**: il commit parte subito; il countdown continua
oltre il tetto; l'Unready diventa impossibile. Senza una regola, l'implementazione la sceglie per caso.

**Regola adottata**: il **timer massimo vince e committa**. Il countdown è una cortesia dentro il tetto, non
un'estensione del tetto — è l'unica lettura compatibile con *«non sostituisce il timer massimo»*, e l'unica
che tiene il turno limitato superiormente.

### 🟡 F4 — In v0.1 l'attore è uno solo, e il goal non è quello scritto — *Cockburn*

> La spec descrive *«tutti Ready → countdown»*: un meccanismo di **attesa reciproca**. La v0.1 è 2v2 offline
> contro bot: **un solo umano**. La spec lo dice da sé — *«in 2v2 offline la differenza è nulla; diventa reale
> con il 3v3 e con M10»*.

Il goal che resta vero **oggi** è un altro: *annullare una chiusura involontaria*. È anche ciò che #2193
dichiara — *«aggiunge una finestra d'annullamento, non cambia il gesto»*.

∴ in v0.1 «tutti Ready» ≡ «io Ready», e il countdown si arma sul lock-in umano. La forma multi-giocatore non
va implementata adesso, ma la funzione che arma il countdown non deve rendere quella forma più difficile
dopo.

### 🟡 F5 — «senza aver perso il piano» è falso a schermo prima che nei dati — *Wiegers · Fowler*

Misurato in `RTPlayerController.cpp:1618`: `OnLockIn` chiama `RefreshPlanningPreview(GetWorld(), nullptr)`
**prima** di `LockInAndResolve`, con il commento *«l'anteprima muore col lock-in»*.

> Se il countdown si interpone e l'anteprima muore all'**inizio** del countdown, un Unready riporta al
> planning un piano che c'è nei dati e non si vede. Il criterio dice *«senza aver perso il piano»*: per chi
> gioca, un piano invisibile è un piano perso.

∴ l'anteprima muore alla **fine** del countdown, non al suo inizio.

### 🟡 F6 — Tre clausole su quattro sono headless, e il criterio le manda tutte in PIE — *Crispin*

| Clausola di `PIE-V01-READY` | Dove si prova davvero |
|---|---|
| parte un countdown di 3 s prima del commit | **headless** |
| Unready torna al planning senza perdere il piano | **headless** (dati) + PIE (la si vede) |
| il countdown non sostituisce il timer massimo | **headless** |
| si annota a che secondo si dichiara Ready | **PIE** — è osservazione, non un asserto |

> Una voce manuale che porta anche ciò che un test regge è una voce che invecchia: il giorno che qualcuno
> rompe il countdown, il test tace e la voce PIE lo scopre alla prossima seduta. Che può essere fra un mese.

### 🔵 F7 — Nessuno scenario eseguibile — *Adzic*

Gli scenari `Given/When/Then` sono in §3. Sono la forma che i test devono asserire.

---

## 2. La decisione che la issue dichiarava aperta

> #2193: *«Va deciso **prima** se il countdown sia locale alla presentazione — il piano parte solo allo
> scadere — oppure uno stato di simulazione; e nel secondo caso tocca lo snapshot, quindi il replay.»*

**Nessuna delle due, come sono poste.** La dicotomia presuppone che «stato del TurnManager» implichi «stato
di simulazione», e qui non è vero:

```text
[planning]  ── Ready ──▶  [countdown 3 s]  ── scade ──▶  LockInAndResolve()
     ▲                          │                              │
     └──────── Unready ─────────┘                     snapshot immutabile
                                                              │
                                                        risoluzione
```

Il countdown **finisce prima** della riga che crea lo snapshot. Non entra nello snapshot, non entra nel
TurnLog, non raggiunge il replay e non tocca lo `StateHash`.

**Adottato**: stato del **ciclo di partita**, di proprietà di `ARTTurnManager`, a monte del punto comune.

🔑 **E la scelta era già scritta**, in una riga che nessuna delle due fonti principali cita.
`spec-durata-partita-e-scala-mappe.md:542` classifica `ReadyCountdownSeconds` fra i **Tempi UX**, accanto a
`PlanningMaxSeconds` e `FastReactionDefaultSeconds`, con il vincolo: *«Tempo di **parete**: non devono **mai**
raggiungere il TurnLog — `spec-pacing-turno.md` **D3**, "un tempo di parete lì dentro lo renderebbe non
deterministico"»*.

∴ la dicotomia della issue era già risolta dalla spec: il countdown è **tempo di parete**, sta con il timer di
planning, e non attraversa il confine del TurnLog. Il panel non ha deciso — ha **trovato** la decisione.

⚠️ **Il nome canonico è `ReadyCountdownSeconds`**, non `ReadyCountdown`: la tabella di §7.2 usa la forma corta,
le righe 485 e 542 quella lunga, ed è quest'ultima che sta accanto agli altri due parametri di tempo. Il
codice usa la forma lunga.

Perché non nel `PlayerController`:

1. il TurnManager **possiede già** l'altro orologio (`PlanningTimerHandle`); F3 chiede che i due interagiscano
   con una regola, e una regola con due proprietari è due regole;
2. l'autorità non ha accettato niente durante il countdown — il vincolo di #2193 è rispettato **per
   costruzione**, non per disciplina;
3. lo Scenario Harness continua a chiamare `LockInAndResolve()` diretto: **non vede il countdown**, quindi il
   determinismo headless non cambia di una riga.

⛔ E non è presentazione: un countdown che vivesse nel controller morirebbe con lui e non saprebbe del tetto.

### Il gesto — deciso il 2026-09-04

`UndoAction` (**RMB / Backspace**) diventa l'Unready durante il countdown.

| Stato | Spazio | RMB / Backspace |
|---|---|---|
| `Planning` | Ready → arma il countdown | annulla l'ultimo waypoint |
| `ReadyCountdown` | *inerte* | **Unready** → torna al planning |
| `Resolving` | `SkipPlayback` | inerte |

Perché non un toggle su Spazio: chi preme due volte per abitudine annullerebbe senza volerlo — **l'opposto
esatto** del difetto che il countdown esiste per prevenire. Perché non un tasto nuovo: il kit v0.1 ha già
nove voci più quattro generiche (`E48` #1408), e un tasto attivo tre secondi a turno è il candidato peggiore
per allungarlo.

Il riuso regge perché gli stati sono **mutuamente esclusivi**: durante il countdown non ci sono waypoint da
annullare, quindi il tasto Annulla non ha due significati contemporanei — ne ha uno per stato, che è la
condizione che #1957 chiede a `OnLockIn`.

---

## 3. Scenari eseguibili — *Adzic*

```gherkin
Scenario: il Ready anticipato non committa subito
  Given una partita in planning con PlanningSeconds = 30 e ReadyCountdown = 3
    And il giocatore ha un piano su almeno un'unità
  When il giocatore dichiara Ready al secondo 10
  Then il turno NON è ancora risolto al secondo 12
    And il turno è risolto al secondo 13

Scenario: Unready torna al planning senza perdere il piano
  Given un countdown armato
  When il giocatore preme Unready prima che scada
  Then il turno non si risolve
    And i piani delle unità sono identici a prima del Ready
    And l'anteprima di pianificazione è di nuovo visibile
    And il timer massimo NON è stato riarmato: continua da dov'era

Scenario: il tetto vince sul countdown
  Given PlanningSeconds = 30 e ReadyCountdown = 3
  When il giocatore dichiara Ready al secondo 28
  Then il turno si risolve al secondo 30, non al 31

Scenario: l'harness non vede il countdown
  Given uno scenario headless che chiama LockInAndResolve()
  Then la risoluzione avviene nello stesso frame
    And il TurnLog è identico a quello prodotto prima di questa feature

Scenario: l'autobattle non arma nulla
  Given una partita in autobattle senza umani
  Then nessun countdown viene armato
    And il ritmo dei turni è quello di PlanningSeconds
```

L'ultima riga del quarto scenario è la più importante del referto: **è la prova che questa feature non tocca
il determinismo**, e va asserita, non promessa.

---

## 4. Cosa il panel lascia esplicitamente fuori

- ⛔ **La forma multi-giocatore** («tutti Ready»): serve dalla M10, e con un umano solo non è osservabile.
  L'armamento resta una funzione sola, così che aggiungere il quorum dopo non richieda di spostare il
  countdown.
- ⛔ **La UI del countdown**: il numero a schermo è HUD (`E11` #25). Questa issue espone lo stato e i secondi
  rimanenti; disegnarli è di chi possiede l'HUD.
- ⛔ **Il Ready come stato replicato**: privacy e replica sono di `CR-NET` (#773). In offline non c'è nessuno
  a cui replicare, e introdurre ora un campo replicato sarebbe scope di una release che non è questa.
