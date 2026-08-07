# Spec — Knockback (spinta)

> ⚠️ **Superato dal pivot esagonale** ([ADR-0002](../decisions/adr-0002-griglia-esagonale.md)) — **riferimento storico, non normativo.**
> Descrive il substrato **quadrato**, rimosso dal codice al **CP 7.2** (`Grid/`, `URTGridLibrary`, `FRTGridCoord`, resolver e bot quadrati). La spinta autorevole è oggi `URTHexCombatLibrary::HexKnockbackDestination` (sei direzioni esagonali, CP 6.5).
> Conservato per provenienza e come comportamento di riferimento della parità hex (M6). Punto di ritorno: tag `pre-hex-only`.

> Implementazione del **2026-08-03** (TDD per la logica pura, wiring + PIE per l'integrazione). Nuova meccanica
> di combattimento: un attacco **respinge** i bersagli colpiti nella fase Blast. Ancorata al canone
> ([`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md), invarianti #1/#4/#7).

## 1. Obiettivo

Profondità tattica: spingere un nemico **fuori dalla copertura**, **giù dal ponte**, o **nella lava** (danno da
attraversamento). Si risolve nel **Blast** (dopo il danno), sulle posizioni snapshot del turno.

## 2. Logica pura (`URTCombatLibrary::KnockbackDestination`)

`KnockbackDestination(Attacker, Target, Distance, Blocked, Width, Height) -> FRTGridCoord`:
- Direzione **cardinale** di allontanamento dall'attaccante: l'asse col delta maggiore (a parità, l'asse X).
- Avanza fino a `Distance` celle; si ferma sulla cella **libera** prima di un ostacolo (`Blocked`) o del **bordo**.
- Preserva il **layer** del bersaglio (spinta orizzontale). `Distance <= 0` o attaccante sulla stessa cella →
  il bersaglio resta. Deterministica.
- **8 test** (`RTCombatLibraryTests.cpp`, RED→GREEN): spinta libera V/H, ostacolo, bordo, distanza 0, stessa
  cella, asse dominante, layer preservato.

## 3. Modello dati

`URTAbilityData::bKnockback` + `KnockbackDistance` (celle). Assegnati alla **"Spazzata"** del Guardian (cono,
knockback 2 celle): il Guardian ora è anche **controllo folla** (spinge i colpiti).

## 4. Risoluzione (`ResolveCombat`, fase Blast)

Dopo l'applicazione del danno, sulle posizioni **snapshot**:
1. Raccoglie gli intenti di spinta per bersaglio (cella attaccante + distanza + **conteggio attaccanti**).
2. **Determinismo**: un bersaglio spinto da **2+** attaccanti (forze contraddittorie) **non** viene spinto.
   Bloccanti = ostacoli + **celle di tutte le unità** (non si spinge dentro un'altra unità).
3. Calcola le destinazioni (dallo snapshot, ordine-indipendenti); **destinazioni contese** (2+ bersagli verso
   la stessa cella) → quei bersagli **restano** (come i conflitti di movimento).
4. Applica: aggiorna `GridCell` + visuale; se il bersaglio non aveva un move pianificato (`PlannedCell == cella
   pre-spinta`) imposta `PlannedCell = destinazione` (niente ritorno indietro nel Move); **cross-damage** lungo
   le celle attraversate (spinto nella lava); morte → evento `Defeated` (fase Blast).
5. **Animazione** (2026-08-03): la spinta è emessa come **evento di movimento in fase Blast** → il bersaglio
   **scivola** `OldCell → NewCell` mentre i colpi sono mostrati. La fase Blast è ora "ibrida" (anima il
   movimento **e** rivela i colpi); la sua durata si estende per contenere lo scivolamento
   (`DurationForPlaybackPhase(Blast) = max(tempo colpi, tempo scivolamento)`).

## 5. Verifica

- **Regressione**: 63/63 automation test verdi (62 + 1 knockback).
- **PIE (log)**: `Spinta: RTUnit_0 -> (0,2)` — il Guardian respinge un nemico in gioco; nessun crash.

## 6. Limiti aperti

- Lo scivolamento è animato sull'intera durata della fase Blast: se i colpi durano più dello scivolamento,
  la spinta appare leggermente più lenta della velocità nominale (`PlaybackCellsPerSecond`). Cosmetico.
- Un bersaglio spinto da 2+ attaccanti annulla la spinta (scelta di determinismo, non fisica dei vettori).
- Tuning della distanza di spinta da tarare in gioco.
