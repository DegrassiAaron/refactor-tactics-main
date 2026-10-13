#include "Unit/RTUnitLayerOutline.h"
#include "HAL/IConsoleManager.h"

// Le leve di #3705: negative = usa il default. L'HUD le rilegge a ogni giro e riscrive solo se sono cambiate.
static TAutoConsoleVariable<int32> CVarRTUnitLayerOutline(
	TEXT("rt.Unit.LayerOutline"), -1,
	TEXT("Il contorno del modello che dice il piano dell'unita' (D-508): 0 = spento, 1 = acceso. Negativo = default (acceso)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarRTUnitLayerOutlineThickness(
	TEXT("rt.Unit.LayerOutline.Thickness"), -1.f,
	TEXT("Spessore del contorno delle unita', in pixel a 1080 righe (0..8). Negativo = default (3)."),
	ECVF_Default);

const TCHAR* const RTUnitLayerOutline::MaterialPackage = TEXT("/Game/RT/Characters/Shared/Materials/M_UnitLayerOutline");
const TCHAR* const RTUnitLayerOutline::MaterialName = TEXT("M_UnitLayerOutline");

const FName RTUnitLayerOutline::ParamColorActive(TEXT("ColorActive"));
const FName RTUnitLayerOutline::ParamColorAbove(TEXT("ColorAbove"));
const FName RTUnitLayerOutline::ParamColorBelow(TEXT("ColorBelow"));
const FName RTUnitLayerOutline::ParamThickness(TEXT("ThicknessPx"));

uint8 RTUnitLayerOutline::StencilFor(ERTLayerRelation InRelation)
{
	switch (InRelation)
	{
	case ERTLayerRelation::Above: return StencilAbove;
	case ERTLayerRelation::Below: return StencilBelow;
	case ERTLayerRelation::Active:
	default:
		return StencilActive;
	}
}

uint8 RTUnitLayerOutline::StencilForLayer(int32 UnitLayer, int32 ActiveLayer, bool bMultiLayer)
{
	// La relazione e' quella dei bordi delle celle: una sede sola, cosi' unita' e celle non possono dirsi piani diversi.
	return StencilFor(RTLayerPresentation::Relation(UnitLayer, ActiveLayer, bMultiLayer));
}

bool RTUnitLayerOutline::ShowsThroughGeometry(uint8 Stencil)
{
	// #3705, scope 5: solo chi sta SOTTO il piano attivo, perche' e' quel piano, pieno, a coprirlo.
	return Stencil == StencilBelow;
}

bool RTUnitLayerOutline::ColorFor(uint8 Stencil, FColor& OutColor)
{
	switch (Stencil)
	{
	case StencilActive: OutColor = RTLayerPresentation::BorderColorFor(ERTLayerRelation::Active); return true;
	case StencilAbove:  OutColor = RTLayerPresentation::BorderColorFor(ERTLayerRelation::Above);  return true;
	case StencilBelow:  OutColor = RTLayerPresentation::BorderColorFor(ERTLayerRelation::Below);  return true;
	default:
		return false;
	}
}

FName RTUnitLayerOutline::ColorParamFor(uint8 Stencil)
{
	switch (Stencil)
	{
	case StencilActive: return ParamColorActive;
	case StencilAbove:  return ParamColorAbove;
	case StencilBelow:  return ParamColorBelow;
	default:
		return NAME_None;
	}
}

FRTUnitLayerOutlineTuning FRTUnitLayerOutlineTuning::FromConsole()
{
	FRTUnitLayerOutlineTuning T;
	const int32 Acceso = CVarRTUnitLayerOutline.GetValueOnGameThread();
	if (Acceso >= 0)
	{
		T.bEnabled = Acceso != 0;
	}
	const float Spessore = CVarRTUnitLayerOutlineThickness.GetValueOnGameThread();
	if (Spessore >= 0.f)
	{
		T.ThicknessPx = FMath::Min(Spessore, RTUnitLayerOutline::MaxThicknessPx);
	}
	return T;
}
