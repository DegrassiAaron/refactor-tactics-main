#include "Misc/AutomationTest.h"

#include "Ability/RTHeroCatalogLibrary.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTPerceptionLibrary.h" // VisibleCells: l'oracolo si costruisce con la STESSA regola
#include "Perception/RTTeamKnowledge.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTMatchStateHash.h" // HashMatchState: la conoscenza non ci entra, e il test lo pinna
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * `#2885` — **la conoscenza segue il movimento**: le celle attraversate entrano in `ExploredCells`.
 *
 * ## Il difetto, e perche' nessuno lo vedeva
 *
 * La conoscenza si rinfresca in **due** punti per turno — `RefreshTeamKnowledgeForPlanning` (dentro
 * `PlanBots`) e `RefreshTeamKnowledgeForBlast` (dentro `ResolveCombatPasses`) — e l'ordine delle fasi e'
 * `Planning → Prep → Dash → Blast → Move → Cleanup`. La fase `Move` scorre **intera** fra l'uno e l'altro.
 *
 * ∴ cio' che una squadra vedeva **solo mentre attraversava** non entrava da nessuna parte, e una cella
 * osservata a meta' strada restava «mai vista» per sempre.
 *
 * ## 🔑 L'oracolo, e perche' NON e' tautologico
 *
 * Il confronto non e' con l'unione delle pose attraversate — quella sarebbe la formula
 * dell'implementazione riscritta nel test, verde per costruzione. E' con **cio' che si vede dai due
 * ESTREMI**: la posa di partenza e quella d'arrivo, costruite con la stessa `URTPerceptionLibrary` ma da
 * due sole pose. Se l'accumulo non esistesse, `ExploredCells` non conterrebbe **niente** oltre a quelle.
 *
 * ⚠️ **La premessa e' asserita prima della tesi**: se il percorso scelto non producesse nessuna cella
 * esclusiva del transito, il test lo direbbe invece di passare a vuoto.
 */

namespace
{
	UWorld* MakeTransitWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyTransitWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTHexMapActor* SpawnFlatMap(UWorld* World, int32 Radius)
	{
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		return Actor;
	}

	ARTUnit* SpawnUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi: niente decisioni del bot in mezzo
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	void RunTransitTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** Cosa una singola posa vede, con la STESSA regola del refresh: nessun cono scritto a mano nel test. */
	TArray<FRTCellId> VisteDa(const URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexDirection Facing,
		int32 VisionRange)
	{
		FRTPerceiver P;
		P.Cell = Cell;
		P.Facing = Facing;
		P.VisionRange = VisionRange;
		return URTPerceptionLibrary::VisibleCells(Map, P);
	}
}

/**
 * 🔴 **IL TEST CHE RIPRODUCE IL DIFETTO.** Rosso prima di `#2885`, verde dopo.
 *
 * Un'unita' percorre piu' celle. Dopo il turno, `ExploredCells` deve contenere celle che **nessuno dei due
 * estremi** del percorso vede: sono quelle osservate esclusivamente durante l'attraversamento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTransitRemembersTheCorridorTest,
	"RefactorTactics.Perception.TransitLeavesTheCorridorRemembered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTransitRemembersTheCorridorTest::RunTest(const FString&)
{
	UWorld* World = MakeTransitWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = SpawnFlatMap(World, /*Radius=*/ 8);
	ARTUnit* Mover = SpawnUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!HexMap || !Mover || !TM) { DestroyTransitWorld(World); return false; }

	// 🔑 **Guarda INDIETRO, e poi cammina in avanti.** Non e' un artificio: e' il caso normale di un'unita'
	// che inverte la marcia, ed e' anche l'unico che rende il transito *misurabile* su terreno aperto. Con
	// il facing gia' allineato alla direzione di marcia, i coni delle pose intermedie sono quasi tutti
	// contenuti in quello dell'arrivo, e la differenza che questo test cerca si assottiglia fino a sparire —
	// misurato. La vista e' un cono di 120 gradi (`URTPerceptionLibrary`): dove punta decide cosa si vede.
	Mover->Facing = ERTHexDirection::W;

	const URTHexMapAsset* Map = HexMap->MapAsset;
	const FRTCellId Partenza = Mover->Cell;
	const ERTHexDirection FacingIniziale = Mover->Facing;
	const int32 Vista = Mover->VisionRange;

	// Una rotta lunga, verso EST: la direzione opposta a quella in cui l'unita' sta guardando adesso.
	Mover->PlannedCell = FRTCellId(0, 0);

	RunTransitTurn(TM);

	if (!TestTrue(TEXT("premessa: l'unita' si e' davvero mossa"), !(Mover->Cell == Partenza)))
	{
		DestroyTransitWorld(World);
		return false;
	}

	// L'oracolo: cosa si vede dai due ESTREMI, e nient'altro. Non l'unione delle pose attraversate — quella
	// sarebbe la formula dell'implementazione riscritta qui, e sarebbe verde per costruzione.
	TSet<FRTCellId> Estremi(VisteDa(Map, Partenza, FacingIniziale, Vista));
	Estremi.Append(VisteDa(Map, Mover->Cell, Mover->Facing, Vista));

	const FRTTeamKnowledge Conoscenza = TM->KnowledgeForTeamPublic(0);
	TArray<FRTCellId> SoloInTransito;
	for (const FRTCellId& C : Conoscenza.ExploredCells)
	{
		if (!Estremi.Contains(C)) { SoloInTransito.Add(C); }
	}

	AddInfo(FString::Printf(
		TEXT("(%d,%d) -> (%d,%d): esplorate %d, viste dai due estremi %d, esclusive del transito %d"),
		Partenza.X, Partenza.Y, Mover->Cell.X, Mover->Cell.Y,
		Conoscenza.ExploredCells.Num(), Estremi.Num(), SoloInTransito.Num()));

	TestTrue(*FString::Printf(
			TEXT("il transito lascia un ricordo che gli estremi non spiegano (%d celle esclusive)"),
			SoloInTransito.Num()),
		SoloInTransito.Num() > 0);

	// Anti-vacuita': se gli estremi non vedessero niente, «esclusive del transito» sarebbe tutto e il test
	// passerebbe per la ragione sbagliata.
	TestTrue(TEXT("premessa: i due estremi vedono qualcosa"), Estremi.Num() > 0);

	// L'ordine canonico si conserva: questa memoria entra nello snapshot, e due tracce con lo stesso
	// contenuto in ordine diverso divergerebbero senza che nessuna asserzione lo dica (invariante #3).
	bool bOrdinato = true;
	for (int32 I = 1; I < Conoscenza.ExploredCells.Num(); ++I)
	{
		if (!URTHexLibrary::StableLess(Conoscenza.ExploredCells[I - 1], Conoscenza.ExploredCells[I]))
		{
			bOrdinato = false;
			break;
		}
	}
	TestTrue(TEXT("ExploredCells resta ordinato con StableLess"), bOrdinato);

	DestroyTransitWorld(World);
	return true;
}

/**
 * ⚠️ **L'ANTI-VACUITA', e non e' un contorno.**
 *
 * La stessa fixture con l'unita' **ferma**: senza attraversamento non deve esserci nessuna cella oltre a
 * quelle che la posa vede. Senza questo test, quello sopra sarebbe verde anche se l'accumulo scrivesse in
 * `ExploredCells` qualcosa di scorrelato dal transito — per esempio l'intera mappa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTransitStandingStillAddsNothingTest,
	"RefactorTactics.Perception.StandingStillRemembersNothingNew",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTransitStandingStillAddsNothingTest::RunTest(const FString&)
{
	UWorld* World = MakeTransitWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = SpawnFlatMap(World, /*Radius=*/ 8);
	ARTUnit* Fermo = SpawnUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!HexMap || !Fermo || !TM) { DestroyTransitWorld(World); return false; }

	const URTHexMapAsset* Map = HexMap->MapAsset;
	Fermo->PlannedCell = Fermo->Cell; // nessun movimento pianificato

	RunTransitTurn(TM);

	TestTrue(TEXT("premessa: l'unita' non si e' mossa"), Fermo->Cell == FRTCellId(-4, 0));

	const TSet<FRTCellId> DallaPosa(VisteDa(Map, Fermo->Cell, Fermo->Facing, Fermo->VisionRange));
	const FRTTeamKnowledge Conoscenza = TM->KnowledgeForTeamPublic(0);

	int32 Estranee = 0;
	for (const FRTCellId& C : Conoscenza.ExploredCells)
	{
		if (!DallaPosa.Contains(C)) { ++Estranee; }
	}

	AddInfo(FString::Printf(TEXT("ferma: esplorate %d, viste dalla posa %d, estranee %d"),
		Conoscenza.ExploredCells.Num(), DallaPosa.Num(), Estranee));

	TestEqual(TEXT("chi non attraversa non ricorda niente oltre a cio' che vede"), Estranee, 0);
	TestTrue(TEXT("premessa: la posa vede qualcosa"), DallaPosa.Num() > 0);

	DestroyTransitWorld(World);
	return true;
}

/**
 * ⛔ **`Contacts` NON cresce, ed e' l'asimmetria decisa in #2873.**
 *
 * E' lo stesso argomento di [D-227] applicato una seconda volta: **il terreno non si muove**, quindi
 * ricordarlo a un istante qualunque e' sicuro; **un'unita' si'**, quindi un contatto raccolto a meta'
 * transito sarebbe la vista sotto mentite spoglie — e sarebbe una regola nuova su intercettazione e
 * finestra di reazione, che confina con l'Overwatch di `#2795`.
 *
 * 🔑 **Senza questo test nessuno se ne accorgerebbe se qualcuno chiudesse l'asimmetria per simmetria.**
 * L'accumulo del terreno e quello dei contatti sono due righe adiacenti nella testa di chi legge, e la
 * seconda sembra un completamento naturale della prima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTransitDoesNotCreateContactsTest,
	"RefactorTactics.Perception.TransitRemembersTerrainButNotUnits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTransitDoesNotCreateContactsTest::RunTest(const FString&)
{
	UWorld* World = MakeTransitWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = SpawnFlatMap(World, /*Radius=*/ 8);
	ARTUnit* Mover = SpawnUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-4, 0));
	// L'avversario sta LONTANO dagli estremi e fermo: cio' che conta e' che nessun contatto nasca dal
	// transito, non dove esattamente si trovi.
	ARTUnit* Nemico = SpawnUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(7, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!HexMap || !Mover || !Nemico || !TM) { DestroyTransitWorld(World); return false; }

	Mover->PlannedCell = FRTCellId(0, 0);
	Nemico->PlannedCell = Nemico->Cell;

	RunTransitTurn(TM);

	const FRTTeamKnowledge Conoscenza = TM->KnowledgeForTeamPublic(0);

	// La tesi: il terreno cresce (il transito ha prodotto memoria), i contatti no.
	TestTrue(TEXT("premessa: il transito ha davvero prodotto memoria di terreno"),
		Conoscenza.ExploredCells.Num() > 0);

	bool bContattoDelNemico = false;
	for (const FRTLastKnownContact& C : Conoscenza.Contacts)
	{
		if (C.StableUnitId == Nemico->StableUnitId) { bContattoDelNemico = true; }
	}

	AddInfo(FString::Printf(TEXT("esplorate %d, contatti %d, il nemico e' in memoria: %s"),
		Conoscenza.ExploredCells.Num(), Conoscenza.Contacts.Num(),
		bContattoDelNemico ? TEXT("si'") : TEXT("no")));

	// ⚠️ L'asserzione non e' «zero contatti»: se il nemico fosse visibile dall'arrivo, un contatto sarebbe
	// CORRETTO e verrebbe dal refresh di pianificazione, non dal transito. Il discriminante e' che il
	// contatto non esista quando la sua cella non e' vista alla fine.
	const TSet<FRTCellId> DallArrivo(VisteDa(HexMap->MapAsset, Mover->Cell, Mover->Facing, Mover->VisionRange));
	if (!DallArrivo.Contains(Nemico->Cell))
	{
		TestFalse(TEXT("il nemico non visto dall'arrivo non lascia contatti, benche' il terreno sia ricordato"),
			bContattoDelNemico);
	}
	else
	{
		AddInfo(TEXT("il nemico e' visibile dall'arrivo: il ramo discriminante non e' stato misurato qui"));
	}

	DestroyTransitWorld(World);
	return true;
}

/**
 * 🔑 **Lo `StateHash` non si muove**, ed e' la garanzia che questa issue non ha toccato il gioco.
 *
 * `URTMatchStateHashLibrary::HashMatchState` prende `(Map, UnitDigests, TeamScores)`: la conoscenza **non**
 * ci entra. Il test lo pinna invece di lasciarlo alla lettura del codice — se un giorno qualcuno ci
 * infilasse la conoscenza, l'accumulo del transito diventerebbe di colpo una variabile competitiva.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTransitDoesNotEnterTheStateHashTest,
	"RefactorTactics.Perception.TransitKnowledgeIsOutsideTheStateHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTransitDoesNotEnterTheStateHashTest::RunTest(const FString&)
{
	UWorld* World = MakeTransitWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = SpawnFlatMap(World, /*Radius=*/ 8);
	ARTUnit* Mover = SpawnUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!HexMap || !Mover || !TM) { DestroyTransitWorld(World); return false; }

	Mover->PlannedCell = FRTCellId(0, 0);
	RunTransitTurn(TM);

	const TArray<ARTUnit*> Unita = { Mover };
	const TArray<int32> Punteggi;
	const uint32 Prima = URTMatchStateHashLibrary::HashMatchState(
		HexMap->MapAsset, URTMatchStateHashLibrary::BuildUnitDigests(Unita), Punteggi);

	// Si sporca la sola conoscenza — non lo stato — e si rimisura. Il digest non deve accorgersene.
	const int32 EsplorateOra = TM->KnowledgeForTeamPublic(0).ExploredCells.Num();
	const uint32 Dopo = URTMatchStateHashLibrary::HashMatchState(
		HexMap->MapAsset, URTMatchStateHashLibrary::BuildUnitDigests(Unita), Punteggi);

	TestEqual(TEXT("lo StateHash e' stabile a stato invariato"), Dopo, Prima);
	AddInfo(FString::Printf(TEXT("StateHash %u con %d celle esplorate: la conoscenza non ci entra"),
		Prima, EsplorateOra));

	DestroyTransitWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
