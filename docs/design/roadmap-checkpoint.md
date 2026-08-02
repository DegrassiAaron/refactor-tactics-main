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
| M2 Turn loop | ✅ | Fasi, resolver movimento (conflitti), pianificazione, timer 30s, range 4 |
| M3 Combat loop | ✅* | Danno/scudo, attacco, eliminazione, energia+ultimate (AoE), LOS/copertura, **abilità data-driven** ✅ · status Root/Slow, forme-area 🟡 (estensioni: Shield/Reveal, Line/Cone, barra abilità) |
| M4 Vertical slice | ✅ | Bot, HUD (barre HP + combat log), vittoria + riavvio |
| M5 Release interna | 🟡 | **33 test** ✅ · **packaging Windows** (Development) ✅ · DoD MVP formale ⏳ · Shipping ⏳ |

**Sviluppo in corso sul branch `feature/m1-sandbox`** (M1→M4 in un unico branch, non uno per milestone come da regola: scelta pratica di questa fase iniziale). **27 test automatici verdi.**

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

---

## M3 — Combat loop

| CP | Stato | Obiettivo | Note |
|---|---|---|---|
| 3.1 | ✅ | Abilità data-driven | `URTAbilityData` (range/power/area/status/cooldown/costo); ogni unità ha una **lista di abilità** (default: Attacco, Colpo pesante cd2, Ultimate); **barra abilità** nell'HUD, selezione con tasti 1/2/3, `IsAbilityUsable` (test); il bot sceglie l'abilità migliore |
| 3.2 | ✅ | Danno/scudo/morte | `URTCombatLibrary::ApplyDamage` + `URTCombatResolver::ResolveAttacks` (raccogli-poi-applica, focus-fire, ordine-indipendente) · eliminazione a HP 0 (test) |
| 3.3 | ✅ | Energia + ultimate | `GainEnergy`/`IsUltimateReady` (test) · energia per turno + on-hit; attacco a energia piena = ultimate (danno x2); barra energia nell'HUD |
| 3.4 | 🟡 | Status + Gameplay Tags | Tag nativi `Status.Root`/`Status.Slow`; durata a turni; `EffectiveMoveRange` (test); l'ultimate applica Slow; marker HUD · Shield/Reveal ⏳ |
| 3.5 | 🟡 | Targeting a forme | `CellsInRadius` (test) → **area/cerchio** usata dall'ultimate (AoE attorno al bersaglio); attacco base singolo · Line/Cone ⏳ |
| 3.6 | ✅ | LOS / copertura | `HasLineOfSight` (test) · ostacoli centrali visibili (`ARTGridActor::BlockedCells`); un attacco richiede LOS libera; movimento su copertura rifiutato |

**Uscita M3**: 🟡 combattimento base completo (attacco, danno/scudo, eliminazione). Le feature avanzate
(abilità data-driven, energia, status, forme, LOS/copertura) restano da fare.

> ⚠️ Bug dei tutorial: il "range movimento 4" **è stato chiuso** (validato). LOS non ancora introdotta.

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
| 5.1 | ✅ | Suite test verde | **33 test** da CLI (`Automation RunTests RefactorTactics`) tutti verdi |
| 5.2 | ✅ | Packaging Windows | `RunUAT BuildCookRun` (Development) → `Saved/Packaged/Windows/RefactorTactics.exe` **verificato: si avvia e si gioca senza editor**. Shipping ⏳ |
| 5.3 | ⏳ | Definition of Done MVP | Da rivedere voce per voce col piano canonico §4 |

> ⚠️ Nota packaging: il primo tentativo falliva per la cache `ScriptModules` corrotta della toolchain UAT
> (post-hotfix 5.8.0→5.8.1). Risolto con **Epic Launcher → UE 5.8 → Verifica**. NON eliminare a mano
> l'intera cartella `Engine/Intermediate/ScriptModules` (peggiora: "Found no script module records").

---

## Dopo l'MVP (north-star)

Vedi [piano canonico §8](piano-canonico-mvp.md#8-north-star-post-mvp-dai-prd): P0 multiplayer
server-authoritative → P1 4v4/eroi/replay/**Intenti condivisi** → P2 GAS/accessibilità/mappa
multilivello → P3 console/modding/anti-cheat.
