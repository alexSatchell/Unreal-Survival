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

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionState(bool bInFocused, bool bInMenuOpen, float InIdleDotOpacity);

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsInteractionFocused() const { return bIsInteractionFocused; }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsInteractionMenuOpen() const { return bIsInteractionMenuOpen; }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	float GetIdleDotOpacity() const { return IdleDotOpacity; }

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void ConfirmInteractionOption(FName ActionId);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FInteractionPromptData PromptData;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool bIsInteractionFocused = false;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool bIsInteractionMenuOpen = false;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IdleDotOpacity = 0.35f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnPromptDataChanged(const FInteractionPromptData& InPromptData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnInteractionStateChanged(bool bInFocused, bool bInMenuOpen, float InIdleDotOpacity);
};
