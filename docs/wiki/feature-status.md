# Stato delle feature

> `GENERATO` · Vista del **Feature Registry** (`docs/roadmap/feature-registry.yaml` nel repository del gioco).
> Non modificare a mano: si rigenera con `python scripts/feature_registry.py wiki`.
> Feature tracciate: **84** · Ultimo audit completo: **2026-08-08** su `2094b86`.

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
| `RT-FEAT-ACTION-BASIC-ATTACK-PROFILES` | Profili di attacco base per eroe | E4 | **IMPLEMENTING** | 6/8 | `Combat.BasicAttack` · `Combat.BastionImpactShotSlows` |
| `RT-FEAT-ACTION-COOLDOWNS` | Cooldown ed economia delle risorse | E4.4 | **TESTABLE** | 5/8 | — |
| `RT-FEAT-ACTION-DASH-DISPLACEMENT` | Dash e spostamento forzato | E2.5, E2.5 | **RELEASE_READY** | 7/8 | `Visual.Combat.PushResistance` |
| `RT-FEAT-ACTION-ENGINE` | Motore delle azioni a priorità intera | E4.1, E4.3, E4.8 | **RELEASE_READY** | 7/8 | `Movement.Collision` · `Movement.SwapRejectedByPlanning` |
| `RT-FEAT-ACTION-EQUIPMENT` | Equipaggiamento e loadout | E7.1, E7.2, E7.3, E7.4 | **IMPLEMENTING** | 1/8 | — |
| `RT-FEAT-ACTION-GENERIC` | Azioni generiche del catalogo | E4.4, E4.6, E4.7 | **IMPLEMENTING** | 3/8 | `Combat.BasicAttack` · `Movement.Basic` |
| `RT-FEAT-ACTION-MOVE-PROFILES` | Profili di movimento (Move, Sprint, Charge) | E4.2, E4.5 | **RELEASE_READY** | 7/8 | `Visual.Movement.Charge` · `Visual.Movement.RoughRefusesCharge` |
| `RT-FEAT-ACTION-PREDICTIVE` | Predictive Action, thin slice | E18.1, E18.2 | **SPECIFIED** | 1/8 | `Spec.Predictive.WhiffOnEmptyCell` |

### Characters

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-CHAR-PRESENTATION` | Presentazione dei personaggi (mesh, animazioni, anelli) | E21.1, E21.2, E21.3 | **IMPLEMENTING** | 1/7 | — |
| `RT-FEAT-CHAR-V01-ROSTER` | Roster v0.1 — Flux, Riva, Bastion, Vektor | E6.1, E6.2, E6.3, E6.4, E6.5, E6.6, E6.7 | **INTEGRATED** | 6/8 | `Combat.BasicAttack` · `Visual.Reaction.Interposition` |

### Core

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-CORE-DECISION-BOUNDARY` | Risoluzione segmentata con Decision Boundary | E14.1, E14.2, E14.3 | **SPECIFIED** | 1/8 | — |
| `RT-FEAT-CORE-DECISION-TIME-BANK` | Decision Time Bank (budget di decisione per giocatore) | E14.8 | **SPECIFIED** | 1/9 | _pianificato_ |
| `RT-FEAT-CORE-DETERMINISM` | Snapshot e resolver deterministico | E12.1, E12.6 | **INTEGRATED** | 5/7 | `RT_Showcase_Relay_v01` |
| `RT-FEAT-CORE-PLAYBACK` | Playback della risoluzione | E11.1 | **INTEGRATED** | 5/7 | `Visual.Core.PhaseOrder` · `Visual.Combat.Defeat` |
| `RT-FEAT-CORE-TURN` | Pipeline del turno simultaneo | E2.2, E2.1 | **RELEASE_READY** | 7/8 | `Visual.Core.PhaseOrder` |
| `RT-FEAT-CORE-TURNLOG` | TurnLog, reason code, hash e replay | E12.1, E12.6 | **RELEASE_READY** | 6/7 | `Visual.Core.PhaseOrder` |

### Data

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-DATA-ASSET-PIPELINE` | Primary Data Asset e cataloghi | E1.2, E1.3 | **RELEASE_READY** | 5/6 | — |
| `RT-FEAT-DATA-HASH` | Hash di regole e contenuti | E12.1 | **RELEASE_READY** | 5/7 | — |
| `RT-FEAT-DATA-STABLE-IDS` | ID stabili e versioni dei contenuti | E1.3 | **RELEASE_READY** | 5/6 | — |

### Environment

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-ENV-ELECTRIC` | Propagazione elettrica sul grafo dell'acqua | E8.3 | **INTEGRATED** | 6/8 | `Visual.Combat.WaterElectric` · `Visual.Combat.WaterElectricCoordinated` |
| `RT-FEAT-ENV-FIRE` | Fuoco e terreno dinamico | E8.1, E8.4 | **INTEGRATED** | 6/8 | `Visual.Environment.FireOnEnter` · `Visual.Environment.WetExtinguishesFire` |
| `RT-FEAT-ENV-ICE` | Ghiaccio e scivolamento | E8.1 | **INTEGRATED** | 6/8 | `Visual.Environment.IceSlide` |
| `RT-FEAT-ENV-STATUS` | Stati temporanei legati alla cella | E8.2 | **INTEGRATED** | 6/8 | `Visual.Environment.FireOnEnter` |
| `RT-FEAT-ENV-STEAM` | Fumo e copertura visiva | E8.1 | **INTEGRATED** | 6/8 | `Visual.Combat.SmokeCapsTargeting` |
| `RT-FEAT-ENV-SYSTEMIC-COMBOS` | Interazioni sistemiche producer/consumer | E8.5 | **INTEGRATED** | 6/8 | `Visual.Combat.WaterElectric` · `Visual.Environment.WetExtinguishesFire` |
| `RT-FEAT-ENV-TERRAIN` | Otto terreni con costi e proprietà | E8.1 | **INTEGRATED** | 6/8 | `Visual.Movement.RoughRefusesCharge` |
| `RT-FEAT-ENV-WATER` | Acqua e stato Wet | E8.1, E8.4 | **INTEGRATED** | 6/8 | `Visual.Environment.WetExtinguishesFire` |

### Map

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-MAP-COVER` | Copertura direzionale per bordo | E9.1, E9.2 | **INTEGRATED** | 6/8 | `Visual.Map.LowCoverEdge` · `Visual.Map.HighCoverBlocks` |
| `RT-FEAT-MAP-DYNAMIC-COVER` | Copertura modificabile e pannello cinetico | E9.5 | **INTEGRATED** | 6/8 | `Spec.Cover.TemporaryCoverExpires` |
| `RT-FEAT-MAP-FACING` | Facing come stato di gioco autorevole | E16.1, E16.2 | **INTEGRATED** | 6/9 | `Spec.Facing.DerivesFromMove` · `Spec.Facing.DashReorients` |
| `RT-FEAT-MAP-HEXGRAPH` | FRTCellId e grafo esagonale multilivello | E2.1 | **RELEASE_READY** | 7/8 | `Visual.Map.MultiLevel` |
| `RT-FEAT-MAP-HIGH-GROUND` | Altura senza bonus numerico alla vista | E9.1 | **INTEGRATED** | 6/8 | `Visual.Map.HighGroundNoBonus` |
| `RT-FEAT-MAP-INTERACTIVE-EDGES` | Porte e bordi commutabili | E9.3 | **INTEGRATED** | 6/8 | `Visual.Map.ClosedDoor` |
| `RT-FEAT-MAP-LOS` | LOS, targeting e traiettoria separati | E2.4 | **RELEASE_READY** | 6/7 | `Combat.BlockedByWall` · `Combat.LineHitsThrough` |
| `RT-FEAT-MAP-PATHFINDING` | A* esagonale autorevole | E2.2 | **RELEASE_READY** | 6/7 | `Movement.Basic` · `Movement.Blocked` |
| `RT-FEAT-MAP-SPECIAL-TRANSITIONS` | Ponti, archi e transizioni multilivello | E9.4 | **INTEGRATED** | 6/8 | `Visual.Map.MultiLevel` · `Spec.Map.BridgeBreaksThePath` |

### Networking

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-NET-PRIVATE-PLANNING` | Intenti privati per squadra | E5.4 | **TESTABLE** | 5/8 | — |

### Objectives

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-MATCH-END-CONDITIONS` | Fine partita a tre vie | E10.3 | **RELEASE_READY** | 7/8 | `Visual.Combat.Defeat` · `RT_Showcase_Relay_v01` |
| `RT-FEAT-MATCH-FORMAT` | Formato di partita e classe di mappa | E19.1, E19.2 | **TESTABLE** | 4/8 | — |
| `RT-FEAT-MATCH-PACING` | Pacing del turno e del match | E12.4 | **TESTABLE** | 5/8 | — |
| `RT-FEAT-OBJECTIVE-SYSTEM` | Obiettivi dinamici in mappa | E10.1, E10.2 | **IMPLEMENTING** | 2/8 | `Spec.Objective.PointSurvivesKO` |
| `RT-FEAT-STRESS-4V4` | Validazione di stress 4v4 | E17.1, E17.2, E17.3 | **SPECIFIED** | 1/7 | _pianificato_ |

### Perception

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-PERCEPTION-MEMORY` | Memoria del contatto e ultima posizione nota | E13.2 | **SPECIFIED** | 1/9 | — |
| `RT-FEAT-PERCEPTION-NOISE` | Rumore e percezione acustica | E13.3 | **SPECIFIED** | 1/9 | `Spec.Perception.HeardNotSeen` |
| `RT-FEAT-PERCEPTION-TEAM-KNOWLEDGE` | TeamKnowledge e informazione parziale | E13.1 | **IMPLEMENTING** | 3/9 | — |
| `RT-FEAT-PERCEPTION-VISION` | Vista, facing e livelli di consapevolezza | E13.1, E13.2 | **IMPLEMENTING** | 3/9 | — |

### Production

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-PROD-PACKAGED` | Verifica su build packaged | E12.5 | **IMPLEMENTING** | 2/6 | — |
| `RT-FEAT-PROD-PERFORMANCE` | Budget di performance misurati | E12.4 | **IMPLEMENTING** | 3/6 | — |

### Reactions

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-REACTION-CLASH` | Reaction Clash (opportunity contested) | E14.7 | **SPECIFIED** | 1/9 | `Spec.Clash.ReadBeatsStand` · `Spec.Clash.StandBeatsShift` |
| `RT-FEAT-REACTION-FAST` | Fast Reaction con finestra limitata | E14.5, E14.6 | **SPECIFIED** | 1/9 | — |
| `RT-FEAT-REACTION-FAST-ACTION` | Fast Action come continuazione della propria azione | E14.6 | **DESIGNED** | 0/9 | — |
| `RT-FEAT-REACTION-OPPORTUNITY` | Modello Opportunity → Commit | E14.3 | **SPECIFIED** | 1/8 | — |
| `RT-FEAT-REACTION-OVERWATCH` | Overwatch universale profilabile | E14.4 | **SPECIFIED** | 1/9 | `Spec.Overwatch.HoldThenFire` |
| `RT-FEAT-REACTION-PREPARED` | Reazioni preparate in planning | E5.1, E5.2, E5.3, E5.4, E5.5 | **INTEGRATED** | 6/8 | `Combat.CounterStrikesBack` · `Combat.NoCounterWhenUnarmed` |
| `RT-FEAT-REACTION-PROFILE` | Reaction Profile armato da Brace | E14.7 | **IMPLEMENTING** | 1/8 | `Visual.Combat.BraceReducesEveryHit` · `Spec.Facing.BraceHoldsFromBehind` |

### Tools

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-BOT-BASE` | Bot a utility scoring deterministico | E2.6 | **RELEASE_READY** | 7/8 | `RT_Showcase_Relay_v01` |
| `RT-FEAT-TEST-GOLDEN` | Golden replay e showcase «Il Relè» | E15.1, E15.2, E15.4, E15.5 | **IMPLEMENTING** | 3/8 | `RT_Showcase_Relay_v01` |
| `RT-FEAT-TEST-SCENARIO-HARNESS` | Scenario Test Harness automatizzato | E15.3 | **INTEGRATED** | 6/8 | `Movement.Basic` · `Movement.BasicFailsOnPurpose` |
| `RT-FEAT-TOOL-BALANCE-GROUND` | Banco di prova del bilanciamento | E1.2 | **IMPLEMENTING** | 3/6 | — |
| `RT-FEAT-TOOL-DEBUG-CONSOLE` | Comandi console rt.Debug e rt.Test | E11.4 | **IMPLEMENTING** | 3/6 | — |
| `RT-FEAT-TOOL-MAP-EDITOR` | Editor mode della mappa esagonale | M9 | **TESTABLE** | 4/6 | — |
| `RT-FEAT-TOOL-VALIDATION` | Validator di dati, mappe e documenti | E1.4 | **DONE** | 5/5 | — |

### UI

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-UI-ACTION-GHOSTS` | Action Ghosts e Ghost Timeline | E11.5, E11.6 | **SPECIFIED** | 1/8 | — |
| `RT-FEAT-UI-CERTAINTY` | Livelli di certezza degli intenti alleati | E11.2 | **IMPLEMENTING** | 3/8 | — |
| `RT-FEAT-UI-COMBAT-LOG` | Combat log e spiegabilità | E11.3 | **RELEASE_READY** | 6/7 | `Visual.Core.PhaseOrder` |
| `RT-FEAT-UI-ICON-LANGUAGE` | HUD Icon Language | E20.1, E20.2, E20.3 | **IMPLEMENTING** | 1/7 | — |
| `RT-FEAT-UI-PLANNING` | HUD di planning, selezione e preview | E11.1 | **RELEASE_READY** | 6/7 | `Movement.SwapRejectedByPlanning` · `Visual.Core.PhaseOrder` |
| `RT-FEAT-UI-SCENARIO-BROWSER` | Selettore e indice degli scenari | fuori scope | **INTEGRATED** | 6/8 | `Movement.Basic` |
| `RT-FEAT-UI-TACTICAL-CAMERA` | Camera tattica | E11.1 | **IMPLEMENTING** | 1/6 | — |
| `RT-FEAT-UI-WARNINGS` | Avvisi di collisione, fuoco amico e risorse | E11.1, E11.2 | **IMPLEMENTING** | 3/7 | `Combat.FriendlyFire` · `Combat.SplashHitsAlliesNotSelf` |

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

### Gameplay

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-CHARACTER-STATE` | Character State / Configuration System | — | **SPECIFIED** | 1/9 | _pianificato_ |
| `RT-FEAT-INTENT-CONDITIONAL` | Conditional Intent — un intento con una biforcazione | — | **SPECIFIED** | 1/9 | — |

### Networking

| Feature | Titolo | Roadmap | Stato | Gate | Scenario |
|---|---|---|---|---:|---|
| `RT-FEAT-NET-AUTHORITY` | Multiplayer con autorità server | M10 | **SPECIFIED** | 1/8 | — |
| `RT-FEAT-NET-DEDICATED` | Dedicated server | — | **IDEA** | 0/8 | — |
