# Spec — Graybox Kit: contratto di ingombro, pivot e presentazione degli oggetti di mappa

> `CURRENT` · **Creato**: 2026-08-17 · **Owner**: questo file — è il contratto che dice **quanto spazio
> occupa** un asset di mappa, **dove sta il suo pivot**, e **come si legge il suo stato** senza chiedere
> nulla alla simulazione.
>
> **Autorità**: subordinata a [`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) e al
> [Decision Log](../decisions/RT_PDR_00_Decision_Log.md) — `D-152` e `D-153` sono le decisioni che questo
> documento esegue, `D-146` è quella che estende.
>
> **Nato da** un handoff d'autore del 2026-08-17 (`Graybox_Kit_Cover_CellVolume`, 1620 righe), archiviato
> in [`../archive/src/README.md`](../archive/src/README.md). Il kit è stato **filtrato** contro `main`, non
> applicato: tre delle sue prescrizioni descrivono un repository che non esiste, e sono elencate in §9.

---

## 1. Cosa questo documento non possiede

È la prima sezione perché il difetto più caro di quest'area è il **secondo owner**: un contratto di ingombro
somiglia a un modello di traversabilità, e chi confonde i due si ritrova con due sorgenti per lo stesso
numero.

| Tema | Owner |
|---|---|
| Coordinate, `FRTCellId`, transizioni fra layer, formato dell'asset | [`spec-mappa-multilivello.md`](spec-mappa-multilivello.md) |
| Geometria **tattica**: asse, offset interi, occupancy, cottura | [`spec-hex-geometry-authoring.md`](spec-hex-geometry-authoring.md) |
| **Standability**: footprint e clearance che decidono dove si sta in piedi | `RT-FEAT-MAP-STANDABILITY` — E23 / CP 23.6 |
| Regole di copertura, riduzione danno, distruzione | [`../gameplay/spec-copertura-cp91.md`](../gameplay/spec-copertura-cp91.md) · [`../gameplay/spec-copertura-alta-cp92.md`](../gameplay/spec-copertura-alta-cp92.md) |
| Verbi e stati degli elementi interattivi | [`../gameplay/spec-interazioni-mappa-cp101.md`](../gameplay/spec-interazioni-mappa-cp101.md) |
| Grammatica visiva delle **celle** (colore + forma) | `D-146` · `RT-FEAT-UI-BOARD-GRAMMAR` |
| Percorsi e naming dentro `Content/` | [`convenzioni-contenuti-ue.md`](convenzioni-contenuti-ue.md) |
| Quali asset esistono e quanti mancano | [`asset-map.md`](asset-map.md) |
| Principi di pipeline: presentazione-only, riferimenti soft, licenze | [`spec-asset-pipeline.md`](spec-asset-pipeline.md) |
| Stato di avanzamento | [`../roadmap/feature-registry.yaml`](../roadmap/feature-registry.yaml) |

> ⚠️ **Non è un tracker.** Se una riga di questo file dichiara uno stato di implementazione, è un difetto.

### 1.1 La distinzione che regge tutto: Safe Placement Volume ≠ clearance

Sono due numeri che si somigliano e rispondono a domande diverse. Confonderli creerebbe esattamente il
secondo owner che §1 esiste per evitare.

```text
SAFE PLACEMENT VOLUME    quanto grande posso MODELLARE un asset
                         → contratto d'authoring, questo documento, EditorOnly

CLEARANCE                se un'unità ci STA IN PIEDI
                         → dato cotto, CP 23.6, authority della simulazione
```

Un asset che rispetta il Safe Placement Volume **non** è per ciò stesso attraversabile, e uno che lo viola
non blocca niente. Il primo è una regola per chi modella, il secondo è una regola per chi risolve il turno.

---

## 2. Il principio: la mesh non è authority — e non è nuovo

Questa riga esiste per essere citata, non per essere decisa qui: è **già** il modello del repository, e
l'handoff che ha generato questo documento chiedeva di «inserire o consolidare esplicitamente» un principio
che era già vigente in tre punti indipendenti.

```text
il gameplay legge     MapState · coperture di bordo · stato porta ·
                      superficie · occupancy cotta · interaction state

il gameplay NON legge  «se la mesh è lì allora blocca»
```

Le prove, misurate e non citate a memoria:

- `RefactorTactics.HexMapActor.BlockerVolumesComeFromCellFlags` — i volumi bloccanti **discendono dai flag
  della cella**, non viceversa;
- `D-129` toglie il volume dal bake, e nel farlo ne cambia il soggetto: non più `bBlocksMovement` ma
  `FRTHexCover`;
- l'invariante **#1** delle convenzioni di contenuto: *«le regole decidono l'esito, gli asset no»*.

**La presentazione segue lo stato logico, non lo crea.** Un asset graybox che imponesse una regola sarebbe un
difetto di architettura, non una scorciatoia.

---

## 3. Placement taxonomy

Quattro classi, e una quinta dichiarata assente. Non è un `enum` e non entra in nessun dato serializzato:
è il **vocabolario** con cui si dice dove un asset si aggancia. `D-152`.

| Classe | Si aggancia a | Rispetta il Safe Volume | Esempi reali su `main` |
|---|---|:--:|---|
| **`CellBound`** | il centro della cella (`CellWorldAnchor`) | **sì** | unità, obiettivo del Relè, proxy di elemento interattivo |
| **`EdgeBound`** | uno dei sei bordi | **no**, e non è un'eccezione | copertura `Low`/`High`, porta, muro |
| **`SurfaceBound`** | la superficie della cella | non applicabile | `ShallowWater`, `Ice`, `Fire`, `Smoke` |
| **`EditorOnly`** | niente: è una guida | non applicabile | Cell Placement Volume, marker di spawn, ancore di debug |
| ~~`MultiCell`~~ | più celle | — | **differito**: nessun contenuto della v0.1 lo richiede |

> 🔑 **`EdgeBound` non è un caso particolare di `CellBound`, ed è il punto che il kit aveva quasi ragione a
> insistere.** Il bordo `E` di `A` **è** il bordo `W` del suo vicino — una primitiva sola, non due
> (`spec-hex-geometry-authoring.md` §4, test `Hex.EdgeMidpointIsSharedByBothCells`). Forzare una copertura
> dentro il footprint di una delle due celle la farebbe appartenere a quella cella, che è falso: non
> appartiene a nessuna delle due, appartiene al bordo.

**`MultiCell` è differito e non «previsto»**: la differenza conta. Il repository non ha nessun asset che
occupi più celle, e la prima cosa che ne avrebbe bisogno — payload, macchinari — vive in epic che nessuna
release corrente possiede. Introdurre la classe ora significherebbe scrivere il vocabolario di un sistema
che non ha un consumatore, ed è la forma di debito che `D-146` ha già rifiutato per le categorie di cella.

---

## 4. Pivot contract

```text
CellBound     pivot = bottom-center del footprint
              obiettivo: ActorLocation == CellWorldAnchor, senza offset compensativi

EdgeBound     pivot = centro del segmento, alla base
              orientamento: quello che URTHexLibrary::EdgeRotation restituisce già
```

Per `EdgeBound` gli assi **non si incidono nel codice né nella mesh**: si chiedono a
`URTHexLibrary::EdgeMidpointWorld` e `EdgeRotation`. È la regola che
[`spec-hex-geometry-authoring.md`](spec-hex-geometry-authoring.md) §4 già scrive per la geometria — *«non
incidere angoli nel codice: se la convenzione dei sei lati cambiasse, la geometria derivata la segue, quella
incisa mente in silenzio»* — e vale identica per gli asset che ci si appoggiano.

### 4.1 Le parti mobili non spostano l'identità logica

```text
Door Actor            ← l'anchor logico sta QUI e non si muove
 ├─ Frame
 └─ Panel             ← la parte che ruota o trasla
      └─ hinge pivot
```

Una porta che si apre **non cambia bordo**. Se ad animarla si spostasse l'Actor, l'identità della porta
diventerebbe funzione del suo stato — e `FRTHexDoor` la dichiara sul bordo, che è immobile per costruzione.

### 4.2 Il precedente costoso, ed è già stato pagato

Il cilindro segnaposto di `ARTUnit` **era** il root e portava una scala non uniforme `(1.2, 1.2, 1.8)`:
ogni componente aggiunto in Blueprint la ereditava, e una Skeletal Mesh veniva stirata di `1.5x`. La
correzione — root neutro `USceneComponent` — è `#593`, chiusa il 2026-08-16.

**È esattamente il difetto che questa sezione previene**, ed è la ragione per cui il pivot è un contratto e
non una preferenza: un pivot sbagliato non fallisce, deforma in silenzio tutto ciò che gli si attacca sotto.

---

## 5. Cell Placement Volume

Una guida d'authoring, non un Actor autorevole. `D-152`.

```text
prisma esagonale derivato dalla cella logica
  ├─ outer footprint   il 100% della cella
  ├─ safe footprint    un inset, misurato in frazione di C — vedi §9, il valore è APERTO
  └─ guide verticali   riferimenti d'altezza per modellare
```

**Che cosa serve a fare**: modellare asset proporzionati, verificare che un `CellBound` non invada la cella
adiacente, dare una silhouette coerente, e diventare il contratto dimensionale che l'arte finale dovrà
rispettare quando sostituirà il graybox.

**Che cosa non è autorizzato a fare**: comparire in partita, entrare in una collisione, essere letto da una
regola. È `EditorOnly` nel senso stretto — se comparisse in una build packaged sarebbe un difetto, non una
funzionalità di debug.

> 🔴 **Il volume non produce clearance, e il numero dell'inset non è il numero di CP 23.6.** Vedi §1.1. Se
> un giorno la cottura di 23.6 avesse bisogno di un inset, lo prenderebbe dal proprio dato: leggerlo da una
> guida d'editor significherebbe far dipendere la simulazione da un asset, che è §2 al contrario.

---

## 6. Dimension grammar — relativa, mai in centimetri

Le misure si esprimono in frazioni di **`C`**, la distanza centro-centro fra due celle adiacenti.

`C` **non è una costante nuova**, e **non è `HexSize`**:

```text
C = √3 · HexSize          ≈ 173 con HexSize al suo default di 100
```

`HexSize` è il **raggio** dell'esagono (circumraggio), non il passo della griglia: `URTHexLibrary::AxialToWorld`
calcola `Wx = HexSize · √3 · (Q + R/2)`, quindi due celle adiacenti distano `√3 · HexSize`. `HexSize` è un
`UPROPERTY` dell'asset mappa e dell'attore, e una mappa può cambiarlo — per questo le misure di questo
documento sono in frazioni di `C` e non in centimetri.

> 🔴 **Questa riga diceva «`C` discende da `HexSize`, default `100.f`» senza il fattore, ed era un errore
> di 1,73×.** Chi avesse letto «0.28 C» come 28 cm avrebbe modellato una copertura bassa **alta un terzo**
> del dovuto — e lo stesso valeva per il Safe Placement inset di `GBX-1`, espresso nella stessa unità.
> Trovato in code review; la formula costa una riga e toglie l'intera classe di errore.

Le guide verticali del volume sono **riferimenti di modellazione**, e non sono categorie di targeting:

```text
1.00 C   guida massima
0.85 C   strutturale
0.55 C   standard
0.28 C   bassa
0.00 C   piano d'appoggio
```

> ⚠️ **Nessuna di queste soglie decide una regola.** Se il gameplay avesse bisogno di classi d'altezza, il
> suo owner sarebbe la copertura (`ERTHexCoverType`, due valori più `None`) — e quel dato **esiste già**,
> quindi una guida visuale che ne inventasse un terzo creerebbe la divergenza descritta in §1.

---

## 7. Grammatica visiva degli oggetti — estende `D-146`, non la duplica

`D-146` ha già deciso la regola per le **celle**: *«l'encoding è ridondante: mai solo il colore»*, ogni
categoria distinguibile da almeno **due** canali indipendenti. Questa sezione la applica agli **oggetti**,
dove il secondo canale è la **geometria**. `D-152`.

```text
GEOMETRIA        che cosa è: silhouette, ingombro, famiglia
COLORE / ACCENT  stato o famiglia funzionale — mai da solo
TRASFORMAZIONE   lo stato meccanico reale (aperto, ruotato, collassato)
```

### 7.1 Gli stati meccanici si vedono nella geometria

Il repository ha **quattro** stati di porta, non tre:

| `ERTHexDoorState` | Che cosa deve mostrare |
|---|---|
| `Open` | pannello ruotato o spostato fuori dal passaggio |
| `Closed` | pannello nel passaggio |
| `Locked` | pannello nel passaggio **più** un marcatore proprio |
| `Destroyed` | pannello rimosso — stato terminale, non richiudibile |

> 🔴 **`Locked` è il caso che il kit aveva dimenticato, ed è quello che rompe la regola.** `Closed` e
> `Locked` **negano entrambi il passaggio** e sono geometricamente identici: la sola differenza è che il
> secondo non si apre. Se l'unico canale che li distingue fosse il colore, `D-146` sarebbe violata dal
> primo asset che il kit produce. Serve un secondo canale non cromatico, e **quale** è una domanda aperta
> (§9) — non una che si chiude modellando.

### 7.2 Integrità: il dato è un intero, gli stati sono una lettura

`FRTHexCover::Integrity` è un `int32` (catalogo v0.1: **30** per la copertura bassa), **e ha già un
produttore vivo**: `URTHexCoverLibrary::ApplyStructureDamage` → `DamageFace` scala l'integrità sulle
**due facce** della barriera e produce `FRTCoverDamageResult{RemainingIntegrity, bDestroyed}`, che entra
nel TurnLog. `RefactorTactics.EnvironmentAction` pinna i valori intermedi — `Integrity == 20` dopo un
colpo, su entrambi i versi.

> 🔴 **Questa sezione diceva «la distruzione arriva con CP 9.2, che lo scalerà», ed era falso: CP 9.2 è
> chiuso.** Il difetto non è il numero, è il metodo — la frase è la trascrizione del commento sopra il
> campo in `RTHexCellData.h`, scritto quando il produttore non c'era e mai aggiornato. È esattamente ciò
> che §2 di questo file dichiara di non fare (*«le prove, misurate e non citate a memoria»*), commesso
> nella sezione successiva. Trovato in code review.

Ne discende una separazione che va scritta prima di modellare:

```text
Intatto      geometria piena, corpo neutro
Danneggiato  stessa geometria + marcatore geometrico
Critico      stessa geometria + marcatore più forte
Distrutto    geometria CAMBIATA — non è lo stesso oggetto ricolorato
```

⚠️ **Le soglie che separano danneggiato da critico non esistono**, e non si inventano qui: sono numeri di
presentazione su una scala che appartiene al balance della copertura, non a chi modella. Ma la ragione
**non** è più «manca il produttore» — il produttore c'è, con una scala osservabile `30 → 20 → 0` e un esito
`bDestroyed` terminale. È `GBX-3`, §9, e ha abbastanza materiale per essere decisa **adesso**.

### 7.3 «Acceso/spento» non basta, e il numero di stati non lo fissa questo documento

Il kit propone `Online/Offline` come «accent acceso/spento». È troppo poco, e l'owner degli elementi
interattivi lo mostra con un esempio di macchina a stati
([`../gameplay/spec-interazioni-mappa-cp101.md`](../gameplay/spec-interazioni-mappa-cp101.md) §5) in cui un
generatore ne ha cinque — `Off · Online · Overloaded · Damaged · Destroyed`. `Overloaded` e `Damaged` sono
due modi diversi di non funzionare, con transizioni d'uscita diverse (`Stabilize`/`Disconnect` contro
`Repair`): una grammatica a due stati li fonderebbe, e il giocatore perderebbe l'informazione che decide
l'azione successiva.

> ⚠️ **Quel blocco è etichettato «Esempio, non catalogo», e questo documento non lo promuove a norma.**
> §2 di quell'owner dice che **ogni elemento dichiara i propri stati**: il primo con quattro o sei stati
> non violerebbe niente. La regola che questo contratto impone è un'altra e vale per qualunque cardinalità:
> **ogni stato dichiarato da un elemento dev'essere distinguibile su due canali**, e gli stati che si
> somigliano funzionalmente — due modi di essere rotto, due modi di essere chiuso — hanno bisogno di un
> canale **non cromatico** che li separi.
>
> 🔴 La prima stesura scriveva *«l'owner ne dichiara cinque»* e fissava una tassonomia che il suo
> proprietario aveva deliberatamente lasciato aperta — il secondo owner che §1 esiste per prevenire,
> creato dalla sezione che lo predica. Trovato in code review.

---

## 8. Il catalogo, classificato

Diciannove elementi, classificati contro `main` il **2026-08-17**. La colonna **Azione** è quella che
l'handoff chiede: `REUSE` · `UPDATE` · `CREATE` · `DEFER`.

| # | Elemento | Classe | Stato su `main` | Azione |
|--:|---|---|---|---|
| 1 | Cella / griglia | `EditorOnly` | **AS_BUILT** — `ARTHexMapActor`, `PIE-HEX` verde dal 2026-08-05 | `REUSE` |
| 2 | **Cell Placement Volume** | `EditorOnly` | **MISSING** — nessun equivalente in `Source/` né in `Content/` | `CREATE` |
| 3 | Unità | `CellBound` | **AS_BUILT** — cilindro con fallback, root neutro (`#593`), anello di team | `UPDATE` |
| 4 | Pavimento | architettonico | **AS_BUILT** — `ERTHexSurface::Floor` e disco della cella | `REUSE` |
| 5 | Muro | `EdgeBound` | **PARTIAL** — `FRTGeometrySegment` è l'authority; la presentazione esiste come volume derivato | `UPDATE` |
| 6 | Copertura bassa | `EdgeBound` | **AS_BUILT** (dato) — `ERTHexCoverType::Low`, `EdgePanelsSitOnTheDeclaredEdge` | `UPDATE` |
| 7 | Copertura alta | `EdgeBound` | **AS_BUILT** (dato) — `ERTHexCoverType::High`, CP 9.2 | `UPDATE` |
| 8 | Porta | `EdgeBound` | **AS_BUILT** — `FRTHexDoor`, **quattro** stati, `DoorPanelsShowWhetherYouCanPass` | `UPDATE` |
| 9 | Macerie | `CellBound` | **PLANNED** — `RT-FEAT-MAP-STRUCTURAL` è `IDEA`, release `future` | `DEFER` |
| 10 | Muro sfondato | `EdgeBound` | **PLANNED** — stesso owner del #9 | `DEFER` |
| 11 | Rampa | transizione | **PARTIAL** — i layer esistono, il traversal verticale no (`RT-FEAT-MAP-VERTICALITY`, `IDEA`) | `DEFER` |
| 12 | Piattaforma | architettonico | **PARTIAL** — statica sì; **mobile è fuori scope v0.1 dichiarato** (CP 10.1 §11) | `DEFER` |
| 13 | Acqua | `SurfaceBound` | **AS_BUILT** (dato) — `ERTHexSurface::ShallowWater` | `UPDATE` |
| 14 | Ghiaccio | `SurfaceBound` | **AS_BUILT** (dato) — `ERTHexSurface::Ice` | `UPDATE` |
| 15 | Valvola | `CellBound` | 🔴 **SUPERSEDED** — *«valvole, pompe e fluidodinamica»* sono **fuori scope v0.1 dichiarato** | `DEFER` |
| 16 | Generatore | `CellBound` | **PLANNED** — è l'esempio di macchina a stati di CP 10.1 §5, non un asset esistente | `DEFER` |
| 17 | Serbatoio hazard | `CellBound` | **PLANNED** — nessun produttore in v0.1 | `DEFER` |
| 18 | Relè | `CellBound` | **AS_BUILT** (dominio) — è l'obiettivo della showcase E15 «Il Relè» | `UPDATE` |
| 19 | Marker di spawn | `EditorOnly` | **PARTIAL** — lo spawn esiste in `RTMatchSetupLibrary`; il marker d'authoring no | `CREATE` |

**Il conto**: `REUSE` 2 · `UPDATE` 8 · `CREATE` 2 · `DEFER` 7 — somma **19**, e nessun elemento resta senza
classificazione. I sette `DEFER` si dividono per **ragione**, e le ragioni sono tre:

| Perché è differito | Quali | Quanti |
|---|---|--:|
| dipende da una feature `IDEA` su release `future` | #9, #10 (`RT-FEAT-MAP-STRUCTURAL`) · #11 (`RT-FEAT-MAP-VERTICALITY`) | **3** |
| **fuori scope v0.1 dichiarato** da CP 10.1 §11 | #12 (piattaforma mobile) · #15 (valvola) | **2** |
| proxy di un elemento che nessuno produce ancora | #16, #17 | **2** |

> ⚠️ **`#12` stava nella riga sbagliata, ed è la seconda correzione dello stesso paragrafo.** La stesura
> precedente lo attribuiva a `RT-FEAT-MAP-VERTICALITY` mentre la riga 12 della tabella, tre centimetri più
> su, dice «mobile è **fuori scope v0.1 dichiarato** (CP 10.1 §11)» — dove l'esclusione elenca *«ascensori e
> piattaforme mobili»* e non nomina la verticalità. L'aritmetica `4+3` chiudeva lo stesso, ed è il punto:
> **un totale che torna non convalida l'attribuzione dei suoi addendi.** Il paragrafo che si complimentava
> per aver contato le righe invece di rileggerle aveva riletto questa. Trovato in code review.

> ⚠️ **Questa riga diceva `UPDATE 7 · DEFER 8`**, ed era la coppia invertita: un conteggio scritto leggendo
> la tabella invece di contarla. La somma tornava lo stesso a **19**, che è il motivo per cui non saltava
> all'occhio — ed è il caso in cui un totale corretto non dice nulla sugli addendi. Rimisurato contando le
> righe con la loro colonna `Azione`, non rileggendole.

> 🔴 **La valvola è il caso che dimostra perché un kit si filtra.** Il kit la mette fra i diciannove della
> v0.1. CP 10.1 §11 la dichiara fuori scope con una motivazione registrata: *«l'acqua ha un produttore nel
> roster ([`D-046`](../decisions/RT_PDR_00_Decision_Log.md), `Hero.Phase.FluidTrail` **è**
> `Action.CreateWater`) e non serve un secondo modello per crearla»*. Modellarla ora non sarebbe lavoro in
> anticipo: sarebbe l'asset di un sistema che il progetto ha deciso di non costruire.

---

## 9. Quello che questo documento **non** decide

Sono domande aperte, e restano aperte. Vivono in [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md), che è il
posto delle cose che aspettano una persona; qui c'è solo *quali sono* e *da quale sezione nascono*.

| ID | Nasce da | In una riga |
|---|---|---|
| `GBX-1` | §5, §6 | quale frazione di `C` è il Safe Placement inset |
| `GBX-2` | §7.1 | quale canale non cromatico distingue `Closed` da `Locked` |
| `GBX-3` | §7.2 | a quali valori di `Integrity` corrispondono «danneggiato» e «critico» |
| `GBX-4` | §8 | sotto quale percorso di `Content/` vive il kit graybox |

### 9.1 Tre prescrizioni del kit che `main` smentisce

Registrate perché non tornino: un handoff respinto senza motivo scritto si ripresenta identico.

1. **«rotation step = 30°».** La grammatica canonica **non ha uno step di rotazione**: è
   `ERTTacticalAxis` più offset **interi**, in tre famiglie (direttrici dell'esagono, loro ortogonali,
   segmenti sul lato o sul perimetro), e ammette configurazioni a **90°**. I dodici settori da 30° esistono,
   ma sono un **righello** per misurare quanto una cella è invasa — *«non definiscono vicini, non
   definiscono facing, non definiscono su quale lato sta una copertura»*. Adottare uno step di 30° come
   quantizzazione d'authoring introdurrebbe il secondo sistema che il kit stesso vieta.

2. **«`Cell.HasCover = true` come modello»**, che il kit cita per scartarlo — ma il suo catalogo poi
   descrive la cover come oggetto della cella. Sul repository la copertura è **direzionale per bordo** dal
   formato mappa v3 (`E9.1`): *«la direzionalità è del bordo, non dell'unità: girarsi non sposta un
   muretto»*.

3. **«Temporary / Energy Cover»** fra le forme della cover. `ERTHexCoverType` ha `None · Low · High` e il
   commento sopra l'enum dichiara il criterio: *«inventare oggi un valore che nessuna regola sa applicare»*
   sarebbe l'errore. La copertura temporanea ha un owner proprio
   ([`../gameplay/spec-coperture-temporanee-cp95.md`](../gameplay/spec-coperture-temporanee-cp95.md)) e
   non entra da qui.

---

## 10. Verifica

**Nessuna parte di questo contratto è oggi difesa da un gate automatico**, ed è dichiarato invece che
sottinteso — è la stessa condizione in cui `D-146` ha lasciato la ridondanza colore+forma.

La ragione non è trascuratezza: l'oracolo di «è leggibile» non esiste nell'harness e non va simulato
contando ferite. Le verifiche di questo dominio sono **manuali**, di classe **C** secondo
[`scenario-map.md`](scenario-map.md), e il loro registro è
[`test-manuali-pie.md`](test-manuali-pie.md).

> 🔴 **Le voci PIE di questo contratto non sono ancora scritte, e il motivo è un vincolo di parallelismo,
> non una dimenticanza.** `test-manuali-pie.md` è nel write-set della track `playback` in
> [`parallel-batch.yaml`](../roadmap/parallel-batch.yaml): per `D-139` scriverci da un'altra track sarebbe
> la «piccola fix» su un file assegnato a qualcun altro. Il contenuto delle voci è pronto e vive nella
> issue di validazione visiva del kit.
>
> ⏱️ **La causa è cambiata tre volte in una sessione, e il vincolo non si è mai mosso.** All'apertura il
> file era in prestito a `playback` per `#1015`; poi `#1015` è stata **chiusa** mentre questo documento
> veniva scritto e il batch dichiarava ancora quella track `ACTIVE`; infine il rilascio è stato scritto e
> il path è **tornato a `playtest`**, che è `IDLE` — e in questo file `writable` su una track `IDLE`
> significa **prenotato**, non libero.
>
> **Il proprietario è cambiato, il permesso no.** La condizione di sblocco non è mai stata «la issue X
> chiude»: è che la track proprietaria **rilasci il path o lo ceda**. Chi legge questa riga apra
> [`parallel-batch.yaml`](../roadmap/parallel-batch.yaml) e guardi chi lo tiene **oggi** — la risposta è
> cambiata tre volte in un giorno, e ogni volta un nome scritto qui sarebbe invecchiato in silenzio.

Quello che una verifica dovrà mostrare, senza HUD e senza selezione:

```text
unità · copertura bassa vs alta · muro vs muro sfondato ·
porta aperta vs chiusa vs bloccata · acqua vs ghiaccio ·
intatto vs distrutto
```

a tre distanze di camera — ravvicinata, di gioco, tattica. **Se non è leggibile, si cambia la grammatica
prima di aggiungere altri asset**: è l'unica prescrizione del kit che questo documento adotta senza
emendarla.
