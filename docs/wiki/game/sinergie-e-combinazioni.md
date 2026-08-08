# Sinergie e combinazioni

> **Tipo:** guida giocatore, non normativa  
> **Owner normativo:** [`../../gameplay/spec-ownership-abilita-interazioni-sinergie.md`](../../gameplay/spec-ownership-abilita-interazioni-sinergie.md)

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ENV-SYSTEMIC-COMBOS -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ENV-SYSTEMIC-COMBOS` · Release: `v0.1` · Roadmap: `E8 · CP 8.5`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Visual.Combat.WaterElectric`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ENV-SYSTEMIC-COMBOS -->

Le **abilità appartengono ai singoli personaggi**. RefactorTactics non raggruppa le abilità in kit di coppia, kit di fazione o pacchetti legati a una composizione specifica.

La sinergia nasce quando definizioni indipendenti interagiscono attraverso le normali regole del gioco: stati, superfici, geometria, movimento, cover, targeting, informazione e reaction.

## Tre livelli

### Kit del personaggio

Una abilità ha il proprio `AbilityId`/Action ID, owner, targeting, fase, costo, cooldown ed effetti. La pagina di un personaggio documenta **solo il suo kit**.

### Interazione sistemica

È una regola che esiste indipendentemente dal personaggio che la produce.

| Interazione | Esempio |
| --- | --- |
| `Wet → Electric` | una sorgente applica Wet; un effetto elettrico può leggerlo |
| `Push → Hazard` | displacement verso fuoco/acqua/ghiaccio |
| `Cover/Door → LOS` | una modifica topologica cambia tiro e visibilità |
| `Route Control → Prediction` | restringere rotte aumenta leggibilità di una Predictive Action |
| `Noise → Reaction` | un evento sonoro autorizzato può generare informazione/opportunity |

### Sinergia tattica

È un **esempio derivato** di più kit che sfruttano la stessa regola. Non crea una nuova abilità, un bonus automatico o un legame hard-coded fra personaggi.

## Esempi del roster

- **Flux + Riva:** Riva può applicare `Wet`; `Flux.LinearDischarge` legge `Wet`. La dipendenza è dallo **stato**, non da `Hero.Riva`.
- **Bastion + Vektor:** Bastion può restringere geometria/rotte; Vektor può beneficiare di percorsi più prevedibili. Nessun bonus di coppia.
- **Steel + Murdock:** protezione locale + settore di tiro è cooperazione fra due kit autonomi.
- **Aurora + Kwang:** terrain shaping + Anchor geometry è cooperazione fra regole ambientali e persistent entity.

## Fazioni

Le fazioni descrivono identità, filosofia, linguaggio visivo e affinità tattiche. **Non possiedono abilità condivise e non concedono `FactionSetBonus` impliciti.**

## Scenari

Uno scenario può dimostrare una sinergia specifica, ma è una **fixture**: seleziona personaggi/intenti e usa il normale percorso `Planning → Commit → Snapshot → Resolver → TurnLog`.

Uno scenario non deve contenere una seconda implementazione dell'abilità né branch competitivi come `if Flux && Riva`.

## Regola editoriale Wiki

Quando una pagina cita una combinazione:

1. nomina prima la regola sistemica;
2. presenta i personaggi come esempio;
3. collega ai singoli kit;
4. non attribuisce abilità a coppia/fazione;
5. non inventa bonus di composizione.
