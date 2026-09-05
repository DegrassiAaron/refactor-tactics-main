# RT3 — Contratto di wave

Contratto condiviso dai prompt di wave `WAVE_EDITOR.md` e `WAVE_VALIDATION.md`.

Owner di questo documento: se una regola qui contraddice `CLAUDE.md` o `AGENTS.md`, vince il documento di repository. Questo file non è autorità su lifecycle Editor, suite o Git: li richiama.

## 1. Due livelli, due documenti

`rt-three-terminals` ha due livelli distinti. Non confonderli.

| Livello | Domanda | Documenti |
|---|---|---|
| Ruolo | Cosa può occupare questo terminale, e con chi confligge | `TERMINAL_DEV.md` · `TERMINAL_VALIDATION.md` · `TERMINAL_EDITOR.md` |
| Wave | Come si esegue e si consegna un lavoro attraverso i ruoli | `RT3_CONTRACT.md` · `WAVE_EDITOR.md` · `WAVE_VALIDATION.md` |

Un prompt di wave presuppone il prompt di ruolo. Non lo sostituisce.

Un prompt di wave si incolla **da solo**. Non incollare due ruoli nella stessa sessione.

## 2. Attori

| Attore | Definizione | Prompt di ruolo |
|---|---|---|
| DEV | Istanza che scrive codice/test senza occupare Unreal. Possono essere N. | `TERMINAL_DEV.md` |
| DEV-LEAD | La singola istanza DEV che, per una wave, possiede l'integrazione: consolida il lavoro dei DEV ed emette l'handoff RT3 di ingresso. È un ruolo di wave, non un quarto ruolo di terminale: le regole di concorrenza restano quelle DEV. | `TERMINAL_DEV.md` |
| EDITOR | Unica istanza che scrive `.uasset`/`.umap` per la wave. | `TERMINAL_EDITOR.md` |
| VALIDATION | Verificatore indipendente. Non ripara, non possiede binari. | `TERMINAL_VALIDATION.md` |

Se per una wave DEV-LEAD non è designato, la wave non ha ingresso. Vedi §4.

## 3. Principi

Tre non-equivalenze. Valgono su ogni sezione di ogni wave.

```text
MCP command sent   != verified
animation success  != simulator correctness
file modificato    != build/test/PIE/packaged verificato
```

Da cui:

```text
performed = 0      != PASS
risposta MCP vuota != capability assente
assenza in UI      != assenza sul client
```

## 4. Preflight — fail-closed

Prima dello step 1, prima di leggere il repository, prima di aprire l'Editor.

Campi obbligatori:

```text
FEATURE
BRANCH
BASE_SHA
INPUT_HANDOFF   path a un file esistente, non testo incollato
```

Se uno qualsiasi è vuoto, è un placeholder non risolto (`[FEATURE]`, `<...>`, `TBD`) o non è risolvibile:

```text
STATUS: BLOCKED
REASON: MISSING_INPUT
FIELDS: <elenco dei campi mancanti>
```

Poi fermati.

Non ispezionare il repository. Non avviare Unreal. Non dedurre il valore mancante dal contesto, dalla cronologia o dal working tree.

Un placeholder risolto per inferenza è un input inventato.

## 5. Precondizioni del repository

Misura, non presumere:

```powershell
git status --short
git rev-parse HEAD
git rev-parse --abbrev-ref HEAD
```

`BLOCKED` se:

- `HEAD` non corrisponde a `BASE_SHA`;
- `HEAD` è detached e `BASE_SHA` non lo prevede esplicitamente;
- il working tree contiene modifiche non dichiarate nel write-set in ingresso;
- `HEAD`, working tree, binari o processi Unreal cambiano durante una finestra di misura — in quel caso la misura è `NON VALIDA`, non `FAIL`.

L'ultimo caso è già normato in `CLAUDE.md` §6. Qui non viene riscritto: viene applicato.

Il working tree è condiviso tra istanze. Una modifica visibile in `git status` non appartiene necessariamente a questa wave: vedi la regola Git per più DEV in `../README.md`.

## 6. Verdetti tipizzati

Un verdetto non è una parola. È un record con campi obbligatori.

| Verdetto | Campi richiesti | Significato |
|---|---|---|
| `PASS` | `EVIDENCE_REF` | Verificato. L'evidenza è rileggibile da terzi. |
| `FAIL` | `EVIDENCE_REF` | Verificato negativo. |
| `BLOCKED` | `REASON`, `UNBLOCK` | Non verificabile ora. `UNBLOCK` dice cosa lo sbloccherebbe. |
| `N/A` | `REASON` | Fuori dal write-set della wave. |
| `OBSERVED` | `EVIDENCE_REF` | Osservazione registrata, **non** un verdetto. Non conta come `PASS`. |
| `NOT RUN` | `REASON` | Non eseguito. Non conta come `PASS`. |

Regole di forma:

- un `PASS` senza `EVIDENCE_REF` è malformato e si legge `BLOCKED`;
- un `N/A` senza `REASON` è malformato e si legge `BLOCKED`;
- `N/A` è giustificato dal write-set, mai dalla difficoltà o dal tempo;
- `OBSERVED` esiste perché alcuni ruoli possono guardare un sistema senza poterlo provare. Vedi §7.

`EVIDENCE_REF` è un riferimento ad artefatto, non prosa:

```text
log:     Saved/Logs/<file>.log#L<riga>
suite:   <comando> -> exit <n>, found <n>, performed <n>, passed <n>, failed <n>
turnlog: <path dump>
asset:   <path>@<sha7>
shot:    docs/rt-three-terminals/waves/<feature>/evidence/<file>.png
```

Una frase descrittiva non è un `EVIDENCE_REF`.

## 7. Matrice canonica

Una sola tabella per entrambi i ruoli. EDITOR e VALIDATION compilano la stessa lista, ciascuno la propria colonna.

`Verdetto max` limita il verdetto più forte che quel ruolo può emettere per quel sistema.

| # | Sistema | EDITOR max | VALIDATION max |
|---:|---|---|---|
| 1 | PROJECT | `PASS` | `PASS` |
| 2 | ARCHITECTURE | `OBSERVED` | `PASS` |
| 3 | BUILD | `OBSERVED` | `PASS` |
| 4 | ASSETS | `PASS` | `PASS` |
| 5 | BLUEPRINT | `PASS` | `PASS` |
| 6 | DATA | `PASS` | `PASS` |
| 7 | DATA VALIDATORS | `PASS` | `PASS` |
| 8 | MAP | `PASS` | `OBSERVED` |
| 9 | GRID/GRAPH | `PASS` | `PASS` |
| 10 | INPUT | `PASS` | `OBSERVED` |
| 11 | CAMERA | `OBSERVED` | `OBSERVED` |
| 12 | PLANNING | `PASS` | `PASS` |
| 13 | READY/COMMIT | `PASS` | `PASS` |
| 14 | SNAPSHOT | `PASS` | `PASS` |
| 15 | MOVEMENT | `PASS` | `PASS` |
| 16 | TARGETING | `PASS` | `PASS` |
| 17 | LOS/COVER | `PASS` | `PASS` |
| 18 | DAMAGE | `PASS` | `PASS` |
| 19 | STATUS/CONTROL | `PASS` | `PASS` |
| 20 | DISPLACEMENT | `PASS` | `PASS` |
| 21 | REACTIONS | `PASS` | `PASS` |
| 22 | ENVIRONMENT | `PASS` | `PASS` |
| 23 | OBJECTIVES | `PASS` | `PASS` |
| 24 | KO/CLEANUP | `PASS` | `PASS` |
| 25 | UI/HUD | `PASS` | `OBSERVED` |
| 26 | CERTAINTY | `PASS` | `PASS` |
| 27 | COMBAT LOG | `PASS` | `PASS` |
| 28 | TURNLOG/REPLAY | `OBSERVED` | `PASS` |
| 29 | DETERMINISM | `OBSERVED` | `PASS` |
| 30 | NETWORK AUTHORITY | `OBSERVED` | `PASS` |
| 31 | PRIVACY | `OBSERVED` | `PASS` |
| 32 | AUTOMATION/SCENARIO | `OBSERVED` | `PASS` |
| 33 | ERRORS | `PASS` | `PASS` |
| 34 | PERFORMANCE | `OBSERVED` | `PASS` |
| 35 | SAVE/RELOAD | `PASS` | `PASS` |
| 36 | PACKAGED | `N/A` | `PASS` |

Lettura di `OBSERVED` come tetto: quel ruolo può guardare il sistema e registrare cosa vede, ma non possiede lo strumento che lo prova.

Caso guida — `PRIVACY`. EDITOR può constatare che un dato privato non compare nella UI avversaria. Non è una prova: il dato può essere presente sul client. La prova richiede canary lato connessione, che appartiene a VALIDATION. EDITOR emette `OBSERVED`, mai `PASS`.

Disaccordo tra colonne: vince VALIDATION quando il suo verdetto è `FAIL`. Un `PASS` EDITOR contro un `FAIL` VALIDATION apre un Finding, non una media.

## 8. Scoping dal write-set

La matrice non si compila per intero a ogni wave.

L'handoff in ingresso dichiara `WRITE_SET`. Da lì:

1. i sistemi toccati dal write-set sono **in scope** e richiedono un verdetto verificato;
2. i sistemi non toccati sono `N/A` con `REASON: fuori write-set`;
3. i sistemi non toccati ma a valle di uno toccato sono in scope come regressione.

Il punto 3 non è opzionale. Se il write-set tocca il resolver, `TURNLOG/REPLAY` e `DETERMINISM` sono in scope anche se nessun file di quei sistemi è stato modificato.

Non ampliare lo scope per completezza. Non restringerlo per costo.

## 9. Schema di handoff

Tre punti fissi, non uno. Un ruolo che scrive produce un commit diverso da quello che ha ricevuto.

```text
=== RT3 HANDOFF ===

FROM:          DEV-LEAD | EDITOR | VALIDATION
TO:            EDITOR | VALIDATION | DEV-LEAD
FEATURE:
WAVE_ID:       <feature-slug>/<n>

BRANCH:
PARENT_BRANCH: base reale della PR, non "main" per default
BASE_SHA:      commit ereditato in ingresso
PRODUCED_SHA:  commit dopo le scritture di questo ruolo
               = BASE_SHA se questo ruolo non ha scritto

WRITE_SET:     path espliciti, testuali e binari
BINARY_ASSETS: .uasset/.umap toccati, oppure "nessuno"

## MATRICE
<voci in scope, con verdetto tipizzato e campi richiesti>

## FINDINGS
<Finding ID, severità, owner, evidenza — vedi §12>

## EVIDENCE
<EVIDENCE_REF, uno per riga>

## USER_REQUIRED
<check a oracolo umano, con Result: NOT RUN>

STATUS: READY | PARTIAL | BLOCKED
```

`PARENT_BRANCH` è obbligatorio: la PR va aperta sul branch padre, non su `main` per default.

Se `PRODUCED_SHA` differisce da `BASE_SHA`, il ruolo successivo verifica `PRODUCED_SHA`. Un mismatch è `BLOCKED`, non un avviso.

## 10. Persistenza

Un handoff che vive solo nella conversazione non esiste per il ruolo successivo.

```text
docs/rt-three-terminals/waves/<feature-slug>/
  RT3-DEVLEAD-<sha7>.md
  RT3-EDITOR-<sha7>.md
  RT3-VALIDATION-<sha7>.md
  contrib/
  evidence/
```

`<sha7>` è il `PRODUCED_SHA` del ruolo che emette.

I tre file `RT3-*` sono gli handoff dei tre punti fissi della catena. `contrib/` raccoglie i **contributi** delle istanze DEV che non sono DEV-LEAD: non sono handoff, non portano verdetti di §7, e la loro identità non deriva dallo SHA — vedi [`../waves/README.md`](../waves/README.md).

Il ruolo che riceve legge il file. Non ricostruisce l'handoff dal contesto della chat.

Vale anche per l'evidenza: uno screenshot descritto a parole non è riverificabile.

## 11. Propagazione di BLOCKED

Un handoff in ingresso con `STATUS: BLOCKED` blocca il ruolo successivo.

```text
STATUS: BLOCKED
REASON: upstream BLOCKED — <WAVE_ID> <FROM>
UNBLOCK: <ciò che il ruolo a monte deve produrre>
```

Non validare sopra una base che il ruolo precedente ha dichiarato inaffidabile.

`STATUS: PARTIAL` non blocca. Ma i sistemi che il ruolo a monte ha lasciato `BLOCKED` o `NOT RUN` non diventano `PASS` a valle per ereditarietà: vanno misurati, oppure restano non provati.

## 12. Defect policy

`P0`/`P1` trovato da VALIDATION: non riparare il codice di produzione e poi approvare sé stessi.

```text
FINDING_ID:   <WAVE_ID>-F<n>
SEVERITY:     P0 | P1 | P2 | P3
EVIDENCE_REF:
ROOT_CAUSE:
OWNER:        DEV-LEAD | EDITOR
REQUIRED_FIX:
REGRESSION:   test che deve esistere prima della richiusura
ATTEMPT:      <n>
```

Poi richiedi un nuovo `PRODUCED_SHA` e rivalida.

Terminazione — il ciclo non è illimitato:

- ogni ripresentazione dello stesso `FINDING_ID` incrementa `ATTEMPT`;
- ad `ATTEMPT = 3` il ciclo si ferma;
- il Finding viene escalato a decisione umana con `STATUS: BLOCKED` e `REASON: defect loop`.

Un `FINDING_ID` è stabile. Ricomparire con un id nuovo per azzerare il contatore è un aggiramento.

## 13. Definition of Done

`DONE` richiede la Definition of Done **viva**, non quella citata da un handoff.

Rileggila alla chiusura. Se è cambiata durante la wave, vale quella corrente.

Nessun verdetto verde senza il campo che lo prova.
