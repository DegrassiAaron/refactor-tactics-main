# Acqua ed elettricità

> **Stato v0.1:** `Wet`, bonus di Flux e propagazione elettrica sul terreno implementati/testati · nessun eroe v0.1 possiede ancora `Action.Electrify` come skill normale
> **Tipo:** guida giocatore, non normativa

## Due meccaniche collegate ma diverse

RefactorTactics distingue:

1. **Wet sull'unità** — uno stato che può modificare abilità specifiche;
2. **conduttività della cella** — ciò che permette all'elettricità di propagarsi sulla mappa.

Non sono la stessa cosa.

## Wet

`ShallowWater` applica `Wet` finché l'unità resta sostenuta dalla cella. Riva può applicare Wet anche lontano dall'acqua con le proprie abilità; in quel caso la durata adottata è **1 turno**.

Wet ha due interazioni importanti nella v0.1:

- rimuove `Burning`;
- `Flux.LinearDischarge` ottiene **+8 danni** contro un bersaglio Wet.

## Conduttività

Le celle `ShallowWater` e `Conductive` dichiarano che conducono elettricità.

La propagazione segue **il grafo delle celle conduttive**, non un semplice raggio geometrico e non una catena di personaggi bagnati.

> Un'unità Wet su terreno asciutto **non diventa un ponte elettrico**.

## Propagazione

L'azione core `Action.Electrify` usa:

- **20 danni** sul bersaglio iniziale;
- **12 danni** sulle unità raggiunte dalla propagazione;
- massimo **3 passi** nel grafo conduttivo;
- ogni cella viene visitata una sola volta per evento.

La sorgente della propagazione è la cella sotto il bersaglio iniziale.

## Ponti conduttivi

Un arco/ponte può dichiarare `bConductsElectricity`. La propagazione può quindi salire o scendere di layer attraverso un ponte attivo e conduttivo.

Un ponte spento o distrutto interrompe la catena.

## Stato reale del roster

Il motore della propagazione è implementato e verificato, ma nessuno dei quattro eroi v0.1 possiede normalmente `Action.Electrify` nel proprio kit. La combo **Flux + Wet** invece è già una meccanica del roster.

## Cosa deve ricordare il giocatore

- Wet ≠ conduttività.
- L'elettricità segue **l'acqua collegata**, non la distanza in linea d'aria.
- Un ponte conduttivo può trasformare una scorciatoia in un rischio.
- Flux premia il setup Wet anche senza usare la propagazione ambientale.

## Fonti normative

- `docs/gameplay/spec-propagazione-elettrica-cp83.md`
- `docs/gameplay/spec-stati-temporanei-cp82.md`
- `docs/gameplay/spec-ponti-cp94.md`
- `docs/balance/RT_HeroCatalog_v0.1.md`
