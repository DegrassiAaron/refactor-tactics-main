# Spec — EditorMap: la vista operativa in editor, generata

> **Stato**: Bozza in revisione (design) · **Data**: 2026-08-10 · **Branch**: da aprire
> **Esito di**: `/sc:spec-panel` — panel Wiegers · Cockburn · Adzic · Fowler · Nygard · Crispin
> **Sostituisce**: [`../roadmap-editor.md`](../roadmap-editor.md), ritirata il 2026-08-08 (17 sedute U1–U17)
> **Non è approvata**: §8 registra le decisioni già prese con l'autore, §10 il piano di lavoro non ancora eseguito.

## 1. Il fatto che giustifica il documento

`roadmap-editor.md` è stata ritirata il 2026-08-08 perché era la **terza vista di stato mantenuta a mano** e
aveva perso la gara con il codice. Il ritiro reindirizzava a tre documenti — registro, priorità, guide — ma
**la domanda che quella vista rispondeva non è stata riassegnata a nessuno**.

L'evidenza è che quattro documenti vivi la citano ancora come autorità:

| File | Riga | Cosa afferma |
|---|--:|---|
| `docs/technical/test-manuali-pie.md` | 9 | «**Quale voce affrontare e quando** lo dice `roadmap-editor.md`» |
| `docs/roadmap/roadmap-checkpoint.md` | 424 | la elenca fra i documenti correnti, ruolo «Operativo in editor» |
| `docs/roadmap/roadmap-v0.1.md` | 290, 1361 | idem, e ne fa il tracker di `RT-FEAT-TOOL-MAP-EDITOR` |
| `docs/roadmap/feature-registry.yaml` | 3185, 3189 | `out_of_release_scope: «Tracciato da M9 e da roadmap-editor.md»` |

Un puntatore verso un documento `HISTORICAL` è peggio di un puntatore rotto: **risponde**, e risponde con lo
stato del 2026-08-06. Quindi o si riassegna la domanda, o si tagliano i quattro puntatori — e in quel caso
`RT-FEAT-TOOL-MAP-EDITOR` resta senza tracker dichiarato.

Questa spec sceglie la prima strada, alla condizione che ha fatto fallire la prima: **generata, non scritta**.
È la condizione posta dal ritiro stesso — «servirà se un giorno questa vista verrà *generata* invece che scritta».

## 2. Il difetto da non ripetere, nominato

Il fallimento non fu la prosa: fu che **ogni campo di stato era dichiarato a mano**. `U1 ⏳`, `CP 6.3 🟡`,
«al 2026-08-06 i checkpoint 6.1, 6.2 e 6.3 sono tutti 🟡» — tre affermazioni che nessun comando poteva
smentire, in un documento che nessuno rileggeva perché non era in mezzo al lavoro.

Il repository ha già risolto lo stesso problema quattro volte, ed è la ragione per cui esiste
`scripts/feature_registry.py shortlist`: quattro viste corte (`roadmap` · `featuremap` · `scenariomap` ·
`milestonemap`) dove **ogni numero e ogni simbolo arrivano dalla propria sorgente, misurati**, e la sola cosa
scritta da una persona — la riga di descrizione — viene *preservata* fra un run e l'altro (`preserved_column`).

**L'EditorMap è la quinta vista di quella famiglia.** Non un documento nuovo: una voce nuova in una
infrastruttura che esiste, funziona ed è già sotto `--check`.

## 3. Attore, goal, e perché non è ridondante

**Attore primario**: l'autore, seduto davanti a Unreal, con un blocco di tempo finito e l'editor da aprire.

**Goal**: *«apro l'editor adesso — cosa faccio, in che ordine, e come so di aver finito»*, senza tenere aperti
cinque documenti.

Questo goal **non è coperto** dalle altre viste, ed è il test di non-ridondanza da superare:

| Vista | Risponde a | Perché non basta qui |
|---|---|---|
| `test-manuali-pie.md` | *cosa devo verificare, e com'è andata* | è un **registro**: 135 voci in ordine tematico, senza sequenza né prerequisiti *(diceva `117`, rimisurato il 2026-08-13 con `grep -c '^| \*\*PIE-'`)* |
| `roadmap-checkpoint.md` | *a che punto è il lavoro* | ragiona in checkpoint di **codice**, non in sedute d'editor |
| `scenariomap.shortlist.md` | *chi esegue cosa* | classifica (A/B/C/D), non ordina |
| `featuremap.shortlist.md` | *questa cosa esiste* | stato per feature, non lavoro per persona |
| **EditorMap** | *cosa faccio all'editor adesso* | **sequenza · preparazione condivisa · artefatti · condizione di chiusura** |

Le quattro cose che solo l'EditorMap dice sono le sue **colonne derivate**: ordine, prerequisiti, cosa
produce, quando è finita. Se un giorno una di queste finisce altrove, la vista va ritirata di nuovo.

## 4. Requisiti

Ogni requisito è verificabile da un comando. `R-1`…`R-4` sono la difesa contro il drift; `R-5`…`R-8` sono il
contenuto che l'attore consuma.

| ID | Requisito | Verifica |
|:--:|---|---|
| **R-1** | Nessun simbolo di stato è dichiarato a mano: ogni ✅/🟡/⏳ è derivato dalla sua sorgente | `shortlist --check` fallisce se il file diverge dalle sorgenti |
| **R-2** | La prosa scritta da una persona (procedura, motivazione) **sopravvive** a ogni rigenerazione | rigenerare due volte di fila non cambia il file (idempotenza) |
| **R-3** | Una seduta senza descrizione compare con `—`, non sparisce | un buco è visibile, come nelle altre quattro shortlist |
| **R-4** | Il file dichiara `GENERATA` e il comando che lo riscrive, nel primo blockquote | convenzione già in uso nelle quattro sorelle |
| **R-5** | Ogni seduta dichiara: **produce · sbloccata da · verifica · finita quando · sblocca** | campi obbligatori; assenti solo dove il codice sotto non esiste (regola già usata nei blocchi 5–6 della vista ritirata) |
| **R-6** | Ogni seduta cita gli **ID** delle voci `PIE-*`, mai il loro esito atteso | `grep` di una colonna «esito atteso» in questo file → 0 righe |
| **R-7** | Ogni seduta dichiara i riferimenti trasversali che possiede: issue GitHub, epic/checkpoint, scenari `Scenarios/`, feature `RT-FEAT-*` | i riferimenti risolvono (`check-docs-links.py` per i documenti; `gh issue view` per le issue) |
| **R-8** | La seduta dichiara **cosa non deve cercare** quando la voce è già coperta headless | il registro già distingue «coperto headless» — l'informazione va portata, non ricreata |

**R-6 è la regola che ha tenuto** anche nella vista ritirata, ed è l'unica sua parte da copiare alla lettera:
*questa mappa non ripete mai l'esito atteso di una voce `PIE-*`. Cita l'ID e basta.*

## 5. Architettura: cosa deriva da dove

Nessun campo nuovo inventato. Ogni colonna ha già un owner nel repository.

| Campo della seduta | Sorgente | Come |
|---|---|---|
| Voci `PIE-*` e loro stato | `docs/technical/test-manuali-pie.md` | righe `\| **PIE-…`, stato = primo glifo della penultima cella — **il parser esiste già** nel comando `awk` del registro |
| Stato della seduta | derivato: ✅ sse tutte le sue voci sono ✅ **e** i suoi asset sono in `git` | `git ls-files` sui path degli artefatti |
| Prerequisiti di codice | `roadmap-checkpoint.md` (M*), `roadmap-v0.1.md` §2.1 (E*) | `milestone_status()` / `epic_status()`, già scritte |
| Feature toccate | `feature-registry.yaml` → `gates` | `derive_status()`, già scritta |
| Scenari collegati | `Scenarios/` + capability da `RTScenarioSession.cpp` | `scenario_corpus()` / `available_capabilities()`, già scritte |
| Issue GitHub | dichiarate nella sorgente delle sedute (§8, decisione aperta) | non derivabili: lo script non parla con la rete — **vincolo già accettato** in `milestonemap.shortlist.md` §2 |
| Procedura passo-passo | **scritta a mano**, preservata | `preserved_column` |

**Il rapporto con le sessioni A–G del registro va deciso, non improvvisato.** Oggi
`test-manuali-pie.md` §«Sessioni di verifica consigliate» raggruppa le voci aperte in sette sessioni per
*preparazione condivisa* — che è metà del lavoro di una seduta. Le sedute U1–U17 della vista ritirata e le
sessioni A–G sono **lo stesso concetto modellato due volte, in due file**. Duplicarlo una terza volta è il
modo esatto in cui il drift ricomincia: vedi §8, decisione **D-A**.

## 6. Forma del file

- **Percorso**: `docs/roadmap/editormap.shortlist.md` — la famiglia esistente, stesso posto, stesso suffisso.
- **Marker**: `<!-- RT_SHORTLIST_EDITOR:BEGIN -->` … `:END`, registrato in `SHORTLIST_MARKERS` /
  `SHORTLIST_FILES` di `scripts/feature_registry.py`.
- **Comando**: nessuno nuovo — `python scripts/feature_registry.py shortlist` ne genera cinque invece di quattro.
- **ID delle sedute**: si riusano `U1`–`U17`, che hanno provenienza e sono citati in `git log` e nei piani.

Struttura: §1 tabella a colpo d'occhio (generata) → §2 le sedute in blocchi ordinati (derivato + preservato) →
§3 rapporto con le altre viste (statico).

## 7. Criteri di accettazione

```
Given  una voce PIE passa da ⏳ a ✅ in test-manuali-pie.md
When   python scripts/feature_registry.py shortlist
Then   lo stato della seduta che la contiene si aggiorna da solo
And    la procedura scritta a mano in quella seduta è ancora lì, invariata
```

```
Given  editormap.shortlist.md è stato modificato a mano in una colonna derivata
When   python scripts/feature_registry.py shortlist --check
Then   il comando esce diverso da 0 e nomina la riga divergente
```

```
Given  una seduta dichiara un artefatto (es. DA_HexMap_Arena)
When   quell'asset non è tracciato da git
Then   la seduta non può risultare ✅, qualunque sia lo stato delle sue voci PIE
```

**Gate di consegna**: i quattro puntatori orfani di §1 puntano all'EditorMap; `check-docs-links.py` verde;
`shortlist --check` verde su un albero pulito; rigenerazione idempotente.

## 8. Decisioni — prese con l'autore il 2026-08-10

**D-A · Dove vive la dichiarazione delle sedute** → **file dati dedicato
`docs/roadmap/editor-sessions.yaml`**.

Scartate: parsare le sessioni A–G dalla prosa del registro (parser fragile, e quelle sessioni non dichiarano
artefatti, prerequisiti né condizione di chiusura — non reggono R-5); e una chiave `editor_sessions:` dentro
`feature-registry.yaml` (allarga a 3722 righe la responsabilità di un file che è l'owner dei **gate delle
feature**, che è un'altra cosa).

**Conseguenza accettata**: le sessioni A–G **si spostano** dal registro alla nuova sorgente. Il registro
torna a essere solo un registro — esito atteso e stato, niente sequenza. È la condizione che rende la vista
non ridondante (§3), e va eseguita come *move*, non come copia: due modelli della stessa cosa sono il difetto
che ha ucciso la prima vista.

Schema di record concordato:

```yaml
- id: U1
  title: Mappa-arena hex
  critical: true
  produces: [Content/RT/Maps/Dev/L_HexArena.umap, .../DA_HexMap_Arena.uasset]
  unblocked_by: [CP-6.0]
  shares_setup_with: [U2]
  verifies: [PIE-HEX-MODE-E, PIE-HEX-LAYER]     # ID soltanto — mai l'esito atteso (R-6)
  issues: [33]
  done_when: asset committati e le otto voci hanno esito reale
  unblocks: [U2, CP-6.8]
  steps: |                                       # prosa umana, preservata (R-2)
    1. Livello nuovo in /Game/RT/Maps/Dev/...
```

**D-B · Sedute fuori percorso critico** → **restano tutte e 17**, marcate `critical: false`. Presentazione e
loadout (`U7`–`U9`, `U12`) sono il lavoro che dà un risultato visibile quando se ne ha voglia: toglierle dalla
vista significa non farle mai. La colonna *critico* resta, così la lettura urgente si filtra invece di
perdere il resto.

**D-C · Le issue GitHub** — dichiarate a mano in `issues:`, unico modo possibile: lo script non parla con la
rete. È lo stesso compromesso già accettato e dichiarato in `milestonemap.shortlist.md` §2. Il generatore le
riporta come link senza verificarne lo stato.

## 9. Non obiettivi

- **Non è un manuale**: la procedura passo-passo resta nelle guide (`convenzioni-contenuti-ue.md`,
  `guida-animazioni-paragon.md`, `debug-vs-unreal.md`). I passi espliciti compaiono solo dove **nessuna guida
  copre ancora** — nella vista ritirata erano due casi su diciassette.
- **Non è il registro degli esiti**: quello resta `test-manuali-pie.md`.
- **Non decide lo scope**: milestone e DoD restano nelle roadmap.
- **Non sostituisce il gate G9**: il subset `RELEASE-V01` ha già un owner in `scenario-map.md`.

## 10. Piano di lavoro proposto

| # | Passo | Esito osservabile |
|--:|---|---|
| 1 | Creare `editor-sessions.yaml` con lo schema di §8 | il file esiste, un record di prova valida |
| 2 | Portare le 17 sedute dalla vista ritirata **e** le sessioni A–G dal registro (move, non copia) | 17 record; il registro non contiene più la sezione «Sessioni di verifica consigliate» |
| 3 | Estendere `feature_registry.py`: marker, render, `--check` | `shortlist` genera cinque file |
| 4 | Rigenerare e verificare l'idempotenza | due run consecutive, `git diff` vuoto alla seconda |
| 5 | Reindirizzare i quattro puntatori di §1 | `check-docs-links.py` verde, nessun link a `HISTORICAL` |
| 6 | Aggiornare `roadmap-editor.md`: il banner punta alla vista nuova | la provenienza resta leggibile, l'autorità no |

Il passo 2 è **lavoro di trascrizione, non di invenzione**: le 17 sedute esistono già scritte, con procedure,
dipendenze e artefatti. Vanno rilette contro il codice di oggi — la vista ritirata contiene almeno due note
già superate (`ARTGridActor` rimosso al CP 7.2, i pack Paragon da riscaricare da Fab).
