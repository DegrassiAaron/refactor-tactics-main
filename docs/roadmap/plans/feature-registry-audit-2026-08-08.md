# Audit di tracciabilità delle feature — 2026-08-08

> ## 📸 `HISTORICAL` — REFERTO DI UN AUDIT, NON UNA VISTA DI STATO
>
> Questo documento registra **cosa è stato trovato** costruendo il Feature Registry, su HEAD
> `2094b86`. Non si aggiorna: lo stato vive in [`../feature-registry.yaml`](../feature-registry.yaml)
> e il modello in [`../feature-registry.md`](../feature-registry.md). Serve a rispondere fra sei mesi
> alla domanda «perché quella feature è marcata così».
>
> **Origine**: handoff `RefactorTactics_FeatureRegistry_Roadmap_Wiki_Claude_2026-08-08.md`.

## Base verificata

| | |
|---|---|
| HEAD | `2094b86` (`main`) |
| Suite automatica | **490** test unici in **69** file — misurati, non citati |
| Scenari in `Scenarios/` | **31** con `scenarioId` dichiarato |
| Epic della roadmap di release | **20** (E1–E20), 92 checkpoint |
| Pagine Wiki sorgente | 15 `game/` · 11 `meccaniche/` · 5 `fazioni/` · 42 personaggio |
| Documenti owner distinti referenziati | **60** |

Il conteggio dei test **non** coincide con i 456 dichiarati in `roadmap-v0.1.md` §2 il 2026-08-08:
quella misura precede i merge successivi. Il registry non lo duplica — usa i **nomi** dei test come
riferimento, che è ciò che il validator può verificare.

## A. Feature audit

78 feature. La tabella completa, con owner spec, test, scenari e pagine, si ottiene con
`python scripts/feature_registry.py report`; qui c'è la distribuzione.

| Release | Feature | Distribuzione |
|---|---:|---|
| **v0.1** | 66 | 18 `INTEGRATED` · 15 `RELEASE_READY` · 13 `SPECIFIED` · 13 `IMPLEMENTING` · 5 `TESTABLE` · 1 `DESIGNED` · 1 `DONE` |
| **v0.2** | 8 | 5 `DESIGNED` · 2 `IDEA` · 1 `IMPLEMENTING` |
| **future** | 4 | 2 `IDEA` · 1 `DESIGNED` · 1 `SPECIFIED` |

Nessuna feature di gameplay è `DONE`. L'unica `DONE` è `RT-FEAT-TOOL-VALIDATION`, dove i gate
`scenario`, `ui_wiki`, `packaged` e `network_privacy` sono `na` perché un validator non ha scenari,
UI né build packaged.

### Feature aggiunte rispetto alla seed list dell'handoff

Esistono nel repository e nessuna voce della seed list le copriva:

| FeatureId | Perché |
|---|---|
| `RT-FEAT-ACTION-ENGINE` | È l'ossatura di tutta la sezione «action economy»: 8 CP di E4, 58 test |
| `RT-FEAT-ACTION-EQUIPMENT` | Epic **E7** intera, con `URTEquipmentData` e catalogo già in repo |
| `RT-FEAT-CORE-PLAYBACK` | `URTPlaybackLibrary`, 8 test |
| `RT-FEAT-ENV-TERRAIN` | Base comune delle feature ambientali (costi dal catalogo) |
| `RT-FEAT-ENV-STATUS` | Stati temporanei legati alla cella, CP 8.2 |
| `RT-FEAT-MATCH-FORMAT` | `URTMatchFormatData` + epic **E19** |
| `RT-FEAT-MATCH-END-CONDITIONS` | Fine partita a tre vie, CP 10.3, distinta dagli obiettivi |
| `RT-FEAT-UI-ICON-LANGUAGE` | Epic **E20** (D-031) |
| `RT-FEAT-TOOL-DEBUG-CONSOLE` | `rt.Debug.*` / `rt.Test.*`, CP 11.4 |
| `RT-FEAT-CHAR-PRESENTATION` | Il residuo di **M8**, che nessuna epic v0.1 copre |
| `RT-FEAT-MAP-HIGH-GROUND` | Decisione attiva con uno scenario che la dimostra |
| `RT-FEAT-ENV-ICE-ENGINE` | Separata da `ENV-ICE`: la roadmap dichiara che solo lo scivolamento base è v0.1 |

### Voci della seed list non promosse

| Voce | Stato | Motivo |
|---|---|---|
| `RT-FEAT-ACTION-TRAPS` | `IDEA` | Nessuna fonte: né spec, né epic, né issue. E18 dichiara il framework di trap fuori dal proprio scope |
| `RT-FEAT-CHAR-TRANSFORMATION` | `IDEA` | Il brief esiste solo nel branch `docs/consolidamento-signature-e-trasformazioni`, non in `main` |
| `RT-FEAT-NET-DEDICATED` | `IDEA` | Direzione del PDR, nessun documento owner |
| `RT-FEAT-BOT-TACTICAL` | `IDEA` | Citato in `roadmap-post-v0.1.md` senza owner proprio |
| `RT-FEAT-ACTION-SUPERS` | `IMPLEMENTING` | Esiste la **risorsa** (energia, `UltimateReady`), non una categoria «super» con regole proprie |

Una nota di nomenclatura: la seed list dice «steam», il codice e il catalogo dicono `Smoke` →
`Obscured`. La feature si chiama `RT-FEAT-ENV-STEAM` per non rompere l'id già proposto, ma il titolo
e la nota seguono il codice, che è la fonte verificabile.

## B. Roadmap

La mappa epic → feature è **generata** in `roadmap-v0.1.md` §2.2. Le due roadmap non hanno ricevuto
stato nuovo da mantenere: hanno ricevuto un puntatore.

| Documento | Modifica |
|---|---|
| `roadmap-checkpoint.md` | Nota sulle tre viste + riga nella tabella dei documenti |
| `roadmap-v0.1.md` | Nuova §2.2 fra i marker `RT_FEATURE_BY_EPIC`, rigenerata dal tool |
| `v0.1-definition-of-done.md` | Punto **9** del DoD trasversale · gate di release **G15** |
| `roadmap-post-v0.1.md`, `v0.1-issue-plan.md` | Solo l'allineamento «14 gate» → «15 gate» |

I banner sui documenti storici (§18 dell'handoff) **esistevano già** e sono corretti:
`hex-map-roadmap.md` è `DELIVERED`, `v0.1-issue-plan.md` è `HISTORICAL — snapshot`,
`roadmap-editor.md` è `HISTORICAL — vista ritirata`, il PDR-10 è `CURRENT come requisiti, non come
stato`. Non è stato aggiunto né modificato nessun banner.

## C. Wiki

- **36 pagine** con blocco `RT_FEATURE_STATUS` generato (11 meccaniche, 12 guide, 5 fazioni,
  8 personaggio, più gli indici).
- **28 banner di stato scritti a mano rimossi**: erano la seconda copia dello stato.
- **12 `wiki_note`** nel registry preservano il dettaglio che quei banner portavano.
- Nuova pagina `docs/wiki/feature-status.md` (deploy: `Stato-delle-feature.md`), collegata da
  `index.md`.
- `_Template.md` non chiede più una riga di stato: spiega come collegare la pagina al registry.
- Pagine con più FeatureId: quella di Flux ne ha 3, Riva 3, Bastion 3, Vektor 3.

**Il deploy verso il clone della Wiki non è stato eseguito.** Il comando esiste
(`deploy --wiki-root <path> --write`) e in sola lettura riporta **36 pagine da aggiornare**, ma:

1. la Wiki è un repository **separato e pubblico**: pubblicare va chiesto;
2. quel clone ha una modifica **non committata** di un'altra sessione
   (`Meccanica-facing-e-direzionalita.md`), che il deploy toccherebbe.

Un dato emerso dal deploy in sola lettura: `docs/wiki/game/avversario-bot.md` **non esiste** nel
clone. È una pagina sorgente mai pubblicata.

## D. Workbook

`docs/characters/data/RefactorTactics_Characters_Wiki_Data_v0.4.xlsx`:

- nuova sheet **`15_Wiki_Feature_Refs`**, generata: 80 relazioni `WikiEntityId → FeatureId → Relation`;
- 4 righe di governance nel `00_README`;
- nessuna colonna di stato aggiunta, nessuna percentuale.

`Design_Status` e `Page_Status` **non sono state rimosse**: cancellare dati da un workbook di
authoring non è una decisione da prendere di iniziativa. Sono dichiarate non autoritative nel README.
`Page_Status` misura peraltro un asse diverso — la completezza dei dati della pagina — e resta
legittima.

`docs/balance/RefactorTactics_Balance_Matrices_v0.1.xlsx` non è stato toccato: contiene target e
metodi di misura, non stato di implementazione.

## E. Validator

```text
python scripts/feature_registry.py validate

Feature: 78
  riferimenti wiki   : 80
  riferimenti roadmap: 66
  riferimenti scenari: 56 (+10 pianificati)
  riferimenti test   : 138

errori: 0 · warning: 33
```

`generate --check`, `wiki --check` e `workbook --check` sono tutti allineati. Il gate documentale
preesistente (`scripts/check-docs-symbols.py`) resta verde: 147 documenti, 165 simboli.

I 33 warning sono lacune reali, non rumore: 24 feature senza pagina Wiki (validator, pipeline dati,
performance — cose che una guida al giocatore non descrive), 10 scenari `planned`, alcune feature di
gameplay testabili senza scenario che le dimostri.

### Cosa ha trovato il validator sul registry stesso

Tre riferimenti a test che l'audit aveva assunto esistessero, e che non esistono:
`Actions.Electrify`, `Actions.Ignite`, `Actions.Activate`. L'ultimo è significativo: conferma che
`Action.Activate` è a catalogo ma **nessun test la esercita**, che è la ragione per cui
`RT-FEAT-OBJECTIVE-SYSTEM` ha `runtime: partial`.

Due status dichiarati non erano retti dai gate, e 19 erano **sotto** il valore che i gate reggevano.
È il motivo per cui la coerenza status/gate è diventata un errore e non un avviso: se lo stato è
derivabile, dichiararlo a mano reintroduce esattamente la soggettività che il registry rimuove.

## F. Conflitti — non risolti qui

### F1. Tre feature della v0.1 senza epic

| Feature | Dove vive | Nota |
|---|---|---|
| `RT-FEAT-CHAR-PRESENTATION` | milestone **M8** | Personaggi animati, anelli, leggibilità: M8 è aperta ma nessuna delle 20 epic la copre |
| `RT-FEAT-TOOL-MAP-EDITOR` | milestone **M9** | Residuo H5 dell'editor |
| `RT-FEAT-UI-SCENARIO-BROWSER` | **nessuna** | Nasce dall'issue `#209`, senza checkpoint |

**Decisione richiesta**: assegnare un'epic (o dichiarare che la vista di release non copre il lavoro
di presentazione e di tooling, il che è legittimo ma va scritto). **Impatto**: la §2.2 generata
elenca queste tre a parte a ogni rigenerazione, quindi il buco resta visibile finché non si decide.

### F2. I checkpoint di milestone e quelli di epic condividono la numerazione

`CP 10.1` è «Activate e Interact sugli oggetti» in E10 **e** «Listen server + autorità» in M10.
Idem per 10.2, 10.3 e per 9.1 (copertura bassa in E9, residuo editor in M9).

Nel registry il problema è aggirato con il campo `milestone`, ma il conflitto resta nei documenti.
**Decisione richiesta**: prefissare i checkpoint di milestone (`M10.1`) oppure accettare l'ambiguità
e non citarli mai senza contesto. **Impatto**: oggi un `CP 10.1` scritto in una issue o in un commit
non è disambiguabile senza leggere il contesto.

### F3. La Wiki descrive meccaniche che il gioco non ha

Non è un difetto — la Wiki racconta il gioco progettato — ma prima di questa consegna **non era
dichiarato in modo verificabile**. Casi principali:

| Pagina | Feature | Stato reale |
|---|---|---|
| `Meccanica-overwatch` | `RT-FEAT-REACTION-OVERWATCH` | `SPECIFIED`, 1/9 gate: nessun codice |
| `Meccanica-facing-e-direzionalita` | `RT-FEAT-MAP-FACING` | `SPECIFIED`: ADR accettato, nessun codice |
| `Meccanica-obiettivi-dinamici` | `RT-FEAT-OBJECTIVE-SYSTEM` | `IMPLEMENTING`: la regola c'è, l'oggetto in mappa no |
| `visibilita-rumore-e-informazione` | `RT-FEAT-PERCEPTION-*` | `SPECIFIED`: la vista non decide nulla |
| `Fazioni.md` e le 4 pagine fazione | `RT-FEAT-FACTION-SYSTEM` | `DESIGNED`: esiste solo nel manifest della Wiki, niente nel codice |

I quattro scenari di fazione (`Team.Conflux.FluxRiva.ConductiveFlood` e gli altri tre) sono
dichiarati nel manifest con `scenarioStatus: GATED` e **non esistono** in `Scenarios/`: nel registry
sono `planned`.

### F4. Cinque ScenarioId vivono in un branch non mergiato

`Spec.Cover.TemporaryCoverExpires`, `Spec.Objective.PointSurvivesKO`, `Spec.Overwatch.HoldThenFire`,
`Spec.Perception.HeardNotSeen`, `Spec.Predictive.WhiffOnEmptyCell` sono nel branch
`feat/scenari-test-automatici`. Sono registrati come `planned`, che è la loro condizione su `main`.

**Al merge di quel branch** il validator emetterà un warning per ciascuno («dichiarato planned ma ora
esiste»): vanno promossi a scenari presenti, il che alza il gate `scenario` di quattro feature.

### F5. Il conteggio dei test diverge di nuovo

`roadmap-v0.1.md` §2 dichiara **456 test in 68 file** (2026-08-08); su `2094b86` sono **490 in 69**.
Non è stato corretto in questa consegna: la roadmap è toccata da un'altra sessione in parallelo e la
riga ha una storia sua (quattro correzioni documentate). **Decisione richiesta**: rimisurare al
prossimo merge, come prescrive il documento stesso.

## G. Sessioni parallele

Al momento dell'audit due altri branch toccavano gli stessi file:

| Branch | Sovrapposizione |
|---|---|
| `docs/consolidamento-signature-e-trasformazioni` | 62 file, fra cui `roadmap-v0.1.md`, `roadmap-post-v0.1.md`, `v0.1-issue-plan.md` e il Decision Log |
| `feat/scenari-test-automatici` | I 5 scenari `Spec/` e `scenario-index-e-tag.md` |

Per questo le modifiche alle roadmap sono **additive e chirurgiche** (un blocco delimitato, una riga
di tabella, una sostituzione di parola) invece che riscritture: un merge deve poter succedere senza
arbitrati.

Nessuna voce è stata aggiunta al Decision Log: gli id `D-nnn` si assegnano al merge, e due sessioni
in parallelo hanno già preso lo stesso numero una volta (`D-028`, il 2026-08-08).
