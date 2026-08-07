# Spec H6.4 — Linea di vista e forme di targeting esagonali

> **Stato**: **Implementata** (TDD RED→GREEN; suite **156/156**, build Editor Succeeded) · **Data**: 2026-08-05 ·
> **Branch**: `feat/hex-vision`
> **Fonte**: milestone **H6** di [`hex-map-roadmap.md`](../roadmap/hex-map-roadmap.md). Prerequisito dello switch di
> `ARTTurnManager` su hex. Non tocca il turn loop quadrato.

## 1. Perché adesso

Lo switch del turn loop su esagoni è **bloccato**: `ARTTurnManager::ResolveCombat` valida gli attacchi con
`URTGridLibrary::HasLineOfSight` e le abilità usano le forme `CellsInRadius/Line/Cone`. Su hex esiste solo
`HexArea` (il raggio). Portare il turn loop senza LOS né linea/cono significherebbe **perdere funzionalità**
(copertura e abilità ad area diventerebbero inefficaci), non migrarla.

Questo slice colma il divario, resta **puro e headless** e non tocca nulla di esistente.

## 2. Decisioni di design

| # | Decisione | Motivo |
|---|-----------|--------|
| D1 | `HexLine` in **aritmetica intera** (lerp razionale + arrotondamento cubico su resti interi) | La LOS decide l'esito del combattimento: è logica di turno, dove l'invariante #4 vieta il float. Il float resta confinato alle conversioni verso il mondo (`WorldToAxial`). Elimina anche l'oscillazione dell'arrotondamento sulle linee che passano esattamente sul confine tra due celle. |
| D2 | Tie-break dell'arrotondamento in **ordine fisso q → r → s** | Stessa regola di `CubeRound` già in `RTHexLibrary.cpp`: a parità di resto si aggiusta l'ultima componente. Deterministico e coerente col codice esistente. |
| D3 | **Regola di elevazione identica al quadrato**: un ostacolo blocca solo se sta sul layer del **tiratore** | `URTGridLibrary::HasLineOfSight` fa già così (si spara "sotto" un ponte e "oltre" le coperture basse da un piano superiore). Cambiare semantica fra le due topologie sarebbe una regressione mascherata da migrazione. |
| D4 | Una cella **assente** dall'asset non blocca la vista | Il vuoto è un buco nella mappa, non un muro. Coerente con `GraphNeighbors`, dove l'assenza impedisce il passaggio ma non è un ostacolo solido. |
| D5 | Cono **a 120°** = unione di due settori esagonali a 60° attorno alla direzione principale | Sull'esagono il settore a 60° è la primitiva naturale (`a·D1 + b·D2`, con `a,b ≥ 0` e distanza `a+b`); l'unione di due settori adiacenti dà un ventaglio simmetrico, tutto in aritmetica intera. Copertura: 3 celle a distanza 1, 5 a distanza 2 — confrontabile col cono a 45° del quadrato. |
| D6 | LOS in una libreria **dedicata** `URTHexVisionLibrary`, geometria pura in `URTHexLibrary` | La visione ha bisogno dell'asset mappa (`bBlocksLineOfSight`), la geometria no. Stessa separazione già in uso fra `URTHexLibrary` (matematica) e `URTHexPathLibrary` (grafo con mappa). |

## 3. API

### `URTHexLibrary` (geometria pura, nessuna mappa)

- `static TArray<FRTCellId> HexLine(const FRTCellId& A, const FRTCellId& B)` — celle attraversate da A a B,
  **estremi inclusi**, sul layer di `A` (linea planare, come `HexDistance` che ignora il layer). `A == B` → una
  sola cella. Celle consecutive sempre adiacenti; lunghezza = `HexDistance(A,B) + 1`.
- `static TArray<FRTCellId> HexCone(const FRTCellId& From, const FRTCellId& Target, int32 Range)` — ventaglio di
  120° da `From` verso `Target`, profondo `Range`; `From` **escluso**; ordinato con `StableLess` (deterministico).
  `Target == From` o `Range <= 0` → vuoto.

### `URTHexVisionLibrary` (visione sulla mappa)

- `static bool HasLineOfSight(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To)` — falso se
  una cella **intermedia** esiste sul layer di `From` e ha `bBlocksLineOfSight`. Estremi mai bloccanti (tiratore e
  bersaglio non si coprono da soli); mappa nulla → vero (nessun ostacolo noto); `From == To` → vero.

## 4. Test — prefissi `RefactorTactics.Hex.*` / `RefactorTactics.HexVision.*`

| Test | Comportamento |
|---|---|
| `Hex.HexLineStraight` | linea lungo una direzione: lunghezza `dist+1`, estremi corretti |
| `Hex.HexLineAdjacency` | ogni cella consecutiva è adiacente alla precedente (anche su linee oblique) |
| `Hex.HexLineDegenerate` | `A == B` → una sola cella; linea di lunghezza 1 fra vicini |
| `Hex.HexLineSymmetricLength` | `Line(A,B)` e `Line(B,A)` hanno la stessa lunghezza e gli estremi scambiati |
| `Hex.HexConeCoverage` | Range 1 → 3 celle; Range 2 → 8 celle; nessuna oltre il range; `From` escluso |
| `Hex.HexConeDegenerate` | `Target == From` o `Range <= 0` → vuoto |
| `HexVision.WallBlocks` | un muro (`bBlocksLineOfSight`) fra le due celle blocca; senza muro passa |
| `HexVision.EndpointsNeverBlock` | muro **sul** tiratore o **sul** bersaglio → LOS libera; adiacenti sempre visibili |
| `HexVision.EmptyCellDoesNotBlock` | cella assente dall'asset lungo la linea → non blocca |
| `HexVision.ElevationRule` | muro su un layer diverso da quello del tiratore → non blocca (regola D3) |

## 5. Definition of Done (raggiunta)

☑ TDD RED→GREEN misurato (RED: 9 test falliti con gli stub; `HexConeDegenerate` passava già perché lo stub
restituiva un array vuoto — è **caratterizzazione**, non un RED reale, e viene dichiarato come tale) ·
☑ build Editor `Succeeded` · ☑ suite Automation **156/156** misurata (146 + 10) · ☑ nessuna modifica a `Grid/`,
`RTMovementResolver`, `ARTTurnManager` · ☑ nessun float nella geometria di gioco (invariante #4: `HexLine` usa
`int64`, il float resta confinato a `WorldToAxial`) · ☑ nessuna verifica PIE necessaria · ☑ spec, roadmap H6 e
architettura aggiornate.

**Nota sull'implementazione di `HexLine`**: la formulazione classica applica un epsilon in virgola mobile agli
estremi per rompere i pareggi dell'arrotondamento. Qui l'equivalente è un **bias intero** `(+1, +1, −2)` su
numeratori scalati di 1024 (somma nulla → l'invariante `q+r+s=0` regge): stessa funzione, senza float e con la
garanzia che una linea sul confine fra due celle scelga sempre lo stesso lato.

## 6. Fuori scope (YAGNI)

Copertura **direzionale** (mezza copertura, fianchi) · visione a 360° con raggio (`VisibleCells`) · LOS fra layer
diversi che attraversa gli archi · occlusione parziale/probabilistica · cono ad ampiezza configurabile.

## 7. Cosa sblocca

Con LOS e forme disponibili, lo switch di `ARTTurnManager` su hex diventa una migrazione **senza perdita di
funzionalità**: restano da portare il bot (`URTBotLibrary`, che usa distanze e path quadrati) e il wiring di
unità/controller/HUD.
