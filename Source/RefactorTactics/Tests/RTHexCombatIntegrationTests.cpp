#include "Misc/AutomationTest.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexDoorLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Turn/RTActionFallbackLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"

// La guardia: senza, i test di questo file finiscono compilati DENTRO il binario Shipping che si
// distribuisce. Non e' una formalita' di build — e' cio' che tiene il codice di test fuori dal gioco.
// Aggiunta con #923, dopo che lo stesso difetto ha rotto la build Shipping due volte (2026-08-09 e
// 2026-08-15) senza che la suite, che gira sul target Editor, se ne accorgesse.
#if WITH_DEV_AUTOMATION_TESTS

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

	ARTUnit* SpawnHexBlastUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // niente decisioni del bot in mezzo
		U->ConfigureFromHeroData(Hero);
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

	// Attacco base dell'eroe (indice 0). Bersaglio a distanza esagonale 3, vista libera.
	ARTUnit* Shooter = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Shooter || !Foe) { DestroyHexBlastWorld(World); return false; }

	const int32 HealthBefore = Foe->Health;
	const int32 ShieldBefore = Foe->Shield;
	Shooter->PlannedAbilityIndex = 0;
	Shooter->PlannedAttackTarget = Foe;

	RunBlastTurn(TM);

	TestTrue(TEXT("il bersaglio ha incassato il colpo"), Foe->Shield < ShieldBefore || Foe->Health < HealthBefore);
	// Il colpo pieno lo dichiara l'attacco base di chi spara: la proprieta' e' «lo scudo assorbe per primo».
	const int32 FullHit = Shooter->AttackPower;
	TestEqual(TEXT("scudo assorbito per primo, poi HP"), Foe->Shield, FMath::Max(0, ShieldBefore - FullHit));
	TestEqual(TEXT("HP residui coerenti col danno"), Foe->Health, HealthBefore - FMath::Max(0, FullHit - ShieldBefore));
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

	ARTUnit* Shooter = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(3, 0));
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


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBlastOutOfRangeTest,
	"RefactorTactics.HexBlast.OutOfHexRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBlastOutOfRangeTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBlastMap(World, /*Radius=*/ 9);

	// Distanza ESAGONALE 8 > portata 6 del "Tiro": la portata si misura in celle esagonali, non in Manhattan.
	ARTUnit* Shooter = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(8, 0));
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

	ARTUnit* Attacker = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0)); // Spazzata, portata 3
	ARTUnit* Runner = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attacker || !Runner) { DestroyHexBlastWorld(World); return false; }

	const int32 HealthBefore = Runner->Health;
	Attacker->PlannedAbilityIndex = 0;
	Attacker->PlannedAttackTarget = Runner;

	// Il bersaglio scatta lontano quanto la portata del PROPRIO scatto: era scritto `7`, che solo lo scatto
	// da 5 celle del Ranger legacy raggiungeva. La destinazione si deriva, cosi' la premessa del test —
	// «quando il Blast risolve il bersaglio e' fuori portata» — resta vera con qualunque eroe scappi.
	const int32 DashIdx = Runner->FindDashAbilityIndex();
	if (!TestTrue(TEXT("premessa: chi scappa ha uno scatto"), Runner->Abilities.IsValidIndex(DashIdx)))
	{
		DestroyHexBlastWorld(World);
		return false;
	}
	const int32 DashRange = Runner->Abilities[DashIdx] ? Runner->Abilities[DashIdx]->RangeCells : 0;
	const FRTCellId Escape(2 + DashRange, 0);
	TestTrue(TEXT("premessa: la fuga porta oltre la portata di chi attacca"),
		URTHexLibrary::HexDistance(Escape, Attacker->Cell) > Attacker->AttackRange);

	Runner->PlannedDashAbility = DashIdx;
	Runner->PlannedDashCell = Escape;

	RunBlastTurn(TM);

	TestTrue(TEXT("il bersaglio si e' spostato prima del Blast"), Runner->Cell == Escape);
	TestEqual(TEXT("l'attacco non lo raggiunge"), Runner->Health, HealthBefore);

	int32 Fallbacks = 0;
	int32 OutOfRangeReasons = 0;
	int32 ConIdentita = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category != ERTLogCategory::Fallback) { continue; }
		++Fallbacks;
		if (E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::OutOfRange))
		{
			++OutOfRangeReasons;
		}
		if (!E.ActionId.IsNone())
		{
			++ConIdentita;
		}
	}
	TestEqual(TEXT("il TurnLog registra un fallback"), Fallbacks, 1);
	TestEqual(TEXT("annullata perche' fuori portata: l'esito dice anche il motivo"), OutOfRangeReasons, 1);
	// QUALE azione e' fallita ([D-196], `#1412` punto 1b). Senza, un'azione che non avviene lascia una voce
	// che non dice se a mancare sia stata l'ultimate o l'attacco base — e due annullamenti della stessa
	// unita' nello stesso turno erano indistinguibili.
	TestEqual(TEXT("e la voce dice QUALE azione e' fallita"), ConIdentita, 1);

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

	ARTUnit* Defender = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	ARTUnit* Shooter = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(3, 0)); // Tiro: 25 danni
	if (Shooter) { Shooter->Facing = ERTHexDirection::W; }
	// Guarda il proprio bersaglio: da CP 13.2 il targeting consuma la conoscenza, e un tiratore rivolto
	// altrove non VEDE cio' che sta a piu' di due celle. In partita l'orientamento lo deriva il movimento;
	// qui l'unita' e' ferma, quindi lo si dichiara.
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Defender || !Shooter) { DestroyHexBlastWorld(World); return false; }

	const int32 StartHealth = Defender->Health;
	const int32 GuardIdx = AddCoreAbility(Defender, TEXT("Action.Guard"));
	Defender->PlannedAbilityIndex = GuardIdx;
	Defender->PlannedAttackTarget = Defender; // su se stessi: la guardia si prepara addosso
	Shooter->PlannedAbilityIndex = 0;
	Shooter->PlannedAttackTarget = Defender;

	RunBlastTurn(TM);

	// Il danno pieno lo dichiara l'attacco base di CHI SPARA, non un numero scritto qui: la proprieta' sotto
	// esame e' «Guard toglie 15 al primo colpo diretto», e deve reggere qualunque eroe schieri il test.
	// Prima era `25`, il danno del Ranger legacy, e cambiare unita' faceva cadere il test su un dettaglio
	// che non stava verificando.
	const int32 FullHit = Shooter->AttackPower;
	TestEqual(TEXT("in guardia il primo colpo fa FullHit - 15"), StartHealth - Defender->Health, FullHit - 15);

	// Controprova: senza guardia lo stesso tiro arriva intero.
	Defender->ApplyCombatState(StartHealth, 0);
	Defender->PlannedAbilityIndex = INDEX_NONE;
	Shooter->PlannedAbilityIndex = 0;
	Shooter->PlannedAttackTarget = Defender;

	RunBlastTurn(TM);

	TestEqual(TEXT("senza guardia il colpo arriva intero: la riduzione viene dallo stato"),
		StartHealth - Defender->Health, FullHit);

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

	ARTUnit* Defender = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0));
	ARTUnit* Pusher = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, 0));
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

	ARTUnit* Charger = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0));
	ARTUnit* Victim = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, 0));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTChargeHeadOnStopsTest,
	"RefactorTactics.Actions.Charge.HeadOnStops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTChargeHeadOnStopsTest::RunTest(const FString&)
{
	// Nome vincolante del catalogo v0.1 (CP 4.8). Due cariche OPPOSTE (nessun bersaglio e' fermo: e' l'altra
	// carica) a distanza dispari (3 celle, la portata dichiarata): senza la protezione dallo scontro frontale,
	// il resolver a microstep le lascerebbe scambiarsi la cella di mezzo (`scambio diretto -> consentito`, la
	// regola di base per il Move) e ciascuna finirebbe adiacente al bersaglio DALL'ALTRO lato, infliggendo
	// comunque danno — le due si sarebbero attraversate senza mai scontrarsi davvero. Qui devono fermarsi
	// l'una davanti all'altra, senza completare l'impatto: nessun danno, nessuna spinta.
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBlastMap(World, /*Radius=*/ 6);

	ARTUnit* A = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0));
	ARTUnit* B = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !A || !B) { DestroyHexBlastWorld(World); return false; }

	URTActionData* ChargeA = NewObject<URTActionData>(A);
	ChargeA->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge"));
	ChargeA->RangeCells = ChargeA->Def.RangeCells;
	A->Abilities.Add(ChargeA);

	URTActionData* ChargeB = NewObject<URTActionData>(B);
	ChargeB->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge"));
	ChargeB->RangeCells = ChargeB->Def.RangeCells;
	B->Abilities.Add(ChargeB);

	const int32 HealthA = A->Health;
	const int32 HealthB = B->Health;
	A->PlannedDashAbility = A->Abilities.Num() - 1;
	A->PlannedDashCell = FRTCellId(3, 0); // dritto verso B, esattamente la portata (3)
	B->PlannedDashAbility = B->Abilities.Num() - 1;
	B->PlannedDashCell = FRTCellId(0, 0); // dritto verso A, esattamente la portata (3)

	RunBlastTurn(TM);

	// Ciascuna avanza di UNA cella (non tre): si fermano l'una davanti all'altra, adiacenti, senza scambiarsi
	// i lati e senza mai completare l'impatto.
	TestTrue(TEXT("A si ferma a meta' strada, non attraversa B"), A->Cell == FRTCellId(1, 0));
	TestTrue(TEXT("B si ferma a meta' strada, non attraversa A"), B->Cell == FRTCellId(2, 0));
	TestEqual(TEXT("nessun danno a A: lo scontro frontale non e' un impatto riuscito"), A->Health, HealthA);
	TestEqual(TEXT("nessun danno a B: lo scontro frontale non e' un impatto riuscito"), B->Health, HealthB);

	DestroyHexBlastWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionsAoEFriendlyFireInMatchTest,
	"RefactorTactics.Actions.AoE.FriendlyFireInMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionsAoEFriendlyFireInMatchTest::RunTest(const FString&)
{
	// `Action.CircularAoE` dichiara `bFriendlyFire = true` (`RTCatalogLibrary.cpp:361`) e il resolver puro lo
	// rispetta (`Actions.AoE.FriendlyFire`) — ma finora `ARTTurnManager` non copiava mai il flag nell'intento,
	// che nasce con `bFriendlyFire = false`: in partita nessuna area ha mai colpito un alleato.
	//
	// Il test e' d'INTEGRAZIONE per costruzione: la versione pura passava gia' e non poteva vedere il difetto.
	// La specifica delle Delayed Actions (§3.3) elenca la "friendly fire policy" fra i campi che ogni azione
	// deve dichiarare in planning: se il dato c'e' ma il resolver non lo legge, la policy non esiste.
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBlastMap(World, /*Radius=*/ 6);

	ARTUnit* Thrower = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(3, 0));
	ARTUnit* Ally = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, 0)); // adiacente al centro
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Thrower || !Foe || !Ally) { DestroyHexBlastWorld(World); return false; }

	URTActionData* Area = NewObject<URTActionData>(Thrower);
	Area->Def = URTCatalogLibrary::FindCoreAction(FName(TEXT("Action.CircularAoE")));
	Area->RangeCells = Area->Def.RangeCells;
	Area->Shape = ERTAbilityShape::Area;
	Area->AreaRadius = 1;
	Area->Power = 0;
	for (const FRTActionEffectSpec& Spec : Area->Def.Effects)
	{
		if (Spec.Effect == ERTActionEffect::Damage) { Area->Power = Spec.Amount; break; }
	}
	const int32 AreaIdx = Thrower->Abilities.Add(Area);

	Foe->Shield = 0;
	Ally->Shield = 0;
	const int32 FoeStart = Foe->Health;
	const int32 AllyStart = Ally->Health;

	Thrower->PlannedAbilityIndex = AreaIdx;
	Thrower->PlannedAttackTarget = Foe;

	RunBlastTurn(TM);

	if (!TestEqual(TEXT("premessa: il nemico al centro incassa l'area"), FoeStart - Foe->Health, Area->Power))
	{
		DestroyHexBlastWorld(World);
		return false;
	}
	TestEqual(TEXT("l'alleato adiacente incassa il fuoco amico dichiarato dal catalogo"),
		AllyStart - Ally->Health, Area->Power);

	DestroyHexBlastWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionsMarkTargetReachesTargetTest,
	"RefactorTactics.Actions.MarkTarget.ReachesTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionsMarkTargetReachesTargetTest::RunTest(const FString&)
{
	// `Action.MarkTarget` dichiara `Range 0` = «portata del portatore» (stessa convenzione di
	// `Action.PrecisionAttack`, «range dell'arma +1»). Il ponte che traduce quello 0 copriva pero' le sole
	// azioni senza `ActionId`: l'istanza si validava a portata 0 e degradava sempre al fallback `Cancel`,
	// quindi il marchio non raggiungeva mai un bersaglio distante. Corretto al CP 8.2.
	//
	// L'osservabile non e' il tag a fine turno (durata 1: scade nel Cleanup dello stesso turno) ma il suo
	// EFFETTO: il +6 sul colpo dell'alleato, che arriva solo se il marchio e' stato davvero applicato.
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBlastMap(World, /*Radius=*/ 6);

	ARTUnit* Marker = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	ARTUnit* Ally = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 1));
	ARTUnit* Foe = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(3, 0)); // a 3 celle dal marcatore
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Marker || !Ally || !Foe) { DestroyHexBlastWorld(World); return false; }

	// Azioni dal catalogo, senza toccare le portate: e' esattamente il caso che prima non funzionava.
	URTActionData* Mark = NewObject<URTActionData>(Marker);
	Mark->Def = URTCatalogLibrary::FindCoreAction(FName(TEXT("Action.MarkTarget")));
	// `MarkTarget` non fa danno: il catalogo non dichiara alcun effetto `Damage`. Il campo legacy `Power`
	// dell'asset (default 30) e' il ripiego per le abilita' NON catalogate e va azzerato qui, altrimenti il
	// marcatore infliggerebbe 30 danni che il catalogo non prevede.
	Mark->Power = 0;
	const int32 MarkIdx = Marker->Abilities.Add(Mark);

	URTActionData* Shot = NewObject<URTActionData>(Ally);
	Shot->Def = URTCatalogLibrary::FindCoreAction(FName(TEXT("Action.PrecisionAttack")));
	Shot->Power = 0;
	for (const FRTActionEffectSpec& Spec : Shot->Def.Effects)
	{
		if (Spec.Effect == ERTActionEffect::Damage) { Shot->Power = Spec.Amount; break; }
	}
	const int32 ShotIdx = Ally->Abilities.Add(Shot);

	TestEqual(TEXT("il catalogo dichiara 0 = portata del portatore"), Mark->Def.RangeCells, 0);

	Foe->Shield = 0;
	const int32 FoeStart = Foe->Health;

	Marker->PlannedAbilityIndex = MarkIdx;
	Marker->PlannedAttackTarget = Foe;
	Ally->PlannedAbilityIndex = ShotIdx;
	Ally->PlannedAttackTarget = Foe;

	RunBlastTurn(TM);

	// 24 del colpo + 6 del marchio: il marchio e' arrivato a 3 celle, e il pass a priorita' lo ha speso.
	TestEqual(TEXT("il marchio ha raggiunto il bersaglio distante e ha dato il suo +6"),
		FoeStart - Foe->Health, Shot->Power + URTCombatLibrary::MarkedFirstHitBonus);

	DestroyHexBlastWorld(World);
	return true;
}

/**
 * Il pezzo che i test puri non possono dare: che in PARTITA il danno alle strutture venga davvero raccolto,
 * applicato e SCRITTO nel TurnLog. Senza, `ApplyStructureDamage` potrebbe essere corretta e non venire
 * chiamata da nessuno - il difetto ricorrente di questi checkpoint.
 *
 * Scena: muro alto sul bordo fra l'attaccante e il bersaglio. Il colpo non arriva a nessuno (la barriera
 * toglie la linea di tiro), ma la barriera incassa: e' l'unico modo che l'attaccante ha di aprirsi la strada.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCoverDestructionLoggedTest,
	"RefactorTactics.Cover.Destruction.LoggedInPlayedTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCoverDestructionLoggedTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnHexBlastMap(World, /*Radius=*/ 4);

	// Muro alto sul bordo W di (1,0): separa (0,0) da (1,0), e con esse l'attaccante dal bersaglio.
	const FRTCellId Walled(1, 0);
	FRTHexCellData WithWall = *MapActor->MapAsset->FindCell(Walled);
	WithWall.Covers.Add(FRTHexCover(ERTHexDirection::W, ERTHexCoverType::High,
		FRTHexCover::DefaultIntegrity(ERTHexCoverType::High)));
	MapActor->MapAsset->AddOrUpdateCell(WithWall);
	MapActor->MapAsset->SortCells();

	ARTUnit* Breacher = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Breacher || !Foe) { DestroyHexBlastWorld(World); return false; }

	// L'abilita' dichiara di poter sfondare: e' il catalogo a concederlo (qui lo si simula sull'istanza,
	// perche' l'archetipo di prova non ha `HeavyAttack` fra le sue).
	Breacher->Abilities[0]->Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::DamageStructure, 20));
	Breacher->PlannedAbilityIndex = 0;
	Breacher->PlannedAttackTarget = Foe;

	const int32 HealthBefore = Foe->Health;
	RunBlastTurn(TM);

	TestEqual(TEXT("il muro ha fermato il colpo: il bersaglio e' intatto"), Foe->Health, HealthBefore);

	// Il dato e' stato scalato sulla COPIA di lavoro della mappa, quella su cui gira la partita.
	const FRTHexCellData* After = MapActor->MapAsset->FindCell(Walled);
	TestTrue(TEXT("la copertura e' ancora in piedi, danneggiata"),
		After && After->Covers.Num() == 1 && After->Covers[0].Integrity == 30);

	// E il TurnLog lo dice, col bordo scritto come coppia di celle.
	int32 Logged = 0;
	for (const FRTTurnLogEntry& Entry : TM->GetTurnLog())
	{
		if (Entry.Category == ERTLogCategory::Environment
			&& Entry.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::CoverDamaged))
		{
			++Logged;
			TestTrue(TEXT("la voce indica il bordo colpito"),
				Entry.SrcCell == Walled && Entry.TgtCell == FRTCellId(0, 0));
			TestEqual(TEXT("e l'integrita' residua"), Entry.Amount, 30);
		}
	}
	TestEqual(TEXT("una voce di copertura danneggiata"), Logged, 1);

	DestroyHexBlastWorld(World);
	return true;
}

/**
 * Il difetto che CP 9.3 esiste per impedire, e che nessun test puro sul pathfinding puo' trovare: il percorso
 * del Move e' stato validato PRIMA, al momento del click, e `ResolveMovement` lo esegue com'e'. Se una porta si
 * chiude nel Blast — cioe' a meta' turno — un percorso che la attraversava produrrebbe un passo fantasma.
 *
 * Scena: porta aperta sul bordo (1,0)<->(2,0). Il Mover ha gia' pianificato (0,0) -> (1,0) -> (2,0). Il Closer,
 * dall'altro lato, spara verso di lui: la sua azione dichiara `SetDoorState` e la prima porta sulla linea di
 * tiro si chiude a fase conclusa. Il Move che segue deve fermarsi PRIMA del varco (`Fallback.Stop`), non
 * attraversarlo e non annullarsi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexDoorClosingStopsMovementTest,
	"RefactorTactics.Structures.Door.ClosingStopsMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexDoorClosingStopsMovementTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnHexBlastMap(World, /*Radius=*/ 4);

	// Porta APERTA sul bordo E di (1,0): separa (1,0) da (2,0) quando sara' chiusa.
	const FRTCellId Hinge(1, 0);
	FRTHexCellData WithDoor = *MapActor->MapAsset->FindCell(Hinge);
	WithDoor.Doors.Add(FRTHexDoor(ERTHexDirection::E, ERTHexDoorState::Open));
	MapActor->MapAsset->AddOrUpdateCell(WithDoor);
	MapActor->MapAsset->SortCells();

	ARTUnit* Mover = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0));
	ARTUnit* Closer = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(3, 0));
	if (Closer) { Closer->Facing = ERTHexDirection::W; } // vedi CP 13.2: guarda chi vuole bloccare
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Closer) { DestroyHexBlastWorld(World); return false; }

	// Il percorso e' gia' validato: con la porta aperta lo era davvero, ed e' esattamente il punto.
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) };
	TestTrue(TEXT("con la porta aperta il passo esisteva"),
		!URTHexCoverLibrary::BlocksTraversal(MapActor->MapAsset, FRTCellId(1, 0), FRTCellId(2, 0)));

	// L'azione dichiara di chiudere la prima porta sulla propria linea di tiro (qui la si simula
	// sull'istanza: l'azione di catalogo che apre e chiude porte e' CP 10.1).
	Closer->Abilities[0]->Def.Effects.Add(
		FRTActionEffectSpec(ERTActionEffect::SetDoorState,
			static_cast<int32>(ERTHexDoorState::Closed)));
	Closer->PlannedAbilityIndex = 0;
	Closer->PlannedAttackTarget = Mover;

	RunBlastTurn(TM);

	// 1. La porta si e' chiusa sulla copia di lavoro della mappa (non sull'asset su disco).
	TestTrue(TEXT("la porta e' chiusa"),
		URTHexCoverLibrary::BlocksTraversal(MapActor->MapAsset, FRTCellId(1, 0), FRTCellId(2, 0)));

	// 2. Il movimento si e' FERMATO prima del varco, non annullato: l'unita' e' avanzata di una cella.
	TestTrue(TEXT("l'unita' si e' fermata davanti alla porta"), Mover->Cell == FRTCellId(1, 0));

	// 3. Il TurnLog dice entrambe le cose, con il bordo scritto come coppia di celle.
	int32 DoorEntries = 0;
	int32 StoppedEntries = 0;
	for (const FRTTurnLogEntry& Entry : TM->GetTurnLog())
	{
		if (Entry.Category == ERTLogCategory::Environment
			&& Entry.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::DoorClosed))
		{
			++DoorEntries;
			TestTrue(TEXT("la voce indica il bordo della porta"),
				(Entry.SrcCell == FRTCellId(1, 0) && Entry.TgtCell == FRTCellId(2, 0))
				|| (Entry.SrcCell == FRTCellId(2, 0) && Entry.TgtCell == FRTCellId(1, 0)));
		}
		if (Entry.Category == ERTLogCategory::Move
			&& Entry.Outcome == static_cast<uint8>(ERTMoveOutcome::BlockedByTopology))
		{
			++StoppedEntries;
			TestTrue(TEXT("il reason code riporta partenza e arrivo veri"),
				Entry.SrcCell == FRTCellId(0, 0) && Entry.TgtCell == FRTCellId(1, 0));
		}
	}
	TestEqual(TEXT("una voce di porta chiusa"), DoorEntries, 1);
	TestEqual(TEXT("una voce di movimento fermato dalla topologia"), StoppedEntries, 1);

	DestroyHexBlastWorld(World);
	return true;
}

// Chi aggiunge un test in fondo a questo file lo aggiunge PRIMA di questa riga: e' il difetto di #923,
// invisibile in Editor dove la guardia vale 1. Il controllo che lo dimostra e'
// `Build.bat RefactorTactics Win64 Shipping`, non la suite.
#endif // WITH_DEV_AUTOMATION_TESTS
