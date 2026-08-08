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
| [`design/2026-08-08-roster-8-conflux-constrine.md`](../src/design/2026-08-08-roster-8-conflux-constrine.md) | E21 |
| [`design/2026-08-08-cover-window-open-fire-seal.md`](../src/design/2026-08-08-cover-window-open-fire-seal.md) | E22 |
| [`design/2026-08-08-muri-porte-e-interazioni.md`](../src/design/2026-08-08-muri-porte-e-interazioni.md) | E23 |
| [`design/match-timing-e-scala-mappe.md`](../src/design/match-timing-e-scala-mappe.md) | E19 (v0.1), E24, E30 |
| [`design/2026-08-08-hud-faction-icons.md`](../src/design/2026-08-08-hud-faction-icons.md) | E20 (v0.1), E25 |
| [`handoff/2026-08-08-bot-ai-roadmap-e-test-pie.md`](../src/handoff/2026-08-08-bot-ai-roadmap-e-test-pie.md) | E26, E28 |
| [`handoff/2026-08-07-nuove-decisioni-e-scenario-4v4.md`](../src/handoff/2026-08-07-nuove-decisioni-e-scenario-4v4.md) | E24, E27 |

## Le release

| Release | Nome | Tema | Epic | Formato di gioco |
|---|---|---|---|---|
| **v0.1** | Vertical slice | Il turno simultaneo funziona e si vede | E1–E20 | Skirmish 2v2 vs bot |
| **v0.2** | Struttura e finestre | Il campo diventa manipolabile; roster 8 | E21–E26 | Standard 3v3 |
| **v0.3** | Informazione | Quello che non sai vale quanto quello che fai | E27–E29 | Standard 3v3 |
| **v0.4** | Operations | Partite lunghe su mappe grandi | E30–E32 | Operations 4v4+ |

Oltre la v0.4 resta **north-star non pianificato**: multiplayer in rete (milestone M10 del piano canonico),
progressione, modding, editor mappe a runtime. Non si aprono epic per ciò che non ha una release.

> **Numerazione continua.** Le epic proseguono da E18 (ultima della v0.1) senza azzerarsi per release: un
> riferimento a «E23» resta univoco per sempre. Le due epic **E19** ed **E20** appartengono alla **v0.1** pur
> nascendo da sorgenti di questa roadmap — vedi [Cosa la v0.1 deve già rispettare](#cosa-la-v01-deve-già-rispettare).

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

### E21 — Roster 8: Sentinel Directorate e Resonance · P0

**Obiettivo**: portare il roster da 4 a 8 eroi aggiungendo le due fazioni v0.2, senza introdurre bonus di
fazione né kit di coppia — [ADR-0006](../decisions/adr-0006-ownership-abilita-sinergie.md) vale invariato.

Le schede esistono già come `DATA_SPEC`/`DESIGN_SPEC`: [`../characters/v0.2/`](../characters/v0.2/) e
[`../wiki/fazioni/`](../wiki/fazioni/index.md). Questa epic le porta a runtime.

| CP | Obiettivo | DoD misurabile |
|---|---|---|
| **21.1** | Sentinel Directorate — Steel, Murdock | I due `URTHeroData` esistono con azioni a catalogo; affinità *Protection → Fire Sector* emerge da meccaniche generiche, nessun `FactionSetBonus` |
| **21.2** | Resonance — Aurora, Kwang | Idem per *Terrain Shaping → Anchor Geometry* |
| **21.3** | Bilanciamento a 8 | `Heroes.RosterIsBalanced` esteso a 8: nessun eroe domina, le coppie di affinità restano simmetriche |
| **21.4** | Wiki e cataloghi allineati | `../wiki/fazioni/` e `../balance/RT_HeroCatalog_v0.1.md` descrivono 8 eroi con i valori realmente a runtime |

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

**Dipendenze**: E9. **Rischi**: gli ID stabili si decidono una volta — cambiarli dopo il primo cook invalida
scenari, golden replay e mappe salvate.

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

**Obiettivo**: estendere il catalogo di E19/E20 alle dodici categorie piene, con world-space HUD e pagine wiki.

**Dipendenze**: E20. **Fuori scope**: rifacimento dell'HUD della v0.1.

### E26 — Tactical Bot v1 · P1

**Obiettivo**: il bot passa da «gioca legalmente» a «gioca di squadra».

Aggiunge (§5.2 del sorgente bot): TeamKnowledge integrato, contatti last-known e acustici, threat map,
opportunity map, information value, coordinazione vera, sinergie ambientali, belief weights, predictive
action scoring, reaction policy migliore, stress 4v4.

**Dipendenze**: E13 (conoscenza parziale), E26 richiede il bot v0.1 della v0.1 chiuso.
**Rischi**: la belief map è il punto in cui un bot smette di essere deterministico per distrazione — il
determinismo a parità di seed resta un gate, non un'aspirazione.

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

---

## Cosa questo documento non decide

- **Numeri di bilanciamento**: nessun valore di danno, costo o durata è fissato qui. Stanno in
  [`../balance/`](../balance/README.md) quando esistono, altrimenti restano aperti.
- **Dimensione delle mappe in celle**: esplicitamente non bloccata dal sorgente.
- **Il formato competitivo finale**: 3v3 è baseline da playtestare, non una decisione chiusa.
- **Le date**: nessuna epic qui ha una scadenza. La v0.1 non ha ancora chiuso i suoi 15 gate.
