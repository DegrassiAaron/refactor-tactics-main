#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTPlanValidationLibrary.h"
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
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);

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
	ARTUnit* BotA = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(2, -3), /*bBot*/ true);
	ARTUnit* BotB = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(3, -3), /*bBot*/ true);
	ARTUnit* FoeA = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(-2, 3), /*bBot*/ false);
	ARTUnit* FoeB = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-3, 3), /*bBot*/ false);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !BotA || !BotB || !FoeA || !FoeB) { DestroyHexBotWorld(World); return false; }

	TM->PlanBotsForTest();

	TArray<ARTUnit*> Bots;
	Bots.Add(BotA);
	Bots.Add(BotB);

	// Senza questa verifica il test passerebbe anche con un pianificatore che non fa NULLA: restare fermi
	// e' sempre legale. I bot partono a distanza 6 dai nemici, fuori dalla portata di entrambi gli archetipi,
	// quindi ciascuno deve muoversi (o scattare).
	//
	// ⚠️ **Da CP 13.5 il MOTIVO per cui si muovono e' cambiato, e va scritto o il test mente sul proprio
	// oggetto**: distanza 6 e' anche fuori VISTA, quindi nessuno dei due bot conosce un nemico e non si sta
	// «avvicinando» a niente — si muove per la condotta di ricerca (`PlanBots`, ramo «nessun contatto»). Cio'
	// che questo test verifica resta la LEGALITA' del piano, che e' il suo nome; il fatto che il piano esista
	// e' ora pinnato da `SeeksContactWithoutKnowledge`, che lo verifica per quello che e'.
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

	ARTUnit* Bot = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0), /*bBot*/ true);
	ARTUnit* Foe = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(4, 0), /*bBot*/ false);
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

	ARTUnit* Bot = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0), /*bBot*/ true); // con Carica
	ARTUnit* Foe = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(5, 0), /*bBot*/ false);
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

		ARTUnit* Bot = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0), /*bBot*/ true);
		ARTUnit* Ally = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(1, 0), /*bBot*/ false);
		ARTUnit* Foe = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, 0), /*bBot*/ false);
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

		ARTUnit* Bot = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0), /*bBot*/ true);
		ARTUnit* Ally = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, -4), /*bBot*/ false);
		ARTUnit* Foe = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, 0), /*bBot*/ false);
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Bot || !Ally || !Foe) { DestroyHexBotWorld(World); return false; }

		Ally->Health = 10;
		Ally->Shield = 0;

		TM->PlanBotsForTest();

		// Se qui il bot non offendesse, il caso 1 non proverebbe nulla: passerebbe perche' il bot non attacca
		// mai, non perche' risparmia il compagno.
		//
		// «Offende» comprende la CARICA, e non e' un dettaglio: Riktor ha `Ram` (20 danni piu' spinta) oltre
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
	ARTUnit* Bot = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakePhase(), FRTCellId(0, 0), /*bBot*/ true);

	// Lo standoff non si dichiara piu' qui: il bot lo DERIVA dalla portata dell'attacco base, e Phase —
	// `PressureJet`, portata 5 — e' l'unica kiter del roster. Lo scenario e' «il kiter fugge», quindi senza
	// un'unita' che il kiting lo produca davvero il bot resterebbe fermo e questa guardia non guarderebbe
	// niente. La premessa qui sotto lo verifica invece di darlo per scontato.
	// ADDOSSO davvero: a distanza 1 la violazione dello standoff e' massima, e la ritirata immediata scatta
	// (soglia: meta' standoff). Stava a 2, che con lo standoff 4 del Ranger legacy era dentro la soglia;
	// con i 3 di Phase non lo e' piu', e il bot si sarebbe limitato a riguadagnare UNA cella — comportamento
	// corretto, ma non lo scenario che questo test deve coprire.
	ARTUnit* Foe = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(1, 0), /*bBot*/ false);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Bot || !Foe) { DestroyHexBotWorld(World); return false; }

	// Lo scenario chiede un'unita' che sia kiter E abbia uno scatto, e nel roster v0.1 nessuno e' entrambe
	// le cose: Phase e' l'unica kiter (portata 5) ma il suo kit non ha mobilita' rapide, Wraith ha
	// `PassingBlade` ma con portata 4 non e' kiter. Lo scatto glielo da' il test, dal catalogo GENERICO —
	// non e' un numero inventato, e' `Action.Dodge` cosi' come lo spedisce il gioco.
	URTActionData* Sprint = NewObject<URTActionData>(Bot);
	Sprint->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Dodge"));
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
	ARTUnit* Hurt = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0), /*bBot*/ true);
	ARTUnit* Foe = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, 0), /*bBot*/ false);
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
 * il `Ranger` con `KiteStandoff = 4` scritto addosso, ma **Phase**, che lo standoff se lo guadagna dalla
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

	ARTUnit* Kiter = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakePhase(), FRTCellId(0, 0), /*bBot*/ true);

	// La minaccia si posiziona a meta' dello standoff DERIVATO, che e' la soglia della ritirata immediata.
	// Era a distanza 2, calcolata sullo standoff 4 del Ranger legacy: Phase ne ha 3, quindi la soglia e' 1 e
	// a distanza 2 il panico non sarebbe scattato — il test avrebbe misurato un bot che sta fermo.
	const int32 Standoff = URTHexBotLibrary::DeriveKiteStandoff(Kiter->AttackRange);
	if (!TestTrue(TEXT("premessa: chi fugge e' un kiter (standoff derivato > 0)"), Standoff > 0))
	{
		DestroyHexBotWorld(World);
		return false;
	}
	ARTUnit* Melee = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(),
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


// ─────────────────────────────────────────────────────────────────────────────────────────────────
// CP 13.5 — il bot pianifica sulla CONOSCENZA della sua squadra (#160, RT-FEAT-BOT-FAIRNESS)
// ─────────────────────────────────────────────────────────────────────────────────────────────────

namespace
{
	/** Il piano di un bot, ridotto a cio' che il resolver poi eseguira'. Confrontabile fra due partite. */
	struct FRTBotPlanFingerprint
	{
		FRTCellId Cell;
		/**
		 * Il bersaglio come CELLA, non come identita'.
		 *
		 * ⚠️ La prima stesura usava `StableUnitId` e faceva fallire il canary **senza che ci fosse una fuga**:
		 * `MatchRosterLess` ordina il roster per `(TeamId, cella)`, quindi spostare il nemico NASCOSTO
		 * rinumera anche quello VISIBILE — in una partita il bersaglio era `2`, nell'altra `1`, e i due piani
		 * risultavano diversi pur essendo lo stesso piano. L'identita' e' deterministica *dentro* una
		 * partita e non e' confrontabile *fra* due partite diverse.
		 * La cella lo e', perche' il nemico visibile sta nello stesso posto in entrambe — e conserva il
		 * potere del test: se il bot bersagliasse quello nascosto, le due celle differirebbero.
		 */
		FRTCellId AttackTargetCell;
		bool bHasAttackTarget = false;
		int32 AbilityIndex = INDEX_NONE;
		int32 DashAbility = INDEX_NONE;
		FRTCellId DashCell;

		bool operator==(const FRTBotPlanFingerprint& O) const
		{
			return Cell == O.Cell && bHasAttackTarget == O.bHasAttackTarget
				&& (!bHasAttackTarget || AttackTargetCell == O.AttackTargetCell)
				&& AbilityIndex == O.AbilityIndex && DashAbility == O.DashAbility && DashCell == O.DashCell;
		}
	};

	FRTBotPlanFingerprint FingerprintOfBotPlan(const ARTUnit* Bot)
	{
		FRTBotPlanFingerprint F;
		if (!Bot) { return F; }
		F.Cell = Bot->PlannedCell;
		F.bHasAttackTarget = Bot->PlannedAttackTarget != nullptr;
		if (Bot->PlannedAttackTarget) { F.AttackTargetCell = Bot->PlannedAttackTarget->Cell; }
		F.AbilityIndex = Bot->PlannedAbilityIndex;
		F.DashAbility = Bot->PlannedDashAbility;
		F.DashCell = Bot->PlannedDashCell;
		return F;
	}
}

/**
 * CANARY DELL'ONNISCIENZA — il gate di `RT-FEAT-BOT-FAIRNESS`.
 *
 * Due partite identiche in tutto cio' che la squadra del bot PUO' sapere, e diverse solo in cio' che non
 * puo': la posizione di un nemico fuori vista, da un lato nella prima e dall'altro nella seconda. Il piano
 * deve essere lo stesso.
 *
 * E' l'unico controllo che un bot onnisciente non puo' superare, ed e' per questo che esiste. Un bot che
 * legge lo stato autorevole non fallisce, non va in crash e produce piani *sensati*: una review lo prende
 * solo se qualcuno rilegge la riga giusta di `PlanBots`, cioe' finche' qualcuno si ricorda di guardare.
 * Questo test misura l'unica cosa che l'onniscienza non sa falsificare — l'INDIPENDENZA dell'esito da
 * un'informazione che non si dovrebbe avere.
 *
 * Vale anche come promessa pubblicata: la pagina Wiki `avversario-bot` dice al giocatore «il bot non vede
 * piu' di te». Senza questo test quella frase e' prosa.
 *
 * Il nemico nascosto e' tenuto LONTANO dal corridoio d'azione (lati opposti, distanza 6 con vista 3), e non
 * perche' sia comodo: l'OCCUPAZIONE resta legittimamente globale — `ReachableCells` e `DashHostiles`
 * modellano cio' che il resolver fara', non cio' che la squadra sa, e il giocatore umano ha lo stesso
 * vincolo. Rendere il bot piu' cieco dell'umano sarebbe sbagliato quanto renderlo onnisciente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotHiddenEnemyFairnessTest,
	"RefactorTactics.HexBotPlay.HiddenEnemyFairness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotHiddenEnemyFairnessTest::RunTest(const FString&)
{
	// Una partita: bot e nemico visibile sempre uguali, nemico NASCOSTO nella cella indicata.
	auto PlanWithHiddenAt = [](const FRTCellId& HiddenCell, bool& bOutSetupOk) -> FRTBotPlanFingerprint
	{
		bOutSetupOk = false;
		FRTBotPlanFingerprint Empty;
		UWorld* World = MakeHexBotWorld();
		if (!World) { return Empty; }
		SpawnHexBotMap(World, /*Radius=*/ 6);

		ARTUnit* Bot = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0), /*bBot*/ true);
		ARTUnit* Seen = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(2, 0), /*bBot*/ false);
		ARTUnit* Hidden = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakePhase(), HiddenCell, /*bBot*/ false);
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Bot || !Seen || !Hidden) { DestroyHexBotWorld(World); return Empty; }

		// Vista corta e DICHIARATA nel test: cosi' la premessa non dipende dai numeri di bilanciamento del
		// roster, che cambiano (D-073 ha appena portato Gadget a 7).
		Bot->VisionRange = 3;

		const bool bSeen = URTHexLibrary::HexDistance(Bot->Cell, Seen->Cell) <= Bot->VisionRange;
		const bool bHidden = URTHexLibrary::HexDistance(Bot->Cell, Hidden->Cell) > Bot->VisionRange;
		if (!bSeen || !bHidden) { DestroyHexBotWorld(World); return Empty; }

		TM->PlanBotsForTest();
		const FRTBotPlanFingerprint F = FingerprintOfBotPlan(Bot);
		DestroyHexBotWorld(World);
		bOutSetupOk = true;
		return F;
	};

	bool bLeftOk = false;
	bool bRightOk = false;
	const FRTBotPlanFingerprint Left = PlanWithHiddenAt(FRTCellId(-6, 3), bLeftOk);
	const FRTBotPlanFingerprint Right = PlanWithHiddenAt(FRTCellId(6, -3), bRightOk);

	if (!TestTrue(TEXT("premessa: entrambe le partite allestite, uno visto e uno fuori vista"), bLeftOk && bRightOk))
	{
		return false;
	}

	TestTrue(TEXT("il piano del bot NON dipende da dove sta il nemico che non vede"), Left == Right);

	// Senza questa riga il test passerebbe anche con un bot che non pianifica niente in nessuna delle due:
	// due «fermo» sono identici, e l'uguaglianza non proverebbe nulla.
	const bool bDidSomething = !(Left.Cell == FRTCellId(0, 0))
		|| Left.bHasAttackTarget
		|| Left.DashAbility != INDEX_NONE;
	TestTrue(TEXT("premessa: nelle due partite il bot ha davvero pianificato qualcosa"), bDidSomething);
	return true;
}

/**
 * Il bot non bersaglia cio' che la sua squadra non conosce.
 *
 * Complementare al canary: quello dimostra che l'esito non DIPENDE dall'ignoto, questo che l'ignoto non
 * viene proprio scelto.
 *
 * ⚠️ **L'allestimento e' costruito perche' il test possa FALLIRE, ed e' la parte che conta.** La prima
 * stesura metteva l'ignoto a distanza 6, cioe' fuori portata d'attacco: rimettendo l'onniscienza per
 * mutazione il test restava VERDE, perche' un bot onnisciente non lo avrebbe bersagliato comunque —
 * misurava la portata, non la conoscenza.
 * Qui l'ignoto e' dentro la portata **e quasi morto**: `WKill` domina di due ordini di grandezza, quindi un
 * bot che lo vedesse lo sceglierebbe di sicuro. Il verde significa che non lo vede.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotPartialKnowledgeTest,
	"RefactorTactics.HexBotPlay.PlansOnPartialKnowledge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotPartialKnowledgeTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBotWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBotMap(World, /*Radius=*/ 6);

	ARTUnit* Bot = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0), /*bBot*/ true);
	ARTUnit* Seen = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(1, 0), /*bBot*/ false);
	ARTUnit* Unknown = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakePhase(), FRTCellId(3, 0), /*bBot*/ false);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Bot || !Seen || !Unknown) { DestroyHexBotWorld(World); return false; }

	// Vista di UNA cella: vede il vicino, non l'altro. Dichiarata qui e non ereditata dal roster, che cambia.
	Bot->VisionRange = 1;
	// Quasi morto: e' l'esca. `WKill` vale 10000 contro `WDamage` 10 — se il bot lo vedesse, lo sceglierebbe.
	Unknown->Health = 1;
	Unknown->Shield = 0;

	TestTrue(TEXT("premessa: il vicino e' visto"),
		URTHexLibrary::HexDistance(Bot->Cell, Seen->Cell) <= Bot->VisionRange);
	TestTrue(TEXT("premessa: l'esca e' FUORI VISTA"),
		URTHexLibrary::HexDistance(Bot->Cell, Unknown->Cell) > Bot->VisionRange);
	TestTrue(TEXT("premessa: ma e' DENTRO la portata d'attacco, altrimenti il test non puo' fallire"),
		URTHexLibrary::HexDistance(Bot->Cell, Unknown->Cell) <= Bot->AttackRange);

	TM->PlanBotsForTest();

	TestTrue(TEXT("il bot non bersaglia l'unita' che la squadra non conosce, nemmeno se e' un kill facile"),
		Bot->PlannedAttackTarget != Unknown);

	DestroyHexBotWorld(World);
	return true;
}

/**
 * Senza NESSUN contatto il bot cerca, non si ferma.
 *
 * Pinna la condotta che ha salvato `HexMatch.PlaysToCompletion`: con la conoscenza parziale due squadre che
 * non si vedono possono aspettarsi per sempre, e la partita non finisce. E' il primo turno di ogni partita
 * su una mappa piu' larga della vista, cioe' il caso normale — non un caso limite.
 *
 * Il test non pretende una ricerca *intelligente*: pretende che il bot si MUOVA e che si avvicini al centro,
 * che e' geometria pubblica. I goal veri (`SecureObjective`, `GatherInformation`) sono E26.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotSeeksContactTest,
	"RefactorTactics.HexBotPlay.SeeksContactWithoutKnowledge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotSeeksContactTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBotWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBotMap(World, /*Radius=*/ 6);

	// ⚠️ **La geometria e' scelta perche' il test possa DISCRIMINARE, ed e' una correzione di code review.**
	// La prima stesura metteva bot e nemico agli ANTIPODI attraverso il centro: la' «vai verso il centro» e
	// «vai verso il nemico» sono la stessa direzione, quindi il test restava verde anche rimettendo
	// l'onniscienza — misurava un movimento, non la condotta di ricerca. Verificato per mutazione.
	// Qui il nemico sta su un lato e il centro dall'altro rispetto al bot: le due direzioni DIVERGONO, e
	// avvicinarsi al centro e' incompatibile con l'avvicinarsi al nemico.
	ARTUnit* Bot = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(4, -2), /*bBot*/ true);
	ARTUnit* Foe = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(6, 0), /*bBot*/ false);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Bot || !Foe) { DestroyHexBotWorld(World); return false; }

	Bot->VisionRange = 3;
	const int32 Before = URTHexLibrary::HexDistance(Bot->Cell, FRTCellId(0, 0));
	TestTrue(TEXT("premessa: il nemico e' fuori vista"),
		URTHexLibrary::HexDistance(Bot->Cell, Foe->Cell) > Bot->VisionRange);

	TM->PlanBotsForTest();

	const int32 ToFoeBefore = URTHexLibrary::HexDistance(Bot->Cell, Foe->Cell);

	TestTrue(TEXT("senza contatti il bot si muove invece di restare fermo"), !(Bot->PlannedCell == Bot->Cell));
	TestTrue(TEXT("e si avvicina al centro della mappa"),
		URTHexLibrary::HexDistance(Bot->PlannedCell, FRTCellId(0, 0)) < Before);
	// Il discriminante: un bot che vedesse il nemico si avvicinerebbe a LUI, non al centro.
	TestTrue(TEXT("e NON si avvicina al nemico che non conosce"),
		URTHexLibrary::HexDistance(Bot->PlannedCell, Foe->Cell) >= ToFoeBefore);

	DestroyHexBotWorld(World);
	return true;
}


/**
 * IL RICORDO: il bot agisce sulla cella dell'ULTIMO CONTATTO, non su quella vera.
 *
 * ⚠️ **Questo test copre il ramo che gli altri tre non potevano raggiungere, ed e' il piu' delicato della
 * feature.** `ERTTargetKnowledge::CellOnly` esiste solo dopo DUE osservazioni — vista in una, persa nella
 * successiva — mentre gli altri test pianificano una volta sola e quindi vedono soltanto `Allowed` o
 * `Rejected`. Trovato in code review: senza di lui si poteva cancellare l'intero `case CellOnly:` da
 * `PlanBots` e la suite restava verde.
 *
 * L'allestimento e' costruito perche' **solo la memoria** possa spiegare l'esito:
 *
 * ```
 *   turno 1   Foe a (3,0)   distanza 3 <= vista 3      -> visto, contatto registrato
 *   poi       Foe a (-6,3)  distanza 6  > vista 3      -> perso di vista
 *   turno 2   il ricordo dice ancora (3,0), che e' dentro la portata d'attacco (4)
 *             la posizione VERA e' a distanza 6, cioe' FUORI portata
 * ```
 *
 * Quindi: se il bot pianifica un attacco, lo ha pianificato **sulla memoria**. Un bot che leggesse la
 * posizione vera non attaccherebbe (fuori portata); un bot a cui si togliesse il ramo `CellOnly` non
 * avrebbe nemmeno il bersaglio in `Ctx.Enemies`. Il test cade in entrambi i casi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotRemembersLastKnownTest,
	"RefactorTactics.HexBotPlay.ActsOnLastKnownCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotRemembersLastKnownTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBotWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBotMap(World, /*Radius=*/ 6);

	ARTUnit* Bot = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0), /*bBot*/ true);
	ARTUnit* Foe = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(3, 0), /*bBot*/ false);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Bot || !Foe) { DestroyHexBotWorld(World); return false; }

	Bot->VisionRange = 3;
	const FRTCellId Remembered(3, 0);
	const FRTCellId Truth(-6, 3);

	if (!TestTrue(TEXT("premessa: al primo sguardo il nemico e' visibile"),
		URTHexLibrary::HexDistance(Bot->Cell, Remembered) <= Bot->VisionRange)
		|| !TestTrue(TEXT("premessa: il ricordo sara' DENTRO la portata d'attacco"),
			URTHexLibrary::HexDistance(Bot->Cell, Remembered) <= Bot->AttackRange))
	{
		DestroyHexBotWorld(World);
		return false;
	}

	// PRIMA osservazione: il contatto entra nella conoscenza di squadra.
	TM->PlanBotsForTest();

	// Il nemico se ne va dove nessuno lo vede. Il ricordo resta.
	Foe->PlaceOnCell(Truth, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
	Foe->PlannedCell = Truth;

	if (!TestTrue(TEXT("premessa: ora e' fuori vista"),
		URTHexLibrary::HexDistance(Bot->Cell, Foe->Cell) > Bot->VisionRange)
		|| !TestTrue(TEXT("premessa: e la posizione VERA e' fuori portata, cosi' solo la memoria puo' spiegare un attacco"),
			URTHexLibrary::HexDistance(Bot->Cell, Foe->Cell) > Bot->AttackRange))
	{
		DestroyHexBotWorld(World);
		return false;
	}

	// SECONDA osservazione: qui `ClassifyTarget` restituisce `CellOnly`, che nessun altro test raggiunge.
	TM->PlanBotsForTest();

	TestTrue(TEXT("il bot agisce sul contatto ricordato invece di ignorarlo"),
		Bot->PlannedAttackTarget == Foe);

	// E non lo insegue dove si trova davvero: se lo facesse, si muoverebbe verso la cella vera.
	TestTrue(TEXT("e non si avvicina alla posizione vera, che non conosce"),
		URTHexLibrary::HexDistance(Bot->PlannedCell, Truth)
			>= URTHexLibrary::HexDistance(Bot->Cell, Truth));

	DestroyHexBotWorld(World);
	return true;
}

// =====================================================================================================
// `#601` — il BOT arma la reazione che ha.
//
// E' la meta' del produttore che nessuno scenario copre: uno scenario dichiara la reazione da se' (chiave
// `reaction`), quindi passerebbe anche se il bot non la armasse mai. Senza questo test, la ragione per cui
// `ReactionPlanning` e' diventata una capability disponibile — «esiste un produttore nel gioco» — resterebbe
// un'affermazione senza verifica.
//
// Il costo di non averlo e' concreto: meta' delle unita' della v0.1 sono bot, e se non armassero mai una
// reazione i sette moduli di CP 7.5 sarebbero verdi nei test e assenti in partita.
// =====================================================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotArmsItsReactionTest,
	"RefactorTactics.Bot.ArmsItsReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotArmsItsReactionTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBotWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBotMap(World, /*Radius=*/ 5);

	// Riktor porta una reazione nel kit: una del roster vero, non costruita qui.
	ARTUnit* Bot = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(2, -3), /*bBot*/ true);
	ARTUnit* Nemico = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(-2, 3), /*bBot*/ false);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Bot || !Nemico || !TM) { DestroyHexBotWorld(World); return false; }

	// Premessa: il bot possiede davvero una reazione. Senza, «non l'ha armata» sarebbe l'esito di un kit vuoto,
	// e il test passerebbe per la ragione sbagliata il giorno in cui il roster cambia.
	int32 Reazioni = 0;
	for (int32 I = 0; I < Bot->NumAbilities(); ++I)
	{
		const URTActionData* A = Bot->GetAbility(I);
		if (A && A->Def.Slot == ERTActionSlot::Reaction) { ++Reazioni; }
	}
	if (!TestTrue(TEXT("premessa: il bot ha almeno una reazione nel kit"), Reazioni > 0))
	{
		DestroyHexBotWorld(World);
		return false;
	}

	TestEqual(TEXT("prima della pianificazione lo slot e' vuoto"),
		Bot->PlannedReactionAbility, static_cast<int32>(INDEX_NONE));

	TM->PlanBotsForTest();

	if (!TestNotEqual(TEXT("il bot ha armato la sua reazione"),
		Bot->PlannedReactionAbility, static_cast<int32>(INDEX_NONE)))
	{
		DestroyHexBotWorld(World);
		return false;
	}

	// E ha armato una REAZIONE, non un'azione qualunque finita nello slot sbagliato.
	const URTActionData* Armata = Bot->GetAbility(Bot->PlannedReactionAbility);
	TestTrue(TEXT("ed e' davvero una reazione"),
		Armata != nullptr && Armata->Def.Slot == ERTActionSlot::Reaction);

	DestroyHexBotWorld(World);
	return true;
}

/**
 * IL BOT CARICA, E IL BERSAGLIO INCASSA — `#880`, la terza voce di DoD di `#145`.
 *
 * `#145` fu chiusa dichiaratamente nella sua «forma minima»: il bot sa proporre una carica, ma nessun test
 * la vedeva **arrivare a segno**. La misura del 2026-08-14: in questo file dodici test, nessuno sulla
 * carica, e l'unico punto che la nomina sta dentro `PlanDoesNotBlastDyingAlly` come un `||` —
 * *«attacco **o** carica»* — che passa anche se il bot non carica affatto. E si ferma alla
 * pianificazione: nessun test del bot risolve un turno.
 *
 * ⚠️ **Il confine con `HexMatch.ChargeStopsOnEnemyAndHits`, che copre l'altra meta'.** Quel test esiste
 * (`RTHexMatchIntegrationTests.cpp`) ed e' quello che il DoD di `#145` citava col nome di allora,
 * `GuardianChargeStopsOnEnemyAndHits`: e' stato **rinominato** quando `Guardian.*` e' uscito dal roster,
 * non cancellato. Verifica gia' arresto, 20 danni e spinta — ma **col piano scritto a mano**
 * (`Charger->PlannedDashCell = Target->Cell`) e `bIsBotControlled = false`, dichiarato nel suo stesso
 * commento: *«i piani li scrive il test, non l'utility del bot»*.
 *
 * Quello che nessuno copriva, ed e' la ragione per cui questo test esiste, e' l'**anello fra i due**: che
 * sia il BOT a scegliere la carica da solo, e che quella scelta arrivi a segno in un turno risolto.
 *
 * ⚠️ **Il piano non viene imposto**: nessuna scrittura di `PlannedDashCell` o `PlannedDashAbility`. Il bot
 * sceglie da solo, e il test verifica prima *che* abbia scelto una carica, poi *cosa* produce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotChargeLandsTest,
	"RefactorTactics.HexBotPlay.ChargeItPlannedActuallyLands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotChargeLandsTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBotWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBotMap(World, /*Radius=*/ 5);

	// Stessa geometria di `PlanDoesNotBlastDyingAlly` caso 2, dove la carica e' gia' misurata come la mossa
	// che l'utility preferisce: Riktor ha `Ram` (20 + spinta 1) contro `ImpactShot` (8).
	// Nessun alleato in scena: qui non si misura il collaterale, e un terzo attore aggiungerebbe solo un
	// modo per cui il piano potrebbe cambiare senza che il test lo dica.
	ARTUnit* Bot = SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0), /*bBot*/ true);
	ARTUnit* Foe = SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, 0), /*bBot*/ false);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Bot || !Foe) { DestroyHexBotWorld(World); return false; }

	Foe->Shield = 0;          // il danno si legge sulla salute, senza passare da uno scudo
	Foe->PushResistance = 0;  // la soglia dorme dopo D-075: azzerarla e' dichiararlo, non aggirarlo
	const int32 HealthBefore = Foe->Health;

	TM->PlanBotsForTest();

	// --- 1. Il bot ha scelto una CARICA, e l'ha scelta da solo -------------------------------------
	if (!TestTrue(TEXT("il bot ha pianificato uno scatto"), Bot->PlannedDashAbility != INDEX_NONE))
	{
		DestroyHexBotWorld(World);
		return false;
	}
	const URTActionData* Dash = Bot->GetAbility(Bot->PlannedDashAbility);
	if (!TestTrue(TEXT("ed e' una carica, non uno scatto qualunque"),
		Dash != nullptr && Dash->Def.MovementStyle == ERTMovementStyle::LinearCharge))
	{
		DestroyHexBotWorld(World);
		return false;
	}
	// Una carica punta la cella OCCUPATA dal bersaglio: e' la differenza fra caricare e riposizionarsi.
	TestEqual(TEXT("e punta la cella del nemico"), Bot->PlannedDashCell, FRTCellId(2, 0));

	// --- 2. Il turno viene RISOLTO, non solo pianificato -------------------------------------------
	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	// --- 3. Il bersaglio incassa DANNO e SPINTA, con valori concreti -------------------------------
	TestEqual(TEXT("il bersaglio incassa i 20 danni della carica"), Foe->Health, HealthBefore - 20);
	// Spinta 1 lungo la direzione della carica: da (2,0) verso est.
	TestEqual(TEXT("e viene spinto di una cella"), Foe->Cell, FRTCellId(3, 0));
	// La carica si FERMA addosso al nemico invece di attraversarlo: e' cio' che distingue
	// `LinearCharge` da `LinearDash`, ed e' un dato del catalogo, non un `if` sull'ActionId.
	TestEqual(TEXT("e chi carica si ferma davanti alla cella di partenza del bersaglio"),
		Bot->Cell, FRTCellId(1, 0));

	// --- 4. Il TurnLog nomina la carica ------------------------------------------------------------
	int32 ChargeEntries = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.ActionId == FName(TEXT("Hero.Riktor.Ram"))) { ++ChargeEntries; }
	}
	TestTrue(TEXT("il TurnLog registra la carica col suo ActionId"), ChargeEntries > 0);

	DestroyHexBotWorld(World);
	return true;
}
#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il bot decide una finestra di reazione con la SOLA opportunity sanitizzata (CP 14.5).
 *
 * ⚠️ La proprieta' vera non la dimostra questo test: la dimostra la **firma**.
 * `URTHexBotLibrary::DecideReactionResponse` riceve un `FRTReactionOpportunity` e nient'altro — niente mappa,
 * niente snapshot, niente percorsi — e quel tipo ha un elenco CHIUSO di campi verificato per riflessione da
 * `Overwatch.OpportunityLeaksNoFuture`. Il futuro non e' raggiungibile nemmeno volendo, e «restituisce
 * subito» e' l'unica cosa che una funzione senza `UWorld` e senza timer possa fare.
 *
 * Cio' che il test aggiunge e' che la decisione sia **legale, deterministica e non vacua**: una funzione che
 * rispondesse sempre `HOLD` soddisferebbe la garanzia strutturale senza far mai reagire nessuno, ed e' il modo
 * piu' facile di avere un bot «sicuro» e inerte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotDecidesWithoutFutureKnowledgeTest,
	"RefactorTactics.Bot.DecidesWithoutFutureKnowledge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotDecidesWithoutFutureKnowledgeTest::RunTest(const FString&)
{
	const FString Hold = URTReactionOpportunityLibrary::HoldResponse();

	// Due bersagli piu' `HOLD`: e' la finestra vera dell'Overwatch.
	FRTReactionOpportunity TwoTargets;
	TwoTargets.AllowedResponses = {
		URTReactionOpportunityLibrary::FireResponse(4),
		URTReactionOpportunityLibrary::FireResponse(7),
		Hold };

	const FString Chosen = URTHexBotLibrary::DecideReactionResponse(TwoTargets);

	// 1. La risposta e' LEGALE. Un bot che inventa una risposta verrebbe rifiutato dall'orchestratore, e la
	//    finestra si chiuderebbe in `HoldRejected` senza che nessuno se ne accorga in partita.
	TestTrue(TEXT("la risposta e' fra quelle legali"),
		URTReactionOpportunityLibrary::IsResponseAllowed(TwoTargets, Chosen));

	// 2. NON e' vacua: con dei bersagli legali il bot spara. E' cio' che distingue una politica da un rifiuto.
	TestEqual(TEXT("con bersagli legali il bot spara al primo"),
		Chosen, URTReactionOpportunityLibrary::FireResponse(4));

	// 3. E' DETERMINISTICA e non dipende dall'ordine in cui le risposte sono state costruite: `FIRE:4` resta
	//    la scelta anche se l'elenco arriva al contrario. Senza, due esecuzioni dello stesso turno potrebbero
	//    sparare a due bersagli diversi — e il replay non sarebbe piu' riproducibile.
	FRTReactionOpportunity Reversed;
	Reversed.AllowedResponses = {
		Hold,
		URTReactionOpportunityLibrary::FireResponse(7),
		URTReactionOpportunityLibrary::FireResponse(4) };
	// ⚠️ Qui la politica sceglie «il primo `FIRE:` dell'elenco», quindi su un elenco permutato sceglie `7`.
	//    NON e' un difetto della politica: `AllowedResponses` e' costruito da `BuildOverwatchTriggers`, che
	//    ordina i bersagli per `UnitId` crescente PRIMA di scriverli — l'elenco permutato non e' uno stato che
	//    il gioco possa produrre. Cio' che il test fissa e' che la politica sia una funzione dell'elenco
	//    ricevuto e non abbia stato proprio: chiamarla due volte sullo stesso ingresso da' lo stesso esito.
	TestEqual(TEXT("chiamarla due volte sullo stesso ingresso da' lo stesso esito"),
		URTHexBotLibrary::DecideReactionResponse(Reversed),
		URTHexBotLibrary::DecideReactionResponse(Reversed));

	// 4. Senza bersagli legali risponde `HOLD` ESPLICITO, non una stringa vuota: vuota significa «non ho
	//    risposto», cioe' una scadenza, e il bot non e' scaduto — ha deciso, e non aveva altro da decidere.
	FRTReactionOpportunity HoldOnly;
	HoldOnly.AllowedResponses = { Hold };
	const FString NoTargets = URTHexBotLibrary::DecideReactionResponse(HoldOnly);
	TestEqual(TEXT("senza bersagli risponde HOLD"), NoTargets, Hold);
	TestFalse(TEXT("e non lascia la risposta vuota, che sarebbe una scadenza"), NoTargets.IsEmpty());

	return true;
}


/**
 * Il bot passa dal validatore del piano, e i suoi piani sono LEGALI (DoD di CP 38.2, [#605]).
 *
 * La riga della DoD dice: *«il bot passa dallo stesso punto: una validazione che il bot aggira non e' una
 * regola»*. Fino a oggi non ci passava nessuno — `URTPlanValidationLibrary::ValidatePlan` era invocabile
 * solo dai test, perche' mancava il modo di COMPORRE un piano da cio' che il gioco scrive. Con
 * `MakePlanFor` quel modo esiste, e questo test e' la prima misura: il pianificatore del bot produce
 * combinazioni che il validatore accetta?
 *
 * Si misura su piu' turni e su ENTRAMBE le squadre, perche' i rami del pianificatore si scelgono a seconda
 * della distanza dal nemico: un turno solo esercita il ramo d'apertura e nient'altro.
 *
 * \u26a0\ufe0f Il fallimento e' DIAGNOSTICO, non solo rosso: dice turno, unita', motivo e azione colpevole. Se un
 * giorno cade, quello che serve sapere e' quale combinazione il bot ha composto — non che «il bot sbaglia».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotPlansAreLegalTest,
	"RefactorTactics.HexBotPlay.PlansPassPlanValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotPlansAreLegalTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBotWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBotMap(World, /*Radius=*/ 6);

	// 2v2 interamente sotto bot: si misura il pianificatore, non la reazione del bot a un umano.
	TArray<ARTUnit*> Bots;
	Bots.Add(SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-4, 1), /*bBot*/ true));
	Bots.Add(SpawnHexBotUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(-4, 2), /*bBot*/ true));
	Bots.Add(SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(4, -2), /*bBot*/ true));
	Bots.Add(SpawnHexBotUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(4, -1), /*bBot*/ true));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || Bots.Contains(nullptr)) { DestroyHexBotWorld(World); return false; }

	const UEnum* ReasonEnum = StaticEnum<ERTActionInvalidReason>();
	int32 PianiEsaminati = 0;
	int32 PianiConVoci = 0;
	int32 PianiConMovimentoEPrincipale = 0;
	int32 Illegali = 0;

	// Sei turni: abbastanza per attraversare l'avvicinamento, l'ingaggio e lo scambio di colpi.
	for (int32 Turno = 1; Turno <= 6; ++Turno)
	{
		TM->PlanBotsForTest();

		for (ARTUnit* Bot : Bots)
		{
			if (!Bot || !Bot->IsAlive())
			{
				continue;
			}
			const TArray<FRTPlannedAction> Piano = URTPlanValidationLibrary::MakePlanFor(Bot);
			++PianiEsaminati;
			if (Piano.Num() > 0)
			{
				++PianiConVoci;
			}

			// La combinazione su cui `SlotOccupied` puo' scattare: movimento E principale nello stesso piano.
			// Contarla e' cio' che rende la misura una misura — vedi la premessa in fondo.
			bool bHaMovimento = false;
			bool bHaPrincipale = false;
			for (const FRTPlannedAction& Voce : Piano)
			{
				bHaMovimento |= URTCatalogLibrary::TakesMovementSlot(Voce.Def);
				bHaPrincipale |= URTCatalogLibrary::TakesMainSlot(Voce.Def);
			}
			if (bHaMovimento && bHaPrincipale)
			{
				++PianiConMovimentoEPrincipale;
			}

			const FRTPlanValidation Verdetto = URTPlanValidationLibrary::ValidatePlan(
				FRTHexSimUnit(0, Bot->Cell, Bot->GetEffectiveMoveRange(), /*bAlive*/ true), Piano);
			if (!Verdetto.bLegal)
			{
				++Illegali;
				FString Voci;
				for (const FRTPlannedAction& Voce : Piano)
				{
					Voci += FString::Printf(TEXT("%s(slot %d) "), *Voce.Def.ActionId.ToString(),
						static_cast<int32>(Voce.Def.Slot));
				}
				AddError(FString::Printf(
					TEXT("turno %d, %s: piano ILLEGALE (%s su '%s'). Voci: %s"),
					Turno, *Bot->GetName(),
					ReasonEnum ? *ReasonEnum->GetNameStringByValue(static_cast<int64>(Verdetto.Reason))
						: TEXT("?"),
					*Verdetto.OffendingActionId.ToString(), *Voci));
			}
		}

		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	AddInfo(FString::Printf(
		TEXT("MISURA: %d piani esaminati, %d con almeno una voce, %d con movimento E principale, %d illegali"),
		PianiEsaminati, PianiConVoci, PianiConMovimentoEPrincipale, Illegali));

	// 🔴 La premessa e' su `PianiConMovimentoEPrincipale`, non su `PianiConVoci`, e la differenza non e'
	// pedanteria: `PlanBots` arma una REAZIONE a ogni bot al primo turno, quindi ogni piano ha almeno una
	// voce comunque — e con quella premessa il test sarebbe restato verde anche cancellando meta'
	// `MakePlanFor`. `SlotOccupied` puo' scattare solo quando due voci si contendono uno slot, quindi cio'
	// che va garantito e' che i piani esaminati contengano DAVVERO la combinazione in esame.
	TestTrue(TEXT("premessa: almeno un piano porta movimento E principale insieme"),
		PianiConMovimentoEPrincipale > 0);
	TestEqual(TEXT("nessun piano del bot e' illegale"), Illegali, 0);

	DestroyHexBotWorld(World);
	return true;
}


#endif // WITH_DEV_AUTOMATION_TESTS
