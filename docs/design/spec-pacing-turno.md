# Spec — Pacing del turno: misurare prima di tarare

> **Stato**: design approvato in sessione, **da implementare** · **Data**: 2026-08-06
> **Ambito**: tempo reale di un turno (pianificazione + risoluzione), **non** il numero di turni per partita.
> **Decisione abilitante di questa spec**: le reazioni **restano pre-committed** — nessuna finestra
> interattiva nel resolver, CP 5.1 (`#50`) e l'invariante #3 restano invariati.
>
> ⚠️ **Aggiornamenti successivi, che non invalidano il metodo**:
> **(a)** [ADR-0004](adr-0004-finestre-di-reazione.md) (2026-08-07) reintroduce finestre interattive come
> *decision boundary* — la D1 qui sotto resta la premessa storica di questa spec, non lo stato del canone;
> il metodo di misura è invariato, e la Fast Reaction va misurata **a parte** (è Decision Time, non planning).
> **(b)** [`spec-durata-partita-e-scala-mappe.md`](spec-durata-partita-e-scala-mappe.md) fissa le **bande
> obiettivo** per formato: `PlanningMax` **40–45 s** in 3v3 Standard, **30 s** nel 2v2 corrente. Questa spec
> resta il modo in cui quei numeri si **verificano** invece di essere scelti.

## 1. Il problema

`PlanningSeconds = 30` (`Turn/RTTurnManager.h:200`) è un numero **scelto, mai misurato**. Lo stesso vale per
`MaxPlaybackSeconds = 12` (`:114`), `PhaseBeatSeconds = 0.30` (`:106`) e `AttackShowSeconds = 0.50` (`:110`).
È esattamente la categoria di «target mitico» che CP 3.3 (`#41`) chiede di sostituire con numeri reali — solo
che #41 misura la **macchina** (FPS, path, preview, resolver) e nessuno misura il **giocatore**.

Il numero regge oggi perché le scelte sono poche: 8 azioni, budget di movimento a celle fisse, nessuno slot
reazione. Tre epic in coda allargano lo spazio di decisione senza toccare il timer:

| Epic | Cosa aggiunge alla decisione |
|---|---|
| **E4** | catalogo da 8 a **~35 azioni**, budget 5 MP con costo per cella (1/2/2) |
| **E5** | uno **slot reazione** da dichiarare in planning, per ogni eroe |
| **E6** | **4 eroi** con statistiche e abilità proprie al posto di 2 archetipi |

Con le reazioni pre-committed il costo delle «finestre di risposta» non sparisce: **si sposta nella
pianificazione**. La reazione si dichiara prima, dentro gli stessi 30 s.

## 2. Vincolo di accettazione

> Il timer di pianificazione non deve **mai tagliare** una decisione in corso, né **scadere a vuoto** su un
> giocatore che ha già deciso.

Il secondo caso è già in parte coperto: Spazio fa lock-in immediato (`Player/RTPlayerController.cpp:476`) e,
durante la risoluzione, salta il playback (`:472`). Resta scoperto il taglio — e resta scoperto il fatto che
**nessuna delle due condizioni è oggi osservabile**.

## 3. Decisioni

| # | Decisione | Perché |
|---|---|---|
| **D1** | Le reazioni **restano pre-committed**; nessuna finestra interattiva nel resolver | CP 5.1 (`#50`) lo richiede esplicitamente («senza attese nel resolver», test `Reactions.NoResolverWait`); l'invariante #3 vieta `Delay`/timeline nel resolver; `spec-anima-risoluzione.md:14` mette stack e Reaction Points fuori MVP salvo ADR dedicato |
| **D2** | `PlanningSeconds` resta una **costante**, ma tarata su **dato misurato** | Un timer adattivo per squadra sarebbe asimmetrico: in PvP (M10) la sua durata diventerebbe un canale che rivela lo stato avversario, contro l'invariante #6 |
| **D3** | La telemetria vive in un **canale separato dal TurnLog** | `FRTTurnLogEntry` è composto di soli interi e `HashTurnLog` è un FNV-1a su quei campi con round-trip garantito (`Turn/RTTurnLogLibrary.h:38-57`): è la base del KPI di replay divergence. Un tempo di parete lì dentro lo renderebbe non deterministico |
| **D4** | Il dato è tutto **intero, in millisecondi** | La formattazione di un `float` in CSV con locale italiano produce la virgola decimale, che collide col separatore; e gli interi rendono i test esatti invece che a tolleranza |
| **D5** | Il campione registra **`MsSinceLastInput` al lock-in** | Senza, «taglio» e «attesa a vuoto» sono indistinguibili pur avendo cure opposte (§7) |
| **D6** | Questa fetta **non cambia** `PlanningSeconds` | Consegnare la sonda e il valore nuovo insieme renderebbe impossibile dire se il turno è migliorato o se è solo cambiato il metro. Stesso ragionamento di D7 nella spec E4 |

## 4. Architettura e confini

Tre pezzi, una sola direzione di dipendenza, un solo punto impuro. Segue il pattern già in uso nel repo —
`struct` puri + librerie statiche pure (`URTCombatLibrary`, `URTTurnLogLibrary`, `URTPlaybackLibrary`) + un
Actor che le orchestra. **Nessun `Subsystem`, nessun `ActorComponent`**: il progetto non ne usa, e
introdurne uno per un solo caso d'uso creerebbe un paradigma isolato.

| Pezzo | File | Natura | Responsabilità |
|---|---|---|---|
| `FRTPacingSample` | `Turn/RTPacing.h` | struct puro | Un turno misurato. Nessun metodo, nessuna dipendenza |
| `URTPacingLibrary` | `Turn/RTPacingLibrary.h/.cpp` | statica pura | Percentili, conteggi, riga CSV. Testabile headless |
| `ARTTurnManager` | esistente | impuro | Cronometra, accumula, appende il file. Non calcola niente |

Sta in `Turn/` e non in una cartella nuova: è pacing del turno, e il codice è raggruppato per dominio
(`Ability/ Combat/ Map/ Player/ Turn/ UI/ Unit/`).

> **Invariante di questo canale**: la sonda **non ha ritorno verso il gameplay**. Nessun campo di pacing entra
> nel TurnLog, nel suo hash o in una decisione. È l'unica ragione per cui questo dato può permettersi di non
> essere deterministico.

## 5. Il campione

```
FRTPacingSample                     // un turno
├─ int32 TurnNumber                 // contesto
├─ int32 UnitsAliveTeam0            // contesto
├─ int32 UnitsAliveTeam1            // contesto
├─ int32 ActionsAvailable           // contesto: azioni selezionabili non in cooldown → cresce con E4/E6
├─ int32 MsToFirstInput             // composizione: quanto prima di toccare qualcosa
├─ int32 SelectionCount             // composizione: unità selezionate
├─ int32 OrderCount                 // composizione: ordini impartiti
├─ int32 UndoCount                  // composizione: waypoint annullati
├─ int32 MsToLockIn                 // base
├─ int32 MsSinceLastInput           // al momento del lock-in — il campo che rende il vincolo misurabile
├─ ERTLockInSource LockInSource     // Input | Timeout
├─ int32 MsPlayback                 // risoluzione: durata effettiva
└─ bool  bPlaybackSkipped           // risoluzione: Spazio premuto
```

Definizioni che non devono restare a interpretazione:

| Campo | Definizione esatta |
|---|---|
| `ActionsAvailable` | Somma delle azioni selezionabili dalle unità **vive della squadra del giocatore**, escluse quelle in cooldown |
| `SelectionCount` | Quante volte il giocatore ha selezionato un'unità nel turno |
| `OrderCount` | Quanti ordini (abilità o destinazione) ha impartito nel turno |
| `MsToFirstInput` | Se il turno si chiude **senza alcun input**, vale `MsToLockIn` |
| `MsSinceLastInput` | Se non c'è stato **nessun** input, vale `MsToLockIn` — quindi un turno passato inerte finisce fra gli `IdleTimeouts`, che è la classificazione corretta |

### Perché `MsSinceLastInput` porta il peso

Due timeout molto diversi si assomigliano, se si contano soltanto:

| Situazione | `LockInSource` | `MsSinceLastInput` | Cos'è davvero |
|---|---|---|---|
| Stavi ancora piazzando waypoint quando è scaduto | `Timeout` | basso (< soglia) | **Taglio vero** → il timer è corto |
| Avevi finito, non hai premuto Spazio | `Timeout` | alto (≥ soglia) | **Attesa a vuoto** → timer lungo, *oppure* lock-in poco scopribile |
| Hai premuto Spazio | `Input` | — | Il caso sano |

Le due patologie hanno **cure opposte** — allungare o accorciare. Senza questo campo la misura non sa dire
quale delle due sta guardando.

## 6. Gli agganci

Sei punti, nessuno dei quali contiene logica.

| # | Punto | Cosa fa |
|---|---|---|
| 1 | `StartPlanningTimer()` (`RTTurnManager.cpp:281`) | Apre il campione: azzera i contatori, timestamp d'inizio, cattura il contesto |
| 2 | `RecordPlanningInput(Kind)` *(nuovo)* | Ingresso unico per gli input: fissa `MsToFirstInput` la prima volta, incrementa il contatore, aggiorna l'ultimo timestamp |
| 3 | `OnPlanningTimeout()` (`:299`) | Marca `LockInSource = Timeout` prima del lock-in; il default resta `Input` |
| 4 | `LockInAndResolve()` (`:315`) | Chiude i tempi di pianificazione: `MsToLockIn`, `MsSinceLastInput` |
| 5 | `SkipPlayback()` (`:1211`) | Alza `bPlaybackSkipped` |
| 6 | `ConcludeTurn()` (`:394`) | Chiude il campione: `MsPlayback = PlaybackElapsedTotal`, accumula, appende la riga |

Lato `ARTPlayerController`, l'aggancio #2 si chiama da `OnSelect`, `OnAbility1..4` e `OnUndoWaypoint` — punti
che esistono già (`RTPlayerController.cpp:189-196`). **Il controller non cronometra**: passa solo il tipo di
input. Tutto il tempo vive in un posto solo.

**Perché `ConcludeTurn()` e non `FinishPlayback()`**: da `ConcludeTurn` passano entrambi i rami, quello col
playback (`:1208`) e quello senza (`:390`, headless o turno senza eventi). Agganciarsi a `FinishPlayback`
perderebbe **silenziosamente** ogni turno senza playback — cioè tutte le partite automatiche, cioè proprio
quelle che si giocano in massa per raccogliere campioni.

**Orologio**: `FPlatformTime::Seconds()`, non `World->GetTimeSeconds()`. Il secondo risente di time dilation
e pausa; qui si misura un essere umano che pensa, quindi serve il tempo di parete.

**Accensione**: `bRecordPacing` (`EditAnywhere`, default `false`) governa **solo la scrittura su file**.
L'accumulo in memoria è sempre attivo — costa circa 1,3 KB per partita — così `rt.Debug.Pacing` funziona
senza preparativi. Il file è `Saved/RT/pacing_<AAAAMMGG-HHMMSS>.csv`, **una riga appesa per turno**: un
restart con `R` non perde niente. `Saved/` è già fuori dal versionamento.

**Lettura in gioco**: `rt.Debug.Pacing` stampa il sommario (§7) della sessione corrente. Il prefisso
`rt.Debug.*` anticipa il namespace di CP 11.4 (`#80`): il comando va aggiunto a quell'elenco quando la issue
verrà lavorata, non duplicato.

## 7. La libreria e la regola di taratura

```
FRTPacingSummary  ←  URTPacingLibrary::SummarizeSamples(Samples, CutoffWindowMs)
├─ int32 SampleCount
├─ int32 MedianMsToLockIn
├─ int32 P90MsToLockIn
├─ int32 TrueCutoffs        // Timeout && MsSinceLastInput <  CutoffWindowMs
├─ int32 IdleTimeouts       // Timeout && MsSinceLastInput >= CutoffWindowMs
├─ int32 SkippedPlaybacks
└─ int32 MedianMsPlayback
```

Percentile con **nearest-rank su interi ordinati**: nessuna interpolazione, risultato esatto, test scrivibile
a mano su un campione di sette elementi. `CutoffWindowMs` è un **parametro della funzione** — proposta:
**3000** — non una costante sepolta: la soglia che separa taglio da attesa resta una decisione visibile.

### Regola di lettura

Si legge in quest'ordine:

1. **`TrueCutoffs > 0`** → il timer taglia davvero. **Alza** `PlanningSeconds`.
2. **`TrueCutoffs == 0`, `IdleTimeouts` alti** → non è il timer: il giocatore non usa il lock-in manuale. È un
   problema di interfaccia. **Allungare qui peggiora**, perché aggiunge attesa a un'attesa già inutile.
3. **`TrueCutoffs == 0`, `IdleTimeouts ≈ 0`** → il timer non è mai il vincolo. Si può **abbassare** fino a
   `P90 + margine`, e il turno si accorcia senza costi.

Valore proposto: `PlanningSeconds = ceil((P90 + 5 s) / 5 s) * 5 s`. Il margine di **5 s** è la prima proposta,
e va scritto nella riga KPI insieme al risultato: serve a coprire la coda oltre il p90 senza inseguire il
massimo, che un singolo momento di distrazione renderebbe inutilizzabile. L'arrotondamento a 5 s è voluto — è
un parametro di design che qualcuno deve leggere e giustificare, non una cifra ottimizzata al decimo.

### Quando ri-misurare

Obbligatorio alla chiusura di **E4**, **E5**, **E6** (§1): sono le epic che allargano lo spazio di decisione,
e un `PlanningSeconds` tarato prima di E4 non dice nulla su dopo. Registrazione nella tabella KPI di
[`v0.1-definition-of-done.md`](v0.1-definition-of-done.md) §4 con data, metodo, numero di campioni e la
riserva sul campione (§12).

## 8. Test

| Test | Cosa protegge |
|---|---|
| `RefactorTactics.Pacing.PercentileNearestRank` | p50/p90 su sette valori scritti a mano: nearest-rank, nessuna interpolazione |
| `RefactorTactics.Pacing.SummaryOfEmptySampleIsZero` | Fail-closed su array vuoto: nessuna divisione per zero, nessun indice fuori range |
| `RefactorTactics.Pacing.CutoffVsIdleTimeout` | La soglia separa i due casi, **confine incluso**: `MsSinceLastInput == CutoffWindowMs` è attesa, non taglio |
| `RefactorTactics.Pacing.CsvRowMatchesHeader` | La riga ha le colonne dell'header, tutti interi, nessuna virgola da locale |
| `RefactorTactics.Pacing.EveryTurnProducesOneSample` | Su una partita headless completa, campioni == turni giocati |
| `RefactorTactics.Pacing.DoesNotAffectTurnLogHash` | Stessa partita con sonda accesa e spenta → `HashTurnLog` identico |

Gli ultimi due non sono di routine. `EveryTurnProducesOneSample` cattura **la trappola che il design evita**
(§6): l'aggancio sbagliato funzionerebbe in PIE e perderebbe le partite automatiche, e il bug resterebbe
invisibile fino al momento di leggere i numeri. Si appoggia all'infrastruttura di
`RefactorTactics.HexMatch.PlaysToCompletion`, che esiste già. `DoesNotAffectTurnLogHash` è la dimostrazione
eseguibile dell'invariante di §4: oggi passa quasi per costruzione, il suo valore è nel futuro, quando
qualcuno sarà tentato di far leggere un tempo a una decisione.

## 9. Definition of Done

- [ ] Compila su **entrambi i target**, Editor e Development
- [ ] La suite esistente resta verde **senza modifiche ai test**; +6 nuovi
- [ ] `HashTurnLog` invariato con sonda accesa; nessun campo di pacing in `FRTTurnLogEntry` né nella serializzazione
- [ ] Il CSV finisce in `Saved/RT/` e **non compare in `git status`**
- [ ] `rt.Debug.Pacing` stampa il sommario della sessione in PIE
- [ ] Riga KPI **predisposta** in `v0.1-definition-of-done.md` §4 con metodo e riserva sul campione — i numeri restano vuoti finché non c'è il playtest
- [ ] Limiti di §12 riportati nel documento KPI accanto al numero, non solo qui

## 10. Cosa questa fetta non fa

- **Non cambia `PlanningSeconds`** (D6): il valore arriva dal playtest, non dall'implementazione.
- **Non tocca le reazioni**: restano pre-committed, CP 5.1 e invariante #3 invariati (D1).
- **Non aggrega più sessioni da codice**: nessun parser CSV: un foglio di calcolo lo fa, e oggi sarebbe codice
  non richiesto.
- **Non affronta `#96`**: quello è il **numero** di round (misurato a 10 dopo la scadenza dello scudo nel
  Cleanup; era 25 prima), questo è il **tempo** di un turno. Confonderli farebbe tarare il pacing su una
  partita che è lunga per un altro motivo. Il numero di round e la durata della partita vivono in
  [`spec-durata-partita-e-scala-mappe.md`](spec-durata-partita-e-scala-mappe.md).
- **Non misura la durata della partita né la Fast Reaction**: `MatchDurationSeconds`, `ReadyAtSeconds`,
  `ReactionDecisionSeconds` e le altre metriche di formato sono elencate in quella spec (§17) e useranno
  **questo stesso canale** — separato dal TurnLog, interi in millisecondi, nessun ritorno verso il gameplay.
- **Non introduce finestre di risposta durante la risoluzione**: sarebbe un ADR che revisiona CP 5.1 e
  l'invariante #3, e non è ciò che questa sessione ha deciso.

## 11. Collocazione

**Nuova issue, non un'estensione di `#41`.** Tre ragioni:

1. La DoD di `#41` elenca FPS, path mediana, preview e tempo del resolver — misure di **macchina**; questa è
   telemetria di **comportamento umano**.
2. `#41` dipendeva da `#40`, che è chiusa: **può chiudersi subito**. Agganciarci il pacing lo bloccherebbe in
   attesa di una sessione di gioco che non è ancora stata fatta.
3. La ri-misura va ripetuta a ogni epic (§7), mentre `#41` si chiude una volta sola.

**Relazione con E4**: nessuna dipendenza di codice, ma E4 è il primo evento che **invalida** la taratura.
Conviene quindi che la sonda esista **prima** che E4 chiuda, per avere un prima e un dopo confrontabili.

## 12. Limiti dichiarati

- **Il campione è un solo giocatore, che è l'autore del gioco.** Lo scope corrente è 2v2 offline contro bot:
  c'è un umano solo, e conosce il gioco meglio di chiunque. Un p90 misurato così **sottostima** il tempo di
  un giocatore nuovo. Il numero va pubblicato con questa riserva accanto, non arrotondato via — è ciò che
  chiede la DoD di `#41` («un valore fuori target va registrato come tale, non nascosto né arrotondato»).
- **La pausa del PIE gonfia il campione del turno in corso**, perché l'orologio è il tempo di parete (§6). Un
  campione così va scartato a mano.
- **Nessuna aggregazione fra sessioni da codice** (§10): il sommario copre la sessione corrente.
- `ActionsAvailable` misura le azioni *selezionabili*, non le combinazioni possibili: è un indicatore della
  crescita del catalogo, non una misura della complessità reale della decisione.

## 13. Riferimenti

- Timer e playback: `Source/RefactorTactics/Turn/RTTurnManager.h:106-114, 200`, `RTTurnManager.cpp:281-320, 394, 1170-1219`
- Lock-in e skip da input: `Source/RefactorTactics/Player/RTPlayerController.cpp:189-196, 469-477`
- Hash e serializzazione del TurnLog: `Source/RefactorTactics/Turn/RTTurnLogLibrary.h:38-67`
- Reazioni pre-committed: issue `#50` (CP 5.1), DoD «senza attese nel resolver»
- KPI di macchina: issue `#41` (CP 3.3) · tabella in [`v0.1-definition-of-done.md`](v0.1-definition-of-done.md) §4
- Durata della partita in round (problema distinto): issue `#96` · [`spec-durata-partita-e-scala-mappe.md`](spec-durata-partita-e-scala-mappe.md)
- Pacing del playback e conflitto di fonte sul PDF: [`spec-anima-risoluzione.md`](spec-anima-risoluzione.md), righe 10-15 (stack e Reaction Points fuori MVP) e 117-119 (perché i 45-60 s del PDF non si applicano qui)
- Parametri north-star delle finestre di reazione: [`spec-sequenza-turno.md`](spec-sequenza-turno.md), riga «Budget UX» (2 RP/squadra, 2-3 finestre, timer 3 s)
- Catalogo che allarga la decisione: [`spec-motore-azioni-e4.md`](spec-motore-azioni-e4.md), [`adr-0003-modello-azioni-v01.md`](adr-0003-modello-azioni-v01.md)
