#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

/** Represents a collectible world item that can be claimed by only one player. */
UCLASS()
class SURVIVAL_API APickup : public AActor
{
	GENERATED_BODY()

public:
	APickup();

	UPROPERTY(ReplicatedUsing = OnRep_IsAvailable, BlueprintReadOnly, Category = "Pickup", meta = (DisplayName = "IsAvailable"))
	bool bIsAvailable = true;

	/** Attempts to claim the pickup on the authority and returns whether the claim succeeded. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool TryPickup();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_IsAvailable(const bool bPreviousIsAvailable) const;
};
