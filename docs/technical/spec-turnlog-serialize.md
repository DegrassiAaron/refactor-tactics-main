# Spec — Serializzazione TurnLog versionata (SR + SR.file)

> Slice successivo a [`spec-turnlog.md`](spec-turnlog.md) §11: chiude il ciclo determinismo/replay
> (KPI [`roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md) «Replay divergence = 0»). **Puro C++, TDD.**
> **Stato: implementato** — `SR` (in-memory, merge `8b6dc32`) + `SR.file` (checksum v2 + I/O su file,
> branch `feat/turnlog-file` → `main`). Suite **126/126**.

## 1. Obiettivo & scope

Serializzare/deserializzare il TurnLog in un **buffer binario versionato** e in **forma canonica**, con
round-trip verificato dall'hash (`URTTurnLogLibrary::HashTurnLog`), **persistenza su file** e **checksum**
che rileva la corruzione del contenuto. Chiude l'invariante #4 («ogni formato serializzato è versionato»)
e porta il KPI «Replay divergence = 0» a ✅ (traccia salvabile, ricaricabile e riconfrontabile).

**In scope:** serializzazione in-memory (`TArray<uint8>`), header versionato, forma canonica
(permutazione-invariante come l'hash), **checksum del payload**, **save/load su file**, fail-closed su input corrotto.
**Fuori scope (dichiarato):** reason di hazard/status nel TurnLog (slice successivo).

## 2. Stato di partenza (verificato sul codice)

- `FRTTurnLogEntry` = 6 campi **interi**: `Phase`/`Category`/`Outcome` (uint8) + `SrcCell`/`TgtCell`
  (tre `int32` per cella: **assiali `q, r, Layer`** in esagonale — erano offset `X, Y, Layer` nel quadrato,
  e il marcatore `ERTLogTopology` nei flags dell'header distingue i due) + `Amount` (int32). Nessun float.
- `URTTurnLogLibrary::{EntryLess, SortTurnLog, HashTurnLog}` (FNV-1a 32-bit, permutazione-invariante) `ff5e079`.

> ⚠️ **Allineamento 2026-08-10 — il formato in codice è `v6`.** Questa
> sezione descrive la **v2**, che era il formato al momento della stesura. Da allora `ERTTurnLogFormatVersion`
> è cresciuto **tre volte**, sempre in modo retrocompatibile:
>
> | Versione | Cosa aggiunge | Nell'hash | Le tracce precedenti |
> |---|---|---|---|
> | `Initial = 1` | header + voci, senza checksum | — | mai persistita su file |
> | `WithChecksum = 2` | checksum FNV del payload in coda | — | — |
> | `WithActionId = 3` | `ActionId` per voce (CP 5.5): `uint16` di lunghezza + byte UTF-8 in coda alla voce — **primo campo a lunghezza variabile** | **sì** | leggibili, `ActionId` vuoto: che è esattamente ciò che quei byte dicevano |
> | `WithFormatId = 4` | `FormatId` nell'**header** (CP 10.3), dopo i flags. Sta nell'header perché nelle voci sarebbe una costante ripetuta N volte | no | leggibili, `FormatId` neutro |
> | `WithBaseActionId = 5` | `BaseActionId` per voce ([#354](https://github.com/DegrassiAaron/refactor-tactics-main/issues/354)): l'azione generica di cui `ActionId` è un profilo, scritta come l'ActionId | no — è una **funzione** di `ActionId`, che c'è già | leggibili, `BaseActionId` vuoto |
> | `WithUnitId = 6` | `UnitId`, `TurnNumber` ([D-063](../decisions/RT_PDR_00_Decision_Log.md)) e `GraphRevision` ([D-067](../decisions/RT_PDR_00_Decision_Log.md)): tre int32 in coda alla voce, dopo `BaseActionId` | i primi due **no** — rendono la traccia spiegabile, non la discriminano. `GraphRevision` **sì**: due tracce possono differire solo per lei, ed è un'altra partita | leggibili, campi a `0` (`UnitId = 0` = nessuna unità) |
>
> ⚠️ **Ogni campo che questo formato SCRIVE deve stare anche in `EntryLess`.** La forma canonica è definita
> dall'ordinamento: un campo serializzato che il confronto non guarda lascia due voci a pari merito, e
> l'ordine fra loro lo decide `TArray::Sort`, che **non è stabile** — due inserimenti diversi produrrebbero
> due file diversi con lo stesso contenuto. È esattamente ciò che `D-SR-1` promette non accada, ed è successo:
> `UnitId` e `TurnNumber` sono arrivati nella v6 senza entrare nel confronto. Corretto in
> [D-067](../decisions/RT_PDR_00_Decision_Log.md), pinnato da `TurnLog.CanonicalOrderCoversSerializedFields`.
> L'unica eccezione legittima è un campo che **non può produrre pareggi** perché funzione di un altro:
> `BaseActionId` è funzione di `ActionId`, e per questo resta fuori.
>
> ⚠️ **L'hash ordinato di [D-062](../decisions/RT_PDR_00_Decision_Log.md) NON è in questo formato, ed è
> deliberato.** La prima stesura di D-062 diceva di metterlo nell'header: sarebbe stato un errore, perché i
> byte sono in forma canonica (`D-SR-1`) e un hash dell'ordine d'inserimento li renderebbe dipendenti da
> quell'ordine — cioè romperebbe `SerializeCanonicalPermutationInvariant`. Quel valore appartiene all'header
> del **Replay Archive**. Vedi §6.
>
> **Il criterio è sempre lo stesso, ed è quello che protegge il corpus golden**: un campo entra nell'hash **se
> e solo se** due tracce possono differire *solo* per quel campo. Un campo di contesto o derivabile che vi
> entrasse invaliderebbe in blocco ogni hash già prodotto.
>
> Quindi «il loader accetta solo v2» **non vale più**: accetta da v2 in su, e rifiuta le versioni sconosciute
> invece di interpretare byte arbitrari (invariante #4).
>
> **Aggiungere una categoria o un outcome non richiede una nuova versione**, e non è prudenza: i valori nuovi
> sono **accodati** e viaggiano come `uint8`, quindi i file già scritti restano leggibili. Si incrementa la
> versione solo quando cambia il **layout** di header o voce. Inserire un valore *in mezzo* rinumererebbe
> `Combat` e riscriverebbe il significato dei file esistenti: quello sì.

## 3. Formato binario (versione 2, `WithChecksum`) — *storico, vedi il riquadro sopra*

Little-endian **esplicito** (indipendente dall'endianness della piattaforma; non usa `FArchive`):

| Offset | Campo | Tipo |
|---|---|---|
| 0 | magic `'RTTL'` (byte `52 54 54 4C`) | uint32 LE |
| 4 | versione (`ERTTurnLogFormatVersion::WithChecksum = 2`) | uint16 LE |
| 6 | **flags = topologia** (`ERTLogTopology`: `0` Square, `1` Hex; altri valori → rifiuto) | uint16 LE |
| 8 | conteggio voci | uint32 LE |
| 12.. | N voci (forma canonica), 31 byte/voce | — |
| coda | **checksum FNV** di tutto ciò che precede (header + voci) | uint32 LE |

Il campo a offset 6 era `reserved` e scritto a 0; da **H6.3** dichiara la topologia delle celle (le voci portano
3 interi: offset `X,Y,Layer` nel quadrato, assiali `q,r,Layer` nell'esagonale). `Square = 0` mantiene i file
esistenti leggibili e i byte del quadrato invariati. Senza il marcatore due tracce di topologia diversa sarebbero
indistinguibili e un confronto incrociato darebbe un falso «nessuna divergenza».

Voce (31 byte): `Phase`(1) + `Category`(1) + `Outcome`(1) + `SrcCell.X/Y/Layer` (3×int32 LE) +
`TgtCell.X/Y/Layer` (3×int32 LE) + `Amount` (int32 LE). Voci scritte **dopo `SortTurnLog`** → byte
**permutazione-invarianti**. La v1 (`Initial`, senza checksum) non è mai stata persistita → il loader accetta solo v2.

## 4. API (in `URTTurnLogLibrary`)

- `static TArray<uint8> SerializeTurnLog(const TArray<FRTTurnLogEntry>&, ERTLogTopology = Square)` — forma
  canonica + topologia nei flags + checksum.
- `static bool DeserializeTurnLog(const TArray<uint8>&, TArray<FRTTurnLogEntry>& Out, ERTLogTopology* OutTopology = nullptr)`
  — **fail-closed** (`false`, `Out` svuotato) su magic/versione/**topologia sconosciuta**/troncamento/**checksum
  mismatch**; bounds-check in ogni lettura.
- `static bool SaveTurnLogToFile(const FString& Path, const TArray<FRTTurnLogEntry>&, ERTLogTopology = Square)` —
  wrapper `FFileHelper::SaveArrayToFile`.
- `static bool LoadTurnLogFromFile(const FString& Path, TArray<FRTTurnLogEntry>& Out, ERTLogTopology* OutTopology = nullptr)`
  — wrapper `FFileHelper::LoadFileToArray` + `DeserializeTurnLog`; `false` se il file manca o è invalido/corrotto.

Contratto: `HashTurnLog(in) == HashTurnLog(Load(Save(in)))`.

## 5. Test — `Tests/RTTurnLogSerializationTests.cpp` (TDD RED→GREEN)

| Test | Comportamento |
|---|---|
| `SerializeRoundTripPreservesHash` | il round-trip in-memory preserva l'hash |
| `SerializeCanonicalPermutationInvariant` | stesse voci in ordine diverso → byte identici |
| `DeserializeRejectsBadMagic` | magic errato → `false` |
| `DeserializeRejectsUnknownVersion` | versione ≠ 2 → `false` |
| `SerializeEmptyRoundTrip` | log vuoto → solo header+checksum, round-trip ok |
| `DeserializeRejectsTruncated` | buffer troncato → `false`, nessun crash |
| `DeserializeDetectsPayloadCorruption` | bit-flip nel payload (magic/versione validi) → `false` (checksum) |
| `FileRoundTripPreservesHash` | save→load su file preserva l'hash |
| `LoadMissingFileFails` | file inesistente → `false`, output svuotato |
| `LoadCorruptedFileFails` | file valido corrotto su disco → `false` (checksum) |

Comportamenti sostanziali (round-trip, canonicità, rifiuto magic/versione, **checksum**, **file round-trip**)
guidati da un **RED reale**; bounds-check, caso vuoto, file mancante/corrotto = caratterizzazione dichiarata.

## 6. Decisioni

- **D-SR-1** — **forma canonica** (ordinata) → byte permutazione-invarianti; replay confrontabili byte-per-byte.
  ⚠️ **Conseguenza di [D-062](../decisions/RT_PDR_00_Decision_Log.md) (2026-08-10)**: siccome i byte sono
  **ordinati**, la serializzazione **perde l'ordine di append**. L'hash ordinato quindi **non è ricalcolabile
  da un file**: si calcola in memoria prima di scrivere, e va conservato nell'header del **Replay Archive** —
  **non** in quello del TurnLog, che deve restare permutazione-invariante. Il verificatore lo confronta con
  quello prodotto dalla ri-simulazione, mai con uno ricalcolato dal file: un «ricalcola e confronta» sui byte
  salvati confronterebbe l'ordine canonico con sé stesso e **passerebbe sempre**.
  Pinnato da `RefactorTactics.TurnLog.OrderedHashIsLostBySerialization`.
- **D-SR-2** — **little-endian esplicito** (non `FArchive`) per determinismo/portabilità.
- **D-SR-3** — versione `uint16` **non-UENUM**; loader **fail-closed** su versioni ignote.
- **D-SR-4** — **checksum FNV del payload in coda** (`WithChecksum = 2`): rileva la corruzione del contenuto
  che magic/versione non catturano. La v1 non è mai stata persistita → il loader accetta solo v2 (nessun problema di retrocompatibilità).
- **D-SR-5** — **file I/O = thin wrapper** su `FFileHelper` (Save/Load); l'integrità è delegata al checksum.

## 7. Definition of Done (raggiunta)

☑ TDD RED→GREEN per ogni comportamento sostanziale · ☑ suite **126/126** (116 preesistenti + 10 SR) ·
☑ build target Editor **Succeeded** (editor chiuso; Live Coding blocca la build CLI) · ☑ solo interi
(invariante #4) · ☑ i test file puliscono `Saved/` (nessun residuo) · ☑ spec/roadmap aggiornate.

### Estensione H6.3 (2026-08-05)

Il campo flags porta ora la **topologia** (`D-SR-6`): il formato dichiara se le celle sono quadrate o esagonali,
il loader rifiuta i valori sconosciuti (fail-closed) e restituisce la topologia letta. `SerializeTurnLog` col
default `Square` produce **gli stessi byte di prima** (test `RefactorTactics.TurnLog.SquareBytesUnchanged`).
La topologia **non** entra nell'hash: due esecuzioni della stessa partita condividono la topologia per
costruzione, e così l'hash del quadrato resta invariato. Vedi [`h6-hex-sim-spec.md`](h6-hex-sim-spec.md) §H6.3.

## 7-bis. Il Replay Archive — dove finiscono le tracce (2026-08-10, `#469`)

`D-077` nomina **questo documento** come owner del formato dell'archivio, e da oggi l'archivio esiste.

```
Replays/<MatchId>/
  match.rtmanifest    <- header di partita, JSON
  turn-001.rtlog      <- SerializeTurnLog, byte invariati rispetto al §3
  turn-002.rtlog
```

**Le tracce non cambiano.** Il recorder chiama `SerializeTurnLog` e ne scrive il risultato: non c'è un
secondo serializzatore, ed è l'unico modo di rendere *vero* il criterio «byte-identiche» invece di
ripromettersi di tenere due implementazioni allineate. Il test che lo pinna è
`Replay.Recorder.TurnBytesMatchSerializeTurnLog`.

**Il manifest è JSON**, non binario, perché è **metadati** e non payload: un archivio rotto lo si diagnostica
leggendo l'header a occhio, e `Json` era già una dipendenza del modulo. Versionato da
`ERTReplayManifestVersion` (oggi `Initial = 1`) e **fail-closed** sulle versioni sconosciute, con la stessa
convenzione del formato binario: rifiutare invece di interpretare campi arbitrari.

| Campo | Note |
|---|---|
| `MatchId` | `FGuid` generato all'avvio. **Fuori da ogni hash** (`D-077`): identifica la registrazione, non il contenuto |
| `FormatId` · `HexTopology` | la stessa identità che l'header della traccia porta |
| `OrderedHashPerTurn` | `HashTurnLogOrdered` per turno — la casa che `D-062` gli aveva assegnato e che prima non esisteva |
| `FinalStateHash` | checksum di fine partita; `0` finché non è calcolato |
| `Outcome` · `WallClockSeconds` | il wall-clock vive **solo** qui, mai in un campo che entri in un hash |
| `Closed` · `TurnCount` | vedi sotto |

> ⚠️ **Un archivio parziale non ha un campo che lo dichiari: è l'assenza della chiusura.** Il recorder scrive
> la traccia a **ogni turno** e chiude il manifest solo a partita conclusa, quindi un manifest con
> `Closed = false` *è* la dichiarazione di parzialità. Niente flag da ricordarsi di scrivere nel percorso di
> crash, che è esattamente il percorso in cui nessuno si ricorda niente. Per la stessa ragione il manifest si
> riscrive con **temporaneo + move**: viene sovrascritto a ogni turno, e un crash a metà scrittura
> corromperebbe un file che era valido — proprio nel momento per cui il recorder esiste.

**Cosa non c'è ancora**: i campi di compatibilità `ContentManifestHash`/`RulesVersion`, **rinviati alla v0.2**
da `D-083` (issue `#413`). Il manifest è versionato, quindi li accoglierà senza rompere gli archivi scritti
prima.

---

## 8. Prossimo slice possibile

- **Hazard/status nel TurnLog** — reason di cella (lava/terreno) e status (Root/Slow/Reveal) con relativo
  outcome; tocca `RTTurnManager`/resolver (wiring, verifica in PIE), meno puro dei precedenti.

## 9. Riferimenti

- [`spec-turnlog.md`](spec-turnlog.md) — slice precedente (reason codes + hash).
- Codice: `Source/RefactorTactics/Turn/RTTurnLog.h`, `RTTurnLogLibrary.h/.cpp`, `Tests/RTTurnLogSerializationTests.cpp`.
