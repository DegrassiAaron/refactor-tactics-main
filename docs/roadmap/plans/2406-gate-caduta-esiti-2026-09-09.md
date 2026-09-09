# Gate della caduta - #2406

> `REFERTO` · **Oggetto**: [#2406](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2406)
> **Prodotto da**: `tools/mutation/caduta-gate.py`, esecuzione del 2026-09-09 09:52-09:58 su `main` = `c9d88c9e`.
> **Esito**: **6 su 6 CADUTA**, baseline `VALIDA` 28/28, exit **0**. Una sola esecuzione, nessun consolidamento.
>
> 🔑 **La sesta mutazione era `NON APPLICABILE` da quando il gate esiste**: cercava `ApplyFallEffects`, e quel
> simbolo non c'era. Lo ha creato [#2430](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2430),
> che a sua volta aspettava [`D-357`](../../decisions/RT_PDR_00_Decision_Log.md) — una decisione che nessuno
> aveva mai scritto, benche' il `Meta` della issue la dichiarasse come dipendenza.
>
> ⚠️ **Questa esecuzione sostituisce quella delle 08:52**, dove la mutazione 2 era uscita `MISURA NON VALIDA`
> per contaminazione e aveva richiesto una riesecuzione isolata. Qui non serve: un solo giro, una sola baseline.

Filtro: `RefactorTactics.Fall+RefactorTactics.ForcedMovement`. Mutazioni eseguite: 6.

Baseline: **VALIDA** - esito 28/28 completati, 0 fallimenti - test eseguiti: **28**

| # | Mutazione | Esito | Nuovi rossi | Bersagli attesi |
|---|---|---|---|---|
| 1-passi-residui | far cadere anche una spinta ESAURITA sul ciglio | ✅ CADUTA | RefactorTactics.ForcedMovement.ExhaustedPushAtEdgeDoesNotFall | RefactorTactics.ForcedMovement.ExhaustedPushAtEdgeDoesNotFall |
| 2-effetti-saturo | sopprimere gli effetti di caduta nel fallback saturo | ✅ CADUTA | RefactorTactics.Fall.OccupiedLandingAppliesImpact, RefactorTactics.Fall.SaturatedLandingAppliesFallEffects | RefactorTactics.Fall.OccupiedLandingAppliesImpact, RefactorTactics.Fall.SaturatedLandingAppliesFallEffects |
| 3-ordine-adiacente | ignorare il Facing dell'occupante e usare il solo anello | ✅ CADUTA | RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing | RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing |
| 4-sovrapposizione | permettere la sovrapposizione sul primario occupato | ✅ CADUTA | RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing, RefactorTactics.Fall.NeverOverlaps, RefactorTactics.Fall.OccupiedLandingAppliesImpact, RefactorTactics.Fall.OccupiedLandingUsesAdjacentAlternative, RefactorTactics.Fall.SaturatedLandingAppliesFallEffects, RefactorTactics.Fall.SaturatedLandingStaysOnLastStable, RefactorTactics.Fall.TwoFallersOutcomeIsOrderInvariant, RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic | RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing, RefactorTactics.Fall.NeverOverlaps, RefactorTactics.Fall.OccupiedLandingUsesAdjacentAlternative, RefactorTactics.Fall.SaturatedLandingStaysOnLastStable, RefactorTactics.Fall.TwoFallersOutcomeIsOrderInvariant, RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic |
| 5-percorso-volontario | non annullare il percorso volontario dopo uno spostamento forzato | ✅ CADUTA | RefactorTactics.ForcedMovement.CancelsRemainingVoluntaryPath | RefactorTactics.ForcedMovement.CancelsRemainingVoluntaryPath |
| 6-cadute-concorrenti | sopprimere la protezione della destinazione contesa fra due cadute | ✅ CADUTA | RefactorTactics.Fall.TwoFallersOutcomeIsOrderInvariant, RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic | RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic |

## Verdetto

✅ **GATE VERDE**: tutte le mutazioni hanno fatto cadere almeno un test.
