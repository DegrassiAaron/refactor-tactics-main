# Regole fondamentali

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-CORE-TURNLOG -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-CORE-TURNLOG` · Release: `v0.1` · Roadmap: `E12 · CP 12.1, 12.6`  
> Stato: **RELEASE_READY** · Gate: `6/7`  
> Scenario: `Visual.Core.PhaseOrder`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-CORE-TURNLOG -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-CORE-DETERMINISM -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-CORE-DETERMINISM` · Release: `v0.1` · Roadmap: `E12 · CP 12.1, 12.6`  
> Stato: **INTEGRATED** · Gate: `5/7`  
> Scenario: `RT_Showcase_Relay_v01`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-CORE-DETERMINISM -->

## Le regole da sapere prima di giocare

1. **Tutti pianificano contemporaneamente.** Non esiste il turno individuale di una singola unità.
2. **Il piano viene committato.** Dopo il Commit non puoi riscriverlo liberamente in base a ciò che scopri durante la Resolution.
3. **Le macro-fasi hanno un ordine fisso:** `Prep → Dash → Blast → Move`.
4. **Il Move normale è l'ultima azione volontaria standard.** Non esiste la sequenza libera `Move → Attack`.
5. **Dash non significa Move.** Dash, Charge, Leap, Blink e Reposition sono mobilità speciali che possono accadere prima del Blast.
6. **Attacco, Ability e Overwatch competono per la stessa scelta offensiva**, salvo eccezioni esplicitamente dichiarate da un personaggio.
7. **Le reazioni devono essere preparate oppure rese disponibili dalle regole.** Non puoi interrompere liberamente qualunque cosa in qualunque momento.
8. **La mappa ha regole vere.** Un terreno non è solo un colore: può cambiare costo di movimento, LOS, propagazione e rischio.
9. **L'informazione è una risorsa.** Planning avversario e conoscenza incompleta non devono essere trasformati in informazione gratuita.
10. **La simulazione è autorevole e deterministica.** Le animazioni mostrano il risultato, non lo decidono.

## Una conseguenza importante

Un attacco pianificato può essere perfetto rispetto alla posizione attuale del bersaglio e fallire rispetto alla posizione che avrà **nella fase in cui l'attacco risolve**. Viceversa, il normale Move arriva dopo il Blast: un nemico che ha pianificato soltanto un Move standard non scapperà da un colpo del Blast prima che quel colpo venga risolto.

## Fonti normative

- `docs/gameplay/spec-sequenza-turno.md`
- `docs/gameplay/brief-azioni-generiche-overwatch.md`
- `docs/product/piano-canonico-mvp.md`
