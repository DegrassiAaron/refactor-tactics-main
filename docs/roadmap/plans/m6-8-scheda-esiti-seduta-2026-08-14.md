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

| Campo | Atteso | Osservato |
|---|---|---|
| Data e ora della run | — | |
| `MapSource` | `GeneratedTestArena` (il default è `LevelAsset`: **va cambiato**) | |
| Riga di avvio | `[RT] Board 2v2 esagonale avviata su N celle con 4 eroi` | |
| `RoundLimit` nell'HUD | **12** — se legge `n/5` la build è vecchia: **fermati** | |
| Formato | `Format.Skirmish2v2` | |
| Percorso del log | `Saved/Logs/….log` (non si allega: si incollano le righe) | |

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
- Esito:
- Evidenza:
- Resta:

**`PIE-HEXPLAY-4`** — playback del movimento · *partenza* ⏳
Le unità scorrono di cella in cella; a fine playback ognuna è **esattamente** sul centro della cella finale,
nessuna deriva accumulata. Il test headless (`HexMove.UnitReachesPlannedCell`) copre l'arrivo, non la fluidità.
- Esito:
- Evidenza:
- Resta:

**`PIE-HEXPLAY-5`** — collisione simultanea · *partenza* ✅ (2026-08-10)
Prima metà verificata. ⚠️ La seconda (scambio A↔B) è **dichiarata falsa** nel registro: non è pianificabile,
quindi non c'è nulla da osservare. Non riaprirla.
- Esito:
- Evidenza:
- Resta:

**`PIE-CAM-START`** — apertura della camera · *partenza* ⏳
Centrata sul **punto medio delle proprie unità**, non sul centro mappa; braccio `MatchStartArmLength` 450
contro `DefaultArmLength` 800. **Osservabile solo nel primo istante**, prima di toccare il mouse.
- Esito:
- Evidenza (`Camera sulla squadra <TeamId> (<N> unità, arm=…)`):
- Resta:

### `U3` — Input e pianificazione

**`PIE-HEXPLAY-2`** — selezione e cella sotto il cursore · *partenza* 🟡
Selezione, guardia sulle avversarie ed evidenziazione gialla sono già ✅. **Manca solo il layer**: cliccando
la piattaforma si evidenzia la cella del **layer 1**, non quella sotto con gli stessi `q,r`.
- Esito:
- Evidenza:
- Resta:

**`PIE-HEXPLAY-3`** — pianificazione entro budget · *partenza* ✅ (2026-08-10)
Verificata anche nel visivo. Si rilegge in U6.
- Esito: 🟡 **parziale, 2026-08-14** — riferito dall'utente: *«si muovono, vanno sui waypoint»*. La parte
  della voce che riguarda i waypoint e il percorso **regge**. ⚠️ Non è un ✅: rifiuti (oltre budget /
  bloccata / occupata / fuori mappa), undo e budget cumulativo **non sono stati esercitati** in questa
  osservazione, e la voce li chiede tutti.
- Evidenza: da raccogliere — servono le righe `costo n/m` e almeno un rifiuto col motivo
- Resta: i quattro rifiuti, l'undo, il costo cumulativo. Vedi anche `OSS-1` (#877), che nasce da questa
  stessa osservazione ma **non** appartiene a questa voce.

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
- Esito:
- Evidenza (`RTUnit_2: utility -> (q=1,r=0,L0) score=-50`):
- Resta:

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
- Esito:
- Evidenza (`Fase Dash: N scatti` → `Playback fase: Dash` → `Playback fase: Blast`):
- Resta:

**`PIE-HEXPLAY-10`** — partita completa · *partenza* 🟡
**Mancano tre cose**: «PARTITA FINITA» a schermo, il riavvio con **`R`**, e una chiusura **per
eliminazione**. ⚠️ Se chiude per scadenza dei round **non è un fallimento**: è un numero per G11. La DoD
chiede **tre run** prima di trarne una conclusione.
- Esito run 1:
- Esito run 2:
- Esito run 3:
- Evidenza (`Partita finita: <esito> (round n/12, obiettivo x-y, formato Format.Skirmish2v2)`):
- Resta:

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

### `OSS-2` — …

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
