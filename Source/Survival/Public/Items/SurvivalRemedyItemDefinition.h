#pragma once

#include "CoreMinimal.h"
#include "Items/SurvivalConsumableItemDefinition.h"
#include "SurvivalRemedyItemDefinition.generated.h"

/** Defines a consumable item category intended to provide sickness recovery or other remedy-specific effects. */
UCLASS(BlueprintType)
class SURVIVAL_API USurvivalRemedyItemDefinition : public USurvivalConsumableItemDefinition
{
	GENERATED_BODY()
};

