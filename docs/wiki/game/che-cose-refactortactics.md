# Che cos'è RefactorTactics

> **Stato nel gioco:** identità di prodotto consolidata · v0.1 in sviluppo
> **Tipo:** guida giocatore, non normativa

## In breve

**RefactorTactics** è un gioco tattico a **turni simultanei** su una mappa esagonale multilivello. Non scegli un personaggio, fai la tua azione e poi aspetti il turno dell'avversario: **tutte le unità pianificano insieme**, i piani vengono bloccati e il gioco li risolve secondo regole comuni.

Il cuore del gioco è quindi la **previsione**: non devi soltanto trovare una buona azione, ma capire cosa pensi che faranno avversari e alleati nello stesso round.

## Le idee fondamentali

### Tutti decidono nello stesso momento

Durante il **Planning** ogni squadra prepara movimento, azione e possibili reazioni. Quando il piano viene confermato, non può più essere riscritto liberamente.

### L'ordine delle fasi è fisso

La Resolution usa questa struttura:

```text
Planning → Commit → Prep → Dash → Blast → Move → Cleanup
```

Il normale `Move` arriva **dopo** gli attacchi del Blast. Dash, Charge e altre mobilità speciali possono avvenire prima perché appartengono a una fase diversa.

### La mappa è parte del combattimento

Terreno, acqua, fuoco, ghiaccio, fumo, coperture, porte, ponti e differenze di quota cambiano percorsi, linee di tiro e opportunità tattiche.

### Il gioco premia coordinazione e lettura

Gli alleati possono coordinare piani e intenti. Gli avversari, invece, non devono conoscere il planning della squadra nemica prima della Resolution.

### Il risultato non dipende dalle animazioni

La simulazione decide prima cosa è successo; animazioni e VFX lo rappresentano. A parità di stato, regole e input, il risultato logico deve essere riproducibile.

## La v0.1

La release v0.1 è un **vertical slice 2v2 offline contro bot**. I quattro personaggi della v0.1 sono:

- [Flux](../../characters/v0.1/flux.md)
- [Riva](../../characters/v0.1/riva.md)
- [Bastion](../../characters/v0.1/bastion.md)
- [Vektor](../../characters/v0.1/vektor.md)

La versione successiva espande il roster con Steel, Aurora, Murdock e Kwang.

## Cosa deve ricordare il giocatore

> **Pensa prima a dove saranno tutti quando una fase risolverà, non soltanto a dove sono adesso.**

## Fonti normative

- `docs/product/piano-canonico-mvp.md`
- `docs/gameplay/spec-sequenza-turno.md`
- `docs/roadmap/roadmap-checkpoint.md`
