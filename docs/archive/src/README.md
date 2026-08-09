# Archivio dei sorgenti recepiti

> `HISTORICAL` · **Materiale NON autorevole** · **Archiviato il 2026-08-08**
>
> I 25 documenti in questa cartella sono i **sorgenti** da cui è nata parte della documentazione normativa.
> Erano in [`../../src/`](../../src/); sono stati spostati qui quando un owner documentale li ha recepiti.
> Restano per **provenienza**: servono a ricostruire *da dove* è nata una decisione, non a deciderla.
>
> **Il testo originale non è stato riscritto.** Dove un sorgente conteneva un errore di fatto, la correzione è
> una nota `⚠️` accanto all'affermazione, non una modifica del paragrafo.

**Se cerchi la regola, non sei nel posto giusto**: la colonna «Recepito da» dice chi la possiede oggi.
In caso di conflitto prevalgono [`../../product/piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md),
[`../../decisions/RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md) e gli ADR.

## `design/` — specifiche per sistema

| File | Sistema | Recepito da |
|---|---|---|
| [`overwatch-e-fast-reaction.md`](design/overwatch-e-fast-reaction.md) | Overwatch, Fast Action, Fast Reaction | ADR-0004, [`brief-overwatch-reazioni.md`](../../gameplay/brief-overwatch-reazioni.md), piano canonico |
| [`action-ghosts-fasi-fast-reactions.md`](design/action-ghosts-fasi-fast-reactions.md) | Ghost di azione, ordine fasi, `Facing` | ADR-0005, [`brief-planning-visuale.md`](../../technical/brief-planning-visuale.md) |
| [`rumore-e-percezione-acustica.md`](design/rumore-e-percezione-acustica.md) | Rumore, percezione acustica, fog of war | [`brief-conoscenza-parziale.md`](../../gameplay/brief-conoscenza-parziale.md) + roadmap v0.1 |
| [`delayed-actions-e-phase-windows.md`](design/delayed-actions-e-phase-windows.md) | Delayed actions, phase boundaries | [`brief-delayed-actions.md`](../../gameplay/brief-delayed-actions.md) |
| [`terreno-ghiaccio-v0.1.md`](design/terreno-ghiaccio-v0.1.md) | Terreno ghiaccio in UE5 | [`brief-ghiaccio.md`](../../gameplay/brief-ghiaccio.md) |
| [`auxiliary-units.md`](design/auxiliary-units.md) | Pet, evocazioni, droni, torrette | [`brief-unita-ausiliarie.md`](../../gameplay/brief-unita-ausiliarie.md) |
| [`azioni-generiche-overwatch-universale-v0.1.md`](design/azioni-generiche-overwatch-universale-v0.1.md) | Azioni generiche, Overwatch universale | [`brief-azioni-generiche-overwatch.md`](../../gameplay/brief-azioni-generiche-overwatch.md) |
| [`predictive-actions-e-trappole.md`](design/predictive-actions-e-trappole.md) | Azioni predittive, trappole, gambit | [`brief-delayed-actions.md`](../../gameplay/brief-delayed-actions.md) |
| [`fazioni-v0.2-identita-visiva-e-roster.md`](design/fazioni-v0.2-identita-visiva-e-roster.md) | Fazioni, identità visiva, cooperazione | D-029 / ADR-0006 |
| [`match-timing-e-scala-mappe.md`](design/match-timing-e-scala-mappe.md) | Durata partita, round budget, scala mappe | [`spec-durata-partita-e-scala-mappe.md`](../../gameplay/spec-durata-partita-e-scala-mappe.md) · D-030 · **E19** |
| [`2026-08-08-hud-faction-icons.md`](design/2026-08-08-hud-faction-icons.md) | Icone fazioni, HUD icon language | D-031 · **E20** · immagini in [`../../src/media/hud/`](../../src/media/hud/) |
| [`2026-08-08-roster-8-conflux-constrine.md`](design/2026-08-08-roster-8-conflux-constrine.md) | Roster 8, Conflux e Constrine | [`../../wiki/fazioni/`](../../wiki/fazioni/) + [`../../characters/v0.2/`](../../characters/v0.2/) · runtime in **E35** *(era `E21`, rinumerata il 2026-08-09: [D-039](../../decisions/RT_PDR_00_Decision_Log.md))* |
| [`2026-08-08-cover-window-open-fire-seal.md`](design/2026-08-08-cover-window-open-fire-seal.md) | Cover Window, Open → Fire → Seal | 📅 **E22** (v0.2), con i 12 scenari di test |
| [`2026-08-08-muri-porte-e-interazioni.md`](design/2026-08-08-muri-porte-e-interazioni.md) | Muri, porte, interazioni, validazione | 📅 **E23** (v0.2) |
| [`trasformazioni-e-stati-personaggio.md`](design/trasformazioni-e-stati-personaggio.md) | Trasformazioni, stance, stati del personaggio | [`brief-stati-personaggio-e-trasformazioni.md`](../../gameplay/brief-stati-personaggio-e-trasformazioni.md) · D-035 · 📅 **E34** |

## `handoff/` — task esecutivi

| File | Oggetto | Recepito da |
|---|---|---|
| [`consolidamento-prd-source-of-truth.md`](handoff/consolidamento-prd-source-of-truth.md) | Consolidare PRD e source of truth | [`brief-consolidamento-documentale.md`](../../roadmap/plans/brief-consolidamento-documentale.md) |
| [`scenario-browser-bp-gamemode.md`](handoff/scenario-browser-bp-gamemode.md) | Selettore scenari in `BP_GameMode` | [`scenario-index-e-tag.md`](../../technical/scenario-index-e-tag.md) |
| [`scenario-harness-task-originale.md`](handoff/scenario-harness-task-originale.md) | Task originale dello Scenario Test Harness | [`test-automatico-unreal.md`](../../technical/test-automatico-unreal.md) |
| [`roadmap-v0.1-prompt-originale.md`](handoff/roadmap-v0.1-prompt-originale.md) | Prompt da cui è nata la roadmap v0.1 | ADR-0003 |
| [`roadmap-docs-test-e-showcase-v0.1.md`](handoff/roadmap-docs-test-e-showcase-v0.1.md) | Consolidamento roadmap/test/showcase v0.1 | [`showcase-v01-audit.md`](../../roadmap/plans/showcase-v01-audit.md) |
| [`2026-08-07-nuove-decisioni-e-scenario-4v4.md`](handoff/2026-08-07-nuove-decisioni-e-scenario-4v4.md) | Nuove decisioni, scenario 4v4, roadmap | decisioni §3 già canone · scenario → **E17** / **E32** |
| [`2026-08-08-bot-ai-roadmap-e-test-pie.md`](handoff/2026-08-08-bot-ai-roadmap-e-test-pie.md) | Bot AI tattica, test PIE, scenari | `PIE-AI-01…05` · [`avversario-bot.md`](../../wiki/game/avversario-bot.md) · **E26**/**E28** |
| [`2026-08-08-tre-aggiunte-signature-mechanics.md`](handoff/2026-08-08-tre-aggiunte-signature-mechanics.md) | ConditionalIntent, GenericActionModifier, Misplay | D-032 · D-033 · D-034 — vedi il banner in testa al file: **una sola** delle tre era davvero assente |
| [`2026-08-08-azioni-base-e-facing.md`](handoff/2026-08-08-azioni-base-e-facing.md) | Azioni base e facing: consolidamento | [ADR-0005](../../decisions/adr-0005-orientamento.md) copriva già il canone. Restano tre **proposte di modifica** (righe 50–52 della conflict matrix) e `FAC-4…FAC-10` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) |

## `audit/` — stato della documentazione

| File | Oggetto | Recepito da |
|---|---|---|
| [`2026-08-08-docs-gameplay.md`](audit/2026-08-08-docs-gameplay.md) | Audit di `gameplay/` + piano di consolidamento | [`CHANGELOG_DOCUMENTATION.md`](../../CHANGELOG_DOCUMENTATION.md), Decision Log |
| [`2026-08-08-docs-non-gameplay-v2.md`](audit/2026-08-08-docs-non-gameplay-v2.md) | Audit del resto di `docs/` | [`CHANGELOG_DOCUMENTATION.md`](../../CHANGELOG_DOCUMENTATION.md), Decision Log |

## Nota sui path interni

I documenti sono scesi di un livello (`docs/src/X/` → `docs/archive/src/X/`) e i loro link relativi sono stati
riscritti di conseguenza. Restano **volutamente non corretti** i riferimenti *in prosa* a percorsi che non
esistono più — per esempio l'audit del 2026-08-08 che cita `docs/src/` o un nome file poi rinominato: quella
è la fotografia di com'era il repository quel giorno, e riscriverla falsificherebbe l'audit.
