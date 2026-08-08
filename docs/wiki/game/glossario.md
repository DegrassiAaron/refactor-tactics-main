# Glossario

> **Tipo:** guida giocatore, non normativa

| Termine | Significato |
|---|---|
| **Match** | Una partita completa, dall'allestimento alla condizione di fine. |
| **Round** | Un ciclo completo `Planning → Commit → Resolution → Cleanup`. Nel codice storico è spesso chiamato `Turn`. |
| **Planning** | Fase in cui i giocatori preparano simultaneamente le proprie intenzioni. |
| **Ready** | Segnale che un giocatore ha terminato il piano. |
| **Commit** | Momento in cui gli intenti diventano canonici e non più liberamente modificabili. |
| **Resolution** | Esecuzione delle fasi `Prep → Dash → Blast → Move`. |
| **Prep** | Preparazioni e setup pre-offensivi. |
| **Dash** | Macro-fase della mobilità speciale pre-Blast. |
| **Blast** | Macro-fase principale di attacchi, abilità e controllo. |
| **Move** | Movimento normale; è l'ultima fase volontaria standard. |
| **Cleanup** | Chiusura del round: scadenze, cooldown, KO, aggiornamenti finali. |
| **Normal Action** | Azione scelta nel Planning che non richiede input durante la Resolution. |
| **Predictive / Delayed Action** | Azione interamente decisa nel Planning che risolve a un boundary successivo. |
| **Prepared Reaction** | Reazione armata nel Planning con risposta già determinata o unica. |
| **Fast Reaction** | Scelta live provocata da un evento esterno, per esempio `FIRE/HOLD`. |
| **Fast Action** | Scelta live limitata che continua una propria azione, non una reazione a un evento esterno. |
| **Phase Boundary** | Passaggio automatico tra macro-fasi. |
| **Decision Boundary** | Punto previsto dalle regole in cui può essere richiesta una scelta live limitata. |
| **Fallback** | Regola che dice cosa fare se un'azione non può più risolvere come pianificato. |
| **Facing** | Direzione verso cui l'unità è orientata; può influenzare difesa, percezione e reazioni. |
| **LOS** | Line of Sight: possibilità geometrica di vedere attraverso la mappa. |
| **Wet** | Stato legato all'acqua e alle interazioni elettriche. |
| **TurnLog** | Registro canonico degli eventi prodotti dalla simulazione. |

## Nota terminologica

Nei documenti nuovi si preferisce **Round** per il ciclo di tutte le unità. RefactorTactics non ha un «turno personale» in cui una singola unità gioca mentre le altre aspettano.

## Fonti normative

- `docs/gameplay/spec-durata-partita-e-scala-mappe.md`
- `docs/gameplay/spec-sequenza-turno.md`
