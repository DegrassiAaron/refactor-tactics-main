# Modalità PIE

Usa questa modalità per verificare comportamento e presentazione nel runtime dell'Editor: gameplay, HUD, targeting preview, movimento, Overwatch, scenari e replay.

## Contratto dello scenario

Prima di avviare PIE, definisci:

- mappa e scenario;
- setup iniziale necessario;
- azioni da eseguire;
- risultato atteso osservabile;
- log, assertion, screenshot o replay che dimostrano l'esito;
- limite della prova e criterio di arresto.

Se uno scenario tracciato esiste, usalo. Non sostituirlo con un setup improvvisato senza dichiararlo. Preferisci setup automatico e analisi log quando il progetto li supporta; altrimenti elenca con precisione i passaggi manuali.

## Procedura

1. Verifica mappa, GameMode, dati e asset richiesti prima di entrare in PIE.
2. Registra il momento di inizio o un marker utile a isolare i log della prova.
3. Avvia una sola istanza PIE, salvo requisito esplicito multiplayer.
4. Esegui soltanto le azioni del contratto e raccogli evidenze durante i passaggi significativi.
5. Confronta risultato atteso e risultato osservato, includendo reason code o catena causale quando disponibili.
6. Termina PIE prima di interrogare stato Editor-only o salvare asset.
7. Se una query MCP restituisce vuoto durante PIE, ripetila fuori da PIE prima di concludere che la capability o il dato non esistono.

## Esito

`PASS` richiede un comportamento osservato coerente con il risultato atteso e almeno un'evidenza oggettiva. Crash, timeout, risultato differente o causalità incoerente producono `FAIL`. Un test non eseguito per mancanza di mappa, scenario, tool o ownership produce `BLOCKED` oppure `NOT RUN`, non `PASS`.
