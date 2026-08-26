# Piano — Ciò che il turno scarta lo dichiara chi lo scarta

> ⛔ **Eseguito.** Questo piano è stato completato sul branch `feat/605-validatore-lato-giocatore`; le sue
> decisioni sono recepite in [D-194](../decisions/RT_PDR_00_Decision_Log.md) e in
> [`spec-turnlog.md`](architecture/spec-turnlog.md), che sono gli **owner** correnti. Il documento resta per
> la provenienza, non è una spec viva: non seguirlo come autorità per lavoro futuro.

> **Per esecutori agentici:** SUB-SKILL RICHIESTA — usa `superpowers:subagent-driven-development`
> (consigliata) o `superpowers:executing-plans` per eseguire questo piano task per task. Gli step usano
> checkbox (`- [ ]`) per il tracciamento.

**Obiettivo:** chiudere le due righe aperte del DoD di
[#605](https://github.com/DegrassiAaron/refactor-tactics-main/issues/605) — un punto solo che risponde
LEGALE / ILLEGALE prima del commit, e una traccia nel TurnLog per ciò che la risoluzione scarta — con il
formato deciso **prima** di scrivere il codice, invece che aggiustato a rimbalzo.

**Architettura:** due meccanismi separati che restano separati. Al **lock-in** `ValidatePlansAtLockIn`
registra la contraddizione nel **combat log** e non blocca (D-194). In **risoluzione** `ResolveDash` scrive
nel TurnLog ciò che scarta davvero: una voce `Move`/`SupersededByDash` per il movimento e una voce
`Fallback`/`Cancelled` per la principale, entrambe chiavate sulla cella di **partenza**.

**Stack:** UE 5.8.1 · C++ · automation test `WITH_DEV_AUTOMATION_TESTS` · nessun GAS.

**Spec:** questo documento. I difetti che lo motivano sono misurati nel
[commento di chiusura della PR #1400](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1400#issuecomment-5422755579);
le tre decisioni di design sono nella sezione qui sotto.

---

## Decisioni che questo piano implementa

Prese il 2026-08-26, dopo che quattro cicli di code review su #1400 hanno mostrato che correggerle a
rimbalzo produceva scelte in contraddizione fra loro.

**D-a — `SrcCell` è la cella di PARTENZA.** `BuildMoveLog` (`RTHexSimLibrary.cpp:804`) la dichiara
*«chiave stabile dell'unita' nel turno»*, e `FilterTracesByEmitter` (`RTScenarioRunner.cpp:219`) ci filtra
sopra con `ExcludedSources.Contains(Entry.SrcCell)` per confrontare le varianti a informazione nascosta.
Scriverci la cella d'arrivo — che è ciò che #1400 faceva — fa sopravvivere al filtro la traccia di una
variante e manda rosso un confronto *perché la variante ha funzionato*. Con la partenza, la coppia
(`SrcCell`, `TgtCell`) descrive **una rotta**: quella che non si è percorsa.

**D-b — il messaggio al giocatore nomina ENTRAMBE le azioni in conflitto.** `Verdict.OffendingActionId` è
l'azione che l'ordine canonico del validatore incontra per seconda: per il caso scatto + movimento è
`Hero.Riktor.Ram`, che invece **esegue**. Nominarla è l'errore che #1400 argomenta per pagine e poi commette
nell'unica superficie che il giocatore legge. Nominarle entrambe è vero indipendentemente da chi vince in
risoluzione, e non costringe il validatore a sapere come risolve il resolver.

**D-c — lo scarto si dichiara per ENTRAMBI gli slot.** Una mobilità `MovementAndMain` azzera
`PlannedAbilityIndex` e `PlannedAttackTarget` in silenzio, trentacinque righe sotto la voce del movimento.
Le decisioni sul formato sono le stesse per le due metà: farle una volta sola evita di riaprire un formato
serializzato una seconda volta.

## Vincoli globali

- **UE 5.8.1.** Non inventare API: verifica le firme realmente presenti.
- **Il TurnLog non è un log**: è serializzato, ordinato canonicamente (`EntryLess`) e riprodotto. Ogni campo
  di una voce nuova entra nell'ordine e nell'hash.
- **`TgtCell` va sempre assegnato**, almeno `= SrcCell`: il default è `(0,0,0)`, che è una cella vera.
- **`Priority` si legge dal catalogo** (`URTCatalogLibrary::FindCoreAction`), mai hardcodata: è chiave di
  `EntryLess` ed è serializzata in v7. `FindCoreAction` **ricostruisce** l'array dei ~30 `FRTActionDef` a
  ogni chiamata → `static const`, mai dentro un loop.
- **Valori di enum solo IN CODA**: viaggiano come `uint8` nel formato serializzato.
- **Niente `SetActorLocation` / `ApplyDamage` / `if (IsTest)`** nei test: si passa dal gameplay reale.
- **Niente `Delay`, montage, `Tick` o `DeltaTime`** per decidere sequencing.
- **Nessun nuovo `D-nnn` in questo piano.** `D-194` esiste già su questo branch e va **modificata**, non
  affiancata. L'ID si riverifica prima del merge: `git fetch --prune origin && gh pr list --state open`.
- **Lingua**: commenti e documenti in italiano; identificatori in inglese. Nei sorgenti C++ gli accenti si
  scrivono con l'apostrofo (`unita'`, `e'`, `cio'`), come nel codice circostante.

## Comandi di questa macchina

```bash
# Build editor (~30 s incrementale)
"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development \
  -Project="D:\Repositories\refactor-tactict-dev\RefactorTactics.uproject" -WaitMutex

# Test mirati
"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  "D:\Repositories\refactor-tactict-dev\RefactorTactics.uproject" \
  -ExecCmds="Automation RunTests RefactorTactics.PlayerInteraction;Quit" \
  -unattended -nopause -nosplash -nullrhi -NoLiveCoding -log > /dev/null 2>&1

# Esito: NON dedurre «verde» da zero Fail — conta Found contro Test Completed
LOG=Saved/Logs/RefactorTactics.log
grep -oE 'Found [0-9]+ automation tests' "$LOG" | tail -1
grep -c 'Test Completed' "$LOG"
grep -c 'Result={Fail}' "$LOG"
```

⚠️ Prima di lanciare: `Get-Process -Name UnrealEditor-Cmd` deve essere **vuoto**. Una seconda istanza uccide
la prima e il log troncato sembra verde.

## Struttura dei file

| File | Responsabilità in questo piano |
|---|---|
| `Source/RefactorTactics/Turn/RTPlanValidationLibrary.h` | `FRTPlanValidation` guadagna `HolderActionId`: chi teneva già lo slot |
| `Source/RefactorTactics/Turn/RTPlanValidationLibrary.cpp` | `Reject` e il loop degli slot tracciano il detentore per slot |
| `Source/RefactorTactics/Turn/RTTurnLogLibrary.h` / `.cpp` | `DescribeInvalidReason` diventa pubblica e condivisa; `DescribeEntry` mostra la rotta scartata |
| `Source/RefactorTactics/Turn/RTTurnManager.cpp` | messaggio di lock-in; `SrcCell` di partenza; voce per lo slot principale |
| `Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp` | tutti i test di questo piano — ha già gli helper, non se ne creano di nuovi |
| `docs/decisions/RT_PDR_00_Decision_Log.md` | `D-194` allineata a D-a, D-b, D-c |
| `docs/technical/architecture/spec-turnlog.md` | famiglie di voci e semantica dei campi |

⚠️ **Nessun golden corpus si rompe.** `ERTMoveOutcome::SupersededByDash` è una famiglia introdotta su questo
branch e non ancora su `main`: nessuna traccia archiviata la contiene, quindi cambiare `SrcCell` non tocca
hash storici. Se un golden cade, è un difetto vero — non rigenerarlo.

---

### Task 1: Il validatore al lock-in acquista un test

Quando questo piano è stato scritto, `grep -rn ValidatePlansAtLockIn Source/` restituiva **tre** occorrenze — dichiarazione, definizione,
chiamata — e **zero** test. La riga di #605 che questa PR chiude si può cancellare senza che la suite se ne
accorga.

**File:**
- Modifica: `Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp` (in coda, prima di
  `#endif // WITH_DEV_AUTOMATION_TESTS`)

**Interfacce:**
- Consuma: `ARTTurnManager::GetRecentEvents()` → `const TArray<FString>&` (`RTTurnManager.h:273`); gli helper
  già nel file — `MakeInteractionWorld()`, `SpawnCleanInteractionMap(World, Radius)`,
  `SpawnInteractionUnit(World, TeamId, HeroData, Cell)`, `DestroyInteractionWorld(World)`
- Produce: nulla per i task successivi

- [ ] **Step 1: scrivi i due test**

In coda a `RTPlayerInteractionTests.cpp`, prima di `#endif`:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLockInDeclaresTheContradictionTest,
	"RefactorTactics.PlayerInteraction.LockInDeclaresTheContradiction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLockInDeclaresTheContradictionTest::RunTest(const FString&)
{
	// La meta' del DoD di #605 che vive al COMMIT: «un punto solo che risponde LEGALE / ILLEGALE prima del
	// commit». Fino al 2026-08-26 non aveva un test: cancellare la chiamata a `ValidatePlansAtLockIn`
	// lasciava la suite verde.
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	if (!TestNotNull(TEXT("mappa senza ostacoli"), SpawnCleanInteractionMap(World, /*Radius=*/ 6)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	ARTUnit* U = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(1, 1));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), U) || !TestNotNull(TEXT("controller"), PC)
		|| !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	PC->SelectActorForTest(U);

	const int32 DashIdx = U->FindDashAbilityIndex();
	if (!TestNotEqual(TEXT("premessa: l'eroe ha una mobilita' rapida"), DashIdx, static_cast<int32>(INDEX_NONE)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	// Il piano incoerente: scatto, stato neutro (D-128), waypoint. Due azioni sullo slot Movimento.
	U->SelectAbility(DashIdx);
	PC->HandleClickOnCell(FRTCellId(1, 2));
	U->SelectAbility(INDEX_NONE);
	PC->HandleClickOnCell(FRTCellId(2, 1));

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	int32 Righe = 0;
	for (const FString& Evento : TM->GetRecentEvents())
	{
		if (Evento.Contains(TEXT("piano non valido al lock-in"))) { ++Righe; }
	}
	TestEqual(TEXT("il lock-in dichiara la contraddizione nel combat log"), Righe, 1);

	DestroyInteractionWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLockInStaysSilentOnALegalPlanTest,
	"RefactorTactics.PlayerInteraction.LockInStaysSilentOnALegalPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLockInStaysSilentOnALegalPlanTest::RunTest(const FString&)
{
	// Il gemello: senza, un `AddLogEvent` incondizionato passerebbe il test qui sopra.
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	if (!TestNotNull(TEXT("mappa senza ostacoli"), SpawnCleanInteractionMap(World, /*Radius=*/ 6)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	ARTUnit* U = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(1, 1));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), U) || !TestNotNull(TEXT("controller"), PC)
		|| !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	PC->SelectActorForTest(U);

	// Il piano canonico di D-028: un movimento e basta.
	U->SelectAbility(INDEX_NONE);
	PC->HandleClickOnCell(FRTCellId(1, 2));
	PC->HandleClickOnCell(FRTCellId(2, 2));

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	int32 Righe = 0;
	for (const FString& Evento : TM->GetRecentEvents())
	{
		if (Evento.Contains(TEXT("piano non valido al lock-in"))) { ++Righe; }
	}
	TestEqual(TEXT("un piano legale non produce nessuna riga di rifiuto"), Righe, 0);

	DestroyInteractionWorld(World);
	return true;
}
```

- [ ] **Step 2: compila ed esegui — devono passare**

```bash
"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development \
  -Project="D:\Repositories\refactor-tactict-dev\RefactorTactics.uproject" -WaitMutex
```

Poi i test mirati (comando nella sezione «Comandi»). Atteso: **entrambi `Result={Success}`**. Il codice di
produzione esiste già: qui si sta aggiungendo copertura, non funzionalità.

- [ ] **Step 3: prova mutante — senza di essa il test non vale**

Commenta la chiamata in `RTTurnManager.cpp` (cerca `ValidatePlansAtLockIn();` nel corpo di
`LockInAndResolve`), ricompila, riesegui.

Atteso: `LockInDeclaresTheContradiction` **`Result={Fail}`** con `Expected … to be 1, but it was 0`;
`LockInStaysSilentOnALegalPlan` resta `Success`.

Se il primo resta verde, il test non tocca il codice che crede di coprire: **fermati e correggilo** prima di
proseguire.

- [ ] **Step 4: ripristina e riverifica**

Rimetti la chiamata, ricompila, riesegui: entrambi `Success`.

- [ ] **Step 5: commit**

```bash
git add Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp
git commit -m "test(605): il validatore al lock-in non aveva un test, e si poteva cancellare"
```

---

### Task 2: Il messaggio nomina entrambe le azioni, in italiano

Implementa **D-b**. Oggi il messaggio stampa `Verdict.OffendingActionId` — per scatto + movimento è
`Hero.Riktor.Ram`, che esegue — e ci infila l'identificatore C++ grezzo dell'enum (`SlotOccupied`) in un log
altrimenti in prosa italiana.

**File:**
- Modifica: `Source/RefactorTactics/Turn/RTPlanValidationLibrary.h` (struct `FRTPlanValidation`, riga ~43)
- Modifica: `Source/RefactorTactics/Turn/RTPlanValidationLibrary.cpp` (`Reject`, riga ~10; loop degli slot,
  righe ~138-158)
- Modifica: `Source/RefactorTactics/Turn/RTTurnLogLibrary.h` / `.cpp` (estrazione di `DescribeInvalidReason`)
- Modifica: `Source/RefactorTactics/Turn/RTTurnManager.cpp` (`AddLogEvent` del lock-in, riga ~1975)
- Modifica: `Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp`

**Interfacce:**
- Consuma: `FRTPlanValidation { bool bLegal; ERTActionInvalidReason Reason; FName OffendingActionId; }`
- Produce:
  - `FRTPlanValidation::HolderActionId` → `FName` — l'azione che teneva già lo slot; `NAME_None` se il piano
    è legale o se il motivo non è un conflitto di slot
  - `static FString URTTurnLogLibrary::DescribeInvalidReason(ERTActionInvalidReason Reason)` → testo
    italiano del motivo, usato dal Task 6

- [ ] **Step 1: scrivi il test che asserisce il testo**

In coda a `RTPlayerInteractionTests.cpp`:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLockInNamesBothActionsTest,
	"RefactorTactics.PlayerInteraction.LockInNamesBothActions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLockInNamesBothActionsTest::RunTest(const FString&)
{
	// 🔴 Il messaggio NON deve nominare un solo colpevole. `ValidatePlan` ordina per larghezza di slot e poi
	// per `ActionId`: `Action.Move` precede `Hero.Riktor.Ram` alfabeticamente, quindi Move prende lo slot e
	// Ram risulta «l'offender» — ma e' Ram che ESEGUE, ed e' Move che il resolver scarta. Nominare una sola
	// azione qui significa accusare quella sbagliata; nominarle entrambe e' vero comunque vada.
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	if (!TestNotNull(TEXT("mappa senza ostacoli"), SpawnCleanInteractionMap(World, /*Radius=*/ 6)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	ARTUnit* U = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(1, 1));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), U) || !TestNotNull(TEXT("controller"), PC)
		|| !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	PC->SelectActorForTest(U);

	const int32 DashIdx = U->FindDashAbilityIndex();
	if (!TestNotEqual(TEXT("premessa: l'eroe ha una mobilita' rapida"), DashIdx, static_cast<int32>(INDEX_NONE)))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	U->SelectAbility(DashIdx);
	PC->HandleClickOnCell(FRTCellId(1, 2));
	U->SelectAbility(INDEX_NONE);
	PC->HandleClickOnCell(FRTCellId(2, 1));

	const FName DashId = U->GetAbility(DashIdx) ? U->GetAbility(DashIdx)->Def.ActionId : NAME_None;
	if (!TestFalse(TEXT("premessa: la mobilita' ha un ActionId"), DashId.IsNone()))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	FString Riga;
	for (const FString& Evento : TM->GetRecentEvents())
	{
		if (Evento.Contains(TEXT("piano non valido al lock-in"))) { Riga = Evento; break; }
	}
	if (!TestFalse(TEXT("premessa: la riga di rifiuto esiste"), Riga.IsEmpty()))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	TestTrue(TEXT("nomina il movimento"), Riga.Contains(TEXT("Action.Move")));
	TestTrue(TEXT("e nomina anche la mobilita'"), Riga.Contains(*DashId.ToString()));
	TestTrue(TEXT("dice il motivo in italiano"), Riga.Contains(TEXT("slot")));
	TestFalse(TEXT("e non stampa l'identificatore grezzo dell'enum"), Riga.Contains(TEXT("SlotOccupied")));

	DestroyInteractionWorld(World);
	return true;
}
```

- [ ] **Step 2: esegui — deve fallire**

Compila ed esegui i test mirati. Atteso: **`Result={Fail}`** — la riga attuale contiene `SlotOccupied` e una
sola azione.

- [ ] **Step 3: estendi il verdetto**

In `RTPlanValidationLibrary.h`, dentro `FRTPlanValidation`, subito dopo `OffendingActionId`:

```cpp
	/**
	 * Quale voce del piano teneva GIA' lo slot quando il rifiuto e' scattato; `NAME_None` se il piano e'
	 * legale o se il motivo non e' un conflitto di slot (`OnCooldown` non ha un detentore).
	 *
	 * 🔴 **Esiste perche' `OffendingActionId` da solo accusa l'azione sbagliata.** L'ordine canonico di
	 * `ValidatePlan` — per larghezza di slot, poi per `ActionId` — incontra `Action.Move` prima di
	 * `Hero.Riktor.Ram`, quindi nomina Ram; ma in risoluzione e' lo scatto a vincere ed e' Move a essere
	 * scartato. Chi comunica il rifiuto nomina ENTRAMBE e non deve indovinare il perdente.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	FName HolderActionId;
```

- [ ] **Step 4: traccia il detentore per slot**

In `RTPlanValidationLibrary.cpp`, sostituisci l'helper `Reject` (riga ~10):

```cpp
	/** Rifiuto con motivo, colpevole e detentore dello slot: un solo punto di costruzione, cosi' nessun
	 *  ramo dimentica un campo. `Holder` resta `NAME_None` per i motivi che non sono conflitti di slot. */
	FRTPlanValidation Reject(ERTActionInvalidReason Reason, const FName& ActionId,
		const FName& Holder = NAME_None)
	{
		FRTPlanValidation Result;
		Result.bLegal = false;
		Result.Reason = Reason;
		Result.OffendingActionId = ActionId;
		Result.HolderActionId = Holder;
		return Result;
	}
```

Poi sostituisci il blocco degli slot (righe ~138-158) — il `bool` diventa una `FName`, così il detentore è
noto:

```cpp
	// 2. Combinazione: l'occupazione degli slot. `None` non toglie niente (`Action.Wait`).
	//
	// Gli slot portano CHI li ha presi, non un `bool`: chi comunica il rifiuto nomina entrambe le azioni in
	// conflitto, perche' l'ordine canonico di questa funzione non e' l'ordine con cui il resolver scarta.
	FName MovementHolder;
	FName MainHolder;
	FName ReactionHolder;
	for (const FRTPlannedAction& Entry : Ordered)
	{
		const int32 Width = SlotWidth(Entry.Def.Slot);
		const bool bWantsMovement = (Entry.Def.Slot == ERTActionSlot::Movement) || (Width > 1);
		const bool bWantsMain = (Entry.Def.Slot == ERTActionSlot::Main) || (Width > 1);
		const bool bWantsReaction = (Entry.Def.Slot == ERTActionSlot::Reaction);

		if (bWantsMovement && !MovementHolder.IsNone())
		{
			return Reject(ERTActionInvalidReason::SlotOccupied, Entry.Def.ActionId, MovementHolder);
		}
		if (bWantsMain && !MainHolder.IsNone())
		{
			return Reject(ERTActionInvalidReason::SlotOccupied, Entry.Def.ActionId, MainHolder);
		}
		if (bWantsReaction && !ReactionHolder.IsNone())
		{
			return Reject(ERTActionInvalidReason::SlotOccupied, Entry.Def.ActionId, ReactionHolder);
		}

		if (bWantsMovement) { MovementHolder = Entry.Def.ActionId; }
		if (bWantsMain)     { MainHolder = Entry.Def.ActionId; }
		if (bWantsReaction) { ReactionHolder = Entry.Def.ActionId; }
	}
```

⚠️ Apri il file e confronta con l'originale prima di sostituire: le righe `const int32 Width = …` e le tre
`bWants…` devono restare **identiche** a quelle già presenti. Se differiscono, tieni quelle del file — cambia
solo la parte del `bool` → `FName`.

- [ ] **Step 5: rendi pubblica la traduzione dei motivi**

In `RTTurnLogLibrary.h`, nella sezione pubblica della classe, accanto a `DescribeEntry`:

```cpp
	/**
	 * Testo italiano di un motivo di invalidita'. UNA tabella sola: `DescribeEntry` la usa per le voci di
	 * Fallback e il combat log la usa per il rifiuto al lock-in. Due tabelle divergerebbero al primo motivo
	 * aggiunto — ed e' successo: la prima stesura del lock-in stampava l'identificatore C++ dell'enum.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|TurnLog")
	static FString DescribeInvalidReason(ERTActionInvalidReason Reason);
```

In `RTTurnLogLibrary.cpp`, definisci la funzione **spostandoci dentro** la tabella `switch` che oggi vive
dentro `DescribeEntry` (cerca `case ERTActionInvalidReason::TargetGone:`), e aggiungi il caso mancante:

```cpp
FString URTTurnLogLibrary::DescribeInvalidReason(ERTActionInvalidReason Reason)
{
	switch (Reason)
	{
	case ERTActionInvalidReason::TargetGone:     return TEXT("bersaglio assente");
	case ERTActionInvalidReason::TargetDead:     return TEXT("bersaglio eliminato");
	case ERTActionInvalidReason::TargetFriendly: return TEXT("bersaglio alleato");
	case ERTActionInvalidReason::OutOfRange:     return TEXT("fuori portata");
	case ERTActionInvalidReason::NoLineOfSight:  return TEXT("nessuna linea di tiro");
	case ERTActionInvalidReason::NoMap:          return TEXT("nessuna mappa autorevole");
	case ERTActionInvalidReason::SlotOccupied:   return TEXT("lo slot e' gia' occupato");
	default:                                     return TEXT("non eseguibile");
	}
}
```

⚠️ Copia i casi **dal file**, non da qui: se la tabella nel sorgente ha motivi che questo piano non elenca,
vanno mantenuti. `SlotOccupied` è quello da **aggiungere**.

Nel corpo di `DescribeEntry`, sostituisci il `switch` locale con la chiamata:

```cpp
		const FString Why = DescribeInvalidReason(static_cast<ERTActionInvalidReason>(Entry.Amount));
```

⚠️ Adatta il nome della variabile a quello già usato lì (`Why`) e rimuovi il `switch` che hai spostato.

- [ ] **Step 6: riscrivi il messaggio del lock-in**

In `RTTurnManager.cpp`, sostituisci l'`AddLogEvent` (riga ~1975):

```cpp
		// 🔴 **Entrambe le azioni, mai una sola.** `OffendingActionId` e' l'azione che l'ordine canonico del
		// validatore incontra per seconda, e per il caso canonico scatto + movimento e' la MOBILITA', che
		// invece esegue. Nominare lei significa mandare il giocatore a correggere l'azione sbagliata.
		// Dire che due azioni occupano lo stesso slot e' vero comunque il resolver decida.
		const FString Dettaglio = Verdict.HolderActionId.IsNone()
			? FString::Printf(TEXT("%s: %s"),
				*Verdict.OffendingActionId.ToString(),
				*URTTurnLogLibrary::DescribeInvalidReason(Verdict.Reason))
			: FString::Printf(TEXT("%s e %s occupano lo stesso slot"),
				*Verdict.OffendingActionId.ToString(),
				*Verdict.HolderActionId.ToString());

		AddLogEvent(FString::Printf(TEXT("%s: piano non valido al lock-in (%s)"),
			*Unit->GetName(), *Dettaglio));
```

- [ ] **Step 7: esegui — devono passare tutti**

Compila ed esegui i test mirati. Atteso: `LockInNamesBothActions` `Success`, e i due del Task 1 ancora
`Success`.

- [ ] **Step 8: commit**

```bash
git add Source/RefactorTactics/Turn/RTPlanValidationLibrary.h \
        Source/RefactorTactics/Turn/RTPlanValidationLibrary.cpp \
        Source/RefactorTactics/Turn/RTTurnLogLibrary.h \
        Source/RefactorTactics/Turn/RTTurnLogLibrary.cpp \
        Source/RefactorTactics/Turn/RTTurnManager.cpp \
        Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp
git commit -m "fix(605): il rifiuto al lock-in nominava l'azione che esegue"
```

---

### Task 3: `SrcCell` torna la cella di partenza

Implementa **D-a**.

**File:**
- Modifica: `Source/RefactorTactics/Turn/RTTurnManager.cpp` (`ResolveDash`, il blocco `Superseded` intorno a
  riga 3320-3340)
- Modifica: `Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp`
  (`FRTDashSupersedesTheNormalMoveTest`)
- Modifica: `docs/decisions/RT_PDR_00_Decision_Log.md` (`D-194`)
- Modifica: `docs/technical/architecture/spec-turnlog.md`

**Interfacce:**
- Consuma: `const bool bHadNormalMove` già calcolato prima della riscrittura di `Unit->Cell`
- Produce: `const FRTCellId PreDashCell` — la cella da cui l'unità è partita, usata anche dal Task 6

- [ ] **Step 1: cambia l'asserzione del test esistente**

In `FRTDashSupersedesTheNormalMoveTest`, sostituisci l'asserzione su `SrcCell`:

```cpp
	TestEqual(TEXT("SrcCell e' da dove il movimento sarebbe partito — chiave stabile dell'unita' nel turno"),
		Found.SrcCell, FRTCellId(1, 1));
```

⚠️ `FRTCellId(1, 1)` è la cella su cui il test spawna l'unità: se il test la cambia, cambia anche qui.

- [ ] **Step 2: esegui — deve fallire**

Atteso: **`Result={Fail}`** con `SrcCell` uguale a `(1,2)`, la cella d'arrivo dello scatto.

- [ ] **Step 3: cattura la cella di partenza e usala**

In `ResolveDash`, accanto al calcolo di `bHadNormalMove` (che sta **prima** di `Unit->Cell = Final;`),
aggiungi:

```cpp
		const FRTCellId PreDashCell = Unit->Cell;
```

Poi, nel blocco che costruisce la voce, sostituisci le due righe di celle:

```cpp
			// 🔴 **`SrcCell` e' la cella di PARTENZA**, non quella d'arrivo: `BuildMoveLog` la dichiara
			// «chiave stabile dell'unita' nel turno», e `FilterTracesByEmitter` ci filtra sopra con
			// `ExcludedSources.Contains(Entry.SrcCell)` per confrontare le varianti a informazione nascosta.
			// Con la cella d'arrivo la traccia di una variante sopravvive al filtro e manda rosso un
			// confronto PERCHE' la variante ha funzionato. E la coppia (SrcCell, TgtCell) qui descrive una
			// ROTTA: quella che non si e' percorsa.
			Superseded.SrcCell = PreDashCell;       // da dove il movimento sarebbe partito
			Superseded.TgtCell = Unit->PlannedCell; // la destinazione dichiarata e mai raggiunta
```

- [ ] **Step 4: esegui — devono passare**

Compila ed esegui i test mirati. Atteso: tutti `Success`.

- [ ] **Step 5: esegui la suite COMPLETA**

```bash
-ExecCmds="Automation RunTests RefactorTactics;Quit"
```

Atteso: `Found` = `Test Completed`, zero `Result={Fail}`. Se un test del corpus golden cade, **non
rigenerarlo**: `SupersededByDash` è una famiglia nuova che nessuna traccia archiviata contiene, quindi un
golden rosso qui significa che qualcos'altro è cambiato.

- [ ] **Step 6: allinea i due documenti**

In `docs/decisions/RT_PDR_00_Decision_Log.md`, nella riga `D-194`, sostituisci la descrizione dei campi della
voce: dove dice che `SrcCell` è *«dove lo scatto ha portato l'unita'»*, deve dire che è la cella di
**partenza**, con la ragione: è la chiave su cui `FilterTracesByEmitter` filtra, e la coppia con `TgtCell`
descrive la rotta scartata.

In `docs/technical/architecture/spec-turnlog.md`, stessa correzione nella descrizione della famiglia
`SupersededByDash`.

⚠️ La riga di `D-194` è **una riga sola** di tabella Markdown: non spezzarla, o `doc-tables` la vede come una
riga fuori posto.

- [ ] **Step 7: gate documentali**

```bash
node tools/radar/doc-links.ts --check
node tools/radar/doc-tables.ts --check
```

Attesi entrambi: exit `0`.

- [ ] **Step 8: commit**

```bash
git add Source/RefactorTactics/Turn/RTTurnManager.cpp \
        Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp \
        docs/decisions/RT_PDR_00_Decision_Log.md \
        docs/technical/architecture/spec-turnlog.md
git commit -m "fix(605): SrcCell e' la cella di partenza, ed e' la chiave su cui il harness filtra"
```

---

### Task 4: Il rendering mostra la rotta scartata

Oggi `DescribeEntry` stampa `TgtCell` solo per `Moved` e `Displaced`: `SupersededByDash` cade nel ramo con il
solo `SrcCell`, e la destinazione mai raggiunta — *«il dato che rende la voce utile invece che una nota»* —
non compare mai.

**File:**
- Modifica: `Source/RefactorTactics/Turn/RTTurnLogLibrary.cpp` (riga ~285)
- Modifica: `Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp`

**Interfacce:**
- Consuma: `URTTurnLogLibrary::DescribeEntry(const FRTTurnLogEntry&)` → `FString`
- Produce: nulla

- [ ] **Step 1: scrivi il test**

In coda a `RTPlayerInteractionTests.cpp`:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSupersededEntryRendersTheDiscardedRouteTest,
	"RefactorTactics.PlayerInteraction.SupersededEntryRendersTheDiscardedRoute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSupersededEntryRendersTheDiscardedRouteTest::RunTest(const FString&)
{
	// La voce esiste per portare la destinazione mai raggiunta. Se il rendering non la stampa, la traccia
	// dice al lettore la cella dove l'unita' E', che e' l'informazione che gia' aveva.
	FRTTurnLogEntry Entry;
	Entry.Phase = ERTMatchPhase::Dash;
	Entry.Category = ERTLogCategory::Move;
	Entry.Outcome = static_cast<uint8>(ERTMoveOutcome::SupersededByDash);
	Entry.ActionId = TEXT("Action.Move");
	Entry.SrcCell = FRTCellId(1, 1);
	Entry.TgtCell = FRTCellId(2, 1);
	Entry.Amount = 2;

	const FString Testo = URTTurnLogLibrary::DescribeEntry(Entry);

	TestTrue(TEXT("nomina la cella di partenza"), Testo.Contains(TEXT("q=1,r=1")));
	TestTrue(TEXT("e la destinazione mai raggiunta"), Testo.Contains(TEXT("q=2,r=1")));
	TestTrue(TEXT("e quante celle sono state scartate"), Testo.Contains(TEXT("2 celle")));

	return true;
}
```

⚠️ Il formato di `CellText` va verificato: apri `RTTurnLogLibrary.cpp`, cerca `CellText` e usa **la sua** resa
nei `Contains`. Se stampa `(q=1,r=1,L=0)`, le stringhe sopra vanno bene; se stampa altro, adeguale.

- [ ] **Step 2: esegui — deve fallire**

Atteso: **`Result={Fail}`** sull'asserzione della destinazione — oggi non viene stampata.

- [ ] **Step 3: estendi il ramo**

In `RTTurnLogLibrary.cpp` (riga ~285):

```cpp
		// `SupersededByDash` sta QUI e non nel ramo breve: la sua ragione d'essere e' la destinazione mai
		// raggiunta, e un rendering che stampa solo `SrcCell` la nasconde. La coppia descrive la rotta
		// scartata, ed e' la stessa forma di `Moved` — cambia il motivo, non la geometria.
		if (static_cast<ERTMoveOutcome>(Entry.Outcome) == ERTMoveOutcome::Moved
			|| static_cast<ERTMoveOutcome>(Entry.Outcome) == ERTMoveOutcome::Displaced
			|| static_cast<ERTMoveOutcome>(Entry.Outcome) == ERTMoveOutcome::SupersededByDash)
		{
```

- [ ] **Step 4: esegui — devono passare**

Compila, esegui i test mirati: tutti `Success`.

- [ ] **Step 5: commit**

```bash
git add Source/RefactorTactics/Turn/RTTurnLogLibrary.cpp \
        Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp
git commit -m "fix(605): la voce nascondeva la destinazione per cui esiste"
```

---

### Task 5: I campi dell'ordine canonico sono difesi, e il catalogo esce dal loop

Tre rilievi che stanno insieme perché toccano lo stesso blocco: nessun test difende `Amount` e `Priority`,
`FindCoreAction` viene richiamata per ogni unità che scatta, e un commento dice che il test portante è il
secondo quando è il terzo.

**File:**
- Modifica: `Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp` (asserzioni + commento riga ~358)
- Modifica: `Source/RefactorTactics/Turn/RTTurnManager.cpp` (`FindCoreAction` fuori dal loop)

**Interfacce:**
- Consuma: `FRTTurnLogEntry Found` già in mano al test `DashSupersedesTheNormalMove`
- Produce: nulla

- [ ] **Step 1: aggiungi le due asserzioni mancanti**

In `FRTDashSupersedesTheNormalMoveTest`, dopo le asserzioni su `SrcCell` e `TgtCell`:

```cpp
	// I due campi che entrano nell'ORDINE CANONICO e nel formato serializzato. Senza queste asserzioni,
	// mettere `Priority` a zero o `Amount` a un numero qualsiasi lascia la suite verde — ed e' esattamente
	// il difetto che la correzione del 2026-08-26 ha chiuso, non difeso da nessuno.
	TestEqual(TEXT("Priority viene dal catalogo, non da uno zero implicito"),
		Found.Priority, URTCatalogLibrary::FindCoreAction(TEXT("Action.Move")).Priority);
	TestEqual(TEXT("Amount conta le celle del percorso scartato"), Found.Amount, CelleAttese);
```

⚠️ `CelleAttese` va catturata **prima** di `LockInAndResolve()`, perché il resolver azzera il piano:
`const int32 CelleAttese = U->PlannedWaypoints.Num();` subito dopo i due `HandleClickOnCell`.

- [ ] **Step 2: esegui — devono passare**

Le asserzioni descrivono il comportamento attuale corretto: servono da rete, non da correzione. Atteso:
`Success`.

- [ ] **Step 3: prova mutante sui due campi**

Metti `Superseded.Priority = 0;` in `RTTurnManager.cpp`, ricompila, riesegui: `DashSupersedesTheNormalMove`
deve andare **`Fail`**. Ripristina e riverifica verde.

Se resta verde, l'asserzione non morde: correggila prima di proseguire.

- [ ] **Step 4: porta il catalogo fuori dal loop**

`FindCoreAction` ricostruisce l'array dei ~30 `FRTActionDef` a ogni chiamata — `RTPlanValidationLibrary.cpp:83`
lo documenta dopo averlo misurato. Sostituisci la riga dentro il blocco:

```cpp
			// `static const`: `FindCoreAction` COSTRUISCE il catalogo a ogni invocazione, e questo blocco sta
			// dentro il loop delle unita' che scattano. Stessa correzione gia' applicata in
			// `RTPlanValidationLibrary.cpp` dopo una misura in code review.
			static const FRTActionDef MoveDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Move"));
```

- [ ] **Step 5: correggi il commento stantio**

In `RTPlayerInteractionTests.cpp` (riga ~358), il commento dice che i due test vanno tenuti insieme perché
*«senza il secondo, un `AppendLogEntry` incondizionato passerebbe il primo»*. È falso: il secondo test non
pianifica nessuno scatto, quindi `ResolveDash` esce a `DasherCount == 0` e non esegue il blocco.
Sostituiscilo:

```cpp
	// I tre test vanno tenuti INSIEME, e il portante e' il TERZO. Il secondo — piano legale senza scatto —
	// non attraversa il blocco: `ResolveDash` esce a `DasherCount == 0` prima di arrivarci, quindi un
	// `AppendLogEntry` incondizionato lo passerebbe. E' `NoSupersededEntryOnADashWithoutAPlannedMove` a
	// coprire quel caso, ed e' il test che il 2026-08-26 ha misurato il falso positivo su ogni scatto.
```

- [ ] **Step 6: esegui la suite completa**

Atteso: `Found` = `Test Completed`, zero fail.

- [ ] **Step 7: commit**

```bash
git add Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp \
        Source/RefactorTactics/Turn/RTTurnManager.cpp
git commit -m "test(605): Amount e Priority non li difendeva nessuno"
```

---

### Task 6: Anche lo slot principale dichiara ciò che perde

Implementa **D-c**. Una mobilità `MovementAndMain` azzera `PlannedAbilityIndex` e `PlannedAttackTarget` senza
lasciare traccia: l'obiettivo *«ciò che il turno scarta lo dichiara chi lo scarta»* oggi è chiuso per metà.

La famiglia giusta è **`Fallback`** con esito `Cancelled` e motivo `SlotOccupied` in `Amount`, come fa
`RTTurnManager_Blast.cpp:453`: l'azione principale non esegue affatto, quindi `Cancelled` dice il vero (a
differenza del movimento, dove il resolver fa comunque muovere l'unità).

**File:**
- Modifica: `Source/RefactorTactics/Turn/RTTurnManager.cpp` (il ramo `MovementAndMain`, riga ~3371)
- Modifica: `Source/RefactorTactics/Tests/RTHexMovementIntegrationTests.cpp` (accanto a
  `FRTKitDeclaredBothSlotsTest`, riga ~913)
- Modifica: `docs/decisions/RT_PDR_00_Decision_Log.md` (`D-194`)
- Modifica: `docs/technical/architecture/spec-turnlog.md`

⚠️ Il test di questo task **non** va in `RTPlayerInteractionTests.cpp`: il caso non passa dal
`PlayerController` — il piano si scrive direttamente sui campi — e l'unico allestimento che costruisce un kit
`MovementAndMain` vive in `RTHexMovementIntegrationTests.cpp` con i propri helper (`MakeHexMoveWorld`,
`SpawnHexMap`, `SpawnHexUnit`, `RunTurn`, `DestroyHexMoveWorld`). Il test nuovo sta accanto al gemello e
riusa quelli.

**Interfacce:**
- Consuma: `const FRTCellId PreDashCell` (Task 3); `URTTurnLogLibrary::DescribeInvalidReason` (Task 2);
  `URTActionData::Def.BaseActionId` (riempito come in `RTTurnManager.cpp:1562`); gli helper di
  `RTHexMovementIntegrationTests.cpp`
- Produce: nulla

- [ ] **Step 1: scrivi il test**

Subito dopo `FRTKitDeclaredBothSlotsTest` in `RTHexMovementIntegrationTests.cpp`. L'allestimento è quello del
gemello — che prova che il colpo **non parte**; questo prova che il turno lo **dice**:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKitDeclaredBothSlotsDeclaresTheDiscardTest,
	"RefactorTactics.Actions.KitDeclaringBothSlotsDeclaresTheDiscard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKitDeclaredBothSlotsDeclaresTheDiscardTest::RunTest(const FString&)
{
	// L'altra meta' di «cio' che il turno scarta lo dichiara chi lo scarta». Il gemello
	// `KitCanDeclareAMobilityThatCostsBothSlots` prova che il colpo NON parte; qui si prova che il turno lo
	// DICE, invece di far sparire l'azione principale in silenzio.
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 8);

	ARTUnit* Runner = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Runner) { DestroyHexMoveWorld(World); return false; }

	constexpr int32 DashTo = 3;
	const int32 ShotRange = Runner->AttackRange;
	ARTUnit* Foe = SpawnHexUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(DashTo + ShotRange, 0));
	if (!TestNotNull(TEXT("Foe"), Foe)) { DestroyHexMoveWorld(World); return false; }

	// Uno scatto identico ad `Action.Dash` TRANNE lo slot: e' il kit a dichiarare che costa tutto il turno.
	URTActionData* Costly = NewObject<URTActionData>(Runner);
	Costly->DisplayName = FText::FromString(TEXT("Scatto totale"));
	Costly->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Dash"));
	Costly->Def.ActionId = FName(TEXT("Hero.CostlyDash"));
	Costly->Def.Slot = ERTActionSlot::MovementAndMain;
	Runner->Abilities.Add(Costly);

	// Catturate PRIMA della risoluzione: il resolver azzera il piano e sposta l'unita'.
	const FRTCellId CellaDiPartenza = Runner->Cell;
	const URTActionData* Principale = Runner->GetAbility(0);
	if (!TestNotNull(TEXT("premessa: l'unita' ha un'azione principale"), Principale))
	{
		DestroyHexMoveWorld(World);
		return false;
	}
	const FName IdDellaPrincipale = Principale->Def.ActionId;

	Runner->PlannedDashAbility = Runner->Abilities.Num() - 1;
	Runner->PlannedDashCell = FRTCellId(DashTo, 0);
	Runner->PlannedAbilityIndex = 0;   // l'attacco base, che lo scatto si portera' via
	Runner->PlannedAttackTarget = Foe;
	Foe->PlannedCell = Foe->Cell;

	RunTurn(TM);

	int32 Cancellate = 0;
	FRTTurnLogEntry Found;
	for (const FRTTurnLogEntry& Entry : TM->GetTurnLog())
	{
		if (Entry.Category == ERTLogCategory::Fallback
			&& static_cast<ERTFallbackOutcome>(Entry.Outcome) == ERTFallbackOutcome::Cancelled
			&& static_cast<ERTActionInvalidReason>(Entry.Amount) == ERTActionInvalidReason::SlotOccupied)
		{
			++Cancellate;
			Found = Entry;
		}
	}

	if (!TestEqual(TEXT("una voce dichiara la principale scartata"), Cancellate, 1))
	{
		DestroyHexMoveWorld(World);
		return false;
	}
	TestEqual(TEXT("la voce sta nella fase in cui lo scarto avviene"), Found.Phase, ERTMatchPhase::Dash);
	TestEqual(TEXT("nomina l'azione che NON si esegue"), Found.ActionId, IdDellaPrincipale);
	TestEqual(TEXT("SrcCell e' la cella di partenza"), Found.SrcCell, CellaDiPartenza);
	TestEqual(TEXT("TgtCell = SrcCell quando non applicabile"), Found.TgtCell, Found.SrcCell);

	// E il rendering dice il motivo invece di «non eseguibile».
	TestTrue(TEXT("il motivo si legge in italiano"),
		URTTurnLogLibrary::DescribeEntry(Found).Contains(TEXT("slot")));

	// E lo scatto e' avvenuto davvero: senza, la voce descriverebbe un turno che non c'e' stato.
	TestEqual(TEXT("premessa: lo scatto ha spostato l'unita'"), Runner->Cell, FRTCellId(DashTo, 0));

	DestroyHexMoveWorld(World);
	return true;
}
```

- [ ] **Step 2: esegui — deve fallire**

Il filtro è `RefactorTactics.Actions` per questo file:

```bash
-ExecCmds="Automation RunTests RefactorTactics.Actions;Quit"
```

Atteso: **`Result={Fail}`** con `Cancellate` a `0` — la voce non esiste ancora. Il gemello
`KitCanDeclareAMobilityThatCostsBothSlots` deve restare `Success`.

- [ ] **Step 3: scrivi la voce**

In `ResolveDash`, sostituisci il corpo del ramo `MovementAndMain`:

```cpp
	if (Used->Def.Slot == ERTActionSlot::MovementAndMain)
	{
		// 🔴 **Anche questo scarto si DICHIARA.** Fino ad ora la principale spariva in silenzio: stessa
		// forma del movimento scartato qui sopra, stessa ragione. Famiglia `Fallback`/`Cancelled` e non
		// `Move`/`SupersededByDash` perche' qui l'azione non avviene AFFATTO — mentre il movimento, dopo
		// lo scatto, l'unita' l'ha comunque compiuto. Il motivo viaggia in `Amount`, come per ogni voce
		// di Fallback (`RTTurnManager_Blast.cpp`).
		if (Unit->PlannedAbilityIndex != INDEX_NONE && Unit->IsAlive())
		{
			const URTActionData* Dropped = Unit->GetAbility(Unit->PlannedAbilityIndex);
			if (Dropped)
			{
				FRTTurnLogEntry Discarded;
				Discarded.Phase = ERTMatchPhase::Dash;
				Discarded.Category = ERTLogCategory::Fallback;
				Discarded.Outcome = static_cast<uint8>(ERTFallbackOutcome::Cancelled);
				Discarded.ActionId = Dropped->Def.ActionId;
				Discarded.BaseActionId = Dropped->Def.BaseActionId; // D-033: la traccia si spiega da sola
				Discarded.SrcCell = PreDashCell;   // chiave stabile dell'unita' nel turno
				Discarded.TgtCell = PreDashCell;   // = SrcCell: qui non c'e' una destinazione
				Discarded.Amount = static_cast<int32>(ERTActionInvalidReason::SlotOccupied);
				AppendLogEntry(Discarded, Unit);
				AddLogEvent(FString::Printf(TEXT("%s: %s"),
					*Unit->GetName(), *URTTurnLogLibrary::DescribeEntry(Discarded)));
			}
		}

		Unit->PlannedAbilityIndex = INDEX_NONE; // lo slot principale e' speso
		Unit->PlannedAttackTarget = nullptr;
	}
```

⚠️ `PreDashCell` è la `const` introdotta dal Task 3: verifica che sia in scope qui — il ramo
`MovementAndMain` sta più in basso nello stesso corpo di loop.

- [ ] **Step 4: esegui — devono passare**

Compila, esegui `RefactorTactics.Actions`: il test nuovo e il gemello entrambi `Success`.

- [ ] **Step 5: suite completa + Shipping**

```bash
-ExecCmds="Automation RunTests RefactorTactics;Quit"
```

Poi il gate G1, che è l'unico posto dove `WITH_DEV_AUTOMATION_TESTS` vale `0` e il codice di test fuori dai
`#if` esplode:

```bash
"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTactics Win64 Shipping \
  -Project="D:\Repositories\refactor-tactict-dev\RefactorTactics.uproject" -WaitMutex
```

Attesi: suite con `Found` = `Test Completed` e zero fail; Shipping `Result: Succeeded`.

⚠️ Dopo una build Shipping, **ricompila `RefactorTacticsEditor`** prima di rilanciare l'automation:
altrimenti `UnrealEditor-Cmd` muore durante il caricamento moduli e la run non produce risultati.

- [ ] **Step 6: allinea i due documenti**

In `D-194`, aggiungi che lo scarto si dichiara per **entrambi** gli slot, con la ragione della famiglia
diversa: `Move`/`SupersededByDash` per il movimento perché l'unità si muove comunque,
`Fallback`/`Cancelled` per la principale perché quella non avviene affatto.

In `spec-turnlog.md`, aggiungi la voce alla tabella delle famiglie.

- [ ] **Step 7: gate documentali**

```bash
node tools/radar/doc-links.ts --check
node tools/radar/doc-tables.ts --check
```

- [ ] **Step 8: commit**

```bash
git add Source/RefactorTactics/Turn/RTTurnManager.cpp \
        Source/RefactorTactics/Tests/RTHexMovementIntegrationTests.cpp \
        docs/decisions/RT_PDR_00_Decision_Log.md \
        docs/technical/architecture/spec-turnlog.md
git commit -m "feat(605): anche lo slot principale dichiara cio' che perde"
```

---

## Chiusura

- [ ] `git fetch --prune origin` e `gh pr list --state open`: riverifica che `D-194` non sia stata presa da
  un'altra PR mentre questo piano girava. Se lo è, rinumera **questa**.
- [ ] Aggiorna il corpo della PR #1400 con le misure reali (suite, Editor, Shipping, gate).
- [ ] `/code-review 1400`.
- [ ] Aggiorna il DoD di #605 con ciò che le due righe ora hanno: il test del lock-in e le due voci in
  risoluzione.

## Cosa questo piano NON fa

- ~~**Lo snapshot inutile al lock-in.**~~ ✅ **Fatto** in [#1419](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1419)
  (issue [#1413](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1413)): la raccolta è
  estratta in `ARTTurnManager::CollectLivingUnits` e lo stato dell'unità in `MakeSimUnit`, così il lock-in
  non paga più la mappa e non costruisce niente a mano.
- ~~**Il predicato «ha un movimento pianificato» in due file.**~~ ✅ **Fatto** nella stessa PR (issue
  [#1414](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1414)): è
  `ARTUnit::HasPlannedNormalMove()`, e i chiamanti sono diventati **tre** — la HUD ne teneva una copia con
  metà regola.
- **Il ventunesimo builder di mappa piatta nei test.** Questo piano non ne aggiunge uno nuovo — riusa gli
  helper già presenti in `RTPlayerInteractionTests.cpp` — ma non promuove nemmeno quelli esistenti in
  `URTMatchSetupLibrary`, che è il consolidamento che venti file aspettano.

Nessuno dei tre è registrato da nessuna parte: aprire le issue è parte della chiusura, non un'opzione.
