# `docs/src/` — materiale sorgente

> **Non è fonte normativa.** `AGENTS.md`: «`docs/src/` contiene soprattutto input, audit e materiale di
> consolidamento/north-star: **non è fonte normativa per default**». In caso di conflitto prevalgono
> [`../product/piano-canonico-mvp.md`](../product/piano-canonico-mvp.md),
> [`../decisions/RT_PDR_00_Decision_Log.md`](../decisions/RT_PDR_00_Decision_Log.md) e gli ADR applicabili.

Qui vive il materiale **in ingresso** che non è ancora stato consumato: i PRD originari da cui è nato il
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
├── prd/        5  PRD e prompt sorgente, tutti in Markdown
├── design/    27  mockup e sorgenti grafici: fazioni, infografiche, UI
├── media/     28  icone fazioni, icone e mockup HUD, infografiche
├── showcase/   5  scenario «Relay Basin» v0.1: spec, draft JSON, mappe
├── RefactorTactics_Wiki_WidePoster_Integration_Package_v0.2/   37  kit wide-poster, in integrazione
└── CLAUDE_v0.2_Super_Actions_Cooldowns_Docs_Wiki_Roadmap.md    handoff v0.2, non ancora triagiato
```

> ⚠️ **Due cose si sono spostate il 2026-08-08 e non sono più qui**: il dataset personaggi è ora
> [`../characters/data/RefactorTactics_Characters_Wiki_Data_v0.4.xlsx`](../characters/data/RefactorTactics_Characters_Wiki_Data_v0.4.xlsx)
> — dove appartiene, accanto alle 44 schede che lo citano — e il cheatsheet è
> [`../superclaude-cheatsheet.md`](../superclaude-cheatsheet.md), perché non era un sorgente di progetto.

## `prd/` — i PRD originari, consolidati per tema

> ⚠️ **Cambio di regola — 2026-08-12.** Fino a oggi qui vivevano **dieci PDF e un `.docx`**, e questo README diceva: «I PDF
> **non si archiviano**». La regola confondeva due cose diverse — *dove* sta un sorgente e *in che formato*. La
> posizione non cambia: i PRD restano qui, restano livello 8, restano *visione north-star* consultata di
> continuo. Cambia il formato: **il testo vive in Markdown**, per la stessa ragione registrata in
> [D-009](../decisions/RT_PDR_00_Decision_Log.md) e prescritta da PDR-00 §6 #5 — *«i PDF sono snapshot di
> consultazione: le sorgenti testuali devono vivere nel repository Git»*. Un PDF non si diffa, non si `grep`-a
> e non si cita per riga: dieci binari da 2,5 MB rendevano invisibile metà della gerarchia delle fonti.
>
> Gli undici binari sono stati rimossi dal working tree e restano nella storia Git — comando di recupero in
> fondo a questa sezione. **In `docs/` non c'è più prosa in formato binario.**

| File | Cosa contiene | Da quali PDF | Recepito da |
|---|---|---|---|
| [`prd-visione-e-requisiti.md`](prd/prd-visione-e-requisiti.md) | Visione, obiettivi, perimetro, requisiti funzionali del gioco, idee fondative | `idee-base` · `prd-stampabile` · `prd-e-piano-di-sviluppo` · `prd-roadmap-e-percorso-didattico` | canone §2/§5/§8 · `../archive/gameplay/spec-terreni.md` (solo `idee-base`) |
| [`prd-architettura-rete-e-intenti.md`](prd/prd-architettura-rete-e-intenti.md) | Architettura, stack, networking, modello dati, modding, **PRD «Intenti condivisi» completo** | `prd-intenti-condivisi` · `prd-stampabile` · `prd-e-piano-di-sviluppo` · `prd-roadmap-e-percorso-didattico` | canone §5 (invarianti) — **il PRD Intenti condivisi è ancora da recepire** |
| [`prd-personaggi-azioni-e-bilanciamento.md`](prd/prd-personaggi-azioni-e-bilanciamento.md) | Roster, azioni, terreni, coperture, equipaggiamento, catalogo di bilanciamento | `prd-personaggi-e-combattimento-reattivo` · `idee-ruoli-characters` · `catalogo-e-bilanciamento-v0.1` | tutti i `../balance/` · [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) · `../roadmap/roadmap-v0.1.md` |
| [`prd-percorso-didattico-e-produzione.md`](prd/prd-percorso-didattico-e-produzione.md) | Curriculum UE, roadmap, rischi, analytics, gate di release, guida agli asset gratuiti | `prd-stampabile` · `prd-e-piano-di-sviluppo` · `prd-roadmap-e-percorso-didattico` · `guida-trovare-asset-free` | — (fase didattica chiusa; produzione **da recepire**) |
| [`editor-griglia-esagonale-e-mappa.md`](prd/editor-griglia-esagonale-e-mappa.md) | Il **prompt** che ha commissionato il pivot esagonale: assunzioni, architettura, editor, milestone H0–H9 | `editor-griglia-esagonale-e-mappa` (`.docx`) | [ADR-0002](../decisions/adr-0002-griglia-esagonale.md) · [`hex-map-roadmap.md`](../roadmap/hex-map-roadmap.md) — **il sorgente più consumato della cartella** |

L'undicesimo PDF, `sequenza-risoluzione-turno.pdf`, **non è confluito qui**: la sua trascrizione integrale
esisteva già in [`../archive/gameplay/sequenza-turno-exploratory.md`](../archive/gameplay/sequenza-turno-exploratory.md)
(verificata sezione per sezione, similarità 0,91–1,00). Rimetterne il testo avrebbe creato una seconda copia
della stessa cosa, che è precisamente ciò che il principio «una sola fonte logica per concetto» vieta.

> ⚠️ **Una correzione a questa tabella.** Fino al 2026-08-12 `idee-ruoli-characters.pdf` risultava
> **non recepito**. Non era vero: **Flux · Riva · Bastion · Vektor** nascono lì, e le quattro abilità di Flux
> del [catalogo eroi](../balance/RT_HeroCatalog_v0.1.md) — `LinearDischarge`, `ConductiveNode`, `Overload`,
> `ReactiveCapacitor` — sono le sue, nome per nome. Il documento era *già consumato e non registrato*: la
> colonna diceva «c'è ancora lavoro da fare» su un lavoro fatto mesi prima.

Ogni file si apre con una sezione **«Cosa resta vero, cosa no»** che separa tre stati: *recepito nel canone*,
*superato*, *recuperabile e non ancora recepito da nessuno*. È lì che si guarda prima di riaprire un PRD.

**Recuperare un PDF originale** (funziona senza conoscere lo SHA):

```bash
P=docs/src/prd/prd-stampabile.pdf
git show "$(git rev-list -1 HEAD -- "$P")^:$P" > prd-stampabile.pdf
```

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

## `design/` e `media/` — sorgenti grafici

- `design/fazioni/`, `design/infografica/`, `design/ui/` — mockup e sorgenti visivi. La cartella `design/`
  è rinata dopo l'archiviazione del 2026-08-08: non contiene più specifiche di sistema, che ora stanno in
  [`../archive/src/design/`](../archive/src/README.md), ma **materiale grafico** in ingresso.
- `media/fazioni/` — `faction-01..04.png`, icone delle fazioni.
- `media/hud/` — `icon-01..04.png`, `hud-example.png` e `hud-style.png`; sorgente di design in
  [`../archive/src/design/2026-08-08-hud-faction-icons.md`](../archive/src/design/2026-08-08-hud-faction-icons.md).
- `media/infografica/` — 18 infografiche su azioni, turno, feature e interazione con l'ambiente.

## In lavorazione

- `RefactorTactics_Wiki_WidePoster_Integration_Package_v0.2/` — kit di integrazione della wiki, **in corso**.
  I `![…](…)` dei suoi `.md` sono *markdown suggerito*: i path sono relativi alla pagina wiki di
  **destinazione**, non al file che li contiene, quindi non risolvono da qui ed è corretto così.
- [`CLAUDE_v0.2_Super_Actions_Cooldowns_Docs_Wiki_Roadmap.md`](CLAUDE_v0.2_Super_Actions_Cooldowns_Docs_Wiki_Roadmap.md)
  — handoff v0.2 su super action e cooldown, **non ancora triagiato**. Quando un owner lo recepisce, si sposta
  in [`../archive/src/handoff/`](../archive/src/README.md).

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
