// Le viste dello Screen HUD (§4.1, CP 11.7): cosa i widget leggono, e cosa NON possono leggere.
//
// Il valore di queste funzioni non e' che risparmiano righe al widget — e' che gli tolgono la possibilita' di
// sbagliare. `BuildTeamRoster` non ha un parametro «mostra anche gli avversari», e `BuildMatchHeader` non ha
// un posto dove scrivere `12`: le due regole che il DoD di CP 11.1 e la spec §4.1 impongono diventano
// proprieta' della firma invece che disciplina.

#include "Misc/AutomationTest.h"
#include "UI/RTHudViewModel.h"
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Turn/RTTurnManager.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	UWorld* MakeHudVmWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyHudVmWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTUnit* SpawnHudVmUnit(UWorld* World, FName HeroId, int32 TeamId)
	{
		const URTHeroData* Hero = nullptr;
		for (const URTHeroData* H : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (H && H->HeroId == HeroId) { Hero = H; break; }
		}
		if (!World || !Hero) { return nullptr; }

		ARTUnit* Unit = World->SpawnActor<ARTUnit>();
		if (!Unit) { return nullptr; }
		Unit->ConfigureFromHeroData(Hero);
		Unit->TeamId = TeamId;
		return Unit;
	}
}

// ---------------------------------------------------------------------------------------------------------
// Il roster e' della PROPRIA squadra, e le morte restano
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmRosterIsOwnTeamTest,
	"RefactorTactics.HudViewModel.RosterIsOwnTeamOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmRosterIsOwnTeamTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTUnit* Gadget = SpawnHudVmUnit(World, TEXT("Hero.Gadget"), /*TeamId*/ 0);
	ARTUnit* Phase = SpawnHudVmUnit(World, TEXT("Hero.Phase"), /*TeamId*/ 0);
	ARTUnit* Riktor = SpawnHudVmUnit(World, TEXT("Hero.Riktor"), /*TeamId*/ 1);
	ARTUnit* Wraith = SpawnHudVmUnit(World, TEXT("Hero.Wraith"), /*TeamId*/ 1);

	if (!TestNotNull(TEXT("Gadget"), Gadget) || !TestNotNull(TEXT("Phase"), Phase)
		|| !TestNotNull(TEXT("Riktor"), Riktor) || !TestNotNull(TEXT("Wraith"), Wraith))
	{
		DestroyHudVmWorld(World);
		return false;
	}

	const TArray<ARTUnit*> All = { Gadget, Phase, Riktor, Wraith };

	const TArray<FRTUnitCardView> Mine = URTHudViewModel::BuildTeamRoster(All, /*PlayerTeamId*/ 0);
	TestEqual(TEXT("il roster ha le due unita' della mia squadra"), Mine.Num(), 2);
	for (const FRTUnitCardView& Card : Mine)
	{
		TestTrue(*FString::Printf(TEXT("%s e' un alleato"), *Card.HeroId.ToString()), Card.bIsAlly);
		TestFalse(TEXT("nessun avversario nel roster"),
			Card.HeroId == TEXT("Hero.Riktor") || Card.HeroId == TEXT("Hero.Wraith"));
	}

	// Simmetrico: cambiando squadra cambia il roster, e la funzione non ha altri parametri con cui sbagliare.
	const TArray<FRTUnitCardView> Theirs = URTHudViewModel::BuildTeamRoster(All, /*PlayerTeamId*/ 1);
	TestEqual(TEXT("dall'altra parte se ne vedono due"), Theirs.Num(), 2);

	// Una morta NON sparisce: il conto della squadra deve restare leggibile.
	Gadget->Health = 0;
	const TArray<FRTUnitCardView> AfterDeath = URTHudViewModel::BuildTeamRoster(All, /*PlayerTeamId*/ 0);
	TestEqual(TEXT("il roster resta di due voci anche con una unita' morta"), AfterDeath.Num(), 2);

	int32 Dead = 0;
	for (const FRTUnitCardView& Card : AfterDeath) { if (!Card.bAlive) { ++Dead; } }
	TestEqual(TEXT("e una e' marcata come non viva"), Dead, 1);

	DestroyHudVmWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// La carta riflette il simulatore, non una copia
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmCardMirrorsUnitTest,
	"RefactorTactics.HudViewModel.CardMirrorsSimulator",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmCardMirrorsUnitTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTUnit* Riktor = SpawnHudVmUnit(World, TEXT("Hero.Riktor"), /*TeamId*/ 0);
	if (!TestNotNull(TEXT("Riktor"), Riktor)) { DestroyHudVmWorld(World); return false; }

	Riktor->Health = 42;
	Riktor->Shield = 7;
	Riktor->Energy = 13;

	const FRTUnitCardView Card = URTHudViewModel::BuildUnitCard(Riktor, /*PlayerTeamId*/ 0);
	TestEqual(TEXT("salute"), Card.Health, 42);
	TestEqual(TEXT("scudo"), Card.Shield, 7);
	TestEqual(TEXT("energia"), Card.Energy, 13);
	TestEqual(TEXT("salute massima dal catalogo eroi"), Card.MaxHealth, Riktor->MaxHealth);
	TestEqual(TEXT("identita'"), Card.HeroId, Riktor->HeroId);
	TestTrue(TEXT("alleato"), Card.bIsAlly);
	TestTrue(TEXT("vivo"), Card.bAlive);

	// La stessa unita' vista dall'altra squadra e' la stessa unita': cambia la RELAZIONE, non i numeri.
	const FRTUnitCardView Enemy = URTHudViewModel::BuildUnitCard(Riktor, /*PlayerTeamId*/ 1);
	TestFalse(TEXT("vista da squadra 1 non e' alleata"), Enemy.bIsAlly);
	TestEqual(TEXT("ma la salute e' la stessa"), Enemy.Health, 42);

	// Nulla non e' «unita' a zero»: e' assenza. `bAlive` falso lo distingue da un'unita' morta con un nome.
	const FRTUnitCardView None = URTHudViewModel::BuildUnitCard(nullptr, /*PlayerTeamId*/ 0);
	TestFalse(TEXT("una carta senza unita' non e' viva"), None.bAlive);
	TestTrue(TEXT("e non ha identita'"), None.HeroId.IsNone());

	DestroyHudVmWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// L'intestazione senza manager e' neutra, non «zero»
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmHeaderWithoutManagerTest,
	"RefactorTactics.HudViewModel.HeaderWithoutManagerIsNeutral",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmHeaderWithoutManagerTest::RunTest(const FString&)
{
	const FRTMatchHeaderView View = URTHudViewModel::BuildMatchHeader(nullptr);

	TestEqual(TEXT("nessun round"), View.Round, 0);

	// ⚠️ La voce che conta: `RoundLimit` a 0 significa «non dichiarato», e un widget deve poterlo distinguere
	// da un limite vero. E' la stessa ragione per cui `ARTHUD` oggi stampa «Turno n» senza «/0».
	TestEqual(TEXT("nessun limite dichiarato"), View.RoundLimit, 0);

	// Negativo, non zero: «la domanda non si applica» non e' «il tempo e' scaduto».
	TestTrue(TEXT("il timer non si applica"), View.PlanningSecondsRemaining < 0.f);
	TestFalse(TEXT("non si sta risolvendo"), View.bResolving);

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Un Planning SENZA orologio non e' un Planning scaduto
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmUntimedPlanningTest,
	"RefactorTactics.HudViewModel.UntimedPlanningIsNotExpired",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmUntimedPlanningTest::RunTest(const FString&)
{
	// ⚠️ Questo test esiste per un difetto trovato in code review, non per completezza.
	//
	// `GetPlanningTimeRemaining()` clampa a `0.f`, e restituisce quello stesso `0.f` in DUE situazioni che il
	// giocatore vede in modo opposto: il tempo e' finito, oppure non c'e' mai stato un tempo. Il timer si
	// imposta solo `if (PlanningSeconds > 0.f)`, e `RTScenarioSession` usa `SetPlanningSeconds(0.f)` per le
	// run headless — quindi il secondo caso e' un percorso reale, non un'ipotesi.
	//
	// Il test precedente non poteva accorgersene: passando `nullptr` riceveva i default della struct senza
	// mai attraversare il ramo del timer.
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHudVmWorld(World); return false; }

	// Nessun orologio: e' cio' che fa l'harness headless.
	TM->SetPlanningSeconds(0.f);

	const FRTMatchHeaderView Untimed = URTHudViewModel::BuildMatchHeader(TM);
	TestEqual(TEXT("siamo in Planning"), Untimed.Phase, ERTMatchPhase::Planning);
	TestTrue(TEXT("senza orologio il timer NON si applica, non e' scaduto"),
		Untimed.PlanningSecondsRemaining < 0.f);

	// Con un orologio vero la domanda si applica, e la risposta non e' negativa.
	TM->SetPlanningSeconds(30.f);
	const FRTMatchHeaderView Timed = URTHudViewModel::BuildMatchHeader(TM);
	TestTrue(TEXT("con un orologio il tempo residuo e' un numero utilizzabile"),
		Timed.PlanningSecondsRemaining >= 0.f);

	DestroyHudVmWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// CP 11.1 — gli SLOT occupati derivano dal piano, e `MovementAndMain` ne occupa due
// ---------------------------------------------------------------------------------------------------------

/**
 * A parita' di piano, la stessa terna. E soprattutto: un'azione che dichiara `MovementAndMain` occupa
 * **entrambi** gli slot, che e' il caso in cui la mappatura ovvia «un'azione, uno slot» sbaglia.
 *
 * Senza quest'ultima meta' il pannello mostrerebbe il movimento libero a chi ha speso tutto il turno per
 * correre — e chi legge lo schermo pianificherebbe un movimento che il validatore poi rifiuta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmSlotsTest,
	"RefactorTactics.HudViewModel.SlotsDeriveFromThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmSlotsTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTUnit* Unit = SpawnHudVmUnit(World, TEXT("Hero.Gadget"), 0);
	if (!TestNotNull(TEXT("unita'"), Unit)) { DestroyHudVmWorld(World); return false; }

	// 1. Piano vuoto: tre slot liberi. Un'unita' appena selezionata non deve sembrare gia' impegnata.
	{
		const FRTUnitSlotsView Slots = URTHudViewModel::BuildUnitSlots(Unit);
		TestFalse(TEXT("movimento libero"), Slots.Movement.bOccupied);
		TestFalse(TEXT("principale libera"), Slots.Main.bOccupied);
		TestFalse(TEXT("reazione libera"), Slots.Reaction.bOccupied);
	}

	// 2. Un PERCORSO occupa il movimento e non ha un ActionId: e' la distinzione che il tipo dichiara.
	{
		Unit->PlannedWaypoints.Add(FRTCellId(1, 0, 0));
		const FRTUnitSlotsView Slots = URTHudViewModel::BuildUnitSlots(Unit);
		TestTrue(TEXT("un percorso occupa il movimento"), Slots.Movement.bOccupied);
		TestTrue(TEXT("e non lo nomina: un percorso non e' un'azione scelta"),
			Slots.Movement.ActionId.IsNone());
		Unit->PlannedWaypoints.Reset();
	}

	// 3. Un'azione principale occupa la principale, e la NOMINA.
	{
		int32 MainIdx = INDEX_NONE;
		for (int32 i = 0; i < Unit->NumAbilities(); ++i)
		{
			const URTActionData* A = Unit->GetAbility(i);
			if (A && A->Def.Slot == ERTActionSlot::Main) { MainIdx = i; break; }
		}
		if (TestTrue(TEXT("premessa: il kit ha un'azione principale"), MainIdx != INDEX_NONE))
		{
			Unit->PlannedAbilityIndex = MainIdx;
			const FRTUnitSlotsView Slots = URTHudViewModel::BuildUnitSlots(Unit);
			TestTrue(TEXT("la principale risulta occupata"), Slots.Main.bOccupied);
			TestEqual(TEXT("e porta l'ActionId dell'azione scelta"),
				Slots.Main.ActionId, Unit->GetAbility(MainIdx)->Def.ActionId);
			TestFalse(TEXT("il movimento resta libero: una Main non lo tocca"), Slots.Movement.bOccupied);
			Unit->PlannedAbilityIndex = INDEX_NONE;
		}
	}

	// 4. 🔴 `MovementAndMain`: UN'azione, DUE slot. E' il caso che rende sbagliata la mappatura ovvia.
	//    L'azione si costruisce qui invece di cercarla nel kit: nessun eroe del roster ne possiede una
	//    (`Action.Sprint` e' nel catalogo core), e un test che dipendesse da questo resterebbe verde
	//    smettendo di verificare il giorno in cui il kit cambia.
	{
		URTActionData* Sprint = NewObject<URTActionData>(Unit);
		Sprint->Def.ActionId = TEXT("Test.SprintLike");
		Sprint->Def.Slot = ERTActionSlot::MovementAndMain;
		Sprint->DisplayName = FText::FromString(TEXT("Scatto lungo"));
		Unit->Abilities.Add(Sprint);
		const int32 SprintIdx = Unit->NumAbilities() - 1;

		Unit->PlannedAbilityIndex = SprintIdx;
		const FRTUnitSlotsView Slots = URTHudViewModel::BuildUnitSlots(Unit);
		TestTrue(TEXT("MovementAndMain occupa la principale"), Slots.Main.bOccupied);
		TestTrue(TEXT("e ANCHE il movimento"), Slots.Movement.bOccupied);
		TestEqual(TEXT("il movimento e' nominato dalla stessa azione"),
			Slots.Movement.ActionId, FName(TEXT("Test.SprintLike")));

		Unit->PlannedAbilityIndex = INDEX_NONE;
		Unit->Abilities.Pop();
	}

	// 5. La reazione e' uno slot INDIPENDENTE: non tocca gli altri due.
	{
		int32 ReactionIdx = INDEX_NONE;
		for (int32 i = 0; i < Unit->NumAbilities(); ++i)
		{
			const URTActionData* A = Unit->GetAbility(i);
			if (A && A->Def.Slot == ERTActionSlot::Reaction) { ReactionIdx = i; break; }
		}
		if (TestTrue(TEXT("premessa: il kit ha una reazione"), ReactionIdx != INDEX_NONE))
		{
			Unit->PlannedReactionAbility = ReactionIdx;
			const FRTUnitSlotsView Slots = URTHudViewModel::BuildUnitSlots(Unit);
			TestTrue(TEXT("la reazione risulta armata"), Slots.Reaction.bOccupied);
			TestFalse(TEXT("e non occupa il movimento"), Slots.Movement.bOccupied);
			TestFalse(TEXT("ne' la principale"), Slots.Main.bOccupied);
		}
	}

	DestroyHudVmWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// CP 11.1 — i COOLDOWN vengono dal simulatore, sono interi e non negativi
// ---------------------------------------------------------------------------------------------------------

/**
 * La vista non tiene una copia: legge `ARTUnit`. Il test lo dimostra **muovendo il simulatore** e
 * ricostruendo la vista — se ci fosse una copia, il secondo valore sarebbe uguale al primo.
 *
 * ⚠️ E `bUsableNow` non e' `TurnsRemaining == 0`: serve anche l'energia. Sono due domande diverse, e un
 * widget che ne mostrasse una sola direbbe «pronta» di un'ultimate che non si puo' lanciare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmCooldownTest,
	"RefactorTactics.HudViewModel.CooldownsMirrorTheSimulator",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmCooldownTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTUnit* Unit = SpawnHudVmUnit(World, TEXT("Hero.Gadget"), 0);
	if (!TestNotNull(TEXT("unita'"), Unit)) { DestroyHudVmWorld(World); return false; }

	// Una riga per ogni azione del kit, nell'ordine del kit: l'indice serve all'hotkey.
	{
		const TArray<FRTAbilityCooldownView> Cds = URTHudViewModel::BuildAbilityCooldowns(Unit);
		TestEqual(TEXT("una riga per azione del kit"), Cds.Num(), Unit->NumAbilities());
		for (int32 i = 0; i < Cds.Num(); ++i)
		{
			TestEqual(TEXT("l'indice e' quello del kit"), Cds[i].AbilityIndex, i);
			TestTrue(TEXT("mai negativo"), Cds[i].TurnsRemaining >= 0);
			TestEqual(TEXT("e coincide con il simulatore"),
				Cds[i].TurnsRemaining, FMath::Max(0, Unit->GetAbilityCooldown(i)));
		}
	}

	// Si muove il SIMULATORE e la vista lo segue: e' la prova che non c'e' una copia.
	{
		int32 Idx = INDEX_NONE;
		for (int32 i = 0; i < Unit->NumAbilities(); ++i)
		{
			const URTActionData* A = Unit->GetAbility(i);
			if (A && A->CooldownTurns > 0) { Idx = i; break; }
		}
		if (TestTrue(TEXT("premessa: il kit ha un'azione con ricarica"), Idx != INDEX_NONE))
		{
			const int32 Before = URTHudViewModel::BuildAbilityCooldowns(Unit)[Idx].TurnsRemaining;
			Unit->ConsumeAbility(Idx);
			const TArray<FRTAbilityCooldownView> After = URTHudViewModel::BuildAbilityCooldowns(Unit);

			TestTrue(TEXT("dopo l'uso la ricarica e' salita"), After[Idx].TurnsRemaining > Before);
			TestEqual(TEXT("e vale quella del simulatore"),
				After[Idx].TurnsRemaining, Unit->GetAbilityCooldown(Idx));
			TestFalse(TEXT("un'azione in ricarica non e' usabile adesso"), After[Idx].bUsableNow);
		}
	}

	// Unita' nulla: elenco vuoto, non una riga fantasma.
	TestEqual(TEXT("nessuna unita' -> nessuna riga"),
		URTHudViewModel::BuildAbilityCooldowns(nullptr).Num(), 0);

	DestroyHudVmWorld(World);
	return true;
}

/**
 * `#1896` — LA BARRA DI RICARICA NON RICHIEDE UNA DIVISIONE AL WIDGET.
 *
 * Il difetto misurato in PIE: `WBP_RT_UnitCard` divideva per il totale di ricarica, che per la maggior
 * parte del kit vale **0**, e ne usciva un `Divide by zero: Divide_DoubleDouble` a ogni selezione. In
 * Blueprint quella divisione non si ferma — restituisce 0 — quindi il difetto **non** e' il messaggio: e'
 * una barra che dichiara «scarica» un'abilita' pronta.
 *
 * ⛔ Questo test **non riproduce il warning**, che nasce in un nodo dentro il `.uasset` e headless non
 * viene nemmeno costruito. Prova la cosa che rende il warning impossibile: che la vista porti gia' il
 * risultato, con la guardia sullo zero applicata in un posto solo e misurabile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmChargeFractionTest,
	"RefactorTactics.HudViewModel.ChargeFractionNeedsNoDivisionInTheWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmChargeFractionTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTUnit* Unit = SpawnHudVmUnit(World, TEXT("Hero.Gadget"), 0);
	if (!TestNotNull(TEXT("unita'"), Unit)) { DestroyHudVmWorld(World); return false; }

	const TArray<FRTAbilityCooldownView> Cds = URTHudViewModel::BuildAbilityCooldowns(Unit);
	if (!TestTrue(TEXT("premessa: il kit non e' vuoto"), Cds.Num() > 0))
	{
		DestroyHudVmWorld(World);
		return false;
	}

	// PREMESSA DEL DIFETTO: esiste almeno un'azione a ricarica zero. Senza, questo test proverebbe la
	// guardia su un caso che non si presenta mai — e resterebbe verde anche togliendola.
	int32 SenzaRicarica = 0;
	for (const FRTAbilityCooldownView& V : Cds)
	{
		if (V.TotalTurns == 0) { ++SenzaRicarica; }
	}
	if (!TestTrue(TEXT("premessa: il kit ha almeno un'azione senza ricarica (il caso che divideva per zero)"),
			SenzaRicarica > 0))
	{
		DestroyHudVmWorld(World);
		return false;
	}

	for (const FRTAbilityCooldownView& V : Cds)
	{
		const FString Chi = V.ActionId.ToString();

		// Il contratto della vista, su OGNI riga: la frazione e' sempre disegnabile.
		TestTrue(FString::Printf(TEXT("%s: la frazione sta in [0,1]"), *Chi),
			V.ChargeFraction >= 0.f && V.ChargeFraction <= 1.f);
		TestTrue(FString::Printf(TEXT("%s: il totale non e' negativo"), *Chi), V.TotalTurns >= 0);

		if (V.TotalTurns == 0)
		{
			// 🔴 IL CASO DEL DIFETTO: senza ricarica la barra e' PIENA, non vuota.
			TestEqual(FString::Printf(TEXT("%s: senza ricarica la barra e' piena, non a zero"), *Chi),
				V.ChargeFraction, 1.f);
		}
	}

	// La frazione SEGUE il simulatore: si consuma un'azione con ricarica e la barra deve scendere.
	int32 Idx = INDEX_NONE;
	for (int32 i = 0; i < Cds.Num(); ++i)
	{
		if (Cds[i].TotalTurns > 0) { Idx = i; break; }
	}
	if (TestTrue(TEXT("premessa: il kit ha anche un'azione CON ricarica"), Idx != INDEX_NONE))
	{
		TestEqual(TEXT("prima dell'uso la barra e' piena"), Cds[Idx].ChargeFraction, 1.f);

		Unit->ConsumeAbility(Idx);
		const TArray<FRTAbilityCooldownView> Dopo = URTHudViewModel::BuildAbilityCooldowns(Unit);

		TestTrue(TEXT("dopo l'uso la barra e' scesa"), Dopo[Idx].ChargeFraction < 1.f);
		TestTrue(TEXT("e resta disegnabile"),
			Dopo[Idx].ChargeFraction >= 0.f && Dopo[Idx].ChargeFraction <= 1.f);

		// Il valore si DERIVA, non si scrive a mano: un letterale qui resterebbe vero solo per la ricarica
		// che quell'azione ha oggi, e diventerebbe falso in silenzio a un ribilanciamento.
		const float Atteso = 1.f - static_cast<float>(Dopo[Idx].TurnsRemaining)
			/ static_cast<float>(Dopo[Idx].TotalTurns);
		TestEqual(TEXT("e vale quanto la ricarica residua dichiara"), Dopo[Idx].ChargeFraction, Atteso);

		// Il totale non si muove con l'uso: e' la ricarica DICHIARATA dall'azione, non quella residua.
		TestEqual(TEXT("il totale non cambia usando l'azione"), Dopo[Idx].TotalTurns, Cds[Idx].TotalTurns);
	}

	DestroyHudVmWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// CP 11.1 — il limite di round viene dal FORMATO, non da una costante
// ---------------------------------------------------------------------------------------------------------

/**
 * Cambiando `RoundLimit` nelle regole di formato, il valore della vista cambia. E' la voce di DoD che esiste
 * per impedire a un widget di stampare `12` — il valore del formato di ripiego, non una costante del gioco.
 *
 * Il test usa **due** valori e nessuno dei due e' `12`: pinnare il default renderebbe verde anche una vista
 * che restituisce sempre la stessa costante.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmRoundLimitTest,
	"RefactorTactics.HudViewModel.RoundLimitComesFromTheFormat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmRoundLimitTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHudVmWorld(World); return false; }

	{
		FRTMatchRules Rules;
		Rules.RoundLimit = 7;
		TM->SetMatchRules(Rules);
		TestEqual(TEXT("il limite e' quello del formato (7)"),
			URTHudViewModel::BuildMatchHeader(TM).RoundLimit, 7);
	}

	{
		FRTMatchRules Rules;
		Rules.RoundLimit = 14;
		TM->SetMatchRules(Rules);
		TestEqual(TEXT("cambiato il formato, cambia la vista (14)"),
			URTHudViewModel::BuildMatchHeader(TM).RoundLimit, 14);
	}

	// `0` non e' «scaduta»: e' «nessun limite dichiarato», e la vista lo distingue perche' il widget deve
	// poter mostrare «Round 3» invece di «Round 3/0».
	{
		FRTMatchRules Rules;
		Rules.RoundLimit = 0;
		TM->SetMatchRules(Rules);
		TestEqual(TEXT("nessun limite dichiarato resta 0"),
			URTHudViewModel::BuildMatchHeader(TM).RoundLimit, 0);
	}

	DestroyHudVmWorld(World);
	return true;
}

/**
 * `CP 10.2` / `#75` — il progresso sull'obiettivo arriva fino all'HUD.
 *
 * La DoD di CP 10.2 chiede che il progresso sia «un intero, mai un float, e compaia **nell'HUD e nel
 * TurnLog**». La metà nel TurnLog era soddisfatta da quando il Cleanup registra `Objective.Control`; la
 * metà nell'HUD no: `GetTeamScore` aveva per lettori lo Scenario Harness e l'hash di stato, e nessun
 * widget. Un punteggio che nessuno mostra è un punteggio che il giocatore non può contendere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmObjectiveScoreTest,
	"RefactorTactics.HudViewModel.ObjectiveProgressReachesTheHud",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmObjectiveScoreTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHudVmWorld(World); return false; }

	// A zero punti la vista dice zero, non «nessun dato»: una partita appena cominciata ha un punteggio, ed
	// è `0 — 0`.
	{
		const FRTMatchHeaderView View = URTHudViewModel::BuildMatchHeader(TM);
		TestEqual(TEXT("all'inizio la squadra 0 e' a zero"), View.Team0Score, 0);
		TestEqual(TEXT("e anche la squadra 1"), View.Team1Score, 0);
	}

	// Il progresso si vede, e si vede SEPARATO per squadra: un solo numero non distinguerebbe «2 a 0» da
	// «0 a 2», che è esattamente la differenza che un obiettivo conteso produce.
	{
		TM->AddTeamScore(0, 1);
		TM->AddTeamScore(0, 1);
		TM->AddTeamScore(1, 1);
		const FRTMatchHeaderView View = URTHudViewModel::BuildMatchHeader(TM);
		TestEqual(TEXT("la squadra 0 ha due punti"), View.Team0Score, 2);
		TestEqual(TEXT("la squadra 1 ne ha uno"), View.Team1Score, 1);
	}

	// ⚠️ **La vista dice la stessa cosa del simulatore, non una cosa simile.** È la proprieta' che rende il
	// numero mostrato verificabile contro il TurnLog: se qui comparisse una conversione, un arrotondamento o
	// un `float`, l'HUD e il log potrebbero divergere senza che nessun test se ne accorga.
	{
		const FRTMatchHeaderView View = URTHudViewModel::BuildMatchHeader(TM);
		TestEqual(TEXT("la vista rispecchia il simulatore, squadra 0"), View.Team0Score, TM->GetTeamScore(0));
		TestEqual(TEXT("la vista rispecchia il simulatore, squadra 1"), View.Team1Score, TM->GetTeamScore(1));
	}

	// `ScoreToWin` viene dal FORMATO, come `RoundLimit`: senza di lui un widget non sa se `2` sia molto o
	// niente.
	{
		FRTMatchRules Rules;
		Rules.ScoreToWin = 3;
		TM->SetMatchRules(Rules);
		TestEqual(TEXT("la soglia e' quella del formato (3)"),
			URTHudViewModel::BuildMatchHeader(TM).ScoreToWin, 3);
	}

	// ⚠️ `0` è il valore della v0.1 e significa «via per obiettivo DISATTIVATA», non «si vince a zero
	// punti». La vista lo tiene distinto per la stessa ragione per cui tiene distinto `RoundLimit = 0`: un
	// widget deve poter mostrare «2» invece di «2/0».
	{
		FRTMatchRules Rules;
		Rules.ScoreToWin = 0;
		TM->SetMatchRules(Rules);
		const FRTMatchHeaderView View = URTHudViewModel::BuildMatchHeader(TM);
		TestEqual(TEXT("via disattivata: la soglia resta 0"), View.ScoreToWin, 0);
		TestEqual(TEXT("ma il progresso continua a vedersi"), View.Team0Score, 2);
	}

	// Senza manager la vista è NEUTRA, e per il punteggio «neutro» è zero — come per il round. Il caso
	// esiste gia' in `HeaderWithoutManagerIsNeutral`; qui si fissa che i campi nuovi non lo rompano.
	{
		const FRTMatchHeaderView View = URTHudViewModel::BuildMatchHeader(nullptr);
		TestEqual(TEXT("senza manager, squadra 0 a zero"), View.Team0Score, 0);
		TestEqual(TEXT("senza manager, squadra 1 a zero"), View.Team1Score, 0);
		TestEqual(TEXT("senza manager, nessuna soglia"), View.ScoreToWin, 0);
	}

	DestroyHudVmWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
