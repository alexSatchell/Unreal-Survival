#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Interactable.h"
#include "Pickup.generated.h"

/** Represents a collectible world item that can be claimed by only one player. */
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

	UPROPERTY(ReplicatedUsing = OnRep_IsAvailable, BlueprintReadOnly, Category = "Pickup", meta = (DisplayName = "IsAvailable"))
	bool bIsAvailable = true;

	/** Attempts to claim the pickup on the authority and returns whether the claim succeeded. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool TryPickup();

	virtual void Interact_Implementation(AController* InteractingController) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_IsAvailable(const bool bPreviousIsAvailable) const;
};
