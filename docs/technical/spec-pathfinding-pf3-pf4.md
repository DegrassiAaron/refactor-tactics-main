# Spec — Path Finding avanzato: PF.3 (cost provider + A* pesato) e PF.4 (grafo multilivello)

> Prodotta da un panel di revisione specifiche (`/sc:spec-panel`) il **2026-08-02**.
> Segue [`spec-pathfinding.md`](spec-pathfinding.md) (PF.1/PF.2, già implementati).
> Autorità: subordinata a [`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md). I riferimenti PDF = north-star.

## 0. Riconciliazioni recepite (decisioni 2026-08-02)

Le tre contraddizioni tra fonti sono state decise. **Da recepire nel piano canonico** (fonte di verità #1)
prima di implementare PF.3/PF.4.

| # | Contraddizione | Decisione | Motivazione |
|---|---|---|---|
| **R1** | Schema fasi turno | **`Planning → Prep → Dash → Blast → Move → Cleanup`** (quello del codice / Atlas) | Già implementato/testato (`ERTMatchPhase`, `URTTurnRules::NextPhase`). Gli schemi di [Piano completo, p.15] e [PRD, p.4] sono elaborazioni mappabili su questo. Il path finding serve a **Move** (normale) e **Dash** (mobilità rapida, profilo distinto). |
| **R2** | Naming coordinata / campo Z | **`FRTGridCoord` + campo `Layer`** | Nome struct già nel codice (=PRD); campo `Layer` = [Piano completo] + commento già presente in `RTTypes.h`. Estensione retro-compatibile: `int32 Layer = 0` → il 2D resta invariato; `GetTypeHash` includerà `Layer`. |
| **R3** | Modello di costo | **Additivo intero** | `TraversalCost = Σ costi interi dei provider` [Piano completo, p.13]. Interi → deterministici/hashabili (**invariante #4**). I `MovementMultiplier` float dei Data Asset [PRD, p.16] si convertono a intero al caricamento; **niente float nel resolver/hash**. |

## 1. Gate di scope (il panel raccomanda di NON implementare finché non si aprono)

| Incremento | Gate |
|---|---|
| **R1/R2/R3** | nessuno — sono decisioni, si recepiscono subito nel piano canonico |
| **PF.3** cost provider + A\* pesato | **`FR-TERRAIN-01`** — deve esistere ≥1 tipo di terreno con costo di traversata > 1. Senza un terreno a costo, l'A\* pesato non ha nulla da pesare (YAGNI). Il terreno è a sua volta una feature di gioco da volere per *ragioni di gameplay*. |
| **PF.4** grafo multilivello | Un **design di mappa multilivello** + una **meccanica** che lo usi (scale/portali/dislivelli). È il salto architetturale maggiore (da griglia implicita a grafo esplicito). North-star. |

## 2. PF.3 — Cost provider + A\* pesato (gated da FR-TERRAIN-01)

**Architettura (seam già presente):** `ReachableCells`/`FindPath` incapsulano già la nozione di passo.
Si sostituisce «costo 1 se libero / bloccato» con un provider di costo; il BFS diventa **Dijkstra/A\***.

- `ICostProvider::Step(From, To) → { int Cost | Blocked }`, con **`Cost ≥ 1`** (clamp; niente costi negativi).
- Primi due provider (non lo zoo di 10 del PRD): **Terrain** (costo per tipo di cella) e **Occupancy**
  (celle occupate/prenotate → bloccate o penalizzate).
- **Euristica ammissibile:** `H = Manhattan × minCost` con `minCost ≥ 1` (resta Manhattan se minCost=1)
  [Piano completo, p.14]. Tie-break deterministico (`StableTieBreak`).

**Requisiti (SMART):**
- `FR-PATH-06` — `FindPath` ritorna il percorso a **costo totale minimo** (non a celle minime): fra due rotte,
  preferisce quella più economica anche se più lunga in celle. *Verifica: terreno costoso vs giro economico.*
- `FR-PATH-07` — reachability entro un **budget di costo** (non di passi): `ReachableCellsByCost(From, Budget, …)`.
- `FR-PATH-02` (invariato) — aggiungere un provider **non** modifica l'algoritmo (criterio PRD [p.11,23]).
- `FR-PATH-08` — euristica ammissibile: il costo stimato non supera mai il costo reale (A\* ottimo).

**Esempi (Given/When/Then):**
```
Given  terreno "fango" costo 3 su (5,4)..(5,6); resto costo 1; unita' a (5,3), bersaglio (5,7)
When   FindPath a costo minimo
Then   preferisce il giro (5,3)->(4,3)->(4,4)->(4,5)->(4,6)->(4,7)->(5,7) [costo 6]
And    NON la retta (5,3)->(5,4)->(5,5)->(5,6)->(5,7) [costo 1+3+3+3+1 = 11]

Given  budget di costo 4, terreno come sopra
When   ReachableCellsByCost
Then   (5,6) [costo 1+3+3=7] NON e' raggiungibile; (2,3) [costo 3] si'
```

**Failure mode:** costo negativo → clamp a 0/rifiuto; costo statico nello snapshot del turno (nessun ricalcolo
mid-turno nell'MVP; `CostRevision` è north-star [Piano completo, p.14]); path assente → unità ferma.

**Test plan:** (1) path a costo minimo preferisce il giro economico *(discriminante)*; (2) ammissibilità
euristica (A\* trova l'ottimo su griglia nota); (3) determinismo con costi pari; (4) reachability-per-costo;
(5) provider aggiuntivo non cambia i risultati dell'algoritmo (FR-PATH-02).

## 3. PF.4 — Grafo multilivello (motore ✅ 2026-08-02 · mappa gated da design)

> **Motore consegnato (TDD, 52 test):** `FRTGridCoord{X,Y,Layer}` (retro-compatibile, Layer 0 = 2D);
> `FRTTraversalEdge{From,To,Cost}`; `URTGridLibrary::ReachableCellsByGraph`/`FindPathByGraph` (Dijkstra
> che espande i 4 vicini ortogonali same-layer **+** gli archi uscenti, tie-break `(X,Y,Layer)`).
> Verificato: portale che accorcia, scala tra layer, reachability cross-layer per costo.
> **Ancora ⏳** (gated da un design di mappa multilivello): rendering del 2° layer, scale/portali come
> oggetti di gioco, wiring del graph-pathfinding nel controller/resolver, camera e bot multilivello,
> `GraphRevision`/`SchemaVersion`. Sotto il design originario.

**Cambio di paradigma:** da griglia implicita (4 vicini) a **grafo esplicito** con archi. Il pathfinder itera
sugli **archi uscenti** invece che sui vicini ortogonali.

**Modello dati (recepisce R2):**
- `FRTGridCoord { int32 X, Y, Layer = 0 }` — `GetTypeHash` include `Layer`.
- `FRTTraversalEdge { FRTGridCoord From, To; EEdgeType EdgeType; int32 Cost; FGameplayTagContainer RequiredTags; bool bEnabled }`
  — scale/rampe/portali/ascensori/salti [Piano completo, p.12]. Celle collegate anche se non adiacenti.
- Versionamento: `GraphRevision` (mutazioni topologiche) + `SchemaVersion` (serializzazione) — **invariante #4**.

**Requisiti:** `FR-GRID-01` (≥3 livelli logici [PRD, p.22]); `FR-GRID-02` (archi con direzione/costo/requisiti/
enabled [PRD, p.23]); `FR-GRID-03` (le mutazioni incrementano `GraphRevision`).

**Compatibilità (Newman):** `Layer = 0` di default → i dati/percorsi 2D esistenti restano validi (migrazione
implicita). Nessun formato serializzato senza campo versione.

**Vertical slice minimo** [Piano completo, p.32]: 2 layer, collegamenti = scala + jump pad, 4 terreni, effetti
fuoco/acqua; dimostrare "movimento multilivello" e "un percorso diverso per due unità".

## 4. Non-goal / YAGNI (esplicito)

- Nessuno zoo di 10 cost provider: 2 all'inizio (Terrain, Occupancy).
- Nessuna cache/invalidazione per revisione finché non c'è profiling su mappe grandi (100 celle = irrilevante;
  budget `<50/100 ms` sono per 2.000–3.000 celle [PRD, p.23]).
- Nessun "percorso più sicuro" (rischio/esposizione) nel path mostrato al giocatore: resta comando esplicito/IA
  [Piano completo, p.14].
- PF.4 non parte senza una mappa multilivello disegnata e una meccanica che la usi.

## Appendice — citazioni PDF

`TraversalCost = EdgeBase+Terrain+UnitProfile+Status+Hazard+Reservation+Scenario` [Piano completo, p.13] ·
`H = Manhattan orizzontale + min cambi layer` [p.14] · `FRTTraversalEdge{From,To,EdgeType,BaseCost,RequiredTags,
BlockedTags,Capacity}` [p.12] · `FR-GRID-01..03`, `FR-PATH-02` [PRD, p.22-23] · budget `<50/100 ms` su
2.000–3.000 celle [PRD, p.23] · vertical slice 2 layer [Piano completo, p.32].
