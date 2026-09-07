# `tools/rt3` — control plane RT3

Motore del coordinamento fra le sessioni RT3 aperte in terminali diversi.

**La documentazione d'uso sta altrove**, e questo file esiste per portarci:
[`docs/rt-three-terminals/RT3_CONTROL_PLANE.md`](../../docs/rt-three-terminals/RT3_CONTROL_PLANE.md).

## Cosa c'è qui

```text
rt3/__init__.py   PROTOCOL_VERSION, SCHEMA_VERSION, ROADMAP_SCHEMA_VERSION
rt3/model.py      vocabolario: ruoli, lane, tipi di evento, stati, validazione
rt3/routing.py    regole di routing v1 — modulo PURO, nessun database
rt3/store.py      SQLite: schema, migrazioni, sessioni, eventi, mailbox, roadmap
rt3/daemon.py     rt3d — l'unico processo che apre il database
rt3/client.py     client HTTP verso rt3d
rt3/binding.py    quale sessione rappresenta questo terminale
rt3/gitmeta.py    repoRoot, worktreePath, branch, HEAD — letti da Git, mai dedotti
rt3/cli.py        la CLI `rt3`

  -- roadmap orchestration: il PLAN e ciò che se ne deriva
rt3/yamlmini.py   parser del sottoinsieme YAML — PURO, rifiuta ciò che non capisce
rt3/roadmap.py    forma del documento, validazione, confine PLAN/RUNTIME
rt3/graph.py      dipendenze, cicli, ordine topologico, cammino critico — PURO
rt3/planner.py    readiness derivata, capacità, assegnazioni — PURO

smoke.py          smoke end-to-end con tre terminali reali
smoke_multi.py    smoke sui TRE checkout reali, un solo control plane
tests/            test automatici (unittest, libreria standard)
```

I quattro moduli della roadmap sono **puri**: prendono dati, ritornano dati. Non aprono il
database, non parlano in rete, non toccano Git. È la ragione per cui il cammino critico e
il piano si provano senza avviare niente.

## Vincoli

**Solo libreria standard.** `sqlite3`, `http.server`, `urllib`, `argparse`. Nessuna
dipendenza da installare: `AGENTS.md` §9 vieta di introdurre package manager senza una
decisione esplicita, e i tre workspace sono tre macchine da configurare se ce ne fosse
anche una sola.

**Non tocca il data plane.** Nessuna funzione qui esegue un comando che muta il
repository. Git è letto, mai scritto.

## Eseguirlo

```powershell
scripts\rt3.ps1 daemon start          # dalla radice del repository
scripts\rt3.ps1 -SelfTest             # i test
python tools\rt3\smoke.py             # lo smoke
python tools\rt3\smoke_multi.py       # lo smoke sui tre checkout
```

**Il parser YAML è scritto qui** e non importato: PyYAML non è libreria standard, e i tre
workspace possono finire su macchine diverse. Una roadmap che si apre su una workstation e
non sull'altra è il difetto peggiore, perché si manifesta lontano. Dove PyYAML *è*
installato, `tests/test_yamlmini.py` confronta i due parser sugli stessi documenti — così
il modulo resta onesto senza dipendere da quello.

Senza il wrapper:

```bash
cd tools/rt3
PYTHONPATH=. python -m rt3 status
PYTHONPATH=. python -m unittest discover -s tests -t .
```
