# Due unità vive sulla stessa cella (`#1733`) — spec panel sull'issue come specifica di fix

> `CURRENT` · **Stato**: revisione chiusa, **e le cinque voci del DoD riscritto sono state consumate il
> 2026-08-31** — vedi §0 qui sotto. La riga precedente diceva *«nessun fix applicato, nessuna misura
> eseguita»*: vero per la revisione, non più per ciò che ne è seguito ·
> **Data**: 2026-08-30
> **HEAD della revisione**: scritta su `685c8780`, **riverificata su `71261937`** (= `origin/main` al
> 2026-08-30, sei commit più avanti). Dopo il fast-forward le **21 citazioni `file:riga`** di questo referto
> sono state ricontrollate una per una: **reggono tutte**, e con esse i fatti che sostengono — sei chiamanti
> di `ValidateSnapshot`, **zero in partita**; i tre test citati, agli stessi nomi e alle stesse righe. I sei
> commit non toccano `Source/`.
> **Oggetto**: la issue [`#1733`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1733) letta
> **come specifica di fix**, non il difetto che denuncia. Il difetto non è stato riprodotto qui.
> **Panel**: Wiegers (lead) · Cockburn · Adzic · Crispin · Fowler · Nygard
> **Modo**: critique · **Focus**: requirements, testing, architecture

---

## 0. Com'è finita *(aggiunto il 2026-08-31)*

Le cinque voci del DoD riscritto in §6 sono state consumate, e **`#1733` è chiusa**.

| | Voce | Esito | Dove |
|---|---|---|---|
| **A** | la sovrapposizione non è dimostrata: riclassificare | ✅ chiusa | `#1733` chiusa con il verdetto, senza test di riproduzione |
| **B** | il rifiuto nomina l'occupante, non chi pianifica | ✅ chiusa | [#1939](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1939) · PR #1947 |
| **C** | `MakeSnapshot` scarta la sovrapposizione in silenzio | ⏳ aperta | [#1970](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1970) — scorporata, sopravvive ad A |
| **D** | la prosa divergente sulla carica | ✅ chiusa | `D-296` · PR #1929 |
| **E** | il TurnLog non dice CHI | ✅ chiusa | [#1932](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1932) · PR #1935 |

🔎 **La raccomandazione più contro-intuitiva di questo referto ha retto.** §6 chiedeva di **non** scrivere il
test del DoD #5: una terza copia di quell'asserzione era stata davvero abbozzata in un worktree, ed è stata
**scartata** riconoscendola come duplicato di `HexMatch.TestArenaKeepsUnitsOnLegalCells` — stessa arena,
stessi 12 turni, stesso filtro `IsAlive()`, stessa asserzione.

⚠️ **E la voce E si è rivelata la CAUSA di A.** Le tre righe che `#1733` citava come prova sembravano parlare
di due unità perché il TurnLog non nominava il soggetto: chiusa quella, la prova si scioglie da sé. Il
referto lo aveva intuito trattandole come voci separate; erano la stessa storia vista da due lati.

---

## 1. Il verdetto in una riga

L'issue è **ben scritta e mal diagnosticata**: la sezione «Perché nessuno se n'è accorto» è il suo contributo
reale e va conservata intatta, ma la sezione «Cosa va deciso» riapre **due decisioni già prese, implementate,
motivate per iscritto e pinnate da un test di partita** — e il DoD che ne discende porterebbe a una PR che
spunta **quattro caselle su cinque senza toccare il difetto**.

🔴 **E poi il difetto stesso è caduto.** Dopo la prima stesura sono arrivate due misure che la revisione
chiedeva: la suite (§5-bis) e il TurnLog della sessione PIE (§5-ter). La seconda scioglie il caso —
**nessuna delle tre occorrenze citate come prova è una sovrapposizione**. Resta un difetto vero, ma è un
altro: il TurnLog **non dice CHI**, e chi lo legge attribuisce a due unità righe che parlano di una sola.

| | Voci |
|---|---:|
| 🔴 Critico | **3** |
| 🟡 Medio | **3** |

**Raccomandazione operativa**: **non aprire un ADR, non scrivere il test del DoD #5, non correggere `A`.**
Le due misure che la revisione chiedeva sono state fatte e concordano: la suite è verde su 1397 test
(§5-bis) e il TurnLog non contiene sovrapposizioni (§5-ter). **`A` va riclassificato**, non risolto. `B` e
`C` restano veri, si scorporano, e `B` cresce di importanza: è la punta di un difetto di **attribuzione**
che ha appena prodotto una issue di bug su un comportamento corretto.

---

## 2. Ciò che l'issue afferma, contro ciò che c'è

La colonna «misura» è la prova, non l'impressione.

> ⚠️ **La tabella qui sotto è della prima stesura.** La prima riga — «le celle nel TurnLog sono il dato
> autorevole, e sono quelle a coincidere» — è stata poi **falsificata** in §5-ter: nel TurnLog non
> coincidono. Le righe restano perché ogni singola misura regge; cambia la conclusione che se ne trae.

| L'issue afferma | Esito | Misura su `71261937` |
|---|---|---|
| `ValidateSnapshot` ha 0 chiamanti nel percorso di partita | ✅ **confermato** | 5 siti in `Tests/RTHexSimTests.cpp`, 1 in `Debug/RTDebugReportLibrary.cpp:215`, **nessun altro** |
| Il rifiuto del waypoint nomina chi pianifica | ✅ **confermato** | `Player/RTPlayerController.cpp:1006` stampa `*SelectedUnit->GetName()`, come gli altri due rami del `switch` |
| «L'occupancy blocca il Move normale… queste non le blocca» | ❌ **contraddetto** | la funzione che risolve entrambe le abilità le blocca: `RTMovementActionLibrary.cpp:116-143` |
| «Un `LinearCharge` deve fermarsi *sulla* o *adiacente*? — va deciso» | ❌ **già deciso** | `Result.Final = Current` (`RTMovementActionLibrary.cpp:143`) — cella **precedente** |
| …e non è solo codice | ❌ **già pinnato da un test** | `RefactorTactics.HexMatch.ChargeStopsOnEnemyAndHits` asserisce `Charger->Cell == (2,0,0)` con il bersaglio su `(3,0)` |
| «Un Dash che attraversa può *terminare* su occupata? — va deciso» | ❌ **già deciso e motivato** | `RTMovementActionLibrary.cpp:116` — il ramo pass-through vale solo `K < Distance` |
| DoD #5: serve un test headless che giochi una partita e verifichi l'invariante | ❌ **esiste già due volte** | `Tests/RTHexMatchIntegrationTests.cpp:125` e `:343`, **stessi due eroi, stessa arena** |

---

## 3. 🔴 Critico

### 🔴-1 · Il test del DoD #5 esiste, usa esattamente il caso del bug, ed è verde

**Crispin.** *«Prima di chiedere un test nuovo, chiedete perché quello vecchio tace. Un test che dovrebbe
rompersi e non si rompe è un difetto più grave del bug che stava cercando.»*

`Source/RefactorTactics/Tests/RTHexMatchIntegrationTests.cpp` contiene **due** cicli che giocano una partita
completa e asseriscono l'invariante turno per turno:

| Test | Riga | Setup | Turni |
|---|---:|---|---:|
| `RefactorTactics.HexMatch.PlaysToCompletion` | 125 | 2v2 `MakeWraith` + `MakeRiktor`, mappa raggio 5 | ≤ 40 |
| `RefactorTactics.HexMatch.TestArenaKeepsUnitsOnLegalCells` | 343 | 2v2 `MakeWraith` + `MakeRiktor` su `URTMatchSetupLibrary::MakeTestArena` | ≤ 12 |

Il secondo **è la sessione PIE dell'issue**: stessa arena generata, stessi due eroi, stesso ordine di
grandezza di turni. Il suo docstring dichiara già il mandato che il DoD chiede — *«non verifica un esito
voluto, verifica che non accada nulla di illegale»*.

∴ Il DoD chiede di scrivere ciò che c'è. Un sesto ciclo identico non catturerebbe nulla che questi due non
catturino già. **Le uniche uscite, tutte da misurare prima di una riga di fix:**

1. ~~sono **rossi** e nessuno li ha eseguiti di recente~~ → **ESCLUSA, misurata** (vedi §5-bis): sono
   verdi, e con loro l'intera suite;
2. sono **verdi** perché il bot non arma mai `PassingBlade`/`Ram` in quegli scenari → il difetto è nella
   **copertura**, e il test nuovo deve **forzare** l'abilità (come fa già `ChargeStopsOnEnemyAndHits`, che
   scrive il piano a mano con `bIsBotControlled = false`), non sperare che il bot la scelga;
3. sono verdi perché l'oracolo misura `ARTUnit::Cell` — gli Actor — mentre la sovrapposizione che l'issue
   documenta vive nel **TurnLog/Snapshot** → l'oracolo guarda il posto sbagliato;
4. il difetto richiede la **pianificazione umana** (waypoint dal `PlayerController`) che `PlayOneTurn` non
   esercita → non è riproducibile headless senza aggiungere quell'ingresso, e va detto.

📝 **Il DoD #5 va sostituito**: *«misura perché `TestArenaKeepsUnitsOnLegalCells` non cattura il caso, poi
estendi quel test»*.

🔴 **E sta già succedendo, mentre questo referto veniva scritto.** Misurato il 2026-08-30: esiste un worktree
`D:/Repositories/rt-wt-overlap` sul branch `test/unita-sovrapposte`, che sta scrivendo un test **nuovo** —
`RefactorTactics.Match.Autobattle.NoTwoUnitsShareACell` — invece di estendere i due che ci sono. Al momento
della misura non compila (`RTAuthoredMapEngagementTests.cpp:614`, manca `#include "Turn/RTMatchSetupLibrary.h"`)
e la sua `rt-suite` è uscita `2` dopo 768 s di attesa, con **tre** checkout in coda sullo stesso motore — la
dimostrazione operativa di ciò che [`CLAUDE.md`](../../../CLAUDE.md) §7 dichiara sui worktree. Il test non è
ancora su `main` (`grep` su `71261937`: nessuna occorrenza).

### 🔴-2 · Le due «domande di regola» hanno già una risposta. Il difetto vero è terminologico

**Wiegers.** *«Un requisito che riapre una decisione già presa non è un requisito: è un invito a
implementarla una seconda volta, diversa.»*

**Domanda 1 — la carica si ferma *sopra* o *adiacente*?** Il codice risponde **adiacente**, e un test di
partita lo pinna. Ma la prosa che lo descrive esiste in **due versioni che si contraddicono**:

| Sede | Riga | Testo | Dice |
|---|---:|---|---|
| `Ability/RTActionDef.h` | 212 | «si ferma **SUL** primo nemico incontrato e lo colpisce» | sopra |
| `Turn/RTMovementActionLibrary.h` (`ERTLinearStop::Impact`) | 23 | «Fermata **SUL** primo nemico incontrato» | sopra |
| `Turn/RTMovementActionLibrary.h` (`ResolveLinearMove`) | 100 | «si ferma **lì davanti** e lo segnala come impatto» | adiacente |
| `Turn/RTMovementActionLibrary.h` (`IsLinearReachable`) | 124 | «esito `Impact`, **cella precedente**» | adiacente |
| `Turn/RTMovementActionLibrary.cpp` | 143 | `Result.Final = Current;` | **adiacente** ← autorità |
| `Tests/RTHexMatchIntegrationTests.cpp` | 553 | `TestEqual(«chi carica si ferma davanti al nemico», …, (2,0,0))` | **adiacente** ← pinnato |

Il test disambigua anche la parola che l'issue chiama ambigua, e lo fa in un commento: *«Si ferma ADDOSSO:
adiacente al bersaglio, non sopra e non oltre.»*

∴ Non c'è una regola da decidere. C'è **una regola con due prose che la smentiscono a due file di distanza**,
e l'issue ha creduto a quella sbagliata. Il lavoro non è un ADR: è allineare due commenti al comportamento.

**Domanda 2 — un Dash che attraversa può terminare su occupata?** Risposta già scritta, con il suo rationale,
dentro `ResolveLinearMove` (`RTMovementActionLibrary.cpp:113-116`):

> *«La lama ATTRAVERSA e tira dritto — tranne che sulla cella d'arrivo: si passa in mezzo a qualcuno, non ci
> si ferma dentro. Due unità nella stessa cella non sono rappresentabili, e un'eccezione qui la renderebbe
> possibile per una sola azione.»*

📝 Il DoD #1 non chiede una decisione nuova: chiede di **registrare** quella esistente (un `D-nnn` che cita
`RTMovementActionLibrary.cpp` come fonte) e di **cancellare** le due prose divergenti. Sono due lavori con
costi diversi, e il secondo è un diff di quattro righe.

### 🔴-3 · Il DoD #2 è già vero: si può chiudere l'issue senza correggere il difetto

**Nygard.** *«Il criterio di accettazione più pericoloso è quello che il sistema soddisfa già. Lo si spunta,
si chiude il ticket, e il difetto resta in produzione con un'issue chiusa sopra.»*

`ResolveLinearMove` — chiamata dal percorso di partita a `Turn/RTTurnManager.cpp:3668` con
`Snapshot.Occupancy` — blocca **tutti e tre** gli stili sulla cella d'arrivo:

| Stile | Cella d'arrivo occupata → | `Final` |
|---|---|---|
| `LinearDash` | `ERTLinearStop::BlockedByUnit` | cella precedente |
| `LinearCharge` (nemico) | `ERTLinearStop::Impact` | cella precedente |
| `LinearPass` (`K == Distance`) | `ERTLinearStop::BlockedByUnit` | cella precedente |
| `LinearLeap` | `ERTLinearStop::BlockedByUnit` | non parte |

∴ **«Un'abilità di movimento non può terminare su una cella occupata» è già vero in `ResolveLinearMove`.**
Chi prende l'issue lo verifica, lo spunta, e la partita continua a sovrapporre le unità. È il modo in cui un
difetto reale sopravvive a una fix.

---

## 4. 🟡 Medio

### 🟡-1 · Tre difetti in una issue, con tre attori e tre criteri di chiusura

**Cockburn.** *«Chiedete chi sta cercando di ottenere cosa. Se le risposte sono tre, i ticket sono tre.»*

| Difetto | Attore | Sede | Chiude quando |
|---|---|---|---|
| **A** — due unità vive sulla stessa cella | il giocatore che **vede** | resolver, fasi `Dash`/`Blast`/`Move` | un test riproduce e poi passa |
| **B** — il rifiuto nomina chi pianifica | il giocatore che **legge** | `RTPlayerController.cpp:1006` | un test sul testo del log |
| **C** — l'invariante non è osservata in partita | chi **diagnostica**, mesi dopo | `MakeSnapshot` / `ValidateSnapshot` | è decisa la sede e la severità |

**B** è una fix di una riga — `ClassifyWaypointCell` conosce già la cella, l'`UnitId` sta in
`Snapshot.Occupancy` — e non ha alcuna dipendenza da **A**. Tenerla dentro la fa aspettare una decisione di
regola che 🔴-2 ha appena dichiarato non necessaria.

**C** sopravvive ad **A**: anche con **A** corretto, **C** resta il motivo per cui la prossima sovrapposizione
sarà di nuovo invisibile. È il valore duraturo dell'issue, ed è l'unico punto che nessun altro documento
copre.

### 🟡-2 · Il log è una traccia, non un esempio: manca il *Given*

**Adzic.** *«Tre occorrenze e nessuna riproducibile. La sezione "Riproduzione" dice dove giocare, non cosa
fare.»*

```gherkin
Given  arena GeneratedTestArena, Format.Skirmish2v2
  And  Wraith(team 0) su (0,0), Hero.Wraith.PassingBlade armata verso (1,-1)
  And  <unità X> su (4,-4) con Action.Move pianificato su (1,-1)
  And  chi occupava (1,-1) a INIZIO fase Dash            ← MANCANTE, ed è il fatto decisivo
When   si risolve il turno
Then   nessuna cella regge due unita' vive
```

La riga mancante distingue **H1** da **H2** (§5): se `(1,-1)` era **libera** a inizio Dash, il colpevole è la
frontiera `Dash → Move` e non `ResolveLinearMove`. Il TurnLog della sessione contiene già il dato — va
estratto, non ridedotto.

✅ **Estratto (§5-ter), ed è stato il finding decisivo dell'intera revisione.** La cella era **libera**, ma la
risposta non è «H2»: è che **non c'è nessuna sovrapposizione**. Questo è il valore di pretendere il *Given*
prima del fix — la riga mancante non ha scelto fra due ipotesi, le ha eliminate tutte e quattro.

⚠️ **Le tre occorrenze non hanno la stessa forma.** Il turno 3 mostra due unità che *arrivano* sulla stessa
cella in fasi diverse; i turni 5 e 6 mostrano una che arriva su una cella dove un'altra **resta** — che
`ResolveLinearMove` blocca esplicitamente. O sono **due difetti sotto un titolo solo**, o uno dei due log va
riletto.

### 🟡-3 · Il DoD #4 è l'unico ben posto, ma non è ancora eseguibile

**Fowler.** *«"O `ValidateSnapshot` entra nel percorso di partita, o si dichiara perché no" è la domanda
giusta. Ma non dice dove, e in un resolver a microstep il dove cambia tutto.»*

`ValidateSnapshot` è O(n) sulle unità: su quattro unità l'argomento «troppo caro» non regge in v0.1. Quel che
il DoD non specifica:

- **dove** — a fine turno (barato: `MakeSnapshot` può aver già scelto un vincitore) oppure **subito dopo ogni
  `ResolveHexPaths`**, dove il fatto nasce;
- **cosa fa quando fallisce** — `UE_LOG(Error)`, `ensureMsgf`, `checkf`? In PIE un `ensure` ferma il
  playtest, un log passa inosservato: sarebbe **lo stesso difetto** che l'issue denuncia, con una riga in più;
- **chi** — `MakeSnapshot` (`Turn/RTHexSimLibrary.cpp:48-51`) sa già che sta scartando un errore strutturale,
  e lo scrive nel proprio commento. È l'unico punto in cui l'informazione esiste ed è anche l'unico che la
  butta via.

**Nygard.** *«Una riga di `UE_LOG(LogRT, Error)` dentro il `Contains(Unit.Cell)` che oggi scarta in silenzio,
e il difetto non avrebbe aspettato uno schermo.»*

---

## 5-bis. La misura che mancava, eseguita il 2026-08-30 alle 08:00:55

**La prima riga del DoD riscritto è stata soddisfatta**, e non da una run di questo panel: una `rt-suite`
completa (`RunTests RefactorTactics`) lanciata da un'altra sessione **su questo stesso checkout** ha
misurato l'intera suite mentre il referto veniva scritto. Il log è stato letto, non dedotto.

| | Esito |
|---|---|
| Dichiarati / completati / `Success` | **1397 / 1397 / 1397** — zero fallimenti |
| `HexMatch.TestArenaKeepsUnitsOnLegalCells` | `Result={Success}` |
| `HexMatch.PlaysToCompletion` | `Result={Success}` |
| `HexMatch.ChargeStopsOnEnemyAndHits` | `Result={Success}` — la regola «adiacente» di 🔴-2 è viva e verde |
| Chiusura | `**** TEST COMPLETE. EXIT CODE: 0 ****` |

I due numeri coincidono — `Found 1397` in testa e 1397 `Test Completed` — che è la condizione che
[`CLAUDE.md`](../../../CLAUDE.md) §4 chiede di verificare a mano da quando **D-181** ha ritirato lo script che
li confrontava. L'exit code non è stato usato come oracolo: i `Result={...}` sono stati contati uno per uno.

**Validità della finestra** (08:00:55 → `TEST COMPLETE` ~08:09:01), ricostruita dai fatti osservabili perché
il referto di `rt-suite` appartiene alla sessione che l'ha lanciata:

| Asse | Stato |
|---|---|
| `HEAD` | `0621ab1f` stabile; il salto a `14e08843` è **posteriore** alla fine |
| albero | 4 file; il quinto (`docs/technical/test-manuali-pie.md`) toccato alle **08:10:09**, dopo |
| binario | `UnrealEditor-RefactorTactics.dll` `07:32:20 / 10413056`, identico prima e dopo |
| processi | uno solo; il `ScreenHud` da `wt-hud-scenario` è partito alle 08:09:18, **dopo** |

⚠️ **Il punto cieco dichiarato resta uno**: su quale commit sia stato compilato il DLL delle `07:32:20` non è
noto — è il limite che `rt-suite` dichiara di non coprire (binario *già* stantio all'avvio). Attenuante
misurabile, non rassicurazione: né `71261937` né `0621ab1f` toccano `Source/`, e i tre test citati sono
codice invariato da settimane.

∴ **`#1733` non è un gate rosso che nessuno guardava.** L'uscita (1) di 🔴-1 cade, e con essa l'ipotesi più
comoda. Restano le tre che costano lavoro: **copertura** (il bot non arma mai `p30`/`p35`), **oracolo**
(`ARTUnit::Cell` invece dello Snapshot), **ingresso** (la pianificazione umana che `PlayOneTurn` non
esercita). Un test nuovo che non risolva una di queste tre sarà verde quanto i due che ci sono già.

---

## 5-ter. Il TurnLog PIE, letto per intero: le tre sovrapposizioni non ci sono

Sorgente: `refactor-tactics-main/Saved/Logs/RefactorTactics-backup-2026.08.30-04.47.11.log` —
`MapSource=GeneratedTestArena`, `Format.Skirmish2v2`, `RoundLimit 12`, pareggio al round 12. È la sessione
dell'issue. Unità: `Wraith` + `Riktor` (team 0), `Phase` + `Gadget` (team 1).

### Turno 3 — la cella era libera, e la riga che lo dice sta in mezzo alle due citate

```
Dash    Wraith  (0,0)  -> (1,-1)   Hero.Wraith.PassingBlade  p30
Blast   Spinta: BP_Unit_Wraith_C_0 -> (-1,-1)
        spostata (1,-1) -> (-1,-1) (2 celle) (Hero.Phase.PressureJet)   <- il Wraith LASCIA (1,-1)
Move    Gadget  (4,-4) -> (1,-1)   Action.Move               p50
```

L'issue mette in fila la prima e l'ultima riga. Fra loro c'è il **Blast**, che spinge il Wraith fuori dalla
cella con `Hero.Phase.PressureJet`. Fine turno: `(-1,-1)` `(1,0)` `(3,-2)` `(1,-1)` — **quattro celle
distinte**.

### Turni 5 e 6 — la riga `resta` è la stessa unità arrivata col Dash

| | Dash | La riga letta come «un altro» | Chi è davvero | Fine turno |
|---|---|---|---|---|
| **T5** | Wraith `(-1,-1) → (1,-1)` | `resta (1,-1)` | il **Wraith stesso** — nel Blast nessuna spinta | `(-1,0)` `(1,-1)` `(2,-2)` `(1,0)` |
| **T6** | Riktor `(-1,0) → (0,0)` | `resta (0,0)` | il **Riktor stesso** — l'unica spinta porta Phase `(1,0) → (2,0)` | `(0,0)` `(1,-1)` `(2,-2)` `(2,0)` |

Quattro celle distinte in entrambi i casi. **In nessuno dei dodici turni due unità vive condividono una
cella** — il che è anche il motivo per cui i 1397 test di §5-bis sono verdi: l'invariante non è mai stata
violata.

### 🔴 Il difetto vero: il TurnLog non dice CHI

Due proprietà, entrambe reali, che insieme producono la lettura sbagliata:

1. **Le righe di movimento non nominano l'unità.** `si muove (q=…) -> (q=…) (Action.Move, p50)`: solo celle.
   Distinguere «la stessa unità che resta» da «un'altra che occupa» è **impossibile per chi legge**. È
   letteralmente la stessa famiglia del difetto `B` (§4 🟡-1) — il rifiuto che nomina `SelectedUnit` invece
   dell'occupante. Il sistema sa chi è e non lo scrive.
2. **Il playback stampa le fasi in blocco**, con lo stesso timestamp: `Dash`, poi `Blast`, poi `Move`. Due
   righe che finiscono sulla stessa cella si trovano a poche righe di distanza, e lo spostamento che le
   separa è scritto in **un'altra forma verbale** (`spostata`, non `si muove`).

∴ Il costo di questo difetto è misurato: ha prodotto **una issue di bug con `Definition of Done` su un
comportamento corretto**, e — senza questa lettura — una fix a un resolver che non ha nulla che non va.

⚠️ **Cosa questa ricostruzione NON spiega**: cosa il giocatore abbia visto **a schermo**. Ipotesi non
verificata: nel turno 3 il playback anima `Dash → Blast → Move`, e se la spinta del Wraith fuori da `(1,-1)`
non è resa, o è resa in ritardo, due modelli stanno davvero sulla stessa cella **durante l'animazione**.
Sarebbe presentazione — proprio ciò che l'issue esclude — e si conferma guardando il playback, non il log.

---

## 5. ~~Le quattro ipotesi ancora vive~~ — superata da §5-ter

⛔ **Conservata per la provenienza, non più valida.** Le quattro ipotesi rispondevano a «dove sta il difetto»;
§5-ter mostra che il difetto **non è nel resolver**, quindi cadono tutte e quattro — `H2` compresa, che era
la favorita. Resta l'insegnamento: erano tutte plausibili, e **nessuna era la risposta**, perché tutte
davano per buona la premessa dell'issue invece di verificarla.

Nessuna è esclusa dall'issue, e nessuna è stata verificata qui. `ResolveLinearMove` è **scagionata** da 🔴-3:
la causa sta a valle o accanto.

| # | Ipotesi | Ancora | Perché è plausibile |
|---|---|---|---|
| **H1** | occupancy **congelata a inizio fase** | `RTTurnManager.cpp:3738` | il dasher vede le posizioni di inizio `Dash`, non quelle finali |
| **H2** | frontiera **`Dash` → `Move`** | fasi `Dash → Blast → Move` | tutte e tre le occorrenze accoppiano una `p30`/`p35` a una `p50`: **mai due dasher**. La fase `Move` vede la cella d'arrivo del dasher? |
| **H3** | il **`Push 1`** di `Ram` | `docs/balance/RT_HeroCatalog_v0.1.md:153` | il bersaglio è spostato nel `Blast`; la riconciliazione con l'occupancy della fase `Move` è un'altra macchina (`ERTDisplacementBlockReason`) |
| **H4** | microstep **`bPassThrough`** | `RTHexSimLibrary.cpp:535`, `RTTurnManager.cpp:3728` | è l'unico punto che *volutamente* ignora un'unità ferma; il percorso è già troncato, ma la garanzia è indiretta |

**H2 è la più economica da falsificare** e spiega il pattern di tutte e tre le occorrenze. Va misurata prima
di scrivere il fix, non dopo.

---

## 6. Il DoD riscritto

```markdown
### Prima di tutto — misurare (ENTRAMBE FATTE, e concordano)
- [x] Eseguire `RefactorTactics.HexMatch` e registrare l'esito dei due test anti-sovrapposizione
      -> FATTO (§5-bis): 1397/1397 Success, entrambi VERDI
- [x] Estrarre dal TurnLog PIE chi occupava (1,-1) e (0,0) a INIZIO fase Dash
      -> FATTO (§5-ter): erano LIBERE. Nessuna delle tre occorrenze e' una sovrapposizione

### A — la sovrapposizione: NON DIMOSTRATA, da riclassificare
- [ ] L'issue si riclassifica: le tre occorrenze citate come prova non lo sono (§5-ter)
- [ ] Se il difetto a schermo resta, si riapre come PRESENTAZIONE con una prova nuova:
      il playback del turno 3 rende la spinta del Wraith fuori da (1,-1)?
- [ ] NON scrivere un test di riproduzione: non c'e' niente da riprodurre nel resolver

### E — il TurnLog non dice CHI (difetto NUOVO, il piu' costoso dei quattro)
- [ ] Le righe di movimento nominano l'unita', non solo le celle
- [ ] Distinguere «resta» di chi era gia' li' da «resta» di chi e' appena arrivato col Dash
- [ ] Costo misurato di non farlo: una issue di bug con DoD su un comportamento corretto

### C — l'invariante osservata (issue separata, sopravvive ad A)
- [ ] MakeSnapshot segnala la sovrapposizione nel punto in cui oggi la scarta in silenzio
- [ ] E' dichiarato COSA fa quando accade (UE_LOG Error / ensureMsgf) e con quale costo

### B — la diagnostica del rifiuto (issue separata, indipendente)
- [ ] RTPlayerController.cpp:1006 nomina l'OCCUPANTE, non SelectedUnit
- [ ] Test sul testo, come HexSim.WaypointRejectionSaysWhich gia' fa per il ramo budget

### D — la prosa divergente (diff di quattro righe, nessun ADR)
- [ ] RTActionDef.h:212 e ERTLinearStop::Impact dicono «SUL»; il codice e il test dicono
      «adiacente». Allineare i commenti a Result.Final = Current
- [ ] D-nnn che REGISTRA la regola esistente citando RTMovementActionLibrary.cpp come fonte
```

---

## 7. Cosa questa revisione **non** ha fatto

**Crispin.** Il confine di ciò che è stato verificato è parte del referto, non una nota a piè di pagina.

- 🔄 **Una suite è stata eseguita — non da questo panel.** La prima stesura dichiarava «nessuna suite
  eseguita, l'esito dei due test è non misurato». Non è più vero: §5-bis riporta una `rt-suite` completa
  lanciata da un'altra sessione sullo stesso checkout, letta dal suo log. **Nessuna build, nessun Editor
  aperto da qui**, e nessuna run lanciata da questo panel: quella armata alle 08:0x è stata **fermata**
  proprio per non misurare due volte la stessa cosa tenendo il motore mentre tre checkout aspettavano.
- 🔄 **Il difetto non è stato riprodotto — e §5-ter spiega perché**: nel resolver non c'è. La prima stesura
  diceva «l'issue non sa dove sia il bug, quattro ipotesi restano aperte»; oggi il TurnLog dice che le tre
  occorrenze citate come prova non sono sovrapposizioni. Resta **non spiegato** cosa sia stato visto a
  schermo: l'ipotesi presentazione è dichiarata in §5-ter e **non verificata**, perché richiede di guardare
  il playback, non un log.
- ⛔ **Nessun file di `Source/` toccato.** Questo consumo scrive solo `docs/`.
- ✅ **Il caveat sul checkout è caduto.** La prima stesura misurava `685c8780`, sei commit dietro
  `origin/main`. Il checkout è stato allineato a `71261937` e le 21 citazioni sono state **riverificate una
  per una**: nessuna è scaduta. Restano citabili in una PR senza ricontrollo.
- 🔄 **`Push` e `Pull` non sono stati istruiti a fondo**: `RTTurnManager.cpp:4692` mostra che il
  displacement ha una macchina propria con reason code (`ERTDisplacementBlockReason`), quindi **H3** non è
  né confermata né esclusa.

---

## 8. Provenienza

Panel richiesto in sessione con `/sc:spec-panel #1733` il 2026-08-30. L'oggetto della review è la issue
stessa, non un kit esterno: non c'è sorgente da archiviare.

I findings `E3`–`E6` di §2 e il DoD riscritto di §6 sono stati riportati come commento su
[`#1733`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1733).

Documenti correlati: [`RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md) per il `D-nnn`
di §6-D — **D-118** (`Traversal` / `Transfer`) è il vicino semantico e va letto prima di numerarne uno nuovo.
