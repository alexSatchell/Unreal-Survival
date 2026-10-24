#include "SurvivalItemDefinition.h"

void USurvivalItemDefinition::GetWorldInteractionOptions(TArray<FInteractionOptionData>& OutOptions) const
{
	FInteractionOptionData StoreOption;
	StoreOption.ActionId = SurvivalItemInteractionActions::Store;
	StoreOption.DisplayText = NSLOCTEXT("Survival", "StoreItemAction", "Store");
	OutOptions.Add(StoreOption);
}
