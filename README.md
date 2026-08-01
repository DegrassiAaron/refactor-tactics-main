# RefactorTactics

Gioco tattico PvP a **turni simultanei** ispirato ad *Atlas Reactor*, sviluppato come
**percorso didattico per imparare Unreal Engine 5.8** (partendo da un background C#).

Ogni turno: **pianificazione simultanea** delle mosse → risoluzione a fasi
**Prep → Dash → Blast → Move**, calcolate simultaneamente e applicate in ordine deterministico.

> **Stato attuale**: fase di *design & documentazione*. Il progetto Unreal Engine non è ancora
> stato creato: si costruirà seguendo il [piano canonico MVP](docs/design/piano-canonico-mvp.md).

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
RefactorTactics.uproject   # (da creare) progetto Unreal
Source/                    # (da creare) codice C++
Content/                   # (da creare) asset UE
Config/                    # (da creare) configurazione UE
docs/                      # documentazione
  ├─ design/
  │   └─ piano-canonico-mvp.md   # ⭐ fonte di verità per l'MVP
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

## Roadmap MVP (~14 settimane)

| Milestone | Contenuto |
|---|---|
| M0 Fondamenta | Toolchain, progetto C++, Git LFS, classi base |
| M1 Sandbox | Camera tattica, input, selezione, griglia |
| M2 Turn loop | Pianificazione, fasi, timer, risoluzione movimento |
| M3 Combat loop | Abilità, danni, scudi, energia, status, targeting, LOS |
| M4 Vertical slice | Bot, HUD, combat log, condizione di vittoria |
| M5 Release interna | Test, packaging Windows, Definition of Done |

Dettaglio completo nel [piano canonico](docs/design/piano-canonico-mvp.md).

## Come iniziare (quando si crea il progetto UE)

1. Installare **Unreal Engine 5.8.1** (Epic Games Launcher) e **Visual Studio 2022** con il
   workload *Game development with C++* + *Visual Studio Tools for Unreal Engine*.
2. Installare **Git LFS**: `git lfs install`.
3. Creare il progetto **Blank C++** `RefactorTactics` nella radice del repo (Ray Tracing off,
   Starter Content on) e seguire il piano canonico, milestone per milestone.

## Note

- Ispirazione ad *Atlas Reactor* solo a livello di **meccaniche**. Per una pubblicazione servono
  nomi, personaggi e asset **originali**.
- Documentazione e commenti del progetto sono in **italiano**; il codice in inglese.

## Licenza

Non ancora definita.
