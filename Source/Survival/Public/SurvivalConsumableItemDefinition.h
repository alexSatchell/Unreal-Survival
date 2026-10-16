#pragma once

#include "CoreMinimal.h"
#include "SurvivalItemDefinition.h"
#include "SurvivalConsumableItemDefinition.generated.h"

/** Defines item data for an item that can be consumed and apply effects to the character's vital state. */
UCLASS(BlueprintType)
class SURVIVAL_API USurvivalConsumableItemDefinition : public USurvivalItemDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float HydrationRestoration = 0.0f;
};
