# Bundle `GrayToolkit_Cloud_Bundle` — archiviato il 2026-08-17

> `HISTORICAL` · **Materiale NON autorevole.** Si legge per sapere da dove viene una decisione. Le fonti
> autorevoli sono [`../../../decisions/RT_PDR_00_Decision_Log.md`](../../../decisions/RT_PDR_00_Decision_Log.md)
> (`D-158`), [`../../../technical/spec-graybox-placement-contract.md`](../../../technical/spec-graybox-placement-contract.md)
> e [`../../../technical/spec-asset-pipeline.md`](../../../technical/spec-asset-pipeline.md) §11-bis.

## Cosa conteneva

| File | Esito |
|---|---|
| `01_Graybox_Kit_Cover_CellVolume_Consolidation.md` | **non archiviato qui: era già in archivio.** Byte-identico al kit consumato la mattina stessa — `md5 4048a39b17513e88da41d3c7ba75aaee`, verificato contro l'originale nella root. Vive in [`../CLAUDE_RefactorTactics_Graybox_Kit_Cover_CellVolume_Consolidation_2026-08-17.md`](../CLAUDE_RefactorTactics_Graybox_Kit_Cover_CellVolume_Consolidation_2026-08-17.md) |
| `02_GrayToolkit_AssetRoadmap_Wiki_Issues.md` | handoff operativo — la fonte di `D-158` |
| `03_GrayToolkit_Wiki_Pages_Original.md` | prima stesura delle pagine Wiki; superata da `04` |
| `04_GrayToolkit_Wiki_Pages_v2_Latest.md` | struttura Wiki v2, con il testo pronto delle pagine |
| `README_FIRST.md` | ordine di lettura proposto dall'autore |
| `images/RT_GrayToolkit_Public_Infographic_v2.png` | pubblicata come `images/wiki/core/24_graybox-toolkit-overview.png` |
| `images/RT_GrayToolkit_UML_Developer_v2.png` | pubblicata come `images/wiki/technical/25_graybox-toolkit-architettura-proposta.png` — il nome dice **proposta**, e il perché è sotto |

## Cosa è entrato

**Le lane di maturità `C0–C6` e `E0–E5`** — l'unica parte che il repository non aveva, misurata: zero
occorrenze in `docs/` fuori da archivio. Sono in `spec-asset-pipeline.md` §11-bis, che è l'owner dei
principi di pipeline, con la regola che le rende utili: **il contratto si congela a `C2`/`E1`**, non a
`C6`/`E5`.

**La conferma della scala d'arte**, che ha prodotto una correzione: `1 UU = 1 cm` e lato `1,50 m` erano già
in `convenzioni-contenuti-ue.md` §11-bis, ma `spec-graybox-placement-contract.md` §6 portava «`C ≈ 173` con
`HexSize` al default `100`» come se fosse *la* scala del progetto. Era vera del **default del campo** e
falsa della **scala d'arte** (`C ≈ 2,60 m`). Il bundle non l'ha introdotta: l'ha resa visibile mettendo i
due numeri accanto.

## Cosa non è entrato, e perché

🔴 **La UML non è una vista as-built.** Misurati tutti i simboli che dichiara:

| Simbolo | In `Source/` |
|---|---|
| `FRTCellId` | ✅ reale |
| `FRTCellData` · `ESurfaceType` | esistono con **altro nome** — `FRTHexCellData`, `ERTHexSurface` |
| `FRTCellPlacementVolume` · `URTGrayboxAssetContract` · `URTGrayboxPrototypeCatalog` · `ARTGrayboxPrototypeActor` · `URTGrayboxMapAssembler` · `URTGrayboxValidator` · `URTUnitScaleReference` · `EHeightClass` · `EOccupancy` · `FCoverData` | **zero occorrenze** |

È pubblicata come **proposta di architettura**. Pubblicarla come «vista tecnica per sviluppatori» — che è
ciò che il bundle chiedeva — avrebbe mandato qualcuno a cercare `URTGrayboxValidator` in `Source/`. È lo
stesso difetto del kit *Camera Roadmap*, già registrato in [`../README.md`](../README.md): *«nomi
plausibili di feature assenti»*.

🔴 **Tre «regole asset» dell'infografica contraddicono il canone**, e la pagina Wiki che la ospita le
corregge invece di lasciarle passare:

| L'immagine | La regola |
|---|---|
| «snap alla griglia esagonale, allineamento perfetto sempre» | `spec-hex-geometry-authoring.md` §3: *«un muro non deve seguire il perimetro di un esagono»* |
| «pivot centrato» | `D-152`: **bottom-center** per `CellBound`, centro-segmento per `EdgeBound` |
| «niente sbordo oltre il volume cella» | `EdgeBound` **non** sta nel footprint: il bordo appartiene a due celle |

🔵 **L'ingombro dell'unità aveva tre valori nei tre documenti** — `0.23 C` nel kit, `70–80 cm`
nell'handoff, `1,10–1,20 m` nell'infografica. La misura scioglie *quale sia vero oggi* e non *quale sia
giusto*: `BaseMeshScale = (1.2, 1.2, 1.8)` su un cilindro engine da 50 uu di raggio dà **1,20 m**, cioè il
valore dell'immagine — che quindi fotografa il presente e lo etichetta «consigliato», mentre il kit
chiedeva l'opposto. È `GBX-5` in [`../../../OPEN_DECISIONS.md`](../../../OPEN_DECISIONS.md).

⛔ **Le cinque pagine Wiki proposte sono diventate una.** Il bundle chiedeva `Graybox Toolkit`, `Asset
Roadmap`, `Gray Toolkit UML`, `Asset Rules & Import Contract` e `Character & Environment Art Roadmap`. Le
ultime quattro descrivono materiale che ha già un owner nel repository — la roadmap asset è `D-153`, le
regole d'ingombro sono `D-152`, le lane sono `spec-asset-pipeline.md` — e cinque pagine Wiki per un kit che
non ha ancora **un solo asset committato** sarebbero state un secondo tracker. Il bundle stesso lo vieta:
*«non deve creare roadmap o tracker paralleli»*.
