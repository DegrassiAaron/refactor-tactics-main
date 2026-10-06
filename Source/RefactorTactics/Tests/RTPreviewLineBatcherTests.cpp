// L'anteprima di pianificazione si disegna anche quando il disegno di DEBUG e' spento (`#3508`).
//
// Misurato sul pacchetto il 2026-10-06: con `DrawDebugLine` il Development mostrava ventaglio, portata e
// percorso, lo Shipping nessuno dei tre, perche' li' la funzione e' vuota. L'anteprima ora scrive nel line
// batcher del mondo, e questo test e' la guardia contro il ritorno a `DrawDebugLine`.
//
// ⚠️ Non misura la RESA in Shipping, dove il batcher scrive sul PDI della scena invece che su quello di
// debug: quella la misura soltanto il pacchetto.

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/LineBatchComponent.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTOverlayPalette.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "RTConsoleVariableGuardForTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Mondo per le linee dell'anteprima (nomi distinti per file: in unity build i test condividono la TU). */
	UWorld* MakeLineeAnteprimaWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyLineeAnteprimaWorld(UWorld* World)
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
}

/**
 * 🔑 **Perche' `r.EnableDrawDebugHelpers` e non un conteggio nudo.** In Development `DrawDebugLine` scrive nello
 * STESSO line batcher: un test che contasse le linee e basta resterebbe verde anche tornando a `DrawDebugLine`,
 * che in Shipping e' vuota. Con la CVar spenta `DrawDebugLine` esce subito — il controllo positivo qui sotto lo
 * misura — e nei batcher restano solo le linee che non dipendono dal debug, cioe' quelle che lo Shipping vedra'.
 *
 * ⚠️ **I conteggi sono ESATTI, per batcher.** Con un «almeno uno» una mira riportata a `DrawDebugLine` resterebbe
 * coperta dal contorno della sua origine, che puo' cadere nello stesso batcher `Foreground`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPreviewDrawsWithDebugDrawingOffTest,
	"RefactorTactics.Preview.DrawsWithDebugDrawingOff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPreviewDrawsWithDebugDrawingOffTest::RunTest(const FString&)
{
	UWorld* World = MakeLineeAnteprimaWorld();
	if (!TestNotNull(TEXT("World creato"), World))
	{
		return false;
	}

	ULineBatchComponent* Mondo = World->GetLineBatcher(UWorld::ELineBatcherType::World);
	ULineBatchComponent* Primo = World->GetLineBatcher(UWorld::ELineBatcherType::Foreground);
	if (!TestTrue(TEXT("premessa: il mondo di prova ha i line batcher World e Foreground"), Mondo != nullptr && Primo != nullptr))
	{
		DestroyLineeAnteprimaWorld(World);
		return false;
	}

	IConsoleVariable* Debug = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EnableDrawDebugHelpers"));
	if (!TestNotNull(TEXT("premessa: la CVar r.EnableDrawDebugHelpers esiste"), Debug))
	{
		DestroyLineeAnteprimaWorld(World);
		return false;
	}
	RTTestConsoleVariable::TGuardia<int32> DebugSpento(*Debug, 0);

	// Controllo positivo: con il debug spento `DrawDebugLine` non disegna in NESSUNO dei due batcher. Senza questa
	// premessa il test non distinguerebbe il line batcher da `DrawDebugLine`.
	Mondo->Flush();
	Primo->Flush();
	DrawDebugLine(World, FVector::ZeroVector, FVector(100.f, 0.f, 0.f), FColor::Red, false, -1.f, SDPG_World, 1.f);
	DrawDebugLine(World, FVector::ZeroVector, FVector(100.f, 0.f, 0.f), FColor::Red, false, -1.f, SDPG_Foreground, 1.f);
	if (!TestEqual(TEXT("controllo: con il debug spento DrawDebugLine non disegna nel batcher World"), Mondo->BatchedLines.Num(), 0)
		|| !TestEqual(TEXT("controllo: con il debug spento DrawDebugLine non disegna nel batcher Foreground"), Primo->BatchedLines.Num(), 0))
	{
		DestroyLineeAnteprimaWorld(World);
		return false;
	}

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	if (!TestNotNull(TEXT("HexMap spawnato"), HexMap))
	{
		DestroyLineeAnteprimaWorld(World);
		return false;
	}
	HexMap->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);

	// Due canali, due batcher: il ventaglio sta su celle vuote e resta in `World`; la mira attraversa le unita'
	// e sta in `Foreground`. Il contorno della sua origine va dove la palette dice.
	const TArray<FRTCellId> Ventaglio = { FRTCellId(0, 0), FRTCellId(1, 0) };
	HexMap->SetPreviewReachableCells(Ventaglio);
	HexMap->SetPreviewAttack(FRTCellId(0, 0), FRTCellId(2, 0), /*bValid=*/ true, /*bOriginPredicted=*/ false);

	// `Tick` e' protetto in `ARTHexMapActor` e pubblico in `AActor`: si chiama dalla base, la strada del motore.
	static_cast<AActor*>(HexMap)->Tick(0.f);

	const bool bOrigineDavanti = URTOverlayPalette::DrawsThroughUnits(ERTOverlayMeaning::AttackOriginAim);
	const int32 AttesoWorld = Ventaglio.Num() * 6 + (bOrigineDavanti ? 0 : 6);
	const int32 AttesoForeground = 1 + (bOrigineDavanti ? 6 : 0); // la mira, piu' l'origine se attraversa
	TestEqual(TEXT("nel batcher World: sei lati per cella del ventaglio, e l'origine se non attraversa"),
		Mondo->BatchedLines.Num(), AttesoWorld);
	TestEqual(TEXT("nel batcher Foreground: la linea di mira, e l'origine se attraversa"),
		Primo->BatchedLines.Num(), AttesoForeground);

	// Le regole di `DrawDebugLine`, ricopiate: il colore della palette e la durata di UN fotogramma.
	const FLinearColor Movimento(URTOverlayPalette::ColorFor(ERTOverlayMeaning::Movement));
	int32 LineeMovimento = 0;
	bool bDurataDiUnFotogramma = true;
	for (const FBatchedLine& Linea : Mondo->BatchedLines)
	{
		LineeMovimento += Linea.Color.Equals(Movimento) ? 1 : 0;
		bDurataDiUnFotogramma &= FMath::IsNearlyEqual(Linea.RemainingLifeTime, Mondo->DefaultLifeTime);
	}
	TestEqual(TEXT("il ventaglio ha il colore Movement della palette"), LineeMovimento, Ventaglio.Num() * 6);
	TestTrue(TEXT("ogni linea dura un fotogramma, come con DrawDebugLine"), bDurataDiUnFotogramma);

	DestroyLineeAnteprimaWorld(World);
	return true;
}

#endif
