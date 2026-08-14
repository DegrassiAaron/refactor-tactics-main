# RefactorTactics — Parallel Claude Workflow + Atomic Shared-ID Allocator

## Handoff operativo per Claude Code

**Scopo:** eliminare le collisioni di numerazione e le sovrascritture causate da più sessioni Claude Code eseguite in parallelo sullo stesso PC.

**Problema osservato:** più terminali lavorano sullo stesso repository e spesso sulla stessa working directory. Gli ID sequenziali condivisi, in particolare `D-nnn` del Decision Log, vengono scelti da più sessioni partendo dallo stesso “ultimo numero”, poi rinumerati o sovrascritti. Il Decision Log documenta già numerose collisioni e `AGENTS.md` riconosce esplicitamente che i contatori condivisi non sono isolati dai worktree.

**Baseline osservata durante la preparazione di questo handoff:** `main` a `0ef97d1833b83cfac9d673ad89acff111a1e742f` il 2026-08-13. **Prima di implementare, rileggere sempre `main`: questa SHA è contesto, non un pin.**

---

# 1. Obiettivo

Implementare una soluzione semplice e robusta per il lavoro parallelo locale:

1. **una working directory Git per ogni sessione Claude**, tramite `git worktree`;
2. **un allocatore atomico locale per gli ID `D-nnn`**, condiviso da tutti i worktree dello stesso clone;
3. **un validator** che rilevi duplicati/collisioni prima del merge;
4. **istruzioni operative in `AGENTS.md` / `CLAUDE.md`** affinché Claude non scelga più manualmente “il prossimo D”;
5. nessuna CI nuova: i gate restano locali/manuali, coerentemente con la policy corrente del repository.

La soluzione deve eliminare la race locale senza cambiare il formato canonico esistente `D-001`, `D-002`, ...

---

# 2. Diagnosi: due problemi distinti

## 2.1 Più Claude nella stessa working directory

Questa configurazione è intrinsecamente insicura.

Due sessioni possono:

- editare lo stesso file contemporaneamente;
- vedere modifiche non committate dell’altra sessione;
- cambiare branch sotto i piedi dell’altra;
- fare `git restore`, format o generatori su file toccati dall’altra;
- produrre diff e `git status` che non appartengono a una sola task;
- sovrascrivere righe del Decision Log anche se gli ID fossero perfettamente unici.

**Conclusione:** l’allocatore ID da solo non basta.

### Regola nuova proposta

> **Una sessione Claude esecutiva = un Git worktree + un branch dedicato.**

La working directory principale può restare il punto di controllo/integrazione, ma non deve essere condivisa da più sessioni esecutive.

Esempio Windows / PowerShell:

```powershell
git fetch origin
git worktree add ..\rt-wt-621 -b feat/621-geometry-bake origin/main
cd ..\rt-wt-621
claude
```

Secondo terminale:

```powershell
git fetch origin
git worktree add ..\rt-wt-622 -b feat/622-altro-task origin/main
cd ..\rt-wt-622
claude
```

Non serve duplicare il clone Git: i worktree condividono il repository Git comune.

---

## 2.2 `D-nnn` è un contatore globale

Il repository ha già tentato mitigazioni manuali:

- guardare l’ultimo ID su `main`;
- guardare i branch remoti;
- rileggere `main` prima del merge;
- “chi arriva secondo rinumera”.

Queste regole riducono il danno ma **non eliminano la race**: due processi possono osservare lo stesso stato e scegliere lo stesso ID.

Serve una vera sezione critica.

---

# 3. Soluzione scelta per `D-nnn`

Creare:

```text
scripts/rt_shared_id.py
```

Il tool deve essere **Python standard library only**, come il resto della toolchain locale del repository.

## 3.1 Perché lo stato deve vivere nel Git common dir

NON creare un file versionato tipo:

```text
docs/decisions/next-id.txt
```

perché ogni worktree ne avrebbe una copia e il file stesso diventerebbe un punto di merge conflict.

Usare invece:

```bash
git rev-parse --git-common-dir
```

e sotto quella directory creare stato locale non versionato:

```text
<git-common-dir>/rt-shared-ids/
    state.json
    allocator.lock
```

Tutti i worktree dello stesso clone vedono lo stesso `git-common-dir`.

Quindi:

```text
worktree A ----\
worktree B -----+--> .git comune --> rt-shared-ids/state.json
worktree C ----/
```

Questo è il coordinatore locale.

---

# 4. Comando minimo

Prima versione:

```bash
python scripts/rt_shared_id.py reserve D
```

Output normale:

```text
D-132
```

Opzioni utili:

```bash
python scripts/rt_shared_id.py reserve D --reason "#621 geometry bake provenance"
python scripts/rt_shared_id.py reserve D --count 3
python scripts/rt_shared_id.py status
python scripts/rt_shared_id.py check
python scripts/rt_shared_id.py audit-refs
```

Non introdurre un framework CLI esterno. `argparse` basta.

---

# 5. Semantica di `reserve D`

## 5.1 Lock cross-platform

L’allocazione deve essere serializzata fra processi.

Implementare un file lock reale:

- Windows: `msvcrt.locking`;
- POSIX: `fcntl.flock`.

Il lock deve coprire tutta la sequenza:

```text
acquire lock
  -> read state
  -> inspect canonical max
  -> choose next
  -> persist state atomically
release lock
```

Non fare:

```text
read max
release
write max+1
```

perché ricrea la race.

## 5.2 Scrittura atomica

Aggiornare `state.json` tramite:

1. file temporaneo nella stessa directory;
2. flush;
3. `os.replace(temp, state)`.

Un crash non deve lasciare mezzo JSON.

## 5.3 Monotonicità

Gli ID riservati **non si riusano**.

Se una branch viene abbandonata:

```text
D-132 usato
D-133 prenotato ma abbandonato
D-134 usato
```

è perfettamente accettabile.

**I gap costano zero; il riuso costa ambiguità.**

Non decrementare mai il contatore.

---

# 6. Bootstrap del contatore

Alla prima esecuzione su un clone che non possiede `state.json`:

1. leggere `docs/decisions/RT_PDR_00_Decision_Log.md`;
2. estrarre **solo le dichiarazioni canoniche di riga** della tabella, non ogni riferimento testuale a `D-nnn`;
3. ricavare il massimo;
4. inizializzare `last_issued` a quel massimo;
5. assegnare il successivo.

Esempio di forma da riconoscere:

```markdown
| **D-131** | ...
| D-131 | ...
| ~~D-002~~ | ...
```

Non usare il semplice massimo di tutte le occorrenze nell’intero repository: Note, archive e riferimenti storici contengono molti ID e non sono dichiarazioni.

Il parser deve avere test propri.

---

# 7. Stato delle reservation

Esempio indicativo di `state.json`:

```json
{
  "schema_version": 1,
  "namespaces": {
    "D": {
      "last_issued": 134
    }
  },
  "reservations": [
    {
      "id": "D-132",
      "created_at": "2026-08-13T16:50:00+02:00",
      "branch": "feat/621-geometry-bake",
      "worktree": "C:/dev/rt-wt-621",
      "base_sha": "abc1234",
      "reason": "#621 geometry bake provenance"
    }
  ]
}
```

Campi diagnostici, non autorità competitiva.

Non memorizzare username, token, path sensibili non necessari o dati personali. Il path del worktree può essere omesso se si preferisce minimizzare i dati; branch + SHA sono sufficienti.

---

# 8. `check`: validator del Decision Log corrente

```bash
python scripts/rt_shared_id.py check
```

Deve fallire con exit code `1` se:

- lo stesso `D-nnn` è dichiarato in più di una riga canonica con contenuto diverso;
- una riga decisionale usa un formato ID invalido;
- una nuova dichiarazione usa un ID inferiore/uguale a un ID già riservato da un’altra reservation incompatibile, quando il caso è rilevabile localmente.

Deve stampare errori azionabili, per esempio:

```text
ERROR decision-id duplicate: D-132
  line 184: "La cover generated..."
  line 191: "Il cooldown..."
Use a new reservation; do not renumber by global search/replace.
```

Non trattare le normali citazioni `[D-132](...)` come dichiarazioni duplicate.

---

# 9. `audit-refs`: difesa prima del merge

Il lock locale elimina la race fra processi dello **stesso clone**.

Resta il caso futuro:

- altro PC;
- altro clone;
- branch creato altrove;
- modifica via GitHub.

Per questo aggiungere:

```bash
git fetch --prune origin
python scripts/rt_shared_id.py audit-refs
```

`audit-refs` deve confrontare le dichiarazioni `D-nnn` del Decision Log fra:

```text
refs/heads/*
refs/remotes/origin/*
```

Regola importante:

- lo stesso ID presente su più branch con **la stessa decisione ereditata da main** è normale;
- lo stesso ID associato a **testi canonici differenti** è una collisione.

Quindi confrontare un fingerprint della riga decisionale normalizzata, non il semplice numero di occorrenze.

Output esempio:

```text
COLLISION D-132
  origin/main           -> fingerprint A
  feat/621-geometry     -> fingerprint A
  feat/700-cooldowns    -> fingerprint B
```

Exit code `1`.

Questo sostituisce il ciclo shell manuale documentato nelle Note del Decision Log con un controllo ripetibile.

---

# 10. Workflow Claude richiesto

Aggiornare `AGENTS.md` nella sezione Git.

La nuova regola deve essere inequivocabile.

## 10.1 Avvio task parallelo

Claude deve verificare:

```bash
git status --short
git branch --show-current
git worktree list
```

Se rileva che l’utente sta eseguendo più task paralleli nello stesso worktree, non deve “gestire con attenzione” la situazione: deve indicare che la task va spostata in un worktree dedicato.

Non creare/distruggere worktree automaticamente senza richiesta esplicita dell’utente, coerentemente con le attuali regole Git non distruttive.

## 10.2 Nuova decisione

È vietato inventare manualmente:

```text
ultimo = D-131
quindi uso D-132
```

Usare:

```bash
python scripts/rt_shared_id.py reserve D --reason "<task/issue>"
```

e usare **esattamente** l’ID restituito.

## 10.3 Prima della consegna/merge

Eseguire:

```bash
python scripts/rt_shared_id.py check
git fetch --prune origin
python scripts/rt_shared_id.py audit-refs
```

Il `fetch` modifica solo refs remote locali; se la policy corrente richiede consenso prima di operazioni di rete Git, non automatizzarlo dentro lo script: documentarlo come step esplicito.

---

# 11. `CLAUDE.md`

`CLAUDE.md` deve restare corto.

Non duplicare tutta la procedura.

Aggiungere al massimo un pin sintetico nella sezione Git/guardrail, ad esempio:

```markdown
- Parallel work: una sessione esecutiva per worktree.
- `D-nnn` non si sceglie a mano: `python scripts/rt_shared_id.py reserve D`.
- Prima del merge: `rt_shared_id.py check` + `audit-refs` dopo `git fetch`.
```

Il dettaglio vive in `AGENTS.md` e, se serve, in una nuova spec tecnica.

---

# 12. Documento tecnico owner

Creare:

```text
docs/technical/workflow-parallel-claude.md
```

Contenuto minimo:

1. perché lo stesso worktree non è sicuro;
2. worktree-per-session;
3. esempi PowerShell;
4. come prenotare un `D`;
5. dove vive lo stato locale;
6. perché i gap sono ammessi;
7. differenza fra `check` e `audit-refs`;
8. limite multi-PC;
9. recovery da `state.json` mancante/corrotto;
10. cleanup worktree.

Non trasformare questo documento in una seconda copia di `AGENTS.md`: `AGENTS.md` contiene le regole, la spec spiega il meccanismo.

---

# 13. Recovery

## `state.json` assente

Bootstrap dal massimo canonico del Decision Log.

## `state.json` corrotto

- non tentare di “ripararlo” a intuito;
- rinominarlo localmente come backup;
- ricostruire `last_issued` dal massimo fra:
  - dichiarazioni canoniche correnti;
  - reservation leggibili dal backup, se disponibili;
- scegliere sempre un valore **maggiore**, mai tentare di riempire gap.

## Reservation orfana

Non riusare l’ID.

Può essere mostrata da `status` come orphan/stale, ma il contatore resta monotono.

---

# 14. Test automatici Python

Aggiungere test senza dipendenze esterne, usando `unittest` o lo stile Python già adottato dal repository.

Minimo:

### Parser

1. estrae `D-001` da una riga normale;
2. estrae `D-002` da una riga strike-through;
3. ignora riferimenti `D-123` nel testo della decisione;
4. rileva due dichiarazioni dello stesso ID.

### Allocator

5. bootstrap dal massimo;
6. due allocazioni sequenziali danno ID diversi;
7. `--count 3` produce tre ID contigui;
8. reservation abbandonata non viene riutilizzata;
9. state write/read round trip.

### Concorrenza

10. avvia almeno 20 processi/thread concorrenti contro lo stesso git-common-dir temporaneo;
11. tutti ottengono ID unici;
12. il set è monotono e senza duplicati.

**Questo è il test più importante.**

### Audit refs

13. stessa decisione ereditata su due refs -> verde;
14. stesso ID con testo differente -> rosso;
15. due ID diversi -> verde.

I test devono usare repository Git temporanei creati nella temp directory, non il repository reale.

---

# 15. Errori da evitare

## 15.1 File `next-id.txt` versionato

No. Crea merge conflict e non è atomico fra worktree.

## 15.2 “Leggi main e fai +1”

No. È esattamente la race attuale.

## 15.3 Timestamp come `D-202608131650`

Non necessario per questa migrazione: romperebbe il formato e centinaia di riferimenti senza risolvere il problema dello stesso worktree.

## 15.4 UUID nel Decision Log

Non serve per ora. Il formato `D-nnn` è utile e può restare se l’allocazione è centralizzata.

## 15.5 Range per terminale (`D-200..219`, `D-220..239`)

Fragile, spreca coordinazione manuale e prima o poi i range collidono o finiscono.

## 15.6 Lock dentro il worktree

No. Ogni worktree avrebbe il proprio lock, quindi due sessioni non si vedrebbero.

Il lock deve stare nel **Git common dir**.

## 15.7 Rendere il lock “distribuito” con un file Git

No. Git non è un sistema di lock distribuito per commit concorrenti.

---

# 16. Limite dichiarato: più PC

Questa soluzione risolve completamente la race fra sessioni che condividono lo stesso clone/common Git dir.

Non rende atomica l’allocazione fra **clone su PC diversi**.

Per quel caso esistono due evoluzioni possibili:

### Opzione A — allocatore remoto

Usare GitHub come autorità per la reservation.

Contro:
- richiede rete;
- autenticazione;
- API/tooling;
- aumenta il coupling operativo.

### Opzione B — eliminare i contatori globali

Passare in futuro a ID collision-resistant permanenti, per esempio ULID/UUID o issue-backed IDs.

Contro:
- migrazione molto più invasiva;
- leggibilità inferiore;
- non necessaria per il problema attuale.

**Decisione proposta per ora:** implementare la soluzione locale + `audit-refs`; non cambiare il formato `D-nnn`.

---

# 17. File previsti

Creare:

```text
scripts/rt_shared_id.py
docs/technical/workflow-parallel-claude.md
```

Creare un file test coerente con la struttura effettiva trovata nel repository, per esempio:

```text
tests/scripts/test_rt_shared_id.py
```

**Non assumere questo path se il repo ha una convenzione diversa: verificare prima.**

Modificare:

```text
AGENTS.md
CLAUDE.md
```

Valutare se integrare il nuovo gate in un documento di comandi locali già esistente; non creare duplicati.

Non modificare:

```text
.github/
```

e non introdurre CI senza decisione esplicita.

---

# 18. Acceptance criteria

La feature è accettata quando:

1. due worktree dello stesso clone possono eseguire contemporaneamente `reserve D`;
2. 20 allocazioni concorrenti producono 20 ID distinti;
3. nessun ID viene riutilizzato dopo reservation orfana;
4. `check` rileva un duplicato reale nel Decision Log;
5. `audit-refs` distingue correttamente eredità normale da collisione semantica;
6. il tool funziona su Windows;
7. nessuna dipendenza Python esterna è introdotta;
8. `AGENTS.md` vieta esplicitamente la scelta manuale del prossimo `D`;
9. la procedura raccomanda un worktree per sessione Claude;
10. i gate documentali esistenti restano verdi;
11. non viene introdotta CI;
12. non viene rinumerata nessuna decisione esistente solo per implementare questo sistema.

---

# 19. Verifica manuale consigliata

Aprire due terminali.

### Terminale A

```powershell
cd C:\dev\rt-wt-a
python scripts\rt_shared_id.py reserve D --reason "manual concurrency A"
```

### Terminale B, nello stesso momento

```powershell
cd C:\dev\rt-wt-b
python scripts\rt_shared_id.py reserve D --reason "manual concurrency B"
```

Atteso:

```text
A -> D-XYZ
B -> D-(XYZ+1)
```

o viceversa.

Mai lo stesso ID.

Poi:

```powershell
python scripts\rt_shared_id.py status
python scripts\rt_shared_id.py check
```

---

# 20. Commit proposti

Tenere i commit focalizzati.

Possibile sequenza:

```text
feat(tooling): add atomic shared decision id allocator
test(tooling): cover concurrent decision id allocation
docs(workflow): require worktree isolation for parallel Claude sessions
```

Non fare commit/push se l’utente non lo ha richiesto.

---

# 21. Passo successivo dopo questa implementazione

Dopo qualche giorno di utilizzo misurare:

- quante reservation `D` vengono create;
- quante restano orfane;
- se si verifica ancora almeno una collisione locale;
- se le collisioni residue arrivano solo da clone/PC diversi.

Solo se resta un problema reale, estendere lo stesso meccanismo agli altri contatori condivisi (`E...`) o valutare una reservation remota.

**Non generalizzare prima di aver dimostrato il caso `D-nnn`.**

---

# Istruzione finale a Claude

Implementa questo sistema sullo stato **corrente** del repository, non sulla SHA citata in apertura.

Prima:

1. leggi `AGENTS.md` e `CLAUDE.md`;
2. verifica `main`, branch, worktree e status;
3. rileggi la sezione finale del Decision Log sulle collisioni di contatore;
4. cerca eventuali script/test già esistenti che coprono ID o worktree;
5. proponi il diff minimo.

Durante:

- non rinumerare decisioni esistenti;
- non introdurre CI;
- non aggiungere dipendenze;
- non sostituire il formato `D-nnn`;
- non automatizzare operazioni Git distruttive;
- non nascondere il limite multi-PC.

Alla fine riporta:

**Risultato · File modificati · Comandi introdotti · Test concorrenza · Gate eseguiti · Limiti · Prossimo passo**.
