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
  (`FRTGridCoord{X,Y,Layer}` int32) + `Amount` (int32). Nessun float.
- `URTTurnLogLibrary::{EntryLess, SortTurnLog, HashTurnLog}` (FNV-1a 32-bit, permutazione-invariante) `ff5e079`.

> ⚠️ **Allineamento 2026-08-08 — il formato è avanzato a `v4`.** Questa sezione descrive la **v2**, che era il
> formato al momento della stesura. Da allora `ERTTurnLogFormatVersion` è cresciuto **due volte**, sempre in
> modo retrocompatibile:
>
> | Versione | Cosa aggiunge | Le tracce precedenti |
> |---|---|---|
> | `Initial = 1` | header + voci, senza checksum | mai persistita su file |
> | `WithChecksum = 2` | checksum FNV del payload in coda | — |
> | `WithActionId = 3` | `ActionId` per voce (CP 5.5): `uint16` di lunghezza + byte UTF-8 in coda alla voce — **primo campo a lunghezza variabile** | leggibili, `ActionId` vuoto: che è esattamente ciò che quei byte dicevano |
> | `WithFormatId = 4` | `FormatId` nell'**header** (CP 10.3), dopo i flags. Sta nell'header perché nelle voci sarebbe una costante ripetuta N volte, e **non entra nell'hash**: includervi un campo di contesto invaliderebbe in blocco ogni hash golden | leggibili, `FormatId` neutro |
>
> Quindi «il loader accetta solo v2» **non vale più**: accetta v2, v3 e v4, e rifiuta le versioni sconosciute
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

## 8. Prossimo slice possibile

- **Hazard/status nel TurnLog** — reason di cella (lava/terreno) e status (Root/Slow/Reveal) con relativo
  outcome; tocca `RTTurnManager`/resolver (wiring, verifica in PIE), meno puro dei precedenti.

## 9. Riferimenti

- [`spec-turnlog.md`](spec-turnlog.md) — slice precedente (reason codes + hash).
- Codice: `Source/RefactorTactics/Turn/RTTurnLog.h`, `RTTurnLogLibrary.h/.cpp`, `Tests/RTTurnLogSerializationTests.cpp`.
