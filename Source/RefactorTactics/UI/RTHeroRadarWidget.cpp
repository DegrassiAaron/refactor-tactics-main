#include "UI/RTHeroRadarWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/SizeBox.h"

void URTHeroRadarWidget::EnsureDrawableArea()
{
	// 🔴 Senza questa garanzia il radar non disegna mai: un `SizeBox` senza figli e senza override ha
	// desired size `0`, e uno slot `Auto` gli assegna zero pixel. Il disegno fallisce in silenzio —
	// nessun errore, nessun warning, solo una sezione vuota con tutti i test verdi.
	if (!RadarBox || MinRadarSize <= 0.f)
	{
		return;
	}

	// ⛔ Solo se l'asset NON dichiara gia' la propria misura: sovrascrivere un `SizeBox` configurato a
	// mano cancellerebbe una scelta di layout di chi ha authorato il WBP.
	if (RadarBox->GetWidthOverride() <= 0.f)
	{
		RadarBox->SetWidthOverride(MinRadarSize);
	}

	if (RadarBox->GetHeightOverride() <= 0.f)
	{
		RadarBox->SetHeightOverride(MinRadarSize);
	}
}

void URTHeroRadarWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// ⚠️ `PreConstruct` e non `Construct`, perche' vale anche nella preview del designer: e' li' che si
	// guarda un widget per decidere se e' fatto bene, ed e' esattamente li' che l'area zero si e'
	// nascosta finche' non l'abbiamo aperto.
	EnsureDrawableArea();
}

float URTHeroRadarWidget::ComputeAxisAngleDegrees(int32 AxisIndex, int32 AxisCount)
{
	if (AxisCount <= 0)
	{
		return -90.f;
	}

	// -90 = ore 12. In coordinate schermo la Y cresce verso il basso, quindi il seno di -90 vale -1 e il
	// primo asse punta in alto — che e' la convenzione del Profile Radar pubblicato.
	return -90.f + (static_cast<float>(AxisIndex) * 360.f) / static_cast<float>(AxisCount);
}

float URTHeroRadarWidget::ComputeAxisRatio(const FRTProfileRadarAxisView& Axis)
{
	if (Axis.MaxValue <= 0)
	{
		return -1.f;
	}

	return FMath::Clamp(static_cast<float>(Axis.Value) / static_cast<float>(Axis.MaxValue), 0.f, 1.f);
}

bool URTHeroRadarWidget::ComputeRadarPoints(const TArray<FRTProfileRadarAxisView>& Axes, FVector2D Center, float Radius, TArray<FVector2D>& OutPoints)
{
	OutPoints.Reset();

	const int32 AxisCount = Axes.Num();
	if (AxisCount == 0)
	{
		// Niente da disegnare non e' un errore: e' un profilo senza radar, e la scheda nasconde la sezione.
		return true;
	}

	OutPoints.Reserve(AxisCount);

	for (int32 Index = 0; Index < AxisCount; ++Index)
	{
		const float Ratio = ComputeAxisRatio(Axes[Index]);
		if (Ratio < 0.f)
		{
			// Fail-closed: un solo asse rotto invalida la figura intera. Un poligono a cui manca un vertice
			// non e' «quasi giusto», e' una sagoma diversa da quella dell'eroe.
			OutPoints.Reset();
			return false;
		}

		const float AngleRadians = FMath::DegreesToRadians(ComputeAxisAngleDegrees(Index, AxisCount));
		const float Distance = Radius * Ratio;

		OutPoints.Add(FVector2D(
			Center.X + Distance * FMath::Cos(AngleRadians),
			Center.Y + Distance * FMath::Sin(AngleRadians)));
	}

	return true;
}

void URTHeroRadarWidget::ComputeRingPoints(int32 AxisCount, FVector2D Center, float Radius, TArray<FVector2D>& OutPoints)
{
	OutPoints.Reset();

	if (AxisCount <= 0)
	{
		return;
	}

	OutPoints.Reserve(AxisCount);

	for (int32 Index = 0; Index < AxisCount; ++Index)
	{
		const float AngleRadians = FMath::DegreesToRadians(ComputeAxisAngleDegrees(Index, AxisCount));
		OutPoints.Add(FVector2D(
			Center.X + Radius * FMath::Cos(AngleRadians),
			Center.Y + Radius * FMath::Sin(AngleRadians)));
	}
}

float URTHeroRadarWidget::ComputeRadiusForSize(FVector2D LocalSize) const
{
	const float ShortSide = FMath::Min(LocalSize.X, LocalSize.Y);
	return FMath::Max(0.f, 0.5f * ShortSide * RadiusScale);
}

void URTHeroRadarWidget::SetRadarAxes(const TArray<FRTProfileRadarAxisView>& InAxes)
{
	RadarAxes = InAxes;
	OnRadarAxesChanged();
	InvalidateLayoutAndVolatility();
}

bool URTHeroRadarWidget::HasDrawableAxes() const
{
	if (RadarAxes.Num() == 0)
	{
		return false;
	}

	TArray<FVector2D> Points;
	return ComputeRadarPoints(RadarAxes, FVector2D::ZeroVector, 1.f, Points);
}

int32 URTHeroRadarWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const int32 AxisCount = RadarAxes.Num();
	if (AxisCount == 0)
	{
		return BaseLayer;
	}

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const float Radius = ComputeRadiusForSize(LocalSize);
	if (Radius <= 0.f)
	{
		// Un widget largo zero pixel: non c'e' niente da disegnare, e insistere produrrebbe solo linee
		// degeneri sovrapposte nell'angolo.
		return BaseLayer;
	}

	const FVector2D Center(LocalSize.X * 0.5f, LocalSize.Y * 0.5f);

	TArray<FVector2D> ValuePoints;
	const bool bDrawable = ComputeRadarPoints(RadarAxes, Center, Radius, ValuePoints);

	// ⚠️ `FPaintContext` e' costruito qui e non ricevuto: in UE 5.8 `NativePaint` ha la firma Slate
	// completa, mentre `UWidgetBlueprintLibrary::DrawLines` — l'API di disegno stabile di UMG — vuole il
	// contesto. Il costruttore prende esattamente gli stessi parametri, quindi l'adattamento e' un
	// travaso, non una reimplementazione.
	FPaintContext Context(AllottedGeometry, MyCullingRect, OutDrawElements, BaseLayer, InWidgetStyle, bParentEnabled);

	// ---- Griglia: cornice esterna piu' gli anelli intermedi -------------------------------------
	const int32 RingCount = FMath::Max(0, GridRingCount) + 1; // +1 = il bordo
	for (int32 Ring = 1; Ring <= RingCount; ++Ring)
	{
		const float RingRadius = (Radius * static_cast<float>(Ring)) / static_cast<float>(RingCount);

		TArray<FVector2D> RingPoints;
		ComputeRingPoints(AxisCount, Center, RingRadius, RingPoints);
		if (RingPoints.Num() > 0)
		{
			// La polilinea e' aperta: ripetere il primo vertice chiude il poligono.
			RingPoints.Add(RingPoints[0]);
			UWidgetBlueprintLibrary::DrawLines(Context, RingPoints, GridColor, true, GridThickness);
		}
	}

	// ---- Raggi: dal centro a ciascun asse --------------------------------------------------------
	TArray<FVector2D> BoundaryPoints;
	ComputeRingPoints(AxisCount, Center, Radius, BoundaryPoints);
	for (const FVector2D& Spoke : BoundaryPoints)
	{
		UWidgetBlueprintLibrary::DrawLine(Context, Center, Spoke, AxisColor, true, GridThickness);
	}

	// ---- Il poligono dei valori ------------------------------------------------------------------
	//
	// ⚠️ Si disegna **solo** se tutti gli assi sono rappresentabili. Griglia e raggi restano visibili
	// anche in quel caso: dicono «qui c'e' un radar», senza affermare nulla sull'eroe.
	if (bDrawable && ValuePoints.Num() > 0)
	{
		TArray<FVector2D> ClosedValuePoints = ValuePoints;
		ClosedValuePoints.Add(ValuePoints[0]);
		UWidgetBlueprintLibrary::DrawLines(Context, ClosedValuePoints, ValueColor, true, ValueThickness);
	}

	return FMath::Max(BaseLayer, Context.MaxLayer);
}
