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

In tattica la camera è **prospettica quasi-isometrica**: `USpringArmComponent` + `UCameraComponent`,
e il `ProjectionMode` è prospettico in tattica e ortografico in strategica (§5, [D-498](../../decisions/RT_PDR_00_Decision_Log.md)). Questo importa più di quanto sembri — `ZoomTowards` documenta che la sua
formula è *esatta in ortografica e approssimata in prospettiva*, ed è la ragione per cui la sua tolleranza è
mezza cella e non zero.

> ✅ **[D-498](../../decisions/RT_PDR_00_Decision_Log.md) è attuata dal 2026-10-11 ([#3662](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3662)): in strategica la proiezione è ortografica**, dimetrica 2:1 con pitch −30°. La leva di [#3631](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3631) si è ritirata. In strategica la formula di `ZoomTowards` è quindi esatta.
>
> ✅ **[D-497](../../decisions/RT_PDR_00_Decision_Log.md), attuata in [#3661](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3661): la camera si rende nel rettangolo della scena, fra le fasce dell'HUD.**
> - **Chi lo decide**: `URTGameViewportClient::LayoutPlayers` scrive `Origin`/`Size` del giocatore mentre l'HUD tattico è montato. Proiezione, deproiezione e `ProjectWorldLocationToScreen` lo seguono da sé, perché passano da `ULocalPlayer::GetProjectionData`.
> - **L'estensione verticale non cambia**: con `MaintainYFOV` UE tiene fisso il semiangolo verticale `atan(tan(FOV/2) / AspectRatio)`, quindi a parità di braccio il rettangolo inquadra la stessa altezza di mondo dello schermo intero, e celle e unità appaiono circa 0,64×. Lo misura `Camera.SceneRectKeepsTheVerticalExtentOfTheFullScreen` sulla matrice di proiezione.
> - **Il limite del pivot** (`ComputeEffectivePivotBounds`, D-251) legge l'aspetto del rettangolo del giocatore (`RefreshViewportMetrics`), non `Viewport->GetSizeXY()`, e usa il modello di `MaintainYFOV`: semiasse verticale fisso, orizzontale che segue l'aspetto. A 16:9 i limiti sono quelli di prima; il difetto che `Camera.EffectivePivotBoundsShrinkWithZoomPitchAndAspect` teneva pinnato («il limite laterale non dipende dall'aspetto») è corretto.
> - **Un click fuori dal rettangolo non raggiunge il mondo**: la deproiezione di UE lo estrapolerebbe, quindi il controller passa da `DeprojectCursorInsideScene` (`Pointer.ScenePointRaysOnlyInsideTheRect`).
> - ✅ **`ViewportAspectRatio` si rinfresca a ogni scrittura di pivot** ([D-525](../../decisions/RT_PDR_00_Decision_Log.md), [#3798](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3798)): `SetCameraPivot` chiama `RefreshViewportMetrics` prima del clamp. Prima lo facevano solo zoom, rotazione, inquadratura e `BeginPlay`, e in una PIE avviata da zero, col solo pan, a `BeginPlay` il viewport non c'è: l'aspetto restava 0 e il limite era quello per sole celle (455–490 unità più in là del dovuto, misurato in PIE il 2026-10-18).
> - 🔑 **Il pivot sta nell'involucro convesso delle celle** ([D-525](../../decisions/RT_PDR_00_Decision_Log.md)): `ClampToSoftBounds` interseca il rettangolo di sempre con i semipiani dei lati dell'involucro (`ComputeConvexHull`), ciascuno spostato di `BoundsMarginCells` verso l'esterno e di `AllowedOutsideFraction` del **supporto del quadro nella sua normale** verso l'interno (`ComputeEffectivePivotPolygon`; per le normali degli assi il supporto è l'estensione di `ComputeEffectivePivotBounds`). La zona è contenuta nel rettangolo, quindi non ammette nulla che prima fosse escluso. Il rettangolo da solo ammetteva angoli a circa sei celle dalla cella più vicina, con lo schermo vuoto. Prove: `Camera.ConvexHullOfAHexagonOfCellsHasSixCorners`, `Camera.PivotRegionCutsTheCornersOfTheBoxAndNeverLeavesIt`, `Camera.PanTowardsACornerStopsWithinTheMarginOfTheCells`.
> - 🔑 **Oltre l'involucro il pivot non sta a più di metà di quanto il quadro arriva verso l'interno** ([D-526](../../decisions/RT_PDR_00_Decision_Log.md), `MaxPivotOutsideSupport`, `ComputeFrameFootprint`): `margine − 0,65·supporto` da solo, a braccio piccolo, portava il pivot oltre l'arrivo del quadro (scena vuota a braccio 450, misurato in PIE il 2026-10-18). L'impronta a terra è un trapezio, non un rettangolo simmetrico. `BoundsMarginCells` è un **massimo**; a braccio 800 il tetto non morde; è un tetto duro (vale anche con `AllowedOutsideFraction` alta); senza quadro noto non c'è tetto. ⚠️ Dove il tetto morde uno zoom indietro allarga il limite.

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
| Pan | `W` `A` `S` `D` | `AddPlanarVelocity` (a velocità, [#3792](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3792)) → `AddPlanarMovement`, **relativo alla vista** |
| Edge pan | cursore entro la banda dal bordo del viewport | `ApplyEdgePan` → `AddPlanarVelocity` → `AddPlanarMovement` ([#1838](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1838)), §3.4: acceso di default, spegnibile |
| Zoom | rotella | `ZoomTowards` (ancorato al cursore), ripiego al centro fuori mappa |
| Rotazione a scatti | `Q` / `E` | `AddYaw`, passo `YawStep = 45°`; in strategica `StrategicYawStep = 60°`, con l'imbardata allineata all'ingresso ([D-499](../../decisions/RT_PDR_00_Decision_Log.md)) |
| Orbita continua | **MMB tenuto** + movimento | `AddOrbit` (X → yaw, Y → pitch). Il pitch va nell'offset manuale, come `AddPitch`: dal 2026-10-10 il primo zoom non lo cancella più ([#3630](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3630), `Camera.OrbitPitchSurvivesTheNextZoom`). In strategica l'orbita non agisce, né la Y ([D-498](../../decisions/RT_PDR_00_Decision_Log.md)) né la X ([D-499](../../decisions/RT_PDR_00_Decision_Log.md)) |
| Select | `LMB` | contratto del puntatore ([#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705)) |
| Cancel | `RMB` · `BackSpace` | `UndoAction` |
| Home | `Home` | `FrameHome` ([#1837](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1837)): selezione propria viva → squadra → centro mappa, con orientamento e distanza di default (§4.1) |
| Focus | `F` | `FocusOn` sulla cella dell'unità selezionata |
| Vista strategica | `Tab` | `ToggleStrategicView`: sposta la distanza alla soglia e ritorno ([D-488](../../decisions/RT_PDR_00_Decision_Log.md), §5) |

### 3.2 Binding del modificatore `Ctrl` — ✅ consegnati

Owner: [#1771](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1771) (`CAM-02`) e [#1772](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1772) (`CAM-03`), chiusi il 2026-08-30.

| Gesto | Effetto | Stato |
|---|---|---|
| `Ctrl` + `LMB` **click** | pivot sulla cella valida sotto il cursore | ✅ non tocca la selezione |
| `Ctrl` + `LMB` **drag** | orbita | ✅ oltre la soglia click/drag; in strategica non agisce ([D-499](../../decisions/RT_PDR_00_Decision_Log.md)) |
| `Ctrl` + `RMB` drag | dolly | ✅ sopprime l'`UndoAction` per tutto il gesto |
| `Ctrl` + movimento senza tasti | peek | ✅ limitato, rientra da sé al rilascio |
| `Ctrl` + `MMB` drag | precision pan | ✅ `PrecisionPanScale` |
| `PageUp` / `PageDown` | `ActiveLayer` sopra / sotto | ✅ limitato ai layer che la mappa ha |
| doppio `LMB` | Select + Focus | ✅ [#1773](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1773) |
| `MMB` drag | **orbita**, invariata | ✅ `PIE-CAM-ORBIT` resta valida; in strategica non agisce ([D-498](../../decisions/RT_PDR_00_Decision_Log.md), [D-499](../../decisions/RT_PDR_00_Decision_Log.md)) |
| `Tab` → Strategic View | porta lo zoom oltre la soglia e ritorno ([D-488](../../decisions/RT_PDR_00_Decision_Log.md)) | ⏳ non cablato: lo stato esiste, la vista no (§5). ⌫ *Fino al 2026-10-09 questa riga diceva `M`, che dichiara `Sneak` ([D-456](../../decisions/RT_PDR_00_Decision_Log.md) punto 3, [#3470](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3470); il badge è cliccabile da [D-457](../../decisions/RT_PDR_00_Decision_Log.md))* |

> ⌫ **Fino al 2026-10-13 il modificatore era `Alt`, sinistro e destro.** Lo cambia
> [D-514](../../decisions/RT_PDR_00_Decision_Log.md), in [#3749](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3749):
> i gesti della tabella sono gli stessi, su `Ctrl`, e i due `Alt` non sono più mappati. I due blocchi qui
> sotto sono del 2026-08-30 e nominano il tasto di allora. La risposta «entrambi» non cambia: il secondo modo di
> orbitare è `Ctrl`+`LMB`, e con `Ctrl` premuto `MMB` pana invece di orbitare.
>
> ⚠️ **Non verificato a schermo**, e nessun test headless lo vede. Su una tastiera italiana `AltGr` arriva a
> Windows come `Ctrl` sinistro più `Alt` destro: armava il modificatore prima, passando dall'`Alt` destro, e lo
> arma ancora. E dentro l'Editor `Ctrl` tenuto più un tasto di gioco può essere un accordo dell'Editor. Sono un
> passo di `PIE-PC-GYM`.

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

Un gesto deve produrre **una** operazione. `Ctrl`+`LMB` è click *oppure* drag, mai entrambi, e la
discriminante è una **soglia configurabile** in pixel: sotto la soglia al rilascio è un click (set pivot),
oltre la soglia è un'orbita e il click non si emette più.

La soglia è un campo, non un letterale: la distanza che il pollice di una persona percorre senza volerlo
non è una costante universale, e il valore giusto si tara in `L_CameraFeatureLab` (§9).

### 3.4 Edge pan — [#1838](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1838)

Il cursore entro una banda dal bordo del viewport fa scorrere la vista verso quel bordo, tanto più veloce
quanto più è vicino al bordo. **Non è una seconda pipeline**: `ARTCameraPawn::ApplyEdgePan` chiama
`AddPlanarMovement`, quindi eredita la relatività allo yaw, la somma dal pivot, i limiti del viewport
(`D-251`) e la velocità di `WASD` allo stesso `ZoomAlpha`.

| Proprietà | Stato |
|---|---|
| l'asse è la penetrazione nella banda, zero fuori, e nessun pan col cursore fuori dal viewport; i quattro bordi arrivano alla penetrazione piena sull'ultimo pixel, `Size - 1` | ✅ `Camera.EdgePanAxisIsZeroOutsideTheBandAndGrowsWithPenetration` |
| relativo alla vista, negli otto orientamenti | ✅ `Camera.EdgePanFollowsTheViewYawInAllEightOrientations` |
| a penetrazione piena va come `WASD`, a due zoom | ✅ `Camera.EdgePanMovesLikeWasdAtTheSameZoom` |
| a riposo zero movimento e zero aggiornamenti; uscendo dalla banda si ferma nello stesso fotogramma | ✅ `Camera.EdgePanDoesNoWorkAtRestAndStopsTheSameFrame` |
| spento non si attiva in nessuna posizione | ✅ `Camera.EdgePanSwitchedOffNeverMoves` |
| dentro i limiti del viewport: arriva dove arriva `WASD`; contro il limite lavora e risponde che non ha mosso | ✅ `Camera.EdgePanStaysInsideTheViewportBounds` |
| rifiutato con un modale aperto, un'orbita o `Ctrl` tenuti, o l'interruttore spento; con un tasto del pan premuto vincono i tasti | ✅ `Camera.EdgePanIsRefusedDuringModalsAndCameraGestures`: il modale sulla regola, il resto dall'ingresso del controller |

- **L'interruttore** è `bEdgePanEnabled`, acceso di default (scelta dell'autore del 2026-10-15). È una proprietà
  `Config`, e si spegne da `Game.ini` sotto `[/Script/RefactorTactics.RTCameraPawn]` con `bEdgePanEnabled=False`.
- **Le precedenze.** «Modale» è `IsGameplayInputBlocked()`, la lettura unica del contratto del puntatore (#705).
  ⚠️ È un'eccezione voluta al suo commento, che dice che i modali non bloccano la camera: `WASD`, zoom e orbita
  restano vivi perché non toccano il piano, ma l'edge pan lo guida il cursore, e con un modale aperto il cursore
  sta sul modale. Rifiuta anche un gesto camera in corso (orbita, `Ctrl` tenuto). **Con un tasto del pan premuto
  vincono i tasti**: due ingressi nello stesso fotogramma sommerebbero il doppio della velocità di `WASD`.
- **Il focus.** Senza il viewport a fuoco non c'è cursore: `GetMousePosition` risponde con la posizione in cache,
  che sopravvive alla perdita del focus. Dopo un Alt-Tab col cursore sul bordo la camera continuerebbe a scorrere da
  sola, mentre `WASD` si ferma.
- **Il log** scrive le transizioni a `Verbose`: `[RT] Edge pan: attivo`, `fermo`, `rifiutato: modale aperto`,
  `rifiutato: gesto camera in corso`, `rifiutato: interruttore spento`, `rifiutato: i tasti del pan hanno la
  precedenza`. Lo stato è un enum, non un testo; a riposo il lavoro del controller è un asse calcolato e uno stato
  che non cambia.
- ⚠️ **Che `PlayerTick` lo chiami davvero, e il controllo del focus, non si provano headless**: senza viewport
  `GetMousePosition` risponde falso. Li copre la voce `PIE-CAM-EDGE-PAN`.
- ✅ **La velocità è al secondo, e non dipende dal frame rate** ([#3792](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3792)).
  `WASD` e edge pan passano da `AddPlanarVelocity(Axis, DeltaTime)`: `asse × PanSpeedPerSecond × DeltaTime`, con
  `PanSpeedPerSecond` in unità mondo al secondo a distanza di default (1500, cioè le 25 unità per fotogramma di
  prima a 60 fps). `AddPlanarMovement` resta **a spostamento** e non prende il tempo: lo usa il pan a trascinamento
  (`Ctrl`+`MMB`), pixel trascinati → distanza, e dividerlo per `DeltaTime` lo romperebbe. Provato da
  `Camera.PanCoversTheSameDistanceAtAnyFrameRate` (30, 60 e 144 fps, edge pan e `WASD` allo stesso profilo) e
  `Camera.PanAtSixtyFpsMovesAsItDidPerFrame`. ⏳ La sensazione a schermo a frame rate diversi non è stata vista: è di
  `PIE-CAM-EDGE-PAN`. ⌫ *Fino alla correzione questa riga diceva che la velocità era per fotogramma.*
- ⚠️ **La schermata finale non lo ferma**: `IsGameplayInputBlocked()` vale per la pausa e le impostazioni, mentre il
  risultato di fine partita è una schermata e non un modale. Se si decide che deve bloccarlo, si corregge lì, e
  l'edge pan segue.
- ⚠️ **Un pannello sotto il cursore non lo ferma**: il contratto del puntatore ha il contesto `Modal`, ma non un
  «cursore su un widget». Le fasce dell'HUD (#3661) stanno proprio ai bordi, quindi un pulsante a meno di una
  banda dal bordo fa anche scorrere la vista. Si giudica in PIE; se disturba, il rimedio è un'issue sul contratto
  del puntatore, non una seconda matrice qui.
- `N/A` **Il dominio di conoscenza** del criterio 6: era #1836, chiusa senza codice per D-373.
- ⏳ **Prescritto e non ancora visto**: la banda (`EdgePanBandPixels`, 16 px), la velocità e la curva si tarano in
  [#1780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1780). La banda è in pixel **fisici**: su un 4K al 200% vale metà di quella su un 1080p, e
  se scalarla col DPI è una domanda di #1780. Restano da fare la voce `PIE-CAM-EDGE-PAN` (seduta `U93`) e lo smoke packaged:
  il cursore al bordo si comporta in modo diverso a schermo intero.

---

## 4. Focus e transizioni — [#1773](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1773)

Il focus deve essere **poco invasivo durante il Planning**: chi sta pianificando ha costruito
un'inquadratura, e portargliela via è peggio che non aiutarlo.

| Comando | Effetto |
|---|---|
| `LMB` singolo | Select — ✅ non muove la camera |
| `F` | Focus — ✅ `FocusOn`, **conserva lo zoom** |
| doppio `LMB` | Select + Focus — ✅ [#1773](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1773), su unità propria, entro `DoubleClickInterval` |
| `Home` | Recupero contestuale — ✅ `FrameHome` ([#1837](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1837)), §4.1 |

`FocusOn` sposta il **pivot** e conserva yaw, pitch e distanza. Conserva anche la **quota del piano** e non
quella dell'attore: `ARTUnit` sta mezzo corpo sopra la cella (`VisualZOffset`), quindi chi inquadra
un'unità converte la sua cella invece di leggerne la posizione — è la correzione di
[#887](https://github.com/DegrassiAaron/refactor-tactics-main/issues/887), dove `F` mostrava il vuoto.

**Due regole, e vanno lette con il loro stato reale:**

1. ✅/⏳ **Un input manuale interrompe immediatamente** qualunque movimento automatico della camera. Chi
   tocca il mouse ha ripreso il comando, e finire l'interpolazione «perché era iniziata» è la forma più
   comune di camera che combatte contro il giocatore.
   ⚠️ **Ha oggi un solo consumatore**, ed è onesto dirlo: il rientro del peek, che è l'**unico** movimento
   automatico che esista — `UpdatePeekReturn` non fa nulla mentre `Ctrl` è premuto, cioè mentre il giocatore
   sta guidando. `FocusOn` è istantaneo, quindi non c'è ancora niente da interrompere. La regola è scritta
   **prima** della prima transizione interpolata, non dopo: è l'unico momento in cui costa poco.
2. **Ping, notifiche e UI ordinaria non rubano la camera.** Nessuna eccezione per «è importante»: se
   qualcosa merita davvero l'inquadratura, è un evento di Resolution e il suo owner è un Camera Director
   dedicato (§10), non una chiamata sparsa.

### 4.1 `Home` — il recupero contestuale — [#1837](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1837)

`Home` è il tasto del «riportami a un'inquadratura che conosco». Da #1837 sceglie **dove** guardare con
una gerarchia di ripiego, e azzera **sempre** orientamento e distanza: è la scelta dell'autore del
2026-10-15, *«Home resta il tasto di recupero: inquadratura contestuale e orientamento di default»*.

| Anello | Quando | Dove va il pivot | Stato |
|---|---|---|---|
| 1 | c'è una selezione, **viva** e della **propria squadra** | sulla cella dell'unità, alla quota del piano | ✅ `Camera.HomeFramesTheSelectedOwnUnit` |
| 2 | nessuna selezione utile, almeno un'unità propria viva | sul punto medio delle unità vive della propria squadra | ✅ `Camera.HomeWithoutSelectionFramesTheOwnTeam` |
| 3 | nessuna delle due, oppure la squadra non è nota, oppure la sessione è un'osservazione | sul centro della mappa: è `RecenterView`, il `Home` di prima | ✅ `Camera.HomeWithoutOwnUnitsFallsBackToTheMapCentre` · `Camera.HomeWithoutPlayerStateDoesNotGuessTheTeam` · `Camera.HomeWhileObservingFramesTheMapNotTheUnits` |

- ✅ **Misurato**: in tutti e tre gli anelli yaw, pitch, distanza (`DefaultArmLength` clampata) e peek tornano
  al default, e il pivot passa dalla porta unica con la vista **dopo** il reset
  (`Camera.HomeKeepsThePivotInsideTheViewportBounds`, in tattica). Il ramo della squadra **non** usa la
  distanza d'inizio partita di `FrameOwnTeam`: sono due inquadrature diverse. Il tasto passa davvero da
  `FrameHome` (`Camera.HomeKeyFramesTheControllersSelection`).
- ✅ **Dalla vista strategica** il pivot va alla quota **tattica** del piano: la board posata con i piani
  separati torna alla quota tattica solo al prossimo tick dell'HUD, e `ResetAndFrame` legge la quota che la
  board avrà dopo `Home` (`Camera.HomeFromStrategicFramesTheTacticalFloorHeight`).
- ✅ **In osservazione** — autobattle in vigore, o una run di scenario (`ARTPlayerController::IsObservationSession`) —
  `Home` resta il campo intero, l'inquadratura con cui `ARTGameMode::RecenterCameraOnScenario` apre uno scenario,
  e la posizione delle unità non la cambia (`Camera.HomeKeyInAScenarioFramesTheWholeMap`). È il ramo che #1837
  dava al `Replay`, applicato alle osservazioni che esistono.
- ⛔ **Senza `ARTPlayerState` la squadra non si indovina**: `TeamIdOf` ripiegherebbe sulla 0, e per un
  giocatore della 1 il cui stato non è ancora arrivato `Home` inquadrerebbe la squadra avversaria.
- ⛔ **`Home` non inquadra mai un'unità di un'altra squadra**, nemmeno passata come selezione e nemmeno
  quando è l'unica viva (`Camera.HomeNeverFramesAnotherTeamsUnit`). La guardia sta in `FrameHome`, dove si
  scrive il pivot; «la propria squadra» è quella di `FrameOwnTeam`, cioè `ARTPlayerState::TeamIdOf`.
- 🔎 Il log dice quale anello ha scelto (`[RT] Home: selezione | squadra | centro mappa`): dall'esito i tre
  non si distinguono sempre, e un ripiego sbagliato sembrerebbe un'inquadratura sbagliata.
- ⚠️ **Il debito di [D-143](../../decisions/RT_PDR_00_Decision_Log.md) resta aperto**: la squadra si
  ricava leggendo `TeamId` e `IsAlive()` di tutte le unità del mondo (`CollectOwnTeamCells`) e tenendo le
  proprie. Non è un canale finché il filtro è la squadra dell'osservatore; lo diventerebbe il giorno in
  cui `Home` inquadrasse qualcosa di non proprio.
- `N/A` **Il `Replay` di #1837, alla lettera**: il visore dei replay (`URTReplayViewerSubsystem`) è una
  schermata senza camera di mappa. La regola che la issue gli dava vale per l'osservazione, qui sopra.
- ⏳ **Prescritto e non ancora visto**: `PIE-CAM-HOME-CONTEXT`, i tre anelli a mano.

---

## 5. Zoom e Strategic View — [#1774](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1774)

Lo zoom è **continuo** ✅ e resta tale. **Non** esistono tre modalità rigide `Close / Tactical / Strategic`:
una modalità è uno stato in più da mantenere, e la distanza la sta già tenendo `TargetArmLength`.

✅ **Lo stato** esiste dal 2026-08-30 ([#1774](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1774)); ✅ **la vista** dal 2026-10-10, nel nucleo di [D-495](../../decisions/RT_PDR_00_Decision_Log.md). La Strategic View è una
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

✅ **Il consumatore visivo** (2026-10-10, [D-495](../../decisions/RT_PDR_00_Decision_Log.md)).
- `ARTHUD::UpdateObserverVeil` legge `IsStrategicView` nello stesso giro in cui scrive il velo, e lo consegna a ogni unità con `ARTUnit::SetStrategicPresentation`.
- In Strategic lo skeletal dell'eroe si spegne e si riaccende il cilindro segnaposto. Il flag entra **in AND** con `bRender` nei predicati, quindi non riaccende niente che il velo abbia spento (`Unit.StrategicPresentationSwapsTheBodyButNeverRevealsAVeiledUnit`, `Veil.StrategicViewDoesNotRevealAVeiledUnit`).
  - 🔑 **Il segnalino sta in piedi sulla cella** ([#3647](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3647)). Un `BP_Unit_*` d'eroe ha il pivot ai piedi e un cilindro alto 100 centrato lì, cioè per metà sotto la cella. In Strategic il cilindro prende la scala del segnaposto nativo e sale di `UnitHalfHeight − VisualZOffset`, come il segnalino del ricordo. Senza, dalla camera lontana il raggio del click lo mancava e prendeva la cella dietro (`PlayerInput.StrategicHeroTokenStandsOnTheCellAndTakesTheClick`). In Strategic i bracci graykit si spengono: sono ancorati ai piedi.
- Il ricordo diventa un segnalino con «X» (`ContactToken`) sulla cella del contatto, dalla stessa sorgente della sagoma (`Veil.StrategicRememberedContactIsATokenOnTheContactCell`).
- ✅ **Inclinazione e proiezione sono quelle di [D-498](../../decisions/RT_PDR_00_Decision_Log.md)** dal 2026-10-11 ([#3662](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3662)): ortografica, pitch fisso a `StrategicPitch` (−30°), per stato (sotto). La rampa di [#3630](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3630) e la leva di [#3631](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3631) si sono ritirate. 🔧 La separazione dei piani ha la sua leva (§6, [#3632](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3632)), e vale 3 dalla seduta `U78`.

🔑 **2026-10-09 — [D-488](../../decisions/RT_PDR_00_Decision_Log.md): come si entra e che cosa si vede.**
- **Come si entra** (✅ cablato il 2026-10-10, `ARTCameraPawn::ToggleStrategicView`). `Tab` porta il braccio a `StrategicEnterThreshold` e, al secondo `Tab`, lo riporta all'ultima distanza tattica. Da [#3646](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3646) quella distanza si registra **al momento del salto**: `FrameOwnTeam` apre la partita scrivendo il braccio senza passare da `UpdateStrategicState`, e il primo `Tab` tornava alla distanza di `BeginPlay` (`Camera.TabReturnsToADistanceWrittenOutsideTheZoomGate`). Lo stato resta derivato dalla distanza, quindi questa sezione non cambia: `Tab` è una scorciatoia, non una modalità.
  - ⚠️ **Il ritorno ha un tetto: mai oltre `StrategicExitThreshold`.** Se si è entrati con la rotella, l'ultima distanza tattica cade fra le due soglie, e tornarci lascerebbe la vista strategica per isteresi. D-488 diceva «la distanza da cui era partita» e non copriva questo caso: la regola sta in `Camera.TabAfterWheelEntryLeavesTheStrategicView`, e il caso base in `Camera.TabReachesTheStrategicThresholdAndTheSecondTabReturns`.
- **Che cosa si vede.** Al centro, la 3D tattica diventa un'**isometrica semplificata**, quella delle tavole A–F di [`hud-screens-2026-10`](../../research/design/hud/hud-screens-2026-10/SPECIFICA-ZONE.md). L'HUD intorno resta lo stesso.
- **Che cosa sposta.** Il ciclo della selezione di [D-421](../../decisions/RT_PDR_00_Decision_Log.md) passa da `Tab` a `N`, nello stesso commit che lega `Tab`. ✅ Fatto il 2026-10-10, e pinnato da `PlayerInput.StrategicViewIsOnTabAndSelectionCycleOnN`.
- **Che cosa resta aperto.** La proiezione esatta e le soglie si decidono in `L_CameraFeatureLab`.
  - ✅ **La proiezione e l'inclinazione le ha decise [D-498](../../decisions/RT_PDR_00_Decision_Log.md) (2026-10-11), attuata in [#3662](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3662)**: isometrica dimetrica 2:1, cioè ortografica con pitch fisso a −30° (`StrategicPitch`). Vale **per stato**: anche fra le due soglie, dove l'isteresi tiene la strategica, e dal fotogramma in cui lo stato cambia (`Camera.StrategicPitchAndProjectionFollowTheStateInBothHysteresisDirections`). Per questo ogni porta di gioco del braccio rilegge lo stato **prima** del pitch e del clamp del pivot, `FrameOwnTeam` compresa (`Camera.ArmGatesReadTheStateBeforeClampingThePivot`); fa eccezione la vista di misura `ApplyTopDownView`. In strategica l'orbita verticale non agisce, e l'offset manuale della tattica torna al ritorno (`Camera.FreeOrbitIsInertInStrategicAndTheTacticalTiltReturns`, `Camera.SecondTabRestoresTheWholeTacticalView`). La rotazione in strategica è a **60°** per [D-499](../../decisions/RT_PDR_00_Decision_Log.md), deciso dall'autore nella seduta `U79`: entrando l'imbardata si allinea al multiplo di 60° più vicino e uscendo torna quella tattica, e in iso l'orbita orizzontale libera non agisce (`Camera.YawSnapsTo60InStrategicAndTheTacticalYawReturns`, `Camera.SnapYawToStepRoundsToTheNearestMultiple`). In tattica resta [D-142](../../decisions/RT_PDR_00_Decision_Log.md), a 45°. Restano aperte le soglie.
  - 🔑 **`OrthoWidth` è l'estensione VERTICALE.** Col vincolo `AspectRatio_MaintainYFOV`, il default di `BaseEngine.ini`, UE usa `OrthoWidth / 2` come mezza altezza del quadro, e la prospettica tiene fisso il semiangolo verticale `atan(tan(FOV/2) / AspectRatio)`. Quindi `OrthoWidthForArm = 2 · Arm · tan(FOV/2) / AspectRatio`, con l'`AspectRatio` di riferimento della camera (16:9): al cambio di proiezione la scala sul pivot non salta, a ogni aspetto, e lo zoom resta il braccio (D-252). Lo prova `Camera.StrategicOrthographicMatchesThePerspectiveOnTheProjection` sulla matrice di proiezione che UE costruisce per il render, a 16:9, 21:9 e 16:10. ⚠️ La formula di [#3631](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3631) non divideva per l'aspetto e inquadrava 1,78× la prospettica; il suo test confrontava la formula con sé stessa (seconda review di [#3663](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3663)). 🔴 **I piani di clipping automatici di UE si spostano indietro di due bracci** (`StrategicOrthoPlaneShiftArms`): in ortografica il bordo basso del quadro parte quasi dal suolo, e senza lo spostamento la geometria più alta del pivot di circa `0,23 · Arm` spariva lì (547 alla soglia d'ingresso, contro i 750 del piano 1). Lo prova `Camera.StrategicOrthographicPlanesKeepTheMapLayersInView` sulla matrice di vista e proiezione di UE (review di [#3671](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3671)).
  - 🔑 **I limiti del pivot non hanno un ramo ortografico.** Con la correzione di [#3662](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3662) l'impronta ortografica sul piano del pivot coincide con quella prospettica, e `ComputeEffectivePivotBounds` modella quel rettangolo. ⚠️ Il modello è esatto solo a 16:9: con `MaintainYFOV` il semiasse fisso è il verticale (`Arm · tan(FOV/2) / 1,78`) e l'orizzontale segue l'aspetto. Si corregge quando il limite passa al rettangolo della scena ([#3661](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3661)).

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
| pitch | 🟡 **canale aperto, curva piatta** | In tattica `DefaultPitch + PitchZoomDelta * GetZoomAlpha()`, con `PitchZoomDelta` a `0` di **default**: in partita l'inclinazione tattica è identica a prima, e la curva si tara in [#1780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1780). In strategica il pitch è fisso a `StrategicPitch` (−30°, [D-498](../../decisions/RT_PDR_00_Decision_Log.md), [#3662](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3662)), e la rampa di [#3630](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3630) si è ritirata. |
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

⏳ **Resta prescritto** il canale di debug, ora [#1979](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1979)
(`CAM-13a`): nessun `UE_LOG` sul cambio di alpha e nessun consumatore fuori dal pawn, quindi la taratura di
[#1780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1780) si farebbe **a occhio**.

🔴 **E l'occhio qui sbaglia, con evidenza**: in `PIE-CAM-ZOOM-ALPHA` del 2026-08-31 l'inclinazione
*sembrava* muoversi con la rotella mentre `PitchZoomDelta` valeva `0` — era **parallasse**, e si è
sciolta solo leggendo il numero `Camera Pitch` nel Details. Chi tara la curva guardando la scena non la
distingue dal cambio di prospettiva.

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
> 🔧 **La separazione verticale in Strategic ha la sua leva dal 2026-10-11** ([#3632](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3632)):
> `ARTHexMapActor::StrategicLayerSeparation`, oppure `rt.Map.StrategicLayerSeparation` in console, che se > 0
> vince sulla proprietà. **Vale 2** ([D-501](../../decisions/RT_PDR_00_Decision_Log.md) (7)): l'ha scelto l'autore in isometrica, sulle catture della seduta PIE `U81` (2026-10-11, [#3678](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3678)), sui tre piani sovrapposti di `Visual.Map.LayerStack`. Prima valeva 3, scelto in prospettiva nella seduta `U78` sui due piani di `Visual.Map.MultiLevel`. `1` è neutro.
> - 🔑 **È la quota del contesto, non un secondo contesto.** `GetHexContext` è da allora il contesto della
>   *presentazione*: in Strategic la quota dei piani vale quella dell'asset per il fattore. Board, unità, playback,
>   overlay dell'HUD e picking la leggono dalla stessa porta, quindi restano allineati fra loro
>   (`HexMapActor.StrategicLayerSeparationPickingFollowsThePose`).
> - È la quota con cui la board è **posata**, e la cambia solo una ricostruzione intera. Una parziale (il velo che
>   insegue una revisione dell'asset) ripone le sue famiglie alla stessa quota delle altre, così la board non resta
>   mai mista (`HexMapActor.StrategicLayerSeparationSurvivesAPartialRebuild`).
> - Può farlo perché la simulazione non usa la quota. Misurato nel codice il 2026-10-11: i lettori fuori dalla
>   mappa la usano per posare i modelli su celle già decise. Lo fissa `HexMove.StrategicLayerSeparationKeepsTheOutcome`:
>   lo stesso turno, con e senza separazione, finisce con la stessa cella, la stessa salute, lo stesso TurnLog e lo
>   stesso hash.
> - ⛔ **Chi trasforma una posizione in una cella autorevole usa `GetAuthoringHexContext`**, cioè lo spawn da
>   marker e i template. Letto con la quota separata, un marker del piano 1 cadrebbe sul piano terra
>   (`HexMapActor.StrategicLayerSeparationLeavesSpawnCellsAlone`).
> - 🔴 **Il velo resta intero.** Cambiare quota ricostruisce la board, e la ricostruzione riporta ogni istanza alla
>   scala piena. Nello stesso fotogramma la mappa ridisegna il velo che mostrava, riletto dallo stato che il velo
>   stesso aveva scritto sul disco (`HexMapActor.StrategicLayerSeparationKeepsTheVeil`).
>   - ⛔ Non da una copia della conoscenza di squadra, che l'actor non conserva: è la regola di
>     [`D-242`](../../decisions/RT_PDR_00_Decision_Log.md) che `RebuildInstances` dichiara per `KnowledgeVolumes`.
>   - ⚠️ Come ogni ricostruzione, spegne `rt.Debug.Knowledge` e lo scrive nel log: va rilanciato. Non è il velo rotto.
> - 🔴 **Le unità salgono o scendono, e basta.** Le riposa il `TurnManager`, scalando la loro quota sopra il piano
>   terra: è lineare nella quota dei piani, anche a metà di una salita, e la pianta non si tocca
>   (`HexMove.StrategicLayerSeparationReposesStandingUnits`).
>   - A playback in corso `Cell` è già la cella **finale**, mentre il modello sta dove il playback lo ha messo:
>     all'inizio di un'animazione di una fase futura, alla fine di una passata. Riposarlo su `Cell` lo farebbe
>     saltare (`HexMove.StrategicLayerSeparationDuringPlaybackKeepsThePlan`).
>
> ⌫ *Fino al 2026-10-12 qui restavano «gli indicatori sopra/sotto» di [#1775](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1775). [D-501](../../decisions/RT_PDR_00_Decision_Log.md) li ha superati:
> gli altri piani non si segnalano, si **mostrano**, trattati rispetto a quello attivo.*
>
> 🎨 **I piani rispetto a quello attivo** ([D-501](../../decisions/RT_PDR_00_Decision_Log.md), [#3679](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3679)), in tattica e in strategica. I piani sopra sono traslucidi e schiariti ([#3674](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3674)).
> I piani sotto sono desaturati e scuriti, di più quanto più sono lontani. L'attivo ha i colori pieni. Le unità e i
> fantasmi del piano restano pieni. Una mappa a un piano si vede come prima.
>
> 🎨 **E il piano si legge dal bordo** ([D-508](../../decisions/RT_PDR_00_Decision_Log.md), [#3704](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3704)): nero sull'attivo, bianco sopra, viola sotto,
> lo stesso colore a ogni distanza. I bordi delle celle restano **opachi su ogni piano** e non ricevono il
> trattamento: un piano sopra è un riempimento traslucido con un contorno bianco netto. I fantasmi del piano hanno un
> contorno del colore del piano, e il riempimento resta la certezza. Su una mappa a un piano i bordi sono quelli di
> sempre, e i fantasmi non hanno contorno. I volumi lo dicono con una tinta ([#3706](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3706)): più sotto.
>
> 🎨 **E il piano di un'unità si legge dal contorno del modello** ([D-508](../../decisions/RT_PDR_00_Decision_Log.md), [#3705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3705)), in tattica sul
> modello e in strategica sul segnalino: nero sul piano attivo, bianco sopra, viola sotto. Su una mappa a un piano
> è nero per tutti. L'anello della squadra resta com'è: la squadra la dice l'anello, il piano il contorno.
> ⌫ *Fino a #3705 un'unità sotto il piano attivo, coperta da una sua cella, non si vedeva: il piano attivo è pieno.
> L'autore ha deciso, in seduta il 2026-10-12, che il contorno del modello si veda* **attraverso** *il piano.*
> - 🔑 **La tecnica: custom depth con uno stencil per relazione, e un solo post-process** (`RTUnitLayerOutline.h`).
>   Il corpo mostrato dell'unità si disegna anche nel custom depth con lo stencil della sua relazione, e
>   `M_UnitLayerOutline`, sulla camera di chi guarda, colora i pixel appena fuori dalla sagoma. Il costo è un
>   passaggio a schermo intero, qualunque sia il numero di unità. Serve `r.CustomDepth=3` (`DefaultEngine.ini`).
>   Scartato il guscio invertito per mesh: resta dietro la geometria che copre l'unità.
> - 🔴 **Attraverso la geometria passa solo il contorno di chi sta SOTTO il piano attivo.** È il caso che l'autore
>   ha visto: col mezzanino attivo, l'unità sul pavimento sotto di esso. Per le unità del piano attivo e di quelli
>   sopra il materiale confronta la profondità dell'unità con quella della scena, e il contorno si ferma dove un
>   muro la copre. Si disegna il solo contorno, non una sagoma piena. Scelte della PR, senza l'autore: stavano
>   nelle tavole che ha giudicato il 2026-10-13, e non le ha contestate.
> - ⛔ **Privacy: chi non si vede non ha contorno, e nemmeno una quota.** Lo stencil lo decide l'HUD dalla vista di
>   chi guarda (`ARTHUD::UnitOutlineStencilForUnit`): per un nemico il piano è quello della sua voce di conoscenza,
>   mai la cella dell'attore, e un nemico ricordato o senza voce non ne ha. `ARTUnit` lo applica ai soli corpi che
>   il velo lascia accesi; anelli, freccia e sagoma del ricordo non lo ricevono.
> - 🔑 **I colori e la relazione non si ridefiniscono**: sono `RTLayerPresentation::BorderColorFor` e `Relation`, e
>   il piano attivo è quello che la board sta presentando. Unità e celle non possono dirsi piani diversi.
> - 🔧 **Il materiale è generato**, non autorato ([D-229](../../decisions/RT_PDR_00_Decision_Log.md)): lo scrive
>   `UnrealEditor-Cmd … -run=RTBuildUnitOutlineMaterial` dal clone principale. È un nodo Custom il cui codice porta
>   gli stencil del runtime; `UnitOutline.MaterialMatchesTheRecipe` confronta l'asset con la ricetta, e cade se uno
>   dei due resta indietro. Davanti a un conflitto sul binario si rigenera.
> - 🔧 **Le leve**: `rt.Unit.LayerOutline` (`0` spegne) e `rt.Unit.LayerOutline.Thickness` (3 pixel a 1080 righe,
>   scalati con l'altezza del quadro). Negativo vuol dire il default. L'autore le ha giudicate il 2026-10-13,
>   sulle tavole della seduta `U85`: il contorno resta acceso, e lo spessore passa da 2 a 3 pixel. Lo spessore di
>   default sta anche nell'asset: chi lo cambia rilancia il commandlet.
> - ⚠️ **Limiti noti**: il piano di un'unità è quello della sua cella logica, quindi durante il playback di una
>   salita il contorno può cambiare colore prima che il modello arrivi (non misurato in PIE); il contorno bianco
>   di un'unità su un piano sopra sta su un velo schiarito ([#3674](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3674)), e stacca poco.
> - 🔑 **L'ingresso è il piano del controller.** `ARTPlayerController` lo manda alla board con
>   `ARTHexMapActor::SyncActiveLayerPresentation`, in `SetActiveLayer` e a ogni fotogramma; la board riscrive solo se
>   il piano, o una leva, è cambiato. ⛔ `ARTHexMapActor::ActiveLayer` resta dell'editor mode.
> - 🔑 **La tecnica: tre canali per istanza, neutri a zero, applicati da `M_HexCell`** (`RTLayerPresentation.h`). I
>   canali 0..2 restano il colore e il velo. Nei canali 3..5 ci sono trasparenza (`OpacityMask` a dithering, materiale
>   `Masked`), desaturazione ed esposizione in stop. Si attenua l'**uscita** del materiale, come chiede D-501 (3) per i
>   terreni con texture. Il dithering non ordina la traslucenza e lascia profondità e ombre come sono.
>   ⚠️ **Il pin `Opacity Mask` vuole la costante in linea spenta** (`UseConstant`). Se resta accesa il traduttore
>   compila la costante e il nodo collegato non viene letto, senza errori: i piani sopra restano opachi coi canali
>   giusti. È successo da #3679 a #3704, col nodo collegato via bridge. Lo guarda
>   `HexMap.CellMaterialCompilesTheInputsItHasConnected`; che si **veda** lo dice il controllo positivo di
>   `PIE-CAM-PIANI-ATTIVO`.
> - 🔴 **Il velo viene prima, per costruzione**: il velo nasconde azzerando la scala, il trattamento scrive solo i suoi
>   canali, e una cella mai vista resta non disegnata su qualunque piano (`HexMap.LayerTreatmentLeavesTheVeilAlone`).
> - **Il picking non cambia**: la collisione è quella di prima, e un colpo su un piano traslucido sopra l'attivo si
>   scarta come ogni colpo fuori dal piano attivo (D-255).
> - 🎨 **I volumi ricevono il trattamento, e dicono il piano con una tinta** ([D-508](../../decisions/RT_PDR_00_Decision_Log.md), [#3706](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3706)):
>   coperture alte e basse e porte (`EdgeFeatures`), blocchi (`Blockers`), corpi strutturali (`StructuralBodies`) e
>   volumi di superficie (`SurfaceVolumes`). Sopra il piano attivo sono traslucidi e schiariti, sotto scuriti. E tutto il
>   volume prende una tinta leggera del colore del suo piano: nero sull'attivo, bianco sopra, viola sotto. Su una
>   mappa a un piano non hanno né tinta né trattamento, e si vedono come prima.
>   ⌫ *Fino a #3706 queste famiglie non avevano dati per istanza e usavano il materiale della loro mesh: era il
>   limite dichiarato da [#3679](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3679).*
>   - 🔑 **Sette dati per istanza, e un materiale generato** (`M_HexVolume`, [D-229](../../decisions/RT_PDR_00_Decision_Log.md)). I canali 0..2
>     sono il colore della tinta, 3..5 il trattamento (gli stessi di `M_HexCell`), 6 la forza della tinta. La
>     tinta è una funzione pura, `RTLayerPresentation::VolumeTint(piano, piano attivo, mappa a più piani)`. Il
>     materiale lo scrive `UnrealEditor-Cmd … -run=RTBuildHexVolumeMaterial` dal clone principale, e
>     `HexVolume.MaterialMatchesTheRecipe` confronta l'asset con la ricetta. Davanti a un conflitto sul binario si
>     rigenera.
>   - 🔑 **L'ordine nel materiale è la regola**: la desaturazione sul colore proprio, poi la tinta, poi la luce.
>     Una tinta messa prima della desaturazione perderebbe il viola su un piano sotto; messa dopo la luce
>     schiarirebbe un corpo scurito. La luce di un piano sotto vale anche per il **riflesso**: lasciato com'era,
>     un blocco due piani sotto risultava più chiaro di uno un piano sotto.
>   - 🔑 **Il colore proprio è quello che c'era**: due grigi, 0,13 per i prismi e 0,092 per i pannelli, tarati
>     sulle catture di `Visual.Map.BlockVolumes` e `Visual.Map.GrayKitYard` perché una mappa a un piano non
>     cambi. I pannelli lo ricevono da un'istanza dinamica del materiale.
>   - 🔴 **Il velo viene prima anche qui**: il velo nasconde un volume azzerandone la scala, e la tinta scrive solo
>     i suoi canali. I pannelli di bordo la riprendono dopo una ricostruzione parziale
>     ([#3687](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3687)). Lo guarda `HexMap.VolumeTintSurvivesTheVeilAndAPartialRebuild`.
>   - ⚠️ **`Relief` resta fuori**: è il rilievo che fa leggere il costo di una cella, non un oggetto, e tiene il
>     materiale di prima. Su un piano non attivo non riceve nemmeno il trattamento. Scelta della PR, senza l'autore:
>     stava nel rapporto che ha letto il 2026-10-13, e non l'ha contestata.
>   - ⚠️ **La tinta sposta anche la luminosità**: il nero scurisce un volume del piano attivo, il viola schiarisce
>     nel blu uno che sta sotto. La forza (0,25) l'ha scelta l'autore il 2026-10-13, sulle tavole della seduta
>     `U86`, fra 0, 0,25 e 0,6: *«Come tinta la seconda che mi hai mostrato. Sembra tutto ok»*.
> - 🔆 **L'esposizione della camera di gioco è fissa** ([D-501](../../decisions/RT_PDR_00_Decision_Log.md), [#3723](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3723)), per decisione
>   dell'autore del 2026-10-13. Il metodo è `AEM_Manual` senza la camera fisica, con una compensazione di 0,25
>   stop (`RTCameraExposure.h`): la luce di una cella non dipende da ciò che il quadro contiene, e lo scurimento
>   dei piani sotto vale quanto dichiara. Le leve: `rt.Camera.FixedExposure` (`0` rimette l'adattamento del
>   motore) e `rt.Camera.FixedExposure.Bias`. La giudica `PIE-CAM-ESPOSIZIONE`.
>   ⌫ *Fino a #3723 l'esposizione automatica del motore era accesa, e lo scurimento dei piani sotto, a schermo,
>   era un rapporto e non un livello: quando gran parte del quadro è «sotto» l'adattamento alza il guadagno.
>   Misurato il 2026-10-13 su `Visual.Map.LayerVolumes`, in tattica: una cella del piano terra era 136, 136 e 134
>   coi tre piani attivi; con `ShowFlag.EyeAdaptation 0` era 170, 110 e 61.*
>   ⚠️ **Vale per ogni scena.** La compensazione è tarata perché le scene a un piano si vedano come prima: la
>   differenza è fra +0,10 e +0,16 stop. Dove gran parte del quadro è nera l'adattamento schiariva, e ora no: su
>   una mappa col velo (`rt.Map.Fixture VisionSplit`), in tattica, la board è 0,48 stop più scura di prima.
> - 🔧 **Le leve**: `rt.Map.Layers.UpperOpacity` (0,35), `rt.Map.Layers.UpperExposure` (1,5 stop),
>   `rt.Map.Layers.LowerDesaturation` (1 per piano), `rt.Map.Layers.LowerDarkening` (0,4 per piano) e
>   `rt.Map.Layers.VolumeTint` (0,25, la forza della tinta dei volumi). Negativo
>   vuol dire il default. Lo scurimento, lo schiarimento e la desaturazione li ha scelti l'autore il 2026-10-13,
>   a esposizione fissa, sulle tavole della seduta `U85`. **L'opacità «sopra»** l'ha scelta nella seduta `U84`,
>   sull'asset riparato: prima la maschera era inerte e nessun valore era stato visto.
>   La forza della tinta l'ha scelta sulle tavole della seduta `U86`.
>   ⌫ *Fino a #3723 erano 3 stop, 0,7 e 0,55, scelti nella seduta `U84` con l'esposizione automatica accesa.*
> - 🔆 **I piani sopra sono anche schiariti** ([#3674](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3674)). Un velo dello stesso colore del piano che
>   copre non si vede a nessuna opacità: 35% di grigio sopra 65% di grigio è grigio. Sulla piattaforma di
>   `Visual.Map.MultiLevel`, a 120° e a 180°, l'autore aveva detto: *«No, si sovrappongono e sembrano sparire»*.
>   `rt.Map.Layers.UpperExposure` scrive nel canale di esposizione gli stop di cui schiarire. Con `0` lo
>   schiarimento si spegne. Vale per ogni piano sopra, uguale a ogni distanza, e non tocca i bordi.
>   **Il default è 1,5 stop** (luce ×2,8), scelto dall'autore il 2026-10-13 a esposizione fissa, fra 3, 2,5, 2 e
>   1,5. ⌫ *La sera del 2026-10-12, con l'esposizione automatica accesa, aveva provato 1,5, 4 e 3 e scelto 3 (luce
>   ×8): «impostata a 3 , si distingue». Su `Visual.Map.LayerStack` a 3 stop i veli erano bianchi, il contorno
>   bianco di D-508 non si staccava dal riempimento e un'unità sotto i veli era più pallida; l'aveva visto e
>   deciso: «tenere 3, in futuro con colori diversi forse dovremmo cambiare, al momento va bene».*
>   ⚠️ **A 1,5 stop il costo è dall'altra parte.** Dove i piani sopra coprono il piano attivo i veli non sono più
>   bianchi. Ma sulla piattaforma di `Visual.Map.MultiLevel`, in strategica, il velo è più chiaro del pavimento
>   di 5 livelli su 255 (a 3 stop erano 25), e la tiene distinta quasi solo il bordo bianco; in tattica lo scarto
>   è 18 a 120° e 26 a 180°. L'autore l'ha visto sulle tavole finali della seduta `U85` e ha confermato:
>   *«F1 sì, F2 tieni 1,5, F3 sì, F6 sì»*.
>   Il valore va riguardato quando i terreni avranno colori diversi.
>   ⌫ *`rt.Map.Layers.ActiveBorderStops` non c'è più da #3704: il bordo dell'attivo è nero.*

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
| Automation | ✅ i test `RefactorTactics.Camera.*` in [`RTCameraPawnTests.cpp`](../../../Source/RefactorTactics/Tests/RTCameraPawnTests.cpp): quanti, lo dice `grep -c IMPLEMENT_SIMPLE_AUTOMATION_TEST Source/RefactorTactics/Tests/RTCameraPawnTests.cpp`. ⌫ *Fino al 2026-10-15 questa cella diceva «28», un totale fermo al 2026-08-30* |
| PIE | ✅ `PIE-CAM-START` · `PIE-CAM-ORBIT` · `PIE-CAM-ZOOM-ANCHOR` · `PIE-CAM-BOUNDS` · `PIE-CAM-FOCUS` · ⏳ `PIE-CAM-HOME-CONTEXT` ([#1837](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1837)) · ⏳ `PIE-CAM-EDGE-PAN` ([#1838](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1838)) |
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
