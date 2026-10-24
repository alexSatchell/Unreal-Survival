#pragma once

#include "CoreMinimal.h"
#include "UObject/PrimaryAssetId.h"
#include "SurvivalItemTypes.generated.h"

/** Identifies one physical item instance without duplicating the shared data asset that defines its item type. */
USTRUCT(BlueprintType)
struct SURVIVAL_API FSurvivalItemInstance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FGuid InstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FPrimaryAssetId DefinitionAssetId;

	UPROPERTY(BlueprintReadOnly, Category = "Item|Presentation")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Item|Presentation")
	bool bIsConsumable = false;

	bool IsValid() const { return InstanceId.IsValid() && DefinitionAssetId.IsValid(); }
};
