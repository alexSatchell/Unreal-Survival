#include "Loot/SurvivalLootTable.h"
#include "Items/SurvivalItemDefinition.h"

void USurvivalLootTable::RollItems(const FGameplayTagContainer& ContextTags, FRandomStream& RandomStream, const int32 InMaximumItems, TArray<USurvivalItemDefinition*>& OutDefinitions) const
{
	// Start with an empty result so this call returns only the items from this roll.
	OutDefinitions.Reset();
	// Respect both the caller's limit and this table's configured maximum.
	const int32 ItemLimit = FMath::Min(FMath::Max(InMaximumItems, 0), FMath::Max(MaximumRolledItems, 0));
	for (int32 RollIndex = 0; RollIndex < ItemLimit; ++RollIndex)
	{
		float TotalWeight = 0.0f;
		// Sum the weights of entries eligible for this location; required tags must all match and excluded tags must not match.
		for (const FSurvivalLootTableEntry& Entry : Entries)
		{
			if (IsValid(Entry.ItemDefinition) && Entry.Weight > 0.0f && ContextTags.HasAll(Entry.RequiredTags) && !ContextTags.HasAny(Entry.ExcludedTags))
			{
				TotalWeight += Entry.Weight;
			}
		}

		// Stop if no entry can be selected, or if this roll lands in the table's empty-result chance.
		if (TotalWeight <= 0.0f || RandomStream.FRand() < FMath::Clamp(EmptyRollChance, 0.0f, 1.0f))
		{
			break;
		}

		// Pick a point across the combined eligible weight range; each entry owns a range proportional to its weight.
		const float Selection = RandomStream.FRandRange(0.0f, TotalWeight);
		float RunningWeight = 0.0f;
		// Walk the same eligible entries again to find which cumulative weight range contains the selection.
		for (const FSurvivalLootTableEntry& Entry : Entries)
		{
			if (!IsValid(Entry.ItemDefinition) || Entry.Weight <= 0.0f || !ContextTags.HasAll(Entry.RequiredTags) || ContextTags.HasAny(Entry.ExcludedTags))
			{
				continue;
			}

			RunningWeight += Entry.Weight;
			if (Selection <= RunningWeight)
			{
				// After choosing the item type, roll its quantity and cap additions at the overall item limit.
				const int32 Quantity = RandomStream.RandRange(FMath::Min(Entry.MinimumQuantity, Entry.MaximumQuantity), FMath::Max(Entry.MinimumQuantity, Entry.MaximumQuantity));
				for (int32 QuantityIndex = 0; QuantityIndex < Quantity && OutDefinitions.Num() < ItemLimit; ++QuantityIndex)
				{
					OutDefinitions.Add(Entry.ItemDefinition);
				}
				break;
			}
		}
	}
}
