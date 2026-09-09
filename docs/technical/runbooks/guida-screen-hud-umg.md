# Guida — costruire i `WBP_RT_*` dello Screen HUD

> `CURRENT` · **Ricetta d'Editor** per [CP 11.7](../../roadmap/roadmap-v0.1.md) ·
> issue [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613) ·
> owner del *cosa mostrare*: [`progettazione-hud.md`](../systems/progettazione-hud.md) §4.1.

Questo file dice **come costruire i Blueprint**. Non dice cosa l'HUD mostra — quello è `progettazione-hud.md`
— e non dichiara regole di gioco.

Esiste perché il codice può preparare tutto tranne l'ultimo passo — un `.uasset` si costruisce nell'Editor —
e senza una ricetta scritta quell'ultimo passo si rifà a memoria ogni volta.

> 🔁 **Corretto il 2026-08-26.** Questa riga diceva che i `.uasset` **non sono versionati** (`Content/**/*.uasset`
> in `.gitignore`). È **falso per questo percorso**: `.gitignore:78` porta l'eccezione
> `!Content/RT/UI/**/*.uasset`, quindi i Blueprint che costruirai qui **entrano nel repository** come
> qualunque sorgente. Non è un dettaglio amministrativo: cambia cosa fai a fine lavoro (`git add` del
> binario, non «l'ho fatto in locale»), e ricorda che un `.uasset` **non si fonde** — si tocca da un lavoro
> solo per volta.

---

## 1. Cosa esiste già, e cosa devi fare tu

| Pezzo | Dove | Stato |
|---|---|---|
| Classi base C++ dei widget | `Source/RefactorTactics/UI/RTScreenHudWidgets.h` | ✅ |
| Viste sanitizzate (round, roster, slot, cooldown) | `URTHudViewModel` | ✅ |
| Catalogo icone (chiave → asset) | `URTIconCatalogData` + `URTIconLibrary` | 🟡 codice sì, **il `.uasset` no** ([#220](https://github.com/DegrassiAaron/refactor-tactics-main/issues/220)) |
| Il layer che lo mette a schermo | `URTFrontendNavigator::PresentMatchHud` | ✅ **dal 2026-08-26** (#613, Task 1) |
| I sei `WBP_RT_*` | `Content/RT/UI/Match/` | ⛔ **questo lavoro** |

> 🔁 **Corretto il 2026-08-26.** Qui c'era scritto «`Content/RT/UI/` **non esiste**: va creata». Esiste, e
> contiene già otto `WBP_RT_*` — ma sono la shell di frontend di E46 (`MainMenu`, `LoadingScreen`,
> `SettingsPanel`, `ModalLayer`…), in `Content/RT/UI/Framework/`. **Nessuno dei sei nomi di questo
> checkpoint.** Il tuo lavoro va in una cartella nuova, `Content/RT/UI/Match/`, che sta accanto a
> `Framework/` e non dentro.

---

## 2. I sei Blueprint, e la classe da cui derivano

Crea ogni widget con **Widget Blueprint → scegli la classe padre**, non con il padre di default
`UserWidget`. Se ne hai già creato uno sbagliato: `Class Settings → Parent Class`.

| Blueprint | Parent Class | Cosa legge |
|---|---|---|
| `WBP_RT_TacticalHUD` | `RTTacticalHUDWidget` | contenitore a schermo intero; tiene `IconCatalog` |
| `WBP_RT_TurnHeader` | `RTTurnHeaderWidget` | `GetRoundCounterText`, `GetHeader` |
| `WBP_RT_TeamRoster` | `RTTeamRosterWidget` | `GetRoster`, `GetOpposingRoster` |
| `WBP_RT_SelectedUnitPanel` | `RTSelectedUnitPanelWidget` | `HasSelection`, `GetCard`, `GetSlots` |
| `WBP_RT_ActionDock` | `RTActionDockWidget` | `GetActions`, `GetArmedActionIndex` |
| `WBP_RT_ActionSlot` | `RTActionSlotWidget` | riceve `SetAction`; implementa `OnActionChanged` |
| `WBP_RT_FastDecision` | `RTFastDecisionWidget` | `IsWindowOpen`, `GetWindow`, `GetRemainingSeconds`; chiama `ChooseOption` |

> ⚠️ **I nomi non sono suggerimenti.** `progettazione-hud.md` §45 li dichiara, e su un `.uasset` il rename
> costa più che scriverlo giusto la prima volta (redirector, riferimenti, Fix Up).

> 🔴 **`GetOpposingRoster` esiste in C++ e nessun Blueprint lo lega** — misurato il 2026-09-09 su
> `WBP_RT_TeamRoster.uasset`, dove `GetRoster` compare e `GetOpposingRoster` no. È il residuo d'editor di
> [`#2744`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2744).
>
> **Cosa lega**: una seconda lista, sotto la propria, che si **nasconde quando è vuota** — e lo è per
> costruzione in ogni sessione presidiata, quindi non serve una condizione a parte nel grafo.
>
> ⚠️ **Non fondere le due liste in una.** `FRTUnitCardView` porta `bIsAlly` e non `TeamId`: in una lista
> sola metà delle carte direbbe «alleata» a uno spettatore che non comanda nessuno. È la lista a portare
> l'identità di squadra, ed è anche il «secondo canale oltre al colore» che `D-146` chiede.

> 🔁 **La tabella diceva «i sei» e ne elencava sei — il 2026-09-09 sono sette, e uno era già mancante prima.**
> `WBP_RT_FastDecision` entra con [`#166`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166) (CP 14.6). ⚠️ **Manca ancora `WBP_RT_EventLog`**, classe padre
> `RTPlayerEventLogWidget`, che [`#2697`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2697) ha portato in C++ senza che nessuno lo aggiungesse qui:
> misurato, `Content/RT/UI/Match/` non lo contiene. Non è di questo checkpoint e resta al proprio owner —
> è annotato perché una tabella che si dichiara completa e non lo è manda a cercare nel posto sbagliato.

---

## 3. Il layout di `WBP_RT_TacticalHUD`

Quattro zone ancorate ai bordi, **centro libero**:

```text
┌─────────────────────────────────────────┐
│              Top: TurnHeader            │
├──────────┬───────────────────┬──────────┤
│  Left:   │                   │  Right:  │
│  Team    │   ← CENTRO        │  (spazio │
│  Roster  │     LIBERO →      │  futuro) │
│          │                   │          │
├──────────┴───────────────────┴──────────┤
│  Bottom: SelectedUnitPanel + ActionDock │
└─────────────────────────────────────────┘
```

🔴 **Il centro libero è un requisito, non un gusto.** Il layer §4.2 (`ARTHUD::DrawHUD`) continua a disegnare
path, waypoint, AoE, fuoco amico e le barre ancorate **sopra la mappa**: un pannello al centro glieli
coprirebbe. Si verifica a occhio in `PIE-V01-HUD`, ed è il primo difetto che un playtest segnalerebbe.

Usa un `Canvas Panel` con anchor ai bordi, non una `Vertical Box` a schermo pieno: quest'ultima non lascia un
centro davvero libero.

---

## 4. Le tre regole che il codice non può importi

Le prime due il C++ le rende difficili da violare; la terza è solo tua.

### 4.1 🔴 Nessun widget referenzia una texture

Le icone viaggiano come **chiave** (`UI.Icon.Action.Move`) e si risolvono dal catalogo — [D-031](../../decisions/RT_PDR_00_Decision_Log.md).

- `URTActionSlotWidget::GetIconId()` restituisce un `FName`. **Non aggiungere una variabile `Texture2D` al
  Blueprint** per «comodità»: è esattamente la scorciatoia che il catalogo esiste per impedire, e il giorno in
  cui `Status.Wet` cambia disegno diventa un refactor di ogni widget invece di una riga di dato.
- `RefactorTactics.ScreenHud.WidgetApiExposesNoTexture` pinna la superficie **C++** via reflection e nomina la
  proprietà colpevole. ⚠️ **Non vede i Blueprint**: quella metà è tua.

Finché il catalogo `.uasset` di #220 non esiste, `ResolveIcon` restituisce il missing-icon con
`bResolved = false` e una warning che nomina la chiave. **A schermo si vede che manca** — è il comportamento
voluto, non un difetto da nascondere con una texture cablata.

### 4.2 I widget non ricalcolano

Nessuna classe base espone `ARTTurnManager` o `ARTUnit` a un Blueprint: restituiscono solo le viste di
`URTHudViewModel`, già sanitizzate. Se ti serve un dato che non c'è, **la risposta non è `Get All Actors Of
Class`**: è aggiungere un campo alla vista, dove è testabile.

Il caso concreto che si sbaglia per primo: il roster non ha un parametro «mostra anche gli avversari», e non
va aggirato leggendo le unità dal mondo. La privacy degli intenti è verificata in `FilterForTeam`, e un
secondo filtro nel widget sarebbe una seconda verità da tenere allineata.

### 4.3 Usa i binding di proprietà, non variabili copiate

Tutte le funzioni delle basi sono `BlueprintPure`: si collegano **direttamente** a un binding di proprietà
(`Text`, `Visibility`, `Percent`). Non chiamarle in `Event Tick` per salvarne il risultato in una variabile —
quella copia è la seconda verità che si scollega al primo turno in cui qualcuno dimentica di aggiornarla.

`URTActionSlotWidget` è l'eccezione **voluta**: riceve i dati da `SetAction` e ridisegna in
`OnActionChanged`. Un dock con sei slot che leggono ciascuno il proprio stato farebbe sei letture per frame e
potrebbe mostrarne una disallineata dalle altre.

---

## 5. I tre stati del contatore di round

`WBP_RT_TurnHeader` deve mostrare **tre** cose diverse, e due si confondono:

| Stato | Testo | Perché |
|---|---|---|
| nessun contesto | `—` | un widget costruito prima del `TurnManager`; `Round 0` sembrerebbe un dato |
| formato con limite | `Round 3/12` | il limite viene dal **formato**, mai da una costante |
| formato senza limite | `Round 3` | `RoundLimit == 0` = «nessun limite», **non** «su zero» |

`GetRoundCounterText()` li decide già tutti e tre: **usa quella**, non comporre il testo nel Blueprint. Un
binding ingenuo stampa `Round 3/0`, che si legge come una partita già scaduta.

---

## 6. Agganciare l'HUD

> 🔁 **Riscritto il 2026-08-26.** Questa sezione diceva: *«`WBP_RT_TacticalHUD` va creato e aggiunto al
> viewport dal `PlayerController` o dal `GameMode`. Oggi non c'è codice che lo faccia»*. Entrambe le metà
> sono superate — il codice c'è, e **non** va nel `PlayerController`.

**Non devi scrivere codice di aggancio.** `URTFrontendNavigator::PresentMatchHud()` lo fa già, ed è chiamato
da `EnterMatch()`. Ti resta **una riga di configurazione**.

In `Config/DefaultGame.ini`, sezione `[/Script/RefactorTactics.RTFrontendNavigator]`, scommenta:

```ini
MatchHudWidgetClass="/Game/RT/UI/Match/WBP_RT_TacticalHUD.WBP_RT_TacticalHUD_C"
```

⚠️ **Nessun test verifica quel percorso.** `EveryConfiguredScreenLoads` esige che ogni riga punti a un widget
che carica davvero, ma itera `GetRegisteredScreenIds()` — cioè le sole voci `+Screens=`, e questa non lo è.
Un refuso non fa fallire nulla: l'unico segnale è la warning che `PresentMatchHud` logga nominando il
percorso. Controlla il log alla prima PIE.

### Perché non è una schermata, e perché non è nel `PlayerController`

Due porte che sembravano ovvie e sono entrambe chiuse:

- **Un binding di `RTScreenIds::Match`** — «nessun widget» è la *definizione* di quella schermata
  (`RTFrontendScreenIds.h`): dargliene uno rimetterebbe qualcosa sopra il gioco a ogni `RESUME`.
- **`SyncPresentation` smonta** ogni widget di `LiveWidgets` che non sia la cima dello stack o un modale.
  Un HUD registrato lì **sparirebbe all'apertura della pausa**.

Per questo il HUD vive in un campo suo, fuori dalla mappa, con `ZOrder -100` — sotto le schermate. E per
questo l'aggancio sta nel navigator e non nel `PlayerController`: l'invariante «un solo posto chiama
`CreateWidget`» è il criterio con cui tutto il frontend si verifica con un `grep`.

`bShowDebug` resta `false` — è `EditDefaultsOnly` sulla classe proprio per non restare acceso in una sola
schermata dimenticata.

---

## 7. Cosa NON migrare

⚠️ `ARTHUD::DrawHUD` **resta dov'è**. La spec dice testualmente che il layer §4.2 «non deve essere
realizzato come grandi widget HUD statici», e sono **910 righe** coperte da `RefactorTactics.HUD.*`
*(misurate il 2026-08-26; la riga diceva 594, che era il conteggio del 2026-08-13 — se lo citi, rimisuralo
con `wc -l` invece di copiarlo)*.

| Elemento | Layer |
|---|---|
| Barra ancorata **sopra l'unità in campo** | §4.2 — Canvas, resta in `ARTHUD` |
| **Roster** di squadra | §4.1 — UMG |
| **Selected unit panel** | §4.1 — UMG |

Le barre coesistono di proposito: quella ancorata risponde a «quanto è ferito *quello lì*», il pannello a
«quanto è ferito *chi sto comandando*».

---

## 7-bis. `WBP_RT_FastDecision` — la finestra di reazione

Sezione aggiunta il **2026-09-09** con la metà C++ di [`#166`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166) (CP 14.6, voci 2 · 3 · 4 della DoD).
Il C++ è atterrato: quello che resta qui è **solo layout**.

### Che cosa il widget legge

**I vestiti dei binding** — si collegano **direttamente** a una proprietà, senza un nodo in mezzo:

| Nodo | Rende | Si lega a |
|---|---|---|
| `Get Window Visibility` | `ESlateVisibility` | `WindowRoot` → **Visibility** |
| `Get Countdown Text` | `FText` | `CountdownText` → **Text** |
| `Get Prompt Text` | `FText` | `PromptText` → **Text** |

**Le letture** — per il grafo, non per un binding diretto:

| Nodo | Rende | Nota |
|---|---|---|
| `Is Window Open` | `bool` | **è questa** che governa visibilità e input, non il countdown |
| `Get Window` | `FRTReactionWindowView` | `Options`, `SafeResponse`, `WindowSeconds` |
| `Get Remaining Seconds` | `float` | **negativo** quando nessuna finestra attende |
| `Choose Option` | — | prende l'**indice** dell'opzione premuta |

⚠️ **I tre vestiti esistono perché senza di loro non c'è nulla da legare**: un binding di `Visibility` vuole
una funzione che renda `ESlateVisibility`, uno di `Text` una che renda `FText` — e `Is Window Open` rende
`bool` mentre `Get Remaining Seconds` rende `float`. È lo stesso motivo per cui
`URTTurnHeaderWidget::GetRoundCounterText` esiste come funzione invece che come regola scritta qui: la
formattazione messa in un grafo è formattazione che nessun test legge.

🔴 **`Get Prompt Text` NON nomina il bersaglio, e non è una semplificazione.** La DoD chiede *«countdown **e
bersaglio**»*, ma l'unico riferimento che il DTO porta è `TargetSnapshotIndex` — un indice nello spazio di
`MakeCurrentSnapshot`, che scarta i morti. Risolverlo su un roster nominerebbe **l'unità sbagliata** in ogni
partita in cui qualcuno è già caduto. Mostrare un nome richiede che il **produttore** lo metta nel DTO;
finché non c'è, non inventarlo nel Designer.

### Le quattro regole, e perché

**1. 🔴 Un bottone per elemento di `Options`, e il testo viene da `Response`.**
Non scrivere `FIRE` o `HOLD` a mano nel grafo. `FIRE:<indice>` è un **formato** con un solo produttore
(`URTReactionOpportunityLibrary::FireResponse`), e comporlo qui ne creerebbe un secondo — fuori dai test che
presidiano il primo. Un `ListView` sulle `Options` è la forma naturale: l'indice della riga **è** l'indice da
passare a `Choose Option`.

**2. 🔴 La scelta sicura si riconosce da `SafeResponse`, non dalla parola «HOLD».**
Nel `Brace` si chiama `Hold Ground`. Un bottone etichettato a mano sarebbe corretto oggi e sbagliato con la
prima finestra che non è un Overwatch — è la regressione che `SafeResponse` ha già evitato una volta nel
core. Confronta `Option.Response == SafeResponse` per dargli lo stile «tieni».

**3. ⛔ La visibilità e l'input si legano a `Is Window Open`, MAI al countdown a zero.**
I due orologi non hanno lo stesso tick: quello autorevole scorre col `Tick` dell'Actor, il widget disegna col
proprio. Un countdown arrotondato per difetto mostra `0` per almeno un frame **prima** che la finestra si
chiuda, e un giocatore che preme in quel frame vedrebbe il proprio input sparire in un prompt che dice zero
ed è ancora aperto. Il numero è cosmesi; l'apertura è il contratto.

**4. ⛔ Niente animazione di apertura.**
Chiudere una finestra **riprende** la resolution, e la ripresa può aprirne un'altra nello stesso stack —
misurato da [`#2723`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2723). Il giocatore vedrà due finestre in fila senza un frame di stacco. Il widget si
**ricostruisce** sui dati; non anima una transizione: 300 ms costerebbero il **10%** di una finestra da
3,0 s, e il countdown autorevole non li aspetta.

### Cosa NON cercare nel grafo

Non c'è un nodo che dia il `TurnManager`, l'unità o il view model della finestra: `GetReactionWindow()` è
`protected` in C++, e `RefactorTactics.ScreenHud.FastDecisionApiCarriesNoAuthority` lo pinna per tutta la
famiglia. Se ti serve un dato che non è in `Get Window`, la risposta non è aggiungere una variabile al
Blueprint — è che quel dato non deve arrivare al widget, oppure va aggiunto al DTO **con il suo produttore**.

### ✅ L'asset esiste — creato il 2026-09-09

`Content/RT/UI/Match/WBP_RT_FastDecision.uasset`, parent class `RTFastDecisionWidget`, creato via MCP dal
**clone principale** come `CLAUDE.md` §10 richiede. L'albero è lo scheletro, non l'aspetto:

```text
[0] VerticalBox   WindowRoot      ← variabile: la visibilità si lega a `Is Window Open`
  [1] TextBlock   PromptText
  [2] TextBlock   CountdownText   ← binding su `Get Remaining Seconds`
  [3] HorizontalBox OptionsBox    ← variabile: il grafo lo popola, un bottone per elemento di `Options`
```

E la riga del gate è atterrata **con** l'asset, in quattro siti di
`Source/RefactorTactics/Tests/RTMatchWidgetAssetTests.cpp`:

```cpp
const TCHAR* const FastDecisionPath =
    TEXT("/Game/RT/UI/Match/WBP_RT_FastDecision.WBP_RT_FastDecision_C");
```

⚠️ **La riga atterra CON l'asset, non prima**: quel test carica per path, e un path che non esiste è un
test rosso che aspetta un file che nessuno ha ancora creato.

⚠️ **Il nome e la cartella non sono negoziabili**: misurato, **25 widget su 25** seguono `WBP_RT_*`, e
`RTMatchWidgetAssetTests` carica da `/Game/RT/UI/Match/`. Un asset fuori convenzione non fallirebbe il
gate — **non lo incontrerebbe**.

### ⛔ Cosa NON è nell'asset, e va fatto nel Designer

Lo scheletro non è la finestra. Restano da fare, e sono lavoro d'autore:

1. **I tre binding** — tre menù a tendina, e i nodi da scegliere sono **esattamente** questi:

   | Widget | Proprietà | Funzione da collegare |
   |---|---|---|
   | `WindowRoot` | **Visibility** | `Get Window Visibility` |
   | `CountdownText` | **Text** | `Get Countdown Text` |
   | `PromptText` | **Text** | `Get Prompt Text` |

   ⛔ **Non collegare `Visibility` al countdown.** È la scorciatoia che viene in mente per prima ed è il
   difetto **F7**: i due orologi non hanno lo stesso tick, il numero tocca lo zero **prima** che la finestra
   si chiuda, e il prompt sparirebbe mentre il gioco sta ancora aspettando. La risposta mancata diventa un
   `HoldTimeout` che nel TurnLog è indistinguibile da una scelta deliberata.

2. **Il popolamento di `OptionsBox`** — vedi §7-ter, ha una ricetta sua;
3. **L'aspetto**: colori, font, ingombro, e la posizione dentro `WBP_RT_TacticalHUD`. ⚠️ Il **centro libero**
   di §3 vale anche per questa finestra: §4.2 disegna path e AoE sopra la mappa.

### ✅ Come sapere di aver collegato i nodi giusti

```bash
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics.ScreenHud.FastDecisionBindingsAreWiredToTheRightNodes;Quit" \
  -unattended -nopause -nosplash -nullrhi -NoLiveCoding -log
```

`RefactorTactics.ScreenHud.FastDecisionBindingsAreWiredToTheRightNodes` legge `Class->Bindings` e verifica
le **tre triplette** widget → proprietà → funzione. Finché non le trovi è rosso, e l'errore nomina il widget,
la proprietà, la funzione mancante e la ragione.

🔑 **La metà che conta non è «c'è un binding», è «è collegato alla funzione giusta»**: un binding presente e
sbagliato è peggio di uno assente, perché a schermo sembra funzionare.

✅ Gli altri gate headless coprono il **contratto** — parent class, caricamento, nessuna texture. Ciò che
resta dopo questi due è esattamente ciò che solo `PIE-V01-OVERWATCH` può guardare.

---

## 7-ter. `OptionsBox` — i bottoni della finestra

Sezione aggiunta il **2026-09-09**. È l'ultimo pezzo di UI delle voci 2 · 4 di CP 14.6.

### Perché serve un secondo Blueprint

⛔ **Non si può fare con un `ForEach` e basta.** In un ciclo Blueprint l'indice **non è catturabile** dentro
un delegate: `OnClicked` non porta parametri, quindi tutti i bottoni finirebbero per rispondere con lo stesso
indice — l'ultimo. Serve un widget figlio che **tenga il proprio indice**, ed è lo stesso motivo per cui
`WBP_RT_ActionSlot` esiste accanto a `WBP_RT_ActionDock`.

| Blueprint | Parent Class | Cosa fa |
|---|---|---|
| `WBP_RT_FastDecisionOption` | `RTFastDecisionOptionWidget` | un `Button` + un `Text Block`; il click chiama `Choose` |

### ✅ Il grafo — scritto il 2026-09-09

```text
Event On Window Changed
  └─ OptionsBox → Clear Children
  └─ ForEach i in range(Get Option Count)
       └─ Make Option Widget (OptionClass = WBP_RT_FastDecisionOption, OptionIndex = i)
       └─ OptionsBox → Add Child
```

E dentro **`WBP_RT_FastDecisionOption`**: `OnClicked` del bottone → **`Choose`**, e
`Event On Option Changed` → `LabelText → Set Text (Get Option Label)`. Nient'altro.

🔑 **Il grafo NON costruisce il bottone da sé, e non è una semplificazione.** La prima stesura di
questa sezione chiedeva `Create Widget` + `Set Option(Owner, Option, Index, bIsSafe)`. Sarebbe stato il
grafo a decidere tre cose che non deve toccare:

1. **il proprietario** — e con esso una seconda porta su `Choose Option`, con un indice non suo;
2. **quale opzione è la sicura** — che si decide confrontando con `SafeResponse`, non cercando `HOLD`;
3. **la risposta** — che resterebbe visibile al grafo.

`Make Option Widget` le tiene tutte e tre in C++, dove sono testate. ⚠️ La **classe** invece arriva dal
grafo, ed è voluto: un percorso di `Content/` scritto in C++ sarebbe un riferimento duro a un `.uasset`
dentro il modulo, che nessun altro widget di questo progetto ha.

### 🔴 Le tre regole che il C++ non può importi

**1. Ricostruisci SOLO su `On Window Changed`, mai su `Tick`.**
Un click Slate è **due eventi su due frame** — `MouseButtonDown` e `MouseButtonUp` — e devono atterrare sulla
**stessa istanza** di widget. Svuotare e ripopolare il box ogni frame non lo garantisce, e in una finestra da
3,0 s un click perso matura in `HoldTimeout` — indistinguibile, nel TurnLog, da una scelta deliberata.

**2. L'evento scatta anche quando la finestra si CHIUDE.** `Clear Children` va eseguito **sempre**, prima del
ciclo: se la finestra è chiusa, `Options` è vuoto e il box resta vuoto. Un `if (Is Window Open)` messo prima
del `Clear` lascerebbe a schermo i bottoni dell'ultima domanda.

**3. La scelta sicura non si riconosce dal testo.** Confronta `Response` con `SafeResponse`, come nello
pseudo-grafo qui sopra — nel `Brace` si chiama `Hold Ground`, non `HOLD`.

⚠️ **E due finestre di fila esistono davvero**, non è un caso limite teorico: misurato da
`ScreenHud.FastDecisionDetectsTheWindowChanging`, che registra nel log *«dopo A si è aperta un'altra
finestra»*. È esattamente lo scenario in cui una ricostruzione legata a `Is Window Open` — vero prima e dopo —
lascerebbe i bottoni sbagliati.

### ✅ Verifica

```bash
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics.ScreenHud;Quit" \
  -unattended -nopause -nosplash -nullrhi -NoLiveCoding -log
```

⚠️ **Il gate headless copre il C++, NON il tuo grafo.** `Class->Bindings` vede i property binding, **non** il
consumo dentro un event graph — è l'errore che `ActionDockConsumesArmedIndex` ha già commesso una volta, ed è
scritto in `RTMatchWidgetAssetTests.cpp`. Ciò che il grafo fa davvero si guarda in **`PIE-V01-OVERWATCH`**.

---

## 8. Verifica

Quando i sei Blueprint esistono e l'HUD è agganciato, esegui **`PIE-V01-HUD`**
([`test-manuali-pie.md`](../test-manuali-pie.md)) e registra l'esito. È la parte che richiede un occhio:
leggibilità delle barre, ingombro, coerenza visiva durante il playback, e il **centro libero** che nessun
test automatico può guardare.

Registra l'esito in [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml), seduta **U15** — è la
seduta che dichiara `PIE-V01-HUD` fra le sue `verifies`. ⚠️ L'header di quel file è normativo: leggilo prima
di scrivere.

Cosa **non** serve la PIE per verificarlo, e quindi non va rimandato lì:

```bash
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics.Frontend+Quit" \
  -unattended -nopause -nullrhi -NoSound
```

I quattro `RefactorTactics.Frontend.MatchHud*` provano il ciclo di vita del layer senza aprire l'Editor.

⚠️ **L'exit code di `UnrealEditor-Cmd` non dice se i test sono girati** — il processo può restare appeso
dopo il `Quit` e uscire con `127` a suite completata. Giudica dal log (`Saved/Logs/RefactorTactics.log`):
cerca `Found N tests` e i `Test Completed. Result={...}`.

> 🔁 **Corretto il 2026-08-26.** Questa sezione rimandava ai gate di `RT-FEAT-UI-SCREEN-HUD` in
> `feature-registry.yaml`. ⛔ **Quel file non esiste più**: il tooling è uscito dal repository con
> **D-181/D-182** (2026-08-21). Puntare lì mandava a cercare uno stato che nessuno scrive più.
