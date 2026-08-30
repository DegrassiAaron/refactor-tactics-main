# Spec — Domini spaziali della mappa: giocabile, scenico, camera, sfondo

> **Owner documentale** dei quattro domini spaziali e del linguaggio del bordo. `CURRENT` · normativo.
> Epic **E49** ([#1769](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1769)) · decisioni `D-250`, `D-251`.
> La camera ha un owner separato: [`spec-tactical-camera.md`](spec-tactical-camera.md).
> Le convenzioni di contenuto e i percorsi `/Game/RT/` restano di
> [`convenzioni-contenuti-ue.md`](../tooling/convenzioni-contenuti-ue.md).

Una mappa tattica ha **quattro** estensioni diverse, e finora il repository ne conosceva una sola: quella
delle celle. Tutto il resto — dove la camera può arrivare, cosa si vede oltre il bordo, cosa c'è
all'orizzonte — non aveva un nome, e ciò che non ha un nome viene deciso caso per caso da chi costruisce
il livello.

> ℹ️ **Misurato il 2026-08-30**: zero occorrenze di `scenic`, `skyline`, `PlayableBounds` o
> `BackgroundBounds` in `Source/`, e nessuna in `docs/` fuori dall'archivio. Questo documento apre un
> vocabolario, non ne riordina uno esistente.

---

## 1. I quattro domini — [#1777](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1777)

```text
┌──────────────────────────────────────────────────────────┐
│  VisualBackgroundBounds — skyline, montagne, cielo       │
│  ┌────────────────────────────────────────────────────┐  │
│  │  ScenicBufferArea — visibile, mai giocabile        │  │
│  │  ┌──────────────────────────────────────────────┐  │  │
│  │  │  PlayableMapBounds — le celle, il gioco      │  │  │
│  │  └──────────────────────────────────────────────┘  │  │
│  │        CameraTravelBounds attraversa i due         │  │
│  └────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────┘
```

### 1.1 `PlayableMapBounds`

L'area tattica realmente giocabile: celle, unità, percorsi, copertura, hazard, targeting, LOS, obiettivi,
interazioni. È l'unica delle quattro che **esiste già** — è l'estensione delle celle di `URTHexMapAsset`,
e `ARTCameraPawn::ClampToSoftBounds` la calcola come AABB sui centri-cella reali.

⚠️ **Reale, non dedotta.** Il codice itera le celle e prende min/max invece di ricavare i limiti dal
raggio, e il commento dice perché: *«una mappa dipinta a mano non è un esagono pieno, e dedurne i limiti
dal numero di celle darebbe un bordo che non esiste»*. La regola vale per tutti e quattro i domini: un
dominio si misura su ciò che contiene.

### 1.2 `ScenicBufferArea`

Area **visibile e non giocabile** attorno a quella giocabile. Serve a dare continuità ambientale e a
nascondere il bordo artificiale della board: senza, il campo tattico galleggia nel vuoto e ogni
inquadratura al bordo lo dichiara.

Può contenere edifici, strade, alberi, rocce, rovine, container, strutture industriali, acqua,
scenografia e occluder visuali. **Deve** essere:

- non pathabile;
- non selezionabile;
- priva di celle tattiche;
- priva di spawn;
- priva di bersagli gameplay;
- priva di interazioni gameplay;
- priva di marker tattici non autorizzati.

Può usare LOD e HLOD aggressivi: nessuno ci gioca.

> 🔴 **È qui che si rompe `D-143` per distrazione.** Una mesh scenica che *sembra* un muro non blocca
> nulla, perché la LOS vive nei dati autorevoli. Se una struttura del buffer deve davvero bloccare,
> allora non appartiene al buffer: appartiene alla mappa giocabile, con la sua cella o il suo edge. La
> domanda da farsi davanti a ogni mesh sul bordo è *«se un'unità sparasse attraverso, cosa deve
> succedere?»* — e la risposta decide in quale dominio va.

### 1.3 `CameraTravelBounds`

Il limite del **pivot** della camera. È un dominio **separato** da `PlayableMapBounds`: il pivot può
entrare parzialmente nel buffer scenico, altrimenti il giocatore non può portare il bordo al centro dello
schermo per vedere cosa c'è lì.

Oggi esiste in forma minima: `BoundsMarginCells = 3` celle oltre il bordo, misurate dal centro camera.
Il valore è deciso da [#864](https://github.com/DegrassiAaron/refactor-tactics-main/issues/864) e istruito
sulla scala reale — su una mappa di raggio 4 il centro arriva a 7 celle dall'origine. È una misura
**fissa e non proporzionale**, perché il margine serve al bordo e il bordo è locale.

Ciò che il travel bound deve garantire è una sola cosa: **non mostrare il vuoto esterno**. Un margine in
celle non basta a garantirlo, ed è il motivo del §2.

### 1.4 `VisualBackgroundBounds`

Il dominio più esterno, usato **solo** per lo sfondo distante: skyline, montagne, città, oceano, pianure,
mega-strutture, cielo, atmospheric fog. Deve essere molto economico — è ciò che si vede sempre e non si
guarda mai.

---

## 2. Effective Camera Bounds — [#1778](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1778)

✅ **Implementato il 2026-08-30** ([#1778](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1778)) come funzione **pura**:
`ARTCameraPawn::ComputeEffectivePivotBounds(AllowedArea, ArmLength, Pitch, Yaw, FOV, AspectRatio,
AllowedOutsideFraction)`. Prende le metriche come **parametri** invece di leggerle, ed è la ragione per cui
è verificabile headless — dove un viewport non esiste.

Prima c'era solo questo:

```cpp
FVector(FMath::Clamp(Desired.X, Min.X - Margin, Max.X + Margin),
        FMath::Clamp(Desired.Y, Min.Y - Margin, Max.Y + Margin), Desired.Z)
```

cioè `Pivot = Clamp(Pivot, RettangoloFisso)`. È corretto per ciò che è stato chiesto a `#864` — non
perdere la mappa — e **insufficiente** per non mostrare il vuoto, perché il pivot non è ciò che si vede.

Quanto mondo entra nello schermo dipende anche da distanza, inclinazione, orientamento, campo visivo e
forma del viewport:

```text
EffectivePivotBounds = f(MapBounds, ScenicBuffer, Zoom, Pitch, Yaw, FOV, AspectRatio)
```

Il caso che rompe il clamp fisso è concreto: a `pitch` quasi orizzontale e distanza massima, un pivot
**dentro** i limiti mostra comunque metà schermo oltre il bordo — e su un monitor 32:9 lo mostra ai due
lati contemporaneamente, dove nessuno ha costruito nulla.

⚠️ **L'obiettivo non è «zero pixel fuori»**: è che una porzione consistente dello schermo non finisca fuori
dalla zona prevista, soprattutto ai bordi. La soglia è una taratura, e si tara guardando.

### Casi provati, e quelli che restano

✅ **Headless** (`Camera.EffectivePivotBoundsShrinkWithZoomPitchAndAspect`): distanza, pitch a picco contro
pitch radente, `16:9` contro `32:9`, yaw a 90° che scambia gli assi dell'ingombro, aspect ratio ignoto che
riporta al clamp per sole celle, e area più piccola dell'inquadratura.

⚠️ **Quest'ultimo caso è quello che un `Clamp` ingenuo sbaglia in silenzio**: su una mappa piccola, o a
zoom massimo, l'inset supera l'area e l'intervallo si rovescia — `FMath::Clamp` su estremi invertiti
inchioda il valore a un estremo **senza dirlo**, e la camera resterebbe incollata a un angolo. Il
comportamento corretto è il centro, ed è l'unico punto che minimizza il fuori-zona.

⏳ **Restano le verifiche percettive**, che headless non si fanno: zoom minimo e massimo su una mappa vera,
i quattro angoli, un focus vicino al bordo, e il giudizio *«si vede il vuoto»* su un monitor ultrawide
reale. `AllowedOutsideFraction` è **taratura aperta**: nessun numero è stato istruito.

🔴 **Un effetto da verificare in PIE prima di tarare qualunque altra cosa.** I limiti nuovi stringono
**più** dei vecchi a distanza alta, ed è il loro scopo — ma su una mappa piccola l'inset supera l'area già
a zoom medio, e lì il pivot si inchioda al centro. Calcolato sui valori di oggi (`HexSize = 150`, raggio 4,
`BoundsMarginCells = 3`, FOV 90°, 16:9): a `MatchStartArmLength = 450` l'inset vale ~290 unità contro una
semi-area di ~1800, quindi nessun effetto; a `MaxArmLength = 4000` l'inset supera l'area e il pivot resta
fermo al centro. **A zoom massimo è il comportamento voluto** — si vede già tutto, non c'è dove andare —
ma il punto in cui la camera comincia a irrigidirsi non è stato misurato a schermo, e
`AllowedOutsideFraction` è la manopola che lo sposta.

⛔ **L'area consentita è ancora `celle + margine`**: il buffer scenico non esiste come dato, quindi
`ScenicBuffer` non entra ancora nella formula. Il punto dove dovrà entrare è marcato nel codice.

---

## 3. Boundary Language

Il bordo non deve leggersi come un muro invisibile. Dove è possibile, il level design spiega da sé perché
non si prosegue — e la spiegazione appartiene al tema della mappa:

| Ambiente | Il bordo è… |
|---|---|
| città | palazzi, barricate, strade bloccate |
| industriale | container, recinzioni, macchinari |
| montagna | pareti rocciose o precipizi |
| foresta | vegetazione impenetrabile |
| porto | acqua, moli distrutti |
| tunnel | muri o porte sigillate |
| tetti | il bordo dell'edificio |
| zona militare | barriere e strutture |

Non è arredamento: un bordo che si spiega da solo riduce i tentativi di andare oltre, e ogni tentativo
mancato è un momento in cui il giocatore scopre che il mondo finisce.

> ⚠️ **Il bordo narrativo resta scenografia.** Un palazzo che chiude la strada sta nel `ScenicBufferArea`
> e non blocca nulla: ciò che impedisce di proseguire è l'assenza di celle, non la mesh. Le due cose
> devono coincidere **visivamente**, ed è esattamente questo il lavoro — non farle coincidere
> *meccanicamente*, che sarebbe la violazione descritta al §1.2.

---

## 4. Cosa questo documento non decide

- **I numeri.** Nessun margine, soglia o distanza è fissato qui: si tarano in `L_CameraFeatureLab`
  ([`spec-tactical-camera.md`](spec-tactical-camera.md) §9) e vivono in configurazione.
- **Dove vivono i domini nei dati.** Se `ScenicBufferArea` sia un volume in mappa, un campo dell'asset o
  una convenzione di cartella è una scelta di implementazione con un costo diverso per l'autore, e va
  fatta guardando `URTHexMapAsset` e le convenzioni di `/Game/RT/`, non a tavolino.
- **Le mappe esistenti.** `L_DevSandbox`, `L_HexArena`, `L_Prototype` non hanno buffer scenico e non è un
  difetto: sono scene di lavoro. La regola vale per le mappe di gioco.
