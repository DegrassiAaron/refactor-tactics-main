# `docs/technical/` — quattro nature, e diciassette documenti che aspettano

> `CURRENT` · **Aggiornato**: 2026-08-19 · Owner della IA: [`../README.md`](../README.md)
> §*Le quattro nature di un file*. Questo file dice **dove sta cosa**, non cosa decidono i documenti.

| Cartella | Risponde a | n |
|---|---|--:|
| [`architecture/`](architecture/) | *com'è fatto il sistema* — classi, mappa, pathfinding, TurnLog, pipeline degli asset | 7 |
| [`systems/`](systems/) | *come si comporta un sottosistema* — hex sim, vision, bot, geometria, HUD, puntatore | 10 |
| [`tooling/`](tooling/) | *con cosa si lavora* — tactical designer, scenari, test automatici, workflow, convenzioni | 9 |
| *primo livello* | **runbook, piani e due documenti bloccati** — vedi sotto | 17 |

## Perché diciassette documenti sono ancora al primo livello

Non è una migrazione lasciata a metà per stanchezza: sono **bloccati da
[D-139](../decisions/RT_PDR_00_Decision_Log.md)**, la regola per cui un path che sta nel `writable` di
un'altra track non si tocca.

**`runbooks/` non è stata aperta**, e la cartella non esiste. Dei dodici documenti che ci andrebbero —
`test-manuali-pie.md`, i quattro `qa-prompt-terminal-*`, le quattro `guida-*`, `debug-vs-unreal.md`,
`test-e-diagnosi.md`, `scenari-validazione-visiva.md` — **sette sono assegnati**: due alla track
`playtest`, che è `ACTIVE`, gli altri a `content_editor` e `frontend_shell`. Mezza cartella `runbooks/`
sarebbe uno stato peggiore di nessuna: chi cerca un runbook dovrebbe guardare in due posti e indovinare
il criterio. Si apre quando quelle track rilasciano, ed è un `git mv` di dodici righe.

Restano fuori per la stessa ragione **`spec-frontend-navigazione.md`** (`frontend_shell`, andrebbe in
`architecture/`) e **`issue-tracking-completeness.md`** (`content_editor`, andrebbe in `tooling/`).

E restano i tre **`piano-*.md`**, che non hanno un blocco ma una domanda aperta: `docs/roadmap/plans/`
e `docs/archive/roadmap-plans/` sono **due case per la stessa categoria**, e nessuno dei due nomi dice
quale. La decide la fase 6 di [#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165).

## Cosa è costato lo spostamento, misurato

26 documenti spostati, e i riferimenti da riscrivere sono stati **più di ventiquattro volte tanti**:

| | n |
|---|--:|
| link **entranti** verso un file spostato | 264 |
| link **uscenti** dai file spostati, scesi di un livello | 297 |
| link fra **due** file spostati in cartelle diverse | 49 |
| **etichette** che nominavano il path vecchio con il target giusto | 149 |
| `owner_specs` del Feature Registry — un contratto **macchina**, non prosa | 41 |

Le prime tre righe sono i «tre modi» che [`check-docs-links.py`](../../scripts/check-docs-links.py)
elenca da solo, e la quarta è quella che nessun controllo sui soli target vede. La quinta è la più
insidiosa: `feature_registry.py validate` è uscito **1 con 39 errori** `owner spec inesistente`, e
nessun gate sui link lo avrebbe detto — un `owner_spec` non è un link Markdown.

⚠️ **Se sposti un documento da qui, il conto è questo.** Il grep sulla forma assoluta ne vede una
frazione: i link veri sono relativi, e `../gameplay/x.md` non contiene la stringa `docs/technical/`.
