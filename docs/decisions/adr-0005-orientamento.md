# ADR-0005 — Orientamento: il facing come stato di gioco derivato dal movimento

> `CANONICAL` · **Stato**: Accettato — da implementare (E16) · **Data**: 2026-08-07 · **Decisore**: utente (dev singolo)
>
> ⚠️ **Emendamento 2026-08-08 — [D-020](RT_PDR_00_Decision_Log.md)**: le §1 e §2 dicevano che il facing si
> aggiorna **solo** al termine del `Move`. Non è più così: un'azione con bersaglio o direzione **orienta
> l'unità prima di risolvere**. Vedi la nuova **§2-bis**, che nomina i sei punti della timeline. Il resto
> dell'ADR — facing come stato di gioco, arco frontale unico, determinismo e privacy — resta invariato.
> **Contesto sorgente**: `docs/archive/src/design/action-ghosts-fasi-fast-reactions.md` §17 (campo `Facing`
> nel view model) e [`brief-planning-visuale.md`](../technical/systems/brief-planning-visuale.md) §C5, che registrava il punto come aperto.
> **Estende**: [ADR-0003](adr-0003-modello-azioni-v01.md) (stili di movimento) · [ADR-0004](adr-0004-finestre-di-reazione.md) (reazioni direzionali)
>
> ⛔ **Superato in parte 2026-08-10 — [ADR-0008](adr-0008-rotazione-e-policy-di-facing.md)**: la **§1** (tabella
> delle direzioni legali per stile) e la **§3** (regola universale dello spostamento subìto) **non sono più in
> vigore come scritte**. La rotazione finale è ora una **capacità del personaggio** misurata in step
> (`FAC-1`), e le due regole universali sono diventate il **default** di policy dichiarate sul dato (`FAC-2`).
> ADR-0008 definisce inoltre il facing **durante** i micro-step (`FAC-4`), che qui era fuori perimetro.
> **Resta invariato tutto il resto**: il facing come stato di gioco, la timeline di D-020 (§2-bis), l'arco
> frontale unico e i suoi tre consumatori (§4), il determinismo e la privacy (§5).

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

> ⛔ **Tabella superata da [ADR-0008](adr-0008-rotazione-e-policy-di-facing.md) §1 (2026-08-10), e a runtime
> dal 2026-09-03** ([#1605](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1605)).
> Le direzioni legali non dipendono più dallo **stile** ma dal **budget di pivot del personaggio**
> (`MoveEndPivotMaxSteps` / `DashEndPivotMaxSteps`, 0–3 step). Le tre righe qui sopra restano vere come
> **casi particolari**: `Linear*` ≡ budget 0, `Budget` ≡ budget 1, `None` ≡ `StationaryPivotMaxSteps` = 3, che
> ADR-0008 conferma universale.
>
> 🔑 **E i tre casi particolari sono ora i DEFAULT del codice**, non una nota storica: `FRTPivotBudget`
> nasce a `Move = 1 / Dash = 0`, così un'unità mai configurata da un eroe applica ancora questa tabella.
> Il comportamento cambia solo per gli eroi del catalogo, che dichiarano il proprio budget.
>
> ⚠️ **Per diciotto giorni questa tabella è stata superata sulla carta e applicata dal codice**: ADR-0008 era
> `CANONICAL` dal 2026-08-10, e `MoveEndPivotMaxSteps` aveva **0** occorrenze in `Source/`. Nessun gate
> confronta un ADR accettato con i simboli del codice — è la forma di difetto, non un incidente di questo
> documento.

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

> ⛔ **Superata come regola universale da [ADR-0008](adr-0008-rotazione-e-policy-di-facing.md) §3 (2026-08-10).**
> Quanto segue resta il **comportamento vigente**, ma non più come regola implicita nel resolver: è il
> **default** di `ERTDisplacementFacingPolicy` (`FaceSource` per `Forced`, `Keep` per `Environmental`). Un
> effetto che non dichiara nulla si comporta esattamente come descritto qui.

Spinta, knockback e displacement da reazione **non** sono la Move Phase (regola già consolidata in
[`brief-planning-visuale.md`](../technical/systems/brief-planning-visuale.md) §A7) e quindi non seguono la regola §1. L'unità
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

> ⚠️ **Emendato il 2026-08-13 da [D-126](RT_PDR_00_Decision_Log.md)**: la frase qui sopra resta vera per i
> consumatori d'**area**, che non si spostano di lato, ma `HexCone` **non è più la primitiva semantica** del
> facing. Vedi la [§4-bis](#4-bis-emendamento-2026-08-13--la-primitiva-semantica-sono-i-sei-lati-il-cono-resta-la-geometria-d-126).

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

### 4-bis. Emendamento 2026-08-13 — la primitiva semantica sono i sei lati, il cono resta la geometria (`D-126`)

[D-126](RT_PDR_00_Decision_Log.md) chiude `FAC-11` e separa due cose che la §4 teneva insieme:

| Domanda | Chi risponde | Cosa restituisce |
|---|---|---|
| **da quale lato** arriva questo colpo? | relazione a sei direzioni (`IncomingDirection` relativa a `TargetFacing`) | una di `Front · FrontRight · RearRight · Rear · RearLeft · FrontLeft` |
| **quale area** copre questa unità? | `URTHexLibrary::HexCone` | un ventaglio di 120° profondo quanto serve |

Le quattro direzioni non frontali restano **distinte** — niente `Side`/`Flank` generico — e un'abilità può
raggruppare i lati che le servono (`{FrontLeft, Front, FrontRight}`), ma quell'insieme appartiene al
**consumatore**, non al canone. Non esiste più una banda globale obbligatoria `Front Arc / Flank / Rear`.

🔴 **Nessuno dei tre consumatori di questa sezione cambia comportamento, e il motivo è misurato.** Il cono a
120° è **strettamente contenuto** nell'insieme dei tre lati frontali: replicando `HexCone`/`HexLine` con le
costanti reali su un difensore e raggio `1..10` si contano **45** celle di divergenza (diceva `50`, cifra
della regola a linea poi scartata, corretta da [D-147](RT_PDR_00_Decision_Log.md)), **tutte** nel verso
«tre-lati dentro / cono fuori» e **zero** nel verso opposto, con la prima a distanza **2**. Spostare §4a, §4b
o §4c sull'insieme dei lati sarebbe quindi un **buff difensivo netto**, non una rinomina — e il divieto di
avere due definizioni di «davanti» resta in vigore proprio perché nessun consumatore d'area si muove.

Il lavoro runtime che la relazione a sei lati richiede è
[#726](https://github.com/DegrassiAaron/refactor-tactics-main/issues/726).

### 4-ter. La relazione esiste a runtime, e la mappatura nome↔indice è stata fissata (2026-09-02, `#726`)

> ⌫ **La riga sopra diceva «oggi in `Source/` non esiste»**, ed era vera dal 2026-08-13 al 2026-09-02.

| Livello | Sede |
|---|---|
| geometria pura — in quale dei sei spicchi cade una cella | `URTHexLibrary::DirectionWedgeTowards` |
| semantica — da quale lato, *relativamente a un facing* | `URTFacingLibrary::RelativeDirectionFrom` |
| traccia — il primo consumatore che `D-126` chiedeva | `ERTFacingOutcome::HitCameFromSide` nel TurnLog |

🔴 **E la voce non contiene la cella dell'attaccante**: `SrcCell` e `TgtCell` portano entrambi il difensore,
come ogni altra voce `Facing` che descrive l'orientamento di un'unità. Il verdetto di visibilità si congela su
chi **subisce**, quindi una posizione dell'attaccante lì sarebbe pubblicata a chiunque percepisca il bersaglio
— e su *ogni* colpo risolto, non sui rari bypass. L'informazione della voce è il **lato**, che sta in `Amount`
e non rivela una posizione.

Lo spicchio `i` è **semiaperto**: `cella − centro = a·D(i) + b·D(i+1)` con `a > 0, b >= 0`. È la stessa
algebra di `HexCone` con una differenza deliberata — il cono somma settori **chiusi** perché deve coprire
un'area contigua, qui i settori **partizionano** — e da quel semiaperto viene l'equipartizione: **36** celle
per lato a raggio `1..8`, ed esattamente `r` a ogni anello `r`.

🔑 **La mappatura fra i sei nomi e gli indici era indecisa, e `D-147` la assegnava esplicitamente a `#726`.**
Le due fonti si contraddicevano: gli indici girano `E, NE, NW, W, SW, SE` e `AxialToWorld` manda `NE` a
`−Y`, quindi con la convenzione UE (`+X` avanti, `+Y` a destra) l'indice `f+1` cade a **sinistra** di chi
guarda, mentre l'elenco di `D-126` chiama `FrontRight` proprio quella posizione.

**Si è scelta la fedeltà geometrica**, cioè l'elenco di `D-126` percorso in senso **inverso**:

| `(spicchio − facing + 6) % 6` | `0` | `1` | `2` | `3` | `4` | `5` |
|---|---|---|---|---|---|---|
| nome | `Front` | `FrontLeft` | `RearLeft` | `Rear` | `RearRight` | `FrontRight` |

`D-126` fissa i sei **nomi**, non il verso di enumerazione; e l'argomento residuo che `D-147` registra sullo
skew è la **fedeltà dei nomi** — un `TurnLog` che dicesse `FrontRight` per una cella visibilmente a sinistra
sarebbe il difetto di explainability (E16) che quell'argomento teme.

⚠️ **Lo skew resta**, ed è dichiarato: `Front` è il raggio dritto davanti **più uno solo** dei due spicchi
adiacenti, e **168 celle su 216** non ricevono la direzione speculare della propria immagine speculare. Uno
`Shield = {Front}` proteggerebbe un fianco e non l'altro. Chi dichiarerà il primo insieme di lati lo sa
prima di sceglierlo. La regola alternativa — lo spicchio **centrato**, con `24` celle asimmetriche invece di
`168` — resta misurata e non adottata in `D-147`.

⛔ **`IsInFrontalArc` non è cambiata e nessun suo chiamante si è mosso**, per la ragione misurata sopra.
`RefactorTactics.Facing.RelativeDirectionDivergesFromCone` pinna le **45** divergenze e le **0** nel verso
opposto, così che il contenimento stretto smetta di essere una nota e diventi un test.

### 4-quater. Il perimetro della traccia copre anche le reazioni (2026-09-03, `#2128`)

> ⌫ **La riga qui sotto diceva che contrattacchi e Overwatch «non la portano»**, ed era vera dal 2026-09-02 al
> 2026-09-03.
>
> ⚠️ **Perimetro della traccia**: la voce è emessa per i colpi del **piano di Blast**. I **contrattacchi** e
> il fuoco di **Overwatch** non la portano — non passano da `Plan.Hits`, e `FRTAttack` non trasporta
> l'attaccante, quindi la geometria da cui la relazione si calcola lì non esiste.

La voce è emessa da **quattro** produttori, e copre ogni colpo risolto:

| Produttore | Origine del colpo | Cella e facing del difensore | Fase |
|---|---|---|---|
| piano di Blast — ciclo su `Plan.Hits` | `ResolveImpactOrigin` | `HexUnits[TargetId]` | `Blast` |
| **contrattacchi** — coda di `Attacks` dopo l'`Append` | `Reactions.CounterAttackSrc` | `HexUnits[TargetIndex]` | `Blast` |
| **Overwatch** — ramo `FIRE` di `ApplyReactionDecision` | cella del watcher | `State.Pos[TargetIdx]` · `Target->Facing` | `Move` |
| **Predictive boundary** — `ResolvePredictiveBoundary` | `Shooter->Cell` | `Armed.LockedCell` · `Victim->Facing` | `Move` |

> ⌫ **Questa tabella ne elencava TRE, e la riga sotto diceva «un'assenza significa una cosa sola» mentre non
> era vero**, dal 2026-09-03 fino alla stessa giornata. Il colpo al boundary della Predictive Action —
> `Hero.Wraith.InterceptShot`, il thin slice dichiarato della v0.1 — applica danno `ERTDamageSource::Direct` e
> **non emetteva la voce**. Il difetto non è stato trovato da un test ma da una **code review dopo il merge**:
> nessun oracolo copriva «predictive + voce direzionale», quindi l'affermazione era falsa e verde.
>
> 🔑 **La lezione non è l'omissione, è la forma dell'affermazione**: «copre ogni colpo risolto» è un
> quantificatore universale scritto in un documento normativo, e nessun gate lo verifica. Chi ne aggiunge uno
> deve **enumerare i produttori di danno**, non i propri.

⚠️ **Le due famiglie della fase `Move` misurano la cella dell'IMPATTO, non quella dell'Actor.** Entrambe
risolvono prima di `PlaceOnCell`, quindi `Unit->Cell` è ancora la cella di partenza del turno: l'Overwatch usa
`State.Pos[TargetIdx]` (dove il movimento è stato troncato) e la Predictive usa `Armed.LockedCell` (la cella su
cui si è scommesso).

> ⌫ **La riga qui sopra proseguiva con «*Le rispettive `BoundaryCoverReduction` fanno già la stessa scelta*»,
> ed era falsa per l'Overwatch**, dal 2026-09-03 alla stessa giornata
> ([#2142](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2142)).
>
> La voce direzionale usava `State.Pos[TargetIdx]`; la **copertura** dello stesso colpo, trenta righe sopra,
> riceveva `Target->Cell`. Due letture della stessa posizione nello stesso micro-step, con esiti diversi: chi
> usciva da un riparo incassava ridotto da una copertura che non aveva più. Il difetto era simmetrico sul
> watcher, che sparava dalla propria cella di partenza mentre la sua **zona** era già costruita da
> `State.Pos[OwnerIdx]`.
>
> 🔑 **Non è stato un cambio di regola**: [ADR-0004](adr-0004-finestre-di-reazione.md) §*«Quale cella»*
> prescrive *«Overwatch `FIRE` | la cella corrente»* da quando la tabella esiste, e il commento della
> funzione **citava** quella riga mentre il codice faceva l'opposto. Ciò che è cambiato è il comportamento
> osservato, non la decisione vigente.
>
> ⚠️ **E la riga `Shooter->Cell` della tabella qui sopra resta una cella di PARTENZA**, non quella da cui il
> colpo parte davvero: `ResolvePredictiveBoundary` gira a ciclo dei micro-step già chiuso, e un tiratore che
> si sia mosso nello stesso turno non è più lì. Non è correggibile senza scegliere una regola che nessun ADR
> ha preso — la tabella di ADR-0004 copre il **bersaglio** e non la **sorgente** — ed è aperta in
> [#2148](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2148).

**Pinnata da tre test**, sul percorso reale e non sulla funzione pura: la decisione qui è *quale cella il
chiamante passa*, e [D-312](RT_PDR_00_Decision_Log.md) ha misurato che una prova sulla funzione lascia il
chiamante scoperto.

| Test | Lato misurato |
|---|---|
| `Reactions.OverwatchCoverReadsTheMicroStepCell` | **bersaglio**: chi esce dal riparo incassa pieno |
| `Reactions.OverwatchLogLineAnnouncesDealtDamage` | **bersaglio**, verso opposto: chi entra in copertura incassa ridotto, e il log annuncia il danno inflitto |
| `Reactions.OverwatchMovingWatcherFiresFromItsCurrentCell` | **attaccante**: il watcher che si è spostato spara dalla cella in cui è |

⚠️ **Il terzo non è un di più, ed è stato aggiunto dopo una code review.** Con il watcher fermo
`State.Pos[OwnerIdx]` e `WatchOwner->Cell` coincidono a ogni micro-step: i primi due restano **verdi** con la
metà attaccante della correzione disfatta. È quindi lui a chiudere la metà che
[D-169](RT_PDR_00_Decision_Log.md) dichiarava non coperta — e che i primi due, da soli, non chiudevano.

🔑 **Non è servita una regola d'origine nuova**: [D-302](RT_PDR_00_Decision_Log.md) punto 3 classifica già il
colpo *diretto/mischia* come **sorgente→bersaglio**, e sia il contrattacco sia il fuoco di Overwatch — che è
letteralmente `ERTDamageSource::Direct` — vi ricadono. Ciò che mancava a `FRTAttack` era l'attaccante, non la
geometria: la sorgente vive accanto al colpo, registrata dal pass che l'ha prodotto.

🔴 **Perché emettere invece di documentare il buco.** L'alternativa lasciava **due silenzi indistinguibili**:
l'assenza legittima — origine coincidente in pianta col difensore, che `RelativeDirectionFrom` restituisce
`false` — e l'assenza per famiglia non coperta. Un consumatore che conta una voce per colpo subito non aveva
modo di separarli. Ora un'assenza significa una cosa sola.

🔑 **La voce si costruisce in una sede unica**: `URTFacingLibrary::MakeHitCameFromSideEntry`. Con tre
produttori la convenzione di privacy sotto sarebbe stata scritta tre volte, e tre copie di un invariante di
sicurezza divergono — una code review ne corregge una e le altre due restano.

⚠️ **Il facing dell'Overwatch è quello dell'ULTIMO PASSO COMPIUTO, non quello d'ingresso nella fase né
quello finale.** È la formula di [ADR-0008](adr-0008-rotazione-e-policy-di-facing.md) §2 —
`FacingAt(k) = FacingFromPath(Path[0..k], FacingAtMoveStart)` — e il pivot finale si applica **dopo**, senza
rileggere i boundary già passati. `Spec.Facing.OverwatchHitCameFromSide` la rende osservabile facendo
dichiarare al bersaglio una rotazione finale, così che il facing di fine turno **diverga** da quello con cui
il lato è stato misurato.

> 🔁 **Corretto il 2026-09-03 da [#2131](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2131).**
> Questo capoverso diceva *«il facing dell'Overwatch è quello d'INGRESSO nella fase Move»* e aggiungeva
> *«è anche la lettura giusta — si viene colpiti mentre ci si muove, con l'orientamento che si aveva»*. La
> **premessa** era esatta e lo resta: `ResolveReactionBoundary` gira dentro il ciclo dei micro-step e la
> `RecordFacingChange` che fissa `DerivedFromMove` scrive dopo l'uscita, quindi al momento del colpo
> l'orientamento finale non esiste ancora. La **conclusione** no: da «il finale non c'è ancora» non segue
> «vale quello d'ingresso», e ADR-0008 §2 — `CANONICAL` dal 2026-08-10 — dice che vale il passo appena
> compiuto. Descriveva fedelmente il codice del 2026-09-03 mattina, cioè una §2 non ancora implementata.
> *«L'orientamento che si aveva»* resta la frase giusta: camminando, è quello del passo appena fatto.

⚠️ **In un duello frontale i due lati coincidono, e non è un difetto**:
`FacingAfterPrepActionTargeting` gira l'attaccante verso il proprio bersaglio prima che il colpo risolva,
quindi chi attacca incassa il contrattacco **di fronte**. Un test che attendesse `Rear` da entrambe le parti
sarebbe sbagliato lui — è la trappola in cui è caduta la prima stesura di
`RefactorTactics.Reactions.Counter.TracesTheSideItCameFrom`.

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
| `UI.IntentViewFieldsAreClassified` | ogni campo di `FRTIntentView` ha una classe di privacy **dichiarata**: un campo nuovo non classificato fa rosso (#2331) |
| `UI.EnemyViewCarriesNoAllyOnlyField` | la riga sopra vale per **ogni** campo ally-only, non per i quattro di oggi — e i campi pubblici arrivano comunque a un avversario rivelato (#2331) |
| `Combat.BackAttackIgnoresCover` · `…IgnoresGuard` | un colpo fuori dall'arco frontale annulla −10 e −15 |
| `Combat.FlankAttackKeepsCover` | dai fianchi la copertura vale: l'arco frontale è quello di `HexCone`, non solo la direzione esatta |
| `Vision.ConeUsesHexConePrimitive` | la vista non ha una geometria propria |
| `Vision.AwarenessWithinTwoCellsIgnoresFacing` | la consapevolezza ravvicinata vale in ogni direzione |
| `Overwatch.ArcComesFromFacing` | la zona controllata non dichiara una direzione propria |
| `HexMove.StationaryUnitAppliesDeclaredRotation` | **turno vero**: chi non si muove dichiara una direzione e la ottiene — il caso «resto fermo e mi giro» |
| `HexMove.IllegalDeclaredRotationIsRejectedInPlayedTurn` | **turno vero**: una dichiarazione fuori dall'insieme legale lascia il facing **derivato** e produce `DeclarationRejected` nel TurnLog |
| `HexMove.DeclaredRotationDoesNotSurviveItsTurn` | la dichiarazione è un pezzo del piano: consumata a fine Move, non si ridichiara da sola il turno dopo |

> ⚠️ **Stato del cablaggio, 2026-08-09** ([#291](https://github.com/DegrassiAaron/refactor-tactics-main/issues/291)).
> Fino a questa data i primi quattro test della tabella verificavano regole che **nessuno chiamava**:
> `URTFacingLibrary::TryApplyDeclaredFacing` era invocata soltanto dai test della libreria pura, e la
> rotazione dichiarata non aveva un produttore in nessun anello. Ora la catena esiste — campo su `ARTUnit`,
> ingresso in `FRTPlannedIntent`, filtro per squadra, consumo nel `TurnManager` a fine Move con
> `DeclaredInPlanning` / `DeclarationRejected` — e i tre test qui sopra la esercitano su un **turno giocato**.
>
> Resta scoperto **l'input**, che non è una regola: nessun comando permette al giocatore di dichiarare una
> rotazione e il bot non ne dichiara. È lavoro di **E11**, e serve insieme al feedback visivo — senza
> l'insieme legale mostrato, il giocatore non può sapere quali direzioni gli restano — e da ADR-0008 §1 non sono più «tre» per tutti, ma quante il budget di pivot del suo eroe gliene concede. Per la stessa
> ragione l'harness **non** ha una chiave `facing`: gliela si darebbe solo per farlo diventare il primo
> produttore del campo, cioè più capace del gioco. La capability `DeclaredRotation` è dichiarata **non
> disponibile** in `RTScenarioSession.cpp`, accanto a `ReactionPlanning`, che ha la stessa forma.

> ✅ **Aggiornamento 2026-08-13 — l'ultimo capoverso qui sopra non descrive più il repository.** Resta
> scritto perché il ragionamento era giusto ed è la ragione per cui la chiave è stata tenuta fuori per
> quattro giorni; ma la sua **premessa è caduta**, e va detto dove qualcuno la leggerebbe.
>
> | Cosa diceva | Stato misurato oggi |
> |---|---|
> | «nessun comando permette al giocatore di dichiarare una rotazione» | `ARTPlayerController::HandleFacingSector` ([#737](https://github.com/DegrassiAaron/refactor-tactics-main/issues/737), PR #743) |
> | «l'harness **non** ha una chiave `facing`» | `FRTScenarioIntent::Facing` + `bDeclaresFacing` (`RTTestScenario.h:372`), letta a `RTScenarioLoader.cpp:489` (PR #744) |
> | «`DeclaredRotation` è dichiarata **non disponibile**» | è **fra le disponibili** (`RTScenarioSession.cpp:124`) |
>
> L'harness non è più il primo produttore, quindi il verde dice ora qualcosa di vero sul giocatore — che è
> esattamente la condizione che il capoverso poneva. Due scenari la esercitano sul percorso reale,
> `Spec.Facing.IllegalDeclaredRotationIsRejected` e `Spec.Facing.StationaryDeclaredRotationApplies`, e
> l'ancora `RefactorTactics.Scenario.DeclaredRotationScenariosPass` ne pinna il `Pass` perché uno
> scivolamento in `BLOCKED` non passi in silenzio. **`ReactionPlanning` invece non è cambiata**: il
> paragone regge solo per la forma, non per lo stato.
>
> Di [#291](https://github.com/DegrassiAaron/refactor-tactics-main/issues/291) restano **il bot** — che non
> dichiara rotazioni — e **l'insieme legale a schermo**, che è
> [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613): il comando esiste, la sua
> leggibilità no, e oggi il rifiuto è un `UE_LOG` invece di un reason code visibile.

## Perimetro — cosa questo ADR non decide *(nota 2026-08-08)*

L'ADR decide **come si determina** il facing e **chi lo consuma**. Non decide, e non va letto come se lo
facesse:

| Fuori perimetro | Dove vive la domanda |
|---|---|
| ~~Il facing **durante** i micro-step di un Move~~ | ✅ **Chiuso 2026-08-10**: `FAC-4` è deciso in [ADR-0008](adr-0008-rotazione-e-policy-di-facing.md) §2 — è la direzione dell'**ultimo passo compiuto** |
| Se una **reazione** possa ruotare chi reagisce | `FAC-5` — la §4c e D-020 nominano il facing dell'Overwatch come valore **letto**, mai scritto |
| Se `Interact` richieda o imponga un orientamento | `FAC-6` |
| Se **status** o **terreno** possano limitare la rotazione | `FAC-7`, `FAC-8` |
| Se il pathfinding debba diventare orientation-aware | `FAC-9` — l'ADR assume `CellId → CellId` con facing derivato e validato alla fine |
| ~~Il **vocabolario** della rotazione in posto~~ | ✅ **Chiuso 2026-08-10**: `FAC-10` è risolto in [ADR-0008](adr-0008-rotazione-e-policy-di-facing.md) §4 — **pivot** è la capacità, **rotazione dichiarata** è l'atto |

Tre proposte di **modifica** dell'ADR erano registrate come `FAC-1`, `FAC-2` e `FAC-3` nello stesso file, e
come righe **50–52** di [`../DOC_CONFLICT_MATRIX.md`](../DOC_CONFLICT_MATRIX.md).

**Esito, 2026-08-10.** `FAC-1` (rotazione come capacità **del personaggio**) e `FAC-2` (**policy dichiarative**
per azione ed effetto) sono state **accettate** dall'autore e sono ora in
[ADR-0008](adr-0008-rotazione-e-policy-di-facing.md): la §1 e la §3 di questo ADR sono superate di conseguenza.
`FAC-3` (`Brace` **direzionale**, contro la §4a) **non è stata decisa** e resta aperta: la §4a è tuttora in
vigore e `Combat.ShieldWorksFromAnyDirection` continua a proteggerla dalla deriva. Da notare che, con `FAC-2`
accettata, `FAC-3` avrebbe ora una sede naturale in cui esprimersi — una policy di facing dichiarata su
`Brace` — il che ne abbassa il costo di implementazione ma **non** ne cambia il merito.

## Revisione

Rivedere alla chiusura di **CP 16.2** (difesa direzionale), con i dati del playtest.

**Soglia di allarme**: se aggirare diventa dominante — cioè se il modo migliore di giocare è sempre e solo
prendere il fianco — le vie di rientro sono parametri, non modifiche del modello: ridurre l'arco scoperto
(retro = solo la direzione opposta), oppure trasformare l'annullamento della copertura in una riduzione
parziale. **Seconda revisione** alla chiusura di **CP 13.5**, quando il bot gioca sulla percezione a cono:
se il bot risulta cieco in modo illeggibile, il parametro da muovere è la consapevolezza ravvicinata (2 → 3
celle), non la forma del cono.
