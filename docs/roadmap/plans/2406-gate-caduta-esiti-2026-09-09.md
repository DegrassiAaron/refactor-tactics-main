# Gate della caduta - #2406

Filtro: `RefactorTactics.Fall+RefactorTactics.ForcedMovement`. Mutazioni eseguite: 6.

Baseline: **VALIDA** - esito 28/28 completati, 0 fallimenti - test eseguiti: **28**

| # | Mutazione | Esito | Nuovi rossi | Bersagli attesi |
|---|---|---|---|---|
| 1-passi-residui | far cadere anche una spinta ESAURITA sul ciglio | ✅ CADUTA | RefactorTactics.ForcedMovement.ExhaustedPushAtEdgeDoesNotFall | RefactorTactics.ForcedMovement.ExhaustedPushAtEdgeDoesNotFall |
| 2-effetti-saturo | sopprimere gli effetti di caduta nel fallback saturo | ✅ CADUTA | RefactorTactics.Fall.OccupiedLandingAppliesImpact, RefactorTactics.Fall.SaturatedLandingAppliesFallEffects | RefactorTactics.Fall.OccupiedLandingAppliesImpact, RefactorTactics.Fall.SaturatedLandingAppliesFallEffects |  *(riesecuzione isolata del 2026-09-09 09:11, baseline VALIDA 28/28)*
| 3-ordine-adiacente | ignorare il Facing dell'occupante e usare il solo anello | ✅ CADUTA | RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing | RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing |
| 4-sovrapposizione | permettere la sovrapposizione sul primario occupato | ✅ CADUTA | RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing, RefactorTactics.Fall.NeverOverlaps, RefactorTactics.Fall.OccupiedLandingAppliesImpact, RefactorTactics.Fall.OccupiedLandingUsesAdjacentAlternative, RefactorTactics.Fall.SaturatedLandingAppliesFallEffects, RefactorTactics.Fall.SaturatedLandingStaysOnLastStable, RefactorTactics.Fall.TwoFallersOutcomeIsOrderInvariant, RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic | RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing, RefactorTactics.Fall.NeverOverlaps, RefactorTactics.Fall.OccupiedLandingUsesAdjacentAlternative, RefactorTactics.Fall.SaturatedLandingStaysOnLastStable, RefactorTactics.Fall.TwoFallersOutcomeIsOrderInvariant, RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic |
| 5-percorso-volontario | non annullare il percorso volontario dopo uno spostamento forzato | ✅ CADUTA | RefactorTactics.ForcedMovement.CancelsRemainingVoluntaryPath | RefactorTactics.ForcedMovement.CancelsRemainingVoluntaryPath |
| 6-cadute-concorrenti | sopprimere la protezione della destinazione contesa fra due cadute | ✅ CADUTA | RefactorTactics.Fall.TwoFallersOutcomeIsOrderInvariant, RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic | RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic |

## Verdetto

✅ **GATE VERDE**: tutte e sei le mutazioni hanno fatto cadere i propri bersagli.

⚠️ **Due esecuzioni, e va detto.** Le mutazioni 1 e 3-6 vengono dal giro delle 08:52-09:08;
la 2 usciva li` `MISURA NON VALIDA` per contaminazione — non `SOPRAVVISSUTA`: i suoi rossi
includevano gia' i bersagli — ed e' stata rifatta da sola alle 09:11 con `--solo`, su baseline
`VALIDA` e macchina libera. Il ritenta automatico l'aveva provata tre volte prima di
registrarla come non valutabile, che e' cio' per cui esiste.

🔑 **Non e' una somma di misure diverse**: ogni riga ha la propria baseline `VALIDA`, e il
sorgente fra le due esecuzioni non e' cambiato — solo l'elenco dei bersagli di `2`, che ora
dichiara entrambi i test che cadono invece di uno.
