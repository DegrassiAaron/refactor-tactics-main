# Modalità Automation

Usa questa modalità per Unreal Automation Framework, commandlet o test mirati già supportati dal repository. Non interpretare automaticamente `automation` come build, cook o package completo.

## Procedura

1. Scopri i test e i launcher reali dal repository, dalla documentazione e dall'help dei comandi disponibili.
2. Seleziona il test più stretto che verifica il goal. Riporta il suo identificatore esatto.
3. Registra worktree, branch, SHA, configurazione, target e binario usato.
4. Esegui il test con timeout proporzionato. Non trasformare un hang in attesa indefinita.
5. Conserva exit code, riepilogo, assertion fallite e percorso dei log o artefatti.
6. Se il test richiede un Editor, applica le stesse regole di ownership e cleanup della skill principale.
7. Non avviare automaticamente build, cook o package costosi se non richiesti dal goal o necessari al test; dichiara prima il costo e la dipendenza.

## Esito

`PASS` richiede exit code e assertion coerenti con il successo del test. Un comando avviato correttamente ma privo di un risultato verificabile non è `PASS`. Se il test pertinente non esiste, usa `NOT RUN` e proponi il minimo test da aggiungere senza implementarlo fuori scope.
