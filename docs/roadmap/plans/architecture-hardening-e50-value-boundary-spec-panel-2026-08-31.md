# E50 · Il confine di valore di `FRTResolvedEvent` (`#1800`) — misura, fetta e quattro falsificazioni

> `CURRENT` · **Stato**: fetta implementata, suite **VALIDA** · **Data**: 2026-08-31
> **HEAD della revisione**: misurato e implementato su `origin/main` `0c0ee87c`; `origin/main` è avanzato
> a `6f4e5edc` **durante** la sessione (tre movimenti: `c84e3dd2` → `b498afec` → `0c0ee87c` → `6f4e5edc`).
> **Worktree**: `D:/Repositories/rt-wt-e50` — branch `refactor/1800-resolvedevent-value-boundary`.
> **Oggetto**: la seconda metà di [`#1800`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1800)
> — i due `TWeakObjectPtr<ARTUnit>` in `FRTResolvedEvent` — dentro l'Epic **E50** (`#1816`), più la rimisura
> di `#1818` e l'audit di `#1821` / `#1820` / `ARTPlayerController`.
> **Modo**: misura → implementazione di una fetta → review avversariale

---

## 1. Perché il lavoro non è stato fatto nella working directory principale

Un'altra sessione la stava usando: branch `feat/1864-selezione-condivisa`, editor aperto, e un commit
atterrato **mentre misuravo** (`c84e3dd2` → `940b16e5`). Compilare lì avrebbe compilato il loro lavoro in
corso, e la suite sarebbe stata NON REGISTRABILE per il digest dell'albero. Worktree dedicato.

---

## 2. Issue e PR analizzate

| # | Stato | Esito di questa sessione |
|---|---|---|
| **#1816** E50 | OPEN | epic, invariata |
| **#1817** playback | **CLOSED** | prima fetta, già chiusa il 2026-08-30 |
| **#1818** God Object | OPEN | rimisurata — §5 |
| **#1863** pacing | OPEN, `MERGEABLE/CLEAN` | **revisionata → merge-ready**, due rilievi sciolti |
| **#1862** | **MERGED** 06:16 | scioglie la dipendenza dichiarata da #1863 |
| **#1800** digest / value | OPEN | **riusata**, non duplicata: il commento del 2026-08-30 registra già questo lavoro nel suo scope |
| **#1820** GameState | OPEN / DEFER | **premessa scaduta** — commento pubblicato, nessuna implementazione |
| **#1821** ISP | OPEN | misurata, **non** aperta |

---

## 3. Quattro cose che la misura ha falsificato

### 3.1 🔴 `FRTResolvedEvent` non è un tipo di confine

`ResolvedTimeline` (`RTTurnManager.h:1689`), `PlaybackAttacks` (`:1735`) e `PlaybackDefeated` (`:1736`) sono
**tutti e tre `private`**. La struct nasce e muore dentro `ARTTurnManager`, a una trentina di righe di
distanza. Il confine simulazione → presentazione vero sono i delegate **pubblici** `OnAttackResolved`
(`:661`) e `OnUnitDefeated` (`:658`), che passano `ARTUnit*` — e continuano a passarlo anche dopo questa
fetta.

Conseguenza dichiarata **prima** di implementare: la fetta non riduce i test che richiedono un mondo (zero
test toccavano il tipo) e non riduce le righe. È stata implementata comunque, per decisione dell'autore del
mandato; questo referto ne registra il costo in §5.

### 3.2 🔴 La direzione proposta in `#1800` è sbagliata di uno

Il commento del 2026-08-30 propone `int32 UnitId` con «lo Stable ID che lo snapshot già usa —
`Units[i].UnitId == i`». `RTTurnManager.h:1582` dice l'opposto, **e lo dice come avvertenza**:

> dedurlo dall'indice della simulazione legherebbe la traccia a una corrispondenza
> (`StableUnitId == FRTHexSimUnit::UnitId + 1`) che nessuno ha dichiarato.

L'identità canonica è `ARTUnit::StableUnitId`, assegnata una volta sola da `EnsureMatchRoster` **a partire
da 1** (`Roster[i]->StableUnitId = i + 1`), con lo `0` lasciato libero da [D-063]. Implementare la proposta
alla lettera avrebbe prodotto un off-by-one silenzioso proprio nel punto in cui la presentazione sceglie
quale cilindro muovere. Il precedente giusto era già nel repository: `FRTMoveRoute::StableUnitId`.

### 3.3 🔴 `URTPlaybackLibrary::EstimatePlaybackSeconds` è dead code, ma non per la ragione che si dice

L'affermazione corrente è che sia «semanticamente sovrapposta a `PhaseDuration`».
`RTPlaybackLibrary.h:82-84` dice il contrario, esplicitamente:

> ⚠️ **Non è `EstimatePlaybackSeconds`.** Quella è una stima aggregata dell'intero round, questa è la durata
> di una singola fase e **è la formula che il gioco usa davvero** … Le due non sono intercambiabili.

E le formule divergono davvero: `PhaseDuration` per il `Blast` prende `Max(colpi, spinta)`,
`EstimatePlaybackSeconds` somma movimento + colpi + beat sull'intero round.

Ciò che è vero è più preciso, e più forte: il **ruolo** di stima aggregata è oggi svolto sommando
`PhaseDuration` — `RTTurnManager.cpp:6107`, `RawTotal += DurationForPlaybackPhase(Ph)`. Restano **due
formule diverse per la stessa domanda**, e a una non risponde più nessuno.

| Misura | Valore |
|---|---|
| chiamanti di produzione C++ | **0** |
| riferimenti Blueprint (`.uasset` sotto `Content/`) | **0** |
| asserzioni che la coprono | 4, in `RTPlaybackLibraryTests.cpp:76-85` |
| docstring che la citano come contro-termine | 2 — `RTPlaybackLibrary.h:57`, `:82` |

### 3.4 🔴 La premessa di `#1820` è scaduta

`ARTPlayerState` **esiste** — `Player/RTPlayerState.h:21` — arrivata con `#1815` (`1f3a633d`, 2026-08-30
**18:16**), cioè circa novanta minuti **dopo** l'audit `b45c314b` che `#1820` cita. Non è un contenitore
vuoto: sei consumatori di produzione (`RTGameMode:624`, `RTPlayerController:1094`/`:1114`, `RTHUD:439`,
`RTScreenHudWidgets:82`, `RTKnowledgeVeilPresenter:34`, `RTCameraPawn:148`) e un file di test dedicato.

`AGameState` invece manca davvero, e l'innesco dichiarato da `#1820` **non è scattato**: `Replicated` /
`NetMulticast` / `GetLifetimeReplicatedProps` in tutto `Source/` → **0**. La issue resta DEFER; va corretta
solo la riga di Evidence.

### 3.5 ⚠️ Il conteggio `GetAllActorsOfClass` di E50 è gonfiato

L'audit dichiara **19** in `RTTurnManager.cpp`. Le chiamate vere — `UGameplayStatics::GetAllActorsOfClass(`
— sono **11**; le altre 8 sono occorrenze del nome **in commento**. Non cambia la direzione di nessuna
conclusione, ma il numero pubblicato non è riproducibile, e la stessa metrica compare nelle misure
prima/dopo di `#1863`.

---

## 4. La fetta implementata

### Il problema di progetto, e come è stato neutralizzato

Sostituire due puntatori con due id costringe la presentazione a **tornare** all'Actor. Fatto ingenuamente
richiederebbe un nuovo `GetAllActorsOfClass` — cioè peggiorare esattamente la metrica che E50 dichiara — o
una nuova `TMap<int32, TWeakObjectPtr<ARTUnit>>`, cioè reintrodurre i weak pointer spostandoli.

Nessuna delle due. `EnsureMatchRoster` **costruiva già** la lista ordinata degli Actor per assegnare gli id,
e poi la buttava via. Ora la conserva come indice inverso:

| Simbolo | Cosa fa |
|---|---|
| `ARTTurnManager::MatchRoster` | `TArray<TWeakObjectPtr<ARTUnit>>`, ordinato per `StableUnitId`: l'indice `i` porta l'unità di id `i + 1` |
| `ARTTurnManager::UnitByStableId` | la porta: da id ad Actor, `nullptr` per «nessuno da animare» |
| `ARTTurnManager::RosterIndexForStableId` | **pura e `static`**: la metà inversa della convenzione, provabile **senza mondo** |

**Zero `GetAllActorsOfClass` aggiunti**: 11 prima, 11 dopo.

### Scelta di nome, e perché non è cosmetica

I campi si chiamano `SourceStableUnitId` / `TargetStableUnitId`, non `SourceUnitId`. Nel repository esiste
già una famiglia di campi con quel nome — `FRTActionInstance`, `FRTActionEvent`, `FRTNoiseEvent`,
`FRTPredictiveInstance` — e `RTActionQueue.h:12` dichiara che lì l'intero è l'**indice nello snapshot**, con
sentinella `INDEX_NONE`. Sono due identità diverse con due sentinelle diverse: lo stesso nome avrebbe
lasciato al compilatore niente da dire il giorno in cui qualcuno assegna l'una all'altra.

### File toccati

| File | Responsabilità |
|---|---|
| `Turn/RTResolvedEvent.h` | il fatto risolto diventa value type; **zero** riferimenti ad Actor nel codice |
| `Turn/RTTurnManager.h` | `MatchRoster`, `UnitByStableId`, `RosterIndexForStableId` |
| `Turn/RTTurnManager.cpp` | `EnsureMatchRoster` riempie l'indice; 5 siti di produzione; 4 regioni di consumo |
| `Tests/RTAttackPlaybackProbeForTest.h` | contatori di risoluzione (additivi, non cambiano il probe esistente) |
| `Tests/RTUnitIdentityTests.cpp` | 2 test **headless** |
| `Tests/RTMatchAutobattleTests.cpp` | 1 test end-to-end |

---

## 5. Misure prima / dopo

### `ARTTurnManager` (rimisura di `#1818`)

| Metrica | E50 (`b45c314b`) | Oggi (`0c0ee87c`) | Dopo la fetta |
|---|---|---|---|
| `RTTurnManager.h` | 1 749 | 1 749 | **1 793** (+44) |
| `RTTurnManager.cpp` | 6 479 | 6 464 | **6 512** (+48) |
| `RTTurnManager_Blast.cpp` | 2 184 | 2 184 | 2 184 |
| **totale** | 10 412 | 10 397 | **10 489** |
| metodi `ARTTurnManager::` definiti | ~90 | 90 | **92** |
| `UGameplayStatics::GetAllActorsOfClass(` reali | *(19 dichiarati, gonfiato)* | **11** | **11** |
| metodi pubblici nell'header | — | 44 | **45** |

🔴 **Questa fetta AUMENTA le righe del TurnManager di 92.** Su 334 righe aggiunte in tutto il diff, 139 sono
commento e 162 codice. `#1818` impegna a registrare le misure sfavorevoli e non solo quelle favorevoli:
questa è una di quelle. Se il criterio fossero le righe, la fetta andrebbe scartata.

### Ciò che invece migliora

| | Prima | Dopo |
|---|---|---|
| `RTResolvedEvent.h`: riferimenti ad `ARTUnit` nel **codice** | 2 `TWeakObjectPtr` + 1 forward declaration | **0** |
| test che esercitano la convenzione d'identità **senza mondo** | 0 | **2** |
| test che provano la risoluzione id → Actor | 0 | **1** |
| accessi al mondo aggiunti | — | **0** |
| macro di test nel repository | 1 527 | **1 530** |

---

## 6. Verifiche

Due misure, perché la prima era su un albero che non si sarebbe mergiato: CLAUDE.md §6 chiede il gate sul
commit che si mergia davvero.

| Prova | Pre-merge (`0c0ee87c` + diff) | **Post-merge (`c2786864`)** |
|---|---|---|
| Build `RefactorTacticsEditor Win64 Development` | `Result: Succeeded` — 110 s | **`Result: Succeeded`** — 49 s |
| `scripts/rt-suite.ps1` (suite intera) | VALIDA · 1 529 / 1 529 · 0 fallimenti | **VALIDA · 1 531 / 1 531 · 0 fallimenti** |
| copertura (`Found N` contro `Test Completed`) | 1 529 = 1 529 | **1 531 = 1 531** |
| `UnitIdentity.RosterIndexIsTheExactInverseOfTheIdAssignment` | `Result={Success}` | **`Result={Success}`** |
| `UnitIdentity.AResolvedFactOutlivesTheActorItCameFrom` | `Result={Success}` | **`Result={Success}`** |
| `Match.Autobattle.PlaybackResolvesEveryAttackSubjectFromItsStableId` | `Result={Success}` | **`Result={Success}`** |

I due test in più (1 529 → 1 531) vengono dal merge, non da questo diff: `RTBotAllyTests` (#1887) e
`RTHexProbeReadoutTests` (#1900).

Il test end-to-end **ha misurato materiale reale**, e lo dichiara nel log invece di lasciarlo dedurre:

```
colpi rivelati: 40 · sorgenti risolte 40/40 · bersagli risolti 40/40
```

Quaranta colpi rivelati, quaranta sorgenti e quaranta bersagli ritornati a essere Actor. La premessa
`Revealed > 0` non è teorica, e senza di essa il verde non significherebbe nulla.

### ⛔ NOT RUN

- **PIE / packaged**: nessuna esecuzione interattiva. Il playback è presentazione, e la parte che gli occhi
  giudicano — cilindri che scivolano, montage, morte differita — non è coperta da automation.
- **Build `Shipping`**: non eseguita; `WITH_DEV_AUTOMATION_TESTS = 0` compila cose che Editor e Development
  non compilano.

`origin/main` è avanzato ancora dopo il gate post-merge — `28b88522` → `e30361e9` — ma i due commit (#1905)
toccano **un solo file markdown**: zero codice, zero sovrapposizione con `Source/RefactorTactics/Turn/`. La
misura su `c2786864` resta rappresentativa dell'albero mergiabile.

---

## 7. Review avversariale del diff

| Domanda | Esito |
|---|---|
| deref nullo su `Unit->StableUnitId` (spinta) | ✅ `ApplyForcedDisplacement` apre con `if (!IsValid(Unit)) { return; }` |
| deref nullo su `Attacker` / `Victim` | ✅ guardati esplicitamente (`? : 0`) — `TWeakObjectPtr` accettava il null, ora lo accetta l'espressione |
| nuovi accessi al `World` | ✅ **0** — 11 prima, 11 dopo |
| ordine di iterazione (`TMap` / `TSet`) | ✅ nessuna struttura nuova; l'ordine di `ResolvedTimeline` è invariato |
| TurnLog canonico | ✅ intatto — `AddLogEvent` scrive in `RecentEvents` (combat log di presentazione), non nel TurnLog |
| `FRTLogSubject` | ✅ riceve ancora l'`ARTUnit*`, come **pretende**: `RTTurnManager.h:211` dichiara che «non esiste una forma che prenda il solo `StableUnitId`», perché il verdetto di [D-223] vuole anche squadra e cella |
| serializzazione / replay / hash | ✅ nessun campo di `FRTResolvedEvent` entra in un hash o in un archivio: la struct non è mai serializzata |
| Blueprint | ✅ **0** `.uasset` sotto `Content/` citano `FRTResolvedEvent` o i suoi campi |
| UHT | ✅ build pulita; i campi restano `BlueprintReadOnly` su `USTRUCT(BlueprintType)`, come prima |
| sezioni di accesso dell'header | ✅ verificato dopo l'inserimento: un primo tentativo declassava `PlanningTimerHandle` da `protected` a `private`, corretto |

### ⚠️ Una differenza latente, non raggiungibile oggi

Un'unità spawnata **dopo** il congelamento del roster conserva `StableUnitId = 0`, e un suo evento non
verrebbe animato — prima portava un puntatore valido e sarebbe stato animato.

Non è raggiungibile allo stato attuale: gli unici due spawner di `ARTUnit` fuori dai test sono
`RTMatchBootstrapper.cpp:285` e `RTScenarioSession.cpp:561`, entrambi in allestimento, prima della prima
risoluzione. Nessuna evocazione esiste. La conseguenza è documentata accanto ai campi in
`RTResolvedEvent.h`, perché è esattamente il tipo di cosa che un'evocazione futura farebbe emergere in
silenzio.

---

## 8. `#1821` (ISP) — misurata, non aperta

| Metrica | Valore su `0c0ee87c` |
|---|---|
| metodi pubblici in `RTTurnManager.h` | **44**, di cui **26 già `const`** |
| metodi protected | 67 |
| metodi private | 8 |

Il passo 1 che `#1821` propone — «rendere `const` ciò che semanticamente è const» — è **già fatto per il
59 %** della superficie pubblica. Il lavoro residuo non è rendere `const` dei metodi: è decidere quali dei
18 non-`const` sono davvero mutanti. Non è una fetta da aprire alla cieca, e resta bloccata come la issue
già dichiara.

---

## 9. `ARTPlayerController` (Fase 6) — audit senza issue

3 043 righe (733 header + 2 310 cpp), **71 metodi**, 31 `UPROPERTY`, e **5 soli include** nell'header —
igiene migliore di quella del TurnManager, che ne ha 21.

| Responsabilità | metodi |
|---|---|
| ability selection | 11 |
| **gesture camera / orbita / zoom** | **12 — 239 righe — 10 %** |
| selection | 9 |
| click / pointer / drag state | 7 |
| ActiveLayer | 6 |
| path planning / waypoint | 4 |
| facing | 3 |
| targeting · lock-in · playback · knowledge · input setup | 2 · 2 · 2 · 1 · 1 |

Il candidato indicato dal mandato — l'interpretazione delle gesture, con `RouteCameraGesture` da sola a 74
righe — **è** il gruppo più grande e coerente fra quelli non autoritativi. Ma il 10 % di un controller da
2 310 righe non è un SRP violato in modo misurabile, e non giustifica un owner nuovo.

**Nessuna issue aperta.** Aprirla significherebbe creare un proprietario per un problema che i numeri non
sostengono, che è esattamente ciò che `#1818` mette fuori scope.

---

## 10. Rischi residui

1. ~~la base del worktree è dietro `origin/main`~~ — **chiuso**: `origin/main` mergiato, gate rieseguito su
   `c2786864`, VALIDA 1 531/1 531. Resta vero che `origin/main` si muove ogni poche ore in questo
   repository, e un gate invecchia;
2. la fetta **aumenta** le righe del TurnManager — tensione reale e dichiarata con la metrica di `#1818`;
3. `MatchRoster` è un secondo posto dove vive un riferimento all'unità. **Non** è una seconda identità
   (quella resta `StableUnitId`, e il roster è solo il suo indice inverso), ma è stato nuovo dentro una
   classe che ne ha già troppo;
4. il playback verificato headless copre gli **attacchi**. Le morti mostrate — `HideForDefeat`,
   `PlayDefeatMontage` — passano dalla stessa porta `UnitByStableId` ma non hanno un oracolo dedicato:
   restano coperte solo dalla suite esistente.

---

## 11. Prossima fetta raccomandata — una sola

**Rimuovere `URTPlaybackLibrary::EstimatePlaybackSeconds`.**

Le misure che lo dimostrano stanno in §3.3: zero chiamanti di produzione, zero riferimenti Blueprint, e il
ruolo che svolgeva coperto dalla somma di `PhaseDuration` in `BeginPlayback`. Il costo è tre file più la
riscrittura di due docstring che la citano come contro-termine; il rischio è **LOW** perché non esiste
nessun consumatore da rompere.

È l'opposto della fetta appena chiusa. Questa ha aggiunto 92 righe al God Object per un confine di valore
che nessun test osservava; quella **toglie** una verità duplicata — due formule per «quanto dura questo
round», che danno numeri diversi — senza toccare niente che qualcuno legga.

⛔ **Non è una terza estrazione dal TurnManager**, e non deve diventarlo: §3.1 mostra che il criterio per
sceglierne una non è «cosa sembra fuori posto», ma se la superficie che si sposta è davvero attraversata da
qualcuno.
