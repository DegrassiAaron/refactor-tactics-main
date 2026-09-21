# Gate della caduta — #2402

> `REFERTO` · **Oggetto**: [#2402](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2402),
> casella D010 *«verifica di mutazione su `OpenLedgeStartsFall` e `NeverOverlaps`»*.
> **Prodotto da**: `tools/mutation/caduta-gate.py`, esecuzione del 2026-09-21 11:08–11:16 su
> `issue/2402-gate-caduta-openledge` = **`69def9e1`**.
> **Esito**: **7 su 7 CADUTA**, baseline `VALIDA` 30/30, exit **0**, durata 08:09.
>
> 🔑 **La settima mutazione è la ragione di questo referto.** `NeverOverlaps` era già bersaglio della 4;
> **`OpenLedgeStartsFall` non era bersaglio di nessuna** — il test che pinna l'affermazione centrale della
> issue (*«chi è spinto oltre un bordo aperto cade»*) non aveva nessuno che ne misurasse la caduta.
>
> ⚠️ **E la mutazione 1 non lo copriva, benché lo sembri**: sopprime la guardia dei *passi residui* e fa
> cadere chi **non** doveva, cioè misura il **confine** della regola. Una suite che copre solo il confine
> resta verde su un ramo cancellato, purché nessuno cada per sbaglio.

## Le due esecuzioni scartate, e perché

Questa è la **terza** esecuzione. Le prime due non sono registrabili, per due ragioni **diverse**, e la
seconda è quella che vale la pena leggere.

### 10:22–10:37 — finestra condivisa

`6-cadute-concorrenti` uscì `MISURA NON VALIDA`: il gate registrò processi estranei nelle sue quattro
finestre — **PID 38820, 35868, 35900, 38892**.

⚠️ **I PID restano senza clone, e non è pigrizia.** Sono processi ormai morti, e un PID morto non si risolve
più in un `.uproject`. Attribuirli interrogando i processi *vivi* dopo la run sarebbe prendere una fotografia
per spiegare un film — errore commesso e ritirato durante questa stessa serie. Di uno solo, **35900**, esiste
una conferma diretta della sessione che lo aveva avviato (clone `refactor-tactics-dev`, verifica di mutazione
di #1805); per gli altri tre nessuna delle tre sessioni coinvolte ha potuto confermare né escludere, perché
`UnrealEditor-Cmd` **non scrive il proprio PID nel log** (`grep -ci "process id"` → `0`).

➡️ **All'indietro la domanda resta aperta; in avanti è chiudibile a costo zero.** Il PID non è l'unica
identità disponibile: `-abslog` dentro lo scratchpad di sessione mette il **nome della sessione nel percorso
del log**, e la `CommandLine` lo espone finché il processo vive — è così che **35900** è stato riconosciuto
senza indovinare. È la tesi di [`AGENTS.md`](../../../AGENTS.md) §11. ⚠️ Il gate, oggi, non ne approfitta:
`misura.py` invoca il motore con `-log=<basename>` e non con `-abslog` nello scratchpad, quindi le **sue** run
non sono attribuibili nemmeno da vive. Sta nei follow-up della PR.

⚠️ **E va detto com'è finita, perché è scomodo**: alla rimisura `6-cadute-concorrenti` è uscita `CADUTA` con
**gli stessi due rossi** che aveva in finestra sporca. Il risultato contestato era già quello giusto. La
rimisura non ha cambiato il fatto — ha cambiato la possibilità di *affermarlo*: sotto contesa non si distingue
«caduta per la mutazione» da «caduta per la macchina», e quella distinzione è tutto ciò che separa una misura
da una coincidenza.

🔑 **Il verso del bias, per chi ne vorrà uno proprio.** La contesa **aggiunge** fallimenti, non ne toglie.
Quindi un `CADUTA` in finestra sporca è un risultato *più forte*, e il caso da rimisurare sarebbe una
`SOPRAVVISSUTA`. ⚠️ Ma la regola copre *il rosso che la mutazione doveva produrre*, e **non** distingue *un
rosso che la contesa può aver prodotto da sé*: sono due rossi diversi sotto lo stesso colore.

### 10:40–10:49 — misura valida, mutazione sbagliata

Finestra esclusiva, baseline `VALIDA`, 7 su 7 `CADUTA`, zero processi estranei. **E il risultato non andava
pubblicato lo stesso**, per una ragione che nessuna sonda sui processi vede: la mutazione era scritta male, e
due dei suoi venti rossi cadevano per una regola **diversa** da quella dichiarata in `prova`. Vedi
*«La stesura precedente sbagliava»*, più sotto.

⛔ È il caso in cui il rosso è vero e la **conclusione** no. Una finestra sporca lascia rumore a cui dare la
colpa; qui non c'era niente di anomalo da vedere — il conteggio tornava, l'esito era verde, e a essere
sbagliata era la *spiegazione*, che è l'unica parte che nessuno misura.

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
| 7-ramo-spinta | sopprimere il collocamento della caduta nella SPINTA | ✅ CADUTA | RefactorTactics.Fall.AdjacentEdgeStillFalls, RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing, RefactorTactics.Fall.FallDamageIsEnvironmentalNotDirect, RefactorTactics.Fall.OccupiedLandingAppliesImpact, RefactorTactics.Fall.OccupiedLandingUsesAdjacentAlternative, RefactorTactics.Fall.OutcomeIsFellNotDisplaced, RefactorTactics.Fall.PrimaryLandingAppliesFallEffects, RefactorTactics.Fall.PrimaryLandingFree, RefactorTactics.Fall.ReplayMatchesResolvedOutcome, RefactorTactics.Fall.SaturatedLandingAppliesFallEffects, RefactorTactics.Fall.TracePreservesCauseChain, RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic, RefactorTactics.ForcedMovement.CancelsRemainingVoluntaryPath, RefactorTactics.ForcedMovement.FallConsumesRemainingDisplacement, RefactorTactics.ForcedMovement.OpenLedgeStartsFall | RefactorTactics.ForcedMovement.OpenLedgeStartsFall |

## Verdetto

✅ **GATE VERDE**: tutte le mutazioni hanno fatto cadere almeno un test.

## Che cosa NON è caduto sotto `7-ramo-spinta`, e perché è l'informazione utile

Molti rossi su una mutazione dicono poco da soli. Ciò che è rimasto **verde** dice di più:

| Test rimasto verde | Che cosa dimostra |
|---|---|
| `ForcedMovement.PullOverOpenLedgeStartsFall` | i due siti di chiamata reggono **separatamente**: la trazione non cade perché cade la spinta, ma per il proprio ramo. Se un domani smettesse di funzionare da sola, questa mutazione non lo nasconderebbe |
| `ForcedMovement.ExhaustedPushAtEdgeDoesNotFall` | sopprimere il collocamento non fa cadere chi **non doveva** cadere comunque: la mutazione non sta misurando il proprio complemento |
| `ForcedMovement.AdjacentGuardedLedgeSaysEdgeGuard` · `...GuardedLedgeIsNamedInTheLog` | la causa `StoppedByEdgeGuard` di [`D-354`](../../decisions/RT_PDR_00_Decision_Log.md) **sopravvive** alla mutazione, ed è il criterio per cui questa mutazione è stata riscritta — vedi sotto |
| `Fall.StaticValidatorRejectsIsolatedLanding` · `...IgnoresRuntimeOccupancy` | la validazione d'authoring di [#2404](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2404) **non passa dal resolver**: è una funzione dell'asset, e la sua indipendenza si vede qui |

### ⌫ La stesura precedente sbagliava, e la prova era nella sua stessa tabella

La prima versione di questa mutazione cortocircuitava l'intera chiamata — `if (false && Cade(...))` — e
questo referto spiegava che `ForcedMovement.GuardedLedgeDoesNotFall` restava verde *«perché il parapetto
nega la caduta a monte»*.

🔴 **Falso.** Il parapetto **non** è a monte: la decisione vive **dentro** la lambda `Cade`
(`RTTurnManager_Blast.cpp:2183-2196`), che vi scrive `OutEsito = ERTMoveOutcome::StoppedByEdgeGuard`.
Cortocircuitando la chiamata spariva anche quell'effetto collaterale, e con lui la causa di `D-354`: nel giro
precedente **`AdjacentGuardedLedgeSaysEdgeGuard` e `GuardedLedgeIsNamedInTheLog` erano rossi**, cioè due dei
venti rossi cadevano per una regola **diversa** da quella dichiarata in `prova`.

`GuardedLedgeDoesNotFall` era verde per un'altra ragione ancora: asserisce la **posizione** e nient'altro,
e la posizione non cambia.

∴ la mutazione ora tocca la **congiunzione** `&& Atterraggio != T->Cell`, non la chiamata: `Cade` resta
invocata, l'esito continua a essere calcolato, e cade solo il collocamento. È la differenza fra una
mutazione che isola una regola e una che ne abbatte due.

## 🔴 Una vacuità trovata dalla mutazione, su un test che D010 nomina

`Fall.NeverOverlaps` restava verde sotto la **prima** stesura della mutazione — quella che sopprimeva l'intera
chiamata — e **non** per una buona ragione.

Il test mette in scena tre unità e una spinta, e asseriva solo `TestNotEqual` a coppie sulle celle finali.
Senza caduta, `Bersaglio` si ferma su `(1,0,1)` per spinta invece che per ripiego saturo, `Occupante` non si
muove, e le tre celle restano distinte: **l'asserzione reggeva per costruzione**.

⚠️ Cioè la garanzia di occupazione che D010 di [#2402](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2402)
nomina era soddisfatta anche da un resolver in cui **non cade nessuno**.

➡️ Aggiunta una premessa sull'**esito** (`FellToLastStable`), non sulla posizione — che non distingue i due
casi. È la stessa ragione per cui [#2403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2403)
ha dovuto separare `FellToLastStable` da `FellWithoutLanding`.

### La premessa è stata misurata, non dedotta

⛔ **E il gate in tabella non la misura**, perché la mutazione ristretta preserva l'esito: `Cade` resta
chiamata, scrive `FellToLastStable` in `OutEsito`, e il ramo `else if (Dest != T->Cell)` lo registra comunque
nel TurnLog. In questa mappa anche la cella finale coincide. ∴ sotto `7-ramo-spinta` il test è verde
**legittimamente**: la mutazione non cambia niente di ciò che quel test osserva.

Per non lasciare la premessa come ragionamento, è stata misurata a mano contro la stesura **larga**:

```text
mutazione:  if (false && Cade(...))          # la chiamata, non la congiunzione
build:      Result: Succeeded
esito:      Result={Fail} Name={NeverOverlaps}
riga:       RTFallOverLedgeTests.cpp(819)
            «premessa: la caduta e' avvenuta ed e' finita sul ripiego saturo: The two values are not equal»
ripristino: git checkout -- ; albero pulito ; rebuild Result: Succeeded
```

🔑 Prima della modifica **quella stessa mutazione lo lasciava verde**. È la premessa a farlo cadere, ed è ciò
che la distingue da una riga di cautela.

⚠️ **Resta una debolezza, e va nominata**: la mappa del test non lascia mai atterrare chi cade — sotto c'è
solo il primario, occupato — quindi `NeverOverlaps` verifica la non-sovrapposizione in uno scenario dove il
cadente non si muove. A farlo cadere sulla sovrapposizione vera è la mutazione **4**, ed è così che D010 è
soddisfatta.

🔑 **È il gate che ha trovato il buco**, non una rilettura: una mutazione che sopravvive *su un test verde* è
ciò che rivela un'asserzione vacua, e nessuna suite verde l'avrebbe mostrato.

## ⛔ Che cosa questa mutazione non può fare, e va detto

Il bersaglio dichiarato di `7-ramo-spinta` **non è esigibile dal codice di uscita**.

`classifica()` chiede l'**intersezione** fra rossi osservati e bersagli attesi. Se domani
`ForcedMovement.OpenLedgeStartsFall` diventasse cieco — per esempio perdendo l'asserzione sulla cella
d'atterraggio — i suoi fratelli cadrebbero lo stesso, l'esito sarebbe `CADUTA (BERSAGLI DIVERSI)`, che
`e_caduta()` conta come caduta, e il gate uscirebbe **0**.

∴ questa mutazione **misura** che quel test sa diventare rosso; non lo **pinna**. Per pinnarlo servirebbe
rendere bloccante `BERSAGLI DIVERSI`, oppure un campo `bersagli_obbligatori` per mutazione: è un cambio di
semantica dello strumento, ed è nei follow-up della PR.

⚠️ La stessa cosa vale per la mutazione 4, e lì la deriva **è già avvenuta**: il suo elenco dichiara sei
bersagli e i referti del 2026-09-09 e di oggi ne registrano otto — `OccupiedLandingAppliesImpact` e
`SaturatedLandingAppliesFallEffects` si sono aggiunti con [#2430](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2430).
Nessuno se n'era accorto, perché un soprainsieme non fa rumore.

## Note

- **Filtro** `RefactorTactics.Fall+RefactorTactics.ForcedMovement`, come il referto precedente. Tiene fuori
  `Simulation.GoldenCorpusMatches`, che riscrive i propri artefatti quando l'esito della simulazione cambia e
  renderebbe `NON VALIDA` ogni mutazione che tocca il resolver.
- **Suite intera**, misurata sullo stesso commit dopo che il gate aveva ripristinato e ricostruito:
  **2637 Success / 0 Fail**, 02:23, `HEAD` e albero invariati, stesso binario prima e dopo (`11:21:48`).
  ⚠️ **La finestra non era dimostrabilmente esclusiva**: il campione contava **un** processo
  `UnrealEditor` prima e dopo, e non l'ho attribuito — il candidato più probabile è un mio
  `UnrealEditor-Cmd` che non era ancora uscito dopo `Quit`, comportamento che `misura.py` documenta
  ([#3048](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3048)), ma **probabile non è
  misurato**. Per un esito il verso del bias regge comunque: la contesa aggiunge fallimenti e non ne
  toglie, quindi zero rossi in finestra eventualmente condivisa è un risultato più forte, non più debole.
- ⚠️ Questo referto è **documentazione di una run già avvenuta** e viene committato dopo di essa: non fa
  parte di ciò che il gate misura (sorgenti e test), e nomina il commit su cui la misura è stata fatta.
