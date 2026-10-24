#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/Interactable.h"
#include "SurvivalInteractionMenuWidget.generated.h"

class ASurvivalPlayerController;
class UScrollBox;
class UTextBlock;
class USurvivalInteractionOptionRowWidget;

/** Code-built action menu used for interactables that expose multiple container actions. */
UCLASS()
class SURVIVAL_API USurvivalInteractionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForController(ASurvivalPlayerController* InController);
	void SetPromptData(const FInteractionPromptData& InPromptData);
	void SetMenuOpen(bool bInOpen);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidget();
	void RefreshOptions();

	UFUNCTION()
	void HandleOptionSelected(FName ActionId);

	TWeakObjectPtr<ASurvivalPlayerController> PlayerController;
	FInteractionPromptData PromptData;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Heading;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> OptionList;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USurvivalInteractionOptionRowWidget>> OptionRows;
};
