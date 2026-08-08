# Esempio di un round

> **Stato nel gioco:** esempio esplicativo basato sull'ordine canonico delle fasi
> **Tipo:** guida giocatore, non normativa

Immagina quattro unità. Non serve conoscere tutte le abilità: l'esempio serve a capire **quando** accadono le cose.

## Planning

La Squadra A pianifica:

- **Vektor:** una mobilità speciale in Dash;
- **Flux:** un attacco che risolve nel Blast;
- entrambe le unità hanno anche un normale percorso di Move per la fine del round.

La Squadra B pianifica due normali Move e un attacco.

Nessuna squadra vede il piano completo dell'altra.

## Commit

Tutti confermano. Ora i piani sono bloccati.

## Dash

Vektor esegue prima la sua mobilità speciale. La sua posizione per il Blast può quindi essere diversa da quella vista durante il Planning.

Le unità che hanno pianificato **solo Move normale non si muovono ancora**.

## Blast

Flux e gli altri attaccanti risolvono le loro azioni offensive secondo priorità e regole di targeting.

Questo significa che un nemico che aveva pianificato un normale Move è ancora nella posizione pre-Move durante il Blast, salvo che un Dash, una reazione o uno spostamento forzato lo abbia già mosso.

## Move

Solo adesso vengono risolti i percorsi normali. Collisioni, celle bloccate e cambi della topologia possono fermare un percorso prima della destinazione prevista.

## Cleanup

Il gioco aggiorna gli effetti di fine round e passa al Planning successivo.

## Cosa insegna l'esempio

- Pianificare un Move non permette di schivare automaticamente un attacco del Blast.
- Una mobilità speciale pre-Blast può invece cambiare la situazione prima degli attacchi.
- Il valore tattico nasce dal prevedere **la fase**, non soltanto la destinazione finale.

## Fonti normative

- `docs/gameplay/spec-sequenza-turno.md`
- `docs/balance/RT_ActionCatalog_v0.1.md`
