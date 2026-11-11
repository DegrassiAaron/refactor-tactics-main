// I gesti della camera fuori dal controller (#4065, S3): le regole che prima non si potevano isolare.
//
// `FRTCameraGestures::ReleasePrimary` decide cosa e' stato un `Ctrl`+sinistro al rilascio. Prima la decisione stava in
// `ARTPlayerController::OnSelectReleased` insieme al Set Pivot, e solo il trascinamento era provato (da un test che
// guardava la camera). Qui si provano i quattro esiti. E il controller passa ai gesti la propria taratura: un campo
// copiato nel posto sbagliato compila, e cambia la sensibilita' di un gesto diverso.
//
// ⚠️ **Cosa NON copre.** Il Set Pivot dopo un `Click` legge la cella sotto il cursore, e headless un cursore non c'e':
// resta di PIE. L'edge pan dal viewport vero, idem.

#include "Misc/AutomationTest.h"
#include "Camera/RTCameraGestures.h"
#include "Camera/RTCameraPawn.h"
#include "Engine/World.h"
#include "Player/RTPlayerController.h"
#include "Tests/Shared/RTWorldFixtures.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FString GesturesReleaseName(ERTPrimaryRelease Release)
	{
		switch (Release)
		{
		case ERTPrimaryRelease::NotAGesture: return TEXT("NotAGesture");
		case ERTPrimaryRelease::Disarmed:    return TEXT("Disarmed");
		case ERTPrimaryRelease::Drag:        return TEXT("Drag");
		case ERTPrimaryRelease::Click:       return TEXT("Click");
		}
		return TEXT("?");
	}
}

/**
 * **Il rilascio del sinistro con `Ctrl`: niente, click, trascinamento, o disarmo sotto una schermata.**
 *
 * 🔑 Il disarmo vale anche quando il gesto aveva superato la soglia: sotto una modale non si emette niente, e il gesto
 * non resta armato per sempre. E `Ctrl` rilasciato fa cadere i gesti armati con lui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraPrimaryReleaseTest,
	"RefactorTactics.Camera.PrimaryReleaseIsClickDragOrDisarmed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraPrimaryReleaseTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	ARTCameraPawn* Cam = World ? World->SpawnActor<ARTCameraPawn>() : nullptr;
	if (!TestNotNull(TEXT("premessa: il mondo"), World) || !TestNotNull(TEXT("premessa: la camera"), Cam))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	FRTCameraGestureTuning Taratura;
	Taratura.ClickDragThreshold = 10.f;
	const auto Atteso = [&](ERTPrimaryRelease Esito, ERTPrimaryRelease Voluto, const TCHAR* Caso)
	{
		TestTrue(FString::Printf(TEXT("%s: %s (atteso %s)"), Caso, *GesturesReleaseName(Esito), *GesturesReleaseName(Voluto)),
			Esito == Voluto);
	};

	// Senza `Ctrl`+sinistro il rilascio non e' un gesto camera: e' la fine di un click di selezione.
	FRTCameraGestures G;
	Atteso(G.ReleasePrimary(/*bGameplayInputBlocked=*/ false, Taratura.ClickDragThreshold), ERTPrimaryRelease::NotAGesture, TEXT("niente armato"));

	// Armato e fermo: un click, cioe' un Set Pivot. Il gesto si disarma.
	G.PressModifier();
	G.ArmPrimary();
	TestTrue(TEXT("premessa: il sinistro e' armato"), G.IsPrimaryDown());
	Atteso(G.ReleasePrimary(false, Taratura.ClickDragThreshold), ERTPrimaryRelease::Click, TEXT("fermo"));
	TestFalse(TEXT("dopo un click il gesto e' disarmato"), G.IsPrimaryDown());

	// Armato e trascinato sotto la soglia: ancora un click.
	G.ArmPrimary();
	G.Route(*Cam, FVector2D(4.f, 0.f), Taratura);
	Atteso(G.ReleasePrimary(false, Taratura.ClickDragThreshold), ERTPrimaryRelease::Click, TEXT("sotto la soglia"));

	// Trascinato oltre la soglia: era un'orbita, e il click non si emette.
	G.ArmPrimary();
	G.Route(*Cam, FVector2D(20.f, 0.f), Taratura);
	TestTrue(TEXT("premessa: la strada percorsa supera la soglia"), G.GetPrimaryDragDistance() > Taratura.ClickDragThreshold);
	Atteso(G.ReleasePrimary(false, Taratura.ClickDragThreshold), ERTPrimaryRelease::Drag, TEXT("oltre la soglia"));
	TestEqual(TEXT("e la strada percorsa riparte da zero"), G.GetPrimaryDragDistance(), 0.f);

	// Sotto una schermata bloccante: disarmo, fermo o trascinato che fosse.
	G.ArmPrimary();
	Atteso(G.ReleasePrimary(/*bGameplayInputBlocked=*/ true, Taratura.ClickDragThreshold), ERTPrimaryRelease::Disarmed, TEXT("fermo, sotto una modale"));
	TestFalse(TEXT("sotto una modale il gesto e' disarmato"), G.IsPrimaryDown());
	G.ArmPrimary();
	G.Route(*Cam, FVector2D(20.f, 0.f), Taratura);
	Atteso(G.ReleasePrimary(true, Taratura.ClickDragThreshold), ERTPrimaryRelease::Disarmed, TEXT("trascinato, sotto una modale"));

	// `Ctrl` rilasciato fa cadere i gesti armati.
	G.ArmPrimary();
	G.ArmSecondary();
	G.ReleaseModifier();
	TestFalse(TEXT("rilasciato Ctrl, il sinistro non e' piu' armato"), G.IsPrimaryDown());
	TestFalse(TEXT("rilasciato Ctrl, il destro non e' piu' armato"), G.IsSecondaryDown());
	TestFalse(TEXT("e Ctrl non e' piu' tenuto"), G.IsModifierDown());

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **La taratura dei gesti viene dalle proprieta' del controller**, una per una.
 *
 * Restano `UPROPERTY` `EditAnywhere` di `ARTPlayerController`, e `GetCameraGestureTuning` le copia nei gesti a ogni
 * movimento. Qui si scrivono sei valori distinti attraverso la riflessione — la stessa via di un override in un asset —
 * e si chiede che ciascuno arrivi al suo campo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraGestureTuningTest,
	"RefactorTactics.Camera.GestureTuningComesFromTheControllerProperties",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCameraGestureTuningTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	ARTPlayerController* PC = World ? World->SpawnActor<ARTPlayerController>() : nullptr;
	if (!TestNotNull(TEXT("premessa: il mondo"), World) || !TestNotNull(TEXT("premessa: il controller"), PC))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	struct FVoce { const TCHAR* Proprieta; float Valore; float FRTCameraGestureTuning::* Campo; };
	const FVoce Voci[] = {
		{ TEXT("ClickDragThreshold"), 11.f,  &FRTCameraGestureTuning::ClickDragThreshold },
		{ TEXT("PrecisionPanScale"),  0.5f,  &FRTCameraGestureTuning::PrecisionPanScale },
		{ TEXT("PixelsPerPanUnit"),   22.f,  &FRTCameraGestureTuning::PixelsPerPanUnit },
		{ TEXT("DollySensitivity"),   0.07f, &FRTCameraGestureTuning::DollySensitivity },
		{ TEXT("PeekSensitivity"),    4.5f,  &FRTCameraGestureTuning::PeekSensitivity },
		{ TEXT("PeekReturnSpeed"),    9.5f,  &FRTCameraGestureTuning::PeekReturnSpeed },
	};
	for (const FVoce& Voce : Voci)
	{
		FFloatProperty* Prop = FindFProperty<FFloatProperty>(ARTPlayerController::StaticClass(), Voce.Proprieta);
		if (!TestNotNull(FString::Printf(TEXT("premessa: %s e' una proprieta' del controller"), Voce.Proprieta), Prop))
		{
			continue;
		}
		Prop->SetPropertyValue_InContainer(PC, Voce.Valore);
	}
	const FRTCameraGestureTuning Taratura = PC->GetCameraGestureTuning();
	for (const FVoce& Voce : Voci)
	{
		TestEqual(FString::Printf(TEXT("%s arriva al suo campo"), Voce.Proprieta), Taratura.*(Voce.Campo), Voce.Valore);
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
