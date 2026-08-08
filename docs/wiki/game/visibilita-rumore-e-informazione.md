# Visibilità, rumore e informazione

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-PERCEPTION-MEMORY -->

> ⚠️ **Progettata, non implementata.** Questa pagina descrive una meccanica **decisa e documentata** che il gioco **non esegue ancora**: oggi non è giocabile. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-PERCEPTION-MEMORY` · Release: `v0.1` · Roadmap: `E13.2`  
> Stato: **SPECIFIED** · Gate: `1/9`  
> Scenario: `—`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-PERCEPTION-MEMORY -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-PERCEPTION-NOISE -->

> ⚠️ **Progettata, non implementata.** Questa pagina descrive una meccanica **decisa e documentata** che il gioco **non esegue ancora**: oggi non è giocabile. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-PERCEPTION-NOISE` · Release: `v0.1` · Roadmap: `E13.3`  
> Stato: **SPECIFIED** · Gate: `1/9`  
> Scenario: `Spec.Perception.HeardNotSeen`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-PERCEPTION-NOISE -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-PERCEPTION-VISION -->

> ⚠️ **Progettata, non implementata.** Questa pagina descrive una meccanica **decisa e documentata** che il gioco **non esegue ancora**: oggi non è giocabile. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-PERCEPTION-VISION` · Release: `v0.1` · Roadmap: `E13.1, E13.2`  
> Stato: **SPECIFIED** · Gate: `1/9`  
> Scenario: `—`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-PERCEPTION-VISION -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-PERCEPTION-TEAM-KNOWLEDGE -->

> ⚠️ **Progettata, non implementata.** Questa pagina descrive una meccanica **decisa e documentata** che il gioco **non esegue ancora**: oggi non è giocabile. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-PERCEPTION-TEAM-KNOWLEDGE` · Release: `v0.1` · Roadmap: `E13.1`  
> Stato: **SPECIFIED** · Gate: `1/9`  
> Scenario: `—`  
> Modello deciso per la v0.1; l'implementazione (E13, dopo E16) **non e' iniziata**: oggi la vista non decide nulla.  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-PERCEPTION-TEAM-KNOWLEDGE -->

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
