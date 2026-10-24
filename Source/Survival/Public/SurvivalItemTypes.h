#pragma once

#include "CoreMinimal.h"
#include "UObject/PrimaryAssetId.h"
#include "SurvivalItemTypes.generated.h"

/** Identifies one physical item instance and its static item definition. */
USTRUCT(BlueprintType)
struct SURVIVAL_API FSurvivalItemInstance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FGuid InstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FPrimaryAssetId ItemDefinitionId;

	bool IsValid() const { return InstanceId.IsValid() && ItemDefinitionId.IsValid(); }
};
