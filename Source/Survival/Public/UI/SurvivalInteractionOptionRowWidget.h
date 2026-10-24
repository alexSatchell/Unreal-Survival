#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/Interactable.h"
#include "SurvivalInteractionOptionRowWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSurvivalInteractionOptionSelected, FName, ActionId);

/** Displays one selectable action in the focused container interaction menu. */
UCLASS()
class SURVIVAL_API USurvivalInteractionOptionRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeOption(const FInteractionOptionData& InOption, bool bInSelected);

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnSurvivalInteractionOptionSelected OnOptionSelected;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidget();
	void RefreshRow();

	UFUNCTION()
	void HandleClicked();

	FInteractionOptionData Option;
	bool bIsSelected = false;

	UPROPERTY(Transient)
	TObjectPtr<UButton> OptionButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> OptionLabel;
};
