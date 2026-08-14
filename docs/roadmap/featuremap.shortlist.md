# Feature map — shortlist delle feature

> `GENERATA` · il blocco qui sotto lo riscrive `python scripts/feature_registry.py shortlist`.
> **Cosa è**: l'elenco corto di tutte le feature del registry, per area, con una riga ciascuna.
> **Cosa non è**: una fonte di stato. Lo stato vive **solo** in
> [`feature-registry.yaml`](feature-registry.yaml), è **derivato dai gate** e il validator lo verifica
> (`python scripts/feature_registry.py validate`). Qui nessuno stato è scritto a mano: la sola colonna
> umana è *Cosa fissa*, e il generatore la conserva. Una feature nuova compare con `—`, che è un buco
> visibile invece di una riga che sparisce.
> Modello, gate e comandi: [`feature-registry.md`](feature-registry.md).

## La scala degli stati — cosa vuol dire ogni gradino

Non è una percentuale, è **quali gate reggono**. Si legge dal basso: `IDEA` (esiste come nome) ·
`DESIGNED` (spec parziale) · `SPECIFIED` (spec completa, **nessun codice**) · `IMPLEMENTING` (runtime
presente o parziale) · `TESTABLE` (+ automazione) · `INTEGRATED` (+ gira in partita e uno **scenario** lo
dimostra) · `RELEASE_READY` (+ documentato in Wiki/UI) · `DONE` (+ packaged e privacy di rete).

---

<!-- RT_SHORTLIST_FEATURES:BEGIN -->

**110 feature** · v0.1 **74** · v0.2 **19** · v0.3 **5** · v0.4 **4** · v0.5 **1** · v0.6 **1** · v0.7 **1** · v0.8 **1** · future **4**.

| Stato | Quante |
|---|--:|
| `IDEA` | 7 |
| `DESIGNED` | 18 |
| `SPECIFIED` | 11 |
| `IMPLEMENTING` | 24 |
| `TESTABLE` | 7 |
| `INTEGRATED` | 22 |
| `RELEASE_READY` | 16 |
| `DONE` | 4 |
| `DEFERRED` (fuori scala) | 1 |

### Actions · 17

| Feature | Rel. | Stato | Gate | Vista | Cosa fissa |
|---|:--:|:--:|--:|:--:|---|
| `RT-FEAT-ACTION-BASIC-ATTACK-PROFILES` — Profili di attacco base per eroe | v0.1 | RELEASE_READY | 7/9 | E4 | Un eroe resta riconoscibile **anche senza usare le sue speciali**; e un attacco base debole non è un pulsante finto |
| `RT-FEAT-ACTION-DASH-DISPLACEMENT` — Dash e spostamento forzato | v0.1 | RELEASE_READY | 8/9 | E2 | Spinte opposte si annullano, la contesa resta ferma |
| `RT-FEAT-ACTION-ENGINE` — Motore delle azioni a priorità intera | v0.1 | RELEASE_READY | 8/9 | E4 | Ordine per priorità, permutazione-invarianza, nessun bias di Player ID |
| `RT-FEAT-ACTION-MOVE-PROFILES` — Profili di movimento (Move, Sprint, Charge) | v0.1 | RELEASE_READY | 8/9 | E4 | **Sprint è un profilo di Move, non un Dash** |
| `RT-FEAT-ACTION-PREDICTIVE` — Predictive Action, thin slice | v0.1 | INTEGRATED | 7/9 | E18 | Decisa in Planning, risolta a un boundary, **senza input live** |
| `RT-FEAT-ACTION-COOLDOWNS` — Cooldown ed economia delle risorse | v0.1 | TESTABLE | 6/9 | E4 | ⚠️ non verificabile finché i world di test non chiamano `BeginPlay()` (`#135`) |
| `RT-FEAT-ACTION-EQUIPMENT` — Equipaggiamento e loadout | v0.1 | IMPLEMENTING | 1/8 | E7 | Scelta orizzontale: ogni variante ha uno svantaggio |
| `RT-FEAT-ACTION-GENERIC` — Azioni generiche del catalogo | v0.1 | IMPLEMENTING | 3/9 | E4 | Le sette di D-025: `Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch` |
| `RT-FEAT-ACTION-SUPERS` — Ultimate e azioni ad alto impegno | v0.2 | IMPLEMENTING | 0/9 | — | Fuori dal contenuto della v0.1 |
| `RT-FEAT-ACTION-MOVEMENT-COMPAT` — Compatibilita' abilita' <-> movimento: normale, ridotta, potenziata, bloccata | v0.2 | SPECIFIED | 1/9 | E38 | — |
| `RT-FEAT-ACTION-SPATIAL-TRANSFER` — Spatial Transfer — cambiare posizione senza percorrere lo spazio | v0.2 | SPECIFIED | 1/10 | E39 | — |
| `RT-FEAT-ACTION-DELAYED` — Delayed Action ai boundary di fase | v0.3 | DESIGNED | 0/9 | — | Brief scritto, **nessuna epic aperta**: attende una decisione di scope |
| `RT-FEAT-ACTION-PLAN-VALIDATION` — Validazione del piano in Planning, con reason code deterministico | v0.2 | DESIGNED | 0/10 | E38 | — |
| `RT-FEAT-STATUS-FRAMEWORK` — Framework degli status — categorie, severity e primitive | v0.2 | DESIGNED | 0/9 | E36 | Undici status cablati uno per uno, nessun livello che li governi: **E36** in v0.2 |
| `RT-FEAT-ABILITY-RUNTIME` — Runtime delle abilita' — GAS come esecutore, non come autorita' | v0.6 | IDEA | 0/10 | E41 | — |
| `RT-FEAT-ACTION-BUDGET` — Capacita' d'azione del turno: slot o budget numerico | v0.2 | DEFERRED | 0/9 | E38 | — |
| `RT-FEAT-ACTION-TRAPS` — Trappole e gambit tattici | v0.3 | IDEA | 0/9 | — | Solo un nome |

### Characters · 7

| Feature | Rel. | Stato | Gate | Vista | Cosa fissa |
|---|:--:|:--:|--:|:--:|---|
| `RT-FEAT-CHAR-RADAR-MODEL` — Modello dei radar di profilo — Profile e Balance | v0.4 | DONE | 4/4 | E37 | — |
| `RT-FEAT-CHAR-RADAR-RATINGS-V01` — Rubrica — i rating derivati dal catalogo eroi | v0.4 | DONE | 4/4 | E37 | — |
| `RT-FEAT-CHAR-V01-ROSTER` — Roster v0.1 — Gadget, Phase, Riktor, Wraith | v0.1 | INTEGRATED | 6/8 | E6 | I 4 eroi corrispondono al catalogo; **3 reazioni su 5** cablate |
| `RT-FEAT-CHAR-PRESENTATION` — Presentazione dei personaggi (mesh, animazioni, anelli) | v0.1 | IMPLEMENTING | 1/7 | E21 · M8 | Il lavoro che viveva solo in M8, reso visibile dal registry |
| `RT-FEAT-CHAR-AUXILIARY-UNITS` — Unità ausiliarie (pet, evocazioni, gadget) | v0.2 | DESIGNED | 0/9 | — | — |
| `RT-FEAT-CHAR-TRANSFORMATION` — Stati di personaggio, stance e trasformazioni | v0.2 | DESIGNED | 0/9 | — | Le 10 voci `PIE-STATE-*` sono la sua controparte umana, e restano ⏳ |
| `RT-FEAT-CHAR-V02-ROSTER` — Roster v0.2 — Steel, Aurora, Murdock, Kwang | v0.2 | DESIGNED | 0/8 | E35 | Epic E35 |

### Core · 7

| Feature | Rel. | Stato | Gate | Vista | Cosa fissa |
|---|:--:|:--:|--:|:--:|---|
| `RT-FEAT-CORE-TURN` — Pipeline del turno simultaneo | v0.1 | RELEASE_READY | 8/9 | E2 | `Planning → Prep → Dash → Blast → Move → Cleanup`, tutti risolvono insieme |
| `RT-FEAT-CORE-TURNLOG` — TurnLog, reason code, hash e replay | v0.1 | RELEASE_READY | 7/8 | E12 | Ogni esito è spiegato da un reason code; l'hash è permutazione-invariante |
| `RT-FEAT-CORE-DETERMINISM` — Snapshot e resolver deterministico | v0.1 | INTEGRATED | 6/8 | E12 | Stessa snapshot + stesso seed ⇒ stesso risultato, 100 ripetizioni |
| `RT-FEAT-CORE-PLAYBACK` — Playback della risoluzione | v0.1 | INTEGRATED | 5/7 | E11 | La presentazione **riproduce**, non decide (invariante #1) |
| `RT-FEAT-REPLAY-ARCHIVE` — Replay Archive — recorder, Player e indice delle partite | v0.2 | INTEGRATED | 4/6 | E12 | — |
| `RT-FEAT-CORE-DECISION-BOUNDARY` — Risoluzione segmentata con Decision Boundary | v0.1 | SPECIFIED | 1/9 | E14 | Il turno diventa una sequenza di sotto-risoluzioni con un punto di decisione |
| `RT-FEAT-CORE-DECISION-TIME-BANK` — Decision Time Bank (budget di decisione per giocatore) | v0.1 | SPECIFIED | 1/10 | E14 | Quanto tempo di reazione ha un giocatore, e cosa costa un timeout |

### Data · 3

| Feature | Rel. | Stato | Gate | Vista | Cosa fissa |
|---|:--:|:--:|--:|:--:|---|
| `RT-FEAT-DATA-ASSET-PIPELINE` — Primary Data Asset e cataloghi | v0.1 | RELEASE_READY | 5/6 | E1 | Contenuti **feature-first** sotto `/Game/RT` |
| `RT-FEAT-DATA-HASH` — Hash di regole e contenuti | v0.1 | RELEASE_READY | 5/7 | E12 | Un replay non vale se le regole sotto sono cambiate |
| `RT-FEAT-DATA-STABLE-IDS` — ID stabili e versioni dei contenuti | v0.1 | RELEASE_READY | 5/6 | E1 | Un ID che cambia rompe replay, scenari e Wiki insieme |

### Environment · 9

| Feature | Rel. | Stato | Gate | Vista | Cosa fissa |
|---|:--:|:--:|--:|:--:|---|
| `RT-FEAT-ENV-ELECTRIC` — Propagazione elettrica sul grafo dell'acqua | v0.1 | INTEGRATED | 6/9 | E8 | Sul grafo dell'acqua, limite 3 passi, unicità per evento, ordine totale |
| `RT-FEAT-ENV-FIRE` — Fuoco e terreno dinamico | v0.1 | INTEGRATED | 6/9 | E8 | `Burning` danneggia nel Cleanup **prima** dei KO |
| `RT-FEAT-ENV-ICE` — Ghiaccio e scivolamento | v0.1 | INTEGRATED | 6/9 | E8 | Vigente e testato benché il catalogo lo dicesse «rimandabile» |
| `RT-FEAT-ENV-STATUS` — Stati temporanei legati alla cella | v0.1 | INTEGRATED | 6/9 | E8 | La durata è legata **alla cella**, non all'unità |
| `RT-FEAT-ENV-STEAM` — Fumo e copertura visiva | v0.1 | INTEGRATED | 6/9 | E8 | Il bersaglio si **vede** e non si può colpire |
| `RT-FEAT-ENV-SYSTEMIC-COMBOS` — Interazioni sistemiche producer/consumer | v0.1 | INTEGRATED | 7/9 | E8 | Il bonus viene dal **terreno**, non dall'identità di chi lo ha creato |
| `RT-FEAT-ENV-TERRAIN` — Otto terreni con costi e proprietà | v0.1 | INTEGRATED | 6/9 | E8 | Costi **interi**, proprietà dichiarate a catalogo |
| `RT-FEAT-ENV-WATER` — Acqua e stato Wet | v0.1 | INTEGRATED | 6/9 | E8 | `Wet` spegne le fiamme e apre la strada all'elettricità |
| `RT-FEAT-ENV-ICE-ENGINE` — Motore del ghiaccio (momentum, rottura, prone) | v0.2 | DESIGNED | 0/9 | — | Momentum, rottura, prone: brief scritto, fuori v0.1 |

### Factions · 2

| Feature | Rel. | Stato | Gate | Vista | Cosa fissa |
|---|:--:|:--:|--:|:--:|---|
| `RT-FEAT-FACTION-SCENARIOS` — Scenari di cooperazione per fazione | v0.2 | DESIGNED | 0/7 | — | Quattro scenari `Team.*` dichiarati `planned`: la sinergia è un **esempio**, non un kit di coppia (D-029) |
| `RT-FEAT-FACTION-SYSTEM` — Fazioni, identità e iconografia | v0.2 | DESIGNED | 0/7 | E35 | L'appartenenza dà identità visiva, **non** regole: nessun branch `if fazione` nel core |

### Gameplay · 2

| Feature | Rel. | Stato | Gate | Vista | Cosa fissa |
|---|:--:|:--:|--:|:--:|---|
| `RT-FEAT-CHARACTER-STATE` — Character State / Configuration System | v0.4 | SPECIFIED | 1/10 | E34 | Epic E34, `#244` |
| `RT-FEAT-INTENT-CONDITIONAL` — Conditional Intent — un intento con una biforcazione | v0.3 | SPECIFIED | 1/10 | E33 | Un intento con **una** biforcazione — epic E33, `#330` |

### Map · 16

| Feature | Rel. | Stato | Gate | Vista | Cosa fissa |
|---|:--:|:--:|--:|:--:|---|
| `RT-FEAT-MAP-HEXGRAPH` — FRTCellId e grafo esagonale multilivello | v0.1 | RELEASE_READY | 8/9 | E2 | Un solo substrato, coordinate **intere**, layer collegati da transizioni |
| `RT-FEAT-MAP-LOS` — LOS, targeting e traiettoria separati | v0.1 | RELEASE_READY | 6/7 | E2 | Vedere ≠ poter colpire ≠ come passa il colpo: tre domande, tre risposte |
| `RT-FEAT-MAP-PATHFINDING` — A* esagonale autorevole | v0.1 | RELEASE_READY | 6/7 | E2 | Il percorso lo calcola l'autorità, il client mostra un'anteprima |
| `RT-FEAT-MAP-COVER` — Copertura direzionale per bordo | v0.1 | INTEGRATED | 6/8 | E9 | La copertura è di un **bordo**: ripara da un lato solo |
| `RT-FEAT-MAP-DYNAMIC-COVER` — Copertura modificabile e pannello cinetico | v0.1 | INTEGRATED | 7/9 | E9 | Si erige e si sposta in partita, e **scade** nel Cleanup |
| `RT-FEAT-MAP-HIGH-GROUND` — Altura senza bonus numerico alla vista | v0.1 | INTEGRATED | 6/8 | E9 | **Nessun** bonus numerico in v0.1 (D-024): l'altura vale per la topologia |
| `RT-FEAT-MAP-INTERACTIVE-EDGES` — Porte e bordi commutabili | v0.1 | INTEGRATED | 7/9 | E9 | Una porta chiusa toglie passo **e** linea di tiro con lo stesso `BlocksTraversal` |
| `RT-FEAT-MAP-SPECIAL-TRANSITIONS` — Ponti, archi e transizioni multilivello | v0.1 | INTEGRATED | 7/9 | E9 | L'arco è **additivo**: romperlo annulla il percorso, non lo allunga |
| `RT-FEAT-MAP-FACING` — Facing come stato di gioco autorevole | v0.1 | IMPLEMENTING | 6/10 | E16 | Deriva da Move e Dash, entra in snapshot e hash, e da dietro annulla `Guard` |
| `RT-FEAT-MAP-INTERACTION-GRAPH` — Interaction graph: la relazione sorgente -> bersaglio e' un dato | v0.2 | DESIGNED | 0/10 | E23 | — |
| `RT-FEAT-MAP-STANDABILITY` — Standability derivata da anchor, footprint e clearance | v0.2 | DESIGNED | 0/8 | E23 | — |
| `RT-FEAT-MAP-STRUCTURE-IDENTITY` — Identita' stabile di una struttura attraverso cook, scenari e replay | v0.2 | DESIGNED | 0/8 | E23 | — |
| `RT-FEAT-MAP-TRANSITION-CLEARANCE` — Validità della transizione distinta da quella della cella | v0.2 | DESIGNED | 0/8 | E23 | — |
| `RT-FEAT-MAP-STRUCTURAL` — Strutture: integrita', crolli a catena e macerie | future | IDEA | 0/9 | — | Lo slot si chiama **`Bulkhead`** (D-082): il verbo *breach* resta un'azione, e `BreachCharge` è ciò che lo apre |
| `RT-FEAT-MAP-VERTICALITY` — Verticalita': vault, drop, salita e cadute | future | IDEA | 0/9 | — | — |
| `RT-FEAT-MAP-WATER-DYNAMICS` — Acqua tattica: profondita', flooding e corrente | future | IDEA | 0/9 | — | La profondità è una **superficie**, non un asse (D-081): il flooding esiste già come cambio di superficie |

### Networking · 3

| Feature | Rel. | Stato | Gate | Vista | Cosa fissa |
|---|:--:|:--:|--:|:--:|---|
| `RT-FEAT-NET-PRIVATE-PLANNING` — Intenti privati per squadra | v0.1 | TESTABLE | 5/8 | E5 | Nessun byte del piano avversario prima del reveal (invariante #6) |
| `RT-FEAT-NET-AUTHORITY` — Multiplayer con autorità server | v0.5 | SPECIFIED | 1/8 | E40 · M10 | Nessuna epic né issue: vive in M10 |
| `RT-FEAT-NET-DEDICATED` — Dedicated server | v0.7 | IDEA | 0/8 | E42 | — |

### Objectives · 5

| Feature | Rel. | Stato | Gate | Vista | Cosa fissa |
|---|:--:|:--:|--:|:--:|---|
| `RT-FEAT-MATCH-END-CONDITIONS` — Fine partita a tre vie | v0.1 | RELEASE_READY | 7/9 | E10 | Eliminazione · obiettivo · `RoundLimit`, con pareggio **dichiarato** |
| `RT-FEAT-MATCH-FORMAT` — Formato di partita e classe di mappa | v0.1 | TESTABLE | 5/8 | E19 | `URTMatchFormatData` esiste: mancano classe di mappa e unità per squadra |
| `RT-FEAT-MATCH-PACING` — Pacing del turno e del match | v0.1 | TESTABLE | 5/8 | E12 | Percentili misurati sul giocatore, non stimati |
| `RT-FEAT-OBJECTIVE-SYSTEM` — Obiettivi dinamici in mappa | v0.1 | IMPLEMENTING | 2/9 | E10 | La partita **finisce** per obiettivo, ma non c'è **nulla da attivare** |
| `RT-FEAT-STRESS-4V4` — Validazione di stress 4v4 | v0.1 | IMPLEMENTING | 1/7 | E17 | Misura dove si rompe il sistema con 8 unità. **Non** decide il formato |

### Perception · 4

| Feature | Rel. | Stato | Gate | Vista | Cosa fissa |
|---|:--:|:--:|--:|:--:|---|
| `RT-FEAT-PERCEPTION-MEMORY` — Memoria del contatto e ultima posizione nota | v0.1 | TESTABLE | 4/9 | E13 | Il contatto perso lascia una traccia, non un vuoto |
| `RT-FEAT-PERCEPTION-TEAM-KNOWLEDGE` — TeamKnowledge e informazione parziale | v0.1 | TESTABLE | 5/9 | E13 | Cosa sa la squadra, non cosa sa il motore |
| `RT-FEAT-PERCEPTION-VISION` — Vista, facing e livelli di consapevolezza | v0.1 | IMPLEMENTING | 3/9 | E13 | Oggi la vista è **una statistica che non decide nulla** |
| `RT-FEAT-PERCEPTION-NOISE` — Rumore e percezione acustica | v0.1 | SPECIFIED | 3/9 | E13 | Il secondo canale: sentito senza essere visto |

### Production · 2

| Feature | Rel. | Stato | Gate | Vista | Cosa fissa |
|---|:--:|:--:|--:|:--:|---|
| `RT-FEAT-PROD-PACKAGED` — Verifica su build packaged | v0.1 | IMPLEMENTING | 2/6 | E12 | **P0**: senza, non è una release |
| `RT-FEAT-PROD-PERFORMANCE` — Budget di performance misurati | v0.1 | IMPLEMENTING | 3/6 | E12 | Path 0,025 ms e resolver 0,41 ms/turno misurati; FPS e preview richiedono rendering |

### Reactions · 7

| Feature | Rel. | Stato | Gate | Vista | Cosa fissa |
|---|:--:|:--:|--:|:--:|---|
| `RT-FEAT-REACTION-PREPARED` — Reazioni preparate in planning | v0.1 | INTEGRATED | 7/9 | E5 | Una attivazione per turno, **nessuna attesa nel resolver** |
| `RT-FEAT-REACTION-OPPORTUNITY` — Modello Opportunity → Commit | v0.1 | IMPLEMENTING | 3/10 | E14 | Il modello unico di tutte le finestre; mai annidate |
| `RT-FEAT-REACTION-OVERWATCH` — Overwatch universale profilabile | v0.1 | IMPLEMENTING | 2/10 | E14 | Profilabile, non un'abilità d'eroe |
| `RT-FEAT-REACTION-PROFILE` — Reaction Profile armato da Brace | v0.1 | IMPLEMENTING | 1/9 | E14 | `Brace` non è solo riduzione danno: arma un profilo di risposta |
| `RT-FEAT-REACTION-CLASH` — Reaction Clash (opportunity contested) | v0.1 | SPECIFIED | 1/10 | E14 | Due opportunity contese: chi vince, e una sola volta |
| `RT-FEAT-REACTION-FAST` — Fast Reaction con finestra limitata | v0.1 | SPECIFIED | 1/10 | E14 | Finestra **3,0 s**, `Timeout → HOLD`; Fast/Headless risponde subito via policy |
| `RT-FEAT-REACTION-FAST-ACTION` — Fast Action come continuazione della propria azione | v0.1 | DESIGNED | 0/10 | E14 | Continuazione della **propria** azione, non risposta all'avversario |

### Tools · 15

| Feature | Rel. | Stato | Gate | Vista | Cosa fissa |
|---|:--:|:--:|--:|:--:|---|
| `RT-FEAT-TOOL-VALIDATION` — Validator di dati, mappe e documenti | v0.1 | DONE | 5/5 | E1 | L'unica feature `DONE` del registry |
| `RT-FEAT-WIKI-CHART-GENERATOR` — Generatore SVG deterministico dei radar | v0.4 | DONE | 4/4 | E37 | — |
| `RT-FEAT-BOT-BASE` — Bot a utility scoring deterministico | v0.1 | RELEASE_READY | 7/8 | E2 | Deterministico; le candidate nascono da `ReachableCells`, mai mosse illegali |
| `RT-FEAT-TEST-SCENARIO-HARNESS` — Scenario Test Harness automatizzato | v0.1 | INTEGRATED | 6/8 | E15 | `PASS/FAIL/ERROR/BLOCKED`, corpus scoperto dall'indice, niente bypass |
| `RT-FEAT-TOOL-MAP-EDITOR` — Editor mode della mappa esagonale | v0.1 | INTEGRATED | 5/6 | M9 | **Fuori** dalla vista di release: non entra nella build di gioco |
| `RT-FEAT-BOT-FAIRNESS` — Il bot pianifica sulla conoscenza della sua squadra | v0.1 | TESTABLE | 3/7 | E13 | — |
| `RT-FEAT-TEST-GOLDEN` — Golden replay e showcase «Il Relè» | v0.1 | IMPLEMENTING | 3/8 | E15 | Oggi `BLOCKED` su 5 capability |
| `RT-FEAT-TOOL-BALANCE-GROUND` — Banco di prova del bilanciamento | v0.1 | IMPLEMENTING | 3/6 | E1 | Dove si misurano i numeri prima di scriverli a catalogo |
| `RT-FEAT-TOOL-CONTROL-CENTER` — Project Control Center — la UI web del registry | future | IMPLEMENTING | 2/6 | — | — |
| `RT-FEAT-TOOL-DEBUG-CONSOLE` — Comandi console rt.Debug e rt.Test | v0.1 | IMPLEMENTING | 3/6 | E11 | Ne esistono 2 su 8: senza, si debugga a occhio |
| `RT-FEAT-TOOL-MAP-GEOMETRY` — Geometria architettonica quantizzata e cottura in dati di cella | v0.1 | IMPLEMENTING | 4/7 | M9 | — |
| `RT-FEAT-BOT-BELIEF` — Belief — celle plausibili, decadimento e minaccia proiettata | v0.3 | DESIGNED | 0/8 | E27 | — |
| `RT-FEAT-BOT-PREDICTIVE` — Scenari nemici plausibili e piano robusto | v0.3 | DESIGNED | 0/8 | E28 | — |
| `RT-FEAT-BOT-TACTICAL` — Team Planner — piano di squadra, ruoli dinamici e sinergie | v0.2 | DESIGNED | 0/8 | E26 | Epic E26 |
| `RT-FEAT-BOT-COMPETENCE` — Schema di competenza del bot — cosa il bot sa davvero usare | v0.8 | IDEA | 0/6 | E43 | — |

### UI · 11

| Feature | Rel. | Stato | Gate | Vista | Cosa fissa |
|---|:--:|:--:|--:|:--:|---|
| `RT-FEAT-UI-COMBAT-LOG` — Combat log e spiegabilità | v0.1 | RELEASE_READY | 6/7 | E11 | Ciò che il giocatore legge e ciò che il replay registra sono **la stessa cosa** |
| `RT-FEAT-UI-PLANNING` — HUD di planning, selezione e preview | v0.1 | RELEASE_READY | 6/7 | E11 | Il client valida sullo stato dell'autorità |
| `RT-FEAT-UI-SCENARIO-BROWSER` — Selettore e indice degli scenari | v0.1 | INTEGRATED | 6/8 | fuori scope | Tooling di test, **fuori** dal contenuto di release per decisione |
| `RT-FEAT-UI-CERTAINTY` — Livelli di certezza degli intenti alleati | v0.1 | IMPLEMENTING | 3/8 | E11 | Confermato / previsto / incerto — tre livelli, non una sfumatura |
| `RT-FEAT-UI-ICON-LANGUAGE` — HUD Icon Language | v0.1 | IMPLEMENTING | 1/7 | E20 | Un **catalogo semantico**, non texture referenziate nei widget |
| `RT-FEAT-UI-POINTER-INTERACTION` — Contratto del puntatore — Hover, LMB, RMB | v0.1 | IMPLEMENTING | 1/8 | E11 | — |
| `RT-FEAT-UI-SCREEN-HUD` — Screen HUD in UMG (layer §4.1) | v0.1 | IMPLEMENTING | 1/7 | E11 | — |
| `RT-FEAT-UI-TACTICAL-CAMERA` — Camera tattica | v0.1 | IMPLEMENTING | 2/6 | E11 | Tarata sulla scala esagonale |
| `RT-FEAT-UI-WARNINGS` — Avvisi di collisione, fuoco amico e risorse | v0.1 | IMPLEMENTING | 3/7 | E11 | L'arancione del fuoco amico deve **notarsi** |
| `RT-FEAT-UI-ACTION-GHOSTS` — Action Ghosts e Ghost Timeline | v0.1 | SPECIFIED | 1/8 | E11 | Il planning visuale: cosa succederà, e con quanta certezza |
| `RT-FEAT-UI-STRUCTURE-READABILITY` — Leggibilita' delle strutture: etichette tattiche e relazioni sorgente/bersaglio | v0.2 | DESIGNED | 0/7 | E23 | — |

<!-- RT_SHORTLIST_FEATURES:END -->

---

## Da sapere prima di usare questa lista

- **Le feature senza issue restano tali.** Collegarle è lavoro del registry, non delle viste che lo leggono.
  Il numero **non si scrive qui**, perché una riga di prosa a mano invecchia mentre la tabella sopra no: al
  2026-08-12 sono **19 su 103**, e si rimisurano leggendo `issues: []` in `feature-registry.json`.
  Quelle che meritano attenzione sono le **15** senza issue *né* epic *né* milestone — nessun tracciamento
  di alcun tipo. ⚠️ `RT-FEAT-TOOL-MAP-EDITOR` **non è più fra queste**: porta otto issue (`#567`, `#622`,
  `#623`, `#551`, `#552`, `#553`, `#554`, `#588`) e la milestone `M9`. `RT-FEAT-NET-AUTHORITY` ha `M10` ma
  nessuna issue.
- **`INTEGRATED` non vuol dire finito**: vuol dire che gira e uno scenario lo dimostra. Mancano ancora Wiki/UI.
- **Uno stato alto con `last_verified` vecchio è un errore del validator**, non una sfumatura: è il gate `G15`.
- **Non si aggiorna a mano.** Dopo aver toccato il registry: `python scripts/feature_registry.py shortlist`.
  In un gate automatico: `shortlist --check`, che non scrive e esce 1 se questa vista è indietro.
