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

## Non tutti eseguono la stessa azione allo stesso modo

Il **profilo** non riguarda solo il Move. Le azioni generiche sono lo stesso comando per tutti — stessa fase,
stesse regole, stesso posto nell'economia del turno — ma **come** si comportano dipende dal personaggio.

L'Overwatch è l'esempio più visibile: tutti possono armarlo, ma un tiratore sorveglia un arco stretto a lunga
distanza e risponde con `FIRE` o `HOLD`, mentre un difensore sorveglia un arco largo attorno a un alleato e
risponde intercettando. Non sono due abilità diverse: è la stessa azione con due profili.

Vale la pena saperlo per una ragione pratica: **conoscere il personaggio non basta a prevederlo, serve
conoscere il suo profilo.** Due eroi che dichiarano Overwatch sulla stessa cella non stanno minacciando la
stessa cosa.

Un profilo non è un potenziamento: se dà un vantaggio, ha un prezzo dichiarato — è la stessa logica delle
varianti, dove non esistono scelte gratuitamente migliori.

> In questa versione il profilo di un personaggio è **fisso**: non si cambia durante la partita.

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
