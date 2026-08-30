# Replay R6, l'interfaccia — spec panel

> `CURRENT` · **Stato**: revisione chiusa, decisioni prese · **Data**: 2026-08-16
> **HEAD della revisione**: `385ae694` · branch `feat/472-replay-r6-interfaccia` · worktree `D:/rt-client`
> **Sorgente revisionata**: il body di [`#472`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/472)
> — cinque criteri di accettazione, tre voci di DoD, tre fuori scope motivati.
> **Scopo**: verificare i criteri contro il codice che #472 dichiara di voler cablare, **prima** di
> scrivere la prima riga. Ogni affermazione di questo referto è misurata su `385ae694`; dove il codice
> smentisce la issue, prevale il codice e il criterio si riscrive.

---

## 1. Il verdetto in una riga

La issue è **precisa su ciò che non farà e vaga su ciò che farà**, e la simmetria non è un difetto di
stile: i tre fuori scope — micro-step, timeline continua, export video — sono tutti e tre argomentati sul
dato («la traccia non porta l'informazione») e **tutti e tre verificati esatti**, mentre i cinque criteri
di accettazione contengono un `sempre` che il codice non può soddisfare, due comandi che non hanno una
funzione dietro, e uno stato di errore su quattro.

L'affermazione che regge la stima — *«questa issue è cablaggio e leggibilità, non logica nuova»* — è
**falsa in due punti misurabili**, ed è il risultato che più cambia il lavoro.

---

## 2. Il conto

| | Voci | Significato |
|---|---:|---|
| 🔴 CRITICO | **2** | il criterio non è soddisfacibile dal codice che dichiara di usare |
| 🟡 MAGGIORE | **2** | il criterio è soddisfacibile, ma non con la premessa che la issue dichiara |
| 🟢 MINORE | **2** | il criterio regge; manca il contesto che lo rende verificabile |
| 🔵 PROCESSO | **1** | una voce di DoD la cui condizione è già scattata |

Nessuna voce `DUPLICATE`: R6 non riscrive niente che esista. La tabella «cosa c'è già sotto» della issue
è **corretta in quattro righe su quattro**, ed è il motivo per cui questa revisione è breve dove poteva
essere lunga.

---

## 3. Lo stato verificato

### 3.1 Le dipendenze sono tutte chiuse

| Issue | Ruolo dichiarato da #472 | Stato |
|---|---|---|
| [#415](https://github.com/DegrassiAaron/refactor-tactics-main/issues/415) | seek per turno e fase | **CLOSED** 2026-08-10 |
| [#416](https://github.com/DegrassiAaron/refactor-tactics-main/issues/416) | la lista delle partite | **CLOSED** 2026-08-10 |
| [#469](https://github.com/DegrassiAaron/refactor-tactics-main/issues/469) | gli archivi | **CLOSED** 2026-08-10 |
| [#470](https://github.com/DegrassiAaron/refactor-tactics-main/issues/470) | gli eventi da mostrare | **CLOSED** 2026-08-10 |

∴ *«Non ha senso iniziarla prima»* è soddisfatto. R6 è sbloccata.

### 3.2 Cosa il Player espone davvero

`Source/RefactorTactics/Replay/RTReplayPlayerLibrary.h` — quattro funzioni:

| Funzione | Cosa fa |
|---|---|
| `OpenArchive` | apre, valida, posiziona il cursore all'inizio; **quattro** esiti |
| `AdvancePhase` | emette le voci della fase corrente e **avanza** il cursore |
| `Rewind` | riporta il cursore all'inizio |
| `SeekToTurn` | riusa il seek di #415 |

**Non esistono**: fase precedente, turno precedente, e nessuna query che risponda «dove sono».

---

## 4. I findings

### 4.1 🔴 «dichiara **sempre** turno e fase correnti» — il cursore non porta quella risposta

Il quantificatore universale non è il problema principale. Il problema è che il posto ovvio dove leggere
la risposta **dà quella sbagliata**.

`RTReplayPlayerLibrary.cpp:61-95`: `AdvancePhase` emette la fase e poi lascia `Cursor.EntryIndex` oltre
le voci appena mostrate; il `while` di testa (`:65-70`) può avere già avanzato `TraceIndex` alla traccia
successiva. Quindi:

```
Traces[Cursor.TraceIndex][0].TurnNumber  ≠  il turno di ciò che si sta guardando
```

Una UI che deduca la posizione dal cursore mostra **il turno successivo** a ogni fine turno. Non è un
caso limite: succede una volta per turno, sempre.

E il `sempre` copre tre stati che il criterio non nomina, tutti raggiungibili:

1. **cursore a fine sequenza** — nessuna voce da cui leggere fase e turno;
2. **traccia vuota** — un turno che non ha prodotto voci; `RTReplaySeekLibrary.cpp:35-37` lo tratta
   esplicitamente come non indirizzabile;
3. **`TurnNumber == 0`** — tracce pre-v6. `SeekToTurn` rifiuta `<= 0` per fail-closed
   (`RTReplaySeekLibrary.cpp:23-30`): l'archivio si apre e **nessun turno è indirizzabile**.

📝 Il criterio diventa: *l'interfaccia dichiara turno e fase correnti in ognuno dei quattro stati — prima
dell'inizio, in riproduzione, a fine sequenza, su traccia non indirizzabile — e il valore mostrato è
quello dell'**ultima fase emessa**, non quello del cursore.*

### 4.2 🔴 Due dei sette valori di `ERTMatchPhase` non sono raggiungibili

`ERTMatchPhase` ha **sette** valori (`Turn/RTTurnRules.h:10-19`). `RTReplaySeekLibrary.h:70-71` dichiara,
misurato: *«nessun punto del resolver emette voci con `Planning` o `MatchEnded`, quindi un seek a quelle
due risponde sempre `PhaseNotFound`»*.

Le fasi osservabili in un replay sono **cinque**: `Prep · Dash · Blast · Move · Cleanup`.

Una UI che disegni i controlli enumerando l'enum produce **due comandi morti**. Non è un difetto
d'implementazione da trovare in review: è la spec che non lo dice.

Manca inoltre del tutto la semantica dei bordi. Quattro casi, nessuno coperto:

```gherkin
Dato   il replay all'inizio del turno 3, fase Prep
Quando il giocatore chiede la fase precedente
Allora ???  — torna a Cleanup del turno 2, oppure non fa nulla?
```

E «turno precedente» **non è `SeekToTurn(N-1)`**: le tracce dichiarano il proprio `TurnNumber` e una
sequenza può iniziare da un turno qualsiasi (`RTReplaySeekLibrary.h:98-100`). Il turno precedente è
quello della **traccia adiacente**, che va letto — non calcolato.

📝 Quattro scenari `Given/When/Then` obbligatori, uno per bordo. Più: *i controlli di fase enumerano le
cinque fasi osservabili, non `ERTMatchPhase`.*

### 4.3 🟡 «cablaggio e leggibilità, non logica nuova» è falso in due punti

**(a) Play/pausa non ha un supporto.** La tabella della issue assegna «interpolazione, durate, ritmo» a
`URTPlaybackLibrary`. Quella libreria è **matematica pura senza stato** (`Turn/RTPlaybackLibrary.h:8-10`):
`DirectionYaw`, `InterpolateAlongPath`, `EstimatePlaybackSeconds`, `SpeedMultiplierForCap`. Non c'è
clock, non c'è `bPlaying`, non c'è avanzamento nel tempo. Play/pausa **è stato nuovo**, e qualcuno deve
possederlo.

**(b) «Dove sono» è una funzione che non esiste**, come da §4.1.

📝 Il confine giusto non è «widget che chiama le librerie»: è un **view model**, lo stesso pattern che il
repository ha già scelto due volte e per la stessa ragione.

| Precedente | Cosa dichiara |
|---|---|
| `FRTMatchHeaderView` (`UI/RTHudViewModel.h:12-18`) | esiste perché *«il §4.1 di `progettazione-hud.md` vieta ai widget di ricalcolare»* |
| `FRTScreenStack` (`Frontend/RTScreenStack.h`) | *«la navigazione non è UI: è una macchina a stati che governa la UI»* — 17 test, zero widget |

Un `FRTReplayViewModel` che possieda posizione dichiarata, stato di riproduzione e abilitazione dei
comandi rende automatizzabili **tutti e cinque** i criteri tranne il disegno. È anche il modo di tenere
fede all'intento della issue — sposta la logica fuori dai widget invece di aggiungerne.

### 4.4 🟡 Un esito di apertura su quattro, e la lista può mostrare partite che non si aprono

`ERTReplayOpenResult` ha quattro esiti (`RTReplayPlayerLibrary.h:19-30`): `Opened`, `ManifestUnreadable`,
`TopologyMismatch`, `TraceUnreadable`. La issue ne cita **uno**, e nemmeno quello: l'archivio parziale
(`bComplete == false`) è un caso di apertura **riuscita**.

Il caso operativo più probabile è strutturale, non ipotetico. L'indice di #416 **per design non apre gli
archivi** (`RTMatchHistoryLibrary.h:110-113`, verificato da un test che *«cancella gli archivi e la lista
si legge lo stesso»*): una riga della lista può quindi puntare a una cartella che non c'è più. È
esattamente ciò che rende l'indice veloce, ed è esattamente ciò che la UI deve saper dire.

📝 Criterio nuovo: *ognuno dei quattro esiti produce un messaggio distinto; una riga il cui archivio è
assente lo dice e resta nella lista.*

### 4.5 🟢 L'attore è nominato, il punto d'ingresso no

*«L'unica in cui l'attore è il giocatore»* identifica l'attore ma non il caso d'uso: da dove si entra
nella lista? Dal main menu, dalla schermata di risultato, da entrambi?

`docs/technical/spec-frontend-navigazione.md` §2.1 dichiara la gerarchia del frontend — `MainMenu`,
`ResultScreen`, `PauseMenu`, `LoadingScreen`, `ModalLayer` — e **non contiene schermate di replay**. R6
introdurrebbe due schermate in uno strato il cui owner documentale non le prevede. → **decisione (a)**, §5.

### 4.6 🟢 «per la parte automatizzabile» non è falsificabile, e una parte è già gratis

Se il confine lo decide l'implementatore a fine lavoro, il DoD è soddisfatto per costruzione.

In particolare il quinto criterio — *«nessuna chiamata al resolver dal percorso della UI»* — è già
automatizzabile **per costruzione e non per test**: è la tecnica che `RTReplayPlayerLibrary.h:72-77`
documenta e usa (nessuna catena di `#include` raggiunge il resolver, *«l'assenza della possibilità di
violarla»*). Va scritto così, o finirà fra le verifiche manuali.

### 4.7 🔵 Una voce di DoD la cui condizione è già scattata

> *«le icone e le etichette passano dal catalogo, **se a quel punto E20/E25 esistono**»*

Esistono: `UI/RTIconCatalogData.h` e `UI/RTIconLibrary.h` sono su `main`, con `ValidateIconCatalog` e
`FindMissingRequiredIcons`. Il condizionale è scaduto e va riscritto come obbligo — altrimenti resta una
casella spuntabile a vuoto.

---

## 5. Le due decisioni prese

### (a) Le schermate replay entrano nella gerarchia §2.1 — **sì**

Due schermate, col prefisso `WBP_RT_` deciso a CP 11.7:

```text
WBP_RT_FrontendRoot
├── …
├── WBP_RT_MatchHistory        ← la lista (#416)
└── WBP_RT_ReplayViewer        ← il viewer (#472)
```

Sono `FName` per lo stack (`MatchHistory`, `ReplayViewer`): `FRTScreenStack` non conosce i widget, e la
mappa nome → classe è un **dato** del navigatore. Registrare due nomi non modifica `Frontend/`.

### (b) #472 si allinea a E46 — **promozione a v0.1**

Nasce `RT-FEAT-UI-REPLAY-VIEWER` con `release: v0.1`, e la label `post-v0.1` esce da #472.

> ⚠️ **L'incoerenza che questa scelta introduce, registrata invece che nascosta.**
> `RT-FEAT-REPLAY-ARCHIVE` — il core che questa UI consuma — resta `release: v0.2`. Una feature v0.1 che
> dipende da una v0.2 è una contraddizione di scope, e **il validator non la intercetta**: misurato su
> `scripts/feature_registry.py:524`, il controllo sulle `dependencies` verifica che il `FeatureId`
> esista, non che la release sia coerente. Il gate resterebbe verde su un'incoerenza vera.
>
> Non è bloccante — il core è `INTEGRATED` e funziona, quindi la v0.1 non aspetta niente — ma la
> soluzione pulita è promuovere anche `RT-FEAT-REPLAY-ARCHIVE`, e quella è una decisione di scope della
> release che merita una `D-nnn`. Fino ad allora l'incoerenza vive qui e nella `note` della feature.
>
> ✅ **La `D-nnn` che questa riga chiedeva è arrivata il 2026-08-30: è
> [D-277](../../decisions/RT_PDR_00_Decision_Log.md)**, che sincronizza la decisione d'autore
> `AUTHOR-REPLAY-SCOPE-001`. `RT-FEAT-REPLAY-ARCHIVE` è **scope v0.1 sotto `E12`**, e i gate d'archivio
> ancora aperti sono obblighi della v0.1. L'incoerenza smette di vivere qui: fra il metadato `release: v0.2`
> e l'epic `E12` della v0.1 ha prevalso **l'epic**, e il metadato era la parte stantia.
> ⚠️ Resta vero il rilievo sul validator — un controllo che verifica l'esistenza del `FeatureId` e non la
> coerenza della release sarebbe stato verde su un'incoerenza vera — ma è un rilievo storico: il Feature
> Registry formale è **ritirato** da [D-181](../../decisions/RT_PDR_00_Decision_Log.md) e
> [D-246](../../decisions/RT_PDR_00_Decision_Log.md), e `scripts/feature_registry.py` non esiste più.

---

## 6. I criteri riscritti

Ciò che va sul body di #472, in sostituzione dei cinque attuali.

- [ ] si apre una partita dalla lista e la si guarda dall'inizio alla fine;
- [ ] i salti a turno e fase usano il seek di #415 — nessun secondo meccanismo di posizionamento;
- [ ] **l'interfaccia dichiara turno e fase correnti in ognuno dei quattro stati** — prima dell'inizio,
      in riproduzione, a fine sequenza, su traccia non indirizzabile — **e il valore è quello dell'ultima
      fase emessa, non quello del cursore** (§4.1);
- [ ] **i controlli di fase enumerano le cinque fasi osservabili** — `Prep · Dash · Blast · Move ·
      Cleanup` — e non `ERTMatchPhase`, di cui due valori non sono raggiungibili (§4.2);
- [ ] **ai quattro bordi il comando è disabilitato, non silenzioso**: prima fase del primo turno, ultima
      fase dell'ultimo, e i due omologhi per turno (§4.2);
- [ ] un archivio **parziale** si apre e si guarda fino a dove arriva, dicendo che finisce lì;
- [ ] **ognuno dei quattro esiti di `ERTReplayOpenResult` produce un messaggio distinto**, e una riga
      della lista il cui archivio è assente lo dice restando nella lista (§4.4);
- [ ] nessuna chiamata al resolver dal percorso della UI — **verificata per costruzione sugli `#include`**,
      come per il Player (ADR-0009 §3, §4.6).

E il DoD:

- [ ] la posizione, i bordi, gli esiti di apertura e l'assenza del resolver sono coperti da test sul
      **view model**, senza widget e senza mondo — il confine è dichiarato, non deciso a fine lavoro;
- [ ] la parte visiva entra in `docs/technical/test-manuali-pie.md` come voce ⏳ non bloccante, **in
      handoff alla track `playtest`** che possiede quel file;
- [ ] le icone e le etichette passano dal catalogo — **non più condizionale**: E20/E25 esistono (§4.7);
- [ ] le due schermate sono registrate nel navigatore di #936 e dichiarate in §2.1 della spec frontend.

---

## 7. Cosa resta fuori, e non cambia

I tre fuori scope della issue reggono, verificati:

- **micro-step** — `FRTTurnLogEntry` non porta né indice di sequenza né micro-step, e l'ordine delle voci
  serializzate è la chiave di sort, non l'ordine di emissione (`RTReplaySeekLibrary.h:74-77`, e
  `RTReplayPlayerLibrary.h:84-89` lo ripete dal lato del Player: `SortTurnLog` è chiamato *dentro* la
  risoluzione, quindi l'ordine di emissione è perso **in memoria**);
- **timeline scrubbabile continua** — stessa ragione, la granularità è la fase;
- **export video** — il replay è **logico** (ADR-0009 §0).

## 8. Il blind spot del panel

Nessuno dei sei esperti ha guardato il **volume**: 200 partite nell'indice, o una traccia da migliaia di
voci. Probabilmente irrilevante in v0.1 — il formato è 2v2 offline — ma è la sola dimensione che nessuna
delle sei prospettive copriva, e va detto qui invece di scoprirlo come «stranamente lento».
