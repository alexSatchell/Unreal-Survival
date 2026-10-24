#include "Interaction/InteractionPromptWidget.h"
#include "Interaction/SurvivalPlayerController.h"

void UInteractionPromptWidget::SetPromptData(const FInteractionPromptData& InPromptData)
{
	bool bOptionsUnchanged = PromptData.Options.Num() == InPromptData.Options.Num();
	if (bOptionsUnchanged)
	{
		for (int32 OptionIndex = 0; OptionIndex < PromptData.Options.Num(); ++OptionIndex)
		{
			if (PromptData.Options[OptionIndex].ActionId != InPromptData.Options[OptionIndex].ActionId || !PromptData.Options[OptionIndex].DisplayText.EqualTo(InPromptData.Options[OptionIndex].DisplayText))
			{
				bOptionsUnchanged = false;
				break;
			}
		}
	}

	const bool bPromptUnchanged = PromptData.Style == InPromptData.Style && PromptData.State == InPromptData.State && PromptData.ActionText.EqualTo(InPromptData.ActionText) &&
		PromptData.TargetText.EqualTo(InPromptData.TargetText) && PromptData.DescriptionText.EqualTo(InPromptData.DescriptionText) && PromptData.DefaultActionId == InPromptData.DefaultActionId &&
		PromptData.SelectedOptionIndex == InPromptData.SelectedOptionIndex && bOptionsUnchanged;
	if (bPromptUnchanged)
	{
		return;
	}

	PromptData = InPromptData;
	OnPromptDataChanged(PromptData);
}

void UInteractionPromptWidget::SetInteractionState(bool bInFocused, bool bInMenuOpen, float InIdleDotOpacity)
{
	bIsInteractionFocused = bInFocused;
	bIsInteractionMenuOpen = bInMenuOpen;
	IdleDotOpacity = FMath::Clamp(InIdleDotOpacity, 0.0f, 1.0f);
	OnInteractionStateChanged(bIsInteractionFocused, bIsInteractionMenuOpen, IdleDotOpacity);
}

void UInteractionPromptWidget::ConfirmInteractionOption(FName ActionId)
{
	if (ASurvivalPlayerController* SurvivalPlayerController = Cast<ASurvivalPlayerController>(GetOwningPlayer()))
	{
		SurvivalPlayerController->SelectInteractionOption(ActionId);
	}
}
