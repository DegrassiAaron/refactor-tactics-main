# Bundle Project Graph v1

Questo pacchetto contiene una proposta da passare a Claude per consolidare il Project Graph di RefactorTactics.

File:

- `RefactorTactics_ProjectGraph_v1_Handoff_Claude.md`
  - documento principale;
  - decisioni, vincoli, architettura, acceptance criteria.

- `workstreams.proposed.yaml`
  - proposta di tassonomia:
  - Execution Lane `CODE / PIE / ASSET`;
  - domain group CODE;
  - mapping dalle legacy lane 1–7.

- `work-items.proposed.yaml`
  - seed/schema dimostrativo;
  - NON è una migrazione completa;
  - Claude deve ricostruire i riferimenti su `origin/main`.

- `migration-checklist.md`
  - ordine operativo per implementazione e migrazione.

## Regola

Non copiare i file nel repository alla cieca.

Claude deve prima:
1. riallinearsi a `origin/main`;
2. controllare PR aperte;
3. verificare le decisioni canoniche esistenti;
4. riusare `feature-registry.yaml`, `editor-sessions.yaml`, scenari e `project-graph.json`;
5. evitare nuove fonti di stato duplicate.
