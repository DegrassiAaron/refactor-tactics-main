# `docs/generated/` — gli output, e il contratto che li tiene onesti

> `CURRENT` · **Aggiornato**: 2026-08-19 · Owner del contratto `source → generator → output → consumer`
> per **tutta** la documentazione, non solo per questa cartella.
> ⛔ **La tabella qui sotto era generata** da `docs_inventory.py --emit-contract`, uscito con
> **D-182** il 2026-08-21. Da qui in poi è **testo scritto a mano**, e invecchia come tale:
> nessun comando dice più se è indietro rispetto a ciò che descrive.

Un file in quest'area non si edita. Se lo correggi a mano, la correzione sparisce alla prossima
esecuzione del generatore — e sparisce **in silenzio**, perché nessuno riesegue il generatore per
guardare cosa cambia. Si corregge la **sorgente**, o la regola dentro il generatore.

Oggi la cartella contiene una cosa sola: [`icons/`](icons/), i 296 master iconografici prodotti da
un generatore uscito con **D-182**: la geometria che li ha prodotti viveva dentro di esso, e non è più nel repository. Ma il contratto
riguarda ogni artefatto generato di `docs/`, ovunque viva — oggi i radar in `characters/` e le icone
qui. ⛔ **Fino al 2026-08-21 c'erano anche le cinque `*.shortlist.md` di `roadmap/` e tre blocchi che
vivevano dentro documenti scritti a mano**: sono usciti col Feature Registry (**D-181**), e la tabella
qui sotto è passata da cinque voci a due.

## Il contratto

⛔ **I marcatori sono stati tolti con D-182**: erano il perimetro che un generatore riscriveva, e il
generatore non c'è più. Lasciarli avrebbe promesso una rigenerazione che non può avvenire.

| Output | Generatore | Comando | `--check` | Sorgenti |
|---|---|---|---|---|
| `docs/characters/radar/*.svg` | `tools/radar/generate.ts` | `node tools/radar/generate.ts` | `node tools/radar/generate.ts --check` | `docs/balance/` |
| ~~`docs/generated/icons/`~~ | ⛔ ~~`scripts/build-icon-assets.py`~~ | rimosso con **D-182** | — | la geometria era **dentro** il generatore, e non è più nel repository |

> **Nessun blocco generato vive dentro un documento scritto a mano.** I tre che c'erano —
> `RT_SUITE_COUNT`, `RT_FEATURE_BY_EPIC`, `RT_FEATURE_STATUS` — sono usciti col Feature Registry
> (**D-181**, 2026-08-21).

⚠️ **Resta un solo artefatto generato con un generatore vivo**: i radar. Le 296 icone restano
committate e nessuno può più dire se corrispondono alla geometria che le ha prodotte.

## Come si legge, e cosa manca

**`--check` è la colonna che conta.** Dice se esiste un modo di scoprire che l'output è più vecchio
della sorgente. Dove c'è scritto **nessuno**, quell'artefatto non ha modo di dichiararsi stantio: la
riga non è una lacuna nascosta, è la lacuna **scritta**. ⛔ Le cinque `*.shortlist.md` erano l'unico
caso `nessuno`, e sono uscite con **D-181**: oggi ogni riga della tabella ha un `--check`.

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
storica. ⛔ **Il generatore che avrebbe potuto riscriverla non esiste più** — `feature_registry.py` è
uscito con **D-181** il 2026-08-21 — quindi l'innesco è spento *oggi*. La lezione resta e vale per il
prossimo generatore: basterebbe estendere i target «a tutti i file che hanno il marcatore» perché un
documento d'archivio venisse riscritto, cioè perché la storia venisse falsificata. Il marcatore resta perché l'archivio non si corregge; questa riga esiste
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
un gate che non esiste più (**D-182**): oggi nulla fallisce se qualcuno la modifica qui.
