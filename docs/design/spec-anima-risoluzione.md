# Spec — Animazione della risoluzione del turno (Resolution Playback)

> `/sc:spec-panel` del **2026-08-03**. Obiettivo utente: *«valutare il round, vedere muovere i cilindri,
> avere un senso della durata del round»*. Panel: Fowler (architettura), Nygard (robustezza di stato),
> Adzic (pacing/esempi), Crispin (testabilità/determinismo). Documentale: **nessuna modifica al codice**.
> Decisioni prese: **durata configurabile (default compatto) · movimento Move tutti in parallelo · spec prima del codice**.
> Ancorata al canone ([`piano-canonico-mvp.md`](piano-canonico-mvp.md)), alla roadmap
> ([`roadmap-checkpoint.md`](roadmap-checkpoint.md)), al resolver movimento ([`spec-mappa-multilivello.md`](spec-mappa-multilivello.md)).

> ⚠️ **Conflitto di fonte.** Il PDF *«RefactorTactics – Sequenza di Risoluzione del Turno»* descrive un
> modello **WEGO + stack LIFO (Magic) + reazioni/patch** (ispirato a Phantom Brigade/MtG) che **diverge**
> dalle fasi canoniche Prep→Dash→Blast→Move. È materiale **esplorativo/north-star**. Questa spec ne usa
> **solo** la parte su **presentazione e pacing** (timeline, batching parallelo/serializzato, ritmo ~60s,
> speed-up automatico). **Stack/reazioni/Reaction Points restano fuori MVP** e richiederebbero un ADR dedicato.
> Il presente lavoro è **north-star / fuori MVP-core**: si affronta a MVP chiuso o come slice opzionale.

## 1. Obiettivo

Rendere il round **osservabile**: i cilindri devono **muoversi visibilmente** lungo il percorso risolto e il
round deve avere una **durata percepibile** e regolabile — **senza** toccare la logica deterministica né gli
esiti. Il resolver continua a decidere; nasce un layer di **presentazione temporizzata** che *riproduce*
eventi già risolti.

## 2. Stato attuale (verificato)

- `ARTTurnManager::LockInAndResolve` (`Source/RefactorTactics/Turn/RTTurnManager.cpp:208-292`) esegue **tutte
  le fasi in un unico `do/while` sincrono** (Prep→Dash→Blast→Move→Cleanup) in un frame, poi chiama subito
  `StartPlanningTimer()`. **La fase di esecuzione dura 0 s.**
- `ARTTurnManager::ResolveMovement` (`:478-553`) calcola i percorsi con `URTMovementResolver::ResolvePaths`,
  salva le rotte in `LastMoveRoutes`, **ma poi chiama `Units[i]->PlaceOnCell(Resolved[i].Final, …)`** →
  **snap istantaneo** alla cella finale.
- `ARTUnit::PlaceOnCell` (`Source/RefactorTactics/Unit/RTUnit.h:189`) posiziona la mesh via `SetActorLocation`
  immediato. **Nessuna interpolazione.**
- `PlanningSeconds = 30.f` (`RTTurnManager.h:65`) — già allineato al PDF. Manca invece qualunque durata per la
  risoluzione.
- Il **dato per animare esiste già**: `FRTPathResult.Entered` (celle attraversate, ordine-indipendente) e
  `GetLastMoveRoutes()` (usato dalla HUD solo come scia grigia post-lock). Va **riprodotto sui cilindri**,
  non solo disegnato.

∴ Oggi non c'è nulla da vedere muoversi e il round non ha durata percepibile, ma il percorso risolto è già
disponibile: manca il layer che lo riproduce nel tempo.

## 3. Principio fondante

> **Le regole decidono l'esito; l'animazione riproduce.** (Invariante #1 del canone; coerente col PDF p.4
> *«Sempre salvaguardare l'ordine degli eventi»*.)

- La logica resta **sincrona e autoritativa** in `LockInAndResolve`: lo stato finale è calcolato **una volta
  sola**, a lock-in.
- Il playback legge **eventi già risolti**: se salta/accelera, **l'esito non cambia**.
- Il `DeltaTime` che muove i cilindri governa **solo i pixel**, non le regole → **non** viola l'invariante #4
  (che riguarda la *logica* dei turni, non la presentazione).

## 4. Modello dati — timeline di eventi risolti

`LockInAndResolve` popola, oltre allo stato finale, una **timeline** ordinata per fase:

```cpp
UENUM()
enum class ERTResolvedEventType : uint8 { Move, Attack, StatusApplied, HazardDamage, Defeated };

USTRUCT()
struct FRTResolvedEvent
{
    ERTMatchPhase        Phase;      // fase in cui l'evento è stato risolto
    ERTResolvedEventType Type;
    TWeakObjectPtr<ARTUnit> Source;  // attore già risolto (weak: può essere logicamente morto)
    TWeakObjectPtr<ARTUnit> Target;  // per Attack/StatusApplied
    TArray<FRTGridCoord> Path;       // per Move = Entered (celle attraversate, in ordine)
    int32 Amount = 0;                // danno / scudo / turni-status, secondo Type
};
```

- **Riuso**: `Path` = `FRTPathResult.Entered` (già prodotto); `Amount`/`Target` = dati già calcolati in
  `ResolveCombat`/Cleanup, oggi solo loggati in `RecentEvents`.
- La timeline è un **sottoprodotto della logica esistente**: non introduce nuove decisioni di gioco, quindi
  **non altera** determinismo o test (§8).

## 5. Architettura — stato `Resolving`

```
OGGI      [Planning 30s] → LockInAndResolve (0s, snap)          → [Planning 30s] → …
PROPOSTO  [Planning 30s] → LockIn (calcola + emette timeline, 0s)
                         → [Resolving Ns: riproduce la timeline]  → [Planning 30s] → …
```

Nuovo valore in `ERTMatchPhase` (o stato di presentazione parallelo): **`Resolving`**. Tra il calcolo e il
ritorno a `Planning`, il TurnManager:

1. entra in `Resolving`, **blocca l'input di pianificazione**;
2. avanza la timeline **per fase** (beat Prep→Dash→Blast→Move→Cleanup);
3. anima i cilindri interpolando la posizione **logica→mondo** lungo `Path`;
4. a fine timeline chiama `StartPlanningTimer()` per il turno successivo.

**Divisione C++/Blueprint** (rispetta il canone):

| Livello | Responsabilità |
|---------|----------------|
| **C++** (`ARTTurnManager` + `ARTUnit`) | stato `Resolving`, avanzamento timeline via Tick **di presentazione**, interpolazione lineare della posizione dei cilindri lungo `Path`. |
| **Blueprint** (presentazione) | camera/VFX/SFX/easing, agganciati a **delegate** esposti dal TurnManager. |

Delegate proposti (BlueprintAssignable): `OnPhasePlaybackStarted(ERTMatchPhase)`,
`OnUnitMoveStarted(ARTUnit*, const TArray<FRTGridCoord>&)`, `OnAttackResolved(ARTUnit* Src, ARTUnit* Tgt, int32 Dmg)`,
`OnUnitDefeated(ARTUnit*)`, `OnResolvePlaybackFinished()`.

## 6. Pacing / durata (DECISO: configurabile, default compatto)

Parametri `UPROPERTY(EditAnywhere)` sul TurnManager → tuning **in editor senza ricompilare**:

| Parametro | Default (compatto) | Effetto |
|-----------|--------------------|---------|
| `PlaybackCellsPerSecond` | `~6.5` (≈0.15 s/cella) | velocità di scorrimento dei cilindri nel Move |
| `PhaseBeatSeconds` | `~0.30` | pausa tra una fase e la successiva |
| `AttackShowSeconds` | `~0.50` | durata di visualizzazione di un colpo + numero di danno |
| `MaxPlaybackSeconds` | `~12` | oltre soglia → **speed-up automatico** (PDF p.4) |

- **Target 2v2 offline**: round tipico **≈ 6–12 s**. I **45–60 s del PDF** valgono per **3-4 unità +
  finestre di reazione**, assenti nell'MVP → non replicabili né desiderabili qui. *(Raccomandazione, da
  confermare in PIE.)*
- **Speed-up automatico**: se la durata stimata supera `MaxPlaybackSeconds`, comprimere beat/animazioni
  minori mantenendo l'ordine eventi.

## 7. Batching (DECISO: Move in parallelo)

- **Fase Move — tutti in parallelo**: tutte le unità che si muovono scorrono **contemporaneamente** lungo i
  rispettivi `Path`. Coerente con il resolver **order-independent** e con il *batching* del PDF (p.2):
  *risoluzione simultanea = visione simultanea*. Evita di suggerire un ordine che nella logica **non esiste**.
- **Fase Blast — leggermente serializzabile**: i colpi possono susseguirsi con un piccolo stacco
  (`AttackShowSeconds`) per leggibilità dei numeri di danno. È qui che nasce gran parte del *senso di durata*.

## 8. Robustezza di stato (Nygard)

- **Morte logica vs rimozione visiva.** `ApplyCombatState` può azzerare gli HP a lock-in: la **morte logica**
  avviene subito (conteggio squadre/esito immediati e corretti), ma la **rimozione visiva** è un **evento
  `Defeated` della timeline** riprodotto al momento giusto (tipicamente Cleanup, o subito dopo il colpo che
  uccide nel Blast). Il cilindro **non** deve sparire prima di aver mostrato il colpo che l'ha ucciso.
- Conseguenza: la distruzione dell'`Actor` va **posticipata a fine playback** (o gestita con mesh nascosta),
  non eseguita durante `LockInAndResolve`. `Source`/`Target` sono `TWeakObjectPtr` per tollerare unità
  logicamente morte.
- **Input**: durante `Resolving` la pianificazione è bloccata; il nuovo turno parte solo a
  `OnResolvePlaybackFinished`.

## 9. Testabilità e determinismo (Crispin)

- **Gli automation test non eseguono il playback**: chiamano la logica sincrona e verificano lo **stato
  finale**. Se i **54 test** restano verdi dopo l'introduzione della timeline (AN.1), è la prova che l'esito è
  **indipendente** dall'animazione.
- Test C++ mirato AN.1: «la timeline emessa **non altera** stato finale né esiti dei test esistenti»; verifica
  che, per un caso noto, gli eventi emessi corrispondano per **numero/tipo/ordine** a ciò che la logica ha
  applicato (senza doppie applicazioni).
- Il playback (AN.2+) si verifica **in PIE**, non in automation.

## 10. Roadmap (slicing minimale, TDD-ready)

| ID | Cosa | Verifica |
|----|------|----------|
| **AN.1** | `FRTResolvedEvent` + emissione timeline in `LockInAndResolve` (logica pura) | Test C++: esito invariato, 54 test verdi |
| **AN.2** | Stato `Resolving` + interpolazione lineare dei cilindri lungo `Path` (Move in parallelo) | PIE: **si vedono muovere** |
| **AN.3** | Staging per fase con beat/durata **configurabili** (§6) + speed-up | PIE: **senso di durata** |
| **AN.4** | Delegate BP (camera/VFX/SFX) + morte visiva come evento (§8) + skip manuale | PIE + BP |
| **AN.5** | HUD: barra fase/timeline + pulsante **Skip** | PIE |
| **AN.6** | Morte visiva differita (`NewlyDefeated` + evento `Defeated` + hide + distruzione a fine turno) | Test C++ (`NewlyDefeated`) + PIE: colpo mostrato → poi morte |

## 11. Decisioni

**Prese (2026-08-03):**
- Durata: **configurabile** via `UPROPERTY`, **default compatto** (§6).
- Movimento fase Move: **tutti in parallelo** (§7).
- Ordine di lavoro: **spec prima del codice** (questo documento).

**Aperte (da confermare, tipicamente in PIE):**
- Valori numerici di default finali (`CellsPerSecond`, beat, cap) — da tarare guardando un round reale.
- Se il Blast serializza i colpi o li mostra simultanei come il Move.
- Momento esatto della rimozione visiva del `Defeated` (subito dopo il colpo vs Cleanup).
- Skip solo manuale, o anche auto-speedup oltre `MaxPlaybackSeconds` da subito.

## 12. Stato di implementazione (2026-08-03)

**Implementato e verificato** (TDD per la logica pura, PIE+log per il wiring):

- **AN.1** ✅ — `FRTResolvedEvent` (`Turn/RTResolvedEvent.h`) + timeline popolata in `LockInAndResolve`
  (eventi Move/Attack). Matematica pura in `URTPlaybackLibrary` (`InterpolateAlongPath`,
  `EstimatePlaybackSeconds`, `SpeedMultiplierForCap`) con **5 test** (`RTPlaybackLibraryTests.cpp`), RED→GREEN.
- **AN.2** ✅ — stato `Resolving` (Tick di sola presentazione) + interpolazione dei cilindri lungo `Entered`.
  Log PIE: `Playback fase: Move` → `Risoluzione completata (0.8s)` con 4 unità in parallelo.
- **AN.3** ✅ — staging per fase (Prep→Blast→Move) + tuning `UPROPERTY` (`PlaybackCellsPerSecond`,
  `PhaseBeatSeconds`, `AttackShowSeconds`, `MaxPlaybackSeconds`, `bEnablePlayback`) + auto speed-cap. La durata
  totale è la somma delle durate di fase (progress bar coerente: stima `~0.8s` == reale `0.8s`).
- **AN.4** ✅ — delegate Blueprint (`OnPhasePlaybackStarted`, `OnUnitMoveStarted`, `OnAttackResolved`,
  `OnResolvePlaybackFinished`) + `SkipPlayback` (Spazio durante la risoluzione). Log PIE: `Risoluzione: salto`.
- **AN.5** ✅ — HUD: barra di stato con fase/percentuale + hint "Spazio: salta"; intent-preview nascosta durante
  il playback; traccia grigia del percorso mantenuta.
- **AN.6 (morte visiva differita)** ✅ — `ApplyCombatState` non distrugge più l'Actor: la morte è solo logica
  (HP=0), la rimozione visiva avviene nel playback (evento `Defeated`) alla fine della fase in cui l'unità muore
  (Blast/Move), la distruzione dell'Actor a fine turno (`ConcludeTurn::DestroyDefeatedUnits`). Logica pura
  `URTCombatLibrary::NewlyDefeated` (chi è vivo prima e morto dopo) con **1 test** (RED→GREEN). I morti non
  partecipano più al movimento (filtro in `ResolveMovement`). Delegate `OnUnitDefeated` per VFX/SFX di morte.
  **Verifica PIE (timestamp)**: `Colpo … RTUnit_0 (25)` a `07.05.44:772` → `Morte mostrata: RTUnit_0` a
  `07.05.45:267` (~0.5s dopo); la morte è mostrata solo quando il colpo è **davvero letale**; nessun crash.

**Regressione**: 60/60 automation test verdi (54 preesistenti + 5 playback + 1 `NewlyDefeated`) → invariato l'esito.

**Limiti noti / aperti**:
- **Valori di tuning** (`6.5` celle/s, beat `0.30`, colpo `0.50`, cap `12`) sono default compatti da **tarare
  in gioco**; editabili in editor senza ricompilare.
- **Verifica in sessione unattended**: in `-game -unattended` la finestra può ricevere input spurio (Spazio →
  lock-in), accelerando i turni; il timer di pianificazione reale è ~30s (confermato: senza input la
  pianificazione attende il timer). Per verificare il playback: attendere il lock-in automatico o premere Spazio.

## 13. Riferimenti

- Canone: [`piano-canonico-mvp.md`](piano-canonico-mvp.md) — invarianti #1 (regole decidono), #4 (determinismo).
- Roadmap: [`roadmap-checkpoint.md`](roadmap-checkpoint.md) — collocare AN.x come slice north-star post-MVP.
- Resolver movimento e `Entered`: [`spec-mappa-multilivello.md`](spec-mappa-multilivello.md),
  [`spec-pathfinding-pf3-pf4.md`](spec-pathfinding-pf3-pf4.md).
- Ispirazione (north-star, non canone): `docs/RefactorTactics – Sequenza di Risoluzione del Turno.pdf`
  (timeline stile Phantom Brigade, batching, pacing ~60s, speed-up; **stack/reazioni fuori MVP**).
