#include "Misc/AutomationTest.h"
#include "Ability/RTAbilityData.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

/**
 * Pianificazione dei bot su griglia esagonale (CP 6.6): ARTTurnManager::PlanBots passa da URTHexBotLibrary,
 * quindi le mosse proposte nascono da ReachableCells e non possono essere illegali sulla mappa esagonale.
 * Le guardie del quadrato (supporto, panico del kiter) restano nel TurnManager e vanno verificate qui.
 */
namespace
{
	UWorld* MakeHexBotWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyHexBotWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	URTHexMapAsset* SpawnHexBotMap(UWorld* World, int32 Radius)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return M;
	}

	ARTUnit* SpawnHexBotUnit(UWorld* World, int32 TeamId, ERTArchetype Arch, const FRTCellId& Cell, bool bBot)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureAsArchetype(Arch);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = bBot;
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell;
		return U;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotLegalMovesTest,
	"RefactorTactics.HexBotPlay.PlansOnlyLegalMoves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotLegalMovesTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBotWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	URTHexMapAsset* Map = SpawnHexBotMap(World, /*Radius=*/ 5);

	// 2v2: due bot contro due unita' del giocatore, in posizioni oblique (dove Manhattan e distanza
	// esagonale divergono e il pathfinding quadrato proporrebbe celle fuori dalla mappa).
	ARTUnit* BotA = SpawnHexBotUnit(World, 1, ERTArchetype::Guardian, FRTCellId(2, -3), /*bBot*/ true);
	ARTUnit* BotB = SpawnHexBotUnit(World, 1, ERTArchetype::Ranger, FRTCellId(3, -3), /*bBot*/ true);
	ARTUnit* FoeA = SpawnHexBotUnit(World, 0, ERTArchetype::Ranger, FRTCellId(-2, 3), /*bBot*/ false);
	ARTUnit* FoeB = SpawnHexBotUnit(World, 0, ERTArchetype::Guardian, FRTCellId(-3, 3), /*bBot*/ false);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !BotA || !BotB || !FoeA || !FoeB) { DestroyHexBotWorld(World); return false; }

	TM->PlanBotsForTest();

	TArray<ARTUnit*> Bots;
	Bots.Add(BotA);
	Bots.Add(BotB);

	// Senza questa verifica il test passerebbe anche con un pianificatore che non fa NULLA: restare fermi
	// e' sempre legale. I bot partono a distanza 6 dai nemici, fuori dalla portata di entrambi gli archetipi,
	// quindi ciascuno deve muoversi (o scattare) per avvicinarsi.
	for (ARTUnit* Bot : Bots)
	{
		const bool bActs = !(Bot->PlannedCell == Bot->Cell)
			|| Bot->PlannedDashAbility != INDEX_NONE
			|| Bot->PlannedAttackTarget != nullptr;
		TestTrue(TEXT("il bot pianifica qualcosa invece di restare immobile"), bActs);
	}

	for (ARTUnit* Bot : Bots)
	{
		// Mossa legale sulla griglia ESAGONALE: cella esistente, entro il budget di movimento, non occupata.
		TestTrue(TEXT("la destinazione esiste nella mappa esagonale"), Map->ContainsCell(Bot->PlannedCell));
		TestTrue(TEXT("la destinazione e' entro il budget di movimento"),
			URTHexLibrary::HexDistance(Bot->Cell, Bot->PlannedCell) <= Bot->GetEffectiveMoveRange());

		if (Bot->PlannedDashAbility != INDEX_NONE)
		{
			const URTAbilityData* Dash = Bot->GetAbility(Bot->PlannedDashAbility);
			TestTrue(TEXT("lo scatto pianificato e' un'abilita' di scatto"), Dash && Dash->bDash);
			TestTrue(TEXT("la cella di scatto esiste nella mappa"), Map->ContainsCell(Bot->PlannedDashCell));
			TestTrue(TEXT("la cella di scatto e' entro la portata dello scatto"),
				Dash && URTHexLibrary::HexDistance(Bot->Cell, Bot->PlannedDashCell) <= Bot->GetEffectiveDashRange(Dash->RangeCells));
		}

		if (Bot->PlannedAttackTarget)
		{
			TestTrue(TEXT("il bersaglio e' un nemico"), Bot->PlannedAttackTarget->TeamId != Bot->TeamId);
			// L'attacco parte dalla cella in cui il bot si trovera' nel Blast: quella attuale, o quella
			// post-scatto se ha pianificato uno scatto (il Dash precede il Blast).
			const FRTCellId FiringCell = Bot->PlannedDashAbility != INDEX_NONE ? Bot->PlannedDashCell : Bot->Cell;
			const URTAbilityData* Ability = Bot->GetAbility(Bot->PlannedAbilityIndex);
			TestTrue(TEXT("il bersaglio e' entro la portata dalla cella di tiro"),
				Ability && URTHexLibrary::HexDistance(FiringCell, Bot->PlannedAttackTarget->Cell) <= Ability->RangeCells);
		}
	}

	DestroyHexBotWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotPanicTest,
	"RefactorTactics.HexBotPlay.KiterFleesWhenThreatened",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotPanicTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBotWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBotMap(World, /*Radius=*/ 6);

	// Il Ranger ha KiteStandoff 4: con un nemico a distanza 2 (<= standoff/2) scatta il panico e arretra.
	ARTUnit* Kiter = SpawnHexBotUnit(World, 1, ERTArchetype::Ranger, FRTCellId(0, 0), /*bBot*/ true);
	ARTUnit* Melee = SpawnHexBotUnit(World, 0, ERTArchetype::Guardian, FRTCellId(2, 0), /*bBot*/ false);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Kiter || !Melee) { DestroyHexBotWorld(World); return false; }

	TM->PlanBotsForTest();

	// La fuga puo' avvenire col movimento normale o con lo scatto difensivo: conta il risultato.
	const FRTCellId Escape = Kiter->PlannedDashAbility != INDEX_NONE ? Kiter->PlannedDashCell : Kiter->PlannedCell;
	TestTrue(TEXT("il kiter si allontana dalla minaccia"),
		URTHexLibrary::HexDistance(Escape, Melee->Cell) > URTHexLibrary::HexDistance(Kiter->Cell, Melee->Cell));

	DestroyHexBotWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotSupportTest,
	"RefactorTactics.HexBotPlay.UsesSupportWhenHurt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotSupportTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBotWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBotMap(World, /*Radius=*/ 5);

	// Guardian ferito sotto meta' HP: usa la Barriera (abilita' self-target) invece di attaccare.
	ARTUnit* Hurt = SpawnHexBotUnit(World, 1, ERTArchetype::Guardian, FRTCellId(0, 0), /*bBot*/ true);
	ARTUnit* Foe = SpawnHexBotUnit(World, 0, ERTArchetype::Ranger, FRTCellId(2, 0), /*bBot*/ false);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Hurt || !Foe) { DestroyHexBotWorld(World); return false; }

	Hurt->ApplyCombatState(/*Health*/ 20, /*Shield*/ 0); // sotto meta' dei 140 HP

	TM->PlanBotsForTest();

	const URTAbilityData* Planned = Hurt->GetAbility(Hurt->PlannedAbilityIndex);
	TestTrue(TEXT("pianifica un'abilita' di supporto su se stesso"), Planned && Planned->bSelfTarget);
	TestNull(TEXT("non pianifica un attacco nello stesso turno"), Hurt->PlannedAttackTarget.Get());

	DestroyHexBotWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotTuningTest,
	"RefactorTactics.HexBotPlay.WThreatTuning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotTuningTest::RunTest(const FString&)
{
	// I pesi UPROPERTY del TurnManager devono arrivare al contesto del bot: cambiandoli cambia la scelta
	// senza ricompilare (è ciò che rende possibile il tuning in PIE, PIE-BU2b).
	UWorld* World = MakeHexBotWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBotMap(World, /*Radius=*/ 6);

	ARTUnit* Bot = SpawnHexBotUnit(World, 1, ERTArchetype::Guardian, FRTCellId(0, 0), /*bBot*/ true);
	ARTUnit* Foe = SpawnHexBotUnit(World, 0, ERTArchetype::Guardian, FRTCellId(4, 0), /*bBot*/ false);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Bot || !Foe) { DestroyHexBotWorld(World); return false; }

	// Senza lo scatto (Carica): il riposizionamento rapido mascherebbe l'effetto del peso sul movimento.
	const int32 DashIdx = Bot->FindDashAbilityIndex();
	if (DashIdx != INDEX_NONE) { Bot->Abilities.RemoveAt(DashIdx); }

	TM->WThreat = 100000; // avanzare espone -> resta lontano
	TM->PlanBotsForTest();
	const int32 DistHighThreat = URTHexLibrary::HexDistance(Bot->PlannedCell, Foe->Cell);

	TM->WThreat = 0;      // la minaccia non conta -> la mischia chiude la distanza
	TM->PlanBotsForTest();
	const int32 DistLowThreat = URTHexLibrary::HexDistance(Bot->PlannedCell, Foe->Cell);

	TestTrue(TEXT("WThreat basso avvicina il Guardian al nemico piu' che WThreat alto"),
		DistLowThreat < DistHighThreat);

	DestroyHexBotWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotDashThreatTest,
	"RefactorTactics.HexBotPlay.DashRespectsThreat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotDashThreatTest::RunTest(const FString&)
{
	// Col dash disponibile: con la minaccia alta il bot NON scatta in una cella esposta. Si verifica la
	// posizione EFFETTIVA (cella di scatto se scatta, altrimenti quella di movimento).
	UWorld* World = MakeHexBotWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBotMap(World, /*Radius=*/ 6);

	ARTUnit* Bot = SpawnHexBotUnit(World, 1, ERTArchetype::Guardian, FRTCellId(0, 0), /*bBot*/ true); // con Carica
	ARTUnit* Foe = SpawnHexBotUnit(World, 0, ERTArchetype::Guardian, FRTCellId(5, 0), /*bBot*/ false);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Bot || !Foe) { DestroyHexBotWorld(World); return false; }

	auto Effective = [](const ARTUnit* U) -> FRTCellId
	{
		return (U->PlannedDashAbility != INDEX_NONE) ? U->PlannedDashCell : U->PlannedCell;
	};

	TM->WThreat = 100000; // scattare in una cella esposta deve essere rifiutato
	TM->PlanBotsForTest();
	const int32 DistHigh = URTHexLibrary::HexDistance(Effective(Bot), Foe->Cell);

	TM->WThreat = 0;      // minaccia ignorata: il bot chiude col dash
	TM->PlanBotsForTest();
	const int32 DistLow = URTHexLibrary::HexDistance(Effective(Bot), Foe->Cell);

	TestTrue(TEXT("col dash: WThreat alto tiene il bot piu' lontano (niente scatto in cella esposta)"),
		DistLow < DistHigh);

	DestroyHexBotWorld(World);
	return true;
}
