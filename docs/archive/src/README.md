# Archivio dei sorgenti recepiti

> `HISTORICAL` · **Materiale NON autorevole** · **Primo lotto archiviato il 2026-08-08**
>
> I **40** documenti in questa cartella sono i **sorgenti** da cui è nata parte della documentazione normativa.
> Il primo lotto era in [`../../src/`](../../src/); ogni sorgente si sposta qui quando un owner documentale lo
> ha recepito. Restano per **provenienza**: servono a ricostruire *da dove* è nata una decisione, non a deciderla.
>
> **Il testo originale non è stato riscritto.** Dove un sorgente conteneva un errore di fatto, la correzione è
> una nota `⚠️` accanto all'affermazione, non una modifica del paragrafo.
>
> ➕ **Dal 2026-08-09 la cartella accoglie anche i sorgenti *revisionati e non applicati***: un brief che il
> canone contraddice si archivia con l'esito della revisione in testa, non si scarta. La colonna «Recepito da»
> in quel caso punta al referto, non a un owner — perché non c'è nulla da possedere.
>
> ➕ **Dal 2026-08-10 ci sono anche i dodici sorgenti del pacchetto** `todo/consolidazione-chat-openai/` —
> **sei** master, tre kit e tre documenti *meta* che riguardavano il progetto ChatGPT e non il repository.
> Il pacchetto ne conteneva quattordici: il settimo master,
> `RT_Common_Actions_Master_Consolidation_v0.1.md`, **non è qui** perché non è ancora recepito, e il
> quattordicesimo era il duplicato di un kit già archiviato.
>
> ⚠️ **Il conteggio era già sbagliato prima del 2026-08-09**: l'intestazione diceva «25» mentre le sue stesse
> tabelle elencavano **26** righe. Il numero qui sopra è **misurato**
> (`ls docs/archive/src/{design,handoff,audit}/*.md | wc -l` → 15 + 23 + 2), non incrementato a mano — che è
> il modo in cui era andato fuori sincrono.

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
| [`2026-08-09-attacco-base-per-eroe.md`](handoff/2026-08-09-attacco-base-per-eroe.md) | Profili di attacco base per eroe | [ADR-0007](../../decisions/adr-0007-attacco-base-per-eroe.md) · il documento porta **inline** le sezioni corrette (§9, §10–13, §15-bis, §24, §27, §28): tre valori su quattro della matrice originale contraddicevano il catalogo, e i nomi candidati collidevano con azioni gia' spedite |
| [`2026-08-09-map-editor-roadmap.md`](handoff/2026-08-09-map-editor-roadmap.md) | Roadmap e consolidamento del Map Editor (v0.1–v0.5) | ⛔ **Revisionato e non applicato** — [`map-editor-brief-spec-panel-2026-08-09.md`](../../roadmap/plans/map-editor-brief-spec-panel-2026-08-09.md). 9 duplicati, 5 conflitti (muri vs **E23.1**, porte, terreni, profili di movimento). Sopravvive **una** proposta: la sonda di movimento nell'editor |

### Il pacchetto `consolidazione-chat-openai` — dodici sorgenti, un solo triage

Sei master, tre kit di dettaglio e tre *meta* — dodici righe, contate sulla tabella. Il referto comune è
[`consolidamento-chat-openai-triage-2026-08-09.md`](../../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md):
classifica ogni affermazione contro il canone **prima** che qualcuno la applichi, e la colonna qui sotto dice
dove è finita quella parte che è sopravvissuta al filtro.

| File | Oggetto | Recepito da |
|---|---|---|
| [`2026-08-08-master-mappa-e-ambiente.md`](handoff/2026-08-08-master-mappa-e-ambiente.md) | Mappa, ambiente, propagazione, interazioni | [`spec-interazioni-mappa-cp101.md`](../../gameplay/spec-interazioni-mappa-cp101.md) — E10 · CP 10.1 |
| [`elementi-interattivi-della-mappa.md`](handoff/elementi-interattivi-della-mappa.md) | Catalogo degli elementi interattivi (kit del precedente) | idem, §2–§13. Aperte `INT-1`, `INT-2`, `INT-4` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) |
| [`2026-08-08-master-ui-ux.md`](handoff/2026-08-08-master-ui-ux.md) | UI / UX, pannelli, leggibilità | [`progettazione-hud.md`](../../technical/progettazione-hud.md) · E11 / E20 — su più punti il repository era **più avanti della fonte** |
| [`hud-consolidation-kit.md`](handoff/hud-consolidation-kit.md) | HUD (kit del precedente, e più vecchio) | ⛔ tre dei sette conflitti del pacchetto nascono qui: `TEAM READY`, «Fog of War», eleggibilità per nome d'eroe |
| [`2026-08-08-master-governance.md`](handoff/2026-08-08-master-governance.md) | Governance, Feature Registry, roadmap | I **nove gate coincidono alla lettera** con [`feature-registry.yaml`](../../roadmap/feature-registry.yaml). Il vocabolario di status no: 13 contro 10 derivati |
| [`2026-08-08-master-scenari-qa-e-bot.md`](handoff/2026-08-08-master-scenari-qa-e-bot.md) | Scenari, QA, bot | Il più assorbito: [`test-automatico-unreal.md`](../../technical/test-automatico-unreal.md) · [`scenario-index-e-tag.md`](../../technical/scenario-index-e-tag.md) · il bot era già recepito il 2026-08-08 |
| [`2026-08-08-master-characters-e-roster.md`](handoff/2026-08-08-master-characters-e-roster.md) | Personaggi, roster, fazioni, Super | [ADR-0007](../../decisions/adr-0007-attacco-base-per-eroe.md) per l'attacco base; il residuo in [`brief-super-e-cooldown.md`](../../gameplay/brief-super-e-cooldown.md) — issue `#336`, PR `#349` |
| [`2026-08-08-master-reaction-system.md`](handoff/2026-08-08-master-reaction-system.md) | Cluster Reaction | PR `#305` — `D-047`, `D-048`, `D-049`, CP 14.7. Era già assorbito quando il triage è iniziato |
| [`2026-08-09-decision-time-bank.md`](handoff/2026-08-09-decision-time-bank.md) | Decision / Reaction Time Bank | [`spec-decision-time-bank.md`](../../gameplay/spec-decision-time-bank.md) (CP 14.8) · `D-050`…`D-057` · [conflict report](../../roadmap/plans/decision-time-bank-conflict-report-2026-08-09.md) |
| [`2026-08-08-chat-cleanup-tracker.md`](handoff/2026-08-08-chat-cleanup-tracker.md) | *Meta* — tracker del progetto ChatGPT | ⛔ nessun owner: **sette dei nove «conflitti aperti» che elenca erano già chiusi**. Citato dal triage §4 |
| [`2026-08-08-chat-cleanup-tracker-prima-versione.md`](handoff/2026-08-08-chat-cleanup-tracker-prima-versione.md) | *Meta* — la stessa cosa, una versione prima | ⛔ da ignorare: la seconda copia è più recente. Le due erano indistinguibili per nome |
| [`2026-08-08-final-chat-cleanup-plan.md`](handoff/2026-08-08-final-chat-cleanup-plan.md) | *Meta* — piano di chiusura delle conversazioni | ⛔ riguarda il progetto ChatGPT, non il repository |

> ⬜ **Manca il tredicesimo.** `RT_Common_Actions_Master_Consolidation_v0.1.md` è ancora in
> `docs/archive/consolidazione-chat-openai/`, **untracked e non recepito**: la decisione che lo bloccava è
> stata presa il 2026-08-09 — la migrazione degli Stable ID resta *dichiarata*, i documenti si allineano — ma
> nessun owner ha ancora consumato il master. Finché è così non appartiene a questa cartella, che è
> l'archivio dei sorgenti **recepiti**.

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
