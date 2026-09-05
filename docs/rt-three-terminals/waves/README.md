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
  contrib/
  evidence/
```

`<sha7>` sono i primi 7 caratteri del `PRODUCED_SHA` del ruolo che emette.

`<feature-slug>` è stabile per l'intera wave e compare in `WAVE_ID` e in ogni `FINDING_ID`.

## contrib/

Contiene i **contributi** delle istanze DEV della wave che non sono DEV-LEAD:

```text
contrib/DEV-MAIN-<PID>-<nn>.md
contrib/DEV-TEST-<PID>-<nn>.md
```

L'identità di un contributo **non** deriva dallo SHA, a differenza di quella di un handoff. Nel caso nominale un'istanza DEV non committa: `PRODUCED_SHA` resta uguale a `BASE_SHA` per tutte, e due istanze dello stesso ruolo scriverebbero lo stesso nome, con la seconda che sovrascrive la prima.

- `<PID>` è il PID dell'istanza, lo stesso mostrato dal prompt `[DEV:PID]`;
- `<nn>` è un contatore a due cifre **per istanza**, monotono crescente.

I contributi sono append-only. Non modificare né cancellare quello di un'altra istanza: una correzione è un contributo nuovo con `SUPERSEDES:`.

Un contributo non è un handoff. I tre punti fissi della catena restano DEV-LEAD, EDITOR e VALIDATION: DEV-LEAD legge i contributi, li ordina per `CREATED` e li consolida in un unico `RT3-DEVLEAD-<sha7>.md` con un solo `WRITE_SET` e un solo `PRODUCED_SHA`.

Un contributo non porta verdetti di §7 del contratto: le istanze DEV non possiedono lo strumento che prova quei sistemi.

Sono su disco per la stessa ragione degli handoff: DEV-LEAD non deve ricostruire dal contesto della chat ciò che un'altra sessione ha prodotto.

Prompt: [`../prompts/WAVE_DEV_MAIN.md`](../prompts/WAVE_DEV_MAIN.md) · [`../prompts/WAVE_DEV_TEST.md`](../prompts/WAVE_DEV_TEST.md).

## evidence/

Contiene gli artefatti a cui puntano gli `EVIDENCE_REF`: dump TurnLog, screenshot, hash di replay, estratti di log, csv di profiling.

Un `EVIDENCE_REF` che punta a un file assente è malformato: il sistema corrispondente si legge `BLOCKED`.

## Ciclo di vita

Gli handoff di una wave chiusa restano. Sono la traccia di cosa è stato misurato, da chi, su quale SHA.

Non riscrivere un handoff emesso. Una nuova misura produce un nuovo file con il nuovo `PRODUCED_SHA`.

Non spostare qui documenti di handoff storici: `docs/archive/` e `docs/roadmap/plans/` hanno owner e semantica diversi, e non sono autorità — vedi `CLAUDE.md` §1.
