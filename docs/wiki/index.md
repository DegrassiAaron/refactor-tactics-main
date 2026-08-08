# RefactorTactics — Guida al gioco

**RefactorTactics** è un tattico a turni simultanei: tutti pianificano nello stesso momento, poi il gioco risolve le azioni secondo una sequenza comune e deterministica.

Se vuoi capire il gioco in pochi minuti, leggi in quest'ordine:

- **[Che cos'è RefactorTactics](game/che-cose-refactortactics.md)** — idea generale e cosa lo rende diverso.
- **[Come si gioca](game/come-si-gioca.md)** — flusso completo di una partita.
- **[Struttura del round](game/struttura-del-round.md)** — Planning → Prep → Dash → Blast → Move → Cleanup.
- **[Esempio di un round](game/esempio-di-round.md)** — il flusso visto in pratica.
- **[L'avversario controllato dal gioco](game/avversario-bot.md)** — come ragiona il bot, e perché non bara.
- **[Personaggi](../characters/index.md)** — roster v0.1 e v0.2.

Poi puoi approfondire azioni, movimento, ambiente, reazioni, visibilità e condizioni di vittoria dalle pagine collegate nel [README della Wiki](README.md).

## Manuale delle meccaniche

Hai già capito il loop e vuoi cercare una regola precisa? Vai all'[indice delle meccaniche](meccaniche/index.md).

## Roster e asset Paragon

- [Personaggi RefactorTactics](../characters/index.md)
- [Indice dei 38 asset hero Paragon](../characters/paragon.md)

## Sinergie e fazioni

- [Sinergie e combinazioni](game/sinergie-e-combinazioni.md)
- [Fazioni](fazioni/index.md)

## A che punto è il gioco

- **[Stato delle feature](feature-status.md)** — cosa esiste davvero, feature per feature, con lo stato
  derivato da gate verificabili.

Questa Wiki descrive il gioco **come è progettato**: alcune meccaniche che leggi qui sono decise e
documentate ma non ancora implementate. Ogni pagina lo dichiara nel proprio riquadro «Stato di sviluppo», e la
tabella qui sopra le raccoglie tutte. Nessuno di quei riquadri è scritto a mano: si generano dal
[Feature Registry](../roadmap/feature-registry.md).
