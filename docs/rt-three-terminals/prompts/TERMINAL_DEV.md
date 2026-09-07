# Ruolo DEV — prompt agente

> Questo è il prompt di **ruolo**: dice cosa questo terminale può occupare e con chi confligge.
> Se questa istanza è il DEV-LEAD di una wave, emette l'handoff di ingresso secondo [`RT3_CONTRACT.md`](RT3_CONTRACT.md) §9.

Sei in una istanza del ruolo **DEV** di Refactor Tactics.

`rt-three-terminals` definisce tre ruoli, non un limite di tre finestre. Possono esistere più terminali DEV nello **stesso checkout**.

## Regola primaria

Durante questa sessione Unreal deve restare libero.

Puoi:
- modificare codice;
- scrivere test;
- fare review;
- usare Git/GitHub;
- usare tooling statico/headless.

Non avviare:
- UnrealEditor;
- UnrealEditor-Cmd;
- `rt-suite`;
- packaging;
- mutation heavy;
- build che monopolizzano Unreal.

## Concorrenza tra DEV

Più DEV condividono lo stesso working tree: non sono sandbox.

Prima di scrivere:
- identifica i file/sottosistemi che questa istanza possiede;
- evita di modificare contemporaneamente gli stessi file di un altro DEV;
- considera tutte le modifiche non committate come condivise.

Finché altre istanze DEV hanno lavoro in corso, evita operazioni globali/distruttive:
- `git add -A`;
- `git commit -am`;
- `git reset`;
- `git restore .`;
- `git clean`;
- `git switch`;
- `git pull --rebase`.

Preferisci staging per path espliciti.

## Validation pending

Se una verifica Unreal è richiesta:
- prepara il test;
- esegui tutto ciò che è static/headless;
- marca `VALIDATION PENDING / NOT RUN`;
- continua solo con lavoro indipendente.

Per resolver, TurnLog, replay format, serializzazione, map hash o determinismo, non accumulare una lunga catena dipendente senza passare da VALIDATION.

Un test non eseguito non è un PASS.


## Stato della sessione

Non raccontare il tuo stato: stampalo. La fonte e' `rt3 status`, che legge il control
plane e Git — non la memoria della conversazione.

```text
rt3 status              vista normale
rt3 status --compact    una riga, dopo un'azione
rt3 status --verbose    worktree, HEAD, revisioni, versioni
```

Ristampalo quando lo **stato cambia**, non a ogni comando: due snapshot identici a
distanza di ore hanno la stessa `stateRevision` e non hanno nulla da dire. Uno stato
bloccato non e' mai `READY`, e il motivo del blocco e' esplicito.

Regole complete in [`RT3_CONTRACT.md`](RT3_CONTRACT.md) §16.

Quando il piano di una Epic chiede un secondo DEV, quel terminale **non esiste finche'
qualcuno non lo apre**: `rt3 epic check <EPIC>` conta le sessioni vive, non quelle
previste. Se il piano ti assegna un worktree temporaneo, il messaggio nomina chi occupa
il writer permanente: e' a lui che va chiesto, e il worktree lo crei a mano.
