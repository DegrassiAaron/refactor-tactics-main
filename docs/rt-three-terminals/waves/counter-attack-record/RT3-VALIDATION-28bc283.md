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
