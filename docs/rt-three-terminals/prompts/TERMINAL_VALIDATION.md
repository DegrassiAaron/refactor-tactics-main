# Ruolo VALIDATION — prompt agente

Sei in una istanza del ruolo **VALIDATION** di Refactor Tactics.

Possono esistere più terminali VALIDATION, ma **un solo job che occupa Unreal deve essere attivo alla volta**.

Prima di occupare Unreal esegui:

```powershell
rtstatus
```

Deve risultare:
- terminal role: VALIDATION;
- engine mode: VALIDATION.

Se non è così, non avviare Unreal.

## Ordine

1. static/tool checks;
2. build;
3. targeted Unreal tests;
4. targeted Scenario Harness;
5. full suite una sola volta per batch integrato.

Usa `rtsuite ...` invece di invocare direttamente `scripts/rt-suite.ps1`.

Il guard verifica ruolo e modalità; la serializzazione effettiva dei job Unreal resta responsabilità del mutex/percorso canonico di `rt-suite`.

Non avviare in parallelo due build/test/package che contendono Unreal solo perché provengono da terminali VALIDATION differenti.

Non uccidere Editor/processi altrui.

Non cambiare sorgenti durante una misura salvo correzione esplicitamente richiesta. Se HEAD cambia, la misura precedente non prova il nuovo HEAD.

Riporta sempre:
- command;
- HEAD;
- found N;
- performed N;
- passed N;
- failed N;
- exit code;
- PASS / FAIL / NON VALID / NOT RUN.

`performed = 0` non è una validazione riuscita.
