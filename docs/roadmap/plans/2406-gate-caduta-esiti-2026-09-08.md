# Gate della caduta - #2406

> `REFERTO` · **Oggetto**: [#2406](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2406)
> **Prodotto da**: `tools/mutation/caduta-gate.py`, esecuzione del 2026-09-08 20:28-20:31.
> **Esito**: 5 mutazioni applicabili su 6, **tutte CADUTE**. La sesta e' bloccata da
> [#2430](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2430) e il gate esce `BLOCKED`.
> **Misura VALIDA**: baseline 22/22, nessun processo estraneo durante le run.

Filtro: `RefactorTactics.Fall+RefactorTactics.ForcedMovement`. Mutazioni eseguite: 5.

## ⚠️ Non applicabili (il gate NON le ha misurate)

- `2-effetti-saturo` - il pattern non si trova in Source/RefactorTactics/Turn/RTTurnManager_Blast.cpp

Baseline: **VALIDA** - esito 22/22 completati, 0 fallimenti - test eseguiti: **22**

| # | Mutazione | Esito | Nuovi rossi | Bersagli attesi |
|---|---|---|---|---|
| 1-passi-residui | far cadere anche una spinta ESAURITA sul ciglio | ✅ CADUTA | RefactorTactics.ForcedMovement.ExhaustedPushAtEdgeDoesNotFall | RefactorTactics.ForcedMovement.ExhaustedPushAtEdgeDoesNotFall |
| 3-ordine-adiacente | ignorare il Facing dell'occupante e usare il solo anello | ✅ CADUTA | RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing | RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing |
| 4-sovrapposizione | permettere la sovrapposizione sul primario occupato | ✅ CADUTA | RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing, RefactorTactics.Fall.NeverOverlaps, RefactorTactics.Fall.OccupiedLandingUsesAdjacentAlternative, RefactorTactics.Fall.SaturatedLandingStaysOnLastStable, RefactorTactics.Fall.TwoFallersOutcomeIsOrderInvariant, RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic | RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing, RefactorTactics.Fall.NeverOverlaps, RefactorTactics.Fall.OccupiedLandingUsesAdjacentAlternative, RefactorTactics.Fall.SaturatedLandingStaysOnLastStable, RefactorTactics.Fall.TwoFallersOutcomeIsOrderInvariant, RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic |
| 5-percorso-volontario | non annullare il percorso volontario dopo uno spostamento forzato | ✅ CADUTA | RefactorTactics.ForcedMovement.CancelsRemainingVoluntaryPath | RefactorTactics.ForcedMovement.CancelsRemainingVoluntaryPath |
| 6-cadute-concorrenti | sopprimere la protezione della destinazione contesa fra due cadute | ✅ CADUTA | RefactorTactics.Fall.TwoFallersOutcomeIsOrderInvariant, RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic | RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic |

## Verdetto

⚠️ **GATE PARZIALE**: le applicabili sono cadute, ma 1 non sono state misurate.
