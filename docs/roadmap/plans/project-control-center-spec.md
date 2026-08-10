# Spec — Project Control Center: la dashboard è una UI, non una sesta verità

> **Stato**: Bozza in revisione (design) · **Data**: 2026-08-10 · **Branch**: `docs/consolidamento-roadmap`
> **Esito di**: `/sc:spec-panel --mode critique` — panel Wiegers · Cockburn · Adzic · Fowler · Newman · Nygard · Crispin
> **Fonte**: [`../../src/RefactorTactics_Project_Control_Center_Claude.md`](../../src/RefactorTactics_Project_Control_Center_Claude.md), handoff dell'autore
> **Tracciata da**: `RT-FEAT-TOOL-CONTROL-CENTER` nel registry — `release: future`, fuori dai gate della v0.1.
> **§9 è decisa** (2026-08-10, con l'autore); di **§11 sono eseguiti i passi 1, 2 e 7** — il contratto dati,
> il suo gate e la `My Editor Queue`. **Nessuna riga di pagina è stata scritta**: i passi 4–6 e 8 restano.

## 1. Perché l'handoff non si applica alla lettera

L'handoff è una **fotografia**, e il repository si è mosso dopo lo scatto. Metà delle sue richieste sono già
state soddisfatte — alcune il giorno prima — e una è stata soddisfatta **in modo deliberatamente diverso** da
come lui la propone. Applicarlo voce per voce ricostruirebbe cose che esistono e violerebbe una decisione
presa il 2026-08-10, poche ore prima che il documento arrivasse.

Verifica eseguita su `main @ 918f54c`: 84 feature nel registry, `validate` → **0 errori · 34 warning**,
`scripts/feature_registry.py` a 1860 righe con otto comandi, cinque shortlist generate, 19 sedute in editor.

| § della fonte | Cosa chiede | Stato reale | Cosa resta |
|---|---|---|---|
| §1 | Nessun DB duplicato, il registry resta sorgente | **già così** per costruzione | — |
| §2.1 Overview | Avanzamento, blocchi, warning | dati già tutti derivabili; nessuna vista che li unisca | **la pagina** |
| §2.2 Roadmap | Milestone → Epic → CP → Issue → Feature | `roadmap.shortlist.md` + `milestonemap.shortlist.md` | la resa web |
| §2.3 Feature Map | Card con i campi del registry | `featuremap.shortlist.md`; i campi esistono tutti nel JSON | la resa web |
| §3 Scenario Map | Scenario → feature/issue/test | `scenariomap.shortlist.md` + `scenario_corpus()` | la resa web |
| §4 Editor Map | Vista nuova, solo lavoro umano | **FATTA** — issue #371 chiusa, PR #373 mergiata | — |
| §5 My Editor Queue | Raggruppare `BLOCKING/READY/WAITING/DONE` | parziale: la EditorMap deriva ⏳/🟡/✅ e `critical`, **non** il raggruppamento per sbloccabilità | il raggruppamento, se lo si vuole |
| §6 Relazioni fra mappe | Viste dello stesso grafo | il grafo esiste nei dati; nessuna navigazione | **la pagina** |
| §7 Link derivati da config | `owner/repo/branch` centrale | non esiste: i link sono scritti nei markdown | il blocco di config |
| §8 Relazioni qualificate | `issues: [{id, relation}]` | non esiste: liste piatte | decisione **D-B** (§9) |
| §9 Editor Task schema | `editor_tasks:` nel registry | **decisa in senso opposto** — vedi §8 | non riaprire |
| §10 Sorgente GitHub | Leggere il `.yaml` raw | il `.json` generato è un contratto migliore — **D-1** (§5) | il loader |
| §11 Read-only prima | Nessuna scrittura su `main` | — | vincolo accettato |
| §12 Progresso derivato | `N/M gate`, mai percentuali | **già così**: `gates_done` / `gates_applicable` nel JSON | — |
| §13 Stato e DoD | Status coerente coi gate | **già così**: `derive_status()`, il validator fallisce se divergono | — |
| §16 Warning e validazione | 12 classi di diagnostica | **già tutte** in `validate()`, oggi solo su stdout | esporre l'output (**D-2**) |
| §19 punti 3–5 | Validator Editor Task, `editormap.shortlist` | **fatti** | punti 1, 2, 6–11 restano condizionali |
| §21 Test | 13 classi di test | quelli sul registry esistono nel validator | i test del loader e del grafo |

**Il verdetto in una riga**: la fonte chiede quattordici cose, il repository ne ha già fatte sette, e la
sola cosa nuova che chiede davvero è **una pagina web**. Il resto è vestire dati che esistono.

## 2. Il difetto da non ripetere, nominato

Questo repository ha ucciso una vista, il 2026-08-08, per un motivo preciso: `roadmap-editor.md` era la
**terza vista di stato mantenuta a mano** e aveva perso la gara col codice. È tornata dieci giorni dopo solo
alla condizione di essere **generata**.

Una dashboard è la forma più seducente dello stesso difetto, perché non sembra un documento. I due modi in cui
tradirebbe:

1. **Riderivare in JavaScript** una regola che vive già in Python. `derive_status()` è la definizione di cosa
   significa `RELEASE_READY`. Una seconda implementazione nel browser è una seconda verità che *diverge in
   silenzio*, e diverge sul dato che la pagina esiste per mostrare.
2. **Chiedere un campo nuovo alla UI.** Un raggruppamento `BLOCKING/READY/WAITING` scritto a mano nella pagina
   sarebbe informazione di progetto che vive **solo** nella dashboard: esattamente ciò che §23 della fonte
   vieta, e il modo in cui la vista precedente è morta.

Regola operativa che ne discende, e che vale come criterio di rifiuto in code review:

> **La dashboard non calcola stato. Lo riceve già calcolato, e lo dipinge.**
> Se una vista richiede un valore che il generatore non produce, si estende **il generatore**, non la pagina.

## 3. Attore e goal

**Attore primario**: l'autore, che vuole rispondere in trenta secondi a *«a che punto è il progetto, e cosa
sta bloccando cosa»* senza aprire cinque markdown e incrociarli a mente.

**Goal**: la navigazione fra viste che i markdown non possono dare — da una feature alle sue issue, ai suoi
scenari, alla seduta in editor che la sblocca, e ritorno.

**Il test di non ridondanza**, che questa pagina deve superare come lo superò la EditorMap:

| Vista | Risponde a | Perché non basta |
|---|---|---|
| `featuremap.shortlist.md` | *questa cosa esiste, a che gate* | lista lineare: nessun filtro, nessun salto |
| `roadmap.shortlist.md` | *cosa viene prima* | ordine di esecuzione, non relazioni |
| `editormap.shortlist.md` | *cosa faccio davanti all'editor* | un solo tipo di lavoro |
| `feature-registry.json` | tutto, in forma macchina | non si legge |
| **Control Center** | *cosa blocca cosa, e perché questa non è Done* | **filtri · navigazione del grafo · diagnostica visibile** |

Le tre cose in grassetto sono le sole che giustificano la pagina. Se un giorno finiscono altrove, la pagina
si ritira — la stessa clausola che `editormap-spec.md` §3 si è imposta.

## 4. Cosa esiste già, e va consumato invece che riscritto

| Artefatto | Cosa offre alla pagina |
|---|---|
| `docs/roadmap/feature-registry.json` | 84 feature con `status`, `status_derived`, `gates`, `gates_done`, `gates_applicable`, `roadmap`, `dependencies`, `completed_by`, `owner_specs`, `issues`, `tests`, `scenarios`, `scenarios_planned`, `wiki_refs`, `last_verified`, `notes` |
| `feature_registry.py validate` | 12 classi di diagnostica, oggi su stdout |
| `feature_registry.py report` | audit testuale già renderizzato |
| `epic_status()` · `milestone_status()` · `release_gates()` | stato di epic, milestone e gate di release, **misurati** |
| `scenario_corpus()` · `available_capabilities()` | scenari reali e capability del harness |
| `editor-sessions.yaml` + `session_state()` | 19 sedute con `unblocked_by`, `unblocks`, `verifies`, `artifacts`, `critical` |
| `pie_entries()` | stato delle voci `PIE-*` letto dal registro |

Nulla di questo va reimplementato. Sette funzioni su otto che servono alla Overview **sono già scritte**.

## 5. Decisione architetturale — cosa legge la pagina

**D-1 · La sorgente della pagina è `feature-registry.json`, non il `.yaml` raw.**

La fonte (§10) propone `raw.githubusercontent.com/.../feature-registry.yaml`. Si scarta, per tre motivi che
sono tutti verificabili:

1. **Nessun parser YAML nel browser.** 3737 righe con ancore, blocchi `>-` e commenti semantici: un parser
   in più è una superficie di bug in più, per un dato che esiste già serializzato.
2. **`status` e i gate sono già derivati *e validati*.** Il JSON porta `status_derived`, `gates_done`,
   `gates_applicable`: leggere lo YAML costringerebbe la pagina a riderivarli — il difetto §2.1.
3. **Il JSON dichiara la propria provenienza**: `_generated` e `meta.last_full_audit` sono già i campi che
   la fonte chiede di mostrare in testata (§10).

**Il rischio che questa scelta introduce, e come si misura.** Il JSON è generato a mano e **non c'è CI**:
`.github/` non esiste in questo repository. Un `.yaml` modificato senza `generate` produce una pagina
plausibile e vecchia — il difetto peggiore, perché non si vede.

Contromisura, derivabile senza CI e senza backend:

```text
GET /commits?path=docs/roadmap/feature-registry.yaml   → sha_yaml, date_yaml
GET /commits?path=docs/roadmap/feature-registry.json   → sha_json, date_json

date_yaml > date_json  →  banner rosso in testata:
"registry rigenerato l'ultima volta N commit fa — i dati mostrati possono essere vecchi"
```

La pagina non può *impedire* il drift, ma può **renderlo visibile**, che è la sola cosa che le compete.

## 6. Il buco del contratto dati

Il JSON copre le feature. Non copre le altre quattro viste. Oggi lo stato delle sedute in editor esiste
**solo dentro `render_shortlist_editor()`**, e finisce solo nel markdown: una dashboard che lo volesse
dovrebbe parsare il markdown generato o ricalcolarlo — entrambe seconde verità.

| Dato che la pagina richiede | Dove vive oggi | Esce dal generatore? |
|---|---|---|
| Feature, gate, status | `feature-registry.json` | ✅ |
| Diagnostica del validator | stdout di `validate` | ❌ |
| Stato epic / milestone | `epic_status()`, `milestone_status()` | ❌ (solo markdown) |
| Sedute in editor + stato derivato | `session_state()` | ❌ (solo markdown) |
| Voci `PIE-*` e loro stato | `pie_entries()` | ❌ (solo markdown) |
| Corpus scenari + capability | `scenario_corpus()` | ❌ (solo markdown) |

**D-2 · Il contratto si estende, non si duplica.** Un solo artefatto generato in più — proposta
`docs/roadmap/project-graph.json` — prodotto dallo stesso `generate`, che serializza ciò che le cinque
shortlist già calcolano. Nessuna funzione nuova: solo un secondo consumatore di funzioni esistenti.

Alternativa scartata: gonfiare `feature-registry.json`. È il contratto delle **feature** e ha già consumatori;
allargarlo a sedute e diagnostica ripete su di esso l'errore che `editormap-spec.md` §8 ha evitato sul
`.yaml`.

## 7. Requisiti

Ogni requisito è verificabile da un comando o da un'osservazione. `R-1…R-4` difendono dal drift; `R-5…R-11`
sono il contenuto.

| ID | Requisito | Verifica |
|:--:|---|---|
| **R-1** | Nessuno stato è calcolato nella pagina: ogni `status`, gate, ✅/🟡/⏳ arriva da un artefatto generato | `grep` di logica di derivazione nel sorgente della pagina → 0 occorrenze |
| **R-2** | Nessuna percentuale è inventata: il progresso si legge `N/M gate`, rispettando `na` | una feature con `network_privacy: na` mostra `8` come denominatore, non `9` |
| **R-3** | La pagina non scrive su GitHub | nessuna chiamata `PUT`/`POST` nel sorgente |
| **R-4** | La testata dichiara branch, SHA del dato, data di rigenerazione e data dell'audit | ispezione; e il banner di §5 compare se il `.yaml` è più recente del `.json` |
| **R-5** | Ogni riferimento diventa un link derivato dalla config `owner/repo/branch` — nessun URL assoluto scritto a mano | `grep "https://github.com/"` nel sorgente → solo dentro la funzione che li costruisce |
| **R-6** | Un riferimento che non risolve è **mostrato come rotto**, non nascosto | test: una `issue: 99999` in un fixture compare marcata |
| **R-7** | La Overview mostra i 34 warning del validator, raggruppati per classe | il conteggio in pagina è uguale a quello di `validate` sullo stesso commit |
| **R-8** | Da una feature si raggiunge in un clic: issue, scenari, test, spec owner, Wiki, epic/milestone, sedute in editor | navigazione manuale su `RT-FEAT-REACTION-OVERWATCH` |
| **R-9** | Esistono le relazioni **inverse**: da uno scenario alle feature che valida; da una seduta alle feature che sblocca | navigazione manuale in entrambi i versi |
| **R-10** | I filtri di §14 della fonte operano sui campi reali del registry, senza campi nuovi | ogni filtro corrisponde a una chiave del JSON |
| **R-11** | La pagina funziona anche offline sul file locale, non solo su GitHub | apertura con il `.json` del working tree |

`R-1` è quello che conta: è la traduzione operativa di §2, e il solo criterio che rende questa pagina diversa
dalla vista che è già morta una volta.

## 8. Editor Task — la decisione esiste già, non si riapre

La fonte (§9) propone `editor_tasks:` dentro `feature-registry.yaml`, con `task_id`, `category`,
`related_features`, `blocked_by_issues`, `instructions`, `evidence`.

**Questa decisione è stata presa il 2026-08-10 e va nell'altra direzione**
([`editormap-spec.md`](editormap-spec.md) §8, decisione **D-A**): file dati dedicato
`docs/roadmap/editor-sessions.yaml`, perché allargare il registry gli farebbe portare una responsabilità che
non è la sua. L'implementazione è in `main` (issue #371, PR #373).

Traduzione dei campi, per chi legge la fonte e cerca il proprio schema:

| Fonte §9 | Realtà nel repository | Nota |
|---|---|---|
| `task_id` | `id` (`U1`…`U19`) | ID riusati dalla vista ritirata: hanno provenienza in `git log` |
| `status` | **non esiste, ed è voluto** | derivato da `verifies` + `git ls-files`: `session_state()` |
| `priority` | `critical: true/false` | binario, non `P0…P3` |
| `related_features` | `unblocks` / `unblocked_by` | in termini di checkpoint e sedute |
| `blocked_by_issues` | `issues` | dichiarate a mano: lo script non parla con la rete (**D-C**) |
| `instructions` | `steps` | prosa preservata fra le rigenerazioni |
| `evidence` | `artifacts` + `done_when` | l'artefatto si verifica con `git ls-files` |

**Il `status: TODO` della fonte è il difetto centrale**: sarebbe un campo di stato scritto a mano, cioè
precisamente ciò che ha ucciso `roadmap-editor.md`. Non va introdotto.

## 9. Decisioni — prese con l'autore il 2026-08-10

**D-A · Tecnologia** → **un singolo `.html` statico**, `fetch` sugli artefatti generati, apribile da file o
servito da GitHub Pages. Zero build, zero dipendenze: non introduce una toolchain in un repository che oggi
è C++ e Python e non ha CI. Scartate: una app con build step, e una con backend.

**Conseguenza accettata**: senza backend la pagina **non potrà mai scrivere** su GitHub. Il giorno in cui la
scrittura servisse davvero, è un progetto diverso, non un'evoluzione di questo. Detto ora, non scoperto dopo.

**D-B · Relazioni qualificate** → **rimandate**. `issues: [{id: 142, relation: implements}]` ha valore reale,
ma costa una migrazione di 85 feature e un validator esteso. Si fa **dopo** che la pagina ha dimostrato che
il raggruppamento serve: cambiare schema per una UI che non esiste è il verso sbagliato.

**D-C · `My Editor Queue`** → **filtro derivato, e anche in markdown**. Non una vista nuova: i quattro gruppi
si calcolano da ciò che le sedute già dichiarano. La conseguenza è che questo passo **non aspetta la pagina** —
è lavoro in `render_shortlist_editor()`, e la EditorMap lo mostra da sola.

Regola di derivazione, sui campi esistenti:

| Gruppo | Condizione |
|---|---|
| **DONE** | stato della seduta ✅ |
| **WAITING** | almeno un `unblocked_by` non risolto (⏳ o senza glifo) |
| **BLOCKING** | tutti i prerequisiti risolti (✅ **o** 🟡) · `critical: true` · non ✅ |
| **READY** | tutti i prerequisiti risolti · `critical: false` · non ✅ |
| *(fuori)* | sedute senza stato derivabile: il codice sotto non esiste ancora |

**Un 🟡 conta come risolto**, ed è il punto: il prerequisito è che il *codice* sia fatto, e a quel checkpoint
manca proprio la verifica che porti tu. Aspettare il ✅ è aspettare sé stessi — la EditorMap lo dichiara già
nella propria §1.

Questa derivazione **è diventata possibile il 2026-08-10**, quando `unblocked_by` ha smesso di usare la forma
ambigua `CP 6.3` e ha adottato i prefissi `M6.3` / `E1.3` / `E8` / `U13`
([`consolidamento-roadmap-2026-08-10.md`](consolidamento-roadmap-2026-08-10.md) §2). Prima, «prerequisito
risolto» non era calcolabile: 20 numeri su 22 puntavano a due checkpoint diversi.

**D-D · Scrittura** → **fuori scope**, e vincolata da D-A come sopra.

**Dove vive questo lavoro** → feature `RT-FEAT-TOOL-CONTROL-CENTER` nel registry, `release: future`, **senza
epic e senza milestone**: è tooling di processo, e metterlo nella roadmap di release lo farebbe competere con
i 15 gate della v0.1. Precedente identico: `RT-FEAT-UI-SCENARIO-BROWSER`.

## 10. Non obiettivi

- **Non è una source of truth**: nessun dato nasce nella pagina (§23 della fonte, e §2 di questo documento).
- **Non sostituisce le shortlist**: restano le viste leggibili in repository, e restano generate.
- **Non è un tracker di issue**: GitHub resta l'owner di stato e discussione delle issue.
- **Non decide lo scope**: milestone e DoD restano nelle roadmap.
- **Non scrive su `main`**: nemmeno indirettamente.

## 11. Piano di lavoro proposto

Ordine scelto perché ogni passo è verificabile da solo, e i primi due hanno valore **anche se la pagina non
viene mai scritta**.

| # | Passo | Esito osservabile |
|--:|---|---|
| 1 | ✅ **2026-08-10** — `project-graph.json` generato da `generate`: diagnostica, gate di release, epic/milestone/checkpoint, sedute, voci PIE, corpus scenari, capability | il file esiste; due run consecutive → `git diff` vuoto |
| 2 | ✅ **2026-08-10** — `--check` sul nuovo artefatto, come per le shortlist | verificato per mutazione: una riga aggiunta a mano → exit 1, file nominato |
| 3 | Blocco di config `project.github` — owner, repo, branch — in un solo posto | nessun URL assoluto altrove |
| 4 | Pagina v0.1 read-only: testata + Overview + banner di staleness | apre da file locale (R-11) e da GitHub |
| 5 | Feature Map con filtri e detail drawer | i filtri di §14 operano; `RT-FEAT-*` raggiungibile in un clic |
| 6 | Roadmap, Scenario Map, Editor Map come viste dello stesso grafo | relazioni inverse navigabili (R-9) |
| 7 | ✅ **2026-08-10** — `My Editor Queue` in `render_shortlist_editor()` (**D-C**): i quattro gruppi anche in markdown | 8 `BLOCKING` · 4 `READY` · 4 `WAITING` · 0 `DONE`, derivati; le due regole di risoluzione verificate per mutazione |
| 8 | Test: parsing, unicità, derivazione link, riferimenti rotti, cicli, filtri | fixture con `issue: 99999` → marcata rotta |
| 9 | Aggiornare `feature-registry.md`, `CONTEXT_INDEX.md`, e i puntatori | `check-docs-links.py` verde |

I passi 1–3 e il **7** sono lavoro Python nel repository, misurabile, e **non aspettano la pagina**: dopo
D-C il `My Editor Queue` è una sezione derivata della EditorMap, utile anche se il Control Center non venisse
mai scritto. I passi 4–6 e 8 sono la pagina. Il passo 9 è il debito che questo repository ha già pagato
quattro volte per essersi dimenticato di pagarlo.

**Issue GitHub**: la fonte (§20) ne propone quindici. Prima di aprirle vanno cercate le esistenti — la
ricerca fatta il 2026-08-10 su `dashboard|control|registry|shortlist|editor` non trova nulla di aperto su
questo tema (#371 è chiusa e riguarda la EditorMap). Le quindici della fonte si comprimono nei nove passi
qui sopra: aprirle una a una replicherebbe questo piano su GitHub invece di eseguirlo.

## 12. Verdetto del panel

| Esperto | Rilievo | Severità |
|---|---|:--:|
| **Wiegers** | «Progresso derivato» e «warning visibili» erano requisiti non misurabili: ora sono R-2 e R-7, con un comando che li smentisce | CRITICO → risolto |
| **Fowler** | Leggere il `.yaml` e riderivare `status` nel browser duplica la regola di dominio. La sorgente è il `.json` | CRITICO → **D-1** |
| **Nygard** | Nessuna CI: l'artefatto generato può essere vecchio e la pagina non lo saprebbe. Serve un modo di *vedere* lo staleness | CRITICO → §5, banner |
| **Newman** | Il contratto dati copre le feature e non le altre quattro viste: la pagina finirebbe a parsare markdown generato | MAGGIORE → **D-2** |
| **Cockburn** | Il goal non era dichiarato, quindi non era falsificabile. Ora c'è, con il test di non-ridondanza | MAGGIORE → §3 |
| **Adzic** | Nessun esempio concreto: aggiunti i criteri osservabili in R-6, R-7, R-11 e nel piano | MEDIO → §7 |
| **Crispin** | I 13 test di §21 testavano un parser YAML che non va scritto: riscritti sul contratto reale | MEDIO → passo 8 |
| **Wiegers** | `editor_tasks.status: TODO` è uno stato dichiarato a mano — il difetto che ha ucciso la vista precedente | CRITICO → §8, non introdurre |

**Punteggio della fonte come specifica**: chiarezza 8/10 (prosa esplicita, esempi UI concreti) · completezza
6/10 (nessun contratto dati, nessuna gestione dello staleness) · testabilità 4/10 (i requisiti non erano
misurabili) · coerenza col repository 5/10 (§9 contraddice una decisione presa il giorno prima).

Il suo valore reale non è lo schema: è **§23**, la frase che il resto del documento serve a proteggere —
*«il dashboard non sostituisce il Feature Registry, è la sua UI web»*. Questa spec la traduce in R-1, che è
un requisito che si può far fallire.
