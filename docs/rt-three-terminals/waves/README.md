# RT3 — handoff persistiti

Qui vivono gli handoff di wave prodotti dai ruoli DEV-LEAD, EDITOR e VALIDATION.

Formato e campi obbligatori: [`../prompts/RT3_CONTRACT.md`](../prompts/RT3_CONTRACT.md) §9.

Esempio compilato: [`../prompts/RT3_EXAMPLE.md`](../prompts/RT3_EXAMPLE.md).

## Perché su disco

Un handoff che vive solo nella conversazione non esiste per il ruolo successivo.

Il contesto di una sessione si azzera. L'evidenza descritta a parole non è riverificabile. Un ruolo a valle che riceve testo non rileggibile può solo emettere `BLOCKED`.

## Struttura

```text
waves/<feature-slug>/
  RT3-DEVLEAD-<sha7>.md
  RT3-EDITOR-<sha7>.md
  RT3-VALIDATION-<sha7>.md
  evidence/
```

`<sha7>` sono i primi 7 caratteri del `PRODUCED_SHA` del ruolo che emette.

`<feature-slug>` è stabile per l'intera wave e compare in `WAVE_ID` e in ogni `FINDING_ID`.

## evidence/

Contiene gli artefatti a cui puntano gli `EVIDENCE_REF`: dump TurnLog, screenshot, hash di replay, estratti di log, csv di profiling.

Un `EVIDENCE_REF` che punta a un file assente è malformato: il sistema corrispondente si legge `BLOCKED`.

## Ciclo di vita

Gli handoff di una wave chiusa restano. Sono la traccia di cosa è stato misurato, da chi, su quale SHA.

Non riscrivere un handoff emesso. Una nuova misura produce un nuovo file con il nuovo `PRODUCED_SHA`.

Non spostare qui documenti di handoff storici: `docs/archive/` e `docs/roadmap/plans/` hanno owner e semantica diversi, e non sono autorità — vedi `CLAUDE.md` §1.
