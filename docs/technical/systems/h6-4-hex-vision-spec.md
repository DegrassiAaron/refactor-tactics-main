# Spec H6.4 — Linea di vista e forme di targeting esagonali

> ## 🧱 `AS-BUILT 2026-08-05` — spec di ciò che fu consegnato a H6.4
>
> **Non è il documento in cui si cambia la regola oggi.** Descrive la LOS com'era al checkpoint H6.4; da allora
> è stata **emendata** da E9 (copertura bassa e alta, bordi che bloccano) e lo sarà da E13/E16 (conoscenza
> parziale, facing e arco frontale). Le sezioni sotto non vanno aggiornate come se fossero design corrente.
>
> Owner corrente: [`architettura-codice.md`](../architecture/architettura-codice.md) ·
> [`../../gameplay/spec-copertura-cp91.md`](../../gameplay/spec-copertura-cp91.md) ·
> [`../../gameplay/spec-copertura-alta-cp92.md`](../../gameplay/spec-copertura-alta-cp92.md).

> **Stato**: **Implementata** · **Data**: 2026-08-05 · **Branch**: `feat/hex-vision`
> *(la suite «156/156» è il numero di allora; oggi si misura col comando in [`../../README.md`](../../README.md))*
> **Fonte**: milestone **H6** di [`hex-map-roadmap.md`](../../roadmap/hex-map-roadmap.md). Prerequisito dello switch di
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
| D7 | La LOS consulta anche la **geometria intra-cella** (`InteriorWalls`), tramite `URTHexOcclusionLibrary` — dal 2026-09-01, [`D-269`](../../decisions/RT_PDR_00_Decision_Log.md)/[`D-270`](../../decisions/RT_PDR_00_Decision_Log.md), [#1830](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1830) | Un muro che taglia l'esagono copriva **otticamente e non logicamente**. La stessa primitiva la consuma la linea d'attacco: risposte diverse renderebbero *«visibile un bersaglio che non si può colpire»*. ⚠️ **Non è la connettività di `ClassifyIntraCellTraversal`**: quella risponde *«ci si arriva girando attorno?»*, e la vista non gira attorno a niente. |
| D8 | La **corda d'attraversamento** è dichiarata: `EdgeMid(ingresso) → EdgeMid(uscita)`, col **centro** agli estremi della linea | In una LOS cella-a-cella non esiste altrimenti un «in mezzo»: `HexLine` produce celle, non una retta. Gli estremi sono **anchor** (`ERTAnchorKind`), quindi la corda si dice nel vocabolario che `D-288` ha chiuso. ⚠️ È un'**approssimazione dichiarata**: la retta euclidea fra due centri non passa per i punti medi quando la linea «gira», questa sì — la stessa classe di approssimazione che la LOS cella-a-cella già accetta. |
| D9 | L'occlusione intra-cella è **intera ed esatta**, senza virgola mobile | Ogni anchor vale `(a·r3, b)` in unità `R/4`; il prodotto vettoriale vale `r3·(A1·B2 − A2·B1)`, dove la radice è un fattore positivo comune che **si semplifica**. Nessun epsilon, nessuna dipendenza da `HexSize`, nessuna differenza fra piattaforme — e la LOS entra nell'hash di stato. |

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
  🔑 Il corpo **delega** a `DescribeLineOfSight` e ne butta via la ragione: la parità fra bool e ragione è
  **strutturale**, non asserita, perché non esiste una seconda LOS da tenere allineata.
- `static FRTLineOfSightResult DescribeLineOfSight(...)` — la stessa decisione con la **ragione** e il **punto**.
  `ERTLineOfSightBlock` vale `None` · `EdgeBlocker` (il bordo attraversato nega il passaggio, `BlocksTraversal`)
  · `CellBlocker` (`bBlocksLineOfSight`, estremi esclusi) · **`InteriorGeometry`** (un muro interno alto
  incrociato dalla corda — estremi **inclusi**, perché un muro nella cella del tiratore sta *fra* lui e l'uscita).

### `URTHexOcclusionLibrary` (geometria intra-cella, `D-269`/`D-270`)

- `static bool BlocksSight(const URTHexMapAsset* Map, const FRTCellId& Prev, const FRTCellId& Cell, const FRTCellId& Next)`
  — pura, headless, **intera**. `Prev == Cell` = la linea nasce qui, `Next == Cell` = finisce qui (in entrambi i
  casi quel capo della corda è il **centro**). Blocca solo un muro `High` (`D-271`) sul layer della cella, e solo
  su un incrocio **proprio**: tangenza e collinearità non bloccano (*fail-open*, come la cella assente).
- 🔑 **La consumano in due**: `URTHexVisionLibrary::DescribeLineOfSight` e
  `URTOffensiveActionLibrary::LineCells`. Una funzione sola, non due implementazioni con un test di parità
  sopra — è ciò che `D-269` chiede quando dice *«una sola primitiva deterministica di occlusione»*.

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
| `Occlusion.CrossingWallBlocksSight` | un muro interno alto ferma la vista, e la ragione nomina la cella |
| `Occlusion.WallInShooterCellBlocks` | un muro nella cella del tiratore lo chiude dentro: `StepIndex` può valere `0` |
| `Occlusion.SightAndProjectileAgree` | vista e linea d'attacco non divergono, e il log dice `BlockedByInteriorGeometry` |
| `Occlusion.LowWallDoesNotOcclude` · `Occlusion.WallOnOtherLayerDoesNotOcclude` | `D-271` e regola D3 applicate al muro interno |
| `Occlusion.TangentAndCollinearDoNotBlock` | sfiorare un muro non è attraversarlo; guardarci lungo nemmeno |
| `Occlusion.SelectedCoverDoesNotDisarmGeometry` | la geometria **non scelta** continua a valere |
| `Occlusion.CellWithoutInteriorGeometryIsUnchanged` | nessuna regressione sulla LOS cella-a-cella |
| `Occlusion.BoundaryTableMatchesTheFloatOracle` | la tabella intera dice lo stesso di `SectorBoundaryPoints`, su due `HexSize` |
| `Occlusion.EdgeMidMatchesTheWorldOracle` | la corrispondenza direzione↔lato è **chiesta**, non trascritta (l'errore di `#1920`) |
| `HexMap.InteriorWallEntersHash` | spostare o abbassare un muro cambia `ComputeHash`; rinominarlo no |

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
