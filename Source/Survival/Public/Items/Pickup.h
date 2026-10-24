#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Interactable.h"
#include "Items/SurvivalItemTypes.h"
#include "Pickup.generated.h"

class USurvivalItemDefinition;
class USceneComponent;

/** Represents a collectible world item permanently destroyed after a successful interaction. */
UCLASS()
class SURVIVAL_API APickup : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	APickup();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UBoxComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USurvivalItemDefinition> ItemDefinition;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Pickup")
	FGuid ItemInstanceId;

	UPROPERTY(ReplicatedUsing = OnRep_DefinitionAssetId, BlueprintReadOnly, Category = "Pickup")
	FPrimaryAssetId DefinitionAssetId;

	UPROPERTY(ReplicatedUsing = OnRep_PickupPresentation, BlueprintReadOnly, Category = "Pickup")
	FText ReplicatedDisplayName;

	UPROPERTY(ReplicatedUsing = OnRep_PickupPresentation, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UStaticMesh> ReplicatedPickupMesh;

	UPROPERTY(ReplicatedUsing = OnRep_PickupPresentation, BlueprintReadOnly, Category = "Pickup|Interaction")
	TArray<FInteractionOptionData> ReplicatedInteractionOptions;

	UPROPERTY(ReplicatedUsing = OnRep_PickupPresentation, BlueprintReadOnly, Category = "Pickup|Interaction")
	FName ReplicatedDefaultActionId = NAME_None;

	/** Adds a small interaction targeting margin in centimeters; this does not affect floor placement. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Interaction", meta = (ClampMin = "0.0", UIMin = "0.0"))
	FVector CollisionPadding = FVector(0.5f);

	virtual void Interact_Implementation(AController* InteractingController) override;
	virtual void InteractWithAction_Implementation(AController* InteractingController, FName ActionId) override;
	virtual FInteractionPromptData GetInteractionPrompt_Implementation(AController* InteractingController) const override;
	bool InitializeItemInstance(const FSurvivalItemInstance& InItemInstance, USurvivalItemDefinition* InItemDefinition);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_DefinitionAssetId();

	UFUNCTION()
	void OnRep_PickupPresentation();

	USurvivalItemDefinition* GetItemDefinition() const;
	void ApplyItemPresentation(USurvivalItemDefinition* InItemDefinition);
	void UpdateCollisionBounds();
	bool TryStoreItem(AController* InteractingController);
	bool TryConsumeItem(AController* InteractingController);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
