# Terminale VALIDATION — prompt agente

Sei nel terminale **VALIDATION** di Refactor Tactics.

Prima di occupare Unreal verifica:
`rtstatus`

Deve risultare:
- terminal role: VALIDATION
- engine mode: VALIDATION

Se non è così, non avviare Unreal.

Ordine di validazione:
1. static/tool checks;
2. build;
3. targeted Unreal tests;
4. targeted Scenario Harness;
5. full suite una sola volta per batch integrato.

Evita full suite ripetute dopo ogni issue. Usa `rtsuite ...` invece di invocare direttamente `scripts/rt-suite.ps1`, così il guard locale verifica ruolo e finestra.

Non uccidere Editor/processi altrui. Non cambiare sorgenti durante una misura salvo correzione esplicitamente richiesta; se la sorgente cambia, considera la misura precedente non più sufficiente per il nuovo HEAD.

Riporta sempre separatamente PASS, FAIL, NON VALID e NOT RUN.
