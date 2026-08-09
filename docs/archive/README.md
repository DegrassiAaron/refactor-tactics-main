# Archivio — materiale NON autorevole

I file in questa cartella sono conservati per **storia e provenienza**, ma **non** sono la fonte di verità.
Sono il livello 9 — l'ultimo — della gerarchia in [`../README.md`](../README.md).

La fonte di verità del progetto è:

- [`../product/piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) — invarianti e decisioni vincolanti;
- [`../decisions/`](../decisions/) — ADR e [Decision Log](../decisions/RT_PDR_00_Decision_Log.md);
- [`../roadmap/roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md) — milestone e stato;
- [`../../CLAUDE.md`](../../CLAUDE.md) e [`../../AGENTS.md`](../../AGENTS.md) — guide operative.

> ⚠️ **Corretto il 2026-08-08**: questo file indicava come fonti `docs/design/piano-canonico-mvp.md` e
> `docs/design/roadmap-checkpoint.md`. **La cartella `docs/design/` non esiste** — non è mai esistita con quel
> nome nella struttura corrente. Era un indice che mandava fuori strada proprio chi lo consultava per capire
> dove *non* cercare.

## Contenuto

| Cartella / file | Cos'è |
|---|---|
| [`src/`](src/README.md) | **I 28 sorgenti recepiti** — `design/`, `handoff/`, `audit/`, spostati qui dal 2026-08-08. Hanno un indice proprio con la colonna «Recepito da». Dal 2026-08-09 include anche i sorgenti **revisionati e non applicati**, con l'esito in testa al file |
| `pdr-v0.1/` | Snapshot PDF dei dodici PDR `v0.1`. Le sorgenti testuali vivono in Git (D-009); questi restano di consultazione |
| `gameplay/` | Spec superate: terreni, knockback, bot utility, sequenza turno esplorativa |
| `technical/` | Le versioni **a griglia quadrata** di mappa multilivello e pathfinding, superate dal pivot esagonale (ADR-0002) |
| `session-notes/` | Note di sessione e handoff datati |
| `plan-variant-chatgpt.md` | *(ex `docs/claude.md`)* PRD/piano alternativo **in conflitto** col piano canonico: assume C#/UnrealSharp, formato 4v4, team 5-10 persone, scope molto ampio (mod.io/Steam Workshop, roguelike/deckbuilding, schema DB, Unreal Horde). Superato: il canone usa C++/Blueprint, MVP 2v2 offline, dev singolo |
| `CLAUDE_RefactorTactics-original.md` | *(ex `docs/CLAUDE_RefactorTactics.md`)* guida operativa **assorbita** nel `CLAUDE.md` a radice. Il catalogo comandi `/sc:*` vive in [`../superclaude-cheatsheet.md`](../superclaude-cheatsheet.md) |

## La regola che vale qui

Un documento archiviato che descrive un mondo scomparso **non è un difetto da correggere**: riscriverlo
falsificherebbe la storia. La correzione va nel documento `CURRENT` che possiede la regola; allo storico basta
un rimando in testa.

Il difetto vero è l'opposto — uno storico **senza etichetta**, che si legge come se fosse la specifica di
oggi. Ed è il motivo per cui l'intestazione di questo file è stata corretta invece che lasciata com'era: non
raccontava la storia di una cartella, dava indicazioni operative sbagliate al presente.
