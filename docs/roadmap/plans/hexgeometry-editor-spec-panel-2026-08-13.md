# Hex Geometry & Editor + bundle `grid/` — spec panel

> `CURRENT` · **Stato**: revisione chiusa, applicata · **Data**: 2026-08-13
> **HEAD della revisione**: `05bbe3dc` (merge di #714)
> **Sorgenti revisionati** — cinque documenti in due bundle distinti, tutti untracked alla radice:
> `RefactorTactics_HexGeometry_Editor_Claude_Implementation_Brief.md` (1569 righe) ·
> `grid/SUMMARY.md` · `grid/APPLY_WITH_CLAUDE.md` ·
> `grid/roadmap-chat-geometry-water-planning-2026-08-12.md` (630 righe) ·
> `grid/Data_Consolidation_Audit_2026-08-12.xlsx` (2 fogli)
> **Panel**: Wiegers (lead) · Adzic · Cockburn · Fowler · Nygard · Newman · Crispin
> **Sesto della sua famiglia**: segue `map-editor-brief-spec-panel-2026-08-09`,
> `map-sketch-editor-spec-panel-2026-08-12`, `mapeditor-integration-spec-panel-2026-08-12` e
> `level-designer-handoff-spec-panel-2026-08-12`.

---

## 1. Il verdetto in una riga

Il brief HexGeometry è **il documento più accurato della serie** — ventinove verifiche di stato, zero
premesse false — e proprio per questo il suo unico errore è interessante: prescrive al §12 una cosa che è
stata **decisa in senso contrario venti ore prima**. Il bundle `grid/` ha un perimetro diverso, in gran parte
già recepito, e propone una roadmap manuale che il brief stesso elenca fra gli errori da non fare.

| | Voci | Significato |
|---|---:|---|
| 🔴 Critico | **4** | contraddice una decisione chiusa, o duplica un'authority |
| 🟠 Alto | **6** | il lavoro parte, ma su una base da decidere prima |
| 🟡 Medio | **5** | attrito, tracciabilità |

**La scoperta centrale non è in nessuno dei due documenti**: il repository contiene **due modelli di
calpestabilità** che nessuno ha mai messo uno accanto all'altro. Vedi `C1`.

---

## 2. Le verifiche che hanno deciso la revisione

Fatte contro HEAD `05bbe3dc`, non contro lo snapshot dei documenti.

### 2.1 Il brief HexGeometry è accurato — 29 verifiche di stato su 29

Il conteggio è la somma delle entità che il brief **dichiara**: 10 issue + 7 simboli + 8 feature ID +
4 decisioni = **29**. La quinta riga della tabella non è una di quelle: è la ricerca che ha prodotto il
lavoro, e si conta a parte.

| Verifica | Esito |
|---|---|
| Stato delle **10 issue** del §31 (`#620` `#621` `#622` `#687` `#695` `#711` `#712` `#324` `#554` `#619`) | ✅ **10/10 esatte**, inclusi `#554`/`#619` `CLOSED` e `#324` epic post-v0.1 |
| I **7 simboli** che il §9/§17/§22 dice di riusare (`ReachableCells`, `FromCell`, `EdgeMidpointWorld`, `EdgeRotation`, `OppositeDirection`, `OnlyTheCellsComponentIsClickable`, `RebuildInstances`) | ✅ **7/7 esistono** |
| Le **8 feature** citate dai due bundle | ✅ **8/8 esistono** nel registry |
| Le decisioni `D-071` `D-081` `D-082` `MSE-1` | ✅ tutte registrate |
| «non esiste un owner tecnico unico» (§27) | ✅ **confermato** — il tema compare solo in referti, archivio e registry, mai in `docs/technical/` |

È il primo handoff della serie che non richiede correzioni di stato. Merita di essere detto: i cinque
precedenti ne avevano da due a nove ciascuno.

### 2.2 Il bundle `grid/` propone gate che il repository passa già

| Gate proposto (§11 di `APPLY_WITH_CLAUDE.md`) | Misura |
|---|---|
| `WaterDepth` come asse normativo | ✅ **già superseded**: 0 occorrenze nel codice, 9 nei documenti — 8 in archivio/referti e **1 in `spec-terreni-e8.md` §6-ter, che è la sezione che lo dichiara superseded da `D-081`** |
| `BreachSlot` | ✅ 0 nel codice, 8 in archivio/referti |
| `TransitOnly` | ✅ 0 nel codice |
| Convenzione `Spec.<Area>.<Name>`, niente `SCN-*` | ✅ già rispettata: 73 scenari, 35 in `Scenarios/Spec/`, schema `"scenarioId": "Spec.Area.Name"` |

---

## 3. Findings

### 🔴 Critici

#### C1 — Due modelli di calpestabilità, mai confrontati *(Nygard · Fowler)*

Il repository risponde alla domanda «questa cella è calpestabile?» in **due modi diversi**, entrambi
documentati, entrambi corretti nel proprio contesto, e **mai messi in relazione**:

| | Modello | Dove vive | Stato |
|---|---|---|---|
| **A** | **Cerchio inscritto** — l'ingombro dell'unità è il cerchio più grande dentro l'esagono; se tocca il muro la cella non è valida. Binario. | `D-071` · Wiki `Meccaniche/griglia-e-geometria.md` · `RT-FEAT-MAP-STANDABILITY` | `DESIGNED`, gate 0/8, **v0.2** |
| **B** | **Dodici settori con soglie** — si conta quanti settori da 30° la geometria invade: `≥4` → `Constrained`, `≥6` → `Blocked`. Ternario. | `#619` · `RTHexOccupancyLibrary` | **implementato e testato**, 19 test |

I due danno **risposte diverse sulla stessa geometria**. Un muro appoggiato a un lato dell'esagono è
*tangente* al cerchio inscritto — il modello A lo tratta come caso limite — mentre il modello B, misurato il
2026-08-13, gli assegna **4 settori su 12**, cioè `Constrained`. Con due muri il modello B dice `Blocked`
(6/12) mentre quattro lati restano aperti.

Nessuno dei due bundle nota la collisione: il brief §7 presenta **solo** il modello B come «baseline
corrente»; il bundle `grid/` §1 dichiara **solo** che «prevale D-071: footprint/cerchio inscritto». Letti
insieme, i due documenti prescrivono due sistemi diversi per la stessa domanda.

➡️ Registrata come **`MSE-3`** in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md). Non la decide questa
revisione: è una scelta d'autore, e ha un innesco preciso — `#621`, che cuoce il risultato in
`bBlocksMovement`.

#### C2 — Il §12 del brief riapre una decisione chiusa venti ore prima *(Wiegers)*

Il brief prescrive:

```text
Void / cliff footprint  →  ERTHexSurface::Void
```

Questa esatta proposta è stata **valutata e respinta il 2026-08-12**, ed è registrata in
[`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md): *«Deciso: il bake non scrive `Surface`»*, con tre ragioni di
cui la seconda decide — `Fill` propaga sulla contiguità di superficie, quindi una `Surface` cotta cambierebbe
il confine di **ogni futuro flood fill**, cioè lo strumento e non il dato.

Il brief non poteva saperlo: è stato scritto prima che quella decisione fosse committata. Ma chi lo esegue
**sì**, ed è per questo che il suo stesso §1 impone di rimisurare. Il precipizio resta esprimibile senza
`Surface`: `bBlocksMovement = true` + `bBlocksLineOfSight = false`.

⚠️ Il §12 è citato dal §32 Step B come parte della DoD di `#621`. Chi apre `#621` con il brief in mano
implementerebbe la cosa respinta.

#### C3 — Il bundle `grid/` è la roadmap manuale che il brief vieta *(Fowler · Newman)*

`roadmap-chat-geometry-water-planning-2026-08-12.md` propone **R0–R11** con **25 work item** numerati e una
tabella «sequenza issue consigliata». Il brief HexGeometry §40 elenca fra gli errori da evitare:

> ❌ nuova roadmap manuale che duplica Feature Registry

Le due posizioni sono incompatibili, e ha ragione il brief: le 8 feature che il bundle vuole tracciare
**esistono già** nel registry, con i loro gate e il loro stato derivato. Il bundle è utile come **inventario
di design** — e in quel ruolo è buono — non come tracker.

➡️ Il bundle si archivia come sorgente, e ciò che sopravvive del suo contenuto entra negli owner. Non
diventa un documento di roadmap.

#### C4 — `spec-mappa-multilivello.md` è `CURRENT` e stale di quattro versioni *(Crispin)*

| Dichiara | Misura |
|---|---|
| `CurrentFormatVersion = 3` (§6) | **7** (`RTHexMapAsset.h:65`) |
| Tabella campi cella (§3): `Height`, `Surface`, `MoveCost`, `bBlocksMovement`, `bBlocksLineOfSight`, `Covers` | mancano **`Doors`** (v4) e **`OccupancySurcharge`** (v7, `RTHexCellData.h:163`) |

Il documento porta l'etichetta `CURRENT` e dice «as-built, allineata al codice». Non lo è da quattro
versioni di formato. È l'owner che il brief §29 chiede di consolidare, e il difetto è precisamente quello
che un consolidamento deve trovare.

### 🟠 Alti

| # | Esperto | Problema |
|---|---|---|
| H1 | Nygard | Il §7 del brief dà `0–3 / 4–5 / 6+` come baseline stabile. `MSE-2`, aperta il 2026-08-12 e **quantificata il 2026-08-13**, dice che due muri perimetrali bastano a produrre `Blocked` con quattro lati aperti. Chi legge il §7 crede che le soglie siano un fatto |
| H2 | Wiegers | L'owner tecnico manca davvero (verificato). Il brief ha ragione, e questa revisione lo crea: [`spec-hex-geometry-authoring.md`](../../technical/systems/spec-hex-geometry-authoring.md) |
| H3 | Crispin | La Wiki `Meccaniche/griglia-e-geometria.md` **esiste già** e spiega bene il principio, ma **non nomina i dodici settori**: il vocabolario che il brief §30 chiede («12 settori come misura di occupancy, non movimento») è l'unico pezzo mancante |
| H4 | Newman | `#687` è un **bug P1 aperto**: `FormatVersion` non finisce nei byte serializzati. Sia il brief §37 sia `C4` toccano lo stesso punto. Finché è aperta, nessuna DoD può dipendere dall'avvio di una migrazione |
| H5 | Adzic | Il bundle propone 7 scenari `Spec.Environment.*`: **zero esistono**. La convenzione è giusta e il corpus c'è (35 in `Scenarios/Spec/`), ma sono scenari di capability **non ancora implementate** — restano `planned`, e vanno scritti quando la capability esiste, non prima |
| H6 | Cockburn | Nessuno dei due bundle nomina un **owner umano** né una condizione di scadenza. Il quinto handoff consecutivo con questo difetto |

### 🟡 Medi

| # | Problema |
|---|---|
| M1 | `RefactorTactics_MapEditor_Roadmap_Issue_Integration_2026-08-12.md` è **identico** (a meno dei CRLF: 274 byte di delta su 274 righe) al già archiviato `2026-08-12-mapeditor-roadmap-issue-integration.md`. Già consumato il 2026-08-12, PR #713. Rimosso, non ri-consumato |
| M2 | Il workbook `Data_Consolidation_Audit_2026-08-12.xlsx` è un audit di governance, non dati di gioco. I suoi due fogli dicono correttamente di **non** promuovere il Balance workbook a canone — cosa che `docs/balance/README.md` già stabilisce (`D-023`) |
| M3 | Il §43 del brief propone 7 branch sequenziali; il lavoro reale di consolidamento è documentale e sta in uno |
| M4 | Il bundle `APPLY_WITH_CLAUDE.md` §2 propone di sostituire `CLAUDE.md`, `AGENTS.md` e `README.md` con «replacement» che il bundle non contiene: i tre file citati non esistono in `grid/` |
| M5 | Il §16 del bundle propone milestone con lettere (`G`, `M`, `V`, `S`, `W`, `E`, `I`, `U`, `Q`) mentre il repository usa `M<n>`/`E<n>`: un terzo vocabolario di milestone |

---

## 4. Qualità dei sorgenti

Giudizio qualitativo del panel, **non** una misura.

| Dimensione | Brief HexGeometry | Bundle `grid/` | Motivo |
|---|---:|---:|---|
| Accuratezza dello stato | **10/10** | 7/10 | 29 verifiche di stato su 29 nel primo; il secondo propone gate già verdi |
| Chiarezza | 9/10 | 8/10 | Il §45 «sintesi non negoziabile» è il miglior riassunto del modello prodotto finora |
| Testabilità | 8/10 | 5/10 | Il §33 separa correttamente automation e PIE; il bundle elenca scenari per capability inesistenti |
| Non-duplicazione | 9/10 | 4/10 | Il §40 del brief è una checklist di anti-duplicazione esemplare; il bundle duplica il registry |

**Da preservare**: il **§45** del brief (dieci righe non negoziabili) e il **§26** (tabella
concetto → domanda → authority). Sono entrati quasi intatti nell'owner tecnico.

---

## 5. Cosa è stato applicato

| Azione | Dove |
|---|---|
| **Creato** l'owner tecnico che mancava | [`docs/technical/systems/spec-hex-geometry-authoring.md`](../../technical/systems/spec-hex-geometry-authoring.md) `CURRENT` |
| **Corretto** `C4`: FormatVersion 3 → 7, aggiunti `Doors` e `OccupancySurcharge`, puntatore all'owner | [`spec-mappa-multilivello.md`](../../technical/architecture/spec-mappa-multilivello.md) |
| **Aperta** `MSE-3` (i due modelli di calpestabilità) | [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) |
| **Aggiornata** `MSE-2` con i numeri misurati | idem |
| **Archiviati** brief e bundle, con riga d'indice ciascuno | `docs/archive/src/handoff/` |
| **Rimosso** il duplicato `M1` | — |
| Wiki: aggiunto il vocabolario dei dodici settori (`H3`) | `Meccaniche/griglia-e-geometria.md` |

---

## 6. Cosa NON è stato fatto, e perché

Il consolidamento è **documentale**. Non è stato implementato codice di `#620`/`#621`/`#622`/`#711`/`#712`:
il brief stesso (§43) chiede una PR per issue con scope stretto, e `C1` dice che `#621` non è pronta —
la sua prima riga dipende da una decisione che non esiste ancora.

`MSE-3` e `MSE-2` sono **decisioni d'autore**: questa revisione le pone e le documenta, non le chiude.
