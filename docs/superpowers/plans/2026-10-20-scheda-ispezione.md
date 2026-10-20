# La scheda dell'ispezione (F3) — Piano di implementazione

> Per chi esegue: i passi sono caselle da spuntare. Ogni Task che tocca codice finisce con compilazione, test e mutazione; la review è di un agente che riceve diff e file, non le conclusioni di chi ha scritto.

**Goal:** fissare, con il click su un'unità non comandata e osservata, un pannello nella zona `MiddleRight` che mostra carta, stati, verso, **kit dell'eroe** (numeri di catalogo) ed **equipaggiamento**; `RMB` lo chiude; funziona anche in `ResolutionPlayback`.

**Spec:** `docs/superpowers/specs/2026-10-13-ispettore-del-mondo-design.md`, fetta `F3` (§3 regole 4, 5, 6, 8, 9, 10, 11, 12; §4; §5 P1, P2, P6; §7). **Issue:** #3748. **Branch:** `issue/3748-scheda`, da `main` a `a1bdadd0a`.

**Decisione d'autore (2026-10-20, in sessione):** kit ed equipaggiamento di un nemico osservato sono **pubblici** (voce 1 di §9.3). Restano privati piano, slot del turno, azione armata e ricariche vere; le ricariche dedotte sono di F6 (#3751).

**Fuori da questa fetta:** ricariche dedotte (F6), aree di movimento e portata (F5), `Ctrl` + click (F4), elenco dei verbi di una porta. Lo **sguardo** (F2) è già in `main`: se ne riusano cancello e soggetto.

## Global Constraints

- Il compositore della scheda riceve un **soggetto di soli valori** e non vede mai l'attore (P1). Una sola funzione legge l'attore di un'unità non comandata.
- Un nemico non osservato (nessuna voce `Live` nella vista di conoscenza) **non ha scheda**: stessa uscita di una cella vuota. `ARTUnit::IsKnownToObserver()` nasce vero e non decide niente.
- Nessuna modifica a stato canonico, snapshot, hash, TurnLog, replay. Se un campo nuovo entra su `ARTUnit`, un test dimostra che l'hash dello stato di partita non cambia.
- Nessun `.uasset` modificato da un worktree: l'authoring appartiene al clone principale.
- Niente `DrawDebug*`. Niente totali volatili nei testi: elenchi di nomi, o il comando che conta.
- Build in PowerShell, `"-Project=$P"`, `-WaitMutex -NoHotReloadFromIDE`; leggere `Result:` ed `EXIT=`. Suite con `tools/suite/esegui.py`, `--log` in un file; contare `Test Completed. Result={Success}` con `-CaseSensitive`, e `trovati == verdi + rossi`.
- Mutazione per ogni test che prova una guardia: commit prima, applica, build, test, `git checkout -- <file>`, ricompila, rimisura.
- Nessun merge senza l'ok esplicito dell'autore; nessun push senza autorizzazione.

## Stato misurato (2026-10-20, `a1bdadd0a`)

Ricognizione in sola lettura, citata per simbolo.

| Fatto | Dove |
|---|---|
| `InspectedUnit` è `TWeakObjectPtr<ARTUnit>`. Si scrive in due punti: `DispatchUnitClick` (esito `Inspect`) e `HandleClickOnCell` (click sulla cella del nemico puntato dalla carica). Un'unità distrutta lo azzera da sola; un KO non ancora distrutto no | `Player/RTPlayerController.h/.cpp` |
| `SetInspectorPinned` **non ha nessun chiamante** fuori dai test; il commento dice "il produttore è lo Screen HUD (#613)" e non c'è. `ApplyBack`, caso `Inspector`, fa solo `bInspectorPinned = false` e **non azzera `InspectedUnit`** | idem |
| `ResolveBack` mette `Inspector` dopo `ReactionWindow` e `Modal` e prima di `Facing`; vale in qualunque contesto, playback compreso. `ApplyBack` arriva da `UndoStep`, dietro `IsGameplayInputBlocked`, `IsPlanningInputInert` e il conto alla rovescia di Unready: **non misurato se in playback il tasto destro arrivi a `ApplyBack`** | `Player/RTPointerInteraction.cpp`, `RTPlayerController.cpp` |
| `ResolveOutcome` risponde `Blocked` in `Modal`, `ResolutionPlayback` e `ReactionWindow`, anche per un'unità osservata. `IsWorldReadOnly` ferma i click nei punti che mutano. Il commento di `OnSelect` dice che §5.3 "consente Inspect durante il playback": **contraddice il codice**. `HexMatch.PlaybackAllowsInspectAndRestoresPreview` passa da `SelectUnit` su un'unità propria, non da `DispatchUnitClick` | idem; `Tests/RTHexMatchIntegrationTests.cpp` |
| `GetPointerContext` non produce `ReactionWindow` (lo dice il suo commento): l'inerzia in quel contesto si prova solo sulla funzione pura | `Player/RTPlayerController.cpp` |
| `Pathing`: `ResolveOutcome` risponde `Inspect` per un'unità non comandabile mentre §4.1 mette la cella prima: `IMPLEMENTATION DRIFT`, segnalato a #705, non risolto qui | spec §3 regola 9 |
| `MiddleRight` ha **un solo** NamedSlot `Content`, occupato da `URTPlayerEventLogWidget`, che compare solo in Risoluzione. L'albero è in `WBP_RT_TacticalHUD` (asset). **Non c'è un meccanismo per due inquilini nello stesso slot** | `UI/RTHudZoneWidget.h`, `UI/RTScreenHudWidgets.h` |
| `ScreenHud.TenantsFitTheirBands` salta un inquilino che misura `<= 1`: un pannello vuoto o `Collapsed` **non viene mai misurato**. Un inquilino nuovo va nel blocco `Refresh*` per classe e deve avere un soggetto già impostato nello scenario | `Tests/RTHudScenarioTests.cpp` |
| `URTSelectedUnitPanelWidget::GetSubject` dà la **selezione prima dell'ispezione**; `GetSlots` e `GetPlanWarnings` seguono solo la selezione, quindi per un soggetto non comandato il piano non arriva | `UI/RTScreenHudWidgets.cpp` |
| `BuildActionTooltip(const FRTAbilityCooldownView&, bool bArmed)` non prende l'attore ma una riga già costruita da `BuildAbilityCooldowns(const ARTUnit*)`, che mescola catalogo (nome, descrizione, fase, slot, portata, forma, ricarica, danno) e partita (`TurnsRemaining`, `bPlanned`, `bPlanInvalid`, `bUsableNow`). **Non esiste una funzione solo-catalogo** | `UI/RTHudViewModel.cpp` |
| Tooltip pinnato da: `HudViewModel.TooltipComposesTheCatalogRows`, `TooltipNamesWhyTheSlotIsOff`, `TooltipReadsTheUnitsCatalog`, `ScreenHud.ActionSlotCarriesItsTooltip`, `TooltipNamesADegradedTargetOnlyWhenKnown`, `ScreenHud.DockTooltipShowsTheShapeOfEveryRosterArea`, `ScreenHud.ActionSlotWearsTheTooltipAsset` | `Tests/` |
| Kit: `URTHeroData::Actions` (indice 0 attacco base, 1-4 fondamentali, 5 facoltativa), campi leggibili senza attore. **Non c'è un `FindHero(HeroId)`**: `GetHeroRoster()` costruisce istanze nuove a ogni chiamata | `Ability/RTHeroData.h`, `RTHeroCatalogLibrary.h` |
| Equipaggiamento: `URTEquipmentData` (slot variante d'arma, gadget, modulo). `DefaultLoadoutFor(HeroId)` è **per eroe**, vuoto per Aevik e Ivrin. `ARTUnit::EquipLoadout` applica i pezzi ma **non registra gli id**: nessun campo dell'unità dice cosa è montato | `Ability/RTEquipmentData.h`, `RTCatalogLibrary.h`, `Unit/RTUnit.cpp` |
| Modelli di soggetto pubblico: `FRTGlanceUnitSubject` e `URTGlanceLibrary::ResolveGlance` (cancello `Live`), `FRTContextInspectorView` (solo `FString`/`FRTCellId`/`int32`/`bool`); test per reflection `UI.GlanceViewCarriesNoRawTypes` | `UI/RTGlance.h`, `Debug/RTContextInspector.h` |
| Widget in C++ puro: `URTContextInspectorWidgetBase` (`RebuildWidget`, `Placement()`, righe a `Text_Lambda`). **`NativeOnMouseWheel` e `NativeOnMouseButtonDown` non esistono in nessun widget del progetto**; la rotella è un'azione Enhanced Input (`OnZoom`) che **non guarda se il cursore è su un pannello** e ancora lo zoom alla cella sotto il cursore | `Debug/`, `Player/RTPlayerController.cpp` |
| `OnSelect` fuori dalla scena non prende un'unità (`IsCursorInsideScene`); il tasto destro non guarda `IsCursorInsideScene` | `Player/RTPlayerController.cpp` |
| Nulla azzera `InspectedUnit` quando l'unità esce di vista. `FRTKnowledgeEntry` non porta `bAlive` né salute; `ViewForTeam` salta i non vivi | `Perception/RTKnowledgeView.h` |
| I commenti che chiamano fuga il kit di un'avversaria: `InspectedUnit` e `GetInspectedUnit` (`RTPlayerController.h`), `DispatchUnitClick` (`RTPlayerController.cpp`), `ResolveOutcome` (`RTPointerInteraction.cpp`, senza la parola "fuga" ma con la stessa tesi). Anche `GetSlots` in `RTScreenHudWidgets.cpp` ripete la regola degli slot, che **resta vera** | idem |

## Rulings

Scelte di chi scrive. Quelle marcate **(A)** chiedono la parola dell'autore *prima* del Task indicato; le altre sono revocabili.

| | Ruling | Costo se è sbagliato |
|---|---|---|
| **Q1** | ➕ rev. La **fonte del kit** sono le azioni **dell'unità** (`Unit->GetAbility(i)`, puntatori a dati di catalogo, mai `TurnsRemaining`, piano o `ActiveVariantId`), lette dentro l'unica funzione che legge l'attore: `EquipLoadout` duplica l'attacco base e ne cambia la portata, quindi `URTHeroData::Actions[0]` direbbe una portata diversa da quella che il nemico ha davvero. Le righe si compongono con una funzione solo-catalogo estratta da `BuildActionTooltip`, che prende un `URTActionData`. Lo slot continua a mostrare ciò che mostra oggi: lo provano i sette test del tooltip | se la divisione cambia una riga dello slot, cade uno dei sette |
| **Q2 (A)** | L'**equipaggiamento** si legge da un campo nuovo `ARTUnit::EquippedItemIds` (`TArray<FName>`), scritto da `EquipLoadout`, **mai letto dalla simulazione**, fuori da snapshot e hash. ➕ rev. Registra **solo i pezzi realmente applicati** (`EquipLoadout` salta pezzi inesistenti e varianti senza attacco base) e **si azzera a ogni chiamata**, perché una seconda chiamata accoda. È `UPROPERTY()` nuda, non visibile ai Blueprint, oppure ha la sua riga nel censimento `Unit.BlueprintSurfaceIsCensused`. Il test dell'hash tiene fisse le abilità e varia **solo** il campo, perché `NumAbilities()` entra già nel digest. L'alternativa, `DefaultLoadoutFor(HeroId)`, dice cosa un eroe monta di default e non cosa monta questa unità se il setup della partita lo cambia. **Da decidere prima del Task 3** | un campo su `ARTUnit` da non far entrare nell'hash: lo prova un test; con l'alternativa la scheda può mentire sull'equipaggiamento |
| **Q3** | Il soggetto della scheda è `FRTInspectSubject` (nome, carta, stati, verso, `HeroId`, `StableUnitId`, righe di kit, righe di equipaggiamento) di **soli valori**. Una funzione sola, `ARTHUD::BuildInspectSubject`, legge l'attore; il compositore `URTInspectLibrary::Compose` non lo vede | un campo grezzo che entra: lo prende il test per reflection |
| **Q4** | Il **cancello** è lo stesso dello sguardo: voce `Live` nella vista costruita da `UvViewForObserver`. La scheda riceve il soggetto solo se il cancello è aperto; altrimenti **non c'è scheda** (nessuna riga, come una cella vuota). Esce di vista o cade: si chiude (R13) | vedi Review Focus 1 |
| **Q5** | **Chi chiude**: `RMB` (passo `Inspector`) azzera `bInspectorPinned` **e** `InspectedUnit`. Oggi azzera solo il flag; senza azzerare il soggetto la scheda non si chiuderebbe mai e il pannello di sinistra resterebbe sull'ultimo nemico. Fissarne un'altra la sostituisce. Il passo `Inspector` resta prima di `Declaration` e `Waypoint` (R11) | un `RMB` in più per disarmare, già accettato da R11 |
| **Q6** | **Il produttore della fissazione** è `DispatchUnitClick` (esito `Inspect`): scrive `InspectedUnit` e accende `bInspectorPinned`. Nessun nuovo gesto. Il click sulla cella del bersaglio della carica (`HandleClickOnCell`) fissa allo stesso modo | due punti che scrivono: un test per punto |
| **Q7** | ➕ rev. **Playback**: l'unico cambiamento è in `ResolveOutcome`, che risponde `Inspect` per un'unità **non comandabile e osservata**; **il controllo sul velo passa prima di quello sul contesto**, perché oggi `Blocked` precede il controllo sul velo e un nemico non osservato in playback non darebbe `NoOp`. `DispatchUnitClick` non contiene `IsWorldReadOnly`: **non serve nessuna eccezione**, e non se ne cerca una (i due cancelli sono in `HandleClickOnUnit` e `HandleClickOnCell`, che un `Inspect` non attraversa). Un'unità propria resta `Blocked` per `ResolveOutcome`, ma `OnSelect` prosegue e la seleziona comunque (voluto: `HexMatch.PlaybackAllowsInspectAndRestoresPreview`). **R15 dichiarato:** la scheda in risoluzione segue la targa, quindi il verso è spento come in `bShowFacing` e la cella è quella animata; vita e scudo sono quelli della targa, anche se anticipano la scena (limite di #2453) | un nemico non osservato che in playback non dà `NoOp`: lo prende il test del velo esteso |
| **Q8 (A)** | ➕ rev. **Dove sta il pannello. Il ruling di prima non regge:** `URTPlayerEventLogWidget` è costruito dal `WBP_EventLog` (`LogRoot` e `LinesBox` sono `BindWidgetOptional`), non ha `RebuildWidget`, e `RTMatchWidgetAssetTests` prescrive un registro nel WBP. Restano tre strade, **da decidere prima del Task 5**: (a) **authoring nel clone principale** di un inquilino o contenitore in `MiddleRight` (UMG vero, entra in `TenantsFitTheirBands`; richiede il clone principale e un Editor); (b) un widget C++ puro che avvolge `Super::RebuildWidget()` del registro in un `SOverlay` (nessun asset, ma è un artificio che nessun widget del progetto usa); (c) un overlay a parte con `AddToViewport`, che rompe il criterio `grep AddToViewport|CreateWidget Source/` di `RTFrontendNavigator.h` e non è un inquilino. Consiglio (a) | tre strade con costi diversi: nessuna è gratis |
| **Q9** | **Input sul pannello**: il pannello è `Visible` (consuma il click). La rotella: `NativeOnMouseWheel` sul pannello scorre le righe; in più la decisione "zoomare il mondo?" è una **funzione pura** (`ShouldZoomWorld(bCursorInsideScene)`), usata da `OnZoom`, così si prova senza viewport. ➕ rev. `IsCursorInsideScene` è falso headless e anche nei primi fotogrammi: se la proiezione fallisce, lo zoom da fascia e da finestra non focalizzata è morto senza traccia. Va dichiarato e provato in PIE. Questo cambia lo zoom con il cursore sulle fasce: dichiarato | lo zoom da fascia smette di funzionare: è il comportamento voluto da §3 regola 6 |
| **Q10** | **Nascondere senza chiudere**: in `Modal` e `ReactionWindow` il pannello è `Collapsed` e `InspectedUnit` resta; `RMB` è di quei passi finché sono aperti. Ritorna alla chiusura. In sessione non presidiata (R14) `OnSelect` esce già su `IsPlanningInputInert`: nessun ramo nel widget. La decisione "nascosto" è una **funzione pura** (`ShouldShowInspectCard(Context, bPinned, bUnattended)`); `Modal` in headless richiede `IsPauseOpen()` del navigatore: dove non si ottiene, il test resta sulla funzione pura e lo dice. ⚠️ Durante il conto alla rovescia di Unready il tasto destro è Unready e non arriva a `ApplyBack`: la scheda non si chiude lì, dichiarato | `ReactionWindow` non è prodotto: provato sulla funzione pura |
| **Q11** | **Il taglio non è muto.** Righe che non entrano nell'altezza: la scheda scorre, e l'ultima riga visibile dice che ce ne sono altre (`URTContextInspectorLibrary::VisibleLines` come modello) | un'altezza che taglia in silenzio: lo prende un test |

| **Q12** | ➕ rev. **Chi chiude quando il soggetto esce di vista o cade** (R13). Oggi nessuno: `ResolveInspect` non azzera niente, e senza un produttore `InspectedUnit` e `bInspectorPinned` restano; il pannello di sinistra (`URTSelectedUnitPanelWidget::GetSubject` legge `GetInspectedUnit`, `BuildUnitCard` non ha controlli di conoscenza) continua a mostrare vita e scudo dal vivo di un nemico non più osservato, e l'arco del nemico punta alla cella vera. Un passo nel controller, `ReleaseInspectionUnlessLive(const FRTKnowledgeView&)`, chiamato dall'HUD nel giro del velo (`UpdateObserverVeil`, che ha già la vista), azzera soggetto e pin quando manca la voce `Live` o l'unità non è viva. Il pin orfano (un `RMB` consumato senza effetto visibile) sparisce con lo stesso passo | senza questo il Review Focus 2 sul pannello di sinistra è falso |
| **Q13** | ➕ rev. **Costi**: `GetHeroRoster()` e `FindEquipment` costruiscono cataloghi (istanze e `FText`) a ogni chiamata. La scheda si compone **a ogni cambio** (soggetto, vista, turno), non a ogni fotogramma, e il catalogo si tiene in una `static` come fa `RTBotPlanningLibrary.cpp` | churn di GC per fotogramma |

## Review Focus

1. **Due nemici che differiscono solo nel piano danno la stessa scheda**, carattere per carattere. Il piano, gli slot, l'azione armata e le ricariche vere non hanno un campo in cui entrare.
2. **Nemico non osservato = cella vuota** su scheda e su pannello di sinistra (Q12). Il click su un nemico velato non fissa nulla, e non lascia traccia nemmeno nel ritardo o nel tempo di risposta.
3. **`RMB` chiude davvero**: pannello, `InspectedUnit` e `bInspectorPinned`, e non rompe l'ordine di `ResolveBack`.
4. **Il playback non apre una porta al piano**: l'eccezione di `Inspect` non fa passare nessun altro click dalla guardia `IsWorldReadOnly`.
5. **Il kit non è l'unico dato nuovo**: il campo `EquippedItemIds` (Q2) non entra in hash, snapshot, TurnLog.
6. **L'ordine dei commenti**: i commenti sulla "fuga" del kit vanno corretti, ma la regola di #2757 sugli slot resta vera e deve restare scritta.

---

### Task 0: Preflight

- [ ] **Step 1: stato del worktree.** `git -C D:/Repositories/rt-3748-scheda status --short && git branch --show-current && git rev-parse --short HEAD`. Atteso: `issue/3748-scheda`, `a1bdadd0a`, solo questo piano.
- [ ] **Step 2: chi tiene il motore.** `Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, CommandLine`. Se qualcuno misura tempi, si aspetta.
- [ ] **Step 3: `main` è avanzato sugli stessi file?** `git fetch -q origin && git diff --stat HEAD origin/main -- Source/RefactorTactics/Player Source/RefactorTactics/UI Source/RefactorTactics/Ability`. Se sì, integrare prima di scrivere.

### Task 1: il click che ispeziona e il tasto destro che chiude (Q5, Q6, Q7)

**Files:** `Player/RTPointerInteraction.cpp` (`ResolveOutcome`), `Player/RTPlayerController.h/.cpp` (`DispatchUnitClick`, `ApplyBack`), `Tests/RTPointerInteractionTests.cpp`, `Tests/RTPlayerInteractionTests.cpp`.

- [ ] **Step 1: dichiarare** (letto, non da misurare) che `UndoStep` non ha guardie per `ResolutionPlayback`: il tasto destro arriva a `ApplyBack`. Le guardie sono `IsGameplayInputBlocked`, `IsPlanningInputInert` e il conto alla rovescia di Unready (Q10). Test esistenti da riscrivere: quello di `RTPointerInteractionTests.cpp` che richiede `Blocked` per un nemico osservato con carica armata in `ResolutionPlayback` diventa `Inspect`; il test del velo (`VeiledUnitIsIndistinguishableFromEmptyGround`) va esteso al playback.
- [ ] **Step 2: test rosso.** `Pointer.InspectInResolutionPlaybackForAnObservedEnemy`: tabella contesto × (unità propria, nemico osservato, nemico non osservato); atteso scritto a mano. In `ResolutionPlayback`: nemico osservato `Inspect`, proprio `Blocked`, non osservato `NoOp`; in `Modal` e `ReactionWindow` tutto `Blocked`. Anti-vacuità: la tabella produce `Inspect` almeno una volta.
- [ ] **Step 3: test rosso.** `PlayerInput.InspectingPinsAndRightClickCloses`: `DispatchUnitClickForTest` su un nemico osservato fissa `InspectedUnit` e `bInspectorPinned`; `ApplyBack` con `Inspector` azzera **entrambi** e non tocca la selezione.
- [ ] **Step 4: implementare**, test verdi. Aggiornare il commento di `OnSelect` e correggere o spiegare `HexMatch.PlaybackAllowsInspectAndRestoresPreview`.
- [ ] **Step 5: mutazione.** (a) `ApplyBack` non azzera `InspectedUnit`: rosso. (b) `ResolveOutcome` risponde `Inspect` anche per l'unità propria in playback: rosso.

### Task 2: il kit dal solo catalogo (Q1)

**Files:** `UI/RTHudViewModel.h/.cpp` (funzione solo-catalogo estratta), `Tests/RTActionTooltipTests.cpp`.

- [ ] **Step 1: misurare** i sette test del tooltip: tutti verdi prima di toccare.
- [ ] **Step 2: test rosso.** `HudViewModel.CatalogRowsNeedNoActor`: le righe (fase, slot, portata, forma, ricarica, danno) di un'azione si compongono da un `URTActionData` senza unità e coincidono con quelle che lo slot già mostra per la stessa azione. Anti-vacuità: un'azione con ricarica e una senza.
- [ ] **Step 3: estrarre** la parte fissa, far chiamare la nuova funzione a `BuildActionTooltip`. I sette test restano verdi.
- [ ] **Step 4: mutazione.** La funzione solo-catalogo ignora `CooldownTurns` o scrive la fase sbagliata: il test cade. (Una mutazione che le fa leggere lo stato di partita cambierebbe la firma: errore di compilazione, non rosso.)

### Task 3: soggetto, equipaggiamento e compositore (Q2, Q3) — serve la parola dell'autore su Q2

**Files:** `UI/RTInspect.h/.cpp` (nuovi), `Unit/RTUnit.h/.cpp` (solo se Q2 = campo), `UI/RTHUD.cpp/.h` (`BuildInspectSubject`), `Tests/RTInspectTests.cpp` (nuovo).

- [ ] **Step 1: misurare** chi chiama `EquipLoadout` e con quali id (setup di partita, bot, test), per dire se `DefaultLoadoutFor` basterebbe.
- [ ] **Step 2: attendere Q2.**
- [ ] **Step 3: test rosso, reflection.** `UI.InspectSubjectCarriesNoRawTypes`: allowlist di tipi, anti-vacuità con un campo vietato in un tipo di prova.
- [ ] **Step 4: test rosso, mondi gemelli.** `UI.InspectTwoEnemiesDifferingOnlyInThePlanReadTheSame`: due unità con lo stesso kit e lo stesso equipaggiamento, che differiscono in piano (azione armata, slot, waypoint) **e** in ricariche vere, danno la stessa `FRTInspectView` testuale, carattere per carattere. Il test passa dalla funzione che legge l'attore, non dal solo compositore (che non ha un campo per il piano e passerebbe comunque). Controllo positivo: due unità con kit diverso danno schede diverse.
- [ ] **Step 5: test rosso, equipaggiamento.** Il pezzo montato compare col nome del catalogo; un'unità senza equipaggiamento non scrive una riga vuota.
- [ ] **Step 6: implementare.** Se Q2 è il campo: `EquipLoadout` lo scrive, un test dimostra che l'hash dello stato di partita non cambia.
- [ ] **Step 7: mutazione.** La funzione che legge l'attore scrive nel testo `TurnsRemaining` o l'indice dell'azione pianificata: il gemello rosso. Il campo `TObjectPtr` nel soggetto: la reflection rossa. Verso e carta seguono la targa in playback (Q7).

### Task 4: il cancello, a mondi gemelli (Q4)

**Files:** `UI/RTInspect.h/.cpp` (`ResolveInspect`), `Tests/RTInspectTests.cpp`.

- [ ] **Step 1: test rosso.** `UI.InspectUnobservedEnemyReadsAsAnEmptyCell`: nemico senza voce `Live` → nessuna scheda e uscita uguale a quella di un mondo senza nemico; anche con voce `Remembered`.
- [ ] **Step 2: test rosso.** `UI.InspectNewbornUnitHasNoCardBeforeTheVeil`: `bKnownToObserver` ancora vero, nessuna voce `Live`: nessuna scheda.
- [ ] **Step 3: test rosso.** `UI.InspectClosesWhenTheSubjectLeavesTheView`: una voce `Live` che sparisce o un'unità caduta chiude la scheda.
- [ ] **Step 4: test rosso, il produttore della chiusura (Q12).** `PlayerInput.InspectionIsReleasedWhenTheSubjectIsNotLive`: dopo `ReleaseInspectionUnlessLive` con una vista senza voce `Live`, `InspectedUnit` è nullo e `bInspectorPinned` falso; con la voce `Live` restano. Un nemico caduto (KO, non ancora distrutto) rilascia. Il pannello di sinistra non ha più soggetto.
- [ ] **Step 5: implementare**, test verdi.
- [ ] **Step 6: mutazione, una per test.** Cancello sostituito da `IsKnownToObserver()`; cancello tolto; `ReleaseInspectionUnlessLive` che non azzera il pin; che non controlla `IsAlive`.

### Task 5: il pannello (Q8, Q9, Q10, Q11) — serve la parola dell'autore su Q8

**Files:** `UI/RTInspectCardWidget.h/.cpp` (nuovi) e il suo punto di montaggio, `Player/RTPlayerController.cpp` (`OnZoom`), `UI/RTHudLayout.*` se serve una posa, `Tests/RTHudScenarioTests.cpp`.

- [ ] **Step 1: misurato** (revisione): il registro nasce dal WBP; il ruling Q8 è riscritto.
- [ ] **Step 2: attendere Q8** (tre strade). Se (a), l'authoring è del clone principale e il resto del Task aspetta quell'asset.
- [ ] **Step 3: test rosso.** `ScreenHud.InspectCardStaysInsideTheRightBand`: aggiungere l'inquilino al blocco `Refresh*` di `TenantsFitTheirBands` con un soggetto già impostato nello scenario; mutazione: larghezza fissa in pixel, rosso su più canvas.
- [ ] **Step 4: test rosso.** `ScreenHud.InspectCardNeverTruncatesSilently`: più righe di quante ne entrino, l'ultima visibile dichiara le altre.
- [ ] **Step 5: test rosso.** `PlayerInput.WheelZoomsTheWorldOnlyInsideTheScene`: sulla funzione pura `ShouldZoomWorld`, vero dentro e falso fuori; il collegamento in `OnZoom` e il caso dei primi fotogrammi senza proiezione restano `NOT RUN` finché non sono visti in PIE.
- [ ] **Step 6: test rosso.** `ScreenHud.InspectCardHidesWithoutClosing`: in `Modal` e `ReactionWindow` il pannello è nascosto e `InspectedUnit` resta; alla chiusura torna.
- [ ] **Step 7: implementare**, test verdi. `git grep DrawDebug` sui file nuovi deve dare zero.
- [ ] **Step 8: mutazione.** `ShouldZoomWorld` che risponde sempre vero; `ShouldShowInspectCard` che ignora `Modal`.

### Task 6: i commenti e i documenti

- [ ] I commenti che chiamano fuga il kit di un'avversaria: `InspectedUnit` e `GetInspectedUnit`, `DispatchUnitClick`, `ResolveOutcome`. Dicono ora: kit ed equipaggiamento pubblici, piano e slot privati (regola di #2757 sugli slot).
- [ ] `spec-pointer-interaction.md` §5.3 (l'ispezione in playback ora funziona) e l'esito di `Pathing` (drift segnalato); `progettazione-hud.md` §6 e §18 (l'inquilino di `MiddleRight`); `docs/technical/test-manuali-pie.md`: voce `PIE-HUD-SCHEDA` ⏳ con una scena e una domanda; `capability-map.yaml` se lo richiede il radar (`node tools/radar/*.ts --check`, tutti).
- [ ] **Voce del Decision Log** (testo sotto), numero preso al merge dopo `git fetch` e controllo dei ref remoti.
- [ ] DoD di #3748 aggiornata sul commit della PR; la voce PIE resta ⏳ finché l'autore non giudica su tavole.

**Testo proposto della voce:** *Di un nemico osservato sono pubblici il kit dell'eroe e l'equipaggiamento; le ricariche si leggono solo come deduzione dagli eventi autorizzati. Il piano, gli slot del turno, l'azione armata e le ricariche reali restano privati. L'equipaggiamento resta pubblico anche quando diventerà una scelta. Supera, per il solo kit, il giudizio dei commenti elencati in §9.4 della spec; la regola di #2757 sugli slot resta.* Accettata dall'autore il 2026-10-20.

### Task 7: chiusura

- [ ] Compile PASS con `EXIT=0`. Famiglie: `Pointer`, `PlayerInput`, `UI`, `HudViewModel`, `ScreenHud`, `Inspect`, `Knowledge`, `Perception`, `Match` (scelte dai lettori del dato). **Suite intera** prima della revisione, in una finestra libera.
- [ ] Revisione indipendente (agente con diff e file); correzioni; rimisura sul commit dichiarato.
- [ ] PR verso `main`, con Compile/Tests/Determinism/Replay/Privacy/PIE/Packaged. Determinism e Replay `N/A` salvo il campo `EquippedItemIds`, che ha il suo test sull'hash.
