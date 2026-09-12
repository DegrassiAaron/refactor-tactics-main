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
| Catalogo icone (chiave → asset) | `URTIconCatalogData` + `URTIconLibrary` + `Content/RT/UI/DA_IconCatalog.uasset` | 🟡 esiste, **indietro di una chiave** ([#2551](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2551)); il consumo dai widget è [#220](https://github.com/DegrassiAaron/refactor-tactics-main/issues/220) |
| Il layer che lo mette a schermo | `URTFrontendNavigator::PresentMatchHud` | ✅ **dal 2026-08-26** (#613, Task 1) |
| I `WBP_RT_*` di partita | `Content/RT/UI/Match/` | ✅ **esistono e sono montati** — vedi §2 e §3. `WBP_RT_EventLog` ha radice, contenitore e grafo (#2784, `a7c189f9`) ed è montato dal 2026-09-10 ([#2697](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2697)) — in `Zone_MiddleRight` dal rimontaggio del 2026-09-12, `ZoneRight` prima |

> 🔁 **Corretto il 2026-09-09.** Due righe di questa tabella descrivevano come futuro ciò che è già in
> `main`, misurato su `a897de28`. **(1)** *«Catalogo icone — il `.uasset` no»*: `Content/RT/UI/DA_IconCatalog.uasset`
> esiste; ciò che resta aperto è un'altra cosa — è **indietro di una chiave** (#2551), e il difetto
> «indietro di una chiave» si diagnostica in modo opposto a «non esiste». **(2)** *«I sei `WBP_RT_*` — ⛔
> questo lavoro»*: esistono, e non sono sei. Una guida che dice «costruiscili» a chi li ha già davanti manda
> a ricrearli, ed è il modo in cui un `.uasset` acquista un duplicato.

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
> `WBP_RT_FastDecision` entra con [`#166`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166) (CP 14.6). ✅ **`WBP_RT_EventLog` ora esiste** (2026-09-09), classe padre
> `RTPlayerEventLogWidget` ([#2697](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2697)), e dal 2026-09-10 ha radice, contenitore e il grafo che
> lo popola — `GetFeed` → `ForEach` → `AddChildToVerticalBox`, come fa il roster ([#2784](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2784), `a7c189f9`).
>
> 🔴 **⌫ Questa riga ha dichiarato per un giorno un montaggio che non esisteva.** Diceva *«è montato nella
> `RIGHT` al posto del pannello unità»* dal 2026-09-09; misurato il 2026-09-10 su `origin/main` = `69451996`,
> nella tabella dei nomi di `WBP_RT_TacticalHUD` la stringa `EventLog` compariva **zero** volte, e nessun
> altro `.uasset` tracciato la nominava. La riga riportava ciò che lo strumento di authoring aveva risposto,
> **non ciò che il pacchetto conteneva dopo il salvataggio**.
>
> ✅ **Ora è vero, e lo è per misura**: `WBP_RT_EventLogRight` (classe `WBP_RT_EventLog_C`) è montato —
> nella `ZoneRight` allora, nel `Content` di `Zone_MiddleRight` dal rimontaggio del 2026-09-12 — riletto dal
> `.uasset` dopo il salvataggio e presidiato da
> `RefactorTactics.ScreenHud.TheHudMountsTheFeedThatExplainsTheTurn` — che sull'asset precedente **fallisce**,
> verificato eseguendolo su entrambi. ⛔ **Un `.uasset` si dichiara rileggendolo, non dalla risposta di chi
> lo ha scritto.**

---

## 3. Il layout di `WBP_RT_TacticalHUD`

Otto zone — la griglia 3×3 **meno il centro**, a fasce 20% / 60% / 20% su entrambi gli assi. Le otto celle
sono zone; la nona, quella di mezzo, è definita da ciò che **non** contiene.

⌫ *Fino al 2026-09-12 questa sezione descriveva cinque zone — `TOP`, `LEFT`, `RIGHT`, `BOTTOM`, `CENTER` —
ancorate ai bordi. Non è una rinomina: `MiddleLeft` e `BottomLeft` sono due celle distinte dove prima c'era
un solo `BOTTOM`, e **Selected Unit cambia fascia**.*

```text
        0.0        0.2                    0.8        1.0
   0.0  ┌──────────┬──────────────────────┬──────────┐
        │ TopLeft  │     TopCenter        │ TopRight │   20%   0 .. 216 px
   0.2  ├──────────┼──────────────────────┼──────────┤
        │ Middle   │      ⛔ CENTRO        │  Middle  │
        │ Left     │      keep-out        │  Right   │   60%   216 .. 864 px
   0.8  ├──────────┼──────────────────────┼──────────┤
        │ Bottom   │    BottomCenter      │  Bottom  │
        │ Left     │                      │  Right   │   20%   864 .. 1080 px
   1.0  └──────────┴──────────────────────┴──────────┘
             20%            60%               20%
```

🔑 **I tagli a 0.2 e 0.8 non sono una scelta estetica**: il keep-out di `PanelsLeaveTheCenterFree` è il 60%
centrato (`RTCenterFree::CenterFraction`), quindi il centro libero **è** la cella di mezzo di questa griglia.

Ogni zona è un'istanza di `WBP_RT_HudZone` figlia **diretta** del `Canvas Panel` radice, con anchor stirati
su entrambi gli assi, offset `L4 T4 R4 B4` e `Size To Content` **spento**. L'inquilino sta nel suo
`NamedSlot Content`.

| Zona | Anchors Min → Max | Contiene oggi | Owner del comportamento |
|---|---|---|---|
| `Zone_TopLeft` | (0.0, 0.0) → (0.2, 0.2) | `WBP_RT_TeamRosterLeft` | [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613) · [#2744](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2744) |
| `Zone_TopCenter` | (0.2, 0.0) → (0.8, 0.2) | `WBP_RT_TurnHeader` — round su `RoundLimit`, fase, timer, objective | [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613) · [#77](https://github.com/DegrassiAaron/refactor-tactics-main/issues/77) |
| `Zone_TopRight` | (0.8, 0.0) → (1.0, 0.2) | *vuota* — Objective, da progettare | `progettazione-hud.md` §6 |
| `Zone_MiddleLeft` | (0.0, 0.2) → (0.2, 0.8) | `WBP_RT_SelectedUnitPanelLeft` (+ `WBP_RT_UnitCard`) | [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613) · [#2760](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2760) |
| `Zone_MiddleRight` | (0.8, 0.2) → (1.0, 0.8) | `WBP_RT_EventLogRight` — istanza di `WBP_RT_EventLog_C`, dal 2026-09-10 ([#2697](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2697)) | [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613) · [#1896](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1896) |
| `Zone_BottomLeft` | (0.0, 0.8) → (0.2, 1.0) | *vuota* — dichiarata senza inquilino, ed è il posto che si guarda per decidere cosa ci va | — |
| `Zone_BottomCenter` | (0.2, 0.8) → (0.8, 1.0) | `WBP_RT_ActionDockBottom` (+ `WBP_RT_ActionSlot`) | [#220](https://github.com/DegrassiAaron/refactor-tactics-main/issues/220) · [#2760](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2760) |
| `Zone_BottomRight` | (0.8, 0.8) → (1.0, 1.0) | *vuota* — Confirm · Undo, da progettare | `progettazione-hud.md` §6 |
| *cella centrale* | (0.2, 0.2) → (0.8, 0.8) | ⛔ **NESSUN PANNELLO SCREEN-HUD STATICO** — battlefield e Tactical World Overlay §4.2 | [#2184](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2184) · `progettazione-hud.md` §3.1 |

⚠️ **`WBP_RT_FastDecision` NON è montato da nessuna parte**, e fino al 2026-09-12 questa tabella diceva che
il `BOTTOM` lo conteneva *«a runtime»*. È falso, misurato:

```
python tools/uasset/names.py Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset --filtro FastDecision
#   -> 0 nomi
grep -rn "URTFastDecisionWidget" Source/RefactorTactics/ --include=*.cpp | grep -v /Tests/
#   -> solo i metodi della classe: nessun CreateWidget, nessun AddChild
```

L'asset non lo referenzia e nessun C++ lo crea: la finestra di reazione di
[#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166) non può comparire in partita. Il
montaggio sarebbe a runtime, quindi un gate sull'albero non basterebbe — ci vuole una prova che la finestra
compaia quando la reazione si apre. Segue in
[#3047](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3047).

✅ **Due gate nuovi guardano questa griglia dal 2026-09-12**, e chiedono cose diverse:
`RefactorTactics.ScreenHud.TheEightZonesAreDeclaredExactlyOnce` — c'è una zona per ogni valore di
`ERTHudZone`, nessuna mancante e nessuna doppia — e
`RefactorTactics.ScreenHud.ZoneRectanglesMatchTheThreeByThreeGrid`, che ricalcola il rettangolo di ogni
zona con la formula di `SConstraintCanvas::OnArrangeChildren` e lo confronta con la sua cella. Il secondo
esiste perché `PanelsLeaveTheCenterFree` è un gate **negativo**: dice che nessuna zona invade il centro, e
passerebbe con tutte e otto schiacciate in un angolo.

> 🔴 **Per un giorno il diagramma e la colonna «contiene oggi» hanno descritto un albero che il `.uasset`
> non conteneva, e la ragione va ricordata perché è ripetibile.** `cc5ca967` — il commit che dichiarava
> *«la destra ospita il feed»* — ha in realtà **risalvato l'albero dei widget nello stato precedente** al fix
> di [#2760](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2760): verificato confrontando la
> tabella dei nomi del pacchetto a `cc5ca967` con quella a `bbca9a36`, **identiche nome per nome**. È il modo
> di guasto dell'Editor con una copia in memoria anteriore, e fra i due eventi la suite è rimasta verde
> perché **nessun gate guardava l'albero**.
>
> ✅ **Dal 2026-09-10 due lo guardano**, e non sono decorativi — falliscono entrambi sull'asset precedente,
> verificato eseguendoli su entrambe le versioni:
> `RefactorTactics.ScreenHud.TheHudMountsTheFeedThatExplainsTheTurn` (l'albero ospita un
> `URTPlayerEventLogWidget`) e `RefactorTactics.ScreenHud.NoNodeWearsTheNameOfAWidgetWithoutBeingOne`
> (nessun nodo col prefisso `WBP_` che non sia un `UUserWidget`).
> Referto: [`2697-il-feed-non-e-montato-spec-panel-2026-09-10.md`](../../roadmap/plans/2697-il-feed-non-e-montato-spec-panel-2026-09-10.md).
>
> ⚠️ **Chi rimonta un widget rilegga comunque il `.uasset` DOPO il salvataggio.** È il passo che mancava alle
> tre dichiarazioni sbagliate; i due test lo rendono automatico solo per l'albero della HUD.

🔑 **Il centro è una zona a contratto negativo**, e per questo non ha un `WBP_RT_CenterPanel`: si definisce
per ciò che non deve contenere. Il criterio è misurabile — *la Screen HUD non occupa permanentemente il
centro e non oscura le celle necessarie alla decisione* — e chi lo violasse lo farebbe allargando una delle
altre otto, non aggiungendo la nona.

🔑 **Per la stessa ragione il centro NON è un valore di `ERTHudZone`.** L'enum ha otto zone più la
sentinella `Count`; un `ERTHudZone::Center` sarebbe un invito a riempirlo, cioè esattamente il difetto che
`progettazione-hud.md` §3.1 vieta.

⚠️ **`Zone_TopLeft` è la squadra e `Zone_MiddleRight` il contesto locale**, non il contrario. È
l'assegnazione implementata e compilata; una lettura che le scambi è un **re-layout**, non una correzione,
e passerebbe dal giudizio di `PIE-V01-SCREENHUD` sull'ingombro.

> 🔁 **Corretto il 2026-09-09, poche ore dopo essere stato scritto sbagliato.** ⌫ *I nomi di nodo citati qui
> sotto — `Zone_Top`, `ZoneLeft`, `ZoneRight`, `ZoneBottom` — sono quelli di **allora**: il rimontaggio del
> 2026-09-12 li ha sostituiti con le otto celle della tabella qui sopra. Il riquadro resta perché è la
> ragione per cui i gate esistono, non perché descriva l'albero corrente.* Questa tabella dichiarava
> `RIGHT` **vuota**, *«e non è un difetto»*. **Falso.** Il tree è stato misurato col ponte MCP
> (`UMGToolSet.GetWidgetDescription` su `WBP_RT_TacticalHUD`) e depositato in
> [`test-manuali-pie.md`](../test-manuali-pie.md): *«le quattro zone reali e popolate — `Zone_Top` →
> `WBP_RT_TurnHeader`, `ZoneLeft` → `WBP_RT_TeamRoster`, `ZoneRight` → `WBP_RT_SelectedUnitPanel`,
> `ZoneBottom` → `WBP_RT_SelectedUnitPanel` + `WBP_RT_ActionDock`»*. Il log PIE di
> [#1896](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1896) lo conferma per un'altra via, nominando l'istanza:
> `WBP_RT_TacticalHUD_C_0.WidgetTree_0.WBP_RT_SelectedUnitPanelRight.WidgetTree_0.WBP_RT_UnitCard`.
>
> ⚠️ **L'errore non è nato qui: è stato ereditato e rafforzato.** La versione precedente diceva `Right:
> (spazio futuro)` — stantia, e mai misurata. Riscriverla come *«vuota, e non è un difetto»* ha trasformato
> una riga vecchia in un'affermazione, senza aggiungere la misura che l'avrebbe smentita.
>
> 🔴 **⌫ E qui stava la frase che ha reso possibile l'errore successivo**: *«Un `.uasset` è compresso e
> `strings` non lo legge: questo tree si verifica **solo dall'Editor**»*. È **falsa**, e il 2026-09-10 la sua
> falsità è costata tre dichiarazioni sbagliate (§2, §3 e il messaggio di `cc5ca967`). La **tabella dei nomi**
> di un pacchetto Unreal **non è compressa**: è una sequenza di `FString` con prefisso di lunghezza, e
> contiene il nome di ogni widget, classe e import dell'albero. Si legge dal blob di Git, senza aprire nulla:
>
> ```
> python tools/uasset/names.py Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset --filtro WBP_RT_ --unici
> python tools/uasset/names.py --rev cc5ca967 Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset --filtro EventLog
> #                                                                                        -> 0 nomi
> ```
>
> ⛔ **Non sostituisce l'Editor per il *layout*** — dice chi c'è nell'albero, non com'è disposto. Ma la
> domanda *«questo widget è montato?»* la risponde in un secondo, ed è la domanda su cui l'Editor è stato
> creduto tre volte a torto.

🔴 **Il centro libero è un requisito, non un gusto.** Il layer §4.2 (`ARTHUD::DrawHUD`) continua a disegnare
path, waypoint, AoE, fuoco amico e le barre ancorate **sopra la mappa**: un pannello al centro glieli
coprirebbe. Si verifica a occhio in `PIE-V01-SCREENHUD`, ed è il primo difetto che un playtest segnalerebbe.

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

Quando i Blueprint della §2 esistono e l'HUD è agganciato, esegui **`PIE-V01-SCREENHUD`**
([`test-manuali-pie.md`](../test-manuali-pie.md)) e registra l'esito. È la parte che richiede un occhio:
leggibilità delle barre, ingombro, coerenza visiva durante il playback, e il **centro libero** che nessun
test automatico può guardare.

> 🔁 **Corretto il 2026-09-09.** Questa sezione diceva `PIE-V01-HUD`, seduta **U15**. Quella voce è **chiusa
> dal 2026-08-24** e vive **sul Canvas**: dichiarava fin dall'inizio che *«lo Screen HUD §4.1 di CP 11.7 avrà
> una voce PIE propria»*, ed è `PIE-V01-SCREENHUD`. Governa
> [`test-manuali-pie.md`](../test-manuali-pie.md), che di quelle voci è l'owner — la stessa riga vi è
> registrata come prevalente sul DoD di [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613), che chiede *«`PIE-V01-HUD` estesa all'ingombro del §4.1»*.
> Eseguire U15 avrebbe rimisurato il Canvas e lasciato il §4.1 `NOT RUN`.

La seduta che la convoca è **`U49`** in [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml), aperta
il 2026-09-09 — prima non ne aveva nessuna, ed è la ragione per cui la voce esisteva dal 2026-08-28 senza
che nessuno potesse aprirla.

🔴 **Si esegue da `L_Frontend` premendo `PLAY`, non aprendo una mappa di partita.** Il layer è presentato da
`EnterMatch` e non dallo stack: chi apre `L_DevSandbox` o `L_HexArena` non vede né roster né dock **perché
nessuno li ha montati**, e dichiarerebbe rotto un HUD mai caricato. ⛔ Dal frontend **non è pilotabile via
MCP** — misurato il 2026-08-30: i bottoni del menu non si premono dal ponte. Serve una persona alla
tastiera. ⚠️ E premi `Home` prima di giudicare: la camera parte dall'origine, e la prima inquadratura
sembra un livello rotto.

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
