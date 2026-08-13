> 🗄️ **RECEPITO e ARCHIVIATO il 2026-08-13.** `HISTORICAL` · **materiale non autorevole**.
>
> Piano di integrazione del documento archiviato accanto
> ([`2026-08-13-mouse-world-ui-interaction.md`](2026-08-13-mouse-world-ui-interaction.md)). **Applicato in
> parte, e le parti scartate contano quanto quelle applicate** — il repository era più avanti del piano di
> un giorno, e il piano non poteva saperlo.
>
> | Voce del piano | Esito |
> |---|---|
> | §1 nuova issue ausiliaria E11 | ⛔ **non aperta**: [#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705) (CP 11.8) è già quella issue, dal 2026-08-12 |
> | §1 quattordici test `RefactorTactics.UI.Mouse.*` | 🔄 **rinominati** nello spazio esistente `RefactorTactics.PlayerInput.*`; otto nuovi, uno per regola nuova |
> | §2 cross-link a #25 #613 #172 #173 #291 #74 #324 | ✅ **applicati**, più #737 che il piano non prevedeva |
> | §3 niente JSON `hover/lmb/rmb` adesso | ✅ **confermato** — è il criterio con cui l'harness tiene fuori le capability senza produttore |
> | §4 `PIE-V01-MOUSE-INTERACTION` | ⛔ **non creata**: `PIE-V01-POINTER` esiste ed è stata **estesa** col percorso a tappe |
> | §5 roadmap MI-0…MI-6 | 🔄 **assorbita** dai checkpoint esistenti: CP 11.8 (contratto e contesto), #737 (produttori), #613, #291, #74, #172/#173, E14 |
> | §6 commit suggeriti | 🔄 riscritti sui documenti reali toccati |
>
> Il documento **aveva ragione su un fatto che l'owner non aveva misurato**: `ARTUnit` possiede già i campi
> di piano per bersaglio a cella, bordo di copertura e rotazione — e nessuno li scrive dal gioco. Quella
> riga è diventata [#737](https://github.com/DegrassiAaron/refactor-tactics-main/issues/737) e le §2.1/§6.5
> dell'owner.
>
> Il testo originale non è stato riscritto. Resta per **provenienza**.

# Mouse Interaction — piano di integrazione repository

**Data:** 2026-08-12  
**Target:** RefactorTactics v0.1 / E11  
~~**Documento owner da aggiungere:** `docs/technical/spec-mouse-world-ui-interaction.md`~~
→ **owner reale**: [`../../../technical/spec-pointer-interaction.md`](../../../technical/spec-pointer-interaction.md)

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
