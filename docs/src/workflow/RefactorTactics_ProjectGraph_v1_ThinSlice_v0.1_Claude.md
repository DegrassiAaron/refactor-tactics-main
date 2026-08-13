# RefactorTactics — Project Graph v1 · Thin Slice reale v0.1

> **Handoff operativo per Claude — PROPOSTA, non source of truth**
>
> Data verifica: 2026-08-13.
> Base: repository live `DegrassiAaron/refactor-tactics-main`, issue e file correnti letti prima di scrivere questo handoff.
>
> Obiettivo: provare il modello del Project Graph su **tre catene reali** prima di migrare tutto il backlog:
>
> 1. Reactions / Decision Boundary;
> 2. Golden Replay / Showcase;
> 3. Character Presentation / Asset / PIE.
>
> Non modificare file generati a mano. Prima di applicare: riallineare a `origin/main`, controllare PR aperte,
> rigenerare baseline e rileggerne le issue live.

---

## 0. Perché questo thin slice

La migrazione completa sarebbe troppo grande da validare “a vista”.

Queste tre catene sono state scelte perché insieme esercitano quasi tutti i concetti del nuovo grafo:

| Catena | Cosa prova |
|---|---|
| Reactions | hard dependency, `follows`, capability, scenario blocked/planned, fork verso un consumer |
| Golden / Showcase | junction, prerequisiti multipli, soft “prima del golden”, PIE finale |
| Character | CODE + ASSET + PIE, sessioni umane già esistenti, issue non bloccante, asset esterni |

Se queste tre risultano leggibili nel Control Center, lo schema è abbastanza buono per la migrazione v0.1.

---

# 1. Catena A — Reactions / Decision Boundary

## Stato live verificato

| Nodo | Stato GitHub | Relazione reale |
|---|---|---|
| #164 · CP 14.4 | chiusa, citata come prerequisito di #165 | prerequisito soddisfatto |
| #165 · CP 14.5 | OPEN · P2 | dipende da #164; produce la prima Decision Window viva / Overwatch |
| #166 · CP 14.6 | OPEN · P2 | **hard**: dipende da #165 |
| #314 · CP 14.7 | OPEN · P3 | **hard**: dipende da #165; **soft**: segue #166 |
| #318 · Harness TurnLog assertions | CLOSED | capability consegnata; non è più blocker |
| #361 · Time Bank / TurnLog format reconciliation | CLOSED | decisione/formato consegnati; non è più blocker |
| #319 · CP 14.8 | OPEN · P3 | **soft order**: segue #314; esplicitamente non precede 14.5/14.6 |
| #512 · CP 15.3 metà B | OPEN · P1 | **hard**: bloccata da #165 |

### Punto strutturale da rendere visibile

`#165` è un **fork**:

- sblocca la prosecuzione di E14 (`#166`, `#314`, poi `#319`);
- sblocca anche `#512`, perché il DecisionProvider non ha senso finché una vera opportunity non viene prodotta in partita.

Questa è esattamente la ragione per cui la capability `DecisionBoundary` merita di essere un nodo/edge del grafo
e non una falsa dipendenza “E14 dipende da tutta E13”.

## Scenari

Il corpus attuale dichiara già scenari `Spec.Clash.*` su disco, bloccati da capability come
`DecisionBoundary` e `ReactionClash`.

Il Feature Registry dichiara inoltre scenari planned per:
- Overwatch;
- Reaction Profile;
- Reaction Clash;
- Decision Time Bank.

Nel grafo:

- scenario reale + capability mancante → `BLOCKED`;
- planned → `PLANNED`;
- quando il provider della capability atterra, il generator ricalcola lo stato.

---

# 2. Catena B — Golden Replay / Showcase

## Stato live verificato

| Nodo | Stato | Relazione |
|---|---|---|
| #66 · CP 8.3 propagazione elettrica | CLOSED | prerequisito di #170 già soddisfatto |
| #75 · CP 10.2 obiettivo contestabile | OPEN | **hard** prerequisito di #170 |
| #512 · CP 15.3 metà B | OPEN | **hard** prerequisito di #170; a sua volta hard su #165 |
| #625 · hazard fuori dal TurnLog | OPEN | **soft/pre-golden**: conviene chiuderla prima di pinning #170 |
| #649 · cover bypass osservabilità | OPEN, implementazione quasi tutta fatta | relazione di consistency, non hard blocker di #170 |
| #687 · FormatVersion non serializzato | OPEN | rischio consistency/migrazione; non inventare una hard edge verso #170 |
| #170 · CP 15.4 Golden replay | OPEN · P1 | **hard**: CP15.3 + #66 + #75 |
| #171 · CP 15.5 Presentazione/playtest | OPEN · P1 | **hard**: dipende da #170 |

### Regola importante

Non tradurre automaticamente la vecchia lane:

`#625 + #687 + #649 → #512 → #170`

in hard dependency.

Le issue live non dichiarano tutte quelle frecce come hard.

Proposta typed edge:
- #625 → #170 = `follows_before` / `follows` con rationale “evitare rebaseline del golden”;
- #649 ↔ #170 = `related` / consistency;
- #687 ↔ golden/release consistency = `related`, finché un owner non la rende gate.

Il grafo deve distinguere **ordine raccomandato** da **blocco reale**.

## Junction reale

`#170` è un junction perché riceve almeno:
- la catena DecisionProvider (#165 → #512);
- Objective (#75);
- Environment combo (#66, già chiusa);
- eventuali consistency tasks che è economicamente meglio eseguire prima.

Il junction deve essere **derivato** dal numero/tipo di ingressi, non marcato a mano.

## Uscita umana

`#171` è un ottimo esempio di nodo che finisce in PIE:
- la partita deve girare in PIE;
- serve `L_Showcase_Relay`;
- servono screenshot/video/playtest di leggibilità.

Non è una prova puramente automatica.

---

# 3. Catena C — Character Presentation / ASSET / PIE

## Epic corrente

E21 (#286) dichiara esplicitamente che:
- il C++ di base esiste già;
- ciò che manca è soprattutto lavoro in editor;
- la feature è `RT-FEAT-CHAR-PRESENTATION`;
- i checkpoint sono #287, #288, #289.

Ha inoltre lavoro collegato #715.

## Checkpoint

| Nodo | Stato | Dipendenza |
|---|---|---|
| #287 · E21.1 | OPEN · P1 | E6; mesh/Blueprint personaggi |
| #288 · E21.2 | OPEN · P1 | **hard**: #287 |
| #289 · E21.3 | OPEN · P1 | **hard**: #287 |
| #593 · root scalato ARTUnit | OPEN · P2 bug | **esplicitamente non blocca U7**; relazione `related` |
| #715 · ConfigureFromHeroData perde campi | OPEN · P2 | CODE collegato a E21; sblocca anche perception #159 |

### Nota naming corrente

Il repository corrente distingue:
- Stable ID che restano `Hero.Flux`, `Hero.Riva`, `Hero.Bastion`, `Hero.Vektor`;
- nomi player-facing `Gadget`, `Phase`, `Riktor`, `Wraith`.

Non rinominare Stable ID nel grafo.

## Sessioni umane esistenti

Il file canonico `editor-sessions.yaml` contiene già il blocco 3 “Presentazione”.

### U7 — Personaggi Paragon

- `critical: true`
- produce i quattro Blueprint-unità;
- asset tracciati sotto `Content/RT/Characters/...`;
- `unblocked_by: []`;
- `shares_setup_with: [U8, U9]`;
- verifica `PIE-AS2`, `PIE-FACING`;
- `unblocks: [U8, U19]`.

**Classificazione proposta**: `execution_lane: asset`.

Motivo: l'output primario sono asset Blueprint configurati; le PIE refs sono evidence della sessione.

### U8 — Animazioni

- `critical: true`;
- produce Anim BP e montage;
- `unblocked_by: []`;
- `shares_setup_with: [U7, U9]`;
- verifica `PIE-AS4a`, `PIE-AS4b`;
- `unblocks: [U9, U19]`.

**Classificazione proposta**: `execution_lane: asset`.

### U9 — Leggibilità e riferimento visivo

- `critical: true`;
- produce video/screenshot di riferimento;
- `unblocked_by: []`;
- `shares_setup_with: [U7, U8]`;
- verifica `PIE-AS5`, `PIE-SEL`, `PIE-ICON-01`, `PIE-FMT-01`;
- done quando non ci sono cilindri salvo asset mancanti e il riferimento visivo è nel repo.

**Classificazione proposta**: `execution_lane: pie`.

### Perché U7/U8/U9 NON vanno trasformate in una catena hard

`editor-sessions.yaml` le tiene nello stesso blocco e condivide il setup.
L'owner esplicitamente distingue:
- dipendenza dura;
- ordine di lavoro nello stesso allestimento.

Quindi il graph deve poter dire:
- checkpoint #288 richiede #287;
- sessione U8 può essere mostrata “dopo U7” come `follows`, senza inventare un hard block che il file owner non dichiara.

---

# 4. Edges concrete proposte per il thin slice

## Hard

| From | To | Tipo |
|---|---|---|
| issue:164 | issue:165 | `requires` |
| issue:165 | issue:166 | `requires` |
| issue:165 | issue:314 | `requires` |
| issue:165 | issue:512 | `requires` |
| issue:512 | issue:170 | `requires` |
| issue:66 | issue:170 | `requires` |
| issue:75 | issue:170 | `requires` |
| issue:170 | issue:171 | `requires` |
| issue:287 | issue:288 | `requires` |
| issue:287 | issue:289 | `requires` |

## Soft order

| From | To | Tipo | Perché |
|---|---|---|---|
| issue:166 | issue:314 | `follows` | #314 dice “Segue 14.6” |
| issue:314 | issue:319 | `follows` | #319 dice “Segue 14.7” |
| issue:625 | issue:170 | `follows` | conviene prima del pinning golden |
| session:U7 | session:U8 | `follows` | stesso setup; non hard |
| session:U8 | session:U9 | `follows` | stesso setup; non hard |

## Capability

| Provider | Capability | Consumer |
|---|---|---|
| issue:165 | `DecisionBoundary` | issue:512 |
| issue:165 | `DecisionBoundary` | scenari Overwatch/Clash che la richiedono |
| issue:318 (closed) | `TurnLogAssertions` | scenari Clash / harness |
| issue:361 (closed) | `TimeBankLogContract` | issue:319 |

## Related, NON blocking

| From | To | Tipo |
|---|---|---|
| issue:593 | session:U7 | `related` |
| issue:593 | session:U9 | `related` |
| issue:649 | issue:170 | `related` |
| issue:687 | issue:170 | `related` |
| issue:715 | epic:E21 | `related` |

---

# 5. Come deve apparire nel Control Center

Non mostrare tutte le frecce subito.

## Vista tabellare di default

### Actions / Reactions

| Attività | Lane | Stato derivato | Hard prerequisite | Soft order | Sblocca |
|---|---|---|---|---|---|
| #165 | CODE | candidate READY | #164 ✅ | — | #166, #314, #512 |
| #166 | CODE | BLOCKED | #165 | — | UI/pacing E14 |
| #314 | CODE | BLOCKED | #165 | dopo #166 | Reaction Clash |
| #319 | CODE | queued | — | dopo #314 | Time Bank |
| #512 | CODE | BLOCKED | #165 | — | #170 |

### Character

| Attività | Lane | Stato | Output |
|---|---|---|---|
| U7 | ASSET | derivato da editor registry | Blueprint unità |
| U8 | ASSET | derivato da editor registry | Anim BP / montage |
| U9 | PIE | derivato da editor registry | verifica + riferimento visivo |
| #593 | CODE | OPEN ma non blocker | fix root/scaling |
| #715 | CODE | OPEN | DisplayName + HearingThreshold |

## Focus mode

Cliccando #165:
- upstream: #164;
- downstream hard: #166, #314, #512;
- capability: DecisionBoundary;
- scenari che aspettano DecisionBoundary;
- feature collegate.

Cliccando U7:
- lane: ASSET;
- output;
- PIE refs;
- shared setup U8/U9;
- issue E21 collegate;
- bug #593 come `related`, non blocker.

---

# 6. Regole che questo thin slice deve dimostrare

1. Un `follows` non rende `BLOCKED`.
2. Un `related` non entra nel calcolo readiness.
3. Una capability può sbloccare più consumer.
4. Un issue chiuso può restare nel graph come provider soddisfatto.
5. PIE/ASSET possono esistere senza duplicare issue.
6. Una sessione può produrre asset e anche verificare PIE: `execution_lane` indica il lavoro primario, `verifies`
   indica l'evidence.
7. Lo stesso setup non equivale a hard dependency.
8. Il Control Center non ricalcola nessuna di queste regole.

---

# 7. Prima implementazione consigliata

Implementare SOLO questi nodi/edge nel primo passaggio.

Acceptance:
- il graph renderer li carica;
- i tre gruppi sono leggibili;
- upstream/downstream funziona;
- `#165` mostra il fork;
- `#170` mostra il junction;
- U7/U8/U9 appaiono ASSET/ASSET/PIE;
- #593 non blocca U7;
- gli scenari bloccati da `DecisionBoundary` sono raggiungibili dal provider;
- nessun altro backlog viene migrato ancora.

Solo dopo la review visiva si espande all'intera v0.1.

---

# 8. Attenzione alle PR concorrenti

Al momento dell'audit risultano PR recenti/aperti che toccano il Feature Registry e documentazione di naming.
Prima di applicare:
- rifare search PR;
- se `feature-registry.yaml` è cambiato, rigenerare JSON/shortlist dal source unito;
- non risolvere merge di `project-graph.json` a mano.

Il repository ha già avuto auto-merge silenziosi dei generati: il source unito va sempre rigenerato.
