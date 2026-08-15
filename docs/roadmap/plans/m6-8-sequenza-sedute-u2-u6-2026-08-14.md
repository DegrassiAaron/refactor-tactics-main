# M6.8 — sequenza operativa delle sedute `U2`–`U6`

> `CURRENT` · **Data**: 2026-08-14 · **HEAD**: `c371c9a0`
> **Cosa è**: la procedura per eseguire il playtest di [`#38`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/38)
> — cosa aprire, in che ordine, cosa guardare, e **quali righe di log catturare** per ciascuna voce.
> **Cosa non è**: il registro degli esiti. Quello è
> [`../../technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md), e resta l'unico owner del
> verdetto. Qui non si scrive «✅»: si scrive lì.
> **Compagno**: [`m6-8-playtest-hex-spec-panel-2026-08-14.md`](m6-8-playtest-hex-spec-panel-2026-08-14.md),
> che spiega *perché* la DoD della issue è stata riscritta. Questo file è il *come*.
> **Sorgente delle sedute**: [`../editor-sessions.yaml`](../editor-sessions.yaml), campi `verifies`,
> `shares_setup_with`, `done_when`. Se le due divergono, vince il YAML.

---

## 0. Tre cose da sapere prima di premere Play

**Non si salva nessun asset mappa.** Il terreno arriva da codice (`ERTMapSource::GeneratedTestArena`). Se
l'editor chiede di salvare `L_HexArena` o `DA_HexMap_Arena`, la risposta è **no**: quel package è il prodotto
di U1 ([`#451`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/451)) e la sua Binary Asset
Lease non è emessa per questa seduta. Due `.uasset` non si fondono.

**«Rimuovi l'arco e verifica che il path fallisca» NON si fa qui.** Su `GeneratedTestArena` la transizione è
creata da `MakeTestArena` — `(1,0,0) -> (2,0,1)` — e non c'è un asset da editare. Quella metà di
`PIE-HEXPLAY-8` vive su un asset mappa vero (U1/U13) ed è già coperta headless da
`Structures.Bridge.RemovalBreaksPath` e `…NoTeleportOnRemoval`.

**Dove finiscono gli esiti va deciso prima.** `docs/technical/test-manuali-pie.md` è nel `writable` della
track `content_editor` in `parallel-batch.yaml`. Chi esegue queste sedute e scrive i verdetti tocca un file
non assegnato: per **D-139** è STOP e riallocazione. Sedersi all'editor senza aver sciolto questo nodo
significa produrre un verdetto che non si può registrare.

---

## 1. La preparazione condivisa — una apertura, cinque sedute

`U2`…`U6` dichiarano `shares_setup_with` reciproco: la stessa partita serve a tutte e cinque. Si prepara
una volta.

1. **GameMode Override** sul livello: `RTGameMode` (procedura in
   [`../../technical/debug-vs-unreal.md`](../../technical/debug-vs-unreal.md) §2).
2. **`MapSource = GeneratedTestArena`** — `ARTGameMode`, categoria *RefactorTactics ▸ Map*. Il default è
   `LevelAsset`: **va cambiato**, altrimenti si gioca sull'arena d'autore o sul ripiego liscio, e metà delle
   voci non ha l'oggetto da osservare. `MakeTestArena` fornisce esagono r=4, ostacoli, **muro che blocca la
   vista**, fango a costo 3, piattaforma sul layer 1 e **una** transizione.
3. **Play**.

**Le unità sono cilindri** e non è un difetto: i `BP_Unit_*` non esistono in `Content/`, il fallback è
previsto (li produce U7).

**Formato**: `Format.Skirmish2v2`, `RoundLimit` **12** — verificato in `RTMatchFormatLibrary.cpp`
(`FindShippedFormat`), portato da 5 a 12 il 2026-08-10 per allinearsi a **D-010** (banda 10–14 in 2v2).
⚠️ Se l'HUD mostra `Round n/5`, la build è vecchia: **fermati**, perché il ritmo misurato non varrebbe.

**Prima riga da vedere nel log**, che conferma che l'allestimento è quello giusto:

```
[RT] Board 2v2 esagonale avviata su N celle con 4 eroi
```

### I comandi — letti da `MappingContext->MapKey`, non dalla memoria

| Tasto | Azione |
|---|---|
| **W A S D** | pan della camera |
| **rotellina** | zoom |
| **Q** · **E** | rotazione |
| **click sinistro** | seleziona un'unità · aggiunge un waypoint · sceglie il bersaglio |
| **click destro** o **Backspace** | annulla l'ultimo waypoint |
| **1** · **2** · **3** | abilità · **4** = **scatto** |
| **Spazio** | lock-in **oppure** salta il playback — vedi sotto |
| **Home** | ricentra sul centro della griglia e azzera lo zoom (`DefaultArmLength` 800) |
| **F** | centra sull'unità **selezionata**, mantenendo zoom e quota (`FocusOn`) |
| **R** | riavvia la partita — attivo **solo** a match concluso |

⚠️ **La camera non segue il click.** Selezionare un'unità **non** muove l'inquadratura: il focus è un gesto
esplicito (**`F`**), e `Home` e `F` sono due inquadrature diverse per scelta — centro mappa contro unità.
Cliccare un'**avversaria** non la seleziona nemmeno (`e' avversaria: seleziona prima una tua unita'`), quindi
non esiste un soggetto da inquadrare. Aspettativa emersa in seduta il 2026-08-14 e verificata sul codice:
`FocusOn` ha un solo chiamante, `OnFocusSelected`, legato a `EKeys::F`.

### ⚠️ Premere Spazio fino alla fine non è una run valida

**Spazio è contestuale, non un «avanti»** — `Player/RTPlayerController.cpp` (`OnLockIn`):

```cpp
// Durante il playback lo stesso tasto (Spazio) salta la risoluzione; altrimenti chiude la pianificazione.
if (TurnManager->IsResolving()) { TurnManager->SkipPlayback(); }
else { RefreshPlanningPreview(GetWorld(), nullptr); TurnManager->LockInAndResolve(); }
```

Premuto in sequenza chiude la pianificazione **e poi salta la risoluzione che ha appena avviato**. Tre
conseguenze, tutte misurate:

1. **Il team 0 è tuo.** `RTGameMode.cpp` assegna `bIsBotControlled = (TeamId == 1)`: solo il team 1 è
   pilotato dal bot. Senza ordini le tue due unità «restano ferme», e la partita diventa il bot che elimina
   due bersagli immobili.
2. **Il playback sparisce**, ed è l'oggetto di `-4` (scorrimento e centri cella), `-4b` (lo scatto scivola o
   compare?) e `-8` (salita di quota). Sono le voci che i test headless **non** possono coprire: saltarle
   lascia in piedi solo ciò che è già verde in automatico.
3. **Ritmo e score di quella run non valgono** per `-7` e `-10`: sono stati prodotti contro unità ferme.

**Resta legittimo un uso**: una run «tutto Spazio» chiude rapidamente per eliminazione, e serve a vedere
«PARTITA FINITA» a schermo e a provare **`R`** — un terzo di `-10`. Dichiarala come tale nell'esito, non come
la partita completa.

**E un caso in cui il salto È la verifica**: `PIE-FACING-1` chiede che, saltando il playback con Spazio,
l'orientamento finale sia lo **stesso** di quando lo si guarda. Servono due run, e il confronto.

---

## 2. `U2` — Partita hex, primo giro

`verifies`: `PIE-HEXPLAY-1` · `-4` · `-5` · `PIE-CAM-START`

| Voce | Stato oggi | Cosa resta da osservare |
|---|---|---|
| `-1` | ✅ 2026-08-10 | nulla di nuovo: si rilegge in U6 insieme alle altre |
| `-4` | ⏳ | **il playback**: le unità scorrono di cella in cella, e a fine playback ognuna è **esattamente** sul centro della cella finale — nessuna deriva accumulata |
| `-5` | ✅ 2026-08-10 | la prima metà è verificata. ⚠️ La seconda (scambio A↔B) è **dichiarata falsa** e rimossa: la pianificazione rifiuta il goal occupato, non c'è niente da osservare |
| `PIE-CAM-START` | ⏳ | la camera si apre sul **punto medio delle proprie unità**, non sul centro mappa, e più vicina di `Home`: `MatchStartArmLength` 450 contro `DefaultArmLength` 800 |

**Gesti**: guarda l'inquadratura d'apertura *prima* di toccare il mouse (è l'unico momento in cui
`PIE-CAM-START` è osservabile). Poi due unità pianificate verso la **stessa** cella, lock-in con **Spazio**.

**Log da catturare**:

```
Camera sulla squadra <TeamId> (<N> unità, arm=…)
RTUnit_2: fermo: cella contesa (q=..,r=..,L=..) (Action.Move)
```

Le coordinate devono essere **assiali** `(q,r,L)`. Se compaiono coppie cartesiane, è una regressione.

---

## 3. `U3` — Input e pianificazione

`verifies`: `PIE-HEXPLAY-2` · `-3` · `-3b` · `PIE-PREVIEW-PERSIST`

| Voce | Stato oggi | Cosa resta da osservare |
|---|---|---|
| `-2` | 🟡 | selezione, guardia sulle avversarie ed evidenziazione gialla sono ✅. **Resta il layer**: su celle sovrapposte, il layer viene dalla **quota del punto colpito** — cliccando la piattaforma si evidenzia la cella del layer 1, non quella sotto |
| `-3` | ✅ 2026-08-10 | verificata anche nel visivo (anteprima ciano coincidente col percorso eseguito) |
| `-3b` | 🟡 | «fuori portata» ✅. **Resta «coperto»**: serve una cella con `bBlocksLineOfSight`, che questa arena ha |
| `PIE-PREVIEW-PERSIST` | ⏳ | **difetto noto**: pianificato un attacco con Gadget su Riktor, selezionando *un'altra* unità il marcatore di fuoco amico e la zona arancione devono **restare** |

**Gesti**: seleziona un'unità e prova, in quest'ordine, una cella **valida**, una **oltre il budget**, una
**bloccata**, una **occupata**. Poi **click destro** (o `Backspace`) per annullare l'ultimo waypoint.
Infine clicca la piattaforma sul layer 1 — è il gesto che chiude `-2`.

**Log da catturare**:

```
Selezionata: RTUnit_1
… e' avversaria: seleziona prima una tua unita'
… costo 1/5 → 5/5            (il budget si spende cumulativamente)
RTUnit_3 fuori portata (max 3)
```

⚠️ «fuori portata» non deve **mai** uscire come «coperto» su un bersaglio senza muri in mezzo: è
esattamente il difetto che questa voce esiste per sorvegliare.

---

## 4. `U4` — Combat e linea di tiro

`verifies`: `PIE-HEXPLAY-6` · `-6b` · `-6c`

| Voce | Stato oggi | Cosa resta da osservare |
|---|---|---|
| `-6` | ⏳ | l'attacco attraverso il muro è scartato con «nessuna linea di tiro»; spostandosi di **una cella di lato** va a segno. Con l'ostacolo su un **altro layer** il tiro **passa** (regola di elevazione) |
| `-6b` | ⏳ | la **linea** colpisce anche chi sta sulla traiettoria prima del bersaglio; l'**area** colpisce bersaglio + 6 vicini. Si giudica la **leggibilità**: si capisce dove finirà il colpo *prima* di lanciarlo? |
| `-6c` | ⏳ | la spinta va lungo una delle **6 direzioni esagonali**, non su un asse cardinale; non si sposta se dietro c'è ostacolo, unità o bordo |

**La geometria dell'arena, letta da `MakeTestArena`** — non «cerca un muro», ma le celle esatte:

| Cosa | Celle |
|---|---|
| **Muro che blocca la VISTA** (attraversabile!) | `q=0`, `r` da **-2 a +2** → `(0,-2,0)` … `(0,2,0)` |
| Ostacoli al **movimento** | `(-1,2,0)` · `(1,-2,0)` · `(2,1,0)` |
| Fango, costo 3 | `q=-2`, `r` da -1 a +1 |
| Piattaforma su `L1` | `(2,-1,1)` · `(2,0,1)` · `(3,-1,1)` · `(3,0,1)` |
| Transizione (una sola) | `(1,0,0) → (2,0,1)`, costo **2** |
| Celle di partenza | estremi: `q=-4` (team 0, tuo) e `q=+4` (team 1, bot) |

**I tre gesti di `-6`**, nell'ordine:

1. **«Coperto»** — porta la tua unità a ridosso del muro, es. `(-1,0,0)`, con un nemico a `q=+1` e `r` simile.
   Arma con **1/2/3**, clicca il nemico. Atteso: `[RT] <Nome> coperto (nessuna linea di tiro)`.
   ⚠️ Se esce `fuori portata (max N)` sei troppo lontano: è il motivo *giusto* per un'altra domanda.
2. **«Di lato»** — il muro copre solo `r ∈ [-2,+2]`. Spostati a `r = 3` (o `-3`) e ritenta: la linea non
   incrocia il muro, il colpo parte.
3. **«Altro layer»** — sali sulla piattaforma e spara a terra oltre il muro. Il muro è tutto su `L0`: per la
   regola di elevazione **il tiro passa**. ⚠️ Serve la salita, che costa **due turni** da `q=-4`.

**Chi usare**: linea e area con **Gadget** (`LinearDischarge`, `Overload`) e **Phase** (`PressureJet`,
`CircularTide`). Per la spinta, **Phase** (`PressureJet`) o **Riktor** (`Ram`) — entrambe `Push 1`.

⚠️ **Due cose non sono verificabili qui, e non è un fallimento della seduta**: il **cono** (nessuna abilità
del roster v0.1 usa `ERTAbilityShape::Cone`) e l'**arresto anticipato contro un ostacolo a due celle**
(nessuna azione spinge più di una cella da quando `Guardian.Sweep` è sparita con gli archetipi). Restano
coperti headless. Il fuoco amico si giudica su `Overload`, **non** su `CircularTide`, che cura gli alleati.

**Log da catturare**: il combat log deve elencare **un colpo per bersaglio**, e i rifiuti devono portare il
reason code con coordinate assiali.

---

## 5. `U5` — Bot e HUD

`verifies`: `PIE-HEXPLAY-7` · `-9` · `PIE-AI-01`…`05`

| Voce | Stato oggi | Cosa resta da osservare |
|---|---|---|
| `-7` | 🟡 | il log utility in coordinate assiali è ✅, **anche sul layer 1**. **Resta il giudizio**: gli score hanno senso guardando la partita? La run precedente durava 5 round — «un lampo» — e non permetteva di giudicare |
| `-9` | ⏳ | HUD invariato (HP/scudo/energia, timer, fase, combat log); anteprima **ciano**, marker dei waypoint, preview scatto **magenta** e traccia **grigia** post-lock seguono i centri esagonali e coincidono col percorso eseguito; **`Home`** ricentra |
| `PIE-AI-01`…`05` | ⏳ | intent sempre legali; nessun rifiuto silenzioso; lo **score breakdown** spiega *perché* (copertura, letale, hazard), non un totale opaco |

**Taratura, se serve**: `TurnManager` nel **World Outliner** → *Details ▸ Refactor Tactics ▸ Bot* —
`WKill / WDamage / WThreat / WKiteViolation / WApproach`. Hanno effetto **dal turno successivo, senza
ricompilare**. I default vengono dal quadrato: su hex vanno riguardati, non dati per buoni. ⚠️ Se li
modifichi, **vanno committati**: è metà del `done_when` di U5.

**Log da catturare**:

```
RTUnit_2: utility -> (q=1,r=0,L0) score=-50
RTUnit_3: utility -> (q=2,r=-1,L1) score=-40      ← il bot valuta una cella sul layer 1
RTUnit_0: utility -> scatto (q=..,r=..,L0) + attacca RTUnit_1 score=180
```

---

## 6. `U6` — Multilivello e partita completa

`verifies`: `PIE-HEXPLAY-8` · `-10` · `-4b` · `PIE-FACING-1`
`done_when`: **le nove voci rilette tutte insieme**

| Voce | Stato oggi | Cosa resta da osservare |
|---|---|---|
| `-8` | 🟡 | coperto headless. **Resta il playback**: l'unità **sale di quota** (`LayerHeight`) attraversando la transizione, invece di scivolare sul piano |
| `-4b` | ⏳ | la fase Dash esiste e **precede** il Blast (già osservato). **Resta il visivo**: l'unità scivola lungo la traiettoria o **compare** sulla cella d'arrivo? Serve una run lenta |
| `-10` | 🟡 | **restano tre cose**: «PARTITA FINITA» a schermo, il riavvio con **`R`**, e una chiusura **per eliminazione** — la run del 2026-08-10 finì in pareggio a `round 5/5` e non vale più come misura |
| `PIE-FACING-1` | ⏳ | a fine playback la mesh **guarda dove guarda la regola**: chi si è mosso lungo l'ultimo passo, chi ha attaccato verso il bersaglio. Premendo **Spazio** per saltare il playback l'esito è lo **stesso** |

**Log da catturare**:

```
Fase Dash: N scatti
Playback fase: Dash        →  Playback fase: Blast      (l'ordine è la verifica)
Eliminata: RTUnit_1 (team 0)
Partita finita: <esito> (round n/12, obiettivo x-y, formato Format.Skirmish2v2)
```

⚠️ **Se la partita chiude per scadenza dei round invece che per eliminazione, non è un fallimento.** È un
numero per **G11** e, se si ripete, una issue di bilanciamento — esattamente come accadde il 2026-08-10, dove
il playtest falsificò `RoundLimit 5` e fece cambiare il formato in giornata. La DoD riscritta chiede tre run
prima di trarne una conclusione.

---

## 7. La rilettura finale

`U6.done_when` non chiede «ogni voce spuntata quando la si esegue»: chiede le **nove rilette tutte insieme**,
alla fine. È la differenza fra una checklist e un verdetto — nove verifiche passate in momenti diversi non
dimostrano che la partita *regga*, che è ciò che M6.8 esiste per stabilire.

Le nove del verdetto: `-1 -2 -3 -4 -5 -6 -7 -8 -9`.
Le altre cinque del registro — `-3b -4b -6b -6c -10` — restano voci vive: 🟡 è ammesso **con la ragione
scritta accanto**.

---

## 8. Come si registra un esito

Nel registro, nella colonna di stato della voce, con **la data e l'evidenza**, non con un simbolo solo. La
forma che il file già usa:

> ✅ **2026-08-10** — confermato a schermo: […]. Run su `MapSource=GeneratedTestArena`
> (`Board 2v2 esagonale avviata su 65 celle con 4 eroi`). Coperto headless da `RefactorTactics.…`

Regole che il registro applica già, e che valgono anche qui:

- **il file di log non si allega** — `Saved/` non è versionato. Si incollano le **righe**, che sopravvivono;
- **una metà verificata non fa una voce verde**: 🟡 con scritto cosa manca vale più di un ✅ ottimista;
- **se un esito atteso si rivela falso, si corregge la voce** invece di lasciarla inchiodata: è ciò che è
  successo a `-5` (scambio A↔B) e a `-6c` (`Guardian.Sweep`). Un esito atteso falso impedisce a una voce di
  chiudersi per sempre;
- **`ensure` / `check` / crash** nel log vanno riportati anche se la voce passa: sono il criterio 5 della DoD.

---

## 9. Cosa il PIE aggiunge, e cosa non deve rifare

Quasi tutta la **regola** è già coperta headless. Quello che queste sedute aggiungono è ciò che un test non
vede: **fluidità, quota, colore, leggibilità, ritmo**. Rifare a mano ciò che è già verde in automatico è
tempo speso male — e peggio, produce un ✅ che sembra più forte di quello che è.

| La voce guarda | Già coperto headless da |
|---|---|
| `-4` playback e centri cella | `HexMove.UnitReachesPlannedCell` |
| `-5` contesa | `HexMove.ContestedCellStopsBoth` |
| `-3b` portata contro copertura | `Combat.HexTargetingReasonDistinguishesRangeFromCover`, `HexCombat.NoMapFailClosed` |
| `-6` linea di tiro | `HexBlast.NoLineOfSightOnHexMap` |
| `-6b` forme | `HexCombat.Shape*`, `Preview.HitCellsMatchCombatShape` |
| `-6c` spinta | `HexCombat.KnockbackPushesAway` · `…StopsBeforeObstacle` · `…StopsAtMapEdge` · `…NoPushCases` · `…PreservesLayer`; sul turno vero `TurnLog.OpposingPushesLeaveATrace` e `TurnLog.ContestedPushDestinationLeavesATrace` |
| `-7` bot | `HexBotPlay.*` |
| `-8` transizione fra layer | `HexMove.ClimbsOnlyThroughTransition`, `Structures.Bridge.RemovalBreaksPath`, `…NoTeleportOnRemoval` |
| `-10` tenuta e invarianti | `HexMatch.PlaysToCompletion` |
| `-4b` scatto | `HexMove.DashReachesCellOnHex`, `DashRejectsOutOfBudget` |

**Misurati sul codice**, non trascritti: dodici dei tredici nomi che il registro cita esistono in
`Source/RefactorTactics/Tests/`.

🔴 **Il tredicesimo no.** Alla voce `PIE-HEXPLAY-6c` il registro dichiara *«Coperto headless
(`HexCombat.Knockback*`, `HexBlast.KnockbackOnHexGrid`)»* — e **`HexBlast.KnockbackOnHexGrid` non esiste**:

```sh
grep -rn "KnockbackOnHexGrid" Source/ | wc -l                       # 0
# i tre test che HexBlast ha davvero:
grep -rno 'RefactorTactics\.HexBlast\.[A-Za-z0-9]*' Source/         # AttackDealsDamageOnHexMap
                                                                    # NoLineOfSightOnHexMap · OutOfHexRange
```

⚠️ **È un riferimento morto, non un buco di copertura** — e la distinzione conta, perché la prima cosa
sembra la seconda. La spinta *è* coperta: cinque test sulla geometria (`HexCombat.Knockback*`) e cinque sul
turno vero (`TurnLog.OpposingPushesLeaveATrace`, `…ContestedPushDestinationLeavesATrace`,
`…PushWithoutDestinationLeavesATrace`, `…PushAndPullKeepTheirOwnCause`, `…PushedUnitFacesThePusherInPlay`).
Chi possiede il registro corregga il nome; questa seduta non lo tocca, perché quel file non è nel suo
write-set.

**Regola generale**: prima di citare un test come prova dentro un esito, `grep -rn "<NomeTest>" Source/`.
Un nome trascritto da un documento è un'affermazione di seconda mano.
