# Bundle `GrayToolkit_Cloud_Bundle` — archiviato il 2026-08-17

> `HISTORICAL` · **Materiale NON autorevole.** Si legge per sapere da dove viene una decisione. Le fonti
> autorevoli sono [`../../../decisions/RT_PDR_00_Decision_Log.md`](../../../decisions/RT_PDR_00_Decision_Log.md)
> (`D-158`), [`../../../technical/systems/spec-graybox-placement-contract.md`](../../../technical/systems/spec-graybox-placement-contract.md)
> e [`../../../technical/architecture/spec-asset-pipeline.md`](../../../technical/architecture/spec-asset-pipeline.md) §11-bis.

## Cosa conteneva

| File | Esito |
|---|---|
| `01_Graybox_Kit_Cover_CellVolume_Consolidation.md` | **non archiviato qui: era già in archivio.** Il suo `md5` è `4048a39b17513e88da41d3c7ba75aaee`, uguale a quello dell'originale nella root del checkout — ⚠️ **non** a quello del file archiviato, che porta in testa un banner di 61 righe e quindi hasha diverso: il confronto si fa sul **corpo**, non sul file. Vive in [`../CLAUDE_RefactorTactics_Graybox_Kit_Cover_CellVolume_Consolidation_2026-08-17.md`](../CLAUDE_RefactorTactics_Graybox_Kit_Cover_CellVolume_Consolidation_2026-08-17.md) |
| `02_GrayToolkit_AssetRoadmap_Wiki_Issues.md` | handoff operativo — la fonte di `D-158` |
| `03_GrayToolkit_Wiki_Pages_Original.md` | prima stesura delle pagine Wiki; superata da `04` |
| `04_GrayToolkit_Wiki_Pages_v2_Latest.md` | struttura Wiki v2, con il testo pronto delle pagine |
| `README_FIRST.md` | ordine di lettura proposto dall'autore |
| `images/RT_GrayToolkit_Public_Infographic_v2.png` | pubblicata come `images/wiki/core/24_graybox-toolkit-overview.png` |
| `images/RT_GrayToolkit_UML_Developer_v2.png` | pubblicata come `images/wiki/technical/25_graybox-toolkit-architettura-proposta.png` — il nome dice **proposta**, e il perché è sotto |

## ⚠️ Le due immagini esistono in tre posti, e uno va tolto da chi lo possiede

`git ls-files` le trova sotto:

| Path | Ruolo |
|---|---|
| `docs/archive/src/graytoolkit/images/` | **provenienza** — la copia che questo archivio conserva |
| `docs/src/wiki/graybox/…/images/` | il **bundle sorgente**, ora consumato |
| `docs/src/wiki/graybox/rt-wiki.graybox-toolkit.infografic.png` | una terza copia, con un altro nome |

Sono ~7,6 MB per due immagini. `CLAUDE.md` definisce `docs/src/` come *«input/north-star non ancora
consumato»*: dopo `D-158` quel bundle **è** consumato, quindi le due copie lì dentro descrivono uno stato
che non è più vero.

**Non le rimuovo da qui**: `docs/src/` è la cartella d'ingresso dell'autore, e ripulirla dopo il consumo è
una decisione sua — non di chi archivia. La segnalo perché il difetto è lo stesso che la riga su `01`
registra dieci righe più giù: **la stessa materia sotto più nomi**, che poi diverge senza che nessuno se ne
accorga.

---

## Cosa è entrato

**Le lane di maturità**, che nel repository si chiamano `AC0–AC6` e `AE0–AE5` — l'unica parte che non
aveva. Sono in [`../../../technical/architecture/spec-asset-pipeline.md`](../../../technical/architecture/spec-asset-pipeline.md)
§11-bis, con la regola che le rende utili: **il contratto si congela a `AC2`/`AE1`**, non a `AC6`/`AE5`.
⚠️ **I prefissi non sono quelli del bundle**, che usava `C`/`E`: collidevano con la severity degli status
(`C0`–`C3`) e con l'epic `E1`.

**Una conferma parziale della scala, che ha prodotto due correzioni.** Il **lato `1,5 m`** era già in
`convenzioni-contenuti-ue.md` §11-bis dal 2026-08-09; **`1 UU = 1 cm` no** — `grep -i 'UU'` su quel file dà
zero, ed è il default di Unreal mai dichiarato nel repository. E `spec-graybox-placement-contract.md` §6
portava «`C ≈ 173` con `HexSize` al default `100`» come se fosse *la* scala del progetto: vera del **default
del campo**, falsa della **scala d'arte**.

🔴 **E da lì è emersa una domanda che il bundle non poneva**: nessuna mappa usa la scala d'arte — girano
tutte a `1,00 m`. Le due divergono di **1,5×**, ed è `GBX-6`.

⚠️ *La prima stesura di questa riga diceva che «`1 UU = 1 cm` e lato 1,50 m erano già in §11-bis»:
l'attribuzione era falsa per metà, ed è stata ritirata in code review.*

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
giusto*: `BaseMeshScale = (1.2, 1.2, 1.8)` su un cilindro engine da 50 uu di raggio dà **120 uu**, cioè il
valore dell'immagine — che quindi fotografa il presente e lo etichetta «consigliato», mentre il kit
chiedeva l'opposto. È `GBX-5` in [`../../../OPEN_DECISIONS.md`](../../../OPEN_DECISIONS.md).

⛔ **Le cinque pagine Wiki proposte sono diventate una.** Il bundle chiedeva `Graybox Toolkit`, `Asset
Roadmap`, `Gray Toolkit UML`, `Asset Rules & Import Contract` e `Character & Environment Art Roadmap`. Le
ultime quattro descrivono materiale che ha già un owner nel repository — la roadmap asset è `D-153`, le
regole d'ingombro sono `D-152`, le lane sono `spec-asset-pipeline.md` — e cinque pagine Wiki per un kit che
non ha ancora **un solo asset graybox di mappa** sarebbero state un secondo tracker. Il bundle stesso lo vieta:
*«non deve creare roadmap o tracker paralleli»*.
