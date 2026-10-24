#include "InteractionPromptWidget.h"
#include "SurvivalPlayerController.h"

void UInteractionPromptWidget::SetPromptData(const FInteractionPromptData& InPromptData)
{
	// A different option count immediately means the visible option list changed, so there is no matching entry-by-entry comparison to perform.
	bool bOptionsUnchanged = PromptData.Options.Num() == InPromptData.Options.Num();
	if (bOptionsUnchanged)
	{
		// Compare each option at the same index because the selected option is tracked by its position in this array.
		for (int32 OptionIndex = 0; OptionIndex < PromptData.Options.Num(); ++OptionIndex)
		{
			// The action ID controls which interaction is selected, while the display text controls what the player sees.
			if (PromptData.Options[OptionIndex].ActionId != InPromptData.Options[OptionIndex].ActionId || !PromptData.Options[OptionIndex].DisplayText.EqualTo(InPromptData.Options[OptionIndex].DisplayText))
			{
				bOptionsUnchanged = false;
				// One different option is enough to require a prompt refresh.
				break;
			}
		}
	}

	// Avoid notifying the Blueprint when repeated focus updates contain the same visible prompt data.
	const bool bPromptUnchanged = PromptData.Style == InPromptData.Style && PromptData.State == InPromptData.State && PromptData.ActionText.EqualTo(InPromptData.ActionText) &&
		PromptData.TargetText.EqualTo(InPromptData.TargetText) && PromptData.DescriptionText.EqualTo(InPromptData.DescriptionText) && PromptData.DefaultActionId == InPromptData.DefaultActionId &&
		PromptData.SelectedOptionIndex == InPromptData.SelectedOptionIndex && bOptionsUnchanged;
	if (bPromptUnchanged)
	{
		return;
	}

	// Store the changed data and let the Blueprint update the visible prompt.
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
