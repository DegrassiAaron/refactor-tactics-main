```text
=== RT3 HANDOFF ===

FROM:          DEV-LEAD
TO:            EDITOR
FEATURE:       counter-attack-record
WAVE_ID:       counter-attack-record/1

BRANCH:        refactor/2587-counter-attack-record
PARENT_BRANCH: main
BASE_SHA:      ee71f3e3
PRODUCED_SHA:  0eeb5c10

WRITE_SET:     Source/RefactorTactics/Turn/RTReactionPassResult.h
               Source/RefactorTactics/Turn/RTTurnManager.cpp
               Source/RefactorTactics/Tests/RTDefensiveReactionTests.cpp
               Source/RefactorTactics/Tests/RTReactionTests.cpp
BINARY_ASSETS: nessuno

STATUS: READY
```

Issue: [#2587](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2587) — wave 1 di [#2586](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2586).

`STATUS: READY` significa che esiste un ingresso leggibile per EDITOR. Non significa che qualcosa sia stato verificato.

---

## CONTRATTO COMPORTAMENTALE

Cosa deve continuare a valere dopo questo commit. È ciò che i ruoli a valle verificano; qui non c'è alcun verdetto.

1. **L'ordine dei contrattacchi in `Attacks` è quello di produzione.** Erano sei array accodati con sei `Append`; ora sono un `TArray<FRTCounterAttack>` accodato da un ciclo. L'ordine relativo dei colpi di ritorno, e la loro posizione **in coda** ai colpi del piano, non cambiano — `FirstCounter` continua a marcare l'inizio della coda che la voce direzionale di `#2128` itera.

2. **Ogni contrattacco porta la PROPRIA origine, identità e autore.** `SourceCell`, `ActionId`, `BaseActionId`, `Priority` e `Actor` restano appaiati al colpo che li ha prodotti. Prima era un'invariante mantenuta a mano fra sei array; ora è la forma del tipo.

3. **Nessun valore cambia.** Stesso danno, stessi bersagli, stesse voci di TurnLog con gli stessi campi. Il refactor non tocca aritmetica, eleggibilità, trigger né catalogo.

4. **Il lifetime non cambia.** `ARTUnit* Actor` è un puntatore grezzo in una struct non-`USTRUCT`, esattamente com'era in `TArray<ARTUnit*>`. Nessuna `USTRUCT`, nessun `UObject`, nessuna `UPROPERTY` introdotti.

5. **Nessun ordine non deterministico introdotto.** Nessun `TMap` né `TSet` nel diff; il consumo è un range-for su un `TArray`.

---

## SISTEMI IN SCOPE

Derivati dal `WRITE_SET` con §8. **Nessun verdetto**: DEV-LEAD non compare nella matrice canonica di §7 e non possiede lo strumento che prova questi sistemi.

| # | Sistema | Perché è in scope |
|---:|---|---|
| 2 | ARCHITECTURE | un tipo nuovo (`FRTCounterAttack`) in un header condiviso dal `TurnManager` e dai test |
| 3 | BUILD | il write-set è C++: la compilabilità va provata, non dedotta |
| 18 | DAMAGE | il ciclo riscritto è quello che immette i colpi di ritorno nella pipeline del danno |
| 21 | REACTIONS | il pass delle reazioni è il codice modificato — sistema diretto della wave |
| 27 | COMBAT LOG | la voce `Combat` legge quattro dei cinque satelliti e il soggetto dal record |
| 32 | AUTOMATION/SCENARIO | un Automation Test nuovo, e due file di test toccati |

**In scope per §8 punto 3** (a valle di un sistema toccato, anche senza file modificati):

| # | Sistema | Perché |
|---:|---|---|
| 28 | TURNLOG/REPLAY | le voci dei contrattacchi escono dal codice riscritto; una traccia che cambiasse cambierebbe l'hash |
| 29 | DETERMINISM | l'ordine dei colpi di ritorno è l'invariante che questo refactor deve preservare |

**Considerati e fuori scope**, dichiarati per non farli sembrare dimenticati:

- **31 PRIVACY** e **30 NETWORK AUTHORITY** — `FRTReactionPassResult` è transiente, non serializzata e non replicata; il diff non tocca replica né proiezione degli eventi.
- **36 PACKAGED** — nessun contenuto distribuito cambia.
- **5 BLUEPRINT** — il tipo nuovo non è esposto: nessuna `USTRUCT`, nessuna `UFUNCTION`.

---

## CONTRIBUTI CONSOLIDATI

Nessuno. Lo scope era di tre file misurati in anticipo: DEV-LEAD ha lavorato da solo, senza istanze DEV-MAIN o DEV-TEST. `contrib/` non esiste per questa wave.

---

## RISPOSTE

Nessuna. Nessun contributo bloccante ricevuto.

---

## USER_REQUIRED

Nessun check a oracolo umano previsto, e la ragione è misurabile: il write-set non tocca `.uasset`, `.umap`, Blueprint, UMG, animazione o UI, e non cambia nulla di osservabile a schermo. Non c'è una domanda che solo una persona davanti al gioco possa rispondere.

Se EDITOR ritiene il contrario, la sede è §9, non una deroga.

---

## NOT RUN

Nessun gate è stato eseguito. Sessione DEV: non occupa Unreal.

| Gate | Stato | Motivo |
|---|---|---|
| Compile | `NOT RUN` | build Unreal non compete a questo ruolo |
| Automation | `NOT RUN` | la suite occupa il motore — dominio VALIDATION |
| Determinism | `NOT RUN` | idem |
| Replay / hash | `NOT RUN` | idem |
| PIE | `NOT RUN` | nessuna evidenza visiva prodotta né richiesta |
| Packaged | `NOT RUN` | fuori write-set |
| Performance | `NOT RUN` | dominio VALIDATION |

⚠️ **La compilabilità per ispezione non è `PASS`.** Il diff è stato riletto (include, forward declaration, ordine dei campi nell'inizializzazione aggregata, lifetime, ordinamento), ma nessun compilatore l'ha visto.

---

## 🔴 AVVERTENZA SULLA BASE, per VALIDATION

`BASE_SHA = ee71f3e3` **contiene un commit che il suo autore dichiara non compilato**:

```text
f29dd374 wip(2554): l'anteprima ticka il mondo e si inquadra da sola — NON COMPILATO
```

Tocca `SRTAnimPreviewViewport.cpp`, cioè il modulo Editor, e non ha alcun rapporto con questo write-set.

**Conseguenza operativa**: se la build fallisce, il primo controllo non è questo delta. Compilare `ee71f3e3` da solo separa le due cause; senza quella misura un rosso su questo branch non è attribuibile.

---

## TEST SCRITTO, DA ESEGUIRE

| campo | valore |
|---|---|
| **nome** | `RefactorTactics.Reactions.Counter.TwoCountersKeepTheirOwnOrigin` |
| **file** | `Source/RefactorTactics/Tests/RTDefensiveReactionTests.cpp` |
| **comando** | `./scripts/rt-suite.ps1 -Filter RefactorTactics.Reactions` |
| **proprietà provata** | due contrattacchi nello stesso Blast tengono ciascuno la propria origine (`SrcCell`), il proprio bersaglio e il proprio autore |
| **atteso** | **verde** su `0eeb5c10` — è un test di caratterizzazione del comportamento corretto, non la riproduzione di un difetto attivo |

🔑 **Perché serviva.** Con **un solo** contrattacco in scena ogni permutazione dei sei array dava lo stesso risultato: il difetto che il commento della struct dichiarava non era osservabile da nessun test esistente. Servono due record perché uno scambio produca un rosso.

**Anti-vacuità richiesta a VALIDATION** — il test è nuovo e non ha mai fallito, quindi la sua capacità di discriminare è un'affermazione, non una misura:

> scambiare fra loro i `SourceCell` dei due record al momento della costruzione. Atteso: `CoppiaA` e `CoppiaB` vanno **entrambe a zero** mentre `VociContrattacco` resta `2`.

Se sotto quella mutazione il test resta verde, è muto e va riscritto — indipendentemente da come si comporta il refactor.

---

## PROSSIMO RUOLO

`EDITOR`, per la catena canonica.

⚠️ Con `BINARY_ASSETS: nessuno` e zero sistemi ASSETS/BLUEPRINT/MAP/UI in scope, EDITOR non ha materia propria in questa wave: sui sistemi in scope il suo tetto di §7 è `OBSERVED` su BUILD, ARCHITECTURE, TURNLOG/REPLAY, DETERMINISM e AUTOMATION/SCENARIO — può guardare, non provare. Il ruolo che chiude è VALIDATION.

Non ho trovato nel contratto una regola che consenta di saltarlo: se per questa wave si vuole `DEV-LEAD → VALIDATION`, è una decisione da prendere in §9, non un'inferenza.
