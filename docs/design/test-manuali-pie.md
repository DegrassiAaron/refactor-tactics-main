# Test manuali (PIE) — verifiche interattive da eseguire

> Verifiche che richiedono l'editor UE (PIE, mouse, asset) e **non** sono automatizzabili headless.
> **Complementari** ai test Automation (**77/77** verdi su `feat/bot-utility`). Parte del DoD «playtest ogni incremento» (roadmap §QA).
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
| **PIE-CP1.4** | Evidenziazione cella sotto il cursore | — | La cella sotto il mouse è evidenziata (**M1 polish, codice non ancora scritto**) | ⏳ |
| **PIE-BU2** | Bot: posizionamento via utility scoring | branch `feat/bot-utility` | In pianificazione il bot sceglie la cella pesando **minaccia/kiting** (può **restare** invece di esporsi); il combat log mostra `<Bot>: utility -> (x,y,Lz) score=N`. Il kiter (Ranger) mantiene la distanza, la mischia (Guardian) chiude, nessuno corre in celle sotto tiro. Osserva se gli score hanno senso → base per il **tuning dei pesi** (BU.3) | ⏳ |
| **PIE-BU2b** | Tuning pesi bot in editor | worktree `feat/bot-utility`, PIE attivo | Modificando `WKill/WThreat/WKiteViolation/WApproach/WDamage/WElevation` sul `TurnManager` (World Outliner → Details ▸ *Bot*) il comportamento cambia **dal turno successivo, senza ricompilare**: es. ↑`WThreat` = bot più prudente; ↓`WApproach` = mischia meno aggressiva; ↑`WElevation` = predilige le alte quote. Dettagli nella nota sotto | ⏳ |
| **PIE-BU3** | Bot: utility unica posizione/attacco | worktree `feat/bot-utility`, **dopo** refactor BU.3b | Un'unica utility sceglie fra **{resta e attacca}** e **{muoviti per posizionarti}** (l'attacco vale solo da fermo: il Blast precede il Move). Verifica: se attaccare da fermo espone troppo il bot preferisce ripararsi invece di sparare; se l'attacco **uccide** spara sempre; guardie **support/panic/dash** intatte; log `utility -> ... attacca X score=N` oppure `... score=N (resta)` | ⏳ |
| **PIE-BU3c** | Bot: dash+attacco (scatto poi colpisce) | worktree `feat/bot-utility`, **dopo** BU.3c | Se scattando raggiunge una cella da cui ha tiro e l'attacco conviene (utility), il bot pianifica **scatto + attacco** (log `utility -> scatto (x,y,Lz) + attacca X`): nel Blast (dopo il Dash) colpisce dalla cella post-scatto. **Nota**: se lo scatto è deviato da un conflitto di movimento simultaneo, l'attacco può mancare (log `nessuna linea di tiro`) — coerente coi turni simultanei | ⏳ |

> **PIE-CP1.4** dipende da codice non ancora implementato (evidenziazione cella-cursore): resta ⏳ finché non fatta.
> Le altre voci hanno il **codice pronto**; manca solo la verifica interattiva (e, per AS.2/AS.4/AS.5, gli asset in editor).

> **PIE-BU2 · tuning pesi**: i pesi dell'utility scoring sono ora `UPROPERTY` sul `TurnManager`
> (categoria *Refactor Tactics ▸ Bot*): `WKill / WDamage / WThreat / WKiteViolation / WApproach`
> (default invariati = comportamento BU.2). Il `TurnManager` è **spawnato a runtime** dal `RTGameMode`:
> per calibrare **durante il PIE**, selezionalo nel **World Outliner** e modifica i pesi nel **Details** →
> hanno effetto **dal turno successivo** (`PlanBots` li rilegge ad ogni pianificazione), **senza ricompilare**.
> In alternativa, piazza un `RT Turn Manager` nel livello e imposta i pesi sull'istanza (il GameMode riusa
> quello esistente invece di spawnarne uno nuovo).
