# Tactical Designer v0.1 — integrazione visuale e percezione: audit e sessione 1

**Data**: 2026-08-30
**Base**: `origin/main` @ `aec66789`
**Worktree**: `D:/Repositories/refactor-tactics-technical-designer/refactor-tactics-main`
**Branch**: `feat/td-v01-tactical-visualization`, aperto da `origin/main`

---

## 1. Il lavoro parallelo, misurato prima di toccare qualsiasi cosa

Le tre directory nominate dall'handoff — `main`, `Taktics`, `Tactical Designer` — non corrispondono al
filesystem. Misurato:

| Nome nell'handoff | Sul disco | Branch | HEAD | Stato |
|---|---|---|---|---|
| `main` | `D:/Repositories/refactor-tactics-main` | `feat/td-integration-pass` | `c0cc0693` | 🔴 **dirty, sullo stesso scope** |
| `Taktics` | **non esiste**. Nessuna directory con quel nome, in nessuna forma | — | — | — |
| `Tactical Designer` | `D:/Repositories/refactor-tactics-technical-designer` — che **non è un repository**: contiene un clone annidato `refactor-tactics-main/` | `feat/td-v01-tactical-visualization` | `aec66789` | pulito, **questa sessione** |

Altri checkout dello stesso remoto, letti e non toccati: `rt-wt-ac4` (detached), `rt-wt-overlap`
(`test/unita-sovrapposte`, dirty, -48), `rt-wt-t6` (`docs/spec-panel-bot-stall`), `wt-verifica-main`
(detached), `refactor-tactict-dev` (`main`, -7).

### 🔴 La collisione, e come è stata evitata

`refactor-tactics-main` sta implementando **#1705** — lista scenari, ricerca, readout — e il lavoro è
**non committato**:

```text
 M Source/RefactorTacticsEditor/Private/RTDevSandboxLauncherSubsystem.cpp
 M Source/RefactorTacticsEditor/RefactorTacticsEditor.Build.cs
?? Source/RefactorTacticsEditor/Private/RTLauncherScenarioBrowser.{h,cpp}
?? Source/RefactorTacticsEditor/Private/SRTLauncherScenarioPanel.{h,cpp}
?? Source/RefactorTacticsEditor/Private/Tests/RTLauncherScenarioBrowserTests.cpp
```

Il suo referto è `docs/roadmap/plans/td-integration-pass-audit-2026-08-30.md`, e dichiara `#1682` differita
a R2. ∴ **ogni file di `Source/RefactorTacticsEditor/` è terreno altrui** finché quel lavoro non entra.
Questa sessione ha lavorato solo in `Source/RefactorTactics/` e su GitHub.

⚠️ Quel referto lascia anche `Content/RT/UI/Scenario/WBP_RT_ScenarioComposer.uasset`, binario **senza owner
accertato**, su `wip/938-outline-diagnostica`. Se fosse un tentativo di viewport dello scenario, #1753 lo
assorbe.

---

## 2. Audit delle issue — cosa il codice dice, contro cosa dicono i body

| Issue | Body dichiara | Codice su `aec66789` |
|---|---|---|
| **#1535** velo/viewer | *«nessuno chiama `ApplyKnowledgeVeil`»* | ✅ **superata**: `URTKnowledgeVeilPresenter` esiste (`63acb5bd`, `E-SOLID` fetta 4), chiama `ApplyKnowledgeVeil(TM->KnowledgeForTeamPublic(ViewerTeamId()))`, e la decisione sul viewer è scritta: è `ARTPlayerController::PlayerTeamId`. `ApplyKnowledgeVeilForViewer` e `ViewerTeamId` **come campo del GameMode non esistono più** |
| **#1712** LOS debug | la reason *«oggi nessuno la espone»* | ✅ era vero. **Metà consegnata qui**: vedi §4 |
| **#1115** initial state nel viewport | **CLOSED**, e l'epic la elenca fra le consegnate | 🔴 il commento di chiusura affida *«il click sulla cella»* e il viewport al terminale Editor: `Source/RefactorTacticsEditor/` non contiene **nessun** riferimento a `FRTScenarioUnit` o `URTScenarioAuthoring`. Consegna parziale che nessuno aveva raccolto |
| **#1625** playback TD | — | dichiara out of scope *«qualunque filtro di conoscenza/privacy»*: coerente, e non va toccata |
| **#80** `rt.Debug.*` | otto comandi | dieci registrati; **nessuno** riguarda la LOS |
| #1678 · #1682 · #1683 · #1705 · #1095 · #1714 · #622 · #695 · #1186 · #711 · #1715 · #1525 · #1626–#1630 | OPEN | OPEN, nessuna PR aperta le tocca |

Le tre PR aperte (#1699, #1694, #1674) sono su altri domini.

### L'API canonica oggi, per chi riprende

```text
ARTTurnManager::KnowledgeForTeamPublic(TeamId) -> FRTTeamKnowledge      (in partita)
URTTeamKnowledgeLibrary::Observe(Map, TeamId, Turn, Observers, ...)     (puro, senza world)
URTPerceptionLibrary::TeamVisibleCells(Map, TArray<FRTPerceiver>)       (unione vera)
FRTKnowledgeVerdict::Everyone() / AllowsTeam(n)                         (predicato di presentazione)
ARTHexMapActor::ApplyKnowledgeVeil / SetKnowledgeDebugEnabled           (resa dei tre stati)
URTHexVisionLibrary::HasLineOfSight / DescribeLineOfSight               (LOS, pure, senza world)
```

🔑 **Il fatto che sblocca tutto**: `Observe`, `TeamVisibleCells` e `HasLineOfSight` prendono un
`URTHexMapAsset` e sono **pure**. Non servono né `UWorld` né `ARTTurnManager`. Il Tactical Designer può
essere consumer della percezione e della LOS canoniche **fuori da PIE**, senza duplicare nulla.

---

## 3. I tre gap: verdetto

| Gap | Verdetto | Owner |
|---|---|---|
| **A** — Tactical View `Omniscient`/`Team 0`/`Team 1` | **PARTIAL**: i pezzi canonici esistono tutti, `rt.Debug.Knowledge <team>` rende già i tre stati per una squadra qualsiasi — ma è un comando **PIE**, non ha una posizione `Omniscient` e non ha una superficie nella sessione | **#1754** (nuova) |
| **B** — LOS Inspector nel TD | **MISSING**: `Source/RefactorTacticsEditor/` non nomina `HasLineOfSight`, nessuna issue nomina un ispettore d'editor. #1712 possiede l'osservabilità **in partita**, che è un attore diverso | **#1755** (nuova) · produttore: **#1712** |
| **C** — scenario → viewport iniziale | **MISSING**: #1115 chiusa senza la sua metà viewport, #1682 dichiara di non costruire le superfici, #1625 è il playback | **#1753** (nuova) |

Le tre sono state agganciate al checklist di **#1105**, che ora porta anche la nota sulla consegna parziale
di #1115.

---

## 4. La slice consegnata: la ragione canonica della LOS (metà di #1712)

**Perché questa e non un'altra**: è l'unica dipendenza dura dei tre gap che vive in
`Source/RefactorTactics/`, cioè **fuori** dai file che l'altro terminale sta modificando.

| File | Cosa |
|---|---|
| `Map/RTHexVisionLibrary.h` | `ERTLineOfSightBlock { None, EdgeBlocker, CellBlocker }` · `FRTLineOfSightResult` · `DescribeLineOfSight` |
| `Map/RTHexVisionLibrary.cpp` | il corpo si sposta in `DescribeLineOfSight`; `HasLineOfSight` diventa `DescribeLineOfSight(...).IsClear()` |
| `Tests/RTHexVisionTests.cpp` | cinque test nuovi |

### 🔴 La scelta che devia dalla DoD di #1712, e perché

La DoD chiedeva *«una funzione pura **accanto** che ripercorre `HexLine` con le stesse due condizioni»* più
un test di parità. Sono **due** implementazioni: la parità vale sul corpus provato, e il giorno in cui una
regola cambia in una sola delle due il debug mente proprio quando serve.

Qui `DescribeLineOfSight` è la primitiva e `HasLineOfSight` il guscio: la parità non è asserita, è
**strutturale**. Il vincolo esplicito della DoD — *«la reason non si ottiene cambiando la firma di
`HasLineOfSight`»* — resta rispettato: la firma è identica e i suoi quattro test di comportamento sono gli
stessi e restano verdi.

⚠️ **`EdgeCover` sarebbe stato un nome che promette troppo.** Il predicato canonico è
`URTHexCoverLibrary::BlocksTraversal`, che risponde `true` anche per una **porta** chiusa o bloccata (CP
9.3). Il valore si chiama `EdgeBlocker`.

---

## 5. Referto di build e test

```text
build     : Build.bat RefactorTacticsEditor Win64 Development -NoHotReloadFromIDE
esito     : Result: Succeeded (330 s)
```

⚠️ **`-NoHotReloadFromIDE` è un bypass dichiarato, non una scorciatoia.** L'Editor dell'altro terminale è
aperto e Live Coding blocca UBT: il mutex è `Global\LiveCoding_<eseguibile>` e l'eseguibile è
`D:\EpicGames\UE_5.8\...\UnrealEditor.exe`, **condiviso da tutti i checkout** dello stesso engine
(`HotReload.cs:287`). La guardia esiste perché una sessione Live Coding non patchi gli object file del
target che si sta costruendo — e i due checkout hanno `Binaries/` e `Intermediate/` **disgiunti**, quindi
qui non c'è nulla da proteggere. Chiudere l'Editor altrui non era un'opzione.

⚠️ **`scripts/rt-suite.ps1` non è stato usato, ed è una perdita dichiarata.** Rifiuta di partire (exit 2)
finché esiste un processo `UnrealEditor*`, e quello dell'altro terminale c'è. La run è stata lanciata a mano
con **lo stesso comando** che rt-suite costruisce, quindi mancano le sue invarianti — HEAD, digest
dell'albero, freschezza del log. Chi riprende con il motore libero **rifaccia la misura con rt-suite**.

```text
comando   : UnrealEditor-Cmd RefactorTactics.uproject -ExecCmds="Automation RunTests <filtro>;Quit"
            -unattended -nopause -nosplash -nullrhi -NoLiveCoding
HEAD      : aec66789 + 3 file modificati
filtro    : RefactorTactics.HexVision   -> found 9,  fail 0
filtro    : RefactorTactics             -> vedi §5.1
```

### 5.1 Suite completa

```text
found     :
fail      :
```

---

## 6. Stato

**DONE**
- Audit dei worktree, delle 22 issue del perimetro e delle PR aperte.
- #1753, #1754, #1755 create; #1105 aggiornata.
- Ragione canonica della LOS + 5 test; 9/9 su `RefactorTactics.HexVision`.

**IN PROGRESS**
- Referto della suite completa (§5.1).
- Commento di consegna su #1712.

**NOT STARTED**
- #1753, #1754, #1755: nessuna riga di implementazione. Tutte e tre atterrano in
  `Source/RefactorTacticsEditor/`, che è terreno dell'altro terminale finché #1705 non entra.

**BLOCKED BY PARALLEL WORK**
- Qualunque slice d'editor. `RTDevSandboxLauncherSubsystem.cpp` e `RefactorTacticsEditor.Build.cs` sono
  dirty altrove.

**PIE ANCORA RICHIESTE**
- Leggibilità del graybox per uno scenario aperto (#1753).
- I tre stati di conoscenza a schermo e le transizioni durante il playback (#1754), confrontati con
  `rt.Debug.Knowledge <team>` come oracolo.
- Linea e readout della LOS nel viewport (#1755).

---

## 7. Prossimo passo esatto

1. `git fetch --prune origin` e rileggere: `origin/main`, i worktree, le PR. Il lavoro parallelo cambia.
2. Se `#1705` è entrata: `Source/RefactorTacticsEditor/` si libera, e **#1753** è la prima slice — senza un
   viewport che mostra lo scenario, né #1754 né #1755 hanno un soggetto.
3. Se non è entrata: restano indipendenti la seconda metà di **#1712** (comando nel dominio, scelta
   stampa-vs-disegna) e le fixture di scenario per i casi minimi di #1754 — visione semplice, unione,
   memoria, mai visto.
4. Con il motore libero, rifare la misura con `./scripts/rt-suite.ps1`.
