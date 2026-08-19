# Spec — Dash come fase attiva (riposizionamento rapido)

> Per il confronto **fra le famiglie** di movimento — cosa distingue Dash da Move, da uno spostamento forzato
> e da un teletrasporto: [`spec-tassonomia-movimento.md`](spec-tassonomia-movimento.md). Questa pagina resta
> l'owner del Dash.

> `/sc:design` + implementazione del **2026-08-03**. Obiettivo: attivare la fase **Dash** (finora pass-through)
> con un riposizionamento rapido che si risolve **prima del Blast**. Documentale + implementato in TDD dove la
> logica è pura, wiring + PIE per Actor/UI. Ancorata al canone ([`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md)),
> al pathfinding a grafo ([`spec-pathfinding-pf3-pf4.md`](../technical/architecture/spec-pathfinding-pf3-pf4.md)) e all'animazione della
> risoluzione ([`spec-anima-risoluzione.md`](spec-anima-risoluzione.md)).
>
> ⚠️ **Superato il 2026-08-08 da [D-028](../decisions/RT_PDR_00_Decision_Log.md)** — **dash + move non e'
> piu' consentito**. Lo scatto occupa lo **slot movimento**: un turno da' un movimento e un'azione
> principale, e si sceglie **quando** muoversi — *schivo e sparo* (`Dash` + attacco) oppure *sparo e muovo*
> (attacco + `Move`). Un eroe puo' dichiarare l'eccezione **nel proprio kit**; la regola generale no.
> Tutto il resto di questa spec resta valido — lo scatto risolve **prima del Blast**, gli stili lineari, la
> rivalidazione di gittata/LOS dalla posizione post-scatto: **cambia lo slot, non la fase**.
>
> **Decisioni vigenti**: ~~dash + move (movimento doppio consentito)~~ · scope = **bot + player** ·
> il movimento dello scatto è **lineare** e un ostacolo lo **ferma**, deciso da `ERTMovementStyle`
> (`LinearDash` attraversa, `LinearCharge` si ferma sul primo nemico, `LinearLeap` scavalca).
>
> ⚠️ **Corretta il 2026-08-08.** La riga «movimento del dash = **pathfinding** (aggira ostacoli)» compariva
> ancora fra le *decisioni prese*, smentita quattro righe più sotto dal blocco di aggiornamento: due
> affermazioni opposte nella stessa testata, e la prima è quella che si legge per prima. È stata **rimossa**,
> non barrata — una decisione superata non va lasciata in un elenco intitolato «decisioni prese».
> Superata da **CP 4.5** (`#46`) e `#142`, che hanno anche eliminato `bDash`.
>
> 🧭 **[ADR-0005](../decisions/adr-0005-orientamento.md)**: `LinearDash`, `LinearCharge` e `LinearLeap`
> **derivano il facing** dalla direzione del movimento — una sola direzione legale, non dichiarata a parte.
>
> 🏃 **[D-015](../decisions/RT_PDR_00_Decision_Log.md)**: `Sprint` **non è un Dash**. Appartiene alla famiglia
> `Move` come profilo (`Sneak · Normal · Sprint`) e non introduce una seconda semantica di fase. Questa spec
> descrive **solo** la mobilità speciale pre-Blast. L'`Action.Sprint` a catalogo, classificato oggi come
> azione a budget, è **debito di migrazione** dichiarato — vedi le Note del Decision Log.

## 1. Obiettivo e valore tattico

La fase **Dash** era nel ciclo (`Planning → Prep → Dash → Blast → Move → Cleanup`) ma **senza handler**
(pass-through). Ora un'abilità di **scatto** ci sposta **prima che gli attacchi (Blast) colpiscano**: si schiva
un tiro previsto, si chiude la distanza per il corpo a corpo, o si esce dalla copertura. È il cuore di Atlas
Reactor e dà senso alla fase + profondità alla pianificazione (il Blast usa le posizioni **post-scatto**).

## 2. Modello dati

- **Cosa rende un'azione uno scatto**: la **fase dichiarata dal catalogo**, e nient'altro —
  `URTCatalogLibrary::IsFastMovement(Def)` è vero quando la macro-fase è `Dash`. Il flag `URTActionData::bDash`
  è stato **rimosso** con `#142`: esisteva in parallelo alla fase, e le azioni degli eroi (che dichiarano solo
  la fase) non venivano riconosciute — il bot non pianificava scatti per nessuno dei quattro.
- **Come si sposta** lo dice `FRTActionDef::MovementStyle`: `LinearDash` · `LinearCharge` (si ferma addosso al
  primo nemico e lo colpisce) · `LinearLeap` (scavalca) · `Budget` (`Action.Sprint`, pathfinding). `None` su
  un'azione di fase Dash è un **errore**, sorvegliato da `RefactorTactics.Actions.EveryFastMovementDeclaresStyle`
  su tutti e tre i cataloghi (generico, spedito, eroi).
- `ARTUnit::PlannedDashAbility` (`INDEX_NONE` = nessuno) + `ARTUnit::PlannedDashCell` — pianificazione dello
  scatto per il turno. `ARTUnit::FindDashAbilityIndex()` trova l'abilità di scatto dell'unità.
- Abilità di scatto di default: **Ranger → "Scatto"** (`Ranger.Dash`, 5 celle, ricarica 2, `LinearDash`);
  **Guardian → "Carica"** (`Guardian.Charge`, 4 celle, ricarica 3, `LinearCharge`, 20 danni + spinta 1);
  fallback generico "Scatto" (`Action.Dash`: 3 celle, ricarica 1). Sono la **4ª abilità** (indice 3).
  Gli eroi del catalogo v0.1 hanno `Hero.Riktor.Ram` (`LinearCharge`), `Hero.Wraith.PassingBlade` e `Hero.Phase.FluidTrail`
  (`LinearDash`).
  > Questi sono i valori **oggi nel codice** (`ARTUnit::ConfigureAsArchetype`), non i valori vigenti della
  > v0.1: con il budget a **5 MP** dell'[ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) le mobilità rapide passano a
  > distanza fissa dichiarata dall'azione (`Dash 3`, `Charge 4`, `Leap 3`, `Sprint 8 MP`) — riparametrizzazione
  > al **CP 4.2** (issue `#43`), con la suite del bot come gate.

## 3. Risoluzione (`ARTTurnManager::ResolveDash`, fase Dash)

1. Raccoglie le unità vive con uno scatto pianificato **valido**: azione di fase Dash utilizzabile
   (`IsFastMovement`) e `PlannedDashCell` raggiungibile **secondo lo stile dichiarato** — in linea retta per le
   mobilità lineari (`URTMovementActionLibrary::ResolveLinearMove`, la portata si misura in **celle**), col
   pathfinding per quelle a budget (dove `RangeCells` sono **punti movimento**). Lo scatto è **consumato per il
   turno** in ogni caso (valido o no).
2. Risolve gli scatti **simultanei, ordine-indipendenti** (stesso `URTMovementResolver::ResolvePaths` del
   movimento).
3. Applica le posizioni **senza cancellare il move normale** (dash + move): aggiorna `GridCell` + visuale;
   la path composita (che partiva dalla cella pre-scatto) è invalidata; se **non** c'era un move pianificato
   (`PlannedCell == pre-scatto`) imposta `PlannedCell = destinazione` (niente ritorno indietro), altrimenti il
   `PlannedCell` resta e il Move normale instrada dalla nuova posizione.
4. Consuma l'abilità (cooldown). Applica il **cross-damage** del terreno attraversato (ord.-indip.); una morte
   durante lo scatto genera un evento `Defeated` (fase Dash).
5. Emette eventi `Move` con **fase Dash** per il playback.

**Invarianti**: determinismo (resolver ord.-indip., budget interi); il Blast usa `GridCell` post-scatto; i morti
sono esclusi dal Move (come per il combattimento).

## 4. Bot (`PlanBots`)

- **Scatto offensivo** (nessun tiro dalla cella attuale, non in fuga): se lo scatto è **pronto**, il bot lo usa
  per riposizionarsi/avvicinarsi **in fretta** (più portata del movimento) — prima cerca una **posizione di
  tiro** raggiungibile con lo scatto (`BestFiringCell`), altrimenti chiude la distanza (`BestApproachCell`).
- **Scatto difensivo (schiva)**: quando è minacciato (panic o kiter con nemico entro lo standoff) e lo scatto è
  pronto, il bot **fugge con lo scatto** (`BestKiteCell` col range dello scatto). Poiché il dash si risolve
  **prima del Blast**, la posizione post-scatto **ri-valida gittata/LOS** dell'attaccante: uscendo dal tiro,
  l'attacco previsto **manca**. Fallback al kite normale (movimento) se lo scatto non è pronto.

Il bot è **dash-only** (non pianifica anche il move quando scatta). Le azioni di mobilità rapida sono escluse
dalla selezione d'attacco (non sono attacchi) e dal calcolo della minaccia nemica (spostano, non colpiscono). *(Verifica PIE: osservati sia `scatto -> …` offensivi sia
`scatto difensivo (schiva) -> …`.)*

## 5. Giocatore (`RTPlayerController`, HUD)

- **Tasto `4`** seleziona lo scatto (4ª abilità). Con lo scatto selezionato, il **click su una cella** pianifica
  il dash verso quella cella. La validazione usa **lo stesso codice che eseguirà lo scatto** (`#142`): lineare
  per le mobilità lineari, pathfinding per quelle a budget. Vale la regola di CP 4.5 — **o si arriva sulla cella
  richiesta, o lo scatto non si pianifica**: niente scatto a metà verso una cella che il giocatore non ha
  scelto. L'unica eccezione è la **carica**, per cui fermarsi addosso al nemico *è* il modo di arrivare.
  Cliccare un nemico con lo scatto selezionato non fa nulla (lo scatto è su cella).
- **HUD**: la preview dello scatto è disegnata in **magenta** (percorso + destinazione), distinta dal movimento
  normale (ciano). La traiettoria segue **lo stile dichiarato** (`FRTIntentView::DashStyle`): linea retta per le
  lineari, grafo per quelle a budget — disegnare l'A\* per uno scatto lineare mostrerebbe un percorso curvo
  attorno a un ostacolo che in realtà lo ferma. Soggetta alla privacy dell'intento (invariante #6).

## 6. Playback (animazione)

La fase Dash è **animata prima del Blast** (riusa il sistema di [`spec-anima-risoluzione.md`](spec-anima-risoluzione.md)):
gli eventi di scatto sono `Move`-type con fase Dash; le animazioni sono taggate per fase; un'unità che fa
scatto **e** move ha due animazioni consecutive e continue (scatto → posizione intermedia → move). Ordine di
playback: `Prep → Dash → Blast → Move`.

## 7. Verifica

- **TDD (logica pura riusata)**: il dash riusa `FindPathByGraph`/`PathCost`/`ResolvePaths` (già testati,
  ordine-indipendenza). Nessuna nuova matematica: l'orchestrazione è verificata in PIE.
- **Regressione**: 62/62 automation test verdi (invariati) → il dash non altera gli esiti.
- **PIE (log)**: il bot pianifica ed esegue scatti (`scatto -> (x,y,L)` / `Scatto: … -> (x,y,L)`); la fase Dash
  è animata **prima** del Blast (`Fase Dash → Playback fase: Dash → Blast → Playback fase: Blast → Colpo`);
  nessun crash. Osservati scatti a `layer 0` (terra); lo scatto sul ponte (`layer 1`) emerge quando il bot è
  vicino a una rampa (situazionale).

## 8. Limiti aperti

- **Dash del giocatore**: cablato (tasto `4` + click + preview), ma la conferma **interattiva** richiede il
  mouse dell'utente (come il click→layer di [`spec-mappa-multilivello.md`](../technical/architecture/spec-mappa-multilivello.md)).
- ~~**Dash "leggero"** (salto che scavalca le coperture in linea retta) **non** implementato: scelto il
  pathfinding (aggira gli ostacoli).~~
  > ✅ **Superato il 2026-08-08**: era il residuo della decisione ribaltata da CP 4.5. Lo scavalcamento
  > **esiste** ed è `LinearLeap`; il pathfinding non è più il movimento dello scatto. La riga è conservata
  > barrata perché è la sola traccia di quanto a lungo la spec si è contraddetta da sola.
- Tuning dei valori (portata/ricarica degli scatti) da tarare in gioco.
- **`Action.Sprint` è a budget e vive in questa spec per eredità** ([D-015](../decisions/RT_PDR_00_Decision_Log.md)):
  semanticamente appartiene ai profili di `Move`. Finché la migrazione non è fatta, l'ID resta dov'è —
  ma **nessun documento deve insegnare «Sprint = Dash»**.

## 9. Interazione con gli status (2026-08-03)

Lo scatto rispetta gli status come il movimento normale: **Root azzera** la portata dello scatto (unità
radicata → niente scatto), **Slow la dimezza**. Implementato con `ARTUnit::GetEffectiveDashRange(BaseRange)`
che riusa `URTCombatLibrary::EffectiveMoveRange(...)` (già testato: Root→0, Slow→metà), applicato in
`ResolveDash` (autoritativo) e nella pianificazione di bot e giocatore. Regressione 62/62 invariata.
