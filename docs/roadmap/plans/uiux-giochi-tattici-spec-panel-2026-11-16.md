# Spec panel — «Architettura UI/UX nei Giochi Tattici a Turni» del 2026-11-16 contro il contratto

> `SNAPSHOT` · **Scritto**: 2026-11-16 · **Input**: [`../../research/handoff/uiux-giochi-tattici-input-2026-11-16.md`](../../research/handoff/uiux-giochi-tattici-input-2026-11-16.md)
> (testo incollato nella sessione, senza a capo) · **Richiesta dell'autore**, alla lettera: *«abbiamo creato un documento, ci sono alcune
> inesattezze: le fasi (sono corrette 5, prep,dash,blast,move,clean), gli elementi (sono ancora da definire, inizieremo con queli base e
> tier 2 combinata), risk meter (non so se tenerlo, non l'ho capito). Ecco il doc, analizzalo e integra nella nostra documentazione. se hai
> dubbi (info contrastanti, utilita' o overengineering) fermati,spiegati e chiedi»* · **Esito**: [D-620](../../decisions/RT_PDR_00_Decision_Log.md),
> [#4138](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4138)
> **Metodo**: lo stesso dei referti sulla «Master Spec» ([`master-spec-csharp-spec-panel-2026-11-07.md`](master-spec-csharp-spec-panel-2026-11-07.md))
> e sul GDD ([`master-gdd-spec-panel-2026-11-08.md`](master-gdd-spec-panel-2026-11-08.md)): ogni premessa è misurata contro il contratto
> (Decision Log → ADR → spec owner → cataloghi → codice) e classificata; nessun conflitto è stato riconciliato per plausibilità
> ([`AGENTS.md`](../../../AGENTS.md) §2). Le divergenze sostanziali da una decisione registrata sono state messe in domanda all'autore, in quattro giri; due righe restano
> tradotte o coperte da una sua decisione precedente senza una domanda nuova (lo spicchio come posizione, D-583 (4); il timer, D-590 (2)).

## 1. Verdetto in breve

Il documento è una ricerca di interfaccia con un confronto fra giochi, ma le regole di Refactor Tactics che descrive sono in gran parte
quelle del GDD dell'8 novembre: quattro fasi, nove elementi su tre tier, «Flux», Overclock universale a +2 AP, Flanking al +50% con metà
DEF ignorata, copertura in percentuale, dodici spicchi come posizione. Il referto sul GDD le aveva già misurate, e l'autore le aveva
decise lo stesso giorno (`D-583`…`D-599`).

Il valore nuovo sta nelle **idee di interfaccia**: reset di una sola fase, minaccia nemica al passaggio del mouse, prezzo di uno
svantaggio mostrato prima del click, barra degli AP colorata per fase, aggancio alla copertura migliore, laser di quota, icone in bianco e
nero per famiglia di stato. Il rischio sta nel fatto che il testo presenta come regole del gioco numeri già superati, e tre delle sue
idee, prese alla lettera, mostrerebbero l'intento nemico durante la pianificazione.

**L'autore ha deciso** ([D-620](../../decisions/RT_PDR_00_Decision_Log.md)):
- riapre cinque divergenze come voci aperte: `UXR-1` copertura in percentuale, `UXR-2` elementi sui singoli spicchi, `UXR-3` rotazione
  della camera a 30°, `UXR-4` Fuoco + Acqua = Vapore, `UXR-5` Overwatch e Guard come reazioni;
- tiene il contratto su Flanking, facing, Overclock, costi del camminare, tempi degli stati («letto alla fase», [D-569](../../decisions/RT_PDR_00_Decision_Log.md)) e
  contesa di cella (il calendario dei micro-step);
- i dodici elementi restano, e il primo giro di contenuto progetta le coppie fra i sei base;
- due compagne non possono scegliere la stessa cella finale ([#4138](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4138));
- il Combo Risk Meter per ora non c'è, e le parti giudicate overengineering restano solo in questa ricerca.

## 2. Le correzioni dell'autore, prima della lettura

| Correzione, alla lettera | Che cosa misura | Esito |
|---|---|---|
| *«le fasi (sono corrette 5, prep,dash,blast,move,clean)»* | il documento ne dà quattro, con «Reaction / DoT» come quarta | coincide con [D-583](../../decisions/RT_PDR_00_Decision_Log.md) (1) e ADR-0003: Prep, Dash, Blast, Move, Cleanup. Le reazioni sono una lista ([D-592](../../decisions/RT_PDR_00_Decision_Log.md)) e risolvono nei segmenti di ADR-0004, non in una fase propria |
| *«gli elementi (sono ancora da definire, inizieremo con queli base e tier 2 combinata)»* | il documento dà nove elementi (sei base, tre catalizzatori) su tre tier | [D-620](../../decisions/RT_PDR_00_Decision_Log.md) (2): il vocabolario resta quello di [D-588](../../decisions/RT_PDR_00_Decision_Log.md)/[D-593](../../decisions/RT_PDR_00_Decision_Log.md); il primo giro progetta le coppie fra Fire, Water, Earth, Air, Electric e Frost; il Tier 3 resta fuori |
| *«risk meter (non so se tenerlo, non l'ho capito)»* | il Combo Risk Meter del §5 | spiegato all'autore (§5 di questo referto); [D-620](../../decisions/RT_PDR_00_Decision_Log.md) (3): per ora non c'è |

## 3. Mappa delle premesse

Legenda:
- **CONFERMA**: coincide col contratto.
- **TENUTO (D-620 (n))**: diverge da una decisione registrata; l'autore, interrogato, ha tenuto il contratto.
- **RIAPERTO**: l'autore lo riapre come voce di [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md); finché è aperta vale il contratto.
- **DECISO**: regola nuova scelta dall'autore.
- **INVARIANTE**: preso alla lettera violerebbe un invariante di [`CLAUDE.md`](../../../CLAUDE.md) §7, e qui è scritto nella forma che lo rispetta.
- **INPUT**: proposta per la sede indicata, senza impegno e senza numeri in vigore ([D-620](../../decisions/RT_PDR_00_Decision_Log.md) (8a), lettura dell'assistente).
- **RESEARCH**: nessuna sede la riceve oggi; «(4)» quando la nomina la scelta (4) di D-620.
- **BENCHMARK**: affermazione su un altro gioco, non verificata.

| § | Premessa | Classe | Fonte che governa, e misura |
|---|---|---|---|
| intro | tattico competitivo 3v3 e 4v4, determinismo al 100% | **CONFERMA** parziale | 3v3 è lo Standard (`D-256`), 4v4 è stress ([`../../gameplay/spec-durata-partita-e-scala-mappe.md`](../../gameplay/spec-durata-partita-e-scala-mappe.md)); determinismo: [`CLAUDE.md`](../../../CLAUDE.md) §7 |
| 1 | waypoint manuali posati lungo il percorso | **CONFERMA** | esistono: `ARTPlayerController::HandleClickOnCell` posa il waypoint, `ClassifyWaypointCell` lo rifiuta con un motivo (`ERTHexWaypointReason`), `D-420` e `D-444` ne regolano il troncamento |
| 1 | micro-icone di allerta sopra le celle pericolose del percorso | **INPUT** (HUD §9, §15) | gli hazard attraversati restano applicati ([`../../gameplay/spec-tassonomia-movimento.md`](../../gameplay/spec-tassonomia-movimento.md) §3); nessuna allerta per cella è scritta oggi in `progettazione-hud.md` |
| 1 | «Ghosting di Transito»: nessuna collisione in transito, vincolo solo sulla cella finale | **CONFERMA** | [D-445](../../decisions/RT_PDR_00_Decision_Log.md): nessuna unità blocca il transito di nessun'altra, si ferma alla prima cella libera; fermarsi su una cella occupata resta vietato ([D-443](../../decisions/RT_PDR_00_Decision_Log.md)) |
| 1 | nastro traslucido che attraversa l'ostacolo | **INPUT** (HUD §9 Action Ghost) | presentazione |
| 1 | lucchetto sulla cella dove una compagna ha già pianificato di fermarsi, che impedisce il click | **DECISO** ([D-620](../../decisions/RT_PDR_00_Decision_Log.md) (5)) | oggi `ERTHexWaypointReason` non ha un motivo «destinazione già scelta da una compagna» e il piano accetta la cella ([D-442](../../decisions/RT_PDR_00_Decision_Log.md)); l'autore ha scelto il divieto in pianificazione. Attuazione e domande non decise: [#4138](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4138) |
| 1 | stessa cella finale con un nemico: entrambi rimbalzano all'esagono libero precedente | **CONFERMA** per il pareggio; **TENUTO (D-620 (7))** per il resto | chi vince una contesa lo decide il calendario dei micro-step (`Movement.CellContestIsDecidedByCadenceNotIndex`); solo a parità `BlockedContested` per entrambi senza danno ([D-590](../../decisions/RT_PDR_00_Decision_Log.md) (4), [`../../technical/architecture/spec-turnlog.md`](../../technical/architecture/spec-turnlog.md)) |
| 1 | Snap Magnetico al micro-spicchio con la massima riduzione | **INPUT**, tradotto | lo spicchio non è una posizione ([D-583](../../decisions/RT_PDR_00_Decision_Log.md) (4)); il gesto equivalente nel contratto è la scelta di copertura `CoverSelection`, stato discreto nell'hash ([D-302](../../decisions/RT_PDR_00_Decision_Log.md)): l'aggancio può proporre quella con la riduzione migliore contro i nemici visti |
| 1 | Combo Snap e pulsante `COMBO READY` | **RESEARCH** (4) | — |
| 1 | What-If al passaggio del mouse, prima di spendere AP | **CONFERMA** nel principio | `progettazione-hud.md` §16 (Predicted), §19 (Cost Preview), §19-bis |
| 1 | pulsante Reset Fase | **INPUT** (HUD §6.8) | oggi `UNDO` è il Back, un livello alla volta ([D-458](../../decisions/RT_PDR_00_Decision_Log.md)); col pool di AP una fase porta una lista di azioni ([D-584](../../decisions/RT_PDR_00_Decision_Log.md) (1)), quindi «svuota questa fase» ha un oggetto |
| 1 | `Tab` all'unità successiva durante le animazioni, velocità delle animazioni | **INPUT** (HUD §29) | i comandi del playback sono in `progettazione-hud.md` §29 |
| 1 | gamepad, Steam Deck, testi ridimensionabili | **INPUT** (nota in testa a `progettazione-hud.md`; §47-bis per i testi) | l'HUD è PC-first, 1920×1080 (testata di `progettazione-hud.md`): il gamepad è fuori dal target dichiarato, i testi ridimensionabili sono accessibilità |
| 2 | rotazione a scatti di 30° | **RIAPERTO** (`UXR-3`) | in vigore: `YawStep = 45°` in tattica ([D-142](../../decisions/RT_PDR_00_Decision_Log.md)), 60° in strategica ([D-499](../../decisions/RT_PDR_00_Decision_Log.md)). La lettera di D-142 esclude ogni passo divisore di 60, e 30 lo è; se lo escluda anche il motivo (guardare fra due file di celle) è una lettura dell'assistente ([D-620](../../decisions/RT_PDR_00_Decision_Log.md) (9a)) |
| 2 | schema XCOM 2: WASD, `Q`/`E`, zoom su `T`/`G`, piano su `F`/`C` | **BENCHMARK** | nel contratto `T` inquadra l'unità da dietro ([D-618](../../decisions/RT_PDR_00_Decision_Log.md)) e lo zoom è la rotella ([`../../technical/systems/spec-tactical-camera.md`](../../technical/systems/spec-tactical-camera.md)): lo schema non si importa così com'è |
| 2 | tre livelli di quota; dall'alto si annulla la riduzione delle coperture parziali; caduta deterministica | **CONFERMA** | `FRTCellId::Layer`; [D-597](../../decisions/RT_PDR_00_Decision_Log.md) (la quota toglie la copertura bassa, non la Guard); caduta oltre il bordo ([D-332](../../decisions/RT_PDR_00_Decision_Log.md)). «Non mitigabile» e «tre livelli» non sono misurati qui |
| 2 | vettore laser verso il bersaglio più in basso | **INPUT** (HUD §20) | presentazione della regola di D-597 |
| 2 | tabella dei paradigmi di LoS (XCOM 2, Phoenix Point) | **BENCHMARK** | — |
| 2 | ghiera sotto i piedi a dodici settori | **CONFERMA** come righello | [D-303](../../decisions/RT_PDR_00_Decision_Log.md), [D-583](../../decisions/RT_PDR_00_Decision_Log.md) (4) |
| 2 | riduzione fissa: −35% copertura bassa, −75% alta | **RIAPERTO** (`UXR-1`) | in vigore: piatta, sottratta dopo la DEF, bassa 60 ([D-589](../../decisions/RT_PDR_00_Decision_Log.md), [D-590](../../decisions/RT_PDR_00_Decision_Log.md) (3), [D-602](../../decisions/RT_PDR_00_Decision_Log.md), `URTCombatLibrary::LowCoverDamageReduction`); la copertura **alta interrompe la linea** (`URTHexCoverLibrary::BlocksTraversal`), non riduce il danno |
| 2 | misure dell'esagono (lato 1,5 m, 2,6 m fra i centri) e dodici triangoli | **CONFERMA** nella sostanza | ADR-0002; le misure in metri sono presentazione |
| 2 | facing su uno spicchio; Flanking sui quattro spicchi posteriori, +50% e metà DEF ignorata | **TENUTO (D-620 (1))** | facing su sei lati ([D-367](../../decisions/RT_PDR_00_Decision_Log.md)); bonus per personaggio dai tre lati posteriori, 90 · 60 · 30 ([D-587](../../decisions/RT_PDR_00_Decision_Log.md), [D-599](../../decisions/RT_PDR_00_Decision_Log.md)) |
| 2 | timer di 45 secondi | **CONFERMA** per il 3v3 | timer per formato sul misurato ([D-590](../../decisions/RT_PDR_00_Decision_Log.md) (2)): 30 s nel 2v2, 40–45 s nel 3v3 |
| 2 | regia cinematografica: cut-in, freeze-frame di 0,3–0,5 s, onomatopee | **CONFERMA** nella direzione | cel-shaded / manga ([D-590](../../decisions/RT_PDR_00_Decision_Log.md) (1)), E52.1 ([#4005](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4005)) |
| 2 | split-screen diagonale per due colpi nella stessa fase | **INPUT** (E52.1, playback) | — |
| 3 | 6 AP; attacco base a 2 AP senza cooldown; skill da 1 a 3 AP | **CONFERMA** | [D-594](../../decisions/RT_PDR_00_Decision_Log.md): budget 6, Blast 2, Ultimate un punto in più |
| 3 | quattro fasi con «Reaction / DoT» come quarta | correzione dell'autore | §2; [D-583](../../decisions/RT_PDR_00_Decision_Log.md) (1) |
| 3 | Muoversi Piano 1 AP nel Prep; Movimento Tattico 2 AP e Corsa 3 AP nel Dash; Disengage Roll 2 AP | **TENUTO (D-620 (1))** | il camminare sta nel Move e tutti i profili costano uguale ([D-584](../../decisions/RT_PDR_00_Decision_Log.md) (2), [D-594](../../decisions/RT_PDR_00_Decision_Log.md)); lo Sneak è una dichiarazione per movimento ([D-609](../../decisions/RT_PDR_00_Decision_Log.md)) |
| 3 | colpi simultanei nella stessa fase: entrambi infliggono danno | **CONFERMA** | danno simultaneo nel Blast, anche fra azioni della stessa unità ([D-615](../../decisions/RT_PDR_00_Decision_Log.md)) |
| 3 | gli stati di quei colpi «entrano in vigore dal turno successivo» | **TENUTO (D-620 (6))** | lo stato si legge alla fase in cui l'azione risolve: uno stun del Blast nega il Move dello stesso turno ([D-569](../../decisions/RT_PDR_00_Decision_Log.md) (4), scelta dall'autore il 2026-11-03 e riconfermata); simultaneo nella stessa fase è il **danno** (D-615), mentre `Status.Stunned` e `Status.Prone` disarmano le reazioni preparate già dove nascono (`ARTTurnManager::ApplyStatusLogged` → `DisarmPreparedReactions`, letto nel codice). Le durate scalano nel Cleanup (`ARTUnit::TickStatuses`). La descrizione dell'opzione mostrata all'autore riassumeva D-569 in modo inesatto: D-620 (9c) |
| 3 | le Reaction hanno «priorità assoluta» sulle abilità attive nella fase | non misurato; accanto a `UXR-5` | il contratto non lo formula così: ADR-0004 §4 ordina le reazioni fra loro, per trigger e micro-step |
| 3 | quattro sotto-archetipi di Reaction: Overwatch, Guard, Counter, Evasion | **RIAPERTO** (`UXR-5`) | in vigore: `Counter`, `Intercept`, `Deflect`, `Evade` sono reazioni; `Overwatch` e `Guard` sono principali del Prep ([`../../gameplay/spec-economia-del-turno.md`](../../gameplay/spec-economia-del-turno.md)) |
| 3 | skill nominate per eroe (Static Trap, Chain Pull, Skewer, Magnetic Bastion…) | **INPUT** (`MSP-5.d`, `KIT-*`) | un kit si traduce e si accetta con una `D-nnn` ([D-591](../../decisions/RT_PDR_00_Decision_Log.md) (3), [D-612](../../decisions/RT_PDR_00_Decision_Log.md)) |
| 3 | displacement al centro dei kit; confini della mappa da gestire | **CONFERMA** nel principio | spinte e trazioni esistono; la caduta da un bordo aperto è nella v0.1 ([D-332](../../decisions/RT_PDR_00_Decision_Log.md)) |
| 3 | Baldur's Gate 3 ha ridotto le superfici rispetto a Original Sin 2 | **BENCHMARK** | — |
| 3 | nove elementi: sei base e tre catalizzatori (Scudo, Disintegrazione, Flux) | **TENUTO (D-620 (2))** per il vocabolario | dodici elementi piatti ([D-588](../../decisions/RT_PDR_00_Decision_Log.md), [D-593](../../decisions/RT_PDR_00_Decision_Log.md)); «Flux» si legge `Warp`, «Scudo» si legge `Aegis` |
| 3 | stato elementale ancorato ai singoli spicchi divisi da un muro radiale | **RIAPERTO** (`UXR-2`) | in vigore: lo spicchio è geometria tattica, non sottocella, con un solo slot autoritativo per `FRTCellId` ([`CLAUDE.md`](../../../CLAUDE.md) §11, [D-583](../../decisions/RT_PDR_00_Decision_Log.md) (4)) |
| 3 | Tier 2: Fuoco+Terra = Magma, Acqua+Freddo = Ghiaccio, Acqua+Terra = Fango | **INPUT** (`MSP-3.c`, primo giro di [D-620](../../decisions/RT_PDR_00_Decision_Log.md) (2)) | candidati per le coppie Fire–Earth, Water–Frost, Water–Earth ([D-620](../../decisions/RT_PDR_00_Decision_Log.md) (8d), lettura dell'assistente) |
| 3 | Tier 2: Fuoco+Acqua = Vapore | **RIAPERTO** (`UXR-4`) | la coppia Fire–Water è `CURRENT` con `Reaction.WaterQuenchesFire`, l'acqua spegne il fuoco ([`../../balance/RT_ElementPairCatalog_v0.1.md`](../../balance/RT_ElementPairCatalog_v0.1.md)) |
| 3 | Tier 3: Finisher per fase, Wireframe Magenta del Flux | **TENUTO (D-620 (2))** / **RESEARCH** | «il Tier 3 resta fuori» è nel testo dell'opzione scelta; la combo a tre cariche non era stata scelta nemmeno in [D-583](../../decisions/RT_PDR_00_Decision_Log.md) (8) |
| 4 | formula del danno ed esempio (220 contro DEF 20 = 183) | **CONFERMA** parziale | la formula di [D-589](../../decisions/RT_PDR_00_Decision_Log.md) ha anche spalle, copertura, Guard e il pavimento a zero; con quelle a zero e `Mod` 1 l'esempio torna: 220 × 100/120 = 183,3, per difetto 183 ([D-595](../../decisions/RT_PDR_00_Decision_Log.md)) |
| 4 | Into the Breach: esito proiettato prima del click, ordine numerato delle azioni | **BENCHMARK**; **CONFERMA** nel principio | la Ghost Timeline mostra l'ordine della propria squadra (`progettazione-hud.md` §8) |
| 4 | Fire Emblem: linee di minaccia contro un perimetro a richiesta | **BENCHMARK**; **CONFERMA** nel principio | overlay `Threat` a richiesta (`progettazione-hud.md` §24) |
| 4 | Syntax Ring a tre segmenti | **RESEARCH** (4) | resta il principio «la cella mostra il suo stato» (E52, referto GDD) |
| 4 | icone What-If in bianco e nero, in quattro famiglie | **INPUT** (HUD §38, tassonomia icone di [#637](https://github.com/DegrassiAaron/refactor-tactics-main/issues/637)) | per **nome**, dei nomi di stato del documento `git grep 'Status\.<nome>' -- Source/RefactorTactics` trova solo `Status.Root`, `Status.Slow` e `Status.Stunned`. Per **significato** ci sono anche corrispondenze, lettura dell'assistente: il «DoT» è vicino a `Status.Burning`, l'«Armor Break» a `Status.Exposed`. Un'icona nasce con lo stato che rappresenta |
| 4 | Color-Coded Echoes: ombre e nastri della squadra in quattro colori fissi | **INPUT** (HUD §17) | la palette degli overlay la determina il gate ([D-364](../../decisions/RT_PDR_00_Decision_Log.md)); quattro colori fissi per un formato Standard a tre unità vanno misurati lì |
| 4 | Threat Cones al passaggio del mouse su un nemico visibile | **INPUT** (HUD §18, §24) e **INVARIANTE** | si calcolano dalle capacità note del nemico (portata, kit), mai dal suo intento: «le linee di tiro in Blast Phase» del documento, lette come le linee pianificate, violerebbero la privacy |
| 4 | Filtro Acustico: onde concentriche da un nemico nella nebbia che **pianifica** una corsa | **INVARIANTE** | in pianificazione mostrerebbe l'intento avversario ([`CLAUDE.md`](../../../CLAUDE.md) §7). Il rumore esiste come informazione da eventi risolti (`progettazione-hud.md` §26, soglia d'udito di `D-041`, `RTAcousticPropagationLibrary`); i raggi di 2 e 4 esagoni per profilo sono input per quel modello d'intensità, non regole |
| 5 | XCOM 2: icona `>\|` del fine turno, avviso di rivelazione, scomposizione dei modificatori | **BENCHMARK**; **CONFERMA** nel principio per la scomposizione | `progettazione-hud.md` §14.1 (WHY?); il breakdown del danno porta gli stadi della formula |
| 5 | Into the Breach: tutto a 1080p senza sottomenu, giocabile senza audio | **CONFERMA** nel principio | target 1920×1080; il colore non porta mai il significato da solo (`progettazione-hud.md` §47-bis.1) |
| 5 | timeline a 8 tracce: quattro colonne, quattro righe per squadra | correzione dell'autore per le colonne; **INVARIANTE** per le righe avversarie | le colonne sono le cinque fasi (`progettazione-hud.md` §8.1); le righe nemiche si riempiono solo da eventi risolti e visti, mai in pianificazione |
| 5 | in hover, l'incastro della propria azione con quelle delle compagne si illumina sulla timeline | **INPUT** (HUD §8.3 Scrubbing) | — |
| 5 | ping di squadra sui dodici spicchi | **INPUT** (HUD §23) | lo spicchio come righello basta a indicare un punto |
| 5 | barra dei 6 AP colorata per fase | **INPUT** (HUD §19-bis) | i colori di fase esistono (`progettazione-hud.md` §32, «Barra dei comandi e fasi»); le fasi sono cinque, non quattro |
| 5 | Overclock universale a +2 AP, «settimo AP» a barra | **TENUTO (D-620 (1))** | l'Overclock è una skill del kit, un vantaggio pagato con uno svantaggio dopo ([D-612](../../decisions/RT_PDR_00_Decision_Log.md) (5)) |
| 5 | i quattro moduli di debito dell'Overclock come regola universale | **RESEARCH** (4) | — |
| 5 | il prezzo dello svantaggio mostrato prima del click | **INPUT** (HUD §19, §19-bis) | vale per ogni skill che porta uno svantaggio successivo, non solo per l'Overclock |
| 5 | Combo Risk Meter | **RESEARCH** ([D-620](../../decisions/RT_PDR_00_Decision_Log.md) (3)) | per ora; §5 di questo referto |
| 5 | tooltip a micro-icone su quattro livelli | **INPUT** (HUD §7.1, §18.1) | — |
| 5 | Codex e Combo Lab | **RESEARCH** (4) | il banco Ability Lab ([D-607](../../decisions/RT_PDR_00_Decision_Log.md)) è uno strumento di sviluppo, non una modalità del giocatore |
| 5 | ruota elementale nella selezione eroi | **RESEARCH** | lettura dell'assistente: la (4) non la nomina, ma appartiene allo stesso sistema di Codex e Combo Lab, e nessuna sede scrive oggi la selezione eroi |
| 6 | cinque archetipi funzionali di HUD | **RESEARCH** (4) | sono lenti di analisi, non modalità; il selettore di overlay esiste (`progettazione-hud.md` §24) |
| 6 | «validate attraverso i mockup di progettazione» | non verificabile | nessun mockup è allegato al documento |

## 4. Le scelte dell'autore

Quattro giri di domande a scelta; il testo delle opzioni, alla lettera, è in [D-620](../../decisions/RT_PDR_00_Decision_Log.md).

1. **Precedenza** → *«No, alcuni punti li riapro»*. Nel secondo giro riapre **copertura in percentuale**, **elementi sui singoli
   spicchi** e **rotazione camera a 30°**; non riapre Flanking, facing su dodici spicchi, Overclock universale e costi dei movimenti, e per
   loro vale l'intestazione della domanda: «il canone resta in vigore finché non decidi».
2. **Elementi** → *«Restano i dodici, si comincia dai sei base»*.
3. **Combo Risk Meter** → *«No, per ora non c'è»*.
4. **Overengineering** → *«Restano solo nella ricerca»*.
5. **Terzo giro**, dopo che l'assistente gli ha detto di aver registrato per errore come deciso il testo dell'opzione scartata nella (1):
   riapre anche **Fuoco + Acqua = Vapore** e **Overwatch e Guard come Reaction**, e chiede di parlare degli stati; sul lucchetto sceglie
   il **divieto in pianificazione**.
6. **Quarto giro**: sugli stati tiene **«letto alla fase»** (D-569), sulla contesa tiene **il calendario**, e chiede la issue per il divieto
   ([#4138](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4138)).

## 5. Il Combo Risk Meter, come è stato spiegato all'autore

Due compagne preparano una combinazione su una cella, per esempio Acqua nel Prep e Fulmine nel Blast. Il Prep e il Dash si risolvono
prima del Blast, quindi un nemico, nello stesso turno, può mettere prima un altro elemento su quella cella e cambiare l'esito. Il pannello
avvisa: «questa combo si rompe se prima arriva Freddo o Acqua». Non può leggere i piani nemici, quindi elenca solo ciò che è **possibile**
con i kit avversari noti.

Perché oggi non serve: nel catalogo delle coppie solo Fire–Water e Water–Electric sono `CURRENT` (misura del 2026-11-16 su
[`../../balance/RT_ElementPairCatalog_v0.1.md`](../../balance/RT_ElementPairCatalog_v0.1.md); le altre sono `OPEN`), e il documento lo pensa
soprattutto per il Tier 3, che è fuori. Si riconsidera quando il catalogo avrà contenuto ([D-620](../../decisions/RT_PDR_00_Decision_Log.md) (3)).

## 6. Che cosa entra, e dove

| Contenuto | Sede che lo riceve | Come |
|---|---|---|
| idee di interfaccia: reset di una fase, allerte sul percorso, aggancio alla copertura, minaccia in hover, laser di quota, barra AP per fase, prezzo dello svantaggio prima del click, icone in bianco e nero per famiglia, incastro sulla timeline, ping sullo spicchio, testi ridimensionabili | [`../../technical/systems/progettazione-hud.md`](../../technical/systems/progettazione-hud.md) (nota in testa, con le sezioni) | **INPUT**: nessun requisito nuovo finché un owner non lo scrive con i suoi criteri |
| il divieto sulla cella finale di una compagna | [#4138](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4138); nota in `progettazione-hud.md` | **DECISO**: validazione del piano, bot, HUD; le domande non decise sono nella issue |
| copertura in percentuale | `UXR-1` in [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md); note in [`../../decisions/adr-0005-orientamento.md`](../../decisions/adr-0005-orientamento.md) e sulle righe D-589, D-590, D-602 | aperta; vale il contratto |
| elementi sui singoli spicchi | `UXR-2`; nota in [`../../technical/systems/spec-cover-placement-intra-hex.md`](../../technical/systems/spec-cover-placement-intra-hex.md) | aperta; vale l'invariante di `CLAUDE.md` §11 |
| rotazione a 30° | `UXR-3`; note in [`../../technical/systems/spec-tactical-camera.md`](../../technical/systems/spec-tactical-camera.md) e sulla riga D-142 | aperta; vale D-142 |
| Fuoco + Acqua = Vapore | `UXR-4`; nota nel catalogo delle coppie | aperta; resta `Reaction.WaterQuenchesFire` |
| Overwatch e Guard come reazioni | `UXR-5`; nota in [`../../gameplay/spec-economia-del-turno.md`](../../gameplay/spec-economia-del-turno.md) | aperta; restano principali del Prep |
| primo giro delle coppie: i sei base | [`../../balance/RT_ElementPairCatalog_v0.1.md`](../../balance/RT_ElementPairCatalog_v0.1.md) (nota in testa), `MSP-3.c` | gli stati Tier 2 del documento sono candidati per le coppie corrispondenti, tranne il Vapore |
| tempi degli stati, contesa di cella | righe D-569 e D-590 annotate | riconfermati |
| Combo Risk Meter, Combo Snap, Syntax Ring a tre segmenti, Codex, Combo Lab, cinque archetipi, moduli di debito universali | nessuna | **RESEARCH**: restano nel documento d'ingresso |

## 7. Il panel

- **Wiegers**: il documento non porta un solo criterio d'accettazione, e mescola tre registri nella stessa frase: ciò che fa un altro
  gioco, ciò che fa Refactor Tactics, ciò che dovrebbe fare. La mappa del §3 li separa; senza di lei, «Corsa da 3 AP» si legge come una
  regola in vigore.
- **Adzic**: le idee migliori sono quelle che si scrivono come esempio. «Passo sopra un nemico e vedo l'esito prima del click» si verifica;
  «la UI opera come un calcolatore spaziale privo di attrito» no. Ogni INPUT della §6 diventa requisito solo con un esempio Given/When/Then
  nella sezione dell'HUD che lo riceve.
- **Cockburn**: l'attore primario è chi pianifica sotto il timer. Il reset di una fase e la minaccia in hover servono a lui; i cinque
  archetipi servono a chi analizza l'interfaccia, ed è giusto che restino nella ricerca.
- **Fowler**: il Combo Risk Meter e il Syntax Ring sono un secondo modello delle combinazioni, accanto al catalogo delle coppie. Finché il
  catalogo è quasi tutto `OPEN`, quel modello mostrerebbe dati che non esistono.
- **Nygard**: il guasto più costoso del documento non è un numero, è la privacy. Il Filtro Acustico, le righe nemiche della timeline e i
  coni di tiro, presi alla lettera, chiedono al client l'intento avversario; un'interfaccia che lo riceve e lo nasconde viola
  [`CLAUDE.md`](../../../CLAUDE.md) §7 anche se a schermo non si vede niente.
- **Crispin**: il primo test che il documento rende scrivibile è sull'anteprima: «il danno mostrato in hover è quello che il resolver
  applica», sullo stesso esempio del documento (220 contro DEF 20 → 183). Il secondo è quello di [#4138](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4138):
  la seconda compagna che dichiara la stessa cella finale riceve un rifiuto col motivo nuovo.

## 8. Difetti interni del documento, indipendenti dal contratto

1. §5 dà all'Overclock due varianti nello stesso paragrafo: «+2 AP istantanei […] portando il budget a 8 AP» e «sbloccando il 7° AP
   tramite Overclock Meter nella variante a barra».
2. §2 mette il Flanking sui «4 spicchi del cono posteriore (90°–120°)»: quattro spicchi da 30° sono 120°, non un intervallo che parte da 90°.
   È lo stesso difetto del GDD (referto GDD §6.3).
3. I profili di movimento hanno due nomi: «Movimento Tattico da 2 AP, Corsa da 3 AP» nel §3, «Tactical Run da 2 AP; Fast Sprint da 3 AP»
   nel §4.
4. La timeline ha «8 righe orizzontali (4 per il Team A, 4 per il Team B)» e i colori «dei 4 alleati», ma il documento dichiara anche il
   3v3, che ne avrebbe sei e tre.
5. «Fuoco + Acqua = Vapore» contraddice la sola coppia di quei due elementi che esiste a runtime (`Reaction.WaterQuenchesFire`).
6. §6 dichiara gli archetipi «validati attraverso i mockup», ma nessun mockup accompagna il testo.

## 9. Provenienza, limiti e stati di verifica

- Testo letto per intero. È arrivato incollato senza a capo: titoli, paragrafi, elenchi e tabelle del documento d'ingresso sono ricostruiti
  dall'assistente, le parole no.
- Misure sul repository (`origin/main` = `b477e66c3`): `ERTHexWaypointReason` (`RTHexSimLibrary.h`), `YawStep` (`RTCameraPawn.h`),
  `URTCombatLibrary::LowCoverDamageReduction`, `URTHexCoverLibrary::BlocksTraversal`, `URTHeroCatalogLibrary::ElementIds()`,
  `ARTUnit::TickStatuses`, gli stati della tabella delle coppie in `RT_ElementPairCatalog_v0.1.md`, i tag `Status.*` in
  `Source/RefactorTactics`, `RTAcousticPropagationLibrary`, le sezioni di `progettazione-hud.md` citate nella §3.
- **Revisione avversaria** (un agente indipendente, sul primo commit del branch): ha trovato un errore bloccante e lo si dichiara. La
  prima stesura di D-620 e di questo referto scriveva come deciso «dove il documento diverge governa il contratto», che era il testo
  dell'opzione **scartata**; diverse divergenze non erano state mostrate all'autore. Ne sono nati il terzo e il quarto giro di domande.
  Corretti anche: il «Combo Risk Meter non c'è» senza «per ora», la lettura su D-142 scritta come fatto, la privacy dei coni di tiro
  assente nella nota dell'HUD, classificazioni attribuite a una scelta che non le nominava, una frase superata in `MSP-3.c`, le righe
  riaperte senza rimando. Un **secondo giro** sulle sole correzioni ha trovato che la descrizione di un'opzione del quarto giro
  riassumeva D-569 in modo inesatto ([D-620](../../decisions/RT_PDR_00_Decision_Log.md) (9c), detto all'autore) e cinque rilievi minori, corretti.
- Le righe **BENCHMARK** riportano ciò che il documento dice degli altri giochi: nessuna è stata verificata su una fonte esterna.
- Compile · Tests · Determinism · Replay · Privacy · PIE · Packaged: `N/A`. Nessun codice, asset o scenario cambia.
