# M6.8 — scheda di raccolta esiti (`U2`–`U6`)

> `SNAPSHOT` · **Aperta**: 2026-08-14 · **Stato**: ⏳ in attesa della run
> **Cosa è**: un **buffer con scadenza**. Le note di una seduta di playtest che non può scrivere nel proprio
> registro, perché in questo batch `docs/technical/test-manuali-pie.md` è nel `writable` della track
> `content_editor` (#451) e per **D-139** una track non scrive fuori dal proprio write-set.
> **Cosa NON è**: una seconda fonte di verità. L'owner del verdetto resta e resta solo
> [`../../technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md). Finché questa scheda esiste,
> **il repository ha due posti dove leggere lo stesso stato** — che è il difetto che questo progetto ha già
> pagato due volte (le sessioni A–G contro le sedute `U*`, #371). Si accetta qui solo perché è **dichiarato,
> datato e con una condizione di morte scritta**.
>
> **Condizione di morte**: al primo riversamento nel registro questa scheda si **archivia**, non si aggiorna.
> Se ti trovi a modificarla dopo che gli esiti sono nel registro, il difetto è già successo.
>
> **Procedura**: [`m6-8-sequenza-sedute-u2-u6-2026-08-14.md`](m6-8-sequenza-sedute-u2-u6-2026-08-14.md) —
> preparazione, gesti, log da catturare. **Perché la DoD è quella che è**:
> [`m6-8-playtest-hex-spec-panel-2026-08-14.md`](m6-8-playtest-hex-spec-panel-2026-08-14.md).

---

## 0. Intestazione della run — da compilare per prima

Questi sei campi decidono se la run **conta**. Se uno solo è sbagliato, gli esiti sotto non valgono e vanno
rifatti: meglio scoprirlo adesso che dopo quattordici voci.

| Campo | Atteso | Osservato — **run del 2026-08-14** |
|---|---|---|
| Data e ora della run | — | 2026-08-14, log fino alle **20:08** |
| `MapSource` | `GeneratedTestArena` (il default è `LevelAsset`: **va cambiato**) | ✅ `MapSource=GeneratedTestArena: uso la mappa di PROVA generata (65 celle, con ostacoli, muri, terreno costoso e piattaforma). La mappa del livello e' ignorata.` |
| Riga di avvio | `[RT] Board 2v2 esagonale avviata su N celle con 4 eroi` | ✅ `Board 2v2 esagonale avviata su 65 celle con 4 eroi` |
| `RoundLimit` | **12** — se legge `n/5` la build è vecchia: **fermati** | ✅ `Formato di partita in vigore: 'Format.Skirmish2v2' (RoundLimit 12, soglia obiettivo 0, 2 unita' per squadra)` |
| Formato | `Format.Skirmish2v2` | ✅ formato **spedito** (nessun `URTMatchFormatData` assegnato al GameMode) |
| Percorso del log | `Saved/Logs/….log` (non si allega: si incollano le righe) | `Saved/Logs/RefactorTactics_2.log` (279 KB) |

**I sei campi passano: questa run conta.** ⚠️ Ma **non è arrivata a conclusione** — zero `Eliminata:` e zero
`Partita finita` nel log — quindi tutto ciò che dipende dalla fine partita resta aperto.

⚠️ **Le unità sono cilindri**: i `BP_Unit_*` non esistono in `Content/`, il fallback è previsto e non è un
difetto da registrare.

⚠️ **Il team 0 è tuo** (`bIsBotControlled = (TeamId == 1)`): senza ordini le tue unità restano ferme. E
**Spazio è contestuale** — durante il playback lo **salta**, fuori chiude la pianificazione. Una run
«tutto Spazio» non è la partita completa: vale solo per «PARTITA FINITA» e il riavvio con `R`. Dettaglio e
codice nella sequenza, §1.

| Tipo di questa run | |
|---|---|
| ☐ completa — ordini al team 0, playback **guardato** | |
| ☐ «tutto Spazio» — degenerata, vale solo per fine partita e `R` | |
| ☐ confronto `PIE-FACING-1` — stessa situazione, una guardata e una saltata | |

---

## 1. Le quattordici voci

Formato di ogni blocco: **stato di partenza** (misurato sul registro il 2026-08-14) · **cosa deve mostrare
questa run** · i tre campi da riempire. Un campo lasciato vuoto vale «non osservato», che è un esito
legittimo: `⏳ non raggiunta in questa run` è informazione, un ✅ ottimista no.

### `U2` — Partita hex, primo giro

**`PIE-HEXPLAY-1`** — allestimento · *partenza* ✅ (2026-08-10)
Serve solo alla rilettura finale di U6. Se qualcosa è regredito, va scritto qui.
- Esito: ✅ **riconfermato 2026-08-14** — nessuna regressione: 65 celle, 4 eroi, arena di prova.
- Evidenza: `Board 2v2 esagonale avviata su 65 celle con 4 eroi`
- Resta: nulla.

**`PIE-HEXPLAY-4`** — playback del movimento · *partenza* ⏳
Le unità scorrono di cella in cella; a fine playback ognuna è **esattamente** sul centro della cella finale,
nessuna deriva accumulata. Il test headless (`HexMove.UnitReachesPlannedCell`) copre l'arrivo, non la fluidità.
- Esito:
- Evidenza:
- Resta:

**`PIE-HEXPLAY-5`** — collisione simultanea · *partenza* ✅ (2026-08-10)
Prima metà verificata. ⚠️ La seconda (scambio A↔B) è **dichiarata falsa** nel registro: non è pianificabile,
quindi non c'è nulla da osservare. Non riaprirla.
- Esito: ✅ **riconfermata 2026-08-14** — due unità fermate nello stesso turno, reason code e coordinate
  **assiali**, con la novità del suffisso di priorità `p50`.
- Evidenza:
  ```
  RTUnit_2: fermo: cella contesa (q=4,r=0,L=0) (Action.Move, p50)
  RTUnit_3: fermo: cella contesa (q=4,r=-1,L=0) (Action.Move, p50)
  ```
- Resta: nulla su questa run.

**`PIE-CAM-START`** — apertura della camera · *partenza* ⏳
Centrata sul **punto medio delle proprie unità**, non sul centro mappa; braccio `MatchStartArmLength` 450
contro `DefaultArmLength` 800. **Osservabile solo nel primo istante**, prima di toccare il mouse.
- Esito: 🟡 **2026-08-14** — la metà misurabile è ✅, ed è proprio la riga che la voce prescrive: braccio a
  **450** (non 800) e squadra **propria** (team 0, 2 unità).
- Evidenza: `Camera sulla squadra 0 (2 unita', arm=450)`
- Resta: la metà visiva — che l'inquadratura sia sul **punto medio delle proprie unità** e non sul centro
  della mappa. Il log dice il braccio, non dove punta.

### `U3` — Input e pianificazione

**`PIE-HEXPLAY-2`** — selezione e cella sotto il cursore · *partenza* 🟡
Selezione, guardia sulle avversarie ed evidenziazione gialla sono già ✅. **Manca solo il layer**: cliccando
la piattaforma si evidenzia la cella del **layer 1**, non quella sotto con gli stessi `q,r`.
- Esito: 🟡 **avanzata molto, 2026-08-14 (log `_3`)** — la **guardia** sulle avversarie è ✅ (3 occorrenze), e
  **il layer risolve**: un click sulla piattaforma ha prodotto un waypoint sulla cella `L1`, non su quella
  con gli stessi `q,r` sotto. È la metà logica della voce, ed è la parte che tiene aperta la 🟡 dal 6 agosto.
- Evidenza:
  ```
  RTUnit_3 e' avversaria: seleziona prima una tua unita' per bersagliarla     ← 3 occorrenze
  Waypoint (3,-1,L1) rifiutato: oltre il budget (gia' spesi 0 di 5) per RTUnit_0
  ```
  ⚠️ Il waypoint su `L1` è stato **rifiutato per budget** con «già spesi 0 di 5»: la piattaforma costava più
  di 5 dal punto di partenza. Non è un difetto — ma significa che **nessuna unità ci è salita**, quindi
  `PIE-HEXPLAY-8` (salita di quota nel playback) resta senza osservazione.
- Resta: la conferma **visiva** che l'evidenziazione gialla stia sulla cella del layer giusto. Il log prova
  che il raycast risolve la quota, non che il contorno si disegni dove deve.

**`PIE-HEXPLAY-3`** — pianificazione entro budget · *partenza* ✅ (2026-08-10)
Verificata anche nel visivo. Si rilegge in U6.
- Esito: 🟡 **parziale, 2026-08-14 — misurato sul log, non riferito.** Reggono: **budget cumulativo**,
  **due rifiuti su quattro** col motivo giusto, **undo**, e il *«piano precedente resta intatto»* (dopo un
  rifiuto il conteggio riprende da dove era). ⚠️ **Due rifiuti su quattro non sono mai stati provocati**:
  `cella occupata` e `cella fuori dalla mappa` hanno **0 occorrenze** nel log.
- Evidenza:
  ```
  Piano: RTUnit_1 -> 1 waypoint (costo 3/5)      ← cumulativo, tre passi
  Piano: RTUnit_1 -> 2 waypoint (costo 4/5)
  Piano: RTUnit_1 -> 3 waypoint (costo 5/5)
  Waypoint (0,0,L0)   rifiutato: oltre il budget (gia' spesi 4 di 5) per RTUnit_1   ← 5 occorrenze
  Waypoint (1,-2,L0)  rifiutato: cella bloccata (RTUnit_1)                          ← 5 occorrenze
  Annullato waypoint: RTUnit_1 -> N waypoint                                        ← 2 occorrenze
  ```
  ➕ **Seconda run, log `RefactorTactics_3.log` (20:17)** — il terzo rifiuto è arrivato:
  ```
  Waypoint (4,0,L0)  rifiutato: cella occupata da un'altra unita' (RTUnit_0)        ← 5 occorrenze
  ```
- Resta: **un solo gesto** — cliccare una cella **fuori dalla mappa**. `rifiutato: cella fuori dalla mappa`
  ha ancora **0** occorrenze su entrambi i log. Poi la voce chiude.
  Vedi anche `OSS-1` (#877), che nasce da questa stessa osservazione ma **non** appartiene a questa voce.

**`PIE-HEXPLAY-3b`** — il rifiuto dice il motivo giusto · *partenza* 🟡
«fuori portata (max N)» è ✅. **Manca «coperto»**: serve il bersaglio dietro la cella con
`bBlocksLineOfSight`, che questa arena ha.
- Esito:
- Evidenza (`RTUnit_3 fuori portata (max 3)` / «coperto (nessuna linea di tiro)»):
- Resta:

**`PIE-PREVIEW-PERSIST`** — l'avviso di fuoco amico sopravvive al cambio di selezione · *partenza* ⏳ (difetto noto)
Pianificato l'attacco di Gadget su Riktor, selezionando **un'altra** unità il marcatore e la zona arancione
devono **restare** fino al lock-in.
- Esito:
- Evidenza:
- Resta:

### `U4` — Combat e linea di tiro

**`PIE-HEXPLAY-6`** — LOS esagonale · *partenza* ⏳
Attraverso il muro: scartato con «nessuna linea di tiro». Di **una cella di lato**: va a segno. Ostacolo su
un **altro layer**: il tiro **passa** (regola di elevazione).
- Esito:
- Evidenza:
- Resta:

**`PIE-HEXPLAY-6b`** — forme d'attacco · *partenza* ⏳
Linea (`Gadget.LinearDischarge`, `Phase.PressureJet`) colpisce anche chi sta sulla traiettoria; area
(`Gadget.Overload`, `Phase.CircularTide`) colpisce bersaglio + 6 vicini. Il giudizio è sulla **leggibilità**:
si capisce dove finirà il colpo *prima* di lanciarlo? Fuoco amico su `Overload`, **non** su `CircularTide`,
che cura gli alleati. ⚠️ Il **cono** non è verificabile: nessuna abilità del roster usa `ERTAbilityShape::Cone`.
- Esito:
- Evidenza (un colpo per bersaglio nel combat log):
- Resta:

**`PIE-HEXPLAY-6c`** — spinta · *partenza* ⏳
`Phase.PressureJet` o `Riktor.Ram`, entrambe **Push 1**. Direzione lungo una delle **6** esagonali; nessuno
spostamento se dietro c'è ostacolo, unità o bordo; spinte opposte si annullano. ⚠️ L'arresto contro un
ostacolo a **due** celle non è più osservabile in partita (nessuna azione spinge oltre una cella).
- Esito:
- Evidenza:
- Resta:

### `U5` — Bot e HUD

**`PIE-HEXPLAY-7`** — bot su hex · *partenza* 🟡
Il log utility in coordinate assiali è ✅, **anche sul layer 1**. **Manca il giudizio**: gli score hanno senso
guardando la partita? La run precedente durava 5 round — «un lampo» — e non permetteva di giudicare. Con
`RoundLimit 12` questa è la prima occasione vera.
- Esito: 🟡 — quattro valutazioni, coordinate assiali, e **tre forme di mossa distinte**: scatto composto,
  carica e riposizionamento. ⚠️ Tutte su **`L0`**: in questa run il bot non ha valutato la piattaforma
  (nella run del 2026-08-10 sì, con `(q=2,r=-1,L1)`).
- Evidenza:
  ```
  RTUnit_2: utility -> scatto (q=0,r=0,L0) + attacca RTUnit_1 score=100
  RTUnit_3: utility -> CARICA su RTUnit_1 (impatto da (q=1,r=-1,L0)) score=-10
  RTUnit_2: utility -> (q=0,r=0,L0) attacca RTUnit_0 score=-10
  RTUnit_3: utility -> (q=-1,r=-1,L0) score=-30
  ```
  ➕ **Seconda run (log `_3`): il layer 1 è tornato nelle valutazioni**, e non solo come cella di passaggio —
  il bot ci ha visto un attacco:
  ```
  RTUnit_3: utility -> (q=2,r=-1,L1) score=-10
  RTUnit_3: utility -> (q=2,r=-1,L1) attacca RTUnit_0 score=70
  Pesi bot: WKill=10000 WDamage=10 WThreat=100 WKiteViolation=50 WApproach=10 WElevation=20
  ```
  ⚠️ I pesi in vigore sono i **default**, `WElevation` compreso: non sono stati ritarati in questa seduta, e
  il `done_when` di U5 chiede che se li si modifica vengano committati.
- Resta: **il giudizio sul comportamento** su una partita intera — sei decisioni non bastano a dire se gli
  score hanno senso.

**`PIE-HEXPLAY-9`** — HUD e anteprima piani · *partenza* ⏳
Barre HP/scudo/energia, timer, fase, combat log invariati; anteprima **ciano**, marker waypoint, preview
scatto **magenta**, traccia **grigia** post-lock sui centri esagonali e coincidenti col percorso eseguito;
reason code con coordinate assiali; **`Home`** ricentra.
- Esito:
- Evidenza:
- Resta:

**`PIE-AI-01`…`05`** — comportamento del bot · *partenza* ⏳ (cinque voci)
Intent sempre legali, nessun rifiuto silenzioso, e lo **score breakdown** che spiega *perché* — copertura,
letale, hazard — invece di un totale opaco.
- Esito (`01` legali · `02` obiettivo senza contatto · `03` letale vs posizione · `04` cover vs esposizione · `05` hazard):
- Evidenza:
- Resta:

### `U6` — Multilivello e partita completa

**`PIE-HEXPLAY-8`** — movimento via arco · *partenza* 🟡
**Manca il playback**: l'unità **sale di quota** (`LayerHeight`) attraversando la transizione, invece di
scivolare sul piano. ⚠️ «Rimuovi l'arco» **non si fa qui**: la transizione è generata da codice.
- Esito:
- Evidenza:
- Resta:

**`PIE-HEXPLAY-4b`** — scatto su hex · *partenza* ⏳
La fase Dash esiste e **precede** il Blast (già osservato). **Manca il visivo**: l'unità scivola lungo la
traiettoria o **compare** sulla cella d'arrivo? Serve una run lenta.
- Esito: 🟡 **2026-08-14** — l'**ordine delle fasi** è riconfermato dal log, ed è la metà verificabile senza
  guardare. ⚠️ Gli scatti osservati sono dei **bot** (team 1): lo scatto pianificato dal giocatore (tasto
  **4**) non compare in questa run.
- Evidenza:
  ```
  Scatto: RTUnit_2 -> (q=0,r=0,L0)   ·   Scatto: RTUnit_3 -> (q=1,r=-1,L0)   ·   Fase Dash: 2 scatti
  Playback fase: Dash  →  Playback fase: Blast  →  Playback fase: Move
  ```
- Resta: il **visivo** (scivola o compare?) e uno scatto **pianificato dal giocatore**.

**`PIE-HEXPLAY-10`** — partita completa · *partenza* 🟡
**Mancano tre cose**: «PARTITA FINITA» a schermo, il riavvio con **`R`**, e una chiusura **per
eliminazione**. ⚠️ Se chiude per scadenza dei round **non è un fallimento**: è un numero per G11. La DoD
chiede **tre run** prima di trarne una conclusione.
- Esito run 1 (log `_2`): ⏳ **non arrivata a conclusione** — zero `Eliminata:`, zero `Partita finita`. Run
  interrotta, non un esito negativo.
- Esito run 2 (log `_3`): 🟡 **la partita è finita, e col limite nuovo** — ma **ancora per scadenza dei
  round**, non per eliminazione:
  ```
  Eliminata: RTUnit_1 (team 0)
  Partita finita: Pareggio - allo scadere dei round (round 12/12, obiettivo 0-0, formato Format.Skirmish2v2)
  ```
  ⚠️ **Questo è un numero, e va portato a G11.** Il dato di riferimento della voce dice che *bot contro bot*
  la partita si decide al **turno 10**, dentro il limite. Qui, con un umano al comando del team 0, si è
  arrivati a **12/12** con una sola eliminazione e nessuna decisione. Due run su due chiudono per scadenza —
  la prima con `RoundLimit 5`, questa con **12**, quindi il limite non è più la spiegazione.
- Esito run 3: — *(la DoD ne chiede tre prima di trarre una conclusione)*
- Resta: una chiusura **per eliminazione**, «PARTITA FINITA» a schermo e il riavvio con **`R`**.

**`PIE-FACING-1`** — l'orientamento visto è quello della regola · *partenza* ⏳
A fine playback la mesh **guarda dove guarda la regola**: chi si è mosso lungo l'ultimo passo, chi ha
attaccato verso il bersaglio. Premendo **Spazio** per saltare il playback l'esito è lo **stesso**.
- Esito:
- Evidenza:
- Resta:

---

## 1-bis. Osservazioni fuori voce

Cose viste in partita che **nessuna delle quattordici voci copre**. Non sono esiti: sono lacune del registro,
e vanno da qualche parte prima di essere dimenticate.

### `OSS-1` — il ventaglio verde non si restringe coi waypoint → **#877**

**Osservato il 2026-08-14**: *«si muovono, vanno sui waypoint, ma l'area dove possono arrivare non cambia»*.
Il percorso ciano si allunga e il costo cresce nel log; il ventaglio di celle raggiungibili resta identico.

**Non falsifica nessuna voce.** `PIE-HEXPLAY-3` chiede budget cumulativo e anteprima del **percorso** che si
accorcia — entrambi funzionano. `PIE-PREVIEW-AREA` (✅ dal 2026-08-09) parla del **fango** che accorcia il
raggio, non dei waypoint. È una **lacuna di specifica**, e il codice implementava una delle due letture
possibili senza che nessuno l'avesse scelta.

**Causa, misurata sul codice** — due strati, e il primo da solo non basta:
1. `HandleClickOnCell` e `RebuildPlannedPath` aggiornano `SetPreviewPath` ma non chiamano mai
   `RefreshPlanningPreview`, quindi `SetPreviewReachableCells` non si riaggiorna;
2. anche chiamandolo, `MakeCurrentSnapshot` (`RTTurnManager.cpp:4661`) fotografa `Unit->Cell` e
   `GetEffectiveMoveRange()` — posizione reale e budget pieno — quindi `ReachableCells` restituirebbe lo
   stesso insieme byte per byte.

**Deciso in sessione dall'autore**: il fan **si restringe** e mostra il residuo. Aperta **#877** con DoD,
approccio e test. ⚠️ `Player/` non è nel `writable` di nessuna track: chi la prende dichiara il write-set
prima (**D-139**).

### `OSS-2` — il click sull'avversario non muove la camera → **nessun difetto**

**Osservato il 2026-08-14**: *«il click sull'avversario non ha modificato la posizione della cam»*.

**Verificato sul codice: è il comportamento previsto, e l'aspettativa era doppia.** `FocusOn` ha **un solo
chiamante** — `ARTPlayerController::OnFocusSelected` — legato a **`EKeys::F`**: la camera non segue la
selezione, si centra su comando. E cliccare un'avversaria **non seleziona** (`RTUnit_3 e' avversaria:
seleziona prima una tua unita' per bersagliarla`, 3 occorrenze nel log), quindi non c'era neppure un soggetto
da inquadrare. Nella run: `Focus su …` **0 occorrenze** — `F` non è mai stato premuto.

⚠️ **Da dove nasce l'equivoco, che è la parte utile.** Il corpo di `#865` (chiusa) scrive che
*«`RTPlayerController` usa `FocusOn` **sulla selezione**»*: vero che il soggetto è l'unità selezionata, falso
che l'innesco sia la selezione. Una riga imprecisa in una issue chiusa produce, un mese dopo, un difetto
riferito che non esiste. Non la correggo — non è di questa seduta — ma la sequenza operativa ora dichiara
`F` e il divieto di aspettarsi che la camera segua il click.

**Nessuna issue aperta**: non c'è niente da riparare. Se si volesse *cambiare* il design — camera che segue
la selezione — sarebbe una decisione, non un fix, e andrebbe posta come tale.

### `OSS-3` — `F` porta la vista fuori dalla mappa → **#887**

**Osservato il 2026-08-14**, dopo `OSS-2`: premuto `F` come previsto, *«salta via, fuori dalla mappa»*. E il
secondo sintomo riferito — *«forse la selezionavo, non si capiva»* — **non era un difetto di selezione**: era
la conseguenza di aver perso l'inquadratura. Un difetto solo, che ne produceva due apparenti.

**Causa, per lettura del codice**: `RecenterView` (Home) calcola la quota — `AxialToWorld` dà
`Z = Origin.Z + Layer * LayerHeight` — mentre `FocusOn` (F, e `FrameOwnTeam`) **conserva la `Z` corrente del
pawn**. Quella `Z` nasce al **PlayerStart** (`DefaultPawnClass = ARTCameraPawn`) e nessuno la corregge mai:
`FrameOwnTeam` passa da `FocusOn`, che scarta la quota del centroide; il pan produce delta con `Z = 0`; zoom e
yaw non la toccano. **`Home` è l'unico che la stabilisce**, ed è per questo che il difetto si vede su `F`.

⚠️ **Il test che #865 prescriveva avrebbe cementato il difetto**: chiedeva di verificare che `FocusOn`
*conserva* la quota — un test che passa anche quando la quota conservata è sbagliata. La domanda giusta non è
«resta invariata?» ma «invariata **rispetto a cosa**?».

⚠️ **Per riprodurre non premere `Home` prima**: da lì in avanti la quota è corretta e `F` si comporta bene
fino al ricaricamento del livello. È anche il motivo per cui il difetto è sopravvissuto a diverse sedute.

**Limite dichiarato**: diagnosi per lettura, non misurando la `Z` a runtime — il log non la stampa e il
PlayerStart vive in un `.umap`. La struttura del difetto non dipende dal valore preciso; il test della DoD
di #887 la falsifica in un verso o nell'altro.

---

## 2. La rilettura finale (`U6.done_when`)

Non è la somma delle spunte qui sopra: è la domanda se la partita **regga**, rileggendo le nove del verdetto
— `-1 -2 -3 -4 -5 -6 -7 -8 -9` — tutte insieme, alla fine.

- Verdetto:
- Cosa ha retto:
- Cosa no:

---

## 3. Anomalie e crash

Criterio 5 della DoD: `ensure`, `check` o crash vanno riportati **anche se la voce passa**.

- [ ] Nessun `ensure` / `check` / crash nel log
- Righe, se presenti:

---

## 4. Come si riversa

Quando l'assegnazione di `test-manuali-pie.md` è chiara — cioè quando una track ne ha il write-set — ogni
blocco qui sopra diventa **una cella** nella colonna di stato della voce corrispondente, nella forma che il
registro già usa:

> ✅ **<data>** — confermato a schermo: <esito>. Run su `MapSource=GeneratedTestArena`
> (`Board 2v2 esagonale avviata su N celle con 4 eroi`). ⏳ resta <residuo>.

Regole del registro che valgono al riversamento:

- **una metà verificata non fa una voce verde**: 🟡 con scritto cosa manca vale più di un ✅ ottimista;
- **se un esito atteso si rivela falso, si corregge la voce**, non si lascia inchiodata — è già successo a
  `-5` (scambio A↔B) e a `-6c` (`Guardian.Sweep`);
- **i conteggi aggregati del registro si rimisurano col comando**, non si incrementano a mano: cambiare lo
  stato di una voce sposta il totale del subset `RELEASE-V01` e lo *«Stato in numeri»* in testa al file.

⚠️ Al riversamento va corretto anche il nome morto trovato scrivendo la sequenza: `PIE-HEXPLAY-6c` cita
`HexBlast.KnockbackOnHexGrid`, che **non esiste**. La copertura c'è sotto altri nomi
(`HexCombat.Knockback*`, `TurnLog.*Push*`): è un riferimento da correggere, non copertura da aggiungere.

**Poi questa scheda si archivia.** Vedi la condizione di morte in testa.
