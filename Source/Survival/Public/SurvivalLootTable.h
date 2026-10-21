#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SurvivalLootTable.generated.h"

class USurvivalItemDefinition;

USTRUCT(BlueprintType)
struct SURVIVAL_API FSurvivalLootTableEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<USurvivalItemDefinition> ItemDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	FGameplayTagContainer ExcludedTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1", UIMin = "1"))
	int32 MinimumQuantity = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1", UIMin = "1"))
	int32 MaximumQuantity = 1;
};

UCLASS(BlueprintType)
class SURVIVAL_API USurvivalLootTable : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaximumRolledItems = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TArray<FSurvivalLootTableEntry> Entries;

	void RollItems(const FGameplayTagContainer& ContextTags, FRandomStream& RandomStream, int32 InMaximumItems, TArray<USurvivalItemDefinition*>& OutDefinitions) const;
};
