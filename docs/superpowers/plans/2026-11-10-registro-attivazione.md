# Il Registro nomina ogni attivazione — piano di implementazione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** ogni attivazione che il playback mostra diventa una riga del Registro, nell'istante in cui il playback la mostra, fuori dalla dominanza, in entrambe le viste.

**Architecture:** la riga nasce in `ARTTurnManager::ShowActivation` (dove arriva solo ciò che chi guarda può vedere) con il testo già composto, vive in una lista di presentazione del TurnManager, e una funzione pura del view model la fonde con le righe d'esito del TurnLog prima che il widget le legga.

**Tech Stack:** Unreal Engine 5.8, C++, Automation test (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`).

**Spec:** `docs/superpowers/specs/2026-11-10-registro-attivazione-design.md` (regole R1–R10, R7b). Decisione: `D-600` (1).

## Global Constraints

- **Solo presentazione** (R9): nessun campo nuovo in `FRTTurnLogEntry`, snapshot, replay, `StateHash`, replicazione. Nessun golden cambia.
- **Nessun secondo giudizio di conoscenza** (R2): la riga nasce solo in `ShowActivation`; il feed filtra soltanto per la chiave del pubblico (R7b), mai per verdetto.
- **Il testo si compone quando la riga nasce** (R5): `LOCTEXT` nel namespace del file, forma `«{0} — {1}»` (trattino lungo U+2014, come `"Colpita — {0}"`). Unità: `ARTUnit::DisplayLabel(U->HeroDisplayName, U->HeroId, U->GetName())`. Azione: `U->KitDisplayNameFor(ActionId)`, se vuoto l'ultimo segmento dell'id **senza** maiuscolo.
- **Unity build**: gli helper nei namespace anonimi dei test hanno nomi unici nel progetto (prefisso `RigaAtt`), perché due file nello stesso blob unity collidono.
- **Nomi dei test**: nessun nome di test è prefisso di un altro test esistente o nuovo (un test con nome prefisso SPARISCE dalla suite). Famiglie: `RefactorTactics.ActivationLine.*` (pure) e `RefactorTactics.Playback.ActivationLine.*` (con TurnManager).
- **Unreal è uno per macchina**: prima di compilare o lanciare test, leggi la `CommandLine` dei processi `UnrealEditor%` (`Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, Name, CommandLine`). Se un altro clone sta misurando, aspetta. Il PID con `4013-cp53-2b` nella riga di comando è un processo fantasma noto: ignoralo.
- **Build**: `& "D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development -Project="D:\Repositories\refactor-tactics-main\.claude\worktrees\4033-registro-attivazione\RefactorTactics.uproject" -WaitMutex -NoHotReloadFromIDE` in PowerShell, leggendo `$LASTEXITCODE`. Prima build del worktree ~5-8 minuti.
- **Test**: `python tools/suite/esegui.py <filtro> --log <FILE>` dal worktree; `--log` vuole un FILE nello scratchpad di sessione; la prima run nel worktree può impiegare 30 minuti di avvio a freddo: `--timeout 40`. Conta i `Result={Success}` nel log.
- Commenti e documenti in italiano, come il codice circostante.

## Review Focus

1. **Il salto a metà** (R8): saltare dopo che alcune attivazioni sono già state mostrate non deve duplicarle né perdere le rimanenti. Test in Task 2.
2. **La finestra di reazione** (R7): `BeginPlayback(true)` ricostruisce `PlaybackActivationsPrep/Dash` e `ConcludeResolution` azzera `PlaybackElapsedTotal`: se `ShowActivation` venisse richiamata per un'attivazione già mostrata, nascerebbe un doppione. Test in Task 2.
3. **Una sorgente nascosta** (R2): un nemico non osservato non produce riga anche se la sua azione ha un esito visibile. Test in Task 2.
4. **Il budget pieno** (R6): con le righe d'esito già a `MaxFeedLines` non entra nessuna attivazione e nessuna riga d'esito esce. Test in Task 1.
5. **Il cambio di sguardo** (R7b): un feed chiesto per osservatori che non comprendono la squadra della riga non la contiene. Test in Task 1.

---

### Task 1: la riga d'attivazione e la fusione nel feed (puro, senza mondo)

**Files:**
- Create: `Source/RefactorTactics/UI/RTActivationLine.h`
- Modify: `Source/RefactorTactics/UI/RTHudViewModel.h`, `Source/RefactorTactics/UI/RTHudViewModel.cpp`
- Test: `Source/RefactorTactics/Tests/RTActivationLineTests.cpp`

**Interfaces:**
- Produces:
  - `struct FRTActivationLine { int32 SourceStableUnitId = INDEX_NONE; FName ActionId; ERTMatchPhase Phase = ERTMatchPhase::Planning; int32 ViewerTeamId = INDEX_NONE; FText Text; };` in `RTActivationLine.h` (struct C++ semplice, non `USTRUCT`; header leggero, includibile da `RTTurnManager.h`).
  - `static FText URTHudViewModel::ComposeActivationLineText(const FString& UnitLabel, const FText& KitName, FName ActionId);` — `KitName` vuoto o di soli spazi → ultimo segmento di `ActionId` così com'è.
  - `static TArray<FRTPlayerEventLineView> URTHudViewModel::MergeActivationLines(const TArray<FRTPlayerEventLineView>& ResultLines, const TArray<FRTActivationLine>& Activations, const TArray<int32>& ObserverTeamIds, int32 MaxLines);`

**Regole di `MergeActivationLines`** (R6, R7b):
- `ResultLines` resta intera e nel suo ordine (è già tagliata come oggi).
- Si tengono solo le attivazioni con `ViewerTeamId` contenuto in `ObserverTeamIds`.
- Spazio = `MaxLines - ResultLines.Num()` (mai negativo); se le attivazioni tenute sono di più, si scartano le **più vecchie** (le prime della lista).
- Ogni attivazione rimasta si inserisce subito prima della prima riga d'esito con lo stesso `PrimaryStableUnitId` della sua `SourceStableUnitId`; più attivazioni della stessa unità restano nell'ordine della lista; senza riga d'esito della sua unità, va in fondo.
- La riga d'attivazione: `Text` come composto, `Importance = Important`, `PrimaryStableUnitId = SourceStableUnitId`, `Phase` dall'attivazione, `bHasBlocker = false`, `bReaction = false`.
- `Sequence` si rinumera sulla lista unita con la stessa convenzione delle righe d'esito di `BuildPlayerEventFeed` (leggila lì prima di scrivere).

- [ ] **Step 1: test che falliscono** in `RTActivationLineTests.cpp`:
  - `RefactorTactics.ActivationLine.InsertsAboveTheSameUnitResult`: esiti `[U1 «Colpita — 13», U2 «Curata — 5»]`, attivazione di U2 → ordine `[U1, att U2, U2]`.
  - `RefactorTactics.ActivationLine.WithoutAResultGoesLast`: attivazione di U3 senza riga d'esito → in fondo. ⌫ *Superato da [`D-603`](../../decisions/RT_PDR_00_Decision_Log.md) (2), implementata in #4052: la riga va in fondo alla sua fase, e il test si chiama `WithoutAResultClosesItsPhase`.*
  - `RefactorTactics.ActivationLine.KeepsShowOrderForOneUnit`: due attivazioni di U1 → entrambe sopra la riga di U1, nell'ordine della lista.
  - `RefactorTactics.ActivationLine.BudgetDropsOldestActivationsFirst`: `MaxLines = 4`, 2 esiti e 3 attivazioni → restano 2 esiti e le 2 attivazioni più recenti.
  - `RefactorTactics.ActivationLine.FullBudgetKeepsEveryResult`: `MaxLines = 2`, 2 esiti e 1 attivazione → le 2 righe d'esito invariate, nessuna attivazione.
  - `RefactorTactics.ActivationLine.BelongsToTheTeamItWasShownFor`: attivazione con `ViewerTeamId = 1`, osservatori `{0}` → assente; osservatori `{0,1}` → presente.
  - `RefactorTactics.ActivationLine.SequenceCoversTheMergedList`: `Sequence` coerente con la convenzione esistente sull'intera lista.
  - `RefactorTactics.ActivationLine.TextNamesUnitAndAction`: `ComposeActivationLineText("Kallari", «Lama d'ombra», ...)` → `«Kallari — Lama d'ombra»`; con `KitName` vuoto e `ActionId = Hero.Kallari.ShadowBlade` → `«Kallari — ShadowBlade»`.
- [ ] **Step 2:** build e `esegui.py ActivationLine --log …`: i test falliscono (o non compilano) per l'assenza delle funzioni.
- [ ] **Step 3:** implementa struct e funzioni.
- [ ] **Step 4:** build e test: verdi.
- [ ] **Step 5: commit** `feat(registro): la riga d'attivazione e la sua fusione nel feed, pure e testate (#4033)`.

### Task 2: il TurnManager produce le righe e il feed le legge

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.h`, `Source/RefactorTactics/Turn/RTTurnManager.cpp`, `Source/RefactorTactics/UI/RTHudViewModel.cpp`
- Test: `Source/RefactorTactics/Tests/RTActivationLinePlaybackTests.cpp` (fixture sul modello di `Tests/RTSkillCalloutPlaybackTests.cpp`: `CostruisciFumetto`, `CorriFinoAllaFineDelFumetto`, `MontaFinestraDelFumetto`; copia ciò che serve con nomi `RigaAtt*`, non includere l'altro file)

**Interfaces:**
- Consumes: `FRTActivationLine`, `ComposeActivationLineText`, `MergeActivationLines` (Task 1).
- Produces: `const TArray<FRTActivationLine>& ARTTurnManager::GetActivationLines() const;` (sola lettura, per HUD e test).

**Comportamento:**
- `ShowActivation`: per **ogni** chiamata (prima e indipendentemente dal ramo di `ActionShouts`) aggiunge una `FRTActivationLine` con `ViewerTeamId = PlaybackViewerTeamId`, fase e azione dell'evento, testo composto con l'unità sorgente. Con sorgente non trovata (`Src == nullptr`), nessuna riga. Ogni attivazione dà al massimo **una** riga: la chiave d'identità è `(Phase, SourceStableUnitId, ActionId)`; prima di scrivere, verifica nel codice se due attivazioni dello stesso turno possono avere la stessa chiave e, se sì, aggiungi l'ordinale e dichiaralo nel commento.
- `LockInAndResolve`: la lista si azzera accanto a `TurnLog.Reset()` (R7). Non si azzera in `BeginPlayback`, `ConcludeResolution` o `FinishPlayback`.
- `FinishPlayback`, ramo di **fine vera** (dopo il `return` del ramo `IsResolutionSuspended()`), anche da `SkipPlayback`: per ogni attivazione di `PlaybackActivationsPrep`, poi `PlaybackActivationsDash`, poi gli elementi `AbilityActivated` di `PlaybackBlastSequence` (in quest'ordine) che non ha ancora una riga, aggiunge la riga, senza riprodurre clip né fumetto (R8).
- `URTHudViewModel::BuildPlayerEventFeed(const ARTTurnManager*, const TArray<int32>&)`: le righe d'esito come oggi, poi `MergeActivationLines(…, TurnManager->GetActivationLines(), ObserverTeamIds, MaxFeedLines)`.
- Il commento lungo di `ShowActivation` racconta la riga nuova in due righe, nello stile esistente.

- [ ] **Step 1: test che falliscono** in `RTActivationLinePlaybackTests.cpp`:
  - `RefactorTactics.Playback.ActivationLine.AppearsWhenShownNotBefore`: dopo `BeginPlayback` e prima che il playback arrivi all'attivazione, `GetActivationLines()` vuota e il feed senza la riga; dopo, una riga col testo atteso.
  - `RefactorTactics.Playback.ActivationLine.EveryActionKindHasALine`: una skill del kit, un attacco base, un'azione generica e `Gadget.PortableCover` producono ciascuno una riga; nello stesso test il fumetto si accende solo per la skill del kit.
  - `RefactorTactics.Playback.ActivationLine.HiddenSourceHasNoLine`: sorgente nemica non osservata → nessuna riga, nemmeno a fine playback e dopo un salto.
  - `RefactorTactics.Playback.ActivationLine.OutsideDominance`: l'unità che colpisce ha nel feed sia «Colpita — N» sia la sua riga d'attivazione, quest'ultima subito sopra.
  - `RefactorTactics.Playback.ActivationLine.SurvivesTheReactionWindow`: righe nate prima della finestra presenti alla ripresa, nessun doppione dopo la ripresa; il turno successivo (nuovo `LockInAndResolve`) parte vuoto. La finestra si apre solo con un ascoltatore legato a `OnReactionWindowOpened` (vedi `MontaFinestraDelFumetto`).
  - `RefactorTactics.Playback.ActivationLine.SkipRevealsTheRest`: salto dopo la prima attivazione mostrata → a fine playback una riga per ogni attivazione visibile, ciascuna una volta, nell'ordine del playback.
  - `RefactorTactics.Playback.ActivationLine.TurnWithoutPlaybackStartsEmpty`: un turno con `bEnablePlayback = false` dopo un turno con righe → lista vuota.
  - `RefactorTactics.Playback.ActivationLine.SameInBothViews`: il feed è identico con vista strategica accesa e spenta.
- [ ] **Step 2:** build e `esegui.py Playback.ActivationLine --log …`: falliscono.
- [ ] **Step 3:** implementa.
- [ ] **Step 4:** build e test: verdi; poi le famiglie vicine `esegui.py Playback+ScreenHud+UI.PlayerEventLog+ActivationLine --log …`: verdi.
- [ ] **Step 5: commit** `feat(registro): ShowActivation scrive la riga, il salto recupera le rimaste, il feed le fonde (#4033)`.

### Task 3: documenti e voce PIE

**Files:**
- Modify: `docs/technical/test-manuali-pie.md` (voce nuova `PIE-HUD-REGISTRO-ATTIVA`, sul modello di `PIE-HUD-FUMETTO`: stato ⏳, issue #4033 citata per prima nella cella di stato)
- Modify: `docs/superpowers/specs/2026-11-10-registro-attivazione-design.md` (in testa: stato «implementata in `feat/4033-registro-attivazione`», con i commit)

**Voce PIE, verifiche:** (a) vista tattica: la riga «unità — azione» compare quando il playback mostra l'azione, sopra la riga d'esito della stessa unità; (b) vista strategica (Tab): stessa riga, fumetto assente; (c) un nemico non osservato non produce riga; (d) attacco base e gadget hanno la riga ma non il fumetto; (e) salto del playback a metà: a fine playback ci sono tutte le righe, senza doppioni; (f) nel Planning successivo le righe del turno restano, marcate come quelle d'esito.

- [ ] **Step 1:** scrivi la voce e aggiorna la spec.
- [ ] **Step 2:** `node tools/radar/doc-coherence.ts --check`, `node tools/radar/doc-links.ts --check`, `node tools/radar/doc-tables.ts --check`, `node tools/radar/issue-refs.ts --check`: tutti PASS.
- [ ] **Step 3: commit** `docs(registro): voce PIE e stato della spec (#4033)`.
