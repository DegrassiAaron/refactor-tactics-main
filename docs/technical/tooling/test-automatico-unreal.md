# Spec — RT Scenario Test Harness

> `CURRENT` · **Stato**: as-built · **Owner**: questo file
> **Allineata al codice**: §6 (esiti) e §3 (conteggio scenari) il **2026-08-15**; il resto il **2026-08-08**
> ⚠️ **Ciò che questo file dichiara di sé invecchia per conto proprio.** Le due sezioni ridatate sopra
> erano rimaste indietro senza che niente lo segnalasse — il §6 argomentava **tre** esiti dove il codice
> ne aveva quattro, e il §3 diceva «cinque scenari» dove il repository ne aveva 76 — e da qui l'errore è
> passato a valle, nel mandato QA che questo file governa. **Conteggi ed enum si rimisurano sulla
> sorgente**; questa spec è l'owner delle *decisioni*, non dei numeri.
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

Al **2026-08-15** il repository contiene **76 file JSON** sotto `Scenarios/`, così ripartiti:

| Cartella | File | Nota |
|---|---|---|
| `Spec/` | 38 | il grosso: scenari di specifica, molti `BLOCKED` per capability non ancora costruite |
| `Visual/` | 21 | — |
| `Combat/` | 9 | — |
| `Movement/` | 6 | i cinque originali più uno |
| radice | 2 | `RT_Showcase_Relay_v01.json` (scenario) e `_redirects.json`, che **scenario non è** |

⚠️ **`76` è il conto dei file, non degli scenari**, e le quattro categorie ne contengono **74**. Un totale
nudo — `find Scenarios -name "*.json" | wc -l` — nasconde entrambi gli scarti e vale meno della
ripartizione. Rimisura per cartella:

```sh
git ls-tree -r --name-only origin/main -- Scenarios/ | grep '\.json$' \
  | sed 's|Scenarios/||; s|/.*||' | sort | uniq -c
```

I cinque `Movement.*` nati per primi restano il nucleo che fissa il comportamento base, e vale la pena
sapere cosa ciascuno prova:

| Scenario | Cosa fissa |
|---|---|
| `Movement.Basic` | il caso nominale: l'unità arriva dove è stata mandata |
| `Movement.BasicFailsOnPurpose` | **fallisce apposta**: verifica che l'harness sappia dire `FAIL`. Un test che non sa fallire non è un test |
| `Movement.Blocked` | percorso inesistente ⇒ piano **rifiutato in pianificazione**, l'unità resta dov'è. È il comportamento del gioco, non un errore |
| `Movement.Collision` | due unità che si contendono la stessa cella |
| `Movement.SwapRejectedByPlanning` | lo scambio A↔B **non è pianificabile** |

> ⚠️ Il nome `Movement.Blocked` descrive un percorso bloccato **nel gioco**, e non ha niente a che vedere
> con l'esito `BLOCKED` del §6, che descrive una capability mancante **nel progetto**. Quello scenario
> chiude `PASS`.

## 4. Schema dello scenario

```jsonc
{
  "scenarioId": "Movement.Blocked",   // ID stabile e gerarchico; dà il nome alla cartella di output
  "version": 1,                       // versione del FORMATO, non del contenuto — 2 se usi `decisions`
  "seed": 0,                          // dichiarato ma NON consumato — vedi §4.1
  "mapRadius": 3,                     // arena esagonale GENERATA: nessun .umap da versionare

  "cells":  [ { "cell": [q, r, layer], "blocksMovement": true,
                "blocksLineOfSight": false, "moveCost": 0 } ],   // moveCost 0 = default (1)

  "units":  [ { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-2, 0, 0],
                "facing": "E" } ],                              // opzionale; vedi §4.2

  "turns":  [ { "intents":   [ { "unit": "A1", "move": [[2, -1, 0]] } ],
                "requires":  ["DecisionBoundary"],                      // capability attese; vedi §4.3
                "decisions": [ { "unit": "V1", "respond": "FIRE",
                                 "target": "A1" } ] } ],                // richiede "version": 2

  "expect": [ { "type": "UnitAtCell",     "unit": "A1", "cell": [-2, 0, 0] },
              { "type": "TurnsCompleted", "value": 1 } ]
}
```

- `id` dell'unità è **locale allo scenario** (`A1`), non l'ID di gioco: lo usano intent e assertion.
- `hero` è lo Stable ID del catalogo: `Hero.Gadget`, `Hero.Phase`, `Hero.Riktor`, `Hero.Wraith`.
- `move` sono i **waypoint**, esattamente come li produrrebbe un giocatore che clicca. Vuoto = unità ferma.
- `cells` modifica solo le celle che interessano: le altre restano pavimento a costo 1. È ciò che permette di
  scrivere `Movement.Blocked` senza versionare una mappa.
- `decisions` (**#512**, CP 15.3 metà B) sono le risposte scriptate ai *decision boundary* di quel turno:
  `respond` vale `FIRE` o `HOLD`, e `target` è **obbligatorio con `FIRE`** e **vietato con `HOLD`**. Si
  consumano **in ordine di dichiarazione**, una per finestra, per l'unità che le nomina.
  - ⚠️ **Richiedono `"version": 2`.** Un file che le porta dichiarando `1` viene rifiutato, e non è
    pedanteria: una build vecchia non conosce la chiave, la ignorerebbe in silenzio e giocherebbe il turno
    **non scriptato**. Una lista vuota non chiede nulla di nuovo e resta valida a versione 1.
  - ⚠️ **Il residuo è un errore, non un avanzo**: una decisione dichiarata che nessuna finestra consuma fa
    cadere il turno, e così una finestra che si apre senza una decisione che la nomini. Se restassero
    silenziose, due decisioni scritte e una applicata sarebbero verdi.
  - Il numero di finestre non si deduce dai micro-step: un `FIRE` **tronca** il movimento residuo e ne apre
    quindi una sola, un `HOLD` lascia proseguire. Contale nel TurnLog prima di fissare quante decisioni
    dichiarare.
  - Chi risponde è registrato in `result.json` come `decisionSource` — `scenario`, `test-override` o
    `none` — insieme a `scriptedDecisionsApplied` e `scriptedDecisionsUnused`. Un decisore bindato **prima**
    della sessione ha la precedenza, ed è per questo che la provenienza si scrive invece di dedurla.

### 4.1 Il `seed` non fa niente, e va bene così

Il campo esiste, viene registrato nel report, e **nessun RNG lo consuma**: oggi il progetto non ha RNG, e il
determinismo viene da coordinate intere e ordinamenti totali (`HexSim.ReplayDivergenceZero`). Sta lì perché il
giorno in cui un RNG entrasse nel resolver, lo scenario debba già saperlo dichiarare — non perché serva adesso.

### 4.2 `facing`: dove guarda una figura appena posata

Opzionale, uno fra `E · NE · NW · W · SW · SE` (maiuscole indifferenti). Assente = `E`, che è anche il default
di `ARTUnit`: omettere il campo lascia lo scenario com'era. Un nome sconosciuto è un **errore di caricamento**,
non un ripiego su `E` — un ripiego darebbe allo scenario un orientamento diverso da quello scritto nel file, e
il suo verde direbbe la cosa sbagliata.

**È servito da CP 13.2** (2026-08-11), da quando il targeting consuma la conoscenza di squadra: l'orientamento
decide cosa la squadra vede, quindi uno scenario che non potesse esprimerlo non potrebbe descrivere un tiratore
che guarda il proprio bersaglio. Prima di allora sei scenari passavano perché nessuno leggeva la vista.

⚠️ **Non è la `DeclaredRotation`, e la distinzione non è formale.** La capability `DeclaredRotation` resta
**indisponibile** (D-020, [#291](https://github.com/DegrassiAaron/refactor-tactics-main/issues/291)): dichiarare
una rotazione *in pianificazione* è una mossa che il giocatore non ha ancora modo di chiedere, e darla
all'harness lo renderebbe più capace del gioco. Dire dove una figura guarda quando la si posa sul tavolo è
invece la stessa classe di `cell` — nessuno «dichiara» di stare in una cella, ci si sta e basta. Uno scenario
che voglia *cambiare* orientamento a metà partita resta `BLOCKED`.

## 5. Assertion

`ERTAssertionKind`. Nacquero **due**, e ognuna delle altre è arrivata quando uno scenario reale l'ha
richiesta — mai prima. Si aggiungono sempre **in coda**: gli scenari sul disco riferiscono i valori
dell'enum, e rinumerarli riscriverebbe il significato dei file già scritti.

| Tipo | Verifica | Arrivata con |
|---|---|---|
| `UnitAtCell` | l'unità indicata è sulla cella attesa a fine scenario | — |
| `TurnsCompleted` | sono stati completati almeno N turni senza che la partita si interrompesse | — |
| `UnitHpEquals` | la salute è esattamente il valore atteso: è così che si verifica un danno | — |
| `UnitAlive` | l'unità è viva (`value` ≠ 0) oppure abbattuta | — |
| `UnitFacing` | l'unità guarda nella direzione attesa, per **nome** (`E`, `NE`, …) | CP 16.1 |
| `LogEventCount` | quante volte un evento compare nel TurnLog. `value: 0` asserisce l'**assenza** | [#318](https://github.com/DegrassiAaron/refactor-tactics-main/issues/318) |
| `LogEventOrder` | la prima occorrenza di un evento **precede** la prima di un altro | [#318](https://github.com/DegrassiAaron/refactor-tactics-main/issues/318) |
| `LogEventAmount` | il **valore** di `Amount` della prima voce che corrisponde. `value` è obbligatorio | [#361](https://github.com/DegrassiAaron/refactor-tactics-main/issues/361) |

### 5.1 Le tre assertion che leggono il TurnLog

Le prime cinque guardano tutte lo **stato finale**. Ordine degli eventi e contatori del log erano fuori
portata, e con essi undici scenari già dichiarati nel Feature Registry — **undici allora, tredici oggi**: la
riconciliazione di `#361` ha portato i `Spec.TimeBank.*` da otto a dieci, e i `Spec.Clash.*` restano tre.
Con le tre assertion di questa sezione nessuno dei tredici è più bloccato: restano da scrivere.

La terza è arrivata dopo, e non per dimenticanza: `LogEventAmount` legge un numero, e finché non fu deciso
**quale** numero il Decision Time Bank scrive nel log ([`spec-turnlog.md`](spec-turnlog.md) §4.2, issue
`#361`) l'assertion sarebbe stata scritta due volte. Non è però un'assertion del bank: `Amount` è il payload
numerico di **ogni** categoria — danno per `Combat`, celle percorse per `Move`, direzione per `Facing` — e
questa primitiva le serve tutte. `LogEventCount` dice *che* il colpo è avvenuto; `LogEventAmount` dice
*quanto* ha tolto.

L'evento si nomina **per nome**, come la direzione di `UnitFacing`:

```json
{ "type": "LogEventCount", "category": "Facing", "outcome": "DeclarationRejected", "value": 0 }
{ "type": "LogEventOrder", "category": "Combat", "outcome": "Hit",
  "thenCategory": "Move", "thenOutcome": "Moved" }
{ "type": "LogEventAmount", "category": "Combat", "outcome": "Hit", "value": 22 }
```

Per `LogEventCount`, `value` omesso vale **1**, cioè «l'evento c'è». Per `LogEventAmount` `value` è invece
**obbligatorio**: lì un default sarebbe un numero inventato dal parser, e uno scenario passerebbe senza dire
cosa si aspettava. `LogEventAmount` accetta valori **negativi**, che un conteggio non può avere.

Un evento **assente** fa fallire `LogEventAmount` dicendo che è assente, non confrontando uno zero: un difetto
di produzione non va fatto sembrare un difetto di valore. È lo stesso trattamento di `LogEventOrder`. I nomi si risolvono per **riflessione** sull'enum
(`StaticEnum<>`), non da una tabella scritta a mano: un esito aggiunto al TurnLog diventa scrivibile negli
scenari senza toccare il loader, e non c'è una seconda copia da tenere allineata — è la lezione della chiave
`edge`, dove il parser che la leggeva e l'elenco che la rifiutava sono nati su rami diversi.

Un esito che appartiene a un'**altra** categoria è un `ERROR`, non un confronto fra interi: `BridgeRemoved`
esiste, ma non fra gli esiti di `Facing`, e il messaggio lo dice nominando la categoria.

Due cose che vanno sapute, perché decidono cosa queste assertion possono e non possono verificare.

**Il log si accumula nella sessione, non nel TurnManager.** `ARTTurnManager::TurnLog` viene **azzerato** a
ogni `LockInAndResolve`: a scenario finito conterrebbe il solo ultimo turno. `FRTScenarioSession` appende le
voci di ogni turno appena questo ha finito di risolvere, cioè nell'unico istante in cui il log di quel turno
è completo e non ancora sostituito.

**L'ordine è quello di scrittura, e l'hash non lo conosce.** La forma canonica del log — quella serializzata
e quella che entra in `URTTurnLogLibrary::HashTurnLog` — è **ordinata** (`SortTurnLog`), quindi l'hash è
invariante per permutazione. Chi volesse verificare una sequenza guardando il checksum verificherebbe
un'altra cosa; e un'assertion su un hash **letterale** legherebbe ogni scenario al formato del log, dove la
prima voce nuova li romperebbe tutti insieme. Il determinismo si verifica **eseguendo due volte**: è una
proprietà del runner, non di uno scenario.

Ogni risultato porta **`Expected` e `Actual`**, non un booleano. Un report che dice «fallita» senza dire cosa
si aspettava costringe a rieseguire il test per capire — e a quel punto tanto varrebbe non averlo.

## 6. `PASS` · `FAIL` · `ERROR` · `BLOCKED` — e perché sono quattro

`ERTTestOutcome`, in [`RTTestScenario.h`](../../Source/RefactorTactics/ScenarioHarness/RTTestScenario.h):

| Esito | Significato | Di chi è il difetto |
|---|---|---|
| `Pass` | simulazione completata, tutte le assertion soddisfatte | — |
| `Fail` | simulazione completata, almeno un'assertion non soddisfatta | **del gioco** |
| `Error` | impossibile eseguire: scenario invalido, eroe sconosciuto, mappa mancante | **del test** o dell'ambiente |
| `Blocked` | scenario valido, ha girato fin dove poteva, poi ha incontrato una capability **non ancora costruita** | **di nessuno**: è il progetto che non c'è ancora |

**`Error` non è un `Fail`.** Confonderli fa perdere ore su una regressione che non esiste: uno scenario rotto
si traveste da difetto di gioco, e si va a cercare nel resolver un bug che è nel JSON.

**E `Blocked` non è nessuno dei due.** Esiste per una ragione che vale la pena scrivere, perché senza di
essa sembra un `Fail` addomesticato: serve a **versionare uno showcase prima che tutti i suoi sistemi
esistano**. Senza questo esito la scelta sarebbe fra tenere la partita dimostrativa in Markdown per
settimane, o vederla rossa ogni giorno finché non è completa — e *una suite che ha un rosso «normale»
smette di essere letta*. Il valore è stato aggiunto **in coda** all'enum, così i precedenti non cambiano
numero.

🔴 **`Blocked` è l'esito che tace, ed è quello da trattare con più sospetto.** Non è rosso e non è verde:
uno scenario che non ha mai eseguito la parte interessante passa senza dirlo. ∴ **non si conta né fra i
passati né fra i falliti** — in un report sta in una colonna sua. Un conteggio che lo assorbe da una parte
o dall'altra descrive una copertura che non c'è.

> ⚠️ Fino al 2026-08-15 questa sezione si intitolava «*e perché sono tre*» e ne elencava tre, mentre il
> §4.2 di questo stesso file usava già `BLOCKED` come esito e il codice aveva quattro valori. La
> divergenza è stata misurata dallo spec panel di Terminal B
> ([`../roadmap/plans/qa-terminal-b-scenario-runner-spec-panel-2026-08-15.md`](../roadmap/plans/qa-terminal-b-scenario-runner-spec-panel-2026-08-15.md)),
> che ne aveva ereditato il conteggio nel proprio mandato — è il motivo per cui un enum si legge nell'enum.

## 7. Output

`Saved/RTTests/<ScenarioId>/<RunId>/result.json` — `Saved/` è già escluso da git: i report sono **artefatti**,
non sorgenti. `URTTestReportWriter` versiona lo schema di `result.json` e `FindLatestRunDirectory` recupera
l'ultima esecuzione.

Il report contiene: `ScenarioId`, esito, `ErrorMessage` (solo se `Error`), `TurnsPlayed`, `Seed`, l'elenco
delle assertion con expected/actual, e **`StateHash`**.

### 7.1 `StateHash` e il gate di determinismo

Digest dello stato finale — posizione, salute, scudo, energia e stati di ogni unità, **anche di quelle cadute** (entrano con `bAlive = false`, [D-084](../decisions/RT_PDR_00_Decision_Log.md)) — usato dal gate di
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
| `rt.Map.Source <Enum>` | **CVar**: scavalca `MapSource` del GameMode (es. `LevelAsset`). Un valore sconosciuto non ripiega in silenzio |

`rt.Test.Scenario` va impostata **prima** di premere Play: `ARTGameMode` vede la variabile e la partita normale
**non** viene allestita — al suo posto parte lo scenario.

> ⚠️ **Da riga di comando servono `-dpcvars=`, non `-ExecCmds=`.** `-ExecCmds` gira *dopo*
> l'inizializzazione, quando il GameMode ha già allestito la partita: la variabile viene impostata, non
> serve a niente, e non c'è un errore che lo dica. Misurato sul pacchettizzato il 2026-08-10.
>
>     RefactorTactics.exe -dpcvars=rt.Map.Source=LevelAsset

## 9. Requisiti aperti

| Tema | Stato |
|---|---|
| Assertion su HP, scudo, stati, TurnLog | da aggiungere quando uno scenario le richiede |
| Intent diversi dal movimento (abilità, reazioni) | non implementati: il prompt chiedeva esplicitamente di non procedere finché `Movement.Basic` non fosse stabile |
| **Politica per le Fast Reaction** | ✅ **chiusa il 2026-08-16 con `#512`** (PR #1016). Diceva: *«quando E14 introdurrà le finestre, uno scenario dovrà poter dichiarare la risposta attesa (`FIRE`/`HOLD`/timeout) come dato, altrimenti diventa non deterministico»* — ed è la condizione che si è avverata. Oggi `turns[].decisions` è quel dato (§4, `"version": 2`), nessun timer reale la attraversa, e il decisore è iniettabile con la precedenza al test. ⏳ Resta di **fase B** la sola capability `DecisionBoundary`: scoprirla senza le `decisions` negli scenari che la chiedono li farebbe girare con finestre a cui nessuno risponde |
| **Nessun bypass** | invariante permanente: se un giorno un percorso di test saltasse il resolver, i test smetterebbero di misurare il gioco |

🔴 **Nessuna di queste quattro voci è iniziabile da una sessione che possiede solo `ScenarioHarness/`**, e
va detto qui perché è questa la tabella che un mandato legge come lista di lavoro. Misurato il 2026-08-15:
la prima è in **stallo** — chiede uno scenario, e `Scenarios/` è `integration_only` in
[`../roadmap/parallel-batch.yaml`](../roadmap/parallel-batch.yaml); la seconda è una **precondizione**, non
un task; la terza dipende da **E14**, che non è atterrato; la quarta è un'**invariante**, che si rispetta e
non si chiude.

∴ **il lavoro eseguibile è al §10**, non qui: sono estensioni di `RTScenarioLoader` e `RTTestScenario.h`.
Chi arriva a questa tabella cercando da dove partire, parta da lì.

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
