#include "Map/RTLayerPresentation.h"
#include "HAL/IConsoleManager.h"

// Le leve di #3679: negative = usa il default della PR. Si applicano al fotogramma dopo (`SyncActiveLayerPresentation`).
static TAutoConsoleVariable<float> CVarRTMapLayersUpperOpacity(
	TEXT("rt.Map.Layers.UpperOpacity"), -1.f,
	TEXT("Opacita' dei piani sopra quello attivo (0..1). Negativo = default (0,35)."), ECVF_Default);

static TAutoConsoleVariable<float> CVarRTMapLayersLowerDesaturation(
	TEXT("rt.Map.Layers.LowerDesaturation"), -1.f,
	TEXT("Saturazione persa per piano di distanza sotto l'attivo (0..1, cumulata). Negativo = default (0,5)."), ECVF_Default);

static TAutoConsoleVariable<float> CVarRTMapLayersLowerDarkening(
	TEXT("rt.Map.Layers.LowerDarkening"), -1.f,
	TEXT("Luce persa per piano di distanza sotto l'attivo (0..1, cumulata). Negativo = default (0,35)."), ECVF_Default);

static TAutoConsoleVariable<float> CVarRTMapLayersActiveBorderStops(
	TEXT("rt.Map.Layers.ActiveBorderStops"), -1.f,
	TEXT("Esposizione del bordo delle celle del piano attivo, in stop (+6 = x64). Negativo = default (6)."), ECVF_Default);

FRTLayerPresentationTuning FRTLayerPresentationTuning::FromConsole()
{
	FRTLayerPresentationTuning T;
	auto Leva = [](const TAutoConsoleVariable<float>& CVar, float& Valore, float Max)
	{
		const float Letto = CVar.GetValueOnGameThread();
		if (Letto >= 0.f)
		{
			Valore = FMath::Min(Letto, Max);
		}
	};
	Leva(CVarRTMapLayersUpperOpacity, T.UpperOpacity, 1.f);
	Leva(CVarRTMapLayersLowerDesaturation, T.LowerDesaturationPerLayer, 1.f);
	Leva(CVarRTMapLayersLowerDarkening, T.LowerDarkeningPerLayer, 0.99f);
	Leva(CVarRTMapLayersActiveBorderStops, T.ActiveBorderStops, 10.f);
	return T;
}

FRTLayerTreatment RTLayerPresentation::Treatment(int32 Layer, int32 ActiveLayer, bool bMultiLayer,
	const FRTLayerPresentationTuning& Tuning)
{
	FRTLayerTreatment T;
	if (!bMultiLayer)
	{
		return T; // una mappa a un piano si vede come prima di #3679
	}

	if (Layer == ActiveLayer)
	{
		T.BorderExposureStops = Tuning.ActiveBorderStops;
	}
	else if (Layer > ActiveLayer)
	{
		// Sopra: traslucido, colori invariati. Lo stesso per ogni piano sopra: cio' che serve e' vedere ATTRAVERSO.
		T.Transparency = 1.f - FMath::Clamp(Tuning.UpperOpacity, 0.f, 1.f);
	}
	else
	{
		// Sotto: piu' lontano, piu' spento. Cumulato, cosi' il secondo piano sotto non e' mai piu' acceso del primo.
		const int32 Distanza = ActiveLayer - Layer;
		const float Saturazione = FMath::Pow(1.f - FMath::Clamp(Tuning.LowerDesaturationPerLayer, 0.f, 1.f), static_cast<float>(Distanza));
		const float Luce = FMath::Pow(1.f - FMath::Clamp(Tuning.LowerDarkeningPerLayer, 0.f, 0.99f), static_cast<float>(Distanza));
		T.Desaturation = 1.f - Saturazione;
		T.ExposureStops = FMath::Log2(Luce);
	}
	// Il bordo segue il suo piano, come la cella, tranne sull'attivo dove si marca.
	if (Layer != ActiveLayer)
	{
		T.BorderExposureStops = T.ExposureStops;
	}
	return T;
}
