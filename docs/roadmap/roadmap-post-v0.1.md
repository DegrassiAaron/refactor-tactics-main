# RefactorTactics — Roadmap oltre la v0.1

> `CURRENT` · **Creato**: 2026-08-08 · **Owner**: questo file per le release **v0.2 → v1.0**.
>
> ⚠️ **Questa riga diceva «v0.2 → v0.4» fino al 2026-08-27, ed era falsa di sei release.** Il documento
> contiene già `## v0.5 — «Online Foundation»`, `## v0.6 — «Ability Runtime»`, `## v0.7 — «Competitive
> Alpha»`, `## v0.8 — «Beta / Balance»`, `## v0.9 — «Release Candidate»` e `## v1.0 — «Launch»`: dieci
> release, non tre. Non è un dettaglio di intestazione — è la riga che si legge per decidere **se aprire il
> file**, e chi cercava la strada verso la v1.0 concludeva che non esistesse e ne scriveva un'altra.
> Trovata così, il 2026-08-27, mentre si stava per farlo.
> La v0.1 resta in [`roadmap-v0.1.md`](roadmap-v0.1.md); lo stato di esecuzione in
> [`roadmap-checkpoint.md`](roadmap-checkpoint.md). La traiettoria intera — questo file più la v0.1, con le
> soglie che separano le release — si legge in una pagina sola in [`roadmap-v0.1-v1.0.md`](roadmap-v0.1-v1.0.md),
> che è una **vista** e non sposta di qui l'autorità sullo scope.
>
> **Questo documento non apre lavoro.** Nessuna epic qui dentro si implementa prima che i 15 gate della v0.1
> siano verdi ([`v0.1-definition-of-done.md`](v0.1-definition-of-done.md) §3). Serve a decidere **oggi** le
> cose che, se decise dopo, costringerebbero a rifare: numerazione delle epic, confini fra release, e quali
> vincoli architetturali la v0.1 deve già rispettare.

## Perché esiste

Sette documenti prodotti il 2026-08-07/08 nell'allora `docs/src/` — oggi in `docs/archive/src/` — descrivono
sistemi che **non stanno nella v0.1**:
roster a 8, Cover Window, architettura di muri e porte, formato competitivo 3v3, bot tattico ed esperto,
mappe Operations. Finché sono rimasti solo nella casella d'ingresso, ogni sessione ha dovuto ridecidere da
capo se una cosa fosse v0.1 o no. Questo file registra quella ripartizione una volta sola.

**Fonti** (tutte in `docs/archive/src/`, non normative — vedi [`../archive/src/`](../archive/src/)):

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
| **v0.2** | Struttura e finestre | Il campo diventa manipolabile; roster 8 | E22 · E24–E26 · **E35** · **E36** · **E38** · **E39** · **E51** | Standard 3v3 |
| **v0.3** | Informazione | Quello che non sai vale quanto quello che fai | E27–E29 · **E33** | Standard 3v3 |
| **v0.4** | Operations | Partite lunghe su mappe grandi | E30–E32 · **E34** · **E37** | Operations 4v4+ |
| **v0.5** | Online Foundation | Il turno simultaneo regge la rete | **E40** | Standard 3v3 online, lobby privata |
| **v0.6** | Ability Runtime | Le abilità hanno un runtime, il resolver resta l'autorità | **E41** | Standard 3v3 online |
| **v0.7** | Competitive Alpha | Si gioca su un server che non è il client di nessuno | **E42** | Standard 3v3 su dedicated |
| **v0.8** | Beta / Balance | Le partite si misurano a lotti, e la misura dice cosa vale | **E43** | 3v3 + batch bot-vs-bot |
| **v0.9** | Release Candidate | Niente di nuovo: quello che c'è deve reggere | **E44** | Feature freeze |
| **v1.0** | Launch | Una partita competitiva completa su infrastruttura di produzione | **E45** | Standard 3v3 **non classificato** ([D-259](../decisions/RT_PDR_00_Decision_Log.md)) |

> ⚠️ **`E22 · E24–E26` e non `E22–E26`: il buco è E23, ed è voluto.** L'epic è stata **anticipata alla v0.1** il 2026-08-17 (`D-160`), lo dichiarano la propria sezione qui sotto (*«⛔ E23 NON È PIÙ DI QUESTA RELEASE»*) e la riga `E23` di [`roadmap-v0.1.md`](roadmap-v0.1.md) §2.1. Finché la cella diceva `E22–E26` l'**intervallo continuava a rivendicarla**, e la contraddizione era invisibile a `grep`: la stringa `E23` non compare in una riga che la contiene. Costo misurato — `feature_registry.py` leggeva `E23 → v0.2`, quindi le **cinque** feature che dichiarano `release: v0.1` ed `epic: E23` finivano nella tabella dei *disallineati* di `roadmap-v0.1.md` invece che sotto la propria epic, e `wiki --check` — che è una condizione del gate **G15** — restava rosso.
> 
> **Questa riga non è un indice esaustivo delle epic v0.1**: `E46` ed `E47` sono v0.1 e non compaiono qui. L'owner delle epic della v0.1 è `roadmap-v0.1.md` §2.1; questa tabella dichiara ciò che sta **oltre** la v0.1, ed E23 ne è uscita.

> **Le sei release da v0.5 a v1.0 sono state aggiunte il 2026-08-13**
> ([D-136](../decisions/RT_PDR_00_Decision_Log.md)). Fino a quel giorno questo documento chiudeva qui
> dicendo *«oltre la v0.4 resta **north-star non pianificato**: multiplayer in rete (milestone M10 del
> piano canonico), progressione, modding, editor mappe a runtime. Non si aprono epic per ciò che non ha
> una release»* — e la regola era giusta, ma si era chiusa addosso: `RELEASE_ORDER` non sapeva esprimere
> `v0.5`, quindi la rete **non poteva** avere una release, quindi non poteva avere un'epic. `future`
> significava due cose incompatibili — *pianificata oltre l'orizzonte esprimibile* e *non pianificata* —
> e con esse `RT-FEAT-NET-AUTHORITY` (`SPECIFIED`, milestone **M10**, tre dipendenze dichiarate) stava
> accanto a `RT-FEAT-MAP-WATER-DYNAMICS` (`IDEA`, nessun owner). Non è una promessa di date: **nessuna
> di queste sei righe ne ha una**, esattamente come le quattro sopra.
>
> ⚠️ **Queste sei release non creano una seconda tassonomia della rete.** La vista di **esecuzione** resta
> di [`roadmap-checkpoint.md`](roadmap-checkpoint.md): **M10** (Rete e privacy — `M10.1` listen server,
> `M10.2` piani team-only, `M10.3` canary anti-leak) e **M11** (Production readiness) esistono da prima e
> restano owner dei loro checkpoint. E40 ed E45 sono la vista di **release** dello stesso lavoro, e la
> citano invece di ricopiarla — è la stessa relazione che E12 ha con M7/M11. I due spazi di numerazione
> collidono per costruzione (`CP 10.1` è «Activate e Interact» in E10 e «listen server» in M10): per
> questo `RT-FEAT-NET-AUTHORITY` dichiara `milestone: M10` e non `checkpoints`.

Oltre la v1.0 resta **north-star non pianificato**: progressione, modding pubblico, editor mappe a runtime.
Non si aprono epic per ciò che non ha una release.

### Il Graybox Kit attraversa questa ladder, e non ne genera una seconda

Aggiunto il **2026-08-17** con [D-153](../decisions/RT_PDR_00_Decision_Log.md). Il kit
`Graybox_Kit_Cover_CellVolume` propone dieci cluster di maturità degli asset con una propria mappatura
`v0.1 → v1.0`. **L'ordine si preserva, i numeri di release si prendono da qui** — è il precedente di
[D-138](../decisions/RT_PDR_00_Decision_Log.md), che ha già respinto una ladder proposta da un handoff.

| Cluster del kit | Proposta | Release **canonica** | Owner reale | Azione |
|---|:--:|:--:|---|---|
| Core map | v0.1 | **v0.1**, interamente | **E21** · **E47** · muri e porte su **E23**, anticipata alla v0.1 (`D-160`) | allineato |
| Environment | v0.2 | **v0.1** ⬅️ | **E8** — **otto** feature, tutte `INTEGRATED` | il kit è **indietro** |
| 3D map / verticalità | v0.3 | **`future`** ➡️ | nessuno — `RT-FEAT-MAP-VERTICALITY` è `IDEA` | `DEFER` |
| Interactive map | v0.4 | **v0.1** ⬅️⬅️ | **E23** ([#324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324)), anticipata il 2026-08-17 (`D-160`) | il kit è **indietro di due release**, non di una |
| Tactical devices | v0.5 | **fuori scope dichiarato** | — | `DEFER` |
| Destruction / debris | v0.6 | **v0.2** ⬅️⬅⬅️ | **E51** ([#1848](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1848)), creata il 2026-08-30 | il kit è **indietro di quattro release** |
| Perception / information | v0.7 | **v0.1** ⬅️ e v0.3 | **E13** (base) · **E27** (completa) | il kit è **indietro** |
| Objectives | v0.8 | **v0.1** ⬅️ e v0.4 | **E10** (base) · **E31** (multipli) | il kit è **indietro** |
| Modularization | v0.9 | **nessuna** | authoring, non contenuto di release | `DEFER` |
| Contract freeze | v1.0 | **v1.0** ✅ | **E45** | allineato |

**Una riga su dieci coincide esattamente** — il contract freeze. Le altre nove divergono in **cinque**
modi, e vale la pena non confonderli:

| Modo | Quali | Quante |
|---|---|--:|
| il repository li ha già, o li sta costruendo, prima di dove il kit li mette | Environment · Interactive map · Perception · Objectives | **4** |
| ha una release e un owner, ma il lavoro non è ancora aperto | Destruction / debris (**E51**, v0.2) | **1** |
| nessuna release li possiede | 3D map · Tactical devices (**fuori scope dichiarato**) | **2** |
| a cavallo di due release | Core map (con la coda di muri e porte in v0.2) | **1** |
| non è contenuto di release, è authoring | Modularization | **1** |

> ⚠️ **Il quinto modo è nato il 2026-08-30, e prima non serviva.** Fino a quel giorno `Destruction /
> debris` stava sotto *«nessuna release li possiede»* insieme a 3D map e Tactical devices, e quel gruppo
> contava **3**. La riga non è stata cancellata: è **cambiata di modo**, perché la distruzione ha acquisito
> una release (**v0.2**) e un owner (**E51**, [#1848](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1848))
> mentre le altre due no. Metterla nel primo modo sarebbe stato falso in senso opposto: il repository **non**
> la sta costruendo, e l'epic non apre lavoro prima che i quindici gate della v0.1 siano verdi. Il totale
> resta nove — ma, come questa stessa sezione avverte due paragrafi più in basso, **un totale che torna non
> convalida la partizione**, ed è il modo a essere cambiato, non la somma.
> Provenance: [`plans/structural-debris-canonicalization-spec-panel-2026-08-30.md`](plans/structural-debris-canonicalization-spec-panel-2026-08-30.md).

> ⚠️ *Questa frase ha sbagliato il conto **due volte**. Diceva «due modi opposti» mentre i due paragrafi
> che li spiegano coprivano `4 + 3 = 7` righe su nove — Core map e Modularization restavano fuori da
> entrambi. La correzione elencò i quattro gruppi e continuò a chiamarli «tre». La somma tornava a nove in
> entrambi i casi, ed è la stessa trappola del conto del catalogo: **un totale che torna non convalida la
> partizione**. Ora i modi si contano nella tabella invece che nella prosa.*

> ⚠️ **La riga «Core map» diceva `v0.1 ✅ allineato` elencando E23 fra gli owner, e la riga quattro più
> sotto dichiara E23 **v0.2**.** La stessa tabella usava la stessa epic da due parti della propria
> partizione: il cluster «core» del kit contiene muri e porte, che su `main` sono v0.2. Non è un dettaglio
> di etichetta — è la tabella che `D-153` indica come riconciliazione di riferimento, e «allineato» non era
> misurabile. Trovato in code review.

🔴 **Quattro cluster il kit li mette *dopo* di dove stanno sulla roadmap canonica** — «dopo» riguarda la
**release**, non lo stato di avanzamento, e i quattro non sono allo stesso stadio:

| Cluster | Epic | Stato reale delle feature |
|---|:--:|---|
| Environment | **E8** | **8 su 8 `INTEGRATED`** — `ERTHexSurface` ha nove valori, fuoco/acqua/ghiaccio girano |
| Interactive map | **E23** | `DESIGNED` — la release è **v0.1** dal 2026-08-17 (`D-160`), non v0.2 né v0.4 |
| Perception | **E13** | 3 `TESTABLE`, 1 `IMPLEMENTING`, 1 `SPECIFIED` — **zero** `INTEGRATED` |
| Objectives | **E10** | 1 `RELEASE_READY`, 1 `IMPLEMENTING` — **zero** `INTEGRATED` |

> 🔴 **Questa riga diceva «tutti e tre in gran parte `INTEGRATED`», e reggeva solo per E8.** Misurato sul
> registry che questa stessa PR rigenera: E13 ed E10 non hanno **nessuna** feature `INTEGRATED`. La tesi non
> cade — quei cluster restano **v0.1**, quindi il kit li rinvia comunque a release future — ma la prova era
> più forte del vero, ed è il tipo di frase che il prossimo consolidamento ricopia invece di rimisurare.
> Trovato in code review.

Tre di questi quattro sono **v0.1**; **Interactive map è v0.2**, e il kit lo mette in v0.4 — indietro di
due release invece che di quattro, ma indietro. Seguire la mappatura del kit avrebbe **rinviato asset di
sistemi che il progetto possiede o sta costruendo**, cioè lasciato la board a rappresentare col solo colore
ciò che il resolver calcola già.

Non è un errore di ambizione della sorgente: è che il kit misura la maturità del **contenuto** e la ladder
canonica misura quella del **sistema**, e sul contenuto il progetto è più avanti di quanto la sorgente
sapesse.

> ⚠️ *La prima correzione di questo paragrafo diceva «tutti e quattro lavoro della **v0.1**», e per
> Interactive map era falso — contraddetto dalla tabella sopra e dalla riga «Core map» che questa stessa PR
> aveva aggiunto per registrare che E23 è v0.2. Corretta la metà falsa, era rimasta l'altra.*

➡️ **Due cluster non hanno una release che li possieda, e non per la stessa ragione.** La verticalità
resta senza owner; i devices tattici **non sono `future` affatto** — sono **fuori
scope v0.1 dichiarato** da [`../gameplay/spec-interazioni-mappa-cp101.md`](../gameplay/spec-interazioni-mappa-cp101.md)
§11, che li rinvia a E13/E14 con motivazione registrata. Qui l'ordine del kit è giusto e la release non
esiste: **si lasciano `future` invece di inventarne una**, ed è la parte di `D-136` che questa tabella
applica invece di ridiscutere.

> ⚠️ **Nessun tema di release cambia per far tornare questa tabella**, e la sorgente stessa lo vieta:
> *«non cambiare il tema delle release globali solo per far coincidere questa tabella»*. Dalla **v0.5** in
> poi i temi canonici sono rete, GAS, dedicated server e hardening — **nessuno è un tema di contenuto**, e
> un cluster di asset non ha dove atterrarci. È esattamente il motivo per cui le cinque righe centrali del
> kit non trovano casa: non sono state rifiutate, non c'è la stanza.
>
> 🔑 **L'unica release che acquisisce un impegno nuovo è la v1.0**, e non in asset: **E45** è «un gate di
> produzione, non una release di feature», ed è dove il contratto di ingombro e pivot **si congela** perché
> l'arte finale possa sostituire il graybox senza cambiare le regole competitive. Il contratto vive in
> [`../technical/systems/spec-graybox-placement-contract.md`](../technical/systems/spec-graybox-placement-contract.md).

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

**Tracciata su GitHub**: epic [#322](https://github.com/DegrassiAaron/refactor-tactics-main/issues/322).

**Obiettivo**: portare il roster da 4 a 8 eroi aggiungendo le due fazioni v0.2, senza introdurre bonus di
fazione né kit di coppia — [ADR-0006](../decisions/adr-0006-ownership-abilita-sinergie.md) vale invariato.

Le schede esistono già come `DATA_SPEC`/`DESIGN_SPEC`: [`../characters/v0.2/`](../characters/v0.2/) e
[`Fazioni` (Wiki)](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/Fazioni). Questa epic le porta a runtime.

| CP | Obiettivo | DoD misurabile |
|---|---|---|
| **35.1** | Sentinel Directorate — Ward, Vigil | I due `URTHeroData` esistono con azioni a catalogo; affinità *Protection → Fire Sector* emerge da meccaniche generiche, nessun `FactionSetBonus` |
| **35.2** | Resonance — Rime, Tethra | Idem per *Terrain Shaping → Anchor Geometry* |
| **35.3** | Bilanciamento a 8 | `Heroes.RosterIsBalanced` esteso a 8: nessun eroe domina, le coppie di affinità restano simmetriche |
| **35.4** | Wiki e cataloghi allineati | `../wiki/fazioni/` e `../balance/RT_HeroCatalog_v0.1.md` descrivono 8 eroi con i valori realmente a runtime |
| **35.5** | Paragon naming purge | Nessuna identità Paragon sopravvive nel namespace RT-owned, e un gate contestuale — sui campi `HeroId`, `DisplayName`, namespace dell'`ActionId`, Gameplay Tag e voci di catalogo — impedisce che rientri. Path e package vendor restano leciti. Issue [#2291](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2291) |

**Dipendenze**: E6 (roster 4) chiusa. **Rischi**: il roster raddoppia la matrice di interazioni da testare —
il costo non è lineare.

### E22 — Cover Window: OPEN → FIRE → SEAL · P1

**Tracciata su GitHub**: epic [#323](https://github.com/DegrassiAaron/refactor-tactics-main/issues/323).

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

> ⛔ **E23 NON È PIÙ DI QUESTA RELEASE — anticipata alla v0.1 il 2026-08-17
> ([D-160](../decisions/RT_PDR_00_Decision_Log.md)).** L'owner della sua release è
> [`roadmap-v0.1.md`](roadmap-v0.1.md) §3 e §5, dove i **sette** checkpoint sono dichiarati con lo stato.
>
> **Il dettaglio qui sotto resta** e non è duplicato altrove: è la storia dell'epic — `D-065` (la griglia non
> vincola la geometria del mondo), `D-071` (footprint = cerchio inscritto), `MAP-1` chiusa, `MAP-3` ancora
> aperta, la mappatura CP → issue → feature di `D-138`, e la correzione su `23.2`. Spostarlo avrebbe
> duplicato sessanta righe o perso il contesto in cui sono state decise; lasciarlo senza questo banner
> avrebbe lasciato un documento a dichiarare una release falsa.
>
> ⚠️ **Non era una scommessa: metà dell'epic era già dentro la v0.1 quando la decisione è stata presa.**
> Misurato sul registry, non dedotto: `23.1` è coperto da `RT-FEAT-TOOL-MAP-GEOMETRY` (**v0.1**,
> `IMPLEMENTING`) e `23.2` da `RT-FEAT-MAP-INTERACTIVE-EDGES` (**v0.1**, `INTEGRATED`, epic **E9**) — la
> tabella *«Dove sta il lavoro»* più sotto lo dice già con le sue parole, *«già consegnato in v0.1»*. Delle
> cinque feature che dichiarano `epic: E23`, `RT-FEAT-MAP-INTERACTION-GRAPH` era già passata a `v0.1` col
> giro di `#833`; `D-160` sposta le altre quattro. Dopo, **nessuna feature di E23 dichiara più v0.2**.

**Tracciata su GitHub**: epic [#324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324).

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
> 🔁 **Rimisurato il 2026-08-31: metà di questa riga è superata.** Il footprint standard **non è più**
> il cerchio inscritto: [`D-303`](../decisions/RT_PDR_00_Decision_Log.md) ha superato `D-071` punto (1) e ha
> reso il footprint un **conteggio di settori contigui** (`Small`/`Medium`/`Large` = tre valori di
> `MinContiguousWedges`). ⚠️ La *swept clearance* di `23.7` — punto (2) — **resta**, ma senza un raggio
> da traslare: è `MAP-4` in [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md). La riga sotto conserva ciò che
> `MAP-1` chiuse allora.
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

#### Dove sta il lavoro — misurato il 2026-08-14 ([D-138](../decisions/RT_PDR_00_Decision_Log.md))

| CP | Issue | Stato | Feature |
|---|---|---|---|
| **23.1** | [#619](https://github.com/DegrassiAaron/refactor-tactics-main/issues/619) [#620](https://github.com/DegrassiAaron/refactor-tactics-main/issues/620) [#621](https://github.com/DegrassiAaron/refactor-tactics-main/issues/621) [#712](https://github.com/DegrassiAaron/refactor-tactics-main/issues/712) | tre chiuse, una aperta — *authoring anticipato* | `RT-FEAT-TOOL-MAP-GEOMETRY` |
| **23.2** | — | ✅ **già consegnato in v0.1** | `RT-FEAT-MAP-INTERACTIVE-EDGES` |
| **23.3** | [#832](https://github.com/DegrassiAaron/refactor-tactics-main/issues/832) | ⬜ aperta | `RT-FEAT-MAP-STRUCTURE-IDENTITY` |
| **23.4** | [#833](https://github.com/DegrassiAaron/refactor-tactics-main/issues/833) | ⬜ aperta | `RT-FEAT-MAP-INTERACTION-GRAPH` |
| **23.5** | [#834](https://github.com/DegrassiAaron/refactor-tactics-main/issues/834) | ⬜ aperta | `RT-FEAT-UI-STRUCTURE-READABILITY` |
| **23.6** | — | `DESIGNED` | `RT-FEAT-MAP-STANDABILITY` |
| **23.7** | — | `DESIGNED` | `RT-FEAT-MAP-TRANSITION-CLEARANCE` |

> 🔴 **`23.2` non è lavoro da fare, ed è la correzione che vale la pena leggere.** Il gruppo atomico
> multi-transition **esiste dalla v0.1**: `FRTHexDoor::DoorId` raggruppa i bordi, `SetDoorState` li muta
> insieme incrementando la revisione **una sola volta**. Due test lo dimostrano, e vanno citati per quello che
> fanno davvero: `Structures.Door.GroupClosesTogether` prova il **raggruppamento** — tre bordi di **una stessa
> cella** con `DoorId 3`, più una quarta porta senza gruppo che resta ferma, e il suo commento avverte che «il
> gruppo non deve essere rettilineo» — mentre `Structures.Door.StateChangeBumpsRevision` prova il caso della
> **porta larga**: «Portone largo tre bordi: un comando, una revisione», tre **celle** allineate sul bordo E
> con `DoorId 7`, e l'asserzione `Revision == Before + 1`. È quest'ultimo, non il primo, a essere «la porta
> larga circa 3 m» che i documenti di visione continuano a proporre come obiettivo futuro.
>
> Chi legge questa riga cercando la porta multi-transition **da costruire** riscriverebbe qualcosa che ha già
> dei test verdi, e li romperebbe.
>
> Il delta reale è **l'identità** (`23.3`): `DoorId` è un `int32` locale all'asset — non sopravvive al cook,
> nessuno scenario può citarlo per nome, e senza un nome l'interaction graph di `23.4` non ha come citare il
> proprio bersaglio. Il codice lo dichiarava già: *«`DoorId` e arco esistono già e bastano. L'identità stabile
> è E23 (#324)»* — [`RTPointerInteraction.h`](../../Source/RefactorTactics/Player/RTPointerInteraction.h).

#### L'orizzonte del dominio oltre la v0.2

Il dominio non finisce con `E23`, ma **non genera epic proprie**: si innesta su quelle che già esistono. La
catena è di dipendenza, non di ambizione — e il vincolo lo detta l'owner delle interazioni,
[`spec-interazioni-mappa-cp101.md`](../gameplay/spec-interazioni-mappa-cp101.md) §11, che mette il controllo
remoto sorgente → bersaglio fuori scope perché *«richiede la privacy dei collegamenti e quindi la rete»*.

| Release | Epic ospite | Che cosa il dominio vi aggiunge |
|---|---|---|
| **v0.1** ⬅️ | `E23` | il grafo è un **dato**: cardinalità, ordine deterministico, validator dei binding — epic **anticipata** il 2026-08-17 (`D-160`), quindi il primo gradino di questa scala è nella release corrente |
| **v0.3** | `E27` — Percezione completa | la relazione ha un **pubblico**: `Known`/`Unknown` per squadra, `Controller: ???`, discovery |
| **v0.4** | `E30` — Classe di mappa Operations | **scala**: molte strutture, gate ampi, churn di path cache, leggibilità su mappe grandi |
| **v0.5** | `E40` — Il turno simultaneo in rete | la privacy diventa **verificabile**: stato autoritativo, canary di leak, late join e reconnect |
| **v0.9** | `E44` — Feature freeze | determinismo, soak, corpus di replay, validator sulle mappe di shipping |
| **v1.0** | `E45` — Gate di produzione | il dominio entra nel gate, non porta feature nuove |

⚠️ **Due proposte non hanno un owner, e restano proposte.** Un *production authoring* di strutture (validator
a lotti, migrazione di schema, stabilità del cook) e un *interaction graph v2* (`N sorgenti → 1 bersaglio` con
semantica `AND`/`OR`, power network, interazioni condizionali) sono stati proposti per le release intermedie:
nella taxonomy reale quelle release appartengono a `E41` (GAS come runtime) e `E42` (dedicated server), che
sono altri domini. **Non si creano epic per simmetria.** Se il gameplay le chiederà, la decisione precede
l'epic — e per il graph v2 la decisione ha già un ID: `INT-5`.

### E24 — Formato Standard 3v3 · P1

**Tracciata su GitHub**: epic [#325](https://github.com/DegrassiAaron/refactor-tactics-main/issues/325).

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

> **Skill Card Grammar** ([D-231](../decisions/RT_PDR_00_Decision_Log.md), owner
> [`../technical/systems/spec-icon-card-grammar.md`](../technical/systems/spec-icon-card-grammar.md)): è la
> fondazione compositiva di questa epic. **CP 25.1** la possiede come governance — grammatica e catalogo
> hanno owner separati ma collegati, e le primitive compositive (`Target`, `Shape`, `Delivery`, `HitRule`,
> `Effect`) **non** diventano categorie runtime. **CP 25.2** vi aggancia l'authoring: uno schema o un
> validator di caps nasce solo con un consumer reale. **CP 25.4** ne eredita i test percettivi (grayscale,
> 24 px, card densa al cap). 🔴 Resta **aperto** a che cosa serva il colore: tre owner si contraddicono, e
> uno dei tre è già implementato nel generatore. Non si chiude per simmetria.
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

**Tracciata su GitHub**: epic [#326](https://github.com/DegrassiAaron/refactor-tactics-main/issues/326).

**Obiettivo**: il bot passa da «gioca legalmente» a «gioca di squadra».

> **Architettura fissata il 2026-08-11**, e non era un dettaglio: «coordinazione vera» lasciava leggere due
> modelli incompatibili — un assegnatore che distribuisce compiti, oppure una ricerca sulle combinazioni.
> [D-097](../decisions/RT_PDR_00_Decision_Log.md) sceglie la seconda: **Top-K per unità più una combinazione
> valutata al centro**, con i ruoli tattici che *emergono* dal piano invece di essere assegnati prima.
> [D-098](../decisions/RT_PDR_00_Decision_Log.md) aggiunge il vincolo che impedisce le combo immaginarie: una
> sinergia intra-turno vale solo se l'ordine delle fasi la permette, e la compatibilità si **chiede** alle
> regole invece di riscriverla nel bot. Owner: [`../gameplay/spec-bot-tattico.md`](../gameplay/spec-bot-tattico.md).
> Feature Registry: `RT-FEAT-BOT-TACTICAL` — che dal 2026-08-11 copre **solo** questa epic: belief e
> predictive sono usciti in `RT-FEAT-BOT-BELIEF` e `RT-FEAT-BOT-PREDICTIVE`.

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
| **36.2** | Il dato dello status | Categoria (`Modifier`/`Control`/`Environment`/`Stance`/`Reaction`/`Special`) e **polarità** separata; `StackPolicy` ed expiration dichiarate **sul dato**. ⚠️ Non parte da zero e non basta un campo: `ApplyStatus` implementa già un `Refresh` con `max()`, e uno status può avere **due sorgenti insieme** — a termine (`StatusTurns`) e legato alla cella (`CellBoundStatuses`) — che su `Wet` coesistono. Il modello deve reggere anche il payload di `Marked` (la squadra che ha marcato) e scegliere fra le **due codifiche** di «finché sulla cella»: `0` a catalogo, `-1` a runtime |
| **36.3** | Le sei primitive, derivate | `MODIFY` · `DEGRADE` · `RESTRICT` · `INTERRUPT` · `CONVERT` · `CONSUME` si **leggono** da ciò che lo status dichiara: chi dichiara `Sprint → Move` *è* un `DEGRADE`. Un test verifica che la lettura sia totale. ⚠️ **`INTERRUPT` va rinominata**: `Action.Interrupt` è un'azione viva. `CONVERT` non ha oggi alcuna istanza. E `Slow` è **già** un `MODIFY` conforme (`MoveCostModifier = 1`, costo intero) |
| **36.4** | Severity contata, anti-stun-lock come test | `C0`–`C3` si contano dalle capability toccate (0 · degrada · una categoria · due o più). La regola *«nessuno status comune e ripetibile toglie insieme Movement, Main Action e Reaction»* smette di essere una revisione umana e **diventa un test**. ⚠️ `Root` **non** ha una severity attesa a priori: azzera Move **e** le mobilità lineari, quindi esce `C2` o `C3` a seconda di come `36.1` divide il movimento — è il **collaudo** della tassonomia, non il suo giudice |
| **36.5** | Applicazione: degrada, nega, riprova | Pipeline unica con esito `Full`/`Degraded`/`Rejected`, e danno e status risolti **separatamente**. **Resistance** generalizza la resistenza **viva** — quella di `Action.Guard` (`GuardResistedPushDistance`), non `PushResistance`, che [`D-075`](../decisions/RT_PDR_00_Decision_Log.md) ha reso **dormiente**; **Immunity** **nega**; `Action.Cleanse` passa dal tag esplicito alla **categoria**, conservando fail-closed e scelta del giocatore. La pipeline deve **assorbire** il rifiuto implicito già presente in `ApplyStatus`, non affiancarlo |
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

> 🧭 **Il pattern, su sei checkpoint su sei** *(spec panel del 2026-08-10, `#436`–`#441`)*.
>
> Ogni checkpoint di questa epic nominava un meccanismo che **il repository ha già**, con un nome o una forma
> diversa — e nessuno lo citava. Vale la pena scriverlo qui perché è la firma del difetto, non un incidente:
>
> | CP | Cosa esisteva già e non era nominato |
> |---|---|
> | `36.1` | `ERTActionSlot` · `ERTMovementStyle` |
> | `36.2` | `Refresh` con `max()` in `ApplyStatus`; **due sorgenti** per lo stesso status; il payload di `Marked` |
> | `36.3` | **`Action.Interrupt`**, che occupa il nome della primitiva; `Slow` già `MODIFY` conforme |
> | `36.4` | `Root` azzera anche le mobilità lineari — la severity non è nota a priori |
> | `36.5` | `Action.Cleanse` · `GuardResistedPushDistance` · il rifiuto implicito di `ApplyStatus` |
> | `36.6` | `ERTLogCategory` + un `Outcome` per categoria; `Catalog.Validator*`; il formato a `v7` |
>
> La lezione non è «cercare meglio»: è che **un framework si scrive contro ciò che c'è**, e ciò che c'è in
> questo repository è cablato dove serve — quindi non si trova cercando il nome del framework, ma il nome del
> **meccanismo**. `Cleanse`, `Immun`, `Resist`, `Interrupt`: quattro `grep` che valgono più di una lettura del
> sorgente. È lo stesso corollario che
> [`plans/handoff-status-control-triage-2026-08-10.md`](plans/handoff-status-control-triage-2026-08-10.md) §8
> aveva già scritto — *«quando un sorgente propone un nome, cercarlo in `Source/` prima di tutto il resto»* —
> e che `36.3` ha violato comunque.

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

### E38 — Economia del turno, accoppiamento col movimento e validazione del piano · P2

**Tracciata su GitHub**: epic [#609](https://github.com/DegrassiAaron/refactor-tactics-main/issues/609).

**Obiettivo**: decidere se l'economia del turno resta a **slot** o diventa una **capacità numerica**, e —
qualunque sia la risposta — rendere il piano **validabile in Planning** invece che scopribile in risoluzione.

**Perché non è v0.1**: tocca il modello che la v0.1 sta usando per arrivare in fondo. Il kit d'autore del
2026-08-12 lo dice da sé — *«do not drag future balance/resource complexity into Foundations»* — e la regola
in vigore ([D-028](../decisions/RT_PDR_00_Decision_Log.md)) non è un ripiego: è una decisione presa,
implementata e testata.

> ✅ **38.1 è chiusa il 2026-08-12, prima che l'epic cominciasse** ([D-114](../decisions/RT_PDR_00_Decision_Log.md),
> issue [#604](https://github.com/DegrassiAaron/refactor-tactics-main/issues/604)): **restano gli slot**, e il
> peso di un'azione si paga in **drawback** invece che in costo. L'ordine dei checkpoint si inverte di
> conseguenza — **38.3 diventa il primo lavoro** — e la ragione non è di comodo: il movimento come leva era
> l'esperimento che avrebbe reso misurabile la domanda appena chiusa, e resta l'unico asse di varietà del
> turno ancora aperto.

**Perché non è un'epic vuota**: la decisione è caduta, e **quattro checkpoint su cinque restano**.

> 🔴 **Rimisurato il 2026-08-25: questa riga diceva il falso.** Affermava che *«`git grep ValidatePlan` non
> restituisce nulla, e quel buco esiste con qualunque modello di economia»*. Oggi restituisce
> `URTPlanValidationLibrary` e le sue chiamate nei test: la **fetta pura di 38.2 è atterrata il 2026-08-12**,
> e nessuno è tornato a rimisurare la riga che la dichiarava assente.
>
> Il buco che resta non è la funzione, è il **consumatore**: in partita non la chiama nessuno e il bot non ci
> passa. *Quello* esiste con qualunque modello di economia — e resta la ragione per cui l'epic non è vuota.

| CP | Obiettivo | DoD misurabile |
|---|---|---|
| ~~**38.1**~~ | ~~La decisione: slot o capacità numerica~~ | ✅ **chiusa il 2026-08-12** da [D-114](../decisions/RT_PDR_00_Decision_Log.md). `RT-FEAT-ACTION-BUDGET` passa a `DEFERRED` con la motivazione, invece di sparire: il prossimo kit riproporrà l'`ActionCapacity`, e trovare la decisione scritta costa meno che ridiscuterla |
| **38.2** ✔ **atterrato 2026-09-02** | Validazione del piano in Planning | Esiste un punto solo che risponde `LEGALE` oppure `ILLEGALE + reason code` **prima** del commit, e il bot passa dallo stesso — `URTPlanValidationLibrary::ValidatePlan`, chiamata da `RTTurnManager.cpp:2634` in un ciclo su **tutte** le unità vive, bot inclusi. I reason code **estendono una famiglia esistente** con valori **in coda**: un enum parallelo rifarebbe l'errore respinto in [`spec-tassonomia-movimento.md`](../gameplay/spec-tassonomia-movimento.md) §6. La famiglia è **`ERTActionInvalidReason`** — `SlotOccupied`, `InsufficientMovementPoints`, `OnCooldown` — e ogni valore porta scritto accanto perché sta in coda: *«il motivo viaggia in `Amount`, un `int32` serializzato, e inserirne uno in mezzo rinumera tutti quelli che seguono»*. 🔴 *la riga nominava `ERTMoveOutcome`, `ERTCombatOutcome` e `ERTFallbackOutcome`: esistono tutte e tre, ma nessuna delle tre porta questi valori. `#605` aveva già corretto lo stesso errore nel proprio corpo il 2026-08-12; qui era rimasto.* ⚠️ **`InsufficientMovementPoints` resta in coda senza produttore**: il ramo che lo emetteva è uscito con [D-190](../decisions/RT_PDR_00_Decision_Log.md) perché la legalità qui è **strutturale**, e `Plan.VerdictIgnoresMoveBudget` lo misura. **Non dipende da 38.1** |
| **38.3** 🥇 | La compatibilità abilità↔movimento come dato — **primo lavoro dell'epic** | Un'abilità dichiara come si comporta sotto ciascun profilo (`NORMAL`/`IMPAIRED`/`ENHANCED`/`BLOCKED` o equivalente) e il validatore la legge. Criterio d'accettazione, nella forma di quello di E4: **aggiungere «Wraith spara in corsa» non deve toccare `ARTTurnManager`**. Se lo tocca, il modello non serve. ✅ **`AE-2` decisa sì** il 2026-08-12 ([D-116](../decisions/RT_PDR_00_Decision_Log.md)): categorie da negare allo `Sprint` = **precisione, preparazione, azioni pesanti**. 🔴 **Il checkpoint cresce**: D-116 porta con sé la **migrazione di fase dello `Sprint`** (da `FastMovement` a `NormalMovement`, superando `D-068`) e `Status.Exposed` a **2 turni** — e le tre voci **non sono separabili**, perché la migrazione da sola produce l'upgrade puro vietato da `D-015`. Costi misurati: `Actions.SprintIsAMoveProfileResolvedPreBlast` cade (è il gate), i **golden replay** vanno rebaseline, 4 file di test toccano `Exposed`. ⚠️ Il **bot** invece non è un costo: non nomina `Action.Sprint` — e non sceglierlo mai è un dato a sé — che dal 2026-08-12 ha un **modello davanti** invece di un'idea: [`../gameplay/spec-compatibilita-azioni-movimento.md`](../gameplay/spec-compatibilita-azioni-movimento.md) (`PROPOSED`). Una **soglia** invece della matrice del kit — 31 numeri + 4 contro 124 celle — e **tre stati invece di quattro**, perché `ENHANCED` dipende dai fatti del percorso (`AE-3`). ⚠️ Due costi che il kit non mostrava: i **profili di movimento non esistono come tipo** (`grep -rn MovementProfile Source/` → zero), quindi il primo lavoro non è assegnare soglie ma dargliene uno; e senza **38.2** il dato non è osservabile |
| **38.4** | La preview dice **perché** | L'Action Dock mostra capacità/slot residui, MP usati, cooldown e stato dell'abilità sotto il profilo scelto, e il Ghost Timeline resta **per fase** (`PREP · DASH · BLAST · MOVE`) invece di diventare una coda generica. Mai il solo colore. Il motivo di un rifiuto è leggibile **prima** di confermare |
| **38.5** | Scenari e determinismo | I cinque `Spec.ActionEconomy.*` dichiarati `planned` diventano eseguibili, più i test di permutazione. Nessun esito dipende da frame rate, ordine di `TMap` o rotazione visiva |

**Dipendenze**: `RT-FEAT-ACTION-ENGINE` (E4, chiusa), `RT-FEAT-ACTION-MOVE-PROFILES`, `RT-FEAT-ACTION-GENERIC`.
Cross-link, **non** dipendenze: E14 possiede l'`Overwatch`, E16 il facing, E11 la HUD — questa epic non ne
riapre nessuna.

**Rischi**: il rischio grosso è **caduto con 38.1** — una capacità numerica avrebbe cambiato validatore, HUD,
pesi del bot e ogni riga del catalogo insieme. Quello che resta è di 38.3: la compatibilità abilità↔movimento
introduce un asse di bilanciamento per abilità × quattro profili, e vale la mitigazione di
[D7 di E4](../gameplay/spec-motore-azioni-e4.md) — **una fetta per volta**, mai il modello e i numeri nello
stesso checkpoint.

> 🔴 **Un prerequisito scoperto implementando, il 2026-08-12.** Il tentativo di eseguire la migrazione dello
> `Sprint` prescritta da [D-116](../decisions/RT_PDR_00_Decision_Log.md) ha misurato che **i profili di
> movimento non esistono come entità nel codice** (`grep -rn MovementProfile Source/` → zero): il budget del
> movimento normale viene dall'**unità**, non dall'azione pianificata, e gli 8 MP dello `Sprint` li legge un
> solo punto (`ResolveDash`). Spostare la fase gli toglierebbe distanza e divieto di reazione — tre dei
> quattro prezzi che D-116 gli assegna.
>
> **L'ordine dell'epic cambia di conseguenza**: `#653` (dare un tipo ai profili) precede sia `#641` (la
> migrazione) sia `#606` (la compatibilità, che ha bisogno di un profilo su cui scrivere `Stability`). Il
> tentativo verificato vive su `feat/641-sprint-post-blast`: compila su entrambi i target, e i suoi **4 test
> rossi** sono la misura del buco.

**Non fa**: i valori (`AE-5` per lo `Sneak`, `AE-4` per la risorsa firma) · il costo del pivot (`FAC-12`, che
si guarda alla revisione dei numeri di ADR-0008) · i fatti del percorso (`AE-3`) · il workbook di
bilanciamento, che [`balance/README.md`](../balance/README.md) vieta di correggere cella per cella.

Referto d'origine:
[`plans/action-economy-consolidamento-2026-08-12.md`](../archive/roadmap-plans/action-economy-consolidamento-2026-08-12.md).
Owner della regola: [`../gameplay/spec-economia-del-turno.md`](../gameplay/spec-economia-del-turno.md).
Feature Registry: `RT-FEAT-ACTION-BUDGET` · `RT-FEAT-ACTION-MOVEMENT-COMPAT` ·
`RT-FEAT-ACTION-PLAN-VALIDATION`.

---

### E39 — Spatial Transfer — teleport, blink e movimento istantaneo · P3

**Tracciata su GitHub**: epic [#704](https://github.com/DegrassiAaron/refactor-tactics-main/issues/704).

**Obiettivo**: rendere esplicita una semantica che il repository possiede già, darle il **resolver
appropriato** e costruirci sopra consumatori diversi — senza duplicare Planning, Reaction, Perception,
TurnLog, grafo o facing.

> **Traversal percorre lo spazio. Transfer cambia posizione senza percorrerlo.**

**Perché non è v0.1**: [D-119](../decisions/RT_PDR_00_Decision_Log.md). La v0.1 ha quattro eroi e nessuno di
loro salta; il suo gate è che il turno simultaneo funzioni e si veda.

> ✅ **39.1 è chiusa il 2026-08-12, prima che l'epic cominciasse** — come 38.1 e per la stessa ragione: la
> decisione veniva prima del lavoro. [D-118](../decisions/RT_PDR_00_Decision_Log.md) chiude `MOV-1`
> (**famiglia propria**), [D-119](../decisions/RT_PDR_00_Decision_Log.md) chiude `MOV-2` (**v0.2**).
> **Non ha una issue**, e non per dimenticanza: il suo intero contenuto è documentale ed è atterrato in
> questo stesso commit — owner della tassonomia, Decision Log, `OPEN_DECISIONS`, roadmap, registry.

**Perché non è un'epic vuota, e la misura che lo dice**: la semantica **esiste già**.
`ERTMovementStyle::LinearLeap` produce `Result.Entered = { destinazione }`, il test
`RefactorTactics.Actions.Leap.IgnoresIntermediateCells` è verde, e lo scenario che prova la regola è già
scritto (`Spec.Movement.TeleportSkipsIntermediateCells`, `BLOCKED`). Il buco non è la semantica: è che
`ARTTurnManager` la ritrasforma in `Path = [Origin, Destination]` e la passa a `ResolveHexPaths`, un resolver
progettato per il **micro-step** — destinazione contesa, priorità, collisione frontale, occupazione cella per
cella, `Prog`, `MicroStepIndex`. Oggi c'è una semantica di trasferimento dentro un motore di attraversamento.

| CP | Obiettivo | DoD misurabile |
|---|---|---|
| ~~**39.1**~~ | ~~Il contratto: famiglia propria o policy del Dash, e in quale release~~ | ✅ **chiusa il 2026-08-12** da [D-118](../decisions/RT_PDR_00_Decision_Log.md) e [D-119](../decisions/RT_PDR_00_Decision_Log.md). Nessun runtime nuovo è stato scritto: la decisione precede il codice, che è l'intero punto del checkpoint |
| **39.2** 🥇 | Il resolver puro dei trasferimenti — **primo lavoro dell'epic** | Una **primitive pura**, non un Actor né un subsystem: richiesta → risultato, con `source` valida, `destination` valida/standable/libera, **nessuna** cella intermedia, nessun `MoveBudget`, nessun costo, nessun hazard, nessun crossed-boundary. Conflitto simultaneo: **stessa destinazione → falliscono tutti**, e l'ordine di iterazione **non** decide (invariante 3 di `AGENTS.md`). Il gate è la **permutation invariance**, non il numero di test |
| **39.3** | Short Blink: targeting, validazione e Planning | Il primo consumatore giocabile. Baseline: range 2, stesso layer, destinazione visibile e libera, nessun path. ⚠️ **Non è `LinearLeap` rinominato**: il salto richiede una delle **sei direzioni lineari**, un Blink deve poter scegliere una cella valida entro range anche non allineata — ed è qui che si paga la migrazione dichiarata da D-118. Riusa [#605](https://github.com/DegrassiAaron/refactor-tactics-main/issues/605): `ValidatePlan` resta l'**unica** autorità, i reason code si estendono **in coda** alle famiglie esistenti |
| **39.4** | TurnManager, TurnLog, facing e replay | Il trasferimento passa dal turno reale **senza essere trasformato in un falso traversal**. Il TurnLog ricostruisce origine, destinazione, famiglia, causa, `ActionId` ed esito — e **non** registra celle intermedie inesistenti (riusa [#307](https://github.com/DegrassiAaron/refactor-tactics-main/issues/307), chiusa). ⚠️ Il facing **non** si deriva dal path, perché non c'è un ultimo passo: è policy dell'azione sull'infrastruttura di [ADR-0005](../decisions/adr-0005-orientamento.md), non un secondo sistema. Il Replay **riproduce** la traccia, non ricalcola |
| **39.12** | Corpus scenari, determinismo e gate | `Spec.Movement.TeleportSkipsIntermediateCells` diventa **verde**: capability `Teleport` dichiarata nell'harness, intent del turno 2 riempito, assertion della cella finale aggiunta, **HP 90 invariati**. Più repeat determinism, permutation invariance, hash del TurnLog, replay, packaged. ⚠️ **La capability si dichiara qui e non prima**: l'harness non deve diventare il primo produttore di una feature che il gioco non sa fare |
| **39.5** | Arrival trigger, Overwatch e Reactive Blink | `CrossedBoundary` → **no** (non si attraversa niente); `ArrivedInside` → **sì**, se la reaction definition lo dichiara. Il Reactive Blink è una *response* del Fast Reaction esistente — riusa [#165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/165), **non** una seconda macchina |
| **39.6** | Rumore, percezione e privacy | `DepartureNoise` e `ArrivalNoise`; **mai** rumore lungo il percorso. In Planning la destinazione è **team-only**, sul filtro che esiste già — `URTIntentPrivacyLibrary::FilterForTeam` — e nel dominio di [#159](https://github.com/DegrassiAaron/refactor-tactics-main/issues/159). Mai in un Actor globalmente replicato. ⚠️ Il kit nomina anche `CanonicalIntentStore`, che **non è codice**: zero occorrenze in `Source/`, come il registry già dichiara su `RT-FEAT-NET-PRIVATE-PLANNING`. Chi implementa non lo cerchi |
| **39.7** | UI/UX, preview e bot | La preview **non disegna un path**: origine, destinazioni valide, selezione, AoE d'arrivo, reason code sulla cella invalida, intenti alleati e nessuno avversario. Il bot valuta la **destinazione**, non il percorso — che semanticamente non esiste |
| **39.8** | Swap atomico | Una sola operazione, non due trasferimenti consecutivi: nessun overlap intermedio osservabile, entrambe le destinazioni validate insieme, arrival trigger per entrambi. Scenario `Spec.Movement.SwapIsAtomic` |
| **39.9** | Recall / Return Point | ⚠️ **Il nome `Anchor` è occupato**: `Action.Anchor` e `Reaction.Anchor` significano resistenza allo spostamento ([D-094](../decisions/RT_PDR_00_Decision_Log.md)), e riusarlo erediterebbe una collisione. Validare: il punto esiste, non è scaduto, la destinazione è libera e ancora legale. Con la durata e il counterplay |
| **39.10** | Portal come transizione del grafo | **Portal ≠ Blink**: è un valore in coda a `ERTHexTransitionKind`, non un'abilità. Il pathfinding lo vede, `Revision` cambia, la cache di path si invalida, il validator lo accetta. Un `Unit->SetCell(Uscita)` bypasserebbe il grafo ed è l'errore che questo checkpoint esiste per impedire. Prima riga: **audit della serializzazione** dell'enum |
| **39.11** | Forced transfer | Il contrasto canonico con [#308](https://github.com/DegrassiAaron/refactor-tactics-main/issues/308), chiusa: uno spostamento forzato **attraversa** e prende gli hazard intermedi, un trasferimento forzato **no**. Coordinare con [#436](https://github.com/DegrassiAaron/refactor-tactics-main/issues/436) per `Root`, `Suppressed` e le immunità |
| **39.13** | Blind/Known teleport e spatial blocker | **P3, tagliabile.** Prima la baseline *destinazione visibile*; poi eventualmente `Visible`/`Known`/`Blind` e i blocker (`SpatialJammer`, `NoTransferZone`). Non si introducono prima che esista un consumatore reale |

**Ordine, e non è quello numerico**: `39.2 → 39.3 → 39.4 → 39.12` è il percorso che porta lo scenario da
`BLOCKED` a verde. Solo dopo si aprono in parallelo `39.5` (reazione), `39.6` (rumore) e `39.7` (UI/bot), e
poi i consumatori `39.8`–`39.11`.

**Dipendenze**: `RT-FEAT-ACTION-MOVE-PROFILES` · `RT-FEAT-ACTION-PLAN-VALIDATION` (#605) ·
`RT-FEAT-ACTION-ENGINE`. **Cross-link, non dipendenze**: E14 possiede la finestra di reazione, E13 il rumore,
E36 la bloccabilità. Questa epic non ne riapre nessuna.

**Rischi**: il rischio principale è **architetturale e ha un nome** — costruire un secondo motore. La
mitigazione è scritta come divieto in [D-118](../decisions/RT_PDR_00_Decision_Log.md): nessun subsystem di
trasferimento, nessun secondo validatore, nessuna seconda macchina di reazione, nessun secondo sistema di
percezione o di facing. Il secondo rischio è la **migrazione di `ERTMovementStyle`**, che è serializzato negli
asset: si progetta in `39.3` con compatibilità e validator, non si improvvisa.

**Non fa**: assegnare il Blink a un eroe del roster (è contenuto, e ricade sul kit del suo owner) ·
[#645](https://github.com/DegrassiAaron/refactor-tactics-main/issues/645), che è **prima** e altrove — un
ramo del motore irraggiungibile dal roster resta tale in qualunque release.

Referto d'origine:
[`plans/spatial-transfer-epic-2026-08-12.md`](../archive/roadmap-plans/spatial-transfer-epic-2026-08-12.md), che consolida il
secondo handoff della giornata; il primo è
[`plans/teleport-instant-movement-2026-08-12.md`](../archive/roadmap-plans/teleport-instant-movement-2026-08-12.md).
Owner della regola: [`../gameplay/spec-tassonomia-movimento.md`](../gameplay/spec-tassonomia-movimento.md).
Feature Registry: `RT-FEAT-ACTION-SPATIAL-TRANSFER`.

---

### E51 — Detriti strutturali, crolli e Rubble · P3

**Tracciata su GitHub**: epic [#1848](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1848).

**Obiettivo**: distruggere una struttura non significa farla **sparire** — significa produrre una nuova
trasformazione tattica della mappa, persistente e leggibile.

> **La struttura decide QUANTO materiale esiste. L'effetto decide COME viene distribuito.**

**Perché non è v0.1**: nessuno dei quindici gate riguarda la distruzione strutturale, e questa epic non apre
lavoro prima che siano verdi.

> ⚠️ **Questa epic ESTENDE l'Environmental System esistente. NON introduce una seconda simulazione.**
> `E8` [#22](https://github.com/DegrassiAaron/refactor-tactics-main/issues/22) è chiusa e possiede terreni,
> stati temporanei, propagazione e interazioni sistemiche; `ERTHexSurface` ha nove valori e fuoco/acqua/ghiaccio
> girano. I detriti sono **un elemento in più nella grammatica esistente**. Nessun `RubbleManager` accanto a
> `MapState`, nessun Actor per detrito, nessun pathfinder o sistema di percezione Rubble-specific.

**Perché non è un'epic vuota, e cosa la rende diversa da E39**: qui la semantica **non** esiste già. Misurato
su `d446a051`, `Source/RefactorTactics/` contiene **zero** occorrenze di `StructuralDebrisYield`,
`DebrisBudget`, `DebrisState`, `SpreadProfile` e `CollapseProfile`; le uniche «macerie» sono una metafora in
un commento su archi `Destroyed`. Ciò che esiste è tutto **attorno**: lo stato ambientale di cella
(`ERTHexSurface`), la classificazione graduata dell'occupancy (`ERTCellOccupancy`, cotta da dodici settori
con un sovrapprezzo che la consuma), la primitiva di spostamento forzato
(`ARTTurnManager::ApplyForcedDisplacement`, [#541](https://github.com/DegrassiAaron/refactor-tactics-main/issues/541)),
la revisione del grafo (`CurrentGraphRevision()`) e il canale causale del TurnLog
(`ERTLogCategory::Environment`).

🔴 **Il rischio architetturale ha un nome: costruire un secondo modello di blocco.** `ERTCellOccupancy` già
risponde a «quanto materiale c'è in questa cella e quanto costa attraversarla», con soglie d'autore che
entrano nell'hash di stato partita. La scala dei detriti si innesta **lì**, o il gioco avrà due risposte
diverse alla stessa domanda.

| CP | Obiettivo | DoD misurabile |
|---|---|---|
| **51.1** | Stati discreti, cella occupata e policy `BlockedDebris` — **la decisione precede il codice** | La tassonomia è decisa e i nomi C++ verificati contro il codice reale. È misurato **come** la scala si innesta su `ERTCellOccupancy` invece di affiancarlo. Il comportamento sotto un'unità viva è esplicito per **ogni** stato. La occupied-cell policy è canonizzata o respinta con motivazione, con tie-break stabile su `FRTCellId`. [#1849](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1849) |
| **51.2** 🥇 | Il primo slice: un producer strutturale reale e un consumer | **Non si gonfia.** Un evento che il gioco già produce genera detriti; almeno un'interazione li consuma o trasforma; TurnLog, ordine deterministico, scenario, mutation test, e `StateHash` invariato sugli scenari **senza** detriti. [#1132](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1132) |
| **51.3** | `DebrisBudget`, `CollapseProfile` e distribuzione deterministica | `StructuralDebrisYield` è proprietà della **struttura**, non derivata dal danno. Il budget è temporaneo e verificabilmente distinto dallo stato. La distribuzione produce una **lista ordinata** di mutazioni; accumulo, capacità e overflow hanno una regola. Permutation invariance. [#1850](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1850) |
| **51.4** | Detriti su celle e archi: traversal, topologia e `GraphRevision` | L'ownership cella vs arco è esplicita. ⚠️ La contraddizione su `ERTHexArcState::Destroyed` — dichiarato **TERMINALE**, mentre il consolidamento voleva che `Clear` riaprisse un arco — è **sciolta**, non aggirata; se l'enum si estende il valore va **in coda** con migrazione progettata. `GraphRevision` cambia e la cache di path si invalida. Nessun pathfinder Rubble-specific. [#1851](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1851) |
| **51.5** | `Clear Rubble`: rimozione esplicita e deterministica | Task/interazione, **non** una singola ability: chi può eseguirla è un dato. Progressione inversa all'accumulo, un gradino per step. L'interruzione **preserva** il progresso, asserito da un test. Nessuna risorsa nuova senza decisione esplicita. [#1852](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1852) |
| **51.6** | Determinismo, `StateHash`, TurnLog e scenari integrati | Scenario end-to-end `producer → distribuzione → occupancy → clear` verde. `StateHash` include i detriti; uno scenario **senza** detriti ha hash invariato. Repeat e permutation. Il TurnLog ricostruisce causa/prima/dopo. Mutation test mirato. [#1853](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1853) |

**Ordine, e non è quello numerico**: `51.1 → 51.2` porta il primo detrito nel gioco con una tassonomia già
decisa. Solo dopo si aprono `51.3` (quantità) e `51.4` (topologia), poi `51.5` (rimozione). `51.6` accompagna
le altre invece di chiuderle.

**Invarianti da preservare**: `Rubble` ≠ `Rough` · `Rubble` ≠ `blocked = true` · `DebrisBudget` ≠ `DebrisState`
· nessun degrado naturale · `MovementBlocking`/`Cover`/`Opacity`/`ProjectileBlocking` restano proprietà
distinte · nessun branch `HeroId` nel resolver · Chaos, mesh e VFX non sono autorità.
⛔ **`damage dealt` non è un proxy di `StructuralDebrisYield`.**

**Dipendenze**: `E8` [#22](https://github.com/DegrassiAaron/refactor-tactics-main/issues/22) (chiusa — si
estende, non si riapre) · [#541](https://github.com/DegrassiAaron/refactor-tactics-main/issues/541) per lo
spostamento forzato. **Cross-link, non dipendenze**: `E23`
[#324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324) possiede strutture, archi e
interaction graph ed è **related, non parent**; `E12` [#26](https://github.com/DegrassiAaron/refactor-tactics-main/issues/26)
il determinismo; [#1733](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1733) l'invariante di
occupancy, che questa epic **non duplica** e a cui non crea eccezioni.

**Rischi**: il secondo modello di blocco (mitigazione in `51.1`) · la migrazione di `ERTHexArcState`, che è
serializzato negli asset (si progetta in `51.4`, non si improvvisa) · l'inflazione di `51.2`, che è stretta
per costruzione e resta tale.

**Non fa**: `MaterialProfile` · `Dust` · `Wind` e `Flying Debris` · Chaos come autorità · reazioni materiali
avanzate · valutazione bot · hardening di rete · telemetria · VFX e asset finali. Restano differiti finché non
hanno owner e release canonici.

Referto d'origine:
[`plans/structural-debris-canonicalization-spec-panel-2026-08-30.md`](plans/structural-debris-canonicalization-spec-panel-2026-08-30.md),
che consolida il documento Drive *Structural Debris & Rubble — Canonical Consolidation*.
Mirror di dominio: `05 — Roadmap — Environment Systems & Gameplay Effects`.

⚠️ L'identificatore `RT-FEAT-MAP-STRUCTURAL`, che la tabella del Graybox Kit cita più sopra, è **provenance
storica**: il Feature Registry è uscito dal repository con `D-181`. Non va ricreato.

---

## v0.3 — «Informazione»

La release in cui l'informazione incompleta smette di essere un modificatore e diventa il centro della partita.

### E27 — Percezione completa: vista, udito, memoria · P1

**Tracciata su GitHub**: epic [#327](https://github.com/DegrassiAaron/refactor-tactics-main/issues/327).

Estende E13 oltre il thin slice: stealth, memoria dell'ultima posizione nota condivisa nel team, rumore
propagato con occlusione, gradi di certezza. Il sorgente 4v4 §19.1 fissa il vincolo: **nessun RNG nascosto per
la percezione base** — se un'unità è vista, lo è per una regola, non per un tiro di dado.

> **«Gradi di certezza» non significa un enum nuovo**, e il 2026-08-11 è stato a un passo dal diventarlo.
> [D-099](../decisions/RT_PDR_00_Decision_Log.md): il repository ha già tre modi di dire quanto una squadra
> sa — `ERTAwareness`, `ERTTargetKnowledge` e il turno del ricordo — e un quarto vocabolario avrebbe dato due
> risposte diverse alla domanda *sappiamo dov'è?*. La confidenza del bot **ordina dentro `Uncertain`**, non
> accanto. Con la regola che vale per tutta la epic: una belief non diventa conoscenza perché è lo scenario
> più plausibile. Feature Registry: `RT-FEAT-BOT-BELIEF`.

### E28 — Expert Bot v2 · P2

**Tracciata su GitHub**: epic [#328](https://github.com/DegrassiAaron/refactor-tactics-main/issues/328).

Solo **dopo** la stabilizzazione del resolver (§5.3 del sorgente): simulazioni counterfactual, più ipotesi
sul nemico, opponent model su eventi osservati, pianificazione robusta, personalità tattiche, possibile riuso
del planner come strumento di QA.

Feature Registry: `RT-FEAT-BOT-PREDICTIVE`. Il confine con la simulazione counterfactual è già tracciato da
[D-078](../decisions/RT_PDR_00_Decision_Log.md), che separa *chi riproduce* da *chi verifica*: il planner può
copiare lo stato logico solo quando il resolver è estraibile, ed è la stessa condizione.

### E29 — Predictive avanzato · P2

**Tracciata su GitHub**: epic [#329](https://github.com/DegrassiAaron/refactor-tactics-main/issues/329).

Ciò che E18 ha dichiarato fuori scope: trap persistenti, mine, tripwire su arco, catene di azioni predittive.
Si apre solo se il thin slice E18 ha retto al playtest.

### E33 — Conditional Intent · P2

**Tracciata su GitHub**: epic [#330](https://github.com/DegrassiAaron/refactor-tactics-main/issues/330).

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

**Tracciata su GitHub**: epic [#331](https://github.com/DegrassiAaron/refactor-tactics-main/issues/331).

Mappe grandi, partite da **45–60+ minuti**, più obiettivi, esplorazione, repositioning strategico, valore
reale di rumore e logistica (§4.3 del sorgente match-timing). Il sorgente è esplicito: «l'architettura deve
poterlo supportare, ma **non implementarlo ora**» — E19 e E23 sono ciò che rende questa epic possibile senza
riscritture.

### E31 — Obiettivi multipli e logistica · P3

**Tracciata su GitHub**: epic [#332](https://github.com/DegrassiAaron/refactor-tactics-main/issues/332).

### E32 — Formato 4v4 competitivo · P3

**Tracciata su GitHub**: epic [#333](https://github.com/DegrassiAaron/refactor-tactics-main/issues/333).

La v0.1 usa il 4v4 solo come **stress test** (E17). Qui diventerebbe un formato vero — ma solo se il playtest
del 3v3 dice che il sistema regge la densità.

> ✅ **Aggiornato il 2026-08-30**: questa riga diceva che *«il formato competitivo finale non è deciso»*, e
> non è più vero. [D-256](../decisions/RT_PDR_00_Decision_Log.md) — che sincronizza la decisione d'autore
> `AUTHOR-FMT-001` — fissa **3v3 come formato Standard definitivo**. ⚠️ **Il 4v4 non è promosso da questa
> decisione**: resta **Operations / stress e scala**, ed è precisamente ciò che questa epic dovrebbe
> valutare. La domanda che resta aperta qui non è *quale sia lo Standard*, ma se il 4v4 meriti di esistere
> come formato secondario.

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
Wraith, la cui forma `Siege` spegnerebbe la meccanica firma.

**Tracciata su GitHub**: epic [#244](https://github.com/DegrassiAaron/refactor-tactics-main/issues/244), con
11 checkpoint (`CP 34.1`–`34.11`) e 4 prototipi personaggio. Le candidature per l'intero roster stanno in
[`../characters/matrici-stati-personaggio.md`](../characters/matrici-stati-personaggio.md); l'ordine dei
prototipi va **dal più leggero al più invasivo** — `Phase · Flow` non tocca alcun sistema condiviso,
`Riktor · Bulwark` tocca cover, LOS, collisione e pathing.

### E37 — Radar di personaggio e generatore Wiki · P3

> ✅ **Completata il 2026-08-12** — le tre feature sono `DONE` nel registry.
>
> **Implementata:** Dai cataloghi markdown agli SVG committati, senza che un rating
> sia mai stato scritto a mano: parser, rubrica, gli otto assi delle due viste e il generatore
> deterministico con `--check` vivono in `tools/radar/` — **36 test**, zero dipendenze, nessun build
> step. I quattro Profile Radar sono in [`../characters/radar/`](../characters/radar/).
>
> La Wiki li mostra, i `wiki_refs` sono popolati e il Balance ha i suoi SVG: **otto artefatti**, Profile
> e Balance per ognuno dei quattro eroi.
>
> ⛔ **Non c'è più nessun gate**: `check-docs-links.py`, `check-docs-symbols.py` e `docs_inventory.py`
> giravano a mano perché questo repository non usa CI, e sono usciti con **D-182** il 2026-08-21
> (`feature_registry.py validate` era già uscito con **D-181**, lo stesso giorno). Restano
> `node tools/radar/generate.ts --check` e — dal 2026-08-25, **D-188** — `node tools/radar/doc-links.ts --check`,
> che ha ripreso la parte di `check-docs-links.py` sui percorsi citati dai documenti: il **link** che non
> risolve e l'**etichetta** che ne mostra uno vecchio. I **simboli** e l'**inventario** restano scoperti.
> `--check` è documentato
> in [`../balance/README.md`](../balance/README.md), dove i cataloghi si modificano. Il prezzo dichiarato
> è che protegge solo chi lo esegue.

[D-105](../decisions/RT_PDR_00_Decision_Log.md)…[D-108](../decisions/RT_PDR_00_Decision_Log.md), owner
[`../characters/spec-radar-profilo-personaggio.md`](../characters/spec-radar-profilo-personaggio.md).
Due viste radar — **Profile** (sei assi, pubblica) e **Balance** (cinque assi, tuning) — su scala `1..10`, più
un generatore SVG deterministico che le produce per la Wiki. I rating sono una **vista derivata**: nessuno
entra nel resolver.

**Non blocca nulla della v0.1** ed è deliberatamente P3: è comunicazione e supporto al bilanciamento, non
gameplay. Sta qui e non nella roadmap di release perché competerebbe con la consegna.

**I rating non si scrivono, si calcolano.** [D-106](../decisions/RT_PDR_00_Decision_Log.md): il generatore
legge [`RT_HeroCatalog_v0.1.md`](../balance/RT_HeroCatalog_v0.1.md) — più
[`RT_ActionCatalog_v0.1.md`](../balance/RT_ActionCatalog_v0.1.md) per le abilità che rinviano a un'azione
core ([D-115](../decisions/RT_PDR_00_Decision_Log.md)) — l'autorità dei numeri per
[D-023](../decisions/RT_PDR_00_Decision_Log.md) — applica la rubrica e produce i rating in memoria. Nessun
file di rating esiste, quindi nessuna seconda fonte può nascere né divergere.

> ⚠️ **La domanda su cui l'epic sembrava bloccata era mal posta.** «Quale dei due workbook è autorità sui
> rating» aveva già risposta nel repository: **nessuno dei due**. D-023 aveva declassato quello di balance a
> `RESEARCH`, e [`../balance/README.md`](../balance/README.md) vieta perfino di ripararlo cella per cella —
> *«un workbook rattoppato diventerebbe una falsa fonte corrente»*. Il conflitto non è stato risolto: si è
> **dissolto**.

**Il prerequisito di tutto è la rubrica**, non un dato. Se i rating non sono scritti, senza formula non
esistono affatto: non c'è il ripiego «intanto li mettiamo a mano». In cambio, cambiare `Salute` o `Movimento`
in un catalogo cambia i radar da solo.

**Sei assi, e uno nasce fragile.** [D-107](../decisions/RT_PDR_00_Decision_Log.md) sceglie di modellare i tre
assi senza fonte invece di ridurre il radar. Cinque si derivano dagli input che il catalogo dichiara per eroe;
`information` ha come unico ingrediente la **Vista** (Gadget 7, Wraith 6, Phase e Riktor 5), perché stealth e
detection vivono solo nel workbook escluso. Si arricchisce derivando il rumore dal **kit**
([D-042](../decisions/RT_PDR_00_Decision_Log.md)), non ripescando il foglio.

**Node/TypeScript, SVG committati con gate.** [D-108](../decisions/RT_PDR_00_Decision_Log.md), scelta
dall'autore contro la raccomandazione registrata nel consolidamento (Python accanto a `scripts/`).
⚠️ Le due scelte si combinano in un effetto che nessuna ha da sola: poiché i rating vengono dai cataloghi, il
gate degli SVG diventa **un test di regressione sui dati competitivi** — toccare una stat rende rosso il gate
finché i grafici non sono rigenerati nello stesso commit. Il prezzo è che chi tocca un catalogo deve poter
eseguire il generatore, quindi Node diventa un prerequisito del **bilanciamento**, non solo della
documentazione.

⚠️ **Finché la rubrica non esiste nessun radar è generabile**, perché un asse `TBD` non si renderizza come `0`
(D-105) e senza formula tutti gli assi sono `TBD`. È l'esito voluto: impedisce che quattro poligoni inventati
diventino canonici passando dalla Wiki.

**Tracciata su GitHub**: epic [#555](https://github.com/DegrassiAaron/refactor-tactics-main/issues/555), con
8 checkpoint (`CP 37.1`–`37.8`) collegati come sub-issue. Feature: `RT-FEAT-CHAR-RADAR-MODEL`,
`RT-FEAT-CHAR-RADAR-RATINGS-V01`, `RT-FEAT-WIKI-CHART-GENERATOR`.

---

## v0.5 — «Online Foundation»

### E40 — Il turno simultaneo in rete · P0

**Tracciata su GitHub**: epic [#773](https://github.com/DegrassiAaron/refactor-tactics-main/issues/773).

**Tema**: portare il gioco reale in rete **senza matchmaking prematuro**. Lobby privata, due squadre, una
partita che finisce.

⚠️ **Questa epic non introduce la rete: la rende una release.** Il lavoro è specificato da prima, in due
posti che restano owner e che questa sezione **cita** invece di ricopiare:

| Owner | Cosa possiede | Stato misurato |
|---|---|---|
| **M10** in [`roadmap-checkpoint.md`](roadmap-checkpoint.md#m10--rete-e-privacy) | i tre checkpoint d'esecuzione: `M10.1` listen server + autorità, `M10.2` piani team-only, `M10.3` canary anti-leak | ⏳ nessuno chiuso |
| `RT-FEAT-NET-AUTHORITY` | la feature, con `milestone: M10` e tre dipendenze dichiarate | `SPECIFIED` |
| `RT-FEAT-NET-PRIVATE-PLANNING` | il filtro per squadra, **già in v0.1** | `TESTABLE`, gate `network_privacy` **`todo`** |

**Il pezzo che il repository possiede già, ed è più di quanto sembri.** `FRTPlannedIntent → FilterForTeam →
FRTIntentView` esiste ed è testato da `RefactorTactics.Reactions.IntentNotVisibleToEnemy` e
`RefactorTactics.Combat.IntentVisibleToAlliesAlwaysEnemiesOnlyIfRevealed`. Ma il registry annota accanto la
sola frase che conta: *«oggi la privacy è **banale perché il gioco è offline**»*. Un filtro che nessuno prova
ad aggirare non è una difesa — è una convenzione. Il canary di `M10.3` è la prima riga di quel test che vale.

**Il vincolo che nessuno aveva quando M10 è stata scritta.**
[ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) introduce **N round-trip per turno** — una finestra
di reazione è un'attesa di risposta, non un tick — e `roadmap-checkpoint.md` lo registra già come *«nuovo
vincolo»* su M10. È il motivo per cui `preview ally-only` e `ready/commit` hanno budget di rete **diversi**:
la preview è continua e perdibile, il commit è raro e non può perdersi.

**Percorso critico** — l'ordine è dettato dalle dipendenze, non dal tema:

1. **Lifecycle di partita autoritativo** — chi crea, chi entra, chi è autorità, quando finisce.
2. **Store canonico degli intenti** — un solo posto dove l'intento vive, sul server.
3. **Relay della preview di squadra** — 8–12 Hz, unreliable, sequenced. È il budget KPI *Intent updates* già
   scritto in [`roadmap-checkpoint.md`](roadmap-checkpoint.md#kpi--performance-budget), oggi `⏳ con M10`.
4. **Protocollo ready/commit** — reliable e **idempotente**: un commit riprodotto due volte non è due commit.
5. **Risoluzione autoritativa** — il resolver gira sul server e da nessun'altra parte.
6. **Canary anti-leak** (`M10.3`) — fallisce se un client riceve **un solo byte** del piano avversario prima
   del reveal. Il KPI *Intent leak = 0* smette di essere vero per costruzione e comincia a essere verificato.
7. **Scenario packaged a due squadre** — la prova che il percorso regge fuori dall'editor.

**Fuori perimetro**, e non per fretta: matchmaking, ranked, dedicated server, riconnessione. Il dedicated è
**E42** e non è un dettaglio d'infrastruttura — cambia chi possiede l'autorità.

> ⚠️ **`scenario-decision-provider` non appartiene a questa epic.** Il kit sorgente lo propone qui come
> candidato `P1`; misurato contro `main`, il seam è già deciso da
> [D-101](../decisions/RT_PDR_00_Decision_Log.md) e tracciato da
> [`#542`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/542), **milestone v0.2**. Portarlo in
> v0.5 sposterebbe indietro un lavoro già assegnato, e ne creerebbe una seconda copia. Resta dov'è.

**Gate**: una partita completa fra due client · autorità di stato stabile · **zero intent leak** dimostrato
dal canary · replay divergence 0 sulla partita registrata.

---

## v0.6 — «Ability Runtime»

### E41 — GAS come runtime delle abilità, mai come autorità · P1

**Tracciata su GitHub**: epic [#774](https://github.com/DegrassiAaron/refactor-tactics-main/issues/774).

**Il confine è deciso da prima di questa epic.** [D-005](../decisions/RT_PDR_00_Decision_Log.md) — *«GAS non è
l'autorità del simulatore»* — è **Consolidata**, e la v0.1 la applica per assenza: `URTActionData`,
`URTHeroData` e `URTEquipmentData` sono data asset, non `GameplayAbility`. Questa epic non riapre quella
decisione: la mette alla prova nel solo modo che conta, cioè **introducendo GAS e verificando che il confine
regga**.

> ➕ **E dal 2026-08-30 GAS non è più un'opzione implementativa: è un impegno.**
> [D-260](../decisions/RT_PDR_00_Decision_Log.md), che sincronizza la decisione d'autore `AUTHOR-GAS-001`,
> dichiara GAS **obbligatorio post-v0.1** come layer di supporto per abilità, costi, cooldown, attributi ed
> effetti. ⚠️ **Cosa cambia per questa epic**: niente nel confine, tutto nella sua opzionalità. Il perimetro
> di [D-005](../decisions/RT_PDR_00_Decision_Log.md) resta **intatto** — validazione, costruzione dello
> snapshot e risoluzione restano C++ deterministico del progetto, e GAS non decide esiti né ordine — ma
> «introdurre GAS» smette di essere una via da valutare e diventa il bersaglio di `E41`.
> ⛔ **Nessun GAS nella v0.1**, e questa riga non lo autorizza.

```text
Resolver decide        →  cosa accade, quando, a chi, con che priorità
GAS applica/presenta   →  lifecycle, cue, durata, cleanup
```

Non:

```text
Montage/AbilityTask decide se il colpo va a segno
```

**Perché v0.6 e non prima.** Non per tema, per rischio: GAS in rete prima che la rete sia autoritativa
significa due sistemi non verificati che si accusano a vicenda al primo desync. La v0.5 chiude l'autorità;
questa epic ci appoggia sopra un runtime.

**Percorso critico**: confine architetturale scritto e verificato da un test negativo · `ASC` per unità ·
binding stabile `ActionId ↔ ability` — gli `ActionId` sono **serializzati nel TurnLog**, quindi il binding è
un contratto di compatibilità, non una comodità · applicazione degli eventi risolti · audit di privacy in
rete · **regressione di determinismo del replay**, che è il gate vero: se il TurnLog diverge dopo GAS, GAS ha
deciso qualcosa.

**Fuori perimetro**: usare `GameplayEffect` come sorgente di verità di costi e cooldown. Il bridge legge
l'economia esistente, non la sostituisce — `RT-FEAT-ACTION-BUDGET` è `DEFERRED` e questa epic non lo risuscita.

**Gate**: suite verde con GAS attivo · replay di una partita v0.5 riprodotto **senza divergenza** · nessun
percorso in cui un `AbilityTask` determini un esito competitivo.

---

## v0.7 — «Competitive Alpha»

### E42 — Dedicated server e loop online reale · P0

**Tracciata su GitHub**: epic [#775](https://github.com/DegrassiAaron/refactor-tactics-main/issues/775).

**Tema**: si gioca su un server che **non è il client di nessuno**.

`RT-FEAT-NET-DEDICATED` esiste nel registry come `IDEA` e senza documento owner — è la misura onesta di dove
siamo. La v0.5 costruisce l'autorità su un listen server, dove l'autorità coincide con un giocatore: è la
scelta giusta per validare il protocollo e quella sbagliata per una partita competitiva, perché chi ospita ha
latenza zero verso sé stesso.

**Percorso critico**: target di build dedicated · lifecycle di partita lato server · lobby privata custom ·
**riconnessione e resync** — che non è una comodità: senza, una disconnessione è una partita persa e il
formato non è competitivo · soak 3v3 su packaged dedicated · **matchmaking non-ranked**, che [D-236](../decisions/RT_PDR_00_Decision_Log.md) colloca qui perché qui stanno le sue dipendenze (`CP 42.6`).

**Gate**, ed è un percorso, non una lista:

```text
launch client → lobby → join/create → play → disconnect → reconnect → finish
```

su **dedicated server packaged**. Se un solo passo richiede l'Editor, il gate non è passato.

**Fuori perimetro**: **ranked e rating**, che sono **E44** — il perimetro è `RNK-1` in
[`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md), l'owner è [`#1604`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1604).
Assegnazione squadre e party sono qui perché la lobby esiste qui — e
[D-236](../decisions/RT_PDR_00_Decision_Log.md) estende lo stesso argomento al **matchmaking non-ranked**,
che in questa epic ha già lifecycle server-side, lobby e riconnessione. Il **rollout** in produzione resta
[`#810`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/810) in `v1.0`, e da qui in avanti il suo titolo dice il vero.

---

## v0.8 — «Beta / Balance»

### E43 — Misura a lotti, e il bot che sa cosa sta misurando · P1

**Tracciata su GitHub**: epic [#776](https://github.com/DegrassiAaron/refactor-tactics-main/issues/776).

**Il principio precede lo strumento**, ed è la parte che si dimentica per prima:

> Un dato bot-vs-bot **non è evidenza di bilanciamento** finché non sappiamo che il bot sa usare la capability
> misurata.

Un bot che non sa usare `Overwatch` produce un win-rate in cui `Overwatch` sembra debole. Il numero è vero, la
conclusione è falsa, e nulla nel numero lo segnala. Per questo ogni capability del bot deve poter dichiarare
uno stato **sostenuto da uno scenario riproducibile**:

```text
PASS · PARTIAL · FAIL · UNTESTED
```

`UNTESTED` è il valore che rende lo schema utile: senza, l'assenza di prova si legge come prova d'assenza.

**Non è un simulatore separato.** Il batch runner esercita il gioco reale attraverso il percorso canonico
`Intent → Planning → Snapshot → Resolver → TurnLog` — lo stesso dello Scenario Harness, che esiste già in
[`Source/RefactorTactics/ScenarioHarness/`](../../Source/RefactorTactics/ScenarioHarness/). Un banco di prova
con regole proprie misurerebbe sé stesso. `RT-FEAT-TOOL-BALANCE-GROUND` è **già in v0.1** (`IMPLEMENTING`,
P3): questa epic gli dà la scala e la qualificazione, non lo rifonda.

**Provenienza obbligatoria.** Ogni report di balance porta con sé: build · versione delle regole · hash del
contenuto · profilo del bot · **stato di competenza** delle capability misurate · policy dei seed · formato.
Un report senza queste righe non è confrontabile col precedente, e due numeri non confrontabili sono peggio di
un numero solo.

⚠️ **Win-rate e metriche di balance NON diventano gate.** Informano il design. Diventano gate: crash,
divergence del replay, leak, stato invalido. La differenza non è di severità ma di natura — un win-rate
sbilanciato è un'informazione su un gioco che funziona, un desync è un gioco che ha smesso di funzionare.

**Gate**: batch riproducibile a parità di seed · schema di competenza popolato per il roster · budget di
performance misurati su packaged · soak lungo senza crash.

---

## v0.9 — «Release Candidate»

### E44 — Feature freeze, e ciò che regge · P0

**Tracciata su GitHub**: epic [#777](https://github.com/DegrassiAaron/refactor-tactics-main/issues/777).

**Niente di nuovo.** Questa epic non aggiunge meccaniche: chiude il contenuto, indurisce ciò che esiste e
prepara il rilascio.

**Percorso critico**: freeze del contenuto · **hardening sicurezza/abuso** · **migrazione di versione di
save/replay** · soak da release candidate.

> ✅ **Il debito che questa sezione dichiarava è stato deciso il 2026-08-13, poche ore dopo essere stato
> scritto qui** ([D-137](../decisions/RT_PDR_00_Decision_Log.md)). Diceva che la migrazione di versione «è già
> una domanda **aperta** del repository» e che «se `FMT-1` non è deciso prima, questa epic la eredita come
> debito su un formato che gli utenti avranno già scritto». `FMT-1` **è** deciso: `URTHexMapAsset` passa a
> `FCustomVersionRegistry`, e il lavoro è di
> [`#687`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/687), non della v0.9.
>
> ⚠️ **Ciò che questa epic eredita è cambiato, non è sparito**: non più *una decisione da prendere sotto
> scadenza* ma *un meccanismo da avere già in piedi*. Se `#687` non è chiusa prima del freeze, la v0.9 si
> trova con la decisione presa e la rete assente — che è una posizione migliore ma non buona.
>
> La ragione per cui si è potuto decidere adesso vale la pena di restare scritta: **tutti e sette i passi di
> migrazione v1→v8 sono dichiarativi**, quindi oggi il cambio di meccanismo ha **rischio dati zero**. Dal
> primo passo trasformativo in poi non è più vero, e il costo cresce da solo.

**Ranked e rating** stanno qui e non in E42: un rating ha senso quando le regole non cambiano più.

> 🔴 **Questa frase ha promesso ranked e rating per quindici giorni senza un owner, e il rimando era
> circolare** (misurato il 2026-08-28). I quattro checkpoint di E44 sono freeze, hardening, migrazione e
> soak; `CP 45.3` ([#810](https://github.com/DegrassiAaron/refactor-tactics-main/issues/810)) metteva il
> rating fuori scope dicendo «*`44` ha già deciso il perimetro*», e `44` aveva solo questa riga. Ricerca nel
> tracker lo stesso giorno: **0** issue con `ranked`, **0** con `MMR`, **1** con `matchmaking` — quella.
>
> ✅ L'owner ora è **`CP 44.5`** ([#1604](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1604)),
> e il perimetro è `RNK-1` in [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md).
>
> 🔴 **E il 2026-08-30 il perimetro è stato deciso: è VUOTO per la v1.0.**
> [D-259](../decisions/RT_PDR_00_Decision_Log.md) — che sincronizza la decisione d'autore
> `AUTHOR-ONLINE-001` — chiude `RNK-1` togliendo **ranked, rating e MMR dalla promessa della v1.0**, invece
> di aggiungere account, persistenza e politica di forfeit durante il feature freeze. Restano **fuori**: coda
> classificata, rating persistente, politica di forfeit/disconnessione classificata, rank e rating mostrati
> al giocatore. Resta **dentro** il 3v3 competitivo come formato ([D-256](../decisions/RT_PDR_00_Decision_Log.md))
> e il matchmaking/gioco **non classificato**, coerente con [D-236](../decisions/RT_PDR_00_Decision_Log.md).
> ⚠️ **La contraddizione che questa sezione dichiarava — «Niente di nuovo» contro una meccanica nuova — si
> scioglie così**: non c'era modo di tenere entrambe, e a cedere è la promessa, non il freeze. La coda
> classificata è **post-v1.0**, e questa riga non la cancella: la sposta.
>
> ⚠️ **Resta una contraddizione interna a questa sezione, e va sciolta lì**: apre con «*Niente di nuovo.
> Questa epic non aggiunge meccaniche*» e poi rivendica ranked e rating, che sono meccaniche nuove. È
> plausibilmente il motivo per cui il checkpoint non è mai stato scritto.

⚠️ **Cuttable, dichiarato in anticipo**: progressione orizzontale e modding data-only. Non devono bloccare la
1.0 se il core competitivo è pronto — e dirlo ora costa meno che negoziarlo sotto scadenza.

**Gate**: nessuna feature nuova dopo il freeze · migrazione di formato provata su artefatti scritti dalla
build precedente · soak senza crash né divergence.

---

## v1.0 — «Launch»

### E45 — Un gate di produzione, non una release di feature · P0

**Tracciata su GitHub**: epic [#778](https://github.com/DegrassiAaron/refactor-tactics-main/issues/778).

**La v1.0 non aggiunge niente.** È la verifica che tutto il resto esiste **su infrastruttura di produzione**,
e la sua vista d'esecuzione è **M11** in [`roadmap-checkpoint.md`](roadmap-checkpoint.md#m11--production-readiness),
che possiede già budget, validator commandlet, soak packaged e replay audit.

**Percorso critico**: deployment dedicated di produzione · rollout del matchmaking · osservabilità · audit di
sicurezza e privacy · audit del replay · certificazione di performance · validazione del contenuto · matrice
di smoke su packaged · **piano di rollback**.

**Gate finale**, e si legge come una sola frase perché è una sola cosa:

> Una partita competitiva completa può essere **trovata, giocata, risolta, spiegata, registrata e riprodotta**
> su infrastruttura di produzione, senza replay divergence, senza intent leak e senza dipendenze dall'Editor.

⚠️ **«Senza dipendenze dall'Editor» è il gate che il repository rischia di mancare in silenzio**, ed è
misurabile oggi: il registro delle verifiche manuali
[`../technical/test-manuali-pie.md`](../technical/test-manuali-pie.md) esiste perché una parte della v0.1 è
provata **in PIE**. Ogni voce `PIE-*` che non abbia una controparte automatica su packaged è una riga di
questo gate ancora aperta.

---

## Il frontend oltre la v0.1

*(2026-08-16, [D-144](../decisions/RT_PDR_00_Decision_Log.md))* — **E46** porta in v0.1 il minimo che rende
verificabile `G13`: `Main Menu → Play → partita → Result → Quit`, più pausa, loading ed error modal. Tutto
il resto del frontend vive qui.

Sta in **una sezione trasversale e non spalmato nelle nove release** per la stessa ragione per cui E46 non
è finita in E11: il frontend è uno strato, non un tema di release. Distribuirne quattordici pezzi fra i
temi esistenti li renderebbe invisibili nel punto in cui servono — cioè tutti insieme, quando si decide
cosa costruire dopo il menu.

> ⚠️ **La prima colonna qui sotto non è in grassetto, e non è una svista di formattazione.** La tabella
> «Le release» in cima a questo file è **letta da un parser** (`release_table_rows()`), il cui regex è
> `^\|\s*\*\*(v\d+\.\d+)\*\*\s*\|` seguito da tre celle — e cattura la **quarta**, cercandovi le epic.
> Una seconda tabella di prosa con la stessa forma viene raccolta come se fosse quella owner: è il
> difetto che [D-138](../decisions/RT_PDR_00_Decision_Log.md) ha già pagato una volta, con tre release
> fantasma dichiarate due volte. Qui è ricomparso subito, in un modo nuovo: la riga `v0.7` dice *«segue
> E42, non E40»*, e `E40` — nominata solo per **escluderla** — finiva attribuita a v0.7, producendo una
> contraddizione con `RT-FEAT-NET-AUTHORITY`. Con `` `v0.5` `` invece di `**v0.5**` il regex non aggancia.
> **Chi "sistemasse" il grassetto per uniformità riaprirebbe il bug.**

| Release | Tema della release | Cosa ci arriva del frontend | Perché lì |
|---|---|---|---|
| `v0.2` | Struttura e finestre | **Settings** reale (Video · Audio · Controls · Gameplay · Accessibility) con persistenza · **Briefing** · **Training Lite** (Movement, Basic Combat) | In v0.1 `SETTINGS` è una voce *coming soon* e il Briefing non ha nulla da presentare: con un solo formato e una sola mappa non ci sono scelte. Diventano veri quando il roster raddoppia e le mappe sono più d'una |
| `v0.3` | Informazione | **Knowledge/fog inspector** · UI del rumore | Segue E27 (percezione completa): un'interfaccia che mostra *ciò che non sai* non può precedere il sistema che lo modella |
| `v0.4` | Operations | **Ispezione multilivello** della mappa · replay consapevole dei layer | Segue E30, dove le mappe grandi rendono il problema reale |
| `v0.5` | Online Foundation | **Lobby privata** · reconnect UX · e la **sostituzione della pausa** | ⚠️ Il documento sorgente collocava questo blocco in v0.7. È **v0.5**: il repository ha già E40 (`Il turno simultaneo in rete`) lì, con `Standard 3v3 online, lobby privata` come gate. È anche la release in cui la pausa offline di CP 46.6 cede a `Surrender`/`Leave Match` |
| `v0.7` | Competitive Alpha | **Spectator** · UI del dedicated | Segue E42: guardare una partita altrui richiede un server che non sia il client di nessuno |
| `v0.8` | Beta / Balance | **Match history** · statistiche · analisi competitiva del replay | Segue E43 (misura a lotti): una cronologia serve quando c'è qualcosa da confrontare |
| `v0.9` | Release Candidate | **Accessibility hardening** · localizzazione e layout · **controller** · hardening di error/loading · UX freeze | E44 è feature freeze: qui il frontend non cresce, regge |
| `v1.0` | Launch | **Onboarding** · certificazione UX | E45 è un gate, non feature |

### Le sezioni DEV/TEST non hanno una release: hanno una dipendenza

**Scenario Browser, Scenario Detail, Scenario Runner UI e Bot Visual Simulation** non sono assegnate a
nessuna release, ed è deliberato: vale la classificazione che il repository dà già al tooling — *«serve a
chi sviluppa, non è contenuto della release»*.

> 🔴 **La prima stesura di questo paragrafo, scritta poche ore prima, le faceva seguire
> [`#926`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/926)** — *«`Scenarios/` non è
> staged nel pacchetto, quindi una UI che legge il catalogo reale non avrebbe catalogo»*. Quella causa è
> stata **chiusa il 2026-08-16** da [`#935`](https://github.com/DegrassiAaron/refactor-tactics-main/pull/935):
> i 77 JSON entrano nel pak. **E #926 è stata chiusa per intero** il `2026-08-15T23:46Z`: la causa 2 —
> `-dpcvars` compilato fuori in Shipping — è caduta con #945, che ha dato allo scenario una porta d'ingresso
> che funziona anche lì (`Development PASS` e `Shipping PASS`, stesso `stateHash 572184bb`).
> ∴ **non c'è più nessuna dipendenza tecnica**: c'è una scelta di scope, e sta in piedi da sola.
> *(Questa nota è stata riscritta due volte in un giorno — «resta la causa 2», poi «non resta niente» —
> perché la issue si è chiusa a scaglioni mentre il consolidamento la citava. Il numero dei JSON era
> **76** in entrambe: rimisurato, `git ls-files Scenarios/ | grep -c '\.json$'` → **77**.)*

Assegnare loro una release significherebbe farle competere con la consegna, che è esattamente ciò che
`RT-FEAT-TOOL-CONTROL-CENTER` evita stando in `future`.

⚠️ **`RT-FEAT-UI-SCENARIO-BROWSER` esiste già e non è un widget**: è l'indice C++ di
[`#209`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/209). Il nome è occupato, la cosa
no — ed è la ragione per cui le feature di E46 usano il prefisso `RT-FEAT-UI-FRONTEND-*`.

**La UI di replay** non è qui perché ha già una issue:
[`#472`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/472).

---

## Cosa questo documento non decide

- **Numeri di bilanciamento**: nessun valore di danno, costo o durata è fissato qui. Stanno in
  [`../balance/`](../balance/README.md) quando esistono, altrimenti restano aperti.
- **Dimensione delle mappe in celle**: esplicitamente non bloccata dal sorgente.
- ~~**Il formato competitivo finale**: 3v3 è baseline da playtestare, non una decisione chiusa.~~
  ✅ **Deciso il 2026-08-30** da [D-256](../decisions/RT_PDR_00_Decision_Log.md): **3v3 è il formato Standard
  definitivo**. Restano da playtestare i suoi *parametri* — timer, durata, scala mappa — non la sua scelta.
- **Le date**: nessuna epic qui ha una scadenza. La v0.1 non ha ancora chiuso i suoi 15 gate.
