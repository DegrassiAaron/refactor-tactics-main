# La barra delle skill segue l'unità cliccata — Piano di implementazione

> Per chi esegue: i passi sono caselle da spuntare. Ogni Task che tocca codice finisce con compilazione, test e mutazione; la review è di un agente che riceve diff e file, non le conclusioni di chi ha scritto.

**Goal:** quando si fa click su un'unità che non si comanda (nemica), la barra delle skill mostra le skill di **quell'unità**, in sola lettura, e passando il mouse su uno slot compaiono la portata sulla mappa e il tooltip del catalogo. La scheda perde le righe di kit.

**Architecture:** il dock legge oggi la sola unità comandata. Un soggetto della barra, `ARTPlayerController::GetDockUnit()`, vale l'unità ispezionata e fissata **se non è comandabile** e il click più recente è stato un'ispezione; altrimenti vale la selezione. Per un soggetto in sola lettura il dock costruisce le righe da una vista **solo-catalogo** (`BuildPublicBar`), non da `BuildAbilityCooldowns`, che mescola catalogo e partita. L'hover dello slot passa dalla stessa `SetSkillHover` che già serve la barra propria e che la scheda ha già usato per l'unità a fuoco: la portata e il cancello `Live` non cambiano.

**Tech Stack:** UE 5.8.1, C++ (`Source/RefactorTactics/{Player,UI,Tests}`), nessun `.uasset` modificato se il grafo del dock passa da `GetActionsInReadingOrder()` e `GetArmedActionIndex()` (da provare in Task 6).

**Spec:** `docs/superpowers/specs/2026-10-13-ispettore-del-mondo-design.md` (§3 regole 4, 7, 9; §7). **Issue:** #3748. **Branch:** `issue/3748-scheda-pannello`, da `main` a `115d40cff`. **Piano padre:** `2026-10-20-scheda-ispezione.md` (Task 5 e la sua Q8).

**Decisione d'autore (2026-10-30, in sessione, dopo le tavole di `PIE-HUD-SCHEDA`):** *«se seleziono un'unità — quella che controllo, quella del compagno di squadra o quella avversaria — la barra delle skill che vedo è quella dell'unità selezionata; passo il mouse su una skill e vedo il raggio e altre informazioni.»* Risposte alle due domande poste: la barra **passa all'unità cliccata, in sola lettura** («niente tasti né armare finché non riselezioni una tua unità»); e il lavoro **resta dentro #3748**, dove la barra sostituisce le righe di kit della scheda. Cambia la regola 7 della spec («la barra a chi si comanda, la scheda a chi è a fuoco») e restringe la decisione del 2026-09-11 sulla convivenza di ispezione e selezione: **lo stato** convive ancora (selezione e azione armata non si perdono), cambia **che cosa mostra la barra**.

## Global Constraints

- Il kit e l'equipaggiamento di un nemico osservato sono **pubblici** (decisione d'autore del 2026-10-20). Restano privati piano, slot del turno, azione armata e ricariche vere. **La barra di un soggetto in sola lettura non porta nessuno di questi quattro**, e lo prova un test con un controllo positivo (la stessa unità, letta da `BuildAbilityCooldowns`, li porta).
- Un nemico non osservato (nessuna voce `Live` nella vista di conoscenza) non ha scheda **né barra**: stessa uscita di una cella vuota. Il cancello è `URTInspectLibrary::LiveEntry`; `ARTUnit::IsKnownToObserver()` nasce vero e non decide niente.
- «Compagno di squadra» è un'unità propria non selezionata: è comandabile (`URTCombatLibrary::CanPlayerControlUnitInGroup`), si seleziona e la barra funziona già. La sola lettura riguarda le unità **non comandabili**.
- `Ctrl` + click che mette a fuoco un'unità **propria** non è una selezione: non cambia la barra (spec §3 regola 2).
- Nessuna modifica a stato canonico, snapshot, hash, TurnLog, replay.
- Nessun `.uasset` modificato da un worktree: l'authoring appartiene al clone principale.
- Niente totali volatili nei testi: elenchi di nomi, o il comando che conta.
- Build in PowerShell, `"-Project=$P"`, `-WaitMutex -NoHotReloadFromIDE`; leggere `Result:` ed `EXIT=`. Suite con `tools/suite/esegui.py`, `--log` in un file; contare `Test Completed. Result={Success}` con `-CaseSensitive`, e `trovati == verdi + rossi`. Un test avviato e non completato non è un rosso: si cerca per nome.
- Build unity: nomi distinti nei namespace anonimi. Un nome di test **non** deve essere prefisso di un altro, o sparisce dalla suite.
- Mutazione per ogni test che prova una guardia: commit prima, applica, build, test, `git checkout -- <file>`, ricompila, rimisura. Un mutante che fa crashare il motore non è «ucciso».
- Nessun merge senza l'ok esplicito dell'autore; nessun push senza autorizzazione.

## Review Focus

Condizioni che la spec lascia implicite e che nessun test dei Task di sola costruzione prova da sé. Ognuna ha il suo test nel Task che possiede il codice.

1. **Click sull'unità propria già selezionata mentre una scheda è fissata**: la barra deve tornare alla selezione. `SelectUnit` esce subito se l'attore non cambia, e lì la barra resterebbe sul nemico. → Task 2.
2. **Niente stato nascosto nella barra del nemico**: né ricarica vera, né azione armata, né «pianificata», né tasto. → Task 1 e Task 4.
3. **Il nemico esce di vista, cade, o la scheda si chiude mentre uno slot è sotto il cursore**: la barra torna alla propria, e l'hover si spegne. → Task 2 e Task 4.
4. **Tasti numerici, generiche, click sullo slot, click sulla mappa, `M` (Sneak), `Invio`, verso**: non scrivono il piano mentre la barra è in sola lettura, e il log dice perché; `RMB` e il click su un'unità propria restano vivi. → Task 3.
5. **Due nemici in fila, e `Ctrl` + click su un'unità propria**: passando dal nemico A al nemico B uno slot costruito per A non arma né registra hover; il fuoco su un'unità propria non sposta la barra. → Task 2 e Task 4.

## Scelte di chi scrive (da correggere prima dell'esecuzione)

| # | Scelta | Perché | Se la si rovescia |
|---|---|---|---|
| S1 | La sola lettura blocca **tutti** i punti che scrivono il piano, **mappa compresa**; non blocca selezione, ispezione, `RMB`, camera | «Niente tasti né armare finché non riselezioni una tua unità»: un waypoint posato con la barra su un nemico è un ordine dato a un'unità che non si vede | Si toglie Task 3 per i click sulla mappa e resta il blocco dei soli tasti e slot |
| S2 | Gli slot della barra in sola lettura risolvono in `Available` (`bUsableNow = true`, niente ricarica): **non vuol dire «pronta»**, vuol dire «niente stato» | Un nuovo valore di `ERTActionSlotState` tocca ogni `switch` sugli stati; le tinte si cercano per stato e un valore senza tinta non colora | Nuovo stato `Inspected` e una riga per ogni `switch`; richiede authoring delle tinte |
| S3 | **Nessuna etichetta «sola lettura»** sulla barra in questo giro: il segnale è l'assenza dei tasti sugli slot. Va chiesto in PIE | Una porta nuova nel dock è authoring o un `Ensure*` da costruire, e non è chiesta | Follow-up: `EnsureSubjectBanner` sul modello di `EnsureLayerIndicator` |
| S4 | La scheda **perde** le righe di kit e con loro `URTInspectLineWidget` come produttore di hover, e le «usate viste» (`GetSeenUses`) se nessun altro le legge | La barra sostituisce le righe (risposta d'autore); un produttore senza righe non ha niente da produrre | Si tengono le righe e l'hover dalla riga: la scheda e la barra mostrano due volte il kit |
| S5 | Le «usate viste» non passano alla barra | Non erano chieste; la barra mostra il catalogo | Follow-up: un marcatore sullo slot |

## File

| File | Responsabilità |
|---|---|
| `UI/RTHudViewModel.h/.cpp` | `BuildPublicBar`: righe solo-catalogo per un soggetto in sola lettura |
| `Player/RTPlayerController.h/.cpp` | `GetDockUnit`, `IsDockReadOnly`, `IsPlanningWriteBlocked`, `WhyPlanningWriteIsBlocked`; il click più recente decide il soggetto |
| `UI/RTScreenHudWidgets.h/.cpp` | `URTScreenHudWidgetBase::GetDockUnit`; dock e slot leggono il soggetto della barra |
| `UI/RTInspect.h/.cpp`, `UI/RTInspectCardWidget.h/.cpp`, `UI/RTHUD.cpp` | la scheda senza kit né hover di riga |
| `Player/RTPlanningOverlay.h/.cpp` | il campo `bSkillHoverFromCard` diventa `bSkillHoverOfInspected` |
| `Tests/*` | un test per guardia, con il controllo positivo accanto |
| `docs/…` | decisione, spec, runbook, voce PIE |

---

### Task 1: `BuildPublicBar` — la barra di chi si osserva, senza la partita

**Files:**
- Modify: `UI/RTHudViewModel.h` (accanto a `BuildIdleBar`), `UI/RTHudViewModel.cpp`
- Test: `Tests/RTActionTooltipTests.cpp` o il file che già prova `BuildIdleBar` (`HudViewModel.IdleBarCarriesNothingOfThePlan`)

**Interfaces:**
- Consumes: `URTHudViewModel::BuildCatalogActionRow(const URTActionData*, int32 Index, FName ActiveVariantId)`, `URTHudViewModel::ResolveSlotState`.
- Produces: `static TArray<FRTAbilityCooldownView> URTHudViewModel::BuildPublicBar(const ARTUnit* Subject);` — una riga per posizione di kit, nell'ordine del kit, come `BuildAbilityCooldowns`. Vuota se `Subject` è nullo.

- [ ] **Step 1: test `HudViewModel.PublicBarCarriesTheCatalogAndNothingOfTheMatch`.** Su un'unità con kit, azione armata, `PlannedAbilityIndex = 1` e una ricarica vera sulla posizione 2: per ogni posizione `i`, `Row.AbilityIndex == i`; `ActionId`, `DisplayName`, `Description`, `RangeCells`, `Damage`, `Slot`, `Phase`, `PhaseMark`, `Group`, `Shape`, `AreaRadius`, `CooldownTurns` uguali a quelli di `BuildAbilityCooldowns(Unit)[i]`; `HotkeyLabel.IsEmpty()`, `!bPlanned`, `TurnsRemaining == 0`, `ChargeFraction == 1.f`, `!bPlanInvalid`, `!bPlanDegraded`, `!bTargetRefused`, `bUsableNow`, e `ResolveSlotState(Row, false) == Available` per una riga con azione; una posizione vuota ha `ActionId.IsNone()` e stato `Empty`. **Controllo positivo nello stesso test**: `BuildAbilityCooldowns(Unit)[1].bPlanned` e `[2].TurnsRemaining > 0`, altrimenti il test non prova niente. `BuildPublicBar(nullptr).IsEmpty()`.
- [ ] **Step 2: rosso.** Run: `python tools/suite/esegui.py HudViewModel.PublicBarCarriesTheCatalogAndNothingOfTheMatch --log <file>`. Atteso: non compila (simbolo mancante).
- [ ] **Step 3: implementa `BuildPublicBar`.** Elenco **positivo** dei campi, come `BuildIdleBar`: parte da `BuildCatalogActionRow(Action, Index, Unit->ActiveVariantId)`, svuota `HotkeyLabel`, pone `bUsableNow = true`. Nessuna lettura di `Unit->Planned*`, `GetAbilityCooldown`, `CanUseAbility`.
- [ ] **Step 4: verde**, stesso comando; tutte le righe del filtro `HudViewModel.` ancora verdi.
- [ ] **Step 5: mutazione** (commit prima): M1 copia `bPlanned` dal piano; M2 copia `HotkeyLabel` da `HotkeyLabelFor`; M3 scrive `TurnsRemaining` dal simulatore; M4 `bUsableNow = Unit->CanUseAbility(Index)`. Ciascuno deve far cadere il test, per nome.
- [ ] **Step 6: commit** `feat(3748): BuildPublicBar - la barra di un'unita' osservata porta il catalogo e niente della partita`

### Task 2: il soggetto della barra

**Files:**
- Modify: `Player/RTPlayerController.h/.cpp`
- Test: `Tests/RTHexMatchIntegrationTests.cpp` (stessa fixture dei test di fuoco: `B.PC`, `PinInspectionForTest`)

**Interfaces:**
- Consumes: `PinInspection`, `ApplyBack`, `ReleaseInspectionUnlessLive`, `SelectUnit`, `URTCombatLibrary::CanPlayerControlUnitInGroup(TeamId, ControlGroup, PlayerTeamId, PlayerControlGroup, bIsBotControlled)`.
- Produces: `const ARTUnit* ARTPlayerController::GetDockUnit() const;` e `bool ARTPlayerController::IsDockReadOnly() const;`. `IsDockReadOnly()` vale `bDockFollowsInspection && bInspectorPinned && InspectedUnit viva e non comandabile`. `GetDockUnit()` vale `InspectedUnit` se `IsDockReadOnly()`, altrimenti `GetSelectedUnit()`. Campo privato `bool bDockFollowsInspection = false`: **si accende** in `PinInspection(Unit != nullptr)`, **si spegne** come prima istruzione di `SelectUnit`, **prima** del ritorno anticipato per `Actor == SelectedActor`.

- [ ] **Step 1: test `HexMatch.TheDockFollowsTheLastClickOnAnUnitThatIsNotCommanded`.** Aevik selezionato, un nemico `Live`. (a) senza fissazione: `GetDockUnit() == Aevik`, `!IsDockReadOnly()`. (b) `PinInspectionForTest(Enemy)`: `GetDockUnit() == Enemy`, `IsDockReadOnly()`, e `GetSelectedUnit() == Aevik` ancora. (c) `SelectUnit(Aevik)` — l'unità **già** selezionata: `GetDockUnit() == Aevik`, `!IsDockReadOnly()`, e `bInspectorPinned` ancora vero con `InspectedUnit == Enemy` (la scheda resta). (d) rifissa il nemico; `ApplyBack()` passo `Inspector`: `!IsDockReadOnly()`. (e) fissa **Muiren** (propria, comandabile): `!IsDockReadOnly()` e `GetDockUnit() == Aevik`. (f) fissa il nemico poi `KO`: `GetDockUnit() == Aevik`. (g) fissa il nemico poi `ReleaseInspectionUnlessLive(vista senza la sua voce)`: `GetDockUnit() == Aevik`. (h) fissa un secondo nemico: `GetDockUnit()` è il secondo.
- [ ] **Step 2: rosso**; **Step 3: implementa**; **Step 4: verde**.
- [ ] **Step 5: mutazione**: M1 lo spegnimento dopo il ritorno anticipato di `SelectUnit` (deve cadere (c)); M2 senza `!comandabile` (cade (e)); M3 senza `IsAlive` (cade (f)); M4 senza `bInspectorPinned` (cade (d) o (g)); M5 `GetDockUnit()` che ignora il flag (cade (a)/(c)).
- [ ] **Step 6: commit** `feat(3748): il soggetto della barra e' l'ultima unita' cliccata, se non e' comandabile`

### Task 3: con la barra in sola lettura il piano non si scrive

**Files:**
- Modify: `Player/RTPlayerController.h/.cpp`
- Test: `Tests/RTHexMatchIntegrationTests.cpp` o il file dei test di input (`PlayerInput.`)

**Interfaces:**
- Consumes: `IsWorldReadOnly()`, `PercheIlMondoESoloLettura(Context)`, `IsDockReadOnly()` (Task 2).
- Produces: `bool ARTPlayerController::IsPlanningWriteBlocked() const` (`IsWorldReadOnly() || IsDockReadOnly()`) e `FString ARTPlayerController::WhyPlanningWriteIsBlocked() const` (vuota se non bloccato; per la sola lettura della barra nomina la causa e il rimedio: *«la barra mostra %s, che non comandi: chiudi la scheda con il tasto destro o seleziona una tua unità»*).

- [ ] **Step 1: test `PlayerInput.AReadOnlyBarRefusesEveryWayOfWritingThePlan`.** Tabella `{nome, gesto, sonda}`, eseguita due volte sulla stessa unità. **Fissato** il nemico: la sonda non cambia. **Dopo `ApplyBack`** (fissazione chiusa): la stessa sonda cambia (controllo positivo, altrimenti il test è vacuo). Righe: tasto di kit (`SelectAbilityForCurrent`) → `SelectedAbilityIndex`; porta dello slot (`ArmKitAbility`) → `SelectedAbilityIndex`; generica (`Z`/`G`/`B`/`C`/`X`) → `SelectedAbilityIndex`; click su una cella (`HandleClickOnCell`) → `PlannedWaypoints.Num()`; click su un'unità con un'azione armata prima della fissazione (`HandleClickOnUnit`) → `PlannedAttackTarget`; `ToggleSneakForTest` → `PlannedMovementProfileId`; dichiarazione del piano con `Invio` → `bPlanDeclared` (o il campo che il test esistente di D-468 legge); apertura del selettore del verso (`TryOpenFacingSelector`) → `bDeclaringFacing`. Altro test `PlayerInput.TheReasonNamesTheReadOnlyBar`: `WhyPlanningWriteIsBlocked()` contiene il nome del nemico e «tasto destro»; vuota senza fissazione.
- [ ] **Step 2: rosso**; **Step 3: sostituisci `IsWorldReadOnly()` con `IsPlanningWriteBlocked()`** nei soli punti che scrivono il piano — `Enter` (dichiarazione), `HandleClickOnUnit`, `HandleClickOnCell`, `SelectAbilityForCurrent`, il ramo `Sneak`, `TryOpenFacingSelector`, `HandleFacingClick`, `FacingHandlesClickOnFinalCell`, `UpdateFacingHoverFromRay` — e usa `WhyPlanningWriteIsBlocked()` al posto di `PercheIlMondoESoloLettura(...)` nelle righe di log. **Non** toccare: la telemetria di `OnSelect` (`!IsWorldReadOnly()` per i campioni di ritmo), `SelectUnit`, `DispatchUnitClick`, `ApplyBack`.
- [ ] **Step 4: verde**, più i test di D-468 invariati (`PlayerInput.` e `HexMatch.` che nominano `ReadOnly`).
- [ ] **Step 5: mutazione**: un mutante per ciascun punto sostituito (rimette `IsWorldReadOnly()`), per nome; ciascuno deve far cadere la riga di tabella che lo copre.
- [ ] **Step 6: commit** `feat(3748): con la barra in sola lettura il piano non si scrive, e il log dice perche'`

### Task 4: il dock e lo slot leggono il soggetto della barra

**Files:**
- Modify: `UI/RTScreenHudWidgets.h/.cpp` (`URTScreenHudWidgetBase`, `URTActionDockWidget`, `URTActionSlotWidget`)
- Test: `Tests/RTScreenHudWidgetTests.cpp`, `Tests/RTActionDockPortTests.cpp` (accanto a `FRTSlotHoverProducerTest`)

**Interfaces:**
- Consumes: `GetDockUnit()`, `IsDockReadOnly()`, `BuildPublicBar`, `OrderForReading`.
- Produces: `const ARTUnit* URTScreenHudWidgetBase::GetDockUnit() const` (come `GetSelectedUnit()`, via il controller). `URTActionDockWidget::GetActions()` e `GetActionsInReadingOrder()` leggono `BuildPublicBar(GetDockUnit())` quando la barra è in sola lettura; `GetArmedActionIndex()` vale `INDEX_NONE` in sola lettura; `GetMovementReadout()` vale la vista non autorizzata in sola lettura. `URTActionSlotWidget`: `BuiltForUnit = GetDockUnit()`; `Activate()` **rifiuta e logga** in sola lettura; `Activate()` e `HoverBegin()` confrontano `BuiltForUnit` con `GetDockUnit()`.

- [ ] **Step 1: test `ScreenHud.DockShowsTheBarOfAReadOnlySubject`.** Aevik con un'azione armata (`GetArmedActionIndex() == k`, controllo positivo prima della fissazione). Fissato il nemico: `GetActionsInReadingOrder()` ha gli `ActionId` di `OrderForReading(BuildPublicBar(Enemy))`; `GetArmedActionIndex() == INDEX_NONE`; `!GetMovementReadout().bAuthorized`; nessuna riga porta `HotkeyLabel`. Chiusa la fissazione: tutto com'era.
- [ ] **Step 2: test `ScreenHud.AReadOnlySlotNeitherArmsNorHoversTheWrongUnit`.** Slot costruito per il nemico: `Activate()` lascia `Aevik->SelectedAbilityIndex` invariato; `HoverBegin()` registra `GetSkillHoverUnit() == Enemy` e l'indice dello slot; `HoverEnd()` lo spegne. Slot costruito per il **nemico A** poi fissato il **nemico B**: `Activate()` e `HoverBegin()` non fanno niente (Review Focus 5). Controllo positivo: slot costruito per Aevik, non fissato nessuno: `Activate()` arma.
- [ ] **Step 3: test `HexMatch.TheSlotHoverOfAReadOnlyBarShowsTheRangeFromTheViewCell`**: lo scenario del test esistente «origine dalla cella della vista», con il **produttore** cambiato: lo slot invece di `SetSkillHover` a mano. Stesse asserzioni.
- [ ] **Step 4: rosso**; **Step 5: implementa**; **Step 6: verde**.
- [ ] **Step 7: mutazione**: `GetArmedActionIndex` che ignora la sola lettura; `Activate` senza rifiuto; `BuiltForUnit` da `GetSelectedUnit()`; `GetMovementReadout` non nascosto; `HoverBegin` che confronta con la selezione. Per nome.
- [ ] **Step 8: commit** `feat(3748): il dock mostra la barra dell'unita' cliccata, in sola lettura`

### Task 5: la scheda perde le righe di kit

**Files:**
- Modify: `UI/RTInspect.h/.cpp` (`Compose`, `FRTInspectSubject::KitAbilityIndices`, `FRTInspectView::LineAbilityIndices`), `UI/RTHUD.cpp` (`BuildInspectSubject`), `UI/RTInspectCardWidget.h/.cpp`, `Player/RTPlanningOverlay.h/.cpp`, `Player/RTPlayerController.cpp`
- Test: i file che nominano gli stessi simboli

**Interfaces:**
- Consumes: tutto quanto sopra.
- Produces: `Compose` non emette più la sezione «Kit»; resta carta, stati, verso, equipaggiamento. `bSkillHoverFromCard` → `bSkillHoverOfInspected` (stessa semantica: l'hover è di un'unità a fuoco non comandata). `URTInspectLineWidget` è solo una riga di testo.

- [ ] **Step 1: ricerca prima di togliere** (`Search → Reuse → Extend`): `git grep -n "KitAbilityIndices\|LineAbilityIndices\|GetSeenUses\|HoverBegin\|SetHoverControllerForTest\|bSkillHoverFromCard"` e leggere i lettori **non-test** di ciascuno. Si toglie solo ciò che non ha più un lettore; `GetSeenUses` resta se qualcuno lo legge ancora.
- [ ] **Step 2: riscrivi i test della scheda**: `InspectCard…` perdono le asserzioni sul kit e guadagnano `RefactorTactics.InspectCard.HasNoKitRows` (la vista non contiene nomi di abilità; l'equipaggiamento sì). Test rimossi perché il loro soggetto non esiste più: `InspectLinesCarryTheirAbilityIndex`, `RowsCarryTheirSkill`, `InspectLineHoverAsksTheRangeOfItsSkill`, `InspectCardRetiresTheHoverOfARowThatGoesAway`. Test **tenuti e rinominati** senza «Card» (il produttore ora è lo slot): `ACardRowHoverTakesTheMapForTheInspectedUnit`, `ARowOfTheCardOfARememberedEnemyShowsNoRange`. Vanno dichiarati nella PR, con il motivo.
- [ ] **Step 3: rosso → implementa → verde.** `ScreenHud.TenantsFitTheirBands` e `InspectCardWearsItsLineClass` restano verdi (nessun asset cambia).
- [ ] **Step 4: mutazione** sui test riscritti; per i rinominati, i mutanti R7 («`LiveEntry` ignora la visibilità») e S-flag che li avevano uccisi.
- [ ] **Step 5: commit** `refactor(3748): la scheda non mostra piu' il kit: lo mostra la barra`

### Task 6: documenti, voce PIE, giro finale

**Files:**
- Modify: `docs/decisions/RT_PDR_00_Decision_Log.md`, `docs/superpowers/specs/2026-10-13-ispettore-del-mondo-design.md`, `docs/technical/systems/progettazione-hud.md` (§18.1), `docs/technical/runbooks/guida-screen-hud-umg.md` (§7-octies), `docs/technical/test-manuali-pie.md` (`PIE-HUD-SCHEDA`), `docs/superpowers/plans/2026-10-20-scheda-ispezione.md` (rimando a questo piano nel blocco di stato del Task 5)

- [ ] **Step 1: voce del Decision Log.** «La barra delle skill segue l'ultima unità cliccata, in sola lettura se non è comandabile». Contiene: la decisione d'autore e la sua data; le due risposte; che cambia la regola 7 e restringe la decisione del 2026-09-11; S1–S5; il rimando alle tavole. **Il numero `D-nnn` si misura al momento di scrivere**, su `origin/main` **e** su tutti i ref remoti e le PR aperte, e si rimisura prima della PR: vale al merge, non alla misura.
- [ ] **Step 2: spec.** §3: la riga «Hover su una skill, nella barra o nella scheda» diventa «nella barra, della propria unità o di quella fissata»; la regola 7 riscritta; una regola nuova per il ciclo di vita del soggetto della barra (Task 2) e per il blocco del piano (Task 3), marcate «➕ rev3» con la prima stesura sbarrata (convenzione del file).
- [ ] **Step 3: voce PIE `PIE-HUD-SCHEDA`**: il criterio (g) diventa «passando su uno slot della barra di un nemico fissato compare la portata dalla cella in cui lo si vede» e se ne aggiungono: (i) la barra mostra le skill del nemico cliccato senza tasti; (j) tasto, slot o click sulla mappa non armano né pianificano e il log dice perché; (k) `RMB` o il click su una propria unità riporta la barra. La cella torna ⏳ (le tavole di prima valevano per un'altra interfaccia); `test-manuali-pie`, `editor-sessions` (`U95`) e i radar `doc-coherence`/`doc-tables`/`doc-links`/`issue-refs`/`decision-ids` coerenti.
- [ ] **Step 4: compilazione e suite intera** sul commit dichiarato, `HEAD` e albero invariati da inizio a fine; trovati = avviati = completati; rossi per nome.
- [ ] **Step 5: giro in finestra `-game`** sul banco `PieSeduta.SchedaBench` (`Saved/RTLab`, non versionato): barra di Branth in sola lettura, hover su due slot con portata, slot senza portata, tasto che non arma e riga di log, `RMB`, click su Aevik già selezionato. **Verifica che il grafo del dock non abbia bisogno di authoring** (la barra cambia da sola); se serve un `.uasset`, si ferma qui e si dice. Tavole in `docs/technical/evidence/3748/`, una domanda per tavola, la didascalia dentro il PNG.
- [ ] **Step 6: commit**, poi **si chiede all'autore** il verdetto sulle tavole e l'ok per push e PR verso `main`. Dopo il merge: DoD di #3748 in locale e su GitHub, `git branch -D`, `git remote prune origin`.

## Non misurato, e dichiarato

- Che il grafo di `WBP_RT_ActionDock` costruisca gli slot **solo** da `GetActionsInReadingOrder()` e `SetAction`: lettura del C++ e delle docstring, non del grafo. Si prova in Task 6, Step 5.
- La resa degli slot in `Available` per un soggetto in sola lettura (S2): giudizio d'autore su tavola.
- L'assenza di un'etichetta «sola lettura» (S3): giudizio d'autore su tavola.
- Il costo di `BuildPublicBar` a ogni tick del dock: una chiamata sul catalogo per slot, come `BuildCatalogActionRow` nella scheda. Da misurare in PIE prima di metterci una cache.
