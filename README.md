# RefactorTactics

Gioco tattico PvP a **turni simultanei** ispirato ad *Atlas Reactor*, sviluppato come
**percorso didattico per imparare Unreal Engine 5.8** (partendo da un background C#).

Ogni turno: **pianificazione simultanea** delle mosse → risoluzione a fasi
**Prep → Dash → Blast → Move**, calcolate simultaneamente e applicate in ordine deterministico.

> **Stato attuale**: **MVP giocabile in editor**. Un 2v2 offline contro bot con pianificazione a turni,
> movimento con conflitti, combattimento (danno/scudo/eliminazioni), bot, HUD e condizione di vittoria —
> **27 test automatici verdi**. Restano da fare le feature di combat avanzate (abilità data-driven,
> energia, status, targeting a forme, LOS) e il packaging Windows (bloccato da un problema della toolchain
> dell'engine, non del codice). Dettaglio nella [roadmap/checkpoint](docs/design/roadmap-checkpoint.md).

---

## Obiettivo (MVP)

Uno *vertical slice* giocabile: **2v2 offline contro bot**, con pianificazione a turni,
risoluzione deterministica a fasi, abilità/scudi/energia, targeting a forme, bot, HUD e build
Windows. Il multiplayer, il 4v4 e il resto della visione competitiva sono
[roadmap post-MVP](docs/design/piano-canonico-mvp.md#8-north-star-post-mvp-dai-prd).

## Stack tecnico

| | |
|---|---|
| Motore | Unreal Engine **5.8.1** |
| Linguaggi | **C++** (regole, dati, resolver, test) + **Blueprint** (UI, VFX, camera, input) |
| Ability system | Data-driven (`UPrimaryDataAsset`) — **GAS rimandato** al post-MVP |
| Versionamento | Git + **Git LFS** |
| IDE | Visual Studio 2022 (workload *Game development with C++*) |
| Piattaforma MVP | Windows |

## Struttura del repository

```
RefactorTactics.uproject   # descrittore progetto Unreal
Source/RefactorTactics/    # modulo C++ (Core, Grid, Unit, Turn, Combat, Bot, Camera, Player, UI, Tests)
Config/                    # configurazione UE (Engine/Game/Input)
Content/                   # asset UE (M_Unit, L_Prototype; tracciati da Git LFS)
docs/                      # documentazione
  ├─ design/
  │   ├─ piano-canonico-mvp.md   # ⭐ fonte di verità per l'MVP
  │   ├─ roadmap-checkpoint.md   # milestone, checkpoint e stato
  │   └─ architettura-codice.md  # mappa delle classi C++
  ├─ guides/
  │   └─ debug-vs-unreal.md      # debug con Visual Studio + Unreal
  ├─ 00-Intro.pdf                # brief iniziale
  ├─ 01-StrutturaTutorial.pdf    # curriculum didattico
  ├─ 02-Tutorial.pdf             # corso: MVP multiplayer 1v1
  ├─ 03-TutorialToMVP.pdf        # corso: MVP offline 2v2
  ├─ RefactorTactics — Product Requirements Document*.pdf   # PRD (visione)
  └─ Product Requirements Document — Intenti condivisi.pdf  # PRD di feature (visione)
CLAUDE.md                  # guida per l'assistente Claude Code
```

> ⚠️ I PDF in `docs/` sono documenti di partenza **contraddittori tra loro**. Le decisioni
> effettive sono state riconciliate in [`docs/design/piano-canonico-mvp.md`](docs/design/piano-canonico-mvp.md),
> che ha la precedenza.

## Roadmap MVP

| Milestone | Contenuto | Stato |
|---|---|---|
| M0 Fondamenta | Toolchain, progetto C++, Git LFS | ✅ |
| M1 Sandbox | Camera tattica, input, selezione, griglia | ✅ |
| M2 Turn loop | Pianificazione, fasi, timer, risoluzione movimento | ✅ |
| M3 Combat loop | Danno/scudo, attacco, eliminazione ✅ · abilità/energia/status/forme/LOS ⏳ | 🟡 |
| M4 Vertical slice | Bot, HUD, condizione di vittoria | ✅ |
| M5 Release interna | Test verdi ✅ · packaging Windows ⏳ | 🟡 |

Dettaglio e stato per checkpoint nella [roadmap/checkpoint](docs/design/roadmap-checkpoint.md).

## Come compilare ed eseguire

1. Installare **Unreal Engine 5.8.1** (Epic Games Launcher) e **Visual Studio 2022** (workload
   *Game development with C++*). Installare **Git LFS** (`git lfs install`) e clonare il repo.
2. Aprire **`RefactorTactics.uproject`** (o generare i file di soluzione: tasto destro sul `.uproject`
   → *Generate Visual Studio project files*). Compilare il target **Development Editor**.
3. Aprire il livello **`Content/Maps/L_Prototype`** e premere **Play**: parte un 2v2 contro il bot.
   - **WASD** pan camera · **rotellina** zoom · **click** su unità = selezione · **click** su cella = movimento ·
     **click** su nemico = attacco · **Spazio** = risolvi il turno (o attendi il timer di 30s).
4. Test: **Tools → Session Frontend → Automation** → `RefactorTactics` → *Start Tests* (27 verdi).
   Guida al debug in [`docs/guides/debug-vs-unreal.md`](docs/guides/debug-vs-unreal.md).

## Note

- Ispirazione ad *Atlas Reactor* solo a livello di **meccaniche**. Per una pubblicazione servono
  nomi, personaggi e asset **originali**.
- Documentazione e commenti del progetto sono in **italiano**; il codice in inglese.

## Licenza

Non ancora definita.
