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
| **Pivot** | ✅ `CameraPivot`; la posizione dell'attore è `Pivot + PeekOffset` | `SetCameraPivot` |
| **Yaw** | ✅ `CameraYaw`, normalizzato in `[0,360)` | `RTCameraPawn.h` |
| **Pitch** | ✅ `CameraPitch`, clampato `[-89, 0]` **in codice** | `AddPitch` |
| **Zoom / Distance** | ✅ `SpringArm->TargetArmLength`, clampato `[MinArmLength, MaxArmLength]` | `ApplyViewSettings` |
| **ActiveLayer** | ✅ **sul controller**, non sulla camera (`D-255`) | `ARTPlayerController::ActiveLayer` |
| **Peek offset** | ✅ `PeekOffset`, limitato in lunghezza, rientra da sé | `SetPeekOffset` |
| **Strategic state** | ✅ derivato dalla distanza, con isteresi (`D-252`) | `UpdateStrategicState` |

✅ **Il pivot è esplicito dal 2026-08-30** ([#1770](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1770)), e non era cosmesi. Oggi «dove guarda la camera» e «dove sta il
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

### 3.2 Binding del modificatore `Alt` — ✅ consegnati

Owner: [#1771](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1771) (`CAM-02`) e [#1772](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1772) (`CAM-03`), chiusi il 2026-08-30.

| Gesto | Effetto | Stato |
|---|---|---|
| `Alt` + `LMB` **click** | pivot sulla cella valida sotto il cursore | ✅ non tocca la selezione |
| `Alt` + `LMB` **drag** | orbita | ✅ oltre la soglia click/drag |
| `Alt` + `RMB` drag | dolly | ✅ sopprime l'`UndoAction` per tutto il gesto |
| `Alt` + movimento senza tasti | peek | ✅ limitato, rientra da sé al rilascio |
| `Alt` + `MMB` drag | precision pan | ✅ `PrecisionPanScale` |
| `PageUp` / `PageDown` | `ActiveLayer` sopra / sotto | ✅ limitato ai layer che la mappa ha |
| doppio `LMB` | Select + Focus | ✅ [#1773](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1773) |
| `MMB` drag | **orbita**, invariata | ✅ `PIE-CAM-ORBIT` resta valida |
| `M` → Strategic Overview | — | ⏳ non cablato: lo stato esiste, la vista no (§5) |

> ✅ **`CAM-B` risposta il 2026-08-30: «entrambi».** `MMB` **resta** l'orbita e `Alt`+`LMB` è un secondo
> modo; il pan resta su `WASD`. La domanda era aperta perché il rebinding avrebbe invalidato
> `PIE-CAM-ORBIT`, verde dal 2026-08-16 e tasto per tasto — con questa scelta **quel prezzo non si paga**,
> e la voce resta valida senza essere riscritta.
>
> 🔵 **Un effetto collaterale c'è, ed è registrato**: `Alt`+`MMB` non poteva restare il precision pan
> «puro», perché `MMB` è già armato come orbita. Risolto dando la precedenza al modificatore — con `Alt`
> premuto, `MMB` **pana** invece di orbitare. È l'unico modo di avere entrambi senza un terzo tasto.

> ⚠️ **`BackSpace` è già occupato.** Il consolidamento chiedeva un `Back` che riporti allo stato camera
> precedente; `BackSpace` è oggi `UndoAction` insieme a `RMB`. Uno stack di stati camera resta **fuori dal
> vertical slice** e senza consumatore: non si assegna un tasto a una funzione che non esiste.

> 🔴 **Il test che difende i tasti non esisteva, ed era citato.** `GenericHotkeys()` dichiarava in un
> commento *«è un controllo che `PlayerInput.HotkeysDoNotCollide` rifà»*, e `grep -rn "DoNotCollide"
> Source/` rispondeva **una sola riga**: quel commento. L'unica difesa era un elenco scritto a mano in un
> altro commento. Ora il test esiste e interroga il `UInputMappingContext` **reale** — aggiungere un tasto
> è una riga in `BuildInputMappings`, e il controllo la vede da sé.

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
| doppio `LMB` | Select + Focus — ✅ [#1773](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1773), su unità propria, entro `DoubleClickInterval` |

`FocusOn` sposta il **pivot** e conserva yaw, pitch e distanza. Conserva anche la **quota del piano** e non
quella dell'attore: `ARTUnit` sta mezzo corpo sopra la cella (`VisualZOffset`), quindi chi inquadra
un'unità converte la sua cella invece di leggerne la posizione — è la correzione di
[#887](https://github.com/DegrassiAaron/refactor-tactics-main/issues/887), dove `F` mostrava il vuoto.

**Due regole, e vanno lette con il loro stato reale:**

1. ✅/⏳ **Un input manuale interrompe immediatamente** qualunque movimento automatico della camera. Chi
   tocca il mouse ha ripreso il comando, e finire l'interpolazione «perché era iniziata» è la forma più
   comune di camera che combatte contro il giocatore.
   ⚠️ **Ha oggi un solo consumatore**, ed è onesto dirlo: il rientro del peek, che è l'**unico** movimento
   automatico che esista — `UpdatePeekReturn` non fa nulla mentre `Alt` è premuto, cioè mentre il giocatore
   sta guidando. `FocusOn` è istantaneo, quindi non c'è ancora niente da interrompere. La regola è scritta
   **prima** della prima transizione interpolata, non dopo: è l'unico momento in cui costa poco.
2. **Ping, notifiche e UI ordinaria non rubano la camera.** Nessuna eccezione per «è importante»: se
   qualcosa merita davvero l'inquadratura, è un evento di Resolution e il suo owner è un Camera Director
   dedicato (§10), non una chiamata sparsa.

---

## 5. Zoom e Strategic View — [#1774](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1774)

Lo zoom è **continuo** ✅ e resta tale. **Non** esistono tre modalità rigide `Close / Tactical / Strategic`:
una modalità è uno stato in più da mantenere, e la distanza la sta già tenendo `TargetArmLength`.

✅ **Lo stato** esiste dal 2026-08-30 ([#1774](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1774)); ⏳ **la vista** no. La Strategic View è una
conseguenza semantica dello zoom, non un'altra camera:

```text
Distance ≥ StrategicEnterThreshold  →  entra in Strategic
Distance ≤ StrategicExitThreshold   →  esce
StrategicExitThreshold < StrategicEnterThreshold   (isteresi)
```

L'isteresi non è una raffinatezza: senza, una singola soglia fa oscillare la vista fra due presentazioni
diverse a ogni tacca di rotella nell'intorno del valore, ed è un difetto che si vede subito e si diagnostica
tardi.

I due valori sono **data-driven** (`StrategicEnterThreshold` / `StrategicExitThreshold`) e si tarano in
`L_CameraFeatureLab`. Nessun numero è proposto qui: un default scritto in una spec prima di essere provato
diventa canone per inerzia.

⚠️ **`Exit < Enter` è imposto in codice, non solo documentato.** I due campi sono `BlueprintReadWrite` e il
loro `meta = (ClampMin)` vincola il Details, non un `Set` da Blueprint: `UpdateStrategicState` li ordina
con `Max`/`Min`, così chi li inverte ottiene comunque un'isteresi valida invece di uno stato che entra e
non esce. Coperto da `Camera.StrategicThresholdsAreOrderedInCodeNotOnlyInDocs`.

⏳ **Nessun consumatore visivo.** Lo stato è leggibile (`IsStrategicView`) e si annuncia nel log; cosa si
*mostri* in Strategic — separazione verticale dei piani, densità dei marker — è §6 e resta da fare.

🔗 La Strategic View ha già una premessa documentale in
[`progettazione-hud.md`](progettazione-hud.md) §3.2 — *«Strategic Overview / Tactical Overview … non è la
camera di gameplay standard»* — e §30 nega la minimap permanente **anche perché** l'overview separata è
più leggibile. Le due righe si sostengono a vicenda; questo file possiede il **come**, quello il **se**.

---

### 5.1 `ZoomAlpha` — la sorgente unica — ✅ [#1856](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1856)

La sorgente **esiste** dal 2026-08-31: `ARTCameraPawn::GetZoomAlpha()` / `SetZoomAlpha()`, con 4 test
`RefactorTactics.Camera.*` e verifica di mutazione superata. Questa sezione diceva *«al 2026-08-30 una
sola derivazione»* e *«`grep -c ZoomAlpha` risponde zero»*: era vero **a quella data**, e non lo è più —
il grep risponde `3` nel solo `RTCameraPawn.cpp`.

| Consumatore | Deriva dallo zoom? | Come, oggi |
|---|---|---|
| velocità di pan | ✅ **dalla sorgente** | `Lerp(MinArmLength, MaxArmLength, GetZoomAlpha()) / DefaultArmLength` |
| pitch | 🟡 **canale aperto, curva piatta** | `DefaultPitch + PitchZoomDelta * GetZoomAlpha()`, con `PitchZoomDelta = 0` di **default**: in partita l'inclinazione è identica a prima, e la curva si tara in [#1780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1780) |
| FOV | ❌ no | `ViewportHorizontalFov` viene **letto** dalla camera, mai scritto |
| span dei layer | ❌ non esiste | §6 |
| alpha della UI tattica | ❌ non esiste | [#1835](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1835) |

⚠️ **Il difetto che la sorgente ha chiuso**, tenuto qui perché è la ragione per cui esiste: la vecchia
derivazione ad-hoc non era normalizzata e non aveva tetto — il rapporto valeva `0.125` a zoom minimo e
`5.0` a zoom massimo. Non era quella riga a essere sbagliata: era che **la prossima curva sarebbe stata
scritta allo stesso modo**, accanto al proprio consumatore, e sarebbero state due formule che nessuno
poteva confrontare né tarare insieme.

✅ **Misurato**: `GetZoomAlpha()` normalizza sui **limiti reali** —
`(TargetArmLength - MinArmLength) / (MaxArmLength - MinArmLength)`, clampato `[0..1]` e monotòno — e
`SetZoomAlpha()` è la porta di scrittura equivalente, che riallinea anche `bStrategicView`. I consumatori
derivano da lì invece di rileggere il braccio.

🔑 **Normalizzare sul `DefaultArmLength` è l'errore che i test presidiano**, e non è teorico: darebbe un
valore che vale `1` a metà corsa e supera `1` oltre — un rapporto, non una posizione. La verifica di
mutazione del 2026-08-31 l'ha applicata e **tutti e quattro** i test sono caduti, con `alpha` che a braccio
minimo valeva `0.125` = `Min/Default`.

⏳ **Resta prescritto** il canale di debug che [#1834](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1834)
dichiarava: nessun `UE_LOG` sul cambio di alpha e nessun consumatore fuori dal pawn, quindi la taratura di
#1780 si farebbe **a occhio**.

⛔ **Cosa la sorgente NON autorizza.** Un `alpha` normalizzato rende comodo scrivere il FOV dallo zoom, e
non va fatto per inerzia: `ViewportHorizontalFov` alimenta `ComputeEffectivePivotBounds` (`D-251`), quindi
scriverlo **sposta i limiti del pivot**. È un cambio ai bounds travestito da rifinitura visiva, e va fatto
con i test di [#1778](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1778) davanti.

🔗 Milestone **v0.1** per `D-286` — la sorgente è promossa, i suoi consumatori visivi no.

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

> ✅ **Il piano attivo del giocatore esiste dal 2026-08-30**: `ARTPlayerController::ActiveLayer`,
> `StepActiveLayer` su `PageUp`/`PageDown`, limitato ai layer che la mappa **contiene davvero** —
> misurati sulle celle, non assunti `[0, N]`, perché una mappa dipinta a mano può avere buchi e salire a
> un piano vuoto darebbe un hover che non trova mai niente senza dire perché.
>
> 🔑 **Vive nel controller e non nella camera**, e la sede è parte di `D-255`: da quella decisione
> l'`ActiveLayer` determina *quale cella un click seleziona*, quindi entra nel gameplay. Metterlo su
> `ARTCameraPawn` avrebbe reso la camera un'autorità sull'esito — cioè avrebbe violato `D-143` nel commit
> che dichiara di rispettarlo.
>
> ⚠️ **Non è `ARTHexMapActor::ActiveLayer`**, che ha lo stesso nome e un altro mestiere: quello e
> `ERTLayerViewMode` (`AllLayers` · `ActiveOnly` · contorno) sono strumenti di **authoring**, guidati
> dall'editor mode. Due scrittori su un campo solo sarebbero un difetto che si manifesta in editor e si
> diagnostica in partita.
>
> ⏳ **Resta da fare la presentazione**: indicatori sopra/sotto e separazione verticale in Strategic. Il
> filtro del velo va attraversato **prima** di decidere cosa disegnare, non dopo.

---

## 7. Picking — [#1776](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1776), e la domanda aperta `CAM-A`

Il raycast fisico **non determina da solo la cella logica**. La pipeline prescritta è:

```text
World Mouse Ray → World Hit / candidate position → X/Y candidate
  → ActiveLayer → FRTCellId(X, Y, ActiveLayer) → logical map lookup
```

Sulle stesse coordinate X/Y possono coesistere tunnel, terreno, ponte e tetto. La camera **non deve**
selezionare un layer diverso da `ActiveLayer` solo perché il raggio ha colpito una mesh di quel layer.

> ✅ **Deciso e implementato il 2026-08-30** — `D-255`, uscita (b) di `CAM-A`. La regola qui sopra **è**
> il comportamento: `ARTPlayerController::ResolveCellUnderCursor` calcola `bHasValidHit` come *«il colpo è
> sul piano attivo»* e passa la palla alla funzione pura.
>
> ⚠️ **La decisione tocca il gameplay ed è registrata come tale**, non è entrata dalla porta della
> presentazione: cambia quale cella un click seleziona, quindi quale cella si pianifica. Il comportamento
> superato era dichiarato in chiaro nel codice — *«Il layer viene dalla QUOTA del punto colpito»* — e non
> era una svista: era una scelta, che su X/Y con tunnel · terreno · ponte · tetto lasciava decidere il
> piano a ciò che il raggio incontrava per primo.
>
> ➕ **Hover e click passano ora dalla stessa funzione.** Erano due call site che chiamavano
> `WorldToCellId` per conto proprio (`RTPlayerController.cpp:504` e `:736`), e nulla garantiva che
> evidenziassero e selezionassero la stessa cella.
>
> ⏳ **Il feedback a schermo manca**, ed è un debito dichiarato: cliccando la mesh di un piano diverso il
> giocatore vede selezionarsi una cella che non ha puntato, e oggi ha solo un `UE_LOG` per capire perché.

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

## 10. Camera Director — [#1781](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1781): design **risposto**, implementazione **ancora backlog**

Eventi eccezionali della Resolution possono meritare un'inquadratura dedicata. **Non ora**: il core camera
non è chiuso, e un director che sposta la vista sopra uno stato che non sa ancora dire dove guardava prima
è il modo più diretto per rendere la camera imprevedibile. Resta backlog post-v0.1.

> 🔄 **Aggiornato il 2026-08-30 da [`D-287`](../../decisions/RT_PDR_00_Decision_Log.md).** La riga qui sopra
> resta vera e non è stata riscritta. Ciò che è cambiato è **un'altra cosa**: le quattro domande di design
> che `#1781` teneva in testa a sé — e che questa sezione dava per aperte — **hanno una risposta d'autore**.
> Il gate che cambia stato è quello di **design**, non quello di **implementazione**: `CAM-12` non si
> promuove a v0.1 e non si comincia.

### 10.1 La grammatica ha tre decisioni, e `KEEP` è il default

| Decisione | Quando | Costo |
|---|---|---|
| **`KEEP`** | **default** — l'evento è già leggibile | la camera **non si muove** |
| **`REFRAME`** | l'evento non è leggibile e non è critico | piccolo pan tattico, se serve un piccolo aggiustamento di zoom |
| **`HARD FOCUS`** | **raro** — evento esplicitamente classificato critico | inquadratura dedicata |

🔴 **L'ordine delle domande non è invertibile.** Prima *«è già leggibile?»*; solo se la risposta è no
entrano in gioco priorità e criticità. Un director che consultasse prima la priorità produrrebbe un taglio
per ogni azione importante — cioè il difetto che questa issue esiste per evitare, non la feature.

Candidati a `HARD FOCUS`, e nient'altro senza una decisione: finestra di **Fast Reaction** con scelta viva ·
**KO** decisivo · reaction/counter importante · swing di obiettivo importante.

⚠️ **Niente cambi automatici di yaw/pitch nella baseline.** L'orbita resta un gesto del giocatore
(`CAM-B`, §3.2); una regia che ruotasse da sola invaliderebbe `PIE-CAM-ORBIT` e riaprirebbe una domanda già
chiusa.

⛔ **`CAM-04` ha la precedenza, e vale già adesso** ([#1773](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1773)):
un input manuale di pan/zoom/orbit/peek **interrompe subito** l'automazione. Ping, notifiche e UI ordinaria
**non rubano mai la camera**; l'inquadratura degli eventi di Resolution è di questa issue, non di una
chiamata a `FocusOn` sparsa in un widget.

### 10.2 Le quattro domande — risposte il 2026-08-30, e da dove viene ciascuna risposta

| # | Domanda | Risposta | Dove vive |
|---|---|---|---|
| 1 | quali eventi meritano un'inquadratura, e chi lo decide | li classifica **`CAM-12`** per interesse/criticità di presentazione, su eventi risolti **già sanificati**; `KEEP` se già leggibile. La priorità di presentazione **non è** priorità di gameplay | questa sezione, §10.1 |
| 2 | cosa succede se due eventi la meritano nello stesso segmento | il **segmento di risoluzione** non si rompe: vicini condividono un'inquadratura, lontani possono essere gruppi consecutivi **purché la UI dica che sono dello stesso segmento**; tie-break deterministico | [`../../gameplay/spec-anima-risoluzione.md`](../../gameplay/spec-anima-risoluzione.md) §14.4 |
| 3 | cosa vede chi **non** ha diritto a quell'informazione | **nulla di derivato da quell'evento**: l'evento non entra nella projection passata al Director, quindi non può causare pan né focus | [`conoscenza-parziale-visibile-spec.md`](conoscenza-parziale-visibile-spec.md) §1.3 |
| 4 | il risultato logico può dipendere dal tempo reale | **no** — e non è solo una regola: il gate esiste già, `RefactorTactics.Match.Autobattle.DeterminismIsIndependentOfPlayback` | [`../../gameplay/spec-anima-risoluzione.md`](../../gameplay/spec-anima-risoluzione.md) §14.3, §14.5 |

⚠️ **La risposta 3 non si implementa nascondendo.** Filtrare in camera **non è** un confine di sicurezza: il
dato non autorizzato non deve arrivare al client. Nascondere a valle ciò che si è già replicato lascia il
leak e lo rende invisibile.

### 10.3 🔴 Cosa blocca ancora `CAM-12`, e non è prudenza generica

Tre dipendenze, misurate il 2026-08-30 e ciascuna con la propria ragione:

1. **`CAM-04` — [#1773](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1773), aperta.** Senza
   la regola d'interruzione manuale il giocatore combatte contro la camera nel momento in cui gli serve di
   più. Il doppio click è consegnato; l'interruzione ha **un solo** consumatore e non esiste ancora una
   transizione interpolata da interrompere.
2. **`CAM-01` — [#1770](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1770), ✅ chiusa.** Il
   pivot esplicito esiste: c'è un posto dove tornare dopo l'inquadratura. **Questa dipendenza è soddisfatta.**
3. 🔴 **La projection sanificata non esiste — [#1525](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1525), aperta.**
   È la dipendenza che nessuno aveva scritto. `FRTResolvedEvent` porta `TWeakObjectPtr<ARTUnit>`
   `Source`/`Target` **senza filtro di conoscenza**, e `ResolvedTimeline` non è filtrata per squadra: un
   Director costruito sopra quella timeline **erediterebbe** il leak del playback invece di evitarlo, e la
   risposta 3 di §10.2 resterebbe una frase. È la ragione per cui il gate di design aperto non basta a
   sbloccare l'implementazione.

⚠️ **E la taratura non ha ancora un posto**: i ritmi cinematici sono `PROPOSED FOR PLAYTEST`
([`../../gameplay/spec-anima-risoluzione.md`](../../gameplay/spec-anima-risoluzione.md) §14.2) e si tarano
in [#1780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1780), che non è iniziata.

### 10.4 Il primo slice, quando sarà autorizzato

Cinque beat — `PlanningEnter · ReadyTension · ResolutionLaunch · ResolutionNormal / CriticalImpact ·
ResolutionSettle` — e **solo** le tre decisioni di §10.1.

⛔ Fuori dal primo slice: kill cam · camera in terza persona sopra la spalla · un taglio a ogni `MoveStep` ·
coreografie che riducono la leggibilità tattica.

ℹ️ **E i tipi si cercano prima di crearli.** Misurato il 2026-08-30: `BoundaryId`, `StableEventId` e
`PresentationPriority` danno **0** occorrenze in `Source/`, mentre `ResolvedTimeline`, `FRTResolvedEvent`,
`ViewerPlaybackSpeed`, `URTPlaybackLibrary::EffectivePlaybackSpeed` e `ARTCameraPawn::FocusOn` esistono. Chi
implementa parte da quelli: i nomi proposti da un handoff esterno non sono simboli del repository.

### 10.5 Modalità utente — previste, non decise

`Cinematic Camera: Full / Reduced / Off` cambia **solo la presentazione**: mai il gameplay, mai la quantità
di informazione disponibile. ⚠️ **Non ha ancora un owner**: le opzioni di sensibilità e riduzione del
movimento sono la lacuna che l'epic **E49** dichiara aperta e non colma di lato — nessun Epic di gamefeel o
accessibilità esiste nel repository.

---

## 11. Come si verifica

| Livello | Stato |
|---|---|
| Automation | ✅ **28** test `RefactorTactics.Camera.*` (19 prima di E49) in [`RTCameraPawnTests.cpp`](../../../Source/RefactorTactics/Tests/RTCameraPawnTests.cpp) |
| PIE | ✅ `PIE-CAM-START` · `PIE-CAM-ORBIT` · `PIE-CAM-ZOOM-ANCHOR` · `PIE-CAM-BOUNDS` · `PIE-CAM-FOCUS` |
| Scenari | ❌ **zero** — [`scenario-map.md`](../tooling/scenario-map.md) lo dichiara, con la motivazione |
| Input | ✅ `PlayerInput.HotkeysDoNotCollide`, che **era citato e non esisteva** |

> ✅ **Misura registrata il 2026-08-30**: suite intera **`VALIDA · 1442/1442 completati, 0 fallimenti`**
> su `aedc4656` (il merge di `origin/main` nel branch di E49). Letta nel log e non dedotta dall'exit code —
> `Found 1442` in testa, `EXIT CODE: 0` in fondo, **zero** `Result={Fail}`, e i due numeri che coincidono,
> quindi la run non si è troncata.
>
> ⚠️ **La misura è sul merge, non sul branch isolato**, e la differenza non è formale: `origin/main` era
> avanti di 31 commit e aveva toccato `RTUnit.cpp`, che i test camera esercitano istanziando `ARTUnit`.
> Una misura sul branch isolato non avrebbe detto nulla su quell'interazione.
>
> 🔵 **Una run precedente — `1428/1428, 0 fail` — non è registrabile** e non è stata riclassificata dopo:
> `HEAD` era cambiato a run iniziata perché un'altra sessione aveva fatto checkout nella working directory
> condivisa (`D-222`). Una misura vale o non vale **prima** di sapere come è andata.

Una feature camera è **Done** solo se: funziona in Editor **e** in packaged build; non modifica autorità
gameplay; non altera LOS, targeting o pathfinding; regge un input rimappabile; gestisce l'interruzione
manuale; espone debug visuale o log dove serve; ha almeno un automation test o una voce PIE; non produce
leak di informazione; ed è verificata alle aspect ratio rilevanti quando la geometria dello schermo entra
nel risultato.

> ⚠️ **Gli scenari camera non si aprono con nomi inventati.** Il triage del 2026-08-14 ne aveva ricevuti
> **34** e ne ha recepiti **zero**, per una ragione che regge ancora: non seguono la convenzione degli
> ScenarioId del corpus, e uno ScenarioId è un nome pubblico. La convenzione va scritta prima dei nomi.
