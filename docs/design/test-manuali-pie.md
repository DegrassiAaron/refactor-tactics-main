# Test manuali (PIE) — verifiche interattive da eseguire

> Verifiche che richiedono l'editor UE (PIE, mouse, asset) e **non** sono automatizzabili headless.
> **Complementari** ai test Automation (branch `feat/hex-grid`: 93/93 verdi). Parte del DoD «playtest ogni incremento» (roadmap §QA).
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

> **PIE-CP1.4** dipende da codice non ancora implementato (evidenziazione cella-cursore): resta ⏳ finché non fatta.
> Le altre voci hanno il **codice pronto**; manca solo la verifica interattiva (e, per AS.2/AS.4/AS.5, gli asset in editor).
> **Nota (PIE-HEX-MODE-E, undo)**: dopo Commit, verifica che ripetuti Undo/Redo rimuovano/ripristinino la transizione
> senza lasciare gizmo/transform orfani (l'interleaving delle transazioni del gizmo con la `FScopedTransaction` del
> Commit va osservato; l'asset resta integro perché il proxy è `Transient`).
