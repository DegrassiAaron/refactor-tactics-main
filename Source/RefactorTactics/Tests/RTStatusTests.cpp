#include "Misc/AutomationTest.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

/**
 * Stati temporanei (E8, CP 8.2 — issue #65): durata, scadenza deterministica nel Cleanup ed effetto
 * osservabile IN PARTITA. I test stanno in un `UWorld` con un turno vero e non sulla sola funzione pura:
 * il difetto ricorrente di questa area (issue #137, `Action.Slow` al CP 4.7) e' proprio la formula coperta
 * dal test e il cablaggio assente, che nessun test puro puo' vedere.
 *
 * Decisioni applicate: `spec-stati-temporanei-cp82.md` §3.
 */
namespace
{
	UWorld* MakeStatusWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyStatusWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Esagono pieno di raggio Radius sul layer 0, con una cella di superficie scelta in `Special`. */
	ARTHexMapActor* SpawnStatusMap(UWorld* World, int32 Radius, const FRTCellId& Special, ERTHexSurface Surface,
		int32 MoveCost)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		FRTHexCellData SpecialCell(Special);
		SpecialCell.Surface = Surface;
		SpecialCell.MoveCost = MoveCost;
		M->AddOrUpdateCell(SpecialCell);
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SpawnStatusUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	void RunStatusTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** Il turno successivo senza muoversi: il piano resta la cella su cui l'unita' gia' si trova. */
	void StandStill(ARTUnit* Unit)
	{
		Unit->PlannedAbilityIndex = INDEX_NONE;
		Unit->PlannedPath.Reset();
		Unit->PlannedCell = Unit->Cell;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusPersistsWhileOnCellTest,
	"RefactorTactics.Status.PersistsWhileOnCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusPersistsWhileOnCellTest::RunTest(const FString&)
{
	// L'acqua bassa dichiara `Status.Wet` con durata 0 = "finche' sulla cella" (catalogo terreni §2).
	// Chi ci resta sopra deve restare bagnato turno dopo turno: un contatore che decrementa lo farebbe
	// scadere sotto i piedi dell'unita', ancora nell'acqua.
	UWorld* World = MakeStatusWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnStatusMap(World, /*Radius=*/ 4, FRTCellId(1, 0, 0), ERTHexSurface::ShallowWater, /*MoveCost=*/ 2);

	ARTUnit* Mover = SpawnStatusUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnStatusUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(4, 0)); // fuori portata: nessun colpo
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyStatusWorld(World); return false; }

	Mover->PlannedAbilityIndex = INDEX_NONE;
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0) };
	Mover->PlannedCell = FRTCellId(1, 0);
	StandStill(Foe);

	RunStatusTurn(TM);
	TestTrue(TEXT("entrato in acqua: bagnato"), Mover->HasStatus(TAG_Status_Wet));

	// Secondo turno fermo nell'acqua: nessuna nuova applicazione (OnEnter scatta solo entrando).
	StandStill(Mover);
	StandStill(Foe);
	RunStatusTurn(TM);
	TestTrue(TEXT("ancora nell'acqua: ancora bagnato"), Mover->HasStatus(TAG_Status_Wet));

	DestroyStatusWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusRevokedOnLeavingCellTest,
	"RefactorTactics.Status.RevokedOnLeavingCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusRevokedOnLeavingCellTest::RunTest(const FString&)
{
	// L'altra meta' di "finche' sulla cella": uscire asciuga, e asciuga NELLO STESSO turno in cui si esce.
	// La revoca guarda la posizione FINALE, quindi nel Blast del turno dopo l'unita' non e' piu' un bersaglio
	// conduttivo per Flux.
	//
	// Due turni e non uno: "non e' bagnato dopo essere uscito" e' vero anche in un mondo dove `Wet` non si
	// applica mai. Il primo turno stabilisce che lo stato c'era davvero, il secondo prova la revoca.
	UWorld* World = MakeStatusWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnStatusMap(World, /*Radius=*/ 4, FRTCellId(1, 0, 0), ERTHexSurface::ShallowWater, /*MoveCost=*/ 2);

	ARTUnit* Mover = SpawnStatusUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnStatusUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyStatusWorld(World); return false; }

	Mover->PlannedAbilityIndex = INDEX_NONE;
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0) };
	Mover->PlannedCell = FRTCellId(1, 0);
	StandStill(Foe);

	RunStatusTurn(TM);
	if (!TestTrue(TEXT("premessa: fermarsi in acqua bagna"), Mover->HasStatus(TAG_Status_Wet)))
	{
		DestroyStatusWorld(World); // senza la premessa, l'asserzione sulla revoca sarebbe vera a vuoto
		return false;
	}

	Mover->PlannedAbilityIndex = INDEX_NONE;
	Mover->PlannedPath = { FRTCellId(1, 0), FRTCellId(2, 0) };
	Mover->PlannedCell = FRTCellId(2, 0);
	StandStill(Foe);

	RunStatusTurn(TM);

	TestEqual(TEXT("il movimento e' arrivato a destinazione"), Mover->Cell, FRTCellId(2, 0));
	TestFalse(TEXT("uscito dall'acqua: asciutto gia' a fine turno"), Mover->HasStatus(TAG_Status_Wet));

	DestroyStatusWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusBurningDamagesInCleanupTest,
	"RefactorTactics.Status.Burning.DamagesInCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusBurningDamagesInCleanupTest::RunTest(const FString&)
{
	// Catalogo terreni §2: `Burning` toglie 8 danni NEL CLEANUP. Entrare nel fuoco ne costa 10 subito
	// (CP 8.1, gia' coperto): il turno in cui si entra ne toglie quindi 18 in tutto.
	UWorld* World = MakeStatusWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnStatusMap(World, /*Radius=*/ 4, FRTCellId(1, 0, 0), ERTHexSurface::Fire, /*MoveCost=*/ 2);

	ARTUnit* Mover = SpawnStatusUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnStatusUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyStatusWorld(World); return false; }

	Mover->Shield = 0; // niente scudo di mezzo: qui si misura il danno, non l'assorbimento
	const int32 StartHealth = Mover->Health;
	Mover->PlannedAbilityIndex = INDEX_NONE;
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0) };
	Mover->PlannedCell = FRTCellId(1, 0);
	StandStill(Foe);

	RunStatusTurn(TM);

	TestTrue(TEXT("il fuoco ha acceso Burning"), Mover->HasStatus(TAG_Status_Burning));
	TestEqual(TEXT("10 all'ingresso + 8 nel Cleanup"), Mover->Health, StartHealth - 18);

	DestroyStatusWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusBurningExpiresAfterTwoTurnsTest,
	"RefactorTactics.Status.Burning.ExpiresAfterTwoTurns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusBurningExpiresAfterTwoTurnsTest::RunTest(const FString&)
{
	// Durata 2 turni = 2 Cleanup che bruciano, poi basta. Un danno che continua e' un hazard eterno; uno che
	// si ferma dopo il primo rende la durata dichiarata dal catalogo una decorazione.
	UWorld* World = MakeStatusWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnStatusMap(World, /*Radius=*/ 4, FRTCellId(1, 0, 0), ERTHexSurface::Fire, /*MoveCost=*/ 2);

	ARTUnit* Mover = SpawnStatusUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnStatusUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyStatusWorld(World); return false; }

	Mover->Shield = 0;
	Mover->PlannedAbilityIndex = INDEX_NONE;
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) }; // attraversa il fuoco e ne esce
	Mover->PlannedCell = FRTCellId(2, 0);
	StandStill(Foe);

	RunStatusTurn(TM); // turno 1: 10 (ingresso) + 8 (Cleanup), durata residua 1
	const int32 AfterFirst = Mover->Health;

	StandStill(Mover);
	StandStill(Foe);
	RunStatusTurn(TM); // turno 2: solo gli 8 di Burning, lontano dal fuoco
	const int32 AfterSecond = Mover->Health;

	StandStill(Mover);
	StandStill(Foe);
	RunStatusTurn(TM); // turno 3: lo stato e' scaduto, nessun danno

	TestEqual(TEXT("il secondo turno brucia ancora"), AfterSecond, AfterFirst - 8);
	TestFalse(TEXT("dopo due turni Burning e' scaduto"), Mover->HasStatus(TAG_Status_Burning));
	TestEqual(TEXT("il terzo turno non brucia piu'"), Mover->Health, AfterSecond);

	DestroyStatusWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusBurningDefeatCountsThisTurnTest,
	"RefactorTactics.Status.Burning.DefeatCountsThisTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusBurningDefeatCountsThisTurnTest::RunTest(const FString&)
{
	// ADR-0003 §3: gli effetti ambientali risolvono nel Cleanup PRIMA dei KO. Chi muore bruciato muore in
	// QUESTO turno: se il danno arrivasse dopo il conteggio delle unita' vive, la partita si deciderebbe con
	// un turno di ritardo e il TurnLog racconterebbe un vincitore che al momento del conteggio era gia' morto.
	UWorld* World = MakeStatusWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnStatusMap(World, /*Radius=*/ 4, FRTCellId(1, 0, 0), ERTHexSurface::Fire, /*MoveCost=*/ 2);

	ARTUnit* Mover = SpawnStatusUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnStatusUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyStatusWorld(World); return false; }

	// Sopravvive ai 10 danni dell'ingresso e muore per gli 8 del Cleanup: il KO e' attribuibile a Burning.
	Mover->Shield = 0;
	Mover->Health = 15;
	Mover->PlannedAbilityIndex = INDEX_NONE;
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0) };
	Mover->PlannedCell = FRTCellId(1, 0);
	StandStill(Foe);

	RunStatusTurn(TM);

	TestFalse(TEXT("bruciato a morte"), Mover->IsAlive());
	TestEqual(TEXT("la partita e' finita in questo turno"), TM->GetPhase(), ERTMatchPhase::MatchEnded);

	DestroyStatusWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusWetRemovesBurningTest,
	"RefactorTactics.Status.WetRemovesBurning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusWetRemovesBurningTest::RunTest(const FString&)
{
	// Nome richiesto dalla DoD di #65. Catalogo terreni §2: `Burning` e' "rimosso da `Wet`". La regola sta in
	// ARTUnit::ApplyStatus e non nel chiamante, cosi' vale per OGNI sorgente di Wet — acqua bassa, Riva,
	// e domani `CreateWater` — invece che nel punto che si e' ricordato di scriverla.
	UWorld* World = MakeStatusWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnStatusMap(World, /*Radius=*/ 4, FRTCellId(1, 0, 0), ERTHexSurface::ShallowWater, /*MoveCost=*/ 2);

	ARTUnit* Mover = SpawnStatusUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnStatusUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyStatusWorld(World); return false; }

	Mover->Shield = 0;
	Mover->ApplyStatus(TAG_Status_Burning, /*Turni*/ 2); // in fiamme prima di entrare in acqua
	const int32 StartHealth = Mover->Health;
	Mover->PlannedAbilityIndex = INDEX_NONE;
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0) };
	Mover->PlannedCell = FRTCellId(1, 0);
	StandStill(Foe);

	RunStatusTurn(TM);

	TestTrue(TEXT("l'acqua bagna"), Mover->HasStatus(TAG_Status_Wet));
	TestFalse(TEXT("l'acqua ha spento le fiamme"), Mover->HasStatus(TAG_Status_Burning));
	// La prova che conta e' il danno mancato: senza di essa, "il tag non c'e'" potrebbe voler dire che il
	// Cleanup ha bruciato l'unita' e POI ha lasciato scadere lo stato.
	TestEqual(TEXT("spento prima del Cleanup: nessun danno da fiamme"), Mover->Health, StartHealth);

	DestroyStatusWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusWetAmplifiesFluxDischargeTest,
	"RefactorTactics.Status.Wet.AmplifiesFluxDischarge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusWetAmplifiesFluxDischargeTest::RunTest(const FString&)
{
	// `URTCombatLibrary::FluxWetDischargeBonus` esiste dal CP 6.2 ma finora compariva SOLO nei test: nessuno
	// in partita leggeva lo stato del bersaglio (limite dichiarato in `Heroes.Flux.WetBonus`). Qui il turno
	// e' vero: due bersagli identici, uno bagnato e uno no, colpiti dalla stessa azione.
	UWorld* World = MakeStatusWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnStatusMap(World, /*Radius=*/ 5, FRTCellId(2, 0, 0), ERTHexSurface::ShallowWater, /*MoveCost=*/ 2);

	ARTUnit* Flux = SpawnStatusUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(0, 0));
	ARTUnit* WetTarget = SpawnStatusUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(2, 0)); // nell'acqua
	ARTUnit* DryTarget = SpawnStatusUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(0, 2)); // all'asciutto
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Flux || !WetTarget || !DryTarget) { DestroyStatusWorld(World); return false; }

	// L'azione arriva dal catalogo EROI, non da numeri riscritti qui: se il catalogo cambia i 24 danni o la
	// forma lineare, questo test lo segue invece di sorvegliare una copia.
	const URTHeroData* FluxData = URTHeroCatalogLibrary::MakeFlux();
	const URTActionData* Source = FluxData ? FluxData->Actions[1] : nullptr; // indice 1 = Flux.LinearDischarge
	if (!TestNotNull(TEXT("Flux.LinearDischarge nel catalogo eroi"), Source))
	{
		DestroyStatusWorld(World);
		return false;
	}
	URTActionData* Discharge = NewObject<URTActionData>(Flux);
	Discharge->DisplayName = FText::FromString(TEXT("Scarica lineare"));
	Discharge->Def = Source->Def;
	Discharge->Shape = Source->Shape;             // Line: la forma vive su URTActionData, non su Def
	Discharge->RangeCells = Source->RangeCells;
	Discharge->Power = Source->Power;
	const int32 DischargeIdx = Flux->Abilities.Add(Discharge);

	WetTarget->Shield = 0;
	DryTarget->Shield = 0;
	WetTarget->ApplyStatus(TAG_Status_Wet, ARTUnit::PersistentWhileOnCell); // sta nell'acqua: gia' bagnato
	const int32 WetStart = WetTarget->Health;
	const int32 DryStart = DryTarget->Health;

	Flux->PlannedAbilityIndex = DischargeIdx;
	Flux->PlannedAttackTarget = WetTarget;
	Flux->PlannedCell = Flux->Cell;
	StandStill(WetTarget);
	StandStill(DryTarget);

	RunStatusTurn(TM);

	const int32 WetDamage = WetStart - WetTarget->Health;
	if (!TestTrue(TEXT("premessa: il colpo e' arrivato"), WetDamage > 0))
	{
		DestroyStatusWorld(World);
		return false;
	}
	TestEqual(TEXT("24 dichiarati + 8 perche' il bersaglio e' bagnato"),
		WetDamage, 24 + URTCombatLibrary::FluxWetDischargeBonus);

	// Controprova nello stesso mondo: stesso attaccante, stessa azione, bersaglio ASCIUTTO -> nessun bonus.
	Flux->PlannedAbilityIndex = DischargeIdx;
	Flux->PlannedAttackTarget = DryTarget;
	Flux->PlannedCell = Flux->Cell;
	StandStill(WetTarget);
	StandStill(DryTarget);
	RunStatusTurn(TM);

	TestEqual(TEXT("bersaglio asciutto: i 24 dichiarati e basta"), DryStart - DryTarget->Health, 24);

	DestroyStatusWorld(World);
	return true;
}

namespace
{
	/**
	 * Azione del catalogo core montata sull'unita', con range esplicito.
	 *
	 * Il range va passato a mano per `Action.MarkTarget`: il catalogo C++ la spedisce con `Range 0`
	 * (`RTCatalogLibrary.cpp:379`), cioe' non raggiunge nessuno, mentre la tabella del catalogo documentale
	 * non ha affatto una colonna Range per le offensive. E' un difetto del catalogo azioni (epic E4), non
	 * degli stati: qui si aggira nel test perche' CP 8.2 non e' il posto per deciderne il valore.
	 */
	int32 AddCoreAction(ARTUnit* Unit, const TCHAR* ActionId, int32 RangeCells,
		ERTAbilityShape Shape = ERTAbilityShape::Single, int32 AreaRadius = 0)
	{
		if (!Unit) { return INDEX_NONE; }
		URTActionData* Action = NewObject<URTActionData>(Unit);
		Action->Def = URTCatalogLibrary::FindCoreAction(FName(ActionId));
		Action->Def.RangeCells = RangeCells;
		Action->RangeCells = RangeCells;
		Action->Shape = Shape;
		Action->AreaRadius = AreaRadius;
		Action->Power = 0;
		for (const FRTActionEffectSpec& Spec : Action->Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage) { Action->Power = Spec.Amount; break; }
		}
		return Unit->Abilities.Add(Action);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusMarkedAllyHitConsumesBonusTest,
	"RefactorTactics.Status.Marked.AllyHitConsumesBonus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusMarkedAllyHitConsumesBonusTest::RunTest(const FString&)
{
	// Chiude la issue #137. Catalogo azioni §3: `MarkTarget` ha priorita' 40, "la piu' bassa delle offensive,
	// perche' un marchio che arrivasse dopo i colpi non servirebbe a nulla" — quindi il marchio vale per i
	// colpi dello STESSO Blast a priorita' piu' alta (`Action.PrecisionAttack` e' 60).
	//
	// Due attaccanti alleati sullo stesso bersaglio: il +6 si somma UNA volta sola. Con un attaccante solo,
	// un bonus applicato a ogni colpo sarebbe indistinguibile da un bonus consumato correttamente.
	UWorld* World = MakeStatusWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnStatusMap(World, /*Radius=*/ 5, FRTCellId(4, 0, 0), ERTHexSurface::Floor, /*MoveCost=*/ 1);

	ARTUnit* Marker = SpawnStatusUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(0, 0));
	ARTUnit* Ally1 = SpawnStatusUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(0, 1));
	ARTUnit* Ally2 = SpawnStatusUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(1, 0));
	ARTUnit* Victim = SpawnStatusUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Marker || !Ally1 || !Ally2 || !Victim) { DestroyStatusWorld(World); return false; }

	const int32 MarkIdx = AddCoreAction(Marker, TEXT("Action.MarkTarget"), /*Range*/ 4);
	const int32 Hit1Idx = AddCoreAction(Ally1, TEXT("Action.PrecisionAttack"), /*Range*/ 4);
	const int32 Hit2Idx = AddCoreAction(Ally2, TEXT("Action.PrecisionAttack"), /*Range*/ 4);
	const int32 BasicDamage = Ally1->Abilities[Hit1Idx]->Power; // 24 dal catalogo
	if (!TestTrue(TEXT("l'attacco di precisione del catalogo fa danno"), BasicDamage > 0))
	{
		DestroyStatusWorld(World);
		return false;
	}

	Victim->Shield = 0;
	const int32 StartHealth = Victim->Health;

	Marker->PlannedAbilityIndex = MarkIdx;
	Marker->PlannedAttackTarget = Victim;
	Marker->PlannedCell = Marker->Cell;
	Ally1->PlannedAbilityIndex = Hit1Idx;
	Ally1->PlannedAttackTarget = Victim;
	Ally1->PlannedCell = Ally1->Cell;
	Ally2->PlannedAbilityIndex = Hit2Idx;
	Ally2->PlannedAttackTarget = Victim;
	Ally2->PlannedCell = Ally2->Cell;
	StandStill(Victim);

	RunStatusTurn(TM);

	// Due colpi base + un solo marchio: 2*base + 6, non 2*base + 12.
	TestEqual(TEXT("il marchio vale una volta sola, sui colpi dello stesso Blast"),
		StartHealth - Victim->Health, 2 * BasicDamage + URTCombatLibrary::MarkedFirstHitBonus);
	TestFalse(TEXT("il marchio e' stato consumato"), Victim->HasStatus(TAG_Status_Marked));

	DestroyStatusWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusMarkedEnemyHitDoesNotConsumeTest,
	"RefactorTactics.Status.Marked.EnemyHitDoesNotConsume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusMarkedEnemyHitDoesNotConsumeTest::RunTest(const FString&)
{
	// Il catalogo dice "il prossimo attacco ALLEATO": il marchio e' un'informazione di squadra, non una
	// vulnerabilita' generica. Un AoE della squadra del BERSAGLIO (fuoco amico) lo colpisce ma non ne
	// approfitta e non lo consuma — altrimenti l'avversario potrebbe bruciare il marchio colpendo il proprio
	// compagno.
	//
	// Il caso e' diventato riproducibile in partita solo dopo il cablaggio di `bFriendlyFire` (stesso branch):
	// prima nessun AoE poteva colpire un alleato e questo test usava un setup artificiale.
	UWorld* World = MakeStatusWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnStatusMap(World, /*Radius=*/ 5, FRTCellId(5, 0, 0), ERTHexSurface::Floor, /*MoveCost=*/ 1);

	ARTUnit* Marker = SpawnStatusUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(0, 0));
	ARTUnit* Ally = SpawnStatusUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(1, 0));   // centro dell'AoE nemico
	ARTUnit* Victim = SpawnStatusUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(2, 0)); // marcato, adiacente
	ARTUnit* VictimMate = SpawnStatusUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Marker || !Ally || !Victim || !VictimMate) { DestroyStatusWorld(World); return false; }

	const int32 MarkIdx = AddCoreAction(Marker, TEXT("Action.MarkTarget"), /*Range*/ 4);
	const int32 ShotIdx = AddCoreAction(Ally, TEXT("Action.PrecisionAttack"), /*Range*/ 4);
	const int32 AoEIdx = AddCoreAction(VictimMate, TEXT("Action.CircularAoE"), /*Range*/ 4,
		ERTAbilityShape::Area, /*AreaRadius*/ 1);
	const int32 ShotDamage = Ally->Abilities[ShotIdx]->Power;
	const int32 AoEDamage = VictimMate->Abilities[AoEIdx]->Power;

	Victim->Shield = 0;
	const int32 StartHealth = Victim->Health;

	Marker->PlannedAbilityIndex = MarkIdx;
	Marker->PlannedAttackTarget = Victim;
	Marker->PlannedCell = Marker->Cell;
	Ally->PlannedAbilityIndex = ShotIdx;
	Ally->PlannedAttackTarget = Victim;
	Ally->PlannedCell = Ally->Cell;
	VictimMate->PlannedAbilityIndex = AoEIdx;
	VictimMate->PlannedAttackTarget = Ally;   // centro sul nemico: il compagno marcato ci finisce dentro
	VictimMate->PlannedCell = VictimMate->Cell;
	StandStill(Victim);

	RunStatusTurn(TM);

	// 24 (alleato del marcatore) + 6 (marchio speso una volta) + 18 (fuoco amico, senza bonus) = 48.
	// Se il fuoco amico avesse consumato il marchio per primo, il totale sarebbe 42: il test distingue i casi.
	TestEqual(TEXT("il +6 lo prende solo la squadra del marcatore, e una volta sola"),
		StartHealth - Victim->Health, ShotDamage + AoEDamage + URTCombatLibrary::MarkedFirstHitBonus);

	DestroyStatusWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusObscuredBySmokeTest,
	"RefactorTactics.Status.Obscured.AppliedBySmokeWithoutChangingGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusObscuredBySmokeTest::RunTest(const FString&)
{
	// `Obscured` diventa uno stato REALE (applicato dal fumo, revocato all'uscita) ma NON diventa un secondo
	// gate del targeting: il cap a 2 celle resta calcolato da `URTTerrainLibrary::EffectiveTargetingRange`
	// sulla linea del colpo (decisione D4 della spec). Due verita' sulla stessa regola divergerebbero appena
	// una delle due cambia — qui si verifica che ne resti una sola.
	UWorld* World = MakeStatusWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnStatusMap(World, /*Radius=*/ 6, FRTCellId(1, 0, 0), ERTHexSurface::Smoke, /*MoveCost=*/ 1);

	ARTUnit* Shooter = SpawnStatusUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnStatusUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(5, 0)); // a 5 celle: oltre il cap
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Shooter || !Foe) { DestroyStatusWorld(World); return false; }

	const int32 HitIdx = AddCoreAction(Shooter, TEXT("Action.PrecisionAttack"), /*Range*/ 6);
	Foe->Shield = 0;
	const int32 FoeStart = Foe->Health;

	// Entra nel fumo e spara da li': il cap di targeting vale «dentro o attraverso».
	Shooter->PlannedAbilityIndex = HitIdx;
	Shooter->PlannedAttackTarget = Foe;
	Shooter->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0) };
	Shooter->PlannedCell = FRTCellId(1, 0);
	StandStill(Foe);

	RunStatusTurn(TM);

	TestTrue(TEXT("nel fumo: offuscato"), Shooter->HasStatus(TAG_Status_Obscured));
	TestEqual(TEXT("il cap del terreno regge: il colpo a 5 celle non arriva"), Foe->Health, FoeStart);

	// Esce dal fumo: lo stato se ne va con la cella, come `Wet`.
	Shooter->PlannedAbilityIndex = INDEX_NONE;
	Shooter->PlannedPath = { FRTCellId(1, 0), FRTCellId(2, 0) };
	Shooter->PlannedCell = FRTCellId(2, 0);
	StandStill(Foe);
	RunStatusTurn(TM);

	TestFalse(TEXT("fuori dal fumo: non piu' offuscato"), Shooter->HasStatus(TAG_Status_Obscured));

	DestroyStatusWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusExpiresInCleanupTest,
	"RefactorTactics.Status.ExpiresInCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusExpiresInCleanupTest::RunTest(const FString&)
{
	// Le due nature convivono nello STESSO Cleanup: chi ha una durata in turni scade, chi e' legato alla
	// cella no. Un tick che decrementasse tutto indistintamente romperebbe il secondo; una revoca che
	// ignorasse le durate romperebbe il primo.
	UWorld* World = MakeStatusWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnStatusMap(World, /*Radius=*/ 4, FRTCellId(1, 0, 0), ERTHexSurface::ShallowWater, /*MoveCost=*/ 2);

	ARTUnit* Mover = SpawnStatusUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnStatusUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyStatusWorld(World); return false; }

	Mover->ApplyStatus(TAG_Status_Exposed, /*Turni*/ 1); // durata esplicita: deve scadere in questo Cleanup
	Mover->PlannedAbilityIndex = INDEX_NONE;
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0) };   // e finisce nell'acqua: Wet persistente
	Mover->PlannedCell = FRTCellId(1, 0);
	StandStill(Foe);

	RunStatusTurn(TM);

	TestFalse(TEXT("la durata di 1 turno e' scaduta nel Cleanup"), Mover->HasStatus(TAG_Status_Exposed));
	TestTrue(TEXT("lo stato legato alla cella non scade con lei"), Mover->HasStatus(TAG_Status_Wet));

	DestroyStatusWorld(World);
	return true;
}
