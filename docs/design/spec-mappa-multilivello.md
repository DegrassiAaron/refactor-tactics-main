# Design — Mappa multilivello (vertical slice PF.4)

> `/sc:design` del **2026-08-02**. Ancorato al motore a grafo **PF.4.2** già consegnato
> ([`spec-pathfinding-pf3-pf4.md`](spec-pathfinding-pf3-pf4.md)), al terreno ([`spec-terreni.md`](spec-terreni.md))
> e al canone ([`piano-canonico-mvp.md`](piano-canonico-mvp.md)). Documentale: nessuna modifica al codice.
> Sblocca la parte *visiva/gameplay* di PF.4, finora gated proprio da questo design.

## 1. Obiettivo e scope

**Vertical slice a 2 layer** (allineato a [Piano completo p.32]: 2 layer, scale + jump pad): introdurre
**verticalità tattica** riusando il motore a grafo e i sistemi esistenti, con la modifica **minima** al mondo
di gioco. Non è la mappa multilivello north-star completa (N layer, distruttibilità, meteo): è il primo
incremento *giocabile e giustificato*.

## 2. Concept di gioco — "alta quota contesa"

Le **4 celle-copertura centrali** attuali (`ARTGridActor::BlockedCells` = (4,4)(5,4)(4,5)(5,5)) diventano una
**piattaforma rialzata**:
- **Layer 0** (terra): quelle 4 celle restano **bloccate** (la base della piattaforma = la copertura di oggi;
  continua a bloccare movimento e LOS a terra).
- **Layer 1** (tetto): le stesse 4 celle sono **calpestabili** e portano il buff **Altura** (già esistente:
  `OccupantDamageBonus`, "alta quota"). Chi sale colpisce più forte ma è **esposto**.
- **Accesso**: 2 **scale** su lati opposti (archi bidirezionali, costo 2) → l'alta quota è *contesa*, non di una
  sola squadra. Es. scala A: `(3,4,0) ↔ (4,4,1)`; scala B: `(6,5,0) ↔ (5,5,1)`.

Perché regge: verticalità con **una ragione di gioco** (rischio/ricompensa dell'alta quota), riuso di **Altura**
e delle **coperture** esistenti, modifica minima al mondo. Realizza il pilastro "mappa come sistema di gioco".

## 3. Modello dati

Riusa i tipi PF.4 (`FRTGridCoord{X,Y,Layer}`, `FRTTraversalEdge`). L'`ARTGridActor` diventa la fonte del grafo:

- **Celle valide per layer**: layer 0 = intera 10×10; layer 1 = **insieme esplicito** di celle-piattaforma
  (le 4 centrali). Le celle *non presenti* su un layer sono **impassabili** (costo `RT_BLOCKED_COST` nella
  cost map per quel layer). → un solo meccanismo (la cost map), nessun nuovo concetto di "cella inesistente".
- **Archi**: `TArray<FRTTraversalEdge> Edges` sull'`ARTGridActor` (scale/portali), creati **a runtime in C++**
  (come terreno e abilità, nessun `.uasset`). Bidirezionale = 2 archi.
- **Terreno per cella**: `TerrainCells` già mappa `FRTGridCoord → URTTerrainData`; ora la chiave include il
  Layer → si può mettere Altura sulle celle `(…,1)` del tetto.
- **Costo autorevole**: `ARTGridActor::BuildCostMap` va esteso a includere i layer: per ogni cella valida di
  ogni layer, il costo del terreno; per le celle non-valide di layer 1, `RT_BLOCKED_COST`.

## 4. Rendering

- **Due componenti ISM** per le celle: `Cells0` (layer 0, quota Z base) e `Cells1` (layer 1, quota
  Z + `LayerHeight` ≈ 250 cm, solo le celle-piattaforma). La copertura di layer 0 (cubo) resta come base.
- **Scale**: una mesh-rampa (o un semplice marker inclinato) tra le celle collegate dagli archi.
- **Colore/terreno**: il tetto usa il colore Altura (già `DisplayColor`); `RefreshTerrainVisuals` esteso ai layer.
- **Camera**: la top-down attuale gestisce le altezze (le celle alte appaiono sopra); nessun cambio richiesto
  nell'MVP. Eventuale leggero aumento del pitch se serve leggibilità (tunabile).

## 5. Interazione (il pezzo nuovo più delicato)

Il problema: un click deve risolvere `(X, Y, Layer)`, non solo `(X, Y)`.
- **Soluzione**: ogni layer ha il proprio ISM con collisione; `GetHitResultUnderCursor` ritorna il **componente
  colpito** e l'indice-istanza → si deriva il Layer (Cells0 → 0, Cells1 → 1) e da lì la cella. Le celle di
  layer 1 stanno più in alto e "coprono" quelle di layer 0 sotto: cliccando la piattaforma si seleziona il tetto,
  cliccando il pavimento intorno si seleziona il terra. Deterministico e senza ambiguità.
- **Unità su layer 1**: `PlaceOnCell` posiziona l'attore alla quota del layer (`Z + Layer*LayerHeight`).
- **Movimento**: il controller costruisce la path con `FindPathByGraph` (celle + archi) → salire/scendere è
  un waypoint che attraversa una scala. Il resolver `ResolvePaths` è già layer-agnostico (opera su celle).

## 6. Regole di gioco (MVP)

- **Movimento cross-layer**: via scale (archi), costo 2. Il budget di movimento resta un budget di **costo**.
- **Buff alta quota**: le celle-tetto sono Altura → `+OccupantDamageBonus` (valutato pre-movimento, come già è).
- **LOS**: **MVP = proiezione 2D** — la linea di tiro si calcola su `(X,Y)` ignorando il layer (chi è in alta
  quota può ingaggiare a terra e viceversa se la linea 2D è libera). *Semplice, prevedibile*; la LOS 3D reale
  (l'alta quota vede oltre le coperture basse) è un raffinamento successivo. **Da confermare** (§9).
- **Caduta/spinta**: nessuna meccanica di push nell'MVP → nessuna caduta. Rimandata.
- **Occupazione**: 1 unità per cella *per layer* (celle `(x,y,0)` e `(x,y,1)` sono distinte).

## 7. Wiring (per la futura `/sc:implement`)

1. `ARTGridActor`: insieme celle-piattaforma layer 1 + `Edges` (scale) + `BuildCostMap` multilivello +
   `GetEdges()`; `Cells1` ISM + rendering scale; `RefreshTerrainVisuals` per-layer.
2. `RTPlayerController`: `WorldToCell` → `(X,Y,Layer)` dal componente ISM colpito; movimento/validazione con
   `ReachableCellsByGraph`/`FindPathByGraph` + `Edges`; preview path su più layer.
3. `RTTurnManager::ResolveMovement`: usa `ReachableCellsByGraph`/`FindPathByGraph` (già ordine-indipendente via
   `ResolvePaths`); `PlaceOnCell` con quota del layer.
4. **Bot**: `BuildBotCostMap` multilivello; `BestApproachCell`/`BestKiteCell` su reachability a grafo (per salire
   in alta quota o evitare l'esposizione — estensione scoring).
5. **HUD**: la preview e la traccia già disegnano celle → estendere `CellToWorld` con la quota del layer.

## 8. Determinismo & invarianti

- Il pathfinding a grafo è **deterministico** (tie-break `X,Y,Layer`); costo intero (R3) → hash stabile.
- Autorità server: reachability/path validati col grafo (come già in 2D).
- **`GraphRevision`/`SchemaVersion`**: servono solo con archi **dinamici** (ponte che crolla, portale creato da
  skill). Nell'MVP il grafo è **statico** nello snapshot del turno → non serve ancora (north-star, coerente con
  §8 dello spec PF.3/PF.4).

## 9. Decisioni aperte (da confermare)

1. **Layout**: piattaforma centrale (riusa le coperture) *vs* torri agli angoli *vs* ponte sopraelevato.
2. **LOS**: proiezione 2D (semplice) *vs* regole di elevazione (l'alta quota ignora le coperture basse).
3. **Scope del primo giro**: mappa giocabile completa *vs* prima il wiring minimo (una piattaforma + una scala)
   verificabile in PIE, poi rifinire.

## 10. Rischi

- **Interazione click→layer**: il pezzo più nuovo; va provato in PIE presto (l'ISM-hit potrebbe richiedere
  tuning di collisione/priorità tra Cells0 e Cells1).
- **Leggibilità**: due layer sovrapposti in top-down possono confondere → colori/quota da tarare.
- **Bot**: la reachability a grafo cambia lo spazio di scelta; scoring da estendere per non fare mosse strane.
- **Scope**: è comunque un'epica; conviene affettarla (una piattaforma + una scala prima di tutto).

## 11. Conflitti segnalati (regola CLAUDE.md)

- **Semantica delle coperture centrali**: cambiano da "ostacolo pieno" a "base di piattaforma + tetto
  calpestabile". Va recepito dove il canone/roadmap descrive le 4 celle centrali come sola copertura.
- Coerente con le decisioni canoniche **R1/R2/R3** (§3.1 piano canonico) e col motore PF.4 già consegnato.
