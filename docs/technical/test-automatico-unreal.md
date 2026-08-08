# Spec — RT Scenario Test Harness

> `CURRENT` · **Stato**: as-built, allineata al codice il **2026-08-08** · **Owner**: questo file
> **Guida operativa** (come si lancia, come si legge un esito): [`test-e-diagnosi.md`](test-e-diagnosi.md).
>
> *Fino al 2026-08-08 questo file era il **prompt di implementazione** originale — «TASK — progettare e
> implementare…», 1114 righe di istruzioni a un agente. Un prompt non è una specifica: descrive ciò che si
> voleva provare a costruire, non ciò che è stato costruito. Il prompt è conservato in
> [`../archive/src/handoff/scenario-harness-task-originale.md`](../archive/src/handoff/scenario-harness-task-originale.md).*

---

## 1. Il principio, e perché è l'unico che conta

**Uno scenario passa dallo stesso percorso di gioco di una partita reale.**

```
scenario JSON → piani sulle unità → LockInAndResolve → resolver → TurnLog → assertion
```

Mai `SetActorLocation`, mai una scorciatoia che salti il resolver. Un test che non attraversa il codice vero
non prova niente sul codice vero: proverebbe che l'harness sa spostare un Actor.

È la stessa porta da cui entrano il giocatore e il bot — vedi la pipeline in
[`architettura-codice.md`](architettura-codice.md).

## 2. Cosa fu costruito, e cosa il prompt proponeva

| Il prompt proponeva | Cosa esiste |
|---|---|
| un `ARTTestDirector`, Actor da mettere nel livello | **nessun Actor di test**: `grep -rn "TestDirector" Source/` non trova nulla |
| — | una **CVar** `rt.Test.Scenario` + un ramo in `ARTGameMode` |
| un runner dedicato | lo **stesso** runner della partita |

La soluzione adottata è più semplice di quella proposta, e la differenza è sostanziale: senza un Actor
obbligatorio, uno scenario non ha prerequisiti di livello. Si imposta una variabile e si preme Play.

## 3. Dove vivono gli scenari

**`Scenarios/<Categoria>/<Nome>.json`**, alla **radice del repository** — non in `Content/`.

Sono JSON perché devono essere **leggibili e diffabili in una pull request**. Come `.uasset` sarebbero binari,
e nessuna review potrebbe dire cosa è cambiato in uno scenario.

Al 2026-08-08 ne esistono **cinque**, tutti `Movement.*`:

| Scenario | Cosa fissa |
|---|---|
| `Movement.Basic` | il caso nominale: l'unità arriva dove è stata mandata |
| `Movement.BasicFailsOnPurpose` | **fallisce apposta**: verifica che l'harness sappia dire `FAIL`. Un test che non sa fallire non è un test |
| `Movement.Blocked` | percorso inesistente ⇒ piano **rifiutato in pianificazione**, l'unità resta dov'è. È il comportamento del gioco, non un errore |
| `Movement.Collision` | due unità che si contendono la stessa cella |
| `Movement.SwapRejectedByPlanning` | lo scambio A↔B **non è pianificabile** |

## 4. Schema dello scenario

```jsonc
{
  "scenarioId": "Movement.Blocked",   // ID stabile e gerarchico; dà il nome alla cartella di output
  "version": 1,                       // versione del FORMATO, non del contenuto
  "seed": 0,                          // dichiarato ma NON consumato — vedi §4.1
  "mapRadius": 3,                     // arena esagonale GENERATA: nessun .umap da versionare

  "cells":  [ { "cell": [q, r, layer], "blocksMovement": true,
                "blocksLineOfSight": false, "moveCost": 0 } ],   // moveCost 0 = default (1)

  "units":  [ { "id": "A1", "hero": "Hero.Flux", "team": 0, "cell": [-2, 0, 0] } ],

  "turns":  [ { "intents": [ { "unit": "A1", "move": [[2, -1, 0]] } ] } ],

  "expect": [ { "type": "UnitAtCell",     "unit": "A1", "cell": [-2, 0, 0] },
              { "type": "TurnsCompleted", "value": 1 } ]
}
```

- `id` dell'unità è **locale allo scenario** (`A1`), non l'ID di gioco: lo usano intent e assertion.
- `hero` è lo Stable ID del catalogo: `Hero.Flux`, `Hero.Riva`, `Hero.Bastion`, `Hero.Vektor`.
- `move` sono i **waypoint**, esattamente come li produrrebbe un giocatore che clicca. Vuoto = unità ferma.
- `cells` modifica solo le celle che interessano: le altre restano pavimento a costo 1. È ciò che permette di
  scrivere `Movement.Blocked` senza versionare una mappa.

### 4.1 Il `seed` non fa niente, e va bene così

Il campo esiste, viene registrato nel report, e **nessun RNG lo consuma**: oggi il progetto non ha RNG, e il
determinismo viene da coordinate intere e ordinamenti totali (`HexSim.ReplayDivergenceZero`). Sta lì perché il
giorno in cui un RNG entrasse nel resolver, lo scenario debba già saperlo dichiarare — non perché serva adesso.

## 5. Assertion

`ERTAssertionKind`, deliberatamente **due**:

| Tipo | Verifica |
|---|---|
| `UnitAtCell` | l'unità indicata è sulla cella attesa a fine scenario |
| `TurnsCompleted` | sono stati completati almeno N turni senza che la partita si interrompesse |

Non è una mancanza: il prompt stesso avvertiva di «non creare un mega-framework prematuramente». Le assertion
si aggiungono quando uno scenario reale le richiede.

Ogni risultato porta **`Expected` e `Actual`**, non un booleano. Un report che dice «fallita» senza dire cosa
si aspettava costringe a rieseguire il test per capire — e a quel punto tanto varrebbe non averlo.

## 6. `PASS` · `FAIL` · `ERROR` — e perché sono tre

| Esito | Significato | Di chi è il difetto |
|---|---|---|
| `Pass` | simulazione completata, tutte le assertion soddisfatte | — |
| `Fail` | simulazione completata, almeno un'assertion non soddisfatta | **del gioco** |
| `Error` | impossibile eseguire: scenario invalido, eroe sconosciuto, mappa mancante | **del test** o dell'ambiente |

**`Error` non è un `Fail`.** Confonderli fa perdere ore su una regressione che non esiste: uno scenario rotto
si traveste da difetto di gioco, e si va a cercare nel resolver un bug che è nel JSON.

## 7. Output

`Saved/RTTests/<ScenarioId>/<RunId>/result.json` — `Saved/` è già escluso da git: i report sono **artefatti**,
non sorgenti. `URTTestReportWriter` versiona lo schema di `result.json` e `FindLatestRunDirectory` recupera
l'ultima esecuzione.

Il report contiene: `ScenarioId`, esito, `ErrorMessage` (solo se `Error`), `TurnsPlayed`, `Seed`, l'elenco
delle assertion con expected/actual, e **`StateHash`**.

### 7.1 `StateHash` e il gate di determinismo

Digest dello stato finale — posizione, salute, scudo, energia di ogni unità viva — usato dal gate di
determinismo (CP 12.1): **stesso scenario ⇒ stesso hash**, su qualunque numero di ripetizioni.

È **permutazione-invariante per costruzione**: le unità si ordinano prima di essere mescolate nell'hash.
Quindi cambiare l'ordine degli intent nello scenario **non deve** cambiarlo. Se cambia, l'ordine dell'array sta
decidendo l'esito — che è precisamente ciò che l'invariante #3 vieta.

Vale `0` quando lo scenario non è stato eseguito (`Error`): un hash calcolato su nessuno stato sarebbe un
numero finto.

## 8. Console e auto-run

| Comando | Effetto |
|---|---|
| `rt.Test.List` | elenca gli scenari versionati |
| `rt.Test.Run <ScenarioId>` | esegue nel mondo corrente e scrive il report |
| `rt.Test.DumpResult [Id]` | stampa l'ultimo `result.json` |
| `rt.Test.Scenario <Id>` | **CVar**: scenario da eseguire automaticamente all'avvio della partita |

`rt.Test.Scenario` va impostata **prima** di premere Play: `ARTGameMode` vede la variabile e la partita normale
**non** viene allestita — al suo posto parte lo scenario.

## 9. Requisiti aperti

| Tema | Stato |
|---|---|
| Assertion su HP, scudo, stati, TurnLog | da aggiungere quando uno scenario le richiede |
| Intent diversi dal movimento (abilità, reazioni) | non implementati: il prompt chiedeva esplicitamente di non procedere finché `Movement.Basic` non fosse stabile |
| **Politica per le Fast Reaction** | aperta. Quando E14 introdurrà le finestre, uno scenario dovrà poter dichiarare la risposta attesa (`FIRE`/`HOLD`/timeout) **come dato**, altrimenti diventa non deterministico |
| **Nessun bypass** | invariante permanente: se un giorno un percorso di test saltasse il resolver, i test smetterebbero di misurare il gioco |

---

## 10. Lo schema **target** — cosa serve alla showcase

*Aggiunta il 2026-08-08.* `RT_Showcase_Relay_v01` ([`../product/showcase-v0.1.md`](../product/showcase-v0.1.md))
è il consumatore che dice quanto manca all'harness. **Si estende lo schema esistente, non se ne fa un altro**:
il draft JSON dichiarato dall'handoff non esiste nel repository, e quello attuale regge.

### 10.1 Campi

| Campo | Oggi | Target |
|---|---|---|
| `scenarioId` · `version` · `seed` | ✅ | — |
| `mapRadius` · `cells[]` | ✅ arena generata + override | **+ `mapId`**: riferisce una fixture nominata (`ShowcaseRelayBasin`) invece di ridisegnarla nel JSON |
| `units[]` | ✅ id, eroe, team, cella | + `facing` iniziale |
| `turns[].intents[]` | ✅ **solo `move`** | **+ `ability`** (`actionId`, bersaglio cella/unità, direzione), + `facing` |
| — | — | **+ `surfaces[]` · `structures[]` · `objective`**: stato iniziale dell'ambiente, invece di dedurlo |
| — | — | **+ `reactionPolicy[]`** (§10.2) |
| — | — | **+ `ruleset`**, per fissare la versione di regole del golden |
| `expect[]` | ✅ 2 tipi | **~25** (§10.3) |

### 10.2 Reaction policy

Uno scenario deve poter **automatizzare una Fast Decision vera**, non saltarla:

```text
Hold                 rispondi sempre HOLD
CommitFirstValid     FIRE alla prima opportunity legale
HoldFirstThenCommit  HOLD alla prima, FIRE alla seconda   <- il turno 4 della showcase
CommitSpecificTarget FIRE solo su un bersaglio nominato
Timeout              non rispondere, e verifica che il timeout dia HOLD
```

**La policy risponde alla vera `ReactionOpportunity` del runtime.** In `FAST`/`HEADLESS` la decisione è
immediata, ma attraversa **lo stesso contratto logico** — altrimenti il test verificherebbe una finestra che
in partita non esiste.

### 10.3 Assertion

Si aggiungono **quando uno scenario le richiede**, e molte si costruiscono su poche primitive: «leggi un
campo dello stato finale» e «conta eventi nel TurnLog» coprono la maggior parte della lista.

| Famiglia | Assertion |
|---|---|
| Turno e unità | `TurnCompleted` · `UnitAtCell` · `UnitHasStatus` · `UnitNotHasStatus` · `UnitKO` |
| Ambiente | `SurfaceHasStatus` · `SurfaceNotHasStatus` · `EdgeEnabled` · `EdgeDisabled` · `GraphRevisionChanged` · `CoverExists` |
| Azioni | `AbilityResolved` · `AbilityFizzled` · `EventExists` · `EventCount` |
| Reazioni | `ReactionOpportunityExists` · `ReactionResponseEquals` · `ReactionConsumed` |
| Predizione | `PredictionWhiffed` · `OriginalTargetEquals` · `EffectiveTargetEquals` |
| Partita | `ObjectiveUpdated` · `MatchEnded` |
| Determinismo | `StateHashEquals` · `LogHashEquals` |

### 10.4 Report

```text
Saved/RTTests/<ScenarioId>/<RunId>/
    result.json        esito, assertion, failure diagnosticabili
    turnlog.jsonl      una riga per voce: diffabile, grep-abile
    state_initial.json
    state_final.json
```

`result.json` porta già `schemaVersion`, `scenarioId`, esito, `seed`, assertion e `stateHash`. Mancano:
`runId`, `engineVersion`, `projectVersion`, `rulesVersion`, `contentManifestHash`, `resolverConfigHash`,
`logHash`, `duration`.

Ogni failure deve dire **`assertion` · `expected` · `actual` · `turn` · `phase` · `microStep` ·
`source/unit/cell/event` · `reasonCode`**. Il criterio è quello di sempre: si deve poter diagnosticare un
fallimento **senza aprire migliaia di righe di log Unreal**.

### 10.5 Modi

| Modo | Cosa fa | Vincolo |
|---|---|---|
| `HEADLESS` | nessun rendering, il più veloce | è quello di oggi |
| `FAST` | con mondo, senza attese di presentazione | — |
| `VISUAL` | playback osservabile | **stesso esito logico** dei precedenti |

L'equivalenza logica fra i tre modi è essa stessa un test (`Visual vs Fast`, `Fast vs Headless`): se un modo
producesse un esito diverso, la presentazione starebbe decidendo qualcosa — che è l'invariante #1 rotta.

### 10.6 Matrice degli scenari della showcase

| Test | Tipo | Feature | Turni | Modo | Atteso |
|---|---|---|---|---|---|
| `RT.Scenario.Showcase.T1` | Functional | apertura | T1 | Fast | PASS |
| `RT.Scenario.Showcase.T2` | Functional | predizione | T2 | Fast | PASS |
| `RT.Scenario.Showcase.T3` | Functional | moving target, fuoco | T3 | Fast | PASS |
| `RT.Scenario.Showcase.T4` | Functional | overwatch | T4 | Fast | PASS |
| `RT.Scenario.Showcase.T5` | Functional | struttura, revisione | T5 | Fast | PASS |
| `RT.Scenario.Showcase.T6` | Functional | interposizione | T6 | Fast | PASS |
| `RT.Scenario.Showcase.T7` | Functional | ambiente | T7 | Fast | PASS |
| `RT.Scenario.Showcase.Full` | Golden | partita intera | T1–T8 | Fast | PASS |
| `RT.Scenario.Showcase.Repeat` | Determinismo | hash | Full | Headless | 0 divergenze |
| `RT.Scenario.Showcase.Visual` | Smoke | presentazione | Full | Visual | completa |
| `RT.Scenario.Showcase.Packaged` | Smoke | packaged | Full | Packaged | completa |

Quando una feature è troppo importante per essere verificata **solo** end-to-end, si aggiunge un test core
mirato: uno scenario che fallisce dice *che* qualcosa non va, un test unitario dice *cosa*.
