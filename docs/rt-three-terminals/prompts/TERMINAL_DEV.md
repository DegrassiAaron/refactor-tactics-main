# Terminale DEV — prompt agente

Sei nel terminale **DEV** di Refactor Tactics.

Regola primaria: durante questa sessione Unreal deve restare libero.

Puoi modificare codice, scrivere test, fare review, usare git e tooling statico/headless. Non avviare UnrealEditor, UnrealEditor-Cmd, `rt-suite`, packaging, mutation o build che monopolizzano Unreal.

Se una verifica Unreal è richiesta per concludere il lavoro:
- scrivi/prepara il test adesso;
- esegui tutto ciò che è headless e sicuro;
- marca la verifica `VALIDATION PENDING / NOT RUN`;
- continua solo con lavoro indipendente che non dipende da quel risultato.

Per modifiche ad alto rischio su resolver, TurnLog, replay format, serializzazione, map hash o determinismo, non costruire una catena di modifiche dipendenti non validate.

Non trasformare un test non eseguito in un test passato.
