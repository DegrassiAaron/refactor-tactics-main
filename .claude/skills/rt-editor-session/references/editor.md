# Modalità Editor

Usa questa modalità per ispezionare o modificare asset senza eseguire il gameplay: UMG, HUD, widget, Blueprint, mappe, Data Asset e configurazioni Editor-only.

## Procedura

1. Individua nel repository la procedura supportata per avviare il progetto e connettere l'MCP, se disponibile.
2. Verifica che l'Editor aperto corrisponda al `.uproject` della worktree corrente.
3. Apri soltanto mappa e asset necessari al goal.
4. Per `inspect`, non salvare modifiche. Registra stato iniziale, proprietà rilevanti e prove.
5. Per `author`, fotografa o descrivi lo stato iniziale, dichiara il write-set e applica la modifica più piccola che soddisfa il goal.
6. Compila o valida gli asset modificati con gli strumenti realmente disponibili. Controlla errori, warning pertinenti e dipendenze rotte.
7. Salva esplicitamente solo il write-set e verifica quali file risultano modificati sul filesystem o in Git.

## Esito

`PASS` richiede che lo stato Editor osservato corrisponda al goal e che gli asset attesi siano gli unici asset salvati. Se il risultato deve essere provato durante il gameplay, questa modalità può preparare gli asset ma la verifica resta `NOT RUN`; proponi una successiva seduta `mode=pie`.
