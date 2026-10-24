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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ToolTip = "Item definition awarded when this loot entry is selected."))
	TObjectPtr<USurvivalItemDefinition> ItemDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ToolTip = "All of these context tags must be present on the spawn point for this entry to be eligible."))
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ToolTip = "If any of these context tags are present on the spawn point, this entry is excluded."))
	FGameplayTagContainer ExcludedTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0.0", UIMin = "0.0", ToolTip = "Relative selection weight among eligible entries. Higher values are selected more often; zero disables this entry."))
	float Weight = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1", UIMin = "1", ToolTip = "Lowest number of this item awarded when the entry is selected."))
	int32 MinimumQuantity = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1", UIMin = "1", ToolTip = "Highest number of this item awarded when the entry is selected. The actual quantity is randomized inclusively between the minimum and maximum."))
	int32 MaximumQuantity = 1;
};

UCLASS(BlueprintType)
class SURVIVAL_API USurvivalLootTable : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0", UIMin = "0", ToolTip = "Maximum total item instances this table can award during one population roll."))
	int32 MaximumRolledItems = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Chance that each roll returns no item. Zero guarantees a result when at least one valid entry exists; one always returns empty."))
	float EmptyRollChance = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ToolTip = "Weighted item entries evaluated for each roll. Entries are filtered by the spawn point's context tags."))
	TArray<FSurvivalLootTableEntry> Entries;

	void RollItems(const FGameplayTagContainer& ContextTags, FRandomStream& RandomStream, int32 InMaximumItems, TArray<USurvivalItemDefinition*>& OutDefinitions) const;
};
