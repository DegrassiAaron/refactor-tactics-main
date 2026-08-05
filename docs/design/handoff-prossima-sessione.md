# Handoff — riprendere il lavoro da sessione fresca

> Contesto compatto per una **nuova sessione** che continua a implementare su RefactorTactics.
> Aggiornato: **2026-08-05**. Leggi anche [`piano-canonico-mvp.md`](piano-canonico-mvp.md) (decisioni),
> [`roadmap-checkpoint.md`](roadmap-checkpoint.md) (milestone e DoD) e la memoria di progetto `ue58-build-gotchas`.

## Dove siamo

- **Fase tutorial chiusa** (2026-08-05): il progetto è di prodotto, milestone da **M6** in poi.
- Tutto il lavoro dei filoni paralleli (skeletal units, bot utility, hex, editor mappa) è **mergiato su `main`**
  via PR #1–#10. Non esistono più branch lunghi non pushati né worktree obbligatori.
- **Fondamenta esagonali complete e testate** (H0–H6.5): coordinate, asset mappa, A\*, multilivello, editor
  mode, e lo strato di simulazione puro (`URTHexSimLibrary`, `URTHexPathLibrary`, `URTHexVisionLibrary`,
  `URTHexBotLibrary`).
- **Ma nessuna partita gira su hex**: il turn loop giocabile è ancora quello quadrato. È esattamente il
  contenuto di **M6 — Parità hex**.

## Stato git

- Branch di lavoro correnti creati da `main`, PR verso `main`; branch locale eliminato dopo il merge.
- Al 2026-08-05: `feat/camera-pitch-tunable` aperto come **PR #11** (camera: pitch regolabile, tasto `F`).
- Non committare la modifica automatica di `RefactorTactics.uproject` (l'editor ci scrive il GUID
  dell'engine locale): `git checkout -- RefactorTactics.uproject`.

## Come compilare e testare

```
# build (Editor, Development)
"D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development ^
  -project="D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" -waitmutex

# test headless (conta Result={Success} / Result={Fail} nel log)
"D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" ^
  "D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" ^
  "-ExecCmds=Automation RunTests RefactorTactics; Quit" -nullrhi -unattended -nopause -nosplash -log -abslog="<log>"
```

- L'editor aperto dall'utente **blocca** la build del target Editor (Live Coding): chiedere di chiuderlo, oppure
  lavorare in un **worktree a path corto** (es. `D:\rt-wt-XYZ`; **mai** nello scratchpad `C:\Users\...\Temp\...`,
  il path supera 260 caratteri e la build fallisce con `Filename too long`).
- **Suite**: 172 test (`Source/RefactorTactics/Tests/`, 25 file), 63 esagonali.

## Metodo

- **TDD** per la logica pura in `URT*Library` (RED→GREEN, testabile in automation).
- Wiring/Actor/editor: build verde + voce in [`test-manuali-pie.md`](test-manuali-pie.md), verificata
  dall'utente in PIE. Non dichiarare verde una voce PIE senza esecuzione reale.
- Niente commit/push senza richiesta esplicita.

## Primo passo consigliato

Aprire **M6 — Parità hex** dal checkpoint **6.1** (allestimento della partita su mappa hex e posizione
autorevole di `ARTUnit` in `FRTCellId`). Prima di scrivere: leggere `roadmap-checkpoint.md` § M6, che elenca
i CP con DoD e le verifiche `PIE-HEXPLAY` associate. La sostituzione della coordinata tocca ~34 file: va fatta
a fette compilabili, una PR per checkpoint.
