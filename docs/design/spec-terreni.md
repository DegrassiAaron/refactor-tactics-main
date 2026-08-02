# Brief / Spec — Tipologie di terreno (data-driven) + terreno dinamico

> Discovery `/sc:brainstorm` del **2026-08-02**. Fonti: scelte utente + `IdeeBase.pdf` (idee terreno) +
> canone ([`piano-canonico-mvp.md`](piano-canonico-mvp.md)) + spec pathfinding
> ([`spec-pathfinding.md`](spec-pathfinding.md), [`spec-pathfinding-pf3-pf4.md`](spec-pathfinding-pf3-pf4.md)).
> Autorità: subordinato al piano canonico. `IdeeBase.pdf` = ispirazione north-star (vedi conflitti §7).

## 1. Obiettivo e scope

Sistema di terreno **data-driven** (`URTTerrainData : UPrimaryDataAsset`) capace di quattro assi di effetto
— **costo**, **hazard**, **bloccante/vista**, **buff** — più **terreno dinamico** (campo infiammabile che
una skill può incendiare). Set concreto: **5 tipi** (+ lo stato "Fuoco" derivato). Include **PF.3** (A\* pesato),
perché il terreno a **costo** lo richiede. Scelta utente: **tutto insieme** (non affettato).

Realizza il pilastro **"mappa come sistema di gioco"** e apre `FR-TERRAIN-01` (gate di PF.3).

## 2. I terreni (valori proposti, tunable)

| Terreno | Asse | Effetto | Valori proposti | Riusa |
|---|---|---|---|---|
| **Fango** | costo | attraversamento più caro | `ExtraMoveCost +1` (cella costa 2) | PF.3 (A\* pesato) |
| **Lava** | hazard | danno a chi **termina il turno** sopra | `EndTurnDamage 20` | `ApplyDamage` |
| **Altura** | buff | bonus a chi ci sta sopra | `OccupantDamageBonus +10` (alt: `+1 AttackRange`) | combat |
| **Cespuglio** | bloccante-vista | blocca la LOS, **non** il movimento | `bBlocksVision=true` | `HasLineOfSight` |
| **Erba secca** | dinamico | infiammabile: una skill la trasforma in **Fuoco** | `bFlammable=true`, `IgnitesTo=Fuoco` | mutazione terreno |
| *Fuoco* (stato) | hazard temp. | come Lava ma a tempo, poi torna Erba (bruciata) | `EndTurnDamage 15`, `Duration 2 turni` | `ApplyDamage` + tick |

La **copertura** attuale (`BlockedCells`, blocca movimento+vista) diventa un terreno "Muro"
(`bBlocksMovement=true, bBlocksVision=true`) nello stesso sistema.

## 3. Modello dati

```
UCLASS URTTerrainData : UPrimaryDataAsset
  FText          DisplayName
  FLinearColor   DisplayColor          // rendering per-cella
  int32          ExtraMoveCost = 0      // COSTO (additivo intero — R3); costo cella = 1 + ExtraMoveCost
  bool           bBlocksMovement = false// BLOCCANTE
  bool           bBlocksVision   = false// BLOCCANTE-vista
  int32          EndTurnDamage = 0      // HAZARD (a fine turno su chi occupa)
  FGameplayTag   StatusOnEnter          // HAZARD alternativo (es. Slow) …
  int32          StatusDuration = 0     //   … con durata
  int32          OccupantDamageBonus = 0// BUFF
  int32          OccupantRangeBonus  = 0// BUFF
  int32          OccupantDefenseBonus= 0// BUFF (riduzione danno)
  bool           bFlammable = false     // DINAMICO: può essere incendiato
  URTTerrainData* IgnitesTo = nullptr   //   → diventa questo terreno (Fuoco)
  int32          TransientDuration = 0  // se >0, terreno temporaneo: torna al base dopo N turni
  URTTerrainData* RevertsTo = nullptr   //   → torna a questo (Erba bruciata)
```

**Griglia:** `ARTGridActor` passa da `TArray<FRTGridCoord> BlockedCells` a una **mappa cella→terreno**
(`TMap<FRTGridCoord, TObjectPtr<URTTerrainData>>`, assente = terreno normale costo 1). `BlockedCells` diventa
una *vista derivata* (celle con `bBlocksMovement`), per non rompere i chiamanti esistenti in un colpo solo.

## 4. Cambi ai sistemi (per componente)

- **Pathfinding (PF.3):** `ReachableCells`/`FindPath` da BFS uniforme a **Dijkstra/A\* pesato** su una
  funzione di costo per-cella (seam già prevista). Reachability entro **budget di costo**. Funzioni pure:
  ricevono costo/blocchi come dati (TMap/lambda), **non** `UObject*` → restano testabili senza asset.
- **LOS:** `HasLineOfSight` usa i **vision-blocker** derivati dal terreno (`bBlocksVision`), non più solo `BlockedCells`.
- **Hazard:** a fine turno (fase Move/Cleanup) chi occupa una cella con `EndTurnDamage` subisce danno
  (`ApplyDamage`, "raccogli-poi-applica", deterministico). Come per gli status esistenti.
- **Buff occupante:** in `ResolveCombat`, il danno/portata dell'attaccante (e/o la difesa del bersaglio)
  leggono i bonus della cella occupata.
- **Terreno dinamico (ignite):** un'abilità con `IgniteEffect` trasforma le celle **infiammabili** nella sua
  area (usa la `Shape` dell'abilità) in `IgnitesTo` per `TransientDuration` turni; poi `RevertsTo`. La
  mutazione avviene in `ResolveCombat` (Blast), deterministica. Il conto alla rovescia scala a fine turno.
- **Abilità → terreno:** `URTAbilityData` guadagna un campo opzionale per l'effetto-terreno (ignite).
  Es. "Freccia Infuocata" [IdeeBase p.3]: danno + incendia la cella colpita.
- **HUD/rendering:** celle colorate per `DisplayColor` (ISM/material dell'`ARTGridActor`); la preview del
  percorso (PF.2) già mostrerà la deviazione attorno al terreno costoso/bloccante.
- **Bot:** già path-aware; con PF.3 userà il costo (evita il Fango) e dovrà **evitare Lava/Fuoco** (hazard)
  — piccola estensione allo scoring.

## 5. Determinismo & invarianti

- Costo **intero** (R3) → path e hash deterministici (**invariante #4**).
- Il **terreno mutato** (Fuoco) è parte dello **snapshot** del turno: la mutazione e i suoi tick seguono
  "raccogli-poi-applica"; nessuna dipendenza dall'ordine dei container. È la versione MVP del concetto
  north-star `CostRevision` [Piano completo, p.14] — senza cache (100 celle).
- Autorità server: il costo/raggiungibilità si validano nel resolver (già fatto in PF.1), ora **pesati**.

## 6. Requisiti (SMART, sintesi)

- `FR-TERRAIN-01` — esiste ≥1 terreno con `ExtraMoveCost > 0` (Fango). *(apre PF.3)*
- `FR-TERRAIN-02` — il terreno è definito da `URTTerrainData` (data-driven); nuovi tipi = nuovi asset.
- `FR-TERRAIN-03` — hazard: chi termina il turno su `EndTurnDamage>0` subisce quel danno (test resolver).
- `FR-TERRAIN-04` — vista: `bBlocksVision` blocca la LOS ma `bBlocksMovement=false` **non** blocca il moto.
- `FR-TERRAIN-05` — buff: l'occupante di una cella-buff ottiene i bonus in combattimento.
- `FR-TERRAIN-06` — ignite: un'abilità con effetto-terreno converte le celle infiammabili nella sua area;
  il Fuoco dura `TransientDuration` turni poi torna a `RevertsTo`. Deterministico.
- `FR-PATH-06..08` (da `spec-pathfinding-pf3-pf4.md`) — A\* a costo minimo, reachability per costo, euristica ammissibile.

## 7. ⚠️ Conflitti IdeeBase vs canone (segnalati)

`IdeeBase.pdf` propone **turni alternati** (Fire Emblem), **GAS**, **Punti Azione** — tutti divergenti dal
canone (turni **simultanei** Prep→Dash→Blast→Move, **no-GAS**/`URTAbilityData`, movimento **a range**).
**Prevale il canone**; si adottano solo le *idee di terreno*, rimappate: niente PA (il costo consuma il
budget di movimento a range), danno hazard a fine turno, effetti deterministici, ability-terrain via `URTAbilityData`.

## 8. Piano d'implementazione (TDD, incrementale anche se "tutto insieme")

Ordine a rischio crescente, ogni passo build+test verde prima del successivo:
1. `URTTerrainData` + `ARTGridActor` mappa cella→terreno (+ `BlockedCells` come vista derivata).
2. **PF.3**: costo pesato in `ReachableCells`/`FindPath` (Dijkstra/A\*), euristica ammissibile — TDD (path a
   costo minimo, reachability per costo). *Fango diventa significativo.*
3. LOS da terreno (`bBlocksVision`) — **Cespuglio**.
4. Hazard a fine turno (`EndTurnDamage`) — **Lava** — TDD sul resolver.
5. Buff occupante in `ResolveCombat` — **Altura** — TDD.
6. Terreno dinamico: ability→ignite, Fuoco temporaneo + reversione — **Erba secca/Fuoco** — TDD.
7. Rendering celle colorate + estensione scoring bot (evita Fango costoso e Lava/Fuoco).
8. Verifica PIE end-to-end.

## 9. Domande aperte / da bilanciare

- Valori esatti (ExtraMoveCost, EndTurnDamage, bonus Altura) → tunabili dopo playtest.
- Altura: **+danno** o **+portata**? (proposto +danno; +portata è più leggibile ma più forte).
- Il Fuoco danneggia **anche** chi lo attraversa o solo chi vi **termina** il turno? (proposto: solo fine turno, coerente con Lava).
- La copertura esistente va migrata subito a terreno "Muro" o lasciata come vista derivata? (proposto: vista derivata, migrazione morbida).
