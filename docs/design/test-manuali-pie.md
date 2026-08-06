# Test manuali (PIE) — verifiche interattive da eseguire

> Verifiche che richiedono l'editor UE (PIE, mouse, asset) e **non** sono automatizzabili headless.
> **Complementari** ai test Automation (suite integrata **bot + hex** su `main`, tutti verdi). Parte del DoD «playtest ogni incremento» (roadmap §QA).
> Regola: una voce è ✅ **solo dopo** verifica reale in PIE — non «dovrebbe funzionare».
> Per lavorarci in modo efficiente vedi **«Sessioni di verifica consigliate»** in fondo: le voci aperte sono
> raggruppate per preparazione condivisa, così l'editor si apre una volta per gruppo invece che una per voce.
> **Quale voce affrontare e quando** lo dice [`roadmap-editor.md`](roadmap-editor.md): questo file resta il
> **registro** (esito atteso e stato), quello è la **sequenza** (sedute U1–U17, artefatti da creare, dipendenze
> verso i checkpoint di codice).

## Come eseguire
- Apri il progetto: doppio clic su `RefactorTactics.uproject`. All'avvio l'editor può chiedere **quale versione
  dell'engine** usare: scegli **5.8** (`D:\EpicGames\UE_5.8`), la versione bloccata dal progetto. Se chiede di
  **ricompilare i moduli**, accetta — oppure compila da riga di comando (vedi `ue58-build-gotchas`).
  - **Perché lo chiede**: su questa macchina UE 5.8 è registrato come **build custom** (chiave
    `HKCU\Software\Epic Games\Unreal Engine\Builds`, GUID `{B20BD8AB-…}` → `D:/EpicGames/UE_5.8`), non come
    installazione del Launcher — l'unica registrata per nome è la 5.4. Quindi `"EngineAssociation": "5.8"` non
    risolve nulla in locale e l'editor apre il selettore.
  - Dopo la scelta l'editor **riscrive il GUID** dentro `RefactorTactics.uproject`: è corretto e va lasciato
    così in locale (niente più dialoghi). **Non committare quella modifica**: il GUID vale solo su questa
    macchina, il file versionato deve restare a `"5.8"`. Se finisce nello stage per sbaglio:
    `git checkout -- RefactorTactics.uproject`.
- **PIE**: pulsante Play (o `Alt+P`). Il `TurnManager` ha `PlanningSeconds≈30s`: premi **Spazio** per il lock-in manuale.
- I `LogRT: [RT] ...` nell'**Output Log** narrano il round (fasi, esiti).
- I livelli del demo (`L_Prototype`, `L_DevSandbox`) sono **vuoti nell'editor**: griglia, luce, unità e turn manager
  li allestisce a runtime il `RTGameMode`. Viewport nera prima del Play = normale, non un livello rotto.
  Lo **sfondo resta nero anche in gioco**: il GameMode aggiunge una luce direzionale, nessun cielo.
- Camera: **`Home`** ricentra sulla griglia (il pawn parte dall'origine, la board 10×10 si estende per 2000 uu);
  **`F`** centra sull'**unità selezionata** — lo zoom orbita attorno al pawn, quindi senza spostarlo la rotellina
  avvicina al centro della mappa e non al personaggio. Inclinazione e distanza si tarano dal Details del
  `RTCameraPawn` (`Camera Pitch`, `Default Arm Length`) con effetto immediato, anche a PIE avviato.
  Se la vista sembra bloccata e compaiono le **etichette degli actor** in viewport, hai fatto **Eject** (`F8`):
  stai guardando con la camera dell'editor, non con quella del gioco — `F8` di nuovo per rientrare nel pawn.

## Checklist

| ID | Cosa verificare | Precondizione | Esito atteso | Stato |
|----|-----------------|---------------|--------------|-------|
| **PIE-AS5** | Anello di team a terra | `M_TeamRing` creato + assegnato a `TeamRingMaterial` sui `BP_Unit` | Anello **blu** (team 0) / **rosso** (team 1) sotto ogni unità, visibile dall'alto; senza `M_TeamRing` nessun anello (cilindro colorato come prima) | ✅ 2026-08-05 |
| **PIE-SEL** | Anello di selezione (anche su skeletal) | `M_SelectionRing` creato + assegnato a `SelectionRingMaterial` sui `BP_Unit` | Selezionando un'unità compare un **anello giallo** a terra (cornice esterna al TeamRing), visibile anche quando il cilindro è nascosto (personaggio skeletal); deselezionando sparisce. Senza `M_SelectionRing`: nessun anello (fallback: resta solo l'ingrandimento del cilindro) | ✅ 2026-08-05 — **non serve un materiale dedicato**: basta assegnare `M_TeamRing` a `SelectionRingMaterial`, il colore (giallo) lo imposta il codice sul MID via parametro `Color`; i due anelli restano distinguibili per scala (1.6 team, 1.9 selezione) |
| **PIE-P3** | Combat log mostra i reason (TurnLog) | — (funziona anche col cilindro) | Destinazione contesa → log «fermo (cella contesa)»; attacco senza LOS → «nessuna linea di tiro» | ✅ 2026-08-05 — entrambi i reason osservati nel log: contesa in fase di risoluzione, e `BP_Unit_Ranger_C_1 coperto (nessuna linea di tiro)` **in pianificazione** (il controller valida la LOS al momento del bersagliamento, non solo al lock-in) |
| **PIE-AS2** | Personaggio skeletal appoggiato a terra | `BP_Unit_Guardian` (Gideon, `VisualZOffset=0`) → `GuardianUnitClass` | Al posto del cilindro compare il personaggio, a terra (nessun «fluttuamento») | ✅ 2026-08-05 |
| **PIE-AS4a** | Locomozione Idle↔Run | `ABP_Gideon` + bind dei delegate (guida-animazioni-paragon) | In fase **Move** Gideon passa a `Jog_Fwd`, torna `Idle` a fine risoluzione | ⏳ |
| **PIE-AS4b** | Colpi e morte (montages) | `AM_Gideon_Cast/Hit/Death` + bind `OnAttackResolved`/`OnUnitDefeated` | Nel **Blast**: attaccante gioca `Cast`, bersaglio `Hit`; morte → `Death` | ⏳ |
| **PIE-FACING** | Orientamento al movimento | `bFaceMovementDirection=true` sul `BP_Unit` | L'unità ruota (yaw) verso la direzione di corsa; `Jog_Fwd` credibile in ogni direzione | ✅ 2026-08-05 — corsa orientata correttamente. **Nota di design**: a fine movimento l'unità resta voltata verso l'ultima direzione percorsa (scelta confermata: non torna a un orientamento "avanti") | 
| **PIE-MP4** | Click → layer (multilivello) | mappa col ponte sopraelevato | Il click seleziona la cella del **layer giusto** (terra vs ponte) | 🟡 **logica coperta headless** da `RefactorTactics.Hex.WorldToCellIdRoundTripAcrossLayers` (il punto-mondo torna la cella **completa**, layer incluso, e la composizione e' la stessa usata dal click di gioco). ⏳ al PIE resta il gesto col mouse su celle sovrapposte: che cliccando il ponte si selezioni la cella del ponte e non quella sotto |
| **PIE-CP1.4** | Evidenziazione cella sotto il cursore | — | La cella sotto il mouse è evidenziata | ✅ 2026-08-05 |
| **PIE-HEX** | Griglia esagonale graybox (pivot) | `ARTHexMapActor` in un livello, `DemoRadius > 0` | Griglia di celle esagonali visibile (graybox); con `MapAsset` popolato mostra quelle celle | ✅ 2026-08-05 (con `DemoRadius=0` la griglia resta: viene dall'asset) |
| **PIE-HEX-LAYER** | Filtro layer attivo (H4) | `ARTHexMapActor` con celle su ≥2 layer (es. `GenerateIntoAsset` con `ActiveLayer=0`, poi `ActiveLayer=1`) | `LayerView=ActiveOnly` mostra **solo** le celle di `ActiveLayer`; `AllLayers` le mostra tutte, impilate per quota (`LayerHeight`) → la viz non confonde i livelli | ⏳ (branch `feat/hex-grid`, H4b) |
| **PIE-HEX-TRANS** | Transizione verticale bridge/scala (H4) | due celle sovrapposte (stessi X/Y, Layer diverso), `TransitionFrom`/`TransitionTo` impostati | `AddVerticalTransition` collega i due layer (Undo/Redo ok, package dirty, validator pulito); `RemoveVerticalTransition` lo toglie | ⏳ (branch `feat/hex-grid`, H4b) |
| **PIE-HEX-MODE-A** | Editor Mode hex appare e si attiva (H5a) | modulo `RefactorTacticsEditor` compilato | Nella toolbar Modes compare «Hex Map»; attivandolo il pannello si apre senza crash (nessun tool) | ✅ 2026-08-05 |
| **PIE-HEX-MODE-B** | Selezione a click nel viewport (H5b) | mode Hex Map attivo, `ARTHexMapActor` nel livello (selezionato o unico) | Tool «Select» attivo → click su una cella → esagono giallo sulla cella + `SelectedCell`/superficie/costo/blocco corretti nel pannello; cambiando `ActiveLayer` sull'actor seleziona il piano giusto (celle sovrapposte) | ✅ 2026-08-05 |
| **PIE-HEX-MODE-C** | Paint a click nel viewport (H5c) | mode Hex Map attivo, tool Paint, `ARTHexMapActor` nel livello | Con `Operation=Paint`, click su una cella → esagono verde + cella creata/aggiornata (superficie/costo/blocco del pennello); `LastCell` corretto; Undo ripristina | ✅ 2026-08-05 (il refresh dopo Undo richiedeva il fix `ea51b45`) |
| **PIE-HEX-MODE-D** | Erase a click nel viewport (H5c) | mode Hex Map attivo, tool Paint | Con `Operation=Erase`, click su una cella esistente → esagono rosso + cella rimossa dall'ISM; Undo ripristina; cambiando `ActiveLayer` agisce sul piano giusto | ✅ 2026-08-05 |
| **PIE-HEX-MODE-F** | Render transizioni nel tool Arch (H5c.2a) | mode Hex Map, tool Arch, `ARTHexMapActor` con transizioni | Le transizioni esistenti appaiono come linee colorate (per Kind) con freccia From->To | ⏳ (branch `feat/hex-grid`, H5c.2a) |
| **PIE-HEX-MODE-E** | Crea transizione via gizmo (H5c.2b) | mode Hex Map, tool Arch, `ARTHexMapActor` con celle su >=2 layer | Click From → gizmo → drag su To (anche altro layer, snap a cella) → Commit crea la transizione (visibile); Undo la rimuove; ClearArch annulla il pendente | ⏳ (branch `feat/hex-grid`, H5c.2b) |
| **PIE-HEX-MODE-G** | Ciclo di vita del gizmo (smoke, H5c.2b) | mode Hex Map, tool Arch, `ARTHexMapActor` nel livello | Click su una cella → compare il gizmo di traslazione; **re-click** su un'altra cella → resta **un solo** gizmo (nessun duplicato); **cambio tool** (Select) o uscita dal mode → il gizmo **sparisce** (nessun gizmo orfano in scena) | ⏳ (branch `feat/hex-grid`, H5c.2b) |
| **PIE-HEX-MODE-H** | Snap del gizmo cross-layer (H5c.2b) | mode Hex Map, tool Arch, celle su >=2 layer | Trascinando il gizmo, `To` si aggancia sempre al **centro di una cella**; alzando la quota di ~`LayerHeight` il target passa al **layer superiore** (`WorldToLayer`); nessun jitter/loop durante lo snap (guardia `bSnapping`) | ⏳ (branch `feat/hex-grid`, H5c.2b) |
| **PIE-HEX-MODE-I** | Drag-paint (H5c.3b) | mode Hex Map, tool Paint (`Operation=Paint`), `ARTHexMapActor` con `MapAsset` | Tenere premuto e trascinare dipinge più celle in una pennellata (dedup: ripassare non ridipinge); **un** Ctrl+Z annulla l'intera pennellata; click singolo = 1 cella (PIE-C invariato) | ✅ 2026-08-05 |
| **PIE-HEX-MODE-J** | Drag-erase (H5c.3b) | mode Hex Map, tool Paint (`Operation=Erase`) | Trascinare cancella più celle in una pennellata; un Undo le ripristina tutte; cambiare tool a metà drag non lascia transazioni aperte; **erase su celle inesistenti/vuote NON crea voci Undo né marca l'asset dirty** (transazione lazy) | ✅ 2026-08-05 |
| **PIE-HEX-MODE-K** | Pennello a raggio N (H5c.4) | mode Hex Map, tool Paint, `ARTHexMapActor` con `MapAsset` | `BrushRadius=0` → 1 cella (come prima); `BrushRadius=N>0` → un click dipinge/cancella l'esagono pieno di raggio N; drag dipinge fasce larghe (dedup); **un** Ctrl+Z annulla l'intera pennellata | ✅ 2026-08-05 |
| **PIE-HEX-MODE-L** | Rimuovi arco via tool (H5c.5b) | mode Hex Map, tool Arch, `ARTHexMapActor` con transizioni | Con `Operation=Remove`, click su un arco disegnato lo rimuove (Undo lo ripristina); click nel vuoto (nessun arco entro soglia) non fa nulla; con `Operation=Add` il flusso gizmo resta invariato | ⏳ (branch `feat/hex-grid`, H5c.5b) |
| **PIE-HEX-MODE-M** | Overlay debug superfici (H5c.6) | mode Hex Map, tool Select o Paint, `ARTHexMapActor` con celle di superfici diverse | Con `bShowOverlay` attivo, ogni cella appare come esagono colorato per superficie (Water blu, Fire arancio, Mud marrone, ...); le celle bloccate hanno un esagono rosso interno; `bShowOverlay` off = nessun overlay | ✅ 2026-08-05 |
| **PIE-HEX-MODE-N** | Secchiello / flood-fill (H5c.7) | mode Hex Map, tool Fill, `ARTHexMapActor` con `MapAsset` popolato | In Fill, click su una regione la riempie col pennello corrente; un Ctrl+Z ripristina l'intera regione; click su cella vuota non fa nulla; passando a Select/Paint con overlay si vedono i nuovi colori | ⏳ (branch `feat/hex-grid`, H5c.7) |
| **PIE-BU2** | Bot: posizionamento via utility scoring | branch `feat/bot-utility` | In pianificazione il bot sceglie la cella pesando **minaccia/kiting** (può **restare** invece di esporsi); il combat log mostra `<Bot>: utility -> (x,y,Lz) score=N`. Il kiter (Ranger) mantiene la distanza, la mischia (Guardian) chiude, nessuno corre in celle sotto tiro. Osserva se gli score hanno senso → base per il **tuning dei pesi** (BU.3) | ✅ |
| **PIE-BU2b** | Tuning pesi bot in editor | worktree `feat/bot-utility`, PIE attivo | Modificando `WKill/WThreat/WKiteViolation/WApproach/WDamage/WElevation` sul `TurnManager` (World Outliner → Details ▸ *Bot*) il comportamento cambia **dal turno successivo, senza ricompilare**: es. ↑`WThreat` = bot più prudente; ↓`WApproach` = mischia meno aggressiva; ↑`WElevation` = predilige le alte quote. Dettagli nella nota sotto | ✅ |
| **PIE-BU3** | Bot: utility unica posizione/attacco | worktree `feat/bot-utility`, **dopo** refactor BU.3b | Un'unica utility sceglie fra **{resta e attacca}** e **{muoviti per posizionarti}** (l'attacco vale solo da fermo: il Blast precede il Move). Verifica: se attaccare da fermo espone troppo il bot preferisce ripararsi invece di sparare; se l'attacco **uccide** spara sempre; guardie **support/panic/dash** intatte; log `utility -> ... attacca X score=N` oppure `... score=N (resta)` | ✅ |
| **PIE-BU3c** | Bot: dash+attacco (scatto poi colpisce) | worktree `feat/bot-utility`, **dopo** BU.3c | Se scattando raggiunge una cella da cui ha tiro e l'attacco conviene (utility), il bot pianifica **scatto + attacco** (log `utility -> scatto (x,y,Lz) + attacca X`): nel Blast (dopo il Dash) colpisce dalla cella post-scatto. **Nota**: se lo scatto è deviato da un conflitto di movimento simultaneo, l'attacco può mancare (log `nessuna linea di tiro`) — coerente coi turni simultanei | ✅ |

### Partita su griglia esagonale (M6 — Parità hex)

> Voci **pianificate in anticipo**, per definire *prima* cosa dovrà dimostrare lo switch, così la verifica non
> viene inventata a lavoro finito. Precondizioni comuni: un livello con `ARTHexMapActor` + `MapAsset` popolato
> (vedi «Mappa di prova» sotto) e il `RTGameMode` che allestisce la partita su quella mappa.
>
> **Tutte eseguibili da CP 6.7** (2026-08-06): allestimento, input, movimento, scatto, collisione, LOS, forme,
> spinta, bot e HUD passano dallo strato esagonale, con un'unica fonte di scala
> (`ARTTurnManager::GetHexContext`, la stessa che usa la HUD). La **8** (multilivello) richiede in più una
> mappa con due layer e un arco, cioè l'artefatto d'editor della seduta U-multilivello.

| ID | Cosa verificare | Precondizione | Esito atteso | Stato |
|----|-----------------|---------------|--------------|-------|
| **PIE-HEXPLAY-1** | Allestimento della partita su mappa hex | livello di prova + GameMode hex | All'avvio del PIE si vede la griglia **esagonale** con 4 unità (2v2) **centrate sui centri-cella** (nessun offset né compenetrazione); la camera inquadra l'arena; nessun residuo della griglia quadrata | 🟡 **coperto headless** da `RefactorTactics.MatchSetup.GameModeSpawnsOnHexMap` (board 2v2 sulle celle di partenza, nessuna sovrapposizione), `…GameModeFallsBackOnEmptyMapAsset` e `…MapSourceTestArenaWinsOverLevelAsset`. ⏳ al PIE resta cio' che solo l'occhio vede: unita' **centrate sui centri-cella** senza offset ne' compenetrazione, e nessun residuo di griglia quadrata a schermo. Nota: le unita' sono **cilindri** (i `BP_Unit_*` non esistono piu', fallback previsto); con `MapSource=GeneratedTestArena` si gioca sulla mappa di prova generata da codice |
| **PIE-CAM-START** | La partita si apre sulla propria squadra, da vicino | partita hex avviata | All'avvio la camera è centrata sul **punto medio delle proprie unità** (non sul centro della mappa) e più ravvicinata di `Home`: braccio a `MatchStartArmLength` (default 450) invece di `DefaultArmLength` (800). Nel log compare `Camera sulla squadra <TeamId> (<N> unità, arm=…)` e il `CameraPawn BeginPlay` successivo riporta lo **stesso** braccio — se riportasse 800 l'inquadratura sarebbe stata sovrascritta. `Home` deve continuare a mostrare l'insieme della mappa a 800 | ✅ **2026-08-06** — `Camera sulla squadra 0 (2 unita', arm=450)` seguito da `CameraPawn BeginPlay (arm=450, pitch=-40)`: applicata e non sovrascritta, riuscita al **primo** tentativo (nessun ripiego su `RecenterView`). Confermata a schermo dall'utente |
| **PIE-HEXPLAY-2** | Selezione e cella sotto il cursore | partita hex avviata | Click su un'unità la seleziona; l'evidenziazione (esagono **giallo**) segue la cella **esagonale** sotto il mouse; su mappa multilivello si seleziona la cella del **layer giusto** — il layer viene dalla **quota** del punto colpito, quindi cliccando il ponte si evidenzia la cella del ponte | 🟡 **2026-08-06** — selezione ✅ (`Selezionata: RTUnit_1`, `RTUnit_0`), guardia sulle avversarie ✅ (`e' avversaria: seleziona prima una tua unita'`), evidenziazione gialla sotto il cursore ✅ confermata a schermo dall'utente. ⏳ resta il **layer su mappa multilivello**: l'arena di ripiego ha un solo layer, serve la mappa di prova |
| **PIE-HEXPLAY-3** | Pianificazione del movimento entro budget | unità del giocatore selezionata | Una cella valida aggiunge un waypoint e mostra l'anteprima del percorso (esagoni **ciano** + segmenti fra i centri); una cella **oltre il budget**, **bloccata**, **occupata** o **fuori mappa** viene rifiutata: il piano precedente resta intatto e il log riporta il motivo. Più waypoint deviano il percorso (non prende la scorciatoia) e il budget si spende **cumulativamente**. **Click destro** (o `Backspace`) annulla l'ultimo waypoint e l'anteprima si accorcia | 🟡 **2026-08-06** — verificato dal log: waypoint accettati con budget **cumulativo** (`Piano: RTUnit_0 -> 1 waypoint (costo 1/5)` … fino a `5 waypoint (costo 5/5)`), rifiuto a budget esaurito (`Waypoint (0,-3,L0) rifiutato (…, budget 5)`), undo col click destro (`Annullato waypoint: RTUnit_0 -> 2 waypoint`), budget diverso per archetipo (3 e 5). ⏳ restano **cella bloccata** e **costo del terreno**: l'arena di ripiego è tutta pavimento a costo 1 |
| **PIE-HEXPLAY-4** | Risoluzione e playback del movimento | piani impostati, lock-in con **Spazio** | Le unità scorrono di cella in cella lungo il percorso risolto; a fine playback ogni unità è **esattamente** sul centro della cella finale e la posizione visiva coincide con la cella logica (nessuna deriva accumulata) | ⏳ **eseguibile da CP 6.2**. Coperto headless da `RefactorTactics.HexMove.UnitReachesPlannedCell`; il PIE aggiunge cio' che il test non vede: la fluidita' dello scorrimento |
| **PIE-HEXPLAY-3b** | Il rifiuto del bersaglio dice il motivo **giusto** | unità propria selezionata, un nemico **fuori portata** e uno **dietro una copertura** | Cliccando il nemico **fuori portata** il log dice «fuori portata (max N)» — **mai** «coperto», nemmeno su un'arena senza un solo muro. Cliccando quello dietro una cella con `bBlocksLineOfSight` dice «coperto (nessuna linea di tiro)». Con l'abilità in ricarica o senza energia dice «non pronta», prima di ogni altra verifica. Nel Blast, un attacco fermato dalla copertura compare nel combat log come reason code con coordinate assiali | 🟡 **2026-08-06** — metà **fuori portata** ✅ dal log: `RTUnit_3 fuori portata (max 3)` e `(max 7)` su un'arena **senza un solo muro**, dove prima usciva «coperto». Anche il caso positivo osservato: `Piano: RTUnit_0 usa Colpo preciso su RTUnit_3`. ⏳ resta la metà **coperto**: serve una cella con `bBlocksLineOfSight`, che l'arena di ripiego non ha. Coperto headless da `Combat.HexTargetingReasonDistinguishesRangeFromCover` e `HexCombat.NoMapFailClosed` |
| **PIE-HEXPLAY-4b** | Scatto (fase Dash) su hex | unità con abilità di scatto pronta, destinazione entro la sua portata | Lo scatto si risolve **prima** del Blast e porta l'unità sulla cella scelta anche in direzione obliqua (dove il budget quadrato l'avrebbe rifiutata); oltre la portata, su cella occupata o bloccata lo scatto **non avviene** (l'unità resta, la ricarica scatta comunque); scatto + movimento nello stesso turno restano compatibili | ⏳ **eseguibile da CP 6.5**. Coperto headless da `RefactorTactics.HexMove.DashReachesCellOnHex` / `DashRejectsOutOfBudget` |
| **PIE-HEXPLAY-5** | Collisione simultanea su hex | due unità pianificate verso la **stessa** cella | Entrambe restano ferme (o si fermano prima), **nessuna sovrapposizione**; il combat log riporta il reason «cella contesa»; ripetendo con lo scambio diretto A↔B lo scambio **riesce** | ⏳ **eseguibile da CP 6.2**. La contesa e' coperta headless da `RefactorTactics.HexMove.ContestedCellStopsBoth` (due esiti `BlockedContested` nel TurnLog); lo **scambio diretto A↔B** non e' coperto da test: e' il caso che il PIE deve davvero esercitare |
| **PIE-HEXPLAY-6** | Copertura: LOS esagonale | una cella con `bBlocksLineOfSight` fra attaccante e bersaglio | L'attacco pianificato attraverso il muro viene scartato con «nessuna linea di tiro»; spostandosi di una cella di lato il tiro va a segno. Con un ostacolo su un **altro layer** il tiro passa (regola di elevazione) | ⏳ **eseguibile da CP 6.4**. Coperto headless da `RefactorTactics.HexBlast.NoLineOfSightOnHexMap`; il PIE aggiunge ciò che il test non vede: che il giocatore **capisca** dal log perché il colpo non parte, e la prova sul multilivello |
| **PIE-HEXPLAY-6b** | Forme d'attacco su esagoni | Guardian (Spazzata = cono) e Ranger (Colpo preciso = linea, Raffica = area) con più nemici in zona | Il **cono** colpisce il ventaglio davanti all'attaccante (3 celle a distanza 1), la **linea** colpisce anche chi sta sulla traiettoria prima del bersaglio, l'**area** colpisce il bersaglio e i suoi 6 vicini; **nessun alleato** viene colpito. Il combat log elenca un colpo per bersaglio | ⏳ **eseguibile da CP 6.4**. Forme coperte headless (`RefactorTactics.HexCombat.Shape*`): il PIE verifica che la zona colpita sia **leggibile a schermo**, non solo corretta nei dati |
| **PIE-HEXPLAY-6c** | Spinta del Guardian su hex | Guardian che colpisce con Spazzata (knockback 2) | Il bersaglio è respinto lungo una delle **6 direzioni esagonali** (quella del colpo), non lungo un asse cardinale; si ferma davanti a un ostacolo, a un'altra unità o al bordo della mappa; due spinte opposte sullo stesso bersaglio si **annullano** e due bersagli spinti verso la stessa cella restano entrambi fermi | ⏳ **eseguibile da CP 6.5**. Coperto headless (`HexCombat.Knockback*`, `HexBlast.KnockbackOnHexGrid`); il PIE verifica lo **scivolamento animato** lungo il percorso |
| **PIE-HEXPLAY-7** | Bot su hex | almeno un'unità con `bIsBotControlled` | Il bot propone **solo mosse legali** (mai celle occupate o fuori budget), preferisce le celle al riparo, il kiter mantiene la distanza e la mischia chiude; il log utility mostra celle in coordinate **assiali** `(q,r,L)` | ⏳ **eseguibile da CP 6.6**. Coperto headless da `RefactorTactics.HexBotPlay.*` (mosse legali, panico, supporto, tuning, scatto prudente); il PIE aggiunge il **giudizio sul comportamento**: gli score hanno senso guardando la partita? |
| **PIE-HEXPLAY-8** | Multilivello: movimento via arco | mappa con due layer collegati da una transizione | Un percorso che usa scala/ponte **cambia layer**; il playback porta l'unità alla quota giusta (`LayerHeight`); rimuovendo l'arco i due layer tornano irraggiungibili (il path fallisce, non «teletrasporta») | 🟡 **coperto headless 2026-08-06** da `RefactorTactics.HexMove.ClimbsOnlyThroughTransition`: con la transizione l'unità sale sul layer 1, togliendola resta a terra (il path **fallisce**, non teletrasporta). Al PIE resta da guardare che il **playback** porti l'unità alla quota giusta (`LayerHeight`) invece di scivolare sul piano |
| **PIE-HEXPLAY-9** | HUD e anteprima piani su hex | partita hex avviata | Barre HP/scudo/energia, timer, fase e combat log invariati; l'anteprima dei piani (ciano), i marker dei waypoint, la preview dello scatto (magenta) e la traccia post-lock (grigia) seguono i **centri esagonali** e coincidono col percorso realmente eseguito; il combat log riporta i reason code con coordinate **assiali** `(q=..,r=..,L=..)`; **`Home`** ricentra la camera sulla mappa esagonale | ⏳ **eseguibile da CP 6.7** |
| **PIE-HEXPLAY-10** | Partita completa fino alla vittoria | partita hex avviata, almeno un'unità per squadra col bot | La partita arriva a una **conclusione**: una squadra viene eliminata, compare «PARTITA FINITA» e `R` riavvia. Durante i turni: nessuna unità sovrapposta o fuori mappa, nessun blocco della pianificazione. **Dato di riferimento** (misurato headless il 2026-08-06): bot contro bot la partita si decide al **turno 10**, dentro il limite di 12 del catalogo. Era **25** finché lo scudo di supporto non scadeva e si accumulava (issue `#96`, risolta) | ⏳ **eseguibile da CP 6.7**. Coperta headless da `RefactorTactics.HexMatch.PlaysToCompletion` (tenuta e invarianti); il PIE aggiunge il **giudizio sul ritmo**, che nessun test può dare |

### Contenuto della v0.1 (catalogo azioni, eroi, ambiente, strutture)

> Voci **pianificate in anticipo** per la release **v0.1** ([`roadmap-v0.1.md`](roadmap-v0.1.md)): il codice non
> esiste ancora. Le prime dodici traducono la matrice di test manuali del catalogo di bilanciamento (§14); le
> ultime cinque coprono roster, HUD e strumenti di debug.
> Precondizioni comuni: partita hex avviata (sessione D verde) e catalogo v0.1 caricato dai data asset.
> Riferimento issue in [`v0.1-issue-plan.md`](v0.1-issue-plan.md).

| ID | Cosa verificare | Precondizione | Esito atteso | Stato |
|----|-----------------|---------------|--------------|-------|
| **PIE-V01-COLL** | Collisione sulla stessa cella | due unità pianificate verso la stessa cella, stessa priorità | Entrambe si fermano nella cella precedente; il log riporta il reason; ripetendo con una `Charge` contro un `Move`, la Charge entra e l'altra resta indietro | 🟡 **coperto headless** da `RefactorTactics.HexMove.ContestedCellStopsBoth` e `RefactorTactics.HexSim.ResolveContestedDestination` (destinazione contesa → entrambe ferme, esito indipendente dall'ordine). Al PIE resta da vedere che **a schermo** non si sovrappongano e che il combat log riporti il reason |
| **PIE-V01-ROUGH** | Costo del terreno accidentato | mappa con celle `Terrain.Rough` | Attraversare una cella accidentata consuma **2 MP** invece di 1; con 5 MP il raggio raggiungibile si accorcia di conseguenza; `Dash` e `Charge` **non** la attraversano | 🟡 **coperto headless** da `RefactorTactics.HexSim.ReachableRespectsTerrainCost` e `RefactorTactics.MatchSetup.TestArenaHasTheFeaturesItPromises` (attraversare il fango costa più dei passi percorsi). Al PIE resta da vedere che il **budget mostrato** si riduca di conseguenza |
| **PIE-V01-DASHCOVER** | Dash contro copertura alta | copertura alta sulla traiettoria del Dash | Il Dash si ferma prima della copertura oppure è invalidato in pianificazione (nessun attraversamento, nessun crash) | 🟡 **coperto headless 2026-08-06** — lo scatto e' ora **lineare** (`#46`, CP 4.5 parziale): `RefactorTactics.HexSim.DashIsLinear` verifica che un ostacolo sulla traiettoria **annulli** lo scatto (non lo aggira e non ci si ferma prima), che una cella non allineata alle sei direzioni sia rifiutata e che un layer diverso non sia mai «in linea». `HexMove.DashRefusesBlockedDestination` copre la destinazione bloccata. Al PIE resta da vedere **a schermo** che lo scatto non parta, invece di partire e fermarsi a metà |
| **PIE-V01-PUSH** | Push verso cella occupata | bersaglio con una cella occupata alle spalle | Nessuno spostamento illegale: la spinta si annulla e l'unità resta dov'è; il log spiega il motivo | ⏳ |
| **PIE-V01-ELEC** | Acqua elettrificata | acqua creata da Riva/Sprinkler + `Electrify` di Flux | La propagazione segue le celle conduttive, si ferma a **3 celle**, colpisce ogni unità **una sola volta**; ripetendo la stessa configurazione l'esito è identico | ⏳ |
| **PIE-V01-FIREWATER** | Acqua spegne il fuoco | cella in fiamme + acqua sopra | La cella di fuoco è rimossa e `Burning` cancellato dalle unità coinvolte; il fuoco non si propaga oltre | ⏳ |
| **PIE-V01-LOWCOVER** | Copertura bassa direzionale | copertura bassa su un bordo fra attaccante e bersaglio | L'attacco dal lato protetto infligge **10 danni in meno**; girando attorno e colpendo da un altro lato il danno è pieno | ⏳ |
| **PIE-V01-INTERCEPT** | Intercept protegge l'alleato | alleato entro 2 celle con `Intercept` preparato | L'intercettore **diventa** il bersaglio dell'attacco diretto; con un AoE o un hazard l'intercetto **non** scatta | ⏳ |
| **PIE-V01-FF** | Friendly fire su AoE | `CircularAoE` centrato dove c'è anche un alleato | Il danno è applicato **anche** all'alleato; l'HUD/preview lo segnala prima del lock-in | ⏳ |
| **PIE-V01-FALLBACK** | Fallback su bersaglio che si sposta | attacco diretto su un bersaglio che si muove nello stesso turno | Si applica il fallback dichiarato (`Cancel` per gli attacchi diretti): nessun colpo «inseguente», il log riporta il fallback applicato | ⏳ |
| **PIE-V01-DOOR** | Porta chiusa durante il turno | percorso che attraversa una porta chiusa da un'azione nello stesso turno | Il grafo è ricostruito: l'unità **si ferma** davanti alla porta (`Fallback.Stop`), nessun path fantasma attraverso la porta chiusa | ⏳ |
| **PIE-V01-REPLAY** | Replay dello stesso turno | `rt.Debug.DumpTurnLog` + `rt.Debug.VerifyReplay` | Rieseguendo lo stesso turno con lo stesso seed, TurnLog e checksum sono **identici**; il comando non segnala divergenze | 🟡 **coperto headless** da `RefactorTactics.HexSim.ReplayDivergenceZero` (stesso snapshot → stesso TurnLog e stesso hash). ⏳ al PIE resta il giro con i comandi `rt.Debug.DumpTurnLog` / `rt.Debug.VerifyReplay`, che non esistono ancora (CP 11.4) |
| **PIE-V01-ROSTER** | Roster dei 4 eroi | `URTHeroData` per Flux, Riva, Bastion, Vektor | Le 4 unità in campo hanno statistiche distinte (90/95/120/100 HP, 5/5/4/6 MP); il bot gestisce MP diversi senza proporre mosse illegali; asset mancante = fallback al cilindro | ⏳ |
| **PIE-V01-HUD** | HUD di partita completo | partita v0.1 avviata | Barre HP/scudo/energia, timer, fase, **turno su 12**, slot occupati (movimento/principale/reazione) e cooldown residui, tutti a schermo e coerenti col simulatore | ⏳ |
| **PIE-V01-INTENT** | Intenti alleati e certezza | due unità alleate in pianificazione | Gli intenti alleati mostrano i tre livelli **confermato / previsto / incerto**; **nessun** intento avversario è visibile in alcuna forma | ⏳ |
| **PIE-V01-LOG** | Combat log con reason code | un turno con un fallback e una modifica ambientale | Ogni voce riporta `ActionId`, priorità, coordinate assiali `(q,r,L)` e `ValidationResult`; i fallback e le modifiche ambientali sono espliciti | ⏳ |
| **PIE-V01-DEBUG** | Comandi `rt.Debug.*` | build Development o PIE | Gli 8 comandi rispondono; le celle mostrano `CellId`/`TerrainId`/`TraversalCost`/`OccupantId`/`HazardTags`/`CoverEdges`/`ChunkRevision`; **`DrawIntent` non rivela gli intenti avversari** | ⏳ |

## Sessioni di verifica consigliate

Le voci aperte non vanno affrontate una per una: molte condividono la stessa preparazione. Raggruppandole in
**cinque sessioni** si apre l'editor una volta sola per gruppo. Ordine consigliato: le sessioni **A** e **B** non
dipendono da alcun asset da creare, la **C** sì; la **D** attende M6 e la **E** la release v0.1.

### Sessione A — Editor Mode hex (nessun asset richiesto) → 12 voci
`PIE-HEX-MODE-A/B/C/D/E/F/G/H/I/J/K/L/M/N` + `PIE-HEX`, `PIE-HEX-LAYER`, `PIE-HEX-TRANS`.

1. Editor **chiuso**, ricompila il target Editor, poi apri `RefactorTactics.uproject`.
2. Crea (o apri) un livello con un `ARTHexMapActor`; assegna un `URTHexMapAsset` a `MapAsset`; `DemoRadius > 0`
   oppure `GenerateIntoAsset` per la griglia graybox → **PIE-HEX**.
3. Attiva il mode **Hex Map** dalla toolbar Modes → **PIE-HEX-MODE-A**.
4. Tool **Select**: click su celle, controlla il readout; cambia `ActiveLayer` → **PIE-HEX-MODE-B**;
   attiva `bShowOverlay` → **PIE-HEX-MODE-M**.
5. Tool **Paint**: click singolo (**C**), `Operation=Erase` (**D**), drag (**I**/**J**), `BrushRadius>0` (**K**).
   Dopo ogni prova un **Ctrl+Z** e verifica il ripristino.
6. Tool **Fill**: click su una regione (**N**), poi Ctrl+Z.
7. Tool **Arch**: gli archi esistenti si disegnano (**F**); click From + gizmo + Commit (**E**); riclicca e cambia
   tool per il ciclo di vita del gizmo (**G**); trascina in quota per lo snap cross-layer (**H**);
   `Operation=Remove` e click su un arco (**L**).
8. Con celle su due layer: `LayerView=ActiveOnly` vs `AllLayers` (**PIE-HEX-LAYER**), `Add/RemoveVerticalTransition`
   dal pannello Details (**PIE-HEX-TRANS**).

### Sessione B — Turn loop quadrato, senza asset → 2 voci
`PIE-P3`, `PIE-CP1.4`.

Avvia il PIE sul livello del demo quadrato. Pianifica due unità verso la **stessa** cella e un attacco **attraverso
un ostacolo**: il combat log deve mostrare «fermo (cella contesa)» e «nessuna linea di tiro» (**PIE-P3**). Muovi il
mouse sulla griglia per l'evidenziazione della cella (**PIE-CP1.4**).

### Sessione C — Personaggi e materiali (richiede asset creati in editor) → 6 voci
`PIE-AS5`, `PIE-SEL`, `PIE-AS2`, `PIE-AS4a`, `PIE-AS4b`, `PIE-FACING`.

Da fare **dopo** aver creato `M_TeamRing`, `M_SelectionRing`, `BP_Unit_*`, `ABP_*` e i montaggi
(guida: [`guida-animazioni-paragon.md`](guida-animazioni-paragon.md)). Una sola partita in PIE le copre tutte:
anello team (**AS5**) → selezione (**SEL**) → personaggio a terra (**AS2**) → corsa in fase Move (**AS4a**) →
montaggi nel Blast (**AS4b**) → orientamento (**FACING**).

### Sessione D — Partita su hex → 9 voci
`PIE-HEXPLAY-1..9`. **Non ancora eseguibile**: attende **M6 — Parità hex** (`roadmap-checkpoint.md`), che le
adotta come DoD del checkpoint 6.8. La mappa di prova va però preparata prima (vedi sotto): serve comunque
alla Sessione A.

### Sessione E — Contenuto della v0.1 → 17 voci
`PIE-V01-*`. **Non ancora eseguibile**: attende la release **v0.1** ([`roadmap-v0.1.md`](roadmap-v0.1.md)), che le
adotta come DoD del checkpoint **12.2** (issue [#82](https://github.com/DegrassiAaron/refactor-tactics-main/issues/82)).
Le voci si aprono man mano che le epic chiudono, non tutte in fondo:

1. Dopo **E6** (roster): `PIE-V01-ROSTER`.
2. Dopo **E4** (motore azioni): `PIE-V01-COLL`, `PIE-V01-ROUGH`, `PIE-V01-DASHCOVER`, `PIE-V01-PUSH`, `PIE-V01-FALLBACK`.
3. Dopo **E5** (reazioni): `PIE-V01-INTERCEPT`.
4. Dopo **E8** (ambiente): `PIE-V01-ELEC`, `PIE-V01-FIREWATER`, `PIE-V01-FF`.
5. Dopo **E9** (strutture): `PIE-V01-LOWCOVER`, `PIE-V01-DOOR`.
6. Dopo **E11** (HUD e debug): `PIE-V01-HUD`, `PIE-V01-INTENT`, `PIE-V01-LOG`, `PIE-V01-DEBUG`, `PIE-V01-REPLAY`.

La mappa di prova della sessione D basta anche qui, con due aggiunte: **una porta** su un passaggio strettoia
(per `PIE-V01-DOOR`) e **una copertura bassa** su un bordo esposto (per `PIE-V01-LOWCOVER`).

### Mappa di prova consigliata (serve alle sessioni A, D ed E)

Un solo asset copre quasi tutte le verifiche: esagono pieno di **raggio 4** sul layer 0, con
- 2–3 celle `bBlocksMovement` (ostacoli di movimento),
- 2–3 celle `bBlocksLineOfSight` **allineate** fra le due metà del campo (per la copertura: PIE-HEXPLAY-6),
- una superficie a costo alto (Mud/Water) per vedere il budget mordere (PIE-HEXPLAY-3),
- una piattaforma di 3–4 celle sul layer 1 collegata da **una** transizione (PIE-HEXPLAY-8, PIE-HEX-LAYER/TRANS).

Per la sessione E servono in più: una cella `Terrain.Rough`, una zona d'acqua adiacente a una superficie
conduttiva, una **porta** su un passaggio obbligato e una **copertura bassa** su un bordo esposto.

### Cosa serve da me

Non posso eseguire il PIE (richiede l'editor interattivo). Posso: compilare il target prima di ogni sessione,
prepararti la sequenza esatta dei passi, e **leggere i log** dopo — incolla il percorso di `Saved/Logs/*.log`
oppure dimmi cosa hai osservato e aggiorno le voci con l'esito (e apro un fix se emerge un difetto).

> **PIE-CP1.4**: codice fatto (`c06ef51`), resta solo la verifica interattiva (evidenziazione cella-cursore).
> Le altre voci hanno il **codice pronto**; manca solo la verifica interattiva (e, per AS.2/AS.4/AS.5, gli asset in editor).
> **Nota (PIE-HEX-MODE-E, undo)**: dopo Commit, verifica che ripetuti Undo/Redo rimuovano/ripristinino la transizione
> senza lasciare gizmo/transform orfani (l'interleaving delle transazioni del gizmo con la `FScopedTransaction` del
> Commit va osservato; l'asset resta integro perché il proxy è `Transient`).

> **PIE-BU2 · tuning pesi**: i pesi dell'utility scoring sono ora `UPROPERTY` sul `TurnManager`
> (categoria *Refactor Tactics ▸ Bot*): `WKill / WDamage / WThreat / WKiteViolation / WApproach`
> (default invariati = comportamento BU.2). Il `TurnManager` è **spawnato a runtime** dal `RTGameMode`:
> per calibrare **durante il PIE**, selezionalo nel **World Outliner** e modifica i pesi nel **Details** →
> hanno effetto **dal turno successivo** (`PlanBots` li rilegge ad ogni pianificazione), **senza ricompilare**.
> In alternativa, piazza un `RT Turn Manager` nel livello e imposta i pesi sull'istanza (il GameMode riusa
> quello esistente invece di spawnarne uno nuovo).

> **Playtest 2026-08-04** (`Saved/Logs/RefactorTactics_2.log`, partita T1→T6, **vince il team 1/bot**):
> **PIE-BU2/BU3/BU3c ✅**. Dash+attacco a segno — T1 `RTUnit_2: scatto (4,6)+attacca RTUnit_1`,
> T4 `RTUnit_3: scatto (4,3)+attacca RTUnit_0`, T5 `RTUnit_2: scatto (3,6)+attacca RTUnit_0`; nessun
> `nessuna linea di tiro`. Resta+attacca e posizionamento (`score=-140 (resta)`) osservati.
> **Non ancora esercitati** (restano da verificare): **PIE-BU2b** (tuning pesi non modificato in partita),
> il fattore **quota** (partita interamente a Layer 0, ponte non usato), e le guardie **panic/support**.

> **Playtest 2026-08-04 #2** (tuning + panic): con `WThreat=100` il Guardian resta (`(6,4) score=-140 (resta)`);
> con `WThreat=18` avanza e ingaggia (`(6,5) score=-48` → `scatto (6,6) + attacca score=344`) → **PIE-BU2b ✅**.
> Osservato anche il **panic** del kiter (`RTUnit_2: scatto difensivo (schiva)`) → **panic ✅**. Resta solo il
> **support** (Barriera del Guardian, non ancora emerso).

> **Automatizzati** (2026-08-04): **tuning** (WThreat), **panic** e **support** del bot sono ora coperti da
> **test d'integrazione headless** — `Source/RefactorTactics/Tests/RTBotPlanningTests.cpp` (smoke + panic +
> support + tuning): costruiscono un mondo 2v2, invocano `PlanBotsForTest()` e verificano le decisioni via i
> campi `Planned*`, **senza PIE**. Quindi queste tre guardie **non richiedono più verifica manuale**.
> *(Il dash-avvicinamento ora è pesato da `WThreat`: il bot rinuncia allo scatto se la cella è troppo esposta — test `PlanningDashRespectsThreat`.)*

> **PIE-PACING-1 — il cablaggio degli input** ⏳: in PIE, con `bRecordPacing = true` sul `RT Turn Manager`,
> giocare un turno selezionando due volte un'unità, impartendo un ordine, annullando un waypoint e chiudendo
> con Spazio. Poi `rt.Debug.Pacing`: il sommario deve riportare **1 turno**, `SelectionCount` ≥ 2,
> `OrderCount` ≥ 1, `UndoCount` = 1 e **nessun taglio** (il lock-in è stato manuale). Verificare che
> `Saved/RT/pacing_*.csv` esista, abbia l'intestazione e **una riga per turno giocato**.
> *(I contatori in sé sono già coperti headless da `RefactorTactics.Pacing.RecordsDecisionComposition`:
> qui si verifica solo che il controller li alimenti davvero.)*
