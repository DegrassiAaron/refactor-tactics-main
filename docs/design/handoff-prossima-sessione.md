# Handoff — riprendere l'implementazione da sessione fresca

> Contesto compatto per una **nuova sessione** che continua a implementare codice su RefactorTactics.
> Aggiornato: 2026-08-04. Leggi anche `piano-canonico-mvp.md`, `roadmap-checkpoint.md` e la memoria di progetto
> `ue58-build-gotchas`.

## ⚠️ Situazione multi-branch (leggere PRIMA di toccare git)
Tre filoni **paralleli** sulla stessa macchina; il working tree principale (`D:\Repositories\refactor-tactics-main`)
è su **`feat/hex-grid`** (lavoro dell'utente sulla griglia esagonale — **non toccare** senza coordinarsi).

| Branch | Contenuto | Stato |
|---|---|---|
| `feat/hex-grid` | griglia esagonale + editor mappa (utente, parallelo) | attivo — non toccare |
| `feat/skeletal-units` | personaggi Paragon animati (AS.1–AS.5), camera, anim | C++ fatto; manca editor (montaggi/materiali) + PIE |
| `feat/bot-utility` (da skeletal-units) | bot utility scoring (BU.1–BU.3) | C++ fatto; manca PIE → tuning → refactor completo |

Tutti i branch sono **locali (non pushati)**.

## ⚠️ Regole operative (fondamentali)
- **Non lavorare sul working tree principale** (è su hex, dell'utente). Per skeletal-units/bot-utility usa un
  **worktree isolato in PATH CORTO** (es. `D:\rt-wt-bot`). **Mai nello scratchpad** (`C:\Users\...\Temp\...`):
  il path supera 260 char e la build UE fallisce (`Filename too long`).
  - Creare: `GIT_LFS_SKIP_SMUDGE=1 git worktree add "D:\rt-wt-XYZ" <branch>` (LFS skip = veloce; i binari non
    servono per compilare C++). Un worktree `D:\rt-wt-bot` (feat/bot-utility) **esiste già**.
  - Un hook di sicurezza protegge `D:\rt-wt-*` dalla **rimozione** automatica: se serve, chiedere all'utente.
- **Build** (dal worktree, path corto): `"D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development -project="D:\rt-wt-bot\RefactorTactics.uproject" -waitmutex`. La GUI dell'editor dell'utente (su hex) **non** blocca questa build (dir/DLL separate).
- **Test headless**: `"D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\rt-wt-bot\RefactorTactics.uproject" "-ExecCmds=Automation RunTests RefactorTactics; Quit" -nullrhi -unattended -nopause -nosplash -log -abslog="<log>"`. Conta `Result={Success}` / `Result={Fail}` nel log.
- **TDD**: logica pura in `URT*Library` (testabile in automation, RED→GREEN); presentazione/wiring (Actor/World/editor) → verifica build + **PIE** (vedi `test-manuali-pie.md`).
- Niente commit/push senza richiesta esplicita. Commit trailer: `Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>`.

## Cosa è fatto / cosa manca

### `feat/bot-utility` (il filone di questa sessione)
- ✅ **BU.1** `URTBotLibrary::ScorePlan` puro + `FRTBotPlan`/`FRTBotContext` (5 test).
- ✅ **BU.2** `PlanBots` sceglie la **cella di posizionamento** via `ScorePlan` (resta/tiro/avvicinamento), pesando minaccia/kiting.
- ✅ **BU.3 (debug)** punteggio nel combat log (`utility -> (x,y,Lz) score=N`).
- ⏳ **PIE del bot** (`PIE-BU2` in `test-manuali-pie.md`) → **tuning pesi** (esporre `WKill/WThreat/WKiteViolation/WApproach` come `UPROPERTY` sul `TurnManager`) → **refactor BU.3 completo** (candidate **attacco+movimento** combinate su tutti i rami, mantenendo le guardie support/dash/panic). Suite corrente: **77/77**.

### `feat/skeletal-units` (personaggi)
- ✅ C++: `VisualZOffset`, spawn `TSubclassOf` con fallback cilindro, facing (`DirectionYaw`+`bFaceMovementDirection`), `bIsMovingVisually` (anim corsa), camera (`DefaultArmLength`+ricentro Home), AS.4b eventi montaggio (`PlayAttackMontage/PlayHitMontage/PlayDefeatMontage`), AS.5 `TeamRing`+`TeamColorFor`.
- ⏳ Editor (utente): `BP_Unit_*`, `ABP_*`, montaggi AS.4b, `M_TeamRing`. Guida: `guida-animazioni-paragon.md`. PIE: `test-manuali-pie.md`.

## Primo passo consigliato per la sessione fresca
Se l'utente ha già fatto il **PIE del bot**: procedere con **tuning pesi** e poi **refactor BU.3** su `feat/bot-utility`
(worktree `D:\rt-wt-bot`). Altrimenti chiedere l'esito del PIE. Spec di riferimento: `spec-bot-utility.md`.
