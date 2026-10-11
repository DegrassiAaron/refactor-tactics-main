#include "UI/RTHudLayout.h"

#include "Engine/UserInterfaceSettings.h"

const FRTHudBands& RTHudLayout::Bands()
{
	static const FRTHudBands Dato;
	return Dato;
}

float RTHudLayout::UiScaleFor(const FIntPoint& ViewportPixels)
{
	if (ViewportPixels.X <= 0 || ViewportPixels.Y <= 0)
	{
		return 1.f;
	}
	const float Scala = GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(ViewportPixels);
	return Scala > 0.f ? Scala : 1.f;
}

FBox2D RTHudLayout::SceneRectSlate(const FVector2D& ViewportSlate, const FRTHudBands& InBands)
{
	return FBox2D(
		FVector2D(InBands.Left, InBands.Top),
		FVector2D(ViewportSlate.X - InBands.Right, ViewportSlate.Y - InBands.Bottom));
}

FBox2D RTHudLayout::SceneRectPixels(const FIntPoint& ViewportPixels, float UiScale, const FRTHudBands& InBands)
{
	const FVector2D Viewport(ViewportPixels.X, ViewportPixels.Y);
	const FBox2D Intero(FVector2D::ZeroVector, Viewport);
	if (ViewportPixels.X <= 0 || ViewportPixels.Y <= 0 || UiScale <= 0.f)
	{
		return Intero;
	}

	const FBox2D Slate = SceneRectSlate(Viewport / UiScale, InBands);
	const FBox2D Pixel(Slate.Min * UiScale, Slate.Max * UiScale);
	if (Pixel.Max.X - Pixel.Min.X < 1.0 || Pixel.Max.Y - Pixel.Min.Y < 1.0)
	{
		return Intero;
	}
	return Pixel;
}

void RTHudLayout::SceneOriginSize(const FIntPoint& ViewportPixels, float UiScale, const FRTHudBands& InBands,
	FVector2D& OutOrigin, FVector2D& OutSize)
{
	if (ViewportPixels.X <= 0 || ViewportPixels.Y <= 0)
	{
		OutOrigin = FVector2D::ZeroVector;
		OutSize = FVector2D(1.0, 1.0);
		return;
	}
	const FVector2D Viewport(ViewportPixels.X, ViewportPixels.Y);
	const FBox2D Pixel = SceneRectPixels(ViewportPixels, UiScale, InBands);
	OutOrigin = Pixel.Min / Viewport;
	OutSize = (Pixel.Max - Pixel.Min) / Viewport;
}

FAnchorData RTHudLayout::ZoneLayout(ERTHudZone Zone, const FRTHudBands& InBands)
{
	const float M = ZoneMargin;
	const float L = InBands.Left;
	const float R = InBands.Right;
	const float T = InBands.Top;
	const float B = InBands.Bottom;

	// Con anchor stirati su un asse, `Left`/`Right` (o `Top`/`Bottom`) sono MARGINI dai bordi dell'ancora; con
	// anchor a punto, `Right` (o `Bottom`) e' la MISURA. E' la lettura di `SConstraintCanvas::OnArrangeChildren`.
	FAnchorData Layout;
	Layout.Alignment = FVector2D::ZeroVector;
	switch (Zone)
	{
	case ERTHudZone::TopLeft:
		// La colonna sinistra ha il suo taglio, `LeftTop`, e non quello della fascia alta.
		Layout.Anchors = FAnchors(0.f, 0.f, 0.f, 0.f);
		Layout.Offsets = FMargin(M, M, L - 2.f * M, InBands.LeftTop - 2.f * M);
		break;
	case ERTHudZone::TopCenter:
		Layout.Anchors = FAnchors(0.f, 0.f, 1.f, 0.f);
		Layout.Offsets = FMargin(L + M, M, R + M, T - 2.f * M);
		break;
	case ERTHudZone::TopRight:
		Layout.Anchors = FAnchors(1.f, 0.f, 1.f, 0.f);
		Layout.Offsets = FMargin(-R + M, M, R - 2.f * M, T - 2.f * M);
		break;
	case ERTHudZone::MiddleLeft:
		Layout.Anchors = FAnchors(0.f, 0.f, 0.f, 1.f);
		Layout.Offsets = FMargin(M, InBands.LeftTop + M, L - 2.f * M, B + M);
		break;
	case ERTHudZone::MiddleRight:
		Layout.Anchors = FAnchors(1.f, 0.f, 1.f, 1.f);
		Layout.Offsets = FMargin(-R + M, T + M, R - 2.f * M, B + M);
		break;
	case ERTHudZone::Bottom:
		Layout.Anchors = FAnchors(0.f, 1.f, 1.f, 1.f);
		Layout.Offsets = FMargin(M, -B + M, M, B - 2.f * M);
		break;
	default:
		// `Count` non e' una zona: nessun posto, e una zona con quell'id la vede il test che le conta.
		Layout.Anchors = FAnchors(0.f, 0.f, 0.f, 0.f);
		Layout.Offsets = FMargin(0.f);
		break;
	}
	return Layout;
}

FVector2D RTHudLayout::SceneCenterOffsetSlate(const FRTHudBands& InBands)
{
	return FVector2D((InBands.Left - InBands.Right) * 0.5f, (InBands.Top - InBands.Bottom) * 0.5f);
}
