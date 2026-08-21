# Spec — Tactical Designer: un solo loop fra mappa, skill e scenario

> `CURRENT` · **Stato**: owner del **concetto** e del suo confine, allineato al codice il **2026-08-17**
> **Autorità**: subordinata a [`piano-canonico-mvp.md`](../../product/piano-canonico-mvp.md) e al
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md).
> **Nato da**: [referto di consolidamento del 2026-08-17](../../roadmap/plans/tactical-designer-consolidamento-2026-08-17.md),
> che ha verificato l'assenza di un owner: *Tactical Designer*, *Skill Workbench* e *Scenario Composer* non
> avevano **nessuna** occorrenza fuori da `docs/archive/`.

Questo documento risponde a **una** domanda: *quali strumenti d'authoring esistono, che cosa hanno il
diritto di decidere, e che cosa devono invece chiedere al gioco?*

> ⚠️ **Non è un tracker.** Lo stato di implementazione vive nel
> `feature-registry.yaml` e nelle issue. Le sedute vivono in
> [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml). Se una riga di questo file dichiara uno stato,
> è un difetto.

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
├── Map / Level authoring      URTHexEditorMode + i cinque tool        ✅ esiste
├── Character setup            FRTScenarioUnit                          ✅ esiste (dati, non UI)
├── Skill Workbench            —                                        ⬜ non esiste
└── Scenario Composer          FRTTestScenario (dati)                   🟡 dati sì, authoring no
```

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
| Scenario | `FRTTestScenario` (JSON in `Scenarios/`) | scrivere il file, mai interpretarlo per conto suo |
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

Sono **dodici** campi, misurati sulla struct e non elencati a memoria:

```text
FRTTestScenario
├── ScenarioId                  ID stabile e gerarchico
├── Version                     versione del FORMATO, non del contenuto
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
└── bExpectSameAcrossVariants   il canary: tutte le varianti devono dare lo stesso esito
```

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
4. Scenari con ID stabile e copertura di feature tracciabile — **misurabile**: `feature_registry.py validate`.
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
  [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml) e la vista è
  `editormap.shortlist.md`, **generata**.
- **Un test importante deve essere dimostrato capace di diventare rosso.** Si rompe *una* mutazione per
  volta e deve cadere esattamente il test che protegge quella regola — se ne cadono zero, il test non
  verificava; se ne cadono cinque, non si sa quale.

---

## 10. Dove leggere lo stato

| Domanda | Fonte |
|---|---|
| A che punto è una capability | `feature-registry.yaml` e le viste generate |
| Quale seduta d'editor fare, e in che ordine | `editormap.shortlist.md` |
| Chi verifica cosa fra macchina e persona | [`scenario-map.md`](scenario-map.md) |
| Quali domande di modello sono aperte | [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) |
| Che lavoro è aperto adesso | l'epic [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105) e le sue sub-issue |
| Perché questo documento esiste | [referto 2026-08-17](../../roadmap/plans/tactical-designer-consolidamento-2026-08-17.md) · [D-154](../../decisions/RT_PDR_00_Decision_Log.md) |
