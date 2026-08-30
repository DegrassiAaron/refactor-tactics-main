# Spec — Tactical Camera

> **Owner documentale** della camera tattica. `CURRENT` · normativo.
> Epic **E49** ([#1769](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1769)) · decisioni
> [`D-142`](../../decisions/RT_PDR_00_Decision_Log.md), [`D-143`](../../decisions/RT_PDR_00_Decision_Log.md),
> `D-250`–`D-254`.
> Codice: [`RTCameraPawn.h`](../../../Source/RefactorTactics/Camera/RTCameraPawn.h) ·
> [`RTPlayerController.cpp`](../../../Source/RefactorTactics/Player/RTPlayerController.cpp).
> I domini spaziali della mappa hanno un owner separato:
> [`spec-domini-spaziali-mappa.md`](spec-domini-spaziali-mappa.md).

Prima di questo file la camera esisteva **solo come codice e come decisioni sparse**: `D-142` fissava lo
snap, `D-143` la natura presentation-only, cinque voci `PIE-CAM-*` la provavano a mano, e
[`camera-roadmap-v1-triage-2026-08-14.md`](../../roadmap/plans/camera-roadmap-v1-triage-2026-08-14.md)
era un referto di consumo, non una specifica. Chi voleva sapere *cosa deve fare la camera* doveva leggere
`RTCameraPawn.h` — che lo dice bene, ma solo per ciò che già esiste.

> 🔴 **Questo documento distingue ovunque due cose che si confondono da sole**: ciò che il codice **fa
> oggi** (misurato, con il simbolo ✅ e il nome della funzione) e ciò che è **prescritto e non ancora
> scritto** (⏳, con la issue che lo possiede). Una spec che non separa i due piani diventa una lista di
> bugie ordinate.

---

## 1. Il principio, e cosa ne consegue

**La camera è presentation-only** (`D-143`). Non decide LOS, visibilità, targeting, copertura,
pathfinding, validità di cella o esito di simulazione. Il gameplay autorevole resta su `MapState`,
`FRTCellId`, il grafo tattico e i servizi dedicati.

```text
Camera / Render Geometry  ≠  Gameplay Authority
```

La conseguenza operativa non è ovvia e va scritta: **se una mesh visiva deve bloccare davvero la LOS o il
movimento, non basta che sia lì**. Deve avere una rappresentazione corrispondente nei dati autorevoli della
mappa. Un muro che esiste solo come geometria è scenografia; un muro che blocca è una cella o un edge nel
modello. Costruire il primo e aspettarsi il secondo è il modo più economico di rompere il determinismo
senza accorgersene.

> ⚠️ **`D-143` dichiara il proprio debito, e resta aperto.** `ARTCameraPawn::FrameOwnTeam` itera **tutte**
> le unità del mondo e legge `TeamId`/`IsAlive()` anche degli avversari, per scartarli. «Non modifica lo
> stato» è vero; «legge una vista già sanificata» **non lo è**. Non produce leak a schermo — inquadra solo
> le proprie — ma la proprietà che vorremmo è più forte di quella che abbiamo.

---

## 2. Stato della camera

La camera è **prospettica quasi-isometrica**, non ortografica: `USpringArmComponent` + `UCameraComponent`,
nessun `ProjectionMode` impostato. Questo importa più di quanto sembri — `ZoomTowards` documenta che la sua
formula è *esatta in ortografica e approssimata in prospettiva*, ed è la ragione per cui la sua tolleranza è
mezza cella e non zero.

Lo stato canonico è questo:

| Campo | Oggi | Dove |
|---|---|---|
| **Pivot** | ⏳ implicito: è la posizione dell'attore pawn | [#1770](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1770) |
| **Yaw** | ✅ `CameraYaw`, normalizzato in `[0,360)` | `RTCameraPawn.h` |
| **Pitch** | ✅ `CameraPitch`, clampato `[-89, 0]` **in codice** | `AddPitch` |
| **Zoom / Distance** | ✅ `SpringArm->TargetArmLength`, clampato `[MinArmLength, MaxArmLength]` | `ApplyViewSettings` |
| **ActiveLayer** | ⏳ **non esiste lato gioco** — `ARTHexMapActor::ActiveLayer` è authoring | §6 · [#1775](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1775) |
| **Peek offset** | ⏳ non esiste | §3.2 · [#1772](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1772) |
| **Strategic state** | ⏳ non esiste | §5 · [#1774](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1774) |

⏳ **Il pivot deve diventare esplicito**, e non è cosmesi. Oggi «dove guarda la camera» e «dove sta il
pawn» sono la stessa variabile, quindi qualunque offset temporaneo — un peek, una transizione, uno shake —
si scriverebbe *sopra* il riferimento invece che *accanto*, e al rilascio non ci sarebbe niente a cui
tornare. È lo stesso difetto che `#863` ha già pagato una volta sul pitch: `CameraPitch` faceva da default
**e** da stato corrente, e finché nessun input lo toccava non si vedeva.

---

## 3. Input

⚠️ **L'input della camera è interamente C++**: `ARTPlayerController::BuildInputMappings` costruisce
`UInputAction` e `UInputMappingContext` con `NewObject`, tutti `Transient`. Non ci sono asset Blueprint di
input da versionare, e un cambio di binding è un diff leggibile in una PR — non un `.uasset`.

### 3.1 Binding attuali — misurati

| Comando | Tasto | Funzione |
|---|---|---|
| Pan | `W` `A` `S` `D` | `AddPlanarMovement`, **relativo alla vista** |
| Zoom | rotella | `ZoomTowards` (ancorato al cursore), ripiego al centro fuori mappa |
| Rotazione a scatti | `Q` / `E` | `AddYaw`, passo `YawStep = 45°` |
| Orbita continua | **MMB tenuto** + movimento | `AddOrbit` (X → yaw, Y → pitch) |
| Select | `LMB` | contratto del puntatore ([#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705)) |
| Cancel | `RMB` · `BackSpace` | `UndoAction` |
| Home | `Home` | `RecenterView` |
| Focus | `F` | `FocusOn` sulla cella dell'unità selezionata |

### 3.2 Binding prescritti — ⏳ non implementati

Owner: [#1771](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1771) (`CAM-02`, i binding) e [#1772](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1772) (`CAM-03`, i gesti).

`Alt` diventa il **modificatore camera**, e separa i gesti di vista da quelli di gioco:

| Gesto | Effetto | Nota |
|---|---|---|
| `Alt` + `LMB` **click** | imposta il pivot sulla cella valida sotto il cursore | non tocca la selezione gameplay |
| `Alt` + `LMB` **drag** | orbita (X → yaw, Y → pitch) | yaw libero, pitch clampato |
| `Alt` + `RMB` drag | dolly / zoom continuo | ⚠️ deve **sopprimere** il `RMB` di gioco per tutto il gesto |
| `Alt` + movimento senza tasti | *temporary peek* | offset visuale, limite ridotto e configurabile, rientro progressivo al rilascio |
| `Alt` + `MMB` drag | precision pan | sensibilità inferiore al pan normale |
| `MMB` drag | pan | **collide con l'orbita attuale** — vedi sotto |
| `PageUp` / `PageDown` | `ActiveLayer` sopra / sotto | richiede §6 |
| `M` | Strategic Overview | tasto **libero**, verificato sull'elenco dei `MapKey` |
| doppio `LMB` | Select + Focus | **zero** occorrenze di `DoubleClick` in `Source/` |

> 🔴 **Il rebinding di `MMB` non è un'aggiunta, è un cambio — e ha un costo già pagato.**
> `PIE-CAM-ORBIT` in [`test-manuali-pie.md`](../test-manuali-pie.md) è **verde dal 2026-08-16** e descrive
> l'orbita tasto per tasto: *«tenendo il tasto centrale del mouse e trascinando…»*. Quella verifica ha
> anche chiuso una decisione aperta (il verso di `bInvertOrbitPitch`, provato con le mani). Spostare
> l'orbita su `Alt`+`LMB` **invalida quella voce**, e il lavoro non è finito finché la voce non è
> riscritta. Chi implementa il rebinding senza toccarla lascia un verde che descrive comandi che non
> esistono più.

> ⚠️ **`BackSpace` è già occupato.** Il prompt di consolidamento chiedeva un `Back` che riporti allo stato
> camera precedente; `BackSpace` è oggi `UndoAction` insieme a `RMB`. Uno stack di stati camera è
> comunque **fuori dal vertical slice** e resta senza consumatore: non si assegna un tasto a una funzione
> che non esiste.

### 3.3 Click contro drag

Un gesto deve produrre **una** operazione. `Alt`+`LMB` è click *oppure* drag, mai entrambi, e la
discriminante è una **soglia configurabile** in pixel: sotto la soglia al rilascio è un click (set pivot),
oltre la soglia è un'orbita e il click non si emette più.

La soglia è un campo, non un letterale: la distanza che il pollice di una persona percorre senza volerlo
non è una costante universale, e il valore giusto si tara in `L_CameraFeatureLab` (§9).

---

## 4. Focus e transizioni — [#1773](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1773)

Il focus deve essere **poco invasivo durante il Planning**: chi sta pianificando ha costruito
un'inquadratura, e portargliela via è peggio che non aiutarlo.

| Comando | Effetto |
|---|---|
| `LMB` singolo | Select — ✅ non muove la camera |
| `F` | Focus — ✅ `FocusOn`, **conserva lo zoom** |
| doppio `LMB` | Select + Focus — ⏳ non esiste |

`FocusOn` sposta il **pivot** e conserva yaw, pitch e distanza. Conserva anche la **quota del piano** e non
quella dell'attore: `ARTUnit` sta mezzo corpo sopra la cella (`VisualZOffset`), quindi chi inquadra
un'unità converte la sua cella invece di leggerne la posizione — è la correzione di
[#887](https://github.com/DegrassiAaron/refactor-tactics-main/issues/887), dove `F` mostrava il vuoto.

⏳ **Due regole prescritte che il codice non ha:**

1. **Un input manuale interrompe immediatamente** qualunque movimento automatico della camera. Chi tocca
   il mouse ha ripreso il comando, e finire l'interpolazione «perché era iniziata» è la forma più comune
   di camera che combatte contro il giocatore.
2. **Ping, notifiche e UI ordinaria non rubano la camera.** Nessuna eccezione per «è importante»: se
   qualcosa merita davvero l'inquadratura, è un evento di Resolution e il suo owner è un Camera Director
   dedicato (§10), non una chiamata sparsa.

---

## 5. Zoom e Strategic View — [#1774](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1774)

Lo zoom è **continuo** ✅ e resta tale. **Non** esistono tre modalità rigide `Close / Tactical / Strategic`:
una modalità è uno stato in più da mantenere, e la distanza la sta già tenendo `TargetArmLength`.

⏳ **La Strategic View è una conseguenza semantica dello zoom**, non un'altra camera:

```text
Distance ≥ StrategicEnterThreshold  →  entra in Strategic
Distance ≤ StrategicExitThreshold   →  esce
StrategicExitThreshold < StrategicEnterThreshold   (isteresi)
```

L'isteresi non è una raffinatezza: senza, una singola soglia fa oscillare la vista fra due presentazioni
diverse a ogni tacca di rotella nell'intorno del valore, ed è un difetto che si vede subito e si diagnostica
tardi.

I due valori sono **data-driven** e si tarano in `L_CameraFeatureLab`. Nessun numero è proposto qui: un
default scritto in una spec prima di essere provato diventa canone per inerzia.

🔗 La Strategic View ha già una premessa documentale in
[`progettazione-hud.md`](progettazione-hud.md) §3.2 — *«Strategic Overview / Tactical Overview … non è la
camera di gameplay standard»* — e §30 nega la minimap permanente **anche perché** l'overview separata è
più leggibile. Le due righe si sostengono a vicenda; questo file possiede il **come**, quello il **se**.

---

## 6. Multilayer — [#1775](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1775)

La mappa è multilivello e l'identità di cella è `FRTCellId(X, Y, Layer)`.

**In Tactical View** un solo `ActiveLayer` geometrico è pienamente operativo. Gli altri piani possono
essere presenti come indicatori sopra/sotto — ⚠️ e **nessun indicatore può rivelare informazione non
autorizzata**: un marker che segnala «c'è qualcosa sotto» su una cella che l'osservatore non conosce è un
leak, e il velo di squadra ([`D-242`](../../decisions/RT_PDR_00_Decision_Log.md)) è il filtro che decide
cosa si vede.

**In Strategic View** più piani possono comparire insieme con una separazione verticale **puramente
visuale**: la quota a schermo cresce per leggibilità, le celle restano dove sono nel modello. `ActiveLayer`
continua a esistere anche lì — non è la vista a definirlo.

> ⚠️ **`ActiveLayer` esiste, ma non è questo.** `ARTHexMapActor::ActiveLayer` e `ERTLayerViewMode`
> (`AllLayers` · `ActiveOnly` · contorno) sono strumenti di **authoring**, guidati dall'editor mode:
> `RTHexEditorClick.cpp` li usa perché lì si dipinge su un piano scelto. In partita nessuno li guida.
> Il piano attivo *del giocatore* è una cosa nuova, e chi lo implementa deve decidere dove vive — camera,
> controller o presenter — invece di riusare il campo dell'attore mappa perché ha il nome giusto.

---

## 7. Picking — [#1776](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1776), e la domanda aperta `CAM-A`

Il raycast fisico **non determina da solo la cella logica**. La pipeline prescritta è:

```text
World Mouse Ray → World Hit / candidate position → X/Y candidate
  → ActiveLayer → FRTCellId(X, Y, ActiveLayer) → logical map lookup
```

Sulle stesse coordinate X/Y possono coesistere tunnel, terreno, ponte e tetto. La camera **non deve**
selezionare un layer diverso da `ActiveLayer` solo perché il raggio ha colpito una mesh di quel layer.

> 🔴 **Questa regola contraddice il comportamento attuale, e la contraddizione è deliberata e non ancora
> risolta.** `RTPlayerController.cpp:502` dichiara l'opposto in un commento esplicito: *«Il layer viene
> dalla QUOTA del punto colpito: cliccando il ponte si evidenzia la cella del ponte (in editor lo decide
> invece `ActiveLayer`, perché lì si dipinge su un piano scelto)»*.
>
> **Cambiarlo tocca il gameplay**: cambia quale cella un click seleziona, quindi quale cella si pianifica.
> Non è una modifica di presentazione e non entra da questa spec. Serve prima l'`ActiveLayer` di gioco
> (§6), e la decisione va presa insieme al **Pointer Interaction Contract**
> ([#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705)), che possiede la matrice
> `oggetto × contesto × click → esito` e che dichiara il redesign della camera fuori dal proprio scope.
> Due sedi per la stessa decisione sono peggio di nessuna.

ℹ️ La metà **pura** della risoluzione esiste già e ha due test:
`URTHexLibrary::ResolveRayToCellOnLayer` usa la cella del colpo quando il colpo è valido e ripiega sul
piano attivo quando non lo è (`Hex.ResolveRayToCellOnLayerUsesValidatedHit`,
`…FallsBackToActivePlane`). Il pezzo mancante non è la matematica: è **chi decide** che un colpo su un
altro piano non è valido, e in partita non lo decide nessuno.

---

## 8. Occlusione — [#1779](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1779)

`SpringArm->bDoCollisionTest = false` ✅, nel costruttore. Non è una dimenticanza: la collisione della
SpringArm accorcia il braccio quando qualcosa entra fra camera e pivot, e su una board fitta questo
produce un cambio di zoom continuo che il giocatore non ha chiesto. `#864` lo ha dichiarato fuori scope e
la riga va tenuta.

⏳ **L'occlusione si risolve in presentazione**, non muovendo la camera:

- roof cutaway — il tetto sparisce quando la vista entra nell'edificio;
- wall fade / dissolve — le pareti fra camera e soggetto diventano trasparenti;
- local bridge fade;
- vegetation fade;
- terrain cutaway per i tunnel.

La collisione può tornare **solo** come safety contro compenetrazioni assurde, mai come sistema primario.

```text
Camera Occlusion  ≠  Gameplay LOS
```

Rendere trasparente un muro non lo rende attraversabile dalla vista **di gioco**: la LOS resta quella dei
servizi autorevoli, e un cutaway che cambiasse ciò che un'unità vede sarebbe una violazione di `D-143`,
non una feature.

> ℹ️ **Zero occorrenze** di `cutaway`, `fade` o `dissolve` in `Source/` al 2026-08-30: questa sezione
> descrive lavoro intero, non un miglioramento di qualcosa.

---

## 9. Camera Feature Lab — [#1780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1780)

⏳ `L_CameraFeatureLab` — **una** mappa parametrica, non una per feature. Le mappe che esistono oggi sono
`L_DevSandbox`, `L_HexArena`, `L_Prototype` e `L_Frontend`: nessuna è una scena di prova della camera, e
moltiplicarle costa più della disciplina di tenerne una con zone dedicate.

Zone previste: Open Field · Map Bounds · Scenic Buffer · Occlusion · Multilevel · Tunnel · Bridge ·
Planning · Resolution · Perception/Sound · casi limite ultrawide.

---

## 10. Camera Director — fuori scope v0.1, [#1781](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1781)

Eventi eccezionali della Resolution potrebbero meritare un'inquadratura dedicata. Non ora: il core camera
non è chiuso, e un director che sposta la vista sopra uno stato che non sa ancora dire dove guardava
prima è il modo più diretto per rendere la camera imprevedibile. Resta backlog post-v0.1.

---

## 11. Come si verifica

| Livello | Stato |
|---|---|
| Automation | ✅ **19** test `RefactorTactics.Camera.*` in [`RTCameraPawnTests.cpp`](../../../Source/RefactorTactics/Tests/RTCameraPawnTests.cpp) |
| PIE | ✅ `PIE-CAM-START` · `PIE-CAM-ORBIT` · `PIE-CAM-ZOOM-ANCHOR` · `PIE-CAM-BOUNDS` · `PIE-CAM-FOCUS` |
| Scenari | ❌ **zero** — [`scenario-map.md`](../tooling/scenario-map.md) lo dichiara, con la motivazione |

Una feature camera è **Done** solo se: funziona in Editor **e** in packaged build; non modifica autorità
gameplay; non altera LOS, targeting o pathfinding; regge un input rimappabile; gestisce l'interruzione
manuale; espone debug visuale o log dove serve; ha almeno un automation test o una voce PIE; non produce
leak di informazione; ed è verificata alle aspect ratio rilevanti quando la geometria dello schermo entra
nel risultato.

> ⚠️ **Gli scenari camera non si aprono con nomi inventati.** Il triage del 2026-08-14 ne aveva ricevuti
> **34** e ne ha recepiti **zero**, per una ragione che regge ancora: non seguono la convenzione degli
> ScenarioId del corpus, e uno ScenarioId è un nome pubblico. La convenzione va scritta prima dei nomi.
