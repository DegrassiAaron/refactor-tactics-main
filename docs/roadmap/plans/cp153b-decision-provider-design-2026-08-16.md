# CP 15.3 metà B — design del `DecisionProvider` e dello script di decisione

> `CURRENT` · **Stato**: design approvato in sessione, **non ancora implementato** ·
> **Data**: 2026-08-16
> **HEAD del design**: `cb59c58d` (`feat/512-decisionprovider-finestra`, worktree `D:/rt-simulation`)
> **Oggetto**: [`#512`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/512) nel corpo
> riscritto dallo spec panel del 2026-08-16 (`05:37Z`), **sei** voci di DoD.
> **Owner del write-set**: track `simulation` in
> [`../parallel-batch.yaml`](../parallel-batch.yaml) (`ACTIVE`, quattro path).
> **Riferimenti**: [`adr-0004-finestre-di-reazione.md`](../../decisions/adr-0004-finestre-di-reazione.md) ·
> [`adr-0009-replay-logico-canonico.md`](../../decisions/adr-0009-replay-logico-canonico.md) ·
> [`cp145-finestra-overwatch-spec-panel-2026-08-14.md`](cp145-finestra-overwatch-spec-panel-2026-08-14.md) ·
> [`../../technical/scenario-map.md`](../../technical/scenario-map.md)
> **Particolarità**: tre delle quattro decisioni di questo design sono state prese **contro una misura che
> ha falsificato l'ipotesi di partenza**, e la quarta ha ristretto il write-set prima che una riga di
> codice lo toccasse. Le misure sono riportate accanto alla decisione, non in appendice.

---

## 1. Il verdetto in una riga

Il seam esiste già e nessuno lo usa: il lavoro non è costruire un punto d'iniezione, è **dargli un
iniettore che parli il vocabolario dello scenario** — e accorgersi che il vocabolario del gioco non è
scrivibile a mano.

## 2. Cosa esiste, misurato su `cb59c58d`

| Affermazione | Misura |
|---|---|
| il seam c'è | `ARTTurnManager::ReactionDecider`, delegate `public`, **4** occorrenze tutte in `RTTurnManager.{h,cpp}` |
| nessuno lo binda | `grep -rn "ReactionDecider.Bind" Source/` → **0** |
| la finestra si apre in partita | `RTTurnManager.cpp:5093` chiama `BuildOverwatchTriggers`, consegnato da `#165` |
| la capability è ancora chiusa | `DecisionBoundary` in `KnownUnavailableCapabilities()`, `RTScenarioSession.cpp:161` |
| quanti la chiedono | **7** turni in 7 file, contati col contatore Python che `RTScenarioSession.cpp` pubblica nel proprio commento |

Il fallback di `AskReactionDecision` è fail-closed nel verso giusto — senza decisore la charge non si
spende — quindi **il ramo interattivo non è mai stato esercitato**: verde e muto.

## 3. Le quattro decisioni

### 3.1 Le decisioni entrano come dato, e un test può sovrascriverle

Due sorgenti: lo scenario le porta nel proprio file, un test può bindare il proprio decisore e vincere.
La precedenza non è una regola scritta da qualche parte, è **la forma del delegate**: uno slot solo, e la
session binda soltanto quando `ReactionDecider.IsBound()` è falso. Chi binda prima vince, e in un test
bindare prima è naturale.

⚠️ Il prezzo è che uno scenario con `decisions` e un test che sovrascrive **ignora il JSON in silenzio**.
Perciò la provenienza si scrive: vedi §3.4.

### 3.2 Lo script parla il vocabolario dello scenario, non quello del gioco

Il vincolo che ha deciso lo schema, e non era previsto:

- l'identità di una opportunity è `T4|P3|M7|U12|action.overwatch|S0` — contiene `MicroStepIndex` e
  l'`OwnerId` di **runtime**. Uno scenario non può scriverla;
- il token di risposta è `FIRE:<TargetUnitId>`, e anche quel numero è un id di runtime:
  `FireResponse(int32)` lo dichiara nella firma.

∴ indirizzare per id, in entrambi i sensi, è fuori. Lo scenario nomina le unità come già fanno gli
`intents`, e la traduzione avviene dove esiste la mappa — `UnitsById` in `RTScenarioSession`.

**Schema.** Chiave nuova a livello di turno, `decisions`, array ordinato:

```json
"turns": [{
  "_turno": "T4 — Overwatch",
  "requires": ["DecisionBoundary", "DeclaredRotation"],
  "intents": [ "..." ],
  "decisions": [
    { "unit": "Guardia", "respond": "FIRE", "target": "Corsa" },
    { "unit": "Guardia", "respond": "HOLD" }
  ]
}]
```

⚠️ `Guardia` e `Corsa` sono **id di scenario**, cioè etichette locali al file — non id eroe. La distinzione
conta: l'id eroe è una chiave di catalogo e oggi il catalogo dichiara **solo** i quattro nomi legacy
(`Hero.Flux`, `Hero.Riva`, `Hero.Bastion`, `Hero.Vektor`; misurato in `RTHeroCatalogLibrary.cpp`, e
`Gadget`/`Phase`/`Riktor`/`Wraith` hanno **zero** occorrenze in tutto `Source/` perché la fetta 3 di `D-130`
(`#753`) non è stata eseguita). Una prima stesura di questo esempio usava `riktor` e `wraith` come id di
scenario: legittimo di per sé, ma affiancato a un piano che scrive `Hero.Bastion` invitava a leggerli come
id eroe e a «correggere» il codice in qualcosa che non risolve.

`target` è **obbligatorio con `FIRE` e vietato con `HOLD`** — non ignorato: un campo che c'è e non conta è
il modo in cui uno scenario dice una cosa e ne verifica un'altra.

**Abbinamento**: quando si apre una finestra per un'unità, si consuma **la prima decisione non ancora
usata che nomina quell'unità**. Non per ordine di apertura: l'ordine dei micro-step cambia quando cambia il
movimento, e una coda posizionale risponderebbe per un'altra unità **in silenzio**.

**Tipi** (`ScenarioHarness/RTTestScenario.h`): `FRTScenarioDecision { Unit, Respond, Target }` e
`TArray<FRTScenarioDecision> Decisions` su `FRTScenarioTurn`, accanto a `Requires`.

**Loader** (`RTScenarioLoader.cpp`, dentro il ciclo dei turni):

- `KnownDecisionKeys` stretto, con l'elenco atteso **generato dal set** e ordinato — la lezione che `edge`
  e `facing` hanno già lasciato scritta lì accanto: le due copie vivono a trecento righe di distanza e
  nessun gate le confronta;
- **`KnownTurnKeys`, che oggi non esiste**. Senza, un refuso `desicions` viene ignorato e il turno cade su
  `HoldNoDecider`: verde per il motivo sbagliato. Misurato che il corpus intero passa — le chiavi di turno
  nei 77 file sono **quattro**: `intents` (113), `requires` (36), `_turno` (64), `_nota` (3), e le due con
  `_` sono già la convenzione dei commenti;
- `unit` e `target` sconosciuti sono **errore di caricamento**, non `Blocked`: un nome che non esiste non è
  una capability mancante.

**Versione di formato**: `SupportedVersion` **1 → 2**, e gli scenari che usano `decisions` dichiarano
`"version": 2`. Il gate `Version > SupportedVersion` è l'unico meccanismo per cui una build vecchia
*rifiuta* invece di ignorare, e da `#926` gli scenari viaggiano dentro il pacchetto: la coppia build/dato
può disallinearsi davvero. I 76 file esistenti restano a 1 e non si toccano. ⚠️ Misurato che nessun branch
vivo stava già prendendo la versione 2.

### 3.3 La mutazione è comportamentale, non sul tipo

Il DoD chiede che «sostituendo il provider con uno che restituisce un **esito** invece di una decisione,
cada almeno uno scenario». Ma la firma è `FString(const FRTReactionOpportunity&, int32)`: **un esito non è
già esprimibile**, e preso alla lettera quel test non ha una premessa costruibile — lo stesso caso del test
rimosso in `DeriveOpportunityId`, che era stato tenuto verde pur non potendo fallire.

La verifica diventa quindi **eseguibile**: si sostituisce il provider scriptato con uno che risponde sempre
`HOLD`, e almeno un `expect` deve diventare rosso. Prova che il turno **dipende** dalla decisione — che è
la cosa di cui il golden replay di `#170` ha bisogno. Il divieto sull'esito resta come proprietà del tipo,
dichiarata nel commento e non in un test che non può fallire.

### 3.4 La provenienza va nel referto, non nel TurnLog

`ERTReactionDecisionOutcome` vive in `Turn/RTTurnLog.h` con **7** consumatori in `RTTurnLogLibrary.cpp`:
nessuno dei due nel write-set, e toccare il log serializzato muove i golden.

E non serve: al replay serve **quale** decisione, non **chi** l'ha fornita. La prima è stato di gioco, la
seconda è diagnostica. `result.json` porta quindi per turno la sorgente attiva — `scenario`,
`test-override`, `none` — e le decisioni applicate. Misurato che il referto non è mai confrontato per
intero (`RTScenarioRunnerTests.cpp:229` lo rilegge solo per esistenza) e che ha una `schemaVersion` propria,
che cresce di uno.

## 4. Il meccanismo, in ordine

1. `RTScenarioSession` spawna il `TurnManager` e popola `UnitsById` (`RTScenarioSession.cpp:485-495`);
2. subito dopo — prima serve la mappa, o la traduzione non ha con cosa tradurre — binda `ReactionDecider`,
   **solo se libero**;
3. si apre una finestra: il decisore risale allo scenario id del proprietario, prende la prima decisione
   non consumata che lo nomina, e costruisce il token — `HOLD` → `HoldResponse()`, `FIRE` →
   `FireResponse(TargetUnit->UnitId)`;
4. nessuna decisione che combacia → **stringa vuota**, che è già «non ho risposto» e ricade su
   `DecisionOnTimeout`. Il comportamento di oggi resta intatto per i turni che non scriptano nulla;
5. `TearDown` **sbinda**, accanto a dove il manager viene distrutto: un delegate che sopravvive a uno
   scenario risponderebbe al successivo con la coda del precedente.

⛔ **La legalità della risposta non si verifica nell'harness.** `AskReactionDecision` la controlla già con
`IsResponseAllowed`, e il codice dichiara lì perché deve stare in un posto solo. Se la risposta è illegale
il manager produce `HoldRejected`, e la session lo **legge dal TurnLog** e lo trasforma in un fallimento
che nomina la decisione colpevole, invece di lasciarlo passare come un `HOLD` qualunque.

**Il residuo è un fallimento, non un avanzo.** A fine turno:

1. decisioni dichiarate e **mai consumate** → il turno fallisce, nominandole. Una decisione che non ha
   trovato la sua finestra descrive qualcosa che non è successo;
2. il turno dichiara `decisions` e una finestra si è aperta **senza risposta** → fallisce anch'esso. Senza,
   uno scenario può scriptare due decisioni, vederne applicare una, e restare verde.

## 5. Il vocabolario: `Reaction` si divide fra i due nomi che esistono

Il commento che scade dice: *«la DECISIONE su un'opportunity a due risposte non appartiene a `Reaction`
(vedi sotto)»* — e «sotto» è `DecisionBoundary`, che il vocabolario ha già. La divisione è quindi di
**significato**, fra due nomi esistenti: `Reaction` resta la cardinalità `<= 1` (E5, scatta o non scatta) e
`DecisionBoundary` prende la `>= 2`.

Nessun terzo nome, e la ragione è misurata: i **3** turni che chiedono `Reaction`
(`Combat/CounterStrikesBack`, `Visual/Reaction/Deflection`, `Visual/Reaction/Interposition`) sono tutti nel
regime `<= 1`. Un nome nuovo li costringerebbe a cambiare senza cambiare ciò che chiedono, e `Scenarios/`
è `integration_only`.

## 6. Due fasi, e il confine non è dove sembrava

Scoprire `DecisionBoundary` sblocca **3** dei 7 turni che la chiedono — showcase `T4`,
`Spec/Overwatch/HoldThenFire` `T2`, `Spec/Brace/ProfileChangesResponse` `T2`. I quattro `Spec/Clash/*`
restano `Blocked` su `ReactionClash`, ancora indisponibile.

🔴 **Ma quei due `Spec/*` non sono mai stati eseguiti**, e `EveryShippedScenarioRuns` accetta `BLOCKED`
mentre **non** accetta `FAIL`. Per `HoldThenFire` la caduta è prevista dalla misura: si aspetta `F1` a
`[-2,0,0]`, cioè un movimento **troncato da un `FIRE`**, e senza decisione scriptata risponderebbe `HOLD`,
l'unità arriverebbe più lontano e l'aspettativa cadrebbe. ⚠️ Per `ProfileChangesResponse` il suo `expect`
è stato letto ma **non eseguito**: che cada è probabile per la stessa ragione, non misurato. Si scopre
eseguendolo, in fase B, non deducendolo qui.

| Fase | Chi | Cosa |
|---|---|---|
| **A** | track `simulation` | schema, tipi, loader, bind, precedenza, traduzione, coda, residuo, provenienza, mutazione — provati su scenari **costruiti in memoria**. `DecisionBoundary` resta indisponibile: nessuno scenario cambia esito |
| **B** | atto d'integrazione | le `decisions` di showcase `T4`, `HoldThenFire` e `ProfileChangesResponse`, **e nello stesso atto** la capability che si scopre |

∴ **La track da sola non chiude tutte e sei le voci**, e va detto nella PR invece che dedotto: la voce 5 —
capability disponibile, showcase oltre il `T4` — è di fase B.

## 7. Write-set

Emendato **prima** di scrivere, con la PR [#979](https://github.com/DegrassiAaron/refactor-tactics-main/pull/979):

```
Source/RefactorTactics/ScenarioHarness/
Source/RefactorTactics/Tests/RTShowcaseScenarioTests.cpp
Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp
docs/roadmap/plans/cp153b-decision-provider-design-2026-08-16.md
```

Escono `Turn/RTTurnManager.{h,cpp}` e `Turn/RTReactionOpportunityTypes.{h,cpp}`: erano una prenotazione e
non un vincolo. Il referto entra come **file** e non come cartella — `docs/roadmap/plans/` è scritta in
questo momento da quattro branch remoti e da un worktree con un referto *untracked*.

⚠️ `RTScenarioSession.cpp` alimenta `project-graph.json` e `scenariomap.shortlist.md`: chi lo tocca eredita
**due** `--check`.

## 8. I test

| Test | Cosa falsifica |
|---|---|
| `RefactorTactics.ShowcaseRelay.DecisionProviderIsInjectable` | la voce di DoD che lo nomina |
| chiave sconosciuta in `decisions` → errore | un refuso che diventerebbe un `HOLD` silenzioso |
| chiave di turno sconosciuta → errore | `desicions` ignorato dal loader |
| `target` con `HOLD` → errore · assente con `FIRE` → errore | un campo che c'è e non conta |
| `FIRE` si traduce nell'id di runtime giusto | la traduzione, che è l'unica parte che il JSON non può esprimere |
| la coda si consuma in ordine, per unità | due decisioni per la stessa unità nello stesso turno |
| decisione non consumata → turno rosso | uno scenario che descrive una finestra che non si è aperta |
| finestra senza risposta in un turno con `decisions` → turno rosso | due decisioni scritte, una applicata, verde |
| decisore bindato dal test vince, e il referto dice `test-override` | la seconda sorgente che ignora il JSON in silenzio |
| **mutazione**: provider sempre `HOLD` → almeno un `expect` rosso | uno scenario che sarebbe verde comunque |

## 9. Cosa questo design NON fa

- **nessun timer reale** in Fast/Headless, nessun UMG: la UI della finestra è CP 14.6 (`#166`);
- **nessun secondo seam**: `ReactionDecider` si sostituisce, non si affianca. Il seam generale di `D-101`
  è `#542`, ed è un'altra release — `RTTurnManager.h:603` scrive per esteso che *«sono tre cose in tre
  release»*;
- **nessuna capability dichiarata disponibile prima del suo produttore**, che qui significa: prima dei dati
  che la rendono rispondibile;
- **nessun campo nuovo nel TurnLog**, quindi nessun golden da ribasare in fase A.
