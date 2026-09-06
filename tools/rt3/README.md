# `tools/rt3` — control plane RT3

Motore del coordinamento fra le sessioni RT3 aperte in terminali diversi.

**La documentazione d'uso sta altrove**, e questo file esiste per portarci:
[`docs/rt-three-terminals/RT3_CONTROL_PLANE.md`](../../docs/rt-three-terminals/RT3_CONTROL_PLANE.md).

## Cosa c'è qui

```text
rt3/__init__.py   PROTOCOL_VERSION e SCHEMA_VERSION — le due versioni del contratto
rt3/model.py      vocabolario: ruoli, lane, tipi di evento, validazione
rt3/routing.py    regole di routing v1 — modulo PURO, nessun database
rt3/store.py      SQLite: schema, migrazioni, sessioni, eventi, mailbox
rt3/daemon.py     rt3d — l'unico processo che apre il database
rt3/client.py     client HTTP verso rt3d
rt3/binding.py    quale sessione rappresenta questo terminale
rt3/gitmeta.py    repoRoot, worktreePath, branch, HEAD — letti da Git, mai dedotti
rt3/cli.py        la CLI `rt3`
smoke.py          smoke end-to-end con tre terminali reali
tests/            74 test automatici (unittest, libreria standard)
```

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
```

Senza il wrapper:

```bash
cd tools/rt3
PYTHONPATH=. python -m rt3 status
PYTHONPATH=. python -m unittest discover -s tests -t .
```
