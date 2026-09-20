# Il conduttore di seduta PIE — una apertura, N voci, un tasto per verdetto

> **Statuto**: design accettato in sessione il 2026-09-20, non ancora implementato. Issue [#3208].
> Finché non atterra, una seduta PIE resta quella descritta da `test-manuali-pie.md` e dai runbook.
>
> **Stato misurato**: 2026-09-20, `main` = `15c50b96`, branch `issue/3208-conduttore-seduta-pie`.
> Ogni numero qui sotto porta il comando che lo produce; chi lo rilegge lo **ricalcola**.

---

## 1. Il problema, e perché non è «scrivere degli scenari»

Una seduta PIE costa oggi, per **ogni** voce: aprire la console, digitare `rt.Test.Scenario <Id>`,
premere Play, guardare, Stop, ridigitare. Alla fine il verdetto lo trascrive una persona nella cella
del registro, dove ha due copie derivate che nessun gate riallinea.

La richiesta — *«eseguire le sedute automatizzando il più possibile, lasciando all'utente solo il sì o
il no, senza dover spostare unità o selezionare skill»* — sembra chiedere degli scenari. Non è lì che
sta il costo, e le misure lo dicono.

### 1.1 Spostare unità e scegliere abilità è già automatico

`FRTScenarioIntent` distingue `Move` · `Dash` · `Ability` + `Target`/`TargetCell` · `CoverEdge` ·
`Reaction` · `Condition` · `Facing`, e `turns[].decisions[]` porta `HOLD`/`FIRE`. Il corpus è scritto
per essere guardato: `Scenarios/Visual/Combat/AreaGuardFromImpactCenter.json` apre con
`"_nota": "DA GUARDARE, non solo da eseguire"` e porta un campo `_nota_cosa_guardare`.

```
find Scenarios -name '*.json' | wc -l        → 138   (2026-09-20)
```

### 1.2 Il playback è già osservabile, e la fine di uno scenario è già un aggancio

`FRTScenarioCoordinator::Tick` avanza la sessione **un passo per frame**
(`RTScenarioCoordinator.cpp:63-72`), scelta presa perché risolvendo tutto in `BeginPlay` lo scenario
«finiva prima del primo fotogramma». A `IsFinished()` il coordinator scrive il report, logga
`FINITO <id> -> <esito>` e **si ferma col mondo fermo** (`RTScenarioCoordinator.cpp:78-96`). È
l'istante esatto in cui va posta la domanda: non ne serve uno nuovo.

### 1.3 N scenari di fila nello stesso mondo sono già praticati

```
RTScenarioRunner.cpp:356-368 → for (I < Scenario.RepeatCount) { RunSingle(World, Once, bTearDownAfter=true); }
```

La playlist riusa questo percorso. ⚠️ La trappola è nota e documentata in `RTScenarioSession.h`: un
`BindRaw` lascia un puntatore **grezzo** in un delegate posseduto dall'attore, che sopravvive alla
sessione; chi eseguisse un secondo scenario troverebbe `IsBound()` vero e prenderebbe il ramo
`test-override` ignorando in silenzio le `decisions` del secondo. Sbindare fa parte del teardown, e non
è un dettaglio implementativo: è la ragione per cui il conduttore **non** riscrive il ciclo di vita.

### 1.4 L'uscita macchina-leggibile esiste; il legame voce → scenario no

```
RTTestReportWriter.cpp:145-153 → Saved/RTTests/<ScenarioId>/<runId>/result.json
```

Il dato che manca è quale scenario allestisca quale voce:

```
grep -c '^| \*\*PIE-' docs/technical/test-manuali-pie.md                     → 245
righe-voce la cui Precondizione cita un id di scenario                       → 49
```

Per le altre il legame è prosa, un runbook, o non esiste. **Questo** è il dato nuovo che il conduttore
richiede; il resto è cucitura fra pezzi che ci sono già.

### 1.5 L'unica cosa davvero assente è il pilotaggio dell'input UI

```
grep -rln "AutomationDriver|ProcessMouse|InjectInput|SimulateInput" Source/   → nessun file
```

∴ le voci che chiedono un click, un `TAB` o un hover **restano fuori** da questo lavoro. Non si
travestono da coperte: il conduttore le dichiara `NotJudgeable` col motivo. Il precedente su cosa farne
è [`D-402`], che ha portato `G9` a ✅ **riducendo un criterio** che nessuno poteva produrre, non
costruendo un produttore.

---

## 2. Architettura e confini

Il vincolo che dà la forma è già scritto: *«`ARTGameMode` decide **se** questa sessione è una run di
scenario o una partita… ciò che non gli appartiene è il **come**»* (`RTScenarioCoordinator.h`). Il
conduttore aggiunge una terza domanda — **quale, e quando il prossimo** — e non ne ruba nessuna delle
altre due.

| Pezzo | Possiede | Non sa |
|---|---|---|
| `URTPieSessionSubsystem` **(nuovo)**, `UGameInstanceSubsystem` | playlist, passo corrente, verdetti raccolti, scrittura del file di seduta | come si allestisce uno scenario, cosa c'è a schermo |
| `ARTGameMode` (ruolo invariato) | *se* questa è una run di scenario; avvia il coordinator | in che ordine vanno le voci |
| `FRTScenarioCoordinator` (**due** aggiunte) | il ciclo di vita di **uno** scenario | che esista una playlist |
| `URTPieVerdictOverlay` **(nuovo)**, `UUserWidget` in C++ | mostrare la domanda, catturare i tasti | tutto il resto |

**Le due aggiunte al coordinator**, entrambe minime:

* `OnScenarioFinished` — delegate multicast con `const FRTTestResult&`, sparato dove oggi `Tick` logga
  `FINITO`;
* `TearDown()` pubblico, che inoltra a `FRTScenarioSession::TearDown` — quello che
  `RunSingle(..., bTearDownAfter=true)` già usa fra due varianti, **sbindatura del decisore inclusa**.

Nessuna logica di conduzione entra nel coordinator.

### 2.1 Perché `GameInstance` e non `World`

Alcune voci pretendono un'altra mappa: `PIE-V01-SCREENHUD` va giudicata con la partita avviata da
`L_Frontend`. Un conduttore legato al world morirebbe al primo travel. Con la GameInstance la playlist
sopravvive, e quelle voci potranno entrare senza rifare l'architettura.

⛔ **In v1 restano comunque fuori**: il conduttore sa avviare scenari, non sa navigare il frontend. Una
voce che chiede un'altra mappa è dichiarata `NotJudgeable — richiede allestimento manuale` **all'inizio
della seduta**, non alla fine: chi apre deve sapere prima cosa la playlist non copre.

### 2.2 Nessuna decisione nel widget

Se il widget non c'è — seduta headless, o overlay non ancora scritto — il subsystem funziona lo stesso e
i verdetti arrivano da `rt.Pie.Verdict`. Il widget è la strada comoda; la console è la strada che rende
la cosa **verificabile senza aprire l'Editor**, che in questo progetto non è un lusso.

---

## 3. Il dato, la playlist, il flusso

### 3.1 Il campo nel JSON è un elenco nudo di id

```json
"verifies": ["PIE-V01-DASHINK", "PIE-VIS-SIGHTWALL"]
```

Niente testo della domanda. La regola che `editor-sessions.yaml` esiste per far rispettare vale identica
qui: *«si citano gli ID delle voci PIE, mai il loro esito atteso; l'esito atteso vive in
`test-manuali-pie.md`, che ne resta l'unico owner»*. Una domanda scritta nel JSON sarebbe una terza copia
derivata di un testo che già ne ha due divergenti.

**Cosa vede chi giudica**: l'**id** della voce e, se lo scenario ce l'ha, il suo `_nota_cosa_guardare` —
che non è l'esito atteso ma dove posare l'occhio, e appartiene all'allestimento. Il registro resta aperto
accanto per il criterio pieno.

⌫ **Scartata**: un generatore che estrae la colonna «Cosa verificare» dal registro in un file dati
consumabile a runtime. Costerebbe un generatore e un gate di allineamento, per un guadagno che si ottiene
tenendo aperto un file.

### 3.2 Comporre la playlist

```
rt.Pie.Session Visual.Perception.*              # per prefisso di scenario
rt.Pie.Session PIE-VIS-SIGHTWALL,PIE-V01-LOG    # per id di voce
rt.Pie.Session                                  # stampa cosa comporrebbe, NON avvia
```

La forma senza argomenti è deliberata: la prima cosa che il conduttore fa è **dichiarare la coda** —
quali voci, in che ordine, quali fuori portata e perché. Una voce che nessuno scenario dichiara non
compare per errore: compare fra le escluse.

**Come un selettore diventa una coda.** Un prefisso seleziona **scenari**, e ogni scenario porta in coda
*tutte* le voci del suo `verifies` — un allestimento che ne copre sette si apre una volta e produce sette
passi, che è il guadagno principale. Un elenco di id seleziona **voci**, e il conduttore risale allo
scenario che ciascuna dichiara: se due scenari dichiarano la stessa voce la coda si ferma prima di
partire e chiede quale, invece di sceglierne uno in silenzio. L'ordine è quello dello scenario nel primo
caso, quello scritto dall'utente nel secondo.

### 3.3 Gli stati

```
Idle --rt.Pie.Session--> Playing --OnScenarioFinished--> AwaitingVerdict
         ^                                                     |
         |                                                  verdetto
         |                                                     v
       Done <--- coda vuota --- Advancing <--------------------+
```

Il mondo in `AwaitingVerdict` è già fermo: è lo stato in cui il coordinator lascia la scena oggi.

### 3.4 I quattro casi che non sono «l'utente guarda e decide»

| Caso | Esito del passo |
|---|---|
| `ERTScenarioStart::NotLoadable` | `NotJudgeable`, motivo «scenario non caricabile», si prosegue |
| Sessione avviata ma in errore | `Blocked` col messaggio d'errore — **non si chiede** un giudizio su una scena mai partita |
| `expect` dello scenario fallite | si chiede il verdetto **lo stesso** |
| `rt.Pie.Session.Abort` | i verdetti già dati si scrivono, il resto resta `NotRun` |

Il terzo è il più importante. Un `expect` rosso con «a schermo si capisce» è informazione, non
contraddizione: è esattamente la coppia prodotta dalla seduta `U54`, dove il feed passava a verde mentre
il dock falliva. L'esito macchina e il verdetto umano sono **due campi**, e nessuno dei due si deduce
dall'altro.

---

## 4. Cosa esce, e cosa lo verifica

### 4.1 Un file per seduta

`Saved/RTPieSessions/<sessionId>/session.json`:

```json
{ "sessionId": "20260920-143012", "commit": "15c50b96", "buildVersion": "...",
  "steps": [
    { "pieItem": "PIE-VIS-SIGHTWALL", "criterion": null,
      "scenarioId": "Visual.Map.SightWallIsWalkable",
      "runId": "20260920-143015", "reportDir": "Saved/RTTests/...",
      "machineOutcome": "PASS 12/12", "verdict": "FAIL",
      "reason": null, "at": "2026-09-20T14:30:41Z" } ] }
```

Copiare `machineOutcome` qui non crea una copia che invecchia: una run è immutabile e datata — è un
verbale, non uno stato.

**`verdict` ha cinque valori, e tre non li dà una persona**: `PASS` e `FAIL` vengono dai tasti;
`NotJudgeable` viene dal tasto **oppure** dal conduttore quando la voce non era allestibile;
`Blocked` e `NotRun` li scrive solo il conduttore, e `reason` è obbligatorio per tutti e tre i casi
non umani. ⛔ Nessuno dei cinque significa «verde perché il gate era verde»: l'assenza di un verdetto
umano si scrive, non si riempie.

`criterion` è `null` per una voce giudicata intera, e porta il numero quando la voce dichiara criteri
numerati. Oggi le righe-voce che li numerano nello stile `**(3) Dock**` sono `4` su `245`
(`grep -cE '^\| \*\*PIE-[^|]*\|.*\([1-9]\) '`): le altre partono intere e si spezzano quando serve, senza
lavoro a tappeto prima che il conduttore serva a qualcosa.

### 4.2 Il commit va nel file, e se manca il conduttore lo dice

In questo progetto una misura vale se avviene su un commit dichiarato. Il subsystem legge `.git/HEAD`
all'avvio della seduta (sotto `WITH_EDITOR`). Se non ci riesce scrive `"commit": null` **e lo annuncia
nel riepilogo finale**: *«commit non dichiarato — questa seduta non è attribuibile a un albero»*. Un
campo vuoto silenzioso sarebbe peggio di nessun campo.

⚠️ Il conduttore **non** verifica che l'albero sia pulito, e non lo lascia intendere: dichiara il `HEAD`
che legge, non lo stato del working tree.

### 4.3 L'overlay è C++, senza `.uasset`

`URTPieVerdictOverlay : UUserWidget` costruisce il proprio contenuto in codice, entra in viewport
all'ingresso in `AwaitingVerdict`, mette l'input in modalità UI (il mondo è già fermo), e cattura:

| Tasto | Verdetto |
|---|---|
| `1` | `PASS` |
| `2` | `FAIL` |
| `3` | `NotJudgeable` |
| `Esc` | interrompe la seduta |

Nessun asset toccato, nessun binding da cablare a mano nell'Editor. Se un giorno lo si vuole bello, un
`WBP_` che deriva da questa classe lo eredita senza toccare la logica.

### 4.4 I tre gate, tutti rossi senza aprire l'Editor

1. **La conduzione** — playlist di tre passi, coordinator finto, verdetti iniettati: ordine, file
   prodotto, e i quattro casi di §3.4.
2. **Il catalogo** — ogni id in `verifies` dei JSON esiste come riga in `test-manuali-pie.md`, e nessuna
   voce è dichiarata da due scenari senza che sia voluto. È il mestiere che `RefactorTactics.Icon.*` fa
   già per il catalogo icone.
3. **La non-deduzione** — un test **rosso** se il codice facesse scendere `verdict` da `machineOutcome`:
   playlist con `expect` fallite e verdetto umano `PASS`, e il file deve portare entrambi. Senza questo,
   la scorciatoia più tentante del sottosistema non avrebbe nessun oracolo.

### 4.5 Cosa resta a schermo, dichiarato adesso

Che l'overlay sia leggibile sopra la scena e che i tasti si premano senza pensarci **non** ha un gate
headless. Il conduttore automatizza il giudizio degli altri, non il proprio: la sua prima seduta reale è
anche la sua verifica.

---

## 5. Perimetro

**Dentro**: `verifies` nel formato scenario e il suo gate di catalogo · il subsystem conduttore · i tre
comandi console · l'overlay C++ · il file di seduta.

**Fuori**, e sono i tre sotto-progetti successivi:

1. **la propagazione** dei verdetti dal file di seduta al registro, alle sue copie derivate e alle issue;
2. **la copertura** — gli scenari che oggi non esistono per le voci non allestite;
3. **il pilotaggio dell'input UI** — o, dove nessuno può produrre l'osservazione, la riduzione di
   criterio sul modello di [`D-402`].

---

## 6. Decisioni prese, e quelle scartate

| Decisione | Alternativa scartata | Perché |
|---|---|---|
| Conduttore **sopra** il coordinator | coda **dentro** il coordinator | mescolerebbe ciclo di vita e conduzione, e la verifica richiederebbe scenari veri invece di un coordinator finto |
| Conduttore sopra il coordinator | seduta come **scenario composito** | `FRTScenarioSession` è una macchina a stati per **uno** scenario, e il verdetto umano non ha posto in un formato che descrive un allestimento |
| Id nudi nel JSON | domanda scritta nel JSON | terza copia derivata dell'esito atteso |
| Overlay in C++ | `WBP_` nel Content | authoring nel clone principale, binding da cablare a mano, e una logica che senza Editor non si può provare |
| `GameInstanceSubsystem` | `WorldSubsystem` | morirebbe al travel, e le voci del frontend ne hanno bisogno |

[#3208]: https://github.com/DegrassiAaron/refactor-tactics-main/issues/3208
[`D-402`]: ../../decisions/RT_PDR_00_Decision_Log.md
