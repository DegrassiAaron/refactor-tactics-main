# La cura ad area — `CircularTide` deriva da `Action.Heal`, il percorso delle cure impara la forma `Area` — piano di implementazione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `Hero.Muiren.CircularTide` cura davvero ogni alleato nel suo raggio (18, o il valore della variante attiva), con una voce `Healed` per alleato nel TurnLog, un solo `AbilityActivated` di forma `Area`, il cooldown pagato anche a vuoto; la riga FX dell'abilità non promette più un colpo; il bot non la pianifica come attacco.

**Architecture:** il catalogo costruisce `CircularTide` con `MakeHeroActionFromCore(Action.Heal, Area, 1)` e riscrive i tre numeri dell'eroe; `CollectHealActions` (`Turn/RTTurnManager_Blast.cpp`) riconosce `Shape == Area`, calcola il centro prima di azzerare il piano, e accoda una cura per ogni compagna nel raggio nell'ordine canonico di `Ctx.Units`; `ApplyPlannedHeals` resta com'è. Nessun dato nuovo in snapshot, TurnLog o hash: cambiano gli **esiti** (ora cura), non lo schema.

**Tech Stack:** Unreal Engine 5.8.1, C++ (modulo `RefactorTactics`), Automation Test framework, scenari JSON, registro PIE in Markdown, sedute in YAML, generatore HUD in Python.

**Spec:** `docs/superpowers/specs/2026-10-08-clip-visibili-e-cura-ad-area-design.md` — approvata dall'autore e **ridotta dai fatti** (statuto): la parte clip è di #3595, qui si implementano §2.1 (D1, R1, R2 ribaltato, R7, R8), la riga FX (F4), §2.5, e si prepara §5.2.

**Stato misurato in pianificazione:** worktree `D:/Repositories/rt-wt-sp5-clip`, branch `issue/3593-cura-ad-area` su `origin/main` = `1c5f7150a` (il merge di #3595) più i commit della spec. Ogni `file:riga` qui sotto è letto su quell'albero; chi lo rilegge lo rimisura.

## Global Constraints

- Engine **UE 5.8.1** in `D:/EpicGames/UE_5.8`. Nessun aggiornamento di Engine, plugin o dipendenze.
- ⛔ **Nessun `.uasset`**, nessun Blueprint. ⛔ Nessuna `UFUNCTION` nuova, nessuna `UPROPERTY` nuova su `ARTUnit` (`Unit.BlueprintSurfaceIsCensused` non cambia).
- ⛔ **Nessun dato nuovo in snapshot, TurnLog, `StateHash`, replay**: si riusano `ERTCombatOutcome::Healed`, `ERTFallbackOutcome::Cancelled`, `ERTActionInvalidReason::NoEffect`.
- **Ordinamento solo esplicito**: i destinatari si prendono nell'ordine di `Ctx.Units` (canonico, `Turn/RTTurnManager_Blast.cpp:281`); nessuna `TMap`/`TSet` iterata per produrre un esito.
- **Elenco chiuso delle riscritture** dopo la derivazione (spec §2.1, F5): `Def.RangeCells` e lo specchio `RangeCells`, `Def.Priority`, `Def.Effects`. `Power` resta 0. `Fallback` resta quello del core.
- ⛔ **Nessun totale volatile** in commenti, documenti, celle PIE, YAML, issue, PR. ⛔ **Nessun glifo di stato (⏳ ✅ ❌ 🟡) accanto al nome di una voce PIE** in ciò che si pubblica su GitHub.
- Stile: commenti in **italiano**, nella forma dei file toccati. Nomi dei test nelle famiglie esistenti: `RefactorTactics.Heroes.*`, `RefactorTactics.Bot.*`, `RefactorTactics.Fx.*`.
- Line ending: i sorgenti e i documenti toccati sono **CRLF** (misurato con `file`: `Turn/RTTurnManager_Blast.cpp`, `Ability/RTHeroCatalogLibrary.cpp`, `Tests/RTEquipmentTests.cpp`, le spec). I file nuovi nascono CRLF. Gli scenari JSON in `Scenarios/Visual/Ability/` sono LF.
- Commit: `<type>(3593): <descrizione>`, chiuso da `Co-Authored-By: Claude <modello> <noreply@anthropic.com>` con il modello che ESEGUE il task. Un commit per task.
- `<checkout>` = `D:/Repositories/rt-wt-sp5-clip`. `<scratchpad>` = `C:/Users/Utente/AppData/Local/Temp/claude/D--Repositories-refactor-tactics-main/62a8cd51-c2fa-4563-b0ae-d82c35e12f64/scratchpad/sp5`. I log vanno in `<scratchpad>/logs/`, mai nel repository.
- **Motore uno per macchina**: prima di ogni build o run, `Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, Name, CommandLine`. Un Editor interattivo di **un altro clone** non blocca una build qui; un processo che misura tempi sì. La sessione parallela ha dichiarato di non lanciare suite finché questa non dice «libero»; non serve aspettarla.
- Build (Editor chiuso su questo clone):
  ```powershell
  & "D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -Project="<checkout>/RefactorTactics.uproject" -WaitMutex -NoHotReloadFromIDE
  ```
  Verifica nel log che i blob toccati siano **compilati** (`Compile [x64] Module.RefactorTactics.N.cpp`): «Target is up to date» dopo una modifica è un allarme.
- Test (filtro dopo `RunTests`, `+` fra più filtri, `;Quit` separato, `-abslog` fra virgolette):
  ```powershell
  & "D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<checkout>/RefactorTactics.uproject" "-ExecCmds=Automation RunTests <filtro>;Quit" -unattended -nopause -nosplash -nullrhi -NoLiveCoding "-abslog=<scratchpad>/logs/<nome>.log"
  ```
  Un esito vale **solo** se il log porta `**** TEST COMPLETE`. Conteggio (Git Bash, non PowerShell: il tool PowerShell rifiuta i comandi che contengono `****`): `grep -c '\*\*\*\* TEST COMPLETE' <log>` (atteso 1), `grep -c 'Result={Success}' <log>`, `grep -c 'Result={Fail}' <log>`, `grep -c 'Ensure condition failed' <log>` (atteso 0).
- **Una mutazione** è: modifica della riga indicata, build, test indicato, `Result={Fail}` sull'asserto indicato, ripristino con `git checkout -- <file>` **dopo il commit** del task (`git diff` vuoto), rebuild. ⚠️ Un `return` anticipato che lascia codice irraggiungibile non compila (C4702 è errore): si muta con un blocco sostituito o una guardia sempre vera. Elenca nel report **tutti** i test caduti, non solo quello atteso.
- **Le quattro trappole delle fixture di playback** (memoria di progetto): il viewer è la squadra 0 senza `InitializeActorsForPlay`; `StableUnitId` vale 0 prima di `LockInAndResolve()`; lo StateHash pendente è 0 senza registrazione; una finestra senza delegate. Qui conta la seconda: gli `AbilityActivated` si cercano per `SourceStableUnitId` **dopo** il turno.
- I pack Paragon sono gitignorati e **non** stanno in questo worktree: nessun test di questo piano li richiede.

## Review Focus

Le cinque condizioni che la spec implica e che nessun task esercita da solo, con il test che le pinna nel task che le possiede:

1. **Un'area che contiene solo chi cura** (nessun'altra compagna nel raggio): deve curare chi cura e basta, una voce `Healed`, nessun fallback — `Heroes.TideHealsAlliesInArea` (Task 2) copre il caso con compagne; aggiungere l'asserto «solo sé» come secondo caso nello stesso test.
2. **Centro mirato a un'unità** (`PlannedAttackTarget` invece di `PlannedAttackCell`): il centro è la cella del bersaglio e il bersaglio è curato se alleato — caso aggiunto a `Heroes.TideHealsAlliesInArea`.
3. **Centro fuori portata**: `Fallback/OutOfRange` come oggi, nessuna cura, cooldown **non** pagato (l'azione non è partita); un centro fuori mappa ma in portata non è un caso di questo percorso (`HexDistance` non guarda la mappa: lo scarta la validazione del click) — `Heroes.TideOnEmptyAreaStillStarts` porta il controllo opposto: a vuoto **dentro** la portata il cooldown si paga.
4. **La variante `Impact`** (cura 10 + spinta 1): cura 10, nessuna spinta, nessun errore — asserto aggiunto a `Heroes.TideHealsVariantAmount`.
5. **Il bot con la cura e un'altra abilità**: pianifica l'altra, non la cura — `Bot.DerivedHealIsNotAnAttackCandidate` (Task 1) con due abilità nei fatti.

---

### Task 0: Baseline — suite completa «prima», ledger, issue aggiornate

**Files:** nessuno nel repository (solo log e ledger nello scratchpad).

- [ ] **Step 1: Stato**: `cd <checkout>; git status --short` vuoto; `git log --oneline -1` = il commit della spec ridotta (`2b6b66326`). Registra HEAD nel ledger.
- [ ] **Step 2: Build baseline** (la DLL di questo worktree non esiste ancora): comando di build in Global Constraints; log `<scratchpad>/logs/t0-build-prima.log`; `Result: Succeeded`.
- [ ] **Step 3: Suite completa «prima»** (F7): filtro `RefactorTactics`, log `<scratchpad>/logs/t0-suite-prima.log`, in background. Dura a lungo: i Task 1–3 si scrivono intanto, ma **nessuna build** parte finché il log non porta `**** TEST COMPLETE`. Atteso: un solo `Result={Fail}`, `Packaging.RequiredActionClipsAreCooked`.
- [ ] **Step 4: Elenco dei nomi**: `grep -o 'Path={RefactorTactics[^}]*}' t0-suite-prima.log | sort -u > <scratchpad>/logs/nomi-prima.txt`. Serve al Task 5 per il diff.

---

### Task 1: Catalogo e bot — `CircularTide` deriva da `Action.Heal`; il bot non la pianifica come attacco

**Files:**
- Modify: `Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp:504-522` (la riga di `CircularTide`), `:660-680` (le varianti, indirizzate per indice)
- Modify: `Source/RefactorTactics/Tests/RTHeroCatalogTests.cpp:612-672` (`DerivedActionsDeclareTheirOrigin`: riga, conteggi, elenco delle proprie)
- Modify: `Source/RefactorTactics/Tests/RTHeroMuirenTests.cpp` (nuovo test accanto a `TideHealsWithoutWetting`, `:137-160`)
- Modify: `Source/RefactorTactics/Bot/RTBotPlanningLibrary.cpp:850-858` (passo 2 dei candidati)
- Modify: `Source/RefactorTactics/Tests/RTBotPlanningTests.cpp` (nuovo test, fixture `RTBotPlanningTestsInternal`)

**Interfaces:**
- Consumes: `URTHeroCatalogLibrary::MakeHeroActionFromCore(const FName& HeroActionId, const FName& CoreActionId, int32 Cooldown, ERTAbilityShape Shape, int32 AreaRadius)` (`RTHeroCatalogLibrary.cpp:1051-1066`: copia fase, priorità, portata, fallback, effetti del core; scrive `DerivedFromActionId`, `bSelfTarget`, `LineOfSightPolicy`; **non** copia `Slot` né `MovementStyle`); `MakeHeroAction` (`:88-125`) per gli specchi legacy.
- Produces: `Muiren->Actions[1]` con `Def.DerivedFromActionId == Action.Heal`, `Def.RangeCells == 4`, `Def.Priority == 60`, `Def.Effects == {Heal 18}`, `Shape == Area`, `AreaRadius == 1`, `Def.Fallback == Cancel` (dal core), `Power == 0`.

- [ ] **Step 1: Test del catalogo, rosso.** In `Tests/RTHeroMuirenTests.cpp`, dopo `FRTPhaseTideHealsWithoutWettingTest`:

```cpp
/**
 * #3593: la cura passa dal percorso delle cure SOLO se deriva da `Action.Heal` (`IsCoreAction`,
 * `Turn/RTTurnManager_Blast.cpp`). I numeri restano quelli dell'eroe (spec SP5 §2.1, R1): la derivazione dice
 * da quale percorso passa, non con quali numeri. `Power` resta 0 — e' il danno letto dal bot, non la cura.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPhaseTideDerivesFromHealTest,
	"RefactorTactics.Heroes.Phase.TideDerivesFromHeal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPhaseTideDerivesFromHealTest::RunTest(const FString&)
{
	const URTActionData* Tide = URTHeroCatalogLibrary::MakeMuiren()->Actions[1];
	if (!TestNotNull(TEXT("CircularTide all'indice 1"), Tide)) { return false; }
	TestEqual(TEXT("deriva da Action.Heal"), Tide->Def.DerivedFromActionId, FName(TEXT("Action.Heal")));
	TestEqual(TEXT("portata dell'eroe, non del core"), Tide->Def.RangeCells, 4);
	TestEqual(TEXT("specchio legacy della portata"), Tide->RangeCells, 4);
	TestEqual(TEXT("priorita' dell'eroe, non del core"), Tide->Def.Priority, 60);
	TestEqual(TEXT("un solo effetto"), Tide->Def.Effects.Num(), 1);
	if (Tide->Def.Effects.Num() == 1)
	{
		TestTrue(TEXT("e' una cura"), Tide->Def.Effects[0].Effect == ERTActionEffect::Heal);
		TestEqual(TEXT("cura 18, non i 20 del core"), Tide->Def.Effects[0].Amount, 18);
	}
	TestTrue(TEXT("forma area"), Tide->Shape == ERTAbilityShape::Area);
	TestEqual(TEXT("raggio 1"), Tide->AreaRadius, 1);
	TestEqual(TEXT("Power resta 0: non e' danno"), Tide->Power, 0);
	TestTrue(TEXT("il fallback e' quello del core"), Tide->Def.Fallback == ERTActionFallback::Cancel);
	TestFalse(TEXT("non e' auto-bersaglio: l'area si mira"), Tide->bSelfTarget);
	return true;
}
```

Build, poi `RunTests RefactorTactics.Heroes.Phase.TideDerivesFromHeal` → `Result={Fail}` (oggi `DerivedFromActionId` è `None`).

- [ ] **Step 2: Catalogo.** In `RTHeroCatalogLibrary.cpp:518-522` sostituisci la chiamata a `MakeHeroAction` con:

```cpp
	// #3593: deriva da `Action.Heal` perche' le cure passano da `CollectHealActions`, che raccoglie SOLO le
	// derivate (`IsCoreAction`): costruita da zero finiva fra gli attacchi, dove un'area non cura nessuno.
	// I numeri restano dell'eroe (spec SP5 §2.1, R1): si riscrivono dopo la derivazione, elenco chiuso.
	// `Power` resta 0 — e' il danno letto dal bot — e il fallback resta quello del core.
	URTActionData* CircularTide = MakeHeroActionFromCore(TEXT("Hero.Muiren.CircularTide"), TEXT("Action.Heal"),
		/*Cooldown*/ 2, ERTAbilityShape::Area, /*AreaRadius*/ 1);
	if (!CircularTide)
	{
		checkf(false, TEXT("Action.Heal manca dal catalogo core: CircularTide non si costruisce"));
		return Muiren; // in Shipping `checkf` sparisce: niente dereferenza di un nullptr
	}
	CircularTide->Def.RangeCells = 4;
	CircularTide->RangeCells = 4; // specchio legacy, come `MakeHeroAction` lo scrive (`:88-125`)
	CircularTide->Def.Priority = 60;
	CircularTide->Def.Effects = { FRTActionEffectSpec(ERTActionEffect::Heal, 18) };
	Muiren->Actions.Add(CircularTide); // indice 1: le varianti qui sotto e i test lo indirizzano cosi'
```

Rileggi `MakeHeroAction` (`:88-125`) e riscrivi **ogni** specchio legacy che quella funzione deriva da `RangeCells`, `Priority` o `Effects` (se ne scrive altri oltre a `RangeCells`, aggiungili qui con un commento che li nomina; `Power` no). Aggiorna il commento sopra la riga (`:504-517`) togliendo ciò che la derivazione rende falso. Build; Step 1 verde; `RunTests RefactorTactics.Heroes.Phase` tutto verde (`TideHealsWithoutWetting`, `VariantTradeoff` leggono `Actions[1]`).

- [ ] **Step 3: `DerivedActionsDeclareTheirOrigin`.** In `Tests/RTHeroCatalogTests.cpp`, nella mappa `Atteso` di `DerivedActionsDeclareTheirOrigin` (cerca `Hero.Ivrin.PhaseGuard`), aggiungi la riga `{ TEXT("Hero.Muiren.CircularTide"), TEXT("Action.Heal") },` con il commento `// #3593: la cura ad area passa dalle cure solo se deriva da Action.Heal.`; nel commento sopra la mappa, «**undici** da [D-380]» diventa «**undici** da [D-380], **dodici** da #3593»; nel commento del ramo `else` l'elenco delle proprie perde `CircularTide` e diventa «**sette** abilità: `LinearDischarge`, `Overload`, `Reconfigure`, `FlowReaction`, `InterceptShot`, `PassingBlade`, `Feint`», e la somma «Undici derivate + otto proprie + quattro base = 23» diventa «Dodici derivate + sette proprie + quattro base = 23». `RunTests RefactorTactics.Heroes` verde.

- [ ] **Step 4: Test del bot, rosso.** In `Tests/RTBotPlanningTests.cpp`, accanto a `FRTBotPlanningMissingKnowledgeIsNotOmniscienceTest` (`:267`), con gli stessi helper (`MakeFlatMap`, `MakeFacts`) e la stessa conoscenza piena (`FRTTeamKnowledge Vista` con `VisibleCells` sulla cella del nemico, `:306-335`):

```cpp
/**
 * #3593, spec SP5 R8: un'azione che deriva da `Action.Heal` non e' un candidato d'ATTACCO. Il passo 2 dei
 * candidati (`AddCandidates` da fermo) la escludeva solo per caso — `Power` 0 — e il controllo positivo qui
 * sotto mostra che con un `Power` qualunque la pianificherebbe su un nemico.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotDerivedHealIsNotAnAttackCandidateTest,
	"RefactorTactics.Bot.DerivedHealIsNotAnAttackCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotDerivedHealIsNotAnAttackCandidateTest::RunTest(const FString&)
{
	auto Pianifica = [this](bool bDerivata) -> int32
	{
		URTHexMapAsset* M = MakeFlatMap(4);
		TArray<FRTHexSimUnit> SimUnits;
		SimUnits.Add(FRTHexSimUnit(0, FRTCellId(0, 0, 0), /*budget*/ 2));
		SimUnits.Add(FRTHexSimUnit(1, FRTCellId(1, 0, 0), /*budget*/ 2));
		const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshotOmniscient(M, SimUnits);
		TArray<FRTBotUnitFacts> Facts;
		Facts.Add(MakeFacts(0, /*Team*/ 1, FRTCellId(0, 0, 0), /*bBot*/ true));
		Facts.Add(MakeFacts(1, /*Team*/ 0, FRTCellId(1, 0, 0), /*bBot*/ false));
		URTActionData* Cura = NewObject<URTActionData>();
		Cura->RangeCells = 1;
		Cura->Power = 40; // controllo positivo: senza la derivazione e' un attacco appetibile
		Cura->Def.DerivedFromActionId = bDerivata ? FName(TEXT("Action.Heal")) : NAME_None;
		Facts[0].Abilities.Add(Cura);            // indice 0
		Facts[0].bAbilityUsable.Add(true);
		URTActionData* Colpo = NewObject<URTActionData>(); // indice 1: l'attacco vero, piu' debole della cura
		Colpo->RangeCells = 1;
		Colpo->Power = 20;
		Facts[0].Abilities.Add(Colpo);
		Facts[0].bAbilityUsable.Add(true);
		FRTBotWeights Pesi; Pesi.WKill = 100; Pesi.WDamage = 50; Pesi.WApproach = 5;
		FRTTeamKnowledge Vista; Vista.TeamId = 1; Vista.VisibleCells.Add(FRTCellId(1, 0, 0));
		TMap<int32, FRTTeamKnowledge> Conoscenza; Conoscenza.Add(1, Vista);
		TMap<int32, int32> Inattivita; TMap<int32, int32> UltimoRound;
		const FRTBotPlanningOutcome Esito = URTBotPlanningLibrary::PlanTurn(
			Snap, Facts, Pesi, Conoscenza, Inattivita, UltimoRound, /*TurnNumber*/ 1, /*bRecordAudit*/ false);
		if (Esito.Decisions.Num() != 1) { return -99; }
		TestEqual(TEXT("il nemico adiacente e' il bersaglio in entrambi i casi"), Esito.Decisions[0].PlannedAttackTargetIndex, 1);
		return Esito.Decisions[0].PlannedAbilityIndex;
	};
	TestEqual(TEXT("controllo positivo: non derivata e piu' forte, la pianifica (indice 0)"), Pianifica(false), 0);
	TestEqual(TEXT("derivata da Action.Heal: pianifica l'ALTRA abilita' (indice 1), non la cura"), Pianifica(true), 1);
	return true;
}
```

Se `FRTTeamKnowledge` o la chiave della mappa di conoscenza hanno una forma diversa da quella del test a `:306-335`, copia **quella**. `PlannedAbilityIndex` sta in `FRTBotPlanDecision` (`Bot/RTBotPlanning.h`). Build; `RunTests RefactorTactics.Bot.DerivedHealIsNotAnAttackCandidate` → rosso sull'ultimo asserto.

- [ ] **Step 5: Il passo 2.** In `RTBotPlanningLibrary.cpp:853-857`:

```cpp
		// #3593, spec SP5 R8: una cura (derivata da `Action.Heal`) non e' un attacco — fin qui entrava con
		// `Power` 0 e usciva dal punteggio per caso. L'uso della cura ad area dal bot e' un follow-up.
		static const FName ActionHealId(TEXT("Action.Heal"));
		for (int32 A = 0; A < Bot.NumAbilities(); ++A)
		{
			const URTActionData* Ability = Bot.GetAbility(A);
			if (!Ability || URTCatalogLibrary::IsFastMovement(Ability->Def) || Ability->bSelfTarget
				|| Ability->Def.DerivedFromActionId == ActionHealId || !Bot.CanUseAbility(A)) { continue; }
			AddCandidates(StaySnapshot, A, Ability->RangeCells, Ability->Power, /*bViaDash*/ false, /*bAttacksOnly*/ true);
		}
```

Build; Step 4 verde; `RunTests RefactorTactics.Bot` verde.

- [ ] **Step 6: Commit.** `git add` dei cinque file; `feat(3593): CircularTide deriva da Action.Heal con i numeri dell'eroe; il bot non pianifica una cura come attacco`.

- [ ] **Step 7: Mutazioni (dopo il commit, ripristino con `git checkout --`).** (1) `MakeHeroActionFromCore` → `MakeHeroAction(..., ERTResolutionPhase::Attack, 60, 4, 2, ERTActionFallback::AttackCell, {Heal 18}, Area, 1)` come prima → cade `TideDerivesFromHeal` («deriva da Action.Heal») **e** `DerivedActionsDeclareTheirOrigin`; (2) la riga `CircularTide->Def.RangeCells = 4;` tolta (e lo specchio) → cadono «portata dell'eroe» e `TideHealsWithoutWetting` («portata 4»); (11) la condizione `|| Ability->Def.DerivedFromActionId == ActionHealId` tolta → cade `DerivedHealIsNotAnAttackCandidate` sul secondo asserto. Report con i nomi di tutti i test caduti per ciascuna.

---

### Task 2: Il percorso delle cure impara la forma `Area`

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTTurnManager_Blast.cpp:494-608` (`CollectHealActions`)
- Create: `Source/RefactorTactics/Tests/RTHealAreaTests.cpp`

**Interfaces:**
- Consumes: `ARTUnit` — `PlannedAbilityIndex`, `PlannedAttackTarget`, `PlannedAttackCell`, `bAttackTargetsCell` (`Unit/RTUnit.h:306-397`: `DeclareAttackOnCell`, `DeclareAttackOnUnit`, `ClearPlannedAttack`), `ActiveVariantId` (`:332`), `GetAbility`, `CanUseAbility`, `IsAlive`, `Cell`, `TeamId`, `StableUnitId`, `Health`/`MaxHealth`; `URTActionData::FindVariant(FName)`, `FRTAbilityVariant::Effects`; `FRTBlastContext::AddHeal(Actor, Target, Amount, SourceCell, Def)` (`Turn/RTBlastContext.h:237`), `Ctx.MarkAbilitySpent(Unit, Idx)`, `Ctx.Units`; `MakeSupportFallback(Autore, Bersaglio, Def, Motivo)` (`RTTurnManager_Blast.cpp:241-255`); `EmitAbilityActivated(Source, Phase, ActionId, BaseActionId, TargetStableUnitId, AimCell, Shape)` (`Turn/RTTurnManager.cpp:314`); `URTHexLibrary::HexDistance`.
- Produces: per un'azione di cura con `Shape == Area`: N voci `Combat/Healed` (una per compagna nel raggio, cadaveri compresi che diventano `Fallback/TargetDead` in `ApplyPlannedHeals`), o una `Fallback/Cancelled` con `Amount == NoEffect` e `TgtCell == centro` se nessuno della squadra è nel raggio; un `AbilityActivated` con `Shape == Area`, `AimCell == centro`, `TargetStableUnitId` = bersaglio pianificato o 0; cooldown pagato in entrambi i casi.

- [ ] **Step 1: La fixture e i tre test, rossi.** Crea `Tests/RTHealAreaTests.cpp` (CRLF). Riusa `Tests/RTWorldFixtures.h` (`RTWorldFixtures::MakeWorld`, `DestroyWorld`, `PlayOneTurn`: è ciò che `Tests/RTAbilityActivatedTests.cpp` usa — leggi lì come si spawnano mappa e unità) invece di copiare gli helper di `RTEquipmentTests.cpp`; dove serve un helper nuovo mettilo in un namespace **proprio** `RTHealAreaTestsInternal` (due namespace anonimi con lo stesso nome di funzione collidono in unity build). Nel codice dei test qui sotto `MakeHealWorld`/`SpawnHealMap`/`SpawnHealUnit`/`RunHealTurn`/`DestroyHealWorld` sono **segnaposto da sostituire** con gli equivalenti di `RTWorldFixtures` (o con wrapper sottili su di essi). ⚠️ **Ogni mondo ha un `Nemico` di squadra 1 lontano e senza intenti**: con una sola squadra la partita finisce per eliminazione. Aggiungi:

```cpp
	/** Pianifica CircularTide di `Curatrice` su una cella. L'indice 1 e' quello del catalogo (Task 1). */
	int32 PianificaTideSuCella(ARTUnit* Curatrice, const FRTCellId& Centro)
	{
		const int32 Idx = 1;
		Curatrice->PlannedAbilityIndex = Idx;
		Curatrice->PlannedCell = Curatrice->Cell;
		// Il setter della coppia cella/flag (`Unit/RTUnit.h`, `DeclareAttackOnCell`): scrivere i due campi a mano
		// salterebbe l'invariante di `#2884`.
		Curatrice->DeclareAttackOnCell(Centro);
		return Idx;
	}
	int32 ConteggioVoci(const ARTTurnManager* TM, ERTLogCategory Cat, uint8 Esito)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog()) { if (E.Category == Cat && E.Outcome == Esito) { ++N; } }
		return N;
	}
	const FRTResolvedEvent* AttivazioneDi(const ARTTurnManager* TM, const ARTUnit* Sorgente)
	{
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.SourceStableUnitId == Sorgente->StableUnitId) { return &Ev; }
		}
		return nullptr;
	}
```

Poi i tre test:

```cpp
/**
 * #3593, spec SP5 §2.1 punti 1-6: la cura ad area cura OGNI compagna nel raggio, chi cura compresa, con una
 * voce `Healed` ciascuna; il nemico nel raggio e l'alleata fuori non cambiano; la compagna morta produce la
 * voce `TargetDead` che `ApplyPlannedHeals` gia' scrive per un bersaglio morto; un solo `AbilityActivated`,
 * di forma `Area`, sul centro; il cooldown e' pagato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTideHealsAlliesInAreaTest,
	"RefactorTactics.Heroes.TideHealsAlliesInArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTideHealsAlliesInAreaTest::RunTest(const FString&)
{
	UWorld* World = MakeHealWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	SpawnHealMap(World, 6);
	// Centro (1,0): a distanza 1 stanno (0,0) chi cura, (2,0), (1,-1) le compagne, (1,1) il nemico, (0,1) la morta;
	// (4,0) e' a distanza 3, fuori.
	ARTUnit* Curatrice = SpawnHealUnit(World, 0, FRTCellId(0, 0, 0), URTHeroCatalogLibrary::MakeMuiren());
	ARTUnit* A1 = SpawnHealUnit(World, 0, FRTCellId(2, 0, 0), URTHeroCatalogLibrary::MakeAevik());
	ARTUnit* A2 = SpawnHealUnit(World, 0, FRTCellId(1, -1, 0), URTHeroCatalogLibrary::MakeBranth());
	ARTUnit* Lontana = SpawnHealUnit(World, 0, FRTCellId(4, 0, 0), URTHeroCatalogLibrary::MakeIvrin());
	ARTUnit* Nemico = SpawnHealUnit(World, 1, FRTCellId(1, 1, 0), URTHeroCatalogLibrary::MakeIvrin());
	ARTUnit* Morta = SpawnHealUnit(World, 0, FRTCellId(0, 1, 0), URTHeroCatalogLibrary::MakeBranth());
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Curatrice || !A1 || !A2 || !Lontana || !Nemico || !Morta || !TM) { DestroyHealWorld(World); return false; }
	Curatrice->Health -= 10; A1->Health -= 40; A2->Health -= 40; Lontana->Health -= 40; Nemico->Health -= 40;
	Morta->Health = 0;
	const int32 PrimaC = Curatrice->Health, PrimaA1 = A1->Health, PrimaA2 = A2->Health, PrimaL = Lontana->Health, PrimaN = Nemico->Health;
	const int32 Idx = PianificaTideSuCella(Curatrice, FRTCellId(1, 0, 0));
	RunHealTurn(TM);
	TestEqual(TEXT("compagna A1 curata di 18"), A1->Health - PrimaA1, 18);
	TestEqual(TEXT("compagna A2 curata di 18"), A2->Health - PrimaA2, 18);
	TestEqual(TEXT("chi cura e' nel raggio: +10, tetto a MaxHealth"), Curatrice->Health - PrimaC, 10);
	TestEqual(TEXT("fuori raggio: invariata"), Lontana->Health, PrimaL);
	TestEqual(TEXT("il nemico nel raggio non guarisce"), Nemico->Health, PrimaN);
	TestEqual(TEXT("la morta resta morta"), Morta->Health, 0);
	TestEqual(TEXT("tre voci Healed"), ConteggioVoci(TM, ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Healed)), 3);
	const int32 Fallback = ConteggioVoci(TM, ERTLogCategory::Fallback, static_cast<uint8>(ERTFallbackOutcome::Cancelled));
	TestEqual(TEXT("una voce di fallback: la morta"), Fallback, 1);
	bool bMortaRegistrata = false;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Fallback && E.Amount == static_cast<int32>(ERTActionInvalidReason::TargetDead)) { bMortaRegistrata = true; }
	}
	TestTrue(TEXT("e dice TargetDead"), bMortaRegistrata);
	int32 Attivazioni = 0; const FRTResolvedEvent* Beat = nullptr;
	for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
	{
		if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.SourceStableUnitId == Curatrice->StableUnitId) { ++Attivazioni; Beat = &Ev; }
	}
	TestEqual(TEXT("un solo AbilityActivated"), Attivazioni, 1);
	if (Beat)
	{
		TestTrue(TEXT("di forma Area"), Beat->Shape == ERTAbilityShape::Area);
		TestTrue(TEXT("sul centro"), Beat->AimCell == FRTCellId(1, 0, 0));
		TestEqual(TEXT("nessun bersaglio-unita'"), Beat->TargetStableUnitId, 0);
	}
	TestFalse(TEXT("il cooldown e' pagato"), Curatrice->CanUseAbility(Idx));
	DestroyHealWorld(World);
	return true;
}
```

Secondo caso nello stesso test (Review Focus 1 e 2), **dopo** `DestroyHealWorld` del primo, con un mondo nuovo (con il suo `Nemico` lontano): chi cura a (0,0) a −10 e nessun'altra compagna, centro (0,0) → +10, **una** voce `Healed`, nessun fallback; poi un terzo mondo (con `Nemico`) con `Curatrice->DeclareAttackOnUnit(A1)` (A1 alleata a (2,0), a −40, nessuna cella) → A1 +18 e il `Beat->TargetStableUnitId == A1->StableUnitId`, `AimCell == A1->Cell`.

```cpp
/** #3593, spec §2.1 punto 6: l'amount viene dalla variante attiva (`Healing` 24), altrimenti da `Def`. Review Focus 4: `Impact` cura 10 e non spinge. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTideHealsVariantAmountTest,
	"RefactorTactics.Heroes.TideHealsVariantAmount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTideHealsVariantAmountTest::RunTest(const FString&)
{
	auto Corsa = [&](const TCHAR* Variante, int32 Atteso)
	{
		UWorld* World = MakeHealWorld(); SpawnHealMap(World, 6);
		ARTUnit* Curatrice = SpawnHealUnit(World, 0, FRTCellId(0, 0, 0), URTHeroCatalogLibrary::MakeMuiren());
		ARTUnit* A1 = SpawnHealUnit(World, 0, FRTCellId(2, 0, 0), URTHeroCatalogLibrary::MakeAevik());
		ARTUnit* Nemico = SpawnHealUnit(World, 1, FRTCellId(3, 0, 0), URTHeroCatalogLibrary::MakeIvrin());
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		A1->Health -= 40; const int32 Prima = A1->Health; const FRTCellId CellaNemico = Nemico->Cell;
		Curatrice->ActiveVariantId = FName(Variante);
		PianificaTideSuCella(Curatrice, FRTCellId(2, 0, 0));
		RunHealTurn(TM);
		TestEqual(*FString::Printf(TEXT("%s cura %d"), Variante, Atteso), A1->Health - Prima, Atteso);
		TestTrue(TEXT("il nemico nel raggio non si sposta (R3: la spinta di Impact non passa dalle cure)"), Nemico->Cell == CellaNemico);
		DestroyHealWorld(World);
	};
	Corsa(TEXT("Hero.Muiren.CircularTide.Healing"), 24);
	Corsa(TEXT("Hero.Muiren.CircularTide.Impact"), 10);
	Corsa(TEXT(""), 18);
	return true;
}

/** #3593, spec §2.1 punto 4 (R2 ribaltato): un'area senza nessuno della squadra PARTE — cooldown pagato, attivazione emessa — e lascia una sola voce `NoEffect` sul centro. Review Focus 3: fuori portata invece NON parte. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTideOnEmptyAreaStillStartsTest,
	"RefactorTactics.Heroes.TideOnEmptyAreaStillStarts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTideOnEmptyAreaStillStartsTest::RunTest(const FString&)
{
	{
		UWorld* World = MakeHealWorld(); SpawnHealMap(World, 6);
		ARTUnit* Curatrice = SpawnHealUnit(World, 0, FRTCellId(0, 0, 0), URTHeroCatalogLibrary::MakeMuiren());
		ARTUnit* Nemico = SpawnHealUnit(World, 1, FRTCellId(-3, 0, 0), URTHeroCatalogLibrary::MakeIvrin());
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		const FRTCellId Centro(4, -4, 0); // distanza 4 da (0,0): dentro la portata, nessuno intorno
		const int32 Idx = PianificaTideSuCella(Curatrice, Centro);
		RunHealTurn(TM);
		TestEqual(TEXT("nessuna Healed"), ConteggioVoci(TM, ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Healed)), 0);
		int32 Vuote = 0; bool bSulCentro = false;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Fallback && E.Amount == static_cast<int32>(ERTActionInvalidReason::NoEffect)) { ++Vuote; bSulCentro = (E.TgtCell == Centro); }
		}
		TestEqual(TEXT("una voce NoEffect"), Vuote, 1);
		TestTrue(TEXT("con TgtCell = centro, non la cella di chi cura"), bSulCentro);
		TestFalse(TEXT("il cooldown e' pagato: l'azione e' partita"), Curatrice->CanUseAbility(Idx));
		TestNotNull(TEXT("l'attivazione e' emessa"), AttivazioneDi(TM, Curatrice));
		DestroyHealWorld(World);
	}
	{
		UWorld* World = MakeHealWorld(); SpawnHealMap(World, 6);
		ARTUnit* Curatrice = SpawnHealUnit(World, 0, FRTCellId(0, 0, 0), URTHeroCatalogLibrary::MakeMuiren());
		ARTUnit* Nemico = SpawnHealUnit(World, 1, FRTCellId(-3, 0, 0), URTHeroCatalogLibrary::MakeIvrin());
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		const int32 Idx = PianificaTideSuCella(Curatrice, FRTCellId(5, -5, 0)); // distanza 5: fuori portata 4
		RunHealTurn(TM);
		int32 FuoriPortata = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Fallback && E.Amount == static_cast<int32>(ERTActionInvalidReason::OutOfRange)) { ++FuoriPortata; }
		}
		TestEqual(TEXT("fuori portata: una voce OutOfRange"), FuoriPortata, 1);
		TestTrue(TEXT("e il cooldown NON e' pagato: l'azione non e' partita"), Curatrice->CanUseAbility(Idx));
		DestroyHealWorld(World);
	}
	return true;
}
```

`Nemico` serve perché un mondo con una sola squadra può chiudere la partita per eliminazione. Build; `RunTests RefactorTactics.Heroes.Tide` → i tre test rossi (oggi: nessuna cura, nessuna voce).

- [ ] **Step 2: `CollectHealActions`.** ⚠️ **Modifica in sede, non sostituzione del corpo**: il corpo attuale (`:494-608`) porta i commenti D-196, D-197, D-200, #1437, #1445, #1451 e «Niente `AddLogEvent`», che restano tutti; si inseriscono o cambiano **solo** le righe che il blocco qui sotto mostra diverse, e il commento sul bersaglio (`:522-523`) si completa con il caso `Area`. Il blocco è il risultato atteso senza i commenti preesistenti:

```cpp
void ARTTurnManager::CollectHealActions(FRTBlastContext& Ctx)
{
	for (int32 i = 0; i < Ctx.Units.Num(); ++i)
	{
		ARTUnit* Unit = Ctx.Units[i];
		const int32 HealIdx = Unit->PlannedAbilityIndex;
		const URTActionData* Heal = Unit->GetAbility(HealIdx);
		if (!Unit->IsAlive()) { continue; }
		if (!Heal || !IsCoreAction(Heal->Def, ActionHeal) || !Unit->CanUseAbility(HealIdx)) { continue; }

		// #3593: centro e bersaglio si leggono PRIMA di `ClearPlannedAttack`, che azzera anche `bAttackTargetsCell`.
		const bool bArea = Heal->Shape == ERTAbilityShape::Area;
		ARTUnit* HealTarget = Unit->PlannedAttackTarget ? Unit->PlannedAttackTarget.Get() : Unit;
		// Solo l'AREA legge la cella dichiarata: il ramo `Single` resta com'era (bersaglio o se', `AimCell` = la sua cella).
		const FRTCellId Centro = (bArea && Unit->bAttackTargetsCell) ? Unit->PlannedAttackCell : HealTarget->Cell;
		const int32 BersaglioStableId = (bArea && !Unit->PlannedAttackTarget) ? 0 : HealTarget->StableUnitId;
		Unit->PlannedAbilityIndex = INDEX_NONE;
		Unit->ClearPlannedAttack(); // ENTRAMBE le forme (`#2884`): questo ramo esce con `continue`

		if (URTHexLibrary::HexDistance(Unit->Cell, Centro) > Heal->Def.RangeCells)
		{
			FRTTurnLogEntry CuraMancata = MakeSupportFallback(Unit, bArea ? nullptr : HealTarget, Heal->Def, ERTActionInvalidReason::OutOfRange);
			if (bArea) { CuraMancata.TgtCell = Centro; }
			AppendLogEntry(CuraMancata, Unit);
			continue;
		}

		Ctx.MarkAbilitySpent(Unit, HealIdx);
		EmitAbilityActivated(Unit, ERTMatchPhase::Blast, Heal->Def.ActionId, Heal->Def.BaseActionId,
			BersaglioStableId, Centro, bArea ? ERTAbilityShape::Area : ERTAbilityShape::Single);

		// #3593, spec SP5 §2.1 punto 6: l'amount della VARIANTE attiva, se ne dichiara uno; altrimenti di `Def`.
		int32 Amount = 0;
		if (const FRTAbilityVariant* Variante = Heal->FindVariant(Unit->ActiveVariantId))
		{
			for (const FRTActionEffectSpec& Spec : Variante->Effects) { if (Spec.Effect == ERTActionEffect::Heal) { Amount = Spec.Amount; break; } }
		}
		if (Amount <= 0)
		{
			for (const FRTActionEffectSpec& Spec : Heal->Def.Effects) { if (Spec.Effect == ERTActionEffect::Heal) { Amount = Spec.Amount; break; } }
		}
		if (Amount <= 0)
		{
			FRTTurnLogEntry CuraVuota = MakeSupportFallback(Unit, bArea ? nullptr : HealTarget, Heal->Def, ERTActionInvalidReason::NoEffect);
			if (bArea) { CuraVuota.TgtCell = Centro; }
			AppendLogEntry(CuraVuota, Unit);
			continue;
		}

		if (!bArea)
		{
			Ctx.AddHeal(Unit, HealTarget, Amount, Unit->Cell, Heal->Def);
			continue;
		}

		// #3593: ogni compagna nel raggio, chi cura compresa, nell'ordine canonico di `Ctx.Units` (cella per prima,
		// `:281`): niente secondo ordinamento. Le morte entrano lo stesso: `ApplyPlannedHeals` scrive `TargetDead`.
		int32 Destinatarie = 0;
		for (ARTUnit* Compagna : Ctx.Units)
		{
			if (!Compagna || Compagna->TeamId != Unit->TeamId) { continue; }
			if (URTHexLibrary::HexDistance(Centro, Compagna->Cell) > Heal->AreaRadius) { continue; }
			Ctx.AddHeal(Unit, Compagna, Amount, Unit->Cell, Heal->Def);
			++Destinatarie;
		}
		if (Destinatarie == 0)
		{
			// R2 ribaltato (spec SP5): l'azione e' PARTITA — cooldown e attivazione sopra — e non ha trovato nessuno.
			// `NoEffect`, non `TargetGone`, che in `ApplyPlannedHeals` dice «distrutta fra raccolta e applicazione».
			FRTTurnLogEntry Vuota = MakeSupportFallback(Unit, nullptr, Heal->Def, ERTActionInvalidReason::NoEffect);
			Vuota.TgtCell = Centro;
			AppendLogEntry(Vuota, Unit);
		}
	}
}
```

⚠️ Verifica su `Unit/RTUnit.h` i nomi esatti di `bAttackTargetsCell`/`PlannedAttackCell` e che `PlannedAttackTarget` sia un `TObjectPtr` (`.Get()`); verifica che `FRTResolvedEvent::Shape` venga scritto da `EmitAbilityActivated` dal parametro `Shape` (`Turn/RTTurnManager.cpp:314-360`). Se `Heal->Shape`/`AreaRadius` vivono su `Def` e non sui campi legacy, usa `Def`. Build; i tre test verdi; `RunTests RefactorTactics.Equipment.MedkitHealsInMatch+RefactorTactics.Actions+RefactorTactics.Heroes` verde.

- [ ] **Step 3: Commit.** `feat(3593): il percorso delle cure impara la forma Area — una cura per compagna nel raggio, a vuoto parte lo stesso`.

- [ ] **Step 4: Mutazioni.** (3) il filtro `Compagna->TeamId != Unit->TeamId` tolto → cade «il nemico nel raggio non guarisce»; (4) `> Heal->AreaRadius` → `>= ` → cade «compagna A2» o «A1» (chi sta sul bordo); (5) `if (!Compagna || !Compagna->IsAlive() || …)` → cade «una voce di fallback: la morta»; (6) `ERTAbilityShape::Single` passato sempre → cade «di forma Area»; (7) la riga `Ctx.MarkAbilitySpent(Unit, HealIdx);` tolta e sostituita, dopo il ciclo `for (ARTUnit* Compagna …)`, da `if (Destinatarie > 0) { Ctx.MarkAbilitySpent(Unit, HealIdx); }` (cooldown non pagato a vuoto; il ramo `Single` resta senza cooldown in questa mutazione, e lo dice il report) → cade «il cooldown e' pagato» in `TideOnEmptyAreaStillStarts`; (8) la lettura della variante tolta → cade `TideHealsVariantAmount` su `Healing`; (9) il blocco `if (Destinatarie == 0)` tolto → cade «una voce NoEffect»; (10) `Vuota.TgtCell = Centro;` tolta → cade «con TgtCell = centro». Report con i nomi di tutti i test caduti.

---

### Task 3: La riga FX di `CircularTide` non promette un colpo (F4)

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTPresentationBinding.cpp:498`
- Modify: `Source/RefactorTactics/Tests/RTAbilityFxProfileTests.cpp:177`
- Modify: `docs/superpowers/specs/2026-10-08-profilo-fx-per-abilita-design.md:319` (la riga della tabella) e `:792` (la nota)

- [ ] **Step 1: Test rosso.** In `RTAbilityFxProfileTests.cpp:177`: `Attesi.Add(TEXT("Hero.Muiren.CircularTide"), FxP(FxA::Pulse, FxT::None, FxI::None, FxF::None));` con il commento `// #3593: una cura non ha un colpo, quindi ne' Marker ne' onda (spec SP5 F4, R7)`. Build; `RunTests RefactorTactics.Fx.DeclaredOverridesMatchTheProposal` → rosso.
- [ ] **Step 2: Tabella.** `RTPresentationBinding.cpp:498` → `MakeFxProfile(A::Pulse, T::None, I::None, F::None)` con lo stesso commento. Build; verde; `RunTests RefactorTactics.Fx+RefactorTactics.Playback` verde.
- [ ] **Step 3: Spec FX.** A `:319` la riga della tabella riceve in coda `⌫ #3593: `Pulse · None · None · None` — la cura passa dalle cure e non emette `Attack`: `Marker` e `AreaPulse` erano dato morto (spec SP5 F4, R7)`; a `:792` la nota `➕ rev. Se …` riceve `✅ Deciso in #3593: nessun consumatore, riga riscritta`. Niente altro.
- [ ] **Step 4: Commit.** `fix(3593): la riga FX di CircularTide non promette un colpo — Pulse/None/None/None, gemella nel test, nota nella spec del profilo`.
- [ ] **Step 5: Mutazione (20).** La gemella del test riportata a `Marker/AreaPulse` → cade `DeclaredOverridesMatchTheProposal`.

---

### Task 4: Scenario, documenti, generatore HUD

**Files:**
- Modify: `Scenarios/Visual/Ability/ClipMuiren.json` (`expect` di `A1` → 78; `_nota_curare`)
- Modify: `docs/technical/test-manuali-pie.md` (cella `PIE-CLIP-ABILITA`: nota in coda)
- Modify: `docs/roadmap/editor-sessions.yaml` (`U70`: la riconvocazione dichiarata)
- Modify: `docs/superpowers/specs/2026-10-08-clip-visibili-e-cura-ad-area-design.md` (statuto «Implementato»)
- Possibly modify: ciò che il generatore HUD rigenera (F11) — **solo** se il suo diff è spiegabile da questa derivazione

- [ ] **Step 1: Scenario.** In `ClipMuiren.json`: `{ "type": "UnitHpEquals", "unit": "A1", "value": 78 }`; `_nota_curare` perde il blocco `⚠️ **Aevik resta a 60** … l'attesa diventa 78 (60 + 18).` e dice: `Aevik parte a 60 HP perche' una cura su un'unita' piena non si legge: 60 + 18 = 78 e' l'attesa, da #3593 (prima la cura non arrivava: l'effetto Heal di un'azione non derivata da Action.Heal non aveva un consumatore).` File LF. `node tools/radar/scenario-notes.ts --check` verde. `RunTests RefactorTactics.Scenario.EveryShippedScenarioRuns` → `Visual.Ability.ClipMuiren: PASS`.
- [ ] **Step 2: Registro PIE.** Cella di stato di `PIE-CLIP-ABILITA` (riga `| **PIE-CLIP-ABILITA** |`, ultima cella): **in coda**, prima di `⏮️ Stato precedente`, `➕ **Dopo [#3593](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3593)** (la cura ad area): in `Visual.Ability.ClipMuiren` la barra di Aevik sale di 18 e al cast di `CircularTide` si vede il `Pulse` (riga FX riscritta); da giudicare nella riconvocazione di `U70` insieme ad Aevik su #3595.` Nessun glifo di stato cambia. `node tools/radar/doc-coherence.ts --check`: A1 invariato; A4 resta rosso su `PIE-CLIP-ABILITA` per #3590 **e** ora per #3593 (entrambe chiuse al merge): è il segnale previsto, si spegne con il rigiudizio — scriverlo nel report, non «aggiustarlo».
- [ ] **Step 3: Seduta.** In `editor-sessions.yaml`, `U70`, in coda a `notes`: `Da riconvocare dopo #3595 (Aevik: clip piene sugli attacchi) e #3593 (Muiren: la cura arriva e il cast mostra il Pulse): scene 1-3 della spec SP5 §5.2, una seduta sola, Editor ricompilato sul clone principale.`
- [ ] **Step 4: Generatore HUD (F11).** Da Git Bash: `PATH="/c/Program Files/GTK3-Runtime Win64/bin:$PATH" python tools/hud-assets/generate_hud_assets.py` (guida `docs/technical/runbooks/guida-catalogo-icone.md` §1); confronta `Chiavi richieste` con una corsa sul commit di partenza (checkout temporaneo del solo `RTHeroCatalogLibrary.cpp` di `origin/main`, oppure la corsa fatta nel Task 0 se la aggiungi lì). Se `CircularTide` esce dai glifi richiesti per il ripiego a `Action.Heal`, è ciò che `MakeActionIconFallbackId` farà davvero: **accettato**, scritto nel report; se il diff degli asset rigenerati tocca solo file spiegabili da questo, commettili; altrimenti non commettere e scrivi cosa è cambiato. Esegui anche il passo 2 della guida (il commandlet delle icone) e allega l'esito.
- [ ] **Step 5: Spec SP5, due righe.** In §4 «Area di cura senza alleati: una voce `Fallback/TargetGone`» diventa `Fallback/NoEffect` (è R2 ribaltato, §2.1); in §6 si aggiunge «La variante `Impact` promette nel suo `Tradeoff` (`RTHeroCatalogLibrary.cpp`, «applica Push 1 ai nemici») e in `RTActionDescriptions.cpp` («cura gli alleati o colpisce») una spinta che R3 non produce: testo da riallineare nel follow-up della spinta curativa, non qui.»
- [ ] **Step 6: Statuto della spec.** Blocco `✅ **Implementato il 2026-10-08**` con i commit dei Task 1–4 e i gate eseguiti (come nelle spec sorelle).
- [ ] **Step 7: Commit.** `docs(3593): ClipMuiren a 78, nota in coda alla voce PIE, riconvocazione di U70 dichiarata, statuto`.

---

### Task 5: Chiusura — suite completa «dopo», diff, PR, review, merge, issue

- [ ] **Step 1: Suite completa «dopo».** Stesso comando del Task 0, log `t5-suite-dopo.log`; `nomi-dopo.txt`; `diff nomi-prima.txt nomi-dopo.txt` deve mostrare **solo** i test nuovi (`TideDerivesFromHeal`, `TideHealsAlliesInArea`, `TideHealsVariantAmount`, `TideOnEmptyAreaStillStarts`, `DerivedHealIsNotAnAttackCandidate`); i `Result={Fail}` sono il solo `Packaging.RequiredActionClipsAreCooked`; `Ensure condition failed` 0.
- [ ] **Step 2: Merge di `origin/main`** nel branch se è avanzato; se tocca `RTTurnManager_Blast.cpp`, `RTHeroCatalogLibrary.cpp`, `RTPresentationBinding.cpp` o i test di questo piano, la suite «dopo» si ripete sull'albero unito.
- [ ] **Step 3: PR** su `main` con `--body-file`: cosa cambia, le decisioni (D1, R1, R2 ribaltato, R7, R8, F4), i gate con i conteggi di **questa** corsa, «PIE: NOT RUN — riconvocazione di U70 a seguire». Review con un subagente sul diff; findings applicati; commento con l'esito.
- [ ] **Step 4: Merge** guardato (`git merge-base --is-ancestor origin/main <branch>` subito prima), `--delete-branch`; ff del clone principale; chiusura di #3593 con il DoD nel commento; `git worktree remove` **solo dopo** la seduta U70 (il worktree serve a nessuno: la seduta gira sul clone principale) — rimuovere qui.
- [ ] **Step 5: U70** — fuori dal piano: avviso «prendo il motore» alla sessione parallela, ff e build del clone principale (blob di `RTTurnManager_Blast.cpp`, `RTHeroCatalogLibrary.cpp`, `RTPresentationBinding.cpp` compilati), Editor con `-abslog`, scene 1–3 della spec §5.2, verdetti proposti all'autore con i fotogrammi, cella e seduta aggiornate in una PR docs, «libero».

## Self-review del piano (eseguito in scrittura)

- **Copertura della spec**: §2.1 punti 1–6 → Task 2; R1/F5 → Task 1; R2 ribaltato → Task 2 (`TideOnEmptyAreaStillStarts`); R7/F4 → Task 3; R8 → Task 1; F8 (centro prima dell'azzeramento, `TargetStableUnitId`) → Task 2 Step 2; F11 → Task 4 Step 4; §2.5 → Task 4; §5.1 righe nuove → Task 1–3; §5.2 → Task 5 Step 5.
- **Placeholder**: nessun «TBD»; `DeclareAttackOnCell`/`DeclareAttackOnUnit` (`Unit/RTUnit.h:384-397`), `Shape`/`AreaRadius` legacy su `URTActionData` (`Ability/RTActionData.h:128-132`), `FindVariant` (`:166`) e `ResolvedTimelineForTest` (`Turn/RTTurnManager.h:634`) sono stati letti.
- **Tipi**: `ERTActionInvalidReason::{OutOfRange,NoEffect,TargetDead}` esistono (`Turn/RTActionFallbackLibrary.h`); `ERTCombatOutcome::Healed` e `ERTFallbackOutcome::Cancelled` sono usati da `ApplyPlannedHeals`; `FRTResolvedEvent::{Shape,AimCell,TargetStableUnitId,SourceStableUnitId}` esistono (`Turn/RTResolvedEvent.h`); `TM->ResolvedTimelineForTest()` e `TM->GetTurnLog()` esistono.
- **Review Focus**: i cinque casi hanno un test nominato nel task che li possiede.
