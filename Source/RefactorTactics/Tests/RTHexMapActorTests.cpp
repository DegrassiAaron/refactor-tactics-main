#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h" // #2761: la CVar che cambia la semantica degli indici di RemoveInstance
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"                  // TActorIterator: nessun Actor per cella si conta guardando il mondo
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTMapVisuals.h"          // RTCellPrismRadius: la scala Z del corpo si legge contro la mesh
#include "Map/RTStructuralBodyLibrary.h" // il corpo lo calcola il derivatore, il test non lo ricalcola
#include "Perception/RTTeamKnowledge.h" // il velo: la griglia deve seguirlo, non ignorarlo
#include "Map/RTMapVisuals.h"          // le quote condivise: qui si LEGGONO, non si ricopiano
#include "Turn/RTMatchSetupLibrary.h" // MakeTestArena: una board con piu' famiglie popolate
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


// -- #1758 - La griglia: il confine fra celle, in partita e senza comando console --------------------
//
// 🔴 **Perche' questi test esistono e cosa NON possono dire.** Il DoD di #1758 chiede che il toggle
// «spenga e riaccenda senza ricostruire istanze o asset, e senza mutare `FRTMapState`, graph revision,
// path cache, snapshot o TurnLog - **e lo prova un test**, non un'ispezione». Quello e' verificabile
// headless ed e' verificato qui. Che il confine **si legga** a distanza tattica non lo e': non esiste un
// oracolo automatico per «si vede», e inventarne uno direbbe il falso. Quel giudizio appartiene alla voce
// `PIE-*` della issue, e resta li'.

// La griglia copre OGNI cella, non solo quelle con un glifo: il confine non dipende dal terreno, e cinque
// superfici su nove non ricevono alcun segno di superficie.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapGridOnByDefaultTest,
	"RefactorTactics.HexMapActor.GridIsOnByDefaultAndCoversEveryCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapGridOnByDefaultTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	ARTHexMapActor* Actor = SpawnMapActor(World, MakeActorTestAsset(/*Radius=*/ 1)); // 7 celle
	if (!Actor)
	{
		AddError(TEXT("actor non spawnato"));
		DestroyMapActorWorld(World);
		return false;
	}

	// ON di default: e' un requisito del DoD, non una preferenza. Il confine e' cio' con cui si conta il
	// movimento, e chiederlo con un comando sarebbe la soluzione che la issue esclude per nome.
	TestTrue(TEXT("la griglia e' accesa senza che nessuno la chieda"), Actor->AreCellBordersVisible());

	const UInstancedStaticMeshComponent* Grid = FindMapActorIsm(Actor, TEXT("CellBorders"));
	if (!TestNotNull(TEXT("il componente della griglia esiste"), Grid))
	{
		DestroyMapActorWorld(World);
		return false;
	}
	TestTrue(TEXT("ed e' visibile"), Grid->IsVisible());
	TestEqual(TEXT("un'istanza per cella, tutte e sette"), Grid->GetInstanceCount(), 7);
	TestEqual(TEXT("una per cella come il disco, non una per superficie"),
		Grid->GetInstanceCount(), Actor->NumInstanceCells());

	// Il canale colore per istanza: senza, il velo potrebbe solo NASCONDERE il bordo, e ricordo e
	// osservazione diventerebbero indistinguibili sul confine ([D-227]).
	TestEqual(TEXT("porta i tre float del colore, come Cells e i glifi"), Grid->NumCustomDataFloats, 3);

	// Il raycast di selezione valida il COMPONENTE: un bordo che rubasse i click darebbe una cella valida e
	// sbagliata, che e' il difetto di #588.
	TestFalse(TEXT("un colpo sulla griglia non seleziona una cella"),
		Actor->IsPickOnSelectableCell(Grid, 0));

	DestroyMapActorWorld(World);
	return true;
}

// 🔑 Il test che il DoD nomina: spegnere e riaccendere NON ricostruisce e NON muta lo stato di gioco.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapGridToggleIsInertTest,
	"RefactorTactics.HexMapActor.GridToggleChangesNothingButVisibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapGridToggleIsInertTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	URTHexMapAsset* Asset = MakeActorTestAsset(/*Radius=*/ 2); // 19 celle
	ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
	if (!Actor)
	{
		AddError(TEXT("actor non spawnato"));
		DestroyMapActorWorld(World);
		return false;
	}

	// Lo stato PRIMA, su quattro canali indipendenti.
	const uint32 HashPrima = Asset->ComputeHash();
	const int32 CellePrima = Actor->NumInstanceCells();
	const TArray<FTransform> GrigliaPrima = InstancesOf(Actor, TEXT("CellBorders"));
	const TArray<FTransform> DischiPrima = InstancesOf(Actor, TEXT("Cells"));
	const bool OverlayPrima = Actor->IsCellOverlayEnabled();

	// OFF -> ON -> OFF: il ciclo completo che la issue chiede, non un solo verso.
	Actor->SetCellBordersVisible(false);
	const UInstancedStaticMeshComponent* Grid = FindMapActorIsm(Actor, TEXT("CellBorders"));
	if (!TestNotNull(TEXT("il componente della griglia esiste"), Grid))
	{
		DestroyMapActorWorld(World);
		return false;
	}
	TestFalse(TEXT("spenta, il componente non e' visibile"), Grid->IsVisible());
	TestFalse(TEXT("e il flag lo dice"), Actor->AreCellBordersVisible());
	// 🔴 Le istanze restano: spegnere e' un cambio di VISIBILITA'. Ricostruire azzererebbe `LastVeilState`
	// insieme agli indici a cui si riferisce, e il velo successivo dovrebbe ridipingere tutto.
	TestEqual(TEXT("spegnere NON distrugge le istanze"), Grid->GetInstanceCount(), GrigliaPrima.Num());

	Actor->SetCellBordersVisible(true);
	TestTrue(TEXT("riaccesa, torna visibile"), Grid->IsVisible());
	Actor->SetCellBordersVisible(false);

	// Lo stato DOPO: niente si e' mosso tranne la visibilita'.
	TestEqual(TEXT("l'hash dell'asset non cambia: nessuna mutazione di FRTMapState ne' di graph revision"),
		Asset->ComputeHash(), HashPrima);
	TestEqual(TEXT("la mappa istanza->cella e' intatta"), Actor->NumInstanceCells(), CellePrima);

	const TArray<FTransform> GrigliaDopo = InstancesOf(Actor, TEXT("CellBorders"));
	const TArray<FTransform> DischiDopo = InstancesOf(Actor, TEXT("Cells"));
	TestEqual(TEXT("nessuna istanza di griglia aggiunta o persa"), GrigliaDopo.Num(), GrigliaPrima.Num());
	TestEqual(TEXT("nessuna istanza di cella toccata"), DischiDopo.Num(), DischiPrima.Num());

	// Le trasformate, non solo i conteggi: un rebuild che ricostruisse lo stesso NUMERO di istanze in ordine
	// diverso passerebbe un test che conta e basta.
	bool bTrasformateIntatte = GrigliaDopo.Num() == GrigliaPrima.Num();
	for (int32 I = 0; bTrasformateIntatte && I < GrigliaDopo.Num(); ++I)
	{
		bTrasformateIntatte = GrigliaDopo[I].Equals(GrigliaPrima[I], 0.01f);
	}
	TestTrue(TEXT("le trasformate della griglia sono le stesse, non solo lo stesso numero"), bTrasformateIntatte);

	// ⚠️ `rt.Debug.DrawCells` e' un sistema SEPARATO e resta tale: la issue vieta esplicitamente di usarlo
	// come soluzione player-facing, e questo verifica che il toggle di presentazione non lo tocchi.
	TestEqual(TEXT("l'overlay di debug non e' stato toccato dal toggle di presentazione"),
		Actor->IsCellOverlayEnabled(), OverlayPrima);
	Actor->SetCellOverlayEnabled(true);
	TestTrue(TEXT("e continua a funzionare da solo, a griglia spenta"), Actor->IsCellOverlayEnabled());
	TestFalse(TEXT("senza riaccendere la griglia"), Actor->AreCellBordersVisible());

	DestroyMapActorWorld(World);
	return true;
}

// Le quote: sopra la faccia del prisma, e sopra il glifo che altrimenti la coprirebbe.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapGridSitsAboveTheFaceTest,
	"RefactorTactics.HexMapActor.GridSitsAboveTheCellFace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapGridSitsAboveTheFaceTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	ARTHexMapActor* Actor = SpawnMapActor(World, MakeActorTestAsset(/*Radius=*/ 1));
	if (!Actor)
	{
		AddError(TEXT("actor non spawnato"));
		DestroyMapActorWorld(World);
		return false;
	}

	const TArray<FTransform> Griglia = InstancesOf(Actor, TEXT("CellBorders"));
	const TArray<FTransform> Dischi = InstancesOf(Actor, TEXT("Cells"));
	if (Griglia.Num() == 0 || Dischi.Num() != Griglia.Num())
	{
		AddError(TEXT("griglia e dischi non si corrispondono"));
		DestroyMapActorWorld(World);
		return false;
	}

	// 🔴 Sotto `RTCellTopZ` sarebbe DENTRO il prisma, e a schermo non si distinguerebbe da «non disegnato».
	// E' successo due volte, e `PIE-DEBUG-CELLS` registra la prima. Il confronto e' contro la faccia REALE
	// del disco misurata dalla sua istanza, non contro un numero riscritto qui.
	bool bTutteSopraLaFaccia = true;
	for (int32 I = 0; I < Griglia.Num(); ++I)
	{
		const double FacciaZ = Dischi[I].GetLocation().Z + RTCellTopZ;
		bTutteSopraLaFaccia &= Griglia[I].GetLocation().Z > FacciaZ;
	}
	TestTrue(TEXT("ogni istanza di griglia sta SOPRA la faccia del proprio prisma"), bTutteSopraLaFaccia);

	// E la costante e' derivata, non scritta: se un giorno lo spessore del tile cambiasse, la griglia
	// salirebbe con lui senza che nessuno tocchi questa riga.
	TestTrue(TEXT("la quota della griglia deriva da RTCellTopZ"), RTLiftCellBorder > RTCellTopZ);

	DestroyMapActorWorld(World);
	return true;
}

// Nessun Actor e nessun component per cella: e' l'invariante d'apertura di `ARTHexMapActor`, e la issue la
// ripete perche' una griglia e' precisamente il posto in cui verrebbe voglia di violarla.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapGridAddsNoActorPerCellTest,
	"RefactorTactics.HexMapActor.GridAddsNoActorPerCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapGridAddsNoActorPerCellTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();

	// Gli attori di servizio del mondo, PRIMA che la board esista: sono la linea di base da cui si misura.
	int32 AttoriPrima = 0;
	for (TActorIterator<AActor> It(World); It; ++It) { ++AttoriPrima; }

	// 37 celle: abbastanza perche' un Actor-per-cella si veda come un salto, non come rumore.
	ARTHexMapActor* Actor = SpawnMapActor(World, MakeActorTestAsset(/*Radius=*/ 3));
	if (!Actor)
	{
		AddError(TEXT("actor non spawnato"));
		DestroyMapActorWorld(World);
		return false;
	}

	// 🔴 **Si misura il DELTA, non il totale, ed e' la correzione di un oracolo sbagliato**: la prima
	// stesura confrontava il conteggio assoluto con `1` ed e' uscita rossa con **7** — un mondo di test
	// nasce gia' con i propri attori di servizio (`WorldSettings`, `GameMode`, `GameState`…), che non
	// c'entrano nulla con questa invariante. Un test che li contasse misurerebbe Unreal, non la griglia.
	//
	// Il delta e' anche l'oracolo piu' FORTE: se esistesse un Actor per cella varrebbe 38 invece di 1, e la
	// differenza scalerebbe col raggio — che e' precisamente la violazione da intercettare.
	int32 AttoriDopo = 0;
	for (TActorIterator<AActor> It(World); It; ++It) { ++AttoriDopo; }
	TestEqual(TEXT("37 celle hanno aggiunto UN solo actor al mondo"), AttoriDopo - AttoriPrima, 1);

	// E nemmeno un component per cella: la griglia e' istanziata dentro un solo ISM.
	TArray<UInstancedStaticMeshComponent*> Isms;
	Actor->GetComponents(Isms);
	int32 Griglie = 0;
	for (const UInstancedStaticMeshComponent* Ism : Isms)
	{
		if (Ism && Ism->GetName() == TEXT("CellBorders")) { ++Griglie; }
	}
	TestEqual(TEXT("un solo componente di griglia per 37 celle"), Griglie, 1);
	TestEqual(TEXT("e porta 37 istanze"), InstancesOf(Actor, TEXT("CellBorders")).Num(), 37);

	DestroyMapActorWorld(World);
	return true;
}

// Il velo deve ATTENUARE la griglia, non ignorarla: un bordo a piena luminosita' sul ricordo renderebbe una
// cella non osservata piu' marcata di una osservata - il rovesciamento che la corona dei glifi aveva nella
// prima stesura della spec.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapGridFollowsTheVeilTest,
	"RefactorTactics.HexMapActor.GridFollowsTheKnowledgeVeil",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapGridFollowsTheVeilTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	ARTHexMapActor* Actor = SpawnMapActor(World, MakeActorTestAsset(/*Radius=*/ 1)); // 7 celle
	if (!Actor)
	{
		AddError(TEXT("actor non spawnato"));
		DestroyMapActorWorld(World);
		return false;
	}

	// Una cella osservata, una ricordata, cinque mai viste.
	FRTTeamKnowledge Knowledge;
	Knowledge.VisibleCells.Add(FRTCellId(0, 0, 0));
	Knowledge.ExploredCells.Add(FRTCellId(0, 0, 0));
	Knowledge.ExploredCells.Add(FRTCellId(1, 0, 0));
	Actor->ApplyKnowledgeVeil(Knowledge);

	// 🔴 Si legge la SCALA REALE delle istanze, non un contatore: un contatore proverebbe che il velo sa
	// contare, non che ha disegnato. E' la disciplina di `GetVeilCounts`.
	const TArray<FTransform> Griglia = InstancesOf(Actor, TEXT("CellBorders"));
	int32 Nascoste = 0;
	int32 Disegnate = 0;
	for (const FTransform& Xf : Griglia)
	{
		if (Xf.GetScale3D().IsNearlyZero()) { ++Nascoste; } else { ++Disegnate; }
	}

	// Due celle conosciute (una osservata, una ricordata) restano disegnate; le altre cinque spariscono.
	TestEqual(TEXT("la griglia non si disegna dove la squadra non e' mai stata"), Nascoste, 5);
	TestEqual(TEXT("e resta sulle due celle conosciute"), Disegnate, 2);

	// Il ritorno: una cella tornata visibile riprende il proprio bordo. Senza `BorderBaseScale` il velo
	// sarebbe irreversibile, perche' da una scala gia' a zero non si risale a quella piena ([D-227]).
	FRTTeamKnowledge Tutto;
	for (int32 Q = -1; Q <= 1; ++Q)
	{
		for (int32 R = -1; R <= 1; ++R)
		{
			Tutto.VisibleCells.Add(FRTCellId(Q, R, 0));
			Tutto.ExploredCells.Add(FRTCellId(Q, R, 0));
		}
	}
	Actor->ApplyKnowledgeVeil(Tutto);

	int32 NascosteDopo = 0;
	for (const FTransform& Xf : InstancesOf(Actor, TEXT("CellBorders")))
	{
		if (Xf.GetScale3D().IsNearlyZero()) { ++NascosteDopo; }
	}
	TestEqual(TEXT("il velo e' reversibile: la griglia torna su tutte e sette"), NascosteDopo, 0);

	DestroyMapActorWorld(World);
	return true;
}


/**
 * 🔴 **Il corpo dichiarato dall'autore diventa geometria, e quello non dichiarato non esiste** — `#1865`.
 *
 * Il derivatore era gia' provato headless (`RefactorTactics.StructuralBody.*`); questo prova l'altra meta':
 * che `RebuildInstances` lo **posi**, e che le quote arrivino da `DeriveBodies` invece di essere ricalcolate
 * qui — una seconda formula del confine sarebbe la verita' duplicata che #1865 vieta.
 *
 * ➕ **Il controllo negativo e' nello stesso test**: la stessa board con `BodyFill = None` non produce nulla.
 * Senza, «una istanza» sarebbe compatibile con un rendering che disegna un corpo per ogni cella.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorStructuralBodyTest,
	"RefactorTactics.HexMapActor.StructuralBodyIsPosedOnlyWhereDeclared",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorStructuralBodyTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	URTHexMapAsset* Asset = NewObject<URTHexMapAsset>(GetTransientPackage());
	// Due superfici nella stessa colonna: una a Layer 1 che dichiara un corpo pieno, una a Layer 0 che non
	// dichiara niente. Il corpo atteso e' UNO, e sta sotto quella di sopra.
	{
		FRTHexCellData Sotto(FRTCellId(0, 0, 0));
		Asset->AddOrUpdateCell(Sotto);
		FRTHexCellData Sopra(FRTCellId(0, 0, 1));
		Sopra.BodyFill = ERTHexBodyFill::Full;
		Asset->AddOrUpdateCell(Sopra);
		Asset->SortCells();
	}

	ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
	if (!TestNotNull(TEXT("actor mappa"), Actor)) { return false; }

	const TArray<FTransform> Corpi = InstancesOf(Actor, TEXT("StructuralBodies"));
	if (!TestEqual(TEXT("un corpo, solo per la cella che lo dichiara"), Corpi.Num(), 1)) { return false; }

	// 🔑 Le quote vengono dal derivatore: si confrontano con quelle, non con una formula riscritta qui.
	const TArray<FRTStructuralBody> Attesi = URTStructuralBodyLibrary::DeriveBodies(Asset);
	if (!TestEqual(TEXT("il derivatore ne calcola uno"), Attesi.Num(), 1)) { return false; }

	const float AltezzaAttesa = Attesi[0].Height();
	const float CentroAtteso = (Attesi[0].TopZ + Attesi[0].BottomZ) * 0.5f;
	AddInfo(FString::Printf(TEXT("corpo: top %.1f · bottom %.1f · alto %.1f · centro %.1f"),
		Attesi[0].TopZ, Attesi[0].BottomZ, AltezzaAttesa, CentroAtteso));

	TestTrue(TEXT("l'istanza sta alla quota che il derivatore ha calcolato"),
		FMath::IsNearlyEqual(static_cast<float>(Corpi[0].GetLocation().Z), CentroAtteso, 0.5f));
	// La mesh nasce alta `2 * RTCellPrismRadius`: la scala Z e' quel rapporto, e senza questa riga un corpo
	// alto la meta' starebbe comunque nel posto giusto.
	TestTrue(TEXT("ed e' alta quanto il derivatore ha chiesto"),
		FMath::IsNearlyEqual(static_cast<float>(Corpi[0].GetScale3D().Z) * 2.f * RTCellPrismRadius,
			AltezzaAttesa, 0.5f));

	// ➕ CONTROLLO NEGATIVO: senza dichiarazione non nasce nessun corpo.
	{
		FRTHexCellData Sopra(FRTCellId(0, 0, 1));
		Sopra.BodyFill = ERTHexBodyFill::None;
		Asset->AddOrUpdateCell(Sopra);
		Asset->SortCells();
		Actor->RebuildInstances();
		TestEqual(TEXT("senza BodyFill non si posa niente"),
			InstancesOf(Actor, TEXT("StructuralBodies")).Num(), 0);
	}
	return true;
}

/**
 * 🔑 **Il corpo rispetta il filtro dei piani, e non si accumula.**
 *
 * Due proprieta' in un test perche' sono lo stesso rischio visto da due lati: un componente che non
 * partecipa alle viste mostra il volume di un piano nascosto, e uno che non si azzera mostra il volume di
 * una board che non esiste piu' — il difetto che `KnowledgeVolumes` ha gia' avuto (`#2222`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorStructuralBodyLayerViewTest,
	"RefactorTactics.HexMapActor.StructuralBodyFollowsTheLayerFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorStructuralBodyLayerViewTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	URTHexMapAsset* Asset = NewObject<URTHexMapAsset>(GetTransientPackage());
	{
		FRTHexCellData Sopra(FRTCellId(0, 0, 1));
		Sopra.BodyFill = ERTHexBodyFill::Full;
		Asset->AddOrUpdateCell(Sopra);
		Asset->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, 0)));
		Asset->SortCells();
	}

	// `AllLayers`: il corpo del piano 1 si vede. E' il controllo positivo del test.
	ARTHexMapActor* Tutti = SpawnMapActor(World, Asset, /*ActiveLayer=*/ 0, ERTLayerViewMode::AllLayers);
	if (!TestNotNull(TEXT("actor AllLayers"), Tutti)) { return false; }
	if (!TestEqual(TEXT("con tutti i piani il corpo si vede"),
			InstancesOf(Tutti, TEXT("StructuralBodies")).Num(), 1))
	{
		return false;
	}

	// `ActiveOnly` sul piano 0: il corpo appartiene al piano 1, quindi sparisce con la sua superficie.
	ARTHexMapActor* SoloAttivo = SpawnMapActor(World, Asset, /*ActiveLayer=*/ 0, ERTLayerViewMode::ActiveOnly);
	if (!TestNotNull(TEXT("actor ActiveOnly"), SoloAttivo)) { return false; }
	TestEqual(TEXT("col solo piano attivo il corpo di un altro piano non si disegna"),
		InstancesOf(SoloAttivo, TEXT("StructuralBodies")).Num(), 0);

	// 🔑 E non si accumula: due ricostruzioni di seguito lasciano una istanza, non due.
	Tutti->RebuildInstances();
	Tutti->RebuildInstances();
	TestEqual(TEXT("due ricostruzioni non raddoppiano i corpi"),
		InstancesOf(Tutti, TEXT("StructuralBodies")).Num(), 1);
	return true;
}


/**
 * 🔴 **Il costo di una modifica è quello della MODIFICA, non della board** — `#2761`.
 *
 * ## ⌫ Questo test ha sostituito il suo opposto, e la sostituzione è il criterio di successo
 *
 * Fino al 2026-09-10 qui stava `RebuildCostScalesWithTheMapNotTheEdit`, che asseriva **il difetto**:
 * *«il costo segue la mappa — `Grande > Piccola * 3`»*. Era scritto per **essere rosso il giorno in cui il
 * difetto viene corretto**, e il suo commento diceva cosa scrivere al suo posto:
 *
 * > *«Quando il punto 3 di #1865 sarà implementato questa riga DEVE cadere, e va sostituita con il suo
 * > opposto: `Grande` vicino a `Piccola`, perché entrambe hanno dipinto una cella sola. Non riallineare il
 * > numero: è la caduta a dire che l'ottimizzazione è arrivata.»*
 *
 * ⛔ **Non è stato riallineato.** I suoi numeri di baseline, misurati su `65781204`, restano scritti qui
 * perché sono l'unica prova che il lavoro è avvenuto: **14** istanze su board r=1 (7 celle) e **122** su
 * board r=4 (61 celle), per una pennellata su **una** cella.
 *
 * ## Cosa misura adesso, e perché passa dal velo
 *
 * ⚠️ **Il percorso è `ApplyKnowledgeVeil`, non `RebuildInstances`**, e non è un dettaglio di comodo: è il
 * percorso RUNTIME. `PaintCellData` ha solo chiamanti d'editor, mentre il terreno che cambia in partita
 * — `Action.Ignite`, `Action.CreateWater`, `Hero.Muiren.MistVeil` — arriva alla board dal blocco di
 * sincronizzazione della revisione che `#2894` ha messo dentro il velo. È lì che il costo si pagava due
 * volte per turno, ed è lì che il per-cella doveva arrivare.
 *
 * ⚠️ **Si confrontano due board di DIMENSIONE DIVERSA**, non due modifiche: è l'unico modo di distinguere
 * «proporzionale alla modifica» da «proporzionale alla mappa». Con una board sola, qualunque numero
 * sarebbe compatibile con entrambe le ipotesi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorEditCostTest,
	"RefactorTactics.HexMapActor.EditCostScalesWithTheEditNotTheMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorEditCostTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	// La stessa modifica — una cella sola — su due board di taglia diversa.
	auto CostoDiUnaPennellata = [&](int32 Radius) -> int32
	{
		URTHexMapAsset* Asset = MakeActorTestAsset(Radius);
		ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
		if (!Actor) { return -1; }

		// Il primo velo sincronizza la revisione: senza, la pennellata qui sotto verrebbe confusa con
		// «tutto quello che è successo dall'inizio», che è il caso in cui il registro dichiara di non sapere.
		FRTTeamKnowledge Tutto;
		for (const FRTHexCellData& C : Asset->Cells)
		{
			Tutto.VisibleCells.Add(C.Id);
			Tutto.ExploredCells.Add(C.Id);
		}
		Actor->ApplyKnowledgeVeil(Tutto);

		// Una cella sola cambia superficie: è la pennellata minima che l'autore possa dare.
		FRTHexCellData Dipinta = Asset->Cells[0];
		Dipinta.Surface = Dipinta.Surface == ERTHexSurface::Floor ? ERTHexSurface::Rough : ERTHexSurface::Floor;
		Asset->AddOrUpdateCell(Dipinta);

		// Il velo successivo è il percorso runtime: vede la revisione mossa e riallinea.
		Actor->ApplyKnowledgeVeil(Tutto);
		return Actor->LastRepaintTouchedInstances();
	};

	const int32 Piccola = CostoDiUnaPennellata(/*Radius=*/ 1);   // 7 celle
	const int32 Grande  = CostoDiUnaPennellata(/*Radius=*/ 4);   // 61 celle
	if (!TestTrue(TEXT("entrambe le board si sono costruite"), Piccola > 0 && Grande > 0)) { return false; }

	AddInfo(FString::Printf(
		TEXT("una pennellata su UNA cella costa %d istanze su board r=1 (7 celle) e %d su board r=4 (61 celle) ")
		TEXT("— la baseline del 2026-09-10 diceva 14 e 122"),
		Piccola, Grande));

	// 🔑 L'OPPOSTO della baseline: la stessa modifica costa **uguale** sulle due board, perché entrambe
	// hanno dipinto una cella sola. Si asserisce l'uguaglianza e non una soglia — una soglia lascerebbe
	// passare una regressione che riportasse il costo a crescere di poco con la mappa.
	TestEqual(*FString::Printf(
			TEXT("il costo segue la MODIFICA: %d su 7 celle e %d su 61"), Piccola, Grande),
		Grande, Piccola);

	// E il numero è piccolo in assoluto, non solo uguale: una cella dipinta tocca il suo disco, e il glifo
	// solo se la superficie ne cambia il numero di anelli.
	TestTrue(*FString::Printf(TEXT("e in assoluto è il costo di una cella, non di una board: %d"), Piccola),
		Piccola <= 3);

	return true;
}


/**
 * 🔴 **N modifiche incrementali lasciano la board IDENTICA a un rebuild totale** — `#2761`.
 *
 * ## Perché questa asserzione non esisteva, e perché adesso serve
 *
 * Il rebuild per famiglia non ne aveva bisogno: buttava via tutto e rifaceva, quindi non poteva
 * disallineare niente. Il per-cella tocca istanze **in mezzo** ad array paralleli, e il difetto che può
 * introdurre non è un crash — è una board con le celle **scambiate**.
 *
 * ⛔ **Le due asserzioni che già esistevano passerebbero entrambe su una board rimescolata**: il conteggio
 * (`GetVeilCounts`, l'`ensureMsgf` di `VeilInstances`) perché il numero di istanze non cambia, e il costo
 * (`EditCostScalesWithTheEditNotTheMap`) perché misura quante ne ho toccate, non quali. È l'unica che
 * coglie una permutazione.
 *
 * ⚠️ **Il confronto è per CELLA, non per indice**, ed è deliberato: l'ordine delle istanze può
 * legittimamente differire — un glifo rimosso e riaggiunto finisce in coda — mentre ciò che deve
 * coincidere è **dove sta cosa**. Un confronto per indice fallirebbe su una board corretta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorIncrementalEqualsFullTest,
	"RefactorTactics.HexMapActor.IncrementalRepaintEqualsFullRebuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorIncrementalEqualsFullTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	// Le superfici si scelgono per FAR MUOVERE i glifi: `SurfaceRingCount` risponde diverso, quindi le
	// istanze della corona nascono, muoiono e cambiano anello. Su superfici con lo stesso numero di anelli
	// il per-cella non rimuoverebbe niente, e il test non eserciterebbe il caso che esiste per coprire.
	// ⚠️ Le voci si NOMINANO invece di iterare l'enum fino a un sentinella: `ERTHexSurface` non ne ha uno, e
	// inventarlo per un test lo farebbe entrare nel contratto dell'enum da una porta di servizio.
	const TArray<ERTHexSurface> Tavolozza = {
		ERTHexSurface::Floor, ERTHexSurface::ShallowWater, ERTHexSurface::Rough, ERTHexSurface::Fire,
		ERTHexSurface::Conductive, ERTHexSurface::Ice, ERTHexSurface::Smoke, ERTHexSurface::HighGround };

	// ⛔ **Si dipinge la sola SUPERFICIE, e non il costo di movimento.** Il blocco di sincronizzazione del
	// velo riallinea le due famiglie che [D-183] accoppia sulla superficie — disco e corona — e **non** il
	// rilievo, che dipende dal costo. Non e' un buco di questa issue: e' cosi' da `#2894`, che quel blocco lo
	// ha scritto con la maschera `Cells | Glyphs`. Dipingere il costo qui farebbe cadere il confronto per una
	// ragione che non c'entra con il per-cella, ed e' il modo piu' rapido di rendere un test bugiardo.
	auto Dipingi = [&Tavolozza](URTHexMapAsset* Asset, int32 Quante)
	{
		for (int32 K = 0; K < Quante && K < Asset->Cells.Num(); ++K)
		{
			FRTHexCellData Dipinta = Asset->Cells[K];
			Dipinta.Surface = Tavolozza[K % Tavolozza.Num()];
			Asset->AddOrUpdateCell(Dipinta);
		}
	};

	constexpr int32 Pennellate = 12;

	URTHexMapAsset* AssetIncrementale = MakeActorTestAsset(/*Radius=*/ 3);
	ARTHexMapActor* Incrementale = SpawnMapActor(World, AssetIncrementale);
	if (!TestNotNull(TEXT("actor incrementale"), Incrementale)) { return false; }

	FRTTeamKnowledge Tutto;
	for (const FRTHexCellData& C : AssetIncrementale->Cells)
	{
		Tutto.VisibleCells.Add(C.Id);
		Tutto.ExploredCells.Add(C.Id);
	}

	// ⚠️ **Un velo per pennellata**, non uno in fondo: è ciò che rende ognuna un riallineamento per-cella
	// separato. Applicandone dodici e velando una volta sola, il registro ne consegnerebbe dodici in un
	// colpo e il caso «istanza rimossa in mezzo, poi un'altra rimossa dopo» non verrebbe esercitato.
	Incrementale->ApplyKnowledgeVeil(Tutto);
	int32 ToccateInTutto = 0;
	for (int32 K = 0; K < Pennellate; ++K)
	{
		FRTHexCellData Dipinta = AssetIncrementale->Cells[K];
		Dipinta.Surface = Tavolozza[K % Tavolozza.Num()];
		AssetIncrementale->AddOrUpdateCell(Dipinta);
		Incrementale->ApplyKnowledgeVeil(Tutto);
		ToccateInTutto += Incrementale->LastRepaintTouchedInstances();
	}

	// L'altra board: stesse pennellate, ma la costruzione avviene una volta sola alla fine.
	URTHexMapAsset* AssetTotale = MakeActorTestAsset(/*Radius=*/ 3);
	Dipingi(AssetTotale, Pennellate);
	ARTHexMapActor* Totale = SpawnMapActor(World, AssetTotale);
	if (!TestNotNull(TEXT("actor totale"), Totale)) { return false; }
	Totale->RebuildInstances(ERTRebuildFamily::All);
	Totale->ApplyKnowledgeVeil(Tutto);

	// 🔑 La prova che il per-cella è stato DAVVERO usato: senza, questo test passerebbe confrontando due
	// rebuild totali e non direbbe niente.
	AddInfo(FString::Printf(TEXT("%d pennellate hanno toccato %d istanze in tutto"),
		Pennellate, ToccateInTutto));
	if (!TestTrue(TEXT("il percorso per-cella è stato usato (altrimenti il confronto è vuoto)"),
		ToccateInTutto > 0 && ToccateInTutto <= Pennellate * 3))
	{
		return false;
	}

	// Il confronto, famiglia per famiglia: dove sta ogni cella, e con quale posa.
	auto MappaturaDi = [](const ARTHexMapActor* Actor, const TCHAR* Componente)
	{
		TMap<FVector2D, FVector> Out;
		for (const FTransform& Xf : InstancesOf(Actor, Componente))
		{
			// La chiave è la posizione PLANARE, che è la firma della cella; il valore la posa completa.
			// Due istanze della stessa famiglia sulla stessa cella non esistono per disco e glifi, che sono
			// le due famiglie che il per-cella tocca.
			Out.Add(FVector2D(Xf.GetLocation().X, Xf.GetLocation().Y), Xf.GetLocation());
		}
		return Out;
	};

	// Le famiglie che il riallineamento tocca — disco e le quattro corone — piu' la griglia, che **non**
	// tocca: se il per-cella la sfiorasse per sbaglio, il confronto se ne accorgerebbe.
	const TCHAR* Famiglie[] = { TEXT("Cells"), TEXT("CellBorders"),
		TEXT("SurfaceGlyph1"), TEXT("SurfaceGlyph2"), TEXT("SurfaceGlyph3"), TEXT("SurfaceGlyph4") };
	for (const TCHAR* Famiglia : Famiglie)
	{
		const TMap<FVector2D, FVector> A = MappaturaDi(Incrementale, Famiglia);
		const TMap<FVector2D, FVector> B = MappaturaDi(Totale, Famiglia);
		TestEqual(*FString::Printf(TEXT("%s: stesso numero di istanze"), Famiglia), A.Num(), B.Num());
		for (const TPair<FVector2D, FVector>& Voce : A)
		{
			const FVector* Corrispondente = B.Find(Voce.Key);
			if (!TestNotNull(*FString::Printf(TEXT("%s: la cella a (%.0f, %.0f) esiste anche nel rebuild totale"),
					Famiglia, Voce.Key.X, Voce.Key.Y), Corrispondente))
			{
				continue;
			}
			TestTrue(*FString::Printf(TEXT("%s: e sta nello stesso posto"), Famiglia),
				Voce.Value.Equals(*Corrispondente, 1.0));
		}
	}

	// 🔴 **E il COLORE, che è la metà che una permutazione degli indici sposta senza muovere niente a
	// schermo di geometrico.** Due board con le stesse istanze nelle stesse posizioni e i colori scambiati
	// superano tutto il confronto qui sopra: è esattamente il difetto che l'issue descrive — *«celle velate
	// SBAGLIATE»* — e sarebbe l'unico a passare inosservato.
	int32 ColoriConfrontati = 0;
	int32 ColoriDiversi = 0;
	const TArray<FTransform> DischiA = InstancesOf(Incrementale, TEXT("Cells"));
	const TArray<FTransform> DischiB = InstancesOf(Totale, TEXT("Cells"));
	TMap<FVector2D, FLinearColor> ColoreTotale;
	for (int32 I = 0; I < DischiB.Num(); ++I)
	{
		FLinearColor C;
		if (Totale->GetVeilWrittenColor(I, C))
		{
			ColoreTotale.Add(FVector2D(DischiB[I].GetLocation().X, DischiB[I].GetLocation().Y), C);
		}
	}
	for (int32 I = 0; I < DischiA.Num(); ++I)
	{
		FLinearColor C;
		if (!Incrementale->GetVeilWrittenColor(I, C))
		{
			continue;
		}
		const FVector2D Chiave(DischiA[I].GetLocation().X, DischiA[I].GetLocation().Y);
		if (const FLinearColor* Atteso = ColoreTotale.Find(Chiave))
		{
			++ColoriConfrontati;
			if (!C.Equals(*Atteso, 0.01f))
			{
				++ColoriDiversi;
			}
		}
	}
	TestTrue(TEXT("i colori sono stati confrontati su tutte le celle"), ColoriConfrontati > 0);
	TestEqual(TEXT("nessuna cella porta il colore di un'altra"), ColoriDiversi, 0);

	return true;
}


/**
 * 🔴 **La rimozione per-cella segue la semantica di indice che il MOTORE usa, non una che abbiamo
 * scelto** — `#2761`.
 *
 * ## Il difetto che questo test rende impossibile
 *
 * `UInstancedStaticMeshComponent::RemoveInstanceInternal` sceglie a runtime fra due semantiche:
 *
 *     bUseRemoveAtSwap = bForceRemoveAtSwap || bSupportRemoveAtSwap
 *                     || r.InstancedStaticMeshes.ForceRemoveAtSwap != 0
 *
 * `RemoveAt` fa scalare di uno tutti gli indici successivi; `RemoveAtSwap` porta l'**ultima** istanza nel
 * posto liberato. Sono ordini diversi, e un array parallelo che ne ricopiasse una sola resterebbe della
 * **lunghezza giusta** e mappato **storto** — cioè supererebbe l'`ensureMsgf` sui conteggi.
 *
 * ⚠️ **La terza condizione è una console variable**, quindi il difetto non richiede una modifica al codice
 * per manifestarsi: basta che qualcuno accenda quella CVar. Questo test la accende e la spegne, e chiede la
 * stessa board in entrambi i casi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorRemovalSemanticsTest,
	"RefactorTactics.HexMapActor.PerCellRemovalMirrorsTheEngine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorRemovalSemanticsTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	IConsoleVariable* Force =
		IConsoleManager::Get().FindConsoleVariable(TEXT("r.InstancedStaticMeshes.ForceRemoveAtSwap"));
	if (!TestNotNull(TEXT("la CVar del motore esiste (se sparisce, questo test va riscritto, non tolto)"),
		Force))
	{
		return false;
	}
	const int32 Originale = Force->GetInt();

	// La board dipinta per-cella sotto una semantica di indice data.
	auto BoardConSemantica = [&](int32 Swap) -> TMap<FVector2D, FLinearColor>
	{
		Force->Set(Swap);

		URTHexMapAsset* Asset = MakeActorTestAsset(/*Radius=*/ 2);
		ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
		TMap<FVector2D, FLinearColor> Out;
		if (!Actor) { return Out; }

		FRTTeamKnowledge Tutto;
		for (const FRTHexCellData& C : Asset->Cells)
		{
			Tutto.VisibleCells.Add(C.Id);
			Tutto.ExploredCells.Add(C.Id);
		}
		Actor->ApplyKnowledgeVeil(Tutto);

		// Si dipinge in modo da far NASCERE e MORIRE glifi: è la rimozione che questo test esercita, e senza
		// un cambio di `SurfaceRingCount` non ne verrebbe rimosso nessuno.
		for (int32 K = 0; K < Asset->Cells.Num(); ++K)
		{
			FRTHexCellData Dipinta = Asset->Cells[K];
			Dipinta.Surface = (K % 2 == 0) ? ERTHexSurface::Floor : ERTHexSurface::Rough;
			Asset->AddOrUpdateCell(Dipinta);
			Actor->ApplyKnowledgeVeil(Tutto);
		}
		// E poi si riporta tutto a `Floor`, che rimuove i glifi appena nati.
		for (int32 K = 0; K < Asset->Cells.Num(); ++K)
		{
			FRTHexCellData Dipinta = Asset->Cells[K];
			Dipinta.Surface = ERTHexSurface::Floor;
			Asset->AddOrUpdateCell(Dipinta);
			Actor->ApplyKnowledgeVeil(Tutto);
		}

		const TArray<FTransform> Dischi = InstancesOf(Actor, TEXT("Cells"));
		for (int32 I = 0; I < Dischi.Num(); ++I)
		{
			FLinearColor C;
			if (Actor->GetVeilWrittenColor(I, C))
			{
				Out.Add(FVector2D(Dischi[I].GetLocation().X, Dischi[I].GetLocation().Y), C);
			}
		}
		return Out;
	};

	const TMap<FVector2D, FLinearColor> ConRemoveAt = BoardConSemantica(0);
	const TMap<FVector2D, FLinearColor> ConRemoveAtSwap = BoardConSemantica(1);
	Force->Set(Originale);

	if (!TestTrue(TEXT("entrambe le board si sono costruite"),
		ConRemoveAt.Num() > 0 && ConRemoveAtSwap.Num() > 0))
	{
		return false;
	}
	TestEqual(TEXT("stesso numero di celle sotto le due semantiche"),
		ConRemoveAtSwap.Num(), ConRemoveAt.Num());

	int32 Diverse = 0;
	for (const TPair<FVector2D, FLinearColor>& Voce : ConRemoveAt)
	{
		const FLinearColor* Altro = ConRemoveAtSwap.Find(Voce.Key);
		if (!Altro || !Voce.Value.Equals(*Altro, 0.01f))
		{
			++Diverse;
		}
	}
	// 🔑 Se gli array paralleli ricopiassero una semantica sola, una delle due board avrebbe i colori
	// spostati e questo numero non sarebbe zero.
	TestEqual(TEXT("la board non dipende da come il motore rimuove le istanze"), Diverse, 0);

	return true;
}


/**
 * 🔑 **Una ricostruzione parziale costa meno, e non tocca le famiglie fuori dalla maschera** — `#1865`,
 * punto 3, decisione del 2026-09-05.
 *
 * Due proprietà in un test perché sono le due metà della stessa promessa: se il costo scendesse ma le
 * famiglie escluse venissero svuotate, avremmo un numero più basso e una board rotta.
 *
 * ⚠️ **La seconda metà è quella che può fallire in silenzio.** Un `ClearInstances` dimenticato fuori dalla
 * sua condizione non cambia il conteggio della maschera — conta solo le famiglie richieste — quindi
 * sarebbe invisibile a un test scritto sul solo costo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorPartialRebuildTest,
	"RefactorTactics.HexMapActor.PartialRebuildCostsLessAndSparesTheOthers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorPartialRebuildTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	URTHexMapAsset* Asset = MakeActorTestAsset(/*Radius=*/ 3);
	// Una cella con rilievo e una che blocca: senza, `Relief` e `Blockers` sarebbero vuote e «risparmiate»
	// sarebbe vero per la ragione sbagliata.
	{
		FRTHexCellData Costosa = Asset->Cells[1];
		Costosa.MoveCost = 3;
		Asset->AddOrUpdateCell(Costosa);
		FRTHexCellData Muro = Asset->Cells[2];
		Muro.bBlocksMovement = true;
		Asset->AddOrUpdateCell(Muro);
		Asset->SortCells();
	}

	ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
	if (!TestNotNull(TEXT("actor"), Actor)) { return false; }

	Actor->RebuildInstances(ERTRebuildFamily::All);
	const int32 CostoTotale = Actor->LastRebuildCreatedInstances();
	const int32 ReliefPrima  = InstancesOf(Actor, TEXT("Relief")).Num();
	const int32 BlockerPrima = InstancesOf(Actor, TEXT("Blockers")).Num();
	const int32 CellePrima   = InstancesOf(Actor, TEXT("Cells")).Num();

	// ➕ CONTROLLO POSITIVO: le famiglie che devono sopravvivere non sono vuote in partenza.
	if (!TestTrue(TEXT("il rilievo esiste"), ReliefPrima > 0)
		|| !TestTrue(TEXT("i blocchi esistono"), BlockerPrima > 0)
		|| !TestTrue(TEXT("le celle esistono"), CellePrima > 0))
	{
		return false;
	}

	// Il caso di un cambio di `Height`: tocca i transform del pavimento e nient'altro.
	//
	// ⚠️ **Si sceglie una famiglia NON vuota apposta.** Con `Bodies` il costo parziale sarebbe `0` — la board
	// di prova non dichiara `BodyFill` — e `0 < totale` sarebbe vero senza dire quanto si guadagna: un test
	// che passa perche' non c'era niente da fare non misura un'ottimizzazione.
	Actor->RebuildInstances(ERTRebuildFamily::Cells);
	const int32 CostoParziale = Actor->LastRebuildCreatedInstances();

	AddInfo(FString::Printf(TEXT("board r=3: rebuild totale %d istanze, rebuild del solo pavimento %d"),
		CostoTotale, CostoParziale));

	// 🔑 Metà uno: costa meno.
	TestTrue(*FString::Printf(TEXT("il rebuild parziale costa meno del totale (%d < %d)"),
			CostoParziale, CostoTotale),
		CostoParziale < CostoTotale);

	// 🔑 Metà due: le altre famiglie sono INTATTE, non svuotate. È la riga che coglie un `ClearInstances`
	// lasciato fuori dalla sua condizione — invisibile al conteggio, perché quello guarda solo la maschera.
	TestEqual(TEXT("il rilievo è intatto"), InstancesOf(Actor, TEXT("Relief")).Num(), ReliefPrima);
	TestEqual(TEXT("i blocchi sono intatti"), InstancesOf(Actor, TEXT("Blockers")).Num(), BlockerPrima);
	// Le celle SONO state ricostruite — erano nella maschera — e devono essere tornate tutte: una
	// ricostruzione parziale che ne perdesse per strada sarebbe peggio di nessuna ottimizzazione.
	TestEqual(TEXT("le celle ricostruite sono tutte quelle di prima"),
		InstancesOf(Actor, TEXT("Cells")).Num(), CellePrima);

	// E il default resta il comportamento di sempre: ogni chiamante esistente non cambia.
	Actor->RebuildInstances();
	TestEqual(TEXT("senza maschera si ricostruisce tutto, come prima"),
		Actor->LastRebuildCreatedInstances(), CostoTotale);
	return true;
}

// L'anteprima semantica non muta nulla: e' l'invariante presentation-only di #1941, misurata sul CANALE che
// la issue costruisce — non sulla griglia, che ha gia' il suo `GridAddsNoActorPerCell`.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAreaOverlayDrawingIsInertTest,
	"RefactorTactics.AreaOverlay.DrawingMutatesNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAreaOverlayDrawingIsInertTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	URTHexMapAsset* Asset = MakeActorTestAsset(/*Radius=*/ 3); // 37 celle
	ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
	if (!Actor)
	{
		AddError(TEXT("actor non spawnato"));
		DestroyMapActorWorld(World);
		return false;
	}

	// Lo stato PRIMA che un solo overlay esista.
	const uint32 HashPrima = Asset->ComputeHash();
	const int32 CellePrima = Actor->NumInstanceCells();
	int32 AttoriPrima = 0;
	for (TActorIterator<AActor> It(World); It; ++It) { ++AttoriPrima; }
	TArray<UActorComponent*> ComponentiPrima;
	Actor->GetComponents(ComponentiPrima);

	// Un'anteprima densa: raggiungibili, percorso e area colpita insieme, cioe' il caso in cui piu' significati
	// convivono sulla stessa board. Se un canale creasse un Actor o un component per cella, 37 celle lo
	// renderebbero un salto e non rumore.
	TArray<FRTCellId> Raggiungibili;
	for (int32 X = -3; X <= 3; ++X)
	{
		for (int32 Y = -3; Y <= 3; ++Y)
		{
			if (FMath::Abs(X + Y) <= 3) { Raggiungibili.Add(FRTCellId(X, Y, 0)); }
		}
	}
	Actor->SetPreviewReachableCells(Raggiungibili);
	Actor->SetPreviewPath({ FRTCellId(0, 0, 0), FRTCellId(1, 0, 0), FRTCellId(2, 0, 0) });
	Actor->SetPreviewHitCells({ FRTCellId(2, 0, 0), FRTCellId(2, 1, 0) }, { FRTCellId(2, 1, 0) });

	// E' il `Tick` a chiamare `DrawPlanningPreview`: si esercita il percorso vero, non la funzione da sola.
	Actor->TickActor(0.016f, LEVELTICK_All, Actor->PrimaryActorTick);
	// Due frame: un accumulo per-frame si vedrebbe qui e non al primo giro.
	Actor->TickActor(0.016f, LEVELTICK_All, Actor->PrimaryActorTick);

	// ⛔ Presentation-only: nessuna mutazione dello stato canonico.
	TestEqual(TEXT("l'hash dell'asset non cambia: ne' FRTMapState, ne' graph revision, ne' path cache"),
		Asset->ComputeHash(), HashPrima);
	TestEqual(TEXT("la mappa istanza->cella e' intatta"), Actor->NumInstanceCells(), CellePrima);

	// ⛔ Nessun Actor per cella, misurato sul DELTA come gia' fa la griglia.
	int32 AttoriDopo = 0;
	for (TActorIterator<AActor> It(World); It; ++It) { ++AttoriDopo; }
	TestEqual(TEXT("un'anteprima densa non aggiunge NESSUN actor al mondo"), AttoriDopo, AttoriPrima);

	// ⛔ E nessun component per cella: il conteggio dei component dell'actor non si muove.
	TArray<UActorComponent*> ComponentiDopo;
	Actor->GetComponents(ComponentiDopo);
	TestEqual(TEXT("un'anteprima densa non aggiunge NESSUN component"),
		ComponentiDopo.Num(), ComponentiPrima.Num());

	// E spegnere l'anteprima la riporta inerte, senza lasciare residui nello stato canonico.
	Actor->SetPreviewReachableCells({});
	Actor->SetPreviewPath({});
	Actor->SetPreviewHitCells({}, {});
	Actor->TickActor(0.016f, LEVELTICK_All, Actor->PrimaryActorTick);
	TestEqual(TEXT("spenta l'anteprima, l'hash e' ancora quello di partenza"), Asset->ComputeHash(), HashPrima);

	DestroyMapActorWorld(World);
	return true;
}



// ---------------------------------------------------------------------------------------------------------

/**
 * Il canale di PLAYBACK dell'impronta e' additivo, indipendente dall'anteprima, e si spegne — `#2454`.
 *
 * 🔑 **Il punto del test e' l'indipendenza dai due cicli di vita.** `SetPreviewHitCells` mostra cio' che
 * *accadrebbe* e muore al lock-in; `AddPlaybackFootprint` mostra cio' che **e' accaduto** e muore a
 * `FinishPlayback`. Se un giorno qualcuno li fondesse in un array solo, spegnere l'anteprima cancellerebbe
 * un fatto gia' avvenuto — e questo test cade.
 *
 * ⚠️ Verifica cio' che l'actor **riceve**, non cio' che disegna: il disegno non e' misurabile senza schermo,
 * e il suo giudizio appartiene alla voce PIE.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorPlaybackFootprintChannelTest,
	"RefactorTactics.HexMapActor.PlaybackFootprintIsItsOwnChannel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorPlaybackFootprintChannelTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	URTHexMapAsset* Asset = MakeActorTestAsset(/*Radius*/ 1);
	ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
	TestNotNull(TEXT("actor spawnato"), Actor);
	if (!Actor) { DestroyMapActorWorld(World); return false; }

	const FRTCellId A(0, 0);
	const FRTCellId B(1, 0);
	const FRTCellId C(0, 1);

	TestEqual(TEXT("si parte senza impronta"), Actor->NumPlaybackFootprintCells(), 0);

	// Prima impronta: le celle arrivano cosi' come sono passate.
	Actor->AddPlaybackFootprint({ A, B });
	TestEqual(TEXT("due celle dopo la prima impronta"), Actor->NumPlaybackFootprintCells(), 2);
	TestTrue(TEXT("A e' nell'impronta"), Actor->IsPlaybackFootprintCell(A));
	TestTrue(TEXT("B e' nell'impronta"), Actor->IsPlaybackFootprintCell(B));
	TestFalse(TEXT("C non lo e' ancora"), Actor->IsPlaybackFootprintCell(C));

	// 🔴 **Additivo: `un evento -> un segnale`.** Due impronte nello stesso Blast sono due fatti, e la
	// seconda non sostituisce la prima. Se qualcuno cambiasse `Append` in assegnazione, questa riga cade.
	Actor->AddPlaybackFootprint({ C });
	TestEqual(TEXT("tre celle dopo la seconda impronta"), Actor->NumPlaybackFootprintCells(), 3);
	TestTrue(TEXT("A e' ancora li'"), Actor->IsPlaybackFootprintCell(A));
	TestTrue(TEXT("e C si e' aggiunta"), Actor->IsPlaybackFootprintCell(C));

	// ⛔ **I due canali non si toccano.** Spegnere l'anteprima non cancella un fatto gia' avvenuto.
	Actor->SetPreviewHitCells({ A }, {});
	Actor->SetPreviewHitCells({}, {});
	TestEqual(TEXT("spenta l'anteprima, l'impronta di playback resta"),
		Actor->NumPlaybackFootprintCells(), 3);

	// … e viceversa: e' `FinishPlayback` a spegnere questo canale, e nessun altro.
	Actor->ClearPlaybackFootprint();
	TestEqual(TEXT("dopo Clear non resta nessuna cella"), Actor->NumPlaybackFootprintCells(), 0);
	TestFalse(TEXT("e nessuna cella risponde piu' vero"), Actor->IsPlaybackFootprintCell(A));

	DestroyMapActorWorld(World);
	return true;
}

/**
 * 🔴 **`#2731` — IL LEAK: il corpo strutturale si vede sotto una cella mai osservata.**
 *
 * `StructuralBodies` era **l'unica famiglia visiva che `ApplyKnowledgeVeil` non poteva nascondere**, e non
 * per una riga dimenticata: il suo sito di `AddInstance` non registrava nessuna cella, quindi non esisteva
 * un indice da velare — `BodyCells` non compariva da nessuna parte nel repository.
 *
 * ⛔ **E' un leak, non un difetto estetico**: [D-225] dice *«mai vista: non si disegna»*, e su una mappa con
 * `BodyFill != None` il volume solido restava visibile sotto celle che la squadra non aveva mai osservato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilStructuralBodyIsHiddenTest,
	"RefactorTactics.Veil.StructuralBodyDisappearsUnderAnUnobservedCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilStructuralBodyIsHiddenTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	// La stessa colonna del test del derivatore: una superficie a L1 che dichiara il corpo, una a L0 no.
	URTHexMapAsset* Asset = NewObject<URTHexMapAsset>(GetTransientPackage());
	const FRTCellId CellaSotto(0, 0, 0);
	const FRTCellId CellaCorpo(0, 0, 1);
	{
		Asset->AddOrUpdateCell(FRTHexCellData(CellaSotto));
		FRTHexCellData Sopra(CellaCorpo);
		Sopra.BodyFill = ERTHexBodyFill::Full;
		Asset->AddOrUpdateCell(Sopra);
		Asset->SortCells();
	}

	ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
	if (!TestNotNull(TEXT("actor mappa"), Actor)) { DestroyMapActorWorld(World); return false; }

	// Premessa: il corpo c'e' ed e' DISEGNATO. Senza, il test non misurerebbe niente.
	{
		const TArray<FTransform> Corpi = InstancesOf(Actor, TEXT("StructuralBodies"));
		if (!TestEqual(TEXT("premessa: un corpo posato"), Corpi.Num(), 1))
		{
			DestroyMapActorWorld(World);
			return false;
		}
		TestFalse(TEXT("premessa: nasce disegnato"), Corpi[0].GetScale3D().IsNearlyZero());
	}

	// 🔴 La tesi: una conoscenza che NON contiene la cella del corpo lo deve far sparire.
	{
		FRTTeamKnowledge K;
		K.Version = FRTTeamKnowledge::CurrentVersion;
		K.TeamId = 0;
		K.VisibleCells = { CellaSotto };
		K.ExploredCells = { CellaSotto };
		Actor->ApplyKnowledgeVeil(K);

		const TArray<FTransform> Corpi = InstancesOf(Actor, TEXT("StructuralBodies"));
		if (TestEqual(TEXT("il corpo e' ancora una istanza"), Corpi.Num(), 1))
		{
			TestTrue(TEXT("sotto una cella MAI OSSERVATA il corpo non si disegna ([D-225])"),
				Corpi[0].GetScale3D().IsNearlyZero());
		}
	}

	// ➕ Reversibile: osservata, il corpo torna. Senza questo, «scala zero sempre» passerebbe il test sopra.
	{
		FRTTeamKnowledge K;
		K.Version = FRTTeamKnowledge::CurrentVersion;
		K.TeamId = 0;
		K.VisibleCells = { CellaSotto, CellaCorpo };
		K.ExploredCells = { CellaSotto, CellaCorpo };
		Actor->ApplyKnowledgeVeil(K);

		const TArray<FTransform> Corpi = InstancesOf(Actor, TEXT("StructuralBodies"));
		if (Corpi.Num() == 1)
		{
			TestFalse(TEXT("e torna disegnato quando la cella e' osservata"),
				Corpi[0].GetScale3D().IsNearlyZero());
		}
	}

	DestroyMapActorWorld(World);
	return true;
}

/**
 * 🔑 **IL GUARDIANO CHE COPRE LA FAMIGLIA, NON IL CASO** (`#2731`, terza voce della DoD).
 *
 * Il difetto che questa issue chiude e' vissuto fino a una code review perche' **nessun oracolo guardava le
 * famiglie**: `GetVeilCounts` legge il solo `Cells`, quindi la copertura era cieca per costruzione su tutte
 * le altre. Aggiungere il corpo strutturale a un elenco avrebbe spostato la stessa dimenticanza un livello
 * piu' su — *«la decima nascera' con lo stesso buco»*.
 *
 * 🔴 **Quindi qui non c'e' nessun elenco.** Il test **enumera i componenti dell'attore** e chiede a ciascuno
 * quante istanze restino disegnate sotto una conoscenza **vuota**. Una famiglia nuova si presenta da sola:
 * chi la aggiunge senza velarla trova questo test rosso, e deve o velarla o dichiarare per iscritto perche'
 * non vada velata.
 *
 * ⚠️ **Un componente senza istanze passa senza dire niente**, ed e' corretto — non c'e' niente da nascondere
 * — ma renderebbe il test vacuo se fosse il caso di tutti: l'asserzione di anti-vacuita' pretende che almeno
 * tre famiglie siano popolate.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilEveryFamilyDisappearsTest,
	"RefactorTactics.Veil.EveryInstanceFamilyDisappearsUnderAnEmptyKnowledge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilEveryFamilyDisappearsTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	// Una board RICCA: `MakeTestArena` porta ostacoli, muri, terreno costoso e una piattaforma, quindi
	// popola piu' famiglie di una graybox piatta. Il corpo strutturale lo si dichiara a mano, perche'
	// nessuna arena generata lo fa.
	URTHexMapAsset* Asset = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("arena di prova"), Asset)) { DestroyMapActorWorld(World); return false; }
	{
		TArray<FRTHexCellData> Celle = Asset->Cells;
		int32 Dichiarati = 0;
		for (FRTHexCellData& C : Celle)
		{
			if (C.Id.Layer == 0 && Dichiarati < 3)
			{
				C.BodyFill = ERTHexBodyFill::Full;
				++Dichiarati;
			}
		}
		Asset->UpdateCells(Celle);
	}

	ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
	if (!TestNotNull(TEXT("actor mappa"), Actor)) { DestroyMapActorWorld(World); return false; }

	// Prima: quante famiglie hanno davvero delle istanze. E' la premessa del test.
	TArray<UInstancedStaticMeshComponent*> Famiglie;
	Actor->GetComponents(Famiglie);
	int32 Popolate = 0;
	for (const UInstancedStaticMeshComponent* F : Famiglie)
	{
		if (F && F->GetInstanceCount() > 0) { ++Popolate; }
	}
	AddInfo(FString::Printf(TEXT("componenti ISM: %d, di cui popolati: %d"), Famiglie.Num(), Popolate));
	if (!TestTrue(*FString::Printf(TEXT("premessa: almeno tre famiglie popolate (%d)"), Popolate),
			Popolate >= 3))
	{
		DestroyMapActorWorld(World);
		return false;
	}

	// 🔴 Conoscenza VUOTA: la squadra non ha mai osservato niente, quindi nulla dev'essere disegnato.
	FRTTeamKnowledge Nulla;
	Nulla.Version = FRTTeamKnowledge::CurrentVersion;
	Nulla.TeamId = 0;
	Actor->ApplyKnowledgeVeil(Nulla);

	TArray<FString> Scoperte;
	for (const UInstancedStaticMeshComponent* F : Famiglie)
	{
		if (!F) { continue; }
		int32 Disegnate = 0;
		for (int32 I = 0; I < F->GetInstanceCount(); ++I)
		{
			FTransform Xf;
			if (F->GetInstanceTransform(I, Xf, /*bWorldSpace=*/ true)
				&& !Xf.GetScale3D().IsNearlyZero())
			{
				++Disegnate;
			}
		}
		if (Disegnate > 0)
		{
			Scoperte.Add(FString::Printf(TEXT("%s (%d istanze disegnate)"), *F->GetName(), Disegnate));
		}
	}

	if (Scoperte.Num() > 0)
	{
		AddInfo(FString::Printf(TEXT("famiglie NON velate: %s"), *FString::Join(Scoperte, TEXT(", "))));
	}
	TestEqual(*FString::Printf(
			TEXT("sotto una conoscenza vuota nessuna famiglia resta disegnata (scoperte: %d)"), Scoperte.Num()),
		Scoperte.Num(), 0);

	DestroyMapActorWorld(World);
	return true;
}


#endif // WITH_DEV_AUTOMATION_TESTS
