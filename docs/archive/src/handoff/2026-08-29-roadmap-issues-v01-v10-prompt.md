# PROMPT PER CLAUDE CODE — REFACTORTACTICS
## Riconciliazione issue/epic, roadmap v0.1→v1.0 e focus operativo v0.1

> ## 📸 `HISTORICAL` — SORGENTE CONSUMATO, NON NORMATIVO
>
> Work order arrivato in radice come `Prompt_Claude_RefactorTactics_Roadmap_Issues_v0.1_v1.0.md`, **untracked**,
> e consumato il **2026-08-29** dal referto
> [`../../../roadmap/plans/roadmap-issues-v01-v10-spec-panel-2026-08-29.md`](../../../roadmap/plans/roadmap-issues-v01-v10-spec-panel-2026-08-29.md).
>
> **Non è una fonte, e tre delle sue premesse operative erano già scadute quando è stato scritto**: `E48` è
> un'epic v0.1 viva (#1408) e non un nome libero, `#1657` è chiusa, e la vista v0.1→v1.0 che ordina di
> produrre esiste dal 2026-08-28. Conservato per la **provenienza**. Ciò che di normativo sopravvive sta
> nel §6 del referto.

Sei incaricato di lavorare direttamente sul repository **RefactorTactics** e di riallineare **GitHub Issues, epic, roadmap e focus operativo della release v0.1** senza creare una seconda tassonomia o duplicare lavoro già esistente.

Repository di riferimento:

`DegrassiAaron/refactor-tactics-main`

Lavora come **technical product owner + Unreal gameplay/network engineer + maintainer del repository**.

L'obiettivo NON è inventare una roadmap nuova: la roadmap canonica esiste già. Devi **misurare lo stato reale**, riconciliare ciò che è già aperto/chiuso, aggiornare i documenti owner e creare **solo le issue realmente mancanti**.

---

# 1. OBIETTIVO

Alla fine del lavoro devono essere vere contemporaneamente queste quattro condizioni:

1. esiste una vista coerente e aggiornata della roadmap **v0.1 → v1.0**;
2. la **v0.1 ha un critical path molto chiaro**, centrato sulla consegna del vertical slice;
3. GitHub non contiene issue duplicate per lavoro già coperto da epic/checkpoint esistenti;
4. il **Tactical Designer** rimane tooling trasversale e NON diventa una nuova release o una epic `E48`.

Il focus della v0.1 deve essere sintetizzabile così:

> **Complete Match → Understandable → Deterministic → Recorded → Packaged**

La release v0.1 è chiusa quando RefactorTactics può essere avviato, giocato/osservato in una vera partita 2v2 completa contro bot, compreso dal giocatore, risolto deterministicamente, registrato/riprodotto e avviato da build packaged senza dipendere dall'Editor.

---

# 2. REGOLE NON NEGOZIABILI

## 2.1 Non creare una nuova roadmap parallela

Prima di modificare qualsiasi cosa, considera owner canonici almeno:

- `docs/roadmap/roadmap-v0.1.md`
- `docs/roadmap/roadmap-post-v0.1.md`
- `docs/roadmap/roadmap-v0.1-v1.0.md`
- `docs/roadmap/roadmap-main-v0.1.md`
- `docs/roadmap/roadmap-checkpoint.md`
- `docs/roadmap/v0.1-definition-of-done.md`
- `docs/product/piano-canonico-mvp.md`
- `docs/decisions/RT_PDR_00_Decision_Log.md`

Verifica i path reali prima di usarli.

Se due documenti divergono:
- prevale il documento owner;
- registra e correggi la divergenza;
- non creare un terzo documento che diventi un altro owner.

## 2.2 Misura prima di decidere

NON fidarti dello stato descritto in questo prompt.

Prima di creare/modificare issue:

- `git fetch --prune`;
- misura `HEAD`;
- verifica branch corrente;
- controlla le issue direttamente lato GitHub;
- controlla le sub-issue;
- controlla milestone;
- controlla open/closed;
- verifica il codice e i test quando una issue dichiara qualcosa di implementato;
- verifica le decisioni più recenti.

Le issue citate sotto sono **puntatori iniziali**, non verità congelate.

## 2.3 Non duplicare lavoro

Prima di creare una issue cerca:

- titolo;
- concetto;
- Stable ID;
- sistema owner;
- epic padre;
- checkpoint;
- decision log;
- issue chiuse semanticamente equivalenti.

Una issue chiusa non significa automaticamente che il problema è risolto per sempre: verifica il codice.  
Ma NON riaprire o duplicare senza evidenza misurata.

## 2.4 Una feature ha un owner

Non creare issue generiche tipo:

- "migliorare il Tactical Designer";
- "completare la UI";
- "sistemare il gameplay";
- "fare il replay";

Ogni issue deve avere:

- problema concreto;
- owner;
- scope;
- out-of-scope;
- acceptance criteria misurabili;
- dipendenze;
- test/evidenza;
- relazione con epic/checkpoint/release.

## 2.5 Nessun cambio architetturale gratuito

Mantieni gli invarianti di progetto:

- simulatore autorevole;
- stessa snapshot + stesso seed ⇒ stesso risultato;
- TurnLog canonico;
- client/UI/editor non decidono esiti;
- nessuna duplicazione della logica runtime nell'Editor;
- dati canonici e Stable ID;
- niente hard-code sul 2v2 dove il dato appartiene al formato;
- privacy intenti e isolamento dell'autorità preservati anche se la v0.1 è offline.

---

# 3. ROADMAP CANONICA DA PRESERVARE

La struttura da preservare, salvo evidenza contraria nel repository, è:

| Release | Obiettivo |
|---|---|
| **v0.1** | Vertical slice 2v2 offline vs bot |
| **v0.2** | Standard 3v3, struttura e finestre |
| **v0.3** | Informazione / conoscenza parziale |
| **v0.4** | Operations, mappe grandi, 4v4+ |
| **v0.5** | Online Foundation |
| **v0.6** | Ability Runtime |
| **v0.7** | Dedicated / Competitive Alpha |
| **v0.8** | Beta / Balance / batch simulation |
| **v0.9** | Release Candidate / feature freeze |
| **v1.0** | Production Launch |

La traiettoria concettuale è:

```text
v0.1  PROVARE IL GIOCO
      ↓
v0.2  TROVARE IL FORMATO
      ↓
v0.3  AGGIUNGERE L'INFORMAZIONE
      ↓
v0.4  SCALARE IL CAMPO
      ↓
v0.5  ATTRAVERSARE LA RETE
      ↓
v0.6  INDUSTRIALIZZARE LE ABILITÀ
      ↓
v0.7  TOGLIERE L'AUTORITÀ AL CLIENT
      ↓
v0.8  MISURARE
      ↓
v0.9  SMETTERE DI AGGIUNGERE
      ↓
v1.0  SPEDIRE
```

La v1.0 NON deve essere trasformata in una release piena di nuove feature.

La v1.0 è un **gate di produzione**.

Controlla in particolare l'epic:

- `#778` — E45 / v1.0 Launch

La condizione finale deve rimanere concettualmente:

> una partita competitiva completa può essere trovata, giocata, risolta, spiegata, registrata e riprodotta su infrastruttura di produzione, senza replay divergence, intent leak o dipendenze dall'Editor.

---

# 4. AUDIT OBBLIGATORIO PRIMA DI SCRIVERE

Produci prima una tabella temporanea:

| ID | Tipo | Titolo | Stato GitHub | Release | Epic padre | Codice presente? | Test presenti? | Azione |
|---|---|---|---|---|---|---|---|---|

Controlla almeno:

- `#14` — epic master v0.1
- `#26` — E12 determinismo / QA / release
- `#1105` — Tactical Designer
- `#778` — E45 v1.0
- `#1657` — divergenza asset `L_DevSandbox`
- `#166` — CP14.6 / reaction window
- `#613` — HUD
- `#622`
- `#695`
- `#711`
- `#1186`
- `#1625`
- `#1626`
- `#1627`
- `#1628`
- `#1629`
- `#1630`
- `#1631`
- `#1114`
- `#1115`
- `#1116`
- `#1117`

Cerca inoltre tutte le epic `E1...E47` e confronta:

- issue;
- roadmap;
- sub-issue;
- milestone;
- stato reale nel repository.

NON assumere che il numero di epic chiuse scritto nei documenti sia corretto: ricalcolalo.

---

# 5. FOCUS OPERATIVO DELLA v0.1

La v0.1 deve smettere di comportarsi come una collezione di sistemi separati.

Il test principale deve diventare il **Complete Match**.

## F0 — Freeze del core

Da questo punto, nessuna nuova meccanica entra nel critical path salvo blocker.

Sono già sistemi maturi o sostanzialmente esistenti:

- hex;
- pathfinding;
- movement;
- action queue;
- reactions;
- terrain;
- cover;
- roster base;
- facing;
- predictive action;
- determinismo;
- TurnLog;
- Scenario Harness.

Se una nuova proposta non serve direttamente alla partita v0.1:
- non eliminarla necessariamente;
- spostala fuori dal critical path;
- marca `post-v0.1`, P3 o backlog coerentemente con la tassonomia esistente.

---

# 6. F1 — COMPLETE MATCH

Questo è il focus P0.

Il gioco deve completare:

```text
Main Menu
→ Play
→ Match
→ Planning
→ Ready
→ Resolution
→ nuovi turni
→ vittoria/sconfitta
→ Result
→ Play Again oppure Main Menu
```

Il Complete Match deve includere almeno:

- 2v2;
- 4 eroi;
- bot;
- movimento;
- combattimento;
- almeno una reaction significativa;
- terreno;
- objective;
- fine partita;
- TurnLog leggibile;
- replay/playback sufficiente a capire l'esito.

Riconcilia e collega a questo obiettivo le epic/checkpoint esistenti invece di creare un mega-sistema nuovo.

Controlla in particolare:

- E10;
- E11;
- E12;
- E14 baseline;
- E15;
- E21;
- E46;
- E47.

Se esistono issue separate che coprono già i pezzi, il Complete Match deve diventare **integration/gate**, non una duplicazione delle implementazioni.

---

# 7. F2 — PRESENTATION MINIMUM VIABLE

La v0.1 NON richiede art final.

Richiede abbastanza presentazione perché un tester capisca:

- chi è chi;
- squadra;
- HP/status essenziali;
- cella selezionata;
- path;
- intent;
- target/AoE;
- certainty:
  - Confirmed;
  - Predicted;
  - Uncertain;
- reaction `FIRE/HOLD`;
- objective;
- combat log;
- risultato.

Taglia dal critical path:

- polish grafico non necessario;
- FX finali;
- set icone esteso;
- animazioni finali;
- presentation non richiesta dal golden scenario.

Ogni elemento visuale richiesto dalla v0.1 deve essere collegato a:
- una decisione di gameplay;
- un reason code;
- un'informazione necessaria al giocatore;
- oppure un acceptance test.

---

# 8. F3 — REACTION BASELINE

Per la v0.1 deve funzionare il percorso minimo:

```text
Evento
↓
Reaction Opportunity
↓
FIRE / HOLD
↓
Resolver
↓
TurnLog
↓
Playback/UI
```

Non trasformare E14 in uno stack arbitrario.

Mantieni il principio:

- una finestra delimitata;
- niente nesting arbitrario;
- una risposta non apre automaticamente altre finestre.

Le estensioni P3 non devono bloccare la release se la baseline è verificata.

---

# 9. F4 — OBJECTIVE + GOLDEN SHOWCASE

Definisci o aggiorna uno scenario golden che dimostri il gioco.

Deve essere un **consumer delle regole**, non possedere eccezioni speciali.

Idealmente deve esercitare più sistemi possibili:

- movimento;
- quota;
- LOS;
- cover;
- acqua;
- elettricità;
- reaction;
- objective;
- KO;
- TurnLog;
- determinismo.

La domanda da poter rispondere è:

> Se questo scenario gira correttamente, RefactorTactics v0.1 dimostra davvero la propria identità?

Collega il golden scenario al gate di release.

---

# 10. F5 — RELEASE QA

La pipeline finale deve essere:

```text
Automation
↓
Complete Match
↓
Golden Scenario
↓
PIE human validation
↓
Development Packaged
↓
Shipping Packaged
↓
Replay / StateHash / LogHash comparison
↓
v0.1 DONE
```

Verifica `v0.1-definition-of-done.md`.

Non considerare verde un gate solo perché "sembra funzionare".

Ogni gate deve avere evidenza.

Per i KPI:
- se la regola del repository dice che basta misurare, non trasformare un valore fuori budget in blocco automatico;
- ma un KPI mancante resta un gate non soddisfatto.

---

# 11. ROADMAP MAIN v0.1

Mantieni e aggiorna la struttura a tre lane se ancora canonica:

## DIR-A — MAIN / EDITOR / INTEGRATION

Responsabilità:

- HUD;
- frontend;
- `L_DevSandbox`;
- complete match integration;
- ghost/warnings/certainty;
- reaction UI;
- board readability;
- objective presentation;
- Result / Run Again;
- PIE;
- packaged visual acceptance.

## DIR-B — CORE / GAMEPLAY

Responsabilità:

- reaction query;
- reason codes;
- read-only preview;
- replay/decision verification;
- objective runtime;
- TurnLog parity;
- determinismo;
- bugfix core;
- feature freeze.

## DIR-C — QA / SCENARIO / BOT

Responsabilità:

- complete match scenario;
- bot traces;
- reaction scenarios;
- fairness;
- objective scenario;
- environment;
- golden;
- replay repeat;
- packaged handoff.

IMPORTANTE:

Una lane è un **write-set**, non necessariamente una cartella.

Non introdurre worktree o parallelismi che invalidino:
- suite;
- Unreal process mutex;
- binari `.uasset`;
- `.umap`.

Un `.umap` viene modificato da una lane per volta.

---

# 12. TACTICAL DESIGNER

Epic padre:

- `#1105` — Tactical Designer

Il Tactical Designer resta:

`out_of_release_scope`

NON creare:

- `E48`;
- una milestone di gioco dedicata;
- una seconda roadmap di release.

Il suo obiettivo è accelerare sviluppo e validazione.

Mantieni la separazione:

```text
ROADMAP GAME
v0.1 → v0.2 → ... → v1.0

        ↑
        │ supportata da

TACTICAL DESIGNER
TD 0.1 → TD 0.2 → ... → TD 1.0
```

`TD 0.7` NON significa `RefactorTactics v0.7`.

---

# 13. TACTICAL DESIGNER — RIUSO OBBLIGATORIO

Prima di aprire nuove issue verifica cosa è già stato consegnato.

Sono stati almeno pianificati/consegnati:

- Scenario Composer;
- `URTScenarioAuthoring`;
- `FRTScenarioDraft`;
- Scenario Harness;
- Result;
- TurnLog;
- replay ViewModel;
- replay seek.

Controlla lo stato reale di:

- `#1114`
- `#1115`
- `#1116`
- `#1117`

e della Trial:

- `#1625` — playback visuale;
- `#1626` — intent combattimento;
- `#1627` — multi-turn;
- `#1628` — FIRE/HOLD;
- `#1629` — status iniziali;
- `#1630` — State Diff;
- `#711` — runtime probes movimento.

Non ricreare questi sistemi.

---

# 14. NUOVO ACTION LAB / LAUNCHER

Valuta se mancano davvero issue per questo lavoro.

Se e solo se non esistono già owner equivalenti, crea sotto `#1105` issue piccole e mirate.

## Possibile issue A — Tactical Designer Launcher Modes

Scopo:

```text
Open Tactical Designer
        ↓
      MODE
     /    \
Scenario  Action Lab
```

### Scenario

```text
Map
Format
Scenario
→ Start
```

### Action Lab

```text
Map
Hero
Action
Target / Context
→ Start
```

Acceptance criteria:

- launcher unico;
- nessuna duplicazione del runtime;
- scelta esplicita della modalità;
- ogni modalità usa dati canonici;
- errori di caricamento esposti chiaramente;
- test editor/automation dove ragionevole.

## Possibile issue B — Action Lab canonical execution

Flusso:

```text
Map + Hero + Action + Context
          ↓
   FRTScenarioDraft
          ↓
 URTScenarioAuthoring
          ↓
 Scenario Harness
          ↓
 Resolver
          ↓
 TurnLog
```

`PLAY` deve fare:

```text
Reset initial state
↓
Fixed seed
↓
Run
↓
Playback
↓
STOP
```

Un secondo `PLAY` deve ripartire dallo stesso stato iniziale.

Acceptance criteria minimi:

- stesso seed;
- stesso draft;
- stesso risultato;
- stesso StateHash;
- stesso comportamento del Scenario Harness;
- nessun branch di simulazione dedicato al tool;
- il tool chiama la facade/runtime canonici.

NON implementare qui:
- Skill Workbench completo;
- visual scripting;
- mass simulation;
- balance dashboard;
- bot tournament;
- promotion workflow;
- modding;
- networking tool;
- export video.

---

# 15. DEV SANDBOX — DECISIONE DA NON NASCONDERE

Controlla `#1657`.

È stata rilevata una possibile divergenza fra:

- `DA_Format_Scratch`;
- `_Scratch/DA_HexMap_Scratch_Basin`;
- riferimento effettivo di `L_DevSandbox`.

NON assumere quale sia corretto.

Devi:

1. verificare lo stato attuale;
2. capire se esiste già una issue che prende la decisione;
3. se manca, creare una issue decisionale minimale sotto l'owner appropriato;
4. NON cancellare asset o modificare `.umap` senza criterio verificato;
5. evitare che Launcher/Action Lab dipendano implicitamente da un asset volatile senza dichiararlo.

Questa decisione può essere blocker del Tactical Designer.

NON deve diventare automaticamente blocker della release v0.1 se il gioco packaged usa un percorso canonico diverso.

---

# 16. COME CREARE LE ISSUE

Per ogni issue nuova usa questa struttura:

```markdown
# Why

Problema misurato nel repository.

# Evidence

File, codice, test, asset, issue o output che dimostrano il problema.

# Owner

Epic / checkpoint / release / tooling parent.

# Scope

Cosa deve cambiare.

# Out of scope

Cosa NON deve essere fatto in questa issue.

# Acceptance criteria

- [ ] criterio osservabile
- [ ] test automatico
- [ ] verifica editor se necessaria
- [ ] nessuna divergenza runtime/editor
- [ ] documentazione owner aggiornata

# Tests

Nomi o famiglie di test da creare/modificare.

# Dependencies

Issue realmente bloccanti.

# Related

Epic, ADR, decision log, roadmap.
```

Evita titoli generici.

Preferisci:

> `[TD] Launcher: selezione Scenario / Action Lab senza biforcare il runtime`

a:

> `Migliorare Tactical Designer`

---

# 17. COSA TAGLIARE DAL CRITICAL PATH v0.1

Se non sono necessari al Complete Match, togli dal critical path:

- polish;
- art finale;
- reaction avanzate;
- balance fine;
- 4v4 come gate qualitativo;
- Skill Workbench;
- editor avanzato;
- debugger LOS completo;
- mass simulation;
- networking;
- GAS;
- progression;
- modding.

NON serve necessariamente chiudere/cancellare le issue.

Puoi:

- ridurre priorità;
- segnare post-v0.1;
- rimuovere dipendenze false;
- dichiarare "non blocker";
- spostare milestone se la tassonomia esistente lo prevede.

Mai farlo in silenzio: registra la ragione.

---

# 18. AGGIORNAMENTO ROADMAP

Aggiorna i documenti owner, non creare un altro `roadmap-new.md`.

La vista v0.1→v1.0 deve mostrare chiaramente:

```text
v0.1 Vertical Slice
    ↓
v0.2 3v3
    ↓
v0.3 Information
    ↓
v0.4 Operations
    ↓
v0.5 Online
    ↓
v0.6 Ability Runtime
    ↓
v0.7 Dedicated
    ↓
v0.8 Beta / Balance
    ↓
v0.9 Feature Freeze
    ↓
v1.0 Production Gate
```

Per la v0.1 aggiungi una vista operativa evidente:

```text
CORE FREEZE
↓
COMPLETE MATCH
↓
MINIMUM VIABLE PRESENTATION
↓
REACTION BASELINE
↓
OBJECTIVE + GOLDEN SHOWCASE
↓
PIE + PACKAGED + REPLAY EVIDENCE
↓
v0.1
```

Ma NON copiare in più documenti lo stato dettagliato delle epic.

Ogni stato deve avere un solo owner.

Le altre pagine devono linkarlo.

---

# 19. AGGIORNAMENTO EPIC MASTER v0.1

Verifica `#14`.

Deve permettere a una persona di capire immediatamente:

- quali epic appartengono alla v0.1;
- quante sono realmente chiuse;
- cosa blocca davvero la release;
- quali epic restano aperte ma non sono critical path;
- dove si legge il gate;
- dove si legge l'esecuzione.

Non mantenere conteggi manuali falsi.

Se possibile, sostituisci conteggi fragili con:
- query;
- script;
- comando riproducibile;
- oppure formula derivata dagli owner.

---

# 20. AGGIORNAMENTO E12 / RELEASE GATE

Verifica `#26`.

Deve riflettere lo stato reale dei gate.

Riconcilia:

- G1...G14;
- gate ritirati;
- suite Editor;
- suite packaged;
- deterministic repeat;
- PIE;
- complete match;
- KPI;
- no-Editor;
- documentation.

Niente numeri vecchi mantenuti per inerzia.

Se una misura è datata, scrivi la data.

---

# 21. CHECK DELLA v1.0

Verifica `#778` e le issue figlie del production gate.

Non aprire feature premature.

Controlla che la v1.0 resti centrata su:

- dedicated production deployment;
- matchmaking rollout;
- observability;
- security/privacy audit;
- replay audit;
- performance certification;
- content validation;
- packaged smoke;
- rollback.

Il Tactical Designer NON diventa una dipendenza runtime della v1.0.

Gli strumenti possono aiutare il processo, ma la build di gioco deve rimanere indipendente dall'Editor.

---

# 22. TEST E VALIDAZIONE

Dopo le modifiche esegui ciò che è applicabile:

- build;
- automation suite;
- script roadmap/documentation;
- issue consistency check;
- asset references check;
- eventuali test Node/Python/tooling;
- verifica che nessun `.uasset` o `.umap` sia cambiato accidentalmente;
- verifica link/document owner;
- verifica che nuove issue non siano duplicate.

Se un test non può essere eseguito:
- scrivilo esplicitamente;
- NON dichiararlo verde.

---

# 23. OUTPUT CHE VOGLIO DA TE

Alla fine forniscimi questo report.

## A. Stato misurato

```text
HEAD:
Branch:
origin/main:
Issue audit date:
```

Poi:

| Area | Stato prima | Stato dopo |
|---|---|---|
| Roadmap | | |
| v0.1 focus | | |
| GitHub epic | | |
| Tactical Designer | | |
| v1.0 | | |

## B. Issue create

Per ogni issue:

```text
#ID — titolo
Parent:
Priority:
Reason:
Blocks v0.1: YES/NO
```

## C. Issue aggiornate

```text
#ID — modifica effettuata
```

## D. Issue NON create perché duplicate

Molto importante.

```text
Proposta:
Coperta da:
Motivo:
```

## E. Roadmap modificate

File + sezione + motivo.

## F. Critical path v0.1 finale

Mostralo in massimo 10 righe.

## G. Defer

Elenco delle cose esplicitamente tolte dal critical path.

## H. Test

```text
PASS:
FAIL:
NOT RUN:
```

## I. Commit

Proponi commit piccoli, ad esempio:

```text
docs(roadmap): reconcile v0.1 critical path with live issues
chore(github): align v0.1 epic and release gate
docs(tactical-designer): add launcher and action-lab ownership
```

Non mettere modifiche non correlate nello stesso commit.

---

# 24. CRITERIO DI SUCCESSO

Il lavoro è riuscito se, alla fine, una nuova sessione può aprire il repository e capire in meno di cinque minuti:

1. cosa serve per chiudere la v0.1;
2. quali issue deve eseguire adesso;
3. quali issue sono tooling e non release;
4. qual è il percorso fino alla v1.0;
5. dove vive l'autorità di ogni informazione;
6. che cosa NON deve essere implementato adesso.

La priorità assoluta è evitare una v0.1 infinita.

Quando hai un dubbio fra:

- aggiungere una feature;
- oppure chiudere l'integrazione del Complete Match;

scegli il **Complete Match**, salvo che la feature sia un blocker dimostrato.
