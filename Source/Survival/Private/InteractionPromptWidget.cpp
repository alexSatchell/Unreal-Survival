#include "InteractionPromptWidget.h"

void UInteractionPromptWidget::SetPromptData(const FInteractionPromptData& InPromptData)
{
	const bool bPromptUnchanged = PromptData.Style == InPromptData.Style && PromptData.State == InPromptData.State && PromptData.ActionText.EqualTo(InPromptData.ActionText) && PromptData.TargetText.EqualTo(InPromptData.TargetText) &&
		PromptData.DescriptionText.EqualTo(InPromptData.DescriptionText) && PromptData.WorldOffset == InPromptData.WorldOffset;
	if (bPromptUnchanged)
	{
		return;
	}

	PromptData = InPromptData;
	OnPromptDataChanged(PromptData);
}
