# Replay Archive — le sei issue, specificate

> `HISTORICAL` · **Stato**: **consumato**. Le sei issue sono state create il 2026-08-10 e **cinque su sei sono
> chiuse**; le quattro decisioni che questo piano dichiarava aperte sono state prese lo stesso giorno.
> Il documento resta come **fotografia della pianificazione**, non come stato: per lo stato aprire le issue
> (elenco in fondo, «Com'è andata»). Le sue righe al presente vanno lette come «al 2026-08-10».
>
> **Scritto su**: `main` a `781e586` · **Panel**: Wiegers (requisiti), Adzic (esempi), Fowler (confini),
> Nygard (compatibilità e guasti), Cockburn (attore primario), Crispin (testabilità)
> **Fonte**: [conflict report replay](replay-system-conflict-report-2026-08-10.md) §6 — le sette voci
> genuinamente nuove, meno l'ADR che è un prerequisito e non una di queste sei.
> **Regola applicata**: nessuna issue dichiara un criterio che il repository non sappia misurare oggi.

---

## 0. Il verdetto del panel, prima delle specifiche

**WIEGERS**: «Quattro di queste sei non sono specificabili adesso, e non per pigrizia: dipendono da
decisioni aperte. Una specifica scritta sopra una decisione mancante non è una specifica, è un'ipotesi con
l'aspetto di un requisito.»

| Decisione aperta | Blocca | Perché |
|---|---|---|
| **ADR replay logico canonico** | R1, R3 | Senza, «cosa è autorevole» non è deciso e il recorder non sa cosa registrare |
| **`ContentManifestHash` / `RulesVersion`** (§9.3) | R2 | Il §8 dell'handoff — «un replay v1 non va ricalcolato con regole v2» — non è implementabile: quelle hash non esistono |
| **Unità persistente: partita o turno** (§9.4) | R1, R5 | Decide se l'archivio è un file per partita o per turno, cioè la forma di tutto il resto |
| **Identità stabile dell'istanza di unità** ([#405](https://github.com/DegrassiAaron/refactor-tactics-main/issues/405)) | R6 | Una UI che segue un'unità ha bisogno di poterla nominare fra un turno e l'altro |

**Due sono specificabili subito**: **R4** (seek) e **R5** (Match History), perché poggiano solo su ciò che
esiste. Le altre quattro si aprono come issue *con la dipendenza dichiarata nel corpo*, non come lavoro pronto.

**FOWLER**: «E c'è un confine da fissare prima di scrivere le sei: `ReplayPlayer` non chiama mai il resolver,
`ReplayVerifier` è l'unico che lo fa. Se non lo si scrive nell'ADR, la prima implementazione che ha fretta lo
attraversa e nessun test se ne accorge — `REPLAY-04` del risk register è esattamente questo.»

---

## R1 — Replay Archive e Recorder

**Attore primario** (Cockburn): il *sistema di QA*, non il giocatore. La prima utilità è il corpus, non il
playback.

**Why**. Il TurnLog serializzato esiste per turno, ma niente lo raccoglie in una partita: `FRTTestResult`
tiene le tracce solo per la durata di uno scenario. Senza un archivio, «apri il replay X» non ha un
referente.

**Scope**. Registrare, durante una partita reale: header versionato, snapshot iniziale, e per ogni turno
`AcceptedIntents`, TurnLog serializzato, hash canonico e hash ordinato.

**Out of scope**. Nessuna UI. Nessuna decisione runtime (non esistono ancora). Nessuna persistenza remota.

**Acceptance criteria** (Wiegers — misurabili sul repository di oggi):
- una partita di N turni produce un archivio con **esattamente N** blocchi-turno;
- ogni blocco-turno rilegge il proprio TurnLog con `DeserializeTurnLog` e ne riproduce l'hash canonico;
- l'hash **ordinato** è registrato nell'header di ogni blocco-turno — è l'unico posto dove può stare, perché
  i byte del TurnLog sono in forma canonica e lo perdono ([D-062](../../decisions/RT_PDR_00_Decision_Log.md));
- registrare **non cambia** l'esito: stessa partita con e senza recorder ⇒ stesso `StateHash`.

**CRISPIN**: «L'ultimo criterio è il solo che valga davvero. Gli altri tre verificano che il formato regga;
quello verifica che l'osservazione non alteri l'osservato, ed è il difetto che poi costa mesi.»

**Dipende da**: ADR · decisione §9.4 (partita o turno).

---

## R2 — Serializzazione dell'archivio e compatibilità

**Why**. Un archivio riletto con regole diverse racconta un'altra partita e non lo dice.

**Scope**. Formato versionato per l'archivio (non per il TurnLog, che ha già il suo), e un verdetto di
compatibilità esplicito alla lettura.

**NYGARD**: «Il §8 dell'handoff chiede di rifiutare un replay v1 sotto regole v2. Oggi **non è
implementabile**: `RulesVersion`, `ContentManifestHash` e `ResolverConfigHash` non esistono nel codice —
zero occorrenze. `RT-FEAT-DATA-HASH` è `RELEASE_READY` ma copre la **geometria della mappa** e la traccia,
non un manifest di regole. Questa issue va aperta dichiarando che il suo prerequisito è costruire quelle
hash, altrimenti il primo che la prende scopre a metà che manca il dato.»

**Acceptance criteria**:
- l'archivio dichiara la propria versione di formato e la rifiuta se sconosciuta (**fail-closed**, come
  `DeserializeTurnLog`);
- un archivio scritto con un manifest diverso produce un verdetto **esplicito** di incompatibilità, mai un
  ricalcolo silenzioso;
- le versioni precedenti restano **leggibili** finché il layout non cambia — la catena che il TurnLog ha
  tenuto da v2 a v6.

**Dipende da**: la decisione §9.3 (costruire o rinviare le hash di regole/contenuti). **Bloccata.**

---

## R3 — Replay Player

**Why**. Riprodurre una partita archiviata senza ri-deciderla.

**Scope**. Consuma il TurnLog e i checkpoint; guida la presentazione.

**Out of scope, e non è un dettaglio**: il Player **non** chiama il resolver, non calcola collisioni, danni,
reazioni, KO, legalità dei percorsi né targeting. Riproduce eventi già risolti.

**Acceptance criteria**:
- riprodurre un archivio produce la stessa sequenza di eventi canonici della partita originale;
- **un test d'architettura** verifica che il Player non raggiunga il resolver — è `REPLAY-04`, e senza quel
  test nessuno si accorgerebbe della violazione finché un replay non diverge.

**ADZIC**: «Il secondo criterio ha bisogno di un esempio, o resta un'intenzione:

```gherkin
Dato  un archivio in cui al turno 4 un colpo ha inflitto 18 danni
Quando lo si riproduce con un catalogo in cui la stessa abilità ne infligge 20
Allora il playback mostra 18
  E   nessuna chiamata al resolver compare nella traccia di esecuzione
```

Il primo `Allora` è ciò che distingue un player da un simulatore. Se mostra 20, il Player sta ricalcolando.»

**Dipende da**: R1 · ADR.

---

## R4 — Seek per turno e per fase

**Specificabile subito.** Non dipende da nulla di aperto.

**Why**. Aprire un replay al turno 8 senza riprodurre i turni 1–7.

**Scope**. Salto al turno; salto alla fase dentro il turno.

**Out of scope, con motivo**: il **micro-step**. `FRTTurnLogEntry` non ha né indice di sequenza né
micro-step, e l'ordine delle voci in un TurnLog serializzato è la chiave di sort, non l'ordine di emissione.
Il seek si ferma alla fase finché lo schema non cambia — è la conseguenza registrata nel conflict report §4.1.

**Acceptance criteria** (Crispin — l'oracolo è un'equivalenza, non un'ispezione):
- `playback completo fino al turno N` e `seek al turno N` producono **lo stesso stato** e la stessa coda di
  eventi residui;
- un seek a un turno inesistente fallisce in modo esplicito, non silenzioso.

---

## R5 — Match History

**Specificabile subito**, con una riserva.

**Attore primario**: il giocatore che cerca una partita, non il sistema.

**Why**. La lista delle partite non deve caricare gli archivi per mostrarli.

**Scope**. Indice di metadati separato dal payload: id, data, modo, mappa, squadre, esito, turni, durata
wall-clock, versione delle regole, disponibilità del replay.

**Acceptance criteria**:
- aprire la lista **non** legge nessun payload di archivio — verificabile contando le letture di file;
- il wall-clock compare **solo** qui e mai in un campo che entri in un hash.

**WIEGERS**: «La riserva è il primo campo: “id della partita”. Non esiste. È lo stesso vuoto di
[#405](https://github.com/DegrassiAaron/refactor-tactics-main/issues/405) un piano più su — lì mancava
l'identità dell'unità, qui manca quella della partita. Va aperta come parte di questa issue, non scoperta
dopo.»

**Dipende da**: R1 per la disponibilità del replay; la parte metadati è indipendente.

---

## R6 — UI del replay e Combat Log

**Why**. Rendere leggibile ciò che l'archivio contiene.

**Scope**. Controlli di trasporto, timeline per turno e fase, filtri, e un Combat Log che **legge i reason
code canonici invece di ricalcolare**.

**Acceptance criteria**:
- selezionare un evento mostra la spiegazione costruita da `DescribeEntry`, non da un calcolo dell'UI;
- la timeline raggruppa per **fase**, non per micro-step (vedi R4).

⚠️ **Due dipendenze che la specifica deve dichiarare invece di scoprire**:
1. i reason code sono **parziali**: `ActionId` oggi lo popolano solo le voci di categoria `Reaction`, e
   completarlo sul combattimento è **CP 11.3**, aperto. Una UI che promette di spiegare ogni evento
   promette più di quanto la traccia contenga;
2. «segui questa unità» richiede un'identità stabile dell'unità, che non esiste
   ([#405](https://github.com/DegrassiAaron/refactor-tactics-main/issues/405)).

**Dipende da**: R1 · R4 · CP 11.3 · #405.

---

## Sintesi e ordine

| ID | Titolo | Stato | Sbloccata da |
|---|---|---|---|
| **R4** | Seek per turno e per fase | **pronta** | — |
| **R5** | Match History (metadati) | **pronta**, con l'id partita da definire dentro | — |
| **R1** | Replay Archive e Recorder | bloccata | ADR · §9.4 |
| **R3** | Replay Player | bloccata | ADR · R1 |
| **R2** | Serializzazione e compatibilità | bloccata | §9.3 |
| **R6** | UI e Combat Log | bloccata | R1 · R4 · CP 11.3 · #405 |

**COCKBURN, in chiusura**: «Due pronte su sei non è un fallimento della pianificazione: è la pianificazione
che funziona. Il costo di scoprirlo adesso è una tabella; scoprirlo a metà implementazione sono due
riscritture.»

**Il passo unico raccomandato**: aprire **R4** e **R5**, e trasformare le tre decisioni aperte (§9.3, §9.4,
ADR) in altrettante issue di decisione — non di implementazione. Le altre quattro si aprono quando quelle
chiudono.

---

## Com'è andata (aggiornato il 2026-08-11)

Il passo raccomandato è stato eseguito, e le quattro decisioni si sono chiuse **lo stesso giorno** invece che
nell'ordine previsto: da lì le quattro issue «bloccate» si sono sbloccate tutte insieme.

| Decisione dichiarata aperta | Issue | Esito |
|---|---|---|
| ADR replay logico canonico | [#412](https://github.com/DegrassiAaron/refactor-tactics-main/issues/412) | ✅ [ADR-0009](../../decisions/adr-0009-replay-logico-canonico.md) · D-078 |
| `ContentManifestHash` / `RulesVersion` | [#413](https://github.com/DegrassiAaron/refactor-tactics-main/issues/413) | ✅ **D-083 le rinvia alla v0.2**: in v0.1 il corpus vive nello stesso repository delle regole |
| Unità persistente: partita o turno | [#414](https://github.com/DegrassiAaron/refactor-tactics-main/issues/414) | ✅ **D-077**: manifest per partita, traccia per turno |
| Identità stabile dell'unità | [#405](https://github.com/DegrassiAaron/refactor-tactics-main/issues/405) | ✅ D-084 |

| ID | Issue | Stato al 2026-08-11 |
|---|---|---|
| **R1** Archivio e Recorder | [#469](https://github.com/DegrassiAaron/refactor-tactics-main/issues/469) | ✅ chiusa — `RTReplayRecorderLibrary` |
| **R2** Serializzazione e compatibilità | [#471](https://github.com/DegrassiAaron/refactor-tactics-main/issues/471) | ✅ chiusa — `RTReplayManifest` |
| **R3** Player | [#470](https://github.com/DegrassiAaron/refactor-tactics-main/issues/470) | ✅ chiusa — `RTReplayPlayerLibrary`, e con `Replay.Player.RunsWithoutResolver` **`REPLAY-04` si chiude** |
| **R4** Seek | [#415](https://github.com/DegrassiAaron/refactor-tactics-main/issues/415) | ✅ chiusa — `RTReplaySeekLibrary` |
| **R5** Match History | [#416](https://github.com/DegrassiAaron/refactor-tactics-main/issues/416) | ✅ chiusa — `RTMatchHistoryLibrary` |
| **R6** UI e Combat Log | [#472](https://github.com/DegrassiAaron/refactor-tactics-main/issues/472) | 🔓 **aperta**, `post-v0.1` — le dipendenze dichiarate sono tutte chiuse |

**Due scostamenti dalla specifica, che vale la pena aver registrato:**

1. **R6 ha perso il Combat Log** rispetto a questo piano: la issue lo ha scorporato e tenuto solo trasporto e
   posizione. Il pezzo tolto non è sparito — resta **CP 11.3** ([#79](https://github.com/DegrassiAaron/refactor-tactics-main/issues/79)),
   aperta e dentro la v0.1. Questo piano lo dava come dipendenza di R6 e aveva ragione a dichiararlo.
2. La riserva di WIEGERS su R5 — «l'id della partita non esiste» — è stata chiusa **dentro** R1/R5 come il
   piano chiedeva, e l'id vive nel manifest **fuori dagli hash**
   (`Replay.Producer.MatchIdDoesNotEnterHashes`).
