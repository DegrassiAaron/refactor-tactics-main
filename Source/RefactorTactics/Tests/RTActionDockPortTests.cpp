// #2826 — la porta che il dock chiama per armare, e la differenza fra un click e un tasto.
//
// 🔑 **Il difetto che questo file presidia e' che il dock era a schermo e non serviva a niente.**
// `#2759`, `#2760` e `#2784` hanno chiuso i tre difetti di montaggio: gli slot mostrano azione, icona,
// cooldown e stato armato — e cliccarli non faceva nulla, perche' `SelectAbilityForCurrent` e' `private:`
// e senza `UFUNCTION`. L'unica strada per armare era la tastiera.

#include "Misc/AutomationTest.h"
#include "Player/RTPlayerController.h"
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTCellId.h"
#include "UI/RTScreenHudWidgets.h" // URTActionSlotWidget: la meta' del tratto che vive nel widget
#include "Ability/RTMovementProfileLibrary.h" // ProfileSneak: il badge della barra dichiara lo stesso profilo del tasto (#3470)
#include "Ability/RTActionData.h"              // URTActionData::Def: trovare Overwatch nel kit, per la riserva
#include "UI/RTHudViewModel.h"     // FRTAbilityCooldownView: le viste vengono dal dock, non da qui
#include "RTWorldFixtures.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Un'unita' con il kit vero: senza `DispatchBeginPlay` i cooldown restano vuoti e il kit e' finto. */
	ARTUnit* SpawnUnitConKit(UWorld* World, int32 TeamId)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U)
		{
			return nullptr;
		}
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeAevik());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = false;
		U->DispatchBeginPlay();
		U->PlaceOnCell(FRTCellId(0, 0, 0), FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}
}

/**
 * 🔴 **CLICCARE UNO SLOT ARMA, E RICLICCARLO DISARMA — passando dalla stessa porta del tasto.**
 *
 * 🔑 **Le due meta' misurano cose diverse, e la seconda e' quella che il tasto non ha.**
 * `ArmKitAbility` delega a `SelectAbilityForCurrent`, cioe' allo stesso corpo che i dieci numeri
 * attraversano: cooldown, slot reazione e self-target restano decisi in un posto solo, e un click non
 * puo' aggirare un controllo che il tasto rispetta. Cio' che aggiunge e' **soltanto** il toggle.
 *
 * ⚠️ **E il toggle NON deve stare dentro `SelectAbilityForCurrent`**, ed e' il controllo C di questo
 * test: premere due volte `3` e' una riconferma, cliccare due volte lo slot acceso e' una richiesta di
 * spegnerlo. Se il toggle scendesse nel percorso comune, il secondo `3` disarmerebbe — un difetto che
 * nessuna delle due meta' qui sopra, da sola, vedrebbe.
 *
 * ⛔ **Il disarmo passa dalla porta e non scrive `SelectedAbilityIndex` a mano**: eredita cosi' le
 * guardie su input bloccato e pianificazione inerte. Un click che disarmasse durante la risoluzione
 * sarebbe un secondo canale con regole proprie.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionDockPortArmsAndDisarmsTest,
	"RefactorTactics.PlayerInput.TheDockPortArmsAndDisarms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTActionDockPortArmsAndDisarmsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World))
	{
		return false;
	}

	ARTUnit* Unit = SpawnUnitConKit(World, /*TeamId*/ 0);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!Unit || !PC)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("unita' e controller esistono"), false);
	}
	PC->SelectActorForTest(Unit);

	// Anti-vacuita': senza un kit, ogni asserzione qui sotto sarebbe verde per il motivo sbagliato.
	if (!TestTrue(TEXT("premessa: l'unita' ha un kit da armare"), Unit->NumAbilities() > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	const int32 Indice = 0;

	// --- A. il click ARMA ------------------------------------------------------------------------
	PC->ArmKitAbility(Indice);
	TestEqual(TEXT("A: cliccare uno slot arma quella posizione del kit"),
		Unit->SelectedAbilityIndex, Indice);

	// --- B. ricliccarlo DISARMA -----------------------------------------------------------------
	PC->ArmKitAbility(Indice);
	TestEqual(TEXT("B: ricliccare lo slot armato riporta al neutro di D-128"),
		Unit->SelectedAbilityIndex, INDEX_NONE);

	// --- C. il TASTO non fa toggle, ed e' la differenza che giustifica la porta ------------------
	// ⚠️ Senza questo controllo, spostare il toggle dentro `SelectAbilityForCurrent` supererebbe A e B
	// e romperebbe la tastiera in silenzio.
	PC->SelectAbilityForCurrentForTest(Indice);
	PC->SelectAbilityForCurrentForTest(Indice);
	TestEqual(TEXT("C: due pressioni dello stesso tasto RICONFERMANO, non disarmano"),
		Unit->SelectedAbilityIndex, Indice);

	// --- D. armare un'altra posizione sostituisce, non accumula ---------------------------------
	if (Unit->NumAbilities() > 1)
	{
		PC->ArmKitAbility(1);
		TestEqual(TEXT("D: armare un'altra posizione sostituisce la precedente"),
			Unit->SelectedAbilityIndex, 1);
	}

	// --- E. fail-closed senza selezione ----------------------------------------------------------
	// Non c'e' un valore da leggere: la prova e' che non esploda e non tocchi l'unita' deselezionata.
	PC->SelectActorForTest(nullptr);
	PC->ArmKitAbility(0);
	TestTrue(TEXT("E: senza unita' selezionata la porta non fa nulla e non crolla"), true);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **CLICK E TASTO RAGGIUNGONO LA STESSA VOCE DI KIT — PRIMA, INTERMEDIA E ULTIMA POSIZIONE** (`#2987`).
 *
 * 🔑 **L'oracolo e' l'UGUAGLIANZA fra i due percorsi, non che ciascuno «funzioni».** Due canali che
 * armano *qualcosa* passerebbero due test scritti separatamente e potrebbero comunque armare due voci
 * diverse: e' precisamente il difetto che la lista del dock rendeva possibile, quando la vista rinumerava
 * le posizioni e il tasto no.
 *
 * ⚠️ **Le tre posizioni non sono un campione di cortesia.** `ActionSlotLineCarriesKeyArmedAndReason`
 * provava una posizione centrale e per questo restava verde sul difetto: sono la **prima** e l'**ultima** a
 * rompersi per prime quando una lista si accorcia o si riordina.
 *
 * ⛔ **Il toggle e' l'unica differenza ammessa**, ed e' gia' presidiato da `TheDockPortArmsAndDisarms`:
 * qui si parte ogni volta dal neutro di [D-128], cosi' la seconda meta' del confronto misura l'armamento e
 * non un disarmo accidentale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionDockParityTest,
	"RefactorTactics.PlayerInput.DockClickAndHotkeyReachTheSameAbility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTActionDockParityTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World))
	{
		return false;
	}

	ARTUnit* Unit = SpawnUnitConKit(World, /*TeamId*/ 0);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!Unit || !PC)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("unita' e controller esistono"), false);
	}
	PC->SelectActorForTest(Unit);

	const int32 Ultima = Unit->NumAbilities() - 1;
	if (!TestTrue(TEXT("premessa: il kit ha almeno tre posizioni"), Unit->NumAbilities() >= 3))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Prima, intermedia, ultima: gli estremi sono quelli che un riordino o un accorciamento spostano.
	for (const int32 Posizione : { 0, Unit->NumAbilities() / 2, Ultima })
	{
		// Dal neutro, cosi' il click ARMA invece di fare toggle su cio' che il tasto aveva lasciato.
		PC->SelectAbilityForCurrentForTest(INDEX_NONE);
		PC->SelectAbilityForCurrentForTest(Posizione);
		const int32 DalTasto = Unit->SelectedAbilityIndex;

		PC->SelectAbilityForCurrentForTest(INDEX_NONE);
		PC->ArmKitAbility(Posizione);
		const int32 DalClick = Unit->SelectedAbilityIndex;

		TestEqual(
			FString::Printf(TEXT("posizione %d: click e tasto armano la stessa voce"), Posizione),
			DalClick, DalTasto);

		// ⚠️ Anti-vacuita': se entrambi i canali fallissero, `INDEX_NONE == INDEX_NONE` passerebbe
		// l'uguaglianza qui sopra dichiarando parita' fra due percorsi che non armano niente.
		TestNotEqual(
			FString::Printf(TEXT("posizione %d: e l'hanno armata davvero"), Posizione),
			DalClick, static_cast<int32>(INDEX_NONE));
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **LO SLOT INOLTRA IL PROPRIO INDICE, e la sua firma non gli lascia nominare un'azione** (`#2826`).
 *
 * 🔑 **Il tratto che questo test presidia non esisteva: lo slot non aveva NESSUNA porta in uscita.**
 * `SetAction` e' entrante, `GetResolvedIcon`, `GetIconId` e `GetActionLine` sono pure — e `ArmKitAbility`,
 * `BlueprintCallable` e gia' testata da `TheDockPortArmsAndDisarms`, non aveva un solo chiamante in
 * `Content/`. Fra il click e la porta mancava il pezzo, e nessun oracolo lo guardava.
 *
 * 🔑 **Il difetto meccanico che `Activate()` esiste per evitare e' quello di `Choose()`, alla lettera**: in
 * un `ForEach` di Blueprint l'indice non e' catturabile dentro un delegate — `OnClicked` non porta
 * parametri — e senza un figlio che tenga il proprio indice **tutti** gli slot armerebbero lo stesso, cioe'
 * l'ultimo. ∴ la meta' **B** non attiva uno slot: le attiva **tutte**, e chiede a ciascuna un indice
 * diverso. Un test su un solo slot sarebbe verde proprio sul difetto.
 *
 * ⚠️ **E le viste NON sono costruite a mano**: vengono da `URTActionDockWidget::GetActions()`, cioe' dal
 * codice di produzione. Scrivere `AbilityIndex = i` nel test renderebbe l'uguaglianza vera per costruzione
 * e cieca al giorno in cui la vista smettesse di portare la posizione di kit (`#2987`).
 *
 * ⛔ **La meta' C e' una proprieta' della FIRMA, non del comportamento**: `Activate()` non prende parametri,
 * quindi l'unica cosa che puo' spedire e' cio' che lo slot gia' tiene. Un grafo non ha un ingresso da cui
 * infilare una posizione altrui, e non ha un `FName` da nominare — la stessa disciplina di `ChooseOption`,
 * resa qui verificabile invece che affidata a chi legge il diff.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionSlotForwardsItsOwnIndexTest,
	"RefactorTactics.ScreenHud.ActionSlotForwardsItsOwnIndex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTActionSlotForwardsItsOwnIndexTest::RunTest(const FString&)
{
	// --- C. LA FIRMA: nessun parametro, e raggiungibile dal grafo --------------------------------------
	// Si misura per prima perche' non ha bisogno di un mondo, e perche' e' cio' che rende il resto
	// interessante: una `Activate()` non `BlueprintCallable` sarebbe irraggiungibile dal `.uasset`, e il
	// cablaggio del bottone non avrebbe niente da chiamare.
	UFunction* Porta = URTActionSlotWidget::StaticClass()->FindFunctionByName(TEXT("Activate"));
	if (!TestNotNull(TEXT("C: `Activate` esiste come UFUNCTION sullo slot"), Porta))
	{
		return false;
	}
	TestTrue(TEXT("C: ed e' BlueprintCallable, altrimenti il grafo non potrebbe chiamarla"),
		Porta->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	TestEqual(
		TEXT("C: non prende parametri — il grafo non ha un ingresso da cui nominare un'altra posizione"),
		static_cast<int32>(Porta->NumParms), 0);

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World))
	{
		return false;
	}

	ARTUnit* Unit = SpawnUnitConKit(World, /*TeamId*/ 0);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	URTActionDockWidget* Dock = NewObject<URTActionDockWidget>(World);
	if (!Unit || !PC || !Dock)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("unita', controller e dock esistono"), false);
	}
	PC->SelectActorForTest(Unit);
	Dock->SetSelectedUnitForTest(Unit);

	// Le viste di PRODUZIONE: e' da qui che arriva `AbilityIndex`, non da una riga di questo test.
	const TArray<FRTAbilityCooldownView> Azioni = Dock->GetActions();
	if (!TestTrue(
			FString::Printf(TEXT("premessa: il dock costruisce piu' di due riquadri (ne ha %d) — con uno ")
				TEXT("solo, «il proprio indice» non significherebbe niente"), Azioni.Num()),
			Azioni.Num() >= 3))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// --- A. FAIL-CLOSED: uno slot mai assegnato non spegne l'azione di un altro ------------------------
	// 🔴 Il caso non e' teorico e non e' benigno: `Action.AbilityIndex` nasce `INDEX_NONE`, e
	// `ArmKitAbility(INDEX_NONE)` **disarma**. Senza la guardia in `Activate()` un riquadro vuoto — o
	// sopravvissuto a una ricostruzione della lista — spegnerebbe cio' che il giocatore aveva appena armato.
	//
	// ⚠️ **La posizione di riferimento si CERCA, non si sceglie a numero.** `SelectAbilityForCurrent`
	// rifiuta legittimamente una reazione o un supporto non pronti, e una posizione di kit vuota; scrivere
	// `1` renderebbe questo test rosso su un kit che non ha nulla da armare in seconda posizione, cioe' per
	// una ragione che non c'entra col difetto.
	int32 Riferimento = INDEX_NONE;
	for (const FRTAbilityCooldownView& Vista : Azioni)
	{
		PC->SelectAbilityForCurrentForTest(INDEX_NONE);
		PC->ArmKitAbility(Vista.AbilityIndex);
		if (Unit->SelectedAbilityIndex == Vista.AbilityIndex)
		{
			Riferimento = Vista.AbilityIndex;
			break;
		}
	}
	if (!TestNotEqual(
			TEXT("premessa di A: il kit ha almeno una posizione che si arma — senza, «non disarma» non ")
			TEXT("significherebbe niente"),
			Riferimento, static_cast<int32>(INDEX_NONE)))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	URTActionSlotWidget* MaiAssegnato = NewObject<URTActionSlotWidget>(World);
	MaiAssegnato->SetArmingControllerForTest(PC);
	TestEqual(TEXT("A: uno slot mai assegnato porta INDEX_NONE"),
		MaiAssegnato->Action.AbilityIndex, static_cast<int32>(INDEX_NONE));
	MaiAssegnato->Activate();
	TestEqual(TEXT("A: e attivarlo NON disarma cio' che era armato"),
		Unit->SelectedAbilityIndex, Riferimento);

	// --- B. LA MISURA: ogni riquadro arriva dove arriva la porta chiamata con LA SUA posizione ---------
	// 🔑 **L'oracolo e' un confronto fra due percorsi, non «il click fa qualcosa».** Il riferimento e'
	// `ArmKitAbility(Azioni[i].AbilityIndex)` — la porta chiamata direttamente — e cio' che si misura e'
	// l'unico tratto in mezzo: il salto dal widget. Cosi' una posizione che il core rifiuta di suo (reazione
	// non pronta, supporto in ricarica, buco nel kit) non falsa la misura: i due percorsi la rifiutano
	// entrambi e restano uguali.
	//
	// ⚠️ **E questo confronto vede comunque il difetto meccanico**: se tutti i riquadri spedissero l'ultimo
	// indice, per ogni `i` diverso dall'ultimo il click arriverebbe su una posizione e il riferimento su
	// un'altra. Se invece `Activate()` non facesse nulla, il click resterebbe `INDEX_NONE` dove il
	// riferimento arma — e lo dice anche il contatore qui sotto.
	int32 Armate = 0;
	for (int32 i = 0; i < Azioni.Num(); ++i)
	{
		URTActionSlotWidget* Slot = NewObject<URTActionSlotWidget>(World);
		Slot->SetArmingControllerForTest(PC);
		Slot->SetAction(Azioni[i], /*bInArmed=*/ false);

		// Dal neutro entrambe le volte, cosi' si misura un ARMAMENTO e non il toggle: quello e' gia'
		// presidiato da `TheDockPortArmsAndDisarms` e qui sarebbe rumore.
		PC->SelectAbilityForCurrentForTest(INDEX_NONE);
		PC->ArmKitAbility(Azioni[i].AbilityIndex);
		const int32 DallaPorta = Unit->SelectedAbilityIndex;

		PC->SelectAbilityForCurrentForTest(INDEX_NONE);
		Slot->Activate();
		const int32 DalClick = Unit->SelectedAbilityIndex;

		TestEqual(
			FString::Printf(
				TEXT("B: il riquadro %d arriva dove arriva la porta chiamata con la posizione %d"),
				i, Azioni[i].AbilityIndex),
			DalClick, DallaPorta);

		Armate += (DalClick != INDEX_NONE) ? 1 : 0;
	}

	// ⚠️ **Anti-vacuita', e senza di essa il ciclo qui sopra sarebbe verde su un `Activate()` morto**:
	// due percorsi che non armano niente sono uguali a `INDEX_NONE` per ogni posizione.
	TestTrue(
		FString::Printf(
			TEXT("B: almeno un riquadro ha armato DAVVERO (%d su %d) — altrimenti l'uguaglianza qui sopra ")
			TEXT("confronterebbe due percorsi entrambi inerti"),
			Armate, Azioni.Num()),
		Armate > 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * `#3470` — IL BADGE `M` DELLA BARRA E IL TASTO `M` RAGGIUNGONO LA STESSA DICHIARAZIONE ([D-457]).
 *
 * 🔑 **L'oracolo e' la simmetria con il tasto, non un valore scritto qui**: il click dichiara, il tasto ritira,
 * e la riserva di `Overwatch` rifiuta entrambi. Se il badge avesse una regola propria, almeno uno dei tre
 * passaggi divergerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSneakBadgeParityTest,
	"RefactorTactics.PlayerInput.SneakBadgeAndKeyReachTheSameDeclaration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSneakBadgeParityTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	ARTUnit* Unit = SpawnUnitConKit(World, /*TeamId*/ 0);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	URTActionDockWidget* Dock = NewObject<URTActionDockWidget>(World);
	if (!Unit || !PC || !Dock)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("unita', controller e dock esistono"), false);
	}
	PC->SelectActorForTest(Unit);
	Dock->SetCommandControllerForTest(PC);
	const FName Sneak = URTMovementProfileLibrary::ProfileSneak;

	// A. il click dichiara
	Dock->ToggleSneak();
	TestEqual(TEXT("A: il click sul badge dichiara Sneak"), Unit->PlannedMovementProfileId, Sneak);

	// B. il tasto ritira la STESSA dichiarazione: le due strade sono un interruttore solo
	PC->ToggleSneakForTest();
	TestTrue(TEXT("B: il tasto M ritira cio' che il click ha dichiarato"), Unit->PlannedMovementProfileId.IsNone());

	// C. la riserva di Overwatch rifiuta il click come rifiuta il tasto
	int32 OverwatchIdx = INDEX_NONE;
	for (int32 i = 0; i < Unit->NumAbilities(); ++i)
	{
		const URTActionData* A = Unit->GetAbility(i);
		if (A && A->Def.ActionId == FName(TEXT("Action.Overwatch"))) { OverwatchIdx = i; break; }
	}
	if (TestTrue(TEXT("C: premessa — Overwatch e' nel kit"), OverwatchIdx != INDEX_NONE))
	{
		Unit->PlannedAbilityIndex = OverwatchIdx;
		Dock->ToggleSneak();
		TestTrue(TEXT("C: con lo slot riservato il click non dichiara"), Unit->PlannedMovementProfileId.IsNone());
		PC->ToggleSneakForTest();
		TestTrue(TEXT("C: ed e' lo stesso rifiuto del tasto"), Unit->PlannedMovementProfileId.IsNone());
		Unit->PlannedAbilityIndex = INDEX_NONE;
	}

	// D. senza controller il badge non fa nulla e non crolla
	URTActionDockWidget* Orfano = NewObject<URTActionDockWidget>(World);
	Orfano->ToggleSneak();
	TestTrue(TEXT("D: senza controller nessuna dichiarazione"), Unit->PlannedMovementProfileId.IsNone());

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * `#3471` — `CONFERMA` E' `INVIO`, `ANNULLA` E' IL BACK DEL TASTO DESTRO ([D-458]).
 *
 * 🔑 **Oracolo di PARITA'**: lo stesso stato portato due volte allo stesso punto, una volta col pulsante e una
 * col tasto, deve dare lo stesso risultato. Non si asserisce QUALE livello del Back si smonti — lo decide
 * `ResolveBack`, coi suoi test — ma che pulsante e tasto ne smontino lo stesso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanCommitParityTest,
	"RefactorTactics.PlayerInput.PlanCommitButtonsReachTheSamePortsAsTheKeys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanCommitParityTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	ARTUnit* Unit = SpawnUnitConKit(World, /*TeamId*/ 0);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	URTPlanCommitWidget* W = NewObject<URTPlanCommitWidget>(World);
	if (!Unit || !PC || !W)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("unita', controller e widget esistono"), false);
	}

	// E. prima di tutto, senza unita' i pulsanti non hanno a chi parlare
	W->SetCommandControllerForTest(PC);
	TestFalse(TEXT("E: senza selezione non c'e' un'unita' comandata"), W->HasCommandedUnit());
	TestFalse(TEXT("E: e nessun piano dichiarato"), W->IsPlanDeclared());
	W->Confirm();
	TestFalse(TEXT("E: Conferma senza unita' non dichiara niente"), Unit->bTurnPlanDeclared);

	PC->SelectActorForTest(Unit);
	W->SetSelectedUnitForTest(Unit);
	TestTrue(TEXT("A: con la selezione c'e' un'unita' comandata"), W->HasCommandedUnit());

	// B. Conferma dichiara, e il tasto Invio ritratta la STESSA dichiarazione
	W->Confirm();
	TestTrue(TEXT("B: Conferma dichiara il piano dell'unita'"), Unit->bTurnPlanDeclared);
	TestTrue(TEXT("B: e il widget lo legge"), W->IsPlanDeclared());
	PC->ToggleTurnPlanDeclaredForTest();
	TestFalse(TEXT("B: Invio ritratta cio' che Conferma ha dichiarato"), Unit->bTurnPlanDeclared);
	TestFalse(TEXT("B: e il widget lo legge"), W->IsPlanDeclared());

	// C. Annulla e il tasto smontano lo stesso livello del Back, dallo stesso stato
	const TArray<FRTCellId> Piano = { FRTCellId(1, 0, 0), FRTCellId(2, 0, 0) };
	Unit->PlannedWaypoints = Piano;
	W->Undo();
	const TArray<FRTCellId> DopoIlPulsante = Unit->PlannedWaypoints;
	Unit->PlannedWaypoints = Piano;
	PC->OnUndoWaypointForTest();
	const TArray<FRTCellId> DopoIlTasto = Unit->PlannedWaypoints;
	TestTrue(TEXT("C: premessa — il Back ha smontato qualcosa"), DopoIlPulsante.Num() < Piano.Num());
	TestTrue(TEXT("C: pulsante e tasto lasciano lo stesso piano"), DopoIlPulsante == DopoIlTasto);

	// D. le etichette vengono dalle sedi dei tasti, e quelle sedi sono i tasti di D-458
	TestEqual(TEXT("D: il tasto di Conferma e' Invio"), ARTPlayerController::DeclarePlanHotkey(), EKeys::Enter);
	TestEqual(TEXT("D: il tasto di Annulla e' BackSpace"), ARTPlayerController::UndoKeyboardHotkey(), EKeys::BackSpace);
	// [D-496] (#3633): l'etichetta e' il nome del BADGE del tasto mappato, in italiano. Fino a #3633 era il nome
	// dell'engine («Enter»); la sede resta il tasto, e la tabella ne traduce soltanto il nome.
	TestEqual(TEXT("D: l'etichetta di Conferma e' il badge del suo tasto"), W->GetConfirmKeyLabel().ToString(),
		ARTPlayerController::KeyBadgeLabel(EKeys::Enter).ToString());
	TestEqual(TEXT("D: e il badge di Invio dice INVIO"), W->GetConfirmKeyLabel().ToString(), FString(TEXT("INVIO")));
	TestEqual(TEXT("D: l'etichetta di Annulla e' il badge del suo tasto"), W->GetUndoKeyLabel().ToString(),
		ARTPlayerController::KeyBadgeLabel(EKeys::BackSpace).ToString());

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
