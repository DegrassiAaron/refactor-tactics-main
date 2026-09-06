=== RT3 HANDOFF ===

FROM:          DEV-LEAD
TO:            VALIDATION
FEATURE:       ParseCell — arita' limitata e ramo di rifiuto misurato
WAVE_ID:       parsecell-arity/1

BRANCH:        fix/2482-parsecell-arity
PARENT_BRANCH: main
BASE_SHA:      39f3ec95
PRODUCED_SHA:  022977fd

WRITE_SET:     Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp
               Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp
BINARY_ASSETS: nessuno

⚠️ EXPECTED_SHA per VALIDATION e' il TIP del branch, non `PRODUCED_SHA`. Vedi FINDING F3:
   §10 chiede che il nome del file sia il `PRODUCED_SHA` di chi emette, ma il file e'
   dentro il commit successivo — la regola e' auto-referenziale e non e' soddisfacibile.

## MATRICE

Nessun payload di verdetti: §7 non assegna a DEV-LEAD alcuna colonna, e §9 ne trae la
conseguenza. I verdetti sui sistemi in scope li emette VALIDATION.

## SISTEMI IN SCOPE

DATA VALIDATORS · AUTOMATION/SCENARIO · ERRORS · ARCHITECTURE

Invariati rispetto all'handoff di ingresso `RT3-DEVLEAD-39f3ec9.md`. Il write-set
consolidato coincide con quello previsto: nessun sistema si e' aggiunto.

## CONTRIBUTI CONSOLIDATI

contrib/DEV-MAIN-a1-01.md   STATUS: READY   Source/.../RTScenarioLoader.cpp
contrib/DEV-TEST-b1-01.md   STATUS: READY   Source/.../Tests/RTScenarioLoaderTests.cpp

Scope disgiunti come assegnati. Nessuna collisione. Nessun contributo bloccante.

## RISPOSTE

DEFERRED — richiesta di DEV-MAIN in `## INTEGRATION REQUIRED`: estendere la correzione
a `targetCell` (riga 986) e `dashTo` (riga 1073).

CONDIZIONE CHE SBLOCCA: una issue figlia di #2482 che li copra, con la propria
arbitrazione sul messaggio di `dashTo`.

Motivo del differimento, e non e' il costo: DEV-TEST ha derivato i test dal contratto
arbitrato. Allargare lo scope a wave in corso avrebbe prodotto codice corretto con test
che non lo coprono — cioe' esattamente il verde su un'altra partita che questa issue
esiste per impedire. E `dashTo` porta oggi un messaggio diverso («non dichiara una
destinazione») che potrebbe essere asserito altrove: merita la sua arbitrazione, non
un'estensione per inerzia.

## FINDINGS

FINDING_ID:   parsecell-arity/1-F1
SEVERITY:     P2
EVIDENCE_REF: Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp:986 e :1073
ROOT_CAUSE:   `targetCell` e `dashTo` costruiscono `FRTCellId` inline con la vecchia
              guardia `Num() >= 2` e il layer dedotto, senza passare da `ParseCell`.
              `targetCell` ha un difetto in piu': con meno di due elementi non e' un
              errore, il campo SPARISCE — `bTargetsCell` resta falso in silenzio.
OWNER:        DEV-LEAD (issue figlia)
REQUIRED_FIX: entrambi i siti passano da `ParseCell`, oppure applicano la stessa arita'
REGRESSION:   un test per sito, con arita' 4 rifiutata, prima della richiusura
ATTEMPT:      1

FINDING_ID:   parsecell-arity/1-F2
SEVERITY:     P3
EVIDENCE_REF: issue #2482, sezione «Why»
ROOT_CAUSE:   l'issue afferma che `ParseCell` e' «l'unico ingresso delle coordinate» e
              che i chiamanti sono «otto». Misurato: sette chiamanti (l'ottava riga che
              `grep -c` conta e' la definizione) e due ingressi paralleli — vedi F1.
              L'errore e' stato propagato nel WORK-ORDER di questa wave.
OWNER:        DEV-LEAD
REQUIRED_FIX: commento sull'issue con la misura
ATTEMPT:      1

FINDING_ID:   parsecell-arity/1-F3
SEVERITY:     P2
EVIDENCE_REF: questo file
ROOT_CAUSE:   §10 chiede che l'handoff si chiami `RT3-<ROLE>-<sha7>.md` dove `<sha7>` e'
              il `PRODUCED_SHA` di chi emette. Ma il file e' scritto DA quel ruolo, quindi
              finisce in un commit successivo al `PRODUCED_SHA` che dichiara: la regola e'
              auto-referenziale e non e' soddisfacibile. §5 aggrava: chiede a valle
              `HEAD == EXPECTED_SHA`, e il tip non e' il `PRODUCED_SHA`.
OWNER:        owner di RT3_CONTRACT.md
REQUIRED_FIX: decidere se il nome usa il commit del LAVORO e `EXPECTED_SHA` e' il tip,
              o se l'identita' si stacca dallo SHA come per i contributi (`<PID>-<nn>`)
ATTEMPT:      1

FINDING_ID:   parsecell-arity/1-F4
SEVERITY:     P2
EVIDENCE_REF: docs/rt-three-terminals/prompts/WAVE_DEV_LEAD.md, sezione «Preflight e ruolo di ingresso»
ROOT_CAUSE:   il prompt si concede `INPUT_HANDOFF: none (wave entry)`, in conflitto con §4
              che lo vuole «path a un file esistente». Questa wave dimostra che la clausola
              e' INUTILE oltre che in conflitto: il ruolo di ingresso un input ce l'ha —
              il mandato — e persisterlo come file soddisfa §4 alla lettera.
OWNER:        owner di WAVE_DEV_LEAD.md
REQUIRED_FIX: rimuovere la clausola, non ratificarla
ATTEMPT:      1

## EVIDENCE

Nessun `EVIDENCE_REF` di verdetto: nessuna misura e' stata eseguita da questa wave.
Le evidenze citate nei FINDINGS sono riferimenti a codice, verificati in sola lettura.

## USER_REQUIRED

(nessuno: nessun oracolo umano, nessun asset, nessuna percezione visiva)

## VALIDATION REQUESTED

1. Build:  Build.bat RefactorTacticsEditor Win64 Development
2. Suite:  ./scripts/rt-suite.ps1 -Filter RefactorTactics.Scenario -LogName rt-2482-scenario.log

   Copre i due test nuovi, la fixture migrata, e via `ShippedScenariosAreValid` la
   regressione sul corpus.

3. Anti-vacuita' (acceptance criterion 4) — procedura di mutazione, riga da mutare:
   la guardia `Arr->Num() != 3` in `ParseCell`.

   M1  rimuovere il limite superiore  -> attesi rossi: casi 4 e 5 del primo test,
                                          e tutti e sette i blocchi del secondo
   M3  rimuovere il limite inferiore  -> attesi rossi: casi 0, 1 e 2

   Un test che resta verde sotto la sua mutazione non dimostra niente: e' il criterio.

4. Criterio 6: `git grep -c '"cell"' Scenarios/` invariato. Garantito per costruzione —
   nessun file del corpus e' nel write-set — ma va misurato, non dedotto.

## NOT RUN

BUILD                 NOT RUN — dominio VALIDATION, il ruolo DEV non occupa Unreal
AUTOMATION/SCENARIO   NOT RUN — authoring completo, esecuzione a VALIDATION
MUTATION              NOT RUN — procedura descritta sopra, mai eseguita
PACKAGED              NOT RUN — la Definition of Done viva non lo richiede per questa wave
EDITOR ACCEPTANCE     N/A — nessun .uasset/.umap nel write-set

⚠️ Nessun test di questa wave e' stato eseguito. `STATUS: READY` significa che la wave ha
   un'uscita leggibile, non che qualcosa sia stato verificato.

STATUS: READY
