# Brief / Spec — Terreni (data-driven) + terreno dinamico + path composita

> Discovery `/sc:brainstorm` del **2026-08-02**. Fonti: scelte utente + `IdeeBase.pdf` (idee terreno) +
> canone ([`piano-canonico-mvp.md`](piano-canonico-mvp.md)) + spec pathfinding
> ([`spec-pathfinding.md`](spec-pathfinding.md), [`spec-pathfinding-pf3-pf4.md`](spec-pathfinding-pf3-pf4.md)).
> Autorità: subordinato al piano canonico. `IdeeBase.pdf` = ispirazione north-star (vedi conflitti §8).

## 1. Obiettivo e scope

Sistema di terreno **data-driven** (`URTTerrainData`) su quattro assi (**costo · hazard · bloccante/vista ·
buff**) + **terreno dinamico** (campo infiammabile → Fuoco via skill) + **path composita a waypoint** con tre
visualizzazioni. Include **PF.3** (A\* pesato) perché il terreno a **costo** lo richiede. Scelta utente:
**tutto insieme** (è un'epica; sotto un ordine interno incrementale e verificabile).

Realizza il pilastro **"mappa come sistema di gioco"** e apre `FR-TERRAIN-01`.

## 2. Timeline canonica del turno — dove scatta ogni cosa (determinismo)

In turni **simultanei**, "prima/dopo" **non è ambiguo**: ogni effetto ha uno **slot fisso** nella sequenza di
fasi. È la risposta deterministica alla domanda "il trigger della zona va prima o dopo attacco/movimento".

```
Lock-in
 → Prep    : buff su sé (Barriera)                                   [già presente]
 → Dash    : mobilità rapida                                         [futuro]
 → Blast   : attacchi  +  IGNITE terreno (le skill mutano la mappa QUI; simultanei da snapshot)
 → Move    : esecuzione dei path compositi  +  danno hazard "da ATTRAVERSAMENTO" (per cella entrata)
 → Cleanup : danno hazard "di FINE TURNO" (su chi OCCUPA)  +  tick durate (Fuoco/status)  +  reversione terreno
```

**Decisioni fissate:**
- **Hazard = ENTRAMBI**: danno per ogni cella pericolosa **attraversata** (in Move) **e** danno a chi vi
  **termina** il turno (in Cleanup). L'attraversamento e la sosta fanno male.
- **Ignite = STESSO TURNO**: la skill incendia in *Blast*; il Fuoco affligge già il *Move* di questo turno
  (Blast precede Move). Combo letali; il nemico ha pianificato "alla cieca" (natura del gioco simultaneo).
- **Attacco + ignite** nello stesso *Blast* sono **simultanei** ("raccogli-poi-applica" da snapshot,
  ordine-indipendente — invariante #3).

## 3. Path planning (composita + tre visualizzazioni)

Il piano di movimento passa da *cella-destinazione* a **`PlannedPath` (lista ordinata di celle)**.

| Feature | Comportamento |
|---|---|
| **Path automatico (viz)** | click su una cella → auto-route A\* pesato (`FindPath`); la rotta è mostrata in pianificazione (estende PF.2 al costo). |
| **Path composita (waypoint ibrido)** | click aggiuntivi = **aggiungi waypoint** (forza il passaggio, l'A\* raccorda i tratti); undo / tasto = **togli l'ultimo** step. Vincolo: **costo totale del path ≤ MoveRange**. |
| **Path da eseguire (viz)** | dopo il lock-in, la rotta che verrà percorsa; può **troncarsi** in risoluzione per conflitti → la viz mostra fin dove si arriva davvero. Privacy #6: solo la propria pre-lock (nemica solo se rivelata). |

## 4. I terreni (valori proposti, tunable)

| Terreno | Asse | Effetto | Valori proposti |
|---|---|---|---|
| **Fango** | costo | attraversamento più caro | `ExtraMoveCost +1` (cella costa 2) |
| **Lava** | hazard | danno **attraversando** e a **fine turno** | `CrossDamage 10`, `EndTurnDamage 20` |
| **Altura** | buff | bonus a chi ci sta | `OccupantDamageBonus +10` (alt: `+1 AttackRange`) |
| **Cespuglio** | bloccante-vista | blocca LOS, non il movimento | `bBlocksVision=true` |
| **Erba secca** | dinamico | infiammabile → Fuoco via skill | `bFlammable=true`, `IgnitesTo=Fuoco` |
| *Fuoco* (stato) | hazard temp. | come Lava, a tempo, poi Erba bruciata | `CrossDamage 8`, `EndTurnDamage 15`, `Duration 2` |

La **copertura** attuale diventa un terreno "Muro" (`bBlocksMovement + bBlocksVision`) nello stesso sistema.

## 5. Modello dati

```
UCLASS URTTerrainData : UPrimaryDataAsset
  FText DisplayName;  FLinearColor DisplayColor;
  int32 ExtraMoveCost = 0;            // COSTO (additivo intero — R3); costo cella = 1 + ExtraMoveCost
  bool  bBlocksMovement = false;      bool bBlocksVision = false;   // BLOCCANTE / vista
  int32 CrossDamage = 0;             // HAZARD: danno per cella attraversata (Move)
  int32 EndTurnDamage = 0;           // HAZARD: danno a chi occupa a fine turno (Cleanup)
  FGameplayTag StatusOnEnter; int32 StatusDuration = 0;  // HAZARD alternativo (es. Slow)
  int32 OccupantDamageBonus = 0; int32 OccupantRangeBonus = 0; int32 OccupantDefenseBonus = 0; // BUFF
  bool  bFlammable = false;  URTTerrainData* IgnitesTo = nullptr;   // DINAMICO
  int32 TransientDuration = 0;  URTTerrainData* RevertsTo = nullptr;// terreno temporaneo → reversione
```

**Griglia:** `ARTGridActor` passa da `TArray<FRTGridCoord> BlockedCells` a `TMap<FRTGridCoord,
TObjectPtr<URTTerrainData>>` (assente = normale, costo 1). `BlockedCells` resta come **vista derivata**
(celle con `bBlocksMovement`) per non rompere i chiamanti in un colpo solo.

## 6. Cambi ai sistemi

- **Pathfinding (PF.3):** `ReachableCells`/`FindPath` → **A\* pesato** su costo per-cella (seam pronta);
  reachability per **budget di costo**; euristica ammissibile. Funzioni pure: ricevono costo/blocchi come
  **dati** (TMap/lambda), non `UObject*` → restano testabili.
- **Movimento path-aware (⚠️ pezzo più delicato):** il resolver segue `PlannedPath` cella-per-cella; sui
  **conflitti** (cella contesa/occupata da unità ferma) **tronca** il path all'ultima cella valida
  (deterministico, ordine-indipendente — punto fisso come oggi ma su path). Le celle *effettivamente
  attraversate* determinano il `CrossDamage`.
- **LOS:** `HasLineOfSight` usa i vision-blocker derivati dal terreno (`bBlocksVision`).
- **Hazard:** `CrossDamage` in Move (per cella entrata), `EndTurnDamage` in Cleanup (`ApplyDamage`,
  raccogli-poi-applica).
- **Buff occupante:** in `ResolveCombat`, danno/portata/difesa leggono i bonus della cella occupata.
- **Terreno dinamico (ignite):** un'abilità con `IgniteEffect` muta le celle **infiammabili** nella sua
  `Shape` in `IgnitesTo` per `TransientDuration` turni (in Blast), poi `RevertsTo` (Cleanup). Deterministico.
- **Abilità → terreno:** `URTAbilityData` guadagna il campo effetto-terreno (ignite).
- **Rendering:** celle colorate per `DisplayColor`; le tre viz di path (§3).
- **Bot:** path-aware; con PF.3 evita il Fango (costo) e **Lava/Fuoco** (hazard) — estensione scoring.

## 7. Determinismo & invarianti

- Costo **intero** (R3) → path/hash deterministici (**invariante #4**).
- Terreno mutato (Fuoco) e path fanno parte dello **snapshot** del turno; mutazione, attraversamento, danni e
  tick seguono "raccogli-poi-applica"; nessuna dipendenza dall'ordine dei container. Versione MVP del concetto
  north-star `CostRevision` [Piano completo, p.14], senza cache (100 celle).
- Il **troncamento** del path su conflitto deve convergere a un punto fisso indipendente dall'ordine (come
  l'attuale `ResolveMoves`, esteso ai path) — da provare con test dedicati.

## 8. ⚠️ Conflitti IdeeBase vs canone (segnalati)

`IdeeBase.pdf` propone **turni alternati**, **GAS**, **Punti Azione** — divergono dal canone (turni
**simultanei**, **no-GAS**, movimento **a range**). **Prevale il canone**; si adottano solo le idee di
terreno, rimappate (niente PA; il costo consuma il budget di movimento; effetti deterministici).

## 9. Requisiti (SMART, sintesi)

- `FR-TERRAIN-01..06` (terreno data-driven, hazard cross+end, vista, buff, ignite) — vedi §4/§6.
- `FR-PATH-06..08` (A\* costo minimo, reachability per costo, euristica ammissibile).
- `FR-PATH-09` — `PlannedPath` (waypoint): costo totale ≤ MoveRange; add/remove step; auto-route fra waypoint.
- `FR-PATH-10` — il resolver esegue `PlannedPath`, tronca sui conflitti; le celle attraversate guidano `CrossDamage`.
- `FR-VIZ-01` — tre viz: auto-route, path composita in editing, path da eseguire (post-lock, troncabile).

## 10. Piano d'implementazione (TDD, ordine a rischio crescente)

Ogni passo build+test verde prima del successivo:
1. `URTTerrainData` + `ARTGridActor` mappa cella→terreno (+ `BlockedCells` vista derivata).
2. **PF.3**: A\* pesato in `ReachableCells`/`FindPath` (Dijkstra), reachability per costo — TDD. *(Fango conta)*
3. LOS da terreno (`bBlocksVision`) — **Cespuglio** — TDD.
4. Buff occupante in `ResolveCombat` — **Altura** — TDD.
5. **Movimento path-aware**: `PlannedPath` + resolver che segue/tronca — TDD (il pezzo delicato).
6. Hazard: `CrossDamage` (Move) + `EndTurnDamage` (Cleanup) — **Lava** — TDD.
7. Terreno dinamico: ability→ignite, Fuoco temporaneo + reversione (stesso turno) — **Erba/Fuoco** — TDD.
8. UI: path composita (waypoint ibrido) + tre viz + celle colorate; scoring bot evita costo/hazard.
9. Verifica PIE end-to-end.

## 11. Aperte / da bilanciare

Valori esatti (costi, danni, bonus) tunabili dopo playtest · Altura +danno vs +portata · migrazione copertura→"Muro"
(proposto: vista derivata) · comportamento del path composito quando un waypoint diventa irraggiungibile in editing.
