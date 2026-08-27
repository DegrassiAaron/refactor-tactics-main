# Spec — Il bot esagonale (`URTHexBotLibrary`)

> `CURRENT` · **Data**: 2026-08-10 · **Owner del concetto «bot»** · **Issue**: [#202](https://github.com/DegrassiAaron/refactor-tactics-main/issues/202)
>
> **Questa è la spec attiva**: descrive il bot **com'è oggi** ed è il documento in cui il bot si cambia.
> Non va confusa con [`../technical/systems/h6-5-hex-bot-spec.md`](../technical/systems/h6-5-hex-bot-spec.md), che è
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

Punteggio intero, somma di **cinque** contributi — il quinto è il termine di ingaggio, entrato con
[`D-185`](../decisions/RT_PDR_00_Decision_Log.md) e assente da questa lista fino al 2026-08-24. La geometria dell'attacco viene da
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
kiter (KiteStandoff > 0), se passi < standoff:   Score -= WKiteViolation × (standoff − passi)
kiter,  se passi ≥ standoff:                     Score -= WApproach × (passi − standoff)
mischia (KiteStandoff == 0):                     Score -= WApproach × passi
```

🔴 **`passi` è la distanza sul GRAFO, non in linea d'aria** (dal 2026-08-23,
[#1296](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1296)). Una BFS a peso uniforme
sull'adiacenza inversa di `GraphNeighbors`: una cella dietro un muro è lontana quanto costa aggirarlo, e non
due passi. Sono **passi e non costo** — il costo sommerebbe il `MoveCost` del terreno e ritarerebbe
`WApproach` su ogni mappa con fango o ghiaccio, che è bilanciamento e ha la sua sede in
[#149](https://github.com/DegrassiAaron/refactor-tactics-main/issues/149). Su campo aperto senza ostacoli il
numero coincide **esattamente** con `HexDistance`.

⚠️ **`MinDist` non è più limitato dal raggio della mappa**, e la conseguenza va detta: prima
`WApproach × dist` non poteva superare `WThreat`, quindi stare sotto tiro pesava sempre più di una cella di
avvicinamento; con i passi un aggiramento lungo può superarlo. **Nessun test lo pinna**, ed è una domanda di
bilanciamento aperta.

⚠️ **Il `margine` non c'è più nella seconda riga**, e non è una svista di questa revisione: la penalità parte
dallo **standoff** dal fix di [#1287](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1287),
che lo dichiara nel codice — far partire la penalità dalla portata lascia piatta la banda
`[standoff, portata]`, e lì l'elevazione torna a essere l'unico termine posizionale. Questa riga era rimasta
indietro. ⚠️ `KiterStandoffMargin` in `RTHexBotLibrary.h` dichiara ancora che «`ScorePlan` lo RIAGGIUNGE»:
**è falso**, la costante serve solo a `DeriveKiteStandoff`.

E infine `Score += WElevation × Layer` della cella di destinazione: a parità di tutto, l'alta quota vince.

⚠️ **Qui si è formato lo stato assorbente di [#1088](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1088)**,
e la difesa è **un numero, non la formula**. Il termine compete con l'avvicinamento: finché `WElevation × Layer`
supera quello che `WApproach` rende scendendo, restare in alto batte muoversi. Misurato con `WElevation` 20:
un'unità saliva sulla piattaforma al turno 3 e non scendeva fino al 12.

⛔ **Renderlo relativo all'origine non serve, ed è stato provato**: `Origin` è fisso per l'intera scelta,
quindi sottrarne il contributo sposta ogni candidata della stessa costante e non cambia l'ordinamento.

⚠️ **La banda `[standoff, portata]` è piatta**: dentro la propria portata utile il kiter è indifferente
alla distanza, perché spara comunque e restare lontano è il suo vantaggio
([#1088](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1088)). Il ramo oltre la portata
non esisteva, e lì l'elevazione restava l'unico termine posizionale: il kiter si parcheggiava in quota
con qualunque `WElevation > 0`, e l'invariante qui sotto non lo copriva perché `WApproach` non era in gioco.

⚠️ **INVARIANTE: `WElevation × MaxLayer < WApproach`.** È l'unica difesa reale, pinnata da
`HexBot.ElevationNeverOutweighsClosingOneCell`. Alzare `WElevation` in editor oltre quella soglia — che
`PIE-BU2b` documenta come workflow — riapre lo stato assorbente, e nessun gate lo impedisce.

### 3e. «Da qui posso ingaggiare» — lo specchio offensivo della minaccia

Sui piani **senza attacco**, per una cella da cui si vede almeno un contatto noto:

```text
Score += max(0, WEngage − WEngageDecay × IdleTurns)
```

`IdleTurns` è da quanti turni consecutivi il piano scelto per quell'unità **non contiene un attacco** — la
memoria per unità che [`E26`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/326) porterà per
intero, e di cui qui entra il minimo indispensabile. Conta l'**intento**, non l'esito: si azzera quando il bot
*pianifica* un colpo, non quando il colpo va a segno.

🔴 **Il decadimento non è una rifinitura: è il termine.** Un bonus posizionale fisso sulla linea di tiro paga
per *guardare*, e la cella che massimizza il guardare è una **vedetta da cui non si spara** — lo stato
assorbente di [#1088](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1088) sotto un altro nome.
Misurato intero per intero il 2026-08-24 ([#1300](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1300)):
`Match.Autobattle.EngagesOnTheGeneratedTestArena` cade **da `W = 7`**, `NobodyParksOnTheAuthoredMap` si
sblocca **da `W = 11`**, e fra 7 e 10 sono rossi **entrambi**. La finestra del termine senza memoria è vuota.

⚠️ **Guarda dove VAI, non da dove parti**, ed è ciò che lo separa dal filtro sul dominio di
[#1287](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1287): quello si accendeva quando eri
cieco e si spegneva appena vedevi — un ciclo di periodo due, misurato in otto alternanze su dodici turni.

⚠️ **Vale una volta sola per candidata**, non una per nemico visto: è «posso ingaggiare», non «quanti ne
vedo». Contarli renderebbe il termine una seconda misura del focus-fire, che ha già il suo peso.

⚠️ **La linea si chiede nel verso offensivo** (`destinazione → nemico`), lo stesso di `BuildCandidates`, e la
differenza è misurabile: `HexLine` costruisce la linea sul layer del **tiratore**, quindi fra piani diversi i
due versi non coincidono — su `DA_HexMap_Arena` sono **91 coppie asimmetriche su 2016**, tutte fra layer
diversi, e dalle tre celle della piattaforma L1 si vedono **64 celle su 64**.

⚠️ **La coppia `WEngage`/`WEngageDecay` si tara sull'ESITO, e non è il loro rapporto a decidere**: misurati
`15/5` ✅ e `20/10` ✅ contro `20/5` 🔴 e `30/10` 🔴 — e `30/10` e `15/5` si azzerano entrambi dopo tre turni.
Pinnata da `HexBot.EngageBonusFadesWithIdleTurns`, che misura la scelta di `ChooseBestPlan` fresco contro
inerte; la taratura fine resta bilanciamento, sede [#149](https://github.com/DegrassiAaron/refactor-tactics-main/issues/149)
e [D-102](../decisions/RT_PDR_00_Decision_Log.md).

### 3f. I pesi

| Peso | Default | Cosa governa |
|---|---:|---|
| `WKill` | 10000 | il kill domina: nessuna somma di altri termini lo raggiunge |
| `WDamage` | 10 | danno inflitto per punto |
| `WAllyDamage` | 10 | danno collaterale al compagno, per punto |
| `WThreat` | 100 | esposizione al tiro nemico, per nemico |
| `WKiteViolation` | 50 | per cella sotto lo standoff del kiter |
| `WApproach` | 10 | per cella di distanza: dal nemico per la mischia, dalla propria portata per il kiter |
| `WElevation` | 4 | per layer di quota — vincolato da `WElevation × MaxLayer < WApproach` (#1088) |
| `WEngage` | 15 | bonus per una cella da cui si vede un contatto noto, sui piani senza attacco (`D-185`) |
| `WEngageDecay` | 5 | quanto quel bonus cala per ogni turno consecutivo senza ingaggiare — **zero lo riporta alla forma che non passa gli oracoli** |

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

### 4a. Nessun contatto — la condotta di ricerca

Con la conoscenza parziale nasce un caso che prima non poteva esistere: **la squadra non sa dove sia nessuno**.
È la condizione normale del primo turno, perché una mappa più larga della vista non mostra lo schieramento
avversario.

Con `Ctx.Enemies` vuoto lo scoring perde minaccia e avvicinamento, ogni cella vale uguale, e il bot resta
fermo. Non è «perde il contatto e sbaglia» — che il DoD di #160 ammette: è un bot che **smette di giocare**,
e due squadre cieche si aspettano finché la partita non scade. L'ha misurato `HexMatch.PlaysToCompletion`
diventando rosso.

La condotta è la più povera che ristabilisce il contatto: **avvicinarsi al centro della mappa**, che è
geometria pubblica — zero informazione nascosta. Deterministica (distanza minima dal baricentro, poi
`StableLess`) e onesta sui propri limiti: non è una ricerca, è un movimento che fa incontrare le squadre.
I goal veri — `SecureObjective`, `GatherInformation` — sono di [E26](spec-bot-tattico.md) §4, e questo ramo
è il posto in cui atterreranno.

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
| **Pianifica sulla Team Knowledge della propria squadra** | ✅ **vero dal 2026-08-11** (CP 13.5, [#160](https://github.com/DegrassiAaron/refactor-tactics-main/issues/160)). `PlanBots` costruisce `Ctx.Enemies` da `FRTTeamKnowledge` con la **stessa** regola del targeting umano (`ClassifyTarget`): visto → cella e condizione attuali; ricordato → cella dell'ultimo contatto e HP **massimi**, perché la squadra conosce l'identità e non la condizione; ignoto → non esiste. Il gate è `HexBotPlay.HiddenEnemyFairness`, e cade se l'onniscienza rientra |
| **Tiene conto del facing e dell'arco frontale** | ❌ **non ancora**. `ScorePlan` non legge il facing: né il proprio, né quello dei nemici. La minaccia è calcolata su gittata + LOS, senza cono |
| **Ha una politica di reazione esplicita** | ❌ **non ancora**. Il bot non arma reazioni e non dichiara un regime |
| **Validato sotto stress 4v4** | ❌ **non ancora**. La suite lo esercita a 2v2 |

**Perché va scritto così.** Un documento che descrivesse il bot come già conforme a E13/E16 renderebbe
invisibile il lavoro che manca, e i test verdi di oggi sembrerebbero provare qualcosa che non provano.

## 7. Cosa cambierà, e quale sezione tocca

La premessa del bot cambia **tre volte**. Questa spec è scritta sapendo quali sue parti cadranno.

| Epic | Cosa cambia | Sezioni di questa spec |
|---|---|---|
| ~~**E13**~~ — conoscenza parziale ✅ **fatta** | `Ctx.Enemies` smette di essere «tutti i nemici vivi» e diventa la Team Knowledge: posizioni note, contatti incerti, ultimo contatto. Il bot dovrà decidere **anche** cosa fare di un contatto `Incerto` | §6 (riga Team Knowledge) e §3c: la minaccia si calcola su ciò che si **sa**, non su ciò che c'è |
| **E16** — facing e arco frontale | entra il cono: `ScorePlan` dovrà pesare da dove si è **visti** e da che lato si è **scoperti** (ADR-0005 §4a: fuori dall'arco frontale cadono −10 di copertura e −15 di `Guard`). Con [ADR-0008](../decisions/adr-0008-rotazione-e-policy-di-facing.md) si aggiunge il **budget di pivot**: il bot deve scegliere celle da cui può assumere un orientamento utile, altrimenti pianifica facing che non può ottenere | §3c (minaccia), §4 (le candidate acquistano una dimensione: cella **più** facing) |
| **E14** — reazioni | il bot dovrà armare reazioni e rispondere alle finestre `AllowedResponses ≥ 2` | §6 (riga reaction policy) — oggi la spec non ha una §dedicata perché non c'è nulla da descrivere |
| **E17** — stress 4v4 | il numero di candidate cresce col quadrato delle unità; il tie-break e il determinismo vanno riverificati a scala maggiore | §4, §5 |

> **Dal 2026-08-11 le quattro caselle hanno un owner.** Prima erano quattro promesse senza un documento che
> le possedesse — il registry stesso lo dichiarava, «citato in `roadmap-post-v0.1.md` ma **senza owner
> documentale proprio**». Ora la forma del bot che verrà sta in
> [`spec-bot-tattico.md`](spec-bot-tattico.md), che fissa i confini (`D-095`…`D-099`) senza descrivere
> codice che esiste. Questa spec **non cambia**: continua a descrivere il bot di oggi, ed è la sua §6 —
> quella che dice cosa il bot *non* sa — a restare la misura di quanta distanza ci sia fra i due documenti.

## 8. Evidenza — i test che esistono oggi

Due file, e il totale **si misura sul branch** invece di stare scritto qui:

```
grep -c 'RefactorTactics.HexBot\.'     Source/RefactorTactics/Tests/RTHexBotTests.cpp
grep -c 'RefactorTactics.HexBotPlay\.' Source/RefactorTactics/Tests/RTHexBotIntegrationTests.cpp
```

Sono l'unica prova di ciò che questa spec afferma.

> ⚠️ *Rettifica del 2026-08-27*: qui c'era scritto «29 test — 18 + 11», e il primo file ne conteneva già
> **24**. Il numero era stato misurato una volta e da allora invecchiava a ogni test aggiunto — che è
> precisamente il difetto che la rettifica del 2026-08-10 qui sotto dichiarava di aver corretto, tornato
> nella stessa forma perché la correzione aveva sostituito un totale sbagliato con uno giusto invece di
> togliere il totale.

> ⚠️ *Rettifica del 2026-08-10 (review post-merge)*: la prima stesura diceva «26 test … 18 + 8». I nomi
> elencati nelle due tabelle erano già quelli giusti; sbagliati erano i **totali**, dedotti da un `sort -u`
> invece che contati. In un documento la cui unica funzione è essere l'evidenza, un totale non misurato è il
> difetto peggiore possibile.

### `Tests/RTHexBotTests.cpp` — `RefactorTactics.HexBot.*` (logica pura)

> ⚠️ *Il totale non si scrive più qui.* Era «18» e il file ne conteneva già 23 prima di questa riga: un
> numero fissato a mano invecchia al primo test aggiunto, e in un documento che esiste per essere
> l'evidenza è il difetto peggiore. Si conta sul branch:
> `grep -c 'RefactorTactics.HexBot\.' Source/RefactorTactics/Tests/RTHexBotTests.cpp`.

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
| `PathFieldCacheKeepsLiveMapsApart` | due arene vive con la **stessa** `Revision` non si scambiano il campo di distanze ([D-196] l'ha resa non discriminante, `#1436`) |
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
| `HiddenEnemyFairness` | **il canary**: due partite identiche in ciò che la squadra sa e diverse in ciò che non sa producono lo **stesso** piano |
| `PlansOnPartialKnowledge` | il bot non bersaglia un ignoto **nemmeno se è a portata e quasi morto** |
| `SeeksContactWithoutKnowledge` | senza nessun contatto il bot **cerca** invece di fermarsi |
| `ActsOnLastKnownCell` | dopo **due** osservazioni il bot agisce sul **ricordo**, non sulla posizione vera — e' il solo test che raggiunge `CellOnly` |

> **Da dove viene lo standoff.** Il kiting è un comportamento del **bot**, non una caratteristica
> dell'eroe: un'unità che muove il giocatore non lo consulta mai. Per questo lo standoff non è un campo
> di `URTHeroData` né di `ARTUnit` — lo **deriva il bot** dalla portata dell'attacco base, con
> `URTHexBotLibrary::DeriveKiteStandoff`. La regola riproduce i due archetipi che il comportamento lo
> producevano (portata 6 → standoff 4, portata 3 → 0), e sul roster v0.1 rende kiter la sola **Phase**
> (`PressureJet`, portata 5 → standoff 3): Gadget e Wraith (4) e Riktor (3) chiudono la distanza.

> **Cosa i test non coprono**, e va detto: nessuno di essi esercita conoscenza parziale, facing o reazioni —
> perché nessuna delle tre esiste ancora nel bot (§6). I verdi di `HexBotPlay.*` provano che il bot gioca
> **legalmente**, non che gioca **bene**: il bilanciamento è misurato altrove ed è aperto ([#149](https://github.com/DegrassiAaron/refactor-tactics-main/issues/149)).

## 9. Fuori scope

Coordinamento di squadra · previsione dei piani avversari · panic/supporto legati agli `Actor` (erano del bot
quadrato, rimosso al CP 7.2) · difficoltà selezionabile · qualunque ramo `if (Hero == …)` nel punteggio: il
bot legge il **catalogo**, non i nomi.

## 10. Riferimenti

- Codice: `Source/RefactorTactics/Bot/RTHexBotLibrary.{h,cpp}` · consumatore: `ARTTurnManager::PlanBots`
- Il bot che verrà: [`spec-bot-tattico.md`](spec-bot-tattico.md) — Team Planner, belief, reazioni (E13/E26/E27/E28)
- Storia: [`../technical/systems/h6-5-hex-bot-spec.md`](../technical/systems/h6-5-hex-bot-spec.md) (`AS-BUILT` H6.5)
- Architettura: [`../technical/architecture/architettura-codice.md`](../technical/architecture/architettura-codice.md)
- Wiki, lato giocatore: [`avversario-bot` (Wiki)](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/avversario-bot)
- Decisioni che lo vincoleranno: [ADR-0005](../decisions/adr-0005-orientamento.md) · [ADR-0008](../decisions/adr-0008-rotazione-e-policy-di-facing.md) · [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md)
