# Spec — Pathfinding sul grafo tattico esagonale

> `CURRENT` · **Stato**: as-built, allineata al codice il **2026-08-08** · **Owner**: questo file
> **Autorità**: subordinata a [`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) e a
> [ADR-0002](../decisions/adr-0002-griglia-esagonale.md).
>
> Si legge **senza conoscere la migrazione**: qui non si parla di quadrato. Il corpo originale, scritto per
> `FRTGridCoord` a 4 vicini e distanza Manhattan, è conservato in
> [`spec-pathfinding-pf3-pf4-quadrato.md`](../archive/technical/spec-pathfinding-pf3-pf4-quadrato.md) — le sigle
> **PF.3/PF.4** vengono da lì e sopravvivono solo nel nome di questo file.

---

## 1. Che cosa decide questo documento

Come si calcola un percorso fra due celle e quali archi esistono. È **autorevole**: il movimento del gioco
passa da qui, non dalla NavMesh di Unreal, che non conosce né le celle né le regole tattiche.

Owner del codice: `URTHexPathLibrary` (`Source/RefactorTactics/Pathfinding/`).

---

## 2. Il grafo

La mappa **non** è una matrice di celle adiacenti per posizione: è un grafo, e gli archi sono di due specie.

| Specie di arco | Come nasce | Costo |
|---|---|---|
| **Vicinato orizzontale** | **calcolato**, non memorizzato: i 6 vicini di `URTHexLibrary::Neighbors` sullo stesso `Layer` | `MoveCost` della cella di **destinazione** |
| **Transizione fra layer** | **dato esplicito** nell'asset (`FRTHexEdge` in `URTHexMapAsset::Transitions`, via `AddTransition(From, To, Cost, Kind, bBidirectional)`) | costo dichiarato dall'arco |

Che gli adiacenti orizzontali siano *calcolati* non è un dettaglio implementativo: è la ragione per cui una
trappola su transizione possiede la propria coppia `(From → To)` invece di appenderla alla mappa
([D-013](../decisions/RT_PDR_00_Decision_Log.md)). Non c'è un arco su cui appenderla.

`URTHexPathLibrary::GraphNeighbors(Map, Cell)` restituisce i vicini percorribili in **ordine deterministico**:
prima le sei direzioni nell'ordine `E, NE, NW, W, SW, SE`, poi le transizioni nell'ordine dell'asset.

### 2.1 Quando un arco orizzontale non esiste

Un vicino è scartato se:

1. la cella di destinazione ha `bBlocksMovement`; **oppure**
2. `URTHexCoverLibrary::BlocksTraversal(Map, From, To)` è vero — cioè fra le due celle c'è una **copertura
   alta**, che è una proprietà del **bordo**, non della cella.

Il secondo punto è la ragione per cui la copertura alta si comporta come un muro: lo **stesso** predicato è
consultato da `URTHexVisionLibrary` per la LOS. Movimento e vista non possono divergere sulla domanda «questo
bordo è chiuso?», perché non esistono due risposte.

---

## 3. L'algoritmo

**A\* deterministico a costi interi.**

```
FindPath(Map, Start, Goal, MaxCost = 0, MaxNodes = 100000)
FindPathAvoiding(Map, Start, Goal, Blocked, MaxCost = 0, MaxNodes = 100000, ExtraCostPerCell = 0)
```

| Elemento | Scelta | Perché |
|---|---|---|
| Euristica | distanza esagonale (cubica) | ammissibile finché gli archi sono locali: non sovrastima mai |
| Costi | **interi**, mai `float` | invariante #4: due macchine devono ottenere lo stesso percorso |
| Tie-break | **ID della cella**, ordinamento stabile | senza di esso l'esito dipenderebbe dall'ordine di iterazione di `TSet`/`TMap`, che non è garantito |
| `MaxCost` | `0` = illimitato | serve al budget di movimento (MP) |
| `MaxNodes` | guardia, default `100000` | un grafo malformato non deve poter appendere il turno |

### 3.1 Ostacoli dinamici e chi si muove

`FindPathAvoiding` separa due cose che il quadrato confondeva:

- **`Blocked`** — celle non percorribili che **non appartengono all'asset**: tipicamente le unità che le
  occupano. Non sono dati di mappa, quindi non stanno nella mappa. `Blocked == nullptr` equivale a `FindPath`;
  se il *goal* è bloccato il risultato è `NoPath`.
- **`ExtraCostPerCell`** (`>= 0`) — sovrapprezzo su **ogni** arco attraversato (CP 4.7, `Action.Slow`). È un
  parametro **del chiamante**, non della mappa: la stessa cella costa diversamente a un'unità rallentata e a
  una che non lo è. Metterlo nella mappa avrebbe reso il costo una proprietà del terreno anziché di chi cammina.

---

## 4. Determinismo, revisione e cache

- Il risultato dipende **solo** da `(Map, Start, Goal, Blocked, MaxCost, MaxNodes, ExtraCostPerCell)`. Nessun
  `DeltaTime`, nessun RNG, nessuna dipendenza dall'ordine di container non ordinati.
- `URTHexMapAsset::Revision` si incrementa a ogni modifica strutturale della mappa. **Oggi non invalida una
  cache di percorsi, perché una cache di percorsi non esiste**: `URTHexPathLibrary` ricalcola sempre. La
  revisione è consumata dallo **snapshot** di simulazione (`FRTHexSimSnapshot::Revision`), che confronta
  revisione **e** hash della mappa per accorgersi di essere obsoleto.
- Se un giorno servisse una cache, `Revision` è il gancio già presente — ma finché non c'è, questo documento
  non deve raccontarla.

---

## 5. Prestazioni

Misurate, non stimate. Test `RefactorTactics.Perf.PathfindingMedian`:

| Metrica | Valore |
|---|---|
| Mediana di `FindPath` | **0,025 ms** |
| Mediana del resolver di turno (contesto) | **0,41 ms/turno** (`RefactorTactics.Perf.TurnResolverMedian`) |

I numeri valgono sulla macchina di sviluppo e sulle mappe correnti: l'hardware target non è definito
([`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md)), quindi sono **misure**, non garanzie.

---

## 6. Che cosa questo documento **non** possiede

| Tema | Owner |
|---|---|
| Struttura della mappa, layer, transizioni come contenuto | [`spec-mappa-multilivello.md`](spec-mappa-multilivello.md) |
| Copertura: tipi, integrità, distruzione | [`../gameplay/spec-copertura-cp91.md`](../gameplay/spec-copertura-cp91.md) · [`../gameplay/spec-copertura-alta-cp92.md`](../gameplay/spec-copertura-alta-cp92.md) |
| Budget di movimento, profili `Sneak/Move/Sprint`, collisioni simultanee | [`../gameplay/spec-sequenza-turno.md`](../gameplay/spec-sequenza-turno.md) · [`../balance/RT_ActionCatalog_v0.1.md`](../balance/RT_ActionCatalog_v0.1.md) |
| Mappa delle classi C++ | [`architettura-codice.md`](architettura-codice.md) |
| Stato di avanzamento | [`../roadmap/roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md) |
