# RefactorTactics — Project Graph · Migrazione completa v0.1 → v0.4

> **HANDOFF PER CLAUDE — PROPOSTA OPERATIVA**
>
> Data audit: 2026-08-13.
>
> Obiettivo: portare il Project Graph dal thin slice a una rappresentazione completa e navigabile
> delle release **v0.1, v0.2, v0.3 e v0.4**, con Feature, Epic, Checkpoint/Issue, CODE/PIE/ASSET,
> Scenario, PIE evidence, capability e dipendenze tipizzate.
>
> Regola: **non creare una seconda roadmap**. Le sorgenti canoniche esistenti restano owner dello stato.
> `execution-graph.yaml` possiede solo la topologia che oggi non ha un owner strutturato.
>
> Prima di applicare: aggiornarsi a `origin/main`, controllare PR aperte, eseguire baseline e non
> modificare mai `project-graph.json`/shortlist a mano.

---

# 1. Stato corrente misurato

## v0.1

La roadmap corrente dichiara:

- **21 Epic**: E1–E21;
- **100 checkpoint**;
- **74 Feature** nel Feature Registry;
- ordine di lavoro già stabilito dalla roadmap;
- 15 gate di release in `v0.1-definition-of-done.md`.

Questi numeri sono **input di verifica**, non costanti da hardcodare nel nuovo sistema.
Dopo la migrazione devono essere ricalcolati dal generator.

## Feature Registry

La shortlist corrente dichiara:

- 105 Feature totali;
- 74 `v0.1`;
- 16 `v0.2`;
- 15 `future`.

Il registry accetta ancora solo:

```text
v0.1 | v0.2 | future
```

quindi **v0.3 e v0.4 non sono esprimibili oggi**: alcune Feature sono schiacciate in `future`,
e almeno una Feature di E34 è ancora classificata v0.2.

## Roadmap post-v0.1

`roadmap-post-v0.1.md` è owner esplicito delle release v0.2–v0.4:

- v0.2 = E22–E26 + E35 + E36 + E38 + E39;
- v0.3 = E27 + E28 + E29 + E33;
- v0.4 = E30 + E31 + E32 + E34.

Oltre v0.4 resta `future / north-star`.

---

# 2. Correzione strutturale prima della migrazione

La migrazione completa NON deve richiedere di autorializzare a mano 100 checkpoint e 74 feature
dentro `execution-graph.yaml`.

Il generator deve costruire il **baseline graph** dalle sorgenti esistenti:

| Oggetto | Owner / discovery |
|---|---|
| Feature | `feature-registry.yaml` |
| Epic v0.1 | `roadmap-v0.1.md` |
| Epic v0.2–v0.4 | `roadmap-post-v0.1.md` |
| Checkpoint | roadmap + issue binding esistente |
| Issue collegate | Feature Registry / roadmap / issue owner |
| Sessioni umane | `editor-sessions.yaml` |
| PIE | `test-manuali-pie.md` |
| Scenario reale | `Scenarios/` |
| Scenario planned | Feature Registry |
| Gate v0.1 | `v0.1-definition-of-done.md` |
| Stato | owner esistente + funzioni Python |

`execution-graph.yaml` contiene solamente:

- lane/domain;
- override;
- relazioni hard/soft non derivabili in modo sicuro;
- capability execution;
- mapping esplicito quando il collegamento non è già presente altrove.

Questa è la differenza fra **migrare il grafo** e **duplicare il progetto**.

---

# 3. Fase A — Migrazione completa v0.1

## 3.1 Acceptance di copertura

La v0.1 è “migrata” quando il graph contiene e rende raggiungibili:

- tutte le Feature `release: v0.1`;
- E1–E21;
- tutti i checkpoint della roadmap v0.1;
- tutte le issue collegate ai checkpoint/Feature;
- tutte le sessioni Editor che appartengono al lavoro v0.1;
- tutte le PIE ref dichiarate dalle Feature/sessioni;
- tutti gli scenari reali delle Feature v0.1;
- tutti gli scenari planned delle Feature v0.1;
- i gate G1–G15;
- i riferimenti a test e Wiki/spec già esposti dal registry.

Non serve che ogni issue standalone abbia stato GitHub offline.
Se il generator non ne possiede una sorgente offline, lo stato è `UNKNOWN`, non inventato.

## 3.2 Nodi derivati, non scritti

Creare automaticamente:

```text
release:v0.1
feature:RT-FEAT-...
epic:E1 ... epic:E21
checkpoint:E1.1 ...
issue:<n>
session:U<n>
pie:PIE-...
scenario:<ScenarioId>
gate:G1 ... gate:G15
```

## 3.3 Relazioni che si possono derivare in sicurezza

Derivare:

- `release contains feature`;
- `release contains epic`;
- `epic contains checkpoint`;
- `feature tracked_by epic/checkpoint`;
- `feature linked_issue issue`;
- `feature verified_by scenario`;
- `feature verified_by pie`;
- `scenario declared_by feature` (inversa);
- `session verifies pie`;
- `session waits_for / unblocks` dai campi già canonici;
- `gate release` dalla DoD.

Non chiamare `linked_issue` automaticamente `implements`.
La decisione D-B del PCC ha già rifiutato quella qualificazione automatica.

## 3.4 Dipendenze hard

Per v0.1 importare solo dipendenze esplicite da owner:

1. dipendenze delle Feature già nel registry;
2. `Dipende da` / prerequisiti dichiarati nelle Epic/checkpoint;
3. `unblocked_by` delle sessioni;
4. capability requirements già dichiarati dagli scenari;
5. overlay manuale in `execution-graph.yaml`.

Non dedurre hard dependency dal semplice ordine numerico dei checkpoint.

## 3.5 Soft order

Usare `follows` quando l'owner dice:
- “segue”;
- “ordine consigliato”;
- “prima di” per costo/rebaseline;
- stessa apertura/setup ma non blocker.

Esempio già validato:
`#166 → #314` e `#314 → #319` sono ordine; non devono trasformare automaticamente il nodo in BLOCKED.

## 3.6 PIE / ASSET

`editor-sessions.yaml` resta owner.

Aggiungere `execution_lane` con default retrocompatibile `pie`.

Audit minimo già deciso:
- U7 Personaggi Paragon → `asset`;
- U8 Animazioni → `asset`;
- U9 Leggibilità/riferimento visivo → `pie`.

Regola di classificazione:
- ASSET = output primario è download/migrate/import/config di asset esterno;
- PIE = output primario è verdetto/playtest/configurazione/uso Editor.

Non classificare automaticamente ogni `.uasset` come ASSET.

## 3.7 Completezza v0.1: diagnostiche

Dopo la migrazione completa, attivare warning:

- Feature v0.1 senza nessun percorso a Epic/checkpoint/issue;
- checkpoint senza nodo risolvibile;
- issue checkpoint senza Feature e senza rationale;
- scenario di Feature v0.1 senza provider di capability;
- PIE v0.1 non collegata a Feature/sessione/scenario;
- sessione v0.1 orfana;
- hard cycle;
- execution node senza release/domain;
- stale generated artifact.

Solo DOPO che v0.1 è dichiarata completa, altrimenti questi warning sarebbero rumore intenzionale.

---

# 4. Fase B — v0.2 completa

## 4.1 Epic canoniche

La sorgente post-v0.1 stabilisce:

| Epic | Issue Epic | Tema |
|---|---:|---|
| E22 | #323 | Cover Window |
| E23 | #324 | Muri, porte, interaction graph |
| E24 | #325 | Standard 3v3 |
| E25 | #265 | Icon Language completo |
| E26 | #326 | Tactical Bot v1 |
| E35 | #322 | Roster 8 |
| E36 | #435 | Framework status |
| E38 | #609 | Economia turno / validazione |
| E39 | #704 | Spatial Transfer |

**Usare questa tabella owner**, non la vecchia §3 di `roadmap.shortlist.md`: quella vista manuale
è già rimasta indietro su E36/E38/E39 e mostra ancora E25 senza issue nonostante #265 esista.

## 4.2 Feature v0.2 già esistenti

Il registry corrente contiene 16 Feature v0.2, fra cui:

- Action Supers;
- Movement Compatibility;
- Spatial Transfer;
- Plan Validation;
- Status Framework;
- Action Budget (`DEFERRED`);
- Auxiliary Units;
- Roster v0.2;
- Character Transformation (vedi conflitto E34 sotto);
- Ice Engine;
- Faction System;
- Faction Scenarios;
- Map Standability;
- Transition Clearance;
- Replay Archive;
- Tactical Bot.

Non preservare il numero “16” come target: dopo l'allineamento E34 il conteggio può cambiare.

## 4.3 Binding sicuri

Binding espliciti supportati dalle issue/roadmap:

- E23 ↔ `RT-FEAT-MAP-STANDABILITY`
- E23 ↔ `RT-FEAT-MAP-TRANSITION-CLEARANCE`
- E26 ↔ `RT-FEAT-BOT-TACTICAL`
- E35 ↔ `RT-FEAT-CHAR-V02-ROSTER`
- E36 ↔ `RT-FEAT-STATUS-FRAMEWORK`
- E38 ↔ `RT-FEAT-ACTION-BUDGET`
- E38 ↔ `RT-FEAT-ACTION-MOVEMENT-COMPAT`
- E38 ↔ `RT-FEAT-ACTION-PLAN-VALIDATION`
- E39 ↔ `RT-FEAT-ACTION-SPATIAL-TRANSFER`

E22/E24/E25 possono estendere Feature già nate in v0.1:
non cambiare la release della Feature base solo per farla “entrare” nell'Epic nuova.

Usare edge:
```text
extends
```
oppure una relazione equivalente non-bloccante/trace se si decide di ammetterla nello schema.

---

# 5. Fase C — riallineamento v0.3

## 5.1 Estendere enum release

Modificare:

Python:
```python
RELEASES = {"v0.1", "v0.2", "v0.3", "v0.4", "future"}
```

Documentazione schema:
```text
release: v0.1 | v0.2 | v0.3 | v0.4 | future
```

Test:
- nuovo valore accettato;
- valore sconosciuto rifiutato;
- shortlist raggruppa correttamente;
- Control Center filtra v0.3/v0.4.

## 5.2 Reassignment supportati direttamente dalle fonti

### v0.3

**Sicuri:**

- `RT-FEAT-BOT-BELIEF`: `future → v0.3`
  - E27 / #327 lo cita esplicitamente.

- `RT-FEAT-BOT-PREDICTIVE`: `future → v0.3`
  - E28 / #328 lo cita esplicitamente.

- `RT-FEAT-ACTION-TRAPS`: `future → v0.3`
  - E29 / #329 lo cita esplicitamente.

- `RT-FEAT-ACTION-DELAYED`: `future → v0.3`
  - E29 / #329 lo cita esplicitamente.

- `RT-FEAT-INTENT-CONDITIONAL`: `future → v0.3`
  - E33 / #330 lo cita esplicitamente.

## 5.3 E27 non sposta le Feature perception v0.1

E27 dichiara:
`RT-FEAT-PERCEPTION-*` + `RT-FEAT-BOT-BELIEF`.

Le Feature perception base restano v0.1 perché sono capacità già incluse nel vertical slice.
E27 le **estende/completa**, non cambia retroattivamente la release in cui sono nate.

Nel graph:
```text
epic:E27 extends feature:RT-FEAT-PERCEPTION-...
```

e contiene la Feature nuova v0.3 `RT-FEAT-BOT-BELIEF`.

---

# 6. Fase D — riallineamento/population v0.4

## 6.1 Reassignment sicuro

- `RT-FEAT-CHARACTER-STATE`: `future → v0.4`
  - featuremap la collega esplicitamente a E34/#244.

## 6.2 Conflitto da risolvere, non da nascondere

Esiste anche:

`RT-FEAT-CHAR-TRANSFORMATION` — attualmente `v0.2`

ma E34 è il `Character State / Configuration System`.

Prima di migrare:

1. confrontare owner spec e gate dei due Feature ID;
2. verificare se rappresentano due scope distinti;
3. se sono duplicati, scegliere l'ID canonico e usare una relazione di sostituzione/completamento;
4. non cancellare un ID stabile senza una migrazione documentata;
5. non portare entrambi in E34 solo per far sparire il warning.

Il graph deve mostrare il conflitto finché non è deciso.

## 6.3 E30 / E31 / E32: gap di Feature

Le Epic esistono:

- E30 #331 — Operations map class;
- E31 #332 — multi-objective/logistics;
- E32 #333 — competitive 4v4.

Ma la shortlist corrente non mostra Feature future dedicate inequivocabili per queste tre Epic.

NON inventare automaticamente Feature ID.

Prima:
- cercare se una Feature esistente copre davvero l'estensione;
- se l'Epic estende una Feature v0.1, collegarla come `extends`;
- creare un nuovo Feature ID solo se il Feature Registry richiede una capacità distinta con gate propri.

Candidate base che NON vanno rinominate:
- E30 estende `RT-FEAT-MATCH-FORMAT` e la classe mappa;
- E31 estende `RT-FEAT-OBJECTIVE-SYSTEM`;
- E32 viene dopo `RT-FEAT-STRESS-4V4`, ma **stress test ≠ formato competitivo**.

Per E32 è probabile che serva una Feature distinta, ma questo è un esito di audit, non un dato da inventare qui.

---

# 7. Release manifest del graph

Il nuovo graph deve visualizzare almeno:

```text
v0.1 — Vertical Slice
v0.2 — Struttura e finestre
v0.3 — Informazione
v0.4 — Operations
future — North Star
```

Con `release` come nodo vero e filtri nel Control Center.

Il grafo non impone una dipendenza totale:
`release:v0.1 requires release:v0.2` sarebbe semanticamente invertito e inutile.

La relazione corretta è:
- ogni release contiene il proprio scope;
- i work item possono avere hard dependency cross-release;
- il gate di apertura v0.2/v0.3/v0.4 richiede i gate v0.1 verdi, come decisione di roadmap.

---

# 8. Nuova vista post-v0.1 generata

Il difetto attuale:
`roadmap.shortlist.md` §3 è dichiarata “Non generate” e ha già perso:
- E36;
- E38;
- E39;
- issue #265 di E25.

Dopo la migrazione creare una sezione **generata**:

```text
Release Map
```

derivata da:
- `roadmap-post-v0.1.md`;
- Feature Registry;
- execution graph.

Non trasformare `roadmap-post-v0.1.md` in generated:
resta owner umano delle release e dei confini.

Generare soltanto la vista corta.

---

# 9. Ordine dei PR

## PR 1 — infrastruttura + thin slice
Già definita dal bundle precedente.

## PR 2 — v0.1 completa
- discovery di tutte le 74 Feature;
- E1–E21;
- 100 CP;
- issue/ref;
- scenario/PIE/session;
- explicit edge overlay;
- diagnostica completezza.

## PR 3 — v0.2 completa
- E22–E26/E35/E36/E38/E39;
- feature v0.2;
- checkpoint esistenti;
- issue;
- scenario planned/reali;
- ASSET/PIE future dove già definite.

## PR 4 — release schema v0.3/v0.4
- enum release;
- generator;
- filtri;
- test;
- nessuna migrazione contenuto ancora.

## PR 5 — v0.3
- reassignment sicuri;
- E27/E28/E29/E33;
- edge extends;
- scenari.

## PR 6 — v0.4
- E30/E31/E32/E34;
- E34 conflict resolution;
- gap report Feature;
- nuove Feature solo dopo decisione.

Separare schema e migrazione massiva rende i diff revisionabili.

---

# 10. Definition of Done globale

La migrazione v0.1→v0.4 è completa quando:

- Release filter mostra v0.1/v0.2/v0.3/v0.4/future;
- ogni Epic canonica è presente;
- v0.1 ha copertura completa di Feature/Epic/CP;
- le Feature post-v0.1 sono nella release corretta;
- E37 non viene infilata in v0.4 per posizione del testo: la tabella release non la include;
- il conflitto `CHARACTER-STATE` / `CHAR-TRANSFORMATION` è risolto o visibilmente diagnosticato;
- E30/E31/E32 hanno binding Feature esplicito o gap diagnostico motivato;
- CODE/PIE/ASSET sono filtrabili;
- scenari reali, blocked e planned sono navigabili;
- ogni hard edge ha una fonte/rationale;
- `follows`/`related` non bloccano;
- `project_stats` è derivato;
- `generate --check` e `shortlist --check` rilevano drift;
- il Control Center non deriva stato;
- nessun generated è editato a mano.
