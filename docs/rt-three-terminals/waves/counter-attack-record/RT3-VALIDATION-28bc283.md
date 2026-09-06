```text
=== RT3 HANDOFF ===

FROM:          VALIDATION
TO:            USER
FEATURE:       counter-attack-record
WAVE_ID:       counter-attack-record/1
SUPERSEDES:    RT3-VALIDATION-0c06a89.md, RT3-VALIDATION-12342a9.md

BRANCH:        refactor/2587-counter-attack-record
PARENT_BRANCH: main
BASE_SHA:      ee71f3e3
EXPECTED_SHA:  0eeb5c10        risolto da RT3-EDITOR-0eeb5c1.md
OBSERVED_HEAD: 28bc2837
PRODUCED_SHA:  28bc2837        (VALIDATION non committa: invariato)

STATUS: BLOCKED
REASON: RT_SESSION_REQUIRED — il lease del motore rifiuta un processo effimero
UNBLOCK: aprire un terminale RT persistente (VS Code -> Run Task ->
         "RT: Open VALIDATION terminal"), poi rieseguire i gate
```

---

## PREFLIGHT — VERDE, tutti i controlli

`MISSING_INPUT` del ciclo precedente è **risolto**: `RT3-EDITOR-0eeb5c1.md` esiste,
`FROM: EDITOR` · `TO: VALIDATION` · `STATUS: READY` · `PRODUCED_SHA: 0eeb5c10`.
Nessun `BLOCKED` upstream da propagare.

| # | controllo | esito |
|---|---|---|
| 1 | `0eeb5c10` antenato di `28bc2837` | **OK** |
| 2 | write-set byte-identico `0eeb5c10`..HEAD | **OK** |
| 3 | handoff DEV-LEAD e EDITOR presenti | **OK** |
| 4 | PR #2594 `OPEN` sullo stesso branch | **OK** |

Matrice EDITOR ben formata: tutti i suoi verdetti sono `OBSERVED` o `NOT RUN` con `REASON`.
Nessun `PASS` sopra il tetto, nessun conflitto con un mio `FAIL`. Niente da contestare.

---

## PERCHÉ I GATE RESTANO NOT RUN

```text
> rt-lease.ps1 -Action acquire -Operation BUILD -TaskId 2587
BLOCKED: RT_SESSION_REQUIRED - acquire richiede una sessione RT persistente.
Un processo effimero non puo' possedere il motore: terminerebbe subito dopo averlo preso.
exit 2
```

Il rifiuto è **corretto** e non va aggirato: i comandi di questa sessione sono processi
effimeri. Il mandato §2 vieta di scavalcare il wrapper, e `CLAUDE.md` §10 vieta di usare il
bridge MCP come scorciatoia di validazione — `AutomationTestToolset` avvierebbe la suite
bypassando lease e mutex, cioè invalidando la misura di un'altra sessione.

Macchina misurata libera al momento del tentativo: nessun `UnrealEditor`, nessun
`UnrealEditor-Cmd`, `ENGINE LEASE: LIBERO`. Il blocco è di forma della sessione, non di
contesa.

| gate | verdetto | motivo |
|---|---|---|
| baseline `ee71f3e3` da solo | `NOT RUN` | lease non acquisibile |
| BUILD `0eeb5c10` | `NOT RUN` | idem |
| AUTOMATION `RefactorTactics.Reactions` | `NOT RUN` | idem |
| anti-vacuità (scambio `SourceCell`) | `NOT RUN` | idem |
| TURNLOG/REPLAY · DETERMINISM | `NOT RUN` | idem |
| NETWORK · PRIVACY · PACKAGED · BLUEPRINT | `N/A` | fuori scope dichiarato dal write-set |

### Piano di esecuzione, pronto a partire

Un'osservazione che risparmia una misura: `28bc2837` **contiene** `ee71f3e3`, che contiene
`f29dd374 — NON COMPILATO`. Se la build a HEAD è verde, **entrambe** le cause di §1 cadono in
una sola compilazione; solo un rosso obbliga a separarle.

1. `Build.bat RefactorTacticsEditor Win64 Development -Project=<uproject> -WaitMutex`
2. se rosso in `SRTAnimPreviewViewport.cpp` → causa = base, non write-set
3. `rt-suite.ps1 -Filter RefactorTactics.Reactions`
4. anti-vacuità: scambiare i `SourceCell` dei due record alla costruzione; atteso `CoppiaA` e
   `CoppiaB` a zero, `VociContrattacco` = 2. Verde sotto mutazione ⇒ test muto ⇒ finding.
5. suite intera una volta sola, contando `Test Completed` contro `Found N automation tests`
   — non `Fail=0`, che una run troncata soddisfa.
6. HEAD e write-set riletti **dopo** ogni misura: se mossi, `NON VALIDA`.

---

## MISURATO — statico, indipendente dal motore

| controllo | esito |
|---|---|
| `WRITE_SET` conforme, 4 file | conforme |
| §4.4 — nessun `TMap`/`TSet` introdotto | conforme |
| §4.1 — ordine di produzione | coerente *per costruzione*: sei `Append` paralleli → un solo `for` che accoda in sequenza |
| §4.2 — origine/identità/autore nel record | presente in `FRTCounterAttack` |

§4.1 e §4.2 restano `NOT RUN` come verdetti: la lettura di un ciclo non è una prova di ordine.

---

## FINDINGS

```text
FINDING_ID:   counter-attack-record/1-F1
STATUS:       RISOLTO — EDITOR ha emesso RT3-EDITOR-0eeb5c1.md
ATTEMPT:      1
```

```text
FINDING_ID:   counter-attack-record/1-F2
SEVERITY:     MAGGIORE (processo)
EVIDENCE_REF: RT3-VALIDATION-0c06a89.md, scritto 15:06 e distrutto entro le 15:08
ROOT_CAUSE:   la working directory condivisa elimina i file non tracciati. HEAD si è mosso
              CINQUE volte in sessione:
              9da33c59 -> 1b0c3223 -> ee71f3e3 -> 0c06a891 -> 12342a91 -> 28bc2837
REQUIRED_FIX: gate Unreal in worktree --detach su SHA pinnato, o misura NON VALIDA
OWNER:        processo / USER
ATTEMPT:      1
```

```text
FINDING_ID:   counter-attack-record/1-F3
SEVERITY:     MINORE
EVIDENCE_REF: git show --stat 0c06a891
ROOT_CAUSE:   "rt-mcp" aggiunge scripts/rt-mcp-server.ps1 sul branch di task: fuori write-set,
              non dichiarato da alcun handoff, fuori convenzione di messaggio.
              ⚠️ NON è il mismatch di §9 — il predicato sul write-set è verde. È un commit
              senza owner, e EDITOR lo segnala indipendentemente.
OWNER:        da attribuire
REQUIRED_FIX: dichiararlo o spostarlo fuori dal branch di wave
ATTEMPT:      1
```

```text
FINDING_ID:   counter-attack-record/1-F4
SEVERITY:     BLOCCANTE (ambiente)
EVIDENCE_REF: rt-lease.ps1 -Action acquire -> exit 2, RT_SESSION_REQUIRED
ROOT_CAUSE:   una sessione a comandi effimeri non può possedere il lease del motore
OWNER:        USER
REQUIRED_FIX: VS Code -> Run Task -> "RT: Open VALIDATION terminal"
ATTEMPT:      1
```

---

## RISPOSTA ALLE TRE NOTE DI EDITOR

1. **«VALIDATION emette in parallelo e a monte.»** Fondata come osservazione, e questo handoff
   supera entrambi i cicli precedenti (`SUPERSEDES`). Precisazione sul contenuto: nessuno dei
   due portava verdetti di §7 — erano uscite `BLOCKED` di preflight, che il contratto §4
   impone *prima* di leggere il repository. Un preflight che si ferma non è una validazione
   fuori sequenza; è il rifiuto di iniziarla.
2. **`0c06a891`**: concordo, vedi `F3`. L'avevo aperto, poi ritirato — ma ritirato per la
   ragione sbagliata: il work order normava il *mismatch di HEAD*, non l'*assenza di owner*.
   Reintrodotto con l'inquadramento corretto.
3. **`f29dd374 — NON COMPILATO`**: recepito, ed è dentro il piano al punto 2.

## USER_REQUIRED

1. Aprire il terminale RT VALIDATION persistente — è l'unico blocco residuo.
2. Attribuire l'owner di `0c06a891`.
3. Decidere chi committa questo handoff: non tracciato, in questo albero non sopravvive.

---

RISULTATO: BLOCKED
NEXT_WAVE_AUTHORIZED: no

---

# APPENDICE — gate statici eseguiti senza motore

Il lease resta rifiutato (`RT_SESSION_REQUIRED`, riprovato: stesso esito). Quanto segue non
richiede Unreal ed è attribuibile a `0eeb5c10`.

## Gate WAVE 1 soddisfatti staticamente

**Nessun array parallelo residuo.** I sei array sono collassati in `TArray<FRTCounterAttack>`.
I cinque nomi rimossi hanno **0 occorrenze** in tutto `Source/`:

```text
CounterAttackSrc  CounterActionId  CounterBaseActionId  CounterPriority  CounterAttackActors
```

⚠️ Nota di metodo: il primo giro di questo controllo usò due nomi **inventati**
(`CounterSrcCell`, `CounterSourceCell`) che non erano mai esistiti — zero occorrenze per la
ragione sbagliata. I nomi veri vengono dal diff, non dalla memoria.

**Record completo.** `FRTCounterAttack` porta tutti e sei i campi del gate §5:
`Attack`, `SourceCell`, `ActionId`, `BaseActionId`, `Priority`, `Actor`.

**Lifetime/GC — nessuna regressione.** `Actor` è un `ARTUnit*` grezzo, ma lo era già:
proveniva da `TArray<ARTUnit*> CounterAttackActors`. La struct non è una `USTRUCT` e l'header
lo dichiara, con la ragione (dato transiente, fuori da serializzazione e replica). Il refactor
**sposta** il lifetime, non lo cambia.

**Astrazione non nuova.** `FRTDisplacementCause` vive nello stesso header (riga 19): il record
segue un fratello esistente, non introduce un pattern. Non è over-engineering.

## Anti-vacuità — predizione statica

La mutazione prescritta **è capace** di far cadere il test. Scambiando i `SourceCell`:

| assertion | dopo la mutazione |
|---|---|
| `VociContrattacco == 2` | resta 2 |
| `CoppiaA == 1` — cerca `Src=(0,0) ∧ Tgt=(1,0)` | voce A ha `Src=(0,3)`; voce B ha `Tgt=(1,3)` → **0** |
| `CoppiaB == 1` — cerca `Src=(0,3) ∧ Tgt=(1,3)` | simmetrico → **0** |

Coincide con la previsione di DEV-LEAD. ⚠️ Resta `NOT RUN`: una predizione non è una misura.

---

```text
FINDING_ID:   counter-attack-record/1-F5
SEVERITY:     MINORE (qualità del test, non correttezza del refactor)
EVIDENCE_REF: RTDefensiveReactionTests.cpp — TwoCountersKeepTheirOwnOrigin, righe del fixture:
              ReactorA e ReactorB ricevono entrambi AddCoreAbilityInSlot("Action.Counter", 3)
ROOT_CAUSE:   i due record hanno ActionId, BaseActionId e Priority IDENTICI. Scambiarli fra i
              due contrattacchi è un no-op: nessuna assertion può vederlo. Il test filtra su
              `E.ActionId != IdContrattacco` e non assevera affatto Priority né BaseActionId.
              Nessun altro test copre l'associazione: RTDefensiveReactionTests.cpp:147 assevera
              `Def.Priority` del CATALOGO, non del record.
              ∴ tre dei sei campi che la wave promette di tenere insieme restano non provati —
              e sono esattamente quelli che sei array paralleli disallineavano.
OWNER:        DEV-LEAD
REQUIRED_FIX: dare a ReactorB un'abilità di contrattacco con ActionId/Priority diversi, oppure
              asserire i tre campi direttamente sulle due voci.
REGRESSION:   nessuna: è copertura mancante, non un difetto introdotto
ATTEMPT:      1
```

⚠️ `F5` non impedisce alla wave di essere corretta. Impedisce di **dimostrarlo** sui tre campi
che il gate §5 nomina esplicitamente.

---

# POSTILLA — la wave è stata chiusa dal merge, non dal sign-off

`PR #2594` risulta `MERGED` alle `2026-09-06T13:54:15Z`, merge commit `b292357c`.
`0eeb5c10` è in `main`.

Il mandato prescriveva: «⛔ Il merge della PR #2594 avviene dopo il tuo sign-off, non prima.»
Il sign-off di questo ruolo era, ed è, `BLOCKED`.

**Un merge non converte `NOT RUN` in `PASS`.** La matrice resta quella sopra.

## Ciò che risulta mai eseguito, da nessun ruolo

| ruolo | BUILD di `0eeb5c10` |
|---|---|
| DEV-LEAD | dichiarato non eseguito — «DEV non occupa Unreal» |
| EDITOR | `NOT RUN` — «nessuna compilazione eseguita» |
| VALIDATION | `NOT RUN` — lease rifiutato |

Riscontro indipendente: `Binaries/Win64/UnrealEditor-RefactorTactics.dll` porta data
**06:41**, circa nove ore prima del merge e prima che `0eeb5c10` esistesse. Il binario presente
non contiene il codice della wave.

∴ il codice di WAVE 1 è in `main` **senza che sia mai stato compilato né eseguito**, e il test
di caratterizzazione `TwoCountersKeepTheirOwnOrigin` non è mai stato lanciato.

```text
FINDING_ID:   counter-attack-record/1-F6
SEVERITY:     MAGGIORE (processo)
EVIDENCE_REF: gh pr view 2594 -> MERGED 2026-09-06T13:54:15Z, mergeCommit b292357c;
              mtime di Binaries/Win64/UnrealEditor-RefactorTactics.dll = 06:41
ROOT_CAUSE:   merge eseguito con sign-off VALIDATION BLOCKED e zero gate motore eseguiti
OWNER:        USER
REQUIRED_FIX: eseguire build + suite su main da un terminale RT persistente. Se rosso, non è
              più un difetto di branch: è main rotta, e cambia priorità.
ATTEMPT:      1
```

## Priorità raccomandata, ora che è in main

1. `Build.bat RefactorTacticsEditor` su `main` — se rosso in `SRTAnimPreviewViewport.cpp` la
   causa è `f29dd374`, non questo write-set.
2. `rt-suite.ps1 -Filter RefactorTactics.Reactions` + anti-vacuità.
3. `F5`: la copertura mancante su `ActionId`/`BaseActionId`/`Priority` ora vive in `main`.

RISULTATO: BLOCKED (invariato)
NEXT_WAVE_AUTHORIZED: no
