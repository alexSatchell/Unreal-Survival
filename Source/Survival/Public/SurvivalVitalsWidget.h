#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SurvivalVitalsWidget.generated.h"

class ASurvivalCharacter;
class USurvivalRadialIndicatorWidget;

/** Displays the player's four current vital percentages in a compact bottom-right HUD group. */
UCLASS()
class SURVIVAL_API USurvivalVitalsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForCharacter(ASurvivalCharacter* InCharacter);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildWidget();
	void BindToCharacter();

	UFUNCTION()
	void HandleVitalsChanged(float HealthPercent, float HungerPercent, float HydrationPercent, float SicknessPercent);

	void SetIndicator(USurvivalRadialIndicatorWidget* Indicator, float Percent);

	TWeakObjectPtr<ASurvivalCharacter> Character;
	UPROPERTY(Transient)
	TObjectPtr<USurvivalRadialIndicatorWidget> HealthIndicator;
	UPROPERTY(Transient)
	TObjectPtr<USurvivalRadialIndicatorWidget> ThirstIndicator;
	UPROPERTY(Transient)
	TObjectPtr<USurvivalRadialIndicatorWidget> HungerIndicator;
	UPROPERTY(Transient)
	TObjectPtr<USurvivalRadialIndicatorWidget> SicknessIndicator;
	bool bIsBoundToVitals = false;
};
