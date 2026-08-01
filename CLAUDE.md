# CLAUDE.md — RefactorTactics

Guida per Claude Code quando lavora in questo repository.

## Cos'è il progetto

**RefactorTactics** è un gioco tattico PvP a **turni simultanei** (ispirato ad *Atlas Reactor*),
sviluppato come **percorso didattico per imparare Unreal Engine 5.8** partendo da un profilo C#.
Il loop di turno è: **pianificazione simultanea** → risoluzione a fasi **Prep → Dash → Blast → Move**
(calcolate simultaneamente, applicate in ordine deterministico).

> ⚠️ Il progetto UE **non esiste ancora**: al momento il repo contiene solo la documentazione
> (`docs/`). Il codice si costruirà seguendo il piano canonico.

## Fonte di verità

1. **`docs/design/piano-canonico-mvp.md`** — decisioni operative vincolanti. In caso di conflitto,
   **prevale su qualsiasi PDF**.
2. I PDF in `docs/` sono materiale di partenza. Sono **contraddittori tra loro** (nome, formato,
   griglia, prefissi classi, ecc.): non trattarli come specifica unica.
3. I 3 PRD + `Intenti condivisi.pdf` sono la **visione a lungo termine (north-star)**, non lo scope
   attuale. Non implementare feature dei PRD (4v4, GAS, netcode avanzato, modding) nell'MVP.

## Decisioni tecniche fissate

- **Motore**: Unreal Engine **5.8.1** (bloccata; non aggiornare salvo bug bloccanti).
- **Linguaggi**: **regole/dati/resolver/test in C++**, **presentazione/UI/VFX/camera/input in Blueprint**.
- **C# escluso a runtime** (UnrealSharp/UnrealCLR sconsigliati).
- **No GAS** nell'MVP: sistema abilità leggero via `URTAbilityData : UPrimaryDataAsset`. GAS è post-MVP.
- **Nome / prefissi**: progetto `RefactorTactics`; classi con prefisso **`RT`/`URT`** (non `AT`/`UAT`).
- **Scope MVP**: **2v2 offline contro bot**. Il multiplayer è rimandato, ma l'architettura va scritta
  *server-authority-ready*.
- **VCS**: Git + **Git LFS** (asset binari UE tracciati via `.gitattributes`).
- Il progetto UE vive nella **radice del repo** (`RefactorTactics.uproject`, `Source/`, `Content/`, `Config/`).

## Invarianti architetturali (non negoziabili)

1. Le **regole decidono l'esito** (C++); animazioni/VFX non decidono nulla.
2. Posizione autorevole = **griglia logica** `FRTGridCoord`; `FVector` solo per il rendering.
3. Resolver **"raccogli poi applica"**: snapshot a inizio fase, niente `Delay`/timeline/montage nel
   resolver, l'ordine dell'array non deve cambiare l'esito.
4. **Privacy dell'intento**: nessuna mossa avversaria mostrata/replicata durante la pianificazione.
5. **Combat math = funzioni pure** testabili (`URTCombatLibrary`).

## Convenzioni

- **Documentazione**: sempre in `docs/` (sottocartella pertinente, es. `docs/design/`). Non creare
  file di documentazione nella radice del progetto.
- **Asset UE**: prefissi `BP_ WBP_ BPI_ DA_ DT_ IA_ IMC_ L_ M_ MI_ T_ NS_ S_` (vedi piano canonico §5).
- **Test**: Unreal Automation Framework (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`), eseguibili da Editor e CLI.
- **Non versionare**: `Binaries/ DerivedDataCache/ Intermediate/ Saved/ .vs/` (già in `.gitignore`).

## Lingua

Rispondi e commenta **in italiano**. Termini tecnici e identificatori di codice restano in inglese.

## Git

- Repository: `DegrassiAaron/refactor-tactics-main` (owner **DegrassiAaron**).
- Il push HTTPS richiede l'account gh `DegrassiAaron` attivo; vedi la memoria di progetto per il
  workaround al blocco di Git Credential Manager.
- Branch di feature per ogni lavoro; PR verso il branch padre (non sempre `main`).

## Come lavorare qui

- Prima di implementare, **rileggi `docs/design/piano-canonico-mvp.md`**.
- Costruisci per milestone (piano canonico §7): M0 Fondamenta → M1 Sandbox → M2 Turn loop →
  M3 Combat loop → M4 Vertical slice → M5 Release interna.
- Costruisci solo ciò che serve all'MVP; le feature north-star restano fuori scope finché l'MVP non è chiuso.
