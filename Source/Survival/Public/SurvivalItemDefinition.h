#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Interfaces/Interactable.h"
#include "UObject/PrimaryAssetId.h"
#include "SurvivalItemDefinition.generated.h"

class UStaticMesh;

namespace SurvivalItemInteractionActions
{
	static const FName Store = FName(TEXT("Store"));
	static const FName Consume = FName(TEXT("Consume"));
}

/** Defines static data shared by every instance of an item type; inventories store instance identity while the server resolves this definition for weight and presentation. */
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

	virtual void GetWorldInteractionOptions(TArray<FInteractionOptionData>& OutOptions) const;
};
