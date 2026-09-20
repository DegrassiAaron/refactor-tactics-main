# `docs/technical/` — una cartella per natura, e `test-manuali-pie.md` fermo al primo livello

> `CURRENT` · **Aggiornato**: 2026-08-19 · Owner della IA: [`../README.md`](../README.md)
> §*Le quattro nature di un file*. Questo file dice **dove sta cosa**, non cosa decidono i documenti.

> 📐 **Convenzione di questa pagina**: `xx` è un conteggio che **non si scrive**, perché cambia da solo
> ([`../../AGENTS.md`](../../AGENTS.md) §14). Accanto c'è sempre il comando che lo produce — un `xx` qui
> non è un dato mancante, è un dato da misurare.

| Cartella | Risponde a | n |
|---|---|--:|
| [`architecture/`](architecture/) | *com'è fatto il sistema* — classi, mappa, pathfinding, TurnLog, pipeline degli asset, navigazione frontend | `xx` |
| [`systems/`](systems/) | *come si comporta un sottosistema* — hex sim, vision, bot, geometria, HUD, puntatore, privacy di rete | `xx` |
| [`tooling/`](tooling/) | *con cosa si lavora* — tactical designer, scenari, test automatici, workflow, convenzioni, tracking delle issue | `xx` |
| [`runbooks/`](runbooks/) | *cosa si esegue a mano* — verifiche in editor, mandati QA, guide di seduta, diagnosi | `xx` |
| [`evidence/`](evidence/) | *cosa è stato misurato* — log e screenshot allegati a una issue: **provenienza, non regole**, e non sono Markdown | `xx` |
| [`img/`](img/) | riferimenti visuali | `xx` |
| *primo livello* | piani e `test-manuali-pie.md` — vedi sotto | `xx` |

```sh
for d in architecture systems tooling runbooks evidence img; do
  echo "$d: $(ls docs/technical/$d/*.md 2>/dev/null | wc -l) md · $(git ls-files docs/technical/$d | wc -l) file"
done
ls docs/technical/*.md | wc -l          # primo livello
```

> 🔴 **Rimisurata il 2026-09-20: due righe su cinque erano stantie, e una cartella non c'era.**
> La tabella diceva `architecture` **8** · `systems` 17 · `tooling` 10 · `runbooks` **10** · primo livello
> **5**; il comando qui sopra, eseguito oggi, dà `architecture` **9** · `runbooks` **18** · primo livello
> **8**. ⚠️ E [`evidence/`](evidence/) non era elencata affatto: è nata dopo, e un indice che non nomina
> una cartella non la rende invisibile solo a chi legge — la rende invisibile anche a chi decide dove
> mettere un file.
>
> ⛔ **Il rimedio non è un numero più fresco.** La nota che stava qui — *«chi passa di qui con un mandato
> documentale le allinei»* — era già il terzo giro di riallineamento a mano, e ognuno si è aperto
> dichiarando la deriva che il precedente aveva appena chiuso. `AGENTS.md` §14 dice perché: **non c'è un
> gate che rimisuri un numero in prosa**. Qui il numero smette di essere scritto.
>
> ⌫ *La misura del 2026-09-04, conservata come cronaca*: `systems` era stata corretta da **16 → 17** con
> [#589](https://github.com/DegrassiAaron/refactor-tactics-main/issues/589) e le altre righe erano già
> stantie allora — `runbooks` **10 → 12**, primo livello **5 → 7**. Il titolo prometteva *«diciassette
> documenti che aspettano»*, una cifra che nessuno ha mai verificato e di cui non si sa quale grandezza
> contasse: è stata tolta invece di essere corretta.

## Perché ci sono ancora documenti al primo livello

**Uno è `test-manuali-pie.md`**, e va in [`runbooks/`](runbooks/). ⛔ **Non è più «un `git mv` di una
riga»**, ed è la correzione che questa sezione porta.

> 🔵 **`runbooks/` è stata aperta il 2026-08-19 con dieci documenti su dodici.** Restava fuori perché
> due erano assegnati ad altre track del write-set di batch — `test-manuali-pie.md` e
> `qa-prompt-terminal-d-verifiche-pie.md`, entrambi di `playtest`.
>
> ⚠️ **Il 2026-08-20 sono cadute entrambe le ragioni**, e per motivi diversi:
> [D-178](../decisions/RT_PDR_00_Decision_Log.md) ha rimosso il write-set di batch, quindi non esiste
> più una track che tenga un path; e `qa-prompt-terminal-d-verifiche-pie.md` **è stato eliminato** con
> gli altri tre mandati-terminale, che senza il parallelismo non avevano più un soggetto.
>
> 🔴 **Ma il 2026-08-21 se n'è aggiunta una terza, e nessuno l'ha scritta qui.** [D-182](../decisions/RT_PDR_00_Decision_Log.md)
> ha rimosso `scripts/`, e con essa i **tre riscrittori** che il piano di #1165 prescriveva per questo
> spostamento (*«79 link entrano, quindi si fa coi tre riscrittori e non a mano»*). Oggi cercarli non
> produce nulla: l'unica occorrenza della parola nel repository è il messaggio di commit che li nominava.
>
> **Misurato il 2026-09-20** — chi cita il file:
>
> ```sh
> git grep -l 'test-manuali-pie' -- . | grep -v '^docs/technical/test-manuali-pie.md$' | wc -l
> git grep -l 'test-manuali-pie' -- . | grep -v '^docs/' | grep -v '^docs/technical/test-manuali-pie.md$'
> ```
>
> Il primo comando dà **185** file. Il secondo ne isola **quindici fuori da `docs/`**, e sono la ragione
> per cui lo spostamento non appartiene a un mandato documentale:
> `Source/RefactorTactics/Camera/RTCameraPawn.h` · `Source/RefactorTactics/Tests/RTFrontendWidgetAssetTests.cpp` ·
> cinque `Scenarios/Visual/**/*.json` · `tools/editor-sessions/{README.md,build_agenda.py,pie_status.py,registry.py}` ·
> `tools/radar/doc-tables.ts` · `AGENTS.md` · `.gitignore` ·
> `.claude/skills/worktree-issue-runner/SKILL.md`.
>
> ⚠️ **E quei quindici sono esattamente la classe che nessun gate vede**: [#1232](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1232)
> l'ha già misurata — un puntatore che vive in un `.h`, in un `.json` o in un `.py` è invisibile per
> costruzione a [`doc-links.ts`](../../tools/radar/doc-links.ts), che cammina sui Markdown. Uno
> spostamento fatto «a mano coi soli documenti» li lascerebbe indietro **senza che niente diventi rosso**.
>
> ∴ **Resta un solo documento da spostare, e lo spostamento vuole un mandato che possa toccare `Source/`,
> `Scenarios/` e `tools/`** — non uno documentale. Finché non lo ha, il file sta qui e il motivo è scritto.

**Cinque sono piani**, e la cartella dei piani è altrove — [`../roadmap/plans/`](../roadmap/plans/) e
[`../archive/roadmap-plans/`](../archive/roadmap-plans/), separate dal **banner**:

| Piano | Banner |
|---|---|
| [`piano-migrazione-roster.md`](piano-migrazione-roster.md) | `CURRENT` |
| [`piano-migrazione-stable-id.md`](piano-migrazione-stable-id.md) | `HISTORICAL` — ritirato il 2026-08-13 |
| [`piano-riduzione-hotspot.md`](piano-riduzione-hotspot.md) | nessun banner; la parte tooling è decaduta con `D-181` |
| [`piano-derivazioni-azioni-eroe.md`](piano-derivazioni-azioni-eroe.md) | nessun banner — apre con **Stato: implementato** ([#1406](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1406)) |
| [`piano-scarto-dichiarato.md`](piano-scarto-dichiarato.md) | nessun banner — apre con ⛔ **Eseguito** |

⚠️ **Finché il banner è il criterio, cinque documenti con quattro risposte diverse non si spostano
insieme.** ⌫ *Fino al 2026-09-20 questa sezione ne contava tre*: `piano-derivazioni-azioni-eroe.md` e
`piano-scarto-dichiarato.md` non erano nominati, e sono i due che dichiarano il proprio stato **in prosa
invece che in un banner** — cioè proprio quelli che un criterio basato sul banner non vede.

**Uno non è né l'uno né l'altro**: [`asset-licenze.md`](asset-licenze.md) è un **registro di
provenienza**, e dichiara in testa il proprio owner —
[`architecture/spec-asset-pipeline.md`](architecture/spec-asset-pipeline.md) §8, `FR-ASSET-LIC-01`. Un
registro alimentato da una spec non è una spec: sta al primo livello per questo, e non per dimenticanza.

## ⛔ Non esiste una cartella `qa/`, ed è una decisione

Il piano di [#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165) ne prevedeva
una, *prima* che `technical/` si dividesse. Dopo la divisione il materiale QA è già distribuito:
`test-automatico-unreal.md`, `scenario-map.md` e `scenario-index-e-tag.md` stanno in `tooling/`, i
mandati e le verifiche in `runbooks/`. Un `qa/` di primo livello sarebbe un **terzo asse** sugli stessi
file — `test-automatico-unreal.md` è insieme «strumento» e «QA», e dovrebbe stare in due posti.

È la versione-cartella di *un concetto, un owner*: la cartella dice **che natura ha** un documento, non
di quale disciplina parla. Della disciplina rispondono i link.

## Cosa è costato lo spostamento, misurato

26 documenti spostati, e i riferimenti da riscrivere sono stati **più di ventiquattro volte tanti**:

| | n |
|---|--:|
| link **entranti** verso un file spostato | 264 |
| link **uscenti** dai file spostati, scesi di un livello | 297 |
| link fra **due** file spostati in cartelle diverse | 49 |
| **etichette** che nominavano il path vecchio con il target giusto | 149 |
| `owner_specs` del Feature Registry — un contratto **macchina**, non prosa | 41 |

Le prime tre righe sono i «tre modi» che `check-docs-links.py` *(rimosso con **D-182**)*
elenca da solo, e la quarta è quella che nessun controllo sui soli target vede. La quinta è la più
insidiosa: `feature_registry.py validate` è uscito **1 con 39 errori** `owner spec inesistente`, e
nessun gate sui link lo avrebbe detto — un `owner_spec` non è un link Markdown.

⚠️ **Se sposti un documento da qui, il conto è questo.** Il grep sulla forma assoluta ne vede una
frazione: i link veri sono relativi, e `../gameplay/x.md` non contiene la stringa `docs/technical/`.
