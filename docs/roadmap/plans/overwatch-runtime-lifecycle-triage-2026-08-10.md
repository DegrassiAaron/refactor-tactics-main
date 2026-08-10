# Spec Panel — Overwatch Runtime Lifecycle, Watch Stage e Reposition

> `CURRENT` · **Creato**: 2026-08-10 · **Owner** di **una sola domanda**: che cosa del sorgente
> [`2026-08-10-overwatch-runtime-lifecycle-watch-reposition.md`](../../archive/src/handoff/2026-08-10-overwatch-runtime-lifecycle-watch-reposition.md)
> è compatibile col canone, che cosa lo estende e che cosa va deciso prima di poter essere scritto.
>
> Non è una specifica e non decide. Owner delle regole restano il
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md), [ADR-0004](../../decisions/adr-0004-finestre-di-reazione.md)
> e [`brief-overwatch-reazioni.md`](../../gameplay/brief-overwatch-reazioni.md).
>
> Gemello: [`baseaction-signatures-spec-panel-2026-08-10.md`](baseaction-signatures-spec-panel-2026-08-10.md),
> di cui questo sorgente **chiude la domanda `BAS-5`**.

## 1. Verdetto

Il sorgente **non era stato consumato**, e a differenza del gemello **non è stato superato dal canone**: è il
più recente dei due, si dichiara tale, e il modello che porta è coerente, completo e per larga parte già
allineato a decisioni esistenti. Il problema non è il contenuto — è che **tre nomi su cui poggia sono già
occupati**, e che due delle sue regole cambiano l'economia del turno senza dirlo.

| Sezione | Stato | Esito |
|---|---|---|
| §21–22 cadence *once-per-target* | `Spec.Overwatch.HoldThenFire` la esprime già | ✅ **confermata da uno scenario esistente** |
| §24 `MaxPrompts` conta opportunity, non passi | precisa ADR-0004 §8 | ✅ compatibile, ed è una precisazione utile |
| §7 `PreparedWatchFacing` | **D-020**: l'Overwatch usa il cono *pianificato* | ✅ già canone |
| §9 tabella di counterplay pre-Watch | **CP 14.6** ha già `CancelledByStun`/`ByForcedMovement` | ✅ allineata, e più fine |
| §13 nessuna reservation · §30 no nesting | invariante #6, **D-021**, **D-017** | ✅ già canone |
| §12 `Reposition` | **`Action.Reposition` esiste**: 2 celle, fase **Dash** | ❌ **collisione di nome** |
| §40 `REACT-001…011` | schema **già respinto** il 2026-08-08 | ❌ formato ricomparso |
| §36 undici feature ID | **sette non esistono** | ❌ ID inventati |
| §10 «no voluntary Dash» · §4 Move sostituito | **D-012** + **D-028** non lo prevedono | ⚠️ **cambia l'action economy** |
| §5 macro-fasi con `Commit` | il canone ne ha sei, senza `Commit` | ⚠️ minore |

## 2. Panel — modalità critique

Esperti sul dominio: **Fowler** (confini e nomi), **Hohpe** (ordinamento e consegna degli eventi),
**Nygard** (modi di fallimento e costo operativo), **Wiegers** (requisiti misurabili), **Adzic** (esempi
eseguibili), **Crispin** (oracoli). Ogni rilievo porta l'evidenza misurata sul branch.

### 2.1 CRITICO — `Reposition` è già un'azione, e risolve in un'altra fase

**FOWLER**: «Il §12 introduce `MovementProfile = OverwatchReposition` e avverte di non confondere il
Reposition con *"Sneak, Dash, ReactionMovement, Teleport"*. L'elenco omette l'unica cosa con cui si confonde
davvero: **`Action.Reposition`, che esiste**.»

Evidenza — `Source/RefactorTactics/Ability/RTCatalogLibrary.cpp:363-365` e
[catalogo azioni](../../balance/RT_ActionCatalog_v0.1.md) §2.2:

| | `Action.Reposition` (canone) | `Reposition` (sorgente §12) |
|---|---|---|
| Fase | **Dash** (`FastMovement`, priorità 40) | **Move**, Stage B — *dopo* il Watch |
| Forma | linea retta, **2 celle**, non attraversa unità | profilo di Move pre-pianificato, budget ridotto |
| Slot | Movimento (**D-028**) | sostituisce il Move normale |
| Quando si dichiara | come mobilità del turno | in Planning, insieme al Watch |

Due entità, un nome, fasi diverse. È la **terza collisione in due handoff pari-data**: il gemello ne portava
già due (`Vektor.Deflection`, `Riva.Flow`). E qui il costo è più alto, perché `Reposition` non è solo un
nome di catalogo: è cablato in `RTMovementActionLibrary` fra le mobilità lineari, e `Riva.FlowReaction` e
`Vektor.Feint` ne concedono uno.

**Raccomandazione**: se il modello viene accettato, il profilo post-Watch ha bisogno di un nome proprio —
`WatchdrawMove`, `PostWatchMove`, o qualunque cosa l'autore preferisca purché non sia `Reposition`. Rinominare
invece `Action.Reposition` costerebbe catalogo, codice, due kit d'eroe e i test.

### 2.2 CRITICO — `REACT-001…011` è uno schema già respinto

**ADZIC**: «Il §40 dice *"prima riutilizzare gli scenari esistenti"* ed elenca `REACT-001` … `REACT-009`.
Ottima istruzione, elenco sbagliato: **quegli scenari non esistono con quei nomi**, e non è una svista
nuova.»

Evidenza — `docs/archive/src/handoff/2026-08-08-master-reaction-system.md:580`:

> «schema `REACT-001…011` era un **terzo formato** e avrebbe prodotto duplicati»

Il 2026-08-08 quello schema è stato **mappato sugli ScenarioId canonici** (`Spec.*`) proprio per non aggiungere
una terza convenzione. Il sorgente lo ripresenta come se fosse il registro corrente. Chi lo eseguisse alla
lettera creerebbe i duplicati che quella decisione ha evitato.

**Raccomandazione**: la mappatura è al §5. Nessun `REACT-*` va creato.

### 2.3 CRITICO — sette feature ID su undici non esistono

**WIEGERS**: «Il §36 introduce l'elenco con *"sono noti almeno concetti/ID del tipo"*. È una formula che
sembra prudente e non lo è: chi legge la prende per un inventario.»

Misurato sul registry:

| Esiste | Non esiste |
|---|---|
| `RT-FEAT-REACTION-OPPORTUNITY` · `RT-FEAT-REACTION-FAST` · `RT-FEAT-REACTION-OVERWATCH` · `RT-FEAT-CORE-DECISION-BOUNDARY` | `RT-FEAT-REACTION-MULTI-TRIGGER` · `RT-FEAT-REACTION-SIMULTANEOUS` · `RT-FEAT-REACTION-NO-NESTED` · `RT-FEAT-REACTION-FACING` · `RT-FEAT-REACTION-PRIVACY` · `RT-FEAT-CORE-MICROSTEPS` · `RT-FEAT-ACTION-MOVE` |

Il documento **si difende da solo** («creare nuovi ID solo dopo audit del registry machine-readable»): la
raccomandazione è tenere quella riga e buttare l'elenco.

### 2.4 MAGGIORE — l'Overwatch costerebbe anche lo slot movimento

**NYGARD**: «Qui non si sta precisando una regola, si sta cambiando il prezzo di un'azione, e in tre punti
diversi che nessuno legge insieme.»

Il sorgente accumula: §4 «il Move normale viene sostituito da un Reposition limitato», §10 «Choose Overwatch
→ **no voluntary Dash** nello stesso piano», §15 «nessun refund, mai convertire a Move completo».

Il canone dice altro:

- **D-012** / **D-014**: l'economia è `Attack` **oppure** `Ability` **oppure** `Overwatch` — cioè lo slot
  **azione principale**;
- **D-028**: «un turno dà **un movimento e un'azione principale**», e `Dash · Leap · Reposition` stanno nello
  slot **Movimento**.

Con il canone attuale, armare l'Overwatch **non** tocca il movimento: si può ancora dashare. Il sorgente
gliene fa pagare tre (azione, movimento pieno, priorità spaziale tardiva) e lo dichiara apertamente al §51
come *core trade-off*. **È una decisione di bilanciamento legittima e forse giusta — ma è una decisione**, e
il documento la presenta come consolidamento.

**Raccomandazione**: `OW-1` al §4. Il divieto di Dash ha una motivazione strutturale sana (se l'owner si
sposta prima del Watch, la `Watch Origin` pianificata non è più la sua cella, ed è lo stesso caso che il §14
chiama `StartCellMismatch`) — ma quella motivazione giustifica *cancellare l'Overwatch*, non *vietare il Dash*
in planning. Sono due design diversi con conseguenze diverse sul bluff.

### 2.5 ALLINEAMENTO — la cadence *once-per-target* ha già la sua specifica eseguibile

**ADZIC**: «Prima di chiedere scenari nuovi, guardo se il caso è già scritto. Lo è, e coincide.»

`Scenarios/Spec/.../HoldThenFire.json` descrive: Vektor controlla un varco → **Flux** entra e Vektor risponde
`HOLD` → poi **Riva** entra e Vektor risponde `FIRE`. Due bersagli **diversi**, che è esattamente la
semantica che il §22 vuole difendere: «*lascio passare questo bersaglio e scommetto su un'occasione
migliore*». Lo scenario non contraddice la cadence: **la esprime**, ed era stato scritto prima.

Il §24 («`MaxPrompts` conta Reaction Opportunities distinte, non passi né unità dentro un'opportunity
aggregata») **precisa** ADR-0004 §8, che dice «limita le opportunity di **una** reaction» senza dire cosa sia
un'opportunity quando i bersagli sono più d'uno. È il tipo di precisazione che vale la pena consolidare.

### 2.6 ALLINEAMENTO — una conseguenza che il sorgente non dichiara, e che vale più di metà del documento

**NYGARD**: «Il rischio dichiarato numero uno di E14 è la durata della resolution. Questa cadence lo riduce
di un terzo, e il documento non se ne accorge.»

Il conto: ADR-0004 §8 fissa il caso peggiore in `MaxPromptsPerReaction 3 × 3 s = **9 s** per una sola unità
armata`, e la roadmap lo registra come rischio (a) di E14 con soglia d'allarme a 20 s. Con la cadence
*once-per-target*, un'Overwatch può aprire **al massimo un'opportunity per bersaglio distinto** — e la v0.1 è
**2v2**: i bersagli avversari sono **due**.

```
prima:  3 prompt × 3 s = 9 s   (raggiungibile con un solo nemico che cammina)
dopo:   2 prompt × 3 s = 6 s   (e servono DUE nemici distinti)
```

`MaxPromptsPerReaction = 3` diventa **irraggiungibile in 2v2** da una singola Overwatch. Non va cambiato — il
formato competitivo non è deciso ([D-011](../../decisions/RT_PDR_00_Decision_Log.md)) e in 3v3 il terzo prompt
torna possibile — ma il rientro `MaxPromptsPerReaction = 1`, che ADR-0004 §Revisione teneva pronto, diventa
molto meno probabile che serva. **La misura di CP 14.5 va fatta comunque**: questo è un conto, non un dato.

### 2.7 MINORE — `Commit` non è una macro-fase

Il §5 chiede di «preservare» `Planning → Commit → Prep → Dash → Blast → Move → Cleanup`. Il canone ne ha sei,
senza `Commit`: è la stessa formulazione già usata dal gemello. Il `Commit` esiste come **momento** del
planning, non come fase della resolution. Chiedere di preservare qualcosa che non c'è è il modo più efficace
di introdurlo.

### 2.8 MINORE — la lista di validazione contiene il proprio antidoto

**CRISPIN**: «Il §48 chiede di controllare che non restino in giro *"vecchio post-Overwatch Normal/Sneak Move
non aggiornato"*. È il §14 del sorgente gemello. Il documento sa di superarlo, e lo dice in una checklist
invece che in una decisione.» Vedi `BAS-5`, che questo triage chiude come domanda.

## 3. Che cosa si tiene

Il modello **Watch → EndWatchStage → Reposition** è il contributo, ed è più solido dell'alternativa del
gemello: dà all'Overwatch un costo leggibile, elimina la killzone mobile senza casi speciali, e sposta la
scelta del riposizionamento in Planning — dove non può diventare *"vedo dove sei finito, poi decido"*.

Quattro punti sono canone-compatibili così come sono, e potrebbero entrare nel DoD di E14 senza altre
decisioni:

1. **cadence `OncePerTargetPerReactionInstance`** (§21) — con lo scenario che già la esprime;
2. **`MaxPrompts` conta opportunity distinte** (§24) — precisazione di ADR-0004 §8;
3. **eligibility valutata post-transition** (§20) — LOS/detection dopo il passo, non durante;
4. **hard cancel vs soft eligibility block** (§9) — distinzione che CP 14.6 oggi non fa, e che serve perché
   `NoLOS` e `Stun` non possono produrre lo stesso reason code terminale.

Il resto — staging Watch/Reposition, budget, divieto di Dash — dipende da `OW-1` e `OW-2`.

## 4. Decisioni aperte

**Aggiornamento del 2026-08-10**: tre delle quattro sono state **decise dall'autore** in sessione e vivono in
[D-070](../../decisions/RT_PDR_00_Decision_Log.md). Restano qui per la provenienza.

| ID | Domanda | Esito |
|---|---|---|
| ~~`OW-1`~~ | Armare l'Overwatch costa **anche** il movimento? | ✅ **Sì**, ma senza `MovementAndMain`: occupa lo slot **principale** e **riserva** quello di movimento al solo `Withdraw`. Il divieto di Dash è una **conseguenza**, non una regola nuova |
| ~~`OW-2`~~ | Il Move post-Watch come si chiama? | ✅ **`Withdraw`**. `Action.Reposition` resta dov'è |
| ~~`OW-3`~~ | Il suo budget | ✅ **2 MP**, ancorati ad `Action.Reposition` |
| **`OW-4`** | Gli objective che dipendono dalla posizione finale si valutano dopo Stage B? | ⏳ **aperta, ma non è di E14**: gli objective oggi sono solo un motivo di fine partita, e il punto d'ingresso per il progresso è dichiarato per **CP 10.2**. Nessun objective di posizione esiste: la domanda non ha ancora un consumatore |

> **L'ancoraggio che ho proposto per primo era sbagliato, e vale più della risposta.** Avevo indicato
> `ERTActionSlot::MovementAndMain` come «meccanismo già esistente», citando `Action.Sprint` che lo usa. Ma
> **D-028 lo sta togliendo proprio a Sprint** — il catalogo lo dichiara già «occupa il solo slot movimento»,
> con ⚠️ sul codice non allineato. Sprint ne è l'**unico** utente: adottarlo per l'Overwatch l'avrebbe resa
> l'unica utente di uno slot in via di dismissione, riaprendo una decisione chiusa. «Il meccanismo esiste» non
> basta: va guardato **in che direzione si sta muovendo**.

> **`BAS-5` si chiude qui come domanda**, e la risposta è: **prevale il modello Watch/Reposition**. Non perché
> sia più recente — la data è la stessa — ma perché il sorgente gemello lo dichiara superato (§34, §48) e
> perché questo modello è l'unico dei due che spiega *dove* il personaggio si trova quando l'Overwatch
> finisce. Resta da decidere il suo **costo** (`OW-1`) e il suo **nome** (`OW-2`).

## 5. Mappatura sugli ID reali

| Sorgente | Reale |
|---|---|
| `REACT-001` Overwatch Fire | nessuno: è il caso `FIRE` di **CP 14.5**, senza scenario proprio |
| `REACT-002` Hold Then Fire | **`Spec.Overwatch.HoldThenFire`** — esiste, esce `BLOCKED` |
| `REACT-003` Simultaneous Targets | nessuno: è il test `Overwatch.SimultaneousTargetsSingleOpportunity` (CP 14.4) |
| `REACT-004` Timeout | è il test `Overwatch.TimeoutIsHold` (CP 14.5) |
| `REACT-007` Reaction Cancelled | sono `Overwatch.CancelledByStun` / `…ByForcedMovement` (CP 14.6) |
| `REACT-009` Privacy Canary | è `Overwatch.OpportunityLeaksNoFuture` (CP 14.5) + il gate `network_privacy` |
| `RT-FEAT-REACTION-MULTI-TRIGGER` · `…-SIMULTANEOUS` · `…-NO-NESTED` | **non servono**: sono DoD di `RT-FEAT-REACTION-OVERWATCH` e `RT-FEAT-REACTION-OPPORTUNITY` |
| `RT-FEAT-REACTION-FACING` | è `RT-FEAT-MAP-FACING`, che già possiede `FacingUsedByOverwatch` |
| `RT-FEAT-REACTION-PRIVACY` | è il **gate** `network_privacy` di ogni feature, non una feature |
| `RT-FEAT-CORE-MICROSTEPS` | è **CP 14.3**, dentro `RT-FEAT-CORE-DECISION-BOUNDARY` |
| `RT-FEAT-ACTION-MOVE` | è `RT-FEAT-ACTION-MOVE-PROFILES` |
| Epic «Overwatch Runtime Lifecycle v0.1» | **E14**, che esiste e copre già lo scope |

## 6. Che cosa è stato scritto nel repository

| File | Modifica |
|---|---|
| `docs/OPEN_DECISIONS.md` | `OW-1`…`OW-4`; **`BAS-5` chiusa** come domanda, con il rimando qui |
| `docs/roadmap/feature-registry.yaml` | note su `RT-FEAT-REACTION-OVERWATCH`: il modello, i quattro punti canone-compatibili, le due collisioni |
| `docs/roadmap/roadmap-v0.1.md` | E14: la nota del gemello si aggiorna, e il conto dei 9 s → 6 s è registrato dove vive il rischio |
| `docs/archive/src/handoff/` | il sorgente, archiviato come recepito |

**Nessuno scenario nuovo è stato dichiarato**, e la ragione è la stessa del gemello: il contenuto non è
deciso. Scrivere ora i venti scenari del §40 significherebbe fissare in file eseguibili un modello che
`OW-1` e `OW-2` possono ancora cambiare di forma. Quando la decisione arriva, il §40 è già la lista.
