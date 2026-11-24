# Modello del danno (fetta 1 di #4188) — piano di esecuzione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** portare in codice le regole del modello del danno di `D-634`: elemento sull'azione, danno elementale contro la RES, scudo base sul solo `Direct`, marchio e +80 contro `Wet` nel Base, perforante, reazioni ad almeno 2 AP ([#4189](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4189)).

**Architecture:** C++ in `Source/RefactorTactics/` (Ability, Combat, Turn), test d'automazione in `Source/RefactorTactics/Tests/`. Ramo `feat/4189-modello-danno`, worktree `.claude/worktrees/4189-modello-danno`, base `origin/main` = `ce1426769`. Le azioni del kit di oggi restano senza elemento.

**Tech Stack:** Unreal Engine 5.8 (`D:\EpicGames\UE_5.8`), C++, Automation Test Framework, `tools/suite/esegui.py`, `node tools/radar/*.ts`.

**Spec:** [`docs/superpowers/specs/2026-11-23-modello-danno-design.md`](../specs/2026-11-23-modello-danno-design.md)

## Global Constraints

- **Regole** (`D-634`):
  - (4): un colpo con elemento si riduce con la RES di quell'elemento, con la forma della DEF, `100/(100+RES)`;
  - (5): lo scudo base ferma solo il `Direct`;
  - (6): la RES del catalogo è il default per gli elementi (`ResistanceDefault`), accanto a `ElementResistance`;
  - (35): marchio (+60) e +80 contro `Wet` entrano nel Base, prima della difesa;
  - (47): il perforante usa difesa 0 e non sottrae la copertura bassa; Guard e scudo restano;
  - (29): una riga `Slot == Reaction` costa almeno la classe reazione, 2.
- **Il kit di oggi** resta senza elemento, cioè `Direct`. Il +80 contro `Wet` resta legato a `Hero.Gadget.LinearDischarge`.
- **Formula:** `max(0, (Base + Spalle) × Mod × 100/(100+difesa) − Copertura − Guard)`, un solo arrotondamento per difetto, in `URTCombatLibrary::MitigateDirectDamage` (D-589 (3), D-595 (1)).
- **Determinismo:** nessuna dipendenza dall'ordine di `TMap`/`TSet`; `ElementResistance` si legge solo per chiave.
- **Motore:** uno per macchina. Prima di compilare o lanciare test, leggi la `CommandLine` dei processi `UnrealEditor%` (`Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'"`); se è in uso da un'altra sessione, aspetta o dichiara `NOT RUN`.
  - Build: `D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat RefactorTacticsEditor Win64 Development -Project=<worktree>\RefactorTactics.uproject -WaitMutex`, in PowerShell leggendo `$LASTEXITCODE` (aggiungi `-NoHotReloadFromIDE` se un Editor interattivo è aperto in un altro clone).
  - Test: `python tools/suite/esegui.py <filtro> --log <FILE nello scratchpad>`; la prima suite in un worktree nuovo può impiegare molti minuti ad avviarsi (`--timeout 15` o più).
- **Test:** un test nuovo non ha un nome che sia prefisso di un altro (il prefisso fa sparire l'altro dalla suite).
- Commit `<type>(4189): <descrizione>`, percorsi espliciti in `git add`, riga `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.

## Review Focus

1. **Un colpo senza elemento che cambia numero.** Le azioni di oggi devono dare lo stesso danno di prima, salvo il marchio. Test: un'azione di oggi contro un bersaglio con DEF 45 e RES 35 dà lo stesso danno prima e dopo (Task 2).
2. **Bonus contato due volte o nel punto sbagliato.** Con il marchio e Exposed sullo stesso colpo, il bonus entra una volta sola, nel Base. Con difesa 45: `floor((Base + 60) × 120 × 100 / (100 × 145))` meno la copertura (Task 3).
3. **Scudo base che ferma un colpo elementale**, o uno scudo temporaneo che ferma il `Direct` prima dell'elementale. Test sul caso misto (Task 4).
4. **Perforante che salta anche la Guard o lo scudo** (Task 5).
5. **Validatore che rifiuta righe di oggi:** nessuna riga reale va sotto 2; `Action.Overwatch` è Prep, non reazione, e resta a 1 (Task 6).

---

### Task 1: Elemento sull'azione, fino al colpo

**Files:** `Source/RefactorTactics/Ability/RTActionDef.h` (campo `ElementId`), `Ability/Catalog/RTCatalogLibrary.cpp` (`ValidateActions`), `Combat/RTHexCombatLibrary.h` (`FRTHexAttackIntent`, `FRTHexAttackHit`), il punto dove l'intento nasce (`Turn/RTTurnManager_Blast.cpp` ~:1689); test nuovi in `Tests/Combat/Damage/`.

**Interfaces:** Produces: `FName FRTActionDef::ElementId` (vuoto = nessun elemento); `FName FRTHexAttackIntent::ElementId`, `FName FRTHexAttackHit::ElementId`.

- [ ] **Step 1:** Test: `ValidateActions` rifiuta un `ElementId` che non è in `URTHeroCatalogLibrary::ElementIds()`, accetta `NAME_None` e `Element.Electric`.
- [ ] **Step 2:** Test: un'azione di test con `ElementId = Element.Electric` produce un intento e un colpo con lo stesso `ElementId`; un'azione senza elemento, `NAME_None`.
- [ ] **Step 3:** Implementa; build; test della famiglia nuova verdi; i test esistenti di catalogo (`RefactorTactics.Catalog`) verdi.
- [ ] **Step 4:** Commit `feat(4189): elemento sull'azione, portato dall'intento al colpo`.

### Task 2: RES nello snapshot e danno elementale

**Files:** `Combat/RTHexCombatLibrary.h/.cpp` (`FRTHexCombatUnit`, `MitigatedHitPower`), `Turn/RTTurnManager_Blast.cpp` (~:518-519, la copia nello snapshot); test.

**Interfaces:** Consumes: `FRTHexAttackHit::ElementId`. Produces: `FRTHexCombatUnit::ResistanceDefault`, `FRTHexCombatUnit::ElementResistance`, e `int32 FRTHexCombatUnit::DefenseAgainst(FName ElementId) const` (DEF per `NAME_None`, altrimenti la RES dell'elemento).

- [ ] **Step 1:** Test: contro un bersaglio con DEF 45, `ResistanceDefault` 35 e `ElementResistance[Element.Fire] = 10`, un colpo da 240 dà `floor(240×100/145)` = 165 senza elemento, `floor(240×100/135)` = 177 con `Element.Electric`, `floor(240×100/110)` = 218 con `Element.Fire`.
- [ ] **Step 2:** Test del Review Focus 1: un'azione di oggi (`Hero.Gadget.ArcPulse`) contro lo stesso bersaglio dà lo stesso danno di prima.
- [ ] **Step 3:** Implementa; build; test verdi; `RefactorTactics.Combat` verde.
- [ ] **Step 4:** Commit `feat(4189): danno elementale contro la RES dell'elemento`.

### Task 3: Marchio e +80 contro `Wet` nel Base

**Files:** `Turn/RTTurnManager.cpp` (~:6800-6875), `Combat/RTHexCombatLibrary.cpp` (riuso del ricalcolo di `ApplyExposedToFirstHit`, ~:392-429); test esistenti del marchio e del bonus `Wet` (`Tests/Combat/Status/RTStatusTests.cpp`, `Tests/Combat/RTOffensiveActionTests.cpp`, `Tests/Combat/RTHexCombatIntegrationTests.cpp`, `Tests/Ability/RTHeroGadgetTests.cpp`).

**Interfaces:** Consumes: `DefenseAgainst` del Task 2.

- [ ] **Step 1:** Test con difesa maggiore di 0: `LinearDischarge` (240) contro un bersaglio `Wet` con DEF 45, di fronte, senza copertura, dà `floor(320×100/145)` = 220 (oggi 245); un colpo marcato da 240 contro DEF 45 dà `floor(300×100/145)` = 206 (oggi 165 + 60 = 225).
- [ ] **Step 2:** Test del Review Focus 2: marchio ed Exposed sullo stesso colpo, contro DEF 45, `floor((240+60)×120×100/(100×145))` = 248.
- [ ] **Step 3:** Implementa; i test esistenti con difesa 0 restano verdi senza cambiare i numeri; build; `RefactorTactics.Status`, `RefactorTactics.Actions`, `RefactorTactics.Heroes` verdi.
- [ ] **Step 4:** Commit `feat(4189): marchio e bonus contro Wet nel Base, prima della difesa`.

### Task 4: Scudo base sul solo `Direct`

**Files:** `Combat/RTCombatResolver.cpp` (~:33-46, somma per bersaglio), `Combat/RTCombatLibrary.h/.cpp` (`FRTAttack`, `ApplyDamage` ~:105-132); test.

**Interfaces:** Consumes: `FRTHexAttackHit::ElementId`. Produces: `FRTAttack` sa se è elementale.

- [ ] **Step 1:** Test: un bersaglio con scudo base 50 e nessuno scudo temporaneo prende 100 elementali → perde 100 HP; 100 `Direct` → perde 50.
- [ ] **Step 2:** Test del caso misto, con l'ordine del §5 punto 4 del design: scudo base 50, scudo temporaneo 30, colpi da 40 elementali e 60 `Direct` sullo stesso bersaglio. Il temporaneo assorbe 30 dei 40 elementali (10 passano); lo scudo base assorbe 50 dei 60 `Direct` (10 passano): HP persi 20, scudo a 0.
- [ ] **Step 3:** Implementa; build; `RefactorTactics.Combat` e i test dello scudo base (D-224) verdi.
- [ ] **Step 4:** Commit `feat(4189): lo scudo base ferma solo il danno Direct`.

### Task 5: Danno perforante

**Files:** `Ability/RTActionDef.h` (`bPiercing`), `Combat/RTHexCombatLibrary.cpp` (mitigazione e copertura), il passaggio dall'azione all'intento; test.

- [ ] **Step 1:** Test: un'azione perforante da 420 contro DEF 45 in copertura bassa frontale dà 420; con la Guard attiva, 420 − 100 = 320; lo scudo base vale come per ogni colpo `Direct`.
- [ ] **Step 2:** Implementa; build; test verdi.
- [ ] **Step 3:** Commit `feat(4189): danno perforante, senza difesa né copertura bassa`.

### Task 6: Reazioni ad almeno 2 AP

**Files:** `Ability/Catalog/RTCatalogLibrary.cpp` (`ValidateActions` ~:287-297); test in `Tests/Ability/RTActionPointTests.cpp`.

- [ ] **Step 1:** Test: una riga `Slot == Reaction` con `ActionPointCost = 1` è rifiutata; ogni riga reale del catalogo core e d'eroe passa (`RefactorTactics.Catalog.EveryActionDeclaresACost` resta verde); `Action.Overwatch` resta a 1.
- [ ] **Step 2:** Implementa; build; test verdi.
- [ ] **Step 3:** Commit `feat(4189): una reazione costa almeno la sua classe`.

### Task 7: Corpus golden, documenti, suite intera

- [ ] **Step 1:** Suite `RefactorTactics` intera sulla testa del ramo: `PASS`, o ogni rosso spiegato e confrontato con `main`.
- [ ] **Step 2:** Corpus golden: `RefactorTactics.Simulation.GoldenCorpusMatches`. Se è rosso solo per il marchio con difesa maggiore di 0, rigenera con `-dpcvars="rt.Test.RegenerateGolden=1"` e tieni nel commit solo i file il cui cambio si spiega col marchio; mai il fixture legacy v10.
- [ ] **Step 3:** Documenti che descrivono la formula e il marchio (`git grep -n "MarkedFirstHitBonus\|bonus del marchio\|+60" -- docs`, `docs/balance/`, `RTCombatLibrary.h` commenti): aggiornati dove dicono che il bonus si somma dopo la difesa. Gate dei documenti a exit 0.
- [ ] **Step 4:** Commit `test(4189): corpus golden e documenti dopo il modello del danno`.
