# Spec — Graybox Kit: contratto di ingombro, pivot e presentazione degli oggetti di mappa

> `CURRENT` · **Creato**: 2026-08-17 · **Owner**: questo file — è il contratto che dice **quanto spazio
> occupa** un asset di mappa, **dove sta il suo pivot**, e **come si legge il suo stato** senza chiedere
> nulla alla simulazione.
>
> **Autorità**: subordinata a [`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) e al
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md) — `D-152` e `D-153` sono le decisioni che questo
> documento esegue, `D-146` è quella che estende.
>
> **Nato da** un handoff d'autore del 2026-08-17 (`Graybox_Kit_Cover_CellVolume`, 1620 righe), archiviato
> in [`../../archive/src/README.md`](../../archive/src/README.md). Il kit è stato **filtrato** contro `main`, non
> applicato: tre delle sue prescrizioni descrivono un repository che non esiste, e sono elencate in §9.

---

## 1. Cosa questo documento non possiede

È la prima sezione perché il difetto più caro di quest'area è il **secondo owner**: un contratto di ingombro
somiglia a un modello di traversabilità, e chi confonde i due si ritrova con due sorgenti per lo stesso
numero.

| Tema | Owner |
|---|---|
| Coordinate, `FRTCellId`, transizioni fra layer, formato dell'asset | [`spec-mappa-multilivello.md`](../architecture/spec-mappa-multilivello.md) |
| Geometria **tattica**: asse, offset interi, occupancy, cottura | [`spec-hex-geometry-authoring.md`](spec-hex-geometry-authoring.md) |
| **Standability**: footprint e clearance che decidono dove si sta in piedi | `RT-FEAT-MAP-STANDABILITY` — E23 / CP 23.6 |
| Regole di copertura, riduzione danno, distruzione | [`../../gameplay/spec-copertura-cp91.md`](../../gameplay/spec-copertura-cp91.md) · [`../../gameplay/spec-copertura-alta-cp92.md`](../../gameplay/spec-copertura-alta-cp92.md) |
| Verbi e stati degli elementi interattivi | [`../../gameplay/spec-interazioni-mappa-cp101.md`](../../gameplay/spec-interazioni-mappa-cp101.md) |
| Grammatica visiva delle **celle** (colore + forma) | `D-146` · `RT-FEAT-UI-BOARD-GRAMMAR` |
| Percorsi e naming dentro `Content/` | [`convenzioni-contenuti-ue.md`](../tooling/convenzioni-contenuti-ue.md) |
| Quali asset esistono e quanti mancano | [`asset-map.md`](../tooling/asset-map.md) |
| Principi di pipeline: presentazione-only, riferimenti soft, licenze | [`spec-asset-pipeline.md`](../architecture/spec-asset-pipeline.md) |
| Stato di avanzamento | `../../roadmap/feature-registry.yaml` |

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

✅ **I due valori assoluti qui sopra sono il canone, ed è anche ciò che il gioco fa**: `C = 259,8 uu` e
`H = 250 uu` valgono a runtime, non solo sul tavolo di chi modella.
🔴 **Questa riga diceva «finché [`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155) non atterra il mondo gira a `HexSize = 100`, quindi `C` vale
`1,73 m`»: falso dal 2026-08-25**, quando #1155 è atterrata. Rimisurato il 2026-08-28: tutti e quattro i
siti che decidono il mondo portano `150.f` — l'elenco sta in §7.
Le frazioni (`0.28 H` in altezza, `0.92` del lato in larghezza) non ne hanno risentito, ed è esattamente
per questo che il contratto misura in frazioni: la scala è cambiata sotto e nessun budget di §6 si è mosso.

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

In metri: `C = 2,60 m` e `H = 2,50 m`, ed entrambi sono i valori di oggi. 🔴 **Questa riga portava un
secondo numero fra parentesi — «`1,73 m` finché [`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155) non atterra» — e dal 2026-08-25 quella
parentesi è caduta, non il numero fuori.** La grammatica resta in frazioni perché è ciò che ha reso il
cambio di scala indolore, non perché qualcosa stia ancora cambiando.

🔴 **Che siano due è una correzione del 2026-08-17** ([`D-168`](../../decisions/RT_PDR_00_Decision_Log.md)):
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

[`convenzioni-contenuti-ue.md`](../tooling/convenzioni-contenuti-ue.md) §11-bis fissa il lato dal 2026-08-09, e da
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
([`D-163`](../../decisions/RT_PDR_00_Decision_Log.md)). Questo documento **continua a non sceglierla** — non è
il suo owner, che resta [`convenzioni-contenuti-ue.md`](../tooling/convenzioni-contenuti-ue.md) §11-bis.1 — ma ora
riporta una decisione presa invece di una divergenza aperta.

🔵 **Che l'altezza del volume non segua la larghezza cambia le proporzioni della scena, non i budget di
questo contratto.** I budget verticali sono frazioni di `H`, e `H` non si muove: `0.28 H` è 70 cm prima e
dopo. Quello che cambia è la **pianta**: ogni oggetto tiene la sua altezza mentre il pavimento su cui sta
diventa 1,5× più largo, quindi le silhouette si distanziano e la cella smette di essere affollata. È il
margine che rende `GBX-1` e `GBX-5` validabili **guardando** invece che discutendo — e si vede in pianta,
non in alzato.

> ⚠️ **E le altezze di `RTHexMapActor.cpp` non sono evidenza su questi budget**: sono **placeholder di
> visualizzazione**, che [`D-168`](../../decisions/RT_PDR_00_Decision_Log.md) esclude dal perimetro di questo
> contratto.

✅ **Le due righe qui sopra sono ormai la stessa.** `C` vale `2,60 m` a runtime e sul tavolo di chi
modella: **modella per una cella larga `2,60 m`** — cioè un lato di `1,50 m`, che è lo stesso vincolo
detto nell'altra unità.
🔴 **Questa riga aggiungeva «e rimanda il commit dei volumi finiti finché non li puoi validare in
PIE», e dal 2026-08-25 va letta al contrario**: chiuso [`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155), il commit non ha più nulla da
attendere. Il PIE resta dovuto — §10 del `CLAUDE.md` EDITOR — ma è un gate di qualità, non un divieto di
versionare, e confonderli tiene fuori dal repository asset che ci possono entrare.

> ⚠️ **Le due misure sono la stessa cosa, e vale la pena dirlo perché i documenti usano numeri diversi.**
> Qui il termine di paragone è `C`, la larghezza lato-a-lato, perché è a quello che il contratto budgeta le
> frazioni orizzontali (`0.92` del lato per la larghezza di un pannello; le **altezze** vanno in `H`, §6). L'owner della scala —
> [`convenzioni-contenuti-ue.md`](../tooling/convenzioni-contenuti-ue.md) §11-bis.1 — parla del **lato**, perché è
> quello che `HexSize` contiene. `C = √3 · lato`: `1,50 → 2,60`. Le due righe dicono lo stesso
> vincolo in unità diverse, e ciascuna usa quella naturale per il proprio documento.

Misurato il **2026-08-28**: i quattro siti che decidono il mondo portano tutti `HexSize = 150.f`, e
**nessuna mappa li sovrascrive**. Sono `RTHexMapAsset.h:190` (l'autorevole: l'asset vince sull'actor),
`RTHexMapActor.h:132` (il fallback quando `MapAsset` è assente), e i due inizializzatori del ramo
«nessuna mappa nel livello» — `RTHUD.cpp:506` e `RTTurnManager.cpp:4532` in `GetHexContext`.

Le **21 occorrenze residue di `100.f`** non decidono nulla: stanno tutte in `Tests/`, dove fissare una
scala arbitraria è il loro mestiere, oppure in prosa di commento (`RTGeometryGrammar.h:14`,
`RTGeometryGrammarTests.cpp:46`). La distinzione che conta non è mai stata il totale ma **a cosa serve
ciascuna occorrenza**, ed è la ragione per cui il numero grande non allarma.

🔴 **Questo blocco era stale in ogni particolare, ed è stato riscritto il 2026-08-28.** Diceva
«`HexSize` vale `100.f` in **16 occorrenze su 10 file**», nominava `RTHexMapAsset.h:151` /
`RTHexMapActor.h:74` / `RTHUD.cpp:335`, e concludeva che `RTHexMapTests.cpp` **pinna** il default a
`100.f` come contratto di serializzazione «da aggiornare deliberatamente». Oggi quel test pinna `150.f`
e lo dichiara da sé (`RTHexMapTests.cpp:1325-1342`: *«questo default È CAMBIATO il 2026-08-25,
deliberatamente: `100.f` -> `150.f` (`#1155`)»*), e le righe citate si erano spostate di decine di
posizioni. ⚠️ **Valeva la pena correggerlo invece di cancellarlo**: la forma «Misurato:» si legge come
evidenza, ed è la più costosa da lasciare stale — una misura vecchia non sembra vecchia.

⛔ **Il write-set di [`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155) è chiuso, e questo documento non ne è mai stato l'owner.** Il quadro
completo, con chi possedeva cosa, resta in [`D-163`](../../decisions/RT_PDR_00_Decision_Log.md) e nella
issue; qui sopravvive solo la scala che ne è uscita.

> ⚠️ **Questo elenco non è il write-set, ed è la ragione per cui rimanda invece di ripetere.** Chi esegue
> la migrazione partendo da qui — ed è il documento che chi modella legge — cambierebbe i siti che vede e
> lascerebbe rosso il resto. L'owner del write-set è `parallel-batch.yaml`; l'owner del lavoro è la issue.

> ⚠️ **Come si verifica, e come NON si verifica.** `HexSize` è un `UPROPERTY(EditAnywhere)` su asset e
> attore, quindi il valore di una mappa vive dentro un `.uasset` o un `.umap` — **binari**. Un `grep` su
> `Scenarios/` e `Config/` non li apre nemmeno: è la misura che la prima stesura di questa riga citava, e
> non poteva sostenere la conclusione. I binari vanno ispezionati direttamente e **con l'oracolo giusto per
> ciascun tipo**, perché in un `.umap` il nome `Cells` è un *componente* e sarebbe presente comunque: la
> misura, con il suo oracolo, è in [`D-163`](../../decisions/RT_PDR_00_Decision_Log.md).

**Le due scale divergono di 1,5× finché il cambio non atterra, e la divergenza è il costo di transito.** Una copertura bassa modellata a
`0.28 C` con `C = 2,60 m` è alta 73 cm; posata su una mappa reale — dove `C = 1,73 m` — quei 73 cm valgono
il **42% di `C`** invece del 28% che questo contratto budgetava. ⏱️ *Esempio dell'epoca in cui anche le altezze
stavano in `C`: da [`D-168`](../../decisions/RT_PDR_00_Decision_Log.md) la guida bassa è `0.28 H` = **70 cm**, e un'altezza
non si rapporta più a una larghezza. Il costo di transito che l'esempio descrive resta reale — è la scala del
mondo a non essere ancora cambiata, non il modo di misurarla.* 🔴 *Questa riga diceva: «il termine di paragone è `C`, non «l'altezza della cella»: **una cella esagonale
non ha un'altezza**, e `C` è un passo orizzontale», e chiudeva con «la prima stesura scriveva «dell'altezza
di cella» e invitava a cercare un numero che non esiste». La clausola in grassetto è **falsa**. La cella ha un'altezza — `LayerHeight`, un
`UPROPERTY` accanto a `HexSize` e pinnato dallo stesso test — e vale `250`. La nota era nata per correggere
una prima stesura che diceva «dell'altezza di cella»: ha sostituito una formulazione imprecisa con
un'affermazione **falsa**, e nel farlo ha attivamente impedito di vedere che le guide verticali usavano il
denominatore sbagliato. Corretto in [`D-168`](../../decisions/RT_PDR_00_Decision_Log.md); l'esempio qui sopra
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
0.06 H   tile della cella    15 cm   ← il pavimento È un volume, e da D-240 lo dice
0.00 H   piano d'appoggio     0 cm
```

🔑 **`0.06 H` è entrato il 2026-08-29 con [`D-240`](../../decisions/RT_PDR_00_Decision_Log.md), e non è una
guida di modellazione come le altre quattro: è una misura del CODICE**, `RTCellThicknessInH` in
[`Map/RTMapVisuals.h`](../../../Source/RefactorTactics/Map/RTMapVisuals.h). Sta in questa tabella perché è la
quota su cui ogni altra si appoggia — la faccia del tile è il piano d'appoggio reale, non lo zero teorico —
e perché il difetto che l'ha prodotta è esattamente quello che questa sezione descrive.

🔴 **Il codice portava il denominatore che `D-168` aveva già corretto qui.** Lo spessore era
`RTCellFlatScale = 0.05` moltiplicato per il **raggio** della mesh: un'altezza misurata contro una larghezza,
cioè la stessa incoerenza che il 2026-08-17 ha spostato le guide verticali da `C` a `H`. Il documento aveva
diagnosticato il difetto; il codice lo ha portato altri dodici giorni.

⚠️ **E si è pagato in silenzio, che è il motivo per cui vale registrarlo.** `PlanarScale` deriva da
`HexSize`, quella costante no: con [`#1155`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155)
(`HexSize` `100 → 150`) la cella si è allargata di 1,5× e lo spessore è rimasto 5 uu, scendendo dal **2,63%**
all'**1,75%** del diametro senza una riga di diff. È la firma di difetto che
[`D-163`](../../decisions/RT_PDR_00_Decision_Log.md) registra per le altezze, verificatasi una seconda volta
su una costante che nessuno aveva convertito.

⛔ **`0.06 H` è un TETTO, non una preferenza**, e il vincolo non è quello che sembra. Il tile è centrato,
quindi ispessirlo lo fa scendere anche sotto: più spesso di un gradino di rilievo — `ReliefUnitHeight`, 15 uu
— due celle a quota adiacente si **compenetrano** invece di gradinare. `0.06 H` **è** quel gradino, e
l'uguaglianza è ammessa perché a spessore uguale le facce si toccano senza compenetrare. Lo asserisce
`RTHexMapActor.cpp`, il solo file che posa sia il tile sia il rilievo.

> ⏱️ **Erano in `C` fino al 2026-08-17**, con gli stessi coefficienti. Il denominatore cambia, i
> coefficienti no — quindi i **centimetri cambiano**: la guida massima da `260` a `250`, la bassa da `73` a
> `70`. Chi avesse già modellato con i valori vecchi ha uno scarto del **4%**, che a graybox è dentro il
> rumore; chi modella da oggi usa questi. Deciso in [`D-168`](../../decisions/RT_PDR_00_Decision_Log.md).

> ⚠️ **Nessuna di queste soglie decide una regola.** Se il gameplay avesse bisogno di classi d'altezza, il
> suo owner sarebbe la copertura (`ERTHexCoverType`, due valori più `None`) — e quel dato **esiste già**,
> quindi una guida visuale che ne inventasse un terzo creerebbe la divergenza descritta in §1.


### 6.3 I budget di forma del kit — scritti prima del generatore, e per quella ragione

Le guide di §6 dicono **quanto è alto** un oggetto e **quanto è largo** un pannello. Non dicono quanto è
**spesso**, né che forma abbia il marcatore di `Locked`, né come sia fatta una superficie. Finché le sei mesh
del catalogo si modellavano a mano quel vuoto era una libertà; da [`D-229`](../../decisions/RT_PDR_00_Decision_Log.md)
si **generano in codice**, e un vuoto in questa tabella diventa un numero scelto da chi scrive il generatore —
che è esattamente ciò che §6 esiste per impedire.

| Misura | Valore | Denominatore | Su quale asset |
|---|---:|---|---|
| altezza della copertura bassa | `0.28 H` | `H` — 70 cm | `SM_Graybox_Cover_Low` |
| altezza della copertura alta | `0.85 H` | `H` — 213 cm | `SM_Graybox_Cover_High` |
| larghezza di un pannello di bordo | `0.92` | **lato** — 138 cm | tutti gli `EdgeBound` |
| **spessore** della copertura bassa | `0.10` | **lato** — 15 cm | `SM_Graybox_Cover_Low` |
| **spessore** della copertura alta | `0.20` | **lato** — 30 cm | `SM_Graybox_Cover_High` |
| altezza di un pannello di porta | `0.85 H` | `H` — 213 cm | `SM_Graybox_Door_Panel` · `_Locked` |
| spessore di un pannello di porta | `0.10` | **lato** — 15 cm | `SM_Graybox_Door_Panel` · `_Locked` |
| fascia della traversa di `Locked` | `0.12 H` | `H` — 30 cm | `SM_Graybox_Door_Locked` |
| rilievo della traversa, **per faccia** | `0.06` | **lato** — 9 cm | `SM_Graybox_Door_Locked` |
| spessore di una superficie | `0.02 H` | `H` — 5 cm | `SM_Graybox_Surface_*` |
| pianta di una superficie | `1.00` | footprint **esterno** | `SM_Graybox_Surface_*` |

🔑 **Le due coperture si distinguono in PIANTA, e il fattore è `2`.** Non è una preferenza: `PIE-GBX-COVER`
porta il proprio precedente di fallimento — `PIE-HEX-VIZ-BLOCCHI` è ❌ dal 2026-08-20
([#1246](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1246)) perché *«la differenza fra due
volumi stava nell'altezza, che la vista a picco proietta a zero»*. Due prismi con lo stesso spessore e due
valori di Z ripeterebbero quel difetto **identico**, e la voce nascerebbe ❌ senza che nessuno abbia guardato
male. Il fattore `2` sullo spessore non è scelto a occhio: è il contrasto d'area che
[`D-183`](../../decisions/RT_PDR_00_Decision_Log.md) ha già **misurato leggibile a picco** sui glifi di
superficie — `9,7%` contro `18,4%` per uno e due anelli, `PIE-HEX-VIZ-COSTO` ✅.

> ⛔ **Scartata la feritoia**, che sarebbe stata il canale in pianta più vistoso: una copertura alta con
> un'apertura **mente su una regola**. CP 9.2 la dichiara bloccante per la linea di tiro, e una silhouette che
> suggerisce il contrario insegna al giocatore una cosa falsa — che è peggio di una silhouette ambigua.

⚠️ **La porta è larga `0.92` del lato come ogni altro pannello di bordo, e non meno.** Restringerla avrebbe
dato il canale in pianta che la separa da una copertura — ma una porta **chiusa nega il passaggio**, e due
fessure ai lati direbbero il contrario: è lo stesso difetto della feritoia, su un altro asset. La coppia
*porta vs copertura* non è fra quelle che §10 enumera; quelle che ci sono — i **quattro stati** della porta —
si separano fra loro, ed è lì che il canale serve.

🔑 **La traversa di `Locked` sporge su ENTRAMBE le facce**, e discende da §3: un `EdgeBound` non appartiene a
nessuna delle due celle che condividono il bordo, quindi si guarda da entrambi i lati. Un marcatore su una
faccia sola sarebbe leggibile dalla metà delle posizioni di camera, e `D-171` ha scelto la geometria proprio
per non dipendere dal punto di vista.

🔑 **Una superficie copre il footprint ESTERNO, non l'inset**, e non è un'eccezione a §5: il Safe Placement
inset budgeta quanto un **oggetto** si ritrae dai vicini, e l'acqua di una cella arriva al suo bordo perché è
terreno, non oggetto. §3 già lo dichiara classificando `SurfaceBound` come *«rispetta il Safe Volume: non
applicabile»*; qui si scrive il numero che ne discende.

⚠️ **Acqua e ghiaccio si separano per FRATTURA, non per tinta.** `PIE-GBX-SURFACE` chiede di distinguerle a
zoom **tattico** e in scala di grigi: l'acqua è una lastra **piatta e continua**, il ghiaccio è composto di
lastre irregolari con spigoli e dislivelli entro lo stesso `0.02 H`. 🔴 **E questo è il budget che dipende
dallo shading**, quindi dalla verifica che `D-229` mette al primo posto: se la mesh generata arriva senza
normali, le facce del ghiaccio hanno tutte la stessa luminanza e il canale sparisce — la coppia fallirebbe per
una ragione che non ha niente a che vedere con la forma scelta qui.

> ⏳ **Questi budget si validano guardando, come tutto il resto di questo contratto**: sono l'ipotesi con cui
> la seduta **U25** si siede, non il suo verdetto. Se una coppia non regge alle tre distanze, si cambia il
> numero **qui** — è §10 che lo prescrive: *«se non è leggibile, si cambia la grammatica prima di aggiungere
> altri asset»*. Ciò che questa sezione impedisce è di scoprirlo **senza sapere quale numero era stato usato**.
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
> ⚠️ **Costo accettato, non rimosso**: l'elemento **#8** del catalogo passa da una mesh a **due** —
> `SM_Graybox_Door_Panel` e `SM_Graybox_Door_Locked` in §8.1. È il prezzo di avere il marcatore *dentro* la
> silhouette invece che sopra, ed è ciò che lo rende leggibile in scala di grigi a tutte e tre le distanze,
> che è la verifica di §10.
>
> ⚠️ **Il conto degli elementi NON cambia e resta diciannove**: §8 classifica **elementi**, §8.1 elenca
> **path**, e una porta a due mesh è un elemento con due file. *La prima stesura scriveva «il catalogo di §8
> guadagna una voce» — qui, in `D-171` e in `OPEN_DECISIONS.md` — e in nessuno dei tre applicava il cambio:
> chi fosse andato a contare §8 avrebbe trovato `19` con `UPDATE 8` e una famiglia-porta a due mesh che non
> compariva. Un costo dichiarato in tre punti e applicato in zero. Trovato in code review su #1188.*

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

🔑 **Il dominio della lettura è UNA ENTRY di `Covers`**, e i tre predicati non sono mutuamente esclusivi:
l'ordine di valutazione è parte della regola. Si legge dall'alto e **il primo che regge vince**:

```text
Dato: una entry di `Covers` — cioè una copertura che ESISTE.

1. Critico      stessa geometria + marcatore forte  Integrity * 3 <= DefaultIntegrity(Type)
2. Ridotto      stessa geometria + marcatore        Integrity <  DefaultIntegrity(Type)
3. Intatto      geometria piena, corpo neutro       altrimenti
```

🔵 **`Ridotto` si chiamava `Danneggiato` fino a [`D-186`](../../decisions/RT_PDR_00_Decision_Log.md)**
(2026-08-24), e il cambio non è cosmetico: le tre letture dichiarano quanto una copertura è **forte rispetto
al catalogo del proprio tipo**, non se qualcuno l'ha colpita. *«Danneggiato»* affermava una causa, ed era
**falsa** per un pannello `Adaptive` appena eretto — che nasce a `25` contro un catalogo `30` perché la
fragilità è il prezzo della rotazione gratuita, non perché l'abbia colpito qualcosa.
⚠️ **Le soglie non cambiano**, e `DefaultIntegrity` resta il catalogo: cambia il nome della lettura, non il
metro. Le occorrenze di «danneggiato» che restano più in basso sono **dentro i blockquote di code
review**: raccontano la prima stesura con le parole che aveva allora, e restano.
⚠️ **Una terza NON era storica ed è stata corretta**: l'argomento su ¼ descrive al presente cosa
farebbe la soglia scartata, quindi sotto `D-186` quella `Low` a `10` resta «ridotta». La prima
stesura di questa nota le dichiarava storiche tutte e tre — trovato nel panel della stessa PR.

**«Distrutto» NON è in questa scaletta, e a toglierlo è `D-175`.** Non è una lettura del dato di mappa: è la
transizione `ERTEnvironmentOutcome::CoverDestroyed`
([`RTTurnLog.h`](../../../Source/RefactorTactics/Turn/RTTurnLog.h)), che vive nel TurnLog. Il presentatore che
deve cambiare geometria — macerie invece del pannello — la prende **da lì**, non dall'assenza dell'entry.

> 🔴 **Questa è la TERZA stesura, e la seconda sbagliava il dominio in due modi che si sommano — trovati
> nella riverifica di [#1197](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1197), entrambi
> verificati contro `Source/`. Emendamento: `D-175`.**
>
> *(A)* **Il predicato `1. Distrutto ⟸ l'entry non è più in Covers` non aveva dominio.** Gli altri predicati
> leggono `Integrity`, che esiste solo se l'entry c'è; quello leggeva la sua **assenza**. Due domini in una
> catena sola, e §7.2 non dichiarava nessun quantificatore: col dominio «i bordi della mappa» il predicato 1
> regge su **ogni bordo nudo**, e il presentatore rende «geometria CAMBIATA» su una mappa vuota; col dominio
> «le entry di `Covers`» il predicato 1 è **irraggiungibile**, perché chi itera vede solo ciò che c'è. ∴ era
> il punto *(3)* qui sotto — «distrutto non è osservabile dal dato di mappa» — scritto **dentro** una
> scaletta che lo pretendeva osservabile.
>
> *(B)* **L'assenza dell'entry non significa distruzione, e i modi di produrla IN PARTITA sono TRE — che
> nemmeno passano tutti per la stessa funzione:**
>
> | Percorso | Come l'entry sparisce | Esito loggato |
> |---|---|---|
> | il danno porta l'integrità a zero | `Updated.Covers.RemoveAt(I)` dentro `DamageFace`, l'helper che `ApplyStructureDamage` chiama una volta per faccia ([`RTHexCoverLibrary.cpp`](../../../Source/RefactorTactics/Map/RTHexCoverLibrary.cpp)) | `CoverDestroyed` ([`RTTurnManager_Blast.cpp`](../../../Source/RefactorTactics/Turn/RTTurnManager_Blast.cpp)) |
> | la durata scade | `URTHexCoverLibrary::RemoveCover`, da `TickDynamicCovers` ([`RTTurnManager.cpp`](../../../Source/RefactorTactics/Turn/RTTurnManager.cpp)) | `CoverExpired` |
> | `Reconfigure` sposta il pannello su un altro bordo | `URTHexCoverLibrary::RemoveCover` | `CoverMoved` |
>
> Il caso della scadenza è vivo oggi: `Hero.Riktor.KineticPanel.Reinforced` dichiara `Integrity 45` e
> `DurationTurns 1`, quindi a Cleanup il pannello sparisce **integro** — e la seconda stesura lo avrebbe
> renderizzato come macerie. Quello dello spostamento è peggio: il bordo di partenza sarebbe reso come
> macerie mentre lo stesso pannello è intero sul bordo accanto. ∴ il TurnLog distingue i tre casi; il dato
> di mappa, a fase conclusa, non ne conserva **nessuno**.
>
> ⚠️ **«In partita» è una restrizione necessaria, non un'esitazione**: `BakeCell`
> ([`RTGeometryBake.cpp`](../../../Source/RefactorTactics/Map/RTGeometryBake.cpp)) scarta le coperture
> `bGenerated` al rebake, quindi toglie entry senza che nessuno le abbia colpite. È **authoring**: il suo
> unico chiamante fuori dai test è `RTHexGeometryTool`, nel modulo Editor, e non logga nessun
> `ERTEnvironmentOutcome`. Non indebolisce la conclusione — è dichiarato perché chi cerca dove le entry
> spariscono lo incontra comunque, e senza il dominio riaprirebbe la domanda che questa sezione chiude.
>
> ⛔ *Qui c'era un comando di verifica, ed è stato tolto invece che riparato. Sbagliava due volte: senza `-r`
> non cercava nella cartella, e il suo pattern prendeva anche `DynamicCovers`, un altro contenitore — così
> «quarto sito» era falso, perché a mutare le `Covers` di cella sono `DamageFace`, `RemoveCover` e
> `BakeCell`. Un oracolo sbagliato è peggio di nessun oracolo: chi lo esegue conclude che l'affermazione non
> è misurata. I simboli sono nominati; il comando lo sceglie chi verifica.*
>
> ⚠️ *Questo punto ne ha enumerati male DUE volte, e la seconda è istruttiva. Prima diceva «due» —
> distruzione e scadenza — qualificandosi «misurato non dedotto»; poi, correggendolo a tre, ha attribuito
> tutti e tre a `RemoveCover`, che ha **due soli chiamanti di produzione**: la distruzione rimuove l'entry
> inline e non passa di lì. Un conteggio dichiarato è falsificabile contro `Source/` — e la correzione di un
> conteggio lo è esattamente quanto il conteggio. Entrambe trovate in code review su
> [#1208](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1208). La conclusione non è mai
> cambiata, e `D-175` ne esce rafforzata: l'assenza è ancora meno informativa di quanto si sostenesse.*
>
> ✅ **Togliere «distrutto» dalla scaletta chiude entrambi con un emendamento solo**, e non chiede al
> presentatore nessuno stato che oggi non ha: la scaletta resta una funzione pura di una entry esistente, e
> la geometria cambiata la guida l'evento che già la nomina.

> 🔴 **La PRIMA stesura era difettosa in tre punti indipendenti — trovati in code review su #1188, tutti
> verificati contro `Source/`.** Restano scritti perché la correzione qui sopra poggia sul punto *(3)*, e
> perché un difetto rimosso senza traccia si riscrive.
>
> *(1)* **Non era ordinata, e senza ordine `Critico` è irraggiungibile.** Una `High` a `10` soddisfa
> `danneggiato` (`10 < 50`) **e** `critico` (`10*3 = 30 ≤ 50`): chiunque la implementasse come catena
> `if/else if` nell'ordine dichiarato non arriverebbe mai allo stato che la decisione esiste per rendere
> visibile.
>
> *(2)* **`Intatto` era `Integrity == DefaultIntegrity(Type)`, e non copriva i valori SOPRA il catalogo.**
> `Hero.Riktor.KineticPanel.Reinforced` ne produce uno oggi: dichiara `Integrity` **45** nei propri
> `Parameters` ([`RTHeroCatalogLibrary.cpp`](../../../Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp)),
> e `ARTTurnManager` lo applica con `AddCover(..., ERTHexCoverType::Low, Op.Integrity)` — dove
> `DefaultIntegrity(Low)` è `30`. Un pannello rinforzato a `45` non era intatto, non era danneggiato e non
> era critico: **nessuno stato da renderizzare**. Con `altrimenti` è intatto, che è ciò che è.
>
> ⚠️ *Questo punto nominava anche il `25` di `Adaptive` fra i «valori SOPRA il catalogo», e il `25` sta
> **sotto** un default di `30`. Corretto il 2026-08-19: non è un esempio della lacuna che `altrimenti`
> chiude, è un secondo produttore del difetto di
> [#1194](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1194) — vedi il residuo più sotto.*
>
> *(3)* **`Distrutto ⟺ bDestroyed` non era osservabile, e `bDestroyed` non è un «esito enumerato».** È un
> `bool` su una struct di **ritorno** (`FRTCoverDamageResult`,
> [`RTHexCoverLibrary.h`](../../../Source/RefactorTactics/Map/RTHexCoverLibrary.h)), valorizzato e seguito
> immediatamente da `Updated.Covers.RemoveAt(I)`: a fase conclusa l'entry **non esiste più** nella cella,
> quindi chi legge il dato di mappa non può osservarlo mai. `bDestroyed` è il segnale dell'**evento**, e
> vive nel TurnLog di quel turno. La frase «esito enumerato» era una trascrizione di questa sezione, e la
> ripeteva anche `D-172`.
>
> ⚠️ *Questo punto concludeva «per il presentatore distrutto è l'**assenza** dell'entry», ed è la conclusione
> che `D-175` ha dovuto correggere: l'assenza è prodotta **anche** dalla scadenza e dallo spostamento,
> quindi non identifica la distruzione — vedi la tabella dei tre percorsi al punto *(B)*.
> Il resto del punto — `bDestroyed` non è osservabile dal dato di mappa — regge, ed è
> esattamente ciò su cui `D-175` si appoggia. Corretto il 2026-08-19.*

✅ **Le soglie esistono dal 2026-08-18, e sono FRAZIONI del catalogo — `D-172`.** Non potevano essere numeri
assoluti: le partenze sono **due**, `50` per `High` e `30` per `Low`, quindi «critico» o è una frazione o è
due numeri scollegati. La regola è in **aritmetica intera**: nessun float, nessun arrotondamento da
concordare fra chi modella e chi legge.

**Perché ⅓, e cosa la misura dimostra davvero.** `Action.HeavyAttack` fa `20` di `DamageStructure`
([`RTHexCoverTests.cpp`](../../../Source/RefactorTactics/Tests/RTHexCoverTests.cpp)), quindi le sequenze reali
sono `High 50 → 30 → 10 → 0` e `Low 30 → 10 → 0`. Con ⅓ «critico» cade **sull'ultimo passo prima di zero su
entrambi i tipi**, cioè significa *un altro colpo e cade*. La misura **esclude ¼**: lì una `Low` a `10` resta
«ridotta» (`10 > 7`) e cadrebbe senza mai mostrare lo stato più forte, che su metà del catalogo non si
vedrebbe.

> ⚠️ **Ciò che la misura NON fa è selezionare ⅓ contro ½, e la prima stesura lo sosteneva.** Su queste due
> sequenze `Integrity * 2 <= Default` classifica **esattamente come** `* 3`: entrambe danno critico a `10` su
> `High` e su `Low`. Fra le due la scelta è ⅓ perché è la **più stretta** — «critico» resta raro, e il giorno
> in cui arrivasse un colpo più leggero di `20` continuerebbe a significare *l'ultimo colpo* invece di
> anticiparsi. È un argomento di design, non una misura, e chiamarlo «misurato invece che scelto» era
> vendere come discriminante un esperimento che discrimina solo contro ¼. Trovato in code review.

✅ **Chiuso da [`D-186`](../../decisions/RT_PDR_00_Decision_Log.md) il 2026-08-24, e i due produttori non erano
lo stesso difetto** — è la ragione per cui #1194 chiedeva *una* scelta e ne servivano **due**.

**Il dato — il costruttore.** `FRTHexCover` dichiarava `InIntegrity = 30` **fisso e indipendente dal `Type`**,
quindi `FRTHexCover(Edge, ERTHexCoverType::High)` nasceva a `30`, il **60%** del proprio catalogo. Accettava il
tipo e **ignorava** la funzione che sa cosa quel tipo vale. Ora il default deriva dal tipo, con sentinella
`UseCatalogIntegrity` (negativa, perché `0` è un'integrità legittima). ⚠️ Misurato prima di cambiarlo: degli
**undici** siti che omettono il parametro **uno solo** cambia valore, ed è un test che conta pannelli per
bordo.

🔴 **E il costruttore non è tutto il percorso: l'autoraggio dall'EDITOR resta aperto** — [#1317](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1317). Chi
aggiunge una entry `Covers` nel dettaglio di un `URTHexMapAsset` non passa dal costruttore a due argomenti: la
struct nasce da `FRTHexCover()`, cioè `Low`/`30`, e cambiare `Type` in `High` **non ricalcola niente**, perché
l'asset non ha un `PostEditChangeProperty`. `ValidateMap` non lo vede: la sua guardia è `Integrity <= 0`.
⚠️ La prima stesura di questo blocco scriveva che il costruttore era *«ciò che si ottiene aggiungendo a mano
una entry `Covers`»* — vero per il C++, **falso per il pannello dei dettagli**, che è il modo in cui si autora
davvero una mappa. Trovato nel panel della stessa PR.

⚠️ **Misurato il 2026-08-24, e il difetto non ha soggetto: in tutti i map asset versionati esiste UNA
copertura, e non è sotto catalogo.** `DA_HexMap_Arena` ne ha **zero** su 64 celle, `DA_HexMap_Sandbox` è
vuoto, `DA_HexMap_Scratch_Basin` ne ha **una** su 45 — e quella viene da una fixture C++, che passa da
`DefaultIntegrity(Type)`. **Nessuno ha ancora autorato una copertura a mano in questo repository**, ed è la
ragione per cui il difetto è sopravvissuto invisibile: il percorso che lo produce non è ancora stato battuto.

∴ **il meccanismo NON si chiude, e la scelta è dichiarata invece che scoperta a valle.** Una guardia in
`ValidateMap` dovrebbe essere un *warning* — `D-186` dichiara **legittima** una copertura sotto catalogo,
`Adaptive` nasce a `25` di proposito — e un `PostEditChangeProperty` rischierebbe di riscrivere un valore
voluto: si pagherebbero entrambi per zero casi. Al loro posto c'è un **oracolo** che li guarda:
`RefactorTactics.HexMap.AuthoredCoversAreNotBelowCatalog` ([`RTHexMapTests.cpp`](../../../Source/RefactorTactics/Tests/RTHexMapTests.cpp)),
che conta le coperture dei tre asset e fallisce sulla prima che nasca sotto il proprio catalogo.
⛔ **Rileva, non previene**: il giorno in cui un autore ne scrive una, il test lo dice — non glielo impedisce.
Se quel giorno arriva più di una volta, la guardia torna decidibile e la sede è [#1317](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1317).

**L'etichetta — il vocabolario.** `Hero.Riktor.KineticPanel.Adaptive` dichiara `Integrity` **25** contro un
catalogo `Low` di `30`, e quel numero è **voluto**: [`riktor.md`](../../characters/v0.1/riktor.md) dice che
*«scende a 25»*, la fragilità **è** il prezzo della rotazione gratuita. Lì il dato è giusto e a sbagliare era
la parola: *«danneggiato»* afferma che qualcuno l'ha colpita. Le soglie di `D-172` **non cambiano**; la
lettura si chiama ora **`ridotto`**, e le tre dichiarano **forza relativa al catalogo del tipo**, non una
storia di colpi:

```text
critico   Integrity * 3 <= DefaultIntegrity(Type)
ridotto   Integrity < DefaultIntegrity(Type)
intatto   altrimenti
```

⛔ **Le coperture già scritte in un `.uasset` non si toccano**: una `High` autorata a `30` continua a valere
`30`, e sotto questo vocabolario legge «ridotta» — che è **vero**. È anche la ragione per cui non si è scelta
la terza via di #1194, l'integrità di **nascita** registrata nella struct: costa un campo, una versione di
formato e una migrazione, **e non avrebbe chiuso il produttore 1** — una `High` nata a `30` risulterebbe
«intatta», cioè l'etichetta diventerebbe giusta e la copertura resterebbe più debole del 40% senza ragione.

⚠️ **Resta presentazione, e la frazione è ciò che lo garantisce**: la lettura non entra nel resolver, non
cambia la riduzione del danno — che è di `Combat/` — e non entra in `ComputeHash`. Se il balance muove
`DefaultIntegrity` o `DamageStructure`, le letture **seguono** senza che questa sezione vada riscritta. Un
numero assoluto avrebbe legato la presentazione a due costanti di gameplay, ed è la stessa ragione per cui
la dimension grammar di §6 è relativa e mai in centimetri.

### 7.3 «Acceso/spento» non basta, e il numero di stati non lo fissa questo documento

Il kit propone `Online/Offline` come «accent acceso/spento». È troppo poco, e l'owner degli elementi
interattivi lo mostra con un esempio di macchina a stati
([`../../gameplay/spec-interazioni-mappa-cp101.md`](../../gameplay/spec-interazioni-mappa-cp101.md) §5) in cui un
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
> roster ([`D-046`](../../decisions/RT_PDR_00_Decision_Log.md), `Hero.Phase.FluidTrail` **è**
> `Action.CreateWater`) e non serve un secondo modello per crearla»*. Modellarla ora non sarebbe lavoro in
> anticipo: sarebbe l'asset di un sistema che il progetto ha deciso di non costruire.

### 8.1 Dove vivono — deciso il 2026-08-18, `D-173`

```text
/Game/RT/World/Graybox/
  Cover/       SM_Graybox_Cover_Low · SM_Graybox_Cover_High
  Doors/       SM_Graybox_Door_Panel · SM_Graybox_Door_Locked
  Surfaces/    SM_Graybox_Surface_Water · SM_Graybox_Surface_Ice
  Volumes/     BP_Graybox_CellPlacementVolume
  Materials/   M_Graybox_Master · MI_Graybox_* (sei, una per mesh)
```

🔑 **`Materials/` è entrato il 2026-08-30 con [#1714](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1714)**, e non aggiunge un elemento al catalogo di §8: aggiunge ciò che *veste* i sei che già ci sono. Fino a quel giorno le mesh uscivano dal commandlet con lo **slot materiale vuoto** — il progetto aveva pagato un test (`Graybox.MeshesHaveFaceNormals`) per garantire che fossero ombreggiabili e poi non dava loro nulla che le ombreggiasse in modo distinguibile.

Il master ha **due** parametri — `BaseColor` e `Roughness` — e le sei istanze usano **grigi neutri** (`R == G == B`). Non è una rinuncia al colore: è ciò che rende vera *per costruzione* la verifica in scala di grigi che §10 impone, perché un kit senza canale cromatico non può affidare una categoria al solo colore. Il secondo canale resta dove §7 lo vuole: la geometria per gli oggetti, e la **ruvidità** dove la geometria non basta — è il caso di `SM_Graybox_Door_Panel` contro `SM_Graybox_Cover_High`, che il commandlet costruisce alte uguali (`0.85 H`) e lunghe uguali (`0.92` del lato), separate dal solo spessore che la vista a picco proietta quasi a zero.

🔑 **In che forma esistono — deciso il 2026-08-28, [`D-229`](../../decisions/RT_PDR_00_Decision_Log.md).** Le sei
`SM_Graybox_*` sono **generate in C++ e salvate** come `.uasset` da un commandlet del modulo editor: la
geometria vive in una funzione che si diffa e si testa, il file è il suo output. `D-173` aveva deciso **dove**
vivono e questa riga dice **come**, che è la domanda che restava.

> ⛔ **Non sono transienti come `ARTHexMapActor::GetCellPrismMesh`**, e la ragione non è di gusto: una mesh in
> `GetTransientPackage()` **non si serializza in un `.umap`**, quindi la scena di validazione perderebbe i suoi
> riferimenti alla riapertura. Il prisma della cella può permetterselo perché nasce a ogni avvio dentro
> l'attore che lo consuma; un asset posato in un livello salvato no.
>
> ⚠️ **E `D-228` non dice il contrario, per quanto lo sembri**: *«mesh generate in C++ … e nessun `.uasset`»* è
> vero della **board** — `Cells`, `SurfaceGlyphs`, `Relief`, `Blockers`, `EdgeFeatures` — che è la resa di
> sviluppo e cadrà con l'art pass. Il kit di questo catalogo sono **asset di mappa posati**, ed è un altro
> dominio: leggerla come regola del repository invece che del suo dominio porta alla conclusione opposta.

Sotto `World/` e non sotto `World/Grid/`: §5 di
[`convenzioni-contenuti-ue.md`](../tooling/convenzioni-contenuti-ue.md) descrive già `Grid/Generation/` come
*«generatori graybox»*, e porte e coperture stanno sui **bordi** (§3), non sulla griglia. Non un top-level
`/Game/RT/Graybox/`: quel livello è organizzato per **dominio**, e «graybox» è un modo di fare gli asset —
promuoverlo ad arte finale, sotto `World/`, è un rename locale.

⚠️ **Esistono due cartelle `Graybox`, e non è l'ambiguità che `D-173` rifiuta.** `Maps/Dev/L_DevSandbox/Graybox/`
è materiale graybox **locale a quella mappa**; questa è il **kit condiviso**. Le separa il criterio degli asset
di mappa già normativo in **§5b** — *«se è usato da più mappe, va in una cartella condivisa»* — perché la natura è
la stessa e cambia lo scope. Il caso scartato era diverso: generatori e oggetti sono cose di natura diversa, e
nessun criterio di scope li avrebbe separati. *Registrato dopo la code review su #1188, che ha notato la
collisione prima che qualcuno la incontrasse.*

⚠️ **La riga d'allowlist in `.gitignore` viene PRIMA del primo asset**, e c'è già:
[`asset-map.md`](../tooling/asset-map.md) §6 lo prescrive perché senza di essa `git add` **tace e non segnala nulla**.
Oracolo: `git check-ignore -q <file>` → exit **`1`**; con `-v` esce `0` in entrambi i casi e non distingue.

✅ **Il percorso rende committabile un asset, e l'oracolo lo conferma.** Misurato il 2026-08-28 con la
riga d'allowlist a `.gitignore:192`: `git check-ignore -q
Content/RT/World/Graybox/Volumes/BP_Graybox_CellPlacementVolume.uasset` esce **`1`**, cioè non ignorato.

🔴 **Questa riga diceva il contrario — «il percorso non rende committabile un asset oggi… si modella
alla scala nuova e si rimanda il commit, non il lavoro» — ed era la copia stale più dannosa delle sei**,
perché istruiva a NON committare un asset che è committabile: le altre invecchiavano un numero, questa
fermava del lavoro finito sulla soglia del repository. Falsa dal 2026-08-25, quando [#1155](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1155) è
atterrata e il mondo è passato a `HexSize = 150`. La scala d'arte di §6.2 e la scala di runtime sono ora
la stessa, e non c'è più niente da rimandare.

---

## 9. Quello che questo documento **non** decide

Sono domande aperte, e restano aperte. Vivono in [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), che è il
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
   ([`../../gameplay/spec-coperture-temporanee-cp95.md`](../../gameplay/spec-coperture-temporanee-cp95.md)) e
   non entra da qui.

### 9.2 Chiuse

Restano nominate qui perché il Decision Log, `OPEN_DECISIONS.md` e le issue continuano a citarle **per ID**:
questa tabella è la mappa `GBX-* → esito` nell'owner del contratto, così chi arriva da una `D-nnn` trova da
quale sezione la domanda nasceva. Un ID che sparisce dall'unico documento che lo definisce manda il lettore a
cercarlo altrove.

> ⚠️ *La prima stesura giustificava la sezione con «perché §7 e §8 le citano», e il **suo stesso diff** aveva
> reso quella frase falsa: le citazioni in §7.1 e §7.2 erano state sostituite con `D-171` e `D-172`, e §8 non
> ha mai nominato `GBX-4`. Misurato: `grep -n "GBX-[234]"` su questo file trova hit **solo** in §9.2. Vera
> del testo di prima, falsa del testo che la conteneva. Trovato in code review su #1188.*

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
[`scenario-map.md`](../tooling/scenario-map.md), e il loro registro è
[`test-manuali-pie.md`](../test-manuali-pie.md).

> ✅ **Le voci PIE di questo contratto sono scritte dal 2026-08-25**, e sono **sei**: `PIE-GBX-VOLUME`,
> `PIE-GBX-FIT`, `PIE-GBX-COVER`, `PIE-GBX-DOOR`, `PIE-GBX-SURFACE`, `PIE-GBX-ZOOM`
> ([#1096](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1096)). Vivono in
> [`test-manuali-pie.md`](../test-manuali-pie.md), che ne è l'owner — qui non si ricopiano — e la seduta
> **U25** le dichiara in `verifies:`, dove fino a quel giorno c'era `[]`.
>
> ⏳ **Scritte non è eseguite, ed è la distinzione per cui quel registro esiste.** Tutte e sei restano
> aperte, ma **la causa è cambiata il 2026-08-28**: `git ls-files 'Content/RT/World/Graybox/*'` dà ora
> **7** — le sei mesh di §8.1 più il volume di posa — quindi non aspettano più un asset, aspettano una
> **persona che guardi**. È la seduta **U25**, ed è l'unica cosa che abbiano mai aspettato davvero.
>
> 🔴 **Questa riga diceva «dà **0** … le sette mesh no», e conteneva anche un errore di conto**: gli
> asset di §8.1 sono **sei mesh più un Blueprint**, e il volume di posa non è una mesh. È stata
> riscritta quando la causa è decaduta, e il numero corretto insieme.
>
> ✅ **E una parte di questo contratto è ora difesa da un gate**, contro quanto l'apertura di §10
> dichiara: `RefactorTactics.Graybox.*` — cinque test nel modulo editor — lega gli `.uasset` alle
> frazioni di §6.3 **derivandole dal CDO invece di scriverle in uu**, e guarda i numeri che decidono
> le coppie: il fattore `2` in pianta fra le due coperture, la traversa su entrambe le facce, le sei
> quote del ghiaccio contro la lastra sola dell'acqua. Quello che resta senza gate è ciò che
> l'apertura dice davvero: l'oracolo di *«si legge»*, che non esiste nell'harness e non va simulato.
> Quello di *«è spesso `0.10` del lato»* esisteva, ed era una sottrazione.
>
> 🔑 **`PIE-GBX-FIT` è l'oracolo di `GBX-1`**, la domanda che §9 lascia aperta: la frazione di `C` che è il
> Safe Placement inset **si decide guardando**, in quella voce, e l'esito va riportato in
> [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md). E `PIE-GBX-DOOR` copre tutti e **quattro** gli stati,
> `Locked` compreso: `GBX-2` non è più una domanda dal 2026-08-18 (`D-171`, §7.1).
>
> ⚠️ **Il vincolo che le bloccava è decaduto il 2026-08-20.** Fino ad allora `test-manuali-pie.md` era
> assegnato a un'altra track del write-set di batch, e scriverci da qui violava `D-139`. Con
> [D-178](../../decisions/RT_PDR_00_Decision_Log.md) il sistema di lavoro parallelo è stato rimosso: non
> c'è più un proprietario da cui farsi cedere il path, e restava solo il lavoro.
>
> **Il nome dell'owner non si scriveva qui, e il paragrafo sotto dice perché**: era cambiato tre volte in un
> giorno, e ogni nome inciso in questa riga sarebbe invecchiato in silenzio. ⌫ *«Si legge nel batch»* non è
> più un'istruzione eseguibile: `parallel-batch.yaml` è uscito dal repository con
> [D-181](../../decisions/RT_PDR_00_Decision_Log.md), e non c'è più un batch da aprire.
>
> ⏱️ **La causa è cambiata tre volte in una sessione, e il vincolo non si è mai mosso.** All'apertura il
> file era in prestito a `playback` per `#1015`; poi `#1015` è stata **chiusa** mentre questo documento
> veniva scritto e il batch dichiarava ancora quella track `ACTIVE`; infine il rilascio è stato scritto e
> il path è **tornato a `playtest`**, che è `IDLE` — e in questo file `writable` su una track `IDLE`
> significa **prenotato**, non libero.
>
> *(Fino al 2026-08-20 questa riga diceva: «il proprietario è cambiato, il permesso no — la condizione
> di sblocco è che la track proprietaria rilasci il path». Conservata come provenienza: spiega perché le
> voci sono rimaste non scritte così a lungo, e non va letta come una procedura ancora in vigore.)*

Quello che una verifica dovrà mostrare, senza HUD e senza selezione:

```text
unità · copertura bassa vs alta · muro vs muro sfondato ·
porta aperta vs chiusa vs bloccata · acqua vs ghiaccio ·
intatto vs distrutto
```

a tre distanze di camera — ravvicinata, di gioco, tattica. **Se non è leggibile, si cambia la grammatica
prima di aggiungere altri asset**: è l'unica prescrizione del kit che questo documento adotta senza
emendarla.

⚠️ **Due delle sei coppie non sono osservabili in v0.1, e la voce che le guarda lo dichiara invece di
tacerlo**: `muro vs muro sfondato` (§8, elemento 10) e `intatto vs distrutto` (§8, elemento 9 — le macerie)
sono `DEFER` perché dipendono da `RT-FEAT-MAP-STRUCTURAL`, che è `IDEA` su release `future`. `PIE-GBX-ZOOM`
si esegue sulle **quattro** restanti e scrive che due sono fuori. Un contratto «verificato» su quattro coppie
su sei, letto come verificato, è il difetto che questa sezione esiste per non produrre.
