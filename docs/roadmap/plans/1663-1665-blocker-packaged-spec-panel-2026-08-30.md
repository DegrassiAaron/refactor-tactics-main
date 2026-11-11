# `#1663` e `#1665` — i due blocker di `G13` nel pacchetto, spec panel

> `CURRENT` · **Stato**: revisione chiusa · **le tre azioni sono state applicate** su conferma d'autore
> (§ [*il difetto strutturale*](#il-difetto-strutturale-che-vale-più-delle-otto-ac)) · **Data**: 2026-08-30
> **HEAD**: `20d59973`, branch `diag/1665-istanze-board` · ⚠️ `Source/RefactorTactics/Map/RTHexMapActor.cpp`
> è **modificato e non committato**: è la sonda viva di `#1665`, letta ma **non toccata**.
> **Oggetto**: [`#1663`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1663) (animazioni
> assenti nel pacchetto) e [`#1665`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1665)
> (board nera/assente nel pacchetto), corpi **e** commenti — sei in tutto.
>
> **Perché esistono qui**: sono le due uniche issue, fra le 36 senza parent, che toccano un gate di release.
> Trovate dalla sweep `R0` del 2026-08-30
> ([referto](crud-epic-issue-mcp-spec-panel-2026-08-30.md)), non dal tracker.

## Il verdetto, in breve

✅ **Sono due referti di qualità alta, e non è il caso di riscriverli.** Ogni affermazione porta la misura
che l'ha prodotta, le ipotesi cadute sono elencate **con la ragione per cui cadono**, e `#1665` contiene una
ritrattazione e una **contro-ritrattazione**, entrambe motivate — con la frase che vale il commento intero:
*«la ritrattazione è stata più affrettata dell'affermazione»*. La critica qui sotto è sulle **acceptance
criteria**, non sulla diagnosi.

🔴 **Il difetto che le accomuna non è nel testo: nessun documento di gate le conosce.** Misurato:
`v0.1-definition-of-done.md` → **0** occorrenze di `1663`/`1665`; il corpo di `E12` ([`#26`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/26)),
che tiene la tabella di stato dei gate → **0**. `G13` è 🟡 nel DoD per una riserva del **2026-08-10**
(*«gira su `MapSource=GeneratedTestArena`»*), e le due cose che oggi lo tengono davvero fermo sono invisibili
da lì. Due blocker di `G13` esistono e il sistema di tracciamento **non li vede**.

⚠️ **Tre acceptance criteria su otto non sono eseguibili come scritte**, e una chiede di costruire un oracolo
che **esiste già**.

## Cosa è stato verificato nel codice

| Affermazione | Dove | Esito |
|---|---|---|
| Il path delle clip si compone a runtime | `Unit/RTUnitAnimInstance.cpp:19` | ✅ `"/Game/FabAsset/Paragon/Paragon%s/…/%s.%s"` |
| `DirectoriesToAlwaysCook` copre `/Game/RT`, non `/Game/FabAsset` | `Config/DefaultGame.ini:95` | ✅ |
| `ApplyMapSource`: tre rami ricostruiscono, il quarto no | `RTGameMode.cpp` 489 · 512-513 · 524-525 · 559-560 vs **554** | ✅ asimmetria reale |
| `BindToMapAsset()` è chiamata solo in editor | `RTHexMapActor.cpp:662-664`, dentro `#if WITH_EDITOR` | ✅ |
| `OnConstruction` dichiara di coprire il caso gioco via `SpawnActor` | `RTHexMapActor.cpp:659` | ✅ — e un attore **piazzato** non è quel caso |
| L'oracolo del colore per istanza esiste | `RTHexMapActorInstanceColorTests.cpp:56`, `HexMap.InstanceColorFollowsSurface` | ✅ |
| Le mesh procedurali usano `bFastBuild = true` | `RTHexMapActor.cpp` 214 · 295 · 405 | ✅ (295 lo dichiara *obbligatorio fuori dall'Editor*) |
| **Nessun test guarda i bounds** | `git grep -l 'SphereRadius\|GetBounds' Source/RefactorTactics/Tests/` | 🔴 **zero file** |
| La decisione d'autore di `#1663` è registrata | `docs/OPEN_DECISIONS.md` | 🔴 **assente** |
| `E12` / DoD nominano le due issue | `#26`, `v0.1-definition-of-done.md` | 🔴 **0 e 0** |

## `#1663` — le clip non entrano nel cook

**Karl Wiegers · qualità del criterio**

❌ **CRITICO · `AC-1` passa con una clip su otto.** Il criterio è *«`UnrealPak … -List \| findstr /i
"Animations"` non è vuoto»*: verifica che l'insieme sia **non vuoto**, mentre il corpo, sei righe più su,
enumera esattamente **quali otto** clip mancano — `Gadget/Idle`, `Gadget/Run_Fwd`, `Phase/Idle`,
`Phase/Jog_Fwd`, `Riktor/Idle`, `Riktor/Jog_Fwd`, `Wraith/Idle_NonCombat`, `Wraith/Jog_Fwd`.
📝 **Il misurabile è già nel corpo e non è arrivato nell'AC**: asserire **le otto per nome**, non la
cardinalità e non la non-vuotezza.

⚠️ **MAGGIORE · `AC-1` invecchia per costruzione, e il commento del 2026-08-30 lo dice.** Se `#288` porta i
dodici montaggi `AM_<Pack>_{Attack,Hit,Death}`, il perimetro passa da **8** a **20**. Un criterio scritto su
un numero letterale sarà falso al primo montaggio.
📝 **Scrivere l'AC come funzione**: *«ogni `TSoftObjectPtr` dichiarato da `URTUnitAnimInstance` risolve nel
container»* — l'insieme si ricava dal codice, e cresce da solo. È anche l'unica forma che regge la
raccomandazione che il commento stesso dà: *«un meccanismo che regge venti asset, non otto»*.

**Lisa Crispin · l'oracolo che c'è ed è cieco**

❌ **CRITICO · un test verde copre già questo path, e non può vedere il difetto.**
`Tests/RTUnitTests.cpp:465-480` compone la stessa radice `/Game/FabAsset/Paragon/Paragon%s/…` e confronta,
eroe per eroe, la stringa del `ToSoftObjectPath()`. È un test **buono** — il commento spiega perché include
il pack nell'asserto — ma verifica che il path sia **scritto giusto**, non che **risolva**; e gira in Editor,
dove `Content/FabAsset/` è sul disco. ∴ resta verde per costruzione mentre il pacchetto degrada.
📝 **L'AC deve nominare questo test e dire perché non basta**, altrimenti il prossimo che legge «i path delle
animazioni sono testati» chiude la questione.

**Alistair Cockburn · il nodo d'autore non ha un artefatto**

⚠️ **MAGGIORE · il corpo dichiara «la scelta è d'autore» e la decisione non esiste da nessuna parte.**
Misurato: `OPEN_DECISIONS.md` non ha nessuna voce su cook, clip o `FabAsset`. Una decisione che blocca un
gate di release e vive **solo** nel corpo di una issue è una decisione che nessuno troverà cercandola.
📝 **Aprire la voce in `OPEN_DECISIONS.md`** con le due vie già scritte nel corpo (elencare gli asset per
nome → il cook fallisce a chi clona senza i pack; non cuocerle → il vertical slice consegna unità immobili),
e con il vincolo che il commento aggiunge: la scelta deve reggere **venti** asset.

**Martin Fowler · il precedente è nello stesso file**

✅ **Merito, e indica la forma della soluzione.** `Config/DefaultGame.ini:97` porta già, scritto a mano, il
gemello di questo difetto: *«la mappa di partita va elencata QUI, e `DirectoriesToAlwaysCook` non la
copre»*. È la **seconda istanza della stessa classe** — un asset che entra nel gioco per una via che il cook
non segue — e la prima è già stata pagata e documentata a tre righe di distanza.

## `#1665` — la board non arriva a schermo

**Karl Wiegers · il corpo contraddice il proprio commento**

❌ **CRITICO · `AC-3` chiede di costruire un oracolo che esiste.** Dice: *«un test che, dopo
`RebuildInstances`, asserisce che almeno un'istanza ha custom data ≠ 0 — **oggi nulla lo verifica**»*. Il
quarto commento lo **ritratta**: `HexMap.InstanceColorFollowsSurface` esiste, ed è *più severo* — confronta
il valore contro la tavolozza invece di una soglia. Il corpo non è stato corretto.
📝 **La ritrattazione va nel corpo**, non solo nel commento: il corpo è ciò che si legge per primo, e questa
AC come sta prescrive lavoro già fatto. Vedi il difetto gemello che il repository ha già registrato — una
correzione che vive nel commento e non arriva nel corpo.

**Michael Nygard · l'AC prescrive lo strumento che ha già fallito**

❌ **CRITICO · `AC-1` chiede uno *screenshot dal pacchetto*, e la sezione «Limiti» dello stesso corpo dichiara
che la cattura non è riuscita** — `-log` apre una console separata, `SetForegroundWindow` è bloccata da
Windows. Un criterio di accettazione che dipende da uno strumento **già misurato come non funzionante** non
è un criterio: è un blocco.
📝 **E l'alternativa è già stata dimostrata dall'autore stesso**: la sonda in fondo a `RebuildInstances`
risponde dal pacchetto, per iscritto, e ha prodotto il fatto decisivo — `LevelAsset` silenzio, `GeneratedTestArena`
65 istanze. **L'oracolo del pacchetto è il log, non l'occhio.** Le catture, quando ci sono state, sono
servite a confermare l'HUD, non la board.

**Lisa Crispin · l'oracolo che manca davvero, e non è quello nell'AC**

🔴 **CRITICO · zero test guardano i bounds, ed è l'unica ipotesi viva che si può falsificare senza un
pacchetto.** L'ipotesi è già scritta nella sonda non committata: `BuildFromMeshDescriptions` prende i bounds
da `MeshDescriptions[0]->GetBounds()`, e un raggio nullo fa **scartare ogni istanza dal culling** — cioè
esattamente *«tutto corretto e niente a schermo»*, che è lo stato misurato su `GeneratedTestArena` (65
istanze, mesh giusta, materiale giusto, 3 float per istanza, nulla a schermo). È anche l'unica ipotesi che
spiega perché **`Relief` non si vede**, che usa la stessa mesh col materiale di default.
📝 **`TestTrue("la mesh della cella ha un raggio non nullo", Mesh->GetBounds().SphereRadius > 0.f)`** dopo
ciascuna delle tre `BuildFromMeshDescriptions` (`RTHexMapActor.cpp` 214 · 295 · 405). Costa una riga, gira
in Editor, e se è verde **elimina l'ipotesi** invece di lasciarla in campo. Nessuna AC attuale la nomina.

**Martin Fowler · una issue, due difetti**

⚠️ **MAGGIORE · dal quinto commento in poi `#1665` contiene due difetti separati e le AC non li separano.**
(1) ramo `LevelAsset`: **zero istanze** — `ApplyMapSource` duplica `MapAsset` senza ricostruire, e le due
reti che coprirebbero il buco sono editor-only. (2) ramo `GeneratedTestArena`: **65 istanze invisibili**. La
diagnosi del primo è confermata dalla sonda; il secondo è aperto. Con AC comuni, la chiusura è ambigua: si
può riparare (1) e lasciare la board invisibile.
📝 **Scorporare (2)** in una issue propria — *«le istanze esistono e non arrivano a schermo»* — e lasciare a
`#1665` il difetto con causa nota e fix noto. ✅ **E il fix di (1) è già scritto bene**: non la riga
simmetrica, ma `SetMapAsset()` che ricostruisce, *«perché l'accoppiamento diventa impossibile da
dimenticare»*.

**Gojko Adzic · cosa non toccare**

✅ **La tabella «tre ipotesi cadute» del primo commento è il pezzo migliore dei due referti**, e la riga che
la chiude è il metodo: *«ogni percorso di colore sbagliato in questo codice produce un colore — grigio nel
caso peggiore. Solo l'assenza totale di dati produce nero»*. È un'inferenza dal **codice** a ciò che lo
schermo può mostrare, e ha ristretto il campo prima di qualsiasi esperimento.

## Il difetto strutturale, che vale più delle otto AC

Nessuna delle due issue è nominata da `E12`, dal DoD, o da qualunque documento che descriva `G13`. La riga
`G13` del DoD è ferma a una riserva del **2026-08-10** su `MapSource=GeneratedTestArena` — che *era* il
motivo giusto allora, ed è stato **superato dai fatti**: `#1654` ha fatto entrare la mappa d'autore nel
pacchetto, e da lì sono emersi questi due. Un gate 🟡 la cui riserva è scaduta legge come un gate quasi verde.

✅ **Tre azioni, applicate il 2026-08-30 su conferma d'autore:**

1. **Parent a entrambe → `E12`** ([`#26`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/26)),
   che possiede i gate di release ed è dove qualcuno cerca «perché `G13` non è verde». `#1663` e `#1665`
   dichiarano ora `**Epic**: #26` in testa al corpo, **e `#26` le nomina entrambe**: il legame è nei due
   versi, altrimenti avrei ricreato il link a senso unico riparato lo stesso giorno sugli altri sette padri.
   `#1663` conserva `Refs #288` — correttamente `Refs` e non `Depends on`.
2. **Riserva di `G13` aggiornata** nel DoD: la precedente, del 2026-08-10, elencava due mancanze di cui
   `#1654` ha rimosso la prima — ed è proprio quella rimozione ad aver scoperto questi due difetti. Una
   riserva che descriveva correttamente il mondo e ha smesso, **senza che una riga cambiasse**.
3. **`COOK-1` e `COOK-2` aperte** in [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md): quali asset di
   `Content/FabAsset/` entrano nel cook e con quale meccanismo — vincolato a reggere **venti** asset, non
   otto — e, sul ramo negativo, se il DoD dichiara che il vertical slice consegna unità immobili.

## L'oracolo dei bounds — scritto, eseguito, **verde**, e l'ipotesi cade in Editor

`Source/RefactorTactics/Tests/RTHexMapMeshBoundsTests.cpp`,
`RefactorTactics.HexMapActor.ProceduralMeshBoundsContainTheirGeometry`. Copre le **tre** mesh procedurali
dell'attore — prisma della cella, volume di conoscenza, glifo a 1–4 anelli — perché passano tutte per la
stessa `BuildFromMeshDescriptions` con `bFastBuild`.

🔑 **Non è un `SphereRadius > 0`, ed è deliberato.** Quella soglia passerebbe con un raggio di 0,001 uu, che
è zero per il culling e non zero per il test. L'atteso si ricava dai **vertici che la mesh ha davvero**,
letti dal suo render data, e le asserzioni sono quattro: il raggio non è nullo; il box dichiarato
**contiene** i vertici reali; il raggio copre il vertice più lontano; e non è spropositato rispetto ad esso.
È la stessa forma che `HexMap.InstanceColorFollowsSurface` usa per il colore — costruire l'atteso dalla
regola dichiarata invece che da numeri copiati — applicata alla geometria.

### La misura

`Build.bat RefactorTacticsEditor Win64 Development` → `Result: Succeeded`, **zero** warning.
`./scripts/rt-suite.ps1 -Filter RefactorTactics.HexMapActor` → run dichiarata **VALIDA** (`HEAD` `20d59973`,
albero e binario identici a inizio e fine), `Found 10 automation tests`, **10/10 completati, 0 fallimenti**,
`ProceduralMeshBoundsContainTheirGeometry` = `Success`.

| mesh | raggio dichiarato | vertice più lontano |
|---|--:|--:|
| prisma della cella | **70,711** | 70,711 |
| volume di conoscenza | **70,711** | 70,711 |
| glifo, 1–4 anelli | **47,500** | 47,500 |

∴ **in Editor i bounds non solo non sono nulli: sono esatti e stretti.** `70,711` è `sqrt(50² + 50²)`, cioè
il vertice del prisma a circumraggio 50 e mezza-altezza 50. **L'ipotesi «bounds a zero» cade per l'Editor**,
e una divergenza nel cotto dev'essere del cotto — non del calcolo che li produce.

### 🔴 Il primo run è fallito, e il difetto era nell'atteso, non nel motore

`Result={Fail}` su **tutte e sei** le mesh, con lo stesso scarto: *«il raggio (70,711) copre il vertice più
lontano (82,916)»*. Il `82,916` era la distanza dell'**angolo dell'AABB**, non di un vertice — e un esagono
non ha vertici agli angoli del proprio rettangolo circoscritto. Il motore aveva ragione, il criterio no.
Corretto **rileggendo i vertici**, non allargando la tolleranza: una tolleranza da 12 unità avrebbe reso
verde il test e cieco l'oracolo. ✅ **In compenso il fallimento prova che l'asserzione morde**: confronta
valori veri e li rifiuta quando non tornano.

⚠️ **Un verde qui non scagiona il pacchetto**, che è dove vive il difetto di `#1665`: il test è
`EditorContext`. Renderlo `ClientContext` lo porterebbe sul binario **staged**, dove sarebbe decisivo, e ha
un costo dichiarato — la suite raggiungibile dal packaged passerebbe da **11** a 12, e la riga `G2` del §3
del DoD nomina quegli undici uno per uno. È una scelta da fare **insieme** a quel documento, e per questo il
test nasce Editor-only con il limite scritto nel proprio docstring.

## La sonda estesa sul pacchetto — i valori, letti

Eseguita il 2026-08-30 sul binario staged (`Binaries/Win64/RefactorTactics.exe`, **14:53**, più recente del
sorgente delle 14:48; `.utoc` delle 15:01 — non stantio, e la sonda estesa è dentro: `boundsComp` compare
nel binario in UTF-16). Comando: `RefactorTactics.exe L_HexArena -windowed -log
-dpcvars=rt.Map.Source=GeneratedTestArena` — `-dpcvars` e non `-ExecCmds`, che farebbe saltare la coda.

```text
[RT][1665] MapAsset 'RTHexMapAsset_2147482304' con 65 celle
  Cells=[ist 65, mesh 'RT_CellHexPrism' r=70.7 rd=1, mat 'M_HexCell', vis 1, boundsComp r=1567.2, cd 3(195) 0.352/0.352/0.352]
  Relief=[ist 3, ... boundsComp r=1006.1, cd 0(0) -]
  Blockers=[ist 8, ... boundsComp r=1290.7, cd 0(0) -]
  Actor: hidden=0, scale=X=1.000 Y=1.000 Z=1.000
```

🔴 **I custom data NON sono zero, e l'ipotesi «il colore non viene scritto» cade.** `cd 3(195)` sono tre
float per istanza per 65 istanze — il buffer è pieno — e i valori della prima istanza sono
`0.352/0.352/0.352`.

✅ **E quel numero è corretto, non un ripiego.** `0.352` è `linear(160/255)`, cioè `FColor(160, 160, 160)`:
il ramo `Floor` / `default` di `URTHexLibrary::SurfaceColor`. ⛔ **Sembrava un difetto e non lo è**:
`MakeTestArena` contiene **una sola** assegnazione di superficie — tre celle a `Rough` (`RTMatchSetupLibrary.cpp:230-235`,
`R = -1..1`) — e le altre 62 restano `Floor`. Una cella grigia è la risposta giusta.

⚠️ **Ma il commento che descrive quell'arena è falso**: `RTGameMode.cpp:481` dice che *«`MakeTestArena` le
lascia tutte `Rough`»*. Misurato: **3 su 65**. È la riga che avrebbe fatto leggere il grigio come un difetto.

✅ **I bounds nel pacchetto sono identici all'Editor**: `r=70.7` sulla mesh — lo stesso `70,711` che il test
misura — e `boundsComp r=1567.2` sul componente. **L'ipotesi dei bounds cade anche nel cotto**, non solo in
Editor, e questa volta con la misura presa dove il difetto vive.

🔴 **Un fatto nuovo, che nessun commento della issue aveva**: un secondo dopo il caricamento parte un
**ensure**, non un crash —

```text
Ensure condition failed: GetStaticMaterials()[MaterialIndex].UVChannelData.bInitialized
  UStaticMesh::GetUVChannelData() → UInstancedStaticMeshComponent::GetMaterialStreamingData()
  → FRenderAssetStreamingManager::UpdateResourceStreaming()
```

È conseguenza diretta di `bFastBuild = true`, che salta il calcolo degli `UVChannelData`: lo streaming
manager li pretende e trova un campo non inizializzato. **Non spiega da solo l'invisibilità** — un ensure non
nasconde geometria, e anzi prova che l'ISM è registrato nello streaming, cioè *nella scena* — ma è una
divergenza **cooked-only** prodotta dalla mesh costruita a runtime, che è la famiglia di cause rimasta.

### Limiti di questa esecuzione

- ⛔ **La sonda stampa solo la PRIMA istanza.** «I valori non sono zero» è provato per l'istanza 0 e per la
  dimensione del buffer (195), non per tutte e 65.
- ⛔ **Nessuno screenshot**: non so dire se in *questa* run la board si vedesse. E la domanda non è oziosa —
  la sonda estesa forza `MarkRenderStateDirty` sui tre ISM popolati, quindi questa run **non** misura lo
  stato base.
- ⛔ **Solo il ramo `GeneratedTestArena`**: il ramo `LevelAsset`, dove la sonda taceva, non è stato rieseguito.

## ✅ `#1665` — causa radice trovata e **corretta**: `MaterialIndex = -1`

**`BuildFromMeshDescriptions` lega sezione e slot materiale PER NOME**, confrontando il
`PolygonGroupMaterialSlotName` del `MeshDescription` col `MaterialSlotName` di `FStaticMaterial`. Nelle tre
mesh procedurali il gruppo si chiamava `"Default"` e lo slot era un `FStaticMaterial()` nudo
(`MaterialSlotName = NAME_None`): **i nomi non combaciavano**, e la sezione usciva con `MaterialIndex = -1`
— geometria completa e nessun materiale da usare. Invisibile.

```text
prima:  Cells=[... LOD 1 ss0=1.0000 vtx0=36 sez0=1 tri=20 idx=60 matIdx=-1/1 ...]   ⛔ nero
dopo:   Cells=[... LOD 1 ss0=1.0000 vtx0=36 sez0=1 tri=20 idx=60 matIdx= 0/1 ...]   ✅ board visibile
```

✅ **Verificato a schermo sul pacchetto**: 65 celle grigie, la fascia di fango **marrone**, i muri come
lastre scure, la piattaforma del layer 1 rialzata, e gli eroi appoggiati sulle celle invece che sospesi.

### Come ci si è arrivati, e perché l'ordine conta

Quattro ipotesi cadute **con misura**, non per esclusione: custom data (`cd 3(195)`, valori `0.352` =
`linear(160/255)`, cioè il `Floor` corretto), bounds (`r=70.7` identico in Editor e nel cotto),
`ScreenSize` (prisma `1.0`, Cube `2.0`, entrambi validi), materiale (`MSM_Unlit · BLEND_Opaque ·
PerInstanceCustomData3Vector · bUsedWithInstancedStaticMeshes` presente).

🔑 **L'esperimento che ha isolato la causa**: commutare la **sola** mesh a `/Engine/BasicShapes/Cube` da una
CVar, stessa run, stesso materiale, stessi custom data. Il Cube si vedeva — e la sonda diceva `matIdx=0/1`
contro `matIdx=-1/1`. Da lì il campo si è chiuso su un campo solo.

### Il fix, e la sua forma

I sei letterali (tre `PolygonGroup` + tre slot, **200 righe distanti** l'uno dall'altro) sono ora una
costante sola, `RTProceduralMeshSlotName`: cambiarne uno senza l'altro non è più possibile. Resta anche lo
slot inizializzato con `FMeshUVChannelInfo(1.f)`, che ha eliminato l'`ensure` su `UVChannelData` — misurato:
**2** occorrenze prima, **0** dopo. ⚠️ **Era un sintomo della stessa famiglia, non il meccanismo**: la board
è rimasta nera dopo quel fix, e fermarsi al verde dell'ensure avrebbe chiuso il caso sbagliando.

### 🔴 Il test che credevo presidiasse il difetto NON lo presidia, e l'ha detto la mutazione

`RefactorTactics.HexMapActor.ProceduralMeshesAreRenderable` asserisce anche che ogni sezione punti a uno
slot valido. Sembrava l'oracolo mancante. **Verifica di mutazione**: rinominato **un solo lato** del legame
(slot → `"MUTAZIONE_1665"`, gruppo fermo su `"Default"`) il test è rimasto **verde**, e il valore misurato
dice perché — `MaterialIndex = 0`. **In Editor un fallback risolve l'indice anche quando i nomi non
combaciano**; nel cotto no.

⚠️ **Mutare la costante condivisa non sarebbe stata una mutazione valida**: cambia entrambi i lati, che
restano allineati. La mutazione fedele al difetto ne muta **uno solo**.

∴ l'asserzione copre il caso **strutturale** — zero sezioni, zero slot, indice fuori intervallo — non il
disallineamento di nome. ⛔ **L'unico oracolo del difetto vero resta il pacchetto**, finché il test non
diventa `ClientContext`: lo porterebbe sul binario staged dove `-1` si vede, al costo dichiarato di portare
la suite packaged da **11** a 12 e di toccare la riga `G2` del §3 del DoD, che nomina quegli undici uno per
uno. **Scelta d'autore, non applicata.**

## L'esperimento controllato che ha isolato la mesh

**`RT_CellHexPrism` — la mesh costruita a runtime — non arriva a disegnare nel cotto.**

Il controllo: stesso binario, stessa arena, stesso materiale, stessi custom data; **unica variabile la
mesh**, commutata a `/Engine/BasicShapes/Cube` da una CVar (`rt.Map.DebugEngineMesh=1`, registrata a scope di
file perché una registrata dentro una funzione non esisterebbe ancora quando `-dpcvars` la assegna).

| run | mesh | sonda | a schermo |
|---|---|---|---|
| prisma | `RT_CellHexPrism` r=70.7 | `ist 65, mat 'M_HexCell', vis 1, cd 3(195) 0.352/0.352/0.352` | ⛔ **nero**: solo eroi e anelli di squadra |
| Cube | `Cube` r=86.6 | `ist 65, mat 'M_HexCell', vis 1, cd 3(195) 0.352/0.352/0.352` | ✅ **board visibile**, grigio `Floor` + **marrone** `Rough` sulla fascia di fango, piattaforma del layer 1 come lastre rialzate |

∴ **il materiale è scagionato in modo definitivo**: gli stessi identici custom data che erano invisibili sul
prisma sono visibili sul Cube, e per di più coi colori giusti — il che prova in un colpo solo che
`M_HexCell` legge `PerInstanceCustomData`, che i valori sono scritti, e che il componente disegna.

### Il fix dello slot è reale, e non era la causa

`FStaticMaterial()` aggiunto nudo lascia `UVChannelData.bInitialized = false`; inizializzarlo con
`FMeshUVChannelInfo(1.f)` ha **eliminato l'ensure** — misurato: 2 occorrenze prima, **0** dopo, sulla stessa
arena. È un difetto vero e va tenuto. Ma la board è rimasta nera: **l'ensure era un sintomo della stessa
famiglia, non il meccanismo**. Vale la pena scriverlo, perché la tentazione era di dichiarare chiuso il caso
sul verde dell'ensure.

### Cosa resta in campo per `#1665`

Con i bounds eliminati in Editor, delle due ipotesi del quinto commento restano: i **valori** dei custom data
a zero nel cotto (la sonda riporta `NumCustomDataFloats = 3`, cioè lo *spazio*, non i valori — e la sua
estensione non committata li stampa già), oppure una differenza **cooked-only** nella costruzione della mesh
che i bounds in Editor non possono mostrare. 🔑 L'indizio che questo test **non** tocca resta il più forte:
`Relief` usa la stessa mesh col materiale di default e non si vede nemmeno lui.

## Cosa questa revisione non ha fatto

- **Nessun `CREATE`, `CLOSE` o `REOPEN`**: tre corpi di issue toccati (`#1663`, `#1665`, `#26`), tutti in
  modo additivo, più due documenti e un file di test nuovo. Nessuna issue aperta o chiusa, nessuna label,
  nessuna milestone.
- **Non ha eseguito la suite completa**: solo il filtro `RefactorTactics.HexMapActor`, dieci test. Gli altri
  ~1370 non sono stati toccati da questa modifica — è un file nuovo — ma non sono stati rieseguiti.
- **Non ha provato la mutazione sul soggetto**: dimostrare che l'oracolo diventa rosso azzerando davvero i
  bounds richiede di toccare `RTHexMapActor.cpp`, che porta la sonda non committata di un'altra sessione.
  ⛔ Non l'ho fatto. L'evidenza che l'asserzione morde è indiretta e comunque reale: ha **rifiutato** un
  atteso sbagliato al primo run, su sei mesh su sei.
- **Non ha risolto `#1665`**: ha eliminato un'ipotesi in Editor. Le altre due restano.
- **Non ha modificato le acceptance criteria** delle due issue: i rilievi qui sopra restano proposte, perché
  riscrivere l'AC di un difetto aperto è lavoro del suo owner.
- **Non ha toccato la sonda**: `RTHexMapActor.cpp` è modificato in working directory da un lavoro vivo su
  `#1665`, ed è stato solo letto.
- **Non ha eseguito nulla**: né suite, né build, né pacchetto. Ogni riga della tabella di verifica viene da
  `git grep` o dalla lettura del sorgente su `20d59973`.
- **Non ha verificato l'ipotesi dei bounds**: ha misurato che **nessun test la copre** e che è falsificabile
  in Editor. Se sia la causa, non lo dice questo referto.
