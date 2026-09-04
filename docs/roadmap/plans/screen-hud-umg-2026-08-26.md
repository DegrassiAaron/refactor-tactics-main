# Screen HUD in UMG — piano di implementazione (CP 11.7 / #613)

> **Per chi esegue:** i passi usano checkbox (`- [ ]`). I task **1** e **8** sono codice C++ e si fanno
> con `superpowers:test-driven-development`. I task **2–7** sono **lavoro d'Editor su `.uasset`**: nessun
> agente li può fare, e la loro verifica è a occhio dentro l'Editor.

**Obiettivo:** dare allo Screen HUD (§4.1) i sei `WBP_RT_*` che gli mancano e un posto da cui comparire,
chiudendo l'anello *letto* che oggi rende invisibile un view model già scritto e già testato.

**Architettura:** le classi base C++ e le viste sanitizzate esistono già; questo piano aggiunge (a) un
**layer HUD** nel `URTFrontendNavigator`, separato dallo stack delle schermate, e (b) i sei Blueprint che
derivano dalle classi base e si limitano a disporre e a legare. Nessuna regola di gioco si sposta, e
`ARTHUD` (§4.2, Canvas) non si tocca.

**Tech stack:** UE 5.8.1 · C++ (`REFACTORTACTICS_API`) · UMG · `UGameInstanceSubsystem` · automation
`IMPLEMENT_SIMPLE_AUTOMATION_TEST`.

**Spec:** [`progettazione-hud.md`](../../technical/systems/progettazione-hud.md) §4.1 (cosa mostrare) ·
[`guida-screen-hud-umg.md`](../../technical/runbooks/guida-screen-hud-umg.md) (ricetta d'Editor) ·
issue [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613).

---

## ⛔ Superata in un punto — 2026-09-04: la barra dell'energia non ha più un dato

> Questo piano prescrive due volte una **barra dell'energia** — Step 4.2 (*«una seconda [Progress Bar] per
> l'energia»*), Step 5.3 (*«energia → `Percent` = `Energy / MaxEnergy`»*) — e dichiara `Energy/MaxEnergy`
> fra i campi di `FRTUnitCardView`.
>
> 🔴 **Quei campi non esistono più.** [`D-324`](../../decisions/RT_PDR_00_Decision_Log.md) ha tolto `Energy`
> dal gameplay e [#610](https://github.com/DegrassiAaron/refactor-tactics-main/issues/610) l'ha
> implementato: `FRTUnitCardView` non porta `Energy`/`MaxEnergy`, e `URTUnitOverlayWidget` non ha più
> `EnergyBar`.
>
> ⛔ **Gli step qui sotto NON sono stati riscritti**: sono spuntati, cioè sono il registro di lavoro
> realmente fatto il 2026-08-26, e correggerli cancellerebbe la storia invece di aggiornarla. Chi rilegge il
> piano per rifare un widget salti la barra dell'energia; chi cerca *perché* c'era, la trova qui.
>
> ⚠️ **Nei due `WBP_RT_*` la barra è ancora disegnata** — `WBP_RT_UnitOverlay` (`EnergyBar`) e
> `WBP_RT_UnitCard`. Il binding era `BindWidgetOptional`, quindi non rompe nulla: resta un elemento inerte
> che nessuno aggiorna, e va tolto dal Blueprint in Editor.

---

## 🔄 Riconciliazione del registro — 2026-08-30

> Il piano ha portato **61 caselle e zero spuntate** per quattro giorni, mentre il lavoro arrivava fino al
> Task 7. Non era una svista cosmetica: un kit d'autore consumato il 2026-08-30 ordinava di **costruire
> `WBP_RT_TurnHeader`**, che esiste dal 2026-08-26, e la sua premessa nasceva dal leggere queste caselle.
> Il referto è [`debug-hud-graybox-spec-panel-2026-08-30.md`](debug-hud-graybox-spec-panel-2026-08-30.md).

**Misurato su `285d2322`** (`origin/main`). **36 caselle su 61** risultavano fatte e sono state spuntate.

> 🔧 **Aggiornamenti del 2026-08-30**: lo **Step 7.4 è stato implementato e chiuso** — vedi
> «Lo stato neutro del dock» più sotto; la **guardia** dello Step 3.4 è scritta (resta la sua seconda riga);
> e la **suite intera** è stata misurata per la prima volta, Step 8.1. Il conteggio sale a **38 su 61**.

**Metodo, e il suo confine.** Il C++ e i `.ini` sono letti con `git grep` e citati per riga. I sette
`.uasset` sono misurati **per estrazione di stringhe** — `perl -ne 'while(/([\x20-\x7E]{4,})/g){print
"$1\n"}'` — che dice *quali elementi, quali binding e quali funzioni* un Blueprint contiene, e **non** come
si vede a schermo. Le cinque caselle «verifica a schermo» restano quindi aperte per costruzione: nessun
agente le può chiudere, come dice il preambolo.

| Task | Fatte | Aperte | Nota |
|---|---:|---:|---|
| 1 — layer HUD nel navigator | 6 / 9 | 3 | atterrato per intero; le tre aperte sono passi di processo TDD |
| 2 — `WBP_RT_TacticalHUD` | 4 / 6 | 2 | `Zone_Top`, `ZoneLeft`, `ZoneRight`, `ZoneBottom` in un `CanvasPanel` |
| 3 — `WBP_RT_TurnHeader` | 5 / 7 | 2 | lo Step 3.4 è **per metà**: guardia fatta, binding `Visibility` no |
| 4 — `WBP_RT_TeamRoster` | 5 / 6 | 1 | `GetRoster` → `ForEach` → `WBP_RT_UnitCard` |
| 5 — `WBP_RT_SelectedUnitPanel` | 5 / 6 | 1 | i tre slot leggono `bOccupied`/`DisplayName`, non li deducono |
| 6 — `WBP_RT_ActionSlot` | 5 / 5 | 0 | **chiuso** |
| 7 — `WBP_RT_ActionDock` | 5 / 6 | 1 | lo Step 7.4 **chiuso il 2026-08-30** |
| 8 — chiusura | 3 / 16 | 13 | guida corretta, e la suite intera misurata il 2026-08-30 |

### Quattro cose che la misura ha trovato, e che cambiano il piano

**✅ Lo Step 7.4 non era implementato, ed è stato chiuso lo stesso giorno.** `WBP_RT_ActionDock` **non
chiamava `GetArmedActionIndex()`** — la funzione esiste in C++ (`RTScreenHudWidgets.cpp:222`) ma la stringa
era assente dal `.uasset`. Vedi «Lo stato neutro del dock» qui sotto per cosa è stato trovato e cosa è
cambiato.

**⚠️ Lo Step 3.4 era per metà, e ora manca solo la sua seconda riga.** Il troncamento c'era (`FTrunc`, da
`16ba67a4`), la guardia no: fuori dal Planning il campo vale `-1` e a schermo **compariva un numero
negativo**. ✅ **La guardia è stata scritta il 2026-08-30** — vedi «La guardia sul timer» più sotto.
⛔ **Resta il binding `Visibility → HasMatchContext` sul `TimerText`**, e la ragione per cui non è stato
fatto è scritta lì: non è un rinvio per stanchezza, è un limite dello strumento più una domanda aperta.

**🔺 Lo Step 2.3 è superato dai fatti.** Diceva *«lascia `IconCatalog` vuoto, il catalogo è di #220 e non
esiste»*. Oggi `Content/RT/UI/DA_IconCatalog.uasset` **esiste** con 62 icone, e `WBP_RT_TacticalHUD` lo
assegna. La casella resta vuota perché l'istruzione non è stata eseguita — è stata **sorpassata**, e
`#220` risulta ancora `OPEN`: chi la chiude dovrebbe misurare che i widget già consumano il catalogo.

**➕ `WBP_RT_UnitCard` non era nel piano, ed è meglio di ciò che il piano chiedeva.** Gli Step 4.2 e 5.3
prescrivevano di disegnare barre HP/energia/scudo **dentro** roster e pannello — due volte le stesse barre.
L'implementazione ha estratto una card riusabile: `ProgressBar`, `Percent`, `Health`, `Energy`, `Shield`,
`Opacity`, `bAlive` stanno **solo** lì, e i due consumatori la istanziano. Le due caselle sono spuntate su
quell'evidenza. La card deriva da `UUserWidget` e non da una base RT, ed è deliberato:
`RTMatchWidgetAssetTests.cpp` la esclude dal test di parentela **dicendolo**.

### Il vincolo 🔴 più importante regge

*«Nessun widget referenzia una texture»* (**D-031**) è **verificato su tutti e sette** i `.uasset`: nessuno
punta a `/Game/RT/UI/Icons/`. `WBP_RT_ActionSlot` risolve l'icona con `GetResolvedIcon()`
(`RTScreenHudWidgets.cpp:248`) e converte la soft reference in `Texture2D` a runtime. L'unico riferimento
non-widget dell'intero HUD è `DA_IconCatalog`, dal solo `WBP_RT_TacticalHUD` — che è esattamente il
meccanismo previsto. Il piano avverte che *«questa metà è tua, e nessun gate la controlla»*: la metà tiene.

### Lo stato neutro del dock — Step 7.4, chiuso il 2026-08-30

Il grafo, letto come DSL dal ponte MCP dell'Editor, diceva questo:

```lisp
(RefactorTactics|HUD|SetAction _aswbp_rt_action_slot _array_element false (GetIconCatalog self))
```

**`bArmed` era una costante `false`.** Il difetto quindi non era quello che il piano temeva — «un dock che
mostra sempre uno slot armato» — ma il suo opposto: **nessuno slot poteva accendersi mai**. Lo stato neutro
appariva corretto per la ragione sbagliata, e l'armamento non arrivava a schermo in nessun caso.

**Un secondo difetto era annodato al primo.** La guardia di ricostruzione confrontava la lunghezza di
`GetActions()` con quella della variabile `SlotWidgets` — che veniva **svuotata e mai ripopolata**. Restava
quindi sempre a zero, la condizione era sempre vera, e il dock **ricostruiva ogni slot a ogni tick**:
`CreateWidget` + `Cast` + `AddChild` per ogni azione, ogni frame. Non si poteva riparare il primo difetto
lasciando il secondo — con `SetAction` corretto, quel lavoro per frame sarebbe solo aumentato.

Il grafo ora è:

```lisp
(bind _actions (RefactorTactics|HUD|GetActions))
(if (!= (Length _actions) (Widget|Panel|GetChildrenCount (GetSlotBox)))   ; ← lo stato REALE del pannello
  (ClearChildren (GetSlotBox))
  (for _e _actions  … CreateWidget → Cast → AddChildToHorizontalBox …))
(for _index (range (Length _actions))
  (bind _slot (CastToWBP_RT_ActionSlot (Widget|Panel|GetChildAt (GetSlotBox) _index)))
  (SetAction _slot (Array|Get _actions _index)
             (== _index (RefactorTactics|HUD|GetArmedActionIndex))      ; ← Step 7.4
             (GetIconCatalog self)))
```

- La guardia legge `GetChildrenCount` del pannello invece della variabile morta: è lo stato vero, e non
  dipende da una variabile che qualcuno deve ricordarsi di tenere allineata. La ricostruzione avviene ora
  **solo** quando il kit cambia numero di azioni.
- `bArmed` è `(== _index (GetArmedActionIndex))`, **senza `Select`** — come lo step prescrive
  esplicitamente: con `INDEX_NONE` il confronto è falso per ogni indice di lista, e nessuno slot si accende.
- ⚠️ `SlotWidgets` resta nel Blueprint **come variabile non più usata**. Rimuoverla è fuori da questo passo.

**Verifica**: `RefactorTactics.ScreenHud` **9/9**, `RefactorTactics.Frontend` **80/80**,
`RefactorTactics.HUD` **16/16** — eseguiti dentro l'Editor via `AutomationTestToolset`, non da `rt-suite`.
Il Blueprint compila senza errori né warning, e il `.uasset` salvato contiene ora `GetArmedActionIndex` ed
`EqualEqual_IntInt`. ⛔ **Nessuna verifica a schermo**: lo Step 7.5 resta aperto, ed è lì che si vede se lo
slot armato si accende davvero in partita.

### La guardia sul timer — Step 3.4, prima metà, 2026-08-30

Il binding era questo, e non aveva nessuna guardia:

```lisp
(fn Get_TimerText_Text ()
  (return (Utilities|Text|ToText (Math|Float|Truncate (BreakRTMatchHeaderView (GetHeader))))))
```

`PlanningSecondsRemaining` vale **`-1` quando la domanda non si applica** — fuori dal Planning, o senza
contesto — e quel `-1` finiva a schermo come numero. Ora:

```lisp
(fn Get_TimerText_Text ()
  (bind (_round _limit _phase _secs _resolving) (BreakRTMatchHeaderView (GetHeader)))
  (return (select (< _secs 0.0) "—" (Utilities|Text|ToText (Math|Float|Truncate _secs)))))
```

Il glifo è **`—` (U+2014), lo stesso che `GetRoundCounterText()` restituisce** per «nessun contesto»
(`RTScreenHudWidgets.cpp:141`) — non `--:--`, che avrebbe messo due segni diversi per la stessa idea nello
stesso widget. Verificato nel binario: la `FString` è `FE FF FF FF 14 20 00 00`, cioè lunghezza `-2`
(UTF-16) e U+2014, **una** occorrenza ben formata.

⚠️ **`read_graph_dsl` rende quel glifo come `â€”`** — i byte UTF-8 letti come CP1252. È un difetto della
**lettura**, non del dato: il `.uasset` è corretto. Non inseguirlo riscrivendo il letterale.

**Verifica**: `RefactorTactics.ScreenHud` + `.HUD` + `.Frontend` in headless →
**105 test, 105 `Success`, zero fallimenti**.

#### ⛔ Cosa NON è stato fatto: il binding `Visibility → HasMatchContext`

La seconda riga dello step chiede di legare la `Visibility` del `TimerText` a `HasMatchContext`
(`Visible`/`Collapsed`). **Non è creabile dal ponte MCP**, misurato: `UMGToolSet` non ha un tool per i
property binding — il suo `BindToEventProperty` lega i **delegate multicast** (`OnClicked`), non le
proprietà — e `ProgrammaticToolset` è sandboxed sui soli `re, copy, time, datetime, math, json`, senza il
modulo `unreal`. Un property binding è una `FDelegateEditorBinding` in `WidgetBlueprint->Bindings`, e per
scriverla serve un gesto nel pannello Details. ⚠️ Una terza via — eseguire Python nella console dell'Editor
— **non è stata verificata**: l'Editor si è chiuso durante la ricognizione.

🤔 **E prima di farlo, vale una domanda all'owner.** `RoundText` mostra `—` senza contesto **e resta
visibile**. Se `TimerText` si collassasse nello stesso stato, l'header userebbe due comportamenti diversi
per la stessa condizione, che è la classe di incoerenza contro cui è scritto il Task 7-bis. Con la guardia
appena aggiunta, «nessun contesto» è già rappresentato — e in modo uguale al fratello accanto. Il binding
resta da fare se si decide che il timer debba *sparire* invece che dire `—`; non è una scelta da prendere
in silenzio mentre si chiude una casella.

### La suite intera — Step 8.1, 2026-08-30

Mai eseguita per intero fino a oggi. Misurata con `scripts/rt-suite.ps1` (default `-Filter RefactorTactics`),
che è la via giusta proprio per l'avvertenza di questo step — *«una suite troncata da un crash sembra
verde»*: lo script confronta il `Found N` dichiarato in testa al log con i `Test Completed`, e senza quel
riscontro dichiara **NON VALIDA** invece di verde.

```text
[RT-MEASURE] VALIDA
[RT-MEASURE]   HEAD      71261937  albero ae48caf4
[RT-MEASURE]   esito     1397/1397 completati, 0 fallimenti
[RT-MEASURE]   durata    04:48
```

`LogAutomationCommandLine: Found 1397 automation tests based on 'RefactorTactics'` — **1397 trovati, 1397
completati, zero fallimenti**, exit `0`. I due gruppi che lo step chiede di guardare in particolare sono
verdi: `RefactorTactics.HUD.*` **9/9** e `RefactorTactics.ScreenHud.*` **9/9**; `RefactorTactics.Frontend.*`
**80/80**. Il §4.2 non ha regressioni: le due modifiche d'Editor di oggi non hanno avuto effetti fuori dal
loro layer.

⚠️ **`1397` è il numero misurato su `71261937`, non un numero da copiare.** Lo Step 8.6 lo pretende
rimisurato al momento del consuntivo: fra qui e la chiusura di `#613` altre sessioni aggiungono test, e
`1397` invecchia in giornata.

### Cosa resta, in ordine

1. **Step 3.4, seconda riga** — il binding `Visibility` del timer, se la domanda qui sopra ha risposta sì.
3. **Le cinque «verifica a schermo»** — 2.5, 3.6, 4.5, 5.5, 7.5 — più il **Task 8** quasi intero: la suite
   non è stata rieseguita, `PIE-V01-HUD` non ha esito nel registro (la seduta **U15** ha `artifacts: []` e
   `done_when: le voci hanno esito reale nel registro`), e `#613` è ancora `OPEN`.
4. **Il Task 7-bis** resta una decisione aperta, come l'ha lasciata il 2026-08-26. ⚠️ Il suo vincolo di
   lessico non è verificabile da fuori: `percorso` / `occupata` / `armata` **non compaiono** nel
   `.uasset` del pannello, ma i tre testi arrivano da `DisplayName` della vista — quindi il ripiego
   dipende da cosa restituisce il dato, non dal Blueprint.

⛔ **Il primo giro di questa riconciliazione non ha eseguito nulla del piano**: nessuna suite, nessuna build, nessun Editor
aperto, nessuna scrittura su GitHub. Ha solo misurato e registrato. Il **secondo giro** — lo Step 7.4 — ha
aperto l'Editor su un worktree isolato, modificato un `.uasset` e rieseguito tre suite; resta senza
verifica a schermo e senza scrittura su GitHub.

⚠️ **Il difetto dello Step 7.4 è vissuto in `main` senza che nulla lo segnalasse**, e la ragione è scritta
nello step stesso: *«nessun gate lo controlla»*. `RTMatchWidgetAssetTests.cpp` prova i **property binding**
dentro i `.uasset` — `Class->Bindings` — e una chiamata nell'Event Graph non è un binding: nessun test la
vedeva.

✅ **Ora un gate c'è, ed è migliore di quello che avevo previsto.** L'idea iniziale era ispezionare gli
`UbergraphPages` dal modulo editor — cioè provare la **struttura** del grafo. Il gate scritto invece prova
il **comportamento**: `RefactorTactics.ScreenHud.DockArmsOnlyTheSelectedAction` arma un'azione e verifica
che si accenda uno slot, quello giusto. Un grafo riscritto in un altro modo ma corretto passa; un grafo che
somiglia a quello giusto ma non accende niente cade. Vedi «Il HUD guardato da uno scenario» qui sotto.

---

## 🔬 Il HUD guardato da uno scenario — 2026-08-30

La domanda che l'ha aperta: *«riusciamo a testare con scenari, anche creati ad hoc?»*. Sì, e la catena è
riusabile per le altre verifiche funzionali.

```text
Scenarios/Spec/Hud/*.json          costruisce lo STATO dal percorso di gioco reale
        ↓                          (eroi veri dal catalogo ⇒ kit di azioni vero)
URTScenarioRunner::Run             NON smonta il mondo: `RunSingle(..., bTearDownAfter=false)`
        ↓
il test aggancia il BLUEPRINT      e legge cosa mostra — non la classe base
```

Tre cose l'hanno resa possibile, e nessuna è stata inventata per l'occasione:

1. **Il runner lascia gli actor in piedi apposta.** Il suo commento lo dichiara: *«il mondo lo possiede il
   chiamante, e ripulirlo qui gli toglierebbe da sotto i piedi gli actor su cui potrebbe voler guardare»*.
2. **Uno scenario può fermarsi in Planning**: `turns: []` è valido, e la partita resta al round 1 — l'unico
   momento in cui il dock ha senso. Non serve simulare turni per guardare il HUD.
3. ➕ **`SetSelectedUnitForTest`, aggiunto qui.** È l'unico pezzo nuovo, ed è l'altra metà di
   `SetMatchContextForTest` che già esisteva. Senza, **tre widget su sette non erano verificabili affatto**:
   `GetSelectedUnit()` passa da `GetOwningPlayer()`, e `UUserWidget::SetOwningPlayer` memorizza il
   `ULocalPlayer` — che in headless non esiste. Spawnare un `ARTPlayerController` e chiamargli `SelectUnit`
   **non basta**, ed è la ragione per cui pannello unità, dock e slot potevano provare solo il ramo
   «nessuna selezione».

### La prova che il gate non è vacuo

Un test verde non prova di poter fallire, e qui la verifica di mutazione non è stata inventata: il
`.uasset` è stato **riportato alla versione di `53958620`** — quella con `bArmed` costante `false`, viva in
`main` fino a stamattina — e il test è caduto sull'asserzione giusta:

```text
Expected 'e a schermo si accende UNO slot solo' to be 1, but it was 0
```

Poi ripristinato e riverificato. **10/10 `Success`** su `RefactorTactics.ScreenHud` con l'asset corretto.

### Cosa questo NON copre, e resta di `PIE-V01-HUD`

Colori, font, ingombro, leggibilità e «centro libero». Il test prova che il widget **legge il dato giusto**,
non che si veda bene — la stessa distinzione che `RTMatchWidgetAssetTests.cpp` dichiara nel suo docstring.
Lo Step 7.5 resta aperto: la sua metà funzionale ora ha un gate, la sua metà visiva no.

### Le altre tre, scritte lo stesso giorno

Su un secondo scenario, `Spec.Hud.MatchWithTwoAllies` — due alleate e un'avversaria, perché con una sola
alleata «solo la propria squadra» non significherebbe niente.

| Test | Cosa fissa |
|---|---|
| `RosterShowsOnlyOwnTeamAndKeepsTheFallen` | il roster elenca le due alleate e **nessuna** avversaria, e tiene in lista chi è a zero HP |
| `SlotsAreReadNotDeduced` | i tre slot si leggono uno per uno: un percorso occupa MOVEMENT e **lascia MAIN libero** |
| `RoundLimitComesFromTheFormat` | il contatore segue il formato — 12, poi 7, poi nessun limite — **e** `RoundText.Text` è legato a `GetRoundCounterText` |

**`RefactorTactics.ScreenHud` passa da 9 a 13 test**, tutti `Success`, run `VALIDA`.

#### 🔺 Lo Step 5.5 chiedeva una prova che non è più eseguibile

Dice: *«pianificando uno `Sprint` si accendono MOVEMENT e MAIN insieme — è la prova che il widget legge i
tre campi invece di dedurli»*. Ma **nessuna azione dei cataloghi dichiara oggi `MovementAndMain`**:
`Action.Sprint` lo faceva fino a **D-028**, e lo scrivono sia `RTCatalogLibrary.h` sia `BuildUnitSlots` —
quella riga di codice è *«inerte, non superflua: torna a contare il giorno che un kit usa quella forma»*.

La prova equivalente eseguibile oggi è l'**indipendenza** dei tre campi, ed è quella che il test fissa: un
percorso occupa MOVEMENT e lascia MAIN libero; un'azione principale occupa MAIN e lascia REACTION libera.
Un pannello che deducesse uno slot dall'altro — *«se ho pianificato qualcosa, allora ho speso il turno»* —
cadrebbe. È la stessa domanda dello step, posta a un catalogo che nel frattempo è cambiato.

#### La prova che nemmeno questi sono vacui

Il filtro di squadra di `BuildTeamRoster` è stato rimosso, ricompilato, ed eseguito:

```text
Expected 'il roster ha una riga per alleata, e nessuna per l'avversaria' to be 2, but it was 3
```

Poi ripristinato **e ricompilato** — `rt-suite` non rileva un binario stantio, quindi senza il secondo
rebuild avrebbe dichiarato `VALIDA` misurando codice che non esiste più. Verde riconfermato a **13/13**,
con l'albero tornato al digest di prima della mutazione.

### Cosa resta delle sei «verifiche a schermo»

Le caselle **restano vuote**, e non per pignoleria: la loro metà funzionale ora ha un gate, la metà visiva
no. Quello che nessun test copre è ciò per cui `PIE-V01-HUD` esiste — leggibilità a risoluzione di gioco,
ingombro delle quattro zone, **centro libero**, coerenza durante il playback della risoluzione, e il debug
spento nella vista giocatore.

---

## Global constraints

- **UE 5.8.1.** Non inventare API: verifica le firme realmente presenti.
- **Niente GAS.** `URTActionData` / `URTHeroData` / `URTEquipmentData`.
- 🔴 **Nessun widget referenzia una texture.** Le icone viaggiano come `FName`
  (`UI.Icon.Action.Move`) e si risolvono dal catalogo — **D-031**. Vale anche nei Blueprint, dove nessun
  gate lo vede.
- 🔴 **I widget non ricalcolano.** Leggono solo le viste di `URTHudViewModel`. Se manca un dato, la
  risposta è aggiungere un campo alla vista, **non** `Get All Actors Of Class`.
- 🔴 **Il centro dello schermo resta libero.** Il §4.2 continua a disegnarci sopra path, AoE e barre
  ancorate: un pannello centrale glieli coprirebbe.
- ⛔ **`ARTHUD::DrawHUD` non si migra.** La spec dice che il §4.2 «non deve essere realizzato come grandi
  widget HUD statici», e sono 910 righe coperte da `RefactorTactics.HUD.*`.
- ⛔ **`RTScreenIds::Match` non riceve un binding widget.** «Nessun widget» è la sua definizione: darglielo
  metterebbe qualcosa sopra il gioco a ogni `RESUME`.
- **Nomi dei Blueprint esatti**, da `progettazione-hud.md` §45: `WBP_RT_TacticalHUD`, `WBP_RT_TurnHeader`,
  `WBP_RT_TeamRoster`, `WBP_RT_SelectedUnitPanel`, `WBP_RT_ActionDock`, `WBP_RT_ActionSlot`. Su un
  `.uasset` il rename costa più che scriverlo giusto.
- **Debug spento** nella vista giocatore: `bShowDebug` resta `false`.
- **Un binario si tocca da un lavoro solo per volta.** Due `.uasset` non si fondono.

---

## Stato verificato — 2026-08-26

Misurato su `origin/main`, non letto dalle issue. **Tre premesse scritte in #613 e nella guida non
reggono più**, e cambiano il lavoro:

| Premessa scritta | Misura di oggi |
|---|---|
| «`Content/RT/UI/` non esiste, va creata» | esiste, con otto `WBP_RT_*` di frontend |
| «i `.uasset` non sono versionati» | ✅ `.gitignore:78` — `!Content/RT/UI/**/*.uasset` li **include** |
| «non c'è codice che aggiunga l'HUD al viewport» | c'è `URTFrontendNavigator`, **unico** autorizzato a `CreateWidget` |

Cosa esiste già, e non va rifatto:

| Pezzo | Dove | Stato |
|---|---|---|
| Sette classi base dei widget | `UI/RTScreenHudWidgets.h` | ✅ |
| Viste sanitizzate | `UI/RTHudViewModel.h` | ✅ |
| Quattro test dell'API widget | `Tests/RTScreenHudWidgetTests.cpp` | ✅ verdi |
| Modulo `UMG`/`Slate`/`SlateCore` | `RefactorTactics.Build.cs:27-29` | ✅ |
| Catalogo icone `.uasset` | — | ⛔ **non esiste** (#220): si procede col missing-icon visibile |
| I sei `WBP_RT_*` del tactical HUD | `Content/RT/UI/Match/` | ⛔ **questo lavoro** |
| Un posto da cui l'HUD compare | — | ⛔ **questo lavoro** (Task 1) |

⚠️ **Nessuna classe base usa `meta=(BindWidget)`**: nel Designer non devi nominare gli elementi in un modo
preciso. Il contratto passa da funzioni `BlueprintPure`, che si collegano ai **binding di proprietà**.

### Perché il layer HUD non è una schermata

`SyncPresentation` smonta ogni widget di `LiveWidgets` che non sia la cima dello stack o un modale
(`RTFrontendNavigator.cpp:661-669`). Un HUD registrato come schermata **sparirebbe all'apertura della
pausa**. Deve quindi vivere in un campo suo, fuori dalla mappa, con un `ZOrder` sotto le schermate.

```text
URTFrontendNavigator
├── LiveWidgets   [Main] [Pause] [Error] …   ZOrder 0 / 1000
└── MatchHudWidget   WBP_RT_TacticalHUD      ZOrder -100   ← nuovo, fuori dalla mappa

EnterMatch()  → InitializeFrontend(Match) → PresentMatchHud()
ReturnMain()  → InitializeFrontend(Main)  → DismissMatchHud()
```

---

## Struttura dei file

| File | Responsabilità | Azione |
|---|---|---|
| `Source/RefactorTactics/Frontend/RTFrontendNavigator.h` | dichiara il layer HUD e il suo binding di config | Modifica |
| `Source/RefactorTactics/Frontend/RTFrontendNavigator.cpp` | presenta, smonta, e disabilita sotto un modale | Modifica |
| `Source/RefactorTactics/Tests/RTFrontendMatchHudTests.cpp` | i quattro test del layer | **Crea** |
| `Config/DefaultGame.ini` | il percorso del `.uasset` | Modifica |
| `Content/RT/UI/Match/WBP_RT_*.uasset` | i sei Blueprint | **Crea (Editor)** |
| `docs/technical/runbooks/guida-screen-hud-umg.md` | ricetta: tre premesse da correggere | Modifica |

Il layer va nel navigator e non in una classe nuova perché l'invariante che rende verificabile tutto il
frontend — *un solo posto chiama `CreateWidget`* — è dichiarata in `RTFrontendNavigator.h:80` e si prova
con un `grep`. Una seconda classe che istanzia widget la romperebbe per guadagnare un file.

---

## Task 1 — Il layer HUD nel navigator

**Files:**
- Modifica: `Source/RefactorTactics/Frontend/RTFrontendNavigator.h`
- Modifica: `Source/RefactorTactics/Frontend/RTFrontendNavigator.cpp`
- Modifica: `Config/DefaultGame.ini`
- Test: `Source/RefactorTactics/Tests/RTFrontendMatchHudTests.cpp` (nuovo)

**Interfacce:**
- Consuma: `InitializeFrontend`, `SyncPresentation`, `FRTScreenStack`, `RTScreenIds::Match` (esistenti)
- Produce: `PresentMatchHud()`, `DismissMatchHud()`, `GetMatchHudWidget()`,
  `SetMatchHudWidgetClassForTest(TSoftClassPtr<UUserWidget>)` — il Task 8 usa il binding di config;
  i test usano il setter.

---

- [x] **Step 1.1 — Scrivi i quattro test falliti**

Crea `Source/RefactorTactics/Tests/RTFrontendMatchHudTests.cpp`. Il fixture ricalca
`RTFrontendPauseTests.cpp`, che è il precedente più vicino.

```cpp
// CP 11.7 (#613) — il layer HUD: dove compare, quando se ne va, e perche' non e' una schermata.
//
// ⚠️ Qui non si prova un layout. L'ingombro del §4.1 e il centro libero sono `PIE-V01-HUD`. Cio' che si
// prova senza editor e' il ciclo di vita: un HUD che sopravvive alla pausa, se ne va col ritorno al menu,
// e resta inerte sotto un modale.

#include "Misc/AutomationTest.h"
#include "Frontend/RTFrontendNavigator.h"
#include "Frontend/RTFrontendScreenIds.h"
#include "Engine/GameInstance.h"
#include "Blueprint/UserWidget.h"
// Una `UUserWidget` concreta da registrare: `UUserWidget` e' `Abstract` e `CreateWidget` la rifiuta.
#include "UI/RTScreenHudWidgets.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTMatchHudTestsLocal
{
	URTFrontendNavigator* MakeStartedNavigator(UGameInstance*& OutGI)
	{
		OutGI = NewObject<UGameInstance>(GetTransientPackage());
		if (!OutGI)
		{
			return nullptr;
		}
		OutGI->AddToRoot();
		OutGI->Init();

		URTFrontendNavigator* Nav = OutGI->GetSubsystem<URTFrontendNavigator>();
		if (!Nav)
		{
			return nullptr;
		}

		// Una schermata sola basta: qui si prova il HUD, non la navigazione.
		TArray<FRTScreenBinding> Screens;
		FRTScreenBinding Main;
		Main.ScreenId = RTScreenIds::Main;
		Main.WidgetClass = URTTurnHeaderWidget::StaticClass();
		Screens.Add(Main);
		Nav->StartFrontendFrom(Screens);

		// Il HUD si dichiara come i test dichiarano tutto il resto: iniettandolo, non dal `.ini`.
		Nav->SetMatchHudWidgetClassForTest(URTTacticalHUDWidget::StaticClass());
		return Nav;
	}

	void ReleaseNavigator(UGameInstance* GI)
	{
		if (GI)
		{
			GI->Shutdown();
			GI->RemoveFromRoot();
		}
	}
}

/**
 * Il HUD compare con la partita, e non prima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchHudAppearsWithTheMatchTest,
	"RefactorTactics.Frontend.MatchHudAppearsWithTheMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchHudAppearsWithTheMatchTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTMatchHudTestsLocal::MakeStartedNavigator(GI);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav)) { RTMatchHudTestsLocal::ReleaseNavigator(GI); return false; }

	TestNull(TEXT("nel menu non c'e' HUD di partita"), Nav->GetMatchHudWidget());

	TestEqual(TEXT("EnterMatch"), Nav->EnterMatch(), ERTNavResult::Ok);
	TestNotNull(TEXT("con la partita, il HUD c'e'"), Nav->GetMatchHudWidget());

	RTMatchHudTestsLocal::ReleaseNavigator(GI);
	return true;
}

/**
 * 🔴 Il test che vale il layer: il HUD NON e' una schermata.
 *
 * `SyncPresentation` smonta ogni widget di `LiveWidgets` che non sia la cima o un modale. Se il HUD ci
 * stesse dentro, la pausa lo farebbe sparire — e a schermo resterebbe la partita nuda sotto un menu.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchHudSurvivesThePauseTest,
	"RefactorTactics.Frontend.MatchHudSurvivesThePause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchHudSurvivesThePauseTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTMatchHudTestsLocal::MakeStartedNavigator(GI);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav)) { RTMatchHudTestsLocal::ReleaseNavigator(GI); return false; }

	Nav->EnterMatch();
	UUserWidget* Hud = Nav->GetMatchHudWidget();
	if (!TestNotNull(TEXT("il HUD c'e'"), Hud)) { RTMatchHudTestsLocal::ReleaseNavigator(GI); return false; }

	Nav->ShowPause();
	TestEqual(TEXT("il HUD e' lo stesso widget"), Nav->GetMatchHudWidget(), Hud);
	TestNull(TEXT("e non e' registrato come schermata"), Nav->FindLiveWidget(RTScreenIds::Match));

	Nav->ResumeMatch();
	TestEqual(TEXT("e il RESUME non lo ricrea"), Nav->GetMatchHudWidget(), Hud);

	RTMatchHudTestsLocal::ReleaseNavigator(GI);
	return true;
}

/**
 * Sotto un modale il HUD resta VISIBILE e INERTE — non nascosto.
 *
 * E' la meta' di `Modal > HUD > world` che questo checkpoint puo' consegnare: un modale che oscura il
 * contesto che sta interrompendo e' peggio di nessun modale, ma un HUD cliccabile sotto una pausa e' un
 * secondo ruleset. Il resto della precedenza e' di CP 11.8 (#705).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchHudIsInertUnderThePauseTest,
	"RefactorTactics.Frontend.MatchHudIsInertUnderThePause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchHudIsInertUnderThePauseTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTMatchHudTestsLocal::MakeStartedNavigator(GI);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav)) { RTMatchHudTestsLocal::ReleaseNavigator(GI); return false; }

	Nav->EnterMatch();
	UUserWidget* Hud = Nav->GetMatchHudWidget();
	if (!TestNotNull(TEXT("il HUD c'e'"), Hud)) { RTMatchHudTestsLocal::ReleaseNavigator(GI); return false; }

	TestTrue(TEXT("in partita il HUD riceve input"), Hud->GetIsEnabled());

	Nav->ShowPause();
	TestFalse(TEXT("sotto la pausa e' inerte"), Hud->GetIsEnabled());

	Nav->ResumeMatch();
	TestTrue(TEXT("e il RESUME glielo ridà"), Hud->GetIsEnabled());

	RTMatchHudTestsLocal::ReleaseNavigator(GI);
	return true;
}

/**
 * Tornando al menu il HUD se ne va, e l'istanza NON si riusa.
 *
 * ⚠️ L'azzeramento del puntatore e' la lezione di PR #1264: i widget appartengono al mondo in cui sono
 * stati costruiti, e riusarne uno dopo un cambio di livello chiama `AddToViewport` su un mondo smontato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchHudLeavesWithTheMatchTest,
	"RefactorTactics.Frontend.MatchHudLeavesWithTheMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchHudLeavesWithTheMatchTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTMatchHudTestsLocal::MakeStartedNavigator(GI);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav)) { RTMatchHudTestsLocal::ReleaseNavigator(GI); return false; }

	Nav->EnterMatch();
	UUserWidget* First = Nav->GetMatchHudWidget();
	if (!TestNotNull(TEXT("il HUD c'e'"), First)) { RTMatchHudTestsLocal::ReleaseNavigator(GI); return false; }

	Nav->InitializeFrontend(RTScreenIds::Main);
	TestNull(TEXT("tornando al menu il HUD se ne va"), Nav->GetMatchHudWidget());

	Nav->EnterMatch();
	TestNotNull(TEXT("la partita dopo ne ha uno"), Nav->GetMatchHudWidget());
	TestNotEqual(TEXT("e non e' l'istanza del mondo smontato"), Nav->GetMatchHudWidget(), First);

	RTMatchHudTestsLocal::ReleaseNavigator(GI);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 1.2 — Esegui i test e verifica che NON compilino** — ⏭️ *passo TDD storico: il rosso atteso non è verificabile a posteriori*

```bash
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics.Frontend+Quit" \
  -unattended -nopause -nullrhi -NoSound
```

Atteso: **errore di compilazione** — `GetMatchHudWidget`, `PresentMatchHud` e
`SetMatchHudWidgetClassForTest` non esistono. È il rosso giusto: un test che compilasse proverebbe che il
layer c'è già.

- [x] **Step 1.3 — Dichiara il layer nell'header**

In `RTFrontendNavigator.h`, nella sezione pubblica, dopo `ResumeMatch()`:

```cpp
	/**
	 * Il HUD tattico della partita: presente per tutto il match, **sotto** le schermate.
	 *
	 * ⛔ **Non e' una schermata, e non puo' esserlo.** `SyncPresentation` smonta ogni widget di
	 * `LiveWidgets` che non sia la cima dello stack o un modale: un HUD registrato li' sparirebbe
	 * all'apertura della pausa. Vive quindi in un campo suo, con `ZOrder` negativo — sotto tutto.
	 *
	 * ⚠️ Resta dentro questo subsystem, e non in una classe nuova, perche' l'invariante «un solo posto
	 * chiama `CreateWidget`» e' il criterio con cui il frontend si verifica con un `grep`.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Frontend")
	void PresentMatchHud();

	/** Smonta il HUD e **azzera** il puntatore: l'istanza appartiene al mondo che se ne va (PR #1264). */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Frontend")
	void DismissMatchHud();

	/** Il widget del HUD, o `nullptr` fuori dalla partita. Accessor C++, come `FindLiveWidget`. */
	UUserWidget* GetMatchHudWidget() const { return MatchHudWidget; }

	/** Inietta la classe del HUD senza `.ini`: e' il modo in cui i test la dichiarano. */
	void SetMatchHudWidgetClassForTest(TSoftClassPtr<UUserWidget> InClass) { MatchHudWidgetClass = InClass; }
```

E nella sezione privata, accanto a `LiveWidgets`:

```cpp
	/**
	 * La classe del HUD di partita, dichiarata in `DefaultGame.ini`.
	 *
	 * ⚠️ `TSoftClassPtr` per la stessa ragione di `FRTScreenBinding::WidgetClass`: dichiararlo non deve
	 * caricare il `.uasset`, cosi' i test headless girano prima che il Blueprint esista.
	 */
	UPROPERTY(Config)
	TSoftClassPtr<UUserWidget> MatchHudWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> MatchHudWidget;
```

- [x] **Step 1.4 — Implementa il layer**

In `RTFrontendNavigator.cpp`, accanto alle costanti di riga 22-23:

```cpp
	// Sotto le schermate, e di molto: il HUD e' cio' che sta **dietro** a tutto il frontend, e un margine
	// stretto renderebbe l'ordine una coincidenza invece di una dichiarazione.
	constexpr int32 MatchHudZ = -100;
```

Poi le due funzioni:

```cpp
void URTFrontendNavigator::PresentMatchHud()
{
	// Un binding assente e' normale — un test headless non ha `.uasset` — e non e' un errore da loggare.
	if (MatchHudWidgetClass.IsNull())
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	if (!MatchHudWidget)
	{
		UClass* WidgetClass = MatchHudWidgetClass.LoadSynchronous();
		if (!WidgetClass)
		{
			// Un percorso **dichiarato** che non carica e' quasi sempre un nome sbagliato nel `.ini`, e
			// senza questa riga il sintomo sarebbe uno schermo di partita senza HUD, senza spiegazioni.
			UE_LOG(LogRT, Warning,
				TEXT("HUD di partita: la classe '%s' non si carica — controlla MatchHudWidgetClass in ")
				TEXT("[/Script/RefactorTactics.RTFrontendNavigator] di DefaultGame.ini"),
				*MatchHudWidgetClass.ToString());
			return;
		}

		MatchHudWidget = CreateWidget<UUserWidget>(GI, WidgetClass);
		if (!MatchHudWidget)
		{
			return;
		}
	}

	if (!MatchHudWidget->IsInViewport())
	{
		MatchHudWidget->AddToViewport(MatchHudZ);
	}
}

void URTFrontendNavigator::DismissMatchHud()
{
	if (MatchHudWidget)
	{
		if (MatchHudWidget->IsInViewport())
		{
			MatchHudWidget->RemoveFromParent();
		}

		// 🔴 Il puntatore si azzera, e non e' un'ottimizzazione mancata: l'istanza appartiene al mondo in
		// cui e' stata costruita. Riusarla dopo un cambio di livello e' il difetto di PR #1264.
		MatchHudWidget = nullptr;
	}
}
```

- [x] **Step 1.5 — Aggancia il layer al ciclo di vita della partita**

Tre punti, e nessuno è opzionale.

In `InitializeFrontend`, subito dopo `DismissAllWidgets();` (riga 56):

```cpp
	// Il HUD segue la sessione come i widget delle schermate: una radice nuova non eredita il HUD della
	// precedente. `EnterMatch` lo ripresenta subito dopo — l'ordine smonta-poi-monta e' lo stesso.
	DismissMatchHud();
```

`EnterMatch` diventa:

```cpp
ERTNavResult URTFrontendNavigator::EnterMatch()
{
	// E' `InitializeFrontend` con un'altra radice, e non una terza via: la partita e' l'inizio di una
	// sessione di flow come lo e' il menu — widget della precedente buttati, stack nuovo, radice legale.
	const ERTNavResult Result = InitializeFrontend(RTScreenIds::Match);
	if (Result == ERTNavResult::Ok)
	{
		// Il HUD arriva **dopo** la radice: `InitializeFrontend` ha appena smontato tutto, questo e'
		// l'unico ordine in cui il HUD nuovo non finisce nello smontaggio della sessione vecchia.
		PresentMatchHud();
	}
	return Result;
}
```

In `SyncPresentation`, in fondo, dopo il ciclo che disabilita i `LiveWidgets`:

```cpp
	// Il HUD non e' in `LiveWidgets`, quindi il ciclo sopra non lo vede — ma la regola vale anche per lui:
	// visibile e inerte quando qualcosa lo copre.
	//
	// ⚠️ E' **meta'** della precedenza `Modal/Reaction > HUD > world`, non tutta: qui si decide se il HUD
	// riceve input, non se un click che lo attraversa arriva al mondo. Quella meta' e' di CP 11.8 (#705).
	if (MatchHudWidget && MatchHudWidget->IsInViewport())
	{
		MatchHudWidget->SetIsEnabled(
			!Stack.IsModalOpen() && Stack.CurrentScreen() == RTScreenIds::Match);
	}
```

- [ ] **Step 1.6 — Esegui i test e verifica che passino** — ⚠️ *i quattro test esistono e sono in `main`; l'esito non è stato rimisurato*

```bash
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics.Frontend+Quit" \
  -unattended -nopause -nullrhi -NoSound
grep -E "Test Completed|Success|Fail" Saved/Logs/RefactorTactics.log | tail -20
```

Atteso: i quattro `MatchHud*` **Success**, e nessuna regressione negli altri `Frontend.*`.

- [ ] **Step 1.7 — Verifica di mutazione, una alla volta** — ⏭️ *passo TDD storico: la mutazione non lascia traccia nel repository*

Un test verde non prova di poter fallire. Rompi **una** cosa, ricompila, e verifica che il rosso arrivi:

| Mutazione | Test che deve diventare rosso |
|---|---|
| togli `DismissMatchHud()` da `InitializeFrontend` | `MatchHudLeavesWithTheMatch` |
| in `SyncPresentation`, `SetIsEnabled(true)` fisso | `MatchHudIsInertUnderThePause` |
| registra il HUD in `LiveWidgets` invece che nel campo | `MatchHudSurvivesThePause` |

⚠️ Ripristina il sorgente **e ricompila** fra una mutazione e l'altra: senza rebuild la seconda misura
il binario della prima.

- [x] **Step 1.8 — Documenta il binding in `DefaultGame.ini`, ma NON scriverlo**

> 🔁 **Corretto in corso d'opera.** La prima stesura di questo step scriveva la riga subito. Il `.ini`
> stesso dichiara la disciplina contraria, tre righe sopra: la voce `Pause` di CP 46.6 **non** c'è, e la sua
> assenza è deliberata perché il `done_when` della seduta d'editor `U30` chiede l'asset **con la sua voce**.
> Scrivere il percorso prima dell'asset produce solo una warning a ogni PIE fino al Task 2.

Nella sezione `[/Script/RefactorTactics.RTFrontendNavigator]`, dopo il blocco su `Pause`, aggiungi il
**commento** — la riga resta commentata:

```ini
; ⛔ **Anche `MatchHudWidgetClass` di CP 11.7 (#613) NON sta ancora qui, e per la stessa disciplina.**
; ⚠️ Non essendo un `+Screens=`, `EveryConfiguredScreenLoads` non lo vedrebbe: quel test itera
; `GetRegisteredScreenIds()`. Un refuso nel percorso non farebbe fallire nulla — ragione in piu' per
; scrivere la riga **insieme** all'asset.
;   MatchHudWidgetClass="/Game/RT/UI/Match/WBP_RT_TacticalHUD.WBP_RT_TacticalHUD_C"
```

⚠️ **La riga si scommenta nel Task 2**, quando `WBP_RT_TacticalHUD.uasset` esiste.

- [x] **Step 1.9 — Commit**

```bash
git add Source/RefactorTactics/Frontend/RTFrontendNavigator.h \
        Source/RefactorTactics/Frontend/RTFrontendNavigator.cpp \
        Source/RefactorTactics/Tests/RTFrontendMatchHudTests.cpp \
        Config/DefaultGame.ini
git commit -m "feat(hud): il HUD di partita e' un layer, non una schermata"
```

---

## Task 2 — `WBP_RT_TacticalHUD`: il contenitore, e il primo pixel a schermo

**Files:**
- Crea: `Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset` *(Editor)*

**Interfacce:**
- Consuma: `URTTacticalHUDWidget` (parent), `MatchHudWidgetClass` dal Task 1
- Produce: il contenitore in cui i Task 3–7 innestano i figli, e la proprietà `IconCatalog`

> Questo task viene per primo fra quelli d'Editor perché è il momento in cui **si vede qualcosa**: se il
> Task 1 è agganciato bene, un contenitore anche vuoto compare in PIE. Sbagliare il binding e accorgersene
> dopo sei Blueprint costa molto di più.

- [x] **Step 2.1 — Crea la cartella e il Blueprint**

Nel Content Browser: `Content/RT/UI/` → click destro → **New Folder** → `Match`.

Dentro: click destro → **User Interface → Widget Blueprint** → **scegli la classe padre**
`RTTacticalHUDWidget` (non `UserWidget`). Nome esatto: `WBP_RT_TacticalHUD`.

⚠️ Se lo crei col padre di default: `Class Settings → Parent Class → RTTacticalHUDWidget`.

- [x] **Step 2.2 — Costruisci le quattro zone, col centro libero**

Nel Designer, radice `Canvas Panel` (**non** una `Vertical Box`: quella non lascia un centro davvero
libero). Dentro, quattro `Named Slot` o quattro `Size Box` ancorati ai bordi:

```text
┌─────────────────────────────────────────┐
│  Top     anchor 0,0→1,0   h ≈ 96 px     │  ← TurnHeader
├──────────┬───────────────────┬──────────┤
│ Left     │                   │ Right    │
│ anchor   │   ← CENTRO        │ anchor   │
│ 0,0→0,1  │     LIBERO →      │ 1,0→1,1  │  ← Roster / spazio futuro
│ w ≈ 280  │                   │ w ≈ 280  │
├──────────┴───────────────────┴──────────┤
│  Bottom  anchor 0,1→1,1   h ≈ 200 px    │  ← SelectedUnitPanel + ActionDock
└─────────────────────────────────────────┘
```

🔴 **Il centro non è un gusto.** `ARTHUD::DrawHUD` continua a disegnarci sopra path, waypoint, AoE, fuoco
amico e le barre ancorate alle unità: un pannello al centro glieli coprirebbe, ed è il primo difetto che
un playtest segnalerebbe.

- [ ] **Step 2.3 — Lascia `IconCatalog` vuoto, di proposito** — 🔺 *superata dai fatti: vedi la riconciliazione in testa*

In `Class Defaults` la proprietà `Icon Catalog` esiste ed è vuota. **Lasciala così**: il `.uasset` del
catalogo è di #220 e non esiste ancora. `ResolveIcon` restituirà il missing-icon con una warning che nomina
la chiave — a schermo si vede cosa manca, invece di un buco silenzioso.

⛔ **Non aggirare aggiungendo una variabile `Texture2D`.** È esattamente la scorciatoia che il catalogo
esiste per impedire (D-031), e il gate che la vieta non guarda dentro i Blueprint.

- [x] **Step 2.4 — Scommenta il binding in `DefaultGame.ini`**

Ora l'asset esiste, quindi la riga può essere dichiarata. Nella sezione
`[/Script/RefactorTactics.RTFrontendNavigator]`, togli il `;` davanti a:

```ini
MatchHudWidgetClass="/Game/RT/UI/Match/WBP_RT_TacticalHUD.WBP_RT_TacticalHUD_C"
```

⚠️ **Il percorso deve corrispondere esattamente** a dove hai salvato il Blueprint, suffisso `_C` compreso.
Nessun test lo verifica — `EveryConfiguredScreenLoads` itera solo le voci `+Screens=`, e questa non lo è.
L'unico segnale di un refuso è la warning di `PresentMatchHud` nel log, che nomina il percorso.

- [ ] **Step 2.5 — Verifica a schermo** — ⛔ *verifica a schermo: richiede l'Editor*

Metti un `Border` colorato temporaneo in ciascuna delle quattro zone, poi apri `L_HexArena` e premi
**PLAY**.

Atteso: i quattro bordi colorati ai margini, il centro libero, e la mappa visibile sotto. Se non compare
niente, guarda `Saved/Logs/RefactorTactics.log`: la warning del Task 1 nomina il percorso da correggere.

Poi **togli i bordi temporanei**.

- [x] **Step 2.6 — Commit**

```bash
git add Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset
git commit -m "feat(hud): il contenitore dello Screen HUD, con il centro libero"
```

---

## Task 3 — `WBP_RT_TurnHeader`: round, fase, timer

**Files:**
- Crea: `Content/RT/UI/Match/WBP_RT_TurnHeader.uasset` *(Editor)*
- Modifica: `Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset` *(innesto nella zona Top)*

**Interfacce:**
- Consuma: `URTTurnHeaderWidget::GetRoundCounterText() → FText`, `GetHeader() → FRTMatchHeaderView`,
  `HasMatchContext() → bool`
- Produce: il pattern di binding che i Task 4–7 ripetono

> È il più semplice dei cinque, e per questo viene per primo: valida il pattern di binding di proprietà su
> un widget con tre campi, prima di applicarlo a una lista.

- [x] **Step 3.1 — Crea il Blueprint**

**Widget Blueprint** con parent `RTTurnHeaderWidget`. Nome: `WBP_RT_TurnHeader`.

- [x] **Step 3.2 — Layout**

`Horizontal Box` con tre `Text Block`: `RoundText`, `PhaseText`, `TimerText`.

- [x] **Step 3.3 — Il contatore di round: usa la funzione, non comporre il testo**

Su `RoundText` → pannello **Details** → `Content → Text` → **Bind → GetRoundCounterText**.

🔴 **Collega direttamente quella funzione.** Non comporre il testo nel Blueprint da `Round` e `RoundLimit`:
sono tre stati diversi e due si confondono.

| Stato | Testo | Perché |
|---|---|---|
| nessun contesto | `—` | un widget costruito prima del `TurnManager`; `Round 0` sembrerebbe un dato |
| formato con limite | `Round 3/12` | il limite viene dal **formato**, mai da una costante |
| formato senza limite | `Round 3` | `RoundLimit == 0` = «nessun limite», **non** «su zero» |

Un binding ingenuo stampa `Round 3/0`, che si legge come una partita già scaduta.
`GetRoundCounterText()` li decide già tutti e tre.

- [ ] **Step 3.4 — Fase e timer** — ⚠️ *guardia sul negativo **fatta**; resta il binding `Visibility` — vedi la riconciliazione in testa*

Su `PhaseText` → `Text` → **Bind** → funzione nuova `GetPhaseText`:
`GetHeader()` → `break FRTMatchHeaderView` → `Phase` → nodo di conversione dell'enum → `ToText`.

Su `TimerText` → `Text` → **Bind** → funzione nuova `GetTimerText`:
`GetHeader()` → `PlanningSecondsRemaining`. ⚠️ Il valore è **negativo** quando non c'è un timer attivo:
`Select` su `< 0` → `—`, altrimenti i secondi formattati.

Su `TimerText` → `Visibility` → **Bind** → `HasMatchContext` (`Visible` / `Collapsed`).

- [x] **Step 3.5 — Innesta nel contenitore**

Apri `WBP_RT_TacticalHUD` e trascina `WBP_RT_TurnHeader` nella zona **Top**.

- [ ] **Step 3.6 — Verifica in PIE** — ⛔ *verifica a schermo* · ✅ *il limite dal formato è coperto da `RoundLimitComesFromTheFormat`*

**PLAY** su `L_HexArena`. Atteso: `Round 1/12` (o il limite del formato caricato), la fase corrente che
cambia avanzando il turno, e il timer che scorre in Planning.

🔴 **La prova che il limite non è cablato**: apri il data asset del formato, cambia `RoundLimit` da `12` a
un altro valore, e rientra in PIE. Il numero a schermo **deve** cambiare. Se resta `12`, qualcuno ha scritto
la costante nel widget — è esattamente il difetto che il DoD di #77 esiste per impedire.

- [x] **Step 3.7 — Commit**

```bash
git add Content/RT/UI/Match/WBP_RT_TurnHeader.uasset Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset
git commit -m "feat(hud): l'intestazione di turno legge il limite dal formato"
```

---

## Task 4 — `WBP_RT_TeamRoster`: la propria squadra, morti compresi

**Files:**
- Crea: `Content/RT/UI/Match/WBP_RT_TeamRoster.uasset` *(Editor)*
- Modifica: `Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset` *(zona Left)*

**Interfacce:**
- Consuma: `URTTeamRosterWidget::GetRoster() → TArray<FRTUnitCardView>`
- `FRTUnitCardView`: `HeroId (FName)` · `Health/MaxHealth (int32)` · `Shield (int32)` ·
  `Energy/MaxEnergy (int32)` · `bIsAlly (bool)` · `bAlive (bool)`

- [x] **Step 4.1 — Crea il Blueprint**

**Widget Blueprint** con parent `RTTeamRosterWidget`. Nome: `WBP_RT_TeamRoster`.

- [x] **Step 4.2 — Layout**

`Vertical Box` chiamata `RosterBox`. Ogni riga: nome dell'eroe, `Progress Bar` per gli HP, una seconda per
l'energia, e uno `Shield` mostrato solo quando `> 0`.

- [x] **Step 4.3 — Popola dal roster**

In `Event Graph`, su **Event Construct**: `GetRoster()` → `ForEachLoop` → costruisci una riga per elemento
e aggiungila a `RosterBox`.

Per gli HP: `Percent` = `Health / MaxHealth`. ⚠️ **Proteggi la divisione**: `MaxHealth == 0` su una vista
neutra darebbe `NaN`, che in UMG si vede come una barra piena.

Per i morti (`bAlive == false`): opacità ridotta. Il roster li **mostra** — è la vista aggregata della
squadra, e un'unità che sparisce dalla lista si legge come un bug.

- [x] **Step 4.4 — Non aggiungere gli avversari**

⛔ `GetRoster()` non ha un parametro «mostra anche i nemici», ed è voluto: la regola di §4.1 è una proprietà
della firma, non una disciplina. Se serve una vista avversaria, **non** si aggira con
`Get All Actors Of Class` — la privacy è verificata in `FilterForTeam`, e un secondo filtro nel widget
sarebbe una seconda verità da tenere allineata.

- [ ] **Step 4.5 — Innesta e verifica** — ⛔ *verifica a schermo* · ✅ *la parte funzionale è coperta da `RosterShowsOnlyOwnTeamAndKeepsTheFallen`*

Trascina nella zona **Left** di `WBP_RT_TacticalHUD`. **PLAY**.

Atteso: due unità della propria squadra con barre coerenti con quelle ancorate che `ARTHUD` disegna sopra
di loro; **nessuna** unità avversaria; un'unità abbattuta resta in lista, attenuata.

- [x] **Step 4.6 — Commit**

```bash
git add Content/RT/UI/Match/WBP_RT_TeamRoster.uasset Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset
git commit -m "feat(hud): il roster di squadra, senza una riga sugli avversari"
```

---

## Task 5 — `WBP_RT_SelectedUnitPanel`: chi sto comandando, e cosa ha già scelto

**Files:**
- Crea: `Content/RT/UI/Match/WBP_RT_SelectedUnitPanel.uasset` *(Editor)*
- Modifica: `Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset` *(zona Bottom, a sinistra)*

**Interfacce:**
- Consuma: `URTSelectedUnitPanelWidget::HasSelection() → bool`, `GetCard() → FRTUnitCardView`,
  `GetSlots() → FRTUnitSlotsView`
- `FRTUnitSlotsView`: `Movement` · `Main` · `Reaction`, ciascuno un `FRTPlannedSlotView` con
  `bOccupied (bool)` · `ActionId (FName)` · `DisplayName (FText)`

> 🔴 **Questo task chiude l'anello che manca.** `URTHudViewModel::BuildUnitSlots` esiste ed è verde da
> settimane; `RTScreenHudWidgets` lo trasporta. Nessuno lo **disegna** — ed è la sola ragione per cui il
> DoD di #77 non è 6 su 6.

- [x] **Step 5.1 — Crea il Blueprint**

**Widget Blueprint** con parent `RTSelectedUnitPanelWidget`. Nome: `WBP_RT_SelectedUnitPanel`.

- [x] **Step 5.2 — Il pannello si nasconde quando non c'è selezione**

Sulla radice → `Visibility` → **Bind** → `HasSelection` (`Visible` / `Collapsed`).

Un pannello vuoto a schermo si legge come un'unità senza dati, che è peggio di nessun pannello.

- [x] **Step 5.3 — La carta**

Nome dell'eroe da `GetCard() → HeroId`. Due `Progress Bar`:

- HP → `Percent` = `Health / MaxHealth`
- energia → `Percent` = `Energy / MaxEnergy`

⚠️ **Proteggi entrambe le divisioni.** `MaxHealth == 0` o `MaxEnergy == 0` su una vista neutra dà `NaN`,
che in UMG si vede come una **barra piena** — cioè un'unità a piena vita quando non c'è nessun dato.
Un `Select` su `Max > 0` che altrimenti restituisce `0.0` basta.

Lo scudo (`Shield`) si mostra solo quando è `> 0`: uno scudo a zero disegnato come barra vuota si legge
come uno scudo rotto invece che come uno scudo assente.

- [x] **Step 5.4 — I tre slot**

`Horizontal Box` con tre celle: **MOVEMENT**, **MAIN**, **REACTION**. Per ciascuna,
`GetSlots()` → `break FRTUnitSlotsView` → il campo corrispondente → `break FRTPlannedSlotView`:

- `bOccupied == false` → cella spenta, testo `—`
- `bOccupied == true` → cella accesa, testo `DisplayName`

⚠️ **Non sono tre booleani indipendenti.** `Action.Sprint` dichiara `MovementAndMain` e ne occupa **due**.
Chi lo decide è il catalogo, non il widget: leggi i tre campi e disegnali, non dedurne uno dagli altri.

- [ ] **Step 5.5 — Innesta e verifica** — ⛔ *verifica a schermo* · 🔺 *la prova con `Sprint` non è più eseguibile (D-028); l'indipendenza dei tre slot è coperta da `SlotsAreReadNotDeduced`*

Zona **Bottom**, a sinistra. **PLAY**, seleziona un'unità e pianifica.

Atteso: senza selezione il pannello è invisibile; selezionando compare la carta; pianificando un movimento
si accende **MOVEMENT**; pianificando uno `Sprint` si accendono **MOVEMENT e MAIN insieme** — è la prova
che il widget legge i tre campi invece di dedurli.

- [x] **Step 5.6 — Commit**

```bash
git add Content/RT/UI/Match/WBP_RT_SelectedUnitPanel.uasset Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset
git commit -m "feat(hud): gli slot occupati arrivano a schermo — l'anello letto esiste"
```

---

## Task 6 — `WBP_RT_ActionSlot`: una azione, e nessuna texture

**Files:**
- Crea: `Content/RT/UI/Match/WBP_RT_ActionSlot.uasset` *(Editor)*

**Interfacce:**
- Consuma: `URTActionSlotWidget::Action (FRTAbilityCooldownView)` · `bArmed (bool)` ·
  `GetIconId() → FName` · evento `OnActionChanged`
- `FRTAbilityCooldownView`: `ActionId (FName)` · `DisplayName (FText)` · `AbilityIndex (int32)` ·
  `Slot (ERTActionSlot)` · `TurnsRemaining (int32)` · `bUsableNow (bool)`
- Produce: lo slot che il Task 7 istanzia

⚠️ **Questa classe non estende la base di contesto**: riceve i dati da `SetAction`, non va a prenderli. Un
dock con sei slot che leggono ciascuno il proprio stato farebbe sei letture per frame e potrebbe mostrarne
una disallineata dalle altre.

- [x] **Step 6.1 — Crea il Blueprint**

**Widget Blueprint** con parent `RTActionSlotWidget`. Nome: `WBP_RT_ActionSlot`.

- [x] **Step 6.2 — Layout**

`Overlay` con: un `Image` (`IconImage`), un `Text Block` per il cooldown (`CooldownText`), e un `Border`
per lo stato armato (`ArmedBorder`).

- [x] **Step 6.3 — Implementa `OnActionChanged`**

In `Event Graph`, click destro → **Event On Action Changed** (è
`BlueprintImplementableEvent`, il dock la chiama). Dentro:

- `CooldownText` → `Text` = `TurnsRemaining`, e `Visibility` = `Visible` solo se `TurnsRemaining > 0`
- opacità piena se `bUsableNow`, ridotta altrimenti
- `ArmedBorder` visibile solo se `bArmed`

- [x] **Step 6.4 — 🔴 L'icona passa dal catalogo, e da nessun'altra parte**

`GetIconId()` restituisce un `FName` — `UI.Icon.Action.Move` — non un asset. Risolvilo con la libreria del
catalogo (`URTIconLibrary`), usando `IconCatalog` del `WBP_RT_TacticalHUD` padre.

⛔ **Non aggiungere una variabile `Texture2D` a questo Blueprint** per «comodità». È la scorciatoia che il
catalogo esiste per impedire (D-031): il giorno in cui `Status.Wet` cambia disegno diventa un refactor di
ogni widget invece di una riga di dato.

⚠️ `RefactorTactics.ScreenHud.WidgetApiExposesNoTexture` pinna la superficie **C++** via reflection.
**Non vede i Blueprint**: questa metà è tua, e nessun gate la controlla.

Finché il catalogo di #220 non esiste, `ResolveIcon` restituisce il missing-icon con `bResolved = false` e
una warning che nomina la chiave. **A schermo si vede che manca** — è il comportamento voluto.

- [x] **Step 6.5 — Commit**

```bash
git add Content/RT/UI/Match/WBP_RT_ActionSlot.uasset
git commit -m "feat(hud): lo slot azione risolve l'icona per chiave, non per asset"
```

---

## Task 7 — `WBP_RT_ActionDock`: il kit, e lo stato neutro

**Files:**
- Crea: `Content/RT/UI/Match/WBP_RT_ActionDock.uasset` *(Editor)*
- Modifica: `Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset` *(zona Bottom, a destra)*

**Interfacce:**
- Consuma: `URTActionDockWidget::GetActions() → TArray<FRTAbilityCooldownView>`,
  `GetArmedActionIndex() → int32`; `URTActionSlotWidget::SetAction(view, bArmed)` dal Task 6

- [x] **Step 7.1 — Crea il Blueprint**

**Widget Blueprint** con parent `RTActionDockWidget`. Nome: `WBP_RT_ActionDock`.

- [x] **Step 7.2 — Layout**

`Horizontal Box` chiamata `SlotBox`.

- [x] **Step 7.3 — Popola gli slot, e chiama `SetAction` tu**

Su **Event Construct** e a ogni cambio di selezione:

`GetActions()` → `ForEachLoop`:
- crea un `WBP_RT_ActionSlot`
- chiama `SetAction(Element, Index == GetArmedActionIndex())`
- aggiungilo a `SlotBox`

⚠️ **L'ordine è quello del kit**, e l'indice è quello che l'hotkey arma: non riordinare per cooldown o per
disponibilità. Un dock che si riordina da solo cambia il significato dei tasti a metà partita.

- [x] **Step 7.4 — 🔴 Lo stato neutro deve essere mostrabile**

`GetArmedActionIndex()` restituisce `INDEX_NONE` (`-1`) quando **niente è armato**. Non è un caso limite: è
lo stato neutro di **D-128**, quello in cui un click su un nemico *ispeziona* invece di bersagliarlo.

Con `INDEX_NONE` **nessuno slot** deve risultare acceso. Un dock che mostra sempre uno slot armato dice al
giocatore che c'è un'azione pronta a partire quando non c'è, ed è il difetto che D-128 ha deciso di
escludere.

⚠️ Attenzione al confronto: `Index == -1` non è mai vero per un indice di lista, quindi la formula
`Index == GetArmedActionIndex()` gestisce già il caso — **non** aggiungere un `Select` che forza `0`.

- [ ] **Step 7.5 — Innesta e verifica** — ⛔ *verifica a schermo: richiede l'Editor*

Zona **Bottom**, a destra. **PLAY**, seleziona un'unità.

Atteso: uno slot per azione del kit, nell'ordine del kit; **nessuno acceso** all'inizio; premendo l'hotkey
di un'azione quello slot si accende e gli altri no; un'azione in ricarica mostra i turni residui in interi
e non è utilizzabile.

- [x] **Step 7.6 — Commit**

```bash
git add Content/RT/UI/Match/WBP_RT_ActionDock.uasset Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset
git commit -m "feat(hud): il dock delle azioni sa mostrare lo stato neutro"
```

---

## Task 7-bis — La duplicazione fra §4.1 e §4.2

> 🆕 **Emersa dal primo playtest del Task 2** (2026-08-26). Non era prevista, e va decisa qui perché
> nessun documento la copre.
>
> 🔺 **Cresciuta il 2026-08-27, e adesso tocca un criterio di chiusura.** Il playtest del Task 5 ha
> mostrato che `ARTHUD` disegna **anche la terna degli slot**, in basso a destra, col vocabolario giusto:
> `Movimento: libero / Principale / Reazione`. Non è più solo il round.
>
> ⚠️ **Questo falsifica una premessa di #77.** L'issue dichiara che *«oggi la reazione compare solo nella
> riga di intento (`:316-318`), non come terna»* e ne fa **l'unica voce non soddisfatta** del suo DoD. Non è
> più vero: la terna esiste, in Canvas, e non l'ha portata `WBP_RT_SelectedUnitPanel`. Il DoD di #77 va
> riletto — o è già soddisfatto dal §4.2, o va dichiarato che a soddisfarlo dev'essere il §4.1.

### L'interruttore che rende la decisione possibile

`rt.HUD.CanvasPanels 0` (dal 2026-08-27, `RTHUD.cpp`) spegne i **quattro pannelli screen-space** del
Canvas — intestazione, combat log, barra abilità, terna — lasciando intatto il §4.2 world-space e il banner
di scenario.

Non decide niente: permette di **guardare un layer per volta**. È la ragione per cui la decisione non
richiede di cancellare codice prima di averla presa — le 910 righe di `ARTHUD` restano coperte da
`RefactorTactics.HUD.*` fino al momento in cui si sceglie.

### 🔴 Il vocabolario è già deciso, e il widget deve riusarlo

`ARTHUD::ComposeSlotLines` distingue **tre** ripieghi per lo slot occupato-senza-nome, e il commento
spiega perché non è una parola sola:

| Slot | Occupato senza nome |
|---|---|
| Movimento | **`percorso`** — waypoint tracciati, il caso più comune del gioco |
| Principale | **`occupata`** |
| Reazione | **`armata`** |

⚠️ `WBP_RT_SelectedUnitPanel` deve dire **le stesse tre parole**. Un ripiego generico tipo «Move» creerebbe
due vocabolari per la stessa informazione — e la scelta di quale layer tenere diventerebbe anche una scelta
di lessico, che è esattamente ciò che rende le decisioni rimandate difficili da prendere.

`progettazione-hud.md` assegna **turno, fase e timer al §4.1** — cioè a `WBP_RT_TurnHeader`. Ma `ARTHUD`
li **disegna già** in Canvas: la riga `Round 1/12 - Pianificazione - 12s - Velocita': x1` è viva in partita
oggi. Quando il TurnHeader avrà i suoi binding, a schermo ci saranno **due contatori di round**.

⚠️ **Non è il caso delle barre.** #613 aveva previsto una sovrapposizione voluta — barra ancorata sopra
l'unità (§4.2) *e* roster (§4.1) — perché rispondono a domande diverse: «quanto è ferito **quello lì**» e
«quanto è ferito **chi comando**». L'header non ha una giustificazione simile: «Round 3/12» risponde alla
stessa domanda ovunque sia disegnato.

**Decisione del 2026-08-26**: si risolve **dopo** i sei widget, non adesso. Guardando lo schermo pieno —
header, roster, pannello unità e dock insieme — l'ingombro reale è visibile e la stessa domanda si
ripropone per altri elementi. Tagliare adesso una riga di `ARTHUD` significherebbe deciderlo al buio.

Quando ci si arriva, i candidati da confrontare sono:

- il round su `RoundLimit` — dal **2026-09-04 (#2184)** non e' piu' una riga di `DrawHUD`: la decisione vive
  in `URTHudViewModel::ComposeRoundCounter`, che **questo** widget e il Canvas chiamano entrambi, quindi qui
  non c'e' piu' una duplicazione da sciogliere — c'e' una riga di Canvas da spegnere quando UMG la copre
- la fase e il timer di planning — stessa data: sono in `ARTHUD::ComposeMatchStatusLine`, statica pura con
  test headless (`RefactorTactics.HUD.MatchStatus*`)

> ⚠️ I due riferimenti di riga precedenti (`RTHUD.cpp:403` e `:418-430`) sono stati tolti invece che
> aggiornati: puntavano a righe che #2184 ha spostato, e un numero di riga in un piano invecchia al primo
> refactoring. Chi raccoglie questo task cerchi i due nomi di funzione.
- la riga di velocità di playback (`x1 (V)`), che è **debug** e ha regole sue

⚠️ Toccare `ARTHUD` richiede di riverificare `RefactorTactics.HUD.*`, che copre quelle 910 righe.

---

## Task 8 — Chiusura: suite, `PIE-V01-HUD`, e i documenti che mentono

**Files:**
- Modifica: `docs/technical/runbooks/guida-screen-hud-umg.md`
- Modifica: `docs/roadmap/editor-sessions.yaml` *(esito di `PIE-V01-HUD`, seduta U15)*

- [x] **Step 8.1 — Suite completa, e nessuna regressione nel §4.2**

```bash
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics+Quit" \
  -unattended -nopause -nullrhi -NoSound
grep -E "Found [0-9]+ tests|Test Completed" Saved/Logs/RefactorTactics.log | tail -5
```

⚠️ Leggi la riga `Found N tests`: una suite troncata da un crash sembra verde. Verifica in particolare che
`RefactorTactics.HUD.*` e `RefactorTactics.ScreenHud.*` siano tutti Success — il §4.2 non è stato toccato,
e se qualcosa lì è rosso il layer ha effetti che non doveva avere.

Non copiare il totale da questo piano: **misuralo sul branch**.

- [ ] **Step 8.2 — Esegui `PIE-V01-HUD` e registra l'esito reale**

È la parte che richiede un occhio, e nessun test la sostituisce:

- [ ] leggibilità delle barre a risoluzione di gioco
- [ ] ingombro del §4.1: le quattro zone non mangiano la mappa
- [ ] 🔴 **il centro è libero** — path, waypoint, AoE e barre ancorate del §4.2 si vedono tutti
- [ ] coerenza visiva durante il playback della risoluzione
- [ ] il debug è **spento**: nessun overlay diagnostico nella vista giocatore

Registra l'esito in `docs/roadmap/editor-sessions.yaml`, seduta **U15**. ⚠️ L'header di quel file è
normativo: leggilo prima di scrivere.

- [x] **Step 8.3 — Correggi le tre premesse false nella guida**

`docs/technical/runbooks/guida-screen-hud-umg.md` dichiara cose che oggi sono false, e chi la legge dopo di
te ci lavorerebbe sopra:

| Dove | Dice | Va corretto in |
|---|---|---|
| preambolo | «i `.uasset` **non sono versionati**» | `.gitignore:78` li versiona sotto `Content/RT/UI/` |
| §1 | «`Content/RT/UI/` **non esiste**: va creata» | esiste; il lavoro nuovo sta in `Match/` |
| §6 | «Oggi **non c'è codice** che lo faccia» | `URTFrontendNavigator::PresentMatchHud()`, chiamato da `EnterMatch` |
| §7 | «sono 594 righe» | `RTHUD.cpp` ne ha **910** — rimisura, non copiare |
| §8 | «i gate in `feature-registry.yaml`» | ⛔ quel file **non esiste più** (D-181/D-182) |

- [x] **Step 8.4 — Commit della documentazione**

```bash
git add docs/technical/runbooks/guida-screen-hud-umg.md docs/roadmap/editor-sessions.yaml
git commit -m "docs(hud): la ricetta diceva tre cose che il repository ha smentito"
```

- [ ] **Step 8.5 — PR verso il branch padre**

⚠️ **Non verso `main` per default.** Rileva il padre con `git config branch.<current>.parent` o
`git merge-base`.

```bash
git fetch --prune origin
gh pr list --state open   # ID in volo, e collisioni su D-nnn
gh pr create --base <padre> --body-file <file>
```

- [ ] **Step 8.6 — Chiudi #613 con il consuntivo, non con una spunta**

Il DoD si consuntiva **in un commento** con le misure reali, non spuntando le caselle del body. Nel
commento:

- i sei `.uasset` con il loro percorso
- il totale della suite **misurato**, non copiato
- l'esito di `PIE-V01-HUD` con la data
- ⚠️ cosa **resta fuori**: la precedenza `HUD → mondo` completa è di #705 — qui è arrivata solo la metà
  «il HUD è inerte sotto un modale», e il click-through resta suo

- [ ] **Step 8.7 — Aggiorna i derivati**

Chiudere il lavoro include ciò che lo cita:

- [ ] epic **#25** — spunta `#613`, e correggi la riga di `#78` che la elenca aperta (**è chiusa** dal 2026-08-25)
- [ ] **#77** — l'anello *letto* ora esiste: il DoD è 6 su 6, e stavolta con il disegno. Commenta la misura
- [ ] **#705** — il delta (c) ha dove agganciarsi: i widget UMG esistono
- [ ] `docs/roadmap/roadmap-checkpoint.md` — stato di CP 11.7

---

## Cosa questo piano non fa, e va detto

- **Non chiude #705.** La precedenza completa `Modal/Reaction > HUD > world tactical hit` richiede che il
  Canvas registri hitbox e che il click-through sia coperto da test. Qui arriva solo la metà che il layer
  può dare: il HUD è inerte quando un modale lo copre.
- **Non crea il catalogo icone.** `IconCatalog` resta vuoto e le icone si vedono come missing-icon. È #220,
  e ha una tassonomia da riconciliare prima (#637, #266).
- **Non tocca `ARTHUD`.** Il §4.2 resta in Canvas, dove la spec lo vuole.
- **Non copre il combat log** (#79) né la Ghost Timeline (#172/#173): sono checkpoint loro.
