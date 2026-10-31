#include "UI/SurvivalRadialIndicatorWidget.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

class SSurvivalRadialIndicator : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SSurvivalRadialIndicator) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Percent = 1.0f;
		FillColor = FLinearColor::White;
		OutlineColor = FLinearColor(0.12f, 0.12f, 0.12f, 0.9f);
		LineThickness = 3.0f;
	}

	void SetPercent(const float InPercent)
	{
		Percent = FMath::Clamp(InPercent, 0.0f, 1.0f);
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	void SetColors(const FLinearColor& InFillColor, const FLinearColor& InOutlineColor, const float InLineThickness)
	{
		FillColor = InFillColor;
		OutlineColor = InOutlineColor;
		LineThickness = FMath::Max(InLineThickness, 1.0f);
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(58.0f, 58.0f);
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const FVector2D Size = AllottedGeometry.GetLocalSize();
		const FVector2D Center = Size * 0.5f;
		const float Radius = FMath::Max(FMath::Min(Size.X, Size.Y) * 0.5f - LineThickness, 1.0f);
		TArray<FVector2D> OutlinePoints;
		BuildArcPoints(OutlinePoints, Center, Radius, 360.0f, 64);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), OutlinePoints, ESlateDrawEffect::None, OutlineColor * InWidgetStyle.GetColorAndOpacityTint(), true, LineThickness);

		if (Percent > KINDA_SMALL_NUMBER)
		{
			TArray<FVector2D> FillPoints;
			BuildArcPoints(FillPoints, Center, Radius, 360.0f * Percent, FMath::Max(FMath::CeilToInt(64.0f * Percent), 2));
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), FillPoints, ESlateDrawEffect::None, FillColor * InWidgetStyle.GetColorAndOpacityTint(), true, LineThickness);
		}

		return LayerId + 2;
	}

private:
	static void BuildArcPoints(TArray<FVector2D>& OutPoints, const FVector2D& Center, const float Radius, const float ArcDegrees, const int32 SegmentCount)
	{
		OutPoints.Reset(SegmentCount + 1);
		for (int32 SegmentIndex = 0; SegmentIndex <= SegmentCount; ++SegmentIndex)
		{
			const float Degrees = -90.0f + ArcDegrees * static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float Radians = FMath::DegreesToRadians(Degrees);
			OutPoints.Add(Center + FVector2D(FMath::Cos(Radians), FMath::Sin(Radians)) * Radius);
		}
	}

	float Percent = 1.0f;
	FLinearColor FillColor = FLinearColor::White;
	FLinearColor OutlineColor = FLinearColor::White;
	float LineThickness = 3.0f;
};

USurvivalRadialIndicatorWidget::USurvivalRadialIndicatorWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void USurvivalRadialIndicatorWidget::SetPercent(const float InPercent)
{
	Percent = FMath::Clamp(InPercent, 0.0f, 1.0f);
	if (MyRadialIndicator.IsValid())
	{
		MyRadialIndicator->SetPercent(Percent);
	}
}

TSharedRef<SWidget> USurvivalRadialIndicatorWidget::RebuildWidget()
{
	MyRadialIndicator = SNew(SSurvivalRadialIndicator);
	return MyRadialIndicator.ToSharedRef();
}

void USurvivalRadialIndicatorWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (MyRadialIndicator.IsValid())
	{
		MyRadialIndicator->SetPercent(Percent);
		MyRadialIndicator->SetColors(FillColor, OutlineColor, LineThickness);
	}
}

void USurvivalRadialIndicatorWidget::ReleaseSlateResources(const bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyRadialIndicator.Reset();
}
