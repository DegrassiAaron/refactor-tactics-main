# `docs/src/` — materiale sorgente

> **Non è fonte normativa.** `AGENTS.md`: «`docs/src/` contiene soprattutto input, audit e materiale di
> consolidamento/north-star: **non è fonte normativa per default**». In caso di conflitto prevalgono
> [`../product/piano-canonico-mvp.md`](../product/piano-canonico-mvp.md),
> [`../decisions/RT_PDR_00_Decision_Log.md`](../decisions/RT_PDR_00_Decision_Log.md) e gli ADR applicabili.

Qui vive il materiale **in ingresso** che non è ancora stato consumato: i PDF originari da cui è nato il
progetto, i dataset ancora in uso e le immagini a cui la documentazione punta. Un documento entra così com'è;
diventa vincolante solo quando un **owner documentale** lo recepisce (un brief in `../gameplay/`, una spec in
`../technical/`, un ADR in `../decisions/`).

## ⚠️ Cambio di regola — 2026-08-08

**Un sorgente recepito non resta più qui: si sposta in [`../archive/src/`](../archive/src/README.md).**

Fino al 2026-08-08 la convenzione era l'opposta — il sorgente restava in `docs/src/` con un banner in testa che
puntava all'owner. Aveva un difetto misurabile: `docs/src/` cresceva mescolando *materiale da lavorare* e
*materiale già lavorato*, e la sola cosa che li distingueva era una colonna di questo indice. Chi apriva la
cartella non poteva sapere cosa avesse ancora un compito.

Ora la distinzione è la **posizione**:

```text
docs/src/          da consumare, o ancora in uso
docs/archive/src/  consumato: resta per provenienza
```

I **25 documenti** già recepiti — `design/`, `handoff/`, `audit/` — sono stati spostati in blocco. L'indice con
la colonna «Recepito da» si è spostato con loro: [`../archive/src/README.md`](../archive/src/README.md).

Resta valido il resto della convenzione, che è la parte importante: **i sorgenti non si riscrivono.** Una
correzione è una nota `⚠️` accanto all'affermazione sbagliata, mai una modifica del paragrafo.

## Struttura

```
docs/src/
├── prd/      11  PDF/docx originari: PRD, roadmap, idee di partenza
├── showcase/  5  scenario «Relay Basin» v0.1: spec, draft JSON, mappe
├── data/      3  dataset personaggi + materiale di integrazione wiki
├── media/    28  icone fazioni, icone e mockup HUD, infografiche
└── superclaude-cheatsheet.md
```

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

> I PDF **non si archiviano**: descrivono un prodotto più ambizioso dello scope corrente e sono il livello 8
> della gerarchia delle fonti — *visione north-star*, consultata di continuo. Vanno letti come direzione, non
> come backlog.

## `showcase/` — «Relay Basin» v0.1

| File | Contenuto |
|---|---|
| [`relay-v0.1-scenario-spec.md`](showcase/relay-v0.1-scenario-spec.md) | Scenario 2v2 completo: coordinate assiali, turni, risultati attesi |
| [`relay-v0.1-scenario-draft.json`](showcase/relay-v0.1-scenario-draft.json) | Draft dello scenario in JSON (**non** caricato dai test: l'ID `RT_Showcase_Relay_v01` è risolto da `URTScenarioIndex`) |
| [`showcase-v0.1-integrazione-nel-codice.md`](showcase/showcase-v0.1-integrazione-nel-codice.md) | Handoff di integrazione nel codice attuale |
| [`mappa-tattica-bacino-relay.png`](showcase/mappa-tattica-bacino-relay.png) · [`dynamic-map.png`](showcase/dynamic-map.png) | Mappe di riferimento |

Owner: [`../product/showcase-v0.1.md`](../product/showcase-v0.1.md) · audit:
[`../roadmap/plans/showcase-v01-audit.md`](../roadmap/plans/showcase-v01-audit.md).

> **Non archiviato**, benché l'handoff sia già stato recepito: **E15 è ancora aperta**, e il bundle si consulta
> mentre la si costruisce. Si sposterà quando la showcase sarà consegnata.

## `data/` e `media/`

- [`data/characters-wiki-data-v0.4.xlsx`](data/characters-wiki-data-v0.4.xlsx) — dataset dei personaggi,
  citato da **44 schede** in `../characters/`. È il **dataset corrente**, non un sorgente consumato: resta qui.
- `data/wiki/` — materiale di integrazione della wiki.
  ⚠️ [`CLAUDE_INTEGRATION_PROMPT.md`](data/wiki/CLAUDE_INTEGRATION_PROMPT.md) punta a **10 immagini che non
  sono mai state committate** (`data/images/…`). Difetto preesistente, arrivato con il materiale: il documento
  è inutilizzabile così com'è finché le immagini non arrivano o i riferimenti non vengono corretti.
- `media/fazioni/` — `faction-01..04.png`, icone delle fazioni.
- `media/hud/` — `icon-01..04.png`, `hud-example.png` e `hud-style.png`; sorgente di design in
  [`../archive/src/design/2026-08-08-hud-faction-icons.md`](../archive/src/design/2026-08-08-hud-faction-icons.md).
- `media/infografica/` — 18 infografiche su azioni, turno, feature e interazione con l'ambiente.

## Convenzioni

1. **Nomi**: kebab-case ASCII. Niente spazi, em-dash, parentesi o maiuscole nei nomi file — rendono fragili
   i link markdown e i comandi da shell.
2. **Data in testa** (`2026-08-08-oggetto.md`) quando il documento fotografa un momento: audit, handoff,
   decisioni di sessione. Le specifiche per sistema si nominano invece per **sistema**, non per data.
3. **Un documento, una cartella**: se un file è insieme design e handoff, decide il criterio dominante —
   *definisce un sistema* → `design/`; *dice cosa fare nella repo* → `handoff/`. Le due cartelle vivono ora in
   [`../archive/src/`](../archive/src/README.md); un sorgente nuovo di quel tipo nasce qui e ci si sposta
   quando viene recepito.
4. **I sorgenti non si riscrivono.** Il testo originale resta intatto: serve a ricostruire da dove è nata una
   decisione. Le correzioni sono note `⚠️` affiancate, e un elenco in testa che le riassume.
5. **Niente archivi generati**: gli export della wiki (`*.zip`) non stanno nel repository, sono rigenerabili.
6. **Immagini**: `media/`, tranne quelle che appartengono a un bundle auto-contenuto come `showcase/`.

## Come si archivia un sorgente

```bash
git mv docs/src/<cartella>/<file>.md docs/archive/src/<cartella>/<file>.md
```

Poi: banner in testa al file con l'owner che l'ha recepito · riga nella tabella di
[`../archive/src/README.md`](../archive/src/README.md) · **riscrittura dei link**, che scendono di un livello
(`../../` → `../../../`) e vanno aggiornati anche in chi lo cita. L'ultimo passo è quello che si dimentica:
verificarlo risolvendo davvero i link sul filesystem, non a occhio.
