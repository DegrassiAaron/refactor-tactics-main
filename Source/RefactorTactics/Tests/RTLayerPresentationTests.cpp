// I piani rispetto a quello attivo (#3679, [D-501]): sopra traslucidi, sotto spenti, il bordo dell'attivo marcato.
//
// 🔑 **Questi test misurano i DATI che la board scrive, non cio' che si vede.** Il trattamento sta nei canali 3..5 dei
// custom data per istanza, e lo applica `M_HexCell`: che il materiale li legga, e che a schermo si capisca quale piano
// e' attivo, resta della voce PIE di #3679. Qui si misura che i canali giusti arrivino alle istanze giuste, che il
// controller sia l'ingresso, e che il velo e la collisione non ne sappiano niente.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTLayerPresentation.h"
#include "Perception/RTTeamKnowledge.h"
#include "Player/RTPlayerController.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Unit/RTUnit.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	UInstancedStaticMeshComponent* LayerTreatmentIsm(const ARTHexMapActor* Board, const TCHAR* Name)
	{
		TArray<UInstancedStaticMeshComponent*> Isms;
		Board->GetComponents(Isms);
		for (UInstancedStaticMeshComponent* Ism : Isms)
		{
			if (Ism && Ism->GetName() == Name) { return Ism; }
		}
		return nullptr;
	}

	float LayerTreatmentChannel(const UInstancedStaticMeshComponent* Ism, int32 Instance, int32 Channel)
	{
		const int32 Indice = Instance * Ism->NumCustomDataFloats + Channel;
		return Ism->PerInstanceSMCustomData.IsValidIndex(Indice) ? Ism->PerInstanceSMCustomData[Indice] : -999.f;
	}

	/** La board della fixture a tre piani (#3678): 0 di raggio 4, 1 di raggio 2, 2 di raggio 1, impilati. */
	ARTHexMapActor* SpawnLayerTreatmentBoard(UWorld* World, URTHexMapAsset* Asset)
	{
		ARTHexMapActor* Board = World->SpawnActorDeferred<ARTHexMapActor>(ARTHexMapActor::StaticClass(), FTransform::Identity);
		if (Board)
		{
			Board->MapAsset = Asset;
			Board->FinishSpawning(FTransform::Identity); // `OnConstruction` ricostruisce le istanze
		}
		return Board;
	}

	int32 FirstInstanceOnLayer(const ARTHexMapActor* Board, int32 Layer)
	{
		for (int32 I = 0; I < Board->NumInstanceCells(); ++I)
		{
			if (Board->CellForInstance(I).Layer == Layer) { return I; }
		}
		return INDEX_NONE;
	}
}

/**
 * **Il trattamento si ricava da (piano, piano attivo)** — la funzione pura della DoD di #3679.
 *
 * ⚠️ Il controllo «il secondo piano sotto e' piu' spento del primo» e' quello che rende il trattamento CRESCENTE con la
 * distanza, come chiede la issue: un trattamento uguale per tutti i piani sotto passerebbe gli altri controlli.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerTreatmentPureTest,
	"RefactorTactics.HexMap.LayerTreatmentFollowsTheActiveLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerTreatmentPureTest::RunTest(const FString&)
{
	const FRTLayerPresentationTuning T;

	TestTrue(TEXT("una mappa a un piano non ha trattamento, nemmeno il bordo"),
		RTLayerPresentation::Treatment(0, 0, /*bMultiLayer=*/ false, T).IsNeutral());
	TestTrue(TEXT("nemmeno su un piano che non e' l'attivo"),
		RTLayerPresentation::Treatment(1, 0, /*bMultiLayer=*/ false, T).IsNeutral());

	const FRTLayerTreatment Attivo = RTLayerPresentation::Treatment(1, 1, true, T);
	TestEqual(TEXT("l'attivo e' opaco"), Attivo.Transparency, 0.f);
	TestEqual(TEXT("l'attivo ha i colori pieni"), Attivo.Desaturation, 0.f);
	TestEqual(TEXT("l'attivo ha la sua luce"), Attivo.ExposureStops, 0.f);
	TestEqual(TEXT("e il bordo marcato"), Attivo.BorderExposureStops, T.ActiveBorderStops);

	const FRTLayerTreatment Sopra = RTLayerPresentation::Treatment(2, 1, true, T);
	TestTrue(TEXT("sopra e' traslucido"), FMath::IsNearlyEqual(Sopra.Transparency, 1.f - T.UpperOpacity));
	TestTrue(TEXT("sopra non e' spento: si deve vedere attraverso, non leggerlo come lontano"),
		Sopra.Desaturation == 0.f && Sopra.ExposureStops == 0.f);
	TestEqual(TEXT("e il suo bordo non e' marcato"), Sopra.BorderExposureStops, 0.f);

	const FRTLayerTreatment Sotto1 = RTLayerPresentation::Treatment(1, 2, true, T);
	const FRTLayerTreatment Sotto2 = RTLayerPresentation::Treatment(0, 2, true, T);
	TestEqual(TEXT("sotto e' opaco"), Sotto1.Transparency, 0.f);
	TestTrue(TEXT("un piano sotto: desaturato"), FMath::IsNearlyEqual(Sotto1.Desaturation, T.LowerDesaturationPerLayer));
	TestTrue(TEXT("un piano sotto: scurito"),
		FMath::IsNearlyEqual(Sotto1.ExposureStops, FMath::Log2(1.f - T.LowerDarkeningPerLayer), 1.e-4f));
	TestTrue(TEXT("due piani sotto: piu' desaturato di uno"), Sotto2.Desaturation > Sotto1.Desaturation);
	TestTrue(TEXT("due piani sotto: piu' scuro di uno"), Sotto2.ExposureStops < Sotto1.ExposureStops);
	TestEqual(TEXT("il bordo sotto segue la sua cella"), Sotto1.BorderExposureStops, Sotto1.ExposureStops);
	return true;
}

/**
 * 🔴 **Il piano attivo del CONTROLLER arriva alla board, e `ARTHexMapActor::ActiveLayer` resta com'e'** (#3679, scope 1).
 *
 * ⛔ Quel campo e' lo stato di authoring dell'editor mode (D-255): se il trattamento lo leggesse, o lo scrivesse, i due
 * scrittori su un campo solo sarebbero tornati.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerTreatmentFromControllerTest,
	"RefactorTactics.HexMap.ActiveLayerReachesTheBoardAsPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerTreatmentFromControllerTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTHexMapActor* Board = SpawnLayerTreatmentBoard(World, URTMatchSetupLibrary::MakeLayerStackArena(World));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("la board"), Board) || !TestNotNull(TEXT("il controller"), PC)) { return false; }
	UInstancedStaticMeshComponent* Cells = LayerTreatmentIsm(Board, TEXT("Cells"));
	UInstancedStaticMeshComponent* Bordi = LayerTreatmentIsm(Board, TEXT("CellBorders"));
	if (!TestNotNull(TEXT("le celle"), Cells) || !TestNotNull(TEXT("la griglia"), Bordi)) { return false; }

	const int32 Su0 = FirstInstanceOnLayer(Board, 0);
	const int32 Su1 = FirstInstanceOnLayer(Board, 1);
	const int32 Su2 = FirstInstanceOnLayer(Board, 2);
	if (!TestTrue(TEXT("premessa: istanze su tutti e tre i piani"), Su0 != INDEX_NONE && Su1 != INDEX_NONE && Su2 != INDEX_NONE)
		|| !TestEqual(TEXT("premessa: la griglia e' una per cella, nell'ordine delle celle"),
			Bordi->GetInstanceCount(), Cells->GetInstanceCount()))
	{
		return false;
	}
	const int32 Ch = RTLayerPresentation::FirstChannel;

	// Prima che il controller dica qualcosa la board e' neutra: e' la board di prima di #3679.
	TestEqual(TEXT("premessa: nessuno ha ancora detto il piano"), Board->GetPresentedActiveLayer(), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("premessa: il piano 2 nasce opaco"), LayerTreatmentChannel(Cells, Su2, Ch), 0.f);

	const int32 AuthoringPrima = Board->ActiveLayer;
	TestTrue(TEXT("il controller sale al piano 1"), PC->StepActiveLayer(+1));
	TestEqual(TEXT("la board presenta il piano 1"), Board->GetPresentedActiveLayer(), 1);
	TestEqual(TEXT("e il suo ActiveLayer d'authoring non e' cambiato"), Board->ActiveLayer, AuthoringPrima);

	const FRTLayerPresentationTuning T;
	TestTrue(TEXT("piano 2, sopra: traslucido"),
		FMath::IsNearlyEqual(LayerTreatmentChannel(Cells, Su2, Ch), 1.f - T.UpperOpacity));
	TestEqual(TEXT("piano 1, attivo: opaco"), LayerTreatmentChannel(Cells, Su1, Ch), 0.f);
	TestEqual(TEXT("piano 1, attivo: colori pieni"), LayerTreatmentChannel(Cells, Su1, Ch + 1), 0.f);
	TestTrue(TEXT("piano 0, sotto: desaturato"),
		FMath::IsNearlyEqual(LayerTreatmentChannel(Cells, Su0, Ch + 1), T.LowerDesaturationPerLayer));
	TestTrue(TEXT("piano 0, sotto: scurito"), LayerTreatmentChannel(Cells, Su0, Ch + 2) < 0.f);
	TestEqual(TEXT("il bordo del piano attivo e' marcato"), LayerTreatmentChannel(Bordi, Su1, Ch + 2), T.ActiveBorderStops);
	TestEqual(TEXT("la cella del piano attivo no"), LayerTreatmentChannel(Cells, Su1, Ch + 2), 0.f);

	// Il cambio successivo sposta il trattamento: sceso al piano 0, il piano 1 e' sopra.
	TestTrue(TEXT("il controller scende al piano 0"), PC->StepActiveLayer(-1));
	TestEqual(TEXT("il piano 0 torna pieno"), LayerTreatmentChannel(Cells, Su0, Ch + 1), 0.f);
	TestTrue(TEXT("e il piano 1 e' traslucido"), LayerTreatmentChannel(Cells, Su1, Ch) > 0.f);
	return true;
}

/**
 * 🔴 **Il velo viene prima: una cella mai vista resta non disegnata su qualunque piano** (#3679, vincolo 2).
 *
 * Il trattamento scrive i canali 3..5 e non la scala, e il velo nasconde azzerando la scala ([D-225]): una cella del
 * piano 2 mai vista resta a scala zero anche quando il piano 2 diventa traslucido. E i canali del colore, che sono del
 * velo, non cambiano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerTreatmentKeepsTheVeilTest,
	"RefactorTactics.HexMap.LayerTreatmentLeavesTheVeilAlone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerTreatmentKeepsTheVeilTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTHexMapActor* Board = SpawnLayerTreatmentBoard(World, URTMatchSetupLibrary::MakeLayerStackArena(World));
	UInstancedStaticMeshComponent* Cells = Board ? LayerTreatmentIsm(Board, TEXT("Cells")) : nullptr;
	if (!TestNotNull(TEXT("le celle della board"), Cells)) { return false; }

	// La squadra conosce i piani 0 e 1, non il 2.
	FRTTeamKnowledge Conosce;
	for (int32 I = 0; I < Board->NumInstanceCells(); ++I)
	{
		const FRTCellId Cella = Board->CellForInstance(I);
		if (Cella.Layer < 2)
		{
			Conosce.VisibleCells.Add(Cella);
			Conosce.ExploredCells.Add(Cella);
		}
	}
	Board->ApplyKnowledgeVeil(Conosce);

	const int32 Mai = FirstInstanceOnLayer(Board, 2);
	const int32 Vista = FirstInstanceOnLayer(Board, 0);
	FTransform Xf;
	if (!TestTrue(TEXT("premessa: la cella mai vista del piano 2 e' a scala zero"),
		Mai != INDEX_NONE && Cells->GetInstanceTransform(Mai, Xf, true) && Xf.GetScale3D().IsNearlyZero()))
	{
		return false;
	}
	float Colore[3];
	for (int32 C = 0; C < 3; ++C) { Colore[C] = LayerTreatmentChannel(Cells, Vista, C); }

	// Il piano attivo e' 0: il 2 sta sopra, cioe' traslucido.
	Board->SyncActiveLayerPresentation(0);
	TestTrue(TEXT("il trattamento e' arrivato al piano 2"), LayerTreatmentChannel(Cells, Mai, RTLayerPresentation::FirstChannel) > 0.f);
	TestTrue(TEXT("ma la cella mai vista resta a scala zero"),
		Cells->GetInstanceTransform(Mai, Xf, true) && Xf.GetScale3D().IsNearlyZero());
	for (int32 C = 0; C < 3; ++C)
	{
		TestEqual(*FString::Printf(TEXT("il canale %d del colore di una cella vista non cambia"), C),
			LayerTreatmentChannel(Cells, Vista, C), Colore[C]);
	}

	// E nell'altro verso: un velo dopo il trattamento non lo cancella.
	Conosce.VisibleCells.Reset();
	Board->ApplyKnowledgeVeil(Conosce);
	TestTrue(TEXT("un velo riscritto lascia il trattamento dov'era"),
		LayerTreatmentChannel(Cells, Mai, RTLayerPresentation::FirstChannel) > 0.f);
	return true;
}

/**
 * **Una ricostruzione non riporta la board piatta** (#3679). Le istanze rinascono coi canali a zero, e la board le
 * riscrive col piano che aveva: la vista strategica, per esempio, ricostruisce tutto a ogni cambio di separazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerTreatmentSurvivesRebuildTest,
	"RefactorTactics.HexMap.RebuildKeepsTheLayerTreatment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerTreatmentSurvivesRebuildTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTHexMapActor* Board = SpawnLayerTreatmentBoard(World, URTMatchSetupLibrary::MakeLayerStackArena(World));
	UInstancedStaticMeshComponent* Cells = Board ? LayerTreatmentIsm(Board, TEXT("Cells")) : nullptr;
	if (!TestNotNull(TEXT("le celle della board"), Cells)) { return false; }

	Board->SyncActiveLayerPresentation(1);
	Board->RebuildInstances();

	const int32 Su2 = FirstInstanceOnLayer(Board, 2);
	const int32 Su0 = FirstInstanceOnLayer(Board, 0);
	TestTrue(TEXT("dopo la ricostruzione il piano 2 e' ancora traslucido"),
		LayerTreatmentChannel(Cells, Su2, RTLayerPresentation::FirstChannel) > 0.f);
	TestTrue(TEXT("e il piano 0 ancora desaturato"),
		LayerTreatmentChannel(Cells, Su0, RTLayerPresentation::FirstChannel + 1) > 0.f);
	return true;
}

/** **Una mappa a un piano si vede come prima di #3679**: nessun canale scritto, nemmeno l'enfasi del bordo (scope 6). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerTreatmentSingleLayerTest,
	"RefactorTactics.HexMap.SingleLayerMapHasNoLayerTreatment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerTreatmentSingleLayerTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	URTHexMapAsset* Asset = NewObject<URTHexMapAsset>(World);
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), /*Radius=*/ 2))
	{
		Asset->AddOrUpdateCell(FRTHexCellData(Id));
	}
	Asset->SortCells();
	ARTHexMapActor* Board = SpawnLayerTreatmentBoard(World, Asset);
	UInstancedStaticMeshComponent* Cells = Board ? LayerTreatmentIsm(Board, TEXT("Cells")) : nullptr;
	UInstancedStaticMeshComponent* Bordi = Board ? LayerTreatmentIsm(Board, TEXT("CellBorders")) : nullptr;
	if (!TestNotNull(TEXT("le celle"), Cells) || !TestNotNull(TEXT("la griglia"), Bordi)) { return false; }

	TestTrue(TEXT("la board accetta il piano"), Board->SyncActiveLayerPresentation(0));
	int32 Scritti = 0;
	for (const UInstancedStaticMeshComponent* Ism : { Cells, Bordi })
	{
		for (int32 I = 0; I < Ism->GetInstanceCount(); ++I)
		{
			for (int32 C = RTLayerPresentation::FirstChannel; C < RTLayerPresentation::NumCustomDataFloats; ++C)
			{
				Scritti += LayerTreatmentChannel(Ism, I, C) != 0.f ? 1 : 0;
			}
		}
	}
	TestEqual(TEXT("nessun canale del trattamento diverso da zero"), Scritti, 0);
	return true;
}

/**
 * 🔴 **Un raggio che attraversa un piano sopra quello attivo risolve sulla cella del piano attivo** (#3679, D-255).
 *
 * Il trattamento e' solo presentazione: la collisione delle celle non cambia, quindi il raggio colpisce ancora il
 * piano traslucido. E' il picking di D-255 a scartare quel colpo, perche' non e' sul piano attivo, e a ripiegare sul
 * piano attivo lungo lo stesso raggio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerTreatmentRayTest,
	"RefactorTactics.PlayerInput.ARayThroughATranslucentLayerResolvesOnTheActiveLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerTreatmentRayTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	URTHexMapAsset* Asset = URTMatchSetupLibrary::MakeLayerStackArena(World);
	ARTHexMapActor* Board = SpawnLayerTreatmentBoard(World, Asset);
	UInstancedStaticMeshComponent* Cells = Board ? LayerTreatmentIsm(Board, TEXT("Cells")) : nullptr;
	if (!TestNotNull(TEXT("le celle della board"), Cells)) { return false; }

	const ECollisionEnabled::Type Prima = Cells->GetCollisionEnabled();
	Board->SyncActiveLayerPresentation(0);
	TestEqual(TEXT("il trattamento non tocca la collisione delle celle"),
		static_cast<int32>(Cells->GetCollisionEnabled()), static_cast<int32>(Prima));

	// Un raggio verticale sulla cima, (2,-2) al piano 2: il primo colpo e' li', due piani sopra l'attivo.
	FVector Origin; float HexSize = 0.f; float LayerH = 0.f;
	Board->GetHexContext(Origin, HexSize, LayerH);
	const FRTCellId Cima(2, -2, 2);
	const FVector Colpo = URTHexLibrary::AxialToWorld(Cima, Origin, HexSize, LayerH);
	const int32 PianoDelColpo = URTHexLibrary::WorldToLayer(Colpo.Z, Origin.Z, LayerH);
	TestEqual(TEXT("premessa: il colpo e' sul piano 2"), PianoDelColpo, 2);

	const bool bColpoValido = PianoDelColpo == 0; // la regola di `ResolveCellUnderCursor`: vale solo sul piano attivo
	const FRTCellId Risolta = URTHexLibrary::ResolveRayToCellOnLayer(Colpo + FVector(0, 0, 1000), FVector(0, 0, -1),
		Origin, HexSize, LayerH, /*ActiveLayer=*/ 0, bColpoValido, Colpo);
	TestEqual(TEXT("si risolve sul piano attivo"), Risolta.Layer, 0);
	TestTrue(TEXT("sotto la cima, non sulla cima"), Risolta.X == Cima.X && Risolta.Y == Cima.Y);
	TestTrue(TEXT("e la cella esiste sul piano attivo"), Asset->ContainsCell(Risolta));
	return true;
}

/**
 * **Le unita' non ricevono il trattamento** (#3679, scope 5): anello, targa e modello restano pieni su qualunque
 * piano. Il trattamento scrive solo le famiglie della board; qui si guarda che i componenti di un'unita' sul piano 2
 * — sopra l'attivo, quindi traslucido — escano dalla scrittura come ci sono entrati. E i fantasmi del piano, che usano
 * lo stesso materiale delle celle, non hanno i canali del trattamento: restano pieni come le unita'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerTreatmentSparesUnitsTest,
	"RefactorTactics.HexMap.UnitsDoNotTakeTheLayerTreatment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerTreatmentSparesUnitsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTHexMapActor* Board = SpawnLayerTreatmentBoard(World, URTMatchSetupLibrary::MakeLayerStackArena(World));
	if (!TestNotNull(TEXT("la board"), Board)) { return false; }
	FVector Origin; float HexSize = 0.f; float LayerH = 0.f;
	Board->GetHexContext(Origin, HexSize, LayerH);

	ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("l'unita'"), Unit)) { return false; }
	Unit->TeamId = 0;
	Unit->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeAevik());
	UGameplayStatics::FinishSpawning(Unit, FTransform::Identity);
	Unit->DispatchBeginPlay();
	Unit->PlaceOnCell(FRTCellId(2, -2, 2), Origin, HexSize, LayerH);

	// Lo stato di ogni componente visivo dell'unita': dati primitivi per istanza e materiali.
	struct FStato { TArray<float> Dati; TArray<UMaterialInterface*> Materiali; };
	auto Fotografa = [Unit]()
	{
		TMap<const UPrimitiveComponent*, FStato> Out;
		TArray<UPrimitiveComponent*> Primitivi;
		Unit->GetComponents(Primitivi);
		for (const UPrimitiveComponent* P : Primitivi)
		{
			FStato S;
			S.Dati = P->GetCustomPrimitiveData().Data;
			for (int32 M = 0; M < P->GetNumMaterials(); ++M) { S.Materiali.Add(P->GetMaterial(M)); }
			Out.Add(P, S);
		}
		return Out;
	};
	const TMap<const UPrimitiveComponent*, FStato> Prima = Fotografa();
	if (!TestTrue(TEXT("premessa: l'unita' ha componenti visivi"), Prima.Num() > 0)) { return false; }

	Board->SyncActiveLayerPresentation(0); // l'unita' sta due piani sopra l'attivo
	const TMap<const UPrimitiveComponent*, FStato> Dopo = Fotografa();
	for (const TPair<const UPrimitiveComponent*, FStato>& P : Prima)
	{
		const FStato* D = Dopo.Find(P.Key);
		TestTrue(*FString::Printf(TEXT("%s: dati e materiali invariati"), *P.Key->GetName()),
			D != nullptr && D->Dati == P.Value.Dati && D->Materiali == P.Value.Materiali);
	}

	const UInstancedStaticMeshComponent* Fantasmi = LayerTreatmentIsm(Board, TEXT("PlanGhosts"));
	if (TestNotNull(TEXT("i fantasmi del piano"), Fantasmi))
	{
		TestTrue(TEXT("i fantasmi non hanno i canali del trattamento"),
			Fantasmi->NumCustomDataFloats <= RTLayerPresentation::FirstChannel);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
