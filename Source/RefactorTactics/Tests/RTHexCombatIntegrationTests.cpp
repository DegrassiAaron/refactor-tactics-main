#include "Misc/AutomationTest.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Turn/RTActionFallbackLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

/**
 * Combat end-to-end su griglia esagonale (CP 6.4): la fase Blast valida portata e linea di tiro sulla
 * MAPPA esagonale (URTHexVisionLibrary) e risolve le forme con le primitive hex, non con la griglia
 * quadrata. I piani si scrivono a mano sui campi Planned*, cosi' il test programma il turno.
 */
namespace
{
	UWorld* MakeHexBlastWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyHexBlastWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Mappa esagonale di prova nel livello; le celle in SightBlockers bloccano la linea di tiro. */
	ARTHexMapActor* SpawnHexBlastMap(UWorld* World, int32 Radius, const TArray<FRTCellId>& SightBlockers = TArray<FRTCellId>())
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			FRTHexCellData Data(Id);
			Data.bBlocksLineOfSight = SightBlockers.Contains(Id);
			M->AddOrUpdateCell(Data);
		}
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SpawnHexBlastUnit(UWorld* World, int32 TeamId, ERTArchetype Arch, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // niente decisioni del bot in mezzo
		U->ConfigureAsArchetype(Arch);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell; // fermo: il test verifica il Blast, non il movimento
		return U;
	}

	void RunBlastTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** Quante voci di combattimento col dato esito compaiono nel TurnLog. */
	int32 CountCombatOutcome(const ARTTurnManager* TM, ERTCombatOutcome Outcome)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Combat && E.Outcome == static_cast<uint8>(Outcome)) { ++N; }
		}
		return N;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBlastDealsDamageTest,
	"RefactorTactics.HexBlast.AttackDealsDamageOnHexMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBlastDealsDamageTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBlastMap(World, /*Radius=*/ 6);

	// Ranger: "Tiro" (Single, portata 6, 25 danni). Bersaglio a distanza esagonale 3, vista libera.
	ARTUnit* Shooter = SpawnHexBlastUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexBlastUnit(World, 1, ERTArchetype::Guardian, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Shooter || !Foe) { DestroyHexBlastWorld(World); return false; }

	const int32 HealthBefore = Foe->Health;
	const int32 ShieldBefore = Foe->Shield;
	Shooter->PlannedAbilityIndex = 0;
	Shooter->PlannedAttackTarget = Foe;

	RunBlastTurn(TM);

	TestTrue(TEXT("il bersaglio ha incassato il colpo"), Foe->Shield < ShieldBefore || Foe->Health < HealthBefore);
	TestEqual(TEXT("scudo assorbito per primo, poi HP"), Foe->Shield, FMath::Max(0, ShieldBefore - 25));
	TestEqual(TEXT("HP residui coerenti col danno"), Foe->Health, HealthBefore - FMath::Max(0, 25 - ShieldBefore));
	TestTrue(TEXT("il TurnLog registra il colpo"), CountCombatOutcome(TM, ERTCombatOutcome::Hit) >= 1);

	DestroyHexBlastWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBlastBlockedBySightTest,
	"RefactorTactics.HexBlast.NoLineOfSightOnHexMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBlastBlockedBySightTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	// Muro esagonale sulla traiettoria (0,0) -> (3,0): la LOS deve essere valutata sull'ASSET della mappa.
	TArray<FRTCellId> Walls;
	Walls.Add(FRTCellId(1, 0));
	Walls.Add(FRTCellId(2, 0));
	SpawnHexBlastMap(World, /*Radius=*/ 6, Walls);

	ARTUnit* Shooter = SpawnHexBlastUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexBlastUnit(World, 1, ERTArchetype::Guardian, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Shooter || !Foe) { DestroyHexBlastWorld(World); return false; }

	const int32 HealthBefore = Foe->Health;
	const int32 ShieldBefore = Foe->Shield;
	Shooter->PlannedAbilityIndex = 0;
	Shooter->PlannedAttackTarget = Foe;

	RunBlastTurn(TM);

	TestEqual(TEXT("nessun danno attraverso la copertura"), Foe->Health, HealthBefore);
	TestEqual(TEXT("scudo intatto"), Foe->Shield, ShieldBefore);
	TestEqual(TEXT("il TurnLog spiega il perche' (nessuna linea di tiro)"),
		CountCombatOutcome(TM, ERTCombatOutcome::NoLineOfSight), 1);

	DestroyHexBlastWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBlastKnockbackTest,
	"RefactorTactics.HexBlast.KnockbackOnHexGrid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBlastKnockbackTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBlastMap(World, /*Radius=*/ 6);

	// La "Spazzata" del Guardian (cono, portata 3) respinge di 2 celle. Bersaglio in direzione OBLIQUA NE:
	// la spinta esagonale prosegue lungo (+1,-1) fino a (3,-3), mentre quella cardinale del quadrato
	// sceglierebbe l'asse X (delta pari) e finirebbe in (3,-1). E' il caso che distingue le due geometrie.
	ARTUnit* Bruiser = SpawnHexBlastUnit(World, 1, ERTArchetype::Guardian, FRTCellId(0, 0));
	ARTUnit* Victim = SpawnHexBlastUnit(World, 0, ERTArchetype::Ranger, FRTCellId(1, -1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Bruiser || !Victim) { DestroyHexBlastWorld(World); return false; }

	Bruiser->PlannedAbilityIndex = 0; // Spazzata
	Bruiser->PlannedAttackTarget = Victim;

	RunBlastTurn(TM);

	TestTrue(TEXT("il bersaglio e' stato respinto di due celle esagonali"), Victim->Cell == FRTCellId(3, -3));
	TestTrue(TEXT("resta su una cella esistente della mappa"), URTHexLibrary::HexDistance(Victim->Cell, FRTCellId(0, 0)) <= 6);
	TestTrue(TEXT("posizione visiva coerente con la cella logica"),
		Victim->GetActorLocation().Equals(Victim->WorldForCell(Victim->Cell, FVector::ZeroVector, 100.f, 250.f), 1.0f));

	DestroyHexBlastWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBlastOutOfRangeTest,
	"RefactorTactics.HexBlast.OutOfHexRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBlastOutOfRangeTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBlastMap(World, /*Radius=*/ 9);

	// Distanza ESAGONALE 8 > portata 6 del "Tiro": la portata si misura in celle esagonali, non in Manhattan.
	ARTUnit* Shooter = SpawnHexBlastUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexBlastUnit(World, 1, ERTArchetype::Guardian, FRTCellId(8, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Shooter || !Foe) { DestroyHexBlastWorld(World); return false; }

	const int32 HealthBefore = Foe->Health;
	Shooter->PlannedAbilityIndex = 0;
	Shooter->PlannedAttackTarget = Foe;

	RunBlastTurn(TM);

	TestEqual(TEXT("fuori portata: nessun danno"), Foe->Health, HealthBefore);
	TestEqual(TEXT("fuori portata non e' un problema di linea di tiro"),
		CountCombatOutcome(TM, ERTCombatOutcome::NoLineOfSight), 0);

	DestroyHexBlastWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBlastFallbackLoggedTest,
	"RefactorTactics.Actions.Fallback.CancelIsLoggedInMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBlastFallbackLoggedTest::RunTest(const FString&)
{
	// Il caso vero di un turno simultaneo: si punta un bersaglio in pianificazione, e quando l'attacco risolve
	// il bersaglio se n'e' andato con uno scatto (fase Dash, PRIMA del Blast). Prima di CP 4.3 l'azione
	// spariva in silenzio; ora applica il fallback dichiarato (`Cancel`) e lo REGISTRA col motivo.
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBlastMap(World, /*Radius=*/ 9);

	ARTUnit* Attacker = SpawnHexBlastUnit(World, 0, ERTArchetype::Guardian, FRTCellId(0, 0)); // Spazzata, portata 3
	ARTUnit* Runner = SpawnHexBlastUnit(World, 1, ERTArchetype::Ranger, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attacker || !Runner) { DestroyHexBlastWorld(World); return false; }

	const int32 HealthBefore = Runner->Health;
	Attacker->PlannedAbilityIndex = 0;
	Attacker->PlannedAttackTarget = Runner;

	// Il bersaglio scatta a distanza 7: fuori dalla portata 3 della Spazzata quando il Blast risolve.
	Runner->PlannedDashAbility = Runner->FindDashAbilityIndex();
	Runner->PlannedDashCell = FRTCellId(7, 0);

	RunBlastTurn(TM);

	TestTrue(TEXT("il bersaglio si e' spostato prima del Blast"), Runner->Cell == FRTCellId(7, 0));
	TestEqual(TEXT("l'attacco non lo raggiunge"), Runner->Health, HealthBefore);

	int32 Fallbacks = 0;
	int32 OutOfRangeReasons = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category != ERTLogCategory::Fallback) { continue; }
		++Fallbacks;
		if (E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::OutOfRange))
		{
			++OutOfRangeReasons;
		}
	}
	TestEqual(TEXT("il TurnLog registra un fallback"), Fallbacks, 1);
	TestEqual(TEXT("annullata perche' fuori portata: l'esito dice anche il motivo"), OutOfRangeReasons, 1);

	// E lo dice anche il combat log della HUD, non solo il log autoritativo.
	bool bInCombatLog = false;
	for (const FString& Line : TM->GetRecentEvents())
	{
		if (Line.Contains(TEXT("annullata")) && Line.Contains(TEXT("fuori portata"))) { bInCombatLog = true; }
	}
	TestTrue(TEXT("il combat log lo mostra a chi gioca"), bInCombatLog);

	DestroyHexBlastWorld(World);
	return true;
}

namespace
{
	/** Da' all'unita' un'azione del catalogo generico e ne restituisce l'indice. */
	int32 AddCoreAbility(ARTUnit* Unit, const TCHAR* ActionId)
	{
		if (!Unit) { return INDEX_NONE; }
		URTActionData* Action = NewObject<URTActionData>(Unit);
		Action->Def = URTCatalogLibrary::FindCoreAction(FName(ActionId));
		Action->RangeCells = Action->Def.RangeCells;
		Action->Power = 0;
		Action->bSelfTarget = true;
		Unit->Abilities.Add(Action);
		return Unit->Abilities.Num() - 1;
	}

	/** Un attacco di prova che spinge di N celle: serve a distinguere la spinta da 1 da quella piu' forte. */
	int32 AddPushAbility(ARTUnit* Unit, int32 PushCells)
	{
		if (!Unit) { return INDEX_NONE; }
		URTActionData* Action = NewObject<URTActionData>(Unit);
		Action->Def.ActionId = TEXT("Action.TestPush");
		Action->Def.ResolutionPhase = ERTResolutionPhase::Attack;
		Action->Def.RangeCells = 3;
		Action->Def.Fallback = ERTActionFallback::Cancel;
		Action->Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 10));
		Action->Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Push, PushCells));
		Action->RangeCells = 3;
		Action->Power = 10;
		Unit->Abilities.Add(Action);
		return Unit->Abilities.Num() - 1;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardReducesDamageInMatchTest,
	"RefactorTactics.Actions.Guard.ReducesFirstDirectDamageInMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardReducesDamageInMatchTest::RunTest(const FString&)
{
	// `Action.Guard` end-to-end: si prepara nel Prep, e nel Blast dello stesso turno toglie 15 al primo colpo.
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBlastMap(World, /*Radius=*/ 6);

	ARTUnit* Defender = SpawnHexBlastUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	ARTUnit* Shooter = SpawnHexBlastUnit(World, 1, ERTArchetype::Ranger, FRTCellId(3, 0)); // Tiro: 25 danni
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Defender || !Shooter) { DestroyHexBlastWorld(World); return false; }

	const int32 StartHealth = Defender->Health;
	const int32 GuardIdx = AddCoreAbility(Defender, TEXT("Action.Guard"));
	Defender->PlannedAbilityIndex = GuardIdx;
	Defender->PlannedAttackTarget = Defender; // su se stessi: la guardia si prepara addosso
	Shooter->PlannedAbilityIndex = 0;
	Shooter->PlannedAttackTarget = Defender;

	RunBlastTurn(TM);

	TestEqual(TEXT("in guardia il primo colpo fa 25 - 15"), StartHealth - Defender->Health, 10);

	// Controprova: senza guardia lo stesso tiro arriva intero.
	Defender->ApplyCombatState(StartHealth, 0);
	Defender->PlannedAbilityIndex = INDEX_NONE;
	Shooter->PlannedAbilityIndex = 0;
	Shooter->PlannedAttackTarget = Defender;

	RunBlastTurn(TM);

	TestEqual(TEXT("senza guardia il colpo fa 25: la riduzione viene dallo stato"),
		StartHealth - Defender->Health, 25);

	DestroyHexBlastWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardResistsPushTest,
	"RefactorTactics.Actions.Guard.ResistsSinglePush",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardResistsPushTest::RunTest(const FString&)
{
	// La guardia regge UNA cella di spinta, non piu': non e' un'ancora.
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBlastMap(World, /*Radius=*/ 6);

	ARTUnit* Defender = SpawnHexBlastUnit(World, 0, ERTArchetype::Guardian, FRTCellId(0, 0));
	ARTUnit* Pusher = SpawnHexBlastUnit(World, 1, ERTArchetype::Ranger, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Defender || !Pusher) { DestroyHexBlastWorld(World); return false; }

	const int32 GuardIdx = AddCoreAbility(Defender, TEXT("Action.Guard"));
	const int32 Push1 = AddPushAbility(Pusher, /*PushCells=*/ 1);
	const FRTCellId Start = Defender->Cell;

	Defender->PlannedAbilityIndex = GuardIdx;
	Defender->PlannedAttackTarget = Defender;
	Pusher->PlannedAbilityIndex = Push1;
	Pusher->PlannedAttackTarget = Defender;

	RunBlastTurn(TM);

	TestTrue(TEXT("in guardia, una spinta di 1 cella non sposta"), Defender->Cell == Start);

	// Una spinta piu' forte passa comunque: la guardia attutisce, non ancora.
	Defender->PlaceOnCell(Start, FVector::ZeroVector, 100.f, 250.f);
	Defender->PlannedCell = Start;
	Defender->PlannedAbilityIndex = AddCoreAbility(Defender, TEXT("Action.Guard"));
	Defender->PlannedAttackTarget = Defender;
	Pusher->PlannedAbilityIndex = AddPushAbility(Pusher, /*PushCells=*/ 2);
	Pusher->PlannedAttackTarget = Defender;
	Pusher->PlannedCell = Pusher->Cell;

	RunBlastTurn(TM);

	TestTrue(TEXT("una spinta di 2 celle sposta anche chi e' in guardia"), !(Defender->Cell == Start));

	DestroyHexBlastWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTChargeImpactInBlastTest,
	"RefactorTactics.Actions.Charge.ImpactResolvesInBlast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTChargeImpactInBlastTest::RunTest(const FString&)
{
	// La carica in partita: il MOVIMENTO risolve nella fase Dash, il COLPO nel Blast (codice 20/30 del
	// catalogo). Si verifica che accadano entrambe le cose e nell'ordine giusto — l'unita' si ferma davanti al
	// bersaglio, e il bersaglio incassa 20 danni piu' una spinta di 1.
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBlastMap(World, /*Radius=*/ 6);

	ARTUnit* Charger = SpawnHexBlastUnit(World, 0, ERTArchetype::Guardian, FRTCellId(0, 0));
	ARTUnit* Victim = SpawnHexBlastUnit(World, 1, ERTArchetype::Ranger, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Charger || !Victim) { DestroyHexBlastWorld(World); return false; }

	URTActionData* Charge = NewObject<URTActionData>(Charger);
	Charge->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge"));
	Charge->RangeCells = Charge->Def.RangeCells;
	Charger->Abilities.Add(Charge);

	const int32 VictimHealth = Victim->Health;
	Charger->PlannedDashAbility = Charger->Abilities.Num() - 1;
	Charger->PlannedDashCell = FRTCellId(3, 0); // dritto: incontra il bersaglio a (2,0) per strada

	RunBlastTurn(TM);

	TestTrue(TEXT("la carica si ferma davanti al bersaglio, non lo attraversa"), Charger->Cell == FRTCellId(1, 0));
	TestEqual(TEXT("l'impatto fa 20 danni, applicati nel Blast"), VictimHealth - Victim->Health, 20);
	TestTrue(TEXT("e lo spinge di una cella"), Victim->Cell == FRTCellId(3, 0));

	DestroyHexBlastWorld(World);
	return true;
}
