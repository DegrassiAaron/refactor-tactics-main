#include "Map/RTLayerPresentation.h"
#include "HAL/IConsoleManager.h"
#include "Map/RTHexLibrary.h"
#include "UI/RTUIPalette.h" // il bianco e il viola dei bordi sono token di §32 (#3704): una sede sola per il valore

// Le leve di #3679: negative = usa il default. Si applicano al fotogramma dopo (`SyncActiveLayerPresentation`).
// ⌫ *`rt.Map.Layers.ActiveBorderStops` non c'e' piu' da #3704: il bordo dell'attivo e' nero per D-504, non marcato.*
static TAutoConsoleVariable<float> CVarRTMapLayersUpperOpacity(
	TEXT("rt.Map.Layers.UpperOpacity"), -1.f,
	TEXT("Opacita' dei piani sopra quello attivo (0..1). Negativo = default (0,35)."), ECVF_Default);

static TAutoConsoleVariable<float> CVarRTMapLayersLowerDesaturation(
	TEXT("rt.Map.Layers.LowerDesaturation"), -1.f,
	TEXT("Saturazione persa per piano di distanza sotto l'attivo (0..1, cumulata). Negativo = default (0,7)."), ECVF_Default);

static TAutoConsoleVariable<float> CVarRTMapLayersLowerDarkening(
	TEXT("rt.Map.Layers.LowerDarkening"), -1.f,
	TEXT("Luce persa per piano di distanza sotto l'attivo (0..1, cumulata). Negativo = default (0,55)."), ECVF_Default);

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
	return T;
}

ERTLayerRelation RTLayerPresentation::Relation(int32 Layer, int32 ActiveLayer, bool bMultiLayer)
{
	if (!bMultiLayer || Layer == ActiveLayer)
	{
		return ERTLayerRelation::Active;
	}
	return Layer > ActiveLayer ? ERTLayerRelation::Above : ERTLayerRelation::Below;
}

FRTLayerTreatment RTLayerPresentation::Treatment(int32 Layer, int32 ActiveLayer, bool bMultiLayer,
	const FRTLayerPresentationTuning& Tuning)
{
	FRTLayerTreatment T;
	switch (Relation(Layer, ActiveLayer, bMultiLayer))
	{
	case ERTLayerRelation::Active:
		break; // l'attivo, e ogni piano di una mappa a un piano, si vedono come prima di #3679

	case ERTLayerRelation::Above:
		// Sopra: traslucido, colori invariati. Lo stesso per ogni piano sopra: cio' che serve e' vedere ATTRAVERSO.
		T.Transparency = 1.f - FMath::Clamp(Tuning.UpperOpacity, 0.f, 1.f);
		break;

	case ERTLayerRelation::Below:
	{
		// Sotto: piu' lontano, piu' spento. Cumulato, cosi' il secondo piano sotto non e' mai piu' acceso del primo.
		const int32 Distanza = ActiveLayer - Layer;
		const float Saturazione = FMath::Pow(1.f - FMath::Clamp(Tuning.LowerDesaturationPerLayer, 0.f, 1.f), static_cast<float>(Distanza));
		const float Luce = FMath::Pow(1.f - FMath::Clamp(Tuning.LowerDarkeningPerLayer, 0.f, 0.99f), static_cast<float>(Distanza));
		T.Desaturation = 1.f - Saturazione;
		T.ExposureStops = FMath::Log2(Luce);
		break;
	}
	}
	return T;
}

FColor RTLayerPresentation::BorderColorFor(ERTLayerRelation InRelation)
{
	switch (InRelation)
	{
	case ERTLayerRelation::Above: return URTUIPalette::SRGBFor(ERTUIToken::White);
	case ERTLayerRelation::Below: return URTUIPalette::SRGBFor(ERTUIToken::Violet);
	case ERTLayerRelation::Active:
	default:                      return URTHexLibrary::CellBorderColor();
	}
}
