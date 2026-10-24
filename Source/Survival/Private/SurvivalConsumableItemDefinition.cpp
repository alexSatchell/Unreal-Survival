#include "SurvivalConsumableItemDefinition.h"

void USurvivalConsumableItemDefinition::GetWorldInteractionOptions(TArray<FInteractionOptionData>& OutOptions) const
{
	Super::GetWorldInteractionOptions(OutOptions);
	FInteractionOptionData ConsumeOption;
	ConsumeOption.ActionId = SurvivalItemInteractionActions::Consume;
	ConsumeOption.DisplayText = NSLOCTEXT("Survival", "ConsumeItemAction", "Consume");
	OutOptions.Add(ConsumeOption);
}
