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
#include "Tests/RTAbilityFixtures.h"
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
		// [D-224] Lo scudo base sta a 0 in questo file: qui si misura una RIDUZIONE di danno, e sommarci
		// una costante di bilanciamento renderebbe l'asserto illeggibile ("15" diventerebbe "15 piu' 5") e
		// legherebbe questi test al valore del base. Chi vuole lo scudo se lo da' esplicitamente.
		U->Shield = 0;
		return U;
	}

	/** L'azione cercata per `ActionId`, mai per indice: un letterale direbbe «la nona voce apre le porte». */
	int32 FindAbilityByActionId(const ARTUnit* Unit, const TCHAR* ActionId)
	{
		if (!Unit) { return INDEX_NONE; }
		for (int32 i = 0; i < Unit->Abilities.Num(); ++i)
		{
			if (Unit->Abilities[i] && Unit->Abilities[i]->Def.ActionId == FName(ActionId))
			{
				return i;
			}
		}
		return INDEX_NONE;
	}

	/** Una porta CHIUSA su un bordo di `Cell`. Le tre chiamate di seguito sono un rito, non una scelta. */
	void AddClosedDoor(ARTHexMapActor* MapActor, const FRTCellId& Cell, ERTHexDirection Edge)
	{
		FRTHexCellData Data = *MapActor->MapAsset->FindCell(Cell);
		Data.Doors.Add(FRTHexDoor(Edge, ERTHexDoorState::Closed));
		MapActor->MapAsset->AddOrUpdateCell(Data);
		MapActor->MapAsset->SortCells();
	}

	/**
	 * Il piano di un'azione a BORDO **come lo costruisce il puntatore**, e non come farebbe comodo:
	 * `ARTPlayerController::HandleTargetEdge(Cell, Edge)` scrive `PlannedAttackCell = Cell` e
	 * `PlannedCoverEdge = Edge`, cioe' **il bordo appartiene alla cella cliccata**.
	 *
	 * 🔴 Esiste perche' i primi due test di CP 10.1 costruivano il piano a mano con l'ancoraggio
	 * sbagliato — il bordo riferito alla cella dell'ATTACCANTE — e verificavano quindi la stessa assunzione
	 * errata dell'implementazione, restando verdi su un difetto che in partita avrebbe rifiutato ogni
	 * apertura di porta. Una fixture che replica l'assunzione invece del contratto non prova niente.
	 */
	void PlanEdgeAction(ARTUnit* Unit, int32 AbilityIndex, const FRTCellId& TargetCell, ERTHexDirection Edge)
	{
		Unit->PlannedAbilityIndex = AbilityIndex;
		Unit->PlannedAttackTarget = nullptr;
		Unit->PlannedAttackCell = TargetCell;
		Unit->bAttackTargetsCell = true;
		Unit->PlannedCoverEdge = Edge;
		Unit->bHasPlannedCoverEdge = true;
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
	// ⚠️ [D-224] L'assorbimento NON si legge piu' dallo scudo a fine turno: `RechargeBaseShield` gira nel
	// Cleanup, quindi ogni unita' viva chiude il turno con lo scudo base pieno qualunque cosa abbia incassato.
	// Cio' che resta osservabile — ed e' la proprieta' in esame — sono gli HP.
	TestEqual(TEXT("gli HP dicono quanto ha assorbito lo scudo"),
		Foe->Health, HealthBefore - FMath::Max(0, FullHit - ShieldBefore));
	TestEqual(TEXT("e a fine turno lo scudo base e' tornato pieno"),
		Foe->Shield, URTCombatLibrary::BaseShield);
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
	// [D-224] Non si confronta con `ShieldBefore`: la ricarica del Cleanup renderebbe verde questo asserto
	// anche se lo scudo fosse stato consumato e poi ripristinato. Il colpo che non parte si dimostra con gli
	// HP intatti qui sopra e con la voce `NoLineOfSight` qui sotto.
	TestEqual(TEXT("a fine turno lo scudo base e' pieno"), Foe->Shield, URTCombatLibrary::BaseShield);
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
	// Letto ADESSO: la risoluzione azzera il piano, e dopo il turno non resta niente da cui ricavarlo.
	const FName AzionePianificata = Attacker->Abilities.IsValidIndex(0) && Attacker->Abilities[0]
		? Attacker->Abilities[0]->Def.ActionId : FName();

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
	int32 ConIdentitaGiusta = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category != ERTLogCategory::Fallback) { continue; }
		++Fallbacks;
		if (E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::OutOfRange))
		{
			++OutOfRangeReasons;
		}
		// ⚠️ L'azione ATTESA, non «una qualsiasi»: `Instance` viene riassegnata all'istanza del FALLBACK
		// subito dopo che la voce e' stata scritta, quindi leggere l'identita' una riga piu' in basso
		// nominerebbe il ripiego invece dell'azione fallita — e un test che chiedesse solo «non e' vuoto»
		// resterebbe verde.
		if (E.ActionId == AzionePianificata)
		{
			++ConIdentitaGiusta;
		}
	}
	TestEqual(TEXT("il TurnLog registra un fallback"), Fallbacks, 1);
	TestEqual(TEXT("annullata perche' fuori portata: l'esito dice anche il motivo"), OutOfRangeReasons, 1);
	// QUALE azione e' fallita ([D-196], `#1412` punto 1b). Senza, un'azione che non avviene lascia una voce
	// che non dice se a mancare sia stata l'ultimate o l'attacco base — e due annullamenti della stessa
	// unita' nello stesso turno erano indistinguibili.
	TestEqual(*FString::Printf(TEXT("e la voce nomina l'azione fallita (%s)"), *AzionePianificata.ToString()),
		ConIdentitaGiusta, 1);

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
		Action->Def.bCountsAsAttack = true; // aggressione: da [`INT-8`] va dichiarata
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
	const int32 GuardIdx = RTAbilityFixtures::AddCoreAbility(Defender, TEXT("Action.Guard"));
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

	const int32 GuardIdx = RTAbilityFixtures::AddCoreAbility(Defender, TEXT("Action.Guard"));
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
	Defender->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbility(Defender, TEXT("Action.Guard"));
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

	RTAbilityFixtures::AddCoreAbility(Charger, TEXT("Action.Charge"));

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

/**
 * **Un `Action.Interrupt` non ANNULLA l'impatto di una carica: da [D-300] lo DEGRADA.**
 *
 * Il movimento della carica avviene nella fase Dash; l'impatto entra nel Blast come intento a portata 1.
 * Cancellarlo li' annullerebbe **a posteriori** la coda di un'azione risolta a meta': l'unita' resterebbe
 * dove la carica l'ha portata e perderebbe il colpo. Quell'obiezione regge ancora, ed e' la ragione per cui
 * l'impatto e' rimasto immune fino al 2026-08-31.
 *
 * 🔴 **Cosa e' cambiato, e perche' il nome del test resta vero.** `Action.Charge` dichiara
 * `ERTInterruptPolicy::SuppressSecondary` (`#1955`): l'Interrupt adesso RAGGIUNGE l'impatto, ma invece di
 * cancellarlo gli toglie gli effetti oltre il primo. L'impatto **sopravvive** — il danno arriva — e cade
 * la **spinta**. Il test asseriva entrambe; ora asserisce il danno e il contrario della spinta.
 *
 * ⚠️ Prima di `#1437` l'impatto sopravviveva per una ragione **accidentale**: il ciclo si fermava al primo
 * intento della vittima. E' la storia che spiega perche' questo caso va asserito e non dato per scontato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTChargeImpactSurvivesInterruptTest,
	"RefactorTactics.Actions.Charge.ImpactSurvivesInterrupt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTChargeImpactSurvivesInterruptTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBlastMap(World, /*Radius=*/ 6);

	ARTUnit* Charger = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0));
	ARTUnit* Victim = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, 0));
	// L'interruttore parte adiacente a dove la carica FERMA il caricatore — (1,0) — perche'
	// `Action.Interrupt` ha portata 1 e il colpo si valida sulle posizioni del Blast, dopo il Dash.
	ARTUnit* Interrupter = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(1, -1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Charger || !Victim || !Interrupter) { DestroyHexBlastWorld(World); return false; }

	RTAbilityFixtures::AddCoreAbility(Charger, TEXT("Action.Charge"));

	// ⚠️ Il `Power` lo azzera la fixture derivandolo dal catalogo. Prima questo sito copiava la sola
	// portata, quindi `Power` restava al default legacy **30**: `Action.Interrupt` non dichiara `Damage`,
	// e il resolver ricade sullo specchio. L'interruttore infliggeva trenta danni al caricatore che
	// nessuna riga di catalogo autorizza — invisibile qui perche' il test guarda la salute della VITTIMA,
	// non quella del caricatore (#1588).
	RTAbilityFixtures::AddCoreAbility(Interrupter, TEXT("Action.Interrupt"));

	const int32 VictimHealth = Victim->Health;
	Charger->PlannedDashAbility = Charger->Abilities.Num() - 1;
	Charger->PlannedDashCell = FRTCellId(3, 0); // dritto: incontra il bersaglio a (2,0) per strada

	Interrupter->PlannedAbilityIndex = Interrupter->Abilities.Num() - 1;
	Interrupter->PlannedAttackTarget = Charger;

	RunBlastTurn(TM);

	TestTrue(TEXT("premessa: la carica si e' mossa e si e' fermata davanti al bersaglio"),
		Charger->Cell == FRTCellId(1, 0));
	TestEqual(TEXT("l'impatto fa danno lo stesso: l'Interrupt non annulla una carica gia' risolta"),
		VictimHealth - Victim->Health, 20);

	// 🔴 **La meta' che [D-300] rovescia**: la vittima NON viene spinta. Senza interruzione finirebbe a
	// (3,0), e a dirlo e' il test gemello `Charge.ImpactResolvesInBlast` — che asserisce la spinta con la
	// stessa fixture e resta la controprova di questa riga.
	TestTrue(TEXT("ma non viene spinta: SuppressSecondary toglie il secondo effetto"),
		Victim->Cell == FRTCellId(2, 0));

	// E nessuna voce dice che l'abbia annullata: degradare non e' cancellare, e il TurnLog deve saperlo
	// distinguere. Una voce `Cancelled` qui direbbe che l'azione non e' avvenuta, ed e' falso.
	const bool bAnnullata = TM->GetTurnLog().ContainsByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::Fallback
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Interrupted);
	});
	TestFalse(TEXT("e il TurnLog non registra un'interruzione: e' degradata, non annullata"),
		bAnnullata);

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

	RTAbilityFixtures::AddCoreAbility(A, TEXT("Action.Charge"));

	RTAbilityFixtures::AddCoreAbility(B, TEXT("Action.Charge"));

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

/**
 * `Action.Interact` PORTATA DAL KIT apre una porta: la generica di D-025 arriva fino al dato di mappa.
 *
 * E' il test che mancava quando `Interact` e' entrata fra le generiche (2026-08-26). Il ramo delle porte era
 * gia' coperto — `Structures.Door.ClosingStopsMovement`, qui sopra — ma con l'effetto **simulato**
 * sull'istanza (`Abilities[0]->Def.Effects.Add(...)`), e il suo commento lo dichiara: *«l'azione di catalogo
 * che apre e chiude porte e' CP 10.1»*. Da [D-148]/[D-151] quell'azione di catalogo esiste, e nessun test
 * verificava la catena a partire da cio' che un'unita' porta davvero.
 *
 * Cosa cade se la catena si rompe, ed e' il motivo per cui il test tocca quattro anelli e non uno:
 *   kit          `MakeGenericActions` smette di accodare `Interact`  -> l'azione non si trova nel kit
 *   traduzione   `ARTTurnManager` non alza piu' `bChangesDoor`       -> nessuna op raccolta
 *   raccolta     `URTHexCombatLibrary` perde il ramo delle porte     -> nessuna op raccolta
 *   applicazione `URTHexDoorLibrary::SetDoorState` non apre          -> il passaggio resta bloccato
 *
 * ⚠️ L'azione si cerca per `ActionId`, mai per indice: un letterale direbbe *«la nona voce apre le porte»*
 * invece di *«l'unita' porta Interact»*, e resterebbe verde se qualcuno la togliesse dalle generiche
 * lasciando nove voci nel kit.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexInteractFromKitOpensDoorTest,
	"RefactorTactics.Structures.Door.InteractFromKitOpensDoor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexInteractFromKitOpensDoorTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnHexBlastMap(World, /*Radius=*/ 4);

	// Porta CHIUSA sul bordo E di (0,0): separa (0,0) da (1,0), che sono adiacenti — la portata di `Interact`
	// e' 1 (D-149), quindi la cella oltre la porta e' l'unico bersaglio che l'azione puo' avere.
	const FRTCellId Hinge(0, 0);
	const FRTCellId Beyond(1, 0);
	FRTHexCellData WithDoor = *MapActor->MapAsset->FindCell(Hinge);
	WithDoor.Doors.Add(FRTHexDoor(ERTHexDirection::E, ERTHexDoorState::Closed));
	MapActor->MapAsset->AddOrUpdateCell(WithDoor);
	MapActor->MapAsset->SortCells();

	ARTUnit* Opener = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeGadget(), Hinge);
	ARTUnit* Foe = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Opener || !Foe) { DestroyHexBlastWorld(World); return false; }

	TestTrue(TEXT("all'inizio la porta blocca il passaggio"),
		URTHexCoverLibrary::BlocksTraversal(MapActor->MapAsset, Hinge, Beyond));

	int32 InteractIdx = INDEX_NONE;
	for (int32 i = 0; i < Opener->Abilities.Num(); ++i)
	{
		if (Opener->Abilities[i] && Opener->Abilities[i]->Def.ActionId == FName(TEXT("Action.Interact")))
		{
			InteractIdx = i;
			break;
		}
	}
	if (!TestTrue(TEXT("`Action.Interact` e' nel kit: e' una generica (D-025)"), InteractIdx != INDEX_NONE))
	{
		DestroyHexBlastWorld(World);
		return false; // senza l'azione nel kit il resto del test misurerebbe un'altra cosa
	}

	Opener->PlannedAbilityIndex = InteractIdx;
	Opener->PlannedAttackTarget = nullptr;
	Opener->PlannedAttackCell = Beyond;
	Opener->bAttackTargetsCell = true; // si mira a una CELLA: dietro la porta non c'e' nessuno da bersagliare

	RunBlastTurn(TM);

	TestFalse(TEXT("la porta e' aperta: il passaggio non e' piu' bloccato"),
		URTHexCoverLibrary::BlocksTraversal(MapActor->MapAsset, Hinge, Beyond));

	// Il TurnLog lo dice, col bordo scritto come coppia di celle: senza questa meta' il test non
	// distinguerebbe «la porta si e' aperta» da «la porta non c'era».
	int32 DoorOpenedEntries = 0;
	for (const FRTTurnLogEntry& Entry : TM->GetTurnLog())
	{
		if (Entry.Category == ERTLogCategory::Environment
			&& Entry.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::DoorOpened))
		{
			++DoorOpenedEntries;
			TestTrue(TEXT("la voce indica il bordo della porta"),
				(Entry.SrcCell == Hinge && Entry.TgtCell == Beyond)
				|| (Entry.SrcCell == Beyond && Entry.TgtCell == Hinge));
		}
	}
	TestEqual(TEXT("una voce di porta aperta"), DoorOpenedEntries, 1);

	DestroyHexBlastWorld(World);
	return true;
}

/**
 * Un `Interact` dichiarato su un bordo **senza porta** viene RIFIUTATO, e il rifiuto finisce in traccia
 * (CP 10.1, `#74`).
 *
 * ⛔ **Prima non succedeva niente, in silenzio.** E' il difetto che [D-149] registra dopo il proprio merge:
 * *«la decisione resta valida — la portata e' giusta — la sua giustificazione tecnica no, e il divario fra
 * bordo cliccato e bordo agito e' un difetto aperto»*. `FirstDoorEdge` cammina la traiettoria e guarda il
 * solo bordo condiviso con la cella bersaglio; se il giocatore ne clicca un altro dei sei, l'operazione o
 * non trova nulla — e il turno passa senza una voce — o trova **un'altra porta** e apre quella.
 *
 * 🔴 **Il piano si costruisce con `PlanEdgeAction`, che replica il puntatore.** La prima stesura lo scriveva
 * a mano ancorando il bordo alla cella dell'ATTACCANTE, cioe' con la stessa assunzione sbagliata
 * dell'implementazione: verde su un difetto che in partita avrebbe rifiutato ogni apertura di porta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexInteractDoorlessEdgeRefusedTest,
	"RefactorTactics.Structures.Door.InteractOnDoorlessEdgeIsRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexInteractDoorlessEdgeRefusedTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnHexBlastMap(World, /*Radius=*/ 4);

	// L'unita' sta in (0,0). La porta e' sul bordo W della cella (1,0), cioe' fra (1,0) e (0,0): e' quella
	// che il giocatore potrebbe aprire. Ne dichiarera' un'altra della stessa cella — il bordo E, che da'
	// su (2,0) e non ha nessuna porta.
	const FRTCellId Standing(0, 0);
	const FRTCellId Target(1, 0);
	AddClosedDoor(MapActor, Target, ERTHexDirection::W);

	ARTUnit* Opener = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeGadget(), Standing);
	ARTUnit* Foe = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Opener || !Foe) { DestroyHexBlastWorld(World); return false; }

	const int32 InteractIdx = FindAbilityByActionId(Opener, TEXT("Action.Interact"));
	if (!TestTrue(TEXT("`Action.Interact` e' nel kit: e' una generica (D-025)"), InteractIdx != INDEX_NONE))
	{
		DestroyHexBlastWorld(World);
		return false;
	}

	TestTrue(TEXT("premessa: la porta cliccabile blocca il passaggio"),
		URTHexCoverLibrary::BlocksTraversal(MapActor->MapAsset, Standing, Target));

	// Il bordo E di (1,0): un'altra delle sei facce, e li' non c'e' niente.
	PlanEdgeAction(Opener, InteractIdx, Target, ERTHexDirection::E);

	RunBlastTurn(TM);

	// 🔴 La meta' che conta: la porta su W **non si apre**. Prima si apriva — la traiettoria la trovava, e
	// il bordo che il giocatore aveva scelto non arrivava fin li'.
	TestTrue(TEXT("la porta su un bordo che nessuno ha dichiarato resta chiusa"),
		URTHexCoverLibrary::BlocksTraversal(MapActor->MapAsset, Standing, Target));

	int32 DoorOpened = 0;
	int32 RefusedNoEffect = 0;
	for (const FRTTurnLogEntry& Entry : TM->GetTurnLog())
	{
		if (Entry.Category == ERTLogCategory::Environment
			&& Entry.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::DoorOpened))
		{
			++DoorOpened;
		}
		if (Entry.Category == ERTLogCategory::Fallback
			&& Entry.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
			&& Entry.Amount == static_cast<int32>(ERTActionInvalidReason::NoEffect)
			&& Entry.ActionId == FName(TEXT("Action.Interact")))
		{
			++RefusedNoEffect;
			// La coppia di celle E' il bordo rifiutato: senza, due rifiuti su bordi diversi della stessa
			// cella sarebbero righe identiche ([D-196]).
			TestTrue(TEXT("la voce nomina il bordo DICHIARATO, non la cella mirata"),
				Entry.SrcCell == Target && Entry.TgtCell == URTHexLibrary::Neighbor(Target, ERTHexDirection::E));
		}
	}

	TestEqual(TEXT("nessuna porta aperta"), DoorOpened, 0);
	// E il rifiuto e' REGISTRATO: senza questa riga il test non distinguerebbe «rifiutata» da «sparita».
	TestEqual(TEXT("una voce di rifiuto con reason code NoEffect"), RefusedNoEffect, 1);

	DestroyHexBlastWorld(World);
	return true;
}

/**
 * Con due porte sulla stessa cella, `Interact` agisce su **quella dichiarata** e non su quella che la
 * traiettoria incontra per prima (CP 10.1, `#74`).
 *
 * 🔴 **E' il difetto peggiore dei due**, e non produceva silenzio ma un esito sbagliato: la traiettoria
 * verso la cella bersaglio trova la porta sul bordo condiviso e apriva quella — un'altra porta, in un'altra
 * direzione, senza che niente lo dicesse. Un test che guardasse solo «una porta si e' aperta» sarebbe
 * rimasto verde su entrambe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexInteractUsesDeclaredEdgeTest,
	"RefactorTactics.Structures.Door.InteractActsOnTheDeclaredEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexInteractUsesDeclaredEdgeTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnHexBlastMap(World, /*Radius=*/ 4);

	// DUE porte chiuse sulla cella bersaglio: una sul bordo W — quello condiviso con chi agisce, cioe'
	// quello che la traiettoria incontra — e una sul bordo E, dall'altra parte. E' la configurazione che
	// distingue «apre la porta» da «apre LA porta giusta».
	const FRTCellId Standing(0, 0);
	const FRTCellId Target(1, 0);
	const FRTCellId BeyondEast = URTHexLibrary::Neighbor(Target, ERTHexDirection::E);
	AddClosedDoor(MapActor, Target, ERTHexDirection::W);
	AddClosedDoor(MapActor, Target, ERTHexDirection::E);

	ARTUnit* Opener = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeGadget(), Standing);
	ARTUnit* Foe = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Opener || !Foe) { DestroyHexBlastWorld(World); return false; }

	TestTrue(TEXT("premessa: entrambe le porte bloccano"),
		URTHexCoverLibrary::BlocksTraversal(MapActor->MapAsset, Standing, Target)
		&& URTHexCoverLibrary::BlocksTraversal(MapActor->MapAsset, Target, BeyondEast));

	const int32 InteractIdx = FindAbilityByActionId(Opener, TEXT("Action.Interact"));
	if (!TestTrue(TEXT("`Action.Interact` e' nel kit"), InteractIdx != INDEX_NONE))
	{
		DestroyHexBlastWorld(World);
		return false;
	}

	// Si dichiara il bordo E: quello LONTANO, che la traiettoria non incontra.
	PlanEdgeAction(Opener, InteractIdx, Target, ERTHexDirection::E);

	RunBlastTurn(TM);

	TestFalse(TEXT("si apre la porta DICHIARATA (E)"),
		URTHexCoverLibrary::BlocksTraversal(MapActor->MapAsset, Target, BeyondEast));
	// L'altra meta', e senza di lei il test passerebbe anche se si aprissero entrambe.
	TestTrue(TEXT("e quella sulla traiettoria (W) resta chiusa"),
		URTHexCoverLibrary::BlocksTraversal(MapActor->MapAsset, Standing, Target));

	int32 DoorOpened = 0;
	for (const FRTTurnLogEntry& Entry : TM->GetTurnLog())
	{
		if (Entry.Category == ERTLogCategory::Environment
			&& Entry.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::DoorOpened))
		{
			++DoorOpened;
			TestTrue(TEXT("e la voce nomina il bordo E, non quello della traiettoria"),
				(Entry.SrcCell == Target && Entry.TgtCell == BeyondEast)
				|| (Entry.SrcCell == BeyondEast && Entry.TgtCell == Target));
		}
	}
	TestEqual(TEXT("una sola porta aperta"), DoorOpened, 1);

	DestroyHexBlastWorld(World);
	return true;
}

// Chi aggiunge un test in fondo a questo file lo aggiunge PRIMA di questa riga: e' il difetto di #923,
// invisibile in Editor dove la guardia vale 1. Il controllo che lo dimostra e'
// `Build.bat RefactorTactics Win64 Shipping`, non la suite.

/**
 * [D-300] **La separazione fra cancellato e degradato non degrada chi va cancellato.**
 *
 * Controprova indispensabile di `ImpactSurvivesInterrupt`: se il passo (b) di `#1955` mettesse fra i
 * degradati ogni intento interrotto invece dei soli `SuppressSecondary`, questo caso diventerebbe rosso e
 * quello verde — e nessun'altra asserzione se ne accorgerebbe. Un'azione con `InterruptBeforeEffect`
 * interrotta continua a non produrre **NULLA**.
 *
 * Il soggetto e' un attacco pianificato normale (`Action.BasicAttack`, policy di default), interrotto da
 * un avversario adiacente: la vittima designata non deve perdere salute, e il TurnLog **deve** registrare
 * l'interruzione — al contrario del caso degradato, dove quella voce non c'e'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterruptStillCancelsWholeActionTest,
	"RefactorTactics.Actions.Interrupt.StillCancelsAnInterruptBeforeEffectAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterruptStillCancelsWholeActionTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBlastMap(World, /*Radius=*/ 6);

	ARTUnit* Attacker = SpawnHexBlastUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0));
	ARTUnit* Victim = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(1, 0));
	ARTUnit* Interrupter = SpawnHexBlastUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attacker || !Victim || !Interrupter) { DestroyHexBlastWorld(World); return false; }

	// ⚠️ `Action.LineAttack` e non `Action.BasicAttack`: l'attacco base prende la portata dall'ARMA, e la
	// fixture non ne monta una — il colpo cadeva «fuori portata» prima ancora di essere interrotto, e la
	// prima asserzione era verde per la ragione sbagliata (danno zero per assenza, non per interruzione).
	// `LineAttack` dichiara portata 5 nel catalogo, quindi il colpo parte davvero. Trovato dal test stesso,
	// rosso alla prima esecuzione.
	RTAbilityFixtures::AddCoreAbility(Attacker, TEXT("Action.LineAttack"));
	RTAbilityFixtures::AddCoreAbility(Interrupter, TEXT("Action.Interrupt"));

	// La premessa del test, misurata invece che assunta: se un giorno `LineAttack` cambiasse policy, questo
	// test misurerebbe il caso sbagliato restando verde.
	const TArray<FRTActionDef> Catalog = URTCatalogLibrary::GetCoreActionCatalog();
	const FRTActionDef* Linea = Catalog.FindByPredicate([](const FRTActionDef& D)
		{ return D.ActionId == FName(TEXT("Action.LineAttack")); });
	if (!TestNotNull(TEXT("Action.LineAttack e' nel catalogo"), Linea)
		|| !TestEqual(TEXT("premessa: e' InterruptBeforeEffect, cioe' si CANCELLA"), Linea->InterruptPolicy,
			ERTInterruptPolicy::InterruptBeforeEffect))
	{
		DestroyHexBlastWorld(World);
		return false;
	}

	const int32 VictimHealth = Victim->Health;
	Attacker->PlannedAbilityIndex = Attacker->Abilities.Num() - 1;
	Attacker->PlannedAttackTarget = Victim;
	Interrupter->PlannedAbilityIndex = Interrupter->Abilities.Num() - 1;
	Interrupter->PlannedAttackTarget = Attacker;

	RunBlastTurn(TM);

	TestEqual(TEXT("l'azione cancellata non fa danno: tutto o niente"), Victim->Health, VictimHealth);

	// 🔴 **E il TurnLog e' cio' che distingue «cancellata» da «mai partita»**: senza questa riga un
	// colpo fuori portata darebbe lo stesso zero danni e il test sarebbe verde per la ragione sbagliata.
	// E' esattamente com'era prima del fix della fixture qui sopra.

	// E qui la voce `Cancelled` CI DEVE essere: e' cio' che distingue un'azione annullata da una degradata.
	const bool bAnnullata = TM->GetTurnLog().ContainsByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::Fallback
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Interrupted);
	});
	TestTrue(TEXT("e il TurnLog registra l'interruzione, al contrario del caso degradato"), bAnnullata);

	DestroyHexBlastWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
