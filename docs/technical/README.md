# `docs/technical/` — quattro nature, e diciassette documenti che aspettano

> `CURRENT` · **Aggiornato**: 2026-08-19 · Owner della IA: [`../README.md`](../README.md)
> §*Le quattro nature di un file*. Questo file dice **dove sta cosa**, non cosa decidono i documenti.

| Cartella | Risponde a | n |
|---|---|--:|
| [`architecture/`](architecture/) | *com'è fatto il sistema* — classi, mappa, pathfinding, TurnLog, pipeline degli asset, navigazione frontend | 8 |
| [`systems/`](systems/) | *come si comporta un sottosistema* — hex sim, vision, bot, geometria, HUD, puntatore | 10 |
| [`tooling/`](tooling/) | *con cosa si lavora* — tactical designer, scenari, test automatici, workflow, convenzioni, tracking delle issue | 10 |
| [`runbooks/`](runbooks/) | *cosa si esegue a mano* — verifiche in editor, mandati QA, guide di seduta, diagnosi | 10 |
| *primo livello* | **tre piani e due documenti bloccati** — vedi sotto | 5 |

## Perché cinque documenti sono ancora al primo livello

**Uno è `test-manuali-pie.md`**, e ora nulla lo trattiene: va in [`runbooks/`](runbooks/), ed è un
`git mv` di una riga più i riferimenti entranti.

> 🔵 **`runbooks/` è stata aperta il 2026-08-19 con dieci documenti su dodici.** Restava fuori perché
> due erano assegnati ad altre track del write-set di batch — `test-manuali-pie.md` e
> `qa-prompt-terminal-d-verifiche-pie.md`, entrambi di `playtest`.
>
> ⚠️ **Il 2026-08-20 sono cadute entrambe le ragioni**, e per motivi diversi:
> [D-178](../decisions/RT_PDR_00_Decision_Log.md) ha rimosso il write-set di batch, quindi non esiste
> più una track che tenga un path; e `qa-prompt-terminal-d-verifiche-pie.md` **è stato eliminato** con
> gli altri tre mandati-terminale, che senza il parallelismo non avevano più un soggetto. Resta un solo
> documento da spostare, e lo spostamento non è ancora stato fatto.

**Tre sono piani, e non hanno ancora una casa.** `piano-migrazione-roster.md`,
`piano-migrazione-stable-id.md` e `piano-riduzione-hotspot.md` non sono specifiche: sono piani di
esecuzione, e il repository ne ha già due case — [`../roadmap/plans/`](../roadmap/plans/) e
[`../archive/roadmap-plans/`](../archive/roadmap-plans/), separate dal **banner**. ⚠️ I tre non
concordano fra loro: uno è `CURRENT`, uno `HISTORICAL`, e il terzo **non ha banner**. Finché il banner
è il criterio, tre documenti con tre risposte diverse non si spostano insieme.

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
