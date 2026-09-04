#include "UI/RTUnitOverlayWidget.h"

void URTUnitOverlayWidget::SetOverlayView(const FRTUnitOverlayView& InView)
{
	View = InView;
	OnOverlayUpdated();
}
