#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/SurvivalItemTypes.h"
#include "SurvivalInventoryComponent.generated.h"

class USurvivalItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSurvivalInventoryChanged);

/** Owns a character's server-authoritative carried-item state and replicates it to the owning client. */
UCLASS(ClassGroup = (Survival), meta = (BlueprintSpawnableComponent))
class SURVIVAL_API USurvivalInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivalInventoryComponent();

	bool TryAddItem(const FSurvivalItemInstance& InItemInstance, USurvivalItemDefinition* InItemDefinition);
	bool FindItemByInstanceId(const FGuid& InInstanceId, FSurvivalItemInstance& OutItemInstance) const;
	USurvivalItemDefinition* FindDefinitionAssetByInstanceId(const FGuid& InInstanceId) const;
	bool TryRemoveItem(const FGuid& InInstanceId, FSurvivalItemInstance& OutRemovedItem);

	UPROPERTY(ReplicatedUsing = OnRep_Items, BlueprintReadOnly, Category = "Inventory")
	TArray<FSurvivalItemInstance> Items;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
	float CurrentWeight = 0.0f;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnSurvivalInventoryChanged OnInventoryChanged;

private:
	/** Server-only definition-asset cache keyed by instance ID; shared data assets are not replicated for every carried item. */
	UPROPERTY(Transient)
	TMap<FGuid, TObjectPtr<USurvivalItemDefinition>> ServerDefinitionAssets;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Items();
};
