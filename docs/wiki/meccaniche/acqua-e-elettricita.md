# Acqua ed elettricità

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ENV-ELECTRIC -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ENV-ELECTRIC` · Release: `v0.1` · Roadmap: `E8 · CP 8.3`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Visual.Combat.WaterElectric`  
> Il sistema esiste ed e' testato, ma **nessun eroe della v0.1 ha `Action.Electrify` come abilita' normale**: la scarica arriva dall'ambiente e dalle interazioni, non da un kit.  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ENV-ELECTRIC -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ENV-WATER -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ENV-WATER` · Release: `v0.1` · Roadmap: `E8 · CP 8.1, 8.4`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Visual.Environment.WetExtinguishesFire`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ENV-WATER -->

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

Il motore della propagazione è implementato e verificato, ma nessuno dei quattro eroi v0.1 possiede normalmente `Action.Electrify` nel proprio kit. L'interazione **`Wet → Flux.LinearDischarge`** è già una meccanica del roster. Riva è una possibile sorgente di `Wet`, ma Flux dipende dallo stato, non dall'identità di Riva.

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

## Approfondimento

- [Sinergie e combinazioni](../game/sinergie-e-combinazioni.md)
