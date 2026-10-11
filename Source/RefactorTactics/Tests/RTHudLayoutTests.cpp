#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "SceneView.h"
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"
#include "Player/RTPointerInteraction.h"
#include "UI/RTGameViewportClient.h"
#include "UI/RTHudLayout.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * **Il rettangolo della scena viene dalle fasce e dalla scala dell'interfaccia** (#3661, D-497).
 *
 * 🔑 **Non e' una moltiplicazione di frazioni per dimensioni**: la scala viene da `UUserInterfaceSettings`, cioe'
 * dalla curva che UE applica davvero all'HUD (`ShortestSide`). Se il progetto cambiasse la curva o la regola, i
 * numeri di #3661 — 22,2% a 16:10, 15% a 21:9 — cadrebbero qui invece che a schermo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudSceneRectFromBandsTest,
	"RefactorTactics.ScreenHud.SceneRectComesFromTheBandsAndTheUiScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudSceneRectFromBandsTest::RunTest(const FString&)
{
	const FRTHudBands& Fasce = RTHudLayout::Bands();

	struct FCaso { int32 W; int32 H; float Sinistra; float Destra; float Alta; float Bassa; const TCHAR* Nome; };
	// Le frazioni attese di #3661, in percento dello schermo, con mezzo decimo di tolleranza.
	const FCaso Casi[] = {
		{ 1920, 1080, 20.0f, 19.2f, 19.0f, 17.0f, TEXT("1920x1080") },
		{ 2560, 1080, 15.0f, 14.4f, 19.0f, 17.0f, TEXT("2560x1080") },
		{ 1280,  800, 22.2f, 21.3f, 19.0f, 17.0f, TEXT("1280x800") },
		{ 1920, 1200, 22.2f, 21.3f, 19.0f, 17.0f, TEXT("1920x1200") } };

	for (const FCaso& C : Casi)
	{
		const FIntPoint Vp(C.W, C.H);
		const float Scala = RTHudLayout::UiScaleFor(Vp);
		const FBox2D Scena = RTHudLayout::SceneRectPixels(Vp, Scala, Fasce);
		const float Sinistra = 100.f * static_cast<float>(Scena.Min.X) / C.W;
		const float Destra = 100.f * static_cast<float>(C.W - Scena.Max.X) / C.W;
		const float Alta = 100.f * static_cast<float>(Scena.Min.Y) / C.H;
		const float Bassa = 100.f * static_cast<float>(C.H - Scena.Max.Y) / C.H;
		AddInfo(FString::Printf(TEXT("%s: scala %.3f, colonne %.1f%% e %.1f%%, fasce %.1f%% e %.1f%%, scena %.0f%%x%.0f%%"),
			C.Nome, Scala, Sinistra, Destra, Alta, Bassa,
			100.f * (Scena.Max.X - Scena.Min.X) / C.W, 100.f * (Scena.Max.Y - Scena.Min.Y) / C.H));

		TestEqual(*FString::Printf(TEXT("%s: colonna sinistra"), C.Nome), Sinistra, C.Sinistra, 0.05f);
		TestEqual(*FString::Printf(TEXT("%s: colonna destra"), C.Nome), Destra, C.Destra, 0.05f);
		TestEqual(*FString::Printf(TEXT("%s: fascia alta"), C.Nome), Alta, C.Alta, 0.05f);
		TestEqual(*FString::Printf(TEXT("%s: fascia bassa"), C.Nome), Bassa, C.Bassa, 0.05f);
	}

	// La scala di riferimento: a 1080 di lato corto vale 1, ed e' la premessa su cui le frazioni di D-497 sono
	// misurate a 1920x1080.
	TestEqual(TEXT("a 1920x1080 un'unita' Slate e' un pixel"), RTHudLayout::UiScaleFor(FIntPoint(1920, 1080)), 1.f, 1e-3f);

	// Una finestra in cui le fasce non lasciano niente: la scena torna intera invece di sparire.
	const FBox2D Minuscola = RTHudLayout::SceneRectPixels(FIntPoint(600, 300), 1.f, Fasce);
	TestEqual(TEXT("se le fasce non lasciano niente, la scena e' tutto il viewport"), Minuscola.Max, FVector2D(600.0, 300.0));
	return true;
}

/** `Origin`/`Size` del giocatore sono il rettangolo in pixel diviso per il viewport. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudSceneOriginSizeTest,
	"RefactorTactics.ScreenHud.SceneOriginSizeIsThePixelRectOnTheViewport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudSceneOriginSizeTest::RunTest(const FString&)
{
	const FIntPoint Vp(1920, 1080);
	const float Scala = RTHudLayout::UiScaleFor(Vp);
	const FBox2D Pixel = RTHudLayout::SceneRectPixels(Vp, Scala, RTHudLayout::Bands());
	FVector2D Origine;
	FVector2D Dimensione;
	RTHudLayout::SceneOriginSize(Vp, Scala, RTHudLayout::Bands(), Origine, Dimensione);
	TestTrue(TEXT("l'origine e' l'angolo della scena"), (Origine * FVector2D(Vp)).Equals(Pixel.Min, 1e-3));
	TestTrue(TEXT("la dimensione e' quella della scena"), (Dimensione * FVector2D(Vp)).Equals(Pixel.Max - Pixel.Min, 1e-3));
	TestTrue(TEXT("e la scena e' piu' piccola del viewport (anti-vacuita')"), Dimensione.X < 0.9 && Dimensione.Y < 0.9);
	return true;
}

/**
 * **Un punto dello schermo da' un raggio, e una cella, solo dentro la scena** (#3661, D-497 (1)).
 *
 * 🔴 **Il difetto che chiude**: `FSceneView::DeprojectScreenToWorld` normalizza il punto nel rettangolo senza
 * controllarlo, quindi un click sulla fascia nera diventava un raggio estrapolato e una cella sul bordo della
 * mappa. Senza viewport `DeprojectMousePositionToWorld` esce `false`, ed e' per questo che si misura qui, sulla
 * funzione pura che il controller usa.
 *
 * La vista e' costruita come la costruisce UE per il render, nel rettangolo VERO della scena (con l'origine non
 * nulla): il centro del rettangolo deve colpire il pivot e la sua cella.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerScenePointRayTest,
	"RefactorTactics.Pointer.ScenePointRaysOnlyInsideTheRect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerScenePointRayTest::RunTest(const FString&)
{
	const FIntPoint Vp(1920, 1080);
	const FBox2D R = RTHudLayout::SceneRectPixels(Vp, RTHudLayout::UiScaleFor(Vp), RTHudLayout::Bands());
	const FIntRect Scena(FMath::RoundToInt(R.Min.X), FMath::RoundToInt(R.Min.Y),
		FMath::RoundToInt(R.Max.X), FMath::RoundToInt(R.Max.Y));
	if (!TestTrue(TEXT("premessa: la scena non parte dall'angolo dello schermo"), Scena.Min.X > 0 && Scena.Min.Y > 0))
	{
		return false;
	}

	// Una vista di partita: pivot nell'origine, braccio 1500, pitch -40, yaw 0, FOV 90 a 16:9.
	const FVector Pivot = FVector::ZeroVector;
	FMinimalViewInfo Info;
	Info.Rotation = FRotator(-40.f, 0.f, 0.f);
	Info.Location = Pivot - Info.Rotation.Vector() * 1500.0;
	Info.FOV = 90.f;
	Info.AspectRatio = 16.f / 9.f;
	Info.ProjectionMode = ECameraProjectionMode::Perspective;

	FSceneViewProjectionData Dati;
	Dati.ViewOrigin = Info.Location;
	Dati.ViewRotationMatrix = FInverseRotationMatrix(Info.Rotation) * FMatrix(
		FPlane(0, 0, 1, 0),
		FPlane(1, 0, 0, 0),
		FPlane(0, 1, 0, 0),
		FPlane(0, 0, 0, 1));
	Dati.SetViewRectangle(Scena);
	FMinimalViewInfo::CalculateProjectionMatrixGivenViewRectangle(
		Info, GetDefault<ULocalPlayer>()->AspectRatioAxisConstraint, Scena, Dati);
	const FMatrix InversaVP = Dati.ComputeViewProjectionMatrix().InverseFast();

	auto Cella = [](const FVector& O, const FVector& D)
	{
		return URTHexLibrary::ResolveRayToCellOnLayer(O, D, FVector::ZeroVector, /*HexSize=*/ 150.f,
			/*LayerHeight=*/ 250.f, /*ActiveLayer=*/ 0, /*bHasValidHit=*/ false, FVector::ZeroVector);
	};

	// —— Il centro del rettangolo colpisce il pivot, e la sua cella.
	const FVector2D Centro(Scena.Min.X + Scena.Width() * 0.5, Scena.Min.Y + Scena.Height() * 0.5);
	FVector O, D;
	if (!TestTrue(TEXT("il centro della scena da' un raggio"),
		URTPointerLibrary::DeprojectInsideSceneRect(Centro, Scena, InversaVP, O, D)))
	{
		return false;
	}
	const double T = -O.Z / D.Z;
	const FVector Colpo = O + D * T;
	// Entro un pixel: a 1500 uu e FOV 90 un pixel della scena vale circa 2,6 uu, e il rettangolo e' arrotondato ai pixel.
	TestTrue(*FString::Printf(TEXT("e il raggio colpisce il pivot (%.1f, %.1f), entro un pixel"), Colpo.X, Colpo.Y), Colpo.Size2D() < 4.0);
	const FRTCellId CellaColpita = Cella(O, D);
	TestTrue(*FString::Printf(TEXT("e la cella del pivot (e' %s)"), *CellaColpita.ToString()), CellaColpita == FRTCellId(0, 0, 0));

	// —— Sulla fascia sinistra, alla stessa altezza: nessun raggio, quindi nessuna cella.
	const FVector2D SullaFascia(Scena.Min.X - 20.0, Centro.Y);
	TestFalse(TEXT("un punto sulla fascia sinistra non da' un raggio"),
		URTPointerLibrary::DeprojectInsideSceneRect(SullaFascia, Scena, InversaVP, O, D));

	// —— Controllo positivo: un punto appena dentro, sulla stessa riga, il raggio lo da'.
	TestTrue(TEXT("un punto appena dentro il bordo sinistro lo da'"),
		URTPointerLibrary::DeprojectInsideSceneRect(FVector2D(Scena.Min.X + 1.0, Centro.Y), Scena, InversaVP, O, D));

	// —— Bordi: il pixel `Max` e' gia' della fascia, il `Min` no.
	TestTrue(TEXT("il pixel Min e' dentro"), URTPointerLibrary::IsInsideSceneRect(FVector2D(Scena.Min), Scena));
	TestFalse(TEXT("il pixel Max e' fuori"), URTPointerLibrary::IsInsideSceneRect(FVector2D(Scena.Max), Scena));
	TestFalse(TEXT("sotto la scena, sulla fascia bassa, e' fuori"),
		URTPointerLibrary::IsInsideSceneRect(FVector2D(Centro.X, Scena.Max.Y + 10.0), Scena));
	return true;
}

/**
 * `URTGameViewportClient` e' il viewport client del gioco: lo dice `DefaultEngine.ini`, e lo si legge dal motore
 * invece che dal file. Senza, `LayoutPlayers` non sarebbe mai il nostro e la scena resterebbe a schermo intero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudViewportClientRegisteredTest,
	"RefactorTactics.ScreenHud.TheGameViewportClientIsOurs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudViewportClientRegisteredTest::RunTest(const FString&)
{
	if (!TestNotNull(TEXT("GEngine"), GEngine)) { return false; }
	TestTrue(*FString::Printf(TEXT("la classe del viewport client e' URTGameViewportClient (e' %s)"),
		GEngine->GameViewportClientClass ? *GEngine->GameViewportClientClass->GetName() : TEXT("nessuna")),
		GEngine->GameViewportClientClass == URTGameViewportClient::StaticClass());
	return true;
}

/** Le richieste si contano: due HUD montati in sequenza non si spengono a vicenda nell'ordine sbagliato. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudSceneInsetRequestsTest,
	"RefactorTactics.ScreenHud.SceneInsetRequestsAreCounted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudSceneInsetRequestsTest::RunTest(const FString&)
{
	// `ClassWithin=Engine`: un viewport client nasce dentro il motore, non nel pacchetto transiente.
	URTGameViewportClient* Client = GEngine ? NewObject<URTGameViewportClient>(GEngine) : nullptr;
	if (!TestNotNull(TEXT("il viewport client si costruisce"), Client)) { return false; }
	TestFalse(TEXT("nasce spento: senza HUD la scena e' a schermo intero"), Client->IsSceneInsetActive());
	Client->SetSceneInsetRequested(true);
	Client->SetSceneInsetRequested(true);
	Client->SetSceneInsetRequested(false);
	TestTrue(TEXT("con una richiesta ancora aperta resta acceso"), Client->IsSceneInsetActive());
	Client->SetSceneInsetRequested(false);
	TestFalse(TEXT("chiusa l'ultima, si spegne"), Client->IsSceneInsetActive());
	Client->SetSceneInsetRequested(false);
	TestFalse(TEXT("e una restituzione in piu' non lo rende negativo"), Client->IsSceneInsetActive());
	Client->SetSceneInsetRequested(true);
	TestTrue(TEXT("quindi una richiesta nuova lo riaccende subito"), Client->IsSceneInsetActive());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
