# Spec — Serializzazione TurnLog versionata (SR)

> Slice successivo a [`spec-turnlog.md`](spec-turnlog.md) §11: chiude il ciclo determinismo/replay
> (KPI [`roadmap-checkpoint.md`](roadmap-checkpoint.md) «Replay divergence = 0»). **Puro C++, TDD.**
> **Stato: implementato** — branch `feat/turnlog-serialize` → `main` (merge `8b6dc32`). Suite **122/122**.

## 1. Obiettivo & scope

Serializzare/deserializzare il TurnLog in un **buffer binario versionato** e in **forma canonica**,
con round-trip verificato dall'hash già esistente (`URTTurnLogLibrary::HashTurnLog`). Chiude
l'invariante #4 («ogni formato serializzato è versionato») per il TurnLog e trasforma il KPI
«Replay divergence = 0» da 🟡 (determinismo by-design + hash) a ✅ (traccia serializzabile e riconfrontabile).

**In scope:** serializzazione in-memory (`TArray<uint8>`), header versionato, forma canonica
(permutazione-invariante come l'hash), fail-closed su input corrotto.
**Fuori scope (dichiarato):** I/O su file (`Save/LoadTurnLogToFile`), checksum nell'header, reason di
hazard/status (slice successivi).

## 2. Stato di partenza (verificato sul codice)

- `FRTTurnLogEntry` = 6 campi **interi**: `Phase`/`Category`/`Outcome` (uint8) + `SrcCell`/`TgtCell`
  (`FRTGridCoord{X,Y,Layer}` int32) + `Amount` (int32). Nessun float.
- `URTTurnLogLibrary::{EntryLess, SortTurnLog, HashTurnLog}` (FNV-1a 32-bit, permutazione-invariante)
  già presenti (`ff5e079`). La serializzazione mescola **gli stessi 10 interi** dell'hash → round-trip fedele al replay.

## 3. Formato binario (versione 1)

Little-endian **esplicito** (indipendente dall'endianness della piattaforma; non usa `FArchive`):

| Offset | Campo | Tipo |
|---|---|---|
| 0 | magic `'RTTL'` (byte `52 54 54 4C`) | uint32 LE |
| 4 | versione (`ERTTurnLogFormatVersion::Initial = 1`) | uint16 LE |
| 6 | reserved/flags (spazio per estensioni) | uint16 LE |
| 8 | conteggio voci | uint32 LE |
| 12.. | N voci (forma canonica) | 31 byte/voce |

Voce (31 byte): `Phase`(1) + `Category`(1) + `Outcome`(1) + `SrcCell.X/Y/Layer` (3×int32 LE) +
`TgtCell.X/Y/Layer` (3×int32 LE) + `Amount` (int32 LE).

Le voci sono scritte **dopo `SortTurnLog`** → byte **permutazione-invarianti** (come l'hash).

## 4. API (in `URTTurnLogLibrary`)

- `static TArray<uint8> SerializeTurnLog(const TArray<FRTTurnLogEntry>&)` — forma canonica.
- `static bool DeserializeTurnLog(const TArray<uint8>&, TArray<FRTTurnLogEntry>& Out)` —
  **fail-closed**: `false` (con `Out` svuotato) su magic/versione sconosciuti o buffer troncato;
  nessun accesso fuori dal buffer (bounds-check in ogni lettura).

Contratto: `HashTurnLog(in) == HashTurnLog(Deserialize(Serialize(in)))`.

## 5. Test — `Tests/RTTurnLogSerializationTests.cpp` (TDD RED→GREEN)

| Test | Comportamento |
|---|---|
| `SerializeRoundTripPreservesHash` | il round-trip preserva l'hash |
| `SerializeCanonicalPermutationInvariant` | stesse voci in ordine diverso → byte identici |
| `DeserializeRejectsBadMagic` | magic errato → `false`, `Out` vuoto |
| `DeserializeRejectsUnknownVersion` | versione sconosciuta → `false` |
| `SerializeEmptyRoundTrip` | log vuoto → solo header, round-trip ok |
| `DeserializeRejectsTruncated` | buffer troncato → `false`, nessun crash |

I comportamenti sostanziali (round-trip, canonicità, rifiuto magic/versione) sono stati guidati da un
**RED reale** prima dell'implementazione; i bounds-check difensivi e il caso vuoto sono coperti come
caratterizzazione dichiarata.

## 6. Decisioni

- **D-SR-1** — **forma canonica** (ordinata) nella serializzazione → byte permutazione-invarianti,
  coerente con l'hash; i replay diventano confrontabili/deduplicabili byte-per-byte.
- **D-SR-2** — **little-endian esplicito** (non `FArchive`) per determinismo/portabilità cross-macchina.
- **D-SR-3** — versione come `uint16` **non-UENUM** (fuori dai vincoli UHT del `BlueprintType` uint8);
  loader **fail-closed** su versioni ignote invece di interpretare byte arbitrari.
- **D-SR-4** — **I/O su file** e **checksum** nell'header rimandati a uno slice successivo.

## 7. Definition of Done (raggiunta)

☑ TDD RED→GREEN per ogni comportamento sostanziale · ☑ suite **122/122** (116 preesistenti + 6 nuovi) ·
☑ build target Editor **Succeeded** · ☑ solo interi (invariante #4) · ☑ spec/roadmap aggiornate ·
☑ commit isolato `dcd7ce3` → merge `8b6dc32` in `main`, nessun file generato/segreto.

## 8. Prossimi slice possibili

- **SR.file** — `Save/LoadTurnLogToFile` (`FFileHelper`) + eventuale **checksum FNV** nell'header (rileva corruzione al load).
- **Hazard/status nel TurnLog** — reason di cella (lava/terreno) e status (Root/Slow/Reveal) con relativo outcome.

## 9. Riferimenti

- [`spec-turnlog.md`](spec-turnlog.md) — slice precedente (reason codes + hash).
- Codice: `Source/RefactorTactics/Turn/RTTurnLog.h`, `RTTurnLogLibrary.h/.cpp`, `Tests/RTTurnLogSerializationTests.cpp`.
