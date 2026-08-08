# ADR-0005 — Orientamento: il facing come stato di gioco derivato dal movimento

> `CANONICAL` · **Stato**: Accettato — da implementare (E16) · **Data**: 2026-08-07 · **Decisore**: utente (dev singolo)
>
> ⚠️ **Emendamento 2026-08-08 — [D-020](RT_PDR_00_Decision_Log.md)**: le §1 e §2 dicevano che il facing si
> aggiorna **solo** al termine del `Move`. Non è più così: un'azione con bersaglio o direzione **orienta
> l'unità prima di risolvere**. Vedi la nuova **§2-bis**, che nomina i sei punti della timeline. Il resto
> dell'ADR — facing come stato di gioco, arco frontale unico, determinismo e privacy — resta invariato.
> **Contesto sorgente**: `docs/archive/src/design/action-ghosts-fasi-fast-reactions.md` §17 (campo `Facing`
> nel view model) e [`brief-planning-visuale.md`](../technical/brief-planning-visuale.md) §C5, che registrava il punto come aperto.
> **Estende**: [ADR-0003](adr-0003-modello-azioni-v01.md) (stili di movimento) · [ADR-0004](adr-0004-finestre-di-reazione.md) (reazioni direzionali)

## Contesto

Il facing esiste già nel progetto, ma **solo come presentazione**: `ARTUnit::SetVisualLocation` orienta la
mesh verso la direzione dello spostamento visivo (yaw continuo, `URTPlaybackLibrary::DirectionYaw`), e
`bFaceMovementDirection` può disattivarlo. Nessuna regola lo consuma. Il test
`Actions.Wait.AllowsFacingAndReaction` esiste proprio per **non precludere** un facing futuro: verifica che
`Wait` non consumi lo slot che servirebbe a orientarsi e a reagire.

La nota sugli Action Ghosts chiede un campo `Facing` nel view model della preview: da lì la domanda se il
facing debba restare presentazione o diventare stato di gioco. Il brief l'aveva registrata come aperta,
raccomandando «derivato, finché nessuna regola lo consuma».

La decisione dell'utente è di **dargli tre consumatori** — difesa, percezione e reazioni — e di legarlo al
movimento invece che renderlo un input libero.

## Decisione

### 1. Il facing è stato logico, e deriva dall'ultima azione di movimento

L'unità ha un `ERTHexDirection Facing` **autorevole**: enum a sei valori con ordine stabile (`E, NE, NW, W,
SW, SE`), intero, già usato da `URTHexLibrary::AxialDirection`/`Neighbor`. Nessun float, nessun angolo
continuo nella logica.

Si aggiorna **al termine della fase Move**, che è l'ultima fase volontaria del turno (ADR-0003 §1).

| Stile di movimento | Direzioni legali | Chi decide |
|---|---|---|
| `LinearDash` · `LinearCharge` · `LinearLeap` | **una**: la direzione del movimento | derivato, nessun input |
| `Budget` (`Action.Move`, `Sprint`) | **tre**: la direzione dell'ultimo passo e le due adiacenti (`D`, `D±1`) | dichiarato in planning fra le tre |
| `None` (nessun movimento: `Wait`, sola azione principale) | **sei**: rotazione libera | dichiarato in planning |

La rotazione **non consuma slot**: `Action.Wait` ha slot `None` e resta tale. Restare fermi e girarsi verso
un corridoio è una scelta tattica, non un turno sprecato.

### 2. Il facing è una decisione a effetto differito

Poiché l'ordine è `Prep → Dash → Blast → Move` e il Move è l'ultima fase, il facing scelto in un turno vale
per **tutto il turno successivo**, fino al suo Move. Durante il Blast del turno N l'unità guarda dove è
finita al turno N−1 (o dove l'ha portata il Dash del turno N).

**Derivata, non preferita**: è la conseguenza dell'ordine delle fasi, non una regola aggiunta. Ed è
desiderabile — ti orienti verso la minaccia che *prevedi*, e se prevedi male resti scoperto. La stessa forma
di commitment dell'Overwatch armato.

### 2-bis. Emendamento 2026-08-08 — un'azione con bersaglio orienta *prima* di risolvere

Le §1 e §2 sopra sono **superate** nella parte in cui dicono che il facing cambia soltanto a fine `Move`, e che
quindi durante il Blast del turno N l'unità guarda ancora dove l'aveva lasciata il turno N−1.

**Regola ([D-020](RT_PDR_00_Decision_Log.md)).** Quando un'azione ha un bersaglio o una direzione, il
personaggio **si orienta verso quel bersaglio/direzione prima che l'azione risolva**. Il facing resta stato di
gioco, ma cambia **più volte dentro il round**: ogni consumatore legge il valore autorevole più recente.

| Punto della timeline | Quando | Chi lo scrive |
|---|---|---|
| `FacingStartOfRound` | apertura del round | eredità: il `FacingFinalAfterMove` del round precedente |
| `FacingAfterPrepActionTargeting` | fase `Prep`, azione con bersaglio | il bersaglio dichiarato |
| `FacingAfterDash` | fase `Dash` | la direzione o il bersaglio del Dash |
| `FacingUsedByBlast` | fase `Blast` | il bersaglio del Blast, applicato **prima** della risoluzione |
| `FacingUsedByOverwatch` | reazione | il cono pianificato — coerente con §4c, che già lo diceva |
| `FacingFinalAfterMove` | fase `Move` | l'ultima direzione percorsa, o la direzione finale pianificata dove il sistema la supporta |

`FacingFinalAfterMove` **persiste nel round successivo** finché una nuova azione non lo cambia: è il valore che
diventa `FacingStartOfRound`.

**Cosa non cambia.** Il `Move` resta l'ultima fase volontaria, quindi il facing *finale* del round è ancora una
scommessa su quello dopo: su questo la §2 aveva ragione, e il commitment dell'Overwatch armato resta intatto.
Cade solo la clausola più forte — che l'unità non possa girarsi *mentre* agisce, che rendeva innaturale un
personaggio che spara a un bersaglio guardando altrove.

**Conseguenza da non perdere.** Con più valori di facing per round, snapshot e TurnLog non possono più
registrarne **uno** per turno: devono dire *quale* valore ha usato ciascun consumatore, altrimenti il replay
non è ricostruibile e la §5 (determinismo) diventa falsa.

**Test richiesti** (E16): sequenza `Dash → Blast`; cambio di bersaglio nello stesso round; cono Overwatch;
`Move` che fissa il facing finale; ereditarietà del facing nel round successivo.

### 3. Movimento forzato: ci si gira verso la sorgente

Spinta, knockback e displacement da reazione **non** sono la Move Phase (regola già consolidata in
[`brief-planning-visuale.md`](../technical/brief-planning-visuale.md) §A7) e quindi non seguono la regola §1. L'unità
spostata contro la propria volontà si gira **verso la cella di origine dell'ultimo effetto di spostamento
subito**, nell'ordine canonico di risoluzione — che è già totale e deterministico.

Casi limite, tutti derivati senza regole nuove:

| Situazione | Facing risultante |
|---|---|
| Più spinte nello stesso turno | verso l'origine dell'**ultima** nell'ordine canonico |
| Danno o spostamento **ambientale** (nessun attaccante) | **invariato** |
| AoE centrata su una cella | verso la **cella d'origine** dell'effetto, non verso chi l'ha lanciata |
| L'unità viene spinta e poi si muove volontariamente | vince il Move: la §1 si applica per ultima |

### 4. Un solo arco frontale, tre consumatori

L'arco frontale è definito **operativamente** dalla primitiva già esistente:

```
ArcoFrontale(Unità) = URTHexLibrary::HexCone(Cella, Neighbor(Cella, Facing), Range)
```

`HexCone` è un ventaglio di 120°, «unione dei due settori a 60° adiacenti alla direzione principale»: a
distanza 1 copre **3 celle**, a distanza 2 ne copre 5. Passando come bersaglio il vicino nella direzione del
facing si ottiene il cono del facing **senza scrivere una seconda geometria** — la direzione principale di
`HexCone` è per costruzione il primo passo della linea.

> Questa è la scelta architetturale più importante dell'ADR: **vista, difesa e reazioni direzionali usano
> letteralmente la stessa funzione**. Non possono divergere, perché non esistono due definizioni di «davanti».

#### 4a. Difesa — l'emisfero posteriore è scoperto

Un colpo la cui origine **non** è nell'arco frontale della vittima **annulla** la riduzione da **copertura
bassa** (−10) e da **`Action.Guard`** (−15). Nessun modificatore nuovo, nessun numero da bilanciare: si
riusano valori già a catalogo, togliendo una protezione invece di aggiungere danno.

Non cambia: `Deflect`, `Brace`, `Shield` e gli scudi restano validi da ogni direzione — proteggono la
persona, non un lato.

#### 4b. Percezione — vista a cono con consapevolezza ravvicinata (E13)

- Nell'arco frontale: vista piena fino a `VisionRange` dell'eroe (5–6 celle a catalogo), con LOS.
- **In ogni direzione, entro 2 celle**: si percepisce comunque, con LOS. È la *consapevolezza ravvicinata*, e
  riusa il **cap di contatto a 2 celle** già canonico per il fumo (`Max_Contact_Range`).
- Oltre 2 celle fuori dall'arco frontale: nulla.

La consapevolezza ravvicinata non è una concessione: senza di essa un'unità resta cieca alle spalle per un
turno intero, e ciò che il giocatore non può nemmeno percepire smette di essere una scelta e diventa una
punizione arbitraria.

#### 4c. Reazioni direzionali — il cono dell'Overwatch **è** il facing (E14)

La zona controllata di un Overwatch armato nasce dal facing dell'unità, **non** da una direzione dichiarata a
parte come proponeva la nota sorgente (§10: `Direction: North-East`). Due sorgenti per la stessa cosa
sarebbero due verità: chi arma la guardia decide dove guardare **orientandosi**.

### 5. Determinismo e privacy

- `Facing` entra nello **snapshot**, nel **TurnLog** (formato versionato) e nell'hash del replay: è un enum
  intero con ordine stabile, quindi non introduce dipendenze da `TMap`, float o ordine di iterazione.
- La rotazione **dichiarata** (casi `Budget` e `None`) è un **intento di planning**: viaggia in
  `FRTPlannedIntent` e passa da `FilterForTeam` come tutto il resto. L'orientamento che un avversario
  *intende* assumere è informazione; quello che ha **assunto** è pubblico, perché è una posa osservabile.
- La presentazione continua a interpolare lo yaw, ma alla fine del playback **deve atterrare sul facing
  logico**. Se la mesh guarda a nord e la regola dice sud, il giocatore legge una cosa e ne subisce un'altra:
  è il difetto peggiore per la leggibilità tattica (invariante #1).

## Alternative considerate

| Alternativa | Esito |
|---|---|
| Facing solo presentazione (stato attuale) | **Scartata dall'utente**: nessun peso tattico all'orientamento |
| Facing libero fra le 6 dopo un Move a budget | **Scartata**: spezza il legame «come arrivo determina come guardo» e rende il facing un intento del tutto indipendente dal movimento |
| Facing obbligato all'ultimo passo | **Scartata**: inchioda l'orientamento a una direzione che il pathfinding ha scelto, costringendo a pianificare waypoint solo per girarsi |
| Retro = 1 sola direzione con +6 danni | **Scartata**: raro da ottenere su hex, e aggiunge un modificatore invece di riusare quelli esistenti |
| Cono visivo senza consapevolezza ravvicinata | **Scartata**: cecità totale alle spalle per un turno intero |
| Cono visivo a 5 direzioni | **Scartata**: impatto quasi nullo, non giustifica il costo |
| Chi è spinto resta orientato com'era | **Scartata dall'utente** a favore di «si gira verso la sorgente» |

## Conseguenze

**Positive**: l'orientamento diventa una decisione tattica con conseguenze su tre assi (difesa, percezione,
reazioni) senza introdurre un solo numero nuovo · una sola primitiva (`HexCone`) definisce «davanti» per
tutti e tre · il facing è già mezzo implementato lato presentazione · aggirare un nemico diventa una manovra
con un premio leggibile, non solo un riposizionamento.

**Negative / costi**:

- **`Facing` diventa un prerequisito di E13**: la vista a cono non si può costruire prima. La catena della
  v0.1 si allunga: `E16.1 → E13 → E14`;
- la **difesa direzionale** tocca il combat math e va misurata: un colpo alle spalle su un bersaglio in
  copertura passa da −10 a 0, cioè **10 danni in più** su HP che vanno da 90 a 120;
- i test del bot cambiano premessa **due volte**: con E13 (conoscenza parziale) e con il cono (il bot deve
  considerare da dove viene visto e da dove può essere colpito);
- la rotazione dichiarata aggiunge un campo agli intenti, quindi alla privacy e alla serializzazione;
- la **preview** deve mostrare il facing pianificato, altrimenti il giocatore sceglie alla cieca una decisione
  che vale per tutto il turno successivo (CP 11.5 lo prevede già come campo del view model).

**Invarianti**: **#1** rafforzato (la presentazione deve atterrare sul facing logico, non definirlo) · **#2**
rispettato (il facing è un enum, non una rotazione del `FVector`) · **#4** rispettato (intero, ordine
stabile, formato versionato) · **#6** rispettato (la rotazione **intesa** è filtrata per squadra, quella
**assunta** è pubblica) · **#7** rispettato (l'arco frontale è una funzione pura già testata).

## Verifica

| Test | Cosa dimostra |
|---|---|
| `Facing.LinearMoveDerivesDirection` | dopo un Dash/Charge/Leap il facing è la direzione del movimento, senza input |
| `Facing.BudgetMoveAllowsLastStepPlusMinusOne` | dopo un Move a budget sono legali **tre** direzioni e solo quelle |
| `Facing.RejectsIllegalDeclaredRotation` | una rotazione dichiarata fuori dall'insieme legale viene **rifiutata**, non silenziosamente corretta |
| `Facing.StationaryUnitRotatesFreely` | da fermo tutte e sei sono legali e la rotazione non consuma slot |
| `Facing.ForcedMovementFacesSource` | chi è spinto si gira verso l'origine dell'ultimo spostamento subito |
| `Facing.EnvironmentalDisplacementKeepsFacing` | uno spostamento ambientale senza sorgente lascia il facing invariato |
| `Facing.VoluntaryMoveWinsOverForced` | se l'unità viene spinta e poi si muove, vale la regola del Move |
| `Facing.PermutationInvariant` | permutare l'ordine degli input non cambia il facing risultante |
| `Facing.IntentIsTeamFiltered` | la rotazione **dichiarata** non raggiunge il client avversario |
| `Combat.BackAttackIgnoresCover` · `…IgnoresGuard` | un colpo fuori dall'arco frontale annulla −10 e −15 |
| `Combat.FlankAttackKeepsCover` | dai fianchi la copertura vale: l'arco frontale è quello di `HexCone`, non solo la direzione esatta |
| `Vision.ConeUsesHexConePrimitive` | la vista non ha una geometria propria |
| `Vision.AwarenessWithinTwoCellsIgnoresFacing` | la consapevolezza ravvicinata vale in ogni direzione |
| `Overwatch.ArcComesFromFacing` | la zona controllata non dichiara una direzione propria |

## Perimetro — cosa questo ADR non decide *(nota 2026-08-08)*

L'ADR decide **come si determina** il facing e **chi lo consuma**. Non decide, e non va letto come se lo
facesse:

| Fuori perimetro | Dove vive la domanda |
|---|---|
| Il facing **durante** i micro-step di un Move | `FAC-4` in [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md) — la §1 copre inizio e fine del Move, non il mezzo, e i trigger cadono nel mezzo |
| Se una **reazione** possa ruotare chi reagisce | `FAC-5` — la §4c e D-020 nominano il facing dell'Overwatch come valore **letto**, mai scritto |
| Se `Interact` richieda o imponga un orientamento | `FAC-6` |
| Se **status** o **terreno** possano limitare la rotazione | `FAC-7`, `FAC-8` |
| Se il pathfinding debba diventare orientation-aware | `FAC-9` — l'ADR assume `CellId → CellId` con facing derivato e validato alla fine |
| Il **vocabolario** della rotazione in posto | `FAC-10` — qui si dice «rotazione dichiarata»; `Pivot` e `Reorient` non sono termini di questo ADR |

Tre proposte di **modifica** dell'ADR sono registrate come `FAC-1`, `FAC-2` e `FAC-3` nello stesso file, e
come righe **50–52** di [`../DOC_CONFLICT_MATRIX.md`](../DOC_CONFLICT_MATRIX.md): rotazione come capacità
**del personaggio** invece che derivata dal movimento; **policy dichiarative** per azione ed effetto al posto
delle due regole universali; `Brace` **direzionale** contro la §4a. Nessuna è stata applicata. Vengono da un
handoff del 2026-08-08, che nella gerarchia di prevalenza del progetto è l'**ultima** fonte — quindi l'ADR
resta in vigore — ma sono coerenti fra loro e cambiano il modello, non i dettagli: la scelta è dell'autore.

## Revisione

Rivedere alla chiusura di **CP 16.2** (difesa direzionale), con i dati del playtest.

**Soglia di allarme**: se aggirare diventa dominante — cioè se il modo migliore di giocare è sempre e solo
prendere il fianco — le vie di rientro sono parametri, non modifiche del modello: ridurre l'arco scoperto
(retro = solo la direzione opposta), oppure trasformare l'annullamento della copertura in una riduzione
parziale. **Seconda revisione** alla chiusura di **CP 13.5**, quando il bot gioca sulla percezione a cono:
se il bot risulta cieco in modo illeggibile, il parametro da muovere è la consapevolezza ravvicinata (2 → 3
celle), non la forma del cono.
