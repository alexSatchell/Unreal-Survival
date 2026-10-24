#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SurvivalItemTypes.h"
#include "SurvivalInventoryComponent.generated.h"

class USurvivalItemDefinition;

/** Owns a character's server-authoritative carried-item state and replicates it to the owning client. */
UCLASS(ClassGroup = (Survival), meta = (BlueprintSpawnableComponent))
class SURVIVAL_API USurvivalInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivalInventoryComponent();

	bool TryAddItem(const FSurvivalItemInstance& InItemInstance, USurvivalItemDefinition* InItemDefinition);
	bool FindItemByInstanceId(const FGuid& InInstanceId, FSurvivalItemInstance& OutItemInstance) const;
	USurvivalItemDefinition* FindItemDefinitionByInstanceId(const FGuid& InInstanceId) const;
	bool TryRemoveItem(const FGuid& InInstanceId, FSurvivalItemInstance& OutRemovedItem);

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
	TArray<FSurvivalItemInstance> Items;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
	float CurrentWeight = 0.0f;

private:
	UPROPERTY(Transient)
	TMap<FGuid, TObjectPtr<USurvivalItemDefinition>> ServerItemDefinitions;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
