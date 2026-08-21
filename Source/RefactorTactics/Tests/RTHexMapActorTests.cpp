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
	/**
	 * L'ISM che si chiama cosi', o `nullptr`.
	 *
	 * Condiviso dalle due funzioni sotto, che senza divergevano gia' appena nate: una accumulava su tutti i
	 * componenti omonimi, l'altra usciva al primo — due risposte diverse alla stessa domanda.
	 *
	 * ⚠️ **Nome prefissato col dominio del file**, come prescrive il commento di `MakeMapActorWorld`: gli helper in namespace
	 * anonimo di due `.cpp` dello stesso modulo finiscono nella stessa unita' di traduzione con la unity
	 * build, e due `FindIsm` omonimi sarebbero una ridefinizione — riportata sui call site, non sulla
	 * definizione, e comparsa/sparita a seconda del raggruppamento.
	 *
	 * ⚠️ **Presuppone che il nome identifichi UN componente**, che e' vero per costruzione: i quattro ISM
	 * di `ARTHexMapActor` sono `CreateDefaultSubobject` distinti. Non c'e' quindi dipendenza dall'ordine di
	 * `GetComponents` — che itera un `TSet` e non e' ordinato (`CLAUDE.md`, §*Guardrail Claude*). Se un giorno esistessero due
	 * omonimi, questa funzione andrebbe cambiata, non il chiamante.
	 *
	 * ⚠️ `PickIgnoresGeometryThatIsNotTheGrid` ha ancora la propria passata: vive **sopra** questo namespace
	 * e ne risolve due in un giro solo. Chi la unifica sposti prima il namespace.
	 */
	UInstancedStaticMeshComponent* FindMapActorIsm(const ARTHexMapActor* Actor, const TCHAR* ComponentName)
	{
		TArray<UInstancedStaticMeshComponent*> Isms;
		Actor->GetComponents(Isms);
		for (UInstancedStaticMeshComponent* Ism : Isms)
		{
			if (Ism && Ism->GetName() == ComponentName) { return Ism; }
		}
		return nullptr;
	}

	/** Le istanze di un ISM, in world space: quello che l'autore della mappa vede davvero. */
	TArray<FTransform> InstancesOf(const ARTHexMapActor* Actor, const TCHAR* ComponentName)
	{
		TArray<FTransform> Out;
		if (const UInstancedStaticMeshComponent* Ism = FindMapActorIsm(Actor, ComponentName))
		{
			Out.Reserve(Ism->GetInstanceCount());
			for (int32 I = 0; I < Ism->GetInstanceCount(); ++I)
			{
				FTransform Xf;
				Ism->GetInstanceTransform(I, Xf, /*bWorldSpace=*/ true);
				Out.Add(Xf);
			}
		}
		return Out;
	}

	/**
	 * Quante istanze ha un ISM, senza costruirne i transform.
	 *
	 * Esiste perche' chi vuole il solo conteggio pagava un `TArray<FTransform>` per leggerne `.Num()` e
	 * buttava via proprio i transform — e il test dell'idempotenza lo faceva tre volte per componente.
	 * Chi invece **asserisce** sulle posizioni continua a usare `InstancesOf`: le due domande sono diverse.
	 */
	int32 MapActorIsmCount(const ARTHexMapActor* Actor, const TCHAR* ComponentName)
	{
		const UInstancedStaticMeshComponent* Ism = FindMapActorIsm(Actor, ComponentName);
		// ⚠️ **`0` e non `INDEX_NONE` per un componente assente**, e la scelta e' obbligata dall'uso.
		// Un sentinella negativo si propaga in silenzio: le baseline lo raccolgono, e ogni confronto
		// successivo diventa `-1 == -1` e passa. Rinominando `Relief` sarebbero rimaste verdi **sette**
		// asserzioni su un componente che non esiste piu' — cioe' esattamente il difetto che questa issue
		// corregge. Con `0` cade la baseline attesa (`== 1`), e cade per prima.
		// Stessa risposta di `InstancesOf(...).Num()`, che per un componente assente da' `0`: due helper
		// che rispondono alla stessa domanda devono rispondere allo stesso modo.
		return Ism ? Ism->GetInstanceCount() : 0;
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

/**
 * `RebuildInstances` e' idempotente PER LE ISTANZE — e questo test dice esattamente quella meta'.
 *
 * `#996` (AC 5). L'actor giustifica la ricostruzione incondizionata di `PostEditChangeProperty` con un
 * commento: *«l'actor ha poche proprieta' e la ricostruzione e' idempotente»*. L'affermazione e' vera per le
 * istanze e non dice nulla sugli OSSERVATORI — e la issue nasce da un gizmo che sparisce. Qui si pinna la
 * meta' vera, cosi' che se un giorno `RebuildInstances` diventasse condizionale (l'ottimizzazione che #996
 * mette in «da valutare, non da assumere») si sappia subito se ha rotto le celle.
 *
 * ⚠️ **La sola invarianza sarebbe VACUA**: «chiamarla tre volte non cambia niente» e' soddisfatto anche da
 * una `RebuildInstances` che non fa NULLA. Per questo la terza parte misura l'EFFETTO — una cella aggiunta
 * all'asset compare solo dopo la chiamata. La mutazione «corpo di `RebuildInstances` svuotato» fa cadere
 * proprio quella, e senza di essa il test resterebbe verde su un actor rotto.
 * ⚠️ `AddOrUpdateCell` **non** fa broadcast di `OnMapChanged` (incrementa solo `Revision`): verificato, ed e'
 * cio' che rende la chiamata esplicita qui sotto l'unica causa possibile dell'effetto misurato.
 *
 * ⚠️ **`NumInstanceCells()` da solo NON basta, e il nome del test lo promette.** Quel contatore e'
 * `InstanceCells.Num()` — l'array di mapping istanza->cella — e `RebuildInstances` lo `Reset()`
 * **indipendentemente** dalle `ClearInstances()` dei componenti: togliendone una, l'ISM accumula 7, 14, 21
 * istanze mentre l'array ne dichiara sempre 7, la griglia si sdoppia a schermo e ogni conteggio di mapping
 * resta verde. Per questo si legge anche `GetInstanceCount()` di ciascun componente.
 *
 * ⚠️ **E l'asset NON puo' essere quello di default.** Gli ISM sono quattro — `Cells`, `Relief`, `Blockers`,
 * `EdgeFeatures` — ma `MakeActorTestAsset` produce celle di default, dove `MoveCost = 1` da'
 * `ReliefHeightForCost(1) = 0`, nessun flag accende `Blockers` e nessun Cover accende `EdgeFeatures`. Con
 * quell'asset gli ultimi tre non sono «non verificati»: sono **strutturalmente non osservabili**, e tre
 * delle quattro `ClearInstances()` di `ARTHexMapActor::RebuildInstances` si possono cancellare senza che
 * una sola asserzione cada. Per questo la fixture porta una cella costosa, una che blocca e una con un
 * bordo: non e' ampliamento di scope, e' cio' che rende misurabile l'invariante gia' dichiarata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorRebuildIsIdempotentTest,
	"RefactorTactics.HexMapActor.RebuildInstancesIsIdempotentForInstances",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorRebuildIsIdempotentTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	// Sette celle, ma NON di default: tre portano il dato che accende gli altri tre ISM. Con l'asset di
	// default `Relief`, `Blockers` ed `EdgeFeatures` restano a zero istanze, e le loro `ClearInstances()`
	// diventano non osservabili — si possono cancellare tutte e tre senza che una sola asserzione cada.
	URTHexMapAsset* Asset = MakeActorTestAsset(/*Radius*/ 1); // 7 celle
	{
		// `ReliefHeightForCost(1) = 0`: al costo del pavimento il rilievo non esiste, serve un sovrapprezzo.
		// ⚠️ Superficie E costo dal **catalogo**, come fa `CostReliefSurvivesTheSightSlab` e come dichiara
		// l'include di `RTTerrainLibrary.h` in testa al file. Non perche' un `MoveCost` scritto a mano sia
		// impossibile — il pennello ha `Surface` e `MoveCost` come campi indipendenti, quindi `Floor` a
		// costo 2 si puo' dipingere — ma perche' un numero letterale qui non seguirebbe un ribilanciamento
		// di `Rough`, e la fixture smetterebbe di rappresentare il terreno che dice di usare.
		FRTHexCellData Costosa(FRTCellId(1, 0, 0));
		Costosa.Surface = ERTHexSurface::Rough;
		Costosa.MoveCost = URTTerrainLibrary::FindTerrainDef(ERTHexSurface::Rough).MoveCost;
		Asset->AddOrUpdateCell(Costosa);

		FRTHexCellData Blocco(FRTCellId(0, 1, 0));
		Blocco.bBlocksMovement = true;
		Asset->AddOrUpdateCell(Blocco);

		FRTHexCellData ConBordo(FRTCellId(-1, 0, 0));
		ConBordo.Covers.Add(FRTHexCover(ERTHexDirection::E, ERTHexCoverType::Low));
		Asset->AddOrUpdateCell(ConBordo);

		// ⚠️ Nessun `SortCells()`: i tre id esistono gia' nell'area di raggio 1, quindi `AddOrUpdateCell`
		// prende il ramo di aggiornamento in place e l'ordine non cambia. Rimetterlo qui sarebbe un passo
		// che sembra necessario e non lo e', e verrebbe copiato come tale nella prossima fixture.
	}

	ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
	if (!TestNotNull(TEXT("actor spawnato"), Actor))
	{
		DestroyMapActorWorld(World);
		return false;
	}

	// Non basta il conteggio: due ricostruzioni potrebbero dare 7 istanze mappate a celle diverse, e il
	// raycast di selezione leggerebbe la cella sbagliata senza che nessun numero cambi.
	const int32 CelleIniziali = Actor->NumInstanceCells();
	TestEqual(TEXT("le 7 celle dell'asset sono rappresentate"), CelleIniziali, 7);

	// Le DUE misure, e servono entrambe: l'array di mapping (sopra) e le istanze davvero nell'ISM (qui). La
	// prima regge il raycast di selezione, la seconda e' cio' che si vede. Si scollano se la ricostruzione
	// smette di ripulire il componente, ed e' proprio il caso che questo test esiste per prendere.
	// `CelleIniziali` e non `7`: la dimensione della fixture e' gia' asserita sopra, e ripeterne il numero
	// qui creerebbe due fatti apparentemente indipendenti che in realta' sono lo stesso.
	TestEqual(TEXT("l'ISM ha una istanza per cella"), MapActorIsmCount(Actor, TEXT("Cells")), CelleIniziali);

	// ⚠️ **Costanti dichiarate dalla fixture, non misure lette dall'actor**, ed e' la differenza fra un test
	// e una tautologia. La fixture mette **una** cella costosa, **una** che blocca il movimento e **un**
	// solo bordo con copertura: i conteggi corretti sono 1/1/1 e si sanno senza guardare l'actor.
	// 🔴 Leggerli dall'actor e poi confrontarci le misure successive li renderebbe veri per costruzione: se
	// `RebuildInstances` emettesse un rilievo per OGNI cella, la baseline varrebbe 7, cadrebbe **solo**
	// l'asserzione che la confronta con 1, e ogni verifica seguente tornerebbe 7 vs 7. Per questo il numero
	// atteso compare in **tutte** le asserzioni, e nessuna dipende dall'esito di un'altra.
	constexpr int32 RilieviAttesi = 1;
	constexpr int32 BlocchiAttesi = 1;
	constexpr int32 BordiAttesi = 1;

	TestEqual(TEXT("una sola cella costosa, un solo rilievo"),
		MapActorIsmCount(Actor, TEXT("Relief")), RilieviAttesi);
	TestEqual(TEXT("una sola cella che blocca, un solo volume"),
		MapActorIsmCount(Actor, TEXT("Blockers")), BlocchiAttesi);
	TestEqual(TEXT("un solo bordo dichiarato, un solo pannello"),
		MapActorIsmCount(Actor, TEXT("EdgeFeatures")), BordiAttesi);

	TArray<FRTCellId> PrimaDelle;
	PrimaDelle.Reserve(CelleIniziali);
	for (int32 I = 0; I < CelleIniziali; ++I)
	{
		PrimaDelle.Add(Actor->CellForInstance(I));
	}

	for (int32 Giro = 0; Giro < 3; ++Giro)
	{
		Actor->RebuildInstances();
	}

	TestEqual(TEXT("tre ricostruzioni non cambiano il numero di celle mappate"),
		Actor->NumInstanceCells(), CelleIniziali);

	// LE righe che le mutazioni «via `<Componente>->ClearInstances()`» fanno cadere: senza pulizia l'ISM
	// accumula e qui si leggerebbe il quadruplo, mentre ogni altra misura di questo file resterebbe verde.
	// Una per componente, perche' le quattro `ClearInstances()` sono quattro righe distinte e togliendone
	// una sola le altre tre non se ne accorgono.
	TestEqual(TEXT("tre ricostruzioni non accumulano istanze in Cells"),
		MapActorIsmCount(Actor, TEXT("Cells")), CelleIniziali);
	TestEqual(TEXT("tre ricostruzioni non accumulano istanze in Relief"),
		MapActorIsmCount(Actor, TEXT("Relief")), RilieviAttesi);
	TestEqual(TEXT("tre ricostruzioni non accumulano istanze in Blockers"),
		MapActorIsmCount(Actor, TEXT("Blockers")), BlocchiAttesi);
	TestEqual(TEXT("tre ricostruzioni non accumulano istanze in EdgeFeatures"),
		MapActorIsmCount(Actor, TEXT("EdgeFeatures")), BordiAttesi);

	// ⚠️ La guardia `> 0` non e' difensiva: senza, `CelleIniziali == 0` renderebbe il seme vero, il ciclo
	// non girerebbe mai e l'asserzione passerebbe su un actor vuoto. Stesso schema, stesso file: i due
	// cicli `bAllOnActiveLayer` di `LayerFilterOnConstruction` lo proteggono cosi'.
	bool bMappaturaStabile = (CelleIniziali > 0) && (Actor->NumInstanceCells() == CelleIniziali);
	for (int32 I = 0; bMappaturaStabile && I < CelleIniziali; ++I)
	{
		bMappaturaStabile = (Actor->CellForInstance(I) == PrimaDelle[I]);
	}
	TestTrue(TEXT("tre ricostruzioni lasciano la mappa istanza->cella identica, indice per indice"),
		bMappaturaStabile);

	// L'EFFETTO. Senza questa parte l'invarianza qui sopra e' soddisfatta da una funzione inerte.
	const FRTCellId Nuova(2, -1, 0);
	TestFalse(TEXT("la cella scelta per l'effetto non era gia' nell'asset"), Asset->ContainsCell(Nuova));
	Asset->AddOrUpdateCell(FRTHexCellData(Nuova));
	Asset->SortCells();

	// ⚠️ **Questa asserzione e' la GUARDIA DI CAUSALITA' dell'intera sezione «EFFETTO», e va tenuta.**
	// Senza, basta che `RebuildInstances` diventi condizionale **e** che `AddOrUpdateCell` faccia broadcast
	// perche' l'actor si ricostruisca da solo: la chiamata esplicita qui sotto diventerebbe un no-op e ogni
	// asserzione seguente resterebbe verde, misurando una causa che non c'e' piu'.
	// ⚠️ Se un giorno cade, **non cancellarla**: e' il segnale che la causa e' cambiata, e la risposta e'
	// riscrivere la sezione EFFETTO attorno a quella nuova. Un commento al suo posto non asserisce nulla.
	TestEqual(TEXT("finche' non si ricostruisce, l'actor non vede la cella nuova"),
		Actor->NumInstanceCells(), CelleIniziali);

	Actor->RebuildInstances();
	TestEqual(TEXT("dopo la ricostruzione la cella nuova e' mappata"),
		Actor->NumInstanceCells(), CelleIniziali + 1);
	TestEqual(TEXT("dopo la ricostruzione la cella nuova ha la sua istanza nell'ISM"),
		MapActorIsmCount(Actor, TEXT("Cells")), CelleIniziali + 1);

	// ⚠️ E gli altri tre **non** devono essere cambiati: `Nuova` e' una cella di default, quindi non porta
	// rilievo, ne' blocco, ne' bordi. Senza queste tre righe la ricostruzione dell'effetto sarebbe l'unica
	// del test a essere misurata su un componente solo — la stessa cecita' uno-su-quattro, in piccolo.
	TestEqual(TEXT("la cella nuova non aggiunge rilievi"),
		MapActorIsmCount(Actor, TEXT("Relief")), RilieviAttesi);
	TestEqual(TEXT("la cella nuova non aggiunge volumi di blocco"),
		MapActorIsmCount(Actor, TEXT("Blockers")), BlocchiAttesi);
	TestEqual(TEXT("la cella nuova non aggiunge pannelli di bordo"),
		MapActorIsmCount(Actor, TEXT("EdgeFeatures")), BordiAttesi);

	// ⚠️ Il totale che cresce NON dice che sia arrivata `Nuova`: una ricostruzione che emettesse un
	// duplicato di una cella gia' presente darebbe lo stesso `+1` e passerebbe. L'asserzione che conta e'
	// l'appartenenza, ed e' quella che il test dichiarava in apertura senza mai verificarla.
	bool bNuovaMappata = false;
	for (int32 I = 0; !bNuovaMappata && I < Actor->NumInstanceCells(); ++I)
	{
		bNuovaMappata = (Actor->CellForInstance(I) == Nuova);
	}
	TestTrue(TEXT("la cella nuova e' fra quelle mappate, non solo un'unita' in piu' nel totale"),
		bNuovaMappata);

	DestroyMapActorWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
