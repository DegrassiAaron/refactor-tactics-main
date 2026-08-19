# Piani e referti storici — materiale NON autorevole

> `HISTORICAL` · **Archiviato il 2026-08-14** · I file qui sono **istantanee**: dicono cosa si sapeva il
> giorno in cui sono stati scritti, e non si aggiornano. Si leggono per la **provenienza** — perché una
> decisione è stata presa così — mai per la regola.
>
> La regola vive negli owner: [`../../decisions/`](../../decisions/RT_PDR_00_Decision_Log.md),
> [`../../product/piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md),
> [`../../roadmap/roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md),
> [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).

## Perché questa cartella esiste

`docs/roadmap/plans/` conteneva **69** documenti, e nessun lettore aveva un modo di distinguere un piano
ancora vivo da un referto superato: il criterio disponibile era la **data nel nome**, e la data nel nome è un
criterio sbagliato in questo repository — coglieva **35** file su **68** che una data ce l'hanno davvero, e
dei 35 ne prendeva **22 dichiarati `CURRENT`**.

Il criterio giusto era già scritto dentro i documenti: il **banner di stato**. Questa cartella raccoglie i
soli piani che si dichiaravano `HISTORICAL` o `SNAPSHOT` — cioè quelli che avevano già smesso di pretendere
di essere veri.

> ⚠️ **Nessuno stato è stato riscritto per farli entrare qui.** È la differenza fra archiviare e dichiarare
> superato: la prima è una riorganizzazione, la seconda è un'affermazione su un documento. Un archivio che
> obbliga a falsificare un banner non sta ordinando, sta decidendo — e le decisioni hanno un altro posto.

## Cosa c'è

| Documento | Cos'era | Perché è qui |
|---|---|---|
| [`action-economy-consolidamento-2026-08-12.md`](action-economy-consolidamento-2026-08-12.md) | Consolidamento su economia del turno, movimento e facing | `HISTORICAL` — recepito in `D-114` e nelle spec owner |
| [`feature-registry-audit-2026-08-08.md`](feature-registry-audit-2026-08-08.md) | Il primo audit completo del Feature Registry | `HISTORICAL` — il registry è owner di sé stesso dal giorno dopo |
| [`replay-archive-issue-specs-2026-08-10.md`](replay-archive-issue-specs-2026-08-10.md) | Specifiche delle issue dell'archivio replay | `HISTORICAL` — le issue esistono, `ADR-0009` e `D-077` decidono |
| [`spatial-transfer-epic-2026-08-12.md`](spatial-transfer-epic-2026-08-12.md) | Istruttoria dell'epic **E39** | `HISTORICAL` — `D-118`/`D-119`, epic [`#704`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/704) |
| [`teleport-instant-movement-2026-08-12.md`](teleport-instant-movement-2026-08-12.md) | Il kit teleport da cui è nata la tassonomia | `HISTORICAL` — `spec-tassonomia-movimento.md` è l'owner |
| [`wiki-consolidamento-2026-08-10.md`](wiki-consolidamento-2026-08-10.md) | Consolidamento Wiki prima di `D-076` | `HISTORICAL` — dal 2026-08-10 il clone **è** la fonte |
| [`roadmap-lane-index.md`](roadmap-lane-index.md) + [`roadmap_lane_1.md`](roadmap_lane_1.md)…[`roadmap_lane_7.md`](roadmap_lane_7.md) | Le sette lane di esecuzione, fotografate il 2026-08-12 | `SNAPSHOT` — la topologia viva è [`../../roadmap/execution-graph.yaml`](../../roadmap/execution-graph.yaml), lo stato il Feature Registry |
| [`bot-ai-consolidamento-2026-08-11.md`](bot-ai-consolidamento-2026-08-11.md) | Referto su Bot/AI, Team Planner, Belief e tracking | `HISTORICAL` — sorgenti in `../src/handoff/`; le §21–§27 (sette release bot) **non** applicate |
| [`e9-5-coperture-temporanee-plan.md`](e9-5-coperture-temporanee-plan.md) | Piano di CP 9.5, pannello cinetico e coperture temporanee | `HISTORICAL` — epic **E9** e [`#73`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/73) chiuse |
| [`editormap-spec.md`](editormap-spec.md) | Spec della vista operativa in editor | `HISTORICAL` — la vista esiste: [`../../roadmap/editormap.shortlist.md`](../../roadmap/editormap.shortlist.md), generata |
| [`plan-turnlog.md`](plan-turnlog.md) | Piano di TurnLog e reason code | `HISTORICAL` — si dichiarava già consegnato in prosa; owner [`../../technical/architecture/spec-turnlog.md`](../../technical/architecture/spec-turnlog.md) |
| [`roadmap-reconciliation-2026-08-12.md`](roadmap-reconciliation-2026-08-12.md) · [`roadmap-reconciliation-2026-08-13.md`](roadmap-reconciliation-2026-08-13.md) | I due referti di riconciliazione **parziale** | `SNAPSHOT` — il secondo afferma che `FMT-1` è da decidere, e [`D-137`](../../decisions/RT_PDR_00_Decision_Log.md) l'ha chiusa la sera stessa |

**20 documenti** in tutto: **10** `HISTORICAL` e **10** `SNAPSHOT`. Si rimisura con
`ls docs/archive/roadmap-plans/*.md | grep -v README | wc -l`.

> ⚠️ **Sei di questi hanno preso il banner nello stesso commit che li ha spostati** — non ne avevano
> nessuno, e il loro stato è stato **derivato da un fatto verificabile** citato nel banner stesso: un'epic
> chiusa, una vista generata che esiste, lo status di una feature nel registry. È l'unico modo onesto di
> etichettare un documento muto: se lo stato non si deriva, non si scrive.

## Cosa NON è qui, e perché

**I 23 piani `CURRENT` restano in [`../../roadmap/plans/`](../../roadmap/plans/)**, anche quelli con una data
nel nome. Diversi sono citati come **istruttoria** da `OPEN_DECISIONS.md` e dal Decision Log per decisioni
ancora aperte: spostarli non li renderebbe più vecchi, renderebbe solo più lungo il percorso per leggerli.

Un piano si archivia quando **quello che diceva è atterrato altrove** o è stato superato — non quando compie
una settimana.
