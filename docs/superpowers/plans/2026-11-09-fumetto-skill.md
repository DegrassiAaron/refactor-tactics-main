# Il fumetto della skill — piano di implementazione (fetta 1, #4025)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** quando il playback mostra l'attivazione di una skill firma del kit, sopra l'unità compare per 3 s di tempo del playback un fumetto a grido in stile manga con il nome della skill, dentro l'overlay che già c'è.

**Architecture:** un canale in più nell'overlay dell'unità, modellato sul token di danno per il cablaggio (`ARTUnit` scrive un campo `Transient` sempre, il widget se c'è) e sulle cue di playback per il tempo (il TurnManager porta `PlaybackElapsedTotal` al widget; il widget non misura nulla). Tre funzioni pure in `URTHudViewModel` tengono il giudizio fuori dal disegno: chi grida, cosa dice, quanto è visibile.

**Tech Stack:** Unreal Engine 5.8, C++ (`RefactorTactics` module), UMG (`WBP_RT_UnitOverlay`), Automation Tests (`EditorContext | EngineFilter`), PIE.

**Spec:** `docs/superpowers/specs/2026-11-08-fumetto-skill-design.md` (A1–A8, R1–R11, §4 aggancio, §5 aspetto, §7 verifica). Issue: #4025. Branch: `feat/4025-fumetto-skill`, PR verso `main`.

## Global Constraints

- Solo presentazione: nessun campo nuovo in `MapState`, snapshot, TurnLog, `StateHash`; nessun golden cambia (R10, §6).
- Nessun secondo giudizio di visibilità: il velo lo decide `ARTHUD::UpdateObserverVeil` spegnendo l'overlay (R9; `RTUnit.cpp:326-329`).
- Il tempo del fumetto è **solo** `ARTTurnManager::PlaybackElapsedTotal` (R4, A8); nessun `DeltaTime` locale per la sua vita.
- Grida solo se `BaseActionId.IsNone()` **e** `ActionId` non è nel catalogo core (R1b, A7). Nessun campo nuovo in `FRTActionDef`.
- Durata 3,0 s (`EditDefaultsOnly`, `0` spegne); pop 0,1 s da scala 1,15; dissolvenza negli ultimi 0,4 s (R4, §5).
- Nomi `UPROPERTY` nuovi su `ARTUnit` **non** `BlueprintReadOnly`: `RefactorTactics.Unit.BlueprintSurfaceIsCensused` (`Tests/RTUnitBlueprintSurfaceTests.cpp:316`) censisce ogni campo esposto ai Blueprint, e un tell su un'unità velata è il difetto che quel test esiste per fermare.
- `WBP_RT_UnitOverlay.uasset` si modifica **solo in Editor, dal clone principale** (CLAUDE.md §10); il C++ resta completo senza i nuovi elementi (`BindWidgetOptional`).
- Unity build: nomi unici per file nei namespace anonimi dei test (`// ⚠️ Nomi distinti per file`).
- Motore uno per macchina: prima di build, suite o PIE leggi `Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, CommandLine`; headless con `-abslog` nello scratchpad di sessione (AGENTS.md §11).
- Commit `<type>(<scope>): <descrizione> (#4025)` con `Co-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>`. Ogni `gh` che scrive con `GH_TOKEN="$(gh auth token --user DegrassiAaron)"` e `--body-file`. Nessun totale volatile in issue, PR, commenti, documenti.

**Comandi.** Build (Editor chiuso):
```powershell
& "D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development -project="D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" -waitmutex
```
atteso `Result: Succeeded`. Suite (filtro al posto di `<F>`):
```powershell
& "D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" "-ExecCmds=Automation RunTests <F>;Quit" -unattended -nopause -nosplash -nullrhi -NoLiveCoding -log -abslog="<scratchpad>\<F>.log"
```
atteso: ogni test del filtro con `Result={Success}` nel log; un test assente dal log è `NOT RUN`, non verde.

## Review Focus

1. **Stessa unità, due skill entro 3 s** (Dash + Blast): il secondo grido sostituisce il primo e riparte (R5). Test in Task 2 (`SecondCalloutReplacesTheFirst`).
2. **Turno nuovo con `PlaybackElapsedTotal` azzerato** mentre un fumetto del turno prima fosse ancora registrato: `Now − Start` negativo. R7 lo taglia a fine playback; la funzione d'alpha risponde 1 a un elapsed negativo e il test lo pinna, così un taglio mancato si vede come fumetto che non muore (Task 1 `AlphaBeforeStartIsFull`, Task 3 `ClearedWhenPlaybackEnds`).
3. **Unità eliminata durante il playback** (`Defeated`): il TurnManager tiene puntatori deboli e salta i non validi; `DestroyDefeatedUnits` gira in `ConcludeTurn`, dopo il playback. Test in Task 3 (`SkipClearsEveryCallout` esercita il percorso di pulizia; la guardia `IsValid` è nel passo di codice).
4. **Nome del kit assente o di soli spazi**: fallback all'ultimo segmento dell'id con `bFromFallback` (R3). Test in Task 1 (`FallsBackToTheLastIdSegment`).
5. **Vista strategica attivata a fumetto vivo e disattivata prima dei 3 s**: collassato nel mezzo, ricompare dopo (R8). Non misurabile headless (serve il widget): è il punto (e) della PIE in Task 5, con l'aggiunta «torna in tattica entro i 3 s e il fumetto ricompare».

---

### Task 1: le tre funzioni pure e la vista del fumetto

**Files:**
- Create: `Source/RefactorTactics/UI/RTSkillCalloutView.h`
- Modify: `Source/RefactorTactics/UI/RTHudViewModel.h` (accanto a `BuildHealToken`, `:2047`), `Source/RefactorTactics/UI/RTHudViewModel.cpp` (accanto a `BuildDamageToken`, `:1394`)
- Test: `Source/RefactorTactics/Tests/RTSkillCalloutTests.cpp` (nuovo)

**Interfaces:**
- Produces:
  ```cpp
  USTRUCT(BlueprintType) struct FRTSkillCalloutView {
      int32 SourceStableUnitId = 0;  // 0 = nessuno (D-063)
      FName ActionId;                // None = nessun fumetto registrato
      FString Label;                 // già composto, maiuscolo lo fa il widget
      bool bFromFallback = false;    // R3 scattata
      float StartAt = 0.f;           // PlaybackElapsedTotal alla comparsa
  };
  static bool  URTHudViewModel::ActionShouts(const FName& ActionId, const FName& BaseActionId, bool bIsCoreAction);
  static FRTSkillCalloutView URTHudViewModel::BuildSkillCallout(const FText& KitDisplayName, const FName& ActionId, int32 SourceStableUnitId, float StartAt);
  static float URTHudViewModel::SkillCalloutAlpha(float Elapsed, float Seconds, float FadeSeconds = 0.4f);
  static float URTHudViewModel::SkillCalloutScale(float Elapsed, float PopSeconds = 0.1f); // 1.15 → 1.0
  ```

- [ ] **Step 1: scrivi i test che falliscono** in `Tests/RTSkillCalloutTests.cpp`, flag `EditorContext | EngineFilter`, nomi `RefactorTactics.HudViewModel.SkillCallout.*`:
  - `ShoutsOnlyForASignatureSkill`: `ActionShouts("Hero.Muiren.TideGuard", NAME_None, false)` → true; `ActionShouts("Hero.Branth.ImpactShot", "Action.BasicAttack", false)` → false; `ActionShouts("Action.Guard", NAME_None, true)` → false; `ActionShouts("Hero.X.Variant", "Action.Overwatch", false)` → false (Ruling R1b).
  - `LabelComesFromTheKit`: `BuildSkillCallout(FText::FromString("Marea circolare"), "Hero.Muiren.CircularTide", 3, 1.25f)` → `Label == "Marea circolare"`, `bFromFallback == false`, `SourceStableUnitId == 3`, `StartAt == 1.25f`, `ActionId == "Hero.Muiren.CircularTide"`.
  - `FallsBackToTheLastIdSegment`: con `FText::GetEmpty()` → `Label == "CircularTide"`, `bFromFallback == true`; con `FText::FromString("   ")` → stesso esito.
  - `AlphaIsFullThenFadesThenEnds`: `SkillCalloutAlpha(0.f, 3.f)` → 1; `(2.6f, 3.f)` → 1; `(2.8f, 3.f)` → 0.5 (tolleranza 0.01); `(3.f, 3.f)` → 0; `(4.f, 3.f)` → 0; `(1.f, 0.f)` → 0; `(1.f, -1.f)` → 0.
  - `AlphaBeforeStartIsFull`: `SkillCalloutAlpha(-0.5f, 3.f)` → 1.
  - `ScalePopsThenSettles`: `SkillCalloutScale(0.f)` → 1.15; `(0.05f)` → 1.075 (tolleranza 0.01); `(0.1f)` → 1; `(2.f)` → 1; `(-1.f)` → 1.15.

- [ ] **Step 2: build e run per vedere i test fallire**: la build fallisce per simboli mancanti, che qui equivale al rosso atteso. Annota l'errore.

- [ ] **Step 3: implementa** la struct in `RTSkillCalloutView.h` (include guard `#pragma once`, `RTSkillCalloutView.generated.h`, `UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")` su ogni campo, doc-comment che dichiara «solo presentazione, nessun hash») e le quattro statiche in `URTHudViewModel`. `ActionShouts` è `BaseActionId.IsNone() && !bIsCoreAction`. `BuildSkillCallout`: `Label` = `KitDisplayName.ToString()` se `!IsEmptyOrWhitespace()`, altrimenti il testo dopo l'ultimo `.` di `ActionId.ToString()` (tutto l'id se non c'è un punto) e `bFromFallback = true`. `SkillCalloutAlpha`: `Seconds <= 0` → 0; `Elapsed < 0` → 1; `Elapsed >= Seconds` → 0; nella coda `[Seconds − FadeSeconds, Seconds)` lineare da 1 a 0; altrove 1. `SkillCalloutScale`: `PopSeconds <= 0` → 1; `Elapsed <= 0` → 1.15; lineare fino a 1 a `PopSeconds`; oltre 1. Include `UI/RTSkillCalloutView.h` in `RTHudViewModel.h` accanto a `RTDamageTokenView.h` (`:12`).

- [ ] **Step 4: build `Result: Succeeded`, suite `RefactorTactics.HudViewModel.SkillCallout`** → sei test `Result={Success}`.

- [ ] **Step 5: commit** `feat(hud): le funzioni pure del fumetto della skill e la sua vista (#4025)`.

---

### Task 2: il canale sull'unità e sul widget

**Files:**
- Modify: `Source/RefactorTactics/Unit/RTUnit.h` (accanto a `ShowHealToken`/`LastDamageToken`, `:2138-2164`), `Source/RefactorTactics/Unit/RTUnit.cpp` (accanto a `ShowHealToken`, `:333-346`; `SetStrategicPresentation`, `:505`)
- Modify: `Source/RefactorTactics/UI/RTUnitOverlayWidget.h` (binding accanto a `DamageTokenText`, `:204`; proprietà accanto a `DamageTokenSeconds`, `:208`), `Source/RefactorTactics/UI/RTUnitOverlayWidget.cpp`
- Test: `Source/RefactorTactics/Tests/RTSkillCalloutTests.cpp` (stesso file, sezione «unità»)

**Interfaces:**
- Consumes: Task 1.
- Produces, su `ARTUnit`:
  ```cpp
  void  ShowSkillCallout(const FName& ActionId, float StartAt);  // compone con KitDisplayNameFor(ActionId), scrive SEMPRE il campo, widget se c'è
  void  SetSkillCalloutClock(float Now);                         // porta il tempo al widget; registra LastSkillCalloutClock
  void  ClearSkillCallout();                                     // bSkillCalloutLive = false, widget collassato; LastSkillCallout RESTA (oracolo)
  bool  IsSkillCalloutLive() const;
  const FRTSkillCalloutView& LastSkillCalloutForTest() const;
  float SkillCalloutClockForTest() const;
  // UPROPERTY(Transient) FRTSkillCalloutView LastSkillCallout;  NON BlueprintReadOnly (vedi Global Constraints)
  ```
  su `URTUnitOverlayWidget`:
  ```cpp
  void PushSkillCallout(const FRTSkillCalloutView& Callout);  // posa Label in maiuscolo, root visibile, Now = StartAt
  void SetSkillCalloutClock(float Now);                       // Alpha/Scale dalle pure di Task 1 → RenderOpacity/RenderScale del root; Alpha <= 0 → Collapsed
  void ClearSkillCallout();                                   // root Collapsed
  void SetStrategicPresentation(bool bStrategic);             // collassa finché vero; al ritorno riapplica l'ultimo Now
  UPROPERTY(EditDefaultsOnly) float SkillCalloutSeconds = 3.f;
  UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget>    SkillCalloutRoot;
  UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SkillCalloutText;
  ```

- [ ] **Step 1: scrivi i test che falliscono**, nello stesso file, con una fixture locale che spawna un `ARTUnit` configurato da `URTHeroCatalogLibrary::MakeMuiren()` in un `RTWorldFixtures::MakeWorld()` (modello `SpawnBeatUnit`, `Tests/RTPlaybackActivationTests.cpp:51-61`):
  - `UnitRecordsTheCalloutWithoutAWidget`: `ShowSkillCallout("Hero.Muiren.TideGuard", 2.f)` → `LastSkillCalloutForTest().ActionId == "Hero.Muiren.TideGuard"`, `.Label` non vuota e `!bFromFallback` (il kit di Muiren ha il nome), `.StartAt == 2.f`, `IsSkillCalloutLive()` true.
  - `SecondCalloutReplacesTheFirst`: due `ShowSkillCallout` (TideGuard a 2.f, poi CircularTide a 2.5f) → `ActionId` della seconda, `StartAt == 2.5f`.
  - `ClearKeepsTheRecordButEndsTheLife`: dopo `ShowSkillCallout` poi `ClearSkillCallout()` → `IsSkillCalloutLive()` false, `LastSkillCalloutForTest().ActionId` ancora valorizzato.
  - `ClockIsRecorded`: `SetSkillCalloutClock(4.2f)` → `SkillCalloutClockForTest() == 4.2f`.

- [ ] **Step 2: build → rosso per simboli mancanti.**

- [ ] **Step 3: implementa su `ARTUnit`** seguendo la forma di `ShowHealToken` (`RTUnit.cpp:333-346`): il campo si scrive sempre, il widget si aggiorna dentro `if (URTUnitOverlayWidget* Overlay = Cast<…>(GetOverlayWidgetObject()))`, **nessun controllo sul velo** (commento che rimanda a `:326-329`). In `SetStrategicPresentation`, dopo `bStrategicPresentation = bStrategic;` (`:514`), inoltra al widget. Doc-comment su `LastSkillCallout`: `Transient`, fuori da ogni hash, «esiste per rendere il cablaggio osservabile senza rendering» come `LastDamageToken` (`RTUnit.h:2150-2164`).

- [ ] **Step 4: implementa sul widget.** Nessun `DeltaTime`: `NativeTick` non tocca il fumetto. Stato privato: `FRTSkillCalloutView ActiveCallout; float CalloutNow = 0.f; bool bCalloutLive = false; bool bStrategic = false;`. Una sola funzione privata `ApplySkillCalloutState()` decide visibilità (`Collapsed` se `!bCalloutLive || bStrategic || Alpha <= 0`, altrimenti `HitTestInvisible`), `SetRenderOpacity(Alpha)` e `SetRenderScale(FVector2D(S, S))` sul root; `PushSkillCallout`, `SetSkillCalloutClock`, `ClearSkillCallout`, `SetStrategicPresentation` la chiamano. `Label` in maiuscolo con `ToUpper()` al momento di posare il testo (§5). Ogni binding opzionale: senza `SkillCalloutRoot` il widget non fa nulla e non crasha.

- [ ] **Step 5: build `Result: Succeeded`; suite `RefactorTactics.HudViewModel.SkillCallout`** → dieci `Result={Success}`; suite `RefactorTactics.Unit` → `BlueprintSurfaceIsCensused` ancora verde (il campo nuovo non è esposto).

- [ ] **Step 6: commit** `feat(unit): il canale del fumetto sull'unità e sull'overlay, senza misurare il tempo (#4025)`.

---

### Task 3: il TurnManager accende, porta il tempo, spegne

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.h` (dichiarazioni accanto a `ShowActivation`, `:2815`; stato accanto a `ActivationsShown`, `:3951`), `Source/RefactorTactics/Turn/RTTurnManager.cpp` (`ShowActivation`, `:9039-9049`; `TickPlayback` dopo `PlaybackElapsedTotal += Dt`, `:9614`; `FinishPlayback` dopo il ramo `IsResolutionSuspended()`, cioè dopo `:10105`, prima o accanto a `PlaybackLastShownAction = NAME_None`)
- Test: `Source/RefactorTactics/Tests/RTSkillCalloutPlaybackTests.cpp` (nuovo; fixture copiata da `FRTBeatDiProva`/`CostruisciBeat`/`FinoAllaFineDelPlayback`, `Tests/RTPlaybackActivationTests.cpp:42-116`, con nomi locali diversi)

**Interfaces:**
- Consumes: `ARTUnit::ShowSkillCallout / SetSkillCalloutClock / ClearSkillCallout / IsSkillCalloutLive / LastSkillCalloutForTest / SkillCalloutClockForTest` (Task 2); `URTHudViewModel::ActionShouts` (Task 1); `URTCatalogLibrary::FindCoreAction` (`Ability/RTCatalogLibrary.h:512`, ritorna `FRTActionDef()` con `ActionId` None se assente).
- Produces: `TArray<TWeakObjectPtr<ARTUnit>> SkillCalloutUnits;` (privato) e `int32 SkillCalloutUnitsForTest() const`.

- [ ] **Step 1: scrivi i test che falliscono**, `RefactorTactics.Playback.SkillCallout.*`. La fixture ha Scudo (Muiren, `Hero.Muiren.TideGuard` in Prep: skill firma) e Tiratore (Branth, indice 0 = `Hero.Branth.ImpactShot`: attacco base) della squadra 0, un bersaglio della squadra 1, viewer della squadra data.
  - `SignatureSkillShoutsAndBaseAttackDoesNot` (viewer 0): dopo `LockInAndResolve()` e la corsa a fine playback, `Scudo->LastSkillCalloutForTest().ActionId == "Hero.Muiren.TideGuard"`, `Tiratore->LastSkillCalloutForTest().ActionId.IsNone()`.
  - `LiveDuringPlaybackClearedWhenPlaybackEnds` (viewer 0): tick di 0,05 s uno alla volta finché `Scudo->IsSkillCalloutLive()` diventa vero (entro 400 tick, altrimenti fallisce); in quell'istante `Scudo->LastSkillCalloutForTest().StartAt` è `<= TM->PlaybackElapsedTotalForTest()` (accessor nuovo, accanto a `PlaybackPhaseElapsedForTest`, `RTTurnManager.h:740`) e `Scudo->SkillCalloutClockForTest()` è `>= StartAt`; poi corsa a fine playback → `IsSkillCalloutLive()` falso, `TM->SkillCalloutUnitsForTest() == 0`, e `ActionId` ancora valorizzato.
  - `HiddenSourceDoesNotShout` (viewer 7): a fine playback `Scudo->LastSkillCalloutForTest().ActionId.IsNone()` (il filtro `SourceVerdict` di `BeginPlayback` non la fa arrivare a `ShowActivation`).
  - `SkipClearsEveryCallout` (viewer 0): tick finché `Scudo->IsSkillCalloutLive()`, poi `TM->SkipPlayback()` (`RTTurnManager.h:1053`) e corsa a fine → falso, lista vuota.
  - `PausedClockDoesNotAdvanceTheCallout` (viewer 0): tick finché live, leggi `SkillCalloutClockForTest()`, `TM->PausePlayback()` (`RTTurnManager.h:859`, come `Tests/RTPlaybackControlsTests.cpp:54`), tick 10 volte → il clock dell'unità è **uguale** a prima; `TM->ResumePlayback()` (`:863`), tick 1 → maggiore.

- [ ] **Step 2: build → rosso.**

- [ ] **Step 3: implementa.** In `ShowActivation`, dopo `PlayPresentationRole(Cast, …)`: `const bool bIsCore = !URTCatalogLibrary::FindCoreAction(Ev.ActionId).ActionId.IsNone(); if (URTHudViewModel::ActionShouts(Ev.ActionId, Ev.BaseActionId, bIsCore)) { Src->ShowSkillCallout(Ev.ActionId, PlaybackElapsedTotal); SkillCalloutUnits.AddUnique(Src); }`. In `TickPlayback`, subito dopo `PlaybackElapsedTotal += Dt` (`:9614`) e dopo la correzione dello step (`:9624`): per ogni elemento valido di `SkillCalloutUnits`, `SetSkillCalloutClock(PlaybackElapsedTotal)`; gli invalidi si rimuovono. In `FinishPlayback`, **dopo** il `return` del ramo sospeso: per ogni elemento valido `ClearSkillCallout()`, poi `SkillCalloutUnits.Reset()`; commento che cita R7 e il perché della posizione (il prefisso di una finestra non è la fine del turno). Il tick in pausa esce prima (`:9576`), quindi il clock non viene spinto: è la regola, non un caso da gestire.

- [ ] **Step 3b: misura R11.** Leggi `ApplyInterrupts` in `Turn/RTTurnManager_Blast.cpp` (commenti a `:228` e `:263`) e il sito `RTTurnManager.cpp:7394`: un intento interrotto emette comunque `AbilityActivated`? Scrivi la risposta, con `file:riga`, in un commento sopra la chiamata a `ShowSkillCallout` e nella DoD della issue (`gh issue edit 4025 --body-file`). Nessun codice cambia in questa fetta qualunque sia la risposta (spec R11).

- [ ] **Step 4: build `Result: Succeeded`; suite `RefactorTactics.Playback`** → i cinque nuovi test verdi e nessun rosso fra quelli esistenti; `FindMissingBindings` (`Turn/RTPresentationBinding.cpp:273`): se il gate pretende un consumatore registrato per `AbilityActivated`, aggiungi `ShowSkillCallout` alla lista di `:276` e rilancia.

- [ ] **Step 5: commit** `feat(turn): il playback accende il fumetto di una skill firma, gli porta l'orologio e lo spegne a fine turno (#4025)`.

---

### Task 4: l'asset `WBP_RT_UnitOverlay` e la misura del componente

**Files:**
- Modify (Editor, clone principale): `Content/RT/UI/Match/WBP_RT_UnitOverlay.uasset`
- Modify (se serve): `Source/RefactorTactics/Unit/RTUnit.cpp:204` (`SetDrawSize(220, 90)`)
- Test esistente da tenere verde: `Tests/RTHudOverlayClampTests.cpp` (se legge il `DrawSize`)

**Interfaces:**
- Consumes: i binding `SkillCalloutRoot`, `SkillCalloutText` e la proprietà `SkillCalloutSeconds` di Task 2.

- [ ] **Step 1: verifica il motore libero** (`Get-CimInstance … UnrealEditor%`), poi apri l'Editor sul clone principale (`D:\Repositories\refactor-tactics-main`) con il C++ di Task 2 già compilato.
- [ ] **Step 2: in `WBP_RT_UnitOverlay`** aggiungi sopra `NameText` un contenitore chiamato esattamente `SkillCalloutRoot` (un `SizeBox` o `Overlay`; `Visibility` iniziale `Collapsed`; `Render Transform Pivot` (0.5, 1)) che contiene: un `Border`/`Image` con il burst a punte (brush: un'immagine `T_RT_SkillCalloutBurst` importata in `Content/RT/UI/Match/`, oppure un `Border` con angoli vivi e contorno spesso se l'immagine non è pronta: lo decide chi edita, §8) e un `TextBlock` chiamato esattamente `SkillCalloutText` in `F_RT_Orbitron_Bold`, colore scuro, `Auto Wrap` spento, `Justification` centro. Il contorno prende `View.TeamColor` tramite `OnOverlayUpdated` già esistente (`RTUnitOverlayWidget.h:187`) o un binding di colore nel WBP.
- [ ] **Step 3: se il fumetto non entra in 220×90**, alza `DrawSize` in `RTUnit.cpp:204` (per esempio `220×130`): il pivot (0.5, 1) tiene la barra ferma e fa crescere verso l'alto; poi suite `RefactorTactics.ScreenHud` e `RefactorTactics.Hud` verdi.
- [ ] **Step 4: salva l'asset, chiudi l'Editor, `git status`** → solo `WBP_RT_UnitOverlay.uasset` (ed eventualmente la texture e `RTUnit.cpp`) modificati; nessun altro `.uasset` toccato per sbaglio.
- [ ] **Step 5: commit** `feat(ui): WBP_RT_UnitOverlay porta il fumetto della skill (#4025)`.

---

### Task 5: verifica PIE, documentazione, chiusura

**Files:**
- Modify: `docs/roadmap/editor-sessions.yaml` (nuovo record dopo `U98`, `:6558`), `docs/technical/systems/progettazione-hud.md` (una sottosezione accanto alla regola del token, `:188-193`), `docs/superpowers/specs/2026-11-08-fumetto-skill-design.md` (statuto: implementato, commit)

- [ ] **Step 1: build e suite intera** (`RefactorTactics`) sul commit dichiarato; annota lo sha e i rossi preesistenti, se ce ne sono, con la loro issue (`RequiredActionClipsAreCooked` è rosso per costruzione, #3562).
- [ ] **Step 2: PIE** sui punti (a)–(i) di spec §7, più (e′) «torna in tattica entro i 3 s e il fumetto ricompare». Una cattura per punto nello scratchpad di sessione; i tempi stimati a occhio si dichiarano tali.
- [ ] **Step 3: registra la seduta** in `editor-sessions.yaml` sul modello di `U98` (`:6558`): commit, build, punti con `PASS/FAIL`, catture. I punti (b) e (c) restano `da rimisurare` finché una seconda passata, su commit dichiarato, non li firma (CLAUDE.md §6, §10).
- [ ] **Step 4: documenta** in `progettazione-hud.md`: il fumetto vive nell'overlay come il token (privacy per costruzione), ma il suo tempo è `PlaybackElapsedTotal` e non `NativeTick`; chi grida lo decide `ActionShouts`; rimanda alla spec. Aggiorna lo statuto della spec («implementato in `<sha>`»).
- [ ] **Step 5: commit** `docs(hud): il fumetto della skill nell'overlay, seduta PIE e statuto (#4025)`.
- [ ] **Step 6: PR verso `main`** con `--body-file`, DoD della issue spuntata per ciò che è `PASS`, `NOT RUN` dichiarato per packaged e per (b)/(c) se la seconda passata non c'è ancora; review; merge; poi `gh issue view 4025` e `git ls-remote --heads origin feat/4025-fumetto-skill` per verificare entrambe le cose; apri la issue della fetta 2 (azioni bloccate) che rimanda alla spec §9.
