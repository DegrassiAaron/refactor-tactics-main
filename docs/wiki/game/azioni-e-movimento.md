# Azioni e movimento

> **Stato nel gioco:** tassonomia canonica; migrazione degli Stable ID legacy ancora in corso
> **Tipo:** guida giocatore, non normativa

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
