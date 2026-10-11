#include "SurvivalInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "SurvivalItemDefinition.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalInventory, Log, All);

USurvivalInventoryComponent::USurvivalInventoryComponent()
{
	SetIsReplicatedByDefault(true);
}

bool USurvivalInventoryComponent::TryAddItem(const FSurvivalItemInstance& InItemInstance, USurvivalItemDefinition* InItemDefinition)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !InItemInstance.IsValid() || !IsValid(InItemDefinition) || InItemDefinition->GetPrimaryAssetId() != InItemInstance.ItemDefinitionId)
	{
		return false;
	}

	FSurvivalItemInstance ExistingItem;
	if (FindItemByInstanceId(InItemInstance.InstanceId, ExistingItem))
	{
		return false;
	}

	Items.Add(InItemInstance);
	ServerItemDefinitions.Add(InItemInstance.InstanceId, InItemDefinition);
	CurrentWeight += InItemDefinition->Weight;

	const FString ItemName = InItemDefinition->DisplayName.IsEmpty() ? InItemDefinition->GetName() : InItemDefinition->DisplayName.ToString();
	UE_LOG(LogSurvivalInventory, Log, TEXT("Added %s to inventory of %s (%d items, %.2f weight)"), *ItemName, *GetOwner()->GetName(), Items.Num(), CurrentWeight);
	return true;
}

bool USurvivalInventoryComponent::FindItemByInstanceId(const FGuid& InInstanceId, FSurvivalItemInstance& OutItemInstance) const
{
	for (const FSurvivalItemInstance& Item : Items)
	{
		if (Item.InstanceId == InInstanceId)
		{
			OutItemInstance = Item;
			return true;
		}
	}

	return false;
}

USurvivalItemDefinition* USurvivalInventoryComponent::FindItemDefinitionByInstanceId(const FGuid& InInstanceId) const
{
	const TObjectPtr<USurvivalItemDefinition>* ItemDefinition = ServerItemDefinitions.Find(InInstanceId);
	return ItemDefinition ? ItemDefinition->Get() : nullptr;
}

bool USurvivalInventoryComponent::TryRemoveItem(const FGuid& InInstanceId, FSurvivalItemInstance& OutRemovedItem)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !InInstanceId.IsValid())
	{
		return false;
	}

	const int32 ItemIndex = Items.IndexOfByPredicate([&InInstanceId](const FSurvivalItemInstance& Item) { return Item.InstanceId == InInstanceId; });
	if (ItemIndex == INDEX_NONE)
	{
		return false;
	}

	OutRemovedItem = Items[ItemIndex];
	const USurvivalItemDefinition* ItemDefinition = FindItemDefinitionByInstanceId(InInstanceId);
	if (!ItemDefinition)
	{
		UE_LOG(LogSurvivalInventory, Error, TEXT("Rejected removal of item instance %s because its server definition is missing"), *InInstanceId.ToString());
		return false;
	}

	CurrentWeight = FMath::Max(0.0f, CurrentWeight - ItemDefinition->Weight);
	Items.RemoveAt(ItemIndex);
	ServerItemDefinitions.Remove(InInstanceId);
	const FString ItemName = ItemDefinition->DisplayName.IsEmpty() ? ItemDefinition->GetName() : ItemDefinition->DisplayName.ToString();
	UE_LOG(LogSurvivalInventory, Log, TEXT("Removed %s from inventory of %s (%d items, %.2f weight)"), *ItemName, *GetOwner()->GetName(), Items.Num(), CurrentWeight);
	return true;
}

void USurvivalInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(USurvivalInventoryComponent, Items, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(USurvivalInventoryComponent, CurrentWeight, COND_OwnerOnly);
}
