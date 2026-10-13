// L'esposizione della camera di gioco e' fissa (#3723, [D-501]).
//
// 🔑 **Questi test misurano cio' che la camera CHIEDE al motore, non cio' che si vede.** La suite gira in `-NullRHI`:
// che una mappa a un piano si veda come prima, e che i piani sotto scuriscano come livello, e' della voce PIE.
// Qui si misura che la camera di gioco nasca con l'esposizione fissa, che le leve la cambino e la spengano, e che
// scriverla non tocchi nient'altro del post-process: sulla stessa camera vive il contorno delle unita' (#3705).

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Camera/CameraComponent.h"
#include "Camera/RTCameraExposure.h"
#include "Camera/RTCameraPawn.h"
#include "Engine/Scene.h"
#include "Unit/RTUnitLayerOutline.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	void CexCheckFixed(FAutomationTestBase& Test, const FPostProcessSettings& S, float Bias, const TCHAR* Quando)
	{
		Test.TestTrue(*FString::Printf(TEXT("%s: il metodo e' forzato"), Quando), S.bOverride_AutoExposureMethod != 0);
		Test.TestTrue(*FString::Printf(TEXT("%s: ed e' quello manuale"), Quando), S.AutoExposureMethod == AEM_Manual);
		Test.TestTrue(*FString::Printf(TEXT("%s: la compensazione e' forzata"), Quando), S.bOverride_AutoExposureBias != 0);
		Test.TestTrue(*FString::Printf(TEXT("%s: e vale %s stop (letto %s)"), Quando, *FString::SanitizeFloat(Bias),
			*FString::SanitizeFloat(S.AutoExposureBias)), FMath::IsNearlyEqual(S.AutoExposureBias, Bias));
		Test.TestTrue(*FString::Printf(TEXT("%s: la camera fisica e' forzata"), Quando),
			S.bOverride_AutoExposureApplyPhysicalCameraExposure != 0);
		Test.TestTrue(*FString::Printf(TEXT("%s: ed e' spenta"), Quando), S.AutoExposureApplyPhysicalCameraExposure == 0);
	}

	void CexCheckEngine(FAutomationTestBase& Test, const FPostProcessSettings& S, const TCHAR* Quando)
	{
		const FPostProcessSettings Motore;
		Test.TestTrue(*FString::Printf(TEXT("%s: nessuno dei tre override resta acceso"), Quando),
			S.bOverride_AutoExposureMethod == 0 && S.bOverride_AutoExposureBias == 0
			&& S.bOverride_AutoExposureApplyPhysicalCameraExposure == 0);
		Test.TestTrue(*FString::Printf(TEXT("%s: e i tre valori sono quelli del motore"), Quando),
			S.AutoExposureMethod == Motore.AutoExposureMethod
			&& FMath::IsNearlyEqual(S.AutoExposureBias, Motore.AutoExposureBias)
			&& S.AutoExposureApplyPhysicalCameraExposure == Motore.AutoExposureApplyPhysicalCameraExposure);
	}
}

/** **La funzione pura**: scrive i tre campi dell'esposizione e nient'altro, e spenta rimette quelli del motore. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraExposureApplyTest,
	"RefactorTactics.CameraExposure.ApplyWritesOnlyTheExposure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraExposureApplyTest::RunTest(const FString&)
{
	// Le premesse: se il motore nascesse gia' manuale e senza camera fisica, gli asserti sotto non proverebbero niente.
	const FPostProcessSettings Motore;
	TestTrue(TEXT("premessa: il motore non forza l'esposizione"),
		Motore.bOverride_AutoExposureMethod == 0 && Motore.bOverride_AutoExposureBias == 0
		&& Motore.bOverride_AutoExposureApplyPhysicalCameraExposure == 0);
	TestTrue(TEXT("premessa: il metodo del motore non e' quello manuale"), Motore.AutoExposureMethod != AEM_Manual);
	TestTrue(TEXT("premessa: il motore applica la camera fisica"), Motore.AutoExposureApplyPhysicalCameraExposure != 0);

	FPostProcessSettings S;
	S.bOverride_BloomIntensity = true; // un campo che non e' nostro
	S.BloomIntensity = 0.125f;

	FRTCameraExposureTuning Leve;
	Leve.BiasStops = -1.25f;
	TestTrue(TEXT("premessa: la compensazione di prova non e' quella del motore"),
		!FMath::IsNearlyEqual(Leve.BiasStops, Motore.AutoExposureBias));
	RTCameraExposure::Apply(Leve, S);
	CexCheckFixed(*this, S, -1.25f, TEXT("fissa"));
	TestTrue(TEXT("un campo estraneo resta com'era"), S.bOverride_BloomIntensity != 0 && S.BloomIntensity == 0.125f);

	Leve.BiasStops = 99.f;
	RTCameraExposure::Apply(Leve, S);
	CexCheckFixed(*this, S, RTCameraExposure::MaxBiasStops, TEXT("oltre il limite"));

	Leve.bFixed = false;
	RTCameraExposure::Apply(Leve, S);
	CexCheckEngine(*this, S, TEXT("spenta"));
	TestTrue(TEXT("spenta: il campo estraneo resta com'era"), S.bOverride_BloomIntensity != 0 && S.BloomIntensity == 0.125f);
	return true;
}

/** **Le leve**: negative o fuori intervallo vogliono dire il default, e lo zero della compensazione e' un valore. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraExposureLeversTest,
	"RefactorTactics.CameraExposure.LeversReplaceTheDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraExposureLeversTest::RunTest(const FString&)
{
	using namespace RTCameraExposure;
	const FRTCameraExposureTuning Default;
	TestTrue(TEXT("default: fissa"), Default.bFixed);
	// Tarata a schermo il 2026-10-13 su sei viste: vedi `RTCameraExposure::DefaultBiasStops`. Un cambio per sbaglio cade qui.
	TestTrue(TEXT("default: la compensazione tarata su una mappa a un piano (#3723)"),
		FMath::IsNearlyEqual(Default.BiasStops, 0.25f) && Default.BiasStops == DefaultBiasStops);

	TestTrue(TEXT("leve non impostate: i default"), FRTCameraExposureTuning::FromLevers(-1, UnsetBiasLever) == Default);
	TestFalse(TEXT("0 rimette l'adattamento del motore"), FRTCameraExposureTuning::FromLevers(0, UnsetBiasLever).bFixed);
	TestTrue(TEXT("1 e' fissa"), FRTCameraExposureTuning::FromLevers(1, UnsetBiasLever).bFixed);

	TestTrue(TEXT("una compensazione dentro i limiti vale"),
		FMath::IsNearlyEqual(FRTCameraExposureTuning::FromLevers(-1, -0.5f).BiasStops, -0.5f));
	TestTrue(TEXT("premessa: lo zero non e' il default, altrimenti l'asserto dopo e' vuoto"), DefaultBiasStops != 0.f);
	TestTrue(TEXT("lo zero e' un valore, non «usa il default»"),
		FRTCameraExposureTuning::FromLevers(-1, 0.f).BiasStops == 0.f);
	TestTrue(TEXT("i due limiti valgono"),
		FRTCameraExposureTuning::FromLevers(-1, MinBiasStops).BiasStops == MinBiasStops
		&& FRTCameraExposureTuning::FromLevers(-1, MaxBiasStops).BiasStops == MaxBiasStops);
	TestTrue(TEXT("fuori dai limiti e' il default"),
		FRTCameraExposureTuning::FromLevers(-1, MaxBiasStops + 0.5f).BiasStops == DefaultBiasStops
		&& FRTCameraExposureTuning::FromLevers(-1, MinBiasStops - 0.5f).BiasStops == DefaultBiasStops);
	TestTrue(TEXT("e il valore «non impostata» sta fuori dai limiti"), UnsetBiasLever < MinBiasStops);
	return true;
}

/**
 * **La camera di gioco nasce con l'esposizione fissa**, prima di qualunque sincronizzazione: una camera senza un HUD
 * che la segua non deve tornare all'adattamento. Poi le leve la cambiano e la spengono, senza toccare il contorno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCameraExposureGameCameraTest,
	"RefactorTactics.CameraExposure.GameCameraIsFixedFromBirth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCameraExposureGameCameraTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTCameraPawn* Camera = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("la camera"), Camera)) { return false; }
	const UCameraComponent* Lente = Camera->GetCameraForTest();
	if (!TestNotNull(TEXT("il componente camera"), Lente)) { return false; }

	TestEqual(TEXT("il post-process della camera pesa per intero: senza, gli override non conterebbero"),
		Lente->PostProcessBlendWeight, 1.f);
	CexCheckFixed(*this, Lente->PostProcessSettings, RTCameraExposure::DefaultBiasStops, TEXT("appena nata"));

	FRTCameraExposureTuning Leve;
	TestFalse(TEXT("le leve di default non riscrivono: e' il caso di ogni fotogramma"), Camera->SyncExposure(Leve));

	// Il contorno delle unita' sta fra i blendable della stessa camera: l'esposizione non lo deve spostare.
	Camera->SyncUnitLayerOutline(FRTUnitLayerOutlineTuning());
	const float PesoContorno = Camera->GetUnitLayerOutlineWeightForTest();
	TestEqual(TEXT("premessa: il contorno sta fra i blendable, a peso pieno"), PesoContorno, 1.f);

	Leve.BiasStops = 1.5f;
	TestTrue(TEXT("una compensazione diversa riscrive"), Camera->SyncExposure(Leve));
	CexCheckFixed(*this, Lente->PostProcessSettings, 1.5f, TEXT("dopo la leva"));
	TestFalse(TEXT("e la stessa non riscrive due volte"), Camera->SyncExposure(Leve));

	Leve.bFixed = false;
	TestTrue(TEXT("spegnere riscrive"), Camera->SyncExposure(Leve));
	CexCheckEngine(*this, Lente->PostProcessSettings, TEXT("spenta dalla leva"));

	Leve = FRTCameraExposureTuning();
	TestTrue(TEXT("riaccendere riscrive"), Camera->SyncExposure(Leve));
	CexCheckFixed(*this, Lente->PostProcessSettings, RTCameraExposure::DefaultBiasStops, TEXT("riaccesa"));

	TestEqual(TEXT("il contorno e' rimasto dov'era"), Camera->GetUnitLayerOutlineWeightForTest(), PesoContorno);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
