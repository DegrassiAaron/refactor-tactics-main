#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Terrain/RTTerrainLibrary.h" // il costo di Rough arriva dal catalogo, non da un numero scritto qui

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

	// I due componenti REALI dell'actor, non due finti costruiti qui: e' cio' che rende questo un test sul
	// gioco. Se domani `Relief` sparisse o cambiasse nome, il test lo direbbe invece di continuare a passare.
	TArray<UInstancedStaticMeshComponent*> Isms;
	Actor->GetComponents(Isms);
	UInstancedStaticMeshComponent* Cells = nullptr;
	UInstancedStaticMeshComponent* Relief = nullptr;
	for (UInstancedStaticMeshComponent* Ism : Isms)
	{
		if (Ism->GetName() == TEXT("Cells")) { Cells = Ism; }
		else if (Ism->GetName() == TEXT("Relief")) { Relief = Ism; }
	}

	if (!Cells || !Relief)
	{
		AddError(TEXT("l'actor non ha entrambi gli ISM attesi (Cells, Relief)"));
		DestroyMapActorWorld(World);
		return false;
	}

	TestTrue(TEXT("colpo su un'istanza della griglia"), Actor->IsPickOnSelectableCell(Cells, 0));

	// Il caso che da' valore alla regola, e l'unico che il confronto sull'ACTOR lascerebbe passare.
	TestFalse(TEXT("colpo sul rilievo del costo"), Actor->IsPickOnSelectableCell(Relief, 0));

	// Un indice fuori range non e' teorico: `CellForInstance` risponde `(0,0,0)` — una cella VALIDA — a
	// qualunque indice, quindi senza questo controllo il click finirebbe sull'origine della mappa.
	TestFalse(TEXT("indice oltre il numero di istanze"),
		Actor->IsPickOnSelectableCell(Cells, Actor->NumInstanceCells()));
	TestFalse(TEXT("indice negativo"), Actor->IsPickOnSelectableCell(Cells, INDEX_NONE));

	TestFalse(TEXT("nessun colpo"), Actor->IsPickOnSelectableCell(nullptr, 0));

	DestroyMapActorWorld(World);
	return true;
}

namespace
{
	/** Le istanze di un ISM, in world space: quello che l'autore della mappa vede davvero. */
	TArray<FTransform> InstancesOf(const ARTHexMapActor* Actor, const TCHAR* ComponentName)
	{
		TArray<FTransform> Out;
		TArray<UInstancedStaticMeshComponent*> Isms;
		Actor->GetComponents(Isms);
		for (const UInstancedStaticMeshComponent* Ism : Isms)
		{
			if (Ism->GetName() != ComponentName) { continue; }
			for (int32 I = 0; I < Ism->GetInstanceCount(); ++I)
			{
				FTransform Xf;
				Ism->GetInstanceTransform(I, Xf, /*bWorldSpace=*/ true);
				Out.Add(Xf);
			}
		}
		return Out;
	}
}

/**
 * I volumi di blocco nascono dai FLAG della cella, uno per regola, e una cella che ne ha due li mostra
 * entrambi.
 *
 * E' la parte che una verifica a occhio non copre: guardando la mappa si vede *che* c'e' qualcosa, non
 * *quante* regole sta dicendo. Una cella con entrambi i flag che producesse un volume solo sarebbe
 * indistinguibile da una che ne ha uno, e sarebbe una vista che mente — il difetto che
 * `brief-editor-map-viz.md` §1 mette in testa.
 *
 * Il test non incide i valori: verifica le RELAZIONI. Ritoccare le altezze resta libero, farle collassare
 * no.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorBlockerVolumesTest,
	"RefactorTactics.HexMapActor.BlockerVolumesComeFromCellFlags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorBlockerVolumesTest::RunTest(const FString&)
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

	const TArray<FTransform> Volumi = InstancesOf(Actor, TEXT("Blockers"));

	// Tre celle con almeno una regola, ma QUATTRO volumi: la cella con entrambi i flag ne riceve due.
	TestEqual(TEXT("un volume per ogni regola dichiarata, non per ogni cella"), Volumi.Num(), 4);

	// Due forme e non una: le altezze distinte devono essere esattamente due, ciascuna usata due volte.
	// Il test non sa quanto sono alte — sa che sono diverse, ed e' cio' che rende leggibile la mappa.
	TArray<double> Altezze;
	for (const FTransform& Xf : Volumi)
	{
		const double H = Xf.GetScale3D().Z;
		bool bNota = false;
		for (const double A : Altezze) { bNota = bNota || FMath::IsNearlyEqual(A, H, 0.001); }
		if (!bNota) { Altezze.Add(H); }
	}
	TestEqual(TEXT("due regole, due altezze distinte"), Altezze.Num(), 2);

	if (Altezze.Num() == 2)
	{
		const double Alta = FMath::Max(Altezze[0], Altezze[1]);
		const double Bassa = FMath::Min(Altezze[0], Altezze[1]);
		int32 NAlte = 0, NBasse = 0;
		double LarghezzaAlta = 0.0, LarghezzaBassa = 0.0;
		for (const FTransform& Xf : Volumi)
		{
			if (FMath::IsNearlyEqual(Xf.GetScale3D().Z, Alta, 0.001))
			{
				++NAlte;
				LarghezzaAlta = Xf.GetScale3D().X;
			}
			else
			{
				++NBasse;
				LarghezzaBassa = Xf.GetScale3D().X;
			}
		}
		// Due celle bloccano il movimento e due la vista: i conti tornano solo se ogni flag produce il suo.
		TestEqual(TEXT("due volumi alti"), NAlte, 2);
		TestEqual(TEXT("due volumi bassi"), NBasse, 2);

		// «Forma per la regola»: dall'alto — che e' la vista di LAVORO — la sola altezza non si vede. Il
		// volume alto deve essere anche il piu' STRETTO, o a picco i due si somigliano.
		TestTrue(TEXT("il volume alto e' anche il piu' stretto: si distinguono anche a picco"),
			LarghezzaAlta < LarghezzaBassa);
	}

	DestroyMapActorWorld(World);
	return true;
}

/**
 * I pannelli di bordo stanno SUL LATO che il dato dichiara, e coperture alte e basse si distinguono.
 *
 * La posizione e' l'unica cosa che conta davvero: una copertura ripara da un lato e non dall'altro, quindi
 * un pannello sul lato sbagliato non e' un difetto estetico — dice il contrario del vero. Ed e' l'errore
 * piu' facile da fare in silenzio, perche' i sei lati si somigliano e nessuno se ne accorge finche' non
 * perde un'unita' su una rotta che credeva coperta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorEdgePanelsTest,
	"RefactorTactics.HexMapActor.EdgePanelsSitOnTheDeclaredEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorEdgePanelsTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();

	URTHexMapAsset* Asset = NewObject<URTHexMapAsset>();
	FRTHexCellData Cella(FRTCellId(0, 0, 0));
	Cella.Covers.Add(FRTHexCover(ERTHexDirection::E, ERTHexCoverType::Low));
	Cella.Covers.Add(FRTHexCover(ERTHexDirection::NW, ERTHexCoverType::High));
	Cella.Doors.Add(FRTHexDoor(ERTHexDirection::SW, ERTHexDoorState::Closed));
	Asset->AddOrUpdateCell(Cella);
	// Una cella SENZA bordi: se i pannelli venissero disegnati per cella invece che per bordo, il conteggio
	// se ne accorgerebbe.
	Asset->AddOrUpdateCell(FRTHexCellData(FRTCellId(5, 0, 0)));
	Asset->SortCells();

	ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
	if (!Actor)
	{
		AddError(TEXT("actor non spawnato"));
		DestroyMapActorWorld(World);
		return false;
	}

	const TArray<FTransform> Pannelli = InstancesOf(Actor, TEXT("EdgeFeatures"));
	TestEqual(TEXT("tre bordi dichiarati, tre pannelli"), Pannelli.Num(), 3);

	FVector Origin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerH = 0.f;
	Actor->GetHexContext(Origin, HexSize, LayerH);

	// Ogni pannello sul punto che la LIBRERIA calcola per quel lato: il test chiede la stessa cosa che
	// chiede il codice, cosi' se la convenzione dei sei lati cambiasse si muoverebbero insieme.
	auto AltezzaSulLato = [&](ERTHexDirection Lato, double& OutAltezza) -> bool
	{
		const FVector Atteso = URTHexLibrary::EdgeMidpointWorld(FRTCellId(0, 0, 0), Lato, Origin, HexSize,
			LayerH);
		for (const FTransform& Xf : Pannelli)
		{
			const FVector P = Xf.GetLocation();
			if (FMath::IsNearlyEqual(P.X, Atteso.X, 0.5) && FMath::IsNearlyEqual(P.Y, Atteso.Y, 0.5))
			{
				OutAltezza = Xf.GetScale3D().Z;
				return true;
			}
		}
		return false;
	};

	double CopBassa = 0.0, CopAlta = 0.0, Porta = 0.0;
	TestTrue(TEXT("c'e' un pannello sul lato E, dove sta la copertura bassa"),
		AltezzaSulLato(ERTHexDirection::E, CopBassa));
	TestTrue(TEXT("c'e' un pannello sul lato NW, dove sta la copertura alta"),
		AltezzaSulLato(ERTHexDirection::NW, CopAlta));
	TestTrue(TEXT("c'e' un pannello sul lato SW, dove sta la porta"),
		AltezzaSulLato(ERTHexDirection::SW, Porta));

	// Alta e bassa devono restare distinguibili: sono due regole diverse — l'alta NEGA l'attraversamento,
	// la bassa ripara e basta — e confonderle fa credere percorribile un bordo che non lo e'.
	TestTrue(TEXT("la copertura alta e' piu' alta della bassa"), CopAlta > CopBassa);

	DestroyMapActorWorld(World);
	return true;
}

/**
 * Una porta chiusa si vede chiusa, e una aperta si vede aperta.
 *
 * ⚠️ **Quattro stati, due forme, ed e' una scelta**: `RebuildInstances` mostra `Destroyed` come aperta —
 * *«e' terminale e non si richiude: si mostra come aperta, perche' e' cio' che e'»* — e `Locked` come
 * chiusa. Questo test pinna quella scelta invece di contestarla, e la rende esplicita a chi la rilegge.
 *
 * ⚠️ Ne segue un limite noto: l'acceptance di #553 chiedeva che *«lo stato di una porta sia visibile e
 * cambi quando cambia il dato»*, e passare da `Closed` a `Locked` **non cambia nulla di visibile**. Chi
 * dipinge non puo' sapere, guardando, se un varco si apre da solo. Serve un secondo canale — ingombro o
 * spessore — e non e' stato aggiunto qui perche' e' una decisione di presentazione, non un difetto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorDoorPanelsTest,
	"RefactorTactics.HexMapActor.DoorPanelsShowWhetherYouCanPass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorDoorPanelsTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();

	URTHexMapAsset* Asset = NewObject<URTHexMapAsset>();
	const TArray<ERTHexDoorState> Stati = {
		ERTHexDoorState::Open, ERTHexDoorState::Closed, ERTHexDoorState::Locked, ERTHexDoorState::Destroyed
	};
	for (int32 I = 0; I < Stati.Num(); ++I)
	{
		// Celle lontane fra loro: due porte adiacenti condividerebbero un bordo e i pannelli si
		// sovrapporrebbero, rendendo ambiguo di chi e' quale.
		FRTHexCellData C(FRTCellId(I * 4, 0, 0));
		C.Doors.Add(FRTHexDoor(ERTHexDirection::E, Stati[I]));
		Asset->AddOrUpdateCell(C);
	}
	Asset->SortCells();

	ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
	if (!Actor)
	{
		AddError(TEXT("actor non spawnato"));
		DestroyMapActorWorld(World);
		return false;
	}

	FVector Origin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerH = 0.f;
	Actor->GetHexContext(Origin, HexSize, LayerH);
	const TArray<FTransform> Pannelli = InstancesOf(Actor, TEXT("EdgeFeatures"));
	TestEqual(TEXT("quattro porte, quattro pannelli"), Pannelli.Num(), 4);

	auto AltezzaDellaPorta = [&](int32 Indice) -> double
	{
		const FVector Atteso = URTHexLibrary::EdgeMidpointWorld(FRTCellId(Indice * 4, 0, 0),
			ERTHexDirection::E, Origin, HexSize, LayerH);
		for (const FTransform& Xf : Pannelli)
		{
			const FVector P = Xf.GetLocation();
			if (FMath::IsNearlyEqual(P.X, Atteso.X, 0.5) && FMath::IsNearlyEqual(P.Y, Atteso.Y, 0.5))
			{
				return Xf.GetScale3D().Z;
			}
		}
		return -1.0;
	};

	const double Aperta = AltezzaDellaPorta(0);
	const double Chiusa = AltezzaDellaPorta(1);
	const double Bloccata = AltezzaDellaPorta(2);
	const double Sfondata = AltezzaDellaPorta(3);

	// La distinzione che conta: si passa o non si passa. E' netta, non una sfumatura.
	TestTrue(TEXT("una porta chiusa e' molto piu' alta di una aperta"), Chiusa > Aperta * 4.0);
	// E le due coppie stanno insieme, come il codice dichiara di volere.
	TestTrue(TEXT("bloccata come chiusa: bloccano allo stesso modo"),
		FMath::IsNearlyEqual(Bloccata, Chiusa, 0.001));
	TestTrue(TEXT("sfondata come aperta: si passa in entrambe"),
		FMath::IsNearlyEqual(Sfondata, Aperta, 0.001));

	DestroyMapActorWorld(World);
	return true;
}

/**
 * Il costo di attraversamento resta leggibile anche dove una regola di blocco occupa la stessa cella.
 *
 * I due canali convivono per costruzione — il rilievo dice *quanto costa*, i volumi dicono *cosa fa* — ma
 * convivono solo finche' le loro proporzioni glielo permettono: sono concentrici e partono dalla stessa
 * quota, quindi il piu' largo e piu' alto **inghiotte** l'altro senza che nulla segnali il problema.
 *
 * Il caso non e' teorico ed e' l'unico che esista: il costo massimo del catalogo v0.1 e' `2`, quindi il
 * rilievo piu' alto che una mappa possa produrre e' `ReliefUnitHeight`. Se la lastra della vista lo
 * supera, **ogni** cella costosa che blocca la vista smette di dire quanto costa — e una vista che tace
 * su un dato che il gioco applica e' esattamente cio' che il brief §1 chiama una vista che mente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorReliefUnderSlabTest,
	"RefactorTactics.HexMapActor.CostReliefSurvivesTheSightSlab",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorReliefUnderSlabTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();

	// Una cella che costa piu' del pavimento E blocca la vista: la combinazione che serve a una rotta
	// coperta ma percorribile, cioe' quella che l'autore della mappa cerca apposta.
	URTHexMapAsset* Asset = NewObject<URTHexMapAsset>();
	FRTHexCellData Costosa(FRTCellId(0, 0, 0));
	Costosa.Surface = ERTHexSurface::Rough;
	Costosa.MoveCost = URTTerrainLibrary::FindTerrainDef(ERTHexSurface::Rough).MoveCost;
	Costosa.bBlocksLineOfSight = true;
	Asset->AddOrUpdateCell(Costosa);
	Asset->SortCells();

	ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
	if (!Actor)
	{
		AddError(TEXT("actor non spawnato"));
		DestroyMapActorWorld(World);
		return false;
	}

	const TArray<FTransform> Rilievi = InstancesOf(Actor, TEXT("Relief"));
	const TArray<FTransform> Volumi = InstancesOf(Actor, TEXT("Blockers"));
	if (Rilievi.Num() != 1 || Volumi.Num() != 1)
	{
		AddError(FString::Printf(TEXT("atteso un rilievo e un volume, trovati %d e %d"),
			Rilievi.Num(), Volumi.Num()));
		DestroyMapActorWorld(World);
		return false;
	}

	const FVector ScalaRilievo = Rilievi[0].GetScale3D();
	const FVector ScalaLastra = Volumi[0].GetScale3D();

	// Concentrici e con la stessa base: uno sparisce dentro l'altro se e' insieme piu' basso e piu' stretto.
	const bool bPiuBasso = ScalaRilievo.Z <= ScalaLastra.Z;
	const bool bPiuStretto = ScalaRilievo.X <= ScalaLastra.X;
	TestFalse(TEXT("il rilievo del costo non e' inghiottito dalla lastra della vista"),
		bPiuBasso && bPiuStretto);

	DestroyMapActorWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
