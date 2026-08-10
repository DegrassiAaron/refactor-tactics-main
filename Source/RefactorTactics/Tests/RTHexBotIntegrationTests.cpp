#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTMovementActionLibrary.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Bot/RTHexBotLibrary.h"

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

	ARTUnit* SpawnHexBotUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell, bool bBot)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
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
	ARTUnit* BotA = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(2, -3), /*bBot*/ true);
	ARTUnit* BotB = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(3, -3), /*bBot*/ true);
	ARTUnit* FoeA = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(-2, 3), /*bBot*/ false);
	ARTUnit* FoeB = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(-3, 3), /*bBot*/ false);
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
			const URTActionData* Dash = Bot->GetAbility(Bot->PlannedDashAbility);
			TestTrue(TEXT("lo scatto pianificato e' un'abilita' di scatto"),
				Dash && URTCatalogLibrary::IsFastMovement(Dash->Def));
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
			const URTActionData* Ability = Bot->GetAbility(Bot->PlannedAbilityIndex);
			TestTrue(TEXT("il bersaglio e' entro la portata dalla cella di tiro"),
				Ability && URTHexLibrary::HexDistance(FiringCell, Bot->PlannedAttackTarget->Cell) <= Ability->RangeCells);
		}
	}

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

	ARTUnit* Bot = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(0, 0), /*bBot*/ true);
	ARTUnit* Foe = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(4, 0), /*bBot*/ false);
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

	ARTUnit* Bot = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(0, 0), /*bBot*/ true); // con Carica
	ARTUnit* Foe = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(5, 0), /*bBot*/ false);
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


/**
 * Il piano del bot non investe il compagno (#213).
 *
 * Il secondo anello del cablaggio: `PlanBots` deve popolare `FRTHexBotContext::Allies` e passare la FORMA
 * dell'azione, altrimenti ogni attacco viene pesato come un colpo singolo e il collaterale resta invisibile.
 *
 * La verifica e' sul comportamento osservabile e non sull'abilita' scelta: qualunque cosa il bot pianifichi,
 * le celle investite non devono contenere il compagno. L'assert speculare — senza il compagno in mezzo il
 * bot ATTACCA — impedisce che il test passi per il motivo sbagliato, cioe' un bot che non attacca mai.
 */
namespace
{
	/** Celle investite dal piano principale del bot, o vuoto se non ha pianificato un attacco. */
	TArray<FRTCellId> PlannedHitCells(ARTUnit* Bot)
	{
		TArray<FRTCellId> Out;
		if (!Bot || !Bot->PlannedAttackTarget || Bot->PlannedAbilityIndex == INDEX_NONE)
		{
			return Out;
		}
		const URTActionData* Ability = Bot->GetAbility(Bot->PlannedAbilityIndex);
		if (!Ability)
		{
			return Out;
		}
		return URTHexCombatLibrary::HexHitCells(Ability->Shape, Bot->PlannedCell,
			Bot->PlannedAttackTarget->Cell, Ability->RangeCells, Ability->AreaRadius);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotSparesAllyTest,
	"RefactorTactics.HexBotPlay.PlanDoesNotBlastDyingAlly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotSparesAllyTest::RunTest(const FString&)
{
	// --- Caso 1: il compagno morente sta fra il bot e il nemico ---
	{
		UWorld* World = MakeHexBotWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		SpawnHexBotMap(World, /*Radius=*/ 5);

		ARTUnit* Bot = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(0, 0), /*bBot*/ true);
		ARTUnit* Ally = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(1, 0), /*bBot*/ false);
		ARTUnit* Foe = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(2, 0), /*bBot*/ false);
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Bot || !Ally || !Foe) { DestroyHexBotWorld(World); return false; }

		// Morente: il collaterale sul compagno diventa LETALE, quindi il piano che lo investe e' nettamente
		// peggiore di qualunque alternativa. Con HP pieni la penalita' proporzionale potrebbe pareggiare.
		Ally->Health = 10;
		Ally->Shield = 0;

		TM->PlanBotsForTest();

		const TArray<FRTCellId> Hit = PlannedHitCells(Bot);
		TestFalse(TEXT("il piano del bot non investe il compagno morente"), Hit.Contains(Ally->Cell));

		DestroyHexBotWorld(World);
	}

	// --- Caso 2 (speculare): stesso setup, compagno FUORI dalla traiettoria ---
	{
		UWorld* World = MakeHexBotWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		SpawnHexBotMap(World, /*Radius=*/ 5);

		ARTUnit* Bot = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(0, 0), /*bBot*/ true);
		ARTUnit* Ally = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(0, -4), /*bBot*/ false);
		ARTUnit* Foe = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(2, 0), /*bBot*/ false);
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Bot || !Ally || !Foe) { DestroyHexBotWorld(World); return false; }

		Ally->Health = 10;
		Ally->Shield = 0;

		TM->PlanBotsForTest();

		// Se qui il bot non offendesse, il caso 1 non proverebbe nulla: passerebbe perche' il bot non attacca
		// mai, non perche' risparmia il compagno.
		//
		// «Offende» comprende la CARICA, e non e' un dettaglio: Bastion ha `Ram` (20 danni piu' spinta) oltre
		// a `ImpactShot` (8), quindi caricare vale di piu' e l'utility la sceglie — `score=90` contro `80`.
		// La carica pero' passa da `PlannedDashAbility`, non da `PlannedAttackTarget`, e guardare solo il
		// secondo faceva sembrare passivo un bot che stava scegliendo la mossa piu' aggressiva che aveva.
		//
		// L'asserzione stretta ha retto finche' fra le candidate c'era `Action.Wait` con 30 danni fittizi a
		// portata 5 (i default legacy di `URTActionData`, che `MakeGenericActions` non sovrascriveva): quella
		// vinceva sempre ed era un attacco «normale», quindi `PlannedAttackTarget` risultava valorizzato.
		const bool bChargesTarget = Bot->PlannedDashAbility != INDEX_NONE
			&& Bot->Abilities.IsValidIndex(Bot->PlannedDashAbility)
			&& Bot->Abilities[Bot->PlannedDashAbility]
			&& Bot->Abilities[Bot->PlannedDashAbility]->Def.MovementStyle == ERTMovementStyle::LinearCharge;
		TestTrue(TEXT("con il compagno fuori mira il bot offende davvero (attacco o carica)"),
			Bot->PlannedAttackTarget.Get() != nullptr || bChargesTarget);

		DestroyHexBotWorld(World);
	}

	return true;
}

/**
 * Il bot filtra le proprie candidate con lo STESSO codice che la fase Dash usa per eseguirle (issue #140).
 *
 * Il test vive a livello di `PlanBots` — non della libreria — perche' e' li' che la coerenza si puo' rompere:
 * la portata letta dal catalogo, l'instradamento per stile di movimento, l'insieme dei nemici. Un test che
 * confronta il predicato con la funzione su cui e' definito non potrebbe accorgersi di un refuso in nessuna
 * di quelle tre righe.
 *
 * Lo scenario e' quello che PRIMA del consolidamento faceva divergere i due strati: un corridoio di acqua
 * bassa (costo 2). Il vecchio filtro del bot contava il COSTO del terreno, il resolver conta le CELLE — e il
 * catalogo azioni dice che le mobilita' lineari non si riducono col terreno. Su questa mappa il bot si
 * negava scatti che il resolver avrebbe eseguito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotDashAgreesWithResolverTest,
	"RefactorTactics.HexBotPlay.DashPlanIsExecutableOnCostlyTerrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotDashAgreesWithResolverTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBotWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	URTHexMapAsset* Map = SpawnHexBotMap(World, /*Radius=*/ 5);

	// Acqua bassa (costo 2) su tutta la mappa tranne il bordo: qualunque direzione lo scatto prenda, il
	// terreno e' caro. E' cio' che discrimina "portata in celle" da "portata in punti movimento".
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 4))
	{
		FRTHexCellData Water(Id);
		Water.Surface = ERTHexSurface::ShallowWater;
		Water.MoveCost = 2;
		Map->AddOrUpdateCell(Water);
	}
	Map->SortCells();

	// Un kiter con un nemico ADDOSSO fugge, e la fuga passa dallo scatto. E' lo scenario che mette davvero in
	// moto il ramo che questo test deve coprire (col nemico lontano il bot spara e basta).
	ARTUnit* Bot = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeRiva(), FRTCellId(0, 0), /*bBot*/ true);

	// Lo standoff non si dichiara piu' qui: il bot lo DERIVA dalla portata dell'attacco base, e Riva —
	// `PressureJet`, portata 5 — e' l'unica kiter del roster. Lo scenario e' «il kiter fugge», quindi senza
	// un'unita' che il kiting lo produca davvero il bot resterebbe fermo e questa guardia non guarderebbe
	// niente. La premessa qui sotto lo verifica invece di darlo per scontato.
	// ADDOSSO davvero: a distanza 1 la violazione dello standoff e' massima, e la ritirata immediata scatta
	// (soglia: meta' standoff). Stava a 2, che con lo standoff 4 del Ranger legacy era dentro la soglia;
	// con i 3 di Riva non lo e' piu', e il bot si sarebbe limitato a riguadagnare UNA cella — comportamento
	// corretto, ma non lo scenario che questo test deve coprire.
	ARTUnit* Foe = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(1, 0), /*bBot*/ false);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Bot || !Foe) { DestroyHexBotWorld(World); return false; }

	// Lo scenario chiede un'unita' che sia kiter E abbia uno scatto, e nel roster v0.1 nessuno e' entrambe
	// le cose: Riva e' l'unica kiter (portata 5) ma il suo kit non ha mobilita' rapide, Vektor ha
	// `PassingBlade` ma con portata 4 non e' kiter. Lo scatto glielo da' il test, dal catalogo GENERICO —
	// non e' un numero inventato, e' `Action.Dash` cosi' come lo spedisce il gioco.
	URTActionData* Sprint = NewObject<URTActionData>(Bot);
	Sprint->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Dash"));
	Sprint->RangeCells = Sprint->Def.RangeCells;
	Sprint->Power = 0;
	Bot->Abilities.Add(Sprint);

	// Lo scatto dichiara di essere LINEARE: e' il ramo che il consolidamento ha unificato. La linearita' si
	// VERIFICA sul dato del catalogo, non si impone qui: imponendola (come faceva questo test fino a #142)
	// la guardia sarebbe rimasta verde anche se lo scatto fosse tornato senza stile.
	const int32 DashIdx = Bot->FindDashAbilityIndex();
	if (!TestTrue(TEXT("premessa: chi fugge ha una mobilita' rapida"), DashIdx != INDEX_NONE))
	{
		DestroyHexBotWorld(World);
		return false;
	}
	URTActionData* DashAb = Bot->GetAbility(DashIdx);
	if (!TestTrue(TEXT("premessa: lo scatto e' lineare"),
		DashAb && URTMovementActionLibrary::IsLinear(DashAb->Def.MovementStyle)))
	{
		DestroyHexBotWorld(World);
		return false;
	}

	TM->PlanBotsForTest();

	// Se il bot non scatta, il test non prova nulla: e' un fallimento dello SCENARIO, non un successo.
	// Meglio accorgersene subito che scoprire fra sei mesi che questa guardia non guardava niente.
	if (!TestTrue(TEXT("lo scenario mette davvero in moto uno scatto del bot"),
		Bot->PlannedDashAbility != INDEX_NONE))
	{
		DestroyHexBotWorld(World);
		return false;
	}

	// Il piano del bot deve essere ESEGUIBILE: e' la definizione operativa di "i due strati concordano".
	TArray<ARTUnit*> Units;
	FRTHexSnapshot Snapshot = TM->MakeCurrentSnapshot(Units);
	const int32 BotIdx = Units.IndexOfByKey(Bot);
	if (!TestTrue(TEXT("il bot e' nello snapshot"), BotIdx != INDEX_NONE))
	{
		DestroyHexBotWorld(World);
		return false;
	}

	TSet<int32> Hostiles;
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (Units[i] && Units[i]->TeamId != Bot->TeamId) { Hostiles.Add(i); }
	}

	const int32 Declared = DashAb->Def.ActionId.IsNone() ? DashAb->RangeCells : DashAb->Def.RangeCells;
	const int32 Range = Bot->GetEffectiveDashRange(Declared);
	const FRTLinearMoveResult Resolved = URTMovementActionLibrary::ResolveLinearMove(
		Snapshot.Map, Bot->Cell, Bot->PlannedDashCell, Range,
		DashAb->Def.MovementStyle, Snapshot.Occupancy, Hostiles);

	TestTrue(TEXT("lo scatto pianificato dal bot arriva dove il bot lo ha pianificato"),
		Resolved.Final == Bot->PlannedDashCell);

	// E deve poterlo pianificare LONTANO quanto il resolver lo porterebbe. E' la parte che discrimina: le
	// candidate nascono da un Dijkstra sui punti movimento, quindi finche' il budget dello scatto vi finiva
	// dentro come se fossero MP, su acqua (costo 2) il bot non vedeva nulla oltre meta' portata — e nessun
	// filtro puo' recuperare una candidata che non e' mai nata. Il kiter fugge il piu' lontano possibile:
	// se la distanza si ferma a portata/costo, la troncatura e' tornata.
	const int32 Fled = URTHexLibrary::HexDistance(Bot->Cell, Bot->PlannedDashCell);
	AddInfo(FString::Printf(TEXT("fuga di %d celle su portata %d (terreno costo 2)"), Fled, Range));
	TestTrue(TEXT("la fuga non e' troncata dal costo del terreno"), Fled > Range / 2);

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

	// Unita' ferita sotto meta' HP: usa un'abilita' self-target invece di attaccare.
	ARTUnit* Hurt = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(0, 0), /*bBot*/ true);
	ARTUnit* Foe = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(2, 0), /*bBot*/ false);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Hurt || !Foe) { DestroyHexBotWorld(World); return false; }

	// Il supporto su se stessi lo dichiara il TEST, non l'eroe: `bSelfTarget` non e' impostato da nessuna
	// azione del roster — era `Guardian.Barrier`, sparita con gli archetipi legacy. La proprieta' in esame e'
	// la SCELTA DEL BOT («se sono ferito e ho un supporto su di me, lo uso invece di attaccare»), non chi
	// possiede quell'azione: senza darne una all'unita' il ramo non e' raggiungibile e resterebbe senza
	// verifica — e' l'unica che ha. Vedi #425 per la decisione su chi debba dichiararlo nel roster.
	// Deve CURARE o schermare, non solo essere su di se': il ramo del bot chiede un effetto `Heal`/`Shield`.
	// Costruita su `Action.Guard` per fase e slot — quello che le manca, e che qui conta, e' lo scudo:
	// `Guard` applica uno stato difensivo e non rimette in piedi nessuno, quindi da sola non basta piu'.
	// E' la stessa forma di `Guardian.Barrier`, l'azione per cui questo ramo era stato scritto: +40 scudo.
	URTActionData* SelfSupport = NewObject<URTActionData>(Hurt);
	SelfSupport->DisplayName = FText::FromString(TEXT("Barriera di prova"));
	SelfSupport->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Guard"));
	SelfSupport->Def.ActionId = FName(TEXT("Test.SelfSupport"));
	SelfSupport->Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Shield, 40));
	SelfSupport->bSelfTarget = true;
	SelfSupport->RangeCells = 0;
	SelfSupport->Power = 0;
	Hurt->Abilities.Add(SelfSupport);

	Hurt->ApplyCombatState(/*Health*/ 20, /*Shield*/ 0); // ben sotto meta' dei suoi HP massimi

	TM->PlanBotsForTest();

	const URTActionData* Planned = Hurt->GetAbility(Hurt->PlannedAbilityIndex);
	TestTrue(TEXT("pianifica l'abilita' che lo rimette in piedi, non una qualunque su di se'"),
		Planned && Planned->Def.ActionId == FName(TEXT("Test.SelfSupport")));
	TestNull(TEXT("non pianifica un attacco nello stesso turno"), Hurt->PlannedAttackTarget.Get());

	DestroyHexBotWorld(World);
	return true;
}

/**
 * La fuga del kiter, in partita. Torna dopo la rimozione degli archetipi legacy (#426): l'unita' non e' piu'
 * il `Ranger` con `KiteStandoff = 4` scritto addosso, ma **Riva**, che lo standoff se lo guadagna dalla
 * portata del proprio attacco base — `PressureJet` tira a 5, e `DeriveKiteStandoff` ne fa 3.
 *
 * E' la differenza che conta: prima il comportamento esisteva perche' un archetipo lo dichiarava, adesso
 * perche' un eroe del roster ha i numeri per produrlo. Se un domani nessuno li avesse piu', questo test
 * diventerebbe rosso invece di restare verde su un'unita' inventata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotPanicTest,
	"RefactorTactics.HexBotPlay.KiterFleesWhenThreatened",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotPanicTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBotWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBotMap(World, /*Radius=*/ 6);

	ARTUnit* Kiter = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeRiva(), FRTCellId(0, 0), /*bBot*/ true);

	// La minaccia si posiziona a meta' dello standoff DERIVATO, che e' la soglia della ritirata immediata.
	// Era a distanza 2, calcolata sullo standoff 4 del Ranger legacy: Riva ne ha 3, quindi la soglia e' 1 e
	// a distanza 2 il panico non sarebbe scattato — il test avrebbe misurato un bot che sta fermo.
	const int32 Standoff = URTHexBotLibrary::DeriveKiteStandoff(Kiter->AttackRange);
	if (!TestTrue(TEXT("premessa: chi fugge e' un kiter (standoff derivato > 0)"), Standoff > 0))
	{
		DestroyHexBotWorld(World);
		return false;
	}
	ARTUnit* Melee = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeBastion(),
		FRTCellId(FMath::Max(1, Standoff / 2), 0), /*bBot*/ false);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Melee) { DestroyHexBotWorld(World); return false; }

	TestTrue(TEXT("premessa: la minaccia e' entro meta' standoff"),
		URTHexLibrary::HexDistance(Kiter->Cell, Melee->Cell) <= Standoff / 2);

	TM->PlanBotsForTest();

	// La fuga puo' avvenire col movimento normale o con lo scatto difensivo: conta il risultato.
	const FRTCellId Escape = Kiter->PlannedDashAbility != INDEX_NONE ? Kiter->PlannedDashCell : Kiter->PlannedCell;
	TestTrue(TEXT("il kiter si allontana dalla minaccia"),
		URTHexLibrary::HexDistance(Escape, Melee->Cell) > URTHexLibrary::HexDistance(Kiter->Cell, Melee->Cell));

	DestroyHexBotWorld(World);
	return true;
}
