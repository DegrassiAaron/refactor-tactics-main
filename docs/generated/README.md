# `docs/generated/` — gli output, e il contratto che li tiene onesti

> `CURRENT` · **Aggiornato**: 2026-08-19 · Owner del contratto `source → generator → output → consumer`
> per **tutta** la documentazione, non solo per questa cartella.
> La tabella qui sotto è **generata**: `python scripts/docs_inventory.py --emit-contract`.

Un file in quest'area non si edita. Se lo correggi a mano, la correzione sparisce alla prossima
esecuzione del generatore — e sparisce **in silenzio**, perché nessuno riesegue il generatore per
guardare cosa cambia. Si corregge la **sorgente**, o la regola dentro il generatore.

Oggi la cartella contiene una cosa sola: [`icons/`](icons/), i 296 master iconografici prodotti da
`scripts/build-icon-assets.py` da una geometria dichiarata nel generatore stesso. Ma il contratto
riguarda ogni artefatto generato di `docs/`, ovunque viva — i radar stanno in `characters/`, le
shortlist in `roadmap/`, e due blocchi vivono **dentro** documenti scritti a mano.

## Il contratto

<!-- RT_CONTRATTO_GENERATI:BEGIN -->

| Output | Generatore | Comando | `--check` | Sorgenti |
|---|---|---|---|---|
| `docs/roadmap/feature-registry.json` | `scripts/feature_registry.py` | `python scripts/feature_registry.py generate` | `python scripts/feature_registry.py generate --check` | `docs/roadmap/feature-registry.yaml` |
| `docs/roadmap/project-graph.json` | `scripts/feature_registry.py` | `python scripts/feature_registry.py generate` | `python scripts/feature_registry.py generate --check` | `docs/roadmap/feature-registry.yaml` · `docs/roadmap/editor-sessions.yaml` · `docs/roadmap/execution-graph.yaml` · `docs/roadmap/roadmap-v0.1.md` · `docs/roadmap/roadmap-checkpoint.md` · `Scenarios/` |
| `docs/roadmap/charts/roadmap-map.svg` | `scripts/feature_registry.py` | `python scripts/feature_registry.py generate` | `python scripts/feature_registry.py generate --check` | `docs/roadmap/feature-registry.yaml` |
| `docs/roadmap/*.shortlist.md` | `scripts/feature_registry.py` | `python scripts/feature_registry.py shortlist` | **nessuno** | `docs/roadmap/feature-registry.yaml` · `docs/roadmap/editor-sessions.yaml` · `docs/roadmap/roadmap-v0.1.md` · `Scenarios/` |
| `docs/characters/radar/*.svg` | `tools/radar/generate.ts` | `node tools/radar/generate.ts` | `node tools/radar/generate.ts --check` | `docs/balance/` |
| `docs/generated/icons/` | `scripts/build-icon-assets.py` | `python scripts/build-icon-assets.py` | `python scripts/build-icon-assets.py --check` | `scripts/build-icon-assets.py (la geometria e' dichiarata nel generatore)` · `Source/RefactorTactics/Ability/RTCatalogLibrary.cpp` |

| Blocco | Comando | Dentro |
|---|---|---|
| `RT_FEATURE_BY_EPIC` | `python scripts/feature_registry.py generate` | `docs/roadmap/roadmap-v0.1.md` |
| `RT_FEATURE_STATUS` | `python scripts/feature_registry.py deploy` | `docs/characters/index.md` · `docs/characters/v0.1/*.md` |
| `RT_SUITE_COUNT` | `python scripts/feature_registry.py suite` | `docs/README.md` · `docs/roadmap/roadmap-v0.1.md` |

<!-- RT_CONTRATTO_GENERATI:END -->

## Come si legge, e cosa manca

**`--check` è la colonna che conta.** Dice se esiste un modo di scoprire che l'output è più vecchio
della sorgente. Dove c'è scritto **nessuno**, quell'artefatto non ha modo di dichiararsi stantio: la
riga non è una lacuna nascosta, è la lacuna **scritta**. Le cinque `*.shortlist.md` sono in questo
stato — si rigenerano con `shortlist`, e nessun comando dice se sono indietro.

⚠️ **Un `--check` può esistere ed essere morto.** `build-icon-assets.py --check` esce `1` da prima
della fase 2 di [#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165), per una
causa sua: cerca un simbolo C++ che `Source/` non ha più. È
[#1198](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1198), e finché è aperta i 296
master non hanno un oracolo. Averlo scritto in tabella non basta: il gate verifica che il generatore
**implementi** davvero l'opzione che dichiara, non che il comando esca `0`.

## I blocchi dentro i documenti scritti a mano

Sono la forma più facile da rompere: il file è authored, la porzione fra i due marcatori no.
Rigenerare **cancella** ciò che qualcuno ha scritto dentro il blocco, e un `--check` che confronta il
file intero non distingue le due cose. La regola è una: dentro `<!-- RT_*:BEGIN -->` e
`<!-- RT_*:END -->` non si scrive a mano, mai — nemmeno una riga di commento.

🔴 **Un marcatore può sopravvivere a un documento archiviato.**
`docs/archive/roadmap-plans/editormap-spec.md` contiene un `RT_SHORTLIST_EDITOR:BEGIN`, ed è una copia
storica: `feature_registry.py shortlist` scrive **solo** dentro `docs/roadmap/`, verificato leggendo
`SHORTLIST_TARGETS`. Oggi è innocuo, ma è un innesco: basterebbe che qualcuno estendesse i target «a
tutti i file che hanno il marcatore» perché un generatore riscrivesse un documento d'archivio, cioè
falsificasse la storia. Il marcatore resta perché l'archivio non si corregge; questa riga esiste
perché la prossima persona non lo scopra da sola.

## Perché il contratto vive qui

Fino al 2026-08-20 questa domanda aveva un'alternativa: `parallel-batch.yaml`, il write-set del lotto di
sessioni parallele, aveva un `generated_only` con `derives_from` che diceva *«questi path non si
assegnano, seguono la sorgente»*. Era **governance del lotto**, e si riscriveva a ogni batch — «vive
quanto il batch, non è un registro storico». Un contratto che scade quando finisce un lotto di sessioni
non è un contratto, e infatti quel file non esiste più
([D-178](../decisions/RT_PDR_00_Decision_Log.md)).

Qui c'è la **provenienza**, che non scade. E siccome due elenchi della stessa cosa divergono alla prima
aggiunta — questo repository l'ha già pagato con i gate elencati per nome in `AGENTS.md`, sei di cui
ne conosceva tre — la tabella non è scritta a mano: nasce da `CONTRATTI` in
[`../../scripts/docs_inventory.py`](../../scripts/docs_inventory.py), e
`python scripts/docs_inventory.py --check` fallisce se qualcuno la modifica qui.
