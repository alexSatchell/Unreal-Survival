#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SurvivalHud.generated.h"

class UInteractionPromptWidget;
class USurvivalInventoryOverlayWidget;
class USurvivalVitalsWidget;
struct FInteractionPromptData;

/** Displays the player-facing HUD for a Survival match. */
UCLASS()
class SURVIVAL_API ASurvivalHud : public AHUD
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

public:
	void InitializeInteractionPrompt(TSubclassOf<UInteractionPromptWidget> InWidgetClass);
	void InitializeVitalsWidget(TSubclassOf<USurvivalVitalsWidget> InWidgetClass);
	void InitializeInventoryOverlayWidget(TSubclassOf<USurvivalInventoryOverlayWidget> InWidgetClass);
	void SetInventoryOverlayVisible(bool bInVisible);
	bool IsInventoryOverlayVisible() const;
	void UpdateInteractionPrompt(const FInteractionPromptData& InPromptData, bool bInFocused, bool bInMenuOpen);
	void LogLocalPlayerState() const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|UI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InteractionDotOpacity = 0.35f;

	UPROPERTY(Transient)
	TObjectPtr<UInteractionPromptWidget> InteractionPromptWidget;

	UPROPERTY(Transient)
	TObjectPtr<USurvivalVitalsWidget> VitalsWidget;

	UPROPERTY(Transient)
	TObjectPtr<USurvivalInventoryOverlayWidget> InventoryOverlayWidget;
};
