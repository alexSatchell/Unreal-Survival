#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/PrimaryAssetId.h"
#include "SurvivalItemDefinition.generated.h"

class UStaticMesh;

/** Defines shared identity and presentation data for an item type used by pickups and inventories. */
UCLASS(BlueprintType)
class SURVIVAL_API USurvivalItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static FPrimaryAssetType GetPrimaryAssetType() { return FPrimaryAssetType(TEXT("SurvivalItemDefinition")); }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Physical", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|World")
	TObjectPtr<UStaticMesh> PickupMesh;
};
