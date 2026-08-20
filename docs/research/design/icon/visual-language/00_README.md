# RefactorTactics — Visual Language

> **Statuto**: `docs/src/` contiene sorgenti **non ancora consumati** e **non è fonte autorevole**
> (`AGENTS.md` §34). Il canone dell'iconografia è
> [D-031](../../../../decisions/RT_PDR_00_Decision_Log.md) più l'enum `ERTIconCategory` in
> `Source/RefactorTactics/UI/RTIconCatalogData.h`. Dove questi file divergono dal codice o dal Decision Log,
> prevalgono codice e Decision Log.

## Documentazione consolidata

Prodotta il **2026-08-12** consolidando il pack sorgente elencato più sotto. Definisce **come** un concetto
viene rappresentato; **dove e quando** appare è di
[`progettazione-hud.md`](../../../../technical/systems/progettazione-hud.md).

| File | Contenuto |
|---|---|
| [`01-principi.md`](01-principi.md) | i due assi, principi LOCKED, griglia |
| [`02-color-system.md`](02-color-system.md) | palette semantica, temi |
| [`03-forme-e-primitive.md`](03-forme-e-primitive.md) | Target · Geometry · Effect · Movement · Modifier |
| [`04-regole-di-composizione.md`](04-regole-di-composizione.md) | la formula, slot art, payload/stato/superficie |
| [`05-certainty-states.md`](05-certainty-states.md) | Confirmed · Predicted · Uncertain · Invalid |
| [`06-accessibilita.md`](06-accessibilita.md) | grayscale, CVD, dimensioni reali, densità |
| [`07-export-e-naming.md`](07-export-e-naming.md) | `IconId`, naming Unreal, formati |
| [`08-catalogo-v0.1.md`](08-catalogo-v0.1.md) | le chiavi risolvibili e il set obbligatorio |

### La separazione che regge tutto il resto

**Grammatica** e **catalogo** sono due assi distinti. `Target + Geometry + Effect + Modifier` è il vocabolario
con cui si **disegna** un glifo; `ERTIconCategory` è l'indice con cui un widget lo **risolve**. Nessun widget
chiederà mai `UI.Icon.Geometry.Line`: chiederà `UI.Icon.Action.Dash`, che è *disegnata* con una `Line`.

Le primitive non hanno un `IconId`.

## Pack sorgente

Handoff dell'autore, **non canone**. Resta come provenienza; le regole recepite stanno nei file qui sopra.

- [`CLAUDE_CLI_RT_VisualLanguage_Roadmap.md`](CLAUDE_CLI_RT_VisualLanguage_Roadmap.md) — handoff operativo repo/GitHub;
- [`RT_VisualLanguage_Epics_Issues.md`](RT_VisualLanguage_Epics_Issues.md) / [`.yaml`](RT_VisualLanguage_Epics_Issues.yaml) — mappa issue/epic candidate;
- `../CLAUDE_DESIGN_RT_VisualLanguage_MASTER.md` — regole globali;
- `../CLAUDE_DESIGN_01…04_*.md` — grammatica, manifest, asset statici, batch di produzione;
- `../RefactorTactics_UI_Icon_Manifest_v0.1.csv` — 184 voci.

### ⚠️ Il pack è incompleto

Questo README, nella versione originale, indicizzava **dodici brief** `ClaudeDesign/ICON-0…ICON-11.md` e una
cartella `References/`. **Nessuno dei due è mai arrivato nel repository** — verificato il 2026-08-12 con una
ricerca su tutto l'albero.

Confermato dall'autore il **2026-08-12**: quei file **non esistono**. L'indice originale è stato rimosso da
questo README perché puntava a documenti inesistenti. `ICON-0 — Audit e consolidamento` è stato eseguito
direttamente, e il suo esito è la documentazione consolidata qui sopra.

### ⚠️ Il manifest sorgente non passa il validator

Misura sulle 184 voci del CSV, contro `URTIconLibrary::ValidateIconCatalog`:

| Esito | Voci |
|---|---:|
| Passano | 73 |
| Segmento di categoria fuori da `ERTIconCategory` | 87 |
| Forma non `UI.Icon.*` | 24 |

I nomi da correggere prima di usarlo: `Map` → `MapInteraction`, `Intel` → `Information`,
`UI.Style.Certainty.*` → `UI.Icon.Certainty.*`. `Target`, `Geometry`, `Effect`, `Surface` e `Stat` non sono
categorie: sono primitive o chrome, e appartengono agli altri due assi.

Dettaglio in [`08-catalogo-v0.1.md`](08-catalogo-v0.1.md).

## Stato roadmap

`E20` (`#217`–`#220`) e `E25` (`#265`–`#269`) **esistono già** in `feature-registry.yaml` e in
`roadmap-post-v0.1.md`, con i quattro checkpoint che l'handoff proponeva di creare. Non ci sono issue da
aprire per questi due epic: l'handoff è più vecchio della roadmap.
