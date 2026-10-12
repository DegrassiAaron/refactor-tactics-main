#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * `LayerStack` — la fixture a tre piani su cui si giudica come si vedono i piani rispetto a quello attivo
 * (#3678, D-501), e la separazione della vista strategica.
 *
 * 🔑 **Tre piani affiancati non servirebbero a niente.** La presentazione di #3679 esiste perche' un piano
 * COPRE quello attivo: una fixture con tre piani che non si sovrappongono darebbe una seduta in cui il
 * difetto da giudicare non compare. Per questo il test misura la sovrapposizione, non solo i piani.
 */
namespace
{
	/** Le celle di `Map` sul piano `Layer`, come coppie (q,r). Nome distinto per file (unity build). */
	TSet<FIntPoint> LayerStackFootprint(const URTHexMapAsset* Map, int32 Layer)
	{
		TSet<FIntPoint> Impronta;
		for (const FRTHexCellData& Cell : Map->Cells)
		{
			if (Cell.Id.Layer == Layer)
			{
				Impronta.Add(FIntPoint(Cell.Id.X, Cell.Id.Y));
			}
		}
		return Impronta;
	}
}

/**
 * I piani sono 0, 1 e 2, e ognuno dei due alti sta **sopra** quello sotto: ogni sua cella ha una cella del
 * piano sotto nella stessa colonna. E il piano sotto resta in parte scoperto, o la seduta non vedrebbe la
 * differenza fra una cella coperta e una accanto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerStackStacksThreeLayersTest,
	"RefactorTactics.HexMap.LayerStackStacksThreeLayers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLayerStackStacksThreeLayersTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFixtureArena(GetTransientPackage(), TEXT("LayerStack"));
	if (!TestNotNull(TEXT("la fixture si costruisce per nome"), Map))
	{
		return false;
	}

	TSet<int32> Piani;
	for (const FRTHexCellData& Cell : Map->Cells)
	{
		Piani.Add(Cell.Id.Layer);
	}
	TestEqual(TEXT("i piani sono tre"), Piani.Num(), 3);
	TestTrue(TEXT("e sono 0, 1 e 2"), Piani.Contains(0) && Piani.Contains(1) && Piani.Contains(2));

	for (int32 Piano = 1; Piano <= 2; ++Piano)
	{
		const TSet<FIntPoint> Sopra = LayerStackFootprint(Map, Piano);
		const TSet<FIntPoint> Sotto = LayerStackFootprint(Map, Piano - 1);
		const TSet<FIntPoint> Coperte = Sopra.Intersect(Sotto);
		AddInfo(FString::Printf(TEXT("piano %d: %d celle, %d sopra il piano %d (%d celle)"),
			Piano, Sopra.Num(), Coperte.Num(), Piano - 1, Sotto.Num()));

		TestTrue(*FString::Printf(TEXT("il piano %d ha celle"), Piano), Sopra.Num() > 0);
		TestEqual(*FString::Printf(TEXT("ogni cella del piano %d sta sopra una del piano %d"), Piano, Piano - 1),
			Coperte.Num(), Sopra.Num());
		TestTrue(*FString::Printf(TEXT("e il piano %d resta in parte scoperto"), Piano - 1),
			Sotto.Num() > Coperte.Num());
	}
	return true;
}

/**
 * Da terra si sale fino in cima, attraverso i due archi. Senza questo, i piani alti sarebbero scenografia:
 * i layer non hanno adiacenza implicita, quindi un arco mancante li renderebbe irraggiungibili in silenzio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerStackLayersAreReachableTest,
	"RefactorTactics.HexMap.LayerStackLayersAreReachableFromTheGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLayerStackLayersAreReachableTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFixtureArena(GetTransientPackage(), TEXT("LayerStack"));
	if (!TestNotNull(TEXT("la fixture si costruisce"), Map))
	{
		return false;
	}

	const FRTCellId Partenza(-3, 1, 0);
	const FRTHexSnapshot Snap =
		URTHexSimLibrary::MakeSnapshotOmniscient(Map, { FRTHexSimUnit(0, Partenza, /*Budget=*/ 99) });
	const TArray<FRTHexReachableCell> Raggiunte = URTHexSimLibrary::ReachableCells(Snap, 0);

	TSet<int32> PianiRaggiunti;
	for (const FRTHexReachableCell& R : Raggiunte)
	{
		PianiRaggiunti.Add(R.Cell.Layer);
	}
	TestTrue(TEXT("da terra si raggiunge il piano 1"), PianiRaggiunti.Contains(1));
	TestTrue(TEXT("e il piano 2"), PianiRaggiunti.Contains(2));
	// `Link` registra l'arco nei due versi: la firma di `TestArena`, un arco solo, e' «65/2».
	TestEqual(TEXT("i due archi dichiarati, ciascuno nei due versi"), Map->Transitions.Num(), 4);
	return true;
}

/**
 * La fixture e' nominabile: e' cio' che la rende caricabile da uno scenario (`"fixture": "LayerStack"`) e da
 * `rt.Map.Fixture` in seduta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerStackIsNamedTest,
	"RefactorTactics.HexMap.LayerStackIsAFixtureByName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLayerStackIsNamedTest::RunTest(const FString&)
{
	TestTrue(TEXT("`LayerStack` e' fra i nomi noti"),
		URTMatchSetupLibrary::KnownFixtureIds().Contains(TEXT("LayerStack")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
