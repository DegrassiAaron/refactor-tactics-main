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
| [`src/`](src/) | **I 69 sorgenti recepiti** — `design/`, `handoff/`, `audit/`, spostati qui dal 2026-08-08. Hanno un indice proprio con la colonna «Recepito da». Dal 2026-08-09 include anche i sorgenti **revisionati e non applicati**, con l'esito in testa al file. *(Diceva **28** fino al 2026-08-14: un totale scritto a mano che nessun gate confronta con la cartella. Rimisurato con `find docs/archive/src -name '*.md' ! -name README.md \| wc -l` → **69**, che è anche ciò che `src/README.md` dichiara.)* |
| [`roadmap-plans/`](roadmap-plans/README.md) | **I 20 piani e referti storici** spostati da `docs/roadmap/plans/` il 2026-08-14 — **10** `HISTORICAL` e **10** `SNAPSHOT`. ⚠️ Il criterio è il **banner**, non la data nel nome: quella ne coglieva 35 su 68, e di quei 35 ne prendeva **22 dichiarati `CURRENT`**. I `CURRENT` restano in [`../roadmap/plans/`](../roadmap/plans/README.md), che ha un indice del criterio |
| [`pdr-v0.1/`](pdr-v0.1/RT_PDR_v0.1_consolidato.md) | Il corpus dei tredici PDR `v0.1` in **un solo Markdown**, con la tabella «cosa vale oggi» documento per documento. I PDF sono stati rimossi il 2026-08-12 e restano nella storia Git |
| `gameplay/` | Spec superate: terreni, knockback, bot utility, sequenza turno esplorativa |
| `technical/` | Le versioni **a griglia quadrata** di mappa multilivello e pathfinding, superate dal pivot esagonale (ADR-0002) |
| `session-notes/` | Note di sessione e handoff datati |
| `plan-variant-chatgpt.md` | *(ex `docs/claude.md`)* PRD/piano alternativo **in conflitto** col piano canonico: assume C#/UnrealSharp, formato 4v4, team 5-10 persone, scope molto ampio (mod.io/Steam Workshop, roguelike/deckbuilding, schema DB, Unreal Horde). Superato: il canone usa C++/Blueprint, MVP 2v2 offline, dev singolo |
| `CLAUDE_RefactorTactics-original.md` | *(ex `docs/CLAUDE_RefactorTactics.md`)* guida operativa **assorbita** nel `CLAUDE.md` a radice. Il catalogo comandi `/sc:*` vive in [`../superclaude-cheatsheet.md`](../superclaude-cheatsheet.md) |

> ⚠️ **Corretto il 2026-08-12**: la riga `pdr-v0.1/` diceva «*le sorgenti testuali vivono in Git (D-009)*».
> Era vero per **due** pezzi su tredici — il [Decision Log](../decisions/RT_PDR_00_Decision_Log.md) (PDR-00 §4)
> e [PDR-10 v0.2](../roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md). Per gli altri undici documenti il PDF era
> l'unica copia esistente, e la riga rassicurava chi la leggeva su una cosa che non era stata fatta. Ora lo è:
> [`RT_PDR_v0.1_consolidato.md`](pdr-v0.1/RT_PDR_v0.1_consolidato.md).

## La regola che vale qui

Un documento archiviato che descrive un mondo scomparso **non è un difetto da correggere**: riscriverlo
falsificherebbe la storia. La correzione va nel documento `CURRENT` che possiede la regola; allo storico basta
un rimando in testa.

Il difetto vero è l'opposto — uno storico **senza etichetta**, che si legge come se fosse la specifica di
oggi. Ed è il motivo per cui l'intestazione di questo file è stata corretta invece che lasciata com'era: non
raccontava la storia di una cartella, dava indicazioni operative sbagliate al presente.

### Se incontri un path `*.pdf` in un documento archiviato

I ventitré PDF di `docs/` sono stati rimossi il **2026-08-12** e il loro testo vive in Markdown. I documenti
archiviati che li citano — [`src/design/delayed-actions-e-phase-windows.md`](src/design/delayed-actions-e-phase-windows.md),
[`src/handoff/roadmap-docs-test-e-showcase-v0.1.md`](src/handoff/roadmap-docs-test-e-showcase-v0.1.md),
[`gameplay/spec-terreni.md`](gameplay/spec-terreni.md) — **non sono stati riscritti**, per la regola qui sopra:
quei documenti dicono da quali file provenivano, e nel momento in cui sono stati scritti quei file c'erano.

> ⌫ **Le prime due etichette sono state rotte da una correzione, non lasciate indietro da uno spostamento.**
> Dicevano `src/design/…` e `src/handoff/…` — **giuste**, perché in questo file un path relativo parte da
> `docs/archive/` — finché il commit `8d5f2db7` (2026-08-19), il cui titolo è *«docs/src esce dagli elenchi
> che la nominavano»*, non vi ha applicato `src/ → research/`. Ripristinate il 2026-08-25
> ([#1232](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1232)), con l'`href` completato al
> **file** invece che alla cartella — cioè nella forma che il terzo elemento della stessa frase,
> `gameplay/spec-terreni.md`, aveva già.
>
> 🔑 **La causa è che esistono DUE `src/`, e solo una è stata svuotata.** `docs/src/` era la casella di posta
> dei sorgenti e [#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165) l'ha svuotata;
> `docs/archive/src/` è **questa**, i sorgenti già recepiti, ed è corrente — `CLAUDE.md` la cita come *«utile
> per la provenienza, mai per la regola»*. Le due condividono un segmento di path e **un'etichetta relativa
> non dichiara la propria base**: in quell'elenco `src/` non nominava `docs/src/`, e il rename l'ha
> attraversata comunque.
>
> ⚠️ **E non era l'unico segmento condiviso.** Misurate le sottocartelle di qui contro quelle di `docs/`,
> gli omonimi **oggi** sono `gameplay/` e `technical/`; `src/` lo era fino a #1165. Un `sed` su un path che
> non distingue la base colpisce anche quelli, e **nessuno dei due produce un link rotto** — solo
> un'etichetta che mente, che è la classe che [`../README.md`](../README.md) nomina e che da **D-182**
> nessun gate vede. Chi rifà una sostituzione qui la esegua **per riga intera**, non per segmento.

Dove sono finiti:

| Path citato negli storici | Oggi |
|---|---|
| `docs/archive/pdr-v0.1/RT_PDR_NN_*.pdf` | [`pdr-v0.1/RT_PDR_v0.1_consolidato.md`](pdr-v0.1/RT_PDR_v0.1_consolidato.md), sezione `PDR-NN` |
| `docs/src/prd/idee-base.pdf` · `prd-stampabile` · `prd-e-piano-di-sviluppo` · `prd-roadmap-e-percorso-didattico` | [`../research/prd/`](../research/prd/), quattro documenti tematici |
| `docs/src/prd/sequenza-risoluzione-turno.pdf` | [`gameplay/sequenza-turno-exploratory.md`](gameplay/sequenza-turno-exploratory.md) — c'era già |
