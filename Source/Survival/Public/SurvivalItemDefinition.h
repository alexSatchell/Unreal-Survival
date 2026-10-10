#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SurvivalItemDefinition.generated.h"

/** Defines shared identity and presentation data for an item type used by pickups and inventories. */
UCLASS(BlueprintType)
class SURVIVAL_API USurvivalItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxStackSize = 1;
};
