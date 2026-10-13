#include "Camera/RTCameraExposure.h"
#include "Engine/Scene.h"
#include "HAL/IConsoleManager.h"

// Le leve di #3723. L'HUD le rilegge a ogni giro e la camera riscrive solo se sono cambiate.
static TAutoConsoleVariable<int32> CVarRTCameraFixedExposure(
	TEXT("rt.Camera.FixedExposure"), -1,
	TEXT("L'esposizione della camera di gioco (D-501, #3723): 1 = fissa, 0 = l'adattamento del motore. Negativo = default (fissa)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarRTCameraFixedExposureBias(
	TEXT("rt.Camera.FixedExposure.Bias"), RTCameraExposure::UnsetBiasLever,
	TEXT("La compensazione dell'esposizione fissa, in stop (-8..8; +1 = luce doppia). Fuori da quell'intervallo = default."),
	ECVF_Default);

FRTCameraExposureTuning FRTCameraExposureTuning::FromLevers(int32 FixedLever, float BiasLever)
{
	FRTCameraExposureTuning T;
	if (FixedLever >= 0)
	{
		T.bFixed = FixedLever != 0;
	}
	if (BiasLever >= RTCameraExposure::MinBiasStops && BiasLever <= RTCameraExposure::MaxBiasStops)
	{
		T.BiasStops = BiasLever;
	}
	return T;
}

FRTCameraExposureTuning FRTCameraExposureTuning::FromConsole()
{
	return FromLevers(CVarRTCameraFixedExposure.GetValueOnGameThread(), CVarRTCameraFixedExposureBias.GetValueOnGameThread());
}

void RTCameraExposure::Apply(const FRTCameraExposureTuning& Tuning, FPostProcessSettings& Settings)
{
	Settings.bOverride_AutoExposureMethod = Tuning.bFixed;
	Settings.bOverride_AutoExposureBias = Tuning.bFixed;
	Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = Tuning.bFixed;

	if (Tuning.bFixed)
	{
		Settings.AutoExposureMethod = AEM_Manual;
		Settings.AutoExposureBias = FMath::Clamp(Tuning.BiasStops, MinBiasStops, MaxBiasStops);
		// Senza la camera fisica: con lei il punto fisso verrebbe da ISO, tempo e diaframma, che qui nessuno tara.
		Settings.AutoExposureApplyPhysicalCameraExposure = false;
		return;
	}

	// Spenta: i valori tornano quelli del motore, cosi' un override riacceso a mano non trova i nostri.
	const FPostProcessSettings Motore;
	Settings.AutoExposureMethod = Motore.AutoExposureMethod;
	Settings.AutoExposureBias = Motore.AutoExposureBias;
	Settings.AutoExposureApplyPhysicalCameraExposure = Motore.AutoExposureApplyPhysicalCameraExposure;
}
