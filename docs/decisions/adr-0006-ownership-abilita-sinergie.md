# ADR-0006 — Ownership delle abilità e sinergie emergenti

- **Stato:** Accepted
- **Data:** 2026-08-08
- **Decision Log:** D-029
- **Owner spec:** [`../gameplay/spec-ownership-abilita-interazioni-sinergie.md`](../gameplay/spec-ownership-abilita-interazioni-sinergie.md)

## Contesto

La documentazione e la Wiki descrivono spesso combinazioni utili fra personaggi (es. Gadget + Phase). Senza una regola di ownership, una combinazione può diventare accidentalmente una seconda fonte per abilità, numeri o logica di gameplay.

## Decisione

Le Ability/Action Definition sono possedute da un singolo contenuto. Le interazioni appartengono ai sistemi che governano status, superfici, geometria, movimento, informazione e reaction. Le sinergie fra personaggi sono **derivate**: esempi, guide e scenari che compongono definizioni esistenti.

Non vengono introdotti bonus impliciti di coppia/fazione né branch hard-coded fra HeroId per rappresentare una normale sinergia.

## Conseguenze

- Character Wiki: abilità per personaggio; sinergie linkate separatamente.
- Faction Wiki: identità + esempi, non kit condivisi.
- Scenario Harness: fixture di cooperazione, non implementazione della combo.
- Cataloghi: nessuna duplicazione dei valori nelle pagine di sinergia.
- Modding/data pipeline: contenuti componibili tramite ID/tag/stati, non dipendenze ad-hoc fra coppie.

## Non decisioni

Questo ADR non vieta abilità che richiedano esplicitamente una proprietà del source/target (es. tag, summon owner, link creato da una specifica ability). In quel caso il requisito deve essere parte della Ability Definition e avere counterplay/test espliciti.
