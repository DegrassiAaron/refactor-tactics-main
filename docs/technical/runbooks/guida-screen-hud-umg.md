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
| I `WBP_RT_*` di partita | `Content/RT/UI/Match/` | ✅ **esistono e sono montati** — vedi §2 e §3. `WBP_RT_EventLog` ha `LogRoot`, `LinesBox` e `LineClass` dalla seduta `U77`, e le righe le costruisce `RefreshLook` (prima un grafo, da #2784 `a7c189f9`); è montato dal 2026-09-10 ([#2697](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2697)) — in `Zone_MiddleRight` dal rimontaggio del 2026-09-12, `ZoneRight` prima |

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

Le zone stanno su **quattro fasce attorno alla scena**: le tre celle alte, le due di mezzo e **la fascia bassa
intera**. La scena 3D si rende **solo** nel rettangolo che le fasce lasciano libero ([D-497](../../decisions/RT_PDR_00_Decision_Log.md),
[#3661](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3661)): fuori c'è l'HUD, e un click lì non raggiunge il mondo.

🔑 **Le fasce sono un dato di runtime, in unità Slate**: `FRTHudBands` in `Source/RefactorTactics/UI/RTHudLayout.h`.
Sono la misura dei loro inquilini, e a 1920×1080 valgono le frazioni di D-497.

| Fascia | Unità Slate | A 1920×1080 | Chi la misura |
|---|---|---|---|
| sinistra (`Left`) | 384 | 20% | roster e scheda finiscono a x=380 |
| destra (`Right`) | 368,64 | 19,2% | il registro e il badge accanto a `Conferma` cominciano a x=1556 |
| alta (`Top`) | 205,2 | 19% | testata e `Conferma` |
| bassa (`Bottom`) | 183,6 | 17% | il dock comincia a y=903 |
| cella alta della colonna sinistra (`LeftTop`) | 267 | 24,7% | il roster: 259 con tre unità, 201 con due, misurato da `ScreenHud.TenantsFitTheirBands` |

⚠️ **La colonna sinistra ha il suo taglio** (decisione d'autore del 2026-10-11, [#3661](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3661)): il roster vero non
sta nella fascia alta al 19% (201 contro 197 già con le due unità del formato della v0.1), quindi prende la sua
altezza, e la scheda parte sotto. La colonna è tutta fascia: la scena non cambia.

🔑 **A un'altra risoluzione le fasce restano le stesse in unità Slate**, e la scala dell'interfaccia di UE
(`ShortestSide`) le porta in pixel: a 1280×800 la sinistra vale circa il 22,2% e la destra il 21,3%, a 2560×1080
il 15% e il 14,4%. Lo prova `ScreenHud.SceneRectComesFromTheBandsAndTheUiScale`.

⌫ *Dal 2026-09-12 al 2026-10-04 le zone erano **otto**: anche la fascia bassa aveva tre celle, `BottomLeft`,
`BottomCenter` e `BottomRight`. [D-456](../../decisions/RT_PDR_00_Decision_Log.md) le ha unite in
`Zone_Bottom`, della barra dei comandi
([#3469](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3469)): sui kit di partita la barra
non entrava nella cella centrale.*

⌫ *Fino al 2026-09-12 questa sezione descriveva cinque zone — `TOP`, `LEFT`, `RIGHT`, `BOTTOM`, `CENTER` —
ancorate ai bordi. Non è una rinomina: `MiddleLeft` e `BottomLeft` sono due celle distinte dove prima c'era
un solo `BOTTOM`, e **Selected Unit cambia fascia**.*

```text
   a 1920×1080, in unità Slate (= pixel)
        0         384                  1551.36      1920
     0  ┌──────────┬──────────────────────┬──────────┐
        │ TopLeft  │     TopCenter        │ TopRight │   fascia alta   0 .. 205,2
        │ (roster) ├──────────────────────┼──────────┤
   267  ├──────────┤                      │          │
        │ Middle   │   SCENA 3D           │  Middle  │
        │ Left     │   (l'unico posto     │  Right   │
        │ (scheda) │    in cui si rende)  │ (registro│
 896,4  ├──────────┴──────────────────────┴──────────┤
        │                 Bottom                     │   fascia bassa  896,4 .. 1080
        │      (la barra dei comandi, D-456)         │
  1080  └────────────────────────────────────────────┘
```

⌫ *Fino al 2026-10-11 la griglia era 20% / 60% / 20% su entrambi gli assi, con la 3D a tutto schermo **sotto** le
zone e il «centro libero» come riquadro da non coprire (`CenterFraction`). [D-497](../../decisions/RT_PDR_00_Decision_Log.md) ha portato la scena fra le
fasce ([#3661](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3661)).*

Ogni zona è un'istanza di `WBP_RT_HudZone` figlia **diretta** del `Canvas Panel` radice, con `Size To Content`
**spento**. L'inquilino sta nel suo `NamedSlot Content`.

🔴 **Anchor e offset delle zone li scrive il C++, all'avvio dell'HUD**: `URTTacticalHUDWidget::ApplyBandLayout`, da
`NativeConstruct`, con `RTHudLayout::ZoneLayout`. Le zone d'angolo sono ancorate al loro angolo con una misura fissa
in unità Slate, quelle di mezzo stirate sull'asse lungo, e ognuna si stacca di 4 dai bordi della sua cella. ⚠️ **Gli
anchor salvati nell'asset non contano più**: il Designer mostra ancora la griglia a frazioni finché qualcuno non
risalva l'asset, e una zona trascinata lì torna al suo posto in partita. I test misurano la posa del dato, non
l'asset (`ScreenHud.ZoneRectanglesMatchTheBands`, `ScreenHud.PanelsLeaveTheSceneFree`).

🔑 **Chi rende la scena nel rettangolo**: `URTGameViewportClient`, registrato in `Config/DefaultEngine.ini`
(`GameViewportClientClassName`). Sovrascrive `LayoutPlayers` e scrive `Origin`/`Size` del giocatore **mentre l'HUD
tattico è montato**; proiezione, deproiezione e `ProjectWorldLocationToScreen` lo seguono da sé, perché passano da
`ULocalPlayer::GetProjectionData`.

| Zona | Posto (dal dato, in unità Slate) | Contiene oggi | Owner del comportamento |
|---|---|---|---|
| `Zone_TopLeft` | angolo in alto a sinistra, `Left` × `LeftTop` | `WBP_RT_TeamRosterLeft`; la resa della tavola dalla seduta `U76` ([#3636](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3636)) | [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613) · [#2744](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2744) |
| `Zone_TopCenter` | fra le colonne, alta `Top` | `WBP_RT_TurnHeader` — round su `RoundLimit`, fase, timer, objective; la resa delle tavole A e D dalla seduta `U75` ([#3635](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3635)) | [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613) · [#77](https://github.com/DegrassiAaron/refactor-tactics-main/issues/77) |
| `Zone_TopRight` | angolo in alto a destra, `Right` × `Top` | `WBP_RT_PlanCommit` — `Conferma` · `Annulla` ([D-458](../../decisions/RT_PDR_00_Decision_Log.md), [#3471](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3471)), dalla seduta `U64`; la resa della tavola B ([D-496](../../decisions/RT_PDR_00_Decision_Log.md)) dalla seduta `U72` ([#3633](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3633)) | `progettazione-hud.md` §6.8 |
| `Zone_MiddleLeft` | colonna sinistra, da `LeftTop` alla fascia bassa | `WBP_RT_SelectedUnitPanelLeft` (+ `SubjectCard`, una `WBP_RT_UnitCard`); la resa delle tavole A e B dalla seduta `U76` | [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613) · [#2760](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2760) |
| `Zone_MiddleRight` | colonna destra, dalla fascia alta a quella bassa | `WBP_RT_EventLogRight` — istanza di `WBP_RT_EventLog_C`, dal 2026-09-10 ([#2697](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2697)); la resa della tavola D dalla seduta `U77` ([#3637](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3637)) | [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613) · [#1896](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1896) |
| `Zone_Bottom` | tutta la larghezza, alta `Bottom` | `WBP_RT_ActionDockBottom` (+ `WBP_RT_ActionSlot`) — tutta la fascia, [D-456](../../decisions/RT_PDR_00_Decision_Log.md); in coda alla riga del dock, l'indicatore del piano «Piano N di M» con ▲/▼ ([D-501](../../decisions/RT_PDR_00_Decision_Log.md), [#3680](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3680), §7-septies) | [#220](https://github.com/DegrassiAaron/refactor-tactics-main/issues/220) · [#2760](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2760) · [#3469](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3469) |
| *la scena* | il rettangolo fra le fasce | ⛔ **NESSUN PANNELLO SCREEN-HUD PERMANENTE** — la 3D si rende solo qui, con il Tactical World Overlay §4.2. ✅ **Una sola esenzione: `WBP_RT_FastDecision`**, `Collapsed` tranne mentre la finestra è aperta ([#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166)) | [#2184](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2184) · `progettazione-hud.md` §3.1 |

🎨 **Il bordo da cantiere si spegne quando il contenuto arriva**, ed è il contratto di `bBlockoutVisible`
(`RTHudZoneWidget.h`): ogni istanza di `WBP_RT_HudZone` nasce col bordo colorato, e lo si spegne **sull'istanza**
nel `WBP_RT_TacticalHUD`, non nella classe. Oggi è spento su `Zone_Bottom` e `Zone_TopRight`
([#3494](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3494)); le altre zone hanno ancora un
contenuto graybox e lo tengono. Lo legge `ScreenHud.ZonesWithTheirContentHideTheBlockout`: la prossima zona che
lo spegne si aggiunge lì, nella stessa PR dell'asset.

✅ **`WBP_RT_FastDecision` è montato dal 2026-09-12**, nel `Canvas Panel` **radice** — non in una zona,
e non a runtime. Fino a quel giorno non era montato da nessuna parte, e fino allo stesso giorno questa
tabella diceva che il `BOTTOM` lo conteneva *«a runtime»*: era falso in entrambe le direzioni. Resta
scritto perché la correzione non si legga come se il difetto non ci fosse mai stato.

| Proprietà | Valore |
|---|---|
| **Anchors** | preset **centro** — `(0.5, 0.5)` → `(0.5, 0.5)` |
| **Alignment** | `0.5, 0.5` — ⚠️ **senza, al centro finisce l'ANGOLO del widget** |
| **Position X / Y** | **scritta dal C++**: `((Left − Right)/2, (Top − Bottom)/2)`, cioè il centro della **scena** e non dello schermo ([D-497](../../decisions/RT_PDR_00_Decision_Log.md) (5)). Costante in unità Slate: circa `(7.7, 10.8)` |
| **Size** | `600 × 200` |
| **ZOrder** | `10` — sopra le zone |

🔑 **La visibilità NON si lega qui**, e cercare quel binding nell'albero della HUD è il giro sbagliato:
`GetWindowVisibility` è un membro di `URTFastDecisionWidget`, **non** di `URTTacticalHUDWidget`, quindi il
menu `Bind` della HUD non lo offre. Il binding vive **dentro** `WBP_RT_FastDecision`, su `WindowRoot`, dove
`RefactorTactics.ScreenHud.FastDecisionBindingsAreWiredToTheRightNodes` lo pinna. L'istanza nella HUD resta
visibile: è il suo **contenuto** a collassare, ed è quello che si vede a schermo.

⚖️ **L'esenzione dalla scena libera è dichiarata, non silenziosa.** `ScreenHud.PanelsLeaveTheSceneFree` la
concede per **classe** e **solo** per il criterio della scena, e verifica che la finestra sia centrata sulla scena: `AutoSize` e fuori-viewport restano misurati
anche sulla finestra, e una seconda istanza esente è un rosso. La ragione è che `progettazione-hud.md`
§3.1 vieta ciò che occupa il centro **permanentemente**, mentre §47-bis.2 vuole il countdown al centro e
non come «animazione periferica».

⛔ **Resta scoperto ciò che l'archetipo non può mostrare**: che la finestra *compaia* quando la reazione si
apre è una prova di partita, non d'albero — [#3047](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3047).

✅ **Due gate nuovi guardano questa griglia dal 2026-09-12**, e chiedono cose diverse:
`RefactorTactics.ScreenHud.TheZonesAreDeclaredExactlyOnce` (⌫ *`TheEightZones…` fino al 2026-10-04*) — c'è una zona per ogni valore di
`ERTHudZone`, nessuna mancante e nessuna doppia — e
`RefactorTactics.ScreenHud.ZoneRectanglesMatchTheBands` (⌫ *`…ThreeByThreeGrid` fino al 2026-10-11*), che posa
le zone dal dato su una copia dell'albero, ricalcola il rettangolo di ogni zona con la formula di
`SConstraintCanvas::OnArrangeChildren` a quattro risoluzioni e lo confronta con la sua cella. Il secondo esiste
perché `PanelsLeaveTheSceneFree` è un gate **negativo**: dice che nessuna zona invade la scena, e passerebbe con
tutte le zone schiacciate in un angolo. Il terzo, `ScreenHud.TenantsFitTheirBands`, costruisce l'HUD vero in uno
scenario e verifica che ogni inquilino entri nella sua cella.

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
altre zone, non aggiungendone una.

🔑 **Per la stessa ragione il centro NON è un valore di `ERTHudZone`.** L'enum ha le sue zone più la
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

`WBP_RT_TurnHeader` deve mostrare **tre** cose diverse, e due si confondono. Dalla seduta `U75` le decide
`RefreshLook`, nelle porte `RoundValueText` e `RoundLimitText`; la tabella dice la regola, il testo a schermo è
quello della tavola A («ROUND 04 / 12»):

| Stato | Testo | Perché |
|---|---|---|
| nessun contesto | `—` in `RoundValueText`, il limite `Collapsed` | un widget costruito prima del `TurnManager`; `Round 0` sembrerebbe un dato |
| formato con limite | `03` e `/ 12` | il limite viene dal **formato**, mai da una costante |
| formato senza limite | `03`, il limite `Collapsed` | `RoundLimit == 0` = «nessun limite», **non** «su zero» |

Non comporre il testo nel Blueprint: un binding ingenuo stampa `/ 0`, che si legge come una partita già
scaduta. `GetRoundCounterText()` applica la stessa regola in un testo solo (`Round 3/12`, `Round 3`) per chi lo
vuole intero, come la riga di stato del Canvas.

⌫ *Fino alla seduta `U75` ([#3635](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3635)) l'asset legava `RoundText` a `GetRoundCounterText`. La tavola A spezza il contatore
in `RoundValueText` e `RoundLimitText`, che scrive `RefreshLook` con la stessa regola: il limite è `Collapsed` se
`RoundLimit` è 0. `GetRoundCounterText` resta per chi vuole il contatore in un testo solo.*

### Le porte della resa delle tavole A e D ([#3635](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3635))

Tutte `BindWidgetOptional` su `URTTurnHeaderWidget`, scritte a ogni tick da `RefreshLook`; la presenza dell'obiettivo
si rilegge dalla mappa una volta al secondo. La seduta `U75` ha tolto i binding di proprietà di prima, e
`RoundLimitComesFromTheFormat` cade se ne torna uno: sarebbe una seconda fonte dello stesso testo.

| Porta | Tipo | Che cosa ci scrive il C++ |
|---|---|---|
| `HeaderFrame` | `Border` (`RoundedBox`, tinta bianca) | fondo `BG_Panel` al 92%, contorno `Frame_Deep` di 1 px |
| `RoundValueText` · `RoundLimitText` | `TextBlock` | il round a due cifre; «/ 12», `Collapsed` se il formato non ha limite |
| `PlanningGroup` · `ResolutionGroup` | qualunque widget | uno solo visibile: il primo in Pianificazione, il secondo in Risoluzione |
| `PhaseText` · `PhaseMark` | `TextBlock` · `Border` | la fase logica in maiuscolo, in `Cyan`, da `URTHudViewModel::ComposePhaseWord`; il quadrato col contorno `Cyan` |
| `TimerText` | `TextBlock` | `00:21`, da `SecondsUntilCommit`: il commit, non il Planning. Vuoto se nessun orologio corre |
| `PhaseCell_*` · `PhaseCellMark_*` · `PhaseCellText_*` | `Border` · `Border` · `TextBlock` | per `Prep`, `Dash`, `Blast`, `Move`: attiva la fase **riprodotta** ([D-479](../../decisions/RT_PDR_00_Decision_Log.md)) |
| `ObjectiveBox` · `ObjectiveScoreText` · `ObjectiveThresholdText` | widget · `TextBlock` · `TextBlock` | solo se la mappa dichiara un obiettivo ([D-481](../../decisions/RT_PDR_00_Decision_Log.md)): «2–1», squadra 0 – squadra 1; «(a 3)», `Collapsed` se la soglia è 0 |

🔑 **La cella attiva cambia FORMA, non solo colore.** Il segno diventa pieno e quadrato con un contorno bianco;
spento resta vuoto, tondo e più piccolo (una scala di render, che non sposta il layout). In scala di grigi la
forma è ciò che resta. Il segno lo rende `RoundedBox` il C++ stesso, qualunque cosa dica l'asset.

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

### Le porte della resa della tavola E ([#3634](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3634))

Tutte `BindWidgetOptional`: il C++ le scrive a ogni tick in `RefreshLook`, coi colori di `URTUIPalette`. Senza di
loro la finestra resta quella dei tre binding qui sopra, che **non si toccano**.

| Porta | Tipo | Che cosa ci scrive il C++ |
|---|---|---|
| `WindowFrame` | `Border` (`RoundedBox`) | fondo `BG_Deep`, contorno `White` di 2 px. ⚠️ Tinta del brush **bianca**: il fondo è `BrushColor` (§7-quater) |
| `AccentStrip` | `Border` | `BrushColor` `Violet`: la striscia a sinistra |
| `CountdownBar` | `ProgressBar` | la frazione di `Get Countdown Fraction`, riempimento `White`. Il fondo della barra (`Frame_Off`) sta nell'asset |
| `SafeOutcomeText` | `TextBlock` | «Allo scadere: HOLD», da `Get Safe Outcome Text` |

Due letture nuove: **`Get Countdown Fraction`**, il residuo autorevole sulla durata (da 1 a 0, negativo senza
finestra), e **`Get Safe Outcome Text`**, col nome della `SafeResponse`: nel `Brace` dice «HOLD GROUND».

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

### Le porte dell'opzione ([#3634](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3634))

Scritte da `SetOption` **prima** di `On Option Changed`, quindi il grafo le trova già al loro posto. `LabelText` resta
del grafo.

| Porta | Tipo | Che cosa ci scrive il C++ |
|---|---|---|
| `OptionFrame` | `Border` (`RoundedBox`, tinta bianca) | fondo `BG_Raised`; contorno `Frame_Mid` di 1 px, `Text_Primary` di 2 px sulla scelta sicura |
| `KeyText` | `TextBlock` | il tasto ([D-491](../../decisions/RT_PDR_00_Decision_Log.md)): `1`, `2`…, in `Text_Primary` |
| `DescriptionText` | `TextBlock` | la frase della risposta, in `Text_Secondary` |
| `SafeChoiceBadge` | qualunque widget | visibile solo sulla scelta sicura: il chip «SCELTA SICURA». Sta in `LabelRow`, accanto all'etichetta, e non in una terza colonna (§7-quater) |

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

## 7-quater. Le cornici che dipinge il C++, e i glifi

Regole misurate nelle sedute `U72` ([#3633](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3633)) su `WBP_RT_PlanCommit`, `U75` ([#3635](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3635)) su `WBP_RT_TurnHeader`, `U76` ([#3636](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3636)) su roster e pannello, e `U74` ([#3634](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3634)) su `WBP_RT_FastDecisionOption`.
Nessun gate sull'albero le vede: si scoprono solo a schermo.

- 🔴 **Una cornice che il C++ dipinge ha la tinta del brush BIANCA.** `RTPaintFrame` scrive il fondo in `BrushColor`, e
  Slate lo moltiplica per `Background.TintColor`. Se nell'asset la tinta e' gia' il colore di fondo, il fondo esce
  quasi nero (misurato `(0,0,1)` al posto di `BG_Panel`). Le cornici che il C++ non tocca, come i badge statici,
  portano invece il colore nella tinta.
- 🔑 **I glifi (✔ ✖ ▲ ◆ ●) vogliono la Roboto dell'engine.** `/Engine/EngineFonts/Roboto` ha `DroidSansFallback` come
  typeface di ripiego, e quello ha i glifi. I font del progetto (`F_RT_Exo2_*`, `F_RT_Orbitron_*`) hanno un solo
  typeface e nessun ripiego: con loro un glifo assente diventa un quadrato vuoto. Le lettere restano in Exo 2 e
  Orbitron.
- 🔴 **Una cornice che il C++ dipinge con un fondo trasparente o traslucido vuole `bUseBrushTransparency` SPENTO**
  (`U75`). Acceso, l'alfa del `BrushColor` vale anche per il contorno. Il segno della fase, dipinto con fondo
  trasparente e contorno `Cyan`, a schermo non c'era; con un fondo al 20% o al 92% il contorno si attenua allo stesso
  modo. Vale per il segno della fase, per tutte le celle e i loro segni, e per `HeaderFrame`.
- 🔑 **L'area di una zona alta parte da y=−1.** Per mettere un riquadro alla y della tavola, la radice vuole un pixel
  in più: 25 per stare a 24. Misurato in `U72` e in `U75`.
- ✅ **Una radice `Visible` che riempie la zona non mangia i click della mappa** (`U75`). Il click che nessun widget
  gestisce risale al viewport: dentro `TopCenter`, fuori dal riquadro, pianifica il percorso.
- 🔑 **Esagoni no.** Nessun font dell'engine ha U+2B21–U+2B23: lo dice la `cmap` di `Roboto-*` e `DroidSansFallback`.
  Un'icona d'obiettivo passa dal catalogo ([D-031](../../decisions/RT_PDR_00_Decision_Log.md)).
- 🔑 **I glifi geometrici sono circa 0,6 em.** Per un triangolo di 16–20 px il ▲ vuole 24–27 (in `U72` a 32 per il badge).
  Il segno («!», «✕», «i») va centrato col glifo nella scatola naturale del glifo. Un `SizeBox` stretto attorno ai due
  sposta il glifo in basso e il segno ne esce (`U76`).
- 🔴 **Un default di classe scritto sul CDO non arriva alle istanze già caricate** (`U76`). `CardClass` scritto sul CDO
  di `WBP_RT_TeamRoster` lasciava nullo il roster dentro il TacticalHUD in memoria, e il roster usciva vuoto. ⛔ **Non
  salvare il contenitore in quello stato**: il nullo diventerebbe un override scritto. Chiudi l'Editor senza salvarlo e
  riaprilo. `RosterAndPanelWearTheirClasses` vede l'override nullo una volta salvato. Lo stato in memoria non si
  riproduce in un processo fresco, e lo si vede solo a schermo.
- 🔴 **I testi veri sono piu' lunghi di quelli della tavola: si misura con loro** (`U74`). La tavola scrive «Non
  attaccare»; il catalogo (`RTReactionResponseText.cpp`) scrive «Non spari: lasci passare il bersaglio.», e «Hold
  Ground» e' piu' lungo ancora. Un `TextBlock` in una colonna `Fill` non si ritaglia: sborda sotto il widget accanto.
  Il badge della scelta sicura e' salito nella riga dell'etichetta, la descrizione va a capo, e l'altezza
  dell'opzione e' un minimo, non un valore fisso.

## 7-quinquies. Roster, carta e pannello: le porte della tavola ([#3636](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3636))

Tre cose cambiano forma, non solo colore:
- la carta del roster ha una classe C++, `URTUnitCardWidget`;
- il roster costruisce le carte in C++;
- il pannello scrive chip e avvisi dalle sue porte.

Il grafo che costruiva le carte **è stato tolto nella seduta `U76`**, insieme al riparentamento di `WBP_RT_UnitCard`.

### `WBP_RT_UnitCard` → `URTUnitCardWidget`

La carta **riceve** la vista (`SetCard`, solo C++) e non va a prenderla. Le porte:

| Porta | Tipo | Che cosa ci scrive il C++ |
|---|---|---|
| `CardFrame` | qualunque widget | opacità piena da viva, ridotta da caduta |
| `TeamDot` | `Border` | `Cyan` per un'alleata, `Red` per un'avversaria |
| `NameText` | `TextBlock` | il nome leggibile, `FRTUnitCardView::DisplayName`: mai l'id grezzo |
| `ReactionChip` | qualunque widget | visibile solo con una reazione armata ([D-478](../../decisions/RT_PDR_00_Decision_Log.md)) |
| `HealthBar` · `HealthText` | `ProgressBar` · `TextBlock` | riempimento `Cyan`; «90/90» |
| `ShieldText` | `TextBlock` | lo scudo, in `Text_Secondary` |

### `WBP_RT_TeamRoster`

`CardsBox` (un pannello) e, nei default, `CardClass` = `WBP_RT_UnitCard_C`, come `TooltipClass` dello slot. Senza
`CardClass` il roster non costruisce niente; `CardsBox` e `OpposingCardsBox` si riempiono indipendentemente, e vanno
tenuti distinti, perché un pannello condiviso mescolerebbe l'ordine delle due liste. Le carte si **riusano** da un
tick all'altro.

`OpposingCardsBox` (un pannello) riceve le carte delle altre squadre, da `GetOpposingRoster()`: vuoto in sessione
presidiata, pieno solo in autobattle (#2744). Il chip `REAZ.` vi resta spento per costruzione (D-478 punto 4). ⚠️ Era
il secondo ciclo del grafo, che scriveva in `OpposingBox`: togliendo il grafo senza questa porta, le carte avversarie
spariscono dall'autobattle senza che nessun test cada.

🔴 **Le carte avversarie hanno il pallino `Red`.** Fino al seguito C++ di #3636 `GetOpposingRoster` le costruiva con
`BuildTeamRoster(Units, TeamId)`, che usa la squadra elencata anche come punto di vista: ogni avversaria risultava
«alleata». Il vecchio `WBP_RT_UnitCard` non aveva il pallino, e l'errore non si vedeva.

### `WBP_RT_SelectedUnitPanel`

| Porta | Tipo | Che cosa ci scrive il C++ |
|---|---|---|
| `SubjectCard` | `WBP_RT_UnitCard` (`URTUnitCardWidget`) | la carta del soggetto, comandato o ispezionato, da `GetCard()`; il chip `REAZ.` solo dal piano della comandata. Prima la scriveva il grafo nella variabile Blueprint `Card` |
| `PlanChipsBox` | qualunque widget | i tre chip; `Collapsed` per un'unità ispezionata e in Risoluzione. ⛔ Chiuso, non «—»: tre «—» direbbero «non ha pianificato» |
| `Main`/`Movement`/`ReactionChipFrame` | `Border` (`RoundedBox`, tinta bianca) | fondo `BG_Raised`; contorno `Frame_Off`, `Amber` se l'azione ha un avviso |
| `…ChipStrip` | `Border` | il colore della fase dell'azione (`SlotPhaseMark`), trasparente se libero |
| `…ChipValueText` | `TextBlock` | il nome dell'azione, o «—» in `Text_Disabled` |
| `…ChipAlert` | qualunque widget | il «!»: visibile solo se un avviso nomina quell'azione |
| `PlanWarningsBox` | un pannello | le voci degli avvisi; `Collapsed` senza avvisi |

Nei default, `PlanWarningEntryClass` è la classe delle voci, `URTPlanWarningEntryWidget`. Le sue porte sono:
- `EntryFrame`;
- `LevelGlyphText`, con la forma del livello in Roboto dell'engine, ◆ ▲ ● (§7-quater);
- `LevelMarkText`, ✕ ! i;
- `LevelText`, «CRITICAL», «WARNING», «INFO»;
- `WhatText`, `WhyText` e `CostText`.

⚠️ **Gli avvisi si ricontano a 5 Hz**, come il contatore di `Conferma`: `GetPlanWarnings` rifà il validatore e
l'anteprima del Blast. Due eccezioni non aspettano il giro:
- in Risoluzione, o per un'unità ispezionata, l'elenco si svuota subito;
- se cambia l'unità comandata, si riconta subito.

Anche il chip `REAZ.` delle carte si riconta a 5 Hz, mentre i PV seguono a ogni tick.

🔑 **Ogni avviso accende il «!» di un chip solo** (`URTHudViewModel::ChipForWarning`):
- la principale, se ne nomina l'azione;
- la reazione, se nomina la sua;
- altrimenti il movimento. Ci finiscono anche le voci che `MakePlanFor` aggiunge dal core e che lo slot non nomina.

## 7-sexies. Il registro: le porte della tavola D ([#3637](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3637))

Il registro compare **solo in Risoluzione** ([D-496](../../decisions/RT_PDR_00_Decision_Log.md) punto 2). Le righe le costruisce il C++ da una classe
nei default, come le carte del roster. Il grafo che le costruiva è stato tolto nella seduta `U77`, insieme al
riparentamento di `WBP_RT_EventLine`.

### `WBP_RT_EventLog`

| Porta | Tipo | Che cosa ci scrive il C++ |
|---|---|---|
| `LogRoot` | qualunque widget | `Collapsed` fuori dalla Risoluzione |
| `LogTurnText` | `TextBlock` | «ROUND 04», accanto a «REGISTRO»: la parola dell'header ([D-496](../../decisions/RT_PDR_00_Decision_Log.md) punto 1) |
| `LinesBox` | un pannello | una riga per voce di `GetFeed()`, riusate da un tick all'altro |

Nei default, `LineClass` = `WBP_RT_EventLine_C`. Senza `LinesBox` o senza `LineClass` il registro non costruisce
niente.

### `WBP_RT_EventLine` → `URTPlayerEventLineWidget`

| Porta | Tipo | Che cosa ci scrive il C++ |
|---|---|---|
| `PhaseStrip` | `Border` | il colore della fase della riga ([D-233](../../decisions/RT_PDR_00_Decision_Log.md)) |
| `PhaseLabelText` | `TextBlock` | «BLAST», la sigla della fase |
| `SequenceText` | `TextBlock` | «#3» |
| `ReactionBadge` | qualunque widget | il chip `REAZ.`: solo su una reazione scattata |
| `LineText` | `TextBlock` | la frase |

🔴 **Il «#n» è la posizione nel feed GIÀ FILTRATO per chi guarda**, non l'indice della voce nel TurnLog. Un indice
globale lascerebbe buchi dove stanno gli eventi nascosti, e i buchi direbbero quanti sono: lo stesso genere di
metadato che #2757 vieta per gli slot. Lo misura `Privacy.EventFeedNewFieldsRevealNothingHidden`. Lo stesso log,
con e senza le voci nascoste, dà a chi guarda le stesse righe, campo per campo, anche con una voce nascosta della
stessa unità e con una cella d'ambiente nascosta.

⚠️ **Quattro cose da sapere prima della seduta**:
- le righe si costruiscono solo in Risoluzione, e il feed si rilegge a 5 Hz, subito all'ingresso;
- la riga d'ambiente aggrega più celle: la sua striscia è la fase della **prima** cella visibile;
- il feed è già pieno al primo tick di Risoluzione, perché il resolver risolve tutte le fasi prima del playback.
  Le righe di Blast e Move compaiono quindi prima che il playback le mostri. La riga corrente evidenziata, che
  risolverebbe la cosa, chiede l'indice dell'evento riprodotto (T5, #1881);
- ⏳ `DEFERRED`: la riga «nessun evento» per una fase senza eventi, che la tavola disegna tratteggiata. Il C++ costruisce
  una riga per voce del feed, e una fase vuota non ha voci.

## 7-septies. L'indicatore del piano nel dock ([#3680](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3680))

«Piano N di M» con ▲ e ▼, in coda alla riga del dock (`BarRow`), accanto alla lettura del movimento
([D-501](../../decisions/RT_PDR_00_Decision_Log.md)).

🔑 **Il C++ lo costruisce, e l'asset non ne sa niente.** `URTActionDockWidget::EnsureLayerIndicator` gira in
`NativeOnInitialized`, prima che Slate costruisca il dock, e aggiunge l'indicatore al pannello di `MovementReadout`.
Se `WBP_RT_ActionDock` un giorno dichiarasse le porte qui sotto nel Designer, il C++ non costruirebbe niente e le
riempirebbe come quelle della lettura del movimento.

| Porta | Tipo | Che cosa ci scrive il C++ |
|---|---|---|
| `LayerIndicator` | qualunque widget | `Collapsed` senza mappa e con un piano solo |
| `LayerIndicatorText` | `TextBlock` | «Piano N di M»: N e M sono ordinali fra i piani che la mappa ha, da 1, non la quota del layer |
| `LayerUpButton` | `Button` | il click chiama `StepLayerUp()`; disattivato sul piano più alto |
| `LayerDownButton` | `Button` | il click chiama `StepLayerDown()`; disattivato sul piano più basso |

- ▲ e ▼ **inoltrano a `ARTPlayerController::StepActiveLayer`**, il corpo di `PageUp`/`PageDown`. La vista legge piano
  attivo e limiti dallo stesso controller (`GetMapLayerRange`), quindi segue anche la tastiera.
- 🔴 **I tasti non prendono il fuoco della tastiera** (`URTMouseOnlyButton`). Uno `SButton` col fuoco tratta `Spazio`
  e `Invio` come un click: dopo ▲, il lock-in di `Spazio` avrebbe ripremuto ▲. Se le porte si dichiarano nel Designer,
  vanno messe con **Is Focusable** spento.
- Un click sui tasti non arriva al mondo: il dock sta nella fascia bassa, fuori dal rettangolo della scena (§3).

Lo misurano `ScreenHud.LayerIndicatorCountsTheLayersTheMapHas`, `ScreenHud.LayerIndicatorFollowsTheControllerAndItsButtonsStep`,
`ScreenHud.LayerIndicatorHidesOnASingleLayerMap`, `ScreenHud.DockMountsTheLayerIndicatorBesideTheMovementReadout` e,
per la fascia, `ScreenHud.TenantsFitTheirBands`.

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
  -ExecCmds="Automation RunTests RefactorTactics.Frontend;Quit" \
  -unattended -nopause -nullrhi -NoSound
```

> ⚠️ **`;Quit`, non `+Quit`** — questa riga ha prescritto la seconda forma fino al 2026-09-12. `+` separa
> **filtri**, non comandi: con `+Quit` la parola diventa un secondo filtro che non corrisponde a nulla, il
> comando non viene eseguito e il processo **non esce**, restando a tenere il mutex del motore. I test
> girano lo stesso, quindi non c'è niente che lo segnali.
> [`test-e-diagnosi.md` §9](test-e-diagnosi.md) porta la misura; l'origine è
> [#3049](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3049).

I quattro `RefactorTactics.Frontend.MatchHud*` provano il ciclo di vita del layer senza aprire l'Editor.

⚠️ **L'exit code di `UnrealEditor-Cmd` non dice se i test sono girati** — il processo può restare appeso
dopo il `Quit` e uscire con `127` a suite completata. Giudica dal log (`Saved/Logs/RefactorTactics.log`):
cerca `Found N tests` e i `Test Completed. Result={...}`.

> 🔁 **Corretto il 2026-08-26.** Questa sezione rimandava ai gate di `RT-FEAT-UI-SCREEN-HUD` in
> `feature-registry.yaml`. ⛔ **Quel file non esiste più**: il tooling è uscito dal repository con
> **D-181/D-182** (2026-08-21). Puntare lì mandava a cercare uno stato che nessuno scrive più.
