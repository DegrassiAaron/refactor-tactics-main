# Il box di destra mostra la skill sotto il cursore, e tre decisioni dell'autore — Piano di implementazione

> Per chi esegue: i passi sono caselle da spuntare. Ogni Task che tocca codice finisce con compilazione, test e mutazione; la review è di un agente che riceve diff e file, non le conclusioni di chi ha scritto.

**Goal:** (1) il box di destra (`URTInspectCardWidget`, zona `MiddleRight`) diventa una testata compatta più i dettagli della skill sotto il cursore; (2) la dichiarazione del piano (`Conferma`, `Invio`) non è più bloccata dalla barra in sola lettura; (3) la fissazione di un nemico cade a ogni turno nuovo.

**Spec:** `docs/superpowers/specs/2026-10-13-ispettore-del-mondo-design.md` §3 (regole 7, 13, 14). **Piano padre:** `2026-10-30-barra-in-sola-lettura.md`. **Issue:** #3748. **Branch:** `issue/3748-scheda-pannello`, PR #3951 (bozza). **Base di partenza:** `68efec14d`.

**Decisioni d'autore (2026-10-31, in sessione, dopo le tavole V1-V4, tutte «sì»):**
1. *«il box sulla destra invece di avere tutte le info, potrebbe avere le info sulla skill che si sta facendo hover con il mouse»*. Fra le tre forme proposte ha scelto: **testata compatta sempre visibile** (nome, PV e scudo, verso, stati, una riga coi NOMI dell'equipaggiamento) **più, con il cursore su uno slot, i dettagli di quella skill**; senza cursore, una riga di invito.
2. **`Conferma` e `Invio` funzionano sempre**, anche con la barra in sola lettura. Rovescia il ruling R10 del piano padre.
3. **La fissazione cade a ogni turno nuovo**: scheda e barra tornano alla propria unità.
4. **La stima delle ricariche viste va dentro i dettagli della skill sotto il cursore**; la sezione «Ricariche (stima da ciò che hai visto)» sparisce dal box.

## Global Constraints

- Quelli del piano padre restano: la barra e la scheda di un nemico portano **solo catalogo e fatti visti**, mai piano, slot del turno, azione armata, ricariche vere, tasti; un nemico non `Live` non ha scheda né barra; nessuno stato canonico, snapshot, hash, TurnLog o replay toccato; nessun `.uasset` modificato; nessun totale volatile nei testi; mutazione per ogni guardia; nessun merge senza l'ok dell'autore.
- I dettagli di una skill vengono dalle stesse righe di catalogo del tooltip dello slot (`BuildCatalogActionRow`, `BuildCatalogTooltip`): **nessuna seconda fonte** per descrizione, fase, slot, portata, forma, ricarica di catalogo, danno.
- La stima della ricarica compare **solo** per un'azione che la squadra ha visto usare e che ha una ricarica di catalogo (la regola di `URTInspectLibrary::MakeDeducedCooldown`); è dichiarata stima.
- La dichiarazione del piano non scrive il piano: resta bloccata solo dal mondo in sola lettura (playback, finestra di reazione, modale), come prima di #3748.
- Il blocco del piano della barra in sola lettura (tasti, generiche, slot, click su mappa e su unità, `M`, verso) NON cambia.

## Review Focus

1. **L'hover di uno slot della TUA barra non cambia il box del nemico**: l'hover vale solo se l'unità dello slot è l'unità della scheda. → Task 3.
2. **Uno slot vuoto, o un indice fuori kit, sotto il cursore**: nessun dettaglio, la riga di invito resta; mai un indice non valido letto. → Task 3.
3. **Nessuno stato privato nei dettagli**: né ricarica vera, né azione armata, né piano, né tasto. La stima c'è solo se vista usare. → Task 3.
4. **Equipaggiamento di una skill**: il pezzo che CONCEDE l'azione (`GrantedActionId`) e, per l'attacco base, il pezzo d'arma; un'azione senza pezzo non mostra righe di equipaggiamento. → Task 3.
5. **La fissazione cade esattamente a ogni turno nuovo, non prima né dopo**: non a metà turno, non in risoluzione (l'ispezione in playback resta, regola 5), e una fissazione fatta dopo l'inizio del turno nuovo resta. → Task 2.
6. **Con una descrizione lunga che va a capo, la testata non esce dal box** (la finestra di `MaxRighe` con i marcatori regge). → Task 3.

---

### Task 1: la dichiarazione del piano non è più bloccata dalla barra

**Files:**
- Modify: `Player/RTPlayerController.h/.cpp` (`ToggleTurnPlanDeclared` e i docstring di `IsPlanningWriteBlocked`/`WhyPlanningWriteIsBlocked`), `UI/RTScreenHudWidgets.cpp` (`URTPlanCommitWidget::RefreshButtons`)
- Test: `Tests/RTHexMatchIntegrationTests.cpp`

**Interfaces:**
- Consumes: `IsWorldReadOnly()`, `PercheIlMondoESoloLettura(Context)`, `IsDockReadOnly()` (base del widget).
- Produces: `ToggleTurnPlanDeclared` torna a guardare solo `IsWorldReadOnly()` (con il testo di `PercheIlMondoESoloLettura`); `RefreshButtons` imposta `ConfirmButton` come prima dell'ondata di fix (acceso se c'è un'unità e il mondo non è in sola lettura), e `Conferma` (testo, cornice, tick) segue quella stessa condizione.

- [ ] **Step 1: test rossi.** Nuovo `RefactorTactics.PlayerInput.DeclaringThePlanStaysAliveWhileTheBarIsReadOnly`: con un nemico fissato (barra in sola lettura) `ToggleTurnPlanDeclared()` dichiara il piano dell'unità selezionata e una seconda chiamata lo ritira; controllo positivo = stesso risultato senza fissazione; con il mondo in sola lettura (playback) resta rifiutato come prima. Nuovo `RefactorTactics.ScreenHud.ConfirmStaysEnabledWhileTheBarIsReadOnly`, al posto di `ConfirmIsDisabledWhileTheBarIsReadOnly` (che si toglie, dichiarandolo): acceso prima, con il nemico fissato, dopo `ApplyBack`. Dalla tabella di `PlayerInput.AReadOnlyBarRefusesEveryWayOfWritingThePlan` si toglie la riga `Invio`; le altre righe restano com'erano.
- [ ] **Step 2: implementa** i due ritorni indicati in Interfaces; aggiorna i docstring che elencano i siti bloccati (la dichiarazione non c'è più).
- [ ] **Step 3: verde** sui gruppi `PlayerInput`, `ScreenHud`, `HexMatch`. **Mutazione**: rimettere `IsPlanningWriteBlocked()` in `ToggleTurnPlanDeclared` fa cadere il primo test; rimettere `&& !IsDockReadOnly()` nel pulsante fa cadere il secondo.
- [ ] **Step 4: commit** `fix(3748): la dichiarazione del piano resta viva con la barra in sola lettura (decisione d'autore del 2026-10-31)`

### Task 2: la fissazione cade a ogni turno nuovo

**Files:**
- Modify: `Player/RTPlayerController.h/.cpp`
- Test: `Tests/RTHexMatchIntegrationTests.cpp` (stessa fixture dei test della barra: `B.PC`, `PinInspectionForTest`)

**Interfaces:**
- Consumes: `PinInspection`, `ApplyBack` (passo `Inspector`), `bDockFollowsInspection`, `EnsureTurnPresentationSubscriptions`.
- Produces: `void ARTPlayerController::ReleaseInspectionForNewTurn()` privata, che azzera `InspectedUnit`, `bInspectorPinned` e `bDockFollowsInspection` (lo stesso effetto del passo `Inspector` del Back), chiamata **una volta per ogni turno nuovo di pianificazione** da un aggancio esistente del `TurnManager` che l'implementatore sceglie fra i delegati di `Turn/RTTurnManager.h` (`OnResolvePlaybackFinished`, `OnTeamKnowledgeRefreshed(TurnNumber)`…), dichiarando nel report perché è quello giusto e che cosa succede nei percorsi senza playback.

- [ ] **Step 1: test rosso** `RefactorTactics.HexMatch.ThePinFallsWhenANewPlanningTurnStarts`: nemico fissato (barra in sola lettura) → si porta la partita al turno successivo nel modo in cui i test vicini fanno avanzare un turno → `!IsDockReadOnly()`, `GetInspectedUnit() == nullptr`, `GetDockUnit() == GetSelectedUnit()`; controllo positivo: **a metà turno** (nessun turno nuovo) la fissazione resta; **in risoluzione/playback** resta; una fissazione fatta DOPO l'inizio del turno nuovo resta. La selezione e l'azione armata non cambiano.
- [ ] **Step 2: implementa** `ReleaseInspectionForNewTurn` e l'aggancio; **Step 3: verde** (`HexMatch`, `PlayerInput`, `SkillHover`, `Reactions`). **Mutazione**: togliere la chiamata fa cadere il test; chiamarla a ogni commit (anziché a ogni turno nuovo) fa cadere il controllo «resta in playback».
- [ ] **Step 4: commit** `feat(3748): la fissazione cade a ogni turno nuovo (decisione d'autore del 2026-10-31)`

### Task 3: il box di destra: testata compatta e dettagli della skill sotto il cursore

**Files:**
- Modify: `UI/RTInspect.h/.cpp` (`FRTInspectSubject`, `Compose`), `UI/RTHUD.h/.cpp` (`BuildInspectSubject`, `BuildInspectCard`), `UI/RTInspectCardWidget.h/.cpp`
- Test: `Tests/RTInspectTests.cpp`, `Tests/RTInspectCardWidgetTests.cpp`, `Tests/RTSeenUseLedgerTests.cpp` (le righe «ricariche» si spostano nei dettagli)

**Interfaces:**
- Consumes: `ARTPlayerController::GetSkillHoverUnit()` e `GetSkillHoverIndex()` (già esistono), `URTHudViewModel::BuildCatalogActionRow(Action, Index, NAME_None)`, `BuildCatalogTooltip`/`ComposeTooltipText`, `FRTInspectEquipmentLine`, `FRTInspectDeducedCooldown`, `URTInspectLibrary::MakeDeducedCooldown`, `FRTEquipmentPiece::GrantedActionId`.
- Produces: `struct FRTInspectSkillDetail { FName ActionId; FText Title; TArray<FText> Lines; TArray<FRTInspectEquipmentLine> Equipment; TOptional<FRTInspectDeducedCooldown> Deduced; }` (solo valori, nessun puntatore), e il campo `FRTInspectSubject::HoveredSkill` (vuoto senza hover). `ARTHUD::BuildInspectSubject(..., int32 HoverAbilityIndex = INDEX_NONE)` e `ARTHUD::BuildInspectCard(..., int32 HoverAbilityIndex = INDEX_NONE)` lo riempiono dalle righe di catalogo dello slot indicato. `Compose(Subject)` emette: nome; PV e scudo; verso (se acceso); stati col glossario; **una sola riga** «Equipaggiamento: A · B · C» (solo nomi); poi, con `HoveredSkill`, il titolo e le righe della skill, le righe `+`/`-` dei pezzi che la riguardano, e la riga della stima se c'è; senza hover, una riga di invito («Passa su una skill della barra per i dettagli»). **Non** emette più la sezione «Ricariche».
- Regola dei pezzi che riguardano la skill: il pezzo con `GrantedActionId == ActionId`; per un'azione del gruppo `Base`, il pezzo d'arma (cerca il nome esatto dell'enum di slot in `Ability/RTEquipmentData.h`); nessun altro.
- Il widget legge l'hover a ogni `RefreshLook`: vale solo se `GetSkillHoverUnit() == l'unità della scheda`; un hover su una skill della TUA barra non lo cambia.

- [ ] **Step 1: test rossi.** `RefactorTactics.UI.InspectCardHeaderIsCompact`: senza hover la vista ha nome, PV, stati, la riga con i nomi dell'equipaggiamento e la riga di invito, e **nessuna** riga `+`/`-`, nessuna sezione «Ricariche», nessun titolo di skill. `RefactorTactics.UI.InspectCardShowsTheHoveredSkill`: con hover su una skill del catalogo compaiono titolo e righe uguali a quelle del tooltip dello stesso slot (confronto con `BuildCatalogTooltip`, controllo positivo), le righe `+`/`-` del pezzo che la concede (e per l'attacco base quelle dell'arma), nessuna per una skill senza pezzo; con indice vuoto o fuori kit resta la riga di invito. `RefactorTactics.UI.InspectCardHoveredSkillShowsTheDeducedCooldownOnlyIfSeen`: la riga della stima c'è solo con un avvistamento (ledger), e mai un contatore residuo. Riscrivi i test della sezione «Ricariche» perché verifichino la stessa cosa nei dettagli, senza indebolirli; il test per reflection `InspectSubjectCarriesNoRawTypes` copre il nuovo campo. Widget: `RefactorTactics.UI.InspectCardFollowsTheSkillHover`: `SetSkillHover(Nemico, k)` cambia le righe, `ClearSkillHover` le riporta alla testata, e `SetSkillHover(UnitaPropria, k)` non le cambia.
- [ ] **Step 2: implementa**; **Step 3: verde** (`UI.Inspect`, `ScreenHud`, `HudViewModel`, `SeenUse*`, `PlayerInput`, `SkillHover`, `Reactions`). `ScreenHud.TenantsFitTheirBands`, `InspectCardWearsItsLineClass` e `MiddleRightStackKeepsTheLogUnderTheCard` restano verdi senza toccare le asserzioni. **Mutazione**: dettagli di un'altra posizione di kit; il pezzo sbagliato per la skill; la sezione «Ricariche» rimessa; la stima mostrata senza avvistamento; un campo di stato vero nel soggetto (deve cadere la reflection); l'hover di un'unità diversa accettato.
- [ ] **Step 4: commit** `feat(3748): il box di destra mostra la testata e i dettagli della skill sotto il cursore (decisione d'autore del 2026-10-31)`

### Task 4: documenti e voce PIE

**Files:**
- Modify: `docs/decisions/RT_PDR_00_Decision_Log.md` (D-562), la spec dell'ispettore (§3 e le regole 7, 13, 14), `docs/technical/systems/progettazione-hud.md` §18.1, `docs/technical/runbooks/guida-screen-hud-umg.md` §7-octies, `docs/technical/test-manuali-pie.md` (`PIE-HUD-SCHEDA`), `docs/roadmap/editor-sessions.yaml` solo se `U95` nomina cose cambiate, i due piani.

- [ ] **Step 1: D-562**: le quattro decisioni d'autore del 2026-10-31, **come sue** (la voce 4 e il box sono sue parole; le tre domande aperte sono chiuse da lui con la raccomandazione del controller), rovesciano R10 (la dichiarazione resta viva; l'avvertenza sul `Invio` toggle resta come rischio noto), tolgono le domande aperte, e la sezione delle ricariche passa ai dettagli. Il numero resta quello misurato (libero su `origin/main`, ref e PR aperte alla creazione della PR #3951): **rimisuralo** prima di dichiararlo.
- [ ] **Step 2: spec e design**: nella spec le regole 13 e 14 (la dichiarazione non è fra i siti bloccati; la fissazione cade al turno nuovo), la riga dell'hover in §3, il box; `progettazione-hud.md` §18.1 e la guida UMG: che cosa mostra il box con e senza cursore, e che la sezione «Ricariche» non c'è più. Marcatura «➕ rev3» e «⌫» come il resto del file, nessun totale volatile.
- [ ] **Step 3: voce PIE**: criterio (a) riscritto (testata compatta), nuovo criterio per i dettagli sotto il cursore (con una skill con pezzo e una senza, e uno slot vuoto), (j) senza `Conferma`/`Invio` (ora vivi, da provare), (m) riscritto: «al turno nuovo la scheda si chiude e la barra torna alla tua unità» (**non provabile nel banco `SchedaBench`**: il lock-in resta a `Ready 1/2`; dichiaralo `NOT RUN`). La cella resta ⏳.
- [ ] **Step 4: radar** `doc-links`, `doc-coherence`, `doc-tables`, `issue-refs`, `decision-ids` con `--check` e `python -m unittest discover -s tools/editor-sessions -p "*_test.py"`, tutti exit 0, senza pipe; commit `docs(3748): ...`.

## Dopo i Task (del controller)

Suite intera sul commit finale; giro in finestra `-game` sul banco `PieSeduta.SchedaBench` per le tavole V5 (box con e senza cursore, con una skill con pezzo e una senza) e V6 (`Conferma`/`Invio` vivi con la barra in sola lettura); push sulla PR #3951; domanda all'autore su V5-V6; poi si toglie la bozza. Il criterio (m) resta `NOT RUN` finché non c'è una partita che risolve il turno.
