#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Interactable.h"
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
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USurvivalItemDefinition> ItemDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = "1", UIMin = "1"))
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FVector PromptWorldOffset = FVector(0.0f, 0.0f, 50.0f);

	virtual void Interact_Implementation(AController* InteractingController) override;
	virtual FInteractionPromptData GetInteractionPrompt_Implementation(AController* InteractingController) const override;
};
