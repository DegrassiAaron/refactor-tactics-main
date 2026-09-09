# #2741 — Il rifiuto di bersaglio che arriva al giocatore · panel di specifica · 2026-09-09

> **Comando**: `/issue-run 2741` → `sc:spec-panel 2741`, modalità **critique**, focus `requirements · compliance`.
> **Panel**: Wiegers (lead) · Nygard · Hightower · Adzic · Cockburn.
> **Base**: `main = c3151afd`, in worktree isolato `rt-wt-2741`.
> ⚠️ Ogni affermazione qui viene dal sorgente, non dal corpo della issue.

## §1 — 🔴 Il rilievo che cambia la specifica: la feature, fatta ingenuamente, **converte un leak latente in un leak reale**

Tre fatti misurati, che presi insieme decidono la forma della feature:

| Fatto | Dove |
|---|---|
| il click traccia sul canale **`ECC_Visibility`** | `RTPlayerController.cpp:1161` — `GetHitResultUnderCursor(ECC_Visibility, …)` |
| la mesh di un'unità **blocca** quel canale | `RTUnit.cpp:62-63` — `SetCollisionEnabled(QueryOnly)` + `SetCollisionResponseToAllChannels(ECR_Block)` |
| il velo spegne la **visibilità**, non la **collisione** | `RefreshComponentVisibility` — **zero** occorrenze di `Collision` nel corpo |

∴ **un nemico velato resta cliccabile**: il suo collider risponde al trace anche quando la mesh non si disegna. E il ramo di targeting non filtra per conoscenza — nessun `bKnownToObserver` fra il `Cast<ARTUnit>` e `ClassifyHexTargeting`.

🔑 **L'unica ragione per cui oggi questo non è un leak è che il rifiuto è muto.** `UE_LOG` finisce in un canale che in partita nessuno apre.

⛔ **Rendere visibile il rifiuto — che è precisamente ciò che questa issue chiede — rende visibile anche questo.** Un giocatore che clicca su una cella apparentemente vuota e riceve *«nessuna linea di tiro»* ha appena appreso che **lì c'è qualcuno**, e `D-225` esiste per impedirlo.

## §2 — Il panel

### 🔴 WIEGERS — il requisito ha **due** esiti e ne servono **tre**

*«`OutOfRange` e `NoLineOfSight` restano distinti»* è corretto e insufficiente: `ClassifyHexTargeting` ne produce **quattro** (`Ok`, `OutOfRange`, `NoLineOfSight`, `NoMap`), e nessuno di essi copre il caso che conta — **il bersaglio che l'osservatore non conosce**.

📝 **Raccomandazione**: la specifica dichiara **tre** esiti visibili, e il terzo è definito da ciò che **non** deve distinguere:

| Caso | Cosa vede il giocatore |
|---|---|
| bersaglio noto, coperto | il motivo: **copertura**, con la cella se è un `CellBlocker` |
| bersaglio noto, fuori portata | il motivo: **portata**, con la distanza |
| **bersaglio non conosciuto** | **esattamente ciò che vedrebbe cliccando una cella vuota** |

🎯 Priorità: **massima** — senza il terzo la feature è un rilevatore di presenze.

### 🔴 NYGARD — il modo di fallimento non è il crash, è il silenzio che parla

Il difetto qui non si manifesta come errore: si manifesta come **un'informazione corretta consegnata a chi non doveva riceverla**. Non c'è log che lo segnali, nessun test lo vede, e chi lo introduce sta implementando fedelmente la issue.

📝 **Raccomandazione**: il canary va scritto **prima** dell'implementazione, e deve pinnare l'**indistinguibilità** — non l'assenza. Un test che verifichi *«il rifiuto non nomina il nemico»* passerebbe anche se il rifiuto dicesse *«qui non puoi tirare»* su una cella vuota e *«nessuna linea di tiro»* su una occupata: due messaggi diversi sono già un canale.

### 🟡 HIGHTOWER — due canali, due pubblici, e non vanno fusi

Il `UE_LOG` attuale **non va rimosso**: è diagnostica per chi sviluppa, e dice più di quanto il giocatore debba sapere — inclusi i casi che al giocatore vanno nascosti.

📝 **Raccomandazione**: il canale del giocatore è **derivato e più povero**. Il log resta come è; ciò che nasce è una seconda uscita, filtrata. ⚠️ Se il messaggio del giocatore si ottiene formattando la stessa stringa del log, il filtro non c'è.

### 🟡 ADZIC — gli esempi, e il terzo è quello che oggi nessuno scriverebbe

```gherkin
Dato   Gadget selezionata, e un nemico NOTO dietro il muro, a portata
Quando clicco sul nemico
Allora il rifiuto dice «copertura» e indica la cella che interrompe la linea

Dato   un nemico NOTO, in vista, oltre la portata
Quando clicco sul nemico
Allora il rifiuto dice «portata», e NON parla di copertura

Dato   un nemico che la mia squadra NON osserva, su una cella qualsiasi
Quando clicco su quella cella
Allora ricevo esattamente ciò che riceverei se la cella fosse VUOTA
  E    nessun messaggio, nessun suono e nessun ritardo la distinguono   ← il canary
```

### 🟢 COCKBURN — l'attore e il suo goal, che non è «sapere perché»

Il giocatore in pianificazione vuole **decidere il prossimo click**, non ricevere una diagnosi. ∴ il rifiuto utile risponde a *«cosa devo cambiare?»* — avvicinarmi, spostarmi di lato — più che a *«qual è la classificazione interna?»*.

📝 Ricaduta pratica: *«copertura»* e *«portata»* restano distinti **perché suggeriscono azioni diverse**, non per fedeltà all'enum.

## §3 — Consenso

1. **Tre esiti, non due.** Il terzo — bersaglio non conosciuto — è il requisito di sicurezza, e va scritto come **indistinguibilità**, non come omissione.
2. **Il canary precede l'implementazione**, e pinna che due stati nascosti diversi producano lo **stesso** output osservabile.
3. **Il log diagnostico resta**; il canale del giocatore è più povero e nasce filtrato, non formattato dallo stesso testo.
4. **La forma è una funzione pura**, sul modello di `ComputeBlockerMarks`/`ComputePlannedHitMarks`: testabile senza viewport, ed è il produttore che #2742 riuserà quando #1941 chiuderà.

## §4 — Un difetto latente da registrare a parte

⚠️ **Il collider di un'unità velata resta attivo e cliccabile.** Non lo introduce questa issue e non è suo compito ripararlo, ma va **registrato**: qualunque futura risposta al click — un suono, un cursore che cambia, un pannello — eredita lo stesso rischio, e nessun test oggi lo vede.

➡️ Follow-up proprio, non assorbito qui.
