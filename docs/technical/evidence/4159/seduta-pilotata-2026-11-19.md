# `PIE-HUD-AP-PIP`, `-PREVIEW`, `-PLAN-COUNT` e `-STACKS`: seduta pilotata, 2026-11-19

Questa seduta misura a schermo le quattro voci della HUD degli Action Point:
- `PIE-HUD-AP-PIP`, i pip della dock ([#4150](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4150));
- `PIE-HUD-AP-PREVIEW`, il costo del gesto sotto il cursore ([#4155](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4155));
- `PIE-HUD-AP-PLAN-COUNT`, il numero delle copie sullo slot ([#4159](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4159), HUD-AP.5a);
- `PIE-HUD-AP-STACKS`, un ghost per voce nell'ordine del resolver ([#4159](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4159), HUD-AP.5b).

Il metodo è quello della [seduta del 2026-11-13](../4091/seduta-pilotata-2026-11-13.md): una finestra `-game` pilotata da fuori, con input sintetico. Misura la meccanica dei criteri, non l'occhio dell'autore. Il giudizio a schermo l'autore l'ha dato sulle tavole, ed è riportato più sotto con le domande a cui risponde.

⚠️ **Chi misura ha scritto il codice di HUD-AP.5a e HUD-AP.5b, e il testo di `PIE-HUD-AP-STACKS`.** Non è un verdetto indipendente. La misura avviene dopo il merge, su un commit dichiarato.

## Condizioni

- **Commit:** `main` = `a862e1973` (PR [#4162](https://github.com/DegrassiAaron/refactor-tactics-main/pull/4162)). Il clone principale era su `main` a `068d93dd5`, pulito salvo i file non tracciati dell'autore: è stato portato a `a862e1973` in fast-forward, e l'Editor compilato lì (`Build.bat RefactorTacticsEditor Win64 Development`, exit 0). Il log di ogni giro conferma il modulo: «modulo compilato il Nov 19 2026 alle 11:49:54».
- **Motore:** nessun altro processo Unreal durante la seduta (`CommandLine` di `UnrealEditor`, `UnrealBuildTool` e `dotnet` letta prima della build; i `dotnet` presenti erano il server di VS Code e i nodi di MSBuild).
- **Finestra:** `UnrealEditor.exe … L_DevSandbox -game -windowed -ResX=1920 -ResY=1080 -RTScenario=<Id> -RTPlanningSeconds=3600`. Catture `PrintWindow` a 1920×1080, mai massimizzata (`zoomed=False` in ogni cattura).
- **Camera:** `Tab`, otto tacche di rotella verso la scena, due indietro; per STACKS anche `N` e `F` su Gadget. La sequenza ha dato la stessa inquadratura in ogni giro.
- **Banchi:** non versionati, in `Saved/RTLab/Scenarios/` durante la seduta, poi tolti:
  - `Lab.HudAp.Pip`: `Visual.Input.PcGym` con Riktor spostato da `(1,3,0)` a `(-1,3,0)`, a portata 4 di Muiren. In `PcGym` l'unico nemico sta a 5 e l'attacco base non lo raggiunge (log: «fuori portata (max 4)»). Budget di default, 6;
  - `Lab.HudAp.Stacks`: Gadget in `(-4,2,0)` con `actionPointBudget` 10, Riktor in `(-1,3,0)` e Muiren in `(-2,4,0)` nemici. Il budget alto serve perché il piano della voce (Prep, due principali, movimento a due tratti) costa più di 6.
- **Giri:**
  - `p1`: interrotto. Durante il giro sono comparse righe che l'assistente non aveva mandato (`T`, due richieste di fuoco, una «rotazione 0» nel piano di Muiren): la tastiera dell'autore scriveva nella finestra che lo script teneva in primo piano. L'assistente si è fermato e ha chiesto; l'autore ha risposto «1» (pilota l'assistente).
  - `p2`: `PcGym`, abbandonato quando l'attacco base non ha trovato un bersaglio a portata.
  - `p3`: `Lab.HudAp.Pip`, abbandonato: il click sul corpo di Riktor prendeva il pavimento (vedi «Trappole»).
  - `p4`: `Lab.HudAp.Pip`. PIP (a)–(c), PREVIEW (a)–(c), PLAN-COUNT (a), (b), (d) in `Planned`.
  - `p5`: `Lab.HudAp.Pip`. PLAN-COUNT (c) e (d) in `Invalid`.
  - `p6`: `Lab.HudAp.Pip`. PREVIEW (f).
  - `p7`: `Lab.HudAp.Pip`. PREVIEW (d).
  - `s1`, `s2`: `Lab.HudAp.Stacks`. STACKS, con le due voci di Prep in due ordini.

## Il giudizio dell'autore

Quattro tavole, una per voce: `T1_pip.jpg`, `T2_preview.jpg`, `T3_plan_count.jpg`, `T4_stacks.jpg`. Ognuna porta sotto le sue domande e cosa non mostra.

La risposta, alla lettera: «ok a tutte le tavole». È **una risposta sola per quattro tavole**, letta come nessuna obiezione a ciascuna delle domande scritte sotto di esse (elencate qui sotto per voce). Non decide il rilievo su PREVIEW (d), che la tavola 2 presentava come un fatto e non come una domanda: resta un FAIL del criterio scritto, in attesa della decisione dell'autore su come trattarlo.

## Esiti

### `PIE-HUD-AP-PIP` (tavola 1, giro `p4`, Muiren)

| Criterio | Esito |
|---|---|
| (a) piano vuoto, sei pip spenti | **PASS** |
| (b) Move tracciato: due ambra; con l'attacco base su Riktor: quattro | **PASS**. Log `Piano: BP_Unit_Muiren_C_0 -> 1 waypoint (costo 1/20)` e `usa Getto in pressione su BP_Unit_Riktor_C_0` |
| (c) l'ispezione di un nemico toglie i pip, la riselezione li rimette | **PASS**. Click su Riktor senza azione armata: la dock mostra il kit di Riktor e nessun pip; riselezionata Muiren tornano i due ambra |
| (d) i pip accanto alla lettura del movimento, senza sovrapporsi all'indicatore del piano | Domanda della tavola 1: «i pip stanno "accanto alla lettura del movimento"? Fra "Still/Move x1 M" e "AP" c'è il selettore "Piano 1 di 2"». Risposta: «ok». Nessuna sovrapposizione a 1920×1080 |

Domande della tavola 1: «(a), (b), (c) si leggono come dice la voce?» e la (d) sopra.

### `PIE-HUD-AP-PREVIEW` (tavola 2, giri `p4`, `p6`, `p7`, Muiren)

| Criterio | Esito |
|---|---|
| (a) piano vuoto, cursore su una cella del ventaglio: due pip bianchi; fuori dalla cella si spengono | **PASS** |
| (b) con un percorso tracciato, il cursore su un'altra cella non accende bianchi | **PASS** |
| (c) cursore su uno slot pagabile: i pip del costo; su uno slot già nel piano: nessuno | **PASS**. Slot 1 (attacco base): +2 bianchi prima, nessuno dopo averlo pianificato |
| (d) piano a budget pieno senza movimento: ventaglio rosso cupo, «Non si muove: …» in basso al centro, due pip rossi, il click non traccia | ❌ **FAIL in parte.** Piano: attacco base su Riktor, Scia fluida (scatto) su `(-3,3,0)`, Fuga hazard (reazione) = 6/6. Il ventaglio è rosso cupo e il click è rifiutato: `Cella (-2,2,L0) rifiutata per BP_Unit_Muiren_C_0: col percorso (costo 1) il piano spenderebbe 8 AP su 6`. **Ma col cursore sulla cella non compaiono né «Non si muove: …» né i pip rossi**, provato con la reazione armata e poi con lo scatto armato |
| (e) i colori si distinguono | Domanda della tavola 2 su bianco/ambra e rosso cupo/rosso dell'attacco: «ok». Non mostrati accanto: il rosa del tiro bloccato e il marcatore di blocco |
| (f) con l'attacco nel piano, il ventaglio verde fino ai tratti pagati e rosso cupo nel tratto dopo; pip bianchi su una cella che apre un tratto nuovo | **NOT RUN come scritto.** Con un'azione a bersaglio armata la mappa mostra la portata e non il ventaglio: per vederlo è stata aggiunta Guardia (piano 3/6). Il ventaglio è tutto verde, a tratto pieno vicino e tratteggiato lontano; l'arena ha raggio 4 e il tratto non pagato non ci cade dentro |

**Perché (d) non compare.** La lettura viene da `ARTPlayerController::HoveredMoveRefusal`, che risponde solo se `IsMoveCellUnderCursor` è vero. Questo chiede il contesto `Planning` o `Pathing` e il click del waypoint aperto (`ARTUnit::WaypointClickIsOpen`). Con un'azione armata che non è uno scatto il contesto è `Targeting` (`URTPointerLibrary::DeriveContext`); con uno scatto armato il click del waypoint è chiuso. E in un piano pieno qualcosa è sempre armato: disarmare (`ArmKitAbility` con lo stesso indice, il destro) toglie la voce dal piano. Due fonti concordano: il codice e lo schermo in due stati armati diversi. Non è stato corretto.

Domande della tavola 2: «(a), (b), (c) si leggono come dice la voce?» e «(e) il bianco del gesto si distingue dall'ambra dell'impegnato? Il rosso cupo del ventaglio (d) dal rosso dell'attacco?».

### `PIE-HUD-AP-PLAN-COUNT` (tavola 3, giri `p4` e `p5`, Muiren)

| Criterio | Esito |
|---|---|
| (a) due copie: «×2» in ambra in alto a destra; con una copia sparisce | **PASS**, con Guardia fra le azioni di Prep (`aggiunge Guardia al piano (voce di Prep)` due volte, dietro l'attacco base come principale). ⚠️ Con l'attacco base, l'altro esempio della voce, non si può: un secondo click con un'azione a bersaglio riarma la stessa voce e non ne aggiunge una (vedi STACKS) |
| (b) il numero sopra l'icona, non copre tasto né nome | Domanda della tavola 3: «il ×2 si legge? Sta sopra la scritta PREP e sulla barra colorata di fase: copre qualcosa?». Risposta: «ok» |
| (c) con due copie di un'azione con ricarica: avviso «… (2ª copia)» e tooltip «Piano illegale: … (2ª copia)» | **PASS**, con Guardia di marea (ricarica 2). Avviso: «Guardia di marea (2ª copia)», CRITICAL, «l'azione ha una ricarica ed e' gia' nel piano». Tooltip: «Piano illegale: l'azione ha una ricarica ed e' gia' nel piano (2ª copia)» |
| (d) il numero a sinistra dell'angolo di `Planned`, leggibile in `Planned`, `Invalid` e `Warning` | `Planned` e `Invalid` visti, giudicati «ok». **`Warning` NOT RUN**: nessun avviso di quel livello prodotto in seduta |

Per vedere Guardia in `Planned` serve che lo slot armato sia un altro: dopo le due Guardie, il tasto `1` riarma l'attacco già nel piano senza cambiarlo.

### `PIE-HUD-AP-STACKS` (tavola 4, giri `s1` e `s2`, Gadget)

| Criterio | Esito |
|---|---|
| (a) due principali del Blast su due bersagli, una voce di Prep, un movimento a due tratti: un ghost per voce | **NOT RUN come scritto.** Due principali del Blast oggi non si pianificano a mano: la seconda **sostituisce** la prima. Misurato: con Impulso ad arco su Riktor nel piano, Scarica lineare su Muiren ha dato `usa Scarica lineare su BP_Unit_Muiren_C_0`, la principale è diventata Scarica lineare e gli AP sono rimasti 8. Il codice dice lo stesso (`ARTUnit::DeclarePlannedMainAction` riscrive la principale; il gesto della lista, `ARTUnit::DeclarePlannedAction`, il controller lo chiama solo per le azioni di Prep su sé stessi), e il piano di E38 lo dichiara: [`e38-checkpoint-d613-2026-11-14.md`](../../../roadmap/plans/e38-checkpoint-d613-2026-11-14.md) riga 1240, «un gesto che la aggiunga è dell'owner dell'input (E48)». ⚠️ **La premessa sbagliata è nel testo della voce**, scritto in HUD-AP.5b. Giocata la parte raggiungibile: movimento a due tratti (`costo 6/30`, «Sprint ×2»), Impulso ad arco su Riktor, Guardia e Irrigidimento come voci di Prep = 8 AP. Il ghost del movimento è uno solo, sull'ultimo waypoint (compare lì e si sposta con lui); sulla cella di Gadget la pila cresce con le voci |
| (b) i ghost sulla stessa cella si impilano in verticale e nessuno ne copre un altro | Domanda della tavola 4: «sulla cella di Gadget i ghost si impilano in verticale: ti sembra che uno copra l'altro? Dall'alto si vede solo la faccia superiore e i bordi sottili di quelli sotto». Risposta: «ok». I ghost sono tutti gialli (certezza) |
| (c) lo stesso piano dichiarato in un altro ordine dà gli stessi ghost | **NOT RUN come scritto** (servono due principali). Fatto con le voci di Prep: ordine A (Guardia, Irrigidimento) nel giro `s1`, ordine B (Irrigidimento, Guardia) nel giro `s2`. Domanda: «con le due voci di Prep dichiarate in ordine inverso la pila è la stessa?». Risposta: «ok». Differenza media assoluta fra le due catture: circa 5 su 255 sulla cella di Gadget, dove c'è il personaggio animato, e 1,4 sulla scena |
| (d) area e linea di mira della sola principale a fuoco | Banale con una principale sola: non discrimina |

## Trappole di questa seduta

- **Il click sul corpo prende il pavimento dietro** (già misurato il 2026-11-13, #4101). Con un'azione a bersaglio unità armata, un click sul pavimento ricade nel waypoint (`HandleClickOnCell`, «ogni altro contesto ricade sul waypoint come prima»): due giri hanno messo un waypoint sulla cella di Riktor invece del colpo. Il punto giusto si trova passando il cursore e leggendo il tooltip («Riktor» invece di «Pavimento»).
- **Il destro disarma e toglie**: `Back: 'BP_Unit_Muiren_C_0' disarma l'azione pianificata` ha tolto l'attacco dal piano. Per cambiare lo slot a fuoco senza toccare il piano si riarma un'azione già nel piano.
- **`N` non cambia lo slot armato**: riselezionando l'unità, l'azione armata resta quella di prima.
- **PowerShell non distingue `$G` da `$g`**: una variabile `$G='p3'` ha sovrascritto il percorso dello script, e nessun gesto è partito.

## Osservazioni (non giudizi)

- Durante l'ispezione di Riktor e dopo la riselezione di Muiren, il pannello ISPEZIONE a destra è rimasto aperto su Riktor.
- Il pannello del piano a sinistra elenca PRINCIPALE, MOVIMENTO e REAZIONE: le voci di Prep aggiunte non vi compaiono.
- Con la camera avvicinata (`s1`), il segno del waypoint finale è disegnato sopra la fascia sinistra dell'HUD, fuori dal riquadro della scena.
