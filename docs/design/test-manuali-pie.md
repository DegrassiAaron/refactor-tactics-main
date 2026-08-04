# Test manuali (PIE) — verifiche interattive da eseguire

> Verifiche che richiedono l'editor UE (PIE, mouse, asset) e **non** sono automatizzabili headless.
> **Complementari** ai test Automation (suite integrata **bot + hex** su `main`, tutti verdi). Parte del DoD «playtest ogni incremento» (roadmap §QA).
> Regola: una voce è ✅ **solo dopo** verifica reale in PIE — non «dovrebbe funzionare».

## Come eseguire
- Apri il progetto: doppio clic su `RefactorTactics.uproject` (EngineAssociation `"5.8"`; se l'editor l'ha
  risporcato a un GUID, ripristina la riga a `"5.8"`).
- **PIE**: pulsante Play (o `Alt+P`). Il `TurnManager` ha `PlanningSeconds≈30s`: premi **Spazio** per il lock-in manuale.
- I `LogRT: [RT] ...` nell'**Output Log** narrano il round (fasi, esiti).

## Checklist

| ID | Cosa verificare | Precondizione | Esito atteso | Stato |
|----|-----------------|---------------|--------------|-------|
| **PIE-AS5** | Anello di team a terra | `M_TeamRing` creato + assegnato a `TeamRingMaterial` sui `BP_Unit` | Anello **blu** (team 0) / **rosso** (team 1) sotto ogni unità, visibile dall'alto; senza `M_TeamRing` nessun anello (cilindro colorato come prima) | ⏳ |
| **PIE-SEL** | Anello di selezione (anche su skeletal) | `M_SelectionRing` creato + assegnato a `SelectionRingMaterial` sui `BP_Unit` | Selezionando un'unità compare un **anello giallo** a terra (cornice esterna al TeamRing), visibile anche quando il cilindro è nascosto (personaggio skeletal); deselezionando sparisce. Senza `M_SelectionRing`: nessun anello (fallback: resta solo l'ingrandimento del cilindro) | ⏳ |
| **PIE-P3** | Combat log mostra i reason (TurnLog) | — (funziona anche col cilindro) | Destinazione contesa → log «fermo (cella contesa)»; attacco senza LOS → «nessuna linea di tiro» | ⏳ |
| **PIE-AS2** | Personaggio skeletal appoggiato a terra | `BP_Unit_Guardian` (Gideon, `VisualZOffset=0`) → `GuardianUnitClass` | Al posto del cilindro compare il personaggio, a terra (nessun «fluttuamento») | ⏳ |
| **PIE-AS4a** | Locomozione Idle↔Run | `ABP_Gideon` + bind dei delegate (guida-animazioni-paragon) | In fase **Move** Gideon passa a `Jog_Fwd`, torna `Idle` a fine risoluzione | ⏳ |
| **PIE-AS4b** | Colpi e morte (montages) | `AM_Gideon_Cast/Hit/Death` + bind `OnAttackResolved`/`OnUnitDefeated` | Nel **Blast**: attaccante gioca `Cast`, bersaglio `Hit`; morte → `Death` | ⏳ |
| **PIE-FACING** | Orientamento al movimento | `bFaceMovementDirection=true` sul `BP_Unit` | L'unità ruota (yaw) verso la direzione di corsa; `Jog_Fwd` credibile in ogni direzione | ⏳ |
| **PIE-MP4** | Click → layer (multilivello) | mappa col ponte sopraelevato | Il click seleziona la cella del **layer giusto** (terra vs ponte) | ⏳ |
| **PIE-CP1.4** | Evidenziazione cella sotto il cursore | — | La cella sotto il mouse è evidenziata | ⏳ (codice fatto `c06ef51`) |
| **PIE-HEX** | Griglia esagonale graybox (pivot) | `ARTHexMapActor` in un livello, `DemoRadius > 0` | Griglia di celle esagonali visibile (graybox); con `MapAsset` popolato mostra quelle celle | ⏳ (branch `feat/hex-grid`) |
| **PIE-HEX-LAYER** | Filtro layer attivo (H4) | `ARTHexMapActor` con celle su ≥2 layer (es. `GenerateIntoAsset` con `ActiveLayer=0`, poi `ActiveLayer=1`) | `LayerView=ActiveOnly` mostra **solo** le celle di `ActiveLayer`; `AllLayers` le mostra tutte, impilate per quota (`LayerHeight`) → la viz non confonde i livelli | ⏳ (branch `feat/hex-grid`, H4b) |
| **PIE-HEX-TRANS** | Transizione verticale bridge/scala (H4) | due celle sovrapposte (stessi X/Y, Layer diverso), `TransitionFrom`/`TransitionTo` impostati | `AddVerticalTransition` collega i due layer (Undo/Redo ok, package dirty, validator pulito); `RemoveVerticalTransition` lo toglie | ⏳ (branch `feat/hex-grid`, H4b) |
| **PIE-HEX-MODE-A** | Editor Mode hex appare e si attiva (H5a) | modulo `RefactorTacticsEditor` compilato | Nella toolbar Modes compare «Hex Map»; attivandolo il pannello si apre senza crash (nessun tool) | ⏳ (branch `feat/hex-grid`, H5a) |
| **PIE-HEX-MODE-B** | Selezione a click nel viewport (H5b) | mode Hex Map attivo, `ARTHexMapActor` nel livello (selezionato o unico) | Tool «Select» attivo → click su una cella → esagono giallo sulla cella + `SelectedCell`/superficie/costo/blocco corretti nel pannello; cambiando `ActiveLayer` sull'actor seleziona il piano giusto (celle sovrapposte) | ⏳ (branch `feat/hex-grid`, H5b) |
| **PIE-HEX-MODE-C** | Paint a click nel viewport (H5c) | mode Hex Map attivo, tool Paint, `ARTHexMapActor` nel livello | Con `Operation=Paint`, click su una cella → esagono verde + cella creata/aggiornata (superficie/costo/blocco del pennello); `LastCell` corretto; Undo ripristina | ⏳ (branch `feat/hex-grid`, H5c) |
| **PIE-HEX-MODE-D** | Erase a click nel viewport (H5c) | mode Hex Map attivo, tool Paint | Con `Operation=Erase`, click su una cella esistente → esagono rosso + cella rimossa dall'ISM; Undo ripristina; cambiando `ActiveLayer` agisce sul piano giusto | ⏳ (branch `feat/hex-grid`, H5c) |
| **PIE-HEX-MODE-F** | Render transizioni nel tool Arch (H5c.2a) | mode Hex Map, tool Arch, `ARTHexMapActor` con transizioni | Le transizioni esistenti appaiono come linee colorate (per Kind) con freccia From->To | ⏳ (branch `feat/hex-grid`, H5c.2a) |
| **PIE-HEX-MODE-E** | Crea transizione via gizmo (H5c.2b) | mode Hex Map, tool Arch, `ARTHexMapActor` con celle su >=2 layer | Click From → gizmo → drag su To (anche altro layer, snap a cella) → Commit crea la transizione (visibile); Undo la rimuove; ClearArch annulla il pendente | ⏳ (branch `feat/hex-grid`, H5c.2b) |
| **PIE-HEX-MODE-G** | Ciclo di vita del gizmo (smoke, H5c.2b) | mode Hex Map, tool Arch, `ARTHexMapActor` nel livello | Click su una cella → compare il gizmo di traslazione; **re-click** su un'altra cella → resta **un solo** gizmo (nessun duplicato); **cambio tool** (Select) o uscita dal mode → il gizmo **sparisce** (nessun gizmo orfano in scena) | ⏳ (branch `feat/hex-grid`, H5c.2b) |
| **PIE-HEX-MODE-H** | Snap del gizmo cross-layer (H5c.2b) | mode Hex Map, tool Arch, celle su >=2 layer | Trascinando il gizmo, `To` si aggancia sempre al **centro di una cella**; alzando la quota di ~`LayerHeight` il target passa al **layer superiore** (`WorldToLayer`); nessun jitter/loop durante lo snap (guardia `bSnapping`) | ⏳ (branch `feat/hex-grid`, H5c.2b) |
| **PIE-HEX-MODE-I** | Drag-paint (H5c.3b) | mode Hex Map, tool Paint (`Operation=Paint`), `ARTHexMapActor` con `MapAsset` | Tenere premuto e trascinare dipinge più celle in una pennellata (dedup: ripassare non ridipinge); **un** Ctrl+Z annulla l'intera pennellata; click singolo = 1 cella (PIE-C invariato) | ⏳ (branch `feat/hex-grid`, H5c.3b) |
| **PIE-HEX-MODE-J** | Drag-erase (H5c.3b) | mode Hex Map, tool Paint (`Operation=Erase`) | Trascinare cancella più celle in una pennellata; un Undo le ripristina tutte; cambiare tool a metà drag non lascia transazioni aperte; **erase su celle inesistenti/vuote NON crea voci Undo né marca l'asset dirty** (transazione lazy) | ⏳ (branch `feat/hex-grid`, H5c.3b) |
| **PIE-HEX-MODE-K** | Pennello a raggio N (H5c.4) | mode Hex Map, tool Paint, `ARTHexMapActor` con `MapAsset` | `BrushRadius=0` → 1 cella (come prima); `BrushRadius=N>0` → un click dipinge/cancella l'esagono pieno di raggio N; drag dipinge fasce larghe (dedup); **un** Ctrl+Z annulla l'intera pennellata | ⏳ (branch `feat/hex-grid`, H5c.4) |
| **PIE-HEX-MODE-L** | Rimuovi arco via tool (H5c.5b) | mode Hex Map, tool Arch, `ARTHexMapActor` con transizioni | Con `Operation=Remove`, click su un arco disegnato lo rimuove (Undo lo ripristina); click nel vuoto (nessun arco entro soglia) non fa nulla; con `Operation=Add` il flusso gizmo resta invariato | ⏳ (branch `feat/hex-grid`, H5c.5b) |
| **PIE-HEX-MODE-M** | Overlay debug superfici (H5c.6) | mode Hex Map, tool Select o Paint, `ARTHexMapActor` con celle di superfici diverse | Con `bShowOverlay` attivo, ogni cella appare come esagono colorato per superficie (Water blu, Fire arancio, Mud marrone, ...); le celle bloccate hanno un esagono rosso interno; `bShowOverlay` off = nessun overlay | ⏳ (branch `feat/hex-grid`, H5c.6) |
| **PIE-HEX-MODE-N** | Secchiello / flood-fill (H5c.7) | mode Hex Map, tool Fill, `ARTHexMapActor` con `MapAsset` popolato | In Fill, click su una regione la riempie col pennello corrente; un Ctrl+Z ripristina l'intera regione; click su cella vuota non fa nulla; passando a Select/Paint con overlay si vedono i nuovi colori | ⏳ (branch `feat/hex-grid`, H5c.7) |
| **PIE-BU2** | Bot: posizionamento via utility scoring | branch `feat/bot-utility` | In pianificazione il bot sceglie la cella pesando **minaccia/kiting** (può **restare** invece di esporsi); il combat log mostra `<Bot>: utility -> (x,y,Lz) score=N`. Il kiter (Ranger) mantiene la distanza, la mischia (Guardian) chiude, nessuno corre in celle sotto tiro. Osserva se gli score hanno senso → base per il **tuning dei pesi** (BU.3) | ✅ |
| **PIE-BU2b** | Tuning pesi bot in editor | worktree `feat/bot-utility`, PIE attivo | Modificando `WKill/WThreat/WKiteViolation/WApproach/WDamage/WElevation` sul `TurnManager` (World Outliner → Details ▸ *Bot*) il comportamento cambia **dal turno successivo, senza ricompilare**: es. ↑`WThreat` = bot più prudente; ↓`WApproach` = mischia meno aggressiva; ↑`WElevation` = predilige le alte quote. Dettagli nella nota sotto | ✅ |
| **PIE-BU3** | Bot: utility unica posizione/attacco | worktree `feat/bot-utility`, **dopo** refactor BU.3b | Un'unica utility sceglie fra **{resta e attacca}** e **{muoviti per posizionarti}** (l'attacco vale solo da fermo: il Blast precede il Move). Verifica: se attaccare da fermo espone troppo il bot preferisce ripararsi invece di sparare; se l'attacco **uccide** spara sempre; guardie **support/panic/dash** intatte; log `utility -> ... attacca X score=N` oppure `... score=N (resta)` | ✅ |
| **PIE-BU3c** | Bot: dash+attacco (scatto poi colpisce) | worktree `feat/bot-utility`, **dopo** BU.3c | Se scattando raggiunge una cella da cui ha tiro e l'attacco conviene (utility), il bot pianifica **scatto + attacco** (log `utility -> scatto (x,y,Lz) + attacca X`): nel Blast (dopo il Dash) colpisce dalla cella post-scatto. **Nota**: se lo scatto è deviato da un conflitto di movimento simultaneo, l'attacco può mancare (log `nessuna linea di tiro`) — coerente coi turni simultanei | ✅ |

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
