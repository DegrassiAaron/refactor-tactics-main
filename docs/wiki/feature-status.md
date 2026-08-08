# Stato delle feature

> `GENERATO` · Vista del **Feature Registry** (`docs/roadmap/feature-registry.yaml` nel repository del gioco).
> Non modificare a mano: si rigenera con `python scripts/feature_registry.py wiki`.
> Feature tracciate: **78** · Ultimo audit completo: **2026-08-08** su `2094b86`.

Questa pagina dice **cosa esiste davvero**. Una meccanica descritta altrove nella Wiki e
marcata qui come `SPECIFIED` o `DESIGNED` e' progettata, non giocabile: la Wiki racconta
il gioco che sara', questa tabella dice a che punto e'.

## Cosa significano gli stati

| Stato | Significato |
|---|---|
| `IDEA` | Nominata, nessun documento che la definisca |
| `DESIGNED` | Un brief la descrive, le regole non sono chiuse |
| `SPECIFIED` | Regole decise e documentate, nessun codice |
| `IMPLEMENTING` | Codice presente ma incompleto o non coperto da test |
| `TESTABLE` | Implementata e coperta da test automatici |
| `INTEGRATED` | Testata **e** dimostrata da uno scenario giocabile |
| `RELEASE_READY` | Anche UI e documentazione allineate |
| `DONE` | Verificata anche su build packaged |

«Gate» conta i controlli superati sui controlli applicabili: `6/8` si verifica,
«73%» no.

## Release v0.1

### Actions

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-ACTION-COOLDOWNS` | Cooldown ed economia delle risorse | E4 · CP 4.4 | **TESTABLE** | 5/8 | — |
| `RT-FEAT-ACTION-DASH-DISPLACEMENT` | Dash e spostamento forzato | E2 · CP 2.5, CP 4.5 | **RELEASE_READY** | 7/8 | `Visual.Combat.PushResistance` |
| `RT-FEAT-ACTION-ENGINE` | Motore delle azioni a priorità intera | E4 · CP 4.1, CP 4.3, CP 4.8 | **RELEASE_READY** | 7/8 | `Movement.Collision` · `Movement.SwapRejectedByPlanning` |
| `RT-FEAT-ACTION-EQUIPMENT` | Equipaggiamento e loadout | E7 · CP 7.1, CP 7.2, CP 7.3, CP 7.4 | **IMPLEMENTING** | 1/8 | — |
| `RT-FEAT-ACTION-GENERIC` | Azioni generiche del catalogo | E4 · CP 4.4, CP 4.6, CP 4.7 | **IMPLEMENTING** | 3/8 | `Combat.BasicAttack` · `Movement.Basic` |
| `RT-FEAT-ACTION-MOVE-PROFILES` | Profili di movimento (Move, Sprint, Charge) | E4 · CP 4.2, CP 4.5 | **RELEASE_READY** | 7/8 | `Visual.Movement.Charge` · `Visual.Movement.RoughRefusesCharge` |
| `RT-FEAT-ACTION-PREDICTIVE` | Predictive Action, thin slice | E18 · CP 18.1, CP 18.2 | **SPECIFIED** | 1/8 | _pianificato_ |

### Characters

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-CHAR-PRESENTATION` | Presentazione dei personaggi (mesh, animazioni, anelli) | M8 | **IMPLEMENTING** | 1/7 | — |
| `RT-FEAT-CHAR-V01-ROSTER` | Roster v0.1 — Flux, Riva, Bastion, Vektor | E6 · CP 6.1, CP 6.2, CP 6.3, CP 6.4, CP 6.5, CP 6.6, CP 6.7 | **INTEGRATED** | 6/8 | `Combat.BasicAttack` · `Visual.Reaction.Interposition` |

### Core

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-CORE-DECISION-BOUNDARY` | Risoluzione segmentata con Decision Boundary | E14 · CP 14.1, CP 14.2, CP 14.3 | **SPECIFIED** | 1/8 | — |
| `RT-FEAT-CORE-DETERMINISM` | Snapshot e resolver deterministico | E12 · CP 12.1, CP 12.6 | **INTEGRATED** | 5/7 | `RT_Showcase_Relay_v01` |
| `RT-FEAT-CORE-PLAYBACK` | Playback della risoluzione | E11 · CP 11.1 | **INTEGRATED** | 5/7 | `Visual.Core.PhaseOrder` · `Visual.Combat.Defeat` |
| `RT-FEAT-CORE-TURN` | Pipeline del turno simultaneo | E2 · CP 2.2, CP 4.1 | **RELEASE_READY** | 7/8 | `Visual.Core.PhaseOrder` |
| `RT-FEAT-CORE-TURNLOG` | TurnLog, reason code, hash e replay | E12 · CP 12.1, CP 12.6 | **RELEASE_READY** | 6/7 | `Visual.Core.PhaseOrder` |

### Data

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-DATA-ASSET-PIPELINE` | Primary Data Asset e cataloghi | E1 · CP 1.2, CP 1.3 | **RELEASE_READY** | 5/6 | — |
| `RT-FEAT-DATA-HASH` | Hash di regole e contenuti | E12 · CP 12.1 | **RELEASE_READY** | 5/7 | — |
| `RT-FEAT-DATA-STABLE-IDS` | ID stabili e versioni dei contenuti | E1 · CP 1.3 | **RELEASE_READY** | 5/6 | — |

### Environment

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-ENV-ELECTRIC` | Propagazione elettrica sul grafo dell'acqua | E8 · CP 8.3 | **INTEGRATED** | 6/8 | `Visual.Combat.WaterElectric` |
| `RT-FEAT-ENV-FIRE` | Fuoco e terreno dinamico | E8 · CP 8.1, CP 8.4 | **INTEGRATED** | 6/8 | `Visual.Environment.FireOnEnter` · `Visual.Environment.WetExtinguishesFire` |
| `RT-FEAT-ENV-ICE` | Ghiaccio e scivolamento | E8 · CP 8.1 | **INTEGRATED** | 6/8 | `Visual.Environment.IceSlide` |
| `RT-FEAT-ENV-STATUS` | Stati temporanei legati alla cella | E8 · CP 8.2 | **INTEGRATED** | 6/8 | `Visual.Environment.FireOnEnter` |
| `RT-FEAT-ENV-STEAM` | Fumo e copertura visiva | E8 · CP 8.1 | **INTEGRATED** | 6/8 | `Visual.Combat.SmokeCapsTargeting` |
| `RT-FEAT-ENV-SYSTEMIC-COMBOS` | Interazioni sistemiche producer/consumer | E8 · CP 8.5 | **INTEGRATED** | 6/8 | `Visual.Combat.WaterElectric` · `Visual.Environment.WetExtinguishesFire` |
| `RT-FEAT-ENV-TERRAIN` | Otto terreni con costi e proprietà | E8 · CP 8.1 | **INTEGRATED** | 6/8 | `Visual.Movement.RoughRefusesCharge` |
| `RT-FEAT-ENV-WATER` | Acqua e stato Wet | E8 · CP 8.1, CP 8.4 | **INTEGRATED** | 6/8 | `Visual.Environment.WetExtinguishesFire` |

### Map

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-MAP-COVER` | Copertura direzionale per bordo | E9 · CP 9.1, CP 9.2 | **INTEGRATED** | 6/8 | `Visual.Map.LowCoverEdge` |
| `RT-FEAT-MAP-DYNAMIC-COVER` | Copertura modificabile e pannello cinetico | E9 · CP 9.5 | **IMPLEMENTING** | 2/8 | _pianificato_ |
| `RT-FEAT-MAP-FACING` | Facing come stato di gioco autorevole | E16 · CP 16.1, CP 16.2 | **SPECIFIED** | 1/8 | — |
| `RT-FEAT-MAP-HEXGRAPH` | FRTCellId e grafo esagonale multilivello | E2 · CP 2.1 | **RELEASE_READY** | 7/8 | `Visual.Map.MultiLevel` |
| `RT-FEAT-MAP-HIGH-GROUND` | Altura senza bonus numerico alla vista | E9 · CP 9.1 | **INTEGRATED** | 6/8 | `Visual.Map.HighGroundNoBonus` |
| `RT-FEAT-MAP-INTERACTIVE-EDGES` | Porte e bordi commutabili | E9 · CP 9.3 | **INTEGRATED** | 6/8 | `Visual.Map.ClosedDoor` |
| `RT-FEAT-MAP-LOS` | LOS, targeting e traiettoria separati | E2 · CP 2.4 | **RELEASE_READY** | 6/7 | `Combat.BlockedByWall` · `Combat.LineHitsThrough` |
| `RT-FEAT-MAP-PATHFINDING` | A* esagonale autorevole | E2 · CP 2.2 | **RELEASE_READY** | 6/7 | `Movement.Basic` · `Movement.Blocked` |
| `RT-FEAT-MAP-SPECIAL-TRANSITIONS` | Ponti, archi e transizioni multilivello | E9 · CP 9.4 | **INTEGRATED** | 6/8 | `Visual.Map.MultiLevel` |

### Networking

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-NET-PRIVATE-PLANNING` | Intenti privati per squadra | E5 · CP 5.4 | **TESTABLE** | 5/8 | — |

### Objectives

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-MATCH-END-CONDITIONS` | Fine partita a tre vie | E10 · CP 10.3 | **RELEASE_READY** | 7/8 | `Visual.Combat.Defeat` · `RT_Showcase_Relay_v01` |
| `RT-FEAT-MATCH-FORMAT` | Formato di partita e classe di mappa | E19 · CP 19.1, CP 19.2 | **TESTABLE** | 4/8 | — |
| `RT-FEAT-MATCH-PACING` | Pacing del turno e del match | E12 · CP 12.4 | **TESTABLE** | 5/8 | — |
| `RT-FEAT-OBJECTIVE-SYSTEM` | Obiettivi dinamici in mappa | E10 · CP 10.1, CP 10.2 | **IMPLEMENTING** | 2/8 | _pianificato_ |
| `RT-FEAT-STRESS-4V4` | Validazione di stress 4v4 | E17 · CP 17.1, CP 17.2, CP 17.3 | **SPECIFIED** | 1/7 | _pianificato_ |

### Perception

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-PERCEPTION-MEMORY` | Memoria del contatto e ultima posizione nota | E13 · CP 13.2 | **SPECIFIED** | 1/9 | — |
| `RT-FEAT-PERCEPTION-NOISE` | Rumore e percezione acustica | E13 · CP 13.3 | **SPECIFIED** | 1/9 | _pianificato_ |
| `RT-FEAT-PERCEPTION-TEAM-KNOWLEDGE` | TeamKnowledge e informazione parziale | E13 · CP 13.1 | **SPECIFIED** | 1/9 | — |
| `RT-FEAT-PERCEPTION-VISION` | Vista, facing e livelli di consapevolezza | E13 · CP 13.1, CP 13.2 | **SPECIFIED** | 1/9 | — |

### Production

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-PROD-PACKAGED` | Verifica su build packaged | E12 · CP 12.5 | **IMPLEMENTING** | 2/6 | — |
| `RT-FEAT-PROD-PERFORMANCE` | Budget di performance misurati | E12 · CP 12.4 | **IMPLEMENTING** | 3/6 | — |

### Reactions

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-REACTION-FAST` | Fast Reaction con finestra limitata | E14 · CP 14.5, CP 14.6 | **SPECIFIED** | 1/9 | — |
| `RT-FEAT-REACTION-FAST-ACTION` | Fast Action come continuazione della propria azione | E14 · CP 14.6 | **DESIGNED** | 0/9 | — |
| `RT-FEAT-REACTION-OPPORTUNITY` | Modello Opportunity → Commit | E14 · CP 14.3 | **SPECIFIED** | 1/8 | — |
| `RT-FEAT-REACTION-OVERWATCH` | Overwatch universale profilabile | E14 · CP 14.4 | **SPECIFIED** | 1/9 | _pianificato_ |
| `RT-FEAT-REACTION-PREPARED` | Reazioni preparate in planning | E5 · CP 5.1, CP 5.2, CP 5.3, CP 5.4, CP 5.5 | **INTEGRATED** | 6/8 | `Combat.CounterStrikesBack` · `Combat.NoCounterWhenUnarmed` |

### Tools

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-BOT-BASE` | Bot a utility scoring deterministico | E2 · CP 2.6 | **RELEASE_READY** | 7/8 | `RT_Showcase_Relay_v01` |
| `RT-FEAT-TEST-GOLDEN` | Golden replay e showcase «Il Relè» | E15 · CP 15.1, CP 15.2, CP 15.4, CP 15.5 | **IMPLEMENTING** | 3/8 | `RT_Showcase_Relay_v01` |
| `RT-FEAT-TEST-SCENARIO-HARNESS` | Scenario Test Harness automatizzato | E15 · CP 15.3 | **INTEGRATED** | 6/8 | `Movement.Basic` · `Movement.BasicFailsOnPurpose` |
| `RT-FEAT-TOOL-BALANCE-GROUND` | Banco di prova del bilanciamento | E1 · CP 1.2 | **IMPLEMENTING** | 3/6 | — |
| `RT-FEAT-TOOL-DEBUG-CONSOLE` | Comandi console rt.Debug e rt.Test | E11 · CP 11.4 | **IMPLEMENTING** | 3/6 | — |
| `RT-FEAT-TOOL-MAP-EDITOR` | Editor mode della mappa esagonale | M9 | **TESTABLE** | 4/6 | — |
| `RT-FEAT-TOOL-VALIDATION` | Validator di dati, mappe e documenti | E1 · CP 1.4 | **DONE** | 5/5 | — |

### UI

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-UI-ACTION-GHOSTS` | Action Ghosts e Ghost Timeline | E11 · CP 11.5, CP 11.6 | **SPECIFIED** | 1/8 | — |
| `RT-FEAT-UI-CERTAINTY` | Livelli di certezza degli intenti alleati | E11 · CP 11.2 | **IMPLEMENTING** | 3/8 | — |
| `RT-FEAT-UI-COMBAT-LOG` | Combat log e spiegabilità | E11 · CP 11.3 | **RELEASE_READY** | 6/7 | `Visual.Core.PhaseOrder` |
| `RT-FEAT-UI-ICON-LANGUAGE` | HUD Icon Language | E20 · CP 20.1, CP 20.2, CP 20.3 | **SPECIFIED** | 1/7 | — |
| `RT-FEAT-UI-PLANNING` | HUD di planning, selezione e preview | E11 · CP 11.1 | **RELEASE_READY** | 6/7 | `Movement.SwapRejectedByPlanning` · `Visual.Core.PhaseOrder` |
| `RT-FEAT-UI-SCENARIO-BROWSER` | Selettore e indice degli scenari | — | **INTEGRATED** | 6/8 | `Movement.Basic` |
| `RT-FEAT-UI-TACTICAL-CAMERA` | Camera tattica | E11 · CP 11.1 | **IMPLEMENTING** | 1/6 | — |
| `RT-FEAT-UI-WARNINGS` | Avvisi di collisione, fuoco amico e risorse | E11 · CP 11.1, CP 11.2 | **IMPLEMENTING** | 3/7 | `Combat.FriendlyFire` · `Combat.SplashHitsAlliesNotSelf` |

## Release v0.2

### Actions

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-ACTION-SUPERS` | Ultimate e azioni ad alto impegno | — | **IMPLEMENTING** | 0/8 | — |

### Characters

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-CHAR-AUXILIARY-UNITS` | Unità ausiliarie (pet, evocazioni, gadget) | — | **DESIGNED** | 0/8 | — |
| `RT-FEAT-CHAR-TRANSFORMATION` | Stati di personaggio, stance e trasformazioni | — | **IDEA** | 0/8 | — |
| `RT-FEAT-CHAR-V02-ROSTER` | Roster v0.2 — Steel, Aurora, Murdock, Kwang | — | **DESIGNED** | 0/8 | — |

### Environment

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-ENV-ICE-ENGINE` | Motore del ghiaccio (momentum, rottura, prone) | — | **DESIGNED** | 0/8 | — |

### Factions

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-FACTION-SCENARIOS` | Scenari di cooperazione per fazione | — | **DESIGNED** | 0/7 | _pianificato_ |
| `RT-FEAT-FACTION-SYSTEM` | Fazioni, identità e iconografia | — | **DESIGNED** | 0/7 | — |

### Tools

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-BOT-TACTICAL` | Bot tattico con conoscenza e reazioni | — | **IDEA** | 0/8 | — |

## Oltre la v0.2

### Actions

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-ACTION-DELAYED` | Delayed Action ai boundary di fase | — | **DESIGNED** | 0/8 | — |
| `RT-FEAT-ACTION-TRAPS` | Trappole e gambit tattici | — | **IDEA** | 0/8 | — |

### Networking

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-NET-AUTHORITY` | Multiplayer con autorità server | M10 | **SPECIFIED** | 1/8 | — |
| `RT-FEAT-NET-DEDICATED` | Dedicated server | — | **IDEA** | 0/8 | — |
