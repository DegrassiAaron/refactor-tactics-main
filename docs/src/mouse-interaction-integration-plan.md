# Mouse Interaction — piano di integrazione repository

**Data:** 2026-08-12  
**Target:** RefactorTactics v0.1 / E11  
**Documento owner da aggiungere:** `docs/technical/spec-mouse-world-ui-interaction.md`

## 1. Nuova issue

**Titolo:** `[E11] Semantic mouse interaction: hover, LMB/RMB e target modes`

### Scope

- semantic target classification;
- resolver puro `PointerMode + candidati + stato autorizzato -> InteractionView`;
- LMB/RMB routing;
- hook Tactical World Overlay;
- hook ViewModel UMG #613;
- no nuove regole gameplay.

### Test DoD

- `RefactorTactics.UI.Mouse.HoverNeverChangesPlan`
- `RefactorTactics.UI.Mouse.EnemyClickInInspectDoesNotAttack`
- `RefactorTactics.UI.Mouse.AbilityThenEnemyClickPlansAttack`
- `RefactorTactics.UI.Mouse.MoveCellWinsOverDoorMesh`
- `RefactorTactics.UI.Mouse.HazardDoesNotStealMove`
- `RefactorTactics.UI.Mouse.TargetUnitRejectsCell`
- `RefactorTactics.UI.Mouse.TargetCellIgnoresUnitHit`
- `RefactorTactics.UI.Mouse.TargetEdgeResolvesDeclaredEdge`
- `RefactorTactics.UI.Mouse.TargetObjectResolvesWholeDoor`
- `RefactorTactics.UI.Mouse.InteractReasonIsSanitized`
- `RefactorTactics.UI.Mouse.OwnGhostFocusesPhaseWithoutEditing`
- `RefactorTactics.UI.Mouse.AllyGhostCannotBeEdited`
- `RefactorTactics.UI.Mouse.RightClickBackIsDeterministic`
- `RefactorTactics.UI.Mouse.DecisionWindowWorldClickCannotCommit`

## 2. Cross-link da aggiungere

### #25 — E11

Il semantic mouse contract è superficie trasversale di E11. Non apre una nuova epic: resolver ausiliario E11; UMG #613; Ghost #172/#173; facing #291; Interact #74.

### #613 — UMG

Aggiungere:

- widget interactive vs hit-test-invisible decoration;
- hover Action Slot -> world preview;
- warning -> cross-highlight;
- roster/ghost/objective cross-highlight;
- `InteractionView` come sorgente;
- Interaction Inspector.

### #172 — Ghost

- Own Ghost: hover informativo, LMB pin `PhaseFocus`;
- Ally Ghost: focus, mai edit;
- Enemy Ghost: non esiste lato client;
- Ghost mai gameplay target.

### #173 — Scrubbing

`PhaseFocus = Auto|Prep|Dash|Blast|Move` separato da `PointerMode`; RMB torna ad Auto dopo i livelli Back più prioritari.

### #291 — Facing

`PointerMode::Facing` è il produttore UI mancante: sector legali, LMB conferma, illegale reason, RMB cancel, nessuna correzione silenziosa.

### #74 — Interact

Dato aggiornato importante: `ARTUnit` possiede già `PlannedAttackCell`/`bAttackTargetsCell` e `PlannedCoverEdge`/`bHasPlannedCoverEdge`. La UI riusa cella/bordo/door/arc esistenti; non introdurre `MapElementId` generico in v0.1 senza consumer reale.

### #324 — E23

Porta multi-segmento = un singolo semantic target logico. Stable ID e interaction graph restano owner di E23.

## 3. Scenario policy

Non creare JSON `hover/lmb/rmb` adesso: il Scenario Harness non ha questa grammatica. Prima serve un produttore runtime reale.

Candidati futuri:

- `Spec.UI.Mouse.EnemyInspectIsReadOnly`
- `Spec.UI.Mouse.TargetCellUnderUnit`
- `Spec.UI.Mouse.InteractionReasonPrivacy`
- `Spec.UI.Mouse.GhostFocusDoesNotEdit`

## 4. PIE

Nuova voce prevista: `PIE-V01-MOUSE-INTERACTION`.

Gate: selezione -> Move -> undo -> Inspect enemy -> Ability -> target -> cancel -> Interact -> Ghost phase focus, senza mai dover cliccare per scoprire il significato del click.

## 5. Roadmap incrementale

1. **MI-0 Semantic Hover** — classificazione + InteractionView; nessuna mutazione.
2. **MI-1 Explicit Target Modes** — Inspect/Move/TargetUnit/TargetCell/TargetEdge + Back.
3. **MI-2 HUD bridge** — #613, cursor semantic, cross-highlight, inspector shell.
4. **MI-3 Facing** — #291.
5. **MI-4 Environment Interact** — #74.
6. **MI-5 Ghost Scrubbing** — #172/#173.
7. **MI-6 Decision Window routing** — quando E14 ha il boundary runtime.
8. PIE + packaged.

## 6. Commit suggeriti

```text
docs(ui): specify semantic mouse interaction contract
docs(roadmap): map mouse interaction tests and owners
feat(ui): add semantic pointer resolver
feat(ui): add explicit pointer target modes
test(ui): cover mouse interaction contract
```
