# Evidenze, cleanup e report

## Stati ammessi

- `PASS`: il goal è stato verificato con evidenza oggettiva.
- `FAIL`: la prova è stata eseguita e il risultato osservato contraddice il goal o contiene un errore pertinente.
- `BLOCKED`: la prova non può proseguire per ownership, dipendenze, ambiente o input indispensabile mancante.
- `NOT RUN`: un controllo previsto non è stato eseguito o non apparteneva alla seduta completata.

Non usare `PASS WITH WARNINGS` per nascondere un requisito non verificato. Registra separatamente finding e controlli `NOT RUN`.

## Evidenza minima

Associa ogni conclusione a una o più prove:

- nome e risultato di un'assertion o test;
- estratto breve e localizzabile del log;
- screenshot con momento e contesto descritti;
- stato/proprietà Editor verificata;
- replay o output deterministico;
- confronto atteso/ottenuto;
- elenco esatto degli asset o file modificati.

Un acknowledgement MCP, l'apertura dell'Editor o l'assenza apparente di errori non costituiscono da soli evidenza di successo.

## Ownership e cleanup

Traccia quali processi sono stati avviati dalla seduta. Arresta soltanto quelli posseduti. Se la chiusura normale fallisce, raccogli prima stato e log; non usare comandi di terminazione ampi, wildcard o nomi di processo come bersaglio indiscriminato.

Alla fine verifica:

- PIE/run terminata;
- Editor avviato dalla seduta chiuso;
- processi posseduti residui;
- asset dirty non intenzionali;
- stato Git e file modificati;
- log o evidenze disponibili.

## Report finale

```text
EDITOR SESSION REPORT

Goal:
Mode / Scope / Intent:
Issue:
Worktree / Branch / SHA:
Project / Map / Scenario:
Write-set:
Checks executed:
Evidence:
Result: PASS | FAIL | BLOCKED | NOT RUN
Findings fuori scope:
Files changed:
PIE stopped:
Owned Editor closed:
Owned processes remaining:
Checks not run:
Recommended next action:
```

Non omettere le righe di cleanup. Usa `N/A` quando una voce non è applicabile.
