# ADR-0010 — Lo Scenario Harness si affaccia su Blueprint da una porta, non da tutte le finestre

> `CANONICAL` · **Stato**: Accettato — contratto implementato, operazioni di editing da implementare (TD-CODE-02)
> **Data**: 2026-08-27 · **Decisore**: utente (dev singolo)
>
> **Abilita**: [#1115](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1115) ·
> [#1116](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1116) ·
> [#1117](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1117) — epic
> [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105)
> **Poggia su**: [#1114](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1114) (il writer, senza il
> quale un authoring visuale dovrebbe conoscere il JSON da sé) ·
> [D-127](RT_PDR_00_Decision_Log.md) (un tipo marcato si legge come authority) ·
> [`spec-tactical-designer.md`](../technical/tooling/spec-tactical-designer.md) §3 (l'invariante unidirezionale)
> **Non tocca**: il formato scenario, `SupportedVersion`, il resolver, il runner, il TurnLog, né alcun hash

## Contesto

`URTScenarioLoader`, `URTScenarioRunner` e `FRTScenarioSession` esistono, sono testati e girano headless.
**Nessuno dei tre è raggiungibile da Blueprint**: misurato il 2026-08-27, `grep -c UFUNCTION` su
`Source/RefactorTactics/ScenarioHarness/` restituisce **zero** su tutti i file. Le due classi sono
`UBlueprintFunctionLibrary` con soli metodi `static` C++ — che nel repository è un modo diffuso di usare quella
base come *namespace*, non come esposizione: la maggior parte delle venti `UBlueprintFunctionLibrary` del modulo
non ha un solo `BlueprintCallable`.

Questo è il muro contro cui SC-2, SC-3 e SC-4 vanno a sbattere prima di cominciare. Le tre issue chiedono un
authoring visuale che *apra, modifichi, validi, salvi ed esegua* uno scenario; il terminale Editor lavora in
Widget/UMG e Blueprint. Senza una porta, l'Editor ha due sole strade, **ed entrambe sono quelle che il §3 della
spec vieta**: reimplementare in Blueprint ciò che non può chiamare, oppure toccare il JSON da sé.

La domanda non è dunque *se* esporre, ma **quanto**, e la risposta larga è più pericolosa di quella stretta:
marcare `BlueprintType` le nove `USTRUCT` del formato renderebbe Blueprint capace di costruire un
`FRTTestScenario` membro per membro — cioè uno scenario incoerente — e congelerebbe come API pubblica un modello
che è ancora in movimento (`Tags` gli è stato aggiunto ieri da `#1114`).

⚠️ **Il precedente esiste già nel repository e non è stato inventato qui.** `URTReplayViewerSubsystem` espone
diciannove `UFUNCTION` a una UI di replay, e la logica **non** sta lì: sta in `FRTReplayViewModel`, una
`USTRUCT()` non-`BlueprintType` con test propri (`RTReplayViewModelTests.cpp`), mentre il subsystem ha i suoi
(`RTReplayViewerSubsystemTests.cpp`). Ciò che attraversa il confine sono DTO dedicati — `FRTReplayPosition`,
`FRTReplayManifest` — marcati `BlueprintType` **uno per uno**, e gli esiti sono enum tipizzati
(`ERTReplayOpenResult`, `ERTReplaySeekResult`), non `bool` nudi. Questo ADR non sceglie una forma nuova:
**dichiara che quella è la forma**, e la estende allo Scenario Harness.

## Decisione

### 1. La porta è una facade `UObject`, e il modello non passa mai per Blueprint

```text
Blueprint / UMG  (terminale EDITOR)
        │  vede SOLO questa riga
        ▼
URTScenarioAuthoring : UObject          ← UFUNCTION, la porta
        │  possiede
        ▼
FRTScenarioDraft : USTRUCT()            ← C++ puro, la logica, testabile headless
        │  possiede
        ▼
FRTTestScenario                         ← il dato canonico, MAI esposto a BP
        │
        └── URTScenarioLoader / Runner / Session   ← invariati, restano C++
```

`FRTTestScenario` e le otto struct che lo compongono **restano `USTRUCT()` senza `BlueprintType`**. Blueprint
non le vede, non le costruisce e non le muta: chiede alla facade di fare qualcosa, e la facade risponde.

### 2. Ciò che attraversa il confine sono DTO, e sono di sola lettura

I tipi `BlueprintType` nuovi sono **viste**, non il modello: `FRTScenarioSummary` (identità, versione, tag,
conteggi) e `FRTScenarioUnitView` (id, eroe, squadra, cella, facing, bot). Portano `FRTCellId` — che è
`USTRUCT(BlueprintType)` **da prima di questo ADR** — e `ERTHexDirection`, già `UENUM(BlueprintType)`: il
vocabolario delle coordinate e delle direzioni resta quello del gioco, come `#1115` richiede esplicitamente.

Un DTO è una **fotografia**: modificarlo in Blueprint non modifica niente. È la proprietà che rende impossibile
all'actor visuale di diventare authority — non per disciplina di chi scrive il Blueprint, ma per costruzione.

### 3. Gli esiti sono enum tipizzati, non booleani

`ERTScenarioAuthoringResult` distingue `Success`, `NotFound`, `Invalid`, `WriteFailed`, `NoScenarioOpen`. Un
`bool` costringerebbe la UI a indovinare perché qualcosa non è andato, e `#1115` chiede un **errore leggibile che
nomini il problema**. Dove serve la frase, la facade la restituisce accanto al codice — l'una per la logica,
l'altra per l'occhio.

### 4. `UObject` istanziabile, non subsystem — e la ragione è dove vive la UI

`#1115` colloca il Composer **«nel Hex Map viewport»**, cioè nell'Editor, dove `URTHexEditorMode` già vive. Un
`UGameInstanceSubsystem` come quello del replay **non sarebbe raggiungibile**: fuori da PIE non esiste una
`GameInstance`. Un `UEditorSubsystem` lo sarebbe, ma vivrebbe nel modulo Editor — e il modulo Editor non è dove
si mette ciò che deve restare chiamabile anche headless dai test.

Una facade `UObject` creata da una factory `BlueprintCallable` non ha nessuno dei due problemi: funziona in
Editor, in PIE e nei test automation, il widget la tiene come variabile, e più scenari possono essere aperti
insieme senza che nulla sia globale. Se un giorno servirà un punto d'accesso unico in Editor, sarà un
`UEditorSubsystem` di poche righe **sopra questa stessa facade**, nel modulo Editor, dove un punto d'accesso
d'editor appartiene.

> ⚠️ **Questa è l'unica parte della decisione che dipende da una scelta del terminale Editor**, e va detto: se il
> Composer finisse in PIE invece che nel viewport, un subsystem tornerebbe praticabile. La facade resterebbe
> comunque corretta — è il wrapper a cambiare, non il contratto — e per questo la decisione è stata presa nella
> forma che non richiede di sapere la risposta.

### 5. La logica sta nel draft, non nella facade

`FRTScenarioDraft` è C++ puro e contiene tutto ciò che si può sbagliare: aprire, creare, validare, salvare, e —
con TD-CODE-02 — aggiungere, spostare e togliere unità. La facade `UObject` traduce e basta. È la separazione che
rende i test headless possibili senza costruire un `UObject`, ed è la stessa di `FRTReplayViewModel`.

## Conseguenze

✅ L'Editor ha un contratto chiamabile e non ha un modo di diventare authority: non può costruire un
`FRTTestScenario`, non può scrivere JSON, non può eseguire un turno se non attraverso il runner.

✅ Il modello resta libero di cambiare. `Tags` è entrato ieri; il prossimo campo entrerà senza rompere un
Blueprint, perché nessun Blueprint nomina i campi.

✅ `Validate` resta l'unico giudice della validità, e sta dove stava.

⚠️ **Costo, e non è nullo**: ogni operazione che l'Editor deve fare va **esposta una per una**. Non c'è la
scorciatoia di marcare la struct e lasciare che Blueprint si arrangi. È lavoro in più su C++ a ogni slice, ed è
esattamente il prezzo che compra l'invariante — chi lo trova troppo caro sta chiedendo di pagare l'altro.

> 🟡 **Un'eccezione, deliberata il 2026-08-28 con [#1117](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1117): `ERTTestOutcome` e' `UENUM(BlueprintType)`.**
>
> Il divieto qui sotto riguarda le **`USTRUCT` del formato**, e la ragione e' precisa: una struct esposta
> lascia comporre uno scenario incoerente membro per membro. Un enum di quattro valori non lo permette — non
> c'e' niente da comporre male — e `ERTTestOutcome` non e' il modello dello scenario: e' il **valore di
> ritorno** di una sua esecuzione, che il pannello di `RUN` deve poter ramificare per non mostrare un
> `Blocked` come se fosse un successo.
>
> ⚠️ **L'alternativa era un secondo enum DTO con gli stessi quattro valori, e sarebbe stata peggiore**: due
> elenchi paralleli divergono al primo esito aggiunto a uno solo dei due, ed e' il difetto che questo
> repository ha gia' pagato con le tabelle scritte a mano nel loader. Fra esporre un tipo che non puo' essere
> composto male e mantenere due copie della stessa lista, la prima costa meno.
>
> Il guardiano e' `RefactorTactics.Scenario.RunAndResetAreReachableFromBlueprint`, che verifica sia il
> `BlueprintType` sia che i valori restino **quattro**: comprimerli in due — «passato» e «non passato» —
> butterebbe via la distinzione fra difetto del gioco e difetto del test, che e' la ragione per cui quell'enum
> esiste nella forma che ha.

⚠️ **Cosa questo ADR NON autorizza**:

- ❌ marcare `BlueprintType` una qualunque delle nove `USTRUCT` del formato scenario;
- ❌ un `URTTacticalDesignerSubsystem` — la spec §2 lo vieta per nome;
- ❌ esporre `URTScenarioLoader`, `URTScenarioRunner` o `FRTScenarioSession` direttamente a Blueprint: restano
  C++, e la facade è la sola cosa che li chiama;
- ❌ un secondo modello di scenario nel modulo Editor, in qualunque forma, incluso «solo per la UI»;
- ❌ regole di gioco nella facade. La facade **traduce**; se si trova a decidere qualcosa, la decisione era del
  runtime e va spostata.

## Alternative scartate

| Alternativa | Perché no |
|---|---|
| **Marcare tutto `BlueprintType`** | Blueprint potrebbe costruire scenari incoerenti membro per membro, e il modello diventerebbe API congelata. [D-127](RT_PDR_00_Decision_Log.md) ha già registrato che un tipo marcato *si legge* come authority anche quando non lo è — lì il costo era emerso al primo salvataggio |
| **`UGameInstanceSubsystem`** come il replay | Fuori da PIE non esiste una `GameInstance`, e `#1115` mette il Composer nel viewport dell'Editor |
| **`UEditorSubsystem`** | Vivrebbe nel modulo Editor, dove non può stare ciò che i test automation devono poter chiamare headless |
| **Nessuna esposizione, l'Editor scrive JSON** | È il difetto che `#1114` esiste per impedire: una seconda autorità sul formato |
| **`UBlueprintFunctionLibrary` con funzioni statiche** | L'authoring ha **stato** — uno scenario aperto che si modifica a passi. Funzioni pure costringerebbero a far viaggiare il modello avanti e indietro per Blueprint, cioè a esporlo |
