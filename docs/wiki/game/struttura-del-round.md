# Struttura del round

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-CORE-TURN -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-CORE-TURN` · Release: `v0.1` · Roadmap: `E2.2, E2.1`  
> Stato: **RELEASE_READY** · Gate: `7/8`  
> Scenario: `Visual.Core.PhaseOrder`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-CORE-TURN -->

## Sequenza completa

```text
PLANNING
   ↓
READY / COMMIT
   ↓
PREP
   ↓
DASH
   ↓
BLAST
   ↓
MOVE
   ↓
CLEANUP
```

## Planning

È la fase delle decisioni. Il giocatore prepara intenzioni e fallback senza conoscere il planning nemico.

## Ready / Commit

`Ready` segnala che il piano è completo. Il **Commit** lo rende definitivo per il round.

## Prep

Qui risolvono gli effetti che devono essere preparati prima della parte offensiva: stance, setup e altre azioni definite come preparazione.

## Dash

Qui risolvono gli spostamenti speciali che devono avvenire prima degli attacchi: Dash, Charge, Leap, Blink e Reposition quando previsti dal kit.

La conseguenza è enorme: un'unità può cambiare posizione **prima** di un attacco del Blast.

## Blast

È la fase principale di attacchi, abilità offensive, controllo e molte interazioni.

## Move

Il normale movimento di percorso risolve qui, **dopo** il Blast. È una delle regole più importanti del gioco.

## Cleanup

Chiude il round e prepara il successivo: durata degli status, danni o effetti di Cleanup, KO, cooldown e stato degli obiettivi vengono aggiornati secondo le rispettive regole.

## Segmenti e decision boundary

Una macro-fase può essere divisa in più **segmenti** quando una regola apre un `decision boundary`, per esempio una Fast Reaction.

Il modello è:

```text
snapshot → risoluzione del segmento → eventuale decision boundary
snapshot → segmento successivo → ...
```

Il boundary non è una nuova fase e non trasforma la Resolution in un secondo Planning.

## Phase boundary vs decision boundary

- **Phase boundary:** passaggio automatico fra macro-fasi; nessun input.
- **Decision boundary:** punto previsto dalle regole in cui può essere richiesta una scelta limitata.

## Fonti normative

- `docs/gameplay/spec-sequenza-turno.md`
- `docs/decisions/adr-0004-finestre-di-reazione.md`
