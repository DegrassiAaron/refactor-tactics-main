// LA PIANTA: il terreno ricordato si disegna a linee, non attenuato (#3760).
//
// L'autore ha scelto «Pianta» (2026-10-14): *«la cella ricordata diventa un disegno a linee monocromo, quasi
// olografico. Si capisce la forma, non che e' il presente.»* Sulle tavole ha scelto poi il ciano chiaro, e muri,
// coperture e porte ricordati come linee della pianta. Questi test fissano le tre cose che la scelta dice:
//
//  - una cella ricordata non ha disco e ha l'anello nel colore della pianta, e i tre stati restano tre;
//  - il colore della pianta non si confonde con cio' che si vede adesso;
//  - un muro ricordato diventa una linea della pianta, e torna solido quando lo si rivede.

#include "Misc/AutomationTest.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/LineBatchComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTLayerPresentation.h"
#include "Perception/RTTeamKnowledge.h"
#include "RTConsoleVariableGuardForTest.h"
#include "RTWorldFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: in unity build i test condividono la TU. */
	ARTHexMapActor* MakePiantaBoard(UWorld* World, URTHexMapAsset* Map)
	{
		ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
		if (!HexMap)
		{
			return nullptr;
		}
		HexMap->MapAsset = Map;
		HexMap->RebuildInstances();
		return HexMap;
	}

	FRTTeamKnowledge ConoscenzaPianta(const TArray<FRTCellId>& Visibili, const TArray<FRTCellId>& Esplorate)
	{
		FRTTeamKnowledge K;
		K.Version = FRTTeamKnowledge::CurrentVersion;
		K.TeamId = 0;
		K.TurnNumber = 1;
		K.VisibleCells = Visibili;
		K.ExploredCells = Esplorate;
		return K;
	}

	const UInstancedStaticMeshComponent* IsmPianta(const ARTHexMapActor* HexMap, const TCHAR* Nome)
	{
		TInlineComponentArray<UInstancedStaticMeshComponent*> Tutte;
		HexMap->GetComponents(Tutte);
		for (const UInstancedStaticMeshComponent* C : Tutte)
		{
			if (C && C->GetName() == Nome)
			{
				return C;
			}
		}
		return nullptr;
	}

	/** L'istanza del disco di una cella. Disco e anello sono uno-a-uno: lo stesso indice vale per l'anello. */
	int32 IstanzaPianta(const ARTHexMapActor* HexMap, const FRTCellId& Cella)
	{
		for (int32 I = 0; I < HexMap->NumInstanceCells(); ++I)
		{
			if (HexMap->CellForInstance(I) == Cella)
			{
				return I;
			}
		}
		return INDEX_NONE;
	}

	bool DisegnataPianta(const UInstancedStaticMeshComponent* C, int32 I)
	{
		FTransform Xf;
		return C && C->GetInstanceTransform(I, Xf, /*bWorldSpace=*/ true) && !Xf.GetScale3D().IsNearlyZero();
	}

	FLinearColor ColorePianta(const UInstancedStaticMeshComponent* C, int32 I)
	{
		const int32 Base = I * C->NumCustomDataFloats;
		if (!C->PerInstanceSMCustomData.IsValidIndex(Base + 2))
		{
			return FLinearColor::Transparent;
		}
		return FLinearColor(C->PerInstanceSMCustomData[Base], C->PerInstanceSMCustomData[Base + 1],
			C->PerInstanceSMCustomData[Base + 2]);
	}
}

/**
 * 🔑 **Una cella ricordata e' una pianta: niente disco, l'anello nel colore della pianta.** E i tre stati restano tre:
 * osservata (disco e anello della griglia), ricordata (solo l'anello, ciano), mai vista (niente).
 *
 * ⚠️ L'ultima meta' misura `GetVeilCounts`, che legge cio' che e' disegnato: col disco spento nel ricordo, il conteggio
 * deve trovare il ricordo nell'anello. Senza, ogni test del velo conterebbe il ricordo fra le celle mai viste.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRememberedCellIsAPlanTest,
	"RefactorTactics.Veil.RememberedCellIsAPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRememberedCellIsAPlanTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ARTHexMapActor* HexMap = MakePiantaBoard(World, URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 3));
	const UInstancedStaticMeshComponent* Dischi = HexMap ? IsmPianta(HexMap, TEXT("Cells")) : nullptr;
	const UInstancedStaticMeshComponent* Anelli = HexMap ? IsmPianta(HexMap, TEXT("CellBorders")) : nullptr;
	if (!TestTrue(TEXT("board con dischi e anelli"), Dischi != nullptr && Anelli != nullptr))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const FRTCellId Vista(0, 0, 0);
	const FRTCellId Ricordata(1, 0, 0);
	const FRTCellId MaiVista(3, 0, 0);
	HexMap->ApplyKnowledgeVeil(ConoscenzaPianta({ Vista }, { Vista, Ricordata }));

	const int32 IV = IstanzaPianta(HexMap, Vista);
	const int32 IR = IstanzaPianta(HexMap, Ricordata);
	const int32 IM = IstanzaPianta(HexMap, MaiVista);
	if (!TestTrue(TEXT("premessa: le tre celle hanno un'istanza"), IV != INDEX_NONE && IR != INDEX_NONE && IM != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const FLinearColor Pianta = ARTHexMapActor::PlanColor();
	TestTrue(TEXT("osservata: il disco c'e'"), DisegnataPianta(Dischi, IV));
	TestTrue(TEXT("osservata: l'anello c'e'"), DisegnataPianta(Anelli, IV));
	TestFalse(TEXT("osservata: l'anello NON ha il colore della pianta"), ColorePianta(Anelli, IV).Equals(Pianta, 1e-3f));

	TestFalse(TEXT("ricordata: il disco non c'e'"), DisegnataPianta(Dischi, IR));
	TestTrue(TEXT("ricordata: l'anello c'e'"), DisegnataPianta(Anelli, IR));
	TestTrue(TEXT("ricordata: l'anello ha il colore della pianta, a piena luminosita'"),
		ColorePianta(Anelli, IR).Equals(Pianta, 1e-3f));

	TestFalse(TEXT("mai vista: niente disco"), DisegnataPianta(Dischi, IM));
	TestFalse(TEXT("mai vista: niente anello"), DisegnataPianta(Anelli, IM));

	int32 Accese = 0, Ricordate = 0, Nascoste = 0;
	HexMap->GetVeilCounts(Accese, Ricordate, Nascoste);
	TestEqual(TEXT("il conteggio trova UNA cella osservata"), Accese, 1);
	TestEqual(TEXT("e UNA ricordata, letta dall'anello"), Ricordate, 1);
	TestEqual(TEXT("e tutte le altre mai viste"), Nascoste, HexMap->NumInstanceCells() - 2);

	// E si torna indietro: rivista, la cella ritrova disco e anello della griglia.
	HexMap->ApplyKnowledgeVeil(ConoscenzaPianta({ Vista, Ricordata }, { Vista, Ricordata }));
	TestTrue(TEXT("rivista: il disco torna"), DisegnataPianta(Dischi, IR));
	TestFalse(TEXT("rivista: l'anello lascia il colore della pianta"), ColorePianta(Anelli, IR).Equals(Pianta, 1e-3f));

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * Il ricordo non si deve confondere col presente. Ripensa `Hex.VeiledSurfaceColorsAreDistinguishable`, che misurava i
 * colori di superficie attenuati: con la pianta il ricordo non ha piu' un colore di superficie, e il canale che dice
 * «ricordato» e' l'anello. Quindi il colore della pianta si confronta con l'anello delle celle osservate, su tutti i
 * piani, e con la linea di confine della vista (#3761), che e' a sua volta un ciano.
 *
 * ⚠️ La soglia e' quella del test che ripensa, 50 sulla somma delle differenze in sRGB.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanColorStandsApartTest,
	"RefactorTactics.Hex.PlanColorStandsApartFromThePresent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanColorStandsApartTest::RunTest(const FString&)
{
	const FColor Pianta = ARTHexMapActor::PlanColor().ToFColor(/*bSRGB=*/ true);
	const auto Distanza = [](const FColor& A, const FColor& B)
	{
		return FMath::Abs(A.R - B.R) + FMath::Abs(A.G - B.G) + FMath::Abs(A.B - B.B);
	};
	const int32 Soglia = 50;
	for (const ERTLayerRelation R : { ERTLayerRelation::Active, ERTLayerRelation::Above, ERTLayerRelation::Below })
	{
		const int32 D = Distanza(Pianta, RTLayerPresentation::BorderColorFor(R));
		TestTrue(*FString::Printf(TEXT("la pianta si distingue dall'anello osservato del piano %d (distanza %d)"),
			static_cast<int32>(R), D), D >= Soglia);
	}
	return true;
}

/**
 * 🔑 **Un muro ricordato diventa una linea della pianta, e torna solido quando lo si rivede.** La scelta dell'autore
 * sulla domanda dei muri di #3760.
 *
 * ⚠️ Col disegno di debug spento, come in Shipping: le linee passano dal line batcher del mondo e non da `DrawDebug*`
 * (#3508). Il controllo positivo e' quello di `Preview.DrawsWithDebugDrawingOff`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRememberedWallIsAPlanLineTest,
	"RefactorTactics.Veil.RememberedWallIsAPlanLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRememberedWallIsAPlanLineTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	// Una copertura alta sul lato est della cella (1,0): dall'altra parte (2,0), che la squadra non ha mai visto.
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 3);
	const FRTCellId ConMuro(1, 0, 0);
	FRTHexCellData Cella = *Map->FindCell(ConMuro);
	FRTHexCover Copertura;
	Copertura.Edge = ERTHexDirection::E;
	Copertura.Type = ERTHexCoverType::High;
	Cella.Covers = { Copertura };
	Map->AddOrUpdateCell(Cella);

	ARTHexMapActor* HexMap = MakePiantaBoard(World, Map);
	const UInstancedStaticMeshComponent* Pannelli = HexMap ? IsmPianta(HexMap, TEXT("EdgeFeatures")) : nullptr;
	ULineBatchComponent* Mondo = World->GetLineBatcher(UWorld::ELineBatcherType::World);
	IConsoleVariable* Debug = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EnableDrawDebugHelpers"));
	if (!TestTrue(TEXT("premessa: board, pannelli, batcher e CVar del debug"),
		HexMap != nullptr && Pannelli != nullptr && Mondo != nullptr && Debug != nullptr))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	if (!TestEqual(TEXT("premessa: la copertura ha UN pannello"), Pannelli->GetInstanceCount(), 1))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	RTTestConsoleVariable::TGuardia<int32> DebugSpento(*Debug, 0);

	const FLinearColor Pianta(ARTHexMapActor::PlanColor().ToFColor(/*bSRGB=*/ true));
	float QuotaMassima = -BIG_NUMBER;
	const auto LineeDellaPianta = [&]()
	{
		Mondo->Flush();
		static_cast<AActor*>(HexMap)->Tick(0.f);
		int32 N = 0;
		QuotaMassima = -BIG_NUMBER;
		for (const FBatchedLine& L : Mondo->BatchedLines)
		{
			if (L.Color.Equals(Pianta))
			{
				++N;
				QuotaMassima = FMath::Max(QuotaMassima, FMath::Max(L.Start.Z, L.End.Z));
			}
		}
		return N;
	};

	// Ricordata: il pannello sparisce, e al suo posto c'e' il suo CONTORNO a linee: la cima e i due montanti.
	HexMap->ApplyKnowledgeVeil(ConoscenzaPianta({ FRTCellId(0, 0, 0) }, { FRTCellId(0, 0, 0), ConMuro }));
	TestFalse(TEXT("ricordato: il pannello solido non si disegna"), DisegnataPianta(Pannelli, 0));
	TestEqual(TEXT("ricordato: tre linee della pianta lo sostituiscono, la cima e i due montanti"), LineeDellaPianta(), 3);
	// 🔴 La cima sta SOPRA il pavimento: a terra il tratto coinciderebbe coi lati degli anelli della pianta, dello stesso
	// colore, e sparirebbe nella griglia — misurato nella seduta `U89`.
	FVector Origin; float Size = 0.f; float LayerH = 0.f;
	HexMap->GetHexContext(Origin, Size, LayerH);
	const float Pavimento = URTHexLibrary::AxialToWorld(ConMuro, Origin, Size, LayerH).Z;
	TestTrue(*FString::Printf(TEXT("la cima del contorno sta sopra il pavimento (%.1f contro %.1f)"), QuotaMassima, Pavimento),
		QuotaMassima > Pavimento + 20.f);

	// Rivista: il pannello torna solido, e la linea della pianta sparisce.
	HexMap->ApplyKnowledgeVeil(ConoscenzaPianta({ FRTCellId(0, 0, 0), ConMuro }, { FRTCellId(0, 0, 0), ConMuro }));
	TestTrue(TEXT("rivisto: il pannello solido torna"), DisegnataPianta(Pannelli, 0));
	TestEqual(TEXT("rivisto: nessuna linea della pianta"), LineeDellaPianta(), 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
