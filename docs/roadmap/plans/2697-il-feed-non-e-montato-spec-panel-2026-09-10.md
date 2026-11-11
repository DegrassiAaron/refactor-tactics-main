# Spec panel — #2697 · «Il feed esiste, ha un grafo, e non è montato»

> Seduta del **2026-09-10**, aperta da `/issue-run 2697`.
> Misure su `origin/main` = `69451996`; gli asset sono letti dai blob di Git, non dal working tree di un clone.
> Referto precedente sulla stessa issue: [`2697-combat-log-senza-consumatori-spec-panel-2026-09-09.md`](2697-combat-log-senza-consumatori-spec-panel-2026-09-09.md).

Convenzione dei conteggi: i numeri qui sotto sono **esiti del passaggio corrente** e portano accanto il
comando che li produce. Nessuno è un totale che invecchia da solo.

## Il panel

| Esperto | Perché è al tavolo |
|---|---|
| **Wiegers** | la DoD di #2697 è verificabile? le voci chiuse lo sono state su evidenza? |
| **Adzic** | quale esempio falsificabile manca, e perché il verde non lo produce |
| **Crispin** | dove va l'oracolo, e quale gate avrebbe dovuto vedere la regressione |
| **Nygard** | il modo di guasto: un salvataggio d'asset che disfa il lavoro di un altro, in silenzio |
| **Fowler** | il confine fra «il widget è fatto bene» e «il widget è montato» |

---

## 1. Cosa regge — verificato riga per riga

La catena C++ che #2697 ha costruito con [#2707](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2707) è **intera**, e la misura del 2026-09-09 non decade.

| Anello | Sito su `69451996` | Esito |
|---|---|---|
| il proiettore ha un consumatore | `RTHudViewModel.cpp:502` → `URTPlayerEventProjector::Project` | ✅ |
| il consumatore ha un chiamante di gioco | `RTScreenHudWidgets.cpp:242` → `URTPlayerEventLogWidget::GetFeed()` | ✅ |
| il marcatore nel mondo | `RTHUD.cpp:994` → `ComputeBlockerMarks` | ✅ |
| l'autorizzazione precede la proiezione | `RTPlayerEventProjector.cpp` — `IsAuthorized` come primo passo | ✅ |
| la cella viaggia come argomento, non come testo | `Candidate.BlockerCell = Entry.SightBlockerCell`, dentro la guardia `AttackBlocked` | ✅ |
| la frase è narrazione, non coordinate | `ComposePlayerEventText` → `"Nessuna linea di tiro"` | ✅ |
| l'asset del feed ha radice, contenitore e grafo | `WBP_RT_EventLog` — `FeedBox`, `GetFeed`, `AddChildToVerticalBox`, `ExecuteUbergraph_WBP_RT_EventLog` (#2784, `a7c189f9`) | ✅ |
| la HUD è presentata anche sui banchi di scenario | `RTGameMode.cpp` — `EnterMatch()` in `BeginPlay`, **prima** del ramo di auto-run | ✅ |

∴ Ogni pezzo esiste e funziona. Il difetto è **fra** i pezzi.

---

## 2. I findings

### 🔴 F1 — CRITICO · Il feed non è montato in nessun albero

`WBP_RT_EventLog` non è referenziato da nulla, a parte se stesso.

```
# ogni .uasset tracciato che nomina il widget
for f in $(git ls-tree -r --name-only origin/main -- Content/ | grep "\.uasset$"); do
  git show "origin/main:$f" | tr -d "\000" | grep -qa WBP_RT_EventLog && echo "$f"
done
→ Content/RT/UI/Match/WBP_RT_EventLog.uasset      (se stesso, e nient'altro)

git grep -In "WBP_RT_EventLog" origin/main -- Source Config
→ una riga sola, ed è un commento:  UI/RTScreenHudWidgets.h:262
```

E nella tabella dei nomi di `WBP_RT_TacticalHUD` la stringa `EventLog` compare **zero** volte — misurato due
volte con metodi indipendenti: scansione ASCII del pacchetto, e scansione delle `FString` con prefisso di
lunghezza. La `ZoneRight` ospita ancora `WBP_RT_SelectedUnitPanelRight`.

⛔ **∴ la diagnosi di #2697 si sposta di un gradino per la terza volta**, e questa volta è l'ultimo:

| stesura | dove si fermava la catena |
|---|---|
| 2026-09-09 (corpo) | manca il consumatore di `Project` |
| 2026-09-09 (commento delle 12:56) | il consumatore c'è, manca il `.uasset` |
| 2026-09-10 (qui) | il `.uasset` c'è **e ha il grafo**, manca il **montaggio** |

🔑 **E ogni gradino è stato scoperto guardando, non da un rosso.** È la tesi della issue applicata a se
stessa: *«un canale che nessuno legge non produce nessun rosso»* — e nemmeno un widget che nessuno monta.

### 🔴 F2 — CRITICO · `cc5ca967` dichiara un montaggio che non ha fatto, e ne ha disfatto un altro

Il commit `cc5ca967` si intitola *«feat(2697): la destra ospita il feed invece di un secondo pannello
unita'»*. Cosa ha fatto davvero all'asset, misurato sulla tabella dei nomi:

| pacchetto | `WBP_RT_EventLog…` | `WBP_RT_ActionDockBottom` | `WBP_RT_SelectedUnitPanelBottom` | `WBP_RT_ActionDock_C` |
|---|---|---|---|---|
| `bbca9a36` — prima del fix di #2760 | assente | assente | assente | assente |
| `9943dfda` — il fix di #2760 | assente | **presente** | **presente** | **presente** |
| `cc5ca967` = `origin/main` | assente | assente | assente | assente |

E il confronto che chiude la questione: gli insiemi di stringhe di `bbca9a36` e `cc5ca967` sono **identici**
— `Compare-Object` non restituisce una sola differenza. I byte differiscono (GUID, ricompilazione), i nomi no.

∴ `cc5ca967` ha **risalvato l'albero dei widget nello stato precedente a #2760**. I due `HorizontalBox` vuoti
chiamati `WBP_RT_SelectedUnitPanel` e `WBP_RT_ActionDock` — cioè il difetto che il titolo di #2760 descrive
verbatim, *«due nodi portano il nome di un widget senza esserne un'istanza»* — sono tornati.

⚠️ **È il modo di guasto che la pratica di questa macchina già conosce**: un Editor con una copia in memoria
anteriore risalva l'asset, e lo staging lo raccoglie. Nessuno se n'è accorto perché **nessun gate guarda
l'albero**: la suite `ScreenHud` era verde prima, durante e dopo.

### 🔴 F3 — CRITICO · #2760 è chiusa e la sua correzione non è su `main`

Conseguenza diretta di F2, ma va nominata a parte perché ha un proprietario diverso e un effetto proprio:
il dock delle azioni non può popolarsi **per costruzione**, che è esattamente ciò che #2760 aveva misurato e
corretto. La issue risulta chiusa; il repository non porta il fix.

### ⚠️ F4 — MAGGIORE · Tre documenti dichiarano il montaggio come fatto

| Fonte | Cosa dichiara | Misura |
|---|---|---|
| `guida-screen-hud-umg.md` §3 | *«`RIGHT` \| `WBP_RT_EventLog` — l'istanza si chiama `WBP_RT_EventLogRight`»* | l'istanza non esiste |
| corpo di #2784 | *«è montato nella `ZoneRight` di `WBP_RT_TacticalHUD` come `WBP_RT_EventLogRight`»* | idem |
| messaggio di `cc5ca967` | *«la destra ospita il feed»* | idem |

⛔ Nessuna delle tre è stata scritta in malafede: tutte e tre riportano ciò che lo strumento aveva risposto.
Ciò che mancava era la **rilettura dell'asset dopo il salvataggio** — la stessa che `a7c189f9` ha fatto per il
grafo, e che infatti ha prodotto un record vero.

### ⚠️ F5 — MAGGIORE · La DoD 3 di #2697 non è falsificabile finché il montaggio non c'è

DoD 3, verbatim: *«Sul banco `Visual.Map.SightWallIsWalkable`, in PIE, il giocatore legge a schermo che il
tiro è fermato da un ostacolo»*.

Con il feed non montato, una seduta PIE su quel banco **non può che dare ❌**, e lo darebbe per una ragione
che la seduta non è in grado di distinguere da tutte le altre già sbagliate quattro volte su questa voce (la
colonna scambiata per la lastra, la build senza marcatore, l'auto-run scambiato per la pianificazione,
l'assenza d'intento scambiata per assenza di legame). ⚠️ **Convocare la quinta seduta prima del montaggio
significa spendere un verdetto d'autore per riscoprire un fatto che `grep` risponde in un secondo.**

### ⚠️ F6 — MEDIO · La frase del feed non nomina l'ostacolo, e la DoD 3 lo chiede

`ComposePlayerEventText` restituisce `"Nessuna linea di tiro"`. La DoD 3 chiede che il giocatore legga *«che
il tiro è fermato da **un ostacolo**»*. Il **dove** viaggia in `BlockerCell` e lo disegna il marcatore ambra;
il **cosa** non è in nessuna delle due metà.

🔑 Non è un difetto da correggere al buio: la scelta fra *«Nessuna linea di tiro»* con un marcatore accanto e
una frase che nomina l'ostacolo è una decisione di narrazione, e l'owner è **#1936 §D/§E**. Va posta come
domanda alla seduta, non risolta qui. ⛔ E in nessun caso stampando `(q=..,r=..,L=..)`, che #1936 §A vieta.

### 💬 F7 — MINORE · Il titolo di #2697 descrive la prima stesura

*«i tre canali verso lo schermo hanno zero chiamanti fuori dai test»* non è più vero da `c3151afd`: il
proiettore ne ha uno. Il titolo regge come storia e non come stato.

---

## 3. Sintesi

**🤝 Convergenza.** Tutti e cinque leggono lo stesso schema: **il difetto non è mai stato dove il verde
guardava**. Il C++ è coperto da capo a fondo — proiettore, autorizzazione, dominanza, composizione, marcatore
— e ogni anello ha il suo test. Ciò che non ha mai avuto un test è **la giunzione fra il C++ e l'asset**, ed
è lì che la catena si è rotta tre volte di fila.

**⚖️ Tensione produttiva — Crispin ⚡ Fowler.** Fowler: un test che ispeziona un `.uasset` mette la
presentazione dentro un gate di correttezza, e il layout non è materia di test. Crispin: il gate non chiede
*come* è fatto il pannello, chiede **che ci sia**; è la stessa distinzione che `ActionSlotHasIconSurface` ha
già risolto in questo file — *«il test chiede la superficie, non il binding»*. Risoluzione: l'oracolo
asserisce la **presenza nell'albero**, mai la zona, e la zona finisce nel report.

**🕸️ Il punto di leva** (Nygard). Il guasto non è un bug: è un **processo di authoring senza lettura di
ritorno**. Chiunque salvi `WBP_RT_TacticalHUD` da un Editor con una copia stantia disfa il lavoro altrui, e
oggi nulla lo intercetta. Un oracolo sull'albero non impedisce il salvataggio — lo rende **rumoroso**, che è
tutto ciò che serve perché non attraversi una PR.

**💬 La frase, per chi legge dopo.** *«#2697 ha costruito il feed intero e non l'ha attaccato allo schermo.
La prova che mancava non era una seduta PIE: era un test che guardasse l'albero della HUD.»*

**⚠️ Punto cieco.** Nessuno di noi ha guardato **gli altri** `.uasset` per lo stesso modo di guasto. Se un
Editor stantio ha risalvato la HUD, può averne risalvati altri. Non è materia di #2697 — è un follow-up.

---

## 4. Raccomandazione

**#2697 resta aperta e cambia residuo.** Non si duplica in una issue nuova: il montaggio è stato tentato
sotto il suo numero (`cc5ca967`), quindi ripulirne l'esito è lavoro suo.

### Cosa ha consegnato questa seduta

1. **L'oracolo.** Due test in `RTMatchWidgetAssetTests.cpp`, il file che già carica `WBP_RT_TacticalHUD`:
   * `RefactorTactics.ScreenHud.TheHudMountsTheFeedThatExplainsTheTurn` — l'albero contiene un
     `URTPlayerEventLogWidget`;
   * `RefactorTactics.ScreenHud.NoNodeWearsTheNameOfAWidgetWithoutBeingOne` — nessun nodo con prefisso
     `WBP_` che non sia un `UUserWidget`.

   🔑 **Ed è stato verificato che DISCRIMINANO**, che è l'unica cosa che il verde da solo non prova: eseguiti
   sull'asset **precedente** al montaggio falliscono entrambi, con i tre messaggi giusti — *«non monta nessun
   `URTPlayerEventLogWidget`»*, e i due nodi `WBP_RT_SelectedUnitPanel` / `WBP_RT_ActionDock` che *«portano il
   nome di un widget-blueprint ma sono un `HorizontalBox`»*.

2. **Il montaggio**, fatto nel clone principale con un commandlet Python headless e **riletto dal `.uasset`
   dopo il salvataggio**:

   ```
   ZoneRight            -> WBP_RT_EventLogRight            WBP_RT_EventLog_C
   ZoneBottomContainer  -> WBP_RT_SelectedUnitPanelBottom  WBP_RT_SelectedUnitPanel_C
                           WBP_RT_ActionDockBottom         WBP_RT_ActionDock_C
   ```

   ⚠️ **Gli slot sono stati letti prima della rimozione e riapplicati**, quindi il layout non cambia: la
   `ZoneRight` conserva `padding (4, 96, 4, 2)` e `FILL/FILL`, i due della fascia conservano `Automatic` e
   `FILL/FILL`. ⚠️ **E sono serviti due giri di compile+save**, per la ragione che #2760 aveva già scritto:
   `unreal.new_object` non assegna i GUID che `ConstructWidget` assegnerebbe, e
   `ValidateAndFixUpVariableGuids` li ripara **in memoria** — l'ensure *«was added but did not get a GUID»*
   si conta **3 volte** durante il primo giro e **0** al caricamento successivo.

3. **La correzione del record** — `guida-screen-hud-umg.md` §2/§3, il corpo di #2697, e le due issue chiuse i
   cui consuntivi contengono un fatto falsificato.

### Cosa resta, e a chi

| Residuo | Owner | Perché non qui |
|---|---|---|
| la frase nomina l'ostacolo? | #1936 §D/§E | è narrazione, e ha un owner documentale |
| rigiudicare `PIE-HEXPLAY-6` / `PIE-VIS-SIGHTWALL` | seduta PIE, **dopo** questo commit | è un verdetto d'autore a schermo, e chi ha scritto la correzione non lo firma da solo |
| il record di #2760 | #2760, da riaprire o annotare | la correzione è di nuovo in `main`, ma la issue è stata chiusa senza l'oracolo che la teneva |

### DoD di #2697 — emendamenti proposti

Le voci 1, 2, 4, 5 restano consuntivate ✅ e non si riaprono. Cambiano queste:

- [x] **NUOVA · `WBP_RT_TacticalHUD` monta un `URTPlayerEventLogWidget`** — fatto, e la prova è la rilettura
      del `.uasset` **dopo** il salvataggio, non la risposta dello strumento che l'ha scritto.
- [x] **NUOVA · l'oracolo esiste, è verde, e sull'asset precedente è rosso** — la seconda metà è ciò che
      distingue un oracolo da un promemoria.
- [ ] **EMENDATA · DoD 3** — la seduta PIE si convoca **su questo commit**, e giudica ciò che si legge a
      schermo, non se il widget c'è. Quella domanda l'ha ora chiusa un test.
- [ ] **DoD 6 invariata**: la rigiudicazione è UNA, si consuntiva in `test-manuali-pie.md`.

---

## 5. Punteggi

| Dimensione | Prima della seduta | Dopo gli emendamenti proposti |
|---|---|---|
| Chiarezza | 7/10 — il corpo descrive uno stato di due gradini fa | 9/10 |
| Completezza | 5/10 — la giunzione C++↔asset non è nominata da nessuna voce | 9/10 |
| Falsificabilità | **3/10** — la voce aperta si può giudicare solo in PIE, e darebbe ❌ per la ragione sbagliata | 9/10 — due voci le risponde `Automation` |
| Coerenza | **4/10** — tre documenti dichiarano un montaggio che non esiste | 9/10 |

---

## Follow-up candidates

* **Lo stesso modo di guasto altrove.** Nessuno ha verificato se altri `.uasset` abbiano subito un
  risalvataggio stantio. Il confronto fra la tabella dei nomi di un pacchetto e quella del commit che lo
  dichiara è meccanico e si può fare per tutti i `WBP_RT_*`.
* **`spec-player-event-log.md` non esiste.** Nominata da `D-299` come owner documentale della semantica degli
  eventi giocatore, e da #2784 per il budget di righe. Owner: #1936.
* **`ArcRejected` (`:606`) e `FallbackEntry` (`:784`)** escono come «nessuna linea di tiro» senza nominare il
  muro. Il criterio corrente dice `Combat`/`NoLineOfSight` e si ferma lì — estenderlo è lavoro a sé.
