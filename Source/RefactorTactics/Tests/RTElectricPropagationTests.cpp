#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 8.3 — propagazione elettrica deterministica: la combo firma del gioco (acqua + elettricita').
 *
 * Il catalogo terreni §2 fissa la regola: l'elettricita' attraversa celle CONDUTTIVE adiacenti fino al limite
 * dichiarato dall'azione (`Action.Electrify`: 3 passi, 20 iniziali, 12 propagati), ogni unita' colpita una
 * sola volta, ordine `distanza dalla sorgente -> CellId -> UnitId`.
 *
 * Prefissi `Prop*` negli helper: unity build, namespace anonimi fusi con gli altri file di test.
 */
namespace
{
	/** Mappa di celle `Floor` con le superfici indicate sovrascritte: l'acqua si dipinge dove serve. */
	URTHexMapAsset* MakePropMap(int32 Radius, const TMap<FRTCellId, ERTHexSurface>& Surfaces)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			FRTHexCellData Cell(Id);
			if (const ERTHexSurface* Surface = Surfaces.Find(Id))
			{
				Cell.Surface = *Surface;
				Cell.MoveCost = URTTerrainLibrary::FindTerrainDef(*Surface).MoveCost;
			}
			M->AddOrUpdateCell(Cell);
		}
		M->SortCells();
		return M;
	}

	FRTHexCombatUnit MakePropUnit(int32 UnitId, int32 TeamId, const FRTCellId& Cell, bool bAlive = true)
	{
		FRTHexCombatUnit U;
		U.UnitId = UnitId;
		U.TeamId = TeamId;
		U.Cell = Cell;
		U.bAlive = bAlive;
		return U;
	}

	/** Gli UnitId colpiti, nell'ordine restituito: l'ordine E' parte del risultato (determinismo). */
	TArray<int32> PropHitIds(const TArray<FRTPropagationHit>& Hits)
	{
		TArray<int32> Ids;
		for (const FRTPropagationHit& Hit : Hits) { Ids.Add(Hit.UnitId); }
		return Ids;
	}

	const FRTPropagationHit* FindPropHit(const TArray<FRTPropagationHit>& Hits, int32 UnitId)
	{
		for (const FRTPropagationHit& Hit : Hits)
		{
			if (Hit.UnitId == UnitId) { return &Hit; }
		}
		return nullptr;
	}

	// Numeri del catalogo, non del test: se cambiano li', questi test cambiano con loro.
	int32 PropInitialDamage() { return URTCatalogLibrary::FirstDamage(URTCatalogLibrary::FindCoreAction(TEXT("Action.Electrify"))); }
	int32 PropLimit() { return URTCatalogLibrary::FindCoreAction(TEXT("Action.Electrify")).PropagationLimit; }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWaterElectricPropagationTest,
	"RefactorTactics.Environment.WaterElectricPropagation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWaterElectricPropagationTest::RunTest(const FString&)
{
	// **Nome vincolante** del catalogo §15. La combo firma, verificata in PARTITA: Flux elettrifica un nemico
	// nell'acqua, e la scarica raggiunge il secondo nemico due celle piu' in la' lungo la pozza.
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	if (GEngine)
	{
		FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
		Ctx.SetCurrentWorld(World);
	}

	// Pozza continua: (1,0) - (2,0) - (3,0). L'attaccante resta all'asciutto in (0,0).
	TMap<FRTCellId, ERTHexSurface> Water;
	Water.Add(FRTCellId(1, 0), ERTHexSurface::ShallowWater);
	Water.Add(FRTCellId(2, 0), ERTHexSurface::ShallowWater);
	Water.Add(FRTCellId(3, 0), ERTHexSurface::ShallowWater);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = MakePropMap(/*Radius*/ 5, Water);

	auto SpawnPropUnit = [World](int32 TeamId, const FRTCellId& Cell) -> ARTUnit*
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeVektor());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell;
		return U;
	};

	ARTUnit* Caster = SpawnPropUnit(/*Team*/ 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnPropUnit(/*Team*/ 1, FRTCellId(1, 0)); // nell'acqua: prende il colpo iniziale
	ARTUnit* Bystander = SpawnPropUnit(/*Team*/ 1, FRTCellId(3, 0)); // nella stessa pozza, due passi piu' in la'
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Caster"), Caster) || !TestNotNull(TEXT("Target"), Target)
		|| !TestNotNull(TEXT("Bystander"), Bystander) || !TestNotNull(TEXT("TM"), TM))
	{
		if (GEngine) { GEngine->DestroyWorldContext(World); }
		World->DestroyWorld(false);
		return false;
	}

	// `Action.Electrify` sostituisce lo scatto (indice 3): `AbilityCooldowns` non si allarga da solo.
	URTActionData* Electrify = NewObject<URTActionData>(Caster);
	Electrify->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Electrify"));
	Electrify->RangeCells = Electrify->Def.RangeCells;
	Electrify->CooldownTurns = Electrify->Def.CooldownTurns;
	Electrify->Power = URTCatalogLibrary::FirstDamage(Electrify->Def);
	Caster->Abilities[3] = Electrify;
	Caster->PlannedAbilityIndex = 3;
	Caster->PlannedAttackTarget = Target;

	const int32 TargetBefore = Target->Health;
	const int32 BystanderBefore = Bystander->Health;

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	TestEqual(TEXT("il bersaglio incassa il danno iniziale"), TargetBefore - Target->Health, 20);
	TestEqual(TEXT("e l'elettricita' arriva a chi condivide la pozza, col danno propagato"),
		BystanderBefore - Bystander->Health, 12);

	// L'esito e' nel TurnLog con l'identita' dell'azione: un danno comparso su un'unita' mai bersagliata
	// sarebbe inspiegabile nel replay.
	int32 PropagationEntries = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Combat && E.ActionId == FName(TEXT("Action.Electrify")))
		{
			++PropagationEntries;
		}
	}
	TestEqual(TEXT("due voci di TurnLog, una per unita' colpita"), PropagationEntries, 2);

	if (GEngine) { GEngine->DestroyWorldContext(World); }
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPropagationHitsUnitOnceTest,
	"RefactorTactics.Environment.Propagation.HitsUnitOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPropagationHitsUnitOnceTest::RunTest(const FString&)
{
	// Un'unita' raggiungibile per DUE cammini d'acqua non prende il danno due volte: e' il vincolo esplicito
	// del catalogo («ogni unita' e' colpita una sola volta dallo stesso evento»), e senza di esso una pozza
	// larga trasformerebbe una scarica in un'esecuzione.
	TMap<FRTCellId, ERTHexSurface> Water;
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), /*Radius*/ 2))
	{
		Water.Add(Id, ERTHexSurface::ShallowWater); // pozza piena: cammini multipli garantiti
	}
	const URTHexMapAsset* Map = MakePropMap(/*Radius*/ 4, Water);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(MakePropUnit(0, 1, FRTCellId(0, 0)));  // sorgente
	Units.Add(MakePropUnit(1, 1, FRTCellId(2, 0)));  // raggiungibile per piu' cammini

	const TArray<FRTPropagationHit> Hits = URTTerrainLibrary::CollectElectricPropagation(
		Map, FRTCellId(0, 0), PropLimit(), /*Initial*/ 20, /*Propagated*/ 12, Units);

	int32 TimesHit = 0;
	for (const FRTPropagationHit& Hit : Hits)
	{
		if (Hit.UnitId == 1) { ++TimesHit; }
	}
	TestEqual(TEXT("colpita una volta sola"), TimesHit, 1);
	TestEqual(TEXT("due unita' colpite in tutto"), Hits.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPropagationDeterministicOrderTest,
	"RefactorTactics.Environment.Propagation.DeterministicOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPropagationDeterministicOrderTest::RunTest(const FString&)
{
	// L'ordine dichiarato e' `distanza dalla sorgente -> CellId -> UnitId`, e non dipende dall'ordine in cui
	// le unita' arrivano: permutare l'input non cambia l'output (invariante #4).
	TMap<FRTCellId, ERTHexSurface> Water;
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), /*Radius*/ 2))
	{
		Water.Add(Id, ERTHexSurface::ShallowWater);
	}
	const URTHexMapAsset* Map = MakePropMap(/*Radius*/ 4, Water);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(MakePropUnit(0, 1, FRTCellId(0, 0)));   // sorgente, 0 passi
	Units.Add(MakePropUnit(1, 1, FRTCellId(2, 0)));   // 2 passi
	Units.Add(MakePropUnit(2, 1, FRTCellId(1, 0)));   // 1 passo
	Units.Add(MakePropUnit(3, 1, FRTCellId(0, 1)));   // 1 passo, cella diversa

	const TArray<FRTPropagationHit> Direct = URTTerrainLibrary::CollectElectricPropagation(
		Map, FRTCellId(0, 0), PropLimit(), 20, 12, Units);

	TArray<FRTHexCombatUnit> Shuffled;
	Shuffled.Add(Units[2]);
	Shuffled.Add(Units[0]);
	Shuffled.Add(Units[3]);
	Shuffled.Add(Units[1]);
	const TArray<FRTPropagationHit> Permuted = URTTerrainLibrary::CollectElectricPropagation(
		Map, FRTCellId(0, 0), PropLimit(), 20, 12, Shuffled);

	TestEqual(TEXT("permutare l'input non cambia l'ordine dell'output"), PropHitIds(Direct), PropHitIds(Permuted));

	// La sorgente e' sempre prima (0 passi), poi i vicini ordinati per cella.
	if (TestEqual(TEXT("quattro unita' colpite"), Direct.Num(), 4))
	{
		TestEqual(TEXT("la sorgente risolve per prima"), Direct[0].UnitId, 0);
		TestTrue(TEXT("le distanze non decrescono mai"),
			Direct[0].Steps <= Direct[1].Steps && Direct[1].Steps <= Direct[2].Steps
			&& Direct[2].Steps <= Direct[3].Steps);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPropagationRespectsRangeLimitTest,
	"RefactorTactics.Environment.Propagation.RespectsRangeLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPropagationRespectsRangeLimitTest::RunTest(const FString&)
{
	// «Propagazione senza limite» e' un errore DICHIARATO del catalogo §17: su una mappa d'acqua colpirebbe
	// tutti e renderebbe il turno impredicibile. Il limite si conta in PASSI sull'acqua, non in distanza
	// esagonale: e' l'acqua che conduce, non l'aria.
	TMap<FRTCellId, ERTHexSurface> Water;
	for (int32 X = 1; X <= 5; ++X)
	{
		Water.Add(FRTCellId(X, 0), ERTHexSurface::ShallowWater); // canale rettilineo lungo 5
	}
	const URTHexMapAsset* Map = MakePropMap(/*Radius*/ 6, Water);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(MakePropUnit(0, 1, FRTCellId(1, 0)));  // sorgente
	Units.Add(MakePropUnit(1, 1, FRTCellId(4, 0)));  // 3 passi: dentro il limite
	Units.Add(MakePropUnit(2, 1, FRTCellId(5, 0)));  // 4 passi: fuori

	const TArray<FRTPropagationHit> Hits = URTTerrainLibrary::CollectElectricPropagation(
		Map, FRTCellId(1, 0), PropLimit(), 20, 12, Units);

	TestEqual(TEXT("il limite del catalogo e' 3 passi"), PropLimit(), 3);
	TestNotNull(TEXT("l'unita' a 3 passi e' colpita"), FindPropHit(Hits, 1));
	TestNull(TEXT("quella a 4 no"), FindPropHit(Hits, 2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPropagationStopsAtNonConductiveTest,
	"RefactorTactics.Environment.Propagation.StopsAtNonConductive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPropagationStopsAtNonConductiveTest::RunTest(const FString&)
{
	// Il caso NEGATIVO, che nessuno dei test della DoD copriva: la catena si interrompe su terreno asciutto.
	// Senza questo, una propagazione che ignorasse del tutto le superfici resterebbe verde.
	TMap<FRTCellId, ERTHexSurface> Water;
	Water.Add(FRTCellId(1, 0), ERTHexSurface::ShallowWater);
	// (2,0) resta Floor: interrompe il canale
	Water.Add(FRTCellId(3, 0), ERTHexSurface::ShallowWater);
	const URTHexMapAsset* Map = MakePropMap(/*Radius*/ 5, Water);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(MakePropUnit(0, 1, FRTCellId(1, 0)));  // sorgente, nell'acqua
	Units.Add(MakePropUnit(1, 1, FRTCellId(2, 0)));  // sull'asciutto, adiacente: NON conduce
	Units.Add(MakePropUnit(2, 1, FRTCellId(3, 0)));  // nella seconda pozza, isolata

	const TArray<FRTPropagationHit> Hits = URTTerrainLibrary::CollectElectricPropagation(
		Map, FRTCellId(1, 0), PropLimit(), 20, 12, Units);

	TestEqual(TEXT("solo la sorgente e' colpita"), Hits.Num(), 1);
	TestNotNull(TEXT("il bersaglio del colpo diretto"), FindPropHit(Hits, 0));
	TestNull(TEXT("chi sta all'asciutto non conduce"), FindPropHit(Hits, 1));
	TestNull(TEXT("e la pozza oltre l'interruzione resta fuori"), FindPropHit(Hits, 2));

	// Contro-prova: un'unita' BAGNATA all'asciutto non e' un ponte. La conduzione e' della CELLA
	// (`bConductsElectricity`), non dello stato dell'unita' — `Status.Wet` resta il bonus di
	// `Flux.LinearDischarge`. Due modelli di conduzione paralleli sarebbero la duplicazione che il canone vieta.
	TestFalse(TEXT("Floor non conduce"),
		URTTerrainLibrary::FindTerrainDef(ERTHexSurface::Floor).bConductsElectricity);
	TestTrue(TEXT("acqua bassa e superficie conduttiva si'"),
		URTTerrainLibrary::FindTerrainDef(ERTHexSurface::ShallowWater).bConductsElectricity
		&& URTTerrainLibrary::FindTerrainDef(ERTHexSurface::Conductive).bConductsElectricity);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPropagationDamageDiffersTest,
	"RefactorTactics.Environment.Propagation.InitialAndPropagatedDamageDiffer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPropagationDamageDiffersTest::RunTest(const FString&)
{
	// 20 alla sorgente, 12 a chi la scarica raggiunge: due numeri diversi del catalogo, che nessuno dei test
	// della DoD verificava. Se il codice ne usasse uno solo, tutti gli altri test resterebbero verdi.
	TMap<FRTCellId, ERTHexSurface> Water;
	Water.Add(FRTCellId(0, 0), ERTHexSurface::ShallowWater);
	Water.Add(FRTCellId(1, 0), ERTHexSurface::ShallowWater);
	const URTHexMapAsset* Map = MakePropMap(/*Radius*/ 3, Water);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(MakePropUnit(0, 1, FRTCellId(0, 0)));
	Units.Add(MakePropUnit(1, 1, FRTCellId(1, 0)));

	const TArray<FRTPropagationHit> Hits = URTTerrainLibrary::CollectElectricPropagation(
		Map, FRTCellId(0, 0), PropLimit(), /*Initial*/ 20, /*Propagated*/ 12, Units);

	const FRTPropagationHit* Source = FindPropHit(Hits, 0);
	const FRTPropagationHit* Reached = FindPropHit(Hits, 1);
	if (TestNotNull(TEXT("sorgente colpita"), Source) && TestNotNull(TEXT("propagazione arrivata"), Reached))
	{
		TestEqual(TEXT("20 alla sorgente"), Source->Damage, 20);
		TestEqual(TEXT("12 a chi la scarica raggiunge"), Reached->Damage, 12);
		TestEqual(TEXT("la sorgente e' a zero passi"), Source->Steps, 0);
		TestEqual(TEXT("l'altro a uno"), Reached->Steps, 1);
	}

	// Il danno iniziale non e' un numero del test: viene dagli `Effects` dell'azione di catalogo.
	TestEqual(TEXT("`Action.Electrify` dichiara 20 danni"), PropInitialDamage(), 20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPropagationNoMapFailsClosedTest,
	"RefactorTactics.Environment.Propagation.NoMapFailsClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPropagationNoMapFailsClosedTest::RunTest(const FString&)
{
	// Senza mappa non si sa quali celle conducano: fail-closed, come LOS e Intercept. Inventare una
	// propagazione su una topologia sconosciuta sarebbe danno arbitrario in partita.
	TArray<FRTHexCombatUnit> Units;
	Units.Add(MakePropUnit(0, 1, FRTCellId(0, 0)));
	Units.Add(MakePropUnit(1, 1, FRTCellId(1, 0)));

	const TArray<FRTPropagationHit> Hits = URTTerrainLibrary::CollectElectricPropagation(
		nullptr, FRTCellId(0, 0), PropLimit(), 20, 12, Units);
	TestEqual(TEXT("nessuna mappa, nessun colpo"), Hits.Num(), 0);

	// Limite non positivo = nessuna propagazione (il validator del catalogo rifiuta gia' i negativi):
	// la sorgente incassa comunque il colpo diretto, che non e' propagazione.
	TMap<FRTCellId, ERTHexSurface> Water;
	Water.Add(FRTCellId(0, 0), ERTHexSurface::ShallowWater);
	Water.Add(FRTCellId(1, 0), ERTHexSurface::ShallowWater);
	const URTHexMapAsset* Map = MakePropMap(/*Radius*/ 3, Water);
	const TArray<FRTPropagationHit> NoSpread = URTTerrainLibrary::CollectElectricPropagation(
		Map, FRTCellId(0, 0), /*MaxSteps*/ 0, 20, 12, Units);
	TestEqual(TEXT("limite 0: solo la sorgente"), NoSpread.Num(), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
