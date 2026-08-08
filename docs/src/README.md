# `docs/src/` — materiale sorgente

> **Non è fonte normativa.** `AGENTS.md`: «`docs/src/` contiene soprattutto input, audit e materiale di
> consolidamento/north-star: **non è fonte normativa per default**». In caso di conflitto prevalgono
> [`../product/piano-canonico-mvp.md`](../product/piano-canonico-mvp.md),
> [`../decisions/RT_PDR_00_Decision_Log.md`](../decisions/RT_PDR_00_Decision_Log.md) e gli ADR applicabili.

Qui vive il materiale **in ingresso**: i PDF originari da cui è nato il progetto, le specifiche di design
prodotte in sessione, gli audit della documentazione e gli handoff operativi. Un documento entra in questa
cartella così com'è; diventa vincolante solo quando un **owner documentale** lo recepisce (un brief in
`../gameplay/`, una spec in `../technical/`, un ADR in `../decisions/`).

## Struttura

```
docs/src/
├── prd/         11  PDF/docx originari: PRD, roadmap, idee di partenza
├── design/      14  specifiche di design per sistema o meccanica
├── audit/        2  fotografie dello stato di docs/ con piano di rientro
├── handoff/      7  prompt/task esecutivi rivolti alla repository
├── showcase/     5  scenario «Relay Basin» v0.1: spec, draft JSON, mappe
├── data/         1  dataset personaggi
├── media/       22  icone fazioni, icone e mockup HUD, infografiche
└── superclaude-cheatsheet.md
```

**Colonna «Recepito da»**: documenti fuori da `docs/src/` che linkano il file — misurato, non dichiarato.
Un trattino significa *nessun link in ingresso*: il materiale può essere ancora da consolidare **oppure**
essere stato assorbito senza lasciare tracciabilità. Vedi [Tracciabilità mancante](#tracciabilità-mancante).

## `prd/` — documenti originari

| File | Contenuto | Recepito da |
|---|---|---|
| [`catalogo-e-bilanciamento-v0.1.pdf`](prd/catalogo-e-bilanciamento-v0.1.pdf) | Catalogo azioni/eroi/equip/terreni + matrice di test | tutti i `../balance/`, ADR-0003, roadmap v0.1 |
| [`editor-griglia-esagonale-e-mappa.docx`](prd/editor-griglia-esagonale-e-mappa.docx) | Griglia esagonale ed editor mappa | ADR-0002, `../roadmap/hex-map-roadmap.md` |
| [`sequenza-risoluzione-turno.pdf`](prd/sequenza-risoluzione-turno.pdf) | Sequenza di risoluzione del turno | `../gameplay/spec-anima-risoluzione.md` |
| [`idee-base.pdf`](prd/idee-base.pdf) | Idee fondative del progetto | `../archive/gameplay/spec-terreni.md` |
| [`prd-intenti-condivisi.pdf`](prd/prd-intenti-condivisi.pdf) | PRD — intenti condivisi | — |
| [`prd-personaggi-e-combattimento-reattivo.pdf`](prd/prd-personaggi-e-combattimento-reattivo.pdf) | PRD — personaggi e combattimento reattivo | — |
| [`prd-roadmap-e-percorso-didattico.pdf`](prd/prd-roadmap-e-percorso-didattico.pdf) | PRD + roadmap + percorso didattico UE | — |
| [`prd-e-piano-di-sviluppo.pdf`](prd/prd-e-piano-di-sviluppo.pdf) | PRD e piano completo di sviluppo | — |
| [`prd-stampabile.pdf`](prd/prd-stampabile.pdf) | PRD, versione stampabile | — |
| [`idee-ruoli-characters.pdf`](prd/idee-ruoli-characters.pdf) | Idee sui ruoli dei personaggi | — |
| [`guida-trovare-asset-free.pdf`](prd/guida-trovare-asset-free.pdf) | Guida operativa: reperire asset gratuiti | — |

> I PDF descrivono un prodotto **più ambizioso dello scope corrente** (vedi `README.md` di progetto): vanno
> letti come north-star, non come backlog.

## `design/` — specifiche per sistema

| File | Data | Sistema | Recepito da |
|---|---|---|---|
| [`overwatch-e-fast-reaction.md`](design/overwatch-e-fast-reaction.md) | — | Overwatch, Fast Action, Fast Reaction | ADR-0004, `../gameplay/brief-overwatch-reazioni.md`, piano canonico |
| [`action-ghosts-fasi-fast-reactions.md`](design/action-ghosts-fasi-fast-reactions.md) | — | Ghost di azione, ordine fasi, `Facing` | ADR-0005, `../technical/brief-planning-visuale.md` |
| [`rumore-e-percezione-acustica.md`](design/rumore-e-percezione-acustica.md) | — | Rumore, percezione acustica, fog of war | `../gameplay/brief-conoscenza-parziale.md` + roadmap v0.1 |
| [`delayed-actions-e-phase-windows.md`](design/delayed-actions-e-phase-windows.md) | 2026-08-07 | Delayed actions, phase boundaries | `../gameplay/brief-delayed-actions.md` |
| [`terreno-ghiaccio-v0.1.md`](design/terreno-ghiaccio-v0.1.md) | — | Terreno ghiaccio in UE5 | `../gameplay/brief-ghiaccio.md` |
| [`auxiliary-units.md`](design/auxiliary-units.md) | — | Pet, evocazioni, droni, torrette | `../gameplay/brief-unita-ausiliarie.md` |
| [`azioni-generiche-overwatch-universale-v0.1.md`](design/azioni-generiche-overwatch-universale-v0.1.md) | 2026-08-07 | Azioni generiche, Overwatch universale | `../gameplay/brief-azioni-generiche-overwatch.md` |
| [`predictive-actions-e-trappole.md`](design/predictive-actions-e-trappole.md) | — | Azioni predittive, trappole, gambit | `../gameplay/brief-delayed-actions.md` |
| [`fazioni-v0.2-identita-visiva-e-roster.md`](design/fazioni-v0.2-identita-visiva-e-roster.md) | — | Fazioni, identità visiva, cooperazione | D-029 / ADR-0006 (banner in testa al file) |
| [`match-timing-e-scala-mappe.md`](design/match-timing-e-scala-mappe.md) | — | Durata partita, round budget, scala mappe | — ⚠️ vedi nota |
| [`2026-08-08-cover-window-open-fire-seal.md`](design/2026-08-08-cover-window-open-fire-seal.md) | 2026-08-08 | Cover Window, Open → Fire → Seal | — |
| [`2026-08-08-muri-porte-e-interazioni.md`](design/2026-08-08-muri-porte-e-interazioni.md) | 2026-08-08 | Muri, porte, interazioni, validazione | — |
| [`2026-08-08-roster-8-conflux-constrine.md`](design/2026-08-08-roster-8-conflux-constrine.md) | 2026-08-08 | Roster 8 personaggi, Conflux e Constrine | — ⚠️ vedi nota |
| [`2026-08-08-hud-faction-icons.md`](design/2026-08-08-hud-faction-icons.md) | 2026-08-08 | Icone fazioni, HUD icon language | — (immagini in [`media/hud/`](media/hud)) |

## `audit/` — stato della documentazione

| File | Oggetto | Recepito da |
|---|---|---|
| [`2026-08-08-docs-gameplay.md`](audit/2026-08-08-docs-gameplay.md) | Audit di `../gameplay/` + piano di consolidamento | `../CHANGELOG_DOCUMENTATION.md`, Decision Log |
| [`2026-08-08-docs-non-gameplay-v2.md`](audit/2026-08-08-docs-non-gameplay-v2.md) | Audit del resto di `docs/` — decisioni chiuse | `../CHANGELOG_DOCUMENTATION.md`, Decision Log |

## `handoff/` — task esecutivi

| File | Oggetto | Recepito da |
|---|---|---|
| [`consolidamento-prd-source-of-truth.md`](handoff/consolidamento-prd-source-of-truth.md) | Consolidare PRD e source of truth | `../roadmap/plans/brief-consolidamento-documentale.md` |
| [`scenario-browser-bp-gamemode.md`](handoff/scenario-browser-bp-gamemode.md) | Selettore scenari per categoria in `BP_GameMode` | `../technical/scenario-index-e-tag.md` |
| [`scenario-harness-task-originale.md`](handoff/scenario-harness-task-originale.md) | Task originale dello Scenario Test Harness | `../technical/test-automatico-unreal.md` |
| [`roadmap-v0.1-prompt-originale.md`](handoff/roadmap-v0.1-prompt-originale.md) | Prompt da cui è nata la roadmap v0.1 | ADR-0003 |
| [`roadmap-docs-test-e-showcase-v0.1.md`](handoff/roadmap-docs-test-e-showcase-v0.1.md) | Consolidamento roadmap/test/showcase v0.1 | `../roadmap/plans/showcase-v01-audit.md` |
| [`2026-08-07-nuove-decisioni-e-scenario-4v4.md`](handoff/2026-08-07-nuove-decisioni-e-scenario-4v4.md) | Nuove decisioni, scenario 4v4, roadmap | — |
| [`2026-08-08-bot-ai-roadmap-e-test-pie.md`](handoff/2026-08-08-bot-ai-roadmap-e-test-pie.md) | Bot AI tattica, test PIE, scenari | — |

## `showcase/` — «Relay Basin» v0.1

| File | Contenuto |
|---|---|
| [`relay-v0.1-scenario-spec.md`](showcase/relay-v0.1-scenario-spec.md) | Scenario 2v2 completo: coordinate assiali, turni, risultati attesi |
| [`relay-v0.1-scenario-draft.json`](showcase/relay-v0.1-scenario-draft.json) | Draft dello scenario in JSON (**non** caricato dai test: l'ID `RT_Showcase_Relay_v01` è risolto da `URTScenarioIndex`) |
| [`showcase-v0.1-integrazione-nel-codice.md`](showcase/showcase-v0.1-integrazione-nel-codice.md) | Handoff di integrazione nel codice attuale |
| [`mappa-tattica-bacino-relay.png`](showcase/mappa-tattica-bacino-relay.png) · [`dynamic-map.png`](showcase/dynamic-map.png) | Mappe di riferimento |

Owner: [`../product/showcase-v0.1.md`](../product/showcase-v0.1.md) · audit: `../roadmap/plans/showcase-v01-audit.md`.

## `data/` e `media/`

- [`data/characters-wiki-data-v0.4.xlsx`](data/characters-wiki-data-v0.4.xlsx) — dataset dei personaggi.
  Citato da **44 schede** in `../characters/`: è il file di `docs/src/` con più consumatori.
- `media/fazioni/` — `faction-01..04.png`, icone delle fazioni.
- `media/hud/` — `icon-01..04.png` (icone HUD), `hud-example.png` e `hud-style.png` (mockup e linguaggio
  visivo); sorgente di design in `design/2026-08-08-hud-faction-icons.md`.
- `media/infografica/` — 12 infografiche su azioni, turno, feature e interazione con l'ambiente.

## Convenzioni

1. **Nomi**: kebab-case ASCII. Niente spazi, em-dash, parentesi o maiuscole nei nomi file — rendono fragili
   i link markdown e i comandi da shell.
2. **Data in testa** (`2026-08-08-oggetto.md`) quando il documento fotografa un momento: audit, handoff,
   decisioni di sessione. Le specifiche per sistema si nominano invece per **sistema**, non per data.
3. **Un documento, una cartella**: se un file è insieme design e handoff, decide il criterio dominante —
   *definisce un sistema* → `design/`; *dice cosa fare nella repo* → `handoff/`.
4. **I sorgenti non si riscrivono.** Quando un documento viene recepito, si aggiunge un banner in testa che
   punta all'owner (vedi `design/fazioni-v0.2-identita-visiva-e-roster.md`) e si aggiorna questo indice.
   Il testo originale resta intatto: serve a ricostruire da dove è nata una decisione.
5. **Niente archivi generati**: gli export della wiki (`*.zip`) non stanno nel repository, sono rigenerabili.
6. **Immagini**: `media/`, tranne quelle che appartengono a un bundle auto-contenuto come `showcase/`.

## Tracciabilità mancante

Sette documenti non sono linkati da nessun owner. Per due esiste già un documento sullo stesso tema che però
**non li cita** — vale la pena aggiungere il link o dichiararli superati:

| Sorgente | Owner probabile | Azione |
|---|---|---|
| `design/match-timing-e-scala-mappe.md` | [`../gameplay/spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md) | verificare e linkare |
| `design/2026-08-08-roster-8-conflux-constrine.md` | [`../wiki/fazioni/constrine.md`](../wiki/fazioni/constrine.md) | verificare e linkare |
| `design/2026-08-08-cover-window-open-fire-seal.md` | nessuno | da consolidare |
| `design/2026-08-08-muri-porte-e-interazioni.md` | nessuno | da consolidare |
| `design/2026-08-08-hud-faction-icons.md` | nessuno | da consolidare (immagini già in `media/hud/`) |
| `handoff/2026-08-07-nuove-decisioni-e-scenario-4v4.md` | nessuno | da consolidare |
| `handoff/2026-08-08-bot-ai-roadmap-e-test-pie.md` | nessuno | da consolidare |

I sette PDF di `prd/` senza citazioni sono materiale fondativo — più la guida agli asset, che è di supporto —
già superato dal piano canonico: non richiedono azione.
