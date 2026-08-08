# Mappa, terreni e ambiente

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ENV-ICE -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ENV-ICE` · Release: `v0.1` · Roadmap: `E8 · CP 8.1`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Visual.Environment.IceSlide`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ENV-ICE -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ENV-STEAM -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ENV-STEAM` · Release: `v0.1` · Roadmap: `E8 · CP 8.1`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Visual.Combat.SmokeCapsTargeting`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ENV-STEAM -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ENV-TERRAIN -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ENV-TERRAIN` · Release: `v0.1` · Roadmap: `E8 · CP 8.1`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Visual.Movement.RoughRefusesCharge`  
> Gli otto terreni sono implementati e testati; strutture e interazioni continuano a essere estese.  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ENV-TERRAIN -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-MAP-HIGH-GROUND -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-MAP-HIGH-GROUND` · Release: `v0.1` · Roadmap: `E9 · CP 9.1`  
> Stato: **INTEGRATED** · Gate: `6/8`  
> Scenario: `Visual.Map.HighGroundNoBonus`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-MAP-HIGH-GROUND -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-MAP-HEXGRAPH -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-MAP-HEXGRAPH` · Release: `v0.1` · Roadmap: `E2 · CP 2.1`  
> Stato: **RELEASE_READY** · Gate: `7/8`  
> Scenario: `Visual.Map.MultiLevel`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-MAP-HEXGRAPH -->

## La mappa è un grafo tattico

Ogni esagono è una posizione logica e i collegamenti determinano dove un'unità può andare. I livelli permettono di rappresentare terreno, ponti, quote e altri percorsi sovrapposti.

Porte e ponti non sono soltanto mesh decorative: possono cambiare la **topologia** del percorso.

## Gli 8 terreni della v0.1

| Terreno | Costo Move | Effetto principale |
|---|---:|---|
| **Floor** | 1 | terreno normale |
| **Rough** | 2 | rallenta e blocca Dash/Charge passo-passo |
| **Shallow Water** | 2 | applica `Wet` ed è conduttivo |
| **Fire** | 2 | all'ingresso: 10 danni + `Burning` per 2 turni |
| **Conductive** | 1 | conduce elettricità senza applicare `Wet` |
| **Smoke** | 1 | applica `Obscured`; targeting attraverso il fumo limitato a 2 celle |
| **Ice** | 1 | può aggiungere scivolamento al Move normale |
| **High Ground** | 1 | valore da geometria/LOS/copertura; nessun `+1 Vision` automatico |

## Acqua ed elettricità

L'acqua bassa rende l'unità `Wet` e crea una superficie conduttiva. Il sistema ambientale può quindi trasformare una scelta di percorso in una vulnerabilità o in un setup di interazione sistemica.

## Fuoco

Entrare o attraversare il fuoco può infliggere danno e applicare Burning. Acqua e stati ambientali possono interagire secondo le regole del sistema.

## Ghiaccio

Nel Move normale il ghiaccio può prolungare il percorso di una cella nella direzione d'ingresso quando le condizioni previste sono soddisfatte. La mobilità lineare speciale non usa automaticamente lo stesso scivolamento.

## Quota

La quota non dà un bonus numerico universale alla vista nella v0.1. È già importante perché cambia geometria, LOS, coperture e topologia.

## Fonti normative

- `docs/gameplay/spec-terreni-e8.md`
- `docs/balance/RT_TerrainCatalog_v0.1.md`
- `docs/technical/spec-mappa-multilivello.md`

## Approfondimenti

- [Coperture](../meccaniche/coperture.md)
- [Porte](../meccaniche/porte.md)
- [Ponti](../meccaniche/ponti.md)
- [Acqua ed elettricità](../meccaniche/acqua-e-elettricita.md)
- [Fuoco e stati](../meccaniche/fuoco-e-stati.md)
- [Topologia dinamica](../meccaniche/topologia-dinamica.md)

## Sinergie

Le interazioni ambientali sono regole del sistema, non abilità di una coppia. Vedi [Sinergie e combinazioni](sinergie-e-combinazioni.md).
