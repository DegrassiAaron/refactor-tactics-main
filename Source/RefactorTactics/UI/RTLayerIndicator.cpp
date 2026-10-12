#include "UI/RTLayerIndicator.h"

FRTLayerIndicatorView RTLayerIndicator::Build(bool bHasRange, int32 Active, int32 MinLayer, int32 MaxLayer)
{
	FRTLayerIndicatorView Vista;
	if (!bHasRange || MaxLayer <= MinLayer)
	{
		return Vista; // senza mappa, o con un piano solo, non c'e' niente da scegliere
	}

	const int32 Attivo = FMath::Clamp(Active, MinLayer, MaxLayer);
	Vista.bVisible = true;
	Vista.Ordinal = Attivo - MinLayer + 1;
	Vista.Count = MaxLayer - MinLayer + 1;
	Vista.bCanGoUp = Attivo < MaxLayer;
	Vista.bCanGoDown = Attivo > MinLayer;
	Vista.Label = FText::Format(NSLOCTEXT("RTHud", "LayerIndicator", "Piano {0} di {1}"),
		FText::AsNumber(Vista.Ordinal), FText::AsNumber(Vista.Count));
	return Vista;
}

URTMouseOnlyButton::URTMouseOnlyButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InitIsFocusable(false);
}
