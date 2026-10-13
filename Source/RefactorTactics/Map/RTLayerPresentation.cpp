#include "Map/RTLayerPresentation.h"
#include "HAL/IConsoleManager.h"
#include "Map/RTHexLibrary.h"
#include "UI/RTUIPalette.h" // il bianco e il viola dei bordi sono token di §32 (#3704): una sede sola per il valore

// Le leve di #3679: negative = usa il default. Si applicano al fotogramma dopo (`SyncActiveLayerPresentation`).
// ⌫ *`rt.Map.Layers.ActiveBorderStops` non c'e' piu' da #3704: il bordo dell'attivo e' nero per D-508, non marcato.*
static TAutoConsoleVariable<float> CVarRTMapLayersUpperOpacity(
	TEXT("rt.Map.Layers.UpperOpacity"), -1.f,
	TEXT("Opacita' dei piani sopra quello attivo (0..1). Negativo = default (0,35)."), ECVF_Default);

static TAutoConsoleVariable<float> CVarRTMapLayersUpperExposure(
	TEXT("rt.Map.Layers.UpperExposure"), -1.f,
	TEXT("Di quanti stop si schiarisce un piano sopra quello attivo (0..4, +1 = luce doppia; 0 = spento). Negativo = default (1,5)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarRTMapLayersLowerDesaturation(
	TEXT("rt.Map.Layers.LowerDesaturation"), -1.f,
	TEXT("Saturazione persa per piano di distanza sotto l'attivo (0..1, cumulata). Negativo = default (1)."), ECVF_Default);

static TAutoConsoleVariable<float> CVarRTMapLayersLowerDarkening(
	TEXT("rt.Map.Layers.LowerDarkening"), -1.f,
	TEXT("Luce persa per piano di distanza sotto l'attivo (0..1, cumulata). Negativo = default (0,4)."), ECVF_Default);

static TAutoConsoleVariable<float> CVarRTMapLayersVolumeTint(
	TEXT("rt.Map.Layers.VolumeTint"), -1.f,
	TEXT("Quanto un volume (muro, copertura, porta, blocco) prende il colore del suo piano (0..1). Negativo = default (0,25)."),
	ECVF_Default);

const TCHAR* const RTLayerPresentation::VolumeMaterialPackage = TEXT("/Game/RT/Core/Grid/M_HexVolume");
const TCHAR* const RTLayerPresentation::VolumeMaterialName = TEXT("M_HexVolume");
const FName RTLayerPresentation::VolumeParamOwnColor(TEXT("OwnColor"));
const FName RTLayerPresentation::VolumeParamRoughness(TEXT("Roughness"));

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
	Leva(CVarRTMapLayersUpperExposure, T.UpperExposureStops, RTLayerPresentation::MaxUpperExposureStops);
	Leva(CVarRTMapLayersLowerDesaturation, T.LowerDesaturationPerLayer, 1.f);
	Leva(CVarRTMapLayersLowerDarkening, T.LowerDarkeningPerLayer, 0.99f);
	Leva(CVarRTMapLayersVolumeTint, T.VolumeTintStrength, 1.f);
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

FLinearColor RTLayerPresentation::VolumeOwnColor(bool bEdgePanel)
{
	// Tarati sulle catture di prima di #3706 (`Visual.Map.BlockVolumes`, `Visual.Map.GrayKitYard`): vedi l'header.
	if (bEdgePanel)
	{
		return FLinearColor(0.092f, 0.092f, 0.092f, 1.f);
	}
	return FLinearColor(0.13f, 0.13f, 0.13f, 1.f);
}

FRTLayerVolumeTint RTLayerPresentation::VolumeTint(int32 Layer, int32 ActiveLayer, bool bMultiLayer,
	const FRTLayerPresentationTuning& Tuning)
{
	FRTLayerVolumeTint Tint;
	if (!bMultiLayer)
	{
		return Tint; // una mappa a un piano non ha niente da dire: nessuna tinta, come prima di #3706
	}
	// Il colore e' quello del bordo: una sede sola per «che colore ha questo piano».
	Tint.Color = FLinearColor::FromSRGBColor(BorderColorFor(Relation(Layer, ActiveLayer, bMultiLayer)));
	Tint.Strength = FMath::Clamp(Tuning.VolumeTintStrength, 0.f, 1.f);
	return Tint;
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
		// Sopra: traslucido. Lo stesso per ogni piano sopra: cio' che serve e' vedere ATTRAVERSO.
		T.Transparency = 1.f - FMath::Clamp(Tuning.UpperOpacity, 0.f, 1.f);
		// #3674: e schiarito. Mai scurito: «sopra» non deve leggersi come «lontano», che e' di «sotto».
		T.ExposureStops = FMath::Clamp(Tuning.UpperExposureStops, 0.f, MaxUpperExposureStops);
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
