# Spec — Tactical Designer: un solo loop fra mappa, skill e scenario

> `CURRENT` · **Stato**: owner del **concetto** e del suo confine, allineato al codice il **2026-08-17**
> **Autorità**: subordinata a [`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) e al
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md).
> **Nato da**: [referto di consolidamento del 2026-08-17](../../roadmap/plans/tactical-designer-consolidamento-2026-08-17.md),
> che ha verificato l'assenza di un owner: *Tactical Designer*, *Skill Workbench* e *Scenario Composer* non
> avevano **nessuna** occorrenza fuori da `docs/archive/`.

Questo documento risponde a **una** domanda: *quali strumenti d'authoring esistono, che cosa hanno il
diritto di decidere, e che cosa devono invece chiedere al gioco?*

> ⚠️ **Non è un tracker.** Lo stato di implementazione vive nelle **issue** — l'epic
> [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105) e le sue sub-issue — e nei
> checkpoint di [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md) (**M9.4**). Le sedute vivono
> in [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml). Se una riga di questo file dichiara uno
> stato, è un difetto.
>
> 🔵 **Questa riga mandava al `feature-registry.yaml` fino al 2026-08-29, e quel file non esiste dal
> 2026-08-21** ([D-181](../../decisions/RT_PDR_00_Decision_Log.md)). Era una delle **cinque** occorrenze
> superstiti in questo documento — le altre quattro sono al §8, al §9 e nelle due righe del §10, corrette
> nella stessa passata. ⚠️ **Ciò che si è perso va detto invece di essere rimpiazzato**: D-181 dichiara che
> *«non esiste più un punto unico in cui lo stato di una feature si scrive»*. Le issue e i checkpoint sono
> **due** fonti, non una vista: rispondono a *«che lavoro è aperto»* e *«quale checkpoint manca»*, non a
> *«a che punto è la capability X»*.

---

## 1. Cosa questo documento non possiede

| Tema | Owner |
|---|---|
| Grammatica dei segmenti, occupancy a dodici settori, cottura verso i bordi | [`spec-hex-geometry-authoring.md`](../systems/spec-hex-geometry-authoring.md) |
| Coordinate, `FRTCellId`, transizioni fra layer, formato dell'asset mappa | [`spec-mappa-multilivello.md`](../architecture/spec-mappa-multilivello.md) |
| Come si scrive ed esegue uno scenario | [`test-e-diagnosi.md`](../runbooks/test-e-diagnosi.md) |
| Come si identifica e si trova uno scenario | [`scenario-index-e-tag.md`](scenario-index-e-tag.md) |
| Chi verifica cosa — macchina, occhio umano, nessuno | [`scenario-map.md`](scenario-map.md) |
| Il registro delle verifiche interattive | [`test-manuali-pie.md`](../test-manuali-pie.md) |
| Serializzazione del TurnLog e replay canonico | [`spec-turnlog-serialize.md`](../architecture/spec-turnlog-serialize.md) · [ADR-0009](../../decisions/adr-0009-replay-logico-canonico.md) |
| Priorità, milestone e checkpoint | [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md) |

---

## 2. Il nome, e che cosa non implica

**Tactical Designer** è il nome del *workflow*, non di un modulo. Non esiste — e non deve nascere — un
`URTTacticalDesignerSubsystem`. Le classi si chiamano come si chiamano: `URTHexEditorMode`,
`URTHexPaintTool`, `URTHexGeometryTool`, `FRTTestScenario`.

> ⚠️ **Nessun rename è stato fatto e nessuno è previsto.** Un nome di prodotto che diventa un mass rename di
> API stabili produce churn in file ad alto conflitto e non riduce nessuna ambiguità: `URTHexEditorMode`
> dice già che cos'è. Se un giorno una classe nuova avrà bisogno del prefisso, lo prenderà da sola.

Il workflow copre quattro superfici che oggi hanno owner diversi:

```text
Tactical Designer
├── Map / Level authoring      URTHexEditorMode + i cinque tool
├── Character setup            FRTScenarioUnit
├── Skill Workbench            —
└── Scenario Composer          FRTTestScenario + URTScenarioAuthoring
```

> 🔵 **Questo blocco aveva una quarta colonna di stato fino al 2026-08-29, ed è stata rimossa — non
> aggiornata.** Diceva `✅ esiste` · `✅ esiste (dati, non UI)` · `⬜ non esiste` · `🟡 dati sì, authoring no`,
> e l'ultima era **falsa**: l'authoring visuale del Composer è consegnato da
> [#1114](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1114)–[#1117](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1117),
> con la porta su Blueprint decisa da [ADR-0010](../../decisions/adr-0010-esposizione-blueprint-scenario-harness.md).
>
> **Aggiornarla sarebbe stato il difetto, non la correzione.** Il banner in testa a questo documento dice
> *«Se una riga di questo file dichiara uno stato, è un difetto»*: tutte e quattro le celle lo erano, non
> solo quella falsa — le altre tre erano vere e sarebbero marcite allo stesso modo, con la stessa
> impossibilità di accorgersene. Lo stato vive nell'epic
> [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105) e nelle sue sub-issue.
>
> ⚠️ **Resta ciò che è di questo documento**: *quali* superfici esistono e *chi possiede* ciascuna. Un `—`
> nella colonna owner non è uno stato di avanzamento — è la constatazione che una superficie non ha ancora
> un proprietario, che è esattamente la domanda a cui questo file risponde.

---

## 3. L'invariante che tiene in piedi tutto il resto

Uno strumento d'authoring **non è mai un'autorità di gioco**. La catena è unidirezionale:

```text
Dati canonici + regole del gioco
        │
        ├─── resolver                (l'unica autorità sull'esito)
        ├─── Scenario Harness        (esegue il percorso reale)
        ├─── TurnLog / replay        (spiega cosa è successo)
        │
        └─── pure query / DTO
                    │
                    ▼
            visualizzazione d'editor
```

Mai:

```text
l'editor inventa una regola parallela
        │
        ▼
   somiglia al runtime
```

**Se l'editor e il runtime possono divergere, lo strumento ha perso il suo valore** — non è più una lente
sul gioco, è un secondo gioco che nessuno testa.

Il repository applica già questo vincolo in una forma più forte di una raccomandazione:

> **La logica pura vive nel modulo runtime, e l'editor la chiama.**

Non è teorico: lo snap del gesto d'autore vive in `Map/RTGeometryGrammar` e i suoi due test sono
`RefactorTactics.GeometryGrammar.Snap*`, benché il gesto sia interamente d'editor.

> 🔴 **La ragione che quasi tutti danno per questa regola è FALSA dal 2026-08-16, ed è già costata quattro
> volte.** La formulazione corrente in tre punti del repository è *«in `Source/RefactorTacticsEditor/` non
> esiste alcun test — `find … -iname "*test*"` è vuoto»*. **Misurato il 2026-08-17: restituisce due voci**,
> `Private/Tests/` e `Private/Tests/RTHexToolPropertiesTests.cpp`, con **due** test — arrivati con `#993`.
>
> Il file di test lo dice da sé, e vale la pena citarlo perché nomina il difetto per quello che è:
>
> > *«Tre issue di fila (#871, #921, #931) hanno dichiarato "RefactorTacticsEditor/ non ha test, quindi la
> > verifica è manuale" trattandolo come un dato di fatto. Non lo era: il `Build.cs` ha già `Core` — dove
> > vive `Misc/AutomationTest.h` — e il modulo runtime, e `WITH_DEV_AUTOMATION_TESTS` è definito sui target
> > Editor. I test non erano impossibili: non erano stati scritti.»*
>
> La prima stesura di **questo** documento ha ripetuto la stessa frase, ed è la quarta volta. Il correttivo
> non è ammorbidire la regola: **è cambiarne la giustificazione.** La logica non va nel runtime perché
> l'editor sia intestabile — non lo è più — ma perché *il modulo editor non è dove una regola di gioco
> appartiene*: l'editor **visualizza** una risposta che il gioco dà, e una regola che vive solo lì è una
> seconda risposta alla stessa domanda. È il §3 di questo documento, non un vincolo di tooling.
>
> Quello che i due test dell'editor coprono è **il proprio dominio, non il gioco**: che il pennello Fill
> derivi il costo dalla superficie, e che il readout non si aggiorni da solo. Sono esattamente i test che un
> modulo d'editor deve avere — e nessuno dei due decide un esito di partita.

---

## 4. Chi possiede cosa

| Dato | Owner del dato | L'editor può |
|---|---|---|
| Celle, superfici, costi, layer, transizioni | `URTHexMapAsset` | scrivere tramite `ARTHexMapActor` e le primitive di stroke |
| Geometria d'authoring (segmenti quantizzati) | `FRTGeometrySegment` | scrivere; è **arte** dopo la cottura |
| `FRTHexCover`, `bBlocksMovement` | la **cottura** (`RTGeometryBake`) e il pennello | ⚠️ due produttori — la provenienza li distingue |
| Occupancy, `ERTCellOccupancy` | `URTHexOccupancyLibrary` | **solo leggere** |
| Percorsi, raggiungibilità | `Pathfinding/` | **solo leggere** |
| LOS, copertura applicata | `Perception/`, resolver | **solo leggere** |
| Scenario | `FRTTestScenario` (JSON in `Scenarios/`) | scrivere il file **tramite `URTScenarioLoader::SaveToFile`**, mai interpretarlo per conto suo (§5.1) |
| Esito di un turno | resolver | **niente** |
| TurnLog | `ARTTurnManager` | **solo leggere** |

⚠️ **La riga con due produttori è l'unica delicata, ed è già risolta.**
[`D-131`](../../decisions/RT_PDR_00_Decision_Log.md) dà a `FRTHexCover` il campo `bGenerated`: il rebake rimuove
e riscrive **solo** le coperture generate e non tocca mai quelle dipinte a mano. Il campo **non entra
nell'hash di stato** — è metadato d'authoring, e due mappe che si giocano identiche non devono divergere.

---

## 5. Il formato scenario è già la lingua comune

Un authoring visuale degli scenari non ha bisogno di un formato proprio: `FRTTestScenario` esprime già
quasi tutto ciò che serve, e va **esteso**, mai affiancato.

Sono **diciassette** campi, misurati sulla struct e non elencati a memoria:

```text
FRTTestScenario
├── ScenarioId                  ID stabile e gerarchico
├── Version                     versione del FORMATO, non del contenuto
├── Tags[]                      parole per il filtro dell'indice, conservate GREZZE
├── Fixture                     l'allestimento da cui si parte
├── MapRadius                   dimensione dell'arena generata
├── Seed                        dichiarato e NON consumato (vedi sotto)
├── PreviewUnit                 presentazione: headless non fa niente
├── Cells[]                     FRTScenarioCell  — le celle che questo scenario cambia
├── Units[]                     FRTScenarioUnit  — eroe, squadra, cella, facing, HP, scudo, vista, loadout
├── Turns[]                     FRTScenarioTurn
│   ├── Intents[]               FRTScenarioIntent    — chi, dove si muove, quale abilità, su quale bersaglio
│   ├── Decisions[]             FRTScenarioDecision  — risposta scriptata a una finestra: FIRE | HOLD
│   └── Requires[]              capability necessarie → altrimenti ERTTestOutcome::Blocked
├── Expect[]                    FRTTestExpectation + ERTAssertionKind
├── Variants[]                  FRTScenarioVariant — stesso allestimento, celle diverse
├── bExpectSameAcrossVariants   il canary: tutte le varianti devono dare lo stesso esito
├── bFreeRun                    la partita decide quando finire, non il file
├── MaxTurns                    tetto di sicurezza del free-run
├── RepeatCount                 ripetizioni per misurare il determinismo
└── Requires[]                  capability di scenario (solo con free-run)
```

> ⚠️ **Questo elenco diceva «dodici» ed era misurato — nell'agosto 2026.** Quattro campi sono arrivati con
> `#957` (le chiavi del free-run, `version: 4`) e uno con [#1114](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1114)
> (`Tags`), e il conteggio non li aveva seguiti. Vale la pena dirlo invece di correggerlo in silenzio: un
> numero scritto in un documento è una misura **con una data**, e questa sezione ne dichiara una senza
> nominarla. Se conta di nuovo diverso, è la struct ad avere ragione.

Tre proprietà che rendono questo formato adatto a essere il bersaglio di un editor visuale:

1. **Gli ID sono di scenario, non di runtime.** `FRTScenarioDecision::Unit` è l'id di scenario, e la
   traduzione verso l'identità di runtime avviene dove esiste la mappa — non nel JSON. Un authoring layer
   non deve conoscere gli id interni.
2. **Un turno dichiara ciò che gli manca.** `Requires` più `Blocked` permettono di versionare uno scenario
   **prima** che i suoi sistemi esistano, senza una suite rossa cronica.
3. **Le varianti esistono, e il loro limite è scritto.** `FRTScenarioVariant` cambia **solo le celle**, e la
   struct dichiara il prezzo di allargarla: *«una variante che potesse cambiare eroi, squadre o condizione
   iniziale non sarebbe più lo stesso scenario con un ingresso diverso, e il confronto fra le sue tracce non
   direbbe più quale ingresso ha prodotto la differenza»*.

### 5.1 Il formato si legge e si scrive dallo stesso posto

`URTScenarioLoader` sapeva leggere uno scenario e non sapeva scriverlo. Finché è stato così, qualunque
authoring visuale avrebbe dovuto conoscere il JSON da sé — diventando una **seconda autorità sul formato**
accanto al loader, che è la forma che il §3 vieta applicata ai dati invece che alle regole.

`SaveToString` / `SaveToFile` chiudono il verso mancante, e stanno **nello stesso header** del loader
([#1114](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1114)): lettura e scrittura sono due
metà della stessa regola, e separarle in due classi renderebbe possibile aggiungere una chiave da una parte
sola. L'implementazione vive in `RTScenarioWriter.cpp` solo perché il `.cpp` del loader ha già 1769 righe.

Tre garanzie, tutte verificate da `RefactorTactics.Scenario.Writer*`:

1. **`Validate` prima di scrivere.** Uno scenario invalido non viene serializzato a metà, il file già presente
   non viene toccato, e l'errore **nomina il campo** che ha impedito la scrittura.
2. **Forma canonica.** I campi si scrivono in ordine esplicito e i default si omettono: due scritture dello
   stesso scenario producono lo stesso testo. Non si costruisce un `FJsonObject` per poi serializzarlo — le
   sue chiavi vivono in una `TMap`, il cui ordine di iterazione non è quello di inserimento, e un diff di PR
   diventerebbe rumore.
3. **Identità preservate.** `ScenarioId`, i tag dell'indice e gli Stable Unit ID sopravvivono al round-trip.
   L'identità è **dichiarata dal file**, non dedotta dalla cartella: salvare altrove non la cambia.

> 🔴 **`tags` era nel formato ma il loader non lo leggeva**, e per questo `FRTTestScenario` ha oggi un campo
> `Tags`. La chiave esisteva già — la legge `URTScenarioIndex::ReadHeader` per costruire i filtri — ma
> `LoadFromString` la ignorava: due letture dello stesso file che vedevano campi diversi. Finché nessuno
> scriveva scenari la differenza non si vedeva; il primo `load → save` avrebbe cancellato i tag da ogni file
> che li dichiara. **Non è un'estensione del formato**, è il modello che ha smesso di perdere per strada una
> chiave che il formato aveva già.
>
> I tag si conservano **grezzi**, non normalizzati. La forma canonica di un tag appartiene a
> `URTScenarioIndex::NormalizeTag`, e l'indice la applica per conto suo: se la applicasse anche il loader,
> salvare uno scenario riscriverebbe `"Gadget"` in `"gadget"` in tutti i file che lo dichiarano così — una
> modifica che nessuno ha chiesto, prodotta da uno strumento che doveva solo preservare.

### 5.2 Blueprint vede una porta, non il modello

Il formato si legge e si scrive dal C++. L'authoring visuale vive in Blueprint/UMG, e fra i due c'è **una sola
porta**: `URTScenarioAuthoring`, un `UObject` creato da factory che possiede un `FRTScenarioDraft` — il
ViewModel C++ puro dove sta la logica. Decisione registrata in
[ADR-0010](../../decisions/adr-0010-esposizione-blueprint-scenario-harness.md).

Le nove `USTRUCT` del formato **restano non-`BlueprintType`**: Blueprint non le vede, non le costruisce, non le
muta. Ciò che attraversa il confine sono DTO di sola lettura — `FRTScenarioSummary`, `FRTScenarioUnitView` —
che portano `FRTCellId` ed `ERTHexDirection`, cioè il vocabolario del gioco. Un DTO è una fotografia:
modificarlo non modifica niente, ed è la proprietà che rende impossibile all'actor visuale di diventare
authority **per costruzione**, non per disciplina di chi scrive il Blueprint.

> ⚠️ Il costo è che ogni operazione va esposta **una per una**, a ogni slice. È il prezzo che compra
> l'invariante del §3: chi lo trova troppo caro sta chiedendo di pagare l'altro — un editor che diverge dal
> gioco. Il guardiano è `RefactorTactics.Scenario.AuthoringContractIsReachableFromBlueprint`, che verifica
> per riflessione **entrambi i versi**: che il contratto sia raggiungibile da Blueprint, e che il modello non
> lo sia.

**Ciò che il formato non esprime**, e che va aggiunto solo quando ha un consumatore:

| Manca | Serve a | Innesco |
|---|---|---|
| status/condizioni iniziali | fixture con mitigazione o controllo attivo | il primo scenario che ne ha bisogno |
| stato d'ambiente (acqua, fuoco, ghiaccio) | fixture d'interazione ambientale | M9.2 |
| override di abilità in una variante | *baseline vs variante* | lo Skill Workbench |

> 🔴 **Il seed NON è in questa tabella, e la prima stesura ce l'aveva messo.** `FRTTestScenario::Seed`
> **esiste**, ed è documentato per esteso: *«Seed dichiarato ma non consumato: oggi il progetto non ha alcun
> RNG e il determinismo viene da coordinate intere e ordinamenti totali. Il campo esiste perché il giorno in
> cui un RNG entrerà nel resolver lo scenario debba già saperlo dichiarare — non perché faccia qualcosa
> adesso.»*
>
> ⚠️ E ha un **guardiano**: `RefactorTactics.Simulation.SeedIsDeclaredAndUnconsumed` verifica che due seed
> **diversi** diano lo stesso risultato — l'unico verso che morde su un progetto senza casualità. Chi
> introducesse un RNG non troverebbe un campo mancante: **contraddirebbe un test verde**. Il *se* è aperto
> (`RNG-1`/`RNG-2` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md)), il *come* no.

---

## 6. Scala di maturità del Tactical Designer

⚠️ **`TD 0.x` non è una release di gioco.** `TD 0.7` non ha **niente** a che vedere con
`RefactorTactics v0.7`: è il grado di maturità di uno *strumento*, che non entra nella build, non ha un gate
di release e non compete con la consegna. Le release di gioco stanno in
[`roadmap-post-v0.1.md`](../../roadmap/roadmap-post-v0.1.md) e non sono state toccate.

> 🔴 **Il precedente.** Un sorgente del 2026-08-13 proponeva una milestone *«Skill Balance Lab v0.3»*, e il
> consolidamento di quel giorno la dichiarò superata: `RT-FEAT-TOOL-BALANCE-GROUND` era **già v0.1
> `IMPLEMENTING`**. Una scala di maturità di uno strumento collocata nella roadmap di release si mette in
> concorrenza con il gioco, e perde.

| Stadio | Il designer può | Owner reale |
|---|---|---|
| **TD 0.1** | aprire una mappa canonica, disegnarla, caricare ed eseguire uno scenario esistente, vedere perché un ordine è invalido | `RT-FEAT-TOOL-MAP-EDITOR` · `RT-FEAT-TOOL-MAP-GEOMETRY` · `RT-FEAT-TEST-SCENARIO-HARNESS` · **M9.1** |
| **TD 0.2** | creare e modificare uno scenario **senza scrivere JSON**, e ottenere lo stesso TurnLog di una fixture scritta a mano | `RT-FEAT-TOOL-SCENARIO-COMPOSER` · **M9.4** |
| **TD 0.3** | configurare una skill *variante* senza toccare il dato di produzione, e provarla sulla mappa con le regole runtime | `RT-FEAT-TOOL-SKILL-WORKBENCH` · **M9.4** |
| **TD 0.4** | legare la variante a più scenari e leggere il diff baseline↔variante | TD 0.2 + TD 0.3 |
| **TD 0.5** | spiegare con dati runtime perché un bersaglio è valido, un percorso passa, una copertura si applica | `RT-FEAT-TOOL-MAP-EDITOR` — `#711`, `#695` |
| **TD 0.6** | trasformare una sessione registrata in scenario editabile e rieseguibile | `RT-FEAT-REPLAY-ARCHIVE` + conversione |
| **TD 0.7** | confrontare due varianti su una suite e ottenere metriche riconducibili a eventi del TurnLog | `RT-FEAT-TOOL-BALANCE-GROUND` · **E43** |
| **TD 0.8** | sapere quali scenari una modifica impatta, e classificarne l'esito | `RT-FEAT-UI-SCENARIO-BROWSER` · `RT-FEAT-TEST-GOLDEN` |
| **TD 0.9** | promuovere una variante a dato di produzione con un gate di validazione, e non per errore | dipende da TD 0.3 |
| **TD 1.0** | fare tutto il giro senza leggere il codice sorgente | — |

### Il DoD della v1.0, ridotto a ciò che è verificabile

1. Nessun rules engine parallelo nell'editor — **misurabile**: nessuna regola di gioco definita in
   `Source/RefactorTacticsEditor/`.
2. Preview ed esecuzione usano gli stessi dati e le stesse regole — **misurabile**: una fixture che
   confronta l'esito della preview con quello del resolver.
3. Una variante non può sovrascrivere il dato di produzione senza un atto esplicito — **misurabile** con un
   test.
4. Scenari con **ID stabile** — **misurabile**: `ScenarioId`, tag e Stable Unit ID sopravvivono al
   round-trip, verificato da `RefactorTactics.Scenario.Writer*` ([#1114](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1114)).
   La **copertura di feature tracciabile** che questa riga chiedeva accanto **non ha più una misura**:
   la produceva `feature_registry.py validate`, uscito con [D-181](../../decisions/RT_PDR_00_Decision_Log.md)
   il 2026-08-21. 🔵 *Fino al 2026-08-29 questa riga prescriveva quel comando come metodo di verifica — non
   un link rotto ma un criterio che fallisce all'esecuzione, e chi lo eseguiva non poteva sapere se il
   difetto fosse suo. Il criterio resta legittimo e oggi si verifica **a mano**, oppure non si verifica: è
   un costo dichiarato di D-181, non un buco di questo documento.*
5. Regressioni distinguibili da cambi di bilanciamento intenzionali — **misurabile**: i tipi di assertion
   sono già distinti, la classificazione deve derivare da quelli e non da string matching.
6. La documentazione descrive il workflow reale — **verificabile solo da una persona**, e per questo è una
   voce di seduta, non un gate automatico.

---

## 7. Distinguere le aspettative, o un nerf sembra un bug

Uno scenario dichiara aspettative di **quattro** nature diverse, e confonderle è il modo in cui una modifica
di bilanciamento diventa un falso allarme strutturale — o, peggio, un difetto vero viene archiviato come
«cambio di balance».

| Natura | Deve sempre passare | Chi la cambia |
|---|---|---|
| **Invariante forte** | sì | nessuno: se cade, è un difetto |
| **Aspettativa di design** | sì, finché il design non cambia | chi cambia il design, nello stesso commit |
| **Soglia di bilanciamento** | può cambiare intenzionalmente | richiede review, e la review è il punto |
| **Osservazione di telemetria** | no — è informazione | nessuno: non è un gate |

⚠️ **Oggi questa distinzione non esiste nei dati**: `ERTAssertionKind` dice *che cosa* si verifica, non *di
che natura* è l'aspettativa. Finché non esiste, la classificazione automatica di TD 0.8 non è costruibile —
e costruirla su string matching del nome dello scenario sarebbe fragile esattamente dove serve robustezza.
È un DoD di TD 0.4, non un lavoro separato.

---

## 8. Guardrail

Il Tactical Designer **non** deve:

- creare un resolver, un targeting system o un calcolo di LOS d'editor;
- mantenere copie indipendenti dei dati eroe/skill;
- scrivere valori derivati che il runtime dovrebbe calcolare;
- registrare coordinate world-space o eventi di widget dove esistono `FRTCellId` e ID stabili;
- mostrare una preview che *sembra* una cella reale quando non lo è;
- introdurre metriche che richiedono un modello statistico che il runtime non alimenta.

E in particolare, sul bilanciamento:

> **Nessun punteggio opaco diventa un gate.** Un `Power = 83.7` senza scomposizione non è una misura: è
> un'opinione con i decimali. Un indicatore euristico può esistere se mostra **quali componenti** lo
> compongono, e non decide niente da solo.

Il vincolo che viene prima di tutti gli altri è
[`D-102`](../../decisions/RT_PDR_00_Decision_Log.md): *un risultato bot-vs-bot non è evidenza di bilanciamento
finché non sappiamo che il bot sa usare la capability misurata*. Un bot che non usa una reazione produce un
numero in cui quella reazione sembra debole — **il numero è vero e la conclusione è falsa**, e niente nel
numero lo segnala. Per questo TD 0.7 segue il competence gate, e non lo precede.

---

## 9. Strati di test

| Strato | Che cosa verifica | Dove vive |
|---|---|---|
| **Puro / dati** | serializzazione, round-trip, ID stabili, precedenza degli override, migrazione | modulo runtime, Automation |
| **Fixture runtime** | targeting, LOS, copertura, percorso, spostamento, reazioni | modulo runtime, Automation |
| **Scenario** | esecuzione deterministica, baseline↔variante, invarianti forti | `Scenarios/`, Scenario Harness |
| **Editor** | binding modello↔toolkit, save/load, selezione, Undo/Redo | dove sostenibile sotto il layer widget |
| **PIE / manuale** | leggibilità, overlay, ergonomia, percezione della latenza | [`test-manuali-pie.md`](../test-manuali-pie.md), dentro una seduta |

Due regole che il repository ha già pagato per imparare:

- **Una verifica PIE che non appartiene a una seduta tende a non essere mai eseguita.** Le sedute vivono in
  [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml), che oggi si **legge direttamente**: la vista
  generata `editormap.shortlist.md` è uscita con [D-181](../../decisions/RT_PDR_00_Decision_Log.md).
  🔵 *Questa riga la dava per esistente fino al 2026-08-29.* ⚠️ **Ed è la stessa riga a essere diventata più
  vera**: D-181 dichiara fra i propri costi che `editor-sessions.yaml` è ora *«dato senza consumatore e senza
  vista — chi aggiunge una seduta scrive in un file che nessuno rende»*. Una verifica PIE che non appartiene
  a una seduta non viene eseguita; una seduta in un file che nessuno rende ha lo stesso problema un livello
  più su.
- **Un test importante deve essere dimostrato capace di diventare rosso.** Si rompe *una* mutazione per
  volta e deve cadere esattamente il test che protegge quella regola — se ne cadono zero, il test non
  verificava; se ne cadono cinque, non si sa quale.

---

## 10. Dove leggere lo stato

| Domanda | Fonte |
|---|---|
| A che punto è una capability | ⚠️ **non c'è più una vista che risponda**: si legge dalle issue e da [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md), che però rispondono a due domande più strette (vedi sotto) |
| Quale seduta d'editor fare, e in che ordine | [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml), letto direttamente |
| Chi verifica cosa fra macchina e persona | [`scenario-map.md`](scenario-map.md) |
| Quali domande di modello sono aperte | [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) |
| Che lavoro è aperto adesso | l'epic [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105) e le sue sub-issue |
| Perché questo documento esiste | [referto 2026-08-17](../../roadmap/plans/tactical-designer-consolidamento-2026-08-17.md) · [D-154](../../decisions/RT_PDR_00_Decision_Log.md) |

> 🔵 **Le prime due righe di questa tabella mandavano a due file inesistenti fino al 2026-08-29** —
> `feature-registry.yaml` ed `editormap.shortlist.md`, usciti entrambi con
> [D-181](../../decisions/RT_PDR_00_Decision_Log.md) il 2026-08-21. Erano **code span, non link**: nessun
> gate poteva vederle, perché `doc-links.ts` cammina i collegamenti e `doc-tables.ts` la larghezza delle
> righe, non il significato delle celle. Trovate leggendo, in code review sulla PR
> [#1620](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1620).
>
> ⚠️ **La prima riga resta la domanda a cui questo repository non sa più rispondere in un posto solo.** Le
> issue dicono *«che lavoro è aperto»*, `roadmap-checkpoint.md` dice *«quale checkpoint manca»*: nessuna
> delle due dice *«a che punto è la capability X»*, che è ciò che il registry derivava. D-181 lo elenca fra
> i propri costi — *«le viste che rispondevano a "a che punto è la consegna" non hanno più dati»* — e questa
> riga lo dichiara invece di indicare un sostituto che non c'è. Un puntatore a un file inesistente si nota;
> un puntatore a una fonte che **non risponde a quella domanda** no.
