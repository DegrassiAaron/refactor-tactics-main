# Technical Designer Integration Pass — Audit (R0) e R1

**Data**: 2026-08-30
**Scope deciso**: audit + R1 (#1705). R2..R10, #1665, il packaged e la Sessione Editor sono **fuori** da
questa passata e restano backlog ordinato.
**Checkout canonico**: `D:/Repositories/refactor-tactics-main`
**Branch**: `feat/td-integration-pass`, aperto da `origin/main` @ `c0cc0693`

---

## 1. Precondizioni riconciliate prima di toccare qualsiasi cosa

Tre erano bloccanti e nessuna delle tre era dichiarata nell'handoff.

### 1.1 Il checkout nominato non era un repository

L'handoff dà come nome operativo `refactor-tactics-technical-designer` e apre (§18) con sei comandi git.
Misurato: quella directory **non è un repository** (`fatal: not a git repository`). Su `D:/Repositories`
esistono cinque candidati più due worktree registrati (`rt-wt-t6` su `docs/spec-panel-bot-stall`).

Il checkout su cui l'Editor è realmente aperto — e quindi il solo che soddisfa la hard rule «2 terminali,
1 checkout» — è `refactor-tactics-main`. Adottato come canonico.

### 1.2 Il checkout era 141 commit indietro

| | prima | dopo |
|---|---|---|
| HEAD | `3c0123a9` | `c0cc0693` |
| rispetto a origin/main | **-141** | allineato |

⚠️ **Conseguenza che avrebbe falsato l'intero audit**: su `3c0123a9` il simbolo
`URTDevSandboxLauncherSubsystem` **non esiste**, e nessun file sorgente contiene `Launcher` o `DevSandbox`
nel nome. Un audit fatto lì avrebbe concluso che il launcher di #1680 non era mai stato implementato — e
avrebbe riaperto come regressione una issue chiusa correttamente. Il launcher esiste, in
`Source/RefactorTacticsEditor/`, con i suoi cinque test.

### 1.3 Sei `.uasset` sporchi, di stamattina, su path della passata

`git status` all'apertura mostrava 7 file modificati e 8 cancellati, con mtime **2026-08-30 09:15**. Non
erano residui: `RTFrontendWidgetAssetTests.cpp` conteneva una `DescribeBrush()` nuova, cioè lavoro su
#938. Quattro dei path collidevano con i 141 commit in arrivo: il pull sarebbe fallito.

Messi al sicuro su `wip/938-outline-diagnostica`, **separati per owner** invece che in un commit unico:

| commit | contenuto | attribuzione |
|---|---|---|
| `4ca3fca7` | `RTFrontendWidgetAssetTests.cpp`, `WBP_RT_MenuEntry`, `WBP_RT_ErrorModal` | #938, dal contenuto del diff |
| `37856c80` | `BP_Unit_Phase/Riktor/Wraith` | #1719 **ipotizzata**: sono le tre unità di quella issue e crescono della stessa quantità. Non verificata — sono binari |
| `fb190a94` | `BP_GameMode`, `WBP_RT_ScenarioComposer` (untracked) | **senza owner accertato** |
| `83b4201c` | 17 asset di design mai entrati in git + le 8 cancellazioni di radice | le cancellazioni erano già fatte a monte |

🔴 **Da decidere, fuori da questa passata**: `Content/RT/UI/Scenario/WBP_RT_ScenarioComposer.uasset` non
esiste in `origin/main`, nessuna issue lo nomina e nessun codice lo referenzia. Il nome lo colloca sulla
rotta di #1705/#1682. Va identificato prima di considerarlo lavoro utile o residuo di un esperimento.

---

## 2. Correzioni alla baseline dell'handoff (§2)

La baseline è **sostanzialmente accurata**: 21 issue misurate, 3 deviazioni, tutte nella direzione di
«più chiuso di quanto dichiarato».

| Issue | Handoff dichiara | Misurato il 2026-08-30 | Nota |
|---|---|---|---|
| #1679 | «verificare live» | **CLOSED** | L0 contract consegnato |
| #1681 | «verificare live» | **CLOSED** | l'asse è deciso, strada 🅐 |
| #623 | «dichiarata consegnata da #1105» | **CLOSED** in proprio | non è un'assunzione di secondo grado: ha una chiusura sua |

Le altre 18 coincidono con quanto dichiarato.

---

## 3. Tabella di riconciliazione

`Issue | Before | Action | After | Parent | Why | Evidence`

| Issue | Before | Action | After | Parent | Why | Evidence |
|---|---|---|---|---|---|---|
| #1105 | OPEN | LINK_ONLY | OPEN | — | epic tooling, nessuna mutazione dovuta | `gh issue view 1105` |
| #1678 | OPEN | LINK_ONLY | OPEN | #1105 | parent del launcher; #1705 è la slice che avanza | `gh issue view 1678` |
| #1679 | «da verificare» | ALREADY_DONE | CLOSED | #1678 | contract L0 consegnato | stato live |
| #1680 | CLOSED | REUSE | CLOSED | #1678 | il subsystem esiste e ha 5 test | `Source/RefactorTacticsEditor/Private/RTDevSandboxLauncherSubsystem.cpp` |
| #1681 | «da verificare» | ALREADY_DONE | CLOSED | #1678 | asse deciso, consumato da #1705 | stato live |
| **#1705** | OPEN | **IMPLEMENT (R1)** | OPEN → PR | #1678 | è R1 | §4 di questo documento |
| #1682 | OPEN | DEFER | OPEN | #1678 | fuori scope: R2 | dipende da #1705 |
| #1683 | OPEN | DEFER | OPEN | #1678 | «funzionante prima, persistente dopo» | — |
| #939 | CLOSED | REUSE | CLOSED | #934 | percorso 2v2 canonico, non si rifà | `URTFrontendNavigator::StartMatch` |
| #934 | OPEN | LINK_ONLY | OPEN | — | owner del player flow | `gh issue view 934` |
| #1330 | OPEN | DEFER | OPEN | #934 | fuori scope: C6 | non riprodotto in questa passata |
| #613 | OPEN | DEFER | OPEN | — | fuori scope: R5 | non rimisurato |
| #1545 | CLOSED | NO ACTION | CLOSED | #613 | non duplicare | stato live |
| #1608 | CLOSED | NO ACTION | CLOSED | — | non duplicare | stato live |
| #623 | «dichiarata» | ALREADY_DONE | CLOSED | #1105 | chiusura propria, non derivata | stato live |
| #1174 | CLOSED | NO ACTION | CLOSED | — | nessuna regressione osservata | stato live |
| #1714 | OPEN | DEFER | OPEN | — | fuori scope: R4 | — |
| #1095 | OPEN | DEFER | OPEN | — | fuori scope: R4 | — |
| #1665 | OPEN | DEFER | OPEN | — | **escluso dallo slice per decisione di scope** | — |
| #1719 | OPEN | UPDATE (nota) | OPEN | — | tre `.uasset` in `37856c80` sono probabilmente suoi | `git show 37856c80 --stat` |
| #938 | OPEN | UPDATE (nota) | OPEN | — | lavoro non committato salvato in `4ca3fca7` | `git show 4ca3fca7 --stat` |

### Sezioni

- **REUSED** — #1680, #939, #1679, #1681, #623 (i cinque su cui il piano poggia e che esistono davvero).
- **UPDATED** — #1719 e #938: hanno lavoro binario recuperato su un branch, e vanno informate.
- **CREATED** — nessuna. La candidata unica è il bridge TD→Play (§5), e **non è stata creata**: vedi lì.
- **CLOSED** — nessuna chiusura eseguita in questa passata.
- **DEFERRED** — #1682, #1683, #1330, #613, #1714, #1095, #1665: fuori dallo scope «audit + R1».
- **NO ACTION** — #1545, #1608, #1174, #1105, #934, #1678.

---

## 4. R1 — #1705, la lista e il readout

Cosa è stato aggiunto, e dove passa il confine fra ciò che un test vede e ciò che non vede.

| File | Ruolo |
|---|---|
| `Public/RTLauncherScenarioBrowser.h` · `Private/RTLauncherScenarioBrowser.cpp` | **modello puro**: ricerca, classificazione del vuoto, readout. Nessun accesso al disco |
| `Private/Tests/RTLauncherScenarioBrowserTests.cpp` | 5 test, namespace `RefactorTactics.DevSandboxLauncher.*` |
| `Private/SRTLauncherScenarioPanel.h` · `.cpp` | il pannello Slate: dispone widget e chiama l'indice, nient'altro |
| `Private/RTDevSandboxLauncherSubsystem.cpp` | il tab monta il pannello al posto del segnaposto di #1680 |
| `RefactorTacticsEditor.Build.cs` | `+ ToolWidgets` per `SSearchBox` |

**Scelta portante**: la stessa di #1680 con `ShouldOpenFor`. Di una slice fatta di Slate, un automation
test vede solo la decisione — quindi ricerca, stato vuoto e formattazione stanno in funzioni statiche
pure, e il pannello resta un guscio. Una regola scritta nel widget è una regola che nessun test guarda.

**Acceptance criteria di #1705, e dove sono soddisfatti**

| AC | Dove | Verificato da |
|---|---|---|
| tendine col vocabolario **reale** | `URTScenarioIndex::ListTags()`, nessun elenco a mano | ispezione |
| due filtri si **intersecano** | `ListIds(A, B)`, non reimplementato | `TagFiltersIntersect` sul corpus vero |
| la ricerca restringe l'elenco **già filtrato** | `ApplySearch` non sa da dove pescare altri id | `SearchNarrowsTheFilteredList` |
| terreno **come dichiarato** | `DescribeTerrain`: `fixture <nome>` \| `radius <n>` | `TerrainReadoutKeepsTheDeclaredForm` |
| composizione = readout, non colonna | si legge dopo l'apertura di **uno** scenario | `CompositionCountsPerTeam` |
| il vuoto dice **quale** causa | `Classify` + due messaggi distinti | `EmptyListNamesItsCause` |
| nessuna aritmetica nella UI | i valori vengono da `ListIds` e `FRTScenarioSummary` | ispezione |

**Guardrail rispettati**: nessuna scansione di directory parallela; si apre **uno** scenario alla
selezione e non 88 a ogni ridisegno; i filtri sono una lente e non toccano `SelectedId`; il modello
scenario resta non-BlueprintType e si passa dai DTO.

**Fuori da #1705, e non anticipato**: `Start Session` (#1682), la persistenza della selezione (#1683),
qualunque comparsa del vocabolario `Format.*`.

### Esito della misura

Compilata e misurata: vedi §6. **10/10 completati, 0 fallimenti, run dichiarata VALIDA.**

---

## 5. Il bridge TD → Play Default 2v2: perché non è stato creato

L'handoff (§3) prevede al massimo una issue nuova, e solo se non esiste un owner. Misurato:

**Su GitHub** — quattro ricerche (`Play Default 2v2`, `DevSandbox Play`, `launcher Play`,
`partita default`) non restituiscono alcuna issue che possieda l'azione. #1678 è il parent del launcher,
#939 è il percorso di Play già consegnato, nessuna delle due copre il ponte fra i due.

**Nel codice** — il seam esiste e ha un nome: **`URTFrontendNavigator::StartMatch()`**, che restituisce
`ERTNavResult`. È già protetto da una nota esplicita nell'header — *«Passa dallo stesso `StartMatch`, non
da una seconda via d'avvio»* — e progettato come *«chiede, non apre»*: pubblica una richiesta pendente e
azzera il livello richiesto quando qualcuno la consuma. Il modulo `RefactorTacticsEditor`, correttamente,
**non contiene alcun riferimento** a `Skirmish2v2`, `MatchFormat` o `ARTGameMode`.

🔴 **Il vincolo che l'handoff non nomina**: `URTFrontendNavigator` è un subsystem che, per stessa
ammissione del launcher, non esiste fuori da PIE — *«fuori da PIE non esiste una `GameInstance`»*. Un
«Play Default 2v2» premuto nel launcher d'Editor **non può chiamare `StartMatch` direttamente**: deve
avviare PIE e far consumare la richiesta dal percorso esistente. Questo cambia la forma dell'issue, non
solo il suo testo.

**Perché non è stata creata qui**: lo scope di questa passata è audit + R1, e una issue di
implementazione creata senza il suo posto nell'ordine di merge è tracking che invecchia. Il gap è reale,
è documentato qui con il simbolo e il vincolo, ed è pronto da aprire quando R3 entra in scope.

---

## 6. Referto di build e test

### Build

```
comando   : Build.bat RefactorTacticsEditor Win64 Development
            -Project=D:\Repositories\refactor-tactics-main\RefactorTactics.uproject -WaitMutex
HEAD      : c0cc0693  (branch feat/td-integration-pass)
esito     : Succeeded
durata    : 58,93 s
exit code : 0
```

⚠️ **Sono servite tre esecuzioni, e le prime due sono informative.**

| # | esito | causa |
|---|---|---|
| 1 | Failed | Live Coding attivo: l'Editor era aperto sul checkout |
| 2 | Failed (5 errori) | 2 miei + **3 preesistenti**, vedi sotto |
| 3 | **Succeeded** | dopo i tre fix |

I due miei: `TStrongObjectPtr` istanziato su un tipo incompleto (forward declaration insufficiente) e un
`C4458` — una variabile `Tag` che nasconde `SWidget::Tag`, che in questo progetto è un errore.

🔴 **Il terzo non era mio, ed è il più interessante.** `RTBuildIconCatalogCommandlet.cpp` e
`RTBuildGrayboxMeshesCommandlet.cpp` definiscono entrambi una `SaveAssetPackage` **identica** in un
namespace **anonimo**. In compilazione separata non si vedono; nella unity build finiscono nella stessa
unità di traduzione e la seconda definizione è un `C2084`. Il difetto era latente: **aggiungere sorgenti
al modulo cambia come UBT li raggruppa**, quindi sarebbe emerso al primo che aggiungeva un file lì, senza
aver toccato nessuno dei due commandlet. Risolto rinominando (`SaveIconAssetPackage`) e dichiarando nel
commento che è il cerotto e non la cura — la cura è una sola funzione condivisa, fuori dal raggio di R1.

### Test

```
comando   : ./scripts/rt-suite.ps1 -Filter "RefactorTactics.DevSandboxLauncher"
            -LogName rt-suite-launcher.log -WaitMinutes 40 -PollSeconds 30
HEAD      : c0cc0693   albero e4fd78c9 (9 file)
verdetto  : VALIDA
found N   : 10
performed : 10
fail N    : 0
durata    : 00:29  (+ 741 s di attesa del motore)
exit code : 0
```

I dieci, per nome — cinque di #1680 e cinque di #1705, nessun conteggio ereditato:

| test | issue |
|---|---|
| `OpensOnTheBootstrapLevel` · `DoesNotOpenOnGameplayLevels` · `PathSpellingDoesNotChangeTheAnswer` · `BootstrapNameMatchesTheEditorStartupMap` · `SubscribesOnInitialize` | #1680 |
| `TagFiltersIntersect` · `SearchNarrowsTheFilteredList` · `EmptyListNamesItsCause` · `TerrainReadoutKeepsTheDeclaredForm` · `CompositionCountsPerTeam` | **#1705** |

⚠️ **La prima esecuzione è uscita `2` e non è un fallimento**: `rt-suite` ha rilevato una run di
automation su un **altro checkout** (`D:\Repositories\wt-verifica-main`, filtro `RefactorTactics.Scenario`)
e si è rifiutata di partire — il mutex del motore è globale sull'eseguibile e vale fra checkout diversi.
Rilanciata con `-WaitMinutes 40`, è partita da sola dopo 741 s. Nessun processo altrui è stato terminato.

⛔ **Cosa questi test non dicono**: che le tendine si popolino, che la lista si ridisegni mentre si digita
e che il readout compaia sono Slate su un editor vivo. Restano voce di seduta, insieme a `U31`.

---

## 7. Cosa resta aperto, per chi riprende

1. **`WBP_RT_ScenarioComposer.uasset`** — binario senza owner in `fb190a94`. Identificarlo.
2. **`wip/938-outline-diagnostica`** — quattro commit da riconciliare: #938 e #1719 vanno informate, e
   l'attribuzione dei tre `BP_Unit` va confermata riaprendoli nell'Editor.
3. **Il bridge TD→Play** — §5: gap reale, issue non creata, vincolo PIE documentato.
4. **R2 (#1682)** — sbloccata da R1: `Start Session` ora ha qualcosa da selezionare.
