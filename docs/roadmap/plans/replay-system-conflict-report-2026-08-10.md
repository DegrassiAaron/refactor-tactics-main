# Replay System — conflict report

> `CURRENT` · **Stato**: audit chiuso · **due difetti strutturali da decidere prima di scrivere codice** (§4)
> **Audit eseguito su**: `5f9c2df` · **riverificato su**: `8707044` — una sessione parallela ha committato
> sullo stesso branch durante l'audit. Il commit aggiunge `RT-FEAT-TOOL-CONTROL-CENTER` e riallinea gli
> indici: **nessun owner auditato qui è cambiato**, i conteggi di epic sono quelli di `8707044`
> **Sorgente auditato**: `docs/src/RefactorTactics_Replay_System_Claude_Consolidation_2026-08-10.md`
> — **non ancora versionato** a HEAD, quindi citato per nome e non linkato
> **Scopo**: eseguire lo Step 1–3 che l'handoff stesso impone (§0, §38) — classificare ogni sua affermazione
> contro le source of truth reali — **prima** di toccare Decision Log, ADR, Feature Registry, roadmap,
> Scenario Map, Wiki o issue GitHub.
> **Regola applicata**: un handoff AI è l'ultima fonte della gerarchia. Dove contraddice una decisione
> accettata o il codice a HEAD, prevale il canone e la proposta si **registra**, non si applica.

---

## 1. Perimetro dell'audit

Consultati a HEAD:

| Fonte | Ruolo nell'audit |
|---|---|
| `Source/RefactorTactics/Turn/RTTurnLog.h` | schema reale di `FRTTurnLogEntry` |
| `Source/RefactorTactics/Turn/RTTurnLogLibrary.{h,cpp}` | `HashTurnLog`, serializzazione versionata, `CompareSerializedTraces` |
| `Source/RefactorTactics/Turn/RTHexSim.h` | `FRTHexSnapshot` e `MapHash` |
| `Source/RefactorTactics/ScenarioHarness/` | dove vive davvero `StateHash` (`RTTestResult.h`, `RTScenarioSession.cpp`) |
| `Source/RefactorTactics/Tests/RTSimulationDeterminismTests.cpp` | le 100 ripetizioni di CP 12.1 |
| [`technical/spec-turnlog.md`](../../technical/spec-turnlog.md) · [`spec-turnlog-serialize.md`](../../technical/spec-turnlog-serialize.md) | owner documentali; `D-TL-*`, `D-SR-*` |
| [`decisions/adr-0004-finestre-di-reazione.md`](../../decisions/adr-0004-finestre-di-reazione.md) | timeout `HOLD`, finestra 3,0 s, snapshot a inizio segmento |
| [`roadmap/feature-registry.yaml`](../feature-registry.yaml) | FeatureId reali e gate |
| [`roadmap/roadmap-v0.1.md`](../roadmap-v0.1.md) | epic E12, E15 e le tranche `S0`…`S10` |
| [`roadmap/roadmap-checkpoint.md`](../roadmap-checkpoint.md) | milestone M10, M11 |
| [`roadmap/roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) | v0.2 / v0.3 / v0.4 |
| [`technical/scenario-map.md`](../../technical/scenario-map.md) | chi verifica cosa |
| GitHub (`gh`) | `#26`, `#81`, `#153`, `#170`, `#178`, `#295` |

**Non** consultati, e restano da verificare prima delle azioni che li toccano: la Wiki (repo separato),
`docs/OPEN_DECISIONS.md`, `docs/DOC_CONFLICT_MATRIX.md`. La ricerca issue è stata `replay in:title`: non è
un censimento esaustivo delle issue aperte. **Nessun test eseguito, nessuna build**: le affermazioni sul
codice vengono dalla lettura dei sorgenti, non da una run.

---

## 2. Sintesi

| Classificazione | Voci | Significato |
|---|---|---|
| `CURRENT` | 8 | l'handoff riporta correttamente il canone |
| `PROPOSED` | 11 | idea nuova, nessun conflitto: si registra o si costruisce |
| `CONFLICT` | 4 | contraddice una decisione accettata o il codice |
| `STALE` | 4 | descrive il repository come non è (più) |
| `DUPLICATE` | 6 | ridefinisce qualcosa che ha già un owner |

**Esito in una riga**: il modello concettuale è buono e in larga parte nuovo per il progetto; **la mappa del
repository che l'handoff porta con sé è sbagliata quasi ovunque** — ID di feature, ampiezza degli hash,
milestone, issue. Ma i quattro `CONFLICT` non riguardano nomi: riguardano **l'ordine degli eventi**, ed è
l'unica cosa che, decisa dopo, costringe a rigenerare il corpus golden.

---

## 3. Matrice

### 3.1 Modello e decisioni architetturali

| # | Tema | Cosa dice l'handoff | Cosa dice HEAD | Stato | Azione |
|---|---|---|---|---|---|
| 1 | Replay ≠ video, ≠ network replay UE (§2, §17) | il replay canonico è logico: snapshot + intenti + decisioni + TurnLog + hash | nessun documento lo afferma; il progetto lo *pratica* ma non lo ha mai deciso | `PROPOSED` | **ADR** — è il contributo più forte del pacchetto |
| 2 | Formula del determinismo (§3.2) | stessa snapshot + intenti + decisioni + seed ⇒ stesso stato | invariante #4 + CP 12.1 chiuso: `Simulation.DeterministicReplay`, 100 ripetizioni | `CURRENT` | — |
| 3 | `RulesVersion`, `ContentManifestHash`, `ResolverConfigHash` nella formula | esistono, vanno registrati nell'header | **zero occorrenze in `Source/`**. `RT-FEAT-DATA-HASH` è RELEASE_READY, ma i suoi test dicono di cosa parla: `HexMap.*Hash*` e `TurnLog.Hash*` — **geometria della mappa e traccia**, nessun manifest di regole o cataloghi | `STALE` | costruirli è lavoro nuovo, non «consolidamento» |
| 4 | `uint64 StateHash / LogHash` (§9) | hash a 64 bit | tutti gli hash sono **uint32 FNV-1a**: `HashTurnLog`, `FRTTestResult::StateHash`, `URTHexMapAsset::ComputeHash` | `CONFLICT` | allargare a 64 bit invalida in blocco ogni hash golden: se si fa, si fa **prima** di `#178` |
| 5 | `StateHash` come dato di dominio (§3.2, §7) | ogni turno ha `StartStateHash`/`EndStateHash` | `StateHash` esiste **solo** in `FRTTestResult` (harness): digest di fine scenario calcolato in `RTScenarioSession.cpp:713` | `STALE` | un `StateHash` per turno è **da costruire**, e va nel resolver, non nel runner |
| 6 | Snapshot logici immutabili (§1) | fondamenta presenti | `FRTHexSnapshot` esiste, con `MapHash` e `Revision`; `IsSnapshotStale` rileva l'invalidazione | `CURRENT` | — |
| 7 | Playback ≠ verifica (§3) | due use case distinti, un replay può restare guardabile senza essere ri-simulabile | il progetto non nomina la distinzione | `PROPOSED` | entra nell'ADR §1 |
| 8 | Gli intent non bastano più (§4) | con Fast Reaction servono record di decisione runtime | vero e non registrato: `RT-FEAT-CORE-DECISION-BOUNDARY` è `SPECIFIED` e **non dichiara** questa conseguenza sul replay | `PROPOSED` | la conseguenza va scritta nella feature, non solo nell'ADR |
| 9 | Wall-clock fuori dal determinismo (§5) | il timeout entra come `Reason = Timeout`, la latenza resta telemetria | coerente con l'invariante #4 (no float) e con [ADR-0004](../../decisions/adr-0004-finestre-di-reazione.md) §3, dove il timeout è funzione **pura** dello stato | `CURRENT` | promuovere a invariante esplicita: oggi è implicita |
| 10 | Timeout Overwatch ⇒ `HOLD` (§5) | riportato | `HOLD`, mai `FIRE` | `CURRENT` | — |
| 11 | `DecisionRecord` ≠ `TurnLog` (§6) | concetti distinti, l'archivio può contenerli entrambi | il progetto non ha né il primo né la distinzione | `PROPOSED` | — |
| 12 | Un checkpoint per turno (§7, §11) | default ragionevole, niente intra-turno finché un test non lo dimostra | nessun checkpoint esiste | `PROPOSED` | ⚠️ vedi §4.2: senza `TurnNumber` nella voce, il turno è un contenitore esterno |

### 3.2 Ordinamento — i conflitti che contano

| # | Tema | Cosa dice l'handoff | Cosa dice HEAD | Stato | Azione |
|---|---|---|---|---|---|
| 13 | «Bloccare presto il **canonical event ordering**» (§19) | l'ordine degli eventi è parte del contratto | **il contrario è già deciso**: `HashTurnLog` ordina prima di mescolare (`RTTurnLogLibrary.cpp:186`), `D-SR-1` rende i byte permutazione-invarianti, e `Simulation.ChecksumStableAcrossPermutations` lo verifica dal 2026-08-08 | `CONFLICT` | **decidere** (§4.1) |
| 14 | Seek a `Turn 8 / Move / MicroStep 3` (§11) | il micro-step è indirizzabile | `FRTTurnLogEntry` non ha né `MicroStep` né indice di sequenza né `TurnNumber` | `CONFLICT` | il seek arriva alla **fase**, non oltre, finché lo schema non cambia |
| 15 | «First divergent event: Turn 4 / Move / MicroStep 3 / Event 12» (§16) | diagnostica per evento | un hash permutazione-invariante non può localizzare un evento; può dire **quale turno** diverge | `CONFLICT` | riformulare il DoD come «primo **turno** divergente» oppure aggiungere un secondo hash |
| 16 | Timeline UI per evento in ordine cronologico (§12) | marker ordinati nel tempo | l'ordine di visualizzazione sarebbe la chiave di sort di `EntryLess`, non l'ordine di risoluzione | `CONFLICT` | conseguenza di #13: si risolve con la stessa decisione |

### 3.3 Schema, contenuto e spiegabilità

| # | Tema | Cosa dice l'handoff | Cosa dice HEAD | Stato | Azione |
|---|---|---|---|---|---|
| 17 | Il replay come black box (§15) | il developer trova `UnitId`, `Edge/TransitionId`, `GraphRevision`, `MovementProfile`, `Cost`, `ReasonCode` | la voce ha `Phase`, `Category`, `Outcome`, `SrcCell`, `TgtCell`, `Amount`, `ActionId`, `BaseActionId`. **Sei campi su otto non esistono** | `PROPOSED` | è un'estensione dello **schema del TurnLog**, non del replay: vedi §4.2 |
| 18 | «Non inserire tutto in ogni evento se il dato è recuperabile dal checkpoint» (§15) | il checkpoint copre il resto | `UnitId` non è ricostruibile dal checkpoint quando due unità finiscono sulla stessa cella | `CONFLICT` *(sulla singola affermazione)* | non usarla come argomento per non estendere lo schema |
| 19 | Il Combat Log usa reason code canonici, non ricalcola (§12) | requisito | `DescribeEntry` fa già così — ma `ActionId` **oggi lo popolano solo le voci `Reaction`**: completarlo sul combattimento è **CP 11.3**, aperto | `CURRENT` *(parziale)* | dichiarare CP 11.3 come dipendenza del §12 |
| 20 | `ReplayFormatVersion` da bloccare presto (§19) | nuovo meccanismo di versioning | esiste già: `SerializeTurnLog` ha versione di formato, `WithChecksum`, letture legacy e `CompareSerializedTraces` | `DUPLICATE` | riusare, non affiancare |

### 3.4 Privacy

| # | Tema | Cosa dice l'handoff | Cosa dice HEAD | Stato | Azione |
|---|---|---|---|---|---|
| 21 | Full archive server-only durante il match (§13) | regola da introdurre | **già deciso**: `M10.2` piani in DTO filtrati, `M10.3` canary anti-leak | `CURRENT` | — |
| 22 | Il filtro non deve toccare il dato autorevole (§14) | «authoritative hidden state → sanitized view → client» | **stessa conclusione di [`#295`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/295)** (chiusa, nata da uno spec panel su E13): TurnLog **completo** nell'hash, filtro in un DTO separato, sul modello `FRTPlannedIntent → FilterForTeam → FRTIntentView` | `CURRENT` | ⭐ l'handoff riscopre la regola in modo indipendente: è una conferma, non una novità |
| 23 | Perspective Replay come modalità (§13) | modalità di riproduzione | il **modello** esiste, la **modalità** no | `PROPOSED` | M10, dopo `RT-FEAT-NET-PRIVATE-PLANNING` |

### 3.5 Inquadramento: feature, epic, milestone

| # | Tema | Cosa dice l'handoff | Cosa dice HEAD | Stato | Azione |
|---|---|---|---|---|---|
| 24 | `RT-FEAT-CORE-HASH-REPLAY` (§1, §22) | umbrella esistente da estendere | **non esiste.** L'area è divisa fra `RT-FEAT-CORE-TURNLOG` (RELEASE_READY), `-DETERMINISM` (INTEGRATED), `-PLAYBACK` (INTEGRATED), `RT-FEAT-DATA-HASH` (RELEASE_READY), `RT-FEAT-TEST-GOLDEN` (IMPLEMENTING) | `DUPLICATE` | non creare l'umbrella: cinque owner esistono già |
| 25 | «replay persistente/audit ancora parziale» (§1) | fondamenta presenti, resto parziale | **nessun tipo `Replay*` runtime esiste**: la parola compare in `Source/` solo in nomi di test e in un commento | `STALE` | lo stato è `DESIGNED`, non `PARTIAL` |
| 26 | Milestone v0.5 / v0.6 / v0.7 (§21) | tre versioni di destinazione | non esistono: il repo ha **v0.1** (E1–E21), **post-v0.1** v0.2/v0.3/v0.4, e le milestone d'esecuzione **M10** (rete e privacy) e **M11** (production readiness — che ha già «replay audit» in scope) | `STALE` | rimappare, vedi §7 |
| 27 | Epic candidata `[EPIC] Replay, Match History & Deterministic Audit` (§27) | crearla se manca | esistono [`#26`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/26) **E12** e [`#153`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/153) **E15** | `DUPLICATE` | nessuna epic nuova per la v0.1 |
| 28 | Issue 8 — corpus golden / CI (§28) | nuova | [`#178`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/178) CP 12.6 + [`#170`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/170) CP 15.4, entrambe aperte | `DUPLICATE` | — |
| 29 | Issue 7 — Deterministic Verifier (§28) | nuova | CP 12.1 è **chiuso nei fatti**; il verifier esiste come **Scenario Harness**, che richiama il resolver reale | `DUPLICATE` *(parziale)* | il Verifier è un adattatore su `URTScenarioRunner`, non un componente parallelo |
| 30 | Issue 11 — perspective/privacy (§28) | nuova | decisione presa (`#295`); esecuzione già pianificata a M10.2/M10.3 | `DUPLICATE` | — |
| 31 | `RTReplayVerifier` come classe nuova (§18) | nuovo modulo | vedi #29; e il §16 dello stesso handoff vieta «un simulatore parallelo» | `DUPLICATE` | contraddizione interna dell'handoff |
| 32 | Match Replay Archive per partita (§7) · ReplayHeader (§8) · Player (§18) · seek (§11) · Match History (§20) · UI (§12) | sei capability | nessuna esiste | `PROPOSED` ×6 | è **tutto** il lavoro nuovo reale |
| 33 | Risk register REPLAY-01…09 (§32) · telemetria (§33) · console `rt.Replay.*` (§34) | da consolidare | nessuno esiste; `REPLAY-04` (il player ri-risolve il gameplay) non sarebbe intercettato da nessun test attuale | `PROPOSED` | REPLAY-04 merita un test d'architettura |

---

## 4. I due difetti strutturali

Tutto il resto della matrice è rimappabile meccanicamente. Questi due no: vanno **decisi**, e vanno decisi
prima che `#178` produca il corpus golden, perché entrambi lo invaliderebbero.

### 4.1 L'hash del TurnLog è cieco all'ordine — per scelta

```cpp
// RTTurnLogLibrary.cpp:186
// Ordina prima di mescolare: stesso insieme di voci -> stessa sequenza -> stesso hash
// (permutazione-invariante).
TArray<FRTTurnLogEntry> Sorted = Entries;
SortTurnLog(Sorted);
```

La proprietà è deliberata, documentata in `D-SR-1`, e ha un test che la difende
(`Simulation.ChecksumStableAcrossPermutations`, CP 12.1). Anche i **byte serializzati** sono normalizzati:
la forma canonica è quella ordinata.

Conseguenze che l'handoff non vede:

1. un difetto di **ordinamento** nel resolver è invisibile all'hash, quindi invisibile al corpus golden;
2. l'ordine di risoluzione **non è conservato** da nessuna parte: quello che si rilegge è la chiave di sort;
3. quindi «primo evento divergente» (§16), seek al micro-step (§11) e timeline cronologica (§12) **non sono
   implementabili** sopra la traccia attuale.

**Non è un bug.** È un contratto più debole di quello che l'handoff assume. La scelta è binaria:

| | (a) l'ordine resta fuori | (b) l'ordine entra |
|---|---|---|
| Cosa si perde | seek al micro-step, timeline per evento, diagnostica per evento | la permutazione-invarianza, e con essa `ChecksumStableAcrossPermutations` |
| Cosa si guadagna | zero costo, corpus golden intatto | il replay può dire *dove* diverge |
| Come si fa | riformulare i DoD del §11/§12/§16 in termini di **turno e fase** | **secondo** hash order-sensitive accanto a quello esistente, non al posto suo |

La via (b) con due hash è compatibile con entrambe le esigenze e non tocca gli hash già prodotti.

> ✅ **Deciso il 2026-08-10 — [D-062](../../decisions/RT_PDR_00_Decision_Log.md): due hash affiancati.**
> La tabella qui sopra sovrastima il costo dell'opzione (b): l'ordine è **già canonico a monte**, quindi la
> permutazione-invarianza non si perde. Vedi §9.

### 4.2 Lo schema della voce non regge i casi d'uso dichiarati

`FRTTurnLogEntry` (`RTTurnLog.h:193-254`) contiene otto campi. Il caso d'uso di apertura del §15 —
*«al turno 9 una unità ha attraversato un muro»* — richiede di partire da un `UnitId` che **non c'è**, e di
ispezionare una transizione che **non è registrata**.

| Campo chiesto dal §15 | Presente |
|---|---|
| `FromCell` / `ToCell` | ✅ `SrcCell` / `TgtCell` |
| `ReasonCode` | ⚠️ `Outcome` sì; `ActionId` **solo sulle voci `Reaction`** — completarlo è CP 11.3 |
| `UnitId` · `TransitionId` · `GraphRevision` · `MovementProfile` · `Cost` | ❌ |
| `TurnNumber` · `MicroStep` | ❌ |

Ogni campo aggiunto entra o non entra nell'hash, e la scelta ha un precedente esplicito nel codice:
`BaseActionId` e `FormatId` sono rimasti **fuori** perché funzioni di campi già presenti. La stessa domanda
va posta per ciascun campo nuovo, **prima** di generare il corpus.

> ✅ **Deciso il 2026-08-10 — [D-063](../../decisions/RT_PDR_00_Decision_Log.md): `UnitId` e `TurnNumber`,
> fuori dall'hash** (formato v6), sul precedente di `BaseActionId`. I campi di movimento restano fuori e
> diventano la domanda aperta §9.5. `D-TL-2` è emendata: la cella resta chiave di **ordinamento**, non di
> **identità**.

---

## 5. Cosa il pacchetto aggiunge davvero

Al netto di tutto, quattro contributi che nessun documento del repository ha oggi:

1. **Il replay canonico è logico** (§2, §17) — mai deciso, sempre praticato. È materiale da ADR.
2. **Gli intent non bastano più** (§4) — con Decision Boundary e Fast Reaction lo snapshot + intenti non
   ricostruisce il turno. Ha una conseguenza **immediata**: `RT-FEAT-CORE-DECISION-BOUNDARY` non dichiara
   la dipendenza verso il determinismo del replay, e dovrebbe.
3. **La regola temporale** (§5) — il wall-clock resta telemetria, il timeout diventa `Reason = Timeout`.
   Trasforma una finestra real-time in un dato canonico; oggi è implicita in ADR-0004, non scritta.
4. **`REPLAY-04`** (§32) — il rischio che il player ri-risolva il gameplay. Nessun test attuale lo coglierebbe.

---

## 6. Issue ed epic — cosa esiste, cosa manca

Delle dodici issue candidate del §28, **quattro sono duplicati** (7, 8, 11, più l'epic del §27) e una
(**2**, DecisionRecord) è bloccata a monte: dipende da `RT-FEAT-CORE-DECISION-BOUNDARY`, che è `SPECIFIED`
con `runtime: todo`, dentro E14 — e la nota del registry dice che **E14 non parte prima di E13**.

Restano genuinamente nuove **sette**: 1 (ADR), 3 (Archive/Recorder), 4 (serializzazione + compatibilità),
5 (Player), 6 (seek), 9 (Match History), 10 (UI).

> ⚠️ **Prima di ricostruire qualunque piano**: [`#81`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/81)
> «CP 12.1 — Replay deterministico rinforzato» risulta **aperta su GitHub**, ma la roadmap la dà chiusa
> (`CP 12.1 ✅ 2026-08-08`, 100 iterazioni, 0 divergenze) e il codice conferma. È lo scarto già noto fra
> stato locale e stato remoto: va sanato, non ereditato.

---

## 7. Mappatura delle milestone

| §21 dell'handoff | Dove va davvero |
|---|---|
| Fondazioni / v0.1 | **già fatto** — E12 CP 12.1, `RT-FEAT-CORE-TURNLOG` RELEASE_READY |
| «First Playable Replay» / v0.5 | **v0.1 E12** per la parte deterministica (CP 12.6 aperto) + lavoro nuovo per archive/player/seek |
| Network Alpha / v0.6 | **M10** — rete e privacy (M10.2, M10.3 esistono già) |
| Production Ready Replay / v0.7 | **M11** — che ha già «replay audit» in scope |

Il replay deterministico **non è tooling futuro: è core della v0.1**, e lo è già. La domanda giusta non era
in quale versione collocarlo, ma quanta parte del §21 fosse già coperta.

---

## 8. Scenari — cosa scrivere, e dove

I dodici ScenarioId del §23 sono titoli con un assert di una riga. Il repository ha una convenzione più
forte: file JSON versionato sotto `Scenarios/Spec/`, che gira dal resolver reale e dichiara `BLOCKED` col
nome della capability mancante quando la feature non c'è — è così che `RT_Showcase_Relay_v01` esiste da
prima del codice che deve consumarlo.

Nessuno scenario `Replay.*` esiste oggi (`grep -rli replay Scenarios/` → nessun esito). Il più informativo
da scrivere per primo è `Replay.Decision.TimeoutFallback`, perché il suo oracolo è **discriminante**:

```gherkin
Dato  un Decision Boundary aperto al MicroStep 4 del turno 6, fase Move
  E   nessuna risposta entro la finestra (3,0 s, ADR-0004 §8)
Quando la policy applica il fallback
Allora il DecisionRecord contiene SelectedResponse=HOLD, Reason=Timeout
  E   nessun campo di latenza entra nell'hash
  E   due esecuzioni con latenze diverse (10 ms, 2900 ms) producono lo STESSO LogHash
```

L'ultima riga è ciò che coglie la regressione: senza, il test passa anche se il wall-clock è entrato nel
determinismo. Le verifiche che restano umane vanno nel registro PIE, non qui — la ripartizione è in
[`scenario-map.md`](../../technical/scenario-map.md).

---

## 9. Domande per l'autore — due chiuse, due aperte

### Chiuse il 2026-08-10

| # | Domanda | Risposta | Registrata in |
|---|---|---|---|
| 1 | L'ordine degli eventi entra nel contratto? (§4.1) | **Sì, con un secondo hash affiancato.** `HashTurnLog` resta invariato; `HashTurnLogOrdered` mescola nell'ordine di append | [D-062](../../decisions/RT_PDR_00_Decision_Log.md) |
| 2 | Lo schema di `FRTTurnLogEntry` si estende? (§4.2) | **Sì: `UnitId` e `TurnNumber`, entrambi fuori dall'hash** (formato v6). `D-TL-2` emendata | [D-063](../../decisions/RT_PDR_00_Decision_Log.md) |

**L'elemento che ha deciso la #1** è emerso dopo la stesura di §4.1 e la corregge in meglio: l'ordine è
**già canonico a monte**. `InstanceLess` (`RTActionQueueLibrary.cpp:10-31`) impone alla coda l'ordine totale
a cinque chiavi — `Phase → Priority → ActionId → SourceUnitId → EventSequence` — che
[D-061](../../decisions/RT_PDR_00_Decision_Log.md) dichiara in vigore, indipendente dall'ordine dell'array e
testato in `RTActionQueueTests.cpp`. Quindi un hash sensibile all'ordine di append è **anch'esso**
permutazione-invariante: il costo che §4.1 attribuiva all'opzione (b) non esiste.

Il che sposta il valore del secondo hash: non è soprattutto diagnostica, è **una verifica che oggi manca**.
L'ordine della *coda* è testato; l'ordine di *append* nel TurnLog non lo osserva nessun hash — un ciclo che
iterasse una `TMap` cambierebbe la traccia lasciando ogni hash identico.

### Ancora aperte

| # | Domanda | Perché blocca |
|---|---|---|
| 3 | **`ContentManifestHash` / `RulesVersion` si costruiscono ora o alla v0.2?** | Senza, la regola «un replay v1 non va ricalcolato con regole v2» (§8) non è implementabile, e il corpus golden è protetto solo dal fatto che i cataloghi cambiano di rado |
| 4 | **L'unità persistente è la partita (§7) o il turno?** | Con 6–12 turni la differenza in storage è trascurabile, ma il turno è già l'unità del TurnLog serializzato esistente |
| 5 | **I campi di movimento (`TransitionId`, `GraphRevision`, costo) entrano?** *(nuova, generata da D-063)* | Sono quelli che **discriminano**, quindi entrerebbero nell'hash e invaliderebbero i golden. D-063 li ha lasciati fuori di proposito: la decisione va presa comunque prima di [`#178`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/178), o il caso d'uso «black box» del §15 resta coperto a metà |

Non sono domande: nome dei tipi C++, formato binario finale, elenco dei comandi console. Si decidono in
implementazione.

---

## 10. Cosa questo documento non fa

Non modifica Feature Registry, roadmap, Scenario Map, Wiki né alcuna issue GitHub. **Tocca** il Decision Log
e i due owner del TurnLog, ma solo per registrare le due decisioni del §9 — nessun codice è stato scritto.

Restano da fare:

1. **implementare** `D-062` e `D-063` — formato **v6**, con il test di riordino che rende il secondo hash
   verificato invece che decorativo. Prima di [`#178`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/178),
   o il corpus golden nasce sul formato vecchio;
2. ADR sul replay logico canonico — sarebbe il **numero 0009** (gli 0001–0008 esistono), ma il numero si
   assegna **al merge**: due sessioni parallele hanno già collisionato su un ID in questo repository, e ne
   sta girando una adesso su questo stesso branch. Vale anche per `D-062`/`D-063`;
3. le sette issue nuove del §6, dopo il punto 1;
4. `RT-FEAT-CORE-DECISION-BOUNDARY`: aggiungere la dipendenza verso il replay (§5.2);
5. sanare lo stato di [`#81`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/81) (§6).

**Il passo unico raccomandato** è il punto 1 — non «costruire l'archive minimale» come suggerisce il §43
dell'handoff: quell'archive poggerebbe su un formato che sta per cambiare.
