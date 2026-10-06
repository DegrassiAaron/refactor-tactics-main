# Spec panel — le aree tattiche di contesto: cosa vedrò, cosa colpisco, fin dove arrivo

> `SNAPSHOT` · fotografia del 2026-10-06: vale finché è l'ultima misura del suo oggetto.

**Data**: 2026-10-06 · **Misurato su**: `main` `903154a41` · **Modalità**: discussion · critique · **Focus**: requirements · architecture · testing
**Panel**: Wiegers (requisiti) · Cockburn (attore/goal) · Adzic (esempi) · Crispin (testabilità) · Fowler (confini) · Nygard (failure mode), in tre coppie, più un verificatore avversario; preceduto da una ricognizione mirata (tre lettori più un critico)

> Referto della sessione chiesta dall'autore il 2026-10-06: *«voglio vedere anche le altre aree, in base al
> contesto. l'area che vedro' se finisco il movimento qua, che area colpisco se click qua, dove riesco a colpire,
> etc...»*.
>
> **Allarga** il referto gemello [`raggio-di-mira-spec-panel-2026-10-06.md`](raggio-di-mira-spec-panel-2026-10-06.md),
> che resta la specifica dell'area «dove riesco a colpire» (R) e di cui questo referto richiama i requisiti per nome
> (`v1 FR-n`, `v1 DR-n`).
>
> **Non è un owner.** Le aree le implementano [#3420](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3420) (V, vista dalla destinazione),
> [#3512](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3512) (E, area sotto il cursore — fetta di [#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705)) e [#3507](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3507) (R, la portata). I
> prerequisiti comuni sono [#3509](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3509), [#3510](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3510) e [#3511](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3511); [#3508](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3508) (canale di resa
> in Shipping) è un difetto che tutte le aree ereditano. ⛔ Nessuna riga di codice è cambiata con questo referto.
>
> Convenzione: niente totali che cambiano da soli. Gli esempi di §8 sono ricavati leggendo il codice e **non
> eseguiti**: un test headless li conferma o li smentisce. Le decisioni (DR) sono numerate in uno spazio condiviso col
> referto gemello: DR-9…DR-13 e DR-15 qui, DR-1…DR-8 e DR-14 là. I requisiti dell'area E si chiamano `AE-n`, per non
> confondersi con gli esempi `E1…E16` del gemello.

---

## 0. Richiesta e lettura

Utente, 2026-10-06: *«voglio vedere anche le altre aree, in base al contesto. l'area che vedro' se finisco il
movimento qua, che area colpisco se click qua, dove riesco a colpire, etc...»*.

| Domanda del giocatore | Area | Owner |
|---|---|---|
| «cosa vedrò se finisco il movimento qui?» | **V** — vista prevista dalla destinazione | #3420, da allargare |
| «che area colpisco se clicco qui?» | **E** — area d'effetto sotto il cursore | #3512, fetta di #705 (riga `Hover = Preview` di §5.1) |
| «dove riesco a colpire?» | **R** — raggio di mira | #3507, famiglia #1944 (referto gemello) |
| «etc.» | §6 | vari, per lo più DEFERRED |

⚠️ **Lettura da confermare (Q1).** «L'area che vedrò se finisco il movimento qua» si legge come **vista** (V),
ed è ciò che l'autore ha già chiesto in #3420 il 2026-09-30: *«quando si pianifica il movimento, alla fine del
movimento si decide il facing e si ha una preview della zona che diventa visibile»*. L'altra lettura —
**dove potrò colpire** da lì — vale solo il turno dopo, perché il Blast risolve prima del Move (ADR-0003): è
DEFERRED (DR-13) finché l'autore non sceglie.

## 1. Obiettivo

In ogni momento della pianificazione la board mostra in primo piano l'area che risponde **alla decisione in
corso**, e solo quella (#1943: *«Render what is relevant to the current decision, not everything the client
knows»*). Ogni area viene dalle stesse funzioni che decidono l'esito, e quando è un'ipotesi lo dichiara.

## 2. Contesti

Il contesto non è uno stato nuovo: è `GetPointerContext()`, derivato a ogni chiamata con precedenza fissa
`Modal > ResolutionPlayback > IdleSelection > Facing > Targeting > Pathing > Planning`. La chiave delle righe di
§3 è la terna (`ERTPointerContext`, `TargetKind`, fatti del piano).

| Contesto del puntatore | Contesto #1943 | Note verificate |
|---|---|---|
| `IdleSelection` | Default map | |
| `Planning`, `Pathing` | Movement Mode | un piano fatto **solo di scatto** resta `Planning` (`Pathing` vuole un waypoint); una mobilità rapida armata non produce `Targeting` |
| `Targeting` (+ `TargetKind`) | Ability Targeting | dopo la dichiarazione del bersaglio l'azione resta armata e il contesto resta `Targeting` (verificato sul percorso cella); armare l'Overwatch dà uno stato persistente `Targeting` con azione su se stessi |
| `Facing` | — (riga da aggiungere a #1943) | esiste **solo con l'unità ferma**: in marcia il verso si sceglie col secondo click sull'esagono finale, restando in `Pathing` (#291) |
| `ResolutionPlayback`, `Modal` | — | nessuna area di pianificazione |
| — | Vision Inspection, ispezione nemica (#2597) | **nessun contesto del puntatore** le produce: DEFERRED |

## 3. Matrice contesto → aree

Al più **un** Primary e **due** Secondary per riga (#1943). **Una sola V alla volta.** Fino a quando #1943 non
porta i pattern dei Secondary, un Secondary si disegna come **solo perimetro** (`ExtractBoundaryEdges`, OVL-02).

| Contesto | Primary | Secondary | Owner | Stato |
|---|---|---|---|---|
| IdleSelection | esagono di hover | — (Hazard/Objective DEFERRED) | #1614 | ✅ |
| Planning, nessuna destinazione | ventaglio `Movement` | — | #1941 | ✅ |
| Planning/Pathing con destinazione, cursore fuori dall'esagono finale | ventaglio `Movement` | `PathTrace` · **V** dalla destinazione | #1941 · #3420 | ✅ · 🟡 |
| Pathing, cursore sull'esagono finale | **V** col settore puntato (solo se legale) | `PathTrace` · ventaglio | #3420 | 🟡 |
| Pathing col verso già dichiarato | ventaglio `Movement` | `PathTrace` · **V** col verso dichiarato | #3420 | 🟡 |
| Facing (unità ferma) | **V** dalla propria cella col settore puntato (solo se legale) | settori pieno/barrato (CURRENT OPTIONAL, #705) | #3420 | 🟡 |
| Targeting, cursore non su un bersaglio accettato | **R** (il ventaglio sparisce: decisione d'autore in #3507) | — | #3507 | 🟡 |
| Targeting, cursore su un bersaglio accettato | **E** | R, solo perimetro | #3512 | 🟡 |
| Targeting, bersaglio già dichiarato | anteprima A (E al suo posto sull'hover di un altro bersaglio valido, DR-7 estesa) | — | #3512 (E sull'hover); A è già su `main` | ✅ A |
| ResolutionPlayback, Modal | nessuna (un solo reset) | — | #3511 (reset) · #3510 (armo nel playback) | difetto aperto |

In `Targeting` V è **spenta** — proposta del panel, non ancora una decisione (Q7, da registrare su #1943) —: R è
Primary e i Secondary non bastano. Con una
mobilità rapida armata (contesto `Planning`/`Pathing`) non c'è V all'hover, e le destinazioni di scatto sono
DEFERRED.

## 4. Area V — «cosa vedrò se finisco il movimento qui»

- **V-1 Formula.** `V(D) = VisibleCells(Map, FRTPerceiver{ Cell = D, Facing = F(D), VisionRange = Unit->VisionRange })`.
  `VisibleCells` (`Perception/RTPerceptionLibrary`) è pura, legge solo mappa e posa — **nessuna unità entra** — e
  somma due canali: arco frontale di 120° (`HexCone`) fino a `VisionRange`, e consapevolezza a 360° entro
  `CloseAwarenessRange` (2), entrambi con LOS e cap del Fumo (`ContactHolds`). I candidati vengono da tutti i
  piani: la vista è cross-layer, la mira no (D-393). Con `VisionRange <= 0` resta l'anello di 2 (il docstring di
  `FRTPerceiver` dice il contrario: deriva da correggere). `VisionRange` è quello dell'unità, mai il default.
  ⛔ Non `HasLineOfSight` a 360° come imposta #3420: l'anteprima mentirebbe a ogni turno.
- **V-2 Destinazione `D`** — una sola V per unità selezionata, con questa precedenza:
  1. `Pathing` col cursore sull'**esagono finale**, oppure contesto `Facing` (unità ferma): `D = FacingCellFor(Unit)`
     e il facing è il **settore sotto il puntatore**, proiettato sul pavimento come fa
     `TryHandleFacingClickUnderCursor`;
  2. *(seconda fetta, DR-9)* `Planning`/`Pathing` col cursore su una cella di `ReachableCellsAfterPlan` — il set già
     tenuto dall'ultimo refresh, **mai ricalcolato all'hover** (criterio di #711) — senza verso dichiarato e senza
     mobilità armata: `D = HoveredCell`;
  3. esiste una destinazione pianificata diversa dalla cella attuale (waypoint **o scatto**: `FacingCellFor`
     restituisce la cella dello scatto quando lo scatto si applica, DR-11): `D = FacingCellFor(Unit)`;
  4. altrimenti nessuna V.
  Una cella non raggiungibile sotto il cursore non produce mai V: si passa alla sorgente successiva.
- **V-3 Facing d'arrivo `F(D)`.** Lo restituisce **una** funzione pura pubblica,
  `URTFacingLibrary::ArrivalFacing(Style, Path, Current, bDeclares, Declared, Budget)`, composta dalle primitive
  che il resolver già applica a fine Move (`FacingFromPath` sull'**ultimo** passo, `LegalFacings`,
  `TryApplyDeclaredFacing`). Ingressi:
  - sorgente 3: stile e rotta da una versione pura di `PlannedMovementForFacing` (oggi metodo del controller),
    più il verso dichiarato;
  - sorgente 2: rotta `{FromCell(D), D}` — basta l'ultimo passo, `ProbePathTo` non serve — e **nessun** verso
    dichiarato, che vale solo per la rotta per cui è stato dichiarato;
  - sorgente 1: `Declared` = il settore puntato; se non è in `LegalFacings`, `F` è il verso che
    `TryApplyDeclaredFacing` lascerebbe davvero, e il settore si rende barrato. ⛔ **Mai la vista di un verso
    illegale.**
  Non si parte da `ArrivalFacingOf` del bot, che ignora verso dichiarato e stile dello scatto. Il **resolver non si
  tocca**: è l'oracolo del test di equivalenza (AC-V1). Il bot può passare alla funzione nuova come follow-up.
- **V-4 Insieme mostrato** (DR-10): la vista **piena dell'unità**. Il «guadagno» rispetto alla vista attuale della
  squadra mentirebbe, perché anche gli alleati si muovono nello stesso turno; «meno esplorato» nasconderebbe i
  corridoi noti e mostrerebbe solo l'ignoto, cioè la forma di massimo leak. Il risalto delle celle nuove è
  DEFERRED. ⚠️ Si scosta dalla lettera della DoD di #3420 («le celle che si aprirebbero»): decide l'autore (Q2).
- **V-5 Cosa promette, e cosa no.** V è la vista che l'unità avrà al refresh di Planning del **turno dopo**, se il
  movimento non è conteso. Non entrano: le superfici create dal Blast di questo turno (anche della propria squadra)
  o scadute al Cleanup; un movimento conteso o interrotto; una dichiarazione di verso rifiutata a fine Move
  (`DeclarationRejected`). È la vista **dell'unità**, non della squadra. Lungo la rotta il resolver calcola la vista
  posa per posa, ma la scrive **solo** in `ExploredCells` (verificato: `AccumulateExploredFromTransit`, «VisteInTransito»):
  V dice «vedrai da lì», non «esplorerai lungo la rotta» né «chi ti vedrà».
- **V-6 Certezza.** V è sempre `Predicted`.
- **V-7 Privacy.** Solo terreno. `VisibleCells` legge muri, porte e Fumo — anche dinamico, che sta nella mappa —
  dentro le celle mai osservate: è la stessa funzione che alimenta la conoscenza reale. Quindi la scelta di DR-2
  entra come **parametro di vista della geometria** con default = mappa autorevole: l'autorità resta invariata, e
  click, R e V ricevono la stessa vista nello stesso commit. Qui il leak non è un effetto collaterale: è la funzione
  stessa dell'anteprima.
- **V-8 Significato e resa.** Un valore nuovo in coda, `VisibleArea` (nome da fissare), condiviso con la vista reale
  di #1944 (`Confirmed`) e con V (`Predicted`). ⛔ Non `Vision`, che è la **linea** di tiro di #2742. D-376
  dichiara l'enum «STABILE» fino a #1614 senza distinguere aggiunte e rinomine; il precedente di `Vision`, aggiunto in
  coda *dopo* D-376 (`90a99f5bb` è antenato di `5149867ff`), mostra che le aggiunte in coda col loro produttore sono
  state accettate. Si legge così, ed è da confermare dall'autore. V porta un proprio indicatore del verso su `D`: la rotazione della
  mesh guarda il **primo** passo (`PreviewPlannedFacing`) e non è il verso di V.
- **V-9 Multilivello** (DR-15). Il modello resta cross-layer e non si filtra mai. Il disegno segue la modalità di
  piano corrente, senza nuovo cambio di piano automatico, con un segnale non cromatico quando V ha celle su altri
  piani. Prima di decidere: misurare quante celle di V cadono su un altro piano (esempio V-E).

## 5. Area E — «che area colpisco se clicco qui»

- **AE-1 Quando compare.** E compare **se e solo se il click sarebbe accettato**, deciso da un predicato **positivo** —
  per esempio `CanAcceptTargetUnderPointer()` — che replica le uscite d'accettazione di `HandleTargetCell` e
  `HandleClickOnUnit`: contesto `Targeting`, azione pronta (`CanUseAbility`), cella sotto il cursore valida; per kind
  `Cell` verdetto `Ok` dalla stessa origine del click; per kind `Unit` un nemico vivo, **noto** e non comandabile sulla
  cella, con verdetto `Ok`. Allora vale `MakeBlastPreview` col bersaglio sotto il cursore. Così E non dipende né dal
  produttore di R né da DR-2, e può arrivare prima di R.
  ⛔ **Non** `RefusalUnderPointerForArmed() == None`: quella funzione risponde `None` anche fuori da `Targeting`, con
  l'azione in ricarica, sulla cella non valida, su una mobilità rapida, e — per kind `Unit` — su ogni cella senza un
  nemico noto, cioè proprio dove il click non pianifica.
  Altrimenti nessuna area d'effetto, e il rifiuto: oggi lo slot lo riduce a un booleano, mentre il controller ha già
  il motivo (`ERTTargetRefusal`); il testo del motivo è il residuo di #172. **v1 DR-1 si estende** (#3509): il
  predicato e il produttore di E consumano la funzione d'origine unica.
- **AE-2 Bersaglio unità.** Solo su un nemico **noto** (`IsKnownToObserver`). ⛔ Il bersaglio non si risolve mai da
  `Units` o `Snapshot.Units` senza la guardia: un nemico ignoto deve dare un'uscita identica alla cella vuota. Su
  un'unità nemica E è `Predicted`, perché il resolver mira la cella del bersaglio al Blast, dopo il suo eventuale
  scatto. Un contatto noto solo come ultima posizione: Q8.
- **AE-3 Forme.** Single = la cella; Line = `HexLine(O, T)` senza l'origine, perforante; Area = `HexArea(T, r)`.
  L'adattatore di presentazione scarta le celle fuori mappa, senza toccare `HexHitCells`.
- **AE-4 Separata dall'anteprima A.** E è un'area propria (`Source` hover) e **non scrive mai** i setter dell'anteprima
  del bersaglio dichiarato. A bersaglio dichiarato, l'hover su un altro bersaglio valido mostra E al posto di A — è
  l'anteprima del click che ridichiarerebbe — e A torna quando il cursore esce. Il dato di A resta invariato
  (DR-7 estesa, Q5).
- **AE-5 Privacy.** E non apre canali nuovi: la sua forma non legge né mappa né unità, e il verdetto che la accende è
  lo stesso che lo slot `Invalid` espone già all'hover (D-459).
- **AE-6 Owner.** [#3512](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3512), fetta di #705 (la colonna Hover di §5.1 non aveva fette dichiarate), con #172
  come dipendenza.

## 6. «Etc.» — scope

**Tornata corrente**: E, R (v1), V nelle sorgenti 1 e 3. **Seconda fetta**: V all'hover (sorgente 2), dopo la misura
di costo e la decisione DR-2. **CURRENT OPTIONAL**: resa dei settori pieno/barrato sull'esagono finale (#705), utile
perché V non compaia su settori illegali.

**DEFERRED**, con l'owner dove esiste:
- portata dalla destinazione ipotetica (DR-13) — vale il turno dopo;
- zona Overwatch all'armo — riuserà `MakeSuppressiveZone`, dipende dal facing d'arrivo, nessun owner;
- destinazioni di scatto all'armo di una mobilità — nessun owner;
- regioni nemiche all'ispezione (#2597) e Vision Inspection (#1943) — manca il contesto del puntatore;
- disegno del percorso sondato all'hover (#705, `ProbePathTo`): non è prerequisito di V;
- linea di tiro all'hover (#2742);
- terreno esplorato lungo la rotta; «chi mi vede lungo la rotta» (solo contatti noti, `ViewForTeam`);
- Hazard e Objective (#1944); attenuazione del ventaglio (#1943); risalto delle celle nuove in V.

## 7. Requisiti trasversali

- **CX-1 Nessuna sede nuova.** L'hover è un **ingresso** della funzione che produce le aree, non un `Meaning`. Le aree
  riusano i significati esistenti, distinte da `Source` e `Certainty`: E → `Attack`/`FriendlyFire`; percorso sondato
  → `PathTrace`; V → `VisibleArea`. I soli valori nuovi in coda sono `AbilityRange` (v1) e `VisibleArea`. #1943
  governa la composizione, non il vocabolario. Resta aperta solo la **resa** di `Predicted` (un consumatore di
  `Certainty`, #1943/#1944).
- **CX-2 Un solo ingresso, misurabile** ([#3511](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3511)). Gli ingressi stanno in una struct con uguaglianza — per esempio
  `FRTPlanningOverlayInputs`: unità selezionata, contesto, `TargetKind`, azione armata, cella sotto il cursore e
  validità, settore puntato, piano attivo, una revisione del piano incrementata a ogni refresh, indice del turno. Il
  confronto avviene a ogni tick, il calcolo solo quando la struct cambia, in una funzione chiamabile senza
  `PlayerTick`. Nessun produttore all'hover ricalcola il reachable set. Le aree nuove entrano da un solo punto
  (`FRTOverlayArea`). Commit, inizio della risoluzione o del playback, Cleanup, `Modal`, KO e fine partita sono **un solo
  reset** (l'unione con v1 FR-5). Il criterio di #172 «nessun refresh dentro un Tick» si riformula: «nessun ricalcolo con ingressi
  invariati».
- **CX-3 Privacy.** Nessuna area prende posizioni nemiche in ingresso (test a mondi gemelli per ciascuna); i
  bersagli all'hover solo da nemici noti; la geometria mai osservata segue **una** regola per click, R e V (DR-2
  estesa).
- **CX-4 Certezza.** `Predicted`: V; E su unità nemica; R ed E quando l'origine dipende da uno scatto. La resa
  richiede un consumatore di `Certainty`: finché manca, la distinzione non è visibile e va dichiarato.
- **CX-5 Canale** ([#3508](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3508)). Vale v1 DR-4: `DrawDebugLine` è vuota in Shipping, quindi ogni area di questa spec è
  probabilmente invisibile nella build distribuita finché il canale non cambia.

## 8. Esempi (ricavati dal codice, non eseguiti)

Coordinate `(X, Y, Layer)` come `FRTCellId` (`X` = q, `Y` = r); direzioni `E(+1,0) NE(+1,-1) NW(0,-1) W(-1,0) SW(-1,+1) SE(0,+1)`. `VisionRange` forzato
nella fixture. «Da misurare» = il valore lo dà il test, non la lettura.

| # | Given | When | Then |
|---|---|---|---|
| V-A arco contro 360° | `MakeFlatArena` r6, unità (0,0,0), Facing E, VisionRange 3 | V con D = (0,0,0) | (3,0,0) ∈ V (arco); (-2,0,0) ∈ V (anello di 2); (-3,0,0) ∉ V; (0,-3,0) ∉ V. Con `HasLineOfSight` a 360° le ultime due entrerebbero: l'esempio smentisce l'impostazione di #3420. Bordo del cono (3,-3,0): da misurare |
| V-B ultimo passo | flat r6, unità (0,0,0) Facing E, VisionRange 3, waypoint (1,0,0) → (1,1,0) → (0,2,0) | Pathing, nessun verso dichiarato | F = SW (ultimo passo); (-3,5,0) ∈ V. Col primo passo (E, come la mesh) (-3,5,0) ∉ V |
| V-C settore | V-B, cursore sull'esagono finale, settore W | legale o no secondo il budget dell'eroe: da misurare | legale → V col Facing W; illegale → settore barrato e V resta SW |
| V-D Fumo | flat r6, Fumo in (2,0,0), unità (0,0,0) Facing E, VisionRange 5 | | (1,0,0) ∈ V; (4,0,0) ∉ V; prima cella esclusa sulla riga: da misurare |
| V-E piani | `MakeTestArena`, unità sulla piattaforma (2,0,1) Facing W, VisionRange 3 | | (1,0,0) ∈ V; (2,0,0), stessa proiezione, atteso ∈ V: da misurare; celle di V sul piano 0: da misurare (ingresso di DR-15) |
| V-F muro | `MakeTestArena`, unità (-1,0,0), VisionRange 3 | Facing E / Facing W | E: (0,0,0) ∈ V (gli estremi non bloccano), (1,0,0) ∉ V · W: (-4,0,0) ∈ V |
| V-G solo scatto | scatto pianificato su X, nessun waypoint | | contesto `Planning`, D = X, V presente (DR-11) |
| V-H non raggiungibile | cursore su una cella ∉ `ReachableCellsAfterPlan` | | V dalla sorgente successiva, mai da quella cella |
| E-a Area | flat r6, unità (0,0,0), azione Area r1 portata 4 (es. `CircularTide`), alleato in (3,0,0) | hover (2,0,0) | colpite (2,0,0) (3,0,0) (3,-1,0) (2,-1,0) (1,0,0) (1,1,0) (2,1,0); (3,0,0) fuoco amico. Dopo il click `RefreshPlanningPreview` dà lo stesso insieme |
| E-b Line | stessa fixture, azione Line portata 4, alleato in (1,0,0) | hover (3,0,0) | (1,0,0) (2,0,0) (3,0,0), origine esclusa; (1,0,0) fuoco amico |
| E-c fuori portata | portata 4 | hover (5,0,0) | nessuna area; rifiuto di portata, lo stesso del click |
| E-d noto/ignoto | `MakeTestArena`, unità (-3,0,0); mondo A nemico noto in (3,0,0), B nemico ignoto in (3,0,0), C cella vuota | hover (3,0,0) | A: area e anteprima d'attacco; B e C: uscite **identiche** |
| E-e scatto | `MakeTestArena`, `FluidTrail` (-1,0)→(2,0), poi `PressureJet` | hover (4,0,0) e (-2,0,0) | E e slot concordano (riuso di v1 E10) |

## 9. Criteri di accettazione e verifica

**Oracoli indipendenti** — nessuno ricalcola con la funzione del produttore.
- **AC-V1** In uno scenario headless con rotta non contesa e senza superfici dinamiche, V del piano è uguale a
  `VisibleCells(Map, {cella finale, facing finale, VisionRange})` letti **dall'unità risolta**, e V ⊆
  `TeamVisibleCells` al refresh del turno dopo. Le mutazioni «primo passo al posto dell'ultimo» e «`ArrivalFacing`
  ignora il verso dichiarato» lo fanno fallire.
- **AC-V2** Per ciascuno dei sei settori, su una fixture con budget di rotazione limitato: V(settore) = `VisibleCells`
  col verso che `TryApplyDeclaredFacing` restituirebbe.
- **AC-V3** Mondi gemelli diversi solo per un muro o un Fumo in una cella mai osservata: V identica con DR-2 (ii) o
  (iii), diversa con (i), dove la differenza è il leak scritto come atteso.
- **AC-V4** `MakeTestArena`, osservatore in (2,0,1): il segnale «V su altri piani» si accende ⇔ V ha celle con
  `Layer != 1`.
- **AC-V5** Misura di tempo di `VisibleCells` sulla mappa spedita più grande, con la `VisionRange` massima del
  roster, a motore libero (CLAUDE.md §10: è una misura di tempo). Soglia fissata dopo la misura; prima, `NOT RUN`.
- **AC-E1** Per ogni esempio E-a…E-c, l'insieme all'hover coincide con quello di `RefreshPlanningPreview` dopo un
  click accettato sulla stessa cella; un click rifiutato non produce area e dà lo stesso motivo.
- **AC-E2** Per ogni `c` in `HexArea(O, RangeCells + 1)`: l'hover su `c` produce E ⇔ il click su `c` posa il piano.
- **AC-E3** Mondi gemelli E-d; la mutazione «bersaglio senza `IsKnownToObserver`» lo fa fallire.
- **AC-E4** A bersaglio dichiarato T1, l'hover su T2 lascia invariato il dato dell'anteprima A.
- **AC-ER** Coerenza fra E e R, per le sole azioni a kind `Cell` (per il bersaglio unità vale la garanzia per famiglia
  del gemello, §2): con `Ok` soltanto, `c ∈ R` ⇔ il predicato di AE-1 è vero su `c`; con la regola di #3507 (`Ok` ∪
  `NoLineOfSight`, DR-14), `c ∈ R` e verdetto diverso da `NoLineOfSight` ⇔ il predicato è vero. Su ogni cella, con e
  senza scatto.
- **AC-X1 Conteggio**: N tick con ingressi invariati → un solo calcolo; cambio della sola azione armata → +1; della
  sola cella → +1; in un test di oscillazione fra due celle, tanti calcoli quanti cambi. Il seam esiste già
  (`ARTHexMapActor::SetHoveredCell`, usato da `FSlotRefusalBench`).
- **AC-X2 Transizioni**, una per riga sul banco del lock-in (`FRTLockInPreviewBench`, dopo aver esteso
  `PreviewCellsLit` alle aree nuove): Planning→Pathing (V passa alla destinazione) · Pathing→Targeting (V spenta se Q7 conferma la proposta, R
  accesa) · Targeting→Pathing col Back (E spenta, V ripristinata) · cursore sull'esagono finale (V segue il settore
  legale) · Back dopo il verso dichiarato (V torna al facing derivato) · cambio di unità (nessuna area della
  precedente) · ingresso in playback o `Modal` (zero aree) · Planning del turno dopo (nessuna V del piano
  precedente).

**Mutazioni sulla riga del CHIAMANTE**, ognuna col test che la uccide: `Unit->Facing` al posto di F (V-B) · primo
passo al posto dell'ultimo (V-B) · verso dichiarato applicato all'hover (V-C) · settore illegale accettato (V-C) ·
`VisionRange` di default al posto di quello dell'unità (V-A) · guardia «∈ Reachable» tolta (V-H) · bersaglio senza
`IsKnownToObserver` (E-d) · origine dello slot `Unit->Cell` (E-e) · chiave ridotta alla sola cella (AC-X1) · reset
mancante (AC-X2).

**PIE — una domanda binaria per area**, contata per tasti, da `L_DevSandbox` via console:
- V: *«Col percorso curvo di V-B, l'area di vista all'arrivo è rivolta a sud-ovest — dove arrivi, non da dove
  parti?»*
- V sul settore: *«Scorrendo col cursore i settori dell'esagono finale, l'area ruota sui settori ammessi e resta
  ferma su quelli barrati?»*
- E: *«Con l'azione ad area armata, le celle mostrate passando su (2,0,0) restano identiche dopo il click?»*
- E fuori portata: *«Su una cella fuori portata non compare nessuna area d'effetto?»*

**Stati attesi a fine implementazione**: Compile, Tests, Privacy (headless) misurabili · PIE dopo le sedute ·
AC-V5 `NOT RUN` finché il motore non è libero per una misura di tempo · **Packaged `NOT RUN`** finché vale v1 DR-4.

## 10. Decisioni richieste (BLOCKED — DECISION REQUIRED)

Decide l'autore; registro: `D-nnn` nel Decision Log più un commento sulla issue indicata. Restano aperte anche le
decisioni del referto gemello — tranne **v1 DR-3** (tinta `#AF52DE`), decisa in #3507 — e la sua **DR-14** (portata
o mira).

| DR | Domanda | Raccomandazione del panel | Registro |
|---|---|---|---|
| **DR-2 estesa** | Geometria non visibile, per click, R e V insieme | Tre casi, non due: mai osservata · esplorata ma non visibile ora · visibile ora. **(ii) estesa**: nel mai osservato nessun occlusore; nell'esplorato non visibile la sola geometria statica (non esiste un modello «ultima superficie vista»); il Fumo dinamico conta solo dove si vede ora. V diventa un limite superiore nel buio, reso `Uncertain`. (iii) svuoterebbe V proprio dove serve. Dissenso (Adzic): valutare l'ottimismo **solo per V**, che è pura anteprima | `D-nnn` in #3270 (allargata) o #2791 |
| **DR-9** | In quali casi compare V | **Fetta 1**: destinazione pianificata (waypoint o scatto) + settore sull'esagono finale in `Pathing` + `Facing` da fermo, sui soli settori legali. **Fetta 2**: hover sulle celle raggiungibili, dopo la misura di costo e DR-2 (all'hover V diventa una sonda gratuita su ogni cella sorvolata). Mai con il movimento chiuso dal verso dichiarato | commento #3420 |
| **DR-10** | Insieme di V | **Vista piena dell'unità** (unanime); il risalto delle celle nuove è DEFERRED | commento #3420 |
| **DR-11** | V con uno scatto e senza waypoint | **Inclusa** (unanime): emendare la DoD di #3420, che dice «senza percorso non compare» | commento #3420 |
| **DR-12** | Sede delle aree guidate dall'hover | **Nessuna sede nuova** (unanime): l'hover è un ingresso; unico valore nuovo `VisibleArea`. Resta da decidere la resa di `Predicted` | commento #1944 / #1943 |
| **DR-13** | Portata dalla destinazione ipotetica | **DEFERRED** (unanime): per ADR-0003 vale il turno dopo, e accanto a R farebbe credere di sparare da lì | — |
| **DR-15** | V su più piani | Modello mai filtrato; disegno secondo la modalità di piano, con segnale non cromatico; prima la misura V-E | commento #1944 |
| **DR-7 estesa** | A bersaglio dichiarato, hover su un altro bersaglio | E al posto di A finché il cursore resta lì; il dato di A non cambia | commento #1944 |

## 11. Domande per l'autore

- **Q1** Su `L_DevSandbox` porti Aevik su una cella dietro il muro e ti fermi. Cosa ti aspetti evidenziato: **(A)**
  le celle che **vedrà** da lì, o **(B)** quelle che potrà **colpire** da lì (che valgono solo il turno dopo)?
- **Q2** Nella lettura A: tutte le celle che vedrà, o solo quelle che oggi la squadra non vede?
- **Q3** La vista la vuoi già passando col mouse sulle celle raggiungibili, prima di cliccare, o basta sulla
  destinazione scelta e mentre scegli il verso?
- **Q4** Un Fumo nemico comparso in una zona esplorata ma fuori vista: l'anteprima può tenerne conto (rivelandolo)
  o deve ignorarlo finché non lo vedi?
- **Q5** Col bersaglio già dichiarato, passando su un altro nemico valido vuoi vedere l'area che colpiresti cambiando
  bersaglio?
- **Q6** Con l'unità su una piattaforma, le celle del piano sotto che vedrebbe devono comparire anche quando mostri
  solo il piano attivo?
- **Q7** Con una rotta posata, armare un'azione spegne la vista dalla destinazione (proposta) o la lascia?
- **Q8** Un nemico noto solo come ultima posizione è un bersaglio accettato dal click? Se sì, E su di lui è
  `Uncertain` o assente?

## 12. Sequenza e owner

1. **Prerequisiti comuni** (CURRENT REQUIRED perché nessuna area menta o resti stantia):
   - una funzione d'origine consumata anche da `RefusalUnderPointerForArmed` e dal produttore di E —
     [#3509](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3509) (v1 DR-1 estesa);
   - un solo ingresso misurabile e un solo reset (CX-2) — [#3511](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3511);
   - nessun armo durante il playback — [#3510](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3510).
   ⚠️ **Non è un prerequisito, ma un difetto ereditato da tutte le aree**: in Shipping non si vedrebbero — [#3508](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3508)
   (v1 DR-4). Finché il canale non è deciso, ogni implementazione dichiara `Packaged: NOT RUN` con questo motivo.
2. **E** in `Targeting/Cell`, poi in `Targeting/Unit` dopo v1 DR-6 — [#3512](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3512), con #172.
3. **R** — [#3507](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3507).
4. **V fetta 1** — [#3420](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3420) allargata, con la DoD emendata (VisibleCells e non HasLineOfSight;
   `VisibleArea` e non `Vision`; scatto incluso; settore sull'esagono finale; settori legali). Emenda
   anche l'insieme mostrato (vista piena, DR-10 e Q2) e la seduta PIE: la DoD di #3420 la vuole da `L_Frontend` →
   `PLAY`, gli esempi di questo referto usano scenari da `L_DevSandbox`. Richiede `ArrivalFacing`, DR-2 estesa e
   DR-10; la resa di `Predicted` segue CX-4 (si spedisce dichiarando che la distinzione non è ancora visibile).
5. **V fetta 2** (hover) — #3420, dopo AC-V5.

**Opportunistico**, non prerequisito: il bot usa `ArrivalFacing` invece della copia privata; i setter esistenti
migrano al punto d'ingresso unico; `ExposedEdges` confluisce in `ExtractBoundaryEdges`.

## 13. Derive e conflitti trovati (fuori scope, con evidenza)

- **CONTRACT CONFLICT** — #3420 contro D-373: la premessa «la mappa il giocatore la conosce» non regge per il
  contenuto delle celle mai osservate.
- **IMPLEMENTATION DRIFT** — #3420 imposta l'anteprima su `HasLineOfSight` (360°) e sul significato `Vision` (la linea
  di #2742), mentre la vista vera è direzionale (`VisibleCells`).
- **IMPLEMENTATION DRIFT** — `spec-pointer-interaction.md` §5.1: le righe `Hover = Preview` in `Targeting`, il
  «Preview percorso e costo» in `Planning`/`Pathing` (`ProbePathTo` ha solo un consumatore Editor) e il «settore
  pieno/barrato» in `Facing` non hanno produttore.
- **CONTRACT CONFLICT** — i contesti di #1943 (Vision Inspection) e di #2597 (ispezione nemica) non esistono fra quelli
  del puntatore; #1943 non ha righe per `Facing`, `ResolutionPlayback` e `Modal`.
- **STALE ROADMAP** — D-376 affida a #1614 l'Interaction Context, ma il corpo di #1614 non lo dichiara.
- **STALE** — #2742 e #3085 risultano aperte con il lavoro già su `main`; `progettazione-hud.md` §4.2 chiama «cono» la
  zona Overwatch, che D-387 definisce una linea.
- **Deriva di docstring** — `FRTPerceiver::VisionRange`: «0 = non vede oltre la propria cella», mentre resta l'anello
  di 2.
- `MakePlanPreview` ignora il verso dichiarato di #291 (`ERTPreviewFacingSource` non ha un valore per lui), e la mesh in
  pianificazione guarda il primo passo.

## 14. Provenienza e limiti

- Ricognizione mirata su `903154a41`: tre lettori più un critico. Fatti chiave riverificati dal coordinatore: modello
  di `VisibleCells`, scrittura della vista di transito in `ExploredCells` soltanto, testo di D-376 e ordine dei commit
  di D-376 e di `Vision`.
- Panel: tre coppie più un verificatore. Fra i rilievi CRITICAL il verificatore ha confermato le premesse di COC-1,
  WIE-2, C-1, C-2 e NY-1; WIE-1 è risultato parziale (il conteggio dei Secondary è vero, ma #1943 prevede di disegnare
  i due a priorità più alta e tenere gli altri nei dati). Il verificatore ha letto D-376 come un enum congelato; il
  testo non distingue, e il precedente di git (§4 V-8) sostiene la lettura opposta, da confermare.
- Nessun codice modificato, nessun gate eseguito: Compile, Tests, Determinism, Replay, Privacy, PIE e Packaged sono
  `N/A` per questo passaggio.
