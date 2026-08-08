# Facing e direzionalità

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-MAP-FACING -->

> ⚠️ **Progettata, non implementata.** Questa pagina descrive una meccanica **decisa e documentata** che il gioco **non esegue ancora**: oggi non è giocabile. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-MAP-FACING` · Release: `v0.1` · Roadmap: `E16.1, E16.2`  
> Stato: **SPECIFIED** · Gate: `1/8`  
> Scenario: `—`  
> Decisione accettata (ADR-0005) ma **non implementata**: quello che leggi qui descrive come funzionera', non come funziona oggi.  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-MAP-FACING -->

## Che cos'è il facing

Il facing è la direzione logica verso cui una unità guarda su una griglia esagonale: una delle **sei direzioni** possibili.

La direzione di design è farlo contare per tre sistemi con **la stessa geometria frontale**:

- difesa;
- percezione;
- reazioni direzionali / Overwatch.

## Come verrà scelto

### Dash, Charge e Leap lineari

Il facing deriva automaticamente dalla direzione del movimento.

### Move normale / Sprint

Dopo l'ultimo passo il giocatore può scegliere fra **tre direzioni**:

- quella dell'ultimo passo;
- le due direzioni adiacenti.

### Unità ferma

Se non si muove, può scegliere liberamente fra tutte le sei direzioni senza consumare uno slot d'azione.

## Quando ha effetto

Il Move normale è l'ultima fase volontaria. Di conseguenza il facing scelto con il Move di un round influenza soprattutto il **round successivo**, finché un nuovo spostamento non lo cambia.

Una mobilità speciale nel Dash del round corrente può invece aggiornare il facing prima del Blast.

## Movimento forzato

Una unità spinta si orienta verso la **sorgente dell'ultimo spostamento forzato**. Uno spostamento ambientale senza sorgente lascia il facing invariato.

## Arco frontale

L'arco frontale è un cono esagonale di circa **120°**, ottenuto dalla stessa primitiva `HexCone` usata dal progetto.

Questa singola forma deve essere condivisa da difesa, vista e Overwatch: niente tre definizioni diverse di “davanti”.

## Difesa prevista

Un attacco proveniente fuori dall'arco frontale annulla:

- riduzione da **Low Cover (-10)**;
- riduzione da **Guard (-15)**.

Scudi, Deflect, Brace e protezioni non geometriche restano invece valide da ogni direzione.

## Percezione prevista

- vista piena nell'arco frontale fino a `VisionRange`;
- consapevolezza a **360° entro 2 celle**;
- oltre 2 celle, ciò che è dietro non viene percepito soltanto perché è entro il range numerico.

## Overwatch previsto

La direzione sorvegliata deriva dal facing. Non si sceglie un secondo orientamento separato per l'Overwatch.

## Perché è ancora marcato “non implementato”

L'ADR è stato accettato, ma la roadmap indica ancora la catena **E16 → E13 → E14**. La Wiki non deve far credere che il backstab/facing sia già attivo nella build corrente.

## Fonti normative

- `docs/decisions/adr-0005-orientamento.md`
