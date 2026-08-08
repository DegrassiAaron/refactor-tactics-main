# Fuoco, Burning e Wet

> **Stato v0.1:** implementato e testato
> **Tipo:** guida giocatore, non normativa

## Entrare nel fuoco

Una cella `Fire` ha costo Move **2**. Entrare o attraversarla applica:

- **10 danni immediati**;
- `Burning` per **2 turni**.

## Burning

`Burning` infligge **8 danni nel Cleanup**. Il danno passa attraverso lo stesso sistema di danno degli altri effetti, quindi consuma prima eventuale scudo temporaneo.

Il danno avviene prima del conteggio finale dei KO: un personaggio che muore per Burning nel Cleanup è considerato KO **in quel round**, non in quello successivo.

## Wet spegne Burning

Se un'unità diventa `Wet`, `Burning` viene rimosso. È una delle interazioni ambientali centrali del gioco e rende l'acqua anche uno strumento difensivo.

## Acqua su una cella di fuoco

`CreateWater` su una cella in fiamme produce uno **spegnimento**, distinto nel TurnLog da un normale cambio di superficie.

Fuoco e acqua si annullano quindi sia sullo stato dell'unità sia sullo stato della mappa.

## Terreno dinamico

La superficie corrente può cambiare durante una partita. La mappa di gioco è una copia della mappa d'autore: le modifiche temporanee non devono sporcare l'asset originale né sopravvivere alla partita successiva.

Le modifiche dinamiche standard usano una durata di **2 turni** quando dichiarato dal catalogo ambientale.

## Cosa può bruciare

Nella v0.1 il terreno usa un flag `bIsFlammable`. Attualmente le superfici combustibili utili sono `Floor` e `Rough`; acqua e terreno conduttivo/metallico non vengono incendiati.

Il fuoco non si propaga automaticamente fra celle nella v0.1: non va confuso con la propagazione elettrica.

## Ordine ambientale nel Cleanup

In forma semplificata:

1. scadono le vecchie modifiche dinamiche;
2. risolvono le nuove azioni ambientali;
3. Burning infligge il proprio danno;
4. vengono revocati gli stati legati alla cella quando non più sostenuti;
5. scadono le durate e avanzano cooldown/risorse;
6. vengono valutati KO/esito.

## Cosa deve ricordare il giocatore

> Il fuoco non è soltanto danno immediato: crea un problema che può seguirti fino al Cleanup, ma l'acqua può risolverlo.

## Fonti normative

- `docs/gameplay/spec-terreni-e8.md`
- `docs/gameplay/spec-stati-temporanei-cp82.md`
- `docs/gameplay/spec-fuoco-acqua-cp84.md`
