# #2755 — Il collider di un'unità velata · panel di specifica · 2026-09-09

> **Comando**: `/issue-run 2755` → `sc:spec-panel`, modalità **critique**, focus `architecture · compliance`.
> **Panel**: Fowler (lead) · Nygard · Hohpe · Crispin.
> **Base**: `main = 583a5d9d`, worktree isolato.
> **Esito**: ⛔ **la premessa della issue è falsa.** Il lavoro non è l'implementazione: è la prova che mancava.

## §1 — La misura che ribalta la issue

La issue #2755 chiedeva di scegliere fra tre vie per un difetto così enunciato:

> *«il collider di un'unità velata resta attivo — il trace del click usa `ECC_Visibility`, la mesh lo blocca (`RTUnit.cpp:62-63`), e `RefreshComponentVisibility` spegne la visibilità ma non la collisione.»*

🔴 **Non è vero.** La citazione punta al **costruttore**. La stessa `RefreshComponentVisibility`, quaranta righe più in basso, chiude con:

```cpp
// La collisione si spegne sull'ACTOR: `SetVisibility` non la tocca, e l'unico proxy di click è `Mesh`.
// Un'unità invisibile ma cliccabile è peggio di una visibile: il giocatore selezionerebbe qualcosa che non vede.
SetActorEnableCollision(bRender);          // RTUnit.cpp:597
```

con `bRender = ShouldBeRendered(IsAlive(), bKnownToObserver)` — cioè `bAlive && bKnownToObserver`.

📊 `git log -L 597,597` data quella riga al **2026-08-27**, commit `678cc8fc`, il cui titolo è già la risposta alla issue: *«un'unità ignota alla squadra non si vede e non si clicca»*.

∴ **La via (a) — «il velo spegne anche la collisione» — non è una scelta da fare: è ciò che il codice fa da tredici giorni.**

### Come è nato l'errore

Lo spec panel di #2741 ha letto `RTUnit.cpp:62-63` — dove la collisione viene *configurata* — e si è fermato lì, senza leggere il punto in cui lo **stato viene applicato**. L'affermazione è poi stata ripetuta tre volte: nel referto di #2741, nel commento del test `RefusalHidesTheUnknownTarget`, e nel corpo di questa issue.

⚠️ Nessuna delle tre ripetizioni ha aggiunto evidenza. Erano **la stessa deduzione citata a se stessa**.

## §2 — Ciò che davvero mancava, e che il panel conferma

### 🔴 CRISPIN — una riga che nessun test tiene ferma

`SetActorEnableCollision` non compare in **nessun** test: `grep` su `Source/RefactorTactics/Tests/` restituisce zero occorrenze di `EnableCollision`, `GetActorEnableCollision` e `LineTrace`.

🔑 **È qui il valore residuo della issue.** Il comportamento è corretto e non è protetto: un refactor che togliesse quella riga — o che riordinasse `RefreshComponentVisibility` — non renderebbe rosso nulla. Il difetto tornerebbe in silenzio, e si scoprirebbe in PIE.

∴ Il deliverable di #2755 non è codice di produzione. È **il test che la issue stessa aveva già scritto nel proprio DoD**, e che resta valido parola per parola.

### 🟡 NYGARD — resta una finestra, ed è quella vera

`bKnownToObserver` nasce **`true`** (`RTUnit.h:1392`) e il velo lo corregge nel `Tick` dell'HUD (`UpdateObserverVeil`).

∴ fra lo spawn di un'unità e il primo tick esiste una finestra in cui la collisione è accesa su un nemico mai visto. È breve e in partita si chiude al frame successivo, ma è **la ragione per cui il collasso a valle di #2741 va tenuto**: non come unica difesa, come difesa in profondità su quella finestra.

⚠️ Il commento del test di #2741 va corretto: dichiara una ragione falsa per una guardia giusta. Una guardia motivata male è una guardia che il prossimo lettore rimuove.

### 🟢 FOWLER — e la (b) e la (c) decadono con la premessa

Non c'è più un difetto da riparare, quindi non c'è da scegliere un canale di trace dedicato (b) né da far filtrare ogni consumatore (c). ✅ La forma attuale è anche quella che il panel avrebbe raccomandato: la collisione è **funzione dello stesso stato** che governa la visibilità, calcolata nell'unico posto che conosce i componenti.

### 🟢 HOHPE — l'hover eredita la riparazione senza saperlo

Il secondo trace su `ECC_Visibility` (`RTPlayerController.cpp:717`, che risolve la cella sotto il cursore) è coperto **dalla stessa riga**: `SetActorEnableCollision` agisce sull'attore, non sul canale, quindi un'unità velata non devia neppure il raggio dell'hover. Nessun lavoro aggiuntivo.

## §3 — Consenso

1. ⛔ **La issue va chiusa come *premessa falsa*, non come *implementata*.** La differenza conta: chiuderla come implementata attribuirebbe a questa run un lavoro fatto il 2026-08-27.
2. ✅ **Il test del DoD si scrive lo stesso**, ed è l'unico artefatto di codice della run.
3. 🔑 **L'oracolo è il trace, non il flag** — asserire `GetActorEnableCollision() == false` proverebbe che il flag è falso; asserire il trace prova che il click non trova l'unità, che è l'affermazione che interessa.
4. ⚠️ **Servono due controlli anti-vacuità**: il nemico **noto** dev'essere colpito (senza, un mondo senza scena fisica renderebbe il test verde per assenza di geometria), e la **propria** unità deve restare cliccabile (senza, spegnere la collisione a tutti passerebbe).
5. 📝 **Le tre ripetizioni della premessa falsa vanno corrette**, e il referto di #2741 va **annotato, non riscritto**: registra ciò che quel panel misurò, e la cronaca dell'errore vale più della sua cancellazione.

## §4 — Raccomandazione

Nessuna modifica a `Source/RefactorTactics/Unit/`. Un test in `RTUnitVisibilityTests.cpp` — dove vive già la fixture del velo — che misura il picking sul percorso reale, più la correzione dei tre punti che affermano il falso.

⛔ **Verifica di mutazione**: sostituire `SetActorEnableCollision(bRender)` con `SetActorEnableCollision(true)` deve far cadere l'asserzione centrale e **solo** quella.
