// Le viste dello Screen HUD (§4.1, CP 11.7): cosa i widget leggono, e cosa NON possono leggere.
//
// Il valore di queste funzioni non e' che risparmiano righe al widget — e' che gli tolgono la possibilita' di
// sbagliare. `BuildTeamRoster` non ha un parametro «mostra anche gli avversari», e `BuildMatchHeader` non ha
// un posto dove scrivere `12`: le due regole che il DoD di CP 11.1 e la spec §4.1 impongono diventano
// proprieta' della firma invece che disciplina.

#include "Misc/AutomationTest.h"
#include "UI/RTHudViewModel.h"
#include "UI/RTIconLibrary.h"      // MakeIconId: la chiave che la vista porta gia' derivata (#2274)
#include "UI/RTUnitOverlayWidget.h" // ComposeStatusDurationLabel: la regola di formato di uno stato (#2336)
#include "Core/RTGameplayTags.h"   // i tag di stato usati dai test dei badge (#2274)
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Player/RTPlayerController.h" // AbilityHotkeys / HotkeyLabelFor: l ORACOLO del tasto (#2987)
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

	ARTUnit* Aevik = SpawnHudVmUnit(World, TEXT("Hero.Aevik"), /*TeamId*/ 0);
	ARTUnit* Muiren = SpawnHudVmUnit(World, TEXT("Hero.Muiren"), /*TeamId*/ 0);
	ARTUnit* Branth = SpawnHudVmUnit(World, TEXT("Hero.Branth"), /*TeamId*/ 1);
	ARTUnit* Ivrin = SpawnHudVmUnit(World, TEXT("Hero.Ivrin"), /*TeamId*/ 1);

	if (!TestNotNull(TEXT("Aevik"), Aevik) || !TestNotNull(TEXT("Muiren"), Muiren)
		|| !TestNotNull(TEXT("Branth"), Branth) || !TestNotNull(TEXT("Ivrin"), Ivrin))
	{
		DestroyHudVmWorld(World);
		return false;
	}

	const TArray<ARTUnit*> All = { Aevik, Muiren, Branth, Ivrin };

	const TArray<FRTUnitCardView> Mine = URTHudViewModel::BuildTeamRoster(All, /*PlayerTeamId*/ 0);
	TestEqual(TEXT("il roster ha le due unita' della mia squadra"), Mine.Num(), 2);
	for (const FRTUnitCardView& Card : Mine)
	{
		TestTrue(*FString::Printf(TEXT("%s e' un alleato"), *Card.HeroId.ToString()), Card.bIsAlly);
		TestFalse(TEXT("nessun avversario nel roster"),
			Card.HeroId == TEXT("Hero.Branth") || Card.HeroId == TEXT("Hero.Ivrin"));
	}

	// Simmetrico: cambiando squadra cambia il roster, e la funzione non ha altri parametri con cui sbagliare.
	const TArray<FRTUnitCardView> Theirs = URTHudViewModel::BuildTeamRoster(All, /*PlayerTeamId*/ 1);
	TestEqual(TEXT("dall'altra parte se ne vedono due"), Theirs.Num(), 2);

	// Una morta NON sparisce: il conto della squadra deve restare leggibile.
	Aevik->Health = 0;
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

	ARTUnit* Branth = SpawnHudVmUnit(World, TEXT("Hero.Branth"), /*TeamId*/ 0);
	if (!TestNotNull(TEXT("Branth"), Branth)) { DestroyHudVmWorld(World); return false; }

	Branth->Health = 42;
	Branth->Shield = 7;

	const FRTUnitCardView Card = URTHudViewModel::BuildUnitCard(Branth, /*PlayerTeamId*/ 0);
	TestEqual(TEXT("salute"), Card.Health, 42);
	TestEqual(TEXT("scudo"), Card.Shield, 7);
	TestEqual(TEXT("salute massima dal catalogo eroi"), Card.MaxHealth, Branth->MaxHealth);
	TestEqual(TEXT("identita'"), Card.HeroId, Branth->HeroId);
	TestTrue(TEXT("alleato"), Card.bIsAlly);
	TestTrue(TEXT("vivo"), Card.bAlive);

	// La stessa unita' vista dall'altra squadra e' la stessa unita': cambia la RELAZIONE, non i numeri.
	const FRTUnitCardView Enemy = URTHudViewModel::BuildUnitCard(Branth, /*PlayerTeamId*/ 1);
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

	ARTUnit* Unit = SpawnHudVmUnit(World, TEXT("Hero.Aevik"), 0);
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

	ARTUnit* Unit = SpawnHudVmUnit(World, TEXT("Hero.Aevik"), 0);
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

	ARTUnit* Unit = SpawnHudVmUnit(World, TEXT("Hero.Aevik"), 0);
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

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
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

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
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

// ---------------------------------------------------------------------------------------------------
// Quali stati mostrare, in che ordine, con quale durata (`#2274`, `D-320`).
//
// Il giudizio sta qui e non nel widget perche' un `UserWidget` in Blueprint ha copertura headless zero.
// ---------------------------------------------------------------------------------------------------

/**
 * 🔴 **I controlli vengono per primi, e la gravita' NON e' ricopiata: si chiede a chi la possiede.**
 *
 * `ARTHUD::DrawHUD` mostrava `ROOT` e poi `SLOW` in un `if`/`else if` — lo stesso ordine che
 * `URTReactionLibrary::ControlStatusesBySeverity` dichiara, scritto una seconda volta. Questo test cade se
 * l'ordine torna a essere una copia: e' costruito **al contrario** della gravita' attesa (`Slow` applicato
 * prima di `Root`, e `Burning` prima di entrambi in ordine alfabetico), quindi solo un riordino vero lo
 * soddisfa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusBadgesControlsComeFirstTest,
	"RefactorTactics.HudViewModel.StatusBadgesControlsComeFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusBadgesControlsComeFirstTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTUnit* Unit = SpawnHudVmUnit(World, TEXT("Hero.Aevik"), /*TeamId*/ 0);
	if (!TestNotNull(TEXT("unita'"), Unit)) { DestroyHudVmWorld(World); return false; }

	// Applicati DELIBERATAMENTE nell'ordine sbagliato: `Burning` precede entrambi in alfabetico, e `Slow`
	// e' meno grave di `Root`. Se la funzione non riordinasse, uscirebbero cosi' come sono entrati.
	Unit->ApplyStatus(TAG_Status_Burning, /*Turni*/ 2);
	Unit->ApplyStatus(TAG_Status_Slow,    /*Turni*/ 3);
	Unit->ApplyStatus(TAG_Status_Root,    /*Turni*/ 1);

	const TArray<FRTStatusBadgeView> Badges = URTHudViewModel::BuildStatusBadges(Unit);

	if (!TestEqual(TEXT("tre stati attivi, tre badge"), Badges.Num(), 3))
	{
		DestroyHudVmWorld(World);
		return false;
	}

	// 🔴 Il cuore: `Root` (rango 0) prima di `Slow` (rango 1), ed entrambi prima di cio' che non e' controllo.
	TestEqual(TEXT("primo: Root, il controllo piu' grave"),
		Badges[0].Tag, TAG_Status_Root.GetTag().GetTagName());
	TestEqual(TEXT("secondo: Slow, il controllo meno grave"),
		Badges[1].Tag, TAG_Status_Slow.GetTag().GetTagName());
	TestEqual(TEXT("terzo: Burning, che controllo non e'"),
		Badges[2].Tag, TAG_Status_Burning.GetTag().GetTagName());

	TestTrue(TEXT("Root e' marcato come controllo"),  Badges[0].bIsControl);
	TestTrue(TEXT("Slow e' marcato come controllo"),  Badges[1].bIsControl);
	TestFalse(TEXT("Burning NON e' un controllo"),    Badges[2].bIsControl);

	// La durata e' quella vera, non un numero qualsiasi.
	TestEqual(TEXT("Root: un turno residuo"),    Badges[0].RemainingTurns, 1);
	TestEqual(TEXT("Slow: tre turni residui"),   Badges[1].RemainingTurns, 3);
	TestEqual(TEXT("Burning: due turni residui"), Badges[2].RemainingTurns, 2);

	// L'icona arriva gia' derivata: chi disegna non compone chiavi.
	TestEqual(TEXT("l'IconId e' derivato dal tag"),
		Badges[0].IconId, URTIconLibrary::MakeIconId(TAG_Status_Root.GetTag().GetTagName()));

	DestroyHudVmWorld(World);
	return true;
}

/**
 * 🔴 **Uno stato legato alla cella non porta un conteggio, e il `-1` non attraversa il confine.**
 *
 * `ARTUnit::PersistentWhileOnCell` vale `-1`: stampato sopra la testa di un'unita' si leggerebbe «meno un
 * turno». La vista lo traduce in `bCellBound`, che dice la stessa cosa senza fingere di essere un numero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusBadgesCellBoundHasNoCountTest,
	"RefactorTactics.HudViewModel.StatusBadgesCellBoundHasNoCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusBadgesCellBoundHasNoCountTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTUnit* Unit = SpawnHudVmUnit(World, TEXT("Hero.Aevik"), /*TeamId*/ 0);
	if (!TestNotNull(TEXT("unita'"), Unit)) { DestroyHudVmWorld(World); return false; }

	Unit->ApplyStatus(TAG_Status_Wet, ARTUnit::PersistentWhileOnCell);

	const TArray<FRTStatusBadgeView> Badges = URTHudViewModel::BuildStatusBadges(Unit);
	if (!TestEqual(TEXT("un solo stato attivo"), Badges.Num(), 1))
	{
		DestroyHudVmWorld(World);
		return false;
	}

	TestTrue(TEXT("Wet dall'acqua e' legato alla cella"), Badges[0].bCellBound);
	// 🔴 Il cuore: `0`, non `-1`. Il valore sentinella resta dentro `ARTUnit`.
	TestEqual(TEXT("legato alla cella: nessun conteggio da mostrare"), Badges[0].RemainingTurns, 0);
	TestNotEqual(TEXT("il -1 non attraversa il confine"),
		Badges[0].RemainingTurns, ARTUnit::PersistentWhileOnCell);

	// L'altra meta': lo stesso tag puo' essere ANCHE a termine, e allora vince la durata che scade — perche'
	// e' l'unica che chi guarda puo' vedere scendere.
	Unit->ApplyStatus(TAG_Status_Wet, /*Turni*/ 2);
	const TArray<FRTStatusBadgeView> Dopo = URTHudViewModel::BuildStatusBadges(Unit);
	if (TestEqual(TEXT("resta un solo badge: le due nature non lo duplicano"), Dopo.Num(), 1))
	{
		TestFalse(TEXT("con una durata a termine non e' piu' 'finche' resti li''"), Dopo[0].bCellBound);
		TestEqual(TEXT("e il conteggio e' quello a termine"), Dopo[0].RemainingTurns, 2);
	}

	DestroyHudVmWorld(World);
	return true;
}

/**
 * 🔴 **La vista mostra ESATTAMENTE cio' che l'unita' porta: uno stato non attivo non compare.**
 *
 * ⚠️ **Questo test e' stato riscritto perche' la prima stesura affermava una cosa falsa**, e il rosso l'ha
 * mostrato. Diceva *«`Status.Electrified` non compare mai»* e lo applicava con `ApplyStatus(Tag, 1)`
 * aspettandosi che venisse rifiutato. Non lo e': `ApplyStatus` ritorna in silenzio per `Turns <= 0`, ma con
 * una durata **positiva** applica qualunque tag — l'inerzia di `Electrified` (`#1324`) sta nel fatto che
 * **nessun produttore lo chiama con una durata**, non in un rifiuto di questa API.
 *
 * 🔑 Ne segue dove vive davvero la garanzia, e vale saperlo prima di cercarla qui: **a monte**, nel
 * produttore — che per la scarica emette `AppliedInstantly` e non tocca `StatusTurns` — ed e' coperta da
 * `RTElectricPropagationTests`. `BuildStatusBadges` non ha un filtro su `Electrified` e non deve averlo:
 * mostra cio' che l'unita' porta, e l'unita' non lo porta.
 *
 * ⚠️ **Anti-vacuita'**: si applica anche uno stato vero, altrimenti «non compare» sarebbe verde in un mondo
 * dove la funzione non restituisce mai niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusBadgesShowOnlyWhatTheUnitCarriesTest,
	"RefactorTactics.HudViewModel.StatusBadgesShowOnlyWhatTheUnitCarries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusBadgesShowOnlyWhatTheUnitCarriesTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTUnit* Unit = SpawnHudVmUnit(World, TEXT("Hero.Aevik"), /*TeamId*/ 0);
	if (!TestNotNull(TEXT("unita'"), Unit)) { DestroyHudVmWorld(World); return false; }

	Unit->ApplyStatus(TAG_Status_Burning, /*Turni*/ 2);

	const TArray<FRTStatusBadgeView> Badges = URTHudViewModel::BuildStatusBadges(Unit);

	// Anti-vacuita': lo stato applicato esce davvero.
	TestEqual(TEXT("anti-vacuita': lo stato attivo c'e'"), Badges.Num(), 1);

	// 🔴 Il cuore: uno stato che l'unita' NON porta non compare — nemmeno uno che esiste nel registro dei
	// tag e ha la sua icona nel catalogo, come `Electrified`.
	const bool bHaNonApplicato = Badges.ContainsByPredicate([](const FRTStatusBadgeView& B)
	{
		return B.Tag == TAG_Status_Electrified.GetTag().GetTagName()
			|| B.Tag == TAG_Status_Root.GetTag().GetTagName();
	});
	TestFalse(TEXT("uno stato non attivo non compare"), bHaNonApplicato);

	// E la porta che lo dice, interrogata direttamente: `0` significa «non attivo», e non e' una durata.
	TestEqual(TEXT("un tag non attivo ha durata residua 0"),
		Unit->GetStatusRemainingTurns(TAG_Status_Root), 0);

	DestroyHudVmWorld(World);
	return true;
}

/**
 * 🔴 **La vista della sovrapposizione unisce i due produttori, e il colore dipende da CHI GUARDA.**
 *
 * `BuildUnitOverlay` non calcola nulla di nuovo: e' il collante fra `BuildUnitCard` e `BuildStatusBadges`.
 * Cio' che vale testare e' proprio il collante — che i due arrivino davvero, e che il nome e il colore
 * (le sole due cose che nessuno dei due possiede) siano quelli giusti **per l'osservatore**.
 *
 * ⚠️ **Il colore e' l'asserzione che conta**: la stessa unita' vista da due osservatori deve dare due
 * viste diverse. Un'implementazione che leggesse `TeamId` invece di `bIsAlly` passerebbe ogni test scritto
 * da un solo punto di vista, e a schermo colorerebbe i nemici come i propri per lo spettatore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitOverlayViewIsObserverRelativeTest,
	"RefactorTactics.HudViewModel.UnitOverlayViewIsObserverRelative",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitOverlayViewIsObserverRelativeTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTUnit* Unit = SpawnHudVmUnit(World, TEXT("Hero.Aevik"), /*TeamId*/ 0);
	if (!TestNotNull(TEXT("unita'"), Unit)) { DestroyHudVmWorld(World); return false; }

	Unit->Health = 60;
	Unit->ApplyStatus(TAG_Status_Burning, /*Turni*/ 2);

	// Visto da un compagno di squadra...
	const FRTUnitOverlayView Alleata = URTHudViewModel::BuildUnitOverlay(Unit, /*PlayerTeamId*/ 0, {}, {});
	// ...e dall'avversario.
	const FRTUnitOverlayView Nemica  = URTHudViewModel::BuildUnitOverlay(Unit, /*PlayerTeamId*/ 1, {}, {});

	// I due produttori sono arrivati entrambi.
	TestEqual(TEXT("la card porta la vita vera"), Alleata.Card.Health, 60);
	TestEqual(TEXT("gli stati sono quelli attivi"), Alleata.Statuses.Num(), 1);
	TestEqual(TEXT("e sono quelli giusti"),
		Alleata.Statuses[0].Tag, TAG_Status_Burning.GetTag().GetTagName());

	// Il nome e' quello canonico del catalogo, non l'ID: `Hero.Aevik` si legge `Aevik`.
	TestFalse(TEXT("il nome non e' vuoto"), Alleata.DisplayName.IsEmpty());
	TestFalse(TEXT("il nome non e' l'ID grezzo"), Alleata.DisplayName.Equals(TEXT("Hero.Aevik")));

	// 🔴 Il cuore: stessa unita', due osservatori, due viste.
	TestTrue(TEXT("per il compagno e' un'alleata"),  Alleata.Card.bIsAlly);
	TestFalse(TEXT("per l'avversario non lo e'"),    Nemica.Card.bIsAlly);
	TestNotEqual(TEXT("e il colore cambia con chi guarda"), Alleata.TeamColor, Nemica.TeamColor);

	// Anti-vacuita': cio' che NON dipende dall'osservatore resta identico.
	TestEqual(TEXT("la vita non dipende da chi guarda"), Alleata.Card.Health, Nemica.Card.Health);

	DestroyHudVmWorld(World);
	return true;
}

/**
 * Una vista neutra per un'unita' che non c'e': un widget costruito prima dell'unita' mostra un vuoto, non
 * un morto a 0 HP. E' la stessa scelta gia' presa da `BuildMatchHeader`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitOverlayViewIsNeutralWithoutUnitTest,
	"RefactorTactics.HudViewModel.UnitOverlayViewIsNeutralWithoutUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitOverlayViewIsNeutralWithoutUnitTest::RunTest(const FString&)
{
	const FRTUnitOverlayView Vuota = URTHudViewModel::BuildUnitOverlay(nullptr, /*PlayerTeamId*/ 0, {}, {});

	TestTrue(TEXT("nessun nome"), Vuota.DisplayName.IsEmpty());
	TestEqual(TEXT("nessuno stato"), Vuota.Statuses.Num(), 0);
	// ⚠️ `MaxHealth` a zero e' cio' che distingue «non c'e' unita'» da «unita' morta»: un morto ha
	// `MaxHealth > 0` e `Health == 0`. Chi disegna una barra deve poterli separare.
	TestEqual(TEXT("nessuna vita massima: non e' un morto, e' un'assenza"), Vuota.Card.MaxHealth, 0);

	return true;
}

/**
 * 🔴 **L'avviso di fuoco amico sopravvive al cambio di supporto, e il fuoco amico VINCE sul bersaglio.**
 *
 * ⚠️ **Questo test esiste perche' la prima stesura di `#2288` aveva PERSO l'avviso.** Rimuovendo il blocco
 * di disegno dal canvas e' andato via anche l'unico consumatore di `ComputePlannedHitMarks`: i due `TSet`
 * restavano calcolati e mai letti, e a schermo spariva un avviso che nasce da un'osservazione in PIE del
 * 2026-08-08 — *«non capisco se sto facendo un tiro e se nel tiro si interseca con un cilindro»*.
 *
 * Nessun test lo ha visto, perche' nessun test lo copriva: il canvas non ne aveva. Ora ce l'ha.
 *
 * 🔑 **Perche' il fuoco amico deve vincere**: e' l'unico caso in cui chi guarda potrebbe voler **cambiare
 * idea** prima del lock-in. Due avvisi sullo stesso nome si annullerebbero a vicenda.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitOverlayFriendlyFireWinsOverTargetedTest,
	"RefactorTactics.HudViewModel.UnitOverlayFriendlyFireWinsOverTargeted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitOverlayFriendlyFireWinsOverTargetedTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTUnit* Unit = SpawnHudVmUnit(World, TEXT("Hero.Aevik"), /*TeamId*/ 0);
	if (!TestNotNull(TEXT("unita'"), Unit)) { DestroyHudVmWorld(World); return false; }
	Unit->Cell = FRTCellId(3, 1, 0);

	const TSet<FRTCellId> Vuoto;
	const TSet<FRTCellId> SullaCella = { FRTCellId(3, 1, 0) };
	const TSet<FRTCellId> Altrove = { FRTCellId(9, 9, 0) };

	// Nessun piano la tocca: nessun avviso.
	{
		const FRTUnitOverlayView V = URTHudViewModel::BuildUnitOverlay(Unit, 0, Vuoto, Vuoto);
		TestFalse(TEXT("nessun piano: niente fuoco amico"), V.bFriendlyFire);
		TestFalse(TEXT("nessun piano: non e' bersaglio"), V.bTargeted);
	}

	// Dentro l'area di un attacco: e' un bersaglio.
	{
		const FRTUnitOverlayView V = URTHudViewModel::BuildUnitOverlay(Unit, 0, SullaCella, Vuoto);
		TestTrue(TEXT("nell'area: e' bersaglio"), V.bTargeted);
		TestFalse(TEXT("ma non e' fuoco amico"), V.bFriendlyFire);
	}

	// 🔴 Il cuore: dentro l'area E alleato -> **fuoco amico**, e il bersaglio si spegne.
	{
		const FRTUnitOverlayView V = URTHudViewModel::BuildUnitOverlay(Unit, 0, SullaCella, SullaCella);
		TestTrue(TEXT("alleato nell'area: fuoco amico"), V.bFriendlyFire);
		TestFalse(TEXT("e il bersaglio NON si accende insieme"), V.bTargeted);
	}

	// Anti-vacuita': un piano su un'altra cella non riguarda questa unita'.
	{
		const FRTUnitOverlayView V = URTHudViewModel::BuildUnitOverlay(Unit, 0, Altrove, Altrove);
		TestFalse(TEXT("piano altrove: niente fuoco amico"), V.bFriendlyFire);
		TestFalse(TEXT("piano altrove: non e' bersaglio"), V.bTargeted);
	}

	DestroyHudVmWorld(World);
	return true;
}

/**
 * 🔴 **L'etichetta della durata: un numero quando c'è un conteggio, NIENTE quando non c'è.**
 *
 * È l'unica regola di formato della riga di uno stato, ed è pura apposta: sta qui invece che dentro
 * `SetOverlayView`, dove verificarla richiederebbe di costruire un widget.
 *
 * ⚠️ **Il caso che conta è il legato-alla-cella.** `Wet` dall'acqua bassa dura *finché resti dov'è*, e
 * `FRTStatusBadgeView` gli mette `RemainingTurns = 0` proprio perché il `-1` di `PersistentWhileOnCell` non
 * attraversi il confine. Un formato che stampasse quel `0` scriverebbe «zero turni» su uno stato che non sta
 * per finire — l'opposto del vero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusDurationLabelTest,
	"RefactorTactics.HudViewModel.StatusDurationLabelIsEmptyWhenThereIsNoCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusDurationLabelTest::RunTest(const FString&)
{
	// Con un conteggio: il numero, e nient'altro — l'icona dice già quale stato è.
	TestEqual(TEXT("due turni residui"),
		URTUnitOverlayWidget::ComposeStatusDurationLabel(2, /*bCellBound*/ false), FString(TEXT("2")));
	TestEqual(TEXT("un turno residuo"),
		URTUnitOverlayWidget::ComposeStatusDurationLabel(1, /*bCellBound*/ false), FString(TEXT("1")));

	// 🔴 Legato alla cella: nessun conteggio, **anche se il numero fosse diverso da zero**. È il verso che
	// conta: `bCellBound` decide, non `RemainingTurns`.
	TestTrue(TEXT("legato alla cella: nessuna etichetta"),
		URTUnitOverlayWidget::ComposeStatusDurationLabel(0, /*bCellBound*/ true).IsEmpty());
	TestTrue(TEXT("legato alla cella: nessuna etichetta nemmeno con un numero"),
		URTUnitOverlayWidget::ComposeStatusDurationLabel(3, /*bCellBound*/ true).IsEmpty());

	// Una durata non positiva non è un tempo da mostrare: uno stato scaduto non arriva nemmeno alla vista.
	TestTrue(TEXT("zero turni: nessuna etichetta"),
		URTUnitOverlayWidget::ComposeStatusDurationLabel(0, /*bCellBound*/ false).IsEmpty());

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Il countdown del Ready arriva alla vista (`#2358`)
// ---------------------------------------------------------------------------------------------------------

/**
 * **IL COUNTDOWN ARMATO DIVENTA UN NUMERO NELLA VISTA, E DA SPENTO RESTA «NON SI APPLICA»**.
 *
 * ⚠️ **La prima meta' e' il controllo, e senza di lei la seconda non proverebbe niente**: `-1.f` e' il
 * DEFAULT del campo, quindi «non armato → negativo» e' vero anche in una vista che non avesse mai letto il
 * `TurnManager`. E' la stessa forma di `UntimedPlanningIsNotExpired` qui sopra.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmReadyCountdownTest,
	"RefactorTactics.HudViewModel.ReadyCountdownReachesTheView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmReadyCountdownTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHudVmWorld(World); return false; }

	// CONTROLLO: senza Ready dichiarato la domanda non si applica.
	const FRTMatchHeaderView Spento = URTHudViewModel::BuildMatchHeader(TM);
	TestTrue(TEXT("countdown non armato: il campo dice «non si applica»"),
		Spento.ReadyCountdownSecondsRemaining < 0.f);

	// LA MISURA: dopo il Ready il countdown e' un numero utilizzabile.
	TM->SetPlanningSeconds(30.f);
	TM->RequestLockIn();
	if (!TestTrue(TEXT("il countdown e' armato: senza, la misura non proverebbe niente"),
		TM->IsReadyCountdownActive()))
	{
		DestroyHudVmWorld(World);
		return false;
	}

	const FRTMatchHeaderView Armato = URTHudViewModel::BuildMatchHeader(TM);
	TestTrue(TEXT("countdown armato: il campo porta un numero"),
		Armato.ReadyCountdownSecondsRemaining >= 0.f);
	TestTrue(TEXT("e non supera la durata dichiarata"),
		Armato.ReadyCountdownSecondsRemaining <= TM->GetReadyCountdownSeconds());

	// E dopo l'Unready torna a tacere: il campo segue lo stato, non lo ricorda.
	TM->CancelLockIn();
	const FRTMatchHeaderView Annullato = URTHudViewModel::BuildMatchHeader(TM);
	TestTrue(TEXT("dopo l'Unready il campo torna a «non si applica»"),
		Annullato.ReadyCountdownSecondsRemaining < 0.f);

	DestroyHudVmWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// I due orologi e la regola che li combina (`#2193`, `#2358`, `#613`)
// ---------------------------------------------------------------------------------------------------------

/**
 * **IL NUMERO DA MOSTRARE E' IL PIU' VICINO DEI DUE, E LA REGOLA STA IN UN POSTO SOLO.**
 *
 * 🔴 **Perche' questo test esiste adesso.** La regola c'era gia' — dentro `ARTHUD::ComposeMatchStatusLine`,
 * e `HUD.MatchStatusShowsTheReadyCountdown` la copre **attraverso la riga di testo del Canvas**. Con lo
 * Screen HUD in UMG (`#613`) arriva un secondo consumatore che quella riga non la attraversa: un
 * `WBP_RT_TurnHeader` che stampasse `ReadyCountdownSecondsRemaining` sarebbe corretto secondo il tipo e
 * sbagliato secondo il gioco, e **nessun test lo direbbe**. Qui la regola e' misurata da sola, cosi' che
 * ogni consumatore possa consumarla invece di riscriverla.
 *
 * ⚠️ Il caso che discrimina e' il secondo: se qualcuno «semplificasse» la funzione restituendo il countdown,
 * tutti gli altri casi resterebbero verdi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmSecondsUntilCommitTest,
	"RefactorTactics.HudViewModel.SecondsUntilCommitIsTheNearerClock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmSecondsUntilCommitTest::RunTest(const FString&)
{
	FRTMatchHeaderView View;

	// CONTROLLO della premessa: sulla vista neutra non c'e' nessun conto alla rovescia da mostrare, e la
	// risposta e' un negativo — non uno zero, che si leggerebbe «commit adesso».
	TestTrue(TEXT("vista neutra: nessun orologio, risposta negativa"),
		URTHudViewModel::ComputeSecondsUntilCommit(View) < 0.f);

	// Solo il tetto del Planning: e' lui il momento del commit.
	View.PlanningSecondsRemaining = 25.f;
	View.ReadyCountdownSecondsRemaining = -1.f;
	TestEqual(TEXT("senza Ready: il commit arriva alla scadenza del Planning"),
		URTHudViewModel::ComputeSecondsUntilCommit(View), 25.f);

	// 🔴 IL CASO CHE DISCRIMINA: il tetto e' piu' corto del countdown e vince lui (`#2193`).
	View.PlanningSecondsRemaining = 1.5f;
	View.ReadyCountdownSecondsRemaining = 3.f;
	TestEqual(TEXT("tetto piu' corto del countdown: vince il tetto"),
		URTHudViewModel::ComputeSecondsUntilCommit(View), 1.5f);

	// Il verso opposto, che da solo non proverebbe niente ma serve a escludere un `Min` invertito.
	View.PlanningSecondsRemaining = 25.f;
	View.ReadyCountdownSecondsRemaining = 3.f;
	TestEqual(TEXT("countdown piu' corto del tetto: vince il countdown"),
		URTHudViewModel::ComputeSecondsUntilCommit(View), 3.f);

	// Run headless: il Planning non ha orologio, e un `Min` cieco avrebbe restituito il negativo spegnendo
	// l'unico conto in corsa.
	View.PlanningSecondsRemaining = -1.f;
	View.ReadyCountdownSecondsRemaining = 3.f;
	TestEqual(TEXT("senza tetto: il countdown resta l'unico orologio"),
		URTHudViewModel::ComputeSecondsUntilCommit(View), 3.f);

	// Zero non e' «non si applica»: e' un numero da mostrare, ed e' l'ultimo istante prima del commit.
	View.PlanningSecondsRemaining = 25.f;
	View.ReadyCountdownSecondsRemaining = 0.f;
	TestEqual(TEXT("countdown a zero: commit adesso, non «non si applica»"),
		URTHudViewModel::ComputeSecondsUntilCommit(View), 0.f);

	return true;
}

/**
 * **E il campo pubblicato dice la stessa cosa della funzione.**
 *
 * ⚠️ `FRTMatchHeaderView::SecondsUntilCommit` e' una **comodita' derivata**: chi costruisce la vista a mano
 * lo trova al default. Questo test copre l'unico produttore che deve popolarlo — `BuildMatchHeader` — e lo
 * fa confrontandolo con la funzione invece che con un numero scritto qui: un valore atteso costante
 * pinnerebbe l'orologio, non la coerenza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmSecondsUntilCommitIsPublishedTest,
	"RefactorTactics.HudViewModel.SecondsUntilCommitIsPublishedByTheProducer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmSecondsUntilCommitIsPublishedTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHudVmWorld(World); return false; }

	TM->SetPlanningSeconds(30.f);
	TM->RequestLockIn();
	if (!TestTrue(TEXT("il countdown e' armato: senza, la misura non proverebbe niente"),
		TM->IsReadyCountdownActive()))
	{
		DestroyHudVmWorld(World);
		return false;
	}

	const FRTMatchHeaderView Vista = URTHudViewModel::BuildMatchHeader(TM);
	TestEqual(TEXT("il campo pubblicato coincide con la funzione"),
		Vista.SecondsUntilCommit, URTHudViewModel::ComputeSecondsUntilCommit(Vista));
	TestTrue(TEXT("e con un countdown armato porta un numero, non «non si applica»"),
		Vista.SecondsUntilCommit >= 0.f);

	DestroyHudVmWorld(World);
	return true;
}


// ---------------------------------------------------------------------------------------------------------
// Il token di danno: la cifra viene dall'evento, e il caso zero e' una decisione (`#2455`)
// ---------------------------------------------------------------------------------------------------------

/**
 * 🔴 **Il numero mostrato e' quello dell'evento, e il caso `Amount <= 0` ha un comportamento SCELTO.**
 *
 * È l'unica regola di giudizio che il token possiede, ed è pura apposta: `URTUnitOverlayWidget` è un
 * `UUserWidget` con copertura headless **zero** ([D-320] punto 5), quindi tutto ciò che decide deve vivere
 * qui, dove un test lo chiama senza costruire un widget.
 *
 * ⚠️ **Il caso zero non è di frontiera.** `URTHexCombatLibrary::CollectHexAttacks` dichiara *«Il danno si
 * ferma a 0: il colpo resta avvenuto»* e aggiunge comunque il colpo a `Plan.Hits`: con
 * `LowCoverDamageReduction = 10`, un intento di potenza ≤ 10 dietro copertura bassa arriva qui a zero. Un
 * ramo `else` non deciso lo mostrerebbe come «niente» — cioè un attacco che non è successo, quando invece
 * è successo e non ha tolto nulla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageTokenCarriesEventAmountTest,
	"RefactorTactics.HudViewModel.DamageTokenCarriesTheEventAmountAndDecidesTheZeroCase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageTokenCarriesEventAmountTest::RunTest(const FString&)
{
	// (a) Il caso ordinario: la cifra dell'evento, col segno. Il segno è un canale, non decorazione — `#2453`
	// vieta il colore come UNICO canale, e una cifra nuda si leggerebbe come una cura.
	{
		const FRTDamageTokenView T = URTHudViewModel::BuildDamageToken(17, /*Target*/ 3);
		TestTrue(TEXT("(a) c'e' danno"), T.bHasDamage);
		TestEqual(TEXT("(a) l'importo e' quello dell'evento"), T.Amount, 17);
		TestEqual(TEXT("(a) l'etichetta porta cifra e segno"), T.Label, FString(TEXT("-17")));
		TestEqual(TEXT("(a) il soggetto e' chi subisce"), T.TargetStableUnitId, 3);
	}

	// (b) 🔴 Il caso zero: NON la cifra nuda, e NON il vuoto.
	{
		const FRTDamageTokenView T = URTHudViewModel::BuildDamageToken(0, /*Target*/ 3);
		TestFalse(TEXT("(b) nessun danno da mostrare come cifra"), T.bHasDamage);
		TestFalse(TEXT("(b) ma un'etichetta c'e': il colpo e' avvenuto"), T.Label.IsEmpty());
		TestNotEqual(TEXT("(b) e non e' la cifra nuda 0"), T.Label, FString(TEXT("0")));
		TestNotEqual(TEXT("(b) ne' un meno seguito da zero"), T.Label, FString(TEXT("-0")));
	}

	// (c) Difesa: un valore negativo non è atteso — `Hit.Power` passa da `FMath::Max(0, ...)` — ma se
	// arrivasse, `-(-3)` stamperebbe `+3` sopra la testa di chi ha appena incassato. Cade nel ramo dello
	// zero, che dice «nessuna quantità da mostrare» senza inventarne una.
	{
		const FRTDamageTokenView T = URTHudViewModel::BuildDamageToken(-3, /*Target*/ 3);
		TestFalse(TEXT("(c) un negativo non e' danno da mostrare"), T.bHasDamage);
		TestEqual(TEXT("(c) e cade nella stessa etichetta dello zero"),
			T.Label, URTHudViewModel::BuildDamageToken(0, 3).Label);
		TestFalse(TEXT("(c) e non stampa un segno piu'"), T.Label.Contains(TEXT("+")));
	}

	// (d) `0` non è l'unità numero zero: è «nessuno» ([D-063]). Si conserva com'è, così chi disegna
	// confronta con il proprio `StableUnitId` e non trova nessuna corrispondenza.
	{
		const FRTDamageTokenView T = URTHudViewModel::BuildDamageToken(17, /*Target*/ 0);
		TestEqual(TEXT("(d) lo zero del soggetto attraversa intatto"), T.TargetStableUnitId, 0);
		TestTrue(TEXT("(d) e non impedisce di comporre la cifra"), T.bHasDamage);
	}

	// (e) ⛔ Anti-vacuità: l'etichetta **dipende** dall'importo. Senza questa riga tutte le sopra
	// starebbero in piedi anche con una costante al posto della composizione.
	TestNotEqual(TEXT("(e) importi diversi, etichette diverse"),
		URTHudViewModel::BuildDamageToken(17, 3).Label,
		URTHudViewModel::BuildDamageToken(18, 3).Label);

	return true;
}

/**
 * 🔴 **A scadenza l'animazione vale esattamente `0`, e lo zero esatto è il contenuto del test.**
 *
 * L'AC di `#2455` chiede che la barra *«torni a riposo senza restare in uno stato transitorio»*. Con un
 * ritorno approssimato la scala si fermerebbe a `1.0001` e la sovrapposizione resterebbe per sempre appena
 * più grande, in un punto dove nessuno guarderebbe più — un difetto che non fa rumore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverlayFadeAlphaTest,
	"RefactorTactics.HudViewModel.FadeAlphaReachesRestExactly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverlayFadeAlphaTest::RunTest(const FString&)
{
	// All'inizio: pieno.
	TestEqual(TEXT("a tempo zero l'animazione e' piena"),
		URTUnitOverlayWidget::FadeAlpha(0.f, 1.f), 1.f);

	// A metà: metà. Non è una proprietà del riposo, ma senza di essa «0 all'inizio e 0 alla fine» sarebbe
	// soddisfatto anche da una funzione costante a zero.
	TestEqual(TEXT("a meta' durata vale meta'"),
		URTUnitOverlayWidget::FadeAlpha(0.5f, 1.f), 0.5f);

	// 🔴 A scadenza: **esattamente** zero, non «circa».
	TestEqual(TEXT("a scadenza e' esattamente zero"),
		URTUnitOverlayWidget::FadeAlpha(1.f, 1.f), 0.f);

	// Oltre la scadenza: resta zero, non diventa negativa — una scala negativa specchierebbe il widget.
	TestEqual(TEXT("oltre la scadenza resta zero"),
		URTUnitOverlayWidget::FadeAlpha(5.f, 1.f), 0.f);

	// Durata non positiva: «già finita», e nessuna divisione per zero. `DamageTokenSeconds = 0` è il modo
	// dichiarato per spegnere l'effetto senza ricompilare.
	TestEqual(TEXT("durata zero: gia' finita"),
		URTUnitOverlayWidget::FadeAlpha(0.f, 0.f), 0.f);
	TestEqual(TEXT("durata negativa: gia' finita, non un errore"),
		URTUnitOverlayWidget::FadeAlpha(1.f, -1.f), 0.f);

	// Un tempo negativo non risale sopra il pieno: il clamp sta sul tempo, e vale in entrambi i versi.
	TestEqual(TEXT("un tempo negativo non supera il pieno"),
		URTUnitOverlayWidget::FadeAlpha(-2.f, 1.f), 1.f);

	return true;
}

/**
 * IL FEED MOSTRA SOLO CIO' CHE L'OSSERVATORE PUO' VEDERE — `#2697`, ed e' l'assertion sul CONSUMATORE.
 *
 * 🔴 **Il test che mancava aveva un altro soggetto.** `RTCombatLogTests` esercita `GetRecentEventsForTeam`
 * e asserisce sull'uscita: prova che il **filtro** funziona, ed e' verde da prima che questa issue nascesse.
 * Cio' che nessuno provava e' che il filtro fosse **quello usato da chi disegna** — un canale corretto senza
 * consumatori non produce nessun rosso, ed e' la forma di `#2549` e `#2492`.
 *
 * ⛔ **Una regressione a un canale non filtrato e' un LEAK, non un dettaglio di UI.** La riga completa
 * mostrerebbe fatti che l'osservatore non ha diritto di conoscere: questo test e' cio' che la rende rossa.
 *
 * ⚠️ **Le due meta' sono entrambe necessarie.** Senza la premessa positiva, «zero righe per il non
 * autorizzato» sarebbe soddisfatto anche da una funzione che non restituisce mai niente — cioe' proprio dal
 * difetto che stiamo chiudendo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudEventFeedRespectsTheObserverTest,
	"RefactorTactics.ScreenHud.EventFeedShowsOnlyWhatTheObserverMaySee",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudEventFeedRespectsTheObserverTest::RunTest(const FString&)
{
	// Il tiro rifiutato di un'unita' della squadra 1, con un muro che la SUA squadra conosce.
	TArray<FRTTurnLogEntry> Log;
	FRTTurnLogEntry Rifiutato;
	Rifiutato.Category = ERTLogCategory::Combat;
	Rifiutato.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
	Rifiutato.UnitId = 9;
	Rifiutato.SrcCell = FRTCellId(-1, 0, 0);
	Rifiutato.TgtCell = FRTCellId(1, 0, 0);
	Rifiutato.SightBlockerCell = FRTCellId(0, 0, 0);
	Rifiutato.Verdict.AllowTeam(1);
	Log.Add(Rifiutato);

	// PREMESSA: chi e' autorizzato riceve la riga, e il riferimento nel mondo per trovarla.
	const TArray<FRTPlayerEventLineView> Autorizzato =
		URTHudViewModel::BuildPlayerEventFeed(Log, /*ObserverTeamId*/ 1);
	if (!TestEqual(TEXT("premessa: chi e' autorizzato legge la riga"), Autorizzato.Num(), 1))
	{
		return false;
	}
	TestTrue(TEXT("con un ostacolo da mostrare nel mondo"), Autorizzato[0].bHasBlocker);
	TestTrue(TEXT("e la cella e' quella della voce"), Autorizzato[0].BlockerCell == FRTCellId(0, 0, 0));
	TestFalse(TEXT("e la riga non e' vuota"), Autorizzato[0].Text.IsEmpty());

	// ⛔ **La riga non stampa coordinate**, ed e' il confine di `#1936` §A: `(q=..,r=..,L=..)` e' diagnostica
	// e resta a `#79`. Il **dove** viaggia come cella, non come testo — cosi' chi disegna puo' marcarlo nel
	// mondo, che e' il «riferimento video» che il verdetto d'autore chiedeva.
	TestFalse(TEXT("e non stampa coordinate assiali"), Autorizzato[0].Text.ToString().Contains(TEXT("q=")));

	// E chi non lo e' non riceve NIENTE: non una riga anonima, non un conteggio, non una cella.
	const TArray<FRTPlayerEventLineView> NonAutorizzato =
		URTHudViewModel::BuildPlayerEventFeed(Log, /*ObserverTeamId*/ 0);
	TestEqual(TEXT("chi non e' autorizzato non riceve nessuna riga"), NonAutorizzato.Num(), 0);
	return true;
}

namespace
{
	/** Una voce autorizzata alla squadra 1, distinguibile per `UnitId`, nel turno chiesto. */
	FRTTurnLogEntry VoceDiTurno(int32 TurnNumber, int32 UnitId)
	{
		FRTTurnLogEntry Voce;
		Voce.Category = ERTLogCategory::Combat;
		Voce.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
		Voce.UnitId = UnitId;
		Voce.TurnNumber = TurnNumber;
		Voce.SrcCell = FRTCellId(-1, 0, 0);
		Voce.TgtCell = FRTCellId(1, 0, 0);
		Voce.Verdict.AllowTeam(1);
		return Voce;
	}
}

/**
 * 🔴 **IL FEED PARLA DEL TURNO, NON DELLA PARTITA — e non e' una questione di ingombro.**
 *
 * 🔑 **Il difetto che questo test chiude e' che il feed NASCONDE.** `URTPlayerEventProjector::Project`
 * applica la **dominanza**: una riga per unita', dove «il KO prende il posto del danno, il danno quello
 * del colpo». Il suo commento la descrive *«in questo turno»* — ma il chiamante gli passava
 * `TurnManager->GetTurnLog()`, cioe' la **partita intera**. Con quel perimetro l'unita' andata KO al
 * round 3 tiene la propria riga fino alla fine, perche' nessun evento successivo ha rango piu' alto: cio'
 * che le e' accaduto dopo non compare, e il giocatore legge una cronaca ferma a tre round prima.
 *
 * ⚠️ **Stessa sorte per l'ambiente**: §E vuole una riga sola con un contatore («quante celle, non
 * quali»), e su dodici round quel contatore sommava l'intera partita in una voce che non dice piu' nulla.
 *
 * ⛔ **Il taglio sta nella VISTA e non nel `TurnLog`**, che e' la fonte del replay (`#469`): troncare li'
 * cambierebbe cio' che si puo' rigiocare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudFeedIsScopedToTheCurrentTurnTest,
	"RefactorTactics.ScreenHud.FeedIsScopedToTheCurrentTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHudFeedIsScopedToTheCurrentTurnTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Log;
	Log.Add(VoceDiTurno(/*TurnNumber*/ 1, /*UnitId*/ 11)); // turno vecchio
	Log.Add(VoceDiTurno(/*TurnNumber*/ 1, /*UnitId*/ 12)); // turno vecchio
	Log.Add(VoceDiTurno(/*TurnNumber*/ 2, /*UnitId*/ 21)); // turno corrente

	const TArray<FRTPlayerEventLineView> Feed =
		URTHudViewModel::BuildPlayerEventFeed(Log, /*ObserverTeamId*/ 1);

	if (!TestEqual(TEXT("il feed porta solo le voci del turno piu' recente"), Feed.Num(), 1))
	{
		return false;
	}
	TestEqual(TEXT("ed e' quella dell'unita' che ha agito in quel turno"),
		Feed[0].PrimaryStableUnitId, 21);

	// ⚠️ Il controllo che il test sarebbe inutile senza: le voci vecchie erano **autorizzate** e
	// **componibili**, quindi la loro assenza dice «filtrate per turno» e non «scartate per privacy».
	TArray<FRTTurnLogEntry> SoloVecchie;
	SoloVecchie.Add(VoceDiTurno(1, 11));
	SoloVecchie.Add(VoceDiTurno(1, 12));
	TestEqual(TEXT("controllo: da sole quelle voci PRODUCONO righe"),
		URTHudViewModel::BuildPlayerEventFeed(SoloVecchie, 1).Num(), 2);

	return true;
}

/**
 * 🔴 **IL TETTO E' UN NUMERO, E SI PRENDONO LE ULTIME.**
 *
 * ⚠️ **«Le prime N» sarebbe altrettanto implementabile e completamente inutile**, ed e' esattamente cio'
 * che questo test distingue: un tetto che tagliasse la coda lascerebbe a schermo l'inizio del turno e
 * nasconderebbe l'esito. Verificare solo il **numero** di righe non vedrebbe la differenza.
 *
 * 🔑 Il tetto e' la **rete**, non il rimedio: il rimedio e' il perimetro
 * (`FeedIsScopedToTheCurrentTurn`). Serve per il turno anomalo — molte voci di **mondo**, che sfuggono
 * alla dominanza perche' non appartengono a nessuna unita' e si accodano una per una.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudFeedShowsTheLastLinesWithinBudgetTest,
	"RefactorTactics.ScreenHud.FeedShowsTheLastLinesWithinBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHudFeedShowsTheLastLinesWithinBudgetTest::RunTest(const FString&)
{
	// Un turno con molte piu' voci del budget, ciascuna di un'unita' diversa: senza unita' distinte la
	// dominanza le fonderebbe in una riga sola e il tetto non verrebbe mai raggiunto.
	const int32 Quante = URTHudViewModel::MaxFeedLines * 3;
	TArray<FRTTurnLogEntry> Log;
	for (int32 i = 0; i < Quante; ++i)
	{
		Log.Add(VoceDiTurno(/*TurnNumber*/ 7, /*UnitId*/ 100 + i));
	}

	const TArray<FRTPlayerEventLineView> Feed =
		URTHudViewModel::BuildPlayerEventFeed(Log, /*ObserverTeamId*/ 1);

	if (!TestEqual(TEXT("il feed si ferma al budget"), Feed.Num(), URTHudViewModel::MaxFeedLines))
	{
		return false;
	}

	// 🔑 La meta' che conta: **quali** dodici. La prima riga mostrata e' la tredicesima dal fondo, non la
	// prima del turno.
	const int32 PrimaAttesa = 100 + (Quante - URTHudViewModel::MaxFeedLines);
	TestEqual(TEXT("e sono le ULTIME: la prima riga e' quella giusta"),
		Feed[0].PrimaryStableUnitId, PrimaAttesa);
	TestEqual(TEXT("e l'ultima riga e' l'evento piu' recente del turno"),
		Feed[Feed.Num() - 1].PrimaryStableUnitId, 100 + Quante - 1);

	// Sotto il budget non si taglia niente: un tetto che accorciasse sempre sarebbe un altro difetto.
	TArray<FRTTurnLogEntry> Poche;
	Poche.Add(VoceDiTurno(7, 1));
	Poche.Add(VoceDiTurno(7, 2));
	TestEqual(TEXT("sotto il budget il feed non taglia"),
		URTHudViewModel::BuildPlayerEventFeed(Poche, 1).Num(), 2);

	return true;
}

/**
 * `HealthFraction`: la barra della card non deve dividere, e il caso che rompeva e' la card VUOTA.
 *
 * 🔴 **Il difetto e' stato MISURATO in PIE, non ipotizzato.** Seduta `U49` del 2026-09-10
 * (`PIE-V01-SCREENHUD`, `#613`): `Script Msg: Divide by zero: Divide_DoubleDouble` da
 * `WBP_RT_UnitCard_C`, dentro `WBP_RT_SelectedUnitPanelBottom`.
 *
 * ⛔ **Questo test NON riproduce il warning**, che nasce in un nodo dentro il `.uasset` e headless non
 * viene nemmeno costruito — stessa dichiarazione, e per la stessa ragione, di
 * `ChargeFractionNeedsNoDivisionInTheWidget`. Prova la cosa che rende il warning impossibile: che la
 * vista porti gia' il risultato, con la guardia sullo zero in un posto solo e misurabile.
 *
 * ⚠️ **Finche' il grafo continua a dividere da se', la warning resta.** Ricablare la progress bar su
 * questo campo e' lavoro di Editor, e non lo copre nessun test di questo file.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmHealthFractionTest,
	"RefactorTactics.HudViewModel.HealthFractionNeedsNoDivisionInTheWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmHealthFractionTest::RunTest(const FString&)
{
	// 🔴 IL CASO DEL DIFETTO, e viene per primo: nessuna unita' selezionata.
	const FRTUnitCardView Vuota = URTHudViewModel::BuildUnitCard(nullptr, 0);

	// PREMESSA DEL DIFETTO: senza questa riga il test proverebbe la guardia su un caso che non si
	// presenta mai, e resterebbe verde anche togliendo la guardia.
	if (!TestEqual(TEXT("premessa: la card vuota ha MaxHealth = 0 (il caso che divideva per zero)"),
			Vuota.MaxHealth, 0))
	{
		return false;
	}

	TestEqual(TEXT("card vuota: la barra e' a zero"), Vuota.HealthFraction, 0.f);

	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTUnit* Unit = SpawnHudVmUnit(World, TEXT("Hero.Aevik"), 0);
	if (!TestNotNull(TEXT("unita'"), Unit)) { DestroyHudVmWorld(World); return false; }

	const FRTUnitCardView Illesa = URTHudViewModel::BuildUnitCard(Unit, 0);
	if (!TestTrue(TEXT("premessa: un'unita' viva ha un massimo positivo"), Illesa.MaxHealth > 0))
	{
		DestroyHudVmWorld(World);
		return false;
	}

	TestEqual(TEXT("illesa: la barra e' piena"), Illesa.HealthFraction, 1.f);

	// La frazione SEGUE il simulatore: si toglie salute e la barra deve scendere. Senza questo controllo
	// la guardia potrebbe restituire una costante e il test resterebbe verde.
	Unit->Health = Unit->MaxHealth / 2;
	const FRTUnitCardView Ferita = URTHudViewModel::BuildUnitCard(Unit, 0);
	TestTrue(TEXT("ferita: la barra scende sotto la piena"),
		Ferita.HealthFraction < Illesa.HealthFraction);
	TestTrue(TEXT("ferita: la frazione resta in [0,1]"),
		Ferita.HealthFraction >= 0.f && Ferita.HealthFraction <= 1.f);

	// Il `Clamp` non e' decorativo: `Health` viene dal simulatore e puo' uscire dall'intervallo in
	// entrambe le direzioni. Una barra fuori da `[0,1]` disegna fuori dal proprio riquadro.
	Unit->Health = -5;
	TestEqual(TEXT("salute negativa: la barra si ferma a zero, non va sotto"),
		URTHudViewModel::BuildUnitCard(Unit, 0).HealthFraction, 0.f);

	Unit->Health = Unit->MaxHealth * 2;
	TestEqual(TEXT("salute oltre il massimo: la barra si ferma a uno"),
		URTHudViewModel::BuildUnitCard(Unit, 0).HealthFraction, 1.f);

	DestroyHudVmWorld(World);
	return true;
}

/**
 * La diagnostica del feed distingue le TRE cause che a schermo hanno lo stesso aspetto.
 *
 * 🔑 **Una diagnostica non verificata e' peggio del silenzio**: se dicesse «nessun turno risolto» mentre il
 * log ha voci, manderebbe chi indaga dalla parte sbagliata con l'autorita' di una misura. Questo test
 * asserisce che ogni ramo nomini il proprio caso, e che NON nomini gli altri.
 *
 * ⛔ Il difetto che esiste per fermare e' quello vero di `#2964`: un feed vuoto letto come «feed rotto»,
 * quando le cause possibili erano tre e due di esse non sono difetti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudFeedStateNamesItsOwnCauseTest,
	"RefactorTactics.ScreenHud.FeedStateNamesItsOwnCause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHudFeedStateNamesItsOwnCauseTest::RunTest(const FString&)
{
	auto Unisci = [](const TArray<FString>& R)
	{
		FString S;
		for (const FString& X : R) { S += X + TEXT(" | "); }
		return S;
	};

	// ── (1) Nessun manager: il caso che NON e' privacy, e la riga deve dirlo.
	{
		const FString S = Unisci(URTHudViewModel::DescribeFeedState(nullptr, TArray<int32>{ 1 }));
		TestTrue(TEXT("senza manager lo dichiara"), S.Contains(TEXT("nessun TurnManager")));
		TestTrue(TEXT("e avverte che NON e' privacy"), S.Contains(TEXT("NON e' privacy")));
	}

	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHudVmWorld(World); return false; }

	// ── (2) Manager presente, log VUOTO: stato normale prima della prima risoluzione, e va detto che non
	//        e' un difetto — altrimenti si cerca un guasto che non c'e'.
	{
		const FString S = Unisci(URTHudViewModel::DescribeFeedState(TM, TArray<int32>{ 1 }));
		TestTrue(TEXT("log vuoto: lo nomina"), S.Contains(TEXT("TurnLog e' vuoto")));
		TestTrue(TEXT("e dichiara che non e' un difetto"), S.Contains(TEXT("Non e' un difetto")));
		TestFalse(TEXT("e NON accusa il filtro"), S.Contains(TEXT("filtro dell'osservatore")));
	}

	// Una voce autorizzata alla sola squadra 1.
	FRTTurnLogEntry Voce = VoceDiTurno(/*TurnNumber*/ 3, /*UnitId*/ 77);
	// `VoceDiTurno` autorizza gia' la squadra 1 nella sua forma: il caso (3) dipende da quel default.
	TM->AppendTurnLogEntryForTest(Voce);

	// ── (3) Log pieno, osservatore NON autorizzato: il filtro funziona, e la riga punta a chi decide
	//        l'insieme invece che al feed.
	{
		const FString S = Unisci(URTHudViewModel::DescribeFeedState(TM, TArray<int32>{ 2 }));
		TestTrue(TEXT("nomina il filtro"), S.Contains(TEXT("filtro dell'osservatore")));
		TestTrue(TEXT("e indica ResolveObserverTeamIds"), S.Contains(TEXT("ResolveObserverTeamIds")));
		TestFalse(TEXT("e NON dice che il log e' vuoto"), S.Contains(TEXT("TurnLog e' vuoto")));
	}

	// ── (4) ⚠️ Il CONTROLLO che rende il test non vacuo: con l'osservatore GIUSTO la stessa voce passa, e la
	//        riga sposta il sospetto sul widget. Senza questo caso, i tre rami sopra passerebbero anche se
	//        `DescribeFeedState` dicesse sempre «filtrato».
	{
		const FString S = Unisci(URTHudViewModel::DescribeFeedState(TM, TArray<int32>{ 1 }));
		TestTrue(TEXT("con l'osservatore giusto il ViewModel produce righe"),
			S.Contains(TEXT("il difetto e' nel widget")));
		TestFalse(TEXT("e non accusa piu' il filtro"), S.Contains(TEXT("NESSUNA passa")));
	}

	DestroyHudVmWorld(World);
	return true;
}

/**
 * 🔴 **IL TASTO SI LEGGE DALLA TABELLA DEI BINDING, E NON SI CALCOLA DALL'INDICE** (`#2987`).
 *
 * 🔑 **L'oracolo e' l'UGUAGLIANZA con `ARTPlayerController::AbilityHotkeys()`**, e la scelta e' il punto
 * del test. Confrontare con una stringa attesa — `"1"`, `"0"` — rifarebbe qui la traduzione da tasto a
 * etichetta, cioe' creerebbe la seconda verita' che questa issue esiste per togliere: due composizioni che
 * divergono al primo cambio di tabella, e un test verde su entrambe.
 *
 * ⚠️ **La posizione `9` e' il caso che il difetto rendeva falso**, e va nominata: `AbilityHotkeys()` chiude
 * con `EKeys::Zero`, quindi la riga diceva `10.` per un tasto che e' `0`. Il controllo B e' scritto come
 * disuguaglianza da `"10"` perche' e' **quel** difetto a dover cadere, non un formato qualunque.
 *
 * ⛔ **E il vuoto oltre la tabella e' la meta' che nessun `Index + 1` puo' dare**: quell'aritmetica
 * risponde a ogni indice, anche a quelli che nessun tasto raggiunge — il kit a undici voci che
 * `GenericHotkeys()` dichiara possibile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmShortcutFromBindingTableTest,
	"RefactorTactics.HudViewModel.ShortcutComesFromTheBindingTableNotTheIndex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmShortcutFromBindingTableTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTUnit* Unit = SpawnHudVmUnit(World, TEXT("Hero.Aevik"), 0);
	if (!TestNotNull(TEXT("unita'"), Unit)) { DestroyHudVmWorld(World); return false; }

	const TArray<FRTAbilityCooldownView> Cds = URTHudViewModel::BuildAbilityCooldowns(Unit);

	// Anti-vacuita': senza un kit, ogni asserzione del ciclo sarebbe verde per il motivo sbagliato.
	if (!TestTrue(TEXT("premessa: l'unita' ha un kit"), Cds.Num() > 0))
	{
		DestroyHudVmWorld(World);
		return false;
	}

	// --- A. ogni posizione porta il tasto che le TABELLE le assegnano ----------------------------------
	// 🔑 **L'ordine di risoluzione e' quello di [D-397] §4**, ed e' il punto: `GenericHotkeys()` per
	// `ActionId` -> `AbilityHotkeys()` per posizione -> vuoto. Una generica mostra la propria LETTERA perche'
	// quella e' legata all'azione, mentre il numero dipende da quante voci porta l'eroe.
	//
	// ⌫ **Questo ciclo attendeva il numero per OGNI posizione, e la decisione l'ha superato**: misurato,
	// la posizione 5 portava `Z` dove il test chiedeva `6`. Non era un difetto del codice — era il test a
	// pinnare una regola che nel frattempo era stata decisa diversamente.
	const TArray<FKey>& Tabella = ARTPlayerController::AbilityHotkeys();
	const TArray<TPair<FName, FKey>>& Generiche = ARTPlayerController::GenericHotkeys();

	int32 GenericheViste = 0;

	for (int32 i = 0; i < Cds.Num(); ++i)
	{
		const TPair<FName, FKey>* Generica = Generiche.FindByPredicate(
			[&](const TPair<FName, FKey>& G) { return G.Key == Cds[i].ActionId; });
		if (Generica) { ++GenericheViste; }

		const FString Atteso = Generica
			? Generica->Value.GetDisplayName(/*bLongDisplayName=*/ false).ToString()
			: (Tabella.IsValidIndex(i)
				? Tabella[i].GetDisplayName(/*bLongDisplayName=*/ false).ToString()
				: FString());

		TestEqual(
			FString::Printf(TEXT("A: la posizione %d (%s) porta il tasto che le tabelle le assegnano"),
				i, *Cds[i].ActionId.ToString()),
			Cds[i].HotkeyLabel.ToString(), Atteso);
	}

	// --- B. la posizione 9 NON dice «10», che e' il difetto misurato -----------------------------------
	if (Cds.IsValidIndex(9))
	{
		TestNotEqual(TEXT("B: la decima posizione non annuncia un tasto `10` che non esiste"),
			Cds[9].HotkeyLabel.ToString(), FString(TEXT("10")));
	}
	else
	{
		// Il kit di questo eroe non arriva a dieci voci: il caso non e' osservabile QUI, e dirlo vale piu'
		// di un verde che sembra averlo coperto. Il controllo C lo esercita comunque, sulla funzione.
		AddInfo(TEXT("B: kit piu' corto della fila dei numeri — caso coperto dal solo controllo C"));
	}

	// ⛔ **Anti-vacuita' della meta' che [D-397] §4 ha deciso**: senza generiche nel kit il ciclo non
	// asserirebbe mai il ramo della lettera, e il test resterebbe verde misurando il solo numero. Le
	// generiche sono accodate al kit di OGNI unita' (`MakeGenericActions`), quindi zero qui e' un difetto.
	TestTrue(
		FString::Printf(TEXT("il kit porta delle generiche, che sono il ramo `lettera` (ne ha %d)"),
			GenericheViste),
		GenericheViste > 0);

	// --- C. oltre la tabella non c'e' un tasto, e la risposta e' VUOTA ---------------------------------
	// ⚠️ Si interroga la funzione e non la vista: serve un indice che la tabella non copre, e costruire
	// un'unita' con undici voci di kit misurerebbe la composizione del kit invece di questa regola.
	TestTrue(TEXT("C: una posizione oltre la fila dei numeri non porta nessun tasto"),
		ARTPlayerController::HotkeyLabelFor(NAME_None, Tabella.Num()).IsEmpty());
	TestTrue(TEXT("C: e un indice negativo nemmeno"),
		ARTPlayerController::HotkeyLabelFor(NAME_None, INDEX_NONE).IsEmpty());

	DestroyHudVmWorld(World);
	return true;
}

/**
 * 🔴 **UNA POSIZIONE DI KIT VUOTA PRODUCE UNA RIGA, E NON SPOSTA QUELLE DOPO** (`#2987`).
 *
 * 🔑 **E' il test che `CooldownsMirrorTheSimulator` non poteva essere.** Quello asserisce
 * `Num() == NumAbilities()` e `[i].AbilityIndex == i` su `Hero.Aevik`, cioe' su un kit **senza buchi**: il
 * ramo che salta non veniva mai eseguito, e l'asserzione pinnava il contratto esattamente nel caso in cui
 * era gia' vero. Qui il buco si crea, ed e' l'unico modo di far parlare quel ramo.
 *
 * ⚠️ **Il caso non e' teorico**: tre punti del progetto ammettono che una posizione non produca un'azione —
 * il `continue` di `MakeGenericActions` su un `ActionId` che il catalogo non conosce, il
 * `if (!Ability) return;` di `SelectAbilityForCurrent`, e il bounds-check di `GetAbility`. Cio' che
 * mancava era una risposta **sola**.
 *
 * 🔑 **L'oracolo forte e' il controllo C**, non il conteggio: che l'azione dopo il buco resti dove il tasto
 * la cerca. Con il `continue` di prima, `Num()` scendeva **e** ogni azione successiva slittava di una
 * posizione — e' lo slittamento a rendere il sesto riquadro e il tasto `6` due cose diverse.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmKitHoleTest,
	"RefactorTactics.HudViewModel.KitHoleDoesNotRenumberTheSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmKitHoleTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTUnit* Unit = SpawnHudVmUnit(World, TEXT("Hero.Aevik"), 0);
	if (!TestNotNull(TEXT("unita'"), Unit)) { DestroyHudVmWorld(World); return false; }

	// Anti-vacuita': serve un buco FRA due voci popolate, quindi almeno tre posizioni.
	if (!TestTrue(TEXT("premessa: il kit ha almeno tre posizioni"), Unit->NumAbilities() >= 3))
	{
		DestroyHudVmWorld(World);
		return false;
	}

	// L'identita' dell'azione che sta DOPO il buco, letta prima di scavarlo: e' il riferimento del
	// controllo C. Presa dal kit e non dalla vista, cosi' non dipende da cio' che il test sta misurando.
	const URTActionData* DopoIlBuco = Unit->GetAbility(2);
	if (!TestNotNull(TEXT("premessa: la posizione 2 porta un'azione"), DopoIlBuco))
	{
		DestroyHudVmWorld(World);
		return false;
	}
	const FName IdDopoIlBuco = DopoIlBuco->Def.ActionId;

	// Il buco: una posizione di kit che non produce un'azione, fra due che la producono.
	const int32 NumPrima = Unit->NumAbilities();
	Unit->Abilities[1] = nullptr;

	const TArray<FRTAbilityCooldownView> Cds = URTHudViewModel::BuildAbilityCooldowns(Unit);

	// --- A. una riga per posizione, buco compreso ------------------------------------------------------
	TestEqual(TEXT("A: l'array non si accorcia quando una posizione e' vuota"), Cds.Num(), NumPrima);

	// --- B. il buco si riconosce, e porta comunque il proprio indice -----------------------------------
	if (TestTrue(TEXT("premessa: la riga del buco esiste"), Cds.IsValidIndex(1)))
	{
		TestTrue(TEXT("B: la posizione vuota si riconosce da `ActionId` nullo"), Cds[1].ActionId.IsNone());
		TestEqual(TEXT("B: e porta comunque il proprio indice di kit"), Cds[1].AbilityIndex, 1);
		TestFalse(TEXT("B: una posizione vuota non e' usabile"), Cds[1].bUsableNow);
	}

	// --- C. cio' che sta DOPO il buco non slitta -------------------------------------------------------
	// 🔑 E' l'asserzione che cade col `continue`: li' `Cds[2]` sarebbe stata l'azione della posizione 3.
	if (TestTrue(TEXT("premessa: la riga dopo il buco esiste"), Cds.IsValidIndex(2)))
	{
		TestEqual(TEXT("C: l'azione dopo il buco resta dove il tasto la cerca"),
			Cds[2].ActionId, IdDopoIlBuco);
		TestEqual(TEXT("C: e il suo indice non e' cambiato"), Cds[2].AbilityIndex, 2);
	}

	DestroyHudVmWorld(World);
	return true;
}

/**
 * 🔴 **LO STATO DI UNO SLOT E' UN VALORE, E LA SUA PRECEDENZA VIVE IN UN POSTO SOLO** (`#2988`).
 *
 * 🔑 **L'oracolo non e' «risponde qualcosa»: sono le COPPIE in conflitto.** Uno stato calcolato bene sui
 * casi puri — pronta, in ricarica, vuota — e sbagliato quando due condizioni valgono insieme passerebbe un
 * test scritto caso per caso. Le tre coppie qui sotto sono quelle che un grafo Blueprint ricomporrebbe in
 * ordine diverso, ed erano l'unica cosa che rendeva la deduzione locale pericolosa.
 *
 * ⚠️ **`Selected` sopra `Cooldown` non e' una scelta di questo test**: e' la regola che
 * `ARTHUD::ComposeAbilityLine` applica al colore da prima — *«Armata batte inutilizzabile»* — e qui viene
 * pinnata nella sede in cui ora vive.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmSlotStateTest,
	"RefactorTactics.HudViewModel.SlotStatePrefersTheArmedOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmSlotStateTest::RunTest(const FString&)
{
	// --- I casi puri, che sono la premessa delle coppie ------------------------------------------------
	FRTAbilityCooldownView Vuota; // `ActionId` nullo: e' cio' che dichiara la posizione vuota (#2987)
	TestEqual(TEXT("posizione vuota"),
		URTHudViewModel::ResolveSlotState(Vuota, /*bArmed=*/ false), ERTActionSlotState::Empty);

	FRTAbilityCooldownView Pronta;
	Pronta.ActionId = TEXT("Action.Guard");
	Pronta.bUsableNow = true;
	TestEqual(TEXT("pronta"),
		URTHudViewModel::ResolveSlotState(Pronta, false), ERTActionSlotState::Available);

	FRTAbilityCooldownView InRicarica = Pronta;
	InRicarica.TurnsRemaining = 2;
	InRicarica.bUsableNow = false;
	TestEqual(TEXT("in ricarica"),
		URTHudViewModel::ResolveSlotState(InRicarica, false), ERTActionSlotState::Cooldown);

	FRTAbilityCooldownView Pianificata = Pronta;
	Pianificata.bPlanned = true;
	TestEqual(TEXT("nel piano"),
		URTHudViewModel::ResolveSlotState(Pianificata, false), ERTActionSlotState::Planned);

	// --- A. armata BATTE in ricarica -------------------------------------------------------------------
	// 🔑 La regola di `ComposeAbilityLine`: un'ultimate armata e ancora in ricarica resta riconoscibile
	// come quella scelta, e il motivo lo dice il numero.
	TestEqual(TEXT("A: armata e in ricarica -> armata"),
		URTHudViewModel::ResolveSlotState(InRicarica, /*bArmed=*/ true), ERTActionSlotState::Selected);

	// --- B. armata BATTE pianificata -------------------------------------------------------------------
	// Una posizione puo' essere entrambe: si arma un'abilita' e la si pianifica su un bersaglio.
	TestEqual(TEXT("B: armata e pianificata -> armata"),
		URTHudViewModel::ResolveSlotState(Pianificata, /*bArmed=*/ true), ERTActionSlotState::Selected);

	// --- C. pianificata BATTE in ricarica --------------------------------------------------------------
	// Una reazione pianificata e poi entrata in ricarica resta un impegno preso: dire «in ricarica»
	// nasconderebbe che il turno la eseguira'.
	FRTAbilityCooldownView PianificataEInRicarica = InRicarica;
	PianificataEInRicarica.bPlanned = true;
	TestEqual(TEXT("C: pianificata e in ricarica -> pianificata"),
		URTHudViewModel::ResolveSlotState(PianificataEInRicarica, false), ERTActionSlotState::Planned);

	// --- D. vuota BATTE tutto --------------------------------------------------------------------------
	// ⛔ Senza questo, un segnaposto che ereditasse `bArmed` dall'unita' direbbe «armata» di una posizione
	// che non porta nessuna azione — e il dock accenderebbe uno slot vuoto.
	FRTAbilityCooldownView VuotaMaArmata;
	VuotaMaArmata.bPlanned = true;
	TestEqual(TEXT("D: una posizione vuota resta vuota anche se armata e pianificata"),
		URTHudViewModel::ResolveSlotState(VuotaMaArmata, /*bArmed=*/ true), ERTActionSlotState::Empty);

	return true;
}

/**
 * 🔴 **`Planned` ARRIVA ALLA VISTA, E LEGGE TUTTI E TRE I CAMPI DEL PIANO** (`#2988`).
 *
 * 🔑 **La reazione e' il caso che rende il test non ovvio.** `PlannedReactionAbility` esiste come campo
 * separato da `#601` — *«una reazione selezionata finiva nello slot PRINCIPALE, dove il pass delle reazioni
 * non la guarda mai»* — quindi una vista che leggesse il solo `PlannedAbilityIndex` direbbe «non
 * pianificata» di una reazione che il turno eseguira'. E' esattamente il difetto che `#2986` descrive dal
 * lato opposto: uno stato che afferma il contrario di cio' che il modello fara'.
 *
 * ⚠️ **Il controllo C non e' cortesia**: senza, un `bPlanned` scritto `true` per tutti passerebbe A e B.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmPlannedReachesTheViewTest,
	"RefactorTactics.HudViewModel.PlannedIsDistinctFromArmed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmPlannedReachesTheViewTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTUnit* Unit = SpawnHudVmUnit(World, TEXT("Hero.Aevik"), 0);
	if (!TestNotNull(TEXT("unita'"), Unit)) { DestroyHudVmWorld(World); return false; }

	if (!TestTrue(TEXT("premessa: il kit ha almeno tre posizioni"), Unit->NumAbilities() >= 3))
	{
		DestroyHudVmWorld(World);
		return false;
	}

	// --- A. la principale pianificata si vede ----------------------------------------------------------
	Unit->PlannedAbilityIndex = 1;
	{
		const TArray<FRTAbilityCooldownView> Cds = URTHudViewModel::BuildAbilityCooldowns(Unit);
		if (TestTrue(TEXT("premessa: la riga esiste"), Cds.IsValidIndex(1)))
		{
			TestTrue(TEXT("A: la principale pianificata arriva alla vista"), Cds[1].bPlanned);
		}
	}

	// --- B. la REAZIONE pure, e vive in un campo suo ---------------------------------------------------
	Unit->PlannedAbilityIndex = INDEX_NONE;
	Unit->PlannedReactionAbility = 2;
	{
		const TArray<FRTAbilityCooldownView> Cds = URTHudViewModel::BuildAbilityCooldowns(Unit);
		if (TestTrue(TEXT("premessa: la riga esiste"), Cds.IsValidIndex(2)))
		{
			TestTrue(TEXT("B: la reazione pianificata arriva alla vista"), Cds[2].bPlanned);
		}
	}

	// --- C. e chi NON e' nel piano non risulta pianificato ---------------------------------------------
	// ⛔ Senza, un `bPlanned` scritto `true` per tutti passerebbe A e B.
	{
		const TArray<FRTAbilityCooldownView> Cds = URTHudViewModel::BuildAbilityCooldowns(Unit);
		if (TestTrue(TEXT("premessa: la riga esiste"), Cds.IsValidIndex(0)))
		{
			TestFalse(TEXT("C: una posizione fuori dal piano non risulta pianificata"), Cds[0].bPlanned);
		}
	}

	// --- D. armato e pianificato restano DUE cose ------------------------------------------------------
	// `SelectedAbilityIndex` e' «cosa sto per fare», il piano e' «cosa ho gia' deciso»: armare la posizione
	// 0 non deve rendere pianificata la 0 ne' spianificare la 2.
	Unit->SelectAbility(0);
	{
		const TArray<FRTAbilityCooldownView> Cds = URTHudViewModel::BuildAbilityCooldowns(Unit);
		if (TestTrue(TEXT("premessa: le righe esistono"), Cds.IsValidIndex(2)))
		{
			TestFalse(TEXT("D: armare non pianifica"), Cds[0].bPlanned);
			TestTrue(TEXT("D: e non spianifica cio' che era nel piano"), Cds[2].bPlanned);
		}
	}

	DestroyHudVmWorld(World);
	return true;
}

/**
 * 🔵 **CHE COSA SOPRAVVIVE a un cambio di selezione — e cosa no** (`#2988`, deciso da [D-397] §5).
 *
 * ⌫ **Nasceva CARATTERIZZANTE — fotografava un comportamento che nessuno aveva deciso — e ora dichiara una
 * scelta.** [D-397] §5 ha risolto la domanda 3 di `#2990` mentre questo test era in main, e l'ha risolta in
 * due meta' che vanno nomi nate separatamente:
 *
 *  - **cambio di unita': SOPRAVVIVE, ed e' corretto.** `SelectedAbilityIndex` vive su `ARTUnit`: ritrovare
 *    su un'unita' cio' che le si era armato e' la conseguenza del modello. *«Va scritto, non corretto.»*
 *  - **risoluzione: NON sopravvive.** Uno slot armato dopo che il piano e' stato consumato afferma una
 *    scelta che non esiste piu', e con `bPlanned` la contraddizione diventa visibile — armato senza
 *    pianificato. Il ritorno al neutro sta nel **Cleanup**, un sito solo, lato autorita'.
 *
 * 🔑 **Il fatto che il test esistesse PRIMA della decisione e' il suo valore**: ha reso osservabile il
 * comportamento su cui la decisione si e' poi pronunciata, invece di lasciarlo indovinare.
 *
 * ⚠️ **La meta' del Cleanup non si prova qui**, e non per dimenticanza: richiede un turno risolto, quindi
 * vive dove gia' abita quell'invariante — `Turn.PlansDoNotSurviveTheTurn`, accanto agli altri campi che
 * muoiono nello stesso punto. Duplicarla qui significherebbe un secondo posto da aggiornare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVmArmingLifecycleTest,
	"RefactorTactics.HudViewModel.ArmingIsPerUnitAndReturnsToNeutralAtCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVmArmingLifecycleTest::RunTest(const FString&)
{
	UWorld* World = MakeHudVmWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTUnit* Prima = SpawnHudVmUnit(World, TEXT("Hero.Aevik"), 0);
	ARTUnit* Seconda = SpawnHudVmUnit(World, TEXT("Hero.Muiren"), 0);
	if (!Prima || !Seconda)
	{
		DestroyHudVmWorld(World);
		return TestTrue(TEXT("due unita' esistono"), false);
	}

	// --- A. l'armamento e' PER UNITA', non del controller ----------------------------------------------
	Prima->SelectAbility(1);
	TestEqual(TEXT("A: la prima unita' porta il proprio armamento"), Prima->SelectedAbilityIndex, 1);
	TestEqual(TEXT("A: e la seconda resta al neutro di D-128"),
		Seconda->SelectedAbilityIndex, static_cast<int32>(INDEX_NONE));

	// --- B. armare la seconda non disarma la prima -----------------------------------------------------
	// ✅ **Deciso da [D-397] §5**, e non piu' una fotografia: l'armamento e' stato dell'unita', quindi due
	// unita' possono essere armate insieme, ciascuna sulla propria voce. Cio' che NON sopravvive e' la
	// risoluzione, e quella meta' vive in `Turn.PlansDoNotSurviveTheTurn`.
	Seconda->SelectAbility(0);
	TestEqual(TEXT("B: la prima resta armata quando si arma la seconda — l'armamento e' dell'unita'"),
		Prima->SelectedAbilityIndex, 1);

	// --- C. il neutro e' raggiungibile, ed e' un ingresso legittimo ------------------------------------
	// ⛔ Questa meta' NON e' in attesa di decisione: `SelectAbility(INDEX_NONE)` disarma per contratto —
	// *«senza di esso non esisterebbe un modo di tornare allo stato neutro»* — ed e' cio' su cui poggiano
	// il disarmo col click (`ArmKitAbility`) e l'uscita dal targeting con `RMB`.
	Prima->SelectAbility(INDEX_NONE);
	TestEqual(TEXT("C: il neutro si raggiunge"),
		Prima->SelectedAbilityIndex, static_cast<int32>(INDEX_NONE));

	// --- D. un indice fuori range non arma e non disarma -----------------------------------------------
	Seconda->SelectAbility(9999);
	TestEqual(TEXT("D: un indice non valido lascia l'armamento dov'era"), Seconda->SelectedAbilityIndex, 0);

	DestroyHudVmWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
