# RefactorTactics — Consolidamento operativo a 4 processi paralleli
## Roadmap sincronizzata fino a v1.0 + ID condivisi + lease `.uasset/.umap`
### Handoff operativo per Claude Code

> **Tipo:** handoff da consumare e consolidare nel repository, non nuova fonte canonica.
>
> **Snapshot di riferimento:** `main` successivo al consolidamento roadmap fino a v1.0 (2026-08-13).
>
> **Precedenza:** stato live di `origin/main`, GitHub e documenti owner correnti > questo handoff.
>
> Prima di applicare qualsiasi cosa, rileggi `AGENTS.md`, `CLAUDE.md`, le roadmap correnti, il Feature Registry,
> `workflow-parallel-claude.md`, gli issue live e i file realmente presenti. Non riapplicare lavoro già assorbito.

---

# 1. Decisioni da consolidare

1. RefactorTactics deve poter avanzare con **quattro processi paralleli reali**.
2. Tre processi sono sessioni Claude su **worktree + branch distinti**.
3. Il quarto processo è **l'autore umano davanti a Unreal Editor**.
4. Il quarto processo prepara/importa/configura asset, mappe e Blueprint e svolge le verifiche manuali Editor/PIE.
5. `.uasset` e `.umap` sono **human-first per default**, non human-only: Claude può modificarli con una lease binaria esclusiva.
6. Nessun processo assegna a mano un nuovo ID globale progressivo osservando “l'ultimo numero”.
7. Il meccanismo esistente `scripts/rt_shared_id.py` resta l'owner per gli ID già supportati, in particolare `D-nnn`.
8. I quattro processi devono comparire come **viste dello stesso grafo di progetto**, integrate con Roadmap,
   Milestone Map, Feature Map, Scenario Map, Editor Map, Project Graph / Control Center e GitHub.
9. Non creare un nuovo asse di milestone o gate. Restano le nomenclature correnti:
   release `v0.1..v1.0`, milestone `M*`, epic `E*`, checkpoint `CP x.y`, gate ufficiali v0.1 `G1..G15`,
   Decision Log `D-nnn`.
10. Se una track non ha un task sicuro e indipendente, può essere **IDLE**. Non inventare lavoro per saturare quattro processi.

---

# 2. Stato corrente da rispettare

La roadmap corrente arriva già a:

| Release | Tema | Epic principali |
|---|---|---|
| v0.1 | Vertical Slice | E1–E21 |
| v0.2 | Struttura e finestre | E22–E26, E35, E36, E38, E39 |
| v0.3 | Informazione | E27–E29, E33 |
| v0.4 | Operations | E30–E32, E34, E37 |
| v0.5 | Online Foundation | E40 |
| v0.6 | Ability Runtime | E41 |
| v0.7 | Competitive Alpha | E42 |
| v0.8 | Beta / Balance | E43 |
| v0.9 | Release Candidate | E44 |
| v1.0 | Launch | E45 |

Non introdurre una seconda roadmap F0/F1/F2 o nuovi gate paralleli.

Il repository possiede già:
- grafo hex multilivello, A*, LOS e map editor;
- snapshot, resolver, TurnLog e Scenario Harness;
- replay recorder/manifest/seek;
- Feature Registry;
- cinque shortlist generate;
- EditorMap generata;
- Project Graph / Control Center;
- `rt_shared_id.py`;
- roadmap post-v0.1 fino a v1.0.

---

# 3. Superare il vecchio modello a 7 lane

Sono presenti:

```text
docs/roadmap/plans/roadmap-lane-index.md
docs/roadmap/plans/roadmap_lane_1.md
...
docs/roadmap/plans/roadmap_lane_7.md
```

Questi file dichiarano già di essere snapshot, non ownership, branch, milestone o fonte di stato.

La nuova decisione cambia la premessa: i nuovi quattro tracciati rappresentano **processi eseguibili in parallelo**.

Claude deve:
1. cercare tutti i riferimenti alle vecchie lane;
2. non cancellarle alla cieca;
3. marcarle `SUPERSEDED/HISTORICAL` o archiviarle secondo la convenzione corrente;
4. aggiornare i link;
5. mantenere la provenienza storica;
6. sostituire la vista operativa con quattro tracciati derivati/generati.

---

# 4. Processo SPATIAL / WORLD — Claude

Missione: lo **spazio logico come dato e query**.

Include:
- `FRTCellId`, celle, layer;
- MapState;
- graph / edges / transitions;
- pathfinding;
- LOS geometry;
- trajectory geometry;
- targeting spatial queries;
- GraphRevision/cache;
- standability e clearance;
- cover geometry;
- doors / bridges / tunnels / portals;
- surface topology;
- geometry bake contract;
- spatial occupancy;
- map/spatial performance.

Non decide:
- sequencing del turno;
- danni/status;
- reaction policy;
- HUD/replay presentation;
- contenuti binari per default.

Assorbe principalmente la vecchia lane Spatial/Map e le parti spatial di Editor/Map, E23, E24, E27, E30, E39.

---

# 5. Processo SIMULATION / RULES / SERVER / AI — Claude

Missione: **ciò che il gioco decide**.

Include:
- Planning logical model;
- Intent;
- Snapshot;
- TurnManager;
- resolver;
- actions / ability outcome;
- movement resolution;
- reactions / opportunity / commit;
- status;
- environment resolution;
- objectives;
- combat;
- bot / AI;
- Team Knowledge rules;
- authoritative networking/server rules;
- dedicated gameplay lifecycle;
- TurnLog production;
- StateHash / LogHash;
- determinismo e reason code.

Regola fondamentale:

> Simulation produce il TurnLog canonico. Replay lo consuma: non nasce un secondo simulatore.

Assorbe la vecchia Simulation/Turn, il gameplay logico dei Character, bot e authority/server.

---

# 6. Processo CLIENT / REPLAY / TOOLING — Claude

Missione: **come il risultato viene visto, riprodotto e diagnosticato**.

Include:
- HUD C++;
- input/camera/selection;
- view model;
- planning presentation;
- ghosts/previews/certainty;
- combat log view;
- playback;
- replay recorder/reader/player/seek;
- replay audit/verifier tooling secondo ADR esistenti;
- replay browser;
- network client UX;
- Editor module C++;
- map editor tooling C++;
- validators/inspectors;
- scenario launcher;
- debug panels;
- Control Center;
- presentation-side character code;
- VFX/SFX hooks e presentation adapters.

Non decide esiti competitivi.

Assorbe Client/UX, Replay/Audit, il codice della vecchia Editor/Tooling, parte presentation Character e i tool della vecchia VFX/AssetFab.

---

# 7. Processo CONTENT / EDITOR — autore umano

Missione: il lavoro **davanti a Unreal Editor**.

Include:
- Content Browser;
- Migrate/import dal vault;
- Blueprint;
- Data Asset;
- UMG asset;
- Material/MI/Texture;
- Static/Skeletal Mesh;
- animation/retarget;
- Niagara;
- audio;
- map authoring;
- level placement;
- lighting;
- visual scenario setup;
- asset references;
- Fix Up Redirectors;
- manual PIE;
- Editor Sessions;
- visual QA;
- packaged visual smoke quando richiesto.

Owner operativi già esistenti da non duplicare:
```text
docs/roadmap/editor-sessions.yaml
docs/roadmap/editormap.shortlist.md
docs/technical/test-manuali-pie.md
docs/technical/convenzioni-contenuti-ue.md
vault README / Fab pipeline
```

Assorbe il lavoro umano della vecchia Editor/Tooling, gli asset Character e VFX/AssetFab.

---

# 8. Non basta il dominio: serve un write-set per batch

Due processi diversi possono comunque dover toccare lo stesso file.

Prima di avviare un batch parallelo dichiarare i file scrivibili.

Creare, salvo equivalente già presente:

```text
docs/roadmap/parallel-batch.yaml
```

Schema minimo concettuale:

```yaml
schema_version: 1

batch:
  base_sha: "<origin/main sha>"
  created_at: "YYYY-MM-DD"

tracks:
  spatial:
    issue: 41
    writable: []

  simulation:
    issue: 583
    writable: []

  client_tools:
    issue: 77
    writable: []

  content_editor:
    issue: 38
    writable: []

integration_only: []

binary_leases: []
```

Questo file descrive il **batch**, non lo stato delle feature.

---

# 9. Regola file: non assegnato = STOP

Prima di modificare un file:

1. verifica il `parallel-batch.yaml`;
2. il path deve appartenere al `writable` della tua track;
3. altrimenti fermati;
4. registra una richiesta di riallocazione/integrazione;
5. non fare “solo questa piccola fix”.

Vale per C++, docs, scripts, Config, `.uasset`, `.umap`, test e generated outputs.

---

# 10. File Integration-Only

Non assegnare ownership permanente arbitraria. Per batch, rendere `integration_only` ciò che potrebbe essere conteso.

Default da valutare:

```text
AGENTS.md
CLAUDE.md
docs/decisions/RT_PDR_00_Decision_Log.md
docs/OPEN_DECISIONS.md
docs/DOC_CONFLICT_MATRIX.md
docs/roadmap/roadmap-checkpoint.md
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-post-v0.1.md
docs/roadmap/feature-registry.yaml
generated roadmap files
RefactorTactics.uproject
shared Build.cs
shared Config
```

Questi si aggiornano una volta in integrazione quando più branch potrebbero averne bisogno.

---

# 11. `.uasset` / `.umap`: human-first, non human-only

La regola è:

> Processo Content/Editor è holder predefinito dei binari Unreal.
> Claude può creare/modificare/rinominare/resalvare/migrare un binario solo con **Binary Asset Lease esclusiva**.

Questo evita di bloccare task che Claude può eseguire correttamente attraverso Unreal, ma impedisce due save concorrenti.

---

# 12. Binary Asset Lease

Aggiungere al batch:

```yaml
binary_leases:
  - key: "BINARY-GH620-MAP-GRAMMAR"
    holder: spatial
    issue: 620
    base_sha: "<sha>"
    operation: modify
    paths:
      - Content/RT/Maps/Dev/L_DevSandbox.umap
    verification:
      - load_in_editor
      - save_without_errors
      - run_validator
```

Regole:
- `key` semantica e legata a GitHub, MAI `LEASE-001`;
- holder unico;
- path esatti;
- una lease vale sul `base_sha`;
- se main modifica lo stesso asset, lease stale;
- `create`, `modify`, `rename`, `resave`, `migrate` sono permessi distinti;
- nuovo asset = destinazione prenotata;
- nessun secondo holder umano/Claude sullo stesso binario.

---

# 13. Nessun binary merge

Due versioni di `.uasset/.umap` non si “fondono”.

Se due modifiche sono entrambe richieste:

```text
scegli una base
→ apri Unreal
→ riapplica l'altra modifica
→ save
→ validator/test
```

Per `.umap`, prima verificare World Partition, External Actors, sublevel e Data Layer: se Unreal produce package correlati, anche quelli entrano nella lease.

---

# 14. Claude modifica asset solo tramite Unreal

Anche con lease:

Consentito:
- Unreal Editor;
- Content Browser;
- editor scripting approvato;
- commandlet/resave package supportato;
- tool del progetto.

Vietato:
- hex editor;
- binary patch;
- filesystem rename raw;
- spostare package senza redirector workflow.

Rename/move: Content Browser + Fix Up Redirectors + reference audit.

---

# 15. Preservare Vault/Fab e ignore policy

La pipeline esistente ha già deciso che molti asset Fab:
- vivono nel vault;
- restano fuori git;
- vengono migrati per dipendenza;
- possono essere presenti su disco senza essere versionati.

Non trasformare automaticamente `Content/` in un albero tutto tracciato/LFS.

Prima di modificare allowlist/ignore:
1. leggere `.gitignore`;
2. leggere `convenzioni-contenuti-ue.md`;
3. verificare il vault;
4. preservare la motivazione corrente.

---

# 16. Collisioni degli ID globali: problema generale

Race tipica:

```text
branch A vede D-101 -> crea D-102
branch B vede D-101 -> crea D-102
```

Lo stesso rischio può esistere per:
- `D-nnn`;
- nuovi `E-nn`;
- nuovi `CP x.y`;
- ADR progressivi;
- Map Format Version;
- Replay Format Version;
- TurnLog/schema version;
- altri namespace monotoni.

Mai risolvere con “guarda l'ultimo e fai +1”.

---

# 17. `rt_shared_id.py` è già la soluzione per D-nnn

Esistono:

```text
scripts/rt_shared_id.py
docs/technical/workflow-parallel-claude.md
```

e D-135.

Il tool:
- usa il git common dir;
- usa un lock reale;
- vede i worktree dello stesso clone;
- considera branch/ref;
- supporta `reserve`;
- supporta `check`;
- supporta `audit-refs`.

NON costruire un secondo allocatore se questo è estendibile.

---

# 18. Audit obbligatorio dei namespace monotoni

Produrre una tabella reale, per esempio:

| Namespace | Globale | Può collidere | Serializzato | Owner | Policy |
|---|---:|---:|---:|---|---|
| D-nnn | sì | sì | no | Decision Log | `rt_shared_id.py` |
| E-nn | sì | sì | no | release roadmap | audit |
| CP x.y | nell'epic | sì | no | epic | audit |
| ADR | sì | possibile | no | decisions | audit |
| map format | sì | sì | sì | map serialization | audit forte |
| replay format | sì | sì | sì | replay | audit forte |

Non assumere che tutti usino lo stesso algoritmo.

Ma nessun nuovo numero globale progressivo viene assegnato a mano in un branch parallelo.

---

# 19. Policy transitoria per namespace non supportati

Se un namespace non è ancora supportato atomicamente:

NON:
```text
"ultima epic E45 -> creo E46"
```

Usare temporaneamente l'identità già globale:
```text
GitHub #NNN
feature_id
nome semantico
```

Il numero canonico viene assegnato in integrazione oppure dopo l'estensione dell'allocatore.

Non rinumerare ID già canonici.

---

# 20. Più clone / più PC

Il lock locale non serializza clone diversi.

Prima del merge resta obbligatorio:

```bash
git fetch --prune origin
python scripts/rt_shared_id.py check
python scripts/rt_shared_id.py audit-refs
```

Una reservation remota è futura solo se il problema reale la rende necessaria.

---

# 21. Quattro tracciati, ma zero nuove fonti di stato

Creare quattro viste generate:

```text
docs/roadmap/track-spatial.shortlist.md
docs/roadmap/track-simulation.shortlist.md
docs/roadmap/track-client-tools.shortlist.md
docs/roadmap/track-content-editor.shortlist.md
```

e un indice:

```text
docs/roadmap/plans/parallel-track-index.md
```

Se la convenzione corrente suggerisce nomi diversi, adattarla.

Le quattro viste devono avere marker `GENERATA` e un `--check`.

---

# 22. Sorgente della classificazione

Preferire un owner piccolo e senza stato, salvo equivalente esistente:

```text
docs/roadmap/parallel-tracks.yaml
```

Può dire:
- feature -> track;
- issue/category -> track dove necessario;
- editor session -> content_editor.

NON deve contenere:
- status;
- done;
- progress;
- test count;
- gate outcome.

Prima di crearlo verificare se il Feature Registry supporta già elegantemente un campo non-state come `work_tracks`.

Scegliere **un solo owner** della classificazione.

---

# 23. Integrazione con Feature Map

Lo stato resta nel Feature Registry.

Le track consumano:
- feature_id;
- release;
- epic/milestone;
- dependencies;
- gates;
- derived status;
- issues;
- tests;
- scenarios.

Una feature può avere più track senza essere duplicata:

```yaml
work_tracks:
  - simulation
  - client_tools
```

---

# 24. Integrazione con Roadmap / Milestone Map

Le track sono un overlay delle release correnti.

Non creare:
```text
Milestone Spatial 01
Milestone Simulation 01
```

Esempio:

```text
E39 Spatial Transfer
SPATIAL       -> spatial resolver/topology
SIMULATION    -> TurnManager/triggers/rules
CLIENT        -> preview/replay
CONTENT       -> eventuali asset consumer
```

---

# 25. Integrazione con Scenario Map

La Scenario Map deve poter mostrare quali track sono coinvolte, derivandolo da feature/issue/capability/session.

Esempio:

```text
Spec.Movement.TeleportSkipsIntermediateCells
Spatial     E39.2
Simulation  E39.4
Client      replay representation
Content     N/A
```

Non duplicare lo stato dello scenario nei quattro file.

---

# 26. Integrazione con Editor Map

EditorMap resta l'owner della coda umana.

La track `content_editor` deve referenziare:
```text
U1
U7
U20
U21
...
```

Non copiare:
- steps;
- status;
- produces;
- done_when;
- PIE result.

Owner: `editor-sessions.yaml`.
Vista: `editormap.shortlist.md`.

---

# 27. Project Graph / Control Center

Se il contratto dati lo consente con un diff piccolo, aggiungere:
- filtro `track`;
- grouping per track;
- cross-track dependency;
- active batch;
- binary leases.

La pagina NON calcola stato. Dipinge JSON generato.

Se è troppo ampio, limitarsi a esporre i dati nel `project-graph.json`.

---

# 28. GitHub è già l'ID del task

Usare:
```text
#583
#77
#38
#41
```

Non introdurre:
```text
SP-01
SIM-01
CLIENT-01
CONTENT-01
```

Le track classificano; GitHub identifica.

---

# 29. Nessun nuovo Gate globale

Il parallelismo usa un **Integration Gate operativo**, ma non riceve un nuovo `Gxx`.

Identità batch non progressiva:

```text
BATCH-<base_sha>-gh41-gh583-gh77-gh38
```

Un batch chiude quando:
1. branch freeze;
2. ID check/audit;
3. write-set disgiunti;
4. lease binarie risolte;
5. merge;
6. canonical docs aggiornati una volta;
7. generated views rigenerate;
8. test/build richiesti verdi;
9. editor session/manual result registrati quando applicabili.

---

# 30. Roadmap 4 processi — v0.1

## Spatial
Verificare live:
- `#41` CP 3.3 KPI hex;
- regressioni/perf spatial necessarie;
- nessun nuovo scope spatial se E2/E8/E9 restano chiuse.

## Simulation
Verificare live:
- `#583` producer D-109, se il producer è logico/bot;
- `#512`;
- `#165`;
- `#159`;
- `#74 -> #75`;
- residui E7/reaction/status/objective.

## Client / Replay / Tooling
Verificare live:
- `#77 -> #78`;
- `#79 -> #80`;
- `#172 -> #173`;
- `#83 -> #84 -> #85`;
- `#625`;
- `#170 -> #171`;
- `#593`/E21 presentation code se il write-set è sicuro;
- residui editor-tooling.

## Content / Editor
Verificare live:
- `#38`;
- `#451`;
- `#82`;
- U7;
- U20;
- `#623` / U21;
- subset PIE `RELEASE-V01`.

La v0.1 chiude solo con i gate ufficiali già esistenti.

---

# 31. Roadmap — v0.2 «Struttura e finestre»

Epic correnti:
```text
E22 E23 E24 E25 E26 E35 E36 E38 E39
```

## Spatial
- E22 cover state/LOS boundary spatial support;
- E23 muri/porte/interaction graph/geometry bake/clearance;
- E24 Standard map;
- E39 transfer spatial primitive, destination validity, portal topology/cache.

## Simulation
- E22 runtime Cover Window;
- E24 ruleset 3v3;
- E26 Tactical Bot v1;
- E35 gameplay runtime roster 8;
- E36 status framework;
- E38 plan validation + movement/action compatibility;
- E39 transfer in real turn, triggers/perception semantics.

## Client / Replay / Tooling
- E25 complete Icon Language consumers;
- E38 planning reasons/UX;
- E39 no-fake-path preview + replay;
- E23/E24 authoring/debug tools.

## Content / Editor
- wall/door geometry;
- Standard map authoring/playtest;
- icon assets;
- roster 8 import/retarget/BP/Data Assets;
- content/map QA.

Release gate: usare quello già scritto per v0.2.

---

# 32. Roadmap — v0.3 «Informazione»

Epic:
```text
E27 E28 E29 E33
```

## Spatial
- acoustic propagation/occlusion;
- spatial support traps/tripwire;
- visibility geometry support.

## Simulation
- Team Knowledge;
- vista/udito/memoria;
- Expert Bot v2;
- Predictive advanced;
- Conditional Intent.

## Client / Replay / Tooling
- perception overlays;
- certainty/last-known UI;
- conditional/predictive UX;
- replay privacy e knowledge-change representation;
- diagnostics.

## Content / Editor
- audio/noise assets;
- stealth readability;
- environmental sound;
- perception cues;
- maps/scenarios per loss/reacquisition;
- manual perception playtests.

---

# 33. Roadmap — v0.4 «Operations»

Epic:
```text
E30 E31 E32 E34 E37
```

`E37` risulta già completata: non riaprirla.

## Spatial
- Operations large maps;
- large graph perf;
- logistics map support;
- 4v4 spatial stress.

## Simulation
- multiple objectives/logistics;
- 4v4 rules if validated;
- Character States/Configurations/Transformations.

## Client / Replay / Tooling
- large map UX;
- objective HUD;
- camera/readability;
- long-match replay/seek;
- state presentation adapters.

## Content / Editor
- Operations map authoring;
- objectives/lighting;
- 4v4 visual stress;
- state/form assets;
- long playtests.

---

# 34. Roadmap — v0.5 «Online Foundation» — E40

## Spatial
Solo se necessario:
- map/GraphRevision serialization compatibility;
- server/client spatial query parity.

## Simulation
Percorso critico canonico:
1. authoritative match lifecycle;
2. CanonicalIntentStore;
3. team-only preview relay;
4. ready/commit reliable + idempotent;
5. server-only resolution;
6. canary anti-leak;
7. packaged two-team scenario.

Preview 8–12 Hz unreliable/sequenced; commit reliable.

## Client / Replay / Tooling
- network client flow;
- ally preview reception;
- reaction round-trip UX;
- ack/error;
- network diagnostics;
- replay network match;
- privacy audit tooling.

## Content / Editor
- lobby/HUD assets se richiesti;
- packaged 2-team visual smoke/manual checks.

Gate E40 già canonico:
complete two-client match + zero intent leak + replay divergence 0.

---

# 35. Roadmap — v0.6 «Ability Runtime» — E41

## Spatial
Solo query/adapters realmente necessari; altrimenti `IDLE`.

## Simulation
- GAS bridge;
- ASC lifecycle;
- stable ActionId<->ability binding;
- resolver authority invariata;
- logical events -> GAS application;
- privacy/determinism regression.

## Client / Replay / Tooling
- GameplayCue/presentation bridge;
- ability debug;
- HUD;
- replay regression;
- audit no AbilityTask authority.

## Content / Editor
- GAS/ability assets;
- GameplayCue;
- VFX;
- animation/audio;
- editor configuration;
- tutte le modifiche binarie con lease.

Gate: replay v0.5 riproducibile e nessun AbilityTask decide un outcome.

---

# 36. Roadmap — v0.7 «Competitive Alpha» — E42

## Spatial
- server spatial perf/serialization soltanto se necessario.

## Simulation
- dedicated lifecycle;
- dedicated authority;
- reconnect;
- resync;
- team/party rules.

## Client / Replay / Tooling
- lobby;
- disconnect/reconnect UX;
- resync presentation;
- replay catch-up/audit;
- telemetry tools.

## Content / Editor
- cook validation;
- packaged client/server smoke;
- asset presence;
- zero Editor dependency.

Gate:
```text
launch → lobby → join/create → play → disconnect → reconnect → finish
```
su packaged dedicated.

---

# 37. Roadmap — v0.8 «Beta / Balance» — E43

## Spatial
- map corpus;
- representative spatial perf/map variants.

## Simulation
- batch runner sul resolver reale;
- bot competence schema;
- deterministic seeds;
- simulation corpus.

## Client / Replay / Tooling
- reports;
- replay diff;
- audit tools;
- Control Center balance view se utile;
- performance analysis.

## Content / Editor
- content/balance tuning;
- playtest;
- map tuning;
- editor validation.

Hard gate: crash/divergence/leak/invald state, non win-rate.

---

# 38. Roadmap — v0.9 «Release Candidate» — E44

Feature freeze.

## Spatial
- map format migration;
- perf hardening;
- map/content compatibility.

## Simulation
- rules freeze;
- security/abuse hardening;
- server robustness.

## Client / Replay / Tooling
- save/replay migration;
- compatibility diagnostics;
- ranked/rating UX;
- release diagnostics.

## Content / Editor
- content freeze;
- redirect cleanup;
- cook validation;
- final asset audit.

Gate: nessuna feature nuova + migration test + soak no crash/divergence.

---

# 39. Roadmap — v1.0 «Launch» — E45

La v1.0 è un production gate, non una feature release.

## Spatial
Certificare:
- production maps;
- query perf;
- cook/package compatibility.

## Simulation
Certificare:
- production dedicated;
- matchmaking authority integration;
- privacy/security;
- determinism;
- production lifecycle.

## Client / Replay / Tooling
Certificare:
- production matchmaking UX;
- observability/diagnostics;
- replay audit;
- rollback support tooling;
- production smoke.

## Content / Editor
Certificare:
- final content audit;
- all required assets cooked;
- no critical redirect/orphan;
- manual last-mile smoke;
- packaged content validation.

Gate finale canonico:
> Una partita competitiva completa può essere trovata, giocata, risolta, spiegata, registrata e riprodotta
> su infrastruttura di produzione, senza replay divergence, senza intent leak e senza dipendenze dall'Editor.

---

# 40. Processo IDLE è valido

Se non esiste lavoro disgiunto utile:

```text
SPATIAL = IDLE
```

Non creare task fittizi.

Il parallelismo minimizza wall-clock time, non massimizza branch count.

---

# 41. Primo batch: solo dopo verifica live

Candidati storicamente plausibili:

```text
SPATIAL        #41
SIMULATION     #583 o #512
CLIENT         #77 o #83
CONTENT        #38
```

Ma prima:

```bash
git fetch --prune origin
gh issue view 41
gh issue view 583
gh issue view 512
gh issue view 77
gh issue view 83
gh issue view 38
```

Poi calcolare i file reali toccati.

Se due task condividono un file, uno esce dal batch.

Esempio: se `#593` e un task Simulation toccano entrambi `RTUnit.*`, non girano insieme.

---

# 42. Algoritmo di selezione del batch

1. ordina backlog per release/P0/P1/dipendenze;
2. seleziona il task più utile;
3. stima/verifica `WritableSet`;
4. cerca il prossimo task con set disgiunto;
5. ripeti;
6. assegna eventuali Binary Lease;
7. human task solo se sbloccato;
8. se nessun task è sicuro, `IDLE`.

```text
WritableSet(T1) ∩ WritableSet(T2) = ∅
BinaryLeaseSet(T1) ∩ BinaryLeaseSet(T2) = ∅
```

---

# 43. Integrazione batch

Sequenza consigliata:

1. freeze delle quattro attività;
2. `git fetch --prune origin`;
3. shared-ID `check`;
4. shared-ID `audit-refs`;
5. verifica base SHA;
6. verifica write-set;
7. integra branch C++/testuali;
8. integra una Binary Asset Lease alla volta;
9. Unreal load/save/validator;
10. aggiorna owner docs una volta;
11. aggiorna Feature Registry;
12. rigenera JSON/shortlist/project graph;
13. script/doc validation;
14. Unreal Automation;
15. build;
16. PIE/manuale se richiesto;
17. packaged se gate;
18. aggiorna GitHub;
19. genera il batch successivo.

---

# 44. Tracking ecosystem da riallineare

Verificare la coerenza almeno di:

```text
docs/roadmap/roadmap-checkpoint.md
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-post-v0.1.md

docs/roadmap/feature-registry.yaml
docs/roadmap/feature-registry.json

docs/roadmap/roadmap.shortlist.md
docs/roadmap/featuremap.shortlist.md
docs/roadmap/scenariomap.shortlist.md
docs/roadmap/milestonemap.shortlist.md
docs/roadmap/editormap.shortlist.md

docs/roadmap/editor-sessions.yaml
docs/technical/test-manuali-pie.md

docs/roadmap/project-graph.json
docs/control-center/**

docs/CONTEXT_INDEX.md
AGENTS.md
CLAUDE.md
```

Non tutti devono cambiare. Tutti devono essere verificati.

---

# 45. Artefatti nuovi proposti

Solo se non esiste un equivalente:

```text
docs/roadmap/parallel-tracks.yaml
docs/roadmap/parallel-batch.yaml
docs/roadmap/plans/parallel-track-index.md

docs/roadmap/track-spatial.shortlist.md
docs/roadmap/track-simulation.shortlist.md
docs/roadmap/track-client-tools.shortlist.md
docs/roadmap/track-content-editor.shortlist.md
```

Le shortlist sono generate, non editate.

---

# 46. Estendere l'infrastruttura, non costruirne una seconda

Preferire l'estensione di:

```text
scripts/feature_registry.py
scripts/rt_shared_id.py
project-graph generation
Control Center data contract
```

a nuovi tool sovrapposti.

Per i track:
- output idempotente;
- `--check`;
- test;
- nessuna duplicazione di status.

Per ID:
- audit namespace;
- estendere `rt_shared_id.py` solo dove sensato;
- namespace serialized format con gate più forte.

---

# 47. Versioni serializzate: livello di rischio superiore

Prima di incrementare:
```text
MapFormatVersion
ReplayManifestVersion
TurnLog format
serialized schema/enums
```

un branch deve:
1. cercare rivendicazioni concorrenti;
2. verificare tutti i ref live;
3. definire migrazione;
4. definire compatibility;
5. mantenere fixture old/new;
6. passare un integration gate.

Due branch con `FormatVersion = 8` ma schemi diversi sono un incidente di dati, non solo documentazione.

---

# 48. Validator parallelismo

Solo se non esiste equivalente, aggiungere un validator minimo che rilevi:

```text
same writable path assigned to 2 tracks -> ERROR
same binary path leased twice -> ERROR
binary lease without issue -> ERROR
binary lease without base_sha -> ERROR
integration-only assigned to track -> ERROR
unknown track -> ERROR
duplicate asset create destination -> ERROR
```

Non costruire una piattaforma.

---

# 49. Test workflow

Riutilizzare prima i test esistenti.

Da coprire se mancano:

Shared IDs:
- concurrent reserve -> distinct IDs;
- live-ref collision -> audit fails.

Track mapping:
- all feature IDs resolve;
- generated views idempotent;
- one source of track classification.

Batch:
- write overlap detected;
- integration-only violation detected.

Binary leases:
- same asset two holders -> fail;
- stale base detectable;
- duplicate create path -> fail.

---

# 50. AGENTS.md / CLAUDE.md

Aggiungere solo una sintesi:

### Parallel work
- one executable session = one worktree + branch;
- four-process model;
- batch write-set required;
- unassigned file = stop.

### Shared IDs
- never manual max+1;
- use `rt_shared_id.py` where supported;
- audit monotonic namespaces;
- `audit-refs` before merge.

### Unreal binaries
- human-first;
- Claude only with exclusive Binary Asset Lease;
- one holder/path;
- no binary merge;
- Unreal/Content Browser only.

Il dettaglio vive nel documento owner del workflow.

---

# 51. `workflow-parallel-claude.md` diventa owner tecnico

Estendere il documento esistente con:
- 4-process model;
- Parallel Batch schema;
- write-set;
- integration-only;
- Binary Asset Lease;
- binary conflict recovery;
- namespace audit;
- integration algorithm.

Preservare D-135 e le garanzie già implementate.

---

# 52. Validation finale indicativa

Usare soltanto comandi realmente presenti:

```bash
python scripts/rt_shared_id.py check
git fetch --prune origin
python scripts/rt_shared_id.py audit-refs

python scripts/feature_registry.py validate
python scripts/feature_registry.py shortlist
python scripts/feature_registry.py shortlist --check

python scripts/check-docs-links.py
python scripts/check-docs-symbols.py
python scripts/check-docs-naming.py --check
```

Adattare alle CLI correnti.

Non introdurre CI nuova solo per questo consolidamento: il repository attuale ha scelto gate locali/manuali.

---

# 53. Definition of Done del consolidamento

- [ ] modello 4 processi dichiarato;
- [ ] vecchio 7-lane ritirato/superseded senza link rotti;
- [ ] quattro tracciati generati o equivalente;
- [ ] tracciati allineati v0.1→v1.0;
- [ ] nessun nuovo asse milestone/gate;
- [ ] parallel batch con write-set;
- [ ] Binary Asset Lease;
- [ ] Content/Editor human-first ma Claude non escluso;
- [ ] `D-nnn` resta su `rt_shared_id.py`;
- [ ] altri namespace monotoni auditati;
- [ ] Feature/Scenario/Editor/Milestone/Project Graph coerenti;
- [ ] generated views rigenerate;
- [ ] validator/docs checks verdi;
- [ ] prossimo batch proposto con file esatti.

---

# 54. Cose da NON fare

Non:
- creare quattro roadmap manuali con checkbox;
- duplicare stato GitHub;
- rinumerare epic canoniche;
- creare nuovi Gxx;
- assegnare `D-nnn` a mano;
- costruire un secondo ID allocator;
- dare lo stesso `.uasset/.umap` a due holder;
- fare binary merge;
- patchare asset raw;
- versionare in massa asset Fab ignorando la policy;
- spostare una feature in due copie;
- inventare task per una track vuota.

---

# 55. Prima sequenza operativa Claude

## Audit
1. fetch origin;
2. annota HEAD;
3. leggi AGENTS/CLAUDE;
4. leggi `workflow-parallel-claude.md`;
5. leggi vecchio lane index + lane files;
6. leggi `roadmap-v0.1.md`;
7. leggi `roadmap-post-v0.1.md`;
8. leggi registry/schema;
9. leggi EditorMap + editor sessions;
10. leggi Control Center spec;
11. leggi `rt_shared_id.py` + tests;
12. ricerca tutte le reference ai vecchi lane files.

## Reconciliation
- misura stato live;
- identifica owner docs;
- identifica generated views;
- inventaria namespace globali;
- inventaria issue aperte;
- inventaria file binari che i task potrebbero toccare.

## Design minimo
Prima del diff scrivi:
```text
Files create
Files modify
Files retire
Generated outputs
Script changes
Tests
Risks
Open decisions
```

## Implementazione
Diff minimo, niente refactor opportunistici.

---

# 56. Report finale richiesto a Claude

Restituire:

1. HEAD verificato;
2. vecchio modello lane trovato;
3. file lane ritirati/superseded;
4. nuovo modello 4 processi;
5. track Spatial;
6. track Simulation;
7. track Client/Replay/Tools;
8. track Content/Editor;
9. roadmap v0.1→v1.0;
10. Binary Asset Lease;
11. shared ID policy;
12. namespace progressivi auditati;
13. Feature Registry changes;
14. Scenario Map changes;
15. Editor Map changes;
16. Roadmap/Milestone Map changes;
17. Project Graph/Control Center changes;
18. generated artifacts;
19. files changed;
20. tests/validators;
21. conflicts;
22. open decisions;
23. next parallel batch.

Formato obbligatorio del prossimo batch:

```text
SPATIAL
  issue:
  branch:
  writable:
  binary leases:

SIMULATION
  issue:
  branch:
  writable:
  binary leases:

CLIENT / REPLAY / TOOLS
  issue:
  branch:
  writable:
  binary leases:

CONTENT / EDITOR
  issue/session:
  writable assets:
  binary leases:
  manual verification:
```

Se una track non ha lavoro sicuro:

```text
IDLE — nessun task utile con write-set disgiunto.
```

---

# 57. Principio finale

Ottimizzare:

```text
parallelismo reale
- merge conflict
- stato duplicato
- collisioni ID
- conflitti binari
```

non il numero di branch.

Architettura operativa:

```text
                 CURRENT MAIN
                      |
        +-------------+-------------+--------------+
        |             |             |              |
        v             v             v              v
    SPATIAL       SIMULATION    CLIENT/TOOLS    HUMAN EDITOR
     Claude         Claude         Claude          Content
        |             |             |              |
        +-------- write-set + binary leases -------+
                      |
                      v
                 INTEGRATION
                      |
             shared ID / overlap
                      |
               regenerate maps
                      |
             tests + PIE + packaged
                      |
                      v
                    MAIN
```

Le mappe di tracciamento devono restare **viste dello stesso grafo**, non verità indipendenti.
