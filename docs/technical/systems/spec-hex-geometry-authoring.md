# Spec — Geometria architettonica, occupancy e cottura verso i dati tattici

> `CURRENT` · **Stato**: owner del modello, allineato al codice il **2026-08-13** · **Owner**: questo file
> **Autorità**: subordinata a [`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md),
> [ADR-0002](../../decisions/adr-0002-griglia-esagonale.md) e al
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md).
> **Nato da**: [revisione spec panel del 2026-08-13](../../roadmap/plans/hexgeometry-editor-spec-panel-2026-08-13.md),
> che ha verificato l'assenza di un owner per questo tema — viveva solo in referti, archivio e registry.

Questo documento risponde a **una** domanda: *cosa succede fra il gesto con cui un designer disegna un muro e
il dato che il resolver legge per dire «non si passa»?*

> ⚠️ **Non è un tracker.** Lo stato di implementazione vive nel
> `feature-registry.yaml` e nelle issue. Qui c'è il **modello**: cosa
> significano le cose e perché sono separate. Se una riga di questo file dichiara uno stato, è un difetto.

---

## 1. Cosa questo documento non possiede

| Tema | Owner |
|---|---|
| Coordinate, `FRTCellId`, transizioni fra layer, formato dell'asset | [`spec-mappa-multilivello.md`](../architecture/spec-mappa-multilivello.md) |
| A\*, archi percorribili, costi del cammino | [`spec-pathfinding-pf3-pf4.md`](../architecture/spec-pathfinding-pf3-pf4.md) |
| Regole di copertura, riduzione danno, distruzione | [`../../gameplay/spec-copertura-cp91.md`](../../gameplay/spec-copertura-cp91.md) · [`../../gameplay/spec-copertura-alta-cp92.md`](../../gameplay/spec-copertura-alta-cp92.md) |
| Superfici, stati, propagazione | [`../../gameplay/spec-terreni-e8.md`](../../gameplay/spec-terreni-e8.md) |
| LOS e forme di targeting | [`h6-4-hex-vision-spec.md`](h6-4-hex-vision-spec.md) |
| Visualizzazione in editor: cosa serve vedere e perché | [`brief-editor-map-viz.md`](../tooling/brief-editor-map-viz.md) |
| Stato di avanzamento | `../../roadmap/feature-registry.yaml` |

---

## 2. Le due griglie di numeri, e perché non vanno confuse

È l'errore più costoso di quest'area, e si previene con una riga:

```text
movimento · facing · bordi · cover · porte   →  SEI direzioni
misura di quanto una cella è invasa          →  DODICI settori
```

I dodici settori **non sono** dodici direzioni. Non definiscono vicini, non definiscono facing, non
definiscono su quale lato sta una copertura. Sono un righello.

La corrispondenza è esatta e non approssimata: ogni direzione esagonale copre **due** settori consecutivi
(`E` = 0 e 1, `NE` = 2 e 3, …), perché il settore 0 è ancorato al primo vertice del pointy-top, a `-30°`, lo
stesso da cui `URTHexLibrary` costruisce il perimetro.

### 2.1 Pointy-top: non esistono Nord e Sud

Le sei direzioni sono `E (+1,0)` · `NE (+1,−1)` · `NW (0,−1)` · `W (−1,0)` · `SW (−1,+1)` · `SE (0,+1)`.

In un pointy-top i **vertici** stanno in alto e in basso, quindi non esiste un lato che guarda a nord. Ogni
ragionamento in termini di «via nord / via sud» usa un vocabolario che la griglia non ha — ed è un errore già
commesso in sede di authoring.

---

## 3. La geometria architettonica non è vincolata ai lati dell'esagono

**Un muro non deve seguire il perimetro di un esagono.** Un edificio ha angoli a 90°, stanze rettangolari e
porte dove serve; la griglia si **sovrappone** a quella geometria, non viceversa.

```text
HEX       = discretizzazione tattica
GEOMETRIA = gesto d'authoring, forma architettonica
```

> ⚠️ Questa riga esiste perché la sua negazione è stata dedotta **due volte** da lettori diversi, a partire
> dalla frase «le coperture stanno sui bordi». Quella frase parla del **dato cotto**, non del mondo: ciò che
> sta sul bordo è l'effetto tattico, perché la direzionalità dev'essere esprimibile con interi dentro un hash.
> Vedi [`spec-mappa-multilivello.md`](../architecture/spec-mappa-multilivello.md) §4.

### 3.1 Ma non è nemmeno float arbitrario

La geometria **tatticamente significativa** è quantizzata. La grammatica ammette tre famiglie:

1. direttrici principali derivate dall'esagono;
2. ortogonali a tali direttrici;
3. segmenti sul lato o sul perimetro dell'esagono.

L'effetto pratico: sono possibili configurazioni a **90°** senza ridurre la mappa a sei soli muri possibili.

```text
la geometria PUÒ non stare sul bordo dell'hex
la geometria tattica NON PUÒ avere endpoint o angoli float arbitrari nell'authority serializzata
```

> 🔎 **«Junction» non è un concetto separato di questa grammatica.** La rappresentazione è a **polilinea**, e
> la continuità strutturale ne discende: `ComputeMask` fa l'OR dei settori attraversati, quindi una junction
> è trasparente al modello. Deciso il 2026-08-12 —
> [referto](../../roadmap/plans/level-designer-handoff-spec-panel-2026-08-12.md) §6.
>
> ⚠️ **L'elenco qui sopra diceva «quattro famiglie» e la quarta era `junction compatibili`**: la nota la
> toglieva, la lista la rimetteva. Portato a **tre** il 2026-08-13 — perché quella lista è la stessa che
> #620 copia nel proprio *Scope*, e da lì finiva nel suo elenco di validator e quindi in un DoD che chiedeva
> «un caso rosso e uno verde» per un concetto già cancellato dalla suite. Una lista e la sua eccezione non
> possono convivere in due paragrafi consecutivi: chi copia, copia la lista.

### 3.2 Il tipo dell'authority è discreto, la polilinea è un derivato — `D-127`

```text
AUTHORITY          asse (enum) + offset (interi) + layer     serializzata, hashabile
      │
      │ conversione
      ▼
CALCOLO            FRTOccupancyPolyline { Points, bClosed }  float, ingresso di ComputeMask
```

Ciò che il designer disegna si serializza **senza estremi float**. `FRTOccupancyPolyline` non è l'authority:
è il tipo che entra in `ComputeMask`, e il float ci sta legittimamente — è la §11, *«il mondo Unreal usa
`FVector` per disegnare»*.

> ⚠️ **Questa riga diceva un'altra cosa, e la lettura opposta era quella naturale.** Diceva: *«Il tipo
> d'ingresso (`FRTOccupancyPolyline`) esiste già, dichiarato in anticipo perché altrimenti il primo commit
> avrebbe inventato un tipo che #620 avrebbe dovuto cambiare»* — che si legge come «è l'authority». Ma quel
> tipo è `USTRUCT(BlueprintType)` con `UPROPERTY(EditAnywhere)` su `TArray<FVector2D>`
> (`RTHexOccupancyLibrary.h:114-126`), cioè **già pronto a essere salvato**: in quella lettura la voce di DoD
> di `#620` *«la grammatica è espressa in interi o enum»* era **insoddisfacibile**, e il difetto sarebbe
> emerso al primo salvataggio. Resta vero il motivo per cui il tipo fu dichiarato in anticipo — evitare che il
> primo commit ne inventasse uno — solo che quel tipo è il **derivato**, non la sorgente.

`MSE-1` ne esce **ristretta**: decide *dove* vive il source editabile e chi vince al rebake, non più *in che
tipo*. — E una seconda volta con `D-129`, che togliendo il volume dal bake ne cambia anche il **soggetto**:
non più `bBlocksMovement`, ma `FRTHexCover`. Vedi §12.

> 🔑 **Il quanto è relativo al punto notevole della direzione, non a `HexSize` — ed è questo che rende
> esprimibile la terza famiglia.** Un quanto lineare uniforme non può esprimere insieme i due punti
> notevoli dell'esagono: l'apotema vale `HexSize · √3/2`, e nessun numero di suddivisioni di `HexSize` la
> rende intera — con `HexSize = 100` fa `86,602540…`. Un muro appoggiato al lato avrebbe quindi un offset
> **non rappresentabile**, e «segmenti sui lati / perimetro» sarebbe inesprimibile in interi.
>
> Qui `Q` quanti lungo una direzione valgono *esattamente il suo punto di confine* — vertice a raggio
> pieno o punto medio di lato ad apotema, a seconda della direzione. Le due lunghezze diverse diventano lo
> stesso intero: il muro sul lato `E` è `Offset = Q`, estremi `±Q/2`, e ricostruisce i due vertici a meno
> dell'epsilon di macchina. Lo dimostra
> `RefactorTactics.GeometryGrammar.PerimeterWallReconstructsHexVertices`; mutando la perpendicolare in una
> direzione normalizzata, quel test cade.

### 3.3 Il validator dice **quale** regola è caduta, e non blocca da solo

Il rifiuto è a **due strati**, e il precedente esiste già nel repository:

| Strato | Forma | Precedente |
|---|---|---|
| **Segnala** | lista di violazioni, non blocca | `URTHexMapAsset::ValidateMap()` → `TArray<FString>` (`RTHexMapAsset.h:199`) |
| **Rifiuta** | l'operazione d'authoring non avviene | `URTHexCoverLibrary`, *«più severo di `ValidateMap`»* (`RTHexCoverLibrary.h:136`) |

La violazione è un **reason code enumerato**, non una stringa: §10 elenca già `reason code` fra ciò che il
runtime possiede, e il pattern è consolidato in cinque enum — `ERTActionInvalidReason`,
`ERTHexTargetReason`, `ERTHexWaypointReason`, `ERTDisplacementBlockReason`, `ERTMatchEndReason` — tutti
`UENUM(BlueprintType)`, con `None` come primo valore e un commento per voce che dice **perché** è separata
dalle altre.

> 🔑 **Non è una preferenza di stile: è ciò che rende possibile la verifica di mutazione.** Se il validator
> restituisce testo libero, «allentata una regola, cade *esattamente* il test che la protegge» non è
> asseribile — due regole diverse producono messaggi simili e il test dovrebbe confrontare stringhe.

I simboli: `FRTGeometrySegment` (l'authority), `ERTTacticalAxis`, `ERTGeometryViolation` e
`URTGeometryGrammarLibrary` in `Source/RefactorTactics/Map/RTGeometryGrammar.h`. Lo stato di avanzamento
vive nel `feature-registry.yaml` e nelle issue, non qui — §1.

---

### 3.4 Le regole che riguardano **due** segmenti — `D-288`

Le regole di §3.3 guardano un segmento **preso da solo**: asse, lunghezza, layer, bordi editabili. Sono
quelle che `ValidateSegment` può rifiutare prima che il gesto venga committato.

Ce n'è una seconda famiglia, che un segmento solo non può violare: riguarda la **collezione**, e vive
perciò nello strato che *segnala* — `URTGeometryGrammarLibrary::Validate`, e da lì `ValidateMap`.

| Reason code | Configurazione | Perché non è la regola accanto |
|---|---|---|
| `DuplicateSegment` | lo **stesso** segmento due volte, anche percorso al contrario | identità geometrica: `operator==` usa `Min`/`Max` sugli estremi |
| `OverlappingSegments` | due **collineari** — stesso asse, offset e layer — i cui tratti si intersecano in **più di un punto** | gli estremi sono diversi: non è un duplicato |
| `CrossingOffAnchor` | due segmenti di **assi diversi** che si incontrano in un punto che **non** è uno dei tredici anchor | non è una sovrapposizione: si toccano in un punto solo |

Tre configurazioni, tre codici. Non è ridondanza: è ciò che rende asseribile la verifica di mutazione di
§3.3 — allentata una regola, deve cadere **esattamente** il test che la protegge, e due configurazioni che
condividessero un codice non lo permetterebbero.

**Che cosa NON è una violazione**, e va detto perché sono i casi che una regola scritta male sacrifica:

- due muri che si **incrociano al centro** della cella, o su qualunque altro anchor — è la configurazione
  più comune che esista, e resta legale;
- due muri collineari **consecutivi** che condividono un estremo — è un muro lungo disegnato in due gesti,
  cioè il modo normale di disegnarlo;
- una **T** che termina su un anchor di un altro segmento: legale, e **non** impone di spezzare il segmento
  attraversato. Le junction non appartengono a questa grammatica in v0.1 (`GEO-6` di `D-288`), e una regola
  d'incidenza che segnalasse *«passa per un anchor dove un altro finisce»* le reintrodurrebbe dalla porta di
  servizio;
- due segmenti su **layer diversi**: la geometria di un piano non tocca quella di un altro.

> 🔑 **L'incidenza si decide sugli interi, ed è §11 applicata a una domanda fra due segmenti.** *«Quel punto
> è un anchor?»* chiesta in `FVector2D` sarebbe un confronto con tolleranza, e una tolleranza qui non è
> precisione ma **regola di gioco**: decide quali muri un livello può contenere.
>
> La misura che lo rende possibile: i tredici punti notevoli cadono su coordinate **intere** nella base
> `(HexSize · √3/4, HexSize/4)` — il vertice a `-30°` è `(2, -2)`, il punto medio del lato `E` è `(2, 0)`,
> quello del lato `NE` è `(1, 3)`. L'apotema irrazionale che `RT_GeometryQuanta` esiste per aggirare
> sparisce, perché `√3` finisce nell'**unità** dell'asse `X` invece che nelle coordinate. Il punto
> d'intersezione si confronta moltiplicato per il denominatore, così la divisione non viene mai eseguita.

⚠️ **Il costo è quadratico nel numero di segmenti di una cella**, e va tenuto onesto: la validazione della
collezione gira in authoring e su `ValidateMap`, mai nel ciclo di gioco. Se la scala lo richiedesse, il
raggruppamento naturale è per **asse e offset**.

⚠️ **Il raggruppamento per cella non è un'ottimizzazione, è la correttezza.** I segmenti sono in coordinate
**locali** di cella: due muri di celle diverse con gli stessi numeri non si incrociano affatto, e validare
`InteriorWalls` in un colpo solo li segnalerebbe tutti.

---

## 4. Il bordo condiviso è una primitiva sola

Coperture e porte sono proprietà di **bordo**, e il bordo `E` di `A` è **lo stesso bordo fisico** del bordo
`W` del suo vicino. Il punto medio deve coincidere visto dalle due celle.

```cpp
URTHexLibrary::EdgeMidpointWorld(...)   // il centro del bordo
URTHexLibrary::EdgeRotation(...)        // il suo orientamento
URTHexLibrary::OppositeDirection(...)   // il bordo visto dall'altra cella
```

Il test che tiene la proprietà è `RefactorTactics.Hex.EdgeMidpointIsSharedByBothCells`.

**Non** ricavare un bordo scegliendo a mano indici di `HexCorners`, e **non** incidere angoli nel codice: se
la convenzione dei sei lati cambiasse, la geometria derivata la segue, quella incisa mente in silenzio.

---

## 5. Occupancy: dodici settori più il centro

`FRTOccupancyMask` porta dodici bit e un booleano:

| Campo | Ruolo |
|---|---|
| `Sectors` | dodici bit, uno per settore da 30°, ancorati al primo vertice (`−30°`) |
| `bCoreBlocked` | il **centro** della cella è occupato |

`ComputeMask` è **pura, headless e indipendente dall'ordine**: i settori si accendono con un OR, quindi la
stessa geometria presentata in ordine diverso produce la stessa maschera *per costruzione* — e un test lo
dimostra invece di dedurlo (`MaskIsIndependentOfInputOrder`).

`bCoreBlocked` non è deducibile dal conteggio: un footprint più grande dell'intera cella non tocca un solo
triangolo di settore, e senza quel booleano risulterebbe `Free`.

> 🔑 **La maschera è una MISURA, e da sola non è un verdetto** — `D-289`, 2026-08-30. Che un'unità ci stia lo
> decide la **forma** dello spazio libero, non il conteggio dei bit: i gruppi di settori liberi *contigui*
> sono le **regioni di posa**, e vivono in
> [`spec-cover-placement-intra-hex.md`](spec-cover-placement-intra-hex.md) §3. Questa sezione resta l'owner
> di **come si misura**; non lo è più di **cosa se ne deduce**.

I dodici triangoli **pavimentano l'esagono esattamente**: i punti di confine alternano vertice
(raggio pieno) e punto medio di lato (raggio inscritto), e fra un vertice e il punto medio adiacente il bordo
dell'esagono è un segmento dritto.

### 5.1 Che cosa alimenta la misura: il volume, non i muri — `D-125`

È la distinzione che decide tutto il resto di questa sezione.

```text
entità con VOLUME   →  footprint chiuso  →  occupancy  →  libera · ingombrata · piena
muro                →  polilinea aperta  →  bordo      →  FRTHexCover{Low|High}
```

Una cella è **libera**, **ingombrata** o **piena** — `Free`, `Constrained`, `Blocked` — e ciò che la riempie
sono **unità, materiali, elementi interattivi e statici, props**: cose che stanno *dentro* la cella e tolgono
spazio a chi vorrebbe starci.

**Un muro no.** È un concetto di **bordo**: sta *fra* due celle, ha spessore trascurabile, e la domanda a cui
risponde non è «quanto è ingombra questa cella» ma «cosa succede attraversando questo lato». Cuoce in
`FRTHexCover` — vedi §8.1 — e **non entra nel conteggio dei settori**.

> 🔑 **Il tipo d'ingresso lo diceva già, e nessuno l'aveva letto così.** `FRTOccupancyPolyline` ha due campi:
> `Points` e `bClosed`. **Nessuno spessore.** Le due forme che ammette non sono due sintassi per la stessa
> cosa — sono le **due domande**: chiusa = un footprint, cioè volume; aperta = un bordo.

⚠️ **Un numero di questo repository è stato misurato sull'ingresso sbagliato.** `MSE-2` dava «due muri
consecutivi rendono la cella `Blocked` con quattro lati aperti», ottenuto passando muri perimetrali a
`ComputeMask`. Nella pipeline reale quei muri diventano coperture. Il segnale c'era: le quattro fixture
originali stanno tutte a raggio `0.3`–`0.6`, **dentro** la cella.

### 5.2 Il contatto sul confine conta

Un footprint appoggiato esattamente al confine fra due settori **li invade entrambi**. È una scelta
conservativa e deliberata, protetta da
`RefactorTactics.HexOccupancy.SegmentOnSectorBoundaryOccupiesBothAdjacentSectors`.

Resta aperto il solo caso **puntuale** — un bordo che passa esattamente per un **vertice**, punto in comune
fra quattro triangoli di settore: è `MSE-4` in §12.

> Negli esagoni la regola conservativa non deve difendere il *varco diagonale*: le sei direzioni di
> `ERTHexDirection` condividono ciascuna un **lato intero** (`RTCellId.h:11-19`), e non esiste adiacenza per
> solo vertice.

---

## 6. Free, Constrained, Blocked — 🔴 **non decidono più la calpestabilità**

> ### ⚠️ Questa sezione è stata delimitata il 2026-08-30 da [`D-289`](../../decisions/RT_PDR_00_Decision_Log.md)
>
> La tabella qui sotto **resta vera come classificazione di strettezza**, ed è quello che il suo unico
> consumatore ha sempre letto: `OccupancySurcharge`, cioè *«quanto costa passare di qui»*.
>
> 🔴 **Ciò che non è più vero è la parola `Blocked` letta come «non ci si sta».** Due righe di questa tabella
> rispondevano a una domanda che non è la loro:
>
> | Riga | Perché è caduta |
> |---|---|
> | `≥ BlockedFrom` (6) | lo **stesso numero** di settori liberi descrive spazi utilizzabili diversi: rocce su `1,2,3` più albero su `7,8,9` lasciano sei liberi **in due gruppi da tre**, e un'unità ci sta |
> | `bCoreBlocked` ⇒ `Blocked` | è `D-179` punto (3), superata: un muro che attraversa il centro **divide** lo spazio di posa, e dividere non è vietare |
>
> **Chi risponde adesso**: `URTHexCoverPlacementLibrary::HasLegalPlacement`, che cerca una **regione di
> settori liberi contigui** compatibile con il footprint dell'unità. Owner documentale:
> [`spec-cover-placement-intra-hex.md`](spec-cover-placement-intra-hex.md).
>
> ✅ **Nessun comportamento di partita è cambiato**, e va detto: `Classify` non ha mai avuto un chiamante di
> produzione — solo test — e `D-179` punto (3) non era mai stata implementata (`git grep "Offset == 0" --
> Source/` è vuoto). È una correzione di **contratto**.

| Settori occupati | Classificazione | Cosa significa **oggi** |
|---|---|---|
| `0 – 3` | `Free` | si passa senza sovrapprezzo |
| `≥ ConstrainedFrom` (4) | `Constrained` | si passa, e costa `ConstrainedSurcharge` in più |
| `≥ BlockedFrom` (6) | `Blocked` | ⚠️ **cella molto stretta**, non «cella impassabile» |
| `bCoreBlocked` | `Blocked` | ⚠️ idem — il centro è un requisito di *profilo*, non un divieto |

Le soglie vivono in `FRTOccupancyThresholds`, nel modulo **runtime** e non in un property set d'editor, per
due ragioni: in `Source/RefactorTacticsEditor/` non esiste alcun test — e una soglia che nessun test può
cambiare è una costante travestita — e il costo di cella entra nell'hash di stato partita, quindi una soglia
per-utente farebbe produrre a due autori due mappe con hash diverso dalla stessa geometria.

**Il pannello d'editor le espone; non le possiede.**

---

## 7. `Constrained` ha un consumatore, ed è separato da `MoveCost`

`Constrained` non è un'etichetta: costa. Il sovrapprezzo vive in un campo **suo**,
`FRTHexCellData::OccupancySurcharge`, e **non** dentro `MoveCost`.

La ragione è una sequenza concreta:

```text
corridoio stretto (surcharge geometrico)
  → la superficie diventa acqua           (E8, superfici dinamiche)
    → Cleanup ripristina Floor.MoveCost
      → se il surcharge fosse fuso in MoveCost, il costo geometrico SPARIREBBE
```

Il costo totale canonico è calcolato in un punto solo (`FRTHexCellData`, `RTHexCellData.h:193`):
`max(0, MoveCost) + max(0, OccupancySurcharge)`. **Non reimplementare quella somma** in nuovi consumatori:
due modi di dire la stessa cosa prima o poi divergono.

`Blocked` **non** paga sovrapprezzo: chi la rende impassabile è il bordo, non il costo.

---

## 8. La cottura: authoring → dati tattici canonici

```text
Authoring Geometry
   ↓  quantizzazione + validazione        (#620)
celle investite / bordi toccati
   ↓  occupancy                            (#619, fatto)
   ↓  bake                                 (#621)
dati tattici canonici
   ↓
pathfinding · LOS · cover · resolver
```

**Dopo il bake la geometria è arte.** Il runtime non interroga una mesh per sapere se si passa, se si vede,
se c'è copertura o se una cella è calpestabile: legge dati tattici.

### 8.1 Mapping della cottura

| Authoring | Dato cotto |
|---|---|
| `LOW WALL` | `FRTHexCover{Low}` sul bordo |
| `WALL` | `FRTHexCover{High}` sul bordo |

> 🔑 **La cottura si ferma ai bordi — `D-129`.** La tabella aveva due righe in più: `footprint solido →
> bBlocksMovement` e `footprint void/precipizio → bBlocksMovement + !bBlocksLineOfSight`. **Sono uscite**, e
> la ragione è la §5.1 letta fino in fondo: il volume viene da *«unità, materiali, elementi interattivi e
> statici, props»* — cose che si **piazzano**, non che si **disegnano**. Coerentemente la grammatica di
> `#620` esprime **solo segmenti aperti** (`FRTGeometrySegment` non ha campo di chiusura), quindi il
> footprint non è mai stato un suo ingresso.
>
> `bBlocksMovement` e `bBlocksLineOfSight` restano scritti dal **dato d'autore** — un produttore solo. Il
> volume non è cancellato: è un lavoro che avrà un owner quando esisteranno entità con footprint proprio,
> e quell'owner non è `#621`.

> 🔎 **Il bake NON scrive `Surface`.** La proposta `void/cliff → ERTHexSurface::Void` è stata valutata e
> **respinta** il 2026-08-12. `Void` sarebbe una superficie *dipinta* fra nove valori, e nessuna regola
> geometrica sa scegliere fra nove; soprattutto, `Fill` propaga sulla **contiguità di superficie**, quindi una
> `Surface` cotta cambierebbe il confine di ogni futuro flood fill — un effetto sullo **strumento**, non sul
> dato. La coppia `bBlocksMovement` + `!bBlocksLineOfSight` dice già «non ci si sta sopra, ma ci si vede
> attraverso», e distingue un precipizio da un muro.
> [Decisione](../../OPEN_DECISIONS.md) · [referto](../../roadmap/plans/level-designer-handoff-spec-panel-2026-08-12.md).

### 8.2 Una copertura sa se l'ha prodotta il bake — `D-131`

`FRTHexCover` porta `bGenerated`, e il rebake della regione investita:

```text
1. rimuove le coperture con bGenerated = true
2. riscrive quelle derivate dai segmenti correnti
3. non tocca MAI quelle a false — dipinte a mano
```

Ne segue che il bake è **idempotente** e che *togliere* un segmento rimuove la sua copertura. È il nodo
vero di `MSE-1`, e non era la sovrascrittura: senza provenienza, un rebake non sa distinguere «la copertura
che avevo prodotto io e ora va tolta» da «quella dipinta a mano che va preservata».

> ⚠️ **`bGenerated` non entra in `ComputeHash`.** Le coperture ci entrano *«perché sono dato autorevole —
> cambiano il danno subito»* (`RTHexMapAsset.cpp:249`), ma la **provenienza** non cambia una partita. Se
> entrasse, due mappe che si giocano in modo **identico** avrebbero hash diversi solo perché una copertura è
> stata disegnata invece che dipinta — un falso positivo di divergenza contro *replay divergence = 0*.
> Va pinnato da un test.

> 🔎 **La metà runtime di `MSE-1` non esisteva.** `RTGameMode.cpp:264` **duplica** la mappa d'autore a inizio
> partita (CP 8.4), quindi `Action.CreateCover` e lo spostamento delle coperture scrivono sulla **copia**:
> nessun rebake può cancellarli. Misurato, non assunto.

Un `LOW WALL` visuale che non cuocesse in `FRTHexCover{Low}` creerebbe **due rappresentazioni dello stesso
oggetto**. Non esiste un secondo `Walls[]` autorevole interrogato dal gameplay.

---

## 9. Cinque domande diverse, cinque dati diversi

Non collassare queste proprietà per semplificare il tool: una cella può essere traversabile **e** opaca.

| Concetto | Che domanda risponde | Authority |
|---|---|---|
| Geometry authoring | cosa ha disegnato il designer? | source editor-only |
| Occupancy | quanto la cella è invasa? | dato runtime |
| Movement block | posso stare, entrare, passare? | dato runtime |
| LOS block | la vista attraversa? | dato runtime |
| Cover | ricevo protezione attraversando questo bordo? | dato runtime di **bordo** |
| Door | questo bordo è transitabile nello stato corrente? | stato runtime di bordo/oggetto |
| Transition | quali celle e layer sono connessi? | grafo runtime |
| Mesh | cosa vede l'umano? | sola presentazione |

---

## 10. Runtime e Editor: il confine

| Runtime possiede | Editor possiede |
|---|---|
| coordinate · grammatica · validazione · occupancy · classificazione · bake · pathfinding · reachability · LOS · costi · reason code · hash/revision · dati serializzati | input mouse · stato dei tool · ghost · snap · highlight · overlay · pannelli · transazioni Undo/Redo · visualizzazione |

```text
Editor chiama Runtime.  Runtime non dipende da Editor.
```

Nessuna regola competitiva in `Source/RefactorTacticsEditor/`: quel modulo **non ha test**, e ciò che nasce lì
nasce non verificabile. È la ragione per cui `URTHexOccupancyLibrary` vive nel modulo runtime pur servendo
soprattutto l'editor.

### 10.1 La vista non è il dato

```text
dato → RebuildInstances → visualizzazione
```

Mai il contrario. La geometria di lettura è **transient**, si rigenera dal dato, non si salva nel `.umap`
come seconda verità, e **non ruba i click**: solo il componente delle celle destinato al picking ha la
collisione prevista. L'invariante è
`RefactorTactics.HexMap.OnlyTheCellsComponentIsClickable`, e vale anche per la geometria che verrà.

> Una vista che mente costa più di una vista che manca: per un'ora la mappa ha mostrato 61 esagoni mentre il
> criterio d'arena diceva «non ha celle». L'invariante gemella è
> `RefactorTactics.Arena.CriterionAndOverlayCountTheSameCells`.

### 10.2 Vocabolario visuale

```text
FORMA  = cosa fa la cella      (parete piena, lastra bassa, pannello di bordo, rilievo, freccia)
COLORE = che superficie è      (terreno)
```

Due canali indipendenti, così una cella dice due cose insieme. L'editor può usare colori aggressivi: è uno
strumento per il designer, **non** la leggibilità in partita, che è `E21`.

---

## 11. Determinismo

Tutto ciò che influenza il gameplay è intero, enum o ID stabile; indipendente dall'ordine delle collection;
hashabile; riproducibile fra macchine.

Il mondo Unreal usa `FVector` per **disegnare**. La decisione tattica **serializzata** non dipende
dall'arrotondamento di coordinate float.

`Revision` e `ComputeHash()` lavorano insieme allo snapshot di simulazione: `FRTHexSimSnapshot` cattura
entrambi e si dichiara obsoleto se uno cambia. È il meccanismo che impedisce a un turno di essere risolto
contro una mappa diversa da quella su cui era stato pianificato.

---

## 12. Decisioni aperte

Nessuna di queste si decide in un commit di implementazione. Vivono in
[`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).

| ID | Domanda | Innesco |
|---|---|---|
| ~~`MSE-1`~~ | ✅ **Chiusa da `D-131`**: `FRTHexCover` acquista `bGenerated`, il rebake tocca solo le proprie — vedi §8.2 | — |
| `MSE-4` | Un settore toccato in un **solo punto** va contato come occupato, o serve un'intersezione di lunghezza non nulla? | 🔴 **INNESCATA il 2026-08-30, e non dal footprint** — [#1826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1826). L'innesco è arrivato dal primo **consumatore di posa**: il **centro** della cella è il vertice comune di *tutti e dodici* i triangoli, quindi ogni segmento con `Offset == 0` accende dodici bit su dodici e lascia la cella senza alcuna regione di posa. È l'istanza più severa della stessa classe — 12 triangoli invece di 4, 8 settori di sovrastima invece di 2, e la conseguenza è **cella inagibile** invece di «cella più stretta». Misurata da `RefactorTactics.CoverPlacement.CentreContactRuleStillCollapsesTheWholeCell` |
| ~~`MSE-2`~~ | ✅ **Sciolta da `D-125`**: misurava i **muri**, che non alimentano l'occupancy — vedi §5.1 | — |
| ~~`MSE-3`~~ | ✅ **Chiusa da `D-125`**: i due modelli misurano la stessa cosa a due granularità | — |

> 🔑 **`MSE-3` non era un conflitto.** Il cerchio inscritto di `D-071` chiede *«ci sta un'unità?»* — binario —
> e i dodici settori chiedono *«e quanto ci sta stretta?»* — ternario. Il secondo **raffina** il primo.
> La domanda «quale dei due scrive `bBlocksMovement`» aveva una premessa falsa: nessuno dei due lo scrive per
> i **muri**, che sono bordi; entrambi lo fanno per il **volume**, e concordano perché misurano la stessa cosa.
>
> 🔁 **Aggiornato il 2026-08-31: `D-071` punto (1) È ora superseded**, da
> [`D-303`](../../decisions/RT_PDR_00_Decision_Log.md) — il cerchio inscritto **centrato sull'anchor**
> presuppone che l'unità stia al centro, e il modello a regioni di posa rimuove quella premessa. ∴ la
> riga qui sotto descrive lo stato **fino al 2026-08-30**: il cerchio non è più il gate binario della
> calpestabilità, che è *«esiste una regione compatibile col profilo»*
> ([`spec-cover-placement-intra-hex.md`](spec-cover-placement-intra-hex.md) §13.0). ⚠️ **`MSE-3` non si
> riapre**: la relazione *«il secondo raffina il primo»* valeva, e ciò che è cambiato è che il
> raffinamento ha **sostituito** il misurando invece di affiancarlo. Il punto (2) di `D-071` — la *swept
> clearance* — resta in piedi.
>
> 🔧 **La precisazione storica di `D-125`, che resta vera per ciò che diceva**: *«non tocca»* si legge *«non vi entra»*. Un muro
> appoggiato al lato dell'esagono è **esattamente tangente** al cerchio inscritto (misurato: `86.602540`
> contro un'apotema di `86.602540`, differenza **zero**), e alla lettera avrebbe reso non calpestabile ogni
> cella addossata a una parete.

**Vincolo che vale già**: non salvare la geometria tattica come mesh autorevole nel `.umap`, e non introdurre
una seconda authority mentre `MSE-1` è aperta.

---

## 13. Gli strumenti che consumano questo modello

Ognuno ha la sua issue; qui c'è solo il **contratto** che devono rispettare.

| Strumento | Contratto | Issue |
|---|---|---|
| **Workspace Grid** | mostra *dove potresti creare celle*, non che esistano. Ghost, transient, nessun Actor per cella, nessun salvataggio nel `.umap`, nessun click ambiguo | [#622](https://github.com/DegrassiAaron/refactor-tactics-main/issues/622) |
| **Geometry Authoring Tool** | ghost valido/invalido prima del commit, snap alla grammatica, **una gesture = una transazione** (un solo `Ctrl+Z`), validator e bake chiamati al runtime | [#712](https://github.com/DegrassiAaron/refactor-tactics-main/issues/712) |
| **Movement Probe** | usa `URTHexSimLibrary::ReachableCells` e ricostruisce il path risalendo `FRTHexReachableCell::FromCell`. **Nessun secondo Dijkstra, nessun A\* per cella.** I `reason` usano il vocabolario runtime esistente | [#711](https://github.com/DegrassiAaron/refactor-tactics-main/issues/711) |

### 13.1 Che cosa muore con che cosa — le regole di dipendenza

Owner: `URTMapDependencyLibrary` (`Source/RefactorTactics/Map/`), [#1864](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1864).

La regola sta nel **runtime** ed è **pura**, per la stessa ragione per cui la grammatica non sta nell'editor:
è del dominio, non dello strumento, e deve poter essere provata headless. Un tool chiede l'elenco dei
dipendenti *prima* di aprire la transazione, e cancella dentro la propria.

⛔ **`CollectDependents` non modifica l'asset.** Raccogliere e cancellare sono due gesti distinti: tenerli
separati è ciò che permette a un tool di mostrare l'elenco prima di eseguirlo, e alla regola di essere
testabile senza un mondo.

Cancellando una **cella**, tre array le sopravvivono e vanno raccolti — `Covers` e `Doors` no, perché vivono
*dentro* `FRTHexCellData` e se ne vanno con lei:

| Array | Perché non può restare | Regola di `ValidateMap` |
|---|---|---|
| `InteriorWalls` | un muro su una cella che non esiste | *«muro interno %d su cella inesistente»* |
| `Transitions` | **entrambi i versi**: `FRTHexEdge` è direzionale | *«transizione verso cella inesistente»* |
| `InteractionBindings` | un binding la cui sorgente sparisce | *«riferimento a una struttura inesistente»* |

🔑 **Portare un bordo di una struttura non significa esserne l'unica sede.** Un portone è un *gruppo* di
bordi che condividono lo `StableId` (CP 23.3): cancellare una delle sue celle ne toglie metà, e il nome
continua a risolvere. Un binding che lo comanda **sopravvive**, e rimuoverlo sarebbe una correzione
silenziosa di uno stato ancora valido — con perdita di dato. Il nome muore solo se *nessun* bordo resta
fuori dalla cella cancellata.

⚠️ Gli indici restituiti valgono finché l'asset non cambia, e si consumano **dal più alto al più basso**.

Verifica: `RefactorTactics.Map.Dependency.*` — un test per array, uno per il gruppo che sopravvive, e uno
che applica la cascata e chiede a `ValidateMap` se è rimasto qualcosa.

### 13.2 Identità di un elemento, e il move

Owner: `URTMapEditLibrary` (`Source/RefactorTactics/Map/`), [#1864](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1864).

Un elemento autorato si nomina con un **handle**, che è **dato** e non oggetto: nessun puntatore, nessun
Actor. Come lo si identifica dipende da cosa può succedergli.

| Elemento | Identità | Perché |
|---|---|---|
| cella | `FRTCellId` | già stabile per costruzione |
| copertura | `(Cell, Edge)` | **una sola copertura per bordo** — regola già applicata da `ValidateMap` |
| porta · transizione | `StableId` | CP 23.3, #832 |
| **muro interno** | `StableId` (**v12**) | 🔑 **si sposta**, e il move cambia `(Cell, Segment)` |

🔑 **La chiave naturale del muro interno è ciò che il move modifica.** Un handle derivato si romperebbe
esattamente durante l'operazione a cui deve sopravvivere: è per questo, e non per simmetria con le porte,
che `FRTHexInteriorWall` prende un campo e `FRTHexCover` no.

⛔ **`FRTHexInteriorWall::StableId` non entra in `ComputeHash`** — l'intero array ne resta fuori, perché
vista e passo non consultano un muro interno. Qui il criterio **diverge** da `FRTHexDoor::StableId`, che
nell'hash ci entra (#986): un nome di porta lo si risolve a runtime, un nome di muro solo nell'editor.

**Il move valida prima di scrivere**, e un rifiuto è un valore di ritorno con la sua ragione
(`ERTMapEditOutcome`) — mai una correzione silenziosa, mai uno stato scritto e poi segnalato dal validator:

```text
RefusedUnresolved      l'handle non nomina niente
RefusedNoSuchCell      la destinazione non esiste
RefusedOutOfGrammar    ValidateSegment decide, il move la chiama
RefusedWouldCloseEdge  chiuderebbe un bordo: allora e' una COPERTURA
RefusedDuplicate       muro identico gia' presente
```

Verifica: `RefactorTactics.Map.Edit.*` — l'handle sopravvive al move, il round-trip di serializzazione, e i
quattro rifiuti, ciascuno con la controprova che la mappa resta valida.

⚠️ **Un muro senza nome resta identificabile**, e non è un ripensamento su v12: `StableId` nasce `NAME_None`,
quindi ogni muro disegnato prima di v12 è anonimo. L'handle porta allora la chiave `(Cell, Segment)` — unica
per una regola che `ValidateMap` già applica. Il nome, quando c'è, **vince**: è l'unico che sopravvive al
move. ⛔ Un nome che non risolve **non** ricade sulla chiave: chi ha chiesto quella struttura vuole quella, e
restituirne un'altra perché sta nello stesso posto sarebbe un errore silenzioso.

### 13.3 Che cosa c'è sotto un punto, e il ciclo di selezione

`URTMapEditLibrary::ElementsAt` (runtime) · `URTHexSelectionStore` (editor) · [#1864](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1864).

`ValidateMap` **permette** una porta e una copertura `Low` sullo stesso bordo — vieta solo la coppia con
`High`. Due elementi selezionabili nello stesso punto sono quindi uno stato che l'autoraggio produce e il
validatore approva, non un caso limite.

L'ordine dei candidati è **contratto, non dettaglio**, perché un click ripetuto ci scorre sopra:

```text
Door  ->  Cover  ->  InteriorWall(i della cella)  ->  Cell
```

Il ciclo vive nello store: stesso punto → si avanza; punto nuovo → si riparte dal più specifico. ⚠️ Senza il
confronto col punto precedente l'indice sarebbe un contatore globale, e il primo click su una cella nuova
prenderebbe un elemento a caso a seconda dei click fatti altrove.

⛔ **Le transizioni non compaiono fra i candidati, ed è dichiarato**: un arco collega celle su layer diversi
e non giace su un bordo, quindi «cosa c'è sotto questo bordo» non lo raggiunge. Il suo hit-test è di
viewport, e appartiene al tool.

🔴 **La selezione vive fuori dai `UInteractiveToolPropertySet`**, ed è il punto: [#921](https://github.com/DegrassiAaron/refactor-tactics-main/issues/921)
ha misurato il difetto opposto — `bShowOverlay` vive in due property set distinti, quindi accenderlo in
Select non lo accende in Paint e cambiando strumento si perde. **Uno stato che deve sopravvivere al cambio di
tool non può stare dentro il tool.** Un `UEditorSubsystem` sopravvive ai tool e al mode, non è un Actor, e
non tocca l'asset: la selezione è stato d'editor puro e non si serializza.

Verifica: `RefactorTactics.Editor.Selection.*` — il ciclo che ricomincia, il reset cliccando altrove, e
l'aggiunta che accumula senza duplicati.

---

## 14. Come si verifica

| Va in automation headless | Va in seduta PIE |
|---|---|
| grammatica · quantizzazione · maschera · classificazione · bake · mapping dei bordi · costi · path e reachability · ordinamento deterministico · hash e revisione · serializzazione e migrazione | ghost leggibile · snap percepibile · workspace ≠ cella vera · Undo/Redo visuale · stati di porta leggibili · visibilità delle transizioni · composizione degli overlay · selezione col mouse · focus di layer |

**Le fixture geometriche non sono scenari.** Segmento, angolo, footprint solido e footprint void sono
l'ingresso di una funzione pura e vivono in
[`RTOccupancyFixtures.h`](../../../Source/RefactorTactics/Tests/RTOccupancyFixtures.h); uno scenario JSON esiste
solo per dimostrare un comportamento **di partita** — per esempio che un'unità non attraversa un muro cotto.

Non estendere lo schema dello Scenario Harness per infilarci input d'editor.

Le verifiche PIE stanno in [`test-manuali-pie.md`](../test-manuali-pie.md) e devono comparire in una seduta di
[`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml): una voce PIE senza seduta rischia di non essere
mai eseguita.

---

## 15. Errori che questo modello previene

| ❌ | Perché è un errore |
|---|---|
| Dodici settori letti come dodici direzioni | le direzioni restano **sei**: §2 |
| `North` / `South` aggiunti a `ERTHexDirection` | in pointy-top non esistono lati N/S: §2.1 |
| Muro obbligato a stare sul lato di una cella | §3 |
| Endpoint float arbitrario nella struttura autoritativa | §3.1, §11 |
| `LOW WALL` separato da `FRTHexCover{Low}` | due rappresentazioni dello stesso oggetto: §8.1 |
| Mesh interrogata dal runtime per passaggio o LOS | §8 |
| Geometria salvata nel `.umap` come seconda authority | §10.1, §12 |
| Secondo A\* o Dijkstra nel modulo Editor | §13 |
| Secondo calcolo del punto medio del bordo | §4 |
| `MoveCost` sovrascritto col surcharge geometrico | §7 |
| Regole competitive in `Source/RefactorTacticsEditor/` | §10 |
| Scenario JSON usato per una verifica visuale d'editor | §14 |
