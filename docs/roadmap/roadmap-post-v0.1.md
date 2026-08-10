# RefactorTactics — Roadmap oltre la v0.1

> `CURRENT` · **Creato**: 2026-08-08 · **Owner**: questo file per le release **v0.2 → v0.4**.
> La v0.1 resta in [`roadmap-v0.1.md`](roadmap-v0.1.md); lo stato di esecuzione in
> [`roadmap-checkpoint.md`](roadmap-checkpoint.md).
>
> **Questo documento non apre lavoro.** Nessuna epic qui dentro si implementa prima che i 15 gate della v0.1
> siano verdi ([`v0.1-definition-of-done.md`](v0.1-definition-of-done.md) §3). Serve a decidere **oggi** le
> cose che, se decise dopo, costringerebbero a rifare: numerazione delle epic, confini fra release, e quali
> vincoli architetturali la v0.1 deve già rispettare.

## Perché esiste

Sette documenti di `docs/src/` prodotti il 2026-08-07/08 descrivono sistemi che **non stanno nella v0.1**:
roster a 8, Cover Window, architettura di muri e porte, formato competitivo 3v3, bot tattico ed esperto,
mappe Operations. Finché sono rimasti solo lì, ogni sessione ha dovuto ridecidere da capo se una cosa fosse
v0.1 o no. Questo file registra quella ripartizione una volta sola.

**Fonti** (tutte in `docs/src/`, non normative — vedi [`../src/README.md`](../src/README.md)):

| Sorgente | Alimenta |
|---|---|
| [`design/2026-08-08-roster-8-conflux-constrine.md`](../archive/src/design/2026-08-08-roster-8-conflux-constrine.md) | E35 |
| [`design/2026-08-08-cover-window-open-fire-seal.md`](../archive/src/design/2026-08-08-cover-window-open-fire-seal.md) | E22 |
| [`design/2026-08-08-muri-porte-e-interazioni.md`](../archive/src/design/2026-08-08-muri-porte-e-interazioni.md) | E23 |
| [`design/match-timing-e-scala-mappe.md`](../archive/src/design/match-timing-e-scala-mappe.md) | E19 (v0.1), E24, E30 |
| [`design/2026-08-08-hud-faction-icons.md`](../archive/src/design/2026-08-08-hud-faction-icons.md) | E20 (v0.1), E25 |
| [`handoff/2026-08-08-bot-ai-roadmap-e-test-pie.md`](../archive/src/handoff/2026-08-08-bot-ai-roadmap-e-test-pie.md) | E26, E28 |
| [`handoff/2026-08-07-nuove-decisioni-e-scenario-4v4.md`](../archive/src/handoff/2026-08-07-nuove-decisioni-e-scenario-4v4.md) | E24, E27 |

## Le release

| Release | Nome | Tema | Epic | Formato di gioco |
|---|---|---|---|---|
| **v0.1** | Vertical slice | Il turno simultaneo funziona e si vede | E1–E21 | Skirmish 2v2 vs bot |
| **v0.2** | Struttura e finestre | Il campo diventa manipolabile; roster 8 | E22–E26 · **E35** · **E36** | Standard 3v3 |
| **v0.3** | Informazione | Quello che non sai vale quanto quello che fai | E27–E29 · **E33** | Standard 3v3 |
| **v0.4** | Operations | Partite lunghe su mappe grandi | E30–E32 · **E34** | Operations 4v4+ |

Oltre la v0.4 resta **north-star non pianificato**: multiplayer in rete (milestone M10 del piano canonico),
progressione, modding, editor mappe a runtime. Non si aprono epic per ciò che non ha una release.

> **Numerazione continua.** Le epic proseguono da E18 (ultima della v0.1) senza azzerarsi per release: un
> riferimento a «E23» resta univoco per sempre. Le due epic **E19** ed **E20** appartengono alla **v0.1** pur
> nascendo da sorgenti di questa roadmap — vedi [Cosa la v0.1 deve già rispettare](#cosa-la-v01-deve-già-rispettare).
> Le release non sono più contigue nella numerazione, e non devono esserlo: **E33** sta in v0.3, **E34** ed
> **E35** in v0.4 e v0.2. La contiguità è una comodità di lettura, l'univocità è un requisito.

> ⚠️ **`E21` è stata assegnata due volte, e questo documento ha ceduto il numero** *(2026-08-09,
> [D-039](../decisions/RT_PDR_00_Decision_Log.md))*. Lo stesso giorno — il 2026-08-08 — due sessioni parallele
> hanno preso `E21`: qui per il roster a 8, e in [`roadmap-v0.1.md`](roadmap-v0.1.md) per *Presentazione e
> leggibilità*, l'epic che il Feature Registry aveva appena reso visibile come buco della v0.1. È lo stesso
> meccanismo che aveva già prodotto il doppio `D-028`: **il contatore condiviso si assegna al merge**, e
> nessuna delle due sessioni poteva vedere l'altra.
>
> Il numero resta alla v0.1 perché lì è **verificato da una macchina**: `feature-registry.yaml` mappa
> `RT-FEAT-CHAR-PRESENTATION` su `E21.1`–`E21.3`, la tabella §2.2 di `roadmap-v0.1.md` è **generata** da quel
> dato, e il Decision Log ([D-037](../decisions/RT_PDR_00_Decision_Log.md)) cita «E21/M8» per gli slot Paragon.
> Spostare quel lato avrebbe richiesto di rigenerare il registry e correggere una decisione consolidata; il
> roster a 8 viveva invece in questo file soltanto, in quattro punti. **Il roster a 8 diventa E35.**

---

## Cosa la v0.1 deve già rispettare

Due vincoli dei sorgenti nuovi **non** possono aspettare la v0.2: se la v0.1 li ignora, il lavoro va rifatto
invece che esteso. Sono diventati **E19** ed **E20** dentro [`roadmap-v0.1.md`](roadmap-v0.1.md).

**E19 — Match Format e classe di mappa, data-driven** · P1
`match-timing-e-scala-mappe.md` §4: «La classificazione deve essere data-driven e associabile al Match Format
/ Ruleset, **non hard-coded** nella logica». La v0.1 ha una sola mappa (Skirmish 2v2): il rischio concreto è
che round budget, timer e dimensione entrino nel codice come costanti. Quando arriva Standard 3v3 quelle
costanti sono da estirpare una per una, e ogni test che le assume va riscritto.

**E20 — HUD Icon Language, catalogo data-driven** · P2
`2026-08-08-hud-faction-icons.md` §4: dodici categorie semantiche (Identity, Action, Phase, Environment,
Map/Interaction, Status, Information, Reaction, Coordination, Certainty, Warning, Objective). E11 costruisce
l'HUD della v0.1: se le icone nascono come riferimenti diretti a texture nei widget, il catalogo semantico
diventa un refactor di tutti i widget invece di un file di dati in più.

Il resto dei sette sorgenti **non tocca la v0.1**. In particolare Cover Window (E22) dipende da E9 e E14, che
in v0.1 sono P2 e ancora aperte: anticiparla significherebbe costruire sopra fondamenta non verificate.

---

## v0.2 — «Struttura e finestre»

Il campo di battaglia passa da fondale a materia manipolabile: si aprono e si richiudono coperture, i muri e
le porte diventano oggetti logici con stato, e il roster raddoppia. È la release che rende vero il pilastro
«la mappa è un'arma».

**Gate di release**: Standard 3v3 giocabile end-to-end su una mappa di classe Standard, roster 8 completo,
Cover Window dimostrabile in scenario automatico, suite verde, replay deterministico.

### E35 — Roster 8: Sentinel Directorate e Resonance · P0

**Obiettivo**: portare il roster da 4 a 8 eroi aggiungendo le due fazioni v0.2, senza introdurre bonus di
fazione né kit di coppia — [ADR-0006](../decisions/adr-0006-ownership-abilita-sinergie.md) vale invariato.

Le schede esistono già come `DATA_SPEC`/`DESIGN_SPEC`: [`../characters/v0.2/`](../characters/v0.2/) e
[`Fazioni` (Wiki)](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/Fazioni). Questa epic le porta a runtime.

| CP | Obiettivo | DoD misurabile |
|---|---|---|
| **35.1** | Sentinel Directorate — Steel, Murdock | I due `URTHeroData` esistono con azioni a catalogo; affinità *Protection → Fire Sector* emerge da meccaniche generiche, nessun `FactionSetBonus` |
| **35.2** | Resonance — Aurora, Kwang | Idem per *Terrain Shaping → Anchor Geometry* |
| **35.3** | Bilanciamento a 8 | `Heroes.RosterIsBalanced` esteso a 8: nessun eroe domina, le coppie di affinità restano simmetriche |
| **35.4** | Wiki e cataloghi allineati | `../wiki/fazioni/` e `../balance/RT_HeroCatalog_v0.1.md` descrivono 8 eroi con i valori realmente a runtime |

**Dipendenze**: E6 (roster 4) chiusa. **Rischi**: il roster raddoppia la matrice di interazioni da testare —
il costo non è lineare.

### E22 — Cover Window: OPEN → FIRE → SEAL · P1

**Obiettivo**: rendere possibile che un alleato apra temporaneamente una copertura, un secondo sfrutti la
linea di tiro e un terzo la richiuda — **senza combo hard-coded fra tre personaggi**.

Il sorgente è esplicito: deve emergere da primitive già esistenti (modifica di cover, modifica di un
arco/transizione). Se serve una regola speciale «se A ha aperto e B spara», la soluzione è sbagliata.

| CP | Obiettivo | DoD misurabile |
|---|---|---|
| **22.1** | Cover con stato temporaneo | Una copertura ha stati e durata dichiarati nei dati; la transizione è nel `TurnLog` e sopravvive al replay |
| **22.2** | LOS rivalutata al boundary | La linea di tiro si rivaluta al Decision Boundary, non al momento del planning: un'anteprima stale non concede il tiro |
| **22.3** | Seal e ricostruzione | Richiudere è un'azione ordinaria; una copertura **distrutta** non può essere richiusa (core test B del sorgente) |
| **22.4** | Le 12 varianti come scenari | Gli scenari 1–12 del sorgente (happy path, opening fails, shooter displaced, enemy exploits, destroyed before seal, door variant, ice variant, overwatch trigger, determinism repeat, network privacy, stale preview) sono scenari dell'harness |

**Dipendenze**: E9 (coperture e strutture), E14 (overwatch), E23 per la variante porta.
**Rischi**: la finestra è sfruttabile **anche dal nemico** — è una feature, non un bug, e i test devono
fissarla (scenario 4 del sorgente).

### E23 — Muri, porte e interaction graph · P1

**Obiettivo**: muri e porte come **oggetti logici sugli archi**, non come mesh che il gameplay interroga.

| CP | Obiettivo | DoD misurabile |
|---|---|---|
| **23.1** | Separazione geometria/logica | La logica di transizione non legge la mesh: legge archi e stati. Cambiare l'arte non cambia il gameplay (§2.1 del sorgente) |
| **23.2** | Porta come oggetto logico unico | Una porta larga più celle resta **un** oggetto con **uno** stato, non N archi indipendenti (§6.1, §6.4: gruppo atomico) |
| **23.3** | Stable ID e binding | Gli ID sono stabili attraverso il cook; binding duplicati o in conflitto sono errori di validazione, non comportamenti impliciti (§5.3, §8.2) |
| **23.4** | Interaction graph | Chi può agire su cosa è un grafo dato, con cardinalità dichiarata (§8.1, §8.3) |
| **23.5** | Leggibilità | Etichette tattiche, hover sorgente→bersagli e bersaglio→controllori; **mai il solo colore** a distinguere uno stato (§12.4) |
| **23.6** | Standability cotta da geometria | Il muro sta dove vuole — 90°, obliquo, a metà cella — e la calpestabilità è l'esito di `Footprint @ CellAnchor ∩ blocking geometry`, calcolato **in cottura**. Il runtime continua a leggere `bBlocksMovement`. Scenari: `Spec.Map.WallCrossesCellStillStandable`, `.FootprintCollisionBlocksCell`, `.NinetyDegreeCornerBakesCorrectly` |
| **23.7** | La transizione è un dato, non un corollario della cella | `Cell A` valida ∧ `Cell B` valida ∧ `A→B` chiusa è esprimibile **senza** inventare una copertura che non copre. Include la *swept clearance*: si verifica il corridoio attraversato, non i soli estremi. Scenari: `Spec.Map.ValidCellsBlockedTransition`, `.DoorOpensTransition` |

**Dipendenze**: E9. **Rischi**: gli ID stabili si decidono una volta — cambiarli dopo il primo cook invalida
scenari, golden replay e mappe salvate.

> **23.6 e 23.7 arrivano da [D-065](../decisions/RT_PDR_00_Decision_Log.md)**, che ha fissato il principio
> — *la griglia non vincola la geometria del mondo; fra muro e dato autorevole sta una cottura* — dopo che
> **due sorgenti indipendenti** l'avevano chiesto ([triage 2026-08-09](plans/map-editor-brief-spec-panel-2026-08-09.md) §4,
> [conflict report 2026-08-10](plans/handoff-geometry-reazioni-conflict-report-2026-08-10.md) §4).
> Feature: `RT-FEAT-MAP-STANDABILITY`, `RT-FEAT-MAP-TRANSITION-CLEARANCE`, entrambe **DESIGNED**.
>
> ✅ **`MAP-1` è chiusa — [D-071](../decisions/RT_PDR_00_Decision_Log.md), 2026-08-10.** Il footprint
> standard è il **cerchio inscritto** nell'esagono (raggio = **apotema**), e la *swept clearance* di 23.7 usa
> **lo stesso raggio**. **Zero numeri nuovi**: l'apotema si deriva dal lato, già fissato, e la misura resta in
> esagoni — che è ciò che tiene il dato autorevole intero.
>
> ⚠️ **Il limite che ne consegue, dichiarato adesso e non a mappa cotta**: con un raggio solo, *calpestabile*
> e *attraversabile* condividono la soglia. Il varco «ci passo ma non ci sto» — cunicoli, porte strette —
> **non è modellabile in v0.2**. Servirebbe un secondo numero, scartato di proposito.
>
> ⚠️ **Resta un blocco**: `MAP-3` — la cottura **non è invertibile**, quindi una modifica a mano sul dato
> cotto sparisce al ricalcolo successivo, in silenzio. Rischio di produzione registrato dal 2026-08-09 e non
> ancora chiuso.

### E24 — Formato Standard 3v3 · P1

**Obiettivo**: il formato competitivo principale diventa giocabile: 3v3 su mappa di classe Standard.

| CP | Obiettivo | DoD misurabile |
|---|---|---|
| **24.1** | Ruleset 3v3 | Composizione, round budget e timer vengono dal Match Format di E19, non da costanti |
| **24.2** | Mappa Standard | Attraversamento 5–7 Move normali, primo contatto in 1–2 round, 2–3 macro-rotte con choke e counter-route, contatto visivo perdibile e riacquistabile (§4.2) |
| **24.3** | Playtest e misura | Le baseline dichiarate sono **misurate**, non assunte: durata reale, round effettivi, distanza di primo contatto |

> §4.2 del sorgente: «150–200 celle **non è un requisito**. È solo una dimensione plausibile da prototipare e
> misurare». Nessun checkpoint qui fissa un numero di celle.

### E25 — Icon Language completo · P2

**Obiettivo**: estendere il catalogo di E20 alle dodici categorie piene, con world-space HUD e pagine wiki.

E20 popola cinque categorie su dodici — Identity, Action, Phase, Status, Certainty — e lascia le altre sette
dichiarate e vuote. Questa epic le riempie **quando** il consumer esiste: Reaction dipende da E14, Information
da E13/E27, le icone di fazione oltre le due canoniche da E35. Nessuna categoria fa nascere una feature di
gameplay per poter mostrare un'icona.

| CP | Obiettivo | DoD misurabile |
|---|---|---|
| **25.1** | Tassonomia completa e governance | Ogni `IconId` dichiarato ha categoria, significato e **consumer nominato** — o è marcato «senza consumer». Una sola owner spec, naming stabile verificato, `IconId` ≠ `GameplayTag`, regola scritta per la rinomina |
| **25.2** | Catalogo completo, validator e authoring | Il catalogo di CP 20.1 esteso, non un secondo catalogo. Il validator rifiuta `IconId` duplicato, asset nullo, fallback ciclico, categoria invalida. Lo stack ordina per `Priority` → `IconId` stabile, mai per ordine di `TMap` |
| **25.3** | Integrazione HUD, world-space, reaction, perception | Nessun widget referenzia una texture. Il DTO della Reaction UI non contiene trigger futuri, path avversari né dati dal `CanonicalIntentStore` — dimostrato da un test. Un contatto acustico resta un'area |
| **25.4** | Accessibility, Wiki e documentazione | `Confirmed/Predicted/Uncertain` distinguibili in **grayscale**: la differenza è nella forma. Le pagine Wiki distinguono SPECIFICATO, DATO PRESENTE e CONSUMATO A RUNTIME, e non chiamano «implementato» un dato |

**Dipendenze**: E20 (fondazione), poi E11, E14, E13/E27, E35 per i consumer.
**Fuori scope**: rifacimento dell'HUD della v0.1; authoring workflow completo, localization audit, theme
variants, high-contrast pack ed export generato del catalogo, che restano post-v0.2.

**Regola che non si negozia**: `Team` e `Faction` sono assi distinti. L'identità di squadra dipende dalla
partita, quella di fazione è narrativa e resta corretta nei mirror match. Non si codifica
`Conflux = Team Blue`, e la fazione non produce bonus di gameplay ([ADR-0006](../decisions/adr-0006-ownership-abilita-sinergie.md)).

**Tracciata su GitHub** *(2026-08-08)*: epic [#265](https://github.com/DegrassiAaron/refactor-tactics-main/issues/265),
con 4 checkpoint (`CP 25.1`–`25.4`). Fondazione v0.1 in
[#217](https://github.com/DegrassiAaron/refactor-tactics-main/issues/217) (E20): **non le si sottrae scope**.
Feature Registry: `RT-FEAT-UI-ICON-LANGUAGE`.

### E26 — Tactical Bot v1 · P1

**Obiettivo**: il bot passa da «gioca legalmente» a «gioca di squadra».

Aggiunge (§5.2 del sorgente bot): TeamKnowledge integrato, contatti last-known e acustici, threat map,
opportunity map, information value, coordinazione vera, sinergie ambientali, belief weights, predictive
action scoring, reaction policy migliore, stress 4v4.

**Dipendenze**: E13 (conoscenza parziale), E26 richiede il bot v0.1 della v0.1 chiuso.
**Rischi**: la belief map è il punto in cui un bot smette di essere deterministico per distrazione — il
determinismo a parità di seed resta un gate, non un'aspirazione.

> **Invariante di difficoltà — da fissare prima di scrivere il primo livello.**
>
> Una difficoltà più alta dà al bot **più ragionamento, mai più informazione**: più candidati valutati, pesi
> diversi, orizzonte più lungo, reaction policy meno prudente. Non un accesso più ampio allo stato.
>
> Vale la pena scriverlo qui perché è la scorciatoia più economica che esista: rendere un bot «difficile»
> togliendogli la Team Knowledge e lasciandogli leggere lo stato vero costa cinque righe e funziona
> benissimo — finché qualcuno non lo scopre. A quel punto ha già invalidato ogni playtest fatto contro di
> lui, ed è il playtest la ragione per cui il bot esiste.
>
> È anche la sola promessa che il gioco fa già al giocatore in prima persona:
> [«Il bot non vede più di te»](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/avversario-bot) è una **sezione della Wiki**, non una nota
> interna. Un livello «Esperto» che bara la trasforma in una bugia pubblicata.
>
> Se un livello di difficoltà introduce errore intenzionale, quell'errore è **deterministico** e attinge a
> uno stream di seed dedicato: non tocca l'RNG competitivo. Origine:
> [`../archive/src/handoff/2026-08-08-bot-ai-roadmap-e-test-pie.md`](../archive/src/handoff/2026-08-08-bot-ai-roadmap-e-test-pie.md)
> §1 e §24, recepito il 2026-08-08 tranne questa riga — che era l'unica senza un documento corrente che la
> possedesse.

### E36 — Framework degli status: capability, primitive e severity · P2

**Obiettivo**: dare agli undici status esistenti il livello che oggi non c'è. Non aggiungerne un dodicesimo:
rendere il dodicesimo economico.

Misurato su `Source/` (`TAG_Status_*`, 2026-08-10): `Braced` · `Burning` · `Electrified` · `Exposed` ·
`Guarded` · `Marked` · `Obscured` · `Reveal` · `Root` · `Slow` · `Wet`. **Undici status, zero framework** —
ogni effetto è cablato dove serve. Funziona, ed è esattamente il motivo per cui il dodicesimo costerà come i
primi undici messi insieme.

L'ordine dei checkpoint non è una preferenza di stile: **`36.1` è il prerequisito di `36.3` e `36.4`**, e per
transitività di `36.6`, che ne valida le derivazioni. [`D-072`](../decisions/RT_PDR_00_Decision_Log.md) ha
deciso che primitive e severity si **derivano** dal dato invece di essere dichiarate — nessun campo
`primitive:`, nessun campo `severity:` — e derivarle richiede di sapere prima *da cosa*. `36.2` è invece
indipendente e può procedere in parallelo: descrive lo status, non ciò che lo status toglie.

| CP | Obiettivo | DoD misurabile |
|---|---|---|
| **36.1** | Tassonomia delle capability | Esiste l'elenco esplicito di cosa un effetto può togliere, su **due assi** — *disponibilità dell'azione* e *modifica della regola* (cover, targeting, payoff) — e copre le **33 azioni core + 20 d'eroe**, non le sole sette generiche. Parte da `ERTActionSlot` e `ERTMovementStyle`, che **esistono già**: la domanda è se bastino, vadano raffinati o affiancati. Chiude [`STA-4`](../OPEN_DECISIONS.md). Senza, 36.3 e 36.4 non hanno da cosa derivare |
| **36.2** | Il dato dello status | Categoria (`Modifier`/`Control`/`Environment`/`Stance`/`Reaction`/`Special`) e **polarità** separata; `StackPolicy` ed expiration dichiarate **sul dato**, non nel codice che le applica. Le durate sono boundary di fase, non secondi |
| **36.3** | Le sei primitive, derivate | `MODIFY` · `DEGRADE` · `RESTRICT` · `INTERRUPT` · `CONVERT` · `CONSUME` si **leggono** da ciò che lo status dichiara: chi dichiara `Sprint → Move` *è* un `DEGRADE`. Un test verifica che la lettura sia totale — nessuno status resta senza primitiva derivabile |
| **36.4** | Severity contata, anti-stun-lock come test | `C0`–`C3` si contano dalle capability toccate (0 · degrada · una categoria · due o più). La regola *«nessuno status comune e ripetibile toglie insieme Movement, Main Action e Reaction»* smette di essere una revisione umana e **diventa un test** |
| **36.5** | Applicazione: degrada, nega, riprova | Pipeline unica con esito `Full`/`Degraded`/`Rejected`, e danno e status risolti **separatamente** — un'abilità fa danno anche se il control viene resistito. **Resistance** generalizza `PushResistance` da scalare su un dominio solo a profilo che **degrada** (`Root → Slow`); **Immunity** **nega**; `Action.Cleanse` passa dalla lista esplicita di tag alla **categoria**, conservando fail-closed e scelta del giocatore. La reapply policy impedisce di rinfrescare all'infinito lo stesso control con lo stesso setup |
| **36.6** | Reason code, TurnLog e validator | Un **nuovo** `ERTLogCategory::Status` e un **nuovo** `ERTStatusOutcome`, seguendo il pattern per categoria del `TurnLog` — non riusando gli enum di esito altrui. Porta una **`v8`** del formato, con le tre decisioni che il precedente impone: rivendicare il numero verificando **tutti** i branch remoti, dire se `StatusId` entra nell'**hash** (e dichiarare in anticipo il rebaseline dei golden) e, separatamente, se entra in **`EntryLess`**. Le regole del §53 **estendono** `RefactorTactics.Catalog.Validator*` |

**Dipendenze**: `RT-FEAT-ENV-STATUS` (E8, chiusa) e `RT-FEAT-ACTION-ENGINE` (E4, chiusa). **Non** dipende da
E14: la metà su Brace/Overwatch del sorgente era già decisa altrove.

**Rischi**: la granularità di 36.1 è la scelta non ovvia — «movimento» è una capability sola o si divide nei
profili e nelle azioni che esistono davvero (`Move` · `Sneak` · `Sprint` · `Dash` · `Leap` · `Charge` ·
`Reposition`)? Da lì dipende se `Suppressed` conta `C1` o `C2`, cioè **il gate anti-stun-lock cambia di
significato**.

> 🧭 **Anche 36.1 parte da qualcosa che esiste, e ignorarlo costerebbe una terza verità** *(spec panel del
> 2026-08-10 su [#436](https://github.com/DegrassiAaron/refactor-tactics-main/issues/436))*.
>
> | Cosa gira già | Valori |
> |---|---|
> | `ERTActionSlot` (`RTActionDef.h`) | `None` · `Movement` · `Main` · `MovementAndMain` · `Reaction` |
> | `ERTMovementStyle` (`RTActionDef.h`) | `None` · `Budget` · `LinearDash` · `LinearCharge` · `LinearLeap` · `LinearPass` |
>
> La regola anti-stun-lock di 36.4 dice *«nessuno status toglie insieme **Movement**, **Main Action** e
> **Reaction**»*: sono **tre dei cinque valori di `ERTActionSlot`**, alla lettera. La tassonomia grossolana
> non manca — spedisce, e il `TurnLog` la usa. Scrivere `capability:` accanto a `Slot` sarebbe esattamente la
> **seconda verità** che [`D-072`](../decisions/RT_PDR_00_Decision_Log.md) ha appena respinto per le primitive.
>
> Due avvertenze che il checkpoint deve risolvere, non ereditare:
>
> - **`ERTActionSlot::MovementAndMain` non ha produttori.** [`D-028`](../decisions/RT_PDR_00_Decision_Log.md)
>   ne ha tolto l'unico utente (`Action.Sprint`), [`D-070`](../decisions/RT_PDR_00_Decision_Log.md) ha
>   **rifiutato** di adottarlo per l'Overwatch, e il ramo del resolver resta vivo solo grazie a un'azione
>   sintetica in un test. È però la definizione operativa di un `C3` — «togli movimento *e* principale» —
>   quindi il primo hard control lo rianima per inerzia se nessuno decide.
> - **Non tutti gli status tolgono un'azione.** `Exposed` toglie uno *step di copertura*, `Obscured` cambia
>   l'*eleggibilità di targeting*, `Marked` abilita un *payoff altrui*: tre degli undici non si esprimono su
>   uno slot. È il motivo dei due assi — ed è anche la tesi dell'epic, *«le regole che puoi sfruttare sono
>   cambiate»*, non «hai un'azione in meno».
>
> `Climb` **non è un candidato**, perché non esiste come azione: cambiare layer è `Move` attraverso un arco di
> transizione (`RefactorTactics.HexMove.ClimbsOnlyThroughTransition`). `Sneak` invece esiste come profilo ma
> è **senza numeri** — costo, portata e rumore non definiti da nessuna fonte corrente.

> 🧭 **E 36.6 parte dal pattern opposto: qui NON si riusa** *(spec panel del 2026-08-10 su
> [#441](https://github.com/DegrassiAaron/refactor-tactics-main/issues/441))*.
>
> La voce di `TurnLog` porta `ERTLogCategory` (`Move · Combat · Fallback · Reaction · Environment · Facing ·
> Predictive`) e **un solo `uint8 Outcome`**, che *è l'enum della propria categoria* — `ERTMoveOutcome` se
> `Move`, `ERTCombatOutcome` se `Combat`, e così via. Una famiglia **per categoria** non è una seconda verità:
> **è** l'architettura, e il byte è condiviso proprio perché il discriminatore lo disambigua. Riusare
> `ERTCombatOutcome` per una transizione di status romperebbe quel contratto.
>
> È la lezione di 36.1 applicata al contrario, e vale la pena scriverlo: là il concetto era **lo stesso**
> (`Slot` e «capability»), quindi duplicarlo era il difetto; qui i concetti sono **diversi** e il pattern
> chiede di aggiungere.
>
> **La domanda costosa è il formato.** Il `TurnLog` è a **`v7`** (`WithPriority`); uno `StatusId` per voce è
> una **`v8`**, e il precedente impone tre decisioni distinte:
>
> - **Rivendicare il numero** verificando **tutti i branch remoti**, non solo `main` — il commento della `v7`
>   racconta perché: la `v6` era già presa da un altro ramo, e *«due formati con lo stesso numero»* sono il
>   caso peggiore, perché il loader sceglie dal numero e non può accorgersi dello scambio
>   ([`D-070`](../decisions/RT_PDR_00_Decision_Log.md)).
> - **`StatusId` entra nell'hash?** `GraphRevision` sì ([`D-067`](../decisions/RT_PDR_00_Decision_Log.md)),
>   `Priority` e `BaseActionId` no — ma quelli sono *funzioni* di campi già presenti, e uno `StatusId` non lo
>   è: due tracce che differiscono solo per quale status è stato applicato **sono partite diverse**. Se entra,
>   **ogni hash golden cambia**, e il rebaseline va dichiarato prima, non scoperto in CI.
> - **Entra in `EntryLess`?** Domanda **separata**: `Priority` sta fuori dall'hash e **dentro** l'ordinamento,
>   o due voci restano a pari merito e `TArray::Sort`, che non è stabile, rompe `D-SR-1`.
>
> Due avvertenze di perimetro. Il validator **estende** `RefactorTactics.Catalog.Validator*` — la famiglia
> esiste e `RT-FEAT-TOOL-VALIDATION` è `DONE`. E le due regole sui **cicli di conversione** oggi non hanno
> soggetto: nel repository non esiste alcuna conversione status→status — `Wet` che spegne `Burning` è una
> `RemoveStatus`, e il `CONVERT` ambientale è fuori scope in 36.3 perché `Cold`/`Ice`/`Frozen` non esistono.
> Una regola senza istanza violabile non può avere il test rosso che il checkpoint stesso pretende.

> 🧭 **Tre pezzi di 36.5 esistono già, e l'epic li estende invece di costruirli** *(misurato il 2026-08-10)*.
> Il sorgente §21–§23 li elenca come mancanti, e per uno dei tre è falso:
>
> | Il sorgente chiede | Cosa gira già | Cosa manca davvero |
> |---|---|---|
> | Cleanse per categoria | **`Action.Cleanse`**, azione principale in Blast (priorità 25, CP 5.2): rimuove **un solo** stato dalla lista che il *giocatore* dichiara in `PlannedCleansePriority`, fail-closed se non ne trova. Più `Reaction.Cleanse` a catalogo equipaggiamento | Il salto dal **tag esplicito** alla **categoria** — che è ciò che rende `Cleanse.Control` scrivibile senza elencare gli status uno per uno |
> | Resistance che degrada | **`PushResistance`** su `URTHeroData` e `GuardResistedPushDistance`: una resistenza reale, ma su **un dominio solo** e come **scalare**, non come regola di degradazione | La generalizzazione: `Root → Slow` è una *conversione*, non una sottrazione, e nessun dato oggi la sa esprimere |
> | Immunity che nega | **Niente.** `grep -rn "Immun" Source/` è vuoto | Tutto — ed è l'unico dei tre che nasce da zero |
>
> Vale la pena scriverlo perché il costo dei tre non è lo stesso, e un'epic che li tratta allo stesso modo
> stima male: due sono estensioni di un meccanismo vivo con i suoi test, uno è una feature nuova.

> ⚠️ **`Suppressed` e `Dazed` non sono un checkpoint di questa epic, e non è una dimenticanza.**
> Il sorgente li mette nel set ridotto del vertical slice, ma **nessuno dei quattro kit li produce oggi**:
> entrerebbero come due status senza consumatore, che è il difetto ricorrente di questo repository. Entrano
> quando un'abilità li applica — [`STA-3`](../OPEN_DECISIONS.md), aperta. Quando entreranno saranno due casi
> del framework, non due eccezioni: `Suppressed` **è** un `DEGRADE`, `Dazed` **è** un `INTERRUPT` sulla scelta
> manuale della reazione.

**Tracciata su GitHub** *(2026-08-10)*: epic [#435](https://github.com/DegrassiAaron/refactor-tactics-main/issues/435),
con sei checkpoint [#436](https://github.com/DegrassiAaron/refactor-tactics-main/issues/436)–[#441](https://github.com/DegrassiAaron/refactor-tactics-main/issues/441)
collegati **anche come sub-issue native**, non solo dalla task list del corpo — E36 è la prima epic del
repository a usarle.

Origine: [`../archive/src/handoff/2026-08-10-status-control-brace-overwatch.md`](../archive/src/handoff/2026-08-10-status-control-brace-overwatch.md)
§2–§28 e §52–§54, filtrato da
[`plans/handoff-status-control-triage-2026-08-10.md`](plans/handoff-status-control-triage-2026-08-10.md).
Feature Registry: `RT-FEAT-STATUS-FRAMEWORK`.

---

## v0.3 — «Informazione»

La release in cui l'informazione incompleta smette di essere un modificatore e diventa il centro della partita.

### E27 — Percezione completa: vista, udito, memoria · P1

Estende E13 oltre il thin slice: stealth, memoria dell'ultima posizione nota condivisa nel team, rumore
propagato con occlusione, gradi di certezza. Il sorgente 4v4 §19.1 fissa il vincolo: **nessun RNG nascosto per
la percezione base** — se un'unità è vista, lo è per una regola, non per un tiro di dado.

### E28 — Expert Bot v2 · P2

Solo **dopo** la stabilizzazione del resolver (§5.3 del sorgente): simulazioni counterfactual, più ipotesi
sul nemico, opponent model su eventi osservati, pianificazione robusta, personalità tattiche, possibile riuso
del planner come strumento di QA.

### E29 — Predictive avanzato · P2

Ciò che E18 ha dichiarato fuori scope: trap persistenti, mine, tripwire su arco, catene di azioni predittive.
Si apre solo se il thin slice E18 ha retto al playtest.

### E33 — Conditional Intent · P2

[D-034](../decisions/RT_PDR_00_Decision_Log.md). Un intento con **1** condizione e **2** rami, dichiarati per
intero in Planning e valutati a un boundary nominato. Non è un sistema nuovo: è la **condizione dichiarata**
che D-012 ha già ammesso per il regime `Conditional` dell'Overwatch, spostata dal profilo di reazione
all'intento. Forma e vincoli in
[`../gameplay/brief-delayed-actions.md`](../gameplay/brief-delayed-actions.md) §7.

**Dipende da E18**, non da E29: `EvaluationBoundary` è il campo `boundary` che oggi non esiste in
`FRTActionDef`. Costruire i rami prima dei boundary significherebbe inventarne un secondo modello — lo stesso
errore che il §5 del brief sulle azioni generiche evita per le policy.

Sta in v0.3 e non prima perché il valore si vede solo quando l'informazione è imperfetta: coprirsi con un ramo
alternativo ha senso contro un avversario di cui **non sai** cosa farà, ed è E27 a rendere vero quel «non sai».

Il rischio da sorvegliare è uno solo, e non è tecnico: una lista di predicati che cresce a ogni richiesta
diventa un linguaggio di scripting. Il gate è che la lista resti **chiusa e validata**, e che ogni estensione
sia una decisione, non una configurazione.

---

## v0.4 — «Operations»

### E30 — Classe di mappa Operations · P2

Mappe grandi, partite da **45–60+ minuti**, più obiettivi, esplorazione, repositioning strategico, valore
reale di rumore e logistica (§4.3 del sorgente match-timing). Il sorgente è esplicito: «l'architettura deve
poterlo supportare, ma **non implementarlo ora**» — E19 e E23 sono ciò che rende questa epic possibile senza
riscritture.

### E31 — Obiettivi multipli e logistica · P3

### E32 — Formato 4v4 competitivo · P3

La v0.1 usa il 4v4 solo come **stress test** (E17). Qui diventerebbe un formato vero — ma solo se il playtest
del 3v3 dice che il sistema regge la densità. Resta la nota del piano canonico: il formato competitivo finale
**non è deciso**, 3v3 è baseline e 4v4 stress test.

### E34 — Stati del personaggio e trasformazioni · P3

[D-035](../decisions/RT_PDR_00_Decision_Log.md). Un sistema di **stati del personaggio** presentato in cinque
famiglie — `Stance · Form · Overdrive · Environmental · Configuration` — non un sistema chiamato
`Transformation`. Forma, guardrail e anti-pattern in
[`../gameplay/brief-stati-personaggio-e-trasformazioni.md`](../gameplay/brief-stati-personaggio-e-trasformazioni.md).

**Metà dell'epic potrebbe non servire.** Uno `Stance` è un **profilo commutabile in Planning**
([D-033](../decisions/RT_PDR_00_Decision_Log.md)), e i profili esistono già come concetto: se il primo
prototipo è un cambio di profilo, valida `Stance` e `Configuration` senza toccare i kit. Solo `Form`,
`Overdrive` ed `Environmental` richiedono override di abilità e movimento.

**Perché qui e non prima.** Non per tema — la v0.4 è «Operations» — ma per dipendenze e priorità: serve il
roster stabilizzato (**E35**), i profili reali (**E14**), e per la famiglia `Environmental` il canale
ambientale di **E27**. È anche la più cara in carico cognitivo, e il documento sorgente lo dice meglio di
qualunque stima: *una buona trasformazione deve aumentare le decisioni strategiche più di quanto aumenti le
informazioni da ricordare*.

**Nessun eroe ha uno stato assegnato**, ed è deliberato: le alternative Light/Medium/Signature del sorgente
restano tre per personaggio. I banchi di prova coerenti col kit sono Howitzer, Murdock e GRIM.exe — **non**
Vektor, la cui forma `Siege` spegnerebbe la meccanica firma.

**Tracciata su GitHub**: epic [#244](https://github.com/DegrassiAaron/refactor-tactics-main/issues/244), con
11 checkpoint (`CP 34.1`–`34.11`) e 4 prototipi personaggio. Le candidature per l'intero roster stanno in
[`../characters/matrici-stati-personaggio.md`](../characters/matrici-stati-personaggio.md); l'ordine dei
prototipi va **dal più leggero al più invasivo** — `Riva · Flow` non tocca alcun sistema condiviso,
`Bastion · Bulwark` tocca cover, LOS, collisione e pathing.

---

## Cosa questo documento non decide

- **Numeri di bilanciamento**: nessun valore di danno, costo o durata è fissato qui. Stanno in
  [`../balance/`](../balance/README.md) quando esistono, altrimenti restano aperti.
- **Dimensione delle mappe in celle**: esplicitamente non bloccata dal sorgente.
- **Il formato competitivo finale**: 3v3 è baseline da playtestare, non una decisione chiusa.
- **Le date**: nessuna epic qui ha una scadenza. La v0.1 non ha ancora chiuso i suoi 15 gate.
