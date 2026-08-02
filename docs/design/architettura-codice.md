# RefactorTactics — Architettura del codice

Mappa del modulo C++ `Source/RefactorTactics/` allo stato attuale (MVP giocabile).
Per le decisioni vincolanti vedi [piano canonico](piano-canonico-mvp.md); per lo stato per checkpoint
vedi [roadmap](roadmap-checkpoint.md).

## Principi applicati

1. **Logica pura separata dagli Actor.** Le regole (griglia, movimento, danno, esito, bot) vivono in
   `UBlueprintFunctionLibrary` con funzioni statiche pure → **testabili senza mondo/Actor**. Gli Actor
   (`ARTUnit`, `ARTTurnManager`, …) orchestrano ma non contengono la matematica.
2. **Griglia logica vs rendering.** La posizione autorevole è `FRTGridCoord`; il `FVector` serve solo a
   `ARTGridActor`/`ARTUnit` per posizionare le mesh.
3. **Resolver "raccogli poi applica".** Movimento e attacchi calcolano l'esito su uno **snapshot** dello
   stato iniziale e lo applicano insieme → l'**ordine dell'input non cambia il risultato** (coperto da test).
4. **Autorità nel `ARTTurnManager`.** Il controller propone piani (preview); il turn manager valida
   (range, bersaglio nemico/vivo) e risolve. Predisposto al futuro server-authority.
5. **Presentazione isolata.** Camera, HUD, colore team sono presentazione: non decidono nulla sull'esito.

## Mappa per cartella

| Cartella / file | Tipo | Responsabilità |
|---|---|---|
| `RefactorTactics.{h,cpp}` | Modulo | Primary game module + categoria log `LogRT` |
| `Core/RTTypes.h` | `USTRUCT` | `FRTGridCoord{X,Y}` (posizione logica; `Layer` riservato al multilivello) |
| `Grid/RTGridLibrary` | Function Library | `CellToWorld` · `WorldToCell` · `IsInsideGrid` · `ManhattanDistance` · `IsWithinRange` (pure) |
| `Grid/RTGridActor` | `AActor` | Griglia visuale 10×10 (Instanced Static Mesh) posizionata via `RTGridLibrary` |
| `Selection/RTSelectable.h` | `UINTERFACE` | `IRTSelectable` (`OnSelected`/`OnDeselected`) |
| `Camera/RTCameraPawn` | `APawn` | Camera tattica: SpringArm inclinato, pan sul piano, zoom |
| `Player/RTPlayerController` | `APlayerController` | Enhanced Input **in C++**; selezione; pianificazione movimento/attacco; lock-in |
| `Unit/RTUnit` | `AActor` + `IRTSelectable` | Team, cella, HP/scudo, attacco (range/power), piani; colore team (`M_Unit`); eliminazione |
| `Turn/RTTurnRules` | Function Library | `ERTMatchPhase` + `NextPhase`; `ERTMatchOutcome` + `EvaluateOutcome` (pure) |
| `Turn/RTMovementResolver` | Function Library | `FRTMoveRequest`; `ResolveMoves` (conflitti, ordine-indipendente) |
| `Turn/RTTurnManager` | `AActor` | Orchestratore: fasi, timer 30s, `PlanBots`, `ResolveCombat` (Blast), `ResolveMovement` (Move), esito |
| `Combat/RTCombatLibrary` | Function Library | `FRTDamageResult`; `ApplyDamage` (scudo poi HP) |
| `Combat/RTCombatResolver` | Function Library | `FRTUnitCombatState`, `FRTAttack`; `ResolveAttacks` (raccogli-poi-applica) |
| `Bot/RTBotLibrary` | Function Library | `StepToward` (avvicinamento greedy entro il range) |
| `UI/RTHUD` | `AHUD` | Barre HP/scudo sopra le unità + "PARTITA FINITA" (disegno C++, no UMG) |
| `RTGameMode` | `AGameModeBase` | Allestisce il demo (griglia, luce, 2v2, turn manager); imposta pawn/controller/HUD; marca team 1 come bot |
| `Tests/` | Automation | `RTGridTests`, `RTMovementResolverTests`, `RTTurnRulesTests`, `RTCombatLibraryTests`, `RTCombatResolverTests`, `RTBotLibraryTests` — **27 test** |

## Flusso di un turno

```
Inizio pianificazione (ARTTurnManager::StartPlanningTimer)
  → PlanBots(): ogni unità team 1 sceglie il nemico più vicino
      · in portata d'attacco  → PlannedAttackTarget
      · altrimenti            → PlannedCell = StepToward(...)
  → il giocatore pianifica (click): PlannedCell / PlannedAttackTarget (preview, con check range)
  → timer 30s

Lock-in (Spazio) oppure timeout → LockInAndResolve()
  → NextPhase fino a tornare a Planning:
      · fase Blast → ResolveCombat():
          raccoglie gli attacchi validi (nemico/vivo/in portata, posizione ATTUALE)
          ResolveAttacks (snapshot, danni sommati per bersaglio) → ApplyCombatState → eliminazione
      · fase Move  → ResolveMovement():
          sanitizza i piani fuori range → ResolveMoves (conflitti) → PlaceOnCell
  → EvaluateOutcome(vivi team0, vivi team1):
      · InProgress → nuovo turno (StartPlanningTimer)
      · altrimenti → MatchEnded (turni fermi, HUD "PARTITA FINITA")
```

**Ordine delle fasi**: Blast **prima** di Move → si spara dalla posizione attuale, poi ci si sposta
(modello *Atlas Reactor*).

## Come si estende (per le prossime feature)

- **Abilità data-driven** (M3.1): `URTAbilityData : UPrimaryDataAsset` con fase/range/potenza; `ARTUnit`
  espone una lista di abilità invece dell'attacco base hardcoded.
- **Status/energia/forme/LOS** (M3.3–3.6): nuove Function Library pure + campi su `ARTUnit`, agganciate
  nelle fasi giuste del `RTTurnManager` (Prep per buff/status, Blast per il targeting a forme).
- **Multiplayer** (post-MVP): il `RTTurnManager` è già il punto di autorità; i piani diventano RPC
  server-side con replica filtrata per squadra (privacy dell'intento).

Ogni nuova regola nasce come **funzione pura con test** prima del wiring negli Actor.
