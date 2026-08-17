# RefactorTactics — Gray Toolkit Cloud Bundle

> ## 🗄️ `HISTORICAL` — sorgente archiviato il **2026-08-17**
>
> **Materiale NON autorevole**: è l'ordine di lettura proposto dall'autore del bundle, conservato per la
> provenienza. L'esito del consumo sta in [`README.md`](README.md) e in
> [`D-158`](../../../decisions/RT_PDR_00_Decision_Log.md).

Data: 2026-08-17

Questo bundle raccoglie il materiale consolidato da passare a Claude per:

- audit repository-first;
- consolidamento delle decisioni Graybox / Gray Toolkit;
- Cell Placement Volume;
- Cover visual grammar;
- world scale e asset contract;
- Asset Roadmap;
- creazione/aggiornamento Issue ed Epic senza duplicati;
- aggiornamento Roadmap, Feature Registry, Asset lane ed Editor lane;
- creazione/aggiornamento Wiki;
- integrazione delle ultime immagini approvate.

## File consigliato da usare per primo

`docs/02_GrayToolkit_AssetRoadmap_Wiki_Issues.md`

È l'handoff operativo più completo per:
1. audit;
2. issue/epic;
3. roadmap;
4. documentazione;
5. wiki.

Per la struttura Wiki più recente usare:

`docs/04_GrayToolkit_Wiki_Pages_v2_Latest.md`

## Immagini approvate

### Overview / pubblico
`images/RT_GrayToolkit_Public_Infographic_v2.png`

Uso:
- pagina Graybox Toolkit;
- overview;
- onboarding;
- Asset Roadmap high-level.

### UML / sviluppatori
`images/RT_GrayToolkit_UML_Developer_v2.png`

Uso:
- pagina Graybox Toolkit UML;
- Asset Rules & Import Contract;
- issue / epic;
- documentazione tecnica.

## Ordine consigliato per Claude

1. Leggere `README_FIRST.md`.
2. Leggere `docs/02_GrayToolkit_AssetRoadmap_Wiki_Issues.md`.
3. Leggere `docs/04_GrayToolkit_Wiki_Pages_v2_Latest.md`.
4. Usare `docs/01_Graybox_Kit_Cover_CellVolume_Consolidation.md` come dettaglio tecnico delle decisioni precedenti.
5. Usare `docs/03_GrayToolkit_Wiki_Pages_Original.md` solo come storico/confronto.
6. Auditare `main` prima di creare nuovi owner, issue o pagine.
7. Copiare le immagini nella directory canonica della documentazione/wiki.
8. Rigenerare eventuali viste derivate con il tooling della repository.

## Nota importante

La repository live è la source of truth.
Questo bundle è un handoff operativo e di consolidamento: non deve creare roadmap o tracker paralleli.
