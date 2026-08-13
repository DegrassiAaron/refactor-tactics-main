# Spec — Geometria architettonica, occupancy e cottura verso i dati tattici

> `CURRENT` · **Stato**: owner del modello, allineato al codice il **2026-08-13** · **Owner**: questo file
> **Autorità**: subordinata a [`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md),
> [ADR-0002](../decisions/adr-0002-griglia-esagonale.md) e al
> [Decision Log](../decisions/RT_PDR_00_Decision_Log.md).
> **Nato da**: [revisione spec panel del 2026-08-13](../roadmap/plans/hexgeometry-editor-spec-panel-2026-08-13.md),
> che ha verificato l'assenza di un owner per questo tema — viveva solo in referti, archivio e registry.

Questo documento risponde a **una** domanda: *cosa succede fra il gesto con cui un designer disegna un muro e
il dato che il resolver legge per dire «non si passa»?*

> ⚠️ **Non è un tracker.** Lo stato di implementazione vive nel
> [`feature-registry.yaml`](../roadmap/feature-registry.yaml) e nelle issue. Qui c'è il **modello**: cosa
> significano le cose e perché sono separate. Se una riga di questo file dichiara uno stato, è un difetto.

---

## 1. Cosa questo documento non possiede

| Tema | Owner |
|---|---|
| Coordinate, `FRTCellId`, transizioni fra layer, formato dell'asset | [`spec-mappa-multilivello.md`](spec-mappa-multilivello.md) |
| A\*, archi percorribili, costi del cammino | [`spec-pathfinding-pf3-pf4.md`](spec-pathfinding-pf3-pf4.md) |
| Regole di copertura, riduzione danno, distruzione | [`../gameplay/spec-copertura-cp91.md`](../gameplay/spec-copertura-cp91.md) · [`../gameplay/spec-copertura-alta-cp92.md`](../gameplay/spec-copertura-alta-cp92.md) |
| Superfici, stati, propagazione | [`../gameplay/spec-terreni-e8.md`](../gameplay/spec-terreni-e8.md) |
| LOS e forme di targeting | [`h6-4-hex-vision-spec.md`](h6-4-hex-vision-spec.md) |
| Visualizzazione in editor: cosa serve vedere e perché | [`brief-editor-map-viz.md`](brief-editor-map-viz.md) |
| Stato di avanzamento | [`../roadmap/feature-registry.yaml`](../roadmap/feature-registry.yaml) |

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
> Vedi [`spec-mappa-multilivello.md`](spec-mappa-multilivello.md) §4.

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
> [referto](../roadmap/plans/level-designer-handoff-spec-panel-2026-08-12.md) §6.
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
> (`RTHexOccupancyLibrary.h:115-127`), cioè **già pronto a essere salvato**: in quella lettura la voce di DoD
> di `#620` *«la grammatica è espressa in interi o enum»* era **insoddisfacibile**, e il difetto sarebbe
> emerso al primo salvataggio. Resta vero il motivo per cui il tipo fu dichiarato in anticipo — evitare che il
> primo commit ne inventasse uno — solo che quel tipo è il **derivato**, non la sorgente.

`MSE-1` ne esce **ristretta**: decide *dove* vive il source editabile e chi vince al rebake, non più *in che
tipo*.

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

**Stato**: la grammatica, il suo tipo e il suo validator sono
[#620](https://github.com/DegrassiAaron/refactor-tactics-main/issues/620), **aperta**.

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

## 6. Free, Constrained, Blocked

| Settori occupati | Classificazione |
|---|---|
| `0 – 3` | `Free` |
| `≥ ConstrainedFrom` (4) | `Constrained` |
| `≥ BlockedFrom` (6) | `Blocked` |
| `bCoreBlocked` | `Blocked`, comunque siano i settori |

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
| footprint solido | `bBlocksMovement`, secondo la classificazione di §6 |
| footprint void / precipizio | `bBlocksMovement = true` **+** `bBlocksLineOfSight = false` |

> 🔎 **Il bake NON scrive `Surface`.** La proposta `void/cliff → ERTHexSurface::Void` è stata valutata e
> **respinta** il 2026-08-12. `Void` sarebbe una superficie *dipinta* fra nove valori, e nessuna regola
> geometrica sa scegliere fra nove; soprattutto, `Fill` propaga sulla **contiguità di superficie**, quindi una
> `Surface` cotta cambierebbe il confine di ogni futuro flood fill — un effetto sullo **strumento**, non sul
> dato. La coppia `bBlocksMovement` + `!bBlocksLineOfSight` dice già «non ci si sta sopra, ma ci si vede
> attraverso», e distingue un precipizio da un muro.
> [Decisione](../OPEN_DECISIONS.md) · [referto](../roadmap/plans/level-designer-handoff-spec-panel-2026-08-12.md).

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
[`OPEN_DECISIONS.md`](../OPEN_DECISIONS.md).

| ID | Domanda | Innesco |
|---|---|---|
| `MSE-1` | Dove vive il **source editabile** della geometria d'authoring, e chi vince se un dato cotto viene modificato a mano e poi si rifà il bake? — ⚠️ **ristretta da `D-127`**: *in che tipo* non è più parte della domanda | [#621](https://github.com/DegrassiAaron/refactor-tactics-main/issues/621) |
| `MSE-4` | Un settore toccato in un **solo punto** dal bordo di un footprint va contato come occupato, o serve un'intersezione di lunghezza non nulla? | [#621](https://github.com/DegrassiAaron/refactor-tactics-main/issues/621) |
| ~~`MSE-2`~~ | ✅ **Sciolta da `D-125`**: misurava i **muri**, che non alimentano l'occupancy — vedi §5.1 | — |
| ~~`MSE-3`~~ | ✅ **Chiusa da `D-125`**: i due modelli misurano la stessa cosa a due granularità | — |

> 🔑 **`MSE-3` non era un conflitto.** Il cerchio inscritto di `D-071` chiede *«ci sta un'unità?»* — binario —
> e i dodici settori chiedono *«e quanto ci sta stretta?»* — ternario. Il secondo **raffina** il primo.
> La domanda «quale dei due scrive `bBlocksMovement`» aveva una premessa falsa: nessuno dei due lo scrive per
> i **muri**, che sono bordi; entrambi lo fanno per il **volume**, e concordano perché misurano la stessa cosa.
>
> 🔧 **`D-071` acquista una parola, e non è superseded**: *«non tocca»* si legge *«non vi entra»*. Un muro
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

---

## 14. Come si verifica

| Va in automation headless | Va in seduta PIE |
|---|---|
| grammatica · quantizzazione · maschera · classificazione · bake · mapping dei bordi · costi · path e reachability · ordinamento deterministico · hash e revisione · serializzazione e migrazione | ghost leggibile · snap percepibile · workspace ≠ cella vera · Undo/Redo visuale · stati di porta leggibili · visibilità delle transizioni · composizione degli overlay · selezione col mouse · focus di layer |

**Le fixture geometriche non sono scenari.** Segmento, angolo, footprint solido e footprint void sono
l'ingresso di una funzione pura e vivono in
[`RTOccupancyFixtures.h`](../../Source/RefactorTactics/Tests/RTOccupancyFixtures.h); uno scenario JSON esiste
solo per dimostrare un comportamento **di partita** — per esempio che un'unità non attraversa un muro cotto.

Non estendere lo schema dello Scenario Harness per infilarci input d'editor.

Le verifiche PIE stanno in [`test-manuali-pie.md`](test-manuali-pie.md) e devono comparire in una seduta di
[`editor-sessions.yaml`](../roadmap/editor-sessions.yaml): una voce PIE senza seduta rischia di non essere
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
