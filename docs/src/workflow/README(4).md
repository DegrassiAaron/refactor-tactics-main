# RefactorTactics — Full Project Graph Migration Bundle

Questo bundle porta il piano dal thin slice a:

1. migrazione completa v0.1;
2. migrazione v0.2;
3. estensione schema release;
4. riallineamento v0.3;
5. popolamento v0.4.

## File

- `RefactorTactics_ProjectGraph_FullMigration_v01-v04_Claude.md`
  - handoff principale.
- `release-alignment.proposed.yaml`
  - mapping release/Epic e reassignment sicuri.
- `execution-graph-migration-rules.proposed.yaml`
  - regole di discovery/ownership.
- `release-alignment-conflicts.md`
  - conflitti/gap da NON risolvere a intuito.
- `migration-checklist-v01-v04.md`
  - checklist operativa.

## Nota importante

La “migrazione completa v0.1” NON significa copiare a mano 74 Feature e 100 checkpoint
in un altro YAML.

Il generator deve scoprirli dagli owner esistenti; `execution-graph.yaml` contiene solo le
relazioni/override che non sono derivabili con sicurezza.
