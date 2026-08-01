# RefactorTactics — Piano Canonico MVP

> **Stato**: bozza di lavoro · **Ultimo aggiornamento**: 2026-08-01
> **Scopo**: riconciliare i due corsi tutorial (`02-Tutorial` e `03-TutorialToMVP`) in un'unica
> specifica operativa coerente, risolvendo le contraddizioni prima di scrivere codice.
> Questo documento è la **fonte di verità** per l'implementazione dell'MVP. In caso di
> conflitto con i PDF in `docs/`, prevale questo file.

---

## 1. Gerarchia delle fonti

| Livello | Documenti | Ruolo |
|---|---|---|
| **Canonico (vincolante)** | *questo file* | Decisioni operative per l'MVP |
| **Corsi di riferimento** | `02-Tutorial.pdf`, `03-TutorialToMVP.pdf` | Materiale didattico da cui deriva il piano |
| **Visione (north-star)** | i 3 PRD + `Intenti condivisi.pdf` | Prodotto a lungo termine, **non** scope attuale |
| **Storico / superato** | `00-Intro.pdf`, `01-StrutturaTutorial.pdf` | Brief iniziali, mantenuti per contesto |

I 4 PRD descrivono un prodotto molto più ambizioso (4v4 competitivo, GAS, netcode avanzato,
modding). Sono la **direzione futura**, non l'obiettivo dell'MVP. Vedi §8.

---

## 2. Identità del progetto

- **Nome**: **RefactorTactics** (coincide col repository; i tutorial usano
  `ReactorTactics`/`Reactor Tactics` — allineati a questo nome).
- **.uproject**: `RefactorTactics.uproject` nella **radice del repository**
  (`D:\Repositories\refactor-tactics-main`), così il progetto UE è versionato col resto.
- **Natura**: gioco tattico PvP a **turni simultanei** ispirato ad *Atlas Reactor*, sviluppato
  come **percorso didattico per imparare Unreal Engine 5.8** partendo da un profilo C#.
- **Nota IP**: il riferimento ad Atlas Reactor è solo meccanico. Per un'eventuale pubblicazione
  servono nomi, personaggi, lore e asset **originali** (decisione aperta, vedi §9).

---

## 3. Decisioni canoniche — riconciliazione `02` ↔ `03`

| Tema | `02-Tutorial` | `03-TutorialToMVP` | **Canonico** | Motivazione |
|---|---|---|---|---|
| Nome / `.uproject` | ReactorTactics | Reactor Tactics | **RefactorTactics** | Deciso; = repo |
| Prefissi classi | `AT` / `UAT` | `RT` / `URT` | **`RT` / `URT`** | Coerente col nome; `03` è la spina dorsale |
| Scope MVP | Multiplayer 1v1 | Offline 2v2 vs bot | **Offline 2v2 vs bot** | Minor rischio per dev singolo; multiplayer rimandato |
| Split linguaggi | ~60% C++ | ~70% Blueprint | **Regole in C++, presentazione in Blueprint** (nessuna % fissa) | Principio condiviso; la % è conseguenza |
| Griglia | 12×12 @ 100 cm | 10×10 @ 200 cm | **10×10 @ 200 cm** | Base `03`; celle da 200 cm adatte alla scala UE. Tunable |
| Coord. cella | `FGridCoord{X,Y}` | `FIntPoint` | **`FRTGridCoord{X,Y}`** (`Layer` riservato) | USTRUCT estendibile verso mappa multilivello (visione PRD) |
| Enum fasi | `EATMatchPhase` + `EATActionPhase` | `ERTCombatPhase` | **`ERTMatchPhase{ Planning, Prep, Dash, Blast, Move, Cleanup, MatchEnded }`** | Un solo enum; fasi Prep/Dash/Blast/Move condivise |
| Resolver | `UATTurnResolver : UObject` | `ARTTurnManager : Actor` | **`URTTurnResolver : UObject`** (logica pura) orchestrato da **`ARTTurnManager`** | Testabile, senza dipendenze da Actor |
| Combat math | `FATCombatMath` (struct) | funzione libera `CalculateDamage` | **`URTCombatLibrary`** (BlueprintFunctionLibrary, funzioni pure) | Testabile e richiamabile da Blueprint |
| Griglia visuale | `BP_CellHighlight` | Instanced Static Mesh | **Instanced Static Mesh** | Più efficiente |
| Timer pianificazione | 30 s | 15–30 s | **30 s (configurabile)** | Entrambi ~30 s |
| Versione UE | 5.8.1 | 5.8.x | **5.8.1** | Deciso; bloccata per l'intero corso |
| Ability system | `UPrimaryDataAsset`, no GAS | `UPrimaryDataAsset`, no GAS | **`URTAbilityData : UPrimaryDataAsset` (no GAS)** | Concordi; GAS post-MVP |
| Energia / Ultimate / Status / Tag | assenti | presenti | **Presenti** | Parte dello slice `03` |
| Targeting a forme | assente | Self/Single/Line/Cone/Circle | **Presente** | Feature dello slice |
| Bot AI | assente | utility scoring | **Presente** (utility scoring) | Necessario per 2v2 offline |
| Vittoria | 3 elim. o miglior punteggio/10 turni | squadra eliminata | **Squadra eliminata** (niente punteggio nell'MVP) | Il "punteggio" non è mai definito in `02` → escluso |
| Test | Automation Framework | Automation Framework + CI | **Automation Framework** (CI = milestone successiva) | CI self-hosted è avanzata |
| Networking | core (lez. 9) | roadmap P0 post-tutorial | **Rimandato post-MVP**, ma architettura *server-authority-ready* | Si mantiene la disciplina di `02` come target |
| Percorso progetto | `C:\Dev\...` | `D:\Dev\...` | **Radice del repo** | Il progetto UE vive nel repo versionato |

---

## 4. Definizione dell'MVP (vertical slice)

Checklist di completamento — l'MVP è "fatto" quando **tutte** queste voci sono vere:

- [ ] Il gioco si avvia direttamente nell'arena
- [ ] **2v2**: 2 unità alleate (giocatore) + 2 nemiche (bot)
- [ ] Selezione delle proprie unità
- [ ] Pianificazione: per ogni unità si scelgono **abilità + bersaglio + movimento**
- [ ] Lock-in del turno con **timer 30 s**
- [ ] Il **bot** pianifica le unità nemiche (utility scoring)
- [ ] Risoluzione a 4 fasi **Prep → Dash → Blast → Move**, deterministica ("raccogli poi applica")
- [ ] Combattimento: danni, **scudi**, **energia/ultimate**, **status** (Root/Slow/Shield/Reveal via Gameplay Tags), **LOS/copertura**
- [ ] **Targeting a forme** (Self/Single/Line/Cone/Circle)
- [ ] Vittoria: la partita termina quando una squadra è eliminata
- [ ] UI: salute, energia, abilità, timer, fase, **combat log**
- [ ] Test automatici: griglia, danno-dopo-scudo, ordine fasi, conflitti di movimento
- [ ] Build Windows (**Development** e **Shipping**) che parte senza Editor

---

## 5. Architettura & invarianti

Principi non negoziabili (valgono anche in offline, per preparare il multiplayer futuro):

1. **Autorità delle regole in C++**: il resolver decide l'esito; le animazioni/VFX non decidono nulla.
2. **Griglia logica autoritativa**: la posizione vera è `FRTGridCoord`; il `FVector` serve solo al rendering.
3. **Resolver "raccogli poi applica"**: snapshot a inizio fase, nessun `Delay`/timeline/montage nel resolver; l'ordine dell'array **non** deve cambiare il risultato.
4. **Privacy dell'intento**: nessuna mossa avversaria viene mostrata/replicata durante la pianificazione (invariante rilevante all'arrivo del multiplayer — invariante di `Intenti condivisi`).
5. **Combat math = funzioni pure** in `URTCombatLibrary`, coperte da test.

### Classi principali (prefissi `RT`/`URT`)

| Classe | Tipo | Responsabilità |
|---|---|---|
| `ARTGameMode` | GameMode | Regole autorevoli, avanzamento fasi |
| `ARTGameState` | GameState | Stato replicabile (fase, turno, timer) |
| `ARTPlayerController` | PlayerController | Input, selezione, invio piani |
| `ARTCameraPawn` | Pawn (BP `BP_TacticalCamera`) | Camera tattica pan/zoom |
| `ARTTurnManager` | Actor | Orchestrazione del turno |
| `URTTurnResolver` | UObject | Logica pura di risoluzione fasi |
| `ARTUnit` | Actor | Unità: team, HP, scudo, energia, cella, abilità |
| `URTAbilityData` | PrimaryDataAsset | Definizione data-driven abilità |
| `URTGridLibrary` | BlueprintFunctionLibrary | Utility griglia (Manhattan, celle) |
| `URTCombatLibrary` | BlueprintFunctionLibrary | Calcoli puri danno/copertura |

### Convenzioni asset

`BP_` Blueprint · `WBP_` Widget · `BPI_` Interface · `DA_` DataAsset · `DT_` DataTable ·
`IA_` InputAction · `IMC_` InputMappingContext · `L_` Level · `M_`/`MI_` Material/Instance ·
`T_` Texture · `NS_` Niagara · `S_` Sound.

### Layout del progetto (radice repo)

```
RefactorTactics.uproject
Source/RefactorTactics/        # codice C++ (Core, Grid, Units, Turn, Combat, UI)
Content/                        # asset (Blueprints, Maps, Characters, UI, ...)
Config/                         # DefaultEngine.ini, DefaultGame.ini, ...
docs/                           # documentazione (questa cartella)
```

---

## 6. Regole di gioco numeriche (valori iniziali, da bilanciare)

| Parametro | Valore |
|---|---|
| Griglia | 10×10, cella 200 cm |
| Occupazione | max 1 unità/cella |
| Movimento normale | 4 celle |
| Dash | 3 celle |
| HP unità | 100 |
| Scudo base | 20 (assorbito prima degli HP) |
| Attacco base | 30 |
| Energia | max 100 (guadagno per turno / per colpo: **da bilanciare**) |
| Copertura | riduce il danno (valore da bilanciare; `02` cita 50%) |
| Timer pianificazione | 30 s |
| Conflitto: destinazione contesa | entrambe le unità restano ferme |
| Conflitto: scambio diretto | consentito |
| Conflitto: cella occupata da unità ferma | movimento bloccato |
| Morte | applicata dopo l'intero batch della fase Blast |

---

## 7. Roadmap MVP (~14 settimane, dev singolo 8–12 h/sett)

| Milestone | Contenuto | Fine indicativa |
|---|---|---|
| **M0 Fondamenta** | Toolchain, progetto Blank C++ `RefactorTactics` nel repo, Git LFS, classi base | sett. 2 |
| **M1 Sandbox** | Camera tattica, Enhanced Input, selezione, griglia logica + visuale | sett. 5 |
| **M2 Turn loop** | Pianificazione, state machine fasi, timer, lock-in, risoluzione movimento simultaneo | sett. 8 |
| **M3 Combat loop** | Abilità data-driven, danni/scudi/morte, energia/ultimate, status/Tag, targeting a forme, LOS | sett. 11 |
| **M4 Vertical slice** | Bot (utility scoring), HUD + combat log, condizione di vittoria, riavvio | sett. 13 |
| **M5 Release interna** | Test in Automation Framework, packaging Windows, Definition of Done | sett. 14 |

---

## 8. North-star (post-MVP, dai PRD)

Esplicitamente **fuori** dall'MVP, in ordine di priorità indicativa (P0 → P3):

- **P0**: pulizia resolver deterministico, test conflitti, **multiplayer server-authoritative** (replica azioni/GameState, timeout/reconnect), lobby privata.
- **P1**: 4v4, 6+ eroi, draft, fog of war, replay, spectator, matchmaking, ranking, **Intenti condivisi** completo.
- **P2**: loadout/moduli, **migrazione a GAS**, tutorial interattivo, accessibilità, localizzazione, controller, Steam/EOS, mappa multilivello.
- **P3**: console, cosmetici, anti-cheat, **modding** (Blueprint sandbox *vs* Lua/UnLua — **decisione aperta** nei PRD).

### 8.1 Riferimento dettagliato (north-star)

Il documento più completo per il post-MVP è **`docs/RefactorTactics_ Product Requirements Document e piano
completo di sviluppo.pdf`** (45 pagine). Rispetto ai 3 PRD aggiunge decisioni/specifiche non altrove presenti,
da usare come riferimento quando le relative feature entrano in scope:

- **Modalità "Relay Control"**: relay da controllare a fine round, ruota ogni 2 round, vittoria a punteggio, knockout con rientro, max 12 round.
- **Economia del round + 7 intent label**: Focus, Protezione, Scout, Controllo, Fuga, Trappola, Attesa (1 movimento + 1 azione + 0-1 reazione + 0-1 interazione, 1 label).
- **Config build competitiva**: chassis fisso, 1 spec su 3, 2 modificatori abilità, 2 gadget, 2 tratti a budget, 1 ultimate su 2-3 + tabella varianti.
- **Formule**: costo traversal, euristica A* multilivello, utility IA (nel PDF).
- **Modello dati ricco**: 12 struct (`FRTGridCellId`, `FRTCellStaticData/RuntimeState`, `FRTTraversalEdge/Profile`, `FRTPlannedAction/Turn`, `FRTTeamIntentView`, `FRTResolvedEvent`, `FRTModManifest`, `FRTRunState`) + 12 moduli `RTCore…RTEditor`.
- **Roguelike cooperativo**: 1-4 giocatori, 3 atti, deck 8-12, energia, reliquie, save versionati, "stanze curate + validator".
- **Modding**: 4 livelli (Data/Content/Game Feature/Native), 3 schemi JSON, 16 operazioni autorizzate, handshake su hash del manifest.
- **Analytics/Test/Rilascio**: 15 eventi analytics, test plan (unit/deterministico con golden hash/rete/funzionale) + log per-round, 13 gate di release, risk register (15 rischi).
- **Toolchain/hardware** e tabella migrazione C#→Unreal.

**Conflitti col MVP** (restano distinzioni north-star, NON cambiano l'MVP): il PDF è **4v4**, mette **GAS** nello
stack, vittoria **a punteggio**, pianificazione **40-60s**. L'MVP resta 2v2, no-GAS, vittoria per eliminazione,
timer 30s. Nota naming: il PDF usa `FRTGridCellId` (modello ricco); l'MVP usa `FRTGridCoord{X,Y}` (semplice) —
da riconciliare se in futuro si adotta il modello a chunk multilivello.

---

## 9. Decisioni ancora aperte (business & prodotto)

Non bloccano l'MVP tecnico, ma vanno decise crescendo:

- **Budget** e **modello commerciale**: non specificati in nessun documento.
- **Team**: assunzioni discordanti (single-dev / 3–5 FTE / 4–6). L'MVP assume **dev singolo**.
- **Direzione artistica**: inesistente. L'MVP usa asset placeholder / Starter Content.
- **PC target hardware**: mai definito → i target di performance (60 FPS, A* <2 ms, turn <100 ms) restano **budget da misurare**, non garanzie.
- **Piattaforme**: MVP solo Windows; controller/console rimandati.
- **Identità originale**: nomi eroi, lore e naming definitivo per la pubblicazione.
