# Azioni e movimento

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ACTION-SUPERS -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ACTION-SUPERS` · Release: `v0.2` · Roadmap: `—`  
> Stato: **IMPLEMENTING** · Gate: `0/8`  
> Scenario: `—`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ACTION-SUPERS -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ACTION-COOLDOWNS -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ACTION-COOLDOWNS` · Release: `v0.1` · Roadmap: `E4 · CP 4.4`  
> Stato: **TESTABLE** · Gate: `5/8`  
> Scenario: `—`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ACTION-COOLDOWNS -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ACTION-DASH-DISPLACEMENT -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ACTION-DASH-DISPLACEMENT` · Release: `v0.1` · Roadmap: `E2 · CP 2.5, 4.5`  
> Stato: **RELEASE_READY** · Gate: `7/8`  
> Scenario: `Visual.Combat.PushResistance`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ACTION-DASH-DISPLACEMENT -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ACTION-MOVE-PROFILES -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ACTION-MOVE-PROFILES` · Release: `v0.1` · Roadmap: `E4 · CP 4.2, 4.5`  
> Stato: **RELEASE_READY** · Gate: `7/8`  
> Scenario: `Visual.Movement.Charge`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ACTION-MOVE-PROFILES -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ACTION-GENERIC -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ACTION-GENERIC` · Release: `v0.1` · Roadmap: `E4 · CP 4.4, 4.6, 4.7`  
> Stato: **IMPLEMENTING** · Gate: `3/8`  
> Scenario: `Combat.BasicAttack`  
> Sei azioni generiche su sette sono a catalogo e nel runtime: manca **Overwatch**, che arriva con E14.  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ACTION-GENERIC -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ACTION-ENGINE -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ACTION-ENGINE` · Release: `v0.1` · Roadmap: `E4 · CP 4.1, 4.3, 4.8`  
> Stato: **RELEASE_READY** · Gate: `7/8`  
> Scenario: `Movement.Collision`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ACTION-ENGINE -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-MAP-PATHFINDING -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-MAP-PATHFINDING` · Release: `v0.1` · Roadmap: `E2 · CP 2.2`  
> Stato: **RELEASE_READY** · Gate: `6/7`  
> Scenario: `Movement.Basic`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-MAP-PATHFINDING -->

## Le azioni generiche

La grammatica comune del gioco è:

```text
Wait · BasicAttack · Interact · Brace · Move · Overwatch
```

Il kit di un personaggio aggiunge abilità specifiche sopra questa base.

### Wait

Rinunci a un'azione offensiva. Può comunque avere senso per mantenere posizione, facing o preparazioni consentite.

### Basic Attack

L'attacco base del personaggio. Danno, range e forma dipendono dall'eroe.

### Interact

Interazione con elementi della mappa come porte, console, ponti o obiettivi quando le relative regole lo permettono.

### Brace

Azione difensiva universale prevista dal modello. I numeri definitivi sono ancora da playtestare.

### Move

Il movimento normale segue un percorso e risolve **dopo il Blast**.

### Overwatch

Rinunci all'azione offensiva immediata per sorvegliare un'area o un trigger e reagire se la previsione si verifica.

## Move ha profili

La direzione di design consolidata tratta:

- **Sneak:** meno distanza, meno rumore/esposizione;
- **Normal:** profilo di riferimento;
- **Sprint:** più distanza, più rumore/esposizione.

I valori numerici definitivi sono ancora da playtestare e la migrazione dal vecchio `Action.Sprint` non è completata.

## Mobilità speciale

Queste non sono varianti del Move normale:

- Dash
- Charge
- Leap
- Blink
- Reposition
- displacement forzato

Possono risolvere prima del Blast perché appartengono alla mobilità speciale della relativa fase.

## Regola fondamentale

> **Sprint non è Dash.** Sprint è un profilo del Move; Dash è uno spostamento speciale pre-Blast.

## Fonti normative

- `docs/gameplay/brief-azioni-generiche-overwatch.md`
- `docs/gameplay/spec-sequenza-turno.md`
- `docs/balance/RT_ActionCatalog_v0.1.md`
