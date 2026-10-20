# Lo sguardo all'hover - piano di implementazione, fetta F2

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** passando il mouse su un'unita' osservata, su una cella in vista, su una porta o un obiettivo *di una cella in vista* (P9: bordi e ghost sono fuori da F2), compaiono poche righe ancorate al bersaglio: la targa si apre sull'unita', un'etichetta compare sulla cella. Lo sguardo e' l'annotazione di `spec-pointer-interaction.md` 4.1: non cambia il bersaglio del click ne' l'esito dell'hover.

**Architecture:** quattro pezzi puri e uno di superficie. (1) un risolutore dell'annotazione, accanto a `URTPointerLibrary::ResolveTarget`; (2) un compositore che riceve un *soggetto pubblico* (mai l'attore) e restituisce righe di testo gia' pronte, dal glossario `RTGlossary`; (3) il cancello di conoscenza, una voce `Live` in `FRTKnowledgeView` per le unita' e `FRTTeamKnowledge::VisibleCells` per le celle; (4) il ritardo dell'hover come dato; (5) le superfici: righe nuove sulla targa (`FRTUnitOverlayView`, `URTUnitOverlayWidget`) e un widget nuovo per l'etichetta di cella, in C++ puro.

**Tech Stack:** Unreal Engine 5.8, C++ (`RefactorTactics`), Slate; Automation `RefactorTactics.<Famiglia>.<Nome>`.

**Spec:** `docs/superpowers/specs/2026-10-13-ispettore-del-mondo-design.md`, fetta `F2` (3 regola 1, 4, 5 P1/P2/P5, 7). **Issue:** #3747. **Branch:** `issue/3747-sguardo`, da `origin/main` = `3707f8b31`, nel worktree `D:/Repositories/rt-3747-sguardo`. **Decisioni:** D-512 (lo sguardo e' UI nel mondo) e D-513 (e' l'annotazione di 4.1), accettate.

**Che cosa NON e' in questo piano:** la scheda e il fissaggio (F3, #3748); le aree di movimento e la portata (F5, #3750); le ricariche dedotte (F6, #3751); l'anteprima del gesto (#3512); lo stato ricordato di una cella esplorata.

## Global Constraints

- **Il motore e' uno per macchina.** Prima di ogni build e di ogni run: leggere le `CommandLine` dei processi `UnrealEditor*` (`-abslog` e `.uproject` dicono sessione e clone). Le run headless passano `-abslog` nello scratchpad.
- **Authoring asset solo nel clone principale.** Questo piano non modifica nessun `.uasset`: la targa si estende in C++ (Ruling P3). Se il Task 7 scopre che serve un asset, si ferma e si chiede.
- **Chi corregge non firma.** La revisione di questo piano, e quella del codice, le fa un agente con diff e file, non con le mie conclusioni.
- **Niente `DrawDebug*`** (vuoto in Shipping). Niente totali volatili nei testi: elenchi di nomi, o il comando che conta.
- **Build in PowerShell**, `"-Project=$P"`, `-WaitMutex -NoHotReloadFromIDE`; leggere `Result:` ed `EXIT=` nello stesso comando. Suite headless con `-abslog`; contare `Test Completed. Result={Success}` con `-CaseSensitive`, e `trovati == verdi + rossi`.
- **Mutazione per ogni test che prova una guardia**, con ripristino in `finally` e ricompilazione; working tree committato prima della prima mutazione.
- **Nessun merge** senza l'ok esplicito dell'autore; nessun push senza autorizzazione.

## Stato misurato (2026-10-19, `3707f8b31`)

Citati per simbolo. Misurati da una ricognizione in sola lettura sul worktree.

| Fatto | Dove |
|---|---|
| `ARTPlayerController::PlayerTick` risolve solo una **cella** (`ResolveCellUnderCursor`) e chiama `ARTHexMapActor::SetHoveredCell`. Non costruisce `FRTPointerCandidates`, non conosce `bObserved` ne' `bCommandable` | `Player/RTPlayerController.cpp` |
| `FRTPointerCandidates` e `ResolveTarget` sono usati al click, nel ramo `Targeting`, con candidati quasi vuoti (`bHasCell` e `Cell`). `bCommandable` (`URTCombatLibrary::CanPlayerControlUnitInGroup`) e `bObserved` (`ARTUnit::IsKnownToObserver`) compaiono in piu' punti del controller, non solo in `DispatchUnitClick` (`grep -cE "CanPlayerControlUnitInGroup\|IsKnownToObserver\(\)" Player/RTPlayerController.cpp` > 1). ➕ rev. | idem |
| ➕ rev. **`bMapElement`, `bGhost` e `bHasEdge` non hanno nessun produttore fuori dai test**: compaiono solo nei rami di `ResolveTarget`. L'hover non sa trovare una porta, un obiettivo, un bordo o un ghost dal raggio: `grep -rnE "bMapElement\|bGhost\|bHasEdge" Source --include=*.cpp --include=*.h` esclusi `Tests/` e l'header | `Player/RTPointerInteraction.cpp` |
| ➕ rev. L'HUD usa **due** fonti di conoscenza: `KnowledgeForTeamPublic` (canonica; `CurrentRefusedShotLine`) e `PlaybackKnowledgeForTeam` (le unita'; in playback ricalcola le celle in vista dalle pose animate) | `UI/RTHUD.cpp` |
| ➕ rev. `FRTStatusBadgeView::Tag` e' un `FName`; `RTGlossary::KeyForStatus` vuole un `FGameplayTag` | `UI/RTHudViewModel.h`, `UI/RTGlossary.h` |
| ➕ rev. `ARTUnit::Facing` e' il verso *autorevole*, che a fine playback la mesh deve raggiungere: non e' misurato quando cambia rispetto al playback (come `Cell`, #3458) | `Unit/RTUnit.h` |
| ➕ rev. `GetPointerContext` non sembra produrre `ReactionWindow` (da misurare nel Task 5): l'inerzia in finestra di reazione potrebbe essere solo nominale | `Player/RTPlayerController.cpp` |
| ➕ rev. `BuildUnitOverlay` e' chiamata nel `Tick` della HUD, mentre il bersaglio arriva da `PlayerTick`: lo sguardo ha un fotogramma di ritardo, dichiarato | `UI/RTHUD.cpp` |
| **Non esiste un ritardo dell'hover** (grep su Dwell, HoverDelay, HoverTime: zero) | `Source/` |
| `FRTUnitOverlayView` ha `DisplayName`, `Card`, `Statuses`, `TeamColor`, `bFriendlyFire`, `bTargeted`; la costruisce `URTHudViewModel::BuildUnitOverlay`; la applica `ARTHUD::UpdateObserverVeil` ogni fotogramma | `UI/RTHudViewModel.h/.cpp`, `UI/RTHUD.cpp` |
| `URTUnitOverlayWidget` riempie in C++ `NameText` (nome, piu' `!` e `*` per fuoco amico e bersaglio) e `StatusBox` (caselle con `NewObject<UOverlay/UImage/UTextBlock>`); degli stati mostra icona e durata, **non il nome**. Il WBP esiste (`WBP_RT_UnitOverlay`) e carica da `ARTUnit` con `FClassFinder` | `UI/RTUnitOverlayWidget.*` |
| Glossario: `RTGlossary::NameFor`, `EffectFor`, `KeyForStatus`, `KeyForSurface`, `KeyForCover`. **Nessun tetto di lunghezza dell'effetto**: `GlossaryCheckText` controlla non vuoto, ASCII, nessuna cifra, nessun nome di tag, punto finale | `UI/RTGlossary.*`, `Tests/RTGlossaryTests.cpp` |
| `FRTKnowledgeEntry` ha `Visibility` (`Live` / `Remembered`); un'unita' ignota non ha voce. L'HUD costruisce la vista con `UvViewForObserver` (namespace anonimo in `RTHUD.cpp`); **il controller non ne ha una** | `Perception/RTKnowledgeView.h`, `UI/RTHUD.cpp` |
| `FRTTeamKnowledge::VisibleCells` ed `ExploredCells` esistono; l'HUD le legge in `CurrentRefusedShotLine` con `KnowledgeForTeamPublic` | `Perception/RTTeamKnowledge.h` |
| La cella: `FRTHexCellData` ha `Surface`, `TotalMoveCost()`, `bBlocksLineOfSight`, `bIsObjective`, `CoverOn(Edge)`, `DoorOn(Edge)`; si prende con `URTHexMapAsset::FindCell`. Il piano e' `FRTCellId::Layer`. **Lo stato di runtime di porte e obiettivo non ha un accessor sull'actor**: le porte si leggono dall'asset (`URTHexDoorLibrary`) | `Map/RTHexCellData.h`, `Map/RTHexMapAsset.h` |
| `ARTUnit::Facing` e' il verso autorevole; **non c'e' un accessor "verso pubblico"** e la targa non lo mostra | `Unit/RTUnit.h` |
| `ARTUnit::bKnownToObserver` nasce `true`; l'unico produttore e' `ARTHUD::UpdateObserverVeil` (`ShouldDrawUnitOverlay`). Il cancello di P2 e' la voce `Live`, non questo flag | `Unit/RTUnit.h`, `UI/RTHUD.cpp` |
| Modelli di test: `Pointer.OutcomeSeparatesInspectFromSelect`, `Pointer.VeiledUnitIsIndistinguishableFromEmptyGround` (esito), `Debug.ContextViewCarriesNoRawTypes` (reflection con allowlist e anti-vacuita'), helper `RTTestReflection::ReflectedNames` / `Listed`, `ScreenHud.TenantsFitTheirBands` | `Tests/RTPointerInteractionTests.cpp`, `RTContextInspectorTests.cpp`, `RTReflectedFieldsForTest.h` |
| Widget in C++ puro con posa come dato: `URTContextInspectorWidgetBase`, `URTPieVerdictOverlay` (`RebuildWidget`, `Placement()`) | `Debug/`, `PieSession/` |
| Proiezione: `AHUD::Project` in `DrawHUD` con `HexCellWorld` e `WorldHeadOffset`; il fermo al viewport e' `ARTHUD::ClampOverlayAnchor`; rettangolo della scena `RTHudLayout::SceneRectPixels` | `UI/RTHUD.*`, `UI/RTHudLayout.h` |

## Rulings

Scelte di chi scrive. Quelle marcate **(A)** chiedono la parola dell'autore *prima* del Task indicato; le altre sono revocabili.

| | Ruling | Costo se e' sbagliato |
|---|---|---|
| **P1** | Il risolutore e' una funzione pura *nuova*, `URTPointerLibrary::ResolveGlanceTarget`, accanto a `ResolveTarget`. Non modifica `ResolveTarget`, che resta del click | due funzioni con un ordine simile: il test del Task 1 pinna la differenza |
| **P2** | Il compositore riceve due strutture di **valori** (`FRTGlanceUnitSubject`, `FRTGlanceCellSubject`) e restituisce `FRTGlanceView` (righe `FString`). Il codice che legge l'attore e quello che legge la mappa sono due funzioni sottili, *fuori* dal compositore | un campo grezzo che entra: lo prende il test per reflection |
| **P3** | La targa si estende **in C++**: `URTUnitOverlayWidget::SetOverlayView` costruisce le righe dello sguardo come gia' fa per `StatusBox`. Nessun `.uasset` cambia | la resa non segue le tavole finche' non arriva la pelle (come R9 della spec) |
| **P4 (A)** | Il **tetto di lunghezza di un effetto** lo lascio al Task 3, *dopo* aver misurato la voce piu' lunga del glossario: il tetto e' quel massimo arrotondato per eccesso, con un gate. Un testo piu' lungo non si accorcia io: e' lavoro d'autore | tetto troppo stretto: costringe a riscrivere testi che l'autore ha gia' accettato. Per questo chiedo prima il valore |
| **P5** | Il **verso** si mostra col nome della direzione dell'enum (`ERTHexDirection`), non con un testo d'autore: il glossario non copre le direzioni | il testo e' provvisorio e va giudicato a tavola |
| **P6** | L'**obiettivo** e la **porta** di una cella in vista: nome e stato presi dall'asset (`FRTHexCellData::bIsObjective`, `DoorOn`). Lo stato di *runtime* di una porta aperta/chiusa non ha un accessor sull'actor: se il Task 5 non lo trova, lo sguardo dice solo il nome e dichiara `NOT RUN` sullo stato | una porta aperta letta come chiusa: e' un difetto visibile, non una fuga |
| **P7 (A)** | Il **ritardo** parte da 0,3 secondi (R10 della spec), campo `EditDefaultsOnly` sul controller o sulla HUD, valore ritarato in PIE dall'autore | ritardo sbagliato: etichette che lampeggiano, o sguardo che sembra rotto |
| **P8** | ➕ rev. La **vista di conoscenza** per il cancello la costruisce l'HUD (ha gia' `UvViewForObserver`); il controller passa all'HUD il *candidato grezzo* (`StableUnitId` dell'unita' colpita, se c'e', e la cella): **non** decide se l'unita' e' osservata, e non usa `IsKnownToObserver()`, che nasce vero. Il risolutore quindi non prende `bUnitObserved` | se il controller decidesse, riaprirebbe la finestra di nascita |
| **P9** | ➕ rev. **Da dove vengono i candidati dell'hover.** L'hover ha gia' l'unita' colpita (`GetHitResultUnderCursor` in `ResolveCellUnderCursor`) e la cella. **Porta e obiettivo si ricavano dalla cella** (`FRTHexCellData::DoorOn`, `bIsObjective`), non da una mesh: `bMapElement`, `bGhost` e `bHasEdge` non hanno produttore e questo piano non ne scrive uno. Lo sguardo su un *bordo* o su un *ghost* e' fuori da F2: i ripari per lato stanno nell'etichetta della cella. Il Goal "porta o obiettivo" resta, nella forma "porta o obiettivo di una cella in vista" | un sottoinsieme dei bersagli di 4.1: dichiarato nella PR, e a #3747 |
| **P10** | ➕ rev. **Dove avviene il degrado unita' -> cella.** Se il candidato e' un'unita' *senza* voce `Live`, lo fa l'HUD: lo sguardo cade sulla cella di quell'unita', e vale solo se la cella e' in vista. Un nemico velato su una cella in vista da' cosi' la stessa etichetta di una cella vuota; fuori vista, nulla, come una cella vuota | se il degrado tacesse (`nullopt`), un nemico velato darebbe *meno* di una cella vuota: un canale |
| **P11** | ➕ rev. **Fonte della conoscenza delle celle.** Unita' e celle leggono la **stessa** fonte, `PlaybackKnowledgeForTeam`, cosi' in `ResolutionPlayback` una cella non si dice in vista mentre l'unita' sopra di lei non lo e'. `CurrentRefusedShotLine` (canonica) non e' il modello | in playback le due fonti divergono: lo prova un test |
| **P12** | ➕ rev. Il **verso** non si mostra in `ResolutionPlayback` finche' non e' misurato quando `Facing` cambia rispetto alla scena (A5). Se la misura dice che e' coerente con la mesh, si accende | un verso in meno in playback: dichiarato |

## Review Focus

1. **Nemico non osservato = cella vuota, carattere per carattere.** Il caso peggiore e' l'unita' che il raggio tocca ma che il velo nasconde: lo sguardo deve cadere sulla cella sotto, non su una versione "vuota" dell'unita', e **non deve sparire** dove una cella vuota parlerebbe (P10): un `nullopt` e' un canale. ➕ rev.
2. **Il flag `bKnownToObserver` nasce acceso.** Nella finestra fra la nascita e il primo `UpdateObserverVeil` il cancello giusto (voce `Live`) deve tacere. E' il gemello in piu' di P2.
3. **Una cella esplorata e non in vista** deve dare la stessa uscita di una mai vista: nulla. Lo stato "di adesso" di una cella fuori vista e' la fuga che P5 chiude.
4. **Lo sguardo non cambia l'esito dell'hover** ne' il bersaglio del click. Qualunque funzione nuova che tocca `SetHoveredCell` o il percorso di `DispatchUnitClick` e' un difetto di questo piano.
5. **Inerte** in `ReactionWindow`, in `Modal`, e quando nessun giocatore comanda.
6. **Il taglio non e' muto.** Se gli stati non entrano nelle righe, la riga dice che ce ne sono altri.
7. **Un percorso condiviso**: la targa e' ricomposta ogni fotogramma per ogni unita'. Le righe dello sguardo si compongono solo per l'unita' sotto il cursore, mai per tutte.

---

### Task 0: Preflight

- [ ] **Step 1: stato del worktree.** `git -C D:/Repositories/rt-3747-sguardo status --short && git branch --show-current && git rev-parse --short HEAD`. Atteso: `issue/3747-sguardo`, `3707f8b31`, solo questo piano.
- [ ] **Step 2: chi tiene il motore.** `Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, Name, CommandLine`. Se qualcuno misura tempi, si aspetta.
- [ ] **Step 3: `main` e' avanzato sugli stessi file?** `git fetch -q origin && git diff --stat HEAD origin/main -- Source/RefactorTactics/Player Source/RefactorTactics/UI Source/RefactorTactics/Perception`. Se si', integrare prima di scrivere.

### Task 1: il risolutore dell'annotazione

**Files:** `Player/RTPointerInteraction.h/.cpp` (aggiunta), `Tests/RTPointerInteractionTests.cpp` (aggiunta).

**Interfaces:** produce `enum class ERTGlanceKind { None, Unit, MapElement, Cell }`, `struct FRTGlanceTarget { Kind; StableUnitId (int32, INDEX_NONE se assente); Cell; bHasCell }` e `URTPointerLibrary::ResolveGlanceTarget(int32 HitStableUnitId, bool bHasCell, const FRTCellId& Cell, bool bCellHasMapElement)`. Senza il contesto: lo sguardo ha un ordine solo (spec regola 1); quando si mostra e' di `ShouldShowGlance` (Task 5). Porta e obiettivo sono un genere solo, `MapElement`: il compositore li distingue leggendo la cella. **Fatto** (commit del Task 1). ➕ rev. Non prende `bUnitObserved` (P8) e non prende `FRTPointerCandidates`, che per l'hover non ha produttori (P9).

L'ordine e' quello della riga `IdleSelection` / `Planning` di 4.1 (spec regola 1), nel sottoinsieme che l'hover sa produrre (P9): unita' (comandabile o nota; chi la decide e' l'HUD, P10), poi oggetto logico od obiettivo della cella, poi cella.

- [ ] **Step 1: misurare** come `PlayerTick` / `ResolveCellUnderCursor` ottengono l'unita' colpita e la cella, e confermare che nessun produttore di `bMapElement`, `bGhost`, `bHasEdge` esiste (il comando di "Stato misurato"). Se esiste, rivedere P9.
- [ ] **Step 2: test rosso.** `RefactorTactics.Pointer.GlanceFollowsTheAnnotationOrder`: una tabella di combinazioni x contesti, con l'atteso scritto a mano dalla tabella 4.1 (non derivato dalla funzione). Anti-vacuita': la tabella contiene almeno una combinazione per ogni posizione dell'ordine.
- [ ] **Step 3: implementare**, test verde.
- [ ] **Step 4: mutazione.** Invertire due posizioni dell'ordine: rosso. Ripristinare e ricompilare.
- [ ] **Step 5: il degrado di un'unita' non osservata non e' di questa funzione** (P10): il test che lo prova e' al Task 4.

### Task 2: il soggetto pubblico e il compositore

**Files:** `UI/RTGlance.h/.cpp` (nuovi), `Tests/RTGlanceTests.cpp` (nuovo).

**Interfaces:** `FRTGlanceUnitSubject` (nome, `FRTUnitCardView`, stati `FRTStatusBadgeView`, verso `ERTHexDirection`, cella, squadra), `FRTGlanceCellSubject` (cella, `Surface`, costo d'ingresso, piano, ripari per lato, `bBlocksLineOfSight`, obiettivo, porte), `FRTGlanceView` (`TArray<FString> Lines`, `bTruncated`), `URTGlanceLibrary::ComposeUnit` e `ComposeCell`. Funzioni pure, `static`, senza viewport.

- [ ] **Step 1: test rosso per reflection.** `RefactorTactics.UI.GlanceViewCarriesNoRawTypes`: sul modello di `Debug.ContextViewCarriesNoRawTypes`, allowlist di tipi ammessi nei tre struct, anti-vacuita' con un campo vietato inserito a mano in un tipo di prova. Nessun `TObjectPtr`, `AActor`, `FGameplayTag` grezzo.
- [ ] **Step 2: test rosso.** `Glance.UnitLinesNameEveryStatus`: ogni stato del soggetto compare col nome del glossario e col suo effetto; il verso compare. ➕ rev. **La chiave:** `FRTStatusBadgeView::Tag` e' un `FName`, `KeyForStatus` vuole un `FGameplayTag`. Prima di scrivere, misurare come l'HUD passa da uno all'altro (`FGameplayTag::RequestGameplayTag` su `Tag`, oppure una funzione del glossario che prenda il nome) e usare quella strada; se manca, aggiungere in `RTGlossary` un'overload su `FName`, con il suo test. Il soggetto resta senza `FGameplayTag` grezzo.
- [ ] **Step 3: test rosso.** `Glance.CellLinesFollowSection4`: superficie con effetto; costo d'ingresso **solo se diverso da 1**; piano; ripari per lato; "blocca la vista"; obiettivo.
- [ ] **Step 4: test rosso.** `Glance.NeverTruncatesSilently`: con piu' stati di quanti ne entrino nelle righe, il primo gruppo mantiene l'ordine della targa (`BuildStatusBadges`) e l'ultima riga dichiara che ce ne sono altri.
- [ ] **Step 5: test rosso.** ➕ rev. `Glance.FitsTheDeclaredLineBudget`: sul caso peggiore (unita' con ogni stato, cella con superficie, ripari su tutti i lati, obiettivo e porta) il numero di righe non supera il massimo dichiarato dalla spec (quattro per lo sguardo di un'unita'; per la cella il massimo e' una scelta, da scrivere nel test e nel commento). Senza, il compositore passerebbe con piu' righe di quante l'etichetta regga.
- [ ] **Step 6: verso (P12).** Il soggetto porta `bShowFacing`; il compositore lo scrive solo se vero. Il test `Glance.FacingIsOffInResolutionPlayback` fissa il valore finche' non e' misurato.
- [ ] **Step 7: implementare** il minimo, test verdi.
- [ ] **Step 8: mutazione.** Sopprimere la dichiarazione del taglio: `NeverTruncatesSilently` rosso. Aggiungere un campo `TObjectPtr` al soggetto: la reflection rossa. Aggiungere una riga oltre il massimo: `FitsTheDeclaredLineBudget` rosso.

### Task 3: il tetto di lunghezza di un effetto (P4, serve la parola dell'autore)

**Files:** `Tests/RTGlossaryTests.cpp`, `UI/RTGlossary.h`.

- [ ] **Step 1: misurare** la voce piu' lunga di `RTGlossary::Effects()`, con un comando riproducibile; riportare il numero all'autore con il valore proposto del tetto.
- [ ] **Step 2: attendere il valore.** Senza, il task non procede. La DoD di #3747 lo lascia aperto *per scelta*.
- [ ] **Step 3: gate.** Estendere `GlossaryCheckText` con il tetto; anti-vacuita': una voce di prova piu' lunga lo fa cadere.
- [ ] **Step 4: mutazione.** Abbassare il tetto di un carattere sotto il massimo: rosso con il nome della voce.

### Task 4: il cancello di conoscenza, a mondi gemelli

**Files:** `UI/RTHUD.cpp/.h` (due funzioni sottili che ricavano il soggetto), `Tests/RTGlanceTests.cpp`.

➕ rev. Il cancello vive in **una** funzione pura, `URTGlanceLibrary::ResolveGlanceSubject(const FRTKnowledgeView& View, const FRTTeamKnowledge& Knowledge, const FRTGlanceTarget& Target)`, che fa il degrado di P10 e restituisce il soggetto pubblico o nulla. I test passano **da qui**, non dai soli costruttori di soggetto: il difetto (un `nullopt` dove ci vuole una cella) vive nel degrado. Due funzioni sottili fuori dalla pura leggono attore e mappa: `ARTHUD::BuildGlanceUnitSubject` e `BuildGlanceCellSubject`. Entrambe leggono `PlaybackKnowledgeForTeam` (P11).

- [ ] **Step 1: test rosso, gemelli P2.** `Glance.UnobservedEnemyReadsAsAnEmptyCell`: due mondi con la stessa conoscenza autorizzata e un nemico nascosto presente o no sulla cella; **cella in vista** *e* **cella fuori vista**: stessa `FRTGlanceView` in ognuna, **carattere per carattere**, e uguale a quella del mondo senza nemico.
- [ ] **Step 2: test rosso, gemelli P5.** `Glance.ExploredCellReadsAsNeverSeen`: cella esplorata non in vista con superficie A contro mai vista con superficie B: uscita identica e vuota.
- [ ] **Step 3: test rosso, finestra di nascita.** `Glance.NewbornUnitSpeaksOnlyAfterTheVeil`: unita' con `bKnownToObserver` ancora vero ma senza voce `Live`: lo sguardo e' quello della sua cella (P10), non quello dell'unita'.
- [ ] **Step 4: test rosso, playback.** ➕ rev. `Glance.CellAndUnitAgreeDuringPlayback`: con una conoscenza di playback in cui le celle in vista differiscono da quelle canoniche, una cella dichiarata in vista dal playback da' lo sguardo e una dichiarata fuori vista no, in accordo con l'unita' sopra di lei.
- [ ] **Step 5: implementare**, test verdi.
- [ ] **Step 6: mutazione, una per test.** (a) cancello sulla `Live` sostituito da `IsKnownToObserver()`: il test della finestra di nascita rosso. (b) `VisibleCells` sostituito da `ExploredCells`: il gemello P5 rosso. (c) cancello tolto dai nemici: il gemello P2 rosso. (d) degrado sostituito da `nullopt`: il gemello P2 su cella in vista rosso. (e) fonte delle celle riportata alla canonica: `CellAndUnitAgreeDuringPlayback` rosso.

### Task 5: il ritardo, l'inerzia e il collegamento

**Files:** `Player/RTPlayerController.h/.cpp`, `UI/RTHUD.h/.cpp`, `Tests/RTPointerInteractionTests.cpp`.

- [ ] **Step 1: ritardo come dato e stato del dwell** (P7). ➕ rev. Un piccolo stato puro `FRTGlanceDwell { Target; Elapsed }` con `Advance(const FRTGlanceTarget&, float Dt)`: **si azzera** quando cambia il bersaglio (unita' diversa, cella diversa, tipo diverso) e quando il cursore esce dal rettangolo della scena (`IsCursorInsideScene`). Test `Pointer.GlanceDwellRestartsWhenTheTargetChanges`: passare da un'unita' all'altra non mostra lo sguardo subito; mutazione: togliere il reset, rosso. Campo del ritardo `UPROPERTY(EditDefaultsOnly)`, default 0,3 s.
- [ ] **Step 2: l'inerzia.** Funzione pura `ShouldShowGlance(ERTPointerContext, bool bSomeoneCommands, bool bDwellElapsed)`. Inerte in `Modal` e in `ReactionWindow`, e quando nessuno comanda; **acceso** in `IdleSelection`, `Planning`, `Targeting`, `Pathing`, `ResolutionPlayback`. `Facing`: l'annotazione di 4.1 per quel contesto e' un settore e non un'unita': **inerte**, dichiarato. `bSomeoneCommands` viene da `URTHudViewModel::ResolveObserverTeamIds` (regola 12 della spec: "stesso insieme di squadre del registro"), non da un solo `PlayerTeamId`. ➕ rev. Misurare se `GetPointerContext` produce davvero `ReactionWindow`; se no, l'inerzia in finestra di reazione e' provata solo sulla funzione pura e va scritto cosi' nella PR.
- [ ] **Step 3: il punto di chiamata.** ➕ rev. Un test che esercita il percorso reale, non la funzione isolata: con un controller di prova in `Modal`, il bersaglio passato all'HUD e' `None`; in `Planning` con dwell trascorso e' quello atteso. Mutazione: bypassare `ShouldShowGlance` nel punto di chiamata, rosso.
- [ ] **Step 4: in `PlayerTick`** calcolare il candidato (unita' colpita, cella) e passare all'HUD il `FRTGlanceTarget` (P8, P9). Non toccare `SetHoveredCell` ne' `DispatchUnitClick`.
- [ ] **Step 5: test rosso, sul percorso reale.** ➕ rev. `Pointer.GlanceDoesNotChangeTheHoverOutcomeNorTheClickTarget`: la versione precedente (due funzioni pure senza ingresso dallo sguardo) era vera anche col difetto. Questa usa un `ARTHexMapActor` di prova e il controller: dopo aver attivato lo sguardo su una cella e su un'unita', `GetHoveredCell`, `IsHoveredCellValid`, `InspectedUnit` e la selezione sono uguali a prima, in ogni contesto; `ResolveOutcome` per lo stesso ingresso e' uguale. Mutazione: far scrivere allo sguardo `SetHoveredCell` o `InspectedUnit`, rosso.
- [ ] **Step 6: porta/obiettivo** (P6): misurare se lo stato di runtime e' leggibile (quale asset legge `GetHexContext`); se no, annotarlo nel commento della PR come `NOT RUN` sullo stato.

### Task 6: le superfici

**Files:** `UI/RTHudViewModel.h/.cpp` (`FRTUnitOverlayView` guadagna `GlanceLines`), `UI/RTUnitOverlayWidget.cpp`, `UI/RTCellGlanceWidget.h/.cpp` (nuovo), `UI/RTHUD.cpp`, `Tests/RTHudScenarioTests.cpp`.

- [ ] **Step 1: targa.** `BuildUnitOverlay` resta com'e' per le altre unita'; solo per quella sotto il cursore (dopo il ritardo) riempie `GlanceLines` dal compositore. `SetOverlayView` costruisce le righe in C++ (P3).
- [ ] **Step 2: etichetta di cella.** Un widget unico che si sposta, `RebuildWidget` come `URTPieVerdictOverlay`, posa come dato; ancorato alla cella con `Project(HexCellWorld(...))` e tenuto dentro il rettangolo della scena con `ClampOverlayAnchor` e `RTHudLayout::SceneRectPixels`.
- [ ] **Step 3: test rosso.** `ScreenHud.CellGlanceStaysInsideTheSceneRect` su piu' canvas (come `RTHudLabStepLineTests`: 1914x1048, 1280x720, 2560x1440, 3840x2160 e un 16:10); mutazione con ancoraggio fisso in pixel.
- [ ] **Step 4: nessuna resa da `DrawDebug*`**: `git grep DrawDebug` sui file nuovi deve dare zero.

### Task 7: documenti e voce PIE

- [ ] `spec-pointer-interaction.md` 4.1: lo sguardo nominato come annotazione. `progettazione-hud.md` 6 e 18: l'inquilino di `MiddleRight` (rimando alla scheda, F3) e l'inspector del bersaglio. ➕ rev. Dichiarati `DEFERRED` a F3: i commenti che chiamano fuga il kit di un'avversaria (§9.4 della spec). `docs/technical/test-manuali-pie.md`: voce nuova (una scena, una domanda). `capability-map.yaml` se lo richiede il radar (`node tools/radar/*.ts --check`, tutti).
- [ ] Aggiornare la DoD di #3747 sul commit della PR; la voce PIE resta `NOT RUN` finche' l'autore non giudica su tavole.

### Task 8: chiusura

- [ ] Compile PASS con `EXIT=0`. Famiglie: `Pointer`, `UI`, `HudViewModel`, `ScreenHud`, `Glance`, `Knowledge`, `Perception` (scelte dai lettori del dato). **Suite intera** prima della revisione, in una finestra concordata con le altre sessioni.
- [ ] Revisione indipendente (agente con diff e file); correzioni; rimisura sul commit dichiarato.
- [ ] PR verso `main`, descrizione con Compile/Tests/Determinism/Replay/Privacy/PIE/Packaged. Determinism e Replay `N/A`: nessuno stato canonico cambia; lo prova un test sull'hash dello stato di partita.
