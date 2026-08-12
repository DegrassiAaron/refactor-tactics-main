#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Mondo minimo per spawnare un actor (nomi distinti per file: unity build). */
	UWorld* MakeMapActorWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyMapActorWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		if (GEngine)
		{
			GEngine->DestroyWorldContext(World);
		}
		World->DestroyWorld(false);
	}

	URTHexMapAsset* MakeActorTestAsset(int32 Radius, int32 Layer = 0)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, Layer), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}

	/** Spawna l'actor con l'asset gia' assegnato, cosi' OnConstruction lo vede (come al caricamento del livello). */
	ARTHexMapActor* SpawnMapActor(UWorld* World, URTHexMapAsset* Asset, int32 ActiveLayer = 0,
		ERTLayerViewMode View = ERTLayerViewMode::AllLayers)
	{
		ARTHexMapActor* Actor = World->SpawnActorDeferred<ARTHexMapActor>(
			ARTHexMapActor::StaticClass(), FTransform::Identity);
		if (!Actor)
		{
			return nullptr;
		}
		Actor->MapAsset = Asset;
		Actor->ActiveLayer = ActiveLayer;
		Actor->LayerView = View;
		Actor->FinishSpawning(FTransform::Identity);
		return Actor;
	}
}

// La vista deve rigenerarsi da sola alla costruzione dell'actor: senza questo, riaprendo il livello la griglia
// non viene ridisegnata e la mappa istanza->cella resta vuota (il click nel viewport non trova piu' le celle).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorRebuildOnConstructionTest,
	"RefactorTactics.HexMapActor.RebuildsOnConstruction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorRebuildOnConstructionTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	URTHexMapAsset* Asset = MakeActorTestAsset(/*Radius*/ 1); // 7 celle
	ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
	TestNotNull(TEXT("actor spawnato"), Actor);

	if (Actor)
	{
		TestEqual(TEXT("le 7 celle dell'asset sono rappresentate senza chiamare RebuildInstances"),
			Actor->NumInstanceCells(), 7);

		// La mappa istanza -> cella e' popolata: e' cio' che serve al raycast di selezione.
		const FRTCellId First = Actor->CellForInstance(0);
		TestTrue(TEXT("la prima istanza corrisponde a una cella dell'asset"), Asset->ContainsCell(First));
	}

	DestroyMapActorWorld(World);
	return true;
}

// Il filtro di layer va applicato dalla ricostruzione automatica, non solo dal pulsante manuale.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorLayerFilterTest,
	"RefactorTactics.HexMapActor.LayerFilterOnConstruction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorLayerFilterTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	// 7 celle sul layer 0 + 7 sul layer 1.
	URTHexMapAsset* Asset = MakeActorTestAsset(/*Radius*/ 1, /*Layer*/ 0);
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 1), 1))
	{
		Asset->AddOrUpdateCell(FRTHexCellData(Id));
	}
	Asset->SortCells();

	if (ARTHexMapActor* All = SpawnMapActor(World, Asset, /*ActiveLayer*/ 0, ERTLayerViewMode::AllLayers))
	{
		TestEqual(TEXT("AllLayers rappresenta entrambi i piani"), All->NumInstanceCells(), 14);
	}

	if (ARTHexMapActor* Active = SpawnMapActor(World, Asset, /*ActiveLayer*/ 1, ERTLayerViewMode::ActiveOnly))
	{
		TestEqual(TEXT("ActiveOnly rappresenta solo il layer attivo"), Active->NumInstanceCells(), 7);
		bool bAllOnActiveLayer = Active->NumInstanceCells() > 0;
		for (int32 I = 0; I < Active->NumInstanceCells(); ++I)
		{
			bAllOnActiveLayer &= Active->CellForInstance(I).Layer == 1;
		}
		TestTrue(TEXT("tutte le celle rappresentate stanno sul layer attivo"), bAllOnActiveLayer);
	}

	// Focus mostra i piani vicini come CONTORNO, non come istanze: se i piani di contesto diventassero istanze
	// finirebbero in `Cells`, che e' l'unico ISM con collisione, e il raycast del pennello potrebbe agganciarli
	// dipingendo su un piano diverso da quello attivo. L'invariante e' quindi
	// «Focus istanzia esattamente quanto ActiveOnly», e va verificata qui perche' il disegno del contorno non
	// e' osservabile senza schermo.
	if (ARTHexMapActor* Focus = SpawnMapActor(World, Asset, /*ActiveLayer*/ 1, ERTLayerViewMode::Focus))
	{
		TestEqual(TEXT("Focus istanzia solo il layer attivo, come ActiveOnly"), Focus->NumInstanceCells(), 7);
		bool bAllOnActiveLayer = Focus->NumInstanceCells() > 0;
		for (int32 I = 0; I < Focus->NumInstanceCells(); ++I)
		{
			bAllOnActiveLayer &= Focus->CellForInstance(I).Layer == 1;
		}
		TestTrue(TEXT("nessuna istanza appartiene a un piano di contesto"), bAllOnActiveLayer);
	}

	// GhostLayerRange e' presentazione pura: cambiarlo non deve spostare una sola istanza, altrimenti il
	// parametro che regola quanto contesto si vede finirebbe per decidere anche cosa e' cliccabile.
	if (ARTHexMapActor* Wide = SpawnMapActor(World, Asset, /*ActiveLayer*/ 1, ERTLayerViewMode::Focus))
	{
		Wide->GhostLayerRange = 8;
		Wide->RebuildInstances();
		TestEqual(TEXT("GhostLayerRange non cambia le istanze"), Wide->NumInstanceCells(), 7);
	}

	DestroyMapActorWorld(World);
	return true;
}

/**
 * CP 6.3: il contesto geometrico deve avere UNA sola definizione, perche' resolver, playback e input non
 * possono divergere di scala. Regola: l'ASSET e' autorevole sulla scala (HexSize/LayerHeight), l'ACTOR sulla
 * posizione (origine); senza asset valgono i valori dell'actor (graybox demo).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorContextTest,
	"RefactorTactics.HexMapActor.HexContextAssetIsAuthoritativeOnScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorContextTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	// Asset con scala DIVERSA dai default dell'actor: cosi' si vede chi vince.
	URTHexMapAsset* Asset = MakeActorTestAsset(/*Radius*/ 1);
	Asset->HexSize = 140.f;
	Asset->LayerHeight = 300.f;

	const FVector Where(1500.0, -250.0, 75.0);
	ARTHexMapActor* Actor = World->SpawnActorDeferred<ARTHexMapActor>(
		ARTHexMapActor::StaticClass(), FTransform(Where));
	TestNotNull(TEXT("actor spawnato"), Actor);
	if (!Actor) { DestroyMapActorWorld(World); return false; }
	Actor->MapAsset = Asset;
	Actor->HexSize = 100.f;      // valori dell'actor, che l'asset deve sovrascrivere
	Actor->LayerHeight = 250.f;
	Actor->FinishSpawning(FTransform(Where));

	TestTrue(TEXT("la mappa del livello si trova da sola"), ARTHexMapActor::FindInWorld(World) == Actor);

	FVector Origin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerHeight = 0.f;
	const URTHexMapAsset* Map = Actor->GetHexContext(Origin, HexSize, LayerHeight);

	TestTrue(TEXT("ritorna l'asset autorevole"), Map == Asset);
	TestTrue(TEXT("origine = posizione dell'actor"), Origin.Equals(Where, 0.01));
	TestEqual(TEXT("HexSize dall'asset, non dall'actor"), HexSize, 140.f);
	TestEqual(TEXT("LayerHeight dall'asset, non dall'actor"), LayerHeight, 300.f);

	// Senza asset: la scala e' quella dell'actor e non c'e' mappa autorevole.
	Actor->MapAsset = nullptr;
	const URTHexMapAsset* NoMap = Actor->GetHexContext(Origin, HexSize, LayerHeight);
	TestNull(TEXT("nessun asset -> nessuna mappa"), NoMap);
	TestEqual(TEXT("fallback: HexSize dell'actor"), HexSize, 100.f);
	TestEqual(TEXT("fallback: LayerHeight dell'actor"), LayerHeight, 250.f);

	DestroyMapActorWorld(World);
	return true;
}

/**
 * Il pennello deve dipingere dove si clicca, e la geometria di LETTURA non deve poterlo dirottare.
 *
 * L'actor ha piu' di un `UInstancedStaticMeshComponent` — `Cells`, selezionabile, e `Relief`, che mostra il
 * costo — e il raycast dell'editor riceve l'indice di istanza del componente EFFETTIVAMENTE colpito.
 * Risolvere quell'indice contro le celle di `Cells` senza prima verificare *cosa* e' stato colpito non
 * produce un crash: produce una cella **valida e sbagliata**, cioe' il pennello che dipinge altrove senza un
 * solo errore a log.
 *
 * La difesa non puo' essere «ricordarsi di mettere NoCollision» sulla prossima geometria: e' una promessa a
 * un umano. Questa regola la rende strutturale, perche' guarda il COMPONENTE e non l'actor che lo contiene.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorPickTest,
	"RefactorTactics.HexMapActor.PickIgnoresGeometryThatIsNotTheGrid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorPickTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	ARTHexMapActor* Actor = SpawnMapActor(World, MakeActorTestAsset(/*Radius=*/ 1));
	if (!Actor)
	{
		AddError(TEXT("actor non spawnato"));
		DestroyMapActorWorld(World);
		return false;
	}

	// I componenti REALI dell'actor, non dei finti costruiti qui: e' cio' che rende questo un test sul gioco.
	// Se domani `Relief` sparisse o cambiasse nome, il test lo direbbe invece di continuare a passare.
	TArray<UInstancedStaticMeshComponent*> Isms;
	Actor->GetComponents(Isms);
	UInstancedStaticMeshComponent* Cells = nullptr;
	UInstancedStaticMeshComponent* Relief = nullptr;
	UInstancedStaticMeshComponent* Blockers = nullptr;
	UInstancedStaticMeshComponent* EdgeFeatures = nullptr;
	for (UInstancedStaticMeshComponent* Ism : Isms)
	{
		if (Ism->GetName() == TEXT("Cells")) { Cells = Ism; }
		else if (Ism->GetName() == TEXT("Relief")) { Relief = Ism; }
		else if (Ism->GetName() == TEXT("Blockers")) { Blockers = Ism; }
		else if (Ism->GetName() == TEXT("EdgeFeatures")) { EdgeFeatures = Ism; }
	}

	if (!Cells || !Relief || !Blockers || !EdgeFeatures)
	{
		AddError(TEXT("l'actor non ha tutti gli ISM attesi (Cells, Relief, Blockers, EdgeFeatures)"));
		DestroyMapActorWorld(World);
		return false;
	}

	TestTrue(TEXT("colpo su un'istanza della griglia"), Actor->IsPickOnSelectableCell(Cells, 0));

	// Il caso che da' valore alla regola, e l'unico che il confronto sull'ACTOR lascerebbe passare.
	TestFalse(TEXT("colpo sul rilievo del costo"), Actor->IsPickOnSelectableCell(Relief, 0));

	// Ogni geometria di lettura aggiunta va messa qui: la regola vale perche' e' strutturale, ma resta vera
	// solo finche' qualcuno verifica che il componente NUOVO non sia diventato l'eccezione.
	TestFalse(TEXT("colpo sui volumi di blocco"), Actor->IsPickOnSelectableCell(Blockers, 0));
	TestFalse(TEXT("colpo sui pannelli di bordo"), Actor->IsPickOnSelectableCell(EdgeFeatures, 0));

	// Un indice fuori range non e' teorico: `CellForInstance` risponde `(0,0,0)` — una cella VALIDA — a
	// qualunque indice, quindi senza questo controllo il click finirebbe sull'origine della mappa.
	TestFalse(TEXT("indice oltre il numero di istanze"),
		Actor->IsPickOnSelectableCell(Cells, Actor->NumInstanceCells()));
	TestFalse(TEXT("indice negativo"), Actor->IsPickOnSelectableCell(Cells, INDEX_NONE));

	TestFalse(TEXT("nessun colpo"), Actor->IsPickOnSelectableCell(nullptr, 0));

	DestroyMapActorWorld(World);
	return true;
}

/**
 * I volumi di blocco nascono dai FLAG della cella, uno per regola, e una cella che ne ha due li mostra
 * entrambi.
 *
 * E' la parte che una verifica a occhio non copre: guardando la mappa si vede *che* c'e' qualcosa, non
 * *quante* regole sta dicendo. Una cella con entrambi i flag che producesse un volume solo sarebbe
 * indistinguibile da una che ne ha uno — e sarebbe una vista che mente, il difetto che il brief mette in
 * testa (`brief-editor-map-viz.md` §1: «una vista che mente costa piu' di una vista che manca»).
 *
 * Il conteggio e' fatto per PROPORZIONE e non per indice: l'ordine delle istanze e' un dettaglio di
 * `RebuildInstances`, mentre «quante colonne e quante lastre» e' cio' che l'autore della mappa legge.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorBlockerInstancesTest,
	"RefactorTactics.HexMapActor.BlockerVolumesComeFromCellFlags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorBlockerInstancesTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();

	// Le quattro combinazioni possibili, una per cella: nessuna regola, solo vista, solo movimento, entrambe.
	URTHexMapAsset* Asset = NewObject<URTHexMapAsset>();
	FRTHexCellData Libera(FRTCellId(0, 0, 0));
	FRTHexCellData SoloVista(FRTCellId(1, 0, 0));
	SoloVista.bBlocksLineOfSight = true;
	FRTHexCellData SoloMovimento(FRTCellId(2, 0, 0));
	SoloMovimento.bBlocksMovement = true;
	FRTHexCellData Entrambi(FRTCellId(3, 0, 0));
	Entrambi.bBlocksLineOfSight = true;
	Entrambi.bBlocksMovement = true;
	Asset->AddOrUpdateCell(Libera);
	Asset->AddOrUpdateCell(SoloVista);
	Asset->AddOrUpdateCell(SoloMovimento);
	Asset->AddOrUpdateCell(Entrambi);
	Asset->SortCells();

	ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
	if (!Actor)
	{
		AddError(TEXT("actor non spawnato"));
		DestroyMapActorWorld(World);
		return false;
	}

	TArray<UInstancedStaticMeshComponent*> Isms;
	Actor->GetComponents(Isms);
	UInstancedStaticMeshComponent* Blockers = nullptr;
	for (UInstancedStaticMeshComponent* Ism : Isms)
	{
		if (Ism->GetName() == TEXT("Blockers")) { Blockers = Ism; }
	}
	if (!Blockers)
	{
		AddError(TEXT("l'actor non ha l'ISM `Blockers`"));
		DestroyMapActorWorld(World);
		return false;
	}

	// Due celle bloccano il movimento (`SoloMovimento`, `Entrambi`) e due la vista (`SoloVista`, `Entrambi`):
	// tre celle con almeno una regola, ma QUATTRO volumi. La cella libera non ne produce nessuno.
	TestEqual(TEXT("un volume per ogni regola dichiarata, non per ogni cella"),
		Blockers->GetInstanceCount(), 4);

	int32 Colonne = 0;
	int32 Lastre = 0;
	for (int32 I = 0; I < Blockers->GetInstanceCount(); ++I)
	{
		FTransform Xf;
		Blockers->GetInstanceTransform(I, Xf, /*bWorldSpace=*/ true);
		// La mezza-altezza del cilindro engine e' 50 uu, quindi la scala Z e' altezza/100 — la stessa
		// aritmetica che `RebuildInstances` usa per posarle.
		const double ScalaZ = Xf.GetScale3D().Z;
		if (FMath::IsNearlyEqual(ScalaZ, URTHexLibrary::MovementBlockerHeight / 100.f, 0.001)) { ++Colonne; }
		else if (FMath::IsNearlyEqual(ScalaZ, URTHexLibrary::SightBlockerHeight / 100.f, 0.001)) { ++Lastre; }
	}

	TestEqual(TEXT("due colonne: le celle che bloccano il movimento"), Colonne, 2);
	TestEqual(TEXT("due lastre: le celle che bloccano la vista"), Lastre, 2);

	// La cella libera resta libera: senza questo, un bug che mettesse un volume ovunque passerebbe i due
	// conteggi qui sopra solo perche' i numeri tornano.
	TestEqual(TEXT("nessun volume che non venga da un flag"), Colonne + Lastre, 4);

	DestroyMapActorWorld(World);
	return true;
}

/**
 * I pannelli di bordo nascono dagli array sparsi della cella, e stanno SUL LATO che il dato dichiara.
 *
 * La posizione e' l'unica cosa che conta davvero qui: una copertura ripara da un lato e non dall'altro,
 * quindi un pannello nel posto sbagliato non e' un difetto estetico — dice il contrario del vero. E' anche
 * l'errore piu' facile da fare in silenzio, perche' i sei lati si somigliano e nessuno si accorge di
 * guardare quello sbagliato finche' non perde un'unita' su una rotta che credeva coperta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorEdgePanelsTest,
	"RefactorTactics.HexMapActor.EdgePanelsSitOnTheDeclaredEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorEdgePanelsTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();

	// Una cella con due coperture su lati DIVERSI e una porta su un terzo: tre pannelli, tre lati.
	URTHexMapAsset* Asset = NewObject<URTHexMapAsset>();
	FRTHexCellData Cella(FRTCellId(0, 0, 0));
	Cella.Covers.Add(FRTHexCover(ERTHexDirection::E, ERTHexCoverType::Low));
	Cella.Covers.Add(FRTHexCover(ERTHexDirection::NW, ERTHexCoverType::High));
	Cella.Doors.Add(FRTHexDoor(ERTHexDirection::SW, ERTHexDoorState::Closed));
	Asset->AddOrUpdateCell(Cella);
	// Una seconda cella SENZA bordi: se i pannelli venissero disegnati per cella invece che per bordo, il
	// conteggio se ne accorgerebbe.
	Asset->AddOrUpdateCell(FRTHexCellData(FRTCellId(5, 0, 0)));
	Asset->SortCells();

	ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
	if (!Actor)
	{
		AddError(TEXT("actor non spawnato"));
		DestroyMapActorWorld(World);
		return false;
	}

	TArray<UInstancedStaticMeshComponent*> Isms;
	Actor->GetComponents(Isms);
	UInstancedStaticMeshComponent* EdgeFeatures = nullptr;
	for (UInstancedStaticMeshComponent* Ism : Isms)
	{
		if (Ism->GetName() == TEXT("EdgeFeatures")) { EdgeFeatures = Ism; }
	}
	if (!EdgeFeatures)
	{
		AddError(TEXT("l'actor non ha l'ISM `EdgeFeatures`"));
		DestroyMapActorWorld(World);
		return false;
	}

	TestEqual(TEXT("tre bordi dichiarati, tre pannelli"), EdgeFeatures->GetInstanceCount(), 3);

	// Ogni pannello deve stare sul punto che la libreria calcola per QUEL lato. Il confronto e' sul piano:
	// la quota la decide l'actor (segue il pavimento della cella), il lato lo decide la libreria.
	FVector Origin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerH = 0.f;
	Actor->GetHexContext(Origin, HexSize, LayerH);
	const TArray<ERTHexDirection> LatiAttesi = {
		ERTHexDirection::E, ERTHexDirection::NW, ERTHexDirection::SW
	};
	for (const ERTHexDirection Lato : LatiAttesi)
	{
		const FVector Atteso = URTHexLibrary::EdgeTransform(FRTCellId(0, 0, 0), Lato, Origin, HexSize, LayerH)
			.GetLocation();
		bool bTrovato = false;
		for (int32 I = 0; I < EdgeFeatures->GetInstanceCount(); ++I)
		{
			FTransform Xf;
			EdgeFeatures->GetInstanceTransform(I, Xf, /*bWorldSpace=*/ true);
			const FVector P = Xf.GetLocation();
			if (FMath::IsNearlyEqual(P.X, Atteso.X, 0.5) && FMath::IsNearlyEqual(P.Y, Atteso.Y, 0.5))
			{
				bTrovato = true;
				break;
			}
		}
		TestTrue(FString::Printf(TEXT("c'e' un pannello sul lato %d"), static_cast<int32>(Lato)), bTrovato);
	}

	DestroyMapActorWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
