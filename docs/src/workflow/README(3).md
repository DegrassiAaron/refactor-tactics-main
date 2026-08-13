# RefactorTactics Project Graph v1 — Implementation Bundle

Questo bundle è la versione **consolidata dopo l'audit del codice reale del Control Center**.

## Decisione principale

Usare:
- `feature-registry.yaml` per le Feature;
- `editor-sessions.yaml` per lavoro umano;
- un solo nuovo `execution-graph.yaml` per la topologia di esecuzione;
- `project-graph.json` come contratto generato;
- Control Center come sola UI.

La vecchia proposta `workstreams.yaml + work-items.yaml` è **superseded** da questo bundle.

## File

- `RefactorTactics_ProjectGraph_v1_Implementation_Claude.md`
  - handoff principale.
- `execution-graph.proposed.yaml`
  - schema + thin slice reale.
- `project-graph.execution-contract.example.json`
  - forma target generata.
- `control-center-execution-map-spec.md`
  - UI.
- `test-matrix.md`
  - test richiesti.

## Prima di applicare

Claude deve:
1. aggiornarsi a `origin/main`;
2. controllare la PR #734 o eventuali PR successive;
3. misurare baseline;
4. non modificare generated a mano;
5. implementare il thin slice prima della migrazione massiva.
