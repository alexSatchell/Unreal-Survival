#include "SurvivalLootTable.h"
#include "SurvivalItemDefinition.h"

void USurvivalLootTable::RollItems(const FGameplayTagContainer& ContextTags, FRandomStream& RandomStream, const int32 InMaximumItems, TArray<USurvivalItemDefinition*>& OutDefinitions) const
{
	OutDefinitions.Reset();
	const int32 ItemLimit = FMath::Min(FMath::Max(InMaximumItems, 0), FMath::Max(MaximumRolledItems, 0));
	for (int32 RollIndex = 0; RollIndex < ItemLimit; ++RollIndex)
	{
		float TotalWeight = 0.0f;
		for (const FSurvivalLootTableEntry& Entry : Entries)
		{
			if (IsValid(Entry.ItemDefinition) && Entry.Weight > 0.0f && ContextTags.HasAll(Entry.RequiredTags) && !ContextTags.HasAny(Entry.ExcludedTags))
			{
				TotalWeight += Entry.Weight;
			}
		}

		if (TotalWeight <= 0.0f || RandomStream.FRand() < FMath::Clamp(EmptyRollChance, 0.0f, 1.0f))
		{
			break;
		}

		const float Selection = RandomStream.FRandRange(0.0f, TotalWeight);
		float RunningWeight = 0.0f;
		for (const FSurvivalLootTableEntry& Entry : Entries)
		{
			if (!IsValid(Entry.ItemDefinition) || Entry.Weight <= 0.0f || !ContextTags.HasAll(Entry.RequiredTags) || ContextTags.HasAny(Entry.ExcludedTags))
			{
				continue;
			}

			RunningWeight += Entry.Weight;
			if (Selection <= RunningWeight)
			{
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
