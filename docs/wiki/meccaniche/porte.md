# Porte

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-MAP-INTERACTIVE-EDGES -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-MAP-INTERACTIVE-EDGES` · Release: `v0.1` · Roadmap: `E9 · CP 9.3`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Visual.Map.ClosedDoor`  
> Dato, regole di blocco e mutazione del bordo sono implementati e testati; l'**interazione del giocatore** (`Interact`/`Activate`) arriva con E10.  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-MAP-INTERACTIVE-EDGES -->

## In breve

Una porta è una barriera su un **bordo tra due celle adiacenti**. Non è una cella e non è un ponte.

Questa distinzione è importante: una porta **toglie** un passaggio planare che normalmente esisterebbe.

## Stati

| Stato | Effetto |
|---|---|
| **Open** | si passa e si vede attraverso |
| **Closed** | blocca passaggio e vista; può essere riaperta |
| **Locked** | come Closed, ma non si apre con una normale mutazione |
| **Destroyed** | aperta definitivamente; stato terminale |

## Porta singola e portone largo

Un portone largo può essere composto da più bordi con lo stesso `DoorId`. Tutti i bordi del gruppo cambiano stato come **un singolo evento**.

Questo permette porte larghe o anche varchi non perfettamente rettilinei senza introdurre una regola speciale.

## Una porta chiusa durante il round

Le modifiche topologiche raccolte nel Blast vengono applicate prima del Move. Quindi una porta può essere aperta quando pianifichi il percorso e risultare chiusa quando il Move prova davvero ad attraversarla.

In quel caso il personaggio **non attraversa un path fantasma**: il percorso viene troncato e l'unità si ferma nell'ultima cella ancora valida.

Il reason code previsto è `BlockedByTopology`.

## Porta e muro sullo stesso bordo

La regola è restrittiva: se sul bordo esistono sia una porta aperta sia una High Cover/muro, il bordo resta bloccato. Aprire una porta non crea un buco dentro un muro pieno.

## Stato reale per il giocatore

Il sistema runtime è presente e testato. La normale azione giocatore che attiva porte adiacenti appartiene però a **E10.1**: finché quella parte non è completata, non bisogna presentare tutte le porte come interattive in una build giocabile.

## Cosa deve ricordare il giocatore

> Una porta cambia **il grafo**, non soltanto la grafica della mappa.

## Fonti normative

- `docs/gameplay/spec-porte-cp93.md`
- `docs/roadmap/roadmap-v0.1.md` §E10
