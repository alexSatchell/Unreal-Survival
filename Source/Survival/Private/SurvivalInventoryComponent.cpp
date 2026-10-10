#include "SurvivalInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "SurvivalItemDefinition.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalInventory, Log, All);

USurvivalInventoryComponent::USurvivalInventoryComponent()
{
	SetIsReplicatedByDefault(true);
}

bool USurvivalInventoryComponent::TryAddItem(USurvivalItemDefinition* InItemDefinition, int32 InQuantity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValid(InItemDefinition) || InQuantity <= 0 || InItemDefinition->MaxStackSize <= 0)
	{
		return false;
	}

	int32 RemainingQuantity = InQuantity;
	for (FSurvivalInventoryEntry& Entry : InventoryEntries)
	{
		if (Entry.ItemDefinition != InItemDefinition || Entry.Quantity >= InItemDefinition->MaxStackSize)
		{
			continue;
		}

		const int32 QuantityToAdd = FMath::Min(RemainingQuantity, InItemDefinition->MaxStackSize - Entry.Quantity);
		Entry.Quantity += QuantityToAdd;
		RemainingQuantity -= QuantityToAdd;
		if (RemainingQuantity == 0)
		{
			break;
		}
	}

	while (RemainingQuantity > 0)
	{
		FSurvivalInventoryEntry& NewEntry = InventoryEntries.AddDefaulted_GetRef();
		NewEntry.ItemDefinition = InItemDefinition;
		NewEntry.Quantity = FMath::Min(RemainingQuantity, InItemDefinition->MaxStackSize);
		RemainingQuantity -= NewEntry.Quantity;
	}

	const FString ItemName = InItemDefinition->DisplayName.IsEmpty() ? InItemDefinition->GetName() : InItemDefinition->DisplayName.ToString();
	UE_LOG(LogSurvivalInventory, Log, TEXT("Added %d x %s to inventory of %s (%d entries)"), InQuantity, *ItemName, *GetOwner()->GetName(), InventoryEntries.Num());
	return true;
}

void USurvivalInventoryComponent::OnRep_InventoryEntries()
{
}

void USurvivalInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(USurvivalInventoryComponent, InventoryEntries, COND_OwnerOnly);
}
