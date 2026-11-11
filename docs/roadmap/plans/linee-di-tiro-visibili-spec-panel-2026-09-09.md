# Visualizzare le linee di tiro, e quelle bloccate — panel di specifica · 2026-09-09

> **Richiesta d'autore**: *«abbiamo bisogno di visualizzare le linee di tiro e quelle bloccate»*.
> **Comando**: `/sc:spec-panel`, modalità **discussion**, focus `requirements · architecture · compliance`.
> **Panel**: Cockburn (attore e goal) · Wiegers (requisiti misurabili) · Adzic (esempi eseguibili) · Fowler (dove vive il confine) · Nygard (modi di fallimento e costo) · Crispin (testabilità).
> **Base misurata**: `main = b1178002`. Nessuna riga di questo referto viene dalla richiesta: tutte dal sorgente.

## §1 — Cosa esiste già, prima di proporre qualsiasi cosa

`SEARCH → REUSE → EXTEND → CREATE` (`CLAUDE.md` §4). La ricerca dà molto, ed è la ragione per cui questo non è un foglio bianco.

| Anello | Dove | Stato |
|---|---|---|
| **Il produttore della LOS, uno solo** | `URTHexVisionLibrary::HasLineOfSight` · `DescribeLineOfSight` (`Map/RTHexVisionLibrary.h:126,151`) | ✅ esiste, con spec owner |
| **Il risultato è già ricco** | `FRTLineOfSightResult`: `Block`, `BlockedAt`, `BlockedFrom`, `StepIndex` | ✅ dice **dove** e **di che tipo** |
| **Il classificatore del bersaglio** | `URTCombatLibrary::ClassifyHexTargeting` → `Ok` · `OutOfRange` · `NoLineOfSight` · `NoMap` | ✅ **distingue già portata da copertura** |
| **L'ispezione da console** | `rt.Debug.Los <fromQ> <fromR> <toQ> <toR>` (`Map/RTHexLosConsole.cpp`) | ✅ ma **stampa, non disegna** — per scelta dichiarata |
| **Il modello degli overlay** | **#1941** OVL-01, e la famiglia #1942 · #1943 · #1944 | 🔴 **aperta** |
| **La resa attuale in pianificazione** | `DrawPlanningPreview`: cinque significati, cinque `FColor` **letterali** | ⚠️ senza modello |

🔑 **E la scelta di non disegnare è motivata per iscritto**, in `RTHexLosConsole.cpp`:

> *«Disegnare avrebbe voluto dire un **sesto** blocco dentro `DrawPlanningPreview`, dove cinque significati convivono già ciascuno col proprio `FColor` letterale. Il modello che dica cosa significa un'area, chi l'ha prodotta e quanto è certa è il lavoro di #1941 […] Un sesto colore letterale avrebbe chiuso questa issue **allargando** quel difetto.»*

⚠️ **E la collisione di palette è già nota**: la spec v0.2 assegna il **ciano-blu** a Vision/LOS, ma in questo repository il ciano `FColor(40, 220, 220)` è **già** la traccia del percorso. È una delle collisioni che #1941 deve portare a un `D-nnn`.

## §2 — Il fatto che rende la richiesta urgente, e che nessuna issue registra

Misurato oggi durante l'istruttoria di `PIE-HEXPLAY-6`:

**Quando il giocatore clicca su un nemico dietro un muro, non succede assolutamente nulla di visibile.**

```cpp
// ARTPlayerController::HandleClickOnCell
const ERTHexTargetReason Reason = URTCombatLibrary::ClassifyHexTargeting(...);
if (bReady && Reason == ERTHexTargetReason::Ok) { /* pianifica */ }
else switch (Reason) {
    case ERTHexTargetReason::OutOfRange:    UE_LOG(...);   // ← finisce nell'Output Log
    case ERTHexTargetReason::NoLineOfSight: UE_LOG(...);   // ← e basta
}
```

∴ il gioco **sa già** perché il bersaglio non è ingaggiabile, lo **distingue** in due categorie diverse, e lo scrive in un canale che in partita nessuno apre.

🔴 **Non è un buco nel modello: è un'informazione prodotta e buttata.**

## §3 — Il panel

### COCKBURN — «chi guarda, e quando?» La frase ne contiene **due** richieste, con attori diversi

*«Visualizzare le linee di tiro»* e *«quelle bloccate»* non sono lo stesso bisogno:

| | Attore | Momento | Domanda |
|---|---|---|---|
| **(A)** | il giocatore che **pianifica** | prima del lock-in | *«chi posso colpire da qui?»* |
| **(B)** | il giocatore che **ha appena cliccato** | nell'istante del rifiuto | *«perché non questo?»* |
| **(C)** | designer / dev | in editor | *«perché questa linea passa e quella no?»* |

⚠️ **(C) esiste già** (`rt.Debug.Los`) ed è testuale per scelta. Confonderla con (A) e (B) porta a rifare uno strumento invece di consegnarne uno nuovo.

🔑 **(B) costa pochissimo e vale subito**: l'informazione è già calcolata, già classificata, e oggi va nel log. **(A) è un overlay**, e un overlay senza il modello di #1941 aggrave un difetto noto.

### WIEGERS — *«visualizzare»* non è un requisito finché non dice **quante linee, per chi, e quando spariscono**

Tre domande che la richiesta non risponde e che decidono l'intera forma:

1. **Cardinalità**: una linea per il bersaglio **sotto il cursore**, oppure tutte le linee da un'unità a tutti i nemici? La seconda è O(unità × nemici) di geometria per frame, e su una board a 64 celle con 4 unità significa fino a 16 linee simultanee;
2. **Durata**: la linea vive quanto l'hover, quanto la selezione, o fino al lock-in? `OnLockInCommitted` esiste già ed è il segnale che spegne le anteprime di pianificazione;
3. **Il caso «bloccata»**: si mostra la linea **fino al punto di blocco** o l'intera traiettoria con l'ostacolo marcato? `FRTLineOfSightResult` porta `BlockedAt` **e** `StepIndex`, quindi entrambe sono costruibili senza nuovo calcolo.

### ADZIC — l'esempio che rende il requisito verificabile

```gherkin
Dato   Gadget selezionata, e Branth dietro la barriera centrale
Quando il cursore passa su Branth
Allora la linea appare fino alla cella che la interrompe
  E    quella cella è marcata come causa
  E    il motivo è distinto da «fuori portata»

Quando il cursore passa su un nemico in vista e a portata
Allora la linea appare intera, in uno stato visivamente diverso

Quando il cursore passa su un nemico che la mia squadra NON osserva
Allora non appare nessuna linea, e nessun segno   ← il caso che protegge D-225
```

### NYGARD — i due modi di fallimento che questa feature può introdurre

**🔴 Il primo è un leak, e va escluso per costruzione.** Una linea disegnata verso un nemico **non osservato** ne rivelerebbe la posizione: è esattamente ciò che `D-225` vieta (*«mai vista: non si disegna»*). E il rischio non è teorico — il velo è **già** una superficie che ha prodotto una diagnosi falsa in passato.

⛔ La difesa non è un filtro nuovo nel disegno: è che **la sorgente sia già autorizzata**. Il precedente corretto è `ComputeBlockerMarks`, che *non* rifiltra perché il feed arriva filtrato — e il commento accanto dichiara che un secondo contratto di conoscenza sarebbe il difetto.

**⚠️ Il secondo è il costo.** `DescribeLineOfSight` cammina la linea cella per cella. Chiamarla per ogni nemico a ogni frame di hover è un profilo diverso dal chiamarla una volta al click. La cardinalità di Wiegers non è un dettaglio di UX: è la scelta che decide se serve una cache.

### FOWLER — dove vive il confine, e perché **non** in `DrawPlanningPreview`

Il codice ha già la risposta scritta: un sesto `FColor` letterale in `DrawPlanningPreview` chiuderebbe questa richiesta **allargando** il difetto che #1941 esiste per risolvere.

∴ due strade, e non sono equivalenti:

| | Strada | Costo | Rischio |
|---|---|---|---|
| **1** | **(B) subito, come feedback del rifiuto** — l'informazione che `ClassifyHexTargeting` produce raggiunge lo schermo, senza overlay nuovi | basso | quasi nullo: nessun nuovo significato grafico, nessuna nuova sorgente |
| **2** | **(A) come overlay semantico**, dentro il modello di #1941 | alto | **dipende da una issue aperta**; farlo prima significa aggiungere il sesto colore che il repo ha già rifiutato |

### CRISPIN — cosa può cadere in un test, e cosa no

✅ **Testabile headless**: che la linea prodotta per un osservatore **non** contenga celle non autorizzate; che il motivo `NoLineOfSight` sia distinto da `OutOfRange`; che l'ordine sia `StableLess`; che due stati nascosti diversi producano **lo stesso** output (il canary che #2596 già prescrive per la sua query).

⛔ **Non testabile headless**: che si **veda**. Quella è una voce PIE, e va scritta insieme alla feature — non dopo, come è successo per il marcatore d'ostacolo, che è arrivato a schermo tre sedute prima che qualcuno lo giudicasse.

## §4 — Consenso del panel

1. **La richiesta contiene due lavori con costi diversi di un ordine di grandezza.** Separarli è la prima decisione.
2. **(B) — il rifiuto che si spiega — è pronto per essere specificato adesso**: l'informazione esiste, è classificata, ed è buttata in un log. Non richiede il modello degli overlay perché non introduce un significato d'area nuovo.
3. **(A) — le linee di tiro come overlay — non va aperta prima di #1941**, o si paga due volte: una per il sesto colore letterale, una per toglierlo.
4. **Il vincolo di privacy non è una clausola: è la forma della feature.** La sorgente deve essere già autorizzata, e il test che lo dimostra va scritto con la feature.
5. **La collisione di palette è già dichiarata** e appartiene a #1941: chi disegna la LOS prima di quel `D-nnn` sceglie un colore che dovrà cambiare.

## §5 — Cosa raccomanda il panel, in ordine

**Primo — una issue per (B).** *Il rifiuto di bersaglio raggiunge il giocatore*: `OutOfRange` e `NoLineOfSight` diventano un ritorno visibile al click, distinti fra loro, con la cella bloccante indicata quando il blocco è una **cella** (`CellBlocker`) e senza dettaglio quando è un **bordo** — che è la regola che `SightBlockerForLog` già applica.

**Secondo — una issue per (A)**, dichiarata **bloccata da #1941**, che consuma il modello invece di anticiparlo.

**Terzo — la voce PIE si scrive con la feature**, non dopo.

⛔ **Cosa il panel sconsiglia**: aggiungere il disegno della LOS a `DrawPlanningPreview` con un sesto colore letterale. È la via più rapida, ed è quella che il repository ha già valutato e rifiutato per iscritto.
