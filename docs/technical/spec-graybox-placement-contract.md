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
prisma esagonale — la cella logica È un volume, non una superficie
  ├─ lato              HexSize = 150       (1,50 m)
  ├─ lato-a-lato       C = √3 · HexSize    (2,60 m)
  ├─ altezza           H = LayerHeight = 250   (2,50 m)
  ├─ safe footprint    un inset, misurato in frazione di C — vedi §9, il valore è APERTO
  └─ guide verticali   frazioni di H — §6.2
```

⏱️ **I due valori assoluti qui sopra sono il canone, non ciò che il gioco fa oggi**: finché [`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155) non
atterra il mondo gira a `HexSize = 100`, quindi `C` vale `1,73 m` — mentre `H = 250` è già vero adesso.
Le frazioni (`0.28 H` in altezza, `0.92` del lato in larghezza) non ne risentono: è per questo che il contratto misura in frazioni.

🔑 **`H` è l'altezza del volume, non una distanza fra oggetti separati.** `AxialToWorld` pone i centri di
layer adiacenti a `Layer · LayerHeight`, quindi i prismi **tassellano** anche in verticale: sopra il
soffitto di una cella c'è il pavimento della successiva, senza intercapedine. Il campo `Height` di
`FRTHexCellData` non è questa altezza — è un offset di rendering *dentro* il volume, e il suo commento lo
dice: *«la logica usa Layer + archi»*.

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

Le misure si esprimono in frazioni, e i denominatori sono **due**, uno per asse:

```text
ingombri e inset       →  frazioni di C      passo centro-centro
elementi di BORDO      →  frazioni del lato  il bordo su cui stanno (= HexSize)
altezze                →  frazioni di H      altezza del volume-cella
```

I riferimenti sono **tre**, e la regola non è «due assi» ma **ogni misura sul segmento che la contiene**.
`C` e il lato sono entrambi orizzontali ma non intercambiabili: `C = √3 · lato`, quindi confonderli sbaglia
di **1,73×**. Un pannello appoggiato a un bordo si budgeta sul **bordo** — è `0.92` del lato — mentre un
inset, che misura quanto un asset si ritrae dal centro verso i vicini, si budgeta su `C`.

⏱️ In metri: `C = 2,60 m` **al canone** (`1,73 m` finché [`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155) non atterra) e `H = 2,50 m`, che è già il
valore di oggi. La grammatica è in frazioni proprio perché la prima delle due sta cambiando.

🔴 **Che siano due è una correzione del 2026-08-17** ([`D-168`](../decisions/RT_PDR_00_Decision_Log.md)):
fino a quel giorno anche le **altezze** erano espresse in frazioni di `C`, cioè misurate contro una
larghezza. L'incoerenza era invisibile finché nessuno dichiarava l'altezza del volume, e si vedeva solo
guardando i totali: `1.00 C` — la guida chiamata «massima» — valeva il **69%** del volume con la scala
vecchia e il **104%** con quella nuova. Una guida massima che *sfora* il volume che delimita, e che
cambia segno quando cambia la scala, è la firma di un denominatore sbagliato.

⚠️ **`0.92` per la larghezza di un pannello di bordo è una frazione del LATO, non di `C`**, ed è giusto così: è una larghezza.
La regola non è «tutto in `H`», è **ogni misura sul proprio asse**.

`C` è la distanza centro-centro fra due celle adiacenti.

`C` **non è una costante nuova**, e **non è `HexSize`**:

```text
C = √3 · HexSize
```

`HexSize` è il **raggio** dell'esagono (circumraggio), non il passo della griglia:
`URTHexLibrary::AxialToWorld` calcola `Wx = HexSize · √3 · (Q + R/2)`, quindi due celle adiacenti distano
`√3 · HexSize`. `HexSize` è un `UPROPERTY` dell'asset mappa e dell'attore, e una mappa **può cambiarlo** —
per questo le misure di questo documento sono in frazioni di `C`.

### 6.1 Quanto vale `C` in metri, e perché la domanda ha due risposte

[`convenzioni-contenuti-ue.md`](convenzioni-contenuti-ue.md) §11-bis fissa il lato dal 2026-08-09, e da
`D-163` quel valore governa **anche il mondo**, non solo l'authoring:

```text
lato dell'esagono = 1,5 m        (esattamente: HexSize = 150)
```

Il resto si deriva per geometria — `C = √3 · lato ≈ 2,60 m`, vertice-vertice `3,00 m`, apotema `~1,30 m` —
**a patto** che una unità Unreal valga un centimetro.

⚠️ **Che `1 UU = 1 cm` non è scritto da nessuna parte**: è la convenzione di default di Unreal, vera in
pratica e mai dichiarata nel repository (`grep -i 'UU'` sulle convenzioni dà **zero**). Qui si assume, e
l'assunzione è esplicita perché il giorno in cui qualcuno la cambiasse ogni numero di questa sezione
diventerebbe falso in silenzio.

### 6.2 La scala d'arte governa anche il mondo — deciso, non ancora atterrato

```text
canone (D-163)     lato 1,50 m      →  C ≈ 2,60 m     ← a cui si MODELLA, e a cui il mondo DEVE girare
codice di oggi     lato 1,00 m      →  C ≈ 1,73 m     ← a cui ogni mappa gira ANCORA
```

✅ **`GBX-6` è chiusa il 2026-08-17**: vince la scala d'arte, `HexSize = 150`, e la quota fra i piani
**resta `250` uu — 2,50 m — invece di seguirla**
([`D-163`](../decisions/RT_PDR_00_Decision_Log.md)). Questo documento **continua a non sceglierla** — non è
il suo owner, che resta [`convenzioni-contenuti-ue.md`](convenzioni-contenuti-ue.md) §11-bis.1 — ma ora
riporta una decisione presa invece di una divergenza aperta.

🔵 **Che l'altezza del volume non segua la larghezza cambia le proporzioni della scena, non i budget di
questo contratto.** I budget verticali sono frazioni di `H`, e `H` non si muove: `0.28 H` è 70 cm prima e
dopo. Quello che cambia è la **pianta**: ogni oggetto tiene la sua altezza mentre il pavimento su cui sta
diventa 1,5× più largo, quindi le silhouette si distanziano e la cella smette di essere affollata. È il
margine che rende `GBX-1` e `GBX-5` validabili **guardando** invece che discutendo — e si vede in pianta,
non in alzato.

> ⚠️ **E le altezze di `RTHexMapActor.cpp` non sono evidenza su questi budget**: sono **placeholder di
> visualizzazione**, che [`D-168`](../decisions/RT_PDR_00_Decision_Log.md) esclude dal perimetro di questo
> contratto.

⏱️ **Le due righe qui sopra non sono ancora la stessa.** Finché [`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155) non chiude, `C` vale
`1,73 m` a runtime e `2,60 m` sul tavolo di chi modella. **Modella per una cella larga `2,60 m`** — cioè
un lato di `1,50 m`, che è lo stesso vincolo detto nell'altra unità — e **rimanda il commit dei volumi
finiti** finché non li puoi validare in PIE.

> ⚠️ **Le due misure sono la stessa cosa, e vale la pena dirlo perché i documenti usano numeri diversi.**
> Qui il termine di paragone è `C`, la larghezza lato-a-lato, perché è a quello che il contratto budgeta le
> frazioni orizzontali (`0.92` del lato per la larghezza di un pannello; le **altezze** vanno in `H`, §6). L'owner della scala —
> [`convenzioni-contenuti-ue.md`](convenzioni-contenuti-ue.md) §11-bis.1 — parla del **lato**, perché è
> quello che `HexSize` contiene. `C = √3 · lato`: `1,50 → 2,60`. Le due righe dicono lo stesso
> vincolo in unità diverse, e ciascuna usa quella naturale per il proprio documento.

Misurato: `HexSize` vale `100.f` in **16 occorrenze su 10 file** di `Source/`, e **nessuna mappa lo
sovrascrive**. La distinzione che conta non è il totale ma **a cosa serve ciascuna**: **quattro** sono
codice di gioco e decidono il mondo, le altre stanno in `Tests/` e fissano una scala arbitraria.
I due che decidono il mondo sono `RTHexMapAsset.h:151` (l'autorevole: l'asset vince sull'actor) e
`RTHexMapActor.h:74` (il fallback quando `MapAsset` è assente); gli altri due — `RTHUD.cpp:335` e
`RTTurnManager.cpp` in `GetHexContext` — sono inizializzatori del ramo «nessuna mappa nel livello».

🔴 **E i siti da toccare non coincidono con i file da toccare**: c'è anche
`Source/RefactorTactics/Tests/RTHexMapTests.cpp`, che **pinna** il default a `100.f` come contratto di
serializzazione e va aggiornato deliberatamente. ⛔ **`RTHexMapTests.cpp` non è assegnato a nessuna track** — è un `D-139` STOP, da sciogliere **prima**,
non durante — e **`RTTurnManager.cpp` è della track `hotspot_split`, ACTIVE**: non si assegna, si coordina. Il quadro completo, con chi possiede cosa, sta in
[`D-163`](../decisions/RT_PDR_00_Decision_Log.md) e in [`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155); questo documento non è l'owner del write-set
e non deve diventarne una seconda copia.

> ⚠️ **Questo elenco non è il write-set, ed è la ragione per cui rimanda invece di ripetere.** Chi esegue
> la migrazione partendo da qui — ed è il documento che chi modella legge — cambierebbe i siti che vede e
> lascerebbe rosso il resto. L'owner del write-set è `parallel-batch.yaml`; l'owner del lavoro è la issue.

> ⚠️ **Come si verifica, e come NON si verifica.** `HexSize` è un `UPROPERTY(EditAnywhere)` su asset e
> attore, quindi il valore di una mappa vive dentro un `.uasset` o un `.umap` — **binari**. Un `grep` su
> `Scenarios/` e `Config/` non li apre nemmeno: è la misura che la prima stesura di questa riga citava, e
> non poteva sostenere la conclusione. I binari vanno ispezionati direttamente e **con l'oracolo giusto per
> ciascun tipo**, perché in un `.umap` il nome `Cells` è un *componente* e sarebbe presente comunque: la
> misura, con il suo oracolo, è in [`D-163`](../decisions/RT_PDR_00_Decision_Log.md).

**Le due scale divergono di 1,5× finché il cambio non atterra, e la divergenza è il costo di transito.** Una copertura bassa modellata a
`0.28 C` con `C = 2,60 m` è alta 73 cm; posata su una mappa reale — dove `C = 1,73 m` — quei 73 cm valgono
il **42% di `C`** invece del 28% che questo contratto budgetava. ⏱️ *Esempio dell'epoca in cui anche le altezze
stavano in `C`: da [`D-168`](../decisions/RT_PDR_00_Decision_Log.md) la guida bassa è `0.28 H` = **70 cm**, e un'altezza
non si rapporta più a una larghezza. Il costo di transito che l'esempio descrive resta reale — è la scala del
mondo a non essere ancora cambiata, non il modo di misurarla.* 🔴 *Questa riga diceva: «il termine di paragone è `C`, non «l'altezza della cella»: **una cella esagonale
non ha un'altezza**, e `C` è un passo orizzontale», e chiudeva con «la prima stesura scriveva «dell'altezza
di cella» e invitava a cercare un numero che non esiste». La clausola in grassetto è **falsa**. La cella ha un'altezza — `LayerHeight`, un
`UPROPERTY` accanto a `HexSize` e pinnato dallo stesso test — e vale `250`. La nota era nata per correggere
una prima stesura che diceva «dell'altezza di cella»: ha sostituito una formulazione imprecisa con
un'affermazione **falsa**, e nel farlo ha attivamente impedito di vedere che le guide verticali usavano il
denominatore sbagliato. Corretto in [`D-168`](../decisions/RT_PDR_00_Decision_Log.md); l'esempio qui sopra
resta in `C` perché descrive il costo di transito della scala, che è orizzontale.*

Chi modella oggi deve sapere che sta autorando per una cella che **nessuna mappa ha ancora** — non più per
una che nessuna mappa avrà mai. La differenza è tutta la decisione: prima era una domanda senza risposta,
ora è una scadenza.

> 🔑 **E §11-bis dichiara il proprio limite, che vale anche qui**: *«non è una metrica di design, e non va
> usata come tale»*. Serve a dimensionare mesh e proporzioni — *quanto è grande questo modello* — non a
> misurare mappe, che si dimensionano contando i **Move** e non i metri. Il contratto di ingombro sta dal
> lato dell'arte, ed è per questo che può usarla.

> ⏱️ **Questa sezione ha portato «≈ 173 con HexSize al suo default di 100» come se fosse la scala del
> progetto.** Era vero del default e falso della scala d'arte, che §11-bis fissava da giorni: un esempio
> numerico calcolato sul valore sbagliato dei due. Corretto il **2026-08-17** consumando il bundle
> `GrayToolkit`, che ha reso la divergenza visibile mettendo i due numeri accanto.

> 🔴 **La riga precedente diceva «`C` discende da `HexSize`, default `100.f`» senza il fattore `√3`.**
> *(La soglia citata qui sotto è quella dell'epoca, `0.28 C`; dal 2026-08-17 la guida bassa è `0.28 H` — il
> caso d'errore resta identico nella forma.)* Chi
> avesse letto «0.28 C» come 28 cm avrebbe modellato una copertura bassa alta il **58%** del dovuto —
> `0.28 · 173 ≈ 48 cm`, e `28/48 ≈ 0,58`, cioè il fattore `1/√3`. **Il 58% è ancorato a `C ≈ 173`**, non
> invariante: con la scala d'arte (`C ≈ 260`) lo stesso errore darebbe il 38%. Lo stesso valeva per il Safe
> Placement inset di `GBX-1`, espresso nella stessa unità.
> ⚠️ *E la prima stesura di questa nota diceva «alta un terzo del dovuto», confondendo il fattore d'errore
> (`√3 ≈ 1,73`) con il suo reciproco. Trovato in code review.*

Le guide verticali del volume sono **riferimenti di modellazione**, e non sono categorie di targeting:

```text
1.00 H   guida massima      250 cm   ← il soffitto del volume, esattamente
0.85 H   strutturale        213 cm
0.55 H   standard           138 cm
0.28 H   bassa               70 cm
0.00 H   piano d'appoggio     0 cm
```

> ⏱️ **Erano in `C` fino al 2026-08-17**, con gli stessi coefficienti. Il denominatore cambia, i
> coefficienti no — quindi i **centimetri cambiano**: la guida massima da `260` a `250`, la bassa da `73` a
> `70`. Chi avesse già modellato con i valori vecchi ha uno scarto del **4%**, che a graybox è dentro il
> rumore; chi modella da oggi usa questi. Deciso in [`D-168`](../decisions/RT_PDR_00_Decision_Log.md).

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
| `Locked` | pannello nel passaggio **più una traversa in rilievo** sul pannello stesso (`D-171`) |
| `Destroyed` | pannello rimosso — stato terminale, non richiudibile |

> 🔴 **`Locked` è il caso che il kit aveva dimenticato, ed è quello che rompeva la regola.** `Closed` e
> `Locked` **negano entrambi il passaggio** e sono geometricamente identici: la sola differenza è che il
> secondo non si apre. Se l'unico canale che li distingue fosse il colore, `D-146` sarebbe violata dal
> primo asset che il kit produce.
>
> ✅ **Chiuso il 2026-08-18 da `D-171`: il secondo canale è una traversa in rilievo modellata sul
> pannello.** Quindi `Locked` è una mesh **diversa** da `Closed`, non la stessa ricolorata — nessuno stato
> di visibilità da guidare, nessun secondo componente da tenere allineato al pivot di §4. Scartate le altre
> due opzioni di design: un catenaccio come mesh separata costava un asset più una regola di visibilità per
> stato; un'icona d'overlay spostava il canale nella **UI**, cioè fuori da questo contratto e dentro il
> linguaggio icone di **E20**, che ha un owner diverso.
>
> ⚠️ **Costo accettato, non rimosso**: il catalogo di §8 guadagna una voce, perché i due stati non
> condividono più una mesh sola. È il prezzo di avere il marcatore *dentro* la silhouette invece che sopra
> — ed è ciò che lo rende leggibile in scala di grigi a tutte e tre le distanze, che è la verifica di §10.

### 7.2 Integrità: il dato è un intero, gli stati sono una lettura

`FRTHexCover::Integrity` è un `int32` **e ha già un produttore vivo**:
`URTHexCoverLibrary::ApplyStructureDamage` → `DamageFace` scala l'integrità sulle **due facce** della
barriera e produce `FRTCoverDamageResult{RemainingIntegrity, bDestroyed}`, che entra nel TurnLog.

Il catalogo v0.1 ha **due** soglie di partenza, non una — `FRTHexCover::DefaultIntegrity` restituisce
`50` per `High` e `30` per `Low` — e i valori residui che i test pinnano attraverso `CoverIntegrityOn`
sono **`0 · 18 · 22 · 25 · 30`**. Non esiste una scala unica: dipende dal tipo di copertura e
dall'ammontare del colpo.

> 🔴 **Questa sezione ha sbagliato due volte, e la seconda peggio della prima.**
> *(1)* Diceva «la distruzione arriva con CP 9.2, che lo scalerà»: **falso**, CP 9.2 è chiuso dal
> 2026-08-07 e il produttore esiste. Era la trascrizione del commento sopra il campo in
> `RTHexCellData.h`, scritto quando il produttore non c'era.
> *(2)* La correzione ha poi citato come prova `RefactorTactics.EnvironmentAction` con
> «`Integrity == 20`, su entrambi i versi» — che è un'assertion su **`FRTHexEdge`**, cioè un **ponte**
> (`DefaultIntegrity = 40`), e i «due versi» sono le due direzioni di un **arco**, non le due facce di una
> copertura. Le assertion vere sulla copertura, nello stesso file, danno `0/18/22/25/30` e **mai 20**.
> ⚠️ **Sostituire una citazione sbagliata con un'altra citazione sbagliata è il difetto originale al
> quadrato**, ed è successo dentro la PR che esisteva per ripararlo. Entrambe trovate in code review.
> ➕ **E il generatore del difetto è tracciato invece di lasciato in piedi**: il commento in
> `RTHexCellData.h` che prometteva CP 9.2 al futuro è
> [#1107](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1107) — quel file è nel write-set
> della track `spatial`, quindi la correzione le viene **passata**, non scritta da fuori. Correggere le
> quattro trascrizioni e lasciare la fonte significa che il prossimo lettore le riscrive.

Ne discende una separazione che va scritta prima di modellare:

```text
Intatto      geometria piena, corpo neutro       Integrity == DefaultIntegrity(Type)
Danneggiato  stessa geometria + marcatore        Integrity <  DefaultIntegrity(Type)
Critico      stessa geometria + marcatore forte  Integrity * 3 <= DefaultIntegrity(Type)
Distrutto    geometria CAMBIATA — non è lo       bDestroyed
             stesso oggetto ricolorato
```

✅ **Le soglie esistono dal 2026-08-18, e sono FRAZIONI del catalogo — `D-172`.** Non potevano essere numeri
assoluti: le partenze sono **due**, `50` per `High` e `30` per `Low`, quindi «critico» o è una frazione o è
due numeri scollegati. La regola è in **aritmetica intera** — nessun float, nessun arrotondamento da
concordare fra chi modella e chi legge — e `Distrutto` non è una soglia affatto: `bDestroyed` era **già** un
esito enumerato in `FRTCoverDamageResult`.

**Perché ⅓, misurato invece che scelto**: `Action.HeavyAttack` fa `20` di `DamageStructure`
([`RTHexCoverTests.cpp`](../../Source/RefactorTactics/Tests/RTHexCoverTests.cpp)), quindi le sequenze reali
sono `High 50 → 30 → 10 → 0` e `Low 30 → 10 → 0`. Con ⅓ «critico» cade **esattamente sull'ultimo passo
prima di zero su entrambi i tipi**, cioè significa *un altro colpo e cade* — l'informazione che decide
l'azione successiva. Con ½ e ¼ una `Low` non sarebbe **mai** critica (`10 > 7`): cadrebbe da «danneggiata»,
e lo stato più forte della grammatica non si vedrebbe su metà del catalogo.

⚠️ **Resta presentazione, e la frazione è ciò che lo garantisce**: la lettura non entra nel resolver, non
cambia la riduzione del danno — che è di `Combat/` — e non entra in `ComputeHash`. Se il balance muove
`DefaultIntegrity` o `DamageStructure`, le letture **seguono** senza che questa sezione vada riscritta. Un
numero assoluto avrebbe legato la presentazione a due costanti di gameplay, ed è la stessa ragione per cui
la dimension grammar di §6 è relativa e mai in centimetri.

### 7.3 «Acceso/spento» non basta, e il numero di stati non lo fissa questo documento

Il kit propone `Online/Offline` come «accent acceso/spento». È troppo poco, e l'owner degli elementi
interattivi lo mostra con un esempio di macchina a stati
([`../gameplay/spec-interazioni-mappa-cp101.md`](../gameplay/spec-interazioni-mappa-cp101.md) §5) in cui un
generatore ne ha cinque — `Off · Online · Overloaded · Damaged · Destroyed`. `Overloaded` e `Damaged` sono
due modi diversi di non funzionare, con transizioni d'uscita diverse (`Stabilize`/`Disconnect` contro
`Repair`): una grammatica a due stati li fonderebbe, e il giocatore perderebbe l'informazione che decide
l'azione successiva.

> ⚠️ **Quel blocco è etichettato «Esempio, non catalogo», e questo documento non lo promuove a norma.**
> la stessa §5 dice che **ogni elemento dichiara i propri stati**: il primo con quattro o sei stati
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

### 8.1 Dove vivono — deciso il 2026-08-18, `D-173`

```text
/Game/RT/World/Graybox/
  Cover/       SM_Graybox_Cover_Low · SM_Graybox_Cover_High
  Doors/       SM_Graybox_Door_Panel · SM_Graybox_Door_Locked
  Surfaces/    SM_Graybox_Surface_Water · SM_Graybox_Surface_Ice
  Volumes/     BP_Graybox_CellPlacementVolume
```

Sotto `World/` e non sotto `World/Grid/`: §5 di
[`convenzioni-contenuti-ue.md`](convenzioni-contenuti-ue.md) descrive già `Grid/Generation/` come
*«generatori graybox»*, e porte e coperture stanno sui **bordi** (§3), non sulla griglia. Non un top-level
`/Game/RT/Graybox/`: quel livello è organizzato per **dominio**, e «graybox» è un modo di fare gli asset —
promuoverlo ad arte finale, sotto `World/`, è un rename locale.

⚠️ **La riga d'allowlist in `.gitignore` viene PRIMA del primo asset**, e c'è già:
[`asset-map.md`](asset-map.md) §6 lo prescrive perché senza di essa `git add` **tace e non segnala nulla**.
Oracolo: `git check-ignore -q <file>` → exit **`1`**; con `-v` esce `0` in entrambi i casi e non distingue.

⏱️ **Il percorso non rende committabile un asset oggi**: finché
[#1155](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155) non atterra il mondo gira a
`HexSize = 100` mentre si modella alla scala d'arte di §6.2 — si modella alla scala nuova e si rimanda il
**commit**, non il lavoro.

---

## 9. Quello che questo documento **non** decide

Sono domande aperte, e restano aperte. Vivono in [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md), che è il
posto delle cose che aspettano una persona; qui c'è solo *quali sono* e *da quale sezione nascono*.

**Ne restano due, e condividono l'oracolo**: si validano **guardando**, e la scena in cui guardarle è la
seduta **U25** ([#1095](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1095)). È ciò che le
separa dalle tre chiuse il 2026-08-18, che avevano già sul repository tutto il materiale per essere decise.

| ID | Nasce da | In una riga |
|---|---|---|
| `GBX-1` | §5, §6 | quale frazione di `C` è il Safe Placement inset |
| `GBX-5` | §6, §7 | quanto è grande l'unità rispetto alla cella — i `1,20 m` di oggi sono uno stato o un target |

⚠️ **`GBX-5` mancava da questa tabella**, pur essendo in `OPEN_DECISIONS.md` dal 2026-08-17: aggiunta il
2026-08-18. Una sezione che dichiara *«qui c'è solo quali sono»* e ne elenca una di meno non è un elenco,
è un campione.

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

### 9.2 Chiuse

Restano nominate qui perché §7 e §8 le citano, e un ID che sparisce dall'unico posto che lo definisce manda
il lettore a cercarlo altrove.

| ID | Esito | Dove |
|---|---|---|
| ~~`GBX-2`~~ | traversa in rilievo sul pannello — il marcatore è geometria, non colore e non UI | `D-171`, §7.1 |
| ~~`GBX-3`~~ | frazioni del catalogo: `critico ⟺ Integrity * 3 <= DefaultIntegrity(Type)` | `D-172`, §7.2 |
| ~~`GBX-4`~~ | `/Game/RT/World/Graybox/`, con `Cover/ · Doors/ · Surfaces/ · Volumes/` | `D-173`, §8 |
| ~~`GBX-6`~~ | vince la scala d'arte: lato `1,5 m`, `HexSize = 150` | `D-163`, §6.2 |

---

## 10. Verifica

**Nessuna parte di questo contratto è oggi difesa da un gate automatico**, ed è dichiarato invece che
sottinteso — è la stessa condizione in cui `D-146` ha lasciato la ridondanza colore+forma.

La ragione non è trascuratezza: l'oracolo di «è leggibile» non esiste nell'harness e non va simulato
contando ferite. Le verifiche di questo dominio sono **manuali**, di classe **C** secondo
[`scenario-map.md`](scenario-map.md), e il loro registro è
[`test-manuali-pie.md`](test-manuali-pie.md).

> 🔴 **Le voci PIE di questo contratto non sono ancora scritte, e il motivo è un vincolo di parallelismo,
> non una dimenticanza.** `test-manuali-pie.md` è **assegnato a un'altra track** in
> [`parallel-batch.yaml`](../roadmap/parallel-batch.yaml): per `D-139` scriverci da qui sarebbe la
> «piccola fix» su un file di qualcun altro. Il contenuto delle voci è pronto e vive nella issue di
> validazione visiva del kit.
>
> **Il nome dell'owner non si scrive qui, e il paragrafo sotto dice perché**: è cambiato tre volte in un
> giorno, e ogni nome inciso in questa riga sarebbe invecchiato in silenzio. Si legge nel batch.
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
