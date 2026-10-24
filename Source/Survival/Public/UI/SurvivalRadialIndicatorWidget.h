#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "SurvivalRadialIndicatorWidget.generated.h"

/** Draws a lightweight clockwise radial fill without requiring a material asset. */
UCLASS(BlueprintType)
class SURVIVAL_API USurvivalRadialIndicatorWidget : public UWidget
{
	GENERATED_BODY()

public:
	USurvivalRadialIndicatorWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Vitals")
	void SetPercent(float InPercent);

	UFUNCTION(BlueprintPure, Category = "Vitals")
	float GetPercent() const { return Percent; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Percent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals")
	FLinearColor FillColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals")
	FLinearColor OutlineColor = FLinearColor(0.12f, 0.12f, 0.12f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float LineThickness = 3.0f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	TSharedPtr<class SSurvivalRadialIndicator> MyRadialIndicator;
};
