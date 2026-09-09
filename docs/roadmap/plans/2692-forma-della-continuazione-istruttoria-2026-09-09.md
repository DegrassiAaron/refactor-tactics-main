# Istruttoria — la forma della continuazione (#2692, C5), 2026-09-09

> `ISTRUTTORIA` · **Oggetto**: [#2692](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2692) rilievo C5
> **Decisione da prendere**: quale forma prenda la ripresa del ciclo delle fasi dopo una sospensione nel Blast.
> **Governata da**: [`D-355`](../../decisions/RT_PDR_00_Decision_Log.md), che la dichiara **non decisa qui**.
> **Misure**: su `520352c8`, motore libero. Compile `PASS`, Automation `PASS` (203 + 20 test, 0 fail).
> **Esito**: la scelta **non è simmetrica**. Una delle due forme corregge **tre difetti già in campo**;
> l'altra li estende al Blast. Raccomandazione motivata in coda — la decisione resta d'autore.

---

## 1. Il fatto di partenza

`LockInAndResolve` (`Turn/RTTurnManager.cpp:1760`) risolve il turno in un ciclo solo:

```cpp
do {
    Phase = URTTurnRules::NextPhase(Phase);
    if      (Phase == Prep)  ResolvePrep();
    else if (Phase == Dash)  ResolveDash();
    else if (Phase == Blast) ResolveCombat();
    else if (Phase == Move)  ResolveMovement();
} while (Phase != Planning);

if (IsResolutionSuspended()) { BeginPartialPlayback(); return; }
ConcludeResolution();
```

Sequenza: `Planning → Prep → Dash → Blast → Move → Cleanup → Planning` (`Turn/RTTurnRules.cpp:3-16`).

**Quando il movimento si sospende, il ciclo arriva comunque in fondo.** `ResolveMovement` ritorna
normalmente, `Cleanup` non ha un ramo, `Planning` chiude il `while`. È la scorciatoia che #2679 ha usato:
dopo `Move` nessuna fase fa lavoro, quindi la ripresa non ha bisogno del ciclo —
`ResumeSuspendedResolution` chiama solo `FinishMovementResolution()` e `ConcludeResolution()`.

⛔ **Col Blast quella scorciatoia produce un difetto garantito**: se `ResolveCombat` si sospende, il ciclo
prosegue e chiama `ResolveMovement()`. Le unità si muovono mentre gli spostamenti del Blast sono applicati a
metà e una finestra è aperta sullo schermo.

---

## 2. Lo stato che il gioco espone durante l'attesa — misurato

Questa è la misura che la issue non aveva, ed è ciò che rende la scelta non simmetrica.

Durante una sospensione, **oggi**:

| Variabile | Valore | Perché |
|---|---|---|
| `Phase` | **`Planning`** | il `do…while` è arrivato in fondo |
| `bIsResolving` | **`true`** | `= false` sta a `:7508`, **dopo** il `return` per sospensione di `FinishPlayback` (`:7502`) |

Cioè il gioco dichiara *«siamo in pianificazione»* mentre il turno non è finito, il playback scorre e una
finestra attende una risposta.

### I lettori di `Phase`, e quali sono protetti

Fuori da `Tests/`, i lettori esterni interrogano quasi sempre `Planning` o `MatchEnded`. **Tre non
accoppiano `IsResolving()`, e sbagliano già oggi:**

| Sede | Cosa fa | Effetto durante l'attesa |
|---|---|---|
| `UI/RTHUD.cpp:974` | `switch (Header.Phase)` per il nome della fase | 🔴 stampa **«Pianificazione»** (o «Preparazione»/«Ready») mentre il giocatore ha davanti una finestra col countdown. Il ramo `default:` scriverebbe **«Risoluzione»**, che è la parola giusta |
| `UI/RTHUD.cpp:623` | `GetPhase() == Planning` → disegna la traccia post-lock | 🔴 mostra il percorso dell'ultima risoluzione **sopra** un turno in corso |
| `Turn/RTTurnManager.cpp:7697` | `RecordPlanningInput`: rifiuta se `Phase != Planning` | 🔴 **accetta** gli input del giocatore come input di pianificazione — telemetria di pacing sporca, e il pacing è ciò con cui si tara `D-348` |

Protetti, invece: `RTHUD.cpp:665` (`== Planning && !IsResolving()`), `RTHudViewModel.cpp:48`
(`== Planning && !bResolving`), `RTTurnManager.cpp:1533` e le due guardie di `:1684` / `:1762`.

⚠️ **I tre difetti sono già in campo dal 2026-09-08**, li ha introdotti #2679 e non li vede nessun test:
la suite non ha un caso che interroghi l'HUD a resolution sospesa.

---

## 3. Le due forme, misurate

### (A) `Phase` resta la fase vera, e il ciclo esce presto

Si estrae il `do…while` in una funzione — chiamiamola `RunPhaseLoop()` — che esce **appena**
`IsResolutionSuspended()` diventa vero. `LockInAndResolve` fa il setup e la chiama; la ripresa la chiama
senza setup.

```cpp
void ARTTurnManager::RunPhaseLoop()
{
    do {
        Phase = URTTurnRules::NextPhase(Phase);
        ... risolve la fase ...
        if (IsResolutionSuspended()) { return; }   // ← Phase resta dov'è
    } while (Phase != Planning);
}
```

✅ **Una sola sorgente di verità**: dove sia il turno lo dice `Phase`, come ha sempre fatto.
✅ **I tre difetti si chiudono da soli**, e per costruzione: con `Phase` uguale a `Blast` o `Move`
durante l'attesa, i tre lettori `== Planning` diventano falsi — l'header scrive «Risoluzione» dal ramo
`default`, la traccia post-lock non si disegna, `RecordPlanningInput` rifiuta. **Nessuno dei tre va toccato.**
✅ **Uniforma i due siti**: la stessa uscita anticipata vale per Overwatch e `Brace`.
⚠️ **Il rischio**: `Phase` assume, a ciclo fermo, valori che prima non assumeva mai (`Blast`, `Move`). I
lettori esterni misurati interrogano `Planning` e `MatchEnded`, per cui entrambi restano falsi — che è il
comportamento voluto. Ma è un'affermazione sui lettori **di oggi**.

### (B) `Phase` si comporta come adesso, la posizione vive nel contesto sospeso

Il ciclo arriva in fondo come ora, e un indice di fase nel contesto dice da dove ripartire.

⚠️ **Due sorgenti di verità** su dove sia il turno: `Phase` dice `Planning`, il contesto dice `Blast`.
⛔ **I tre difetti restano, e si estendono**: oggi durano quanto una finestra dell'Overwatch; col `Brace`
durerebbero anche durante il Blast, che è la fase in cui il giocatore guarda di più.
⛔ **Non basta comunque**: il ciclo non può arrivare in fondo dopo una sospensione nel Blast — risolverebbe
`Move` a Blast incompleto. Serve **lo stesso** un'uscita anticipata, quindi (B) paga il costo di (A) e in
più si porta il secondo stato.

---

## 4. La superficie di regressione — misurata, ed è più piccola del temuto

I chiamanti di `LockInAndResolve` fuori dai test sono cinque: i tre timer interni
(`RTTurnManager.cpp:1593` finestra di preparazione, `:1667` ripresa dalla pausa, `:1700` countdown di ready)
e l'harness (`ScenarioHarness/RTScenarioRunner.cpp:105`, `RTScenarioSession.cpp:1540`).

✅ **Nessuno di loro cambia con (A) né con (B)**, purché il **setup resti dentro `LockInAndResolve`** e solo
il ciclo venga estratto. Continuano a chiamare la stessa funzione con la stessa firma.

🔴 **La trappola vera è il setup, ed è comune alle due forme.** Sopra il ciclo vivono `EnsureMatchRoster()`,
la sonda di pacing, `ClearTimer` ×3, `ResolvedTimeline.Reset()`, **`TurnLog.Reset()`** e
`ValidatePlansAtLockIn()`. Rieseguirli alla ripresa **azzera il TurnLog di ciò che è già stato risolto**: il
replay perde il pezzo, e nessun test lo vede perché nessun test sospende il Blast. Qualunque forma si
scelga, il taglio va **sotto** `ValidatePlansAtLockIn()` e **sopra** il `do…while`.

⚠️ E la guardia in testa — `if (Phase != Planning || bIsResolving) return;` — è scritta per respingere un
secondo lock-in. Va **lasciata dov'è**: con il ciclo estratto, la ripresa non passa più da lì.

---

## 5. Cosa resta da decidere anche dopo

`ResumeSuspendedResolution()` (`Turn/RTTurnManager_Movement.cpp:641`) è **cablata sul movimento**:

```cpp
while (Step == Advanced) Step = AdvanceMovementResolution();
if (Step == Suspended) return;
FinishMovementResolution();
ConcludeResolution();
```

Non ha modo di chiedersi *che cosa* sia sospeso. Con (A) diventa: «riprendi ciò che è sospeso, poi
`RunPhaseLoop()`, poi concludi» — e il «ciò che è sospeso» è il punto di estensione che #2692 deve
progettare comunque, indipendentemente dalla forma scelta.

---

## 🧩 Raccomandazione

**(A)**, e non per eleganza: è l'unica delle due che **toglie** stato invece di aggiungerne, e chiude tre
difetti già in campo senza toccare i tre file che li contengono.

L'argomento decisivo non è architetturale, è di misura: **(B) paga tutto il costo di (A)** — l'uscita
anticipata dal ciclo serve comunque, altrimenti `Move` si risolve su un Blast incompleto — **e in più
introduce una seconda sorgente di verità** su dove sia il turno. Non è un compromesso fra due opzioni: è la
stessa opzione con un campo in più e tre difetti conservati.

📝 **Se accettata, va registrata** come decisione nel Decision Log, con i tre difetti nominati: chi la
rileggerà deve trovare scritto che l'header che dice «Pianificazione» durante una finestra di reazione è
stato **chiuso da questa scelta**, non risolto altrove.

⚠️ **Il residuo di rischio, dichiarato**: (A) afferma che nessun lettore esterno si rompe con `Phase` uguale
a `Blast`/`Move` a ciclo fermo. L'affermazione è verificata sui lettori **di oggi** (`grep` su
`ERTMatchPhase::Planning` e `GetPhase()` fuori da `Tests/`). Un test che interroghi l'HUD a resolution
sospesa non esiste, e andrebbe scritto **insieme** all'implementazione — è anche l'unico modo di impedire
che i tre difetti tornino.

---

## Verification

| Gate | Esito |
|---|---|
| Compile | **`PASS`** — `RefactorTacticsEditor Win64 Development` su `520352c8`, 64 s |
| Automation — `Overwatch` · `Reactions` · `Movement` · `Replay` · `Blast` | **`PASS`** — 203 test, 0 fail |
| Automation — `Pacing` | **`PASS`** — 20 test, 0 fail |
| Determinism · Replay · Privacy | **`PASS`** — `Replay.Verifier.ResimulationIsDeterministic` verde nella run sopra |
| PIE · Packaged | **`N/A`** — istruttoria, nessuna modifica al codice |

* Misure di questa istruttoria: `grep` sui lettori di `Phase` e sui chiamanti di `LockInAndResolve` fuori da
  `Tests/`, letture di `RTHUD.cpp`, `RTHudViewModel.cpp`, `RTTurnRules.cpp`, `RTTurnManager.cpp` e
  `RTTurnManager_Movement.cpp` su `520352c8`.
* ⛔ **Nessuna riga di codice è stata scritta**: i tre difetti sono **riportati, non corretti**. Correggerli
  qui significherebbe chiudere in silenzio la porta all'opzione (B) prima che la decisione sia presa.
