#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SurvivalInventoryComponent.generated.h"

class USurvivalItemDefinition;

USTRUCT(BlueprintType)
struct SURVIVAL_API FSurvivalInventoryEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<USurvivalItemDefinition> ItemDefinition;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Quantity = 0;
};

/** Owns a character's server-authoritative carried-item state and replicates it to the owning client. */
UCLASS(ClassGroup = (Survival), meta = (BlueprintSpawnableComponent))
class SURVIVAL_API USurvivalInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivalInventoryComponent();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool TryAddItem(USurvivalItemDefinition* InItemDefinition, int32 InQuantity = 1);

	UPROPERTY(ReplicatedUsing = OnRep_InventoryEntries, BlueprintReadOnly, Category = "Inventory")
	TArray<FSurvivalInventoryEntry> InventoryEntries;

protected:
	UFUNCTION()
	void OnRep_InventoryEntries();

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
