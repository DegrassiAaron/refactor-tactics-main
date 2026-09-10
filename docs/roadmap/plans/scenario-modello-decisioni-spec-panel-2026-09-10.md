# Modello degli scenari — separazione semantica, input di Resolution, facing dell'Overwatch

> `CURRENT` · **Creato**: 2026-09-10 · Referto di ricognizione e prima fetta verticale.
> **Misurato su** `origin/main` = `18065c28`. Lavoro su `feat/scenario-opportunity-selector`, misurato su
> `f4fceb0a`.
> **Convenzione dei numeri**: dove un conteggio cambia da solo si usa `xx` con accanto il comando che lo
> produce ([`AGENTS.md`](../../../AGENTS.md) §14). I numeri che restano sono **esiti di questo passaggio**.

## 0. In una riga

Delle sette separazioni chieste dal mandato, **una era già nel formato** — metadata / setup / sequence /
expect, appiattiti sul livello superiore ma distinti — e **una era il difetto vero**: gli *input di
Resolution* si abbinavano alle finestre **per ordine di dichiarazione**. Quella è la fetta implementata. Le
altre cinque hanno un owner canonico che le governa già, e tre di esse **respingono** la forma proposta: sono
dichiarate qui come `BLOCCO` o `PROPOSTA`, non applicate in silenzio.

---

## 1. FATTO — la ricognizione

### 1.1 Il formato esiste ed è alla `version: 4`

`FRTTestScenario` ([`RTTestScenario.h`](../../../Source/RefactorTactics/ScenarioHarness/RTTestScenario.h))
porta già tutte le regioni del mandato:

| Regione chiesta | Dove sta |
|---|---|
| Metadata | `scenarioId` · `version` · `tags` · `seed` · `previewUnit` |
| Setup | `fixture` · `mapRadius` · `cells` · `interiorWalls` · `doors` · `interactionBindings` · `units` |
| Sequence → Turn | `turns[]` → `FRTScenarioTurn { intents[], requires[], decisions[] }` |
| Planning per unità | `FRTScenarioIntent` — `move` · `ability` · `target`/`targetCell` · `dash`/`dashTo` · `edge` · `reaction` · `condition` · `facing` |
| Resolution inputs | `turns[].decisions[]` — `unit` · `respond` · `target` |
| Checkpoint | **non esiste** |
| Final expectations | `expect[]` · `variants` · `expectSameAcrossVariants` · `repeatCount` |

Il blocco `setup` del mandato è **già rappresentabile per intero**: fixture, seed, override di
celle/bordi/coperture/porte/hazard/obiettivi, Stable ID, eroe, squadra, cella, `facing` iniziale, HP, scudo,
vista, status con durata, loadout (con la distinzione «assente» ≠ `[]`), `bot`, e la conoscenza iniziale di
squadra ([`RTScenarioKnowledge.h`](../../../Source/RefactorTactics/ScenarioHarness/RTScenarioKnowledge.h)).
L'unico campo del mandato senza corrispondente è il **formato della partita** (2v2/3v3): oggi lo determina il
roster dichiarato in `units`.

### 1.2 Il difetto vero: gli input di Resolution si abbinano per ordine

`FRTScenarioTurn::Decisions` è documentato come *«le risposte scriptate ai decision boundary di questo turno,
**in ordine di dichiarazione**»*, e `FRTScenarioSession::DecideScriptedResponse` lo implementa così: la prima
voce non consumata la cui `unit` combacia con il proprietario della finestra.

Il corpus contiene **due** scenari in cui questo è fragile, e sono i due più importanti:

* [`Spec.Overwatch.HoldThenFire`](../../../Scenarios/Spec/Overwatch/HoldThenFire.json) — due voci `V1`
  consecutive, `HOLD` poi `FIRE`;
* [`RT_Showcase_Relay_v01`](../../../Scenarios/RT_Showcase_Relay_v01.json) — due voci `Ivrin` consecutive.

In entrambi, *quale* delle due risponda a *quale* varco lo decide il micro-step in cui i mover entrano nella
zona. Cambiare un waypoint di un mover — un dato che quegli scenari non stanno verificando — **scambia le due
risposte senza che nulla lo dica**: lo scenario resta verde e verifica un'altra cosa.

Le altre due regole del mandato sul matching **esistevano già** e non andavano costruite:

* opportunità senza risposta in uno scenario scriptato → `ERROR` (*«finestra aperta per 'X' senza una
  decisione che la nomini»*);
* risposta dichiarata e mai consumata → `ERROR`, contata in `FRTTestResult::ScriptedDecisionsUnused`.

### 1.3 Il seam del decisore, e cosa può arrivarci

```cpp
DECLARE_DELEGATE_RetVal_TwoParams(FString, FRTReactionDeciderSignature,
    const FRTReactionOpportunity& /*Opportunity*/, int32 /*OwnerUnitId*/);
```

`FRTReactionOpportunity` è `Key` + `AllowedResponses`, e il suo docstring dichiara un **elenco chiuso di
campi**, protetto da `RefactorTactics.Overwatch.OpportunityLeaksNoFuture`:

> *«I bersagli viaggiano ACCANTO al DTO e non dentro … allargarlo per comodita' toglierebbe al progetto
> l'unica barriera che impedisce a un campo di informazione futura di entrare nel DTO.»*

`FRTReactionOpportunityKey` porta `TurnNumber · MacroPhase · MicroStepIndex · OwnerId · ReactionDefId · Seq`
ed **entra nell'hash del replay**. Quindi, di ciò che il mandato chiede al selettore:

* `reactor` → da `Key.OwnerId`, tradotto in id di scenario; ✅
* tipo di finestra → `Key.ReactionDefId` (`Action.Overwatch`, `Action.Brace`, …); ✅
* `triggerUnit` → dai token `FIRE:<id>` di `AllowedResponses`, che per l'Overwatch **sono** i mover entrati
  nella zona (`FRTOverwatchTrigger::TargetUnitIds`, *«uno per ogni risposta `FIRE:`»*); ✅
* `triggerCell` → **non c'è**, e portarcela significa allargare il DTO e la chiave del replay. ⛔

---

## 2. BLOCCO — `watchDirection` è già stato respinto, e il documento lo dice per nome

Il mandato chiede quattro facing distinti — `initialFacing`, `watchDirection`, `actionFacing`,
`finalFacing` — e prescrive: *«Se questa semantica confligge con un owner canonico, non modificarla
silenziosamente: documenta il conflitto e identifica la decisione che deve essere aggiornata.»* Confligge.

### 2.1 L'owner: [ADR-0005](../../decisions/adr-0005-orientamento.md) §4c

> **4c. Reazioni direzionali — il cono dell'Overwatch *è* il facing (E14)**
>
> *«La zona controllata di un Overwatch armato nasce dal facing dell'unità, **non** da una direzione
> dichiarata a parte come proponeva la nota sorgente (§10: `Direction: North-East`). Due sorgenti per la
> stessa cosa sarebbero due verità: chi arma la guardia decide dove guardare **orientandosi**.»*

La forma proposta dal mandato — `{"action": "Action.Overwatch", "watchDirection": "W"}` — è **la stessa
proposta che quella sezione respinge**, nella stessa forma.

### 2.2 Confermata due volte da allora

* **[D-020](../../decisions/RT_PDR_00_Decision_Log.md)** fissa la timeline nominata del facing e vi colloca
  l'Overwatch come **lettore**:
  `FacingStartOfRound → FacingAfterPrepActionTargeting → FacingAfterDash → FacingUsedByBlast →`
  `FacingUsedByOverwatch → FacingFinalAfterMove`. La tabella di ADR-0005 annota `FacingUsedByOverwatch` con
  *«il cono pianificato — coerente con §4c, che già lo diceva»*.
* **[D-365](../../decisions/RT_PDR_00_Decision_Log.md)** (2026-09-10, `FAC-5`) chiude la domanda gemella: una
  reazione **non** ruota chi reagisce, il default è `KeepFacing`, e *«la §4c e D-020 avevano ragione a
  nominare il facing dell'Overwatch come valore **letto**: resta tale»*.

### 2.3 E il codice lo implementa così

[`RTTurnManager.cpp:4549`](../../../Source/RefactorTactics/Turn/RTTurnManager.cpp)

```cpp
Armed.Facing = Unit->Facing; // il cono E' il facing (ADR-0005 §4c), dichiarato in Planning
```

### 2.4 Due dei quattro campi **esistono già e sono già separati**

| Chiesto | Oggi | Sede |
|---|---|---|
| `initialFacing` | `FRTScenarioUnit::Facing` — dato di **piazzamento** | `RTTestScenario.h` |
| `finalFacing` | `FRTScenarioIntent::Facing` + `bDeclaresFacing` — rotazione **dichiarata** (D-020, prodotta da `ARTPlayerController::HandleFacingSector`, [#737](https://github.com/DegrassiAaron/refactor-tactics-main/issues/737)) | `RTTestScenario.h` |
| `actionFacing` | **output della simulazione**, non un input: lo scrive il resolver e lo si verifica dal TurnLog (`ERTFacingOutcome::UsedByBlast`) | `RTTurnManager.cpp` |
| `watchDirection` | ⛔ **non esiste, ed è una decisione — non una lacuna** | ADR-0005 §4c |

⚠️ **Aggiungerlo al solo harness sarebbe peggio che non aggiungerlo**, per la regola che `RTTestScenario.h`
applica già a `DeclaredRotation` e `ReactionPlanning`: l'harness diventerebbe il **primo produttore** di un
campo che nessun giocatore può chiedere in partita — verdi che dicono che il giocatore può orientare la
propria guardia mentre non ha alcun modo di farlo. La chiave `facing` sull'intent è entrata **solo dopo** che
`#737` le ha dato un produttore, e la nota nel file racconta esattamente quell'attesa.

### 2.5 Cosa servirebbe per sbloccarlo

Una `D-nnn` nuova che **emenda ADR-0005 §4c** e risponde a tre domande che nessun documento corrente copre:

1. l'orientamento della guardia è un **secondo ingresso** in Planning, o resta il facing dell'unità?
2. se è un secondo ingresso, chi lo produce in partita — [D-367](../../decisions/RT_PDR_00_Decision_Log.md)
   ha appena fissato il facing finale sui sei triangoli della cella di destinazione, e un secondo selettore
   direzionale entra in conflitto con quel gesto;
3. entra nella timeline di D-020 come settimo punto, o **sostituisce** `FacingUsedByOverwatch`?

**Owner da coinvolgere**: [#152](https://github.com/DegrassiAaron/refactor-tactics-main/issues/152) (E14 ·
Overwatch e reazioni interattive) e [#339](https://github.com/DegrassiAaron/refactor-tactics-main/issues/339)
(`FAC-*`), con [#291](https://github.com/DegrassiAaron/refactor-tactics-main/issues/291) per l'ingresso.

### 2.6 Il resto della decisione utente sul facing: già vero, e già verificato

Il mandato chiede che `finalFacing` si applichi **anche** quando l'Overwatch spara, quando la risposta è
`HOLD` e quando nessuna opportunità si apre. È già la semantica di D-020 — il `Move`, ultimo, fissa
`FacingFinalAfterMove` — e il corpus la esercita (`Scenarios/Spec/Facing/`, con
`RefactorTactics.Scenario.DeclaredRotationScenariosPass` verde in questa misura). L'eccezione chiesta —
*«non si applica se l'unità è KO o non può ruotare»* — è coperta da `FAC-7`/D-365: nessuno status della v0.1
limita la rotazione, e un'unità abbattuta non esegue il proprio Move.

---

## 3. PROPOSTA — rimandata al proprio owner

### 3.1 Target tipizzato discriminato — owner: [#1119](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1119) (`RCI-1`)

Il mandato propone `{ "kind": "unit" | "cell" | "edge" | "direction" | "self", … }` al posto dei campi
paralleli `target` / `targetCell` / `edge` / `facing`.

🔴 **`ERTIntentTargetKind` non esiste nel codice**, misurato:

```bash
grep -rn "ERTIntentTargetKind\|FRTCanonicalIntent" Source/    # nessuna riga
```

e non è una svista: `#1119` è la issue di design che possiede la domanda, elenca `ERTIntentTargetKind` fra i
termini *«citati dalla firma proposta nel sorgente … non sono stati misurati»*, e dichiara nel proprio
*Out of scope*: *«Implementare uno qualunque dei quattro: questa issue produce risposte, non codice.»*

Introdurre il target discriminato **nell'harness** prima che `RCI-1` risponda creerebbe la seconda verità che
quella issue esiste per prevenire: una tassonomia dei bersagli nel formato scenario e un'altra, diversa, nel
Canonical Intent quando arriverà — due formati serializzati da riconciliare, uno dei quali entra nel replay.

⚠️ E il modello a slot **non è arbitrario**: i campi paralleli riflettono l'economia del turno di
[D-028](../../decisions/RT_PDR_00_Decision_Log.md) — `Dash` prende lo slot movimento, `Ability` la
principale, e *«schivo e sparo»* è un turno legale che l'intent deve poter esprimere. La normalizzazione è
possibile, ma è un cambiamento del **modello del turno**, non una pulizia del formato.

### 3.2 `actions[]` — più decisioni compatibili nello stesso turno

Il mandato chiede un array generico di azioni per unità. Oggi il piano **è già** multi-decisione — movimento,
abilità principale, scatto rapido e reazione armata convivono nello stesso `FRTScenarioIntent` — ma per
**slot nominati**, non per elenco. Un array generico permetterebbe di dichiarare due azioni nello stesso
slot, che è precisamente ciò che il piano non può essere; la validazione dell'economia del turno è
[#609](https://github.com/DegrassiAaron/refactor-tactics-main/issues/609) /
[#608](https://github.com/DegrassiAaron/refactor-tactics-main/issues/608).

Il test `RefactorTactics.Scenario.EveryD025ActionIsExpressible` (verde in questa misura) verifica già che
**tutte** le azioni universali di D-025 — `Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch`
— siano esprimibili nel formato corrente, e che le abilità d'eroe usino lo stesso involucro identificandosi
per Stable Action ID. La copertura chiesta dal mandato c'è già.

### 3.3 Checkpoint di fase e `afterEvent` — non implementati

`PlanningLocked · PrepEnded · DashEnded · BlastEnded · MoveEnded · CleanupEnded` non esistono nel formato.
L'assenza non è un difetto silenzioso: `expect[]` verifica lo stato **finale** e il TurnLog, e
`LogEventOrder` copre già l'ordine relativo di due eventi. Un checkpoint di fase aggiunge la capacità di dire
**dove** uno stato cambia, che oggi si ottiene solo indirettamente.

⚠️ **Non è una fetta piccola**: richiede un punto di sospensione per macro-fase dentro `LockInAndResolve`,
cioè nel cuore del resolver. Il vincolo del mandato — *«niente frame, animazioni, `DeltaTime` o callback
presentation-only»* — è già l'invariante 6 di `AGENTS.md`, quindi la forma è concordata; è il **seam** che non
esiste.

### 3.4 Separazione sintattica `metadata`/`setup` nel JSON — costo alto, valore basso *oggi*

Annidare le chiavi esistenti sotto `metadata:` e `setup:` è meccanicamente possibile e semanticamente neutro.
Costa il corpus versionato intero (`git ls-files Scenarios | grep -c '\.json$'` → `xx` file), il loader, il
writer, il Draft, l'authoring e il tooling Editor. Non chiude nessun difetto misurato — la separazione
**semantica** esiste già nel data model — e va fatta quando c'è un consumatore che la richiede, cioè il
Composer visuale ([#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105),
[#1628](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1628)).

---

## 4. DECISIONE UTENTE implementata — il selettore semantico `on`

### 4.1 Vecchio schema

```json
"decisions": [
  { "unit": "V1", "respond": "HOLD" },
  { "unit": "V1", "respond": "FIRE", "target": "R1" }
]
```

Abbinamento **posizionale**: prima voce → prima finestra di `V1`.

### 4.2 Nuovo schema (`"version": 5`)

```json
"decisions": [
  { "on": { "reactor": "V1", "reaction": "Action.Overwatch", "triggerUnit": "F1" },
    "respond": "HOLD" },
  { "on": { "reactor": "V1", "reaction": "Action.Overwatch", "triggerUnit": "R1" },
    "respond": "FIRE", "target": "R1" }
]
```

| Regola | Esito |
|---|---|
| `on` presente e vuoto | `ERROR` — *«'on' non dichiara nessun vincolo»* |
| `on` senza `reactor` | `ERROR` — *«'on' richiede 'reactor'»* |
| `unit` e `on` insieme | `ERROR` — *«non convivono»*: sarebbero due posti per lo stesso fatto |
| `on` non è un oggetto | `ERROR` — *«'on' deve essere un oggetto»* |
| chiave sconosciuta dentro `on` (compreso `triggerCell`) | `ERROR` con l'elenco delle chiavi che esistono |
| `reactor` / `triggerUnit` non schierati | `ERROR` col nome dell'id |
| `on` sotto `"version": 5` | `ERROR` dal gate di versione, che nomina la build |
| zero finestre soddisfano il selettore | `ERROR` a fine turno — *«nessuna finestra ha soddisfatto il selettore»* |
| **più** finestre soddisfano lo stesso selettore | `ERROR` — *«il selettore … è ambiguo»*, distinto dalla finestra scoperta |
| finestra senza decisione che la nomini | `ERROR` (già esistente) |
| turno senza `decisions` | `DecisionOnTimeout` → `HOLD`, il comportamento normale del gioco |

⛔ **`triggerCell` non è una chiave, e il rifiuto lo dice.** La cella del trigger non è in
`FRTReactionOpportunity` (§1.3): un selettore che la nominasse dichiarerebbe un vincolo che il matching non
può verificare — un campo che dichiara e non verifica, cioè il difetto che questo formato rifiuta ovunque.
Portarla richiederebbe di allargare la `Key` che entra nell'hash del replay, e quello appartiene all'owner del
replay, non all'harness.

### 4.3 Strategia di compatibilità

1. **Modello interno normalizzato.** Il reactor resta **un campo solo** — `FRTScenarioDecision::Unit` — che il
   loader popola da `on.reactor`. Validazione, messaggi d'errore e matching per unità continuano a leggere una
   verità sola; il selettore aggiunge soltanto i vincoli in più.
2. **Lettura retrocompatibile.** Una decisione senza `on` conserva l'abbinamento per ordine. Nessuno scenario
   del corpus cambia comportamento: nessuno usa `on`, e le versioni presenti si contano con
   `grep -h '"version"' Scenarios/**/*.json | sort | uniq -c`.
3. **Writer.** Emette `on` quando c'è, `unit` altrimenti, **mai entrambi**; `MinimumVersionFor` restituisce
   `5` in presenza di un selettore ed è controllato **per primo**, perché quella funzione ritorna al primo
   requisito trovato — un `on` accanto a una risposta di profilo avrebbe altrimenti ottenuto la `3`, cioè una
   versione che il loader poi rifiuta.
4. **Round-trip verificato.** Il comparatore campo-per-campo di `RTScenarioWriterTests.cpp` confronta ora
   anche `bHasSelector`, `On.Reactor`, `On.Reaction`, `On.TriggerUnit`.
5. **Migrazione: nessuno scenario è stato migrato**, deliberatamente. Il mandato prescrive di non migrare
   finché loader, writer e compatibilità non sono verdi — ora lo sono, e la migrazione è il follow-up §7.1.

### 4.4 File modificati

| File | Cosa |
|---|---|
| `ScenarioHarness/RTTestScenario.h` | `FRTScenarioOpportunitySelector` + `On`/`bHasSelector` su `FRTScenarioDecision` |
| `ScenarioHarness/RTScenarioLoader.h` | `SupportedVersion` 4 → 5, con l'argomento del verso |
| `ScenarioHarness/RTScenarioLoader.cpp` | parsing di `on`, chiavi note, forma e gate di versione in `ValidateDecisionForm` (condivisa fra parser e `Validate`), `triggerUnit` schierato in entrambi |
| `ScenarioHarness/RTScenarioWriter.cpp` | serializzazione di `on`, `MinimumVersionFor` |
| `ScenarioHarness/RTScenarioSession.cpp` | matching semantico, errore di ambiguità, messaggio del residuo |
| `ScenarioHarness/RTScenarioDraft.cpp` | il ritiro di un'unità conta anche `on.triggerUnit` fra i riferimenti appesi |
| `Tests/RTScenarioDecisionSelectorTests.cpp` | **nuovo** — quattro test |
| `Tests/RTScenarioWriterTests.cpp` | il selettore entra nel comparatore del round-trip |

---

## 5. Verifica

**Ambiente della misura**: `HEAD = f4fceb0a`, working tree pulito (`git diff HEAD` vuoto, nessun untracked),
`Binaries/Win64/UnrealEditor-RefactorTactics.dll` ricompilato dopo l'ultimo commit. Nessun altro processo
`UnrealEditor*` durante le run (`Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'"` → vuoto).
Engine: installed build `D:/EpicGames/UE_5.8`, `InstalledBuild.txt` presente. Ogni run headless ha passato
`-abslog` nello scratchpad di sessione ([D-362](../../decisions/RT_PDR_00_Decision_Log.md)).

| Gate | Esito |
|---|---|
| Build `RefactorTacticsEditor Win64 Development` | `PASS` |
| `Automation RunTests RefactorTactics.Scenario` | `PASS` — 178 verdi, 1 rosso **preesistente** (§7.7) |
| `Automation RunTests RefactorTactics` (suite intera) | `PASS` — 2300 verdi, 4 rossi **tutti preesistenti**, chiusi da allora (§7.7) |
| Corpus scenari | `122 PASS, 9 BLOCKED, 1 dichiarati expected-fail` |
| Determinismo | `PASS` — coperto dalla suite intera (`HexSim.ReplayDivergenceZero`, corpus `repeatCount`) |
| Replay | `N/A` — nessun formato di replay toccato; il selettore vive nel file di scenario |
| Privacy | `N/A` — nessun dato replicato toccato |
| PIE | `NOT RUN` — nessun comportamento in-game modificato: il selettore è authoring di scenario |
| Packaged | `NOT RUN` |

### 5.1 I quattro test nuovi

| Test | Esito |
|---|---|
| `RefactorTactics.Scenario.SelectorLoadsAndRoundTrips` | `PASS` |
| `RefactorTactics.Scenario.SelectorRejectsMalformedForms` | `PASS` |
| `RefactorTactics.Scenario.SelectorPicksWindowByTrigger` | `PASS` |
| `RefactorTactics.Scenario.SelectorAmbiguousIsAnError` | `PASS` |

`SelectorPicksWindowByTrigger` è quello che porta il valore: le due decisioni sono dichiarate nell'ordine
**opposto** a quello in cui le finestre si aprono, e l'assertion è sulla **posizione finale** dei due mover —
un `FIRE` tronca il movimento, un `HOLD` no. Con l'abbinamento per ordine la finestra di `M1` riceverebbe un
`FIRE:<M2>` non fra le sue risposte legali, `IsResponseAllowed` lo respingerebbe, e `M2` non verrebbe mai
fermata. Contare le decisioni applicate non sarebbe bastato: nel ramo posizionale una delle due viene
comunque consumata.

### 5.2 I tre casi che il mandato chiede di verificare

Eseguiti dal corpus (`RefactorTactics.Scenario.EveryShippedScenarioRuns`), su `f4fceb0a`:

| Scenario | Esito |
|---|---|
| `Movement.Basic` | `PASS (2/2 assertion, 1 turni)` |
| `Spec.Overwatch.HoldThenFire` | `PASS (6/6 assertion, 2 turni)` |
| `RT_Showcase_Relay_v01` | `PASS (22/22 assertion, 8 turni)` |

⚠️ Il log contiene anche righe `FAIL` per i primi due: **sono intenzionali e appartengono ad altri test** —
`RefactorTactics.Simulation.StateHashDistinguishesOutcomes` muta `Movement.Basic` per verificare che l'hash
distingua gli esiti, e `RefactorTactics.Replay.Verifier.OrphanRecordedResponseIsReported` muta
`HoldThenFire` per verificare la diagnostica. Entrambi i test contenitori sono `Success`.

### 5.3 Test non eseguiti

* **PIE** e **packaged**: nessun comportamento in-game cambia.
* **Radar di `tools/`**: non eseguiti — il write-set non tocca documenti generati né asset. Fa eccezione
  `node tools/radar/scenario-notes.ts --check`, che sarà **pertinente al follow-up §7.1** (la migrazione del
  corpus tocca la prosa degli scenari), non a questa fetta che non modifica nessun `.json`.

---

## 6. Rischi residui

1. **La finestra scoperta e l'ambiguità si distinguono a runtime, non al caricamento.** Un selettore troppo
   largo è legale nel file e diventa `ERROR` solo quando due finestre lo soddisfano davvero — cioè in
   un'esecuzione che le apre entrambe. È intrinseco: quante finestre si aprano lo decide la simulazione, ed è
   ciò che lo scenario sta misurando.
2. **`triggerUnit` su una finestra senza `FIRE:`** — un profilo di `Brace`, per esempio — non può essere
   soddisfatto da nessuna opportunità, e la decisione finisce nel residuo. Il messaggio lo dice, ma non
   spiega **perché**: chi lo incontra deve sapere che quel vincolo vale solo dove esistono bersagli offerti.
3. **`on.reaction` accetta qualunque `FName`**: il loader non la confronta con un catalogo. Un refuso
   (`Action.Overwtach`) non trova finestre e cade nel residuo invece di essere rifiutato al caricamento. È la
   stessa classe di difetto che [#2698](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2698)
   misura per gli altri campi dell'intent.
4. **Il corpus non usa ancora il selettore**, quindi la strada nuova è esercitata solo dai test: finché
   `HoldThenFire` e `RT_Showcase_Relay_v01` restano posizionali, il difetto §1.2 resta vivo **su di loro**.

---

## 7. Follow-up

| # | Cosa | Dipendenza |
|---|---|---|
| 7.1 | Migrare al selettore gli scenari con `decisions`: `Spec.Overwatch.HoldThenFire` e `RT_Showcase_Relay_v01` (i due a rischio), più `Spec/Brace/ProfileChangesResponse`, `Spec/Facing/OverwatchHitCameFromSide`, `Spec/Overwatch/ThreeArmedWatchersOnOneEntry` | questa fetta ✅ |
| 7.2 | `on.reaction` validata contro il catalogo delle reaction al caricamento | — |
| 7.3 | Checkpoint di fase (`PrepEnded` … `CleanupEnded`) e `afterEvent`: serve un seam di sospensione per macro-fase in `LockInAndResolve` | resolver |
| 7.4 | Decisione su `watchDirection`: emendare o confermare ADR-0005 §4c | `#152` · `#339` · `#291` |
| 7.5 | Target tipizzato discriminato | `#1119` (`RCI-1`) risponde prima |
| 7.6 | Separazione sintattica `metadata`/`setup` nel JSON | consumatore: Composer (`#1105`, `#1628`) |
| 7.7 | ✅ Difetti **preesistenti** trovati durante questa verifica — chiusi da [#2862](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2862) | ~~`#2491`~~ |

### 7.7 — Quattro test sono rossi su `main`, e nessuno per questa fetta

Misurati **due volte**: sulla suite intera a `f4fceb0a` e, come controprova, ricompilando e rieseguendo gli
stessi quattro test su `origin/main` = `18065c28`. Falliscono identici in entrambe.

| Test | Messaggio |
|---|---|
| `RefactorTactics.Anim.RosterMigrationKeepsPaths` | attende `.../ParagonAevik/.../Aevik/Animations/Idle`, ottiene `.../ParagonGadget/.../Gadget/Animations/Idle` |
| `RefactorTactics.IconCatalog.RealCatalogCoversRequiredIds` | *«2 chiave/i richiesta/e non coperta/e: `UI.Icon.Identity.Aevik` \| `UI.Icon.Identity.Muiren`»* |
| `RefactorTactics.Packaging.RequiredAnimationClipsAreCooked` | la mesh Paragon di Aevik non è referenziata da un asset versionato |
| `RefactorTactics.Scenario.WriterKeepsIdentityAcrossPaths` | *«Expected 'il tag 'gadget' e' filtrabile' to be true»* |

Sono **una sola famiglia**: la migrazione dei nomi d'eroe di
[#2491](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2491) (`Gadget → Aevik`,
`Wraith → Ivrin`, …), che ha spostato i **dati** e ha lasciato indietro asset, catalogo icone e — nell'ultimo
caso — l'assertion che li interroga.

Il quarto è a una riga di distanza: `RTScenarioWriterTests.cpp:44` dichiara
`"tags": ["movement", "Aevik", "regressione"]` mentre la riga `424` cerca ancora `"gadget"`. Misurato:
`git log -S'"tags": ["movement", "Aevik", "regressione"]' -- <file>` → `203805b9 refactor(2491): Hero.Gadget
diventa Hero.Aevik`. Il diff di questa fetta su quel file sono sette righe nel comparatore del round-trip
(~riga 190), non alla 424.

⛔ **Non corretti in questa fetta, deliberatamente.** Appartengono a `#2491`, e correggerli dentro questa
PR sarebbe stato il refactor collaterale che `AGENTS.md` §8 vieta.

> ✅ **Chiusi il 2026-09-10 da [#2862](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2862)**, in
> un passaggio separato e su un branch proprio. Tre erano refusi di un passaggio di rename che aveva toccato
> letterali **non RT-owned** — i nomi dei pack Paragon, che nessuna migrazione RT rinomina — e il quarto era
> vero: `generate_hud_assets.py` disegnava `Identity.Gadget`/`Identity.Phase` mentre `RequiredIconIds()`
> deriva le chiavi dal roster. Riaperta e richiusa `#2551` con la misura.
>
> ⚠️ **La misura di questa fetta resta quella dichiarata sopra**, presa quando i quattro erano ancora rossi:
> non è stata riscritta a posteriori. Il loro esito non dipendeva da questo write-set — è ciò che la
> controprova su `origin/main` aveva già dimostrato.
