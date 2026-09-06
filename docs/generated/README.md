# `docs/generated/` — gli output, e il contratto che li tiene onesti

> `CURRENT` · **Aggiornato**: 2026-08-19 · Owner del contratto `source → generator → output → consumer`
> per **tutta** la documentazione, non solo per questa cartella.
> ⛔ **La tabella qui sotto era generata** da `docs_inventory.py --emit-contract`, uscito con
> **D-182** il 2026-08-21. Da qui in poi è **testo scritto a mano**, e invecchia come tale:
> nessun comando dice più se è indietro rispetto a ciò che descrive.

Un file in quest'area non si edita. Se lo correggi a mano, la correzione sparisce alla prossima
esecuzione del generatore — e sparisce **in silenzio**, perché nessuno riesegue il generatore per
guardare cosa cambia. Si corregge la **sorgente**, o la regola dentro il generatore.

Oggi la cartella contiene una cosa sola: [`icons/`](icons/), **635** file rigenerati il 2026-09-06 da
`tools/hud-assets/generate_hud_assets.py` (#2537). ⛔ **La storia va tenuta**: erano **296**, prodotti da uno script uscito con **D-182** — `scripts/build-icon-assets.py` — la cui geometria viveva dentro di esso e non è più nel repository. Per undici mesi nessuno ha potuto rigenerarli; ora il produttore è un altro. Ma il contratto
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
| `docs/generated/icons/` | `tools/hud-assets/generate_hud_assets.py` | `python tools/hud-assets/generate_hud_assets.py --out <dir>` | ⛔ **nessuno** | la geometria vive nel generatore. ⚠️ La struttura prodotta (`Icons/` piatto) **non è** quella di questa cartella (`svg/` + `png/&lt;size&gt;/`): serve una rimappatura; la dimensione **96** non è più producibile; e le **icone d'eroe** che il generatore produce restano **fuori** da questa cartella, che non ne ha mai avute, finché [#2297](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2297) non ne fissa i nomi |

> **Nessun blocco generato vive dentro un documento scritto a mano.** I tre che c'erano —
> `RT_SUITE_COUNT`, `RT_FEATURE_BY_EPIC`, `RT_FEATURE_STATUS` — sono usciti col Feature Registry
> (**D-181**, 2026-08-21).

✅ **Gli artefatti con un generatore vivo sono due**: i radar e — dal 2026-09-06 — le icone. ⚠️ **Ma solo i radar hanno un `--check`**: `tools/hud-assets/generate_hud_assets.py` espone il solo `--out`, quindi nessuno può ancora dire se le 635 icone corrispondono alla geometria che le ha prodotte. Rigenerare e confrontare a mano è l'unico modo, ed è ciò che #2537 ha fatto una volta.

## Come si legge, e cosa manca

**`--check` è la colonna che conta.** Dice se esiste un modo di scoprire che l'output è più vecchio
della sorgente. Dove c'è scritto **nessuno**, quell'artefatto non ha modo di dichiararsi stantio: la
riga non è una lacuna nascosta, è la lacuna **scritta**. ⛔ Le cinque `*.shortlist.md` erano l'unico caso
`nessuno` **dichiarato**, e sono uscite con **D-181**. ⚠️ Oggi il caso peggiore è un altro: la riga
`docs/generated/icons/` non ha `nessuno` ma un `—`, perché il generatore stesso è stato rimosso. Delle
due righe rimaste, **una sola** ha un `--check` vivo.

⚠️ **Un `--check` poteva esistere ed essere morto** — e ora quello non esiste affatto.
`build-icon-assets.py --check` usciva `1` da prima della fase 2 di
[#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165), per una causa sua: cercava
un simbolo C++ che `Source/` non ha più. ⛔ Lo script è uscito con **D-182** il 2026-08-21, quindi il
`--check` morto è diventato un `--check` assente — e le icone, allora **296**, restavano senza modo di verificarle. Era
[#1198](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1198), **oggi chiusa** — ma chiuderla non ha restituito
un oracolo, e la frase che diceva *«finché è aperta»* è rimasta a indicare una condizione che non esiste più. ⚠️ **Il fatto sopravvive al suo riferimento**: dopo la rigenerazione del 2026-09-06 i master sono **635** e continuano a non avere un `--check`. Averlo scritto in tabella non basta: il gate verifica che il generatore
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
