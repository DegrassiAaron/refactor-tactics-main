# Refactor Tactics — piano consolidato delle sessioni PIE

Aggiornamento: 2026-09-10  
Fonte normativa: repository/runtime/TurnLog. Le tavole 05–09 sono riferimenti di design e non attestano che UI, animazioni o asset siano presenti.

## Esito del consolidamento

- `PIE-HEXPLAY-6` + `PIE-VIS-SIGHTWALL` sono stati eseguiti più volte, ma restano **FAIL**: il prossimo test deve avvenire durante una vera pianificazione umana, non nell'auto-run dello scenario. Owner: [#2697](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2697).
- La Reaction Window è ora raggiungibile e i relativi widget risultano presenti; `PIE-V01-OVERWATCH`, `PIE-V01-RXPLAYBACK` e `PIE-V01-RXBRACE` sono diventati eseguibili. Owner: [#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166); blocker tecnico storico [#2723](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2723) chiuso.
- `PIE-VIS-DEFLECT` e `PIE-VIS-INTERPOSE` non vanno eseguiti prima delle cue: gli eventi esistono, ma nessuno li disegna ancora. Owner: [#2454](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2454).
- `PIE-V01-GHOSTS` non ha ancora una seduta dedicata e la feature è aperta. Owner: [#172](https://github.com/DegrassiAaron/refactor-tactics-main/issues/172), [#173](https://github.com/DegrassiAaron/refactor-tactics-main/issues/173), correzione registro/seduta [#2622](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2622).
- Gli scenari del corpus spawnano il cilindro `ARTUnit` base. Per verificare le animazioni degli eroi bisogna usare una partita 2v2 normale con i quattro `BP_Unit_*`.
- Non creare dodici montage `AM_*`: il registro corrente usa clip `Cast/Hit/Death` nel CDO e `PlaySlotAnimationAsDynamicMontage`. La issue [#288](https://github.com/DegrassiAaron/refactor-tactics-main/issues/288) va riallineata al contratto consegnato da #2450/#2448/#2444.

## Ordine operativo

### S0 — Preflight headless e congelamento della build

**Editor:** nessuna apertura.  
**Obiettivo:** evitare di spendere una seduta umana su una build incoerente.

1. Aggiornare `main`, verificare assenza di processi Unreal/MCP estranei e registrare commit + timestamp del binario.
2. Eseguire build condivisa, Automation rilevante, validatore scenari e corpus scenario.
3. Salvare per ogni scenario: `runId`, assertion, `stateHash`, warning/error e TurnLog.
4. Se un test headless è rosso, fermare la relativa sessione PIE: il PIE non serve a diagnosticare correctness già fallita.

**Issue collegate:** [#2616](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2616), [#84](https://github.com/DegrassiAaron/refactor-tactics-main/issues/84), [#1881](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881).

### S1 — Screen HUD, card selezionata e Action Dock

**Stato:** eseguire dopo la correzione di #2826; così una sola apertura giudica tutto il layer.  
**Mappa/setup:** avviare da `L_Frontend`, premere `PLAY`, poi `Home`. Non aprire direttamente `L_HexArena`.

**Controlli:**

- `PIE-V01-SCREENHUD`: zone Top/Left/Right/Bottom popolate, centro libero, round/fase/timer leggibili;
- feed a destra: massimo 12 righe del turno corrente e aggiornamento in-place quando un attacco diventa KO;
- `PIE-HUD-CARD-ZERO`: prima selezione, barra HP corretta senza frame vuoto/divisione per zero;
- click sugli slot del dock: l'azione si arma davvero e lo stato selezionato/cooldown cambia;
- debug assente nella vista giocatore; icone risolte per chiave, nessun fallback testuale.

**UI/HUD richiesti:** `WBP_RT_TacticalHUD`, `TurnHeader`, `TeamRoster`, `SelectedUnitPanel`, `ActionDock`, `ActionSlot`, `UnitCard`, `EventLog`.  
**Animazioni:** non necessarie al verdetto.  
**Asset:** risultano presenti; modifiche solo se il test fallisce.  
**Issue:** [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613), [#2764](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2764), [#1896](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1896), **blocker** [#2826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2826).

### S2 — Pianificazione umana, pointer, rifiuti e muro LOS

**Stato:** eseguibile dopo #2826; #2697 è l'owner del verdetto, non un motivo per rimandarlo.  
**Setup A:** `Visual.Hud.FirstPlayable`, giocatore umano.  
**Setup B:** `Visual.Map.SightWallIsWalkable`, ma con una finestra di vera pianificazione umana o un intento mantenuto vivo; non usare l'auto-run come oracolo della planning UI.

**Controlli:**

- `PIE-V01-POINTER`: hover, LMB, RMB/cancel, precedenza HUD→mondo, input bloccato durante playback, nessun leak;
- `PIE-V01-REFUSAL`: fuori portata e LOS bloccata hanno messaggi diversi; nemico non visto e cella vuota restano indistinguibili;
- `PIE-HEXPLAY-6` + `PIE-VIS-SIGHTWALL`: collegare linea/intento, esagono ambra e riga del feed al muro attraversabile mentre si pianifica il turno seguente;
- controllare nei log che il reason code e la cella `(q,r,L)` corrispondano al segno visto.

**UI/HUD richiesti:** intent line, blocker mark, feed giocatore, Action Dock funzionante.  
**Animazioni:** movimento base sufficiente; non sono l'oracolo.  
**Asset:** nessun nuovo asset previsto.  
**Issue:** [#2826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2826), [#2697](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2697), [#2741](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2741) chiusa; blind/unseen rimane separato in [#2792](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2792)–[#2795](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2795) e [#2870](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2870).

### S3 — Reaction Window e giunzione del playback

**Stato:** eseguibile ora sulla build che contiene i widget e i binding.  
**Mappa/setup:** partita 2v2 live; unità non-bot con Overwatch orientato verso una rotta nemica. Secondo caso con Phase in `Brace` colpita da push durante Blast.

**Controlli:**

- `PIE-V01-OVERWATCH`: una sola finestra compatta, opzioni corrette, countdown server-authoritative, timeout, input e slow-motion;
- `PIE-V01-RXPLAYBACK`: il nemico entra nella zona, il playback si ferma sull'ultimo frame e riparte dallo stesso punto, senza replay da capo o teleport;
- `PIE-V01-RXBRACE`: sospensione durante Blast, risposta/timeout, ripresa dalla fase corretta;
- misurare con 1/2/3 reazioni: numero di Manual Decision Boundaries, durata Resolution e tempi di risposta; soglia d'allarme 20 s, non target.

**UI/HUD richiesti:** `WBP_RT_FastDecision`, option widget, countdown, feedback AUTO/HOLD/FIRE.  
**Animazioni:** locomozione visibile necessaria per giudicare la giunzione.  
**Asset:** risultano presenti; verificare il consumo negli Event Graph, non solo i property binding.  
**Issue:** [#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166), [#2723](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2723) chiusa, [#1881](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881).

### S4 — Combat feedback, animazioni e simultaneità

**Stato:** attendere il merge di #2454 e #2456, poi fare una sola apertura pulita.  
**Setup A:** partita 2v2 normale con `BP_Unit_*`, non scenario harness.  
**Setup B:** `Visual.Reaction.Deflection`, `Visual.Reaction.Interposition` e scenari di status solo dopo che le cue consumano i rispettivi eventi.

**Controlli:**

- `PIE-AS4b`: `Cast` sull'attaccante, `Hit` sul bersaglio, `Death` visibile prima della rimozione;
- `PIE-HUD-DAMAGE-TOKEN`: pulse HP in tempo reale, caso cifra ≠ calo barra con scudo/Brace, privacy su nemico velato;
- `PIE-HEXPLAY-11`: eventi simultanei/Blast leggibili uno per volta, senza raffica indistinguibile;
- `PIE-VIS-DEFLECT`: difesa riuscita leggibile, non “attacco debole”;
- `PIE-VIS-INTERPOSE`: partenza verso Wraith e deviazione/arrivo su Riktor entrambe visibili;
- nascita/morte stato: pulse/fade coerenti con `StatusChanged`, senza seconda lista di stato;
- cue di sconfitta distinguibile anche se una clip non parte.

**UI/HUD richiesti:** overlay unità, token danno, barra HP, badge stato, diagnostica opzionale.  
**Animazioni:** clip CDO + dynamic montage; nessun nuovo `AM_*` richiesto.  
**Asset:** primitive semplici (linee, decal, mesh, pulse, icone); niente Niagara in v0.1.  
**Issue:** [#2453](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2453), **blocker** [#2454](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2454), **blocker** [#2456](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2456), diagnostica [#2457](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2457), animazioni [#288](https://github.com/DegrassiAaron/refactor-tactics-main/issues/288).

### S5 — Catena Unbalanced → Prone e leggibilità icone

**Stato:** eseguibile ora; aprire subito dopo S4 se non è cambiato il binario.  
**Scenari:** nell'ordine `Visual.Environment.IceSlide`, `Visual.Combat.UnbalancedAmplifiesPush`, `Visual.Movement.ProneStandUpCosts`.

**Controlli:**

- `PIE-VIS-SLIDESTATE`: lo stato nato a fine turno si nota;
- `PIE-VIS-UNBAL`: contraccolpo amplificato distinguibile da due spinte;
- `PIE-VIS-PRONE`: la cella di movimento persa si legge come costo per rialzarsi;
- `PIE-ICON-02`: Unbalanced e Prone distinguibili sia da vicino sia alla distanza reale di gioco;
- nessuna warning `ResolveIcon`.

**UI/HUD:** badge stato sopra unità.  
**Animazioni:** feedback procedurale o clip sufficiente a distinguere inclinato/orizzontale.  
**Asset:** icone già nel catalogo; la sessione decide se sono leggibili, non se risolvono.  
**Issue:** [#2378](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2378), [#2253](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2253) chiusa, [#2456](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2456) se si pretende pulse/fade.

### S6 — Bot, Ready, ritmo e match completo

**Stato:** eseguibile; sessione lunga, da registrare.  
**Mappa/setup:** `L_HexArena`/`ArenaV01`; una partita umana vs bot per Ready e leggibilità, più autobattle completa per comportamento. Per gli scenari diagnostici usare fixture riproducibili, non configurazioni improvvisate.

**Controlli:**

- `PIE-HEXPLAY-7`: kiter mantiene distanza, melee chiude, preferenza reale per copertura; legalità è già coperta;
- `PIE-V01-READY`: Ready/Unready, piano conservato, countdown solo al quorum e visibile nell'header;
- `PIE-V01-MATCHLEN`: round, durata, primo contatto, round morti;
- `PIE-PACING-1`: input, undo e modifiche al piano entrano nella telemetria;
- playtest diagnostici: Collision Choke, Dash→Blast→Move, Cover Choice, Overwatch Pressure, Objective Deadlock.

**UI/HUD richiesti:** header Ready/countdown, ghost/preview per Dash→Blast→Move quando disponibile, log causale.  
**Animazioni:** locomozione leggibile; non serve qualità finale.  
**Asset:** mappa stabile con almeno due rotte e trade-off; eventuali fixture mancanti vanno formalizzate prima della misura.  
**Issue:** [#84](https://github.com/DegrassiAaron/refactor-tactics-main/issues/84), piano storico [#2277](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2277) chiuso, epic gameplay [#2276](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2276), Ready [#2193](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2193)/[#2358](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2358) chiuse.

### S7 — Porta dell'arena

**Stato:** non aprire finché #2330 non salva una porta reale nella mappa.  
**Mappa/setup:** `L_HexArena`, porta chiusa adiacente a un'unità controllata.

**Controlli:** `PIE-V01-ARENADOOR`: porta visibile, Interact raggiungibile, apertura leggibile, passaggio e LOS aggiornati; ricaricare l'Editor per provare persistenza dell'asset.

**UI/HUD:** prompt/slot Interact e feedback di rifiuto se fuori portata.  
**Animazioni:** una rotazione/scorrimento procedurale basta.  
**Asset:** modifica a `DA_HexMap_Arena`/`L_HexArena`; richiede lane asset e poi apertura PIE pulita.  
**Issue blocker:** [#2330](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2330).

### S8 — Ghost Timeline e replay/debug

**Stato:** non aprire come sessione unica oggi; ci sono due blocchi indipendenti.

**Ghost:** dopo #172/#173 e dopo aver aggiunto la seduta mancante tramite #2622, verificare `PIE-V01-GHOSTS`: posizioni per fase, `Confirmed/Predicted/Uncertain` anche senza affidarsi solo al colore, ramo `?` delle reaction, scrubbing senza seconda simulazione.

**Replay/debug:** `PIE-V01-DEBUG` è eseguibile per la sola resa dei cinque `Draw*`; `PIE-V01-REPLAY` resta bloccato finché esistono davvero `rt.Debug.DumpTurnLog` e `rt.Debug.VerifyReplay`. Non tenere aperto l'Editor aspettando quei comandi.

**UI/HUD:** ghost path, legenda di affidabilità, controlli Play/Pause/Step/Slow/Fast, summary e dettaglio causale.  
**Animazioni:** il replay consuma eventi; la durata delle animazioni non deve diventare autorità.  
**Asset:** eventuali ghost/materiali sono presentation-only.  
**Issue:** [#172](https://github.com/DegrassiAaron/refactor-tactics-main/issues/172), [#173](https://github.com/DegrassiAaron/refactor-tactics-main/issues/173), [#2622](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2622), [#80](https://github.com/DegrassiAaron/refactor-tactics-main/issues/80), [#1881](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881), [#1937](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1937).

## Issue da aggiornare o creare

1. **Aggiornare #288**: rimuovere come lavoro attuale i dodici montage `.uasset`; collegare `PIE-AS4b` al contratto clip CDO/dynamic montage già consegnato.
2. **Aggiornare #166**: dichiarare esplicitamente che i widget/binding sono presenti e che la fase residua è S3 + misura p50/p90/Manual Decision Boundaries; sistemare il `done_when` che oggi non reclama realmente `PIE-V01-OVERWATCH`.
3. **Aggiornare #2622/editor-sessions**: creare una seduta stabile per `PIE-V01-GHOSTS` dopo #172/#173, invece di lasciarla orfana.
4. **Aggiornare #2697** con la ricetta S2: vera pianificazione umana, intento vivo, stesso verdetto per `PIE-HEXPLAY-6` e `PIE-VIS-SIGHTWALL`.
5. **Mantenere #2826 come blocker esplicito** di S1/S2: finché il dock non può armare un'azione, un click visibile non prova l'interazione.
6. **Creare una sola issue sotto #1990, se non esiste in un branch non visibile:** `Procedural Graybox Unit — silhouette, facing e locomotion cues`. Scope minimo: cilindro correttamente scalato, facing già esistente, braccia/appendici primitive, idle/walk/run/stealth/knockback leggibili. Non assorbe le Basic Combat Cues di #2453 né le animazioni degli eroi di #288.

## Strategia di apertura Editor

- **Apertura A — asset authoring:** soltanto dopo che sono pronte tutte le modifiche binarie necessarie (#2330 e gli eventuali asset delle cue). Salvare e chiudere.
- **Gate pulito:** build + Automation + validator, senza Editor.
- **Apertura B — acceptance umana:** S1 → S2 → S3 → S4 → S5, cambiando scenario senza riavviare. Un riavvio extra è ammesso solo per nuovo binario, persistenza/reload o configurazione incompatibile.
- **Sessioni lunghe separate:** S6, perché produce metriche e registrazioni; S7 dopo authoring porta; S8 solo quando cadono i relativi blocchi.

Ogni verdetto deve registrare commit, binario, mappa/scenario, setup, risultato PIE, `runId`/TurnLog quando disponibile e link all'issue owner. Un `PASS` dello scenario non equivale a un `PASS` visivo.
