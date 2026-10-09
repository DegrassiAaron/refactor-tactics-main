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
#include "Map/RTPlaybackTracer.h"
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

// Il tracer del playback si disegna anche col debug spento (`#2454`), come l'anteprima (`#3508`, D-467).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPreviewTracerDrawsWithDebugDrawingOffTest,
	"RefactorTactics.Preview.TracerDrawsWithDebugDrawingOff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPreviewTracerDrawsWithDebugDrawingOffTest::RunTest(const FString&)
{
	UWorld* World = MakeLineeAnteprimaWorld();
	if (!TestNotNull(TEXT("World creato"), World)) { return false; }

	ULineBatchComponent* Primo = World->GetLineBatcher(UWorld::ELineBatcherType::Foreground);
	IConsoleVariable* Debug = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EnableDrawDebugHelpers"));
	if (!TestTrue(TEXT("premessa: batcher Foreground e CVar di debug presenti"), Primo != nullptr && Debug != nullptr))
	{
		DestroyLineeAnteprimaWorld(World);
		return false;
	}
	RTTestConsoleVariable::TGuardia<int32> DebugSpento(*Debug, 0);

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	if (!TestNotNull(TEXT("HexMap spawnato"), HexMap)) { DestroyLineeAnteprimaWorld(World); return false; }
	HexMap->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);

	FRTPlaybackTracer T;
	T.From = FRTCellId(0, 0);
	T.To = FRTCellId(2, 0);
	T.Alpha = 0.5f;
	T.Style = ERTTracerStyle::Projectile;
	HexMap->SetPlaybackTracers({ T });

	Primo->Flush();
	static_cast<AActor*>(HexMap)->Tick(0.f);

	// 🔑 Un tracer, UNA linea, nel batcher Foreground: attraversa le unita' come la linea di mira.
	TestEqual(TEXT("una linea nel batcher Foreground per il tracer"), Primo->BatchedLines.Num(), 1);
	if (Primo->BatchedLines.Num() == 1)
	{
		const FLinearColor Attacco(URTOverlayPalette::ColorFor(ERTOverlayMeaning::Attack));
		TestTrue(TEXT("col colore Attack della palette"), Primo->BatchedLines[0].Color.Equals(Attacco));
	}

	DestroyLineeAnteprimaWorld(World);
	return true;
}

// Le cue del profilo FX e lo `Zigzag` si disegnano col debug spento (#3578, D-467), nel batcher Foreground, col colore
// `Attack` (R1): gemello di `TracerDrawsWithDebugDrawingOff`. Un tipo per volta, col numero di segmenti di §2.1.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPreviewFxCuesDrawWithDebugDrawingOffTest,
	"RefactorTactics.Preview.FxCuesDrawWithDebugDrawingOff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPreviewFxCuesDrawWithDebugDrawingOffTest::RunTest(const FString&)
{
	UWorld* World = MakeLineeAnteprimaWorld();
	if (!TestNotNull(TEXT("World creato"), World)) { return false; }
	ULineBatchComponent* Primo = World->GetLineBatcher(UWorld::ELineBatcherType::Foreground);
	IConsoleVariable* Debug = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EnableDrawDebugHelpers"));
	if (!TestTrue(TEXT("premessa: batcher Foreground e CVar di debug presenti"), Primo != nullptr && Debug != nullptr))
	{
		DestroyLineeAnteprimaWorld(World);
		return false;
	}
	RTTestConsoleVariable::TGuardia<int32> DebugSpento(*Debug, 0);
	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	if (!TestNotNull(TEXT("HexMap spawnato"), HexMap)) { DestroyLineeAnteprimaWorld(World); return false; }
	HexMap->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);
	const FLinearColor Attacco(URTOverlayPalette::ColorFor(ERTOverlayMeaning::Attack));

	const TArray<TPair<ERTPlaybackCueKind, int32>> Attesi = {
		{ ERTPlaybackCueKind::Ring, 6 }, { ERTPlaybackCueKind::Pulse, 12 }, { ERTPlaybackCueKind::Flash, 6 },
		{ ERTPlaybackCueKind::Marker, 4 }, { ERTPlaybackCueKind::AreaPulse, 12 }, { ERTPlaybackCueKind::ConeSweep, 3 } };
	for (const TPair<ERTPlaybackCueKind, int32>& Atteso : Attesi)
	{
		FRTPlaybackCue C;
		C.Kind = Atteso.Key;
		C.At = FRTCellId(0, 0);
		C.Toward = FRTCellId(2, 0);
		C.Alpha = 0.5f;
		HexMap->SetPlaybackCues({ C });
		Primo->Flush();
		static_cast<AActor*>(HexMap)->Tick(0.f);
		const FString Nome = UEnum::GetValueAsString(Atteso.Key);
		TestEqual(FString::Printf(TEXT("🔴 %s: le sue linee nel batcher Foreground"), *Nome), Primo->BatchedLines.Num(), Atteso.Value);
		bool bTutteAttack = Primo->BatchedLines.Num() > 0;
		for (const FBatchedLine& L : Primo->BatchedLines) { bTutteAttack &= L.Color.Equals(Attacco); }
		TestTrue(FString::Printf(TEXT("%s: col colore Attack della palette"), *Nome), bTutteAttack);
	}
	HexMap->ClearPlaybackCues();

	FRTPlaybackTracer T;
	T.From = FRTCellId(0, 0);
	T.To = FRTCellId(3, 0);
	T.Alpha = 1.f;
	T.Style = ERTTracerStyle::Zigzag;
	HexMap->SetPlaybackTracers({ T });
	Primo->Flush();
	static_cast<AActor*>(HexMap)->Tick(0.f);
	TestEqual(TEXT("🔴 lo Zigzag intero: otto linee"), Primo->BatchedLines.Num(), 8);

	DestroyLineeAnteprimaWorld(World);
	return true;
}

#endif
