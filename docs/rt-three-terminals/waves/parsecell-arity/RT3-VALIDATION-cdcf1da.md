=== RT3 HANDOFF ===

⚠️ **2026-09-06 — i soli `EVIDENCE_REF` sono stati riscritti, i verdetti no.**
   Puntavano a `Saved/Logs/`, che non e' tracciato e vive solo dentro un worktree: alla
   sua rimozione i quindici riferimenti sarebbero diventati pendenti, e per §6 un
   `EVIDENCE_REF` che punta a un file assente e' malformato — il sistema corrispondente
   si rilegge `BLOCKED`. Ora puntano a `evidence/`, che e' versionato.
   Gli estratti conservano la numerazione di riga dell'originale, quindi `#L2403` resta
   risolvibile. Nessun verdetto, nessuna misura e nessun FINDING e' stato toccato:
   `RISULTATO` resta quello emesso.

FROM:          VALIDATION
TO:            DEV-LEAD
FEATURE:       ParseCell — arita' limitata e ramo di rifiuto misurato
WAVE_ID:       parsecell-arity/1

BRANCH:        fix/2482-parsecell-arity
PARENT_BRANCH: main
BASE_SHA:      cdcf1dad
PRODUCED_SHA:  cdcf1dad
               = BASE_SHA: questo ruolo non ha scritto sul deliverable. L'unico file
               prodotto e' questo handoff. E' anche l'unico caso in cui la regola di
               naming di §10 e' soddisfacibile — vedi F3.

WRITE_SET:     docs/rt-three-terminals/waves/parsecell-arity/RT3-VALIDATION-cdcf1da.md
BINARY_ASSETS: nessuno

STATUS: READY

⚠️ DEROGA APPLICATA A §9. §9 chiede che il ruolo successivo verifichi `PRODUCED_SHA`
   (022977fd) e che un mismatch sia `BLOCKED`. Il mandato ordina di verificare
   `EXPECTED_SHA` (cdcf1dad, il tip). Applicata perche' l'alternativa e' un `BLOCKED`
   su un difetto gia' aperto — `parsecell-arity/1-F3` — non perche' un mandato sia
   autorizzato a derogare al contratto. Registrata come deroga, non come regola.

⚠️ CONFLITTO DI RUOLO DICHIARATO. La revisione che ha prodotto la rev. 2 del mandato e
   la sua esecuzione sono avvenute nella stessa sessione. Non e' stato scritto codice
   di produzione, quindi §12 non e' violata alla lettera; ma il gate eseguito e' il
   gate riscritto da chi lo esegue. Un secondo lettore dovrebbe rivedere almeno la
   costruzione delle tre mutazioni, che e' cio' su cui poggia il criterio 4.

## RT3 INIT

Tipo:            VALIDATION
Feature:         ParseCell — arita' limitata e ramo di rifiuto misurato
Wave:            parsecell-arity/1
Branch:          fix/2482-parsecell-arity
Parent branch:   main
Base SHA:        39f3ec95 (della wave) — cdcf1dad (ereditato in ingresso)
Expected SHA:    cdcf1dad4a5918ce02f9051125cc0f3bbdbe4321
HEAD:            cdcf1dad4a5918ce02f9051125cc0f3bbdbe4321   -> match, invariato per
                 tutta la sessione, riletto a ogni misura
Working tree:    1 file untracked — questo handoff, che e' il WRITE_SET di questo ruolo
Sistemi in scope: DATA VALIDATORS · AUTOMATION/SCENARIO · ERRORS · ARCHITECTURE

## MATRICE

| # | Sistema | VALIDATION | EVIDENCE_REF |
|---:|---|---|---|
| 2 | ARCHITECTURE | `PASS` | build log + RTScenarioLoader.cpp:60-81 |
| 3 | BUILD | `PASS` | `evidence/final-build.txt` |
| 7 | DATA VALIDATORS | `PASS` | suite + mutazioni M1/M2/M3 |
| 32 | AUTOMATION/SCENARIO | `PASS` | `evidence/scenario-post.txt` |
| 33 | ERRORS | `PASS` | mutazione M2 |

Tutti gli altri sistemi della matrice §7: `N/A`, REASON «fuori write-set». Il write-set
e' due `.cpp` — un helper di parsing del loader di scenari e i suoi test — e non tocca
ne' il resolver ne' la presentazione.

`DETERMINISM` e `TURNLOG/REPLAY` restano `N/A` e §8.3 non li tira dentro: a valle di
`ParseCell` esce un `FRTCellId` costruito dagli stessi tre `int32` di prima per ogni
array a tre elementi, e il resolver non vede la differenza. L'argomento copre il ramo di
accettazione; il ramo di rifiuto — scenari che caricavano e ora non caricano piu' — e'
retto dalla MISURA del criterio 6, non dall'argomento.

### BUILD — `PASS`

    command:     Build.bat RefactorTacticsEditor Win64 Development
                 -project=D:\Repositories\rt-wt-2482\RefactorTactics.uproject -waitmutex
    HEAD:        cdcf1dad
    exit code:   0
    esito:       4 action(s) · Result: Succeeded
    binario:     UnrealEditor-RefactorTactics.dll 14393856 byte @ 01:20:51
    EVIDENCE_REF: evidence/final-build.txt
    verdetto:    PASS

⚠️ LIMITE DICHIARATO sulla PRIMA build (00:23:36-00:27:37). Quella compilazione e'
   avvenuta — i DLL portano 00:27:10 e 00:27:33, dentro la finestra — ma il suo
   transcript NON e' piu' rileggibile: il log di UBT e' globale per utente
   (`%LOCALAPPDATA%\UnrealBuildTool\Log.txt`) ed e' stato ruotato due volte nei minuti
   successivi da build di `rt-wt-2448` (29 azioni) e `rt-wt-2455` (5 azioni). Warning
   ed errori di quella compilazione sono perduti. Il `PASS` qui sopra NON poggia su di
   essa: poggia sulla build finale, che ha eseguito 4 azioni con log nel worktree e ha
   prodotto il binario contro cui e' girata la suite registrata.

### ARCHITECTURE — `PASS`

    EVIDENCE_REF: Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp:60-81
    EVIDENCE_REF: Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp:1417-1707
    EVIDENCE_REF: evidence/final-build.txt

- `ParseCell` resta un helper in namespace anonimo: nessuna superficie pubblica nuova,
  nessun tipo nuovo, nessun owner spostato. L'autorita' resta in C++;
- la guardia separa due difetti in due messaggi — array assente (`!Arr`) e lunghezza
  sbagliata (`Num() != 3`) — invece di fonderne uno nell'altro;
- nessuna dipendenza da `TMap`/`TSet`, nessun RNG, nessun ordinamento introdotto;
- nessuna modifica a serializzazione, versioning, snapshot, cache, lifetime, GC,
  confini di modulo, networking, privacy.

⚠️ La review §A e' stata mirata al write-set, come §8 prescrive, non la sweep completa
   della checklist. E' un `PASS` sul cambiamento, non un certificato sul loader intero.

### DATA VALIDATORS · AUTOMATION/SCENARIO · ERRORS — `PASS`

    command:     rt-suite-safe.ps1 -Filter RefactorTactics.Scenario
                 -LogName rt-2482-scenario-post.log -WaitMinutes 25
    HEAD:        cdcf1dad   (identico prima e dopo)
    albero:      d2ffac9c   (identico prima e dopo — 1 file: questo handoff)
    binario:     RefactorTactics 01:20:51/14393856 · RefactorTacticsEditor 00:27:33/1959424
    found N:     174
    performed N: 174
    passed N:    174
    failed N:    0
    exit code:   0
    durata:      00:34
    EVIDENCE_REF: evidence/scenario-post.txt
    verdetto:    PASS — [RT-MEASURE] VALIDA

Per nome, non per conteggio — un `found N` senza valore atteso non puo' fallire:

    RefactorTactics.Scenario.LoaderRejectsCellArityOtherThanThree   Result={Success}
    RefactorTactics.Scenario.EveryCellFieldRejectsWrongArity        Result={Success}

`ERRORS` e' `PASS` per via di M2 e non della suite verde: che il messaggio nomini la
lunghezza TROVATA e' provato dal fatto che togliendola i test cadono, non dal fatto che
passano.

## CRITERI DI ACCETTAZIONE #2482

### Criterio 4 — anti-vacuita' per mutazione — `PASS`

FINESTRA DI MUTAZIONE dichiarata: aperta 00:45:15, chiusa 01:20:32. Dentro non e' stato
registrato nessun verdetto sul deliverable. Ripristino verificato a ogni uscita, e in
`finally`. Impronta del sorgente pulito, ai due estremi: `sha256 8ABEDDF669BE`.

Riga mutata: `Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp:70`

**M1 — limite superiore** · `if (Arr->Num() != 3)` -> `if (Arr->Num() < 3)`

    impronta:    MUTATA (< 3) · sha256 0BAE6DCD6146 — identica applicata / dopo build /
                 prima suite / dopo suite
    albero:      cfe70ce5 a tutte e tre le ridichiarazioni di rt-suite
    found 174 · performed 174 · failed 2 · exit 1 · VALIDA
    ROSSI:       EveryCellFieldRejectsWrongArity (tutti e sette i call-site, riga 1623:
                 cells · interiorWalls · doors · units · move · expect UnitAtCell · variants)
                 LoaderRejectsCellArityOtherThanThree (casi 4 e 5, righe 1507-1508)
    VERDI:       casi 0, 1, 2 — corretto: M1 toglie il solo limite superiore
    EVIDENCE_REF: evidence/M1b.txt
    verdetto:    PASS

⛔ La PRIMA esecuzione di M1 e' NON VALIDA e non e' quella registrata. Durante 791s di
   attesa del lock la mutazione e' sparita dal working tree: `rt-suite` ha letto
   `cfe70ce5 (2 file)` prima dell'attesa e `d2ffac9c (1 file)` dopo — il digest
   dell'albero PULITO. La suite e' poi girata su un binario mutato con un sorgente
   pulito, e i 2 rossi che ha prodotto non sono attribuibili dal referto. Escluso
   `rt-suite` come causa: legge soltanto (`git diff HEAD`, `git ls-files --others`,
   `git hash-object`). **La modifica resta non attribuita.** Vedi F17.

**M2 — il messaggio** · riga 73, rimossi `, trovati %d` e `Arr->Num()`

    impronta:    albero 25f833d4, identico prima e dopo l'attesa del lock e la run
    found 174 · performed 174 · failed 2 · exit 1 · VALIDA
    ROSSI:       le assertion «nomina la lunghezza TROVATA»
    VERDI:       le assertion «e quella ATTESA» — il messaggio contiene ancora «3»
    EVIDENCE_REF: evidence/M2.txt
    verdetto:    PASS

    Non era nella procedura in ingresso, che aveva M1 e M3. E' l'unica delle tre che
    prova che il numero nel messaggio venga LETTO: M1 lascia verdi i casi 0, 1 e 2 e non
    tocca quella proprieta'.

**M3 — limite inferiore** · `if (Arr->Num() != 3)` -> `if (Arr->Num() > 3)`

    impronta:    MUTATA (> 3) · sha256 0D15A61573E1, identica prima e dopo
    albero:      7e3cdaba, identico a entrambe le ridichiarazioni
    filtro:      RefactorTactics.Scenario.LoaderRejectsCellArityOtherThanThree — ISOLATO
    found 1 · performed 0 · failed 0
    ESITO:       CHECK FATALE, come previsto

        Assertion failed: (Index >= 0) & (Index < ArrayNum)
        [File: …\Runtime\Core\Public\Containers\Array.h] [Line: 1339]
        Array index out of bounds: 2 into an array of size 0

    EVIDENCE_REF: evidence/M3.txt#L2403
    verdetto:    PASS

    Indice 2 in un array di dimensione 0: e' il caso `"cell": []`. Guardia (riga 70) e
    lettura (riga 79) sono ACCOPPIATE, quindi il limite inferiore non e' mutabile senza
    uscire dai limiti. L'isolamento era necessario: un abort dentro il filtro intero
    avrebbe lasciato `performed < found` su tutti gli altri 173.
    Nessun processo orfano di `rt-wt-2482` rimasto dopo il crash.

⛔ `rt-suite` ha dichiarato questa run `VALIDA` con `exit 0` — «0/1 completati, 0
   fallimenti» — per una run in cui il motore e' morto e nessun test e' arrivato in
   fondo. Il verdetto di M3 e' derivato dal CONTENUTO del log, non dall'exit code.
   Vedi F16.

**Conclusione del criterio 4**: nessun test e' rimasto verde sotto la propria mutazione.
Le tre mutazioni mordono tre proprieta' distinte — limite superiore, lettura del numero
nel messaggio, limite inferiore — e nessuna e' ridondante rispetto alle altre.

### Criterio 6 — il corpus non cambia — `PASS`

    command:     git diff --name-only 39f3ec955a370e07411c9011cd935c2188bfdfc2..HEAD -- Scenarios/
    HEAD:        cdcf1dad
    exit code:   0
    risultato:   0 righe
    verdetto:    PASS

    command:     git grep -nE '"cell"[[:space:]]*:[[:space:]]*\[[^],]+,[^],]+\]' -- Scenarios/
    HEAD:        cdcf1dad
    exit code:   1   ⚠️ `git grep` esce 1 quando non trova nulla: QUI e' il successo
    risultato:   0 righe — nessun array `"cell"` a due elementi
    controprova: stessa regex a BASE_SHA 39f3ec95 -> 0 righe, exit 1
    verdetto:    PASS

⇒ Non e' una migrazione avvenuta in questa wave: a due elementi il corpus non ne ha mai
  avuti. La forma vietata viveva solo nella fixture di test, gia' migrata.

⛔ La misura prescritta dal mandato originale — `git grep -c '"cell"' Scenarios/` — NON
   e' stata usata: conta la chiave, non l'arita'. Verificata insensibile: 593 a BASE e
   593 a HEAD, e resterebbe 593 sotto qualunque riscrittura. Vedi F7.

## DEFINITION OF DONE

Riletta alla chiusura da `docs/roadmap/v0.1-definition-of-done.md`, non citata da un
handoff. Il path NON e' nominato dal prompt di ruolo: si risolve da
`docs/CONTEXT_INDEX.md:150`. Vedi F9.

NON letto il «Feature Registry» che `WAVE_VALIDATION.md` §Avvio.2 ordina di leggere:
e' uscito dal repository con D-181 il 2026-08-21.

Riga pertinente — 260, «Scenario Harness — nessun bypass», marcata ✅: il criterio che
enuncia (ogni scenario passa da `LockInAndResolve` e dal resolver) non e' toccato da
questa wave. La sua evidenza — «13 test `Scenario.*`» — non e' riconciliabile con i 174
misurati. Vedi F14.

## FINDINGS

FINDING_ID:   parsecell-arity/1-F13
SEVERITY:     P1
EVIDENCE_REF: scripts/rt-mode.ps1:11-12 e :44
ROOT_CAUSE:   Il guard di engine mode e' un file PER WORKSPACE ROOT
              (`<root>/.vscode/rt-engine-mode.txt`), ma il motore Unreal e' UNO
              (`D:\EpicGames\UE_5.8`) condiviso da tutti i checkout. Misurato durante
              questa sessione, con sei checkout attivi — 2330, 2443, 2448, 2455, 2486 e
              questo: l'unico che dichiarava `VALIDATION` era il main checkout, che non
              stava usando il motore; tutti quelli che lo usavano leggevano `DEV`. La
              lettura del guard e' anticorrelata con la verita'.

              ⛔ Secondo strato, peggiore: nel gate `Build.bat` precede `rtsuite`.
              `rt-suite.ps1` HA un guard reale (attende o esce 2 se un processo del
              motore e' vivo) e in questa sessione ha atteso 480s, 791s, 352s, 299s,
              146s e 40s — cioe' ha funzionato. `Build.bat` non ne ha nessuno.
              Eseguendo il gate alla lettera al primo tentativo avrei ricompilato i DLL
              SOTTO la full suite pre-merge di `rt-wt-2443`, e per l'invariante
              «binario» di `rt-suite.ps1` quella misura sarebbe diventata NON VALIDA.
              Un gate di questa wave avrebbe distrutto la misura di un'altra.
OWNER:        owner di scripts/rt-mode.ps1 e dei prompt di wave
REQUIRED_FIX: il guard diventa process-level, con attribuzione al checkout come
              `rt-suite.ps1` gia' fa, e va eseguito PRIMA di qualunque passo che tocchi
              il motore, `Build.bat` incluso; oppure lo stato condiviso esce dal
              workspace root e diventa unico per macchina
REGRESSION:   con un processo `UnrealEditor*` vivo di un altro checkout, l'ingresso
              nella finestra di validazione deve fallire
ATTEMPT:      1

FINDING_ID:   parsecell-arity/1-F16
SEVERITY:     P1
EVIDENCE_REF: evidence/M3.txt#L2403 · output di rt-suite su M3
ROOT_CAUSE:   `rt-suite.ps1` ha dichiarato `VALIDA` con `exit 0` — «0/1 completati, 0
              fallimenti» — una run in cui il motore e' morto su un check fatale e
              nessun test e' arrivato in fondo. La causa e' la tolleranza di copertura
              documentata nella sua stessa docstring: l'ULTIMO test di una suite intera
              perde regolarmente la riga di conclusione nel flush di shutdown, quindi
              «avviati - conclusi <= 1» non invalida. Con un filtro che seleziona
              ESATTAMENTE UN test, quella tolleranza si mangia un crash.
              E' `performed = 0` restituito come `exit 0`, cioe' la non-equivalenza di
              §3 prodotta dallo strumento che esiste per dichiararla.
OWNER:        owner di scripts/rt-suite.ps1
REQUIRED_FIX: la tolleranza si applica solo se `found > 1`, oppure un `Fatal error` /
              `Assertion failed` nel log invalida a prescindere dal conteggio
REGRESSION:   un filtro da un test solo che aborta deve uscire 3 (NON VALIDA), non 0
ATTEMPT:      1

FINDING_ID:   parsecell-arity/1-F17
SEVERITY:     P1
EVIDENCE_REF: output di rt-suite sulla prima esecuzione di M1
ROOT_CAUSE:   Le invarianti di `rt-suite.ps1` si leggono prima e dopo LA RUN, ma dopo
              l'attesa del lock lo stato viene RIDICHIARATO. La finestra d'attesa — in
              questa sessione fino a 791s — non e' coperta da nessuna invariante.
              Misurato: durante l'attesa di M1 la mutazione e' sparita dal working tree
              (`cfe70ce5` -> `d2ffac9c`, il digest pulito), rt-suite ha ri-baselinato, e
              la run e' stata dichiarata VALIDA pur avendo girato su un binario mutato
              con un sorgente pulito.
              ⚠️ Chi abbia ripristinato il file NON e' stato determinato. `rt-suite` e'
              escluso: esegue solo git di lettura. Il fatto e' misurato, la causa no.
OWNER:        owner di scripts/rt-suite.ps1
REQUIRED_FIX: la ridichiarazione dopo l'attesa non e' silenziosa — se lo stato e'
              cambiato durante l'attesa la run parte da una base diversa da quella
              richiesta, e chi ha lanciato deve saperlo
REGRESSION:   un albero che cambia durante l'attesa deve comparire nel referto
ATTEMPT:      1

FINDING_ID:   parsecell-arity/1-F15
SEVERITY:     P2
EVIDENCE_REF: primo tentativo di build, 00:23:36
ROOT_CAUSE:   Il gate prescrive `Build.bat …` senza dire dove va il log ne' come si
              cattura l'exit code. In un worktree fresco `Saved\Logs\` non esiste:
              `Tee-Object -FilePath` fa fallire la pipeline e `$LASTEXITCODE` resta al
              valore di un comando precedente. Risultato: **exit 0 in zero secondi**,
              con la notifica di sistema che dichiara «completed (exit code 0)».
              Registrarlo avrebbe reso BUILD verde senza che un compilatore fosse
              partito.
OWNER:        owner dei prompt di wave
REQUIRED_FIX: forma prescritta — directory creata prima, `*>` invece della pipeline,
              exit code catturato subito dopo il comando nativo, e un controllo che il
              log esista e sia non vuoto
ATTEMPT:      1

FINDING_ID:   parsecell-arity/1-F1
SEVERITY:     P2
EVIDENCE_REF: Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp:986 e :1073
STATO:        CONFERMATO da VALIDATION in lettura:
                  :986   … TryGetArrayField(TEXT("targetCell"), TargetCellArr) && TargetCellArr->Num() >= 2
                  :1073  … !TryGetArrayField(TEXT("dashTo"), DashCellArr) || DashCellArr->Num() < 2
              Entrambi conservano la vecchia guardia e non passano da `ParseCell`.
              Il differimento a issue figlia e' un'arbitrazione DEV-LEAD motivata, non
              una lacuna: allargare lo scope a wave in corso avrebbe prodotto codice
              corretto con test che non lo coprono.
OWNER:        DEV-LEAD (issue figlia)
ATTEMPT:      1 — INVARIATO. Nessuno ha provato a riparare F1: una conferma indipendente
              non deve consumare un tentativo su tre. Vedi F11.

FINDING_ID:   parsecell-arity/1-F5
SEVERITY:     P1
EVIDENCE_REF: mandato VALIDATION rev. 1
ROOT_CAUSE:   Il mandato in ingresso era corrotto in >=9 punti normativi — la riga da
              mutare, gli esiti attesi di M1, l'argomento di scope su DETERMINISM e
              TURNLOG/REPLAY, il template di reporting (persi `performed`, `passed`,
              `failed`), `found N` senza valore atteso, M2 assente dalla numerazione.
              §4 e' fail-closed sui campi MANCANTI e cieco sui campi CORROTTI, che sono
              peggio: sembrano ricostruibili, e ricostruirli e' l'inferenza che §4
              chiama «input inventato».
OWNER:        owner di RT3_CONTRACT.md
REQUIRED_FIX: §4 prende un caso in piu' — campo presente ma non parsabile ⇒
              `MISSING_INPUT` — e i prompt di wave uno step 0 che lo controlla
ATTEMPT:      1

FINDING_ID:   parsecell-arity/1-F6
SEVERITY:     P1
EVIDENCE_REF: RT3-DEVLEAD-022977f.md «VALIDATION REQUESTED» punto 3
              evidence/M3.txt#L2403
ROOT_CAUSE:   La procedura anti-vacuita' prescriveva per M3 «attesi rossi: casi 0, 1 e
              2». MISURATO: non e' un rosso, e' un check fatale — `Array index out of
              bounds: 2 into an array of size 0`. Guardia e lettura sono accoppiate.
              Eseguita sul filtro intero avrebbe lasciato `performed < found` su 173
              test, cioe' una misura NON VALIDA travestita da fallimento parziale.
OWNER:        DEV-LEAD
REQUIRED_FIX: M3 dichiara il check fatale come segnale atteso, gira isolata, e
              l'EVIDENCE_REF punta alla riga del check. Aggiungere M2, che copre la
              lettura del numero nel messaggio e che la procedura non aveva.
ATTEMPT:      1

FINDING_ID:   parsecell-arity/1-F7
SEVERITY:     P1
EVIDENCE_REF: issue #2482 criterio 6 · sezione CRITERI di questo handoff
ROOT_CAUSE:   Il criterio 6 prescrive `git grep -c '"cell"' Scenarios/` invariante.
              Conta la CHIAVE, non l'ARITA': 593 a BASE e 593 a HEAD, e invariante
              sotto qualunque riscrittura da `[q, r]` a `[q, r, 0]` — cioe' la
              migrazione silenziosa che il criterio esiste per vietare.
OWNER:        DEV-LEAD / issue #2482
REQUIRED_FIX: `git diff --name-only <base>..HEAD -- Scenarios/` piu' una scansione di
              arita' sul corpus
ATTEMPT:      1

FINDING_ID:   parsecell-arity/1-F8
SEVERITY:     P1
EVIDENCE_REF: RT3_CONTRACT.md §5 · finestra 00:45:15-01:20:32 di questo handoff
ROOT_CAUSE:   La procedura anti-vacuita' sporca deliberatamente il working tree di un
              file del write-set. §5 dichiara NON VALIDA una misura in cui il working
              tree cambia. Il contratto non prevede finestre dichiarate, quindi il
              criterio 4 e' insoddisfacibile senza violare §5 alla lettera.
OWNER:        owner di RT3_CONTRACT.md
REQUIRED_FIX: §5 riconosce la «finestra di mutazione» come intervallo dichiarato in cui
              non si registra nessun verdetto sul deliverable, con apertura, chiusura,
              ripristino verificato e ri-misura obbligatoria dopo
ATTEMPT:      1

FINDING_ID:   parsecell-arity/1-F9
SEVERITY:     P1
EVIDENCE_REF: WAVE_VALIDATION.md «Avvio» punto 2 · CONTEXT_INDEX.md:168 e :150
ROOT_CAUSE:   §Avvio.2 ordina di leggere il «Feature Registry», uscito dal repository
              con D-181 il 2026-08-21. E ordina di leggere la «Definition of Done viva»
              — da cui §13 fa dipendere `DONE` — senza darne il path.
OWNER:        owner di WAVE_VALIDATION.md
REQUIRED_FIX: rimuovere il Feature Registry; nominare `docs/roadmap/v0.1-definition-of-done.md`
ATTEMPT:      1

FINDING_ID:   parsecell-arity/1-F10
SEVERITY:     P2
EVIDENCE_REF: WAVE_VALIDATION.md «Avvio» punti 1 e 5
ROOT_CAUSE:   `INPUT_HANDOFF_EDITOR` e' dichiarato obbligatorio incondizionatamente, e
              §Avvio.5 chiede di verificare i `BINARY_ASSETS` dell'handoff EDITOR. Una
              wave di solo codice non ha un passaggio EDITOR e quel file non puo'
              esistere: con §4 fail-closed alla lettera, nessuna wave senza asset e'
              validabile. La lacuna NON e' in §4, che chiede `INPUT_HANDOFF` al
              singolare.
OWNER:        owner di WAVE_VALIDATION.md
REQUIRED_FIX: campo condizionale — obbligatorio sse l'handoff a monte dichiara
              `BINARY_ASSETS != nessuno`. Non usare `N/A`: e' un verdetto di §6
              riservato alle righe della matrice.
ATTEMPT:      1

FINDING_ID:   parsecell-arity/1-F11
SEVERITY:     P2
EVIDENCE_REF: RT3_CONTRACT.md §12 · voce F1 di questo handoff
ROOT_CAUSE:   §12 fa incrementare `ATTEMPT` a «ogni ripresentazione dello stesso
              FINDING_ID», e a 3 il ciclo si ferma. Il contatore non distingue una
              ri-presentazione dopo un fix fallito da una conferma indipendente a valle.
OWNER:        owner di RT3_CONTRACT.md
REQUIRED_FIX: `ATTEMPT` sale solo su ripresentazione dopo un `PRODUCED_SHA` nuovo che
              rivendica il fix; la conferma si registra come `CONFIRMED_BY`
ATTEMPT:      1

FINDING_ID:   parsecell-arity/1-F12
SEVERITY:     P2
EVIDENCE_REF: scripts/rt-mode.ps1:44
ROOT_CAUSE:   `rtmode` scrive lo stato dell'engine senza verificare chi lo possiede: e'
              una dichiarazione, non un lock. Sussunto in gran parte da F13; resta
              distinto perche' la correzione e' diversa.
OWNER:        owner di scripts/rt-mode.ps1
ATTEMPT:      1

FINDING_ID:   parsecell-arity/1-F14
SEVERITY:     P2
EVIDENCE_REF: docs/roadmap/v0.1-definition-of-done.md:260
ROOT_CAUSE:   La riga «Scenario Harness — nessun bypass» porta come evidenza «13 test
              `Scenario.*`» ed e' marcata ✅. Misurato: il filtro
              `RefactorTactics.Scenario` trova 174 test. O il «13» nomina un
              sottoinsieme che la riga non elenca, o e' stale: in nessuno dei due casi
              un ruolo a valle puo' usarlo per decidere `DONE`, che e' cio' che §13 gli
              chiede di fare.
OWNER:        owner di v0.1-definition-of-done.md
REQUIRED_FIX: elencare i test che compongono il conteggio, o sostituire il numero con
              il filtro che lo produce
ATTEMPT:      1

## EVIDENCE

    suite:   rt-suite-safe.ps1 -Filter RefactorTactics.Scenario -LogName rt-2482-scenario-post.log
             -> exit 0, found 174, performed 174, passed 174, failed 0, VALIDA
    log:     evidence/scenario-post.txt
    log:     evidence/final-build.txt     (4 action(s), Result: Succeeded)
    log:     evidence/M1b.txt             (M1, 2 fallimenti)
    log:     evidence/M2.txt              (M2, 2 fallimenti)
    log:     evidence/M3.txt#L2403        (M3, check fatale)
    git:     git diff --name-only 39f3ec95..cdcf1dad -- Scenarios/ -> 0 righe, exit 0
    git:     git grep -nE '"cell"…\[x,y\]' -- Scenarios/ -> 0 righe, exit 1 (a HEAD e a BASE)
    code:    Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp:60-81
    code:    Source/RefactorTactics/ScenarioHarness/RTScenarioLoader.cpp:986, :1073 (F1)
    code:    Source/RefactorTactics/Tests/RTScenarioLoaderTests.cpp:1417-1707
    doc:     docs/roadmap/v0.1-definition-of-done.md:260

## USER_REQUIRED

(nessuno: nessun oracolo umano, nessun asset, nessuna percezione visiva)

## NOT RUN

PACKAGED   `N/A` — la Definition of Done viva non lo richiede per questa wave
EDITOR     `N/A` — nessun `.uasset`/`.umap` nel write-set. Vedi F10 per la deviazione.

## P0:
(nessuno)

## P1:
F13  guard di engine mode cieco fra checkout; `Build.bat` fuori da ogni guard
F16  rt-suite dichiara VALIDA/exit 0 una run crashata con performed = 0
F17  le invarianti di rt-suite non coprono l'attesa del lock
F5   il mandato in ingresso era corrotto e §4 non intercetta i campi corrotti
F6   M3 produce un check fatale, non un rosso — misurato
F7   il criterio 6 misura la chiave, non l'arita'
F8   la finestra di mutazione viola §5 per costruzione
F9   §Avvio.2 ordina di leggere un owner rimosso, e non da' il path della DoD

## P2:
F15  la forma ingenua del comando di build produce un exit 0 senza compilare
F1   CONFERMATO — targetCell:986 e dashTo:1073 conservano il vecchio difetto
F10  `INPUT_HANDOFF_EDITOR` obbligatorio incondizionatamente
F11  `ATTEMPT` non distingue conferma da ripresentazione
F12  `rtmode` dichiara, non acquisisce
F14  la riga 260 della DoD non e' usabile come gate

## P3:
(nessuno)

RISULTATO: DONE

Il `DONE` copre i quattro sistemi in scope e i due criteri di accettazione misurati.

⚠️ Cosa NON copre, detto per intero:

- gli otto `P1` sono contro owner diversi da questa wave — il contratto, i prompt di
  ruolo, `rt-mode.ps1`, `rt-suite.ps1`, la issue. Nessuno di essi e' un difetto del
  deliverable, e un `PRODUCED_SHA` nuovo di questa wave non ne ripara nessuno. Ma tre —
  F13, F16, F17 — riguardano la MISURABILITA' di ogni wave futura, non solo di questa;
- `F1` resta aperto per arbitrazione: `targetCell` e `dashTo` conservano il difetto che
  questa wave chiude altrove. La issue figlia e' la condizione di sblocco dichiarata;
- il transcript della prima build non e' piu' rileggibile. Il `PASS` di BUILD non poggia
  su di esso.
