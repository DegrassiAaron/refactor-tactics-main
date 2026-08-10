# Spec — Il bot esagonale (`URTHexBotLibrary`)

> `CURRENT` · **Data**: 2026-08-10 · **Owner del concetto «bot»** · **Issue**: [#202](https://github.com/DegrassiAaron/refactor-tactics-main/issues/202)
>
> **Questa è la spec attiva**: descrive il bot **com'è oggi** ed è il documento in cui il bot si cambia.
> Non va confusa con [`../technical/h6-5-hex-bot-spec.md`](../technical/h6-5-hex-bot-spec.md), che è
> `AS-BUILT` al checkpoint H6.5 (2026-08-05) e descrive uno stato superato: da allora il bot ha acquistato
> aree, fuoco amico, penalità sul collaterale agli alleati e la cella di fuga. Quel documento resta com'è —
> un `AS-BUILT` superato non si riscrive, si supera.
>
> **Non serve una nuova AI.** Serve la documentazione di quella che esiste.

## 1. Cos'è, e cosa non è

`URTHexBotLibrary` è **logica pura**: nessun `Actor`, nessun `UWorld`, solo interi (invariante **#4**). Riceve
uno snapshot e un contesto, restituisce un piano. Non conosce il turno, non scrive nel TurnLog, non decide
quale abilità usare.

La **scelta dell'abilità** fra quelle disponibili vive in `ARTTurnManager::PlanBots`, non qui: il TurnManager
costruisce un contesto per ciascuna abilità candidata e mette tutte le candidate risultanti in **una sola
lista**, che `ChooseBestPlan` confronta. È il motivo per cui `FRTHexBotPlan` porta la **forma** dell'attacco
(`Shape`, `AreaRadius`, `RangeCells`, `bFriendlyFire`) invece di leggerla dal contesto: un solo contesto
descriverebbe la forma di una sola abilità.

## 2. La politica, in una frase

> Colpisci chi puoi uccidere, da una cella che non ti espone, restando alla distanza giusta per il tuo ruolo
> e preferendo la quota — e a parità di tutto, non muoverti.

## 3. Utility scoring — `ScorePlan`

Punteggio intero, somma di quattro contributi. La geometria dell'attacco viene da
`URTHexCombatLibrary::HexHitCells`, **la stessa che usa il resolver**: il bot non stima una forma propria,
legge quella vera.

### 3a. Focus-fire — sulle celle investite, non sul bersaglio

Per **ogni nemico** dentro `HexHitCells`:

```text
Score += WDamage × AttackDamage
Score += WKill              se AttackDamage >= (HP + scudo) del nemico
```

Il bersaglio mirato porta gli HP dichiarati dal piano; i nemici presi *in più* da un'area li leggono dal
contesto. Un'area che prende due nemici vale il doppio di una che ne prende uno, e questo cade fuori
automaticamente dal conto per cella — non è una regola a parte.

### 3b. Collaterale sugli alleati — penalità proporzionale, non veto

Solo se l'attacco dichiara `bFriendlyFire`. Per ogni alleato dentro `HexHitCells`:

```text
Score -= WAllyDamage × AttackDamage
Score -= WKill                        se il colpo lo uccide
```

`WAllyDamage` vale `WDamage` per default: **un punto di danno al compagno annulla esattamente un punto di
danno al nemico**. Quindi prendere due nemici e un alleato resta conveniente, prenderne uno solo non lo è. È
un peso, non un divieto — si tara senza toccare la logica (decisione del 2026-08-09).

Il bot che pianifica **non conta se stesso** fra gli alleati: `CollectHexAttacks` salta sempre l'attaccante, e
contarsi renderebbe il bot timido su un danno che non subirebbe mai.

### 3c. Minaccia sulla cella di destinazione

Per ogni nemico che ha la cella **entro la propria gittata** *e* **linea di vista** su di essa:

```text
Score -= WThreat
```

Le due condizioni insieme sono ciò che fa valere la copertura: un nemico che potrebbe raggiungerti ma ha un
muro davanti non minaccia.

### 3d. Posizionamento e quota

Sulla distanza dal nemico **più vicino**:

```text
kiter (KiteStandoff > 0), se dist < standoff:   Score -= WKiteViolation × (standoff − dist)
mischia (KiteStandoff == 0):                     Score -= WApproach × dist
```

E infine `Score += WElevation × Layer` della cella: a parità di tutto, l'alta quota vince.

### 3e. I pesi

| Peso | Default | Cosa governa |
|---|---:|---|
| `WKill` | 10000 | il kill domina: nessuna somma di altri termini lo raggiunge |
| `WDamage` | 10 | danno inflitto per punto |
| `WAllyDamage` | 10 | danno collaterale al compagno, per punto |
| `WThreat` | 100 | esposizione al tiro nemico, per nemico |
| `WKiteViolation` | 50 | per cella sotto lo standoff del kiter |
| `WApproach` | 10 | per cella di distanza, per chi è in mischia |
| `WElevation` | 20 | per layer di quota |

Sono **interi bilanciabili senza toccare la logica**. La scala relativa fra `WThreat` e `WDamage` è nota
essere un punto dolente: vedi [#149](https://github.com/DegrassiAaron/refactor-tactics-main/issues/149), che
misura come nessuna costante di premio al posizionamento riesca a stare insieme fra «battere due minacce»
(> 200) e «non battere un attacco vero» (< 200).

## 4. Pool di candidate — `BuildCandidates`

Le mosse candidate nascono da `URTHexSimLibrary::ReachableCells`, che ha **già** applicato budget di
movimento, celle bloccate, unità occupanti e archi verticali. Conseguenza: **il bot non rifà pathfinding e non
può proporre mosse illegali**.

Per ogni cella raggiungibile:

| Candidata | Quando nasce |
|---|---|
| **riposizionamento** (nessun attacco) | sempre, una per cella |
| **attacco dalla cella** | una per ciascun nemico entro `AttackRange` **e** in linea di vista *da quella cella* |

A queste il `ARTTurnManager` aggiunge, fuori dalla libreria:

| Candidata | Dove |
|---|---|
| **scatto + attacco** e **scatto di riposizionamento** | `PlanBots` costruisce un `DashSnapshot` e ne genera le candidate con lo stesso `BuildCandidates` |
| **fuga del kiter** | `BestKiteCell`, chiamata direttamente quando il kiter è minacciato |

L'ordine di generazione è deterministico.

## 5. Tie-break — `ChooseBestPlan`

Punteggio massimo; a parità, in quest'ordine:

1. **mossa minima** da `Context.Origin` — *restare vince*;
2. **`StableLess`** sulla cella (Layer, X, Y).

L'ordine è **totale**, quindi permutare le candidate non cambia l'esito. Nessuna candidata ⇒ resta a `Origin`.

`BestKiteCell` ha un tie-break proprio, con la stessa proprietà: distanza massima dalla minaccia, poi percorso
più economico, poi `StableLess`. È una scelta di posizionamento pura, non un'utility, e resta separata da
`ScorePlan` — come nel bot quadrato che ha sostituito.

## 6. Cosa il bot sa — e cosa **non** sa ancora

Questa sezione è la più importante, perché è quella dove la spec e il DoD della issue divergono da com'è oggi.

| Requisito | Stato reale, misurato il 2026-08-10 |
|---|---|
| **Nessun accesso agli intenti nemici nascosti** | ✅ **vero**. `FRTHexBotContext` non contiene intenti: solo posizioni, gittate e HP. Il bot non può leggere il piano avversario perché il tipo non lo trasporta |
| **Pianifica sulla Team Knowledge della propria squadra** | ❌ **non ancora**. `ARTTurnManager::PlanBots` popola `Ctx.Enemies` da **tutte** le unità nemiche vive (`TeamId != Bot->TeamId && IsAlive()`), senza filtro di percezione. Oggi il bot **vede tutte le posizioni nemiche** |
| **Tiene conto del facing e dell'arco frontale** | ❌ **non ancora**. `ScorePlan` non legge il facing: né il proprio, né quello dei nemici. La minaccia è calcolata su gittata + LOS, senza cono |
| **Ha una politica di reazione esplicita** | ❌ **non ancora**. Il bot non arma reazioni e non dichiara un regime |
| **Validato sotto stress 4v4** | ❌ **non ancora**. La suite lo esercita a 2v2 |

**Perché va scritto così.** Un documento che descrivesse il bot come già conforme a E13/E16 renderebbe
invisibile il lavoro che manca, e i test verdi di oggi sembrerebbero provare qualcosa che non provano.

## 7. Cosa cambierà, e quale sezione tocca

La premessa del bot cambia **tre volte**. Questa spec è scritta sapendo quali sue parti cadranno.

| Epic | Cosa cambia | Sezioni di questa spec |
|---|---|---|
| **E13** — conoscenza parziale | `Ctx.Enemies` smette di essere «tutti i nemici vivi» e diventa la Team Knowledge: posizioni note, contatti incerti, ultimo contatto. Il bot dovrà decidere **anche** cosa fare di un contatto `Incerto` | §6 (riga Team Knowledge) e §3c: la minaccia si calcola su ciò che si **sa**, non su ciò che c'è |
| **E16** — facing e arco frontale | entra il cono: `ScorePlan` dovrà pesare da dove si è **visti** e da che lato si è **scoperti** (ADR-0005 §4a: fuori dall'arco frontale cadono −10 di copertura e −15 di `Guard`). Con [ADR-0008](../decisions/adr-0008-rotazione-e-policy-di-facing.md) si aggiunge il **budget di pivot**: il bot deve scegliere celle da cui può assumere un orientamento utile, altrimenti pianifica facing che non può ottenere | §3c (minaccia), §4 (le candidate acquistano una dimensione: cella **più** facing) |
| **E14** — reazioni | il bot dovrà armare reazioni e rispondere alle finestre `AllowedResponses ≥ 2` | §6 (riga reaction policy) — oggi la spec non ha una §dedicata perché non c'è nulla da descrivere |
| **E17** — stress 4v4 | il numero di candidate cresce col quadrato delle unità; il tie-break e il determinismo vanno riverificati a scala maggiore | §4, §5 |

## 8. Evidenza — i test che esistono oggi

**25 test**, in due file — 18 + 7, contati sulle macro `IMPLEMENT_*_AUTOMATION_TEST`. Sono l'unica prova di
ciò che questa spec afferma.

> ⚠️ *Rettifica del 2026-08-10 (review post-merge)*: la prima stesura diceva «26 test … 18 + 8». I nomi
> elencati nelle due tabelle erano già quelli giusti; sbagliati erano i **totali**, dedotti da un `sort -u`
> invece che contati. In un documento la cui unica funzione è essere l'evidenza, un totale non misurato è il
> difetto peggiore possibile.

### `Tests/RTHexBotTests.cpp` — `RefactorTactics.HexBot.*` (18, logica pura)

| Test | Cosa dimostra |
|---|---|
| `ScoreFocusFire` | un colpo letale batte uno che non uccide; più danno batte meno danno |
| `ScoreThreatRespectsCover` | un nemico in gittata **con** LOS abbassa il punteggio; con un muro in mezzo no |
| `ScoreKiterVsMelee` | kiter penalizzato sotto lo standoff, mischia penalizzata dalla distanza |
| `ScoreElevationBonus` | a parità di tutto vince la quota |
| `ScoreAreaCountsExtraEnemies` | l'area che prende due nemici vale più di quella che ne prende uno |
| `ScoreAreaPenalizesAlly` | il collaterale sul compagno sottrae |
| `ScoreAllyPenaltyScalesWithDamage` | la penalità è **proporzionale**, non un veto |
| `ScoreIgnoresAllyWithoutFriendlyFire` | senza fuoco amico l'alleato nell'area non subisce nulla |
| `ScoreSingleShapeIgnoresNeighbours` | con `Single` il conto resta una cella, un bersaglio |
| `CandidatesCarryShape` | la forma viaggia sul piano, non sul contesto |
| `ChooseBestPlanOrderIndependent` | permutare le candidate non cambia l'esito |
| `ChooseBestPlanTieBreak` | a parità vince la mossa minima, poi `StableLess` |
| `ChooseBestPlanEmptyStays` | nessuna candidata ⇒ resta a `Origin` |
| `PlanUnitTakesKillingShot` | si sposta nella cella da cui uccide |
| `PlanUnitRespectsBudgetAndOccupancy` | non propone celle fuori budget né occupate |
| `PlanUnitSeeksCover` | fra due celle equivalenti sceglie quella non esposta |
| `KiteCellMaximizesDistance` | la fuga massimizza la distanza dalla minaccia |
| `KiteCellStaysLegal` | la fuga non esce dalle celle raggiungibili |

### `Tests/RTHexBotIntegrationTests.cpp` — `RefactorTactics.HexBotPlay.*` (7, su partita)

| Test | Cosa dimostra |
|---|---|
| `PlansOnlyLegalMoves` | in partita vera il bot non produce mosse illegali |
| `DashPlanIsExecutableOnCostlyTerrain` | lo scatto pianificato è eseguibile dove il terreno costa di più |
| `DashRespectsThreat` | lo scatto non ignora l'esposizione |
| `KiterFleesWhenThreatened` | il kiter rinuncia al tiro per non farsi raggiungere |
| `UsesSupportWhenHurt` | il supporto entra quando serve |
| `PlanDoesNotBlastDyingAlly` | il collaterale non uccide il compagno |
| `WThreatTuning` | la scala di `WThreat` è esercitata, non assunta |

> **Cosa i test non coprono**, e va detto: nessuno di essi esercita conoscenza parziale, facing o reazioni —
> perché nessuna delle tre esiste ancora nel bot (§6). I verdi di `HexBotPlay.*` provano che il bot gioca
> **legalmente**, non che gioca **bene**: il bilanciamento è misurato altrove ed è aperto ([#149](https://github.com/DegrassiAaron/refactor-tactics-main/issues/149)).

## 9. Fuori scope

Coordinamento di squadra · previsione dei piani avversari · panic/supporto legati agli `Actor` (erano del bot
quadrato, rimosso al CP 7.2) · difficoltà selezionabile · qualunque ramo `if (Hero == …)` nel punteggio: il
bot legge il **catalogo**, non i nomi.

## 10. Riferimenti

- Codice: `Source/RefactorTactics/Bot/RTHexBotLibrary.{h,cpp}` · consumatore: `ARTTurnManager::PlanBots`
- Storia: [`../technical/h6-5-hex-bot-spec.md`](../technical/h6-5-hex-bot-spec.md) (`AS-BUILT` H6.5)
- Architettura: [`../technical/architettura-codice.md`](../technical/architettura-codice.md)
- Wiki, lato giocatore: [`../wiki/game/avversario-bot.md`](../wiki/game/avversario-bot.md)
- Decisioni che lo vincoleranno: [ADR-0005](../decisions/adr-0005-orientamento.md) · [ADR-0008](../decisions/adr-0008-rotazione-e-policy-di-facing.md) · [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md)
