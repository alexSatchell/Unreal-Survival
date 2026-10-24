#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Interactable.h"
#include "SurvivalItemTypes.h"
#include "Pickup.generated.h"

class USurvivalItemDefinition;

/** Represents a collectible world item that is permanently destroyed after a successful interaction. */
UCLASS()
class SURVIVAL_API APickup : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	APickup();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UBoxComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USurvivalItemDefinition> ItemDefinition;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Pickup")
	FGuid ItemInstanceId;

	UPROPERTY(ReplicatedUsing = OnRep_ItemDefinitionId, BlueprintReadOnly, Category = "Pickup")
	FPrimaryAssetId ItemDefinitionId;

	UPROPERTY(ReplicatedUsing = OnRep_PickupPresentation, BlueprintReadOnly, Category = "Pickup")
	FText ReplicatedDisplayName;

	UPROPERTY(ReplicatedUsing = OnRep_PickupPresentation, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UStaticMesh> ReplicatedPickupMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FVector PromptWorldOffset = FVector(0.0f, 0.0f, 50.0f);

	virtual void Interact_Implementation(AController* InteractingController) override;
	virtual FInteractionPromptData GetInteractionPrompt_Implementation(AController* InteractingController) const override;
	bool InitializeItemInstance(const FSurvivalItemInstance& InItemInstance, USurvivalItemDefinition* InItemDefinition);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_ItemDefinitionId();

	UFUNCTION()
	void OnRep_PickupPresentation();

	void ApplyItemDefinitionState(USurvivalItemDefinition* InItemDefinition);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
