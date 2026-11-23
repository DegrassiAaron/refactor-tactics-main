# Gadget — traduzione del catalogo del 2026-11-20

> 🧪 **Stato:** `DESIGN_SPEC` · **PROPOSTA** per la `D-nnn` d'accettazione del kit di Gadget, che [D-633](../../decisions/RT_PDR_00_Decision_Log.md) prevede per ogni eroe e la [#4170](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4170) chiede.
> **Non è in vigore.** Il kit in vigore è quello della [scheda di Gadget](../../characters/v0.1/gadget.md) e del [catalogo eroi](../../balance/RT_HeroCatalog_v0.1.md) §1, costruito da `URTHeroCatalogLibrary::MakeGadget`. Questa pagina non decide nulla: traduce il catalogo nelle regole del contratto e mette in fila ciò che l'autore deve scegliere. I numeri del catalogo sono la base del bilanciamento e sostituiscono quelli in vigore solo con la `D-nnn` [LETTURA D-633 (11f)]; a runtime, solo quando arriva il codice.
>
> **Base:** `origin/main` `f012ed660` (worktree `4170-kit-gadget`, ramo `docs/4170-kit-gadget`).
> **Input:** il catalogo dell'autore del 2026-11-20, conservato invariato nel [file d'input](../../research/handoff/catalogo-14-eroi-input-2026-11-20.md) (§1 regole, §2 roster, §3 «Gadget») e nelle righe `GAD_*` del [`.csv`](../../research/handoff/catalogo-14-eroi-input-2026-11-20.csv); le scelte dell'autore della sessione del 2026-11-23, nel §2 del [design](../../superpowers/specs/2026-11-23-gadget-kit-traduzione-design.md). La forma è quella della [traduzione di Riktor](riktor-kit-traduzione-2026-11-13.md); le sue regole sono rilette sul Decision Log di oggi, non copiate, perché alcune sono cambiate dopo il 2026-11-13.
> **Decisioni rispettate:** [D-633](../../decisions/RT_PDR_00_Decision_Log.md) (il catalogo come base, nove elementi, overclock per tutti, Overwatch nel kit, spalle di oggi, copertura bassa fissa, nessuna progressione, Tier 3 fuori, tutte le opzioni nel kit, nomi Paragon); [D-583](../../decisions/RT_PDR_00_Decision_Log.md) (fasi e posizione); [D-584](../../decisions/RT_PDR_00_Decision_Log.md), [D-594](../../decisions/RT_PDR_00_Decision_Log.md), [D-613](../../decisions/RT_PDR_00_Decision_Log.md), [D-614](../../decisions/RT_PDR_00_Decision_Log.md) e [D-615](../../decisions/RT_PDR_00_Decision_Log.md) (Action Point, cooldown, ripetizione, ordine nella fase); [D-587](../../decisions/RT_PDR_00_Decision_Log.md) e [D-599](../../decisions/RT_PDR_00_Decision_Log.md) (spalle); [D-589](../../decisions/RT_PDR_00_Decision_Log.md), [D-595](../../decisions/RT_PDR_00_Decision_Log.md) e [D-602](../../decisions/RT_PDR_00_Decision_Log.md) (danno); [D-597](../../decisions/RT_PDR_00_Decision_Log.md) (quota e copertura bassa); [D-592](../../decisions/RT_PDR_00_Decision_Log.md) e [D-619](../../decisions/RT_PDR_00_Decision_Log.md) (reazioni); [D-593](../../decisions/RT_PDR_00_Decision_Log.md) (identificatori degli elementi); [D-621](../../decisions/RT_PDR_00_Decision_Log.md) (catena di `LinearDischarge`).

## Come leggere le provenienze

Ogni valore porta fra parentesi quadre la sua fonte. Un numero senza fonte non c'è.

| Etichetta | Significato |
|---|---|
| **[CATALOGO]** | testo del `.md` del catalogo, §3 «Gadget» del file d'input; §1 o §2 quando la cella lo dice |
| **[CSV <Skill_ID>]** | campo che c'è solo nel `.csv`, nella riga di quel `Skill_ID` (per esempio `[CSV GAD_BAS_01]`) |
| **[DECISO D-nnn (k)]** | testo scelto o scritto dall'autore in una voce del [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md) la cui cella Stato la dà in vigore («Accettata» o «Consolidata») e non dichiara superata la parte citata |
| **[LETTURA D-nnn (k)]** | lettura dell'assistente registrata dentro una voce, nella parte «Letture dell'assistente, non decisioni»: **non** è decisa |
| **[SESSIONE 2026-11-23]** | scelta dell'autore nella sessione del 2026-11-23, registrata nel §2 del [design](../../superpowers/specs/2026-11-23-gadget-kit-traduzione-design.md); la `D-nnn` la riporterà alla lettera |
| **[PROPOSTA]** | scelta di questa traduzione, col motivo; la decide la `D-nnn` |
| **[DOMANDA → TG-n]** | decisione d'autore; l'elenco consolidato è in [Domande per l'autore](#domande-per-lautore). Una domanda già aperta per Riktor non si duplica: la `TG-n` cita la sua `TR-n`, e una risposta vale per entrambi |
| **[codice]** | `file:riga` letto su `f012ed660`, **non eseguito** |

**Schede.** Ogni voce del catalogo ha una scheda intitolata ``### <Nome del catalogo> — `Hero.Gadget.<Id>` ``, con l'id del §6 del design (il nome del catalogo in PascalCase, per esempio `Hero.Gadget.SparkBolt`), e una tabella `Campo · Valore` in cui ogni cella di valore porta almeno un'etichetta di questa legenda.

**Conteggi.** Un numero che cambia da solo (scenari, test, file che citano un id) non è scritto: al suo posto c'è il comando che lo produce, da eseguire nella radice del repository. Gli zeri misurati restano scritti, col comando e il commit su cui valgono.

**Convenzione degli esempi.** Salvo dove l'esempio dice altro: unità sul `Layer` 0; bersaglio colpito di fronte (il termine Spalle vale 0), senza copertura né Guard, `Mod` 100; lo scudo base, 50, che ferma solo il danno `Direct` [codice: `RTCombatLibrary.h:302-312`], è già eroso; un solo arrotondamento, per difetto [DECISO D-595 (1); codice: `RTCombatResolver.h:141-144`]. Un colpo energetico dice quale lettura di `TG-7` usa. Gli esempi sono criteri d'accettazione candidati: valgono dopo che le domande che citano sono chiuse.

## Come leggere la traduzione

Ogni riga è riletta sulla cella Stato della decisione che cita, al 2026-11-23.

| Nel catalogo | Nel contratto, in questa pagina |
|---|---|
| «4 Fasi Cronologiche»: Prep (Fase 0), Dash (Fase 1), Blast (Fase 2), «Reaction / DoT Phase» (Fase 3) [CATALOGO §1.1] | il Planning e cinque macro-fasi di risoluzione, Prep → Dash → Blast → Move → Cleanup [DECISO D-583 (1); DECISO D-633 (1), «Le fasi sono 5»]. «La «Reaction Phase» della specifica non si recepisce», e «Overwatch, Guard e Brace restano segmenti che scattano sulla causa, nella fase in cui la causa avviene» [DECISO D-583 (1)]. Il danno nel tempo di oggi, `Burning`, si applica nel Cleanup [codice: `BurningCleanupDamage`, `RTCombatLibrary.h:300`, applicato nel Cleanup in `RTTurnManager.cpp:1998`]. Nel resolver: Preparation 10 (Prep), FastMovement 20 (Dash), Control 30 e Attack 40 (dentro il Blast), NormalMovement 20 (Move), Environment 50 e Cleanup 60 (Cleanup) [codice: `RTActionDef.h:20-39`; `URTCatalogLibrary::MapResolutionPhase`, `RTCatalogLibrary.cpp:171-187`; `ResolutionPhaseCode`, `:203-216`] |
| ordine dentro la fase: il catalogo non lo dà [CATALOGO §1.1, §3] | prima la macro-fase, poi la priorità crescente (il numero basso risolve prima), poi l'id dell'azione in ordine lessicale, poi l'unità [codice: `URTActionQueueLibrary::InstanceLess`, `RTActionQueueLibrary.cpp:6-30`; `DeclarationIndex` non entra nel confronto, `RTActionQueue.h:100-110`]. L'ordine di dichiarazione **non decide mai** la risoluzione, neanche fra due azioni della stessa unità: a pari priorità decide l'ordine del motore, «Per id stabile dell'azione», e la dichiarazione resta nel piano e nel TurnLog solo per mostrarla [DECISO D-615 (5), che ribalta D-613 (3)]. Nel Cleanup le azioni ambientali di unità diverse restano nell'ordine delle unità di oggi, la cella e poi `StableUnitId` [DECISO D-616 (4)]. Nel Blast il danno è simultaneo, anche fra due azioni della stessa unità [DECISO D-615 (3); D-503] |
| «Economia AP: Budget fisso di 6 AP per eroe per turno» [CATALOGO §1.1]; il costo nel titolo di ogni slot, «2 AP», «1 AP», «3 AP», «1–2 AP» [CATALOGO] | il budget è un numero dell'unità per il turno, noto quando si pianifica, e un piano validato non cambia mai [DECISO D-613 (1)]; 6 è il default [LETTURA D-613 (5); codice: `RTActionPoints::DefaultBudget`, `RTTypes.h:39`, in `ARTUnit::ActionPointBudget`, `RTUnit.h:1608`]. Costi per classe: Prep 1 · Dash 2 · Blast 2 · Move 2 · reazione 2 [DECISO D-594 (1)]; ogni riga dichiara il proprio costo, col default della classe [DECISO D-594 (3)]; ogni Ultimate costa 3 in qualunque fase [DECISO D-613 (4)] [codice: `URTCatalogLibrary::DefaultActionPointCost`, `RTCatalogLibrary.cpp:219-247`; costanti in `RTCatalogLibrary.h:116-124`]. Al lock-in un piano oltre il budget si svuota e il Ready si rifiuta [DECISO D-615 (1)] |
| «0 CD», «CD n» [CATALOGO]; `Cooldown_Turns` [CSV GAD_BAS_01 e seguenti] | il cooldown resta: «Cooldown, status auto-inflitti e rinunce restano contenuto del catalogo; il costo AP si aggiunge» [DECISO D-584 (4)]. Un'azione con cooldown entra al più una volta nel piano di un'unità; una senza cooldown (l'attacco base, muoversi) si ripete finché il budget lo consente [DECISO D-614]; conta il cooldown della riga di catalogo, prima della variante d'arma [DECISO D-615 (6)]. Il cooldown si imposta all'uso e si sconta nel Cleanup dello stesso turno [codice: `ARTUnit::ConsumeAbility`, `RTUnit.cpp:2662-2673`; `TickCooldowns`, `:2675-2684`, chiamata in `RTTurnManager.cpp:2137`; il rifiuto `OnCooldown` legge un residuo maggiore di 0, `RTPlanValidationLibrary.cpp:274-276`]: con cooldown 1 un'azione vale «una volta per piano, ma ancora in ogni turno» [DECISO D-615 (4), testo dell'opzione scelta per l'Overwatch], con 2 salta il turno dopo. [PROPOSTA] il «CD n» del catalogo è lo stesso numero, perché è la semantica che l'autore ha scelto per l'Overwatch; se il catalogo intende «n turni d'attesa», ogni cooldown sale di 1 [DOMANDA → TG-6] |
| «Danno Energetico», «Danno Fuoco», «gravi danni elettrici» [CATALOGO] | [PROPOSTA] danno elementale dell'elemento della voce, mitigato dalla RES di quell'elemento con la forma della DEF [DECISO D-589 (1) per la forma]. Se lo sia davvero è la domanda di Riktor `TR-1`, ancora aperta [DOMANDA → TG-7; una risposta vale per entrambi]. Oggi `ERTDamageSource` è {`Direct`, `Environmental`} [codice: `RTCombatLibrary.h:177-181`]: un percorso elementale **non esiste**, e nessun lettore in combattimento chiama `ARTUnit::ResistanceAgainst` [codice: `RTUnit.h:143-151`; `git grep -n ResistanceAgainst -- Source/RefactorTactics/Combat Source/RefactorTactics/Turn` → exit 1 su `f012ed660`] |
| «Danni perforanti», «ignora coperture parziali» [CATALOGO, Sticky Bomb]; `Ignores_Armor` = `Yes` [CSV GAD_ULT_01] | la formula del danno non ha un termine che salti DEF o RES, copertura e Guard [DECISO D-589 (3), la formula]: è la domanda di Riktor `TR-3`, ancora aperta [DOMANDA → TG-8; una risposta vale per entrambi]. La copertura bassa oggi non riduce né le aree né il danno ambientale [codice: `LowCoverDamageReduction`, `RTCombatLibrary.h:502-511`] |
| «I 9 Elementi Ufficiali»: Fuoco, Acqua, Terra, Aria, Fulmine, Freddo come «Elementi Base»; Scudo, Disintegrazione, Flux come «Catalizzatori / Modificatori» [CATALOGO §1.3]; `Element_9`, `Legacy_Element` [CSV GAD_BAS_01 e seguenti] | nove elementi, «per il momento» [DECISO D-633 (1)], con gli identificatori di D-593: `Element.Fire`, `Element.Water`, `Element.Earth`, `Element.Air`, `Element.Electric`, `Element.Frost`, `Element.Aegis` (Scudo), `Element.Disintegration`, `Element.Warp` (Flux) [LETTURA D-633 (11b)]. Escono `Element.Decay`, `Element.Life` e `Element.Arcane`, che il catalogo riallinea su Disintegrazione, Scudo e Flux [CATALOGO §1.3; LETTURA D-633 (11b)]: la colonna `Legacy_Element` («Arcano» in EMP Disruption) si legge così, e conta solo `Element_9` [PROPOSTA: è il riallineamento che il catalogo stesso dichiara]. In codice `URTHeroCatalogLibrary::ElementIds` ne elenca ancora dodici [codice: `RTHeroCatalogLibrary.cpp:184-193`], e `FRTActionDef` **non ha** un campo elemento [codice: `git grep -n Element -- Source/RefactorTactics/Ability/RTActionDef.h` → exit 1 su `f012ed660`] |
| «Gli elementi si accumulano sugli esagoni (fino a 3 cariche: Tier 1 innesco → Tier 2 stato composto → Tier 3 detonazione …)»; «esagoni già caricati», «carica Flux» [CATALOGO §1.3, §3] | il Tier 3 resta fuori; si progettano le coppie (Tier 2) a partire dai sei base, e le skill che citano una combo a tre si traducono senza [DECISO D-633 (8)]. Una carica elementale di cella non esiste [codice: `git grep -n -i -e ElementCharge -e ElementalCharge -e CellCharge -e ChargeTier -e Status.Charged -- Source` → exit 1 su `f012ed660`]: oggi una cella ha una superficie e un'unità ha degli stati, come `Status.Wet` [codice: `ERTHexSurface`, `RTHexCellData.h:17`]. Ogni scheda che usa una carica la nomina fra i sistemi che mancano [PROPOSTA: il sistema si nomina, non si inventa] |
| «Matrice Combo, Sinergie e Matchup» di Gadget [CATALOGO] | candidate per le righe del [catalogo delle coppie](../../balance/RT_ElementPairCatalog_v0.1.md) (owner [DECISO D-275]), mai voci della scheda dell'eroe [LETTURA D-633 (11e); AGENTS.md §3, invarianti 15 e 16]: «le sinergie sono derivate», e «Nessun bonus implicito di coppia/fazione» [DECISO D-029]. Le combo a tre elementi, Collasso Strutturale e Polarizzazione Ionica [CATALOGO], restano candidate senza regola [DECISO D-633 (8)] |
| «Tramite Overclock è possibile ottenere +2 AP immediati (totale 8 AP) subendo un Phase Lock (blocco integrale di una fase a scelta nel turno successivo)» [CATALOGO §1.1] | «overclock e' un tipo di skill che hanno tutti (aumentano i AP disponibili o altri vantaggi per un round con un costo nel round successivo)» [DECISO D-633 (1), risposta libera dell'autore, alla lettera]: Gadget ne ha una, da progettare [DECISO D-633 (1)]. Un budget sopra 6 è ammesso e noto al planning, «8 con l'Overclock se entra» [DECISO D-613 (1)]; una skill scelta nel planning entra nel piano prima della validazione [LETTURA D-613 (5)], e l'overclock scelto resta fuori dal digest di stato perché è un intento [DECISO D-615 (2)]. Il meccanismo unico del §1.1 è il punto (d) che D-591 registra come superato per il GDD [DECISO D-591 (1)]; per questo catalogo D-633 non l'ha misurato (#4170, punto 4), e resta un'alternativa alla skill di Gadget nella sezione sull'overclock [PROPOSTA: la definizione dell'autore descrive un tipo di skill per eroe, non un meccanismo uguale per tutti] |
| «Danno Reale = (Danno Base × Moltiplicatore) × 100/(100 + DEF o RES_eff) × (1 − DR_copertura)» [CATALOGO §1.2] | `Danno = max(0, (Base + Spalle) × Mod × 100/(100+DEF) − Copertura − Guard)` [DECISO D-589 (3): «− Copertura − Guard, mai sotto zero»], con DEF e RES per elemento nella stessa forma [DECISO D-589 (1)], sulla scala ×10 [DECISO D-589 (2)]; un solo arrotondamento, per difetto, dopo il prodotto [DECISO D-595 (1)] [codice: `URTCombatLibrary::MitigateDirectDamage`, `RTCombatLibrary.cpp:134-147`]. La copertura bassa è una sottrazione fissa, 60, dopo la DEF, e non il −35% del catalogo [DECISO D-633 (7)]; Guard 100 e Brace 60 [DECISO D-602] [codice: `RTCombatLibrary.h:292`, `:497`, `:511`]. La parte moltiplicativa coincide: «DEF Fisica: 15 (≈13% rid.)» e «RES Energetica: 35 (≈26% rid.)» [CATALOGO §3] sono `1 − 100/115` e `1 − 100/135`, quindi i numeri del catalogo stanno già sulla forma di D-589 [PROPOSTA: conto di questa pagina] |
| «Flanking: colpire un bersaglio nel suo cono posteriore (90°–120°, i 4 spicchi opposti al Facing) applica +50% di danno fisso e ignora il 50% della DEF» [CATALOGO §1.2] | resta il bonus alle spalle di oggi: «Tre lati posteriori, un valore fisso per eroe che attacca (Kallari 90, standard 60, tank 30), sommato al danno prima della DEF.» [DECISO D-633 (2)]; Gadget è nella fascia standard, 60 [DECISO D-599; codice: `RTHeroCatalogLibrary.cpp:403`] |
| «Vantaggio di Elevazione: attaccare da un livello superiore (Z_attaccante > Z_difensore) ignora la Riduzione Danno (DR 35%) delle coperture basse» [CATALOGO §1.2] | è già una regola: un colpo il cui attaccante sta su un `Layer` maggiore di quello del difensore annulla la riduzione da copertura bassa, non la Guard [DECISO D-597], **non implementata** (cella Stato di D-597; l'asserto che la voce nomina non c'è: `git grep -n HighGroundIgnoresLowCover -- Source` → exit 1 su `f012ed660`). Del catalogo cambia solo la misura della copertura, fissa a 60 [DECISO D-633 (7)]. La #4170, punto 4, la mette fra le regole che D-633 non ha misurato: è vero, ma la decide già D-597, e qui non nasce una domanda [PROPOSTA] |
| «salti sull'Asse Z (Z = 0, 1, 2)» [CATALOGO §1.1] | il livello è il `Layer` di `FRTCellId` [codice: `RTCellId.h:28-39`], letto sul piano della cella e non sull'altezza del mesh [LETTURA D-597]. In codice «Z» è un'altra cosa, la terza coordinata cubica, `CubeZ()` [codice: `RTCellId.h:23`, `:45`]. `Layer` è un intero senza massimo nella struttura [codice: `RTCellId.h:38-39`]: se i tre livelli del catalogo siano un tetto per le mappe o un esempio è [DOMANDA → TG-9] (#4170, punto 4) |
| «Sottogriglia a 12 Micro-Spicchi da 30°»; «esagono + 3 spicchi contigui», «3 spicchi adiacenti (90°)», «ruota forzatamente il Facing del bersaglio di 60° (2 spicchi)», «60° (uno spicchio adiacente)» [CATALOGO §1.2, §3] | bersagli e rotazioni a spicchi da 30° si traducono in celle e in passi da 60° [LETTURA D-633 (11c)]: un solo slot di occupancy, perché «La cella è l'unità di posizione» e «la posa visiva nel settore è presentazione» [DECISO D-583 (4)]; il facing è una delle sei direzioni della cella [codice: `ERTHexDirection`, `RTCellId.h:11-19`]. `UXR-2` (lo stato elementale sui singoli spicchi) e `UXR-3` (la camera a 30°) restano aperte [LETTURA D-633 (11c)]. Il catalogo usa «spicchio» con due misure, 30° in Phase Dart («60° (2 spicchi)») e 60° in Flux Deflector («60° (uno spicchio adiacente)») [CATALOGO]: la scheda dice quale legge. Ogni area porta la forma scelta e il numero di celle; un'area esagonale di raggio r ha `3r(r+1)+1` celle, 7 per r = 1 e 37 per r = 3 [PROPOSTA: conto della griglia] |
| «Guard (mitigazione/protezione)», «Counter (contrattacco/punizione)», «Evasion (spostamento/schivata)» [CATALOGO §1.1]; `Reaction_Archetype` [CSV GAD_RCT_01, GAD_RCT_02] | si traducono sulle semantiche che esistono già, `Intercept`, `Deflect`, `Counter`, `Evade` [LETTURA D-633 (11d)] [codice: `Action.Counter`, `RTCatalogLibrary.cpp:1613`; `Action.Deflect`, `:1629`; `Action.Intercept`, `:1647`; `Action.Evade`, `:1709`]; quale per ciascuna voce lo dice la sua scheda. Un'unità arma più reazioni, in lista [DECISO D-592]; una reazione scatta al più una volta per turno, e reazioni diverse della stessa unità anche sullo stesso colpo, in ordine canonico [DECISO D-619]. Il catalogo dà 1 AP a Flux Deflector e a Feedback Loop [CATALOGO; CSV GAD_RCT_01, GAD_RCT_02], sotto la classe reazione 2 [DECISO D-594 (1)]. Il testo scelto dice che ogni riga «dichiara il proprio costo AP (default: quello della classe di fase)» [DECISO D-594 (3)]; [PROPOSTA] l'1 AP vale come costo di riga, perché quel testo fa della classe un default e non un minimo |
| «Overwatch (intercettazione in Dash Phase)» [CATALOGO §1.1]; `Reaction_Archetype` = `Overwatch` [CSV GAD_RCT_03] | una reazione del kit: «Overwatch diventa un tipo di reazione del kit: chi non l'ha nel kit non può fare overwatch. Riapre D-012/D-014 e chiude UXR-5.» [DECISO D-633 (6)]; l'Overwatch di Gadget è Tesla Trap [DECISO D-633 (6)]. Oggi è ancora un'azione del core, `Action.Overwatch`, in Preparation, priorità 45, cooldown 1 [DECISO D-615 (4); codice: `RTCatalogLibrary.cpp:1405-1407`], e con D-633 nessun codice cambia (cella Stato di D-633) |
| «Sintonizzazione Liv. 5: variante Fuoco» [CATALOGO, Seeker Bot] | nessuna progressione: le note «Liv.» si tolgono, e una voce che è già un'opzione di uno slot resta come voce normale [DECISO D-633 (3); D-598] |
| «Catalogo Abilità (6 Slot × 3 Opzioni)» [CATALOGO] | tutte le voci entrano nel kit, più la skill di overclock; la struttura del kit e la dock restano aperte, `KIT-1` e `KIT-2` [DECISO D-633 (9)]. `Hero.Gadget.LinearDischarge` resta [DECISO D-633, richiesta dell'autore: «Nella lista manca LinearDischarge di Gadget, da mantenere.»], come quarta opzione dell'Abilità Speciale con i numeri e la catena di oggi [LETTURA D-633 (11g); DECISO D-621 per la catena]. Escono `ArcPulse`, `ConductiveNode`, `Overload` e `ReactiveCapacitor` [SESSIONE 2026-11-23]: vedi «Che cosa esce» |
| nomi dell'eroe e delle voci [CATALOGO] | i nomi sono quelli di Paragon [DECISO D-633 (1), «I nomi che usiamo, al momento, sono quelli di Paragon»]; l'id di una voce è `Hero.Gadget.<nome del catalogo in PascalCase>`, per esempio `Hero.Gadget.SparkBolt` [PROPOSTA: scelta del design §6; il prefisso è quello di oggi, `RTHeroCatalogLibrary.cpp:369`] |

## Statistiche base

Restano quelle in vigore finché la `D-nnn` non decide [LETTURA D-633 (11f)]. «In vigore» è il [catalogo eroi](../../balance/RT_HeroCatalog_v0.1.md) (§1 e la tabella di confronto del §5) con `MakeGadget` [codice: `RTHeroCatalogLibrary.cpp:366-415`]; «Catalogo» è il §2 e il §3 del file d'input.

| Statistica | In vigore | Catalogo | Proposta |
|---|---|---|---|
| HP | 900 [codice: `RTHeroCatalogLibrary.cpp:380`] | 1000 [CATALOGO §2, §3] | 1000 [PROPOSTA: il numero del catalogo, base del bilanciamento per la LETTURA D-633 (11f); sta nella fascia «HP 900–1200» della scala ×10, DECISO D-589 (2)] |
| DEF | 15 [codice: `RTHeroCatalogLibrary.cpp:401`; DECISO D-595 (3)] | 15, «DEF Fisica», «≈13% rid.» [CATALOGO §2, §3] | 15 [PROPOSTA: coincide con quella in vigore] |
| RES | 35, default per ogni elemento, nessuno differenziato [codice: `ResistanceDefault`, `RTHeroCatalogLibrary.cpp:402`; `ElementResistance` vuota, `RTHeroData.h:126-134`; DECISO D-595 (3)] | 35, «RES Energetica», una sola [CATALOGO §3] | 35 come default per i nove elementi, `ElementResistance` vuota [PROPOSTA: stesso numero e stessa forma di oggi, nessun codice cambia]; una RES sola o una per elemento [DOMANDA → TG-1] |
| Movimento | 5 MP [codice: `RTHeroCatalogLibrary.cpp:381`] | «3 esagoni» [CATALOGO §3; §2, colonna «Move»] | 3 MP [PROPOSTA: il numero del catalogo]; 3 o 5 [DOMANDA → TG-2] |
| Gittata | nessun campo dell'eroe: la portata è di ogni azione [codice: `URTHeroData`, `RTHeroData.h:40-204`; `FRTActionDef::RangeCells`, `RTActionDef.h:584`]; `ArcPulse` 4 [codice: `RTHeroCatalogLibrary.cpp:419`], `LinearDischarge` 5 [codice: `RTHeroCatalogLibrary.cpp:428-430`] | «Gittata Base: 6 esagoni» [CATALOGO §3]; Spark Bolt «a lungo raggio (6 esagoni)» [CATALOGO] | nessun campo nuovo: 6 è la portata dell'attacco base del catalogo, e ogni voce porta la propria [PROPOSTA: in codice la portata è un dato dell'azione] |
| Vista | 7 [codice: `RTHeroCatalogLibrary.cpp:393`; DECISO D-073] | non data [CATALOGO §2, §3] | 7 [PROPOSTA: il catalogo non la dà]; [DOMANDA → TG-3] |
| Resistenza alla spinta | 0 [codice: `RTHeroCatalogLibrary.cpp:395`] | non data [CATALOGO §2, §3] | 0 [PROPOSTA: il catalogo non la dà]; [DOMANDA → TG-3] |
| Spalle | 60 [codice: `RTHeroCatalogLibrary.cpp:403`; DECISO D-599] | nessun numero: il catalogo dà il flanking [CATALOGO §1.2] | 60 [DECISO D-633 (2), «Resta quello di oggi»] |
| Affinità e debolezza | `Affinity.Electricity`, debolezza `Affinity.Water` [codice: `RTHeroCatalogLibrary.cpp:409`, `:412`] | «Flux (Primario), Fulmine, Disintegrazione (varianti su Aria, Scudo, Freddo, Fuoco)»; nessuna debolezza [CATALOGO §3] | il campo di oggi resta finché la domanda non è chiusa [PROPOSTA: nessuna regola di gioco lo legge, vedi «Che cosa resta da decidere»]; [DOMANDA → TG-5] |

⚠️ **Fonti che divergono.** La [scheda di Gadget](../../characters/v0.1/gadget.md) porta ancora numeri della scala ÷10 (Arc Pulse 22, Linear Discharge 24 + 8, Overload 18, nella sezione «Abilità») e, nella tabella «Visione e stealth», una vista 6, contro i 220, 240 + 80 e 180 del catalogo eroi e del codice e la vista 7 di D-073 [codice: `RTHeroCatalogLibrary.cpp:393`, `:428-430`, `:472-474`; `GadgetWetDischargeBonus`, `RTCombatLibrary.h:521`]. Per questa pagina valgono il catalogo eroi e il codice.

**Domande che nascono qui.**

- [DOMANDA → TG-1] **RES Energetica.** La «RES Energetica» del catalogo, una sola, è il default per tutti e nove gli elementi, come oggi [DECISO D-595 (3)], o sostituisce la RES per elemento di D-589 (1)? Per il GDD la RES unica era già registrata come superata [DECISO D-591 (1), punto (f)]; il catalogo la ripropone, e D-633 non l'ha misurata (#4170, punto 4). Tocca anche `TR-35` di Riktor («RES per elemento di Riktor?»), ancora aperta: la risposta sul modello vale per entrambi.
- [DOMANDA → TG-2] **Movimento.** 3 MP del catalogo o 5 di oggi? Il catalogo dà al roster da 2 a 4 [CATALOGO §2], il codice ai quattro eroi di oggi da 4 a 6 [codice: `MovePoints`, `RTHeroCatalogLibrary.cpp:381`, `:545`, `:783`, `:1011`]: la scala è del roster, e con 3 Gadget sarebbe il più lento dei quattro a runtime. I MP sono la distanza dentro un profilo e camminare costa AP [DECISO D-584 (2)]; muoversi si ripete finché il budget lo consente [DECISO D-614].
- [DOMANDA → TG-3] **Vista e spinta.** Il catalogo non dà né la vista né la resistenza alla spinta: restano 7 e 0? Il 7 viene da D-073, che lo sceglie perché «con questo nessun eroe domina più nessun altro», fra gli eroi di allora, Vektor, Riva e Flux [DECISO D-073]; nel catalogo Murdock ha gittata 8 [CATALOGO §2], oltre il 7 di Gadget, che oggi è l'unico a vedere oltre il raggio 6 [codice: `RTHeroCatalogLibrary.cpp:392-393`].

## Che cosa esce

Escono dal kit le quattro voci di oggi che il catalogo non ha: `ArcPulse`, `ConductiveNode`, `Overload` e `ReactiveCapacitor` [SESSIONE 2026-11-23, «Escono tutte e quattro (Recommended)»]. Escono dalla proposta, non dal gioco: in vigore restano le voci di oggi fino alla `D-nnn`, e `MakeGadget` esce con lei [LETTURA D-633 (11i)]. `LinearDischarge` resta [DECISO D-633].

| Voce | Oggi | Che cosa lascia |
|---|---|---|
| `Hero.Gadget.ArcPulse` | attacco base all'indice 0: 220 danni, portata 4, cooldown 0, dalla tabella a fasce condivisa [codice: `MakeBasicAttack(4)`, `RTHeroCatalogLibrary.cpp:419-421`; fascia «medio», `RTCatalogLibrary.cpp:1949-1958`] | il posto dell'attacco base va alle voci del gruppo 1 del catalogo, e chi lo occupi nella dock è la domanda `KIT-2` [DECISO D-633 (9)]. Con lui esce l'unico attacco base d'eroe costruito sulla tabella a fasce [codice: `git grep -n "MakeBasicAttack(" -- Source/RefactorTactics/Ability/Catalog/RTHeroCatalogLibrary.cpp` → una riga, `:419`, su `f012ed660`]. ⚠️ `CONTRACT CONFLICT`: il commento del test `RefactorTactics.Heroes.BasicAttackByRangeBand` dice che per ogni eroe futuro «l'attacco base (indice 0) si costruisce con `MakeBasicAttack`, mai con un danno scelto a mano» [codice: `RTHeroCatalogTests.cpp:116-118`], mentre Spark Bolt fa 170 a portata 6 [CATALOGO] contro i 200 della fascia «lungo» [codice: `RTCatalogLibrary.cpp:1949-1958`]; già oggi Muiren, Riktor e Ivrin hanno un danno dell'eroe, 160, 80 e 210 [codice: `RTHeroCatalogLibrary.cpp:571-574`, `:842-845`, `:1038-1040`]. Il test usa un eroe di prova e resta verde [codice: `RTHeroCatalogTests.cpp:111-129`]; la regola del commento no. L'attacco base di Gadget prende i numeri del catalogo o quelli della fascia, e il commento si corregge? [DOMANDA → TG-10] |
| `Hero.Gadget.ConductiveNode` | **è** `Action.Electrify`: scarica sul grafo conduttivo, portata 4, propagazione 3, fase Environment, cooldown 2 dell'eroe [DECISO D-046, D-064; codice: `RTHeroCatalogLibrary.cpp:449-461`] | `Action.Electrify` resta un'azione del core senza un eroe che la usi [SESSIONE 2026-11-23, testo dell'opzione scelta] [codice: `RTCatalogLibrary.cpp:1831`; il kit che la porta oggi è questo, `git grep -n "Action.Electrify" -- Source/RefactorTactics/Ability/Catalog/RTHeroCatalogLibrary.cpp`]. D-046 aveva dato un owner nel roster alle azioni ambientali [DECISO D-046]: la `D-nnn` dovrà dire che per `Action.Electrify` quell'owner cade [PROPOSTA: la scelta dell'autore lo implica, e D-046 non lo dice]. Dell'acqua resta solo il bonus di `LinearDischarge` contro `Wet` [SESSIONE 2026-11-23] |
| `Hero.Gadget.Overload` | area di raggio 1, 180 danni, portata 3, cooldown 3, priorità 65, ripiego `AttackCell` [codice: `RTHeroCatalogLibrary.cpp:472-474`]; l'«Interrupt sui dispositivi» non è rappresentato, perché manca un modello di dispositivo interrompibile [codice: `RTHeroCatalogLibrary.cpp:463-466`; nessun tipo «dispositivo» nel modulo di gioco, `git grep -n -E -e "class [A-Za-z_]*Device" -e "struct [A-Za-z_]*Device" -- Source/RefactorTactics` → exit 1 su `f012ed660`, mentre lo stesso pattern con `CellId` trova `struct FRTCellId`] | il dispositivo interrompibile perde la voce che lo chiedeva: nessuna voce di Gadget nel catalogo interrompe un dispositivo, ed EMP Disruption silenzia le reazioni [CATALOGO] |
| `Hero.Gadget.ReactiveCapacitor` | reazione costruita su `Action.Counter`: scudo 150 a Gadget e 100 danni all'attaccante, cooldown 3 [codice: `RTHeroCatalogLibrary.cpp:483-486`] | la reazione Counter di Gadget diventa Feedback Loop [CSV GAD_RCT_02, `Reaction_Archetype` = `Counter`]. Lo scudo che risponde a un colpo già partito resta nel modulo `Reaction.ReactiveShield`, il modulo di reazione di default di Gadget [codice: `DefaultReactionModuleFor`, `RTCatalogLibrary.cpp:836-841`]; l'equipaggiamento di default va fra le domande [PROPOSTA: design §6] |

**Scenari e test che le citano.** Si contano col comando; su `f012ed660` nessuno dei quattro risponde 0.

```
git grep -l 'Hero.Gadget.ArcPulse' -- Scenarios | wc -l
git grep -l 'Hero.Gadget.ConductiveNode' -- Scenarios | wc -l
git grep -l 'Hero.Gadget.Overload' -- Scenarios | wc -l
git grep -l 'Hero.Gadget.ReactiveCapacitor' -- Scenarios | wc -l
```

Gli stessi comandi con `-- Source/RefactorTactics/Tests` contano i file di test, anche loro diversi da 0. Gli scenari che usano Gadget come eroe sono un insieme più largo, e cambiano se cambiano le sue statistiche: `git grep -l '"hero": *"Hero.Gadget"' -- Scenarios | wc -l`. Nessuno scenario nomina un'abilità per indice: `git grep -n -E '"[A-Za-z]*[Ii]ndex"' -- Scenarios` e `git grep -n -E '"ability": *[0-9]' -- Scenarios` → exit 1 su `f012ed660`.

### Che cosa resta da decidere

- [DOMANDA → TG-4] **`Profile.Grounding`.** È il profilo di reazione che il `Brace` di Gadget arma [DECISO D-132 (1); codice: `RTHeroCatalogLibrary.cpp:413-415`], con la risposta `GROUND` dichiarata e senza effetti, perché «Charge del `Grounding`» è ancora aperta [codice: `RTCatalogLibrary.cpp:25-32`]: oggi il resolver non la offre [codice: `BraceExecutableResponses`, `RTTurnManager_Blast.cpp:3658-3663`]. Nessuna voce del catalogo lo nomina [CATALOGO]. Con il kit nuovo il profilo resta, cambia (con una risposta Flux, per esempio) o esce, e Gadget resta senza finestra sul `Brace` come Riktor oggi [codice: `RTCatalogLibrary.h:142-148`]?
- [DOMANDA → TG-5] **Affinità e debolezza.** `Affinity.Electricity` e `Affinity.Water` [codice: `RTHeroCatalogLibrary.cpp:409`, `:412`] sono un vocabolario diverso dagli `Element.*`. Il validatore rifiuta un eroe senza l'una o l'altra [codice: `RTHeroCatalogLibrary.cpp:325-334`]; le leggono il profilo dell'eroe nella HUD e il banco [codice: `RTHeroProfileLibrary.cpp:18-19`, `RTHeroLab.cpp:22-23`], e nessuna regola di combattimento [codice: `git grep -n -w Affinity -- Source/RefactorTactics/Combat Source/RefactorTactics/Turn` → exit 1, e lo stesso con `-w Weakness`, su `f012ed660`]. Il catalogo dà tre affinità fra i nove elementi, Flux primaria, e nessuna debolezza [CATALOGO §3]. L'affinità diventa l'elemento primario del catalogo, `Element.Warp` [LETTURA D-633 (11b) per l'identificatore], una lista di elementi, o resta il campo di oggi? E la debolezza acqua, che il catalogo non dà?
- Restano, e si trattano nelle loro schede e fra le domande: `LinearDischarge` con le varianti `Concentrated` e `Branched` [codice: `RTHeroCatalogLibrary.cpp:503-521`], e l'equipaggiamento di default, `Gadget.Insulator` e `Reaction.ReactiveShield` [codice: `RTCatalogLibrary.cpp:823`, `:841`; PROPOSTA: domande, design §6].

---

## 1. Attacchi base

Gruppo del catalogo: «Attacco Base (🔴 Blast Phase | 2 AP | 0 CD)» [CATALOGO].

**Bersaglio degli esempi** (gruppi 1–3). Dove l'esempio non dice altro, il nemico R ha HP 1750, DEF 45 e RES 35, la riga di Riktor nel §2 del catalogo [CATALOGO §2], ed è colpito di fronte, con la convenzione della testa. La RES è quella unica del catalogo; se sia per elemento è `TG-1`. Ogni colpo energetico si conta due volte: con la RES, lettura elementale, e con la DEF, lettura diretta [DOMANDA → TG-7].

**Scelte comuni.** Fase Blast [CATALOGO «Blast Phase»; DECISO D-583 (1)], segmento `Attack` (40), priorità 50 e ripiego `Cancel`, cioè i dati di `Action.BasicAttack` [PROPOSTA; codice: `RTCatalogLibrary.cpp:1276-1279`]. Si costruiscono con `MakeHeroBasicAttack`, che copia dal core la mira al buio con traiettoria diretta, `BlindAimDirect` [DECISO D-490; codice: `RTHeroCatalogLibrary.cpp:167-179`, `RTCatalogLibrary.cpp:1295`]. 2 AP [CATALOGO; coincide con la classe Blast, DECISO D-594 (1)]. Cooldown 0 [CATALOGO], quindi ognuno si ripete nel piano finché il budget lo consente [DECISO D-614]: con 6 AP fanno tre colpi, anche di tre attacchi base diversi, se la dock li offre tutti; la dock è `KIT-1` e `KIT-2`, aperte [PROPOSTA: conto; DECISO D-633 (9)]. Portata 6 per tutti e tre [CATALOGO per Spark Bolt; CSV GAD_BAS_02 e CSV GAD_BAS_03 per gli altri due]. Il danno è scelto a mano, 170, 130 e 120, contro i 200 della fascia «lungo» [codice: `RTCatalogLibrary.cpp:1949-1958`]: è il conflitto della tabella a fasce [DOMANDA → TG-10].

**Un solo Blast.** Il danno del Blast è simultaneo, anche fra due azioni della stessa unità [DECISO D-615 (3)]: un effetto di un colpo, come la rotazione di Phase Dart, non cambia gli altri colpi dello stesso Blast, a meno di una regola nuova (`TG-12`).

### Spark Bolt — `Hero.Gadget.SparkBolt`

> «Opzione 1 (Core) — Spark Bolt [⚡ Fulmine]: Dardo energetico a lungo raggio (6 esagoni) da 170 Danno Energetico; agisce da catalizzatore a distanza su esagoni già caricati.»

Nel `.csv` (`GAD_BAS_01`): `Range_Hex` 6, `Base_Damage_or_Shield` 170, `Status_or_Effect` «Remote Catalyst», `Ignores_Armor` e `Modifies_Walls` «No».

| Campo | Valore |
|---|---|
| Ruolo | attacco base, «Opzione 1 (Core)» [CATALOGO], una voce del kit come le altre due [DECISO D-633 (9), «Tutte nel kit, come Riktor»]. Occupante di default del posto base della dock [PROPOSTA: è l'opzione «Core» e il danno base per AP più alto del gruppo, 85 contro 65 e 60]; chi lo occupi resta `KIT-2` [DECISO D-633 (9)] |
| Fase | Blast · `Attack` (40) [CATALOGO «Blast Phase»; PROPOSTA: come `Action.BasicAttack`, codice: `RTCatalogLibrary.cpp:1276`] |
| Costo AP | 2 [CATALOGO «2 AP»; classe Blast, DECISO D-594 (1)] |
| Cooldown | 0 [CATALOGO «0 CD»]; si ripete finché il budget lo consente [DECISO D-614] |
| Priorità | 50 [PROPOSTA: quella di `Action.BasicAttack` e degli attacchi base d'eroe di oggi, codice: `RTCatalogLibrary.cpp:1276`, `RTHeroCatalogLibrary.cpp:571`, `:842`, `:1038`]. A pari priorità decide l'id dell'azione, mai la dichiarazione [DECISO D-615 (5)] |
| Portata e linea di vista | 6 [CATALOGO «a lungo raggio (6 esagoni)»], nessuna portata minima [PROPOSTA: il catalogo non ne dà]. Mira al buio con traiettoria diretta: si dichiara anche su una cella che non si vede, e il primo ostacolo sul percorso ferma il colpo [DECISO D-490; codice: `BlindAimDirect`, `RTCatalogLibrary.cpp:1295`]. La mira non insegue il bersaglio [DECISO D-415 (3)] |
| Forma | `Single`: una cella [CATALOGO «Dardo»; PROPOSTA] |
| Bersaglio | un'unità nemica, o la cella dichiarata al buio [PROPOSTA; DECISO D-490] |
| Elemento | `Element.Electric` [CATALOGO «⚡ Fulmine»; DECISO D-593 (1)], che resta fra i nove [LETTURA D-633 (11b)] |
| Tipo di danno | elementale Electric, mitigato dalla RES [PROPOSTA: «Danno Energetico»; DOMANDA → TG-7] |
| Danno base | 170 [CATALOGO]; la fascia «lungo» darebbe 200 [codice: `RTCatalogLibrary.cpp:1949-1958`; DOMANDA → TG-10]. Il colpo vale `max(0, (Base + Spalle) × Mod × 100/(100+RES) − Copertura − Guard)` [DECISO D-589 (1) per la RES, DECISO D-589 (3) per la formula], con le spalle di Gadget a 60 [DECISO D-599; codice: `RTHeroCatalogLibrary.cpp:403`] |
| Movimento | nessuno [CATALOGO] |
| Stati | nessuno [CATALOGO] |
| Oggetti persistenti | nessuno [CATALOGO] |
| Catalizzatore | «agisce da catalizzatore a distanza su esagoni già caricati» [CATALOGO]. [PROPOSTA] il colpo porta Fulmine sulla cella colpita; se la cella ha già la carica di un altro elemento, l'esito è quello della riga della coppia nel [catalogo delle coppie](../../balance/RT_ElementPairCatalog_v0.1.md), e c'è solo se la riga ne ha uno, perché crea un comportamento «solo un `ReactionId` stabile con dati di effetto espliciti» [DECISO D-275 (2)]. Spark Bolt non ha un effetto suo: «status/surface/event/geometry appartengono al relativo sistema» [DECISO D-029] [DOMANDA → TG-11] |
| Fallback | `Cancel` [PROPOSTA: il ripiego di `Action.BasicAttack`, codice: `RTCatalogLibrary.cpp:1277`] |
| Sistemi che mancano | le cariche di cella [codice: `git grep -n -i -e ElementCharge -e ElementalCharge -e CellCharge -e ChargeTier -e Status.Charged -- Source` → exit 1 su `f012ed660`; controllo, lo stesso stile di pattern su uno stato che c'è: `git grep -n -e Status.Electrified -- Source/RefactorTactics/Core` → exit 0]; il campo elemento dell'azione e il percorso di danno elementale [codice: vedi «Come leggere la traduzione»] |

**Esempio.** *Given* Gadget in (0,0,0), budget 6; R a distanza 6. *When* Gadget pianifica Spark Bolt su R (2 AP, ne restano 4) e il Blast risolve. *Then* lettura elementale: `floor(170 × 100/135)` = 125, R 1750 → 1625; lettura diretta: `floor(170 × 100/145)` = 117, R → 1633. Tre Spark Bolt nello stesso piano (6 AP) sono tre colpi da 125, ciascuno arrotondato per conto suo [DECISO D-595 (1)]: R → 1375. Con R dietro una copertura bassa sul lato da cui arriva il colpo: 125 − 60 = 65 [DECISO D-633 (7)]. Alle spalle di R: `floor((170 + 60) × 100/135)` = 170. Con R a distanza 7: rifiutato per portata. Il catalizzatore non ha esempio: dipende da `TG-11` e da un sistema che manca.

**Domande che nascono qui.**

- [DOMANDA → TG-11] **Catalizzatore di Spark Bolt.** «Agisce da catalizzatore a distanza su esagoni già caricati»: il colpo porta Fulmine sulla cella e l'esito è della riga della coppia (la proposta), o Spark Bolt ha un effetto suo? E su una cella d'acqua o su un bersaglio `Wet`, l'unica coppia con Fulmine che oggi ha un `ReactionId`, `Reaction.ElectricPropagatesThroughWater` ([catalogo delle coppie](../../balance/RT_ElementPairCatalog_v0.1.md), riga 69), si attiva? È la domanda di Riktor `TR-8` per Shock Whip («Un colpo Electric su un bersaglio Wet o in acqua attiva anche la reazione della coppia, o la catena la sostituisce?»): una risposta vale per entrambi.

**Domande della scheda.** `TG-7` (tipo di danno), `TG-10` (fascia dell'attacco base), `TG-11` (catalizzatore); `TG-1` per la RES degli esempi.

### Phase Dart — `Hero.Gadget.PhaseDart`

> «Opzione 2 (Controllo Facing) — Phase Dart [🌌 Flux]: Proiettile di distorsione da 130 Danno Energetico che ruota forzatamente il Facing del bersaglio di 60° (2 spicchi), esponendolo al Flanking.»

Nel `.csv` (`GAD_BAS_02`): `Range_Hex` 6, `Status_or_Effect` «Force Rotate 60°».

| Campo | Valore |
|---|---|
| Ruolo | attacco base, «Opzione 2 (Controllo Facing)» [CATALOGO]; voce del kit [DECISO D-633 (9)] |
| Fase | Blast · `Attack` (40) [CATALOGO «Blast Phase»; PROPOSTA: come gli altri attacchi base] |
| Costo AP | 2 [CATALOGO; classe Blast, DECISO D-594 (1)] |
| Cooldown | 0 [CATALOGO]; si ripete [DECISO D-614]: due Phase Dart sullo stesso bersaglio fanno due passi, 120° [PROPOSTA: le rotazioni si sommano; DOMANDA → TG-12] |
| Priorità | 50 [PROPOSTA]; il danno del Blast è simultaneo, e la priorità ne ordina solo il racconto [DECISO D-615 (3)] |
| Portata e linea di vista | 6 [CSV GAD_BAS_02; il testo non la dà, CATALOGO]; `BlindAimDirect` [DECISO D-490; codice: `RTCatalogLibrary.cpp:1295`] |
| Forma | `Single`: una cella [CATALOGO «Proiettile»; PROPOSTA] |
| Bersaglio | un'unità nemica [PROPOSTA]; la rotazione vale solo se il colpo arriva, e un colpo fermato da un ostacolo non arriva [PROPOSTA; DECISO D-490] |
| Elemento | `Element.Warp` [CATALOGO «🌌 Flux»; DECISO D-593 (5) per l'identificatore; LETTURA D-633 (11b), Flux è Warp] |
| Tipo di danno | elementale Warp [PROPOSTA: «Danno Energetico»; DOMANDA → TG-7] |
| Danno base | 130 [CATALOGO]; la fascia «lungo» darebbe 200 [codice: `RTCatalogLibrary.cpp:1949-1958`; DOMANDA → TG-10] |
| Movimento | nessuno [CATALOGO] |
| Stati | nessuno stato: la rotazione cambia il facing, che è un campo dell'unità [codice: `ARTUnit::Facing`, `RTUnit.h:185`; PROPOSTA] |
| Rotazione | «di 60° (2 spicchi)»: qui lo spicchio è di 30°, e 60° sono un passo fra le sei direzioni della cella [LETTURA D-633 (11c); codice: `ERTHexDirection`, `RTCellId.h:11-19`]. Il «Flanking» che espone è il bonus alle spalle di oggi [DECISO D-633 (2)]. Il verso, a sinistra o a destra, lo dichiara Gadget nel planning [PROPOSTA: con un verso fisso la voce esporrebbe sempre lo stesso fianco]. Vale dopo i colpi del Blast, che leggono il facing di prima [PROPOSTA; DECISO D-615 (3)]. Il facing del bersaglio cambia ancora alla fine del suo movimento e poi con la rotazione che ha dichiarato, l'ultimo passo del Move [codice: `RTTurnManager_Movement.cpp:1935`, `:2000-2036`], e nel Blast chi attacca si gira verso il proprio bersaglio prima dei colpi [codice: `RTTurnManager_Blast.cpp:1390-1410`]: così letta, la rotazione porta a un colpo alle spalle solo se il bersaglio, nel turno N, non si muove e non dichiara un facing, e nel Blast di N+1 non attacca [PROPOSTA: conto di questa pagina; DOMANDA → TG-12] |
| Oggetti persistenti | nessuno [CATALOGO] |
| Fallback | `Cancel` [PROPOSTA: come `Action.BasicAttack`, codice: `RTCatalogLibrary.cpp:1277`] |
| Sistemi che mancano | una rotazione imposta da un'azione [codice: `git grep -n -i -e ForceFacing -e ForcedFacing -e RotateFacing -e ForcedRotation -e ForceRotat -- Source` → exit 1 su `f012ed660`; `ERTActionEffect` non ha un valore di rotazione, `RTActionEvent.h:17-162`]. L'unico facing che oggi un'altra unità impone è quello di chi subisce uno spostamento forzato, girato verso la sorgente [codice: `ERTFacingOutcome::TurnedToDisplacementSource`, `RTTurnLog.h:174`, scritto in `RTTurnManager.cpp:2396`; controllo: `git grep -n -e TurnedToDisplacementSource -- Source/RefactorTactics ':!Source/RefactorTactics/Tests'` → exit 0]; il TurnLog vorrà un motivo nuovo in coda a `ERTFacingOutcome` [PROPOSTA]. Il campo elemento e il percorso elementale [codice: vedi «Come leggere la traduzione»] |

**Esempio.** *Given* Gadget in (3,−4,0), a distanza 4 da R sulla direzione NW di R; R in (3,0,0) con facing W: il colpo arriva dalla direzione NW, che sta nell'arco frontale di R, il cono di 120° verso il facing che decide il bonus alle spalle [codice: `URTHexCombatLibrary::IsInFrontalArc`, `RTHexCombatLibrary.cpp:213`, letto in `MitigatedHitPower`, `:337`]. *When* nel turno N Gadget pianifica Phase Dart su R con il verso «a sinistra», W → SW [PROPOSTA]. *Then* nel Blast di N R prende il colpo di fronte, `floor(130 × 100/135)` = 96 (lettura elementale) o `floor(130 × 100/145)` = 89 (diretta), perché i colpi dello stesso Blast leggono il facing di prima [DECISO D-615 (3)]; poi R passa a SW, e la direzione NW esce dal suo arco frontale. Se R, nel turno N, resta fermo e non dichiara un facing, e nel Blast di N+1 non attacca, uno Spark Bolt di Gadget dallo stesso punto nel turno N+1 è alle spalle: `floor((170 + 60) × 100/135)` = 170 invece di 125. Se R si muove o dichiara un facing, la rotazione si perde alla fine del Move di N.

**Domande che nascono qui.**

- [DOMANDA → TG-12] **Rotazione di Phase Dart.** Il verso lo sceglie Gadget (la proposta) o è fisso? Vale dopo i colpi del Blast, come vuole il danno simultaneo, o prima, con una regola nuova del resolver? Questa parte è il nodo dell'ultima domanda di Riktor `TR-25` (lo Stun di Skewer prima degli attacchi dello stesso Blast): una risposta sul principio vale per entrambi. Quanto dura: fino al prossimo facing derivato o dichiarato, che la rende quasi sempre inerte, o blocca il facing del bersaglio fino al turno dopo? Due rotazioni sullo stesso bersaglio si sommano?

**Domande della scheda.** `TG-7`, `TG-10`, `TG-12`.

### Static Arc — `Hero.Gadget.StaticArc`

> «Opzione 3 (Rimbalzo) — Static Arc [⚡ Fulmine]: Dardo da 120 Danno Energetico che si propaga automaticamente su 1 esagono adiacente se entrambi contengono già una carica elementale.»

Nel `.csv` (`GAD_BAS_03`): `Range_Hex` 6, `Status_or_Effect` «Bounce on Charged Hex».

| Campo | Valore |
|---|---|
| Ruolo | attacco base, «Opzione 3 (Rimbalzo)» [CATALOGO]; voce del kit [DECISO D-633 (9)] |
| Fase | Blast · `Attack` (40) [CATALOGO «Blast Phase»; PROPOSTA] |
| Costo AP | 2 [CATALOGO; classe Blast, DECISO D-594 (1)] |
| Cooldown | 0 [CATALOGO]; si ripete [DECISO D-614] |
| Priorità | 50 [PROPOSTA]; danno simultaneo [DECISO D-615 (3)] |
| Portata e linea di vista | 6 [CSV GAD_BAS_03; il testo non la dà, CATALOGO]; `BlindAimDirect` [DECISO D-490]. Il rimbalzo parte dal bersaglio e non chiede vista [PROPOSTA: va a una cella adiacente al bersaglio, non a una cella mirata da Gadget] |
| Forma | `Single` sul bersaglio, una cella; con la condizione vera, un secondo colpo su una delle sei celle adiacenti, un salto solo [CATALOGO «su 1 esagono adiacente»; PROPOSTA]. Con più adiacenti che la soddisfano, quella col `FRTCellId` minore, per X, poi Y, poi `Layer` [PROPOSTA: un ordine esplicito e stabile, come chiede il determinismo; DOMANDA → TG-14] |
| Bersaglio | un'unità nemica; il secondo colpo prende l'unità della cella adiacente, se c'è, anche un alleato, col fuoco amico di default [PROPOSTA; codice: `bFriendlyFire = true`, `RTActionDef.h:810`; DOMANDA → TG-13] |
| Elemento | `Element.Electric` [CATALOGO «⚡ Fulmine»; DECISO D-593 (1); LETTURA D-633 (11b)] |
| Tipo di danno | elementale Electric [PROPOSTA; DOMANDA → TG-7] |
| Danno base | 120 al bersaglio [CATALOGO]; 120 al secondo [PROPOSTA: il catalogo non dichiara una riduzione; DOMANDA → TG-14]. La fascia «lungo» darebbe 200 [codice: `RTCatalogLibrary.cpp:1949-1958`; DOMANDA → TG-10] |
| Condizione | la cella del bersaglio e la cella adiacente portano entrambe «già una carica elementale» [CATALOGO], lette sullo snapshot del colpo [PROPOSTA]. Le cariche non esistono: oggi la condizione non è mai vera, e la voce è un colpo singolo da 120 [codice: vedi Spark Bolt, «Sistemi che mancano»] |
| Movimento | nessuno [CATALOGO] |
| Stati | nessuno [CATALOGO] |
| Oggetti persistenti | nessuno [CATALOGO] |
| Fallback | `Cancel` [PROPOSTA: come `Action.BasicAttack`] |
| Sistemi che mancano | le cariche di cella, come per Spark Bolt [codice: lo stesso comando, exit 1 su `f012ed660`]; un secondo bersaglio scelto dallo stato della mappa: per la variante `Branched` di `LinearDischarge` «QUALE secondo nemico venga colpito e' targeting non ancora cablato» [codice: `RTHeroCatalogLibrary.cpp:509-512`], e un rimbalzo non c'è [codice: `git grep -n -i -w -e bounce -e bounces -e ricochet -- Source` → exit 1 su `f012ed660`; controllo dello stile: `git grep -n -i -w -e chain -- Source/RefactorTactics ':!Source/RefactorTactics/Tests'` → exit 0]; il campo elemento e il percorso elementale [codice: vedi «Come leggere la traduzione»] |

**Esempio.** *Given* R in (3,0,0) e un secondo nemico S, con gli stessi numeri, in (4,0,0); la cella di R e quella di S portano già una carica elementale, che è il sistema che manca. *When* Gadget pianifica Static Arc su R. *Then* R prende `floor(120 × 100/135)` = 88 (lettura elementale; 82 con la DEF), R → 1662; il rimbalzo colpisce S con altri 88 [PROPOSTA: danno pieno]. Con le cariche che oggi non esistono, Static Arc è un colpo singolo da 120: R → 1662, S intatto.

**Domande che nascono qui.**

- [DOMANDA → TG-13] **Fuoco amico.** Il rimbalzo di Static Arc e l'area del Cryo-Drone toccano anche gli alleati, e Gadget? È la domanda di Riktor `TR-12` («Gli alleati nell'area sono colpiti?»): una risposta vale per entrambi.
- [DOMANDA → TG-14] **Rimbalzo di Static Arc.** Con più celle adiacenti cariche, quale riceve il rimbalzo (la proposta: il `FRTCellId` minore)? Quale carica conta, una qualunque o solo una che con Fulmine fa una coppia? Il secondo colpo fa 120 pieni o meno? Per il danno è la domanda di Riktor `TR-7` sulla catena di Shock Whip («Il secondario fa 160 pieni o un danno ridotto?»): una risposta vale per entrambi.

**Domande della scheda.** `TG-7`, `TG-10`, `TG-13`, `TG-14`.

---

## 2. Prep

Gruppo del catalogo: «Prep Phase (🔵 Prep Phase | 1 AP | CD 1)» [CATALOGO].

**Scelte comuni.** Fase `Preparation` (10), la macro-fase Prep [CATALOGO «Prep Phase»; DECISO D-583 (1); codice: `RTActionDef.h:25-26`]. 1 AP [CATALOGO; coincide con la classe Prep, DECISO D-594 (1)]. Cooldown 1 [CATALOGO «CD 1»], letto come in «Come leggere la traduzione»: una volta per piano, ma ancora in ogni turno [PROPOSTA; DOMANDA → TG-6]. Le tre voci cambiano la mappa; lo stato di mappa che ne nasce è pubblico dalla risoluzione del Prep, come una copertura eretta, e l'intento resta privato fino ad allora [PROPOSTA; AGENTS.md §4].

**Lati e bordi.** Il catalogo dice «lato» e «segmento di muro». Un lato dell'esagono è un bordo fra due celle, non una cella: in codice si indirizza dalla cella con una delle sei direzioni, come le coperture e le porte [codice: `FRTHexCover::Edge`, `RTHexCellData.h:58-64`; `FRTHexDoor`, `:252-258`], e un passo lo attraversa quando va da una cella a quella adiacente sul suo lato [DECISO D-524 (5)]. Un bordo non ha occupancy e conta 0 celle. Speed Gate e Holo-Barricade si traducono su un bordo, Structure Shift anche, con l'alternativa della cella nella sua scheda [PROPOSTA].

**Priorità** [PROPOSTA]: Speed Gate 41, perché si arma soltanto e il suo effetto si decide nel Dash; Structure Shift 74 e Holo-Barricade 75, accanto a `Action.CreateCover` (75) [codice: `RTCatalogLibrary.cpp:1921-1922`], perché le strutture di bordo del Prep si raccolgono e si applicano a fase conclusa [codice: `RTTurnManager.h:2736-2737`], e la priorità decide solo fra due operazioni sullo stesso bordo, dove vince quella raccolta per prima [codice: `RTTurnManager.h:2745`]. Le priorità di Prep di oggi sono 30 (`Action.SuppressiveLine`, `Action.Brace`, `Hero.Ivrin.InterceptShot`), 35 (`Action.Shield`), 36 (`Hero.Muiren.FlowReaction`), 40 (`Action.Guard`), 45 (`Action.Overwatch`), 75 (`Action.CreateCover`) e 76 (`Hero.Riktor.Reconfigure`) [codice: `RTCatalogLibrary.cpp:1309`, `:1405`, `:1578`, `:1721`, `:1732`, `:1921-1922`; `RTHeroCatalogLibrary.cpp:736`, `:881-882`, `:1054-1055`]: 41 e 74 sono libere. Holo-Barricade a 75 pareggia con `Action.CreateCover`, che un equipaggiamento può dare a ogni eroe [codice: `RTCatalogLibrary.cpp:376`], e il pareggio si scioglie per id [DECISO D-615 (5)].

**Oggetti persistenti.** Il varco, il bordo Wireframe e la parete sono stato di partita con una scadenza, come `DynamicSurfaces`, `DynamicArcs` e `DynamicCovers` [codice: `RTTurnManager.h:2664-2733`]. ⚠️ L'hash di fine partita copre il valore corrente di terreni e strutture, non la loro scadenza [codice: `RTMatchStateHash.h:121-126`]: un varco che non sta nella mappa resterebbe del tutto fuori dall'hash, e va aggiunto con lo snapshot e il replay [PROPOSTA; CLAUDE.md §7].

### Structure Shift — `Hero.Gadget.StructureShift`

> «Opzione 1 (Core) — Structure Shift [🌌 Flux]: Smaterializza un segmento di muro solido per 1 turno trasformandolo in Wireframe (trasparente a LoS e proiettili, blocco fisico attivo) o innescando la reazione Flux del muro.»

Nel `.csv` (`GAD_PRP_01`): `Range_Hex` «3-4 (Wall)», `Base_Damage_or_Shield` 0, `Modifies_Walls` «Yes», `Status_or_Effect` «Wireframe Wall 1T».

| Campo | Valore |
|---|---|
| Ruolo | Prep, «Opzione 1 (Core)» [CATALOGO]; voce del kit [DECISO D-633 (9)]. È la «riscrittura della mappa via Flux» che il catalogo mette nel Prep [CATALOGO §1.1] |
| Fase | `Preparation` (10) [CATALOGO; DECISO D-583 (1)], applicata a fase conclusa con le strutture di bordo [PROPOSTA; codice: `RTTurnManager.h:2736-2737`] |
| Costo AP | 1 [CATALOGO; classe Prep, DECISO D-594 (1)] |
| Cooldown | 1 [CATALOGO «CD 1»; PROPOSTA, la semantica di «Come leggere la traduzione»; DOMANDA → TG-6] |
| Priorità | 74 [PROPOSTA: vedi «Priorità» del gruppo] |
| Portata e linea di vista | 4 [CSV GAD_PRP_01 «3-4 (Wall)»; PROPOSTA: la cifra alta della forbice; DOMANDA → TG-15]; vista sulla cella da cui si indirizza il bordo, con la policy di default [PROPOSTA; codice: `Required`, `RTActionDef.h:212-218`] |
| Forma | un bordo, 0 celle [PROPOSTA]: il «segmento di muro» è il lato di una cella con copertura alta, `ERTHexCoverType::High`, che «NEGA l'attraversamento del bordo a vista, passo e proiettili» [codice: `RTHexCellData.h:46`]. L'altra lettura di «muro» è una cella che blocca passo e vista [codice: `bBlocksMovement`, `bBlocksLineOfSight`, `RTHexCellData.h:400`, `:423`; DOMANDA → TG-16] |
| Bersaglio | un bordo con copertura alta entro la portata, di chiunque [PROPOSTA: un bordo non ha squadra, codice: `git grep -n -w TeamId -- Source/RefactorTactics/Map/RTHexCellData.h` → una sola riga, un commento, `:435`] |
| Elemento | `Element.Warp` [CATALOGO «🌌 Flux»; DECISO D-593 (5); LETTURA D-633 (11b)] |
| Tipo di danno | nessuno [CATALOGO] |
| Danno base | 0 [CSV GAD_PRP_01] |
| Movimento | nessuno [CATALOGO] |
| Stati | nessuno [CATALOGO] |
| Oggetti persistenti | il bordo Wireframe, che nega il passo e lascia passare vista e proiettili [CATALOGO «trasparente a LoS e proiettili, blocco fisico attivo»], per il turno del lancio, dal Prep al Cleanup [PROPOSTA: «per 1 turno», col conto alla rovescia del Cleanup delle coperture erette nel Prep, codice: `RTTurnManager.h:2707-2712`]; poi il bordo torna com'era [PROPOSTA: il tipo d'origine si conserva come l'`Original` di una superficie dinamica, codice: `RTTurnManager.h:2672-2677`]. L'integrità del muro non cambia [PROPOSTA] |
| Reazione Flux del muro | «o innescando la reazione Flux del muro» [CATALOGO]: non si traduce. Le righe di `Element.Warp` del [catalogo delle coppie](../../balance/RT_ElementPairCatalog_v0.1.md) sono tutte `OPEN` (righe 47–57 su `f012ed660`), e una voce `OPEN` «non deve ricevere effetti o numeri inventati» [DECISO D-275 (1)]; le combo Flux della matrice di Gadget sono candidate per quelle righe, non regole dell'eroe [LETTURA D-633 (11e)] [DOMANDA → TG-16] |
| Fallback | `Cancel` [PROPOSTA: come `Action.CreateCover`, codice: `RTCatalogLibrary.cpp:1921-1922`]: se il bordo non è più un muro quando l'operazione si applica, non succede niente |
| Sistemi che mancano | un bordo che neghi il passo e non la vista: i tipi sono `None`, `Low` e `High` [codice: `ERTHexCoverType`, `RTHexCellData.h:42-47`; `git grep -n -i wireframe -- Source` → exit 1 su `f012ed660`]; un'operazione che in partita cambi il tipo di un bordo e poi lo rimetta: `ERTStructureOp` erige, sposta, apre e chiude porte [codice: `RTActionDef.h:354-373`], e fuori dai test il solo sito che riscrive il tipo è la cottura della geometria [codice: `git grep -n -E -e "\.Type = ERTHexCoverType" -e "->Type = ERTHexCoverType" -- Source/RefactorTactics ':!Source/RefactorTactics/Tests'` → una riga, `Map/Structure/RTGeometryBake.cpp:470`]; la scadenza nell'hash [codice: `RTMatchStateHash.h:121-126`] |

**Esempio.** *Given* Gadget in (0,0,0); R in (3,0,0); una copertura alta sul bordo fra (1,0,0) e (2,0,0). *When* nel Prep di N Gadget usa Structure Shift su quel bordo (1 AP) e nel Blast di N Spark Bolt su R (2 AP). *Then* senza Structure Shift lo Spark Bolt si ferma sul muro [DECISO D-490] e R resta a 1750; con Structure Shift arriva, R → 1625 (lettura elementale). Se R, nel Move di N, prova a passare da (2,0,0) a (1,0,0), il bordo nega il passo come prima. Nel Cleanup di N il bordo torna copertura alta, e uno Spark Bolt nel turno N+1 si ferma di nuovo [PROPOSTA: «per 1 turno» è il turno del lancio].

**Domande che nascono qui.**

- [DOMANDA → TG-15] **Fonti che divergono, `.md` e `.csv`.** Nei gruppi 1–3 i due file non dicono la stessa cosa: Seeker Bot avanza «di 3 esagoni» nel testo e ha `Range_Hex` «3-6 (Line)»; il Cryo-Drone ha 80 danni solo nel `.csv`; il «al primo nemico incontrato» di Seeker Bot c'è solo nel `.md`. Quando divergono quale vale, e le forbici del `.csv` («3-4 (Wall)», «3-6 (Line)») sono una portata minima e una massima o due misure diverse?
- [DOMANDA → TG-16] **Structure Shift.** Il «segmento di muro solido» è un bordo con copertura alta (la proposta), una cella che blocca passo e vista, o entrambi? Il Wireframe dura il turno del lancio (la proposta) o anche il successivo? Che cos'è «la reazione Flux del muro», che oggi non ha una riga di coppia che la descriva?

**Domande della scheda.** `TG-6`, `TG-15`, `TG-16`.

### Speed Gate — `Hero.Gadget.SpeedGate`

> «Opzione 2 (Mobilità) — Speed Gate [💨 Aria]: Proietta un varco di accelerazione su un lato dell'esagono; gli alleati che lo attraversano in Dash Phase ottengono +2 esagoni di movimento per 2 turni.»

Nel `.csv` (`GAD_PRP_02`): `Range_Hex` «3 (Edge)», `Base_Damage_or_Shield` 0, `Modifies_Walls` «Yes», `Status_or_Effect` «+2 Move Hex Allies (2T)».

| Campo | Valore |
|---|---|
| Ruolo | Prep, «Opzione 2 (Mobilità)» [CATALOGO]; voce del kit [DECISO D-633 (9)] |
| Fase | `Preparation` (10) [CATALOGO; DECISO D-583 (1)]: il varco si arma nel Prep e agisce nel Dash dello stesso turno [PROPOSTA: il catalogo lo fa attraversare «in Dash Phase»] |
| Costo AP | 1 [CATALOGO; classe Prep, DECISO D-594 (1)] |
| Cooldown | 1 [CATALOGO «CD 1»; PROPOSTA; DOMANDA → TG-6] |
| Priorità | 41 [PROPOSTA: si arma soltanto; vedi «Priorità» del gruppo] |
| Portata e linea di vista | 3 [CSV GAD_PRP_02 «3 (Edge)»; il testo non la dà, CATALOGO]; vista sulla cella da cui si indirizza il bordo, con la policy di default [PROPOSTA; codice: `RTActionDef.h:212-218`] |
| Forma | un bordo, 0 celle [CATALOGO «su un lato dell'esagono»; PROPOSTA: il lato è un bordo fra due celle, come quello di una copertura, codice: `FRTHexCover::Edge`, `RTHexCellData.h:58-64`]. Il varco non nega niente: non è una copertura e non cambia il grafo [PROPOSTA] |
| Bersaglio | un bordo entro la portata [PROPOSTA]; agisce sugli alleati di Gadget, lei esclusa [CATALOGO «gli alleati»; PROPOSTA per Gadget; DOMANDA → TG-17] |
| Elemento | `Element.Air` [CATALOGO «💨 Aria»; DECISO D-593 (5); LETTURA D-633 (11b)] |
| Tipo di danno | nessuno [CATALOGO] |
| Danno base | 0 [CSV GAD_PRP_02] |
| Movimento | nessuno per Gadget [CATALOGO]. Un alleato attraversa il varco quando un passo del suo scatto va da una cella all'altra attraverso quel bordo [PROPOSTA; DECISO D-524 (5)]: negli scatti `LinearDash`, `LinearCharge` e `LinearPass`. Il salto `LinearLeap`, che «scavalca le celle intermedie e guarda solo dove atterra» [DECISO D-524 (5)], non lo attraversa [PROPOSTA; DOMANDA → TG-17] |
| Stati | +2 MP sul budget del Move [CATALOGO «+2 esagoni di movimento»; PROPOSTA: gli scatti non hanno un costo per cella da cambiare, codice: commento di `Action.Slow`, `RTCatalogLibrary.cpp:1786-1790`]. Per 2 turni, il Move di N+1 e quello di N+2, con la scadenza nel Cleanup di N+2 [CATALOGO «per 2 turni»; PROPOSTA: il Move di N è pianificato e validato prima del Dash, e un piano validato non si allunga a risoluzione, come per il budget di AP, DECISO D-613 (1); DOMANDA → TG-17]. Stato nuovo, nome di lavoro `Status.Hasted` [PROPOSTA], letto da `ARTUnit::GetEffectiveMoveRange`, il punto unico che alimenta lo snapshot del Move e la validazione del piano [codice: `RTUnit.cpp:1931-1941`] |
| Oggetti persistenti | il varco sul bordo, dal Prep al Cleanup del turno del lancio [PROPOSTA: il catalogo dà la durata del bonus, non quella del varco; DOMANDA → TG-17]; stato di partita con la sua scadenza, fuori dall'hash di oggi [codice: `RTMatchStateHash.h:121-126`] |
| Fallback | `Cancel` [PROPOSTA] |
| Sistemi che mancano | un fatto «attraversa un bordo» che fa scattare un effetto [PROPOSTA: AGENTS.md §5 elenca «attraversamento bordo» fra i fatti di un Traversal]: `SpatialTrigger` («tripwire che scatta attraversando un bordo») è fra le capacità dello Scenario Harness senza un produttore in partita [codice: `RTScenarioSession.cpp:503-511`, owner #704; `git grep -n SpatialTrigger -- Source/RefactorTactics ':!Source/RefactorTactics/Tests' ':!Source/RefactorTactics/ScenarioHarness'` → exit 1 su `f012ed660`, e lo stesso comando sullo Scenario Harness → exit 0]; uno stato che aumenti i MP [codice: `git grep -n -i -e "Status\.Haste" -e "Status\.Hasted" -e "Status\.Speed" -e "Status\.Accelerat" -- Source` → exit 1 su `f012ed660`; controllo: `git grep -n -e "Status\.Slow" -- Source/RefactorTactics/Core` → exit 0]; un oggetto di bordo che non sia copertura, porta o parapetto [codice: `Covers`, `Doors`, `Guards`, `RTHexCellData.h:447`, `:453`, `:460`] |

**Esempio.** *Given* un alleato B in (0,0,0), con 4 MP, che nel turno N pianifica `Action.Dodge` verso (3,0,0) e poi un Move di 4 celle, legale perché uno scatto e un Move nello stesso piano restano legali [LETTURA D-625 (e)]; Gadget, nel Prep di N, mette Speed Gate sul bordo fra (1,0,0) e (2,0,0). *When* il Dash di N risolve. *Then* B attraversa il varco e prende +2 MP [PROPOSTA]: il Move di N resta di 4 celle, quello pianificato, e nel planning di N+1 e di N+2 B cammina con 6 MP; nel Cleanup di N+2 il bonus scade. Un nemico che attraversa lo stesso bordo non prende niente [CATALOGO «gli alleati»]; B che lo attraversa nel Move invece che nel Dash non prende niente [CATALOGO «in Dash Phase»]; con `Action.Leap`, che non attraversa i bordi, nemmeno [PROPOSTA].

**Domande che nascono qui.**

- [DOMANDA → TG-17] **Speed Gate.** Conta solo l'attraversamento in uno scatto (la lettera) o anche nel Move, e il salto che scavalca il bordo? Una spinta o una trazione attraverso il varco valgono? Per queste è la domanda uniforme di Riktor `TR-18` («Una spinta o una trazione nel Dash attraverso le celle di Anchor e di Spikes le fa scattare?»): una risposta vale per entrambi. I +2 valgono sul Move (la proposta) o anche sulla portata degli scatti? I «2 turni» partono dal turno dopo, perché il Move del turno è già validato (la proposta), o il validatore deve contare il varco della propria squadra nello stesso turno? Gadget è fra gli «alleati»? Quanto dura il varco?

**Domande della scheda.** `TG-6`, `TG-17`.

### Holo-Barricade — `Hero.Gadget.HoloBarricade`

> «Opzione 3 (Copertura) — Holo-Barricade [🛡️ Scudo]: Materializza una parete olografica leggera (150 HP) su un lato vuoto bloccando la Linea di Vista nemica per 1 turno.»

Nel `.csv` (`GAD_PRP_03`): `Range_Hex` «3 (Edge)», `Base_Damage_or_Shield` 150, `Modifies_Walls` «Yes», `Status_or_Effect` «Holo Wall 150 HP (1T)».

| Campo | Valore |
|---|---|
| Ruolo | Prep, «Opzione 3 (Copertura)» [CATALOGO]; voce del kit [DECISO D-633 (9)]. Il precedente più vicino è `Action.CreateCover`, che erige una copertura bassa su un bordo per 2 turni [codice: `RTCatalogLibrary.cpp:1907-1924`] |
| Fase | `Preparation` (10) [CATALOGO; DECISO D-583 (1)], applicata a fase conclusa con le strutture di bordo [PROPOSTA; codice: `RTTurnManager.h:2736-2737`]: c'è già nel Dash e nel Blast del turno, come la copertura di `Action.CreateCover` [codice: `RTTurnManager.h:2707-2712`] |
| Costo AP | 1 [CATALOGO; classe Prep, DECISO D-594 (1)] |
| Cooldown | 1 [CATALOGO «CD 1»; PROPOSTA; DOMANDA → TG-6] |
| Priorità | 75 [PROPOSTA: come `Action.CreateCover`, codice: `RTCatalogLibrary.cpp:1921-1922`; il pareggio con lei si scioglie per id, DECISO D-615 (5)] |
| Portata e linea di vista | 3 [CSV GAD_PRP_03 «3 (Edge)»; coincide con `Action.CreateCover`, codice: `RTCatalogLibrary.cpp:1922`]; vista sulla cella da cui si indirizza il bordo, con la policy di default [PROPOSTA; codice: `RTActionDef.h:212-218`] |
| Forma | un bordo, 0 celle [CATALOGO «su un lato vuoto»; PROPOSTA]. «Vuoto» vuol dire senza copertura né porta, perché un bordo ne porta al più una di ciascuna [PROPOSTA; codice: `RTHexCellData.h:442-453`] |
| Bersaglio | un bordo vuoto entro la portata [CATALOGO; PROPOSTA] |
| Elemento | `Element.Aegis` [CATALOGO «🛡️ Scudo»; DECISO D-593 (4); LETTURA D-633 (11b)] |
| Tipo di danno | nessuno [CATALOGO] |
| Danno base | 0; 150 è l'integrità della parete [CATALOGO «(150 HP)»; CSV GAD_PRP_03 `Base_Damage_or_Shield` 150] |
| Movimento | nessuno [CATALOGO] |
| Stati | nessuno [CATALOGO] |
| Oggetti persistenti | una parete sul bordo, eretta come una copertura di `Action.CreateCover` [PROPOSTA; codice: `ERTStructureOp::CreateCover`, `RTActionDef.h:358-359`], ma alta: la sola copertura che nega la vista è `High`, e nega anche passo e proiettili [codice: `RTHexCellData.h:45-46`; PROPOSTA; DOMANDA → TG-18]. Integrità 150 [CATALOGO], contro i 30 di una copertura bassa e i 50 di una alta [codice: `FRTHexCover::DefaultIntegrity`, `RTHexCellData.h:123-126`], e oggi un colpo la consuma solo se l'azione dichiara `DamageStructure` [codice: `RTActionEvent.h:52-66`; `RTCatalogLibrary.cpp:1283-1285`; DOMANDA → TG-18]. Dura il turno del lancio: nasce nel Prep e scade nel Cleanup dello stesso turno [CATALOGO «per 1 turno»; PROPOSTA: `TurnsRemaining` 1, che il Cleanup di N porta a 0, codice: `RTTurnManager.h:2707-2733`] |
| Fallback | `Cancel` [PROPOSTA: come `Action.CreateCover`]; se due operazioni cercano lo stesso bordo, vince quella raccolta per prima [codice: `RTTurnManager.h:2745`] |
| Sistemi che mancano | un'erezione di copertura alta in partita: `CreateCover` «Erige una copertura bassa temporanea» [codice: `RTActionDef.h:358`], e fuori dall'allestimento delle arene il turno nomina solo il tipo basso [codice: `git grep -n -e "ERTHexCoverType::High" -- Source/RefactorTactics/Turn ':!Source/RefactorTactics/Turn/Format'` → exit 1 su `f012ed660`; controllo: lo stesso comando con `ERTHexCoverType::Low` → exit 0, `RTTurnManager.cpp:4794`]; una vista negata a una sola squadra, «la Linea di Vista nemica» [CATALOGO]: bordi e celle non hanno squadra [codice: `git grep -n -w TeamId -- Source/RefactorTactics/Map/RTHexCellData.h` → una sola riga, un commento, `:435`; controllo: lo stesso pattern su `Source/RefactorTactics/Turn/Resolution/RTHexSim.h` → exit 0]; la scadenza nell'hash [codice: `RTMatchStateHash.h:121-126`] |

**Esempio.** *Given* un alleato A in (0,0,0); R in (3,0,0), che vede A e nel turno N pianifica un attacco base su A. *When* nel Prep di N Gadget mette Holo-Barricade sul bordo fra (0,0,0) e (1,0,0). *Then* dal Prep al Cleanup di N la vista di R su A è negata [PROPOSTA]; l'attacco di R, dichiarato quando la parete non c'era, parte e si ferma sulla parete [DECISO D-490], e A non prende danno. La parete non perde integrità, perché l'attacco base non dichiara `DamageStructure` [codice: `RTCatalogLibrary.cpp:1283-1285`]; con la lettura «150 punti che ogni colpo consuma» di `TG-18`, scenderebbe a 150 meno il danno del colpo. Nel Cleanup di N la parete scompare.

**Domande che nascono qui.**

- [DOMANDA → TG-18] **Holo-Barricade.** I 150 HP sono un'integrità sulla scala delle strutture (30 e 50 oggi, consumata solo da `DamageStructure`) o punti che ogni colpo consuma, sulla scala dei danni? La parete nega solo la vista (un bordo che oggi non esiste) o anche passo e proiettili, come la copertura alta della proposta? «La Linea di Vista nemica» vuol dire che la squadra di Gadget vede attraverso?

**Domande della scheda.** `TG-6`, `TG-18`.

---

## 3. Dash

Gruppo del catalogo: «Dash Phase (🟡 Dash Phase | 2 AP | CD 2)» [CATALOGO].

**Scelte comuni.** Macro-fase Dash, `FastMovement` (20) [CATALOGO «Dash Phase»; DECISO D-583 (1); codice: `RTActionDef.h:27-28`]. 2 AP [CATALOGO; coincide con la classe Dash, DECISO D-594 (1)]. Cooldown 2 [CATALOGO «CD 2»]: una volta per piano, e salta il turno dopo [PROPOSTA: la semantica di «Come leggere la traduzione»; DOMANDA → TG-6]. Ripiego `Stop` per tutte e tre [PROPOSTA: è la regola che il validatore del catalogo dà a ogni azione di movimento, «azione di movimento con fallback diverso da Stop», perché «un movimento bloccato si ferma nell'ultima cella valida», codice: `URTCatalogLibrary::ValidateActions`, `RTCatalogLibrary.cpp:327-334`; la segue anche `Hero.Ivrin.PassingBlade`, `RTHeroCatalogLibrary.cpp:1085`].

**Uno scatto per piano.** Due scatti diversi sono rifiutati anche quando il budget li paga [DECISO D-625 (4); codice: `SecondDash`, `RTPlanValidationLibrary.cpp:322-337`], e uno scatto è ogni azione che risolve nella macro-fase Dash [LETTURA D-625 (i); codice: `URTCatalogLibrary::IsFastMovement`, `RTCatalogLibrary.cpp:189-192`]. Le tre voci, quindi, si escludono a vicenda nello stesso piano, anche Seeker Bot, che non sposta Gadget, finché resta nel Dash (`TG-21`) [PROPOSTA: conseguenza]. Uno scatto e un Move nello stesso piano restano legali [LETTURA D-625 (e)].

**Priorità** [PROPOSTA]: Flux Blink 25, come `Action.Leap`; Cryo-Drone 40, come `Action.Reposition`; Seeker Bot 45, dopo ogni scatto di oggi, perché il drone legga le posizioni dove gli scatti sono finiti. Le priorità del Dash di oggi sono 25 (`Action.Leap`), 30 (`Action.Dodge`, `Hero.Ivrin.PassingBlade`), 35 (`Action.Charge`) e 40 (`Action.Reposition`) [codice: `RTCatalogLibrary.cpp:1436`, `:1452`, `:1466`, `:1473`; `RTHeroCatalogLibrary.cpp:1084-1085`]. Con uno scatto per piano, un pareggio è solo fra unità diverse, e lo scioglie l'id dell'azione, poi l'unità [DECISO D-615 (5); codice: `RTActionQueueLibrary.cpp:6-30`].

### Seeker Bot — `Hero.Gadget.SeekerBot`

> «Opzione 1 (Core) — Seeker Bot [⚡ Fulmine]: Rilascia un drone che avanza di 3 esagoni lungo le macro-direttrici infliggendo 150 Danno Energetico, applicando Slow (+1 AP al movimento) e drenando -1 AP al primo nemico incontrato (Sintonizzazione Liv. 5: variante Fuoco).»

Nel `.csv` (`GAD_DSH_01`): `Range_Hex` «3-6 (Line)», `Base_Damage_or_Shield` 150, `Status_or_Effect` «Slow + -1 AP Drain»; la descrizione del `.csv` non dice «al primo nemico incontrato» e non ha la nota «Sintonizzazione».

| Campo | Valore |
|---|---|
| Ruolo | Dash, «Opzione 1 (Core)» [CATALOGO]; voce del kit [DECISO D-633 (9)]. La nota «Sintonizzazione Liv. 5: variante Fuoco» si toglie: «Le note «Liv. 3–10» del catalogo si tolgono» [DECISO D-633 (3); DECISO D-598] |
| Fase | Dash, `FastMovement` (20), con l'impatto applicato nel Blast come quello di `Action.Charge` [CATALOGO «Dash Phase»; PROPOSTA; codice: `RTCatalogLibrary.cpp:1440-1442`, «Gli effetti sono dichiarati qui, ma si applicano nel Blast»]; l'alternativa è un colpo del Blast con la cornice del Dash [DOMANDA → TG-21] |
| Costo AP | 2 [CATALOGO; classe Dash, DECISO D-594 (1)] |
| Cooldown | 2 [CATALOGO «CD 2»; PROPOSTA; DOMANDA → TG-6] |
| Priorità | 45 [PROPOSTA: vedi «Priorità» del gruppo] |
| Portata e linea di vista | 3 celle lungo la linea [CATALOGO «avanza di 3 esagoni»]; il `.csv` dice «3-6 (Line)» [CSV GAD_DSH_01; DOMANDA → TG-15]. Il drone va dritto e il primo ostacolo lo ferma, come la traiettoria diretta dell'attacco base [PROPOSTA; DECISO D-490 per l'attacco base] |
| Forma | una linea di 3 celle su una delle sei direzioni [CATALOGO «lungo le macro-direttrici»; PROPOSTA: gli attacchi lineari restano sulle sei direzioni, DECISO D-524 (1)], risolta come `Action.LineAttack`, «al PRIMO bersaglio valido su una delle sei direzioni», e non come la `Shape::Line`, che colpisce tutti quelli attraversati [codice: `RTCatalogLibrary.cpp:1505-1507`, `:1514-1518`; `ERTLineResolution::StopAtFirstTarget`, `RTActionDef.h:262-274`]. Un alleato non la ferma [codice: `RTActionDef.h:271-272`]: tocca un'unità sola, sulla prima delle 3 celle che ne ha una nemica [PROPOSTA] |
| Bersaglio | una direzione [PROPOSTA]; il primo nemico sulla linea [CATALOGO «al primo nemico incontrato»] |
| Elemento | `Element.Electric` [CATALOGO «⚡ Fulmine»; DECISO D-593 (1); LETTURA D-633 (11b)] |
| Tipo di danno | elementale Electric [PROPOSTA: «Danno Energetico»; DOMANDA → TG-7] |
| Danno base | 150 [CATALOGO] |
| Movimento | nessuno per Gadget [CATALOGO «Rilascia un drone»]; il drone non è un'unità e non occupa celle [PROPOSTA; DECISO D-583 (4)] |
| Stati | `Status.Slow` per 1 turno [CATALOGO «Slow»; PROPOSTA: la durata di `Action.Slow`, codice: `RTCatalogLibrary.cpp:1791-1793`]: oggi +1 al costo di ogni cella del Move, letto già sul Move dello stesso turno [codice: `RTTurnManager.cpp:8057-8060`]. Il catalogo dice «+1 AP al movimento» [CATALOGO], che è un'altra regola [DOMANDA → TG-19]. Drenaggio: −1 al budget di AP del bersaglio nel turno dopo, perché «Un piano già validato non cambia mai» [CATALOGO «drenando -1 AP»; DECISO D-613 (1); DOMANDA → TG-20] |
| Oggetti persistenti | nessuno [PROPOSTA: il drone vive dentro l'azione e sparisce con lei] |
| Fallback | `Stop`: il drone si ferma nell'ultima cella valida [PROPOSTA: la regola del catalogo per le azioni di movimento, codice: `RTCatalogLibrary.cpp:327-334`] |
| Sistemi che mancano | un effetto che cambi in partita il budget di AP di un'unità: fuori da test e Scenario Harness `SetActionPointBudget(` compare solo nella sua definizione [codice: `git grep -n "SetActionPointBudget(" -- Source/RefactorTactics ':!Source/RefactorTactics/Tests' ':!Source/RefactorTactics/ScenarioHarness'` → una riga, `RTUnit.h:1392`, su `f012ed660`; lo dice anche la cella Stato di D-613: «nessun effetto lo cambia ancora in partita»]; un'azione del Dash che non sposti chi la usa: `Action.Dodge`, `Action.Charge`, `Action.Leap`, `Action.Reposition` e `Hero.Ivrin.PassingBlade` hanno tutte uno stile di movimento [codice: `git grep -n "ERTResolutionPhase::FastMovement" -- Source/RefactorTactics/Ability/Catalog` → `RTCatalogLibrary.cpp:1436`, `:1452`, `:1466`, `:1473`, `RTHeroCatalogLibrary.cpp:1084`, con lo stile in `RTCatalogLibrary.cpp:1438`, `:1455`, `:1468`, `:1475`, `RTHeroCatalogLibrary.cpp:1092`]; il campo elemento e il percorso elementale [codice: vedi «Come leggere la traduzione»] |

⚠️ **Commento stantio.** La dichiarazione di `TAG_Status_Slow` dice ancora «range di movimento dimezzato» [codice: `RTGameplayTags.h:7`], contro i lettori, che sommano 1 al costo di ogni cella [codice: `RTTurnManager.cpp:8057-8060`, `RTFocusArea.cpp:46`], e contro il commento di `Action.Slow`, «non dimezza il raggio» [codice: `RTCatalogLibrary.cpp:1786`]. Per questa pagina valgono i lettori.

**Esempio.** *Given* Gadget in (0,0,0) con facing E; un alleato A in (1,0,0), un nemico X in (2,0,0) e un nemico Y in (3,0,0), X e Y con i numeri di R, X con 3 MP su terreno piano. *When* nel turno N Gadget pianifica Seeker Bot verso E (2 AP). *Then* il drone passa A, che non lo ferma [codice: `RTActionDef.h:271-272`], e si ferma su X: X prende `floor(150 × 100/135)` = 111 (lettura elementale; 103 con la DEF) e `Status.Slow`, e il suo budget del turno N+1 è 5, perché il piano di N è già validato [DECISO D-613 (1): «Un piano già validato non cambia mai»]; Y è intatto. Con lo `Status.Slow` di oggi, X nel Move di N paga 2 MP per cella e arriva a una cella invece che a tre; con la lettura «+1 AP» di `TG-19` il Move di N non cambia, e quello di N+1 costa 3 AP. Se X, nel Dash di N, esce dalla linea con uno scatto di priorità minore di 45, il drone prende Y [PROPOSTA]. Seeker Bot e Flux Blink nello stesso piano danno `SecondDash` [DECISO D-625 (4)].

**Domande che nascono qui.**

- [DOMANDA → TG-19] **«+1 AP al movimento».** Lo Slow di Seeker Bot e il Freddo del Cryo-Drone sono lo `Status.Slow` di oggi (+1 MP per cella, già sul Move del turno, la proposta) o un sovrapprezzo di 1 AP sul Move, che per D-613 (1) può valere solo dal planning del turno dopo? Il Freddo è lo stesso stato o uno nuovo, e quanto dura?
- [DOMANDA → TG-20] **Drenaggio di AP.** Il −1 di Seeker Bot colpisce il budget del turno dopo, come vuole D-613 (1); due drenaggi si sommano, e con quale budget minimo? È la stessa domanda che Riktor `TR-16` pone per Necrotic Anchor («Due Anchor sommano (−2), con quale tetto e quale budget minimo?»), voce che D-633 (5) toglie dal kit di Riktor ma che vale per il modello: una risposta vale per entrambi.
- [DOMANDA → TG-21] **Seeker Bot nel Dash o nel Blast.** Il drone parte nel Dash e l'impatto si applica nel Blast, come la carica (la proposta, che ne fa uno scatto per D-625 (4) e lo esclude dal piano di Flux Blink e del Cryo-Drone), o è un colpo del Blast con la cornice del Dash, che non conta come scatto? E danno, Slow e drenaggio sono tutti al primo nemico incontrato, come dice solo il `.md`?

**Domande della scheda.** `TG-6`, `TG-7`, `TG-15`, `TG-19`, `TG-20`, `TG-21`.

### Flux Blink — `Hero.Gadget.FluxBlink`

> «Opzione 2 (Disimpegno) — Flux Blink [🌌 Flux]: Teletrasporto istantaneo di 2 esagoni che ignora i muri solidi (senza attivare Overwatch) lasciando carica Flux sull'esagono di partenza.»

Nel `.csv` (`GAD_DSH_02`): `Range_Hex` 2, `Base_Damage_or_Shield` 0, `Status_or_Effect` «Teleport / No Overwatch».

| Campo | Valore |
|---|---|
| Ruolo | Dash, «Opzione 2 (Disimpegno)» [CATALOGO]; voce del kit [DECISO D-633 (9)] |
| Fase | `FastMovement` (20) [CATALOGO «Dash Phase»; DECISO D-583 (1)] |
| Costo AP | 2 [CATALOGO; classe Dash, DECISO D-594 (1)] |
| Cooldown | 2 [CATALOGO «CD 2»; PROPOSTA; DOMANDA → TG-6] |
| Priorità | 25 [PROPOSTA: come `Action.Leap`, lo stesso stile, codice: `RTCatalogLibrary.cpp:1466`] |
| Portata e linea di vista | fino a 2 [CATALOGO «di 2 esagoni»; PROPOSTA «fino a»: come ogni mobilità lineare, che oltre la portata non parte e dentro va dove si è mirato, codice: `RTMovementActionLibrary.cpp:127-131`]; nessuna linea di vista [PROPOSTA: un teletrasporto «ignora i muri solidi», CATALOGO] |
| Forma | l'atterraggio, una cella, scelta fra le celle a distanza 1 o 2 sullo stesso `Layer`, 18 [PROPOSTA: `3R(R+1)` con R = 2, il conto che D-524 (1) scrive per gli stili lineari, «Le celle a distanza ≤ R passano da 6R (le sei direzioni) a 3R(R+1)»; DECISO D-524 (1) per la retta libera di tutti e quattro gli stili]; il salto non cambia `Layer` [codice: `RTMovementActionLibrary.cpp:95`] |
| Bersaglio | una cella percorribile e libera [PROPOSTA; codice: `RTMovementActionLibrary.cpp:136-152`] |
| Elemento | `Element.Warp` [CATALOGO «🌌 Flux»; DECISO D-593 (5); LETTURA D-633 (11b)] |
| Tipo di danno | nessuno [CATALOGO] |
| Danno base | 0 [CSV GAD_DSH_02] |
| Movimento | stile `LinearLeap` con portata 2 [PROPOSTA]: il salto di oggi «scavalca le celle intermedie e guarda solo dove atterra» [DECISO D-524 (5)], ignora unità, celle in mezzo e bordi, e vuole un atterraggio percorribile e libero [codice: `RTMovementActionLibrary.cpp:136-152`]: ignora già i muri solidi [CATALOGO «ignora i muri solidi»]. È il «Transfer» di AGENTS.md §5, che «Cambia posizione senza percorrere celle intermedie», e quali azioni vi appartengano lo stabilisce l'owner gameplay [PROPOSTA; AGENTS.md §5]. Facing: l'ultimo passo della retta [DECISO D-524 (4)], con 2 passi di rotazione a fine scatto [codice: `DashEndPivotMaxSteps`, `RTHeroCatalogLibrary.cpp:408`] |
| Stati | nessuno [CATALOGO] |
| Overwatch | «senza attivare Overwatch» [CATALOGO]: il salto non apre un'opportunità di Overwatch, né alla partenza né all'atterraggio [PROPOSTA: la lettera]. Oggi un salto entra nella sola cella d'atterraggio [codice: `RTMovementActionLibrary.cpp:149`], ed è quella che un Overwatch potrebbe vedere entrare; l'Overwatch è una reazione del kit [DECISO D-633 (6)] |
| Oggetti persistenti | una carica Flux sulla cella di partenza [CATALOGO «lasciando carica Flux sull'esagono di partenza»]: un sistema che manca, e senza le cariche il ramo non fa niente [PROPOSTA] |
| Fallback | `Stop`: con l'atterraggio occupato o non percorribile Gadget resta dov'è [codice: `RTMovementActionLibrary.cpp:140`, `:145`; PROPOSTA: la regola del catalogo per le azioni di movimento, `RTCatalogLibrary.cpp:327-334`] |
| Sistemi che mancano | un'azione esente dall'Overwatch [codice: `git grep -n -i -e IgnoresOverwatch -e NoOverwatch -e TriggersOverwatch -e ProvokesOverwatch -e EvadesOverwatch -- Source` → exit 1 su `f012ed660`; controllo: `git grep -n -e BuildOverwatchTriggers -- Source/RefactorTactics/Turn/RTTurnManager_Movement.cpp` → exit 0]; un teletrasporto come stile proprio, se l'owner non lo vuole un salto: fuori da test e Scenario Harness non c'è [codice: `git grep -n -i -e teleport -e blink -- Source/RefactorTactics ':!Source/RefactorTactics/Tests' ':!Source/RefactorTactics/ScenarioHarness'` → exit 1 su `f012ed660`; controllo: lo stesso comando con `-e LinearLeap` → exit 0], e `Teleport` è fra le capacità dello Scenario Harness senza un produttore in partita [codice: `RTScenarioSession.cpp:503-513`]; le cariche di cella [codice: vedi Spark Bolt] |

**Esempio.** *Given* Gadget in (0,0,0); una copertura alta sul bordo fra (0,0,0) e (1,0,0); (2,0,0) libera e percorribile; un nemico in Overwatch la cui zona comprende (2,0,0). *When* nel Dash di N Gadget pianifica Flux Blink su (2,0,0) (2 AP). *Then* Gadget atterra in (2,0,0): il muro e la cella in mezzo non contano [DECISO D-524 (5)]; l'Overwatch non scatta [CATALOGO; PROPOSTA: nemmeno sull'atterraggio]; (0,0,0) riceverebbe una carica Flux, che oggi non esiste. Il facing è E, l'ultimo passo della retta [DECISO D-524 (4)]. Con (2,0,0) occupata Gadget resta in (0,0,0) (`BlockedByUnit`); con (3,0,0) come bersaglio il piano è rifiutato per portata; Flux Blink e Cryo-Drone nello stesso piano danno `SecondDash` [DECISO D-625 (4)].

**Domande della scheda.** `TG-6`. La carica Flux è il sistema delle cariche di cella che manca (vedi Spark Bolt), e nessuna domanda nuova nasce qui.

### Cryo-Drone — `Hero.Gadget.CryoDrone`

> «Opzione 3 (Zoning) — Cryo-Drone [❄️ Freddo]: Arretra di 1 esagono rilasciando un micro-drone stazionario sulla cella d'origine che applica Freddo (+1 AP al movimento) entro 1 esagono.»

Nel `.csv` (`GAD_DSH_03`): `Range_Hex` «1 (Back)», `Base_Damage_or_Shield` 80, `Status_or_Effect` «Stationary Slow Drone»; gli 80 non sono nel testo del `.md`.

| Campo | Valore |
|---|---|
| Ruolo | Dash, «Opzione 3 (Zoning)» [CATALOGO]; voce del kit [DECISO D-633 (9)] |
| Fase | `FastMovement` (20) [CATALOGO «Dash Phase»; DECISO D-583 (1)] |
| Costo AP | 2 [CATALOGO; classe Dash, DECISO D-594 (1)] |
| Cooldown | 2 [CATALOGO «CD 2»; PROPOSTA; DOMANDA → TG-6] |
| Priorità | 40 [PROPOSTA: come `Action.Reposition`, lo scatto corto, codice: `RTCatalogLibrary.cpp:1473`] |
| Portata e linea di vista | 1 [CATALOGO «Arretra di 1 esagono»; CSV GAD_DSH_03 «1 (Back)»] |
| Forma | lo scatto: una cella, quella dietro Gadget, opposta al facing [CATALOGO «Arretra»; PROPOSTA; codice: `ERTRelativeDirection::Rear`, `RTFacingLibrary.h:64`]. L'area del drone: la cella d'origine e le sei adiacenti, un esagono di raggio 1, 7 celle [CATALOGO «entro 1 esagono»; PROPOSTA: `3r(r+1)+1` con r = 1] |
| Bersaglio | nessuno da scegliere [PROPOSTA: la direzione la dà il facing]; il drone agisce sui nemici nell'area [PROPOSTA: dopo lo scatto Gadget è adiacente al drone, e col fuoco amico la voce rallenterebbe lei; DOMANDA → TG-13] |
| Elemento | `Element.Frost` [CATALOGO «❄️ Freddo»; DECISO D-593 (2); LETTURA D-633 (11b)] |
| Tipo di danno | elementale Frost, se il danno c'è [PROPOSTA; DOMANDA → TG-7] |
| Danno base | 80, solo nel `.csv` [CSV GAD_DSH_03]; il testo del `.md` non ne dà [CATALOGO]. A chi e quando [DOMANDA → TG-15; DOMANDA → TG-22] |
| Movimento | Gadget arretra di 1: uno scatto `LinearDash` di portata 1, come `Action.Reposition` con una cella sola [PROPOSTA; codice: `RTCatalogLibrary.cpp:1473-1475`], che si ferma prima di un muro, di un bordo che nega il passo o di un'unità [codice: `RTMovementActionLibrary.h:283-285`]. Il facing a fine scatto è l'ultimo passo [DECISO D-524 (4)], cioè le spalle verso la direzione di prima; con 2 passi di rotazione a fine scatto [codice: `RTHeroCatalogLibrary.cpp:408`] Gadget può riportare quella direzione nell'arco frontale, il cono di 120° verso il facing, ma non di fronte [PROPOSTA: conto sulle sei direzioni; codice: `URTHexCombatLibrary::IsInFrontalArc`, `RTHexCombatLibrary.cpp:213`] |
| Stati | `Status.Slow` per 1 turno a ogni nemico che, alla fine del Dash, sta nell'area [CATALOGO «applica Freddo (+1 AP al movimento)»; PROPOSTA: lo stesso stato di Seeker Bot, perché uno stato del Freddo non esiste e il catalogo gli dà la stessa regola]; letto già sul Move dello stesso turno [codice: `RTTurnManager.cpp:8057-8060`]. «+1 AP al movimento» [DOMANDA → TG-19] |
| Oggetti persistenti | il micro-drone sulla cella d'origine, che non la occupa [PROPOSTA; DECISO D-583 (4)]. Come superficie sostituirebbe quella della cella, che ne ha una sola [codice: `ERTHexSurface Surface`, `RTHexCellData.h:366`], quindi è un oggetto nuovo, stato di partita con una scadenza come `DynamicSurfaces` [codice: `RTTurnManager.h:2664-2677`]. Il precedente più vicino di un effetto legato a un'area di celle è il fumo di raggio 1, che applica `Status.Obscured` all'ingresso [codice: `Action.CreateSmoke`, `RTCatalogLibrary.cpp:1878`, `:1881-1885`; `FRTTerrainDef::OnEnterEffects`, `RTTerrainData.h:101-103`]. Durata: il turno del lancio [PROPOSTA: il catalogo non la dà; DOMANDA → TG-22] |
| Fallback | `Stop`: con la cella dietro occupata o chiusa Gadget resta dov'è [PROPOSTA: la regola del catalogo per le azioni di movimento, codice: `RTCatalogLibrary.cpp:327-334`]; dove nasca allora il drone è [DOMANDA → TG-22] |
| Sistemi che mancano | un oggetto di cella che non sia superficie, copertura o porta, con un effetto d'area [codice: `git grep -n -i -w -e drone -e drones -e summon -e deployable -e turret -e torretta -- Source` → exit 1 su `f012ed660`, e `git grep -n -i -w -e aura -e auras -- Source` → exit 1; controllo dello stile: `git grep -n -i -w -e trap -e trappola -- Source` → exit 0]; uno stato del Freddo [codice: `git grep -n -i -e "Status\.Frost" -e "Status\.Chill" -e "Status\.Cold" -e "Status\.Frozen" -e "Status\.Freez" -- Source` → exit 1 su `f012ed660`; controllo: `git grep -n -e "Status\.Slow" -- Source/RefactorTactics/Core` → exit 0]; le cariche di cella, se il Freddo è anche una carica [codice: vedi Spark Bolt; DOMANDA → TG-22]; il campo elemento e il percorso elementale [codice: vedi «Come leggere la traduzione»] |

**Esempio.** *Given* Gadget in (0,0,0) con facing E; (−1,0,0) libera; un nemico Z in (1,0,0), adiacente, con 3 MP su terreno piano e i numeri di R. *When* nel Dash di N Gadget usa Cryo-Drone (2 AP). *Then* Gadget arretra in (−1,0,0), la cella opposta al facing [PROPOSTA]; il drone resta in (0,0,0) e la sua area è (0,0,0) e le sei adiacenti, 7 celle, fra cui (−1,0,0). Z, nell'area alla fine del Dash, prende `Status.Slow` e nel Move di N arriva a una cella invece che a tre [PROPOSTA]; se gli 80 del `.csv` sono un danno all'area, Z prende anche `floor(80 × 100/135)` = 59 (lettura elementale; 55 con la DEF). Gadget, in (−1,0,0), resta fuori dallo stato, perché la proposta lo dà ai soli nemici. Il facing di Gadget a fine scatto è W; con 2 passi di rotazione arriva al più a NE o a SE, che hanno E nell'arco frontale. Con (−1,0,0) occupata Gadget resta in (0,0,0).

**Domande che nascono qui.**

- [DOMANDA → TG-22] **Cryo-Drone.** Quanto dura il drone (la proposta: il turno del lancio)? Il Freddo è uno stato dei nemici (la proposta) o anche una carica sulle celle dell'area, nel sistema delle cariche che manca? Lo applica a chi sta nell'area alla fine del Dash (la proposta), a chi vi entra, o in ogni fase? Gli 80 danni del `.csv` sono un colpo all'area, a chi e quando? «Arretra» vuol dire la cella opposta al facing (la proposta)? Se Gadget non può arretrare, il drone nasce lo stesso sulla sua cella? Il drone si può distruggere?

**Domande della scheda.** `TG-6`, `TG-7`, `TG-13`, `TG-15`, `TG-19`, `TG-22`.
