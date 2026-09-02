# Spec — Animazione della risoluzione del turno (Resolution Playback)

> ℹ️ **Regola vigente, esempi datati.** Il playback resta **presentazione** (invariante #1) e vive in `URTPlaybackLibrary`, conservata al CP 7.2.
> Gli snippet citano API rimosse al **CP 7.2** (`URTGridLibrary`, `FRTGridCoord`): vanno letti come pseudo-codice, non come firme correnti.

> `/sc:spec-panel` del **2026-08-03**. Obiettivo utente: *«valutare il round, vedere muovere i cilindri,
> avere un senso della durata del round»*. Panel: Fowler (architettura), Nygard (robustezza di stato),
> Adzic (pacing/esempi), Crispin (testabilità/determinismo). Documentale: **nessuna modifica al codice**.
> Decisioni prese: **durata configurabile (default compatto) · movimento Move tutti in parallelo · spec prima del codice**.
> Ancorata al canone ([`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md)), alla roadmap
> ([`roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md)), al resolver movimento ([`spec-mappa-multilivello.md`](../technical/architecture/spec-mappa-multilivello.md)).

> ⚠️ **Riscritta nel modello a segmenti il 2026-08-08.** Questa spec assumeva che il lock-in calcolasse
> **una volta sola** l'intera timeline del round, e che il playback la riproducesse dall'inizio alla fine.
> Con [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) **non regge più**: una scelta live a un
> decision boundary può cambiare i segmenti successivi, quindi al lock-in il futuro del round **non è ancora
> scritto**. Vedi §3.1.
>
> Superata anche la riga «**stack/reazioni/Reaction Points restano fuori MVP**»: le finestre di reazione sono
> **in scope** (E14). Restano north-star lo stack **LIFO interattivo**, gli interrupt annidati e le `Patch`.
> Il materiale esplorativo è in
> [`../archive/gameplay/sequenza-turno-exploratory.md`](../archive/gameplay/sequenza-turno-exploratory.md);
> la sequenza canonica è [`spec-sequenza-turno.md`](spec-sequenza-turno.md).

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

- La logica resta **autoritativa**: il playback legge **eventi già risolti** e, se salta o accelera,
  **l'esito non cambia**.
- Il `DeltaTime` che muove i cilindri governa **solo i pixel**, non le regole → **non** viola l'invariante #4
  (che riguarda la *logica* dei turni, non la presentazione).

### 3.1 Il round si risolve a segmenti, non in un colpo solo

> ⚠️ La formulazione precedente diceva «lo stato finale è calcolato **una volta sola**, a lock-in».
> Era vera prima di [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md), e ora è falsa: se una finestra
> di reazione può cambiare cosa succede dopo, al lock-in la timeline futura **non esiste ancora**.

Modello corretto:

```text
COMMIT
  └─ snapshot del segmento
       → risolvi il segmento
       → emetti eventi autorevoli
       → riproduci il segmento                    (presentazione)
       → nessun decision boundary?  continua
       → decision boundary?
            ferma GLOBALMENTE la simulazione logica
            presenta la finestra
            la risposta diventa input autorevole  (entra nel TurnLog come dato)
            snapshot del segmento successivo
            riprendi
  └─ Cleanup
```

Cosa cambia **davvero** per questa spec, che resta di presentazione:

1. La timeline **non si emette una volta**: si emette **per segmento**. Il playback consuma quello che c'è e
   si ferma al boundary.
2. La durata del round **non è più nota al lock-in**: dipende da quante finestre si aprono e da quanto il
   giocatore ci mette. I target di §6 restano validi come **banda da misurare**, non come tempo calcolabile
   in anticipo.
3. Lo stato `Resolving` guadagna un sotto-stato di **attesa**: la presentazione può continuare in slow motion
   mentre la logica è ferma. Il rallentamento è **solo visuale** — se decidesse qualcosa, l'esito
   dipenderebbe dal frame rate.
4. **Skip e accelerazione restano leciti** fino al boundary, mai attraverso: saltare una finestra
   significherebbe scegliere al posto del giocatore. Il `Timeout → HOLD` è l'unica scelta automatica, ed è
   una funzione pura.

Il playback resta **presentation-only** in ogni caso: non decide, non ordina, non cambia l'esito.

## 4. Modello dati — timeline di eventi risolti

`LockInAndResolve` popola, oltre allo stato del segmento, una **timeline** ordinata per fase.
⚠️ Con ADR-0004 la timeline copre **il segmento corrente**, non l'intero round (§3.1):

```cpp
UENUM(BlueprintType)
enum class ERTResolvedEventType : uint8
{
    Move, Attack, HazardDamage, Defeated,
    AttackFootprint                          // l'impronta a terra di un attacco — D-301
};

USTRUCT(BlueprintType)
struct FRTResolvedEvent
{
    ERTMatchPhase        Phase;              // fase in cui l'evento è stato risolto
    ERTResolvedEventType Type;
    int32                SourceStableUnitId; // ID, non puntatore (#1800). 0 = nessuno (D-063)
    int32                TargetStableUnitId; // solo Attack
    TArray<FRTCellId>    Path;               // per Move: start + celle attraversate, in ordine
    TArray<FRTKnowledgeVerdict> CellVerdicts;// parallelo a Path per indice — D-223 / #1525
    int32                Amount;             // danno / scudo / durata, secondo Type

    // Solo AttackFootprint (D-301):
    TArray<FRTCellId>    HitCells;           // celle investite, ordine di HexHitCells
    ERTAbilityShape      Shape;              // dichiarata, non dedotta dal numero di celle
    FRTCellId            Origin;             // da dove il colpo è partito, al calcolo
    FRTCellId            AimCell;            // cella mirata: sopravvive anche senza vittime
};
```

> 🔴 **Il blocco qui sopra è stato riallineato il 2026-08-31 (#1945), e non descriveva più il codice da
> tempo.** Quattro derive, tre delle quali precedenti a questo lavoro:
>
> | Diceva | Dice il codice | Da quando |
> |---|---|---|
> | `StatusApplied` fra i valori | **non è mai esistito**: l'enum ne ha avuti quattro fino a oggi | dall'origine |
> | `TWeakObjectPtr<ARTUnit> Source/Target` | `int32 …StableUnitId` — un evento si confronta e si serializza **senza mondo** | [#1800](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1800) |
> | `FRTGridCoord` | `FRTCellId` — la griglia quadrata non è più nella partita | — |
> | *(assente)* | `CellVerdicts`, parallelo a `Path` | `D-223` / [#1525](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1525) |
>
> ⚠️ **La quarta riga è il motivo per cui `HitCells` non riusa `Path`**: quel campo ha ormai un compagno
> indicizzato, e un *insieme* di celle colpite non ha indici da mettere in parallelo.

- **Riuso**: `Path` = `FRTPathResult.Entered` (già prodotto); `Amount`/`Target` = dati già calcolati in
  `ResolveCombat`/Cleanup, oggi solo loggati in `RecentEvents`.
- **`AttackFootprint` non è un dato nuovo**: `HexHitCells` era già calcolato dal resolver una volta per
  intento e poi scartato (`URTHexCombatLibrary::CollectHexAttacks`). L'evento lo fa **sopravvivere** al
  confine, così la presentazione dell'area non deve ricalcolare una primitiva canonica — che `D-278`
  vieta. Ne esce **uno per intento aggressivo**, non uno per vittima: un'area su celle vuote produce zero
  colpi e un'impronta.
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
| `MaxPlaybackSeconds` | `~12` | oltre soglia → **le ATTESE si comprimono**, la locomozione no |

- ~~**Target 2v2 offline**: round tipico **≈ 6–12 s**.~~ **Aggiornato 2026-08-07**
  ([`spec-durata-partita-e-scala-mappe.md`](spec-durata-partita-e-scala-mappe.md) §9): playback tipico
  **8–15 s** in 2v2 e **12–20 s** in 3v3 Standard. I **45–60 s del PDF** restano fuori scala.
  ⚠️ Conseguenza da tarare quando la banda sarà misurata: `MaxPlaybackSeconds = 12` è **dentro** la nuova banda
  2v2, quindi lo speed-up automatico scatterebbe sui round più pieni invece che sui casi patologici. **Non si
  cambia adesso**: il valore si sposta col dato, non con la spec.
- **Speed-up automatico**: se la durata stimata supera `MaxPlaybackSeconds`, comprimere beat/animazioni
  minori mantenendo l'ordine eventi.

> ### 📌 Emendamento 2026-09-02 — il budget è soft, e ora il codice lo esegue (`#1878`)
>
> ⛔ **La regola qui sopra non cambia: cambia il fatto che sia vera.** *«Comprimere beat/animazioni
> minori»* è ciò che questa spec ha sempre prescritto, e non è ciò che il codice faceva.
>
> **Cosa faceva.** `ARTTurnManager::TickPlayback` derivava dal tetto un moltiplicatore di velocità
> (`URTPlaybackLibrary::SpeedMultiplierForCap`) e lo applicava a `Dt`, l'unico orologio del playback —
> lo stesso che produce l'`Alpha` con cui i cilindri si interpolano. Il tetto non comprimeva i beat:
> **accelerava tutto**, locomozione inclusa.
>
> **Perché nessuno se n'era accorto.** Misurato il 2026-09-02 su **125.780 risoluzioni** nei log: il tetto
> **non era mai intervenuto** — durata raw massima **4,4 s** contro 12, e zero moltiplicatori frazionari.
> Il ramo esisteva, contraddiceva questa spec, e non si era mai eseguito.
>
> **Cosa fa adesso.** `URTPlaybackLibrary::PhaseTime` scompone la fase in due termini — `Locomotion`
> (incomprimibile) e `Slack` (beat, e nel `Blast` il tempo di lettura che eccede la spinta) — e
> `URTPlaybackLibrary::SlackScaleForBudget` comprime **solo il secondo**. Quando il comprimibile finisce,
> la durata **sfora**: è la definizione di *soft*, ed è la decisione del PO del 2026-08-30 applicata al
> caso peggiore.
>
> ⚠️ **La nota di taratura qui sopra va letta al rovescio, adesso.** *«`MaxPlaybackSeconds = 12` è dentro
> la nuova banda 2v2, quindi lo speed-up scatterebbe sui round più pieni»* era un rischio quando lo
> speed-up accelerava i personaggi. Ora che comprime le attese, un tetto che morde sui round pieni è il
> comportamento voluto — e diventa **atteso** appena la velocità base scenderà, perché a rate più bassi la
> stessa risoluzione dura di più.
>
> ⚠️ Il riferimento «(PDF p.4)» nella tabella non è un'autorità: `CLAUDE.md` §1 esclude i PDF. Resta come
> traccia di provenienza, non come fonte.

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
  > ⚠️ **Aggiornamento del 2026-08-31**: `EstimatePlaybackSeconds` **non esiste più**. Era la stima
  > *aggregata* del round, e nessun codice di produzione la chiamava: il totale grezzo lo calcola già
  > `ARTTurnManager::BeginPlayback` sommando `URTPlaybackLibrary::PhaseDuration` su ogni fase attiva — che è
  > quanto la riga di **AN.3** qui sotto dichiara. Due formule per la stessa domanda, con risultati diversi
  > (`PhaseDuration` per il `Blast` prende `Max(colpi, spinta)`, non la somma): quella morta è stata tolta.
  > La riga sopra resta com'era perché registra cosa era vero il 2026-08-03.
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

- Canone: [`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md) — invarianti #1 (regole decidono), #4 (determinismo).
- Roadmap: [`roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md) — collocare AN.x come slice north-star post-MVP.
- Resolver movimento e `Entered`: [`spec-mappa-multilivello.md`](../technical/architecture/spec-mappa-multilivello.md),
  [`spec-pathfinding-pf3-pf4.md`](../technical/architecture/spec-pathfinding-pf3-pf4.md).
- Ispirazione (north-star, non canone): [`sequenza-turno-exploratory.md`](../archive/gameplay/sequenza-turno-exploratory.md)
  (timeline stile Phantom Brigade, batching, pacing ~60s, speed-up; **stack/reazioni fuori MVP**).
  *Trascrizione del PDF `sequenza-risoluzione-turno.pdf`, rimosso il 2026-08-12.*

---

## 14. Il ritmo cinematografico del turno — [`D-287`](../decisions/RT_PDR_00_Decision_Log.md), 2026-08-30

> ➕ **Aggiunta il 2026-08-30** dal consolidamento della decisione d'autore `AUTHOR-PRES-002`. Non supera
> nulla di ciò che precede: §3 resta il principio, §3.1 resta il modello a segmenti, §6 resta il pacing.
> Questa sezione dice **che forma ha** il tempo che §6 misura, e da dove la camera lo legge.

Il turno non diventa una successione di cutscene. **È il turno stesso a diventare una micro-scena**, e la
scena è generata dai risultati che il resolver ha già prodotto — mai da una regia che li anticipa.

### 14.1 I beat, e cosa NON sono

```text
PlanningEnter  →  ReadyTension  →  ResolutionLaunch  →  ResolutionNormal
                                                      ↘ CriticalImpact
                                    ResolutionSettle  ←
                                          ↓
                                    PlanningEnter (turno successivo)
```

🔴 **I beat nominano stati di PRESENTAZIONE, non fasi del loop.** Le fasi restano
`Planning → Prep → Dash → Blast → Move → Cleanup` e nessun beat ne aggiunge una: `ResolutionLaunch` non è
un momento in cui qualcosa si risolve, è il momento in cui si comincia a **mostrare** ciò che è già
risolto. Confonderli produrrebbe esattamente la fase fantasma che l'invariante #1 esiste per impedire.

Il rallentamento selettivo non è nuovo: §3.1 punto 3 lo dichiara già — *«la presentazione può continuare
in slow motion mentre la logica è ferma. Il rallentamento è solo visuale — se decidesse qualcosa, l'esito
dipenderebbe dal frame rate.»* Questa sezione gli dà un vocabolario, non un permesso.

### 14.2 Il ritmo è taratura, non canone — e i numeri sono candidati

| Beat | Ritmo candidato | Stato |
|---|---|---|
| `PlanningEnter` | ~`0,60`–`0,75x` percepito | ⏳ `PROPOSED FOR PLAYTEST` |
| `ReadyTension` | ~`0,80x` | ⏳ `PROPOSED FOR PLAYTEST` |
| `ResolutionLaunch` | ~`1,15`–`1,30x`, impulso breve | ⏳ `PROPOSED FOR PLAYTEST` |
| `ResolutionNormal` | `1,0x` | ✅ è già il default di `ViewerPlaybackSpeed` |
| `CriticalImpact` | ~`0,50`–`0,70x` per ~`0,15`–`0,30 s` | ⏳ `PROPOSED FOR PLAYTEST` |
| KO / counter / combo decisiva | ~`0,40`–`0,60x`, tetto ~`0,35 s` | ⏳ `PROPOSED FOR PLAYTEST` |
| `ResolutionSettle` | ~`0,70x` che rientra a `1,0x` | ⏳ `PROPOSED FOR PLAYTEST` |
| Finestra di Fast Reaction | **`1,0x`, sempre** | ✅ regola, non taratura — §14.5 |

⚠️ **Nessuno di questi numeri è canonico, ed è deliberato.** Sono ipotesi di partenza da provare guardando
un round vero, non costanti: è il principio 4 di **E49** — un default scritto in una spec prima di essere
provato diventa canone per inerzia. Il posto dove si tarano è il Camera Feature Lab,
[`#1780`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1780), che già possiede le altre
tarature camera aperte. **Chi promuove uno di questi valori lo fa con una misura in mano o non lo fa.**

### 14.3 Da dove passa il ritmo — un produttore solo

Il ritmo di presentazione **non guadagna un secondo produttore**. `ViewerPlaybackSpeed`
([`RTTurnManager.h`](../../Source/RefactorTactics/Turn/RTTurnManager.h)) è la preferenza di chi guarda e si
compone con l'accelerazione automatica in **un punto solo**, `URTPlaybackLibrary::EffectivePlaybackSpeed`,
che prende il massimo dei due. Un beat cinematico che volesse un proprio moltiplicatore passa di lì.

⛔ **E non passa da `SetGlobalTimeDilation`.** Misurato il 2026-08-30:
`git grep -n "SetGlobalTimeDilation" -- Source/` dà **tre** occorrenze e **zero** chiamate — sono un
commento e due oracoli che ne verificano l'assenza. Una dilatazione globale del tempo sarebbe autorità di
simulazione travestita da presentazione, ed è il modo più diretto per rendere un turno simultaneo in rete
non riproducibile.

✅ **Che il ritmo non tocchi l'esito è già dimostrato, non solo dichiarato**: il gate è
`RefactorTactics.Match.Autobattle.DeterminismIsIndependentOfPlayback`, e precede questa sezione.

> 🔴 **E su questa composizione esiste una decisione d'autore in volo, presa lo stesso giorno.** Il
> consolidamento *Resolution Pacing + Micro-step Synchronization* (Drive, `06 — Development`, 2026-08-30)
> osserva che dopo il Planning i personaggi **sembrano accelerati**, e ne trae una regola: *«la durata
> target della Resolution non deve determinare la velocità visuale base della locomozione;
> `MaxPlaybackSeconds` è un budget di presentazione soft»*. Il recupero di tempo dovrebbe venire prima da
> idle gap, beat non informativi, hold e transizioni di camera, code di VFX non critiche ed eventi
> logicamente simultanei mostrati in parallelo — **non** da un moltiplicatore nascosto sulla locomozione.
>
> ~~⚠️ **Quella regola NON è ancora canonica, e questa riga non la anticipa**: misurato il 2026-08-30,
> nessuna issue aperta la possiede~~ — **superata il 2026-09-02**: la issue esiste, è **`#1878`**, ed è
> stata lavorata. La regola è canonica e vive nell'emendamento di §6.
>
> ✅ **`#955` non è stata riaperta e non è stata contraddetta.** Scelse `Max(ViewerSpeed, CapSpeed)`
> scartando tre alternative, e una di quelle — *«solo Viewer»* — fu scartata perché avrebbe reso
> `MaxPlaybackSeconds` un campo morto. `#1878` **non è quella alternativa**: il tetto è vivo e ha un
> consumatore, `URTPlaybackLibrary::SlackScaleForBudget`. Cade il secondo argomento di
> `EffectivePlaybackSpeed` perché non ha più un produttore, non perché il tetto abbia smesso di valere.
> Ciò che questa sezione dichiarava resta vero nella forma che conta: **la composizione avviene in un
> punto solo**, e un beat non ha guadagnato un secondo produttore di velocità — ne ha perso uno.
>
> ➕ **Ma per il ritmo cinematico la direzione conta**: due delle sei voci che quel consolidamento vuole
> comprimere per prime — `camera hold` e `camera transitions` — sono **materia di `CAM-12`**. Chi prende
> quella issue e chi prende questa devono leggersi, altrimenti una comprime ciò che l'altra ha appena
> deciso di tenere.

### 14.4 Simultaneità — il segmento non si rompe per far posto all'inquadratura

Ciò che il resolver ha risolto **insieme** non può sembrare sequenziale solo perché una camera guarda un
posto per volta.

L'identità da preservare **esiste già e ha un nome**: è il **segmento di risoluzione** di §3.1, delimitato
dall'inizio di una macro-fase oppure da un decision boundary
([ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) §1).

⚠️ **Non si introduce un `BoundaryId` parallelo.** Misurato il 2026-08-30: `BoundaryId`, `StableEventId` e
`PresentationPriority` danno **0** occorrenze in `Source/`. Sono nomi di un kit esterno, non simboli del
repository; il concetto che descrivono è il segmento, e duplicarlo creerebbe due spazi di identità per lo
stesso fatto.

Regole:

- eventi dello stesso segmento **vicini** nello spazio possono condividere un'inquadratura;
- eventi dello stesso segmento **lontani** possono essere mostrati come gruppi consecutivi, **ma la UI deve
  continuare a dire che appartengono allo stesso segmento** — altrimenti la presentazione insegna al
  giocatore un ordine che nella logica non esiste, che è il difetto che §7 evita già per il Move;
- l'ordine di visita è **deterministico** e non dipende da `Tick`, ordine degli Actor, iterazione di
  `TMap`/`TSet`, ordine di spawn, arrivo dei pacchetti o fine di un'animazione (invarianti #4, #5, #6).

### 14.5 Fast Reaction — la decisione si guarda a `1,0x`

§3.1 punto 4 dice già che skip e accelerazione sono leciti **fino** al boundary e mai **attraverso**.
Questa sezione ne aggiunge il lato visivo:

- all'apertura di una finestra, il fast-forward si **sospende**;
- la finestra di decisione si presenta a **`1,0x`**;
- un `HARD FOCUS` è consentito e atteso se serve a rendere leggibile la scelta;
- alla chiusura autorevole, il ritmo cinematico riprende.

🔴 **E il tempo della decisione non è il tempo del playback.** I due sono già separati e nominati
dall'owner del pacing — [`spec-pacing-turno.md`](spec-pacing-turno.md): `Decision Time` e
`Presentation Time` si campionano **separatamente**, e mediarli nasconde il dato che serve. Nessuna
variazione di presentazione può spostare il Decision Boundary logico: allungare l'inquadratura non allunga
la finestra, e il `Timeout → HOLD` resta una funzione pura.

### 14.6 Planned-vs-Actual — una timeline sola

Il ritmo cinematico si aggancia alla presentazione Ghost già prevista; **non apre una seconda timeline**.
L'obiettivo è una frase in quattro tempi: *questo era il piano · questo è successo · qui è iniziata la
deviazione · questa ne è la causa autorevole*. Ghost e camera **consumano** il risultato del resolver; non
lo producono, e la causa che mostrano è quella che il TurnLog registra.

### 14.7 Cosa questa sezione NON autorizza

- ⛔ **Non autorizza a implementare `CAM-12`**: la grammatica della camera è in
  [`../technical/systems/spec-tactical-camera.md`](../technical/systems/spec-tactical-camera.md) §10, e le
  sue dipendenze sono lì.
- ⛔ **Non promuove i valori di §14.2.**
- ⛔ **Non tocca `ERTResolvedEventType`.** Se un beat richiedesse un tipo di evento nuovo, quello passa da
  [`D-278`](../decisions/RT_PDR_00_Decision_Log.md): ogni valore risolve in una voce di mapping o dichiara
  `NoPresentation`, e la copertura è imposta da un gate.
- 🔴 **Non sanifica `ResolvedTimeline`, che oggi non lo è.** `FRTResolvedEvent` porta
  `TWeakObjectPtr<ARTUnit> Source`/`Target` senza filtro di conoscenza: è il canale aperto
  [`#1525`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1525). Finché resta aperto,
  qualunque consumatore di presentazione costruito sopra quella timeline **eredita il leak** invece di
  evitarlo — vedi
  [`../technical/systems/conoscenza-parziale-visibile-spec.md`](../technical/systems/conoscenza-parziale-visibile-spec.md) §1.3.
