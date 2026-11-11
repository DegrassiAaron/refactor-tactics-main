# CP 15.4 — Golden replay degli 8 turni · spec panel del 2026-09-03

> Referto di `/sc:spec-panel #170`, modalità **critique**, focus `requirements` + `testing`.
> Panel: Wiegers (lead requisiti) · Adzic · Crispin · Nygard · Fowler · Cockburn.
> Misure prese sul branch `test/170-golden-replay-otto-turni` @ `55717433`, con i comandi
> riportati accanto a ogni numero. **Nessun numero di questo referto è trascritto dalla issue.**

## Perché il panel è stato convocato

Il corpo di #170 è stato riscritto il 2026-08-17 e dichiara, in testa, di non trascrivere più
lo stato di nulla: lo riferisce a tre issue e a un comando. Quella disciplina ha retto —
**ma le tre issue riferite si sono chiuse tutte nelle ultime 48 ore**, e un corpo che rimanda
a fonti chiuse non è stantio, è *muto*: chi legge trova tre `CLOSED` e nessuna indicazione di
cosa resti.

| dipendenza | ruolo nel corpo | stato al 2026-09-03 |
|---|---|---|
| #1060 | vocabolario di assertion del T6 + capability `InterceptRevalidation` | CLOSED `2026-09-02T16:54Z` |
| #75 | `Objective`, cioè il T8 | CLOSED `2026-09-02T08:55Z` |
| #833 | chi apre la porta D1, metà mancante del T5 | CLOSED `2026-09-01T19:46Z` |
| #1061 | roster legacy nella spec owner | CLOSED `2026-08-25T09:51Z` |

Il panel non riscrive il corpo: stabilisce **cosa di quel corpo è già fatto, cosa è ancora
vero, e cosa è diventato falso**.

---

## Lo stato misurato, non ricordato

```
$ python -c "import json;d=json.load(open('Scenarios/RT_Showcase_Relay_v01.json',encoding='utf-8'));
             [print(i,len(t.get('intents',[])),t.get('requires')) for i,t in enumerate(d['turns'],1)]"
1 4 None                    5 3 ['ReactionPlanning']
2 1 ['PredictiveAction']    6 2 ['InterceptRevalidation']
3 2 None                    7 4 None
4 3 ['DecisionBoundary']    8 0 ['PredictiveAction', 'Objective']
```

`expect` di radice: `TurnsCompleted = 7`. Non cinque — il valore che l'ultimo commento della
issue cita è a sua volta superato.

| blocco del corpo | voce | stato reale |
|---|---|---|
| 1 | T6 contenuto + capability | ✅ **già fatto, non da questa issue** |
| 1 | T7 «fatto arrivare» | ✅ arriva — è nei 7 turni giocati |
| 1 | T7 asserito | ⛔ da fare, **e non come scritto** (§F4) |
| 2 | T8 contenuto + `Objective` | ⛔ da fare, **e manca un terzo pezzo che nessuno nomina** (§F2) |
| 3 | T3 asserito | ⛔ da fare, **la scelta è già stata presa altrove** (§F5) |
| 3 | nota `_nota_cosa_non_e_asserribile` | ⛔ da fare — motiva ancora il T3 con #625, chiusa |
| 3 | T5 metà mancante | ✅ già fatto: il T5 porta `Action.Interact` di Riktor |
| 3 | ogni turno lascia traccia falsificabile | ⛔ da fare |
| 4 | granularità del golden | ✅ **decisa dal repository, non da questa issue** (§F6) |
| 4 | golden multi-turno dello showcase | ⛔ da fare |
| 4 | rinomina `RunsTurnOne` | ⛔ da fare |
| 4 | ordine-indipendenza per turno | ⛔ da fare |
| — | difetto della spec owner (roster) | ✅ risolto da #1061 |

---

## F1 · WIEGERS — 🔴 CRITICO · Il blocco 1 è chiuso, e attribuirlo a questa issue falserebbe il consuntivo

> *«Un requisito che descrive lavoro già consegnato non è un requisito: è un doppione di
> contabilità. Chi lo spunta si accredita il lavoro di qualcun altro, e chi legge il consuntivo
> non sa più chi ha fatto cosa.»*

Misurato — le tre condizioni che la vecchia riga di `InterceptRevalidation` prescriveva sono
tutte soddisfatte, e nel repository, non nella issue:

- `RTScenarioSession.cpp:292` — `TEXT("InterceptRevalidation")` è fra le **disponibili**;
- `RTScenarioLoader.cpp:1124` — `OriginalTargetEquals` ed `EffectiveTargetEquals` sono tipi di
  assertion con parsing proprio (consegnati da **#1196**, non da questa issue);
- il T6 ha **2** intent, non `[]`: il contenuto è atterrato nello stesso commit della capability,
  che era la condizione posta.

**Raccomandazione**: le due voci del blocco 1 relative al T6 e alla capability si consuntivano
`✅ già soddisfatto da #1060/#1196`, con attribuzione esplicita. Non si spuntano come lavoro di
#170. Resta a #170 **solo** l'assertion del T7.

---

## F2 · ADZIC — 🔴 CRITICO · Il T8 non è bloccato da #75, ed è bloccato da qualcosa che nessuno ha nominato

> *«"Serve `Objective`" è un requisito scritto sul nome di una feature. Quando la feature
> atterra e il turno resta impossibile, il requisito non ha detto niente di utile — perché non
> nominava la condizione, nominava il fornitore.»*

Il criterio con cui una capability si sposta fra le disponibili è scritto in
`RTScenarioSession.cpp` e non è «la issue è chiusa»: è **il campo ha un produttore che non è
l'harness**. Per `Objective` quel produttore esiste (#75: `URTTurnRules::ResolveObjectiveControl`,
valutato nel Cleanup prima di `EvaluateMatchEnd`, con voce nel TurnLog e nell'HUD).

Ma il T8 chiede a **Phase** di segnare sul Relay, e:

```
$ grep -rn "bIsObjective = true" Source/
Source/RefactorTactics/Tests/RTObjectiveTests.cpp:52
Source/RefactorTactics/Tests/RTObjectiveTests.cpp:169
Source/RefactorTactics/Tests/RTObjectiveTests.cpp:394
Source/RefactorTacticsEditor/Private/Content/RTSetObjectiveCellCommandlet.cpp:158
```

**Nessun costruttore di arena posa un obiettivo** — `MakeShowcaseRelayBasinArena` compreso. Il
flag `FRTHexCellData::bIsObjective` esiste (formato mappa **v11**, `#75`), `HasObjectiveCell()` e
`FirstObjectiveCell()` esistono, e su una mappa senza obiettivi il Cleanup **tace di proposito**
(`Objectives.SilentWithoutObjectiveCell`).

E la spec owner dice il contrario della fixture:

> `docs/product/showcase-v0.1.md` §2.2 — | **Relay** | `(0,0,0)` | `Objective.Relay`, contendibile |

∴ **Il blocco 2 ha tre voci, non una**, e la terza non è mai stata scritta da nessuna parte:

1. `MakeShowcaseRelayBasinArena` marca `(0,0,0)` come obiettivo — è la riga che riconcilia
   fixture e §2.2, ed è il vero prerequisito del T8;
2. `Objective` scende fra le disponibili **nello stesso commit** del contenuto del T8 (la regola
   che il file impone a sé stesso, e che #512 fase B ha speso un giro a difendere);
3. il T8 riceve i propri intent.

⚠️ E c'è un ordine obbligato fra la 1 e la 2: scoprire la capability su una mappa senza obiettivo
produrrebbe un T8 che gira, non segna, e **passa** — il verde bugiardo che questo file
documenta due volte.

---

## F3 · CRISPIN — 🟠 MAGGIORE · «Asserito» è definito nel corpo e non è richiesto dal DoD

> *«Il corpo definisce tre stati e dice che il terzo si misura rompendo l'intent finché un test
> non diventa rosso. Poi elenca quindici voci e non ne chiede **una** che quella rottura la
> esegua. La definizione sta nella prosa, non nel criterio di accettazione.»*

La tabella del corpo:

| | significato | come si misura |
|---|---|---|
| **asserito** | un `expect` cade se il turno non fa ciò che dice | **rompi l'intent: un test deve diventare rosso** |

Nessuna voce del DoD chiede quella verifica. È esattamente la forma di difetto che il repository
già combatte altrove: `GoldenCorpusCoversItsCategories` porta scritto *«verificato per
mutazione»* accanto alla propria soglia, e la nota spiega che senza alzare il numero il test
sarebbe *«verde prima e dopo»*.

**Raccomandazione**: una voce di DoD esplicita — *per ogni assertion aggiunta, la mutazione che
la fa cadere è eseguita e il nome del test che diventa rosso è registrato nel consuntivo*.
Senza, si consegnano assertion la cui capacità di fallire non è misurata.

---

## F4 · NYGARD — 🟠 MAGGIORE · Due delle quattro assertion del T7 sono note false, e la fonte è nel dato

> *«Il corpo prescrive quattro assertion per il T7. Il file su cui andrebbero scritte dichiara,
> nel proprio campo `_turno`, che due di esse non possono essere vere. Chi le scrive ottiene un
> rosso; chi le inverte per farlo passare pinna un difetto come se fosse una regola. Nessuna
> delle due è il lavoro.»*

Il corpo chiede: *«fuoco spento, propagazione che non colpisce due volte, scivolata deterministica
su `Ice`, `Ram` rifiutato da `Rough`»*.

Il `_turno` del T7 nel JSON — scritto **dopo** il corpo — dice:

- ⛔ **la combo non è ottenibile in un turno solo** (#1111, CLOSED `2026-08-25`):
  `Gadget.Sprinkler` è `Action.CreateWater` → **Cleanup**, `LinearDischarge` è `Attack` →
  **Blast**, e Blast precede Cleanup. *«Nello stesso turno la scarica risolve prima che l'acqua
  esista»*. ∴ **«fuoco spento» è falso nel T7 come è scritto oggi**;
- ⛔ **dal 2026-08-27 il `Ram` di Riktor parte da un'altra cella**: dopo aver aperto il gate al T5
  si ferma su `(2,0,0)`, che è **essa stessa `Rough`**. Il rifiuto che l'assertion vorrebbe
  pinnare *«avrebbe un'altra causa, o nessuna»*.

Restano solide: la **scivolata deterministica su `Ice`** e la **non-doppia propagazione**.

**Raccomandazione**: il T7 si assicura su **ciò che il turno fa davvero** — che è comunque
falsificabile e comunque prova qualcosa: l'ordine Blast → Move → Cleanup è *osservabile*, e un
`LogEventOrder` che pinna la scarica **prima** dell'acqua è un'assertion vera che documenta il
vincolo invece di negarlo. Le due battute non ottenibili **non si scrivono come `expect`**: si
lasciano dichiarate nella nota, con il rimando a #1111 che le spiega.

⚠️ Questa è la voce a rischio più alto dell'intero checkpoint, perché il corpo la presenta come
meccanica (*«non va scritto: va fatto arrivare e poi asserito»*) mentre è una decisione di
contenuto.

---

## F5 · FOWLER — 🟠 MAGGIORE · La scelta lasciata aperta per il T3 è già stata fatta da D-162

> *«Il corpo offre due strade e chiede di dichiarare quale. Una delle due contraddice una
> decisione ratificata. Non è una scelta: è una verifica.»*

Il corpo: *«Si chiude in uno di due modi, e la scelta va dichiarata: estendere il vocabolario con
un filtro su `ActionId` … oppure usare `LogEventAmount` se il valore nominale discrimina
davvero.»*

**D-162** (ratificata, `RT_PDR_00_Decision_Log.md`):

> *«un'unità che perde punti vita deve dire `Hit`/`Lethal`/`ShieldAbsorbed` … quindi la voce è
> `Combat` **anche quando la causa è ambientale** — `Status.Burning` nel Cleanup (#625) …
> **La causa di un danno la porta `ActionId`, non la categoria.**»*

∴ la strada è **il filtro su `ActionId`**. `LogEventAmount` userebbe un **numero di bilanciamento**
come discriminante d'identità: cambierebbe significato al primo ritocco del danno da fuoco, e la
sua caduta non direbbe *«il `Burning` non c'è»* ma *«il `Burning` fa un altro numero»*.

Misurato — il filtro oggi non esiste:

```
RTScenarioLoader.cpp:84  ParseScenarioLogEvent(Obj, CategoryField, OutcomeField, …)
                         → legge `category` e `outcome`. Nessun `ActionId`.
```

⚠️ Il corpo aggiunge *«conviene farlo insieme a #1060, che estende lo stesso vocabolario»*.
**Quel consiglio è scaduto**: #1060 è chiusa e il suo vocabolario è consegnato. L'estensione la
fa questa issue, da sola, sullo stesso punto d'innesto.

---

## F6 · WIEGERS — 🟡 MINORE · Due premesse del corpo mandano l'implementatore su file che non esistono

**(a) La granularità del golden non è più una decisione aperta.**

Il corpo: *«`Tests/Golden/` contiene oggi due file … non esiste un precedente multi-turno da
imitare, quindi la granularità la decide questa issue.»*

```
$ find Source/RefactorTactics/Tests/Golden -type f
… 7 corpus, 11 file — di cui TRE multi-turno:
   Spec.Environment.WaterQuenchesFire/turn-01..03.rttl
   Spec.Overwatch.HoldThenFire/turn-01..02.rttl
   Spec.Predictive.WhiffOnEmptyCell/turn-01..02.rttl
```

Il precedente esiste, ed è **presidiato**: `GoldenCorpusHasNoOrphanFolders` e
`GoldenCorpusHasNoOrphanTurns` fanno cadere una cartella non dichiarata e un `turn-NN` di troppo.
∴ la forma è **una cartella per `ScenarioId`, un file `turn-%02d.rttl` per turno prodotto**, e
questa issue la **eredita** invece di sceglierla. Otto file per lo showcase non sono una scelta:
sono la convenzione applicata a otto turni.

**(b) `parallel-batch.yaml` non esiste più.**

Il commento del 2026-08-17 avverte: *«`RT_Showcase_Relay_v01` è `integration_only` — verificare
l'assegnazione in `docs/roadmap/parallel-batch.yaml` prima di aprirlo (D-139)»*.

**D-139 è superata da D-178 il 2026-08-20**, e il Decision Log lo scrive: *«il write-set di batch
e la Binary Asset Lease sono rimossi, e `docs/roadmap/parallel-batch.yaml` non esiste più»*.
Verificato: il file non è nel repository. La cautela residua è *«una sessione per volta sui
binari»*, che non riguarda un `.json`.

---

## F7 · COCKBURN — 🟠 MAGGIORE · Il corpus golden ha già scritto chi chiude questo checkpoint, e il corpo non lo cita

> *«Il criterio di accettazione del blocco 4 esiste, è scritto nel codice, ed è più preciso di
> quello nella issue. Chi lavora sulla issue non lo trova.»*

`RTGoldenCorpusTests.cpp`, accanto a `GoldenScenarioIds`:

> *«`Objective` (#75, CP 10.2) è scoperta per **assenza di contenuto**, non di codice: la regola
> gira e i suoi test la esercitano, ma **nessuna mappa del corpus dichiara una cella obiettivo**
> … Si copre il giorno in cui uno scenario golden posa un obiettivo, e allora **la soglia qui
> sotto sale a dieci**.»*

E la soglia porta il proprio argomento di falsificabilità:

> *«Alzare questo numero è ciò che rende la copertura nuova falsificabile. Aggiungere lo scenario
> all'elenco senza toccare la soglia avrebbe lasciato il test verde **prima e dopo**.»*

∴ il blocco 4 ha un criterio di accettazione pre-scritto e misurabile che il corpo non nomina:

- `RT_Showcase_Relay_v01` entra in `GoldenScenarioIds`;
- `CategorieMinime` sale da **9** a **10**;
- l'elenco delle categorie dichiarate scoperte perde `Objective` e tiene `ReactionClash`.

⚠️ E c'è una dipendenza d'ordine con F2: lo showcase copre `Objective` **solo se** il T8 gira,
che richiede l'obiettivo nella fixture. Entrare nel corpus prima congelerebbe sette turni e
costerebbe una rigenerazione da spiegare — che è esattamente l'avvertenza che il corpo dà
(*«non prima dei blocchi 1 e 2»*) e che qui resta valida.

---

## F8 · ADZIC — 🟡 MINORE · La decisione sulla §6, chiusa

Il corpo lascia una sola decisione esplicita: *«se la §6 sia normativa — e allora il formato del
golden va scritto sui nomi nuovi — oppure illustrativa, e allora va detto quale parte lo è»*.
La nota del 2026-08-31 chiarisce che il roster non era il contenuto della domanda, «era il suo
pretesto», e la lascia aperta.

Letta la §6 per intero, la risposta è **entrambe, e il confine è leggibile**:

- **Normativo** — la formula di determinismo (rimanda ad ADR-0004 §3), i cinque campi registrati
  nel replay canonico (`OpportunityId`, `ReactionInstanceId`, `DecisionBoundary`, `Response`,
  `SelectedTargetId`), ciò che **non** entra nell'hash, il timeout come risposta canonica, e —
  testualmente — *«I file golden della showcase vivono con quelli del CP 12.6, stesso meccanismo e
  stessa cartella»* più *«rigenerazione solo con flag esplicito; la PR che rigenera dichiara
  perché»*.
- **Illustrativo** — il blocco ASCII `Turn 1 / Gadget: MoveIntent … / ReactionDecisions: Boundary
  X -> HOLD`. Porta ellissi al posto dei valori e nomi di boundary segnaposto (`X`, `Y`): non è un
  formato di file, è uno schizzo di *quali informazioni* siano input. L'input reale è lo scenario
  JSON, che quelle informazioni le porta già.

∴ **l'oracolo è la coppia `JSON + golden`** — come il corpo stesso intuisce — e la §6 va emendata
con una riga che marca quel blocco come illustrativo, invece di lasciare che qualcuno provi a
realizzarlo.

---

## F9 · CRISPIN — 🟡 MINORE · Il nome del test che invecchia

`Scenario.ShowcaseRelayV01RunsTurnOne` oggi pinna **sette** turni:

```
RTShowcaseScenarioTests.cpp:741  TestEqual(TEXT("arriva a sette turni: …"), Result.TurnsPlayed, 7);
```

Il corpo chiede la rinomina, e ha ragione. Ma un nome che dica *otto* invecchierebbe come ha fatto
*uno*. **Raccomandazione**: un nome che descriva la **proprietà** e non il numero — es.
`ShowcaseRelayV01PlaysEveryTurn` una volta che il T8 gira, oppure
`ShowcaseRelayV01ReachesItsLastPlayableTurn` se restasse un turno bloccato. Il numero resta
nell'assertion, dove cambiarlo è un diff visibile.

---

## Sintesi — cosa il panel chiede di cambiare nel DoD

| # | esperto | severità | effetto sul DoD |
|---|---|---|---|
| F1 | Wiegers | 🔴 | Il T6 e la capability si consuntivano come **già fatti da #1060/#1196**, non spuntati |
| F2 | Adzic | 🔴 | Il blocco 2 acquista la voce mancante: **l'obiettivo nella fixture**, prima della capability |
| F3 | Crispin | 🟠 | Voce nuova: **ogni assertion aggiunta è verificata per mutazione**, col nome del test che cade |
| F4 | Nygard | 🟠 | Le assertion del T7 si riducono a **ciò che il turno fa**; le due impossibili restano nella nota |
| F5 | Fowler | 🟠 | La strada del T3 è **decisa da D-162**: filtro su `ActionId`, non `LogEventAmount` |
| F6 | Wiegers | 🟡 | Granularità **ereditata**, non scelta; l'avvertenza su `parallel-batch.yaml` si rimuove |
| F7 | Cockburn | 🟠 | Il blocco 4 adotta il criterio del corpus: **`CategorieMinime` 9 → 10** |
| F8 | Adzic | 🟡 | §6: normativa salvo il blocco ASCII, che si marca illustrativo |
| F9 | Crispin | 🟡 | La rinomina evita un nome che porti di nuovo un numero |

### Punteggi

| dimensione | prima | dopo le raccomandazioni |
|---|---|---|
| chiarezza | 8.5/10 — la prosa è eccellente | 8.5/10 (invariata: il difetto non era la scrittura) |
| completezza | 6.0/10 — manca il prerequisito del T8 | 9.0/10 |
| testabilità | 5.5/10 — «asserito» definito e non richiesto | 8.5/10 |
| coerenza | 4.5/10 — quattro premesse superate dai fatti | 9.0/10 |

⚠️ **La chiarezza non è il problema di questa specifica, ed è la lezione del panel.** Il corpo è
scritto meglio della media del repository e si è difeso da solo dalla trascrizione di stato. È
invecchiato lo stesso, perché **le fonti a cui rimandava si sono chiuse** — e un rimando a una
fonte chiusa è muto esattamente come una tabella scaduta. La difesa che mancava non era «non
trascrivere lo stato», era **«dì cosa resta quando la dipendenza si chiude»**.
