# RefactorTactics — Roadmap & Checkpoint

> Espande le milestone del [piano canonico §7](piano-canonico-mvp.md#7-roadmap-mvp-14-settimane) in
> **checkpoint** con Definition of Done (DoD) **misurabile** e metodo di verifica.
> Regola: un checkpoint è "fatto" solo quando il DoD è verificato col metodo indicato — non "sembra funzionare".

## Come tracciamo il lavoro

- **1 milestone = 1 branch** (`feature/m0-fondamenta`, …), PR verso `main` a milestone completa.
- **1 checkpoint = ≥1 commit** con messaggio significativo; si committa quando il DoD del checkpoint è verde.
- **Tag** a fine milestone (`m0`, `m1`, …) come punto di ripristino.
- Test automatici prima di chiudere un checkpoint che tocca le regole (mai saltarli).

---

## M0 — Fondamenta  ·  *target sett. 2*

| CP | Obiettivo | Definition of Done (verificabile) | Verifica |
|---|---|---|---|
| 0.1 | Toolchain | UE **5.8.1** + VS 2022 (workload *Game development with C++*) + **Git LFS** installati | `git lfs version` risponde; l'editor UE 5.8.1 si avvia |
| 0.2 | Progetto Blank C++ | `RefactorTactics.uproject` apre nell'editor e il modulo C++ **compila** | Build in VS senza errori; l'editor apre il progetto |
| 0.3 | Repo versionato | Progetto committato; `.gitignore`/`.gitattributes` attivi; un `.uasset` è gestito da LFS | `git status` pulito; `git lfs ls-files` elenca l'asset |

**Uscita M0**: un progetto UE vuoto che compila, apre e sta nel repo. *(Scaffolding già presente — vedi §Scaffolding sotto.)*

---

## M1 — Sandbox  ·  *target sett. 5*

| CP | Obiettivo | Definition of Done | Verifica |
|---|---|---|---|
| 1.1 | Livello + camera tattica | `L_Prototype` con `BP_TacticalCamera` (SpringArm, pitch ~-55°) | In PIE la camera inquadra l'arena dall'alto |
| 1.2 | Enhanced Input | `IA_Pan/IA_Zoom/IA_Select` + `IMC_Tactical` → pan e zoom | Pan e zoom rispondono a input |
| 1.3 | Griglia logica | `ARTGridManager` (10×10, cella 200) con `CellToWorld`/`WorldToCell`/`IsInsideGrid` | Test: `WorldToCell(CellToWorld(c)) == c` per ogni cella valida |
| 1.4 | Griglia visuale | Instanced Static Mesh + evidenziazione cella sotto il cursore | La cella sotto il mouse si evidenzia in PIE |
| 1.5 | Selezione | Interfaccia `BPI_Selectable`; click seleziona un'unità segnaposto | Click su unità → evidenziata; click a vuoto → deseleziona |

**Uscita M1**: si naviga l'arena e si seleziona un'unità.

---

## M2 — Turn loop  ·  *target sett. 8*

| CP | Obiettivo | Definition of Done | Verifica |
|---|---|---|---|
| 2.1 | State machine fasi | `ERTMatchPhase{Planning,Prep,Dash,Blast,Move,Cleanup,MatchEnded}` in `ARTGameState`/`ARTTurnManager` | Un turno scorre tutte le fasi in ordine (log/HUD di fase) |
| 2.2 | Timer + lock-in | Timer **30s** in Planning; "Lock In" congela il piano; scadenza = lock automatico | Il timer scade → fase avanza senza input |
| 2.3 | Pianificazione azioni | `FRTPlannedAction{Unit, AbilityIndex, TargetCell, Destination, Phase, bLocked}` per unità | Ogni unità mostra il piano scelto prima del lock |
| 2.4 | Risoluzione movimento | Conflitti: destinazione contesa → ferme; scambio diretto → consentito; cella occupata da unità ferma → bloccato | Test: i 3 casi di conflitto danno l'esito atteso; l'**ordine dell'array non cambia il risultato** |

**Uscita M2**: un turno completo con movimento simultaneo risolto in modo deterministico.

---

## M3 — Combat loop  ·  *target sett. 11*

| CP | Obiettivo | Definition of Done | Verifica |
|---|---|---|---|
| 3.1 | Abilità data-driven | `URTAbilityData` + `DA_RifleShot/DA_EnergyShield/DA_Dash` assegnate alle unità | Le abilità appaiono nella barra e sono selezionabili |
| 3.2 | Danno/scudo/morte | `URTCombatLibrary::DamageAfterShield` (scudo assorbe prima); morte **dopo il batch Blast** | Test: `DamageAfterShield(30, 20, 100)` e casi limite danno i valori attesi |
| 3.3 | Energia + ultimate | Energia max 100; ultimate sbloccata a energia piena | Guadagno energia per turno/colpo osservabile; ultimate attivabile solo a 100 |
| 3.4 | Status + Gameplay Tags | Root/Slow/Shield/Reveal via tag, con durata a turni | Test: un'unità con `Status.Root` non può muoversi |
| 3.5 | Targeting a forme | `ERTTargetShape{Self,Single,Line,Cone,Circle}` collegato al resolver | Test: forma "Line" colpisce tutte le celle sulla linea del bersaglio |
| 3.6 | LOS / copertura | `HasLineOfSightTo` **usata** in `ResolveBlast`; copertura riduce il danno | Test: bersaglio senza LOS non subisce danno; dietro copertura ne subisce meno |

**Uscita M3**: combattimento completo su una singola unità e su forme, con status e copertura.

> ⚠️ Correzioni note dai tutorial (da non ripetere): in `02` la LOS era scritta ma **mai usata** nel
> resolver, e il "range di movimento 4" non era **mai validato** lato regole. Entrambi vanno chiusi qui.

---

## M4 — Vertical slice  ·  *target sett. 13*

| CP | Obiettivo | Definition of Done | Verifica |
|---|---|---|---|
| 4.1 | Bot | Utility scoring pianifica le **2 unità nemiche** ogni turno | La squadra nemica gioca da sola, senza input |
| 4.2 | HUD + combat log | Salute, energia, abilità, timer, fase + log testuale degli eventi | Tutti gli elementi si aggiornano durante il match |
| 4.3 | Vittoria + riavvio | La partita termina quando una squadra è eliminata; schermata fine; riavvio | Playthrough 2v2 completo dall'avvio alla fine, poi restart |

**Uscita M4**: MVP giocabile 2v2 contro bot, dall'avvio alla vittoria.

---

## M5 — Release interna  ·  *target sett. 14*

| CP | Obiettivo | Definition of Done | Verifica |
|---|---|---|---|
| 5.1 | Suite test verde | Test su griglia, danno-dopo-scudo, ordine fasi, conflitti movimento | Tutti i test passano da CLI (`-ExecCmds="Automation RunTests ..."`) |
| 5.2 | Packaging Windows | Build **Development** e **Shipping** che parte senza editor | L'eseguibile pacchettizzato si avvia e si gioca su un PC senza UE |
| 5.3 | Definition of Done MVP | Checklist [piano canonico §4](piano-canonico-mvp.md#4-definizione-dellmvp-vertical-slice) tutta spuntata | Revisione voce per voce |

**Uscita M5**: MVP pacchettizzato, testato e distribuibile internamente.

---

## Dopo l'MVP (north-star)

Vedi [piano canonico §8](piano-canonico-mvp.md#8-north-star-post-mvp-dai-prd): P0 multiplayer
server-authoritative → P1 4v4/eroi/replay/**Intenti condivisi** → P2 GAS/accessibilità/mappa
multilivello → P3 console/modding/anti-cheat.

## Scaffolding

Lo scheletro del progetto UE (`.uproject`, target/module C++, `Config/`) è già nel repo come base di
M0. Va aperto in **UE 5.8.1** (che rigenera i file di soluzione e compila il modulo) prima di procedere
con M1. Le classi di gioco vere si creano milestone per milestone, non in anticipo.
