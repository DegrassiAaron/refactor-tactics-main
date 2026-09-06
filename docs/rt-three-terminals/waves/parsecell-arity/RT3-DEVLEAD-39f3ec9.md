=== RT3 HANDOFF ===

FROM:          DEV-LEAD
TO:            EDITOR | VALIDATION
FEATURE:       ParseCell — arita' limitata e ramo di rifiuto misurato
WAVE_ID:       parsecell-arity/1

BRANCH:        fix/2482-parsecell-arity
PARENT_BRANCH: main
BASE_SHA:      39f3ec95c8b1a3e4d5f60718293a4b5c6d7e8f90
PRODUCED_SHA:  = BASE_SHA — questo ruolo non ha scritto codice di produzione

WRITE_SET:     Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp
               Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp
BINARY_ASSETS: nessuno

## MATRICE

Nessun payload di verdetti. §7 non assegna a DEV-LEAD alcuna colonna, e §9 ne trae
la conseguenza: la busta si porta, il payload no. Non e' un handoff malformato.

## CONTRATTO COMPORTAMENTALE

Given              un array JSON di coordinate in ingresso a `ParseCell`, da uno degli otto chiamanti
When               il caricamento di uno scenario legge quell'array
Then               l'array e' accettato se e solo se ha ESATTAMENTE tre elementi `[q, r, layer]`
Authority          il loader; nessuna autorita' di simulazione coinvolta
Timing boundary    N/A — il parsing avviene a caricamento, fuori da ogni fase di turno
Target/recipient   il chiamante, che riceve `false` e un messaggio
Failure            arita' != 3 -> `false`, con messaggio che nomina la lunghezza TROVATA e quella attesa
Fallback           nessuno, e deliberatamente: la cella dedotta E' il difetto
Ordering           N/A — una chiamata produce al massimo un esito
TurnLog            NONE — il parsing precede la partita e non e' osservabile in essa
Replay             N/A — a valle arriva un `FRTCellId` gia' formato
Privacy            N/A — nessun dato di squadra attraversa questo percorso
SEED_SOURCE        none — nessun RNG
Editor-visible     PREDICTED — NOT VERIFIED: uno scenario malformato smette di caricare in Editor,
                   con il messaggio visibile nel log invece di una cella silenziosamente sbagliata

### Arbitrazione DEV-LEAD — la forma a due elementi

L'issue delega esplicitamente la scelta fra due uscite. Scelta: **(b) — rifiutata**.

Motivo: `[q, r]` che diventa `layer = 0` e' la STESSA classe di difetto dell'arita' non
limitata — un input sotto-specificato che diventa una cella valida e forse sbagliata.
La diagnosi dell'issue e' "una cella valida e sbagliata": accettare due elementi la
contraddice. Il corpus e' 532 su 532 a tre elementi e il writer ne scrive sempre tre,
quindi il costo di migrazione e' zero e nessun ramo utile viene rimosso.

⚠️ E' una restrizione di formato. Se l'owner del formato la vuole diversa, la sede e'
l'issue, non un ramo di codice.

## SISTEMI IN SCOPE

Derivati dal write-set con §8. Nessun verdetto: li emettono EDITOR e VALIDATION.

| Sistema | Perche' in scope |
|---|---|
| DATA VALIDATORS | il write-set tocca la validazione dell'input dello scenario |
| AUTOMATION/SCENARIO | il ramo di rifiuto acquisisce test che oggi non esistono |
| ERRORS | il messaggio d'errore e' parte del contratto, non un dettaglio |
| ARCHITECTURE | §8 punto 3: `ParseCell` ha otto chiamanti, la regressione e' a valle di essa |

Fuori scope con `REASON: fuori write-set` — MAP, GRID/GRAPH, PLANNING, MOVEMENT,
TARGETING, LOS/COVER, DAMAGE, TURNLOG/REPLAY, DETERMINISM, NETWORK AUTHORITY,
PRIVACY, UI/HUD, PACKAGED e ogni altro sistema di §7 non elencato sopra.

⚠️ DETERMINISM e TURNLOG/REPLAY sono fuori scope e NON per costo: a valle di `ParseCell`
arriva un `FRTCellId` gia' formato, e il resolver non vede la differenza. Il difetto
sta nell'autorevolezza del corpus, non nella riproducibilita' della partita.

## SCOPE ASSEGNATI

DEV-MAIN   Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp
DEV-TEST   Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp

Disgiunti. Nessun file e' scrivibile da entrambi.

OUT OF SCOPE per entrambi: Scenarios/ (il corpus non si tocca — criterio 6),
Source/RefactorTacticsEditor/ (l'omonimo `ParseCell` del commandlet non e' questa
funzione), ogni altro path.

## CONTRIBUTI CONSOLIDATI

(nessuno ancora — la wave parte adesso)

## RISPOSTE

(nessuna richiesta bloccante ricevuta)

## USER_REQUIRED

(nessuno previsto: nessun oracolo umano, nessun asset, nessuna percezione visiva)

## NOT RUN

BUILD                 NOT RUN — dominio VALIDATION, ruolo DEV non occupa Unreal
AUTOMATION/SCENARIO   NOT RUN — authoring in corso, esecuzione a VALIDATION
EDITOR ACCEPTANCE     N/A — nessun .uasset/.umap nel write-set

STATUS: READY
