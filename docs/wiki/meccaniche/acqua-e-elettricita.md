# Acqua ed elettricità

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ENV-ELECTRIC -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ENV-ELECTRIC` · Release: `v0.1` · Roadmap: `E8.3`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Visual.Combat.WaterElectric`  
> Dal 2026-08-09 la scarica ha un **owner nel roster**: `Flux.ConductiveNode` **e'** `Action.Electrify` (D-039). Prima nessun eroe la possedeva e il motore era verde ma non innescabile in partita.  
> Verificato il `2026-08-09` su `f1f85b1`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ENV-ELECTRIC -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ENV-WATER -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ENV-WATER` · Release: `v0.1` · Roadmap: `E8.1, E8.4`  
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

Dal **2026-08-09** la scarica ha un owner nel roster: `Flux.ConductiveNode` **è** `Action.Electrify`
([D-039](../../decisions/RT_PDR_00_Decision_Log.md)). Fino a quel giorno il motore era implementato,
verificato e **non innescabile in partita** — nessun eroe possedeva l'azione, quindi la propagazione arrivava
solo dall'ambiente.

L'altra via resta e non è cambiata: **`Wet → Flux.LinearDischarge`**, che ottiene `+8` contro un bersaglio
bagnato. Riva è una possibile sorgente di `Wet`, ma il bonus dipende dallo **stato della cella**, non
dall'identità di chi l'ha applicato — è la proprietà registrata da
[D-029](../../decisions/RT_PDR_00_Decision_Log.md).

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
