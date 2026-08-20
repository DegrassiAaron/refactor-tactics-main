# Indice degli scenari e tag

> **Owner** del modello di identità e classificazione degli scenari.
> Come si scrive ed esegue uno scenario sta in [`test-e-diagnosi.md`](../runbooks/test-e-diagnosi.md); qui c'è **dove
> vive** e **come lo si trova**.
>
> Deciso il **2026-08-08**, issue `#209`. Origine: handoff
> `docs/archive/src/handoff/scenario-browser-bp-gamemode.md`, rivisto con `/sc:spec-panel`.

## 1. Le tre responsabilità

```
ScenarioId    identifica       stabile, dichiarato, univoco
Tag           fa trovare       aperti, molteplici, incrociabili
Resolver      esegue           non sa che i primi due esistano
```

Non si mescolano. In particolare: **nessun tag cambia il risultato di uno scenario**. Sono metadati di
navigazione, non entrano nello stato logico, non toccano `StateHash`.

## 2. Cosa è cambiato, e perché

Prima l'ID **era** il percorso: `Movement.Basic` → `Scenarios/Movement/Basic.json`, calcolato in un senso da
`PathForScenarioId` e nell'altro da `ListScenarioIds`. Elegante — un solo posto dove sta la verità — e
sufficiente finché gli scenari stavano in una cartella.

Non regge due bisogni insieme:

1. **Le lenti si incrociano.** Lo stesso scenario si apre per verificare una regola *oppure* per guardare
   un'animazione, e si cerca «reactions **e** gadget». Una gerarchia di cartelle esprime **un** asse.
2. **La combo di UE non filtra da testo** (verificato in Editor). Oltre la ventina di voci, scorrerle smette
   di essere un modo di trovare qualcosa.

Staccando l'ID dal percorso, le cartelle diventano storage e la classificazione passa ai tag.

### Il prezzo, dichiarato

Due file possono ora dichiarare lo stesso `scenarioId` — cosa che il filesystem prima rendeva impossibile.
È la sola classe di errore che questo modello **introduce**, ed è il motivo per cui l'indice esiste:
`ResolvePath` rifiuta un ID ambiguo invece di sceglierne uno. Coperto da
`ScenarioIndex.DuplicateIdIsRejected`.

Secondo prezzo: i tag sono stringhe libere e possono driftare (`reaction` vs `reactions`). Si auto-denuncia —
un tag scritto male compare come **voce nuova** nella tendina, visibile alla prima apertura. Sotto le
quaranta voci è sufficiente; oltre, si aggiunge un controllo contro un elenco.

## 3. Il modello

```
scenarioId : "Movement.Basic"                              identità, dichiarata dal file
tags       : ["movement", "core", "animation", "gadget"]     tipologia E lente, stesso asse
percorso   : Scenarios/<qualunque>/<qualunque>.json        storage, senza promesse
```

**Un asse solo** e non `PrimaryCategory` + `PurposeTags`: `reactions` è una tipologia, `animation` è una
lente, ma entrambi sono «parole per cui vorrei filtrare». Separarli costringerebbe a decidere per ogni
parola in quale casella vive, e la risposta onesta è spesso «entrambe».

I tag sono **opzionali**: uno scenario senza tag esiste, si esegue, e compare solo nell'elenco non filtrato.
Renderli obbligatori bloccherebbe la scrittura di uno scenario nuovo su una decisione di catalogazione, che
è l'ordine sbagliato.

### Il vocabolario non si dichiara

Non esiste un enum né una lista di tag validi. Il vocabolario è l'**unione di quelli realmente presenti nei
file** (`URTScenarioIndex::ListTags`). Aggiungere `animation` a uno scenario lo fa comparire nel filtro;
toglierlo dall'ultimo scenario che lo portava lo fa sparire. Stessa ragione per cui la tendina degli scenari
legge i file: un elenco scritto a mano invecchia, e una voce di filtro che non filtra niente è un invito a
cercare qualcosa che non c'è.

<!-- rename-exempt: misura datata: riscriverla la renderebbe falsa -->
Vocabolario al 2026-08-08 (**22 voci**, dopo i corpus `Visual.*` e `Spec.*`): `animation` · `bastion` ·
`combat` · `core` · `environment` · `expected-fail` · `gadget` · `friendly-fire` · `los` · `map` · `movement` ·
`objectives` · `pathfinding` · `perception` · `planning` · `reactions` · `relay` · `phase` · `shapes` ·
`showcase` · `spec` · `wraith`.

Il tag **`spec`** ha un significato operativo e non solo di navigazione: marca gli scenari che descrivono una
feature **non ancora costruita**, e che quindi escono `BLOCKED` per progetto. Filtrarci sopra risponde alla
domanda «cosa ho già dichiarato e non ho ancora fatto».

> Questa riga è una **fotografia**, e quando è stata rimisurata era indietro di **dieci** voci su venti.
> Tre (`objectives`, `relay`, `showcase`) erano entrate con `RT_Showcase_Relay_v01`; tre
> (`reactions`, `shapes`, `friendly-fire`) con gli scenari di combattimento aggiunti nel frattempo; quattro
> (`environment`, `map`, `phase`, `wraith`) col corpus visivo. Nessuna di queste è passata di qui, ed è il
> drift previsto due paragrafi più su — osservato, non ipotizzato.
>
> Il vocabolario **vero** resta `URTScenarioIndex::ListTags`. Si rilegge senza aprire l'editor:
>
> ```bash
> grep -ho '"tags": \[[^]]*\]' Scenarios/*.json Scenarios/*/*.json Scenarios/*/*/*.json \
>   | grep -o '"[a-z0-9-]*"' | grep -v '^"tags"$' | sort -u | tr -d '"'
> ```
>
> Che il conteggio sia raddoppiato senza che nessuno se ne accorgesse è la conferma della soglia dichiarata
> sopra: sotto le quaranta voci la tendina basta a sé stessa, ma **questa riga** va rimisurata col comando
> ogni volta che si tocca il corpus, non aggiornata a memoria.

## 4. L'indice

`URTScenarioIndex` — `Source/RefactorTactics/ScenarioHarness/RTScenarioIndex.h`

```
Scenarios/**/*.json   (esclusi i file _*)
      ↓ legge solo scenarioId + tags
Indice:  ScenarioId → { Path, Tags }
      ↓
ResolvePath(id)        percorso, o errore che dice DOVE si è cercato
ListIds(A, B)          intersezione dei due filtri, ordine totale
ListTags()             vocabolario
```

Nota implementativa: la scansione legge solo l'**intestazione** di ogni file, non unità/turni/assertion. Uno
scenario con un intent malformato deve restare **trovabile** — altrimenti un errore in fondo al file lo
farebbe sparire dalla tendina invece di farlo fallire con un motivo.

L'indice si ricostruisce a ogni chiamata, senza cache. Con qualche decina di file è irrilevante e il
comportamento resta prevedibile; se un giorno le voci diventassero centinaia, la cache va dietro a un
invalidamento esplicito, non introdotta di soppiatto.

### Convenzione `_`

File e chiavi che cominciano per `_` **non sono scenari**. È così che la tabella di redirect vive dentro
`Scenarios/` senza comparire nella tendina, e che quella tabella resta commentabile.

## 5. Redirect

`Scenarios/_redirects.json` — `{"vecchio.id": "nuovo.id"}`

Consultata **solo** quando un ID non risulta nell'indice: un ID vivo vince sempre su una voce rimasta
indietro, così riusare un nome liberato non dirotta al file sbagliato. La catena si segue fino a 8 salti,
poi si ferma — un ciclo `a → b → a` in un file scritto a mano è un errore plausibile, e un harness che si
pianta su un file di configurazione è peggio di uno che rifiuta l'ID.

Rinominare uno scenario è quindi: cambia `scenarioId`, aggiungi una riga qui. I riferimenti nella
documentazione restano validi come ID; il testo va aggiornato quando ci si passa accanto.

## 6. I filtri nell'Editor

Due property in `ARTGameMode`, dichiarate **prima** di `ScenarioToRun` perché il Details Panel segue
l'ordine di dichiarazione:

```
▼ RefactorTactics|Test
    Scenario Filter A   [movement ▼]
    Scenario Filter B   [core     ▼]
    Scenario To Run     [...      ▼]   ← solo chi passa entrambi
```

**Due filtri e non tre**: due assi coprono il caso che serve — una tipologia incrociata con un personaggio o
una lente — e il terzo diventerebbe rumore prima di diventare utile.

**Intersezione e non unione**: il caso è «reactions E gadget». Se il filtro restituisse l'unione sembrerebbe
funzionare (l'elenco cambia) mentre mostrerebbe *più* scenari invece di meno.

### I filtri non toccano la selezione

Restringere l'elenco non modifica mai `ScenarioToRun`. Uno scenario già scelto resta scelto e viene eseguito
anche mentre i filtri mostrano altro.

È una conseguenza di cosa *sono*: un filtro dice «cosa sto cercando adesso», non «a cosa questo scenario
appartiene». Una lente che cancella una configurazione salvata nel `.uasset` sarebbe un modo elaborato di
perdere lavoro. Ne segue anche che **non esiste un errore di "category mismatch"**: con una lente non c'è
niente da cui uno scenario possa essere incoerente.

I filtri sono property salvate, quindi si ricordano fra una sessione e l'altra — giusto per una lente che si
tiene per giorni.

## 7. Test

| Test | Cosa può romperlo |
|---|---|
| `ScenarioIndex.IdIsIndependentOfPath` | l'ID torna a dipendere dalla cartella |
| `ScenarioIndex.DuplicateIdIsRejected` | due file con lo stesso ID passano in silenzio |
| `ScenarioIndex.BrokenFileDoesNotHideTheOthers` | un JSON rotto svuota l'indice |
| `ScenarioIndex.TwoFiltersIntersect` | il secondo filtro non filtra, o filtra in unione |
| `ScenarioIndex.FilteredListIsTotallyOrdered` | l'ordine della tendina dipende dal filesystem |
| `ScenarioIndex.RedirectsFollowTheChain` | catena non seguita, o ciclo che non termina |
| `ScenarioIndex.UnknownIdFailsWithReason` | errore muto invece di un motivo |
| `ScenarioIndex.ShippedScenariosAreTagged` | uno scenario committato senza tag, o l'indice in disaccordo col runner |
| `Scenario.AutoRunOptionsAreFilteredByTags` | i filtri non restringono, **o azzerano la selezione** |
| `Scenario.ShippedScenariosAreValid` | un ID che non risolve al proprio file |

Verifica di mutazione eseguita il 2026-08-08: disattivare il secondo filtro fa cadere **esattamente**
`TwoFiltersIntersect` e `AutoRunOptionsAreFilteredByTags`; disattivare il rilevamento dei duplicati fa cadere
**esattamente** `DuplicateIdIsRejected`.

## 8. Rimandato

Non serve finché il corpus resta sotto le poche decine di scenari, e va riaperto solo con un bisogno
misurato, non per completezza:

- enum di categorie primarie — scartato: nessuna categoria fissa descriveva il corpus reale, e una categoria
  primaria sola non esprime lenti che si incrociano;
- `ExecutionMode` Visual/Fast/Headless — è una feature a sé (cosa fa «Fast»? tocca il `Tick`?), non un
  dettaglio di UI;
- pannello *Scenario Info* read-only;
- query per personaggio/fazione/milestone verso una wiki che non esiste;
- estensione di `result.json` con i metadati di classificazione;
- validazione di `MilestoneId`/`FactionId`: nel codice non esistono né milestone né fazioni, quindi non c'è
  niente contro cui validare. `gadget` e `conflux` restano tag, non ID.

## 9. Verifiche in Editor — fatte

`PIE-SCEN-FILTER` e `PIE-SCEN-KEEP` in [`test-manuali-pie.md`](../test-manuali-pie.md), **entrambe verdi il
2026-08-08**.

Due fatti che valgono oltre questa feature:

- **`GetOptions` rivaluta l'elenco quando cambia un'altra property dello stesso actor.** Era il solo rischio
  tecnico del disegno — se la tendina fosse rimasta ferma finché non si deselezionava l'actor, sarebbe
  servita una `PostEditChangeProperty` con la dipendenza `PropertyEditor`. Non serve: il Details Panel
  ridisegna da sé. Una property con `GetOptions` può quindi dipendere da un'altra property senza macchinario.
- **Il combo box mostra il valore corrente anche quando non è fra le opzioni.** È ciò che rende sicura la
  scelta «i filtri non toccano la selezione»: uno scenario escluso dalla vista resta visibile nel campo e
  non sembra perso.

Il macchinario per il refresh non era stato scritto in anticipo, e non è servito.
