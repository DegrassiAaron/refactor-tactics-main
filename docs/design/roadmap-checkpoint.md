# RefactorTactics — Roadmap & Checkpoint

> Espande le milestone del [piano canonico §7](piano-canonico-mvp.md#7-roadmap-mvp-14-settimane) in
> **checkpoint** con Definition of Done (DoD) **misurabile** e metodo di verifica, e **integra la roadmap
> tecnica dei PDR** (`docs/PDR/RT_PDR_10_Roadmap_QA_Rischi`) — vedi §«Allineamento con i PDR (F0–F6)».
> Regola: un checkpoint è "fatto" solo quando il DoD è verificato col metodo indicato — non "sembra funzionare".

## Come tracciamo il lavoro

- **1 milestone = 1 branch**, PR verso `main` a milestone completa.
- **1 checkpoint = ≥1 commit** con messaggio significativo; si committa quando il DoD del checkpoint è verde.
- Test automatici prima di chiudere un checkpoint che tocca le regole (mai saltarli).

## Stato attuale (2026-08-03)

Legenda: ✅ fatto e verificato · 🟡 fatto in forma ridotta (vedi nota) · ⏳ da fare

| Milestone | Stato | Sintesi |
|---|---|---|
| M0 Fondamenta | ✅ | Progetto UE 5.8.1 compila (Game + Editor), repo + LFS |
| M1 Sandbox | ✅ | Camera, input (C++), griglia + test, selezione, demo 2v2 |
| M2 Turn loop | ✅ | Fasi, resolver movimento (conflitti), pianificazione, timer 30s · **incrementi post-MVP**: path finding PF.1–PF.3, terreno v1, movimento v2 (→ sezione dedicata) |
| M3 Combat loop | ✅ | Danno/scudo, attacco, eliminazione, energia+ultimate (AoE), LOS/copertura, **abilità data-driven** ✅ · status Root/Slow/**Reveal** (intento nemico, invariante #6) ✅ · **forme targeting complete** (Single/Area/Line/Cone) ✅ · barra abilità |
| M4 Vertical slice | ✅ | Bot (focus-fire, **aggiramento ostacoli**, **kiting** del Ranger), HUD (barre HP + combat log + **anteprima piani** ciano/reveal), vittoria + riavvio |
| M5 Release interna | ✅ | **63 test** ✅ · **packaging Windows** (Development + **Shipping**) ✅ · DoD MVP formale ✅ |

**Sviluppo consolidato su `main`**: l'MVP (M0–M5) è stato sviluppato in un unico branch di fase e poi mergiato; gli incrementi post-MVP usano feature branch dedicati. **146 test automatici verdi** (`Source/RefactorTactics/Tests/`; conteggio autorevole del repo, misurato 2026-08-05; include TurnLog P3 + hash + serializzazione versionata + checksum + I/O su file + **simulazione esagonale H6.1–H6.3** con TurnLog e replay su hex).

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
| 5.1 | ✅ | Suite test verde | **63 test** da CLI (`Automation RunTests RefactorTactics`) tutti verdi — conteggio autorevole del repo (2026-08-03) |
| 5.2 | ✅ | Packaging Windows | `RunUAT BuildCookRun` — **Development** (`Saved/Packaged/Windows/`, verificato: si avvia e si gioca senza editor) **e Shipping** (`Saved/StagedBuilds/Windows/RefactorTactics-Win64-Shipping.exe`, ~166 MB, BUILD SUCCESSFUL) |
| 5.3 | ✅ | Definition of Done MVP | Rivista voce per voce col piano canonico §4 (vedi riga di sintesi) |

> ⚠️ Nota packaging: il primo tentativo falliva per la cache `ScriptModules` corrotta della toolchain UAT
> (post-hotfix 5.8.0→5.8.1). Risolto con **Epic Launcher → UE 5.8 → Verifica**. NON eliminare a mano
> l'intera cartella `Engine/Intermediate/ScriptModules` (peggiora: "Found no script module records").

---

## Incrementi post-MVP consegnati (aggiornato 2026-08-03)

Oltre all'MVP (M0–M5) sono stati consegnati incrementi **post-MVP**, tutti verificati in PIE (o via log) e
coperti dalla suite (**70 test**). Le voci ⏳ restano north-star / richiedono verifica interattiva dell'utente.

| Incremento | Stato | Sintesi | Spec |
|---|---|---|---|
| **PF.1–PF.2** · path finding obstacle-aware | ✅ | `ReachableCells` (BFS) + validazione nel resolver (chiude "movimento attraverso le colonne"); `FindPath` + preview a schermo; bot path-aware | [`spec-pathfinding.md`](spec-pathfinding.md) |
| **PF.3** · path finding pesato | ✅ | Dijkstra su costo per cella (cost provider); percorso a costo minimo | [`spec-pathfinding-pf3-pf4.md`](spec-pathfinding-pf3-pf4.md) |
| **PF.4** · grafo multilivello | 🟡 | **motore ✅** (`FRTGridCoord`+`Layer`, `FRTTraversalEdge`, `ReachableCellsByGraph`/`FindPathByGraph` — TDD). **Mappa "ponte sopraelevato"**: MP.1 ponte + movimento cross-layer (rampe) + click→layer + elevazione unità ✅ · MP.2 **LOS di elevazione** ✅ TDD · MP.3 preview HUD elevata ✅ · **MP.4 bot-sul-ponte** ✅ (`BestFiringCell`: il bot sale in quota per sparare oltre le coperture — TDD + osservato in gioco). ⏳ resta solo la verifica interattiva **click→layer** (serve il mouse dell'utente) + tuning colori/quota | [`spec-mappa-multilivello.md`](spec-mappa-multilivello.md) |
| **Terreno v1** | ✅ | `URTTerrainData` (5 tipi): Fango (costo), Cespuglio (blocca vista), Altura (+danno), Lava (hazard fine turno), Erba secca → Fuoco (dinamico); rendering celle colorate; bot cost/hazard-aware | [`spec-terreni.md`](spec-terreni.md) |
| **Movimento v2** | ✅ | `URTMovementResolver::ResolvePaths` (microstep sincroni, ordine-indipendente); path composita a waypoint (**pallini visibili** · click aggiunge · **tasto destro** toglie · editing per-unità · rifiuto oltre budget); cross-damage + double-dip con l'hazard; **traccia grigia** del percorso risolto post-lock | [`spec-terreni.md`](spec-terreni.md) §7/§10 |
| **Consolidamento «Sequenza di Risoluzione del Turno»** | ✅ *(doc)* | spec-panel: classificazione north-star + recepimento `FR-RESOLVE-01..03` (ordinamento deterministico APNAP) nel canone §5.1 | [`spec-sequenza-turno.md`](spec-sequenza-turno.md) |
| **Animazione della risoluzione (AN.1–AN.6)** | ✅ | Il round è **osservabile**: i cilindri si **muovono** animati per fase (Prep→Dash→Blast→Move), **senso di durata** configurabile (`UPROPERTY`), **skip** (Spazio), HUD fase/%, **morte visiva differita** (il colpo mortale si vede prima della sparizione, `NewlyDefeated`). Playback = presentazione, non decide (invariante #1); 60/60→ test invariati | [`spec-anima-risoluzione.md`](spec-anima-risoluzione.md) |
| **Dash come fase attiva** | ✅ | Fase **Dash** ora attiva: abilità di scatto (`bDash`) risolta **prima del Blast** (riposizionarsi/schivare prima degli attacchi). Bot (offensivo + **difensivo/schiva**) e **giocatore** (tasto 4 + click, preview magenta). **Dash + Move** consentiti. Status: Root blocca, Slow dimezza (`GetEffectiveDashRange`) | [`spec-dash.md`](spec-dash.md) |
| **Knockback (spinta)** | ✅ | Un attacco **respinge** i colpiti (fase Blast): fuori copertura, giù dal ponte, nella lava (cross-damage). `KnockbackDestination` (pura, 8 test, TDD); Guardian "Spazzata" spinge 2 celle; deterministico (2+ spinte si annullano; conteso→resta); **animato** (scivolamento nel Blast) | [`spec-knockback.md`](spec-knockback.md) |
| **TurnLog + reason codes (P3, TL.1–TL.3)** | ✅ | Osservabilità autoritativa separata dal playback: `FRTTurnLogEntry` con reason codes interi — Movimento `ERTMoveOutcome{Stayed,Moved,BlockedContested,BlockedByUnit}` esposto da `ResolvePaths`; Combat `ERTCombatOutcome{Hit,ShieldAbsorbed,Lethal,NoLineOfSight,TerrainBonus}` via `ClassifyCombatOutcome` (pura). Chiave = cella di partenza (**permutazione-invariante**); log ordinato deterministicamente; combat log HUD arricchito. TDD (3 test nuovi, 70/70 verdi). Reason allineati al codice reale (no CoverReduced: la copertura blocca la LOS). ⏳ hash di replay | [`spec-turnlog.md`](spec-turnlog.md) · [`plan-turnlog.md`](plan-turnlog.md) |

**Polish**: viz del percorso *risolto* post-lock (traccia grigia) + pallini sui waypoint + undo col tasto destro ✅ (2026-08-02); **animazione della risoluzione** (cilindri in movimento, morte differita, spinta) ✅ (2026-08-03).

---

## Allineamento con i PDR (roadmap tecnica F0–F6)

> Integra la **roadmap tecnica dei PDR** (`docs/PDR/RT_PDR_10_Roadmap_QA_Rischi`, owner della roadmap) nel
> tracciamento a checkpoint. **Gerarchia di prevalenza** (PDR-00 §governance): *decisioni esplicite del
> progetto > requisiti consolidati > **proposte PDR** > ricerca*. Dove il PDR diverge dalle decisioni MVP
> consolidate, **prevale il canone** e la divergenza è segnalata.
>
> ⚠️ **Governance PDR-00 §6**: l'owner della roadmap è **PDR-10**; i PDF sono *snapshot di consultazione*, le
> sorgenti testuali devono vivere in Git. **Fatto (2026-08-03)**: creata la sorgente canonica
> [`docs/PDR/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md`](../PDR/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md) e registrata la
> decisione **D-009** in [`docs/PDR/RT_PDR_00_Decision_Log.md`](../PDR/RT_PDR_00_Decision_Log.md). Questo tracker
> resta la vista di **esecuzione**; PDR-10 v0.2 è il **piano/requisiti** (una sola fonte logica per concetto).

### Mappatura milestone MVP (M) ↔ fasi PDR (F)

Le fasi PDR sono **ordinate per rischio** ("sequenziale per rischio, non un calendario"); gli M dell'MVP le
coprono in ordine diverso (**offline-first**).

| Fase PDR | Obiettivo | Exit gate PDR | Stato nel repo |
|---|---|---|---|
| **F0** Fondazioni | loop locale movimento deterministico | Golden tests + packaged demo | ✅ ≈ **M0–M2** (fasi, resolver ordine-indip., timer 30s, packaging) |
| **F1** Rete privata | listen server, preview team-only, commit, privacy test | **Zero canary leak** | ⏳ **differito** — ⚠️ il PDR lo anticipa (F1); l'MVP è 2v2 **offline**, architettura *server-authority-ready* |
| **F2** Abilities | 4×4 kit, **GAS mirror**, target/LOS | Golden test per ability | 🟡 abilità data-driven + targeting/LOS ✅ (**M3**, `URTAbilityData`); **GAS** ⏳ ⚠️ (canone: No-GAS nell'MVP); kit = 2 archetipi (non 4×4) |
| **F3** Mappa multilivello | layer, porte, ponte, tunnel, acqua/elettrico | Revision/cache/LOS tests | 🟡 **ponte + LOS di elevazione** ✅ (PF.4 / MP.1–MP.4); porte/tunnel/acqua/elettrico ⏳ |
| **F4** Vertical slice | 2v2, objective, UI completa, bot base | Playtest interno 20–30 min | 🟡 2v2 + UI + bot ✅ (**M4**) + risoluzione **animata**; **objective/modalità** ⏳ · playtest lungo ⏳ |
| **F5** Dedicated | server target, reconnect, telemetry, replay audit | Packaged soak test | ⏳ north-star |
| **F6** Beta systems | content pipeline, balance, accessibilità | Release checklist | ⏳ north-star |

**Divergenze** (prevale il canone MVP, PDR recepito come direzione north-star non come override): **(a)** rete
F1 anticipata dal PDR vs **differita** dall'MVP; **(b)** **GAS** F2 dal PDR vs **No-GAS** nell'MVP. Restano
decisioni consolidate — vedi [`piano-canonico-mvp.md`](piano-canonico-mvp.md).

### QA — Test pyramid (PDR-10 §5)

| Livello | Frequenza | Stato |
|---|---|---|
| Core automation (logica pura: resolver, combat, path, dash, knockback, turnlog) | ogni commit | ✅ **70 test** (CLI + Session Frontend) |
| Feature tests (comportamenti integrati) | PR/CI | 🟡 via PIE + log |
| Network tests (privacy/leak canary) | PR critiche/nightly | ⏳ (con F1) |
| Functional maps | nightly | ⏳ |
| Packaged tests | milestone/release | 🟡 build Shipping ok; soak ⏳ |
| Playtest (ogni incremento giocabile) | continuo | 🟡 PIE headless; playtest utente ⏳ |

### KPI / Performance budgets (PDR-10 §6)

| Budget | Target | Stato |
|---|---|---|
| Client FPS | **60** | ⏳ non misurato |
| Path (mediana) | **< 2 ms** | ⏳ non misurato |
| Preview completa | **< 50 ms** | ⏳ (offline: preview locale immediata) |
| Resolver server | **< 100 ms/match** | ⏳ non misurato (risoluzione sincrona rapida) |
| Intent updates | 8–12 Hz | ⏳ (con rete) |
| **Replay divergence** | **0** | ✅ determinismo by-design + test ordine-indip.; **TurnLog ✅** (permutazione-invariante); **hash di replay ✅** (`ff5e079`); **serializzazione versionata ✅** (`SR`, [`spec-turnlog-serialize.md`](spec-turnlog-serialize.md)); **anche su griglia esagonale ✅** (H6.3: `BuildMoveLog` + topologia dichiarata nel formato, test `RefactorTactics.HexSim.ReplayDivergenceZero`) |
| **Intent leak** | **0** | ⏳ canary (con F1) — privacy già invariante #6 |

### Risk register (PDR-10 §7) — con stato mitigazione

| Rischio | P/I | Mitigazione | Stato |
|---|---|---|---|
| Resolver difficile da spiegare | H/H | TurnLog reason codes + UI certainty | ✅ combat log a schermo ✅; **reason codes/TurnLog ✅** (Movimento+Combat, TDD); UI certainty ⏳ |
| Leak di planning | M/H | DTO team-only, canary, no global replication | 🟡 privacy #6 (offline); canary ⏳ |
| GAS invade l'autorità | M/H | resolver **puro prima** di GAS | ✅ resolver puro consolidato; GAS non introdotto |
| Mappa Actor-heavy | M/H | dati compatti + instancing/chunk | ✅ griglia/terreno/ponte via ISM |
| Scope roster/ambienti | H/M | slice a pochi personaggi, 1 combo primaria | 🟡 2 archetipi (Ranger/Guardian) |
| Upgrade UE in milestone | M/H | patch lock, upgrade tra milestone | ✅ UE **5.8.1 bloccata** (canone) |
| Modding prematuro | M/M | rimandato | ✅ fuori scope |

### Definition of Done (PDR-10 §8) — 7 criteri

1. Funziona **server/client** (non solo Standalone) — ⏳ con rete. 2. Non espone dati oltre la classificazione
(privacy) — 🟡 invariante #6 offline. 3. Log/debug spiegano l'esito — ✅ `LogRT` + combat log. 4. Include
Automation/Functional test pertinente — ✅ 63 test. 5. Rispetta i budget o registra la deviazione — ⏳ budget
non misurati. 6. Verificata in **build packaged** — 🟡 Shipping ok. 7. Documentazione, changelog, commit
focalizzato — ✅ `docs/design/`.

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
