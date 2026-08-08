# Visibilità, rumore e informazione

> **Stato nel gioco:** modello deciso per la v0.1, implementazione E13/E16 non ancora completa
> **Tipo:** guida giocatore, non normativa

## Perché l'informazione conta

RefactorTactics non vuole che il giocatore conosca automaticamente tutto ciò che l'avversario sta facendo. La posizione, il facing, la linea di vista e il rumore devono creare **informazione parziale**, non caos casuale.

## Tre livelli di conoscenza

Il modello ridotto della v0.1 usa:

- **Nascosto:** la squadra non ha contatto utile;
- **Contatto Incerto:** sa che qualcosa è presente, ma non ha un bersaglio pienamente affidabile;
- **Rilevato:** il bersaglio è noto alla squadra;
- **Ultimo Contatto:** memoria temporanea dell'ultima posizione conosciuta.

L'identificazione completa e sistemi di stealth più profondi restano fuori dallo slice iniziale.

## Vista

La LOS geometrica e il raggio di vista sono concetti separati. La direzione consolidata aggiunge anche il **facing**:

- piena percezione nell'arco frontale fino al valore di Vista;
- consapevolezza a 360° entro **2 celle**.

## Conoscenza di squadra

Il targeting usa la conoscenza **della squadra**, non soltanto quella dell'unità che spara. Un alleato può quindi fare da osservatore per un altro personaggio.

## Rumore

Il rumore è il secondo canale di percezione. Non deve rivelare automaticamente la posizione esatta: può produrre un **Contatto Incerto**.

La propagazione prevista usa il grafo della mappa e costi interi, non una semplice sfera 3D attorno al personaggio.

## Fumo

Il fumo limita il contatto/targeting a corto raggio: la regola attuale usa un cap di **2 celle** attraverso il fumo.

## Stato reale nella v0.1

Questa pagina descrive il **modello deciso**, non un sistema già completamente giocabile. LOS esiste; conoscenza parziale, facing percettivo e rumore sono ancora nella catena di implementazione E16 → E13 → E14.

## Fonti normative

- `docs/gameplay/brief-conoscenza-parziale.md`
- `docs/decisions/adr-0005-orientamento.md`
- `docs/gameplay/spec-terreni-e8.md`
