#pragma once

#include "CoreMinimal.h"
#include "Interfaces/Interactable.h"
#include "Blueprint/UserWidget.h"
#include "InteractionPromptWidget.generated.h"

/** Provides reusable prompt data to a Blueprint widget that presents a focused interaction. */
UCLASS(Abstract, Blueprintable)
class SURVIVAL_API UInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetPromptData(const FInteractionPromptData& InPromptData);

	UFUNCTION(BlueprintPure, Category = "Interaction")
	FInteractionPromptData GetPromptData() const { return PromptData; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FInteractionPromptData PromptData;

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnPromptDataChanged(const FInteractionPromptData& InPromptData);
};
