# Spec — Path Finding (RefactorTactics)

> Prodotta da un panel di revisione specifiche (`/sc:spec-panel`) il **2026-08-02**.
> Autorità: subordinata a [`piano-canonico-mvp.md`](piano-canonico-mvp.md) e [`roadmap-checkpoint.md`](roadmap-checkpoint.md).
> I riferimenti ai PDF sono la **visione north-star**, non lo scope MVP (vedi CLAUDE.md, fonte di verità §4).

## Ambito deciso

**PF.1 + PF.2** (vedi §7). PF.1 chiude un bug attivo; PF.2 aggiunge la preview del percorso.
PF.3/PF.4 restano progettati-per ma fuori scope corrente.

---

## 1. Collocazione nella roadmap

- Il path finding **non è implementato come sistema**. Il movimento oggi valida solo
  `distanza Manhattan ≤ range` (`URTGridLibrary::IsWithinRange`) + rifiuto della cella-copertura
  come *destinazione* (`RTTurnManager::ResolveMovement`).
- `CLAUDE.md` lo elenca tra le priorità di test ("pathfinding/cost provider") ma la funzione non esiste.
- M2 (Turn loop) è ✅ per il *movimento base*; il path finding non è mai stato nello scope MVP realizzato.
- Nei PDF è esercizio/north-star: *"sostituisci la distanza semplice con una BFS che aggira i muri"*
  [TutorialToMVP, p.23]; curriculum `A* minimale → Cost provider → Grafo multilivello` [Piano completo, p.29].

## 2. As-is vs target — e il bug

| Dimensione | As-is (codice) | Target north-star (PDF) |
|---|---|---|
| Raggiungibilità | Manhattan ≤ range, **ignora ostacoli** | Reachability entro budget, aggira ostacoli [PRD, p.6] |
| Costo | uniforme implicito (1/cella) | `TraversalCost = EdgeBase+Terrain+UnitProfile+Status+Hazard+Reservation+Scenario` [Piano completo, p.13] |
| Algoritmo | nessuno | A\* custom, euristica ammissibile, `StableTieBreak` [Piano completo, p.13-14] |
| Ostacoli | solo LOS/attacchi (`HasLineOfSight`, `BlockedCells`) | archi/celle con costo e blocchi, provider [PRD, p.6] |
| Validazione server | range + cella-copertura | ricalcolo path: contiguità, arco attraversabile, costo ≤ budget [PRD, p.19; Intenti, p.8] |

**Bug attivo (🔴):** dopo M3.6 gli ostacoli bloccano la **linea di tiro** ma **non il movimento**.
Un'unità può "attraversare" una colonna (basta non atterrarci sopra), perché la validazione è
Manhattan sulla sola destinazione. È il caso d'uso che rende PF.1 necessario ora, non solo north-star.

## 3. Requisiti (SMART)

- **FR-PATH-01** — `ReachableCells(From, MoveRange, Blockers, W, H)` ritorna esattamente le celle con
  percorso ortogonale libero di lunghezza ≤ MoveRange (costo 1/passo, niente diagonali). *Verifica: griglia nota.*
- **FR-PATH-02** (design) — l'algoritmo interroga una funzione di costo `Step(a,b) → {Open(cost)|Blocked}`;
  sostituirla con costi pesati **non** deve cambiare l'algoritmo (criterio PRD FR-PATH-02 [p.11,23]).
  *Verifica: firma di costo isolata dietro una seam.*
- **FR-PATH-03** — `ResolveMovement` accetta una destinazione **solo se** ∈ `ReachableCells` dell'unità
  (autorità server); altrimenti l'unità resta ferma. *Verifica: destinazione dietro muro rifiutata.*
- **FR-PATH-04** — determinismo: stesso input → stesso output, ordine celle stabile. *Verifica: doppia chiamata.*
- **FR-PATH-05** (PF.2) — `FindPath(From, To, Blockers, W, H)` ritorna un percorso ortogonale minimo (BFS/costo
  uniforme) o vuoto se irraggiungibile; l'HUD lo disegna per l'unità selezionata in pianificazione. *Verifica: test + PIE.*

## 4. Esempi (Given/When/Then)

```
Given  griglia 10x10, colonne {(4,4),(5,4),(4,5),(5,5)}, unita' a (3,4), MoveRange 3
When   ReachableCells
Then   (6,4) NON raggiungibile (Manhattan=3, path reale intorno alla colonna costa 5)
And    (3,7) raggiungibile (percorso libero costo 3)

Given  il giocatore pianifica (6,4) come sopra
When   il turno si risolve
Then   l'unita' resta a (3,4) (destinazione non raggiungibile, autorita' server)

Given  unita' selezionata con destinazione pianificata raggiungibile (PF.2)
When   fase di pianificazione
Then   l'HUD disegna il percorso cella-per-cella fino alla destinazione
```

## 5. Architettura (seam per la crescita)

- `ReachableCells`/`FindPath` come **funzioni pure** in `URTGridLibrary` (coerente con la suite testabile).
- Costo dietro seam `Step(a,b)`: in PF.1/PF.2 ritorna sempre `Open(1)` salvo cella bloccante → `Blocked`.
  In PF.3 la stessa seam ospita i cost provider (terreno/occupazione) **senza toccare** l'algoritmo.
- Path preview del giocatore: **legale e prevedibile** — le regole tattiche NON deviano il percorso
  mostrato [Piano completo, p.14]. Il "percorso più sicuro" resta comando esplicito/IA (fuori scope).

## 6. Failure mode & determinismo

- Path assente → l'unità resta ferma (log). Snapshot ostacoli fissato al lock ("raccogli-poi-applica").
- Nessuna cache: griglia 10×10 = 100 celle → i budget PDF (`<50 ms` preview / `<100 ms` reachability su
  **2.000–3.000 celle** [PRD, p.23]) non sono un rischio MVP. *Nessuna ottimizzazione non misurata.*
- Ordine celle stabile (scansione deterministica) → invariante di determinismo #4.

## 7. Roadmap a gradini

| Gradino | Contenuto | Scope |
|---|---|---|
| **PF.1** | BFS reachability obstacle-aware + validazione resolver (FR-PATH-01..04) | **MVP — ora** |
| **PF.2** | `FindPath` (costo uniforme) + preview del percorso a schermo (FR-PATH-05) | **MVP+ — ora** |
| **PF.3** | Cost provider (terreno/occupazione) sulla seam + A\* pesato | vertical-slice |
| **PF.4** | Grafo multilivello (`Layer`, `FRTTraversalEdge`, scale/portali) | north-star |

## 8. Test plan (mappato al DoD)

Casi obbligatori (logica pura, TDD): (a) percorso libero = Manhattan; (b) **ostacolo a L** dove Manhattan
direbbe "raggiungibile" ma il path reale supera il budget → **non** raggiungibile *(caso discriminante)*;
(c) cella dietro muro chiuso → irraggiungibile a ogni range; (d) determinismo; (e) resolver rifiuta
destinazione con path invalido anche se la cella è libera; (f) `FindPath` percorso minimo attorno all'ostacolo;
(g) `FindPath` vuoto se irraggiungibile. Regressione: il bot (`BestApproachCell/KiteCell`) va reso coerente
con la reachability per non pianificare oltre-muro.

## 9. ⚠️ Contraddizioni tra fonti (da riconciliare prima di PF.3/PF.4)

> **Aggiornamento 2026-08-02:** le contraddizioni 1/2/3 sono state **decise** (R1 schema Atlas del codice ·
> R2 `FRTGridCoord`+`Layer` · R3 costo additivo intero) — vedi [`spec-pathfinding-pf3-pf4.md`](spec-pathfinding-pf3-pf4.md) §0.
> Da recepire nel piano canonico. 4/5 restano note informative (nessuna azione).

1. **Tre schemi di fasi:** `Prep→Dash→Blast→Move` [adottato] vs `Preparation→Movement→Actions→…`
   [Piano completo, p.15] vs `…→Mobilità rapida→Movimento→…` [PRD, p.4]. Impatta se il path finding
   serve solo a `Move` o anche a `Dash`.
2. **Naming struct:** `FRTGridCellId{X,Y,Layer}` [Piano completo] vs `FRTGridCoord{X,Y,Level}` [PRD] vs
   `FRTGridCoord{X,Y}` attuale (2D). Riconciliare prima del livello Z.
3. **Costo additivo vs moltiplicativo:** `+Terrain` [Piano completo, p.13] vs `MovementMultiplier ×2.0`
   [PRD, p.16].
4. **Mito "A\* < 2 ms":** quel budget è l'update-intento server a 8 player [Intenti, p.23], **non** l'A\*.
5. **MVP 2D vs multilivello:** il core MVP è 2D/Manhattan; multilivello/archi = north-star.

## Appendice — NFR north-star (citati, non gate MVP)

Path preview `<50 ms` (p95), reachability `<100 ms` (p95), aggiornamento post-mutazione `<100 ms`,
turn resolution server `<250 ms`, su mappa 2.000–3.000 celle [PRD, p.23]. Cache key: `Start, Goal,
TraversalProfileId, GraphRevision, CostRevision, Path` [Piano completo, p.14]. Rischio dichiarato:
"pathfinding lento → cache per revisione, query asincrone, provider semplici" [PRD, p.33].
