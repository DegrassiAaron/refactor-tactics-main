# LOS e proiettili leggono la geometria intra-cella (`#1830`) — spec panel sulla issue come specifica

> `CURRENT` · **Stato**: revisione chiusa, definizione `DNNN` consegnata alla issue, implementazione nello
> stesso run ·
> **Data**: 2026-09-01
> **HEAD della revisione**: `a5a4162f` (= `origin/main` al 2026-09-01)
> **Oggetto**: la issue [`#1830`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1830) letta
> **come specifica di implementazione** per `D-269` e `D-270`.
> **Panel**: Wiegers (lead) · Fowler · Nygard · Adzic · Crispin · Cockburn
> **Modo**: critique · **Focus**: requirements, architecture, testing

---

## 1. Il verdetto in una riga

La issue è **implementabile così com'è scritta**, e i suoi sei acceptance criteria reggono. Ma tre cose che
decidono l'implementazione **non sono nel testo**, e una quarta che il testo suggerisce è **una trappola**:

| | Rilievo | Chi | Gravità |
|---|---|---|---|
| **R1** | «il modello unico di `D-270`» letto come *«riusa `ClassifyIntraCellTraversal`»* dà risposte **sbagliate** per la vista | Fowler | 🔴 critico |
| **R2** | `FRTHexInteriorWall` **non entra in `ComputeHash`**, con una motivazione scritta che questa issue rende falsa | Nygard | 🔴 critico |
| **R3** | la **corda** che attraversa la cella non è definita da nessuna parte: senza, l'AC 1 non è verificabile | Adzic | 🟡 bloccante per i test |
| **R4** | `Low` occlude o no? Il testo non lo dice, e l'implementatore sceglierebbe per inerzia | Wiegers | 🟡 |
| **R5** | il proiettile (`LineCells`) **già oggi diverge** dalla LOS sui bordi, e non è questa issue | Newman/Cockburn | 🟢 da scorporare |

---

## 2. R1 — «un solo modello» non vuol dire «una sola funzione»

**FOWLER**: il testo della issue dice *«il modello unico che `D-270` chiede: la geometria intra-cella si
interroga da un posto solo, e la posa (`URTHexCoverPlacementLibrary`) è già l'altro lato di quello stesso
modello»*. Letto di fretta, questo autorizza la scorciatoia più ovvia: la cella ha già
`URTHexCoverPlacementLibrary::ClassifyIntraCellTraversal(Mask, FromWedge, ToWedge)`, che risponde
*«si passa da questo settore a quest'altro?»*. Sembra la stessa domanda. **Non lo è.**

Quella funzione risponde per **connettività**: due settori stanno nella stessa regione libera se esiste un
percorso fra loro *girando attorno* alla geometria — ed è la definizione giusta, perché è quella che
`RTHexCoverPlacementLibrary.h` dichiara di volere (*«chi sta dietro un raggio centro-vertice non è
sull'altra faccia, ma ci arriva girando attorno all'estremo»*).

**La vista non gira attorno a niente.** Un raggio centro→vertice lascia i settori connessi — una sola
regione libera, `SameRegion` — e taglia in due la retta che passa per il centro della cella. Con
`ClassifyIntraCellTraversal` quel muro sarebbe **trasparente**, ed è esattamente il caso che `D-269`
esiste per non lasciare passare.

∴ **Il modello unico è la rappresentazione, non il predicato.** `InteriorWalls` e la grammatica di
`FRTGeometrySegment` sono l'unica autorità sulla geometria intra-cella, e si interrogano da un posto solo;
sopra quella rappresentazione vivono **due predicati diversi**, perché le domande sono due:

| Domanda | Predicato | Chi lo consuma |
|---|---|---|
| *«ci si sposta da qui a lì dentro la cella?»* | connettività di regioni | posa, calpestabilità (`D-289`) |
| *«questa retta arriva dall'altra parte?»* | **incidenza segmento con segmento** | LOS, proiettile (`D-269`) |

Questo **soddisfa** `D-270` invece di violarlo: la decisione chiede *«un modello di interrogazione»*, non
una funzione sola, e la ragione che porta — *«non uno per il movimento e uno per la vista»* — è contro due
**rappresentazioni** parallele, che è precisamente ciò che qui non succede.

⚠️ **Va scritto nella issue**, perché il testo attuale si presta alla lettura sbagliata e la lettura
sbagliata compila, passa i test esistenti e produce un muro che non ferma niente.

---

## 3. R2 — l'hash, e la riga di commento che questa issue rende falsa

**NYGARD**: `RTHexMapAsset.h:22` dichiara, sopra `FRTHexInteriorWall`:

> 🔑 **NON entra in `ComputeHash`** […] il movimento è **cella-a-cella** e un muro che sta dentro una cella
> non ne blocca nessuno. […] ⛔ **Non tocca le regole, e non è una svista**: vista e passo oggi non lo
> consultano. **Il giorno in cui un muro interno dovrà bloccare la linea di vista, quella è una decisione di
> gioco e va scritta come tale — e allora, ma solo allora, questo tipo entrerà nell'hash.**

Quel giorno è questa issue. `D-269` è la decisione, ed è già accettata.

Se `InteriorWalls` resta fuori dall'hash **dopo** che la LOS lo consulta, due mappe che si giocano in modo
diverso hanno lo stesso `ComputeHash`. Le conseguenze non sono estetiche e sono già cablate:

1. `IsSnapshotStale` lascia **fresco** uno snapshot in cache dopo che un muro interno si è spostato — e lo
   spostamento cambia chi vede chi;
2. l'hash di mappa entra nell'hash di stato (`RTMatchStateHash`), quindi la divergenza di replay diventa
   **non diagnosticabile**: due esiti diversi, stesso hash. È l'inverso del KPI `replay divergence = 0` —
   non un falso positivo, un **falso negativo**, che è la metà peggiore.

∴ **Requisito, non miglioria.** E va aggiornato anche il commento: un commento che spiega perché un campo
resta fuori, mentre il campo è dentro, è debito che il prossimo lettore paga.

⚠️ Attenzione al criterio del repo, che è coerente e va rispettato: nell'hash entra ciò che può **cambiare
un esito**. Quindi entrano `Cell`, `Segment` (asse, offset, estremi, layer) e `WallType`; **`StableId` no**,
con lo stesso argomento con cui `FRTHexDoor::StableId` invece ci entra — nessuno risolve un muro interno
per nome a runtime, e il campo esiste per l'editor.

🟢 **Nessun bump di `FormatVersion`**: nessun campo nuovo viene serializzato, e i passi di formato del repo
sono dichiarativi. Cambia il **valore** dell'hash per gli asset che hanno muri interni — cioè, misurato
oggi, **nessun asset versionato**: `grep -rl InteriorWalls Content/` non trova niente.

---

## 4. R3 — quale retta? La corda va DICHIARATA, o l'AC 1 non è testabile

**ADZIC**: l'AC 1 dice *«un segmento interno fra chi tira e chi è mirato blocca la linea di vista»*. Fra?
La LOS di questo progetto **non è una retta**: `URTHexLibrary::HexLine` produce una sequenza di **celle**, e
`DescribeLineOfSight` cammina i passi. Dentro una cella non esiste, oggi, nessun luogo geometrico su cui un
segmento possa stare «in mezzo». Finché non lo si dichiara, l'AC non ha un test: due implementatori
onesti scriverebbero due fixture diverse e avrebbero entrambi ragione.

La definizione che il panel raccomanda, perché è l'unica che resta **discreta** e non introduce una seconda
geometria accanto a quella che il progetto ha già:

```text
cella INTERMEDIA  :  EdgeMid(lato d'ingresso)  ->  EdgeMid(lato d'uscita)
cella del TIRATORE:  Center                    ->  EdgeMid(lato d'uscita)
cella del BERSAGLIO: EdgeMid(lato d'ingresso)  ->  Center
```

Tre proprietà, tutte e tre necessarie:

- **gli estremi sono anchor.** `ERTAnchorKind` li nomina già tutti e tredici (`Center`, sei `Vertex`, sei
  `EdgeMid`), quindi la corda si dice nel vocabolario che `D-288` ha appena chiuso, e non se ne inventa uno;
- **è simmetrica.** `HexLine(A,B)` e `HexLine(B,A)` percorrono le stesse celle e le stesse corde al
  contrario: la LOS resta indipendente dall'ordine, che è un vincolo di `D-269`;
- **è approssimata, e va detto.** La retta euclidea fra i due centri non passa per i punti medi dei lati
  quando la linea «gira»; questa corda sì. È la stessa classe di approssimazione che la LOS cella-a-cella
  già accetta da sempre, ed è coerente con essa — ma è un'approssimazione **dichiarata**, non un dettaglio
  d'implementazione da riscoprire.

**CRISPIN**: con la corda dichiarata l'AC 1 diventa una fixture di tre righe e un'asserzione. Senza, è una
frase.

---

## 5. Il determinismo si può avere ESATTO, e senza float

**FOWLER**: la issue chiede *«determinismo e interi dove applicabile»*. Misurato: qui *applicabile* vuol
dire **tutto**, e conviene saperlo prima di scrivere una `FVector2D`.

Ogni anchor di cella ha coordinate locali della forma `(a·r3, b)` in unità `R/4` — dove `r3` sta per la
radice di tre e `a, b` sono interi. Vertici: `(±2·r3, ±2)` e `(0, ±4)`. Punti medi di lato: `(±2·r3, 0)` e
`(±r3, ±3)`. Ogni punto della grammatica è `Perp·Offset/12 + Along·AlongQ/12` su due punti notevoli della
stessa forma: scalando di `12`, resta `(A·r3, B)` con `A, B` interi.

Il prodotto vettoriale fra due vettori di quella forma è

```text
cross( (A1·r3, B1), (A2·r3, B2) )  =  r3 · (A1·B2 − A2·B1)
```

— cioè un **intero moltiplicato per una costante positiva**. La radice non si calcola mai: si semplifica.
Tutti i test di orientamento e tutti i casi collineari sono esatti in `int64`, con valori piccoli
(`RT_GeometryMaxQuanta = 48`). Nessun epsilon, nessuna dipendenza dal `HexSize`, nessuna differenza fra
piattaforme.

∴ La primitiva di occlusione è **aritmetica intera pura**. Se l'implementazione finisce per confrontare
`FVector2D`, ha preso la strada sbagliata: `ToPolyline` esiste per **disegnare**, ed è la §11 di
`spec-hex-geometry-authoring.md` a dirlo.

---

## 6. R4 — `Low` occlude?

**WIEGERS**: il testo non lo dice, e la risposta non è deducibile dall'AC. È deducibile da `D-271`, che è
già accettata e che il panel considera vincolante:

> `Low` copertura direzionale parziale, `High` copertura/occlusione piena.

∴ **solo `WallType == High` occlude** vista e proiettile. Un `Low` resta copertura e non ferma la linea —
che è anche la simmetria giusta col bordo: `URTHexCoverLibrary::BlocksTraversal` nega l'attraversamento
per la copertura **alta**, non per il muretto.

⚠️ E il **layer**: `FRTGeometrySegment::Layer` va confrontato con quello su cui la linea sta ragionando, che
per la regola d'elevazione è `From.Layer` — la stessa che `RTHexVisionLibrary.h` documenta come *«la linea
resta sul layer del tiratore»*. Un muro interno su un altro piano non è in mezzo a niente.

---

## 7. R5 — la divergenza che c'è GIÀ, e che non è questa issue

**COCKBURN**: `URTOffensiveActionLibrary::LineCells` si ferma su `Data->bBlocksLineOfSight` — la proprietà
di **cella** — e **non consulta i bordi**. `DescribeLineOfSight` invece consulta entrambi. Quindi oggi, prima
di questa issue e senza alcun muro interno, una copertura alta di bordo **ferma la vista e non ferma la
linea di soppressione**: due risposte diverse alla stessa geometria, che è la classe di difetto che `D-269`
proibisce.

**NEWMAN**: chiuderlo qui sarebbe la cosa giusta da fare nel posto sbagliato. Cambia esiti di gameplay già
giocabili, tocca `ERTLineStop`, e non ha niente a che vedere con la geometria intra-cella: si porterebbe
dentro una regressione possibile sotto il nome di un'altra feature.

∴ **Follow-up scorporato.** Questa issue garantisce che la geometria **intra-cella** dia una risposta sola a
entrambe le domande, e lo garantisce **strutturalmente** — la stessa funzione, non due implementazioni con
un test di parità sopra. Il precedente è nel file che stiamo per toccare, ed è esplicito:

> Qui la parità non è asserita, è **strutturale**: esiste un solo attraversamento della linea, e il bool è
> `Reason == None`. Non c'è una seconda LOS da tenere allineata perché non c'è una seconda LOS.

---

## 8. Rischio d'implementazione che il panel segnala per nome

**NYGARD**: `URTHexLibrary::DirectionForEdgeIndex(k)` vale `(6 − k) % 6`. L'indice geometrico di lato e
l'ordinale di `ERTHexDirection` **non coincidono**, e girano in versi opposti. Chi costruisce la corda
trascrivendo a mano *«E è il lato 0, NE il lato 1»* sbaglia di segno su quattro direzioni su sei, e il
difetto è **simmetrico**: sulle mappe simmetriche i test passano lo stesso.

È lo stesso errore che `#1920` ha appena pagato due volte (`fix/1920-coordinate-parallele-al-lato`,
`fix/1920-glifi-ribaltati`, mergiate il 2026-08-31 e il 2026-09-01). ∴ la corrispondenza **si chiede** a
`EdgeIndexForDirection`, non si riscrive; e serve un test che la ancori su una direzione **asimmetrica**.

---

## 9. Il DoD che il panel consegna alla issue

Le sei voci della issue reggono e restano. Il panel ne aggiunge tre, tutte conseguenza dei rilievi sopra:

| | Voce | Da |
|---|---|---|
| **7** | `InteriorWalls` entra in `ComputeHash` (senza `StableId`), e il commento che diceva il contrario è aggiornato | R2 |
| **8** | la corda d'attraversamento è dichiarata nel codice e in `docs/`, con l'approssimazione detta | R3 |
| **9** | la primitiva è intera: nessun `float` nel percorso decisionale, e un test lo esercita su un `HexSize` diverso | §5 |

E una che il panel toglie dal perimetro:

| | Voce | Dove va |
|---|---|---|
| **—** | la divergenza di bordo fra `LineCells` e `DescribeLineOfSight` | scorporata in [#2035](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2035) (R5) |

---

## 10. Chiusura

La issue era già una buona specifica: aveva il *perché*, i non-goal scritti col loro argomento, e sei AC
verificabili. Ciò che le mancava era il ponte fra la decisione e il codice che esiste — e il ponte aveva
**due campate marce**: un riuso che sembra ovvio e non lo è (R1), e un campo fuori dall'hash con un
commento che si autodistrugge il giorno dell'implementazione (R2).

Entrambe sono ora nella definizione `DNNN` della issue.

---

## 11. Com'è finita *(chiuso il 2026-09-01, stesso giorno)*

| | Rilievo | Esito |
|---|---|---|
| **R1** | il riuso di `ClassifyIntraCellTraversal` | ✅ evitato, e la precisazione è scritta in `D-270` e nel commento di `URTHexOcclusionLibrary` |
| **R2** | l'hash | ✅ `InteriorWalls` entra in `ComputeHash` **e** in `URTMatchStateHash` — il secondo non era nel piano, ed è emerso dalla riconciliazione: `#986` aveva già pagato quel difetto su `bConductsElectricity` |
| **R3** | la corda | ✅ dichiarata in `RTHexOcclusionLibrary.h` e in `h6-4-hex-vision-spec.md` `D8` |
| **R4** | `Low` occlude? | ✅ no, con `Occlusion.LowWallDoesNotOcclude` a pinnarlo |
| **R5** | la divergenza di bordo | ✅ scorporata in [#2035](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2035), non silenziata |

**Tre cose che il panel non aveva previsto**, trovate implementando:

1. 🔴 **`URTMatchStateHash` è un secondo hash**, e saltava i muri interni mentre `ComputeHash` li mescola. Il panel aveva guardato solo il primo. L'ordinamento canonico ora vive in un posto solo (`URTHexMapAsset::SortInteriorWallsCanonically`) proprio perché due ordinamenti equivalenti sono il modo in cui i due hash tornerebbero a divergere.
2. 🔴 **Il readout d'editor cadeva nel `default`** e diceva `unavailable`: la ragione esisteva e il pannello la perdeva — cioè il DoD *«la ragione del blocco è esposta»* sarebbe stato verde nel runtime e falso a schermo.
3. ⚠️ **Il test `InteriorWallIsWhatStopsTheShot` sarebbe stato tautologico** nella forma del suo gemello: da quando la geometria entra nell'hash di stato, due partite che differiscono per un muro hanno hash diversi *comunque*. La discriminante vera è `High` contro `Low`, non muro contro niente.
