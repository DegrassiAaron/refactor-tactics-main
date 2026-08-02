# RefactorTactics — Roadmap & Checkpoint

> Espande le milestone del [piano canonico §7](piano-canonico-mvp.md#7-roadmap-mvp-14-settimane) in
> **checkpoint** con Definition of Done (DoD) **misurabile** e metodo di verifica.
> Regola: un checkpoint è "fatto" solo quando il DoD è verificato col metodo indicato — non "sembra funzionare".

## Come tracciamo il lavoro

- **1 milestone = 1 branch**, PR verso `main` a milestone completa.
- **1 checkpoint = ≥1 commit** con messaggio significativo; si committa quando il DoD del checkpoint è verde.
- Test automatici prima di chiudere un checkpoint che tocca le regole (mai saltarli).

## Stato attuale (2026-08-02)

Legenda: ✅ fatto e verificato · 🟡 fatto in forma ridotta (vedi nota) · ⏳ da fare

| Milestone | Stato | Sintesi |
|---|---|---|
| M0 Fondamenta | ✅ | Progetto UE 5.8.1 compila (Game + Editor), repo + LFS |
| M1 Sandbox | ✅ | Camera, input (C++), griglia + test, selezione, demo 2v2 |
| M2 Turn loop | ✅ | Fasi, resolver movimento (conflitti), pianificazione, timer 30s · **incrementi post-MVP**: path finding PF.1–PF.3, terreno v1, movimento v2 (→ sezione dedicata) |
| M3 Combat loop | ✅ | Danno/scudo, attacco, eliminazione, energia+ultimate (AoE), LOS/copertura, **abilità data-driven** ✅ · status Root/Slow/**Reveal** (intento nemico, invariante #6) ✅ · **forme targeting complete** (Single/Area/Line/Cone) ✅ · barra abilità |
| M4 Vertical slice | ✅ | Bot (focus-fire, **aggiramento ostacoli**, **kiting** del Ranger), HUD (barre HP + combat log + **anteprima piani** ciano/reveal), vittoria + riavvio |
| M5 Release interna | ✅ | **50 test** ✅ · **packaging Windows** (Development + **Shipping**) ✅ · DoD MVP formale ✅ |

**Sviluppo consolidato su `main`**: l'MVP (M0–M5) è stato sviluppato in un unico branch di fase e poi mergiato; gli incrementi post-MVP usano feature branch dedicati. **50 test automatici verdi** (7 file in `Source/RefactorTactics/Tests/`; conteggio autorevole del repo).

Scelte che divergono dai DoD originali (equivalenti o migliori, documentate qui):
- **Input in C++** (nessun asset `IA_*`/`IMC_*`): il controller costruisce Enhanced Input via codice.
- **Selezione via `IRTSelectable`** (interfaccia C++), non `BPI_Selectable` (Blueprint).
- **Colore team** via materiale `M_Unit` (unico asset creato a mano) + parametro `Color`.
- **Attacco base** su `ARTUnit` (AttackRange/AttackPower) invece di `URTAbilityData` data-driven (rimandato a M3 avanzato).

---

## M0 — Fondamenta

| CP | Stato | Obiettivo | Definition of Done | Verifica |
|---|---|---|---|---|
| 0.1 | ✅ | Toolchain | UE **5.8.1** + VS 2022 + **Git LFS** | Editor 5.8.1 si avvia; LFS attivo |
| 0.2 | ✅ | Progetto Blank C++ | `RefactorTactics.uproject` apre e il modulo C++ **compila** | Build Game + Editor Development → Succeeded |
| 0.3 | ✅ | Repo versionato | `.gitignore`/`.gitattributes`; `.uasset` gestiti da LFS | `git lfs ls-files` elenca `M_Unit`/`L_Prototype` |

---

## M1 — Sandbox

| CP | Stato | Obiettivo | Note |
|---|---|---|---|
| 1.1 | ✅ | Camera tattica | `ARTCameraPawn` (SpringArm, pitch -55°, pan/zoom) |
| 1.2 | ✅ | Enhanced Input | Costruito **in C++** (pan WASD, zoom rotellina, select click, lock-in Spazio) — nessun asset |
| 1.3 | ✅ | Griglia logica | `URTGridLibrary` + `FRTGridCoord` (10×10 @ 200) · test roundtrip verde |
| 1.4 | 🟡 | Griglia visuale | `ARTGridActor` (Instanced Static Mesh) ✅ · evidenziazione cella-sotto-cursore ⏳ |
| 1.5 | ✅ | Selezione | `IRTSelectable` + click (ingrandimento, deseleziona il precedente) |

**Uscita M1**: ✅ si naviga l'arena e si seleziona un'unità (verificato in PIE).

---

## M2 — Turn loop

| CP | Stato | Obiettivo | Note |
|---|---|---|---|
| 2.1 | ✅ | State machine fasi | `ERTMatchPhase` + `URTTurnRules::NextPhase` (test) |
| 2.2 | ✅ | Timer + lock-in | Timer **30s** + lock-in manuale (Spazio) / automatico |
| 2.3 | ✅ | Pianificazione azioni | `PlannedCell` / `PlannedAttackTarget` per unità (click su cella/nemico) |
| 2.4 | ✅ | Risoluzione movimento | `URTMovementResolver`: contesa/scambio/blocco · ordine-indipendente (test) |
| — | ✅ | Range movimento | Validazione max 4 celle (`IsWithinRange`) — chiude un bug dei tutorial |

**Uscita M2**: ✅ turno completo con movimento simultaneo deterministico (verificato in PIE).

> **Incrementi post-MVP innestati su M2** (path finding PF.1–PF.3, terreno v1, movimento v2) → consolidati nella
> sezione **«Incrementi post-MVP consegnati»** in fondo al documento.

---

## M3 — Combat loop

| CP | Stato | Obiettivo | Note |
|---|---|---|---|
| 3.1 | ✅ | Abilità data-driven | `URTAbilityData` (range/power/area/status/cooldown/costo); ogni unità ha una **lista di abilità** (default: Attacco, Colpo pesante cd2, Ultimate); **barra abilità** nell'HUD, selezione con tasti 1/2/3, `IsAbilityUsable` (test); il bot sceglie l'abilità migliore |
| 3.2 | ✅ | Danno/scudo/morte | `URTCombatLibrary::ApplyDamage` + `URTCombatResolver::ResolveAttacks` (raccogli-poi-applica, focus-fire, ordine-indipendente) · eliminazione a HP 0 (test) |
| 3.3 | ✅ | Energia + ultimate | `GainEnergy`/`IsUltimateReady` (test) · energia per turno + on-hit; attacco a energia piena = ultimate (danno x2); barra energia nell'HUD |
| 3.4 | ✅ | Status + Gameplay Tags | Tag nativi `Status.Root`/`Status.Slow`/`Status.Reveal`; durata a turni; `EffectiveMoveRange` (test); l'ultimate applica Slow, "Colpo preciso" applica Reveal; `IsIntentVisibleTo` (test, invariante #6); marker HUD + intento nemico rivelato · Shield = "Barriera" (già presente) |
| 3.5 | ✅ | Targeting a forme | `CellsInRadius`/`CellsInLine`/`CellsInCone` (test) → forme **Single · Area · Line · Cone** guidate da `ERTAbilityShape`; Ranger "Colpo preciso" (Line), Guardian "Spazzata" (Cone), ultimate AoE (Area) |
| 3.6 | ✅ | LOS / copertura | `HasLineOfSight` (test) · ostacoli centrali visibili (`ARTGridActor::BlockedCells`); un attacco richiede LOS libera; movimento su copertura rifiutato |

**Uscita M3**: ✅ combattimento completo — abilità data-driven, danno/scudo/eliminazione, energia/ultimate,
status (Root/Slow/Reveal), forme (Single/Area/Line/Cone), LOS/copertura. Tutti i CP 3.1–3.6 verdi (verificato in PIE).

> ⚠️ Bug dei tutorial: il "range movimento 4" **è stato chiuso** (validato); **LOS/copertura** ora implementata (CP 3.6).

---

## M4 — Vertical slice

| CP | Stato | Obiettivo | Note |
|---|---|---|---|
| 4.1 | ✅ | Bot | `URTBotLibrary::StepToward` + `PlanBots`: team 1 si avvicina e attacca (test) |
| 4.2 | ✅ | HUD | Barre HP/scudo sopra le unità (`ARTHUD`, C++) + **combat log a schermo** (ultimi eventi in basso a sinistra) |
| 4.3 | ✅ | Vittoria | Squadra eliminata → `MatchEnded`, turni fermi (test) + **riavvio con tasto R** ("premi R per rigiocare") |

**Uscita M4**: ✅ **completo** — MVP giocabile 2v2 contro bot, dall'avvio alla vittoria e ripetibile (verificato in PIE).

---

## M5 — Release interna

| CP | Stato | Obiettivo | Note |
|---|---|---|---|
| 5.1 | ✅ | Suite test verde | **50 test** da CLI (`Automation RunTests RefactorTactics`) tutti verdi — conteggio autorevole del repo |
| 5.2 | ✅ | Packaging Windows | `RunUAT BuildCookRun` — **Development** (`Saved/Packaged/Windows/`, verificato: si avvia e si gioca senza editor) **e Shipping** (`Saved/StagedBuilds/Windows/RefactorTactics-Win64-Shipping.exe`, ~166 MB, BUILD SUCCESSFUL) |
| 5.3 | ✅ | Definition of Done MVP | Rivista voce per voce col piano canonico §4 (vedi riga di sintesi) |

> ⚠️ Nota packaging: il primo tentativo falliva per la cache `ScriptModules` corrotta della toolchain UAT
> (post-hotfix 5.8.0→5.8.1). Risolto con **Epic Launcher → UE 5.8 → Verifica**. NON eliminare a mano
> l'intera cartella `Engine/Intermediate/ScriptModules` (peggiora: "Found no script module records").

---

## Incrementi post-MVP consegnati (2026-08-02)

Oltre all'MVP (M0–M5) sono stati consegnati incrementi **post-MVP**, tutti verificati in PIE e coperti dalla
suite (50 test). Le voci ⏳ restano north-star.

| Incremento | Stato | Sintesi | Spec |
|---|---|---|---|
| **PF.1–PF.2** · path finding obstacle-aware | ✅ | `ReachableCells` (BFS) + validazione nel resolver (chiude "movimento attraverso le colonne"); `FindPath` + preview a schermo; bot path-aware | [`spec-pathfinding.md`](spec-pathfinding.md) |
| **PF.3** · path finding pesato | ✅ | Dijkstra su costo per cella (cost provider); percorso a costo minimo | [`spec-pathfinding-pf3-pf4.md`](spec-pathfinding-pf3-pf4.md) |
| **PF.4** · grafo multilivello | 🟡 | **motore ✅** (`FRTGridCoord`+`Layer`, `FRTTraversalEdge`, `ReachableCellsByGraph`/`FindPathByGraph` — TDD). **Mappa "ponte sopraelevato"**: MP.1 ponte + movimento cross-layer (rampe) + click→layer + elevazione unità ✅ *(codice completo, non PIE-verificato dall'utente)* · MP.2 **LOS di elevazione** ✅ TDD · MP.3 preview HUD elevata ✅ · MP.4 tuning/verifica PIE ⏳ · bot-sul-ponte ⏳ | [`spec-mappa-multilivello.md`](spec-mappa-multilivello.md) |
| **Terreno v1** | ✅ | `URTTerrainData` (5 tipi): Fango (costo), Cespuglio (blocca vista), Altura (+danno), Lava (hazard fine turno), Erba secca → Fuoco (dinamico); rendering celle colorate; bot cost/hazard-aware | [`spec-terreni.md`](spec-terreni.md) |
| **Movimento v2** | ✅ | `URTMovementResolver::ResolvePaths` (microstep sincroni, ordine-indipendente); path composita a waypoint (**pallini visibili** · click aggiunge · **tasto destro** toglie · editing per-unità · rifiuto oltre budget); cross-damage + double-dip con l'hazard; **traccia grigia** del percorso risolto post-lock | [`spec-terreni.md`](spec-terreni.md) §7/§10 |
| **Consolidamento «Sequenza di Risoluzione del Turno»** | ✅ *(doc)* | spec-panel: classificazione north-star + recepimento `FR-RESOLVE-01..03` (ordinamento deterministico APNAP) nel canone §5.1 | [`spec-sequenza-turno.md`](spec-sequenza-turno.md) |

**Polish**: viz del percorso *risolto* post-lock (traccia grigia) + pallini sui waypoint + undo col tasto destro ✅ (2026-08-02).

---

## Dopo l'MVP (north-star)

Vedi [piano canonico §8](piano-canonico-mvp.md#8-north-star-post-mvp-dai-prd): P0 multiplayer
server-authoritative → P1 4v4/eroi/replay/**Intenti condivisi** → P2 GAS/accessibilità/mappa
multilivello → P3 console/modding/anti-cheat.

> **Sequenza di risoluzione ricca** (reazioni/reveal/stack LIFO) — north-star, vedi
> [`spec-sequenza-turno.md`](spec-sequenza-turno.md). L'unica parte adottata a breve è l'**ordinamento
> deterministico degli effetti simultanei** (APNAP + tie-break totale), **recepita nel piano canonico §5.1**
> (`FR-RESOLVE-01..03`, 2026-08-02). Il resto (finestre live, categorie di velocità, modello JSON) resta
> post-MVP per conflitto con gli invarianti #3/#4.
