# Gate della caduta — #2402

> `REFERTO` · **Oggetto**: [#2402](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2402),
> casella D010 *«verifica di mutazione su `OpenLedgeStartsFall` e `NeverOverlaps`»*.
> **Prodotto da**: `tools/mutation/caduta-gate.py`, esecuzione del 2026-09-21 10:40–10:49 su
> `issue/2402-gate-caduta-openledge` = **`7efa71a1`**.
> **Esito**: **7 su 7 CADUTA**, baseline `VALIDA` 30/30, exit **0**, durata 08:17.
>
> 🔑 **La settima mutazione è la ragione di questo referto.** `NeverOverlaps` era già bersaglio della 4;
> **`OpenLedgeStartsFall` non era bersaglio di nessuna** — il test che pinna l'affermazione centrale della
> issue (*«chi è spinto oltre un bordo aperto cade»*) non aveva nessuno che ne misurasse la caduta.
>
> ⚠️ **E la mutazione 1 non lo copriva, benché lo sembri**: sopprime la guardia dei *passi residui* e fa
> cadere chi **non** doveva, cioè misura il **confine** della regola. Una suite che copre solo il confine
> resta verde su un ramo cancellato, purché nessuno cada per sbaglio.

## La finestra, e il giro che ha dovuto precederlo

⛔ **Questa esecuzione sostituisce quella delle 10:22–10:37**, dove `6-cadute-concorrenti` era uscita
`MISURA NON VALIDA`: la macchina era condivisa, e il gate ha registrato processi estranei nelle sue quattro
finestre — **PID 38820, 35868, 35900, 38892**.

⚠️ **I PID restano senza clone, e non è pigrizia.** Sono numeri di processi ormai morti, e un PID morto non
si risolve più in un `.uproject`. Attribuirli interrogando i processi *vivi* dopo la run sarebbe prendere una
fotografia per spiegare un film — errore commesso e ritirato durante questa stessa serie. Di uno solo,
**35900**, esiste una conferma diretta della sessione che lo aveva avviato (clone `refactor-tactics-dev`,
verifica di mutazione di #1805); per gli altri tre nessuna delle tre sessioni coinvolte ha potuto confermare
né escludere, perché `UnrealEditor-Cmd` **non scrive il proprio PID nel log** (`grep -ci "process id"` → `0`).

➡️ **All'indietro la domanda resta aperta; in avanti è chiudibile a costo zero.** Il PID non è l'unica
identità disponibile: `-abslog` dentro lo scratchpad di sessione mette il **nome della sessione nel percorso
del log**, e la `CommandLine` lo espone finché il processo vive — è così che **35900** è stato riconosciuto
senza indovinare. È la tesi di [`AGENTS.md`](../../../AGENTS.md) §11: *l'unica dichiarazione di possesso che
non può restare stantia è il processo stesso*. ⚠️ Il gate, oggi, non ne approfitta: `misura.py` invoca il
motore con `-log=<basename>`, non con `-abslog` nello scratchpad, quindi le **sue** run non sono
attribuibili nemmeno da vive. Sta nei follow-up della PR, non qui.

🔑 **Perché si è rifatto il giro intero invece di rimisurare la sola mutazione contestata.** Il referto
precedente di questo gate dichiara *«un solo giro, una sola baseline»* proprio per non ricucire una
riesecuzione isolata dentro una tabella. La finestra è stata ottenuta concordandola con le altre due sessioni
attive sulla macchina: *«ti dico quando ho finito»*, non *«finché non vedi processi»* — perché una
`Build.bat` non ha un processo `UnrealEditor` e a una sonda non compare
([`AGENTS.md`](../../../AGENTS.md) §9, [#3226](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3226)).

⚠️ **E va detto com'è finita, perché è scomodo**: `6-cadute-concorrenti` è uscita `CADUTA` con **gli stessi
due rossi** che aveva in finestra sporca. Il risultato contestato era già quello giusto. La rimisura non ha
cambiato il fatto — ha cambiato la possibilità di *affermarlo*: sotto contesa non si distingue «caduta per la
mutazione» da «caduta per la macchina», e quella distinzione è tutto ciò che separa una misura da una
coincidenza.

🔑 **Il verso del bias, per chi legge questo referto e ne vorrà uno proprio.** La contesa **aggiunge**
fallimenti, non ne toglie. Quindi un `CADUTA` in finestra sporca è un risultato *più forte*, e il caso da
rimisurare sarebbe una `SOPRAVVISSUTA`. ⚠️ Ma la regola vale per *il rosso che la mutazione doveva produrre*,
e **non** distingue *un rosso che la contesa può aver prodotto da sé*: sono due rossi diversi sotto lo stesso
colore, ed è per quello che lo strumento rifiuta.

## La misura

Filtro: `RefactorTactics.Fall+RefactorTactics.ForcedMovement`. Mutazioni eseguite: 7.

Baseline: **VALIDA** - esito 30/30 completati, 0 fallimenti - test eseguiti: **30**

| # | Mutazione | Esito | Nuovi rossi | Bersagli attesi |
|---|---|---|---|---|
| 1-passi-residui | far cadere anche una spinta ESAURITA sul ciglio | ✅ CADUTA | RefactorTactics.ForcedMovement.ExhaustedPushAtEdgeDoesNotFall | RefactorTactics.ForcedMovement.ExhaustedPushAtEdgeDoesNotFall |
| 2-effetti-saturo | sopprimere gli effetti di caduta nel fallback saturo | ✅ CADUTA | RefactorTactics.Fall.OccupiedLandingAppliesImpact, RefactorTactics.Fall.SaturatedLandingAppliesFallEffects | RefactorTactics.Fall.OccupiedLandingAppliesImpact, RefactorTactics.Fall.SaturatedLandingAppliesFallEffects |
| 3-ordine-adiacente | ignorare il Facing dell'occupante e usare il solo anello | ✅ CADUTA | RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing | RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing |
| 4-sovrapposizione | permettere la sovrapposizione sul primario occupato | ✅ CADUTA | RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing, RefactorTactics.Fall.NeverOverlaps, RefactorTactics.Fall.OccupiedLandingAppliesImpact, RefactorTactics.Fall.OccupiedLandingUsesAdjacentAlternative, RefactorTactics.Fall.SaturatedLandingAppliesFallEffects, RefactorTactics.Fall.SaturatedLandingStaysOnLastStable, RefactorTactics.Fall.TwoFallersOutcomeIsOrderInvariant, RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic | RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing, RefactorTactics.Fall.NeverOverlaps, RefactorTactics.Fall.OccupiedLandingUsesAdjacentAlternative, RefactorTactics.Fall.SaturatedLandingStaysOnLastStable, RefactorTactics.Fall.TwoFallersOutcomeIsOrderInvariant, RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic |
| 5-percorso-volontario | non annullare il percorso volontario dopo uno spostamento forzato | ✅ CADUTA | RefactorTactics.ForcedMovement.CancelsRemainingVoluntaryPath | RefactorTactics.ForcedMovement.CancelsRemainingVoluntaryPath |
| 6-cadute-concorrenti | sopprimere la protezione della destinazione contesa fra due cadute | ✅ CADUTA | RefactorTactics.Fall.TwoFallersOutcomeIsOrderInvariant, RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic | RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic |
| 7-ramo-spinta | sopprimere del tutto il ramo della caduta nella SPINTA | ✅ CADUTA | RefactorTactics.Fall.AdjacentEdgeStillFalls, RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing, RefactorTactics.Fall.FallDamageIsEnvironmentalNotDirect, RefactorTactics.Fall.NoCellBelowStaysOnLastStable, RefactorTactics.Fall.NoLandingStillAppliesEffects, RefactorTactics.Fall.OccupiedLandingAppliesImpact, RefactorTactics.Fall.OccupiedLandingUsesAdjacentAlternative, RefactorTactics.Fall.OutcomeIsFellNotDisplaced, RefactorTactics.Fall.PrimaryLandingAppliesFallEffects, RefactorTactics.Fall.PrimaryLandingFree, RefactorTactics.Fall.ReplayMatchesResolvedOutcome, RefactorTactics.Fall.SaturatedLandingAppliesFallEffects, RefactorTactics.Fall.SaturatedLandingStaysOnLastStable, RefactorTactics.Fall.TracePreservesCauseChain, RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic, RefactorTactics.ForcedMovement.AdjacentGuardedLedgeSaysEdgeGuard, RefactorTactics.ForcedMovement.CancelsRemainingVoluntaryPath, RefactorTactics.ForcedMovement.FallConsumesRemainingDisplacement, RefactorTactics.ForcedMovement.GuardedLedgeIsNamedInTheLog, RefactorTactics.ForcedMovement.OpenLedgeStartsFall | RefactorTactics.ForcedMovement.OpenLedgeStartsFall |

## Verdetto

✅ **GATE VERDE**: tutte le mutazioni hanno fatto cadere almeno un test.

## Che cosa NON è caduto sotto `7-ramo-spinta`, e perché è l'informazione utile

Venti rossi su una mutazione dicono poco da soli. Ciò che è rimasto **verde** dice molto:

| Test rimasto verde | Che cosa dimostra |
|---|---|
| `ForcedMovement.PullOverOpenLedgeStartsFall` | i due siti di chiamata reggono **separatamente**: la trazione non cade perché la spinta cade, ma per il proprio ramo. Se un domani smettesse di funzionare da sola, questa mutazione non lo nasconderebbe |
| `ForcedMovement.ExhaustedPushAtEdgeDoesNotFall` | sopprimere la caduta non fa cadere chi **non doveva** cadere comunque: la mutazione non sta misurando il proprio complemento |
| `ForcedMovement.GuardedLedgeDoesNotFall` | idem per il parapetto, che nega la caduta a monte |
| `Fall.StaticValidatorRejectsIsolatedLanding` · `...IgnoresRuntimeOccupancy` | la validazione d'authoring di [#2404](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2404) **non passa dal resolver**: è una funzione dell'asset, e la sua indipendenza si vede qui |

## Un bersaglio dichiarato su venti osservati

⚠️ La mutazione `7-ramo-spinta` dichiara **un solo** bersaglio, e non è una svista.

`classifica()` chiede l'**intersezione** fra rossi osservati e bersagli attesi: un soprainsieme resta
`CADUTA` comunque, quindi elencarne venti non cambierebbe nessun verdetto. Cambierebbe invece la
manutenzione — quell'elenco andrebbe riscritto a ogni test nuovo del gruppo `Fall`, **senza che il gate se ne
accorga**.

🔑 È il contrario del caso della mutazione 4, che ne dichiara sei: lì il gruppo è stabile e l'elenco *dice
quali test sanno accorgersi della sovrapposizione*. Qui i rossi sono «tutto ciò che osserva una caduta da
spinta», e la loro misura per esteso è in questa tabella — che è la sede giusta, perché un referto è datato e
un elenco nel sorgente finge di non esserlo.

## Note

- **Filtro** `RefactorTactics.Fall+RefactorTactics.ForcedMovement`, come il referto precedente. Tiene fuori
  `Simulation.GoldenCorpusMatches`, che riscrive i propri artefatti quando l'esito della simulazione cambia e
  renderebbe `NON VALIDA` ogni mutazione che tocca il resolver.
- **Suite intera**, misurata subito dopo sullo stesso commit e nella stessa finestra:
  **2637 Success / 0 Fail**, 02:36, `HEAD` e albero invariati, stesso binario prima e dopo (`10:49:02`),
  zero altri motori → **VALIDA**.
- ⚠️ Questo referto è **documentazione di una run già avvenuta** e viene committato dopo di essa: non fa
  parte di ciò che il gate misura (sorgenti e test), e nomina il commit su cui la misura è stata fatta.
