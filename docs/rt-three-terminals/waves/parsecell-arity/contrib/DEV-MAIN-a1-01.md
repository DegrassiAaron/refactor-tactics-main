=== RT3 CONTRIB ===

FROM:            DEV-MAIN:a1
TO:              DEV-LEAD
FEATURE:         ParseCell — arita' limitata e ramo di rifiuto misurato
WAVE_ID:         parsecell-arity/1
CREATED:         2026-09-05 19:54
SUPERSEDES:      none
BASE_SHA:        39f3ec95
PRODUCED_SHA:    = BASE_SHA (non committo: consolida DEV-LEAD)
WORKTREE:        uncommitted
SEED_SOURCE:     none — nessun RNG introdotto
ASSIGNED_SCOPE:  Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp
WRITE_SET:       Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp
                 docs/rt-three-terminals/waves/parsecell-arity/contrib/DEV-MAIN-a1-01.md (questo contributo)

## IMPLEMENTED

`ParseCell` (`RTScenarioLoader.cpp:61`, commento da 47) accetta l'array di coordinate **se e solo se** ha esattamente
tre elementi `[q, r, layer]`. Applicata l'arbitrazione DEV-LEAD: uscita **(b)**, la forma a due
elementi e' rifiutata.

Tre cambiamenti, nessun helper nuovo:

1. **Guardia dell'arita'** — `Arr->Num() < 2` diventa `Arr->Num() != 3`. Cadono insieme il basso
   (`[q, r]`) e l'alto (`[q, r, layer, 7]`, oggi accettato con la coda scartata in silenzio).
2. **Terzo elemento obbligatorio** — sparisce il ternario `Arr->Num() >= 3 ? ... : 0`. Il layer si
   legge sempre da `(*Arr)[2]`, mai dedotto. Nessun fallback: la cella dedotta *e'* il difetto.
3. **Ramo `nullptr` separato** — l'array assente e l'array di lunghezza sbagliata sono due difetti
   diversi e portano due messaggi. Prima ne condividevano uno solo. Motivo scritto in un commento
   accanto alla guardia: `«trovati 0»` su un campo che manca del tutto direbbe una cosa falsa su un
   array che non c'e'.

Commento della funzione riscritto: diceva *«Il layer e' opzionale (default 0)»*, ora dichiara
l'arita' esatta, il perche' (uno scenario e' autorevole per il gate: una cella valida e sbagliata
non produce un rosso, produce un verde su un'altra partita) e il perche' il messaggio nomina la
lunghezza trovata.

## PUBLIC CONTRACT

| Campo | Valore |
|---|---|
| Given | un array JSON di coordinate in ingresso a `ParseCell`, da uno dei suoi chiamanti |
| When | il loader legge quell'array durante il caricamento di uno scenario |
| Then | accettato **sse** `Num() == 3`; `Out = FRTCellId(Arr[0], Arr[1], Arr[2])` |
| Authority | il loader; nessuna autorita' di simulazione coinvolta |
| Timing boundary | N/A — parsing a caricamento, fuori da ogni fase di turno |
| Target/recipient | il chiamante, che riceve `false` e `OutError` valorizzato |
| Failure | `Arr == nullptr` -> `false`; `Num() != 3` -> `false` con lunghezza trovata nel messaggio |
| Fallback | **nessuno**, deliberatamente |
| Ordering | N/A — una chiamata, al massimo un esito |
| TurnLog | NONE — il parsing precede la partita |
| Replay | N/A — a valle arriva un `FRTCellId` gia' formato |
| Privacy | N/A — nessun dato di squadra su questo percorso |
| SEED_SOURCE | none |

### Messaggi d'errore — testo esatto

Arita' sbagliata (e' quello che l'acceptance criterion 3 chiede di leggere):

```
%s: la cella deve essere [q, r, layer] — attesi 3 elementi, trovati %d
```

Array assente o non-array:

```
%s: la cella manca o non e' un array [q, r, layer]
```

`%s` resta `Where`, come in tutti gli altri errori del loader. Esempio reso, per `"cell": [0, 0, 0, 0]`
nella sezione `cells`:

```
cells: la cella deve essere [q, r, layer] — attesi 3 elementi, trovati 4
```

⚠️ Il messaggio contiene un em-dash (`—`) e le virgolette basse nel commento. **E' la convenzione
preesistente del file**, non una novita': em-dash dentro `TEXT()` compare gia' in altri nove
messaggi (righe post-modifica 131, 178, 722, 732, 2031, 2059, 2066, 2160, 2188). Il file non ha BOM
ne' prima ne' dopo — verificato
(`head -c 3` = `#in`). Nessun cambio di profilo di rischio sull'encoding, ma se VALIDATION vede
mojibake nel log, la causa e' preesistente e vale per nove messaggi, non per questo.

Verificato inoltre che i literal del file **non usano accenti**: `e'`, `puo'`, `arita'`, `unita'`.
Il testo nuovo li rispetta.

## FILES

- `Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp` — modificata la sola `ParseCell`
  (righe 47-81 dopo la modifica: commento 47-60, firma 61, corpo fino a 81). Nessun'altra funzione
  toccata, nessuna firma cambiata, nessun
  include aggiunto o rimosso.

## TESTS

**Owner DEV-TEST** (`Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp`). Non ho scritto test e
non ho letto il lavoro dell'istanza parallela.

Dato utile a chi li scrive: la stringa del messaggio vecchio (`"la cella deve essere [q, r] oppure
[q, r, layer]"`) compariva **una volta sola in tutto `Source/`**, nel punto che la emetteva. Grep
rieseguita ora: nessun test asseriva il testo precedente, quindi nessuna assertion esistente e'
resa rossa dalla riscrittura del messaggio.

## BUILD

`NOT RUN — dominio VALIDATION (ruolo DEV, motore non occupabile)`.

Buildability statica curata per costruzione: nessun include nuovo, nessuna dipendenza `.Build.cs`
nuova, firma di `ParseCell` invariata, `Arr->Num()` e' `int32` e corrisponde a `%d`.

## TURNLOG / REPLAY

Nessun evento emesso, nessun formato di replay toccato. A valle di `ParseCell` arriva un `FRTCellId`
gia' formato: il resolver non vede la differenza. Concorda con `DETERMINISM`/`TURNLOG/REPLAY` fuori
scope nell'handoff.

## NETWORK / PRIVACY

Nessun impatto. Il parsing e' locale al caricamento, precede la partita, non attraversa dati di
squadra ne' repliche.

## EDITOR EXPECTATION

**PREDICTED — NOT VERIFIED.** Non ho aperto l'Editor.

Uno scenario con un array di coordinate di lunghezza != 3 smette di caricare, e il log mostra il
messaggio con la lunghezza trovata invece di una cella silenziosamente sbagliata. Corpus attuale:
nessuno scenario e' in questo stato (misura sotto), quindi l'atteso e' *nessun cambiamento visibile*
sul corpus esistente.

## INTEGRATION REQUIRED

**none.** Nessuna scrittura fuori scope necessaria per chiudere il comportamento assegnato.

Due osservazioni che NON ho applicato — sono decisioni di DEV-LEAD, non difetti da aggiustare in
silenzio dentro il mio scope:

### 1. Due siti inline che non passano da `ParseCell`

Lo stesso file costruisce una `FRTCellId` in due punti **senza** chiamare `ParseCell`, e quei due
punti conservano il comportamento vecchio (`>= 2`, layer dedotto, coda scartata):

- riga 986-991 — `targetCell` di un intent:

  ```cpp
  if (IntentObj->TryGetArrayField(TEXT("targetCell"), TargetCellArr) && TargetCellArr->Num() >= 2)
  {
      Intent.TargetCell = FRTCellId(
          static_cast<int32>((*TargetCellArr)[0]->AsNumber()),
          static_cast<int32>((*TargetCellArr)[1]->AsNumber()),
          TargetCellArr->Num() >= 3 ? static_cast<int32>((*TargetCellArr)[2]->AsNumber()) : 0);
  ```

- riga 1073-1083 — `dashTo` di un intent (stessa forma, con in piu' un messaggio proprio sul ramo
  basso, che pero' parla di destinazione mancante, non di arita').

⚠️ **Il work order dice che `ParseCell` e' «l'unico ingresso delle coordinate negli scenari». Alla
lettera non lo e'**: `targetCell` e `dashTo` entrano da questi due siti. Il difetto dell'issue
sopravvive li' anche dopo la mia modifica — `"targetCell": [0, 0, 0, 0]` resta accettato.

Non li ho toccati: il contratto comportamentale dell'handoff nomina `ParseCell` e i suoi chiamanti,
e questi due non lo sono. Unificarli e' una restrizione di formato aggiuntiva su due chiavi diverse,
con test propri, e la sede della decisione e' DEV-LEAD o l'issue. Diff proposto, **non applicato**,
se la decisione e' unificare:

```diff
-							if (IntentObj->TryGetArrayField(TEXT("targetCell"), TargetCellArr) && TargetCellArr->Num() >= 2)
+							if (IntentObj->TryGetArrayField(TEXT("targetCell"), TargetCellArr))
 							{
-								Intent.TargetCell = FRTCellId(
-									static_cast<int32>((*TargetCellArr)[0]->AsNumber()),
-									static_cast<int32>((*TargetCellArr)[1]->AsNumber()),
-									TargetCellArr->Num() >= 3 ? static_cast<int32>((*TargetCellArr)[2]->AsNumber()) : 0);
+								if (!ParseCell(TargetCellArr, Intent.TargetCell, OutError,
+									*FString::Printf(TEXT("intent di '%s', targetCell"), *Intent.UnitId)))
+								{
+									return false;
+								}
 								Intent.bTargetsCell = true;
```

⛔ Il diff su `dashTo` non e' equivalente e non lo scrivo a cuor leggero: li' il ramo basso emette
oggi *«non dichiara una destinazione»*, che e' un messaggio diverso e probabilmente asserito da
qualche test. Sostituirlo con il messaggio d'arita' cambierebbe un errore esistente. Serve una
decisione, non un'applicazione.

### 2. `Source/RefactorTacticsEditor/.../RTSetObjectiveCellCommandlet.cpp:14`

Ha un `ParseCell` **omonimo** che legge una `FString`, non un array JSON. Confermo che non e'
questa funzione e non l'ho toccato — coerente con l'`OUT OF SCOPE` dell'handoff.

## VALIDATION REQUESTED

Gate che occupano Unreal, per DEV-LEAD:

1. Build del modulo `RefactorTactics`.
2. Automation Test del loader, incluso il ramo di rifiuto scritto da DEV-TEST.
3. Suite scenari (`Scenarios/` a 3 elementi ovunque) — la misura statica sotto dice che deve restare
   verde; se diventa rossa, il corpus ha una cella che la mia grep non ha visto.

## RISKS

- **Restrizione di formato retroattiva.** Ogni scenario esistente con una cella != 3 elementi smette
  di caricare. Misura statica read-only sul corpus, eseguita ora, nessun file modificato:

  | Forma | Occorrenze in `Scenarios/` |
  |---|---|
  | array numerico a 2 elementi | **4 — tutte dentro una stringa di prosa**, `Spec/Reaction/DeflectionPoolSpansMultipleHits.json:22`, chiave `_nota_geometria` (`[1,0]`, `[-1,0]`, `[0,1]`, `[0,-1]` citati in un commento). **Nessuna e' un array JSON.** |
  | array numerico a 4+ elementi | **0** |
  | array numerico a 3 elementi | 654 |

  Le uniche corrispondenze a due elementi sono testo, non dati: il corpus non esercita ne' il ramo
  basso ne' quello alto. **Costo di migrazione: zero**, come sosteneva l'arbitrazione DEV-LEAD.

- **Round-trip writer -> loader.** `RTScenarioWriter.cpp:48` `WriteCellArray` scrive sempre
  `X`, `Y`, `Layer` — tre valori, incondizionatamente. Uno scenario riscritto dal writer resta
  caricabile. Verificato leggendo la funzione, **non** eseguendo un round-trip.

- **Il messaggio del ramo `nullptr` e' nuovo e nessun test lo copre**, salvo che DEV-TEST lo aggiunga.
  Non e' una regressione (prima quel caso emetteva l'altro messaggio, anch'esso non asserito da
  nessuno), ma e' un ramo misurabile che resta non misurato. Segnalato a DEV-TEST via questo file.

- **Conteggio dei chiamanti.** L'issue e il work order dicono «otto chiamanti». Ne ho contati
  **sette** call site nel loader (righe 396, 432, 505, 607, 1158→1179, 1196→1217, 1377→1398 nella
  numerazione pre/post modifica), piu' la riga della definizione: `git grep -c ParseCell` sul file
  restituisce 8 righe perche' conta anche la definizione. Non cambia nulla di sostanziale, ma
  l'acceptance criterion 5 parla di «tutti e otto i chiamanti» e DEV-TEST rischia di cercare un
  ottavo che non esiste. Consiglio di leggerlo come **sette chiamanti, una funzione sola**.

## NOT RUN

| Verifica | Stato | Motivo |
|---|---|---|
| BUILD | NOT RUN | dominio VALIDATION; il ruolo DEV non occupa Unreal |
| AUTOMATION TEST | NOT RUN | test di proprieta' DEV-TEST, esecuzione a VALIDATION |
| SCENARIO HARNESS | NOT RUN | occupa Unreal |
| PIE / EDITOR | NOT RUN | occupa Unreal; nessun `.uasset`/`.umap` nel write-set |
| PACKAGED | NOT RUN | fuori dal percorso toccato |
| `HEAD == BASE_SHA` | NOT RUN | il mandato vieta ogni comando git; `BASE_SHA 39f3ec95` assunto da `RT3-DEVLEAD-39f3ec9.md`, non misurato |
| Round-trip writer→loader eseguito | NOT RUN | verificato per lettura del codice del writer, non eseguendo |
| Mutation test (criterio 4) | NOT RUN | e' un gate, non authoring; richiede build |

## AUTO-REVIEW — i chiamanti

Cercati con grep su tutto il repo. Nessun chiamante passa deliberatamente due elementi: tutti e sette
passano un `TArray<TSharedPtr<FJsonValue>>*` ottenuto da `TryGetArrayField`/`TryGetArray` sul file,
mai un array costruito in codice. Non esiste quindi un sito che perda una funzionalita' voluta.

| # | Riga (post-modifica) | `Where` | Passa 2 elementi deliberatamente? |
|---|---|---|---|
| 1 | 417 | `cells` | no — array dal file |
| 2 | 453 | `interiorWalls` | no |
| 3 | 526 | `doors` | no |
| 4 | 628 | `unita' '%s'` | no |
| 5 | 1179 | `move di '%s'` | no |
| 6 | 1217 | `expect UnitAtCell` | no |
| 7 | 1398 | `variante '%s', unita' '%s'` | no |

Nessuno da fermare. Le due costruzioni inline (`targetCell`, `dashTo`) non sono chiamanti e stanno in
`## INTEGRATION REQUIRED`.

STATUS:   READY
