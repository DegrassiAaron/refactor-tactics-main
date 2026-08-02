# Spec — Terreni (data-driven) + terreno dinamico + path composita

> Discovery `/sc:brainstorm` + review `/sc:spec-panel` del **2026-08-02**. Fonti: scelte utente +
> `IdeeBase.pdf` (idee) + canone ([`piano-canonico-mvp.md`](piano-canonico-mvp.md)) + spec pathfinding
> ([`spec-pathfinding.md`](spec-pathfinding.md), [`spec-pathfinding-pf3-pf4.md`](spec-pathfinding-pf3-pf4.md)).
> Autorità: subordinato al piano canonico. `IdeeBase.pdf` = north-star (conflitti §9).

## 1. Re-slicing (raccomandazione del panel, accettata)

L'epica accoppia due blocchi di rischio diverso. Si consegnano **in sequenza**:

| Slice | Contenuto | Rischio | Poggia su |
|---|---|---|---|
| **Terreno v1** | `URTTerrainData` data-driven · **PF.3** (A\* pesato) · Fango (costo) · Cespuglio (vista) · Altura (buff) · Lava/Fuoco hazard **solo fine-turno** · ignite stesso-turno · celle colorate · **movimento a destinazione singola con auto-route** (resolver attuale, invariato) | medio | PF.1/PF.2 già in produzione |
| **Movimento v2** | **path composita** (waypoint) · **cross-damage** · **resolver path-aware** (microstep, troncamento ordine-indipendente) · le 3 viz complete | **alto** | Terreno v1 |

Perché regge: il `CrossDamage` **richiede** il resolver path-aware (deve conoscere le celle attraversate) → è inscindibile da v2. Tenendo l'hazard **solo a fine turno** in v1, il terreno gira sul resolver-a-destinazione **già validato** (PF.1). v2 aggiunge il pezzo difficile su fondamenta stabili, con la sua prova di determinismo.

## 2. Timeline canonica del turno (determinismo dei trigger)

In turni **simultanei**, "prima/dopo" non è ambiguo: ogni effetto ha uno **slot fisso**.

```
Lock-in
 → Prep    : buff su sé (Barriera)                                   [già presente]
 → Dash    : mobilità rapida                                         [futuro]
 → Blast   : attacchi  +  IGNITE terreno (mutazione mappa; simultaneo da snapshot)
 → Move    : movimento (v1: a destinazione; v2: path + CrossDamage per cella attraversata)
 → Cleanup : EndTurnDamage (su chi OCCUPA)  +  tick durate (Fuoco/status)  +  reversione terreno
```

Decisioni: **ignite = stesso turno** (Blast precede Move); **attacco+ignite simultanei** (snapshot,
ordine-indipendente, invariante #3); **hazard** = fine-turno (v1) e anche attraversamento (v2).

## 3. Architettura — la logica dura resta PURA (rilievo panel)

Nessuna regola di gioco nuova nasce dentro un Actor: tutto in **librerie/USTRUCT puri**, testabili senza mondo.

- `FRTTerrainProps` (USTRUCT plain): specchio *gameplay* del terreno (MoveCost, bBlocksMove, bBlocksVision,
  CrossDamage, EndTurnDamage, buff…). Le funzioni pure lavorano su questo, **non** su `UObject*`.
- Pathfinding pesato: `URTGridLibrary` riceve il **costo per cella** come dati (mappa/array), non asset.
- **v2** — `URTMovementResolver` (già puro) esteso a `ResolvePaths(...)`: dato per unità un `PlannedPath`,
  ritorna *(cella finale, celle attraversate)* con troncamento **ordine-indipendente** (vedi §7). Il
  `CrossDamage` deriva dalle celle attraversate ritornate. L'Actor solo orchestra.

## 4. I terreni (valori proposti, tunable)

| Terreno | Asse | Effetto | Valori | Slice |
|---|---|---|---|---|
| **Fango** | costo | attraversamento più caro | `MoveCost 2` (=1+`ExtraMoveCost 1`) | v1 |
| **Cespuglio** | vista | blocca LOS, non il movimento | `bBlocksVision=true` | v1 |
| **Altura** | buff | +danno a chi ci sta (pos. **pre-movimento**, §6) | `OccupantDamageBonus +10` | v1 |
| **Lava** | hazard | danno a chi **termina** (v1); anche attraversando (v2) | `EndTurnDamage 20`; `CrossDamage 10` (v2) | v1/v2 |
| **Erba secca** | dinamico | infiammabile → Fuoco via skill | `bFlammable`, `IgnitesTo=Fuoco` | v1 |
| *Fuoco* (stato) | hazard temp. | come Lava, `Duration 2`, poi Erba bruciata | `EndTurnDamage 15` (+`CrossDamage 8` in v2) | v1 |
| **Muro** (= copertura attuale) | bloccante | blocca movimento+vista | `bBlocksMovement + bBlocksVision` | v1 |

## 5. Modello dati

```
UCLASS URTTerrainData : UPrimaryDataAsset
  FText DisplayName;  FLinearColor DisplayColor;
  int32 ExtraMoveCost = 0;   bool bBlocksMovement=false;  bool bBlocksVision=false;
  int32 CrossDamage=0; int32 EndTurnDamage=0;  FGameplayTag StatusOnEnter; int32 StatusDuration=0;
  int32 OccupantDamageBonus=0; int32 OccupantRangeBonus=0; int32 OccupantDefenseBonus=0;
  bool  bFlammable=false; URTTerrainData* IgnitesTo=nullptr;
  int32 TransientDuration=0; URTTerrainData* RevertsTo=nullptr;
// FRTTerrainProps: stesso set gameplay in USTRUCT plain, per le funzioni pure.
```
`ARTGridActor`: `TMap<FRTGridCoord, TObjectPtr<URTTerrainData>>` (assente = normale, costo 1).
`BlockedCells` resta **vista derivata** (celle con `bBlocksMovement`) → i chiamanti attuali non si rompono.

## 6. Regole chiuse (erano sotto-specificate — rilievi panel)

- **Precedenza**: `bBlocksMovement` **vince** su `ExtraMoveCost` (una cella bloccante non ha costo, è
  non-attraversabile). `bBlocksVision` è indipendente dal movimento.
- **Double-dip hazard** (v2): entrare in una cella = `CrossDamage`; restarci a fine turno = `EndTurnDamage`.
  La **cella finale li prende entrambi** (ci entri *e* ci resti). In v1 esiste solo `EndTurnDamage`.
- **Timing buff Altura**: valutato in *Blast* → usa la posizione **pre-movimento**. Chi sale sull'altura in
  questo turno ne beneficia **dal prossimo**. (Conseguenza della timeline; esplicitata.)
- **`StatusOnEnter`**: applicato quando l'unità **occupa** la cella a fine turno (Cleanup), coerente con
  `EndTurnDamage` (in v1; in v2 anche all'attraversamento se si deciderà).
- **Re-ignite**: incendiare una cella già in Fuoco **rinnova** `TransientDuration` (refresh, come gli status).
- **Waypoint oltre budget** (v2): un waypoint che porta il costo totale **> MoveRange** viene **rifiutato**
  (feedback UI), non troncato. L'auto-route a destinazione singola resta sempre valido entro budget.
- **Viz "path da eseguire" vs privacy #6**: pre-lock si mostra il path **inteso** (proprio); il path
  **risolto** (eventualmente troncato dalle mosse altrui) si mostra **solo dopo** il lock/risoluzione.

## 7. Determinismo (v2 — la parte difficile, requisito esplicito)

`ResolvePaths` deve essere **ordine-indipendente**. Algoritmo previsto: **microstep sincroni** — tutte le
unità avanzano di **1 cella** lungo il proprio `PlannedPath`; si risolvono le collisioni del microstep
(cella contesa → contendenti fermi da lì in poi; cella occupata da unità ferma → bloccata; scambio →
consentito), si ripete finché nessuno avanza. Le celle *effettivamente entrate* da ciascuna unità sono
l'output per il `CrossDamage`. **Requisito testabile**: permutare l'ordine delle richieste non cambia
(finale, celle-attraversate) di nessuna unità. Costo intero (R3) → hash deterministico (invariante #4).

## 8. Requisiti (SMART)

- `FR-TERRAIN-01..06` — terreno data-driven; costo (Fango); vista (Cespuglio); buff (Altura); hazard
  fine-turno (Lava, v1); ignite→Fuoco stesso-turno + reversione (v1).
- `FR-PATH-06..08` — A\* a costo minimo; reachability per **budget di costo**; euristica ammissibile
  (`Manhattan × minCost`).
- **(v2)** `FR-PATH-09` — `PlannedPath` waypoint: costo totale ≤ MoveRange; add/remove step; auto-route fra waypoint.
- **(v2)** `FR-PATH-10` — `ResolvePaths` esegue/tronca i path, ordine-indipendente (§7); celle attraversate → `CrossDamage`.
- **(v2)** `FR-VIZ-01` — 3 viz: auto-route, path in editing, path risolto (post-lock).

### Esempi (Given/When/Then)
```
[Costo] Given Fango (costo 2) su (5,4)(5,5); resto 1; unita' (5,3) budget-costo 4
        When  ReachableCellsByCost
        Then  (5,6) [costo 2+2=4 sul Fango... ] valutato per costo, non per passi

[Buff]  Given Altura (+10 danno) su (3,3); unita' A attacca da (3,3) in Blast
        When  A si trova su (3,3) PRIMA del movimento
        Then  il danno di A e' +10 (se A ci sale questo turno, niente bonus fino al prossimo)

[Ignite]Given Erba secca su (4,4); abilita' Area colpisce (4,4) in Blast
        When  risoluzione Blast
        Then  (4,4) diventa Fuoco per 2 turni; in Cleanup chi vi termina subisce EndTurnDamage
```

## 9. ⚠️ Conflitti IdeeBase vs canone (segnalati)
`IdeeBase.pdf`: turni **alternati**, **GAS**, **Punti Azione** → divergono dal canone (simultanei, no-GAS,
movimento a range). **Prevale il canone**; si adottano solo le idee di terreno, rimappate (niente PA; il
costo consuma il budget di movimento; effetti deterministici).

## 10. Piano d'implementazione (TDD)

### Terreno v1 — ✅ COMPLETO (2026-08-02, 46 test verdi)
1. ✅ `URTTerrainData` + `FRTTerrainProps` + **derivazione pura** props→(costo/blocco/vista) — TDD.
2. ✅ **PF.3**: `ReachableCellsByCost`/`FindPathByCost` (Dijkstra) + reachability-per-costo — TDD. *Fango conta.*
3. ✅ `ARTGridActor` mappa cella→terreno (+ `BlockedCells` vista derivata); wiring pathfinding/resolver/LOS pesati — **Cespuglio**.
4. ✅ Buff occupante in `ResolveCombat` (pos. pre-movimento) — **Altura** — TDD (`EffectiveAttackPower`).
5. ✅ Hazard **fine-turno** (Cleanup) + ignite stesso-turno + reversione — **Lava/Erba/Fuoco**.
6. ✅ Rendering celle colorate (`RefreshTerrainVisuals`, M_Unit MID) + bot cost/hazard-aware (`BuildBotCostMap`).
7. ✅ Verifica PIE: terreno a schermo corretto; log confermano costo Fango attivo + loop combattimento intatto.

### Movimento v2 (successivo)
8. `URTMovementResolver::ResolvePaths` (microstep, ordine-indipendente) — *TDD* (permutazione ordine · troncamento · celle-attraversate).
9. `PlannedPath` waypoint (add/remove, budget, auto-route) + `CrossDamage` + double-dip — *TDD/wiring*.
10. Le 3 viz (auto-route, editing, risolto post-lock).

## 11. Aperte / da bilanciare
Valori esatti (costi/danni/bonus) tunabili dopo playtest · Altura +danno vs +portata · migrazione copertura→"Muro"
(proposto: vista derivata).
