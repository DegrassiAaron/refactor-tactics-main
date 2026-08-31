# Spec — Posa nella cella, copertura selezionabile e geometria intra-hex

> **Owner documentale** del modello di posa (*placement*), delle sorgenti/opzioni/facce di copertura e della
> traversata dentro una cella. `CURRENT` · normativo.
> Decisione abilitante: **`D-289`** in [`RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md).
> Epic **E23** ([#324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324)), checkpoint
> `E23.6` ([#1827](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1827)) e
> `E23.7` ([#1828](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1828)).
>
> **Non possiede** la grammatica dei segmenti né la misura dell'occupancy: quelle restano di
> [`spec-hex-geometry-authoring.md`](spec-hex-geometry-authoring.md), che questo documento **corregge** in
> due punti (§5, §6) e per il resto consuma.
> **Non possiede** i numeri: percentuali di mitigazione e raggi di clearance restano decisioni aperte in
> [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) § `COV-*`
> 🔁 **Aggiornato il 2026-08-31**: [`D-302`](../../decisions/RT_PDR_00_Decision_Log.md) ha ratificato `COV-2`…`COV-6` — §13 — e le
> **categorie** di footprint (`Small`/`Medium`/`Large`) non sono più aperte; la loro **clearance** sì.
> ([#1833](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1833)).

---

## 1. Che cosa questo modello sostituisce

Il repository ha portato per mesi due scorciatoie che rispondevano alla domanda *«un'unità ci sta, in questa
cella?»* senza mai porsela.

| Scorciatoia | Dove viveva | Perché è caduta |
|---|---|---|
| «≥ 6 settori occupati ⇒ `Blocked`» | `FRTOccupancyThresholds::BlockedFrom` | lo **stesso numero** di settori liberi descrive spazi utilizzabili completamente diversi |
| «centro toccato ⇒ `Blocked`» | `FRTOccupancyMask::bCoreBlocked` letto da `Classify`, e `D-179` punto (3) | impedisce layout che il gioco vuole: un muro a 120°, un muro dal centro a un vertice, un diametro |

L'esempio che le rompe entrambe è nel Decision Record e sta in una riga:

```text
rocce sui settori 1,2,3   +   albero sui settori 7,8,9
  →  sei settori liberi          la soglia dice Blocked
  →  in DUE gruppi da tre        e uno ci sta benissimo
```

⚠️ **Nessuna delle due aveva un consumatore di produzione**, misurato il 2026-08-30:
`URTHexOccupancyLibrary::Classify`, `ComputeMask` e `Surcharge` sono chiamate **solo dai test**, e
`FRTHexCellData::OccupancySurcharge` è scritto dagli scenari a mano. Il superamento è quindi una correzione
di **contratto**, non un cambio di comportamento in partita: non c'era comportamento.

---

## 2. Il vincolo che NON cade

`FRTCellId` resta il **solo** nodo di navigazione e il **solo** portatore di occupancy. Da qui non nascono:

- ⛔ dodici sottocelle navigabili;
- ⛔ un mini-navmesh dentro l'esagono;
- ⛔ un secondo pathfinder;
- ⛔ coordinate float **autorevoli** intra-hex;
- ⛔ un secondo slot di occupancy;
- ⛔ un secondo Cover Resolver o un secondo sistema di Facing.

I dodici settori restano ciò che [#619](https://github.com/DegrassiAaron/refactor-tactics-main/issues/619)
li ha fatti: un **righello angolare**, non dodici caselle.

---

## 3. La regione di posa

Una **regione di posa** è un gruppo massimale di settori liberi **contigui**, con contiguità **circolare**:
i settori `11` e `0` sono adiacenti, e una regione può scavalcare lo zero.

```text
occupati {1,2,3,7,8,9}
  →  liberi {0,4,5,6,10,11}
  →  regione A = {4,5,6}     FirstWedge 4   Size 3
  →  regione B = {10,11,0}   FirstWedge 10  Size 3
```

`FirstWedge` è il settore il cui **precedente è occupato**, ed è la chiave d'ordinamento canonica: due
enumerazioni della stessa maschera producono le stesse regioni con gli stessi `FirstWedge`. È un **dato**,
non un indice d'array.

⚠️ **Un caso degenere, dichiarato**: la cella interamente libera non ha alcun settore «il cui precedente è
occupato». La regione è una, copre l'anello, e `FirstWedge` vale `0` per convenzione.

`bCoreBlocked` **non** entra nel calcolo delle regioni: il centro non è un settore.

---

## 4. Calpestabilità = esiste una posa

Una cella è calpestabile per un'unità quando **esiste almeno una regione compatibile con il suo footprint**.

```cpp
FRTFootprintProfile{ MinContiguousWedges, bRequiresFreeCore }
```

⚠️ **I valori di catalogo non sono decisi, e il tipo non li inventa.** Il default è l'**identità** —
`MinContiguousWedges = 1`, `bRequiresFreeCore = false` — che è la scelta più debole possibile e quindi
l'unica che non decide al posto di chi dovrà decidere (`COV-1`).

🔑 **`bRequiresFreeCore` è il punto in cui `D-179` viene superata.** Prima il centro occupato bloccava la
cella da solo e per tutti; ora è un requisito che un profilo **può** dichiarare — un'unità grande che deve
stare a cavallo del centro — e che nessuno paga per conto d'altri.

**Cosa questa regola NON è**: non è l'ultima parola sull'ingresso. `bBlocksMovement` resta la scelta d'autore
diretta e vince comunque; l'occupazione da parte di un'altra unità è di `URTHexSimLibrary`. Sono le tre
domande che `ERTHexWaypointReason` già distingue — `BlocksMovement`, `Occupied`, e il budget.

---

## 5. Sorgenti, opzioni, facce

Vocabolario canonico, e la mappatura sui tipi che **esistono già**:

| Termine | Che cos'è | Rappresentazione riusata |
|---|---|---|
| **CoverSource** | l'oggetto o segmento che può riparare | `FRTHexCover` su un bordo · `FRTGeometrySegment` interno |
| **CoverOption** | un modo **legale**, per chi sta in questa cella, di usare quella sorgente | `FRTCoverOption` |
| **CoverSide** | la faccia di una sorgente a due lati | `ERTCoverSide{None, A, B}` |
| **CoverAnchor** | l'ancora di presentazione/authoring associata a un'opzione | ⏳ non ancora rappresentata — `COV-2` |
| **CoverArc** | l'arco su cui la sorgente ripara | ⏳ non ancora separato da `SolidWedges` |

⚠️ **Il vocabolario dei livelli non si tocca**: `None` · `Low` · `High`, e a dirlo è `D-271`. Non esiste un
`Medium`, e `ERTHexCoverType` è riusato invece di introdurre un enum parallelo — la stessa ragione per cui
`FRTGeometrySegment::WallType` lo riusa già.

### 5.1 `OccupiedWedge != Cover`

È l'invariante che tiene separate cinque domande che una maschera sola confonderebbe:

```text
SolidWedges     dove la geometria STA
CoverArc        su quale arco RIPARA
standability    ci sta un'unità?
occupancy       chi c'è adesso?
LOS/proiettili  cosa passa attraverso?
```

Una roccia può occupare tre settori senza riparare; un muretto può riparare un arco più largo del proprio
ingombro. Che una geometria fermi vista o proiettili è deciso da `D-269`/`D-270` e implementato da
[#1830](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1830), **non** da questo modello.

### 5.2 L'identità di una sorgente è fatta di solo dato d'autore

`FRTCoverSourceId` è `{Kind, AxisOrEdge, Offset, AlongMin, AlongMax}` — un enum e quattro interi.

⚠️ **Non è un indice d'array, ed è la ragione per cui il tipo esiste.** Un indice dentro `Covers` o
`InteriorWalls` cambia appena qualcuno cancella la voce precedente, quindi non può viaggiare in un intento,
in un TurnLog o in un replay. Questa chiave sopravvive a un riordino della collezione, e riconosce come
**stesso** segmento anche quello percorso al contrario — come già fa `FRTGeometrySegment::operator==`.

### 5.3 Due facce restano due opzioni anche quando ci si gira intorno

`AccessMask` porta i settori liberi **da cui l'opzione si usa** — non l'intera regione, ma la parte di
regione che sta dalla parte giusta della sorgente.

```text
muro CONTINUO (diametro)          muro dal CENTRO a un VERTICE
  regione A ── faccia A             ┌── faccia A ──┐
  ═══════════════════════           │  una sola    │──── raggio
  regione B ── faccia B             └── faccia B ──┘  regione
  traversata: BLOCKED               traversata: SAME REGION
```

🔑 **«Valida» e «raggiungibile dal lato in cui mi trovo» sono due domande**, e il Decision Record chiede
all'editor di mostrarle separate. Schiacciarle in una sola nasconde proprio il caso da mostrare.

---

## 6. La traversata dentro la cella

`ERTIntraCellTraversal` risponde *«si passa da questo settore a quest'altro senza uscire dalla cella?»*:

| Valore | Quando |
|---|---|
| `SameRegion` | i due settori appartengono alla stessa regione libera |
| `Blocked` | regioni diverse, oppure un settore occupato — non c'è posa da cui partire o a cui arrivare |

**Stesso `CellId` non significa passaggio libero.** Un muro continuo divide lo spazio di posa di **un solo**
`FRTCellId` in due regioni sconnesse, e questo **non crea** un secondo slot di occupancy.

`SideA → SideB` richiede una traversata **esplicita**: porta/apertura, vault, reposition autorizzato, o un
percorso reale attorno all'estremo del muro sul grafo. Senza, è invalida.

⛔ **La scelta della faccia non è mai un modo di attraversare geometria bloccante.**

⚠️ **Due valori, entrambi raggiungibili, ed è deliberato.** Il terzo — la traversata autorata — esiste nel
modello e **non** nel tipo, perché in v0.1 nessun vocabolario la esprime: `FRTHexDoor` sta sui **bordi**. Va
aggiunto **in coda insieme al suo produttore** ([#1828](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1828)):
un valore d'enum che nessuno può emettere è un campo che nessuno legge, ed è il difetto che questo
repository ha già pagato quattro volte.

### 6.1 Se il livello vuole davvero due unità sui due lati

Servono **due nodi tattici distinti**, non due sottocelle implicite. Il validator deve identificarlo come
disallineamento del modello topologico e **dire la soluzione**, non solo il problema
([#1832](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1832)).

---

## 7. Occupancy: uno slot, e nessuna eccezione

**Un `FRTCellId` ha esattamente uno slot di occupancy.** Più `CoverOption` o `CoverSide` nello stesso
esagono sono stati **alternativi** dello stesso occupante, non posti in più.

```text
Unità A occupa   Hex42 / Wall17 / SideA
Unità B tenta    Hex42 / Wall17 / SideB
                 ────────────────────────
                 B sta comunque tentando Hex42.  Rifiutata.
```

Le tre conseguenze, tutte già rette dal resolver esistente:

1. **Contesa simultanea**: due unità che puntano lo stesso `FRTCellId` nello stesso micro-step ottengono
   l'esito canonico — `BlockedContested`, o `BlockedByPriority` se le priorità differiscono. Una
   `CoverOption` diversa **non** rende distinte le destinazioni, e **l'ordine di processing non sceglie mai
   un vincitore**.
2. **Arrivo tardivo**: chi entra in un micro-step precedente rende la cella occupata per chi arriva dopo,
   qualunque faccia questi avesse chiesto.
3. **Vacate-and-enter**: si applicano le normali regole deterministiche di dipendenza/convoglio. Se la
   catena uscente è bloccata, l'occupancy resta e l'ingresso è bloccato.

Pinnato da `RefactorTactics.CoverPlacement.CoverOptionsDoNotIncreaseCellCapacity`, **sul resolver vero** e
non su un predicato isolato.

---

## 8. La copertura scelta non spegne il resto della geometria

Scegliere la roccia a nord come riparo attivo **non rende l'albero a sud intangibile**. Un muro, un albero o
una roccia non selezionati continuano a influenzare LOS e proiettili quando le loro proprietà tattiche lo
richiedono — che è `D-269` e `D-270`, e ha il suo owner in
[#1830](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1830).

⚠️ **Se la copertura selezionabile arrivasse prima di quel lavoro**, il gioco avrebbe una copertura scelta e
una geometria muta: esattamente lo stato *«otticamente copre, logicamente no»* che `D-269` ha scartato.

---

## 9. Presentazione e autorità

La presentazione **può** spostare o agganciare il personaggio verso un `CoverAnchor` associato all'opzione
scelta, così che il giocatore veda quale oggetto e quale lato sta usando.

La simulazione autoritativa continua a riferirsi allo stesso `FRTCellId` più lo stato di copertura. Lo snap
è **lettura**, non decisione: nessuno stato di sola presentazione, nessun timing d'animazione, nessuna
collisione di rendering e nessun risultato di NavMesh/Recast diventa logica competitiva.

---

## 10. Privacy

La scelta di copertura di un piano è **informazione di piano**, e segue il modello team-only esistente
(invariante #6). Un client avversario **non** riceve la `CoverOption`/`CoverSide` di un piano che non ha
diritto di vedere, e il canary di privacy deve coprire anche questi campi.

🔐 **Precisato da [`D-302`](../../decisions/RT_PDR_00_Decision_Log.md) / `COV-3`**, e sono quattro canali distinti:

| Canale | Cosa vede |
|---|---|
| Server | tutto: la `CoverSelection` è canonica qui |
| Client della **propria** squadra | la proiezione di planning della propria squadra |
| Client **avversario** | **sanitizzato**: nessuna `CoverOption`/`CoverSide` nascosta |
| Replay **pubblico** | **sanitizzato** |
| Audit **privato** | completo, **solo** nel canale autorizzato |

⚠️ **Autorevole non significa replicabile, e non lo significa nemmeno dopo §13.3.** Che la
`CoverSelection` entri nell'hash di stato la rende **identità di stato**, non un permesso di trasmissione:
l'hash è un valore derivato, e un avversario che lo riceve non riceve i campi da cui è derivato.

---

## 11. Determinismo

- Tutto intero: maschere, regioni, identità di sorgente. Nessun float attraversa un valore destinato a un
  hash.
- Indipendente dall'ordine: le regioni si calcolano percorrendo un anello di dodici bit, quindi non c'è
  input da permutare; l'enumerazione delle opzioni ordina per bordo crescente, poi per regione, poi `A`
  prima di `B`. Nessun `TMap` attraversa queste funzioni.
- Stabile: l'identità di una sorgente sopravvive a un riordino della collezione e all'inversione degli
  estremi.
- 🔁 **Aggiunto da [`D-302`](../../decisions/RT_PDR_00_Decision_Log.md)**: l'**ordine di generazione non cambia l'identità** di un anchor
  (`COV-2`), e la **precedenza dell'override autorato è deterministica e locale** alla sorgente/opzione
  mirata — non dipende da quale override è stato scritto per ultimo.
- 🔁 **Nessun riferimento ad Actor** attraversa serializzazione autorevole o hash (`COV-3`): è la stessa
  ragione per cui `FRTUnitStateDigest` è una struttura di dati e non un `ARTUnit`.

---

## 12. 🔴 Il blocco misurato: `MSE-4` al centro

Il modello descritto qui è implementato e verde, **e non è ancora osservabile su geometria reale**, per una
ragione precisa e registrata.

Un segmento passante per il centro **tocca il centro**. Il centro è il vertice comune di **tutti e dodici** i
triangoli di settore, e la regola d'intersezione di `ComputeMask` è dichiaratamente conservativa. Ne segue
che ogni muro con `Offset == 0` accende dodici bit su dodici, e con zero settori liberi non esiste posa:
**la regola superata sopravvive dentro il produttore della maschera.**

```text
Deg0, Offset=0, Along ±12   →   Mask.Sectors == 0xFFF   →   0 regioni
                                (attraversa QUATTRO settori)
```

È `MSE-4`, aperta, con owner [#1826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1826) e
due uscite già scritte. **Non si decide dentro una PR che parla d'altro.**

Misurato da `RefactorTactics.CoverPlacement.CentreContactRuleStillCollapsesTheWholeCell`, che diventa rosso
il giorno in cui si chiude — ed è il suo scopo.

---

## 13. Ciò che `D-302` ratifica

[`D-302`](../../decisions/RT_PDR_00_Decision_Log.md) consuma le risposte d'autore su `COV-2`…`COV-6`. Le regole qui sotto sono **normative**;
⚠️ **nessuna di esse è implementata**, ed è misurato — vedi §13.5.

### 13.1 Chi produce un `CoverAnchor`: **ibrido** (`COV-2`)

Il default è **generato** e deterministico dalla geometria tattica compatta. L'autore **aggiunge** anchor e
**sovrascrive** quelli generati per i casi speciali.

🔑 **La precedenza autorata è locale.** Vale per la **sorgente/opzione mirata**, non per la cella: le
opzioni generate non correlate **restano**. Sovrascrivere una faccia non azzera le altre.

✅ **La forma non è nuova.** È quella di [`D-131`](../../decisions/RT_PDR_00_Decision_Log.md) / `FRTHexCover::bGenerated`, il cui contratto in
`RTGeometryBake.h` dice già che il rebake *«rimuove le coperture con `bGenerated = true`»* e che *«una
copertura dipinta a mano vince sempre»*. Il salto è dichiarato: là la provenienza distingue due produttori
di un **campo**, qui di un'**entità**.

⛔ Non riapre `GEO-5` (identità dell'anchor derivata) né `GEO-7` (bordo condiviso = due facce),
chiuse da [`D-288`](../../decisions/RT_PDR_00_Decision_Log.md).

### 13.2 `CoverSelection` è stato autorevole discreto (`COV-3`)

- **Identificatori stabili e canonici.** Nessun float autorevole, **nessun riferimento ad Actor** in
  serializzazione o hash.
- **Canonica sul server.** Il client propone, l'autorità valida e applica — [`D-003`](../../decisions/RT_PDR_00_Decision_Log.md).
- ✅ **Metà di questa regola è già vera, e non è un'aspirazione**: `FRTCoverSourceId` è già interamente
  intero (`Kind`, `AxisOrEdge`, `Offset`, `AlongMin`, `AlongMax`), con `operator==` e `GetTypeHash` a ordine
  fisso — *«nessun float attraversa un identificatore che prima o poi finirà in un hash»*.

Il **formato** di bit-packing resta non congelato: sceglierlo dentro una PR d'implementazione sarebbe
sceglierlo per inerzia, ed è la ragione per cui `COV-3` esisteva. Ciò che è deciso è il **contratto**.

### 13.3 `CoverSelection` entra nel digest e nell'hash (`COV-4`)

Due stati che differiscono **solo** per la copertura scelta possono risolvere in modo diverso, quindi sono
**stati logici distinti**. È letteralmente il criterio scritto in `RTMatchStateHash.h`: *«un campo entra
nell'hash se e solo se due oggetti possono differire solo per quello»*.

⚠️ **Delimita [`D-243`](../../decisions/RT_PDR_00_Decision_Log.md) senza ribaltarla**, dove [`D-289`](../../decisions/RT_PDR_00_Decision_Log.md) l'aveva già delimitata: lo
*spicchio di posa* resta fuori da snapshot e hash — non è uno stato —, la **scelta** di copertura entra.
Sono due oggetti diversi.

🔑 **`COV-3` e `COV-4` sono una decisione sola**, come `PLC-1` e `PLC-7`: l'identità di stato senza una
serializzazione stabile non è verificabile, e una serializzazione stabile che non entra nell'hash non
protegge nulla.

### 13.4 `Facing` e copertura sono indipendenti (`COV-5`)

- La copertura **non** auto-ruota l'unità e **non** vincola il `Facing`.
- La mitigazione si applica **solo se passano entrambi** i test direzionali applicabili: quello della faccia
  di copertura **e** quello dell'arco di `Facing`. ✅ Questo **conferma** `CP 16.2` — *un colpo fuori
  dall'arco frontale annulla la riduzione* — invece di emendarlo.
- **Direzione d'impatto**, per categoria: diretto/mischia = **sorgente → bersaglio**; linea/traiettoria =
  **direzione d'impatto**; area = **centro d'impatto → bersaglio**.
- `IgnoreCover` **scavalca il cover resolver**: non è una mitigazione portata a zero, è un ramo che non si
  percorre.

🔑 **Tre dei quattro valori che `FAC-13` proponeva sono decisi qui** (`FromSource`, `FromTrajectory`,
`FromImpactCenter`). ⚠️ **`FAC-13` resta aperta per il quarto**: il colpo **senza sorgente puntuale**
— terreno che brucia, effetto non direzionale — non è deciso, ed è ciò che l'autore ha rinviato.

### 13.5 Entrare in copertura costa **+1 MP** (`COV-6`)

Raggiungere una cella in copertura costa **un punto movimento oltre** il costo normale del terreno.
**Nessun bonus generico cover-to-cover**: se muoversi fra coperture fosse gratis o premiato, restare in
copertura smetterebbe di essere una scelta.

🔑 **È una voce esplicita di economia dell'azione**, non un costo dedotto dalla geometria: il numero
vive nel catalogo, non in `ComputeMask`. Il **riposizionamento dentro la cella** non è questo costo ed è
`COV-7`, aperta.

### 13.6 ⏳ Stato d'implementazione — misurato, non dichiarato

| Regola | Implementata? | Misura |
|---|---|---|
| `COV-2` provenienza ibrida | ❌ **no** | nessun tipo `CoverAnchor` esiste in `Source/` |
| `COV-3` `CoverSelection` serializzata | ❌ **no** | nessun tipo `CoverSelection` esiste; `FRTCoverSourceId` è già discreto e stabile, ed è l'unica metà vera |
| `COV-4` nel digest | ❌ **no** | `FRTUnitStateDigest` ha **sette** campi: `UnitId`, `Cell`, `Health`, `Shield`, `Energy`, `bAlive`, tag. Nessuno è la copertura |
| `COV-5` doppio test direzionale | ❌ **no** | `EffectiveCoverReduction` legge il `Facing`, non una copertura **scelta** |
| `COV-6` +1 MP | ❌ **no** | un costo di copertura in `Source/` dà **zero** |

⛔ **Una decisione non chiude un'implementazione.** Le sette sub-issue di `E23.6`/`E23.7`
([#1826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1826)…[#1832](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1832)) **restano aperte**.

---

## 14. Decisioni aperte

Tutte in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) §
*Aperte — copertura selezionabile e posa nella cella*, con owner
[#1833](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1833).

| ID | Domanda |
|---|---|
| `COV-1` | **solo la clearance**: le categorie `Small`/`Medium`/`Large` sono decise — §13 — e aperta è la riconciliazione con [`D-071`](../../decisions/RT_PDR_00_Decision_Log.md) |
| ~~`COV-2`~~ | ~~`CoverAnchor` autorati, generati o ibridi~~ — ✅ **ibrido**, [`D-302`](../../decisions/RT_PDR_00_Decision_Log.md) §13.1 |
| ~~`COV-3`~~ | ~~serializzazione e rappresentazione di rete della scelta~~ — ✅ **contratto deciso**, §13.2 |
| ~~`COV-4`~~ | ~~la scelta entra nel digest di stato e nell'hash?~~ — ✅ **sì**, §13.3 |
| ~~`COV-5`~~ | ~~interazione finale fra `Facing` e copertura scelta~~ — ✅ **indipendenti, doppio test**, §13.4 |
| ~~`COV-6`~~ | ~~costi/bonus del movimento da copertura a copertura~~ — ✅ **+1 MP, nessun bonus**, §13.5 |
| `COV-7` | regole finali di vault e reposition |
| `COV-8` | rigenerazione delle `CoverOption` dopo la distruzione di una sorgente |
| `MSE-4` | il contatto puntuale è invasione? — **innesco arrivato**, vedi §12 |

---

## 15. Come si verifica

| Domanda | Chi risponde |
|---|---|
| le regioni sono canoniche e circolari? | `CoverPlacement.RegionsAreCanonicalAndCircular` |
| la cella vuota è calpestabile e senza copertura? | `.EmptyCellIsStandableWithNoCover` |
| sei settori occupati in due gruppi non bloccano? | `.TwoObstacleGroupsExposeIndependentOptions` |
| il centro non blocca più da solo? | `.CenterCrossingWallNoLongerBlocksTheCell` |
| la posa dipende dal footprint e non dal conteggio? | `.FootprintDecidesStandabilityNotTheCount` |
| un muro a 120° lascia la cella calpestabile? | `.WideWallLeavesCellStandable` |
| un muro continuo separa le facce e rifiuta la transizione? | `.ContinuousWallSeparatesSidesAndRejectsTransition` |
| un raggio centro-vertice espone due facce in una regione? | `.CenterToVertexWallExposesBothSidesInOneRegion` |
| le opzioni aumentano la capacità? | `.CoverOptionsDoNotIncreaseCellCapacity` |
| l'identità sopravvive al riordino? | `.SourceIdIsStableAcrossCollectionReordering` |
| scegliere una sorgente ne spegne altre? | `.SelectingOneSourceDoesNotSuppressTheOthers` |
| il blocco `MSE-4` è ancora là? | `.CentreContactRuleStillCollapsesTheWholeCell` |

**Misurato il 2026-08-30**: 13/13 verdi, e suite completa `1472/1472, 0 fallimenti`, run dichiarata
**VALIDA** da `scripts/rt-suite.ps1`.

⏳ **Cosa NON è ancora verificato, e va dichiarato**: PIE e pacchetto. Le primitive non hanno chiamanti di
produzione, quindi non c'è nulla da guardare a schermo finché
[#1827](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1827) non le cabla. La Definition of
Done della v0.1 chiede evidenza, e qui l'evidenza è la suite — non un'immagine.

---

## 16. Errori che questo modello previene

1. **Contare i settori invece di guardarne la forma.** Sei liberi in fila e sei in due gruppi da tre sono lo
   stesso numero e due spazi diversi.
2. **Dedurre il divieto dal centro.** Un muro che attraversa il centro **divide**, e dividere non è vietare.
3. **Confondere «valida» con «raggiungibile».** Due facce possono essere entrambe legali e una sola
   raggiungibile da dove ci si trova.
4. **Trattare la faccia come un posto.** Due facce non sono due unità: se il livello ne vuole due, servono
   due celle.
5. **Far sparire la geometria non scelta.** La copertura attiva è una scelta tattica, non un filtro sulla
   fisica.
6. **Mettere un numero di bilanciamento dentro una costante.** `BlockedFrom = 6` non l'ha scelto nessuno per
   una regola di gioco, e per mesi ha significato «cella non calpestabile».
